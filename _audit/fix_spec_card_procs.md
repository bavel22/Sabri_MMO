# Fix Specification: Card Proc Hook Violations

**Date**: 2026-03-23
**Severity**: Critical (6 PvE paths) + Moderate (1 PvP path)
**File**: `server/src/index.js`

---

## Problem Summary

When monsters use skills on players (via `executeMonsterPlayerSkill` and the 6 NPC skill execution functions inside `executeNPCSkill`), NONE of the when-hit card procs trigger. This means:

- **Orc Hero Card** (stun attacker when hit) -- never activates from monster skills
- **Maya Purple Card** (auto-cast when hit) -- never activates from monster skills
- **Hodremlin Card** (autobonus when hit) -- never activates from monster skills
- **Orc Lord Card / High Orc Card** (melee damage reflection) -- never activates from monster skills
- All `bAddEffWhenHit`, `bAutoSpellWhenHit`, `autobonus2` card effects are dead against monster skill damage

Additionally, the PvP auto-attack path (player-vs-player) is missing ALL 8 card hook categories (both on-attack and when-hit).

---

## Reference: Correct Implementation (Enemy Auto-Attack Path)

The enemy auto-attack path at **lines 30960-30981** is the ONLY correct implementation. After damage application (line 30906) and before the death check (line 31114), it calls:

```
Location: Combat tick loop, enemy auto-attack section
Lines 30960-30981

// Card hooks when player is hit by enemy (Phase 4/5)
if (!isMiss && damage > 0) {
    // Phase 5: Status procs when being hit (bAddEffWhenHit)
    processCardStatusProcsWhenHit(atkTarget, enemy, true);            // L30963
    // System E: Auto-cast when hit (bAutoSpellWhenHit)
    processCardAutoSpellWhenHit(atkTarget, enemy, true, zone, io);    // L30965
    // System G: Autobonus2 procs when hit
    processAutobonusWhenHit(atkTarget, enemy.targetPlayerId);         // L30967
    // Phase 4: Melee damage reflection (Orc Lord Card, High Orc Card)
    const reflectedDmg = processCardMeleeReflection(atkTarget, damage);  // L30969
    if (reflectedDmg > 0 && !enemy.isDead) {                            // L30970
        enemy.health = Math.max(0, enemy.health - reflectedDmg);        // L30971
        broadcastToZone(zone, 'enemy:health_update', {...});             // L30972
    }
    // Phase 3: Cast interrupt from damage
    interruptCast(enemy.targetPlayerId, 'damage');                    // L30977
    // System A: Walk delay (hit stun)
    if (!atkTarget.cardNoWalkDelay && !hasBuff(atkTarget, 'endure')) {
        atkTarget.walkDelayUntil = Date.now() + 46;                   // L30980
    }
}
```

### Hook Order (canonical):
1. `processCardStatusProcsWhenHit(target, attacker, attackerIsEnemy)`
2. `processCardAutoSpellWhenHit(target, attacker, attackerIsEnemy, zone, io)`
3. `processAutobonusWhenHit(target, characterId)`
4. `processCardMeleeReflection(target, damage)` + apply reflected damage to enemy
5. `interruptCast(characterId, 'damage')`
6. Walk delay (46ms hit stun)

---

## Function Signatures

### processCardStatusProcsWhenHit (line 3283)
```js
function processCardStatusProcsWhenHit(target, attacker, attackerIsEnemy)
```
- `target`: The player object being hit (has `cardAddEffWhenHit` array)
- `attacker`: The enemy object dealing damage (checked for `modeFlags.statusImmune`)
- `attackerIsEnemy`: Boolean, `true` when attacker is a monster

### processCardAutoSpellWhenHit (line 3543)
```js
function processCardAutoSpellWhenHit(target, attacker, attackerIsEnemy, zone, io)
```
- `target`: The player object being hit (has `cardAutoSpellWhenHit` array)
- `attacker`: The enemy object dealing damage
- `attackerIsEnemy`: Boolean, `true` when attacker is a monster
- `zone`: String zone name for broadcasts
- `io`: Socket.io server instance

### processAutobonusWhenHit (line 3473)
```js
function processAutobonusWhenHit(player, characterId)
```
- `player`: The player object being hit (has `cardAutobonus` array)
- `characterId`: The player's character ID string

### processCardMeleeReflection (line 3306)
```js
function processCardMeleeReflection(target, damage)
```
- `target`: The player object being hit (has `cardShortWeaponDamageReturn`)
- `damage`: Numeric damage dealt
- Returns: reflected damage amount (0 if no reflection)

### processCardMagicReflection (line 3315)
```js
function processCardMagicReflection(target, damage)
```
- `target`: The player object being hit (has `cardMagicDamageReturn`)
- `damage`: Numeric damage dealt
- Returns: reflected damage amount (0 if no reflection)

---

## CRITICAL FINDING: Missing Death Checks

During analysis, I discovered that 5 of the 6 NPC skill types (`elemental_melee`, `status_melee`, `multi_hit`, `forced_crit`, `drain_hp`) have **NO death check at all**. They apply damage via `target.health = Math.max(0, target.health - dmg)` but never check if `target.health <= 0` to trigger `isDead = true`, `clearBuffsOnDeath`, `applyDeathPenalty`, or the `combat:death` broadcast.

Only `aoe_physical` (lines 29420-29430) has a proper death check.

This is a separate bug that should be fixed alongside the card proc hooks. The death handling block from `executeMonsterPlayerSkill` (lines 29214-29228) serves as the template.

---

## Fix 1: `executeMonsterPlayerSkill` (line 29015)

### Current Flow
1. Calculate damage (lines 29130-29158)
2. Auto Guard check (lines 29162-29180)
3. Apply damage: `target.health = Math.max(0, target.health - damage)` (line 29184)
4. Broadcast `skill:effect_damage` (lines 29188-29197)
5. Health update to target (lines 29200-29206)
6. Plagiarism hook (lines 29210-29212)
7. Death check (lines 29215-29228)
8. Break statuses on hit (lines 29231-29237)

### Missing Hooks
No card when-hit hooks anywhere.

### Insertion Point
After damage application (line 29184) and health broadcast, before the plagiarism hook. Insert after line 29206 (health_update emit), before line 29208 (plagiarism comment).

### Exact Code to Insert

Insert between line 29206 and line 29208:

```js
    // ── Card when-hit hooks (monster skill → player) ──
    if (!isMiss && damage > 0 && target.health > 0) {
        processCardStatusProcsWhenHit(target, enemy, true);
        processCardAutoSpellWhenHit(target, enemy, true, zone, io);
        processAutobonusWhenHit(target, targetCharId);
        // Reflection: physical skills use melee reflection, magic skills use magic reflection
        if (isMagic) {
            const magicReflect = processCardMagicReflection(target, damage);
            if (magicReflect > 0 && !enemy.isDead) {
                enemy.health = Math.max(0, enemy.health - magicReflect);
                broadcastToZone(zone, 'enemy:health_update', {
                    enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                });
            }
        } else {
            const meleeReflect = processCardMeleeReflection(target, damage);
            if (meleeReflect > 0 && !enemy.isDead) {
                enemy.health = Math.max(0, enemy.health - meleeReflect);
                broadcastToZone(zone, 'enemy:health_update', {
                    enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                });
            }
        }
        interruptCast(targetCharId, 'damage');
        if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
            target.walkDelayUntil = Date.now() + 46;
        }
    }
```

### Parameter Mapping
| Parameter | Value | Source |
|-----------|-------|--------|
| `target` (player) | `target` | `connectedPlayers.get(targetCharId)` at line 29016 |
| `enemy` (attacker) | `enemy` | Function parameter |
| `attackerIsEnemy` | `true` | Monster attacking player |
| `zone` | `zone` | Function parameter |
| `io` | `io` | Function parameter |
| `targetCharId` | `targetCharId` | Function parameter |
| `damage` | `damage` | Calculated at lines 29142/29156 |
| `isMagic` | `isMagic` | Determined at line 29037 |

### Guard Conditions
- `!isMiss`: No procs on missed attacks
- `damage > 0`: No procs on zero damage (blocked by Auto Guard)
- `target.health > 0`: No procs on dead players (they'll be handled by the death check below)

---

## Fix 2: `elemental_melee` (line 29248)

### Current Flow
1. `calculateEnemyDamage(enemy, targetCharId)` (line 29251)
2. Auto Guard check (lines 29258-29267)
3. Apply damage: `target.health = Math.max(0, target.health - dmg)` (line 29269)
4. Broadcast `combat:damage` (lines 29271-29278)
5. Health update (lines 29279-29283)
6. `break` (line 29284)

### Missing: Death check AND card hooks

### Insertion Point
After health update (line 29283), before `break` (line 29284).

### Exact Code to Insert

Replace the `break;` at line 29284 with:

```js
            // ── Card when-hit hooks (NPC elemental_melee → player) ──
            if (!result.isMiss && dmg > 0 && target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                const reflectedDmg = processCardMeleeReflection(target, dmg);
                if (reflectedDmg > 0 && !enemy.isDead) {
                    enemy.health = Math.max(0, enemy.health - reflectedDmg);
                    broadcastToZone(zone, 'enemy:health_update', {
                        enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                    });
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Check player death
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

### Parameter Mapping
| Parameter | Value | Source |
|-----------|-------|--------|
| `target` (player) | `target` | From outer scope `connectedPlayers.get(targetCharId)` |
| `enemy` (attacker) | `enemy` | From outer scope (function parameter) |
| `attackerIsEnemy` | `true` | Monster attacking player |
| `zone` | `zone` | From outer scope (function parameter) |
| `io` | `io` | From outer scope (function parameter) |
| `targetCharId` | `targetCharId` | From outer scope (function parameter) |
| `dmg` | `dmg` | Calculated at line 29255 |
| `result.isMiss` | `result.isMiss` | From `calculateEnemyDamage` at line 29251 |

### Guard Conditions
- `!result.isMiss`: The miss flag from `calculateEnemyDamage`
- `dmg > 0`: After Auto Guard may have zeroed it
- `target.health > 0`: Player still alive

### Note on Reflection Type
`elemental_melee` is a melee attack variant, so uses `processCardMeleeReflection` (not magic).

---

## Fix 3: `status_melee` (line 29287)

### Current Flow
1. `calculateEnemyDamage(enemy, targetCharId)` (line 29290)
2. Auto Guard check (lines 29295-29304)
3. Apply damage: `target.health = Math.max(0, target.health - dmg)` (line 29306)
4. Broadcast `combat:damage` (lines 29308-29315)
5. Status effect roll (lines 29318-29332)
6. Health update (lines 29334-29338)
7. `break` (line 29339)

### Missing: Death check AND card hooks

### Insertion Point
After health update (line 29338), before `break` (line 29339).

### Exact Code to Insert

Replace the `break;` at line 29339 with:

```js
            // ── Card when-hit hooks (NPC status_melee → player) ──
            if (!result.isMiss && dmg > 0 && target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                const reflectedDmg = processCardMeleeReflection(target, dmg);
                if (reflectedDmg > 0 && !enemy.isDead) {
                    enemy.health = Math.max(0, enemy.health - reflectedDmg);
                    broadcastToZone(zone, 'enemy:health_update', {
                        enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                    });
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Check player death
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

### Variables
Same as `elemental_melee` -- `result` from line 29290, `dmg` from line 29292.

---

## Fix 4: `multi_hit` (line 29342)

### Current Flow
1. Loop `hits` times (line 29346)
2. Each iteration: `calculateEnemyDamage`, apply damage, broadcast (lines 29348-29360)
3. Health update (lines 29362-29366)
4. `break` (line 29367)

### Special Consideration
Multi-hit deals damage in a loop. Card procs should trigger ONCE after all hits complete (not per-hit), matching RO Classic behavior where multi-hit attacks trigger card procs once per attack action.

### Insertion Point
After health update (line 29366), before `break` (line 29367).

### Exact Code to Insert

Replace the `break;` at line 29367 with:

```js
            // ── Card when-hit hooks (NPC multi_hit → player) — once after all hits ──
            // Calculate total damage dealt across all hits for reflection
            const totalMultiDmg = Math.max(0, (target.maxHealth > target.health ? target.maxHealth - target.health : 0));
            if (target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                if (totalMultiDmg > 0) {
                    const reflectedDmg = processCardMeleeReflection(target, totalMultiDmg);
                    if (reflectedDmg > 0 && !enemy.isDead) {
                        enemy.health = Math.max(0, enemy.health - reflectedDmg);
                        broadcastToZone(zone, 'enemy:health_update', {
                            enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                        });
                    }
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Check player death
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

### Note on Total Damage Calculation
The `multi_hit` loop doesn't accumulate total damage in a variable. We need to calculate how much HP was lost. Since we know `target.health` started at some value before the loop and the loop may have reduced it, but we don't have the pre-loop HP cached. A simpler approach is to track total damage in the loop:

**Revised**: Add a `totalDmg` accumulator BEFORE the loop. Change the loop at line 29346:

```js
            let totalMultiHitDmg = 0;
            for (let h = 0; h < hits; h++) {
                if (target.health <= 0) break;
                const result = calculateEnemyDamage(enemy, targetCharId);
                if (!result) continue;
                const dmg = result.isMiss ? 0 : result.damage;
                totalMultiHitDmg += dmg;
                if (dmg > 0) target.health = Math.max(0, target.health - dmg);
                // ... existing broadcast ...
            }
```

Then the hooks block uses `totalMultiHitDmg` instead:

```js
            if (totalMultiHitDmg > 0 && target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                const reflectedDmg = processCardMeleeReflection(target, totalMultiHitDmg);
                if (reflectedDmg > 0 && !enemy.isDead) {
                    enemy.health = Math.max(0, enemy.health - reflectedDmg);
                    broadcastToZone(zone, 'enemy:health_update', {
                        enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                    });
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Death check (same as other cases)
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

---

## Fix 5: `forced_crit` (line 29370)

### Current Flow
1. `calculateEnemyDamage(enemy, targetCharId)` (line 29373)
2. Force crit: `critDmg = Math.floor((result.damage || 0) * 1.4)` (line 29376)
3. Apply damage: `target.health = Math.max(0, target.health - critDmg)` (line 29377)
4. Broadcast `combat:damage` (lines 29378-29385)
5. Health update (lines 29386-29390)
6. `break` (line 29391)

### Missing: Death check AND card hooks

### Insertion Point
After health update (line 29390), before `break` (line 29391).

### Exact Code to Insert

Replace the `break;` at line 29391 with:

```js
            // ── Card when-hit hooks (NPC forced_crit → player) ──
            if (critDmg > 0 && target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                const reflectedDmg = processCardMeleeReflection(target, critDmg);
                if (reflectedDmg > 0 && !enemy.isDead) {
                    enemy.health = Math.max(0, enemy.health - reflectedDmg);
                    broadcastToZone(zone, 'enemy:health_update', {
                        enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                    });
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Check player death
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

### Guard Conditions
- `critDmg > 0`: Forced crit always hits (no miss check), but damage could theoretically be 0 from a 0-ATK enemy
- `target.health > 0`: Player still alive
- No `isMiss` guard needed -- forced_crit never misses (`result.isMiss` is not checked in current code, crit bypasses flee)

---

## Fix 6: `aoe_physical` (line 29394)

### Current Flow
1. Loop all players in radius (lines 29398-29401)
2. Calculate and apply damage (lines 29402-29404)
3. Broadcast `skill:effect_damage` (lines 29405-29413)
4. Health update (lines 29414-29418)
5. Death check (lines 29420-29430) -- **ALREADY EXISTS**
6. End of loop

### Missing: Card hooks only (death check exists)

### Insertion Point
After health update (line 29418) and before the existing death check (line 29420).

### Exact Code to Insert

Insert between line 29418 and line 29419:

```js
                // ── Card when-hit hooks (NPC aoe_physical → player) ──
                if (reducedDmg > 0 && player.health > 0) {
                    processCardStatusProcsWhenHit(player, enemy, true);
                    processCardAutoSpellWhenHit(player, enemy, true, zone, io);
                    processAutobonusWhenHit(player, charId);
                    const reflectedDmg = processCardMeleeReflection(player, reducedDmg);
                    if (reflectedDmg > 0 && !enemy.isDead) {
                        enemy.health = Math.max(0, enemy.health - reflectedDmg);
                        broadcastToZone(zone, 'enemy:health_update', {
                            enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                        });
                    }
                    interruptCast(charId, 'damage');
                    if (!player.cardNoWalkDelay && !hasBuff(player, 'endure')) {
                        player.walkDelayUntil = Date.now() + 46;
                    }
                }
```

### Parameter Mapping (different variable names in this loop)
| Parameter | Value | Source |
|-----------|-------|--------|
| `player` | `player` | Loop variable from `connectedPlayers.entries()` at line 29398 |
| `enemy` | `enemy` | From outer scope |
| `charId` | `charId` | Loop variable from `connectedPlayers.entries()` at line 29398 |
| `reducedDmg` | `reducedDmg` | Calculated at line 29403 |

### Note
No miss check needed -- `aoe_physical` (NPC_EARTHQUAKE) always hits, there is no miss calculation.

---

## Fix 7: `drain_hp` (line 29457)

### Current Flow
1. `calculateEnemyDamage(enemy, targetCharId)` (line 29460)
2. Early return on miss: `if (!result || result.isMiss) return` (line 29461)
3. Apply damage: `target.health = Math.max(0, target.health - dmg)` (line 29463)
4. Drain HP to enemy (lines 29464-29465)
5. Broadcast `combat:damage` (lines 29466-29473)
6. Broadcast `enemy:health_update` (lines 29474-29476)
7. Health update to player (lines 29477-29481)
8. `break` (line 29482)

### Missing: Death check AND card hooks

### Insertion Point
After health update (line 29481), before `break` (line 29482).

### Exact Code to Insert

Replace the `break;` at line 29482 with:

```js
            // ── Card when-hit hooks (NPC drain_hp → player) ──
            if (dmg > 0 && target.health > 0) {
                processCardStatusProcsWhenHit(target, enemy, true);
                processCardAutoSpellWhenHit(target, enemy, true, zone, io);
                processAutobonusWhenHit(target, targetCharId);
                const reflectedDmg = processCardMeleeReflection(target, dmg);
                if (reflectedDmg > 0 && !enemy.isDead) {
                    enemy.health = Math.max(0, enemy.health - reflectedDmg);
                    broadcastToZone(zone, 'enemy:health_update', {
                        enemyId: enemy.enemyId, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true
                    });
                }
                interruptCast(targetCharId, 'damage');
                if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                    target.walkDelayUntil = Date.now() + 46;
                }
            }
            // Check player death
            if (target.health <= 0 && !target.isDead) {
                target.isDead = true;
                clearBuffsOnDeath(target, targetCharId, zone);
                const deadSock = io.sockets.sockets.get(target.socketId);
                applyDeathPenalty(target, targetCharId, deadSock);
                autoAttackState.delete(targetCharId);
                broadcastToZone(zone, 'combat:death', {
                    killedId: targetCharId, killedName: target.characterName,
                    killerId: enemy.enemyId, killerName: enemy.name, isEnemy: false,
                    targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now(),
                });
            }
            break;
```

### Guard Conditions
- No `isMiss` guard needed -- the function already returns early on miss (line 29461)
- `dmg > 0`: Should always be true after the miss guard, but defensive check

---

## Fix 8: PvP Auto-Attack Path (Moderate Priority)

### Location
Lines 26060-26276, specifically the damage application section starting at line 26188.

### Current Flow
1. `calculatePhysicalDamage(...)` (line 26151)
2. Miss check: early `continue` on miss (lines 26182-26186)
3. Apply damage: `target.health = Math.max(0, target.health - pvpDmg)` (line 26188)
4. `checkAutoBerserk` (line 26190)
5. Party HP broadcast (lines 26193-26197)
6. Cast interruption (lines 26200-26202)
7. Log + Broadcast `combat:damage` (lines 26204-26206)
8. Status break (lines 26209-26214)
9. Arrow consumption (lines 26217-26219)
10. Death check (lines 26222-26276)

### Missing: ALL 8 card hook categories
**On-attack hooks** (attacker's cards):
- `processCardDrainEffects(attacker, pvpDmg, attackerId)`
- `processCardStatusProcsOnAttack(attacker, target, false)` -- `false` because target is NOT an enemy
- `processCardAutoSpellOnAttack(attacker, target, false, pvpAtkZone, io)`
- `processAutobonusOnAttack(attacker, attackerId)`

**When-hit hooks** (target's cards):
- `processCardStatusProcsWhenHit(target, attacker, false)` -- `false` because attacker is NOT an enemy
- `processCardAutoSpellWhenHit(target, attacker, false, pvpAtkZone, io)`
- `processAutobonusWhenHit(target, atkState.targetCharId)`
- `processCardMeleeReflection(target, pvpDmg)` + apply to attacker

### Insertion Point
After the damage broadcast at line 26206, before status break at line 26208.

### Exact Code to Insert

Insert between line 26206 and line 26208:

```js
            // ── Card on-attack hooks (PvP attacker cards) ──
            processCardDrainEffects(attacker, pvpDmg, attackerId);
            processCardStatusProcsOnAttack(attacker, target, false);
            processCardAutoSpellOnAttack(attacker, target, false, pvpAtkZone, io);
            processAutobonusOnAttack(attacker, attackerId);

            // ── Card when-hit hooks (PvP target cards) ──
            processCardStatusProcsWhenHit(target, attacker, false);
            processCardAutoSpellWhenHit(target, attacker, false, pvpAtkZone, io);
            processAutobonusWhenHit(target, atkState.targetCharId);
            const pvpReflect = processCardMeleeReflection(target, pvpDmg);
            if (pvpReflect > 0) {
                attacker.health = Math.max(0, attacker.health - pvpReflect);
                broadcastToZone(pvpAtkZone, 'combat:damage', {
                    attackerId: atkState.targetCharId, attackerName: target.characterName,
                    targetId: attackerId, targetName: attacker.characterName,
                    isEnemy: false, damage: pvpReflect, isCritical: false, isMiss: false,
                    hitType: 'reflect', element: 'neutral',
                    targetHealth: attacker.health, targetMaxHealth: attacker.maxHealth,
                    timestamp: now
                });
                const atkSock = io.sockets.sockets.get(attacker.socketId);
                if (atkSock) atkSock.emit('combat:health_update', {
                    characterId: attackerId, health: attacker.health, maxHealth: attacker.maxHealth,
                    mana: attacker.mana, maxMana: attacker.maxMana,
                });
                // Check if reflected damage killed the attacker
                if (attacker.health <= 0 && !attacker.isDead) {
                    attacker.isDead = true;
                    clearBuffsOnDeath(attacker, attackerId, pvpAtkZone);
                    autoAttackState.delete(attackerId);
                    broadcastToZone(pvpAtkZone, 'combat:death', {
                        killedId: attackerId, killedName: attacker.characterName,
                        killerId: atkState.targetCharId, killerName: target.characterName,
                        isEnemy: false, targetHealth: 0, targetMaxHealth: attacker.maxHealth,
                        timestamp: now
                    });
                }
            }
            // Walk delay on PvP hit
            if (!target.cardNoWalkDelay && !hasBuff(target, 'endure')) {
                target.walkDelayUntil = Date.now() + 46;
            }
```

### Parameter Mapping
| Parameter | Value | Source |
|-----------|-------|--------|
| `attacker` | `attacker` | `connectedPlayers.get(attackerId)` from combat tick |
| `target` | `target` | `connectedPlayers.get(atkState.targetCharId)` at line 26074 |
| `attackerIsEnemy` | `false` | Attacker is a player, not a monster |
| `pvpAtkZone` | `pvpAtkZone` | Defined at line 26181 |
| `io` | `io` | From combat tick scope |
| `attackerId` | `attackerId` | From combat tick loop variable |
| `atkState.targetCharId` | `atkState.targetCharId` | From auto-attack state |
| `pvpDmg` | `pvpDmg` | From `calculatePhysicalDamage` result at line 26160 |

### Guard Conditions
This code block is only reached if `!pvpMiss` (line 26182 does `continue` on miss). So no additional miss guard needed. The `pvpDmg > 0` guard is implicit since the code wouldn't reach here with 0 damage.

---

## Summary of All Changes

| # | Function | Line Range | Hooks Added | Death Check Added | Priority |
|---|----------|-----------|-------------|-------------------|----------|
| 1 | `executeMonsterPlayerSkill` | ~29206 | 3 when-hit + reflection | Already exists | Critical |
| 2 | `elemental_melee` | ~29284 | 3 when-hit + reflection | **YES (new)** | Critical |
| 3 | `status_melee` | ~29339 | 3 when-hit + reflection | **YES (new)** | Critical |
| 4 | `multi_hit` | ~29367 | 3 when-hit + reflection | **YES (new)** | Critical |
| 5 | `forced_crit` | ~29391 | 3 when-hit + reflection | **YES (new)** | Critical |
| 6 | `aoe_physical` | ~29418 | 3 when-hit + reflection | Already exists | Critical |
| 7 | `drain_hp` | ~29482 | 3 when-hit + reflection | **YES (new)** | Critical |
| 8 | PvP auto-attack | ~26206 | 4 on-attack + 4 when-hit | Attacker death from reflect | Moderate |

### Cards Now Functional After Fix

**When-hit procs (bAddEffWhenHit)**: Orc Hero Card (stun), Mastering Card (stun), Injustice Card (stun), Deviace Card (freeze), etc.

**Auto-spell when hit (bAutoSpellWhenHit)**: Maya Purple Card, RSX-0806 Card, etc.

**Autobonus2 when hit**: Hodremlin Card (perfect dodge buff), Ice Titan Card (freeze defense), etc.

**Melee/Magic reflection**: Orc Lord Card (short weapon reflect), High Orc Card (short weapon reflect), Maya Card (magic reflect).

### Bonus: Death Check Bug Fix

5 NPC skill types (`elemental_melee`, `status_melee`, `multi_hit`, `forced_crit`, `drain_hp`) currently have NO death handling. Players can be reduced to 0 HP by these skills and remain in a zombie state (health=0 but `isDead=false`). The death check blocks in this spec fix that alongside the card proc hooks.

---

## Testing Plan

1. **Orc Hero Card test**: Equip card, get hit by any monster skill (e.g., Goblin Archer's Double Attack via `multi_hit`). Verify stun procs on the monster.

2. **Reflection test**: Equip Orc Lord Card, get hit by `status_melee` monster. Verify reflected damage appears on enemy health bar.

3. **Death check test**: Have a low-HP player get hit by `elemental_melee` NPC skill. Verify `combat:death` broadcasts and player enters dead state.

4. **PvP test**: Enable PvP, equip attacker with Strouf Card (bAutoSpell on attack), hit another player. Verify auto-spell triggers.

5. **Multi-hit reflection test**: Get hit by `multi_hit` (e.g., NPC_COMBOATTACK). Verify reflection applies once based on total damage, not per-hit.

6. **AoE procs test**: Get hit by `aoe_physical` (NPC_EARTHQUAKE from MVP). Verify each player in range independently triggers their own card procs.
