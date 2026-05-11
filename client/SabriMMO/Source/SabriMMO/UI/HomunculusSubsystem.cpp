// HomunculusSubsystem.cpp — Homunculus UWorldSubsystem + inline SHomunculusWidget (Slate)
// RO Classic brown/gold theme, HP/SP bars, hunger/intimacy, feed/vaporize buttons.

#include "HomunculusSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
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
#include "Styling/CoreStyle.h"

// Map homunculus type → existing enemy sprite atlas (placeholder visuals).
// Using small/cute monsters from the Poring family + Picky for variety.
static FString GetSpriteClassForHomunculusType(const FString& Type)
{
    if (Type == TEXT("lif"))        return TEXT("poring");    // pink blob — friendly healer
    if (Type == TEXT("amistr"))     return TEXT("poporing");  // green horned blob — sturdy tank
    if (Type == TEXT("filir"))      return TEXT("picky");     // yellow chick — bird theme
    if (Type == TEXT("vanilmirth")) return TEXT("drops");     // yellow blob — chaotic variant
    return TEXT("poring");
}

DEFINE_LOG_CATEGORY_STATIC(LogHomUI, Log, All);

namespace HomColors
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
	static const FLinearColor HPRed         (0.85f, 0.15f, 0.15f, 1.f);
	static const FLinearColor SPBlue        (0.20f, 0.45f, 0.90f, 1.f);
	static const FLinearColor HungerGreen   (0.25f, 0.75f, 0.20f, 1.f);
	static const FLinearColor IntimacyPink  (0.85f, 0.40f, 0.60f, 1.f);
	static const FLinearColor ButtonBrown   (0.35f, 0.24f, 0.14f, 1.f);
}

// ============================================================
// SHomunculusWidget — Inline Slate Widget
// ============================================================
class SHomunculusWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHomunculusWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UHomunculusSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Sub = InArgs._Subsystem;

		ChildSlot
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(SBox).WidthOverride(230.f)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::GoldTrim).Padding(FMargin(2.f))
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::PanelDark).Padding(FMargin(1.f))
					[
						SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::PanelBrown).Padding(FMargin(6.f, 4.f))
						[
							SNew(SVerticalBox)

							// Title
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasHomunculus) return FText::FromString(TEXT("Homunculus"));
									return FText::FromString(FString::Printf(TEXT("%s Lv.%d"), *Sub->HomunculusName, Sub->HomunculusLevel));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FSlateColor(HomColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(HomColors::TextShadow)
								.Justification(ETextJustify::Center)
							]

							// Divider
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildDivider() ]

							// No homunculus message
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("No homunculus summoned.\nUse Call Homunculus skill.")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								.AutoWrapText(true)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Collapsed : EVisibility::Visible; })
							]

							// Type + Status
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasHomunculus) return FText::GetEmpty();
									FString Status = Sub->bIsAlive ? (Sub->bIsEvolved ? TEXT("Evolved") : TEXT("Active")) : TEXT("Dead");
									return FText::FromString(FString::Printf(TEXT("Type: %s  |  %s"), *Sub->HomunculusType, *Status));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Visible : EVisibility::Collapsed; })
							]

							// HP Bar
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBar(TEXT("HP"), HomColors::HPRed,
									TAttribute<TOptional<float>>::CreateLambda([this]() -> TOptional<float> {
										return (Sub.IsValid() && Sub->MaxHP > 0) ? FMath::Clamp((float)Sub->HP / (float)Sub->MaxHP, 0.f, 1.f) : 0.f;
									}),
									TAttribute<FText>::CreateLambda([this]() -> FText {
										return Sub.IsValid() ? FText::FromString(FString::Printf(TEXT("%d/%d"), Sub->HP, Sub->MaxHP)) : FText::GetEmpty();
									})
								)]
							]

							// SP Bar
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBar(TEXT("SP"), HomColors::SPBlue,
									TAttribute<TOptional<float>>::CreateLambda([this]() -> TOptional<float> {
										return (Sub.IsValid() && Sub->MaxSP > 0) ? FMath::Clamp((float)Sub->SP / (float)Sub->MaxSP, 0.f, 1.f) : 0.f;
									}),
									TAttribute<FText>::CreateLambda([this]() -> FText {
										return Sub.IsValid() ? FText::FromString(FString::Printf(TEXT("%d/%d"), Sub->SP, Sub->MaxSP)) : FText::GetEmpty();
									})
								)]
							]

							// Hunger Bar
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildBar(TEXT("Feed"), HomColors::HungerGreen,
									TAttribute<TOptional<float>>::CreateLambda([this]() -> TOptional<float> {
										return Sub.IsValid() ? FMath::Clamp((float)Sub->Hunger / 100.f, 0.f, 1.f) : 0.f;
									}),
									TAttribute<FText>::CreateLambda([this]() -> FText {
										return Sub.IsValid() ? FText::FromString(FString::Printf(TEXT("%d/100"), Sub->Hunger)) : FText::GetEmpty();
									})
								)]
							]

							// Divider
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildDivider() ]
							]

							// Feed button
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(HomColors::ButtonBrown)
								.OnClicked_Lambda([this]() -> FReply {
									if (Sub.IsValid()) Sub->FeedHomunculus();
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Feed")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								]
							]

							// Vaporize button
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(HomColors::ButtonBrown)
								.OnClicked_Lambda([this]() -> FReply {
									if (Sub.IsValid()) Sub->VaporizeHomunculus();
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Vaporize (Rest)")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								]
							]

							// Skill point readout + Allocate row (3 small buttons)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 1)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									if (!Sub.IsValid() || !Sub->bHasHomunculus) return FText::GetEmpty();
									return FText::FromString(FString::Printf(
										TEXT("Skill Points: %d   Lv: %d/%d/%d"),
										Sub->SkillPoints, Sub->Skill1Level, Sub->Skill2Level, Sub->Skill3Level));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
								.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
							]

							+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
							[
								SNew(SHorizontalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildSlotButton(1, TEXT("S1+")) ]
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildSlotButton(2, TEXT("S2+")) ]
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildSlotButton(3, TEXT("S3+")) ]
							]

							// Use Skill row (3 buttons)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
							[
								SNew(SHorizontalBox)
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildUseSkillButton(1, TEXT("Use 1")) ]
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildUseSkillButton(2, TEXT("Use 2")) ]
								+ SHorizontalBox::Slot().FillWidth(1).Padding(1, 0)
								[ BuildUseSkillButton(3, TEXT("Use 3")) ]
							]

							// Command cycle button
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(HomColors::ButtonBrown)
								.OnClicked_Lambda([this]() -> FReply {
									if (!Sub.IsValid()) return FReply::Handled();
									// Cycle: follow → attack → standby → follow
									FString Next = TEXT("follow");
									if (Sub->CurrentCommand == TEXT("follow"))      Next = TEXT("attack");
									else if (Sub->CurrentCommand == TEXT("attack")) Next = TEXT("standby");
									Sub->SetCommand(Next);
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() { return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive) ? EVisibility::Visible : EVisibility::Collapsed; })
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText {
										if (!Sub.IsValid()) return FText::GetEmpty();
										return FText::FromString(FString::Printf(TEXT("Mode: %s"), *Sub->CurrentCommand));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
								]
							]

							// Evolve button (only when intimacy >= 911 / Loyal bracket and not yet evolved)
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								SNew(SButton)
								.ContentPadding(FMargin(4, 2))
								.HAlign(HAlign_Center)
								.ButtonColorAndOpacity(HomColors::GoldHighlight)
								.OnClicked_Lambda([this]() -> FReply {
									if (Sub.IsValid()) Sub->EvolveHomunculus();
									return FReply::Handled();
								})
								.Visibility_Lambda([this]() {
									return (Sub.IsValid() && Sub->bHasHomunculus && Sub->bIsAlive
										&& !Sub->bIsEvolved && Sub->Intimacy >= 911)
										? EVisibility::Visible : EVisibility::Collapsed;
								})
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Evolve")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(HomColors::PanelDark))
								]
							]
						]
					]
				]
			]
		];

		SetRenderTransform(FSlateRenderTransform(FVector2f(240.f, 400.f)));
	}

private:
	TWeakObjectPtr<UHomunculusSubsystem> Sub;

	TSharedRef<SWidget> BuildDivider()
	{
		return SNew(SBox).HeightOverride(1.f).Padding(FMargin(0, 2))
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::GoldDivider)
		];
	}

	TSharedRef<SWidget> BuildSlotButton(int32 Slot, const FString& Label)
	{
		return SNew(SButton)
			.ContentPadding(FMargin(2, 1))
			.HAlign(HAlign_Center)
			.ButtonColorAndOpacity(HomColors::ButtonBrown)
			.IsEnabled_Lambda([this]() { return Sub.IsValid() && Sub->SkillPoints > 0; })
			.OnClicked_Lambda([this, Slot]() -> FReply {
				if (Sub.IsValid()) Sub->AllocateSkillPoint(Slot);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
			];
	}

	TSharedRef<SWidget> BuildUseSkillButton(int32 Slot, const FString& Label)
	{
		return SNew(SButton)
			.ContentPadding(FMargin(2, 1))
			.HAlign(HAlign_Center)
			.ButtonColorAndOpacity(HomColors::ButtonBrown)
			.IsEnabled_Lambda([this, Slot]() {
				if (!Sub.IsValid()) return false;
				const int32 Lv = Slot == 1 ? Sub->Skill1Level : Slot == 2 ? Sub->Skill2Level : Sub->Skill3Level;
				return Lv > 0;
			})
			.OnClicked_Lambda([this, Slot]() -> FReply {
				if (Sub.IsValid()) Sub->UseSkill(Slot);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(HomColors::TextPrimary))
			];
	}

	TSharedRef<SWidget> BuildBar(const FString& Label, const FLinearColor& Color,
		TAttribute<TOptional<float>> Pct, TAttribute<FText> ValText)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SBox).WidthOverride(32.f)
				[
					SNew(STextBlock).Text(FText::FromString(Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(HomColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(HomColors::TextShadow)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(14.f)
				[
					SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::GoldDark).Padding(FMargin(1.f))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(HomColors::BarBg) ]
						+ SOverlay::Slot()[ SNew(SProgressBar).Percent(Pct).FillColorAndOpacity(Color) ]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(ValText).Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
							.ColorAndOpacity(FSlateColor(HomColors::TextBright))
							.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(HomColors::TextShadow)
						]
					]
				]
			];
	}
};

// ============================================================
// Subsystem Lifecycle
// ============================================================

bool UHomunculusSubsystem::ShouldCreateSubsystem(UObject* Outer) const { return true; }

void UHomunculusSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("homunculus:summoned"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleSummoned(D); });
		Router->RegisterHandler(TEXT("homunculus:vaporized"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleVaporized(D); });
		Router->RegisterHandler(TEXT("homunculus:update"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleUpdate(D); });
		Router->RegisterHandler(TEXT("homunculus:leveled_up"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleLeveledUp(D); });
		Router->RegisterHandler(TEXT("homunculus:died"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleDied(D); });
		Router->RegisterHandler(TEXT("homunculus:fed"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleFed(D); });
		Router->RegisterHandler(TEXT("homunculus:hunger_tick"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleHungerTick(D); });
		Router->RegisterHandler(TEXT("homunculus:evolved"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleEvolved(D); });
		Router->RegisterHandler(TEXT("homunculus:resurrected"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleResurrected(D); });
		// Skill point allocation result
		Router->RegisterHandler(TEXT("homunculus:skill_updated"), this, [this](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
			if (!D->TryGetObject(ObjPtr) || !ObjPtr) return;
			const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
			double Slot = 0, NewLv = 0, Pts = 0;
			Obj->TryGetNumberField(TEXT("skillSlot"), Slot);
			Obj->TryGetNumberField(TEXT("newLevel"), NewLv);
			Obj->TryGetNumberField(TEXT("skillPoints"), Pts);
			const int32 SlotI = (int32)Slot;
			if (SlotI == 1) Skill1Level = (int32)NewLv;
			else if (SlotI == 2) Skill2Level = (int32)NewLv;
			else if (SlotI == 3) Skill3Level = (int32)NewLv;
			SkillPoints = (int32)Pts;
		});
		Router->RegisterHandler(TEXT("homunculus:command_ack"), this, [this](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
			if (!D->TryGetObject(ObjPtr) || !ObjPtr) return;
			(*ObjPtr)->TryGetStringField(TEXT("command"), CurrentCommand);
		});
		// Remote homunculi (other players' homunculi)
		Router->RegisterHandler(TEXT("homunculus:other_summoned"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleOtherSummoned(D); });
		Router->RegisterHandler(TEXT("homunculus:other_dismissed"), this, [this](const TSharedPtr<FJsonValue>& D) { HandleOtherDismissed(D); });
		Router->RegisterHandler(TEXT("homunculus:position"), this, [this](const TSharedPtr<FJsonValue>& D) { HandlePosition(D); });
	}
	// Sprite-based actors (ASpriteCharacterActor) are spawned directly — no BP class load needed.
}

void UHomunculusSubsystem::Deinitialize()
{
	DespawnHomunculusActor();
	for (auto& Pair : RemoteHomActors)
	{
		if (Pair.Value) Pair.Value->Destroy();
	}
	RemoteHomActors.Empty();
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

void UHomunculusSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	HomWidget = SNew(SHomunculusWidget).Subsystem(this);
	AlignmentWrapper = SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Top).Visibility(EVisibility::SelfHitTestInvisible)[ HomWidget.ToSharedRef() ];
	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 22);
	bWidgetAdded = true;
	bWidgetVisible = true;
}

void UHomunculusSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* VC = World->GetGameViewport();
		if (VC && ViewportOverlay.IsValid()) VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}
	HomWidget.Reset(); AlignmentWrapper.Reset(); ViewportOverlay.Reset();
	bWidgetAdded = false; bWidgetVisible = false;
}

void UHomunculusSubsystem::ToggleWidget()
{
	if (bWidgetVisible) HideWidget(); else ShowWidget();
}

// ============================================================
// Event Handlers
// ============================================================

void UHomunculusSubsystem::HandleSummoned(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	Obj->TryGetStringField(TEXT("name"), HomunculusName);
	Obj->TryGetStringField(TEXT("type"), HomunculusType);
	double LvD = 0, HpD = 0, MaxHpD = 0, SpD = 0, MaxSpD = 0, HunD = 0, IntD = 0;
	Obj->TryGetNumberField(TEXT("level"), LvD); HomunculusLevel = (int32)LvD;
	Obj->TryGetNumberField(TEXT("hp"), HpD); HP = (int32)HpD;
	// Server emits `maxHp` / `maxSp` (camelCase 'Hp'/'Sp'), NOT `hpMax`/`spMax`. Match exactly.
	Obj->TryGetNumberField(TEXT("maxHp"), MaxHpD); MaxHP = (int32)MaxHpD;
	Obj->TryGetNumberField(TEXT("sp"), SpD); SP = (int32)SpD;
	Obj->TryGetNumberField(TEXT("maxSp"), MaxSpD); MaxSP = (int32)MaxSpD;
	Obj->TryGetNumberField(TEXT("hunger"), HunD); Hunger = (int32)HunD;
	Obj->TryGetNumberField(TEXT("intimacy"), IntD); Intimacy = (int32)IntD;
	// Server emits the field as `evolved`, not `isEvolved`.
	Obj->TryGetBoolField(TEXT("evolved"), bIsEvolved);
	// Skill levels are nested under `skills` object on the server side.
	const TSharedPtr<FJsonObject>* SkillsPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("skills"), SkillsPtr) && SkillsPtr)
	{
		double Sk1 = 0, Sk2 = 0, Sk3 = 0;
		(*SkillsPtr)->TryGetNumberField(TEXT("skill1Level"), Sk1); Skill1Level = (int32)Sk1;
		(*SkillsPtr)->TryGetNumberField(TEXT("skill2Level"), Sk2); Skill2Level = (int32)Sk2;
		(*SkillsPtr)->TryGetNumberField(TEXT("skill3Level"), Sk3); Skill3Level = (int32)Sk3;
	}
	double SkPts = 0;
	Obj->TryGetNumberField(TEXT("skillPoints"), SkPts); SkillPoints = (int32)SkPts;
	bHasHomunculus = true;
	bIsAlive = true;

	if (!bWidgetAdded) ShowWidget();
	SpawnHomunculusActor();
	UE_LOG(LogHomUI, Log, TEXT("Homunculus summoned: %s (%s) Lv%d"), *HomunculusName, *HomunculusType, HomunculusLevel);
}

void UHomunculusSubsystem::HandleVaporized(const TSharedPtr<FJsonValue>& Data)
{
	DespawnHomunculusActor();
	bHasHomunculus = false;
	bIsAlive = false;
}

void UHomunculusSubsystem::HandleUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double HpD = 0, MaxHpD = 0, SpD = 0, MaxSpD = 0;
	Obj->TryGetNumberField(TEXT("hp"), HpD); HP = (int32)HpD;
	Obj->TryGetNumberField(TEXT("maxHp"), MaxHpD); if (MaxHpD > 0) MaxHP = (int32)MaxHpD;
	Obj->TryGetNumberField(TEXT("sp"), SpD); SP = (int32)SpD;
	Obj->TryGetNumberField(TEXT("maxSp"), MaxSpD); if (MaxSpD > 0) MaxSP = (int32)MaxSpD;
}

void UHomunculusSubsystem::HandleLeveledUp(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
	double LvD = 0, SkPts = 0;
	Obj->TryGetNumberField(TEXT("level"), LvD);
	HomunculusLevel = (int32)LvD;
	if (Obj->TryGetNumberField(TEXT("skillPoints"), SkPts)) SkillPoints = (int32)SkPts;
}

void UHomunculusSubsystem::HandleDied(const TSharedPtr<FJsonValue>& Data)
{
	DespawnHomunculusActor();
	bIsAlive = false;
	HP = 0;
}

void UHomunculusSubsystem::HandleFed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	double HunD = 0, IntD = 0;
	(*ObjPtr)->TryGetNumberField(TEXT("hunger"), HunD); Hunger = (int32)HunD;
	(*ObjPtr)->TryGetNumberField(TEXT("intimacy"), IntD); Intimacy = (int32)IntD;
}

void UHomunculusSubsystem::HandleHungerTick(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	double HunD = 0, IntD = 0;
	(*ObjPtr)->TryGetNumberField(TEXT("hunger"), HunD); Hunger = (int32)HunD;
	(*ObjPtr)->TryGetNumberField(TEXT("intimacy"), IntD); Intimacy = (int32)IntD;
}

void UHomunculusSubsystem::HandleEvolved(const TSharedPtr<FJsonValue>& Data)
{
	bIsEvolved = true;
	if (Data.IsValid()) {
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr) {
			double IntD = 0;
			(*ObjPtr)->TryGetNumberField(TEXT("intimacy"), IntD); Intimacy = (int32)IntD;
		}
	}
}

void UHomunculusSubsystem::HandleResurrected(const TSharedPtr<FJsonValue>& Data)
{
	bIsAlive = true;
	bHasHomunculus = true;
	if (Data.IsValid()) {
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr) {
			double HpD = 0, MaxHpD = 0;
			(*ObjPtr)->TryGetNumberField(TEXT("hp"), HpD); HP = (int32)HpD;
			// Server emits `maxHp` (camelCase), not `hpMax`.
			(*ObjPtr)->TryGetNumberField(TEXT("maxHp"), MaxHpD); if (MaxHpD > 0) MaxHP = (int32)MaxHpD;
		}
	}
}

// ============================================================
// Public API
// ============================================================

void UHomunculusSubsystem::FeedHomunculus()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("homunculus:feed"), Payload);
}

void UHomunculusSubsystem::VaporizeHomunculus()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("homunculus:rest"), Payload);
}

// ============================================================
// Homunculus Actor — Placeholder visual (BP_EnemyCharacter scaled to 60%)
// ============================================================

void UHomunculusSubsystem::SpawnHomunculusActor()
{
	if (HomActor) return;
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	FVector SpawnLoc = Pawn ? Pawn->GetActorLocation() + FVector(-100.f, -80.f, 0.f) : FVector::ZeroVector;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpriteCharacterActor* Sprite = World->SpawnActor<ASpriteCharacterActor>(SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (!Sprite) { UE_LOG(LogHomUI, Warning, TEXT("Failed to spawn homunculus sprite actor")); return; }

	const FString SpriteClass = GetSpriteClassForHomunculusType(HomunculusType);
	Sprite->SetBodyClass(SpriteClass);
	Sprite->SetActorScale3D(FVector(0.7f)); // ~70% scale, slightly bigger than the BP placeholder
	Sprite->bUseServerMovement = false;     // local-controlled follow (own homunculus)
	HomActor = Sprite;

	UE_LOG(LogHomUI, Log, TEXT("Spawned homunculus sprite (%s → %s) for type=%s"),
		*HomunculusName, *SpriteClass, *HomunculusType);

	TWeakObjectPtr<UHomunculusSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(FollowTimerHandle, [WeakThis]()
	{
		if (WeakThis.IsValid()) WeakThis->TickFollowOwner();
	}, 0.1f, true);
}

void UHomunculusSubsystem::DespawnHomunculusActor()
{
	if (HomActor)
	{
		HomActor->Destroy();
		HomActor = nullptr;
	}
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FollowTimerHandle);
}

void UHomunculusSubsystem::TickFollowOwner()
{
	if (!HomActor || !bHasHomunculus || !bIsAlive) { DespawnHomunculusActor(); return; }
	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Pawn) return;

	// Follow at offset on the opposite side from pet (left-back)
	const FVector OwnerLoc = Pawn->GetActorLocation();
	const FRotator OwnerRot = Pawn->GetActorRotation();
	const FVector Offset = OwnerRot.RotateVector(FVector(-100.f, -80.f, 0.f));
	const FVector TargetLoc = OwnerLoc + Offset;

	const FVector CurrentLoc = HomActor->GetActorLocation();
	const FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, 0.1f, 8.f);
	HomActor->SetActorLocation(NewLoc);

	// Sprite faces movement direction via ASpriteCharacterActor's billboard system; no rotation set here.
}

// ============================================================
// Remote homunculus rendering (other players' homunculi)
// ============================================================

void UHomunculusSubsystem::HandleOtherSummoned(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double OwnerD = 0;
	Obj->TryGetNumberField(TEXT("ownerId"), OwnerD);
	const int32 OwnerId = (int32)OwnerD;
	if (OwnerId <= 0) return;

	FString RemoteType;
	Obj->TryGetStringField(TEXT("type"), RemoteType);
	if (RemoteType.IsEmpty()) RemoteType = TEXT("poring");

	UWorld* World = GetWorld();
	if (!World) return;

	// Replace existing if present
	if (RemoteHomActors.Contains(OwnerId) && RemoteHomActors[OwnerId])
	{
		RemoteHomActors[OwnerId]->Destroy();
		RemoteHomActors.Remove(OwnerId);
	}

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpriteCharacterActor* Sprite = World->SpawnActor<ASpriteCharacterActor>(FVector(X, Y, Z), FRotator::ZeroRotator, SpawnParams);
	if (!Sprite) return;

	Sprite->SetBodyClass(GetSpriteClassForHomunculusType(RemoteType));
	Sprite->SetActorScale3D(FVector(0.7f));
	Sprite->bUseServerMovement = true;
	Sprite->ServerTargetPos = FVector(X, Y, Z);
	RemoteHomActors.Add(OwnerId, Sprite);
}

void UHomunculusSubsystem::HandleOtherDismissed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	double OwnerD = 0;
	(*ObjPtr)->TryGetNumberField(TEXT("ownerId"), OwnerD);
	const int32 OwnerId = (int32)OwnerD;
	if (RemoteHomActors.Contains(OwnerId))
	{
		if (RemoteHomActors[OwnerId]) RemoteHomActors[OwnerId]->Destroy();
		RemoteHomActors.Remove(OwnerId);
	}
}

void UHomunculusSubsystem::HandlePosition(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double OwnerD = 0, X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("ownerId"), OwnerD);
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);
	const int32 OwnerId = (int32)OwnerD;

	if (RemoteHomActors.Contains(OwnerId))
	{
		AActor* Actor = RemoteHomActors[OwnerId];
		if (Actor)
		{
			ASpriteCharacterActor* Sprite = Cast<ASpriteCharacterActor>(Actor);
			if (Sprite) Sprite->ServerTargetPos = FVector(X, Y, Z);
		}
	}
}

// ============================================================
// Skill / Allocate / Evolve / Command — emit helpers (Phase 6)
// Each just sends an existing socket event the server already handles.
// ============================================================

void UHomunculusSubsystem::UseSkill(int32 SkillSlot)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillSlot"), SkillSlot);
	GI->EmitSocketEvent(TEXT("homunculus:use_skill"), Payload);
}

void UHomunculusSubsystem::AllocateSkillPoint(int32 SkillSlot)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillSlot"), SkillSlot);
	GI->EmitSocketEvent(TEXT("homunculus:skill_up"), Payload);
}

void UHomunculusSubsystem::EvolveHomunculus()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("homunculus:evolve"), Payload);
}

void UHomunculusSubsystem::SetCommand(const FString& Command)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("command"), Command);
	GI->EmitSocketEvent(TEXT("homunculus:command"), Payload);
}
