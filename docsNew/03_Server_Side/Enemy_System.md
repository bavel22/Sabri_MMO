# Enemy System — Server-Side Documentation

## Overview

The enemy system manages AI-controlled NPCs with Ragnarok Online-style wandering behavior. All enemy state (health, position, combat) is tracked server-side. Enemies spawn on server startup, wander randomly within their spawn radius, and respawn after being killed.

## Enemy Templates

6 enemy templates defined in `ENEMY_TEMPLATES`:

| Template | Level | HP | ATK | Aggro | ASPD | EXP | AI | Respawn |
|----------|-------|----|-----|-------|------|-----|----|---------|
| `blobby` | 1 | 50 | 1 | 300 | 175 | 10 | passive | 10s |
| `hoplet` | 3 | 100 | 3 | 400 | 178 | 25 | passive | 15s |
| `crawlid` | 2 | 75 | 2 | 0 | 176 | 15 | passive | 12s |
| `shroomkin` | 4 | 120 | 4 | 350 | 177 | 30 | passive | 15s |
| `buzzer` | 5 | 150 | 5 | 500 | 179 | 40 | aggressive | 18s |
| `mosswort` | 3 | 5 | 2 | 0 | 174 | 20 | passive | 12s |

**Notes**:
- `aggroRange: 0` means the enemy never aggros (crawlid, mosswort)
- `aiType: 'aggressive'` means the enemy will chase players who enter aggro range (buzzer — not yet implemented in tick loop)
- `aiType: 'passive'` means the enemy only fights back when attacked

## Spawn Configuration

12 spawn points defined in `ENEMY_SPAWNS`:

| # | Template | X | Y | Z | Wander Radius |
|---|----------|---|---|---|---------------|
| 1 | blobby | 500 | 500 | 300 | 300 |
| 2 | blobby | -500 | 300 | 300 | 300 |
| 3 | blobby | 200 | -400 | 300 | 300 |
| 4 | hoplet | 800 | -200 | 300 | 400 |
| 5 | hoplet | -700 | -500 | 300 | 400 |
| 6 | crawlid | -300 | 800 | 300 | 200 |
| 7 | crawlid | 400 | 700 | 300 | 200 |
| 8 | shroomkin | 1000 | 300 | 300 | 350 |
| 9 | shroomkin | -900 | 100 | 300 | 350 |
| 10 | buzzer | 1200 | -600 | 300 | 500 |
| 11 | mosswort | 0 | -800 | 300 | 200 |
| 12 | mosswort | 600 | -900 | 300 | 200 |

## Enemy Data Structure

```javascript
{
    enemyId: 2000001,              // Server-assigned unique ID
    templateId: 'blobby',          // Template key
    name: 'Blobby',               // Display name
    level: 1,
    health: 50,                    // Current HP
    maxHealth: 50,
    damage: 1,
    attackRange: 80,
    aggroRange: 300,
    aspd: 175,
    exp: 10,
    aiType: 'passive',
    stats: { str:1, agi:1, vit:1, int:1, dex:1, luk:1, level:1, weaponATK:0 },
    isDead: false,
    x: 500, y: 500, z: 300,       // Current position
    spawnX: 500, spawnY: 500, spawnZ: 300,  // Spawn point (for respawn & wander clamping)
    wanderRadius: 300,
    respawnMs: 10000,
    targetPlayerId: null,          // Reserved for future aggro
    lastAttackTime: 0,
    inCombatWith: new Set(),       // Set of character IDs attacking this enemy
    // Wander state (added by initEnemyWanderState):
    wanderTargetX: 500,
    wanderTargetY: 500,
    isWandering: false,
    nextWanderTime: Date.now() + randomPause,
    lastMoveBroadcast: 0
}
```

## Enemy ID Allocation

```javascript
let nextEnemyId = 2000001;  // Starting ID
// Each spawnEnemy() call: enemyId = nextEnemyId++
```

Enemy IDs start at 2,000,001 to avoid collision with player character IDs.

## Wandering AI

### Constants
```javascript
const ENEMY_AI = {
    WANDER_TICK_MS: 500,        // AI loop runs every 500ms
    WANDER_PAUSE_MIN: 3000,     // 3s minimum idle pause
    WANDER_PAUSE_MAX: 8000,     // 8s maximum idle pause
    WANDER_SPEED: 60,           // 60 units per second
    WANDER_DIST_MIN: 100,       // Min offset per axis
    WANDER_DIST_MAX: 300,       // Max offset per axis
    MOVE_BROADCAST_MS: 200      // Broadcast position every 200ms
};
```

### Wander Algorithm

```
Every 500ms:
    For each enemy:
        Skip if dead or inCombatWith.size > 0
        
        If NOT wandering:
            If now >= nextWanderTime:
                Pick random target point:
                    offset = WANDER_DIST_MIN + random * (DIST_MAX - DIST_MIN)
                    newX = enemy.x + offset * randomSign
                    newY = enemy.y + offset * randomSign
                    Clamp to wanderRadius from spawn point
                Set isWandering = true
        
        If wandering:
            Move toward target at WANDER_SPEED * (TICK_MS / 1000) per tick
            distance = sqrt(dx² + dy²)
            
            If distance < 10:
                Stop, set isWandering = false
                Schedule next wander (3-8s random pause)
                Broadcast final position (isMoving: false)
            Else:
                Move by stepSize = min(1, speed*dt / distance) ratio
                Broadcast position every 200ms (isMoving: true)
```

### Wander Clamping

Enemies are clamped to their `wanderRadius` from spawn point to prevent infinite drift:

```javascript
const distFromSpawn = Math.sqrt(dxFromSpawn² + dyFromSpawn²);
if (distFromSpawn > wanderRadius) {
    const ratio = wanderRadius / distFromSpawn;
    newX = spawnX + dxFromSpawn * ratio;
    newY = spawnY + dyFromSpawn * ratio;
}
```

## Combat Interaction

When a player attacks an enemy:
1. Enemy is added to `autoAttackState` with `isEnemy: true`
2. Enemy's `inCombatWith` set gets the attacker's character ID
3. Enemy's `isWandering` is set to `false` (stops wandering immediately)
4. Combat tick processes attacks at attacker's ASPD interval

When combat ends:
- On player `combat:stop_attack`: remove from `inCombatWith`
- On player disconnect: remove from `inCombatWith`
- On enemy death: clear entire `inCombatWith` set, clear all auto-attack states

While `inCombatWith.size > 0`, the enemy AI tick skips this enemy (no wandering during combat).

## Drop Tables

Each template has a `drops` array:

```javascript
drops: [
    { itemId: 2001, chance: 0.70, minQty: 1, maxQty: 2 },  // 70% chance, 1-2 items
    { itemId: 2002, chance: 0.15 },                          // 15% chance, 1 item
    { itemId: 1001, chance: 0.05 },                          // 5% chance
    { itemId: 3001, chance: 0.01 }                           // 1% chance (rare)
]
```

### Drop Rolling
```javascript
function rollEnemyDrops(enemy) {
    const droppedItems = [];
    for (const drop of template.drops) {
        if (Math.random() < drop.chance) {
            const qty = drop.minQty && drop.maxQty
                ? minQty + Math.floor(Math.random() * (maxQty - minQty + 1))
                : 1;
            droppedItems.push({ itemId, quantity, itemName });
        }
    }
    return droppedItems;
}
```

Each drop is rolled independently — a single enemy kill can yield 0 to all drops.

### Complete Drop Tables

**Blobby** (Lv1): Gloopy Residue 70%, Viscous Slime 15%, Crimson Vial 5%, Rustic Shiv 1%

**Hoplet** (Lv3): Gloopy Residue 50%, Downy Plume 30%, Verdant Leaf 10%, Crimson Vial 8%, Keen Edge 1%

**Crawlid** (Lv2): Chitin Shard 50%, Gloopy Residue 40%, Barbed Limb 25%, Crimson Vial 5%

**Shroomkin** (Lv4): Spore Cluster 55%, Verdant Leaf 20% (1-2), Crimson Vial 10%, Linen Tunic 2%

**Buzzer** (Lv5): Barbed Limb 45%, Downy Plume 30%, Amber Elixir 5%, Iron Cleaver 2%, Quilted Vest 1%, Hunting Longbow 0.5%

**Mosswort** (Lv3): Silken Tuft 60% (1-3), Verdant Leaf 25%, Viscous Slime 15%, Roasted Haunch 8%

## Death & Respawn

### Death
```
1. enemy.isDead = true
2. Clear all auto-attackers (delete from autoAttackState)
   — Do NOT send combat:target_lost (causes crash)
3. enemy.inCombatWith.clear()
4. Broadcast enemy:death to all
5. Roll drops → addItemToInventory for killer → emit loot:drop
6. setTimeout(respawn, enemy.respawnMs)
```

### Respawn
```
After respawnMs timer fires:
1. enemy.health = enemy.maxHealth
2. enemy.isDead = false
3. Reset position to spawn point
4. enemy.targetPlayerId = null
5. enemy.inCombatWith = new Set()
6. initEnemyWanderState(enemy)
7. Broadcast enemy:spawn to all
```

## Socket.io Events

| Event | Direction | When |
|-------|-----------|------|
| `enemy:spawn` | S→All | Server startup, respawn, player join (existing enemies) |
| `enemy:move` | S→All | During wander movement (every 200ms) |
| `enemy:death` | S→All | Enemy HP reaches 0 |
| `enemy:health_update` | S→All | After each hit on enemy |

---

**Last Updated**: 2026-02-17
