# REST API Full Audit Report

**Date**: 2026-03-23
**Scope**: All Express REST endpoints in `server/src/index.js` and `server/src/test_mode.js`
**Server**: Node.js + Express 4.18 + Socket.io 4.8 + PostgreSQL + Redis

---

## 1. Complete Endpoint Catalog

### Main API Endpoints (index.js)

| # | Method | Path | Auth | Middleware | Line | Purpose |
|---|--------|------|------|-----------|------|---------|
| 1 | GET | `/health` | None | None | 31499 | Database health check |
| 2 | POST | `/api/auth/register` | None | `validateRegisterInput`, `limiter` | 31520 | User registration |
| 3 | POST | `/api/auth/login` | None | `limiter` | 31573 | User login |
| 4 | GET | `/api/auth/verify` | JWT | `authenticateToken`, `limiter` | 31636 | Token verification |
| 5 | GET | `/api/servers` | None | `limiter` | 31670 | Server list |
| 6 | GET | `/api/characters` | JWT | `authenticateToken`, `limiter` | 31687 | List user's characters |
| 7 | POST | `/api/characters` | JWT | `authenticateToken`, `limiter` | 31728 | Create character |
| 8 | GET | `/api/characters/:id` | JWT | `authenticateToken`, `limiter` | 31803 | Get single character |
| 9 | DELETE | `/api/characters/:id` | JWT | `authenticateToken`, `limiter` | 31831 | Soft-delete character |
| 10 | PUT | `/api/characters/:id/position` | JWT | `authenticateToken`, `limiter` | 31884 | Save character position |
| 11 | GET | `/api/test` | None | `limiter` | 31926 | Dev test endpoint |

### Test Mode Endpoints (test_mode.js, mounted at `/test`)

| # | Method | Path | Auth | Line | Purpose |
|---|--------|------|------|------|---------|
| 12 | GET | `/test/setup` | None | 123 | Return all mock data |
| 13 | GET | `/test/character` | None | 131 | Mock character data |
| 14 | GET | `/test/shop` | None | 138 | Mock shop data |
| 15 | GET | `/test/inventory` | None | 145 | Mock inventory data |
| 16 | GET | `/test/combat` | None | 152 | Mock combat data |
| 17 | POST | `/test/socket-event` | None | 160 | Simulate socket events |
| 18 | POST | `/test/reset` | None | 251 | Reset mock data |

---

## 2. Per-Endpoint Security Assessment

### 2.1 GET /health (Line 31499)

**Authentication**: None (correct -- health checks should be public)
**Authorization**: N/A
**Input Validation**: None needed
**SQL**: `SELECT NOW()` -- safe, no user input
**Response on success**: `{ status: "OK", timestamp, message }`
**Response on error**: `{ status: "ERROR", message: "Database connection failed", error: err.message }`

**FINDING [LOW]**: Error response at line 31514 leaks `err.message` to the client. This could expose internal PostgreSQL error details (connection strings, host info, driver version).

### 2.2 POST /api/auth/register (Line 31520)

**Authentication**: None (correct)
**Authorization**: N/A
**Input Validation (validateRegisterInput middleware, line 31453)**:
- Username: length 3-50, **but no character-set restriction** (allows SQL special chars, Unicode, HTML tags)
- Email: regex `/^[^\s@]+@[^\s@]+\.[^\s@]+$/` -- basic, allows `"><script>@x.x`
- Password: min 8 chars, must contain at least one letter and one number
- **No CAPTCHA or abuse prevention** beyond rate limiting

**SQL Operations**:
- Duplicate check: `SELECT user_id FROM users WHERE username = $1 OR email = $2` (parameterized, safe)
- Insert: `INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3)` (parameterized, safe)

**Password Hashing**: bcrypt with 10 salt rounds (industry standard, good)

**Token Generation**: JWT with `{ user_id, username }`, expires in 24h

**Response on success (201)**:
```json
{ "message": "...", "user": { "user_id", "username", "email", "created_at" }, "token" }
```

**FINDING [MEDIUM]**: Username has no character-set validation. A user could register with `<script>alert(1)</script>` as a username. While parameterized queries prevent SQL injection, this is an XSS vector if the username is ever rendered in HTML/UI without escaping.

**FINDING [LOW]**: Registration immediately returns a JWT token, auto-logging the user in. This is standard practice but means there is no email verification step.

### 2.3 POST /api/auth/login (Line 31573)

**Authentication**: None (correct)
**Authorization**: N/A
**Input Validation**: Checks for missing `username`/`password` only -- no character set or length limits on login input

**SQL**: `SELECT user_id, username, email, password_hash FROM users WHERE username = $1` (parameterized, safe)

**Credential Checking**:
- Lookup by username
- bcrypt.compare for password
- Returns generic "Invalid credentials" for both user-not-found and wrong-password (good -- no user enumeration)
- Updates `last_login` timestamp on success

**FINDING [HIGH]**: No brute force protection beyond the global rate limiter (100 requests / 15 minutes per IP). An attacker can attempt 100 password guesses every 15 minutes from a single IP. There is no:
- Per-account lockout after N failed attempts
- Exponential backoff
- CAPTCHA after failed attempts
- Failed login attempt logging to a dedicated table (only logged to console/file)

**FINDING [LOW]**: The login endpoint does not validate input length. An attacker could send an extremely long password string (megabytes), causing bcrypt.compare to consume excessive CPU time. This is a potential DoS vector since `express.json()` has no body size limit configured.

### 2.4 GET /api/auth/verify (Line 31636)

**Authentication**: JWT via `authenticateToken`
**Authorization**: Returns info only for the authenticated user (uses `req.user.user_id`)
**Input Validation**: N/A (no user input beyond the token)
**SQL**: `SELECT ... FROM users WHERE user_id = $1` (parameterized, safe)

**Assessment**: Properly implemented. Returns 404 if user deleted from DB but token still valid.

### 2.5 GET /api/servers (Line 31670)

**Authentication**: None (correct -- server list is public)
**Authorization**: N/A
**Input Validation**: N/A

**Response**: Returns hardcoded server info with live `connectedPlayers.size` as population count.

**FINDING [INFO]**: Exposes real-time player count. Acceptable for an MMO server list, but could be used for competitive intelligence or to identify low-activity windows for attacks.

### 2.6 GET /api/characters (Line 31687)

**Authentication**: JWT
**Authorization**: Filters by `user_id = $1` with `req.user.user_id` (correct -- users can only see their own characters)
**Input Validation**: N/A

**SQL**: Selects character data with `WHERE user_id = $1 AND delete_date IS NULL AND deleted = FALSE` and `LIMIT 9` (correctly excludes deleted characters, enforces limit)

**Enrichment**: Adds `level_name` from zone registry for each character.

**Assessment**: Properly implemented. Ownership check and soft-delete filter both correct.

### 2.7 POST /api/characters (Line 31728)

**Authentication**: JWT
**Authorization**: Creates character under `req.user.user_id`

**Input Validation**:
- Name: 2-24 chars, trimmed, alphanumeric + spaces only (`/^[a-zA-Z0-9 ]+$/`). Good restriction.
- Character limit: `COUNT(*) ... WHERE user_id = $1 AND delete_date IS NULL AND deleted = FALSE` -- max 9 per account
- Name uniqueness: `LOWER(name) = LOWER($1)` -- case-insensitive, global across ALL characters
- Hair style: clamped to 1-19 via `Math.max/min`
- Hair color: clamped to 0-8
- Gender: whitelist to 'male' or 'female'
- Class: **Hardcoded to 'novice'** -- the `characterClass` from `req.body` is completely ignored. This is correct and prevents class injection.

**SQL**: Parameterized INSERT with `RETURNING` clause (safe)

**Duplicate handling**: Both application-level check and PostgreSQL UNIQUE constraint (`err.code === '23505'`)

**FINDING [LOW]**: Character name uniqueness check at line 31753 queries ALL characters including deleted ones (`WHERE LOWER(name) = LOWER($1)` with no `deleted = FALSE` filter). This means once a character name is used, it can never be reused even after deletion. This may be intentional (preventing impersonation) but could frustrate users.

**Assessment**: Well implemented. All user-provided values are validated or overridden.

### 2.8 GET /api/characters/:id (Line 31803)

**Authentication**: JWT
**Authorization**: `WHERE character_id = $1 AND user_id = $2` (correct -- ownership check)
**Input Validation**: `characterId = req.params.id` -- **not parsed to integer**

**FINDING [MEDIUM]**: Missing `deleted = FALSE` filter. A user can retrieve their own soft-deleted characters via this endpoint. The query at line 31809 is:
```sql
SELECT ... FROM characters WHERE character_id = $1 AND user_id = $2
```
This should include `AND deleted = FALSE` for consistency with the list endpoint.

**FINDING [LOW]**: `characterId` is not parsed to integer (`req.params.id` is passed directly). PostgreSQL's parameterized query will handle this safely (implicit cast), but it is inconsistent with the DELETE endpoint which uses `parseInt()`.

### 2.9 DELETE /api/characters/:id (Line 31831)

**Authentication**: JWT
**Authorization**: Multi-layer:
1. JWT ownership: `req.user.user_id`
2. Password re-verification via bcrypt (defense in depth)
3. Character ownership: `WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE`
4. Online check: `connectedPlayers.has(characterId)` prevents deleting active characters

**Input Validation**:
- `characterId = parseInt(req.params.id)` (properly parsed)
- Requires `password` in request body

**SQL**: `UPDATE characters SET deleted = TRUE WHERE character_id = $1` (soft delete, safe)

**Assessment**: Excellent implementation. Password confirmation, ownership check, online guard, and soft-delete. Best-protected endpoint in the API.

### 2.10 PUT /api/characters/:id/position (Line 31884)

**Authentication**: JWT
**Authorization**: `WHERE character_id = $1 AND user_id = $2` (ownership check present)

**Input Validation**:
- `x`, `y`, `z` must be `typeof ... === 'number'` (rejects strings, nulls, etc.)
- `characterId = req.params.id` -- **not parsed to integer**

**FINDING [MEDIUM]**: Missing `deleted = FALSE` filter. The ownership query at line 31898 is:
```sql
SELECT character_id FROM characters WHERE character_id = $1 AND user_id = $2
```
This allows updating the position of a soft-deleted character.

**FINDING [LOW]**: No bounds checking on coordinate values. A client could set `x = Number.MAX_SAFE_INTEGER` or `x = -Infinity`. While server-authoritative combat uses in-memory positions, this corrupts the DB save point.

**FINDING [LOW]**: This endpoint is likely redundant -- position is managed via Socket.io `player:position` events in real-time, and saved to DB on disconnect. Keeping a REST endpoint for position updates creates a second write path that could conflict.

### 2.11 GET /api/test (Line 31926)

**Authentication**: None
**Input Validation**: None

**Assessment**: Harmless test endpoint that returns a static JSON message. Should be removed or gated behind a `NODE_ENV === 'development'` check in production.

### 2.12-18 Test Mode Endpoints (/test/*)

**Authentication**: None
**Rate Limiting**: Explicitly excluded (`app.use('/test', testModeRouter)` is mounted BEFORE the rate limiter at `/api/`)

**FINDING [HIGH]**: The test mode router is always mounted regardless of `NODE_ENV`. In production, these endpoints would:
- Expose mock data structures that reveal internal data schemas
- Allow simulating socket events without authentication (`POST /test/socket-event`)
- Have no rate limiting (mounted before the limiter middleware)

The test endpoints are mounted at line 31418 with no environment guard:
```javascript
app.use('/test', testModeRouter);
```

---

## 3. Infrastructure Security Assessment

### 3.1 CORS Configuration

**Line 31406**: `app.use(cors())`
**Line 136-141**: Socket.io: `cors: { origin: "*", methods: ["GET", "POST"] }`

**FINDING [HIGH]**: CORS is completely open (`cors()` with no options = allow all origins). Both Express and Socket.io allow `origin: "*"`. Any website can make authenticated requests to this API if a user's JWT is compromised or stored in an accessible location (localStorage, etc.). For a game server this is less critical than a web app (the UE5 client uses HTTP directly, not a browser), but it would be a problem if any browser-based admin panel or web client is added.

### 3.2 Security Headers

**FINDING [MEDIUM]**: No `helmet` middleware installed or used. The server does not set:
- `X-Content-Type-Options: nosniff`
- `X-Frame-Options: DENY`
- `Strict-Transport-Security`
- `Content-Security-Policy`
- `X-XSS-Protection`

Package.json confirms `helmet` is not a dependency.

### 3.3 Request Body Size Limits

**Line 31407**: `app.use(express.json())`

**FINDING [MEDIUM]**: No body size limit configured. The default `express.json()` limit is 100KB, which is the Express default and may be adequate. However, it should be explicitly set for clarity:
```javascript
app.use(express.json({ limit: '10kb' })) // Most API payloads are tiny
```

### 3.4 Rate Limiting

**Line 31410-31415**: `express-rate-limit` configured:
- Window: 15 minutes
- Max requests per IP: 100
- Applied to: `/api/*` routes only

**FINDING [MEDIUM]**: 100 requests per 15 minutes is quite restrictive for normal gameplay. The character list endpoint is called every time the character select screen opens. Login + verify + character list + select = 4 requests just to start playing. A player who disconnects and reconnects a few times could hit the limit. This rate limiter is too aggressive for legitimate use while still allowing 100 login attempts per window for brute force.

**Recommendation**: Use separate rate limiters per category:
- Auth endpoints (login/register): 10 per 15 minutes per IP (strict)
- Authenticated endpoints: 200 per 15 minutes per IP (generous for gameplay)
- Health check: Excluded from rate limiting

### 3.5 Parameter Pollution

**FINDING [LOW]**: No HPP (HTTP Parameter Pollution) middleware installed. Express will use the last value if duplicate query parameters are sent. Not a significant risk since no endpoints use `req.query`.

### 3.6 HTTP Method Override

Not present. No `method-override` middleware installed. This is correct -- method override is a legacy pattern that introduces security risks.

### 3.7 Error Handling

**FINDING [LOW]**: No global Express error handler (`app.use((err, req, res, next) => ...)`). If an unhandled error occurs in middleware or an uncaught async rejection in a route, Express returns its default HTML error page with stack traces in development mode.

### 3.8 Logging

- Request logging middleware at line 31421 logs `${req.method} ${req.url} - ${req.ip}` for every request
- Individual endpoints log operations at INFO/WARN/ERROR levels
- Log output goes to console + `server.log` file

**FINDING [LOW]**: Passwords and tokens are not explicitly redacted from logs. The registration/login handlers log `username` but not `password` (good). However, the request logging middleware logs the full URL which could include tokens in query strings if any endpoint accepted them that way (currently none do).

---

## 4. Authentication Flow Analysis

### 4.1 Registration Flow

1. Client sends `{ username, email, password }`
2. `validateRegisterInput` checks length/format
3. Duplicate check via DB query
4. Password hashed with bcrypt (10 rounds)
5. User created in DB
6. JWT issued immediately (auto-login)

**Gaps**:
- No email verification
- No CAPTCHA
- Username allows special characters (only length validated, no charset restriction)
- Registration spam possible (one attacker could create many accounts within rate limit window)

### 4.2 Login Flow

1. Client sends `{ username, password }`
2. Basic presence validation
3. User lookup by username
4. bcrypt.compare for password
5. `last_login` updated
6. JWT issued

**Gaps**:
- No account lockout after failed attempts
- No exponential backoff
- Same generic error for user-not-found and wrong-password (good for security)
- No MFA/2FA support

### 4.3 Token Management

- **Algorithm**: Default `jsonwebtoken` (HS256)
- **Payload**: `{ user_id, username }`
- **Expiry**: 24 hours
- **Refresh**: **No refresh token mechanism exists**
- **Revocation**: **No token blacklist/revocation mechanism**
- **Storage**: Client-side (UE5 GameInstance memory, not persistent)

**FINDING [MEDIUM]**: No token refresh mechanism. After 24 hours, the user must re-authenticate with username/password. For an MMO where sessions can last many hours, this is a UX issue. If a player is in the middle of gameplay and the token expires, their socket connection (which validated the token at `player:join`) continues to work, but any REST API calls will fail.

**FINDING [MEDIUM]**: No token revocation. If a token is compromised, there is no way to invalidate it before expiry. A compromised token grants 24 hours of access. Password change does not invalidate existing tokens (no password change endpoint exists anyway).

### 4.4 Socket.io Authentication

- `player:join` event validates JWT (line 5300-5325)
- Verifies character ownership via DB query
- **No subsequent authentication** -- once connected, the socket is trusted for all events
- Socket.io rate limiting per event type (line 5226-5262)

**FINDING [MEDIUM]**: No duplicate session prevention. If a player connects twice with the same character, `player:join` does not check if the character is already in `connectedPlayers`. The second connection would overwrite the first player's data in the Map, potentially causing desync, inventory duplication, or combat state corruption. The DELETE endpoint checks `connectedPlayers.has(characterId)`, but `player:join` itself does not.

---

## 5. Character Management Analysis

### 5.1 Character Creation

- **Limit**: 9 per account (enforced via COUNT query)
- **Name**: 2-24 chars, alphanumeric + spaces, case-insensitive global uniqueness
- **Class**: Hardcoded to 'novice' (client input ignored -- good)
- **Customization**: Hair style (1-19), hair color (0-8), gender (male/female) -- all clamped/whitelisted
- **Starting position**: 0, 0, 0 (default from SQL)
- **Starting stats**: level 1, 100 HP, 100 MP, 48 stat points

**FINDING [INFO]**: Character names can start or end with spaces (after trim). Names like " Bob " become "Bob", but "A  B" (double space) is valid. The regex `/^[a-zA-Z0-9 ]+$/` allows multiple consecutive spaces.

### 5.2 Character Deletion

- Requires password re-entry (defense in depth)
- Soft-delete: `UPDATE ... SET deleted = TRUE`
- Blocks deletion of online characters
- Ownership verified

**FINDING [INFO]**: No recovery mechanism for deleted characters. Once `deleted = TRUE`, there is no REST endpoint to undelete. Manual DB intervention would be required.

**FINDING [INFO]**: The `delete_date` column exists but is NOT set during deletion. Line 31871 only sets `deleted = TRUE`, not `delete_date = CURRENT_TIMESTAMP`. This means the timestamp of deletion is not recorded.

### 5.3 Character Selection (GET /api/characters/:id)

- Ownership verified via `user_id` in WHERE clause
- **Missing `deleted = FALSE` filter** (see finding 2.8)

### 5.4 Name Injection Testing

Character names: The regex `/^[a-zA-Z0-9 ]+$/` at line 31739 prevents:
- SQL injection (no quotes, semicolons, dashes)
- XSS (no angle brackets, script tags)
- Command injection (no backticks, pipes, etc.)

Usernames: **No character-set validation** -- only length (3-50). Usernames CAN contain:
- HTML tags: `<script>alert(1)</script>`
- SQL fragments: `'; DROP TABLE users; --`
- Unicode: Emoji, RTL override characters, zero-width spaces

While parameterized queries prevent SQL injection, usernames could be XSS vectors if rendered in HTML.

---

## 6. Missing Endpoints Analysis

### Operations that exist in socket handlers but lack REST equivalents:

| Operation | Socket Event | REST Equivalent | Priority |
|-----------|-------------|-----------------|----------|
| Server status/stats | N/A | `GET /api/status` (player count, uptime, zone populations) | LOW |
| Leaderboard | N/A | `GET /api/leaderboards` (top players by level, zeny, etc.) | LOW |
| Character online status | In-memory `connectedPlayers` | `GET /api/characters/:id/online` | LOW |
| Password change | N/A | `PUT /api/auth/password` | **HIGH** |
| Account deletion | N/A | `DELETE /api/auth/account` | MEDIUM |
| Character recovery | N/A | `POST /api/characters/:id/recover` (undelete) | LOW |
| Token refresh | N/A | `POST /api/auth/refresh` | MEDIUM |
| Zone player list | Socket broadcast | `GET /api/zones/:name/players` | LOW |
| Item database | In-memory `ITEM_USE_EFFECTS` | `GET /api/items` (public item database for wiki/tooltip) | LOW |

### Missing endpoints by priority:

**HIGH** -- Should be implemented:
1. `PUT /api/auth/password` -- Password change. Currently there is no way to change a password without direct DB access. Requires current password + new password + token.

**MEDIUM** -- Recommended:
2. `POST /api/auth/refresh` -- Token refresh. Prevents forced re-login after 24 hours.
3. `DELETE /api/auth/account` -- Account deletion (GDPR compliance). Soft-delete user + all characters.

---

## 7. Consolidated Findings

### Critical (0)
None.

### High Severity (3)

| ID | Finding | Location | Description |
|----|---------|----------|-------------|
| H1 | No brute force protection | Login (31573) | 100 attempts per 15min per IP. No per-account lockout, no CAPTCHA, no exponential backoff. |
| H2 | Test mode always active | test_mode.js (31418) | Test endpoints are mounted unconditionally, exposing mock data and simulated socket events without auth or rate limiting in all environments. |
| H3 | CORS fully open | Line 31406, 136-141 | `cors()` with no origin restriction on both Express and Socket.io. Any origin can make API requests. |

### Medium Severity (6)

| ID | Finding | Location | Description |
|----|---------|----------|-------------|
| M1 | Missing `deleted = FALSE` on GET character | Line 31809 | `GET /api/characters/:id` returns soft-deleted characters. |
| M2 | Missing `deleted = FALSE` on PUT position | Line 31898 | `PUT /api/characters/:id/position` allows updating deleted character positions. |
| M3 | No security headers (helmet) | package.json | No `X-Content-Type-Options`, `X-Frame-Options`, HSTS, CSP headers. |
| M4 | No token refresh/revocation | Auth flow | Tokens are valid for 24h with no way to refresh or revoke them. |
| M5 | No duplicate session prevention | player:join (5295) | A character can join twice, overwriting the connectedPlayers entry and causing potential duplication or state corruption. |
| M6 | Username has no charset validation | validateRegisterInput (31453) | Usernames can contain HTML/script tags, Unicode control characters. |

### Low Severity (9)

| ID | Finding | Location | Description |
|----|---------|----------|-------------|
| L1 | Error message leaks DB details | Health check (31514) | `err.message` returned to client on health check failure. |
| L2 | No body size limit configured | express.json() (31407) | Relies on Express default 100KB. Should be explicit and smaller. |
| L3 | Inconsistent parseInt on :id param | Lines 31805, 31833, 31886 | Only DELETE uses `parseInt(req.params.id)`. GET and PUT pass raw string. |
| L4 | No coordinate bounds checking | Position (31884) | x/y/z validated as numbers but not bounded. Infinity, MAX_SAFE_INTEGER accepted. |
| L5 | Rate limiter too aggressive for gameplay | Limiter (31410) | 100 req/15min applies uniformly to all /api/ routes. |
| L6 | Character name includes deleted names | CREATE char (31753) | Name uniqueness check does not exclude deleted characters. |
| L7 | delete_date not set on deletion | DELETE char (31871) | `deleted = TRUE` is set but `delete_date` column is never populated. |
| L8 | No global error handler | Express setup | Missing `app.use((err, req, res, next) => ...)` fallback. |
| L9 | No password change endpoint | Auth flow | Users cannot change their password. |

### Informational (4)

| ID | Finding | Location | Description |
|----|---------|----------|-------------|
| I1 | No email verification | Registration | Users can register with any email without verification. |
| I2 | Multiple consecutive spaces in character names | CREATE char (31739) | Regex allows "A  B" (double space). |
| I3 | No character recovery endpoint | DELETE char | Soft-deleted characters cannot be recovered via API. |
| I4 | Live player count exposed | GET /api/servers | Population count is public. |

---

## 8. Recommendations (Prioritized)

### Immediate (before any production deployment)

1. **Gate test mode behind NODE_ENV** (H2):
   ```javascript
   if (process.env.NODE_ENV !== 'production') {
       app.use('/test', testModeRouter);
   }
   ```

2. **Restrict CORS origins** (H3):
   ```javascript
   app.use(cors({
       origin: process.env.ALLOWED_ORIGINS?.split(',') || ['http://localhost:3000'],
       credentials: true
   }));
   ```

3. **Add per-account login throttling** (H1):
   Track failed login attempts per username. After 5 failures, require a 5-minute cooldown or CAPTCHA.

### Short-term

4. **Add `deleted = FALSE` to character queries** (M1, M2):
   - Line 31809: Add `AND deleted = FALSE`
   - Line 31898: Add `AND deleted = FALSE`

5. **Add helmet** (M3):
   ```bash
   npm install helmet
   ```
   ```javascript
   app.use(helmet());
   ```

6. **Add username charset validation** (M6):
   ```javascript
   if (!/^[a-zA-Z0-9_]+$/.test(username)) {
       return res.status(400).json({ error: 'Username can only contain letters, numbers, and underscores' });
   }
   ```

7. **Prevent duplicate sessions** (M5):
   In `player:join`, check if character is already connected:
   ```javascript
   if (connectedPlayers.has(characterId)) {
       socket.emit('player:join_error', { error: 'Character is already online' });
       return;
   }
   ```

8. **Set explicit body size limit** (L2):
   ```javascript
   app.use(express.json({ limit: '16kb' }));
   ```

9. **Set delete_date on deletion** (L7):
   ```javascript
   await pool.query(
       'UPDATE characters SET deleted = TRUE, delete_date = CURRENT_TIMESTAMP WHERE character_id = $1',
       [characterId]
   );
   ```

10. **Sanitize health check error** (L1):
    ```javascript
    res.status(500).json({ status: 'ERROR', message: 'Database connection failed' });
    // Remove: error: err.message
    ```

### Medium-term

11. **Add password change endpoint** (L9/M4)
12. **Add token refresh mechanism** (M4)
13. **Split rate limiters** per route category (L5)
14. **Add global Express error handler** (L8)
15. **Normalize parseInt on all :id params** (L3)
16. **Add coordinate bounds checking** (L4)

---

## 9. Positive Security Observations

The following security practices are already well implemented:

1. **Parameterized queries throughout** -- All SQL uses `$1, $2` placeholders. Zero string concatenation in queries. No SQL injection risk.
2. **bcrypt password hashing** -- Industry-standard with 10 salt rounds.
3. **JWT on all authenticated endpoints** -- Consistent use of `authenticateToken` middleware.
4. **Ownership checks** -- Every character endpoint verifies `user_id` matches the JWT.
5. **Password re-verification on delete** -- Defense in depth for destructive operations.
6. **Online character guard on delete** -- Prevents deleting active characters.
7. **Character class hardcoded** -- Client input for class is ignored (always 'novice').
8. **Soft-delete pattern** -- Characters are never hard-deleted.
9. **Socket.io per-event rate limiting** -- Custom throttle system with per-event limits.
10. **JWT on socket join** -- `player:join` validates JWT and verifies character ownership.
11. **Generic auth error messages** -- Login returns "Invalid credentials" for both bad username and bad password.
12. **Character name sanitization** -- Alphanumeric + spaces only, preventing injection.
13. **Request logging** -- All requests logged with method, URL, IP.

---

## 10. Summary

| Category | Count |
|----------|-------|
| Total endpoints | 18 (11 main + 7 test) |
| Authenticated endpoints | 6 |
| Public endpoints | 12 (5 main + 7 test) |
| Critical findings | 0 |
| High findings | 3 |
| Medium findings | 6 |
| Low findings | 9 |
| Informational | 4 |

The REST API has a solid foundation -- parameterized queries, bcrypt, JWT auth, ownership checks, and soft-delete are all correctly implemented. The main gaps are operational security hardening (brute force protection, CORS restriction, helmet headers, test mode gating) rather than fundamental architectural flaws. The most actionable items are H2 (test mode gating) and the two missing `deleted = FALSE` filters (M1, M2), which are single-line fixes.
