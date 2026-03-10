// EquipmentSubsystem.h — UWorldSubsystem that tracks equipped items by slot position
// and manages the SEquipmentWidget lifecycle. Reads data from InventorySubsystem.
// F7 key toggle handled by SabriMMOCharacter via Enhanced Input.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "EquipmentSubsystem.generated.h"

class UInventorySubsystem;
class SEquipmentWidget;

// All RO Classic equipment slot positions
namespace EquipSlots
{
	static const FString HeadTop    = TEXT("head_top");
	static const FString HeadMid    = TEXT("head_mid");
	static const FString HeadLow    = TEXT("head_low");
	static const FString Armor      = TEXT("armor");
	static const FString Weapon     = TEXT("weapon");
	static const FString Shield     = TEXT("shield");
	static const FString Garment    = TEXT("garment");
	static const FString Footgear   = TEXT("footgear");
	static const FString Accessory1 = TEXT("accessory_1");
	static const FString Accessory2 = TEXT("accessory_2");

	// Map equip_slot (item definition) → valid equipped_positions
	inline TArray<FString> GetValidPositions(const FString& EquipSlot)
	{
		if (EquipSlot == TEXT("accessory")) return { Accessory1, Accessory2 };
		return { EquipSlot };
	}

	// Display name for UI
	inline FString GetDisplayName(const FString& Position)
	{
		if (Position == HeadTop)    return TEXT("Head Top");
		if (Position == HeadMid)    return TEXT("Head Mid");
		if (Position == HeadLow)    return TEXT("Head Low");
		if (Position == Armor)      return TEXT("Armor");
		if (Position == Weapon)     return TEXT("Weapon");
		if (Position == Shield)     return TEXT("Shield");
		if (Position == Garment)    return TEXT("Garment");
		if (Position == Footgear)   return TEXT("Footgear");
		if (Position == Accessory1) return TEXT("Accessory");
		if (Position == Accessory2) return TEXT("Accessory");
		return Position;
	}
}

UCLASS()
class SABRIMMO_API UEquipmentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- equipped items by slot position ----
	TMap<FString, FInventoryItem> EquippedSlots;

	// Rebuild equipped slots from InventorySubsystem data
	void RefreshEquippedSlots();

	// Get item in a specific slot
	FInventoryItem GetEquippedItem(const FString& SlotPosition) const;
	bool IsSlotOccupied(const FString& SlotPosition) const;

	// Unequip by slot position
	void UnequipSlot(const FString& SlotPosition);

	// Check if a dragged item can go in a specific slot
	bool CanEquipToSlot(const FString& ItemEquipSlot, const FString& SlotPosition) const;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- widget visibility ----
	void ToggleWidget();
	bool IsWidgetVisible() const;

private:
	void HandleInventoryData(const TSharedPtr<FJsonValue>& Data);

	bool bWidgetVisible = false;
	int32 LocalCharacterId = 0;

	TSharedPtr<SEquipmentWidget> EquipWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
