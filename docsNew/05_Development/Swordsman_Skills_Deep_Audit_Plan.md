# Swordsman Skills Deep Audit — RO Classic Pre-Renewal Compliance

**Date:** 2026-03-16 (v2 — deep research verified)
**Scope:** All 10 Swordsman skills (IDs 100-109)
**Reference:** rAthena pre-renewal source (battle.c/battle.cpp, status.c, skill.c), iRO Wiki Classic, RateMyServer, rAthena GitHub Issues #8193/#8187

---

## Executive Summary

Audited all 10 Swordsman skills line-by-line against RO Classic pre-renewal mechanics, verified via deep research against rAthena source code, iRO Wiki Classic, and RateMyServer. Found **8 actionable bugs** (4 critical, 4 moderate) and **6 deferred items**.

**Key corrections from v1 plan:**
- C1 (Bash HIT): rAthena code is `hitrate += hitrate * 5 * skill_lv / 100` — multiplicative on **hitrate**, not HIT stat. Our code uses HIT stat as the base. Needs `hitRatePercent` damage formula parameter.
- C3 (Moving HP Recovery 50%→25%): **REMOVED.** RateMyServer pre-renewal explicitly says "50% of standing recovery." The 25% is Renewal-only. Our 50% is correct.
- M3 (Magnum Break HIT): Also multiplicative on hitrate (`hitrate += hitrate * 10 * skill_lv / 100`), not flat +10.
- M4 (Mastery ATK): rAthena Issue #8193 confirms mastery is added **after skill ratio AND after DEF** in pre-renewal. Worse than initially thought.

---

## Issues Found

### CRITICAL — Must Fix

| # | Skill | Issue | Severity |
|---|-------|-------|----------|
| C1 | Bash (103) | HIT bonus uses HIT stat as base — should use hitrate (80+HIT-FLEE) as base | CRITICAL |
| C2 | Inc HP Recovery (102) | Bonus applied TWICE (in 6s AND 10s regen ticks) | CRITICAL |
| C3 | Inc HP Recovery (102) | Bonus still applies while moving — should be excluded | CRITICAL |
| C4 | Bash (103) + Magnum Break (105) | Missing `checkDamageBreakStatuses()` — won't break Frozen/Stone/Sleep | CRITICAL |

### MODERATE — Should Fix

| # | Skill | Issue | Severity |
|---|-------|-------|----------|
| M1 | Magnum Break (105) | AoE center uses ground coordinates — should always use caster position (self-centered) | MODERATE |
| M2 | Magnum Break (105) | AoE radius 300 — should be 250 (consistent with other 5x5 skills) | MODERATE |
| M3 | Magnum Break (105) | Missing HIT bonus — should be multiplicative hitrate bonus of +10%*SkillLv | MODERATE |
| M4 | All Mastery Passives | `passiveATK` multiplied by skill ratio and reduced by DEF — rAthena adds mastery AFTER both | MODERATE (system-wide) |

### DEFERRED — Acknowledged Gaps

| # | Skill | Issue | Blocked By |
|---|-------|-------|-----------|
| D1 | Provoke (104) | Missing VIT resistance for player targets | PvP system |
| D2 | Bash (103) | Stun only applies to enemies, not players | PvP system |
| D3 | Endure (106) | No anti-flinch on client | Flinch animation system |
| D4 | Bash (103) | No weapon type restriction (bows shouldn't work) | Weapon restriction system |
| D5 | Auto Berserk (108) | 300s duration instead of infinite | No-expiry buff flag |
| D6 | Magnum Break (105) | Fire endow doesn't apply to dual wield left hand | Dual wield fire endow hook |

---

## Detailed Analysis Per Skill

### Sword Mastery (100) — PASS (with system-wide note M4)

| Aspect | Expected (RO Classic) | Actual | Status |
|--------|----------------------|--------|--------|
| ATK bonus | +4 per level | +4 per level | CORRECT |
| Weapon restriction | Daggers & 1H swords | `wType === 'dagger' \|\| wType === 'one_hand_sword'` | CORRECT |
| Non-stacking with Rogue | `Math.max(learned[100], learned[1705])` | Implemented | CORRECT |
| Mastery added after skill% and DEF | Added AFTER skillRatio AND DEF | Added BEFORE both (M4) | **BUG** |

**M4 Detail (system-wide — confirmed by rAthena Issue #8193):**

rAthena Issue #8193 documents the pre-renewal damage order from official server packet analysis:

> "GetAttackFinalDamage seems to take the damage calc'd from the skill ratio, subtracts the values as necessary towards def, **then adds refine atk/mastery atk/star crumbs/spirit spheres**, does element mult, does berserk mult"

Pre-renewal damage order:
```
1. (StatusATK + WeaponATK) × SkillRatio%
2. - DEF (soft + hard)
3. + MasteryATK + RefineATK + StarCrumbs + SpiritSpheres
4. × Element modifier
5. × Card modifiers (race/size/element)
6. × Berserk multiplier
```

Our code:
```
1. (StatusATK + WeaponATK + MasteryATK) × SkillRatio%  ← mastery is HERE (wrong)
2. × Critical bonus
3. × BuffATK multiplier
4. - DEF
5. × Element modifier
6. × Card modifiers
```

Impact at Bash Lv10 (400%) with Sword Mastery Lv10 (+40 ATK):
- rAthena: damage = (baseATK * 4.0) - DEF + 40 → mastery adds flat +40
- Our code: damage = ((baseATK + 40) * 4.0) - DEF → mastery inflated to +160, then DEF applied to it

**Location:** `ro_damage_formulas.js` line ~594
**Fix:** Separate PR — move `passiveATK` to after skill multiplier AND after DEF subtraction. Affects ALL mastery passives across ALL classes. Requires full regression testing.

---

### Two-Handed Sword Mastery (101) — PASS

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| ATK bonus | +4/lv with 2H swords | +4/lv with `two_hand_sword` | CORRECT |
| Prerequisites | Sword Mastery Lv1 | `[{ skillId: 100, level: 1 }]` | CORRECT |

Same M4 system-wide note applies.

---

### Increase HP Recovery (102) — 2 BUGS

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| Flat regen bonus | +5*lv per 10s tick (while standing) | +5*lv in 6s tick AND 10s tick | **BUG (C2)** |
| MaxHP% regen | +lv*MaxHP/500 per 10s tick | Same formula in both ticks | **BUG (C2)** |
| Item heal bonus | +10%/lv on consumables (pre-renewal) | `floor(hpHeal * (100 + lv*10) / 100)` | CORRECT |
| No effect while moving | Excluded while moving | Still included while moving | **BUG (C3)** |
| Tick interval | 10 seconds while standing | 10s tick ✓ (but also 6s tick, see C2) | **BUG (C2)** |

**C2 Detail — Double-Counting Regen:**

The Inc HP Recovery bonus is applied in TWO separate setInterval ticks:

**6-second natural regen tick** (line ~18797):
```js
hpRegen += passive.hpRegenBonus;  // = hprLv * 5
const ihpLv = player.learnedSkills?.[102] || 0;
if (ihpLv > 0) hpRegen += Math.floor(player.maxHealth * ihpLv * 0.002);
```

**10-second skill regen tick** (line ~18886):
```js
const hprLv = learned[102] || 0;
if (hprLv > 0) hpBonus = Math.floor(hprLv * 5 + hprLv * player.maxHealth / 500);
```

Both compute identical formula: `lv*5 + lv*maxHP/500`. Player gets this every 6s AND every 10s.

**rAthena reference:** SM_RECOVERY adds `skill*5 + skill*max_hp/500` in the 10-second skill regen tick only. Confirmed by rAthena `status.c` and [rAthena forum](https://rathena.org/board/topic/82845-sm_recovery-and-bonus-bhprecovrate/).

**iRO Wiki confirms:** "enables the natural recovery of additional HP every 10 seconds while the user is not moving"

**Fix:** Remove Inc HP Recovery from the 6-second natural regen tick. Keep only in the 10-second tick.
- Delete the `hpRegenBonus` addition and `ihpLv * MaxHP * 0.002` lines from the 6s tick
- The 10-second tick already has the correct formula and interval

**C3 Detail — No Exclusion While Moving:**

**RateMyServer (pre-renewal):** Inc HP Recovery is "not affected by Increase Recuperative Power skill" while moving. iRO Wiki: "every 10 seconds while the user is not moving."

**Fix:** In the 10-second skill regen tick, add movement check:
```js
if (hprLv > 0 && player.health < player.maxHealth) {
    const isMoving = player.lastMoveTime && (now - player.lastMoveTime < 4000);
    if (!isMoving) {
        hpBonus = Math.floor(hprLv * 5 + hprLv * player.maxHealth / 500);
    }
}
```

**Item heal bonus verification:**
RateMyServer pre-renewal explicitly states: "Increases the effect of healing items by (10*SkillLV)%". Our +10%/lv implementation is CORRECT.

---

### Bash (103) — 2 BUGS

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| Damage % | 100 + 30*lv (130-400%) | `130 + i*30` (130-400%) | CORRECT |
| SP cost | Lv1-5: 8, Lv6-10: 15 | `i < 5 ? 8 : 15` | CORRECT |
| Element | Inherits weapon element | `skill.element === 'neutral' ? null` → weapon element | CORRECT |
| HIT bonus | Multiplicative on hitrate: `+5%*lv` of hitrate | Multiplicative on HIT stat: `+5%*lv` of HIT | **BUG (C1)** |
| Fatal Blow stun | `(bashLv-5) * baseLv / 10` % | Same formula | CORRECT |
| Stun requires Fatal Blow | `fatalBlowChance > 0 && lv >= 6` | Same check | CORRECT |
| Stun duration | 5 seconds | `baseDuration: 5000` | CORRECT |
| Stun VIT resistance | Via `applyStatusEffect()` | Via `applyStatusEffect()` | CORRECT |
| Range | Melee (150 UE) | `skill.range || MELEE_RANGE` | CORRECT |
| Status break on hit | Breaks Frozen/Stone/Sleep | Not called | **BUG (C4)** |
| Weapon restriction | All except Bow | Not enforced | DEFERRED (D4) |

**C1 Detail — Bash HIT Bonus (verified via rAthena source):**

**rAthena source** (`battle.c` / `battle.cpp`):
```c
case SM_BASH:
case MS_BASH:
    hitrate += hitrate * 5 * skill_lv / 100;
    break;
```

This is **multiplicative on hitrate** (the final hit chance percentage, = 80 + HIT - FLEE), NOT on the HIT stat.

**Our code** (line ~6510):
```js
const bashHitBonus = Math.floor(roDerivedStats(bashEffStats).hit * learnedLevel * 5 / 100);
```
This uses the **HIT stat** as the base, not hitrate.

**Difference matters when FLEE is high:**

| Scenario | HIT | FLEE | hitrate | rAthena bonus | Our bonus | rAthena final | Our final |
|----------|-----|------|---------|--------------|-----------|---------------|-----------|
| Low HIT | 50 | 100 | 30% | 15 | 25 | 45% | 55% |
| Medium | 100 | 80 | 100% | 50 | 50 | 150%→95% | 150%→95% |
| High HIT | 200 | 50 | 230% | 115 | 100 | 345%→95% | 330%→95% |

When hitrate is low (below 100), our code gives MORE bonus than rAthena because HIT > hitrate.
When hitrate is high (above HIT), our code gives LESS bonus.

**Fix — Add `hitRatePercent` option to damage formula:**

In `ro_damage_formulas.js`, after hitRate is calculated:
```js
// Apply skill-specific hitrate multiplier (e.g., Bash +5%*lv, Magnum Break +10%*lv)
if (skillOptions.hitRatePercent) {
    hitRate = Math.floor(hitRate * (100 + skillOptions.hitRatePercent) / 100);
}
```

In the Bash handler:
```js
// OLD:
const bashHitBonus = Math.floor(roDerivedStats(bashEffStats).hit * learnedLevel * 5 / 100);
const bashResult = calculateSkillDamage(..., { skillHitBonus: bashHitBonus });

// NEW:
const bashResult = calculateSkillDamage(..., { hitRatePercent: learnedLevel * 5 });
```

**Sources:**
- [rAthena battle.c SM_BASH hitrate](https://github.com/flaviojs/rathena-commits/blob/master/src/map/battle.c)
- [iRO Wiki Classic - Bash](https://irowiki.org/classic/Bash): "67% hitrate × 1.5 = 100.5% → guaranteed hit"
- [rAthena battle.cpp refactor commit](https://github.com/rathena/rathena/commit/cbd7132142cbf5c8f233fb80b1a18b66a02d23ed)

**C4 Detail — Missing checkDamageBreakStatuses (Bash):**

After `target.health = Math.max(0, target.health - damage)` (line ~6547), add:
```js
if (!isMiss && target.health > 0) {
    checkDamageBreakStatuses(target, isEnemy, bashZone);
}
```

---

### Provoke (104) — PASS (verified)

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| SP cost | 3+lv (4-13) | `4+i` where i=0 → 4-13 | CORRECT |
| Success rate | 50 + 3*lv % | `50 + learnedLevel * 3` | CORRECT |
| ATK increase | 2 + 3*lv % (5-32%) | `effectVal` = 5+i*3 = 5-32% | CORRECT |
| DEF decrease | 5 + 5*lv % (10-55%) | `5 + learnedLevel * 5` = 10-55% | CORRECT |
| Duration | 30 seconds | `duration \|\| 30000` | CORRECT |
| Boss immunity | Before SP deduction | `modeFlags.boss \|\| modeFlags.isBoss` | CORRECT |
| Undead immunity | Before SP deduction | `eleType === 'undead'` | CORRECT |
| Force aggro | On success | `setEnemyAggro(target, characterId, 'skill')` | CORRECT |
| Breaks Play Dead | On player targets | `hasBuff(target, 'play_dead')` → remove | CORRECT |
| Range | 9 cells (450 UE) | `skill.range \|\| 450` | CORRECT |
| VIT resistance (players) | VIT-based in PvP | Not implemented | DEFERRED (D1) |

**Verified by:** [iRO Wiki Classic - Provoke](https://irowiki.org/classic/Provoke) — all level values match exactly.

---

### Magnum Break (105) — 4 BUGS

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| Damage % | 100 + 20*lv (120-300%) | `120+i*20` (120-300%) | CORRECT |
| SP cost | 30 all levels | `spCost: 30` | CORRECT |
| ACD | 2000ms | `afterCastDelay: 2000` | CORRECT |
| Element | Fire (forced) | `skillElement: 'fire'` | CORRECT |
| HP cost | 21 - ceil(lv/2) | Same formula | CORRECT |
| Cannot kill | max(1, hp-cost) | `Math.max(1, ...)` | CORRECT |
| checkAutoBerserk after HP | Called | `checkAutoBerserk(player, ...)` | CORRECT |
| AoE size | 5x5 cells | 300 UE radius (too large) | **BUG (M2)** |
| AoE center | Self-centered | Uses ground coords with fallback | **BUG (M1)** |
| HIT bonus | +10%*lv multiplicative on hitrate | Not implemented | **BUG (M3)** |
| Knockback | 2 cells from caster | `knockbackTarget(enemy, attackerPos, 2)` | CORRECT |
| Fire endow | +20% fire bonus on auto-attacks for 10s | `fireBonusDamage: 20, duration: 10000` | CORRECT |
| Fire endow NOT element conversion | Adds 20% fire damage on top | Our code adds 20% fire on top | CORRECT |
| Fire endow element mod | Subject to element table | `getElementModifier('fire', ...)` | CORRECT |
| Status break on hit | Breaks Frozen/Stone/Sleep | Not called | **BUG (C4)** |
| Zone filtering | Filter by zone | `enemy.zone !== mbZone` | CORRECT |

**Fire endow verified:** iRO Wiki Classic explicitly states: "It is a common misconception that this skill endows weapons with the Fire property... in actuality, this skill adds an additional 20% Fire property damage on top of all normal attacks." Our implementation is CORRECT.

**M1 Detail — Self-Centered AoE:**
Magnum Break is self-centered. Replace ground-coord logic with `const centerPos = { ...attackerPos };`

**M2 Detail — AoE Radius:**
iRO Wiki Classic: "5x5 cells." Current 300 UE (6-cell radius) → change to 250 UE (consistent with Thunderstorm/Fire Ball 5x5 convention).

**M3 Detail — Missing Multiplicative HIT Bonus:**

**iRO Wiki Classic:** "Accuracy: 110% (Lv1) to 200% (Lv10)" — this means hitrate is multiplied by 1.1 to 2.0.

**rAthena source** (`battle.c`):
```c
case SM_MAGNUM:
    hitrate += hitrate * 10 * skill_lv / 100;
    break;
```

**Fix:** Use the same `hitRatePercent` parameter as Bash:
```js
{ skillElement: 'fire', hitRatePercent: learnedLevel * 10 }
```

**C4 Detail — Missing checkDamageBreakStatuses (Magnum Break):**
After `enemy.health = Math.max(0, enemy.health - damage)` in the AoE loop:
```js
if (!isMiss && enemy.health > 0) {
    checkDamageBreakStatuses(enemy, true, mbZone);
}
```

---

### Endure (106) — PASS (verified)

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| SP cost | 10 all levels | `spCost: 10` | CORRECT |
| Cooldown | 10 seconds | `cooldown: 10000` | CORRECT |
| Duration | 7+3*lv seconds (10-37s) | `10000+i*3000` (10000-37000) | CORRECT |
| MDEF bonus | +1/lv (soft MDEF) | `mdefBonus = effectVal` (1-10) | CORRECT |
| Hit counter | 7 monster hits | `hitCount: 7` | CORRECT |
| Counter decrement | Monster hits only | In enemy auto-attack path only | CORRECT |
| Counter removal | Remove buff at 0 | `removeBuff` + broadcast | CORRECT |
| Refresh on recast | Refreshes duration+counter | `stackRule: 'refresh'` | CORRECT |
| Anti-flinch | Prevents walk delay | Not implemented (client) | DEFERRED (D3) |

**Verified by:** [rAthena Pre-renewal DB - Endure](https://pre.pservero.com/skill/SM_ENDURE), [rAthena status_change.txt](https://github.com/rathena/rathena/blob/master/doc/status_change.txt), [Modifying Endure Hit Count thread](https://rathena.org/board/topic/90140-modifying-endure-hit-count/). MDEF bonus confirmed valid for pre-renewal via [rAthena Issue #693](https://github.com/rathena/rathena/issues/693).

---

### Moving HP Recovery (107) — PASS (corrected from v1)

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| Moving regen rate | 50% of standing rate (pre-renewal) | `movingPenalty = 0.5` | CORRECT |
| Movement detection | lastMoveTime within 4s | Same | CORRECT |
| Without passive | No regen while moving | `continue` (blocked) | CORRECT |

**v1 plan said 25% — this was WRONG.** Deep research confirms:
- **RateMyServer (pre-renewal):** "50% of standing recovery" ([source](https://ratemyserver.net/index.php?page=skill_db&skid=144))
- **iRO Wiki:** "at half the rate of if you were not moving" = 50% of standing
- **The 25% figure is Renewal only.** RateMyServer Renewal: "Only 25% of the HP that is recovered while standing"

Our `movingPenalty = 0.5` is CORRECT for pre-renewal.

---

### Auto Berserk (108) — PASS (verified)

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| SP cost | 1 to toggle on, 0 to off | `spCost: 1`, off path skips SP | CORRECT |
| HP threshold | < 25% MaxHP | `player.health / player.maxHealth < 0.25` | CORRECT |
| ATK increase | +32% (Provoke Lv10) | `atkMultiplier *= 1.32` | CORRECT |
| DEF decrease | -55% VIT DEF | `defMultiplier *= 0.45` | CORRECT |
| Toggle behavior | On/off | `hasBuff` check → remove or apply | CORRECT |
| Dynamic activation | On all HP changes | `checkAutoBerserk()` called | CORRECT |
| Undispellable | Cannot be Dispelled | In `UNDISPELLABLE` Set | CORRECT |
| Duration | Infinite (until toggle/death) | 300000ms (5 min) | DEFERRED (D5) |

---

### Fatal Blow (109) — PASS (verified)

| Aspect | Expected | Actual | Status |
|--------|----------|--------|--------|
| Max level | 1 | `maxLevel: 1` | CORRECT |
| Stun formula | `(bashLv-5) * baseLv / 10` % | Same formula | CORRECT |
| Requires Bash Lv6+ | Check in handler | `learnedLevel >= 6` | CORRECT |
| Stun duration | 5 seconds | `baseDuration: 5000` | CORRECT |
| VIT resistance | Via status system | `applyStatusEffect()` handles | CORRECT |

**Verified by:** rAthena `skill.c`: `status_change_start(src, bl, SC_STUN, (skill_lv-5)*sd->status.base_level*10, ...)` on 0-10000 scale → `(lv-5)*baseLv/10` percent. Our formula matches exactly.

---

## Fix Implementation Plan

### Phase A: Critical Fixes (4 items)

**A1: Add `hitRatePercent` option to damage formula + fix Bash HIT bonus (C1)**
- File: `server/src/ro_damage_formulas.js` — add `hitRatePercent` to skillOptions
- After hitRate is calculated from HIT-FLEE, apply: `hitRate = floor(hitRate * (100 + hitRatePercent) / 100)`
- File: `server/src/index.js` Bash handler — change `skillHitBonus` to `hitRatePercent: learnedLevel * 5`
- Impact: Bash hitrate bonus now correctly multiplies the final hit chance, not the HIT stat

**A2: Fix Inc HP Recovery Double-Counting (C2)**
- File: `server/src/index.js` lines ~18797-18799 (6-second regen tick)
- Change: Remove the 3 lines that add `hpRegenBonus` and `ihpLv * MaxHP * 0.002`
- Keep: The 10-second skill regen tick (line ~18886) which has the correct formula and interval
- Impact: Inc HP Recovery applies once per 10s, not also per 6s

**A3: Exclude Inc HP Recovery While Moving (C3)**
- File: `server/src/index.js` line ~18886 (10-second skill regen tick)
- Change: Add `const isMoving = player.lastMoveTime && (now - player.lastMoveTime < 4000)` check
- Only apply Inc HP Recovery bonus when NOT moving
- Impact: Matches iRO Wiki: "every 10 seconds while the user is not moving"

**A4: Add checkDamageBreakStatuses to Bash + Magnum Break (C4)**
- File: `server/src/index.js` Bash handler (after damage application)
- File: `server/src/index.js` Magnum Break handler (in AoE loop + PvP loop)
- Change: Add `checkDamageBreakStatuses(target, isEnemy, zone)` after damage on non-miss hits
- Impact: Physical skill damage now correctly breaks Frozen/Stone/Sleep

### Phase B: Moderate Fixes (3 items)

**B1: Fix Magnum Break Self-Centered AoE (M1)**
- File: `server/src/index.js` line ~6720
- Change: Always use `const centerPos = { ...attackerPos }` (remove ground-coord path)

**B2: Fix Magnum Break AoE Radius (M2)**
- File: `server/src/index.js` line ~6739
- Change: `AOE_RADIUS = 300` → `AOE_RADIUS = 250`

**B3: Add Magnum Break HIT Bonus (M3)**
- File: `server/src/index.js` Magnum Break AoE loop
- Change: Add `hitRatePercent: learnedLevel * 10` to skillOptions (using the new param from A1)
- At Lv10: hitrate is multiplied by 2.0 (200% accuracy)

### Phase C: System-Wide (separate PR, larger refactor)

**C1: Mastery ATK After Skill Ratio + DEF (M4)**
- File: `server/src/ro_damage_formulas.js`
- Change: Move `passiveATK` from step 1 (combined with base ATK before skill%) to step 3 (after skill% and DEF)
- Pre-renewal order: `(baseATK * skillRatio / 100) - DEF + masteryATK`
- Affects ALL mastery passives across ALL classes (Sword/2H/Spear/Katar/Fists/Mace/Book/Music/Dance/Axe/Weaponry Research/Hilt Binding)
- Requires full regression testing of all physical skills
- Source: [rAthena Issue #8193](https://github.com/rathena/rathena/issues/8193)

---

## Verification Checklist

After applying fixes, verify:

- [ ] Bash Lv10 with hitrate 30% (low HIT, high FLEE) → bonus = floor(30 * 50/100) = +15 → hitrate 45%
- [ ] Bash Lv10 with hitrate 100% → bonus = floor(100 * 50/100) = +50 → hitrate 150% → capped 95%
- [ ] Bash Lv6+ with Fatal Blow → stun chance scales with base level
- [ ] Bash on Frozen enemy → freezing broken on damage
- [ ] Provoke on Boss → error, no SP consumed
- [ ] Provoke on Undead → error, no SP consumed
- [ ] Provoke Lv10 → +32% ATK, -55% DEF on target
- [ ] Magnum Break AoE → centered on caster, not ground position
- [ ] Magnum Break AoE → 250 UE radius
- [ ] Magnum Break Lv10 → hitrate multiplied by 2.0 (200% accuracy)
- [ ] Magnum Break on Frozen enemies → freezing broken on damage
- [ ] Magnum Break fire endow → +20% fire bonus on auto-attacks for 10s
- [ ] Magnum Break HP cost → cannot kill (min 1 HP)
- [ ] Endure → 7 hit counter decrements on monster hits only
- [ ] Endure → MDEF bonus = skill level (confirmed pre-renewal)
- [ ] Endure → duration 10-37s by level
- [ ] Inc HP Recovery → regen bonus ONLY in 10-second tick (not 6-second)
- [ ] Inc HP Recovery → NOT applied while moving
- [ ] Inc HP Recovery → +10%/lv item heal bonus still works
- [ ] Moving HP Recovery → 50% of standing regen rate (CORRECT, pre-renewal)
- [ ] Auto Berserk → activates at <25% HP, deactivates at >=25%
- [ ] Auto Berserk → +32% ATK, -55% VIT DEF when active
- [ ] Sword Mastery → +4 ATK/lv with daggers/1H swords
- [ ] 2H Sword Mastery → +4 ATK/lv with 2H swords

---

## Skills Confirmed Working Correctly (7 of 10)

These skills passed all checks with zero issues:

1. **Sword Mastery (100)** — +4 ATK/lv, weapon restriction, non-stacking with Rogue
2. **Two-Handed Sword Mastery (101)** — +4 ATK/lv, weapon restriction, prerequisites
3. **Provoke (104)** — Success rate, ATK/DEF modifiers, immunities, aggro, Play Dead break
4. **Endure (106)** — Duration, MDEF, 7-hit counter, cooldown, refresh
5. **Moving HP Recovery (107)** — 50% standing rate (pre-renewal CORRECT)
6. **Auto Berserk (108)** — Toggle, threshold, ATK/DEF modifiers, dynamic activation
7. **Fatal Blow (109)** — Stun formula with base level scaling, Bash Lv6+ requirement

---

## Sources (all verified 2026-03-16)

- [rAthena battle.c (SM_BASH hitrate)](https://github.com/flaviojs/rathena-commits/blob/master/src/map/battle.c) — `hitrate += hitrate * 5 * skill_lv / 100`
- [rAthena battle.cpp refactor commit](https://github.com/rathena/rathena/commit/cbd7132142cbf5c8f233fb80b1a18b66a02d23ed)
- [rAthena Issue #8193 — Pre-Renewal Damage Rework](https://github.com/rathena/rathena/issues/8193) — mastery ATK after skillratio + DEF
- [rAthena Issue #8187 — Magnum Break Damage](https://github.com/rathena/rathena/issues/8187)
- [iRO Wiki Classic — Bash](https://irowiki.org/classic/Bash)
- [iRO Wiki Classic — Magnum Break](https://irowiki.org/classic/Magnum_Break) — fire endow is +20% bonus, NOT element conversion
- [iRO Wiki Classic — Provoke](https://irowiki.org/classic/Provoke) — all level values confirmed
- [iRO Wiki — Increase HP Recovery](https://irowiki.org/wiki/Increase_HP_Recovery) — "+5*SkillLv per 10s while not moving"
- [iRO Wiki — HP Recovery](https://irowiki.org/wiki/HP_Recovery) — tick intervals: 6s standing, 12s moving
- [RateMyServer — Moving HP Recovery](https://ratemyserver.net/index.php?page=skill_db&skid=144) — pre-renewal: "50% of standing recovery"
- [RateMyServer — Inc HP Recovery](https://ratemyserver.net/index.php?page=skill_db&skid=4) — pre-renewal: "+10*SkillLV% healing items"
- [rAthena Pre-renewal DB — Endure](https://pre.pservero.com/skill/SM_ENDURE)
- [rAthena status_change.txt](https://github.com/rathena/rathena/blob/master/doc/status_change.txt)
- [SM_Recovery and bHPrecovRate — rAthena Forum](https://rathena.org/board/topic/82845-sm_recovery-and-bonus-bhprecovrate/)
- [rAthena Issue #693 — Official MDEF bonus](https://github.com/rathena/rathena/issues/693) — Endure MDEF confirmed pre-renewal
