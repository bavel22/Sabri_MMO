// SSenseResultPopup.cpp — Draggable monster info panel for Wizard Sense skill (ID 812).
// Non-blocking: no backdrop, no click-outside-dismiss. Drag via title bar. Close via X only.

#include "SSenseResultPopup.h"
#include "EnemySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

// ============================================================
// Colors — RO Classic palette
// ============================================================

namespace SenseColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextDim       (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor HPRed         (0.85f, 0.15f, 0.15f, 1.f);
	static const FLinearColor BarBg         (0.10f, 0.07f, 0.04f, 1.f);
	static const FLinearColor SectionBg     (0.18f, 0.12f, 0.07f, 1.f);
	// Element colors
	static const FLinearColor ElemFire      (0.95f, 0.40f, 0.20f, 1.f);
	static const FLinearColor ElemWater     (0.30f, 0.55f, 0.95f, 1.f);
	static const FLinearColor ElemWind      (0.40f, 0.85f, 0.45f, 1.f);
	static const FLinearColor ElemEarth     (0.75f, 0.55f, 0.25f, 1.f);
	static const FLinearColor ElemHoly      (0.95f, 0.90f, 0.50f, 1.f);
	static const FLinearColor ElemDark      (0.55f, 0.30f, 0.70f, 1.f);
	static const FLinearColor ElemPoison    (0.60f, 0.75f, 0.20f, 1.f);
	static const FLinearColor ElemUndead    (0.50f, 0.50f, 0.55f, 1.f);
	static const FLinearColor ElemGhost     (0.70f, 0.70f, 0.85f, 1.f);
	static const FLinearColor ElemNeutral   (0.80f, 0.80f, 0.80f, 1.f);
}

static constexpr float SensePopupWidth = 260.f;
static constexpr float SenseLabelWidth = 72.f;

// ============================================================
// Helpers
// ============================================================

static FLinearColor GetElementColor(const FString& Element)
{
	if (Element == TEXT("fire"))    return SenseColors::ElemFire;
	if (Element == TEXT("water"))   return SenseColors::ElemWater;
	if (Element == TEXT("wind"))    return SenseColors::ElemWind;
	if (Element == TEXT("earth"))   return SenseColors::ElemEarth;
	if (Element == TEXT("holy"))    return SenseColors::ElemHoly;
	if (Element == TEXT("dark") || Element == TEXT("shadow")) return SenseColors::ElemDark;
	if (Element == TEXT("poison"))  return SenseColors::ElemPoison;
	if (Element == TEXT("undead"))  return SenseColors::ElemUndead;
	if (Element == TEXT("ghost"))   return SenseColors::ElemGhost;
	return SenseColors::ElemNeutral;
}

static FString CapitalizeFirst(const FString& Input)
{
	if (Input.IsEmpty()) return Input;
	FString Result = Input;
	Result[0] = FChar::ToUpper(Result[0]);
	return Result;
}

// ============================================================
// Construction — Strategy B (auto-height, WidthOverride only)
// ============================================================

void SSenseResultPopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	Data = InArgs._SenseData;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(SensePopupWidth)
		[
			// 3-layer frame: Gold -> Dark -> Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(SenseColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(SenseColors::PanelBrown)
					.Padding(FMargin(6.f))
					[
						SNew(SVerticalBox)

						// Title bar with close button
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// Gold divider
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
						[ BuildGoldDivider() ]

						// Content
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildContent() ]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SSenseResultPopup::BuildTitleBar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Monster Information")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(SenseColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(SenseColors::TextShadow)
		]
		// X close button
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
			.Cursor(EMouseCursor::Hand)
			.Padding(FMargin(4.f, 0.f))
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					DismissPopup();
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("X")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(SenseColors::TextDim))
			]
		];
}

// ============================================================
// Content
// ============================================================

TSharedRef<SWidget> SSenseResultPopup::BuildContent()
{
	FString ElementStr = FString::Printf(TEXT("%s Lv%d"), *CapitalizeFirst(Data.Element), Data.ElementLevel);

	return SNew(SVerticalBox)

		// Monster name (large, centered)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Data.TargetName))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(SenseColors::TextBright))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(SenseColors::TextShadow)
		]

		// Level (centered)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Level %d"), Data.Level)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(SenseColors::TextDim))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(SenseColors::TextShadow)
		]

		// HP bar
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[ BuildHPBar() ]

		// Divider
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 2)
		[ BuildGoldDivider() ]

		// Properties section (element, race, size)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::SectionBg)
			.Padding(FMargin(4.f, 3.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[
					// Element row with colored text
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(SenseLabelWidth)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Element")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FSlateColor(SenseColors::TextDim))
							.ShadowOffset(FVector2D(1, 1))
							.ShadowColorAndOpacity(SenseColors::TextShadow)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(ElementStr))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(GetElementColor(Data.Element)))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(SenseColors::TextShadow)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("Race"), CapitalizeFirst(Data.Race)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("Size"), CapitalizeFirst(Data.Size)) ]
			]
		]

		// Divider
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 3)
		[ BuildGoldDivider() ]

		// DEF / MDEF
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::SectionBg)
			.Padding(FMargin(4.f, 3.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("DEF"), Data.HardDef) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("MDEF"), Data.HardMdef) ]
			]
		]

		// Divider
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 3)
		[ BuildGoldDivider() ]

		// Base stats (2-column)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::SectionBg)
			.Padding(FMargin(4.f, 3.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("STR"), Data.STR) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("AGI"), Data.AGI) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("VIT"), Data.VIT) ]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("INT"), Data.INT) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("DEX"), Data.DEX) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
					[ BuildStatRow(TEXT("LUK"), Data.LUK) ]
				]
			]
		]

		// Divider
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 3)
		[ BuildGoldDivider() ]

		// EXP
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::SectionBg)
			.Padding(FMargin(4.f, 3.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("Base EXP"), FString::FormatAsNumber(Data.BaseExp)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
				[ BuildStatRow(TEXT("Job EXP"), FString::FormatAsNumber(Data.JobExp)) ]
			]
		];
}

// ============================================================
// HP Bar
// ============================================================

TSharedRef<SWidget> SSenseResultPopup::BuildHPBar()
{
	float HPPercent = (Data.MaxHealth > 0) ? FMath::Clamp((float)(Data.Health / Data.MaxHealth), 0.f, 1.f) : 0.f;
	FString HPText = FString::Printf(TEXT("%.0f / %.0f"), Data.Health, Data.MaxHealth);

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(SBox).WidthOverride(28.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("HP")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(SenseColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(SenseColors::TextShadow)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(14.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(SenseColors::GoldDark)
				.Padding(FMargin(1.f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(SenseColors::BarBg)
					]
					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(HPPercent)
						.FillColorAndOpacity(SenseColors::HPRed)
					]
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(HPText))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(SenseColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(SenseColors::TextShadow)
					]
				]
			]
		];
}

// ============================================================
// Stat row helpers
// ============================================================

TSharedRef<SWidget> SSenseResultPopup::BuildStatRow(const FString& Label, const FString& Value)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(SenseLabelWidth)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(SenseColors::TextDim))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(SenseColors::TextShadow)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(SenseColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(SenseColors::TextShadow)
		];
}

TSharedRef<SWidget> SSenseResultPopup::BuildStatRow(const FString& Label, int32 Value)
{
	return BuildStatRow(Label, FString::FromInt(Value));
}

// ============================================================
// Gold divider
// ============================================================

TSharedRef<SWidget> SSenseResultPopup::BuildGoldDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SenseColors::GoldDivider)
		];
}

// ============================================================
// Drag + layout (DPI-correct, title bar only)
// ============================================================

FVector2D SSenseResultPopup::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return FVector2D(SensePopupWidth, 300.f);
}

void SSenseResultPopup::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}

FReply SSenseResultPopup::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Outside content bounds — let click pass through to the game
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

	// Title bar drag
	if (LocalPos.Y < TitleBarHeight)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Handled(); // Consume click on panel body (don't pass to game)
}

FReply SSenseResultPopup::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SSenseResultPopup::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

// ============================================================
// Actions
// ============================================================

void SSenseResultPopup::DismissPopup()
{
	UEnemySubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		Sub->HideSensePopup();
	}
}
