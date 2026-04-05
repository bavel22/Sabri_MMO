# Performance, Tick Loop, and Memory Audit

**Date**: 2026-03-23
**File audited**: `server/src/index.js` (~32,600 lines)
**Methodology**: 7-pass static analysis of all setInterval/setTimeout, Map/Set lifecycle, algorithm complexity, DB usage, and event loop blocking

---

## Pass 1: All setInterval Tick Loops

The server runs **17 concurrent setInterval loops**. Each one iterates global data structures every tick.

| # | Line | Interval | Purpose | Iterates | Complexity per tick |
|---|------|----------|---------|----------|---------------------|
| 1 | 153 | 60s | Party invite expiry cleanup | `pendingInvites` Map | O(I) where I = pending invites |
| 2 | 5266 | 1s | Socket rate limit counter reset | `socketEventCounts` Map | O(1) — `.clear()` |
| 3 | 24778 | **50ms** | **Combat tick** (cast completion, Maximize Power SP drain, Venom Splasher detonation, auto-attack) | `activeCasts` + `connectedPlayers` + `enemies` + `autoAttackState` | **O(P + E + A)** — P=players, E=enemies, A=auto-attackers. Each auto-attacker does `await getPlayerPosition` (Redis RTT) + damage calc. **Heaviest loop.** |
| 4 | 26551 | 1s | Party HP/SP sync | `activeParties` -> `members` | O(Pa * M) — Pa=parties, M=members |
| 5 | 26577 | 6s | HP natural regen | `connectedPlayers` | O(P) |
| 6 | 26637 | 6s | SP natural regen | `connectedPlayers` | O(P) |
| 7 | 26691 | 10s | Skill regen (Inc HP/SP Recovery) | `connectedPlayers` | O(P) |
| 8 | 26743 | 10s | Sitting regen (Spirits Recovery) | `connectedPlayers` | O(P) |
| 9 | 26775 | 5s | Card periodic effects (HP/SP regen/drain) | `connectedPlayers` | O(P) |
| 10 | 26814 | **1s** | **Status/Buff tick** (status expiry, poison/bleed DoT, buff expiry, Hiding/Cloaking SP drain, Sight/Ruwach reveal, Sight Blaster trigger, **performance tick**, **ensemble tick**, enemy status/buffs) | `connectedPlayers` + `enemies` + `activeEnsembles` | **O(P * E + E)** — performance tick iterates enemies/players for AoE. Second heaviest loop. |
| 11 | 27531 | 60s | Homunculus hunger tick | `activeHomunculi` | O(H) |
| 12 | 27580 | 10s | Homunculus HP/SP regen | `activeHomunculi` | O(H) |
| 13 | 27621 | 10s | Pet hunger tick | `activePets` | O(Pt) |
| 14 | 27684 | **250ms** | **Ground effects tick** (Fire Wall damage, Storm Gust/LoV/Meteor Storm waves, expired cleanup) | `activeGroundEffects` + `enemies` (per effect) | **O(G * E)** — each ground effect iterates all enemies for AoE hits |
| 15 | 30107 | **200ms** | **Enemy AI tick** (idle wander, aggro scan, chase, attack, monster skills) | `enemies` + inner `connectedPlayers` scan | **O(E * P)** — aggressive mobs scan all players for aggro |
| 16 | 31176 | **200ms** | **Trap trigger tick** (enemy-trap proximity, AoE detonation) | `activeGroundEffects` + `enemies` (per trap) | **O(T * E)** — each trap checks all enemies |
| 17 | 32550 | 60s | Periodic DB save (health, mana, EXP, zone) | `connectedPlayers` | O(P) with **3 sequential `await pool.query` per player** |

### Critical Tick Budget Analysis

The **50ms combat tick** (loop #3) is the tightest constraint. Within 50ms, it must:
1. Iterate `activeCasts` for cast completion
2. Iterate `connectedPlayers` for Maximize Power SP drain
3. Iterate `enemies` for Venom Splasher detonation (includes AoE sub-iteration)
4. Iterate `autoAttackState` for auto-attack resolution

Each auto-attacker calls **`await getPlayerPosition()`** which does a **Redis GET** (network RTT ~0.5-2ms). With 100 players auto-attacking, that is 50-200ms of Redis RTT alone, **exceeding the 50ms tick budget**.

The combat tick is also `async` with `await` inside the loop body. If any `await` takes longer than expected, the entire tick overruns and the next tick fires immediately, creating a backlog.

---

## Pass 2: Memory Leak Analysis

### Global Map/Set Inventory

| Map/Set | Line | Entries added when... | Entries removed when... | Leak risk |
|---------|------|-----------------------|-------------------------|-----------|
| `connectedPlayers` | 144 | `player:join` | `disconnect` handler (line 7420) | **LOW** — properly cleaned |
| `activeParties` | 147 | `party:create` | `party:leave` when last member leaves | **LOW** — but parties with all-offline members persist until server restart |
| `pendingInvites` | 148 | `party:invite` | 60s cleanup interval or accept/decline | **LOW** — cleanup handles expiry |
| `autoAttackState` | 477 | `combat:attack` | Disconnect, death, target lost, out of range | **LOW** — multiple cleanup paths |
| `activeHomunculi` | 480 | `homunculus:summon` | Disconnect, death, rest command | **LOW** |
| `activePets` | 481 | `pet:hatch` | Disconnect, death | **LOW** |
| `activePlants` | 484 | `summon_flora` skill | Disconnect, plant death, timer | **MEDIUM** — if plant owner disconnects but plant death event races |
| `playerPlants` | 485 | `summon_flora` skill | Disconnect | **LOW** |
| `activeMarineSpheres` | 489 | `summon_marine_sphere` skill | Disconnect, detonation | **LOW** |
| `playerSpheres` | 490 | `summon_marine_sphere` skill | Disconnect | **LOW** |
| `activeCasts` | 521 | Skill cast start | Cast completion (50ms tick) or disconnect | **LOW** |
| `afterCastDelayEnd` | 523 | Cast completion | Read on next skill use | **MEDIUM** — never explicitly cleaned; entries accumulate for offline chars until their key is overwritten |
| `activeGroundEffects` | 568 | Ground skill placement | Expiry in 250ms tick, or `removeGroundEffect()` | **LOW** — expiry check runs every 250ms |
| `activeEnsembles` | 977 | Ensemble start | Ensemble end (disconnect/death/cc/sp) | **LOW** |
| **`enemies`** | 4266 | `spawnEnemy()` | **Slave death only** (`enemies.delete` at line 2539). Normal enemies are NEVER deleted — they respawn in-place. | **NONE for leaks** — but the Map grows permanently with all spawned zones. Once a zone is spawned, its enemies remain forever even if no players are in the zone. |
| `itemDefinitions` | 4542 | Server startup | Never | **NONE** — static data |
| `socketEventCounts` | 5263 | Each socket event | `.clear()` every 1s | **LOW** |
| `spawnedZones` | 5140 | First player enters zone | Never cleared | **NONE** — bounded by zone count (~46) |

### Critical Finding: `afterCastDelayEnd` Map

The `afterCastDelayEnd` Map (line 523) stores a timestamp for each character that completes a cast. Entries are never removed — they are only overwritten on next cast. If a character disconnects and never reconnects, the entry persists forever. Over time with thousands of unique character IDs, this Map grows unbounded.

**Severity**: LOW (each entry is just `charId -> timestamp`, ~32 bytes). Would take millions of unique characters to matter.

### Critical Finding: `enemies` Map Never Shrinks

Normal enemies are never removed from the `enemies` Map. On death, `enemy.isDead = true` and a `setTimeout` respawns them in-place. Only slave enemies get `enemies.delete()`. Once a zone is spawned, its enemies persist permanently in memory.

With 46 spawn points averaging ~10 enemies each, this is ~460 enemy objects (~2-5KB each) = ~1-2MB total. Not a leak, but the Map is always at full size regardless of player activity.

### Critical Finding: `enemy.inCombatWith` Sub-Maps

Each enemy has an `inCombatWith: new Map()` for MVP damage tracking. This is reset on respawn (`new Map()` at line 2547) and entries are cleaned on disconnect (line 7398). **No leak**, but during combat with many players attacking the same enemy, this can grow to P entries per enemy.

### setTimeout without clearTimeout

There are **zero** `clearTimeout` or `clearInterval` calls in the entire 32,600-line file. All `setTimeout` calls are fire-and-forget:

- **Enemy respawn timers** (line 2542): 5s-130min delays. If an enemy is somehow deleted while a respawn timer is pending, the callback will try to mutate a dead reference. However, since enemies are never deleted (only slaves), this is a low risk.
- **Skill multi-hit delays** (~30 instances): 150-200ms delays for broadcasting individual hit numbers. These are purely cosmetic broadcasts and safe to fire-and-forget.
- **Hotbar data delay** (line 5926): 100ms delay after join. Safe.
- **`_justSpawned` flag** (line 4497): 1s flag clear. Safe.

**No `setTimeout` leak risk identified.** All `setTimeout` callbacks reference data that remains valid for their duration.

---

## Pass 3: Event Listener Cleanup

### Socket Listener Cleanup

The `disconnect` handler (line 7236) performs comprehensive cleanup:
- Removes from `connectedPlayers` (line 7420)
- Cleans up `activeCasts` and `afterCastDelayEnd` (line 7258-7259)
- Cancels performances and ensembles (line 7262-7267)
- Cleans up `root_lock` buffs (line 7273-7282)
- Closes vending shops with DB cleanup (line 7284-7293)
- Saves all state to DB (health, stats, EXP, zone, position, homunculus, pet, plagiarism)
- Removes from `activeHomunculi`, `activePets`, `playerPlants`, `playerSpheres` (line 7331-7364)
- Cleans up `autoAttackState` for self and all attackers targeting this player (line 7378-7394)
- Cleans `enemy.inCombatWith` for ALL enemies (line 7397-7416) -- **O(E) iteration**
- Broadcasts `player:left`

**Finding**: The disconnect handler iterates ALL enemies (line 7397) to clean combat references. With 460+ enemies, this is measurable but runs only once per disconnect.

### Socket Rooms

Players join zone-specific Socket.io rooms (`socket.join('zone:' + zone)`) and leave them on zone transition. Socket.io handles room cleanup on disconnect automatically. **No leak.**

---

## Pass 4: O(n^2) and Worse Algorithm Hotspots

### CRITICAL: Combat Tick — `await getPlayerPosition` in Loop (Line 24952)

```
for each auto-attacker:
    await getPlayerPosition(attackerId)  // Redis GET — ~1ms RTT
    // ... damage calculation, more awaits
```

This is **O(A) Redis calls per 50ms tick** where A = active auto-attackers. Each Redis call is ~0.5-2ms. With 50 auto-attackers, this alone consumes 25-100ms, **blowing the tick budget**.

**The position data (`player.lastX/lastY/lastZ`) is already cached on the player object** (updated on every `player:position` event at line 5976). The Redis call is redundant for auto-attack range checks.

**Severity**: CRITICAL. This is the #1 performance bottleneck.

### CRITICAL: Enemy AI Aggro Scan — O(E * P) per 200ms (Line 30187)

```
for each enemy (if aggressive):
    for each player in connectedPlayers:
        // distance check
```

Every 200ms, every aggressive enemy scans ALL connected players (not just those in the same zone). The zone filter (`p.zone !== enemy.zone`) is present but happens after the iteration starts. With 200 aggressive enemies and 100 players, that is 20,000 distance calculations every 200ms.

The `getActiveZones()` optimization (line 30109-30114) skips enemies in empty zones, which helps, but within an active zone all enemies still scan all players globally.

**Severity**: HIGH. This is the #2 bottleneck. A per-zone player index would reduce this to O(E_zone * P_zone).

### HIGH: `getActiveZones()` — O(P) per 200ms (Line 5132)

```
function getActiveZones() {
    const zones = new Set();
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone) zones.add(player.zone);
    }
    return zones;
}
```

Called every 200ms in the enemy AI tick. Iterates ALL players to build a Set of active zones. With 100 players across 5 zones, this is 100 iterations to produce a 5-element Set.

**Fix**: Maintain a `zonePlayerCount` Map that increments/decrements on join/leave/zone-change. Then `getActiveZones()` is O(Z) where Z is the number of zones (~10-20).

### HIGH: `broadcastToZone` Element Enrichment — O(1) but Called Thousands of Times

`broadcastToZone` (line 5099) does an `enemies.get()` lookup for every `skill:effect_damage` event to enrich with element modifier data. This is O(1) per call but the function is called thousands of times per second in active combat.

### HIGH: `getGroundEffectsAtPosition()` — O(G) Linear Scan (Line 599)

```
function getGroundEffectsAtPosition(x, y, z, radius) {
    for (const [id, effect] of activeGroundEffects.entries()) {
        // distance check
    }
}
```

Called in the combat tick for **every auto-attack** (Safety Wall/Pneuma check, line 24977-24998) and in the enemy AI attack path (line 30602). With 20 active ground effects and 50 auto-attackers, that is 1,000 distance checks per 50ms tick.

### HIGH: Ground Effects Tick — O(G * E) per 250ms (Line 27684)

Each ground effect (Fire Wall, Storm Gust, LoV, Meteor Storm) iterates ALL enemies for AoE damage. Storm Gust with while-loop catch-up can process multiple waves per tick, multiplying the cost.

With 5 active Storm Gusts and 200 enemies: 5 * 200 = 1,000 distance calculations per 250ms tick, plus damage calculations for hits.

### MEDIUM: Trap Trigger Tick — O(T * E) per 200ms (Line 31176)

Each active trap checks ALL enemies for proximity. With 10 traps and 200 enemies, that is 2,000 distance checks per 200ms.

### MEDIUM: Performance Tick — O(P_performing * (E + P)) per 1s (Line 27180)

Each performing Bard/Dancer iterates all enemies (offensive) or all players (supportive) for AoE. Also checks for song overlap by iterating all performing players. Nested iteration for overlap detection.

### MEDIUM: `findAggroTarget()` — O(P) per call (Line 28709)

Called by aggressive enemies during aggro scan. Iterates ALL connected players. Called from within the enemy AI tick which already iterates all enemies — making it O(E_aggressive * P).

Also calls `getCombinedModifiers(player)` for EACH player in the loop, which itself iterates buff arrays.

### MEDIUM: `triggerAssist()` — O(E) per call (Line 28631)

When an enemy is hit, `triggerAssist` iterates ALL enemies to find same-type allies within assist range. Called on every successful attack against an assist-mode enemy.

### MEDIUM: `isZoneActive()` — O(P) per call (Line 5125)

Linear scan of all players to check if any are in a zone. Used in multiple places. Could be replaced with the `zonePlayerCount` Map.

### LOW: `getPlayersInZone()` — O(N) Redis KEYS + O(N) GETs (Line 31951)

Uses `redisClient.keys('player:*:position')` which is O(N) and explicitly warned against in Redis documentation for production use. However, this function does not appear to be called from any hot path (no callers found in tick loops). Likely only used in initialization.

### LOW: Disconnect `enemy.inCombatWith` Cleanup — O(E) (Line 7397)

Iterates ALL enemies on each disconnect to remove the player from combat maps. Runs once per disconnect, not in a tick loop.

---

## Pass 5: Database Connection Management

### Pool Configuration

```js
const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
});
```

**No `max` pool size specified.** The `pg` library defaults to **10 connections**. With 339 `pool.query()` call sites and potentially hundreds of concurrent players, this is a bottleneck.

### Connection Release

- **5 transaction blocks** found (shop buy/sell, batch buy/sell, forging) — all use `pool.connect()` + `try/finally { client.release() }`. Properly managed.
- **334 other queries** use `pool.query()` (implicit checkout/checkin). These are safe — `pg` handles connection return automatically.
- **No leaked connections** found.

### Query Volume in Hot Paths

The **60-second periodic DB save** (loop #17) runs **3 sequential queries per player**:
1. `savePlayerHealthToDB` — UPDATE characters
2. `saveExpDataToDB` — UPDATE characters
3. UPDATE characters SET zone_name

With 100 players, that is 300 sequential queries every 60 seconds. Each query takes ~1-5ms, so 300-1500ms of DB work. This runs in an `async` setInterval and does not block the event loop, but it does consume 300 pool connections (sequentially, not concurrently — each `await` releases before the next).

The **disconnect handler** runs **5-8 sequential queries** (health, stats, zone, homunculus, pet, vending, plagiarism). If many players disconnect simultaneously (server restart, network outage), this could saturate the 10-connection pool.

### Transactions

All 5 transaction blocks properly handle `BEGIN/COMMIT/ROLLBACK/release()`. No stuck transactions found.

---

## Pass 6: Event Loop Blocking

### Synchronous File I/O

- `fs.existsSync(logsDir)` (line 80) — **once at startup only**. No issue.
- `fs.mkdirSync(logsDir)` (line 81) — **once at startup only**. No issue.
- `fs.createWriteStream` for logging (line 83) — non-blocking stream. No issue.

**No synchronous file I/O in hot paths.**

### CPU-Intensive Calculations

- **Damage calculation** (`calculatePhysicalDamage`, `calculateMagicSkillDamage`): Involves stat lookups, element table, size penalty, DEF calculation. Pure math, runs in ~0.01-0.05ms per call. **Not blocking**.
- **`getEffectiveStats()`** (line 3781): Aggregates 6 stat sources (base + equipment + cards + passives + buffs + pets). Calls `getPassiveSkillBonuses()` which has ~20 `if` branches. Runs in ~0.01ms. Called multiple times per combat tick per player. **Not blocking individually** but contributes to cumulative cost.
- **`getCombinedModifiers()`** (line 1704): Calls `getStatusModifiers()` + `getBuffModifiers()` (both iterate arrays). Called extremely frequently — in aggro scan, combat tick, status tick, performance tick. Each call iterates the entity's active buffs and status effects.

### JSON Serialization

83 `JSON.stringify` calls found. Most are in `logger.info` statements (only run at INFO level and above). The `broadcastToZone` function delegates to Socket.io's `emit()` which handles serialization internally with efficient buffering. **No blocking JSON serialization in hot paths.**

### Redis in Combat Tick (`getPlayerPosition`)

**This is the biggest event loop blocker.** Each `await getPlayerPosition()` is a Redis GET that suspends the combat tick's execution context. While one `await` is pending, the event loop can process other events, but the combat tick cannot progress to the next auto-attacker until the Redis response arrives. This serializes Redis RTTs, making the total tick time proportional to the number of auto-attackers.

---

## Pass 7: Scalability Analysis

### Bottleneck Projection

| Players | Enemies (est.) | Combat tick (50ms budget) | AI tick (200ms budget) | Status tick (1s budget) |
|---------|----------------|---------------------------|------------------------|------------------------|
| 10 | ~100 | ~5ms (safe) | ~10ms (safe) | ~20ms (safe) |
| 50 | ~200 | ~30-50ms (borderline) | ~40ms (safe) | ~80ms (safe) |
| 100 | ~300 | **~100-200ms (OVERRUN)** | **~100ms (borderline)** | ~150ms (safe) |
| 500 | ~500 | **~500ms+ (broken)** | **~300ms+ (OVERRUN)** | ~500ms (borderline) |
| 1000 | ~600 | **~1000ms+ (unplayable)** | **~600ms+ (broken)** | **~1000ms+ (OVERRUN)** |

### Primary Bottleneck: Redis RTT in Combat Tick

At 100 players with 50 auto-attacking, the combat tick calls `getPlayerPosition` 50 times sequentially. At 1ms per Redis RTT, that is 50ms just for Redis calls — exactly the tick budget. Add damage calculation, broadcasts, and DB writes for kills, and the tick overruns.

**Fix**: Use `player.lastX/lastY/lastZ` from the in-memory `connectedPlayers` Map instead of Redis. The data is already cached there (updated on every `player:position` event). This eliminates all Redis RTT from the combat tick.

### Secondary Bottleneck: Global Entity Iteration

Every tick loop iterates global Maps (`enemies`, `connectedPlayers`, `activeGroundEffects`) without zone partitioning. The `getActiveZones()` check skips empty zones but still requires building a Set from all players every tick.

**Fix**: Maintain per-zone indexes:
- `zoneEnemies: Map<zoneName, Set<enemyId>>` — O(1) zone lookup instead of O(E) filter
- `zonePlayers: Map<zoneName, Set<charId>>` — O(1) zone lookup instead of O(P) filter
- `zonePlayerCount: Map<zoneName, number>` — O(1) active zone check

### Tertiary Bottleneck: No Spatial Index for AoE

All AoE calculations iterate every entity in a zone and compute distance. With 100 enemies in one zone, every AoE skill does 100 distance calculations. A simple grid-based spatial hash (50-unit cells matching RO cell size) would reduce AoE queries to O(affected_cells) instead of O(E_zone).

### DB Pool Exhaustion

The default `pg` pool of 10 connections can be exhausted when:
- 60s periodic save fires (300 sequential queries for 100 players)
- Multiple disconnects happen simultaneously (5-8 queries each)
- Shop transactions hold connections during BEGIN/COMMIT
- Skill handlers that do inventory queries (339 total `pool.query` call sites)

**Fix**: Set `max: 20-30` on the pool. Consider using `pool.query()` exclusively (no manual `pool.connect()`) for non-transactional queries to minimize connection hold time.

---

## Summary of Findings

### CRITICAL (Fix immediately)

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| C1 | `await getPlayerPosition()` (Redis GET) in combat tick loop | Line 24952 | Serialized Redis RTTs blow 50ms tick budget at >50 players. **Use `player.lastX/lastY/lastZ` instead.** |
| C2 | No pool `max` size configured | Line 31427 | Default 10 connections can be exhausted under load. **Set `max: 20-30`.** |

### HIGH (Fix before scaling)

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| H1 | Enemy AI aggro scan iterates ALL players globally | Line 30187 (findAggroTarget) | O(E_aggressive * P) per 200ms tick. Maintain per-zone player index. |
| H2 | `getActiveZones()` iterates all players every 200ms | Line 5132 | O(P) every tick. Maintain zone player count Map. |
| H3 | Ground effects iterate ALL enemies per effect | Line 27684+ | O(G * E) per 250ms. Add zone filtering to ground effect iteration. |
| H4 | `getGroundEffectsAtPosition()` linear scan | Line 599 | O(G) per call, called per auto-attack and per enemy attack. Partition by zone. |
| H5 | 60s DB save runs 3 sequential queries per player | Line 32550 | 300+ queries with 100 players. Batch into bulk UPDATE or use transactions. |

### MEDIUM (Fix for production)

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| M1 | `afterCastDelayEnd` Map never cleaned | Line 523 | Grows unboundedly with unique character IDs. Add cleanup on disconnect. |
| M2 | Performance tick has O(P * E) nested iteration | Line 27231 | Each performing Bard/Dancer scans all enemies or all players. |
| M3 | `triggerAssist()` iterates ALL enemies | Line 28631 | O(E) per assist-eligible hit. Partition by zone + template. |
| M4 | `getCombinedModifiers()` called excessively in inner loops | Multiple | Called per-entity in aggro scan, per-auto-attacker, per-enemy-tick. Cache per tick. |
| M5 | `getPlayersInZone()` uses Redis KEYS pattern | Line 31952 | O(N) and Redis-blocking. Not in hot path currently but dangerous if used. |
| M6 | Trap tick iterates ALL ground effects + ALL enemies | Line 31176 | O(G * E) per 200ms. Filter traps from non-trap effects, add zone index. |

### LOW (Technical debt)

| # | Issue | Location | Impact |
|---|-------|----------|--------|
| L1 | Zero `clearTimeout`/`clearInterval` calls in entire codebase | Global | No ability to stop tick loops gracefully on shutdown. |
| L2 | `activeParties` with all-offline members persist forever | Line 147 | Parties survive until server restart. Add periodic cleanup. |
| L3 | Disconnect handler iterates ALL enemies for combat cleanup | Line 7397 | O(E) per disconnect. Acceptable for single events. |
| L4 | `enemy.inCombatWith` not bounded | Line 4484 | Can grow to P entries per enemy. Reset on respawn mitigates. |

---

## Recommended Priority Fixes

### Phase 1: Eliminate Redis from Combat Tick (Estimated: 1 hour)

Replace all `await getPlayerPosition(attackerId)` calls in the combat tick with direct reads from `player.lastX/lastY/lastZ` on the `connectedPlayers` Map. This data is already maintained by the `player:position` socket event handler.

Also applies to `getPlayerPosSync()` calls if they exist.

**Expected improvement**: Combat tick drops from ~100ms to ~10ms at 100 players.

### Phase 2: Per-Zone Entity Indexes (Estimated: 3-4 hours)

Add three maintained indexes:
```js
const zoneEnemies = new Map();    // zoneName -> Set<enemyId>
const zonePlayers = new Map();    // zoneName -> Set<charId>
const zonePlayerCount = new Map(); // zoneName -> number
```

Update on: spawn, death, zone transition, disconnect, enemy respawn.

Replace in: `findAggroTarget()`, `getActiveZones()`, ground effects tick, performance tick, trap tick, `isZoneActive()`, `triggerAssist()`.

**Expected improvement**: AI tick drops from O(E * P) to O(E_zone * P_zone). With 5 zones averaging 20 players each, that is a 5x improvement.

### Phase 3: Increase Pool Size + Batch DB Saves (Estimated: 1 hour)

- Add `max: 25` to Pool config
- Batch the 60s periodic save into a single multi-row UPDATE using `unnest()` arrays or a transaction with pipelining

### Phase 4: Ground Effect Zone Filtering (Estimated: 2 hours)

Add `zone` field to all ground effects (already present on most). Filter `getGroundEffectsAtPosition()` by zone. Maintain a `zoneGroundEffects` index.

### Phase 5: Cache `getCombinedModifiers()` Per Tick (Estimated: 2 hours)

Add a tick-local cache (Map) that stores computed modifiers keyed by entity ID. Clear at the start of each tick. Prevents redundant buff/status array iteration for the same entity across multiple callers in the same tick.
