// SLoadingOverlayWidget.cpp — Full-screen "please wait" overlay (RO Classic brown/gold theme)

#include "SLoadingOverlayWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette — Loading Overlay
// NOTE: Use FLinearColor() directly — FColor() applies sRGB->linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace LoadingColors
{
	static const FLinearColor DimBackground(0.0f, 0.0f, 0.0f, 1.f);
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor TextPrimary  (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor BarBg        (0.10f, 0.07f, 0.04f, 1.f);
	static const FLinearColor BarFill      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor TextShadow   (0.0f, 0.0f, 0.0f, 0.85f);
}

// ============================================================
// Construction
// ============================================================
void SLoadingOverlayWidget::Construct(const FArguments& InArgs)
{
	// Start hidden
	SetVisibility(EVisibility::Collapsed);
	bIsVisible = false;

	ChildSlot
	[
		// Full-screen overlay
		SNew(SOverlay)

		// Slot 1: Semi-transparent dark background (click-blocking)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(LoadingColors::DimBackground)
		]

		// Slot 2: Centered dialog box
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(350.f)
			[
				// Layer 1: Gold trim border (outermost)
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(LoadingColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Layer 2: Dark inset
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(LoadingColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Layer 3: Brown content panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(LoadingColors::PanelBrown)
						.Padding(FMargin(12.f, 10.f))
						[
							SNew(SVerticalBox)

							// --- Title: "Please Wait" ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0, 0, 0, 4)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Please Wait")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
								.ColorAndOpacity(FSlateColor(LoadingColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(LoadingColors::TextShadow)
							]

							// --- Gold divider line ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(1.f)
								.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(LoadingColors::GoldTrim)
								]
							]

							// --- Status text ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 8)
							.HAlign(HAlign_Center)
							[
								SAssignNew(StatusTextBlock, STextBlock)
								.Text(FText::FromString(TEXT("Connecting to server...")))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
								.ColorAndOpacity(FSlateColor(LoadingColors::TextPrimary))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(LoadingColors::TextShadow)
								.Justification(ETextJustify::Center)
							]

							// --- Indeterminate progress bar ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(8, 4)
							[
								SNew(SBox)
								.HeightOverride(14.f)
								[
									// Gold border around bar
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(LoadingColors::GoldTrim)
									.Padding(FMargin(1.f))
									[
										SNew(SOverlay)

										// Dark bar background
										+ SOverlay::Slot()
										[
											SNew(SBorder)
											.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
											.BorderBackgroundColor(LoadingColors::BarBg)
										]

										// Indeterminate progress fill (no Percent = marquee/full)
										+ SOverlay::Slot()
										[
											SNew(SProgressBar)
											.FillColorAndOpacity(FSlateColor(LoadingColors::BarFill))
										]
									]
								]
							]

							// --- Bottom padding ---
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
	];
}

// ============================================================
// Public API
// ============================================================
void SLoadingOverlayWidget::SetStatusText(const FString& Text)
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(Text));
	}
}

void SLoadingOverlayWidget::Show()
{
	SetVisibility(EVisibility::Visible);
	bIsVisible = true;
}

void SLoadingOverlayWidget::Hide()
{
	SetVisibility(EVisibility::Collapsed);
	bIsVisible = false;
}

bool SLoadingOverlayWidget::IsShowing() const
{
	return bIsVisible;
}
