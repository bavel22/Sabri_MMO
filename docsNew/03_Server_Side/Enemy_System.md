# Enemy System — Server-Side Documentation

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Monster_Skill_System](Monster_Skill_System.md) | [Combat_System](Combat_System.md) | [EXP_Leveling_System](EXP_Leveling_System.md)
> **RO Reference**: [RagnaCloneDocs/04_Monsters_EnemyAI.md](../../RagnaCloneDocs/04_Monsters_EnemyAI.md)

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
    inCombatWith: new Set(),
    // --- RO Classic AI fields (added 2026-03-04) ---
    aiCode: 2,                     // rAthena AI type code (1-27)
    modeFlags: {                   // Parsed from AI_TYPE_MODES[aiCode]
        canMove: true, looter: true, aggressive: false, assist: false,
        castSensorIdle: false, noRandomWalk: false, noCast: false, canAttack: true,
        castSensorChase: false, changeChase: false, angry: false,
        changeTargetMelee: false, changeTargetChase: false,
        targetWeak: false, randomTarget: false,
        mvp: false, knockbackImmune: false, detector: false, statusImmune: false
    },
    moveSpeed: 125,                // UE units/sec: (50 / walkSpeed) * 1000
    chaseRange: 600,               // Max chase distance from aggroOrigin
    aiState: 'idle',               // AI_STATE: 'idle', 'chase', 'attack', 'dead'
    aggroOriginX: null,            // Position when aggro started (for chase range check)
    aggroOriginY: null,
    lastDamageTime: 0,             // For hit stun (damageMotion ms of inaction)
    lastAggroScan: 0,              // Throttle aggressive mob scans (every 500ms)
    pendingTargetSwitch: null      // { charId, hitType } — consumed by AI tick
}
```

## Enemy ID Allocation

```javascript
let nextEnemyId = 2000001;  // Starting ID
// Each spawnEnemy() call: enemyId = nextEnemyId++
```

Enemy IDs start at 2,000,001 to avoid collision with player character IDs.

## RO Classic AI System

### AI Code Lookup

Each monster is assigned an rAthena AI type code (1-27) that determines its behavior. Codes are loaded from `server/src/ro_monster_ai_codes.js` (1,004 monster ID → AI code mappings). Monsters not in the lookup table fall back to a default based on their `aiType` field: passive→1, aggressive→5, reactive→3.

```javascript
const aiCode = (MONSTER_AI_CODES && MONSTER_AI_CODES[roId]) || getDefaultAiCode(template.aiType);
const hexMode = AI_TYPE_MODES[aiCode] || 0x0081;
const modeFlags = parseModeFlags(hexMode);
```

### Mode Flags (MD Bitmask)

Each AI code maps to a hex bitmask in `AI_TYPE_MODES`. The bitmask is parsed into boolean flags:

| Flag | Hex | Effect |
|------|-----|--------|
| `canMove` | 0x0001 | Monster can move (false = plant/immobile) |
| `looter` | 0x0002 | Picks up items from ground |
| `aggressive` | 0x0004 | Attacks players on sight within aggroRange |
| `assist` | 0x0008 | Joins combat when same-type ally is attacked nearby |
| `castSensorIdle` | 0x0010 | Detects skill casting while idle |
| `noRandomWalk` | 0x0020 | Never wanders (stays at spawn) |
| `canAttack` | 0x0080 | Can perform attacks (false = cannot fight back) |
| `changeChase` | 0x0400 | Switches to closer target while chasing |
| `angry` | 0x0800 | Uses Angry skill behavior (KE/Lex Aeterna triggers) |
| `changeTargetMelee` | 0x1000 | Switches target when hit in ATTACK state |
| `changeTargetChase` | 0x2000 | Switches target when hit in CHASE state |
| `targetWeak` | 0x4000 | Only aggros players 5+ levels below monster |
| `randomTarget` | 0x8000 | Picks random attacker each swing |
| `mvp` | 0x80000 | MVP-class monster (special drops, announcements) |
| `knockbackImmune` | 0x200000 | Immune to knockback effects |
| `detector` | 0x2000000 | Can see hidden/cloaked players |
| `statusImmune` | 0x4000000 | Immune to status effects |

### AI Type Code Reference

| Code | Hex Mode | Behavior | Example Monsters |
|------|----------|----------|-----------------|
| 1 | 0x0081 | Passive, can move+attack | Fabre, Lunatic, Willow |
| 2 | 0x0083 | Passive, looter | Poring, Drops, Poporing |
| 3 | 0x1089 | Passive, assist, changeTargetMelee | Hornet, Condor, Wolf |
| 4 | 0x3885 | Aggressive melee | Zombie, Orc Warrior, Ghoul |
| 5 | 0x2085 | Aggressive ranged | Archer Skeleton, Goblin Archer |
| 6 | 0x0000 | Immobile (no flags) | Plants, Eggs, Treasure Chests |
| 7 | 0x108B | Passive, looter, assist | Thief Bug, Andre, Yoyo |
| 9 | 0x3095 | Aggressive, assist, castSensor | Isis, Mantis, Raydric |
| 13 | 0x308D | Aggressive, assist | Kobold, Desert Wolf, Goblin |
| 21 | 0x3695 | Boss/MVP AI | Baphomet, Osiris, Drake |
| 25 | 0x0001 | Crystal (move only) | Wind/Earth/Fire/Water Crystal |
| 26 | 0xB695 | Aggressive ranged, special | Ragged Zombie, Fire Imp |

### Constants

```javascript
const ENEMY_AI = {
    TICK_MS: 200,               // AI loop runs every 200ms (5× per second)
    WANDER_PAUSE_MIN: 3000,     // 3s minimum idle pause
    WANDER_PAUSE_MAX: 8000,     // 8s maximum idle pause
    WANDER_DIST_MIN: 100,       // Min wander offset per axis (UE units)
    WANDER_DIST_MAX: 300,       // Max wander offset per axis (UE units)
    MOVE_BROADCAST_MS: 200,     // Broadcast position every 200ms during movement
    AGGRO_SCAN_MS: 500,         // Aggressive mobs scan for players every 500ms
    ASSIST_RANGE: 550,          // Assist detection range (11 RO cells × 50 UE units)
    CHASE_GIVE_UP_EXTRA: 200,   // Extra UE units beyond chaseRange before giving up
    IDLE_AFTER_CHASE_MS: 2000   // Delay before resuming wander after losing target
};
```

### Movement Speed

Per-monster speed calculated from the RO `walkSpeed` template field:

```javascript
const moveSpeed = (50 / walkSpeedMs) * 1000;  // UE units per second
// Example: walkSpeed 400 → moveSpeed 125 u/s
// Example: walkSpeed 200 → moveSpeed 250 u/s (fast monster)
```

- **Chase**: Full `moveSpeed`
- **Wander**: 60% of `moveSpeed` (slower, relaxed movement)

### AI State Machine

```
┌────────────────────────────────────────────────────────────┐
│                    AI Tick (every 200ms)                    │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  ┌──────────┐   aggro scan    ┌──────────┐  in range     ┌──────────┐
│  │          │ ──────────────► │          │ ────────────► │          │
│  │   IDLE   │   or attacked   │  CHASE   │               │  ATTACK  │
│  │          │ ◄────────────── │          │ ◄──────────── │          │
│  └──────────┘   chase too far └──────────┘  out of range └──────────┘
│       │              │                           │
│       │              │         target dies        │
│       ▼              └───────────────────────────┘
│   (wander)                       │
│                                  ▼
│                            ┌──────────┐
│                            │   DEAD   │  → respawn → IDLE
│                            └──────────┘
└────────────────────────────────────────────────────────────┘
```

#### IDLE State

- **Aggressive mobs** (`modeFlags.aggressive`): Scan for players within `aggroRange` every 500ms via `findAggroTarget()`. Closest player becomes target → transition to CHASE.
- **TargetWeak** (`modeFlags.targetWeak`): Only aggro players 5+ levels below monster level.
- **Passive mobs**: Wander randomly at 60% moveSpeed. Clamp to `wanderRadius` from spawn.
- **Hit stun**: If `damageMotion` ms haven't elapsed since last damage, skip movement.
- **noRandomWalk**: Never wanders (stays at spawn point).

#### CHASE State

1. **Validate target**: If target disconnected/dead, `pickNextTarget()` from `inCombatWith` or go IDLE.
2. **Process pending target switch**: If `pendingTargetSwitch` was set by a damage hook, apply it.
3. **Chase range check**: If distance from `aggroOrigin` exceeds `chaseRange + 200`, give up → IDLE, clear combat.
4. **Attack range check**: If within `attackRange + RANGE_TOLERANCE` → transition to ATTACK.
5. **Move toward target**: At full `moveSpeed`, broadcast position every 200ms.
6. **ChangeChase** (`modeFlags.changeChase`): While chasing, if any combatant is within attack range, switch to them.

#### ATTACK State

1. **Validate target**: Dead/disconnected → pick next or IDLE.
2. **Process pending target switch**: Apply if present.
3. **RandomTarget** (`modeFlags.randomTarget`): Pick random alive combatant each attack cycle.
4. **Range check**: If target moved out of range → CHASE.
5. **Attack delay**: Wait `attackDelay` ms between attacks.
6. **Calculate damage**: Uses `calculateEnemyDamage()` which calls existing `roPhysicalDamage()`.
7. **Broadcast**: `combat:damage` + `enemy:attack` + `combat:health_update`.
8. **Player death**: Set `isDead`, stop auto-attacks, emit `combat:death`, save HP to DB, pick next target or IDLE.

### Aggro System

**No threat table** — RO Classic uses first-hit priority with mode-flag-based switching.

#### `setEnemyAggro(enemy, attackerCharId, hitType)`

Called from all damage paths (auto-attack, all skills, AOE, Fire Wall):

1. Skip if enemy is dead or can't attack (plant-type).
2. Add attacker to `inCombatWith`, stop wandering, record `lastDamageTime`.
3. If no current target, or in IDLE state, or `shouldSwitchTarget()` returns true: set new target.
4. If entering aggro from IDLE: record `aggroOriginX/Y`.
5. Mobile mobs → CHASE state. Immobile mobs → ATTACK state directly.
6. Call `triggerAssist()` to alert nearby same-type mobs.

#### Target Switching Rules — `shouldSwitchTarget()`

| Mode Flag | State | Behavior |
|-----------|-------|----------|
| `changeTargetMelee` | ATTACK | Switch to new attacker on each hit |
| `changeTargetChase` | CHASE | Switch to new attacker on each hit |
| `randomTarget` | ATTACK | Random combatant each swing (handled in ATTACK state) |

#### Assist System — `triggerAssist()`

When an enemy is attacked, all nearby same-type monsters with the `assist` flag join in:

```
For each enemy in enemies:
    Skip if same enemy, dead, not IDLE, different templateId, no assist flag
    Check distance: sqrt((other.x - attacked.x)² + (other.y - attacked.y)²)
    If distance <= ASSIST_RANGE (550 UE units / 11 RO cells):
        Set other.targetPlayerId = attacker
        Set other.aiState = CHASE
        Record aggroOrigin, add to inCombatWith
```

### Hit Stun (damageMotion)

When an enemy takes damage, `lastDamageTime` is recorded. For `damageMotion` milliseconds after damage, the enemy cannot move or attack (the AI tick skips movement/attack but still validates state).

### Enemy → Player Damage

`calculateEnemyDamage()` reuses the existing `roPhysicalDamage()` formula:

```javascript
const attackerStats = {
    str, agi, vit, int, dex, luk,     // From enemy.stats
    level: enemy.level,
    weaponATK: enemy.damage,
    passiveATK: 0
};
const attackerInfo = {
    weaponType: (enemy.attackRange > MELEE_RANGE) ? 'bow' : 'bare_hand',
    weaponElement: enemy.element.type || 'neutral',
    weaponLevel: 1,
    buffMods: getBuffStatModifiers(enemy)
};
return calculatePhysicalDamage(attackerStats, getEffectiveStats(player), ...);
```

### Boss Protocol

Monsters with `monsterClass: 'boss'` or `'mvp'` automatically get:
- `knockbackImmune: true` — Immune to Fire Wall knockback and other displacement
- `statusImmune: true` — Immune to Freeze, Stone Curse, etc.
- `detector: true` — Can detect hidden/cloaked players

### Combat Interaction Hooks

Aggro is triggered from **all** damage paths:

| Damage Source | Hook Location | hitType |
|---------------|--------------|---------|
| `combat:attack` handler | Player clicks enemy | `'melee'` |
| Combat tick auto-attack | Each ASPD-interval hit | `'melee'` |
| Bash | Single-target skill | `'melee'` |
| Bolt skills (Fire/Cold/Lightning) | Multi-hit skill | `'skill'` |
| Soul Strike | Multi-hit ghost skill | `'skill'` |
| Frost Diver | Single-target water skill | `'skill'` |
| Meteor Storm | AOE fire skill | `'skill'` |
| Thunderstorm | AOE wind skill | `'skill'` |
| Napalm Beat | AOE ghost skill | `'skill'` |
| Fire Ball | AOE fire skill | `'skill'` |
| Fire Wall | Persistent fire damage | `'skill'` |

### Player Disconnect Cleanup

When a player disconnects:
1. Remove from all enemies' `inCombatWith` sets.
2. For each enemy targeting the disconnected player: `pickNextTarget()` from remaining combatants, or go IDLE if none.

### Wander Algorithm (IDLE State)

Wander behavior is extracted into `processWander()`:

```
If NOT wandering AND now >= nextWanderTime:
    Pick random target: spawnX ± (100-300), spawnY ± (100-300)
    Clamp to wanderRadius from spawn
    Set isWandering = true

If wandering:
    Move toward target at 60% moveSpeed
    If distance < 10: stop, schedule next wander (3-8s)
    Else: move by stepSize, broadcast every 200ms
```

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
1. enemy.isDead = true, enemy.aiState = AI_STATE.DEAD, enemy.targetPlayerId = null
2. Clear all auto-attackers (delete from autoAttackState)
   — Do NOT send combat:target_lost (causes crash)
3. enemy.inCombatWith.clear()
4. Broadcast enemy:death to all
5. Award EXP to killer (base + job), process level ups
6. Roll drops → addItemToInventory for killer → emit loot:drop
7. setTimeout(respawn, enemy.respawnMs)
```

### Respawn
```
After respawnMs timer fires:
1. enemy.health = enemy.maxHealth
2. enemy.isDead = false
3. Reset position to spawn point
4. enemy.targetPlayerId = null
5. enemy.inCombatWith = new Set()
6. initEnemyWanderState(enemy) — resets aiState to IDLE, clears all AI fields
7. Broadcast enemy:spawn to all
```

## Socket.io Events

| Event | Direction | When | Payload |
|-------|-----------|------|---------|
| `enemy:spawn` | S→All | Server startup, respawn, player join | `{ enemyId, templateId, name, level, health, maxHealth, monsterClass, size, race, element, x, y, z }` |
| `enemy:move` | S→All | During wander/chase movement (every 200ms) | `{ enemyId, x, y, z, targetX, targetY, isMoving }` |
| `enemy:attack` | S→All | Enemy attacks a player (AI ATTACK state) | `{ enemyId, targetId, attackMotion }` |
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

| Pattern | How Applied |
|---------|-------------|
| Manager | `enemies` Map centralizes all enemy state server-side |
| State Machine | Formal `AI_STATE` enum (IDLE/CHASE/ATTACK/DEAD) with clean transitions |
| Event-Driven | Socket.io broadcasts for all state changes (move, attack, death, health) |
| Single Responsibility | `setEnemyAggro()`, `triggerAssist()`, `calculateEnemyDamage()` each do one thing |
| Dependency Injection | Templates from `ro_monster_templates.js`, AI codes from `ro_monster_ai_codes.js` |
| Data-Driven Design | Mode flags parsed from bitmask per AI code — no hardcoded per-monster behavior |
| Strategy Pattern | `shouldSwitchTarget()` evaluates mode flags to determine target switching rules |

## Files

| File | Purpose |
|------|---------|
| `server/src/ro_monster_templates.js` | 509 RO monster templates (auto-generated, 610KB) |
| `server/src/ro_monster_ai_codes.js` | 1,004 monster ID → rAthena AI type code mappings |
| `server/src/ro_monsters_summary.json` | Summary JSON for verification |
| `server/src/ro_item_mapping.js` | RO item name → ID mapping + existing item extra drops config |
| `scripts/extract_ro_monsters_v2.js` | Extraction script (rAthena YAML → JS templates) |
| `scripts/generate_ro_items_migration.js` | Generates SQL migration + item mapping from drops |
| `database/migrations/add_ro_drop_items.sql` | 126 RO drop items migration (consumables, etc, weapons, armor, cards) |
| `server/src/index.js` | Enemy system runtime (adapter, spawn, AI state machine, combat, drops) |

**Last Updated**: 2026-03-04 — RO Classic AI system: state machine, aggro, assist, target switching, enemy attacks, per-monster AI codes

## Enemy Sprite System

### Overview

Enemies can use the same `SpriteCharacterActor` 3D-to-2D sprite system as player characters. The server sends sprite metadata in `enemy:spawn` events, and the client's `EnemySubsystem` creates animated sprite actors for enemies that have sprite data configured.

### Template Fields

Two optional fields on monster templates in `ro_monster_templates.js`:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `spriteClass` | string | `undefined` | Atlas manifest name (e.g., `'skeleton'`). Maps to `Body/enemies/{name}/` in UE5 Content. If absent or empty, the enemy uses the default BP mesh. |
| `weaponMode` | number | `0` | Weapon animation set: 0=unarmed, 1=onehand, 2=twohand, 3=bow. Selects which weapon group animations to play from the atlas config. |

### Adapter

The `ENEMY_TEMPLATES` adapter in `index.js` (~line 4308) copies these fields from `RO_MONSTER_TEMPLATES`:

```javascript
spriteClass: ro.spriteClass || '',
weaponMode: ro.weaponMode || 0,
```

### enemy:spawn Emit Locations

All 4 `enemy:spawn` emit locations include `spriteClass` and `weaponMode`:

| Location | When | Notes |
|----------|------|-------|
| `spawnEnemy()` | Server startup, enemy first created | Initial broadcast to all connected clients |
| `respawnEnemy()` | After respawn timer fires | Broadcast to all clients in zone |
| `player:join` zone loop | New player joins, receives existing enemies | **Dropped during OpenLevel** -- subsystems not registered yet |
| `zone:ready` zone loop | Client signals subsystems are ready | **This is the emit clients actually receive** |

**CRITICAL**: The `zone:ready` emit is the one that matters. During `player:join`, the client is performing `OpenLevel()` -- all subsystems are destroyed and recreated, so socket events are silently dropped. By `zone:ready`, all C++ subsystem handlers are registered and will process the spawn data including sprite fields.

### enemy:spawn Payload (updated)

```javascript
{
    enemyId, templateId, name, level, health, maxHealth,
    monsterClass, size, race, element,
    x, y, z,
    spriteClass,   // NEW: atlas manifest name (e.g., 'skeleton')
    weaponMode     // NEW: 0=unarmed, 1=onehand, 2=twohand, 3=bow
}
```

### Weapon Variant System

One atlas config can define multiple weapon groups (unarmed, onehand, twohand, bow). Different monster templates sharing the same `spriteClass` can use different `weaponMode` values to select different animation sets from the same atlas. This allows a single set of atlas files to serve multiple monster variants (e.g., unarmed skeleton, skeleton archer, skeleton knight).

### Adding Sprite Support to a New Monster

1. Create 3D model, rig in Mixamo, render sprites, pack atlases (see `sabrimmo-3d-to-2d` skill)
2. Import atlas PNGs + JSONs + manifest into UE5 at `Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}/`
3. Add `spriteClass: '{name}', weaponMode: 0` to the monster's entry in `ro_monster_templates.js`
4. No server code changes needed -- adapter and all 4 emit paths already handle the fields

### Monsters with Sprites

| Monster | Template ID | spriteClass | weaponMode |
|---------|------------|-------------|------------|
| Skeleton | 1028 | `skeleton` | 0 (unarmed) |

**Last Updated**: 2026-03-26 -- Enemy sprite system: spriteClass/weaponMode fields, 4 emit locations, weapon variant system
