# Code Quality & Consistency Audit — server/src/index.js

**Date**: 2026-03-23
**File**: `server/src/index.js` (~32,566 lines)
**Method**: 7-pass manual audit — patterns, deviations, naming, duplication, magic numbers, validation, compilation

---

## 1. Established Patterns (Pass 1)

### 1A. Skill Dispatch Pattern
Skills are dispatched via sequential `if (skill.name === '...')` chains inside the `socket.on('skill:use')` handler (starts at line 9122). There is no switch/case or dispatch table. Each skill is a standalone block ending with `return;`.

**Canonical skill handler structure** (physical, single-target):
```
1. Validate target exists: if (!targetId || !isEnemy) → skill:error
2. Get enemy: enemies.get(targetId); check isDead
3. Range check: getPlayerPosition + sqrt(dx^2+dy^2)
4. Deduct SP: player.mana = Math.max(0, player.mana - spCost)
5. Apply delays: applySkillDelays(characterId, player, skillId, levelData, socket)
6. Calculate damage: calculateSkillDamage(...)
7. Lex Aeterna check: find + consume + double
8. Apply damage: target.health = Math.max(0, ...)
9. checkDamageBreakStatuses → broadcast status:removed + skill:buff_removed
10. setEnemyAggro
11. broadcastToZone('skill:effect_damage', {...})
12. broadcastToZone('enemy:health_update', {...})
13. Death check: processEnemyDeathFromSkill
14. socket.emit('skill:used', {...})
15. socket.emit('combat:health_update', {...})
```

### 1B. Helper Function: executePhysicalSkillOnEnemy (line 1874)
Encapsulates steps 2-15 for simple physical-on-enemy skills. Called by ~11 handlers:
- Sand Attack, Mammonite, Spear Boomerang, Charge Attack, Shield Charge, Phantasmic Arrow, Ki Explosion, Musical Strike, Slinging Arrow, Double Strafe Rogue

### 1C. Buff Application Pattern
```js
applyBuff(target, { skillId, name: 'buff_name', casterId, casterName, ...effects, duration });
broadcastToZone(zone, 'skill:buff_applied', { targetId, targetName, isEnemy, casterId, casterName, skillId, buffName, duration, effects });
```
Used consistently across ~80 buff applications.

### 1D. Status Effect Application Pattern
```js
const result = applyStatusEffect(source, target, statusType, baseChance, overrideDuration);
if (result && result.applied) {
    broadcastToZone(zone, 'status:applied', { targetId, isEnemy, statusType, duration: result.duration });
}
```
Used across ~45 status applications.

### 1E. Damage Break Pattern
```js
const broken = checkDamageBreakStatuses(target);
for (const bt of broken) {
    broadcastToZone(zone, 'status:removed', { targetId, isEnemy, statusType: bt, reason: 'damage_break' });
    broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: bt, reason: 'damage_break' });
}
```

### 1F. Socket Event Naming Convention
Pattern: `domain:action` in snake_case. Examples: `skill:use`, `combat:attack`, `enemy:spawn`, `party:invite`, `inventory:load`.

### 1G. Error Response Pattern
```js
socket.emit('domain:error', { message: 'Human-readable error' });
```
Domain matches the event domain (skill:error, combat:error, zone:error, cart:error, etc.).

---

## 2. Deviations From Patterns (Pass 2)

### 2A. CRITICAL: Rogue Double Strafe — Wrong Arguments to executePhysicalSkillOnEnemy
**Line 19711**: The handler passes arguments in the wrong order, inserting `dsZone` as the 4th argument where `skill` should be.

```js
// BROKEN (line 19711):
await executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {});

// CORRECT signature (line 1874):
executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options)
```

This passes a string (`dsZone`) where the function expects the `skill` object, then `skill` where `skillId` should be, etc. The function would fail silently or crash at runtime. Also passes `spCost` where `effectVal` should be and `totalEffectVal` where `spCost` should be (swapped), and includes extra args `targetId, isEnemy, {}` that don't match the signature.

**Severity**: CRITICAL (skill is completely broken at runtime)

### 2B. HIGH: Inconsistent Damage Break Broadcasting
The canonical pattern (steps 1-15 above) emits BOTH `status:removed` AND `skill:buff_removed` after `checkDamageBreakStatuses`. However, many handlers (particularly Wizard AoE ground effects, Rogue skills, combat tick handlers) emit ONLY `status:removed` and omit `skill:buff_removed`.

**Counts**: 50 `status:removed` emissions with `damage_break` vs only 34 `skill:buff_removed` emissions with `damage_break`.

**Missing `skill:buff_removed`** in at least 16 handlers:
- Water Ball (line 14773)
- Sight Rasher (line 15071)
- Frost Nova (line 15131)
- Back Stab (line 19557)
- Raid (line 19613)
- Intimidate (line 19669)
- Ruwach reveal damage (line 27112)
- Storm Gust ground tick (line 27918)
- Lord of Vermilion ground tick (line 28002)
- Meteor Storm ground tick (line 28091)
- Demonstration ground tick (line 28156)
- Fire Pillar ground tick (line 28320)
- PvP auto-attack (line 29232)

**Impact**: Client may not correctly clear buff icons when these skills break freeze/stone/sleep.

### 2C. HIGH: Redundant CC Checks Inside Magic Handlers
The global `skill:use` handler (lines 9146-9157) already checks all CC conditions (`isFrozen`, `isStoned`, `isStunned`, `isSleeping`, `isSilenced`) and returns early. Despite this, 9 individual magic handlers redundantly re-check CC:
- Cold Bolt/Fire Bolt/Lightning Bolt (line 10148) — uses incomplete check `isFrozen || isStoned` (misses stun/sleep/silence)
- Soul Strike (line 10314) — uses `preventsCasting`
- Napalm Beat (line 10451)
- Fire Ball (line 10599)
- Thunderstorm (line 10735)
- Frost Diver (line 10937)
- Stone Curse (line 11075)
- Sight (line 11157)
- Fire Wall (line 11186)
- Safety Wall (line 11256)

The Bolt handler at line 10148 uses `isFrozen || isStoned` instead of the comprehensive `preventsCasting`, which is both redundant AND incomplete compared to the global check.

**Impact**: No runtime bug (global check catches first), but misleading code. If global check were ever relaxed, the per-handler checks would behave differently.

### 2D. MEDIUM: Inconsistent Use of getBuffStatModifiers vs getCombinedModifiers
`getBuffStatModifiers` is a thin wrapper that calls `getCombinedModifiers` (line 1810). Both names are used throughout:
- `getBuffStatModifiers`: 40+ call sites
- `getCombinedModifiers`: 68+ call sites

Bash handler (line 9642) uses `getBuffStatModifiers(target)` for the defense modifier, while `executePhysicalSkillOnEnemy` (line 1931) uses `getCombinedModifiers(enemy)`. Functionally identical, but naming inconsistency makes it unclear whether these are different functions.

Similarly, `calculateDerivedStats` (line 2562) wraps `roDerivedStats` (imported). Both names are used: 10 calls to the local wrapper, 21 to the import.

### 2E. MEDIUM: Inconsistent Lex Aeterna Check
The Lex Aeterna double-damage check is copy-pasted ~25 times with identical logic:
```js
const lexBuff = target.activeBuffs.find(b => b.name === 'lex_aeterna' && Date.now() < b.expiresAt);
if (lexBuff) {
    damage *= 2;
    removeBuff(target, 'lex_aeterna');
    broadcastToZone(zone, 'skill:buff_removed', { targetId, isEnemy, buffName: 'lex_aeterna', reason: 'consumed' });
}
```

The `executePhysicalSkillOnEnemy` helper includes this check. But all inline handlers duplicate it. Not all damage-dealing handlers include the check.

### 2F. LOW: _lastDamageDealt for MVP Tracking — Inconsistently Set
Only ~17 out of ~88 damage-dealing skill handlers set `enemy._lastDamageDealt`. This means many skills do not contribute to MVP damage tracking correctly. The helper `executePhysicalSkillOnEnemy` does NOT set `_lastDamageDealt` either.

**Affected**: Pierce, Spear Stab, Brandish Spear, Bowling Bash, most Knight/Crusader/Monk/Assassin skills, all ground AoE ticks.

### 2G. LOW: decrementRaidDebuffHits — Called in Only 3 Locations
The raid debuff hit counter (decrements on any damage) is only called in:
1. `executePhysicalSkillOnEnemy` helper (line 1964)
2. Bash handler (inside the helper via code path)

All other inline skill handlers that deal damage do NOT call `decrementRaidDebuffHits`. This means the raid debuff will NOT expire after 7 hits for most skill damage sources.

---

## 3. Naming Inconsistencies (Pass 3)

### 3A. Zeny Update Event — Two Different Names
- `inventory:zeny_update` — used at lines 6754, 7193, 7201, 12939 (cart, vending, mammonite)
- `zeny:update` — used at lines 19745, 24331 (steal coin, refine)

The client must handle BOTH event names or it will miss zeny changes from some sources.

### 3B. Buff Name Casing — Mostly Consistent (snake_case)
Buff names in `applyBuff` calls consistently use `snake_case`:
- `increase_agi`, `decrease_agi`, `two_hand_quicken`, `auto_guard`, `reflect_shield`, `loud_exclamation`, `magnum_break_fire`, `blessing_debuff`, `ensemble_aftermath`, etc.

**One exception**: `buffName` in the broadcast uses display names like `'Increase AGI'`, `'Loud Exclamation'`, `'Energy Coat'` — these are PascalCase/display strings, not the internal snake_case name. This is intentional (display vs internal), but `buffName` is overloaded to mean different things in different broadcasts.

### 3C. Variable Naming — player/p/ptarget/target
- `player` — always the caster/attacker (consistent)
- `target` — the skill's target, either enemy or player (consistent for single-target)
- `ptarget` — specifically a player target in PvP contexts (consistent)
- `enemy` — an enemy entity (consistent)
- `p` — used for players in party loops (line 1368, occasional shorthand)
- Variables in different handlers use different shorthand for the same concept:
  - Zone: `mbZone`, `boltZone`, `tsZone`, `bashZone`, `crZone`, `dsZone`, `envZone`, etc. — prefixed with skill abbreviation. This is fine for scoping but adds visual noise.

### 3D. Monster References — Consistent
`enemy` is used consistently for combat entities. `monster` appears only in template/data contexts. No `mob` usage in index.js.

### 3E. Effect Damage Broadcast — Missing Fields in Some Handlers
The canonical `skill:effect_damage` broadcast includes `attackerX/Y/Z` and `targetX/Y/Z`. Some utility skill handlers (Steal Coin at lines 19747-19762) omit `attackerX/Y/Z` entirely.

---

## 4. Duplicated Logic (Pass 4)

### 4A. HIGH: Damage-Apply-Broadcast-Death Sequence (~90 inline copies)
The following 8-step sequence is repeated inline in virtually every damage-dealing skill handler:

```js
// 1. Calculate damage
const result = calculateSkillDamage(...) or calculateMagicSkillDamage(...)
// 2. Lex Aeterna check (copy-pasted 25 times)
// 3. Apply damage: target.health = Math.max(0, ...)
// 4. Set aggro: setEnemyAggro(enemy, characterId, 'skill')
// 5. Check damage break: checkDamageBreakStatuses(target) → broadcast
// 6. Broadcast skill:effect_damage
// 7. Broadcast enemy:health_update
// 8. Death check: processEnemyDeathFromSkill
```

`executePhysicalSkillOnEnemy` extracts this for simple physical skills but is only used by ~11 handlers. The remaining ~80+ handlers copy-paste this entire sequence with minor variations.

**Extraction opportunity**: A `applySkillDamageToEnemy(enemy, damage, player, characterId, zone, skillBroadcast)` helper could encapsulate steps 2-8.

### 4B. HIGH: Target Resolution Sequence (~15 copies for magic spells)
Single-target magic spells all include this identical block:
```js
let target, targetPos, targetStats, targetHardMdef = 0, targetName = '';
if (isEnemy) {
    target = enemies.get(targetId);
    if (!target || target.isDead) { socket.emit('skill:error', ...); return; }
    targetPos = { x: target.x, y: target.y, z: target.z };
    targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
    targetHardMdef = target.hardMdef || target.magicDefense || 0;
    targetName = target.name;
} else {
    target = connectedPlayers.get(targetId);
    ...
}
```
Copy-pasted in: Cold/Fire/Lightning Bolt, Soul Strike, Napalm Beat, Fire Ball, Frost Diver, Stone Curse, Jupitel Thunder, Earth Spike, Heaven's Drive, Water Ball.

**Extraction opportunity**: `resolveTarget(targetId, isEnemy)` returning `{ target, targetPos, targetStats, targetHardDef/Mdef, targetName }`.

### 4C. HIGH: Range Check Sequence (~30 copies)
```js
const attackerPos = await getPlayerPosition(characterId);
if (!attackerPos) return;
const dx = attackerPos.x - targetPos.x;
const dy = attackerPos.y - targetPos.y;
const dist = Math.sqrt(dx * dx + dy * dy);
const skillRange = (skill.range || DEFAULT) + COMBAT.RANGE_TOLERANCE;
if (dist > skillRange) { socket.emit('combat:out_of_range', {...}); return; }
```
Appears in ~30 handlers with slight variations (some add `bonusRange`, some use different defaults).

**Extraction opportunity**: `checkSkillRange(characterId, targetPos, skill, player)` returning `{ inRange, dist, attackerPos }`.

### 4D. MEDIUM: skill:used + combat:health_update Epilogue
Every single handler emits these two events at the end:
```js
socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
```
~203 emissions of `skill:used`, ~201 of `combat:health_update`. Could be extracted to `emitSkillEpilogue(socket, skillId, skill, learnedLevel, spCost, player, characterId)`.

### 4E. MEDIUM: Multi-Hit Magic Damage Loop
All multi-hit magic spells (Bolts, Soul Strike, Jupitel Thunder, Earth Spike, Water Ball) share this pattern:
```js
const hitDamages = [];
let totalDamage = 0;
for (let h = 0; h < numHits; h++) {
    const hitResult = calculateMagicSkillDamage(...);
    hitDamages.push(hitResult.damage);
    totalDamage += hitResult.damage;
}
// Lex Aeterna doubles totalDamage
// Apply totalDamage
// Emit per-hit events via setTimeout stagger
```
Copy-pasted ~8 times.

### 4F. LOW: AoE Enemy Loop Pattern
AoE handlers iterate enemies with:
```js
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.isDead || enemy.zone !== zone) continue;
    const dx = center.x - enemy.x; const dy = center.y - enemy.y;
    if (Math.sqrt(dx*dx + dy*dy) > aoeRadius) continue;
    // Calculate + apply damage
}
```
Appears in Magnum Break, Arrow Shower, Cart Revolution, Thunderstorm, Bowling Bash, Brandish Spear, Raid, Hammer Fall, and all ground effect ticks.

**Extraction opportunity**: `getEnemiesInArea(zone, x, y, radius)` returning `[{ eid, enemy }]`.

---

## 5. Magic Numbers (Pass 5)

### 5A. Duration Literals (should be constants or derived from skill data)
| Value | Meaning | Occurrences |
|-------|---------|-------------|
| `120000` | 2 min default buff duration | 15+ |
| `300000` | 5 min default buff duration | 10+ |
| `600000` | 10 min safety cap for toggle buffs | 8+ |
| `60000` | 1 min | 5+ |
| `10000` | 10 sec (ensemble aftermath) | 4 |
| `5000` | 5 sec (stun default, respawn delay) | 10+ |
| `30000` | 30 sec (various status effects) | 10+ |
| `15000` | 15 sec | 3 |
| `12000` | 12 sec (Storm Gust freeze) | 2 |

Most of these should come from skill level data (`levelData.duration`), but fallback defaults are hardcoded.

### 5B. Range/Area Literals
| Value | Meaning | Occurrences |
|-------|---------|-------------|
| `150` | 3x3 AoE radius, melee range | 10+ |
| `250` | 5x5 AoE radius | 5+ |
| `125` | 5x5 cells (2.5 cells) | 3 |
| `100` | 2-cell radius | 5+ |
| `450` | Default magic range | 8 |
| `800` | Default ranged range | 3 |
| `50` | Cell size in UE units | used in division |

### 5C. Damage Multiplier Literals
| Value | Context | Lines |
|-------|---------|-------|
| `15 * learnedLevel` | Envenom flat bonus | 12634 |
| `(learnedLevel - 5) * baseLevel / 10` | Fatal Blow stun chance | 9731 |
| `effectVal * 2` | Double Strafe total multiplier | 12208, 19710 |
| `100 * cartWeight / 8000` | Cart Revolution weight bonus | 12968 |
| `Math.floor(learnedLevel * 100 * zenyCostReduction)` | Mammonite zeny cost | 12932 |

These are correct per game design, but having them inline makes verification difficult. A skill-level data table would be cleaner.

### 5D. Hardcoded Fallback Stats
```js
target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 }
```
Appears ~12 times. Should be a constant `DEFAULT_ENEMY_STATS`.

---

## 6. Inconsistent Validation (Pass 6)

### 6A. Target Validation — Inconsistent Between Skills
Most physical skills validate: `if (!targetId || !isEnemy)` (strict enemy-only).
Most magic skills validate: `if (!targetId)` (allows player targets for PvP).
Some support skills don't validate targetId at all (they fall through to self-cast).

**Missing validation**:
- **Blitz Beat** (line 18009): Validates `if (!targetId)` but the handler only has an enemy path. If `isEnemy` is false and PVP is disabled, it will try to `enemies.get(playerId)` and fail silently.
- **Arrow Crafting** (line 20309): No quantity validation beyond "check if item exists in inventory."

### 6B. Zone Validation — Some Single-Target Handlers Don't Check Zone
Most AoE handlers correctly filter `enemy.zone !== zone`. However, some single-target skill handlers do not verify the target enemy is in the same zone:
- The range check implicitly enforces this (cross-zone enemies are far away), but if zone coordinates overlap, it could theoretically hit cross-zone targets.
- Only a few handlers explicitly check: `if (enemy.zone !== zone)` (Back Stab line 19504, Divest line 19777, Acid Terror line 20389).

### 6C. Dead Target Re-Validation After Async Operations
Several handlers perform `await getPlayerPosition(characterId)` or `await pool.query(...)` between the initial target check and the damage application. In theory, the target could die between the check and the apply. This is a minor race condition inherent to async handlers.

### 6D. Missing Weapon Type Validation
Some skills that require specific weapon types don't validate:
- **Cart Revolution** (line 12946): Validates `player.hasCart` but not weapon type (should work with any weapon, correct).
- **Pierce** (line 13106): No spear requirement check (should require spear per RO Classic).
- **Brandish Spear** (line 13294): No spear/riding requirement check.
- **Bowling Bash** (line 13502): No weapon requirement check (should work with any weapon, correct).

### 6E. Missing SP Re-Validation After Cast
When a skill has cast time, the SP check happens before the cast begins (line 9364). When the cast completes (`_castComplete` flag), the SP is not re-validated. If SP was drained during the cast (e.g., by enemy skills or SP consumption), the skill will still execute with potentially negative SP.

---

## 7. Summary of Findings

### By Severity

| Severity | Count | Key Items |
|----------|-------|-----------|
| CRITICAL | 1 | Rogue Double Strafe wrong args (2A) |
| HIGH | 5 | Missing buff_removed broadcasts (2B), 4 major duplication opportunities (4A-4D) |
| MEDIUM | 5 | Redundant CC checks (2C), function alias inconsistency (2D), Lex Aeterna duplication (2E), multi-hit loop duplication (4E), AoE loop duplication (4F) |
| LOW | 6 | MVP tracking (2F), Raid debuff (2G), zeny event naming (3A), effect_damage missing fields (3E), magic numbers (5A-5D), validation gaps (6A-6E) |

### Top Refactoring Priorities

1. **Fix Rogue Double Strafe** (2A) — immediate runtime crash/failure, 1-line fix
2. **Extract `applySkillDamageToEnemy` helper** (4A) — consolidates steps 2-8 from ~80 handlers
3. **Extract `resolveTarget` helper** (4B) — eliminates ~15 copy-paste target resolution blocks
4. **Add missing `skill:buff_removed` emissions** (2B) — 16 handlers need 1 line each
5. **Normalize zeny event name** (3A) — pick one (`inventory:zeny_update` or `zeny:update`) and use it everywhere
6. **Extract range check helper** (4C) — eliminates ~30 duplicated range-check blocks
7. **Set `_lastDamageDealt` consistently** (2F) — add to all damage handlers or move into `processEnemyDeathFromSkill`
8. **Call `decrementRaidDebuffHits` consistently** (2G) — add to all damage paths
9. **Remove redundant per-handler CC checks** (2C) — global handler already covers this
10. **Standardize function names** (2D) — remove `getBuffStatModifiers` alias, use `getCombinedModifiers` everywhere; remove `calculateDerivedStats` alias, use `roDerivedStats` everywhere

### Pattern Compliance Score
- Skill dispatch: if/return chain — **consistent** (100% compliance)
- Buff application: applyBuff + broadcastToZone — **consistent** (95%+ compliance)
- Status application: applyStatusEffect + broadcast — **consistent** (90%+ compliance)
- Damage pipeline: calculateSkillDamage/MagicSkillDamage — **consistent** (95%+ compliance)
- Post-damage: checkDamageBreakStatuses — **present in all damage handlers, but broadcast incomplete** (66% full compliance)
- Event naming: domain:action — **mostly consistent** (1 zeny event deviation)
- Error responses: domain:error + { message } — **consistent** (100% compliance)
- Zone filtering on AoE: **consistent** (100% of AoE loops checked)
- skill:used epilogue: **consistent** (100%)
- combat:health_update epilogue: **consistent** (100%)
