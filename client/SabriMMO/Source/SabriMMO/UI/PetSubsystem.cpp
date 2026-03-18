// PetSubsystem.cpp — Pet system UWorldSubsystem + inline SPetWidget (Slate)
// RO Classic brown/gold theme, hunger/intimacy bars, feed/return/rename buttons.

#include "PetSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogPet, Log, All);

// ============================================================
// RO Colors (matching project palette)
// ============================================================
namespace PetColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor BarBg         (0.10f, 0.07f, 0.04f, 1.f);
	static const FLinearColor HungerGreen   (0.25f, 0.75f, 0.20f, 1.f);
	static const FLinearColor IntimacyPink  (0.85f, 0.40f, 0.60f, 1.f);
	static const FLinearColor ButtonBrown   (0.35f, 0.24f, 0.14f, 1.f);
}

// ============================================================
// SPetWidget — Inline Slate Widget
// ============================================================
class SPetWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPetWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UPetSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Sub = InArgs._Subsystem;

		ChildSlot
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBox)
			.WidthOverride(220.f)
			[
				// 3-layer RO frame
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(PetColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(PetColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(PetColors::PanelBrown)
						.Padding(FMargin(6.f, 4.f))
						[
							SNew(SVerticalBox)

							// Title
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasPet) return FText::FromString(TEXT("Pet"));
									return FText::FromString(FString::Printf(TEXT("Pet - %s"), *Sub->PetName));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FSlateColor(PetColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(PetColors::TextShadow)
								.Justification(ETextJustify::Center)
							]

							// Gold divider
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildDivider() ]

							// No pet message
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasPet) return FText::FromString(TEXT("No pet active.\nUse a Pet Incubator on an egg."));
									return FText::GetEmpty();
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(PetColors::TextPrimary))
								.AutoWrapText(true)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Collapsed : EVisibility::Visible; })
							]

							// Intimacy level
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasPet) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("Mood: %s"), *Sub->IntimacyLevel));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(PetColors::TextPrimary))
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
							]

							// Hunger bar
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBarRow(TEXT("Hunger"), PetColors::HungerGreen,
									TAttribute<TOptional<float>>::CreateLambda([this]() -> TOptional<float> {
										return Sub.IsValid() ? FMath::Clamp((float)Sub->Hunger / 100.f, 0.f, 1.f) : 0.f;
									}),
									TAttribute<FText>::CreateLambda([this]() -> FText {
										return Sub.IsValid() ? FText::FromString(FString::Printf(TEXT("%d/100"), Sub->Hunger)) : FText::GetEmpty();
									})
								)]
							]

							// Intimacy bar
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBarRow(TEXT("Intimacy"), PetColors::IntimacyPink,
									TAttribute<TOptional<float>>::CreateLambda([this]() -> TOptional<float> {
										return Sub.IsValid() ? FMath::Clamp((float)Sub->Intimacy / 1000.f, 0.f, 1.f) : 0.f;
									}),
									TAttribute<FText>::CreateLambda([this]() -> FText {
										return Sub.IsValid() ? FText::FromString(FString::Printf(TEXT("%d/1000"), Sub->Intimacy)) : FText::GetEmpty();
									})
								)]
							]

							// Divider
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildDivider() ]
							]

							// Feed button
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(PetColors::ButtonBrown)
								.OnClicked_Lambda([this]() -> FReply {
									if (Sub.IsValid()) Sub->FeedPet();
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Feed")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(PetColors::TextPrimary))
								]
							]

							// Return to Egg button
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(PetColors::ButtonBrown)
								.OnClicked_Lambda([this]() -> FReply {
									if (Sub.IsValid()) Sub->ReturnPetToEgg();
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasPet) ? EVisibility::Visible : EVisibility::Collapsed; })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Return to Egg")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(PetColors::TextPrimary))
								]
							]
						]
					]
				]
			]
		];

		SetRenderTransform(FSlateRenderTransform(FVector2f(10.f, 400.f)));
	}

private:
	TWeakObjectPtr<UPetSubsystem> Sub;

	TSharedRef<SWidget> BuildDivider()
	{
		return SNew(SBox).HeightOverride(1.f).Padding(FMargin(0, 2))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(PetColors::GoldDivider)
		];
	}

	TSharedRef<SWidget> BuildBarRow(const FString& Label, const FLinearColor& BarColor,
		TAttribute<TOptional<float>> Percent, TAttribute<FText> ValueText)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SBox).WidthOverride(52.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(PetColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(PetColors::TextShadow)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(14.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(PetColors::GoldDark)
					.Padding(FMargin(1.f))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(PetColors::BarBg) ]
						+ SOverlay::Slot()
						[ SNew(SProgressBar).Percent(Percent).FillColorAndOpacity(BarColor) ]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(ValueText)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
							.ColorAndOpacity(FSlateColor(PetColors::TextBright))
							.ShadowOffset(FVector2D(1, 1))
							.ShadowColorAndOpacity(PetColors::TextShadow)
						]
					]
				]
			];
	}
};

// ============================================================
// Subsystem Lifecycle
// ============================================================

bool UPetSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UPetSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("pet:hatched"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetHatched(D); });
		Router->RegisterHandler(TEXT("pet:fed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetFed(D); });
		Router->RegisterHandler(TEXT("pet:hunger_update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetHungerUpdate(D); });
		Router->RegisterHandler(TEXT("pet:ran_away"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetRanAway(D); });
		Router->RegisterHandler(TEXT("pet:returned"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetReturned(D); });
		Router->RegisterHandler(TEXT("pet:renamed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetRenamed(D); });
		Router->RegisterHandler(TEXT("pet:tamed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetTamed(D); });
		Router->RegisterHandler(TEXT("pet:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePetError(D); });
	}

	// Load placeholder pet actor class (reuse BP_EnemyCharacter, scaled down as pet)
	PetActorClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/SabriMMO/Blueprints/BP_EnemyCharacter.BP_EnemyCharacter_C"));
	if (!PetActorClass)
	{
		UE_LOG(LogPet, Warning, TEXT("Failed to load BP_EnemyCharacter as pet placeholder"));
	}
}

void UPetSubsystem::Deinitialize()
{
	DespawnPetActor();
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FollowTimerHandle);
	HideWidget();
	UWorld* World = GetWorld();
	if (World)
	{
		UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
		if (GI)
		{
			USocketEventRouter* Router = GI->GetEventRouter();
			if (Router) Router->UnregisterAllForOwner(this);
		}
	}
	Super::Deinitialize();
}

// ============================================================
// Widget Lifecycle
// ============================================================

void UPetSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	PetWidget = SNew(SPetWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			PetWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 21); // Z=21
	bWidgetAdded = true;
	bWidgetVisible = true;
}

void UPetSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* VC = World->GetGameViewport();
		if (VC && ViewportOverlay.IsValid())
		{
			VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}
	PetWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
	bWidgetVisible = false;
}

void UPetSubsystem::ToggleWidget()
{
	if (bWidgetVisible) HideWidget();
	else ShowWidget();
}

// ============================================================
// Event Handlers
// ============================================================

void UPetSubsystem::HandlePetHatched(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double PetIdD = 0, MobIdD = 0, HungerD = 0, IntimacyD = 0;
	Obj->TryGetNumberField(TEXT("petId"), PetIdD);
	Obj->TryGetNumberField(TEXT("mobId"), MobIdD);
	Obj->TryGetNumberField(TEXT("hunger"), HungerD);
	Obj->TryGetNumberField(TEXT("intimacy"), IntimacyD);
	Obj->TryGetStringField(TEXT("name"), PetName);
	Obj->TryGetStringField(TEXT("intimacyLevel"), IntimacyLevel);

	PetId = (int32)PetIdD;
	MobId = (int32)MobIdD;
	Hunger = (int32)HungerD;
	Intimacy = (int32)IntimacyD;
	bHasPet = true;

	if (!bWidgetAdded) ShowWidget();
	SpawnPetActor();
	UE_LOG(LogPet, Log, TEXT("Pet hatched: %s (mob %d)"), *PetName, MobId);
}

void UPetSubsystem::HandlePetFed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double HungerD = 0, IntimacyD = 0;
	Obj->TryGetNumberField(TEXT("hunger"), HungerD);
	Obj->TryGetNumberField(TEXT("intimacy"), IntimacyD);
	Obj->TryGetStringField(TEXT("intimacyLevel"), IntimacyLevel);
	Hunger = (int32)HungerD;
	Intimacy = (int32)IntimacyD;
}

void UPetSubsystem::HandlePetHungerUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double HungerD = 0, IntimacyD = 0;
	Obj->TryGetNumberField(TEXT("hunger"), HungerD);
	Obj->TryGetNumberField(TEXT("intimacy"), IntimacyD);
	Obj->TryGetStringField(TEXT("hungerLevel"), HungerLevel);
	Obj->TryGetStringField(TEXT("intimacyLevel"), IntimacyLevel);
	Hunger = (int32)HungerD;
	Intimacy = (int32)IntimacyD;
}

void UPetSubsystem::HandlePetRanAway(const TSharedPtr<FJsonValue>& Data)
{
	DespawnPetActor();
	bHasPet = false;
	PetId = 0;
	MobId = 0;
	PetName.Empty();
	Hunger = 0;
	Intimacy = 0;
	UE_LOG(LogPet, Warning, TEXT("Pet ran away!"));
}

void UPetSubsystem::HandlePetReturned(const TSharedPtr<FJsonValue>& Data)
{
	DespawnPetActor();
	bHasPet = false;
	PetId = 0;
	MobId = 0;
	PetName.Empty();
	Hunger = 0;
	Intimacy = 0;
}

void UPetSubsystem::HandlePetRenamed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	(*ObjPtr)->TryGetStringField(TEXT("name"), PetName);
}

void UPetSubsystem::HandlePetTamed(const TSharedPtr<FJsonValue>& Data)
{
	UE_LOG(LogPet, Log, TEXT("Pet tamed successfully!"));
}

void UPetSubsystem::HandlePetError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	FString ErrorMsg;
	(*ObjPtr)->TryGetStringField(TEXT("message"), ErrorMsg);
	UE_LOG(LogPet, Warning, TEXT("Pet error: %s"), *ErrorMsg);
}

// ============================================================
// Public API — Emit to Server
// ============================================================

void UPetSubsystem::FeedPet()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("pet:feed"), Payload);
}

void UPetSubsystem::ReturnPetToEgg()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("pet:return_to_egg"), Payload);
}

void UPetSubsystem::RenamePet(const FString& NewName)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("name"), NewName);
	GI->EmitSocketEvent(TEXT("pet:rename"), Payload);
}

void UPetSubsystem::RequestPetList()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("pet:list"), Payload);
}

// ============================================================
// Pet Actor — Placeholder visual (BP_EnemyCharacter scaled to 50%)
// ============================================================

void UPetSubsystem::SpawnPetActor()
{
	if (PetActor || !PetActorClass) return;
	UWorld* World = GetWorld();
	if (!World) return;

	// Spawn near the player pawn
	APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	FVector SpawnLoc = Pawn ? Pawn->GetActorLocation() + FVector(100.f, 100.f, 0.f) : FVector::ZeroVector;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnLoc);
	SpawnTransform.SetScale3D(FVector(0.5f)); // 50% scale — pet-sized placeholder

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PetActor = World->SpawnActor(PetActorClass, &SpawnTransform, SpawnParams);
	if (PetActor)
	{
		PetActor->SetActorScale3D(FVector(0.5f));
		UE_LOG(LogPet, Log, TEXT("Spawned pet placeholder actor at %s"), *SpawnLoc.ToString());

		// Start follow tick (10Hz — pet follows owner)
		TWeakObjectPtr<UPetSubsystem> WeakThis(this);
		World->GetTimerManager().SetTimer(FollowTimerHandle, [WeakThis]()
		{
			if (WeakThis.IsValid()) WeakThis->TickFollowOwner();
		}, 0.1f, true);
	}
}

void UPetSubsystem::DespawnPetActor()
{
	if (PetActor)
	{
		PetActor->Destroy();
		PetActor = nullptr;
		UE_LOG(LogPet, Log, TEXT("Despawned pet actor"));
	}
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FollowTimerHandle);
}

void UPetSubsystem::TickFollowOwner()
{
	if (!PetActor || !bHasPet) { DespawnPetActor(); return; }
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Pawn) return;

	// Follow at offset behind player (100 units back, 80 units right)
	const FVector OwnerLoc = Pawn->GetActorLocation();
	const FRotator OwnerRot = Pawn->GetActorRotation();
	const FVector Offset = OwnerRot.RotateVector(FVector(-100.f, 80.f, 0.f));
	const FVector TargetLoc = OwnerLoc + Offset;

	// Smooth interpolation toward target (80% speed of player)
	const FVector CurrentLoc = PetActor->GetActorLocation();
	const FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, 0.1f, 8.f);
	PetActor->SetActorLocation(NewLoc);

	// Face the same direction as owner
	PetActor->SetActorRotation(OwnerRot);
}
