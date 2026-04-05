// STradeWidget.h — Dual-panel trade UI widget (RO Classic style).
// Left: Your Offer (10 item slots + zeny input)
// Right: Partner's Offer (10 item slots + zeny display)
// Bottom: OK / Trade / Cancel buttons with state-dependent enable

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UTradeSubsystem;
class SVerticalBox;
class SEditableTextBox;

class STradeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STradeWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UTradeSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UTradeSubsystem> OwningSubsystem;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildItemPanel(bool bIsMyPanel);
	TSharedRef<SWidget> BuildItemSlot(int32 SlotIndex, bool bIsMyItem);
	TSharedRef<SWidget> BuildZenySection(bool bIsMine);
	TSharedRef<SWidget> BuildButtonRow();
	TSharedRef<SWidget> BuildLockIndicator(bool bIsMyLock);

	// Zeny input
	TSharedPtr<SEditableTextBox> ZenyInputBox;

	// Mouse handlers
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
