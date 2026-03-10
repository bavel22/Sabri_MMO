# 05 -- Enemy / Monster System: UE5 C++ Implementation Guide

> Complete implementation reference for the Sabri_MMO enemy system.
> Covers the full server-side architecture (already implemented) and the UE5 C++ client-side architecture (to be built by Claude Code without manual Blueprint input).

---

## Table of Contents

1. [Server-Side Monster Template System](#1-server-side-monster-template-system)
2. [Server-Side Spawn Manager](#2-server-side-spawn-manager)
3. [Server-Side AI State Machine (200ms tick)](#3-server-side-ai-state-machine-200ms-tick)
4. [Server-Side Monster Combat](#4-server-side-monster-combat)
5. [Server-Side Drop System](#5-server-side-drop-system)
6. [Client-Side: Enemy Manager (UE5 C++)](#6-client-side-enemy-manager-ue5-c)
7. [Client-Side: Enemy Animations](#7-client-side-enemy-animations)
8. [Client-Side: Loot System](#8-client-side-loot-system)
9. [MVP/Boss Special Handling](#9-mvpboss-special-handling)
10. [Adding a New Monster (Step-by-Step)](#10-adding-a-new-monster-step-by-step)

---

## 1. Server-Side Monster Template System

### Overview

All monster definitions live in `server/src/ro_monster_templates.js` -- a single JavaScript module exporting the `RO_MONSTER_TEMPLATES` object. Each key is a lowercase snake_case identifier (e.g. `'poring'`, `'archer_skeleton'`, `'baphomet'`), and each value is a template object with every field needed by the server AI, combat, and drop systems.

**File**: `server/src/ro_monster_templates.js`
**Total monsters**: 509 (auto-generated from rAthena pre-renewal mob_db.yml)
**Level range**: 1-99

### Template Data Structure

```javascript
RO_MONSTER_TEMPLATES['poring'] = {
    // ---- Identity ----
    id: 1002,                              // RO monster ID (from rAthena mob_db)
    name: 'Poring',                        // Display name shown to players
    aegisName: 'PORING',                   // rAthena aegis identifier

    // ---- Level & EXP ----
    level: 1,                              // Monster level
    maxHealth: 50,                         // HP pool
    baseExp: 2,                            // Base EXP awarded on kill
    jobExp: 1,                             // Job EXP awarded on kill
    mvpExp: 0,                             // MVP bonus EXP (0 for non-MVPs)

    // ---- Offense ----
    attack: 7,                             // Min ATK damage
    attack2: 10,                           // Max ATK damage
    attackRange: 50,                       // Attack range in UE units (1 RO cell = 50 UE units)

    // ---- Defense ----
    defense: 0,                            // Physical DEF (hard DEF)
    magicDefense: 5,                       // Magic DEF (MDEF)

    // ---- Stats (used by damage formula) ----
    str: 0, agi: 0, vit: 0,
    int: 0, dex: 6, luk: 30,

    // ---- Speed & Timing ----
    aspd: 163,                             // Attack speed (0-195 scale)
    walkSpeed: 400,                        // Movement: ms per RO cell (lower = faster)
    attackDelay: 1872,                     // ms between auto-attacks
    attackMotion: 672,                     // Attack animation duration (ms) -- sent to client
    damageMotion: 480,                     // Hit stun duration (ms)

    // ---- Classification ----
    size: 'medium',                        // small | medium | large
    race: 'plant',                         // formless | undead | brute | plant | insect | fish |
                                           // demon | demi-human | angel | dragon
    element: { type: 'water', level: 1 },  // Element type + level (1-4)
    monsterClass: 'normal',                // normal | boss | mvp

    // ---- AI ----
    aiType: 'passive',                     // Simplified: passive | aggressive | reactive
    aggroRange: 0,                         // UE units. 0 = passive (won't auto-aggro)
    chaseRange: 600,                       // Max chase distance from aggro origin (UE units)
    respawnMs: 5500,                       // Time between death and respawn (ms)

    // ---- Mode Flags (raw) ----
    raceGroups: {},                        // Additional race group flags (guardian, etc.)
    modes: {},                             // Raw hex mode flags (if overridden)

    // ---- Stats Duplicate (for damage formula compatibility) ----
    stats: {
        str: 0, agi: 0, vit: 0,
        int: 0, dex: 6, luk: 30,
        level: 1,
        weaponATK: 7                       // Used by roPhysicalDamage()
    },

    // ---- Drop Table (up to 10 slots) ----
    drops: [
        { itemName: 'Jellopy', rate: 70 },
        { itemName: 'Knife', rate: 1 },
        { itemName: 'Sticky Mucus', rate: 4 },
        { itemName: 'Apple', rate: 10 },
        { itemName: 'Empty Bottle', rate: 15 },
        { itemName: 'Apple', rate: 1.5 },
        { itemName: 'Unripe Apple', rate: 0.2 },
        { itemName: 'Poring Card', rate: 0.01, stealProtected: true },
    ],

    // ---- MVP Drops (up to 3 slots, MVPs only) ----
    mvpDrops: [],
};
```

### Template Fields Reference

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `id` | number | RO monster ID (from rAthena) | `1002` |
| `name` | string | Display name | `'Poring'` |
| `aegisName` | string | rAthena aegis name (uppercase) | `'PORING'` |
| `level` | number | Monster level (1-99) | `1` |
| `maxHealth` | number | Maximum HP | `50` |
| `baseExp` | number | Base EXP on kill | `2` |
| `jobExp` | number | Job EXP on kill | `1` |
| `mvpExp` | number | MVP bonus EXP | `0` |
| `attack` | number | Minimum ATK | `7` |
| `attack2` | number | Maximum ATK | `10` |
| `defense` | number | Physical DEF (hard DEF) | `0` |
| `magicDefense` | number | Magic DEF | `5` |
| `str/agi/vit/int/dex/luk` | number | Base stats | Varies |
| `attackRange` | number | Attack range (UE units) | `50` |
| `aggroRange` | number | Aggro detection range (UE units, 0 = passive) | `0` |
| `chaseRange` | number | Max chase distance from aggro origin | `600` |
| `aspd` | number | Attack speed (0-195 scale) | `163` |
| `walkSpeed` | number | Movement speed (ms per RO cell, lower = faster) | `400` |
| `attackDelay` | number | ms between attacks | `1872` |
| `attackMotion` | number | Attack animation duration (ms) | `672` |
| `damageMotion` | number | Hit stun duration (ms) | `480` |
| `size` | string | `'small'` / `'medium'` / `'large'` | `'medium'` |
| `race` | string | Monster race category | `'plant'` |
| `element` | object | `{ type: string, level: number }` | `{ type: 'water', level: 1 }` |
| `monsterClass` | string | `'normal'` / `'boss'` / `'mvp'` | `'normal'` |
| `aiType` | string | Simplified AI type string | `'passive'` |
| `respawnMs` | number | Respawn timer (ms) | `5500` |
| `drops` | array | Drop table entries | See above |
| `mvpDrops` | array | MVP-only drops (max 3) | `[]` |

### Monster Class Categories

| Class | Description | Boss Protocol | Example |
|-------|-------------|---------------|---------|
| `normal` | Standard monster. No special treatment. | No | Poring, Zombie, Wolf |
| `boss` | Mini-boss. Harder than normal, may have unique drops. | Yes (knockback immune, status immune, detector) | Angeling, Deviling, Ghostring |
| `mvp` | Major boss. Massive HP, unique drops, EXP bonus, tombstone on death. | Yes + MVP flag | Baphomet, Osiris, Dark Lord |

### How to Add a New Monster

1. Open `server/src/ro_monster_templates.js`
2. Add a new entry following the exact schema above
3. Use the RO monster ID from the rAthena database
4. Set `aggroRange` > 0 for aggressive monsters (typical: 300-500 UE units)
5. Set `monsterClass` to `'boss'` or `'mvp'` for boss-tier monsters

```javascript
// Example: Adding a custom monster
RO_MONSTER_TEMPLATES['dark_guardian'] = {
    id: 99001,                          // Custom ID (avoid conflicting with RO IDs 1000-9999)
    name: 'Dark Guardian',
    aegisName: 'DARK_GUARDIAN',
    level: 45,
    maxHealth: 8000,
    baseExp: 2500, jobExp: 1200, mvpExp: 0,
    attack: 250, attack2: 310,
    defense: 20, magicDefense: 15,
    str: 40, agi: 50, vit: 30, int: 20, dex: 60, luk: 10,
    attackRange: 50, aggroRange: 400, chaseRange: 800,
    aspd: 170, walkSpeed: 200,
    attackDelay: 1200, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'demon',
    element: { type: 'shadow', level: 2 },
    monsterClass: 'normal',
    aiType: 'aggressive',
    respawnMs: 10000,
    raceGroups: {},
    stats: { str: 40, agi: 50, vit: 30, int: 20, dex: 60, luk: 10, level: 45, weaponATK: 250 },
    modes: {},
    drops: [
        { itemName: 'Shadow Shard', rate: 45 },
        { itemName: 'Elunium', rate: 8 },
        { itemName: 'Dark Guardian Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};
```

---

## 2. Server-Side Spawn Manager

### Spawn Configuration

Enemy spawns are configured per zone in `server/src/ro_zone_data.js` inside the `ZONE_REGISTRY` object. Each zone's `enemySpawns` array defines individual spawn points.

**File**: `server/src/ro_zone_data.js`

```javascript
ZONE_REGISTRY.prontera_south = {
    // ... zone config ...
    enemySpawns: [
        // Each entry spawns ONE enemy instance
        { template: 'poring',   x: 300,  y: 300,  z: 300, wanderRadius: 400 },
        { template: 'poring',   x: -200, y: 400,  z: 300, wanderRadius: 400 },
        { template: 'lunatic',  x: 600,  y: 500,  z: 300, wanderRadius: 350 },
        { template: 'fabre',    x: -600, y: -400, z: 300, wanderRadius: 300 },
        // ... 30 entries total for this zone
    ]
};
```

### Spawn Point Fields

| Field | Type | Description |
|-------|------|-------------|
| `template` | string | Key into `RO_MONSTER_TEMPLATES` (e.g. `'poring'`) |
| `x` | number | Spawn X coordinate (UE units) |
| `y` | number | Spawn Y coordinate (UE units) |
| `z` | number | Spawn Z coordinate (UE units) |
| `wanderRadius` | number | Max distance from spawn point for wandering (UE units) |

### Current Active Zones

| Zone | Key | Monster Level Range | Spawn Count |
|------|-----|-------------------|-------------|
| Prontera South Field | `prontera_south` | 1-15 | ~30 spawns |
| Prontera North Field | `prontera_north` | 10-20 | ~16 spawns |
| Prontera Dungeon 1F | `prt_dungeon_01` | 15-30 | ~15 spawns |

Zones 4-9 (higher level content) have spawn configs defined but are currently disabled.

### The `spawnEnemy()` Function

**File**: `server/src/index.js` (line ~1054)

This is the core function that creates a live enemy instance from a spawn config. It performs:

1. **Template lookup** from `ENEMY_TEMPLATES[spawnConfig.template]`
2. **Unique ID generation** via `nextEnemyId++`
3. **AI code lookup** from `MONSTER_AI_CODES[roId]` (1,004 monster ID to AI code mappings)
4. **Hex mode conversion** via `AI_TYPE_MODES[aiCode]`
5. **Mode flag parsing** via `parseModeFlags(hexMode)` -- produces 18 boolean flags
6. **Boss protocol** auto-applied for `monsterClass === 'boss' || 'mvp'`
7. **Movement speed calculation**: `moveSpeed = (50 / walkSpeed) * 1000` UE units/sec
8. **Enemy object creation** with all stats, position, AI state fields
9. **Wander state initialization** via `initEnemyWanderState()`
10. **Broadcast** `enemy:spawn` to all players in the zone

```javascript
function spawnEnemy(spawnConfig) {
    const template = ENEMY_TEMPLATES[spawnConfig.template];
    if (!template) {
        logger.warn(`[ENEMY] Unknown template '${spawnConfig.template}' -- skipping spawn`);
        return null;
    }
    const enemyId = nextEnemyId++;

    // RO Classic mode flags from AI code lookup
    const roId = template.id || 0;
    const aiCode = (MONSTER_AI_CODES && MONSTER_AI_CODES[roId]) || getDefaultAiCode(template.aiType);
    const hexMode = AI_TYPE_MODES[aiCode] || 0x0081;
    const modeFlags = parseModeFlags(hexMode);

    // Boss/MVP protocol
    if (template.monsterClass === 'boss' || template.monsterClass === 'mvp') {
        modeFlags.knockbackImmune = true;
        modeFlags.statusImmune = true;
        modeFlags.detector = true;
        if (template.monsterClass === 'mvp') modeFlags.mvp = true;
    }

    // Movement speed: walkSpeed = ms per RO cell. 50 UE units = 1 cell.
    const walkSpeedMs = template.walkSpeed || 200;
    const moveSpeed = (50 / walkSpeedMs) * 1000;

    const enemy = {
        enemyId, templateId: spawnConfig.template,
        zone: spawnConfig.zone || 'prontera_south',
        name: template.name, level: template.level,
        health: template.maxHealth, maxHealth: template.maxHealth,
        damage: template.damage, attackRange: template.attackRange,
        aggroRange: template.aggroRange, aspd: template.aspd,
        chaseRange: template.chaseRange || 600,
        exp: template.exp, baseExp: template.baseExp || 0, jobExp: template.jobExp || 0,
        mvpExp: template.mvpExp || 0,
        aiType: template.aiType,
        monsterClass: template.monsterClass || 'normal',
        size: template.size || 'medium',
        race: template.race || 'formless',
        element: template.element || { type: 'neutral', level: 1 },
        attack2: template.attack2 || template.damage,
        hardDef: template.defense || 0,
        magicDefense: template.magicDefense || 0,
        walkSpeed: walkSpeedMs,
        attackDelay: template.attackDelay || 1500,
        attackMotion: template.attackMotion || 500,
        damageMotion: template.damageMotion || 300,
        raceGroups: template.raceGroups || {},
        modes: template.modes || {},
        stats: { ...template.stats }, isDead: false,
        x: spawnConfig.x, y: spawnConfig.y, z: spawnConfig.z,
        spawnX: spawnConfig.x, spawnY: spawnConfig.y, spawnZ: spawnConfig.z,
        wanderRadius: spawnConfig.wanderRadius, respawnMs: template.respawnMs,
        // AI state machine
        aiCode, modeFlags, moveSpeed,
        aiState: AI_STATE.IDLE,
        targetPlayerId: null,
        aggroOriginX: null, aggroOriginY: null,
        lastAttackTime: 0, lastDamageTime: 0, lastAggroScan: 0,
        lastMoveBroadcast: 0, pendingTargetSwitch: null,
        inCombatWith: new Set()
    };

    initEnemyWanderState(enemy);
    enemies.set(enemyId, enemy);

    broadcastToZone(enemy.zone, 'enemy:spawn', {
        enemyId, templateId: spawnConfig.template, name: enemy.name,
        level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
        monsterClass: enemy.monsterClass, size: enemy.size, race: enemy.race,
        element: enemy.element,
        x: enemy.x, y: enemy.y, z: enemy.z
    });
    return enemy;
}
```

### Lazy Spawning

Enemies for a zone are only created when the **first player enters that zone**. This is checked in `player:join` and `zone:change` handlers.

```javascript
// In player:join handler:
const zone = player.zone || 'prontera_south';
const zoneConfig = ZONE_REGISTRY[zone];
if (zoneConfig && zoneConfig.enemySpawns) {
    // Check if any enemies already exist for this zone
    let hasEnemiesInZone = false;
    for (const [, e] of enemies.entries()) {
        if (e.zone === zone) { hasEnemiesInZone = true; break; }
    }
    if (!hasEnemiesInZone) {
        // First player in zone -- spawn all enemies
        for (const spawn of zoneConfig.enemySpawns) {
            spawnEnemy({ ...spawn, zone });
        }
    }
}
```

### Respawn Timer Management

When an enemy dies, its respawn is handled by a `setTimeout()` within `processEnemyDeathFromSkill()` (and the equivalent auto-attack death handler):

```javascript
// Schedule respawn
setTimeout(() => {
    enemy.health = enemy.maxHealth;
    enemy.isDead = false;
    enemy.x = enemy.spawnX;
    enemy.y = enemy.spawnY;
    enemy.z = enemy.spawnZ;
    enemy.targetPlayerId = null;
    enemy.inCombatWith = new Set();
    initEnemyWanderState(enemy);
    broadcastToZone(enemy.zone, 'enemy:spawn', {
        enemyId: enemy.enemyId, templateId: enemy.templateId, name: enemy.name,
        level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
        x: enemy.x, y: enemy.y, z: enemy.z
    });
}, enemy.respawnMs);
```

**Respawn timers by class**:
| Class | Typical `respawnMs` | Notes |
|-------|-------------------|-------|
| Normal | 3,000 - 10,000 ms | Most field/dungeon mobs use 5,500 ms |
| Boss (mini) | 300,000 - 600,000 ms | 5-10 minutes |
| MVP | 3,600,000 - 7,200,000 ms | 60-120 minutes + 10-min variance window |

### Adding Spawn Points to a Zone

To add monsters to a zone, edit the `enemySpawns` array in the zone's `ZONE_REGISTRY` entry:

```javascript
// In server/src/ro_zone_data.js
ZONE_REGISTRY.my_new_zone = {
    name: 'my_new_zone',
    displayName: 'My New Zone',
    type: 'field',
    // ... flags, defaultSpawn, levelName, warps, kafraNpcs ...
    enemySpawns: [
        { template: 'wolf',      x: 500,  y: 300,  z: 300, wanderRadius: 400 },
        { template: 'wolf',      x: -300, y: 600,  z: 300, wanderRadius: 400 },
        { template: 'orc_warrior', x: 800, y: -200, z: 300, wanderRadius: 500 },
    ]
};
```

---

## 3. Server-Side AI State Machine (200ms tick)

### Overview

The enemy AI runs on a **200ms tick** (`setInterval`, 5 cycles per second). Each enemy is in exactly one of four states at any time:

```
              +---------+
              |  DEAD   |<-------- HP reaches 0
              +----+----+
                   |
              respawnMs timer
                   |
                   v
+------+     +---------+     +---------+
| IDLE |---->|  CHASE  |---->| ATTACK  |
+--+---+     +----+----+     +----+----+
   ^              |               |
   |    target    |    target     |
   |    lost      |    moved      |
   +<-------------+    out of     |
   |              |    range      |
   +<-------------+--------------+
```

### Configuration Constants

**File**: `server/src/index.js` (line ~6883)

```javascript
const ENEMY_AI = {
    TICK_MS: 200,               // 5 ticks/second
    WANDER_PAUSE_MIN: 3000,     // 3s min between wanders
    WANDER_PAUSE_MAX: 8000,     // 8s max between wanders
    WANDER_DIST_MIN: 100,       // 100 UE units min wander distance
    WANDER_DIST_MAX: 300,       // 300 UE units max wander distance
    MOVE_BROADCAST_MS: 200,     // Position broadcast rate limit
    AGGRO_SCAN_MS: 500,         // Aggro scan interval for aggressive mobs
    ASSIST_RANGE: 550,          // 11 RO cells assist detection range
    CHASE_GIVE_UP_EXTRA: 200,   // Extra UE units beyond chaseRange before giving up
    IDLE_AFTER_CHASE_MS: 2000,  // Delay before resuming wander after losing target
};
```

### AI State Definitions

```javascript
const AI_STATE = {
    IDLE:    'idle',     // Wandering or standing (scans for aggro if aggressive)
    CHASE:   'chase',    // Moving toward target player
    ATTACK:  'attack',   // In attack range, auto-attacking target
    DEAD:    'dead',     // Dead, awaiting respawn
};
```

### Mode Flag System

Each monster has an AI type code (1-27) from `server/src/ro_monster_ai_codes.js`. The code maps to a hex bitmask that is parsed into 18 boolean flags:

```javascript
// Bitmask constants
const MD = {
    CANMOVE:            0x0001,     LOOTER:             0x0002,
    AGGRESSIVE:         0x0004,     ASSIST:             0x0008,
    CASTSENSORIDLE:     0x0010,     NORANDOMWALK:       0x0020,
    NOCAST:             0x0040,     CANATTACK:          0x0080,
    CASTSENSORCHASE:    0x0200,     CHANGECHASE:        0x0400,
    ANGRY:              0x0800,     CHANGETARGETMELEE:  0x1000,
    CHANGETARGETCHASE:  0x2000,     TARGETWEAK:         0x4000,
    RANDOMTARGET:       0x8000,     MVP:                0x80000,
    KNOCKBACKIMMUNE:    0x200000,   DETECTOR:           0x2000000,
    STATUSIMMUNE:       0x4000000,
};

// AI code to hex mode lookup
const AI_TYPE_MODES = {
    1:  0x0081,  // Passive: CanMove + CanAttack
    2:  0x0083,  // Passive + Looter
    3:  0x1089,  // Passive + Assist + ChangeTargetMelee + CanAttack
    4:  0x3885,  // Angry: Aggressive + Assist + ChangeTargetMelee/Chase + Angry
    5:  0x2085,  // Aggressive + ChangeTargetChase + CanAttack
    6:  0x0000,  // Plant/Immobile: no flags
    7:  0x108B,  // Passive + Looter + Assist + ChangeTargetMelee + CanAttack
    8:  0x7085,  // Aggressive + ChangeTarget(All) + TargetWeak + CanAttack
    9:  0x3095,  // Aggressive + ChangeTarget(Melee/Chase) + CastSensorIdle + CanAttack
    10: 0x0084,  // Aggressive + Immobile (no CanMove)
    13: 0x308D,  // Aggressive + Assist + ChangeTarget(Melee/Chase) + CanAttack
    17: 0x0091,  // Passive + CastSensorIdle + CanAttack
    20: 0x3295,  // Aggressive + ChangeTarget(Melee/Chase) + CastSensor(Idle/Chase)
    21: 0x3695,  // Boss/MVP: Like 20 + ChangeChase
    24: 0x00A1,  // Slave: Passive + NoRandomWalk + CanAttack
    25: 0x0001,  // Pet: CanMove only
    26: 0xB695,  // Aggressive + all ChangeTarget + CastSensor + RandomTarget
    27: 0x8084,  // Aggressive + Immobile + RandomTarget
};

// Parse bitmask to boolean flags
function parseModeFlags(hexMode) {
    return {
        canMove:            !!(hexMode & MD.CANMOVE),
        looter:             !!(hexMode & MD.LOOTER),
        aggressive:         !!(hexMode & MD.AGGRESSIVE),
        assist:             !!(hexMode & MD.ASSIST),
        castSensorIdle:     !!(hexMode & MD.CASTSENSORIDLE),
        noRandomWalk:       !!(hexMode & MD.NORANDOMWALK),
        canAttack:          !!(hexMode & MD.CANATTACK),
        castSensorChase:    !!(hexMode & MD.CASTSENSORCHASE),
        changeChase:        !!(hexMode & MD.CHANGECHASE),
        angry:              !!(hexMode & MD.ANGRY),
        changeTargetMelee:  !!(hexMode & MD.CHANGETARGETMELEE),
        changeTargetChase:  !!(hexMode & MD.CHANGETARGETCHASE),
        targetWeak:         !!(hexMode & MD.TARGETWEAK),
        randomTarget:       !!(hexMode & MD.RANDOMTARGET),
        mvp:                !!(hexMode & MD.MVP),
        knockbackImmune:    !!(hexMode & MD.KNOCKBACKIMMUNE),
        detector:           !!(hexMode & MD.DETECTOR),
        statusImmune:       !!(hexMode & MD.STATUSIMMUNE),
    };
}
```

### Mode Flag Behavior Summary

| Flag | Effect |
|------|--------|
| `canMove` | Monster can move and chase. Without this, rooted in place. |
| `aggressive` | Auto-scans for players within `aggroRange` to attack. |
| `assist` | When a nearby same-type mob is attacked, joins the fight. |
| `changeTargetMelee` | Switches target when hit by melee in ATTACK/IDLE state. |
| `changeTargetChase` | Switches target when hit during CHASE state. |
| `changeChase` | While chasing, switches to a closer player within attack range. |
| `randomTarget` | Picks a random combatant from `inCombatWith` each swing. |
| `targetWeak` | Only aggros players 5+ levels below the monster. |
| `castSensorIdle` | Detects players casting skills while monster is idle. |
| `castSensorChase` | Detects casters even while in chase state. |
| `noRandomWalk` | Disables random wandering. Stands still unless chasing. |
| `looter` | Picks up items on the ground during idle (future). |
| `angry` | Hyper-active pre-attack states (follow/angry). |
| `canAttack` | Can perform attacks. Without this, purely passive (e.g., Pupa). |
| `knockbackImmune` | Cannot be displaced by skills. Auto-set for boss/mvp. |
| `statusImmune` | Immune to status effects. Auto-set for boss/mvp. |
| `detector` | Can see hidden/cloaked players. Auto-set for boss/mvp. |
| `mvp` | MVP protocol (tombstone, MVP rewards). |

### IDLE State Logic

```javascript
case AI_STATE.IDLE: {
    // 1. Aggressive mobs: scan for players every 500ms
    if (enemy.modeFlags.aggressive && enemy.aggroRange > 0) {
        if (now - (enemy.lastAggroScan || 0) >= ENEMY_AI.AGGRO_SCAN_MS) {
            enemy.lastAggroScan = now;
            const targetCharId = findAggroTarget(enemy);
            if (targetCharId) {
                enemy.targetPlayerId = targetCharId;
                enemy.aggroOriginX = enemy.x;
                enemy.aggroOriginY = enemy.y;
                enemy.isWandering = false;
                enemy.inCombatWith.add(targetCharId);
                if (enemy.modeFlags.canMove) {
                    enemy.aiState = AI_STATE.CHASE;
                } else {
                    enemy.aiState = AI_STATE.ATTACK;  // Immobile turrets
                }
                break;
            }
        }
    }
    // 2. No aggro target: wander normally (skip if in hit stun)
    if (!inHitStun) {
        processWander(enemy, now);
    }
    break;
}
```

### CHASE State Logic

```javascript
case AI_STATE.CHASE: {
    // 1. Validate target still alive/connected
    const target = connectedPlayers.get(enemy.targetPlayerId);
    if (!target || target.isDead) {
        // Pick next target from inCombatWith, or go IDLE
        const nextTarget = pickNextTarget(enemy);
        if (nextTarget) enemy.targetPlayerId = nextTarget;
        else { /* return to IDLE */ }
        break;
    }

    // 2. Process pending target switch (from damage hooks)
    if (enemy.pendingTargetSwitch) {
        enemy.targetPlayerId = enemy.pendingTargetSwitch.charId;
        enemy.pendingTargetSwitch = null;
    }

    // 3. Chase range check -- give up if too far from aggro origin
    const distFromOrigin = distance(enemy, enemy.aggroOrigin);
    if (distFromOrigin > enemy.chaseRange + ENEMY_AI.CHASE_GIVE_UP_EXTRA) {
        // Give up: IDLE, clear inCombatWith
        break;
    }

    // 4. Attack range check -- transition to ATTACK if close enough
    const distToTarget = distance(enemy, targetPos);
    if (distToTarget <= enemy.attackRange + COMBAT.RANGE_TOLERANCE) {
        enemy.aiState = AI_STATE.ATTACK;
        enemyStopMoving(enemy);
        break;
    }

    // 5. Move toward target (skip if hit-stunned)
    if (!inHitStun) {
        enemyMoveToward(enemy, targetPos.x, targetPos.y, now, enemy.moveSpeed);
    }

    // 6. ChangeChase: while chasing, switch to a closer combatant in attack range
    if (enemy.modeFlags.changeChase) {
        for (const charId of enemy.inCombatWith) {
            if (charId === enemy.targetPlayerId) continue;
            const dist2 = distance(enemy, getPlayerPosSync(charId));
            if (dist2 <= enemy.attackRange + COMBAT.RANGE_TOLERANCE) {
                enemy.targetPlayerId = charId;
                break;
            }
        }
    }
    break;
}
```

### ATTACK State Logic

```javascript
case AI_STATE.ATTACK: {
    // 1. Validate target
    const atkTarget = connectedPlayers.get(enemy.targetPlayerId);
    if (!atkTarget || atkTarget.isDead) {
        // Pick next target or go IDLE
        break;
    }

    // 2. Process pending target switch
    // 3. RandomTarget: pick random combatant each swing
    if (enemy.modeFlags.randomTarget && enemy.inCombatWith.size > 1) {
        const combatants = [...enemy.inCombatWith].filter(id => {
            const p = connectedPlayers.get(id);
            return p && !p.isDead;
        });
        if (combatants.length > 0) {
            enemy.targetPlayerId = combatants[Math.floor(Math.random() * combatants.length)];
        }
    }

    // 4. Range check: if target moved out of range, back to CHASE
    if (distToTarget > enemy.attackRange + COMBAT.RANGE_TOLERANCE + 30) {
        enemy.aiState = AI_STATE.CHASE;
        break;
    }

    // 5. Hit stun check
    if (inHitStun) break;

    // 6. Attack timing check
    if (now - enemy.lastAttackTime < enemy.attackDelay) break;

    // 7. EXECUTE ATTACK
    enemy.lastAttackTime = now;
    const combatResult = calculateEnemyDamage(enemy, enemy.targetPlayerId);
    // ... apply damage, broadcast combat:damage + enemy:attack ...
    // ... check player death, pick next target ...
    break;
}
```

### Aggro System

The aggro system is the heart of the AI. Four interconnected functions handle all aggro logic:

#### `setEnemyAggro(enemy, attackerCharId, hitType)`

Called from ALL damage paths -- auto-attack, all 9 skill types, ground effects.

```javascript
function setEnemyAggro(enemy, attackerCharId, hitType) {
    if (enemy.isDead) return;
    if (!enemy.modeFlags.canAttack) return;  // Plant-type: can't fight back

    enemy.inCombatWith.add(attackerCharId);
    enemy.isWandering = false;
    enemy.lastDamageTime = Date.now();

    if (!enemy.targetPlayerId || enemy.aiState === AI_STATE.IDLE
        || shouldSwitchTarget(enemy, attackerCharId, hitType)) {
        enemy.targetPlayerId = attackerCharId;
        if (enemy.aiState === AI_STATE.IDLE) {
            enemy.aggroOriginX = enemy.x;
            enemy.aggroOriginY = enemy.y;
        }
        if (enemy.modeFlags.canMove) {
            enemy.aiState = AI_STATE.CHASE;
        } else {
            enemy.aiState = AI_STATE.ATTACK;
        }
    }

    triggerAssist(enemy, attackerCharId);
}
```

#### `shouldSwitchTarget(enemy, newAttackerCharId, hitType)`

Mode-flag-based target switching decision:

```javascript
function shouldSwitchTarget(enemy, newAttackerCharId, hitType) {
    if (!enemy.targetPlayerId) return true;
    if (enemy.targetPlayerId === newAttackerCharId) return false;
    if (enemy.modeFlags.randomTarget) return true;

    if (enemy.aiState === AI_STATE.ATTACK || enemy.aiState === AI_STATE.IDLE) {
        if (enemy.modeFlags.changeTargetMelee && hitType === 'melee') return true;
    }
    if (enemy.aiState === AI_STATE.CHASE) {
        if (enemy.modeFlags.changeTargetChase) return true;
    }
    return false;
}
```

#### `triggerAssist(attackedEnemy, attackerCharId)`

Alerts nearby same-type mobs when one is attacked:

```javascript
function triggerAssist(attackedEnemy, attackerCharId) {
    for (const [, other] of enemies.entries()) {
        if (other === attackedEnemy) continue;
        if (other.isDead || other.aiState !== AI_STATE.IDLE) continue;
        if (other.zone !== attackedEnemy.zone) continue;
        if (other.templateId !== attackedEnemy.templateId) continue;
        if (!other.modeFlags.assist) continue;
        if (!other.modeFlags.canAttack || !other.modeFlags.canMove) continue;

        const dist = distance(other, attackedEnemy);
        if (dist <= ENEMY_AI.ASSIST_RANGE) {  // 550 UE units = 11 RO cells
            other.targetPlayerId = attackerCharId;
            other.aiState = AI_STATE.CHASE;
            other.aggroOriginX = other.x;
            other.aggroOriginY = other.y;
            other.isWandering = false;
            other.inCombatWith.add(attackerCharId);
        }
    }
}
```

#### `findAggroTarget(enemy)`

Scans for closest player for aggressive mobs:

```javascript
function findAggroTarget(enemy) {
    let closestDist = Infinity;
    let closestCharId = null;

    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if (player.zone !== enemy.zone) continue;

        const px = player.lastX, py = player.lastY;
        if (px === undefined || py === undefined) continue;

        const dist = distance(enemy, { x: px, y: py });
        if (dist > enemy.aggroRange) continue;

        // TargetWeak: only aggro players 5+ levels below monster
        if (enemy.modeFlags.targetWeak) {
            const playerLevel = (player.stats && player.stats.level) || 1;
            if (playerLevel >= enemy.level - 5) continue;
        }

        if (dist < closestDist) {
            closestDist = dist;
            closestCharId = charId;
        }
    }
    return closestCharId;
}
```

### Wander System

```javascript
function processWander(enemy, now) {
    if (!enemy.modeFlags.canMove || enemy.modeFlags.noRandomWalk) return;

    if (!enemy.isWandering) {
        if (now >= enemy.nextWanderTime) {
            const target = pickRandomWanderPoint(enemy);
            enemy.wanderTargetX = target.x;
            enemy.wanderTargetY = target.y;
            enemy.isWandering = true;
        }
    } else {
        const dist = distance(enemy, { x: enemy.wanderTargetX, y: enemy.wanderTargetY });
        if (dist < 10) {
            // Arrived: stop, schedule next wander
            enemy.isWandering = false;
            enemy.nextWanderTime = now + randomBetween(WANDER_PAUSE_MIN, WANDER_PAUSE_MAX);
            enemyStopMoving(enemy);
        } else {
            // Walk at 60% chase speed
            enemyMoveToward(enemy, enemy.wanderTargetX, enemy.wanderTargetY, now, enemy.moveSpeed * 0.6);
        }
    }
}
```

**Key wander rules**:
- Only active in IDLE state with no target
- Disabled for plants (`!canMove`) and mobs with `noRandomWalk` flag
- Random point within 100-300 UE units of current position
- Clamped to `wanderRadius` from spawn point (prevents infinite drift)
- Walk at 60% of chase speed

---

## 4. Server-Side Monster Combat

### Monster Attack Processing

Monsters attack during the ATTACK state within the 200ms AI tick. The attack timing is governed by `attackDelay` (ms between attacks) and gated by `lastAttackTime`.

The execution flow:
1. Check `now - lastAttackTime >= attackDelay`
2. Call `calculateEnemyDamage(enemy, targetCharId)`
3. Apply damage to player's `health`
4. Broadcast `combat:damage` (for DamageNumberSubsystem)
5. Broadcast `enemy:attack` (for client attack animation)
6. Broadcast `combat:health_update` (for health bars)
7. If player HP <= 0: broadcast `combat:death`, save to DB

### Enemy Damage Calculation

The server uses the same `roPhysicalDamage()` formula for both player-to-enemy and enemy-to-player combat:

```javascript
function calculateEnemyDamage(enemy, targetCharId) {
    const player = connectedPlayers.get(targetCharId);
    if (!player) return null;

    const attackerStats = {
        str: (enemy.stats && enemy.stats.str) || 1,
        agi: (enemy.stats && enemy.stats.agi) || 1,
        vit: (enemy.stats && enemy.stats.vit) || 0,
        int: (enemy.stats && enemy.stats.int) || 0,
        dex: (enemy.stats && enemy.stats.dex) || 1,
        luk: (enemy.stats && enemy.stats.luk) || 1,
        level: enemy.level,
        weaponATK: enemy.damage || 1,
        passiveATK: 0,
    };

    const targetInfo = getPlayerTargetInfo(player, targetCharId);
    const attackerInfo = {
        weaponType: (enemy.attackRange > COMBAT.MELEE_RANGE + COMBAT.RANGE_TOLERANCE) ? 'bow' : 'bare_hand',
        weaponElement: (enemy.element && enemy.element.type) || 'neutral',
        weaponLevel: 1,
        buffMods: getBuffStatModifiers(enemy),
        cardMods: null,
    };

    return calculatePhysicalDamage(attackerStats, getEffectiveStats(player), player.hardDef || 0, targetInfo, attackerInfo);
}
```

### Hit Stun System

When an enemy takes damage, `enemy.lastDamageTime` is set to `Date.now()`. The AI tick calculates:

```javascript
const inHitStun = (now - (enemy.lastDamageTime || 0)) < (enemy.damageMotion || 300);
```

During hit stun:
- Movement is paused (no `enemyMoveToward()` calls)
- Attacks are paused
- State checks (target validation, range checks) continue normally
- State transitions can still occur

### Death Processing

When a monster's HP reaches 0 (from either auto-attack or skill damage):

```javascript
async function processEnemyDeathFromSkill(enemy, attacker, attackerId, io) {
    // 1. Mark dead
    enemy.isDead = true;
    enemy.aiState = AI_STATE.DEAD;
    enemy.targetPlayerId = null;

    // 2. Stop all auto-attackers targeting this enemy
    for (const [otherId, otherAtk] of autoAttackState.entries()) {
        if (otherAtk.isEnemy && otherAtk.targetCharId === enemy.enemyId) {
            autoAttackState.delete(otherId);
        }
    }
    enemy.inCombatWith.clear();

    // 3. Award EXP to killer
    const baseExpReward = enemy.baseExp || 0;
    const jobExpReward = enemy.jobExp || 0;
    const expResult = processExpGain(attacker, baseExpReward, jobExpReward);

    // 4. Emit exp:gain + exp:level_up events
    // 5. Broadcast enemy:death to zone
    broadcastToZone(enemyZone, 'enemy:death', {
        enemyId: enemy.enemyId, enemyName: enemy.name,
        killerId: attackerId, killerName: attacker.characterName,
        isEnemy: true, isDead: true,
        baseExp: baseExpReward, jobExp: jobExpReward,
        timestamp: Date.now()
    });

    // 6. Roll and award loot
    const droppedItems = rollEnemyDrops(enemy);
    // ... add items to killer's inventory, emit loot:drop + inventory:data ...

    // 7. Schedule respawn
    setTimeout(() => { /* reset and re-broadcast enemy:spawn */ }, enemy.respawnMs);
}
```

### Monster Skills (Not Yet Implemented)

Monster skills from `mob_skill_db` are NOT yet implemented in Sabri_MMO. When implemented, they will follow this structure:

**Trigger conditions**: HP threshold, timer, on attack, on being attacked, slave count
**Skill types**: Offensive (Bolt spells, AoE), Defensive (Heal, Barrier), Summoning (slaves), Utility (Teleport, Transform)
**Rate per tick**: 500-2000 (5-20%) probability

This is a future feature -- see `docsNew/05_Development/` for planning docs.

---

## 5. Server-Side Drop System

### Drop Table Structure

Each monster template has a `drops` array (max 10 slots) and optionally `mvpDrops` (max 3 slots for MVPs):

```javascript
drops: [
    { itemName: 'Jellopy', rate: 70 },               // 70% chance
    { itemName: 'Knife', rate: 1 },                    // 1% chance
    { itemName: 'Sticky Mucus', rate: 4 },             // 4% chance
    { itemName: 'Poring Card', rate: 0.01, stealProtected: true }, // 0.01% (1 in 10,000)
],
mvpDrops: [
    { itemName: 'Old Blue Box', rate: 40 },
    { itemName: 'Yggdrasil Seed', rate: 30 },
],
```

### `rollEnemyDrops(enemy)` Function

```javascript
function rollEnemyDrops(enemy) {
    const template = ENEMY_TEMPLATES[enemy.templateId];
    if (!template || !template.drops) return [];

    const droppedItems = [];
    for (const drop of template.drops) {
        if (Math.random() < drop.chance) {
            const qty = drop.minQty && drop.maxQty
                ? drop.minQty + Math.floor(Math.random() * (drop.maxQty - drop.minQty + 1))
                : 1;
            const itemId = drop.itemId;
            const itemDef = itemId ? itemDefinitions.get(itemId) : null;
            if (itemId) {
                droppedItems.push({ itemId, quantity: qty, itemName: drop.itemName || itemDef?.name });
            }
        }
    }

    // Also roll MVP drops for MVP monsters
    if (template.mvpDrops?.length > 0 && enemy.monsterClass === 'mvp') {
        for (const drop of template.mvpDrops) {
            if (Math.random() < drop.chance) {
                droppedItems.push({ itemId: drop.itemId, quantity: 1, itemName: drop.itemName, isMvpDrop: true });
            }
        }
    }
    return droppedItems;
}
```

### Drop Rate Categories

| Category | Rate Range | Examples |
|----------|-----------|---------|
| Common | 50-100% | Jellopy, Herbs, basic materials |
| Uncommon | 10-49% | Oridecon, Elunium |
| Rare | 1-9.99% | Equipment, valuable materials |
| Very Rare | 0.01-0.99% | High-end equipment |
| **Cards** | **0.01%** | Every monster's card (1 in 10,000 kills) |
| MVP Slot 1 | 30-60% | Consumables, boxes |
| MVP Slot 2 | 10-55% | Mix of consumables/materials |
| MVP Slot 3 | 5-55% | Rarest MVP reward |

### Loot Distribution

Currently, loot goes directly into the killer's inventory. The server emits:

```javascript
killerSocket.emit('loot:drop', {
    enemyId: enemy.enemyId,
    enemyName: enemy.name,
    items: [
        { itemId: 1001, itemName: 'Jellopy', quantity: 3, icon: 'item_jellopy', itemType: 'etc' },
        // ...
    ]
});
// Followed by updated inventory
killerSocket.emit('inventory:data', { items: killerInventory, zuzucoin: attacker.zuzucoin });
```

### Future: Ground Drops and Pickup

When ground drops are implemented:
- Items drop at the enemy's death position
- Killer has priority pickup for 10 seconds
- After 10s, any player can pick up
- Items despawn after 60 seconds if not picked up
- Party loot rules (Each Take / Party Share)

---

## 6. Client-Side: Enemy Manager (UE5 C++)

### Architecture Overview

The client-side enemy system follows the project's **manager pattern** (one manager per domain) and **UWorldSubsystem** paradigm. The system is entirely data-driven from the server -- the client never decides where to spawn enemies or what their stats are.

**Key principle**: The client is presentation + input only. All enemy state is authoritative on the server.

### Files to Create

| File | Class | Role |
|------|-------|------|
| `Enemy/EnemySubsystem.h/.cpp` | `UEnemySubsystem` | UWorldSubsystem. Receives all `enemy:*` socket events. Manages `TMap<int32, FEnemyInstanceData>`. Spawns/despawns `AMMOEnemy` actors. |
| `Enemy/MMOEnemy.h/.cpp` | `AMMOEnemy` | Individual enemy actor. Skeletal mesh, capsule, nameplate. Receives position updates and interpolates movement. |
| `Enemy/EnemyInterpolationComponent.h/.cpp` | `UEnemyInterpolationComponent` | UActorComponent. Smooth movement interpolation from server positions. |
| `CharacterData.h` (extend) | `FEnemyInstanceData` | USTRUCT with all per-enemy client-side state. |

### Data Structures

Add to `CharacterData.h`:

```cpp
// Per-enemy data received from the server
USTRUCT(BlueprintType)
struct FEnemyInstanceData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 EnemyId = 0;
    UPROPERTY(BlueprintReadOnly) FString TemplateId;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(BlueprintReadOnly) int32 CurrentHP = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxHP = 1;
    UPROPERTY(BlueprintReadOnly) FString MonsterClass; // "normal" / "boss" / "mvp"
    UPROPERTY(BlueprintReadOnly) FString Size;         // "small" / "medium" / "large"
    UPROPERTY(BlueprintReadOnly) FString Race;
    UPROPERTY(BlueprintReadOnly) FString ElementType;
    UPROPERTY(BlueprintReadOnly) int32 ElementLevel = 1;
    UPROPERTY(BlueprintReadOnly) FVector Position = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector TargetPosition = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) bool bIsMoving = false;
    UPROPERTY(BlueprintReadOnly) bool bIsDead = false;
    UPROPERTY(BlueprintReadOnly) bool bInCombat = false;

    // Cached actor reference (set when the enemy actor is spawned in the world)
    TWeakObjectPtr<AActor> CachedActor;
};
```

### UEnemySubsystem -- The Central Manager

```cpp
// Enemy/EnemySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterData.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "EnemySubsystem.generated.h"

class USocketIOClientComponent;
class AMMOEnemy;

// Delegate fired when any enemy's data changes (for UI binding)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDataChanged, int32, EnemyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied, int32, EnemyId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, int32, EnemyId);

UCLASS()
class SABRIMMO_API UEnemySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Enemy Data (keyed by EnemyId from server) ----
    UPROPERTY(BlueprintReadOnly)
    TMap<int32, FEnemyInstanceData> EnemyDataMap;

    // ---- Actor References (keyed by EnemyId) ----
    TMap<int32, TWeakObjectPtr<AMMOEnemy>> EnemyActorMap;

    // ---- Delegates ----
    UPROPERTY(BlueprintAssignable) FOnEnemySpawned OnEnemySpawned;
    UPROPERTY(BlueprintAssignable) FOnEnemyDied OnEnemyDied;
    UPROPERTY(BlueprintAssignable) FOnEnemyDataChanged OnEnemyDataChanged;

    // ---- Blueprint Configuration ----
    // The enemy actor class to spawn. Set via DefaultGame.ini or hardcoded.
    UPROPERTY(EditDefaultsOnly, Category = "Enemy")
    TSubclassOf<AMMOEnemy> EnemyActorClass;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public API ----
    UFUNCTION(BlueprintCallable, Category = "Enemy")
    FEnemyInstanceData GetEnemyData(int32 EnemyId) const;

    UFUNCTION(BlueprintCallable, Category = "Enemy")
    AMMOEnemy* GetEnemyActor(int32 EnemyId) const;

    // Request all existing enemies in current zone (called after zone load)
    void RequestExistingEnemies();

private:
    // ---- Socket Event Wrapping ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> Handler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event Handlers ----
    void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);

    // ---- Actor Management ----
    AMMOEnemy* SpawnEnemyActor(const FEnemyInstanceData& Data);
    void DespawnEnemyActor(int32 EnemyId);
    void DespawnAllEnemies();

    bool bSocketsBound = false;
};
```

### UEnemySubsystem Implementation

```cpp
// Enemy/EnemySubsystem.cpp
#include "EnemySubsystem.h"
#include "MMOEnemy.h"
#include "SocketIOClientComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UEnemySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    // Only create in game levels (detect by presence of SocketManager)
    // Same pattern as other subsystems
    return true;
}

void UEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    // Delay socket binding to next frame (socket component may not be ready)
    InWorld.GetTimerManager().SetTimerForNextTick([this]() {
        TryWrapSocketEvents();
    });
}

void UEnemySubsystem::Deinitialize()
{
    DespawnAllEnemies();
    EnemyDataMap.Empty();
    EnemyActorMap.Empty();
    Super::Deinitialize();
}

USocketIOClientComponent* UEnemySubsystem::FindSocketIOComponent() const
{
    // Same pattern as BasicInfoSubsystem, WorldHealthBarSubsystem, etc.
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>();
        if (Comp) return Comp;
    }
    return nullptr;
}

void UEnemySubsystem::TryWrapSocketEvents()
{
    if (bSocketsBound) return;

    WrapSingleEvent(TEXT("enemy:spawn"),
        [this](const TSharedPtr<FJsonValue>& Data) { HandleEnemySpawn(Data); });
    WrapSingleEvent(TEXT("enemy:move"),
        [this](const TSharedPtr<FJsonValue>& Data) { HandleEnemyMove(Data); });
    WrapSingleEvent(TEXT("enemy:attack"),
        [this](const TSharedPtr<FJsonValue>& Data) { HandleEnemyAttack(Data); });
    WrapSingleEvent(TEXT("enemy:death"),
        [this](const TSharedPtr<FJsonValue>& Data) { HandleEnemyDeath(Data); });
    WrapSingleEvent(TEXT("enemy:health_update"),
        [this](const TSharedPtr<FJsonValue>& Data) { HandleEnemyHealthUpdate(Data); });

    bSocketsBound = true;
}

void UEnemySubsystem::WrapSingleEvent(const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
{
    USocketIOClientComponent* Socket = FindSocketIOComponent();
    if (!Socket) return;

    // Chain with existing listeners (same pattern as WorldHealthBarSubsystem)
    Socket->OnNativeEvent.AddLambda([this, EventName, Handler](const FString& Event, const TSharedPtr<FJsonValue>& Data)
    {
        if (Event == EventName)
        {
            Handler(Data);
        }
    });
}

// ---- Event Handlers ----

void UEnemySubsystem::HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    FEnemyInstanceData EnemyData;
    EnemyData.EnemyId = (int32)Obj->GetNumberField(TEXT("enemyId"));
    EnemyData.TemplateId = Obj->GetStringField(TEXT("templateId"));
    EnemyData.Name = Obj->GetStringField(TEXT("name"));
    EnemyData.Level = (int32)Obj->GetNumberField(TEXT("level"));
    EnemyData.CurrentHP = (int32)Obj->GetNumberField(TEXT("health"));
    EnemyData.MaxHP = (int32)Obj->GetNumberField(TEXT("maxHealth"));
    EnemyData.Position.X = Obj->GetNumberField(TEXT("x"));
    EnemyData.Position.Y = Obj->GetNumberField(TEXT("y"));
    EnemyData.Position.Z = Obj->GetNumberField(TEXT("z"));
    EnemyData.bIsDead = false;
    EnemyData.bIsMoving = false;

    // Optional fields
    if (Obj->HasField(TEXT("monsterClass")))
        EnemyData.MonsterClass = Obj->GetStringField(TEXT("monsterClass"));
    if (Obj->HasField(TEXT("size")))
        EnemyData.Size = Obj->GetStringField(TEXT("size"));
    if (Obj->HasField(TEXT("race")))
        EnemyData.Race = Obj->GetStringField(TEXT("race"));

    // Element
    const TSharedPtr<FJsonObject>* ElementObj;
    if (Obj->TryGetObjectField(TEXT("element"), ElementObj))
    {
        EnemyData.ElementType = (*ElementObj)->GetStringField(TEXT("type"));
        EnemyData.ElementLevel = (int32)(*ElementObj)->GetNumberField(TEXT("level"));
    }

    // Store data
    EnemyDataMap.Add(EnemyData.EnemyId, EnemyData);

    // Spawn or re-use actor
    if (EnemyActorMap.Contains(EnemyData.EnemyId))
    {
        AMMOEnemy* Existing = EnemyActorMap[EnemyData.EnemyId].Get();
        if (Existing)
        {
            // Re-spawning after death -- teleport to new position, reset state
            Existing->SetActorLocation(EnemyData.Position);
            Existing->ReinitializeFromData(EnemyData);
            Existing->SetActorHiddenInGame(false);
            Existing->SetActorEnableCollision(true);
        }
        else
        {
            EnemyActorMap.Remove(EnemyData.EnemyId);
            SpawnEnemyActor(EnemyData);
        }
    }
    else
    {
        SpawnEnemyActor(EnemyData);
    }

    OnEnemySpawned.Broadcast(EnemyData.EnemyId);
}

void UEnemySubsystem::HandleEnemyMove(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    const int32 EnemyId = (int32)Obj->GetNumberField(TEXT("enemyId"));
    FEnemyInstanceData* EnemyData = EnemyDataMap.Find(EnemyId);
    if (!EnemyData) return;

    EnemyData->Position.X = Obj->GetNumberField(TEXT("x"));
    EnemyData->Position.Y = Obj->GetNumberField(TEXT("y"));
    EnemyData->Position.Z = Obj->GetNumberField(TEXT("z"));
    EnemyData->bIsMoving = Obj->GetBoolField(TEXT("isMoving"));

    if (Obj->HasField(TEXT("targetX")))
    {
        EnemyData->TargetPosition.X = Obj->GetNumberField(TEXT("targetX"));
        EnemyData->TargetPosition.Y = Obj->GetNumberField(TEXT("targetY"));
    }

    // Update actor interpolation
    AMMOEnemy* Actor = GetEnemyActor(EnemyId);
    if (Actor)
    {
        Actor->UpdateServerPosition(EnemyData->Position, EnemyData->TargetPosition, EnemyData->bIsMoving);
    }
}

void UEnemySubsystem::HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    const int32 EnemyId = (int32)Obj->GetNumberField(TEXT("enemyId"));
    const int32 TargetId = (int32)Obj->GetNumberField(TEXT("targetId"));
    const int32 AttackMotion = (int32)Obj->GetNumberField(TEXT("attackMotion"));

    AMMOEnemy* Actor = GetEnemyActor(EnemyId);
    if (Actor)
    {
        Actor->PlayAttackAnimation(AttackMotion);
    }
}

void UEnemySubsystem::HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    const int32 EnemyId = (int32)Obj->GetNumberField(TEXT("enemyId"));
    FEnemyInstanceData* EnemyData = EnemyDataMap.Find(EnemyId);
    if (EnemyData)
    {
        EnemyData->bIsDead = true;
        EnemyData->CurrentHP = 0;
        EnemyData->bInCombat = false;
    }

    AMMOEnemy* Actor = GetEnemyActor(EnemyId);
    if (Actor)
    {
        Actor->PlayDeathSequence();
    }

    OnEnemyDied.Broadcast(EnemyId);
}

void UEnemySubsystem::HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
    const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    const int32 EnemyId = (int32)Obj->GetNumberField(TEXT("enemyId"));
    FEnemyInstanceData* EnemyData = EnemyDataMap.Find(EnemyId);
    if (!EnemyData) return;

    EnemyData->CurrentHP = (int32)Obj->GetNumberField(TEXT("health"));
    EnemyData->MaxHP = (int32)Obj->GetNumberField(TEXT("maxHealth"));
    if (Obj->HasField(TEXT("inCombat")))
        EnemyData->bInCombat = Obj->GetBoolField(TEXT("inCombat"));

    OnEnemyDataChanged.Broadcast(EnemyId);
}

// ---- Actor Management ----

AMMOEnemy* UEnemySubsystem::SpawnEnemyActor(const FEnemyInstanceData& Data)
{
    UWorld* World = GetWorld();
    if (!World || !EnemyActorClass) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMMOEnemy* NewEnemy = World->SpawnActor<AMMOEnemy>(
        EnemyActorClass, Data.Position, FRotator::ZeroRotator, SpawnParams);

    if (NewEnemy)
    {
        NewEnemy->InitializeFromData(Data);
        EnemyActorMap.Add(Data.EnemyId, NewEnemy);
        // Update the CachedActor in the data map for position lookups
        FEnemyInstanceData* StoredData = EnemyDataMap.Find(Data.EnemyId);
        if (StoredData) StoredData->CachedActor = NewEnemy;
    }
    return NewEnemy;
}

void UEnemySubsystem::DespawnEnemyActor(int32 EnemyId)
{
    if (TWeakObjectPtr<AMMOEnemy>* Found = EnemyActorMap.Find(EnemyId))
    {
        if (AMMOEnemy* Actor = Found->Get())
        {
            Actor->Destroy();
        }
        EnemyActorMap.Remove(EnemyId);
    }
}

void UEnemySubsystem::DespawnAllEnemies()
{
    for (auto& Pair : EnemyActorMap)
    {
        if (AMMOEnemy* Actor = Pair.Value.Get())
        {
            Actor->Destroy();
        }
    }
    EnemyActorMap.Empty();
}

FEnemyInstanceData UEnemySubsystem::GetEnemyData(int32 EnemyId) const
{
    const FEnemyInstanceData* Found = EnemyDataMap.Find(EnemyId);
    return Found ? *Found : FEnemyInstanceData();
}

AMMOEnemy* UEnemySubsystem::GetEnemyActor(int32 EnemyId) const
{
    const TWeakObjectPtr<AMMOEnemy>* Found = EnemyActorMap.Find(EnemyId);
    return Found ? Found->Get() : nullptr;
}
```

### AMMOEnemy -- Enemy Actor

```cpp
// Enemy/MMOEnemy.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CharacterData.h"
#include "MMOEnemy.generated.h"

class UWidgetComponent;

UCLASS()
class SABRIMMO_API AMMOEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    AMMOEnemy();

    // ---- Server Data ----
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") int32 EnemyId = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") FString EnemyName;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") int32 Level = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") int32 CurrentHP = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") int32 MaxHP = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") FString MonsterClass;
    UPROPERTY(BlueprintReadOnly, Category = "Enemy") bool bIsDead = false;

    // ---- Initialization ----
    void InitializeFromData(const FEnemyInstanceData& Data);
    void ReinitializeFromData(const FEnemyInstanceData& Data);

    // ---- Movement Interpolation ----
    void UpdateServerPosition(const FVector& ServerPos, const FVector& ServerTarget, bool bMoving);

    // ---- Animations ----
    void PlayAttackAnimation(int32 AttackMotionMs);
    void PlayDeathSequence();
    void PlayReceiveHitAnimation();

    // ---- Click Targeting (for skill system) ----
    // Implements BPI_Targetable via Blueprint interface if needed.

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // ---- Nameplate ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* NameplateComponent;

    // ---- Interpolation State ----
    FVector InterpolationTarget = FVector::ZeroVector;
    FVector InterpolationVelocity = FVector::ZeroVector;
    bool bIsInterpolating = false;
    float InterpSpeed = 5.0f;

    // ---- Death ----
    FTimerHandle DeathDespawnTimer;
    float DeathDespawnDelay = 3.0f; // Seconds before hiding after death animation

    void OnDeathDespawnTimerExpired();
};
```

### AMMOEnemy Implementation

```cpp
// Enemy/MMOEnemy.cpp
#include "MMOEnemy.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AMMOEnemy::AMMOEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    // Capsule sizing (adjustable per monster via Blueprint children)
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

    // Disable CharacterMovement's default movement (server-authoritative)
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (MoveComp)
    {
        MoveComp->GravityScale = 1.0f;
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    }

    // Nameplate widget
    NameplateComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Nameplate"));
    NameplateComponent->SetupAttachment(RootComponent);
    NameplateComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    NameplateComponent->SetWidgetSpace(EWidgetSpace::Screen);
    NameplateComponent->SetDrawSize(FVector2D(200.0f, 30.0f));
}

void AMMOEnemy::BeginPlay()
{
    Super::BeginPlay();
}

void AMMOEnemy::InitializeFromData(const FEnemyInstanceData& Data)
{
    EnemyId = Data.EnemyId;
    EnemyName = Data.Name;
    Level = Data.Level;
    CurrentHP = Data.CurrentHP;
    MaxHP = Data.MaxHP;
    MonsterClass = Data.MonsterClass;
    bIsDead = false;

    SetActorLocation(Data.Position);
    InterpolationTarget = Data.Position;
}

void AMMOEnemy::ReinitializeFromData(const FEnemyInstanceData& Data)
{
    // Called on respawn -- reset all state
    CurrentHP = Data.CurrentHP;
    MaxHP = Data.MaxHP;
    bIsDead = false;
    SetActorLocation(Data.Position);
    InterpolationTarget = Data.Position;
    bIsInterpolating = false;

    // Clear death timer
    GetWorldTimerManager().ClearTimer(DeathDespawnTimer);
}

void AMMOEnemy::UpdateServerPosition(const FVector& ServerPos, const FVector& ServerTarget, bool bMoving)
{
    if (bIsDead) return;

    if (bMoving)
    {
        InterpolationTarget = ServerPos;
        bIsInterpolating = true;

        // Face movement direction
        FVector Direction = ServerTarget - ServerPos;
        Direction.Z = 0.0f;
        if (!Direction.IsNearlyZero())
        {
            SetActorRotation(Direction.Rotation());
        }
    }
    else
    {
        InterpolationTarget = ServerPos;
        bIsInterpolating = false;
    }
}

void AMMOEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    // Smooth interpolation toward server position
    if (bIsInterpolating || !GetActorLocation().Equals(InterpolationTarget, 5.0f))
    {
        FVector CurrentLocation = GetActorLocation();
        FVector NewLocation = FMath::VInterpTo(CurrentLocation, InterpolationTarget, DeltaTime, InterpSpeed);
        SetActorLocation(NewLocation);

        if (CurrentLocation.Equals(InterpolationTarget, 5.0f))
        {
            bIsInterpolating = false;
        }
    }
}

void AMMOEnemy::PlayAttackAnimation(int32 AttackMotionMs)
{
    // For now, a simple implementation. When animation assets are added,
    // this will play the appropriate attack montage.
    // The AttackMotionMs value from the server can be used to set playback rate.
    UE_LOG(LogTemp, Verbose, TEXT("Enemy %s (%d) attack animation, motion=%d ms"),
        *EnemyName, EnemyId, AttackMotionMs);
}

void AMMOEnemy::PlayDeathSequence()
{
    bIsDead = true;
    bIsInterpolating = false;

    // Disable collision so other actors can walk through
    SetActorEnableCollision(false);

    // Start despawn timer (hide after delay to allow death animation to play)
    GetWorldTimerManager().SetTimer(
        DeathDespawnTimer, this, &AMMOEnemy::OnDeathDespawnTimerExpired,
        DeathDespawnDelay, false);

    UE_LOG(LogTemp, Log, TEXT("Enemy %s (%d) died -- despawning in %.1f seconds"),
        *EnemyName, EnemyId, DeathDespawnDelay);
}

void AMMOEnemy::OnDeathDespawnTimerExpired()
{
    // Hide the actor until respawn (enemy:spawn event will re-show it)
    SetActorHiddenInGame(true);
}

void AMMOEnemy::PlayReceiveHitAnimation()
{
    // Placeholder for hit stun visual feedback
    UE_LOG(LogTemp, Verbose, TEXT("Enemy %s (%d) received hit"), *EnemyName, EnemyId);
}
```

### Movement Interpolation

The `UEnemyInterpolationComponent` provides smooth movement between server position updates (arriving every 200ms):

```cpp
// Enemy/EnemyInterpolationComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyInterpolationComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SABRIMMO_API UEnemyInterpolationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyInterpolationComponent();

    // Called by EnemySubsystem when new server position arrives
    void SetServerState(const FVector& Position, const FVector& MoveTarget, bool bMoving);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, Category = "Interpolation")
    float InterpolationSpeed = 8.0f;

    // How far behind server the interpolation runs (in seconds)
    UPROPERTY(EditAnywhere, Category = "Interpolation")
    float InterpolationDelay = 0.1f;

private:
    FVector TargetPosition = FVector::ZeroVector;
    FVector MoveDirection = FVector::ZeroVector;
    bool bIsMoving = false;
};
```

```cpp
// Enemy/EnemyInterpolationComponent.cpp
#include "EnemyInterpolationComponent.h"
#include "GameFramework/Actor.h"

UEnemyInterpolationComponent::UEnemyInterpolationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyInterpolationComponent::SetServerState(
    const FVector& Position, const FVector& MoveTarget, bool bMoving)
{
    TargetPosition = Position;
    bIsMoving = bMoving;

    if (bMoving)
    {
        MoveDirection = (MoveTarget - Position).GetSafeNormal();
    }
    else
    {
        MoveDirection = FVector::ZeroVector;
    }
}

void UEnemyInterpolationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!Owner) return;

    FVector CurrentLocation = Owner->GetActorLocation();
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetPosition, DeltaTime, InterpolationSpeed);
    Owner->SetActorLocation(NewLocation);

    // Orient toward movement direction
    if (bIsMoving && !MoveDirection.IsNearlyZero())
    {
        FRotator TargetRotation = MoveDirection.Rotation();
        FRotator CurrentRotation = Owner->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 10.0f);
        Owner->SetActorRotation(NewRotation);
    }
}
```

### Integration with WorldHealthBarSubsystem

The existing `WorldHealthBarSubsystem` already handles `enemy:spawn`, `enemy:move`, `enemy:death`, and `enemy:health_update` events for rendering floating HP bars. The `EnemySubsystem` manages the actual 3D actors. Both subsystems consume the same socket events independently (chain pattern via `OnNativeEvent.AddLambda`).

The `WorldHealthBarSubsystem`'s `FEnemyBarData` struct already has a `CachedActor` weak pointer -- when `EnemySubsystem` spawns an `AMMOEnemy`, the health bar system can find the actor for smooth position tracking.

### Socket Event Reference (Server to Client)

| Event | Payload | Handler |
|-------|---------|---------|
| `enemy:spawn` | `{ enemyId, templateId, name, level, health, maxHealth, monsterClass, size, race, element, x, y, z }` | Create data entry + spawn actor |
| `enemy:move` | `{ enemyId, x, y, z, targetX, targetY, isMoving }` | Update position, drive interpolation |
| `enemy:attack` | `{ enemyId, targetId, attackMotion }` | Play attack animation on actor |
| `enemy:death` | `{ enemyId, enemyName, killerId, killerName, isEnemy, baseExp, jobExp }` | Play death animation, hide actor |
| `enemy:health_update` | `{ enemyId, health, maxHealth, inCombat }` | Update HP data for health bars |

### Socket Events (Client to Server)

| Event | Payload | Purpose |
|-------|---------|---------|
| `combat:attack` | `{ targetId: enemyId, isEnemy: true }` | Start auto-attacking an enemy |
| `skill:use` | `{ skillId, targetId: enemyId, isEnemy: true }` | Cast a targeted skill on enemy |

---

## 7. Client-Side: Enemy Animations

### Animation Blueprint State Machine

Each enemy type needs an Animation Blueprint (ABP) with the following states:

```
[Entry] --> [Idle] --(bIsMoving)--> [Walk]
                   --(bIsAttacking)--> [Attack]
                   --(bReceivedHit)--> [ReceiveHit]
                   --(bIsDead)--> [Die]

[Walk] --(NOT bIsMoving)--> [Idle]
       --(bIsAttacking)--> [Attack]
       --(bIsDead)--> [Die]

[Attack] --(OnComplete)--> [Idle] or [Walk]
         --(bIsDead)--> [Die]

[ReceiveHit] --(OnComplete)--> previous state

[Die] --> (terminal state, no transitions out)
```

### Animation Variables (Set by AMMOEnemy)

| Variable | Type | Set When |
|----------|------|----------|
| `bIsMoving` | bool | `UpdateServerPosition()` with `bMoving=true` |
| `bIsAttacking` | bool | `PlayAttackAnimation()` sets true, cleared on montage end |
| `bReceivedHit` | bool | `PlayReceiveHitAnimation()`, cleared after hit anim |
| `bIsDead` | bool | `PlayDeathSequence()` |
| `AttackPlayRate` | float | Calculated from `attackMotion` to match server timing |

### Attack Motion Sync

The server sends `attackMotion` (in ms) with each `enemy:attack` event. The client adjusts the animation playback rate so the attack visual matches:

```cpp
void AMMOEnemy::PlayAttackAnimation(int32 AttackMotionMs)
{
    UAnimMontage* AttackMontage = /* loaded via data table or assigned in Blueprint */;
    if (!AttackMontage) return;

    float MontageLength = AttackMontage->GetPlayLength();
    float DesiredDuration = AttackMotionMs / 1000.0f;  // Convert ms to seconds
    float PlayRate = (DesiredDuration > 0.0f) ? (MontageLength / DesiredDuration) : 1.0f;
    PlayRate = FMath::Clamp(PlayRate, 0.1f, 5.0f);  // Safety clamp

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        AnimInstance->Montage_Play(AttackMontage, PlayRate);
    }
}
```

### Death Animation and Despawn

```cpp
void AMMOEnemy::PlayDeathSequence()
{
    bIsDead = true;
    bIsInterpolating = false;
    SetActorEnableCollision(false);

    // Play death montage if available
    UAnimMontage* DeathMontage = /* loaded via data table */;
    if (DeathMontage)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance)
        {
            AnimInstance->Montage_Play(DeathMontage);
        }
    }

    // Schedule visual hide after death animation completes
    float HideDelay = DeathMontage ? DeathMontage->GetPlayLength() + 1.0f : 3.0f;
    GetWorldTimerManager().SetTimer(
        DeathDespawnTimer, this, &AMMOEnemy::OnDeathDespawnTimerExpired,
        HideDelay, false);
}
```

---

## 8. Client-Side: Loot System

### Current Implementation Status

Loot is currently handled server-side: items go directly into the killer's inventory on kill. The client receives `loot:drop` and `inventory:data` events.

### Socket Events

```javascript
// Server emits to killer on enemy death:
socket.emit('loot:drop', {
    enemyId: enemy.enemyId,
    enemyName: enemy.name,
    items: [
        { itemId: 1001, itemName: 'Jellopy', quantity: 3, icon: 'item_jellopy', itemType: 'etc' },
        { itemId: null, itemName: 'Unripe Apple', quantity: 1, icon: 'default_item', itemType: 'etc' },
    ]
});

// Followed by updated inventory:
socket.emit('inventory:data', { items: killerInventory, zuzucoin: attacker.zuzucoin });
```

### UE5 Loot Notification (Future Implementation)

A `LootNotificationSubsystem` should:

1. Listen for `loot:drop` events
2. Display a brief on-screen notification showing items received
3. Optionally add a chat message for each item

```cpp
// UI/LootNotificationSubsystem.h (future)
UCLASS()
class SABRIMMO_API ULootNotificationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    void HandleLootDrop(const TSharedPtr<FJsonValue>& Data);
    // Display: "You obtained 3x Jellopy from Poring"
    // Display: "You obtained 1x Knife [3] from Poring"
};
```

### Ground Item Display (Future)

When ground drops are implemented, the client will need:

```cpp
// Loot/GroundItemActor.h (future)
UCLASS()
class SABRIMMO_API AGroundItemActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly) int32 GroundItemId = 0;
    UPROPERTY(BlueprintReadOnly) int32 ItemId = 0;
    UPROPERTY(BlueprintReadOnly) FString ItemName;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 1;
    UPROPERTY(BlueprintReadOnly) float DespawnTimer = 60.0f;

    // Visual: item mesh/sprite hovering above ground with name label
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* ItemMesh;
    UPROPERTY(VisibleAnywhere) UWidgetComponent* NameLabel;

    // Click interaction: calls inventory:pickup on server
    void OnClicked(AActor* ClickedActor);
};
```

### Auto-Loot Setting (Future)

```cpp
// In MMOGameInstance or via game settings:
UPROPERTY(Config, BlueprintReadWrite, Category = "Settings")
bool bAutoLootEnabled = false;

// When enabled, loot:drop items are automatically picked up
// (server already puts them in inventory, so this just skips the ground item step)
```

---

## 9. MVP/Boss Special Handling

### Boss Protocol (Server-Side -- Already Implemented)

For monsters with `monsterClass === 'boss'` or `monsterClass === 'mvp'`, the server auto-applies:

```javascript
if (template.monsterClass === 'boss' || template.monsterClass === 'mvp') {
    modeFlags.knockbackImmune = true;
    modeFlags.statusImmune = true;
    modeFlags.detector = true;
    if (template.monsterClass === 'mvp') modeFlags.mvp = true;
}
```

This means:
- **Knockback Immune**: Cannot be displaced by Arrow Repel, Storm Gust, etc.
- **Status Immune**: Immune to Stun, Freeze, Stone Curse, etc.
- **Detector**: Can see hidden/cloaked players
- **MVP Flag**: Triggers MVP reward system on death

### Client-Side Boss/MVP UI (Future)

#### Boss HP Bar

MVPs and bosses should display a larger, more prominent health bar:

```cpp
// In WorldHealthBarSubsystem or a dedicated BossBarSubsystem:
// When an enemy with monsterClass="mvp" or "boss" enters combat,
// display a large bar at the top-center of the screen (like a raid boss bar)

struct FBossBarData
{
    int32 EnemyId = 0;
    FString Name;
    int32 Level = 1;
    int32 CurrentHP = 0;
    int32 MaxHP = 1;
    FString MonsterClass; // "boss" or "mvp"
};
```

The bar should:
- Appear at top-center of screen when player enters combat with a boss/MVP
- Show boss name, level, and HP percentage
- Use a distinct color scheme (e.g. gold border for MVP, red border for boss)
- Disappear when boss dies or player leaves combat range

#### MVP Announcement (Future)

When an MVP is killed, the server should broadcast a system message:

```javascript
// Server-side (future):
io.emit('chat:receive', {
    channel: 'SYSTEM',
    message: `MVP: ${attacker.characterName} has defeated ${enemy.name}!`,
    timestamp: Date.now()
});
```

#### MVP Tombstone (Future)

A visual marker at the MVP's death location:

```cpp
// Enemy/MVPTombstoneActor.h (future)
UCLASS()
class SABRIMMO_API AMVPTombstoneActor : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly) FString MVPName;
    UPROPERTY(BlueprintReadOnly) FString KillerName;
    UPROPERTY(BlueprintReadOnly) FDateTime TimeOfDeath;

    // Visual: stone pillar/marker with text
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* TombstoneMesh;
    UPROPERTY(VisibleAnywhere) UWidgetComponent* InfoPanel;

    // Interaction: click to see MVP name, killer, and time of death
    // Disappears 5 seconds after the MVP respawns
};
```

The server would emit:

```javascript
// Server-side (future):
broadcastToZone(enemyZone, 'mvp:tombstone', {
    enemyId: enemy.enemyId,
    mvpName: enemy.name,
    killerName: attacker.characterName,
    x: enemy.x, y: enemy.y, z: enemy.z,
    timeOfDeath: Date.now()
});

// On respawn:
broadcastToZone(enemyZone, 'mvp:tombstone_remove', {
    enemyId: enemy.enemyId
});
```

---

## 10. Adding a New Monster (Step-by-Step)

This section provides a complete checklist for adding any new monster to the game, from server template to client visuals.

### Step 1: Add Template to `ro_monster_templates.js`

```javascript
// server/src/ro_monster_templates.js

// ------ Custom Monster: Shadow Knight (ID: 99002) ------ Level 55 | HP 12000 | BOSS
RO_MONSTER_TEMPLATES['shadow_knight'] = {
    id: 99002, name: 'Shadow Knight', aegisName: 'SHADOW_KNIGHT',
    level: 55, maxHealth: 12000, baseExp: 4500, jobExp: 2200, mvpExp: 0,
    attack: 350, attack2: 440, defense: 30, magicDefense: 20,
    str: 60, agi: 70, vit: 40, int: 30, dex: 80, luk: 20,
    attackRange: 50, aggroRange: 450, chaseRange: 800,
    aspd: 175, walkSpeed: 180, attackDelay: 1100, attackMotion: 450, damageMotion: 250,
    size: 'large', race: 'demon',
    element: { type: 'shadow', level: 3 },
    monsterClass: 'boss',       // <-- 'boss' for mini-boss, 'mvp' for MVP
    aiType: 'aggressive',
    respawnMs: 600000,           // 10 minutes
    raceGroups: {},
    stats: {
        str: 60, agi: 70, vit: 40, int: 30, dex: 80, luk: 20,
        level: 55, weaponATK: 350
    },
    modes: {},
    drops: [
        { itemName: 'Shadow Crystal', rate: 50 },
        { itemName: 'Elunium', rate: 20 },
        { itemName: 'Shadow Blade', rate: 3 },
        { itemName: 'Shadow Knight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};
```

### Step 2: Set AI Code and Mode Flags

If using a standard RO monster (ID 1000-9999), the AI code is automatically looked up from `ro_monster_ai_codes.js`. For custom monsters, add an entry:

```javascript
// server/src/ro_monster_ai_codes.js
const MONSTER_AI_CODES = {
    // ... existing entries ...
    99002: 21,   // Shadow Knight -- Boss/MVP AI (aggressive, full target switching, cast sensor)
};
```

**AI code selection guide**:
| Desired Behavior | AI Code | Description |
|-----------------|---------|-------------|
| Passive, no aggro | 1 | Only fights back when attacked |
| Passive + picks up items | 2 | Like 1 but loots ground items |
| Passive + allies assist | 3 | Same-type mobs join fight |
| Aggressive + target switch | 4 | Attacks on sight, switches targets |
| Aggressive + chase switch | 5 | Aggressive, switches during chase |
| Plant/Immobile | 6 | Cannot move or attack |
| Aggressive + cast sensor | 9 | Aggros casters |
| Immobile turret | 10 | Attacks in range, cannot move |
| Full boss AI | 21 | Most active: all sensors, all switches |

### Step 3: Configure Spawn Points in Zone Data

```javascript
// server/src/ro_zone_data.js
ZONE_REGISTRY.prt_dungeon_01 = {
    // ... existing zone config ...
    enemySpawns: [
        // ... existing spawns ...
        // Add Shadow Knight spawn
        { template: 'shadow_knight', x: 500, y: -800, z: 300, wanderRadius: 300 },
    ]
};
```

**Spawn placement tips**:
- `x, y` coordinates should be within the UE5 level's playable bounds
- `z` should match the ground level (typically 300 for field maps)
- `wanderRadius` should keep the enemy within logical bounds (200-500 typical)
- Boss/MVP spawns typically have 1 instance per zone
- Normal mobs typically have 2-5 instances per zone area

### Step 4: Add 3D Model/Mesh to UE5

1. Import the monster's skeletal mesh to `Content/SabriMMO/Enemies/SK_ShadowKnight`
2. Create a material in `Content/SabriMMO/Enemies/M_ShadowKnight`
3. Create a child Blueprint of `AMMOEnemy`: `BP_ShadowKnight` in `Content/SabriMMO/Enemies/`
4. Assign the skeletal mesh, material, and adjust capsule size

**Alternatively**, if using a generic mesh system:
- Create a data table mapping `templateId` to skeletal mesh references
- The `EnemySubsystem` looks up the mesh when spawning

### Step 5: Map Animations

1. Import animation assets: Idle, Walk, Attack, ReceiveHit, Die
2. Create Animation Blueprint: `ABP_ShadowKnight`
3. Set up state machine with transitions based on `bIsMoving`, `bIsAttacking`, `bIsDead`
4. Assign ABP to the skeletal mesh in `BP_ShadowKnight`

### Step 6: Add Drop Table Entries (Optional)

If the monster drops items that do not yet exist in the `items` database table:

```sql
-- database/migrations/add_shadow_knight_items.sql
INSERT INTO items (item_id, name, description, item_type, price, weight, stackable, max_stack, icon)
VALUES
    (5001, 'Shadow Crystal', 'A crystal infused with dark energy.', 'etc', 500, 10, true, 99, 'item_shadow_crystal'),
    (5002, 'Shadow Blade', 'A sword forged from shadow steel.', 'weapon', 15000, 120, false, 1, 'item_shadow_blade');
```

Then map the item names to IDs in the RO item name mapping:

```javascript
// server/src/ro_item_mapping.js (if needed)
RO_ITEM_NAME_TO_ID['Shadow Crystal'] = 5001;
RO_ITEM_NAME_TO_ID['Shadow Blade'] = 5002;
```

### Step 7: Test Spawning, Aggro, Combat, Drops

1. **Start the server**: `cd server && npm run dev`
2. **Open UE5 editor**, launch PIE with 2 instances
3. **Enter the zone** where the monster spawns
4. **Verify spawn**: Check server logs for `[ENEMY] Spawned Shadow Knight (ID: ...) Lv55 [boss] AI21 [AGG,AST,CTM,CTC,CSI]`
5. **Test aggro**: Walk within `aggroRange` (450 UE units) -- aggressive mobs should start chasing
6. **Test combat**: Auto-attack the monster, verify `combat:damage` events fire correctly
7. **Test death**: Kill the monster, verify `enemy:death` event, EXP gain, drops
8. **Test respawn**: Wait `respawnMs` (10 minutes for boss), verify `enemy:spawn` fires again
9. **Test with 2 players**: Verify assist triggers (if another same-type mob exists), target switching, aggro sharing

### Complete Monster Addition Checklist

| Step | File | Action | Required? |
|------|------|--------|-----------|
| 1 | `server/src/ro_monster_templates.js` | Add template with all fields | YES |
| 2 | `server/src/ro_monster_ai_codes.js` | Add AI code mapping (custom monsters only) | For custom only |
| 3 | `server/src/ro_zone_data.js` | Add spawn point(s) to zone | YES |
| 4 | `Content/SabriMMO/Enemies/` | Import 3D model, create Blueprint | YES (for visual) |
| 5 | `Content/SabriMMO/Enemies/` | Create Animation Blueprint | YES (for visual) |
| 6 | `database/migrations/` | Add new items to DB (if new drops) | Only if new items |
| 7 | `server/src/ro_item_mapping.js` | Map item names to DB IDs | Only if new items |
| 8 | PIE testing | Test spawn, aggro, combat, death, drops, respawn | YES |

### Server-Only Quick Add (No Visual)

If you just need the monster to exist server-side (for testing combat/drops without a 3D model), only steps 1-3 are required. The client's `WorldHealthBarSubsystem` will still show floating HP bars at the server-reported positions, and all combat mechanics will work through the existing socket event pipeline.

---

## Appendix A: Socket Event Quick Reference

### Server to Client

| Event | Key Fields | Used By |
|-------|-----------|---------|
| `enemy:spawn` | enemyId, templateId, name, level, health, maxHealth, x, y, z | EnemySubsystem, WorldHealthBarSubsystem |
| `enemy:move` | enemyId, x, y, z, targetX, targetY, isMoving | EnemySubsystem, WorldHealthBarSubsystem |
| `enemy:attack` | enemyId, targetId, attackMotion | EnemySubsystem |
| `enemy:death` | enemyId, enemyName, killerId, killerName, baseExp, jobExp | EnemySubsystem, WorldHealthBarSubsystem |
| `enemy:health_update` | enemyId, health, maxHealth, inCombat | WorldHealthBarSubsystem |
| `combat:damage` | attackerId, targetId, damage, isCritical, isEnemyAttacker | DamageNumberSubsystem, BasicInfoSubsystem |
| `loot:drop` | enemyId, enemyName, items[] | LootNotificationSubsystem (future) |
| `exp:gain` | characterId, baseExpGained, jobExpGained, enemyName | BasicInfoSubsystem |

### Client to Server

| Event | Key Fields | Purpose |
|-------|-----------|---------|
| `combat:attack` | targetId, isEnemy: true | Start auto-attacking enemy |
| `combat:stop` | (none) | Stop auto-attacking |
| `skill:use` | skillId, targetId, isEnemy: true | Cast skill on enemy |

---

## Appendix B: Key File Locations

| File | Path | Purpose |
|------|------|---------|
| Monster templates | `server/src/ro_monster_templates.js` | 509 monster definitions |
| AI code mappings | `server/src/ro_monster_ai_codes.js` | 1,004 monster ID to AI code mappings |
| Zone registry | `server/src/ro_zone_data.js` | Spawn configs per zone |
| Server AI + combat | `server/src/index.js` (lines 6880-7480) | AI tick loop, aggro, combat, death |
| Spawn function | `server/src/index.js` (line ~1054) | spawnEnemy() |
| Drop roll function | `server/src/index.js` (line ~1254) | rollEnemyDrops() |
| Death processing | `server/src/index.js` (line ~476) | processEnemyDeathFromSkill() |
| World health bars | `client/SabriMMO/Source/SabriMMO/UI/WorldHealthBarSubsystem.*` | Floating HP bars |
| Variant combat (reference) | `client/SabriMMO/Source/SabriMMO/Variant_Combat/AI/CombatEnemy.*` | Existing offline enemy (NOT used in MMO) |
| CharacterData | `client/SabriMMO/Source/SabriMMO/CharacterData.h` | Shared data structs |
| RO damage formula | `server/src/ro_damage_formulas.js` | roPhysicalDamage() |

---

## Appendix C: Existing Variant_Combat Enemy (NOT Used)

The files in `client/SabriMMO/Source/SabriMMO/Variant_Combat/AI/` (`CombatEnemy.h/cpp`, `CombatEnemySpawner.h/cpp`) are from the UE5 combat sample template and are NOT used in the MMO system. They implement local AI with StateTree, local damage with sphere traces, and local spawning -- none of which apply to the server-authoritative architecture.

The MMO enemy system (`AMMOEnemy`, `UEnemySubsystem`) is built from scratch to work with the server-authoritative socket event pipeline. Do NOT extend `ACombatEnemy` for MMO enemies.

---

## Appendix D: Performance Considerations

### Server-Side

- **AI tick runs every 200ms** for all enemies in active zones. With ~46 active spawns across 3 zones, this is negligible.
- **Zone optimization**: `getActiveZones()` tracks which zones have players. Enemies in empty zones skip all AI processing.
- **Lazy spawning**: Enemies are only created when the first player enters a zone, avoiding memory waste for unvisited zones.
- **Position broadcast throttling**: Enemy positions are only broadcast every `MOVE_BROADCAST_MS` (200ms), not every tick.

### Client-Side

- **Actor pooling**: Rather than destroying and re-creating actors on death/respawn, the `EnemySubsystem` hides dead actors and reuses them on `enemy:spawn`.
- **Tick optimization**: Only enemies that are interpolating (moving) need Tick processing. Stationary enemies can disable tick.
- **LOD**: For large zones with many enemies, consider distance-based LOD -- hide nameplates and simplify meshes for distant enemies.
- **Health bar culling**: The `WorldHealthBarSubsystem` already only renders bars for enemies within screen bounds.
