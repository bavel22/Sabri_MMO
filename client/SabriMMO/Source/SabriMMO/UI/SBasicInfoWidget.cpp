// SBasicInfoWidget.cpp — Draggable & resizable Slate HUD (RO Classic brown/gold theme)

#include "SBasicInfoWidget.h"
#include "BasicInfoSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette — Brown & Gold Ornamental Fantasy
// NOTE: Use FLinearColor() directly — FColor() applies sRGB→linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace ROColors
{
	// Panel backgrounds
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium   (0.33f, 0.22f, 0.13f, 1.f);
	// Gold trim + highlights
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	// Bars — bright, saturated fill colors
	static const FLinearColor HPRed         (0.85f, 0.15f, 0.15f, 1.f);
	static const FLinearColor SPBlue        (0.20f, 0.45f, 0.90f, 1.f);
	static const FLinearColor EXPYellow     (0.90f, 0.75f, 0.10f, 1.f);
	static const FLinearColor JobExpOrange   (0.90f, 0.55f, 0.10f, 1.f);
	static const FLinearColor WeightGreen   (0.25f, 0.75f, 0.20f, 1.f);
	// Bar background
	static const FLinearColor BarBg         (0.10f, 0.07f, 0.04f, 1.f);
	// Text
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
}

// ============================================================
// Construction
// ============================================================
void SBasicInfoWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(CurrentSize.X)
		.HeightOverride(CurrentSize.Y)
		[
			// Outer gold trim border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(ROColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				// Inner dark inset
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ROColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					// Main brown panel
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ROColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)

						// --- Title Bar (draggable) ---
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildTitleBar()
						]

						// --- Content ---
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							BuildContentArea()
						]

						// --- Bottom row: Weight + Zuzucoin ---
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildBottomRow()
						]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Title bar — Name, Class, Levels (draggable handle)
// ============================================================
TSharedRef<SWidget> SBasicInfoWidget::BuildTitleBar()
{
	UBasicInfoSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ROColors::PanelDark)
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SHorizontalBox)

			// Player name (left)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(Sub->PlayerName);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(ROColors::TextPrimary))
			]

			// Spacer
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNullWidget::NullWidget
			]

			// Job class + levels (right)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("%s  Lv.%d / Job Lv.%d"),
						*Sub->JobClassDisplayName, Sub->BaseLevel, Sub->JobLevel));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
			]
		];
}

// ============================================================
// Gold divider line helper
// ============================================================
static TSharedRef<SWidget> BuildGoldDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(ROColors::GoldDivider)
		];
}

// ============================================================
// Content area — HP, SP, divider, Base EXP, Job EXP bars
// ============================================================
TSharedRef<SWidget> SBasicInfoWidget::BuildContentArea()
{
	UBasicInfoSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ROColors::PanelBrown)
		.Padding(FMargin(5.f, 3.f))
		[
			SNew(SVerticalBox)

			// ---- HP Bar ----
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
			[
				BuildBarRow(
					FText::FromString(TEXT("HP")),
					FSlateColor(ROColors::HPRed),
					TAttribute<TOptional<float>>::CreateLambda([Sub]() -> TOptional<float> {
						if (!Sub || Sub->MaxHP <= 0) return 0.f;
						return FMath::Clamp((float)Sub->CurrentHP / (float)Sub->MaxHP, 0.f, 1.f);
					}),
					TAttribute<FText>::CreateLambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("%d / %d"), Sub->CurrentHP, Sub->MaxHP));
					})
				)
			]

			// ---- SP Bar ----
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
			[
				BuildBarRow(
					FText::FromString(TEXT("SP")),
					FSlateColor(ROColors::SPBlue),
					TAttribute<TOptional<float>>::CreateLambda([Sub]() -> TOptional<float> {
						if (!Sub || Sub->MaxSP <= 0) return 0.f;
						return FMath::Clamp((float)Sub->CurrentSP / (float)Sub->MaxSP, 0.f, 1.f);
					}),
					TAttribute<FText>::CreateLambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("%d / %d"), Sub->CurrentSP, Sub->MaxSP));
					})
				)
			]

			// ---- Gold Divider ----
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildGoldDivider()
			]

			// ---- Base EXP ----
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
			[
				BuildBarRow(
					FText::FromString(TEXT("Base EXP")),
					FSlateColor(ROColors::EXPYellow),
					TAttribute<TOptional<float>>::CreateLambda([Sub]() -> TOptional<float> {
						if (!Sub || Sub->BaseExpNext <= 0) return 0.f;
						return FMath::Clamp((float)Sub->BaseExp / (float)Sub->BaseExpNext, 0.f, 1.f);
					}),
					TAttribute<FText>::CreateLambda([Sub]() -> FText {
						if (!Sub || Sub->BaseExpNext <= 0) return FText::FromString(TEXT("0.0 %"));
						float Pct = (float)Sub->BaseExp / (float)Sub->BaseExpNext * 100.f;
						return FText::FromString(FString::Printf(TEXT("%.1f %%"), Pct));
					})
				)
			]

			// ---- Job EXP ----
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)
			[
				BuildBarRow(
					FText::FromString(TEXT("Job EXP")),
					FSlateColor(ROColors::JobExpOrange),
					TAttribute<TOptional<float>>::CreateLambda([Sub]() -> TOptional<float> {
						if (!Sub || Sub->JobExpNext <= 0) return 0.f;
						return FMath::Clamp((float)Sub->JobExp / (float)Sub->JobExpNext, 0.f, 1.f);
					}),
					TAttribute<FText>::CreateLambda([Sub]() -> FText {
						if (!Sub || Sub->JobExpNext <= 0) return FText::FromString(TEXT("0.0 %"));
						float Pct = (float)Sub->JobExp / (float)Sub->JobExpNext * 100.f;
						return FText::FromString(FString::Printf(TEXT("%.1f %%"), Pct));
					})
				)
			]

			// ---- Gold Divider ----
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildGoldDivider()
			]
		];
}

// ============================================================
// Reusable bar row:  [Label]  [====bar with overlay text====]
// ============================================================
TSharedRef<SWidget> SBasicInfoWidget::BuildBarRow(
	TAttribute<FText> Label,
	const FSlateColor& BarColor,
	TAttribute<TOptional<float>> Percent,
	TAttribute<FText> ValueText)
{
	return SNew(SHorizontalBox)

		// Label (e.g. "HP", "Base EXP")
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 4, 0)
		[
			SNew(SBox)
			.WidthOverride(48.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ROColors::TextShadow)
			]
		]

		// Bar with dark background + colored fill + text overlay
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.HeightOverride(14.f)
			[
				// Gold border around bar
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ROColors::GoldDark)
				.Padding(FMargin(1.f))
				[
					SNew(SOverlay)

					// Dark bar background
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(ROColors::BarBg)
					]

					// Progress fill
					+ SOverlay::Slot()
					[
						SNew(SProgressBar)
						.Percent(Percent)
						.FillColorAndOpacity(BarColor)
					]

					// Value text centered on bar
					+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(ValueText)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(ROColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(ROColors::TextShadow)
					]
				]
			]
		];
}

// ============================================================
// Bottom row — Weight + Zuzucoin
// ============================================================
TSharedRef<SWidget> SBasicInfoWidget::BuildBottomRow()
{
	UBasicInfoSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ROColors::PanelMedium)
		.Padding(FMargin(5.f, 3.f))
		[
			SNew(SHorizontalBox)

			// Weight label + value
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("Weight: %d / %d"), Sub->CurrentWeight, Sub->MaxWeight));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(ROColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ROColors::TextShadow)
			]

			// Zeny label + value
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("Zeny: %d"), Sub->Zuzucoin));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(ROColors::ZuzucoinGold))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ROColors::TextShadow)
			]
		];
}

// ============================================================
// Apply position + size (render transform for position, SBox for size)
// ============================================================
void SBasicInfoWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));

	if (RootSizeBox.IsValid())
	{
		RootSizeBox->SetWidthOverride(CurrentSize.X);
		RootSizeBox->SetHeightOverride(CurrentSize.Y);
	}
}

// ============================================================
// Hit-test which edge(s) the mouse is near
// ============================================================
EResizeEdge SBasicInfoWidget::HitTestEdges(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D Size = MyGeometry.GetLocalSize();
	EResizeEdge Edge = EResizeEdge::None;

	if (LocalPos.X < ResizeGrabZone)                Edge |= EResizeEdge::Left;
	if (LocalPos.X > Size.X - ResizeGrabZone)       Edge |= EResizeEdge::Right;
	if (LocalPos.Y < ResizeGrabZone)                Edge |= EResizeEdge::Top;
	if (LocalPos.Y > Size.Y - ResizeGrabZone)       Edge |= EResizeEdge::Bottom;

	return Edge;
}

// ============================================================
// Mouse interaction — drag (title bar) + resize (edges/corners)
// ============================================================
FReply SBasicInfoWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();

	// Check for resize first (edges/corners)
	EResizeEdge Edge = HitTestEdges(MyGeometry, ScreenPos);
	if (Edge != EResizeEdge::None)
	{
		bIsResizing = true;
		ActiveResizeEdge = Edge;
		ResizeStartMouse = ScreenPos;
		ResizeStartSize  = CurrentSize;
		ResizeStartPos   = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	// Check for drag (title bar — top 20px of local space)
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	if (LocalPos.Y < 20.f)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SBasicInfoWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FReply SBasicInfoWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
// Cursor — show resize arrows when hovering edges/corners
// ============================================================
FCursorReply SBasicInfoWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	if (bIsResizing || bIsDragging)
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
