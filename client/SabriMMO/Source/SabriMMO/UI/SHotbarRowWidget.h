// SHotbarRowWidget.h — Single hotbar row (9 slots), draggable, RO Classic theme.
// One instance per visible row. Managed by HotbarSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UHotbarSubsystem;
class SBox;
class SHorizontalBox;

class SHotbarRowWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHotbarRowWidget) : _Subsystem(nullptr), _RowIndex(0) {}
		SLATE_ARGUMENT(UHotbarSubsystem*, Subsystem)
		SLATE_ARGUMENT(int32, RowIndex)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UHotbarSubsystem> OwningSubsystem;
	int32 RowIndex = 0;
	uint32 LastDataVersion = 0;

	// Slot container for rebuild
	TSharedPtr<SHorizontalBox> SlotContainer;
	TSharedPtr<SBox> RootSizeBox;

	// ---- per-slot builders ----
	void RebuildSlots();
	TSharedRef<SWidget> BuildSlot(int32 SlotIndex);

	// ---- slot dimensions ----
	static constexpr float SlotSize = 34.f;
	static constexpr float IconSize = 28.f;
	static constexpr float HandleWidth = 20.f;
	static constexpr float GearWidth = 16.f;
	static constexpr float TotalWidth = HandleWidth + (SlotSize * 9) + GearWidth + 12.f;
	static constexpr float RowHeight = SlotSize + 8.f;

	// ---- drag (window movement) ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition;
	void ApplyLayout();

	// ---- hover tracking ----
	int32 HoveredSlotIndex = -1;
	bool bPositionInitialized = false;
	float QuantityRefreshTimer = 0.f;

	// ---- item drop tracking ----
	int32 GetSlotIndexAtPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const;
	FVector2D GetContentSize() const;

	// ---- mouse handlers ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
};
