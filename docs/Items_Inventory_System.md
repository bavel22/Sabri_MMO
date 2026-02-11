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
|-------|------|---------|
| 1001-1999 | Consumables | Red Potion, Orange Potion, Blue Potion, Meat |
| 2001-2999 | Etc/Loot | Jellopy, Sticky Mucus, Shell, Feather, Mushroom Spore |
| 3001-3999 | Weapons | Knife, Cutter, Main Gauche, Sword, Falchion |
| 4001-4999 | Armor | Cotton Shirt, Padded Armor, Chain Mail |

---

## Seed Items

### Consumables (1001-1005)
| ID | Name | HP Restore | SP Restore | Weight | Price |
|----|------|-----------|-----------|--------|-------|
| 1001 | Red Potion | 50 | — | 7 | 25 |
| 1002 | Orange Potion | 150 | — | 10 | 100 |
| 1003 | Yellow Potion | 350 | — | 13 | 275 |
| 1004 | Blue Potion | — | 60 | 15 | 500 |
| 1005 | Meat | 70 | — | 15 | 25 |

### Etc/Loot (2001-2008)
| ID | Name | Weight | Price | Max Stack |
|----|------|--------|-------|-----------|
| 2001 | Jellopy | 1 | 3 | 999 |
| 2002 | Sticky Mucus | 1 | 7 | 999 |
| 2003 | Shell | 2 | 14 | 999 |
| 2004 | Feather | 1 | 5 | 999 |
| 2005 | Mushroom Spore | 1 | 10 | 999 |
| 2006 | Insect Leg | 1 | 12 | 999 |
| 2007 | Green Herb | 3 | 8 | 99 |
| 2008 | Fluff | 1 | 4 | 999 |

### Weapons (3001-3005)
| ID | Name | ATK | Req Level | Weight | Price |
|----|------|-----|-----------|--------|-------|
| 3001 | Knife | 17 | 1 | 40 | 50 |
| 3002 | Cutter | 30 | 1 | 40 | 150 |
| 3003 | Main Gauche | 43 | 12 | 60 | 500 |
| 3004 | Sword | 25 | 2 | 80 | 100 |
| 3005 | Falchion | 49 | 18 | 60 | 600 |

### Armor (4001-4003)
| ID | Name | DEF | Req Level | Weight | Price |
|----|------|-----|-----------|--------|-------|
| 4001 | Cotton Shirt | 1 | 1 | 10 | 20 |
| 4002 | Padded Armor | 4 | 1 | 80 | 200 |
| 4003 | Chain Mail | 8 | 20 | 150 | 800 |

---

## Enemy Drop Tables

| Enemy | Drops (itemId @ chance) |
|-------|------------------------|
| **Blobby** (Lv1) | Jellopy@70% (1-2), Sticky Mucus@15%, Red Potion@5%, Knife@1% |
| **Hoplet** (Lv3) | Jellopy@50%, Feather@30%, Green Herb@10%, Red Potion@8%, Cutter@1% |
| **Crawlid** (Lv2) | Shell@50%, Insect Leg@25%, Jellopy@40%, Red Potion@5% |
| **Shroomkin** (Lv4) | Mushroom Spore@55%, Green Herb@20% (1-2), Red Potion@10%, Cotton Shirt@2% |
| **Buzzer** (Lv5) | Insect Leg@45%, Feather@30%, Orange Potion@5%, Sword@2%, Padded Armor@1% |
| **Mosswort** (Lv3) | Fluff@60% (1-3), Green Herb@25%, Sticky Mucus@15%, Meat@8% |

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
| `inventory:equipped` | `{ inventoryId, itemName, equipped, slot }` | Equip/unequip confirmation |
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
- On `inventory:equip` weapon → updates `player.stats.weaponATK` → recalculates derived stats
- On player join → loads equipped weapon ATK from DB into `baseStats.weaponATK`

---

## Client Implementation (Blueprint — TODO)

### WBP_InventoryWindow (New Widget)
- Grid of item slots (10 columns × 10 rows)
- Each slot shows item icon, quantity badge, equipped indicator
- Right-click context menu: Use, Equip, Drop
- Drag-and-drop for slot reordering (future)

### WBP_LootPopup (New Widget)
- Shows items received on enemy kill
- Auto-fades after 3 seconds
- Stacks multiple kills

### BP_SocketManager Bindings
- `inventory:data` → OnInventoryData
- `inventory:used` → OnItemUsed
- `inventory:equipped` → OnItemEquipped
- `inventory:dropped` → OnItemDropped
- `inventory:error` → OnInventoryError
- `loot:drop` → OnLootDrop

### Hotkey
- **I** key or button → emit `inventory:load` → open WBP_InventoryWindow

---

## Related Files

| File | Purpose |
|------|---------|
| `server/src/index.js` | Item system, inventory events, drop logic |
| `database/init.sql` | Item tables + seed data |
| `docs/Enemy_Combat_System.md` | Enemy drop tables |
| `docs/Ragnarok_Online_Reference.md` | RO equipment reference |

---

**Last Updated**: 2026-02-11
**Version**: 1.0
**Status**: Server-side complete, Blueprint UI pending
