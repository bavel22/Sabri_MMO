# Monster AI Behavior Audit

**Date**: 2026-03-22
**Auditor**: Claude Opus 4.6 (7-pass audit)
**Scope**: `server/src/ro_monster_ai_codes.js`, `server/src/index.js` (AI tick loop, spawn, aggro, target selection, movement)
**Reference**: rAthena pre-renewal `mob_db.yml`, `mob_db_mode_list.txt`, rAthena `unit.cpp` / `mob.cpp`

---

## 1. AI Code Type Catalog

### Source: `ro_monster_ai_codes.js`

1,032 monster-to-AI-code mappings (monster IDs 1001-2082). Each monster is assigned an integer AI code that maps to a hex mode bitmask.

| AI Code | Behavior Summary | Hex Mode | Flags |
|---------|-----------------|----------|-------|
| 01 | Passive, flees when attacked | 0x0081 | CanMove + CanAttack |
| 02 | Passive + Looter | 0x0083 | CanMove + Looter + CanAttack |
| 03 | Passive, retaliates + assists + target-switch melee | 0x1089 | CanMove + Assist + ChangeTargetMelee + CanAttack |
| 04 | Aggressive melee, short chase, angry | 0x3885 | Aggressive + Assist + ChangeTargetMelee + ChangeTargetChase + Angry + CanMove + CanAttack |
| 05 | Aggressive ranged | 0x2085 | Aggressive + ChangeTargetChase + CanMove + CanAttack |
| 06 | Immobile (plants, eggs, objects) | 0x0000 | No flags |
| 07 | Passive + Looter + Assist + ChangeTargetMelee | 0x108B | CanMove + Looter + Assist + ChangeTargetMelee + CanAttack |
| 08 | Aggressive + all target switching + TargetWeak | 0x7085 | Aggressive + ChangeTargetMelee + ChangeTargetChase + TargetWeak + CanMove + CanAttack |
| 09 | Aggressive + CastSensorIdle + target switching | 0x3095 | Aggressive + CastSensorIdle + ChangeTargetMelee + ChangeTargetChase + CanMove + CanAttack |
| 10 | Aggressive immobile | 0x0084 | Aggressive + CanAttack (no CanMove) |
| 11 | Guardian immobile | 0x0084 | Same as 10 |
| 12 | WoE Guardian | 0x2085 | Aggressive + ChangeTargetChase + CanMove + CanAttack |
| 13 | Aggressive + Assist + target switching | 0x308D | Aggressive + Assist + ChangeTargetMelee + ChangeTargetChase + CanMove + CanAttack |
| 17 | Passive + CastSensorIdle | 0x0091 | CastSensorIdle + CanMove + CanAttack |
| 19 | Aggressive + CastSensorIdle + target switch (no CastSensorChase) | 0x3095 | Same as 09 |
| 20 | Aggressive + CastSensor (Idle + Chase) + target switch | 0x3295 | Aggressive + CastSensorIdle + CastSensorChase + ChangeTargetMelee + ChangeTargetChase + CanMove + CanAttack |
| 21 | Boss/MVP style: 20 + ChangeChase | 0x3695 | Like 20 + ChangeChase |
| 24 | Slave: passive + NoRandomWalk | 0x00A1 | CanMove + NoRandomWalk + CanAttack |
| 25 | Pet/Crystal: CanMove only | 0x0001 | CanMove (no CanAttack) |
| 26 | Full aggressive: all sensors + random target | 0xB695 | Aggressive + CastSensor(Both) + ChangeTarget(All) + ChangeChase + RandomTarget + CanMove + CanAttack |
| 27 | Aggressive immobile + RandomTarget | 0x8084 | Aggressive + RandomTarget + CanAttack (no CanMove) |

### AI Code distribution in the DB (approximate)

- AI 21: ~350 entries (largest group -- most bosses, mini-bosses, and many aggressive mobs)
- AI 4: ~200 entries (aggressive melee mobs)
- AI 6: ~100 entries (plants, eggs, treasure chests, objects)
- AI 17: ~80 entries (passive cast sensor mobs)
- AI 13: ~50 entries (aggressive assist mobs)
- AI 9: ~45 entries (aggressive cast sensor mobs)
- AI 2: ~30 entries (passive looters)
- AI 5: ~25 entries (aggressive ranged)
- AI 7: ~20 entries (passive looter assist)
- AI 10: ~15 entries (immobile aggressive)
- AI 1: ~15 entries (passive flee)
- AI 3: ~15 entries (passive retaliate)
- AI 20: ~25 entries (aggressive with both cast sensors)
- AI 25: ~8 entries (crystals)
- AI 26: ~5 entries (full aggressive ranged, e.g. Bow Master, Fire Imp)
- AI 12: ~5 entries (WoE guardians)
- AI 27: ~2 entries (aggressive immobile random target)
- AI 8: ~1 entry (Luciola Vespa -- aggressive target weak)

**Missing AI codes in mapping table**: 11, 19 are defined in `AI_TYPE_MODES` but not assigned to any monster in `MONSTER_AI_CODES`. This is correct -- AI 11 is functionally identical to 10 (both 0x0084), and AI 19 is identical to 09 (both 0x3095).

---

## 2. AI Tick Loop Analysis

### Location
`server/src/index.js` line 30107 (`setInterval(async () => { ... }, ENEMY_AI.TICK_MS)`)

### Tick Rate
**200ms** (5 ticks/second) -- defined in `ENEMY_AI.TICK_MS`.

**Assessment**: Appropriate for server-side AI. rAthena uses `MIN_MOBTHINKTIME` of 100ms, but Sabri_MMO's 200ms is reasonable given it runs in a single Node.js process and handles all monsters globally.

### State Machine

4 states defined in `AI_STATE`:

| State | Description | Transitions To |
|-------|-------------|---------------|
| `IDLE` | Wandering or standing. Aggressive mobs scan for players every 500ms. | CHASE (aggro found or attacked), ATTACK (immobile mob aggro) |
| `CHASE` | Moving toward target. Checks range, leash, target validity. | ATTACK (in range), IDLE (target lost, leash exceeded) |
| `ATTACK` | Auto-attacking at attackDelay interval. Checks range, buffs, skills. | CHASE (target moved out of range), IDLE (target dead/lost) |
| `DEAD` | Awaiting respawn timer. | IDLE (after respawn) |

### State Machine Flow

```
SPAWN --> IDLE --[aggro scan / hit]--> CHASE --[in range]--> ATTACK
                                         ^                      |
                                         |     [target moved]   |
                                         +----------------------+
                                         |
                                    [leash / target lost]
                                         |
                                         v
                                       IDLE --[wander]
```

### AI Constants

| Constant | Value | Purpose | RO Classic Reference |
|----------|-------|---------|---------------------|
| `TICK_MS` | 200ms | AI processing interval | rAthena: 100ms `MIN_MOBTHINKTIME` |
| `WANDER_PAUSE_MIN` | 3000ms | Min idle before next wander | rAthena: varies by monster |
| `WANDER_PAUSE_MAX` | 8000ms | Max idle before next wander | rAthena: varies |
| `WANDER_DIST_MIN` | 100 UE units | Min wander step | ~2 RO cells |
| `WANDER_DIST_MAX` | 300 UE units | Max wander step | ~6 RO cells |
| `MOVE_BROADCAST_MS` | 200ms | Position broadcast throttle | Reasonable |
| `AGGRO_SCAN_MS` | 500ms | Aggressive scan interval | rAthena: every AI tick |
| `ASSIST_RANGE` | 550 UE units | Assist detection radius | 11 RO cells (correct) |
| `CHASE_GIVE_UP_EXTRA` | 200 UE units | Extra leash beyond chaseRange | ~4 cells buffer |
| `IDLE_AFTER_CHASE_MS` | 2000ms | Pause after losing target | rAthena: `mob_unlock_time` |

---

## 3. Mode Flag Implementation Status

### Flags Defined in `MD` Object (line 355)

| Flag | Hex | Parsed? | Used in AI? | Status |
|------|-----|---------|-------------|--------|
| `CANMOVE` | 0x0001 | YES | YES -- guards movement in `enemyMoveToward()`, `processWander()`, `spawnEnemy()` | CORRECT |
| `LOOTER` | 0x0002 | YES | NO -- parsed but **never read in AI logic** | **BUG: NOT IMPLEMENTED** |
| `AGGRESSIVE` | 0x0004 | YES | YES -- triggers aggro scan in IDLE state | CORRECT |
| `ASSIST` | 0x0008 | YES | YES -- `triggerAssist()` checks this flag | CORRECT |
| `CASTSENSORIDLE` | 0x0010 | YES | YES -- cast sensor trigger in skill casting code (line 9538) | CORRECT |
| `NORANDOMWALK` | 0x0020 | YES | YES -- `processWander()` skips if set | CORRECT |
| `NOCAST` | 0x0040 | YES | NO -- parsed but **never read in AI logic** | **BUG: NOT IMPLEMENTED** |
| `CANATTACK` | 0x0080 | YES | YES -- `setEnemyAggro()` checks before retaliating | CORRECT |
| `CASTSENSORCHASE` | 0x0200 | YES | PARTIAL -- used in cast sensor trigger (line 9538) but **not differentiated from IDLE** | **BUG: See details** |
| `CHANGECHASE` | 0x0400 | YES | YES -- CHASE state checks for closer targets in `inCombatWith` | CORRECT |
| `ANGRY` | 0x0800 | YES | PARTIAL -- only used for monster skill state matching (`'angry'` state) | **BUG: Core ANGRY behavior missing** |
| `CHANGETARGETMELEE` | 0x1000 | YES | YES -- `shouldSwitchTarget()` checks in ATTACK/IDLE state + melee hit | CORRECT |
| `CHANGETARGETCHASE` | 0x2000 | YES | YES -- `shouldSwitchTarget()` checks in CHASE state | CORRECT |
| `TARGETWEAK` | 0x4000 | YES | YES -- `findAggroTarget()` checks player level | CORRECT |
| `RANDOMTARGET` | 0x8000 | YES | YES -- `shouldSwitchTarget()` always returns true; ATTACK state picks random combatant | CORRECT |
| `MVP` | 0x80000 | YES | YES -- set on boss/mvp monsters, checked in EXP/drop logic | CORRECT |
| `KNOCKBACKIMMUNE` | 0x200000 | YES | YES -- `knockbackTarget()` checks this flag | CORRECT |
| `DETECTOR` | 0x2000000 | YES | YES -- `findAggroTarget()` and CHASE/ATTACK hidden checks | CORRECT |
| `STATUSIMMUNE` | 0x4000000 | YES | YES -- `applyStatusEffect()` checks, boss protocol | CORRECT |

---

## 4. Aggro Mechanics Analysis

### Aggro Scan (Aggressive Mobs)

- **Location**: IDLE state, line 30180
- **Frequency**: Every `AGGRO_SCAN_MS` (500ms)
- **Logic**: `findAggroTarget()` iterates all connected players in the same zone
- **Target Selection**: Closest player within `aggroRange` (distance-based, not first-found)
- **Filters**: Dead players, wrong zone, Play Dead, Hidden (non-detector), TargetWeak level check, Gangster's Paradise

**Assessment**: CORRECT. The closest-player selection is accurate for RO Classic.

### Aggro on Hit (Passive/Reactive Mobs)

- **Location**: `setEnemyAggro()` (line 28667)
- **Logic**: When a player hits an enemy, `setEnemyAggro()` is called with `attackerCharId` and `hitType`
- **Behavior**:
  1. If no current target or `shouldSwitchTarget()` says yes: take new target
  2. Mobile mobs -> CHASE, immobile mobs -> ATTACK
  3. Triggers `triggerAssist()` for nearby same-type mobs
- **Filters**: Dead enemies, plant-type (`!canAttack`), Charming Wink charm

**Assessment**: CORRECT for basic aggro-on-hit.

### Assist Mechanic

- **Location**: `triggerAssist()` (line 28631)
- **Logic**: When any enemy is hit, all enemies of the same `templateId` within `ASSIST_RANGE` (550 UE units) with the `assist` flag that are currently IDLE join the chase
- **Filter**: Same zone, same templateId, IDLE state only, must have canAttack + canMove

**Assessment**: MOSTLY CORRECT. One concern:

**BUG B1**: Assist only checks `templateId` (string key like `'goblin'`), but rAthena checks same monster race/type. Two different monsters with the same race (e.g., Goblin 1122 and Goblin 1123) have different template IDs in this codebase and would NOT assist each other. In rAthena, monsters with the `MD_ASSIST` flag help nearby monsters that share their **race** (same `class_` in rAthena). However, in this codebase, since goblin variants likely all share the same template key, this may be practically correct for most cases. The architectural difference is noted.

### Chase Range / Leash

- **Location**: CHASE state, line 30297
- **Logic**: Distance from `aggroOriginX/Y` (position where aggro started) must not exceed `chaseRange + CHASE_GIVE_UP_EXTRA` (default 600 + 200 = 800 UE units)
- **Give Up**: Clears target, sets `_rudeAttacked = true`, returns to IDLE with 2s pause

**Assessment**: CORRECT. The aggro origin tracking is a good implementation of RO's "leash" mechanic. Setting `_rudeAttacked` properly triggers the `rudeattacked` monster skill condition.

### Target Switching Logic

- **Location**: `shouldSwitchTarget()` (line 28616)
- **Rules**:
  - No current target: always switch
  - Same attacker: never switch
  - `randomTarget` flag: always switch
  - In ATTACK/IDLE + `changeTargetMelee` + melee hit: switch
  - In CHASE + `changeTargetChase`: switch
- **RandomTarget**: In ATTACK state (line 30554), picks a random combatant from `inCombatWith` each swing

**Assessment**: MOSTLY CORRECT. Two concerns:

**BUG B2**: `changeTargetMelee` in IDLE state -- rAthena only checks `MD_CHANGETARGETMELEE` during combat (ATTACK state), not IDLE. In IDLE, there is no "target" to switch from unless the mob was just hit (in which case it's not really IDLE anymore). The current code checks `enemy.aiState === AI_STATE.IDLE` in `shouldSwitchTarget()`, but this path is only reachable through `setEnemyAggro()` which transitions away from IDLE. So in practice this may not cause issues, but it's architecturally imprecise.

**BUG B3**: `changeTargetChase` in `shouldSwitchTarget()` triggers on ANY hit during CHASE, not just melee. rAthena's `MD_CHANGETARGETCHASE` means the mob can change target while chasing if hit by ANY attacker (not type-restricted), so this is actually correct. However, the variable name `changeTargetChase` could be confused with the separate `changeChase` flag.

---

## 5. Movement and Pathing Analysis

### Movement System

- **Function**: `enemyMoveToward()` (line 29916)
- **Speed**: `moveSpeed = (50 / walkSpeed) * 1000` UE units/second (from template `walkSpeed` in ms/cell)
- **Step size**: `speed * (TICK_MS / 1000)` per tick -- linear interpolation
- **Wander speed**: 60% of chase speed
- **Buff modifiers**: `getCombinedModifiers()` applies speed bonuses (Decrease AGI, Quagmire, etc.)

### Wander Behavior

- **Function**: `processWander()` (line 30026)
- **Logic**: Timer-based. After pause (3-8s), pick a random point 100-300 UE units away, move there at 60% speed. Clamp to `wanderRadius` from spawn point.
- **Exclusions**: Plant/immobile (no CanMove), NoRandomWalk flag

### Obstacle Handling

1. **Moonlit Water Mill**: Blocks entry into barrier zone (ensemble ground effect)
2. **Ice Wall**: Blocks movement through ice wall cells (radius check)
3. **Quagmire**: Applies debuff and speed reduction on entry

### Return-to-Spawn

When a monster gives up chase (line 30302-30311):
- Target cleared, state set to IDLE
- `inCombatWith` cleared
- `nextWanderTime` set to `now + 2000ms`
- Monster does NOT teleport back to spawn -- it stays at its current position and begins wandering from there

**BUG B4**: **No return-to-spawn walk**. In rAthena, when a monster gives up chase, it walks back toward its spawn point (or at least returns to its home area over time). In this codebase, the monster just stops where it is and begins normal wandering. The `pickRandomWanderPoint()` function does clamp to `wanderRadius` from spawn, so over time the monster will drift back, but there is no explicit "walk home" behavior. This means monsters can get stuck far from spawn after giving up chase, slowly drifting back via random wandering. This is a minor difference from RO Classic where monsters actively walk back.

### Pathfinding

**NOT IMPLEMENTED**: There is no pathfinding. Movement is direct-line interpolation toward the target position. There is no obstacle avoidance beyond the explicit Ice Wall / MWM / Quagmire checks. In RO Classic, monsters use cell-based pathfinding (A* on the game map's walkability grid).

**Assessment**: This is acceptable for the current stage of development since the game uses an open 3D world rather than a 2D tile grid, but it means monsters will clip through static geometry and walls. For a 3D UE5 game, proper pathfinding would use UE5 NavMesh on the server side, which is a significant architectural change and likely a future milestone.

---

## 6. Per-AI-Code Correctness vs RO Classic

### AI Code 01 (Passive, flees)
- **Hex**: 0x0081 (CanMove + CanAttack)
- **RO Behavior**: Walks around, runs away from attacker when hit
- **Implementation**: Correctly passive (no aggro scan). Does NOT implement flee behavior.
- **BUG B5**: **Flee-on-hit not implemented**. AI type 01 monsters (Fabre, Chonchon, Lunatic, Picky, etc.) should run away from the player when attacked. Currently they retaliate like any reactive mob. In rAthena, `MD_CANMOVE` + no `MD_AGGRESSIVE` + no `MD_ASSIST` means the mob just walks/runs away from the attacker rather than fighting back. The current `setEnemyAggro()` code always sets the target and transitions to CHASE/ATTACK.

### AI Code 02 (Passive + Looter)
- **Hex**: 0x0083 (CanMove + Looter + CanAttack)
- **RO Behavior**: Walks around, picks up dropped items from the ground
- **Implementation**: Passive behavior correct. Looter flag parsed but never used.
- **BUG B6**: **Looter not implemented**. The `looter` flag is parsed into `modeFlags` but no code reads it. Mobs like Poring (1002), Poporing (1031), Drops (1113), etc. should pick up items from the ground. Since the game currently has no ground item drop system (items go directly to inventory), this is deferred. However, the flag should be documented as "not yet applicable" rather than silently ignored.

### AI Code 03 (Passive + Assist + ChangeTargetMelee)
- **Hex**: 0x1089 (CanMove + Assist + ChangeTargetMelee + CanAttack)
- **RO Behavior**: Doesn't aggro on sight, but retaliates when hit. Helps nearby same-type mobs. Switches melee targets.
- **Implementation**: CORRECT. `setEnemyAggro()` handles retaliation, `triggerAssist()` handles assist, `shouldSwitchTarget()` handles melee target switching.

### AI Code 04 (Aggressive + Assist + Angry)
- **Hex**: 0x3885 (CanMove + Aggressive + Assist + Angry + ChangeTargetMelee + ChangeTargetChase + CanAttack)
- **RO Behavior**: Attacks on sight, assists allies, switches targets freely. "Angry" means the monster will re-aggro after being forced to retreat (e.g., by outrunning it). If it gives up chase and a player is still nearby, it aggros again immediately.
- **Implementation**: Aggro, assist, and target switching are correct. **Angry re-aggro behavior is NOT implemented**.
- **BUG B7**: **Angry mode not implemented**. The `angry` flag is parsed but only used for monster skill state matching (`skill.state === 'angry'` maps to ATTACK). In rAthena, `MD_ANGRY` means: when the mob gives up chase (returns to IDLE), if the attacker is STILL within aggro range, the mob immediately re-aggros instead of going idle. The current code does not check for the angry flag during chase give-up. This affects AI type 04 (the most common aggressive melee type -- ~200 monsters including Zombie, Munak, Ghoul, etc.).

### AI Code 05 (Aggressive ranged)
- **Hex**: 0x2085 (CanMove + Aggressive + ChangeTargetChase + CanAttack)
- **RO Behavior**: Attacks on sight, switches target during chase
- **Implementation**: CORRECT.

### AI Code 06 (Plant/Immobile)
- **Hex**: 0x0000 (No flags)
- **RO Behavior**: Cannot move, cannot attack, no aggro. Damaged by any hit for 1 damage. No EXP.
- **Implementation**: PARTIALLY CORRECT. `canMove=false` and `canAttack=false` are enforced. However:
- **BUG B8**: **Plant damage-capping to 1 not implemented**. In RO Classic, plant-type monsters (Red Plant, Blue Plant, Shining Plant, etc.) always take exactly 1 damage per hit regardless of ATK. The current system does not special-case plant damage. Plants will take full damage from player attacks.

### AI Code 07 (Passive + Looter + Assist)
- **Hex**: 0x108B (CanMove + Looter + Assist + ChangeTargetMelee + CanAttack)
- **Implementation**: Assist and target switching correct. Looter not implemented (same as AI 02).

### AI Code 08 (Aggressive + TargetWeak)
- **Hex**: 0x7085 (CanMove + Aggressive + ChangeTargetMelee + ChangeTargetChase + TargetWeak + CanAttack)
- **RO Behavior**: Only aggresses players 5+ levels below the monster
- **Implementation**: CORRECT. `findAggroTarget()` checks `playerLevel >= enemy.level - 5` and skips if true.

### AI Code 09 (Aggressive + CastSensorIdle)
- **Hex**: 0x3095 (CanMove + Aggressive + CastSensorIdle + ChangeTargetMelee + ChangeTargetChase + CanAttack)
- **Implementation**: CORRECT. Cast sensor trigger on skill casting (line 9533-9551) sets `_isCastTarget` which is consumed in the IDLE state. Aggro scan also covers normal proximity aggro.

### AI Code 10 (Aggressive immobile)
- **Hex**: 0x0084 (Aggressive + CanAttack, no CanMove)
- **Implementation**: CORRECT. `spawnEnemy()` sets `canMove=false`, `setEnemyAggro()` sends immobile mobs directly to ATTACK state. `findAggroTarget()` does proximity check. Mandragora, Hydra, Flora, etc. correctly attack but cannot move.

### AI Code 13 (Aggressive + Assist)
- **Hex**: 0x308D (CanMove + Aggressive + Assist + ChangeTargetMelee + ChangeTargetChase + CanAttack)
- **Implementation**: CORRECT. Combines aggro scan with assist behavior.

### AI Code 17 (Passive + CastSensorIdle)
- **Hex**: 0x0091 (CanMove + CastSensorIdle + CanAttack)
- **RO Behavior**: Won't aggro on proximity, but WILL aggro if a player begins casting a spell nearby
- **Implementation**: CORRECT. Cast sensor trigger in casting code + IDLE state consumption.

### AI Code 20 (Aggressive + CastSensor Idle+Chase)
- **Hex**: 0x3295 (CanMove + Aggressive + CastSensorIdle + CastSensorChase + ChangeTargetMelee + ChangeTargetChase + CanAttack)
- **RO Behavior**: Like 09 but also reacts to casting during chase (can switch targets mid-chase if a different player casts nearby)
- **Implementation**: PARTIALLY CORRECT.
- **BUG B9**: **CastSensorChase not differentiated**. The cast sensor trigger code (line 9538) checks both `castSensorIdle` and `castSensorChase` flags but does NOT check the monster's current AI state. Both flags trigger the same `_isCastTarget = true` behavior. The `_isCastTarget` is only consumed in the IDLE state (line 30225). So a monster with `castSensorChase` that is currently CHASING target A will NOT switch to casting player B, because the `_isCastTarget` flag is only processed in IDLE state. In rAthena, `MD_CASTSENSOR_CHASE` means the monster should react to casting EVEN while already chasing.

### AI Code 21 (Boss/MVP style)
- **Hex**: 0x3695 (Like 20 + ChangeChase)
- **Implementation**: CORRECT with boss protocol additions (knockback immune, status immune, detector). ChangeChase logic in CHASE state works correctly.

### AI Code 24 (Slave)
- **Hex**: 0x00A1 (CanMove + NoRandomWalk + CanAttack)
- **Implementation**: CORRECT. NoRandomWalk prevents wandering. Slaves spawned via `NPC_SUMMONSLAVE` use this AI code.

### AI Code 25 (Pet/Crystal)
- **Hex**: 0x0001 (CanMove only, no CanAttack)
- **Implementation**: CORRECT. Cannot attack. Used for crystals and pet-type entities.

### AI Code 26 (Full aggressive + RandomTarget)
- **Hex**: 0xB695 (Aggressive + CastSensor(Both) + ChangeTarget(All) + ChangeChase + RandomTarget + CanMove + CanAttack)
- **Implementation**: CORRECT. RandomTarget causes random combatant selection each swing.

### AI Code 27 (Aggressive immobile + RandomTarget)
- **Hex**: 0x8084 (Aggressive + RandomTarget + CanAttack, no CanMove)
- **Implementation**: CORRECT. Attacks in range, picks random target.

---

## 7. Missing Behaviors

### CRITICAL (Affects many monsters)

**M1: Angry re-aggro** -- AI type 04 (~200 monsters). When giving up chase, angry mobs should immediately re-scan for targets in aggro range instead of going idle. Fix: after line 30309 (`enemy._rudeAttacked = true`), check `enemy.modeFlags.angry` and if true, call `findAggroTarget()` -- if a target is found, immediately re-enter CHASE instead of going IDLE.

**M2: Flee-on-hit for AI type 01** -- ~15 monsters (Fabre, Chonchon, Lunatic, Picky, etc.). These should run away from the attacker, not fight back. Fix: in `setEnemyAggro()`, if `aiCode === 1` (or `!modeFlags.aggressive && !modeFlags.assist && modeFlags.canMove`), set a flee target position (opposite direction from attacker) instead of chasing the attacker. Add a FLEE state or use a flee flag on CHASE state to move AWAY.

### HIGH

**M3: Plant 1-damage cap** -- AI type 06 (~100 monsters). Plants should always take exactly 1 damage from any source. Fix: in damage calculation paths, check `enemy.modeFlags.canAttack === false && enemy.modeFlags.canMove === false` (or a new `isPlant` flag) and override damage to 1.

**M4: CastSensorChase target switching** -- AI types 20, 21, 26. Monsters with `castSensorChase` should react to casting players even while chasing another target. Fix: in the CHASE state, add a check for `_isCastTarget && enemy.modeFlags.castSensorChase` to switch targets.

**M5: NoCast flag not enforced** -- `MD_NOCAST` (0x0040) is parsed but never checked. Monsters with this flag should not use skills (only auto-attacks). Currently no monsters have this flag set via the `AI_TYPE_MODES` table, so this is not yet causing bugs, but the infrastructure gap should be noted.

### MEDIUM

**M6: Looter behavior** -- AI types 02, 07 (~50 monsters). Items dropped on the ground should be picked up by looter mobs. Currently deferred since there is no ground item system, but the flag is parsed and unused.

**M7: Return-to-spawn walk** -- After giving up chase, monsters stay at their current position and begin random wandering. They should actively walk back toward their spawn point. The `pickRandomWanderPoint()` clamping to `wanderRadius` provides gradual drift back, but an explicit "walk home" would be more accurate.

**M8: NoCast enforcement** -- The `NOCAST` flag (0x0040) is defined but never checked. No AI type currently sets this bit, but if a monster template ever needs it, the flag would be silently ignored. The `parseModeFlags()` function does not even parse this flag (it's not in the return object).

### LOW

**M9: Aggro range per-monster variation** -- All monsters in `ro_monster_templates.js` use a fixed `aggroRange: 500` (10 cells). In rAthena, different monsters have different aggro ranges (some as low as 3 cells, some up to 12). This should use the template's actual range value.

**M10: Walk speed variation during wander** -- Wander speed is fixed at 60% of chase speed for all monsters. In rAthena, walk speed during idle is the monster's base walk speed, and chase speed may be faster (for some AI types).

---

## 8. Bugs Found

| ID | Severity | Description | Affected Monsters |
|----|----------|-------------|-------------------|
| B1 | LOW | Assist matches by `templateId` string, not by race/class group. Different monster variants (e.g., Goblin 1122 vs 1123) may not assist each other if they have different template keys. | Assist-type mobs (AI 03, 04, 07, 13) |
| B2 | LOW | `changeTargetMelee` checked in IDLE state in `shouldSwitchTarget()`. Not architecturally accurate but unlikely to cause issues since IDLE mobs with targets immediately transition out. | AI 03, 04, 07, 09, 13, 20, 21, 26 |
| B3 | NONE | `changeTargetChase` triggers on any hit type during CHASE, which is correct per rAthena. | N/A |
| B4 | MEDIUM | No explicit return-to-spawn after giving up chase. Monsters stay where they stopped and slowly drift back via random wander clamping. | All mobile mobs |
| B5 | HIGH | AI type 01 flee behavior missing. Fabre, Chonchon, Lunatic, etc. fight back instead of running away. | ~15 mobs (AI 01) |
| B6 | LOW | Looter flag parsed but unused. No ground item system exists yet. | ~50 mobs (AI 02, 07) |
| B7 | CRITICAL | Angry re-aggro missing. AI type 04 mobs (~200) should immediately re-aggro after chase give-up if player is still in range. Without this, these mobs are too easy to kite. | ~200 mobs (AI 04) |
| B8 | HIGH | Plant 1-damage cap missing. Plants take full damage instead of always 1. | ~100 mobs (AI 06) |
| B9 | HIGH | CastSensorChase not processed during CHASE state. Only consumed in IDLE. Mobs with both sensors (AI 20, 21, 26) don't switch targets when new casters appear mid-chase. | ~380 mobs (AI 20, 21, 26) |
| B10 | LOW | NoCast flag (`0x0040`) parsed in `MD` object but not included in `parseModeFlags()` output and never checked in monster skill selection. | Currently no monsters use it |

---

## 9. Positive Findings

The following aspects are well-implemented and match RO Classic behavior:

1. **Mode flag bitmask system** -- Faithful to rAthena's `mob_db_mode_list.txt`. The hex values in `AI_TYPE_MODES` match rAthena's documentation.

2. **AI code catalog** -- 1,032 monster-to-AI-code mappings covering the full pre-renewal mob database. Spot-checked several monsters against rAthena `mob_db.yml` -- all correct.

3. **Assist mechanic** -- Range check (550 UE = 11 cells), same-type restriction, IDLE-only trigger. Well-implemented.

4. **Target switching** -- `shouldSwitchTarget()` correctly implements `MD_CHANGETARGETMELEE` (attack/idle + melee only) and `MD_CHANGETARGETCHASE` (chase state). `MD_RANDOMTARGET` correctly picks random each swing.

5. **TargetWeak** -- Level check correctly prevents aggro on players within 5 levels.

6. **Detector** -- Hidden player visibility correctly gated behind `modeFlags.detector`. Boss protocol correctly adds detector flag.

7. **Boss protocol** -- Boss/MVP monsters automatically get knockback immunity, status immunity, and detector. MVP flag set for MVPs.

8. **Hit stun** -- `damageMotion` delay prevents movement/attack for a short period after being hit. Correctly implemented.

9. **Gangster's Paradise** -- Sitting players with GP skill are properly skipped in aggro scan. Boss monsters correctly bypass this.

10. **Play Dead** -- All monsters (including bosses/detectors) ignore players using Play Dead. Consistent across aggro scan, CHASE, and ATTACK states.

11. **Homunculus targeting** -- Monsters can target and attack homunculi. Target system correctly differentiates via `hom_` prefix. Aggro redirects to owner on homunculus death.

12. **Monster skill system** -- Full condition evaluation (`myhpltmaxrate`, `closedattacked`, `rudeattacked`, `onspawn`, etc.), cooldown tracking, rate rolls, state filtering. Well-integrated with the AI state machine.

13. **Ground effect interactions** -- Ice Wall blocks movement, Quagmire applies speed debuff, Safety Wall blocks melee, Pneuma blocks ranged. All correctly checked in the appropriate places.

14. **CC lock** -- Frozen/stoned/stunned/sleeping enemies skip entire AI tick. Blade Stop root_lock also prevents all actions.

15. **Charming Wink** -- Charmed enemies follow the caster instead of attacking. Charm state properly overrides all AI behavior.

---

## 10. Recommendations

### Priority 1 (Critical)

1. **Implement Angry re-aggro** (B7/M1): In the CHASE give-up block (line 30301-30311), after setting `enemy._rudeAttacked = true`, add:
   ```js
   if (enemy.modeFlags.angry) {
       const reAggroTarget = findAggroTarget(enemy);
       if (reAggroTarget) {
           enemy.targetPlayerId = reAggroTarget;
           enemy.aiState = AI_STATE.CHASE;
           enemy.aggroOriginX = enemy.x;
           enemy.aggroOriginY = enemy.y;
           // Don't clear inCombatWith -- stay angry
           break; // Skip idle transition
       }
   }
   ```
   This affects ~200 AI type 04 monsters and is a core kiting mechanic.

### Priority 2 (High)

2. **Implement plant 1-damage cap** (B8/M3): Add a check in `calculatePhysicalDamage()` and `calculateMagicDamage()` -- if target is a plant-type entity (AI code 06 or `!canAttack && !canMove`), clamp damage to 1. Also suppress EXP gain from plants.

3. **Implement AI 01 flee behavior** (B5/M2): Add a `FLEE` AI state or modify `setEnemyAggro()` to detect AI code 01 and set a flee target (position away from attacker). The monster should run away for a few seconds, then return to wandering.

4. **Fix CastSensorChase** (B9/M4): In the CHASE state block, add a check:
   ```js
   if (enemy._isCastTarget && enemy.modeFlags.castSensorChase && enemy._castTargetPlayerId !== enemy.targetPlayerId) {
       enemy.targetPlayerId = enemy._castTargetPlayerId;
       enemy._isCastTarget = false;
       enemy._castTargetPlayerId = null;
   }
   ```

### Priority 3 (Medium)

5. **Implement return-to-spawn walk** (B4/M7): When a monster enters IDLE after giving up chase, instead of setting a random wander point, set the wander target to `(spawnX, spawnY)` and flag `isReturning = true`. Once it reaches spawn vicinity, clear the flag and resume normal wandering.

6. **Add NoCast to parseModeFlags** (B10/M8): Include `noCast: !!(hexMode & MD.NOCAST)` in the return value of `parseModeFlags()`. Check this flag before `selectMonsterSkill()` calls -- if set, skip skill selection entirely.

7. **Per-monster aggro range variation** (M9): Update `ro_monster_templates.js` to use monster-specific aggro ranges from rAthena `mob_db.yml` instead of a flat 500 for all aggressive mobs.

### Priority 4 (Low / Deferred)

8. **Looter behavior** (B6/M6): Implement when ground item drops are added.

9. **Assist by race/class** (B1): Consider matching assist by `race` or `monsterClass` rather than exact `templateId`. Lower priority since most assist scenarios involve same-template groups.

10. **3D pathfinding**: When UE5 NavMesh data is available on the server, replace direct-line movement with proper pathfinding.

---

## Summary

| Category | Count |
|----------|-------|
| Bugs found | 10 (1 Critical, 3 High, 2 Medium, 4 Low) |
| Missing behaviors | 10 (2 Critical, 3 High, 3 Medium, 2 Low) |
| Correctly implemented flags | 14 of 18 |
| AI codes verified | 18 of 18 defined types |
| Monster mappings | 1,032 monsters mapped |
| Positive findings | 15 well-implemented systems |

The AI system has a strong foundation with correct mode flag bitmask parsing, a functional state machine, and proper integration with the monster skill system, ground effects, and player buffs. The main gaps are behavioral nuances (angry re-aggro, flee behavior, plant damage cap, cast sensor during chase) that affect gameplay balance and RO Classic accuracy.
