# Cart, Vending, Identification & Change Cart — Full Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Inventory_System](../03_Server_Side/Inventory_System.md) | [Database_Architecture](../01_Architecture/Database_Architecture.md) | [Merchant_Skills_Audit](Merchant_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — CartSubsystem, VendingSubsystem, SIdentifyPopup all implemented

**Date:** 2026-03-16
**Scope:** Skills 604 (Pushcart), 605 (Vending), 606 (Item Appraisal), 607 (Change Cart)
**Verified against:** rAthena source code (pc.cpp, vending.cpp, status.cpp, mmo.hpp), iRO Wiki, iRO Wiki Classic, RateMyServer

---

## System Overview

These 4 systems are interconnected:
```
Pushcart (604) ──► Cart Inventory ──► Vending (605)
                                   ──► Cart Revolution (608) weight scaling
                ──► Cart Speed Penalty
                ──► Change Cart (607) visual
Item Appraisal (606) ──► Identification system (standalone)
```

---

## Phase A: Cart Inventory System (Pushcart 604)

### A1. Constants & Configuration

| Constant | Value | Source |
|----------|-------|--------|
| `MAX_CART_SLOTS` | 100 | rAthena `mmo.hpp`: `#define MAX_CART 100` |
| `MAX_CART_WEIGHT` | 8000 | rAthena `conf/battle/player.conf`: `max_cart_weight: 8000` |
| Cart speed penalty formula | `(100 - (10 - PushcartLv) * 10)%` | iRO Wiki: Lv1=55%, Lv10=100% |
| Classes that can use cart | Merchant, Blacksmith, Alchemist + transcendent equivalents, Super Novice | rAthena `pc.cpp` |
| Cart rental cost | 600z-1,200z (varies by city, see table below) | rAthena Kafra scripts, iRO Wiki |
| Free Ticket (7061) | Item "Free Ticket for Cart Service" can replace zeny | rAthena |
| Cart weight vs char weight | Completely separate — cart items do NOT count toward character overweight | rAthena: separate `cart_weight` tracking |
| Cart items on remove | PRESERVED — items remain in DB, accessible when re-renting | rAthena: cart inventory persists |

### A2. Speed Penalty Table (Pre-Renewal)

| Pushcart Level | Speed % of Normal |
|----------------|-------------------|
| 1 | 55% |
| 2 | 60% |
| 3 | 65% |
| 4 | 70% |
| 5 | 75% |
| 6 | 80% |
| 7 | 85% |
| 8 | 90% |
| 9 | 95% |
| 10 | 100% (no penalty) |

**Formula:** `speedPercent = 50 + (PushcartLevel * 5)`

**rAthena formula:** `speed += speed * (50 - 5 * pushcartLv) / 100;`

**Key behavior:** The speed penalty stacks **multiplicatively** with other speed bonuses. Example: 120% from Increase AGI * 55% from Pushcart Lv1 = 66% actual speed (not 75%).

#### Cart Rental Costs by City

| City | Cost |
|------|------|
| Aldebaran | 600z |
| Amatsu/Kunlun/Louyang | 700z |
| Geffen | 750z |
| Prontera/Juno/Einbroch/Lighthalzen/Hugel/Rachel | 800z |
| Izlude | 820z |
| Alberta | 850z |
| Payon/Morroc | 930z |
| Comodo | 1,000z |
| Archer Village | 1,200z |

### A3. Database Schema

#### New Table: `character_cart`
```sql
CREATE TABLE IF NOT EXISTS character_cart (
    cart_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    slot_index INTEGER DEFAULT -1,
    identified BOOLEAN DEFAULT true,
    refine_level INTEGER DEFAULT 0,
    card0 INTEGER DEFAULT 0,
    card1 INTEGER DEFAULT 0,
    card2 INTEGER DEFAULT 0,
    card3 INTEGER DEFAULT 0,
    attribute INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_cart_character ON character_cart(character_id);
```

#### Character Columns (auto-create at startup)
```sql
ALTER TABLE characters ADD COLUMN IF NOT EXISTS has_cart BOOLEAN DEFAULT false;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS cart_type INTEGER DEFAULT 0;
```

### A4. Server Implementation

#### Player State Fields
```javascript
// Added to player object on player:join
player.hasCart = false;          // Whether cart is equipped
player.cartType = 0;            // Cart visual type (0=none, 1-5=appearance)
player.cartWeight = 0;          // Current cart weight
player.cartMaxWeight = 8000;    // Max cart weight (constant)
player.cartItems = [];          // Cart inventory (loaded from DB)
player.cartSlotCount = 0;       // Current number of items in cart
```

#### Socket Events

| Event | Direction | Data | Description |
|-------|-----------|------|-------------|
| `cart:rent` | C→S | `{ kafraId }` | Rent cart from Kafra NPC |
| `cart:remove` | C→S | `{}` | Remove cart (return to Kafra) |
| `cart:data` | S→C | `{ items[], cartWeight, cartMaxWeight }` | Full cart inventory |
| `cart:move_to_cart` | C→S | `{ inventoryId, amount }` | Move item from inventory to cart |
| `cart:move_to_inventory` | C→S | `{ cartId, amount }` | Move item from cart to inventory |
| `cart:equipped` | S→C | `{ hasCart, cartType }` | Cart state broadcast |
| `cart:weight_update` | S→C | `{ cartWeight, cartMaxWeight }` | Cart weight changed |

#### Cart Rent Handler (`cart:rent`)
```
1. Check player has Pushcart skill (604) learned
2. Check player class is Merchant/Blacksmith/Alchemist/etc.
3. Check player doesn't already have cart (player.hasCart)
4. Deduct rental cost (800z default)
5. Set player.hasCart = true
6. Set player.cartType = 1 (default cart)
7. Update DB: characters SET has_cart = true
8. Apply speed penalty based on Pushcart level
9. Emit cart:equipped to player and zone
10. Load cart inventory from DB (character_cart)
```

#### Cart Remove Handler (`cart:remove`)
```
1. Check player has cart
2. Check player is NOT vending (cannot remove cart while shop is open)
3. Set player.hasCart = false, player.cartType = 0
4. Update DB: characters SET has_cart = false, cart_type = 0
5. Remove speed penalty
6. Emit cart:equipped to zone
NOTE: Cart items are PRESERVED in DB — player can access them again when re-renting
NOTE: Cart appearance reverts to type 1 on next rent (Change Cart must be reused)
```

#### Move to Cart Handler (`cart:move_to_cart`)
```
1. Validate item exists in player inventory
2. Check player has cart
3. Check cart weight: currentWeight + (itemWeight * amount) <= 8000
4. Check cart slots: cartSlotCount < 100
5. Remove from character_inventory (or reduce quantity)
6. Add to character_cart (or increase quantity if stackable)
7. Update player.cartWeight
8. Emit cart:data + inventory:data to player
9. Update cached character weight
```

#### Move to Inventory Handler (`cart:move_to_inventory`)
```
1. Validate item exists in cart
2. Check player has cart
3. Check player weight: currentWeight + (itemWeight * amount) <= maxWeight
4. Check player inventory slots available
5. Remove from character_cart (or reduce quantity)
6. Add to character_inventory (or increase quantity if stackable)
7. Update player.cartWeight
8. Emit cart:data + inventory:data to player
9. Update cached character weight
```

#### Speed Penalty Integration
```javascript
// In movement speed calculation (wherever speed is computed):
function getEffectiveMovementSpeed(player) {
    let speed = player.baseSpeed || 150; // Default RO move speed

    // Apply cart speed penalty
    if (player.hasCart) {
        const pushcartLv = (player.learnedSkills || {})[604] || 0;
        const cartSpeedPercent = 50 + (pushcartLv * 5); // 55-100%
        speed = Math.floor(speed * (100 / cartSpeedPercent)); // Higher = slower
    }

    // Other speed modifiers (Increase AGI, etc.)
    // ...

    return speed;
}
```

#### Cart Revolution Integration
```javascript
// Already wired — just needs player.cartWeight to be populated
// Line ~9518: const cartWeight = player.cartWeight || 0;
// totalEffectVal = effectVal + Math.floor(100 * cartWeight / 8000);
```

#### Cart State Check for Cart Revolution
```javascript
// Add to Cart Revolution handler (before damage calc):
if (!player.hasCart) {
    socket.emit('skill:error', { message: 'Cart Revolution requires a Pushcart' });
    return;
}
```

### A5. Cart Persistence

- Cart inventory saved to `character_cart` table
- `has_cart` and `cart_type` saved on `characters` table
- Cart persists across zone changes (loaded on `player:join`)
- Cart persists across disconnects (DB-backed)
- On disconnect: cart state is already in DB (no special save needed since each move is immediate)

---

## Phase B: Vending System (Skill 605)

### B1. Pre-Renewal Vending Mechanics

| Property | Value | Source |
|----------|-------|--------|
| SP Cost | 30 | rAthena `skill_db.yml` |
| Max items per level | `2 + SkillLevel` (3 to 12) | rAthena `vending.cpp`: `count > 2 + vending_skill_lvl` |
| Requirements | Pushcart Lv3 + Cart equipped + Items in cart | rAthena: `pc_iscarton(sd)` check |
| Shop title max length | 80 characters | rAthena `MESSAGE_SIZE` = 80 |
| Vending tax | 0% for pre-renewal classic | Configurable; iRO added 5% later for items > 10M |
| Max price per item | 1,000,000,000z (1 billion) | rAthena `vending_max_value: 1000000000` |
| NPC distance | Must be 3+ cells from any NPC | rAthena `min_npc_vendchat_distance: 3` (iRO Wiki says 4, rAthena default is 3) |
| Overcharge/Discount | Do NOT affect vending prices | iRO Wiki: purely player-set prices |
| SP on cancel | SP consumed even if player cancels setup | rAthena: SP consumed when UI opens (`state.prevend`) |
| Search Store | NOT in pre-renewal (Renewal feature) | rAthena |
| Buying Store | NOT in pre-renewal (Renewal feature) | rAthena |
| Can vendor move? | No — immobile while vending | rAthena: `sd->state.vending` blocks movement |
| Can vendor chat? | Yes (public chat works) | Standard RO behavior |
| Can vendor be attacked? | Yes in PvP maps, no in towns | Standard RO behavior |
| Items sold from | Cart inventory (NOT player inventory) | rAthena: `sd->cart.u.items_cart[idx]` |
| Unidentified items | Cannot be vended | rAthena: `!sd->cart.u.items_cart[index].identify` check |
| Broken items | Cannot be vended | rAthena: `sd->cart.u.items_cart[index].attribute == 1` check |

### B2. Vending Slot Table

| Vending Level | Max Items |
|---------------|-----------|
| 1 | 3 |
| 2 | 4 |
| 3 | 5 |
| 4 | 6 |
| 5 | 7 |
| 6 | 8 |
| 7 | 9 |
| 8 | 10 |
| 9 | 11 |
| 10 | 12 |

### B3. Database Schema

#### New Table: `vending_shops`
```sql
CREATE TABLE IF NOT EXISTS vending_shops (
    shop_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    title VARCHAR(80) NOT NULL DEFAULT 'Shop',
    zone VARCHAR(50) NOT NULL,
    x REAL NOT NULL,
    y REAL NOT NULL,
    z REAL DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX idx_vending_zone ON vending_shops(zone);
```

#### New Table: `vending_items`
```sql
CREATE TABLE IF NOT EXISTS vending_items (
    vend_item_id SERIAL PRIMARY KEY,
    shop_id INTEGER REFERENCES vending_shops(shop_id) ON DELETE CASCADE,
    cart_slot_index INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    price INTEGER NOT NULL,
    CONSTRAINT price_positive CHECK (price > 0 AND price <= 1000000000)
);
```

### B4. Socket Events

| Event | Direction | Data | Description |
|-------|-----------|------|-------------|
| `vending:open` | C→S | `{ title, items: [{cartSlot, amount, price}] }` | Open a vending shop |
| `vending:close` | C→S | `{}` | Close vending shop |
| `vending:shop_opened` | S→C (zone) | `{ characterId, playerName, title, x, y, z, shopId }` | Broadcast new shop to zone |
| `vending:shop_closed` | S→C (zone) | `{ characterId, shopId }` | Broadcast shop closed |
| `vending:browse` | C→S | `{ vendorId }` | Request item list from a vendor |
| `vending:item_list` | S→C | `{ shopId, vendorName, items: [{itemId, name, amount, price, ...}] }` | Vendor's item list |
| `vending:buy` | C→S | `{ vendorId, items: [{index, amount}] }` | Buy items from vendor |
| `vending:buy_result` | S→C | `{ success, message, totalCost }` | Purchase result |
| `vending:sold` | S→C (vendor) | `{ buyerName, itemName, amount, price }` | Notify vendor of sale |
| `vending:shops_in_zone` | S→C | `{ shops: [{characterId, title, x, y}] }` | All vendors in current zone |

### B5. Vending Open Handler (`vending:open`)
```
1. Check player has Vending skill (605) learned
2. Check player has cart (player.hasCart)
3. Check player is not already vending
4. Check player is not dead, in combat, or trading
5. Check SP >= 30, deduct SP
6. Check item count: 1 <= count <= (2 + vendingLevel)
7. Validate each item:
   a. Item exists in cart at specified slot
   b. Item is identified
   c. Item is not broken (attribute != 1)
   d. Price > 0 and <= 1,000,000,000
   e. Amount <= available quantity in cart
8. Check NPC distance >= 4 cells (200 UE)
9. Set player.isVending = true
10. Store vending state in memory:
    player.vendingShop = { title, items: [...], shopId }
11. Insert into vending_shops DB
12. Insert items into vending_items DB
13. Broadcast vending:shop_opened to zone
14. Block player movement (set isVending flag)
```

### B6. Vending Buy Handler (`vending:buy`)
```
1. Find vendor by vendorId
2. Check vendor is still vending (player.isVending)
3. Check buyer != vendor (can't buy from self)
4. Check buyer is in same zone and within range (AREA_SIZE cells)
5. For each purchase item:
   a. Validate item still available (amount check)
   b. Calculate total cost
6. Check buyer has enough zeny
7. Check buyer has enough inventory space
8. Check buyer weight capacity
9. Process transaction:
   a. Deduct zeny from buyer
   b. Add zeny to vendor (minus optional tax)
   c. Transfer items from vendor's cart to buyer's inventory
   d. Update vendor's cart state
   e. Update vending_items DB (reduce amount or delete)
10. Emit vending:buy_result to buyer
11. Emit vending:sold to vendor
12. Update DB for both players
13. If all vending items sold out → auto-close shop
```

### B7. Vending Close Handler (`vending:close`)
```
1. Check player is vending
2. Set player.isVending = false
3. Delete from vending_shops and vending_items DB
4. Restore player movement
5. Broadcast vending:shop_closed to zone
```

### B8. Vending State Integration

While vending, the player:
- Cannot move (movement input ignored)
- Cannot use skills
- Cannot attack or be auto-attacked (in town)
- CAN receive chat messages and send chat
- CAN be clicked on by other players (opens browse)
- Shows a chat bubble with shop title above head

---

## Phase C: Item Appraisal / Identification System (Skill 606)

### C1. Identification Mechanics

| Property | Value | Source |
|----------|-------|--------|
| SP Cost | 10 | Skill data |
| Max Level | 1 | Skill data |
| Identifies | One item per use | iRO Wiki |
| Magnifier | Item ID 611, buyable from tool shops, same function | rAthena |
| Which items drop unidentified | Weapons (IT_WEAPON), Armor/equipment (IT_ARMOR), Pet armor (IT_PETARMOR) — NOT consumables, cards, misc, ammo | rAthena `itemdb_isidentified()`: types 4, 5, 12 |
| Magnifier | Item ID 611, buy 40z, weight 5, consumed on use, casts MC_IDENTIFY Lv1 | rAthena item script: `itemskill "MC_IDENTIFY", 1;` |
| Old Blue/Violet Box | Equipment from random boxes also drops unidentified | rAthena uses `itemdb_isidentified()` on creation |

### C2. Unidentified Item Rules

| Action | Unidentified Item | Source |
|--------|-------------------|--------|
| Equip | BLOCKED | rAthena |
| Sell to NPC | Allowed (at reduced price) | Standard RO |
| Use | N/A (only equipment is unidentified) | N/A |
| Drop | Allowed | Standard RO |
| Trade | BLOCKED | rAthena blocks trading unidentified items |
| Vend | BLOCKED | rAthena `vending.cpp` |
| Card compound | BLOCKED | Standard RO |
| Refine | BLOCKED | Standard RO |
| Storage | Allowed | Standard RO |
| Cart | Allowed | Standard RO |

### C3. What Players See

**Unidentified items show:**
- Item name (generic or "Unknown Item")
- Weight (hidden or shown — varies by server)
- Type indicator (weapon/armor)
- "Unidentified" label

**After identification:**
- Full item name with prefix/suffix
- All stats (ATK, DEF, slots, etc.)
- Full description
- Card slots visible

### C4. Database Changes

```sql
-- Add identified column to character_inventory
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS identified BOOLEAN DEFAULT true;

-- Add identified column to character_cart
ALTER TABLE character_cart
    ADD COLUMN IF NOT EXISTS identified BOOLEAN DEFAULT true;

-- Items table already exists, no changes needed
```

**Note:** New items added via `addItemToInventory()` set `identified = true` by default. Only monster drops of equipment (type weapon/armor) set `identified = false` when the server config enables it.

### C5. Implementation

#### Server Config
```javascript
// Whether equipment drops as unidentified (configurable)
const IDENTIFY_CONFIG = {
    weaponDropsUnidentified: true,   // Weapons drop unidentified
    armorDropsUnidentified: true,    // Armor drops unidentified
    accessoryDropsUnidentified: false, // Accessories drop identified
    cardDropsUnidentified: false,    // Cards always identified
    shopItemsIdentified: true,       // NPC shop items always identified
    questItemsIdentified: true,      // Quest reward items always identified
};
```

#### Skill Handler (`item_appraisal`) — Two-Step Flow

**Step 1: Skill cast → Server sends unidentified item list**
```
1. Check player has Item Appraisal skill (606) or used Magnifier (611)
2. Deduct SP (10) — or consume 1 Magnifier
3. Scan player inventory for all items where identified = false
4. If none found → emit skill:error "No unidentified items"
5. Emit identify:item_list with array of {inventoryId, genericName, slotIndex}
6. Emit skill:used
```

**Step 2: Player selects item → Server identifies it**
```
1. Player picks ONE item from the list
2. Client sends identify:select with {inventoryId}
3. Validate item exists and is unidentified
4. Set identified = true in DB
5. Emit identify:result with full item data (name, stats, slots revealed)
6. Emit inventory:data with updated item
```

#### Magnifier Item Handler
```
1. Player uses Magnifier (item 611) from inventory
2. Client sends target inventory slot
3. Check target item is unidentified
4. Set identified = true in DB
5. Consume 1 Magnifier
6. Emit inventory:data
```

#### Monster Drop Integration
```javascript
// In processEnemyDeathFromSkill() and auto-attack death handler:
// When creating dropped items:
if (IDENTIFY_CONFIG.weaponDropsUnidentified && itemDef.type === 'weapon') {
    newItem.identified = false;
} else if (IDENTIFY_CONFIG.armorDropsUnidentified && itemDef.type === 'armor') {
    newItem.identified = false;
} else {
    newItem.identified = true;
}
```

#### Equip Block
```javascript
// In equipment:equip handler:
if (!item.identified) {
    socket.emit('skill:error', { message: 'Must identify item before equipping' });
    return;
}
```

---

## Phase D: Change Cart (Skill 607)

### D1. Cart Appearance Table (Pre-Renewal)

| Cart Type | Base Level Range | Description |
|-----------|-----------------|-------------|
| 1 | 1-40 | Basic wooden cart |
| 2 | 41-65 | Reinforced cart |
| 3 | 66-80 | Iron cart |
| 4 | 81-90 | Steel cart |
| 5 | 91+ | Normal cart with bigger wheels, roof, and banner |

**Source:** rAthena `MAX_CARTS = 5` (pre-renewal), confirmed by `clif_parse_ChangeCart` and scripting thresholds.

**Notes:**
- All merchant-line classes share the same 5 cart sprites (no class-specific carts)
- Cart reverts to **type 1** when removed and re-rented (default on equip)
- Cart Decoration (`MC_CARTDECORATE`, 2017) is NOT pre-renewal
- Change Cart Lv1 is prerequisite for Whitesmith's Cart Boost (`WS_CARTBOOST`)
- **Quest NPC:** Charlron in Alberta (119, 221) — requires 50 Trunks + 10 Iron + 20 Animal Skins + Job Level 30

### D2. Skill Mechanics

| Property | Value |
|----------|-------|
| SP Cost | 40 |
| Max Level | 1 (quest skill) |
| Cast Time | 0 |
| Cooldown | 0 |
| Requirement | Must have cart equipped |
| Type | Self-target utility |

### D3. Implementation

#### Skill Handler (`change_cart`)
```
1. Check player has Change Cart skill (607)
2. Check player has cart (player.hasCart)
3. Deduct SP (40)
4. Determine cart type from base level:
   cartType = baseLv >= 91 ? 5 : baseLv >= 81 ? 4 : baseLv >= 66 ? 3 : baseLv >= 41 ? 2 : 1;
5. If cartType === player.cartType → no change needed
6. Update player.cartType = cartType
7. Update DB: characters SET cart_type = cartType
8. Broadcast cart:equipped to zone (includes new cartType)
9. Emit skill:used
```

#### Auto-Update on Level Up
```javascript
// Optional: auto-update cart type when player levels up (if they have Change Cart skill)
if (player.hasCart && (player.learnedSkills[607] || 0) > 0) {
    const newCartType = calculateCartType(player.baseLv);
    if (newCartType !== player.cartType) {
        player.cartType = newCartType;
        // Update DB and broadcast
    }
}
```

---

## Phase E: Client Integration

### E1. Cart UI (Slate Widget)

**New file:** `UI/SCartWidget.*`
- 100-slot grid (similar to inventory)
- Shows cart weight bar (current/8000)
- "Move to Inventory" / "Move to Cart" buttons
- Drag-and-drop between inventory and cart
- Hotkey: Alt+W (standard RO hotkey)

### E2. Vending UI

**New files:** `UI/SVendingSetupWidget.*`, `UI/SVendingBrowseWidget.*`

**Setup Widget (when opening shop):**
- Title input field (80 char max)
- Cart item list with quantity and price inputs
- "Open Shop" / "Cancel" buttons
- Shows max items allowed (2 + Vending level)

**Browse Widget (when clicking on a vendor):**
- Shows vendor name and shop title
- Item list with names, quantities, prices
- "Buy" button per item with quantity selector
- Total cost display

### E3. Vending Visual

- Vendor shows a chat bubble with shop title
- Vendor character sits on ground (or stands with cart visible)
- Vendor name tag shows "[Vending]" indicator
- Other players see a clickable shop icon

### E4. EquipmentSubsystem Integration

- Cart state tracked in EquipmentSubsystem
- Cart visual attached to player character
- Speed penalty applied via movement speed modifier

---

## Implementation Order

### Step 1: Database Migration
1. Create `character_cart` table
2. Add `has_cart`, `cart_type` columns to `characters`
3. Add `identified` column to `character_inventory`
4. Create `vending_shops` and `vending_items` tables

### Step 2: Cart Core (Phase A)
1. Player state fields (`hasCart`, `cartWeight`, etc.)
2. Load cart on `player:join`
3. `cart:rent` / `cart:remove` handlers
4. `cart:move_to_cart` / `cart:move_to_inventory` handlers
5. Cart weight tracking and updates
6. Speed penalty integration
7. Cart Revolution cart check + weight scaling activation
8. Kafra NPC cart rental service

### Step 3: Identification System (Phase C)
1. `identified` column migration
2. Monster drop identification flag
3. Equip block for unidentified items
4. Item Appraisal skill handler
5. Magnifier item handler
6. Vending block for unidentified items

### Step 4: Change Cart (Phase D)
1. Cart type calculation from base level
2. Skill handler
3. Auto-update on level up

### Step 5: Vending System (Phase B)
1. Vending state management
2. `vending:open` handler with all validations
3. `vending:browse` / `vending:item_list` handlers
4. `vending:buy` handler with full transaction flow
5. `vending:close` handler
6. Auto-close on all items sold
7. Zone broadcast for shop visibility

### Step 6: Client UI (Phase E — deferred)
1. SCartWidget (cart inventory UI)
2. SVendingSetupWidget (shop creation UI)
3. SVendingBrowseWidget (shop browsing UI)
4. Cart visual on player character
5. Vending chat bubble / shop indicator

---

## Cross-System Integration Points

| System | Integration |
|--------|-------------|
| Cart Revolution (608) | `player.cartWeight` now populated; `player.hasCart` check added |
| Kafra NPC | New "Rent Cart" / "Return Cart" service options |
| Weight system | Cart items have separate weight from inventory |
| Movement | Speed penalty while cart equipped |
| Equipment | Cannot equip unidentified items |
| Monster drops | Equipment drops unidentified (configurable) |
| NPC shops | Shop items always identified |
| Refining | Cannot refine unidentified items |
| Card compound | Cannot compound into unidentified items |
| Vending | Items must be in cart + identified |
| Zone transitions | Cart state persists (DB-backed) |
| Disconnect | Cart state persists (DB-backed) |
| Skill reset | Does NOT remove cart (cart is rental, not skill-dependent) |

---

## Validation Checklist

### Pushcart
- [ ] Cart rented from Kafra for 800z
- [ ] Cart has 100 slots, 8000 weight
- [ ] Speed penalty: 55% (Lv1) to 100% (Lv10)
- [ ] Cart persists across zones
- [ ] Cart persists across disconnects
- [ ] Cart items preserved when cart removed (re-accessible on re-rent)
- [ ] Cart appearance reverts to type 1 on re-rent
- [ ] Cart weight displayed in UI
- [ ] Cart Revolution damage scales with cart weight
- [ ] Cart Revolution requires cart equipped

### Vending
- [ ] Requires Pushcart Lv3 + cart equipped
- [ ] Items sold from cart (not inventory)
- [ ] Slot limit: 2 + Vending level
- [ ] SP cost: 30
- [ ] Shop title: max 80 chars
- [ ] Vendor cannot move while vending
- [ ] Other players can browse and buy
- [ ] Zeny transferred from buyer to vendor
- [ ] Items transferred from vendor cart to buyer inventory
- [ ] Auto-close when all items sold
- [ ] Cannot vend unidentified or broken items
- [ ] Must be 4+ cells from NPCs
- [ ] Vendor visible in zone with shop title

### Item Appraisal
- [ ] SP cost: 10
- [ ] Identifies one item per use
- [ ] Magnifier (611) works the same way
- [ ] Equipment drops as unidentified
- [ ] Cannot equip unidentified items
- [ ] Cannot vend unidentified items
- [ ] Cannot refine unidentified items
- [ ] Cannot compound cards into unidentified items

### Change Cart
- [ ] SP cost: 40
- [ ] Quest skill (max level 1)
- [ ] Cart type based on base level (5 types)
- [ ] Requires cart equipped
- [ ] Visual updates for zone players

---

## Sources

| Source | URL | Used For |
|--------|-----|----------|
| rAthena `mmo.hpp` | github.com/rathena/rathena | `MAX_CART=100`, `MAX_CARTS=5` |
| rAthena `vending.cpp` | github.com/rathena/rathena | Full vending purchase/open flow, tax calculation |
| rAthena `vending.hpp` | github.com/rathena/rathena | `s_vending` struct (index, amount, value) |
| rAthena `status.cpp` | github.com/rathena/rathena | `status_calc_cart_weight()`, cart speed penalty |
| rAthena `pc.cpp` | github.com/rathena/rathena | `pc_setcart()`, cart add/remove, class checks |
| rAthena `player.conf` | github.com/rathena/rathena | `max_cart_weight: 8000` |
| rAthena `items.conf` | github.com/rathena/rathena | `vending_tax: 500`, `vending_max_value: 1000000000` |
| iRO Wiki: Pushcart | irowiki.org/wiki/Pushcart | Speed table, weight capacity, rental |
| iRO Wiki: Vending | irowiki.org/wiki/Vending | Slot table, tax, NPC distance, requirements |
| iRO Wiki: Item Appraisal | irowiki.org/wiki/Item_Appraisal | SP cost, identification mechanics |
| RateMyServer | ratemyserver.net | Skill data verification |
