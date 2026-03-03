#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CharacterData.generated.h"

// ============================================================
// Drag-and-drop enums (used by Inventory + Equipment UIs)
// ============================================================

UENUM()
enum class EItemDragSource : uint8
{
	None,
	Inventory,
	Equipment
};

UENUM()
enum class EItemDropTarget : uint8
{
	Cancelled,
	InventorySlot,
	EquipmentSlot,
	EquipmentPortrait,
	GameWorld,
	HotbarSlot   // Future: drag to hotbar
};

// ============================================================
// Inventory item — mirrors server inventory:data payload
// ============================================================

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	int32 InventoryId = 0;
	int32 ItemId = 0;
	FString Name;
	FString Description;
	FString ItemType;           // weapon, armor, consumable, etc, card
	FString EquipSlot;          // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory
	int32 Quantity = 1;
	bool bIsEquipped = false;
	FString EquippedPosition;   // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory_1, accessory_2
	int32 SlotIndex = -1;       // Position in inventory grid (-1 = auto)
	int32 Weight = 0;
	int32 Price = 0;
	int32 ATK = 0;
	int32 DEF = 0;
	int32 MATK = 0;
	int32 MDEF = 0;
	int32 StrBonus = 0;
	int32 AgiBonus = 0;
	int32 VitBonus = 0;
	int32 IntBonus = 0;
	int32 DexBonus = 0;
	int32 LukBonus = 0;
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
	int32 RequiredLevel = 1;
	bool bStackable = false;
	int32 MaxStack = 1;
	FString Icon;
	FString WeaponType;         // dagger, one_hand_sword, bow, mace, staff, spear, axe, whip, instrument
	int32 ASPDModifier = 0;
	int32 WeaponRange = 150;

	bool IsValid() const { return InventoryId > 0; }
	bool IsEquippable() const { return !EquipSlot.IsEmpty(); }
	bool IsConsumable() const { return ItemType == TEXT("consumable"); }
};

// ============================================================
// Dragged item — lightweight payload during drag operations
// ============================================================

USTRUCT()
struct FDraggedItem
{
	GENERATED_BODY()

	int32 InventoryId = 0;
	int32 ItemId = 0;
	FString Name;
	FString ItemType;
	FString EquipSlot;
	FString EquippedPosition;
	FString Icon;
	int32 Quantity = 1;
	bool bIsEquipped = false;
	EItemDragSource Source = EItemDragSource::None;
	int32 SourceSlotIndex = -1;

	bool IsValid() const { return InventoryId > 0; }

	static FDraggedItem FromItem(const FInventoryItem& Item, EItemDragSource InSource)
	{
		FDraggedItem D;
		D.InventoryId = Item.InventoryId;
		D.ItemId = Item.ItemId;
		D.Name = Item.Name;
		D.ItemType = Item.ItemType;
		D.EquipSlot = Item.EquipSlot;
		D.EquippedPosition = Item.EquippedPosition;
		D.Icon = Item.Icon;
		D.Quantity = Item.Quantity;
		D.bIsEquipped = Item.bIsEquipped;
		D.Source = InSource;
		D.SourceSlotIndex = Item.SlotIndex;
		return D;
	}
};

// ============================================================
// Server info — returned by GET /api/servers
// ============================================================

USTRUCT(BlueprintType)
struct FServerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	int32 ServerId = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	FString Name;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	FString Host;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	int32 Port = 3001;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	FString Status;  // "online", "offline", "maintenance"

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	int32 Population = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	int32 MaxPopulation = 1000;

	UPROPERTY(BlueprintReadWrite, Category = "Server")
	FString Region;
};

// ============================================================
// Character data — mirrors server GET /api/characters response
// ============================================================

USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString CharacterClass;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Level;

    // Position
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float X;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Y;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Z;

    // Vitals
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Health;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 MaxHealth;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Mana;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 MaxMana;

    // EXP & Leveling (RO-style dual progression)
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 JobLevel;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString JobClass;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int64 BaseExp;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int64 JobExp;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 SkillPoints;

    // Stats
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Str;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Agi;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Vit;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 IntStat;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Dex;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Luk;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 StatPoints;

    // Economy
    UPROPERTY(BlueprintReadWrite, Category = "Economy")
    int32 Zuzucoin;

    // Appearance
    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    int32 HairStyle;

    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    int32 HairColor;

    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    FString Gender;

    // Meta
    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString DeleteDate;

    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString CreatedAt;

    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString LastPlayed;

    FCharacterData()
    {
        CharacterId = 0;
        Name = TEXT("");
        CharacterClass = TEXT("novice");
        Level = 1;
        X = 0.0f;
        Y = 0.0f;
        Z = 0.0f;
        Health = 100;
        MaxHealth = 100;
        Mana = 100;
        MaxMana = 100;
        JobLevel = 1;
        JobClass = TEXT("novice");
        BaseExp = 0;
        JobExp = 0;
        SkillPoints = 0;
        Str = 1;
        Agi = 1;
        Vit = 1;
        IntStat = 1;
        Dex = 1;
        Luk = 1;
        StatPoints = 48;
        Zuzucoin = 0;
        HairStyle = 1;
        HairColor = 0;
        Gender = TEXT("male");
    }
};
