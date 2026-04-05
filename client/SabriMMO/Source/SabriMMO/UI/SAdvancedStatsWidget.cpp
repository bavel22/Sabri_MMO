// SAdvancedStatsWidget.cpp — RO Classic advanced stats panel (Slate)
// Shows elemental resistances. Draggable.

#include "SAdvancedStatsWidget.h"
#include "CombatStatsSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette (shared with CombatStatsWidget)
// ============================================================
namespace AdvColors
{
	static const FLinearColor PanelBg      (0.12f, 0.10f, 0.08f, 0.95f);
	static const FLinearColor PanelDark    (0.18f, 0.14f, 0.10f, 1.0f);
	static const FLinearColor GoldTrim     (0.55f, 0.45f, 0.25f, 1.0f);
	static const FLinearColor GoldDivider  (0.45f, 0.35f, 0.18f, 1.0f);
	static const FLinearColor SectionText  (0.85f, 0.70f, 0.30f, 1.0f);
	static const FLinearColor LabelText    (0.75f, 0.70f, 0.60f, 1.0f);
	static const FLinearColor ValueText    (1.00f, 0.95f, 0.80f, 1.0f);
	static const FLinearColor HeaderText   (0.85f, 0.70f, 0.30f, 1.0f);
	static const FLinearColor TextShadow   (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor ZeroValue    (0.50f, 0.45f, 0.35f, 1.0f);
	static const FLinearColor PositiveValue(0.40f, 0.90f, 0.40f, 1.0f);
	static const FLinearColor CloseBtn     (0.70f, 0.30f, 0.25f, 1.0f);

	// Element colors
	static const FLinearColor ElemNeutral  (0.75f, 0.70f, 0.60f, 1.0f);
	static const FLinearColor ElemFire     (1.00f, 0.40f, 0.20f, 1.0f);
	static const FLinearColor ElemWater    (0.30f, 0.60f, 1.00f, 1.0f);
	static const FLinearColor ElemEarth    (0.65f, 0.50f, 0.25f, 1.0f);
	static const FLinearColor ElemWind     (0.40f, 0.90f, 0.50f, 1.0f);
	static const FLinearColor ElemPoison   (0.70f, 0.30f, 0.80f, 1.0f);
	static const FLinearColor ElemHoly     (1.00f, 0.95f, 0.55f, 1.0f);
	static const FLinearColor ElemDark     (0.55f, 0.35f, 0.65f, 1.0f);
	static const FLinearColor ElemGhost    (0.65f, 0.75f, 0.85f, 1.0f);
	static const FLinearColor ElemUndead   (0.50f, 0.55f, 0.45f, 1.0f);

	// Defense colors
	static const FLinearColor BlockSilver  (0.85f, 0.90f, 1.00f, 1.0f);
}

// ============================================================
// Construction
// ============================================================
void SAdvancedStatsWidget::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(185.f)
		[
			// Outer gold trim border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(AdvColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				// Inner dark inset
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(AdvColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					// Main dark panel
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(AdvColors::PanelBg)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)

						// --- Title Bar (draggable) ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// --- Element Resist/ATK (ATK | DEF) ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Element  ATK / DEF")) ]

						#define ELEM_DUAL_ROW(Name, Color, ResistField, EleKey) \
						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1) \
						[ \
							BuildDualRow(TEXT(Name), Color, \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0%")); \
									const int32* V = Subsystem->EleAtk.Find(TEXT(EleKey)); \
									return FText::FromString(FString::Printf(TEXT("%d%%"), V ? *V : 0)); \
								}), \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0%")); \
									return FText::FromString(FString::Printf(TEXT("%d%%"), Subsystem->ResistField)); \
								})) \
						]

						ELEM_DUAL_ROW("Neutral", AdvColors::ElemNeutral, ResistNeutral, "neutral")
						ELEM_DUAL_ROW("Fire",    AdvColors::ElemFire,    ResistFire,    "fire")
						ELEM_DUAL_ROW("Water",   AdvColors::ElemWater,   ResistWater,   "water")
						ELEM_DUAL_ROW("Earth",   AdvColors::ElemEarth,   ResistEarth,   "earth")
						ELEM_DUAL_ROW("Wind",    AdvColors::ElemWind,    ResistWind,    "wind")
						ELEM_DUAL_ROW("Poison",  AdvColors::ElemPoison,  ResistPoison,  "poison")
						ELEM_DUAL_ROW("Holy",    AdvColors::ElemHoly,    ResistHoly,    "holy")
						ELEM_DUAL_ROW("Dark",    AdvColors::ElemDark,    ResistDark,    "dark")
						ELEM_DUAL_ROW("Ghost",   AdvColors::ElemGhost,   ResistGhost,   "ghost")
						ELEM_DUAL_ROW("Undead",  AdvColors::ElemUndead,  ResistUndead,  "undead")

						#undef ELEM_DUAL_ROW

						// --- Divider ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildDivider() ]

						// --- Race ATK/DEF ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Race  ATK / DEF")) ]

						#define RACE_DUAL_ROW(Name, Color, RaceKey) \
						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1) \
						[ \
							BuildDualRow(TEXT(Name), Color, \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0")); \
									const int32* V = Subsystem->RaceAtk.Find(TEXT(RaceKey)); \
									return FText::FromString(FString::Printf(TEXT("%d"), V ? *V : 0)); \
								}), \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0")); \
									const int32* V = Subsystem->RaceDef.Find(TEXT(RaceKey)); \
									return FText::FromString(FString::Printf(TEXT("%d"), V ? *V : 0)); \
								})) \
						]

						RACE_DUAL_ROW("Demon",     AdvColors::ElemDark,    "demon")
						RACE_DUAL_ROW("Undead",    AdvColors::ElemUndead,  "undead")
						RACE_DUAL_ROW("Brute",     AdvColors::ElemEarth,   "brute")
						RACE_DUAL_ROW("Insect",    AdvColors::ElemWind,    "insect")
						RACE_DUAL_ROW("Dragon",    AdvColors::ElemFire,    "dragon")
						RACE_DUAL_ROW("DemiHuman", AdvColors::LabelText,   "demihuman")
						RACE_DUAL_ROW("Angel",     AdvColors::ElemHoly,    "angel")
						RACE_DUAL_ROW("Fish",      AdvColors::ElemWater,   "fish")
						RACE_DUAL_ROW("Plant",     AdvColors::ElemWind,    "plant")
						RACE_DUAL_ROW("Formless",  AdvColors::ElemGhost,   "formless")

						#undef RACE_DUAL_ROW

						// --- Divider ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildDivider() ]

						// --- Size ATK/DEF ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Size  ATK / DEF")) ]

						#define SIZE_DUAL_ROW(Name, SizeKey) \
						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1) \
						[ \
							BuildDualRow(TEXT(Name), AdvColors::LabelText, \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0%")); \
									const int32* V = Subsystem->SizeAtk.Find(TEXT(SizeKey)); \
									return FText::FromString(FString::Printf(TEXT("%d%%"), V ? *V : 0)); \
								}), \
								TAttribute<FText>::CreateLambda([this]() -> FText { \
									if (!Subsystem) return FText::FromString(TEXT("0%")); \
									const int32* V = Subsystem->SizeDef.Find(TEXT(SizeKey)); \
									return FText::FromString(FString::Printf(TEXT("%d%%"), V ? *V : 0)); \
								})) \
						]

						SIZE_DUAL_ROW("Small",  "small")
						SIZE_DUAL_ROW("Medium", "medium")
						SIZE_DUAL_ROW("Large",  "large")

						#undef SIZE_DUAL_ROW

						// --- Divider ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildDivider() ]

						// --- Block ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Defense")) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1, 6, 4)
						[
							BuildResistRow(TEXT("Block"), AdvColors::BlockSilver,
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("%d%%"), Subsystem->BlockChance));
								}))
						]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Title bar — "Advanced Stats" with [X] close button
// ============================================================
TSharedRef<SWidget> SAdvancedStatsWidget::BuildTitleBar()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(AdvColors::PanelDark)
		.Padding(FMargin(6.f, 3.f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Advanced Stats")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(AdvColors::HeaderText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(AdvColors::TextShadow)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FCoreStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(2.f, 0.f))
				.OnClicked(FOnClicked::CreateSP(this, &SAdvancedStatsWidget::OnCloseClicked))
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(AdvColors::CloseBtn))
				]
			]
		];
}

// ============================================================
// Section header
// ============================================================
TSharedRef<SWidget> SAdvancedStatsWidget::BuildSectionHeader(const FString& Title)
{
	return SNew(SBox)
		.Padding(FMargin(6.f, 3.f, 6.f, 1.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("--- %s ---"), *Title)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(AdvColors::SectionText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(AdvColors::TextShadow)
		];
}

// ============================================================
// Resist row: [ColorDot] [ElementName]  [Value%]
// Value turns green when > 0
// ============================================================
TSharedRef<SWidget> SAdvancedStatsWidget::BuildResistRow(
	const FString& ElementName, const FLinearColor& ElementColor, TAttribute<FText> Value)
{
	// Capture element color for the lambda
	FLinearColor ElemColor = ElementColor;

	return SNew(SHorizontalBox)

		// Color indicator dot
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SBox)
			.WidthOverride(8.f)
			.HeightOverride(8.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ElemColor)
			]
		]

		// Element name
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(58.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(ElementName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(AdvColors::LabelText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(AdvColors::TextShadow)
			]
		]

		// Resist value (green if > 0, dim if 0)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Value)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity_Lambda([this, ElementName]() -> FSlateColor {
				if (!Subsystem) return FSlateColor(AdvColors::ZeroValue);
				int32 Val = 0;
				if (ElementName == TEXT("Neutral")) Val = Subsystem->ResistNeutral;
				else if (ElementName == TEXT("Fire")) Val = Subsystem->ResistFire;
				else if (ElementName == TEXT("Water")) Val = Subsystem->ResistWater;
				else if (ElementName == TEXT("Earth")) Val = Subsystem->ResistEarth;
				else if (ElementName == TEXT("Wind")) Val = Subsystem->ResistWind;
				else if (ElementName == TEXT("Poison")) Val = Subsystem->ResistPoison;
				else if (ElementName == TEXT("Holy")) Val = Subsystem->ResistHoly;
				else if (ElementName == TEXT("Dark")) Val = Subsystem->ResistDark;
				else if (ElementName == TEXT("Ghost")) Val = Subsystem->ResistGhost;
				else if (ElementName == TEXT("Undead")) Val = Subsystem->ResistUndead;
				else if (ElementName == TEXT("Block")) Val = Subsystem->BlockChance;
				return FSlateColor(Val > 0 ? AdvColors::PositiveValue : AdvColors::ZeroValue);
			})
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(AdvColors::TextShadow)
		];
}

// ============================================================
// Dual row: [ColorDot] [Label]  [AtkValue] | [DefValue]
// Shows ATK and DEF side by side
// ============================================================
TSharedRef<SWidget> SAdvancedStatsWidget::BuildDualRow(
	const FString& Label, const FLinearColor& LabelColor, TAttribute<FText> AtkValue, TAttribute<FText> DefValue)
{
	return SNew(SHorizontalBox)

		// Color dot
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SBox)
			.WidthOverride(8.f)
			.HeightOverride(8.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(LabelColor)
			]
		]

		// Label
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(58.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(AdvColors::LabelText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(AdvColors::TextShadow)
			]
		]

		// ATK value
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(AtkValue)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(AdvColors::ValueText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(AdvColors::TextShadow)
		]

		// Separator
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(3, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("|")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(AdvColors::GoldDivider))
		]

		// DEF value
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(DefValue)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(AdvColors::PositiveValue))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(AdvColors::TextShadow)
		];
}

// ============================================================
// Divider
// ============================================================
TSharedRef<SWidget> SAdvancedStatsWidget::BuildDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(4.f, 3.f, 4.f, 3.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(AdvColors::GoldDivider)
		];
}

// ============================================================
// Close button callback
// ============================================================
FReply SAdvancedStatsWidget::OnCloseClicked()
{
	if (Subsystem)
	{
		Subsystem->ToggleAdvancedWidget();
	}
	return FReply::Handled();
}

// ============================================================
// Apply position (render transform)
// ============================================================
void SAdvancedStatsWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}

FVector2D SAdvancedStatsWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
	{
		return RootSizeBox->GetDesiredSize();
	}
	return FVector2D(185.0, 300.0);
}

// ============================================================
// Mouse interaction — drag (title bar)
// ============================================================
FReply SAdvancedStatsWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Outside content area
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

	// Title bar drag (top 22px)
	if (LocalPos.Y < 22.f)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Handled(); // Consume click inside panel
}

FReply SAdvancedStatsWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SAdvancedStatsWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
