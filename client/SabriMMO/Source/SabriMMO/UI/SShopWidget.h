// SShopWidget.h — RO Classic NPC shop window with Buy/Sell modes,
// shopping cart, quantity input popup, item tooltips, and drag support.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"
#include "ShopSubsystem.h"

class UShopSubsystem;
class SBox;
class SVerticalBox;
class SScrollBox;
class SEditableTextBox;
class SOverlay;

class SShopWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SShopWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UShopSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry,
		const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UShopSubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// ---- layout state ----
	uint32 LastDataVersion = 0;
	EShopMode LastMode = EShopMode::Closed;

	// ---- mode panels (all children of ContentOverlay, visibility toggled) ----
	TSharedPtr<SOverlay> ContentOverlay;
	TSharedPtr<SWidget> ModeSelectPanel;
	TSharedPtr<SWidget> BuyModePanel;
	TSharedPtr<SWidget> SellModePanel;
	TSharedPtr<SWidget> QuantityPopupPanel;

	// ---- builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildModeSelectPanel();
	TSharedRef<SWidget> BuildBuyModePanel();
	TSharedRef<SWidget> BuildSellModePanel();
	TSharedRef<SWidget> BuildQuantityPopupOverlay();

	// Buy mode sub-builders
	void RebuildShopItemList();
	void RebuildBuyCart();
	TSharedRef<SWidget> BuildShopItemRow(int32 ItemIndex);
	TSharedRef<SWidget> BuildBuyCartRow(int32 CartIndex);

	// Sell mode sub-builders
	void RebuildSellItemList();
	void RebuildSellCart();
	TSharedRef<SWidget> BuildSellItemRow(const FInventoryItem& Item, int32 SellPrice);
	TSharedRef<SWidget> BuildSellCartRow(int32 CartIndex);

	// Tooltip
	TSharedRef<SWidget> BuildItemTooltip(const FString& Desc, int32 ATK, int32 DEF,
		int32 MATK, int32 MDEF, int32 ReqLvl, int32 Weight, const FString& ItemType);

	// Scrollable containers
	TSharedPtr<SVerticalBox> ShopItemContainer;
	TSharedPtr<SVerticalBox> BuyCartContainer;
	TSharedPtr<SVerticalBox> SellItemContainer;
	TSharedPtr<SVerticalBox> SellCartContainer;

	void SwitchToMode(EShopMode NewMode);

	// ---- quantity input state ----
	bool bShowQuantityPopup = false;
	int32 QuantityItemIndex = -1;
	bool bQuantityIsBuy = true;
	int32 QuantityValue = 1;
	int32 QuantityMax = 99;
	int32 QuantityUnitPrice = 0;
	int32 QuantityWeight = 0;
	FString QuantityItemName;
	int32 QuantityInventoryId = 0; // For sell mode

	void ShowQuantityPopup(const FString& ItemName, int32 MaxQty, int32 UnitPrice, int32 UnitWeight,
		bool bIsBuy, int32 ItemIndex, int32 InvId = 0);
	void HideQuantityPopup();
	void ConfirmQuantity();
	void AdjustQuantity(int32 Delta);
	TSharedPtr<SEditableTextBox> QuantityTextBox;

	// ---- sell list cache ----
	TArray<FInventoryItem> CachedSellableItems;
	uint32 LastInventoryVersion = 0;

	// ---- helpers ----
	static FString FormatPrice(int32 Price);

	// ---- drag ----
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(200.0, 80.0);

	FVector2D CurrentSize = FVector2D(500.0, 420.0);
	static constexpr float MinWidth = 400.f;
	static constexpr float MinHeight = 300.f;
	static constexpr float TitleBarHeight = 22.f;

	void ApplyLayout();
	FVector2D GetContentSize() const;

	// ---- input ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override;
};
