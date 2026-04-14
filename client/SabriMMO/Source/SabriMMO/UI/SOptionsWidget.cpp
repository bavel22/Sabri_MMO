#include "SOptionsWidget.h"
#include "OptionsSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SComboBox.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/GameUserSettings.h"
#include "DynamicRHI.h"

// ============================================================
// RO Classic Color Palette
// ============================================================

namespace OptColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextDim       (0.70f, 0.62f, 0.50f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);

	static const FLinearColor ButtonNormal  (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor ButtonHover   (0.45f, 0.33f, 0.18f, 1.f);
	static const FLinearColor ButtonPressed (0.25f, 0.16f, 0.09f, 1.f);
	static const FLinearColor ButtonBorder  (0.55f, 0.42f, 0.20f, 1.f);

	static const FLinearColor ToggleOn      (0.20f, 0.85f, 0.20f, 1.f);
	static const FLinearColor ToggleOff     (0.60f, 0.45f, 0.30f, 1.f);

	static const FLinearColor SliderBar     (0.50f, 0.38f, 0.20f, 1.f);
	static const FLinearColor SliderThumb   (0.92f, 0.80f, 0.45f, 1.f);

	static const FLinearColor TabActive     (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor TabInactive   (0.22f, 0.14f, 0.08f, 1.f);

	static const FLinearColor FPSBg         (0.00f, 0.00f, 0.00f, 0.50f);
	static const FLinearColor FPSText       (0.10f, 1.00f, 0.10f, 1.f);

	static const FLinearColor OverlayBg     (0.00f, 0.00f, 0.00f, 0.70f);
}

// ============================================================
// SFPSCounterWidget
// ============================================================

void SFPSCounterWidget::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::HitTestInvisible);
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(OptColors::FPSBg)
		.Padding(FMargin(8.f, 2.f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText {
				return FText::FromString(FString::Printf(TEXT("FPS: %d"), CachedFPS));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.ColorAndOpacity(FSlateColor(OptColors::FPSText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
		]
	];
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SFPSCounterWidget::UpdateFPS));
}

EActiveTimerReturnType SFPSCounterWidget::UpdateFPS(double InCurrentTime, float InDeltaTime)
{
	if (InDeltaTime > 0.f)
	{
		float CurrentFPS = 1.0f / InDeltaTime;
		SmoothedFPS = FMath::Lerp(SmoothedFPS, CurrentFPS, 0.1f);
		CachedFPS = FMath::RoundToInt(SmoothedFPS);
	}
	return EActiveTimerReturnType::Continue;
}

// ============================================================
// Macros for getter/setter lambdas
// ============================================================

#define MAKE_BOOL_GETTER(Expr) MakeShared<TFunction<bool()>>([this]() -> bool { \
	UOptionsSubsystem* S = OwningSubsystem.Get(); return S ? (S->Expr) : false; })

#define MAKE_FLOAT_GETTER(Expr) MakeShared<TFunction<float()>>([this]() -> float { \
	UOptionsSubsystem* S = OwningSubsystem.Get(); return S ? (S->Expr) : 0.f; })

#define MAKE_FLOAT_SETTER(Func) MakeShared<TFunction<void(float)>>([this](float V) { \
	if (UOptionsSubsystem* S = OwningSubsystem.Get()) S->Func(V); })

#define TOGGLE_CLICKED(Getter, Setter) \
	FOnClicked::CreateLambda([this]() -> FReply { \
		if (UOptionsSubsystem* S = OwningSubsystem.Get()) S->Setter(!S->Getter()); \
		return FReply::Handled(); })

// ============================================================
// SOptionsWidget Construction
// ============================================================

void SOptionsWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	PopulateVideoOptions();

	ChildSlot
	[
		SNew(SOverlay)

		// ---- Main panel ----
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.WidthOverride(360.f)
			.MaxDesiredHeight(500.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(OptColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(OptColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(OptColors::PanelBrown)
						.Padding(FMargin(8.f, 6.f))
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot().AutoHeight()
							[ BuildTitleBar() ]

							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
							[ BuildTabBar() ]

							+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 2, 0, 4)
							[
								SNew(SWidgetSwitcher)
								.WidgetIndex_Lambda([this]() { return ActiveTabIndex; })

								+ SWidgetSwitcher::Slot()
								[ BuildGameContent() ]

								+ SWidgetSwitcher::Slot()
								[ BuildVideoContent() ]
							]

							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							[
								BuildButton(FText::FromString(TEXT("Close")),
									FOnClicked::CreateLambda([this]() -> FReply {
										if (UOptionsSubsystem* Sub = OwningSubsystem.Get())
											Sub->HideOptionsPanel();
										return FReply::Handled();
									}))
							]
						]
					]
				]
			]
		]

		// ---- Resolution confirm overlay (hidden by default) ----
		+ SOverlay::Slot()
		[
			SAssignNew(ConfirmOverlay, SBox)
			.Visibility(EVisibility::Collapsed)
			[
				BuildResolutionConfirmOverlay()
			]
		]
	];
}

// ============================================================
// Key handling
// ============================================================

FReply SOptionsWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (bWaitingForConfirm)
		{
			OnRevertResolution();
			return FReply::Handled();
		}
		if (UOptionsSubsystem* Sub = OwningSubsystem.Get())
			Sub->HideOptionsPanel();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Title Bar
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildTitleBar()
{
	return SNew(SBox).HAlign(HAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Options")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		.ColorAndOpacity(FSlateColor(OptColors::GoldHighlight))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(OptColors::TextShadow)
	];
}

// ============================================================
// Tab Bar
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildTabBar()
{
	static FButtonStyle TabStyle = []()
	{
		FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		S.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
		S.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		S.Hovered.TintColor = FSlateColor(FLinearColor::Transparent);
		S.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		S.Pressed.TintColor = FSlateColor(FLinearColor::Transparent);
		S.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		return S;
	}();

	auto BuildTab = [this](const FText& Label, int32 TabIdx) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, TabIdx]() -> FSlateColor {
				return FSlateColor((ActiveTabIndex == TabIdx) ? OptColors::TabActive : OptColors::TabInactive);
			})
			.Padding(FMargin(0.f, 4.f))
			[
				SNew(SButton)
				.ButtonStyle(&TabStyle)
				.OnClicked_Lambda([this, TabIdx]() -> FReply {
					ActiveTabIndex = TabIdx;
					return FReply::Handled();
				})
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity_Lambda([this, TabIdx]() -> FSlateColor {
						return FSlateColor((ActiveTabIndex == TabIdx) ? OptColors::GoldHighlight : OptColors::TextDim);
					})
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(OptColors::TextShadow)
				]
			];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)[ BuildTab(FText::FromString(TEXT("Game")), 0) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ BuildTab(FText::FromString(TEXT("Video")), 1) ]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(1.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(OptColors::GoldDivider)
			]
		];
}

// ============================================================
// Game Tab Content (existing 14 settings)
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildGameContent()
{
	return SNew(SScrollBox)
		.ScrollBarVisibility(EVisibility::Collapsed)

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Display"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show FPS")),
			MAKE_BOOL_GETTER(IsShowFPS()), TOGGLE_CLICKED(IsShowFPS, SetShowFPS)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Skill Effects")),
			MAKE_BOOL_GETTER(IsSkillEffects()), TOGGLE_CLICKED(IsSkillEffects, SetSkillEffects)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Miss Text")),
			MAKE_BOOL_GETTER(IsShowMissText()), TOGGLE_CLICKED(IsShowMissText, SetShowMissText)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Brightness")),
			0.5f, 2.0f, 2, MAKE_FLOAT_GETTER(GetBrightness()), MAKE_FLOAT_SETTER(SetBrightness)) ]

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Interface"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Damage Numbers")),
			MAKE_BOOL_GETTER(IsShowDamageNumbers()), TOGGLE_CLICKED(IsShowDamageNumbers, SetShowDamageNumbers)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Enemy HP Bars")),
			MAKE_BOOL_GETTER(IsShowEnemyHPBars()), TOGGLE_CLICKED(IsShowEnemyHPBars, SetShowEnemyHPBars)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Player Names")),
			MAKE_BOOL_GETTER(IsShowPlayerNames()), TOGGLE_CLICKED(IsShowPlayerNames, SetShowPlayerNames)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Enemy Names")),
			MAKE_BOOL_GETTER(IsShowEnemyNames()), TOGGLE_CLICKED(IsShowEnemyNames, SetShowEnemyNames)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show NPC Names")),
			MAKE_BOOL_GETTER(IsShowNPCNames()), TOGGLE_CLICKED(IsShowNPCNames, SetShowNPCNames)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Show Cast Bars")),
			MAKE_BOOL_GETTER(IsShowCastBars()), TOGGLE_CLICKED(IsShowCastBars, SetShowCastBars)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Chat Timestamps")),
			MAKE_BOOL_GETTER(IsShowChatTimestamps()), TOGGLE_CLICKED(IsShowChatTimestamps, SetShowChatTimestamps)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Chat Opacity")),
			0.0f, 1.0f, 2, MAKE_FLOAT_GETTER(GetChatOpacity()), MAKE_FLOAT_SETTER(SetChatOpacity)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Damage Number Size")),
			0.5f, 2.0f, 1, MAKE_FLOAT_GETTER(GetDamageNumberScale()), MAKE_FLOAT_SETTER(SetDamageNumberScale)) ]

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Audio"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Master Volume")),
			0.0f, 1.0f, 2, MAKE_FLOAT_GETTER(GetMasterVolume()), MAKE_FLOAT_SETTER(SetMasterVolume)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("BGM Volume")),
			0.0f, 1.0f, 2, MAKE_FLOAT_GETTER(GetBgmVolume()), MAKE_FLOAT_SETTER(SetBgmVolume)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("SFX Volume")),
			0.0f, 1.0f, 2, MAKE_FLOAT_GETTER(GetSfxVolume()), MAKE_FLOAT_SETTER(SetSfxVolume)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Ambient Volume")),
			0.0f, 1.0f, 2, MAKE_FLOAT_GETTER(GetAmbientVolume()), MAKE_FLOAT_SETTER(SetAmbientVolume)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Mute When Minimized")),
			MAKE_BOOL_GETTER(IsMuteWhenMinimized()), TOGGLE_CLICKED(IsMuteWhenMinimized, SetMuteWhenMinimized)) ]

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Drop Sounds"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("MVP Drops (Red)")),
			MAKE_BOOL_GETTER(IsDropSoundMvp()), TOGGLE_CLICKED(IsDropSoundMvp, SetDropSoundMvp)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Card Drops (Purple)")),
			MAKE_BOOL_GETTER(IsDropSoundCard()), TOGGLE_CLICKED(IsDropSoundCard, SetDropSoundCard)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Equipment Drops (Blue)")),
			MAKE_BOOL_GETTER(IsDropSoundEquip()), TOGGLE_CLICKED(IsDropSoundEquip, SetDropSoundEquip)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Healing Drops (Green)")),
			MAKE_BOOL_GETTER(IsDropSoundHeal()), TOGGLE_CLICKED(IsDropSoundHeal, SetDropSoundHeal)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Usable Drops (Yellow)")),
			MAKE_BOOL_GETTER(IsDropSoundUsable()), TOGGLE_CLICKED(IsDropSoundUsable, SetDropSoundUsable)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Misc Drops (Pink)")),
			MAKE_BOOL_GETTER(IsDropSoundMisc()), TOGGLE_CLICKED(IsDropSoundMisc, SetDropSoundMisc)) ]

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Camera"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Rotation Speed")),
			0.1f, 2.0f, 1, MAKE_FLOAT_GETTER(GetCameraSensitivity()), MAKE_FLOAT_SETTER(SetCameraSensitivity)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildSliderRow(FText::FromString(TEXT("Zoom Speed")),
			20.f, 200.f, 0, MAKE_FLOAT_GETTER(GetCameraZoomSpeed()), MAKE_FLOAT_SETTER(SetCameraZoomSpeed)) ]

		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Gameplay"))) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Auto-Attack (No Ctrl)")),
			MAKE_BOOL_GETTER(IsNoCtrl()), TOGGLE_CLICKED(IsNoCtrl, SetNoCtrl)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("No Shift for Support")),
			MAKE_BOOL_GETTER(IsNoShift()), TOGGLE_CLICKED(IsNoShift, SetNoShift)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Auto-Decline Trades")),
			MAKE_BOOL_GETTER(IsAutoDeclineTrades()), TOGGLE_CLICKED(IsAutoDeclineTrades, SetAutoDeclineTrades)) ]
		+ SScrollBox::Slot().Padding(0, 2)[ BuildToggleRow(FText::FromString(TEXT("Auto-Decline Party")),
			MAKE_BOOL_GETTER(IsAutoDeclineParty()), TOGGLE_CLICKED(IsAutoDeclineParty, SetAutoDeclineParty)) ];
}

// ============================================================
// Video Tab Content
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildVideoContent()
{
	auto GUS = UGameUserSettings::GetGameUserSettings();

	// Window mode getter/setter
	auto WMGetter = MakeShared<TFunction<int32()>>([GUS]() -> int32 {
		if (!GUS) return 2;
		return (int32)GUS->GetFullscreenMode();
	});
	auto WMSetter = MakeShared<TFunction<void(int32)>>([GUS](int32 Idx) {
		if (GUS) GUS->SetFullscreenMode((EWindowMode::Type)Idx);
	});

	// Resolution getter/setter
	auto ResGetter = MakeShared<TFunction<int32()>>([this, GUS]() -> int32 {
		if (!GUS) return 0;
		FIntPoint Cur = GUS->GetScreenResolution();
		FString CurStr = FString::Printf(TEXT("%dx%d"), Cur.X, Cur.Y);
		return FMath::Max(0, FindOptionIndex(ResolutionOptions, CurStr));
	});
	auto ResSetter = MakeShared<TFunction<void(int32)>>([this, GUS](int32 Idx) {
		if (GUS && ResolutionValues.IsValidIndex(Idx))
			GUS->SetScreenResolution(ResolutionValues[Idx]);
	});

	// VSync getter
	auto VSyncGetter = MakeShared<TFunction<bool()>>([GUS]() -> bool {
		return GUS ? GUS->IsVSyncEnabled() : false;
	});

	// Frame rate getter/setter
	auto FPSGetter = MakeShared<TFunction<int32()>>([this, GUS]() -> int32 {
		if (!GUS) return 4;
		float Limit = GUS->GetFrameRateLimit();
		if (Limit <= 0.f) return 4; // Unlimited
		for (int32 i = 0; i < FrameRateLimitValues.Num(); ++i)
		{
			if (FMath::IsNearlyEqual(Limit, FrameRateLimitValues[i], 1.f))
				return i;
		}
		return 4;
	});
	auto FPSSetter = MakeShared<TFunction<void(int32)>>([this, GUS](int32 Idx) {
		if (GUS && FrameRateLimitValues.IsValidIndex(Idx))
			GUS->SetFrameRateLimit(FrameRateLimitValues[Idx]);
	});

	// Overall quality getter/setter
	auto OvGetter = MakeShared<TFunction<int32()>>([GUS]() -> int32 {
		if (!GUS) return 3;
		int32 Level = GUS->GetOverallScalabilityLevel();
		return (Level < 0) ? 5 : Level; // -1 = Custom (index 5)
	});
	auto OvSetter = MakeShared<TFunction<void(int32)>>([this, GUS](int32 Idx) {
		if (!GUS || Idx >= 5) return; // Ignore "Custom" selection
		GUS->SetOverallScalabilityLevel(Idx);
		RefreshVideoDropdowns();
	});

	// Quality group helper macro
	#define QUALITY_GETTER(GetFunc) MakeShared<TFunction<int32()>>([GUS]() -> int32 { \
		return GUS ? GUS->GetFunc() : 3; })
	#define QUALITY_SETTER(SetFunc) MakeShared<TFunction<void(int32)>>([GUS](int32 Idx) { \
		if (GUS) GUS->SetFunc(Idx); })

	return SNew(SScrollBox)
		.ScrollBarVisibility(EVisibility::Collapsed)

		// ---- Display ----
		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Display"))) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Window Mode")),
			&WindowModeOptions, &WindowModeCombo, WMGetter, WMSetter) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Resolution")),
			&ResolutionOptions, &ResolutionCombo, ResGetter, ResSetter) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildToggleRow(FText::FromString(TEXT("VSync")), VSyncGetter,
			FOnClicked::CreateLambda([GUS]() -> FReply {
				if (GUS) GUS->SetVSyncEnabled(!GUS->IsVSyncEnabled());
				return FReply::Handled();
			})) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Frame Rate Limit")),
			&FrameRateLimitOptions, &FrameRateCombo, FPSGetter, FPSSetter) ]

		// ---- Quality ----
		+ SScrollBox::Slot()[ BuildSectionHeader(FText::FromString(TEXT("Quality"))) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Overall Quality")),
			&OverallQualityOptions, &OverallQualityCombo, OvGetter, OvSetter) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Shadow Quality")),
			&QualityLevelOptions, &ShadowCombo,
			QUALITY_GETTER(GetShadowQuality), QUALITY_SETTER(SetShadowQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Texture Quality")),
			&QualityLevelOptions, &TextureCombo,
			QUALITY_GETTER(GetTextureQuality), QUALITY_SETTER(SetTextureQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Anti-Aliasing")),
			&QualityLevelOptions, &AACombo,
			QUALITY_GETTER(GetAntiAliasingQuality), QUALITY_SETTER(SetAntiAliasingQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Effects Quality")),
			&QualityLevelOptions, &EffectsCombo,
			QUALITY_GETTER(GetVisualEffectQuality), QUALITY_SETTER(SetVisualEffectQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("View Distance")),
			&QualityLevelOptions, &ViewDistCombo,
			QUALITY_GETTER(GetViewDistanceQuality), QUALITY_SETTER(SetViewDistanceQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Post-Process")),
			&QualityLevelOptions, &PostProcCombo,
			QUALITY_GETTER(GetPostProcessingQuality), QUALITY_SETTER(SetPostProcessingQuality)) ]

		+ SScrollBox::Slot().Padding(0, 2)
		[ BuildDropdownRow(FText::FromString(TEXT("Foliage Quality")),
			&QualityLevelOptions, &FoliageCombo,
			QUALITY_GETTER(GetFoliageQuality), QUALITY_SETTER(SetFoliageQuality)) ]

		// ---- Apply / Auto-Detect buttons ----
		+ SScrollBox::Slot().Padding(0, 8, 0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 2, 0)
			[
				BuildButton(FText::FromString(TEXT("Apply")),
					FOnClicked::CreateSP(this, &SOptionsWidget::OnApplyVideoSettings))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 0, 0)
			[
				BuildButton(FText::FromString(TEXT("Auto-Detect")),
					FOnClicked::CreateSP(this, &SOptionsWidget::OnAutoDetect))
			]
		];

	#undef QUALITY_GETTER
	#undef QUALITY_SETTER
}

// ============================================================
// Video Options Population
// ============================================================

void SOptionsWidget::PopulateVideoOptions()
{
	// Window modes
	WindowModeOptions.Add(MakeShared<FString>(TEXT("Fullscreen")));
	WindowModeOptions.Add(MakeShared<FString>(TEXT("Windowed Fullscreen")));
	WindowModeOptions.Add(MakeShared<FString>(TEXT("Windowed")));

	// Frame rate limits
	FrameRateLimitOptions.Add(MakeShared<FString>(TEXT("30")));
	FrameRateLimitOptions.Add(MakeShared<FString>(TEXT("60")));
	FrameRateLimitOptions.Add(MakeShared<FString>(TEXT("120")));
	FrameRateLimitOptions.Add(MakeShared<FString>(TEXT("144")));
	FrameRateLimitOptions.Add(MakeShared<FString>(TEXT("Unlimited")));
	FrameRateLimitValues = { 30.f, 60.f, 120.f, 144.f, 0.f };

	// Quality levels (shared by all quality dropdowns)
	QualityLevelOptions.Add(MakeShared<FString>(TEXT("Low")));
	QualityLevelOptions.Add(MakeShared<FString>(TEXT("Medium")));
	QualityLevelOptions.Add(MakeShared<FString>(TEXT("High")));
	QualityLevelOptions.Add(MakeShared<FString>(TEXT("Epic")));
	QualityLevelOptions.Add(MakeShared<FString>(TEXT("Cinematic")));

	// Overall quality (same + Custom for mixed state)
	OverallQualityOptions = QualityLevelOptions;
	OverallQualityOptions.Add(MakeShared<FString>(TEXT("Custom")));

	// Resolutions from RHI
	FScreenResolutionArray RawResolutions;
	if (RHIGetAvailableResolutions(RawResolutions, true))
	{
		TSet<uint64> Seen;
		TArray<FIntPoint> Unique;
		for (const FScreenResolutionRHI& R : RawResolutions)
		{
			uint64 Key = ((uint64)R.Width << 32) | (uint64)R.Height;
			if (!Seen.Contains(Key))
			{
				Seen.Add(Key);
				Unique.Add(FIntPoint(R.Width, R.Height));
			}
		}
		Unique.Sort([](const FIntPoint& A, const FIntPoint& B) {
			return (A.X != B.X) ? (A.X > B.X) : (A.Y > B.Y);
		});
		for (const FIntPoint& Pt : Unique)
		{
			ResolutionOptions.Add(MakeShared<FString>(FString::Printf(TEXT("%dx%d"), Pt.X, Pt.Y)));
			ResolutionValues.Add(Pt);
		}
	}

	// Fallback: add current resolution if list is empty or doesn't contain it
	if (UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings())
	{
		FIntPoint Cur = GUS->GetScreenResolution();
		FString CurStr = FString::Printf(TEXT("%dx%d"), Cur.X, Cur.Y);
		if (FindOptionIndex(ResolutionOptions, CurStr) < 0)
		{
			ResolutionOptions.Insert(MakeShared<FString>(CurStr), 0);
			ResolutionValues.Insert(Cur, 0);
		}
	}
}

// ============================================================
// Refresh all video dropdowns (after auto-detect or overall change)
// ============================================================

void SOptionsWidget::RefreshVideoDropdowns()
{
	UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
	if (!GUS) return;

	auto RefreshCombo = [](TSharedPtr<SComboBox<TSharedPtr<FString>>>& Combo,
		const TArray<TSharedPtr<FString>>& Options, int32 Idx)
	{
		if (Combo.IsValid() && Options.IsValidIndex(Idx))
			Combo->SetSelectedItem(Options[Idx]);
	};

	RefreshCombo(WindowModeCombo, WindowModeOptions, (int32)GUS->GetFullscreenMode());

	FIntPoint CurRes = GUS->GetScreenResolution();
	FString CurResStr = FString::Printf(TEXT("%dx%d"), CurRes.X, CurRes.Y);
	RefreshCombo(ResolutionCombo, ResolutionOptions, FMath::Max(0, FindOptionIndex(ResolutionOptions, CurResStr)));

	float FPSLimit = GUS->GetFrameRateLimit();
	int32 FPSIdx = 4;
	for (int32 i = 0; i < FrameRateLimitValues.Num(); ++i)
	{
		if (FMath::IsNearlyEqual(FPSLimit, FrameRateLimitValues[i], 1.f)) { FPSIdx = i; break; }
	}
	RefreshCombo(FrameRateCombo, FrameRateLimitOptions, FPSIdx);

	int32 OvLevel = GUS->GetOverallScalabilityLevel();
	RefreshCombo(OverallQualityCombo, OverallQualityOptions, (OvLevel < 0) ? 5 : OvLevel);
	RefreshCombo(ShadowCombo, QualityLevelOptions, GUS->GetShadowQuality());
	RefreshCombo(TextureCombo, QualityLevelOptions, GUS->GetTextureQuality());
	RefreshCombo(AACombo, QualityLevelOptions, GUS->GetAntiAliasingQuality());
	RefreshCombo(EffectsCombo, QualityLevelOptions, GUS->GetVisualEffectQuality());
	RefreshCombo(ViewDistCombo, QualityLevelOptions, GUS->GetViewDistanceQuality());
	RefreshCombo(PostProcCombo, QualityLevelOptions, GUS->GetPostProcessingQuality());
	RefreshCombo(FoliageCombo, QualityLevelOptions, GUS->GetFoliageQuality());
}

// ============================================================
// Apply / Auto-Detect
// ============================================================

FReply SOptionsWidget::OnApplyVideoSettings()
{
	UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
	if (!GUS) return FReply::Handled();

	// Remember current state for potential revert
	PrevResolution = GUS->GetLastConfirmedScreenResolution();
	PrevWindowMode = (int32)GUS->GetLastConfirmedFullscreenMode();

	GUS->ApplySettings(false);

	// If resolution or window mode changed, show confirmation
	if (GUS->GetScreenResolution() != PrevResolution ||
		(int32)GUS->GetFullscreenMode() != PrevWindowMode)
	{
		ShowResolutionConfirmation();
	}
	else
	{
		GUS->ConfirmVideoMode();
		GUS->SaveSettings();
	}

	return FReply::Handled();
}

FReply SOptionsWidget::OnAutoDetect()
{
	UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings();
	if (!GUS) return FReply::Handled();

	GUS->RunHardwareBenchmark();
	GUS->ApplyHardwareBenchmarkResults();
	GUS->ApplySettings(false);
	GUS->SaveSettings();
	RefreshVideoDropdowns();

	return FReply::Handled();
}

// ============================================================
// Resolution Confirmation Dialog
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildResolutionConfirmOverlay()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(OptColors::OverlayBg)
			.Padding(FMargin(0.f))
			[
				SNew(SBox)
				.WidthOverride(280.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(OptColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(OptColors::PanelBrown)
						.Padding(FMargin(12.f, 8.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Keep these display settings?")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(OptColors::TextShadow)
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
							[
								SAssignNew(ConfirmCountdownText, STextBlock)
								.Text(FText::FromString(TEXT("Reverting in 15s...")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FSlateColor(OptColors::TextDim))
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
								[
									BuildButton(FText::FromString(TEXT("Yes")),
										FOnClicked::CreateSP(this, &SOptionsWidget::OnConfirmResolution))
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0, 0, 0)
								[
									BuildButton(FText::FromString(TEXT("Revert")),
										FOnClicked::CreateSP(this, &SOptionsWidget::OnRevertResolution))
								]
							]
						]
					]
				]
			]
		];
}

void SOptionsWidget::ShowResolutionConfirmation()
{
	bWaitingForConfirm = true;
	ConfirmTimeRemaining = 15.f;
	if (ConfirmOverlay.IsValid())
		ConfirmOverlay->SetVisibility(EVisibility::Visible);
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(
		this, &SOptionsWidget::TickConfirmCountdown));
}

FReply SOptionsWidget::OnConfirmResolution()
{
	bWaitingForConfirm = false;
	if (ConfirmOverlay.IsValid())
		ConfirmOverlay->SetVisibility(EVisibility::Collapsed);
	if (UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings())
	{
		GUS->ConfirmVideoMode();
		GUS->SaveSettings();
	}
	return FReply::Handled();
}

FReply SOptionsWidget::OnRevertResolution()
{
	bWaitingForConfirm = false;
	if (ConfirmOverlay.IsValid())
		ConfirmOverlay->SetVisibility(EVisibility::Collapsed);
	if (UGameUserSettings* GUS = UGameUserSettings::GetGameUserSettings())
	{
		GUS->RevertVideoMode();
		GUS->ApplySettings(false);
	}
	RefreshVideoDropdowns();
	return FReply::Handled();
}

EActiveTimerReturnType SOptionsWidget::TickConfirmCountdown(double InCurrentTime, float InDeltaTime)
{
	if (!bWaitingForConfirm)
		return EActiveTimerReturnType::Stop;

	ConfirmTimeRemaining -= InDeltaTime;
	if (ConfirmCountdownText.IsValid())
	{
		int32 Secs = FMath::Max(0, FMath::CeilToInt(ConfirmTimeRemaining));
		ConfirmCountdownText->SetText(FText::FromString(
			FString::Printf(TEXT("Reverting in %ds..."), Secs)));
	}
	if (ConfirmTimeRemaining <= 0.f)
	{
		OnRevertResolution();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

// ============================================================
// Helper
// ============================================================

int32 SOptionsWidget::FindOptionIndex(const TArray<TSharedPtr<FString>>& Options, const FString& Value) const
{
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		if (Options[i].IsValid() && *Options[i] == Value)
			return i;
	}
	return -1;
}

// ============================================================
// Section Header
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildSectionHeader(const FText& Title)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 2)
		[
			SNew(SBox).HeightOverride(1.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(OptColors::GoldDivider)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(OptColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(OptColors::TextShadow)
		];
}

// ============================================================
// Styled Button
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildButton(const FText& Label, FOnClicked OnClicked)
{
	static FButtonStyle OptBtnStyle = []()
	{
		FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		S.Normal.TintColor = FSlateColor(OptColors::ButtonNormal);
		S.Normal.DrawAs = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(OptColors::ButtonHover);
		S.Hovered.DrawAs = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(OptColors::ButtonPressed);
		S.Pressed.DrawAs = ESlateBrushDrawType::Box;
		return S;
	}();

	return SNew(SBox).HeightOverride(28.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(OptColors::ButtonBorder)
		.Padding(FMargin(1.f))
		[
			SNew(SButton)
			.ButtonStyle(&OptBtnStyle)
			.OnClicked(OnClicked)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(OptColors::TextShadow)
			]
		]
	];
}

// ============================================================
// Toggle Row
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildToggleRow(const FText& Label,
	TSharedRef<TFunction<bool()>> IsOnGetter, FOnClicked OnToggle)
{
	static FButtonStyle ToggleStyle = []()
	{
		FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		S.Normal.TintColor = FSlateColor(OptColors::ButtonNormal);
		S.Normal.DrawAs = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(OptColors::ButtonHover);
		S.Hovered.DrawAs = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(OptColors::ButtonPressed);
		S.Pressed.DrawAs = ESlateBrushDrawType::Box;
		return S;
	}();

	return SNew(SBox).HeightOverride(28.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(OptColors::ButtonBorder)
		.Padding(FMargin(1.f))
		[
			SNew(SButton)
			.ButtonStyle(&ToggleStyle)
			.OnClicked(OnToggle)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(6, 0, 0, 0)
				[
					SNew(STextBlock).Text(Label)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
					.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(OptColors::TextShadow)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([IsOnGetter]() -> FText {
						return FText::FromString((*IsOnGetter)() ? TEXT("ON") : TEXT("OFF"));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity_Lambda([IsOnGetter]() -> FSlateColor {
						return FSlateColor((*IsOnGetter)() ? OptColors::ToggleOn : OptColors::ToggleOff);
					})
					.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(OptColors::TextShadow)
				]
			]
		]
	];
}

// ============================================================
// Slider Row
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildSliderRow(const FText& Label,
	float MinVal, float MaxVal, int32 Decimals,
	TSharedRef<TFunction<float()>> Getter,
	TSharedRef<TFunction<void(float)>> Setter)
{
	float Range = MaxVal - MinVal;

	static FSliderStyle SldrStyle = []()
	{
		FSliderStyle S = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
		S.NormalBarImage.TintColor = FSlateColor(OptColors::SliderBar);
		S.HoveredBarImage.TintColor = FSlateColor(OptColors::SliderBar);
		S.NormalThumbImage.TintColor = FSlateColor(OptColors::SliderThumb);
		S.HoveredThumbImage.TintColor = FSlateColor(OptColors::GoldHighlight);
		return S;
	}();

	return SNew(SBox).HeightOverride(28.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(OptColors::ButtonBorder)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(OptColors::ButtonNormal)
			.Padding(FMargin(6.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(110.f)
					[
						SNew(STextBlock).Text(Label)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
						.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(OptColors::TextShadow)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SSlider).Style(&SldrStyle)
					.Value_Lambda([Getter, MinVal, Range]() -> float {
						return (Range > 0.f) ? FMath::Clamp(((*Getter)() - MinVal) / Range, 0.f, 1.f) : 0.f;
					})
					.OnValueChanged_Lambda([Setter, MinVal, Range](float N) {
						(*Setter)(MinVal + N * Range);
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(42.f)
					[
						SNew(STextBlock)
						.Text_Lambda([Getter, Decimals]() -> FText {
							float V = (*Getter)();
							if (Decimals <= 0) return FText::FromString(FString::Printf(TEXT("%d"), FMath::RoundToInt(V)));
							if (Decimals == 1) return FText::FromString(FString::Printf(TEXT("%.1f"), V));
							return FText::FromString(FString::Printf(TEXT("%.2f"), V));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(OptColors::TextDim))
						.Justification(ETextJustify::Right)
						.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(OptColors::TextShadow)
					]
				]
			]
		]
	];
}

// ============================================================
// Dropdown Row
// ============================================================

TSharedRef<SWidget> SOptionsWidget::BuildDropdownRow(const FText& Label,
	TArray<TSharedPtr<FString>>* OptionsSource,
	TSharedPtr<SComboBox<TSharedPtr<FString>>>* OutComboPtr,
	TSharedRef<TFunction<int32()>> CurrentIndexGetter,
	TSharedRef<TFunction<void(int32)>> OnSelected)
{
	static FButtonStyle ComboRowBtnStyle = []()
	{
		FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		S.Normal.TintColor = FSlateColor(OptColors::ButtonNormal);
		S.Normal.DrawAs = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(OptColors::ButtonHover);
		S.Hovered.DrawAs = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(OptColors::ButtonPressed);
		S.Pressed.DrawAs = ESlateBrushDrawType::Box;
		return S;
	}();

	TSharedPtr<SComboBox<TSharedPtr<FString>>> Combo;

	// Determine initial selected item
	int32 InitIdx = (*CurrentIndexGetter)();
	TSharedPtr<FString> InitialItem;
	if (OptionsSource && OptionsSource->IsValidIndex(InitIdx))
		InitialItem = (*OptionsSource)[InitIdx];

	TSharedRef<SWidget> Row = SNew(SBox).HeightOverride(28.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(OptColors::ButtonBorder)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(OptColors::ButtonNormal)
			.Padding(FMargin(6.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(130.f)
					[
						SNew(STextBlock).Text(Label)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
						.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(OptColors::TextShadow)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SAssignNew(Combo, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(OptionsSource)
					.InitiallySelectedItem(InitialItem)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget> {
						return SNew(STextBlock)
							.Text(FText::FromString(Item.IsValid() ? *Item : TEXT("")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
							.ColorAndOpacity(FSlateColor(OptColors::TextPrimary));
					})
					.OnSelectionChanged_Lambda([OptionsSource, OnSelected](TSharedPtr<FString> Item, ESelectInfo::Type) {
						if (!OptionsSource || !Item.IsValid()) return;
						int32 Idx = OptionsSource->IndexOfByKey(Item);
						if (Idx >= 0) (*OnSelected)(Idx);
					})
					[
						SNew(STextBlock)
						.Text_Lambda([CurrentIndexGetter, OptionsSource]() -> FText {
							if (!OptionsSource) return FText::GetEmpty();
							int32 Idx = (*CurrentIndexGetter)();
							if (OptionsSource->IsValidIndex(Idx))
								return FText::FromString(*(*OptionsSource)[Idx]);
							return FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FSlateColor(OptColors::TextPrimary))
					]
				]
			]
		]
	];

	if (OutComboPtr)
		*OutComboPtr = Combo;

	return Row;
}
