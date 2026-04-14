#include "SEscapeMenuWidget.h"
#include "EscapeMenuSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette
// ============================================================

namespace EscColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);

	// Button colors
	static const FLinearColor ButtonNormal  (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor ButtonHover   (0.45f, 0.33f, 0.18f, 1.f);
	static const FLinearColor ButtonPressed (0.25f, 0.16f, 0.09f, 1.f);
	static const FLinearColor ButtonBorder  (0.55f, 0.42f, 0.20f, 1.f);

	// Red for exit/danger buttons
	static const FLinearColor ExitRed       (0.65f, 0.18f, 0.18f, 1.f);
	static const FLinearColor ExitRedHover  (0.80f, 0.25f, 0.25f, 1.f);
}

// ============================================================
// Construction
// ============================================================

void SEscapeMenuWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(220.f)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(EscColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(EscColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(EscColors::PanelBrown)
					.Padding(FMargin(8.f, 6.f))
					[
						SNew(SVerticalBox)

						// Title bar
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// Gold divider
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
						[
							SNew(SBox).HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(EscColors::GoldDivider)
							]
						]

						// ---- Dead-only: Save Point (Respawn) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							SNew(SBox)
							.Visibility_Lambda([this]()
							{
								UEscapeMenuSubsystem* Sub = OwningSubsystem.Get();
								return (Sub && Sub->IsPlayerDead()) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								BuildButton(FText::FromString(TEXT("Save Point")),
									FOnClicked::CreateLambda([this]() -> FReply
									{
										if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
											Sub->OnRespawnPressed();
										return FReply::Handled();
									}))
							]
						]

						// ---- Character Select (always visible) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							BuildButton(FText::FromString(TEXT("Character Select")),
								FOnClicked::CreateLambda([this]() -> FReply
								{
									if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
										Sub->OnCharacterSelectPressed();
									return FReply::Handled();
								}))
						]

						// ---- Hotkey (alive only) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							SNew(SBox)
							.Visibility_Lambda([this]()
							{
								UEscapeMenuSubsystem* Sub = OwningSubsystem.Get();
								return (Sub && !Sub->IsPlayerDead()) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								BuildButton(FText::FromString(TEXT("Hotkey")),
									FOnClicked::CreateLambda([this]() -> FReply
									{
										if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
											Sub->OnHotkeyPressed();
										return FReply::Handled();
									}))
							]
						]

						// ---- Option (alive only) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							SNew(SBox)
							.Visibility_Lambda([this]()
							{
								UEscapeMenuSubsystem* Sub = OwningSubsystem.Get();
								return (Sub && !Sub->IsPlayerDead()) ? EVisibility::Visible : EVisibility::Collapsed;
							})
							[
								BuildButton(FText::FromString(TEXT("Option")),
									FOnClicked::CreateLambda([this]() -> FReply
									{
										if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
											Sub->OnOptionPressed();
										return FReply::Handled();
									}))
							]
						]

						// ---- Exit (always visible) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							BuildButton(FText::FromString(TEXT("Exit")),
								FOnClicked::CreateLambda([this]() -> FReply
								{
									if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
										Sub->OnExitGamePressed();
									return FReply::Handled();
								}))
						]

						// ---- Cancel (always visible) ----
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
						[
							BuildButton(FText::FromString(TEXT("Cancel")),
								FOnClicked::CreateLambda([this]() -> FReply
								{
									if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
										Sub->HideMenu();
									return FReply::Handled();
								}))
						]
					]
				]
			]
		]
	];
}

// ============================================================
// Key handling — ESC closes the menu
// ============================================================

FReply SEscapeMenuWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (UEscapeMenuSubsystem* Sub = OwningSubsystem.Get())
		{
			Sub->HideMenu();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Title Bar
// ============================================================

TSharedRef<SWidget> SEscapeMenuWidget::BuildTitleBar()
{
	return SNew(SBox)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Select Option")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(EscColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(EscColors::TextShadow)
		];
}

// ============================================================
// Styled Button
// ============================================================

TSharedRef<SWidget> SEscapeMenuWidget::BuildButton(const FText& Label, FOnClicked OnClicked)
{
	// Static style — SButton stores a pointer, so the style must outlive the widget.
	static FButtonStyle MenuButtonStyle = []()
	{
		FButtonStyle S = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
		S.Normal.TintColor = FSlateColor(EscColors::ButtonNormal);
		S.Normal.DrawAs = ESlateBrushDrawType::Box;
		S.Hovered.TintColor = FSlateColor(EscColors::ButtonHover);
		S.Hovered.DrawAs = ESlateBrushDrawType::Box;
		S.Pressed.TintColor = FSlateColor(EscColors::ButtonPressed);
		S.Pressed.DrawAs = ESlateBrushDrawType::Box;
		return S;
	}();

	return SNew(SBox)
		.HeightOverride(28.f)
		[
			// Gold border around the button
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(EscColors::ButtonBorder)
			.Padding(FMargin(1.f))
			[
				SNew(SButton)
				.ButtonStyle(&MenuButtonStyle)
				.OnClicked(OnClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(EscColors::TextPrimary))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(EscColors::TextShadow)
				]
			]
		];
}
