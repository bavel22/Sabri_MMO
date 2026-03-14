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
// Compounded card info — lightweight card data for display
// ============================================================

USTRUCT()
struct FCompoundedCardInfo
{
	GENERATED_BODY()

	int32 ItemId = 0;
	FString Name;
	FString Description;
	FString FullDescription;
	FString Icon;
	FString CardType;       // weapon, armor, shield, garment, footgear, headgear, accessory
	FString CardPrefix;
	FString CardSuffix;
	int32 Weight = 0;

	bool IsValid() const { return ItemId > 0; }
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
	FString FullDescription;        // Multi-line formatted description from DB
	FString ItemType;               // weapon, armor, consumable, etc, card
	FString EquipSlot;              // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory
	int32 Quantity = 1;
	bool bIsEquipped = false;
	FString EquippedPosition;       // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory_1, accessory_2
	int32 SlotIndex = -1;           // Position in inventory grid (-1 = auto)
	int32 Weight = 0;
	int32 Price = 0;
	int32 BuyPrice = 0;
	int32 SellPrice = 0;
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
	int32 HitBonus = 0;
	int32 FleeBonus = 0;
	int32 CriticalBonus = 0;
	int32 PerfectDodgeBonus = 0;
	int32 RequiredLevel = 1;
	bool bStackable = false;
	int32 MaxStack = 1;
	FString Icon;
	FString WeaponType;             // dagger, one_hand_sword, bow, mace, staff, spear, axe, whip, instrument
	int32 ASPDModifier = 0;
	int32 WeaponRange = 150;

	// --- Inspect/Tooltip fields ---
	int32 Slots = 0;                // Card slots (0-4)
	int32 WeaponLevel = 0;          // Weapon level (1-4), 0 for non-weapons
	bool bRefineable = false;
	int32 RefineLevel = 0;          // Current refine level (+0 to +10)
	FString JobsAllowed;            // "Swordman,Merchant,Thief" or "All"
	FString CardType;               // For cards: which slot type they compound on
	FString CardPrefix;             // Card prefix name ("Bloody", "Titan")
	FString CardSuffix;             // Card suffix name ("of Endure")
	bool bTwoHanded = false;
	FString Element;                // Weapon element: "neutral", "fire", etc.
	TArray<int32> CompoundedCards;  // Card item_ids per slot (-1 = empty, >0 = card ID)
	TArray<FCompoundedCardInfo> CompoundedCardDetails;  // Parallel card detail data

	bool IsValid() const { return InventoryId > 0; }
	bool IsEquippable() const { return !EquipSlot.IsEmpty(); }
	bool IsConsumable() const { return ItemType == TEXT("consumable"); }
	bool IsCard() const { return ItemType == TEXT("card"); }
	bool HasSlots() const { return Slots > 0; }

	/**
	 * Returns formatted display name with RO Classic card naming rules:
	 * "+7 Triple Bloody Boned Blade [4]"
	 *
	 * Duplicate cards get multipliers (Double/Triple/Quadruple) instead of repeating.
	 * Prefix cards go before name, suffix cards after, in slot insertion order.
	 * Slot count [N] always shows TOTAL slots, not remaining empty.
	 */
	FString GetDisplayName() const
	{
		FString Result;

		// 1. Refine prefix
		if (RefineLevel > 0)
			Result = FString::Printf(TEXT("+%d "), RefineLevel);

		// 2. Count unique cards and track insertion order
		static const TCHAR* Multipliers[] = { TEXT(""), TEXT("Double "), TEXT("Triple "), TEXT("Quadruple ") };

		struct FCardNaming
		{
			FString PrefixText;
			FString SuffixText;
			int32 Count = 0;  // 0-indexed: 0=first, 1=double, 2=triple, 3=quad
		};

		TMap<int32, FCardNaming> UniqueCards;
		TArray<int32> InsertionOrder;

		for (const FCompoundedCardInfo& Card : CompoundedCardDetails)
		{
			if (!Card.IsValid()) continue;

			if (FCardNaming* Existing = UniqueCards.Find(Card.ItemId))
			{
				Existing->Count++;
			}
			else
			{
				FCardNaming NewEntry;
				NewEntry.PrefixText = Card.CardPrefix;
				NewEntry.SuffixText = Card.CardSuffix;
				NewEntry.Count = 0;
				UniqueCards.Add(Card.ItemId, NewEntry);
				InsertionOrder.Add(Card.ItemId);
			}
		}

		// 3. Prefix cards (non-empty CardPrefix, in insertion order)
		for (int32 CardId : InsertionOrder)
		{
			const FCardNaming& CN = UniqueCards[CardId];
			if (CN.PrefixText.IsEmpty()) continue;
			const int32 MultIdx = FMath::Clamp(CN.Count, 0, 3);
			Result += Multipliers[MultIdx];
			Result += CN.PrefixText;
			Result += TEXT(" ");
		}

		// 4. Base name
		Result += Name;

		// 5. Suffix cards (non-empty CardSuffix, in insertion order)
		for (int32 CardId : InsertionOrder)
		{
			const FCardNaming& CN = UniqueCards[CardId];
			if (CN.SuffixText.IsEmpty()) continue;
			const int32 MultIdx = FMath::Clamp(CN.Count, 0, 3);
			Result += TEXT(" ");
			Result += Multipliers[MultIdx];
			Result += CN.SuffixText;
		}

		// 6. Slot count (total slots, NOT remaining empty — matches RO Classic)
		if (Slots > 0)
			Result += FString::Printf(TEXT(" [%d]"), Slots);

		return Result;
	}

	/** Convert a FCompoundedCardInfo into an FInventoryItem for inspect display */
	static FInventoryItem FromCardInfo(const FCompoundedCardInfo& Card)
	{
		FInventoryItem Item;
		Item.InventoryId = -1;  // Sentinel: not a real inventory item
		Item.ItemId = Card.ItemId;
		Item.Name = Card.Name;
		Item.Description = Card.Description;
		Item.FullDescription = Card.FullDescription;
		Item.Icon = Card.Icon;
		Item.ItemType = TEXT("card");
		Item.CardType = Card.CardType;
		Item.CardPrefix = Card.CardPrefix;
		Item.CardSuffix = Card.CardSuffix;
		Item.Weight = Card.Weight;
		return Item;
	}
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
// Shop item — item in NPC shop catalog (from shop:data event)
// ============================================================

USTRUCT()
struct FShopItem
{
	GENERATED_BODY()

	int32 ItemId = 0;
	FString Name;
	FString Description;
	FString FullDescription;
	FString ItemType;       // weapon, armor, consumable, etc
	FString Icon;
	int32 BuyPrice = 0;     // NPC buy price (after Discount if applicable)
	int32 SellPrice = 0;    // NPC sell price (after Overcharge if applicable)
	int32 Weight = 0;
	int32 ATK = 0;
	int32 DEF = 0;
	int32 MATK = 0;
	int32 MDEF = 0;
	FString EquipSlot;
	FString WeaponType;
	int32 WeaponRange = 0;
	int32 ASPDModifier = 0;
	int32 RequiredLevel = 1;
	bool bStackable = false;
	int32 StrBonus = 0;
	int32 AgiBonus = 0;
	int32 VitBonus = 0;
	int32 IntBonus = 0;
	int32 DexBonus = 0;
	int32 LukBonus = 0;
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
	int32 HitBonus = 0;
	int32 FleeBonus = 0;
	int32 CriticalBonus = 0;
	int32 PerfectDodgeBonus = 0;

	// --- Inspect fields ---
	int32 Slots = 0;
	int32 WeaponLevel = 0;
	bool bRefineable = false;
	FString JobsAllowed;
	FString CardType;
	FString CardPrefix;
	FString CardSuffix;
	bool bTwoHanded = false;
	FString Element;

	bool IsValid() const { return ItemId > 0; }

	/** Convert to FInventoryItem for shared tooltip/inspect display */
	FInventoryItem ToInspectableItem() const
	{
		FInventoryItem Item;
		Item.InventoryId = -1;  // Sentinel: not a real inventory item
		Item.ItemId = ItemId;
		Item.Name = Name;
		Item.Description = Description;
		Item.FullDescription = FullDescription;
		Item.ItemType = ItemType;
		Item.EquipSlot = EquipSlot;
		Item.Icon = Icon;
		Item.BuyPrice = BuyPrice;
		Item.SellPrice = SellPrice;
		Item.Weight = Weight;
		Item.ATK = ATK;
		Item.DEF = DEF;
		Item.MATK = MATK;
		Item.MDEF = MDEF;
		Item.WeaponType = WeaponType;
		Item.WeaponRange = WeaponRange;
		Item.ASPDModifier = ASPDModifier;
		Item.RequiredLevel = RequiredLevel;
		Item.bStackable = bStackable;
		Item.StrBonus = StrBonus;
		Item.AgiBonus = AgiBonus;
		Item.VitBonus = VitBonus;
		Item.IntBonus = IntBonus;
		Item.DexBonus = DexBonus;
		Item.LukBonus = LukBonus;
		Item.MaxHPBonus = MaxHPBonus;
		Item.MaxSPBonus = MaxSPBonus;
		Item.HitBonus = HitBonus;
		Item.FleeBonus = FleeBonus;
		Item.CriticalBonus = CriticalBonus;
		Item.PerfectDodgeBonus = PerfectDodgeBonus;
		Item.Slots = Slots;
		Item.WeaponLevel = WeaponLevel;
		Item.bRefineable = bRefineable;
		Item.JobsAllowed = JobsAllowed;
		Item.CardType = CardType;
		Item.CardPrefix = CardPrefix;
		Item.CardSuffix = CardSuffix;
		Item.bTwoHanded = bTwoHanded;
		Item.Element = Element;
		return Item;
	}
};

// ============================================================
// Cart item — entry in buy/sell shopping cart (client-side)
// ============================================================

USTRUCT()
struct FCartItem
{
	GENERATED_BODY()

	int32 ItemId = 0;         // For buy cart (shop item ID)
	int32 InventoryId = 0;    // For sell cart (player inventory ID)
	FString Name;
	FString Icon;
	int32 Quantity = 1;
	int32 UnitPrice = 0;      // Per-unit price (after Discount/Overcharge)
	int32 Weight = 0;         // Per-unit weight

	int32 GetTotalPrice() const { return UnitPrice * Quantity; }
	int32 GetTotalWeight() const { return Weight * Quantity; }
	bool IsValid() const { return ItemId > 0 || InventoryId > 0; }
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

    // Zone
    UPROPERTY(BlueprintReadWrite, Category = "Zone")
    FString ZoneName;

    UPROPERTY(BlueprintReadWrite, Category = "Zone")
    FString LevelName;

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
