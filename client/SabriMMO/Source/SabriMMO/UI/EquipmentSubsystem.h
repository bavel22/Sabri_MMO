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
	static const FString WeaponLeft = TEXT("weapon_left");  // Dual wield: Assassin/Assassin Cross only
	static const FString Shield     = TEXT("shield");
	static const FString Garment    = TEXT("garment");
	static const FString Footgear   = TEXT("footgear");
	static const FString Accessory1 = TEXT("accessory_1");
	static const FString Accessory2 = TEXT("accessory_2");
	static const FString Ammo       = TEXT("ammo");  // Arrows, bullets (RO Classic ammunition slot)

	// Check if a job class can dual wield (Assassin/Assassin Cross)
	inline bool CanDualWield(const FString& JobClass)
	{
		return JobClass == TEXT("assassin") || JobClass == TEXT("assassin_cross");
	}

	// Check if a weapon type is valid for left hand
	inline bool IsValidLeftHandWeapon(const FString& WeaponType)
	{
		return WeaponType == TEXT("dagger") || WeaponType == TEXT("one_hand_sword") || WeaponType == TEXT("axe");
	}

	// Map equip_slot (item definition) → valid equipped_positions
	inline TArray<FString> GetValidPositions(const FString& EquipSlot, const FString& JobClass = TEXT(""))
	{
		if (EquipSlot == TEXT("accessory")) return { Accessory1, Accessory2 };
		// Assassin: weapon items can go to weapon or weapon_left
		if (EquipSlot == TEXT("weapon") && CanDualWield(JobClass)) return { Weapon, WeaponLeft };
		if (EquipSlot == TEXT("Ammo") || EquipSlot == TEXT("ammo")) return { Ammo };
		return { EquipSlot };
	}

	// Display name for UI — Assassin shows "Left Hand" instead of "Shield" for weapon_left
	inline FString GetDisplayName(const FString& Position, const FString& JobClass = TEXT(""))
	{
		if (Position == HeadTop)    return TEXT("Head Top");
		if (Position == HeadMid)    return TEXT("Head Mid");
		if (Position == HeadLow)    return TEXT("Head Low");
		if (Position == Armor)      return TEXT("Armor");
		if (Position == Weapon)     return TEXT("Weapon");
		if (Position == WeaponLeft) return TEXT("Left Hand");
		if (Position == Shield)
		{
			// Assassin: show "Left Hand" for the shield slot position
			if (CanDualWield(JobClass)) return TEXT("Left Hand");
			return TEXT("Shield");
		}
		if (Position == Garment)    return TEXT("Garment");
		if (Position == Footgear)   return TEXT("Footgear");
		if (Position == Accessory1) return TEXT("Accessory");
		if (Position == Accessory2) return TEXT("Accessory");
		if (Position == Ammo)       return TEXT("Ammo");
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
	// Overload that also checks weapon type for dual wield
	bool CanEquipToSlot(const FString& ItemEquipSlot, const FString& SlotPosition, const FString& WeaponType) const;

	// Get the local player's job class for dual wield detection
	FString GetLocalJobClass() const;

	/** Broadcast when equipped items change (for sprite weapon mode updates) */
	DECLARE_MULTICAST_DELEGATE(FOnEquipmentChanged);
	FOnEquipmentChanged OnEquipmentChanged;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- widget visibility ----
	void ToggleWidget();
	bool IsWidgetVisible() const;

private:
	void HandleInventoryData(const TSharedPtr<FJsonValue>& Data);

	FTimerHandle RefreshTimerHandle;
	bool bWidgetVisible = false;
	int32 LocalCharacterId = 0;

	TSharedPtr<SEquipmentWidget> EquipWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
