# Socket Event Security Audit

**Auditor**: Claude Opus 4.6 (automated)
**Date**: 2026-03-23
**Target**: `server/src/index.js` (32,566 lines, 80 socket.on handlers)
**Architecture**: Server-authoritative MMO (Node.js + Socket.io + PostgreSQL)

---

## Executive Summary

The server is broadly well-designed for a server-authoritative architecture. Combat damage, stat calculations, ASPD enforcement, skill validation, and economy transactions all happen server-side. However, the audit found **5 CRITICAL**, **8 HIGH**, **11 MEDIUM**, and **9 LOW** severity issues across 6 passes.

### Severity Breakdown

| Severity | Count | Description |
|----------|-------|-------------|
| CRITICAL | 5 | Can break server authority, duplicate items/zeny, or crash the server |
| HIGH | 8 | Can bypass intended game mechanics for significant advantage |
| MEDIUM | 11 | Can gain moderate advantage or leak information |
| LOW | 9 | Minor abuse potential, defense-in-depth improvements |

---

## Pass 1: Socket Event Handler Inventory

### Authentication Model
- `player:join` -- JWT-verified, character ownership checked against DB. **GOOD.**
- All other handlers use `findPlayerBySocketId(socket.id)` to resolve the player from socket identity. **GOOD** (most handlers).
- **EXCEPTION**: `player:position` takes `characterId` from client payload (line 5976). **CRITICAL.**

### Handler Summary (80 total)

| Category | Events | Auth Model |
|----------|--------|------------|
| Connection | `player:join`, `disconnect` | JWT + DB ownership |
| Movement | `player:position`, `player:sit`, `player:stand` | Socket-ID lookup (position: client ID!) |
| Zone | `zone:warp`, `zone:ready` | Socket-ID lookup |
| NPC | `kafra:open`, `kafra:save`, `kafra:teleport` | Socket-ID lookup |
| Cart | `cart:load`, `cart:rent`, `cart:remove`, `cart:move_to_cart`, `cart:move_to_inventory` | Socket-ID lookup |
| Vending | `vending:start`, `vending:close`, `vending:browse`, `vending:buy` | Socket-ID lookup |
| Identify | `identify:select` | Socket-ID lookup |
| Combat | `combat:attack`, `combat:stop_attack`, `combat:respawn` | Socket-ID lookup |
| Stats | `player:request_stats`, `buff:request`, `mount:toggle`, `player:allocate_stat` | Socket-ID lookup |
| Job | `job:change` | Socket-ID lookup |
| Skills | `skill:data`, `skill:learn`, `skill:reset`, `skill:use` | Socket-ID lookup |
| Party | `party:load/create/invite/respond/leave/kick/change_leader/change_exp_share/chat` | Socket-ID lookup |
| Chat | `chat:message` | Socket-ID lookup |
| Inventory | `inventory:load/use/equip/drop/move/merge` | Socket-ID lookup |
| Hotbar | `hotbar:save/request/save_skill/clear` | Socket-ID lookup |
| Shop | `shop:open/buy/sell/buy_batch/sell_batch` | Socket-ID lookup |
| Crafting | `refine:request`, `forge:request`, `pharmacy:craft`, `crafting:craft_converter` | Socket-ID lookup |
| Companions | `homunculus:feed/command/skill_up/use_skill/evolve`, `pet:tame/incubate/return/feed/rename/list`, `summon:detonate` | Socket-ID lookup |
| Card | `card:compound` | Socket-ID lookup |
| Misc | `warp_portal:confirm`, `equipment:repair` | Socket-ID lookup |
| Debug | `debug:apply_status/remove_status/list_statuses` | NODE_ENV check |

### Rate Limiting
Socket-level rate limiting is implemented via `socket.use()` middleware with per-event-type limits (lines 5222-5292). Events exceeding the per-second limit are silently dropped. **GOOD.** However, not all events have limits configured (see Pass 5).

---

## Pass 2: Server-Authoritative Violations

### CRITICAL-01: `player:position` Accepts Unverified characterId from Client

**Location**: Line 5976
**Severity**: CRITICAL
**Description**: The `player:position` handler reads `characterId` from `data.characterId` (client-provided) instead of resolving it from the socket identity. A malicious client can send position updates for ANY connected player by specifying their characterId.

```javascript
// VULNERABLE (line 5976):
const characterId = parseInt(data.characterId);
const { x, y, z } = data;
const player = connectedPlayers.get(characterId);
```

**Impact**:
- Teleport other players by sending crafted position updates with their characterId
- Move other players into walls, off cliffs, or into enemy aggro ranges
- Break other players' casts (movement interrupts casting)
- Trigger warp portal collisions for other players (force zone transitions)
- Overwrite other players' lastX/lastY/lastZ (corrupts their server-side position)

**Fix**: Replace `parseInt(data.characterId)` with socket-identity resolution:
```javascript
const playerInfo = findPlayerBySocketId(socket.id);
if (!playerInfo) return;
const { characterId, player } = playerInfo;
const { x, y, z } = data;
```

### CRITICAL-02: No Position Distance/Speed Validation (Teleport Hack)

**Location**: Lines 5974-6282
**Severity**: CRITICAL
**Description**: The `player:position` handler accepts ANY x/y/z coordinates with no distance-from-previous-position validation (except during Tunnel Drive hiding at line 6074). A client can instantly teleport anywhere within a zone by sending extreme coordinates.

**Checks that DO exist**: vending lock, CC lock, ensemble lock, performance speed reduction, close confine, sitting, blade stop, play dead, hiding (Tunnel Drive only), walk delay, moonlit water barrier, ice wall collision.

**Checks that DO NOT exist**: Maximum distance per tick, speed validation, position bounds checking.

**Impact**:
- Teleport to any location within current zone instantly
- Speed hack: move faster than allowed by moving larger distances per tick
- Exploit enemy aggro by rapidly repositioning
- Skip past warp portal triggers or obstacle collision

**Fix**: Add server-side distance-per-tick validation:
```javascript
const maxDistPerUpdate = calculateMaxMoveDistance(player, timeDelta);
const actualDist = Math.sqrt((x - player.lastX)**2 + (y - player.lastY)**2);
if (actualDist > maxDistPerUpdate * TOLERANCE) {
    // Clamp or reject
}
```

### Assessment of What IS Server-Authoritative (Positive Findings)

The following ARE properly server-authoritative:
- **HP/SP values**: Client never sends HP/SP. All healing, damage, regen computed server-side. **GOOD.**
- **Damage values**: Client sends `combat:attack` (target selection only). Damage calculated by server combat tick. **GOOD.**
- **Stat values**: Client sends `player:allocate_stat` with stat name only. Cost calculated server-side. **GOOD.**
- **Buffs**: Client cannot set buffs. All buffs applied via server skill handlers. **GOOD.**
- **ASPD enforcement**: Auto-attack timing controlled by server combat tick loop at 50ms intervals. Client only requests target. **GOOD.**
- **Skill validation**: Server checks learned level, SP cost, cooldowns, catalysts, class restrictions. **GOOD.**
- **Inventory operations**: Server verifies ownership via `character_id` in all queries. **GOOD.**
- **Equipment bonuses**: Calculated server-side from DB item definitions. **GOOD.**
- **Zeny transactions**: Atomic DB transactions with BEGIN/COMMIT/ROLLBACK. **GOOD.**

---

## Pass 3: Exploitation Vectors

### CRITICAL-03: Combat:Attack Has No Zone Check for PvE Targets

**Location**: Lines 7564-7616
**Severity**: HIGH (downgraded from CRITICAL because enemy AI loop has zone awareness)
**Description**: When a client sends `combat:attack` with a `targetEnemyId`, the handler checks if the enemy exists and is alive, but does NOT verify the enemy is in the same zone as the attacker.

```javascript
if (targetEnemyId != null) {
    const enemy = enemies.get(targetEnemyId);
    if (!enemy) { /* error */ return; }
    if (enemy.isDead) { /* error */ return; }
    // NO ZONE CHECK HERE
    autoAttackState.set(attackerId, { targetCharId: targetEnemyId, isEnemy: true, ... });
```

**Mitigating factor**: The combat tick loop (line 24897+) uses range checks based on player/enemy positions, so damage would likely not occur cross-zone unless coordinates overlap. However, this still allows setting aggro cross-zone and entering the enemy's `inCombatWith` set.

**Impact**:
- Can potentially attack enemies in other zones if coordinates happen to overlap
- Can set `autoAttackState` pointing to cross-zone enemies (wastes server resources)
- Enemy `inCombatWith` set contaminated with cross-zone players

**Fix**: Add zone check after enemy lookup:
```javascript
if (enemy.zone !== attacker.zone) {
    socket.emit('combat:error', { message: 'Enemy not in your zone' });
    return;
}
```

### HIGH-01: Skill:Use Has No Range Check for Targeted Skills

**Location**: Lines 9122-9421
**Severity**: HIGH
**Description**: The `skill:use` handler validates skill ownership, SP cost, cooldowns, catalysts, and CC state, but does NOT validate that the target is within skill range for single-target skills. Range validation only happens inside specific skill handlers (not the generic path).

**Impact**: A client could cast targeted skills on enemies/players anywhere in the zone regardless of range.

**Fix**: Add range check in the generic `skill:use` path before routing to specific handlers.

### HIGH-02: No Maximum Position Coordinate Bounds

**Location**: `player:position` handler
**Severity**: HIGH
**Description**: Position coordinates are accepted as-is with no bounds checking. A client could send `x: Infinity`, `x: NaN`, or `x: 999999999` which would corrupt server state and potentially cause NaN propagation in distance calculations.

**Impact**:
- NaN/Infinity in position breaks distance calculations for aggro, AoE, warp portals
- Potential NaN propagation into damage formulas via distance-based skills
- Redis position cache corrupted

**Fix**: Validate coordinates are finite numbers within zone bounds.

### HIGH-03: Identify:Select Has No Skill/Item Cost Validation

**Location**: Lines 6933-6960
**Severity**: HIGH
**Description**: The `identify:select` handler identifies items with no check that the player has the Identify skill (Merchant 600) or is using a Magnifier item. A client can identify any unidentified item for free by emitting `identify:select`.

**Impact**: Unlimited free item identification, bypassing Magnifier item consumption and Identify skill requirement.

**Fix**: Check that the player has a pending identify session (set by Magnifier use or Identify skill).

### HIGH-04: Vending:Buy Not Wrapped in a DB Transaction

**Location**: Lines 7106-7233
**Severity**: HIGH
**Description**: The `vending:buy` handler performs multiple DB operations (deduct buyer zeny, add vendor zeny, transfer items, reduce stock) without wrapping them in a transaction. If any operation fails midway (e.g., DB connection lost), the state becomes inconsistent.

```javascript
// These are separate queries, not atomic:
await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [player.zeny, characterId]);
await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [vendor.zeny, vendorId]);
// ... item transfers ...
```

**Impact**:
- Zeny deducted from buyer but not added to vendor (or vice versa)
- Items transferred without proper payment
- Potential zeny duplication or loss on partial failure
- Race condition: concurrent buys from same shop could oversell stock

**Fix**: Wrap all vending:buy operations in a single `BEGIN`/`COMMIT` transaction, similar to how `shop:buy` works (lines 23827-23857).

### HIGH-05: Equipment:Repair Has No NPC Proximity Check

**Location**: Line 23284
**Severity**: MEDIUM (not HIGH, as weapon break is rare)
**Description**: The `equipment:repair` handler repairs a broken weapon but does not check if the player is near a repair NPC or has the Weapon Repair skill.

**Fix**: Add NPC proximity or skill check.

### HIGH-06: Chat Message Not HTML-Sanitized for XSS

**Location**: Lines 8682-8926
**Severity**: HIGH
**Description**: Chat messages are trimmed and length-limited (255 chars), but not sanitized for HTML/script content. While the client is UE5 Slate (not a web browser), if any future web admin panel, log viewer, or web-based client reads these messages, they would be vulnerable to stored XSS.

**Impact**: Currently low (UE5 Slate doesn't interpret HTML). Future risk if any web interface is added.

**Fix**: Sanitize messages by stripping or escaping HTML characters.

### HIGH-07: Party:Invite Target Name Leak (Online Status Oracle)

**Location**: Lines 8425-8465
**Severity**: MEDIUM
**Description**: `party:invite` searches for target by name among `connectedPlayers`. If not found, it returns an error revealing the player is offline. This provides an oracle for checking if any character name is currently online.

**Impact**: Information disclosure -- check if specific players are online by spamming party invites with different names.

### HIGH-08: Skill:Use skillLevel from Client Can Downcast

**Location**: Lines 9332-9336
**Severity**: LOW (intentional feature for RO Classic hotbar level selection)
**Description**: The client can send `data.skillLevel` to use a lower skill level than learned. This is intentional per RO Classic mechanics, but worth documenting as it allows intentional downcasting (e.g., lower SP cost, different effect).

---

## Pass 4: Economy Exploits

### CRITICAL-04: Race Condition in Vending:Buy (Concurrent Purchase Exploit)

**Location**: Lines 7106-7233
**Severity**: CRITICAL
**Description**: The `vending:buy` handler reads stock from DB, checks quantity, then modifies it in separate queries without transaction isolation. Two buyers sending `vending:buy` simultaneously for the same item can both pass the stock check before either deduction executes.

**Sequence**:
1. Vendor lists 1x Rare Sword at 1,000,000z
2. Buyer A sends `vending:buy` -- reads stock = 1, passes check
3. Buyer B sends `vending:buy` -- reads stock = 1, passes check (query runs before A's deduction)
4. Both buyers receive the item, vendor gets paid twice
5. 1 item duplicated into 2

**Impact**: Item duplication via concurrent vending purchases. Potentially catastrophic for rare/expensive items.

**Fix**: Use a DB transaction with `SELECT ... FOR UPDATE` to lock the vending_items row:
```javascript
const client = await pool.connect();
await client.query('BEGIN');
const stock = await client.query('SELECT amount FROM vending_items WHERE vend_item_id = $1 FOR UPDATE', [vendItemId]);
// ... validate and deduct within same transaction ...
await client.query('COMMIT');
```

### CRITICAL-05: Potential Race Condition in Inventory:Use + Inventory:Drop

**Location**: Lines 22203-22725 and 23472-23514
**Severity**: HIGH
**Description**: If a client rapidly sends `inventory:use` and `inventory:drop` for the same item, both handlers read the item from DB separately. The use handler consumes the item (removes 1 from quantity), and the drop handler also removes it. Since both are async and use separate DB queries (no transaction), this could result in using an item without consuming it or consuming it twice.

**Mitigating factor**: Rate limiting (5/s each) reduces exploitability. The `removeItemFromInventory` helper may have internal safety, but both paths access the same inventory row concurrently.

**Impact**: Potentially use consumables without consuming them. Severity depends on `removeItemFromInventory` implementation.

**Fix**: Use inventory-level locking or ensure atomic consume+delete.

### MED-01: Shop:Buy Does Not Apply Discount

**Location**: Lines 23794-23858
**Severity**: MEDIUM
**Description**: The `shop:buy` handler uses `itemDef.buy_price || itemDef.price * 2` without applying the Discount skill percentage. The `shop:buy_batch` handler correctly applies `applyDiscount()`. This means the deprecated single-buy path overcharges players with Discount skill.

**Impact**: Minor -- players using the deprecated single-buy path overpay. Not an exploit but a discrepancy.

### MED-02: Vending Price Cap Allows 1 Billion Zeny Items

**Location**: Line 6986
**Severity**: MEDIUM
**Description**: Vending item prices are capped at 1,000,000,000 (1 billion) zeny. Combined with the zeny max of 2,147,483,647 (INT32_MAX), a vendor could price items to cause overflow in buyer calculations if multiple items are purchased.

**Impact**: Potential integer overflow if total purchase exceeds INT32_MAX. The `vending:buy` handler does not check total against MAX_ZENY on the vendor receiving side.

**Fix**: Add `vendor.zeny + totalCost > MAX_ZENY` check.

### MED-03: No Zeny Overflow Check on Vendor Side in Vending:Buy

**Location**: Lines 7149-7155
**Severity**: MEDIUM
**Description**: When a buyer purchases from a vendor, the vendor's zeny is incremented without checking for overflow:
```javascript
vendor.zeny = (vendor.zeny || 0) + totalCost;
```
If vendor.zeny + totalCost > 2^31-1, this causes integer overflow.

**Impact**: Vendor zeny wraps to negative or corrupted value.

**Fix**: Add MAX_ZENY check before crediting vendor.

### MED-04: Cart:Move_to_Cart and Cart:Move_to_Inventory Not Transactional

**Location**: Lines 6780-6866 and 6868-6933
**Severity**: MEDIUM
**Description**: Cart transfer operations delete from one table and insert into another without wrapping in a transaction. Failure between delete and insert loses the item.

**Impact**: Item loss on DB failure during cart transfers.

**Fix**: Wrap in `BEGIN`/`COMMIT` transaction.

---

## Pass 5: Denial of Service Vectors

### MED-05: Events Without Rate Limits

**Severity**: MEDIUM
**Description**: The following events have NO configured rate limit and are not in `SOCKET_RATE_LIMITS`:

| Event | DB Queries | Broadcast | Risk |
|-------|-----------|-----------|------|
| `player:join` | 12+ queries | Zone broadcast | Heavy on DB, once per connect |
| `zone:warp` | 2+ queries | Zone broadcast | Medium |
| `zone:ready` | 0 queries | Zone broadcast | Low |
| `kafra:open` | 1 query | None | Low |
| `kafra:save` | 1 query | None | Low |
| `kafra:teleport` | 3+ queries | Zone broadcast | Medium |
| `combat:respawn` | 3+ queries | Zone broadcast | Medium |
| `player:request_stats` | 0 queries | None | Low (compute) |
| `buff:request` | 0 queries | None | Low |
| `mount:toggle` | 0 queries | Zone broadcast | Low |
| `job:change` | 2+ queries | Zone + global broadcast | Medium |
| `skill:data` | 1 query | None | Medium |
| `skill:learn` | 3+ queries | None | Medium |
| `skill:reset` | 2+ queries | None | Medium |
| `skill:use` | 5+ queries | Zone broadcast | **HIGH** -- most expensive |
| `pharmacy:craft` | 5+ queries | None | Medium |
| `crafting:craft_converter` | 3+ queries | None | Medium |
| `summon:detonate` | 0 queries | Zone broadcast | Low |
| `homunculus:*` (5 events) | 1-3 queries each | Various | Medium |
| `pet:*` (6 events) | 1-3 queries each | Various | Medium |
| `inventory:use` | 5+ queries (already 5/s) | Zone broadcast | Covered |
| `inventory:equip` | 10+ queries (already 5/s) | None | Covered |
| `card:compound` | 5+ queries | None | Medium |
| `refine:request` | 5+ queries | None | Medium |
| `forge:request` | 10+ queries | None | Medium |
| `shop:open` | 2 queries | None | Medium |
| `equipment:repair` | 2 queries | None | Low |
| `warp_portal:confirm` | 0 queries | Zone broadcast | Low |
| `inventory:merge` | 3 queries | None | Medium |

**Most expensive unrated event**: `skill:use` -- can trigger 5+ DB queries, complex damage calculations, AoE iterations over all enemies, ground effect creation, and zone broadcasts. At 10/s limit it IS rate-limited, but other events like `forge:request` (10+ queries) have no limit.

**Fix**: Add rate limits to all events that trigger DB queries. Recommended additions:
```javascript
'player:join':          1,   // Once per connection
'zone:warp':            2,
'kafra:teleport':       2,
'combat:respawn':       2,
'job:change':           1,
'skill:data':           3,
'skill:learn':          5,
'skill:reset':          1,
'card:compound':        3,
'refine:request':       3,
'forge:request':        2,
'shop:open':            5,
'pharmacy:craft':       3,
'crafting:craft_converter': 3,
'equipment:repair':     3,
'inventory:merge':      5,
'homunculus:feed':      2,
'homunculus:command':    5,
'homunculus:skill_up':  3,
'homunculus:use_skill':  5,
'homunculus:evolve':    1,
'pet:tame':             2,
'pet:incubate':         2,
'pet:feed':             2,
'pet:rename':           2,
'pet:list':             3,
```

### MED-06: Debug Handlers Active Based on NODE_ENV

**Location**: Lines 24572-24659
**Severity**: MEDIUM (CRITICAL if NODE_ENV unset)
**Description**: Three debug handlers (`debug:apply_status`, `debug:remove_status`, `debug:list_statuses`) are gated by `process.env.NODE_ENV !== 'development'`. If NODE_ENV is not set or set to 'development' in production, ANY client can:
- Apply status effects (stun, freeze, stone) to any player or enemy
- Remove status effects from any target
- List all active statuses/buffs on any target

**Impact**: Full status effect manipulation of any entity in the game. The `forceApplyStatusEffect` bypasses immunity checks.

**Fix**: Either remove debug handlers entirely, or add additional authentication (admin flag, debug token).

### MED-07: Unbounded Ground Effects / Ensembles

**Severity**: MEDIUM
**Description**: Ground effects (Warp Portal, Fire Pillar, Safety Wall, etc.) are created in `activeGroundEffects` Map. While some have per-caster limits (e.g., Warp Portal = max 3), others are bounded only by SP and cooldowns. A determined attacker could create many ground effects by spam-casting.

**Mitigating factor**: SP cost, catalyst consumption, cast times, after-cast delay, and rate limiting on `skill:use` (10/s) all limit the creation rate.

### MED-08: Chat Message Global Broadcast

**Location**: Lines 8896-8900
**Severity**: MEDIUM
**Description**: Global chat uses `io.emit()` which broadcasts to ALL connected sockets. A client at 5 messages/second (rate limit) sends 5 broadcasts/second to every connected player.

**Impact**: With N players, global chat generates N*5 messages/second server-to-client. At scale (1000+ players), this becomes bandwidth-intensive.

**Fix**: Consider zone-based chat as default, with global chat requiring a higher cost (level requirement, cooldown, etc.).

---

## Pass 6: Information Disclosure

### MED-09: Enemy Health Broadcast Reveals All Enemy HP

**Severity**: LOW
**Description**: `enemy:spawn` and `enemy:health_update` events broadcast exact current HP and max HP for all enemies in a zone. This is standard for MMOs (health bars are visible).

**Assessment**: Normal game behavior. Not a vulnerability.

### MED-10: Player Health Broadcast to Zone

**Severity**: LOW
**Description**: `combat:health_update` broadcasts exact HP/maxHP/SP/maxSP for all players in the same zone. This is needed for party HP bars and PvP targeting.

**Assessment**: Normal. SP visibility could enable strategy (know when a mage is out of SP), but this is standard in RO Classic.

### MED-11: Whisper Reveals Online Status

**Severity**: MEDIUM
**Description**: The `/w` whisper command responds with "user is offline" if the target is not connected, confirming their offline status. The party:invite handler has the same issue.

**Impact**: Online status oracle. A client can check if any character name is currently online.

**Fix**: Use a generic "message could not be delivered" response that doesn't distinguish online/offline.

### LOW-01: Vending Shop Data Visible Across Zone

**Severity**: LOW
**Description**: When a player joins a zone, they receive all active vending shop data (vendor name, title, position) for that zone. This is expected behavior (vendor shops are public).

### LOW-02: Enemy Exact Coordinates Broadcast

**Severity**: LOW
**Description**: Enemy positions (x, y, z) are broadcast to the entire zone. This enables map hacks that show all enemy positions, but since enemies are meant to be visible anyway, this is low severity.

### LOW-03: Player Position Broadcast to Zone

**Severity**: LOW
**Description**: `player:moved` broadcasts exact x, y, z coordinates of all players in the zone. Standard MMO behavior.

### LOW-04: Hidden Players Not Fully Invisible

**Severity**: LOW
**Description**: When a player activates Hiding or Cloaking, there's no explicit removal of their position from the broadcast. Other clients may still see position updates until the next broadcast tick. However, the Hiding/Cloaking skill handlers do broadcast buff application which clients can use to hide the model.

**Assessment**: Depends on client implementation. If the client properly hides the actor on buff receipt, this is fine.

### LOW-05: Stats Payload Contains Full Build Information

**Severity**: LOW
**Description**: `player:stats` is only sent to the owning socket (not broadcast). However, if someone else gains access to the socket (session hijack), they see full stat allocation, equipment bonuses, and derived stats.

### LOW-06: Card Bonuses Visible via Stats

**Severity**: LOW
**Description**: The stats payload includes aggregated card bonuses. Not individual card identities, but the net effect.

### LOW-07: Party Member Data Includes Map/HP/SP

**Severity**: LOW
**Description**: Party members can see each other's current map, HP, SP, level, and online status. This is standard party functionality.

### LOW-08: Learned Skills Visible in Skill:Data

**Severity**: LOW
**Description**: `skill:data` response includes all learned skill IDs and levels. Sent only to the owning socket.

### LOW-09: Chat Command Injection Surface

**Severity**: LOW
**Description**: Chat messages starting with `/` are parsed for commands (`/w`, `/r`, `/ex`, `/memo`, `/mount`). Malformed commands are rejected, but the parsing logic should be audited for edge cases (e.g., very long names in `/w`).

---

## Recommended Fixes by Priority

### P0 -- Fix Immediately (CRITICAL)

1. **CRITICAL-01**: Fix `player:position` to use `findPlayerBySocketId(socket.id)` instead of `data.characterId`
2. **CRITICAL-02**: Add distance-per-tick validation to `player:position` with configurable tolerance
3. **CRITICAL-04**: Wrap `vending:buy` in a DB transaction with row-level locking (`SELECT ... FOR UPDATE`)

### P1 -- Fix Soon (HIGH)

4. **CRITICAL-03**: Add zone check in `combat:attack` for enemy targets
5. **CRITICAL-05**: Ensure inventory operations use transactions or atomic helpers
6. **HIGH-03**: Add identify session state check in `identify:select`
7. **HIGH-06**: Sanitize chat messages (strip HTML entities)
8. **MED-06**: Remove or secure debug handlers (require admin token, not just NODE_ENV)

### P2 -- Fix for Hardening (MEDIUM)

9. **HIGH-01**: Add range validation for targeted skills in `skill:use`
10. **HIGH-02**: Validate position coordinates are finite and within zone bounds
11. **MED-02/03**: Add MAX_ZENY overflow checks in vending:buy (vendor side)
12. **MED-04**: Wrap cart transfers in DB transactions
13. **MED-05**: Add rate limits to all DB-triggering events
14. **MED-08**: Consider zone-scoped default chat channel
15. **MED-11**: Use generic "delivery failed" for whisper to offline players

### P3 -- Defense in Depth (LOW)

16. **LOW-04**: Ensure hidden player positions stop broadcasting
17. Audit all `parseInt()` calls for NaN handling
18. Add input validation for all string fields (shop titles, chat messages, character names)
19. Consider adding a session token on the socket (in addition to JWT on join) to prevent session fixation

---

## Appendix A: Per-Event Security Assessment

### Connection Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `player:join` | JWT + DB ownership check | Full -- loads all state from DB | GOOD |
| `disconnect` | Socket identity | Full -- saves state to DB | GOOD |

### Movement Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `player:position` | CC checks, vending, ensemble, sitting, barriers | **BROKEN** -- accepts client characterId, no distance validation | CRITICAL |
| `player:sit` | Dead, vending, hiding, casting, performing | Full | GOOD |
| `player:stand` | isSitting check | Full | GOOD |

### Combat Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `combat:attack` | Dead, vending, CC, sitting, weight, target exists/alive | Full (damage from server tick) but **missing zone check** | HIGH |
| `combat:stop_attack` | None needed | Full | GOOD |
| `combat:respawn` | isDead check | Full -- uses save point from DB | GOOD |

### Skill Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `skill:use` | Dead, vending, CC, Steel Body, Blade Stop, performance, ensemble, ACD, learned level, SP, catalysts, cooldown | Full (all damage/effects server-side) but **missing range check** | HIGH |
| `skill:learn` | Skill exists, canLearnSkill validation, skill points | Full | GOOD |
| `skill:reset` | Socket identity | Full -- refunds from DB | GOOD |
| `skill:data` | Socket identity | Full -- reads from DB | GOOD |

### Economy Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `shop:buy` | Shop/item validation, level req, zeny check, DB transaction | Full | GOOD |
| `shop:sell` | Ownership, equipped check, DB transaction | Full | GOOD |
| `shop:buy_batch` | Full validation + transaction | Full | GOOD |
| `shop:sell_batch` | Full validation + transaction + overflow check | Full | GOOD |
| `vending:buy` | Stock check, zeny check, weight check | **Missing transaction** -- race condition | CRITICAL |
| `vending:start` | Cart, skill level, price cap | Full | GOOD |
| `refine:request` | Ownership, refineable, max level, ore, zeny | Full | GOOD |
| `forge:request` | Recipe, skill, materials, zeny | Full | GOOD |

### Inventory Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `inventory:use` | CC checks, ownership (character_id in query), item type, status restrictions | Full | GOOD |
| `inventory:equip` | Ownership, level req, class/gender, identified, two-hand rules | Full | GOOD |
| `inventory:drop` | Ownership (character_id in query) | Full | GOOD |
| `inventory:move` | Ownership (character_id in query) | Full | GOOD |
| `inventory:merge` | Ownership, same item_id, stackable | Full | GOOD |
| `card:compound` | Ownership, card type, equip slot, slot availability | Full | GOOD |
| `identify:select` | Ownership, identified check | **Missing skill/item requirement** | HIGH |

### Party Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `party:create` | Not in party, name length | Full (DB-backed) | GOOD |
| `party:invite` | Party membership, leader check, online check | Online status leak | MEDIUM |
| `party:invite_respond` | Pending invite, party size | Full | GOOD |
| `party:leave` | Party membership | Full | GOOD |
| `party:kick` | Leader check | Full | GOOD |
| `party:change_leader` | Leader check, target membership | Full | GOOD |
| `party:change_exp_share` | Leader check | Full | GOOD |
| `party:chat` | Party membership | Full | GOOD |

### Debug Events

| Event | Validation | Server Auth | Rating |
|-------|-----------|-------------|--------|
| `debug:apply_status` | NODE_ENV check | **Dangerous in dev/unset** | MEDIUM |
| `debug:remove_status` | NODE_ENV check | **Dangerous in dev/unset** | MEDIUM |
| `debug:list_statuses` | NODE_ENV check | Info disclosure in dev | MEDIUM |

---

## Appendix B: Rate Limit Coverage

### Covered (35 events)
`player:position`, `player:moved`, `combat:attack`, `combat:stop_attack`, `skill:use`, `chat:message`, `inventory:use`, `inventory:equip`, `inventory:move`, `inventory:drop`, `shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch`, `hotbar:save`, `hotbar:save_skill`, `hotbar:clear`, `player:allocate_stat`, `cart:rent`, `cart:remove`, `cart:move_to_cart`, `cart:move_to_inventory`, `identify:select`, `vending:start`, `vending:close`, `vending:browse`, `vending:buy`, `party:create`, `party:invite`, `party:invite_respond`, `party:leave`, `party:kick`, `party:change_leader`, `party:change_exp_share`, `party:chat`

### Not Covered (45 events)
`player:join`, `zone:warp`, `zone:ready`, `kafra:open`, `kafra:save`, `kafra:teleport`, `cart:load`, `combat:respawn`, `player:request_stats`, `buff:request`, `mount:toggle`, `job:change`, `skill:data`, `skill:learn`, `skill:reset`, `party:load`, `inventory:load`, `hotbar:request`, `pharmacy:craft`, `crafting:craft_converter`, `summon:detonate`, `homunculus:feed`, `homunculus:command`, `homunculus:skill_up`, `homunculus:use_skill`, `homunculus:evolve`, `pet:tame`, `pet:incubate`, `pet:return_to_egg`, `pet:feed`, `pet:rename`, `pet:list`, `card:compound`, `warp_portal:confirm`, `inventory:merge`, `shop:open`, `refine:request`, `forge:request`, `equipment:repair`, `disconnect`, `player:sit`, `player:stand`, `debug:apply_status`, `debug:remove_status`, `debug:list_statuses`

---

## Appendix C: Architecture Strengths

The following security decisions are well-implemented and should be preserved:

1. **JWT on player:join**: Proper token verification with character ownership check against DB
2. **Socket-identity resolution**: Most handlers (78/80) use `findPlayerBySocketId()` -- only `player:position` and `player:join` read characterId from payload
3. **Parameterized queries throughout**: All SQL uses `$1, $2` placeholders -- no SQL injection vectors found
4. **Atomic transactions for shop operations**: `shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch` all use `BEGIN`/`COMMIT`/`ROLLBACK`
5. **Server-computed damage**: All damage from combat tick loop, never from client
6. **Server-computed stats**: Equipment bonuses, derived stats, ASPD all server-side
7. **Ownership checks in all inventory queries**: `WHERE ci.character_id = $1` prevents cross-character access
8. **Rate limiting middleware**: Event-level throttling with silent drop prevents naive spam
9. **Zeny overflow check on sell**: `shop:sell_batch` checks MAX_ZENY before crediting
10. **Weight validation**: Multiple handlers check weight before item addition
