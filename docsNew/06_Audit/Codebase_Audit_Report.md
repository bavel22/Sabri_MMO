# Sabri_MMO — Full Codebase Audit Report

**Date**: 2026-03-08
**Revision**: 2 (triple-verified — all findings cross-checked by 5 independent verification agents)
**Scope**: Server (`server/src/index.js` ~8,455 lines), Client C++ (all subsystems), Database (`init.sql` + auto-migrations)
**Method**: 3-agent initial audit → manual line-by-line verification → 5-agent deep re-verification of every finding

---

## Verification Status Legend

- **CONFIRMED** — Verified against actual source code by 2+ independent checks. Real bug/gap.
- **FALSE POSITIVE** — Flagged by audit but code is correct or handled elsewhere.
- **CORRECTED** — Original finding was real but severity/description was inaccurate; corrected below.

---

## CRITICAL — Fix Immediately

### C1. Authentication Bypass — No Token = No Auth ✅ CONFIRMED

**Location**: `server/src/index.js:1480-1499`
**Severity**: CRITICAL (Security)

```javascript
if (rawToken) {
    try {
        const decoded = jwt.verify(rawToken, process.env.JWT_SECRET);
        // ... ownership check, return on failure
    } catch (err) {
        socket.emit('player:join_error', { error: 'Invalid or expired token' });
        return;
    }
}
// NO ELSE — execution continues without auth if rawToken is falsy
```

**Problem**: If a client sends `player:join` with no token (or `token: ""`), the entire JWT block is skipped. The handler proceeds to load character data from DB and fully connect the player. Any Socket.io client can impersonate any character by sending `{ characterId: 5 }` with no token.

**Fix**: Add after line 1499:
```javascript
else {
    socket.emit('player:join_error', { error: 'Authentication required' });
    return;
}
```

---

### C2. Undefined Variable `ptId` — Magnum Break PvP ✅ CONFIRMED (CORRECTED: Dormant)

**Location**: `server/src/index.js:3753`
**Severity**: CRITICAL code bug, but **currently dormant** — only triggers when `PVP_ENABLED` is true (line 3721)

```javascript
// Line 3722: loop variable is `pid`
for (const [pid, ptarget] of connectedPlayers.entries()) {
    // ...
    // Line 3753: references `ptId` which DOES NOT EXIST
    if (activeCasts.has(ptId)) interruptCast(ptId, 'damage');
}
```

**Problem**: `ptId` is never declared. In non-strict mode, `activeCasts.has(undefined)` returns false — cast interruption silently fails. In strict mode, throws `ReferenceError`. Either way, PvP Magnum Break will never interrupt an opponent's cast.

**Current state**: PvP is disabled (`PVP_ENABLED` is false), so this code path never executes. Will crash/fail the moment PvP is enabled.

**Fix**: Change `ptId` to `pid` on line 3753.

---

### C3. Wrong Column `WHERE id =` — Butterfly Wing Completely Broken ✅ CONFIRMED (CORRECTED: Worse than originally reported)

**Location**: `server/src/index.js:4936, 4965`
**Severity**: CRITICAL (Broken Feature + Error)

```javascript
// Line 4936:
`SELECT save_map, save_x, save_y, save_z FROM characters WHERE id = $1`
// Line 4965:
`UPDATE characters SET zone_name = $1, x = $2, y = $3 WHERE id = $4`
```

**Original report said**: Queries return 0 rows / update 0 rows (silent failure).
**Actually**: PostgreSQL throws `ERROR: column "id" does not exist` because the `characters` table PK is `character_id`. There is no `id` column at all. This means Butterfly Wing **crashes with a DB error on every use**, not just silently fails.

The item is consumed BEFORE the query (line 4933 `removeItemFromInventory` runs first), so the player loses the Butterfly Wing and gets a database error.

**Fix**: Change both to `WHERE character_id = $1` / `WHERE character_id = $4`.

---

### C4. Missing Z-Coordinate in Butterfly Wing UPDATE ✅ CONFIRMED

**Location**: `server/src/index.js:4965`
**Severity**: HIGH (Data Loss — only matters after C3 is fixed)

```javascript
`UPDATE characters SET zone_name = $1, x = $2, y = $3 WHERE id = $4`
// saveZ is computed on line 4943 but never included in the query
```

**Problem**: Even after fixing C3, this UPDATE only saves x and y — z is lost. The `saveZ` variable is computed but never included. The periodic 60-second save (line 8446) DOES save z correctly, so if the player stays online the position self-corrects, but if they disconnect right after Butterfly Wing, z-position is lost.

**Fix**: Change to `UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5` with params `[saveMap, saveX, saveY, saveZ, characterId]`.

---

### C5. Hotbar Slot Normalization Runs Every Restart ✅ CONFIRMED (CORRECTED: More nuanced)

**Location**: `server/src/index.js:8289-8299`
**Severity**: HIGH (Data Corruption OR Migration Failure)

```javascript
// "one-time migration" that runs on EVERY server restart
const result = await pool.query(
  `UPDATE character_hotbar SET slot_index = slot_index - 1 WHERE slot_type = 'skill' AND slot_index >= 1`
);
```

**Problem**: No idempotency guard. Runs on every startup. The PK is `(character_id, row_index, slot_index)`.

**Detailed behavior** (corrected from original report):
- **If a player has skills at adjacent indices (e.g., 0, 1, 2)**: The UPDATE tries to move index 1→0, which collides with the existing slot at index 0. PostgreSQL rejects the entire UPDATE with a unique constraint violation. **The migration fails silently every startup** and the skills are never normalized from 1-based to 0-based.
- **If a player has a single skill at index 5 with no adjacent conflicts**: Progressive decrement — 5→4→3→2→1→0 over 5 restarts. Eventually converges at 0 but puts the skill in the wrong slot for 4 restarts.
- **If a player has skills at non-adjacent indices (e.g., 0, 3, 7)**: Index 3→2 and 7→6 on first restart, then 2→1 and 6→5, etc. Progressive drift toward 0 over multiple restarts.

**Fix**: Either:
- Track this migration in a `migrations` table so it runs exactly once, OR
- Delete this query entirely and instead use: `UPDATE character_hotbar SET slot_index = slot_index - 1 WHERE slot_type = 'skill' AND slot_index > 8` (only normalize truly out-of-range values)

---

## HIGH — Fix This Cycle

### H1. No DB Transactions for Shop Buy/Sell/Equip ✅ CONFIRMED

**Location**: `server/src/index.js:5488-5500 (buy), 5554-5559 (sell), 5106-5208 (equip)`
**Severity**: HIGH (Item Dupe / Item Loss)

Zero `BEGIN/COMMIT/ROLLBACK` statements exist anywhere in the entire file.

**Shop Buy** (line 5488-5500): Deducts zeny → adds item. Manual rollback at line 5496 if item add fails, but rollback can itself fail (`.catch(() => {})` in batch buy at line 5725 silently swallows rollback failures).

**Shop Sell** (line 5554-5559): Adds zeny → removes item. If removal fails, player keeps both the item AND the zeny (dupe).

**Equipment** (line 5106-5208): Multiple sequential UPDATE queries — any failure mid-sequence leaves DB and in-memory state inconsistent.

**Is this exploitable?** Yes. Node.js `await` yields to the event loop. Two rapid `shop:buy` events for the same player CAN interleave at await points.

**Fix**: Wrap each multi-step operation in `BEGIN/COMMIT/ROLLBACK` using `const client = await pool.connect()`.

---

### H2. Inventory Race Conditions (Check-Then-Act) ✅ CONFIRMED

**Location**: `server/src/index.js:1307-1351 (add), 1403-1423 (remove)`
**Severity**: HIGH (Item Dupe / Over-draw)

Classic TOCTOU (time-of-check-time-of-use) bug. Both `addItemToInventory` and `removeItemFromInventory` do SELECT-then-UPDATE as two separate queries. Between the await points, concurrent operations for the same character can interleave, causing lost stacks or over-draws.

**Fix**: Use atomic SQL: `UPDATE character_inventory SET quantity = quantity + $1 WHERE character_id = $2 AND item_id = $3 RETURNING quantity` instead of SELECT-then-UPDATE.

---

### H3. `findPlayerBySocketId()` is O(n) on Every Event ✅ CONFIRMED

**Location**: `server/src/index.js:1426-1433`
**Severity**: HIGH (Performance)

Called **31 times** across the codebase — at the top of virtually every socket event handler. No reverse lookup Map exists anywhere in the code.

**Fix**: Add `const socketIdToCharId = new Map();` — populate on `player:join`, remove on `disconnect`.

---

### H4. No Socket.io Rate Limiting ✅ CONFIRMED

**Location**: `server/src/index.js:7492` (HTTP only)
**Severity**: HIGH (Security / DoS)

HTTP routes have `rateLimit()`. Socket.io has zero throttling — no `io.use()` middleware, no per-event debounce.

**Fix**: Add per-socket rate limiting middleware for high-frequency events (`player:position`, `chat:message`, `combat:attack`, `skill:use`).

---

### H5. No Duplicate Session Prevention ✅ CONFIRMED (NEW — missed by original audit)

**Location**: `server/src/index.js:1662`
**Severity**: HIGH (State Corruption / Memory Leak)

```javascript
connectedPlayers.set(characterId, { ... }); // Silently overwrites if already connected
```

**Problem**: If a character joins twice (two sockets, reconnect before disconnect fires), the second `set()` silently overwrites the first entry. The original socket's `socketId` is lost, so the disconnect handler can never clean up:
- Orphaned entries in `autoAttackState`
- Orphaned entries in `activeCasts`, `activeBuffs`
- Stale character IDs in enemy `inCombatWith` sets
- Orphaned socket room memberships

**Fix**: Before `connectedPlayers.set()`, check if the character is already connected. If so, either disconnect the old socket first or reject the new connection.

---

### H6. No Graceful Shutdown Handler ✅ CONFIRMED (NEW — missed by original audit)

**Location**: Entire `server/src/index.js`
**Severity**: HIGH (Data Loss on Restart)

There are **8 `setInterval` calls** running persistent loops (combat tick 50ms, enemy AI 200ms, regen 6s/8s/10s, buff expiry 1s, ground effects 500ms, DB save 60s). None are stored in variables. There is no `process.on('SIGTERM')` or `process.on('SIGINT')` handler.

When the server receives a shutdown signal:
- All player data since the last 60s periodic save is lost
- DB connections not gracefully closed (`pool.end()` never called)
- Redis connection not closed
- Log file stream not flushed

**Fix**: Add SIGTERM/SIGINT handler that saves all connected player data, clears intervals, closes pool/Redis/logs, and calls `server.close()`.

---

### H7. Redis `KEYS` Command in `getPlayersInZone()` ✅ CONFIRMED (CORRECTED: Dead code — downgraded)

**Location**: `server/src/index.js:8029`
**Severity**: LOW (was HIGH — function is **never called anywhere**)

```javascript
async function getPlayersInZone(zone = 'default') {
    const keys = await redisClient.keys('player:*:position');
```

The function is defined but **never invoked** — zero callers exist. It uses the Redis `KEYS` command which is O(N) and dangerous in production, but since it's dead code, there is no current impact. It's a trap for future developers.

**Fix**: Delete the function or rewrite with `SCAN` if future use is planned.

---

### H8. Code Duplication: FindSocketIOComponent (13 files) ✅ CONFIRMED

**Location**: All UWorldSubsystem `.cpp` files
**Severity**: MEDIUM (Tech Debt)

The exact same ~15-line function is copy-pasted across 13 subsystem files. Implementations are functionally identical (only cosmetic variable name differences like `Comp` vs `SIO` in SkillVFXSubsystem).

**Fix**: Extract to a shared utility header or base class.

---

### H9. Code Duplication: WrapSingleEvent / WrapExistingEvent (13 files) ✅ CONFIRMED

**Location**: Same 13 subsystem files as H8
**Severity**: MEDIUM (Tech Debt)

`WrapSingleEvent()` in 11 files + `WrapExistingEvent()` in 2 files (HotbarSubsystem, SkillTreeSubsystem). Functionally identical — different name, same code.

**Fix**: Extract alongside H8 into shared utility.

---

## MEDIUM — Next Sprint

### M1. No Chat Message Length Limit ✅ CONFIRMED

**Location**: `server/src/index.js:3003-3006`
**Details**: Only checks `message.trim().length === 0`. No max length. A client can send megabyte-sized messages broadcast to all players via `io.emit()`.

---

### M2. No Chat Input Sanitization ✅ CONFIRMED

**Location**: `server/src/index.js:3013`
**Details**: Only `.trim()`. No HTML stripping, no control character filtering. Low XSS risk since UE5 Slate doesn't render HTML, but no defense-in-depth.

---

### M3. No Cross-Zone Combat Validation ✅ CONFIRMED

**Location**: `server/src/index.js:2364-2402 (combat:attack)`
**Details**: No check that `enemy.zone === player.zone`. The `enemies` Map is global, not zone-partitioned. A modified client could target an enemy in another zone by ID. The combat tick's range check via positions provides an indirect guard (cross-zone positions would be far apart), but there's no explicit zone validation.

---

### M4. Redis Reads in Combat Tick Loop ✅ CONFIRMED

**Location**: `server/src/index.js:6003+ (combat tick)`
**Details**: Combat tick calls `getPlayerPosition()` (Redis GET + JSON.parse) per attacker every 50ms. Meanwhile, the enemy AI tick (200ms) correctly uses `getPlayerPosSync()` which reads from in-memory `player.lastX/lastY/lastZ`. The combat tick should use the same in-memory approach.

---

### M5. DB Write Inside 50ms Combat Tick ✅ CONFIRMED

**Location**: `server/src/index.js:6483`
**Details**: `await savePlayerHealthToDB()` on PvP player death blocks the combat tick until DB write completes. Low frequency (only PvP kills) but adds latency to the tick when it occurs.

---

### M6. ~1,179 Individual DB Queries for Skill Sync on Startup ✅ CONFIRMED (CORRECTED: More than originally reported)

**Location**: `server/src/index.js:8217-8249`
**Details**: Original report said ~240 queries for ~20 skills. Actually there are **139 skills** (56 first-class + 83 second-class). With ~7 levels and ~0.5 prerequisites average per skill: 139 skill upserts + ~70 prerequisite upserts + ~970 level upserts = **~1,179 sequential queries** on every startup. All idempotent (`ON CONFLICT DO UPDATE`) but slow.

---

### M7. Inline Migrations Run Every Startup ✅ CONFIRMED (CORRECTED: Mostly idempotent)

**Location**: `server/src/index.js:8059-8426`
**Details**: Most DDL statements are idempotent and safe to re-run (`CREATE TABLE IF NOT EXISTS`, `ADD COLUMN IF NOT EXISTS`, `CREATE INDEX IF NOT EXISTS`). The **only non-idempotent statement** is the hotbar slot normalization (C5). The PK drop/re-add (line 8282-8283) fails harmlessly on subsequent runs (caught by try/catch). The `ALTER TABLE ALTER COLUMN DROP NOT NULL` (line 8266) is effectively idempotent in PostgreSQL but acquires an ACCESS EXCLUSIVE lock each time.

---

### M8. Inconsistent Error Handling ✅ CONFIRMED

**Location**: Throughout `server/src/index.js`
**Details**: Sampled 6 handlers:
- `inventory:load` (line 3051): **No try/catch at all** — unhandled rejection if `getPlayerInventory()` throws
- `skill:data` (line 2801): **No try/catch** — hangs if it throws (client waits forever)
- `inventory:drop` (line 5261): Full try/catch with error emit (correct)
- `inventory:use` (line 4855): Full try/catch with error emit (correct)
- `skill:learn` (line 2870): Partial — inner catch but outer handler unprotected
- `combat:attack` (line 2321): Synchronous handler, no async error risk

---

### M9. Global Broadcasts for Zone-Local Events ✅ CONFIRMED

**Location**: `server/src/index.js` — 5 locations use `io.emit('chat:receive', ...)`
**Details**: Lines 528 (base level-up), 532 (job level-up), 2783 (job change), 6190 (combat base level-up), 6197 (combat job level-up). All broadcast server-wide. The combat tick death announcement at line 6473 correctly uses `broadcastToZone()`. May be intentional (RO Classic announces level-ups server-wide).

---

### M10. `GET /api/characters/:id` Returns Deleted Characters ✅ CONFIRMED

**Location**: `server/src/index.js:7885-7888`
**Details**: `WHERE character_id = $1 AND user_id = $2` — missing `AND deleted = FALSE`. The list endpoint (line 7777) correctly filters with `AND deleted = FALSE`.

---

### M11. Character Name Uniqueness Blocks Deleted Names ✅ CONFIRMED

**Location**: `server/src/index.js:7830-7833`
**Details**: `SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1)` — no `AND deleted = FALSE`. Once used, a name is permanently reserved even after soft-delete. This matches RO Classic behavior (names are never recycled) but is not documented.

---

### M12. `init.sql` Items Table Missing Columns ✅ CONFIRMED (LOW IMPACT — CORRECTED)

**Location**: `database/init.sql:44-68`
**Details**: The CREATE TABLE in `init.sql` lacks `weapon_type`, `aspd_modifier`, `weapon_range`. The server's auto-migration handles this. **However**, the INSERT statements in `init.sql` (lines 128-135) reference these columns, so running `init.sql` alone against a fresh DB would cause INSERT failures. The setup docs note that `init.sql` is optional, so this is a documentation/maintenance issue, not a runtime bug.

---

### M13. No Inventory Weight/Slot Limits ✅ CONFIRMED

**Location**: `server/src/index.js:1307-1351 (addItemToInventory)`
**Details**: No weight check, no slot count check. `max_stack` is enforced for stackable items, but a player can hold unlimited distinct items with unbounded total weight. RO Classic has 100-slot limit and STR-based weight capacity.

---

### M14. `zoneTransitioning` Not Cleaned on Disconnect ✅ CONFIRMED (NEW — missed by original audit)

**Location**: `server/src/index.js` — disconnect handler (lines 2224-2317)
**Details**: The disconnect handler cleans up `activeCasts`, `autoAttackState`, `connectedPlayers`, enemy `inCombatWith` sets, and zone position. But it does NOT remove the characterId from the `zoneTransitioning` Set (line 1465). If a player disconnects mid-zone-transition, their ID stays in the set permanently. Minor memory leak.

---

### M15. Orphaned setTimeout for Enemy Respawn ✅ CONFIRMED (NEW — missed by original audit)

**Location**: `server/src/index.js:578, 6280`
**Details**: `setTimeout(() => { enemy respawn logic }, enemy.respawnMs)` — timer IDs are never stored. Cannot be cancelled for GM commands, server shutdown cleanup, or zone unloading. The respawn resets the existing enemy object (no infinite accumulation), but timers fire even after shutdown begins.

---

### M16. Some Async Socket Handlers Lack try/catch ✅ CONFIRMED (NEW — missed by original audit)

**Location**: `server/src/index.js` — `inventory:load` (line 3051), `skill:data` (line 2801), `player:allocate_stat` (line 2622)
**Details**: These async handlers have no top-level try/catch. If the DB query throws, the promise rejection propagates to Socket.io's default handler (logs but doesn't crash). The client receives no response and hangs.

---

## LOW — Backlog

### L1. `GEngine->AddOnScreenDebugMessage` in Production Code ✅ CONFIRMED

**Location**: `SkillTreeSubsystem.cpp` (10 calls), `HotbarSubsystem.cpp` (1 call)
**Details**: Violates the project's multiplayer-safe coding rules. In multi-PIE, messages from one instance bleed into another. All 11 calls have `UE_LOG` equivalents alongside them — the `GEngine` calls are redundant.

---

### L2. SCombatStatsWidget Raw UObject Pointer ✅ CONFIRMED (CORRECTED: Was marked FALSE POSITIVE, is actually a real LOW issue)

**Location**: `SCombatStatsWidget.h:29`
**Code**: `UCombatStatsSubsystem* Subsystem = nullptr;`

**Original report said**: FALSE POSITIVE — subsystem outlives widget.
**Re-verification found**: `SBasicInfoWidget` and `SWorldHealthBarOverlay` (same codebase, same pattern) both use `TWeakObjectPtr<USubsystemType>` for the same purpose. `SCombatStatsWidget` is the only widget using a raw pointer. During world teardown (PIE stop, level transitions), Slate widgets may briefly outlive their subsystem if still referenced by the viewport widget tree. The `TAttribute` lambdas capture `this` and dereference `Subsystem->` directly — a dangling pointer if the subsystem is GC'd first.

**Severity**: LOW — works correctly in practice during normal gameplay, but inconsistent with the established pattern and a potential crash during teardown edge cases.

**Fix**: Change to `TWeakObjectPtr<UCombatStatsSubsystem> Subsystem;` and use `Subsystem.Get()` with null checks, matching the other widgets.

---

### L3. `testModeRouter` Bypasses Rate Limiting ✅ CONFIRMED

**Location**: `server/src/index.js:7494-7495`
**Details**: `/test` routes have no rate limiting. Exposes mock data endpoints (`/test/setup`, `/test/character`, `/test/shop`, `/test/inventory`, `/test/combat`, `/test/socket-event`, `/test/reset`). These return hardcoded mock data (no real DB access), so risk is low. No `NODE_ENV` guard to disable in production.

---

### L4. Default DB Pool Configuration ✅ CONFIRMED

**Location**: `server/src/index.js:7504-7510`
**Details**: `new Pool({...})` with no `max`, `min`, `idleTimeoutMillis`, `connectionTimeoutMillis`, or `statement_timeout`. Uses pg defaults (max 10, no statement timeout). Fine for dev, may bottleneck under production load.

---

### L5. Inventory Slot Gaps ✅ CONFIRMED

**Location**: `server/src/index.js:1334-1338`
**Details**: `MAX(slot_index) + 1` creates permanent gaps when items are dropped/sold from the middle. Slot indices grow unbounded. No functional impact but creates sparse inventory grids.

---

### L6. Zuzucoin Race Condition ✅ CONFIRMED

**Location**: `server/src/index.js:5483-5490`
**Details**: In-memory check (`player.zuzucoin < totalCost`) + separate DB update. Two rapid `shop:buy` events can both pass the check before either write completes. Related to H1 (no transactions).

---

### L7. Soft-Deleted Character Inventory Never Cleaned ✅ CONFIRMED

**Location**: DB design
**Details**: Characters are soft-deleted (`deleted = TRUE`, `ON DELETE CASCADE` never fires). Inventory, hotbar, and skill rows for deleted characters persist forever. No cleanup job exists.

---

### L8. Dead Code: ASPD Tonic System ✅ CONFIRMED

**Location**: `server/src/index.js:225-232`
**Details**: `SPEED_TONICS` constant defined with 4 tonic types (`veil_draught`, `dusk_tincture`, `ember_salve`, `grey_catalyst`). Appears exactly once in the entire file — at its definition. Never referenced by any handler. Planned feature that was never implemented.

---

### L9. Dead Code: `getPlayersInZone()` ✅ CONFIRMED (Moved from H7)

**Location**: `server/src/index.js:8028-8041`
**Details**: Function defined but never called anywhere. Uses Redis `KEYS` command (O(N) keyspace scan). Delete or rewrite if future use is planned.

---

## FALSE POSITIVES — Not Real Issues

### FP1. SkillTreeSubsystem 1-Based slotIndex ❌ FALSE POSITIVE

**Originally reported as**: HIGH
**Location**: `SkillTreeSubsystem.cpp:411-418`

**Why it's fine**: The SkillTreeSubsystem sends 1-based slotIndex WITHOUT the `zeroBased` flag. The server explicitly handles this at `index.js:3159-3162`:
```javascript
if (slotIndex >= 1 && slotIndex <= 9 && !data.zeroBased) {
    slotIndex = slotIndex - 1; // Convert 1-based to 0-based
}
```
The HotbarSubsystem sends 0-based WITH `zeroBased: true`. Both paths are intentional and work correctly. No data corruption occurs.

---

### FP2. Missing DB Columns in `init.sql` Items Table ❌ FALSE POSITIVE (runtime)

**Originally reported as**: CRITICAL
**Location**: `database/init.sql:44-68`

**Why it's fine at runtime**: The server's auto-migration at `index.js:8086-8113` creates the table with all columns. The `ALTER TABLE ADD COLUMN IF NOT EXISTS` at line 8120-8127 adds any missing columns. No runtime impact. Setup docs note `init.sql` is optional. (The `init.sql` file IS internally inconsistent — INSERT statements reference columns not in its CREATE TABLE — but this is a docs/maintenance issue, not a production bug.)

---

## Summary

| Severity | Count | IDs |
|----------|-------|-----|
| CRITICAL | 3 | C1 (auth bypass), C3 (Butterfly Wing crash), C4 (missing z-coord) |
| CRITICAL (dormant) | 2 | C2 (ptId, PvP disabled), C5 (hotbar normalization) |
| HIGH | 6 | H1 (no transactions), H2 (inventory race), H3 (O(n) lookup), H4 (no socket rate limit), H5 (duplicate session), H6 (no graceful shutdown) |
| MEDIUM (tech debt) | 2 | H8-H9 (code duplication, 13 files each) |
| MEDIUM | 16 | M1-M16 |
| LOW | 9 | L1-L9 |
| FALSE POSITIVE | 2 | FP1 (slot index convention), FP2 (init.sql columns) |
| **Total confirmed** | **38** | |
| **False positives** | **2** | |

---

## Recommended Fix Order

### Immediate (1-5 line fixes — do right now):
1. **C1** — Auth bypass: add `else { emit error; return; }` after JWT check (3 lines)
2. **C2** — `ptId` → `pid` on line 3753 (1 line)
3. **C3** — `WHERE id =` → `WHERE character_id =` on lines 4936, 4965 (2 lines)
4. **C4** — Add `z = $4` to Butterfly Wing UPDATE + add `saveZ` to params (2 lines)

### This week (targeted fixes):
5. **C5** — Make hotbar normalization idempotent or one-time
6. **H5** — Add duplicate session check before `connectedPlayers.set()`
7. **H1** — Wrap shop buy/sell/equip in DB transactions
8. **H2** — Atomic SQL for inventory add/remove
9. **H3** — Add `socketIdToCharId` reverse Map for O(1) lookup

### Next cycle (infrastructure):
10. **H6** — Add SIGTERM/SIGINT handler with graceful shutdown
11. **H4** — Socket.io rate limiting middleware
12. **H8 + H9** — Extract shared SocketIO utilities (reduces ~800 lines of duplication across 13 files)
13. **M4** — Replace Redis reads in combat tick with in-memory `getPlayerPosSync()`
14. **M8 + M16** — Standardize async error handling (add try/catch to all async handlers)

### Backlog:
15. **M1-M3, M5-M7, M9-M15** — Medium-priority fixes
16. **L1-L9** — Low-priority cleanup
