// SVendingSetupPopup.h — Modal popup for configuring a vending shop.
// Merchant selects items from cart, sets quantities and prices, names the shop.
// Pattern adapted from SCraftingPopup + SCardCompoundPopup.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "VendingSubsystem.h"

class UVendingSubsystem;
class STextBlock;
class SEditableTextBox;
class SVerticalBox;

class SVendingSetupPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVendingSetupPopup)
		: _Subsystem(nullptr)
		, _MaxSlots(3)
	{}
		SLATE_ARGUMENT(UVendingSubsystem*, Subsystem)
		SLATE_ARGUMENT(TArray<FVendSetupItem>, CartItems)
		SLATE_ARGUMENT(int32, MaxSlots)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStatusMessage(const FString& Message, bool bIsError);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UVendingSubsystem> OwningSubsystem;
	TArray<FVendSetupItem> AllCartItems;
	int32 MaxVendSlots = 3;

	// Per-row state
	struct FRowState
	{
		bool bChecked = false;
		int32 Quantity = 1;
		int32 MaxQuantity = 1;
		int32 Price = 0;
	};
	TArray<FRowState> RowStates;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildItemList();
	TSharedRef<SWidget> BuildItemRow(int32 Index);
	TSharedRef<SWidget> BuildBottomBar();

	// Widgets
	TSharedPtr<SEditableTextBox> ShopNameTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<STextBlock> SelectedCountText;

	// Actions
	void OnOpenShopClicked();
	int32 CountCheckedItems() const;
	void UpdateSelectedCount();
	void DismissPopup();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
