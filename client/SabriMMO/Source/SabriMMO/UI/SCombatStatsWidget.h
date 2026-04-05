// SCombatStatsWidget.h — RO-style equipment/stats window showing all derived
// combat stats. Draggable & resizable Slate widget, reads data from CombatStatsSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SBasicInfoWidget.h" // EResizeEdge

class UCombatStatsSubsystem;
class SBox;

class SCombatStatsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCombatStatsWidget) {}
		SLATE_ARGUMENT(UCombatStatsSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	UCombatStatsSubsystem* Subsystem = nullptr;

	// --- drag state ---
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(50.0, 200.0);

	// --- resize state ---
	bool bIsResizing = false;
	EResizeEdge ActiveResizeEdge = EResizeEdge::None;
	FVector2D ResizeStartMouse = FVector2D::ZeroVector;
	FVector2D ResizeStartSize  = FVector2D::ZeroVector;
	FVector2D ResizeStartPos   = FVector2D::ZeroVector;
	FVector2D CurrentSize      = FVector2D(175.0, 285.0);
	static constexpr float MinWidth  = 155.f;
	static constexpr float MinHeight = 200.f;
	static constexpr float ResizeGrabZone = 6.f;

	void ApplyLayout();
	FVector2D GetContentSize() const;
	EResizeEdge HitTestEdges(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const;

	// --- size box ref for resize ---
	TSharedPtr<SBox> RootSizeBox;

	// --- helper builders ---
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildSectionHeader(const FString& Title);
	TSharedRef<SWidget> BuildStatRow(TAttribute<FText> Label, TAttribute<FText> Value);
	TSharedRef<SWidget> BuildAllocatableStatRow(
		const FString& StatLabel,
		TAttribute<FText> Value,
		TAttribute<FText> CostText,
		TAttribute<bool> ButtonEnabled,
		const FString& StatName);
	TSharedRef<SWidget> BuildStatPointsBar();
	TSharedRef<SWidget> BuildDivider();

	// --- stat allocation callback ---
	FReply OnAllocateStatClicked(FString StatName);

	// --- advanced stats button ---
	TSharedRef<SWidget> BuildAdvancedStatsButton();
	FReply OnAdvancedStatsClicked();
};
