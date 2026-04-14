// ChatSubsystem.cpp — Chat message log, chat:receive handler, and SChatWidget.
// Phase E of BP_SocketManager bridge migration.

#include "ChatSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"
#include "BuffBarSubsystem.h"
#include "ZoneTransitionSubsystem.h"
#include "OtherPlayerSubsystem.h"
#include "TradeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogChat, Log, All);

// ============================================================
// RO Classic Chat Colors
// ============================================================

// Chat filter tabs
enum class EChatTab : uint8
{
	All,
	System,
	Combat
};

namespace ChatColors
{
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 0.90f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDivider  (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor TextShadow   (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor InputBg      (0.12f, 0.08f, 0.04f, 1.f);
	static const FLinearColor TabActive    (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor TabInactive  (0.28f, 0.18f, 0.10f, 1.f);

	// Channel message colors
	static const FLinearColor Normal       (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor Party        (0.33f, 0.60f, 1.0f, 1.f);
	static const FLinearColor Guild        (0.33f, 1.0f, 0.33f, 1.f);
	static const FLinearColor Whisper      (1.0f, 0.85f, 0.0f, 1.f);  // Yellow/Gold (RO Classic canonical)
	static const FLinearColor System       (1.0f, 1.0f, 0.0f, 1.f);
}

// ============================================================
// SChatWidget — Slate chat window (inline)
// ============================================================

class SChatWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SChatWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UChatSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwningSubsystem = InArgs._Subsystem;

		ChildSlot
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(380.f)
			.HeightOverride(220.f)
			[
				// 3-layer RO frame: Gold → Dark → Brown
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ChatColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ChatColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor_Lambda([this]() -> FSlateColor {
							float Alpha = 0.90f;
							if (UChatSubsystem* S = OwningSubsystem.Get())
								Alpha = S->ChatPanelOpacity;
							return FSlateColor(FLinearColor(0.43f, 0.29f, 0.17f, Alpha));
						})
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)

							// ---- Title bar ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(ChatColors::PanelDark)
								.Padding(FMargin(6.f, 3.f))
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Chat")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
									.ColorAndOpacity(FSlateColor(ChatColors::GoldHighlight))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(ChatColors::TextShadow)
								]
							]

							// ---- Tab bar ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(ChatColors::PanelDark)
								.Padding(FMargin(4.f, 2.f))
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f)
									[ BuildTabButton(TEXT("All"), EChatTab::All) ]
									+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f)
									[ BuildTabButton(TEXT("System"), EChatTab::System) ]
									+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f)
									[ BuildTabButton(TEXT("Combat"), EChatTab::Combat) ]
								]
							]

							// ---- Gold divider ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(1.f)
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(ChatColors::GoldDivider)
								]
							]

							// ---- Message scroll area ----
							+ SVerticalBox::Slot().FillHeight(1.f).Padding(FMargin(4.f, 2.f))
							[
								SAssignNew(ScrollBox, SScrollBox)
								.ScrollBarAlwaysVisible(false)
							]

							// ---- Gold divider ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(1.f)
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(ChatColors::GoldDivider)
								]
							]

							// ---- Input field ----
							+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(4.f, 3.f))
							[
								SAssignNew(InputField, SEditableTextBox)
								.HintText(FText::FromString(TEXT("Press Enter to chat...")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ForegroundColor(ChatColors::Normal)
								.BackgroundColor(ChatColors::InputBg)
								.OnTextCommitted(FOnTextCommitted::CreateSP(this, &SChatWidget::OnInputCommitted))
							]
						]
					]
				]
			]
		];

		ApplyLayout();
	}

	void FocusInput()
	{
		if (InputField.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetKeyboardFocus(InputField);
		}
	}

	void SetInputText(const FString& Text)
	{
		if (InputField.IsValid())
		{
			InputField->SetText(FText::FromString(Text));
		}
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		UChatSubsystem* Sub = OwningSubsystem.Get();
		if (!Sub || !ScrollBox.IsValid()) return;

		const bool bTabChanged = (ActiveTab != LastRenderedTab);
		const bool bNewMessages = (Sub->MessageVersion != LastRenderedVersion);

		if (bTabChanged || bNewMessages)
		{
			RebuildMessages();
			LastRenderedTab = ActiveTab;
			LastRenderedVersion = Sub->MessageVersion;
		}
	}

	// ---- Drag support (title bar only) ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

		// Title bar is ~20px tall
		if (LocalPos.Y < 20.f)
		{
			bIsDragging = true;
			DragOffset = MouseEvent.GetScreenSpacePosition();
			DragStartWidgetPos = WidgetPosition;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (bIsDragging)
		{
			bIsDragging = false;
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (bIsDragging)
		{
			const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
			const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
				? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
			WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
			ApplyLayout();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

private:
	void OnInputCommitted(const FText& Text, ETextCommit::Type CommitType)
	{
		if (CommitType != ETextCommit::OnEnter) return;

		if (!Text.IsEmpty())
		{
			UChatSubsystem* Sub = OwningSubsystem.Get();
			if (Sub)
			{
				Sub->SendChatMessage(Text.ToString());
			}
		}

		// Clear input and return focus to the game
		if (InputField.IsValid())
		{
			InputField->SetText(FText::GetEmpty());
		}
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}

	TSharedRef<SWidget> BuildTabButton(const FString& Label, EChatTab Tab)
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, Tab]()
			{
				return (ActiveTab == Tab) ? ChatColors::TabActive : ChatColors::TabInactive;
			})
			.Padding(FMargin(8.f, 2.f))
			.Cursor(EMouseCursor::Hand)
			.OnMouseButtonDown_Lambda([this, Tab](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					ActiveTab = Tab;
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity_Lambda([this, Tab]() -> FSlateColor
				{
					return (ActiveTab == Tab)
						? FSlateColor(ChatColors::GoldHighlight)
						: FSlateColor(ChatColors::Normal);
				})
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ChatColors::TextShadow)
			];
	}

	bool PassesFilter(const FChatMessage& Msg) const
	{
		switch (ActiveTab)
		{
		case EChatTab::All:     return true;
		case EChatTab::System:  return Msg.Channel == EChatChannel::System;
		case EChatTab::Combat:  return Msg.Channel == EChatChannel::Combat;
		default:                return true;
		}
	}

	void RebuildMessages()
	{
		if (!ScrollBox.IsValid()) return;
		ScrollBox->ClearChildren();

		UChatSubsystem* Sub = OwningSubsystem.Get();
		if (!Sub) return;

		for (const FChatMessage& Msg : Sub->Messages)
		{
			if (!PassesFilter(Msg)) continue;
			AddMessageWidget(Msg);
		}

		ScrollBox->ScrollToEnd();
	}

	void AddMessageWidget(const FChatMessage& Msg)
	{
		if (!ScrollBox.IsValid()) return;

		// Timestamp prefix if enabled
		FString TimePrefix;
		UChatSubsystem* Sub = OwningSubsystem.Get();
		if (Sub && Sub->bShowTimestamps && Msg.Timestamp > 0.0)
		{
			FDateTime DT(static_cast<int64>(Msg.Timestamp));
			TimePrefix = FString::Printf(TEXT("[%02d:%02d] "), DT.GetHour(), DT.GetMinute());
		}

		FString DisplayText;
		if (Msg.Channel == EChatChannel::System)
		{
			DisplayText = FString::Printf(TEXT("[System] %s"), *Msg.Message);
		}
		else if (Msg.Channel == EChatChannel::Combat)
		{
			DisplayText = FString::Printf(TEXT("[Combat] %s"), *Msg.Message);
		}
		else if (Msg.Channel == EChatChannel::Whisper)
		{
			// Whisper messages already formatted as "From Name : msg" or "To Name : msg"
			DisplayText = Msg.Message;
		}
		else
		{
			FString ChannelPrefix;
			if (Msg.Channel == EChatChannel::Party) ChannelPrefix = TEXT("[Party] ");
			else if (Msg.Channel == EChatChannel::Guild) ChannelPrefix = TEXT("[Guild] ");

			DisplayText = FString::Printf(TEXT("%s%s: %s"), *ChannelPrefix, *Msg.SenderName, *Msg.Message);
		}

		DisplayText = TimePrefix + DisplayText;
		const FLinearColor MsgColor = UChatSubsystem::GetChannelColor(Msg.Channel);

		ScrollBox->AddSlot()
		[
			SNew(STextBlock)
			.Text(FText::FromString(DisplayText))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor(MsgColor))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(ChatColors::TextShadow)
			.AutoWrapText(true)
		];
	}

	void ApplyLayout()
	{
		const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
		SetRenderTransform(FSlateRenderTransform(Pos));
	}

	TWeakObjectPtr<UChatSubsystem> OwningSubsystem;
	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SEditableTextBox> InputField;

	// Tab filtering
	EChatTab ActiveTab = EChatTab::All;
	EChatTab LastRenderedTab = EChatTab::All;
	uint32 LastRenderedVersion = 0;

	// Drag state
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(10.0, -10.0);
};

// ============================================================
// UChatSubsystem — Lifecycle
// ============================================================

bool UChatSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UChatSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;
	LocalCharacterName = SelChar.Name;

	// Register for chat + combat log events via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("chat:receive"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleChatReceive(D); });
		Router->RegisterHandler(TEXT("chat:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleChatError(D); });

		// Combat log events
		Router->RegisterHandler(TEXT("combat:damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("skill:effect_damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillEffectDamage(D); });
		Router->RegisterHandler(TEXT("status:applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusApplied(D); });
		Router->RegisterHandler(TEXT("status:removed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusRemoved(D); });
		Router->RegisterHandler(TEXT("skill:buff_applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleBuffApplied(D); });
		Router->RegisterHandler(TEXT("skill:buff_removed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleBuffRemoved(D); });
		Router->RegisterHandler(TEXT("combat:death"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDeath(D); });
		Router->RegisterHandler(TEXT("combat:respawn"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatRespawn(D); });

		// EXP gain events
		Router->RegisterHandler(TEXT("exp:gain"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleExpGain(D); });
	}

	if (GI->IsSocketConnected())
	{
		ShowWidget();
	}

	UE_LOG(LogChat, Log, TEXT("ChatSubsystem started — chat:receive registered. LocalChar=%s"), *LocalCharacterName);
}

void UChatSubsystem::Deinitialize()
{
	HideWidget();

	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	Super::Deinitialize();
}

// ============================================================
// Event handler
// ============================================================

void UChatSubsystem::HandleChatReceive(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FChatMessage Msg;
	Obj->TryGetStringField(TEXT("senderName"), Msg.SenderName);
	Obj->TryGetStringField(TEXT("message"), Msg.Message);

	FString ChannelStr;
	Obj->TryGetStringField(TEXT("channel"), ChannelStr);
	Msg.Channel = ParseChannel(ChannelStr);
	Msg.Timestamp = FDateTime::Now().GetTicks();

	// Whisper: format per official RO msgstringtable — "( From Name: ) message"
	if (ChannelStr.ToUpper() == TEXT("WHISPER"))
	{
		// Track last whisperer for /r reply BEFORE clearing SenderName
		FString OrigSender;
		Obj->TryGetStringField(TEXT("senderName"), OrigSender);
		if (!OrigSender.IsEmpty()) LastWhisperer = OrigSender;
		Msg.Message = FString::Printf(TEXT("( From %s: ) %s"), *OrigSender, *Msg.Message);
		Msg.SenderName = TEXT("");  // Clear sender — message already contains the name

		// Whisper alert chime — RO Classic uses the universal button click sound
		// (버튼소리.wav) as the audio cue for incoming whispers.
		UAudioSubsystem::PlayUIClickStatic(GetWorld());
	}
	else if (ChannelStr.ToUpper() == TEXT("WHISPER_SENT"))
	{
		Msg.Message = FString::Printf(TEXT("( To %s: ) %s"), *Msg.SenderName, *Msg.Message);
		Msg.SenderName = TEXT("");
	}

	// Skip own messages — SendChatMessage() already added the local echo
	if (Msg.Channel == EChatChannel::Normal && !Msg.SenderName.IsEmpty() && Msg.SenderName == LocalCharacterName)
	{
		return;
	}

	// Trim old messages
	if (Messages.Num() >= MAX_MESSAGES)
	{
		Messages.RemoveAt(0);
	}
	Messages.Add(MoveTemp(Msg));
	++MessageVersion;

	UE_LOG(LogChat, Verbose, TEXT("Chat [%s] %s: %s"), *ChannelStr, *Msg.SenderName, *Msg.Message);
}

void UChatSubsystem::HandleChatError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString ErrorMsg;
	(*ObjPtr)->TryGetStringField(TEXT("message"), ErrorMsg);
	if (ErrorMsg.IsEmpty()) return;

	FChatMessage Msg;
	Msg.SenderName = TEXT("");
	Msg.Message = ErrorMsg;
	Msg.Channel = EChatChannel::System;
	Msg.Timestamp = FDateTime::Now().GetTicks();

	if (Messages.Num() >= MAX_MESSAGES) Messages.RemoveAt(0);
	Messages.Add(MoveTemp(Msg));
	++MessageVersion;
}

// ============================================================
// Combat log helper
// ============================================================

void UChatSubsystem::AddCombatLogMessage(const FString& Message)
{
	FChatMessage Msg;
	Msg.SenderName = TEXT("SYSTEM");
	Msg.Message = Message;
	Msg.Channel = EChatChannel::Combat;
	Msg.Timestamp = FDateTime::Now().GetTicks();

	if (Messages.Num() >= MAX_MESSAGES)
	{
		Messages.RemoveAt(0);
	}
	Messages.Add(MoveTemp(Msg));
	++MessageVersion;
}

// ============================================================
// Combat log handlers
// ============================================================

void UChatSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double AttackerIdD = 0, TargetIdD = 0, DamageD = 0, Damage2D = 0;
	FString AttackerName, TargetName, HitType;
	bool bIsEnemy = false, bIsCritical = false, bIsMiss = false, bIsDualWield = false;

	Obj->TryGetNumberField(TEXT("attackerId"), AttackerIdD);
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("damage"), DamageD);
	Obj->TryGetNumberField(TEXT("damage2"), Damage2D);
	Obj->TryGetStringField(TEXT("attackerName"), AttackerName);
	Obj->TryGetStringField(TEXT("targetName"), TargetName);
	Obj->TryGetStringField(TEXT("hitType"), HitType);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetBoolField(TEXT("isCritical"), bIsCritical);
	Obj->TryGetBoolField(TEXT("isMiss"), bIsMiss);
	Obj->TryGetBoolField(TEXT("isDualWield"), bIsDualWield);

	const int32 AttackerId = (int32)AttackerIdD;
	const int32 Damage = (int32)DamageD;
	const int32 Damage2 = (int32)Damage2D;

	if (bIsMiss || HitType == TEXT("miss") || HitType == TEXT("dodge") || HitType == TEXT("perfectDodge"))
	{
		if (AttackerId == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("Your attack missed %s"), *TargetName));
		else if ((int32)TargetIdD == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("%s's attack missed you"), *AttackerName));
		return;
	}

	// Element modifier (optional — sent when attack or target is non-neutral)
	double EleModD = 0;
	Obj->TryGetNumberField(TEXT("eleMod"), EleModD);
	const int32 EleMod = (int32)EleModD;

	// Magnum Break fire endow breakdown (optional)
	double FireBonusDmgD = 0, FireEleModD = 0;
	Obj->TryGetNumberField(TEXT("fireBonusDmg"), FireBonusDmgD);
	Obj->TryGetNumberField(TEXT("fireEleMod"), FireEleModD);
	const int32 FireBonusDmg = (int32)FireBonusDmgD;
	const int32 FireEleMod = (int32)FireEleModD;

	// Element name + effectiveness text helper
	FString Element;
	Obj->TryGetStringField(TEXT("element"), Element);

	auto GetEffectivenessText = [](int32 Mod) -> FString {
		if (Mod >= 200)      return TEXT("super effective");
		if (Mod >= 150)      return TEXT("very effective");
		if (Mod > 100)       return TEXT("effective");
		if (Mod == 100)      return TEXT("neutral");
		if (Mod > 50)        return TEXT("partially resisted");
		if (Mod > 0)         return TEXT("heavily resisted");
		return TEXT("immune");
	};

	if (AttackerId == LocalCharacterId)
	{
		const int32 BaseDmg = (FireBonusDmg > 0) ? (Damage - FireBonusDmg) : Damage;
		FString Msg = FString::Printf(TEXT("You hit %s for %d damage"), *TargetName, BaseDmg);
		if (bIsCritical) Msg += TEXT(" (Critical!)");

		// Element effectiveness on main attack (non-neutral only)
		if (EleMod > 0 && EleMod != 100 && !Element.IsEmpty() && Element != TEXT("neutral"))
			Msg += FString::Printf(TEXT(" [%s — %s]"), *Element, *GetEffectivenessText(EleMod));

		if (bIsDualWield && Damage2 > 0)
			Msg += FString::Printf(TEXT(" + %d (Left Hand)"), Damage2);

		// Magnum Break fire bonus (separate from main element)
		if (FireBonusDmg > 0)
			Msg += FString::Printf(TEXT(" + %d fire (%s)"), FireBonusDmg, *GetEffectivenessText(FireEleMod));

		AddCombatLogMessage(Msg);
	}
	else if ((int32)TargetIdD == LocalCharacterId || (!bIsEnemy && (int32)TargetIdD == LocalCharacterId))
	{
		// Something attacked the player
		AddCombatLogMessage(FString::Printf(TEXT("%s hits you for %d damage"), *AttackerName, Damage));
	}
}

void UChatSubsystem::HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double AttackerIdD = 0, TargetIdD = 0, DamageD = 0, HealAmountD = 0;
	FString SkillName, HitType, AttackerName, TargetName;
	bool bIsEnemy = false;

	Obj->TryGetNumberField(TEXT("attackerId"), AttackerIdD);
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("damage"), DamageD);
	Obj->TryGetNumberField(TEXT("healAmount"), HealAmountD);
	Obj->TryGetStringField(TEXT("skillName"), SkillName);
	Obj->TryGetStringField(TEXT("hitType"), HitType);
	Obj->TryGetStringField(TEXT("attackerName"), AttackerName);
	Obj->TryGetStringField(TEXT("targetName"), TargetName);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	const int32 AttackerId = (int32)AttackerIdD;
	const int32 TargetId = (int32)TargetIdD;
	const int32 Damage = (int32)DamageD;
	const int32 HealAmount = (int32)HealAmountD;

	// Heal
	if (HitType == TEXT("heal") && HealAmount > 0)
	{
		if (AttackerId == LocalCharacterId && TargetId == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("You heal yourself for %d HP (%s)"), HealAmount, *SkillName));
		else if (AttackerId == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("You heal %s for %d HP (%s)"), *TargetName, HealAmount, *SkillName));
		else if (TargetId == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("%s heals you for %d HP (%s)"), *AttackerName, HealAmount, *SkillName));
		return;
	}

	// Miss
	if (HitType == TEXT("miss") || HitType == TEXT("dodge"))
	{
		if (AttackerId == LocalCharacterId)
			AddCombatLogMessage(FString::Printf(TEXT("Your %s missed %s"), *SkillName, *TargetName));
		return;
	}

	// Element effectiveness (auto-enriched by server for all skill:effect_damage)
	double EleModD = 0;
	Obj->TryGetNumberField(TEXT("eleMod"), EleModD);
	const int32 EleMod = (int32)EleModD;
	FString Element;
	Obj->TryGetStringField(TEXT("element"), Element);

	auto GetEffText = [](int32 Mod) -> FString {
		if (Mod >= 200)      return TEXT("super effective");
		if (Mod >= 150)      return TEXT("very effective");
		if (Mod > 100)       return TEXT("effective");
		if (Mod == 100)      return TEXT("neutral");
		if (Mod > 50)        return TEXT("partially resisted");
		if (Mod > 0)         return TEXT("heavily resisted");
		return TEXT("immune");
	};

	// Auto-Blitz Beat (falcon proc) — show distinctly from manual skills
	bool bIsAutoBlitz = false;
	Obj->TryGetBoolField(TEXT("isAutoBlitz"), bIsAutoBlitz);

	// Damage
	if (Damage > 0)
	{
		if (AttackerId == LocalCharacterId)
		{
			FString Msg;
			if (bIsAutoBlitz)
				Msg = FString::Printf(TEXT("Your falcon strikes %s for %d damage"), *TargetName, Damage);
			else
				Msg = FString::Printf(TEXT("Your %s hits %s for %d damage"), *SkillName, *TargetName, Damage);
			if (HitType == TEXT("critical")) Msg += TEXT(" (Critical!)");
			if (EleMod > 0 && EleMod != 100 && !Element.IsEmpty() && Element != TEXT("neutral"))
				Msg += FString::Printf(TEXT(" [%s — %s]"), *Element, *GetEffText(EleMod));
			AddCombatLogMessage(Msg);
		}
		else if (TargetId == LocalCharacterId)
		{
			if (bIsAutoBlitz)
				AddCombatLogMessage(FString::Printf(TEXT("%s's falcon strikes you for %d damage"), *AttackerName, Damage));
			else
				AddCombatLogMessage(FString::Printf(TEXT("%s's %s hits you for %d damage"), *AttackerName, *SkillName, Damage));
		}
	}
}

void UChatSubsystem::HandleStatusApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	bool bIsEnemy = false;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	FString StatusType;
	double DurationD = 0;
	Obj->TryGetStringField(TEXT("statusType"), StatusType);
	Obj->TryGetNumberField(TEXT("duration"), DurationD);

	FString DisplayName = StatusType;
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

	if (bIsEnemy)
	{
		// Show combat log when WE applied a status effect to an enemy (e.g. "Poring is poisoned")
		double SourceIdD = 0;
		Obj->TryGetNumberField(TEXT("sourceId"), SourceIdD);
		if (LocalCharacterId > 0 && (int32)SourceIdD == LocalCharacterId)
		{
			FString TargetName;
			Obj->TryGetStringField(TEXT("targetName"), TargetName);
			if (TargetName.IsEmpty()) TargetName = FString::Printf(TEXT("Enemy %d"), (int32)TargetIdD);
			if (DurationD > 0)
				AddCombatLogMessage(FString::Printf(TEXT("%s is afflicted with %s (%.1fs)"), *TargetName, *DisplayName, DurationD / 1000.0));
			else
				AddCombatLogMessage(FString::Printf(TEXT("%s is afflicted with %s"), *TargetName, *DisplayName));
		}
		return;
	}

	// Status applied to a player — only show if it's us
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	AddCombatLogMessage(FString::Printf(TEXT("You are afflicted with %s (%.1fs)"), *DisplayName, DurationD / 1000.0));
}

void UChatSubsystem::HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	bool bIsEnemy = false;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	FString StatusType;
	Obj->TryGetStringField(TEXT("statusType"), StatusType);

	FString DisplayName = StatusType;
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

	if (bIsEnemy)
	{
		// Show when a status we applied to an enemy wears off (trap stun/freeze/etc.)
		FString TargetName;
		Obj->TryGetStringField(TEXT("targetName"), TargetName);
		if (!TargetName.IsEmpty())
			AddCombatLogMessage(FString::Printf(TEXT("%s's %s wore off"), *TargetName, *DisplayName));
		return;
	}

	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	AddCombatLogMessage(FString::Printf(TEXT("%s wore off"), *DisplayName));
}

void UChatSubsystem::HandleBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	bool bIsEnemy = false;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	FString BuffName;
	double DurationD = 0;
	Obj->TryGetStringField(TEXT("buffName"), BuffName);
	Obj->TryGetNumberField(TEXT("duration"), DurationD);

	FString DisplayName = BuffName;
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

	if (bIsEnemy)
	{
		// Show when WE applied a buff/debuff to an enemy
		double CasterIdD = 0;
		Obj->TryGetNumberField(TEXT("casterId"), CasterIdD);
		if (LocalCharacterId > 0 && (int32)CasterIdD == LocalCharacterId)
		{
			FString TargetName;
			Obj->TryGetStringField(TEXT("targetName"), TargetName);
			if (TargetName.IsEmpty()) TargetName = FString::Printf(TEXT("Enemy %d"), (int32)TargetIdD);
			if (DurationD > 0)
				AddCombatLogMessage(FString::Printf(TEXT("%s is affected by %s (%.0fs)"), *TargetName, *DisplayName, DurationD / 1000.0));
			else
				AddCombatLogMessage(FString::Printf(TEXT("%s is affected by %s"), *TargetName, *DisplayName));
		}
		return;
	}

	// Self buff
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;
	AddCombatLogMessage(FString::Printf(TEXT("%s applied (%.0fs)"), *DisplayName, DurationD / 1000.0));
}

void UChatSubsystem::HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	FString BuffName;
	Obj->TryGetStringField(TEXT("buffName"), BuffName);

	FString DisplayName = BuffName;
	DisplayName.ReplaceInline(TEXT("_"), TEXT(" "));

	AddCombatLogMessage(FString::Printf(TEXT("%s wore off"), *DisplayName));
}

void UChatSubsystem::HandleCombatDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double KilledIdD = 0, KillerIdD = 0;
	FString KilledName, KillerName;
	bool bIsEnemy = false;

	Obj->TryGetNumberField(TEXT("killedId"), KilledIdD);
	Obj->TryGetNumberField(TEXT("killerId"), KillerIdD);
	Obj->TryGetStringField(TEXT("killedName"), KilledName);
	Obj->TryGetStringField(TEXT("killerName"), KillerName);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	if ((int32)KilledIdD == LocalCharacterId)
	{
		AddCombatLogMessage(FString::Printf(TEXT("You have been slain by %s"), *KillerName));
	}
	else if ((int32)KillerIdD == LocalCharacterId)
	{
		AddCombatLogMessage(FString::Printf(TEXT("You have slain %s"), *KilledName));
	}
}

void UChatSubsystem::HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data)
{
	AddCombatLogMessage(TEXT("You have respawned"));
}

void UChatSubsystem::HandleExpGain(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double BaseExpD = 0, JobExpD = 0;
	Obj->TryGetNumberField(TEXT("baseExpGained"), BaseExpD);
	Obj->TryGetNumberField(TEXT("jobExpGained"), JobExpD);
	int32 BaseExp = (int32)BaseExpD;
	int32 JobExp = (int32)JobExpD;

	FString EnemyName;
	Obj->TryGetStringField(TEXT("enemyName"), EnemyName);

	bool bPartyShared = false;
	Obj->TryGetBoolField(TEXT("partyShared"), bPartyShared);

	if (bPartyShared)
	{
		double OrigBaseD = 0, OrigJobD = 0, BonusPctD = 0, MemberCountD = 1;
		Obj->TryGetNumberField(TEXT("originalBaseExp"), OrigBaseD);
		Obj->TryGetNumberField(TEXT("originalJobExp"), OrigJobD);
		Obj->TryGetNumberField(TEXT("partyBonusPct"), BonusPctD);
		Obj->TryGetNumberField(TEXT("memberCount"), MemberCountD);
		int32 OrigBase = (int32)OrigBaseD;
		int32 OrigJob = (int32)OrigJobD;
		int32 BonusPct = (int32)BonusPctD;
		int32 MemberCount = (int32)MemberCountD;

		// "Defeated Poring — +120 Base / +80 Job EXP (Party: 500 base split 4 ways, +60% bonus)"
		AddCombatLogMessage(FString::Printf(
			TEXT("%s — +%d Base / +%d Job EXP (Party: %d/%d base/job split %d ways, +%d%% bonus)"),
			*EnemyName, BaseExp, JobExp, OrigBase, OrigJob, MemberCount, BonusPct));
	}
	else
	{
		// "Defeated Poring — +500 Base / +300 Job EXP"
		AddCombatLogMessage(FString::Printf(
			TEXT("%s — +%d Base / +%d Job EXP"),
			*EnemyName, BaseExp, JobExp));
	}
}

// ============================================================
// Public API
// ============================================================

void UChatSubsystem::SendChatMessage(const FString& Message, const FString& Channel)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	FString ActualMessage = Message;
	FString ActualChannel = Channel;

	// ── /sit and /stand: toggle sitting state (client-side, no server chat) ──
	if (ActualMessage == TEXT("/sit") || ActualMessage == TEXT("/stand"))
	{
		UWorld* W = GetWorld();
		if (!W) return;
		bool bSitting = false;
		if (UBuffBarSubsystem* BBS = W->GetSubsystem<UBuffBarSubsystem>())
			bSitting = BBS->HasBuff(TEXT("sitting"));

		if (ActualMessage == TEXT("/stand") || bSitting)
			GI->EmitSocketEvent(TEXT("player:stand"), MakeShared<FJsonObject>());
		else
			GI->EmitSocketEvent(TEXT("player:sit"), MakeShared<FJsonObject>());
		return;
	}

	// ── /trade PlayerName: initiate trade with another player ──
	if (ActualMessage.StartsWith(TEXT("/trade ")))
	{
		FString TargetName = ActualMessage.Mid(7).TrimStartAndEnd();
		if (TargetName.IsEmpty())
		{
			AddCombatLogMessage(TEXT("Usage: /trade PlayerName"));
			return;
		}
		UWorld* W = GetWorld();
		if (W)
		{
			if (UOtherPlayerSubsystem* OPS = W->GetSubsystem<UOtherPlayerSubsystem>())
			{
				int32 FoundCharId = 0;
				for (const auto& Pair : OPS->GetAllPlayers())
				{
					if (Pair.Value.PlayerName.Equals(TargetName, ESearchCase::IgnoreCase))
					{
						FoundCharId = Pair.Key;
						break;
					}
				}
				if (FoundCharId > 0)
				{
					if (UTradeSubsystem* TradeSub = W->GetSubsystem<UTradeSubsystem>())
						TradeSub->RequestTrade(FoundCharId);
				}
				else
				{
					AddCombatLogMessage(FString::Printf(TEXT("Player '%s' not found nearby."), *TargetName));
				}
			}
		}
		return;
	}

	// ── /where: display current map name and coordinates (RO Classic) ──
	if (ActualMessage == TEXT("/where"))
	{
		UWorld* W = GetWorld();
		if (W)
		{
			FString ZoneName = TEXT("unknown");
			if (UZoneTransitionSubsystem* ZoneSub = W->GetSubsystem<UZoneTransitionSubsystem>())
			{
				ZoneName = ZoneSub->CurrentZoneName;
			}
			int32 CellX = 0, CellY = 0;
			if (ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(W, 0))
			{
				FVector Pos = PlayerChar->GetActorLocation();
				CellX = FMath::RoundToInt32(Pos.X / 50.f);
				CellY = FMath::RoundToInt32(Pos.Y / 50.f);
			}

			FChatMessage Msg;
			Msg.SenderName = TEXT("");
			Msg.Message = FString::Printf(TEXT("%s (%d, %d)"), *ZoneName, CellX, CellY);
			Msg.Channel = EChatChannel::System;
			Msg.Timestamp = FDateTime::Now().GetTicks();
			if (Messages.Num() >= MAX_MESSAGES) Messages.RemoveAt(0);
			Messages.Add(MoveTemp(Msg));
			++MessageVersion;
		}
		return;
	}

	// ── /r reply shortcut: expand to /w "LastWhisperer" message ──
	if (ActualMessage.StartsWith(TEXT("/r ")) && !LastWhisperer.IsEmpty())
	{
		FString ReplyBody = ActualMessage.RightChop(3).TrimStart();
		if (ReplyBody.IsEmpty()) return;
		ActualMessage = FString::Printf(TEXT("/w \"%s\" %s"), *LastWhisperer, *ReplyBody);
	}

	// ── Slash commands: send to server, NO local echo (server sends response) ──
	if (ActualMessage.StartsWith(TEXT("/w ")) || ActualMessage.StartsWith(TEXT("/ex "))
		|| ActualMessage == TEXT("/exall") || ActualMessage.StartsWith(TEXT("/in "))
		|| ActualMessage == TEXT("/inall") || ActualMessage.StartsWith(TEXT("/am"))
		|| ActualMessage.StartsWith(TEXT("/r ")))
	{
		TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
		Payload->SetStringField(TEXT("message"), ActualMessage);
		Payload->SetStringField(TEXT("channel"), TEXT("GLOBAL"));
		GI->EmitSocketEvent(TEXT("chat:message"), Payload);
		return;
	}

	// RO Classic: % prefix routes to party chat channel
	if (ActualMessage.StartsWith(TEXT("%")))
	{
		ActualMessage = ActualMessage.RightChop(1).TrimStart();
		if (ActualMessage.IsEmpty()) return;
		ActualChannel = TEXT("PARTY");
	}

	// Local echo — show message immediately in chat window
	FChatMessage LocalMsg;
	LocalMsg.SenderName = LocalCharacterName;
	LocalMsg.Message = ActualMessage;
	LocalMsg.Channel = ParseChannel(ActualChannel);
	LocalMsg.Timestamp = FDateTime::Now().GetTicks();

	if (Messages.Num() >= MAX_MESSAGES)
	{
		Messages.RemoveAt(0);
	}
	Messages.Add(MoveTemp(LocalMsg));
	++MessageVersion;

	// Party chat: emit via party:chat event instead of chat:message
	if (ActualChannel == TEXT("PARTY"))
	{
		TSharedPtr<FJsonObject> PartyPayload = MakeShared<FJsonObject>();
		PartyPayload->SetStringField(TEXT("message"), ActualMessage);
		GI->EmitSocketEvent(TEXT("party:chat"), PartyPayload);
		return;
	}

	// Emit to server
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("message"), ActualMessage);
	Payload->SetStringField(TEXT("channel"), ActualChannel);

	GI->EmitSocketEvent(TEXT("chat:message"), Payload);
}

void UChatSubsystem::FocusChatInput()
{
	if (ChatWidget.IsValid())
	{
		ChatWidget->FocusInput();
	}
}

void UChatSubsystem::StartWhisperTo(const FString& PlayerName)
{
	if (ChatWidget.IsValid())
	{
		FString Prefill = FString::Printf(TEXT("/w \"%s\" "), *PlayerName);
		ChatWidget->SetInputText(Prefill);
		ChatWidget->FocusInput();
	}
}

// ============================================================
// Channel helpers
// ============================================================

EChatChannel UChatSubsystem::ParseChannel(const FString& ChannelStr)
{
	FString Upper = ChannelStr.ToUpper();
	if (Upper == TEXT("PARTY")) return EChatChannel::Party;
	if (Upper == TEXT("GUILD")) return EChatChannel::Guild;
	if (Upper == TEXT("WHISPER") || Upper == TEXT("WHISPER_SENT")) return EChatChannel::Whisper;
	if (Upper == TEXT("COMBAT")) return EChatChannel::Combat;
	if (Upper == TEXT("SYSTEM")) return EChatChannel::System;
	return EChatChannel::Normal;
}

FLinearColor UChatSubsystem::GetChannelColor(EChatChannel Channel)
{
	switch (Channel)
	{
	case EChatChannel::Party:   return ChatColors::Party;
	case EChatChannel::Guild:   return ChatColors::Guild;
	case EChatChannel::Whisper: return ChatColors::Whisper;
	case EChatChannel::System:  return ChatColors::System;
	case EChatChannel::Combat:  return FLinearColor(1.0f, 0.5f, 0.3f, 1.f); // Orange-red
	default:                    return ChatColors::Normal;
	}
}

// ============================================================
// Widget lifecycle
// ============================================================

void UChatSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	ChatWidget = SNew(SChatWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			ChatWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 13);
	bWidgetAdded = true;

	UE_LOG(LogChat, Log, TEXT("Chat widget added to viewport (Z=13)."));
}

void UChatSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

	UWorld* World = GetWorld();
	UGameViewportClient* VC = World ? World->GetGameViewport() : nullptr;
	if (VC && ViewportOverlay.IsValid())
	{
		VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}

	ChatWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
}
