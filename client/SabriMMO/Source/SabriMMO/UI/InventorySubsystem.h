// InventorySubsystem.h — UWorldSubsystem that owns all inventory item data,
// drag-and-drop state, and the SInventoryWidget lifecycle.
// Registers Socket.io event handlers via the persistent EventRouter.
// F6 key toggle handled by SabriMMOCharacter via Enhanced Input.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "InventorySubsystem.generated.h"

class SInventoryWidget;

UCLASS()
class SABRIMMO_API UInventorySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- inventory data (read by widgets) ----
	TArray<FInventoryItem> Items;           // Full inventory
	int32 Zuzucoin = 0;
	int32 CurrentWeight = 0;
	int32 MaxWeight = 0;
	uint32 DataVersion = 0;  // Incremented on every inventory data change (widgets poll this in Tick)

	// ---- tab filtering ----
	int32 CurrentTab = 0;   // 0=Item(consumable), 1=Equip, 2=Etc
	TArray<FInventoryItem> GetFilteredItems() const;
	void SetTab(int32 Tab);

	// ---- drag-and-drop state (shared by inventory + equipment widgets) ----
	FDraggedItem DragState;
	bool bIsDragging = false;
	void StartDrag(const FInventoryItem& Item, EItemDragSource Source);
	void CompleteDrop(EItemDropTarget Target, const FString& SlotPosition = TEXT(""), int32 TargetSlotIndex = -1);
	void CancelDrag();
	void UpdateDragCursorPosition();  // Call from widget Tick to follow cursor

	// ---- item operations (emit to server) ----
	void UseItem(int32 InventoryId);
	void EquipItem(int32 InventoryId);
	void UnequipItem(int32 InventoryId);
	void DropItem(int32 InventoryId, int32 Quantity = 0);
	void MoveItem(int32 InventoryId, int32 NewSlotIndex);
	void RequestInventoryRefresh();

	// ---- widget lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ToggleWidget();
	bool IsWidgetVisible() const;

	// ---- item lookup ----
	FInventoryItem* FindItemByInventoryId(int32 InventoryId);
	TArray<FInventoryItem> GetEquippedItems() const;

	// ---- item icon utilities (reusable by any widget/system) ----
	FSlateBrush* GetOrCreateItemIconBrush(const FString& IconName);

private:
	// ---- event handlers ----
	void HandleInventoryData(const TSharedPtr<FJsonValue>& Data);
	void HandleInventoryEquipped(const TSharedPtr<FJsonValue>& Data);
	void HandleInventoryDropped(const TSharedPtr<FJsonValue>& Data);
	void HandleInventoryError(const TSharedPtr<FJsonValue>& Data);

	// ---- helpers ----
	FInventoryItem ParseItemFromJson(const TSharedPtr<FJsonObject>& Obj);
	void RecalculateWeight();

	// ---- state ----
	bool bWidgetVisible = false;
	int32 LocalCharacterId = 0;

	TSharedPtr<SInventoryWidget> InventoryWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;

	// ---- drag cursor overlay ----
	void ShowDragCursor(const FInventoryItem& Item);
	void HideDragCursor();
	TSharedPtr<SBox> DragCursorBox;
	TSharedPtr<SWidget> DragCursorAlignWrapper;  // Keeps drag cursor alive + constrains size
	TSharedPtr<SWidget> DragCursorOverlay;

	// ---- item icon brush cache (TSharedPtr so raw FSlateBrush* survives TMap rehash) ----
	TMap<FString, TSharedPtr<FSlateBrush>> ItemIconBrushCache;

	// ---- GC root for loaded textures (CRITICAL: prevents UTexture2D garbage collection) ----
	UPROPERTY()
	TMap<FString, TObjectPtr<UTexture2D>> ItemIconTextureCache;
};
