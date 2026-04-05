# Enemy Combat System

## Overview

Server-authoritative enemy (NPC/monster) combat system modeled after Ragnarok Online. Enemies spawn at predefined locations, wander randomly within a radius, can be attacked by players, take damage, die, and respawn on a timer. Supports multiple players attacking the same enemy simultaneously.

## Architecture

```
Client (Blueprint)                    Server (index.js)                   
┌──────────────────┐                 ┌──────────────────────────────┐
│ BP_EnemyCharacter │ ◄── enemy:spawn ──│ spawnEnemy()                │
│  - Mesh + Anim   │ ◄── enemy:move  ──│ Enemy AI Wander Tick (500ms)│
│  - Health Bar    │ ◄── enemy:death ──│ Combat Tick Loop (50ms)     │
│  - Name Tag      │ ◄── enemy:health──│ calculatePhysicalDamage()   │
│  - Hover Indicator│                  │                              │
│  - Collision     │                   │ enemies Map<int, EnemyObj>   │
├──────────────────┤                   ├──────────────────────────────┤
│ BP_EnemyManager  │                   │ ENEMY_TEMPLATES (6 types)    │
│  - EnemyMap      │                   │ ENEMY_SPAWNS (12 locations)  │
│  - Spawn/Remove  │                   │ ENEMY_AI (wander constants)  │
│  - Lookup by ID  │                   │ autoAttackState Map          │
├──────────────────┤                   ├──────────────────────────────┤
│ BP_SocketManager │                   │ COMBAT constants             │
│  - Bind events   │ ── combat:attack →│  - MELEE_RANGE: 100         │
│  - Parse JSON    │ ── combat:stop  →│  - COMBAT_TICK_MS: 50       │
│  - Emit attacks  │                   │  - RANGE_TOLERANCE: 50      │
└──────────────────┘                   └──────────────────────────────┘
```

## Enemy Templates

| Template | Name | Level | MaxHP | Damage | AggroRange | ASPD | EXP | AI Type | Respawn |
|----------|------|-------|-------|--------|------------|------|-----|---------|---------|
| blobby | Blobby | 1 | 50 | 1 | 300 | 175 | 10 | passive | 10s |
| hoplet | Hoplet | 3 | 100 | 3 | 400 | 178 | 25 | passive | 15s |
| crawlid | Crawlid | 2 | 75 | 2 | 0 | 176 | 15 | passive | 12s |
| shroomkin | Shroomkin | 4 | 120 | 4 | 350 | 177 | 30 | passive | 15s |
| buzzer | Buzzer | 5 | 150 | 5 | 500 | 179 | 40 | aggressive | 18s |
| mosswort | Mosswort | 3 | 5 | 2 | 0 | 174 | 20 | passive | 12s |

## Spawn Locations

12 enemies spawn at server start across the map. Each has a `wanderRadius` defining how far they roam from their spawn point.

## Server Implementation

### Enemy Object Structure
```javascript
{
    enemyId: 2000001,           // Unique ID (starts at 2000001)
    templateId: 'blobby',       // Template key
    name: 'Blobby',             // Display name
    level: 1,                   // Enemy level
    health: 50,                 // Current HP
    maxHealth: 50,              // Max HP
    damage: 1,                  // Base damage
    attackRange: 80,            // Attack range (units)
    aggroRange: 300,            // Aggro detection range
    aspd: 175,                  // Attack speed
    exp: 10,                    // EXP reward on kill
    aiType: 'passive',          // AI behavior type
    stats: { str, agi, vit, int, dex, luk, level, weaponATK },
    isDead: false,              // Death state
    x, y, z,                   // Current position
    spawnX, spawnY, spawnZ,     // Original spawn position
    wanderRadius: 300,          // Max wander distance from spawn
    respawnMs: 10000,           // Respawn delay (ms)
    targetPlayerId: null,       // Current aggro target
    lastAttackTime: 0,          // Last attack timestamp
    inCombatWith: Set<charId>,  // Players currently attacking this enemy
    // Wander AI state
    wanderTargetX, wanderTargetY, // Current wander destination
    isWandering: false,         // Currently moving to wander point
    nextWanderTime: 0,          // When to pick next wander point
    lastMoveBroadcast: 0        // Last position broadcast timestamp
}
```

### Enemy AI Constants (ENEMY_AI)
```javascript
{
    WANDER_TICK_MS: 500,        // AI tick interval
    WANDER_PAUSE_MIN: 3000,     // Min idle pause (ms)
    WANDER_PAUSE_MAX: 8000,     // Max idle pause (ms)
    WANDER_SPEED: 60,           // Movement speed (units/sec)
    MOVE_BROADCAST_MS: 200      // Position broadcast rate (ms)
}
```

### Enemy Lifecycle
1. **Spawn**: `spawnEnemy()` creates enemy object, adds to `enemies` Map, broadcasts `enemy:spawn`
2. **Wander**: AI tick picks random points within `wanderRadius`, moves at `WANDER_SPEED`, broadcasts `enemy:move`
3. **Combat**: Player clicks enemy → `combat:attack` with `targetEnemyId` → server validates → combat tick processes damage
4. **Death**: HP reaches 0 → `enemy:death` broadcast → all attackers' autoAttackState cleared (NO `target_lost` sent) → respawn timer starts
5. **Respawn**: After `respawnMs` → reset HP/position/wander state → `enemy:spawn` broadcast

### Critical Bug Fix Pattern
When an enemy dies, **NEVER** send `combat:target_lost` to any attacker. Only broadcast `enemy:death`. Sending both events causes a race condition where Blueprint processes `target_lost` (nulls enemy reference) before `enemy:death` arrives, causing `EXCEPTION_ACCESS_VIOLATION`.

## Socket.io Events Reference

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `enemy:spawn` | Server→Client | `{ enemyId, templateId, name, level, health, maxHealth, x, y, z }` | Enemy spawned or respawned |
| `enemy:move` | Server→Client | `{ enemyId, x, y, z, targetX?, targetY?, isMoving }` | Enemy wandering position update |
| `enemy:health_update` | Server→Client | `{ enemyId, health, maxHealth, inCombat }` | Enemy health changed |
| `enemy:death` | Server→Client | `{ enemyId, enemyName, killerId, killerName, isEnemy:true, isDead:true, exp, timestamp }` | Enemy killed |
| `combat:attack` | Client→Server | `{ targetEnemyId }` | Player starts attacking enemy |
| `combat:damage` | Server→Client | `{ attackerId, attackerName, targetId, targetName, isEnemy:true, damage, targetHealth, targetMaxHealth, attackerX/Y/Z, targetX/Y/Z }` | Damage dealt to enemy |
| `combat:auto_attack_started` | Server→Client | `{ targetId, targetName, isEnemy:true, attackRange, aspd, attackIntervalMs }` | Auto-attack loop started |
| `combat:auto_attack_stopped` | Server→Client | `{ reason }` | Auto-attack stopped (player clicked ground) |
| `combat:out_of_range` | Server→Client | `{ targetId, isEnemy:true, targetX/Y/Z, distance, requiredRange }` | Player too far from enemy |

## Client Implementation (Blueprint)

### BP_EnemyCharacter
- **Parent**: Character
- **Components**: Mesh, NameTagWidget, HealthBarWidget, HoverOverIndicator, CapsuleComponent
- **Variables**: EnemyId (Int), EnemyName (String), EnemyLevel (Int), TargetPosition (Vector), Health (Float), MaxHealth (Float), DeltaSeconds (Float, renamed from `Delta Seconds`), bIsDead (Bool, renamed from `IsDead`), bIsTargeted (Bool, renamed from `IsTargeted`), bIsActiveTarget (Bool, renamed from `IsActiveTarget`), bIsMoving (Bool, Phase 2.1)
- **Event Tick**: Branch on `bIsDead` (FALSE) → Branch on `bIsMoving` (TRUE) → Interpolate toward TargetPosition (same pattern as BP_OtherPlayerCharacter). Movement logic is skipped entirely when bIsMoving=false (Phase 2.1 optimization).
- **On Death**: Set bIsDead=true, hide widgets, disable collision (CapsuleComponent + Mesh → Set Collision Enabled: No Collision)
- **On Respawn**: Set bIsDead=false, show widgets, re-enable collision (Query and Physics), teleport to spawn position
- **InitializeEnemy**: Sets bIsMoving=true at the end (Phase 2.1)
- **UpdateHealthBar function**: Parameter renamed from `Maxhealth` → `MaxHealth` (Phase 1.3)
- **BPI_Damageable Implementation**: Implements `BPI_Damageable` interface for unified damage handling (Phase 3)
  - `ReceiveDamageVisual`: Rotates actor toward attacker location
  - `UpdateHealthDisplay`: Calls `UpdateEnemyHealth` with health and combat state
  - `GetHealthInfo`: Returns `HealthBarWidget` component

### BP_EnemyManager
- **Variables**: EnemyMap (Map<Integer, BP_EnemyCharacter>)
- **Functions**: SpawnOrUpdateEnemy(data), GetEnemyActor(enemyId), RemoveEnemy(enemyId)

### BP_SocketManager Event Bindings
- `enemy:spawn` → OnEnemySpawn
- `enemy:move` → OnEnemyMove
- `enemy:death` → OnEnemyDeath
- `enemy:health_update` → OnEnemyHealthUpdate

### Click-to-Target (BP_MMOCharacter)
Left-click cast chain: `Cast To BP_EnemyCharacter` (first) → if success, emit `combat:attack` with `targetEnemyId`. If fail, try `Cast To BP_OtherPlayerCharacter` → emit with `targetCharacterId`.

## Collision Behavior
- **Alive**: Full collision (Query and Physics) — blocks movement, clickable
- **Dead**: No collision — doesn't block movement, not clickable
- **Respawned**: Full collision restored

## Troubleshooting

### Issue: "Target not found" when attacking enemies
**Check**: Server must `parseInt()` the `targetEnemyId` from client data. UE5 SocketIOClient sends numbers as strings.

### Issue: Crash when 2 players kill same enemy
**Check**: Server must NOT send `combat:target_lost` during enemy death. Only `enemy:death` broadcast.

### Issue: Enemies not wandering
**Check**: Enemy must not be in combat (`inCombatWith.size === 0`) and not dead (`isDead === false`).

## Related Files

| File | Purpose |
|------|---------|
| `server/src/index.js` | Enemy system, combat tick, AI wander tick |
| `docs/Bug_Fix_Notes.md` | Crash fixes related to enemy combat |
| `docs/SocketIO_RealTime_Multiplayer.md` | Socket.io event architecture |
| `docs/Ragnarok_Online_Reference.md` | RO-style game mechanics reference |
| `docs/BPI_Damageable.md` | Interface for unified damage handling (Phase 3) |

---

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| **Interface** | `BPI_Damageable` replaces Cast chains for damage/health updates |
| **Manager** | `BP_EnemyManager` coordinates all enemy actors |
| **Event-Driven** | Socket.io events trigger health updates |

---

**Last Updated**: 2026-02-17
**Version**: 1.2
**Status**: Complete — Phase 1.2 PrintStrings removed, Phase 1.3 variable renames (bIsDead, bIsActiveTarget, bIsTargeted, MaxHealth), Phase 2.1 bIsMoving optimization, Phase 3 BPI_Damageable interface
