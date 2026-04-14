// TradeSubsystem.cpp — Player-to-player trading subsystem.
// Registers 11 socket event handlers via EventRouter.
// Inline STradeRequestPopup for incoming trade request notifications.

#include "TradeSubsystem.h"
#include "STradeWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "ChatSubsystem.h"
#include "InventorySubsystem.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogTrade);

// ============================================================
// RO Colors
// ============================================================
namespace TradeColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
}

// ============================================================
// Inline STradeRequestPopup
// ============================================================
class STradeRequestPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STradeRequestPopup) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UTradeSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwningSubsystem = InArgs._Subsystem;
		TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

		ChildSlot
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(300.f)
				[
					// 3-layer RO frame
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(TradeColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(TradeColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(TradeColors::PanelBrown)
							.Padding(FMargin(12.f))
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
								[
									SNew(STextBlock)
									.Text_Lambda([WeakSub]() -> FText {
										UTradeSubsystem* Sub = WeakSub.Get();
										if (!Sub) return FText::GetEmpty();
										return FText::FromString(FString::Printf(
											TEXT("%s wants to trade with you"), *Sub->RequestFromName));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(FSlateColor(TradeColors::TextPrimary))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(TradeColors::TextShadow)
									.Justification(ETextJustify::Center)
									.AutoWrapText(true)
								]

								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.Text(FText::FromString(TEXT("Accept")))
										.OnClicked_Lambda([WeakSub]() -> FReply {
											if (UTradeSubsystem* Sub = WeakSub.Get())
												Sub->AcceptRequest();
											return FReply::Handled();
										})
									]
									+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0, 0, 0)
									[
										SNew(SButton)
										.HAlign(HAlign_Center)
										.Text(FText::FromString(TEXT("Decline")))
										.OnClicked_Lambda([WeakSub]() -> FReply {
											if (UTradeSubsystem* Sub = WeakSub.Get())
												Sub->DeclineRequest();
											return FReply::Handled();
										})
									]
								]
							]
						]
					]
				]
			]
		];
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override
	{
		if (KeyEvent.GetKey() == EKeys::Escape)
		{
			if (UTradeSubsystem* Sub = OwningSubsystem.Get())
				Sub->DeclineRequest();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

private:
	TWeakObjectPtr<UTradeSubsystem> OwningSubsystem;
};

// ============================================================
// Helpers
// ============================================================

bool UTradeSubsystem::IsTrading() const
{
	return State != ETradeState::None
		&& State != ETradeState::Requesting
		&& State != ETradeState::RequestReceived;
}

void UTradeSubsystem::UnlockMovement()
{
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
			{
				if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
					CMC->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

// ============================================================
// JSON parsing
// ============================================================

static FTradeItem ParseTradeItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FTradeItem Item;
	if (!Obj) return Item;

	double d = 0;
	if (Obj->TryGetNumberField(TEXT("tradeSlot"), d)) Item.TradeSlot = (int32)d;
	if (Obj->TryGetNumberField(TEXT("inventoryId"), d)) Item.InventoryId = (int32)d;
	if (Obj->TryGetNumberField(TEXT("itemId"), d)) Item.ItemId = (int32)d;
	Obj->TryGetStringField(TEXT("name"), Item.Name);
	Obj->TryGetStringField(TEXT("icon"), Item.Icon);
	if (Obj->TryGetNumberField(TEXT("quantity"), d)) Item.Quantity = (int32)d;
	if (Obj->TryGetNumberField(TEXT("refineLevel"), d)) Item.RefineLevel = (int32)d;
	if (Obj->TryGetNumberField(TEXT("slots"), d)) Item.Slots = (int32)d;
	Obj->TryGetStringField(TEXT("compoundedCards"), Item.CompoundedCards);
	Obj->TryGetStringField(TEXT("itemType"), Item.ItemType);
	Obj->TryGetStringField(TEXT("equipSlot"), Item.EquipSlot);
	if (Obj->TryGetNumberField(TEXT("weight"), d)) Item.Weight = (int32)d;
	bool bIdent = true;
	if (Obj->TryGetBoolField(TEXT("identified"), bIdent)) Item.bIdentified = bIdent;
	Obj->TryGetStringField(TEXT("cardPrefix"), Item.CardPrefix);
	Obj->TryGetStringField(TEXT("cardSuffix"), Item.CardSuffix);
	if (Obj->TryGetNumberField(TEXT("weaponLevel"), d)) Item.WeaponLevel = (int32)d;

	return Item;
}

// ============================================================
// Lifecycle
// ============================================================

bool UTradeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UTradeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;
	LocalCharacterName = SelChar.Name;

	Router->RegisterHandler(TEXT("trade:request_received"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleRequestReceived(D); });
	Router->RegisterHandler(TEXT("trade:opened"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleOpened(D); });
	Router->RegisterHandler(TEXT("trade:item_added"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemAdded(D); });
	Router->RegisterHandler(TEXT("trade:item_removed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemRemoved(D); });
	Router->RegisterHandler(TEXT("trade:zeny_updated"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleZenyUpdated(D); });
	Router->RegisterHandler(TEXT("trade:partner_locked"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePartnerLocked(D); });
	Router->RegisterHandler(TEXT("trade:locks_reset"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleLocksReset(D); });
	Router->RegisterHandler(TEXT("trade:both_locked"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleBothLocked(D); });
	Router->RegisterHandler(TEXT("trade:completed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCompleted(D); });
	Router->RegisterHandler(TEXT("trade:cancelled"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCancelled(D); });
	Router->RegisterHandler(TEXT("trade:error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleError(D); });

	UE_LOG(LogTrade, Log, TEXT("TradeSubsystem initialized - 11 event handlers registered"));
}

void UTradeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RequestTimeoutHandle);

		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
				Router->UnregisterAllForOwner(this);
		}
	}

	HideTradeWidget();
	HideRequestPopup();
	ResetTradeState();

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

void UTradeSubsystem::RequestTrade(int32 TargetCharacterId)
{
	if (State != ETradeState::None)
	{
		if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
			Chat->AddCombatLogMessage(TEXT("You are already in a trade."));
		return;
	}

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("targetCharacterId"), TargetCharacterId);
	GI->EmitSocketEvent(TEXT("trade:request"), Payload);

	State = ETradeState::Requesting;
	PartnerCharacterId = TargetCharacterId;
	++DataVersion;

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(TEXT("Trade request sent..."));

	UE_LOG(LogTrade, Log, TEXT("Sent trade:request to character %d"), TargetCharacterId);
}

void UTradeSubsystem::AcceptRequest()
{
	if (State != ETradeState::RequestReceived) return;

	UAudioSubsystem::PlayUIClickStatic(GetWorld());

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("requesterId"), RequestFromCharId);
	GI->EmitSocketEvent(TEXT("trade:accept"), Payload);

	HideRequestPopup();
	UE_LOG(LogTrade, Log, TEXT("Accepted trade from character %d"), RequestFromCharId);
}

void UTradeSubsystem::DeclineRequest()
{
	if (State != ETradeState::RequestReceived) return;

	UAudioSubsystem::PlayUICancelStatic(GetWorld());

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("requesterId"), RequestFromCharId);
	GI->EmitSocketEvent(TEXT("trade:decline"), Payload);

	HideRequestPopup();
	ResetTradeState();
	UE_LOG(LogTrade, Log, TEXT("Declined trade from character %d"), RequestFromCharId);
}

void UTradeSubsystem::AddItem(int32 InventoryId, int32 Quantity)
{
	if (State != ETradeState::Open) return;
	if (bMyLocked) return;
	if (MyItems.Num() >= MAX_TRADE_ITEMS) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("trade:add_item"), Payload);
}

void UTradeSubsystem::RemoveItem(int32 TradeSlot)
{
	if (State != ETradeState::Open) return;
	if (bMyLocked) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("tradeSlot"), TradeSlot);
	GI->EmitSocketEvent(TEXT("trade:remove_item"), Payload);
}

void UTradeSubsystem::SetZeny(int64 Amount)
{
	if (State != ETradeState::Open) return;
	if (bMyLocked) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("amount"), (double)Amount);
	GI->EmitSocketEvent(TEXT("trade:set_zeny"), Payload);
}

void UTradeSubsystem::Lock()
{
	if (State != ETradeState::Open) return;
	if (bMyLocked) return;

	UAudioSubsystem::PlayUIClickStatic(GetWorld());

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("trade:lock"), TEXT("{}"));
	bMyLocked = true;
	State = ETradeState::Locked;
	++DataVersion;
	UE_LOG(LogTrade, Log, TEXT("Locked trade offer"));
}

void UTradeSubsystem::Confirm()
{
	if (State != ETradeState::BothLocked) return;

	UAudioSubsystem::PlayUIClickStatic(GetWorld());

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("trade:confirm"), TEXT("{}"));
	State = ETradeState::Confirmed;
	++DataVersion;
	UE_LOG(LogTrade, Log, TEXT("Confirmed trade"));
}

void UTradeSubsystem::Cancel()
{
	UAudioSubsystem::PlayUICancelStatic(GetWorld());

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("trade:cancel"), TEXT("{}"));
	HideTradeWidget();
	UnlockMovement();
	ResetTradeState();
	UE_LOG(LogTrade, Log, TEXT("Cancelled trade"));
}

// ============================================================
// Socket event handlers
// ============================================================

void UTradeSubsystem::HandleRequestReceived(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double d = 0;
	if (Obj->TryGetNumberField(TEXT("fromCharacterId"), d))
		RequestFromCharId = (int32)d;
	Obj->TryGetStringField(TEXT("fromName"), RequestFromName);

	// Auto-decline if option enabled
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (GI->bOptionAutoDeclineTrades)
		{
			UE_LOG(LogTrade, Log, TEXT("Auto-declined trade from %s (option enabled)"), *RequestFromName);
			State = ETradeState::RequestReceived;
			DeclineRequest();
			return;
		}
	}

	State = ETradeState::RequestReceived;
	++DataVersion;
	ShowRequestPopup();

	// Auto-dismiss after 30 seconds (match server timeout)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RequestTimeoutHandle);
		TWeakObjectPtr<UTradeSubsystem> WeakThis(this);
		World->GetTimerManager().SetTimer(RequestTimeoutHandle, [WeakThis]()
		{
			if (UTradeSubsystem* Self = WeakThis.Get())
			{
				if (Self->State == ETradeState::RequestReceived)
					Self->DeclineRequest();
			}
		}, 30.f, false);
	}

	UE_LOG(LogTrade, Log, TEXT("Trade request from %s (char %d)"), *RequestFromName, RequestFromCharId);
}

void UTradeSubsystem::HandleOpened(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double d = 0;
	if (Obj->TryGetNumberField(TEXT("partnerId"), d))
		PartnerCharacterId = (int32)d;
	Obj->TryGetStringField(TEXT("partnerName"), PartnerName);

	State = ETradeState::Open;
	MyItems.Empty();
	PartnerItems.Empty();
	MyZeny = 0;
	PartnerZeny = 0;
	bMyLocked = false;
	bPartnerLocked = false;
	++DataVersion;

	HideRequestPopup();
	ShowTradeWidget();

	// Lock movement during trade (RO Classic behavior)
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
			{
				if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
					CMC->DisableMovement();
			}
		}
	}

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(FString::Printf(TEXT("Trading with %s"), *PartnerName));

	UE_LOG(LogTrade, Log, TEXT("Trade opened with %s (char %d)"), *PartnerName, PartnerCharacterId);
}

void UTradeSubsystem::HandleItemAdded(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Side;
	Obj->TryGetStringField(TEXT("side"), Side);

	const TSharedPtr<FJsonObject>* ItemObjPtr = nullptr;
	if (!Obj->TryGetObjectField(TEXT("item"), ItemObjPtr) || !ItemObjPtr) return;
	FTradeItem Item = ParseTradeItemFromJson(*ItemObjPtr);

	if (Side == TEXT("my"))
		MyItems.Add(Item);
	else
		PartnerItems.Add(Item);

	++DataVersion;
	UE_LOG(LogTrade, Log, TEXT("Item added to %s side: %s x%d"), *Side, *Item.Name, Item.Quantity);
}

void UTradeSubsystem::HandleItemRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Side;
	Obj->TryGetStringField(TEXT("side"), Side);
	double d = 0;
	int32 TradeSlot = -1;
	if (Obj->TryGetNumberField(TEXT("tradeSlot"), d)) TradeSlot = (int32)d;

	TArray<FTradeItem>& Items = (Side == TEXT("my")) ? MyItems : PartnerItems;
	Items.RemoveAll([TradeSlot](const FTradeItem& It) { return It.TradeSlot == TradeSlot; });

	++DataVersion;
}

void UTradeSubsystem::HandleZenyUpdated(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Side;
	Obj->TryGetStringField(TEXT("side"), Side);
	double d = 0;
	if (Obj->TryGetNumberField(TEXT("amount"), d))
	{
		if (Side == TEXT("my"))
			MyZeny = (int64)d;
		else
			PartnerZeny = (int64)d;
	}
	++DataVersion;
}

void UTradeSubsystem::HandlePartnerLocked(const TSharedPtr<FJsonValue>& Data)
{
	bPartnerLocked = true;
	++DataVersion;

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(FString::Printf(TEXT("%s has locked their offer."), *PartnerName));
}

void UTradeSubsystem::HandleLocksReset(const TSharedPtr<FJsonValue>& Data)
{
	bMyLocked = false;
	bPartnerLocked = false;
	State = ETradeState::Open;
	++DataVersion;

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(TEXT("Trade offer was modified - locks reset."));
}

void UTradeSubsystem::HandleBothLocked(const TSharedPtr<FJsonValue>& Data)
{
	bMyLocked = true;
	bPartnerLocked = true;
	State = ETradeState::BothLocked;
	++DataVersion;

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(TEXT("Both offers locked. Click Trade to finalize."));
}

void UTradeSubsystem::HandleCompleted(const TSharedPtr<FJsonValue>& Data)
{
	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(FString::Printf(TEXT("Trade with %s completed successfully."), *PartnerName));

	HideTradeWidget();
	UnlockMovement();
	ResetTradeState();
}

void UTradeSubsystem::HandleCancelled(const TSharedPtr<FJsonValue>& Data)
{
	FString Reason = TEXT("Trade cancelled.");
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
			(*ObjPtr)->TryGetStringField(TEXT("reason"), Reason);
	}

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(Reason);

	HideTradeWidget();
	HideRequestPopup();
	UnlockMovement();
	ResetTradeState();
}

void UTradeSubsystem::HandleError(const TSharedPtr<FJsonValue>& Data)
{
	FString Message = TEXT("Trade error.");
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
			(*ObjPtr)->TryGetStringField(TEXT("message"), Message);
	}

	if (UChatSubsystem* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->AddCombatLogMessage(Message);

	// Reset if we were just requesting
	if (State == ETradeState::Requesting)
		ResetTradeState();

	UE_LOG(LogTrade, Warning, TEXT("Trade error: %s"), *Message);
}

// ============================================================
// Widget lifecycle
// ============================================================

void UTradeSubsystem::ShowTradeWidget()
{
	HideTradeWidget();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	TradeWidget = SNew(STradeWidget).Subsystem(this);

	TradeAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			TradeWidget.ToSharedRef()
		];

	TradeOverlay = SNew(SWeakWidget).PossiblyNullContent(TradeAlignWrapper);
	VC->AddViewportWidgetContent(TradeOverlay.ToSharedRef(), 22);
	bTradeWidgetVisible = true;

	FSlateApplication::Get().SetKeyboardFocus(TradeWidget);
	UE_LOG(LogTrade, Log, TEXT("Trade widget shown (Z=22)"));
}

void UTradeSubsystem::HideTradeWidget()
{
	if (!bTradeWidgetVisible) return;

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (TradeOverlay.IsValid())
				VC->RemoveViewportWidgetContent(TradeOverlay.ToSharedRef());
		}
	}

	TradeWidget.Reset();
	TradeAlignWrapper.Reset();
	TradeOverlay.Reset();
	bTradeWidgetVisible = false;
}

void UTradeSubsystem::ShowRequestPopup()
{
	HideRequestPopup();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	RequestPopupWidget = SNew(STradeRequestPopup).Subsystem(this);

	RequestAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			RequestPopupWidget.ToSharedRef()
		];

	RequestOverlay = SNew(SWeakWidget).PossiblyNullContent(RequestAlignWrapper);
	VC->AddViewportWidgetContent(RequestOverlay.ToSharedRef(), 30);
	bRequestPopupVisible = true;

	FSlateApplication::Get().SetKeyboardFocus(RequestPopupWidget);
	UE_LOG(LogTrade, Log, TEXT("Trade request popup shown (Z=30)"));
}

void UTradeSubsystem::HideRequestPopup()
{
	if (!bRequestPopupVisible) return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RequestTimeoutHandle);

		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (RequestOverlay.IsValid())
				VC->RemoveViewportWidgetContent(RequestOverlay.ToSharedRef());
		}
	}

	RequestPopupWidget.Reset();
	RequestAlignWrapper.Reset();
	RequestOverlay.Reset();
	bRequestPopupVisible = false;
}

void UTradeSubsystem::ResetTradeState()
{
	State = ETradeState::None;
	PartnerCharacterId = 0;
	PartnerName.Empty();
	MyItems.Empty();
	PartnerItems.Empty();
	MyZeny = 0;
	PartnerZeny = 0;
	bMyLocked = false;
	bPartnerLocked = false;
	RequestFromCharId = 0;
	RequestFromName.Empty();
	++DataVersion;
}
