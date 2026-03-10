# 06 -- Inventory, Equipment & Item System: UE5 C++ Implementation Guide

> Complete implementation guide for the Sabri_MMO inventory, equipment, refinement, card, and ground item systems.
> All code is server-authoritative. The client is presentation + input only.
> Stack: UE5.7 (C++ Slate) | Node.js + Express + Socket.io | PostgreSQL

---

## Table of Contents

1. [Server-Side Item Database](#1-server-side-item-database)
2. [Server-Side Inventory Management](#2-server-side-inventory-management)
3. [Server-Side Equipment System](#3-server-side-equipment-system)
4. [Server-Side Refinement/Upgrade](#4-server-side-refinementupgrade)
5. [Server-Side Card System](#5-server-side-card-system)
6. [Client-Side: Inventory Subsystem & UI](#6-client-side-inventory-subsystem--ui)
7. [Client-Side: Equipment Subsystem & UI](#7-client-side-equipment-subsystem--ui)
8. [Client-Side: Equipment Visualization on Character](#8-client-side-equipment-visualization-on-character)
9. [Client-Side: Refine UI](#9-client-side-refine-ui)
10. [Ground Items](#10-ground-items)

---

## 1. Server-Side Item Database

### 1.1 Database Schema: `items` Table

The `items` table holds all static item definitions. Every item in the game exists as exactly one row here. The current schema (from `database/init.sql`):

```sql
CREATE TABLE IF NOT EXISTS items (
    item_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT DEFAULT '',
    item_type VARCHAR(20) NOT NULL DEFAULT 'etc',
    equip_slot VARCHAR(20) DEFAULT NULL,
    weight INTEGER DEFAULT 0,
    price INTEGER DEFAULT 0,
    atk INTEGER DEFAULT 0,
    def INTEGER DEFAULT 0,
    matk INTEGER DEFAULT 0,
    mdef INTEGER DEFAULT 0,
    str_bonus INTEGER DEFAULT 0,
    agi_bonus INTEGER DEFAULT 0,
    vit_bonus INTEGER DEFAULT 0,
    int_bonus INTEGER DEFAULT 0,
    dex_bonus INTEGER DEFAULT 0,
    luk_bonus INTEGER DEFAULT 0,
    max_hp_bonus INTEGER DEFAULT 0,
    max_sp_bonus INTEGER DEFAULT 0,
    required_level INTEGER DEFAULT 1,
    stackable BOOLEAN DEFAULT false,
    max_stack INTEGER DEFAULT 1,
    icon VARCHAR(100) DEFAULT 'default_item'
);
```

**Migration required** -- Add columns for refinement, cards, weapon level, and class restrictions:

```sql
-- Migration: add_item_extended_fields.sql
ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_level INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS slots INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS attack_range INTEGER DEFAULT 150;
ALTER TABLE items ADD COLUMN IF NOT EXISTS buy_price INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS sell_price INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS class_restrictions TEXT DEFAULT NULL;
    -- JSON array: ["swordsman","mage","archer"] or NULL = all classes
ALTER TABLE items ADD COLUMN IF NOT EXISTS sub_type VARCHAR(30) DEFAULT NULL;
    -- For consumables: healing, buff, cure, teleport, gem, scroll
    -- For equipment: one_hand_sword, dagger, bow, staff, etc (mirrors weapon_type)
ALTER TABLE items ADD COLUMN IF NOT EXISTS hit_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS flee_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS critical_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS perfect_dodge_bonus INTEGER DEFAULT 0;

-- Backfill buy_price from existing price column (NPC sell price * 2 = buy price in RO)
UPDATE items SET buy_price = price * 2, sell_price = price WHERE buy_price = 0 AND price > 0;
```

### 1.2 Item ID Ranges

Following the existing convention in `database/init.sql`:

| ID Range | Category | Examples |
|----------|----------|----------|
| 1001-1999 | Consumables | Red Potion (1001), Fly Wing (1029) |
| 2001-2999 | Etc / Loot | Jellopy (2009), Phracon (2042) |
| 3001-3999 | Weapons | Knife (3007), Bow (3013) |
| 4001-4999 | Armor / Shields / Headgear | Guard (4004), Hat (4005) |
| 5001-5999 | Cards | Poring Card (5001), Fabre Card (5003) |
| 6001-6999 | Ammunition | (Future: Silver Arrow, Fire Arrow) |

### 1.3 `ro_item_mapping.js` -- Static Item Lookup

The existing `server/src/ro_item_mapping.js` provides name-to-ID mapping for the drop system. The server also loads all item definitions into an in-memory `Map` at startup:

```javascript
// Already in server/src/index.js — itemDefinitions loaded at startup
const itemDefinitions = new Map(); // item_id → full row from items table

async function loadItemDefinitions() {
    const result = await pool.query('SELECT * FROM items');
    for (const row of result.rows) {
        itemDefinitions.set(row.item_id, row);
    }
    logger.info(`[ITEMS] Loaded ${itemDefinitions.size} item definitions`);
}
```

### 1.4 Item Categories Enum

For clarity in server code, define category constants:

```javascript
const ITEM_TYPE = {
    CONSUMABLE: 'consumable',
    WEAPON: 'weapon',
    ARMOR: 'armor',
    CARD: 'card',
    ETC: 'etc',
    AMMO: 'ammo'
};

const EQUIP_SLOT = {
    WEAPON: 'weapon',
    SHIELD: 'shield',
    ARMOR: 'armor',
    HEAD_TOP: 'head_top',
    HEAD_MID: 'head_mid',
    HEAD_LOW: 'head_low',
    GARMENT: 'garment',
    FOOTGEAR: 'footgear',
    ACCESSORY: 'accessory'  // Maps to accessory_1 or accessory_2 at equip time
};
```

---

## 2. Server-Side Inventory Management

### 2.1 Database Schema: `character_inventory` Table

```sql
CREATE TABLE IF NOT EXISTS character_inventory (
    inventory_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    is_equipped BOOLEAN DEFAULT false,
    equipped_position VARCHAR(20) DEFAULT NULL,
    slot_index INTEGER DEFAULT -1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Migration required** -- Add per-instance fields for refinement and cards:

```sql
-- Migration: add_inventory_refine_cards.sql
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS refine_level INTEGER DEFAULT 0;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS card_slot_0 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS card_slot_1 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS card_slot_2 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS card_slot_3 INTEGER DEFAULT NULL;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS is_identified BOOLEAN DEFAULT true;
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS is_damaged BOOLEAN DEFAULT false;

-- Foreign keys for card slots (reference items table)
-- Cards are item_ids from the items table where item_type = 'card'
```

### 2.2 Core Inventory Functions (Server)

These functions already exist in `server/src/index.js`:

#### `addItemToInventory(characterId, itemId, quantity)`

```javascript
// Existing implementation (line ~1307 in index.js)
async function addItemToInventory(characterId, itemId, quantity = 1) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) {
        logger.error(`[ITEMS] Cannot add unknown item ${itemId} to inventory`);
        return null;
    }

    try {
        if (itemDef.stackable) {
            // Stack onto existing entry if possible
            const existing = await pool.query(
                'SELECT inventory_id, quantity FROM character_inventory WHERE character_id = $1 AND item_id = $2 AND is_equipped = false',
                [characterId, itemId]
            );
            if (existing.rows.length > 0) {
                const newQty = Math.min(existing.rows[0].quantity + quantity, itemDef.max_stack);
                await pool.query(
                    'UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2',
                    [newQty, existing.rows[0].inventory_id]
                );
                return { inventoryId: existing.rows[0].inventory_id, itemId, quantity: newQty, isEquipped: false };
            }
        }

        // Find next available slot_index
        const maxSlotResult = await pool.query(
            'SELECT COALESCE(MAX(slot_index), -1) as max_slot FROM character_inventory WHERE character_id = $1',
            [characterId]
        );
        const nextSlot = maxSlotResult.rows[0].max_slot + 1;

        const result = await pool.query(
            'INSERT INTO character_inventory (character_id, item_id, quantity, slot_index) VALUES ($1, $2, $3, $4) RETURNING inventory_id',
            [characterId, itemId, quantity, nextSlot]
        );
        return { inventoryId: result.rows[0].inventory_id, itemId, quantity, isEquipped: false };
    } catch (err) {
        logger.error(`[ITEMS] Failed to add item ${itemId} to char ${characterId}: ${err.message}`);
        return null;
    }
}
```

#### `removeItemFromInventory(inventoryId, quantity)`

```javascript
// Existing implementation (line ~1403 in index.js)
async function removeItemFromInventory(inventoryId, quantity = null) {
    try {
        if (quantity !== null) {
            const existing = await pool.query(
                'SELECT quantity FROM character_inventory WHERE inventory_id = $1', [inventoryId]
            );
            if (existing.rows.length === 0) return false;
            const newQty = existing.rows[0].quantity - quantity;
            if (newQty <= 0) {
                await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
            } else {
                await pool.query(
                    'UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2',
                    [newQty, inventoryId]
                );
            }
        } else {
            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
        }
        return true;
    } catch (err) {
        logger.error(`[ITEMS] Failed to remove inventory entry ${inventoryId}: ${err.message}`);
        return false;
    }
}
```

#### `getPlayerInventory(characterId)` -- Full Inventory Query

```javascript
// Existing implementation (line ~1354 in index.js)
async function getPlayerInventory(characterId) {
    try {
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
                    ci.equipped_position,
                    i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
                    i.atk, i.def, i.matk, i.mdef,
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.stackable, i.icon,
                    i.weapon_type, i.aspd_modifier, i.weapon_range
             FROM character_inventory ci
             JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1
             ORDER BY ci.slot_index ASC, ci.created_at ASC`,
            [characterId]
        );
        return result.rows;
    } catch (err) {
        logger.error(`[ITEMS] Failed to load inventory for char ${characterId}: ${err.message}`);
        return [];
    }
}
```

**Enhancement needed** -- Add refine and card columns to the query:

```javascript
// Updated getPlayerInventory with refine + card data
async function getPlayerInventory(characterId) {
    try {
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
                    ci.equipped_position, ci.refine_level,
                    ci.card_slot_0, ci.card_slot_1, ci.card_slot_2, ci.card_slot_3,
                    ci.is_identified, ci.is_damaged,
                    i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
                    i.atk, i.def, i.matk, i.mdef, i.weapon_level, i.slots,
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                    i.required_level, i.stackable, i.icon,
                    i.weapon_type, i.aspd_modifier, i.weapon_range, i.class_restrictions
             FROM character_inventory ci
             JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1
             ORDER BY ci.slot_index ASC, ci.created_at ASC`,
            [characterId]
        );
        return result.rows;
    } catch (err) {
        logger.error(`[ITEMS] Failed to load inventory for char ${characterId}: ${err.message}`);
        return [];
    }
}
```

### 2.3 Weight System

RO Classic weight formula (already implemented in `server/src/index.js` at line ~1226):

```javascript
// Max weight = 2000 + STR * 30 + Enlarge Weight Limit bonus
function getPlayerMaxWeight(player) {
    const str = (player.stats && player.stats.str) || 1;
    let maxW = 2000 + str * 30;
    const learned = player.learnedSkills || {};
    const ewlLevel = learned[600] || 0;
    if (ewlLevel > 0) {
        const skill = SKILL_MAP.get(600);
        if (skill) {
            const lvlData = skill.levels[Math.min(ewlLevel - 1, skill.levels.length - 1)];
            if (lvlData) maxW += lvlData.effectValue; // +200 per level
        }
    }
    return maxW;
}
```

**Weight threshold effects** (to be implemented):

```javascript
// Weight thresholds — RO Classic rules
function getWeightPenalty(currentWeight, maxWeight) {
    const ratio = currentWeight / maxWeight;
    if (ratio >= 0.90) {
        // 90%+ weight: cannot attack, cannot use skills, no natural HP/SP regen
        return { canAttack: false, canUseSkills: false, naturalRegen: false, moveSpeedMult: 0.6 };
    }
    if (ratio >= 0.50) {
        // 50-89% weight: no natural HP regen (SP regen still works)
        return { canAttack: true, canUseSkills: true, naturalRegen: false, moveSpeedMult: 1.0 };
    }
    // Under 50%: no penalty
    return { canAttack: true, canUseSkills: true, naturalRegen: true, moveSpeedMult: 1.0 };
}
```

**Weight check before adding items:**

```javascript
// Add to addItemToInventory — reject if would exceed max weight
async function addItemToInventoryWeightChecked(characterId, itemId, quantity, player) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) return null;

    const addedWeight = (itemDef.weight || 0) * quantity;
    const maxWeight = getPlayerMaxWeight(player);

    // Query current weight
    const weightRes = await pool.query(
        `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as total_weight
         FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
         WHERE ci.character_id = $1`,
        [characterId]
    );
    const currentWeight = parseInt(weightRes.rows[0].total_weight);

    if (currentWeight + addedWeight > maxWeight) {
        return { error: 'overweight', currentWeight, maxWeight };
    }

    return addItemToInventory(characterId, itemId, quantity);
}
```

### 2.4 Socket Events -- Inventory Operations

All existing socket events in `server/src/index.js`:

#### `inventory:load` -- Client requests full inventory refresh

```javascript
// Existing (line ~3051)
socket.on('inventory:load', async () => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    const inventory = await getPlayerInventory(characterId);
    socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
});
```

#### `inventory:use` -- Use a consumable item

Already fully implemented (lines 4855-5043). Handles:
- HP/SP restoration potions (Red/Orange/Yellow/Blue Potion)
- Fly Wing (1029) -- random teleport, checks `noteleport` flag
- Butterfly Wing (1028) -- save point teleport, checks `noreturn` flag
- Consumes 1 from stack, refreshes inventory

#### `inventory:equip` -- Equip or unequip an item

Already fully implemented (lines 5046-5258). Handles:
- Level requirement validation
- Dual-accessory slot management (accessory_1 / accessory_2)
- Auto-unequip of existing item in same slot
- Stat bonus add/remove pipeline (`equipmentBonuses` object)
- Weapon ATK/range/ASPD modifier tracking
- Derived stat recalculation (maxHP, maxSP, hit, flee, critical)
- Broadcasts `combat:health_update` to zone for HP bar sync

#### `inventory:drop` -- Discard/drop an item

Already fully implemented (lines 5261-5302). Handles:
- Ownership verification
- Partial quantity drop (stackables)
- Full removal via `removeItemFromInventory`

#### `inventory:move` -- Reorder inventory grid

Already fully implemented (lines 5305-5357). Handles:
- Swap logic: if target slot occupied, swap with source slot
- Persists `slot_index` to DB

#### `inventory:data` -- Server-to-client payload

Emitted after every mutation. Payload structure:

```javascript
{
    items: [
        {
            inventory_id: 42,
            item_id: 1001,
            quantity: 5,
            is_equipped: false,
            slot_index: 0,
            equipped_position: null,
            refine_level: 0,         // NEW
            card_slot_0: null,       // NEW
            card_slot_1: null,       // NEW
            card_slot_2: null,       // NEW
            card_slot_3: null,       // NEW
            name: "Crimson Vial",
            description: "A small red tonic. Restores 50 HP.",
            item_type: "consumable",
            equip_slot: null,
            weight: 7,
            price: 25,
            atk: 0, def: 0, matk: 0, mdef: 0,
            str_bonus: 0, agi_bonus: 0, vit_bonus: 0,
            int_bonus: 0, dex_bonus: 0, luk_bonus: 0,
            max_hp_bonus: 0, max_sp_bonus: 0,
            hit_bonus: 0, flee_bonus: 0, critical_bonus: 0,
            required_level: 1,
            stackable: true,
            icon: "red_potion",
            weapon_type: null,
            aspd_modifier: 0,
            weapon_range: 0,
            weapon_level: 0,         // NEW
            slots: 0,                // NEW
            class_restrictions: null  // NEW
        }
        // ... more items
    ],
    zuzucoin: 12500
}
```

### 2.5 New: Stack Split Operation

```javascript
// NEW socket event
socket.on('inventory:split', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;

    const { characterId, player } = playerInfo;
    const inventoryId = parseInt(data.inventoryId);
    const splitQuantity = parseInt(data.quantity);

    if (!inventoryId || !splitQuantity || splitQuantity < 1) {
        socket.emit('inventory:error', { message: 'Invalid split parameters' });
        return;
    }

    try {
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, i.stackable
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
            [inventoryId, characterId]
        );

        if (result.rows.length === 0) {
            socket.emit('inventory:error', { message: 'Item not found' });
            return;
        }

        const item = result.rows[0];
        if (!item.stackable || item.quantity <= splitQuantity) {
            socket.emit('inventory:error', { message: 'Cannot split this item' });
            return;
        }

        const remainingQty = item.quantity - splitQuantity;

        // Reduce original stack
        await pool.query(
            'UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2',
            [remainingQty, inventoryId]
        );

        // Find next slot index
        const maxSlotResult = await pool.query(
            'SELECT COALESCE(MAX(slot_index), -1) as max_slot FROM character_inventory WHERE character_id = $1',
            [characterId]
        );
        const nextSlot = maxSlotResult.rows[0].max_slot + 1;

        // Create new stack
        await pool.query(
            'INSERT INTO character_inventory (character_id, item_id, quantity, slot_index) VALUES ($1, $2, $3, $4)',
            [characterId, item.item_id, splitQuantity, nextSlot]
        );

        logger.info(`[ITEMS] ${player.characterName} split ${item.item_id}: ${remainingQty} + ${splitQuantity}`);

        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
    } catch (err) {
        logger.error(`[ITEMS] Split error: ${err.message}`);
        socket.emit('inventory:error', { message: 'Failed to split item' });
    }
});
```

---

## 3. Server-Side Equipment System

### 3.1 Equipment Slots (10 Total)

| Slot Position | DB Value | RO Classic Equivalent |
|---------------|----------|----------------------|
| Head Top | `head_top` | Upper Headgear |
| Head Mid | `head_mid` | Middle Headgear |
| Head Low | `head_low` | Lower Headgear |
| Armor | `armor` | Body Armor |
| Weapon | `weapon` | Right Hand |
| Shield | `shield` | Left Hand |
| Garment | `garment` | Cape/Manteau |
| Footgear | `footgear` | Shoes/Boots |
| Accessory 1 | `accessory_1` | Accessory (left) |
| Accessory 2 | `accessory_2` | Accessory (right) |

### 3.2 Equip Validation Pipeline

The existing `inventory:equip` handler (lines 5046-5258) validates:

1. **Ownership**: Item must belong to the character (`character_id` match)
2. **Equippable**: Item must have a non-null `equip_slot`
3. **Level requirement**: `player.level >= item.required_level`

**New validations to add:**

```javascript
// Class restriction check (add to inventory:equip handler)
function canClassEquip(playerClass, item) {
    if (!item.class_restrictions) return true; // null = all classes
    try {
        const allowed = JSON.parse(item.class_restrictions);
        return allowed.includes(playerClass);
    } catch {
        return true;
    }
}

// Two-hand weapon check: unequip shield when equipping two-hand weapon
function isTwoHandWeapon(weaponType) {
    const twoHand = ['two_hand_sword', 'two_hand_axe', 'two_hand_spear', 'bow', 'katar', 'instrument', 'whip'];
    return twoHand.includes(weaponType);
}
```

**Enhanced equip handler** (additions to existing code):

```javascript
// Add after level requirement check (line ~5080)
if (equip) {
    // Class restriction
    if (!canClassEquip(player.stats.class || 'novice', item)) {
        socket.emit('inventory:error', { message: `Your class cannot equip this item` });
        return;
    }

    // Two-hand weapon: auto-unequip shield
    if (item.equip_slot === 'weapon' && isTwoHandWeapon(item.weapon_type)) {
        const shieldCheck = await pool.query(
            `SELECT ci.inventory_id, i.def, i.str_bonus, i.agi_bonus, i.vit_bonus,
                    i.int_bonus, i.dex_bonus, i.luk_bonus, i.max_hp_bonus, i.max_sp_bonus,
                    i.hit_bonus, i.flee_bonus, i.critical_bonus
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'shield'`,
            [characterId]
        );
        for (const oldShield of shieldCheck.rows) {
            removeOldBonuses(oldShield);
            await pool.query(
                'UPDATE character_inventory SET is_equipped = false, equipped_position = NULL WHERE inventory_id = $1',
                [oldShield.inventory_id]
            );
        }
    }

    // Shield equip: block if two-hand weapon is equipped
    if (item.equip_slot === 'shield') {
        const weaponCheck = await pool.query(
            `SELECT i.weapon_type FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'`,
            [characterId]
        );
        if (weaponCheck.rows.length > 0 && isTwoHandWeapon(weaponCheck.rows[0].weapon_type)) {
            socket.emit('inventory:error', { message: 'Cannot equip shield while using a two-handed weapon' });
            return;
        }
    }
}
```

### 3.3 Stat Recalculation on Equip/Unequip

The existing system tracks equipment bonuses in `player.equipmentBonuses`:

```javascript
// Existing structure (line ~5086)
player.equipmentBonuses = {
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0
};
player.hardDef = 0; // Aggregated DEF from all armor
```

**Stat aggregation pipeline** (current + future):

```
Final Stat = Base Stat + Equipment Bonus + Refinement Bonus + Card Bonus + Buff Bonus

Where:
  Base Stat        = character.str (from stat points allocation)
  Equipment Bonus  = SUM(all equipped items' str_bonus)
  Refinement Bonus = Per-item refine bonus (see Section 4)
  Card Bonus       = SUM(all card effects on equipped items) (see Section 5)
  Buff Bonus       = Active skill buffs (Blessing, Increase AGI, etc.)
```

**Enhanced `getEffectiveStats` with refine + card bonuses:**

```javascript
function getEffectiveStats(player) {
    const base = player.stats || {};
    const eq = player.equipmentBonuses || {};
    const refine = player.refineBonuses || { atk: 0, matk: 0, def: 0, mdef: 0 };
    const card = player.cardBonuses || { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
        maxHp: 0, maxSp: 0, atk: 0, hit: 0, flee: 0, critical: 0 };

    return {
        level: base.level || 1,
        jobLevel: base.jobLevel || 1,
        class: base.class || 'novice',
        str: (base.str || 1) + (eq.str || 0) + (card.str || 0),
        agi: (base.agi || 1) + (eq.agi || 0) + (card.agi || 0),
        vit: (base.vit || 1) + (eq.vit || 0) + (card.vit || 0),
        int: (base.int_stat || 1) + (eq.int || 0) + (card.int || 0),
        dex: (base.dex || 1) + (eq.dex || 0) + (card.dex || 0),
        luk: (base.luk || 1) + (eq.luk || 0) + (card.luk || 0),
        weaponATK: (base.weaponATK || 0) + (refine.atk || 0) + (card.atk || 0),
        hardDef: (player.hardDef || 0) + (refine.def || 0),
    };
}
```

### 3.4 Broadcast After Equipment Change

The existing handler already broadcasts `combat:health_update` and `player:stats`. Add equipment visual broadcast:

```javascript
// NEW: Broadcast equipment change to other players in zone for visual updates
broadcastToZoneExcept(equipZone, socket.id, 'player:equipment_changed', {
    characterId,
    slot: equippedPosition || item.equip_slot,
    itemId: equip ? item.item_id : null,
    weaponType: equip ? item.weapon_type : null,
    refineLevel: 0 // Will be populated after refine system
});
```

---

## 4. Server-Side Refinement/Upgrade

### 4.1 RO Classic Refine Rules

In RO Classic, equipment can be refined (upgraded) at a Refine NPC. Each +1 adds ATK (weapons) or DEF (armor). Refinement can fail, destroying the item.

**Weapon Level determines materials and success rates:**

| Weapon Level | Material | Zeny Cost | Safe Limit |
|-------------|----------|-----------|------------|
| 1 | Phracon (2042) | 50z | +7 |
| 2 | Emveretarcon | 200z | +6 |
| 3 | Oridecon | 1000z | +5 |
| 4 | Oridecon | 2000z | +4 |
| Armor | Elunium | 2000z | +4 |

"Safe limit" means refine levels at or below this value have 100% success rate.

### 4.2 Success Rate Tables

```javascript
// RO Classic refine success rates (per weapon level, per refine target)
const REFINE_RATES = {
    // weapon_level: { targetRefine: successRate }
    1: { 1: 100, 2: 100, 3: 100, 4: 100, 5: 100, 6: 100, 7: 100,
         8: 60, 9: 40, 10: 19 },
    2: { 1: 100, 2: 100, 3: 100, 4: 100, 5: 100, 6: 100,
         7: 60, 8: 40, 9: 20, 10: 19 },
    3: { 1: 100, 2: 100, 3: 100, 4: 100, 5: 100,
         6: 60, 7: 40, 8: 20, 9: 20, 10: 19 },
    4: { 1: 100, 2: 100, 3: 100, 4: 100,
         5: 60, 6: 40, 7: 20, 8: 20, 9: 10, 10: 10 },
    armor: { 1: 100, 2: 100, 3: 100, 4: 100,
             5: 60, 6: 40, 7: 40, 8: 20, 9: 20, 10: 10 }
};

// Material item IDs
const REFINE_MATERIALS = {
    1: 2042,  // Phracon
    2: 6001,  // Emveretarcon (add to items table)
    3: 6002,  // Oridecon (refined, add to items table)
    4: 6002,  // Oridecon
    armor: 6003 // Elunium (add to items table)
};

// Zeny cost per refine attempt
const REFINE_COST = {
    1: 50,
    2: 200,
    3: 1000,
    4: 2000,
    armor: 2000
};
```

### 4.3 Refine Bonus Calculation

```javascript
// ATK bonus per refine level (weapon)
function getRefineAtkBonus(weaponLevel, refineLevel) {
    if (refineLevel <= 0) return 0;
    // Base per-refine ATK
    const perRefine = { 1: 2, 2: 3, 3: 5, 4: 7 };
    const base = (perRefine[weaponLevel] || 2) * refineLevel;

    // Over-upgrade bonus: for each level above safe limit
    const safeLimit = { 1: 7, 2: 6, 3: 5, 4: 4 };
    const safe = safeLimit[weaponLevel] || 7;
    let overBonus = 0;
    if (refineLevel > safe) {
        const overLevels = refineLevel - safe;
        // Over-refine bonus scales with weapon level
        const overScale = { 1: 3, 2: 5, 3: 8, 4: 14 };
        overBonus = overLevels * (overScale[weaponLevel] || 3);
    }

    return base + overBonus;
}

// DEF bonus per refine level (armor)
function getRefineDefBonus(refineLevel) {
    // Armor: +1 hard DEF per refine level (no over-upgrade bonus in classic)
    return refineLevel;
}

// MATK bonus per refine level (staff weapons)
function getRefineMAtkBonus(weaponLevel, refineLevel) {
    // Staves get MATK bonus: +1 MATK per refine for wLv1, +2 for wLv2, etc.
    return refineLevel * weaponLevel;
}
```

### 4.4 Refine Socket Event

```javascript
// NEW socket event: refine:request
socket.on('refine:request', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;

    const { characterId, player } = playerInfo;
    const inventoryId = parseInt(data.inventoryId);

    if (!inventoryId) {
        socket.emit('refine:result', { success: false, message: 'Invalid item' });
        return;
    }

    try {
        // 1. Load item with full data
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.refine_level, ci.is_equipped,
                    i.item_type, i.equip_slot, i.weapon_level, i.name, i.weapon_type,
                    i.atk, i.def, i.matk
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
            [inventoryId, characterId]
        );

        if (result.rows.length === 0) {
            socket.emit('refine:result', { success: false, message: 'Item not found' });
            return;
        }

        const item = result.rows[0];
        const currentRefine = item.refine_level || 0;
        const targetRefine = currentRefine + 1;

        // 2. Validate refinability
        if (targetRefine > 10) {
            socket.emit('refine:result', { success: false, message: 'Item is already at maximum refine level (+10)' });
            return;
        }

        if (!item.equip_slot) {
            socket.emit('refine:result', { success: false, message: 'This item cannot be refined' });
            return;
        }

        // 3. Determine category (weapon level or armor)
        const isWeapon = item.equip_slot === 'weapon';
        const category = isWeapon ? (item.weapon_level || 1) : 'armor';

        // 4. Check material
        const materialId = REFINE_MATERIALS[category];
        const materialCheck = await pool.query(
            `SELECT ci.inventory_id, ci.quantity
             FROM character_inventory ci
             WHERE ci.character_id = $1 AND ci.item_id = $2 AND ci.is_equipped = false AND ci.quantity > 0`,
            [characterId, materialId]
        );

        if (materialCheck.rows.length === 0) {
            const materialName = itemDefinitions.get(materialId)?.name || 'required material';
            socket.emit('refine:result', { success: false, message: `You need ${materialName} to refine this item` });
            return;
        }

        // 5. Check zeny
        const cost = REFINE_COST[category] || 2000;
        if (player.zuzucoin < cost) {
            socket.emit('refine:result', { success: false, message: `You need ${cost} zeny to refine` });
            return;
        }

        // 6. Consume material and zeny
        await removeItemFromInventory(materialCheck.rows[0].inventory_id, 1);
        player.zuzucoin -= cost;
        await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2',
            [player.zuzucoin, characterId]);

        // 7. Roll success
        const rates = REFINE_RATES[category] || REFINE_RATES[1];
        const successRate = rates[targetRefine] || 10;
        const roll = Math.random() * 100;
        const success = roll < successRate;

        if (success) {
            // Increase refine level
            await pool.query(
                'UPDATE character_inventory SET refine_level = $1 WHERE inventory_id = $2',
                [targetRefine, inventoryId]
            );

            // Recalculate refine bonuses if item is equipped
            if (item.is_equipped) {
                recalculateRefineBonuses(characterId, player);
            }

            logger.info(`[REFINE] ${player.characterName} refined ${item.name} to +${targetRefine} (rate: ${successRate}%, roll: ${roll.toFixed(1)})`);

            socket.emit('refine:result', {
                success: true,
                inventoryId,
                itemName: item.name,
                newRefineLevel: targetRefine,
                successRate,
                message: `Successfully refined ${item.name} to +${targetRefine}!`
            });
        } else {
            // FAILURE: Item is destroyed
            const wasEquipped = item.is_equipped;
            if (wasEquipped) {
                // Remove stat bonuses before destroying
                const oldItem = await pool.query(
                    `SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus,
                            i.dex_bonus, i.luk_bonus, i.max_hp_bonus, i.max_sp_bonus,
                            i.hit_bonus, i.flee_bonus, i.critical_bonus, i.atk, i.equip_slot,
                            i.aspd_modifier, i.weapon_range
                     FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                     WHERE ci.inventory_id = $1`,
                    [inventoryId]
                );
                if (oldItem.rows.length > 0) {
                    removeOldBonuses(oldItem.rows[0]);
                    if (oldItem.rows[0].equip_slot === 'weapon') {
                        player.stats.weaponATK = 0;
                        player.attackRange = COMBAT.MELEE_RANGE;
                        player.weaponAspdMod = 0;
                    }
                }
                recalculateRefineBonuses(characterId, player);
            }

            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);

            logger.info(`[REFINE] ${player.characterName} FAILED to refine ${item.name} +${currentRefine} -> +${targetRefine} (rate: ${successRate}%, roll: ${roll.toFixed(1)}) — ITEM DESTROYED`);

            socket.emit('refine:result', {
                success: false,
                inventoryId,
                itemName: item.name,
                itemDestroyed: true,
                successRate,
                message: `Refinement failed! ${item.name} has been destroyed.`
            });
        }

        // 8. Refresh inventory
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

        // 9. If item was equipped, re-broadcast stats
        if (item.is_equipped) {
            const effectiveStats = getEffectiveStats(player);
            const derived = calculateDerivedStats(effectiveStats);
            player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            player.maxHealth = derived.maxHP;
            player.maxMana = derived.maxSP;
            socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, player.aspd));

            const zone = player.zone || 'prontera_south';
            broadcastToZone(zone, 'combat:health_update', {
                characterId, health: player.health, maxHealth: player.maxHealth,
                mana: player.mana, maxMana: player.maxMana
            });
        }

    } catch (err) {
        logger.error(`[REFINE] Error: ${err.message}`);
        socket.emit('refine:result', { success: false, message: 'Refinement failed due to server error' });
    }
});

// Helper: recalculate all refine bonuses from equipped items
async function recalculateRefineBonuses(characterId, player) {
    if (!player.refineBonuses) player.refineBonuses = { atk: 0, matk: 0, def: 0, mdef: 0 };

    player.refineBonuses.atk = 0;
    player.refineBonuses.matk = 0;
    player.refineBonuses.def = 0;

    try {
        const equipped = await pool.query(
            `SELECT ci.refine_level, i.equip_slot, i.weapon_level, i.weapon_type
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.refine_level > 0`,
            [characterId]
        );

        for (const eq of equipped.rows) {
            if (eq.equip_slot === 'weapon') {
                player.refineBonuses.atk += getRefineAtkBonus(eq.weapon_level || 1, eq.refine_level);
                if (eq.weapon_type === 'staff' || eq.weapon_type === 'rod') {
                    player.refineBonuses.matk += getRefineMAtkBonus(eq.weapon_level || 1, eq.refine_level);
                }
            } else {
                player.refineBonuses.def += getRefineDefBonus(eq.refine_level);
            }
        }
    } catch (err) {
        logger.error(`[REFINE] Failed to recalculate refine bonuses for char ${characterId}: ${err.message}`);
    }
}
```

---

## 5. Server-Side Card System

### 5.1 Card Compound Rules

In RO Classic, cards can be inserted into equipment that has open card slots (`slots > 0`). Each card provides specific bonuses depending on the card and the equipment slot it is placed in.

**Rules:**
- Equipment must have at least 1 open slot (`items.slots > 0`)
- Card slot must be empty (`character_inventory.card_slot_N IS NULL`)
- One card per slot (cannot overwrite)
- Card is consumed on compound (removed from inventory)
- Equipment type must match card's compound type (weapon cards go in weapons, etc.)

### 5.2 Card Bonus Registry

```javascript
// Card effects registry — maps card item_id to bonus effects
// Each card can have multiple effects
const CARD_EFFECTS = new Map([
    // ---- Weapon Cards (compound on weapons) ----
    [5010, { // Hornet Card
        compoundOn: ['weapon'],
        effects: [
            { type: 'flat_stat', stat: 'str', value: 1 },
            { type: 'flat_atk', value: 3 }
        ]
    }],
    [5011, { // Rocker Card
        compoundOn: ['weapon'],
        effects: [
            { type: 'flat_stat', stat: 'dex', value: 1 },
            { type: 'flat_atk', value: 5 }
        ]
    }],
    [5012, { // Familiar Card
        compoundOn: ['weapon'],
        effects: [
            { type: 'hp_drain_percent', value: 5 } // 5% HP drain on attack
        ]
    }],
    [5013, { // Savage Babe Card
        compoundOn: ['weapon'],
        effects: [
            { type: 'chance_stun', value: 5 } // 5% chance to stun
        ]
    }],
    [5016, { // Skeleton Card
        compoundOn: ['weapon'],
        effects: [
            { type: 'flat_atk', value: 10 },
            { type: 'flat_stat', stat: 'hit', value: 5 }
        ]
    }],

    // ---- Armor Cards (compound on armor) ----
    [5003, { // Fabre Card
        compoundOn: ['armor'],
        effects: [
            { type: 'flat_stat', stat: 'vit', value: 1 },
            { type: 'flat_maxhp', value: 100 }
        ]
    }],
    [5004, { // Pupa Card
        compoundOn: ['armor'],
        effects: [
            { type: 'flat_maxhp', value: 700 }
        ]
    }],
    [5014, { // Spore Card
        compoundOn: ['armor'],
        effects: [
            { type: 'flat_stat', stat: 'vit', value: 2 }
        ]
    }],
    [5019, { // Pecopeco Card
        compoundOn: ['armor'],
        effects: [
            { type: 'percent_maxhp', value: 10 } // +10% Max HP
        ]
    }],

    // ---- Garment Cards ----
    [5007, { // Condor Card
        compoundOn: ['garment'],
        effects: [
            { type: 'flat_stat', stat: 'flee', value: 5 }
        ]
    }],

    // ---- Footgear Cards ----
    [5015, { // Zombie Card
        compoundOn: ['footgear'],
        effects: [
            { type: 'hp_regen_percent', value: 20 } // +20% HP recovery
        ]
    }],

    // ---- Shield Cards ----
    // (Add as more cards are introduced)

    // ---- Headgear Cards ----
    [5001, { // Poring Card
        compoundOn: ['head_top', 'head_mid', 'head_low'],
        effects: [
            { type: 'flat_stat', stat: 'luk', value: 2 },
            { type: 'flat_stat', stat: 'perfect_dodge', value: 1 }
        ]
    }],
    [5002, { // Lunatic Card
        compoundOn: ['head_top', 'head_mid', 'head_low'],
        effects: [
            { type: 'flat_stat', stat: 'luk', value: 1 },
            { type: 'flat_stat', stat: 'critical', value: 1 },
            { type: 'flat_stat', stat: 'flee', value: 1 }
        ]
    }],
    [5005, { // Drops Card
        compoundOn: ['head_top', 'head_mid', 'head_low'],
        effects: [
            { type: 'flat_stat', stat: 'dex', value: 1 },
            { type: 'flat_stat', stat: 'hit', value: 3 }
        ]
    }],
    [5006, { // Chonchon Card
        compoundOn: ['head_top', 'head_mid', 'head_low'],
        effects: [
            { type: 'flat_stat', stat: 'agi', value: 1 },
            { type: 'flat_stat', stat: 'flee', value: 2 }
        ]
    }],

    // ---- Accessory Cards ----
    [5008, { // Willow Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'flat_maxsp', value: 80 }
        ]
    }],
    [5009, { // Roda Frog Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'flat_maxhp', value: 400 },
            { type: 'flat_maxsp', value: 50 }
        ]
    }],
    [5020, { // Mandragora Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'flat_stat', stat: 'int', value: 1 },
            { type: 'flat_maxsp', value: 50 }
        ]
    }],
    [5023, { // Yoyo Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'flat_stat', stat: 'agi', value: 1 },
            { type: 'flat_stat', stat: 'perfect_dodge', value: 5 }
        ]
    }],

    // ---- Skill-granting Cards ----
    [5017, { // Creamy Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'grant_skill', skillId: 26, skillLevel: 1 } // Teleport Lv1
        ]
    }],
    [5018, { // Poporing Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'grant_skill', skillId: 52, skillLevel: 1 } // Detoxify Lv1
        ]
    }],
    [5022, { // Smokie Card
        compoundOn: ['accessory'],
        effects: [
            { type: 'grant_skill', skillId: 51, skillLevel: 1 } // Hiding Lv1
        ]
    }],
]);
```

### 5.3 Card Compound Socket Event

```javascript
// NEW socket event: card:compound
socket.on('card:compound', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;

    const { characterId, player } = playerInfo;
    const cardInventoryId = parseInt(data.cardInventoryId);    // The card to insert
    const equipInventoryId = parseInt(data.equipInventoryId);  // The equipment receiving the card
    const slotIndex = parseInt(data.slotIndex);                // Which slot (0-3)

    if (!cardInventoryId || !equipInventoryId || isNaN(slotIndex) || slotIndex < 0 || slotIndex > 3) {
        socket.emit('card:result', { success: false, message: 'Invalid compound parameters' });
        return;
    }

    try {
        // 1. Load the card
        const cardResult = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, i.item_type, i.name
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
            [cardInventoryId, characterId]
        );

        if (cardResult.rows.length === 0) {
            socket.emit('card:result', { success: false, message: 'Card not found' });
            return;
        }

        const card = cardResult.rows[0];
        if (card.item_type !== 'card') {
            socket.emit('card:result', { success: false, message: 'This is not a card' });
            return;
        }

        // 2. Load the target equipment
        const equipResult = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.is_equipped,
                    ci.card_slot_0, ci.card_slot_1, ci.card_slot_2, ci.card_slot_3,
                    i.equip_slot, i.slots, i.name
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
            [equipInventoryId, characterId]
        );

        if (equipResult.rows.length === 0) {
            socket.emit('card:result', { success: false, message: 'Equipment not found' });
            return;
        }

        const equip = equipResult.rows[0];

        // 3. Validate slot count
        if (!equip.slots || equip.slots <= 0) {
            socket.emit('card:result', { success: false, message: 'This equipment has no card slots' });
            return;
        }

        if (slotIndex >= equip.slots) {
            socket.emit('card:result', { success: false, message: `This equipment only has ${equip.slots} card slot(s)` });
            return;
        }

        // 4. Check slot is empty
        const slotColumn = `card_slot_${slotIndex}`;
        const currentCardInSlot = equip[slotColumn];
        if (currentCardInSlot !== null && currentCardInSlot !== undefined) {
            socket.emit('card:result', { success: false, message: 'This card slot is already occupied' });
            return;
        }

        // 5. Validate card compound type
        const cardEffect = CARD_EFFECTS.get(card.item_id);
        if (cardEffect && cardEffect.compoundOn) {
            const equipSlot = equip.equip_slot;
            // Normalize: head_top/mid/low all count as headgear
            const normalizedSlot = ['head_top', 'head_mid', 'head_low'].includes(equipSlot) ? equipSlot : equipSlot;
            if (!cardEffect.compoundOn.includes(normalizedSlot) && !cardEffect.compoundOn.includes(equipSlot)) {
                socket.emit('card:result', {
                    success: false,
                    message: `${card.name} cannot be compounded on ${equip.name} (wrong equipment type)`
                });
                return;
            }
        }

        // 6. Insert card: update the equipment slot, consume the card
        await pool.query(
            `UPDATE character_inventory SET ${slotColumn} = $1 WHERE inventory_id = $2`,
            [card.item_id, equipInventoryId]
        );

        await removeItemFromInventory(cardInventoryId, 1);

        // 7. If equipment is currently equipped, recalculate card bonuses
        if (equip.is_equipped) {
            await recalculateCardBonuses(characterId, player);

            const effectiveStats = getEffectiveStats(player);
            const derived = calculateDerivedStats(effectiveStats);
            player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            player.maxHealth = derived.maxHP;
            player.maxMana = derived.maxSP;
            socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, player.aspd));
        }

        logger.info(`[CARD] ${player.characterName} compounded ${card.name} into ${equip.name} slot ${slotIndex}`);

        socket.emit('card:result', {
            success: true,
            cardName: card.name,
            equipmentName: equip.name,
            slotIndex,
            message: `${card.name} has been compounded into ${equip.name}!`
        });

        // Refresh inventory
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

    } catch (err) {
        logger.error(`[CARD] Compound error: ${err.message}`);
        socket.emit('card:result', { success: false, message: 'Card compound failed' });
    }
});
```

### 5.4 Card Bonus Recalculation

```javascript
// Recalculate all card bonuses from equipped items
async function recalculateCardBonuses(characterId, player) {
    if (!player.cardBonuses) {
        player.cardBonuses = {
            str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
            maxHp: 0, maxSp: 0, atk: 0, hit: 0, flee: 0, critical: 0,
            perfectDodge: 0
        };
    }

    // Reset all card bonuses
    Object.keys(player.cardBonuses).forEach(k => player.cardBonuses[k] = 0);

    try {
        const equipped = await pool.query(
            `SELECT ci.card_slot_0, ci.card_slot_1, ci.card_slot_2, ci.card_slot_3
             FROM character_inventory ci
             WHERE ci.character_id = $1 AND ci.is_equipped = true`,
            [characterId]
        );

        for (const eq of equipped.rows) {
            for (let s = 0; s < 4; s++) {
                const cardId = eq[`card_slot_${s}`];
                if (!cardId) continue;

                const cardEffect = CARD_EFFECTS.get(cardId);
                if (!cardEffect) continue;

                for (const effect of cardEffect.effects) {
                    switch (effect.type) {
                        case 'flat_stat':
                            if (player.cardBonuses[effect.stat] !== undefined) {
                                player.cardBonuses[effect.stat] += effect.value;
                            }
                            break;
                        case 'flat_atk':
                            player.cardBonuses.atk += effect.value;
                            break;
                        case 'flat_maxhp':
                            player.cardBonuses.maxHp += effect.value;
                            break;
                        case 'flat_maxsp':
                            player.cardBonuses.maxSp += effect.value;
                            break;
                        case 'percent_maxhp':
                            // Percent bonuses applied after flat bonuses in getEffectiveStats
                            if (!player.cardBonuses.percentMaxHp) player.cardBonuses.percentMaxHp = 0;
                            player.cardBonuses.percentMaxHp += effect.value;
                            break;
                        // hp_drain_percent, chance_stun, grant_skill handled in combat tick
                    }
                }
            }
        }
    } catch (err) {
        logger.error(`[CARD] Failed to recalculate card bonuses for char ${characterId}: ${err.message}`);
    }
}
