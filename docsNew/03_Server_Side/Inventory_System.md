# Inventory & Item System — Server-Side Documentation

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Card_System](Card_System.md) | [Database_Architecture](../01_Architecture/Database_Architecture.md) | [Combat_System](Combat_System.md)
> **RO Reference**: [RagnaCloneDocs/05_Items_Equipment_Cards.md](../../RagnaCloneDocs/05_Items_Equipment_Cards.md)
> **Item Database**: See [items/](../items/) for complete item reference

## Overview

The inventory system manages items, equipment, and consumables for all characters. **6,169 items** from the rAthena pre-renewal database are stored in PostgreSQL with canonical IDs. Item definitions are loaded into memory on server startup via `loadItemDefinitions()`. Per-character inventory uses the `character_inventory` table with support for stacking, equipping, and dropping items. All consumable effects are data-driven via `ro_item_effects.js`.

**Last Updated**: 2026-03-12 — Added RO Classic weight threshold system (50%/90%/100% penalties, cached weight, `weight:status` event).

---

## Constants

```javascript
const INVENTORY = {
    MAX_SLOTS: 100,           // Max inventory slots per character
    MAX_WEIGHT: 2000,         // Base max carry weight (before STR bonus)
    OVERWEIGHT_50: 0.5,       // Regen stops at 50% weight
    OVERWEIGHT_90: 0.9,       // Attack/skills blocked at 90% weight
};
```

---

## Weight System (RO Classic Overweight Thresholds)

### Max Weight Formula

`maxWeight = 2000 + STR * 30 + (Enlarge Weight Limit level * 200)`

### Thresholds

| Threshold | Effect |
|-----------|--------|
| < 50% | Normal — full regen, all actions |
| 50-89% | HP/SP/skill regen **stops** |
| >= 90% | Cannot **attack** or use **skills** |
| > 100% | Cannot **pick up loot** |

Item use (potions, wings) is never gated by weight.

### Cached Weight

`player.currentWeight` is cached in memory (set from DB on join, updated after every inventory mutation via `updatePlayerWeightCache()`). The cached value feeds all threshold checks — zero DB queries per regen/combat tick.

### Socket Event: `weight:status`

```json
{
    "characterId": 1,
    "currentWeight": 1500,
    "maxWeight": 3500,
    "ratio": 0.429,
    "isOverweight50": false,
    "isOverweight90": false,
    "isOverweight100": false
}
```

Sent on: join, any inventory mutation, STR allocation, Enlarge Weight Limit learned.

All `inventory:data` emissions also include `currentWeight` and `maxWeight` fields.

See skill `/sabrimmo-weight` for full enforcement details.

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
    4. Dual wield check (Assassin: route to weapon_left if right occupied)
    5. Katar check (auto-unequip left-hand weapon + shield)
    6. Two-hand weapon check (auto-unequip shield + left-hand weapon)
    7. Shield check (block if 2H; Assassin: unequip left-hand weapon)
    8. Auto-unequip existing item in TARGET position (by equipped_position, not equip_slot)
    9. Update is_equipped, equipped_position
    10. Track equippedWeaponRight/Left on player object
    11. Recalculate stat bonuses + derived stats (ASPD uses dual wield formula)
    12. Emit player:stats (with dualWield{}), inventory:data, inventory:equipped
```

**Dual Wield Equip Logic (Assassin/Assassin Cross only):**
- `canDualWield(jobClass)`: only assassin/assassin_cross
- `isValidLeftHandWeapon(weaponType)`: dagger, one_hand_sword, axe
- Right hand empty → equip to `weapon`
- Right hand occupied + left hand empty (not Katar) → equip to `weapon_left`
- Both hands occupied → replace right hand
- Katar equip → auto-unequip left-hand weapon + shield
- Shield equip → auto-unequip left-hand weapon for Assassin
- DB: `equipped_position = 'weapon_left'` (no schema changes needed)

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
| `server/src/ro_card_prefix_suffix.js` | Card naming data: 441 prefix/suffix texts + postfix flag set |
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
| "Too heavy to attack!" | combat:error — weight ratio >= 90% |
| "Too heavy to use skills!" | skill:error — weight ratio >= 90% |
| "Overweight" | skill:cast_failed reason — weight >= 90% during cast |

---

## Card Compound System

### Overview

**538 cards** from the rAthena pre-renewal database (item IDs 4001+) can be permanently compounded into slotted equipment. Cards provide passive bonuses — flat stat increases, combat modifiers, or armor element changes — that activate while the equipment is worn. Once compounded, a card cannot be removed (card removal is a future feature). The number of available slots is defined per-item in the `slots` column of the `items` table.

### `card:compound` Event Handler

```
Client: emit('card:compound', {cardInventoryId, equipInventoryId})
Server:
    1. Verify ownership of both card and equipment (character_id match)
    2. Validate card item_type === 'card'
    3. Validate equipment has equip_slot and slots > 0
    4. Check card_type compatibility (weapon cards → weapons, armor cards → armor/shield/garment/footgear/accessory)
    5. Check equipment has an empty slot (compounded_cards array length < slots)
    6. Insert card into equipment's compounded_cards JSONB array
    7. Remove card from inventory (quantity - 1 or delete)
    8. Call rebuildCardBonuses() if equipment is currently worn
    9. Emit inventory:data to refresh client
```

### Validation Pipeline

| Step | Check | Error |
|------|-------|-------|
| 1 | Card exists in player inventory | "Card not found in your inventory" |
| 2 | Equipment exists in player inventory | "Equipment not found in your inventory" |
| 3 | Card `item_type === 'card'` | "This item is not a card" |
| 4 | Equipment has `slots > 0` | "This equipment has no card slots" |
| 5 | Card type matches slot type (weapon/armor) | "This card cannot be placed in this equipment type" |
| 6 | `compounded_cards.length < slots` | "All card slots are already filled" |

### `rebuildCardBonuses(player)`

Called on `player:join`, `inventory:equip`, `inventory:unequip`, and `card:compound`. Queries all equipped items with their `compounded_cards` JSONB column and aggregates bonuses:

```javascript
async function rebuildCardBonuses(player) {
    // 1. Query all equipped items for this character
    // 2. For each equipped item, iterate compounded_cards JSONB array
    // 3. Look up each card_id in itemDefinitions
    // 4. Aggregate bonuses by type (flat stats, combat modifiers, element)
    // 5. Store aggregated result on player.cardBonuses
    // 6. Recalculate derived stats via getEffectiveStats()
}
```

The aggregated `player.cardBonuses` object is consumed by `getEffectiveStats()` alongside buff and passive bonuses.

### Card Bonus Types

| Bonus Type | Source | Example |
|------------|--------|---------|
| Flat stat bonuses | DB columns (`str_bonus`, `agi_bonus`, etc.) | Poring Card (+2 LUK, +1 FLEE) |
| Combat modifiers | Parsed from `script` column | Hydra Card (+20% damage vs Demi-Human) |
| Armor element change | `element` field in card definition | Pasana Card (armor becomes Fire 1) |
| Max HP/SP bonuses | DB columns (`max_hp_bonus`, `max_sp_bonus`) | Peco Peco Card (+10% Max HP) |
| Race/element/size ATK | Parsed race/ele/size modifiers from script | Vadon Card (+20% ATK vs Fire) |

### Stacking Rules

Card bonuses follow RO Classic stacking behavior:

- **Additive within category**: Two Hydra Cards (each +20% vs Demi-Human) = +40% vs Demi-Human
- **Multiplicative between categories**: Hydra (+20% race) × Skeleton Worker (+15% size) = `1.20 × 1.15 = 1.38` (38% total)

Categories for multiplicative stacking: `race`, `element`, `size`, `boss/normal`.

### Database Schema

The `compounded_cards` column on `character_inventory` stores an array of card IDs:

```sql
-- character_inventory
compounded_cards  JSONB  DEFAULT '[]'::jsonb
```

Example value for a weapon with 2 cards compounded:

```json
[4001, 4013]
```

Card definitions come from the `items` table where `item_type = 'card'`. Relevant columns: `card_type`, `card_prefix`, `card_suffix`, `script`.

### Card Naming Display

Compounded cards change equipment display names. 441 cards have prefix or suffix text populated in `card_prefix`/`card_suffix` DB columns. The client's `GetDisplayName()` assembles names with multiplier support (Double/Triple/Quadruple for duplicate cards). This is purely visual — the item's `name` column is never modified. See [Card System — Card Naming](./Card_System.md#card-naming-system-display-names) for full details.

### Cross-Reference

See [Card System](./Card_System.md) for full documentation including card drop sources, card type classification, card naming system, and the complete list of 538 card effects.
