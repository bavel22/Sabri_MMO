// SBasicInfoWidget.h — Draggable & resizable Slate HUD panel (RO Classic style)
// Shows: Name, Job, Levels, HP/SP bars, Base/Job EXP bars, Weight, Zuzucoin

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UBasicInfoSubsystem;
class SBox;

// Resize edge flags (bitmask)
enum class EResizeEdge : uint8
{
	None   = 0,
	Left   = 1 << 0,
	Right  = 1 << 1,
	Top    = 1 << 2,
	Bottom = 1 << 3,
};
ENUM_CLASS_FLAGS(EResizeEdge);

class SBasicInfoWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBasicInfoWidget) {}
		SLATE_ARGUMENT(UBasicInfoSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// --- drag state ---
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(10.0, 10.0);

	// --- resize state ---
	bool bIsResizing = false;
	EResizeEdge ActiveResizeEdge = EResizeEdge::None;
	FVector2D ResizeStartMouse = FVector2D::ZeroVector;
	FVector2D ResizeStartSize  = FVector2D::ZeroVector;
	FVector2D ResizeStartPos   = FVector2D::ZeroVector;
	FVector2D CurrentSize      = FVector2D(200.0, 155.0);
	static constexpr float MinWidth  = 100.f;
	static constexpr float MinHeight = 60.f;
	static constexpr float ResizeGrabZone = 6.f;

	void ApplyLayout();
	EResizeEdge HitTestEdges(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	// --- data source ---
	TWeakObjectPtr<UBasicInfoSubsystem> OwningSubsystem;

	// --- size box ref for resize ---
	TSharedPtr<SBox> RootSizeBox;

	// --- helper builders ---
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildContentArea();
	TSharedRef<SWidget> BuildBarRow(
		TAttribute<FText> Label,
		const FSlateColor& BarColor,
		TAttribute<TOptional<float>> Percent,
		TAttribute<FText> ValueText
	);
	TSharedRef<SWidget> BuildBottomRow();
};
