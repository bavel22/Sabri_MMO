// SCharacterCreateWidget.cpp -- Character creation screen (RO Classic brown/gold theme)

#include "SCharacterCreateWidget.h"
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
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette -- Character Create
// NOTE: Use FLinearColor() directly -- FColor() applies sRGB->linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace CreateColors
{
	// Panel backgrounds
	static const FLinearColor PanelBrown      (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark       (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium     (0.33f, 0.22f, 0.13f, 1.f);
	// Gold trim + highlights
	static const FLinearColor GoldTrim        (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark        (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight   (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider     (0.60f, 0.48f, 0.22f, 1.f);
	// Text
	static const FLinearColor TextPrimary     (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright      (1.0f, 1.0f, 1.0f, 1.f);
	static const FLinearColor TextDim         (0.60f, 0.55f, 0.45f, 1.f);
	static const FLinearColor TextShadow      (0.0f, 0.0f, 0.0f, 0.85f);
	// Error
	static const FLinearColor ErrorRed        (0.90f, 0.20f, 0.20f, 1.f);
	// Buttons
	static const FLinearColor ButtonGreen     (0.20f, 0.55f, 0.20f, 1.f);
	static const FLinearColor ButtonDark      (0.30f, 0.20f, 0.10f, 1.f);
	// Input fields
	static const FLinearColor FieldBg         (0.15f, 0.10f, 0.06f, 1.f);
	// Toggles
	static const FLinearColor SelectedToggle  (0.50f, 0.38f, 0.15f, 0.8f);
	static const FLinearColor UnselectedToggle(0.22f, 0.14f, 0.08f, 1.f);
	// Arrow buttons
	static const FLinearColor ArrowButton     (0.40f, 0.30f, 0.15f, 1.f);
}

// ============================================================
// Construction
// ============================================================
void SCharacterCreateWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		// Full viewport overlay -- center the panel
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// Fixed-width character create panel
			SNew(SBox)
			.WidthOverride(450.f)
			[
				// Layer 1: Gold trim border (outermost, 2px)
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CreateColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Layer 2: Dark inset (1px)
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CreateColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Layer 3: Brown content panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CreateColors::PanelBrown)
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)

							// --- Title: "Create Character" ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(CreateColors::PanelDark)
								.Padding(FMargin(0.f, 6.f))
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Create Character")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FSlateColor(CreateColors::GoldHighlight))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CreateColors::TextShadow)
								]
							]

							// --- Gold divider after title ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(CreateColors::GoldDivider)
								.Padding(FMargin(0.f, 0.5f))
							]

							// --- Main content area: preview + form side by side ---
							+ SVerticalBox::Slot()
							.FillHeight(1.f)
							.Padding(FMargin(8.f, 6.f))
							[
								SNew(SHorizontalBox)

								// --- Left side: Preview area (45%) ---
								+ SHorizontalBox::Slot()
								.FillWidth(0.45f)
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(CreateColors::PanelDark)
									.Padding(FMargin(8.f))
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(SVerticalBox)

										// Character summary text
										+ SVerticalBox::Slot()
										.AutoHeight()
										.VAlign(VAlign_Center)
										.HAlign(HAlign_Center)
										[
											SAssignNew(PreviewText, STextBlock)
											.Text(FText::FromString(TEXT("Male Novice\nHair Style: 1\nHair Color: Default")))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
											.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
											.Justification(ETextJustify::Center)
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CreateColors::TextShadow)
										]

										// 8px spacer
										+ SVerticalBox::Slot()
										.AutoHeight()
										.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("[Character Preview]")))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
											.ColorAndOpacity(FSlateColor(CreateColors::TextDim))
											.Justification(ETextJustify::Center)
										]
									]
								]

								// 8px spacer between preview and form
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(8.f)
								]

								// --- Right side: Form area (55%) ---
								+ SHorizontalBox::Slot()
								.FillWidth(0.55f)
								[
									SNew(SVerticalBox)

									// Name label
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Name")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
									]

									// Name input field
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SAssignNew(NameField, SEditableTextBox)
										.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
										.BackgroundColor(CreateColors::FieldBg)
										.ForegroundColor(FSlateColor(CreateColors::TextBright))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									]

									// 6px spacing
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Class")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
									]

									// Class value (locked)
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Novice")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextDim))
									]

									// 6px spacing
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Gender")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
									]

									// Gender toggle buttons
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										// Male toggle
										+ SHorizontalBox::Slot()
										.FillWidth(1.f)
										[
											SNew(SButton)
											.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor
											{
												return bIsMale ? CreateColors::SelectedToggle : CreateColors::UnselectedToggle;
											})
											.OnClicked(this, &SCharacterCreateWidget::OnMaleClicked)
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Center)
											[
												SNew(SBox)
												.Padding(FMargin(4.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("Male")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]

										// Female toggle
										+ SHorizontalBox::Slot()
										.FillWidth(1.f)
										[
											SNew(SButton)
											.ButtonColorAndOpacity_Lambda([this]() -> FLinearColor
											{
												return !bIsMale ? CreateColors::SelectedToggle : CreateColors::UnselectedToggle;
											})
											.OnClicked(this, &SCharacterCreateWidget::OnFemaleClicked)
											.HAlign(HAlign_Center)
											.VAlign(VAlign_Center)
											[
												SNew(SBox)
												.Padding(FMargin(4.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("Female")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]
									]

									// 6px spacing
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Hair Style")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
									]

									// Hair style picker: [<] Style N [>]
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										// Left arrow
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(CreateColors::ArrowButton)
											.OnClicked(this, &SCharacterCreateWidget::OnHairStyleLeft)
											[
												SNew(SBox)
												.Padding(FMargin(6.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("\x25C4")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]

										// Current style value
										+ SHorizontalBox::Slot()
										.FillWidth(1.f)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										[
											SAssignNew(HairStyleText, STextBlock)
											.Text(FText::FromString(TEXT("Style 1")))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
											.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
										]

										// Right arrow
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(CreateColors::ArrowButton)
											.OnClicked(this, &SCharacterCreateWidget::OnHairStyleRight)
											[
												SNew(SBox)
												.Padding(FMargin(6.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("\x25BA")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]
									]

									// 6px spacing
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Hair Color")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
									]

									// Hair color picker: [<] Color Name [>]
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										// Left arrow
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(CreateColors::ArrowButton)
											.OnClicked(this, &SCharacterCreateWidget::OnHairColorLeft)
											[
												SNew(SBox)
												.Padding(FMargin(6.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("\x25C4")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]

										// Current color value
										+ SHorizontalBox::Slot()
										.FillWidth(1.f)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										[
											SAssignNew(HairColorText, STextBlock)
											.Text(FText::FromString(TEXT("Default")))
											.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
											.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
										]

										// Right arrow
										+ SHorizontalBox::Slot()
										.AutoWidth()
										[
											SNew(SButton)
											.ButtonColorAndOpacity(CreateColors::ArrowButton)
											.OnClicked(this, &SCharacterCreateWidget::OnHairColorRight)
											[
												SNew(SBox)
												.Padding(FMargin(6.f, 2.f))
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(FText::FromString(TEXT("\x25BA")))
													.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
													.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
												]
											]
										]
									]

									// 8px spacing before error
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(FMargin(0.f, 8.f, 0.f, 0.f))
									[
										// Error text (hidden by default)
										SAssignNew(ErrorTextBlock, STextBlock)
										.Text(FText::GetEmpty())
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(CreateColors::ErrorRed))
										.Visibility(EVisibility::Collapsed)
										.AutoWrapText(true)
									]
								]
							]

							// --- Gold divider before buttons ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(CreateColors::GoldDivider)
								.Padding(FMargin(0.f, 0.5f))
							]

							// --- Button row ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.f, 6.f))
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)

								// Create button (green, fills available width)
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CreateColors::ButtonGreen)
									.OnClicked(this, &SCharacterCreateWidget::OnCreateClicked)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(16.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Create")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
											.ColorAndOpacity(FSlateColor(CreateColors::TextBright))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CreateColors::TextShadow)
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

								// Cancel button (dark, auto width)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.ButtonColorAndOpacity(CreateColors::ButtonDark)
									.OnClicked(this, &SCharacterCreateWidget::OnCancelClicked)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(16.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Cancel")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
											.ColorAndOpacity(FSlateColor(CreateColors::TextPrimary))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CreateColors::TextShadow)
										]
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

// ============================================================
// Error Display
// ============================================================
void SCharacterCreateWidget::ShowError(const FString& Message)
{
	if (ErrorTextBlock.IsValid())
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
		ErrorTextBlock->SetVisibility(EVisibility::Visible);
	}
}

void SCharacterCreateWidget::ClearError()
{
	if (ErrorTextBlock.IsValid())
	{
		ErrorTextBlock->SetText(FText::GetEmpty());
		ErrorTextBlock->SetVisibility(EVisibility::Collapsed);
	}
}

// ============================================================
// Preview Update
// ============================================================
void SCharacterCreateWidget::UpdatePreview()
{
	if (PreviewText.IsValid())
	{
		const FString GenderString = bIsMale ? TEXT("Male") : TEXT("Female");
		const FString ColorName = GetHairColorName(CurrentHairColor);
		const FString Preview = FString::Printf(
			TEXT("%s Novice\nHair Style: %d\nHair Color: %s"),
			*GenderString, CurrentHairStyle, *ColorName
		);
		PreviewText->SetText(FText::FromString(Preview));
	}

	if (HairStyleText.IsValid())
	{
		HairStyleText->SetText(FText::FromString(FString::Printf(TEXT("Style %d"), CurrentHairStyle)));
	}

	if (HairColorText.IsValid())
	{
		HairColorText->SetText(FText::FromString(GetHairColorName(CurrentHairColor)));
	}
}

FString SCharacterCreateWidget::GetHairColorName(int32 ColorIndex)
{
	switch (ColorIndex)
	{
	case 0:  return TEXT("Default");
	case 1:  return TEXT("Scarlet");
	case 2:  return TEXT("Lemon");
	case 3:  return TEXT("White");
	case 4:  return TEXT("Orange");
	case 5:  return TEXT("Green");
	case 6:  return TEXT("Cobalt Blue");
	case 7:  return TEXT("Violet");
	case 8:  return TEXT("Black");
	default: return TEXT("Unknown");
	}
}

// ============================================================
// Gender Toggles
// ============================================================
FReply SCharacterCreateWidget::OnMaleClicked()
{
	bIsMale = true;
	UpdatePreview();
	return FReply::Handled();
}

FReply SCharacterCreateWidget::OnFemaleClicked()
{
	bIsMale = false;
	UpdatePreview();
	return FReply::Handled();
}

// ============================================================
// Hair Style Arrows
// ============================================================
FReply SCharacterCreateWidget::OnHairStyleLeft()
{
	CurrentHairStyle--;
	if (CurrentHairStyle < MinHairStyle)
	{
		CurrentHairStyle = MaxHairStyle;
	}
	UpdatePreview();
	return FReply::Handled();
}

FReply SCharacterCreateWidget::OnHairStyleRight()
{
	CurrentHairStyle++;
	if (CurrentHairStyle > MaxHairStyle)
	{
		CurrentHairStyle = MinHairStyle;
	}
	UpdatePreview();
	return FReply::Handled();
}

// ============================================================
// Hair Color Arrows
// ============================================================
FReply SCharacterCreateWidget::OnHairColorLeft()
{
	CurrentHairColor--;
	if (CurrentHairColor < MinHairColor)
	{
		CurrentHairColor = MaxHairColor;
	}
	UpdatePreview();
	return FReply::Handled();
}

FReply SCharacterCreateWidget::OnHairColorRight()
{
	CurrentHairColor++;
	if (CurrentHairColor > MaxHairColor)
	{
		CurrentHairColor = MinHairColor;
	}
	UpdatePreview();
	return FReply::Handled();
}

// ============================================================
// Button Handlers
// ============================================================
FReply SCharacterCreateWidget::OnCreateClicked()
{
	UAudioSubsystem::PlayUIClickStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (!NameField.IsValid())
	{
		return FReply::Handled();
	}

	const FString Name = NameField->GetText().ToString().TrimStartAndEnd();

	// Validate non-empty
	if (Name.IsEmpty())
	{
		ShowError(TEXT("Please enter a character name."));
		return FReply::Handled();
	}

	// Validate length (2-24 characters)
	if (Name.Len() < 2)
	{
		ShowError(TEXT("Name must be at least 2 characters."));
		return FReply::Handled();
	}

	if (Name.Len() > 24)
	{
		ShowError(TEXT("Name must be 24 characters or fewer."));
		return FReply::Handled();
	}

	// Validate alphanumeric + spaces only
	for (const TCHAR Character : Name)
	{
		if (!FChar::IsAlnum(Character) && Character != TEXT(' '))
		{
			ShowError(TEXT("Name may only contain letters, numbers, and spaces."));
			return FReply::Handled();
		}
	}

	ClearError();

	const FString Gender = bIsMale ? TEXT("male") : TEXT("female");

	if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnCreateCharacterSubmitted(Name, TEXT("novice"), CurrentHairStyle, CurrentHairColor, Gender);
	}

	return FReply::Handled();
}

FReply SCharacterCreateWidget::OnCancelClicked()
{
	UAudioSubsystem::PlayUICancelStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnCreateCharacterCancelled();
	}
	return FReply::Handled();
}

// ============================================================
// Keyboard Input
// ============================================================
FReply SCharacterCreateWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter)
	{
		return OnCreateClicked();
	}

	if (Key == EKeys::Escape)
	{
		return OnCancelClicked();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
