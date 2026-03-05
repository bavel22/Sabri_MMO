// SKafraWidget.cpp — Kafra NPC service dialog.
// Main menu: Save / Teleport Service / Cancel
// Teleport view: destination buttons with zeny costs + Back button
// RO Classic brown/gold theme, draggable via title bar.

#include "SKafraWidget.h"
#include "KafraSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Colors (same palette as other widgets)
// ============================================================
namespace KafraColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
	static const FLinearColor ButtonBg      (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor ButtonHover   (0.40f, 0.28f, 0.16f, 1.f);
	static const FLinearColor StatusGreen   (0.30f, 0.85f, 0.30f, 1.f);
	static const FLinearColor ErrorRed      (0.90f, 0.25f, 0.25f, 1.f);
}

// ============================================================
// Construct
// ============================================================

void SKafraWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SNew(SBox)
		.WidthOverride(280.f)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(KafraColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(KafraColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(KafraColors::PanelBrown)
					.Padding(FMargin(6.f))
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
								.BorderBackgroundColor(KafraColors::GoldDivider)
							]
						]

						// Main menu content
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(MainMenuPanel, SBox)
							.Visibility_Lambda([this]()
							{
								return CurrentView == EKafraView::MainMenu
									? EVisibility::Visible : EVisibility::Collapsed;
							})
							[ BuildMainMenuContent() ]
						]

						// Teleport list content
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(TeleportPanel, SBox)
							.Visibility_Lambda([this]()
							{
								return CurrentView == EKafraView::TeleportList
									? EVisibility::Visible : EVisibility::Collapsed;
							})
							[ BuildTeleportContent() ]
						]

						// Gold divider
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
						[
							SNew(SBox).HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(KafraColors::GoldDivider)
							]
						]

						// Bottom row (zeny + status)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildBottomRow() ]
					]
				]
			]
		]
	];

	// Build teleport destinations from subsystem data
	RebuildDestinations();

	ApplyLayout();
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SKafraWidget::BuildTitleBar()
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Kafra Service")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(KafraColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(KafraColors::TextShadow)
		]

		// Close button
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.ContentPadding(FMargin(2.f))
			.OnClicked_Lambda([this]() -> FReply
			{
				auto* Sub = OwningSubsystem.Get();
				if (Sub) Sub->CloseKafra();
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("X")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(KafraColors::GoldHighlight))
			]
		];
}

// ============================================================
// Main menu content
// ============================================================

TSharedRef<SWidget> SKafraWidget::BuildMainMenuContent()
{
	UKafraSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SVerticalBox)

		// Greeting
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 8)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Welcome to the Kafra Service!\nHow may I help you?")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			.ColorAndOpacity(FSlateColor(KafraColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(KafraColors::TextShadow)
			.AutoWrapText(true)
		]

		// Save button
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			BuildKafraButton(
				FText::FromString(TEXT("Save Point")),
				FOnClicked::CreateLambda([this]() -> FReply
				{
					auto* Sub = OwningSubsystem.Get();
					if (Sub) Sub->RequestSave();
					return FReply::Handled();
				})
			)
		]

		// Teleport Service button
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			BuildKafraButton(
				FText::FromString(TEXT("Teleport Service")),
				FOnClicked::CreateLambda([this]() -> FReply
				{
					CurrentView = EKafraView::TeleportList;
					return FReply::Handled();
				})
			)
		]

		// Cancel button
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
		[
			BuildKafraButton(
				FText::FromString(TEXT("Cancel")),
				FOnClicked::CreateLambda([this]() -> FReply
				{
					auto* Sub = OwningSubsystem.Get();
					if (Sub) Sub->CloseKafra();
					return FReply::Handled();
				})
			)
		];
}

// ============================================================
// Teleport content
// ============================================================

TSharedRef<SWidget> SKafraWidget::BuildTeleportContent()
{
	return SNew(SVerticalBox)

		// Header
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 6)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Select Destination:")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(KafraColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(KafraColors::TextShadow)
		]

		// Destination list (populated dynamically)
		+ SVerticalBox::Slot().AutoHeight()
		[ SAssignNew(DestContainer, SVerticalBox) ]

		// Back button
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 2)
		[
			BuildKafraButton(
				FText::FromString(TEXT("Back")),
				FOnClicked::CreateLambda([this]() -> FReply
				{
					CurrentView = EKafraView::MainMenu;
					return FReply::Handled();
				})
			)
		];
}

// ============================================================
// Rebuild destination buttons
// ============================================================

void SKafraWidget::RebuildDestinations()
{
	if (!DestContainer.IsValid()) return;

	DestContainer->ClearChildren();

	UKafraSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	for (const FKafraDestination& Dest : Sub->Destinations)
	{
		FString ZoneName = Dest.ZoneName;
		FString ButtonLabel = FString::Printf(TEXT("%s - %dz"), *Dest.DisplayName, Dest.Cost);

		DestContainer->AddSlot()
		.AutoHeight()
		.Padding(0, 2)
		[
			BuildKafraButton(
				FText::FromString(ButtonLabel),
				FOnClicked::CreateLambda([this, ZoneName]() -> FReply
				{
					auto* Sub = OwningSubsystem.Get();
					if (Sub) Sub->RequestTeleport(ZoneName);
					return FReply::Handled();
				})
			)
		];
	}

	if (Sub->Destinations.Num() == 0)
	{
		DestContainer->AddSlot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No destinations available.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
			.ColorAndOpacity(FSlateColor(KafraColors::TextPrimary))
		];
	}
}

// ============================================================
// Bottom row (zeny + status message)
// ============================================================

TSharedRef<SWidget> SKafraWidget::BuildBottomRow()
{
	UKafraSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SVerticalBox)

		// Status message
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text_Lambda([Sub]() -> FText
			{
				if (!Sub) return FText::GetEmpty();
				if (Sub->StatusMessage.IsEmpty()) return FText::GetEmpty();
				if (FPlatformTime::Seconds() > Sub->StatusExpireTime)
				{
					return FText::GetEmpty();
				}
				return FText::FromString(Sub->StatusMessage);
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity_Lambda([Sub]() -> FSlateColor
			{
				if (!Sub || Sub->StatusMessage.IsEmpty())
					return FSlateColor(KafraColors::StatusGreen);
				// Error messages start with uppercase, save confirmations too
				if (Sub->StatusMessage.Contains(TEXT("!")))
					return FSlateColor(KafraColors::StatusGreen);
				return FSlateColor(KafraColors::ErrorRed);
			})
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(KafraColors::TextShadow)
		]

		// Zeny display
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Zeny: ")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(KafraColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(KafraColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText
				{
					if (!Sub) return FText::AsNumber(0);
					return FText::AsNumber(Sub->PlayerZuzucoin);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(KafraColors::ZuzucoinGold))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(KafraColors::TextShadow)
			]
		];
}

// ============================================================
// Reusable button builder
// ============================================================

TSharedRef<SWidget> SKafraWidget::BuildKafraButton(const FText& Label, FOnClicked OnClicked)
{
	return SNew(SButton)
		.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
		.OnClicked(OnClicked)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(KafraColors::ButtonBg)
			.Padding(FMargin(8.f, 4.f))
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(KafraColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(KafraColors::TextShadow)
			]
		];
}

// ============================================================
// Drag support (title bar only)
// ============================================================

FReply SKafraWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

	// Title bar is top ~24px
	if (LocalPos.Y < 24.f)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SKafraWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SKafraWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

void SKafraWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}
