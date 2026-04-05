# Fix Specification: Pneuma Check Violations + Magic Rod Monster Spell Gap

**Date**: 2026-03-23
**Audit Source**: Round 1 defensive-mechanic audit
**Status**: SPECIFICATION READY

---

## Executive Summary

Round 1 found that only **3/15 ranged physical skills** check for Pneuma. 12 skills are missing the check. Additionally, `executeMonsterPlayerSkill()` is missing Magic Rod absorption for the monster-cast single-target magic path.

This spec covers:
1. Per-skill RO Classic Pneuma interaction analysis (should it be blocked?)
2. Recommended fix approach (centralized vs. per-handler)
3. Exact code changes with line numbers
4. Magic Rod fix for monster spells

---

## Part 1: Per-Skill Pneuma Interaction Analysis

### RO Classic Rule
In RO Classic (pre-renewal), **Pneuma blocks all BF_LONG (ranged) physical attacks** that hit a target standing inside the Pneuma cell. This includes:
- Bow auto-attacks
- Ranged physical skills (projectile-based)
- Shield Boomerang (thrown projectile, ranged)
- Finger Offensive / Throw Spirit Sphere (ranged projectile)

**Exceptions (NOT blocked by Pneuma per rAthena pre-re):**
- **Grimtooth**: rAthena classifies Grimtooth as `BF_SHORT` even though it has range. This is intentional -- Grimtooth is a melee-range splash attack that reaches far due to underground movement, not a projectile. In rAthena `skill.cpp`, Grimtooth has `NK_NO_DAMAGE:0` and `BF_SHORT|BF_WEAPON`. Pneuma does NOT block it.
- **Blitz Beat**: MISC damage type (not physical weapon), uses its own formula. Pneuma only blocks `BF_WEAPON|BF_LONG`. Blitz Beat is `BF_MISC`. NOT blocked.

### Per-Skill Verdict

| # | Skill | ID | Handler Type | Ranged? | Pneuma Blocks? | Current Status |
|---|-------|----|-------------|---------|----------------|----------------|
| 1 | Spear Boomerang | 704 | `executePhysicalSkillOnEnemy` | YES (`isRanged: true`) | YES | CORRECT |
| 2 | Charge Attack | 710 | `executePhysicalSkillOnEnemy` | YES (`isRanged: true`) | YES | CORRECT |
| 3 | Acid Terror | 1801 | Custom handler | YES | YES (damage only, status still applies) | CORRECT |
| 4 | **Double Strafe** | 303 | Custom handler (no EPSE) | YES | **YES -- MISSING** | NEEDS FIX |
| 5 | **Arrow Shower** | 304 | Custom AoE handler | YES | **YES -- per-target** | NEEDS FIX |
| 6 | **Arrow Repel** | 306 | Custom handler (no EPSE) | YES | **YES -- MISSING** | NEEDS FIX |
| 7 | **Shield Boomerang** | 1305 | Custom handler (no EPSE) | YES | **YES -- MISSING** | NEEDS FIX |
| 8 | **Rogue Double Strafe** | 1707 | Calls EPSE (broken args) | YES | **YES -- MISSING** | NEEDS FIX |
| 9 | **Musical Strike** | 1504 | Calls EPSE (no isRanged) | YES | **YES -- MISSING** | NEEDS FIX |
| 10 | **Slinging Arrow** | 1524 | Calls EPSE (no isRanged) | YES | **YES -- MISSING** | NEEDS FIX |
| 11 | **Phantasmic Arrow** | 917 | Calls EPSE (no isRanged) | YES | **YES -- MISSING** | NEEDS FIX |
| 12 | **Throw Venom Knife** | 1111 | Custom handler (no EPSE) | YES | **YES -- MISSING** | NEEDS FIX |
| 13 | **Grimtooth** | 1102 | Custom AoE handler | NO (BF_SHORT) | **NO -- correct as-is** | NO FIX NEEDED |
| 14 | **Finger Offensive** | 1604 | Custom handler (no EPSE) | YES | **YES -- MISSING** | NEEDS FIX |
| 15 | **Blitz Beat** | 900 | Custom MISC handler | MISC type | **NO -- MISC damage** | NO FIX NEEDED |

**Result: 11 skills need Pneuma fixes. Grimtooth and Blitz Beat are correctly exempt.**

---

## Part 2: Fix Approach Analysis

### Option A: Per-handler individual fixes
- Add Pneuma check code block to each of the 11 handlers individually.
- Pros: Explicit, handles edge cases per skill (e.g., Acid Terror blocks damage but not status effects).
- Cons: ~30 lines duplicated 8 times for custom handlers. EPSE-based handlers only need `isRanged: true`.

### Option B: Add `isRanged: true` to EPSE callers + centralized Pneuma in custom handlers (RECOMMENDED)
- For skills that already use `executePhysicalSkillOnEnemy`: just add `isRanged: true` to the options.
- For custom handlers: add the Pneuma check block.
- The centralized check in `executePhysicalSkillOnEnemy` (lines 1891-1913) already works perfectly.

### Option C: Add Pneuma check to damage formula
- Cons: Too deep in the pipeline. Pneuma should block before SP/delay consumption (for EPSE-routed skills) or at minimum before damage application. The damage formula doesn't know about ground effects.

### RECOMMENDED: Option B (Hybrid)

**Group 1 (EPSE-routed, just need `isRanged: true`):**
- Musical Strike (line 18973)
- Slinging Arrow (line 18989)
- Phantasmic Arrow (line 18079)
- Rogue Double Strafe (line 19711 -- also has broken args, separate bug)

**Group 2 (Custom handlers, need inline Pneuma check):**
- Double Strafe (line 12200)
- Arrow Shower (line 12269, per-target in AoE loop)
- Arrow Repel (line 12377)
- Shield Boomerang (line 14018)
- Throw Venom Knife (line 17088)
- Finger Offensive (line 18316)

---

## Part 3: Exact Code Changes

### GROUP 1: EPSE-Routed Skills (add `isRanged: true`)

#### Fix 1: Musical Strike (line 18973)

**Current** (line 18973):
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, {});
```

**Fixed**:
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, { isRanged: true });
```

#### Fix 2: Slinging Arrow (line 18989)

**Current** (line 18989):
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, {});
```

**Fixed**:
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, { isRanged: true });
```

#### Fix 3: Phantasmic Arrow (line 18079)

**Current** (line 18079):
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, { knockback: 3 });
```

**Fixed**:
```javascript
const result = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, { knockback: 3, isRanged: true });
```

#### Fix 4: Rogue Double Strafe (line 19711)

**NOTE**: This handler has a separate bug -- the arguments to `executePhysicalSkillOnEnemy` are in the wrong order. The current call passes `dsZone` as the 4th argument (should be `skill`), and has extra `isEnemy` parameter. This needs to be fixed independently. The Pneuma fix is adding `isRanged: true`.

**Current** (line 19711):
```javascript
await executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {});
```

**Fixed** (also fixing the argument order bug):
```javascript
await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, totalEffectVal, spCost, targetId, { isRanged: true });
```

The correct `executePhysicalSkillOnEnemy` signature is:
```
(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options)
```

The current Rogue DS call has:
- Arg 4: `dsZone` (should be `skill`)
- Arg 5: `skill` (should be `skillId`)
- Arg 6: `skillId` (should be `learnedLevel`)
- Arg 7: `learnedLevel` (should be `levelData`)
- Arg 8: `spCost` (should be `effectVal`)
- Arg 9: `totalEffectVal` (should be `spCost`)
- Arg 10: `targetId` (correct)
- Arg 11: `isEnemy` (extra, should not be here)
- Arg 12: `{}` (should be options, this becomes arg 11)

This means Rogue DS is currently completely broken (wrong skill data, wrong SP cost, wrong effectVal).

---

### GROUP 2: Custom Handlers (inline Pneuma check)

All custom handlers share this pattern. The Pneuma check goes **after SP deduction and delays** (since the skill was "cast" -- SP is consumed even if Pneuma blocks it, matching RO Classic behavior where the skill animation plays but deals 0 damage) but **before damage calculation**.

For Arrow Shower (AoE), the check goes per-target inside the loop.

#### Pneuma Check Helper Pattern

For single-target custom handlers, the check block is:
```javascript
// Pneuma check: ranged physical blocked by Pneuma
const pnEffects = getGroundEffectsAtPosition(enemy.x, enemy.y, enemy.z || 0, 100);
if (pnEffects.find(e => e.type === 'pneuma')) {
    broadcastToZone(ZONE_VAR, 'skill:effect_damage', {
        attackerId: characterId, attackerName: player.characterName,
        targetId, targetName: enemy.name, isEnemy: true,
        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
        damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
        targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
        attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
        targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
        timestamp: Date.now()
    });
    broadcastToZone(ZONE_VAR, 'enemy:health_update', { enemyId: targetId, health: enemy.health, maxHealth: enemy.maxHealth });
    socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
    socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
    return;
}
```

---

#### Fix 5: Double Strafe (ID 303) -- Custom Handler

**Insert AFTER line 12205** (after SP deduction + delays + ammo consume, before damage calc):

```javascript
            // Pneuma check: ranged physical blocked by Pneuma
            const dsPnEffects = getGroundEffectsAtPosition(targetPos.x, targetPos.y, targetPos.z || 0, 100);
            if (dsPnEffects.find(e => e.type === 'pneuma')) {
                broadcastToZone(dsZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName: enemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                    timestamp: Date.now()
                });
                broadcastToZone(dsZone, 'enemy:health_update', { enemyId: targetId, health: enemy.health, maxHealth: enemy.maxHealth });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }
```

**Location**: Between line 12205 (`const dsZone = ...`) and line 12207 (`// Single bundled hit:`).

---

#### Fix 6: Arrow Shower (ID 304) -- AoE, Per-Target Check

**Insert AFTER line 12261** (inside the AoE loop, after range check, before damage calc):

```javascript
                // Pneuma check: skip this target if standing in Pneuma
                const asPnEffects = getGroundEffectsAtPosition(enemy.x, enemy.y, enemy.z || 0, 100);
                if (asPnEffects.find(e => e.type === 'pneuma')) {
                    broadcastToZone(asZone, 'skill:effect_damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId: eid, targetName: enemy.name, isEnemy: true,
                        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                        damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                        targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                        attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                        targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                        groundX, groundY, groundZ: groundZ || 0,
                        timestamp: Date.now()
                    });
                    broadcastToZone(asZone, 'enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth });
                    continue;
                }
```

**Location**: Between line 12261 (`if (Math.sqrt(...) > aoeRadius) continue;`) and line 12263 (`const result = calculateSkillDamage(`).

---

#### Fix 7: Arrow Repel (ID 306) -- Custom Handler

**Insert AFTER line 12379** (after SP deduction + delays + ammo consume, before damage calc):

```javascript
            // Pneuma check: ranged physical blocked by Pneuma
            const arPnEffects = getGroundEffectsAtPosition(targetPos.x, targetPos.y, targetPos.z || 0, 100);
            if (arPnEffects.find(e => e.type === 'pneuma')) {
                const arZone = player.zone || 'prontera_south';
                broadcastToZone(arZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName: enemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                    timestamp: Date.now()
                });
                broadcastToZone(arZone, 'enemy:health_update', { enemyId: targetId, health: enemy.health, maxHealth: enemy.maxHealth });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }
```

**Location**: Between line 12379 (`await consumeAmmo(...)`) and line 12381 (`const result = calculateSkillDamage(`).

---

#### Fix 8: Shield Boomerang (ID 1305) -- Custom Handler

**Insert AFTER line 12019** (after SP deduction + delays, before damage calc):

Note: Shield Boomerang's zone var is `sbZone` (line 14020).

```javascript
            // Pneuma check: Shield Boomerang is ranged, blocked by Pneuma
            const sbPnEffects = getGroundEffectsAtPosition(sbEnemy.x, sbEnemy.y, sbEnemy.z || 0, 100);
            if (sbPnEffects.find(e => e.type === 'pneuma')) {
                broadcastToZone(sbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName: sbEnemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                    targetHealth: sbEnemy.health, targetMaxHealth: sbEnemy.maxHealth,
                    attackerX: sbAttackerPos.x, attackerY: sbAttackerPos.y, attackerZ: sbAttackerPos.z,
                    targetX: sbEnemy.x, targetY: sbEnemy.y, targetZ: sbEnemy.z,
                    timestamp: Date.now()
                });
                broadcastToZone(sbZone, 'enemy:health_update', { enemyId: targetId, health: sbEnemy.health, maxHealth: sbEnemy.maxHealth });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }
```

**Location**: Between line 14020 (`const sbZone = ...`) and line 14022 (`// Damage: batk ...`).

---

#### Fix 9: Throw Venom Knife (ID 1111) -- Custom Handler

**Insert AFTER line 17097** (after SP deduction + delays + venom knife consumption, before damage calc):

```javascript
            // Pneuma check: ranged physical blocked by Pneuma
            const tvkPnEffects = getGroundEffectsAtPosition(tvkEnemy.x, tvkEnemy.y, tvkEnemy.z || 0, 100);
            if (tvkPnEffects.find(e => e.type === 'pneuma')) {
                broadcastToZone(tvkZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName: tvkEnemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                    targetHealth: tvkEnemy.health, targetMaxHealth: tvkEnemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: tvkEnemy.x, targetY: tvkEnemy.y, targetZ: tvkEnemy.z,
                    timestamp: Date.now()
                });
                broadcastToZone(tvkZone, 'enemy:health_update', { enemyId: targetId, health: tvkEnemy.health, maxHealth: tvkEnemy.maxHealth });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }
```

**Location**: Between line 17097 (`await updatePlayerWeightCache(...)`) and line 17099 (`const tvkZone = ...`). Actually, tvkZone is defined at line 17099, so we need the Pneuma check after that line.

**Corrected location**: Between line 17099 (`const tvkZone = ...`) and line 17100 (`const atkBuffMods = ...`).

---

#### Fix 10: Finger Offensive (ID 1604) -- Custom Handler

**Insert AFTER line 18323** (after SP deduction + delays + sphere consume + sphere update, before hit loop):

```javascript
            // Pneuma check: Finger Offensive is ranged, blocked by Pneuma
            const foPnEffects = getGroundEffectsAtPosition(foEnemy.x, foEnemy.y, foEnemy.z || 0, 100);
            if (foPnEffects.find(e => e.type === 'pneuma')) {
                broadcastToZone(zone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName: foEnemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: 'pneuma',
                    targetHealth: foEnemy.health, targetMaxHealth: foEnemy.maxHealth,
                    targetX: foEnemy.x, targetY: foEnemy.y, targetZ: foEnemy.z,
                    timestamp: Date.now()
                });
                broadcastToZone(zone, 'enemy:health_update', { enemyId: targetId, health: foEnemy.health, maxHealth: foEnemy.maxHealth });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }
```

**Location**: Between line 18323 (`emitSphereUpdate(...)`) and line 18324 (`// Multi-hit: 1 hit per sphere consumed`).

**Note**: Spirit spheres ARE consumed even when Pneuma blocks (RO Classic: the skill fires, spheres are used up, but damage is nullified). This matches the placement after sphere consumption.

---

## Part 4: Magic Rod Fix for Monster Spells

### Problem

`executeMonsterPlayerSkill()` (line 29015) handles monster-cast player-class skills on player targets. The magic path (line 29130) calculates damage but never checks for Magic Rod absorption on the player target.

Player-cast single-target magic skills all call `checkMagicRodAbsorption()` before dealing damage. Monster-cast magic skills skip this check entirely.

### RO Classic Rule

Magic Rod absorbs ANY single-target magic spell, regardless of caster (player or monster). When a monster casts Cold Bolt / Fire Bolt / Soul Strike / etc. on a player who has Magic Rod active, the spell should be absorbed.

**Applies to**: All skills in the `magicSkills` set at line 29030 that are single-target (Bolts, Soul Strike, Frost Diver, Earth Spike, Jupitel Thunder, Holy Light, Sight Rasher, Sight Blaster).

**Does NOT apply to**: Ground AoE skills (Storm Gust, LoV, Meteor Storm, etc.) -- these are already routed through the ground effect path at line 29086 and return before reaching the single-target damage code.

### Exact Fix

**Insert AFTER line 29143** (after magic damage calculation, before Auto Guard check):

```javascript
    // Magic Rod absorption check for monster-cast single-target magic on players
    if (isMagic && !isMiss && damage > 0) {
        // Monster skills don't have SP cost in the traditional sense;
        // use the levelData spCost if available, else 0 (player still gains absorption SP)
        const monsterSpellSpCost = levelData?.spCost || 0;
        if (checkMagicRodAbsorption(target, targetCharId, monsterSpellSpCost, zone)) {
            // Spell absorbed -- broadcast 0 damage with absorbed hitType
            broadcastToZone(zone, 'skill:effect_damage', {
                attackerId: enemy.enemyId, attackerName: enemy.name,
                targetId: targetCharId, targetName: target.characterName, isEnemy: false,
                skillId: skillEntry.skillId, skillName: skillDef.displayName,
                skillLevel: skillEntry.level, element,
                damage: 0, isMiss: false, isCritical: false, hitType: 'absorbed',
                targetHealth: target.health, targetMaxHealth: target.maxHealth,
                targetX: target.lastX, targetY: target.lastY, targetZ: target.lastZ,
                timestamp: Date.now(),
            });
            // Plagiarism hook still fires (copies even on absorbed)
            if (target.health > 0) {
                checkPlagiarismCopy(target, targetCharId, skillDef.name, projectSkillId, skillEntry.level);
            }
            return;
        }
    }
```

**Location**: Between line 29143 (`isMiss = magicResult.isMiss || false;`) and line 29144 (`} else {`).

Wait -- this needs to be AFTER both the magic and physical paths converge, because we only want it for magic. Let me re-examine.

**Corrected approach**: Insert after line 29158 (after both magic and physical damage calculation complete), but before the Auto Guard check (line 29160). The check should only fire for magic:

```javascript
    // Magic Rod absorption check: monster-cast single-target magic on players
    if (isMagic && !isMiss && damage > 0) {
        const monsterSpellSpCost = levelData?.spCost || 0;
        if (checkMagicRodAbsorption(target, targetCharId, monsterSpellSpCost, zone)) {
            broadcastToZone(zone, 'skill:effect_damage', {
                attackerId: enemy.enemyId, attackerName: enemy.name,
                targetId: targetCharId, targetName: target.characterName, isEnemy: false,
                skillId: skillEntry.skillId, skillName: skillDef.displayName,
                skillLevel: skillEntry.level, element,
                damage: 0, isMiss: false, isCritical: false, hitType: 'absorbed',
                targetHealth: target.health, targetMaxHealth: target.maxHealth,
                targetX: target.lastX, targetY: target.lastY, targetZ: target.lastZ,
                timestamp: Date.now(),
            });
            if (target.health > 0) {
                checkPlagiarismCopy(target, targetCharId, skillDef.name, projectSkillId, skillEntry.level);
            }
            return;
        }
    }
```

**Location**: Between line 29158 (`}` closing the else block) and line 29160 (`// Auto Guard block check`).

### Magic Rod Buff Consumption

The existing `checkMagicRodAbsorption()` function (line 1837) does NOT call `removeBuff()` to consume the Magic Rod buff after absorption. Reviewing the existing player-cast callers (e.g., line 10155), none of them remove the buff either.

This is actually correct for the buff's behavior: Magic Rod in RO Classic is a **timed buff** (200 + level*200 ms). It absorbs ALL incoming single-target magic during its duration window, then expires naturally. It is NOT consumed on first absorption. The `expiresAt` check on line 1839 handles the expiry. No additional fix needed here.

---

## Part 5: Rogue Double Strafe Argument Bug (Bonus Finding)

### Problem

Line 19711 passes arguments in the wrong order to `executePhysicalSkillOnEnemy`:

```javascript
await executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {});
```

Expected signature: `(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options)`

Issues:
- `dsZone` passed as `skill` (arg 4) -- this would cause skill.name/skill.range to be undefined
- `skill` passed as `skillId` (arg 5)
- `skillId` passed as `learnedLevel` (arg 6)
- `learnedLevel` passed as `levelData` (arg 7)
- `spCost` passed as `effectVal` (arg 8)
- `totalEffectVal` passed as `spCost` (arg 9)
- `isEnemy` is an extra arg
- `levelData` is missing entirely

### Fix

Replace line 19711:
```javascript
            await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, totalEffectVal, spCost, targetId, { isRanged: true });
```

This simultaneously fixes the argument order AND adds the Pneuma check via `isRanged: true`.

Also needs ammo check + consume added (Rogue DS requires bow + arrows, same as Archer DS):
```javascript
            if (!player.equippedAmmo || player.equippedAmmo.quantity <= 0) { socket.emit('skill:error', { message: 'No arrows equipped' }); return; }
```
Insert before the EPSE call. And after:
```javascript
            const dsResult = await executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, totalEffectVal, spCost, targetId, { isRanged: true });
            if (dsResult) await consumeAmmo(player, characterId, 1, io);
```

---

## Part 6: Summary Checklist

| Fix | Skill | Type | Lines Affected | Complexity |
|-----|-------|------|---------------|------------|
| 1 | Musical Strike | Add `isRanged: true` | 18973 | Trivial |
| 2 | Slinging Arrow | Add `isRanged: true` | 18989 | Trivial |
| 3 | Phantasmic Arrow | Add `isRanged: true` | 18079 | Trivial |
| 4 | Rogue Double Strafe | Fix args + add `isRanged: true` | 19703-19712 | Medium |
| 5 | Double Strafe | Add Pneuma block | After 12205 | Small |
| 6 | Arrow Shower | Add per-target Pneuma check | After 12261 | Small |
| 7 | Arrow Repel | Add Pneuma block | After 12379 | Small |
| 8 | Shield Boomerang | Add Pneuma block | After 14020 | Small |
| 9 | Throw Venom Knife | Add Pneuma block | After 17099 | Small |
| 10 | Finger Offensive | Add Pneuma block | After 18323 | Small |
| 11 | Monster Magic Rod | Add MR check | After 29158 | Small |

**No fix needed for**: Grimtooth (BF_SHORT, not blocked by Pneuma), Blitz Beat (MISC damage, not blocked).

**Total skills fixed**: 10 Pneuma + 1 Magic Rod + 1 bonus argument-order fix = 12 changes.

---

## Part 7: Testing Checklist

After applying all fixes, verify:

1. [ ] Archer Double Strafe on target in Pneuma: 0 damage, "pneuma" hitType, SP consumed, arrow consumed
2. [ ] Arrow Shower ground AoE with one target in Pneuma, one outside: Pneuma target takes 0, other takes normal damage
3. [ ] Arrow Repel on target in Pneuma: 0 damage, no knockback, arrow consumed
4. [ ] Shield Boomerang on target in Pneuma: 0 damage, SP consumed
5. [ ] Rogue Double Strafe: verify it works at all (was broken before), then verify Pneuma blocks
6. [ ] Musical Strike on target in Pneuma: 0 damage, arrow consumed
7. [ ] Slinging Arrow on target in Pneuma: 0 damage, arrow consumed
8. [ ] Phantasmic Arrow on target in Pneuma: 0 damage, no knockback
9. [ ] Throw Venom Knife on target in Pneuma: 0 damage, Venom Knife still consumed, NO poison applied
10. [ ] Finger Offensive on target in Pneuma: 0 damage, spirit spheres consumed
11. [ ] Grimtooth on target in Pneuma: NORMAL DAMAGE (should NOT be blocked)
12. [ ] Blitz Beat on target in Pneuma: NORMAL DAMAGE (should NOT be blocked, MISC type)
13. [ ] Monster casts Cold Bolt on player with Magic Rod: spell absorbed, SP recovered, 0 damage
14. [ ] Monster casts Fire Bolt on player without Magic Rod: normal damage (no change)
15. [ ] Spear Boomerang (already working): verify still works correctly
16. [ ] Charge Attack (already working): verify still works correctly
17. [ ] Acid Terror (already working): verify damage blocked but armor break / bleeding still apply
