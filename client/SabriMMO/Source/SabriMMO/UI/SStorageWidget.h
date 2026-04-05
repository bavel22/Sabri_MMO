// SStorageWidget.h — RO Classic-style Kafra Storage window with scrollable grid,
// tab filtering, drag-and-drop to/from inventory/cart, double-click to withdraw,
// slot count bar, and right-click inspect.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UStorageSubsystem;
class SBox;
class SVerticalBox;
class SScrollBox;
class SEditableTextBox;

class SStorageWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStorageWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UStorageSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UStorageSubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// ---- layout builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildTabBar();
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildSlotCountBar();
	TSharedRef<SWidget> BuildGridArea();
	TSharedRef<SWidget> BuildItemSlot(int32 SlotIndex);

	// ---- grid management ----
	void RebuildGrid();
	TSharedPtr<SVerticalBox> GridContainer;
	TSharedPtr<SScrollBox> GridScrollBox;
	int32 GridColumns = 10;
	uint32 LastDataVersion = 0;
	int32 LastTab = 0;

	// Cached filtered items for grid rendering
	TArray<FInventoryItem> FilteredItems;

	// ---- split/partial quantity popup (floating at icon position) ----
	bool bSplitPopupActive = false;
	int32 SplitSourceStorageId = 0;
	int32 SplitMaxQuantity = 0;
	TSharedPtr<SEditableTextBox> SplitInputBox;
	TSharedPtr<SBox> SplitPopupBox;
	void ShowSplitPopup(int32 StorageId, int32 MaxQty, const FVector2D& LocalPos);
	void HideSplitPopup();
	void ConfirmSplit();
	void RebuildSplitPopup(const FVector2D& LocalPos);

	// ---- item drag state ----
	bool bDragInitiated = false;
	FVector2D DragStartPos = FVector2D::ZeroVector;
	int32 DragSourceSlotIndex = -1;
	static constexpr float DragThreshold = 5.f;

	// ---- window drag (title bar movement) ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(200.0, 100.0);

	// ---- window resize (bottom-right grab) ----
	bool bIsResizing = false;
	FVector2D ResizeStartMouse = FVector2D::ZeroVector;
	FVector2D ResizeStartSize = FVector2D::ZeroVector;
	FVector2D CurrentSize = FVector2D(380.0, 380.0);
	static constexpr float MinWidth = 200.f;
	static constexpr float MinHeight = 200.f;
	static constexpr float TitleBarHeight = 20.f;
	static constexpr float TabBarHeight = 22.f;
	static constexpr float ResizeGrabZone = 6.f;

	void ApplyLayout();
	FVector2D GetContentSize() const;

	// ---- input handling ----
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
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
