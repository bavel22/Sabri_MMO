// SEquipmentWidget.h — RO Classic equipment window with 10 slots (head x3, armor,
// weapon, shield, garment, footgear, accessory x2), character portrait,
// and drag-and-drop equip/unequip support.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UEquipmentSubsystem;
class UInventorySubsystem;
class SBox;

class SEquipmentWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEquipmentWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UEquipmentSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UEquipmentSubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// ---- layout builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildEquipmentLayout();
	TSharedRef<SWidget> BuildEquipSlot(const FString& SlotPosition);
	TSharedRef<SWidget> BuildPortrait();
	TSharedRef<SWidget> BuildTooltip(const FInventoryItem& Item);

	// ---- equipment layout rebuild ----
	TSharedPtr<SBox> EquipmentContainer;  // Wraps the equipment layout for rebuild
	void RebuildEquipmentSlots();
	uint32 LastDataVersion = 0;

	// ---- drag + window movement ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(590.0, 100.0);

	bool bIsResizing = false;
	FVector2D ResizeStartMouse = FVector2D::ZeroVector;
	FVector2D ResizeStartSize = FVector2D::ZeroVector;
	FVector2D CurrentSize = FVector2D(280.0, 340.0);
	static constexpr float MinWidth = 220.f;
	static constexpr float MinHeight = 260.f;
	static constexpr float TitleBarHeight = 20.f;
	static constexpr float ResizeGrabZone = 6.f;
	static constexpr float SlotBoxSize = 32.f;

	void ApplyLayout();
	FVector2D GetContentSize() const;

	// ---- item drag from equipment ----
	bool bDragInitiated = false;
	FVector2D DragStartPos = FVector2D::ZeroVector;
	FString DragSourceSlot;
	static constexpr float DragThreshold = 5.f;

	// ---- slot hit testing ----
	FString GetSlotAtPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const;

	// ---- input ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	// ---- helpers ----
	FLinearColor GetItemTypeColor(const FString& ItemType) const;
	UInventorySubsystem* GetInventorySubsystem() const;
};
