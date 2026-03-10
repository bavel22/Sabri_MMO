# 12. Economy, Vending, Storage & Trading — UE5 C++ Implementation Guide

> **Scope**: Zeny management, player vending, buying stores, Kafra storage, ground items, NPC shop enhancements, and anti-dupe security.
> **Stack**: UE5.7 C++ (Slate UI, UWorldSubsystem) | Node.js + Socket.io | PostgreSQL
> **Source spec**: `RagnaCloneDocs/13_Economy_Trading_Vending.md`

---

## Table of Contents

1. [Zeny Management](#1-zeny-management)
2. [Vending System](#2-vending-system)
3. [Buying Store](#3-buying-store)
4. [Storage System](#4-storage-system)
5. [Ground Item System](#5-ground-item-system)
6. [NPC Shop Enhancement](#6-npc-shop-enhancement)
7. [Anti-Dupe & Security](#7-anti-dupe--security)

---

## 1. Zeny Management

### 1.1 Database Schema

The `characters` table already has a `zuzucoin` column (INTEGER). For RO-faithful overflow protection, upgrade to BIGINT and add a transaction log.

```sql
-- Migration: database/migrations/add_zeny_bigint_and_logs.sql

-- 1. Upgrade zuzucoin to BIGINT for safe arithmetic (prevents JS Number overflow)
ALTER TABLE characters ALTER COLUMN zuzucoin TYPE BIGINT USING zuzucoin::BIGINT;
ALTER TABLE characters ALTER COLUMN zuzucoin SET DEFAULT 0;

-- 2. Audit trail for all zeny-modifying operations
CREATE TABLE zeny_transactions (
    transaction_id  SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    transaction_type VARCHAR(30) NOT NULL,  -- 'monster_kill', 'npc_buy', 'npc_sell', 'vend_sale', 'vend_purchase', 'trade', 'storage_fee', 'kafra_teleport', 'refine', 'pickup'
    amount          BIGINT NOT NULL,        -- positive = gain, negative = spend
    balance_after   BIGINT NOT NULL,
    counterpart_id  INTEGER DEFAULT NULL,   -- other character_id (trade/vend) or NULL
    metadata        JSONB DEFAULT '{}',     -- { itemId, itemName, shopId, etc. }
    zone_name       VARCHAR(50) DEFAULT NULL,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_zeny_tx_char ON zeny_transactions(character_id);
CREATE INDEX idx_zeny_tx_type ON zeny_transactions(transaction_type);
CREATE INDEX idx_zeny_tx_time ON zeny_transactions(created_at);
```

### 1.2 Server Constants & Helpers

Add these constants and helper functions to `server/src/index.js`:

```javascript
// ============================================================
// Zeny Constants (RO Classic)
// ============================================================
const ZENY = {
    MAX_PER_CHARACTER: 2147483647,  // 2^31 - 1 (signed 32-bit)
    MAX_PER_TRADE:     999999999,
    MAX_VEND_PRICE:    1000000000,
    MIN:               0,
    VEND_TAX_THRESHOLD: 10000000,   // 10M — items above this get 5% tax
    VEND_TAX_RATE:     0.05,
};

// ============================================================
// Atomic Zeny Modification — ALL zeny changes MUST go through this
// ============================================================
async function modifyZeny(characterId, amount, transactionType, metadata = {}, counterpartId = null, zoneName = null) {
    // amount: positive = gain, negative = spend
    // Returns { success, newBalance, error }

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // Lock the row to prevent concurrent modifications
        const lockResult = await client.query(
            'SELECT zuzucoin FROM characters WHERE character_id = $1 FOR UPDATE',
            [characterId]
        );
        if (lockResult.rows.length === 0) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Character not found' };
        }

        const currentZeny = parseInt(lockResult.rows[0].zuzucoin) || 0;
        const newBalance = currentZeny + amount;

        // Overflow protection
        if (newBalance > ZENY.MAX_PER_CHARACTER) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Zeny overflow — would exceed 2,147,483,647' };
        }
        if (newBalance < ZENY.MIN) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Insufficient Zeny' };
        }

        // Apply the change
        await client.query(
            'UPDATE characters SET zuzucoin = $1 WHERE character_id = $2',
            [newBalance, characterId]
        );

        // Log the transaction
        await client.query(
            `INSERT INTO zeny_transactions
             (character_id, transaction_type, amount, balance_after, counterpart_id, metadata, zone_name)
             VALUES ($1, $2, $3, $4, $5, $6, $7)`,
            [characterId, transactionType, amount, newBalance,
             counterpartId, JSON.stringify(metadata), zoneName]
        );

        await client.query('COMMIT');

        return { success: true, newBalance };
    } catch (err) {
        await client.query('ROLLBACK');
        logger.error(`[ZENY] modifyZeny failed for char ${characterId}: ${err.message}`);
        return { success: false, error: 'Transaction failed' };
    } finally {
        client.release();
    }
}

// Validate that a zeny amount is safe to receive (won't overflow)
function canReceiveZeny(currentZeny, amount) {
    return (currentZeny + amount) <= ZENY.MAX_PER_CHARACTER;
}

// Validate that a zeny amount is safe to spend
function canSpendZeny(currentZeny, amount) {
    return currentZeny >= amount && amount >= 0;
}
```

### 1.3 Zeny Sources (Server Integration Points)

Every place zeny is gained or spent must call `modifyZeny()`. Replace direct SQL updates:

```javascript
// BEFORE (unsafe — no overflow check, no logging):
// await pool.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [amount, charId]);

// AFTER (safe):
const result = await modifyZeny(characterId, amount, 'monster_kill', { enemyId, enemyName }, null, zoneName);
if (!result.success) {
    logger.warn(`[ZENY] Failed to award kill zeny: ${result.error}`);
}
// Update in-memory player state
player.zuzucoin = result.newBalance;
```

**Gain sources requiring `modifyZeny()`:**
| Source | transaction_type | Where in index.js |
|--------|-----------------|-------------------|
| Monster kill zeny | `'monster_kill'` | Enemy death handler |
| NPC sell | `'npc_sell'` | `shop:sell_batch` |
| Vending sale revenue | `'vend_sale'` | `vend:buy` handler |
| Trade received | `'trade_receive'` | `trade:confirm` handler |
| Ground item pickup (zeny drops) | `'pickup'` | `item:pickup` handler |

**Spend sources requiring `modifyZeny()`:**
| Source | transaction_type | Where in index.js |
|--------|-----------------|-------------------|
| NPC buy | `'npc_buy'` | `shop:buy_batch` |
| Vending purchase | `'vend_purchase'` | `vend:buy` handler |
| Trade sent | `'trade_send'` | `trade:confirm` handler |
| Kafra teleport | `'kafra_teleport'` | `kafra:teleport` handler |
| Storage access fee | `'storage_fee'` | `storage:open` handler |
| Refinement cost | `'refine'` | Future refine handler |

### 1.4 Client Zeny Display

`BasicInfoSubsystem` already tracks `Zuzucoin` (int32). Update `SBasicInfoWidget` to format with commas:

```cpp
// In SBasicInfoWidget — zeny display label
SNew(STextBlock)
.Text_Lambda([Sub]() -> FText
{
    if (!Sub) return FText::AsNumber(0);
    return FText::AsNumber(Sub->Zuzucoin);  // FText::AsNumber auto-formats with locale separators
})
.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.82f, 0.48f, 1.f)))
```

No new subsystem needed for zeny display alone — `BasicInfoSubsystem` already handles `shop:bought`, `shop:sold`, `inventory:data`, and `player:joined` events that carry zeny updates.

---

## 2. Vending System

### 2.1 Database Schema

```sql
-- Migration: database/migrations/add_vending_system.sql

-- Active vending shops (in-memory is primary, DB is persistence for crash recovery)
CREATE TABLE vending_shops (
    shop_id         SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    shop_title      VARCHAR(80) NOT NULL,
    zone_name       VARCHAR(50) NOT NULL,
    position_x      FLOAT NOT NULL,
    position_y      FLOAT NOT NULL,
    position_z      FLOAT NOT NULL,
    items           JSONB NOT NULL DEFAULT '[]',
    -- items format: [{ inventoryId, itemId, itemName, quantity, pricePerUnit, icon }]
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (character_id)  -- one shop per character
);
CREATE INDEX idx_vending_zone ON vending_shops(zone_name);

-- Vend sale log (separate from zeny_transactions for detailed per-item tracking)
CREATE TABLE vend_sale_logs (
    sale_id         SERIAL PRIMARY KEY,
    shop_character_id INTEGER NOT NULL,
    buyer_character_id INTEGER NOT NULL,
    item_id         INTEGER NOT NULL,
    item_name       VARCHAR(100) NOT NULL,
    quantity        INTEGER NOT NULL,
    price_per_unit  BIGINT NOT NULL,
    total_price     BIGINT NOT NULL,
    tax_amount      BIGINT NOT NULL DEFAULT 0,
    vendor_revenue  BIGINT NOT NULL,
    zone_name       VARCHAR(50),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_vend_log_seller ON vend_sale_logs(shop_character_id);
CREATE INDEX idx_vend_log_buyer ON vend_sale_logs(buyer_character_id);
```

### 2.2 Server: In-Memory Vending State

```javascript
// ============================================================
// Vending System — In-Memory State
// ============================================================

// Map<characterId, VendShopData>
const activeVendShops = new Map();

// VendShopData structure:
// {
//   characterId: number,
//   characterName: string,
//   shopTitle: string,
//   zoneName: string,
//   positionX: number, positionY: number, positionZ: number,
//   items: [
//     {
//       vendSlot: number,       // 0-based index in the vend list
//       inventoryId: number,    // character_inventory.inventory_id
//       itemId: number,
//       itemName: string,
//       icon: string,
//       quantity: number,       // remaining quantity
//       pricePerUnit: number,
//       weight: number,
//     }
//   ],
//   createdAt: Date,
//   salesLog: [],  // accumulated during session
// }

const VEND = {
    MAX_SLOTS_BASE: 3,          // Vending Lv.1 = 3 stacks
    MAX_SLOTS_PER_LEVEL: 1,     // +1 per level beyond 1
    MAX_PRICE_PER_UNIT: 1000000000,  // 1 billion
    MAX_TITLE_LENGTH: 80,
    MIN_NPC_DISTANCE: 200,      // 4 RO cells = ~200 UE units
    TAX_THRESHOLD: 10000000,    // 10M
    TAX_RATE: 0.05,
};

function getVendMaxSlots(vendingSkillLevel) {
    // Vending Lv.1 = 3, Lv.2 = 4, ... Lv.10 = 12
    return VEND.MAX_SLOTS_BASE + Math.max(0, (vendingSkillLevel || 1) - 1);
}

function calculateVendTax(pricePerUnit, quantity) {
    const totalPrice = pricePerUnit * quantity;
    if (pricePerUnit > VEND.TAX_THRESHOLD) {
        return Math.floor(totalPrice * VEND.TAX_RATE);
    }
    return 0;
}
```

### 2.3 Server: Socket Event Handlers

```javascript
// ============================================================
// Vending Socket Events
// ============================================================

// --- Open a vending shop ---
socket.on('vend:open_shop', async (data) => {
    logger.info(`[RECV] vend:open_shop from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    // 1. Validate player is not already vending
    if (activeVendShops.has(characterId)) {
        socket.emit('vend:error', { message: 'You already have a shop open' });
        return;
    }

    // 2. Validate player is not dead, in trade, etc.
    if (player.isDead) {
        socket.emit('vend:error', { message: 'Cannot vend while dead' });
        return;
    }

    // 3. Validate shop title
    const shopTitle = (data.shopTitle || '').trim().substring(0, VEND.MAX_TITLE_LENGTH);
    if (!shopTitle) {
        socket.emit('vend:error', { message: 'Shop title is required' });
        return;
    }

    // 4. Validate vending skill (simplified — check player skill list)
    const vendingLevel = getPlayerSkillLevel(player, 'MC_VENDING') || 0;
    if (vendingLevel < 1) {
        socket.emit('vend:error', { message: 'Vending skill required' });
        return;
    }

    // 5. Validate SP cost (30 SP)
    if ((player.currentMana || 0) < 30) {
        socket.emit('vend:error', { message: 'Not enough SP (need 30)' });
        return;
    }

    // 6. Validate items array
    const items = data.items;  // [{ inventoryId, pricePerUnit }]
    if (!Array.isArray(items) || items.length === 0) {
        socket.emit('vend:error', { message: 'No items selected for vending' });
        return;
    }

    const maxSlots = getVendMaxSlots(vendingLevel);
    if (items.length > maxSlots) {
        socket.emit('vend:error', { message: `Vending Lv.${vendingLevel} allows max ${maxSlots} item stacks` });
        return;
    }

    // 7. Validate each item exists in player inventory, is not equipped, price is valid
    const vendItems = [];
    const usedInventoryIds = new Set();

    for (let i = 0; i < items.length; i++) {
        const entry = items[i];
        const inventoryId = parseInt(entry.inventoryId);
        const pricePerUnit = parseInt(entry.pricePerUnit);

        if (!inventoryId || isNaN(inventoryId)) {
            socket.emit('vend:error', { message: 'Invalid inventory item' });
            return;
        }
        if (usedInventoryIds.has(inventoryId)) {
            socket.emit('vend:error', { message: 'Duplicate item in vend list' });
            return;
        }
        usedInventoryIds.add(inventoryId);

        if (!pricePerUnit || pricePerUnit < 1 || pricePerUnit > VEND.MAX_PRICE_PER_UNIT) {
            socket.emit('vend:error', { message: `Price must be 1 to ${VEND.MAX_PRICE_PER_UNIT.toLocaleString()}z` });
            return;
        }

        // Verify ownership and not equipped
        try {
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
                        i.name, i.icon, i.weight, i.price
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
            if (result.rows.length === 0) {
                socket.emit('vend:error', { message: 'Item not found in your inventory' });
                return;
            }
            const row = result.rows[0];
            if (row.is_equipped) {
                socket.emit('vend:error', { message: `${row.name} is equipped — unequip first` });
                return;
            }

            vendItems.push({
                vendSlot: i,
                inventoryId: row.inventory_id,
                itemId: row.item_id,
                itemName: row.name,
                icon: row.icon,
                quantity: row.quantity,
                pricePerUnit: pricePerUnit,
                weight: row.weight || 0,
            });
        } catch (err) {
            logger.error(`[VEND] Item validation error: ${err.message}`);
            socket.emit('vend:error', { message: 'Failed to validate items' });
            return;
        }
    }

    // 8. Deduct SP
    player.currentMana = Math.max(0, (player.currentMana || 0) - 30);

    // 9. Create the shop
    const shopData = {
        characterId,
        characterName: player.characterName,
        shopTitle,
        zoneName: player.zoneName,
        positionX: player.lastX || 0,
        positionY: player.lastY || 0,
        positionZ: player.lastZ || 0,
        items: vendItems,
        createdAt: new Date(),
        salesLog: [],
    };
    activeVendShops.set(characterId, shopData);

    // 10. Persist to DB for crash recovery
    try {
        await pool.query(
            `INSERT INTO vending_shops (character_id, shop_title, zone_name, position_x, position_y, position_z, items)
             VALUES ($1, $2, $3, $4, $5, $6, $7)
             ON CONFLICT (character_id) DO UPDATE SET
                shop_title = EXCLUDED.shop_title,
                zone_name = EXCLUDED.zone_name,
                position_x = EXCLUDED.position_x,
                position_y = EXCLUDED.position_y,
                position_z = EXCLUDED.position_z,
                items = EXCLUDED.items,
                created_at = NOW()`,
            [characterId, shopTitle, shopData.zoneName,
             shopData.positionX, shopData.positionY, shopData.positionZ,
             JSON.stringify(vendItems)]
        );
    } catch (err) {
        logger.warn(`[VEND] DB persist failed (non-fatal): ${err.message}`);
    }

    // 11. Mark player as vending (prevents movement, combat, trade)
    player.isVending = true;

    // 12. Notify the vendor
    socket.emit('vend:shop_opened', {
        shopTitle,
        items: vendItems,
        characterId,
    });

    // 13. Broadcast to zone — other players see the shop title billboard
    broadcastToZoneExcept(player.zoneName, socket.id, 'vend:shop_appeared', {
        characterId,
        characterName: player.characterName,
        shopTitle,
        positionX: shopData.positionX,
        positionY: shopData.positionY,
        positionZ: shopData.positionZ,
    });

    logger.info(`[VEND] ${player.characterName} opened shop "${shopTitle}" with ${vendItems.length} items`);
});

// --- Browse a vending shop ---
socket.on('vend:browse', async (data) => {
    logger.info(`[RECV] vend:browse from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;

    const vendorCharId = parseInt(data.vendorCharacterId);
    const shop = activeVendShops.get(vendorCharId);
    if (!shop) {
        socket.emit('vend:error', { message: 'Shop not found or closed' });
        return;
    }

    // Filter out sold-out items (quantity <= 0)
    const availableItems = shop.items.filter(item => item.quantity > 0);

    socket.emit('vend:shop_data', {
        vendorId: vendorCharId,
        vendorName: shop.characterName,
        shopTitle: shop.shopTitle,
        items: availableItems.map(item => ({
            vendSlot: item.vendSlot,
            itemId: item.itemId,
            itemName: item.itemName,
            icon: item.icon,
            quantity: item.quantity,
            pricePerUnit: item.pricePerUnit,
            weight: item.weight,
        })),
    });
});

// --- Buy from a vending shop (ATOMIC) ---
socket.on('vend:buy', async (data) => {
    logger.info(`[RECV] vend:buy from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId: buyerCharId, player: buyer } = playerInfo;

    const vendorCharId = parseInt(data.vendorCharacterId);
    const vendSlot = parseInt(data.vendSlot);
    const requestedQty = Math.max(1, parseInt(data.quantity) || 1);

    // 1. Find the shop
    const shop = activeVendShops.get(vendorCharId);
    if (!shop) {
        socket.emit('vend:error', { message: 'Shop not found or closed' });
        return;
    }

    // 2. Find the item in the shop
    const vendItem = shop.items.find(i => i.vendSlot === vendSlot);
    if (!vendItem || vendItem.quantity <= 0) {
        socket.emit('vend:error', { message: 'Item sold out' });
        return;
    }

    // 3. Clamp quantity to available
    const buyQty = Math.min(requestedQty, vendItem.quantity);
    const totalCost = vendItem.pricePerUnit * buyQty;

    // 4. Validate buyer zeny
    if (!canSpendZeny(buyer.zuzucoin, totalCost)) {
        socket.emit('vend:error', {
            message: `Not enough Zeny (need ${totalCost.toLocaleString()}, have ${buyer.zuzucoin.toLocaleString()})`
        });
        return;
    }

    // 5. Validate buyer weight
    const maxWeight = getPlayerMaxWeight(buyer);
    let currentWeight = 0;
    try {
        const wResult = await pool.query(
            `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as w
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1`, [buyerCharId]
        );
        currentWeight = parseInt(wResult.rows[0].w);
    } catch (err) { /* skip weight check on error */ }

    const addedWeight = (vendItem.weight || 0) * buyQty;
    if (currentWeight + addedWeight > maxWeight) {
        socket.emit('vend:error', { message: 'Would exceed your weight limit' });
        return;
    }

    // 6. Calculate tax
    const taxAmount = calculateVendTax(vendItem.pricePerUnit, buyQty);
    const vendorRevenue = totalCost - taxAmount;

    // 7. Execute atomic transaction
    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // Deduct buyer zeny
        const buyerResult = await modifyZeny(
            buyerCharId, -totalCost, 'vend_purchase',
            { vendorCharId, itemId: vendItem.itemId, itemName: vendItem.itemName, quantity: buyQty },
            vendorCharId, buyer.zoneName
        );
        if (!buyerResult.success) {
            await client.query('ROLLBACK');
            socket.emit('vend:error', { message: buyerResult.error });
            return;
        }
        buyer.zuzucoin = buyerResult.newBalance;

        // Credit vendor zeny (minus tax)
        const vendorResult = await modifyZeny(
            vendorCharId, vendorRevenue, 'vend_sale',
            { buyerCharId, itemId: vendItem.itemId, itemName: vendItem.itemName, quantity: buyQty, tax: taxAmount },
            buyerCharId, shop.zoneName
        );
        // Find vendor's player object to update in-memory state
        const vendorPlayer = findPlayerByCharId(vendorCharId);
        if (vendorResult.success && vendorPlayer) {
            vendorPlayer.player.zuzucoin = vendorResult.newBalance;
        }

        // Reduce vendor's inventory item quantity (or delete if fully sold)
        if (buyQty >= vendItem.quantity) {
            // Fully sold — remove from vendor inventory
            await client.query(
                'DELETE FROM character_inventory WHERE inventory_id = $1 AND character_id = $2',
                [vendItem.inventoryId, vendorCharId]
            );
        } else {
            await client.query(
                'UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2 AND character_id = $3',
                [buyQty, vendItem.inventoryId, vendorCharId]
            );
        }

        // Add items to buyer inventory
        await addItemToInventory(buyerCharId, vendItem.itemId, buyQty);

        // Log the sale
        await client.query(
            `INSERT INTO vend_sale_logs
             (shop_character_id, buyer_character_id, item_id, item_name, quantity,
              price_per_unit, total_price, tax_amount, vendor_revenue, zone_name)
             VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10)`,
            [vendorCharId, buyerCharId, vendItem.itemId, vendItem.itemName, buyQty,
             vendItem.pricePerUnit, totalCost, taxAmount, vendorRevenue, shop.zoneName]
        );

        await client.query('COMMIT');

        // 8. Update in-memory vend shop state
        vendItem.quantity -= buyQty;

        // 9. Record sale in shop's session log
        shop.salesLog.push({
            buyerName: buyer.characterName,
            itemName: vendItem.itemName,
            quantity: buyQty,
            revenue: vendorRevenue,
            timestamp: new Date(),
        });

        // 10. Notify buyer
        socket.emit('vend:buy_success', {
            itemId: vendItem.itemId,
            itemName: vendItem.itemName,
            quantity: buyQty,
            totalCost,
            newZuzucoin: buyer.zuzucoin,
        });

        // Refresh buyer inventory
        const buyerInv = await getPlayerInventory(buyerCharId);
        socket.emit('inventory:data', { items: buyerInv, zuzucoin: buyer.zuzucoin });

        // 11. Notify vendor
        if (vendorPlayer) {
            const vendorSocket = vendorPlayer.socket;
            vendorSocket.emit('vend:item_sold', {
                buyerName: buyer.characterName,
                itemName: vendItem.itemName,
                quantity: buyQty,
                revenue: vendorRevenue,
                tax: taxAmount,
                timestamp: new Date().toISOString(),
                newZuzucoin: vendorPlayer.player.zuzucoin,
            });
        }

        // 12. Auto-close shop if all items sold out
        const allSoldOut = shop.items.every(i => i.quantity <= 0);
        if (allSoldOut) {
            closeVendShop(vendorCharId, 'All items sold');
        }

        logger.info(`[VEND] ${buyer.characterName} bought ${buyQty}x ${vendItem.itemName} from ${shop.characterName} for ${totalCost}z (tax: ${taxAmount}z)`);

    } catch (err) {
        await client.query('ROLLBACK');
        logger.error(`[VEND] Buy transaction failed: ${err.message}`);
        socket.emit('vend:error', { message: 'Purchase failed — please try again' });
    } finally {
        client.release();
    }
});

// --- Close a vending shop ---
socket.on('vend:close_shop', async () => {
    logger.info(`[RECV] vend:close_shop from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    closeVendShop(playerInfo.characterId, 'Manual close');
});

// --- Shared close logic ---
async function closeVendShop(characterId, reason) {
    const shop = activeVendShops.get(characterId);
    if (!shop) return;

    // Remove from active shops
    activeVendShops.delete(characterId);

    // Remove from DB
    try {
        await pool.query('DELETE FROM vending_shops WHERE character_id = $1', [characterId]);
    } catch (err) {
        logger.warn(`[VEND] DB cleanup failed: ${err.message}`);
    }

    // Unmark player
    const vendorPlayer = findPlayerByCharId(characterId);
    if (vendorPlayer) {
        vendorPlayer.player.isVending = false;

        // Send closing report to vendor
        vendorPlayer.socket.emit('vend:shop_report', {
            shopTitle: shop.shopTitle,
            totalSold: shop.salesLog.reduce((sum, s) => sum + s.quantity, 0),
            totalRevenue: shop.salesLog.reduce((sum, s) => sum + s.revenue, 0),
            sales: shop.salesLog,
            reason,
        });
    }

    // Broadcast shop closure to zone
    broadcastToZone(shop.zoneName, 'vend:shop_closed', {
        characterId,
    });

    logger.info(`[VEND] Shop closed for char ${characterId}: ${reason}`);
}

// Close shop on disconnect
// Add to existing disconnect handler:
// if (activeVendShops.has(characterId)) {
//     await closeVendShop(characterId, 'Disconnected');
// }
```

### 2.4 Client: VendingSubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/VendingSubsystem.h`

```cpp
// VendingSubsystem.h — UWorldSubsystem managing vending shop setup,
// browsing other players' shops, purchasing, and shop title billboards.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "VendingSubsystem.generated.h"

class USocketIOClientComponent;
class SVendingSetupWidget;
class SVendingBrowseWidget;

// One item in a vending shop listing
USTRUCT()
struct FVendItem
{
    GENERATED_BODY()

    int32 VendSlot = -1;
    int32 InventoryId = 0;     // Only valid for own shop (setup phase)
    int32 ItemId = 0;
    FString ItemName;
    FString Icon;
    int32 Quantity = 0;
    int64 PricePerUnit = 0;
    int32 Weight = 0;

    bool IsValid() const { return ItemId > 0 && Quantity > 0; }
    int64 GetTotalPrice() const { return PricePerUnit * Quantity; }
};

// A remote vending shop visible on the map (billboard)
USTRUCT()
struct FRemoteVendShop
{
    GENERATED_BODY()

    int32 CharacterId = 0;
    FString CharacterName;
    FString ShopTitle;
    FVector Position = FVector::ZeroVector;
};

// Sale record in the closing report
USTRUCT()
struct FVendSaleRecord
{
    GENERATED_BODY()

    FString BuyerName;
    FString ItemName;
    int32 Quantity = 0;
    int64 Revenue = 0;
    FString Timestamp;
};

UCLASS()
class SABRIMMO_API UVendingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- state (read by widgets) ----
    bool bIsVending = false;              // Local player has a shop open
    FString OwnShopTitle;
    TArray<FVendItem> OwnShopItems;       // Items in our own shop

    // Browsing another player's shop
    bool bIsBrowsing = false;
    int32 BrowsingVendorId = 0;
    FString BrowsingVendorName;
    FString BrowsingShopTitle;
    TArray<FVendItem> BrowsingItems;

    // Remote shops visible on the map
    TArray<FRemoteVendShop> VisibleShops;

    // Last sale notification (vendor side)
    FString LastSaleMessage;
    double LastSaleMessageExpireTime = 0.0;

    // Closing report
    TArray<FVendSaleRecord> ClosingReportSales;
    int32 ClosingReportTotalSold = 0;
    int64 ClosingReportTotalRevenue = 0;
    bool bHasClosingReport = false;

    // Error/status
    FString StatusMessage;
    double StatusExpireTime = 0.0;

    // ---- public API ----
    void RequestOpenShop(const FString& ShopTitle, const TArray<FVendItem>& ItemsToVend);
    void RequestCloseShop();
    void RequestBrowseShop(int32 VendorCharacterId);
    void RequestBuyFromShop(int32 VendorCharacterId, int32 VendSlot, int32 Quantity);
    void CloseBrowseWindow();
    void DismissClosingReport();

    // ---- widget lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void ShowSetupWidget();
    void HideSetupWidget();
    void ShowBrowseWidget();
    void HideBrowseWidget();
    bool IsAnyWidgetVisible() const;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- event handlers ----
    void HandleShopOpened(const TSharedPtr<FJsonValue>& Data);
    void HandleShopData(const TSharedPtr<FJsonValue>& Data);
    void HandleBuySuccess(const TSharedPtr<FJsonValue>& Data);
    void HandleItemSold(const TSharedPtr<FJsonValue>& Data);
    void HandleShopReport(const TSharedPtr<FJsonValue>& Data);
    void HandleShopAppeared(const TSharedPtr<FJsonValue>& Data);
    void HandleShopClosed(const TSharedPtr<FJsonValue>& Data);
    void HandleVendError(const TSharedPtr<FJsonValue>& Data);

    // ---- state ----
    bool bEventsWrapped = false;
    bool bSetupWidgetVisible = false;
    bool bBrowseWidgetVisible = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SVendingSetupWidget> SetupWidget;
    TSharedPtr<SWidget> SetupAlignWrapper;
    TSharedPtr<SWidget> SetupOverlay;

    TSharedPtr<SVendingBrowseWidget> BrowseWidget;
    TSharedPtr<SWidget> BrowseAlignWrapper;
    TSharedPtr<SWidget> BrowseOverlay;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 2.5 Client: VendingSubsystem Implementation

File: `client/SabriMMO/Source/SabriMMO/UI/VendingSubsystem.cpp`

```cpp
// VendingSubsystem.cpp — Manages vending shop state, socket events,
// and SVendingSetupWidget / SVendingBrowseWidget lifecycles.

#include "VendingSubsystem.h"
#include "SVendingSetupWidget.h"
#include "SVendingBrowseWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogVending, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UVendingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UVendingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UVendingSubsystem::TryWrapSocketEvents),
        0.5f, true
    );

    UE_LOG(LogVending, Log, TEXT("VendingSubsystem started — waiting for SocketIO bindings..."));
}

void UVendingSubsystem::Deinitialize()
{
    HideSetupWidget();
    HideBrowseWidget();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    VisibleShops.Empty();

    Super::Deinitialize();
}

// ============================================================
// Socket IO
// ============================================================

USocketIOClientComponent* UVendingSubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
            return Comp;
    }
    return nullptr;
}

void UVendingSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;

    TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
    if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;
    if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

    CachedSIOComponent = SIOComp;

    WrapSingleEvent(TEXT("vend:shop_opened"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleShopOpened(D); });
    WrapSingleEvent(TEXT("vend:shop_data"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleShopData(D); });
    WrapSingleEvent(TEXT("vend:buy_success"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleBuySuccess(D); });
    WrapSingleEvent(TEXT("vend:item_sold"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleItemSold(D); });
    WrapSingleEvent(TEXT("vend:shop_report"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleShopReport(D); });
    WrapSingleEvent(TEXT("vend:shop_appeared"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleShopAppeared(D); });
    WrapSingleEvent(TEXT("vend:shop_closed"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleShopClosed(D); });
    WrapSingleEvent(TEXT("vend:error"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleVendError(D); });

    bEventsWrapped = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    UE_LOG(LogVending, Log, TEXT("VendingSubsystem — events wrapped."));
}

void UVendingSubsystem::WrapSingleEvent(
    const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
    if (!NativeClient.IsValid()) return;

    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
    FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
    if (Existing)
    {
        OriginalCallback = Existing->Function;
    }

    NativeClient->OnEvent(EventName,
        [OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
        {
            if (OriginalCallback) OriginalCallback(Event, Message);
            if (OurHandler) OurHandler(Message);
        },
        TEXT("/"),
        ESIOThreadOverrideOption::USE_GAME_THREAD
    );
}

// ============================================================
// Event Handlers
// ============================================================

void UVendingSubsystem::HandleShopOpened(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString Title;
    Obj->TryGetStringField(TEXT("shopTitle"), Title);
    OwnShopTitle = Title;
    bIsVending = true;

    // Parse items
    OwnShopItems.Empty();
    const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
    {
        for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArr)
        {
            const TSharedPtr<FJsonObject>* ItemObj = nullptr;
            if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObj) && ItemObj)
            {
                FVendItem VI;
                double D = 0;
                (*ItemObj)->TryGetNumberField(TEXT("vendSlot"), D); VI.VendSlot = (int32)D;
                (*ItemObj)->TryGetNumberField(TEXT("itemId"), D); VI.ItemId = (int32)D;
                (*ItemObj)->TryGetStringField(TEXT("itemName"), VI.ItemName);
                (*ItemObj)->TryGetStringField(TEXT("icon"), VI.Icon);
                (*ItemObj)->TryGetNumberField(TEXT("quantity"), D); VI.Quantity = (int32)D;
                (*ItemObj)->TryGetNumberField(TEXT("pricePerUnit"), D); VI.PricePerUnit = (int64)D;
                OwnShopItems.Add(VI);
            }
        }
    }

    HideSetupWidget();

    // Lock movement
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ACharacter* Char = PC->GetCharacter())
            {
                if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
                {
                    MovComp->DisableMovement();
                }
            }
        }
    }

    UE_LOG(LogVending, Log, TEXT("Shop opened: \"%s\" with %d items"), *OwnShopTitle, OwnShopItems.Num());
}

void UVendingSubsystem::HandleShopData(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double VID = 0;
    Obj->TryGetNumberField(TEXT("vendorId"), VID);
    BrowsingVendorId = (int32)VID;
    Obj->TryGetStringField(TEXT("vendorName"), BrowsingVendorName);
    Obj->TryGetStringField(TEXT("shopTitle"), BrowsingShopTitle);

    BrowsingItems.Empty();
    const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
    {
        for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArr)
        {
            const TSharedPtr<FJsonObject>* ItemObj = nullptr;
            if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObj) && ItemObj)
            {
                FVendItem VI;
                double D = 0;
                (*ItemObj)->TryGetNumberField(TEXT("vendSlot"), D); VI.VendSlot = (int32)D;
                (*ItemObj)->TryGetNumberField(TEXT("itemId"), D); VI.ItemId = (int32)D;
                (*ItemObj)->TryGetStringField(TEXT("itemName"), VI.ItemName);
                (*ItemObj)->TryGetStringField(TEXT("icon"), VI.Icon);
                (*ItemObj)->TryGetNumberField(TEXT("quantity"), D); VI.Quantity = (int32)D;
                (*ItemObj)->TryGetNumberField(TEXT("pricePerUnit"), D); VI.PricePerUnit = (int64)D;
                (*ItemObj)->TryGetNumberField(TEXT("weight"), D); VI.Weight = (int32)D;
                BrowsingItems.Add(VI);
            }
        }
    }

    bIsBrowsing = true;
    ShowBrowseWidget();

    UE_LOG(LogVending, Log, TEXT("Browsing shop of %s: \"%s\" (%d items)"),
        *BrowsingVendorName, *BrowsingShopTitle, BrowsingItems.Num());
}

void UVendingSubsystem::HandleBuySuccess(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString ItemName;
    Obj->TryGetStringField(TEXT("itemName"), ItemName);
    double Qty = 0, Cost = 0;
    Obj->TryGetNumberField(TEXT("quantity"), Qty);
    Obj->TryGetNumberField(TEXT("totalCost"), Cost);

    StatusMessage = FString::Printf(TEXT("Bought %dx %s for %sz"), (int32)Qty, *ItemName, *FText::AsNumber((int64)Cost).ToString());
    StatusExpireTime = FPlatformTime::Seconds() + 4.0;

    UE_LOG(LogVending, Log, TEXT("Purchase success: %dx %s for %dz"), (int32)Qty, *ItemName, (int64)Cost);
}

void UVendingSubsystem::HandleItemSold(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FString BuyerName, ItemName;
    Obj->TryGetStringField(TEXT("buyerName"), BuyerName);
    Obj->TryGetStringField(TEXT("itemName"), ItemName);
    double Qty = 0, Rev = 0;
    Obj->TryGetNumberField(TEXT("quantity"), Qty);
    Obj->TryGetNumberField(TEXT("revenue"), Rev);

    LastSaleMessage = FString::Printf(TEXT("%s bought %dx %s (+%sz)"),
        *BuyerName, (int32)Qty, *ItemName, *FText::AsNumber((int64)Rev).ToString());
    LastSaleMessageExpireTime = FPlatformTime::Seconds() + 6.0;

    // Update own shop items quantity
    double Slot = 0;
    // Find the item that was sold and reduce its quantity in our local copy
    for (FVendItem& VI : OwnShopItems)
    {
        if (VI.ItemName == ItemName && VI.Quantity > 0)
        {
            VI.Quantity -= (int32)Qty;
            break;
        }
    }

    UE_LOG(LogVending, Log, TEXT("Item sold: %s bought %dx %s for %dz"), *BuyerName, (int32)Qty, *ItemName, (int64)Rev);
}

void UVendingSubsystem::HandleShopReport(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double TS = 0, TR = 0;
    Obj->TryGetNumberField(TEXT("totalSold"), TS);
    Obj->TryGetNumberField(TEXT("totalRevenue"), TR);
    ClosingReportTotalSold = (int32)TS;
    ClosingReportTotalRevenue = (int64)TR;

    ClosingReportSales.Empty();
    const TArray<TSharedPtr<FJsonValue>>* SalesArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("sales"), SalesArr) && SalesArr)
    {
        for (const TSharedPtr<FJsonValue>& SaleVal : *SalesArr)
        {
            const TSharedPtr<FJsonObject>* SaleObj = nullptr;
            if (SaleVal.IsValid() && SaleVal->TryGetObject(SaleObj) && SaleObj)
            {
                FVendSaleRecord Rec;
                double D = 0;
                (*SaleObj)->TryGetStringField(TEXT("buyerName"), Rec.BuyerName);
                (*SaleObj)->TryGetStringField(TEXT("itemName"), Rec.ItemName);
                (*SaleObj)->TryGetNumberField(TEXT("quantity"), D); Rec.Quantity = (int32)D;
                (*SaleObj)->TryGetNumberField(TEXT("revenue"), D); Rec.Revenue = (int64)D;
                (*SaleObj)->TryGetStringField(TEXT("timestamp"), Rec.Timestamp);
                ClosingReportSales.Add(Rec);
            }
        }
    }

    bIsVending = false;
    bHasClosingReport = true;
    OwnShopItems.Empty();

    // Unlock movement
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ACharacter* Char = PC->GetCharacter())
            {
                if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
                {
                    MovComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
    }

    UE_LOG(LogVending, Log, TEXT("Shop closed — sold %d items for %lldz total"), ClosingReportTotalSold, ClosingReportTotalRevenue);
}

void UVendingSubsystem::HandleShopAppeared(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    FRemoteVendShop Shop;
    double D = 0;
    Obj->TryGetNumberField(TEXT("characterId"), D); Shop.CharacterId = (int32)D;
    Obj->TryGetStringField(TEXT("characterName"), Shop.CharacterName);
    Obj->TryGetStringField(TEXT("shopTitle"), Shop.ShopTitle);
    Obj->TryGetNumberField(TEXT("positionX"), D); Shop.Position.X = D;
    Obj->TryGetNumberField(TEXT("positionY"), D); Shop.Position.Y = D;
    Obj->TryGetNumberField(TEXT("positionZ"), D); Shop.Position.Z = D;

    // Remove existing entry for this character (update)
    VisibleShops.RemoveAll([&](const FRemoteVendShop& S) { return S.CharacterId == Shop.CharacterId; });
    VisibleShops.Add(Shop);

    UE_LOG(LogVending, Log, TEXT("Vend shop appeared: %s \"%s\""), *Shop.CharacterName, *Shop.ShopTitle);
}

void UVendingSubsystem::HandleShopClosed(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    double D = 0;
    (*ObjPtr)->TryGetNumberField(TEXT("characterId"), D);
    int32 CharId = (int32)D;

    VisibleShops.RemoveAll([CharId](const FRemoteVendShop& S) { return S.CharacterId == CharId; });

    // If we were browsing this shop, close the browse window
    if (bIsBrowsing && BrowsingVendorId == CharId)
    {
        CloseBrowseWindow();
        StatusMessage = TEXT("Shop has closed.");
        StatusExpireTime = FPlatformTime::Seconds() + 3.0;
    }

    UE_LOG(LogVending, Log, TEXT("Vend shop closed: charId=%d"), CharId);
}

void UVendingSubsystem::HandleVendError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString Msg;
    (*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
    StatusMessage = Msg;
    StatusExpireTime = FPlatformTime::Seconds() + 4.0;
    UE_LOG(LogVending, Warning, TEXT("Vend error: %s"), *Msg);
}

// ============================================================
// Public API
// ============================================================

void UVendingSubsystem::RequestOpenShop(const FString& ShopTitle, const TArray<FVendItem>& ItemsToVend)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetStringField(TEXT("shopTitle"), ShopTitle);

    TArray<TSharedPtr<FJsonValue>> ItemsArray;
    for (const FVendItem& VI : ItemsToVend)
    {
        TSharedPtr<FJsonObject> ItemObj = MakeShareable(new FJsonObject());
        ItemObj->SetNumberField(TEXT("inventoryId"), VI.InventoryId);
        ItemObj->SetNumberField(TEXT("pricePerUnit"), VI.PricePerUnit);
        ItemsArray.Add(MakeShareable(new FJsonValueObject(ItemObj)));
    }
    Payload->SetArrayField(TEXT("items"), ItemsArray);

    CachedSIOComponent->EmitNative(TEXT("vend:open_shop"), Payload);
    UE_LOG(LogVending, Log, TEXT("Requesting open shop: \"%s\" with %d items"), *ShopTitle, ItemsToVend.Num());
}

void UVendingSubsystem::RequestCloseShop()
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("vend:close_shop"), Payload);
}

void UVendingSubsystem::RequestBrowseShop(int32 VendorCharacterId)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetNumberField(TEXT("vendorCharacterId"), VendorCharacterId);

    CachedSIOComponent->EmitNative(TEXT("vend:browse"), Payload);
}

void UVendingSubsystem::RequestBuyFromShop(int32 VendorCharacterId, int32 VendSlot, int32 Quantity)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetNumberField(TEXT("vendorCharacterId"), VendorCharacterId);
    Payload->SetNumberField(TEXT("vendSlot"), VendSlot);
    Payload->SetNumberField(TEXT("quantity"), Quantity);

    CachedSIOComponent->EmitNative(TEXT("vend:buy"), Payload);
}

void UVendingSubsystem::CloseBrowseWindow()
{
    bIsBrowsing = false;
    BrowsingVendorId = 0;
    BrowsingItems.Empty();
    HideBrowseWidget();
}

void UVendingSubsystem::DismissClosingReport()
{
    bHasClosingReport = false;
    ClosingReportSales.Empty();
}

// ============================================================
// Widget Lifecycle
// ============================================================

void UVendingSubsystem::ShowSetupWidget()
{
    if (bSetupWidgetVisible) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    SetupWidget = SNew(SVendingSetupWidget).Subsystem(this);
    SetupAlignWrapper = SNew(SBox)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [ SetupWidget.ToSharedRef() ];
    SetupOverlay = SNew(SWeakWidget).PossiblyNullContent(SetupAlignWrapper);
    VC->AddViewportWidgetContent(SetupOverlay.ToSharedRef(), 22);
    bSetupWidgetVisible = true;
}

void UVendingSubsystem::HideSetupWidget()
{
    if (!bSetupWidgetVisible) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (SetupOverlay.IsValid())
                VC->RemoveViewportWidgetContent(SetupOverlay.ToSharedRef());
        }
    }
    SetupWidget.Reset();
    SetupAlignWrapper.Reset();
    SetupOverlay.Reset();
    bSetupWidgetVisible = false;
}

void UVendingSubsystem::ShowBrowseWidget()
{
    if (bBrowseWidgetVisible) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    BrowseWidget = SNew(SVendingBrowseWidget).Subsystem(this);
    BrowseAlignWrapper = SNew(SBox)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [ BrowseWidget.ToSharedRef() ];
    BrowseOverlay = SNew(SWeakWidget).PossiblyNullContent(BrowseAlignWrapper);
    VC->AddViewportWidgetContent(BrowseOverlay.ToSharedRef(), 22);
    bBrowseWidgetVisible = true;
}

void UVendingSubsystem::HideBrowseWidget()
{
    if (!bBrowseWidgetVisible) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (BrowseOverlay.IsValid())
                VC->RemoveViewportWidgetContent(BrowseOverlay.ToSharedRef());
        }
    }
    BrowseWidget.Reset();
    BrowseAlignWrapper.Reset();
    BrowseOverlay.Reset();
    bBrowseWidgetVisible = false;
}

bool UVendingSubsystem::IsAnyWidgetVisible() const
{
    return bSetupWidgetVisible || bBrowseWidgetVisible;
}
```

### 2.6 Client: SVendingBrowseWidget (Complete)

File: `client/SabriMMO/Source/SabriMMO/UI/SVendingBrowseWidget.h`

```cpp
// SVendingBrowseWidget.h — Slate widget for browsing another player's vending shop.
// Shows shop title, item list with prices, quantity input, and buy button.
// RO Classic brown/gold theme. Draggable via title bar.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UVendingSubsystem;
class SVerticalBox;
class SEditableTextBox;

class SVendingBrowseWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SVendingBrowseWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UVendingSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TWeakObjectPtr<UVendingSubsystem> OwningSubsystem;

    TSharedPtr<SVerticalBox> ItemListContainer;
    int32 SelectedVendSlot = -1;
    int32 SelectedMaxQty = 0;
    TSharedPtr<SEditableTextBox> QuantityInput;

    TSharedRef<SWidget> BuildTitleBar();
    TSharedRef<SWidget> BuildItemList();
    TSharedRef<SWidget> BuildPurchaseRow();
    TSharedRef<SWidget> BuildBottomRow();
    void RebuildItemList();

    TSharedRef<SWidget> MakeButton(const FText& Label, FOnClicked OnClicked);

    // Drag support
    bool bIsDragging = false;
    FVector2D DragOffset = FVector2D::ZeroVector;
    FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
    FVector2D WidgetPosition = FVector2D(400.0, 200.0);
    void ApplyLayout();

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
};
```

File: `client/SabriMMO/Source/SabriMMO/UI/SVendingBrowseWidget.cpp`

```cpp
// SVendingBrowseWidget.cpp — Browse widget for another player's vending shop.

#include "SVendingBrowseWidget.h"
#include "VendingSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/CoreStyle.h"

namespace VendColors
{
    static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
    static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
    static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
    static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
    static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
    static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
    static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
    static const FLinearColor ButtonBg      (0.33f, 0.22f, 0.13f, 1.f);
    static const FLinearColor SelectedBg    (0.45f, 0.32f, 0.18f, 1.f);
    static const FLinearColor ErrorRed      (0.90f, 0.25f, 0.25f, 1.f);
    static const FLinearColor SuccessGreen  (0.30f, 0.85f, 0.30f, 1.f);
}

void SVendingBrowseWidget::Construct(const FArguments& InArgs)
{
    OwningSubsystem = InArgs._Subsystem;

    ChildSlot
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Top)
    [
        SNew(SBox)
        .WidthOverride(380.f)
        .MaxDesiredHeight(500.f)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(VendColors::GoldTrim)
            .Padding(FMargin(2.f))
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor(VendColors::PanelDark)
                .Padding(FMargin(1.f))
                [
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(VendColors::PanelBrown)
                    .Padding(FMargin(6.f))
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildTitleBar() ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
                        [
                            SNew(SBox).HeightOverride(1.f)
                            [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(VendColors::GoldDivider) ]
                        ]

                        // Scrollable item list
                        + SVerticalBox::Slot().FillHeight(1.f).MaxHeight(300.f)
                        [ BuildItemList() ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
                        [
                            SNew(SBox).HeightOverride(1.f)
                            [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(VendColors::GoldDivider) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildPurchaseRow() ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 2)
                        [
                            SNew(SBox).HeightOverride(1.f)
                            [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(VendColors::GoldDivider) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight()
                        [ BuildBottomRow() ]
                    ]
                ]
            ]
        ]
    ];

    RebuildItemList();
    ApplyLayout();
}

TSharedRef<SWidget> SVendingBrowseWidget::BuildTitleBar()
{
    UVendingSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text_Lambda([Sub]() -> FText
            {
                if (!Sub) return FText::FromString(TEXT("Vending Shop"));
                return FText::FromString(Sub->BrowsingShopTitle);
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
            .ColorAndOpacity(FSlateColor(VendColors::GoldHighlight))
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(VendColors::TextShadow)
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .ContentPadding(FMargin(2.f))
            .OnClicked_Lambda([this]() -> FReply
            {
                if (auto* Sub = OwningSubsystem.Get()) Sub->CloseBrowseWindow();
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("X")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                .ColorAndOpacity(FSlateColor(VendColors::GoldHighlight))
            ]
        ];
}

TSharedRef<SWidget> SVendingBrowseWidget::BuildItemList()
{
    return SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SAssignNew(ItemListContainer, SVerticalBox)
        ];
}

void SVendingBrowseWidget::RebuildItemList()
{
    if (!ItemListContainer.IsValid()) return;
    ItemListContainer->ClearChildren();

    UVendingSubsystem* Sub = OwningSubsystem.Get();
    if (!Sub) return;

    // Header row
    ItemListContainer->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(2.f)
        [ SNew(STextBlock).Text(FText::FromString(TEXT("Item"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(FSlateColor(VendColors::GoldHighlight)) ]
        + SHorizontalBox::Slot().FillWidth(1.f)
        [ SNew(STextBlock).Text(FText::FromString(TEXT("Price"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(FSlateColor(VendColors::GoldHighlight)) ]
        + SHorizontalBox::Slot().FillWidth(0.5f)
        [ SNew(STextBlock).Text(FText::FromString(TEXT("Qty"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 8)).ColorAndOpacity(FSlateColor(VendColors::GoldHighlight)) ]
    ];

    for (const auto& Item : Sub->BrowsingItems)
    {
        if (!Item.IsValid()) continue;

        int32 Slot = Item.VendSlot;
        int32 MaxQ = Item.Quantity;

        ItemListContainer->AddSlot().AutoHeight().Padding(0, 1)
        [
            SNew(SButton)
            .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
            .OnClicked_Lambda([this, Slot, MaxQ]() -> FReply
            {
                SelectedVendSlot = Slot;
                SelectedMaxQty = MaxQ;
                if (QuantityInput.IsValid())
                    QuantityInput->SetText(FText::FromString(TEXT("1")));
                return FReply::Handled();
            })
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                .BorderBackgroundColor_Lambda([this, Slot]() -> FLinearColor
                {
                    return (SelectedVendSlot == Slot) ? VendColors::SelectedBg : VendColors::ButtonBg;
                })
                .Padding(FMargin(6.f, 3.f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(2.f).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Item.ItemName))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(FSlateColor(VendColors::TextPrimary))
                        .ShadowOffset(FVector2D(1, 1))
                        .ShadowColorAndOpacity(VendColors::TextShadow)
                    ]
                    + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(FString::Printf(TEXT("%sz"), *FText::AsNumber(Item.PricePerUnit).ToString())))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(FSlateColor(VendColors::ZuzucoinGold))
                        .ShadowOffset(FVector2D(1, 1))
                        .ShadowColorAndOpacity(VendColors::TextShadow)
                    ]
                    + SHorizontalBox::Slot().FillWidth(0.5f).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .Text(FText::AsNumber(Item.Quantity))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(FSlateColor(VendColors::TextPrimary))
                    ]
                ]
            ]
        ];
    }

    if (Sub->BrowsingItems.Num() == 0)
    {
        ItemListContainer->AddSlot().AutoHeight().Padding(0, 8)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("No items available.")))
            .Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
            .ColorAndOpacity(FSlateColor(VendColors::TextPrimary))
        ];
    }
}

TSharedRef<SWidget> SVendingBrowseWidget::BuildPurchaseRow()
{
    UVendingSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Qty:")))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            .ColorAndOpacity(FSlateColor(VendColors::TextPrimary))
        ]
        + SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
        [
            SAssignNew(QuantityInput, SEditableTextBox)
            .Text(FText::FromString(TEXT("1")))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 4, 0)
        [
            SNew(STextBlock)
            .Text_Lambda([this, Sub]() -> FText
            {
                if (!Sub || SelectedVendSlot < 0) return FText::FromString(TEXT("Total: 0z"));
                for (const auto& VI : Sub->BrowsingItems)
                {
                    if (VI.VendSlot == SelectedVendSlot)
                    {
                        int32 Qty = FCString::Atoi(*QuantityInput->GetText().ToString());
                        Qty = FMath::Clamp(Qty, 1, VI.Quantity);
                        int64 Total = VI.PricePerUnit * Qty;
                        return FText::FromString(FString::Printf(TEXT("Total: %sz"), *FText::AsNumber(Total).ToString()));
                    }
                }
                return FText::FromString(TEXT("Total: 0z"));
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            .ColorAndOpacity(FSlateColor(VendColors::ZuzucoinGold))
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(VendColors::TextShadow)
        ]
        + SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
        [
            MakeButton(FText::FromString(TEXT("Buy")),
                FOnClicked::CreateLambda([this]() -> FReply
                {
                    auto* Sub = OwningSubsystem.Get();
                    if (!Sub || SelectedVendSlot < 0) return FReply::Handled();

                    int32 Qty = FCString::Atoi(*QuantityInput->GetText().ToString());
                    Qty = FMath::Max(1, Qty);

                    Sub->RequestBuyFromShop(Sub->BrowsingVendorId, SelectedVendSlot, Qty);
                    return FReply::Handled();
                })
            )
        ];
}

TSharedRef<SWidget> SVendingBrowseWidget::BuildBottomRow()
{
    UVendingSubsystem* Sub = OwningSubsystem.Get();

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Text_Lambda([Sub]() -> FText
            {
                if (!Sub || Sub->StatusMessage.IsEmpty()) return FText::GetEmpty();
                if (FPlatformTime::Seconds() > Sub->StatusExpireTime) return FText::GetEmpty();
                return FText::FromString(Sub->StatusMessage);
            })
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
            .ColorAndOpacity_Lambda([Sub]() -> FSlateColor
            {
                if (!Sub || Sub->StatusMessage.IsEmpty()) return FSlateColor(VendColors::SuccessGreen);
                if (Sub->StatusMessage.Contains(TEXT("Bought"))) return FSlateColor(VendColors::SuccessGreen);
                return FSlateColor(VendColors::ErrorRed);
            })
            .ShadowOffset(FVector2D(1, 1))
            .ShadowColorAndOpacity(VendColors::TextShadow)
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
        [
            MakeButton(FText::FromString(TEXT("Close")),
                FOnClicked::CreateLambda([this]() -> FReply
                {
                    if (auto* Sub = OwningSubsystem.Get()) Sub->CloseBrowseWindow();
                    return FReply::Handled();
                })
            )
        ];
}

TSharedRef<SWidget> SVendingBrowseWidget::MakeButton(const FText& Label, FOnClicked OnClicked)
{
    return SNew(SButton)
        .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
        .OnClicked(OnClicked)
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(VendColors::ButtonBg)
            .Padding(FMargin(8.f, 4.f))
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                .ColorAndOpacity(FSlateColor(VendColors::TextPrimary))
                .ShadowOffset(FVector2D(1, 1))
                .ShadowColorAndOpacity(VendColors::TextShadow)
            ]
        ];
}

// ============================================================
// Drag support
// ============================================================

FReply SVendingBrowseWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    if (LocalPos.Y < 24.f)
    {
        bIsDragging = true;
        DragOffset = MouseEvent.GetScreenSpacePosition();
        DragStartWidgetPos = WidgetPosition;
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    return FReply::Unhandled();
}

FReply SVendingBrowseWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging) { bIsDragging = false; return FReply::Handled().ReleaseMouseCapture(); }
    return FReply::Unhandled();
}

FReply SVendingBrowseWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsDragging)
    {
        const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
        const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
            ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
        WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
        ApplyLayout();
        return FReply::Handled();
    }
    return FReply::Unhandled();
}

void SVendingBrowseWidget::ApplyLayout()
{
    SetRenderTransform(FSlateRenderTransform(FVector2f((float)WidgetPosition.X, (float)WidgetPosition.Y)));
}
```

---

## 3. Buying Store

### 3.1 Server Implementation

The Buying Store is the reverse of vending. A Merchant advertises items they WANT to buy with their prices, and other players sell TO the store.

```javascript
// ============================================================
// Buying Store — In-Memory State
// ============================================================
const activeBuyingStores = new Map();

const BUY_STORE = {
    MAX_ITEMS: 5,
    CATALYST_ITEM_ID: 6001,  // "Bulk Buyer Shop License" — 200z from Alberta NPC
    SP_COST: 30,
    MAX_TITLE_LENGTH: 80,
    TAX_THRESHOLD: 10000000,
    TAX_RATE: 0.05,
};

// --- Open a buying store ---
socket.on('buystore:open', async (data) => {
    logger.info(`[RECV] buystore:open from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    if (activeBuyingStores.has(characterId)) {
        socket.emit('buystore:error', { message: 'You already have a buying store open' });
        return;
    }
    if (player.isDead || player.isVending) {
        socket.emit('buystore:error', { message: 'Cannot open a buying store right now' });
        return;
    }

    const shopTitle = (data.shopTitle || '').trim().substring(0, BUY_STORE.MAX_TITLE_LENGTH);
    if (!shopTitle) {
        socket.emit('buystore:error', { message: 'Shop title required' });
        return;
    }

    // Validate skill
    const buyStoreLevel = getPlayerSkillLevel(player, 'MC_BUYING_STORE') || 0;
    if (buyStoreLevel < 1) {
        socket.emit('buystore:error', { message: 'Open Buying Store skill required' });
        return;
    }

    // Validate SP
    if ((player.currentMana || 0) < BUY_STORE.SP_COST) {
        socket.emit('buystore:error', { message: 'Not enough SP' });
        return;
    }

    // Validate catalyst item (consume 1x Bulk Buyer Shop License)
    try {
        const catResult = await pool.query(
            `SELECT inventory_id, quantity FROM character_inventory
             WHERE character_id = $1 AND item_id = $2 AND quantity >= 1`,
            [characterId, BUY_STORE.CATALYST_ITEM_ID]
        );
        if (catResult.rows.length === 0) {
            socket.emit('buystore:error', { message: 'Bulk Buyer Shop License required' });
            return;
        }
        // Consume the license
        const catRow = catResult.rows[0];
        if (catRow.quantity > 1) {
            await pool.query('UPDATE character_inventory SET quantity = quantity - 1 WHERE inventory_id = $1', [catRow.inventory_id]);
        } else {
            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [catRow.inventory_id]);
        }
    } catch (err) {
        socket.emit('buystore:error', { message: 'Failed to validate license' });
        return;
    }

    // Validate items: [{ itemId, buyPrice, amountWanted }]
    const items = data.items;
    if (!Array.isArray(items) || items.length === 0 || items.length > BUY_STORE.MAX_ITEMS) {
        socket.emit('buystore:error', { message: `Select 1-${BUY_STORE.MAX_ITEMS} items` });
        return;
    }

    // Must own 1 of each item (RO requirement)
    const buyItems = [];
    let totalZenyNeeded = 0;

    for (const entry of items) {
        const itemId = parseInt(entry.itemId);
        const buyPrice = parseInt(entry.buyPrice);
        const amountWanted = Math.max(1, parseInt(entry.amountWanted) || 1);

        if (!itemId || buyPrice < 1 || buyPrice > ZENY.MAX_VEND_PRICE) {
            socket.emit('buystore:error', { message: 'Invalid item or price' });
            return;
        }

        // Verify the item is an etc/consumable type (not equipment)
        const itemDef = itemDefinitions.get(itemId);
        if (!itemDef || (itemDef.item_type !== 'etc' && itemDef.item_type !== 'consumable')) {
            socket.emit('buystore:error', { message: `${itemDef?.name || 'Unknown'} cannot be listed in a buying store` });
            return;
        }

        // Verify player owns at least 1 of this item
        const ownResult = await pool.query(
            'SELECT 1 FROM character_inventory WHERE character_id = $1 AND item_id = $2 LIMIT 1',
            [characterId, itemId]
        );
        if (ownResult.rows.length === 0) {
            socket.emit('buystore:error', { message: `You must own at least 1x ${itemDef.name}` });
            return;
        }

        totalZenyNeeded += buyPrice * amountWanted;

        buyItems.push({
            itemId,
            itemName: itemDef.name,
            icon: itemDef.icon,
            buyPrice,
            amountWanted,
            amountBought: 0,
            weight: itemDef.weight || 0,
        });
    }

    // Validate player has enough zeny to cover all purchases
    if (player.zuzucoin < totalZenyNeeded) {
        socket.emit('buystore:error', {
            message: `Need ${totalZenyNeeded.toLocaleString()}z to cover all purchases (have ${player.zuzucoin.toLocaleString()}z)`
        });
        return;
    }

    // Validate weight capacity
    const maxWeight = getPlayerMaxWeight(player);
    let currentWeight = 0;
    try {
        const wR = await pool.query(
            `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as w
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1`, [characterId]
        );
        currentWeight = parseInt(wR.rows[0].w);
    } catch (err) { /* skip */ }

    const totalBuyWeight = buyItems.reduce((s, i) => s + i.weight * i.amountWanted, 0);
    const weightCapacity = Math.floor(maxWeight * 0.9) - currentWeight - 1;
    if (totalBuyWeight > weightCapacity) {
        socket.emit('buystore:error', { message: 'Would exceed 90% weight limit' });
        return;
    }

    // Deduct SP
    player.currentMana = Math.max(0, (player.currentMana || 0) - BUY_STORE.SP_COST);

    // Create the store
    const storeData = {
        characterId,
        characterName: player.characterName,
        shopTitle,
        zoneName: player.zoneName,
        positionX: player.lastX || 0,
        positionY: player.lastY || 0,
        positionZ: player.lastZ || 0,
        items: buyItems,
        zenyBudget: totalZenyNeeded,
        createdAt: new Date(),
    };
    activeBuyingStores.set(characterId, storeData);

    player.isBuyingStore = true;

    socket.emit('buystore:opened', {
        shopTitle,
        items: buyItems,
        zenyBudget: totalZenyNeeded,
    });

    broadcastToZoneExcept(player.zoneName, socket.id, 'buystore:appeared', {
        characterId,
        characterName: player.characterName,
        shopTitle,
        positionX: storeData.positionX,
        positionY: storeData.positionY,
        positionZ: storeData.positionZ,
    });

    logger.info(`[BUYSTORE] ${player.characterName} opened buying store "${shopTitle}" for ${buyItems.length} items`);
});

// --- Sell to a buying store ---
socket.on('buystore:sell', async (data) => {
    logger.info(`[RECV] buystore:sell from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId: sellerCharId, player: seller } = playerInfo;

    const storeOwnerCharId = parseInt(data.storeOwnerCharacterId);
    const itemId = parseInt(data.itemId);
    const quantity = Math.max(1, parseInt(data.quantity) || 1);

    const store = activeBuyingStores.get(storeOwnerCharId);
    if (!store) {
        socket.emit('buystore:error', { message: 'Buying store not found' });
        return;
    }

    const storeItem = store.items.find(i => i.itemId === itemId);
    if (!storeItem) {
        socket.emit('buystore:error', { message: 'Item not wanted by this store' });
        return;
    }

    const remaining = storeItem.amountWanted - storeItem.amountBought;
    if (remaining <= 0) {
        socket.emit('buystore:error', { message: 'Store already bought enough of this item' });
        return;
    }

    const sellQty = Math.min(quantity, remaining);
    const totalPayment = storeItem.buyPrice * sellQty;

    // Verify seller owns the items
    try {
        const invResult = await pool.query(
            `SELECT ci.inventory_id, ci.quantity
             FROM character_inventory ci
             WHERE ci.character_id = $1 AND ci.item_id = $2 AND ci.is_equipped = false`,
            [sellerCharId, itemId]
        );
        if (invResult.rows.length === 0 || invResult.rows[0].quantity < sellQty) {
            socket.emit('buystore:error', { message: 'You do not have enough of this item' });
            return;
        }

        const client = await pool.connect();
        try {
            await client.query('BEGIN');

            // Remove items from seller
            const invRow = invResult.rows[0];
            if (invRow.quantity > sellQty) {
                await client.query(
                    'UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2',
                    [sellQty, invRow.inventory_id]
                );
            } else {
                await client.query('DELETE FROM character_inventory WHERE inventory_id = $1', [invRow.inventory_id]);
            }

            // Add items to store owner
            await addItemToInventory(storeOwnerCharId, itemId, sellQty);

            // Calculate tax
            const tax = (storeItem.buyPrice > BUY_STORE.TAX_THRESHOLD)
                ? Math.floor(totalPayment * BUY_STORE.TAX_RATE) : 0;
            const sellerRevenue = totalPayment - tax;

            // Transfer zeny: store owner pays, seller receives
            const ownerResult = await modifyZeny(storeOwnerCharId, -totalPayment, 'buystore_purchase',
                { sellerCharId, itemId, quantity: sellQty }, sellerCharId, store.zoneName);

            const sellerResult = await modifyZeny(sellerCharId, sellerRevenue, 'buystore_sale',
                { storeOwnerCharId, itemId, quantity: sellQty, tax }, storeOwnerCharId, seller.zoneName);

            await client.query('COMMIT');

            // Update in-memory
            storeItem.amountBought += sellQty;
            if (ownerResult.success) {
                const ownerPlayer = findPlayerByCharId(storeOwnerCharId);
                if (ownerPlayer) ownerPlayer.player.zuzucoin = ownerResult.newBalance;
            }
            if (sellerResult.success) {
                seller.zuzucoin = sellerResult.newBalance;
            }

            // Notify seller
            socket.emit('buystore:sell_success', {
                itemName: storeItem.itemName,
                quantity: sellQty,
                revenue: sellerRevenue,
                newZuzucoin: seller.zuzucoin,
            });

            // Notify store owner
            const ownerPlayer = findPlayerByCharId(storeOwnerCharId);
            if (ownerPlayer) {
                ownerPlayer.socket.emit('buystore:item_bought', {
                    sellerName: seller.characterName,
                    itemName: storeItem.itemName,
                    quantity: sellQty,
                    cost: totalPayment,
                    newZuzucoin: ownerPlayer.player.zuzucoin,
                });
            }

            // Auto-close if all items fully bought
            const allFilled = store.items.every(i => i.amountBought >= i.amountWanted);
            if (allFilled) {
                closeBuyingStore(storeOwnerCharId, 'All items purchased');
            }

            logger.info(`[BUYSTORE] ${seller.characterName} sold ${sellQty}x ${storeItem.itemName} to ${store.characterName}'s store for ${totalPayment}z`);

        } catch (err) {
            await client.query('ROLLBACK');
            socket.emit('buystore:error', { message: 'Transaction failed' });
            logger.error(`[BUYSTORE] Sell transaction failed: ${err.message}`);
        } finally {
            client.release();
        }
    } catch (err) {
        socket.emit('buystore:error', { message: 'Failed to validate items' });
    }
});

// --- Close buying store ---
socket.on('buystore:close', async () => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    closeBuyingStore(playerInfo.characterId, 'Manual close');
});

async function closeBuyingStore(characterId, reason) {
    const store = activeBuyingStores.get(characterId);
    if (!store) return;
    activeBuyingStores.delete(characterId);

    const ownerPlayer = findPlayerByCharId(characterId);
    if (ownerPlayer) {
        ownerPlayer.player.isBuyingStore = false;
        ownerPlayer.socket.emit('buystore:closed', {
            reason,
            items: store.items,
        });
    }
    broadcastToZone(store.zoneName, 'buystore:disappeared', { characterId });
    logger.info(`[BUYSTORE] Store closed for char ${characterId}: ${reason}`);
}
```

### 3.2 Client: Buying Store Subsystem (Skeleton)

The `UBuyingStoreSubsystem` follows the exact same pattern as `UVendingSubsystem`. Socket events:

| Client Emits | Server Emits |
|---|---|
| `buystore:open { shopTitle, items }` | `buystore:opened { shopTitle, items, zenyBudget }` |
| `buystore:browse { storeOwnerCharacterId }` | `buystore:data { ownerId, shopTitle, items }` |
| `buystore:sell { storeOwnerCharacterId, itemId, quantity }` | `buystore:sell_success { itemName, quantity, revenue }` |
| `buystore:close {}` | `buystore:closed { reason, items }` |
| | `buystore:appeared { characterId, shopTitle, position }` |
| | `buystore:disappeared { characterId }` |
| | `buystore:item_bought { sellerName, itemName, quantity, cost }` |
| | `buystore:error { message }` |

The widget pair (`SBuyingStoreSetupWidget` / `SBuyingStoreBrowseWidget`) mirrors the vending widgets but inverts the flow: the browse widget shows wanted items and lets the visitor SELL items to the store rather than buy.

---

## 4. Storage System

### 4.1 Database Schema

```sql
-- Migration: database/migrations/add_storage_system.sql

-- Kafra storage — account-shared across all characters
CREATE TABLE account_storage (
    storage_id      SERIAL PRIMARY KEY,
    user_id         INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL REFERENCES items(item_id),
    quantity        INTEGER NOT NULL DEFAULT 1,
    slot_position   INTEGER NOT NULL,         -- 0-599
    is_identified   BOOLEAN NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (user_id, slot_position)
);
CREATE INDEX idx_storage_user ON account_storage(user_id);

-- Guild storage (future — requires guild system)
CREATE TABLE guild_storage (
    storage_id      SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL,         -- REFERENCES guilds(guild_id) when guild table exists
    item_id         INTEGER NOT NULL REFERENCES items(item_id),
    quantity        INTEGER NOT NULL DEFAULT 1,
    slot_position   INTEGER NOT NULL,         -- 0-499
    is_identified   BOOLEAN NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (guild_id, slot_position)
);
CREATE INDEX idx_guild_storage_guild ON guild_storage(guild_id);
```

### 4.2 Server Constants

```javascript
// ============================================================
// Storage Constants
// ============================================================
const STORAGE = {
    MAX_SLOTS: 600,               // Kafra storage capacity
    ACCESS_FEE: 40,               // 40z per access
    GUILD_BASE_SLOTS: 100,
    GUILD_SLOTS_PER_LEVEL: 100,   // Guild Warehouse Expansion: 100 per level, max 500
    GUILD_MAX_SLOTS: 500,
};
```

### 4.3 Server: Socket Event Handlers

```javascript
// ============================================================
// Storage Socket Events
// ============================================================

// --- Open Kafra storage ---
socket.on('storage:open', async () => {
    logger.info(`[RECV] storage:open from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    // Prevent opening during trade, vending, etc.
    if (player.isVending || player.isBuyingStore) {
        socket.emit('storage:error', { message: 'Cannot open storage right now' });
        return;
    }
    if (player.isStorageOpen) {
        socket.emit('storage:error', { message: 'Storage is already open' });
        return;
    }

    // Charge access fee
    const feeResult = await modifyZeny(characterId, -STORAGE.ACCESS_FEE, 'storage_fee',
        { action: 'open' }, null, player.zoneName);
    if (!feeResult.success) {
        socket.emit('storage:error', { message: `Not enough Zeny (need ${STORAGE.ACCESS_FEE}z)` });
        return;
    }
    player.zuzucoin = feeResult.newBalance;

    // Get user_id for this character (storage is account-shared)
    let userId;
    try {
        const charResult = await pool.query(
            'SELECT user_id FROM characters WHERE character_id = $1', [characterId]
        );
        if (charResult.rows.length === 0) {
            socket.emit('storage:error', { message: 'Character not found' });
            return;
        }
        userId = charResult.rows[0].user_id;
    } catch (err) {
        socket.emit('storage:error', { message: 'Failed to look up account' });
        return;
    }

    // Load storage items
    try {
        const storageResult = await pool.query(
            `SELECT s.storage_id, s.item_id, s.quantity, s.slot_position, s.is_identified,
                    i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
                    i.atk, i.def, i.matk, i.mdef, i.icon, i.stackable,
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.weapon_type
             FROM account_storage s
             JOIN items i ON s.item_id = i.item_id
             WHERE s.user_id = $1
             ORDER BY s.slot_position`,
            [userId]
        );

        const storageItems = storageResult.rows.map(row => ({
            storageId: row.storage_id,
            itemId: row.item_id,
            name: row.name,
            description: row.description,
            itemType: row.item_type,
            equipSlot: row.equip_slot || '',
            quantity: row.quantity,
            slotPosition: row.slot_position,
            weight: row.weight,
            price: row.price,
            atk: row.atk,
            def: row.def,
            matk: row.matk,
            mdef: row.mdef,
            icon: row.icon,
            stackable: row.stackable,
            strBonus: row.str_bonus,
            agiBonus: row.agi_bonus,
            vitBonus: row.vit_bonus,
            intBonus: row.int_bonus,
            dexBonus: row.dex_bonus,
            lukBonus: row.luk_bonus,
            maxHPBonus: row.max_hp_bonus,
            maxSPBonus: row.max_sp_bonus,
            requiredLevel: row.required_level,
            weaponType: row.weapon_type || '',
        }));

        player.isStorageOpen = true;
        player.storageUserId = userId;

        socket.emit('storage:opened', {
            items: storageItems,
            maxSlots: STORAGE.MAX_SLOTS,
            usedSlots: storageItems.length,
            newZuzucoin: player.zuzucoin,
        });

        logger.info(`[STORAGE] Opened for char ${characterId} (user ${userId}): ${storageItems.length} items, fee=${STORAGE.ACCESS_FEE}z`);
    } catch (err) {
        logger.error(`[STORAGE] Load failed: ${err.message}`);
        socket.emit('storage:error', { message: 'Failed to load storage' });
    }
});

// --- Close storage ---
socket.on('storage:close', async () => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    playerInfo.player.isStorageOpen = false;
    playerInfo.player.storageUserId = null;
    socket.emit('storage:closed', {});
    logger.info(`[STORAGE] Closed for char ${playerInfo.characterId}`);
});

// --- Deposit item from inventory to storage ---
socket.on('storage:deposit', async (data) => {
    logger.info(`[RECV] storage:deposit from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    if (!player.isStorageOpen) {
        socket.emit('storage:error', { message: 'Storage is not open' });
        return;
    }

    const inventoryId = parseInt(data.inventoryId);
    const depositQty = Math.max(1, parseInt(data.quantity) || 1);
    const userId = player.storageUserId;

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // 1. Verify item exists in player inventory and is not equipped
        const invResult = await client.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
                    i.name, i.stackable, i.icon
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2
             FOR UPDATE`,
            [inventoryId, characterId]
        );
        if (invResult.rows.length === 0) {
            await client.query('ROLLBACK');
            socket.emit('storage:error', { message: 'Item not found' });
            return;
        }
        const invRow = invResult.rows[0];
        if (invRow.is_equipped) {
            await client.query('ROLLBACK');
            socket.emit('storage:error', { message: 'Unequip the item first' });
            return;
        }

        const actualQty = Math.min(depositQty, invRow.quantity);

        // 2. Check storage capacity
        const slotCount = await client.query(
            'SELECT COUNT(*) as c FROM account_storage WHERE user_id = $1', [userId]
        );
        const usedSlots = parseInt(slotCount.rows[0].c);

        // If item is stackable, check if it already exists in storage
        let existingStorageSlot = null;
        if (invRow.stackable) {
            const existResult = await client.query(
                'SELECT storage_id, quantity, slot_position FROM account_storage WHERE user_id = $1 AND item_id = $2 LIMIT 1',
                [userId, invRow.item_id]
            );
            if (existResult.rows.length > 0) {
                existingStorageSlot = existResult.rows[0];
            }
        }

        if (!existingStorageSlot && usedSlots >= STORAGE.MAX_SLOTS) {
            await client.query('ROLLBACK');
            socket.emit('storage:error', { message: `Storage is full (${STORAGE.MAX_SLOTS} slots)` });
            return;
        }

        // 3. Remove from inventory
        if (actualQty >= invRow.quantity) {
            await client.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
        } else {
            await client.query(
                'UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2',
                [actualQty, inventoryId]
            );
        }

        // 4. Add to storage
        if (existingStorageSlot) {
            await client.query(
                'UPDATE account_storage SET quantity = quantity + $1 WHERE storage_id = $2',
                [actualQty, existingStorageSlot.storage_id]
            );
        } else {
            // Find next available slot position
            const maxSlotResult = await client.query(
                'SELECT COALESCE(MAX(slot_position), -1) + 1 as next_slot FROM account_storage WHERE user_id = $1',
                [userId]
            );
            const nextSlot = parseInt(maxSlotResult.rows[0].next_slot);
            await client.query(
                `INSERT INTO account_storage (user_id, item_id, quantity, slot_position)
                 VALUES ($1, $2, $3, $4)`,
                [userId, invRow.item_id, actualQty, nextSlot]
            );
        }

        await client.query('COMMIT');

        // 5. Send updated data to client
        await sendStorageUpdate(socket, userId);
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

        logger.info(`[STORAGE] Deposited ${actualQty}x ${invRow.name} for char ${characterId}`);

    } catch (err) {
        await client.query('ROLLBACK');
        logger.error(`[STORAGE] Deposit failed: ${err.message}`);
        socket.emit('storage:error', { message: 'Deposit failed' });
    } finally {
        client.release();
    }
});

// --- Withdraw item from storage to inventory ---
socket.on('storage:withdraw', async (data) => {
    logger.info(`[RECV] storage:withdraw from ${socket.id}`);
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    if (!player.isStorageOpen) {
        socket.emit('storage:error', { message: 'Storage is not open' });
        return;
    }

    const storageId = parseInt(data.storageId);
    const withdrawQty = Math.max(1, parseInt(data.quantity) || 1);
    const userId = player.storageUserId;

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // 1. Verify item exists in storage
        const stResult = await client.query(
            `SELECT s.storage_id, s.item_id, s.quantity,
                    i.name, i.weight, i.stackable
             FROM account_storage s JOIN items i ON s.item_id = i.item_id
             WHERE s.storage_id = $1 AND s.user_id = $2
             FOR UPDATE`,
            [storageId, userId]
        );
        if (stResult.rows.length === 0) {
            await client.query('ROLLBACK');
            socket.emit('storage:error', { message: 'Item not found in storage' });
            return;
        }
        const stRow = stResult.rows[0];
        const actualQty = Math.min(withdrawQty, stRow.quantity);

        // 2. Check inventory weight
        const maxWeight = getPlayerMaxWeight(player);
        let currentWeight = 0;
        try {
            const wR = await client.query(
                `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as w
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1`, [characterId]
            );
            currentWeight = parseInt(wR.rows[0].w);
        } catch (err) { /* skip */ }

        const addedWeight = (stRow.weight || 0) * actualQty;
        if (currentWeight + addedWeight > maxWeight) {
            await client.query('ROLLBACK');
            socket.emit('storage:error', { message: 'Would exceed weight limit' });
            return;
        }

        // 3. Remove from storage
        if (actualQty >= stRow.quantity) {
            await client.query('DELETE FROM account_storage WHERE storage_id = $1', [storageId]);
        } else {
            await client.query(
                'UPDATE account_storage SET quantity = quantity - $1 WHERE storage_id = $2',
                [actualQty, storageId]
            );
        }

        // 4. Add to inventory
        await addItemToInventory(characterId, stRow.item_id, actualQty);

        await client.query('COMMIT');

        // 5. Send updates
        await sendStorageUpdate(socket, userId);
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

        logger.info(`[STORAGE] Withdrew ${actualQty}x ${stRow.name} for char ${characterId}`);

    } catch (err) {
        await client.query('ROLLBACK');
        logger.error(`[STORAGE] Withdraw failed: ${err.message}`);
        socket.emit('storage:error', { message: 'Withdraw failed' });
    } finally {
        client.release();
    }
});

// Helper: send storage:updated with full item list
async function sendStorageUpdate(socket, userId) {
    try {
        const result = await pool.query(
            `SELECT s.storage_id, s.item_id, s.quantity, s.slot_position,
                    i.name, i.description, i.item_type, i.equip_slot, i.weight,
                    i.price, i.atk, i.def, i.icon, i.stackable
             FROM account_storage s JOIN items i ON s.item_id = i.item_id
             WHERE s.user_id = $1
             ORDER BY s.slot_position`,
            [userId]
        );
        socket.emit('storage:updated', {
            items: result.rows.map(row => ({
                storageId: row.storage_id,
                itemId: row.item_id,
                name: row.name,
                description: row.description,
                itemType: row.item_type,
                equipSlot: row.equip_slot || '',
                quantity: row.quantity,
                slotPosition: row.slot_position,
                weight: row.weight,
                price: row.price,
                atk: row.atk,
                def: row.def,
                icon: row.icon,
                stackable: row.stackable,
            })),
            maxSlots: STORAGE.MAX_SLOTS,
        });
    } catch (err) {
        logger.error(`[STORAGE] Update send failed: ${err.message}`);
    }
}
```

### 4.4 Client: StorageSubsystem Header

File: `client/SabriMMO/Source/SabriMMO/UI/StorageSubsystem.h`

```cpp
// StorageSubsystem.h — UWorldSubsystem managing Kafra storage state,
// deposit/withdraw operations, and SStorageWidget lifecycle.
// Storage is account-shared (all characters on the same account see the same items).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CharacterData.h"
#include "StorageSubsystem.generated.h"

class USocketIOClientComponent;
class SStorageWidget;

USTRUCT()
struct FStorageItem
{
    GENERATED_BODY()

    int32 StorageId = 0;
    int32 ItemId = 0;
    FString Name;
    FString Description;
    FString ItemType;
    FString EquipSlot;
    int32 Quantity = 1;
    int32 SlotPosition = 0;
    int32 Weight = 0;
    int32 Price = 0;
    int32 ATK = 0;
    int32 DEF = 0;
    FString Icon;
    bool bStackable = false;

    bool IsValid() const { return StorageId > 0; }
};

UCLASS()
class SABRIMMO_API UStorageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- state (read by widget) ----
    bool bIsOpen = false;
    TArray<FStorageItem> Items;
    int32 MaxSlots = 600;
    int32 UsedSlots = 0;
    FString StatusMessage;
    double StatusExpireTime = 0.0;

    // ---- public API ----
    void RequestOpen();
    void RequestClose();
    void RequestDeposit(int32 InventoryId, int32 Quantity);
    void RequestWithdraw(int32 StorageId, int32 Quantity);

    // ---- widget lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleStorageOpened(const TSharedPtr<FJsonValue>& Data);
    void HandleStorageClosed(const TSharedPtr<FJsonValue>& Data);
    void HandleStorageUpdated(const TSharedPtr<FJsonValue>& Data);
    void HandleStorageError(const TSharedPtr<FJsonValue>& Data);

    FStorageItem ParseStorageItemFromJson(const TSharedPtr<FJsonObject>& Obj);

    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    FTimerHandle BindCheckTimer;

    TSharedPtr<SStorageWidget> StorageWidget;
    TSharedPtr<SWidget> AlignmentWrapper;
    TSharedPtr<SWidget> ViewportOverlay;

    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 4.5 Client: StorageSubsystem Implementation

File: `client/SabriMMO/Source/SabriMMO/UI/StorageSubsystem.cpp`

```cpp
#include "StorageSubsystem.h"
#include "SStorageWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogStorage, Log, All);

bool UStorageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UStorageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    InWorld.GetTimerManager().SetTimer(BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UStorageSubsystem::TryWrapSocketEvents),
        0.5f, true);
}

void UStorageSubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

USocketIOClientComponent* UStorageSubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (auto* Comp = It->FindComponentByClass<USocketIOClientComponent>())
            return Comp;
    }
    return nullptr;
}

void UStorageSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;
    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;
    TSharedPtr<FSocketIONative> NC = SIOComp->GetNativeClient();
    if (!NC.IsValid() || !NC->bIsConnected) return;
    if (!NC->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

    CachedSIOComponent = SIOComp;

    WrapSingleEvent(TEXT("storage:opened"),  [this](const TSharedPtr<FJsonValue>& D) { HandleStorageOpened(D); });
    WrapSingleEvent(TEXT("storage:closed"),  [this](const TSharedPtr<FJsonValue>& D) { HandleStorageClosed(D); });
    WrapSingleEvent(TEXT("storage:updated"), [this](const TSharedPtr<FJsonValue>& D) { HandleStorageUpdated(D); });
    WrapSingleEvent(TEXT("storage:error"),   [this](const TSharedPtr<FJsonValue>& D) { HandleStorageError(D); });

    bEventsWrapped = true;
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    UE_LOG(LogStorage, Log, TEXT("StorageSubsystem events wrapped."));
}

void UStorageSubsystem::WrapSingleEvent(const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FSocketIONative> NC = CachedSIOComponent->GetNativeClient();
    if (!NC.IsValid()) return;

    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> Original;
    FSIOBoundEvent* Existing = NC->EventFunctionMap.Find(EventName);
    if (Existing) Original = Existing->Function;

    NC->OnEvent(EventName,
        [Original, OurHandler](const FString& Ev, const TSharedPtr<FJsonValue>& Msg)
        {
            if (Original) Original(Ev, Msg);
            if (OurHandler) OurHandler(Msg);
        },
        TEXT("/"), ESIOThreadOverrideOption::USE_GAME_THREAD);
}

// ---- Event Handlers ----

void UStorageSubsystem::HandleStorageOpened(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double MS = 0, US = 0;
    Obj->TryGetNumberField(TEXT("maxSlots"), MS); MaxSlots = (int32)MS;
    Obj->TryGetNumberField(TEXT("usedSlots"), US); UsedSlots = (int32)US;

    Items.Empty();
    const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
    {
        for (const auto& Val : *ItemsArr)
        {
            const TSharedPtr<FJsonObject>* IO = nullptr;
            if (Val.IsValid() && Val->TryGetObject(IO) && IO)
                Items.Add(ParseStorageItemFromJson(*IO));
        }
    }
    UsedSlots = Items.Num();

    bIsOpen = true;
    ShowWidget();
    UE_LOG(LogStorage, Log, TEXT("Storage opened: %d/%d slots"), UsedSlots, MaxSlots);
}

void UStorageSubsystem::HandleStorageClosed(const TSharedPtr<FJsonValue>& Data)
{
    bIsOpen = false;
    Items.Empty();
    HideWidget();
    UE_LOG(LogStorage, Log, TEXT("Storage closed."));
}

void UStorageSubsystem::HandleStorageUpdated(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    Items.Empty();
    const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
    {
        for (const auto& Val : *ItemsArr)
        {
            const TSharedPtr<FJsonObject>* IO = nullptr;
            if (Val.IsValid() && Val->TryGetObject(IO) && IO)
                Items.Add(ParseStorageItemFromJson(*IO));
        }
    }
    UsedSlots = Items.Num();

    double MS = 0;
    Obj->TryGetNumberField(TEXT("maxSlots"), MS);
    if (MS > 0) MaxSlots = (int32)MS;

    UE_LOG(LogStorage, Log, TEXT("Storage updated: %d items"), UsedSlots);
}

void UStorageSubsystem::HandleStorageError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    FString Msg;
    (*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
    StatusMessage = Msg;
    StatusExpireTime = FPlatformTime::Seconds() + 4.0;
    UE_LOG(LogStorage, Warning, TEXT("Storage error: %s"), *Msg);
}

FStorageItem UStorageSubsystem::ParseStorageItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
    FStorageItem SI;
    double D = 0;
    Obj->TryGetNumberField(TEXT("storageId"), D); SI.StorageId = (int32)D;
    Obj->TryGetNumberField(TEXT("itemId"), D); SI.ItemId = (int32)D;
    Obj->TryGetStringField(TEXT("name"), SI.Name);
    Obj->TryGetStringField(TEXT("description"), SI.Description);
    Obj->TryGetStringField(TEXT("itemType"), SI.ItemType);
    Obj->TryGetStringField(TEXT("equipSlot"), SI.EquipSlot);
    Obj->TryGetNumberField(TEXT("quantity"), D); SI.Quantity = (int32)D;
    Obj->TryGetNumberField(TEXT("slotPosition"), D); SI.SlotPosition = (int32)D;
    Obj->TryGetNumberField(TEXT("weight"), D); SI.Weight = (int32)D;
    Obj->TryGetNumberField(TEXT("price"), D); SI.Price = (int32)D;
    Obj->TryGetNumberField(TEXT("atk"), D); SI.ATK = (int32)D;
    Obj->TryGetNumberField(TEXT("def"), D); SI.DEF = (int32)D;
    Obj->TryGetStringField(TEXT("icon"), SI.Icon);
    bool bStack = false;
    Obj->TryGetBoolField(TEXT("stackable"), bStack); SI.bStackable = bStack;
    return SI;
}

// ---- Public API ----

void UStorageSubsystem::RequestOpen()
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> P = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("storage:open"), P);
}

void UStorageSubsystem::RequestClose()
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> P = MakeShareable(new FJsonObject());
    CachedSIOComponent->EmitNative(TEXT("storage:close"), P);
    bIsOpen = false;
    HideWidget();
}

void UStorageSubsystem::RequestDeposit(int32 InventoryId, int32 Quantity)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> P = MakeShareable(new FJsonObject());
    P->SetNumberField(TEXT("inventoryId"), InventoryId);
    P->SetNumberField(TEXT("quantity"), Quantity);
    CachedSIOComponent->EmitNative(TEXT("storage:deposit"), P);
}

void UStorageSubsystem::RequestWithdraw(int32 StorageId, int32 Quantity)
{
    if (!CachedSIOComponent.IsValid()) return;
    TSharedPtr<FJsonObject> P = MakeShareable(new FJsonObject());
    P->SetNumberField(TEXT("storageId"), StorageId);
    P->SetNumberField(TEXT("quantity"), Quantity);
    CachedSIOComponent->EmitNative(TEXT("storage:withdraw"), P);
}

// ---- Widget Lifecycle ----

void UStorageSubsystem::ShowWidget()
{
    if (bWidgetAdded) return;
    UWorld* World = GetWorld();
    if (!World) return;
    UGameViewportClient* VC = World->GetGameViewport();
    if (!VC) return;

    StorageWidget = SNew(SStorageWidget).Subsystem(this);
    AlignmentWrapper = SNew(SBox)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        .Visibility(EVisibility::SelfHitTestInvisible)
        [ StorageWidget.ToSharedRef() ];
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
    VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 18);
    bWidgetAdded = true;
    UE_LOG(LogStorage, Log, TEXT("Storage widget shown (Z=18)."));
}

void UStorageSubsystem::HideWidget()
{
    if (!bWidgetAdded) return;
    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (ViewportOverlay.IsValid())
                VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
        }
    }
    StorageWidget.Reset();
    AlignmentWrapper.Reset();
    ViewportOverlay.Reset();
    bWidgetAdded = false;
}

bool UStorageSubsystem::IsWidgetVisible() const { return bWidgetAdded; }
```

### 4.6 Kafra Integration

Extend `KafraSubsystem` to add a "Storage" button. In `SKafraWidget::BuildMainMenuContent()`, add:

```cpp
// Storage button — between Save Point and Teleport Service
+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
[
    BuildKafraButton(
        FText::FromString(TEXT("Use Storage")),
        FOnClicked::CreateLambda([this]() -> FReply
        {
            // Close Kafra UI first, then open storage
            auto* KSub = OwningSubsystem.Get();
            if (KSub) KSub->CloseKafra();

            // Access StorageSubsystem through the World
            if (UWorld* World = GEngine->GetCurrentPlayWorld())
            {
                if (auto* StorageSub = World->GetSubsystem<UStorageSubsystem>())
                {
                    StorageSub->RequestOpen();
                }
            }
            return FReply::Handled();
        })
    )
]
```

**Important**: Use `GetWorld()` through the owning subsystem, not `GEngine` directly (multiplayer-safe rule). In practice, the Kafra widget should call `KafraSubsystem->RequestOpenStorage()` which then uses the subsystem's own `GetWorld()`.

---

## 5. Ground Item System

### 5.1 Server: In-Memory Ground Item State

Ground items are ephemeral (60s lifetime) so they live in memory, not in the database.

```javascript
// ============================================================
// Ground Item System
// ============================================================

// Map<groundItemId, GroundItemData>
const groundItems = new Map();
let nextGroundItemId = 1;

const GROUND_ITEM = {
    LIFETIME_MS: 60000,          // 60 seconds
    PICKUP_RANGE: 100,           // ~2 RO cells in UE units
    OWNERSHIP_PHASE_1_MS: 3000,  // First 3s: top damage only
    OWNERSHIP_PHASE_2_MS: 4000,  // Next 1s: top 2
    OWNERSHIP_PHASE_3_MS: 5000,  // Next 1s: top 3
    // After 5s: free-for-all

    MVP_PHASE_1_MS: 10000,
    MVP_PHASE_2_MS: 20000,
    MVP_PHASE_3_MS: 22000,
};

// GroundItemData:
// {
//   groundItemId, itemId, itemName, icon, quantity, zoneName,
//   x, y, z,
//   ownershipData: { damageRanking: [charId1, charId2, ...], isMVP: bool },
//   dropTime: number (Date.now()),
//   despawnTimer: NodeJS.Timeout,
// }

function spawnGroundItem(zoneName, itemId, quantity, x, y, z, damageRanking = [], isMVP = false) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) return null;

    const groundItemId = nextGroundItemId++;
    const dropTime = Date.now();

    const item = {
        groundItemId,
        itemId,
        itemName: itemDef.name,
        icon: itemDef.icon,
        quantity,
        zoneName,
        x, y, z,
        ownershipData: { damageRanking, isMVP },
        dropTime,
        despawnTimer: null,
    };

    // Set despawn timer
    item.despawnTimer = setTimeout(() => {
        despawnGroundItem(groundItemId);
    }, GROUND_ITEM.LIFETIME_MS);

    groundItems.set(groundItemId, item);

    // Broadcast to zone
    broadcastToZone(zoneName, 'item:spawned', {
        groundItemId,
        itemId,
        itemName: itemDef.name,
        icon: itemDef.icon,
        quantity,
        x, y, z,
    });

    return groundItemId;
}

function despawnGroundItem(groundItemId) {
    const item = groundItems.get(groundItemId);
    if (!item) return;

    if (item.despawnTimer) clearTimeout(item.despawnTimer);
    groundItems.delete(groundItemId);

    broadcastToZone(item.zoneName, 'item:despawned', { groundItemId });
}

function canPickupItem(characterId, groundItem) {
    const elapsed = Date.now() - groundItem.dropTime;
    const ranking = groundItem.ownershipData.damageRanking;
    const isMVP = groundItem.ownershipData.isMVP;

    if (ranking.length === 0) return true; // No ownership (player-dropped)

    const phase1 = isMVP ? GROUND_ITEM.MVP_PHASE_1_MS : GROUND_ITEM.OWNERSHIP_PHASE_1_MS;
    const phase2 = isMVP ? GROUND_ITEM.MVP_PHASE_2_MS : GROUND_ITEM.OWNERSHIP_PHASE_2_MS;
    const phase3 = isMVP ? GROUND_ITEM.MVP_PHASE_3_MS : GROUND_ITEM.OWNERSHIP_PHASE_3_MS;

    if (elapsed < phase1) {
        return ranking.length > 0 && ranking[0] === characterId;
    }
    if (elapsed < phase2) {
        return ranking.slice(0, 2).includes(characterId);
    }
    if (elapsed < phase3) {
        return ranking.slice(0, 3).includes(characterId);
    }
    return true; // Free-for-all
}

// --- Pickup event ---
socket.on('item:pickup', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    const groundItemId = parseInt(data.groundItemId);
    const item = groundItems.get(groundItemId);

    if (!item) {
        socket.emit('item:pickup_error', { message: 'Item no longer exists' });
        return;
    }

    // Check zone match
    if (item.zoneName !== player.zoneName) {
        socket.emit('item:pickup_error', { message: 'Wrong zone' });
        return;
    }

    // Check range
    const dx = (player.lastX || 0) - item.x;
    const dy = (player.lastY || 0) - item.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > GROUND_ITEM.PICKUP_RANGE) {
        socket.emit('item:pickup_error', { message: 'Too far away' });
        return;
    }

    // Check ownership
    if (!canPickupItem(characterId, item)) {
        socket.emit('item:pickup_error', { message: 'Item is not available to you yet' });
        return;
    }

    // Check weight
    const maxWeight = getPlayerMaxWeight(player);
    let currentWeight = 0;
    try {
        const wR = await pool.query(
            `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as w
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1`, [characterId]
        );
        currentWeight = parseInt(wR.rows[0].w);
    } catch (err) { /* skip */ }

    const itemDef = itemDefinitions.get(item.itemId);
    const addedWeight = (itemDef?.weight || 0) * item.quantity;
    if (currentWeight + addedWeight > maxWeight) {
        socket.emit('item:pickup_error', { message: 'Too heavy' });
        return;
    }

    // Add to inventory
    try {
        await addItemToInventory(characterId, item.itemId, item.quantity);

        // Remove from ground
        if (item.despawnTimer) clearTimeout(item.despawnTimer);
        groundItems.delete(groundItemId);

        // Broadcast pickup
        broadcastToZone(item.zoneName, 'item:picked_up', {
            groundItemId,
            pickedUpByCharId: characterId,
        });

        // Refresh inventory
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

        logger.info(`[GROUND] ${player.characterName} picked up ${item.quantity}x ${item.itemName}`);
    } catch (err) {
        logger.error(`[GROUND] Pickup failed: ${err.message}`);
        socket.emit('item:pickup_error', { message: 'Pickup failed' });
    }
});

// --- Player manually drops an item ---
socket.on('item:drop_ground', async (data) => {
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;

    const inventoryId = parseInt(data.inventoryId);
    const dropQty = Math.max(1, parseInt(data.quantity) || 1);

    try {
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
                    i.name, i.icon
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
            [inventoryId, characterId]
        );
        if (result.rows.length === 0) return;
        const row = result.rows[0];
        if (row.is_equipped) {
            socket.emit('item:pickup_error', { message: 'Unequip item first' });
            return;
        }

        const actualQty = Math.min(dropQty, row.quantity);

        // Remove from inventory
        if (actualQty >= row.quantity) {
            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
        } else {
            await pool.query(
                'UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2',
                [actualQty, inventoryId]
            );
        }

        // Spawn on ground (no ownership — player-dropped)
        spawnGroundItem(player.zoneName, row.item_id, actualQty,
            player.lastX || 0, player.lastY || 0, player.lastZ || 0);

        // Refresh inventory
        const inventory = await getPlayerInventory(characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });

        logger.info(`[GROUND] ${player.characterName} dropped ${actualQty}x ${row.name}`);
    } catch (err) {
        logger.error(`[GROUND] Drop failed: ${err.message}`);
    }
});

// Integration: call from enemy death handler
// In the existing enemy death code, after calculating drops:
// for (const drop of drops) {
//     spawnGroundItem(zoneName, drop.itemId, drop.quantity,
//         enemy.x, enemy.y, enemy.z, damageRanking, enemy.isMVP);
// }
```

### 5.2 Client: GroundItemSubsystem (Header)

File: `client/SabriMMO/Source/SabriMMO/UI/GroundItemSubsystem.h`

```cpp
// GroundItemSubsystem.h — UWorldSubsystem managing ground item actors,
// billboards with item names, click-to-pickup, and despawn tracking.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "GroundItemSubsystem.generated.h"

class USocketIOClientComponent;

USTRUCT()
struct FGroundItemData
{
    GENERATED_BODY()

    int32 GroundItemId = 0;
    int32 ItemId = 0;
    FString ItemName;
    FString Icon;
    int32 Quantity = 1;
    FVector Position = FVector::ZeroVector;
    double SpawnTime = 0.0;       // FPlatformTime::Seconds() when received

    bool IsValid() const { return GroundItemId > 0; }
};

UCLASS()
class SABRIMMO_API UGroundItemSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- state ----
    TMap<int32, FGroundItemData> GroundItems;  // groundItemId -> data

    // ---- public API ----
    void RequestPickup(int32 GroundItemId);
    void RequestDropItem(int32 InventoryId, int32 Quantity);

    // Returns the ground item nearest to WorldLocation within PickupRange, or nullptr
    const FGroundItemData* FindNearestGroundItem(const FVector& WorldLocation, float PickupRange = 150.f) const;

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleItemSpawned(const TSharedPtr<FJsonValue>& Data);
    void HandleItemPickedUp(const TSharedPtr<FJsonValue>& Data);
    void HandleItemDespawned(const TSharedPtr<FJsonValue>& Data);
    void HandlePickupError(const TSharedPtr<FJsonValue>& Data);

    bool bEventsWrapped = false;
    FTimerHandle BindCheckTimer;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

### 5.3 Ground Item Actor (Blueprint)

Create `BP_GroundItem` in Unreal Editor:
- **StaticMeshComponent** (root): Small sprite/billboard mesh
- **TextRenderComponent**: Item name displayed below the mesh, white text, world-facing
- **SphereCollision**: Radius 100 UE units for click detection (not overlap-based pickup)

The `GroundItemSubsystem` spawns and destroys `BP_GroundItem` actors when it receives `item:spawned` / `item:picked_up` / `item:despawned` events. Click-to-pickup is handled through the existing `BPI_Interactable` interface pattern (see `/sabrimmo-click-interact` skill).

---

## 6. NPC Shop Enhancement

### 6.1 Server: Discount/Overcharge Already Implemented

The existing `shop:buy_batch` and `shop:sell_batch` handlers already apply `getDiscountPercent()` and `getOverchargePercent()`. No server changes needed.

### 6.2 Client Enhancement: Quantity Selectors

The existing NPC shop widget (`SShopWidget` or Blueprint equivalent) needs quantity selectors for batch buying/selling.

Add to the buy side of the shop widget:

```cpp
// Quantity selector row — placed after item selection in the buy cart
SNew(SHorizontalBox)
+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
[
    SNew(SButton)
    .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
    .OnClicked_Lambda([this]() -> FReply
    {
        BuyQuantity = FMath::Max(1, BuyQuantity - 1);
        return FReply::Handled();
    })
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("-")))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        .ColorAndOpacity(FSlateColor(FLinearColor::White))
    ]
]
+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
[
    SNew(SBox).WidthOverride(40.f)
    [
        SNew(SEditableTextBox)
        .Text_Lambda([this]() { return FText::AsNumber(BuyQuantity); })
        .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
        {
            BuyQuantity = FMath::Clamp(FCString::Atoi(*Text.ToString()), 1, 99);
        })
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        .Justification(ETextJustify::Center)
    ]
]
+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
[
    SNew(SButton)
    .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
    .OnClicked_Lambda([this]() -> FReply
    {
        BuyQuantity = FMath::Min(99, BuyQuantity + 1);
        return FReply::Handled();
    })
    [
        SNew(STextBlock).Text(FText::FromString(TEXT("+")))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
        .ColorAndOpacity(FSlateColor(FLinearColor::White))
    ]
]
```

### 6.3 Discount/Overcharge Display

Show the effective discount/overcharge percentage in the shop header. The `shop:data` event already sends `discountPercent` and `overchargePercent`. Display them:

```cpp
// In shop header, after shop name
SNew(STextBlock)
.Text_Lambda([Sub]() -> FText
{
    if (!Sub || Sub->DiscountPercent <= 0) return FText::GetEmpty();
    return FText::FromString(FString::Printf(TEXT("Discount: %d%%"), Sub->DiscountPercent));
})
.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.85f, 0.3f, 1.f)))  // Green
```

---

## 7. Anti-Dupe & Security

### 7.1 Atomic Transactions

Every operation that moves items or zeny between entities MUST use PostgreSQL transactions with `FOR UPDATE` row locks:

```javascript
// Pattern: Atomic Item Transfer
async function atomicItemTransfer(fromCharId, toCharId, inventoryId, quantity) {
    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        // Lock source row
        const sourceResult = await client.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped
             FROM character_inventory ci
             WHERE ci.inventory_id = $1 AND ci.character_id = $2
             FOR UPDATE`,
            [inventoryId, fromCharId]
        );
        if (sourceResult.rows.length === 0) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Source item not found' };
        }
        const source = sourceResult.rows[0];

        if (source.is_equipped) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Item is equipped' };
        }
        if (source.quantity < quantity) {
            await client.query('ROLLBACK');
            return { success: false, error: 'Insufficient quantity' };
        }

        // Remove from source
        if (quantity >= source.quantity) {
            await client.query(
                'DELETE FROM character_inventory WHERE inventory_id = $1',
                [inventoryId]
            );
        } else {
            await client.query(
                'UPDATE character_inventory SET quantity = quantity - $1 WHERE inventory_id = $2',
                [quantity, inventoryId]
            );
        }

        // Add to destination (uses existing addItemToInventory helper)
        await addItemToInventory(toCharId, source.item_id, quantity);

        await client.query('COMMIT');
        return { success: true, itemId: source.item_id };
    } catch (err) {
        await client.query('ROLLBACK');
        return { success: false, error: err.message };
    } finally {
        client.release();
    }
}
```

### 7.2 Item ID Validation

NEVER trust client-reported item data. Always validate against the database:

```javascript
// WRONG — trusting client item data:
// const { itemId, itemName, price } = data;  // Client could send anything

// CORRECT — validate from DB:
const dbResult = await pool.query(
    `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
            i.name, i.price, i.stackable, i.weight
     FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
     WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
    [inventoryId, characterId]
);
if (dbResult.rows.length === 0) {
    socket.emit('error', { message: 'Item not found' });
    return;
}
const verifiedItem = dbResult.rows[0]; // Use THIS, not client data
```

### 7.3 Zeny Overflow Protection

The `modifyZeny()` function (Section 1.2) handles this, but every caller must check the return value:

```javascript
// Always check modifyZeny result
const result = await modifyZeny(charId, amount, type, meta);
if (!result.success) {
    socket.emit('error', { message: result.error });
    return;  // DO NOT proceed with the transaction
}
```

**Critical overflow scenarios:**
1. Vending: Vendor's zeny + sale revenue could exceed MAX
2. Trading: Recipient's zeny + trade zeny could exceed MAX
3. NPC sell: Batch sell revenue could exceed MAX
4. Monster kills: Kill zeny could exceed MAX (unlikely but possible with macro farming)

All are handled by `modifyZeny()` checking `newBalance > ZENY.MAX_PER_CHARACTER`.

### 7.4 Trade Logging

Every completed transaction is logged in `zeny_transactions` (Section 1.1) with:
- `character_id`: Who gained/lost
- `transaction_type`: What kind of transaction
- `amount`: How much (positive or negative)
- `balance_after`: Resulting balance (for audit reconciliation)
- `counterpart_id`: The other party (if applicable)
- `metadata`: JSON with itemId, itemName, shopId, etc.
- `zone_name`: Where it happened
- `created_at`: Timestamp

**Additional trade-specific logging** for player-to-player trades:

```sql
-- Future: add to zeny_transactions or separate trade_logs table
-- when implementing player-to-player trading (Section 3 of the design spec)
CREATE TABLE trade_logs (
    trade_id        SERIAL PRIMARY KEY,
    player_a_id     INTEGER NOT NULL REFERENCES characters(character_id),
    player_b_id     INTEGER NOT NULL REFERENCES characters(character_id),
    items_a_gave    JSONB NOT NULL DEFAULT '[]',
    items_b_gave    JSONB NOT NULL DEFAULT '[]',
    zeny_a_gave     BIGINT NOT NULL DEFAULT 0,
    zeny_b_gave     BIGINT NOT NULL DEFAULT 0,
    zone_name       VARCHAR(50),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_trade_log_a ON trade_logs(player_a_id);
CREATE INDEX idx_trade_log_b ON trade_logs(player_b_id);
CREATE INDEX idx_trade_log_time ON trade_logs(created_at);
```

### 7.5 Concurrent Access Prevention

Prevent race conditions with these server-side guards:

```javascript
// 1. Player state flags — checked at the start of every handler
function validatePlayerState(player, allowedStates = {}) {
    if (player.isDead && !allowedStates.allowDead) return 'Cannot do this while dead';
    if (player.isVending && !allowedStates.allowVending) return 'Cannot do this while vending';
    if (player.isBuyingStore && !allowedStates.allowBuyingStore) return 'Cannot do this while buying store is open';
    if (player.isStorageOpen && !allowedStates.allowStorage) return 'Cannot do this while storage is open';
    if (player.isTrading && !allowedStates.allowTrading) return 'Cannot do this while trading';
    return null; // All clear
}

// Usage in every handler:
const stateError = validatePlayerState(player);
if (stateError) {
    socket.emit('some:error', { message: stateError });
    return;
}

// 2. PostgreSQL FOR UPDATE locks prevent two transactions from modifying
//    the same row simultaneously (already shown in modifyZeny and atomicItemTransfer)

// 3. In-memory Maps (activeVendShops, activeBuyingStores, groundItems)
//    are single-threaded in Node.js — no concurrent modification possible
//    within a single event loop tick. Async gaps between awaits are the risk,
//    which is why we always re-validate state after each await.
```

### 7.6 Summary: Security Checklist

| Check | Where | How |
|-------|-------|-----|
| Zeny overflow (gain) | `modifyZeny()` | `newBalance > MAX` check |
| Zeny underflow (spend) | `modifyZeny()` | `newBalance < 0` check |
| Item ownership | Every handler | `character_id` match in SQL WHERE |
| Equipped item protection | Every transfer | `is_equipped = false` check |
| Weight limit | Buy, withdraw, pickup | Weight query before add |
| Slot limit | Buy, withdraw | Slot count query before add |
| Duplicate prevention | Vending, trading | Remove-then-add in single transaction |
| Transaction atomicity | All transfers | `BEGIN` / `COMMIT` / `ROLLBACK` |
| Row locking | Concurrent access | `SELECT ... FOR UPDATE` |
| Audit trail | All zeny changes | `zeny_transactions` table |
| State flags | All handlers | `validatePlayerState()` |
| Client data distrust | All handlers | Always query DB, never trust client |

---

## File Summary

### New Files to Create

| File | Purpose |
|------|---------|
| `database/migrations/add_zeny_bigint_and_logs.sql` | Zeny BIGINT upgrade + transaction log |
| `database/migrations/add_vending_system.sql` | Vending shop + sale log tables |
| `database/migrations/add_storage_system.sql` | Account storage + guild storage tables |
| `client/.../UI/VendingSubsystem.h` | Vending subsystem header |
| `client/.../UI/VendingSubsystem.cpp` | Vending subsystem implementation |
| `client/.../UI/SVendingSetupWidget.h/.cpp` | Shop setup widget (title + item/price selection) |
| `client/.../UI/SVendingBrowseWidget.h/.cpp` | Shop browse widget (item list + buy) |
| `client/.../UI/StorageSubsystem.h` | Storage subsystem header |
| `client/.../UI/StorageSubsystem.cpp` | Storage subsystem implementation |
| `client/.../UI/SStorageWidget.h/.cpp` | Storage grid widget (600-slot grid + deposit/withdraw) |
| `client/.../UI/GroundItemSubsystem.h` | Ground item subsystem header |
| `client/.../UI/GroundItemSubsystem.cpp` | Ground item event handling |
| `client/.../UI/BuyingStoreSubsystem.h/.cpp` | Buying store subsystem (mirrors VendingSubsystem) |

### Existing Files to Modify

| File | Changes |
|------|---------|
| `server/src/index.js` | Add ZENY constants, `modifyZeny()`, vending/storage/ground-item handlers, replace direct zeny SQL |
| `client/.../UI/SKafraWidget.cpp` | Add "Use Storage" button to Kafra menu |
| `client/.../UI/BasicInfoSubsystem.h/.cpp` | (Already tracks Zuzucoin — no changes needed unless upgrading to int64) |
| `client/.../CharacterData.h` | Add `FStorageItem`, `FVendItem`, `FGroundItemData` structs (or keep in subsystem headers) |

### Z-Order Map (Updated)

| Z | Widget |
|---|--------|
| 8 | WorldHealthBarOverlay |
| 10 | BasicInfoWidget |
| 12 | CombatStatsWidget |
| 14 | InventoryWidget |
| 15 | EquipmentWidget |
| 16 | HotbarRowWidget |
| 18 | StorageWidget |
| 19 | KafraWidget |
| 20 | SkillTreeWidget / DamageNumberOverlay |
| 22 | VendingSetupWidget / VendingBrowseWidget / BuyingStore widgets |
| 25 | CastBarOverlay |
| 30 | HotbarKeybindWidget |
| 50 | LoadingOverlay |
