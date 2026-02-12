# Items & Inventory System

## Overview

Server-authoritative item and inventory system modeled after Ragnarok Online. Enemies drop items on death, players collect them automatically, and can use consumables, equip weapons/armor, and discard items. All item logic is validated server-side.

---

## Architecture

```
Client (Blueprint)                    Server (index.js)                    Database (PostgreSQL)
┌──────────────────┐                 ┌──────────────────────────────┐     ┌─────────────────────┐
│ WBP_Inventory    │ ── inventory:   │ Inventory Socket Events      │     │ items               │
│  - Item Grid     │    load/use/    │  - inventory:load            │     │  - item definitions │
│  - Equip Slots   │    equip/drop → │  - inventory:use             │ ──► │  - stats, type,     │
│  - Item Tooltip  │                 │  - inventory:equip           │     │    slot, weight      │
│                  │ ◄── inventory:  │  - inventory:drop            │     ├─────────────────────┤
│ Loot Popup       │    data/used/   │                              │     │ character_inventory  │
│  - Drop Notice   │    equipped/    │ Item Helpers                 │     │  - per-character     │
│                  │    dropped/     │  - loadItemDefinitions()     │ ──► │    item ownership    │
│                  │    error        │  - addItemToInventory()      │     │  - quantities        │
│                  │                 │  - removeItemFromInventory() │     │  - equipped state    │
│                  │ ◄── loot:drop   │  - getPlayerInventory()      │     └─────────────────────┘
│                  │                 │  - rollEnemyDrops()          │
└──────────────────┘                 └──────────────────────────────┘
```

---

## Database Schema

### `items` Table (Item Definitions)

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| item_id | SERIAL PK | auto | Unique item identifier |
| name | VARCHAR(100) | — | Display name |
| description | TEXT | '' | Item description |
| item_type | VARCHAR(20) | 'etc' | weapon, armor, garment, footgear, accessory, consumable, etc |
| equip_slot | VARCHAR(20) | NULL | head_top, head_mid, head_low, weapon, shield, armor, garment, footgear, accessory |
| weight | INTEGER | 0 | Item weight |
| price | INTEGER | 0 | NPC sell price |
| atk | INTEGER | 0 | Weapon ATK bonus |
| def | INTEGER | 0 | Armor DEF bonus |
| matk | INTEGER | 0 | Magic ATK bonus |
| mdef | INTEGER | 0 | Magic DEF bonus |
| str_bonus..luk_bonus | INTEGER | 0 | Stat bonuses when equipped |
| max_hp_bonus | INTEGER | 0 | Max HP bonus |
| max_sp_bonus | INTEGER | 0 | Max SP bonus |
| required_level | INTEGER | 1 | Minimum level to equip |
| stackable | BOOLEAN | false | Can stack in inventory |
| max_stack | INTEGER | 1 | Maximum stack size |
| icon | VARCHAR(100) | 'default_item' | Icon identifier for client |

### `character_inventory` Table (Player Inventories)

| Column | Type | Default | Description |
|--------|------|---------|-------------|
| inventory_id | SERIAL PK | auto | Unique inventory entry |
| character_id | INTEGER FK | — | Owner character |
| item_id | INTEGER FK | — | Item reference |
| quantity | INTEGER | 1 | Stack quantity |
| is_equipped | BOOLEAN | false | Currently equipped |
| slot_index | INTEGER | -1 | Inventory grid position |
| created_at | TIMESTAMP | now | When acquired |

---

## Item ID Ranges

| Range | Type | Examples |
|-------|------|----------|
| 1001-1999 | Consumables | Crimson Vial, Amber Elixir, Golden Salve, Roasted Haunch |
| 2001-2999 | Etc/Loot | Gloopy Residue, Viscous Slime, Chitin Shard, Downy Plume |
| 3001-3999 | Weapons | Rustic Shiv, Keen Edge, Iron Cleaver, Hunting Longbow |
| 4001-4999 | Armor | Linen Tunic, Quilted Vest, Ringweave Hauberk |

---

## Seed Items

### Consumables (1001-1005)
| ID | Name | HP Restore | SP Restore | Weight | Price |
|----|------|-----------|-----------|--------|-------|
| 1001 | Crimson Vial | 50 | — | 7 | 25 |
| 1002 | Amber Elixir | 150 | — | 10 | 100 |
| 1003 | Golden Salve | 350 | — | 13 | 275 |
| 1004 | Azure Philter | — | 60 | 15 | 500 |
| 1005 | Roasted Haunch | 70 | — | 15 | 25 |

### Etc/Loot (2001-2008)
| ID | Name | Weight | Price | Max Stack |
|----|------|--------|-------|-----------|
| 2001 | Gloopy Residue | 1 | 3 | 999 |
| 2002 | Viscous Slime | 1 | 7 | 999 |
| 2003 | Chitin Shard | 2 | 14 | 999 |
| 2004 | Downy Plume | 1 | 5 | 999 |
| 2005 | Spore Cluster | 1 | 10 | 999 |
| 2006 | Barbed Limb | 1 | 12 | 999 |
| 2007 | Verdant Leaf | 3 | 8 | 99 |
| 2008 | Silken Tuft | 1 | 4 | 999 |

### Weapons (3001-3006)
| ID | Name | ATK | Type | ASPD Mod | Range | Req Level | Weight | Price |
|----|------|-----|------|----------|-------|-----------|--------|-------|
| 3001 | Rustic Shiv | 17 | dagger | +5 | 150 | 1 | 40 | 50 |
| 3002 | Keen Edge | 30 | dagger | +5 | 150 | 1 | 40 | 150 |
| 3003 | Stiletto Fang | 43 | dagger | +5 | 150 | 12 | 60 | 500 |
| 3004 | Iron Cleaver | 25 | one_hand_sword | +0 | 150 | 2 | 80 | 100 |
| 3005 | Crescent Saber | 49 | one_hand_sword | +0 | 150 | 18 | 60 | 600 |
| 3006 | Hunting Longbow | 35 | bow | -3 | 800 | 4 | 50 | 400 |

### Weapon Type Properties
| weapon_type | ASPD Modifier | Range (UU) | Description |
|-------------|--------------|------------|-------------|
| dagger | +5 | 150 | Fast attack speed, standard melee range |
| one_hand_sword | +0 | 150 | Standard speed and melee range |
| bow | -3 | 800 | Slower attacks, significant ranged advantage |
| *(unarmed)* | 0 | 150 | Default when no weapon equipped |

### Armor (4001-4003)
| ID | Name | DEF | Req Level | Weight | Price |
|----|------|-----|-----------|--------|-------|
| 4001 | Linen Tunic | 1 | 1 | 10 | 20 |
| 4002 | Quilted Vest | 4 | 1 | 80 | 200 |
| 4003 | Ringweave Hauberk | 8 | 20 | 150 | 800 |

---

## Enemy Drop Tables

| Enemy | Drops (itemId @ chance) |
|-------|------------------------|
| **Blobby** (Lv1) | Gloopy Residue@70% (1-2), Viscous Slime@15%, Crimson Vial@5%, Rustic Shiv@1% |
| **Hoplet** (Lv3) | Gloopy Residue@50%, Downy Plume@30%, Verdant Leaf@10%, Crimson Vial@8%, Keen Edge@1% |
| **Crawlid** (Lv2) | Chitin Shard@50%, Barbed Limb@25%, Gloopy Residue@40%, Crimson Vial@5% |
| **Shroomkin** (Lv4) | Spore Cluster@55%, Verdant Leaf@20% (1-2), Crimson Vial@10%, Linen Tunic@2% |
| **Buzzer** (Lv5) | Barbed Limb@45%, Downy Plume@30%, Amber Elixir@5%, Iron Cleaver@2%, Quilted Vest@1%, Hunting Longbow@0.5% |
| **Mosswort** (Lv3) | Silken Tuft@60% (1-3), Verdant Leaf@25%, Viscous Slime@15%, Roasted Haunch@8% |

---

## Socket.io Events Reference

### Client → Server

| Event | Payload | Description |
|-------|---------|-------------|
| `inventory:load` | (none) | Request full inventory data |
| `inventory:use` | `{ inventoryId }` | Use a consumable item |
| `inventory:equip` | `{ inventoryId, equip: bool }` | Equip (true) or unequip (false) |
| `inventory:drop` | `{ inventoryId, quantity? }` | Discard item (quantity=null drops all) |

### Server → Client

| Event | Payload | Description |
|-------|---------|-------------|
| `inventory:data` | `{ items: [...] }` | Full inventory with item details |
| `inventory:used` | `{ inventoryId, itemName, healed, spRestored, health, mana, ... }` | Consumable use result |
| `inventory:equipped` | `{ inventoryId, itemName, equipped, slot, weaponType, attackRange, aspd, attackIntervalMs }` | Equip/unequip confirmation with weapon stats |
| `inventory:dropped` | `{ inventoryId, itemName, quantity }` | Item discard confirmation |
| `inventory:error` | `{ message }` | Error message |
| `loot:drop` | `{ enemyId, enemyName, items: [{itemId, itemName, quantity, icon, itemType}] }` | Items received from enemy kill |

---

## Server Implementation

### Key Functions

- **`loadItemDefinitions()`** — Loads all items from DB into `itemDefinitions` Map on startup
- **`rollEnemyDrops(enemy)`** — Rolls each drop chance independently, returns array of dropped items
- **`addItemToInventory(charId, itemId, qty)`** — Adds item to inventory (stacks if stackable)
- **`removeItemFromInventory(invId, qty)`** — Removes item or partial stack
- **`getPlayerInventory(charId)`** — Returns full inventory with joined item data

### Startup Flow
1. Create `items` table if not exists
2. Create `character_inventory` table if not exists
3. Seed base items if table is empty (16 items)
4. Load all item definitions into memory cache
5. Enemy templates reference itemIds in their `drops` arrays

### Combat Integration
- On enemy death → `rollEnemyDrops()` → `addItemToInventory()` for killer → `loot:drop` event
- On `inventory:equip` weapon → updates `player.stats.weaponATK`, `player.attackRange`, `player.weaponAspdMod` → recalculates derived stats + ASPD
- On player join → loads equipped weapon ATK, weapon_range, aspd_modifier from DB
- Weapon range affects combat range check (all melee: 150, bow: 800, unarmed: 150)
- Weapon ASPD modifier is added to stat-derived ASPD (daggers: +5, swords: +0, bow: -3)
- Default MELEE_RANGE constant is 150 Unreal units

---

## Client Implementation (Blueprint)

### WBP_InventoryWindow
- Equipment slots row (Weapon, Armor) with info text showing equipped item stats
- ScrollBox with WrapBox (ItemGrid) holding WBP_InventorySlot children
- Item count display, Close button
- Functions: PopulateInventory(Data), UseItem(invId), EquipItem(invId), UnequipItem(invId), DropItem(invId)
- Opened via I key → emits `inventory:load` → server responds with `inventory:data`

### WBP_InventorySlot (Child Widget)
- 56×56 button with item icon, quantity badge, equipped indicator (green dot)
- Variables: InventoryId, ItemId, ItemName, ItemType, Quantity, IsEquipped, EquipSlot, ATK, DEF, WeaponType, WeaponRange, AspdModifier
- Click: consumable → UseItem, equippable → EquipItem/UnequipItem toggle

### WBP_LootPopup
- Shows items received on enemy kill with enemy name header
- Auto-fades after 3 seconds using Event Tick opacity reduction
- Child widget: WBP_LootItemLine (icon + text per dropped item)

### BP_SocketManager Bindings
- `inventory:data` → OnInventoryData → calls PopulateInventory on WBP_InventoryWindow
- `inventory:used` → OnItemUsed → debug print
- `inventory:equipped` → OnItemEquipped → debug print with weapon stats
- `inventory:dropped` → OnItemDropped → debug print
- `inventory:error` → OnInventoryError → error print
- `loot:drop` → OnLootDrop → creates WBP_LootPopup, calls ShowLoot

### Hotkey
- **I** key (IA_ToggleInventory) → toggle WBP_InventoryWindow on/off
- On open: emit `inventory:load`, set input mode Game and UI
- On close: remove widget, set input mode Game Only

---

## Related Files

| File | Purpose |
|------|---------|
| `server/src/index.js` | Item system, inventory events, drop logic |
| `database/init.sql` | Item tables + seed data |
| `docs/Enemy_Combat_System.md` | Enemy drop tables |
| `docs/Ragnarok_Online_Reference.md` | RO equipment reference |

---

## Troubleshooting

### Issue: "Accessed None trying to read property SocketIORef in WBP_InventoryWindow"
**Cause:** The `SocketIORef` variable in WBP_InventoryWindow is not set before `UseItem`/`EquipItem`/`DropItem` try to emit Socket.io events.
**Fix:** See Blueprint fix below — the parent that creates WBP_InventoryWindow (BP_MMOCharacter or BP_SocketManager) must pass the Socket.io component reference when creating the widget, either via **Expose on Spawn** or by calling a **Set SocketIORef** setter immediately after `Create Widget`.

---

**Last Updated**: 2026-02-12
**Version**: 1.2
**Status**: Items renamed to RO-inspired names, bow added, melee range standardized to 150 UU, ASPD modifiers updated, SocketIORef bug documented
