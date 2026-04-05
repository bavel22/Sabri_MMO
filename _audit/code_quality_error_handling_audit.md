# Error Handling Audit — server/src/index.js

**Date**: 2026-03-23
**File**: `server/src/index.js` (32,566 lines)
**Auditor**: Automated multi-pass analysis

---

## Executive Summary

The server has **decent error handling coverage overall** — most REST routes and DB-touching socket handlers use try/catch. However, there are significant gaps in **process-level error handling** (no uncaughtException/unhandledRejection handlers), several **high-frequency socket handlers without try/catch** that could crash the server on unexpected input, and **7 fire-and-forget DB calls with `.catch(() => {})` that silently discard errors**. The combat tick loop and all `setInterval` loops run without top-level try/catch, meaning a single unhandled error in any tick iteration can crash the entire server.

**Critical findings**: 0 process-level crash guards, 15+ unprotected async socket handlers, 17 setInterval loops without try/catch wrappers, no database pool error handler, Redis calls without try/catch in helper functions.

---

## Pass 1: Try/Catch Block Catalog

### Total: ~155 try/catch blocks across the file

### 1A. REST API Routes (Lines 31499-31926) — ALL PROTECTED

Every REST route has try/catch with proper error responses:

| Route | Line | Protection | Error Response |
|-------|------|-----------|----------------|
| `GET /health` | 31500 | try/catch | 500 + error message |
| `POST /api/auth/register` | 31521 | try/catch | 500 "Registration failed" |
| `POST /api/auth/login` | 31574 | try/catch | 500 "Login failed" |
| `GET /api/auth/verify` | 31637 | try/catch | 500 "Verification failed" |
| `GET /api/servers` | 31670 | sync, no DB | No async to fail |
| `GET /api/characters` | 31688 | try/catch | 500 "Failed to fetch" |
| `POST /api/characters` | 31729 | try/catch | 500 "Failed to create" |
| `GET /api/characters/:id` | 31804 | try/catch | 500 "Failed to fetch" |
| `DELETE /api/characters/:id` | 31832 | try/catch | 500 "Failed to delete" |
| `PUT /api/characters/:id/position` | 31885 | try/catch | 500 "Failed to save" |
| `GET /api/test` | 31926 | sync | No async to fail |

**Verdict**: REST routes are well-protected. No hanging responses. Error messages are generic (no stack traces leaked).

### 1B. Socket Event Handlers — MIXED Coverage

**Fully protected (try/catch wrapping entire handler body):**

| Handler | Line | Catch Action |
|---------|------|-------------|
| `player:join` | 5308 | socket.emit('player:join_error') + return |
| `kafra:save` | 6571 | logger.error |
| `kafra:teleport` | 6618 | logger.error |
| `cart:move_to_cart` | 6790 | logger.error |
| `cart:move_to_inventory` | 6878 | logger.error |
| `identify:select` | 6941 | logger.error |
| `vending:start` | 6997 | logger.error |
| `vending:browse` | 7058 | logger.error |
| `vending:buy` | 7119 | logger.error |
| `combat:respawn` | 7740 | logger.warn (partial, only save-point query) |
| `player:allocate_stat` | 7971 | socket.emit('combat:error') |
| `job:change` | 8059 | logger.error |
| `skill:data` | 8125 | logger.error |
| `skill:learn` | 8207 | logger.error |
| `skill:reset` | 8295 | logger.error |
| `party:create` | 8379 | logger.error |
| `party:invite_respond` | 8493 | logger.error |
| `party:leave` | 8527 | logger.error |
| `party:kick` | 8580 | logger.error |
| `party:change_leader` | 8622 | logger.error |
| `party:change_exp_share` | 8649 | logger.error |
| `hotbar:save` | 8971 | logger.error |
| `hotbar:request` | 9013 | logger.error |
| `hotbar:save_skill` | 9051 | logger.error |
| `hotbar:clear` | 9106 | logger.error |
| `pharmacy:craft` | 21154 | logger.error |
| `crafting:craft_converter` | 21260 | logger.error |
| `homunculus:evolve` | 21905 | logger.error |
| `pet:tame` | 21947 | logger.error |
| `pet:incubate` | 22029 | logger.error |
| `pet:return_to_egg` | 22087 | logger.error |
| `pet:feed` | 22111 | logger.error |
| `pet:rename` | 22173 | logger.error |
| `pet:list` | 22191 | logger.error |
| `inventory:use` | 22238 | logger.error |
| `inventory:equip` | 22735 | logger.error |
| `equipment:repair` | 23293 | logger.error |
| `card:compound` | 23336 | logger.error |
| `inventory:drop` | 23481 | logger.error |
| `warp_portal:confirm` | 23586 | logger.error |
| `inventory:move` | 23641 | logger.error |
| `inventory:merge` | 23767 | logger.error |
| `shop:open` | 23828 | logger.error + ROLLBACK |
| `shop:buy` | 23875 | logger.error + ROLLBACK |
| `shop:sell` | 23903 | logger.error + ROLLBACK |
| `shop:buy_batch` | 24015 | logger.error + ROLLBACK |
| `shop:sell_batch` | 24166 | logger.error + ROLLBACK |
| `refine:request` | 24395 | logger.error |
| `forge:request` | 24459 | logger.error + ROLLBACK |
| `debug:apply_status` | 24574 | logger.error |
| `debug:remove_status` | 24619 | logger.error |
| `debug:list_statuses` | 24643 | logger.error |

**UNPROTECTED (no try/catch — could crash server on unexpected errors):**

| Handler | Line | Risk | What Could Fail |
|---------|------|------|-----------------|
| `player:position` | 5974 | **HIGH** | Redis `setPlayerPosition()` + warp portal zone transition DB queries (lines 6157, 6221) |
| `zone:warp` | 6288 | **HIGH** | `pool.query` at line 6381 (zone position save) |
| `zone:ready` | 6416 | **MEDIUM** | No DB calls, but iterates maps that could have unexpected state |
| `kafra:open` | 6515 | **LOW** | No DB calls, but `pool.query` at 6537 is wrapped |
| `cart:load` | 6710 | **MEDIUM** | `pool.query` at 6712 (not visible but implied) |
| `cart:rent` | 6719 | **MEDIUM** | `pool.query` at 6740 has its own try/catch but outer handler does not |
| `cart:remove` | 6760 | **LOW** | Inline try/catch on the one DB call |
| `player:sit` | 7443 | **LOW** | Sync only, no DB/async |
| `player:stand` | 7460 | **LOW** | Sync only, no DB/async |
| `combat:attack` | 7476 | **LOW** | Sync only, sets state in memory |
| `combat:stop_attack` | 7664 | **LOW** | Sync only |
| `player:request_stats` | 7826 | **LOW** | Sync only, computes and emits |
| `buff:request` | 7850 | **LOW** | Sync only |
| `mount:toggle` | 7916 | **LOW** | Delegates to `handleMountToggle` |
| `party:load` | 8350 | **LOW** | Sync only |
| `party:invite` | 8425 | **LOW** | Sync only |
| `party:chat` | 8667 | **LOW** | Sync only |
| `chat:message` | 8682 | **MEDIUM** | Mostly sync but has complex string parsing that could NPE |
| `inventory:load` | 8933 | **HIGH** | Two unprotected `await` calls: `getPlayerInventory()` + `getPlayerHotbar()` |
| `skill:use` | 9122 | **CRITICAL** | Massive handler (~12,000 lines), has internal try/catch for DB queries but no top-level wrap. A TypeError in any skill execution path crashes the server |
| `summon:detonate` | 21305 | **MEDIUM** | Async, no try/catch |
| `homunculus:feed` | 21364 | **MEDIUM** | Async, no try/catch |
| `homunculus:command` | 21410 | **LOW** | Sync only |
| `homunculus:skill_up` | 21430 | **MEDIUM** | Async, no try/catch |
| `homunculus:use_skill` | 21462 | **HIGH** | Raw `JSON.parse(data)` with no try/catch at line 21463 — if data is malformed string, server crashes |
| `disconnect` | 7236 | **HIGH** | Multiple `await pool.query` calls, some wrapped in try/catch individually but the overall handler is not wrapped — any exception before the first try/catch will crash |

### 1C. Utility Functions — MOSTLY PROTECTED

| Function | Line | Protected | Notes |
|----------|------|-----------|-------|
| `saveExpDataToDB` | 4059 | Yes | try/catch, logs error |
| `savePlayerHealthToDB` | 4520 | Yes | try/catch, logs error |
| `loadItemDefinitions` | 4839 | Yes | try/catch, logs error |
| `getPlayerInventory` | 4981 | Yes | try/catch, returns [] |
| `getPlayerHotbar` | 5040 | Yes | try/catch, returns [] |
| `addItemToInventory` | 4908 | Yes | try/catch, returns null |
| `removeItemFromInventory` | 5065 | Yes | try/catch, returns false |
| `calculatePlayerCurrentWeight` | 4786 | Yes | try/catch, returns 0 |
| `consumeAmmo` | 3364 | **No** | Two `await pool.query` calls with no try/catch |
| `consumeSkillCatalysts` | 201 | **No** | Multiple `await pool.query` calls with no try/catch |
| `evaluateConditionalCardBonuses` | 2962 | **No** | `await pool.query` at line 2964 with no try/catch |

---

## Pass 2: Unprotected Async Operations

### 2A. Database Queries Without Error Handling

The following `pool.query` calls are NOT inside try/catch and do NOT have `.catch()`:

| Line | Context | Query |
|------|---------|-------|
| 1921-1925 | `executeCastComplete` inner skill handler | `UPDATE character_inventory SET quantity = quantity - ...` (catalyst consumption) |
| 2964-2970 | `evaluateConditionalCardBonuses` | `SELECT ci.item_id ... FROM character_inventory ci` (refine lookup) |
| 3368 | `consumeAmmo` | `DELETE FROM character_inventory WHERE inventory_id = $1` |
| 3378 | `consumeAmmo` | `UPDATE character_inventory SET quantity = quantity - ...` |
| 6157 | `player:position` handler | `setPlayerPosition` (Redis call, not DB but still async) |
| 6221 | `player:position` warp collision | `UPDATE characters SET zone_name = ...` (inside its own try/catch) |
| 8938 | `inventory:load` | `getPlayerInventory` (has internal try/catch but caller doesn't) |
| 8943 | `inventory:load` | `getPlayerHotbar` (has internal try/catch but caller doesn't) |

### 2B. JSON.parse Without Try/Catch

| Line | Context | Input Source | Risk |
|------|---------|-------------|------|
| 21463 | `homunculus:use_skill` | `typeof data === 'string' ? JSON.parse(data) : data` | **HIGH** — malformed string crashes server |
| 24575 | `debug:apply_status` | Same pattern — but wrapped in try/catch at 24574 | Safe |
| 24620 | `debug:remove_status` | Same pattern — wrapped in try/catch at 24619 | Safe |
| 24644 | `debug:list_statuses` | Same pattern — wrapped in try/catch at 24643 | Safe |
| 31942 | `getPlayerPosition` | `JSON.parse(data)` on Redis data | **MEDIUM** — corrupted Redis data could crash |
| 31957 | `getPlayersInZone` | `JSON.parse(data)` on Redis data | **MEDIUM** — same issue |

### 2C. Redis Operations Without Error Handling

All Redis helper functions (`setPlayerPosition`, `getPlayerPosition`, `removePlayerPosition`, `getPlayersInZone`) at lines 31932-31964 perform Redis operations with **no try/catch**. If Redis is down:
- `setPlayerPosition` will throw on every player movement (called from `player:position` handler)
- `getPlayerPosition` will throw on range checks in combat tick
- These are called from unprotected contexts, so they bubble up to the unprotected socket handler or setInterval

---

## Pass 3: Socket Handler Crash Analysis

### How socket.on handlers work with async

When a `socket.on` handler is `async` and throws an unhandled rejection:
1. In Node.js < 15: Unhandled promise rejection warning (does not crash)
2. In Node.js >= 15: **Crashes the process** (default behavior)
3. With `--unhandled-rejections=throw` (or Node >= 15 default): **Server crash**

### Critical unprotected async handlers

These handlers are `async` but have no top-level try/catch. An unhandled DB error, Redis error, or TypeError in any of these will produce an unhandled promise rejection:

1. **`player:position`** (line 5974) — Called on **every frame** (~30Hz per player). Contains `await setPlayerPosition()` (Redis) and warp portal zone transition logic with `await pool.query`. Highest traffic handler on the server.

2. **`skill:use`** (line 9122) — The largest handler in the file (~12,000 lines of skill execution). Has internal try/catch blocks for specific DB queries but no top-level wrapper. A TypeError in any of the ~170 skill handler branches will crash the server.

3. **`inventory:load`** (line 8933) — Two `await` calls with no try/catch.

4. **`disconnect`** (line 7236) — Multiple `await pool.query` calls, some individually wrapped but the handler itself is not wrapped. If the first `await savePlayerHealthToDB` fails before the individual try/catch blocks, it throws.

5. **`homunculus:use_skill`** (line 21462) — Raw `JSON.parse(data)` on line 21463 with no try/catch.

### Sync handlers (lower risk)

Handlers like `player:sit`, `player:stand`, `combat:attack`, `combat:stop_attack`, `party:invite`, `party:chat` are synchronous. They can still crash if they throw (e.g., accessing property of undefined), but the crash is immediate and would be caught by a `process.on('uncaughtException')` handler — **which does not exist**.

---

## Pass 4: Swallowed Errors

### 4A. Silent `.catch(() => {})` — Errors Completely Discarded

| Line | Context | What's Silenced |
|------|---------|-----------------|
| 2123 | Death: pet runs away | `pool.query('DELETE FROM character_pets ...').catch(() => {})` — Pet deletion failure silently ignored |
| 2132 | Death: pet intimacy loss | `pool.query('UPDATE character_pets ...').catch(() => {})` — Intimacy save failure silently ignored |
| 23852 | `shop:buy` ROLLBACK | `client.query('ROLLBACK').catch(() => {})` — Acceptable: ROLLBACK after error |
| 23911 | `shop:sell` ROLLBACK | Same — acceptable |
| 24079 | `shop:buy_batch` ROLLBACK | Same — acceptable |
| 24195 | `shop:sell_batch` ROLLBACK | Same — acceptable |
| 24553 | `forge:request` ROLLBACK | Same — acceptable |

**Verdict**: The ROLLBACK `.catch(() => {})` calls are acceptable (the transaction is already in an error state). But lines 2123 and 2132 silently discard pet DB errors — if the DELETE/UPDATE fails, the pet state becomes inconsistent between memory and DB with no log entry.

### 4B. catch blocks that log but don't propagate or respond to client

Many catch blocks only `logger.error()` without emitting an error back to the client. This means the client's request silently fails with no feedback:

| Line | Handler | Missing Client Response |
|------|---------|------------------------|
| 5847 | `player:join` (inner block) | Several nested catch blocks that log but don't emit error back |
| 6539 | `kafra:open` save query | Catches error, uses default — acceptable |
| 6674 | `kafra:teleport` zeny check | Logs error, no client response |
| 7977 | `player:allocate_stat` | Catch emits `combat:error` — good |
| 8064 | `job:change` | Logs only, no `job:error` emit |
| 8133 | `skill:data` | Logs only, no `skill:error` emit |

---

## Pass 5: Error Response Quality

### 5A. REST API Error Responses — GOOD

All REST routes:
- Always send an HTTP status code + JSON error body
- Use generic error messages ("Registration failed", "Login failed") — no stack traces
- No internal paths leaked

One minor issue: The `/health` endpoint at line 31514 leaks `err.message` in the response:
```js
res.status(500).json({ status: 'ERROR', message: 'Database connection failed', error: err.message });
```
This could expose internal PostgreSQL error details to callers.

### 5B. Socket Error Responses — INCONSISTENT

**Good pattern** (used by many handlers):
```js
socket.emit('skill:error', { message: 'Skill not learned' });
```

**Missing error responses** — these handlers catch errors but don't notify the client:

| Handler | Line | Catch Action | Missing |
|---------|------|-------------|---------|
| `kafra:save` | 6578 | `logger.error` only | No `kafra:error` emit |
| `kafra:teleport` | 6674 | `logger.error` only | No `kafra:error` emit |
| `job:change` | 8064 | `logger.error` only | No `job:error` emit |
| `skill:data` | 8133 | `logger.error` only | No `skill:error` emit |
| `skill:learn` | 8215 | `logger.error` only | No `skill:error` emit |
| `skill:reset` | 8281 | `logger.error` only | No `skill:error` emit |
| `hotbar:save` | 9004 | `logger.error` only | No `hotbar:error` emit |
| `pharmacy:craft` | 21226 | `logger.error` only | No `pharmacy:result` failure emit |
| All cart handlers | 6862-6925 | `logger.error` only | No `cart:error` emit |

### 5C. Error Message Safety — MOSTLY SAFE

Error messages sent to the client are generally safe generic strings. However, a few places pass `err.message` directly:

| Line | Pattern | Risk |
|------|---------|------|
| 31514 | `error: err.message` in /health response | Leaks DB error details |
| Various skill handlers | `socket.emit('skill:error', { message: 'Internal error' })` | Safe — generic |

---

## Pass 6: Process-Level Error Handling

### 6A. Missing Process Error Handlers — CRITICAL

**There is NO `process.on('uncaughtException')` handler.**
**There is NO `process.on('unhandledRejection')` handler.**

This means:
- Any uncaught throw in a sync context crashes the server immediately
- Any unhandled promise rejection in Node >= 15 crashes the server immediately
- There is no graceful shutdown, no error logging of the crash cause, no cleanup

### 6B. Database Connection Drop Handling

**There is NO `pool.on('error')` handler.**

The `pg` Pool emits `'error'` when an idle client encounters an error. Without this handler, a database connection drop produces an unhandled error event that **crashes the process** (unhandled `'error'` events in Node.js are fatal).

The Pool is configured at line 31427:
```js
const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
});
```
No `max`, `idleTimeoutMillis`, `connectionTimeoutMillis`, or error handler configured.

### 6C. Redis Disconnect Handling — PARTIAL

Redis has event handlers at lines 31441-31447:
```js
redisClient.on('error', (err) => { logger.error('Redis error:', err); });
redisClient.on('connect', () => { logger.info('Connected to Redis'); });
```

The `'error'` handler prevents crashes from Redis errors on the client itself. However:
- Redis helper functions (`setPlayerPosition`, etc.) have no try/catch
- If Redis reconnects but throws during a command, the calling code will get an unhandled rejection
- No `'reconnecting'` or `'end'` event handlers
- `redisClient.connect()` at line 31450 has no `.catch()` — if initial connection fails, it's an unhandled rejection

### 6D. No Graceful Shutdown

There is no `SIGTERM` or `SIGINT` handler. When the process receives a termination signal:
- Database connections are not cleanly closed
- Connected player data is not saved
- Redis connections are not closed

---

## Pass 7: Resource Cleanup in Error Paths

### 7A. Database Pool Connections — MOSTLY GOOD

Transaction-based handlers (`shop:buy`, `shop:sell`, `shop:buy_batch`, `shop:sell_batch`, `forge:request`) properly use `pool.connect()` → `try/finally { client.release() }` pattern:

| Handler | Line | Pattern | Cleanup |
|---------|------|---------|---------|
| `shop:buy` | 23827 | `pool.connect()` + finally | `client.release()` in finally |
| `shop:sell` | 23902 | `pool.connect()` + catch+release | `client.release()` in catch + after try |
| `shop:buy_batch` | 24053 | `pool.connect()` + finally | `client.release()` in finally |
| `shop:sell_batch` | 24165 | `pool.connect()` + finally | `client.release()` in finally |
| `forge:request` | 24458 | `pool.connect()` + finally | `client.release()` in finally |

**Issue with `shop:sell`** (line 23902-23917): The single-item sell handler uses `pool.connect()` but has `client.release()` in the catch block AND after the try/catch. If the catch block is reached, `client.release()` is called twice. This could cause issues. Should use `finally`.

### 7B. Intervals — NEVER Cleaned

There are **17 `setInterval` calls** in the file. None of them:
- Store the interval ID
- Have any mechanism to be cleared on shutdown
- Are wrapped in try/catch

If a setInterval callback throws, the interval continues running (for sync callbacks) but if it's async and rejects, it's an unhandled rejection.

List of all setInterval loops:

| Line | Interval | Purpose | Protected |
|------|----------|---------|-----------|
| 153 | 60s | Party invite cleanup | No |
| 5266 | 1s | Socket rate limit counter reset | No |
| 24778 | 50ms (COMBAT_TICK_MS) | **Combat tick loop** | **No** — most critical loop |
| 26551 | 1s | Party HP sync | No |
| 26577 | 6s | HP natural regen | No |
| 26637 | 8s | SP natural regen | No |
| 26691 | 10s | Skill HP/SP regen (Inc HP/SP Recovery) | No |
| 26743 | 10s | Sitting bonus regen | No |
| 26775 | 5s | Card periodic effects | No |
| 26814 | 1s | **Status/buff tick** (poison drain, expiry) | **No** — touches many player objects |
| 27531 | 60s | Homunculus hunger tick | No |
| 27580 | 10s | Homunculus HP/SP regen | No |
| 27621 | 10s | Pet hunger tick | No |
| 27684 | 250ms | **Ground effects tick** (Fire Wall, Storm Gust) | **No** — complex damage calcs |
| 30107 | 200ms | **Enemy AI loop** | **No** — the second most critical loop |
| 31176 | 200ms | Trap tick loop | No |
| 32550 | 60s | Periodic DB save (health/EXP/zone) | Partially (inner try/catch for zone save) |

### 7C. Socket Room Cleanup — ADEQUATE

The disconnect handler (line 7236) properly:
- Removes player from `connectedPlayers` Map
- Clears `autoAttackState` entries
- Cleans up enemy targeting
- Broadcasts `player:left`
- Saves data to DB

However, it does NOT:
- Clear `activeCasts` timeout references (it deletes the entry but not any pending timers)
- Leave socket rooms explicitly (Socket.io handles this automatically on disconnect)

---

## Recommendations

### P0 — Critical (Server crash prevention)

1. **Add process-level error handlers**:
```js
process.on('uncaughtException', (err) => {
    logger.error('[FATAL] Uncaught exception:', err.message, err.stack);
    // Optionally: save all player data, then exit
    process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
    logger.error('[FATAL] Unhandled rejection:', reason);
    // Log but don't crash — most unhandled rejections are recoverable
});
```

2. **Add pool error handler**:
```js
pool.on('error', (err) => {
    logger.error('[DB] Idle client error:', err.message);
    // Do NOT crash — pg Pool will remove the failed client and create new ones
});
```

3. **Wrap ALL async socket handlers in try/catch**. At minimum, these critical handlers:
   - `player:position` (line 5974) — highest traffic
   - `skill:use` (line 9122) — largest and most complex
   - `inventory:load` (line 8933)
   - `disconnect` (line 7236)
   - `homunculus:use_skill` (line 21462) — has raw JSON.parse

   Recommended pattern:
   ```js
   socket.on('some:event', async (data) => {
       try {
           // ... handler body ...
       } catch (err) {
           logger.error(`[EVENT] some:event error: ${err.message}`);
           socket.emit('some:error', { message: 'Internal error' });
       }
   });
   ```

4. **Wrap all setInterval async callbacks in try/catch**, especially:
   - Combat tick (line 24778)
   - Enemy AI loop (line 30107)
   - Ground effects tick (line 27684)
   - Status/buff tick (line 26814)

   Pattern:
   ```js
   setInterval(async () => {
       try {
           // ... tick body ...
       } catch (err) {
           logger.error(`[TICK] Combat tick error: ${err.message}`);
       }
   }, COMBAT_TICK_MS);
   ```

### P1 — High (Data integrity)

5. **Add try/catch to `consumeAmmo`** (line 3364) — DB queries at lines 3368 and 3378 can reject.

6. **Add try/catch to `consumeSkillCatalysts`** (line 201) — multiple DB queries that can reject.

7. **Add try/catch to `evaluateConditionalCardBonuses`** (line 2962) — DB query at line 2964.

8. **Add try/catch to Redis helper functions** (lines 31932-31964):
   ```js
   async function setPlayerPosition(characterId, x, y, z, zone = 'default') {
       try {
           const key = `player:${characterId}:position`;
           await redisClient.setEx(key, 300, JSON.stringify({ x, y, z, zone, timestamp: Date.now() }));
       } catch (err) {
           logger.warn(`[REDIS] Failed to cache position for ${characterId}: ${err.message}`);
       }
   }
   ```

9. **Replace silent `.catch(() => {})` on pet queries** (lines 2123, 2132) with `.catch(err => logger.warn(...))`.

### P2 — Medium (Client experience)

10. **Add client error emit to all catch blocks** that currently only log. At minimum: `kafra:save`, `kafra:teleport`, `job:change`, `skill:data`, `skill:learn`, `skill:reset`, all cart handlers.

11. **Remove `err.message` from /health endpoint response** (line 31514) — replace with a generic message.

12. **Handle `redisClient.connect()` rejection** (line 31450):
    ```js
    redisClient.connect().catch(err => {
        logger.error('Redis: Initial connection failed -', err.message);
    });
    ```

### P3 — Low (Best practices)

13. **Add graceful shutdown handler**:
```js
async function gracefulShutdown(signal) {
    logger.info(`[SHUTDOWN] Received ${signal}, saving all player data...`);
    for (const [charId, player] of connectedPlayers.entries()) {
        await savePlayerHealthToDB(charId, player.health, player.mana);
        await saveExpDataToDB(charId, player);
    }
    await pool.end();
    await redisClient.quit();
    process.exit(0);
}
process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));
process.on('SIGINT', () => gracefulShutdown('SIGINT'));
```

14. **Configure Pool connection limits**:
```js
const pool = new Pool({
    ...config,
    max: 20,
    idleTimeoutMillis: 30000,
    connectionTimeoutMillis: 5000,
});
```

15. **Store interval IDs** for cleanup:
```js
const intervals = [];
intervals.push(setInterval(...));
// On shutdown: intervals.forEach(clearInterval);
```

16. **Fix `shop:sell` double-release** (line 23902-23917) — use `finally { client.release(); }` pattern instead of releasing in both catch and after try.

---

## Statistics Summary

| Category | Count | Status |
|----------|-------|--------|
| REST routes | 11 | All protected |
| Socket handlers (total) | 72 | ~52 protected, ~20 unprotected |
| Socket handlers (async, unprotected) | 8 | **Needs fix** |
| Try/catch blocks | ~155 | Good coverage where present |
| setInterval loops | 17 | **0 protected** |
| Fire-and-forget DB calls | 7 | 5 have .catch(), 2 silent |
| JSON.parse without try/catch | 3 | **Needs fix** |
| Process error handlers | 0 | **Critical gap** |
| Pool error handler | 0 | **Critical gap** |
| Redis function error handling | 0 | **Needs fix** |
| Swallowed errors (.catch(() => {})) | 7 | 5 acceptable (ROLLBACK), 2 need logging |
| DB client leak potential | 1 | shop:sell double-release |
