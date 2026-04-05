# Swordsman Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14
**Status:** IMPLEMENTED (2026-03-15) -- All 8 fixes applied
**Scope:** All 10 Swordsman class skills (IDs 100-109)
**Skill:** `/sabrimmo-skill-swordsman` — full Swordsman-specific reference

---

## Executive Summary

Deep research against iRO Wiki (classic and renewal), rAthena pre-renewal database, and RateMyServer reveals **significant gaps** in 7 of 10 Swordsman skills. Sword Mastery (100) and Two-Handed Sword Mastery (101) are fully correct. Fatal Blow (109) is mostly correct but the stun chance formula is missing the base-level scaling factor from rAthena. The remaining 7 skills have issues ranging from missing mechanics (Magnum Break fire endow buff, HP cost, wrong targetType; Endure missing 7-hit count; Increase HP Recovery bonus not applied in regen tick or item healing; Auto Berserk missing -55% DEF penalty) to incorrect data definitions (Bash damage percentages don't exactly match rAthena, Magnum Break uses cooldown instead of afterCastDelay).

**New systems needed:**
1. Magnum Break fire endow buff (20% fire bonus damage for 10 seconds after cast)
2. Endure 7-hit count tracking (breaks after 7 monster hits, unlimited player hits)
3. Increase HP Recovery item heal bonus integration (+10% per level on consumable HP healing)
4. Auto Berserk DEF reduction (-55% VIT DEF when active)
5. Bash/Magnum Break HIT bonus (multiplicative accuracy bonus)

---

## Skill-by-Skill Analysis

---

### 1. SWORD MASTERY (ID 100) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | iRO Wiki, rAthena |
| Max Level | 10 | All sources |
| ATK Bonus | +4 per level (+4 to +40) | iRO Wiki Classic, rAthena |
| Weapon Types | One-Handed Swords AND Daggers | iRO Wiki Classic, rAthena |
| Mechanic | Mastery ATK bypasses DEF, added at end of damage calc before elemental modifier | iRO Wiki Classic |
| Prerequisites | None | All sources |
| Classes | Swordman, Rogue, Stalker, Super Novice | iRO Wiki Classic |

#### Current Implementation Status: CORRECT

- ATK bonus: **CORRECT** -- `bonusATK += smLv * 4` in `getPassiveSkillBonuses()` (line 409)
- Weapon check: **CORRECT** -- `wType === 'dagger' || wType === 'one_hand_sword'` (line 408)
- Definition: **CORRECT** -- `effectValue: (i+1)*4`, maxLevel 10, type 'passive'

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| None | -- | -- |

**No changes needed.**

---

### 2. TWO-HANDED SWORD MASTERY (ID 101) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | iRO Wiki Classic, rAthena |
| Max Level | 10 | All sources |
| ATK Bonus | +4 per level (+4 to +40) | iRO Wiki Classic, rAthena |
| Weapon Types | Two-Handed Swords only | iRO Wiki Classic, rAthena |
| Mechanic | Mastery ATK bypasses DEF, added at end of damage calc | iRO Wiki Classic |
| Prerequisites | Sword Mastery Lv1 | All sources |

#### Current Implementation Status: CORRECT

- ATK bonus: **CORRECT** -- `bonusATK += tsmLv * 4` (line 414)
- Weapon check: **CORRECT** -- `wType === 'two_hand_sword'` (line 413)
- Prerequisite: **CORRECT** -- `prerequisites: [{ skillId: 100, level: 1 }]`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| None | -- | -- |

**No changes needed.**

---

### 3. INCREASE HP RECOVERY (ID 102) -- Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| HP Regen Bonus | +5 HP per level per 10-second regen tick (flat addition) | iRO Wiki, rAthena |
| HP Regen MaxHP% | +0.2% MaxHP per level per tick (Lv1=0.2%, Lv10=2.0%) | iRO Wiki Classic |
| Item Heal Bonus | +10% per level (Lv1=110%, Lv10=200%) | iRO Wiki Classic, iRO Wiki |
| Item Heal Stacking | Cumulative with VIT item heal bonus | iRO Wiki |
| Prerequisites | None | All sources |
| Restriction | No regen if overweight or moving (unless Moving HP Recovery) | iRO Wiki |

#### Current Implementation Status: PARTIALLY CORRECT

- Passive defined: **CORRECT** -- `bonuses.hpRegenBonus = hprLv * 5` (line 418)
- HP regen application: **BUG** -- `hpRegenBonus` is computed but NEVER used in the HP regen tick (lines 10404-10414). The regen tick calculates base regen from MaxHP and VIT but never adds the Increase HP Recovery flat bonus.
- MaxHP% bonus: **MISSING** -- The +0.2% of MaxHP per level per tick is not implemented
- Item heal bonus: **MISSING** -- The +10% per level bonus to healing item effectiveness is not applied in `inventory:use` handler (lines 8062-8086). Only card bonuses are applied.

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| `hpRegenBonus` never added to HP regen tick | HIGH | Trivial -- add `hpRegen += passive.hpRegenBonus` at line ~10405 |
| Missing +0.2% MaxHP per level per regen tick | MEDIUM | Easy -- add `hpRegen += Math.floor(player.maxHealth * hprLv * 0.002)` |
| Missing +10% per level item heal bonus | HIGH | Medium -- integrate into `inventory:use` heal handler |

#### Implementation Notes

**HP Regen Fix** (line ~10404 in `index.js`):
```js
const passive = getPassiveSkillBonuses(player);
let hpRegen = Math.max(1, Math.floor(player.maxHealth / 200));
hpRegen += Math.floor((stats.vit || 0) / 5);
// ADD: Increase HP Recovery flat bonus + MaxHP% bonus
hpRegen += passive.hpRegenBonus; // +5 per level flat
const hprLv = player.learnedSkills?.[102] || 0;
if (hprLv > 0) hpRegen += Math.floor(player.maxHealth * hprLv * 0.002); // +0.2% MaxHP per level
```

**Item Heal Fix** (in `inventory:use` handler, after card bonuses):
```js
// Increase HP Recovery item heal bonus: +10% per level
const ihpLv = player.learnedSkills?.[102] || 0;
if (ihpLv > 0 && hpHeal > 0) {
    hpHeal = Math.floor(hpHeal * (100 + ihpLv * 10) / 100);
}
```

---

### 4. BASH (ID 103) -- Active/Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, single target, melee | All sources |
| Max Level | 10 | All sources |
| Element | Uses weapon element (neutral if no endow) | rAthena |
| Range | Melee (1 cell = ~50 UE units) | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 (ASPD-based, no extra delay) | rAthena |
| Cooldown | 0 | rAthena |
| Prerequisites | None | All sources |
| Weapon Restriction | Cannot be used with bows | RateMyServer |

**Damage & SP Table (rAthena authoritative):**

| Level | ATK% | SP Cost | HIT Bonus | Stun (w/ Fatal Blow) |
|-------|------|---------|-----------|---------------------|
| 1 | 130% | 8 | +5% | -- |
| 2 | 160% | 8 | +10% | -- |
| 3 | 190% | 8 | +15% | -- |
| 4 | 220% | 8 | +20% | -- |
| 5 | 250% | 8 | +25% | -- |
| 6 | 280% | 15 | +30% | 5% base |
| 7 | 310% | 15 | +35% | 10% base |
| 8 | 340% | 15 | +40% | 15% base |
| 9 | 370% | 15 | +45% | 20% base |
| 10 | 400% | 15 | +50% | 25% base |

**rAthena damage formula:** `skillratio += 30 * skill_lv` (base 100% + 30% per level)

**HIT Bonus:** Multiplicative -- adds a percentage of the user's HIT to itself. At Lv10, a 67% chance to hit becomes `67 * 1.5 = 100%`. This means the HIT bonus acts as `HIT *= (1 + bonus/100)`.

**Stun (Fatal Blow):** rAthena formula: `(skill_lv - 5) * base_level * 10` on 0-10000 scale = `(skill_lv - 5) * base_level / 10` percent. This is the base chance before VIT resistance. The iRO Wiki table (5/10/15/20/25%) approximates Base Level ~50. At Base Lv99, Bash Lv10 stun chance is ~49.5% base.

**Stun duration:** 5 seconds (rAthena `skill_get_time2`)

#### Current Implementation Status: MOSTLY CORRECT

- Damage formula: **CORRECT** -- `effectValue: 130 + i*30` matches rAthena `100 + 30 * skill_lv`
- SP costs: **CORRECT** -- `i < 5 ? 8 : 15` (Lv1-5=8, Lv6-10=15)
- Range: **CORRECT** -- `range: 150` (melee)
- Cast/delay: **CORRECT** -- `castTime: 0, afterCastDelay: 0`
- Element: **CORRECT** -- `element: 'neutral'` with `skillElement: null` pass-through to weapon element
- Stun mechanic: **PARTIALLY CORRECT** -- checks `hasFatalBlow` and `learnedLevel >= 6`, but uses flat `(learnedLevel - 5) * 5` instead of `(learnedLevel - 5) * baseLevel / 10`

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Cooldown set to 700ms, should be 0 | MEDIUM | Trivial -- change `cooldown: 700` to `cooldown: 0` |
| Missing HIT bonus (multiplicative +5% to +50%) | LOW | Medium -- need to apply HIT modifier in damage calc |
| Stun chance missing base level scaling | LOW | Easy -- change formula to `(learnedLevel - 5) * baseLevel / 10` |

#### Implementation Notes

**Cooldown fix** in `ro_skill_data.js`:
Change `cooldown: 700` to `cooldown: 0`. Bash has no per-skill cooldown in RO Classic -- it is limited only by ASPD.

**HIT bonus** would require passing a HIT modifier into `calculateSkillDamage()`. The accuracy bonus is multiplicative: `effectiveHIT = baseHIT * (1 + bashLevel * 5 / 100)`. This could be added as a `hitBonus` field in `skillOptions`.

**Stun formula fix** at line 5615:
```js
const baseLevel = player.baseLevel || player.level || 1;
const stunChance = (learnedLevel - 5) * baseLevel / 10;
```

---

### 5. PROVOKE (ID 104) -- Active/Single Target Debuff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target debuff | All sources |
| Max Level | 10 | All sources |
| Range | 9 cells (~450 UE units) | rAthena, RateMyServer |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 (ASPD-based) | rAthena |
| Cooldown | 1 second | rAthena |
| Duration | 30 seconds | All sources |
| Prerequisites | None | All sources |
| Immunity | Undead property AND Boss-type monsters cannot be provoked | iRO Wiki Classic, RateMyServer |
| Player DEF | Only VIT DEF is lowered on players (not hard DEF) | iRO Wiki Classic |

**Per-Level Table:**

| Level | ATK Increase | DEF Decrease | Success Rate | SP Cost |
|-------|-------------|-------------|-------------|---------|
| 1 | +5% | -10% | 53% | 4 |
| 2 | +8% | -15% | 56% | 5 |
| 3 | +11% | -20% | 59% | 6 |
| 4 | +14% | -25% | 62% | 7 |
| 5 | +17% | -30% | 65% | 8 |
| 6 | +20% | -35% | 68% | 9 |
| 7 | +23% | -40% | 71% | 10 |
| 8 | +26% | -45% | 74% | 11 |
| 9 | +29% | -50% | 77% | 12 |
| 10 | +32% | -55% | 80% | 13 |

**ATK formula:** `2 + 3 * level` percent (Lv1=5%, Lv10=32%)
**DEF formula:** `5 + 5 * level` percent (Lv1=10%, Lv10=55%)
**Success formula:** `50 + 3 * level` percent (Lv1=53%, Lv10=80%)
**SP formula:** `3 + level` (Lv1=4, Lv10=13)

#### Current Implementation Status: MOSTLY CORRECT

- ATK increase: **CORRECT** -- `effectValue: 5+i*3` (line 27), used as `provokeAtkIncrease` (line 5697)
- DEF decrease: **CORRECT** -- `provokeDefReduction = 5 + learnedLevel * 5` (line 5698)
- Success rate: **CORRECT** -- `provokeSuccessRate = 50 + learnedLevel * 3` (line 5679)
- SP cost: **CORRECT** -- `spCost: 4+i` (4 to 13)
- Range: **CORRECT** -- `range: 450` (9 cells)
- Duration: **CORRECT** -- 30000ms
- Cooldown: **CORRECT** -- 1000ms
- Force aggro on enemy: **CORRECT** -- `setEnemyAggro(target, characterId, 'skill')` (line 5708)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing Undead element immunity check | HIGH | Easy -- check `target.element === 'undead'` before applying |
| Missing Boss-type immunity check | HIGH | Easy -- check `target.modeFlags?.boss` before applying |
| SP is deducted even when target is immune | MEDIUM | Easy -- move SP deduction after immunity check |

#### Implementation Notes

Add immunity checks after target resolution but before SP deduction:
```js
if (isEnemy) {
    // Undead element immunity
    if (target.element === 'undead' || (target.stats && target.stats.element === 'undead')) {
        socket.emit('skill:error', { message: 'Undead monsters cannot be provoked' });
        return;
    }
    // Boss immunity
    if (target.modeFlags && target.modeFlags.boss) {
        socket.emit('skill:error', { message: 'Boss monsters cannot be provoked' });
        return;
    }
}
```

---

### 6. MAGNUM BREAK (ID 105) -- Active/Self-Centered AoE

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Offensive, self-centered AoE | All sources |
| Max Level | 10 | All sources |
| Element | Fire | All sources |
| AoE | 5x5 cells (splash radius 2 cells, ~250 UE units radius) | rAthena, iRO Wiki |
| Knockback | 2 cells | All sources |
| Range | 0 cells (self-centered, NOT ground-targeted) | rAthena |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 2.0 seconds (global skill lockout) | rAthena pre-RE |
| Cooldown | 0 seconds | rAthena pre-RE |
| SP Cost | 30 (all levels) | All sources |
| Prerequisites | Bash Lv5 | All sources |

**Damage, HP Cost, & HIT Bonus Table:**

| Level | ATK% | HP Cost | HIT Bonus |
|-------|------|---------|-----------|
| 1 | 120% | 20 | +10 |
| 2 | 140% | 20 | +20 |
| 3 | 160% | 19 | +30 |
| 4 | 180% | 19 | +40 |
| 5 | 200% | 18 | +50 |
| 6 | 220% | 18 | +60 |
| 7 | 240% | 17 | +70 |
| 8 | 260% | 17 | +80 |
| 9 | 280% | 16 | +90 |
| 10 | 300% | 16 | +100 |

**HP Cost formula:** `21 - ceil(level/2)` = 20, 20, 19, 19, 18, 18, 17, 17, 16, 16
**HP Cost restriction:** Cannot kill the caster (minimum 1 HP)

**Fire Endow After-Effect:** For 10 seconds after Magnum Break, all normal attacks gain an additional 20% damage of Fire property. This is NOT a weapon element endow -- it adds supplementary fire damage that bypasses DEF on top of normal attacks. Does not change the weapon's element.

**HIT Bonus:** `+10 * SkillLv` flat HIT bonus (pre-renewal specific, from RateMyServer)

#### Current Implementation Status: PARTIALLY CORRECT

- Damage formula: **CORRECT** -- `effectValue: 120+i*20` (120-300%)
- SP cost: **CORRECT** -- 30 all levels
- Fire element: **CORRECT** -- `element: 'fire'`, `skillElement: 'fire'`
- Knockback: **CORRECT** -- `knockbackTarget(enemy, attackerPos.x, attackerPos.y, 2, ...)` (line 5817)
- AoE radius: **CORRECT** -- `AOE_RADIUS = 300` (close enough to 250 canonical)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Wrong `targetType: 'ground'` -- should be self-centered AoE | HIGH | Medium -- change to `'aoe'` and refactor handler to always use caster position |
| Missing HP cost (drains 16-20 HP from caster) | HIGH | Easy -- deduct HP, check cannot kill |
| Missing fire endow buff (20% fire bonus for 10s) | HIGH | Complex -- new buff type with damage hook |
| `cooldown: 2000` should be `afterCastDelay: 2000, cooldown: 0` | MEDIUM | Trivial -- swap field values |
| Missing HIT bonus (+10 per level) | LOW | Medium -- would need to integrate into damage calc |
| `range: 50` is misleading for self-centered -- should be 0 | LOW | Trivial |

#### Implementation Notes

**1. Fix targetType** in `ro_skill_data.js`:
Change `targetType: 'ground'` to `targetType: 'aoe'` (self-centered). Change `range: 50` to `range: 0`.

**2. Fix delay fields**:
Change `afterCastDelay: 0, cooldown: 2000` to `afterCastDelay: 2000, cooldown: 0`.

**3. Refactor handler** to always use caster position (no ground coords needed):
```js
const centerPos = { ...attackerPos }; // Always self-centered
```

**4. Add HP cost** (cannot kill):
```js
const hpCost = 21 - Math.ceil(learnedLevel / 2); // 20,20,19,19,18,18,17,17,16,16
if (player.health > 1) {
    player.health = Math.max(1, player.health - hpCost); // Cannot kill
}
```

**5. Add fire endow buff** after AoE damage:
```js
applyBuff(player, {
    skillId: 105, name: 'magnum_break_fire',
    casterId: characterId, casterName: player.characterName,
    fireBonusDamage: 20, // +20% fire bonus on attacks
    duration: 10000 // 10 seconds
});
broadcastToZone(mbZone, 'skill:buff_applied', {
    targetId: characterId, targetName: player.characterName, isEnemy: false,
    casterId: characterId, casterName: player.characterName,
    skillId: 105, buffName: 'Magnum Break Fire', duration: 10000,
    effects: { fireBonusDamage: 20 }
});
```

The fire bonus damage hook would need to be added to the auto-attack combat tick:
```js
// In combat tick, after normal damage calc:
const mbFireBuff = player.activeBuffs?.find(b => b.name === 'magnum_break_fire' && Date.now() < b.expiresAt);
if (mbFireBuff) {
    const fireBonusDmg = Math.floor(normalDamage * 0.20);
    // Apply fire element bonus (bypasses DEF) using fire element table vs target element
    totalDamage += fireBonusDmg; // Simplified -- ideally apply element modifier
}
```

---

### 7. ENDURE (ID 106) -- Active/Self Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self buff | All sources |
| Max Level | 10 | All sources |
| SP Cost | 10 (all levels) | All sources |
| Cast Time | 0 (instant) | rAthena |
| After-Cast Delay | 0 | rAthena |
| Cooldown | 10 seconds | rAthena, RateMyServer |
| Prerequisites | Provoke Lv5 | All sources |
| Target | Self | All sources |

**Per-Level Table:**

| Level | Duration | MDEF Bonus |
|-------|----------|-----------|
| 1 | 10s | +1 |
| 2 | 13s | +2 |
| 3 | 16s | +3 |
| 4 | 19s | +4 |
| 5 | 22s | +5 |
| 6 | 25s | +6 |
| 7 | 28s | +7 |
| 8 | 31s | +8 |
| 9 | 34s | +9 |
| 10 | 37s | +10 |

**Duration formula:** `7 + SkillLv * 3` seconds
**MDEF formula:** `+SkillLv` MDEF

**7-Hit Mechanic:** The anti-flinch effect cancels after receiving 7 hits from monsters. Player attacks do NOT count toward this limit -- the skill continues for its full duration against player hits. (Source: RateMyServer, iRO Wiki)

**Sitting mechanic:** Sitting characters remain seated when hit while Endure is active.

**WoE restriction:** Disabled in War of Emperium.

#### Current Implementation Status: PARTIALLY CORRECT

- SP cost: **CORRECT** -- 10 all levels
- Cooldown: **CORRECT** -- 10000ms
- Duration: **CORRECT** -- `duration: 10000+i*3000` = 10s to 37s (matches `7 + lv*3` formula)
- MDEF bonus: **CORRECT** -- `effectValue: i+1` = 1 to 10

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing 7-hit count tracking (should break after 7 monster hits) | HIGH | Complex -- need hit counter on buff, decrement in damage handler |
| Endure only grants MDEF -- no anti-flinch implementation (client-side) | MEDIUM | Deferred -- requires client-side flinch animation system first |

#### Implementation Notes

**7-Hit Counter:**
Add `hitCount: 7` to the Endure buff when applied:
```js
applyBuff(player, {
    skillId: 106, name: 'endure', casterId: characterId,
    casterName: player.characterName,
    mdefBonus: mdefBonus, defReduction: 0, atkIncrease: 0,
    duration: buffDuration,
    hitCount: 7 // Track remaining hits before breaking
});
```

In every damage path where a monster hits a player (combat tick, enemy attack handler), decrement the counter:
```js
const endureBuff = target.activeBuffs?.find(b => b.name === 'endure' && Date.now() < b.expiresAt);
if (endureBuff && isEnemy) { // Only monster hits count
    endureBuff.hitCount = (endureBuff.hitCount || 7) - 1;
    if (endureBuff.hitCount <= 0) {
        removeBuff(target, 'endure');
        broadcastToZone(zone, 'skill:buff_removed', {
            targetId: characterId, isEnemy: false, buffName: 'endure', reason: 'hit_limit'
        });
    }
}
```

---

### 8. FATAL BLOW (ID 109) -- Passive (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive (quest skill) | All sources |
| Max Level | 1 | All sources |
| Prerequisites | Bash Lv6 (quest requirement, not skill tree) | iRO Wiki Classic |
| Effect | Enables Bash stun at Lv6+ | All sources |
| Stun Duration | 5 seconds (default) | rAthena |

**Stun Chance Formula (rAthena):**
```
Base chance = (BashLevel - 5) * BaseLevel / 10  (on 0-100% scale)
```
Derived from rAthena source: `status_change_start(src, bl, SC_STUN, (skill_lv-5)*sd->status.base_level*10, ...)` where rate is on 0-10000 scale.

**Example stun chances (before VIT resistance):**

| Bash Lv | Base Lv 50 | Base Lv 75 | Base Lv 99 |
|---------|-----------|-----------|-----------|
| 6 | 5.0% | 7.5% | 9.9% |
| 7 | 10.0% | 15.0% | 19.8% |
| 8 | 15.0% | 22.5% | 29.7% |
| 9 | 20.0% | 30.0% | 39.6% |
| 10 | 25.0% | 37.5% | 49.5% |

The iRO Wiki table (5/10/15/20/25%) represents Base Level ~50 values.

#### Current Implementation Status: MOSTLY CORRECT

- Passive detection: **CORRECT** -- `fatalBlowChance: fbLv * 5` in `getPassiveSkillBonuses()` (line 426)
- Bash integration: **CORRECT** -- checks `hasFatalBlow && learnedLevel >= 6` at line 5614
- Stun application: **CORRECT** -- uses `applyStatusEffect(player, target, 'stun', stunChance)` with VIT resistance

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Stun chance uses flat `(lv-5)*5` instead of `(lv-5)*baseLevel/10` | LOW | Trivial -- update formula |
| ID swap: Fatal Blow is ID 109, Moving HP Recovery is ID 107. Passive lookup uses correct IDs but iRO Wiki has Fatal Blow before Moving HP Recovery | NONE | No actual bug -- just different ordering |

#### Implementation Notes

At line 5615 in `index.js`, change:
```js
const stunChance = (learnedLevel - 5) * 5;
```
to:
```js
const baseLevel = player.baseLevel || player.level || getEffectiveStats(player).level || 1;
const stunChance = (learnedLevel - 5) * baseLevel / 10;
```

This gives more accurate stun chances at higher base levels (e.g. Bash Lv10 at Base Lv99 = 49.5% base chance vs current flat 25%).

---

### 9. MOVING HP RECOVERY (ID 107) -- Passive (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive (quest skill) | All sources |
| Max Level | 1 | All sources |
| Effect | Allows HP regen while moving at 50% of standing rate | iRO Wiki, RateMyServer (pre-renewal) |
| Sitting Rate | 4x standing rate (not affected by this skill) | iRO Wiki |
| Interaction with Inc HP Recovery | Does NOT affect Inc HP Recovery bonus -- only base regen | iRO Wiki |
| Quest Requirements | Job Level 35+, 200 Empty Bottles, 1 Moth Wing | iRO Wiki |

**Key distinction:**
- Pre-Renewal: Moving regen = 50% of standing rate
- Renewal/iRO: Moving regen = 25% of standing rate
- Our target: Pre-Renewal = 50%

#### Current Implementation Status: PARTIALLY CORRECT

- Passive flag: **CORRECT** -- `movingHPRecovery: true` in `getPassiveSkillBonuses()` (line 422)
- Regen gate: **CORRECT** -- line 10401 checks `!passive.movingHPRecovery` to skip regen while moving

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Moving regen is 100% of standing rate instead of canonical 50% | MEDIUM | Easy -- halve regen amount when `lastMoveTime` is recent and `movingHPRecovery` is true |

#### Implementation Notes

At line 10401 in the HP regen tick, instead of just bypassing the movement check entirely:
```js
let movingPenalty = 1.0;
if (player.lastMoveTime && (now - player.lastMoveTime < 4000)) {
    if (!passive.movingHPRecovery) continue; // No regen while moving
    movingPenalty = 0.5; // Pre-renewal: 50% rate while moving
}
// ... after calculating hpRegen:
hpRegen = Math.max(1, Math.floor(hpRegen * movingPenalty));
```

---

### 10. AUTO BERSERK (ID 108) -- Toggle (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Toggle (quest skill) | All sources |
| Max Level | 1 | All sources |
| SP Cost | 1 SP (to toggle on) | RateMyServer |
| HP Threshold | Activates when HP < 25% MaxHP | All sources |
| Effect | Self-Provoke Level 10 | All sources |
| ATK Increase | +32% (= Provoke Lv10 ATK bonus) | RateMyServer, iRO Wiki |
| DEF Decrease | -55% VIT DEF (= Provoke Lv10 DEF penalty) | RateMyServer |
| Duration | Until HP > 25% MaxHP or manually toggled off | All sources |
| Works at 0 SP | Yes -- functions even when SP is depleted | RateMyServer |
| Quest Requirements | Job Level 30+, 35 Powder of Butterfly, 10 Horrendous Mouth, 10 Decayed Nail, 10 Honey | iRO Wiki |

**Key mechanic:** Auto Berserk is equivalent to a permanent self-Provoke Level 10. When HP drops below 25%, the Provoke Lv10 effect kicks in automatically (+32% ATK, -55% VIT DEF). When HP rises above 25%, the Provoke effect deactivates (both ATK bonus and DEF penalty removed).

#### Current Implementation Status: PARTIALLY CORRECT

- Toggle mechanic: **CORRECT** -- checks `hasBuff(player, 'auto_berserk')` for toggle off (line 7843)
- SP cost: **CORRECT** -- 1 SP
- HP threshold: **CORRECT** -- `player.health / player.maxHealth < 0.25` (line 7852)
- ATK increase: **CORRECT** -- `conditionalAtkIncrease: effectVal` = 32 (line 7855)
- Auto-check on HP change: **CORRECT** -- `checkAutoBerserk()` called on all HP change events

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing -55% VIT DEF penalty when Auto Berserk is active | HIGH | Medium -- add `defReduction: 55` to buff, integrate into `getBuffStatModifiers()` |
| Duration set to 300000ms (5 min) -- should be infinite (until HP > 25% or toggled off) | LOW | Easy -- use very large duration or no-expiry flag |

#### Implementation Notes

**DEF Reduction Fix:**
Add `defReduction: 55` to the Auto Berserk buff in both the toggle handler (line 7853) and `checkAutoBerserk()` (line 591):
```js
applyBuff(player, {
    skillId: 108, name: 'auto_berserk', casterId: characterId,
    casterName: player.characterName,
    atkIncrease: abAtkIncrease,
    conditionalAtkIncrease: effectVal,
    defReduction: shouldBeActive ? 55 : 0, // ADD THIS
    duration: duration || 300000
});
```

In `checkAutoBerserk()`, when activating (line 591):
```js
abBuff.atkIncrease = abBuff.conditionalAtkIncrease || 32;
abBuff.defReduction = 55; // ADD: Provoke Lv10 DEF penalty
```

When deactivating (line 609):
```js
abBuff.atkIncrease = 0;
abBuff.defReduction = 0; // ADD: Remove DEF penalty
```

The `getBuffStatModifiers()` / `getCombinedModifiers()` already reads `defReduction` from Provoke buffs, so Auto Berserk's `defReduction` should be picked up automatically.

---

## New Systems Required

### System 1: Magnum Break Fire Endow Buff

**Where:** New buff type `'magnum_break_fire'` in `index.js`

**Mechanic:** After Magnum Break is cast, a 10-second buff is applied that adds 20% bonus fire damage to all normal auto-attacks. This bonus damage:
- Is of Fire property (subject to elemental table vs target element)
- Bypasses DEF (added after DEF reduction, not part of ATK)
- Only applies to auto-attacks, not skills

**Implementation:**
1. Apply buff in Magnum Break handler after AoE damage
2. In combat tick auto-attack section, check for the buff and add fire bonus damage:
   ```js
   const mbFireBuff = attacker.activeBuffs?.find(b => b.name === 'magnum_break_fire' && Date.now() < b.expiresAt);
   if (mbFireBuff) {
       const fireBonusDmg = Math.floor(normalDamage * 0.20);
       // Optional: Apply fire element modifier from ELEMENT_TABLE vs target element
       totalDamage += fireBonusDmg;
   }
   ```

### System 2: Endure Hit Count Tracking

**Where:** Buff object on player, decremented in all monster-to-player damage paths

**Mechanic:** Endure buff has a `hitCount: 7` field. Each time a monster hits the player (auto-attack from enemy, enemy skill damage), decrement by 1. At 0, remove the buff. Player hits do NOT decrement.

**Damage paths to hook:**
- Enemy auto-attack in combat tick (when enemy attacks player)
- Enemy skill damage (if implemented)

### System 3: Increase HP Recovery Item Heal Bonus

**Where:** `inventory:use` handler in `index.js`, `processEffect` function, `case 'heal'` block

**Mechanic:** After calculating base HP heal and card bonuses, apply Increase HP Recovery bonus:
```js
const ihpRecovLv = player.learnedSkills?.[102] || 0;
if (ihpRecovLv > 0 && hpHeal > 0) {
    hpHeal = Math.floor(hpHeal * (100 + ihpRecovLv * 10) / 100);
}
```

This stacks multiplicatively with VIT item heal bonus (already handled separately).

---

## Skill Definition Corrections (`ro_skill_data.js`)

| Skill | Field | Current | Correct | Notes |
|-------|-------|---------|---------|-------|
| Bash (103) | cooldown | 700 | 0 | No per-skill cooldown in RO Classic; limited by ASPD only |
| Magnum Break (105) | targetType | `'ground'` | `'aoe'` | Self-centered AoE, not ground-targeted |
| Magnum Break (105) | range | 50 | 0 | Self-centered, no range needed |
| Magnum Break (105) | afterCastDelay | 0 | 2000 | 2-second global after-cast delay |
| Magnum Break (105) | cooldown | 2000 | 0 | No per-skill cooldown; the delay is ACD |
| Magnum Break (105) | description | Current text | `'AoE fire attack centered on self. Knockback, +20% Fire ATK bonus for 10s.'` | More accurate description |

---

## Implementation Priority

### Phase A: Critical Data Fixes (ro_skill_data.js + handler corrections)

1. **Fix Bash cooldown** -- change from 700 to 0
2. **Fix Magnum Break targetType** -- change from `'ground'` to `'aoe'`
3. **Fix Magnum Break range** -- change from 50 to 0
4. **Fix Magnum Break delay fields** -- swap afterCastDelay and cooldown values
5. **Fix Magnum Break handler** -- always use caster position (remove ground targeting logic)
6. **Add Increase HP Recovery to regen tick** -- apply `hpRegenBonus` + MaxHP% bonus in HP regen interval
7. **Add Provoke Undead/Boss immunity** -- check before SP deduction

### Phase B: Missing Mechanics (HP cost, DEF penalties, stun formula)

8. **Add Magnum Break HP cost** -- deduct 16-20 HP, cannot kill caster
9. **Add Auto Berserk DEF reduction** -- -55% VIT DEF when buff is active
10. **Fix Bash stun formula** -- add base level scaling: `(lv-5) * baseLv / 10`
11. **Fix Moving HP Recovery regen rate** -- 50% of standing rate while moving

### Phase C: Complex New Mechanics

12. **Add Magnum Break fire endow buff** -- 10-second buff, +20% fire bonus on auto-attacks
13. **Add Endure 7-hit counter** -- track monster hits, break buff at 0
14. **Add Increase HP Recovery item heal bonus** -- +10% per level on consumable healing

### Phase D: Polish (Low Priority)

15. **Add Bash HIT bonus** -- multiplicative +5% to +50% accuracy bonus
16. **Add Magnum Break HIT bonus** -- +10 per skill level flat HIT bonus
17. **Fix Auto Berserk duration** -- consider very long or infinite duration instead of 5min

---

## Sources

### iRO Wiki Classic (Pre-Renewal)
- [iRO Wiki Classic -- Sword Mastery](https://irowiki.org/classic/Sword_Mastery) -- ATK bonus, weapon types
- [iRO Wiki Classic -- Two-Handed Sword Mastery](https://irowiki.org/classic/Two-Handed_Sword_Mastery) -- ATK bonus, prerequisite
- [iRO Wiki Classic -- Increase HP Recovery](https://irowiki.org/classic/Increase_HP_Recovery) -- Regen formula, item heal bonus table
- [iRO Wiki Classic -- Bash](https://irowiki.org/classic/Bash) -- Damage table, SP costs, stun mechanic
- [iRO Wiki Classic -- Provoke](https://irowiki.org/classic/Provoke) -- ATK/DEF/success tables, immunities
- [iRO Wiki Classic -- Magnum Break](https://irowiki.org/classic/Magnum_Break) -- Damage, HP cost, fire endow
- [iRO Wiki Classic -- Endure](https://irowiki.org/classic/Endure) -- Duration, MDEF table
- [iRO Wiki Classic -- Fatal Blow](https://irowiki.org/classic/Fatal_Blow) -- Stun chance per Bash level

### iRO Wiki (Renewal/Combined)
- [iRO Wiki -- Bash](https://irowiki.org/wiki/Bash) -- HIT bonus formula (multiplicative), damage table
- [iRO Wiki -- Magnum Break](https://irowiki.org/wiki/Magnum_Break) -- Fire endow clarification, HIT bonus
- [iRO Wiki -- Endure](https://irowiki.org/wiki/Endure) -- Cooldown, 7-hit mechanic
- [iRO Wiki -- Increase HP Recovery](https://irowiki.org/wiki/Increase_HP_Recovery) -- MaxHP% bonus, VIT stacking
- [iRO Wiki -- HP Recovery While Moving](https://irowiki.org/wiki/HP_Recovery_While_Moving) -- 50% regen rate, interaction with Inc HP Recovery
- [iRO Wiki -- Fatal Blow](https://irowiki.org/wiki/Fatal_Blow) -- Base level affects stun chance

### rAthena Pre-Renewal Database
- [rAthena Pre-RE -- Sword Mastery](https://pre.pservero.com/skill/SM_SWORD) -- ATK bonus, weapon types
- [rAthena Pre-RE -- Bash](https://pre.pservero.com/skill/SM_BASH) -- SP cost, damage, timing
- [rAthena Pre-RE -- Provoke](https://pre.pservero.com/skill/SM_PROVOKE) -- ATK/DEF values, cooldown, duration
- [rAthena Pre-RE -- Magnum Break](https://pre.pservero.com/skill/SM_MAGNUM) -- HP cost, ACD, fire buff
- [rAthena Pre-RE -- Endure](https://pre.pservero.com/skill/SM_ENDURE) -- MDEF, duration, cooldown
- [rAthena Pre-RE -- Increase HP Recovery](https://pre.pservero.com/skill/SM_RECOVERY) -- Regen values
- [rAthena Pre-RE -- Fatal Blow](https://pre.pservero.com/skill/SM_FATALBLOW) -- Quest skill, max Lv1
- [rAthena Pre-RE -- Moving HP Recovery](https://pre.pservero.com/skill/SM_MOVINGRECOVERY) -- 50% regen rate

### RateMyServer
- [RateMyServer -- Bash](https://ratemyserver.net/index.php?page=skill_db&skid=5) -- HIT bonus, stun formula, SP costs
- [RateMyServer -- Provoke](https://ratemyserver.net/index.php?page=skill_db&skid=6) -- Full ATK/DEF/success table, immunities
- [RateMyServer -- Magnum Break](https://ratemyserver.net/index.php?page=skill_db&skid=7) -- Fire endow, HP cost, HIT bonus, pre-RE delay
- [RateMyServer -- Endure](https://ratemyserver.net/index.php?page=skill_db&skid=8) -- 7-hit mechanic, cooldown
- [RateMyServer -- Auto Berserk](https://ratemyserver.net/index.php?page=skill_db&skid=146) -- Self-Provoke Lv10, +32% ATK/-55% DEF
- [RateMyServer -- Moving HP Recovery](https://ratemyserver.net/index.php?page=skill_db&skid=144) -- 50% standing rate (pre-renewal)

### rAthena Source Code
- [rAthena Forum -- Bash Stun Rate](https://rathena.org/board/topic/140470-increase-bash-stun-rate/) -- `(skill_lv-5)*base_level*10` formula on 0-10000 scale
- [rAthena GitHub -- battle.cpp](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp) -- SM_BASH `skillratio += 30 * skill_lv`
- [rAthena GitHub -- Status Change Durations](https://github.com/rathena/rathena/issues/1019) -- Stun default 5s

### Other
- [rAthena Forum -- Status Resistance Formulas](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/) -- General stun resistance: `Chance - Chance*VIT/100 + srcLv - tarLv - tarLuk`
- [Ragnarok Wiki Fandom -- Auto Berserk](https://ragnarok.fandom.com/wiki/Auto_Berserk) -- Self-Provoke Lv10 mechanic
