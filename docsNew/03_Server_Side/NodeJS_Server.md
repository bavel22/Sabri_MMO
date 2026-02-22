# Node.js Server Documentation

## Overview

The entire server is a single monolithic file: `server/src/index.js` (2,269 lines). It handles REST API, Socket.io real-time events, combat tick loop, enemy AI, inventory management, and database operations.

**Runtime**: Node.js 18+ LTS  
**Port**: 3001 (configurable via `PORT` env var)  
**Entry**: `node src/index.js` or `npm start`

## File Structure

```
server/
├── src/
│   └── index.js          # Main server (2,269 lines)
├── .env                  # Environment variables
├── package.json          # Dependencies
├── package-lock.json     # Lock file
├── start-server.bat      # Windows quick-start
├── logs/
│   └── server.log        # Runtime log output
├── check_database.js     # DB diagnostic script
├── check_table.js        # Table diagnostic script
├── test_auth.js          # Auth test script
├── test_login.js         # Login test script
└── test.txt              # Empty test file
```

## Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| `express` | ^4.18.2 | REST API framework |
| `socket.io` | ^4.8.3 | Real-time WebSocket server |
| `pg` | ^8.11.3 | PostgreSQL client (Pool) |
| `redis` | ^5.10.0 | Redis client (v4+ async API) |
| `bcrypt` | ^5.1.1 | Password hashing (10 salt rounds) |
| `jsonwebtoken` | ^9.0.3 | JWT creation/verification |
| `cors` | ^2.8.5 | Cross-origin request handling |
| `express-rate-limit` | ^8.2.1 | API rate limiting |
| `dotenv` | ^16.3.1 | `.env` file loading |
| `nodemon` | ^3.0.2 | Dev auto-restart (devDependency) |

## Environment Variables (.env)

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT` | 3001 | Server port |
| `DB_HOST` | localhost | PostgreSQL host |
| `DB_PORT` | 5432 | PostgreSQL port |
| `DB_NAME` | sabri_mmo | Database name |
| `DB_USER` | postgres | Database user |
| `DB_PASSWORD` | goku22 | Database password |
| `JWT_SECRET` | _(required)_ | JWT signing secret |
| `LOG_LEVEL` | INFO | Logging level (DEBUG/INFO/WARN/ERROR) |

## Logging System

```javascript
const logger = {
    debug: (...args) => { /* writes to console + server.log */ },
    info:  (...args) => { /* writes to console + server.log */ },
    warn:  (...args) => { /* writes to console + server.log */ },
    error: (...args) => { /* writes to console + server.log */ }
};
```

- **Format**: `[ISO_TIMESTAMP] [LEVEL] message`
- **File**: `server/logs/server.log` (append mode)
- **Level Filter**: Only logs at or above `LOG_LEVEL`
- **Conventions**: `[RECV]` incoming, `[SEND]` outgoing, `[BROADCAST]` all clients, `[COMBAT]` combat logic, `[ENEMY]` enemy system, `[ITEMS]` inventory, `[DB]` database, `[LOOT]` drops

## Middleware Stack

1. **CORS**: `cors()` — allows all origins
2. **JSON Parser**: `express.json()`
3. **Rate Limiter**: 100 requests / 15 minutes on `/api/*`
4. **Request Logger**: Logs `METHOD URL - IP` for every request
5. **JWT Auth** (`authenticateToken`): Applied per-route on protected endpoints

## Server Startup Sequence

```
1. Start HTTP server on PORT
2. Test Redis connection (ping)
3. Ensure stat columns exist on characters table (ALTER TABLE IF NOT EXISTS)
4. Ensure items table exists (CREATE TABLE IF NOT EXISTS)
5. Ensure weapon_type columns exist on items table
6. Ensure character_inventory table + indexes exist
7. Seed base items if items table is empty (16 items)
8. Load item definitions into memory cache
9. Spawn initial enemies (12 enemies from ENEMY_SPAWNS)
```

## In-Memory Data Structures

### connectedPlayers: Map<charId, PlayerData>
```javascript
{
    socketId: string,
    characterId: number,
    characterName: string,
    health: number,
    maxHealth: number,
    mana: number,
    maxMana: number,
    isDead: boolean,
    lastAttackTime: number,        // Date.now() of last attack
    aspd: number,                  // Attack speed (0-190)
    attackRange: number,           // Attack range (UE units)
    stats: {                       // Base stats
        str, agi, vit, int, dex, luk, level, weaponATK, statPoints
    }
}
```

### autoAttackState: Map<attackerId, AttackState>
```javascript
{
    targetCharId: number,          // Target character or enemy ID
    isEnemy: boolean,              // true = enemy target, false = player target
    startTime: number              // When auto-attack was initiated
}
```

### enemies: Map<enemyId, EnemyData>
```javascript
{
    enemyId: number, templateId: string, name: string, level: number,
    health: number, maxHealth: number, damage: number,
    attackRange: number, aggroRange: number, aspd: number, exp: number,
    aiType: string, stats: {...}, isDead: boolean,
    x: number, y: number, z: number,
    spawnX: number, spawnY: number, spawnZ: number,
    wanderRadius: number, respawnMs: number,
    targetPlayerId: number|null, lastAttackTime: number,
    inCombatWith: Set<charId>,
    // Wander state:
    wanderTargetX: number, wanderTargetY: number,
    isWandering: boolean, nextWanderTime: number, lastMoveBroadcast: number
}
```

### itemDefinitions: Map<itemId, ItemDef>
Cached rows from the `items` database table. Loaded on startup via `loadItemDefinitions()`.

## Combat Constants

```javascript
const COMBAT = {
    BASE_DAMAGE: 1,
    DAMAGE_VARIANCE: 0,
    MELEE_RANGE: 150,           // Default melee range (UE units)
    RANGED_RANGE: 800,          // Default ranged range
    DEFAULT_ASPD: 180,          // Default attack speed
    ASPD_CAP: 190,              // Maximum ASPD
    RANGE_TOLERANCE: 50,        // Padding for range checks
    COMBAT_TICK_MS: 50,         // Combat loop interval
    RESPAWN_DELAY_MS: 5000,
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }
};
```

## Enemy AI Constants

```javascript
const ENEMY_AI = {
    WANDER_TICK_MS: 500,
    WANDER_PAUSE_MIN: 3000,     // 3s minimum pause
    WANDER_PAUSE_MAX: 8000,     // 8s maximum pause
    WANDER_SPEED: 60,           // Units per second
    WANDER_DIST_MIN: 100,       // Min wander distance
    WANDER_DIST_MAX: 300,       // Max wander distance
    MOVE_BROADCAST_MS: 200      // Position update frequency
};
```

## Enemy Templates

| Template | Level | HP | ATK | Aggro Range | ASPD | EXP | AI Type | Respawn |
|----------|-------|----|-----|-------------|------|-----|---------|---------|
| Blobby | 1 | 50 | 1 | 300 | 175 | 10 | passive | 10s |
| Hoplet | 3 | 100 | 3 | 400 | 178 | 25 | passive | 15s |
| Crawlid | 2 | 75 | 2 | 0 | 176 | 15 | passive | 12s |
| Shroomkin | 4 | 120 | 4 | 350 | 177 | 30 | passive | 15s |
| Buzzer | 5 | 150 | 5 | 500 | 179 | 40 | aggressive | 18s |
| Mosswort | 3 | 5 | 2 | 0 | 174 | 20 | passive | 12s |

## Stat Formulas (RO-Style)

```javascript
function calculateDerivedStats(stats) {
    const statusATK = str + Math.floor(str/10)**2 + Math.floor(dex/5) + Math.floor(luk/3);
    const statusMATK = intStat + Math.floor(intStat/7)**2;
    const hit = level + dex;
    const flee = level + agi;
    const softDEF = Math.floor(vit*0.5 + (vit**2)/150);
    const softMDEF = Math.floor(intStat*0.5);
    const critical = Math.floor(luk*0.3);
    const aspd = Math.min(190, Math.floor(170 + agi*0.4 + dex*0.1));
    const maxHP = 100 + vit*8 + level*10;
    const maxSP = 50 + intStat*5 + level*5;
    return { statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, aspd, maxHP, maxSP };
}

function getAttackIntervalMs(aspd) {
    return (200 - Math.min(aspd, 190)) * 50;  // ASPD 180 → 1000ms, ASPD 190 → 500ms
}
```

## Periodic Tasks

| Task | Interval | Purpose |
|------|----------|---------|
| Combat Tick | 50ms | Process all auto-attacks |
| Enemy AI Tick | 500ms | Enemy wandering movement |
| Health Save | 60,000ms | Save all online players' HP/MP to DB |

---

**Last Updated**: 2026-02-17
