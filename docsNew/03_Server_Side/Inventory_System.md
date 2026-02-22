# Inventory System — Server-Side Documentation

## Overview

The inventory system manages items, equipment, and consumables for all characters. Item definitions are stored in PostgreSQL and cached in memory on server startup. Per-character inventory uses the `character_inventory` table with support for stacking, equipping, and dropping items.

## Constants

```javascript
const INVENTORY = {
    MAX_SLOTS: 100,    // Max inventory slots per character (future enforcement)
    MAX_WEIGHT: 2000   // Max carry weight (future use)
};
```

## Item Definition Cache

```javascript
const itemDefinitions = new Map();  // Map<itemId, DB row>

async function loadItemDefinitions() {
    const result = await pool.query('SELECT * FROM items');
    for (const row of result.rows) {
        itemDefinitions.set(row.item_id, row);
    }
}
```

Loaded once on server startup after seeding base items. Used for drop name lookups and stackability checks.

## Item Categories

### Consumables (1001–1005)

| ID | Name | Effect | Price | Stack |
|----|------|--------|-------|-------|
| 1001 | Crimson Vial | +50 HP | 25 | 99 |
| 1002 | Amber Elixir | +150 HP | 100 | 99 |
| 1003 | Golden Salve | +350 HP | 275 | 99 |
| 1004 | Azure Philter | +60 SP | 500 | 99 |
| 1005 | Roasted Haunch | +70 HP | 25 | 99 |

HP restoration values are hardcoded in the `inventory:use` handler:
```javascript
const hpRestore = { 1001: 50, 1002: 150, 1003: 350, 1005: 70 };
```

### Loot/Etc Items (2001–2008)

Dropped by enemies, sold to NPCs. All stackable (max 999, except Verdant Leaf at 99).

### Weapons (3001–3006)

| ID | Name | Type | ATK | Range | ASPD Mod | Req Lvl |
|----|------|------|-----|-------|----------|---------|
| 3001 | Rustic Shiv | dagger | 17 | 150 | +5 | 1 |
| 3002 | Keen Edge | dagger | 30 | 150 | +5 | 1 |
| 3003 | Stiletto Fang | dagger | 43 | 150 | +5 | 12 |
| 3004 | Iron Cleaver | 1h_sword | 25 | 150 | 0 | 2 |
| 3005 | Crescent Saber | 1h_sword | 49 | 150 | 0 | 18 |
| 3006 | Hunting Longbow | bow | 35 | 800 | -3 | 4 |

### Armor (4001–4003)

| ID | Name | DEF | Req Lvl |
|----|------|-----|---------|
| 4001 | Linen Tunic | 1 | 1 |
| 4002 | Quilted Vest | 4 | 1 |
| 4003 | Ringweave Hauberk | 8 | 20 |

## Core Functions

### addItemToInventory(characterId, itemId, quantity)

```javascript
async function addItemToInventory(characterId, itemId, quantity = 1) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) return null;
    
    if (itemDef.stackable) {
        // Check for existing unequipped stack
        const existing = await pool.query(
            'SELECT inventory_id, quantity FROM character_inventory WHERE character_id=$1 AND item_id=$2 AND is_equipped=false',
            [characterId, itemId]
        );
        if (existing.rows.length > 0) {
            // Stack: newQty = min(existing + added, max_stack)
            const newQty = Math.min(existing.rows[0].quantity + quantity, itemDef.max_stack);
            await pool.query('UPDATE character_inventory SET quantity=$1 WHERE inventory_id=$2', [newQty, ...]);
            return { inventoryId, itemId, quantity: newQty, isEquipped: false };
        }
    }
    
    // Insert new entry
    const result = await pool.query(
        'INSERT INTO character_inventory (character_id, item_id, quantity) VALUES ($1,$2,$3) RETURNING inventory_id',
        [characterId, itemId, quantity]
    );
    return { inventoryId: result.rows[0].inventory_id, itemId, quantity, isEquipped: false };
}
```

### getPlayerInventory(characterId)

Joins `character_inventory` with `items` to return full item details including all stat bonuses, weapon properties, and icons. Ordered by `slot_index ASC, created_at ASC`.

### removeItemFromInventory(inventoryId, quantity)

- If `quantity` is provided: reduces stack, deletes if result ≤ 0
- If `quantity` is null: deletes entire entry

## Socket.io Event Handlers

### inventory:load

```
Client: emit('inventory:load')
Server:
    1. getPlayerInventory(characterId) → emit('inventory:data', {items})
    2. getPlayerHotbar(characterId)   → emit('hotbar:data', {slots})
```

Both events are sent together. The client uses `inventory:data` to populate the inventory UI (if open) and sync hotbar quantities, and `hotbar:data` to restore hotbar slot assignments on login/reconnect.

### inventory:use

```
Client: emit('inventory:use', {inventoryId})
Server:
    1. Verify ownership (character_id match)
    2. Check item_type === 'consumable'
    3. Apply HP/SP restoration
    4. Remove 1 from stack
    5. Save health/mana to DB
    6. Broadcast combat:health_update to all
    7. Emit inventory:used to requester
    8. Refresh inventory (emit inventory:data)
```

Consumable effects:
```javascript
if (item.item_id === 1001) healed = 50;   // Crimson Vial
if (item.item_id === 1002) healed = 150;  // Amber Elixir
if (item.item_id === 1003) healed = 350;  // Golden Salve
if (item.item_id === 1004) spRestored = 60; // Azure Philter
if (item.item_id === 1005) healed = 70;   // Roasted Haunch
```

### inventory:equip

```
Client: emit('inventory:equip', {inventoryId, equip: true/false})
Server:
    1. Verify ownership
    2. Check item has equip_slot
    3. Check required_level
    4. If equipping:
       a. Unequip any item in same slot
       b. Set is_equipped = true
       c. If weapon: update player.stats.weaponATK, attackRange, weaponAspdMod
    5. If unequipping:
       a. Set is_equipped = false
       b. If weapon: reset weaponATK=0, attackRange=MELEE_RANGE, weaponAspdMod=0
    6. Recalculate derived stats (include weapon ASPD modifier)
    7. Emit player:stats, inventory:data, inventory:equipped
```

### inventory:drop

```
Client: emit('inventory:drop', {inventoryId, quantity?})
Server:
    1. Verify ownership
    2. Remove quantity (or all if quantity not specified)
    3. Emit inventory:dropped
    4. Refresh inventory (emit inventory:data)
```

### inventory:move

```
Client: emit('inventory:move', {sourceInventoryId, targetInventoryId})
Server:
    1. Verify both items belong to this character (single query with ANY)
    2. Normalize slot_index values if any are -1 (assign sequential indexes first)
    3. Swap slot_index values between source and target rows
    4. Emit inventory:data (full inventory refresh)
```

**Called by**: `AC_HUDManager.SendMoveItemRequest` (triggered by `WBP_InventorySlot.On Drop`)

**Slot normalization**: If any item has `slot_index = -1` (default for new items), the handler normalizes all items to sequential indexes before swapping. This prevents no-op swaps when both items share the same default value.

```javascript
// Guard: skip if same slot
if (!sourceInventoryId || !targetInventoryId || sourceInventoryId === targetInventoryId) {
    socket.emit('inventory:error', { message: 'Invalid move request' });
    return;
}
// Normalize → verify ownership → swap slot_index → emit inventory:data
```

## Equipment System

### Equip Slots

| Slot | Description | Currently Used |
|------|-------------|---------------|
| `weapon` | Main hand weapon | ✅ Yes (ATK, ASPD, range) |
| `armor` | Body armor | ✅ Yes (DEF + stat bonuses) |
| `shield` | Off-hand shield | ❌ No items yet |
| `head_top` | Head upper | ❌ No items yet |
| `head_mid` | Head middle | ❌ No items yet |
| `head_low` | Head lower | ❌ No items yet |
| `garment` | Cape/mantle | ❌ No items yet |
| `footgear` | Shoes/boots | ❌ No items yet |
| `accessory` | Ring/necklace | ❌ No items yet |

### Weapon Effects on Player State

When equipping a weapon, the server updates:
```javascript
player.stats.weaponATK = item.atk || 0;
player.attackRange = item.weapon_range || COMBAT.MELEE_RANGE;  // 150 or 800 for bow
player.weaponAspdMod = item.aspd_modifier || 0;  // +5 dagger, 0 sword, -3 bow
player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + player.weaponAspdMod);
```

When unequipping:
```javascript
player.stats.weaponATK = 0;
player.attackRange = COMBAT.MELEE_RANGE;  // 150
player.weaponAspdMod = 0;
```

### Armor Effects on Player State

When equipping armor, the server updates:
```javascript
player.hardDef += item.def || 0;                    // Add to total hard DEF
player.equipmentBonuses.str += item.str_bonus || 0; // Add stat bonuses
player.equipmentBonuses.agi += item.agi_bonus || 0;
player.equipmentBonuses.vit += item.vit_bonus || 0;
player.equipmentBonuses.int += item.int_bonus || 0;
player.equipmentBonuses.dex += item.dex_bonus || 0;
player.equipmentBonuses.luk += item.luk_bonus || 0;
// Also: maxHp, maxSp, hit, flee, critical bonuses
```

When unequipping armor, the server subtracts the same values.

### Equipment Loading on Join

On `player:join`, the server queries all equipped items:
```sql
-- Weapon
SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range
FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'

-- All equipment (armor, accessories)
SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, 
       i.dex_bonus, i.luk_bonus, i.max_hp_bonus, i.max_sp_bonus,
       i.hit_bonus, i.flee_bonus, i.critical_bonus
FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1 AND ci.is_equipped = true
```

## Loot System

When an enemy dies, the killer receives drops:

```javascript
const droppedItems = rollEnemyDrops(enemy);
if (droppedItems.length > 0) {
    for (const drop of droppedItems) {
        await addItemToInventory(attackerId, drop.itemId, drop.quantity);
    }
    killerSocket.emit('loot:drop', {
        enemyId, enemyName,
        items: lootItems.map(i => ({
            itemId, itemName, quantity, icon, itemType
        }))
    });
}
```

Loot goes directly to killer's inventory (no ground drops). The `loot:drop` event is sent only to the killer for UI display.

## Hotbar Persistence

### Overview

Hotbar slot assignments are stored server-side in the `character_hotbar` table. The FK to `character_inventory` uses `ON DELETE CASCADE` — when an inventory item is fully consumed or dropped, PostgreSQL automatically removes the hotbar row. No explicit server-side cleanup code is needed.

### getPlayerHotbar(characterId)

```javascript
async function getPlayerHotbar(characterId) {
    const result = await pool.query(
        `SELECT ch.slot_index, ch.inventory_id, ch.item_id, ch.item_name,
                ci.quantity
         FROM character_hotbar ch
         JOIN character_inventory ci ON ch.inventory_id = ci.inventory_id
         WHERE ch.character_id = $1
         ORDER BY ch.slot_index ASC`,
        [characterId]
    );
    return result.rows;
}
```

JOIN with `character_inventory` ensures `quantity` reflects the current stack count, not a stale cached value.

### hotbar:save

```
Client: emit('hotbar:save', {slotIndex, inventoryId, itemId, itemName})
Server:
    1. Validate slotIndex (0–8)
    2. If inventoryId <= 0: DELETE from character_hotbar (clear slot)
    3. Else: verify ownership → UPSERT character_hotbar row
```

Emitted from `WBP_HotbarSlot.OnDrop` via `AC_HUDManager.SendSaveHotbarSlotRequest` whenever the player drags an item to a hotbar slot.

### hotbar:data

```
Server: emit('hotbar:data', {slots: [{slot_index, inventory_id, item_id, item_name, quantity}]})
```

Sent automatically after every `inventory:load` response, and 0.6s after `player:join` (delayed to allow HUD initialization). The client's `BP_SocketManager.OnHotbarData` calls `AC_HUDManager.PopulateHotbarFromServer(Data)` to restore each slot.

### Slot Lifecycle

| Action | Effect on `character_hotbar` |
|--------|------------------------------|
| Drag item to hotbar | INSERT or UPDATE row (via `hotbar:save`) |
| Item stack depleted | Row auto-deleted (ON DELETE CASCADE FK) |
| Item dropped/sold | Row auto-deleted (ON DELETE CASCADE FK) |
| Player reconnects | Rows loaded and sent via `hotbar:data` |

## NPC Shop System

### Overview

The NPC Shop allows players to buy items with Zuzucoin and sell inventory items back to any shop for their base price. Shop inventories are defined server-side in `NPC_SHOPS` — no database table needed. Pricing follows the Ragnarok Online convention: **buy price = item.price × 2**, **sell price = item.price**.

### Zuzucoin Currency

- Stored in `characters.zuzucoin` (INTEGER, DEFAULT 0)
- Loaded from DB in `player:join`, stored in `connectedPlayers` as `player.zuzucoin`
- Persisted to DB immediately on every buy or sell transaction
- **Migration**: `database/migrations/rename_zeny_to_zuzucoin.sql`

### NPC_SHOPS Definition (server-side constant)

```javascript
const NPC_SHOPS = {
    1: {
        name: 'General Store',
        itemIds: [1001, 1002, 1003, 1004, 1005, 4001, 4002, 4003]
    },
    2: {
        name: 'Weapon Shop',
        itemIds: [3001, 3002, 3003, 3004, 3005, 3006]
    }
};
```

Items must already exist in the `itemDefinitions` cache (loaded from DB on startup). The `shopId` is passed by the client when clicking on an NPC actor.

### shop:open

```
Client: emit('shop:open', {shopId})
Server:
    1. Look up NPC_SHOPS[shopId]
    2. Map itemIds to full item definitions from itemDefinitions cache
    3. Emit 'shop:data' with {shopId, shopName, items[], playerZuzucoin}

shop:data payload:
{
    shopId: int,
    shopName: string,
    playerZuzucoin: int,
    items: [{
        itemId, name, description, itemType,
        buyPrice (price*2), sellPrice (price),
        icon, atk, def, weaponType, weaponRange,
        aspdModifier, requiredLevel, stackable
    }]
}
```

### shop:buy

```
Client: emit('shop:buy', {shopId, itemId, quantity})
Server:
    1. Validate shopId and itemId are in NPC_SHOPS[shopId].itemIds
    2. Check player level >= item.required_level
    3. totalCost = item.price * 2 * quantity
    4. Check player.zuzucoin >= totalCost
    5. UPDATE characters SET zuzucoin = newZuzucoin
    6. addItemToInventory(characterId, itemId, quantity)
    7. If item add fails: rollback zuzucoin UPDATE
    8. Emit 'shop:bought' {itemId, itemName, quantity, totalCost, newZuzucoin}
    9. Emit 'inventory:data' (full refresh)
```

### shop:sell

```
Client: emit('shop:sell', {inventoryId, quantity})
Server:
    1. Verify ownership (character_id match)
    2. Check item is NOT equipped (block sale of equipped items)
    3. sellQty = min(quantity, item.quantity)
    4. sellPrice = item.price * sellQty
    5. UPDATE characters SET zuzucoin = newZuzucoin
    6. removeItemFromInventory(inventoryId, sellQty)
    7. Emit 'shop:sold' {inventoryId, itemName, quantity, sellPrice, newZuzucoin}
    8. Emit 'inventory:data' (full refresh)
```

### Error Responses (shop:error)

| Error | When |
|-------|------|
| "Shop not found" | shopId not in NPC_SHOPS |
| "Item not available in this shop" | itemId not in shop.itemIds |
| "Item not found" | itemId not in itemDefinitions cache |
| `"Requires level X"` | player.stats.level < item.required_level |
| `"Not enough Zuzucoin (need X, have Y)"` | player.zeny < totalCost |
| "Cannot sell an equipped item. Unequip it first." | item.is_equipped = true |
| "Failed to add item to inventory" | addItemToInventory returned null |
| "Purchase failed" / "Sale failed" | Database error |

---

## Error Handling

All inventory operations return `inventory:error` on failure:

| Error | When |
|-------|------|
| "Invalid inventory ID" | inventoryId is NaN or 0 |
| "Item not found in your inventory" | Wrong character_id or nonexistent entry |
| "This item cannot be used" | item_type !== 'consumable' |
| "This item cannot be equipped" | No equip_slot defined |
| "Requires level X" | Player level < required_level |
| "Item not found" | inventoryId doesn't exist |
| "Failed to use/equip/drop item" | Database error |

---

**Last Updated**: 2026-02-21 — Fully renamed database column from zeny to zuzucoin
