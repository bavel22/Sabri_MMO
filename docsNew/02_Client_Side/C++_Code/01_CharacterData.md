# CharacterData.h

**File**: `Source/SabriMMO/CharacterData.h` (357 lines)
**Purpose**: Defines all shared data structures for the MMO client — character data, inventory items, shop items, drag-drop payloads, and server info. Used throughout Blueprints and C++ subsystems.

## Enums (2)

### EItemDragSource
```cpp
UENUM()
enum class EItemDragSource : uint8 { None, Inventory, Equipment };
```
Identifies where a dragged item originated.

### EItemDropTarget
```cpp
UENUM()
enum class EItemDropTarget : uint8 {
    Cancelled, InventorySlot, EquipmentSlot, EquipmentPortrait, GameWorld, HotbarSlot
};
```
Identifies where a dragged item was dropped.

## FCharacterData — USTRUCT(BlueprintType)

Mirrors server `GET /api/characters` response. Stored in `UMMOGameInstance::CharacterList` and `SelectedCharacter`. Parsed by `ParseCharacterFromJson()` in `MMOHttpManager.cpp`.

**32 fields:**

| # | Field | Type | Default | Category |
|---|-------|------|---------|----------|
| 1 | `CharacterId` | int32 | 0 | Identity |
| 2 | `Name` | FString | "" | Identity |
| 3 | `CharacterClass` | FString | "novice" | Identity |
| 4 | `Level` | int32 | 1 | Identity |
| 5 | `X` | float | 0.0f | Position |
| 6 | `Y` | float | 0.0f | Position |
| 7 | `Z` | float | 0.0f | Position |
| 8 | `Health` | int32 | 100 | Vitals |
| 9 | `MaxHealth` | int32 | 100 | Vitals |
| 10 | `Mana` | int32 | 100 | Vitals |
| 11 | `MaxMana` | int32 | 100 | Vitals |
| 12 | `JobLevel` | int32 | 1 | EXP & Leveling |
| 13 | `JobClass` | FString | "novice" | EXP & Leveling |
| 14 | `BaseExp` | int64 | 0 | EXP & Leveling |
| 15 | `JobExp` | int64 | 0 | EXP & Leveling |
| 16 | `SkillPoints` | int32 | 0 | EXP & Leveling |
| 17 | `Str` | int32 | 1 | Stats |
| 18 | `Agi` | int32 | 1 | Stats |
| 19 | `Vit` | int32 | 1 | Stats |
| 20 | `IntStat` | int32 | 1 | Stats (named IntStat to avoid C++ keyword) |
| 21 | `Dex` | int32 | 1 | Stats |
| 22 | `Luk` | int32 | 1 | Stats |
| 23 | `StatPoints` | int32 | 48 | Stats |
| 24 | `Zuzucoin` | int32 | 0 | Economy |
| 25 | `HairStyle` | int32 | 1 | Appearance |
| 26 | `HairColor` | int32 | 0 | Appearance |
| 27 | `Gender` | FString | "male" | Appearance |
| 28 | `ZoneName` | FString | "" | Zone |
| 29 | `LevelName` | FString | "" | Zone |
| 30 | `DeleteDate` | FString | "" | Meta |
| 31 | `CreatedAt` | FString | "" | Meta |
| 32 | `LastPlayed` | FString | "" | Meta |

### JSON Mapping (Server → C++)

| JSON Field | C++ Field | Notes |
|-----------|-----------|-------|
| `character_id` | `CharacterId` | |
| `name` | `Name` | |
| `class` | `CharacterClass` | Default "novice" |
| `level` | `Level` | |
| `x`, `y`, `z` | `X`, `Y`, `Z` | |
| `health` / `max_health` | `Health` / `MaxHealth` | MaxHealth min 1 |
| `mana` / `max_mana` | `Mana` / `MaxMana` | MaxMana min 1 |
| `job_level` | `JobLevel` | |
| `job_class` | `JobClass` | Default "novice" |
| `base_exp` / `job_exp` | `BaseExp` / `JobExp` | int64 |
| `skill_points` | `SkillPoints` | |
| `str`, `agi`, `vit`, `int_stat`, `dex`, `luk` | `Str`, `Agi`, `Vit`, `IntStat`, `Dex`, `Luk` | |
| `stat_points` | `StatPoints` | Default 48 |
| `zuzucoin` | `Zuzucoin` | |
| `hair_style` / `hair_color` | `HairStyle` / `HairColor` | |
| `gender` | `Gender` | Default "male" |
| `zone_name` | `ZoneName` | Default "prontera_south" |
| `level_name` | `LevelName` | Default "L_PrtSouth" |
| `delete_date` | `DeleteDate` | |
| `created_at` | `CreatedAt` | |
| `last_played` | `LastPlayed` | |

## FInventoryItem — USTRUCT(BlueprintType)

Mirrors server `inventory:data` event payload. 31 fields.

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `InventoryId` | int32 | 0 | |
| `ItemId` | int32 | 0 | |
| `Name` | FString | "" | |
| `Description` | FString | "" | |
| `ItemType` | FString | "" | weapon, armor, consumable, etc, card |
| `EquipSlot` | FString | "" | weapon, armor, shield, head_top/mid/low, footgear, garment, accessory |
| `Quantity` | int32 | 1 | |
| `bIsEquipped` | bool | false | |
| `EquippedPosition` | FString | "" | weapon, armor, shield, head_top/mid/low, footgear, garment, accessory_1, accessory_2 |
| `SlotIndex` | int32 | -1 | Position in inventory grid |
| `Weight` | int32 | 0 | |
| `Price` | int32 | 0 | |
| `ATK` | int32 | 0 | |
| `DEF` | int32 | 0 | |
| `MATK` | int32 | 0 | |
| `MDEF` | int32 | 0 | |
| `StrBonus` | int32 | 0 | |
| `AgiBonus` | int32 | 0 | |
| `VitBonus` | int32 | 0 | |
| `IntBonus` | int32 | 0 | |
| `DexBonus` | int32 | 0 | |
| `LukBonus` | int32 | 0 | |
| `MaxHPBonus` | int32 | 0 | |
| `MaxSPBonus` | int32 | 0 | |
| `RequiredLevel` | int32 | 1 | |
| `bStackable` | bool | false | |
| `MaxStack` | int32 | 1 | |
| `Icon` | FString | "" | |
| `WeaponType` | FString | "" | dagger, one_hand_sword, bow, mace, staff, spear, axe, whip, instrument |
| `ASPDModifier` | int32 | 0 | |
| `WeaponRange` | int32 | 150 | |

**Helper methods**: `IsValid()`, `IsEquippable()`, `IsConsumable()`

## FDraggedItem — USTRUCT()

Lightweight drag-drop payload. 11 fields.

| Field | Type | Default |
|-------|------|---------|
| `InventoryId` | int32 | 0 |
| `ItemId` | int32 | 0 |
| `Name` | FString | "" |
| `ItemType` | FString | "" |
| `EquipSlot` | FString | "" |
| `EquippedPosition` | FString | "" |
| `Icon` | FString | "" |
| `Quantity` | int32 | 1 |
| `bIsEquipped` | bool | false |
| `Source` | EItemDragSource | None |
| `SourceSlotIndex` | int32 | -1 |

**Helper methods**: `IsValid()`, `static FromItem(const FInventoryItem&, EItemDragSource)`

## FShopItem — USTRUCT()

NPC shop catalog item (from `shop:data` event). 26 fields.

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `ItemId` | int32 | 0 | |
| `Name` | FString | "" | |
| `Description` | FString | "" | |
| `ItemType` | FString | "" | |
| `Icon` | FString | "" | |
| `BuyPrice` | int32 | 0 | After Discount if applicable |
| `SellPrice` | int32 | 0 | After Overcharge if applicable |
| `Weight` | int32 | 0 | |
| `ATK`, `DEF`, `MATK`, `MDEF` | int32 | 0 | |
| `EquipSlot` | FString | "" | |
| `WeaponType` | FString | "" | |
| `WeaponRange` | int32 | 0 | |
| `ASPDModifier` | int32 | 0 | |
| `RequiredLevel` | int32 | 1 | |
| `bStackable` | bool | false | |
| `StrBonus`, `AgiBonus`, `VitBonus`, `IntBonus`, `DexBonus`, `LukBonus` | int32 | 0 | |
| `MaxHPBonus`, `MaxSPBonus` | int32 | 0 | |

**Helper method**: `IsValid()`

## FCartItem — USTRUCT()

Buy/sell shopping cart entry (client-side only). 7 fields.

| Field | Type | Default | Notes |
|-------|------|---------|-------|
| `ItemId` | int32 | 0 | For buy cart |
| `InventoryId` | int32 | 0 | For sell cart |
| `Name` | FString | "" | |
| `Icon` | FString | "" | |
| `Quantity` | int32 | 1 | |
| `UnitPrice` | int32 | 0 | Per-unit price |
| `Weight` | int32 | 0 | Per-unit weight |

**Helper methods**: `GetTotalPrice()`, `GetTotalWeight()`, `IsValid()`

## FServerInfo — USTRUCT(BlueprintType)

Server list entry from `GET /api/servers`. 8 fields, all `BlueprintReadWrite`.

| Field | Type | Default |
|-------|------|---------|
| `ServerId` | int32 | 0 |
| `Name` | FString | "" |
| `Host` | FString | "" |
| `Port` | int32 | 3001 |
| `Status` | FString | "" (online/offline/maintenance) |
| `Population` | int32 | 0 |
| `MaxPopulation` | int32 | 1000 |
| `Region` | FString | "" |

---

**Last Updated**: 2026-03-09
