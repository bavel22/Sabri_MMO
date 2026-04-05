// SIdentifyPopup.h — Transient popup for Item Appraisal (Identify).
// Shown when the server sends identify:item_list (via Magnifying Glass or skill).
// Displays unidentified items — click one to identify it.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UInventorySubsystem;
class STextBlock;
class SVerticalBox;

class SIdentifyPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SIdentifyPopup)
		: _Subsystem(nullptr)
		, _IsMagnifier(false)
	{}
		SLATE_ARGUMENT(UInventorySubsystem*, Subsystem)
		SLATE_ARGUMENT(TArray<FInventoryItem>, UnidentifiedItems)
		SLATE_ARGUMENT(bool, IsMagnifier)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show error/status message at bottom of popup */
	void SetStatusMessage(const FString& Message, bool bIsError = false);

	/** Remove an item from the list after successful identification */
	void RemoveItem(int32 InventoryId);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UInventorySubsystem> OwningSubsystem;
	TArray<FInventoryItem> PendingItems;
	bool bIsMagnifier = false;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildItemRow(const FInventoryItem& Item);
	void RebuildItemList();

	// Container for the scrollable item list (rebuilt on RemoveItem)
	TSharedPtr<SVerticalBox> ItemListContainer;

	// Status message text block (for error display)
	TSharedPtr<STextBlock> StatusTextBlock;

	// Click handlers
	void OnItemClicked(int32 InventoryId);
	void DismissPopup();

	// Input handling
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
