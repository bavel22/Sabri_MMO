// SSkillTreeWidget.h — Draggable Slate Skill Tree panel (RO Classic style)
// REWRITE v3: Grid layout with prerequisite lines, compact cells, hover tooltips.
// Inner SSkillGridPanel (cpp-local) handles OnPaint for connecting lines.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class USkillTreeSubsystem;
class SBox;
class SVerticalBox;
class SHorizontalBox;
class SScrollBox;
class SSkillGridPanel;  // file-local in .cpp

class SSkillTreeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSkillTreeWidget) {}
		SLATE_ARGUMENT(USkillTreeSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// Called by subsystem when new data arrives — schedules full rebuild (tabs + grid)
	void RebuildSkillContent();

	// Schedules grid-only rebuild (used by tab click handlers)
	void RebuildSkillGrid();

	// Slate Tick — performs deferred rebuilds safely outside event processing
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// --- safe subsystem access ---
	TWeakObjectPtr<USkillTreeSubsystem> OwningSubsystem;
	USkillTreeSubsystem* GetSub() const;

	// --- deferred rebuild flags ---
	bool bPendingFullRebuild = false;
	bool bPendingGridRebuild = false;

	// --- actual rebuild implementations ---
	void DoRebuildSkillContent();
	void DoRebuildSkillGrid();

	// --- window drag state ---
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(300.0, 100.0);
	FVector2D CurrentSize = FVector2D(540.0, 560.0);

	// --- skill drag state (drag skill icon to hotbar) ---
	bool bSkillDragInitiated = false;
	int32 DragSourceSkillId = 0;
	FString DragSourceSkillName;
	FString DragSourceSkillIcon;
	FVector2D SkillDragStartPos = FVector2D::ZeroVector;
	static constexpr float SkillDragThreshold = 5.f;

	void ApplyLayout();

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// --- widget refs for dynamic rebuild ---
	TSharedPtr<SVerticalBox> ClassTabsContainer;
	TSharedPtr<SScrollBox> SkillScrollBox;

	// --- grid panel (inner widget that draws prerequisite lines) ---
	TSharedPtr<SSkillGridPanel> GridPanel;

	// --- active class tab ---
	int32 ActiveClassTab = 0;

	// --- grid layout constants ---
	static constexpr float CELL_WIDTH   = 78.f;
	static constexpr float CELL_HEIGHT  = 88.f;
	static constexpr float CELL_HGAP    = 14.f;
	static constexpr float CELL_VGAP    = 24.f;
	static constexpr float GRID_PADDING = 8.f;
};
