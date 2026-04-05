// SCardCompoundPopup.h — Transient popup for compounding a card into equipment.
// Shown when double-clicking a card in inventory (RO Classic behavior).
// Displays eligible unequipped equipment with open card slots matching the card type.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UInventorySubsystem;
class STextBlock;

class SCardCompoundPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCardCompoundPopup)
		: _Subsystem(nullptr)
	{}
		SLATE_ARGUMENT(UInventorySubsystem*, Subsystem)
		SLATE_ARGUMENT(FInventoryItem, Card)
		SLATE_ARGUMENT(TArray<FInventoryItem>, EligibleEquipment)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show error/status message at bottom of popup */
	void SetStatusMessage(const FString& Message, bool bIsError);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UInventorySubsystem> OwningSubsystem;
	FInventoryItem CardItem;
	TArray<FInventoryItem> EligibleItems;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildEquipmentList();
	TSharedRef<SWidget> BuildEquipmentRow(const FInventoryItem& Equipment);

	// Slot diamond helpers
	TSharedRef<SWidget> BuildSlotDiamonds(const FInventoryItem& Equipment);

	// Status message text block (for error display)
	TSharedPtr<STextBlock> StatusTextBlock;

	// Click handlers
	void OnEquipmentClicked(int32 EquipmentInventoryId);
	int32 FindFirstEmptySlot(const FInventoryItem& Equipment) const;
	void DismissPopup();

	// Input handling
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
