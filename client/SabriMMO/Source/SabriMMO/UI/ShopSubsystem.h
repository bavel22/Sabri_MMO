// ShopSubsystem.h — UWorldSubsystem managing NPC shop state,
// Socket.io event wrapping, shopping cart, and SShopWidget lifecycle.
// RO Classic: Buy/Sell mode select, batch cart, Discount/Overcharge, weight checks.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "ShopSubsystem.generated.h"

class USocketIOClientComponent;
class SShopWidget;

UENUM()
enum class EShopMode : uint8
{
	Closed,
	ModeSelect,    // Buy/Sell/Cancel dialog
	BuyMode,       // Browsing shop catalog + cart
	SellMode       // Browsing inventory + sell cart
};

UCLASS()
class SABRIMMO_API UShopSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- shop state (read by widget) ----
	EShopMode CurrentMode = EShopMode::Closed;
	int32 CurrentShopId = 0;
	FString ShopName;
	TArray<FShopItem> ShopItems;
	int32 PlayerZuzucoin = 0;
	int32 DiscountPercent = 0;
	int32 OverchargePercent = 0;
	int32 CurrentWeight = 0;
	int32 MaxWeight = 2000;
	int32 UsedSlots = 0;
	int32 MaxSlots = 100;
	uint32 DataVersion = 0;
	FString LastErrorMessage;
	double ErrorExpireTime = 0.0;

	// ---- buy cart ----
	TArray<FCartItem> BuyCart;
	int32 GetBuyCartTotalCost() const;
	int32 GetBuyCartTotalWeight() const;
	void AddToBuyCart(const FShopItem& Item, int32 Quantity);
	void RemoveFromBuyCart(int32 CartIndex);
	void ClearBuyCart();

	// ---- sell cart ----
	TArray<FCartItem> SellCart;
	int32 GetSellCartTotalRevenue() const;
	void AddToSellCart(const FInventoryItem& Item, int32 Quantity, int32 SellPrice);
	void RemoveFromSellCart(int32 CartIndex);
	void ClearSellCart();

	// ---- sell list (filtered inventory) ----
	TArray<FInventoryItem> GetSellableItems() const;
	int32 GetSellPrice(const FInventoryItem& Item) const;

	// ---- shop operations (emit to server) ----
	void RequestOpenShop(int32 ShopId);
	void SubmitBuyCart();
	void SubmitSellCart();
	void CloseShop();

	// ---- mode transitions ----
	void SetMode(EShopMode NewMode);

	// ---- widget lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

private:
	// ---- socket event wrapping ----
	void TryWrapSocketEvents();
	void WrapSingleEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
	USocketIOClientComponent* FindSocketIOComponent() const;

	// ---- event handlers ----
	void HandleShopData(const TSharedPtr<FJsonValue>& Data);
	void HandleShopBought(const TSharedPtr<FJsonValue>& Data);
	void HandleShopSold(const TSharedPtr<FJsonValue>& Data);
	void HandleShopError(const TSharedPtr<FJsonValue>& Data);

	// ---- helpers ----
	FShopItem ParseShopItemFromJson(const TSharedPtr<FJsonObject>& Obj);

	// ---- state ----
	bool bEventsWrapped = false;
	bool bWidgetAdded = false;
	FTimerHandle BindCheckTimer;

	TSharedPtr<SShopWidget> ShopWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
