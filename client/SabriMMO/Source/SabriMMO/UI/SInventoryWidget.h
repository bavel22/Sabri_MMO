// SInventoryWidget.h — RO Classic-style inventory window with tabs, scrollable grid,
// drag-and-drop, double-click equip/use, right-click tooltips, and quantity display.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UInventorySubsystem;
class SBox;
class SVerticalBox;
class SScrollBox;

class SInventoryWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SInventoryWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UInventorySubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UInventorySubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// ---- layout builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildTabBar();
	TSharedRef<SWidget> BuildGridArea();
	TSharedRef<SWidget> BuildBottomBar();
	TSharedRef<SWidget> BuildItemSlot(int32 SlotIndex);
	TSharedRef<SWidget> BuildItemIcon(const FInventoryItem& Item);
	TSharedRef<SWidget> BuildTooltip(const FInventoryItem& Item);

	// ---- grid management ----
	void RebuildGrid();
	TSharedPtr<SVerticalBox> GridContainer;
	TSharedPtr<SScrollBox> GridScrollBox;
	int32 GridColumns = 7;
	uint32 LastDataVersion = 0;
	int32 LastTabId = -1;

	// ---- tooltip ----
	TSharedPtr<SWidget> TooltipOverlay;
	int32 TooltipItemId = -1;
	void ShowTooltip(const FInventoryItem& Item, const FGeometry& Geometry, const FVector2D& ScreenPos);
	void HideTooltip();

	// ---- item drag state ----
	bool bDragInitiated = false;
	FVector2D DragStartPos = FVector2D::ZeroVector;
	int32 DragSourceInventoryId = 0;
	int32 DragSourceSlotIndex = -1;   // Visual slot index being dragged (for dimming)
	static constexpr float DragThreshold = 5.f;

	// ---- drag + resize (window movement) ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(300.0, 100.0);

	bool bIsResizing = false;
	FVector2D ResizeStartMouse = FVector2D::ZeroVector;
	FVector2D ResizeStartSize = FVector2D::ZeroVector;
	FVector2D CurrentSize = FVector2D(280.0, 340.0);
	static constexpr float MinWidth = 200.f;
	static constexpr float MinHeight = 200.f;
	static constexpr float TitleBarHeight = 20.f;
	static constexpr float ResizeGrabZone = 6.f;

	void ApplyLayout();
	FVector2D GetContentSize() const;

	// ---- input handling ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// ---- helpers ----
	int32 GetSlotIndexFromPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const;
	FInventoryItem GetItemAtSlot(int32 SlotIndex) const;
	FLinearColor GetItemTypeColor(const FString& ItemType) const;
};
