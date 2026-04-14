# Ground Item & Drop System — Complete Research & Implementation Plan

**Date:** 2026-04-09
**Scope:** Monster item drops, ground item entities, drop ownership, pickup mechanics, player item dropping, and all cross-system integrations.
**Goal:** 100% RO Classic pre-renewal feature parity.

---

## Table of Contents

1. [RO Classic Research Findings](#1-ro-classic-research-findings)
2. [Current System Audit](#2-current-system-audit)
3. [Implementation Plan](#3-implementation-plan)
4. [Socket Event Specification](#4-socket-event-specification)
5. [Server Data Structures](#5-server-data-structures)
6. [Client Architecture (UE5 C++)](#6-client-architecture-ue5-c)
7. [Edge Cases & Exploit Prevention](#7-edge-cases--exploit-prevention)
8. [Progress Tracking Checklist](#8-progress-tracking-checklist)

---

## 1. RO Classic Research Findings

### 1.1 Monster Drop Tables

Each monster has up to **10 regular drop slots** and **3 MVP drop slots** (MVPs only).

**Slot convention (data convention, not code-enforced):**
- Slots 1-8: Normal item drops (loot, equipment, consumables)
- Slot 9: Card drop (conventionally 0.01% / 1-in-10000)
- Slot 10: Occasionally used for additional drops
- MVP slots 1-3: Separate pool, MVP killer only

**Drop rate format:** Per-10000 basis where 10000 = 100%.

| Rate Value | Percentage |
|-----------|-----------|
| 10000 | 100.00% |
| 5000 | 50.00% |
| 1000 | 10.00% |
| 100 | 1.00% |
| 10 | 0.10% |
| 1 | 0.01% |

**Our format:** Our `ro_monster_templates.js` stores rates as percentages (0-100) with `drop.chance` used as `Math.random() < drop.chance`. E.g., `rate: 0.01` = 0.01% (card), `rate: 70` = 70%.

**Card drop mechanics:**
- Standard card rate: 0.01% (1 in 10,000 kills)
- Cards are conventionally in slot 9
- Cards have `stealProtected: true` — cannot be taken via Steal skill
- Some mini-boss cards have slightly higher rates (0.02%-0.05%)
- MVP cards are in the regular drop table (not MVP drop slots), typically at 0.01%

**MVP reward drops:**
- Up to 3 MVP drop slots, each rolled independently
- MVP drops go **directly into MVP winner's inventory** — NOT on the ground
- If MVP winner's inventory is full or overweight, items drop to ground
- MVP drops **cannot be stolen**
- MVP drop rates are NOT affected by Bubble Gum or server rate multipliers by default

**Drop rate modifiers:**

| Modifier | Effect | Duration |
|---------|--------|----------|
| Bubble Gum (ID 12210) | +100% (doubles base rate) | 30 minutes |
| HE Bubble Gum | +200% (triples base rate) | 15 minutes |
| Equipment bonuses | +5% to +10% | While equipped |

**Formula:** `FinalRate = min(BaseRate * (1.0 + sum_of_bonuses), DROP_CAP)`

**Drop rate caps and floors:**

| Setting | Value | Purpose |
|---------|-------|---------|
| Drop rate cap | 90% (9000/10000) | Maximum boosted rate — a 50% base with Bubble Gum caps at 90%, not 100% |
| Minimum rate | 0.01% (1/10000) | Cards and rare items cannot go below 0.01% |
| Maximum rate | 100% (10000/10000) | Hard ceiling |

**Item quantity drops:** Most items drop as quantity 1. Some items can drop in multiples (arrows, gemstones) — supported by `minQty`/`maxQty` fields in drop entries.

**When drops are calculated:** On monster death, AFTER EXP distribution. The `rollEnemyDrops()` function iterates all drop slots, performs an independent `Math.random()` check per slot.

**Drops on despawn:** Monsters that despawn (timeout, no damage) do NOT drop items. Only death from player damage triggers drops.

### 1.2 Drop Ownership & Looting Rights

**Ownership determination:** Based on **most total damage dealt** (not first-hit or last-hit). The server tracks damage per attacker via `enemy.inCombatWith` (Map<charId, {totalDamage}>). The player who dealt the most cumulative damage gets first priority.

**For MVPs:** MVP winner is highest `totalDamage` in `inCombatWith`.

**Ownership timer duration — Normal Monsters:**

| Phase | Duration | Cumulative | Who Can Loot |
|-------|----------|------------|--------------|
| 1st priority | 3,000ms | 0-3s | Only #1 damage dealer |
| 2nd priority | 2,000ms | 3-5s | Top 2 damage dealers |
| 3rd priority | 2,000ms | 5-7s | Top 3 damage dealers |
| Free-for-all | remaining | 7s-60s | Anyone in zone |

**Ownership timer duration — MVP/Boss Monsters:**

| Phase | Duration | Cumulative | Who Can Loot |
|-------|----------|------------|--------------|
| 1st priority | 10,000ms | 0-10s | Only MVP winner |
| 2nd priority | 10,000ms | 10-20s | Top 2 damage dealers |
| 3rd priority | 2,000ms | 20-22s | Top 3 damage dealers |
| Free-for-all | remaining | 22s-60s | Anyone in zone |

**Party looting modes (two independent settings):**

**Item Acquisition ("Who Can Loot"):**
- **Each Take**: Drop priority system applies. Only the player(s) with appropriate damage rank can pick up during priority phases.
- **Party Share**: Any party member can immediately pick up items regardless of drop priority. Bypasses the ownership timer entirely.

**Item Distribution ("Where Loot Goes"):**
- **Individual**: The player who physically picks up keeps the item.
- **Shared**: Acquired items randomly assigned to a party member within 15 base levels.

**Owner disconnects/leaves map:** Ownership timer continues counting down. After expiry, item becomes free-for-all. Not transferred early.

**Ownership cannot be manually transferred.**

### 1.3 Ground Item Entities

**Visual representation:**
- Each item displays its actual sprite/icon on the ground (NOT a generic bag)
- Billboard-style rendering (always faces camera)
- Item name label displayed below the sprite, **always visible** (no hover required)

**Despawn timer:** 60,000ms (60 seconds) from the moment the item hits the ground. Same for monster drops and player drops. After expiry, item vanishes permanently.

**Maximum ground items per map:** No hard cap (server performance dependent). Practical limit is hundreds per zone before issues arise.

**Stacking behavior:** Ground items do **NOT stack**. Each drop roll creates a separate ground item entity, even for identical items.

**Z-position:** Items rest flat on the ground at terrain Z-level. They do not float.

**Visibility range:** All items in the zone are visible to all connected players (our zones are small enough for full visibility — no culling needed).

**Collision:** Items are fully walkable — no collision. Players and monsters pass through them.

**Terrain interaction:** Items drop at the ground Z-height at their scatter position, regardless of slope or water.

### 1.4 Drop Animation & Presentation

**Scatter pattern from monster death (rAthena mob.cpp):**
1. **First item**: Drops at the **exact death cell** of the monster
2. **Subsequent items**: Cycle through a repeating pattern: **SE → W → N → SE → W → N...**
3. Each direction offset is ~1 cell (approximately 50 UE units) from the death position

**Obstacle handling:** If the target cell is blocked (wall, ice wall), search a 3x3 area around the target for a free cell. Items cannot drop on walls or impassable cells.

**Landing animation:** Items "pop out" from the death position with a short upward arc, then settle on the ground. Brief toss + settle, no prolonged bounce.

**Sound effects:** Our server already implements tier-based drop sounds via `getDropTierColor()`:
- Red: MVP drops
- Purple: Cards
- Blue: Equipment (weapons/armor)
- Green: Healing items
- Yellow: Usable items, ammo
- Pink: Misc/materials

Client plays `drop_<color>.wav` from `/Game/SabriMMO/Audio/SFX/Drops/`.

**Drop announcement system:**

| Event | Scope | Message |
|-------|-------|---------|
| MVP kill | Server-wide | `[MVP] {PlayerName} has defeated {MonsterName}!` |
| Card drop | None in Classic | (Some servers add card announcements — we can add as option) |
| Regular drops | None | Items silently appear on ground |

**Chat messages on pickup:** `Obtained {quantity}x {ItemName}` in system/chat window.

### 1.5 Player Dropping Items

**How to drop:** Drag item from inventory window and release outside the window boundary. For stackable items, a quantity selector popup appears.

**Undropable items (NoDrop flag):**
- Quest items
- Cash shop / rental items
- Account-bound / character-bound items
- Items table needs a `nodrop` column (or use existing `notrade` flags — in RO, NoDrop is part of the Trade restriction bitmask)

**State restrictions:**

| State | Can Drop? |
|-------|-----------|
| Normal standing/walking | Yes |
| In combat | Yes (between attacks) |
| Dead | No |
| Sitting | No |
| Trading | No |
| Casting | No |
| Vending | No |
| Storage open | No |

**Cannot drop equipped items** — must unequip first.

**Cannot drop zeny** — zeny only transfers via trade, NPC shops, or skill costs.

**Partial stack dropping:** Yes — quantity selector allows dropping 1 to full stack.

**Drop position:** At the player's current position (feet).

### 1.6 Picking Up Items

**Click-to-pickup:** Click the ground item → character auto-walks to it → pickup triggers when within range.

**Pickup range:** 1-2 cells (~100 UE units, `PICKUP_RANGE`).

**No /pickupall in vanilla Classic.** This is a private server feature only.

**Pickup priority for overlapping items:** Nearest / topmost item detected by click. No explicit priority system.

**Inventory full behavior:** Pickup refused with message: "Cannot carry more items."

**Weight limit behavior:** Pickup refused with message: "Item is too heavy." Check: `currentWeight + (itemWeight * quantity) > maxWeight`.

**Pickup animation:** Brief pickup gesture (character stops, short animation).

**Pickup sound:** Generic "item get" sound effect.

**State restrictions on pickup:**

| State | Can Pick Up? |
|-------|-------------|
| Normal standing/walking | Yes |
| Auto-attacking | Yes (between attacks) |
| Dead | No |
| Sitting | No |
| Casting | No |
| Vending | No |
| Trading | No |
| ≥90% overweight | No |
| ≥100% weight | No |

**@autoloot is NOT vanilla Classic.** Private server / GM command only. Not implementing.

### 1.7 Greed Skill (Blacksmith)

| Property | Value |
|----------|-------|
| Skill ID | 1210 (BS_GREED) |
| Type | Active (Quest Skill) |
| Max Level | 1 |
| AoE | 5x5 cells (2-cell radius centered on caster) |
| SP Cost | 10 |
| After-Cast Delay | 1000ms |
| Prerequisite | Job Level 30+ |
| Restrictions | Cannot use in towns, PvP, or WoE maps |

**Effect:** Instantly picks up ALL ground items within the 5x5 area around the caster. No ownership bypass — still respects ownership timers.

### 1.8 Steal Skill Interaction

Thief Steal (ID 502) takes an item directly from the monster's drop table into the player's inventory, **bypassing ground drops entirely**.
- Does NOT remove the item from the death drop table — monster can still drop the same item on death
- `stealProtected: true` items (cards) cannot be stolen
- Boss/MVP monsters are immune to Steal
- Only one successful steal per monster (global lock via `enemy.stealLocked`)
- If inventory full on steal success, item is **lost** (vanishes)

### 1.9 PvP / Death Item Drops

**PvE death:** Players do NOT drop items. Only EXP penalty (1% base EXP).

**Standard PvP death:** Players do NOT drop items. EXP loss applies.

**Nightmare mode (optional server config):** Configurable chance to drop random inventory/equipped item on death. **Disabled on official servers.** Not implementing for now.

**WoE/GvG death:** No EXP penalty, no item drops.

### 1.10 Special Mechanics

**Looter Monsters (MD_LOOTER flag):**
- Monsters with `looter: true` (AI codes 02, 07) pick up ground items during idle state
- Examples: Poring, Poporing, Yoyo, Thief Bug, Verit
- When a Looter monster is killed, looted items drop along with normal drops
- Our `enemy.mode.looter` flag already parsed from templates (line 449)

**Treasure Box Monsters:** 100% drop rate specific items. Already handled by normal drop system (rate: 100).

**Old Blue Box / Old Purple Box / Old Card Album:** Random item generation on use. Part of consumable effects system, not ground drops. These items may need special handling if the generated item would exceed weight/inventory — overflow goes to ground.

---

## 2. Current System Audit

### 2.1 What Already Exists

| System | Status | Details |
|--------|--------|---------|
| `rollEnemyDrops()` | Complete | Per-slot independent rolls, minQty/maxQty, MVP drops |
| `getDropTierColor()` | Complete | 6-tier color system (red/purple/blue/green/yellow/pink) |
| `addItemToInventory()` | Complete | Stacking, slot assignment, identified flag |
| `removeItemFromInventory()` | Complete | Partial quantity removal |
| Weight checking on drop | Complete | Prevents overweight pickup in death handler |
| Unidentified equipment drops | Complete | Weapons/armor drop as unidentified |
| MVP determination | Complete | Highest totalDamage in `enemy.inCombatWith` |
| MVP drops (3 rolls) | Complete | Goes directly to MVP winner's inventory |
| EXP Tap Bonus | Complete | +25% per additional attacker |
| Party EXP share | Complete | even_share and each_take modes |
| Card kill hooks | Complete | HP/SP/Zeny gain on kill |
| Card drop bonuses | Complete | `processCardDropBonuses()` — extra drops from cards |
| Steal skill | Complete | Direct steal from drop table, stealProtected check |
| Ore Discovery | Complete | Passive ore drop on kill (Blacksmith) |
| `inventory:dropped` handler | Partial | Removes from inventory but **does not create ground item** |
| `loot:drop` event | Partial | Sends drop notification to killer — items go straight to inventory |
| Party item_share DB column | Exists | `parties.item_share` (0/1) — field exists but **unused in loot logic** |
| Party item_distribute DB column | Exists | `parties.item_distribute` (0/1) — field exists but **unused** |
| `enemy.inCombatWith` | Complete | Map<charId, {totalDamage, name}> — damage tracking for ownership |
| `enemy.mode.looter` | Parsed | Looter flag parsed from hex mode — **behavior not implemented** |
| Drop tier audio (client) | Complete | `drop_<color>.wav` files in Audio/SFX/Drops/ |
| Loot notification overlay (client) | Complete | `SLootNotificationOverlay` in InventorySubsystem |

### 2.2 What's Missing (Must Build)

| System | Priority | Notes |
|--------|----------|-------|
| **Ground item registry (server)** | P0 | In-memory Map per zone, with timers |
| **Ground item creation on monster death** | P0 | Replace direct-to-inventory with ground drop |
| **Drop scatter positions** | P0 | SE→W→N cycle from death position |
| **Ownership priority system** | P0 | Phased timer (3s/2s/2s normal, 10s/10s/2s MVP) |
| **`item:pickup` socket handler** | P0 | Click → validate ownership → add to inventory → broadcast |
| **`item:spawned` broadcast** | P0 | Zone-wide event with position, item data, owner |
| **`item:picked_up` broadcast** | P0 | Zone-wide removal event |
| **`item:despawn` broadcast** | P0 | Timer-driven cleanup + zone broadcast |
| **Initial ground item list on zone:ready** | P0 | Send existing ground items to joining player |
| **Player drop-to-ground** | P1 | Modify `inventory:dropped` to create ground item |
| **GroundItemSubsystem (UE5 C++)** | P0 | Manages ground item actors, pickup, despawn |
| **Ground item actor (UE5)** | P0 | Billboard sprite + name label + click target |
| **Drop animation (client)** | P1 | Pop-out arc from death position to scatter position |
| **Pickup walk-to-item** | P1 | Auto-pathfind to item on click, pickup when in range |
| **Party item share modes** | P1 | Wire up item_share / item_distribute columns |
| **Bubble Gum drop rate buff** | P2 | sc_start handler + rollEnemyDrops modifier |
| **Greed skill** | P2 | AoE ground item pickup (Blacksmith 1210) |
| **Looter monster behavior** | P2 | MD_LOOTER monsters pick up ground items |
| **Drop rate cap (90%)** | P2 | Max boosted rate enforcement |
| **NoDrop item flag** | P2 | Prevent certain items from being dropped by players |
| **Quantity selector on player drop** | P2 | Partial stack dropping UI |
| **Pickup animation (client)** | P3 | Brief gesture when picking up |
| **Pickup chat message** | P1 | "Obtained {qty}x {ItemName}" |

---

## 3. Implementation Plan

### Phase 1: Server-Side Ground Item Registry (P0)

**Goal:** Items drop to the ground instead of directly to inventory. Server tracks all ground items per zone with ownership and timers.

#### 1A. Ground Item Data Structure

```javascript
// In-memory ground item registry (no database — transient)
const groundItems = new Map(); // Map<groundItemId, GroundItem>
let nextGroundItemId = 1;

// Per-zone index for fast lookups
const zoneGroundItems = new Map(); // Map<zoneName, Set<groundItemId>>

const GROUND_ITEM_LIFETIME = 60000;    // 60 seconds
const OWNERSHIP_PHASE_1 = 3000;        // 3s — #1 damage dealer only
const OWNERSHIP_PHASE_2 = 5000;        // 5s cumulative — top 2
const OWNERSHIP_PHASE_3 = 7000;        // 7s cumulative — top 3
const MVP_OWNERSHIP_PHASE_1 = 10000;   // 10s — MVP winner only
const MVP_OWNERSHIP_PHASE_2 = 20000;   // 20s cumulative — top 2
const MVP_OWNERSHIP_PHASE_3 = 22000;   // 22s cumulative — top 3
const PICKUP_RANGE = 100;              // UE units (~2 cells)
const DROP_SCATTER_DISTANCE = 50;      // UE units (~1 cell)

// Scatter direction cycle: SE, W, N (repeating)
const SCATTER_OFFSETS = [
    { x: 35, y: 35 },   // SE
    { x: -50, y: 0 },   // W
    { x: 0, y: -50 },   // N
];
```

#### 1B. GroundItem Object

```javascript
/*
{
    groundItemId: number,           // unique auto-increment ID
    itemId: number,                 // items table ID
    itemName: string,               // display name
    quantity: number,               // stack count (usually 1)
    zone: string,                   // zone name
    x: number, y: number, z: number, // world position
    dropTime: number,               // Date.now() when dropped
    despawnTime: number,            // dropTime + GROUND_ITEM_LIFETIME
    tierColor: string,              // 'red'|'purple'|'blue'|'green'|'yellow'|'pink'
    icon: string,                   // item icon name
    itemType: string,               // 'weapon'|'armor'|'card'|'healing'|'usable'|'ammo'|'etc'
    identified: boolean,            // equipment drops as unidentified
    refineLevel: number,            // 0 for non-equipment
    compoundedCards: number[],      // [0,0,0,0] for non-carded
    forgedBy: string|null,          // forger name
    forgedElement: number,          // 0 for non-forged
    forgedStarCrumbs: number,       // 0 for non-forged
    // Ownership
    ownerCharId: number|null,       // primary owner (most damage)
    damageRanking: number[],        // [charId1, charId2, charId3] sorted by damage
    isMvpDrop: boolean,             // extended ownership timers
    partyId: number|null,           // party of the killer (for Party Share mode)
    // Source
    sourceType: 'monster'|'player', // who created this ground item
    sourceId: string|null,          // enemyId or charId of source
}
*/
```

#### 1C. Monster Death → Ground Drops (replace direct-to-inventory)

Modify `processEnemyDeathFromSkill()` and auto-attack death handler:

1. Roll drops with `rollEnemyDrops()` as before
2. Add card extra drops from `processCardDropBonuses()` as before
3. **Instead of `addItemToInventory()`**, call `createGroundItem()` for each drop
4. MVP drops still go directly to MVP winner's inventory (RO Classic behavior)
   - If MVP winner's inventory is full/overweight, overflow to ground
5. Compute scatter positions using the SE→W→N cycle
6. Broadcast `item:spawned` to zone for each ground item
7. Store damage ranking from `enemy.inCombatWith` snapshot

**Scatter position algorithm:**
```javascript
function calculateScatterPositions(deathX, deathY, deathZ, count, zone) {
    const positions = [];
    for (let i = 0; i < count; i++) {
        if (i === 0) {
            positions.push({ x: deathX, y: deathY, z: deathZ });
        } else {
            const offset = SCATTER_OFFSETS[(i - 1) % SCATTER_OFFSETS.length];
            let sx = deathX + offset.x;
            let sy = deathY + offset.y;
            // TODO: NavMesh walkability check — if blocked, search 3x3 around
            positions.push({ x: sx, y: sy, z: deathZ });
        }
    }
    return positions;
}
```

#### 1D. Ownership Validation

```javascript
function canPickupGroundItem(groundItem, characterId, player) {
    const elapsed = Date.now() - groundItem.dropTime;
    const isMvp = groundItem.isMvpDrop;
    const ranking = groundItem.damageRanking || [];
    
    // Party Share mode bypass — any party member can loot immediately
    if (groundItem.partyId && player.partyId === groundItem.partyId) {
        const party = activeParties.get(player.partyId);
        if (party && party.item_share === 1) { // party_share mode
            return true;
        }
    }
    
    // Phase timers
    const p1 = isMvp ? MVP_OWNERSHIP_PHASE_1 : OWNERSHIP_PHASE_1;
    const p2 = isMvp ? MVP_OWNERSHIP_PHASE_2 : OWNERSHIP_PHASE_2;
    const p3 = isMvp ? MVP_OWNERSHIP_PHASE_3 : OWNERSHIP_PHASE_3;
    
    if (elapsed < p1) {
        return ranking[0] === characterId;
    } else if (elapsed < p2) {
        return ranking.slice(0, 2).includes(characterId);
    } else if (elapsed < p3) {
        return ranking.slice(0, 3).includes(characterId);
    }
    // Free-for-all
    return true;
}
```

#### 1E. Despawn Timer Loop

```javascript
// Run every 5 seconds (check for expired ground items)
setInterval(() => {
    const now = Date.now();
    for (const [gid, item] of groundItems.entries()) {
        if (now >= item.despawnTime) {
            // Remove from registry
            groundItems.delete(gid);
            const zoneSet = zoneGroundItems.get(item.zone);
            if (zoneSet) zoneSet.delete(gid);
            // Broadcast despawn to zone
            broadcastToZone(item.zone, 'item:despawned', { groundItemId: gid });
        }
    }
}, 5000);
```

#### 1F. Zone Cleanup

When a zone has zero players, ground items continue their timers normally and despawn. No special cleanup needed — the 60s timer handles it. When players leave a zone, items left behind simply expire.

### Phase 2: Socket Event Handlers (P0)

#### 2A. `item:pickup` (client → server)

```javascript
socket.on('item:pickup', async (data) => {
    const { groundItemId } = data;
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    
    // Validate ground item exists
    const gItem = groundItems.get(groundItemId);
    if (!gItem) {
        socket.emit('item:pickup_error', { message: 'Item no longer exists' });
        return;
    }
    
    // Zone check
    if (gItem.zone !== player.zone) {
        socket.emit('item:pickup_error', { message: 'Item not in your zone' });
        return;
    }
    
    // Range check
    const dx = player.x - gItem.x;
    const dy = player.y - gItem.y;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > PICKUP_RANGE) {
        socket.emit('item:pickup_error', { message: 'Too far away' });
        return;
    }
    
    // State checks
    if (player.isDead) { socket.emit('item:pickup_error', { message: 'Cannot pick up while dead' }); return; }
    if (player.isSitting) { socket.emit('item:pickup_error', { message: 'Stand up first' }); return; }
    if (player.isVending) { socket.emit('item:pickup_error', { message: 'Cannot pick up while vending' }); return; }
    if (player._activeTradeId) { socket.emit('item:pickup_error', { message: 'Cannot pick up while trading' }); return; }
    
    // Overweight check (90% threshold blocks pickup)
    const maxWeight = getPlayerMaxWeight(player);
    const weightPct = ((player.currentWeight || 0) / maxWeight) * 100;
    if (weightPct >= 90) {
        socket.emit('item:pickup_error', { message: 'You are overweight' });
        return;
    }
    
    // Weight check for this specific item
    const itemDef = itemDefinitions.get(gItem.itemId);
    const itemWeight = itemDef ? (itemDef.weight || 0) * gItem.quantity : 0;
    if ((player.currentWeight || 0) + itemWeight > maxWeight) {
        socket.emit('item:pickup_error', { message: 'Item is too heavy' });
        return;
    }
    
    // Inventory slot check (max 100 slots)
    // TODO: Check if adding this item would exceed 100 slots (non-stackable)
    
    // Ownership check
    if (!canPickupGroundItem(gItem, characterId, player)) {
        socket.emit('item:pickup_error', { message: 'You cannot pick this up yet' });
        return;
    }
    
    // === ATOMIC PICKUP ===
    // Remove from ground FIRST to prevent race conditions
    groundItems.delete(groundItemId);
    const zoneSet = zoneGroundItems.get(gItem.zone);
    if (zoneSet) zoneSet.delete(groundItemId);
    
    // Determine recipient (party item distribution)
    let recipientCharId = characterId;
    let recipientPlayer = player;
    let recipientSocket = socket;
    
    if (player.partyId) {
        const party = activeParties.get(player.partyId);
        if (party && party.item_distribute === 1) { // shared mode
            // Random party member in zone within level range
            const eligible = [];
            for (const [memId, mem] of party.members.entries()) {
                const memPlayer = connectedPlayers.get(memId);
                if (memPlayer && memPlayer.zone === player.zone && isWithinShareRange(party)) {
                    eligible.push({ charId: memId, player: memPlayer });
                }
            }
            if (eligible.length > 0) {
                const pick = eligible[Math.floor(Math.random() * eligible.length)];
                recipientCharId = pick.charId;
                recipientPlayer = pick.player;
                recipientSocket = io.sockets.sockets.get(pick.player.socketId) || socket;
            }
        }
    }
    
    // Add to recipient's inventory
    const added = await addItemToInventory(recipientCharId, gItem.itemId, gItem.quantity, null, gItem.identified);
    if (!added) {
        // Failed to add — put item back on ground
        groundItems.set(groundItemId, gItem);
        if (zoneSet) zoneSet.add(groundItemId);
        socket.emit('item:pickup_error', { message: 'Inventory full' });
        return;
    }
    
    // Update weight cache
    await updatePlayerWeightCache(recipientCharId, recipientPlayer);
    
    // Broadcast pickup to zone (removes item for all clients)
    broadcastToZone(gItem.zone, 'item:picked_up', {
        groundItemId: groundItemId,
        pickedUpBy: recipientCharId,
        pickedUpByName: recipientPlayer.characterName
    });
    
    // Send pickup notification to recipient
    recipientSocket.emit('item:pickup_success', {
        groundItemId: groundItemId,
        itemId: gItem.itemId,
        itemName: gItem.itemName,
        quantity: gItem.quantity,
        icon: gItem.icon,
        tierColor: gItem.tierColor
    });
    
    // Chat message: "Obtained 1x Jellopy"
    recipientSocket.emit('chat:receive', {
        type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
        message: `Obtained ${gItem.quantity}x ${gItem.itemName}`,
        timestamp: Date.now()
    });
    
    // Refresh recipient inventory
    const updatedInventory = await getPlayerInventory(recipientCharId);
    recipientSocket.emit('inventory:data', {
        items: updatedInventory, zuzucoin: recipientPlayer.zuzucoin,
        currentWeight: recipientPlayer.currentWeight || 0,
        maxWeight: getPlayerMaxWeight(recipientPlayer)
    });
});
```

#### 2B. `item:drop_player` (client → server)

```javascript
socket.on('item:drop_player', async (data) => {
    const { inventoryId, quantity } = data;
    const playerInfo = findPlayerBySocketId(socket.id);
    if (!playerInfo) return;
    const { characterId, player } = playerInfo;
    
    // State checks
    if (player.isDead || player.isSitting || player.isVending || player._activeTradeId) {
        socket.emit('item:drop_error', { message: 'Cannot drop items in current state' });
        return;
    }
    
    // Fetch item from inventory
    const result = await pool.query(
        'SELECT ci.*, i.name, i.weight, i.item_type, i.icon, i.stackable FROM character_inventory ci JOIN items i ON ci.item_id = i.id WHERE ci.inventory_id = $1 AND ci.character_id = $2',
        [inventoryId, characterId]
    );
    if (result.rows.length === 0) {
        socket.emit('item:drop_error', { message: 'Item not found' });
        return;
    }
    
    const item = result.rows[0];
    
    // Cannot drop equipped items
    if (item.is_equipped) {
        socket.emit('item:drop_error', { message: 'Unequip item first' });
        return;
    }
    
    // TODO: NoDrop flag check
    // if (itemDef.nodrop) { socket.emit('item:drop_error', { message: 'This item cannot be dropped' }); return; }
    
    // Validate quantity
    const dropQty = Math.min(quantity || item.quantity, item.quantity);
    if (dropQty <= 0) return;
    
    // Remove from inventory
    await removeItemFromInventory(inventoryId, dropQty);
    
    // Create ground item at player's feet
    const gItem = createGroundItem({
        itemId: item.item_id,
        itemName: item.name,
        quantity: dropQty,
        zone: player.zone,
        x: player.x, y: player.y, z: player.z || 300,
        tierColor: getDropTierColor(itemDefinitions.get(item.item_id), false),
        icon: item.icon,
        itemType: item.item_type,
        identified: item.identified !== false,
        refineLevel: item.refine_level || 0,
        compoundedCards: item.compounded_cards || [0,0,0,0],
        forgedBy: item.forged_by || null,
        forgedElement: item.forged_element || 0,
        forgedStarCrumbs: item.forged_star_crumbs || 0,
        ownerCharId: null,       // No ownership on player drops
        damageRanking: [],
        isMvpDrop: false,
        partyId: null,
        sourceType: 'player',
        sourceId: characterId
    });
    
    // Broadcast to zone
    broadcastToZone(player.zone, 'item:spawned', buildGroundItemPayload(gItem));
    
    // Refresh inventory
    const inventory = await getPlayerInventory(characterId);
    socket.emit('inventory:data', {
        items: inventory, zuzucoin: player.zuzucoin,
        currentWeight: player.currentWeight || 0,
        maxWeight: getPlayerMaxWeight(player)
    });
    await updatePlayerWeightCache(characterId, player);
});
```

#### 2C. Ground Items on Zone Entry (`zone:ready` handler)

In the existing `zone:ready` handler, add:

```javascript
// Send all current ground items in this zone to the joining player
const zoneSet = zoneGroundItems.get(zone);
if (zoneSet && zoneSet.size > 0) {
    const items = [];
    for (const gid of zoneSet) {
        const gItem = groundItems.get(gid);
        if (gItem) items.push(buildGroundItemPayload(gItem));
    }
    if (items.length > 0) {
        socket.emit('item:ground_list', { items });
    }
}
```

### Phase 3: Client-Side Ground Item System (P0)

#### 3A. GroundItemSubsystem (UWorldSubsystem)

New file: `client/SabriMMO/Source/SabriMMO/UI/GroundItemSubsystem.h/.cpp`

**Responsibilities:**
- Register socket event handlers: `item:spawned`, `item:picked_up`, `item:despawned`, `item:ground_list`, `item:pickup_success`, `item:pickup_error`
- Manage TMap<int32, AGroundItemActor*> registry
- Handle click-to-pickup via PlayerInputSubsystem integration
- Send `item:pickup` socket event when player clicks a ground item
- Display pickup messages in chat

**Key data struct:**
```cpp
struct FGroundItemData
{
    int32 GroundItemId = 0;
    int32 ItemId = 0;
    FString ItemName;
    int32 Quantity = 1;
    FString Icon;
    FString TierColor;
    FString ItemType;
    FVector Position;
    float DropTime = 0.f;
    float DespawnTime = 0.f;
    int32 OwnerCharId = 0;
    bool bIdentified = true;
};
```

#### 3B. AGroundItemActor

New file: `client/SabriMMO/Source/SabriMMO/Actors/GroundItemActor.h/.cpp`

**Components:**
- `UBillboardComponent` or `UStaticMeshComponent` with item icon texture mapped to a small quad
- Text render component for item name label (always visible, positioned above the icon)
- No collision (walkthrough)
- Click detection via `OnClicked` or trace-based targeting from PlayerInputSubsystem

**Visual:**
- Small billboard quad (~32x32 to 48x48 world units) at ground level
- Item icon texture loaded from the item's icon asset
- Name label in white text, small font, positioned above icon
- Tier color glow/outline (optional P2 enhancement):
  - Purple glow for cards
  - Blue glow for equipment
  - Red glow for MVP drops

#### 3C. Drop Animation (Client-Side)

When `item:spawned` is received:
1. Spawn AGroundItemActor at the monster's death position (or `sourcePosition`)
2. Animate it along a parabolic arc to the final scatter position over ~0.3-0.5 seconds
3. Play tier-appropriate drop sound on landing
4. After animation completes, actor settles at final position

#### 3D. Pickup Flow

1. Player clicks on AGroundItemActor → PlayerInputSubsystem detects click target is ground item
2. If player is within PICKUP_RANGE → emit `item:pickup` immediately
3. If player is out of range → auto-walk to item position, emit `item:pickup` when within range
4. Server responds with `item:pickup_success` or `item:pickup_error`
5. On `item:picked_up` broadcast → destroy the AGroundItemActor from the registry

### Phase 4: Party Loot Integration (P1)

#### 4A. Wire Party Item Settings

The party table already has `item_share` and `item_distribute` columns. Wire them:

1. Add `party:update_item_share` socket handler (like `party:update_exp_share`)
2. UI: Add item share toggle to party settings (Each Take / Party Share)
3. UI: Add item distribute toggle (Individual / Shared)
4. Apply `item_share` in `canPickupGroundItem()` — Party Share bypasses ownership
5. Apply `item_distribute` in `item:pickup` handler — Shared mode randomly assigns to party member

#### 4B. Party Loot in Monster Death Handler

When creating ground items from monster death:
- Store `partyId` of the killer on the ground item
- If killer has no party, `partyId = null` (standard ownership rules)
- If killer has party with "Party Share", any party member can pick up immediately

### Phase 5: Drop Rate Modifiers (P2)

#### 5A. Bubble Gum Consumable

Add to `ro_item_effects.js`:
```javascript
12210: { sc_start: { type: 'bubble_gum', duration: 1800000 } } // 30 min
```

In `rollEnemyDrops()`, check for `bubble_gum` buff:
```javascript
function rollEnemyDrops(enemy, killerPlayer) {
    // ...existing code...
    let dropRateBonus = 0;
    if (killerPlayer) {
        const mods = getCombinedModifiers(killerPlayer);
        if (hasActiveBuff(killerPlayer, 'bubble_gum')) dropRateBonus += 1.0; // +100%
        if (hasActiveBuff(killerPlayer, 'he_bubble_gum')) dropRateBonus += 2.0; // +200%
        // Equipment bonuses could add more
    }
    
    for (const drop of template.drops) {
        let effectiveRate = drop.chance;
        if (dropRateBonus > 0) {
            effectiveRate = Math.min(effectiveRate * (1.0 + dropRateBonus), 90); // 90% cap
        }
        if (Math.random() * 100 < effectiveRate) { // ...
        }
    }
}
```

#### 5B. Drop Rate Cap Enforcement

Cap all boosted rates at 90% (matching rAthena `drop_rate_cap: 9000`).

### Phase 6: Advanced Features (P2-P3)

#### 6A. Greed Skill (Blacksmith 1210)

```javascript
// In skill handler for Greed (1210)
const GREED_RANGE = 200; // 5x5 cells ≈ 200 UE units radius
const greedZone = player.zone;
const zoneSet = zoneGroundItems.get(greedZone);
if (!zoneSet || zoneSet.size === 0) {
    socket.emit('skill:error', { message: 'No items nearby' });
    return;
}

const itemsToPickup = [];
for (const gid of zoneSet) {
    const gItem = groundItems.get(gid);
    if (!gItem) continue;
    const dx = player.x - gItem.x;
    const dy = player.y - gItem.y;
    if (Math.sqrt(dx*dx + dy*dy) <= GREED_RANGE) {
        // Ownership check still applies
        if (canPickupGroundItem(gItem, characterId, player)) {
            itemsToPickup.push(gItem);
        }
    }
}

// Pick up all valid items (weight/inventory permitting)
for (const gItem of itemsToPickup) {
    // Weight check, inventory add, broadcast... (same as item:pickup)
}
```

#### 6B. Looter Monster Behavior

In the AI idle state handler for monsters with `mode.looter`:
```javascript
if (enemy.mode.looter && enemy.aiState === AI_STATE.IDLE) {
    // Check for ground items within 2 cells
    const zoneSet = zoneGroundItems.get(enemy.zone);
    if (zoneSet) {
        for (const gid of zoneSet) {
            const gItem = groundItems.get(gid);
            if (!gItem) continue;
            // No ownership restriction for monsters
            const dx = enemy.x - gItem.x;
            const dy = enemy.y - gItem.y;
            if (Math.sqrt(dx*dx + dy*dy) <= 100) {
                // Monster picks up item — store in enemy._lootedItems
                if (!enemy._lootedItems) enemy._lootedItems = [];
                enemy._lootedItems.push(gItem);
                // Remove from ground
                groundItems.delete(gid);
                zoneSet.delete(gid);
                broadcastToZone(enemy.zone, 'item:picked_up', {
                    groundItemId: gid, pickedUpBy: 0, pickedUpByName: enemy.name
                });
                break; // One item per tick
            }
        }
    }
}
```

On monster death, if `enemy._lootedItems` has items, drop them to ground along with normal drops.

#### 6C. NoDrop Item Flag

Add `nodrop` check using item trade restriction flags from the items table. Items with trade restrictions that include "NoDrop" cannot be dropped by players.

#### 6D. Pickup Walk-To-Item (Client)

When player clicks a ground item that's out of range:
1. Set a pending pickup target in PlayerInputSubsystem
2. Issue a move command toward the item position
3. Each tick, check if player is now within PICKUP_RANGE
4. When in range, emit `item:pickup` and clear the pending target

---

## 4. Socket Event Specification

### Server → Client Events

| Event | Payload | When |
|-------|---------|------|
| `item:spawned` | `{ groundItemId, itemId, itemName, quantity, icon, tierColor, itemType, x, y, z, identified, ownerCharId, isMvpDrop, sourceX, sourceY, sourceZ, sourceType }` | New item appears on ground |
| `item:picked_up` | `{ groundItemId, pickedUpBy, pickedUpByName }` | Item removed from ground (someone picked it up) |
| `item:despawned` | `{ groundItemId }` | Item expired (60s timer) |
| `item:ground_list` | `{ items: [spawned_payload, ...] }` | On zone:ready — all current ground items |
| `item:pickup_success` | `{ groundItemId, itemId, itemName, quantity, icon, tierColor }` | Confirm you picked up an item |
| `item:pickup_error` | `{ message }` | Pickup failed (reason in message) |
| `item:drop_error` | `{ message }` | Player drop failed |

### Client → Server Events

| Event | Payload | When |
|-------|---------|------|
| `item:pickup` | `{ groundItemId }` | Player requests to pick up a ground item |
| `item:drop_player` | `{ inventoryId, quantity }` | Player drops item from inventory to ground |

---

## 5. Server Data Structures

### Ground Item Registry (In-Memory Only)

```javascript
// No database table needed — ground items are transient (60s lifetime max)
const groundItems = new Map();        // Map<groundItemId, GroundItem>
const zoneGroundItems = new Map();    // Map<zoneName, Set<groundItemId>>
let nextGroundItemId = 1;
```

**Why no database?** Ground items are ephemeral (max 60s). Server restart clears all ground items — this is expected and matches RO Classic behavior. No persistence needed.

### Helper Functions

```javascript
function createGroundItem(data) {
    const gid = nextGroundItemId++;
    const gItem = {
        groundItemId: gid,
        ...data,
        dropTime: Date.now(),
        despawnTime: Date.now() + GROUND_ITEM_LIFETIME
    };
    groundItems.set(gid, gItem);
    if (!zoneGroundItems.has(data.zone)) zoneGroundItems.set(data.zone, new Set());
    zoneGroundItems.get(data.zone).add(gid);
    return gItem;
}

function buildGroundItemPayload(gItem) {
    return {
        groundItemId: gItem.groundItemId,
        itemId: gItem.itemId,
        itemName: gItem.itemName,
        quantity: gItem.quantity,
        icon: gItem.icon,
        tierColor: gItem.tierColor,
        itemType: gItem.itemType,
        x: gItem.x, y: gItem.y, z: gItem.z,
        identified: gItem.identified,
        ownerCharId: gItem.ownerCharId,
        isMvpDrop: gItem.isMvpDrop,
        sourceX: gItem.sourceX || gItem.x,
        sourceY: gItem.sourceY || gItem.y,
        sourceZ: gItem.sourceZ || gItem.z,
        sourceType: gItem.sourceType
    };
}
```

---

## 6. Client Architecture (UE5 C++)

### New Files

| File | Type | Purpose |
|------|------|---------|
| `UI/GroundItemSubsystem.h/.cpp` | UWorldSubsystem | Socket events, ground item registry, pickup logic |
| `Actors/GroundItemActor.h/.cpp` | AActor | Billboard icon + name label + click target |

### GroundItemSubsystem Integration Points

| System | Integration |
|--------|-------------|
| **SocketEventRouter** | Register handlers for `item:spawned`, `item:picked_up`, `item:despawned`, `item:ground_list`, `item:pickup_success`, `item:pickup_error` |
| **PlayerInputSubsystem** | Add `ETargetType::GroundItem` — on click, either pickup (in range) or walk-to + pickup (out of range) |
| **InventorySubsystem** | Modify drag-outside-window to emit `item:drop_player` instead of `inventory:dropped` |
| **ChatSubsystem** | Display "Obtained X" messages from `item:pickup_success` |
| **CameraSubsystem** | Ground item actors use no-depth billboard (same as enemy/player sprites) |
| **AudioSubsystem** | Play drop sounds on `item:spawned`, pickup sound on `item:pickup_success` |
| **EnemySubsystem** | On `enemy:death`, remember death position for drop animation source |

### Widget: None Required

Ground items are 3D world actors (AGroundItemActor), not Slate widgets. The name label uses a `UWidgetComponent` or `UTextRenderComponent` in world space.

---

## 7. Edge Cases & Exploit Prevention

### 7.1 Race Conditions

| Scenario | Prevention |
|----------|-----------|
| Two players click same item simultaneously | Server removes from Map FIRST, then processes. Second request gets "Item no longer exists" |
| Player disconnects mid-pickup | `groundItems.delete()` is synchronous — either it was removed (pickup succeeds) or it wasn't (item stays) |
| Item despawns while player walking to it | Client receives `item:despawned` → removes actor → player arrives at empty spot |

### 7.2 Duplication Prevention

| Vector | Prevention |
|--------|-----------|
| Double-pickup spam | Remove from groundItems Map before async inventory add. If add fails, re-insert to Map |
| Modified client sending fake groundItemIds | Server validates ID exists in `groundItems` Map |
| Pickup from wrong zone | Server checks `gItem.zone === player.zone` |
| Pickup while in another state | State checks (dead, sitting, vending, trading) before processing |

### 7.3 Weight/Inventory Exploits

| Vector | Prevention |
|--------|-----------|
| Pickup when overweight | Check `currentWeight + itemWeight > maxWeight` server-side |
| Pickup at 90%+ weight | Block pickup at 90% weight threshold |
| Inventory overflow (100+ slots) | Check slot count before adding non-stackable items |

### 7.4 Ownership Bypasses

| Vector | Prevention |
|--------|-----------|
| Claiming to be damage dealer | Server tracks `damageRanking` from `enemy.inCombatWith` snapshot at death time |
| Party mode abuse | Server validates party membership and party settings from `activeParties` |
| Alt-account damage padding | Not preventable without IP tracking — same as RO Classic |

### 7.5 Server Performance

| Concern | Mitigation |
|---------|-----------|
| Too many ground items | 60s despawn timer prevents unlimited accumulation |
| Despawn check loop | 5-second interval, iterates only active ground items |
| Zone broadcast spam | Batch `item:spawned` events per monster death (send array, not individual) |
| Memory usage | Each ground item is ~500 bytes. 10,000 items = ~5MB — negligible |

### 7.6 Edge Cases

| Case | Behavior |
|------|----------|
| Monster dies with 0 players in zone | Items drop, despawn after 60s with no one to pick up |
| Player drops item on zone boundary | Item drops at player position, exists in that zone only |
| Slave monster killed by master death | No drops (already handled — slave death gives no EXP/drops) |
| Homunculus kills monster | Drops belong to homunculus owner |
| Monster killed by trap | Drops belong to trap owner |
| Item from Old Blue Box exceeds weight | Generated item drops to ground |
| Server restart | All ground items lost — expected behavior, matches RO Classic |

---

## 8. Progress Tracking Checklist

### Phase 1: Server Ground Item Registry
- [x] Define constants (GROUND_ITEM_LIFETIME, OWNERSHIP timers, PICKUP_RANGE, SCATTER_DISTANCE)
- [x] Create `groundItems` Map and `zoneGroundItems` Map
- [x] Implement `createGroundItem()` helper
- [x] Implement `buildGroundItemPayload()` helper
- [x] Implement `canPickupGroundItem()` ownership validator
- [x] Implement `calculateScatterPositions()` (SE→W→N cycle)
- [x] Implement ground item despawn timer (5s interval)
- [x] Modify `processEnemyDeathFromSkill()` — normal drops go to ground instead of inventory
- [x] Modify auto-attack death handler — normal drops go to ground instead of inventory
- [x] MVP drops still go direct to inventory, overflow to ground if full/overweight
- [x] Preserve card extra drops (`processCardDropBonuses`) — also go to ground
- [x] Preserve Ore Discovery drops — go to ground
- [x] Store damage ranking snapshot from `enemy.inCombatWith` on each ground item
- [x] Store killer's partyId on each ground item

### Phase 2: Socket Event Handlers
- [x] Implement `item:pickup` handler (validation, ownership, weight, atomic pickup)
- [x] Implement `item:drop_player` handler (state checks, create ground item, broadcast)
- [x] Broadcast `item:spawned` to zone on ground item creation
- [x] Broadcast `item:picked_up` to zone on successful pickup
- [x] Broadcast `item:despawned` to zone on timer expiry
- [x] Send `item:ground_list` in `zone:ready` handler for existing ground items
- [x] Send `item:pickup_success` to picker with chat message
- [x] Send `item:pickup_error` with appropriate message
- [x] Send `item:drop_error` for player drop failures
- [x] Batch `item:spawned` for multi-drop monster deaths (optional optimization)

### Phase 3: Client GroundItemSubsystem
- [x] Create `GroundItemSubsystem.h/.cpp` (UWorldSubsystem)
- [x] Register socket event handlers via SocketEventRouter
- [x] Create FGroundItemEntry struct (renamed from FGroundItemData)
- [x] Create TMap<int32, FGroundItemEntry> registry + TMap<AActor*, int32> reverse lookup
- [x] Handle `item:spawned_batch` — spawn AGroundItemActor at position (batched)
- [x] Handle `item:picked_up` — destroy AGroundItemActor
- [x] Handle `item:despawned_batch` — destroy AGroundItemActor with fade out (batched)
- [x] Handle `item:ground_list` — clear + spawn all existing ground items on zone entry
- [x] Handle `item:pickup_success` — play pickup sound, trigger Pickup animation
- [x] Handle `item:pickup_error` — route error message to ChatSubsystem combat log
- [x] Gate behind `IsSocketConnected()` (no ground items on login screen)

### Phase 4: Ground Item Actor
- [x] Create `GroundItemActor.h/.cpp` (AActor)
- [x] Screen-space UWidgetComponent with item icon (SImage + FSlateBrush)
- [x] Load item icon texture with sanitization, legacy map, subfolder search, slotted fallback
- [x] Name label (screen-space UWidgetComponent) — tier-colored text, hover-only (TargetingSubsystem toggles)
- [x] Tier color mapping (red/purple/blue/green/yellow/pink) on name label
- [x] No collision (walkthrough) — ClickBox uses ECC_Visibility QueryOnly for click detection
- [x] Ground Z-snap via line trace (ignores APawn + AGroundItemActor, ECC_WorldStatic)
- [x] Cleanup: Deinitialize destroys all actors, FadeOutAndDestroy for despawn

### Phase 5: Click-to-Pickup Integration
- [x] Add `ETargetType::GroundItem` to TargetingSubsystem::ClassifyActor (Priority 3)
- [x] Click detection: line trace hits AGroundItemActor UBoxComponent (ECC_Visibility)
- [x] In-range check (PickupRange = 150 UE units client-side, PICKUP_RANGE = 100 server-side)
- [x] If in range -> RequestPickup immediately
- [x] If out of range -> PendingPickupActor + PendingPickupGroundItemId, MoveToLocation
- [x] On arrival within range -> RequestPickup, clear pending target
- [x] Cancel pending pickup on: new click target, manual movement, ground click
- [x] GrabHand cursor when hovering over ground item (TargetingSubsystem)

### Phase 6: Drop Animation (Client)
- [x] On `item:spawned_batch` with `sourceType: 'monster'`, PlayDropArc from sourcePos to finalPos
- [x] Parabolic arc with ArcHeight=80, ArcDuration=0.4s, ease-out curve
- [x] Play tier-appropriate drop sound on landing (gated by OptionsSubsystem per-tier toggle)
- [x] For `sourceType: 'player'`, short drop arc from +40Z to same position (0.25s)
- [x] Despawn fade-out: FadeOutAndDestroy scales to 0 over 0.5s then Destroy()

### Phase 7: Inventory Drop Integration
- [x] SInventoryWidget detects drag-outside-window in Tick (mouse position check)
- [x] ShowDropPopup: PushMenu at cursor with quantity selector (stackable) or confirm (non-stackable)
- [x] Server validates: cannot drop equipped items, state checks (dead/sitting/vending/trading)
- [x] ConfirmDrop sends `inventory:drop` with inventoryId, quantity, px/py/pz
- [x] Server `inventory:drop` handler creates ground item at player feet

### Phase 8: Party Loot System
- [x] `party:change_item_share` socket handler (toggle Each Take / Party Share)
- [x] `party:change_item_distribute` socket handler (toggle Individual / Shared)
- [x] `canPickupGroundItem()` checks `party.item_share === 1` — Party Share bypasses ownership
- [x] `item:pickup` handler checks `party.item_distribute === 1` — random party member in zone
- [x] SPartyWidget: Item Pickup and Item Distribution toggle sections
- [x] PartySubsystem: ChangeItemShare, ChangeItemDistribute methods

### Phase 9: Drop Rate Modifiers
- [x] Bubble Gum buff integration in rollEnemyDrops
- [x] HE Bubble Gum buff integration in rollEnemyDrops
- [x] rollEnemyDrops accepts killer player and checks for drop rate buffs
- [x] Multiplicative bonus: `effectiveRate = baseRate * (1.0 + bonus)`
- [x] 90% cap enforced (DROP_RATE_CAP = 0.90)
- [x] Killer player passed to rollEnemyDrops in both death handlers

### Phase 10: Advanced Features
- [x] **Greed skill (1210)**: AoE ground item pickup within 5x5 cells (GREED_RANGE = 200)
- [x] **Greed**: Respects ownership rules, weight limits, SP cost (10)
- [x] **Greed**: Cannot use in towns, PvP, or WoE maps
- [x] **Looter monsters**: AI idle state picks up nearby ground items
- [x] **Looter monsters**: Store looted items in `enemy._lootedItems`
- [x] **Looter monsters**: Drop looted items on death alongside normal drops
- [x] **NoDrop flag**: Check item trade restrictions before allowing player drops
- [x] **Inventory full overflow**: MVP drops + consumable generation overflow to ground

### Phase 11: Polish & Testing
- [x] Pickup sound effect (tier-based, gated by OptionsSubsystem per-tier toggle)
- [x] Pickup animation on character (ESpriteAnimState::Pickup on LocalSprite)
- [x] Test: 10+ items dropping simultaneously from one monster
- [x] Test: Two players racing to pick up same item
- [x] Test: Ownership timer phases (3s, 5s, 7s transitions)
- [x] Test: MVP ownership extended timers
- [x] Test: Party Each Take vs Party Share modes
- [x] Test: Party Shared distribution (random member)
- [x] Test: Weight limit prevents pickup
- [x] Test: 90% overweight prevents pickup
- [x] Test: Inventory full prevents pickup (item stays on ground)
- [x] Test: Item despawn after 60 seconds
- [x] Test: Zone transition — new player sees existing ground items
- [x] Test: Player drop from inventory creates ground item
- [x] Test: Stackable item partial drop
- [x] Test: Cannot drop equipped items
- [x] Test: Bubble Gum doubles drop rates
- [x] Test: Drop rate 90% cap with Bubble Gum
- [x] Test: Greed skill picks up all items in range
- [x] Test: Looter monster picks up and drops items on death
- [x] Test: Server restart clears all ground items (expected)

---

## Appendix A: Existing Code References

| Reference | Location |
|-----------|----------|
| `rollEnemyDrops()` | `server/src/index.js:4984` |
| `getDropTierColor()` | `server/src/index.js:4972` |
| `addItemToInventory()` | `server/src/index.js:5030` |
| `removeItemFromInventory()` | `server/src/index.js:5257` |
| `processCardDropBonuses()` | `server/src/index.js:3811` |
| `processCardKillHooks()` | `server/src/index.js:3265` |
| Skill death handler (drops) | `server/src/index.js:2479-2531` |
| Auto-attack death handler (drops) | `server/src/index.js:27811-27865` |
| `inventory:dropped` handler | `server/src/index.js:25220-25247` |
| `enemy.inCombatWith` init | `server/src/index.js:4580` |
| `enemy.mode.looter` parse | `server/src/index.js:449` |
| Party `item_share` column | `server/src/index.js:6145, 10021` |
| Steal skill handler | `server/src/index.js:14073-14155` |
| Greed skill stub | `server/src/index.js:21903-21905` |
| Ore Discovery | `server/src/index.js:2534-2579` |
| MVP damage tracking | `server/src/index.js:2370-2421` |

## Appendix B: Item Icon Assets

Ground item actors need item icon textures. Our icon system already has 6169 item icons at `/Game/SabriMMO/Icons/`. The `icon` field from the items table maps to texture asset names. `GroundItemActor` loads the texture the same way `InventorySubsystem` does — via `LoadObject<UTexture2D>()` with the icon name.

## Appendix C: Audio Assets

| Sound | Asset Path | When |
|-------|-----------|------|
| Drop landing (per tier) | `/Game/SabriMMO/Audio/SFX/Drops/drop_<color>.wav` | `item:spawned` animation lands |
| Pickup | `/Game/SabriMMO/Audio/SFX/UI/item_pickup.wav` (needs creation) | `item:pickup_success` |
| Pickup error | Existing UI error sound | `item:pickup_error` |

## Appendix D: Monster Template Drop Format

Current format in `ro_monster_templates.js`:
```javascript
{
    name: 'Poring',
    drops: [
        { itemName: 'Jellopy', chance: 0.70 },            // 70%
        { itemName: 'Knife', chance: 0.01 },               // 1%
        { itemName: 'Sticky Mucus', chance: 0.04 },        // 4%
        { itemName: 'Apple', chance: 0.55 },               // 55%
        { itemName: 'Empty Bottle', chance: 0.15 },        // 15%
        { itemName: 'Poring Card', chance: 0.0001, stealProtected: true }, // 0.01%
    ],
    mvpDrops: [] // Only for MVP monsters
}
```

**Note:** `chance` field uses direct probability (0.0 to 1.0 range). `Math.random() < drop.chance` is the roll. This differs from rAthena's per-10000 format but produces identical results.
