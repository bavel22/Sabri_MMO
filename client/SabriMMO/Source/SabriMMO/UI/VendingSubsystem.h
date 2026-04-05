// VendingSubsystem.h — UWorldSubsystem managing vending state and two popups:
// SVendingSetupPopup (merchant configures shop) and SVendingBrowsePopup (buyer browses).
// Registers Socket.io event handlers via the persistent EventRouter.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "VendingSubsystem.generated.h"

class SVendingSetupPopup;
class SVendingBrowsePopup;

// ============================================================
// Vending data structs
// ============================================================

// Item from cart that can be placed for vending
struct FVendSetupItem
{
	int32 CartId = 0;
	int32 ItemId = 0;
	FString Name;
	FString Icon;
	int32 Quantity = 1;
	int32 MaxQuantity = 1;
	int32 Price = 0;
	int32 Weight = 0;
};

// Item listed in another player's vending shop (browse mode)
struct FVendBrowseItem
{
	int32 VendItemId = 0;
	int32 ItemId = 0;
	FString Name;
	FString Icon;
	int32 Amount = 0;
	int32 Price = 0;
	int32 Weight = 0;
	int32 RefineLevel = 0;
	int32 Slots = 0;
	FString ItemType;
};

UCLASS()
class SABRIMMO_API UVendingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- vending state ----
	bool bIsVending = false;
	int32 VendingMaxSlots = 3;

	// ---- browse state (populated when viewing another player's shop OR own shop) ----
	int32 BrowseVendorId = 0;
	FString BrowseVendorName;
	FString BrowseShopTitle;
	TArray<FVendBrowseItem> BrowseItems;
	int32 PlayerZeny = 0;
	bool bBrowseIsOwnShop = false;  // True when vendor is viewing their own shop

	// ---- vendor's own shop state ----
	FString OwnShopTitle;
	TArray<FVendBrowseItem> OwnShopItems;  // Vendor's items (updated on sold)

	// ---- public API ----
	void OpenVendingSetup(int32 MaxSlots);
	void CloseVendingSetup();
	void StartVending(const FString& Title, const TArray<FVendSetupItem>& Items);
	void StopVending();
	void BrowseShop(int32 VendorId);
	void CloseBrowse();
	void BuyItem(int32 VendorId, int32 VendItemId, int32 Amount);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- socket event handlers ----
	void HandleVendingStarted(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingItemList(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingBuyResult(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingSold(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingShopClosed(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingError(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingSetup(const TSharedPtr<FJsonValue>& Data);

	// ---- popup lifecycle ----
	void ShowSetupPopup();
	void HideSetupPopup();
	void ShowBrowsePopup();
	void HideBrowsePopup();

	// ---- setup popup state ----
	TSharedPtr<SVendingSetupPopup> SetupPopup;
	TSharedPtr<SWidget> SetupAlignWrapper;
	TSharedPtr<SWidget> SetupOverlay;
	bool bSetupVisible = false;

	// ---- browse popup state ----
	TSharedPtr<SVendingBrowsePopup> BrowsePopup;
	TSharedPtr<SWidget> BrowseAlignWrapper;
	TSharedPtr<SWidget> BrowseOverlay;
	bool bBrowseVisible = false;
};
