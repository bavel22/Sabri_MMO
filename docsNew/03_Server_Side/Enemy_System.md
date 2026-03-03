# Enemy System — Server-Side Documentation

## Overview

The enemy system manages AI-controlled monsters with Ragnarok Online pre-renewal stats and behaviors. All enemy state (health, position, combat) is tracked server-side. Enemies spawn on server startup, wander randomly within their spawn radius, and respawn after being killed.

**Monster data source**: rAthena pre-renewal database (`mob_db.yml`)

## Monster Templates

**509 Ragnarok Online monsters** loaded from `server/src/ro_monster_templates.js`, auto-generated from the rAthena pre-renewal `mob_db.yml`. Templates are imported and adapted in `index.js` via an adapter that converts RO fields to the game-compatible `ENEMY_TEMPLATES` format.

### Data Pipeline

```
rAthena mob_db.yml → scripts/extract_ro_monsters_v2.js → server/src/ro_monster_templates.js → ENEMY_TEMPLATES adapter in index.js
```

### Template Statistics

| Category | Count |
|----------|-------|
| **Total monsters** | 509 |
| Normal | 412 |
| Boss (mini-boss) | 54 |
| MVP | 43 |
| With drops | 508 |

### Level Distribution

| Range | Count | Range | Count |
|-------|-------|-------|-------|
| Lv 01-10 | 50 | Lv 51-60 | 67 |
| Lv 11-20 | 41 | Lv 61-70 | 72 |
| Lv 21-30 | 59 | Lv 71-85 | 75 |
| Lv 31-40 | 51 | Lv 86+ | 42 |
| Lv 41-50 | 52 | | |

### Race Distribution

| Race | Count | Race | Count |
|------|-------|------|-------|
| demihuman | 93 | undead | 42 |
| formless | 92 | fish | 19 |
| brute | 87 | dragon | 15 |
| demon | 66 | angel | 13 |
| insect | 55 | | |
| plant | 44 | | |

### Element Distribution

| Element | Count | Element | Count |
|---------|-------|---------|-------|
| neutral | 95 | shadow | 57 |
| earth | 98 | wind | 54 |
| fire | 61 | water | 49 |
| undead | 33 | ghost | 23 |
| poison | 20 | holy | 19 |

### AI Types

| Type | Count | Description |
|------|-------|-------------|
| passive | 113 | Only attacks when attacked first |
| aggressive | 386 | Attacks players who enter aggro range |
| reactive | 10 | Attacks when provoked, chases |

### Template Fields (per monster)

Each RO template contains:

```javascript
{
    id, aegisName, name, level, maxHealth,
    baseExp, jobExp, mvpExp,
    attack, attack2, defense, magicDefense,
    str, agi, vit, int, dex, luk,
    attackRange, aggroRange, chaseRange,
    aspd, walkSpeed, attackDelay, attackMotion, damageMotion,
    size, race, element: { type, level },
    monsterClass, aiType, respawnMs,
    raceGroups, stats, modes,
    drops: [{ itemName, rate, stealProtected }],
    mvpDrops: [{ itemName, rate }]
}
```

### Adapter (RO → Game format)

`index.js` builds `ENEMY_TEMPLATES` from `RO_MONSTER_TEMPLATES`:

| RO Field | Game Field | Conversion |
|----------|-----------|------------|
| `attack` | `damage` | Direct copy |
| `defense` | `hardDef` | Direct copy |
| `baseExp + jobExp` | `exp` | Sum |
| `attackRange` | `attackRange` | Already converted (cells × 50) |
| `drops[].rate` | `drops[].chance` | rate / 100 (% → decimal) |

## Spawn Configuration

**46 spawn points** active (zones 1-3 only), organized by 9 level zones radiating outward from origin. Zones 4-9 are disabled pending higher-level content:

| Zone | Level Range | Distance | Status | Example Monsters |
|------|-------------|----------|--------|-----------------|
| 1 - Starter Area | 1-10 | 0-800 | ✅ Active | Poring, Lunatic, Fabre, Pupa, Drops |
| 2 - Prontera Fields | 5-15 | 900-1500 | ✅ Active | Chonchon, Condor, Willow, Hornet, Rocker |
| 3 - Payon/Morroc | 10-20 | 1600-2300 | ✅ Active | Zombie, Skeleton, Poporing, Peco Peco |
| 4 - Orc/Goblin | 20-30 | 2400-3200 | ❌ Disabled | Orc Warrior, Wolf, Scorpion, Goblin |
| 5 - Byalan/Undersea | 25-40 | 3300-3900 | ❌ Disabled | Vadon, Marina, Swordfish, Marc |
| 6 - Geffen/Clock Tower | 30-50 | 4000-4700 | ❌ Disabled | Whisper, Isis, Bathory, Alarm, Rideword |
| 7 - Pyramid/Sphinx | 35-55 | 4800-5400 | ❌ Disabled | Mummy, Minorous, Sandman, Side Winder |
| 8 - Glast Heim | 50-70 | 5500-6300 | ❌ Disabled | Raydric, Khalitzburg, Anubis, Dark Priest |
| 9 - High-Level | 65-85 | 6400-7100 | ❌ Disabled | Medusa, High Orc, Anolian, Petite |
| Boss/MVP | Various | Specific | ❌ Disabled | Baphomet, Moonlight Flower, Osiris, Pharaoh |

## Enemy Data Structure

```javascript
{
    enemyId: 2000001,              // Server-assigned unique ID (starts at 2,000,001)
    templateId: 'poring',          // Template key (aegis name, lowercase)
    name: 'Poring',               // Display name
    level: 1,
    health: 50,                    // Current HP
    maxHealth: 50,
    damage: 7,                     // RO Attack (min)
    attack2: 10,                   // RO Attack2 (max)
    attackRange: 50,               // Converted from RO cells (cells × 50)
    aggroRange: 0,                 // 0 for passive mobs
    aspd: 163,
    exp: 3,                        // baseExp + jobExp
    baseExp: 2, jobExp: 1, mvpExp: 0,
    aiType: 'passive',
    monsterClass: 'normal',        // normal/boss/mvp
    size: 'medium',                // small/medium/large
    race: 'plant',                 // 10 RO races
    element: { type: 'water', level: 1 },
    hardDef: 0,                    // RO Defense (hard DEF)
    magicDefense: 5,
    walkSpeed: 400,
    attackDelay: 1872,
    attackMotion: 672,
    damageMotion: 480,
    raceGroups: {},
    modes: {},                     // aggressive, detector, assist, etc.
    stats: { str:0, agi:0, vit:0, int:0, dex:6, luk:30, level:1, weaponATK:7 },
    isDead: false,
    x: 300, y: 300, z: 300,       // Current position
    spawnX: 300, spawnY: 300, spawnZ: 300,
    wanderRadius: 400,
    respawnMs: 5500,
    targetPlayerId: null,
    lastAttackTime: 0,
    inCombatWith: new Set()
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

### RO Drop Format

Each template has a `drops` array with rAthena item names and rates:

```javascript
// In ro_monster_templates.js (raw rates as percentages):
drops: [
    { itemName: 'Jellopy', rate: 70, stealProtected: false },     // 70%
    { itemName: 'Knife ', rate: 10 },                              // 10%
    { itemName: 'Poring Card', rate: 0.01, stealProtected: true }  // 0.01%
]

// After adapter conversion in ENEMY_TEMPLATES (decimal for Math.random()):
drops: [
    { itemName: 'Jellopy', chance: 0.70, rate: 70 },
    { itemName: 'Knife ', chance: 0.10, rate: 10 },
    { itemName: 'Poring Card', chance: 0.0001, rate: 0.01, stealProtected: true }
]
```

### Drop Rolling

`rollEnemyDrops()` uses pre-resolved itemIds from the adapter for optimal performance:

```
For each drop in template.drops:
    if Math.random() < drop.chance:
        if drop.itemId exists → add to inventory directly (RO items mapped)
        if drop.itemName exists → try to match by name in itemDefinitions
            Found → add to inventory + loot notification
            Not found → loot notification only (item not yet in DB)

For MVP monsters, also roll template.mvpDrops
```

### Item Name → ID Resolution

The adapter pre-resolves RO item names to database itemIds at startup using `RO_ITEM_NAME_TO_ID` mapping:

```javascript
// In adapter (index.js)
drops: (ro.drops || []).map(d => ({
    itemName: d.itemName,
    itemId: RO_ITEM_NAME_TO_ID[d.itemName] || null,  // Pre-resolved
    chance: d.rate / 100,
    rate: d.rate,
    stealProtected: d.stealProtected || false
}))
```

**Resolution Stats**: 846 drops resolved for zones 1-3 monsters, 2927 drops unresolved (zones 4-9 not spawned).

Each drop is rolled independently — a single enemy kill can yield 0 to all drops.

### Existing Game Items as Extra Drops

15 existing game items are injected as additional drops on appropriate RO monsters:

| Item ID | Item Name | Added To Monsters | Chance |
|---------|----------|------------------|--------|
| 1001 | Crimson Vial | Poring, Lunatic, Fabre | 5% |
| 1002 | Amber Elixir | Drops, Chonchon, Condor | 3% |
| 1003 | Golden Salve | Willow, Hornet, Rocker | 2% |
| 1004 | Azure Philter | Zombie, Skeleton, Poporing | 1% |
| 1005 | Roasted Haunch | Peco Peco, Mandragora | 4% |
| 2001 | Gloopy Residue | All zone 1 monsters | 8% |
| 2002 | Viscous Slime | All slime-type monsters | 12% |
| 2003 | Chitin Shard | All insect-type monsters | 6% |
| 2004 | Downy Plume | All bird-type monsters | 7% |
| 2005 | Spore Cluster | All mushroom-type monsters | 10% |
| 2006 | Barbed Limb | All aggressive monsters | 5% |
| 2007 | Verdant Leaf | All plant-type monsters | 9% |
| 2008 | Silken Tuft | All brute-type monsters | 4% |
| 3001 | Rustic Shiv | Orc Warrior, Goblin types | 2% |
| 3002 | Keen Edge | Skeleton, undead types | 1.5% |
| 3003 | Stiletto Fang | Thief-type monsters | 1% |

These extra drops are configured in `EXISTING_ITEM_EXTRA_DROPS` and injected into `ENEMY_TEMPLATES` during server startup.

### Example Drop Tables

**Poring** (Lv1): Jellopy 70%, Knife 1%, Sticky Mucus 4%, Apple 5.5%, Empty Bottle 15%, Poring Card 0.01%, **+ Crimson Vial 5% (extra)**

**Orc Warrior** (Lv24): Orcish Voucher 90%, Iron 2.1%, Oridecon Stone 0.4%, Cigar 0.03%, Battle Axe 0.1%, Orcish Axe 0.05%, Axe 1%, Orc Warrior Card 0.01%

**Baphomet** (Lv81, MVP): Crescent Scythe 4%, Magestic Goat 3%, Emperium 5%, Elunium 54.32%, Oridecon 41.71%, Baphomet Card 0.01% + MVP drops: Yggdrasilberry 20%, Baphomet Doll 5%, Evil Horn 50%

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

| Event | Direction | When | Payload |
|-------|-----------|------|---------|
| `enemy:spawn` | S→All | Server startup, respawn, player join | `{ enemyId, templateId, name, level, health, maxHealth, monsterClass, size, race, element, x, y, z }` |
| `enemy:move` | S→All | During wander movement (every 200ms) | `{ enemyId, x, y, z, targetX, targetY, isMoving }` |
| `enemy:death` | S→All | Enemy HP reaches 0 | `{ enemyId, enemyName, killerId, killerName, isEnemy, isDead, exp, timestamp }` |
| `enemy:health_update` | S→All | After each hit on enemy | `{ enemyId, health, maxHealth, inCombat }` |
| `loot:drop` | S→Killer | After enemy death, drops rolled | `{ enemyId, enemyName, items: [{ itemId, itemName, quantity, icon, itemType, isMvpDrop }] }` |

### enemy:spawn Payload Changes (v2)

New fields added to support RO monster data on client:
- `monsterClass` — "normal", "boss", or "mvp" (for visual scaling/effects)
- `size` — "small", "medium", or "large"
- `race` — one of 10 RO races
- `element` — `{ type: "water", level: 1 }` (for elemental damage calculations)

## Design Patterns Used

- **Manager Pattern** — `enemies` Map centralizes all enemy state server-side
- **State Machine Pattern** — Enemy AI uses `isDead` + `inCombatWith.size` + `isWandering` states
- **Event-Driven Architecture** — Socket.io broadcasts for all state changes
- **Single Responsibility** — `rollEnemyDrops()` handles only drop logic, `spawnEnemy()` only spawning
- **Dependency Injection** — Templates loaded from external `ro_monster_templates.js`, not hardcoded

## Files

| File | Purpose |
|------|---------|
| `server/src/ro_monster_templates.js` | 509 RO monster templates (auto-generated, 610KB) |
| `server/src/ro_monsters_summary.json` | Summary JSON for verification |
| `server/src/ro_item_mapping.js` | RO item name → ID mapping + existing item extra drops config |
| `scripts/extract_ro_monsters_v2.js` | Extraction script (rAthena YAML → JS templates) |
| `scripts/generate_ro_items_migration.js` | Generates SQL migration + item mapping from drops |
| `database/migrations/add_ro_drop_items.sql` | 126 RO drop items migration (consumables, etc, weapons, armor, cards) |
| `server/src/index.js` | Enemy system runtime (adapter, spawn, AI, combat, drops) |

**Last Updated**: 2026-02-24 — Added 126 RO drop items, disabled zones 4-9, integrated existing items as extra drops
