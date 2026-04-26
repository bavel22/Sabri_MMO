# Kafra Storage & Player-to-Player Trading — Implementation Reference

> **STATUS: BOTH SYSTEMS COMPLETE (2026-04-03)**
> **Kafra Storage** — account-shared 300-slot storage, 10 socket handlers (6 storage + 4 inventory QoL: split/sort/auto-stack/search), 40z Kafra fee, JSONB compounded_cards, 4 bugs fixed during implementation.
> **Player Trading** — TradeSession class, 9 socket handlers, two-step confirm (OK → Trade), 13 cancel paths, atomic transfer, trade_logs audit trail, `/trade` chat command, STradeWidget (Z=22), 4 bugs fixed.
> Memory: `memory/storage-system.md`, `memory/trading-system.md`. Skills: `/sabrimmo-storage`, `/sabrimmo-trading`.

**Created:** 2026-04-02
**Systems:** Kafra Storage (Account-Shared) + Direct Player Trading
**Source:** RO Classic pre-renewal (rAthena canonical), full codebase audit
**Tracks:** Implementation checklist at bottom of each section

---

## Table of Contents

1. [System A: Kafra Storage](#system-a-kafra-storage)
   - [A1. RO Classic Reference](#a1-ro-classic-reference)
   - [A2. Database Schema](#a2-database-schema)
   - [A3. Server Implementation](#a3-server-implementation)
   - [A4. Client Implementation](#a4-client-implementation)
   - [A5. Integration Points](#a5-integration-points)
   - [A6. Implementation Checklist](#a6-implementation-checklist)
2. [System B: Player-to-Player Trading](#system-b-player-to-player-trading)
   - [B1. RO Classic Reference](#b1-ro-classic-reference)
   - [B2. Database Schema](#b2-database-schema)
   - [B3. Server Implementation](#b3-server-implementation)
   - [B4. Client Implementation](#b4-client-implementation)
   - [B5. Integration Points](#b5-integration-points)
   - [B6. Implementation Checklist](#b6-implementation-checklist)
3. [Shared Infrastructure](#shared-infrastructure)
4. [Supporting System Updates](#supporting-system-updates)

---

# System A: Kafra Storage

## A1. RO Classic Reference

### Core Mechanics (Pre-Renewal Canonical)

| Property | Value |
|----------|-------|
| **Slot capacity** | **300 slots** (fixed, no expansion in pre-renewal) |
| **Access fee** | **40 Zeny** per open |
| **Fee bypass** | Free Ticket for Kafra Storage (item ID 7059, weight 0, consumed on use) |
| **Prerequisite** | Basic Skill Level 6 (`learnedSkills[1] >= 6`) |
| **Account-shared** | YES — all characters on the same `user_id` share one storage pool |
| **Zeny storage** | NO — cannot store Zeny (only items) |
| **Kafra Points** | 1 point per 10 Zeny spent on Kafra services (future feature, skip for now) |

### Item Restrictions

| Rule | Implementation |
|------|----------------|
| **Equipped items** | CANNOT deposit — must unequip first |
| **Quest items** | CANNOT deposit — check `trade_flag & 0x004` (NoStorage bit) |
| **Hatched pet eggs** | CANNOT deposit — check for active pet linked to egg |
| **Account-bound items** | CAN deposit (storage is account-shared, so this is safe) |
| **Character-bound items** | CANNOT deposit — check `trade_flag & 0x200` (CharBound bit) |
| **Unidentified items** | CAN deposit — `identified` flag preserved |
| **Refined equipment** | CAN deposit — `refine_level` preserved |
| **Carded equipment** | CAN deposit — `compounded_cards` preserved |
| **Forged equipment** | CAN deposit — `forged_by`, `forged_element`, `forged_star_crumbs` preserved |
| **Stackable items** | Stack in storage if same item_id + both identified (or both unidentified) |

### Weight Interaction

| Action | Weight Effect |
|--------|--------------|
| **Deposit to storage** | DECREASES player weight (this is the primary purpose — offload items) |
| **Withdraw from storage** | INCREASES player weight |
| **Deposit while overweight (90%+)** | ALLOWED — storage is the escape valve for overweight |
| **Withdraw causing overweight** | ALLOWED — item enters inventory, player becomes overweight |

### Cart ↔ Storage Transfers

| Transfer | Supported | Restriction |
|----------|-----------|-------------|
| Cart → Storage | YES | Blocked while vending |
| Storage → Cart | YES | Blocked while vending, weight checked against cart limits |
| Inventory → Storage | YES | Primary flow |
| Storage → Inventory | YES | Primary flow |

### UI Behavior

| Behavior | Detail |
|----------|--------|
| **Open flow** | Click Kafra → Select "Storage" from menu → 40z deducted → Grid opens |
| **Close triggers** | Click X button, press Escape, change zones, disconnect |
| **Walk-away** | Storage stays open (no server-side distance check) |
| **Movement during** | Player CAN move with storage open (NPC dialog closes, storage persists) |
| **Tabs** | None in pre-renewal (single grid). We'll add 3 tabs for QoL: Item, Equip, Etc |
| **Deposit method** | Drag-and-drop from inventory, OR Alt+Right-Click on inventory item |
| **Withdraw method** | Drag-and-drop from storage, OR Alt+Right-Click on storage item |

---

## A2. Database Schema

### New Table: `account_storage`

```sql
-- File: database/migrations/add_account_storage.sql

-- Account-shared Kafra storage (300 slots, shared across all characters on same user_id)
CREATE TABLE IF NOT EXISTS account_storage (
    storage_id      SERIAL PRIMARY KEY,
    user_id         INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL REFERENCES items(item_id),
    quantity        INTEGER NOT NULL DEFAULT 1,
    slot_index      INTEGER NOT NULL DEFAULT -1,
    identified      BOOLEAN NOT NULL DEFAULT true,
    refine_level    INTEGER NOT NULL DEFAULT 0,
    compounded_cards TEXT DEFAULT '[]',       -- JSON array of card item_ids (matches character_inventory format)
    forged_by       VARCHAR(50) DEFAULT NULL,
    forged_element  VARCHAR(10) DEFAULT NULL,
    forged_star_crumbs INTEGER DEFAULT 0,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id, slot_index)
);

CREATE INDEX IF NOT EXISTS idx_storage_user ON account_storage(user_id);

-- Track storage usage per account
COMMENT ON TABLE account_storage IS 'Kafra storage — account-shared, 300 slots, 40z access fee';
```

**Design decisions:**
- Keyed on `user_id` (NOT `character_id`) — storage is account-shared
- Schema mirrors `character_cart` exactly (same item attribute columns)
- `slot_index` with UNIQUE constraint on `(user_id, slot_index)` prevents slot collisions
- `compounded_cards` stored as TEXT/JSON array matching existing inventory pattern
- No `is_equipped` column — storage items are never equipped

### Server Auto-Create Block

Add to the server startup auto-creation section (near `// Ensure stat columns exist`):

```javascript
// Ensure account_storage table exists
await pool.query(`
    CREATE TABLE IF NOT EXISTS account_storage (
        storage_id SERIAL PRIMARY KEY,
        user_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
        item_id INTEGER NOT NULL REFERENCES items(item_id),
        quantity INTEGER NOT NULL DEFAULT 1,
        slot_index INTEGER NOT NULL DEFAULT -1,
        identified BOOLEAN NOT NULL DEFAULT true,
        refine_level INTEGER NOT NULL DEFAULT 0,
        compounded_cards TEXT DEFAULT '[]',
        forged_by VARCHAR(50) DEFAULT NULL,
        forged_element VARCHAR(10) DEFAULT NULL,
        forged_star_crumbs INTEGER DEFAULT 0,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        UNIQUE (user_id, slot_index)
    )
`);
await pool.query(`CREATE INDEX IF NOT EXISTS idx_storage_user ON account_storage(user_id)`);
```

---

## A3. Server Implementation

### Constants

```javascript
const STORAGE = {
    MAX_SLOTS: 300,
    ACCESS_FEE: 40,         // Zeny per open
    FREE_TICKET_ID: 7059,   // Item that bypasses fee
    REQUIRED_BASIC_SKILL: 6, // Basic Skill Lv.6 needed
};
```

### State Tracking

```javascript
// On the player object (in-memory):
player.isStorageOpen = false;   // Prevents concurrent actions (trading, vending)
```

### Socket Events

```
Client → Server:
  storage:open         {}                          -- Request to open storage (deducts fee)
  storage:close        {}                          -- Close storage window
  storage:deposit      { inventoryId, quantity }   -- Move inventory item → storage
  storage:withdraw     { storageId, quantity }     -- Move storage item → inventory
  storage:cart_deposit { cartId, quantity }         -- Move cart item → storage (if cart equipped)
  storage:cart_withdraw { storageId, quantity }     -- Move storage item → cart (if cart equipped)

Server → Client:
  storage:opened       { items, usedSlots, maxSlots, newZuzucoin }  -- Storage contents + fee deducted
  storage:closed       {}                          -- Confirm closed
  storage:updated      { items, usedSlots }        -- Refreshed storage contents after deposit/withdraw
  storage:error        { message }                 -- Error response
```

### Handler: `storage:open`

```javascript
socket.on('storage:open', async () => {
    const player = connectedPlayers.get(socket.id);
    if (!player) return;

    // 1. State validation
    if (player.isDead) return socket.emit('storage:error', { message: 'Cannot use storage while dead' });
    if (player.isVending) return socket.emit('storage:error', { message: 'Cannot use storage while vending' });
    if (player.isTrading) return socket.emit('storage:error', { message: 'Cannot use storage while trading' });
    if (player.isStorageOpen) return socket.emit('storage:error', { message: 'Storage is already open' });

    // 2. Check Basic Skill Lv.6
    const basicSkillLevel = (player.learnedSkills && player.learnedSkills[1]) || 0;
    if (basicSkillLevel < STORAGE.REQUIRED_BASIC_SKILL) {
        return socket.emit('storage:error', { message: 'You need Basic Skill Level 6 to use storage' });
    }

    // 3. Check for Free Ticket (item 7059) — consume if found, else deduct 40z
    const ticketResult = await pool.query(
        `SELECT inventory_id, quantity FROM character_inventory
         WHERE character_id = $1 AND item_id = $2 AND quantity > 0 AND is_equipped = false`,
        [player.characterId, STORAGE.FREE_TICKET_ID]
    );

    if (ticketResult.rows.length > 0) {
        // Consume one free ticket
        const ticket = ticketResult.rows[0];
        if (ticket.quantity > 1) {
            await pool.query('UPDATE character_inventory SET quantity = quantity - 1 WHERE inventory_id = $1', [ticket.inventory_id]);
        } else {
            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [ticket.inventory_id]);
        }
    } else {
        // Deduct 40 Zeny fee
        if ((player.zuzucoin || 0) < STORAGE.ACCESS_FEE) {
            return socket.emit('storage:error', { message: 'Not enough Zeny (40z required)' });
        }
        player.zuzucoin -= STORAGE.ACCESS_FEE;
        await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [player.zuzucoin, player.characterId]);
    }

    // 4. Load storage contents (query by user_id, not character_id)
    const storageResult = await pool.query(`
        SELECT s.storage_id, s.item_id, s.quantity, s.slot_index, s.identified,
               s.refine_level, s.compounded_cards, s.forged_by, s.forged_element, s.forged_star_crumbs,
               i.name, i.description, i.full_description, i.item_type, i.equip_slot, i.weight,
               i.price, i.buy_price, i.sell_price, i.atk, i.def, i.matk, i.mdef,
               i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
               i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
               i.perfect_dodge_bonus, i.weapon_type, i.weapon_level, i.slots, i.refineable,
               i.icon, i.required_level, i.stackable, i.max_stack, i.equip_locations,
               i.two_handed, i.element, i.card_type, i.card_prefix, i.card_suffix, i.jobs_allowed,
               i.sub_type, i.view_sprite, i.aspd_modifier, i.weapon_range
        FROM account_storage s
        JOIN items i ON s.item_id = i.item_id
        WHERE s.user_id = $1
        ORDER BY s.slot_index
    `, [player.userId]);

    // 5. Mark storage open
    player.isStorageOpen = true;

    // 6. Emit storage contents
    const items = storageResult.rows.map(row => ({
        storageId: row.storage_id,
        itemId: row.item_id,
        name: row.name,
        description: row.description,
        fullDescription: row.full_description,
        itemType: row.item_type,
        equipSlot: row.equip_slot,
        weight: row.weight,
        price: row.price,
        buyPrice: row.buy_price,
        sellPrice: row.sell_price,
        quantity: row.quantity,
        slotIndex: row.slot_index,
        identified: row.identified,
        refineLevel: row.refine_level,
        compoundedCards: JSON.parse(row.compounded_cards || '[]'),
        forgedBy: row.forged_by,
        forgedElement: row.forged_element,
        forgedStarCrumbs: row.forged_star_crumbs,
        atk: row.atk, def: row.def, matk: row.matk, mdef: row.mdef,
        strBonus: row.str_bonus, agiBonus: row.agi_bonus, vitBonus: row.vit_bonus,
        intBonus: row.int_bonus, dexBonus: row.dex_bonus, lukBonus: row.luk_bonus,
        maxHpBonus: row.max_hp_bonus, maxSpBonus: row.max_sp_bonus,
        hitBonus: row.hit_bonus, fleeBonus: row.flee_bonus,
        criticalBonus: row.critical_bonus, perfectDodgeBonus: row.perfect_dodge_bonus,
        weaponType: row.weapon_type, weaponLevel: row.weapon_level,
        slots: row.slots, refineable: row.refineable,
        icon: row.icon, requiredLevel: row.required_level,
        stackable: row.stackable, maxStack: row.max_stack,
        twoHanded: row.two_handed, element: row.element,
        cardType: row.card_type, cardPrefix: row.card_prefix, cardSuffix: row.card_suffix,
        jobsAllowed: row.jobs_allowed, subType: row.sub_type,
        viewSprite: row.view_sprite, aspdModifier: row.aspd_modifier, weaponRange: row.weapon_range
    }));

    socket.emit('storage:opened', {
        items,
        usedSlots: items.length,
        maxSlots: STORAGE.MAX_SLOTS,
        newZuzucoin: player.zuzucoin
    });

    // Also refresh inventory (fee consumed ticket or zeny)
    await sendPlayerInventory(socket, player.characterId);
});
```

### Handler: `storage:close`

```javascript
socket.on('storage:close', () => {
    const player = connectedPlayers.get(socket.id);
    if (!player) return;
    player.isStorageOpen = false;
    socket.emit('storage:closed', {});
});
```

### Handler: `storage:deposit`

```javascript
socket.on('storage:deposit', async (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isStorageOpen) return;
    const { inventoryId, quantity } = data;
    if (!inventoryId || !quantity || quantity < 1) return socket.emit('storage:error', { message: 'Invalid request' });

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // 1. Lock and validate inventory item
        const invResult = await client.query(
            `SELECT ci.*, i.item_type, i.stackable, i.weight, i.name, i.trade_flag
             FROM character_inventory ci
             JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2 FOR UPDATE`,
            [inventoryId, player.characterId]
        );
        if (invResult.rows.length === 0) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'Item not found' }); }
        const item = invResult.rows[0];

        // 2. Validation checks
        if (item.is_equipped) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'Unequip the item first' }); }
        if (quantity > item.quantity) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'Not enough quantity' }); }
        // NoStorage flag check (bit 0x004)
        const tradeFlag = item.trade_flag || 0;
        if (tradeFlag & 0x004) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'This item cannot be stored' }); }

        // 3. Check storage slot count
        const countResult = await client.query(
            'SELECT COUNT(*) as cnt FROM account_storage WHERE user_id = $1', [player.userId]
        );
        const usedSlots = parseInt(countResult.rows[0].cnt);

        // 4. Try to stack with existing storage item (if stackable + same identified state)
        let stacked = false;
        if (item.stackable) {
            const existingResult = await client.query(
                `SELECT storage_id, quantity FROM account_storage
                 WHERE user_id = $1 AND item_id = $2 AND identified = $3 FOR UPDATE`,
                [player.userId, item.item_id, item.identified]
            );
            if (existingResult.rows.length > 0) {
                const existing = existingResult.rows[0];
                await client.query(
                    'UPDATE account_storage SET quantity = quantity + $1 WHERE storage_id = $2',
                    [quantity, existing.storage_id]
                );
                stacked = true;
            }
        }

        // 5. If not stacked, insert new storage entry
        if (!stacked) {
            if (usedSlots >= STORAGE.MAX_SLOTS) {
                await client.query('ROLLBACK');
                return socket.emit('storage:error', { message: 'Storage is full (300/300)' });
            }
            // Find next available slot_index
            const maxSlotResult = await client.query(
                'SELECT COALESCE(MAX(slot_index), -1) + 1 as next_slot FROM account_storage WHERE user_id = $1',
                [player.userId]
            );
            const nextSlot = maxSlotResult.rows[0].next_slot;

            await client.query(`
                INSERT INTO account_storage (user_id, item_id, quantity, slot_index, identified, refine_level, compounded_cards, forged_by, forged_element, forged_star_crumbs)
                VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)
            `, [player.userId, item.item_id, quantity, nextSlot, item.identified,
                item.refine_level || 0, item.compounded_cards || '[]',
                item.forged_by, item.forged_element, item.forged_star_crumbs || 0]);
        }

        // 6. Remove from inventory (BEFORE adding — anti-dupe rule)
        if (quantity >= item.quantity) {
            await client.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
        } else {
            await client.query('UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2', [quantity, inventoryId]);
        }

        await client.query('COMMIT');

        // 7. Update weight cache
        player.currentWeight = await calculatePlayerCurrentWeight(player.characterId);

        // 8. Refresh both inventory and storage
        await sendPlayerInventory(socket, player.characterId);
        await sendStorageContents(socket, player.userId);

    } catch (err) {
        await client.query('ROLLBACK');
        console.error('storage:deposit error:', err);
        socket.emit('storage:error', { message: 'Deposit failed' });
    } finally {
        client.release();
    }
});
```

### Handler: `storage:withdraw`

```javascript
socket.on('storage:withdraw', async (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isStorageOpen) return;
    const { storageId, quantity } = data;
    if (!storageId || !quantity || quantity < 1) return socket.emit('storage:error', { message: 'Invalid request' });

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // 1. Lock and validate storage item
        const storResult = await client.query(
            `SELECT s.*, i.name, i.item_type, i.stackable, i.weight
             FROM account_storage s
             JOIN items i ON s.item_id = i.item_id
             WHERE s.storage_id = $1 AND s.user_id = $2 FOR UPDATE`,
            [storageId, player.userId]
        );
        if (storResult.rows.length === 0) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'Item not found in storage' }); }
        const sItem = storResult.rows[0];

        // 2. Quantity check
        if (quantity > sItem.quantity) { await client.query('ROLLBACK'); return socket.emit('storage:error', { message: 'Not enough quantity in storage' }); }

        // 3. Remove from storage FIRST (anti-dupe)
        if (quantity >= sItem.quantity) {
            await client.query('DELETE FROM account_storage WHERE storage_id = $1', [storageId]);
        } else {
            await client.query('UPDATE account_storage SET quantity = quantity - $1 WHERE storage_id = $2', [quantity, storageId]);
        }

        // 4. Add to inventory using existing helper
        await addFullItemToInventory(player.characterId, sItem.item_id, quantity, {
            identified: sItem.identified,
            refine_level: sItem.refine_level,
            compounded_cards: sItem.compounded_cards,
            forged_by: sItem.forged_by,
            forged_element: sItem.forged_element,
            forged_star_crumbs: sItem.forged_star_crumbs,
        }, client);

        await client.query('COMMIT');

        // 5. Update weight cache
        player.currentWeight = await calculatePlayerCurrentWeight(player.characterId);

        // 6. Refresh both
        await sendPlayerInventory(socket, player.characterId);
        await sendStorageContents(socket, player.userId);

    } catch (err) {
        await client.query('ROLLBACK');
        console.error('storage:withdraw error:', err);
        socket.emit('storage:error', { message: 'Withdraw failed' });
    } finally {
        client.release();
    }
});
```

### Handler: `storage:cart_deposit` and `storage:cart_withdraw`

Follow exact same pattern as `storage:deposit` / `storage:withdraw` but:
- Source/destination is `character_cart` instead of `character_inventory`
- Additional check: `player.hasCart` must be true
- Additional check: `player.isVending` must be false (vending locks cart)
- Weight validation: cart has 8000 max weight, check before withdraw-to-cart

### Helper: `sendStorageContents(socket, userId)`

```javascript
async function sendStorageContents(socket, userId) {
    // Same query as storage:open but emits storage:updated instead
    const result = await pool.query(`
        SELECT s.*, i.name, i.description, i.full_description, i.item_type, i.equip_slot,
               i.weight, i.price, i.buy_price, i.sell_price, i.atk, i.def, i.matk, i.mdef,
               i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
               i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
               i.perfect_dodge_bonus, i.weapon_type, i.weapon_level, i.slots, i.refineable,
               i.icon, i.required_level, i.stackable, i.max_stack, i.two_handed, i.element,
               i.card_type, i.card_prefix, i.card_suffix, i.jobs_allowed, i.sub_type,
               i.view_sprite, i.aspd_modifier, i.weapon_range, i.equip_locations
        FROM account_storage s
        JOIN items i ON s.item_id = i.item_id
        WHERE s.user_id = $1
        ORDER BY s.slot_index
    `, [userId]);

    const items = result.rows.map(row => formatStorageItem(row));
    socket.emit('storage:updated', { items, usedSlots: items.length });
}
```

### Auto-Close on Disconnect / Zone Change

```javascript
// In the disconnect handler, add:
if (player.isStorageOpen) {
    player.isStorageOpen = false;
}

// In zone:change handler, add:
if (player.isStorageOpen) {
    player.isStorageOpen = false;
    socket.emit('storage:closed', {});
}
```

---

## A4. Client Implementation

### A4.1 New Files to Create

| File | Purpose |
|------|---------|
| `UI/StorageSubsystem.h` | UWorldSubsystem — socket handlers, storage state, public API |
| `UI/StorageSubsystem.cpp` | Implementation |
| `UI/SStorageWidget.h` | Slate widget — storage grid with tabs, drag-drop |
| `UI/SStorageWidget.cpp` | Implementation |

### A4.2 CharacterData.h Updates

Add `Storage` to existing enums:

```cpp
// EItemDragSource — add Storage:
enum class EItemDragSource : uint8
{
    None, Inventory, Equipment, Cart, Storage  // ADD Storage
};

// EItemDropTarget — add StorageSlot:
enum class EItemDropTarget : uint8
{
    Cancelled, InventorySlot, EquipmentSlot, EquipmentPortrait,
    GameWorld, HotbarSlot, CartSlot, StorageSlot  // ADD StorageSlot
};

// New struct: FStorageItem (lightweight, mirrors FInventoryItem for storage items)
USTRUCT(BlueprintType)
struct FStorageItem
{
    GENERATED_BODY()

    UPROPERTY() int32 StorageId = 0;
    UPROPERTY() int32 ItemId = 0;
    UPROPERTY() FString Name;
    UPROPERTY() FString Description;
    UPROPERTY() FString FullDescription;
    UPROPERTY() FString ItemType;     // weapon, armor, consumable, card, etc
    UPROPERTY() FString EquipSlot;
    UPROPERTY() int32 Quantity = 1;
    UPROPERTY() int32 Weight = 0;
    UPROPERTY() int32 Price = 0;
    UPROPERTY() int32 BuyPrice = 0;
    UPROPERTY() int32 SellPrice = 0;
    UPROPERTY() int32 ATK = 0;
    UPROPERTY() int32 DEF = 0;
    UPROPERTY() int32 MATK = 0;
    UPROPERTY() int32 MDEF = 0;
    UPROPERTY() int32 StrBonus = 0;
    UPROPERTY() int32 AgiBonus = 0;
    UPROPERTY() int32 VitBonus = 0;
    UPROPERTY() int32 IntBonus = 0;
    UPROPERTY() int32 DexBonus = 0;
    UPROPERTY() int32 LukBonus = 0;
    UPROPERTY() int32 MaxHPBonus = 0;
    UPROPERTY() int32 MaxSPBonus = 0;
    UPROPERTY() int32 HitBonus = 0;
    UPROPERTY() int32 FleeBonus = 0;
    UPROPERTY() int32 CriticalBonus = 0;
    UPROPERTY() int32 PerfectDodgeBonus = 0;
    UPROPERTY() FString WeaponType;
    UPROPERTY() int32 WeaponLevel = 0;
    UPROPERTY() int32 Slots = 0;
    UPROPERTY() bool bRefineable = false;
    UPROPERTY() int32 RefineLevel = 0;
    UPROPERTY() FString Icon;
    UPROPERTY() int32 RequiredLevel = 0;
    UPROPERTY() bool bStackable = false;
    UPROPERTY() int32 MaxStack = 1;
    UPROPERTY() bool bTwoHanded = false;
    UPROPERTY() FString Element;
    UPROPERTY() FString CardType;
    UPROPERTY() FString CardPrefix;
    UPROPERTY() FString CardSuffix;
    UPROPERTY() FString JobsAllowed;
    UPROPERTY() bool bIdentified = true;
    UPROPERTY() TArray<int32> CompoundedCards;
    UPROPERTY() FString ForgedBy;
    UPROPERTY() FString ForgedElement;
    UPROPERTY() int32 ForgedStarCrumbs = 0;
    UPROPERTY() int32 SlotIndex = 0;

    // Display name with refine + cards (reuse same logic as FInventoryItem::GetDisplayName)
    FString GetDisplayName() const;
};
```

### A4.3 StorageSubsystem

```cpp
// StorageSubsystem.h
UCLASS()
class UStorageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // State
    TArray<FStorageItem> Items;
    int32 UsedSlots = 0;
    int32 MaxSlots = 300;
    uint32 DataVersion = 0;
    bool bIsOpen = false;

    // Tab filter (0=All, 1=Consumable, 2=Equipment, 3=Etc)
    int32 CurrentTab = 0;
    TArray<FStorageItem> GetFilteredItems() const;

    // Public API
    void RequestOpen();                            // Emit storage:open
    void RequestClose();                           // Emit storage:close
    void DepositItem(int32 InventoryId, int32 Quantity);   // Emit storage:deposit
    void WithdrawItem(int32 StorageId, int32 Quantity);    // Emit storage:withdraw
    void DepositFromCart(int32 CartId, int32 Quantity);     // Emit storage:cart_deposit
    void WithdrawToCart(int32 StorageId, int32 Quantity);   // Emit storage:cart_withdraw

    // Widget
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

private:
    // Socket handlers
    void HandleStorageOpened(TSharedPtr<FJsonObject> Data);
    void HandleStorageClosed(TSharedPtr<FJsonObject> Data);
    void HandleStorageUpdated(TSharedPtr<FJsonObject> Data);
    void HandleStorageError(TSharedPtr<FJsonObject> Data);

    void ParseStorageItems(const TArray<TSharedPtr<FJsonValue>>& JsonItems);

    TSharedPtr<SStorageWidget> StorageWidget;
    TSharedPtr<SWidget> StorageOverlay;
    bool bEventsWrapped = false;
    FTimerHandle WrapTimerHandle;
};
```

### A4.4 SStorageWidget

Layout matches `SInventoryWidget` patterns:

```
┌──────────────────────────────────────────────┐
│  ═══ Kafra Storage (127/300) ═══       [X]   │  ← Title bar (draggable)
├──────────────────────────────────────────────┤
│  [All] [Item] [Equip] [Etc]                  │  ← Tab bar
├──────────────────────────────────────────────┤
│  ┌────┬────┬────┬────┬────┬────┬────┬────┬─┐ │
│  │ .. │ .. │ .. │ .. │ .. │ .. │ .. │ .. │ │ │  ← 10-column scrollable grid
│  ├────┼────┼────┼────┼────┼────┼────┼────┼─┤ │    (matches Cart's 10 columns)
│  │ .. │ .. │ .. │ .. │ .. │ .. │ .. │ .. │ │ │
│  └────┴────┴────┴────┴────┴────┴────┴────┴─┘ │
├──────────────────────────────────────────────┤
│  127/300 slots                                │  ← Slot count (no weight — storage has none)
└──────────────────────────────────────────────┘
```

- Z-Order: **21** (between SkillTree Z=20 and popups Z=23)
- RO brown/gold theme matching all other panels
- Drag-drop: Accept items dragged from Inventory and Cart
- Drag out: Items dragged from storage → drop on Inventory or Cart
- Right-click: Open ItemInspectSubsystem popup (same as inventory)
- Double-click: Quick-withdraw to inventory
- Alt+Right-Click: Quick-deposit from inventory (when storage is open and inventory is open)
- Resizable with min size (200x200)
- Scrollable grid for 300 slots

### A4.5 KafraSubsystem Updates

Add "Storage" button to the existing Kafra menu:

```cpp
// In SKafraWidget::BuildMainMenuContent(), add between "Teleport Service" and "Rent Cart":
BuildKafraButton(TEXT("Use Storage (40z)"), [this]() {
    if (UStorageSubsystem* StorageSub = GetWorld()->GetSubsystem<UStorageSubsystem>()) {
        StorageSub->RequestOpen();
        // Kafra menu stays open — storage widget opens alongside
    }
});
```

The storage widget opens next to the Kafra menu. When the Kafra menu closes (Cancel/Escape), the storage widget also closes (`StorageSubsystem->RequestClose()`).

### A4.6 InventorySubsystem Updates

Handle drops from storage:

```cpp
// In CompleteDrop(), add case for StorageSlot target:
case EItemDropTarget::StorageSlot:
    if (DragState.Source == EItemDragSource::Inventory) {
        // Deposit to storage
        if (UStorageSubsystem* StorageSub = GetWorld()->GetSubsystem<UStorageSubsystem>()) {
            StorageSub->DepositItem(DragState.InventoryId, DragState.Quantity);
        }
    }
    break;

// Handle drag FROM storage TO inventory:
case EItemDropTarget::InventorySlot:
    if (DragState.Source == EItemDragSource::Storage) {
        if (UStorageSubsystem* StorageSub = GetWorld()->GetSubsystem<UStorageSubsystem>()) {
            StorageSub->WithdrawItem(DragState.InventoryId, DragState.Quantity); // InventoryId = StorageId in this context
        }
    }
    break;
```

### A4.7 Build.cs Update

No new module dependencies needed — StorageSubsystem uses the same modules as existing subsystems (Core, CoreUObject, Engine, Slate, SlateCore, UMG, Json, JsonUtilities).

---

## A5. Integration Points

| Existing System | Change Required |
|----------------|-----------------|
| **KafraSubsystem** | Add "Use Storage" menu button, call `StorageSubsystem->RequestOpen()` |
| **SKafraWidget** | Add storage button in `BuildMainMenuContent()` |
| **CharacterData.h** | Add `FStorageItem` struct, extend enums with `Storage`/`StorageSlot` |
| **InventorySubsystem** | Handle `EItemDragSource::Storage` in `CompleteDrop()` |
| **CartSubsystem** | Handle `EItemDragSource::Storage` drops to cart, and cart-to-storage deposits |
| **SabriMMO.Build.cs** | No changes needed |
| **server/src/index.js** | Add `STORAGE` constants, `isStorageOpen` state, 6 socket handlers, auto-close in disconnect/zone-change, auto-create table at startup |
| **database/migrations/** | New file: `add_account_storage.sql` |
| **Weight system** | `storage:deposit` reduces weight, `storage:withdraw` increases weight (both call `calculatePlayerCurrentWeight`) |
| **Vending system** | Check `player.isStorageOpen` before allowing vend start; check `player.isVending` before allowing storage open |
| **Trade system** | Check `player.isStorageOpen` before allowing trade; check `player.isTrading` before allowing storage open |

---

## A6. Implementation Checklist

### Database
- [ ] Create `database/migrations/add_account_storage.sql`
- [ ] Add auto-create block in server startup for `account_storage` table + index

### Server (index.js)
- [ ] Add `STORAGE` constants (MAX_SLOTS, ACCESS_FEE, FREE_TICKET_ID, REQUIRED_BASIC_SKILL)
- [ ] Add `player.isStorageOpen = false` to player initialization
- [ ] Implement `storage:open` handler (fee logic, Basic Skill check, free ticket, load items)
- [ ] Implement `storage:close` handler
- [ ] Implement `storage:deposit` handler (inventory → storage, atomic transaction)
- [ ] Implement `storage:withdraw` handler (storage → inventory, atomic transaction)
- [ ] Implement `storage:cart_deposit` handler (cart → storage)
- [ ] Implement `storage:cart_withdraw` handler (storage → cart)
- [ ] Implement `sendStorageContents()` helper
- [ ] Add `isStorageOpen = false` to disconnect handler
- [ ] Add `isStorageOpen = false` + `storage:closed` emit to zone:change handler
- [ ] Add `isStorageOpen` check to vending:start handler (block if storage open)
- [ ] Add `isStorageOpen` check to trade:request handler (block if storage open)
- [ ] Add `isVending`/`isTrading` checks to storage:open handler

### Client — CharacterData.h
- [ ] Add `Storage` to `EItemDragSource` enum
- [ ] Add `StorageSlot` to `EItemDropTarget` enum
- [ ] Add `FStorageItem` struct with `GetDisplayName()`

### Client — StorageSubsystem
- [ ] Create `StorageSubsystem.h` — class declaration, state, public API, socket handlers
- [ ] Create `StorageSubsystem.cpp` — full implementation
- [ ] `ShouldCreateSubsystem()` — only in game worlds
- [ ] `OnWorldBeginPlay()` — register 4 socket event handlers via Router
- [ ] `Deinitialize()` — unregister all, hide widget, clear state
- [ ] `HandleStorageOpened()` — parse items, set bIsOpen, show widget
- [ ] `HandleStorageClosed()` — clear state, hide widget
- [ ] `HandleStorageUpdated()` — parse items, increment DataVersion
- [ ] `HandleStorageError()` — show error in chat or status bar
- [ ] `RequestOpen()` — emit storage:open
- [ ] `RequestClose()` — emit storage:close, set bIsOpen=false
- [ ] `DepositItem()` — emit storage:deposit
- [ ] `WithdrawItem()` — emit storage:withdraw
- [ ] `DepositFromCart()` — emit storage:cart_deposit
- [ ] `WithdrawToCart()` — emit storage:cart_withdraw
- [ ] `ShowWidget()` / `HideWidget()` — viewport overlay at Z=21
- [ ] `GetFilteredItems()` — tab-based filtering

### Client — SStorageWidget
- [ ] Create `SStorageWidget.h` — layout, drag-drop, tooltip
- [ ] Create `SStorageWidget.cpp` — full implementation
- [ ] `BuildTitleBar()` — "Kafra Storage (X/300)" + close button
- [ ] `BuildTabBar()` — All / Item / Equip / Etc tabs
- [ ] `BuildGridArea()` — 10-column scrollable grid
- [ ] `BuildBottomBar()` — slot count display
- [ ] `BuildItemSlot()` — individual cell with icon, quantity overlay
- [ ] `BuildTooltip()` — RO-style hover tooltip (reuse ItemTooltipBuilder)
- [ ] Drag-drop input (OnMouseButtonDown/Up/Move)
- [ ] Double-click → quick-withdraw
- [ ] Right-click → ItemInspect
- [ ] Window dragging (title bar)
- [ ] Window resize (bottom-right corner)
- [ ] Tick → poll DataVersion for rebuild
- [ ] Brown/gold RO theme colors

### Client — Integration Updates
- [ ] **KafraSubsystem**: No code changes needed (menu is in SKafraWidget)
- [ ] **SKafraWidget**: Add "Use Storage (40z)" button → calls `StorageSubsystem->RequestOpen()`
- [ ] **InventorySubsystem**: Add `Storage` source handling in `CompleteDrop()`
- [ ] **CartSubsystem**: Add `Storage` source handling for cart↔storage transfers

### Testing
- [ ] Open storage, verify 40z deducted, items load
- [ ] Deposit stackable item, verify stacks in storage
- [ ] Deposit equipment (with refine+cards), verify attributes preserved
- [ ] Withdraw item, verify appears in inventory with correct attributes
- [ ] Deposit while overweight (90%+), verify works
- [ ] Drag-drop from inventory to storage grid
- [ ] Drag-drop from storage to inventory
- [ ] Double-click in storage → withdraws to inventory
- [ ] Try to deposit equipped item → error message
- [ ] Storage full (300 items) → error on deposit
- [ ] Close storage via X button, Escape, zone change
- [ ] Open storage on Character A, deposit item, log in as Character B → item visible
- [ ] Cart ↔ Storage transfers (if cart is equipped)
- [ ] Cannot open storage while vending or trading
- [ ] Free Ticket for Kafra Storage bypasses 40z fee

---

# System B: Player-to-Player Trading

## B1. RO Classic Reference

### Core Mechanics (Pre-Renewal Canonical)

| Property | Value |
|----------|-------|
| **Item slots per side** | **10 slots** (items only, not Zeny) |
| **Zeny field** | 1 per side (both players can offer Zeny) |
| **Max Zeny per trade** | **2,147,483,647** (MAX_ZENY, same as character cap) |
| **Initiation** | Right-click player → "Trade" option, or `/trade PlayerName` chat command |
| **Distance required** | **2 cells** (~100 UE units) — checked at request and acceptance only |
| **Confirmation** | Two-step: "OK" (lock) → "Trade" (finalize). Both sides must complete both steps. |
| **Level restriction** | None — any level can trade |
| **Cooldown** | None — can trade again immediately |
| **Map restriction** | Some maps have `notrade` flag |

### Two-Step Confirmation Flow

```
State 0 — OPEN:       Both can add/remove items and set Zeny
State 1 — LOCKED:     Player clicked "OK". Cannot modify own items/Zeny.
                       Partner can still modify theirs.
State 2 — CONFIRMED:  Player clicked "Trade" (only available when both are at State 1+).
                       When BOTH reach State 2, transfer executes.
```

**Critical anti-scam rule:** Once a player clicks OK (State 1), they CANNOT add, remove, or change items/Zeny. The only escape is Cancel (which resets everything). If partner changes their offer after one side locks, the locked player sees the update and can Cancel before confirming.

### Item Restrictions

| Item Type | Tradeable? |
|-----------|-----------|
| Normal items | YES |
| Refined equipment | YES — refine level, cards, slots all transfer |
| Cards (not compounded) | YES |
| Unidentified items | YES — stay unidentified |
| Forged equipment | YES — forger name, element, star crumbs preserved |
| Equipped items | NO — must unequip first (server strips equipped items from deal) |
| Items with NoTrade flag | NO — trade_flag & 0x002 |
| Account-bound items | NO — trade_flag & 0x100 |
| Character-bound items | NO — trade_flag & 0x200 |
| Rental/expiring items | NO |
| Hatched pet eggs | NO |

### Auto-Cancel Triggers

| Trigger | Mechanism |
|---------|-----------|
| Map/zone change | Automatic — `zone:change` handler cancels trade |
| Death | Automatic — death handler cancels trade |
| Disconnect | Automatic — disconnect handler cancels trade |
| Player clicks Cancel | Manual — either player can cancel anytime |
| Opening storage | Blocked — cannot open storage during trade |
| Starting to vend | Blocked — cannot start vending during trade |
| Using NPC | Blocked — cannot interact with NPCs during trade |

### Chat Notifications (from msgstringtable)

| Event | Message |
|-------|---------|
| Trade requested | `"[System] Trade request from {Name}"` |
| Trade accepted | `"[System] Trade accepted"` |
| Trade rejected | `"[System] Trade request declined"` |
| Trade cancelled | `"[System] Trade cancelled"` |
| Trade completed | `"[System] Trade completed successfully"` |
| Trade failed | `"[System] Trade failed"` |
| Too far | `"[System] Too far away to trade"` |
| Target busy | `"[System] That player is busy"` |
| Overweight | `"[System] Trade would exceed weight limit"` |
| Over 10 items | `"[System] Cannot trade more than 10 item types"` |
| No zeny | `"[System] Not enough Zeny"` |

### Validation Checks

| Check | When |
|-------|------|
| Distance (100 UE units) | At request and acceptance |
| Same zone | At request |
| Weight limit (receiver) | At each item add AND at final commit |
| Inventory space (receiver) | At each item add AND at final commit |
| Zeny overflow (receiver) | At Zeny set AND at final commit |
| Item ownership | At final commit (re-verify all items still in inventory) |
| Equipped status | At final commit (strip any equipped items from deal) |
| Trade flags | At each item add |

---

## B2. Database Schema

### New Table: `trade_logs`

```sql
-- File: database/migrations/add_trade_system.sql

-- Player-to-player trade audit log
CREATE TABLE IF NOT EXISTS trade_logs (
    trade_id        SERIAL PRIMARY KEY,
    player_a_id     INTEGER NOT NULL REFERENCES characters(character_id),
    player_b_id     INTEGER NOT NULL REFERENCES characters(character_id),
    items_a_gave    JSONB NOT NULL DEFAULT '[]',   -- [{itemId, name, quantity, refineLevel, cards}]
    items_b_gave    JSONB NOT NULL DEFAULT '[]',
    zeny_a_gave     BIGINT NOT NULL DEFAULT 0,
    zeny_b_gave     BIGINT NOT NULL DEFAULT 0,
    zone_name       VARCHAR(50),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_trade_log_a ON trade_logs(player_a_id);
CREATE INDEX IF NOT EXISTS idx_trade_log_b ON trade_logs(player_b_id);
CREATE INDEX IF NOT EXISTS idx_trade_log_time ON trade_logs(created_at);
```

### Server Auto-Create Block

```javascript
await pool.query(`
    CREATE TABLE IF NOT EXISTS trade_logs (
        trade_id SERIAL PRIMARY KEY,
        player_a_id INTEGER NOT NULL REFERENCES characters(character_id),
        player_b_id INTEGER NOT NULL REFERENCES characters(character_id),
        items_a_gave JSONB NOT NULL DEFAULT '[]',
        items_b_gave JSONB NOT NULL DEFAULT '[]',
        zeny_a_gave BIGINT NOT NULL DEFAULT 0,
        zeny_b_gave BIGINT NOT NULL DEFAULT 0,
        zone_name VARCHAR(50),
        created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
    )
`);
await pool.query('CREATE INDEX IF NOT EXISTS idx_trade_log_a ON trade_logs(player_a_id)');
await pool.query('CREATE INDEX IF NOT EXISTS idx_trade_log_b ON trade_logs(player_b_id)');
```

---

## B3. Server Implementation

### Constants

```javascript
const TRADE = {
    MAX_ITEMS: 10,           // 10 item slots per side
    MAX_ZENY: 2147483647,    // int32 max
    DISTANCE: 100,           // UE units (~2 RO cells)
};
```

### In-Memory Trade State

```javascript
// Active trade sessions — keyed by BOTH participants
const activeTrades = new Map(); // characterId -> TradeSession

class TradeSession {
    constructor(playerA, playerB) {
        this.playerA = { characterId: playerA.characterId, socketId: playerA.socketId, name: playerA.characterName };
        this.playerB = { characterId: playerB.characterId, socketId: playerB.socketId, name: playerB.characterName };
        this.itemsA = [];        // [{inventoryId, itemId, name, quantity, weight, ...fullItemData}]
        this.itemsB = [];
        this.zenyA = 0;
        this.zenyB = 0;
        this.lockedA = false;    // State 1: OK clicked
        this.lockedB = false;
        this.confirmedA = false; // State 2: Trade clicked
        this.confirmedB = false;
        this.createdAt = Date.now();
    }

    getPartner(characterId) {
        if (characterId === this.playerA.characterId) return this.playerB;
        if (characterId === this.playerB.characterId) return this.playerA;
        return null;
    }

    isLocked(characterId) {
        if (characterId === this.playerA.characterId) return this.lockedA;
        return this.lockedB;
    }

    setLocked(characterId) {
        if (characterId === this.playerA.characterId) this.lockedA = true;
        else this.lockedB = true;
    }

    setConfirmed(characterId) {
        if (characterId === this.playerA.characterId) this.confirmedA = true;
        else this.confirmedB = true;
    }

    bothLocked() { return this.lockedA && this.lockedB; }
    bothConfirmed() { return this.confirmedA && this.confirmedB; }

    getItems(characterId) {
        return characterId === this.playerA.characterId ? this.itemsA : this.itemsB;
    }

    getZeny(characterId) {
        return characterId === this.playerA.characterId ? this.zenyA : this.zenyB;
    }

    setZeny(characterId, amount) {
        if (characterId === this.playerA.characterId) this.zenyA = amount;
        else this.zenyB = amount;
    }

    // Reset locks when items change (anti-scam)
    resetLocks() {
        this.lockedA = false;
        this.lockedB = false;
        this.confirmedA = false;
        this.confirmedB = false;
    }
}
```

### Player State Tracking

```javascript
// On the player object:
player.isTrading = false;
player.tradePartnerId = null;   // characterId of trade partner
```

### Pending Trade Requests

```javascript
const pendingTradeRequests = new Map(); // targetCharacterId -> { fromCharacterId, fromName, timestamp }
```

### Socket Events

```
Client → Server:
  trade:request      { targetCharacterId }
  trade:accept       {}                           -- Accept pending request
  trade:decline      {}                           -- Decline pending request
  trade:add_item     { inventoryId, quantity }     -- Place item in trade
  trade:remove_item  { tradeSlot }                -- Remove item from trade (0-9)
  trade:set_zeny     { amount }                   -- Set Zeny offer
  trade:lock         {}                           -- Click "OK" (State 0→1)
  trade:confirm      {}                           -- Click "Trade" (State 1→2, requires both locked)
  trade:cancel       {}                           -- Cancel trade

Server → Client:
  trade:request_received  { fromCharacterId, fromName }
  trade:opened       { partnerId, partnerName }
  trade:item_added   { side, tradeSlot, itemData }  -- side = 'self' or 'partner'
  trade:item_removed { side, tradeSlot }
  trade:zeny_updated { side, amount }
  trade:partner_locked  {}
  trade:partner_unlocked {}                       -- Partner's lock reset (they or you changed items)
  trade:self_unlocked {}                          -- Your lock reset (partner changed items)
  trade:both_locked  {}                           -- Both locked, Trade button becomes clickable
  trade:completed    { receivedItems, receivedZeny, gaveItems, gaveZeny }
  trade:cancelled    { reason }
  trade:error        { message }
```

### Handler: `trade:request`

```javascript
socket.on('trade:request', (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player) return;
    const { targetCharacterId } = data;

    // 1. Validate self state
    if (player.isDead) return socket.emit('trade:error', { message: 'Cannot trade while dead' });
    if (player.isTrading) return socket.emit('trade:error', { message: 'Already in a trade' });
    if (player.isVending) return socket.emit('trade:error', { message: 'Cannot trade while vending' });
    if (player.isStorageOpen) return socket.emit('trade:error', { message: 'Close storage first' });

    // 2. Find target player (must be online, same zone)
    let targetSocket = null, targetPlayer = null;
    for (const [sid, p] of connectedPlayers.entries()) {
        if (p.characterId === targetCharacterId) {
            targetSocket = io.sockets.sockets.get(sid);
            targetPlayer = p;
            break;
        }
    }
    if (!targetPlayer || !targetSocket) return socket.emit('trade:error', { message: 'Player not found' });
    if (targetPlayer.zone !== player.zone) return socket.emit('trade:error', { message: 'Player is in another zone' });

    // 3. Validate target state
    if (targetPlayer.isDead) return socket.emit('trade:error', { message: 'That player is dead' });
    if (targetPlayer.isTrading) return socket.emit('trade:error', { message: 'That player is busy' });
    if (targetPlayer.isVending) return socket.emit('trade:error', { message: 'That player is vending' });
    if (targetPlayer.isStorageOpen) return socket.emit('trade:error', { message: 'That player is busy' });

    // 4. Distance check (~100 UE units)
    const dx = player.x - targetPlayer.x;
    const dy = player.y - targetPlayer.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > TRADE.DISTANCE) return socket.emit('trade:error', { message: 'Too far away to trade' });

    // 5. Check for existing pending request
    if (pendingTradeRequests.has(targetCharacterId)) {
        return socket.emit('trade:error', { message: 'That player has a pending trade request' });
    }

    // 6. Store pending request
    pendingTradeRequests.set(targetCharacterId, {
        fromCharacterId: player.characterId,
        fromName: player.characterName,
        fromSocketId: socket.id,
        timestamp: Date.now()
    });

    // 7. Auto-expire after 30 seconds
    setTimeout(() => {
        const req = pendingTradeRequests.get(targetCharacterId);
        if (req && req.fromCharacterId === player.characterId) {
            pendingTradeRequests.delete(targetCharacterId);
            socket.emit('trade:error', { message: 'Trade request timed out' });
        }
    }, 30000);

    // 8. Notify target
    targetSocket.emit('trade:request_received', {
        fromCharacterId: player.characterId,
        fromName: player.characterName
    });

    // 9. Notify sender
    socket.emit('trade:error', { message: `Trade request sent to ${targetPlayer.characterName}` });
});
```

### Handler: `trade:accept`

```javascript
socket.on('trade:accept', () => {
    const player = connectedPlayers.get(socket.id);
    if (!player) return;

    // 1. Find pending request
    const request = pendingTradeRequests.get(player.characterId);
    if (!request) return socket.emit('trade:error', { message: 'No pending trade request' });
    pendingTradeRequests.delete(player.characterId);

    // 2. Find requester
    const requesterPlayer = connectedPlayers.get(request.fromSocketId);
    const requesterSocket = io.sockets.sockets.get(request.fromSocketId);
    if (!requesterPlayer || !requesterSocket) return socket.emit('trade:error', { message: 'Player disconnected' });

    // 3. Re-validate both states
    if (requesterPlayer.isTrading || player.isTrading) return socket.emit('trade:error', { message: 'One of you is already trading' });

    // 4. Re-check distance
    const dx = player.x - requesterPlayer.x;
    const dy = player.y - requesterPlayer.y;
    if (Math.sqrt(dx * dx + dy * dy) > TRADE.DISTANCE) {
        return socket.emit('trade:error', { message: 'Too far away to trade' });
    }

    // 5. Create trade session
    const session = new TradeSession(requesterPlayer, player);
    activeTrades.set(requesterPlayer.characterId, session);
    activeTrades.set(player.characterId, session);

    // 6. Mark both players as trading
    requesterPlayer.isTrading = true;
    requesterPlayer.tradePartnerId = player.characterId;
    player.isTrading = true;
    player.tradePartnerId = requesterPlayer.characterId;

    // 7. Notify both — open trade windows
    requesterSocket.emit('trade:opened', { partnerId: player.characterId, partnerName: player.characterName });
    socket.emit('trade:opened', { partnerId: requesterPlayer.characterId, partnerName: requesterPlayer.characterName });
});
```

### Handler: `trade:decline`

```javascript
socket.on('trade:decline', () => {
    const player = connectedPlayers.get(socket.id);
    if (!player) return;

    const request = pendingTradeRequests.get(player.characterId);
    if (!request) return;
    pendingTradeRequests.delete(player.characterId);

    // Notify requester
    const requesterSocket = io.sockets.sockets.get(request.fromSocketId);
    if (requesterSocket) {
        requesterSocket.emit('trade:cancelled', { reason: `${player.characterName} declined the trade request` });
    }
});
```

### Handler: `trade:add_item`

```javascript
socket.on('trade:add_item', async (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isTrading) return;

    const session = activeTrades.get(player.characterId);
    if (!session) return;

    // 1. Cannot modify if locked
    if (session.isLocked(player.characterId)) {
        return socket.emit('trade:error', { message: 'Press Cancel to modify your offer' });
    }

    const { inventoryId, quantity } = data;
    if (!inventoryId || !quantity || quantity < 1) return;

    // 2. Check slot limit
    const myItems = session.getItems(player.characterId);
    if (myItems.length >= TRADE.MAX_ITEMS) {
        return socket.emit('trade:error', { message: 'Cannot trade more than 10 item types' });
    }

    // 3. Check item not already in trade
    if (myItems.some(i => i.inventoryId === inventoryId)) {
        return socket.emit('trade:error', { message: 'Item already in trade' });
    }

    // 4. Validate item in inventory
    const itemResult = await pool.query(`
        SELECT ci.*, i.name, i.item_type, i.weight, i.stackable, i.icon, i.trade_flag,
               i.equip_slot, i.weapon_type, i.weapon_level, i.slots, i.refineable,
               i.atk, i.def, i.matk, i.mdef, i.str_bonus, i.agi_bonus, i.vit_bonus,
               i.int_bonus, i.dex_bonus, i.luk_bonus, i.max_hp_bonus, i.max_sp_bonus,
               i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus,
               i.element, i.card_type, i.card_prefix, i.card_suffix, i.two_handed,
               i.required_level, i.max_stack, i.jobs_allowed, i.sub_type, i.view_sprite,
               i.description, i.full_description, i.aspd_modifier, i.weapon_range, i.buy_price, i.sell_price
        FROM character_inventory ci
        JOIN items i ON ci.item_id = i.item_id
        WHERE ci.inventory_id = $1 AND ci.character_id = $2
    `, [inventoryId, player.characterId]);

    if (itemResult.rows.length === 0) return socket.emit('trade:error', { message: 'Item not found' });
    const item = itemResult.rows[0];

    // 5. Validation checks
    if (item.is_equipped) return socket.emit('trade:error', { message: 'Unequip the item first' });
    if (quantity > item.quantity) return socket.emit('trade:error', { message: 'Not enough quantity' });
    const tradeFlag = item.trade_flag || 0;
    if (tradeFlag & 0x002) return socket.emit('trade:error', { message: 'This item cannot be traded' });

    // 6. Weight check on receiver
    const partner = session.getPartner(player.characterId);
    const partnerPlayer = getPlayerByCharacterId(partner.characterId);
    if (partnerPlayer) {
        const partnerMaxWeight = getPlayerMaxWeight(partnerPlayer);
        // Sum weight of all items already offered TO partner + this new item
        const partnerIncomingWeight = session.getItems(player.characterId).reduce((sum, i) => sum + (i.weight * i.quantity), 0);
        const newItemWeight = item.weight * quantity;
        if (partnerPlayer.currentWeight + partnerIncomingWeight + newItemWeight > partnerMaxWeight) {
            return socket.emit('trade:error', { message: 'Trade would exceed partner\'s weight limit' });
        }
    }

    // 7. Add to trade session
    const tradeSlot = myItems.length;
    myItems.push({
        inventoryId, itemId: item.item_id, name: item.name, quantity, weight: item.weight,
        refineLevel: item.refine_level || 0, compoundedCards: JSON.parse(item.compounded_cards || '[]'),
        identified: item.identified, icon: item.icon, itemType: item.item_type,
        equipSlot: item.equip_slot, weaponType: item.weapon_type, weaponLevel: item.weapon_level,
        slots: item.slots, element: item.element, forgedBy: item.forged_by,
        forgedElement: item.forged_element, forgedStarCrumbs: item.forged_star_crumbs,
        atk: item.atk, def: item.def, matk: item.matk, mdef: item.mdef,
        strBonus: item.str_bonus, agiBonus: item.agi_bonus, vitBonus: item.vit_bonus,
        intBonus: item.int_bonus, dexBonus: item.dex_bonus, lukBonus: item.luk_bonus,
        cardPrefix: item.card_prefix, cardSuffix: item.card_suffix,
        twoHanded: item.two_handed, requiredLevel: item.required_level,
        description: item.description, fullDescription: item.full_description,
        refineable: item.refineable, stackable: item.stackable, maxStack: item.max_stack,
        jobsAllowed: item.jobs_allowed,
        maxHpBonus: item.max_hp_bonus, maxSpBonus: item.max_sp_bonus,
        hitBonus: item.hit_bonus, fleeBonus: item.flee_bonus,
        criticalBonus: item.critical_bonus, perfectDodgeBonus: item.perfect_dodge_bonus,
        buyPrice: item.buy_price, sellPrice: item.sell_price,
    });

    // 8. If partner was locked, unlock both (anti-scam: offer changed)
    if (session.lockedA || session.lockedB) {
        session.resetLocks();
        // Notify both that locks were reset
        const socketA = io.sockets.sockets.get(session.playerA.socketId);
        const socketB = io.sockets.sockets.get(session.playerB.socketId);
        if (socketA) socketA.emit('trade:partner_unlocked', {});
        if (socketB) socketB.emit('trade:partner_unlocked', {});
    }

    // 9. Notify both parties
    const partnerSocket = io.sockets.sockets.get(partner.socketId);
    // Full item data to partner (so they can inspect)
    const itemPayload = { tradeSlot, ...myItems[tradeSlot] };
    socket.emit('trade:item_added', { side: 'self', tradeSlot, itemData: itemPayload });
    if (partnerSocket) {
        partnerSocket.emit('trade:item_added', { side: 'partner', tradeSlot, itemData: itemPayload });
    }
});
```

### Handler: `trade:remove_item`

```javascript
socket.on('trade:remove_item', (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isTrading) return;
    const session = activeTrades.get(player.characterId);
    if (!session) return;
    if (session.isLocked(player.characterId)) return socket.emit('trade:error', { message: 'Press Cancel to modify' });

    const { tradeSlot } = data;
    const myItems = session.getItems(player.characterId);
    if (tradeSlot < 0 || tradeSlot >= myItems.length) return;

    myItems.splice(tradeSlot, 1);

    // Reset locks (offer changed)
    if (session.lockedA || session.lockedB) {
        session.resetLocks();
        const socketA = io.sockets.sockets.get(session.playerA.socketId);
        const socketB = io.sockets.sockets.get(session.playerB.socketId);
        if (socketA) socketA.emit('trade:partner_unlocked', {});
        if (socketB) socketB.emit('trade:partner_unlocked', {});
    }

    const partner = session.getPartner(player.characterId);
    const partnerSocket = io.sockets.sockets.get(partner.socketId);
    socket.emit('trade:item_removed', { side: 'self', tradeSlot });
    if (partnerSocket) partnerSocket.emit('trade:item_removed', { side: 'partner', tradeSlot });
});
```

### Handler: `trade:set_zeny`

```javascript
socket.on('trade:set_zeny', (data) => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isTrading) return;
    const session = activeTrades.get(player.characterId);
    if (!session) return;
    if (session.isLocked(player.characterId)) return socket.emit('trade:error', { message: 'Press Cancel to modify' });

    let { amount } = data;
    amount = Math.max(0, Math.min(amount, TRADE.MAX_ZENY));
    if (amount > (player.zuzucoin || 0)) return socket.emit('trade:error', { message: 'Not enough Zeny' });

    // Check partner overflow
    const partner = session.getPartner(player.characterId);
    const partnerPlayer = getPlayerByCharacterId(partner.characterId);
    if (partnerPlayer) {
        const partnerZeny = partnerPlayer.zuzucoin || 0;
        if (partnerZeny + amount > TRADE.MAX_ZENY) {
            return socket.emit('trade:error', { message: 'Partner would exceed Zeny limit' });
        }
    }

    session.setZeny(player.characterId, amount);

    // Reset locks
    if (session.lockedA || session.lockedB) {
        session.resetLocks();
        const socketA = io.sockets.sockets.get(session.playerA.socketId);
        const socketB = io.sockets.sockets.get(session.playerB.socketId);
        if (socketA) socketA.emit('trade:partner_unlocked', {});
        if (socketB) socketB.emit('trade:partner_unlocked', {});
    }

    const partnerSocket = io.sockets.sockets.get(partner.socketId);
    socket.emit('trade:zeny_updated', { side: 'self', amount });
    if (partnerSocket) partnerSocket.emit('trade:zeny_updated', { side: 'partner', amount });
});
```

### Handler: `trade:lock`

```javascript
socket.on('trade:lock', () => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isTrading) return;
    const session = activeTrades.get(player.characterId);
    if (!session) return;
    if (session.isLocked(player.characterId)) return; // Already locked

    session.setLocked(player.characterId);

    const partner = session.getPartner(player.characterId);
    const partnerSocket = io.sockets.sockets.get(partner.socketId);
    if (partnerSocket) partnerSocket.emit('trade:partner_locked', {});

    // If both locked, notify both that Trade button is now active
    if (session.bothLocked()) {
        socket.emit('trade:both_locked', {});
        if (partnerSocket) partnerSocket.emit('trade:both_locked', {});
    }
});
```

### Handler: `trade:confirm`

```javascript
socket.on('trade:confirm', async () => {
    const player = connectedPlayers.get(socket.id);
    if (!player || !player.isTrading) return;
    const session = activeTrades.get(player.characterId);
    if (!session || !session.bothLocked()) return;

    session.setConfirmed(player.characterId);

    // If only one confirmed, wait for the other
    if (!session.bothConfirmed()) return;

    // ===== BOTH CONFIRMED — EXECUTE TRADE =====
    const playerA = getPlayerByCharacterId(session.playerA.characterId);
    const playerB = getPlayerByCharacterId(session.playerB.characterId);
    const socketA = io.sockets.sockets.get(session.playerA.socketId);
    const socketB = io.sockets.sockets.get(session.playerB.socketId);

    if (!playerA || !playerB) {
        cancelTrade(session, 'Player disconnected');
        return;
    }

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // 1. Re-validate all items exist and are owned (impossible_trade_check)
        for (const item of session.itemsA) {
            const check = await client.query(
                'SELECT inventory_id, quantity, is_equipped FROM character_inventory WHERE inventory_id = $1 AND character_id = $2 FOR UPDATE',
                [item.inventoryId, session.playerA.characterId]
            );
            if (check.rows.length === 0 || check.rows[0].is_equipped || check.rows[0].quantity < item.quantity) {
                await client.query('ROLLBACK');
                cancelTrade(session, 'Trade failed — items changed');
                return;
            }
        }
        for (const item of session.itemsB) {
            const check = await client.query(
                'SELECT inventory_id, quantity, is_equipped FROM character_inventory WHERE inventory_id = $1 AND character_id = $2 FOR UPDATE',
                [item.inventoryId, session.playerB.characterId]
            );
            if (check.rows.length === 0 || check.rows[0].is_equipped || check.rows[0].quantity < item.quantity) {
                await client.query('ROLLBACK');
                cancelTrade(session, 'Trade failed — items changed');
                return;
            }
        }

        // 2. Re-validate Zeny
        if (session.zenyA > (playerA.zuzucoin || 0) || session.zenyB > (playerB.zuzucoin || 0)) {
            await client.query('ROLLBACK');
            cancelTrade(session, 'Trade failed — insufficient Zeny');
            return;
        }
        if ((playerB.zuzucoin || 0) + session.zenyA > TRADE.MAX_ZENY ||
            (playerA.zuzucoin || 0) + session.zenyB > TRADE.MAX_ZENY) {
            await client.query('ROLLBACK');
            cancelTrade(session, 'Trade failed — Zeny overflow');
            return;
        }

        // 3. Re-validate weight
        const weightAGiving = session.itemsA.reduce((s, i) => s + i.weight * i.quantity, 0);
        const weightBGiving = session.itemsB.reduce((s, i) => s + i.weight * i.quantity, 0);
        // Player A loses weightAGiving, gains weightBGiving
        // Player B loses weightBGiving, gains weightAGiving

        // 4. Transfer items: A → B
        for (const item of session.itemsA) {
            // Remove from A
            if (item.quantity >= (await getItemQuantity(client, item.inventoryId))) {
                await client.query('DELETE FROM character_inventory WHERE inventory_id = $1', [item.inventoryId]);
            } else {
                await client.query('UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2', [item.quantity, item.inventoryId]);
            }
            // Add to B
            await addFullItemToInventory(session.playerB.characterId, item.itemId, item.quantity, {
                identified: item.identified,
                refine_level: item.refineLevel,
                compounded_cards: JSON.stringify(item.compoundedCards || []),
                forged_by: item.forgedBy,
                forged_element: item.forgedElement,
                forged_star_crumbs: item.forgedStarCrumbs,
            }, client);
        }

        // 5. Transfer items: B → A
        for (const item of session.itemsB) {
            if (item.quantity >= (await getItemQuantity(client, item.inventoryId))) {
                await client.query('DELETE FROM character_inventory WHERE inventory_id = $1', [item.inventoryId]);
            } else {
                await client.query('UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2', [item.quantity, item.inventoryId]);
            }
            await addFullItemToInventory(session.playerA.characterId, item.itemId, item.quantity, {
                identified: item.identified,
                refine_level: item.refineLevel,
                compounded_cards: JSON.stringify(item.compoundedCards || []),
                forged_by: item.forgedBy,
                forged_element: item.forgedElement,
                forged_star_crumbs: item.forgedStarCrumbs,
            }, client);
        }

        // 6. Transfer Zeny
        if (session.zenyA > 0) {
            await client.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2', [session.zenyA, session.playerA.characterId]);
            await client.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [session.zenyA, session.playerB.characterId]);
            playerA.zuzucoin -= session.zenyA;
            playerB.zuzucoin += session.zenyA;
        }
        if (session.zenyB > 0) {
            await client.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2', [session.zenyB, session.playerB.characterId]);
            await client.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [session.zenyB, session.playerA.characterId]);
            playerB.zuzucoin -= session.zenyB;
            playerA.zuzucoin += session.zenyB;
        }

        // 7. Log the trade
        await client.query(`
            INSERT INTO trade_logs (player_a_id, player_b_id, items_a_gave, items_b_gave, zeny_a_gave, zeny_b_gave, zone_name)
            VALUES ($1, $2, $3, $4, $5, $6, $7)
        `, [
            session.playerA.characterId, session.playerB.characterId,
            JSON.stringify(session.itemsA.map(i => ({ itemId: i.itemId, name: i.name, quantity: i.quantity, refineLevel: i.refineLevel, cards: i.compoundedCards }))),
            JSON.stringify(session.itemsB.map(i => ({ itemId: i.itemId, name: i.name, quantity: i.quantity, refineLevel: i.refineLevel, cards: i.compoundedCards }))),
            session.zenyA, session.zenyB, playerA.zone
        ]);

        await client.query('COMMIT');

        // 8. Update weight caches
        playerA.currentWeight = await calculatePlayerCurrentWeight(session.playerA.characterId);
        playerB.currentWeight = await calculatePlayerCurrentWeight(session.playerB.characterId);

        // 9. Notify both — trade complete
        if (socketA) {
            socketA.emit('trade:completed', {
                receivedItems: session.itemsB, receivedZeny: session.zenyB,
                gaveItems: session.itemsA, gaveZeny: session.zenyA
            });
        }
        if (socketB) {
            socketB.emit('trade:completed', {
                receivedItems: session.itemsA, receivedZeny: session.zenyA,
                gaveItems: session.itemsB, gaveZeny: session.zenyB
            });
        }

        // 10. Cleanup trade state
        cleanupTrade(session);

        // 11. Refresh inventories for both
        if (socketA) await sendPlayerInventory(socketA, session.playerA.characterId);
        if (socketB) await sendPlayerInventory(socketB, session.playerB.characterId);

    } catch (err) {
        await client.query('ROLLBACK');
        console.error('trade:confirm error:', err);
        cancelTrade(session, 'Trade failed — server error');
    } finally {
        client.release();
    }
});
```

### Helper: `cancelTrade(session, reason)`

```javascript
function cancelTrade(session, reason) {
    const socketA = io.sockets.sockets.get(session.playerA.socketId);
    const socketB = io.sockets.sockets.get(session.playerB.socketId);
    if (socketA) socketA.emit('trade:cancelled', { reason });
    if (socketB) socketB.emit('trade:cancelled', { reason });
    cleanupTrade(session);
}

function cleanupTrade(session) {
    // Remove from activeTrades
    activeTrades.delete(session.playerA.characterId);
    activeTrades.delete(session.playerB.characterId);

    // Reset player state
    const playerA = getPlayerByCharacterId(session.playerA.characterId);
    const playerB = getPlayerByCharacterId(session.playerB.characterId);
    if (playerA) { playerA.isTrading = false; playerA.tradePartnerId = null; }
    if (playerB) { playerB.isTrading = false; playerB.tradePartnerId = null; }
}

function getPlayerByCharacterId(characterId) {
    for (const [, p] of connectedPlayers.entries()) {
        if (p.characterId === characterId) return p;
    }
    return null;
}

async function getItemQuantity(client, inventoryId) {
    const r = await client.query('SELECT quantity FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
    return r.rows.length > 0 ? r.rows[0].quantity : 0;
}
```

### Auto-Cancel on Disconnect / Zone Change / Death

```javascript
// In disconnect handler:
if (player.isTrading) {
    const session = activeTrades.get(player.characterId);
    if (session) cancelTrade(session, 'Player disconnected');
}
// Also clean up any pending requests TO this player:
pendingTradeRequests.delete(player.characterId);

// In zone:change handler:
if (player.isTrading) {
    const session = activeTrades.get(player.characterId);
    if (session) cancelTrade(session, 'Player changed zones');
}

// In death handler:
if (player.isTrading) {
    const session = activeTrades.get(player.characterId);
    if (session) cancelTrade(session, 'Player died');
}
```

---

## B4. Client Implementation

### B4.1 New Files to Create

| File | Purpose |
|------|---------|
| `UI/TradeSubsystem.h` | UWorldSubsystem — trade state machine, socket handlers, public API |
| `UI/TradeSubsystem.cpp` | Implementation |
| `UI/STradeWidget.h` | Slate widget — dual-panel trade window |
| `UI/STradeWidget.cpp` | Implementation |
| `UI/STradeRequestPopup.h` | Accept/Decline popup for incoming trade requests |
| `UI/STradeRequestPopup.cpp` | Implementation |

### B4.2 CharacterData.h Updates

```cpp
// Add to EItemDragSource:
Trade   // Items dragged from trade window (to remove)

// Add to EItemDropTarget:
TradeSlot   // Items dropped into trade window (to add)
```

### B4.3 TradeSubsystem

```cpp
// TradeSubsystem.h
UENUM()
enum class ETradeState : uint8
{
    None,           // Not trading
    PendingSent,    // We sent a request, waiting for response
    PendingReceived,// We received a request, showing Accept/Decline popup
    Open,           // Trade window open, adding items
    Locked,         // We clicked OK
    Confirmed,      // We clicked Trade (waiting for partner)
};

USTRUCT()
struct FTradeItem
{
    GENERATED_BODY()
    // Same fields as trade:item_added payload (full item data)
    int32 InventoryId = 0;
    int32 ItemId = 0;
    FString Name;
    int32 Quantity = 0;
    int32 Weight = 0;
    int32 RefineLevel = 0;
    TArray<int32> CompoundedCards;
    bool bIdentified = true;
    FString Icon;
    FString ItemType;
    FString EquipSlot;
    int32 Slots = 0;
    // ... all item display fields
};

UCLASS()
class UTradeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // State
    ETradeState State = ETradeState::None;
    int32 PartnerId = 0;
    FString PartnerName;

    // Trade contents
    TArray<FTradeItem> MyItems;      // Our offered items (0-9)
    TArray<FTradeItem> PartnerItems; // Partner's offered items (0-9)
    int32 MyZeny = 0;
    int32 PartnerZeny = 0;
    bool bPartnerLocked = false;

    // Pending request
    int32 PendingRequestFromId = 0;
    FString PendingRequestFromName;

    uint32 DataVersion = 0;

    // Public API
    void RequestTrade(int32 TargetCharacterId);
    void AcceptRequest();
    void DeclineRequest();
    void AddItem(int32 InventoryId, int32 Quantity);
    void RemoveItem(int32 TradeSlot);
    void SetZeny(int32 Amount);
    void Lock();       // OK button
    void Confirm();    // Trade button
    void Cancel();

    // Widget
    void ShowTradeWidget();
    void HideTradeWidget();
    void ShowRequestPopup();
    void HideRequestPopup();
    bool IsTradeWidgetVisible() const;

private:
    void HandleRequestReceived(TSharedPtr<FJsonObject> Data);
    void HandleTradeOpened(TSharedPtr<FJsonObject> Data);
    void HandleItemAdded(TSharedPtr<FJsonObject> Data);
    void HandleItemRemoved(TSharedPtr<FJsonObject> Data);
    void HandleZenyUpdated(TSharedPtr<FJsonObject> Data);
    void HandlePartnerLocked(TSharedPtr<FJsonObject> Data);
    void HandlePartnerUnlocked(TSharedPtr<FJsonObject> Data);
    void HandleBothLocked(TSharedPtr<FJsonObject> Data);
    void HandleCompleted(TSharedPtr<FJsonObject> Data);
    void HandleCancelled(TSharedPtr<FJsonObject> Data);
    void HandleError(TSharedPtr<FJsonObject> Data);

    void ResetTradeState();

    TSharedPtr<STradeWidget> TradeWidget;
    TSharedPtr<SWidget> TradeOverlay;
    TSharedPtr<STradeRequestPopup> RequestPopup;
    TSharedPtr<SWidget> RequestOverlay;
    bool bEventsWrapped = false;
    FTimerHandle WrapTimerHandle;
};
```

### B4.4 STradeWidget Layout

```
┌──────────────────────────────────────────────────────────────┐
│  ═══ Trade with {PartnerName} ═══                      [X]  │  ← Title bar
├────────────────────────────┬─────────────────────────────────┤
│        Your Offer          │       Partner's Offer           │
├────────────────────────────┼─────────────────────────────────┤
│  ┌────┬────┬────┬────┬────┐│  ┌────┬────┬────┬────┬────┐    │
│  │ 1  │ 2  │ 3  │ 4  │ 5  ││  │ 1  │ 2  │ 3  │ 4  │ 5  │    │  ← 5x2 grid per side
│  ├────┼────┼────┼────┼────┤│  ├────┼────┼────┼────┼────┤    │    (10 slots each)
│  │ 6  │ 7  │ 8  │ 9  │ 10 ││  │ 6  │ 7  │ 8  │ 9  │ 10 │    │
│  └────┴────┴────┴────┴────┘│  └────┴────┴────┴────┴────┘    │
│                            │                                 │
│  Zeny: [________] z        │  Zeny: 50,000 z                │  ← Editable vs readonly
├────────────────────────────┴─────────────────────────────────┤
│   [OK]                              [Cancel]                 │  ← Bottom buttons
│   (or [Trade] when both locked)                              │
└──────────────────────────────────────────────────────────────┘
```

- Z-Order: **22** (same as VendingSetup/Browse popups)
- Left panel: YOUR items (draggable in from inventory, right-click to remove)
- Right panel: PARTNER items (read-only, right-click to inspect)
- Zeny field: Editable SEditableTextBox on your side, readonly on partner side
- OK button: Locks your side. Changes to "Locked" text. Partner sees lock indicator.
- Trade button: Only visible when both locked. Executes trade.
- Cancel button: Always available. Closes trade entirely.
- Drag-drop: Accept items from `EItemDragSource::Inventory` only (not equipment, not cart)
- Right-click on your item: Remove from trade
- Right-click on partner item: Open ItemInspect popup (view details)
- Lock indicator: Green checkmark or "LOCKED" badge on the side that clicked OK

### B4.5 STradeRequestPopup

Small centered popup when receiving a trade request:

```
┌──────────────────────────────────┐
│  ═══ Trade Request ═══          │
│                                  │
│  {PlayerName} wants to trade     │
│  with you.                       │
│                                  │
│   [Accept]      [Decline]        │
└──────────────────────────────────┘
```

- Z-Order: **30** (above most UI, below loading overlay)
- Auto-dismiss after 30 seconds (matches server timeout)
- Brown/gold RO theme
- Accept → `TradeSubsystem->AcceptRequest()`
- Decline → `TradeSubsystem->DeclineRequest()`

### B4.6 PlayerInputSubsystem Updates

Add right-click context menu on other players for trade:

```cpp
// In OnRightClickFromCharacter() or existing right-click handler:
// After checking for vending player:

// Check if clicked actor is another player
int32 ClickedPlayerId = OtherPlayerSub->GetPlayerIdFromActor(HitActor);
if (ClickedPlayerId > 0) {
    // Show context menu with "Trade" option
    FMenuBuilder MenuBuilder(true, nullptr);

    const FPlayerEntry* PE = OtherPlayerSub->GetPlayerData(ClickedPlayerId);
    if (PE && !PE->bIsVending) {
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Trade")),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([this, ClickedPlayerId]() {
                if (UTradeSubsystem* TradeSub = GetWorld()->GetSubsystem<UTradeSubsystem>()) {
                    TradeSub->RequestTrade(ClickedPlayerId);
                }
            }))
        );
    }

    // Push context menu at cursor position
    FSlateApplication::Get().PushMenu(...);
}
```

Also add `/trade PlayerName` chat command support in ChatSubsystem.

### B4.7 InventorySubsystem Updates

Handle drag to trade window:

```cpp
// In CompleteDrop():
case EItemDropTarget::TradeSlot:
    if (DragState.Source == EItemDragSource::Inventory) {
        if (UTradeSubsystem* TradeSub = GetWorld()->GetSubsystem<UTradeSubsystem>()) {
            TradeSub->AddItem(DragState.InventoryId, DragState.Quantity);
        }
    }
    break;
```

### B4.8 ChatSubsystem Updates

Add trade-related system messages:

```cpp
// Parse "/trade PlayerName" in chat input:
if (Message.StartsWith(TEXT("/trade "))) {
    FString TargetName = Message.Mid(7).TrimStartAndEnd();
    // Look up character ID from OtherPlayerSubsystem by name
    // Call TradeSubsystem->RequestTrade(characterId)
    return; // Don't send as chat message
}
```

Trade events → system messages in chat:
- `"[System] Trade request sent to {name}"`
- `"[System] {name} wants to trade with you"`
- `"[System] Trade accepted"`
- `"[System] Trade cancelled"`
- `"[System] Trade completed successfully"`

---

## B5. Integration Points

| Existing System | Change Required |
|----------------|-----------------|
| **CharacterData.h** | Add `Trade` to `EItemDragSource`, `TradeSlot` to `EItemDropTarget` |
| **PlayerInputSubsystem** | Add right-click context menu on other players with "Trade" option |
| **InventorySubsystem** | Handle `TradeSlot` drop target in `CompleteDrop()` |
| **ChatSubsystem** | Parse `/trade PlayerName` command, display trade system messages |
| **OtherPlayerSubsystem** | No changes needed — already tracks player names and IDs |
| **VendingSubsystem** | No changes — `isTrading` check already prevents vending during trade (server-side) |
| **StorageSubsystem** | `isTrading` check prevents storage during trade (server-side) |
| **NameTagSubsystem** | Optional: show "Trading" indicator above player name |
| **server/src/index.js** | Add `TRADE` constants, `isTrading`/`tradePartnerId` state, `pendingTradeRequests` Map, `activeTrades` Map, `TradeSession` class, 9 socket handlers, auto-cancel in disconnect/zone-change/death, auto-create table at startup |
| **database/migrations/** | New file: `add_trade_system.sql` |
| **Weight system** | Weight checked per item add AND at final commit |
| **Disconnect handler** | Cancel active trade + cleanup pending requests |
| **Death handler** | Cancel active trade |
| **Zone change handler** | Cancel active trade |
| **Combat handlers** | Block combat:attack if isTrading (optional, RO allows fighting during trade) |

---

## B6. Implementation Checklist

### Database
- [ ] Create `database/migrations/add_trade_system.sql`
- [ ] Add auto-create block for `trade_logs` table + indexes

### Server (index.js)
- [ ] Add `TRADE` constants (MAX_ITEMS, MAX_ZENY, DISTANCE)
- [ ] Add `TradeSession` class
- [ ] Add `activeTrades` Map, `pendingTradeRequests` Map
- [ ] Add `player.isTrading = false`, `player.tradePartnerId = null` to player init
- [ ] Implement `trade:request` handler (distance, state validation, pending request storage, 30s timeout)
- [ ] Implement `trade:accept` handler (create session, mark both as trading)
- [ ] Implement `trade:decline` handler
- [ ] Implement `trade:add_item` handler (validation, weight check, lock reset, partner notify)
- [ ] Implement `trade:remove_item` handler (lock reset, partner notify)
- [ ] Implement `trade:set_zeny` handler (overflow check, lock reset)
- [ ] Implement `trade:lock` handler (set locked, notify partner, check both_locked)
- [ ] Implement `trade:confirm` handler (atomic DB transaction, re-validate everything, transfer items+zeny, log, cleanup)
- [ ] Implement `trade:cancel` handler
- [ ] Implement `cancelTrade()`, `cleanupTrade()`, `getPlayerByCharacterId()` helpers
- [ ] Add `isTrading` cancel to disconnect handler
- [ ] Add `isTrading` cancel to zone:change handler
- [ ] Add `isTrading` cancel to death handler
- [ ] Add `isTrading` check to vending:start handler
- [ ] Add `isTrading` check to storage:open handler
- [ ] Clean up pending requests on disconnect

### Client — CharacterData.h
- [ ] Add `Trade` to `EItemDragSource`
- [ ] Add `TradeSlot` to `EItemDropTarget`

### Client — TradeSubsystem
- [ ] Create `TradeSubsystem.h` — class, state enum, trade items, public API
- [ ] Create `TradeSubsystem.cpp` — full implementation
- [ ] `ShouldCreateSubsystem()` — game worlds only
- [ ] `OnWorldBeginPlay()` — register 11 socket event handlers
- [ ] `Deinitialize()` — unregister, hide widgets, reset state
- [ ] `HandleRequestReceived()` → show request popup
- [ ] `HandleTradeOpened()` → set state, show trade widget
- [ ] `HandleItemAdded()` → add to MyItems or PartnerItems based on `side`
- [ ] `HandleItemRemoved()` → remove from appropriate array
- [ ] `HandleZenyUpdated()` → update MyZeny or PartnerZeny
- [ ] `HandlePartnerLocked()` → set bPartnerLocked, update UI
- [ ] `HandlePartnerUnlocked()` → clear locks, update UI
- [ ] `HandleBothLocked()` → enable Trade button
- [ ] `HandleCompleted()` → show summary, close trade widget, system message
- [ ] `HandleCancelled()` → close trade widget, system message
- [ ] `HandleError()` → display error in chat
- [ ] `RequestTrade()` → emit trade:request
- [ ] `AcceptRequest()` → emit trade:accept, hide popup
- [ ] `DeclineRequest()` → emit trade:decline, hide popup
- [ ] `AddItem()` → emit trade:add_item
- [ ] `RemoveItem()` → emit trade:remove_item
- [ ] `SetZeny()` → emit trade:set_zeny
- [ ] `Lock()` → emit trade:lock, set state to Locked
- [ ] `Confirm()` → emit trade:confirm, set state to Confirmed
- [ ] `Cancel()` → emit trade:cancel

### Client — STradeWidget
- [ ] Create `STradeWidget.h` — layout, dual-panel
- [ ] Create `STradeWidget.cpp` — full implementation
- [ ] Title bar ("Trade with {PartnerName}") + close button
- [ ] Left panel: "Your Offer" — 5x2 grid (10 slots), drag-drop target from inventory
- [ ] Right panel: "Partner's Offer" — 5x2 grid (10 slots), read-only display
- [ ] Zeny input field (editable, your side) + display (read-only, partner side)
- [ ] OK / Trade / Cancel buttons with state-dependent visibility
- [ ] Lock indicator (visual badge when locked)
- [ ] Drag-drop input handling (OnMouseButtonDown/Up/Move)
- [ ] Right-click on your item → remove from trade
- [ ] Right-click on partner item → ItemInspect popup
- [ ] Item tooltip on hover
- [ ] Tick → poll DataVersion for rebuild
- [ ] Brown/gold RO theme

### Client — STradeRequestPopup
- [ ] Create `STradeRequestPopup.h` — small centered popup
- [ ] Create `STradeRequestPopup.cpp`
- [ ] "{PlayerName} wants to trade with you"
- [ ] Accept / Decline buttons
- [ ] 30-second auto-dismiss timer
- [ ] Z-Order 30

### Client — Integration Updates
- [ ] **PlayerInputSubsystem**: Add right-click context menu on other players with "Trade"
- [ ] **InventorySubsystem**: Add `TradeSlot` handling in `CompleteDrop()`
- [ ] **ChatSubsystem**: Parse `/trade PlayerName` command
- [ ] **ChatSubsystem**: Display trade system messages via `AddCombatLogMessage()`

### Testing
- [ ] Right-click player → "Trade" option appears
- [ ] `/trade PlayerName` chat command works
- [ ] Request popup appears for target player
- [ ] Accept → both trade windows open
- [ ] Decline → requester sees decline message
- [ ] Add items via drag-drop from inventory
- [ ] Add items → appear on partner's right panel
- [ ] Remove item → disappears from both sides
- [ ] Set Zeny → partner sees updated amount
- [ ] Click OK → partner sees lock indicator
- [ ] Both click OK → Trade button becomes active
- [ ] Modify items after OK → locks reset for BOTH players (anti-scam)
- [ ] Click Trade → items transfer, Zeny transfers, both inventories refresh
- [ ] Trade with refined+carded equipment → attributes preserved
- [ ] Cannot add equipped items
- [ ] Cannot add NoTrade items
- [ ] Weight check: partner would be overweight → error
- [ ] Zeny overflow: partner at max zeny → error
- [ ] Cancel trade → both windows close, items returned
- [ ] Disconnect during trade → auto-cancel
- [ ] Zone change during trade → auto-cancel
- [ ] Death during trade → auto-cancel
- [ ] Cannot open storage while trading
- [ ] Cannot start vending while trading
- [ ] Request times out after 30 seconds
- [ ] trade_logs table records completed trade

---

# Shared Infrastructure

## `validatePlayerState()` Helper (Server)

Create a reusable state check used by ALL economy handlers:

```javascript
function validatePlayerState(player, action) {
    if (player.isDead) return `Cannot ${action} while dead`;
    if (player.isVending) return `Cannot ${action} while vending`;
    if (player.isTrading) return `Cannot ${action} while trading`;
    if (player.isStorageOpen) return `Cannot ${action} while storage is open`;
    return null; // All clear
}
```

Usage:
```javascript
const err = validatePlayerState(player, 'trade');
if (err) return socket.emit('trade:error', { message: err });
```

---

# Supporting System Updates

## Systems That Need `isTrading` / `isStorageOpen` Guards

| Handler | Add Check |
|---------|-----------|
| `storage:open` | Block if `isTrading` or `isVending` |
| `trade:request` | Block if `isStorageOpen` or `isVending` |
| `vending:start` | Block if `isTrading` or `isStorageOpen` |
| `kafra:teleport` | Close storage if open, cancel trade if active |
| `inventory:equip` | Block if `isTrading` (can't equip during trade) |
| `inventory:drop` | Block if `isTrading` (can't drop during trade) |
| `inventory:use` | Block if `isTrading` (can't use items during trade) |
| `combat:attack` | Optional: allow during trade (RO allows it) |
| `player:join` disconnect cleanup | Close storage + cancel trade |
| `zone:change` | Close storage + cancel trade |
| Death handler | Cancel trade (storage doesn't close on death in RO, but we can) |

## Weight System Integration

Both storage and trade call `calculatePlayerCurrentWeight()` after any item transfer completes, then refresh inventory via `sendPlayerInventory()`. The weight:status event is already emitted as part of `sendPlayerInventory()`.

## Build.cs

No new module dependencies required. Both subsystems use the same modules already included.

---

# File Summary

| New File | System | Lines Est. |
|----------|--------|-----------|
| `database/migrations/add_account_storage.sql` | Storage | ~20 |
| `database/migrations/add_trade_system.sql` | Trade | ~20 |
| `client/.../UI/StorageSubsystem.h` | Storage | ~80 |
| `client/.../UI/StorageSubsystem.cpp` | Storage | ~350 |
| `client/.../UI/SStorageWidget.h` | Storage | ~80 |
| `client/.../UI/SStorageWidget.cpp` | Storage | ~700 |
| `client/.../UI/TradeSubsystem.h` | Trade | ~100 |
| `client/.../UI/TradeSubsystem.cpp` | Trade | ~500 |
| `client/.../UI/STradeWidget.h` | Trade | ~90 |
| `client/.../UI/STradeWidget.cpp` | Trade | ~800 |
| `client/.../UI/STradeRequestPopup.h` | Trade | ~30 |
| `client/.../UI/STradeRequestPopup.cpp` | Trade | ~120 |

| Modified File | Changes |
|---------------|---------|
| `server/src/index.js` | ~800 lines: constants, state init, 15 socket handlers, helpers, auto-create, guards |
| `client/.../CharacterData.h` | ~80 lines: FStorageItem struct, enum additions |
| `client/.../UI/SKafraWidget.cpp` | ~10 lines: add "Use Storage" button |
| `client/.../UI/InventorySubsystem.cpp` | ~20 lines: handle Storage/Trade drop targets |
| `client/.../UI/PlayerInputSubsystem.cpp` | ~40 lines: right-click context menu for trade |
| `client/.../UI/ChatSubsystem.cpp` | ~15 lines: /trade command parsing |
