// SSkillTreeWidget.h — Draggable Slate Skill Tree panel (RO Classic style)
// REWRITE v2: Deferred rebuilds prevent widget-tree mutation during Slate event processing.
// All subsystem references use TWeakObjectPtr to prevent dangling pointer crashes.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class USkillTreeSubsystem;
class SBox;
class SVerticalBox;
class SHorizontalBox;
class SScrollBox;

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

	/** Returns subsystem pointer if still valid, nullptr otherwise. */
	USkillTreeSubsystem* GetSub() const;

	// --- deferred rebuild flags (set by public methods, consumed by Tick) ---
	bool bPendingFullRebuild = false;
	bool bPendingGridRebuild = false;

	// --- actual rebuild implementations (called only from Tick) ---
	void DoRebuildSkillContent();
	void DoRebuildSkillGrid();

	// --- window drag state ---
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(300.0, 100.0);
	FVector2D CurrentSize = FVector2D(460.0, 500.0);

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
	TSharedPtr<SVerticalBox> SkillContentBox;

	// --- active class tab ---
	int32 ActiveClassTab = 0;
};
