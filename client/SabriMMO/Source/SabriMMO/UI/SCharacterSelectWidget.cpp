// SCharacterSelectWidget.cpp -- Character selection screen (RO Classic brown/gold theme)
// 3x3 card grid on the left, detail panel on the right, delete confirmation overlay.

#include "SCharacterSelectWidget.h"
#include "LoginFlowSubsystem.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette -- Character Select
// NOTE: Use FLinearColor() directly -- FColor() applies sRGB->linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace CharSelectColors
{
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium  (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark     (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider  (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary  (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright   (1.0f, 1.0f, 1.0f, 1.f);
	static const FLinearColor TextDim      (0.60f, 0.55f, 0.45f, 1.f);
	static const FLinearColor TextShadow   (0.0f, 0.0f, 0.0f, 0.85f);
	static const FLinearColor CardBg       (0.28f, 0.19f, 0.11f, 1.f);
	static const FLinearColor CardSelected (0.50f, 0.38f, 0.15f, 0.8f);
	static const FLinearColor CardEmpty    (0.20f, 0.14f, 0.08f, 0.6f);
	static const FLinearColor ButtonGreen  (0.20f, 0.55f, 0.20f, 1.f);
	static const FLinearColor ButtonRed    (0.55f, 0.15f, 0.15f, 1.f);
	static const FLinearColor ButtonGold   (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor HPRed        (0.85f, 0.15f, 0.15f, 1.f);
	static const FLinearColor SPBlue       (0.20f, 0.45f, 0.90f, 1.f);
	static const FLinearColor EXPYellow    (0.90f, 0.75f, 0.10f, 1.f);
	static const FLinearColor BarBg        (0.10f, 0.07f, 0.04f, 1.f);
	static const FLinearColor DimOverlay   (0.0f, 0.0f, 0.0f, 0.7f);
	static const FLinearColor ErrorRed     (0.90f, 0.20f, 0.20f, 1.f);
	static const FLinearColor ZuzucoinGold (0.95f, 0.82f, 0.48f, 1.f);
}

// ============================================================
// Construction
// ============================================================
void SCharacterSelectWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SNew(SOverlay)

		// --- Slot [0]: Main content ---
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(780.f)
			[
				// Layer 1: Gold trim border (outermost, 2px)
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CharSelectColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Layer 2: Dark inset (1px)
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CharSelectColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Layer 3: Brown content panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::PanelBrown)
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)

							// --- Title: "Character Select" ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(CharSelectColors::PanelDark)
								.Padding(FMargin(0.f, 6.f))
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Character Select")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FSlateColor(CharSelectColors::GoldHighlight))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CharSelectColors::TextShadow)
								]
							]

							// --- Gold divider after title ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(1.f)
								.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(CharSelectColors::GoldDivider)
								]
							]

							// --- Main content: Card Grid (left) + Detail Panel (right) ---
							+ SVerticalBox::Slot()
							.FillHeight(1.f)
							[
								SNew(SHorizontalBox)

								// Left: Card Grid area (60%)
								+ SHorizontalBox::Slot()
								.FillWidth(0.6f)
								.Padding(FMargin(6.f))
								[
									SAssignNew(CardGridBox, SVerticalBox)
								]

								// Vertical gold divider (2px wide)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(2.f)
									[
										SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
										.BorderBackgroundColor(CharSelectColors::GoldDivider)
									]
								]

								// Right: Detail Panel area (40%)
								+ SHorizontalBox::Slot()
								.FillWidth(0.4f)
								.Padding(FMargin(6.f))
								[
									SAssignNew(DetailPanelBox, SVerticalBox)
								]
							]

							// --- Gold divider before buttons ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(1.f)
								.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(CharSelectColors::GoldDivider)
								]
							]

							// --- Status message text ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(FMargin(0.f, 2.f))
							[
								SAssignNew(StatusText, STextBlock)
								.Text(FText::GetEmpty())
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
								.Visibility(EVisibility::Collapsed)
							]

							// --- Button row ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.f, 4.f, 8.f, 4.f))
							[
								SNew(SHorizontalBox)

								// "Back" button (dark)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CharSelectColors::PanelDark)
									.OnClicked(this, &SCharacterSelectWidget::OnBackClicked)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(12.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Back")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CharSelectColors::TextShadow)
										]
									]
								]

								// Fill spacer
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNew(SBox)
								]

								// "Delete" button (red, disabled if no selection)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CharSelectColors::ButtonRed)
									.OnClicked(this, &SCharacterSelectWidget::OnDeleteClicked)
									.IsEnabled_Lambda([this]() -> bool
									{
										return SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num();
									})
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(12.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Delete")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.ColorAndOpacity(FSlateColor(CharSelectColors::TextBright))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CharSelectColors::TextShadow)
										]
									]
								]

								// 8px spacer
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(8.f)
								]

								// "PLAY" button (green, large, disabled if no selection)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CharSelectColors::ButtonGreen)
									.OnClicked(this, &SCharacterSelectWidget::OnPlayClicked)
									.IsEnabled_Lambda([this]() -> bool
									{
										return SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num();
									})
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(24.f, 6.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("PLAY")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
											.ColorAndOpacity(FSlateColor(CharSelectColors::TextBright))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CharSelectColors::TextShadow)
										]
									]
								]
							]

							// 4px bottom padding
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(4.f)
							]
						]
					]
				]
			]
		]

		// --- Slot [1]: Delete confirmation overlay (hidden by default) ---
		+ SOverlay::Slot()
		[
			SAssignNew(DeleteOverlay, SBox)
			.Visibility(EVisibility::Collapsed)
			[
				BuildDeleteConfirmationOverlay()
			]
		]
	];

	// Initialize with empty state
	RebuildCardGrid();
	RebuildDetailPanel();
}

// ============================================================
// Populate Characters
// ============================================================
void SCharacterSelectWidget::PopulateCharacters(const TArray<FCharacterData>& Characters)
{
	CachedCharacters = Characters;

	RebuildCardGrid();

	// Auto-select the first character if available
	if (CachedCharacters.Num() > 0)
	{
		SelectSlot(0);
	}
	else
	{
		SelectedIndex = -1;
		RebuildDetailPanel();
	}
}

// ============================================================
// Card Grid
// ============================================================
TSharedRef<SWidget> SCharacterSelectWidget::BuildCardGrid()
{
	// This method is unused; grid is built inline via RebuildCardGrid.
	return SNullWidget::NullWidget;
}

void SCharacterSelectWidget::RebuildCardGrid()
{
	if (!CardGridBox.IsValid())
	{
		return;
	}

	CardGridBox->ClearChildren();

	// Build 3 rows of 3 cards
	for (int32 Row = 0; Row < 3; ++Row)
	{
		CardGridBox->AddSlot()
		.FillHeight(1.f)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(2.f, 0.f))
			[
				BuildCharacterCard(Row * GridColumns + 0)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(2.f, 0.f))
			[
				BuildCharacterCard(Row * GridColumns + 1)
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(FMargin(2.f, 0.f))
			[
				BuildCharacterCard(Row * GridColumns + 2)
			]
		];
	}
}

// ============================================================
// Build a single character card
// ============================================================
TSharedRef<SWidget> SCharacterSelectWidget::BuildCharacterCard(int32 SlotIndex)
{
	const bool bHasCharacter = SlotIndex < CachedCharacters.Num();
	const bool bIsSelected = (SlotIndex == SelectedIndex) && bHasCharacter;

	// Determine card border and background colors
	const FLinearColor BorderColor = bIsSelected
		? CharSelectColors::GoldTrim
		: CharSelectColors::PanelDark;

	const FLinearColor BgColor = bHasCharacter
		? (bIsSelected ? CharSelectColors::CardSelected : CharSelectColors::CardBg)
		: CharSelectColors::CardEmpty;

	if (bHasCharacter)
	{
		// --- Occupied card ---
		const FCharacterData& Character = CachedCharacters[SlotIndex];
		const FString LevelText = FString::Printf(TEXT("Lv. %d / Job Lv. %d"), Character.Level, Character.JobLevel);

		return SNew(SBox)
			.MinDesiredHeight(110.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(BorderColor)
				.Padding(FMargin(2.f))
				.OnMouseButtonDown_Lambda([this, SlotIndex](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
				{
					if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
					{
						SelectSlot(SlotIndex);
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(BgColor)
					.Padding(FMargin(4.f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)

						// Character name (Bold 9, centered)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 2.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(Character.Name))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
							.ColorAndOpacity(FSlateColor(
								bIsSelected ? CharSelectColors::GoldHighlight : CharSelectColors::TextPrimary))
							.ShadowOffset(FVector2D(1, 1))
							.ShadowColorAndOpacity(CharSelectColors::TextShadow)
						]

						// Job class (Regular 8, centered, dim)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FormatJobClass(Character.JobClass)))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
						]

						// Level info (Regular 8, centered)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(LevelText))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
						]
					]
				]
			];
	}
	else
	{
		// --- Empty card ---
		return SNew(SBox)
			.MinDesiredHeight(110.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CharSelectColors::PanelDark)
				.Padding(FMargin(2.f))
				.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
				{
					if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
					{
						if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
						{
							Subsystem->OnCreateCharacterRequested();
						}
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(BgColor)
					.Padding(FMargin(4.f))
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)

						// Large "+" (Bold 16, centered, dim)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
							.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
						]

						// "Create" text (Regular 8, centered, dim)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Create")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
						]
					]
				]
			];
	}
}

// ============================================================
// Detail Panel
// ============================================================
TSharedRef<SWidget> SCharacterSelectWidget::BuildDetailPanel()
{
	// This method is unused; detail panel is built inline via RebuildDetailPanel.
	return SNullWidget::NullWidget;
}

void SCharacterSelectWidget::RebuildDetailPanel()
{
	if (!DetailPanelBox.IsValid())
	{
		return;
	}

	DetailPanelBox->ClearChildren();

	// No character selected — show placeholder
	if (SelectedIndex < 0 || SelectedIndex >= CachedCharacters.Num())
	{
		DetailPanelBox->AddSlot()
		.FillHeight(1.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Select a character\nor create a new one")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
			.Justification(ETextJustify::Center)
		];
		return;
	}

	const FCharacterData& Character = CachedCharacters[SelectedIndex];

	// --- Character name ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 2.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(Character.Name))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		.ColorAndOpacity(FSlateColor(CharSelectColors::GoldHighlight))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(CharSelectColors::TextShadow)
	];

	// --- Job class + gender ---
	const FString JobGenderText = FString::Printf(TEXT("%s  %s"),
		*FormatJobClass(Character.JobClass),
		*FormatJobClass(Character.Gender));

	DetailPanelBox->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(JobGenderText))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
	];

	// --- Gold divider ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f))
	[
		SNew(SBox)
		.HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CharSelectColors::GoldDivider)
		]
	];

	// --- Base Level / Job Level ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Base Level: %d"), Character.Level)))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
	];

	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 1.f, 0.f, 4.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Job Level: %d"), Character.JobLevel)))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
	];

	// --- HP bar ---
	{
		const FString HPText = FString::Printf(TEXT("HP: %d / %d"), Character.Health, Character.MaxHealth);
		const float HPPercent = (Character.MaxHealth > 0)
			? FMath::Clamp(static_cast<float>(Character.Health) / static_cast<float>(Character.MaxHealth), 0.f, 1.f)
			: 0.f;

		DetailPanelBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 1.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(HPText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
			[
				SNew(SBox)
				.HeightOverride(10.f)
				[
					SNew(SOverlay)

					// Bar background
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::BarBg)
					]

					// Bar fill
					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(HPPercent)
						.FillColorAndOpacity(CharSelectColors::HPRed)
						.BackgroundImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.FillImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					]
				]
			]
		];
	}

	// --- SP bar ---
	{
		const FString SPText = FString::Printf(TEXT("SP: %d / %d"), Character.Mana, Character.MaxMana);
		const float SPPercent = (Character.MaxMana > 0)
			? FMath::Clamp(static_cast<float>(Character.Mana) / static_cast<float>(Character.MaxMana), 0.f, 1.f)
			: 0.f;

		DetailPanelBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(SPText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
			[
				SNew(SBox)
				.HeightOverride(10.f)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::BarBg)
					]

					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(SPPercent)
						.FillColorAndOpacity(CharSelectColors::SPBlue)
						.BackgroundImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.FillImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					]
				]
			]
		];
	}

	// --- Gold divider before stats ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f))
	[
		SNew(SBox)
		.HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CharSelectColors::GoldDivider)
		]
	];

	// --- Stats grid: STR/AGI/VIT on left, INT/DEX/LUK on right ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	[
		SNew(SHorizontalBox)

		// Left column: STR, AGI, VIT
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("STR"), FString::FromInt(Character.Str))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("AGI"), FString::FromInt(Character.Agi))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("VIT"), FString::FromInt(Character.Vit))
			]
		]

		// Right column: INT, DEX, LUK
		+ SHorizontalBox::Slot()
		.FillWidth(0.5f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("INT"), FString::FromInt(Character.IntStat))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("DEX"), FString::FromInt(Character.Dex))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildStatRow(TEXT("LUK"), FString::FromInt(Character.Luk))
			]
		]
	];

	// --- Stat Points remaining ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Stat Points: %d"), Character.StatPoints)))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
	];

	// --- Gold divider before economy/EXP ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 4.f))
	[
		SNew(SBox)
		.HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CharSelectColors::GoldDivider)
		]
	];

	// --- Zuzucoin ---
	DetailPanelBox->AddSlot()
	.AutoHeight()
	.Padding(FMargin(0.f, 1.f))
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Zuzucoin: %s"), *FText::AsNumber(Character.Zuzucoin).ToString())))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(CharSelectColors::ZuzucoinGold))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(CharSelectColors::TextShadow)
	];

	// --- Base EXP bar ---
	{
		// EXP bars use a simple percentage representation; exact max values come from server tables.
		// For display we show the raw value and an approximate visual bar.
		const FString BaseExpText = FString::Printf(TEXT("Base EXP: %lld"), Character.BaseExp);

		DetailPanelBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(BaseExpText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
			[
				SNew(SBox)
				.HeightOverride(8.f)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::BarBg)
					]

					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(0.f) // Exact percentage requires EXP table lookup; shown as raw value
						.FillColorAndOpacity(CharSelectColors::EXPYellow)
						.BackgroundImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.FillImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					]
				]
			]
		];
	}

	// --- Job EXP bar ---
	{
		const FString JobExpText = FString::Printf(TEXT("Job EXP: %lld"), Character.JobExp);

		DetailPanelBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f, 0.f, 0.f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(JobExpText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.f, 1.f, 0.f, 0.f))
			[
				SNew(SBox)
				.HeightOverride(8.f)
				[
					SNew(SOverlay)

					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::BarBg)
					]

					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(0.f) // Exact percentage requires EXP table lookup; shown as raw value
						.FillColorAndOpacity(CharSelectColors::EXPYellow)
						.BackgroundImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.FillImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					]
				]
			]
		];
	}
}

// ============================================================
// Slot Selection
// ============================================================
void SCharacterSelectWidget::SelectSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= CachedCharacters.Num())
	{
		return;
	}

	SelectedIndex = SlotIndex;
	RebuildCardGrid();
	RebuildDetailPanel();
}

// ============================================================
// Delete Confirmation Overlay
// ============================================================
TSharedRef<SWidget> SCharacterSelectWidget::BuildDeleteConfirmationOverlay()
{
	return SNew(SOverlay)

		// Dimmed full-viewport background
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CharSelectColors::DimOverlay)
		]

		// Centered dialog
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(320.f)
			[
				// Gold trim border
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CharSelectColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Dark inset
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CharSelectColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Brown content panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CharSelectColors::PanelBrown)
						.Padding(FMargin(12.f, 10.f))
						[
							SNew(SVerticalBox)

							// Title: "Delete [CharName]?"
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(FMargin(0.f, 0.f, 0.f, 6.f))
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText
								{
									if (SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num())
									{
										return FText::FromString(FString::Printf(TEXT("Delete %s?"),
											*CachedCharacters[SelectedIndex].Name));
									}
									return FText::FromString(TEXT("Delete Character?"));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FSlateColor(CharSelectColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(CharSelectColors::TextShadow)
							]

							// Instruction text
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(FMargin(0.f, 0.f, 0.f, 8.f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Enter your password to confirm")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
							]

							// Password field
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
							[
								SAssignNew(DeletePasswordField, SEditableTextBox)
								.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
								.BackgroundColor(FLinearColor(0.15f, 0.10f, 0.06f, 1.f))
								.ForegroundColor(FSlateColor(CharSelectColors::TextBright))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								.IsPassword(true)
							]

							// Error text (hidden by default)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
							[
								SAssignNew(DeleteErrorText, STextBlock)
								.Text(FText::GetEmpty())
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(FSlateColor(CharSelectColors::ErrorRed))
								.Visibility(EVisibility::Collapsed)
							]

							// Gold divider
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(0.f, 2.f, 0.f, 6.f))
							[
								SNew(SBox)
								.HeightOverride(1.f)
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(CharSelectColors::GoldDivider)
								]
							]

							// Button row: Confirm Delete + Cancel
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)

								// "Confirm Delete" button (red)
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CharSelectColors::ButtonRed)
									.OnClicked(this, &SCharacterSelectWidget::OnConfirmDeleteClicked)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(8.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Confirm Delete")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.ColorAndOpacity(FSlateColor(CharSelectColors::TextBright))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CharSelectColors::TextShadow)
										]
									]
								]

								// 8px spacer
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(8.f)
								]

								// "Cancel" button (dark)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CharSelectColors::PanelDark)
									.OnClicked(this, &SCharacterSelectWidget::OnCancelDeleteClicked)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(12.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Cancel")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
											.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CharSelectColors::TextShadow)
										]
									]
								]
							]
						]
					]
				]
			]
		];
}

void SCharacterSelectWidget::ShowDeleteConfirmation()
{
	if (DeleteOverlay.IsValid())
	{
		DeleteOverlay->SetVisibility(EVisibility::Visible);
	}

	// Clear previous state
	if (DeletePasswordField.IsValid())
	{
		DeletePasswordField->SetText(FText::GetEmpty());
	}

	if (DeleteErrorText.IsValid())
	{
		DeleteErrorText->SetText(FText::GetEmpty());
		DeleteErrorText->SetVisibility(EVisibility::Collapsed);
	}
}

void SCharacterSelectWidget::HideDeleteConfirmation()
{
	if (DeleteOverlay.IsValid())
	{
		DeleteOverlay->SetVisibility(EVisibility::Collapsed);
	}
}

// ============================================================
// Status Message
// ============================================================
void SCharacterSelectWidget::ShowStatusMessage(const FString& Message)
{
	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetVisibility(Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
	}
}

// ============================================================
// Button Handlers
// ============================================================
FReply SCharacterSelectWidget::OnPlayClicked()
{
	UAudioSubsystem::PlayUIClickStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num())
	{
		if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
		{
			Subsystem->OnPlayCharacter(CachedCharacters[SelectedIndex]);
		}
	}
	return FReply::Handled();
}

FReply SCharacterSelectWidget::OnDeleteClicked()
{
	UAudioSubsystem::PlayUIClickStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num())
	{
		ShowDeleteConfirmation();
	}
	return FReply::Handled();
}

FReply SCharacterSelectWidget::OnBackClicked()
{
	UAudioSubsystem::PlayUICancelStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnBackToServerSelect();
	}
	return FReply::Handled();
}

FReply SCharacterSelectWidget::OnConfirmDeleteClicked()
{
	UAudioSubsystem::PlayUIClickStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (!DeletePasswordField.IsValid())
	{
		return FReply::Handled();
	}

	const FString Password = DeletePasswordField->GetText().ToString();

	if (Password.IsEmpty())
	{
		if (DeleteErrorText.IsValid())
		{
			DeleteErrorText->SetText(FText::FromString(TEXT("Please enter your password.")));
			DeleteErrorText->SetVisibility(EVisibility::Visible);
		}
		return FReply::Handled();
	}

	if (SelectedIndex >= 0 && SelectedIndex < CachedCharacters.Num())
	{
		if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
		{
			Subsystem->OnDeleteCharacterConfirmed(
				CachedCharacters[SelectedIndex].CharacterId,
				Password
			);
		}
	}

	return FReply::Handled();
}

FReply SCharacterSelectWidget::OnCancelDeleteClicked()
{
	UAudioSubsystem::PlayUICancelStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	HideDeleteConfirmation();
	return FReply::Handled();
}

// ============================================================
// Helpers
// ============================================================
TSharedRef<SWidget> SCharacterSelectWidget::BuildStatRow(const FString& Label, const FString& Value)
{
	return SNew(SHorizontalBox)

		// Stat label (fixed width for alignment)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.WidthOverride(32.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CharSelectColors::TextDim))
			]
		]

		// Stat value
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor(CharSelectColors::TextPrimary))
		];
}

FString SCharacterSelectWidget::FormatJobClass(const FString& JobClass)
{
	if (JobClass.IsEmpty())
	{
		return JobClass;
	}

	FString Result = JobClass;
	Result[0] = FChar::ToUpper(Result[0]);
	return Result;
}

// ============================================================
// Keyboard Input
// ============================================================
FReply SCharacterSelectWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// If delete overlay is visible, handle Escape to close it
	if (DeleteOverlay.IsValid() && DeleteOverlay->GetVisibility() == EVisibility::Visible)
	{
		if (Key == EKeys::Escape)
		{
			return OnCancelDeleteClicked();
		}
		if (Key == EKeys::Enter)
		{
			return OnConfirmDeleteClicked();
		}
		// Consume all other keys while the overlay is open
		return FReply::Handled();
	}

	// Enter -> Play
	if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
	{
		return OnPlayClicked();
	}

	// Delete key -> initiate deletion
	if (Key == EKeys::Delete)
	{
		return OnDeleteClicked();
	}

	// Escape -> back to server select
	if (Key == EKeys::Escape)
	{
		return OnBackClicked();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
