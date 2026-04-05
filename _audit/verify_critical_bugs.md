# Verification Pass: Critical/High Bugs from Round 1 Audit

**Date**: 2026-03-23
**Verifier**: Claude Opus 4.6 (1M context)
**Method**: Read exact source lines, traced function signatures, confirmed argument types

---

## T1 -- Rogue Double Strafe Shifted Arguments

**Verdict: CONFIRMED -- Critical**

**Function signature** (line 1874):
```js
async function executePhysicalSkillOnEnemy(player, characterId, socket, skill, skillId, learnedLevel, levelData, effectVal, spCost, targetId, options = {})
```

**Actual call** (line 19711):
```js
await executePhysicalSkillOnEnemy(player, characterId, socket, dsZone, skill, skillId, learnedLevel, spCost, totalEffectVal, targetId, isEnemy, {});
```

**Analysis**: The call passes 12 arguments to an 11-parameter function. Starting at position 4:
- `dsZone` (string) -> `skill` (expects object) -- **WRONG**
- `skill` (object) -> `skillId` (expects number) -- **WRONG**
- `skillId` (number) -> `learnedLevel` (expects number) -- shifted value
- `learnedLevel` (number) -> `levelData` (expects object) -- **WRONG**
- `spCost` (number) -> `effectVal` -- shifted value
- `totalEffectVal` (number) -> `spCost` -- shifted value
- `targetId` -> `targetId` -- correct by coincidence (position 10)
- `isEnemy` (boolean) -> `options` (expects object) -- **WRONG**
- `{}` -> extra argument (silently ignored)

Additionally, `levelData` is completely missing from the call. This will crash or produce garbage damage when Rogue uses Double Strafe. The function tries to access `skill.range` on the zone string, `skillId.levels` on the skill object, etc.

**Impact**: Rogue Double Strafe (ID 1707) is completely non-functional. Any cast attempt will produce errors or undefined behavior.

---

## T2 -- Ki Explosion Stun: Object Passed as Duration

**Verdict: CONFIRMED -- High**

**`applyStatusEffect` signature** (`ro_status_effects.js` line 353):
```js
function applyStatusEffect(source, target, statusType, baseChance, overrideDuration)
```
Parameter `overrideDuration` expects a **number** (milliseconds).

**Ki Explosion call** (line 18543):
```js
applyStatusEffect({ level: player.stats.level || 1 }, enemy, 'stun', 100, { duration: 2000 });
```

**What happens**: Line 357: `const duration = overrideDuration || result.duration;` -- the object `{ duration: 2000 }` is truthy, so `duration` becomes the object itself. Line 368: `expiresAt: now + duration` computes `Date.now() + [object Object]` = `NaN`. The stun is applied (`applied: true` is returned and broadcast at line 18546 with hardcoded 2000ms display), but the internal `expiresAt` is `NaN`. Any expiry check `Date.now() >= NaN` returns `false`, so the stun **never naturally expires**. It will only be removed by damage-break checks or buff clearing.

**Impact**: Ki Explosion stun becomes permanent until broken by external means. This is game-breaking in PvE as stunned monsters never recover.

---

## T2b -- Hammer Fall Stun: Same Object-as-Duration Bug

**Verdict: CONFIRMED -- High**

**Hammer Fall call** (line 20111):
```js
applyStatusEffect({ level: player.level || player.stats.level || 1, luk: player.stats.luk || 1 }, enemy, 'stun', stunChance, { duration: 5000 });
```

Identical pattern to T2. Object `{ duration: 5000 }` passed where number `5000` expected. Same NaN expiresAt result. Same permanent-stun consequence.

**Impact**: Blacksmith Hammer Fall produces permanent stuns on all affected enemies in AoE. Combined with AoE targeting, this is extremely game-breaking.

---

## B1 -- BUFFS_SURVIVE_DEATH: `song_humming` vs `dance_humming`

**Verdict: CONFIRMED -- Low-Medium**

**BUFFS_SURVIVE_DEATH** (line 2032-2036):
```js
const BUFFS_SURVIVE_DEATH = new Set([
    'auto_berserk', 'endure', 'shrink',
    'song_whistle', 'song_assassin_cross', 'song_bragi', 'song_apple_of_idun',
    'song_humming', 'dance_fortune_kiss', 'dance_service_for_you', 'dance_pdfm'
]);
```

**PERFORMANCE_BUFF_MAP** (line 959):
```js
'humming': 'dance_humming',
```

The Humming (A Drum on the Battlefield) performance applies a buff named `dance_humming` (via `PERFORMANCE_BUFF_MAP` at line 959, consumed at line 1150 and applied at line 27270). However, `BUFFS_SURVIVE_DEATH` lists `song_humming` which does not match any applied buff name. The string `song_humming` appears nowhere else in the codebase.

**Impact**: The Humming buff will be incorrectly stripped on player death when it should survive. Other songs use `song_` prefix (whistle, assassin_cross, bragi, apple_of_idun) because they are Bard songs. Humming is a Dancer dance, so it correctly uses `dance_humming` at application time, but the BUFFS_SURVIVE_DEATH entry uses the wrong prefix.

**Fix**: Change `'song_humming'` to `'dance_humming'` in the Set.

---

## B2 -- UNDISPELLABLE Strip Names: `stripweapon` vs `strip_weapon`

**Verdict: CONFIRMED -- Medium**

**UNDISPELLABLE set** (line 15320):
```js
'stripweapon', 'stripshield', 'striparmor', 'striphelm',
```

**Divest skill handler** (lines 19792-19801):
```js
stripName = 'strip_weapon'; stripType = 'weapon';
stripName = 'strip_shield'; stripType = 'shield';
stripName = 'strip_armor'; stripType = 'armor';
stripName = 'strip_helm'; stripType = 'helm';
```

Buffs applied via `applyBuff(enemy, { name: stripName, ... })` at line 19815 use underscored names (`strip_weapon`, `strip_shield`, `strip_armor`, `strip_helm`). The UNDISPELLABLE set uses non-underscored names (`stripweapon`, `stripshield`, `striparmor`, `striphelm`). Since Set.has() is exact-match, Dispel will incorrectly remove strip debuffs.

**Impact**: Rogue Divest debuffs can be removed by Sage Dispel when they should be UNDISPELLABLE per RO Classic design. This undermines Rogue's core PvP utility.

**Fix**: Change UNDISPELLABLE entries to `'strip_weapon'`, `'strip_shield'`, `'strip_armor'`, `'strip_helm'`.

---

## B5 -- Blessing Early Return After Cure

**Verdict: CONFIRMED -- Intended Behavior (NOT a bug)**

**Blessing handler** (lines 11489-11500):
```js
// PATH 2: If target has Curse or Stone, cure those statuses -- do NOT apply stat buff
const curedStatuses = cleanseStatusEffects(buffTarget, ['curse', 'stone']);
if (curedStatuses.length > 0) {
    for (const type of curedStatuses) {
        broadcastToZone(bZone, 'status:removed', { ... });
        broadcastToZone(bZone, 'skill:buff_removed', { ... });
    }
    // When curing Curse/Stone, target does NOT receive stat bonuses (RO Classic)
    socket.emit('skill:used', { ... });
    socket.emit('combat:health_update', { ... });
    return;
}
// PATH 3: Normal buff -- +STR/DEX/INT
applyBuff(buffTarget, { ... });
```

The early return after curing Curse/Stone is **correct RO Classic behavior**. In pre-renewal RO, Blessing on a cursed/stoned target cures the status but does NOT apply the stat buff. The code comments explicitly state this. PATH 3 (stat buff) only runs when there are no statuses to cure.

**Impact**: None. This is working as designed per RO Classic reference.

---

## CP1 -- Monster Skill When-Hit Card Procs Missing

**Verdict: CONFIRMED -- Medium**

**`executeMonsterPlayerSkill` function** (lines 29015-29240): The function handles monster-cast player-class skills (e.g., Cold Bolt, Fire Bolt). After applying damage to the player target (line 29184), it:
- Broadcasts skill effect (line 29188)
- Sends health update (line 29202)
- Checks Plagiarism copy (line 29211)
- Handles player death (line 29215)
- Breaks damage-break statuses (line 29231)

**Missing**: No calls to `processCardStatusProcsWhenHit`, `processCardAutoSpellWhenHit`, or `processAutobonusWhenHit` anywhere in the function.

**Where they exist**: These hooks are called in the auto-attack combat tick (lines 30962-30967) but NOT in the skill damage path.

**Impact**: When a monster uses a player-class skill (e.g., Fire Bolt, Cold Bolt), the player's when-hit card effects do not trigger. This affects cards like High Orc Card (reflect), Khalitzburg Card (auto Guard vs Demon), etc. Only auto-attacks trigger when-hit effects.

---

## MS1 -- Slave Monsters Give No EXP/Drops

**Verdict: CONFIRMED -- Intended Behavior (NOT a bug)**

**Enemy death handler** (lines 2160-2178):
```js
if (enemy._isSlave && enemy._masterId) {
    const master = enemies.get(enemy._masterId);
    if (master) {
        if (master._slaves) master._slaves.delete(enemy.enemyId);
        master._slaveCount = master._slaves ? master._slaves.size : 0;
    }
    // RO Classic: Slaves give NO EXP and NO drops. Only broadcast death + cleanup.
    ...
    enemies.delete(enemy.enemyId);
    return; // Skip EXP, drops, and everything else
}
```

The code explicitly returns early for slave monsters, skipping all EXP distribution and drop generation. The comment states this is intentional RO Classic behavior.

**Verification against RO Classic**: In pre-renewal Ragnarok Online, slave monsters summoned by MVPs via `NPC_SUMMONSLAVE` do NOT give EXP or drops when killed. This is correct behavior per rAthena source (`mob.cpp` -- slaves have `MD_SUMMONSLAVE` flag and skip reward distribution).

**Impact**: None. Working as designed.

---

## AI1 -- Angry (Type 04) Re-Aggro After Kill

**Verdict: CONFIRMED -- Missing Feature (Low-Medium)**

**AI type 04 mode flags** (line 382):
```js
4:  0x3885,  // Angry: Aggressive + Assist + ChangeTargetMelee/Chase + Angry
```

The `angry` flag (0x0800) IS parsed and stored in `modeFlags.angry` (line 422):
```js
angry: !!(hexMode & MD.ANGRY),
```

However, `modeFlags.angry` is **never referenced** anywhere in the behavior logic. A grep for `modeFlags.angry` returns zero results.

**What `angry` should do** (per rAthena): When a type-04 monster's target dies or escapes, it should immediately re-aggro the nearest player instead of returning to wandering. Currently, when a target dies (line 30527-30539), the code calls `pickNextTarget(enemy)` which checks `inCombatWith` -- this handles the case where the enemy has multiple attackers. But if the `inCombatWith` map is empty (e.g., only one attacker who killed it from range), the enemy goes to IDLE and relies on the normal `aggressive` aggro scan timer. The `angry` flag should make this re-scan immediate.

**Partial mitigation**: AI type 04 DOES have `aggressive` flag set (0x3885 & 0x0004 = true), so type-04 monsters will re-aggro on the next IDLE scan tick. The difference is a potential delay of up to `AGGRO_SCAN_MS` milliseconds before re-aggro vs immediate.

**Impact**: Low-Medium. Type-04 monsters (Familiar, Zombie, Orc Warrior, etc.) may have a brief passive delay after killing their target before acquiring a new one. The `assist` flag is also parsed but only partially used (line 28637). The `changeTargetMelee`/`changeTargetChase` flags from 0x3885 are also not used in behavior logic.

---

## AI3 -- Plant-Type 1-Damage Cap Missing

**Verdict: CONFIRMED -- Missing Feature (Medium)**

Plant-type monsters (AI code 6: Red Plant, Blue Plant, Green Plant, Yellow Plant, Shining Plant, Mushroom, etc.) are defined with mode `0x0000` (line 384):
```js
6:  0x0000,  // Plant/Immobile: no flags
```

This means no `canMove`, no `canAttack`, no `aggressive` -- they just exist. However, there is **no damage cap** implemented anywhere in the damage pipeline. In RO Classic, plant-type monsters should:
1. Always take exactly 1 damage from any attack (physical or magical)
2. Never miss (100% hit rate)
3. Not be affected by elements, cards, size modifiers, or any damage multipliers

**Search results**: No references to `aiCode === 6`, `monsterClass === 'plant'`, `modeFlags.plant`, or any plant-mode damage capping logic exist in the damage pipeline functions (`calculateSkillDamage`, `calculateMagicSkillDamage`, `calculateEnemyDamage`, or the auto-attack combat tick).

**Impact**: Plant-type monsters take full damage instead of always 1. This allows high-level characters to one-shot Shining Plants, Blue Plants, etc., which are meant to be RNG loot pinatas where everyone has equal kill speed (1 damage per hit, plant HP typically 1). Currently they die to any hit since they likely have very low HP, so the practical impact depends on whether any plant has HP > 1. Plants with multiple HP (e.g., Mushroom with higher HP templates) would die much faster than intended.

---

## Summary Table

| ID | Bug Description | Verdict | Severity |
|----|----------------|---------|----------|
| T1 | Rogue Double Strafe shifted arguments | **CONFIRMED** | Critical |
| T2 | Ki Explosion stun object-as-duration (permanent stun) | **CONFIRMED** | High |
| T2b | Hammer Fall stun object-as-duration (permanent stun) | **CONFIRMED** | High |
| B1 | BUFFS_SURVIVE_DEATH `song_humming` vs `dance_humming` | **CONFIRMED** | Low-Medium |
| B2 | UNDISPELLABLE `stripweapon` vs `strip_weapon` | **CONFIRMED** | Medium |
| B5 | Blessing early return after cure | **DENIED** -- Intended RO Classic behavior | N/A |
| CP1 | Monster skill when-hit card procs missing | **CONFIRMED** | Medium |
| MS1 | Slave EXP suppression | **DENIED** -- Intended RO Classic behavior | N/A |
| AI1 | Angry (type 04) re-aggro not implemented | **CONFIRMED** | Low-Medium |
| AI3 | Plant-type 1-damage cap missing | **CONFIRMED** | Medium |

**Confirmed bugs**: 8 of 10
**Denied (intended behavior)**: 2 of 10 (B5, MS1)
**Critical/High requiring immediate fix**: 3 (T1, T2, T2b)
