# Inventory & Item System — Server-Side Documentation

## Overview

The inventory system manages items, equipment, and consumables for all characters. **6,169 items** from the rAthena pre-renewal database are stored in PostgreSQL with canonical IDs. Item definitions are loaded into memory on server startup via `loadItemDefinitions()`. Per-character inventory uses the `character_inventory` table with support for stacking, equipping, and dropping items. All consumable effects are data-driven via `ro_item_effects.js`.

**Last Updated**: 2026-03-11 — Migrated to rAthena canonical IDs (6,169 items), data-driven consumable system, separate buy/sell prices.

---

## Constants

```javascript
const INVENTORY = {
    MAX_SLOTS: 100,    // Max inventory slots per character
    MAX_WEIGHT: 2000   // Base max carry weight (+ STR*30 + skills)
};
```

---

## Item Definition Cache

Two Maps loaded at server startup from the `items` table:

```javascript
const itemDefinitions = new Map();  // Map<item_id, DB row>  — 6,169 entries
const itemNameToId = new Map();     // Map<item name, item_id> — name→ID lookup for drops

async function loadItemDefinitions() {
    const result = await pool.query('SELECT * FROM items');
    itemDefinitions.clear();
    itemNameToId.clear();
    for (const row of result.rows) {
        itemDefinitions.set(row.item_id, row);
        itemNameToId.set(row.name, row.item_id);
    }
}
```

After `loadItemDefinitions()`, `resolveDropItemIds()` resolves monster drop item names to IDs:

```javascript
function resolveDropItemIds() {
    for (const t of Object.values(ENEMY_TEMPLATES)) {
        for (const d of t.drops) {
            if (d.itemName && !d.itemId) d.itemId = itemNameToId.get(d.itemName) || null;
        }
        for (const d of (t.mvpDrops || [])) {
            if (d.itemName && !d.itemId) d.itemId = itemNameToId.get(d.itemName) || null;
        }
    }
}
```

---

## Item Categories (rAthena Canonical IDs)

All items use rAthena canonical IDs. The old custom ID ranges (1xxx-5xxx) have been migrated.

| item_type | Description | ID Range Examples |
|-----------|-------------|-------------------|
| `consumable` | Potions, scrolls, wings, gems, food | 501-999 (Red Potion=501, Fly Wing=601) |
| `weapon` | All weapon subtypes | 1101+ (Sword=1101, Knife=1201, Bow=1701) |
| `armor` | Armor, shield, headgear, garment, footgear, accessory | 2101+ (Guard=2101, Cotton Shirt=2301) |
| `card` | Monster cards for equipment slots | 4001+ (Poring Card=4001) |
| `etc` | Loot, materials, ores, quest items | 7001+ (Jellopy=909, etc.) |
| `ammo` | Arrows, bullets, spheres | 1750+ |

### Key Consumable Items

| ID | Name | Effect |
|----|------|--------|
| 501 | Red Potion | HP +45~65 |
| 502 | Orange Potion | HP +105~145 |
| 503 | Yellow Potion | HP +175~235 |
| 504 | White Potion | HP +325~405 |
| 505 | Blue Potion | SP +40~60 |
| 506 | Green Potion | Cure: Poison, Silence, Blind |
| 522 | Mastela Fruit | HP +400~600 |
| 525 | Panacea | Cure all status effects |
| 526 | Royal Jelly | HP +325~405, SP +40~60, Cure all |
| 601 | Fly Wing | Random teleport (checks `noteleport` flag) |
| 602 | Butterfly Wing | Teleport to save point (checks `noreturn` flag) |

---

## Data-Driven Consumable System

### `ro_item_effects.js`

All consumable use-effects are defined in `server/src/ro_item_effects.js`, auto-generated from rAthena scripts. **490 effects** across 6 types:

| Type | Fields | Example |
|------|--------|---------|
| `heal` | `hpMin, hpMax, spMin, spMax` | Red Potion: `{ type:'heal', hpMin:45, hpMax:65 }` |
| `percentheal` | `hp, sp` (%) | Yggdrasil Berry: `{ type:'percentheal', hp:100, sp:100 }` |
| `cure` | `cures: [SC_...]` | Green Potion: `{ type:'cure', cures:['SC_Poison','SC_Silence','SC_Blind'] }` |
| `itemskill` | `skill, level` | Fly Wing: `{ type:'itemskill', skill:'AL_TELEPORT', level:1 }` |
| `sc_start` | `status, duration, value` | Buff potions (future) |
| `multi` | `effects: [...]` | Royal Jelly: heal + cure combined |

### Server Handler (index.js ~line 6322)

The `inventory:use` handler processes effects via `processEffect()`:
1. **Special items first**: Fly Wing (601) and Butterfly Wing (602) handled by `item_id` check
2. **Data-driven lookup**: `ITEM_USE_EFFECTS[item.item_id]`
3. **Effect processing**: `heal` → `randBetween(hpMin,hpMax)`, `cure` → `cleanseStatusEffects()`, `multi` → recursive
4. **SC_Name mapping**: `scToStatus('SC_Poison')` → `'poison'` for our status system

---

## NPC Shop System

### Shop Definitions (Canonical IDs)

```javascript
const NPC_SHOPS = {
    1: { name: 'Tool Dealer',
         itemIds: [501, 502, 503, 505, 506, 507, 508, 511, 510, 520, 602, 601] },
    2: { name: 'Weapon Dealer',
         itemIds: [1201, 1202, 1204, 1101, 1109, 1701, 1601, 1504, 1401, 1404, 1301, 1501, 1604, 1907, 1950] },
    3: { name: 'Armor Dealer',
         itemIds: [2301, 2312, 2314, 2101, 2220, 2401, 2321, 2208, 2213, 2609, 2298, 2262, 5113, 2207] },
    4: { name: 'General Store',
         itemIds: [501, 502, 503, 505, 506, 2301, 2312, 2314] }
};
```

### Pricing Model

Each item has separate `buy_price` and `sell_price` columns (from rAthena data):
- **Buy price**: `item.buy_price` (what player pays at NPC shop)
- **Sell price**: `item.sell_price` (what player gets selling to NPC)
- **Legacy fallback**: `item.buy_price || item.price * 2` / `item.sell_price || item.price`

Merchant skills modify prices:
- **Discount** (skill 601): reduces buy prices by `7 + (level-1)*2`% (levels 1-10 = 7-25%)
- **Overcharge** (skill 602): increases sell prices by same formula

### shop:open → shop:data

```javascript
socket.emit('shop:data', {
    shopId, shopName,
    items: shopItems.map(item => ({
        itemId, name, description, itemType, equipSlot,
        buyPrice: applyDiscount(item.buy_price || item.price * 2, discountPct),
        sellPrice: applyOvercharge(item.sell_price || item.price, overchargePct),
        weight, icon, atk, def, matk, mdef,
        strBonus, agiBonus, vitBonus, intBonus, dexBonus, lukBonus,
        maxHpBonus, maxSpBonus, weaponType, weaponRange, aspdModifier,
        requiredLevel, stackable
    })),
    playerZuzucoin, discountPercent, overchargePercent,
    currentWeight, maxWeight, usedSlots, maxSlots
});
```

---

## Core Functions

### addItemToInventory(characterId, itemId, quantity)

Handles stacking for stackable items and creates new entries for non-stackable:

```javascript
async function addItemToInventory(characterId, itemId, quantity = 1) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) return null;
    if (itemDef.stackable) {
        // Check for existing unequipped stack, add to it (capped at max_stack)
    }
    // Insert new entry
}
```

### getPlayerInventory(characterId)

JOINs `character_inventory` with `items` to return full item details:

```javascript
async function getPlayerInventory(characterId) {
    const result = await pool.query(
        `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
                ci.equipped_position,
                i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
                i.atk, i.def, i.matk, i.mdef,
                i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.stackable, i.icon,
                i.weapon_type, i.aspd_modifier, i.weapon_range,
                i.buy_price, i.sell_price
         FROM character_inventory ci
         JOIN items i ON ci.item_id = i.item_id
         WHERE ci.character_id = $1
         ORDER BY ci.slot_index ASC, ci.created_at ASC`,
        [characterId]
    );
    return result.rows;
}
```

### removeItemFromInventory(inventoryId, quantity)

- If `quantity` provided: reduces stack, deletes if result <= 0
- If `quantity` null: deletes entire entry

---

## Monster Drop System

### Drop Resolution

Monster drops defined by name in `ro_monster_templates.js` → resolved to item IDs via `itemNameToId` Map:

```javascript
function rollEnemyDrops(enemy) {
    const template = ENEMY_TEMPLATES[enemy.templateId];
    for (const drop of template.drops) {
        if (Math.random() < drop.chance) {
            // itemId from resolveDropItemIds() or fallback lookup
            const itemId = drop.itemId || (drop.itemName ? itemNameToId.get(drop.itemName) : null);
            const itemDef = itemId ? itemDefinitions.get(itemId) : null;
            if (itemDef) droppedItems.push({ itemId, name: itemDef.name, quantity });
        }
    }
}
```

Loot goes directly to killer's inventory (no ground drops). The `loot:drop` event is sent only to the killer for UI display.

---

## Socket.io Event Handlers

### inventory:load

```
Client: emit('inventory:load')
Server:
    1. getPlayerInventory(characterId) → emit('inventory:data', {items, zuzucoin})
    2. getPlayerHotbar(characterId)    → emit('hotbar:alldata', {slots})
```

### inventory:use

```
Client: emit('inventory:use', {inventoryId})
Server:
    1. Verify ownership (character_id match)
    2. Check item_type === 'consumable'
    3. Special items: Fly Wing (601), Butterfly Wing (602) — zone flag checks, teleport
    4. Data-driven: ITEM_USE_EFFECTS[item_id] → processEffect()
    5. Remove 1 from stack
    6. Save health/mana to DB if changed
    7. Broadcast combat:health_update to zone
    8. Emit inventory:used to requester
    9. Refresh inventory (emit inventory:data)
```

### inventory:equip

```
Client: emit('inventory:equip', {inventoryId, equip: true/false})
Server:
    1. Verify ownership
    2. Check item has equip_slot
    3. Check required_level
    4. Two-hand weapon check (auto-unequip shield)
    5. Auto-unequip existing item in same slot
    6. Update is_equipped, equipped_position
    7. Recalculate stat bonuses + derived stats
    8. Emit player:stats, inventory:data, player:equipment_changed
```

### inventory:drop / inventory:move

- `inventory:drop {inventoryId, quantity?}` — remove items, refresh inventory
- `inventory:move {inventoryId, targetSlot}` — slot swap/reorder, refresh inventory

### shop:buy_batch / shop:sell_batch

- `shop:buy_batch {shopId, items: [{itemId, quantity}]}` — batch buy with Discount
- `shop:sell_batch {items: [{inventoryId, quantity}]}` — batch sell with Overcharge

---

## Database Schema

### items table (6,169 rows)

40+ columns including: `item_id`, `name`, `aegis_name`, `description`, `full_description`, `item_type`, `equip_slot`, `weight`, `price`, `buy_price`, `sell_price`, `atk`, `def`, `matk`, `mdef`, stat bonuses (str/agi/vit/int/dex/luk), `max_hp_bonus`, `max_sp_bonus`, `hit_bonus`, `flee_bonus`, `critical_bonus`, `weapon_type`, `weapon_level`, `armor_level`, `slots`, `refineable`, `jobs_allowed`, `classes_allowed`, `script`, `equip_script`, `unequip_script`, `two_handed`, `element`, card fields, and more.

Full schema in `database/init.sql`.

### character_inventory table

Per-character item instances: `inventory_id`, `character_id`, `item_id` (FK → items), `quantity`, `is_equipped`, `slot_index`.

### character_hotbar table

Hotbar slot assignments with cascading deletes: `character_id`, `slot_index`, `inventory_id` (FK → character_inventory ON DELETE CASCADE), `item_id`, `item_name`.

---

## Database Setup

### Fresh Install
```sql
\i database/init.sql
\i scripts/output/canonical_items.sql
```

### Existing DB Migration
```sql
\i database/migrations/migrate_to_canonical_ids.sql
\i scripts/output/canonical_items.sql
```

---

## Key File References

| File | Purpose |
|------|---------|
| `server/src/index.js` | itemDefinitions/itemNameToId (~line 1375), loadItemDefinitions (~line 1458), resolveDropItemIds (~line 1177), NPC_SHOPS (~line 1379), inventory:use handler (~line 6195), shop events (~line 6738), weight system (~line 1442) |
| `server/src/ro_item_effects.js` | Auto-generated consumable use-effect data (490 effects) |
| `server/src/ro_monster_templates.js` | 509 RO monsters with drop tables (item names) |
| `database/init.sql` | Items table schema (40+ columns) |
| `scripts/output/canonical_items.sql` | 6,169 rAthena items (full data) |
| `scripts/generate_canonical_migration.js` | Master generator: YAML → SQL + JS |
| `database/migrations/migrate_to_canonical_ids.sql` | Migration from custom → canonical IDs |

---

## Error Handling

| Error | When |
|-------|------|
| "Invalid inventory ID" | inventoryId is NaN or 0 |
| "Item not found in your inventory" | Wrong character_id or nonexistent entry |
| "This item cannot be used" | item_type !== 'consumable' |
| "Cannot use this item" | No ITEM_USE_EFFECTS entry for item_id |
| "This item cannot be equipped" | No equip_slot defined |
| "Requires level X" | Player level < required_level |
| "Teleportation is blocked in this area" | Zone has `noteleport` flag (Fly Wing) |
| "Return is blocked in this area" | Zone has `noreturn` flag (Butterfly Wing) |
| "Shop not found" | shopId not in NPC_SHOPS |
| "Not enough Zuzucoin" | Insufficient funds |
