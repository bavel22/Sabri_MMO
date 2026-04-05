// SVendingBrowsePopup.h — Modal popup for browsing another player's vending shop.
// Shows item list with icons, names, amounts, prices, and Buy buttons.
// Pattern adapted from SCraftingPopup + SCardCompoundPopup.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "VendingSubsystem.h"

class UVendingSubsystem;
class STextBlock;

class SVendingBrowsePopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVendingBrowsePopup)
		: _Subsystem(nullptr)
	{}
		SLATE_ARGUMENT(UVendingSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStatusMessage(const FString& Message, bool bIsError);
	void RebuildItemList();
	void UpdateItemAmount(int32 VendItemId, int32 NewAmount);
	void AddSaleMessage(const FString& BuyerName, const FString& ItemName, int32 Amount, int32 Price);

	virtual bool SupportsKeyboardFocus() const override { return true; }

	bool bIsVendorView = false;  // True = vendor viewing own shop (no buy buttons)

private:
	TWeakObjectPtr<UVendingSubsystem> OwningSubsystem;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildItemList();
	TSharedRef<SWidget> BuildItemRow(const FVendBrowseItem& Item);
	TSharedRef<SWidget> BuildBottomBar();

	// Widgets
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SVerticalBox> ItemListContainer;
	TSharedPtr<SVerticalBox> SaleLogContainer;  // Vendor sale notifications

	// Actions
	void OnBuyClicked(int32 VendItemId, int32 Quantity);
	void DismissPopup();

	// Helpers
	static FString FormatItemName(const FVendBrowseItem& Item);
	static FString FormatPrice(int32 Price);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
