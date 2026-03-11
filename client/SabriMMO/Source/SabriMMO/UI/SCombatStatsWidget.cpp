// SCombatStatsWidget.cpp — RO Classic stats window (Slate widget composition)
// Draggable & resizable. Shows: ATK, MATK, HIT, FLEE, CRI, ASPD, DEF, MDEF, PD

#include "SCombatStatsWidget.h"
#include "CombatStatsSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette for Combat Stats
// ============================================================
namespace CombatColors
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
	static const FLinearColor ButtonNormal (0.25f, 0.20f, 0.12f, 1.0f);
	static const FLinearColor ButtonHover  (0.35f, 0.28f, 0.15f, 1.0f);
	static const FLinearColor CostText     (0.60f, 0.55f, 0.40f, 1.0f);
	static const FLinearColor DisabledText (0.40f, 0.35f, 0.28f, 1.0f);
	static const FLinearColor StatPtsGold  (1.00f, 0.85f, 0.30f, 1.0f);
}

// ============================================================
// Construction
// ============================================================
void SCombatStatsWidget::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(CurrentSize.X)
		[
			// Outer gold trim border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CombatColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				// Inner dark inset
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CombatColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					// Main dark panel
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CombatColors::PanelBg)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)

						// --- Title Bar (draggable) ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// --- Offense Section ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Offense")) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("ATK")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("%d + %d"), Subsystem->StatusATK, Subsystem->WeaponATK + Subsystem->PassiveATK));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("MATK")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->StatusMATK));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("HIT")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->HIT));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("Critical")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->Critical));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("ASPD")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->ASPD));
								})
							)
						]

						// --- Divider ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildDivider() ]

						// --- Defense Section ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Defense")) ]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("DEF")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("%d + %d"), Subsystem->HardDEF, Subsystem->SoftDEF));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("MDEF")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("0 + %d"), Subsystem->SoftMDEF));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("FLEE")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->FLEE));
								})
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
						[
							BuildStatRow(
								FText::FromString(TEXT("P.Dodge")),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->PerfectDodge));
								})
							)
						]

						// --- Divider ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildDivider() ]

						// --- Base Stats Section (with [+] buttons) ---
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSectionHeader(TEXT("Base Stats")) ]

						// Stat Points remaining
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildStatPointsBar() ]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("STR"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->STR));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostSTR));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostSTR && Subsystem->BaseSTR < 99;
								}),
								TEXT("str")
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("AGI"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->AGI));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostAGI));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostAGI && Subsystem->BaseAGI < 99;
								}),
								TEXT("agi")
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("VIT"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->VIT));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostVIT));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostVIT && Subsystem->BaseVIT < 99;
								}),
								TEXT("vit")
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("INT"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->INT_Stat));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostINT));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostINT && Subsystem->BaseINT < 99;
								}),
								TEXT("int")
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("DEX"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->DEX));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostDEX));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostDEX && Subsystem->BaseDEX < 99;
								}),
								TEXT("dex")
							)
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(6, 1)
						[
							BuildAllocatableStatRow(
								TEXT("LUK"),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::FromInt(Subsystem->LUK));
								}),
								TAttribute<FText>::CreateLambda([this]() -> FText {
									if (!Subsystem) return FText::GetEmpty();
									return FText::FromString(FString::Printf(TEXT("(%d)"), Subsystem->CostLUK));
								}),
								TAttribute<bool>::CreateLambda([this]() -> bool {
									if (!Subsystem) return false;
									return Subsystem->StatPoints >= Subsystem->CostLUK && Subsystem->BaseLUK < 99;
								}),
								TEXT("luk")
							)
						]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Title bar — "Combat Stats" (draggable handle)
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildTitleBar()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(CombatColors::PanelDark)
		.Padding(FMargin(6.f, 3.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Combat Stats")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(CombatColors::HeaderText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(CombatColors::TextShadow)
		];
}

// ============================================================
// Section header (e.g. "--- Offense ---")
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildSectionHeader(const FString& Title)
{
	return SNew(SBox)
		.Padding(FMargin(6.f, 3.f, 6.f, 1.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("--- %s ---"), *Title)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(CombatColors::SectionText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(CombatColors::TextShadow)
		];
}

// ============================================================
// Single stat row: [Label]  [Value]  (kept close together)
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildStatRow(TAttribute<FText> Label, TAttribute<FText> Value)
{
	return SNew(SHorizontalBox)

		// Label (left, fixed width to align values)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(62.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CombatColors::LabelText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]
		]

		// Value (immediately after label)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(Value)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(CombatColors::ValueText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(CombatColors::TextShadow)
		];
}

// ============================================================
// Stat points remaining display
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildStatPointsBar()
{
	return SNew(SBox)
		.Padding(FMargin(6.f, 2.f, 6.f, 2.f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Points:")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CombatColors::LabelText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([this]() -> FText {
					if (!Subsystem) return FText::FromString(TEXT("0"));
					return FText::FromString(FString::FromInt(Subsystem->StatPoints));
				}))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CombatColors::StatPtsGold))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]
		];
}

// ============================================================
// Allocatable stat row: [Label] [Value] [Cost] [+]
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildAllocatableStatRow(
	const FString& StatLabel,
	TAttribute<FText> Value,
	TAttribute<FText> CostText,
	TAttribute<bool> ButtonEnabled,
	const FString& StatName)
{
	return SNew(SHorizontalBox)

		// Label (fixed width to align values)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(32.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(StatLabel))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CombatColors::LabelText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]
		]

		// Value (e.g. "45")
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(30.f)
			[
				SNew(STextBlock)
				.Text(Value)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CombatColors::ValueText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]
		]

		// Cost display (e.g. "(5)") — hidden when button is hidden
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2, 0, 0, 0)
		[
			SNew(SBox)
			.WidthOverride(26.f)
			.Visibility_Lambda([ButtonEnabled]() -> EVisibility {
				return ButtonEnabled.Get() ? EVisibility::Visible : EVisibility::Hidden;
			})
			[
				SNew(STextBlock)
				.Text(CostText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(CombatColors::CostText))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CombatColors::TextShadow)
			]
		]

		// [+] button (hidden when not enough points)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2, 0, 0, 0)
		[
			SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(2.f))
			.Visibility_Lambda([ButtonEnabled]() -> EVisibility {
				return ButtonEnabled.Get() ? EVisibility::Visible : EVisibility::Hidden;
			})
			.OnClicked(FOnClicked::CreateSP(this, &SCombatStatsWidget::OnAllocateStatClicked, StatName))
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("+")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(CombatColors::StatPtsGold))
			]
		];
}

// ============================================================
// Stat allocation button callback
// ============================================================
FReply SCombatStatsWidget::OnAllocateStatClicked(FString StatName)
{
	if (Subsystem)
	{
		Subsystem->AllocateStat(StatName);
	}
	return FReply::Handled();
}

// ============================================================
// Gold divider line
// ============================================================
TSharedRef<SWidget> SCombatStatsWidget::BuildDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(4.f, 3.f, 4.f, 3.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CombatColors::GoldDivider)
		];
}

// ============================================================
// Apply position + size (render transform for position, SBox for size)
// ============================================================
void SCombatStatsWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));

	if (RootSizeBox.IsValid())
	{
		RootSizeBox->SetWidthOverride(CurrentSize.X);
	}
}

// ============================================================
// Content bounds — actual visible size (not viewport size)
// ============================================================
FVector2D SCombatStatsWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
	{
		return RootSizeBox->GetDesiredSize();
	}
	return CurrentSize;
}

// ============================================================
// Hit-test which edge(s) the mouse is near (uses content bounds)
// ============================================================
EResizeEdge SCombatStatsWidget::HitTestEdges(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D Size = GetContentSize();

	// Outside content bounds entirely — no edge hit
	if (LocalPos.X < -ResizeGrabZone || LocalPos.X > Size.X + ResizeGrabZone ||
		LocalPos.Y < -ResizeGrabZone || LocalPos.Y > Size.Y + ResizeGrabZone)
		return EResizeEdge::None;

	EResizeEdge Edge = EResizeEdge::None;

	if (LocalPos.X < ResizeGrabZone)                Edge |= EResizeEdge::Left;
	if (LocalPos.X > Size.X - ResizeGrabZone)       Edge |= EResizeEdge::Right;
	if (LocalPos.Y < ResizeGrabZone)                Edge |= EResizeEdge::Top;
	if (LocalPos.Y > Size.Y - ResizeGrabZone)       Edge |= EResizeEdge::Bottom;

	return Edge;
}

// ============================================================
// Mouse interaction — drag (title bar) + resize (edges/corners)
// All handlers check content bounds so clicks outside pass through.
// ============================================================
FReply SCombatStatsWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Outside content area — let click pass through to the game
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

	// Check for resize first (edges/corners)
	EResizeEdge Edge = HitTestEdges(MyGeometry, ScreenPos);
	if (Edge != EResizeEdge::None)
	{
		bIsResizing = true;
		ActiveResizeEdge = Edge;
		ResizeStartMouse = ScreenPos;
		ResizeStartSize  = GetContentSize();
		ResizeStartPos   = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	// Check for drag (title bar — top 22px)
	if (LocalPos.Y < 22.f)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SCombatStatsWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	if (bIsDragging || bIsResizing)
	{
		bIsDragging = false;
		bIsResizing = false;
		ActiveResizeEdge = EResizeEdge::None;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SCombatStatsWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}

	if (bIsResizing)
	{
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		const FVector2D Delta = (MouseEvent.GetScreenSpacePosition() - ResizeStartMouse) / DPIScale;
		FVector2D NewSize = ResizeStartSize;
		FVector2D NewPos  = ResizeStartPos;

		if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Right))
			NewSize.X = ResizeStartSize.X + Delta.X;

		if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Bottom))
			NewSize.Y = ResizeStartSize.Y + Delta.Y;

		if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Left))
		{
			NewSize.X = ResizeStartSize.X - Delta.X;
			NewPos.X  = ResizeStartPos.X  + Delta.X;
		}

		if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Top))
		{
			NewSize.Y = ResizeStartSize.Y - Delta.Y;
			NewPos.Y  = ResizeStartPos.Y  + Delta.Y;
		}

		// Clamp to minimum
		if (NewSize.X < MinWidth)
		{
			if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Left))
				NewPos.X = ResizeStartPos.X + (ResizeStartSize.X - MinWidth);
			NewSize.X = MinWidth;
		}
		if (NewSize.Y < MinHeight)
		{
			if (EnumHasAnyFlags(ActiveResizeEdge, EResizeEdge::Top))
				NewPos.Y = ResizeStartPos.Y + (ResizeStartSize.Y - MinHeight);
			NewSize.Y = MinHeight;
		}

		CurrentSize    = NewSize;
		WidgetPosition = NewPos;
		ApplyLayout();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ============================================================
// Cursor — show resize arrows only near content edges
// ============================================================
FCursorReply SCombatStatsWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	if (bIsResizing || bIsDragging)
		return FCursorReply::Unhandled();

	// Outside content area — default cursor
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	const FVector2D ContentBounds = GetContentSize();
	if (LocalPos.X < -ResizeGrabZone || LocalPos.X > ContentBounds.X + ResizeGrabZone ||
		LocalPos.Y < -ResizeGrabZone || LocalPos.Y > ContentBounds.Y + ResizeGrabZone)
		return FCursorReply::Unhandled();

	EResizeEdge Edge = HitTestEdges(MyGeometry, CursorEvent.GetScreenSpacePosition());

	if (Edge == (EResizeEdge::Left | EResizeEdge::Top) || Edge == (EResizeEdge::Right | EResizeEdge::Bottom))
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);

	if (Edge == (EResizeEdge::Right | EResizeEdge::Top) || Edge == (EResizeEdge::Left | EResizeEdge::Bottom))
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthWest);

	if (EnumHasAnyFlags(Edge, EResizeEdge::Left | EResizeEdge::Right))
		return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);

	if (EnumHasAnyFlags(Edge, EResizeEdge::Top | EResizeEdge::Bottom))
		return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);

	return FCursorReply::Unhandled();
}
