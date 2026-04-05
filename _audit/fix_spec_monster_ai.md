# Monster AI Fix Specifications — Round 1 Top 4 Bugs

**File**: `server/src/index.js` (32,566 lines)
**Reference**: `server/src/ro_monster_ai_codes.js` (1,034 lines, 509 monster entries)
**AI Mode Flags**: Lines 355-400 (`MD` bitmask + `AI_TYPE_MODES` table)
**AI State Machine**: Lines 30107-30353 (IDLE/CHASE/ATTACK/DEAD)

---

## Fix 1: AI Type 04 "Angry" Re-Aggro After Chase Give-Up

### Bug

In RO Classic, monsters with the `MD_ANGRY` flag (0x0800) should **immediately re-aggro** after giving up chase if a player is still in their aggro range. Currently, when an Angry monster exceeds its chase range, it transitions to IDLE and waits for the next aggro scan tick (500ms+ delay via `ENEMY_AI.AGGRO_SCAN_MS`). Worse, `inCombatWith` is cleared, so the monster has no memory of who was attacking it.

This means Angry monsters (AI type 04) are trivially kitable: run them past their leash range, they reset, stand idle for 2+ seconds, and then slowly re-scan. In RO Classic, Angry monsters are supposed to be **extremely persistent** -- they immediately re-lock and start chasing again.

### Affected Monsters

- **233 monsters** with AI code 04 in `ro_monster_ai_codes.js` (Zombie, Orc Warrior, Munak, Soldier Skeleton, Hunter Fly, Ghoul, Mummy, Marc, Swordfish, Pirate Skeleton, Poison Spore, etc.)
- Additional monsters with `MD_ANGRY` from AI types that include the flag (type 4 hex `0x3885` has `ANGRY = 0x0800`)
- Impact: These are the bread-and-butter aggressive melee monsters of early-to-mid game maps. Being trivially kitable breaks difficulty.

### Root Cause

Line 30301-30311 in the CHASE state:

```javascript
if (distFromOrigin > (enemy.chaseRange || 600) + ENEMY_AI.CHASE_GIVE_UP_EXTRA) {
    // Too far -- give up chase
    enemy.targetPlayerId = null;
    enemy.aiState = AI_STATE.IDLE;
    enemy.inCombatWith.clear();       // <-- Forgets all attackers
    enemy.isWandering = false;
    enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;  // <-- 2s idle delay
    enemyStopMoving(enemy);
    enemy._rudeAttacked = true;
    logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) gave up chase ...`);
    break;
}
```

No check for `enemy.modeFlags.angry` before fully resetting state.

### Fix

**Location**: Lines 30301-30311 in `case AI_STATE.CHASE:`

Replace the chase give-up block with an Angry re-aggro check. After determining the monster is too far from its aggro origin, if the monster has the `angry` mode flag, immediately re-scan for a player in aggro range. If found, re-lock onto them from the current position (update `aggroOriginX/Y` to current position to give a fresh chase leash). Only fall through to full IDLE reset if no player is found.

```javascript
if (distFromOrigin > (enemy.chaseRange || 600) + ENEMY_AI.CHASE_GIVE_UP_EXTRA) {
    // Too far from aggro origin -- give up chase

    // MD_ANGRY re-aggro: immediately re-scan for players in aggro range
    // RO Classic: Angry monsters re-lock targets instead of going idle,
    // making them NOT trivially kitable by running past leash range.
    if (enemy.modeFlags.angry) {
        const reAggroTarget = findAggroTarget(enemy);
        if (reAggroTarget) {
            // Re-lock: fresh chase from current position
            enemy.targetPlayerId = reAggroTarget;
            enemy.aggroOriginX = enemy.x;
            enemy.aggroOriginY = enemy.y;
            enemy.isWandering = false;
            if (!enemy.inCombatWith.has(reAggroTarget)) {
                enemy.inCombatWith.set(reAggroTarget, { totalDamage: 0, name: '' });
            }
            logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) ANGRY re-aggro → player ${reAggroTarget} (leash reset at ${Math.round(enemy.x)},${Math.round(enemy.y)})`);
            break;  // Stay in CHASE with new leash origin
        }
    }

    // No re-aggro target (or not angry) -- full idle reset
    enemy.targetPlayerId = null;
    enemy.aiState = AI_STATE.IDLE;
    enemy.inCombatWith.clear();
    enemy.isWandering = false;
    enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
    enemyStopMoving(enemy);
    enemy._rudeAttacked = true;
    logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) gave up chase (dist from origin: ${distFromOrigin.toFixed(0)})`);
    break;
}
```

### Guard Conditions

1. `findAggroTarget(enemy)` already filters by: zone match, player alive, not hidden (unless detector), not Play Dead, aggro range distance, TargetWeak level check, Gangster's Paradise. All these are respected for re-aggro.
2. The leash reset (`aggroOriginX/Y = enemy.x/y`) prevents infinite chase -- if the re-aggro target also kites past the new leash, the cycle repeats, but the monster gradually returns toward the player's area rather than chasing across the entire map.
3. `inCombatWith` is NOT cleared on re-aggro, preserving MVP damage tracking for the new target.
4. Non-angry monsters fall through to the existing idle reset unchanged.

### Gameplay Impact

- **233 AI-04 monsters** become properly persistent. Players can no longer trivially kite Zombies, Orc Warriors, Hunter Flies, etc. past their leash range for a free reset.
- Training maps (Payon Dungeon, Orc Dungeon, Pyramid) become more dangerous as intended.
- Kiting is still possible by leaving aggro range entirely (outrunning the scan radius), which is correct RO Classic behavior.

---

## Fix 2: AI Type 01 "Cowardly" Flee-on-Hit Behavior

### Bug

Monsters with AI code 01 (Passive, flees when attacked) should **run away from the attacker** when hit, not retaliate. Currently, `setEnemyAggro()` (line 28667) treats them the same as any other monster: it sets `targetPlayerId` and transitions to CHASE/ATTACK. This means Fabre, Willow, Chonchon, Spore, Lunatic, etc. all fight back when attacked, which is wrong.

In RO Classic, AI-01 monsters:
- Stand idle or wander (passive -- never aggro first)
- When hit, move **away** from the attacker for a few seconds
- Do NOT retaliate or chase the attacker
- After fleeing for a duration, return to idle wandering

### Affected Monsters

- **44 monsters** with AI code 01 in `ro_monster_ai_codes.js` (Fabre, Willow, Chonchon, Roda Frog, Spore, Creamy, Boa, Thara Frog, Picky x2, Rocker, Muka, Santa Poring, Lunatic, Megalodon, Crab, etc.)
- Impact: These are the earliest monsters new players encounter. Having them fight back breaks the "safe beginner monster" role they serve in RO Classic.

### Root Cause

`setEnemyAggro()` at line 28667 has no AI-01 flee check:

```javascript
function setEnemyAggro(enemy, attackerCharId, hitType, damageDealt) {
    if (enemy.isDead) return;
    if (!enemy.modeFlags.canAttack) return;  // Plant-type: can't fight back
    // ... no check for AI code 01 flee behavior ...
    enemy.targetPlayerId = attackerCharId;
    if (enemy.modeFlags.canMove) {
        enemy.aiState = AI_STATE.CHASE;     // <-- Should be FLEE for AI 01
    }
}
```

And the AI state machine has no FLEE state at all.

### Fix

This fix requires two parts: (A) adding a FLEE state to the AI state machine, and (B) routing AI-01 monsters into FLEE instead of CHASE when hit.

#### Part A: Add FLEE state constant and handler

**Location**: Line 407 (AI_STATE definition)

Add a new FLEE state:

```javascript
const AI_STATE = {
    IDLE:    'idle',
    CHASE:   'chase',
    ATTACK:  'attack',
    DEAD:    'dead',
    FLEE:    'flee',     // AI 01: running away from attacker
};
```

**Location**: Line 28556 (ENEMY_AI constants)

Add flee-related timing:

```javascript
const ENEMY_AI = {
    // ... existing constants ...
    FLEE_DURATION_MS: 5000,     // How long AI-01 monsters flee before returning to idle
    FLEE_SPEED_MULT: 1.5,      // Flee at 150% normal move speed
};
```

**Location**: After the CHASE case block ends (line 30353), before the ATTACK case (line 30355), add a new FLEE case:

```javascript
        // ===============================================================
        // FLEE -- AI type 01: run away from last attacker (cowardly)
        // ===============================================================
        case AI_STATE.FLEE: {
            // Flee timer expired -- return to idle
            if (!enemy._fleeUntil || now >= enemy._fleeUntil) {
                enemy.aiState = AI_STATE.IDLE;
                enemy._fleeUntil = null;
                enemy._fleeFromX = null;
                enemy._fleeFromY = null;
                enemy.targetPlayerId = null;
                enemy.isWandering = false;
                enemy.nextWanderTime = now + ENEMY_AI.WANDER_PAUSE_MIN;
                enemyStopMoving(enemy);
                logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) flee expired → IDLE`);
                break;
            }

            if (inHitStun) break;

            // Move away from the attacker's last known position
            const fleeFromX = enemy._fleeFromX || enemy.x;
            const fleeFromY = enemy._fleeFromY || enemy.y;
            const fleeDx = enemy.x - fleeFromX;
            const fleeDy = enemy.y - fleeFromY;
            const fleeDist = Math.sqrt(fleeDx * fleeDx + fleeDy * fleeDy);

            if (fleeDist < 1) {
                // Attacker is on top of us -- pick a random direction
                const angle = Math.random() * Math.PI * 2;
                const fleeTargetX = enemy.x + Math.cos(angle) * 300;
                const fleeTargetY = enemy.y + Math.sin(angle) * 300;
                const fleeMods = getCombinedModifiers(enemy);
                const fleeSpeed = enemy.moveSpeed * ENEMY_AI.FLEE_SPEED_MULT * Math.max(0.1, 1 + (fleeMods.moveSpeedBonus || 0) / 100);
                enemyMoveToward(enemy, fleeTargetX, fleeTargetY, now, fleeSpeed);
            } else {
                // Move in the opposite direction from the attacker
                const ratio = 300 / fleeDist;
                const fleeTargetX = enemy.x + fleeDx * ratio;
                const fleeTargetY = enemy.y + fleeDy * ratio;

                // Clamp to not flee infinitely far from spawn
                const wanderRadius = enemy.wanderRadius || 300;
                const maxFleeRange = wanderRadius * 3;
                const dxSpawn = fleeTargetX - enemy.spawnX;
                const dySpawn = fleeTargetY - enemy.spawnY;
                const distSpawn = Math.sqrt(dxSpawn * dxSpawn + dySpawn * dySpawn);
                let clampedX = fleeTargetX, clampedY = fleeTargetY;
                if (distSpawn > maxFleeRange) {
                    const clampRatio = maxFleeRange / distSpawn;
                    clampedX = enemy.spawnX + dxSpawn * clampRatio;
                    clampedY = enemy.spawnY + dySpawn * clampRatio;
                }

                const fleeMods = getCombinedModifiers(enemy);
                const fleeSpeed = enemy.moveSpeed * ENEMY_AI.FLEE_SPEED_MULT * Math.max(0.1, 1 + (fleeMods.moveSpeedBonus || 0) / 100);
                enemyMoveToward(enemy, clampedX, clampedY, now, fleeSpeed);
            }
            break;
        }
```

#### Part B: Route AI-01 into FLEE in `setEnemyAggro()`

**Location**: Line 28667-28706 (`setEnemyAggro` function)

Add an early-return flee path after the `canAttack` check at line 28669, before the main aggro logic:

```javascript
function setEnemyAggro(enemy, attackerCharId, hitType, damageDealt) {
    if (enemy.isDead) return;
    if (!enemy.modeFlags.canAttack) return;  // Plant-type: can't fight back

    // AI type 01 (Cowardly): flee from attacker instead of retaliating
    // RO Classic: passive monsters with AI code 01 run away when hit
    if (enemy.aiCode === 1 && enemy.modeFlags.canMove) {
        const attacker = connectedPlayers.get(attackerCharId);
        enemy._fleeFromX = attacker ? (attacker.lastX || enemy.x) : enemy.x;
        enemy._fleeFromY = attacker ? (attacker.lastY || enemy.y) : enemy.y;
        enemy._fleeUntil = Date.now() + ENEMY_AI.FLEE_DURATION_MS;
        enemy.aiState = AI_STATE.FLEE;
        enemy.targetPlayerId = null;  // No target -- just fleeing
        enemy.isWandering = false;
        // Still track damage for EXP/drop assignment
        if (!enemy.inCombatWith.has(attackerCharId)) {
            const attackerPlayer = connectedPlayers.get(attackerCharId);
            enemy.inCombatWith.set(attackerCharId, { totalDamage: 0, name: attackerPlayer ? attackerPlayer.characterName : '' });
        }
        const actualDamage = (damageDealt && damageDealt > 0) ? damageDealt : (enemy._lastDamageDealt || 0);
        if (actualDamage > 0) {
            const combatData = enemy.inCombatWith.get(attackerCharId);
            if (combatData) combatData.totalDamage += actualDamage;
            enemy._lastDamageDealt = 0;
        }
        return;  // Do NOT set target or transition to CHASE/ATTACK
    }

    // Charming Wink: charmed monsters ignore aggro until charm expires
    if (enemy.charmedUntil && enemy.charmedUntil > Date.now()) return;
    // ... rest of existing setEnemyAggro unchanged ...
```

### Guard Conditions

1. `enemy.aiCode === 1` -- only AI type 01 gets flee behavior. All other AI types (including AI 02 which stands ground, AI 03 which retaliates) are unaffected.
2. `enemy.modeFlags.canMove` -- immobile variants (shouldn't exist for AI 01, but safety check) just ignore the hit.
3. Damage tracking (`inCombatWith`) is still maintained so the correct player gets EXP/drops when the fleeing monster dies.
4. Flee resets on each hit -- `_fleeFromX/Y` updates to the latest attacker position, and `_fleeUntil` extends. This means repeatedly hitting a fleeing monster keeps it fleeing (correct behavior -- you chase and kill it while it runs).
5. Spawn clamping (`maxFleeRange = wanderRadius * 3`) prevents monsters from fleeing off the map edge.
6. `triggerAssist()` is NOT called for AI-01 (return before that line), which is correct -- cowardly monsters don't have allies join in.

### Gameplay Impact

- **44 AI-01 monsters** now correctly flee when hit. Beginner players see monsters running away as expected.
- Fabre/Willow/Lunatic/Picky become proper "starter field" monsters that don't fight back.
- No effect on AI-02 (Poring, stands ground), AI-03 (Wolf/Condor, retaliates), or any other AI type.

---

## Fix 3: AI Type 06 "Plant" 1-Damage Cap

### Bug

Plant-type monsters (AI code 06, hex mode `0x0000`) should **always take exactly 1 damage** from any attack, regardless of ATK, skill multiplier, element, or any other modifier. Currently, plants take full calculated damage. This affects gameplay balance because:

- Plants are meant to be minor obstacles/resource nodes, not XP-farmable targets
- High-level players can one-shot plants that should function as clickable objects
- Some plants (Red/Blue/Green/Yellow/Shining Plant) drop valuable loot and should require multiple hits

In RO Classic, plants have `mode = 0x0000` (no CanMove, no CanAttack, no CanMove). The 1-damage cap is enforced in the damage calculation pipeline, not via stats.

### Affected Monsters

- **115 monsters** with AI code 06 in `ro_monster_ai_codes.js`:
  - Plants: Red Plant (1078), Blue Plant (1079), Green Plant (1080), Yellow Plant (1081), Shining Plant (1083)
  - Eggs: Pupa (1008), Peco Peco Egg (1047), Thief Bug Egg (1048), Ant Egg (1097), etc.
  - Objects: Mushroom (1084), Black Mushroom (1085), etc.
  - Event/special: Christmas tree objects, treasure chests, etc.

### Root Cause

There is no plant 1-damage cap anywhere in the damage pipeline. The `setEnemyAggro` function at line 28669 has a `canAttack` check that prevents plants from **retaliating**, but nothing prevents them from taking full damage.

Damage reaches plants through multiple paths:
1. **Auto-attack combat tick** (line 25169): `enemy.health = Math.max(0, enemy.health - damage);`
2. **`executePhysicalSkillOnEnemy()`** (line 1955): `enemy.health = Math.max(0, enemy.health - result.damage);`
3. **Individual skill handlers** (dozens of locations): each `enemy.health = Math.max(0, enemy.health - dmg);`
4. **AoE skill loops** (lines 9928, 10781, 11347, 12148, etc.)

### Fix

The most reliable approach is to add a **centralized damage cap function** and call it at the two primary damage application chokepoints, plus a guard at skill damage calc exit.

#### Part A: Add utility function

**Location**: After `setEnemyAggro()` ends (line 28706), add:

```javascript
// Plant 1-damage cap (RO Classic: AI type 06 always takes exactly 1 damage)
// Called before applying damage to any enemy. Returns capped damage value.
function capPlantDamage(enemy, damage) {
    if (enemy.aiCode === 6 && damage > 0) return 1;
    return damage;
}
```

#### Part B: Apply in auto-attack combat tick (right-hand)

**Location**: Line 25167-25169

Current:
```javascript
                if (!isMiss) {
                    enemy._lastDamageDealt = damage; // MVP damage tracking
                enemy.health = Math.max(0, enemy.health - damage);
                }
```

Replace with:
```javascript
                if (!isMiss) {
                    damage = capPlantDamage(enemy, damage);
                    enemy._lastDamageDealt = damage;
                    enemy.health = Math.max(0, enemy.health - damage);
                }
```

Also update `damagePayload.damage` (line 25088) -- since damage is mutated before the payload is sent, we need to ensure the payload reflects the capped value. The payload is built at line 25082-25103 BEFORE the damage application at 25169, so we need to update the payload after capping:

Add after line 25169:
```javascript
                    damagePayload.damage = damage;  // Update payload with plant-capped value
```

#### Part C: Apply in auto-attack combat tick (left-hand dual wield)

**Location**: Line 25211

Current:
```javascript
                    enemy.health = Math.max(0, enemy.health - leftDamage);
```

Replace with:
```javascript
                    leftDamage = capPlantDamage(enemy, leftDamage);
                    damagePayload.damage2 = leftDamage;
                    enemy.health = Math.max(0, enemy.health - leftDamage);
```

Same fix for the right-miss left-hit path at line 25244:
```javascript
                    leftDamage = capPlantDamage(enemy, leftDamage);
                    damagePayload.damage2 = leftDamage;
                    enemy.health = Math.max(0, enemy.health - leftDamage);
```

#### Part D: Apply in `executePhysicalSkillOnEnemy()`

**Location**: Line 1945-1955

Current:
```javascript
    if (!result.isMiss) {
        // Lex Aeterna: double damage on first hit, then consume
        if (result.damage > 0 && enemy.activeBuffs) { ... }
        enemy.health = Math.max(0, enemy.health - result.damage);
```

Replace (add one line after Lex Aeterna block, before health subtraction):
```javascript
    if (!result.isMiss) {
        // Lex Aeterna: double damage on first hit, then consume
        if (result.damage > 0 && enemy.activeBuffs) { ... }
        result.damage = capPlantDamage(enemy, result.damage);
        enemy.health = Math.max(0, enemy.health - result.damage);
```

#### Part E: Apply in AoE skill damage loops

There are many AoE skill handlers that apply damage directly. Rather than modifying each one (high regression risk), add the cap at the point where damage is subtracted from health. The pattern `enemy.health = Math.max(0, enemy.health - X)` appears at approximately these lines:

- 9928 (Magnum Break AoE)
- 10781 (Thunderstorm/AoE ticks)
- 11347 (Turn Undead/Heal on undead)
- 12148 (Magnus Exorcismus ticks)
- 12218 (Double Strafe)
- 12271 (Arrow Shower AoE)
- 12390 (Arrow Vulcan)
- 25691 (Auto-Blitz Beat)
- 25142 (Auto-Blitz on miss path)

For each of these, add `dmgVar = capPlantDamage(enemy, dmgVar);` on the line before health subtraction.

Alternatively, for a more centralized approach, create a wrapper:

```javascript
// Apply damage to enemy with plant cap. Returns actual damage dealt.
function applyDamageToEnemy(enemy, damage) {
    damage = capPlantDamage(enemy, damage);
    const before = enemy.health;
    enemy.health = Math.max(0, enemy.health - damage);
    return damage;
}
```

This wrapper could replace `enemy.health = Math.max(0, enemy.health - X)` everywhere, but is a larger refactor. The per-location approach above covers the most critical paths first.

### Guard Conditions

1. `enemy.aiCode === 6` -- only applies to AI type 06 (plant/immobile objects). All other enemies take full damage.
2. `damage > 0` check ensures misses (damage=0) stay as 0, not get capped UP to 1.
3. The cap applies AFTER all modifiers (Lex Aeterna, element, DEF, etc.) -- this is correct per rAthena, which applies the plant cap at the very end of damage calc.
4. Plants still die when health reaches 0. A Red Plant with 25 HP takes 25 hits of 1 damage each.

### Gameplay Impact

- **115 AI-06 monsters** now properly take 1 damage per hit.
- Plants become multi-hit resource nodes as in RO Classic (Red Plant = ~25 hits, Blue Plant = ~38 hits).
- Encourages correct gameplay loop: kill plants for loot drops, but it takes effort proportional to the reward.

---

## Fix 4: CastSensorChase -- Target Switch During CHASE State

### Bug

Monsters with the `castSensorChase` mode flag should switch targets when a player begins casting a spell nearby, **even while already chasing a different target**. Currently, the CastSensor check in the AI tick (lines 30224-30233) only runs in the `IDLE` state and has a guard `!enemy.targetPlayerId` that prevents it from ever switching an existing target.

The cast sensor trigger code at line 9533-9551 correctly sets `_isCastTarget` on all monsters with `castSensorIdle OR castSensorChase`, but the consumption code only exists in the IDLE state block. Monsters in CHASE state never check `_isCastTarget`.

### Affected Monsters

AI types with `castSensorChase` flag (bit `0x0200`):

| AI Type | Hex Mode  | CastSensorChase? | Monster Count |
|---------|-----------|-------------------|---------------|
| 9       | 0x3095    | No (only CSI)     | ~100          |
| 17      | 0x0091    | No (only CSI)     | ~8            |
| 19      | 0x3095    | No (only CSI)     | ~11           |
| 20      | 0x3295    | **Yes**           | ~98           |
| 21      | 0x3695    | **Yes**           | ~158          |
| 26      | 0xB695    | **Yes**           | ~60           |

- **~316 monsters** with `castSensorChase` (AI types 20, 21, 26) are affected.
- These include many MVPs/bosses (AI type 21), high-level aggressive monsters (AI type 20/26), and elite field monsters.
- For these monsters, casting a spell should be **especially dangerous** because they will snap to the caster mid-chase.

### Root Cause

IDLE state block, lines 30224-30233:
```javascript
// Cast Sensor aggro: if a player is casting near this monster, aggro them
if (enemy._isCastTarget && enemy._castTargetPlayerId && !enemy.targetPlayerId) {
    // ... consume _isCastTarget, call setEnemyAggro ...
}
```

Problems:
1. This block is inside `case AI_STATE.IDLE:` -- never reached during CHASE.
2. The guard `!enemy.targetPlayerId` means even in IDLE, if the monster already has a target (which is cleared on IDLE entry, so this rarely matters), it won't switch.
3. In CHASE state, `_isCastTarget` is set by the trigger code but never consumed, so it persists until the monster returns to IDLE (potentially many seconds later).

### Fix

**Location**: Inside `case AI_STATE.CHASE:`, after the ChangeChase block (line 30337-30351), before the `break` at line 30352.

Add a CastSensorChase check block:

```javascript
            // CastSensorChase: switch target to a player who started casting nearby
            // RO Classic: monsters with this flag snap to casters mid-chase,
            // making spellcasting dangerous near these monsters.
            if (enemy.modeFlags.castSensorChase && enemy._isCastTarget && enemy._castTargetPlayerId) {
                const csCaster = connectedPlayers.get(enemy._castTargetPlayerId);
                if (csCaster && !csCaster.isDead && csCaster.zone === enemy.zone) {
                    // Only switch if the caster is a DIFFERENT target
                    if (enemy._castTargetPlayerId !== enemy.targetPlayerId) {
                        enemy.targetPlayerId = enemy._castTargetPlayerId;
                        if (!enemy.inCombatWith.has(enemy._castTargetPlayerId)) {
                            enemy.inCombatWith.set(enemy._castTargetPlayerId, { totalDamage: 0, name: csCaster.characterName || '' });
                        }
                        logger.info(`[CAST SENSOR CHASE] ${enemy.name}(${enemyId}) switched target → ${csCaster.characterName} (casting detected mid-chase)`);
                    }
                }
                enemy._isCastTarget = false;
                enemy._castTargetPlayerId = null;
            }
```

Additionally, update the existing IDLE CastSensor check to remove the `!enemy.targetPlayerId` guard for `castSensorIdle` monsters, since they should also be able to aggro via cast sensor even if they had a stale target reference:

**Location**: Line 30225

Current:
```javascript
if (enemy._isCastTarget && enemy._castTargetPlayerId && !enemy.targetPlayerId) {
```

Replace with:
```javascript
if (enemy._isCastTarget && enemy._castTargetPlayerId) {
```

This is safe because:
- If the monster already has the same `targetPlayerId`, `setEnemyAggro` will be a no-op (line 28618: `if (enemy.targetPlayerId === newAttackerCharId) return false;`).
- If the monster has a different target, `setEnemyAggro` will use `shouldSwitchTarget` to decide (which respects the monster's mode flags).

### Guard Conditions

1. `enemy.modeFlags.castSensorChase` -- only monsters with this specific flag get mid-chase switching. CastSensorIdle-only monsters (AI 9, 17, 19) are not affected in CHASE state.
2. Caster validity: alive, not dead, same zone -- prevents switching to invalid targets.
3. `enemy._castTargetPlayerId !== enemy.targetPlayerId` -- don't pointlessly switch to the same target.
4. `_isCastTarget` is always cleared after consumption (whether or not the switch happens), preventing stale state from accumulating.
5. Ordering: CastSensorChase check runs AFTER ChangeChase, so both mechanics can coexist. If ChangeChase already switched to a closer target this tick, CastSensorChase can override it if a cast is detected (cast sensor has higher priority in rAthena).

### Gameplay Impact

- **~316 monsters** (AI types 20, 21, 26) now properly snap to casters mid-chase.
- Mages/Priests casting near MVPs and elite monsters will draw aggro as intended, even if the monster is chasing a different party member.
- This makes party play more tactical: healers and casters need to be aware of CastSensor monsters.
- Bosses (AI type 21, many MVPs) become significantly more dangerous for back-line casters, matching RO Classic difficulty.

---

## Summary Table

| # | Bug | Severity | Monsters | Key Lines | Fix Size |
|---|-----|----------|----------|-----------|----------|
| 1 | Angry re-aggro | CRITICAL | 233 (AI 04) | 30301-30311 | ~15 lines added |
| 2 | Cowardly flee | HIGH | 44 (AI 01) | 28667-28706, 30353+ | ~80 lines added (new FLEE state) |
| 3 | Plant 1-damage | HIGH | 115 (AI 06) | 25169, 1955, AoE paths | ~20 lines added + per-AoE caps |
| 4 | CastSensorChase | HIGH | 316 (AI 20/21/26) | 30351, 30225 | ~15 lines added |

**Total estimated change**: ~130 lines of new code across 4 fixes.

**Testing priority**: Fix 1 (Angry) first -- highest impact-to-effort ratio. Then Fix 4 (CastSensorChase) for boss difficulty. Fix 3 (Plant) next for loot economy balance. Fix 2 (Cowardly) last as it's the largest change with new state.
