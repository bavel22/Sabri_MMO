// SHotbarKeybindWidget.h — Keybind configuration panel for hotbar rows.
// Modal popup with 4x9 grid, click-to-rebind, conflict detection, save/reset.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UHotbarSubsystem;
class SBox;

class SHotbarKeybindWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHotbarKeybindWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UHotbarSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<UHotbarSubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// ---- builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildKeybindGrid();
	TSharedRef<SWidget> BuildKeybindCell(int32 RowIndex, int32 SlotIndex);
	TSharedRef<SWidget> BuildButtonRow();

	// ---- listening mode state ----
	bool bIsListening = false;
	int32 ListeningRow = -1;
	int32 ListeningSlot = -1;
	FString ConflictMessage;

	void StartListening(int32 Row, int32 Slot);
	void StopListening();
	void ApplyKeybind(FKey NewKey, bool bAlt, bool bCtrl, bool bShift);
	FString CheckConflict(int32 Row, int32 Slot, FKey Key, bool bAlt, bool bCtrl, bool bShift) const;

	// ---- drag (window movement) ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(0.0, 0.0);
	void ApplyLayout();

	FVector2D GetContentSize() const;

	// ---- key input ----
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	// ---- dimensions ----
	static constexpr float CellWidth = 42.f;
	static constexpr float CellHeight = 28.f;
	static constexpr float PanelWidth = 440.f;
	static constexpr float PanelHeight = 250.f;
};
