# Security, Authentication & Input Validation Audit

**Auditor**: Claude Opus 4.6 (automated, 7-pass)
**Date**: 2026-03-23
**Scope**: `server/src/index.js` (~31,930 lines), `server/src/test_mode.js`, REST API, Socket.io events
**Methodology**: Static analysis of JWT implementation, all REST routes, all 85+ socket event handlers, rate limiting, input validation, privilege escalation vectors, and information leakage

---

## Executive Summary

The Sabri_MMO server has a **solid security foundation**: server-authoritative damage/stats, JWT authentication on REST + Socket.io, parameterized SQL queries, bcrypt password hashing, and socket event rate limiting. However, several gaps exist across authentication, input validation, and anti-cheat enforcement that could be exploited by a modified client.

**Finding distribution**: 3 CRITICAL, 7 HIGH, 11 MEDIUM, 8 LOW, 5 INFORMATIONAL

---

## Pass 1: JWT Authentication

### Token Creation (Lines 31550, 31612)
- **Algorithm**: Default HS256 (HMAC-SHA256) -- adequate
- **Payload**: `{ user_id, username }` -- minimal, good
- **Expiry**: `24h` -- acceptable for a game
- **Secret**: `process.env.JWT_SECRET` from `.env` -- not hardcoded, good

### Token Validation Middleware (Line 31481)
- Extracts Bearer token from `Authorization` header
- Calls `jwt.verify()` with the same secret
- Sets `req.user` on success
- Returns 401/403 on failure

### Socket.io Authentication (Line 5295 `player:join`)
- Strips `Bearer ` prefix from token
- Calls `jwt.verify()` on the raw token
- Verifies character ownership via DB query (`character_id + user_id + deleted = FALSE`)
- Rejects with `player:join_error` on failure

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| AUTH-1 | **MEDIUM** | No JWT `algorithm` whitelist in `jwt.verify()` | Lines 5309, 31489 |
| AUTH-2 | **LOW** | JWT secret strength is unauditable (runtime `.env` value) | `.env` |
| AUTH-3 | **MEDIUM** | No token revocation mechanism (logout = client-side only) | Architecture |

**AUTH-1**: Without specifying `{ algorithms: ['HS256'] }` in `jwt.verify()`, a theoretical "none" algorithm attack exists if the jsonwebtoken library has a vulnerability. Modern versions of `jsonwebtoken` mitigate this, but explicit algorithm whitelisting is defense-in-depth.

**Recommendation**:
```javascript
jwt.verify(token, process.env.JWT_SECRET, { algorithms: ['HS256'] }, (err, user) => { ... });
```

**AUTH-3**: If a JWT is compromised (XSS, network sniff), it remains valid for 24 hours with no server-side way to invalidate it. A Redis-backed token blacklist on logout would close this gap.

---

## Pass 2: REST API Route Authentication

| Route | Method | Auth? | Validated? | Finding |
|-------|--------|-------|------------|---------|
| `/health` | GET | No | N/A | OK -- health checks should be public |
| `/api/auth/register` | POST | No | `validateRegisterInput` | OK |
| `/api/auth/login` | POST | No | Basic (null check) | OK |
| `/api/auth/verify` | GET | `authenticateToken` | Via JWT | OK |
| `/api/servers` | GET | No | N/A | OK -- server list is public |
| `/api/characters` | GET | `authenticateToken` | user_id filter | OK |
| `/api/characters` | POST | `authenticateToken` | Name/class/hair validated | OK |
| `/api/characters/:id` | GET | `authenticateToken` | user_id ownership | OK |
| `/api/characters/:id` | DELETE | `authenticateToken` | Password re-confirm + user_id | OK -- strong |
| `/api/characters/:id/position` | PUT | `authenticateToken` | Type check + user_id | See REST-1 |
| `/api/test` | GET | No | N/A | See REST-2 |
| `/test/*` | ALL | No | N/A | See REST-3 |

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| REST-1 | **MEDIUM** | Position save has no coordinate bounds checking | Line 31891 |
| REST-2 | **LOW** | `/api/test` endpoint exposed in all environments | Line 31926 |
| REST-3 | **HIGH** | `/test/*` routes (test_mode.js) have no auth and no environment gate | Line 31418 |

**REST-1**: The position save validates `typeof x === 'number'` but does not check for `NaN`, `Infinity`, or world bounds. A malicious client could save `x: Infinity` or `x: 999999999`.

**Recommendation**:
```javascript
if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(z)) {
    return res.status(400).json({ error: 'Coordinates must be finite numbers' });
}
// Optional: world bounds check
const MAX_COORD = 1000000;
if (Math.abs(x) > MAX_COORD || Math.abs(y) > MAX_COORD || Math.abs(z) > MAX_COORD) {
    return res.status(400).json({ error: 'Coordinates out of bounds' });
}
```

**REST-3 (HIGH)**: `test_mode.js` is mounted at `/test` (line 31418) with NO authentication and NO `NODE_ENV` check. This exposes mock data endpoints and a `/test/socket-event` POST that simulates server responses. While currently returning only hardcoded mock data (not real DB data), the `/test/reset` endpoint and the pattern of accepting arbitrary event names are attack surface.

**Recommendation**: Gate behind `NODE_ENV === 'development'`:
```javascript
if (process.env.NODE_ENV === 'development') {
    app.use('/test', testModeRouter);
}
```

---

## Pass 3: Socket.io Authentication

### Authentication Model
The server uses a **two-phase** authentication model:
1. `player:join` validates the JWT and character ownership
2. All subsequent events use `findPlayerBySocketId(socket.id)` to identify the player

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| SOCK-1 | **CRITICAL** | Socket events before `player:join` are not blocked | All handlers |
| SOCK-2 | **MEDIUM** | No re-authentication on long-lived sockets (24h+ token expiry vs persistent socket) | Architecture |

**SOCK-1 (CRITICAL)**: After a socket connects (`io.on('connection')`), ALL event handlers are registered immediately. A client can send `combat:attack`, `inventory:use`, `skill:use`, etc. **without ever sending `player:join`**. These handlers call `findPlayerBySocketId()` which returns `null`, and most handlers return early with `if (!playerInfo) return;`. This is safe in isolation -- the attack does nothing. **However**, there is a timing window: if a socket connects, sends events rapidly, and another player disconnects at the same time causing socket ID reuse (unlikely but possible in Socket.io), or if any handler has a code path that does NOT check `findPlayerBySocketId` first, it could lead to unauthorized actions.

More practically, the concern is that `findPlayerBySocketId` iterates all connected players on EVERY socket event (O(n) per event). A malicious client that never joins but floods events wastes server CPU.

**Recommendation**: Track authenticated sockets. Reject all events except `player:join` from unauthenticated sockets:
```javascript
const authenticatedSockets = new Set();

socket.use(([eventName, ...args], next) => {
    if (eventName === 'player:join') return next();
    if (!authenticatedSockets.has(socket.id)) return; // Drop silently
    if (throttleSocketEvent(socket.id, eventName)) return next();
});

// In player:join, after successful JWT verification:
authenticatedSockets.add(socket.id);

// In disconnect:
authenticatedSockets.delete(socket.id);
```

**SOCK-2**: The persistent socket survives zone transitions and stays connected for the entire play session. The JWT that authenticated `player:join` might expire (24h) while the socket remains connected. The server never re-validates the token on an open socket.

**Recommendation**: Store `tokenExpiry` on the player object and periodically check it (e.g., every 5 minutes in the tick loop). Force disconnect on expiry.

---

## Pass 4: Socket Event Input Validation

### Methodology
Examined all 85+ `socket.on()` handlers for:
- ID validation (parseInt, NaN checks)
- String length limits
- Coordinate bounds
- Player impersonation prevention
- Client-trusted values

### Player Identification Pattern (GOOD)
All handlers use `findPlayerBySocketId(socket.id)` to derive the acting player. The client NEVER sends its own characterId for action authorization. This prevents player impersonation.

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| VAL-1 | **CRITICAL** | `player:position` has no coordinate bounds or speed validation | Line 5974 |
| VAL-2 | **HIGH** | `player:position` character ID comes from client `data.characterId` | Line 5976 |
| VAL-3 | **MEDIUM** | Chat messages are not HTML-sanitized (XSS in UE5 is unlikely but future web client possible) | Line 8702 |
| VAL-4 | **LOW** | `inventory:use` checks `item_type !== 'consumable'` but the `IsConsumable()` client function also allows `usable` type -- mismatch | Line 22254 |
| VAL-5 | **HIGH** | SQL column name interpolation in stat allocation | Line 7974 |
| VAL-6 | **MEDIUM** | `skill:use` accepts `data.skillLevel` from client on `_castComplete` | Line 24765 |
| VAL-7 | **LOW** | `party:chat` does not enforce message length limit | Line 8667 |
| VAL-8 | **MEDIUM** | Several socket events lack rate limiting (see Pass 6) | Various |

**VAL-1 (CRITICAL)**: The `player:position` handler at line 5974 accepts `x`, `y`, `z` from the client with zero validation:
- No `typeof` check (could be a string, object, or `NaN`)
- No `Number.isFinite()` check (could be `Infinity`)
- No world bounds check (client can set position to any coordinate)
- No speed validation (client can teleport to any point instantly)

The server stores this directly to Redis (line 6157) and broadcasts to all players (line 6273). The combat tick loop then uses this cached position for range checks (line 24952). A cheater could:
1. **Teleport anywhere** by sending arbitrary coordinates
2. **Speed hack** by sending positions faster than humanly possible (rate-limited to 60/s but that still allows 60 position jumps per second)
3. **Evade attacks** by warping out of range between combat ticks
4. **Attack from across the map** by warping into range, hitting, and warping back

**Recommendation** (multi-layered):
```javascript
// 1. Type validation
if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') return;
if (!Number.isFinite(x) || !Number.isFinite(y) || !Number.isFinite(z)) return;

// 2. World bounds
const MAX_COORD = 500000; // Adjust to actual map size
if (Math.abs(x) > MAX_COORD || Math.abs(y) > MAX_COORD || Math.abs(z) > MAX_COORD) return;

// 3. Speed check (distance / time since last position update)
const elapsed = Math.max(16, now - (player.lastPositionTime || now - 200));
const dist = Math.sqrt((x - player.lastX) ** 2 + (y - player.lastY) ** 2);
const speed = dist / (elapsed / 1000);
const maxSpeed = 800 * (player.isMounted ? 1.36 : 1.0); // UE units/sec, adjusted for mount
if (speed > maxSpeed * 2.0) { // 2x tolerance for lag
    logger.warn(`[SPEED] ${player.characterName} speed=${speed.toFixed(0)} (max=${maxSpeed})`);
    // Option A: Snap back
    socket.emit('player:position_rejected', { x: player.lastX, y: player.lastY, z: player.lastZ, reason: 'speed' });
    return;
}
```

**VAL-2 (HIGH)**: At line 5976, `const characterId = parseInt(data.characterId)`. The character ID is taken from the client payload, not derived from `findPlayerBySocketId()`. While the position is then looked up via `connectedPlayers.get(characterId)` (which would return the wrong player), the `broadcastToZoneExcept` at line 6273 uses this client-supplied characterId. A malicious client could send another player's characterId, causing position broadcasts with the victim's ID but the attacker's coordinates -- making the victim appear to teleport on other clients' screens.

**Recommendation**: Always derive characterId from the socket:
```javascript
const playerInfo = findPlayerBySocketId(socket.id);
if (!playerInfo) return;
const { characterId, player } = playerInfo;
```

**VAL-5 (HIGH)**: At line 7974, the stat allocation handler uses string interpolation for the SQL column name:
```javascript
const dbStatName = statName === 'int' ? 'int_stat' : statName;
await pool.query(`UPDATE characters SET ${dbStatName} = $1, ...`, [...]);
```
While `statName` IS validated against `['str', 'agi', 'vit', 'int', 'dex', 'luk']` at line 7932 (making this safe in practice), dynamic SQL column names are a pattern that should be avoided. If the whitelist is ever expanded incorrectly, this becomes a SQL injection vector.

**Recommendation**: Use a mapping object instead of interpolation:
```javascript
const STAT_COLUMNS = { str: 'str', agi: 'agi', vit: 'vit', int: 'int_stat', dex: 'dex', luk: 'luk' };
const dbCol = STAT_COLUMNS[statName];
if (!dbCol) return; // Already validated above, but defense-in-depth
await pool.query(`UPDATE characters SET ${dbCol} = $1, ...`, [...]);
```

**VAL-6**: When a cast completes (line 24765), the `skillLevel` is passed back through the handler from the original cast data. This is sourced from the player's `learnedSkills` at cast start time, not from the client -- so it is safe. However, the `_castComplete` flag itself is client-accessible: a client could send `{ skillId: 210, _castComplete: true }` to skip cast time entirely.

**Wait -- re-reading**: The `_castComplete` flag is set by `executeCastComplete()` which calls the handler internally via `handlers[0]({...})`. But there is nothing preventing a client from also including `_castComplete: true` in their `skill:use` payload. Need to verify if this is checked.

Let me verify: The `skill:use` handler checks `if (activeCasts.has(characterId))` at line 9260, which blocks a second skill while casting. But `_castComplete: true` causes the handler to skip the cast time and execute immediately. A malicious client sending `{ skillId: X, _castComplete: true, skillLevel: 10 }` could **bypass all cast times for any skill**.

**UPDATE - This is CRITICAL. Reclassifying.**

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| VAL-6 | **CRITICAL** | Client can send `_castComplete: true` in `skill:use` to bypass cast times | Line 9122+ |

**Recommendation**: Validate that `_castComplete` came from the server's internal call, not from the client:
```javascript
socket.on('skill:use', async (data) => {
    // SECURITY: _castComplete is an internal flag set by executeCastComplete()
    // Strip it from client-sent data to prevent cast time bypass
    if (data._castComplete && !data._internalCastComplete) {
        delete data._castComplete;
    }
    // ... rest of handler
});
```
Or better, use a separate internal Map to track completed casts rather than passing a flag through the event data:
```javascript
const completedCasts = new Map(); // characterId -> { skillId, targetId, ... }

async function executeCastComplete(characterId, cast) {
    completedCasts.set(characterId, cast);
    // ... trigger skill:use
}

socket.on('skill:use', async (data) => {
    const isCastComplete = completedCasts.has(characterId);
    if (isCastComplete) completedCasts.delete(characterId);
    // Use isCastComplete instead of data._castComplete
});
```

---

## Pass 5: Privilege Escalation

### Admin Endpoints
There are **no admin endpoints** in the codebase. No role-based access control exists. This means:
- No in-game admin commands (GM commands)
- No way to ban players via API
- No server-side item granting

This is acceptable for development but will need attention before production.

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| PRIV-1 | **HIGH** | Debug handlers only gated by `NODE_ENV`, not by user role | Lines 24572-24659 |
| PRIV-2 | **MEDIUM** | No world-bounds validation on positions allows arbitrary teleportation | Line 5974 |
| PRIV-3 | **LOW** | `skill:reset` is free (no zeny cost) -- economy exploit potential | Line 8288 |
| PRIV-4 | **MEDIUM** | No speed/position validation allows effective speed hacking | Line 5974 |

**PRIV-1 (HIGH)**: The `debug:apply_status`, `debug:remove_status`, and `debug:list_statuses` handlers are gated by `process.env.NODE_ENV !== 'development'`. If the server is accidentally started without `NODE_ENV=production` (the default for Node.js is `undefined`, not `'development'`), these handlers silently drop events. However, if someone sets `NODE_ENV=development` on a production server (or forgets to set it), ANY connected client can:
- Apply any status effect (stun, freeze, stone) to any player or enemy
- Remove any status effect from any entity
- List all active buffs/debuffs on any entity

**Recommendation**: Remove debug handlers entirely from production builds, or add a secondary check (admin role, secret debug key, or IP whitelist).

### What Players CANNOT Do (Server-Authoritative Protections)
The server correctly prevents:
- **Damage manipulation**: All damage is calculated server-side. Client only sends target IDs.
- **Stat manipulation**: Stats are loaded from DB; allocation deducts points server-side.
- **Item duplication**: Inventory operations use DB transactions with ownership checks.
- **Zeny manipulation**: All zeny changes go through server-side deduction/addition.
- **Free items from shops**: Prices are looked up server-side from `NPC_SHOPS` + `itemDefinitions`.
- **Equipping other players' items**: All inventory queries filter by `character_id`.
- **Attacking as another player**: `findPlayerBySocketId()` derives attacker identity from socket.
- **Killing unkillable enemies**: Enemy HP is tracked server-side only.
- **Cross-zone targeting**: Combat tick checks zone matching.
- **Infinite skill usage**: SP deduction and cooldowns are server-side.

---

## Pass 6: Rate Limiting

### REST API Rate Limiting (GOOD)
```javascript
const limiter = rateLimit({ windowMs: 15 * 60 * 1000, max: 100 });
app.use('/api/', limiter);
```
All `/api/` routes are rate-limited to 100 requests per 15 minutes per IP.

### Socket Event Rate Limiting (GOOD, with gaps)
37 event types have explicit per-second limits (lines 5226-5262). Events NOT in the `SOCKET_RATE_LIMITS` map are **unlimited** (line 5270: `if (!limit) return true`).

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| RATE-1 | **MEDIUM** | No auth-specific rate limit (login brute-force at 100/15min) | Line 31410 |
| RATE-2 | **HIGH** | 23+ socket events have NO rate limiting | Line 5270 |
| RATE-3 | **LOW** | No connection rate limit on Socket.io (rapid connect/disconnect DoS) | Line 5283 |

**RATE-1**: The login endpoint shares the global 100/15min limiter. A brute-force attacker gets 100 login attempts per 15 minutes per IP. Should be stricter (10-20 per 15 min).

**Recommendation**:
```javascript
const authLimiter = rateLimit({ windowMs: 15 * 60 * 1000, max: 15, message: 'Too many login attempts' });
app.post('/api/auth/login', authLimiter, async (req, res) => { ... });
app.post('/api/auth/register', authLimiter, async (req, res) => { ... });
```

**RATE-2 (HIGH)**: The following socket events have NO rate limit and can be spammed at full network speed:

| Unrated Event | Risk |
|--------------|------|
| `player:join` | Re-authentication spam, DB queries |
| `player:request_stats` | DB-heavy stat recalculation |
| `buff:request` | Memory iteration per call |
| `mount:toggle` | State toggling |
| `skill:data` | Full skill tree DB query |
| `skill:use` | Has internal cooldown but no rate limit |
| `skill:learn` | DB write per call |
| `skill:reset` | DB delete + rewrite |
| `job:change` | DB write per call |
| `zone:warp` | Zone transition (heavy) |
| `zone:ready` | Zone data load |
| `kafra:open/save/teleport` | DB queries |
| `combat:respawn` | State reset + DB write |
| `inventory:load` | Full inventory DB query |
| `inventory:merge` | DB transactions |
| `shop:open` | Item definition lookup |
| `refine:request` | DB transaction + RNG |
| `forge:request` | DB transaction |
| `pharmacy:craft` | DB transaction |
| `debug:*` | Status effect manipulation |
| `homunculus:*` | DB queries |
| `pet:*` | DB queries |

A malicious client can spam `inventory:load` or `skill:data` at hundreds/second, each triggering a full PostgreSQL query, leading to DB connection pool exhaustion.

**Recommendation**: Add default rate limits for ALL unrated events:
```javascript
function throttleSocketEvent(socketId, eventName) {
    const limit = SOCKET_RATE_LIMITS[eventName] || 5; // Default: 5/sec for unlisted events
    // ... rest unchanged
}
```

---

## Pass 7: Information Leakage

### Findings

| ID | Severity | Finding | Location |
|----|----------|---------|----------|
| LEAK-1 | **LOW** | Health check exposes raw `err.message` on DB failure | Line 31514 |
| LEAK-2 | **LOW** | Username logged on failed login (aids log-based enumeration) | Line 31601 |
| LEAK-3 | **INFORMATIONAL** | CORS set to `origin: "*"` (Socket.io and Express) | Lines 138, 31406 |
| LEAK-4 | **INFORMATIONAL** | No `helmet` middleware (missing security headers) | Missing |
| LEAK-5 | **INFORMATIONAL** | Player data in broadcasts does not include sensitive fields (GOOD) | Various |
| LEAK-6 | **INFORMATIONAL** | Passwords are NEVER returned in API responses (GOOD) | Lines 31542-31564 |
| LEAK-7 | **INFORMATIONAL** | `.env` is in `.gitignore` (GOOD) | `.gitignore` lines 29-33 |

**LEAK-3**: `cors: { origin: "*" }` on both Express (`app.use(cors())` with default config) and Socket.io allows any website to make requests to the server. For a game server this is typically acceptable (the UE5 client is not a browser), but if a web-based admin panel or dashboard is ever added, this should be restricted.

**LEAK-4**: The `helmet` npm package adds security headers (`X-Content-Type-Options`, `X-Frame-Options`, `Strict-Transport-Security`, etc.). While these are primarily relevant for browser-based clients, they are a best practice.

**Positive findings**: Passwords are hashed with bcrypt (cost 10), never returned in responses, and never logged. JWT tokens are not logged. Player broadcasts contain only game-relevant data (HP, position, name) -- no tokens, passwords, or user IDs.

---

## SQL Injection Analysis

### Summary: LOW RISK
The codebase uses parameterized queries (`$1`, `$2`, etc.) consistently throughout all ~200+ SQL queries. Only one instance of SQL interpolation was found:

**Line 7974** (stat allocation): `UPDATE characters SET ${dbStatName} = $1 ...` -- The interpolated value is validated against a hardcoded whitelist of 6 stat names. Safe in practice, but the pattern should be refactored (see VAL-5).

No instances of string concatenation in SQL queries (`\`SELECT...${var}\``) were found beyond the stat column name pattern.

---

## Prioritized Fix Recommendations

### CRITICAL (Fix Immediately)
1. **VAL-1 + PRIV-2 + PRIV-4**: Add position validation (type, bounds, speed) to `player:position`
2. **VAL-6**: Prevent client from sending `_castComplete: true` to bypass cast times
3. **SOCK-1**: Block all socket events from unauthenticated sockets (before `player:join`)

### HIGH (Fix Before Production)
4. **VAL-2**: Derive characterId from socket in `player:position`, not from client data
5. **VAL-5**: Replace SQL column interpolation with mapping object in stat allocation
6. **RATE-2**: Add default rate limit for all unrated socket events
7. **REST-3**: Gate `/test` routes behind `NODE_ENV === 'development'`
8. **PRIV-1**: Remove or properly gate debug handlers
9. **RATE-1**: Add separate auth-specific rate limiter for login/register

### MEDIUM (Fix in Next Sprint)
10. **AUTH-1**: Add `algorithms: ['HS256']` to all `jwt.verify()` calls
11. **AUTH-3**: Implement token revocation (Redis blacklist on logout)
12. **SOCK-2**: Re-validate JWT on long-lived sockets periodically
13. **VAL-3**: Sanitize chat messages (strip HTML/script tags)
14. **VAL-8 + RATE-2**: Rate-limit remaining socket events
15. **REST-1**: Add coordinate bounds check to REST position save

### LOW (Backlog)
16. **REST-2**: Gate `/api/test` behind development environment
17. **VAL-4**: Align server consumable type check with client `IsConsumable()`
18. **VAL-7**: Add message length limit to `party:chat`
19. **PRIV-3**: Add zeny cost to skill reset
20. **RATE-3**: Add Socket.io connection rate limiting
21. **LEAK-1**: Sanitize error messages in health check
22. **LEAK-2**: Use generic log message for login failures

### INFORMATIONAL (Best Practice)
23. **LEAK-3**: Restrict CORS origin when web admin panel is added
24. **LEAK-4**: Add `helmet` middleware

---

## What Is Already Done Well

1. **Server-authoritative design**: All damage, stats, leveling, economy, and item operations are computed server-side. The client is truly presentation-only for all gameplay-critical operations.
2. **Character ownership**: Every REST route and inventory/equipment handler verifies `character_id + user_id` ownership.
3. **Socket identity**: `findPlayerBySocketId()` prevents player impersonation in 80+ handlers.
4. **Parameterized SQL**: Consistent use of `$1, $2` parameters across ~200 queries.
5. **bcrypt**: Cost factor 10 for password hashing.
6. **Rate limiting**: 37 high-frequency socket events are rate-limited per socket per second.
7. **Soft deletes**: Characters are never hard-deleted; queries filter `deleted = FALSE`.
8. **Password confirmation**: Character deletion requires password re-entry.
9. **PvP toggle**: Global `PVP_ENABLED = false` prevents unintended PvP.
10. **Debug gating**: Debug handlers check `NODE_ENV` (though the gate could be stronger).

---

## Attack Scenario Summary

| Attack | Possible? | Severity | Mitigation |
|--------|-----------|----------|------------|
| SQL injection | No (parameterized queries) | -- | Already mitigated |
| Password brute-force | Partially (100/15min) | MEDIUM | Add auth-specific rate limit |
| Token theft + replay | Yes (24h, no revocation) | MEDIUM | Add token blacklist |
| Speed hacking | Yes (no speed validation) | CRITICAL | Add position validation |
| Teleport hacking | Yes (no bounds check) | CRITICAL | Add bounds + speed check |
| Cast time bypass | Yes (`_castComplete` flag) | CRITICAL | Internalize cast completion |
| Stat/damage manipulation | No (server-authoritative) | -- | Already mitigated |
| Item duplication | No (DB transactions) | -- | Already mitigated |
| Player impersonation (actions) | No (socket-based identity) | -- | Already mitigated |
| Player impersonation (position) | Yes (client-sent characterId) | HIGH | Derive from socket |
| Event flooding | Yes (23+ events unrated) | HIGH | Default rate limit |
| Debug abuse | No in production (NODE_ENV gate) | MEDIUM | Strengthen gate |
| Chat XSS | Low risk (UE5 client) | LOW | Sanitize for future web client |
