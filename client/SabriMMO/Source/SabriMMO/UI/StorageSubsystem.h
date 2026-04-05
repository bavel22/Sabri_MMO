// StorageSubsystem.h — UWorldSubsystem that owns Kafra Storage data
// and the SStorageWidget lifecycle. Storage is account-shared (300 slots).
// Registers Socket.io event handlers via the persistent EventRouter.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "StorageSubsystem.generated.h"

class SStorageWidget;

UCLASS()
class SABRIMMO_API UStorageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- storage data (read by SStorageWidget) ----
	TArray<FInventoryItem> StorageItems;   // Reuses FInventoryItem; storage_id maps to InventoryId
	int32 UsedSlots = 0;
	int32 MaxSlots = 300;
	bool bIsOpen = false;
	uint32 DataVersion = 0;            // Incremented on every data change (widget polls in Tick)

	// Tab filter (0=All, 1=Consumable, 2=Equipment, 3=Etc)
	int32 CurrentTab = 0;
	TArray<FInventoryItem> GetFilteredItems() const;

	// ---- public API ----
	void RequestOpen();
	void RequestClose();
	void DepositItem(int32 InventoryId, int32 Quantity);
	void WithdrawItem(int32 StorageId, int32 Quantity);
	void DepositFromCart(int32 CartId, int32 Quantity);
	void WithdrawToCart(int32 StorageId, int32 Quantity);
	void SortStorage(const FString& SortBy = TEXT("type"));  // "type", "name", "weight"

	// ---- search filter (set by widget, read by GetFilteredItems) ----
	FString SearchFilter;

	// ---- widget ----
	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

	// ---- item lookup ----
	FInventoryItem* FindItemByStorageId(int32 StorageId);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- event handlers ----
	void HandleStorageOpened(const TSharedPtr<FJsonValue>& Data);
	void HandleStorageClosed(const TSharedPtr<FJsonValue>& Data);
	void HandleStorageUpdated(const TSharedPtr<FJsonValue>& Data);
	void HandleStorageError(const TSharedPtr<FJsonValue>& Data);

	// ---- helpers ----
	FInventoryItem ParseStorageItemFromJson(const TSharedPtr<FJsonObject>& Obj);
	void ParseStorageItemsFromArray(const TArray<TSharedPtr<FJsonValue>>* ItemsArray);

	// ---- state ----
	bool bWidgetVisible = false;

	TSharedPtr<SStorageWidget> StorageWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
