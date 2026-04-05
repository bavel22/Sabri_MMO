// CartSubsystem.h — UWorldSubsystem that owns all cart inventory data
// and the SCartWidget lifecycle. Cart is the Merchant-class portable storage.
// Registers Socket.io event handlers via the persistent EventRouter.
// F10 key toggle handled by SabriMMOCharacter via Enhanced Input.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "CartSubsystem.generated.h"

class SCartWidget;

UCLASS()
class SABRIMMO_API UCartSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- cart data (read by SCartWidget) ----
	TArray<FInventoryItem> CartItems;   // Reuses FInventoryItem; cart_id maps to InventoryId
	int32 CartWeight = 0;
	int32 CartMaxWeight = 8000;
	bool bHasCart = false;
	uint32 DataVersion = 0;            // Incremented on every cart data change (widget polls in Tick)

	// ---- public API (called by widget / InventorySubsystem drag-drop) ----
	void MoveToCart(int32 InventoryId, int32 Amount);
	void MoveToInventory(int32 CartId, int32 Amount);
	void ToggleWidget();
	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

	// ---- item lookup ----
	FInventoryItem* FindItemByInventoryId(int32 InvId);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- event handlers ----
	void HandleCartData(const TSharedPtr<FJsonValue>& Data);
	void HandleCartError(const TSharedPtr<FJsonValue>& Data);
	void HandleCartEquipped(const TSharedPtr<FJsonValue>& Data);

	// ---- helpers ----
	FInventoryItem ParseCartItemFromJson(const TSharedPtr<FJsonObject>& Obj);

	// ---- state ----
	bool bWidgetVisible = false;

	TSharedPtr<SCartWidget> CartWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
