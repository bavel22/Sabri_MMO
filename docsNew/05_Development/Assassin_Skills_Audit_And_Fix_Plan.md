# Assassin Skills Comprehensive Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Assassin_Class_Research](Assassin_Class_Research.md)
> **Status**: RESOLVED 2026-04-26 — All 10 BUGS + 4 of 5 ISSUES + 2 newly-discovered bugs fixed (I3 intentionally deferred per Priority 8).

**Date:** 2026-03-16 (audit) / 2026-04-26 (fixes confirmed + 2 new bugs fixed)
**Scope:** All 12 Assassin skills (IDs 1100-1111) — Full RO Classic pre-renewal compliance audit
**Status:** ALL ACTIONABLE FIXES APPLIED. See "Resolution Log" section at end of file.
**Sources:** rAthena pre-re source (authoritative), iRO Wiki Classic, RateMyServer, Divine Pride

---

## Executive Summary

Audited all 12 Assassin skills against RO Classic pre-renewal mechanics using rAthena source code, iRO Wiki Classic, and RateMyServer. Found **9 bugs** (incorrect behavior), **5 issues** (missing mechanics), and **1 cross-system bug**. Several findings contradict the original audit due to deeper source-code-level research.

| Category | Count | Impact |
|----------|-------|--------|
| Bugs (incorrect behavior) | 9 | HIGH — wrong damage formulas, wrong element, wrong limits |
| Issues (missing mechanics) | 5 | MEDIUM — incomplete but not broken |
| Cross-system bugs | 1 | HIGH — katar CRI bonus affects all Assassin gameplay |
| Correctly implemented | 4 skills fully correct | — |
| Deferred (known future) | 6 | LOW — wall adjacency, movement speed, EDP, PvP |

---

## BUGS — Must Fix

### B1: Sonic Blow damage formula wrong (too high at Lv1-9)

**Severity:** HIGH
**File:** `server/src/ro_skill_data_2nd.js` line 198
**Source:** rAthena `sonicblow.cpp`: `base_skillratio += 200 + 50 * skill_lv` (added to base 100%)

**Problem:**
Our formula is `400 + 40*Lv` (440-800%). rAthena pre-renewal is `300 + 50*Lv` (350-800%).

| Lv | Our Value | Correct (rAthena) | Difference |
|----|-----------|-------------------|------------|
| 1 | 440% | 350% | +90% over |
| 2 | 480% | 400% | +80% over |
| 3 | 520% | 450% | +70% over |
| 5 | 600% | 550% | +50% over |
| 7 | 680% | 650% | +30% over |
| 10 | 800% | 800% | Correct |

**Result:** Sonic Blow is over-powered at levels 1-9. Only Lv10 matches.

**Fix in skill data:**
```javascript
// Line 198 — CHANGE effectValue FROM:
effectValue: 440+i*40
// TO:
effectValue: 350+i*50
```

Also update description from "440-800%" to "350-800%".

---

### B2: Enchant Poison endow overwrite uses WRONG buff names

**Severity:** HIGH
**File:** `server/src/index.js` line 12069
**Also affects:** Aspersio handler at line 12820

**Problem:**
Sage endow handlers create buffs named `endow_fire`, `endow_water`, `endow_wind`, `endow_earth` (see line 11406). But Enchant Poison tries to remove `endow_blaze`, `endow_tsunami`, `endow_tornado`, `endow_quake`. These names don't match, so Sage endows are never removed.

**Fix:**
```javascript
// Line 12069 — CHANGE FROM:
const endowBuffs = ['aspersio', 'enchant_poison', 'endow_quake', 'endow_tornado', 'endow_tsunami', 'endow_blaze'];
// TO:
const endowBuffs = ['aspersio', 'enchant_poison', 'endow_fire', 'endow_water', 'endow_wind', 'endow_earth'];

// Line 12820 (Aspersio) — SAME FIX
```

---

### B3: Katar Critical Bonus not implemented (doubles TOTAL CRI)

**Severity:** HIGH
**File:** `server/src/ro_damage_formulas.js` line 309
**Source:** rAthena `status.cpp`: `if (sd->status.weapon == W_KATAR) status->cri *= 2;`

**Problem:**
In RO Classic, when wielding a Katar, the **entire CRI value** is doubled — not just the LUK portion, but ALL sources (LUK + equipment bonuses + card bonuses).

**Current code:**
```javascript
const critical = 1 + Math.floor(luk * 0.3) + bonusCritical;
```

**Example impact:** Assassin with LUK 50, +5 CRI from equipment:
- Without katar: 1 + 15 + 5 = 21 CRI
- With katar (correct): (1 + 15 + 5) * 2 = **42 CRI**
- Current code gives: 21 CRI (no bonus at all)

**Fix:**
```javascript
let critical = 1 + Math.floor(luk * 0.3) + bonusCritical;
if (weaponType === 'katar') critical *= 2;
```

**Note:** The doubled CRI is NOT shown in the status window in real RO. For our game, we can choose whether to show it.

---

### B4: Poison React Mode B only applies flat damage (missing ATK component)

**Severity:** MEDIUM
**File:** `server/src/index.js` lines 21053-21056
**Source:** rAthena — Mode B auto-casts full Envenom Lv5

**Problem:**
Mode B auto-Envenom should be the full Envenom skill: **100% ATK + 75 flat poison damage**. Our code only applies flat 75 damage, missing the 100% ATK component.

**Fix:**
Replace flat damage with full `calculateSkillDamage()` call for 100% ATK + 75 flat bypass-DEF component.

---

### B5: Poison React Envenom counter limits wrong

**Severity:** MEDIUM
**File:** `server/src/index.js` line 12107
**Source:** rAthena `status.cpp`: `val2 = val1 / 2` (C integer division)

**Problem:**
| Lv | Our Code `floor((Lv+1)/2), Lv10=6` | rAthena `floor(Lv/2)` | Difference |
|----|--------------------------------------|----------------------|------------|
| 1 | 1 | 0 | +1 extra |
| 2 | 1 | 1 | OK |
| 3 | 2 | 1 | +1 extra |
| 4 | 2 | 2 | OK |
| 5 | 3 | 2 | +1 extra |
| 10 | 6 | 5 | +1 extra |

Our limits are consistently too generous, especially Lv1 (1 counter vs 0).

**Fix:**
```javascript
// Line 12107 — CHANGE FROM:
const prEnvenomLimit = learnedLevel === 10 ? 6 : Math.floor((learnedLevel + 1) / 2);
// TO:
const prEnvenomLimit = Math.floor(learnedLevel / 2);
```

---

### B6: Venom Dust AoE too large AND ground duration wrong

**Severity:** MEDIUM
**File:** `server/src/index.js` line 12167 (radius), line 12160 (duration)
**Source:** rAthena: 2x2 cell AoE, 60s fixed ground duration; `duration` field = poison STATUS duration

**Problem A — AoE:** Radius 75 (3x3) should be 50 (2x2).

**Problem B — Duration:** Our code uses `5*Lv` seconds as the GROUND CLOUD duration. But in RO Classic:
- `Duration1` (5*Lv seconds) = how long the **poison status** lasts on affected enemies
- `Duration2` (60 seconds fixed) = how long the **ground cloud** persists

At Lv1, our cloud disappears after 5 seconds instead of lasting 60 seconds. At Lv10, 50s instead of 60s.

**Fix:**
```javascript
// Line 12160 — CHANGE FROM:
const vdDuration = duration || (5 * learnedLevel * 1000);
// TO:
const vdDuration = 60000; // Ground cloud always persists 60 seconds (Duration2)
const vdPoisonDuration = 5 * learnedLevel * 1000; // Poison status duration (Duration1)

// Line 12167 — CHANGE FROM:
radius: 75,
// TO:
radius: 50, // 2x2 cells

// Pass vdPoisonDuration to ground effect for use in applyStatusEffect
```

Also update the ground tick at line 20012 to use per-level poison duration instead of hardcoded 60000.

---

### B7: Venom Splasher element should be WEAPON, not forced Poison

**Severity:** MEDIUM
**File:** `server/src/index.js` line 17537 and `ro_skill_data_2nd.js` line 207
**Source:** rAthena — Venom Splasher uses right-hand weapon element

**Problem:**
Our code forces `skillElement: 'poison'` on the detonation. In RO Classic, Venom Splasher uses the caster's **weapon element** (right hand). Despite the name, it is NOT forced Poison.

**Fix in detonation tick (line 17537):**
```javascript
// CHANGE FROM:
{ skillElement: 'poison', forceHit: true }
// TO:
{ skillElement: null, forceHit: true }  // null = use weapon element
```

**Fix in skill data (line 207):**
```javascript
// CHANGE element FROM:
element: 'poison'
// TO:
element: 'neutral'  // Uses weapon element at runtime
```

Also fix the mark broadcast at line 12261: change `element: 'poison'` to `element: 'neutral'`.

---

### B8: Venom Splasher damage should be SPLIT among targets

**Severity:** MEDIUM
**File:** `server/src/index.js` lines 17520-17527
**Source:** rAthena — Damage divided by target count in inner 3x3

**Problem:**
Our code applies FULL damage to every target in the 5x5 AoE. In RO Classic, the total damage is **divided by the number of targets** in the inner 3x3 area around the marked target. All targets in the 5x5 receive this split damage.

**Example:** 1000% ATK at Lv10, 4 enemies in 3x3 → each gets 250% ATK.

**Fix:**
```javascript
// After building vsTargets, count targets in inner 3x3 for split
const VS_INNER_RADIUS = 75; // 3x3 inner
const innerCount = vsTargets.filter(t => {
    const tdx = enemy.x - t.target.x, tdy = enemy.y - t.target.y;
    return Math.sqrt(tdx * tdx + tdy * tdy) <= VS_INNER_RADIUS;
}).length;
const splitDivisor = Math.max(1, innerCount);

// In damage calc loop, use: mark.effectVal / splitDivisor
```

---

### B9: Throw Venom Knife SP cost wrong (renewal value)

**Severity:** LOW-MEDIUM
**File:** `server/src/ro_skill_data_2nd.js` line 208
**Source:** rAthena pre-re: SP cost 15. The 35 value is from a 2019+ renewal rebalance.

**Problem:**
Our code uses `spCost: 35` (renewal). Pre-renewal is `spCost: 15`.

**Fix:**
```javascript
// Line 208 — CHANGE FROM:
spCost: 35
// TO:
spCost: 15
```

---

### B10: Venom Splasher cooldown off by 500ms

**Severity:** LOW
**File:** `server/src/ro_skill_data_2nd.js` line 207
**Source:** rAthena: `7.5 + 0.5*SkillLv` seconds = 8000/8500/.../12500ms

**Problem:**
Our formula `7000+500*(i+1)` produces 7500/8000/.../12000ms. Off by 500ms at every level.

| Lv | Our Code | Correct | Diff |
|----|----------|---------|------|
| 1 | 7500ms | 8000ms | -500ms |
| 5 | 9500ms | 10000ms | -500ms |
| 10 | 12000ms | 12500ms | -500ms |

**Fix:**
```javascript
// Line 207 — CHANGE cooldown FROM:
cooldown: 7000+500*(i+1)
// TO:
cooldown: 7500+500*(i+1)
```

---

## ISSUES — Should Fix

### I1: Throw Venom Knife poison procs with 0 Envenom

**Severity:** LOW
**File:** `server/src/index.js` line 12348

**Problem:** Poison chance `10 + 4*0 = 10%` even without Envenom learned. Should not proc at all.

**Fix:** Add `if (envenomLv > 0)` guard before the poison roll.

---

### I2: Throw Venom Knife — no inventory/weight update after consumption

**Severity:** MEDIUM
**File:** `server/src/index.js` after line 12303

**Fix:** Add `updatePlayerWeightCache()` and 0-quantity row cleanup after consuming knife.

---

### I3: Grimtooth melee/ranged classification missing

**Severity:** LOW-MEDIUM
**Source:** rAthena confirms Lv1-2 = melee (Safety Wall blocks), Lv3-5 = ranged (Pneuma blocks)

**Fix:** Pass `isRangedAttack: learnedLevel >= 3` in skill opts. Wire into Safety Wall / Pneuma blocking when those are active.

---

### I4: Enchant Poison not canceled on weapon switch

**Severity:** LOW
**Source:** All sources confirm switching weapons removes Enchant Poison and all endow buffs.

**Fix:** In equipment change handler, add endow buff removal when weapon slot changes.

---

### I5: Venom Splasher missing Poison React passive bonus

**Severity:** LOW-MEDIUM
**Source:** rAthena — Venom Splasher adds `30% * Poison_React_Level` to its damage.

**Problem:** If player has Poison React Lv10, VS should get +300% ATK bonus added to its skill ratio. Not implemented.

**Fix in detonation tick (near line 17534):**
```javascript
// Add Poison React passive bonus to Venom Splasher
let vsEffectVal = mark.effectVal;
if (caster && caster.learnedSkills) {
    const prLv = caster.learnedSkills[1104] || 0;
    if (prLv > 0) vsEffectVal += 30 * prLv;
}
```

---

## CROSS-SYSTEM NOTES

### Sonic Acceleration — Source Discrepancy

rAthena source shows `base_skillratio += base_skillratio / 10` (+10% to skill ratio). However, iRO Wiki Classic, RateMyServer, and all player-facing documentation consistently state **+50% Sonic Blow damage**. The iRO Wiki Sonic Acceleration page shows Lv10 SB without SA = 800%, implying with SA = 1200% (x1.5).

**Decision:** Keep +50% multiplicative (matching wiki consensus and player expectations). Our code at line 11823 (`sbEffectVal * 1.5`) is correct per this interpretation.

### EDP + Sonic Blow — Pre-Renewal Clarification

The original plan mentioned EDP halving the skill modifier. Deep research confirms this ONLY exists in `#ifdef RENEWAL` blocks. **Pre-renewal EDP applies a straight additive damage rate bonus with NO halving.** Since EDP (Assassin Cross) is deferred, no code change needed now, but the plan documentation should reflect this.

### Double Attack Katar Off-Hand Bonus

In pre-renewal, Double Attack provides a passive `(1 + 2*SkillLv)%` additional ATK when using Katars (not a proc, just flat bonus). This is a minor mechanic (+3% to +21% ATK) that we have NOT implemented. Adding as deferred since it's a small bonus.

---

## PER-SKILL AUDIT RESULTS

### Katar Mastery (1100) — PASS

| Check | Status | Notes |
|-------|--------|-------|
| +3 ATK per level | PASS | `bonusATK += kmLv * 3` |
| Katar-only | PASS | `wType === 'katar'` |
| Mastery damage stored | PASS | `katarMasteryATK` for pipeline |

---

### Sonic Blow (1101) — FAIL (B1)

| Check | Status | Notes |
|-------|--------|-------|
| Damage: 350-800% | **FAIL** | Uses 440-800% (wrong formula) |
| SP: 16-34 | PASS | `14+2*(i+1)` correct |
| ACD: 2000ms | PASS | Cannot be reduced (IgnoreStatus) |
| Katar required | PASS | |
| 8 visual hits, 1 calc | PASS | |
| Stun: (10+2*Lv)% | PASS | |
| Stun: 5s, boss immune | PASS | |
| Sonic Acceleration +50% dmg | PASS | Per wiki consensus |
| Sonic Acceleration +50% HIT | PASS | Multiplicative |
| Lex Aeterna | PASS | |

---

### Grimtooth (1102) — PASS with I3

| Check | Status | Notes |
|-------|--------|-------|
| Damage: 120-200% | PASS | `120+i*20` correct per rAthena |
| Hidden required | PASS | |
| Katar required | PASS | |
| Does NOT break hiding | PASS | |
| Range: 3-7 cells per level | PASS | `(2+Lv)*50` UE = 150-350 |
| 3x3 splash | PASS | radius=75 |
| SP: 3 flat | PASS | |
| Lv1-2 melee, Lv3+ ranged | **ISSUE** | I3 — not classified |

---

### Cloaking (1103) — PASS

| Check | Status | Notes |
|-------|--------|-------|
| SP: 15 activation | PASS | |
| SP drain intervals | PASS | [500,1000,...,9000] ms |
| Toggle on/off | PASS | |
| Break conditions | PASS | Attack, skills, SP=0, reveal |
| Replaces hiding | PASS | |
| Deaggros monsters | PASS | |
| isHidden flag | PASS | |

---

### Poison React (1104) — FAIL (B4, B5)

| Check | Status | Notes |
|-------|--------|-------|
| Duration: min(60, 15+5*Lv) s | PASS | |
| SP cost array | PASS | [25,30,35,40,45,50,55,60,45,45] |
| Counter ATK: 100+30*Lv | PASS | 130-400% |
| Envenom limits | **FAIL** | B5 — `floor((Lv+1)/2)` should be `floor(Lv/2)` |
| Mode A: poison counter | PASS | forceHit, 50% poison, one-shot |
| Mode B: auto-Envenom damage | **FAIL** | B4 — flat 75 only, missing 100% ATK |
| Mode B: 30% poison | PASS | |

---

### Venom Dust (1105) — FAIL (B6)

| Check | Status | Notes |
|-------|--------|-------|
| AoE: 2x2 | **FAIL** | radius=75 (3x3), should be 50 (2x2) |
| Ground duration: 60s fixed | **FAIL** | Uses 5*Lv (5-50s) as cloud duration |
| Poison status dur: 5*Lv s | PASS | But applied wrong (used as cloud dur) |
| Red Gemstone catalyst | PASS | |
| No direct damage | PASS | |
| Boss immune | PASS | |

---

### Sonic Acceleration (1106) — PASS

| Check | Status | Notes |
|-------|--------|-------|
| +50% SB damage | PASS | Per wiki consensus |
| +50% SB HIT | PASS | Multiplicative |
| Quest skill Lv1 | PASS | |

---

### Righthand Mastery (1107) — PASS

| Check | Status | Notes |
|-------|--------|-------|
| Base: 50%, recovery 10%/lv | PASS | Lv5 = 100% |
| Auto-attacks only | PASS | |

---

### Lefthand Mastery (1108) — PASS

| Check | Status | Notes |
|-------|--------|-------|
| Base: 30%, recovery 10%/lv | PASS | Lv5 = 80%, never 100% |
| ForceHit, noCrit | PASS | |

---

### Enchant Poison (1109) — FAIL (B2)

| Check | Status | Notes |
|-------|--------|-------|
| Duration: (15+15*Lv) s | PASS | 30-165s |
| Proc: (2.5+0.5*Lv)% | PASS | |
| Weapon → Poison | PASS | |
| Overwrites other endows | **FAIL** | B2 — wrong buff names |
| Auto-attack proc | PASS | |
| Canceled on weapon switch | **ISSUE** | I4 — not implemented |

---

### Venom Splasher (1110) — FAIL (B7, B8, B10, I5)

| Check | Status | Notes |
|-------|--------|-------|
| Target <= 75% HP | PASS | |
| Boss immune | PASS | |
| Damage: 550-1000% | PASS | `500+50*(i+1)` correct |
| Timer: 5-9.5s | PASS | `(4.5+0.5*Lv)` correct |
| Cooldown | **FAIL** | B10 — 500ms too low at all levels |
| Element | **FAIL** | B7 — forced poison, should be weapon |
| Damage split | **FAIL** | B8 — full damage to all, should split |
| PR passive bonus | **ISSUE** | I5 — +30%*PR_Lv not added |
| 5x5 AoE, forceHit | PASS | |
| Red Gemstone catalyst | PASS | |

---

### Throw Venom Knife (1111) — FAIL (B9)

| Check | Status | Notes |
|-------|--------|-------|
| Damage: 100% ATK | PASS | Pre-renewal correct |
| Range: 500 UE (10 cells) | PASS | |
| Poison: (10+4*EnvenomLv)% | PASS | But procs at 0 Envenom (I1) |
| Weapon element | PASS | |
| Venom Knife consumption | PASS | But no weight update (I2) |
| SP cost | **FAIL** | B9 — 35 (renewal), should be 15 |

---

## FIX IMPLEMENTATION PLAN (Priority Order)

### Priority 1: Skill Data Fixes (5 min, 4 lines)

| Fix | File | Change |
|-----|------|--------|
| B1: Sonic Blow damage | `ro_skill_data_2nd.js:198` | `effectValue: 440+i*40` → `350+i*50` |
| B9: TVK SP cost | `ro_skill_data_2nd.js:208` | `spCost: 35` → `spCost: 15` |
| B10: VS cooldown | `ro_skill_data_2nd.js:207` | `7000+500*(i+1)` → `7500+500*(i+1)` |
| B7: VS element | `ro_skill_data_2nd.js:207` | `element: 'poison'` → `element: 'neutral'` |

### Priority 2: Endow Name Fix (2 min, 2 lines)

| Fix | File | Change |
|-----|------|--------|
| B2: EP endow names | `index.js:12069` | `endow_blaze/etc` → `endow_fire/etc` |
| B2: Aspersio same | `index.js:12820` | Same fix |

### Priority 3: Katar CRI Bonus (5 min, 3 lines)

| Fix | File | Change |
|-----|------|--------|
| B3: Katar CRI *2 | `ro_damage_formulas.js:309` | Add `if (katar) critical *= 2` AFTER all CRI bonuses |

### Priority 4: Poison React Fixes (20 min, ~25 lines)

| Fix | File | Change |
|-----|------|--------|
| B5: Envenom limits | `index.js:12107` | `floor((Lv+1)/2)` → `floor(Lv/2)` |
| B4: Mode B full Envenom | `index.js:21053-21079` | Replace flat 75 with calculateSkillDamage + 75 flat |

### Priority 5: Venom Dust Duration + AoE (5 min, 5 lines)

| Fix | File | Change |
|-----|------|--------|
| B6: Cloud = 60s fixed | `index.js:12160` | `5*Lv*1000` → `60000` |
| B6: Radius | `index.js:12167,12178` | `75` → `50` |
| B6: Poison dur per-level | `index.js:20012` | Pass `5*Lv*1000` to applyStatusEffect |

### Priority 6: Venom Splasher Fixes (15 min, ~15 lines)

| Fix | File | Change |
|-----|------|--------|
| B7: Weapon element | `index.js:17537` | `'poison'` → `null` |
| B8: Damage split | `index.js:17520-17538` | Count inner 3x3 targets, divide effectVal |
| I5: PR passive bonus | `index.js:~17534` | Add `30*prLv` to effectVal |

### Priority 7: Minor Fixes (10 min, ~10 lines)

| Fix | File | Change |
|-----|------|--------|
| I1: TVK no-Envenom guard | `index.js:12348` | Add `envenomLv > 0` check |
| I2: TVK weight update | `index.js:after 12303` | Add `updatePlayerWeightCache()` |
| I4: EP weapon switch | `index.js:equip handler` | Remove endow buffs on weapon change |

### Priority 8: Deferred

| Item | Notes |
|------|-------|
| I3: Grimtooth melee/ranged | Wire when Safety Wall/Pneuma blocking is active |
| Cloaking wall adjacency | Server needs wall/collision data |
| Cloaking movement speed | Cosmetic; SP drain limits low levels |
| DA katar off-hand bonus | Minor (+3-21% ATK passive with Katar) |
| EDP pipeline | SinX transcendent skill not yet implemented |
| PvP Venom Dust | PvP system not implemented |

---

## SOURCE DISCREPANCIES NOTED

| Skill | Wiki Value | rAthena Value | Decision |
|-------|-----------|---------------|----------|
| Sonic Blow ATK% | 440-800% (iRO Wiki main page) | 350-800% (rAthena source) | **Use rAthena** — authoritative |
| Sonic Acceleration | +50% (wiki consensus) | +10% ratio (rAthena source) | **Use wiki** — matches player experience |
| PR Envenom limits | [1,1,2,2,3,3,4,4,5,5] (iRO Wiki) | [0,1,1,2,2,3,3,4,4,5] (rAthena) | **Use rAthena** — authoritative |
| Grimtooth ATK% | Flat 200% (iRO Wiki) | 120-200% (rAthena) | **Use rAthena** — scaling confirmed |
| TVK SP cost | 35 (some sources) | 15 (pre-renewal rAthena) | **Use 15** — pre-renewal only |
| EDP + SB halving | Some guides mention | Only in `#ifdef RENEWAL` | **No halving** — pre-renewal |

---

## ESTIMATED EFFORT

| Fix | Time | Lines Changed |
|-----|------|---------------|
| P1: Skill data (B1,B9,B10,B7-data) | 5 min | 4 lines |
| P2: Endow names (B2) | 2 min | 2 lines |
| P3: Katar CRI (B3) | 5 min | 3 lines |
| P4: Poison React (B4,B5) | 20 min | ~25 lines |
| P5: Venom Dust (B6) | 5 min | ~5 lines |
| P6: Venom Splasher (B7,B8,I5) | 15 min | ~15 lines |
| P7: Minor fixes (I1,I2,I4) | 10 min | ~10 lines |
| **Total** | **~62 min** | **~64 lines** |

---

## RESOLUTION LOG (2026-04-26)

End-to-end re-audit of all 12 Assassin skills against current code confirmed every BUG and ISSUE from the original audit was fixed (except I3, intentionally deferred per Priority 8). The re-audit also surfaced 2 new bugs not in the original report — both fixed in this pass.

### Confirmed fixed (audit items)

| Item | Where | Verified at |
|------|-------|-------------|
| B1 Sonic Blow damage 350-800% | `ro_skill_data_2nd.js:197` | `effectValue: 350+i*50` ✓ |
| B2 Enchant Poison endow names | `index.js:18909` (EP) + Aspersio handler | `endow_fire/water/wind/earth` ✓ |
| B3 Katar CRI doubled | `ro_damage_formulas.js:311` | `if (weaponType === 'katar') critical *= 2` ✓ (after all CRI bonuses) |
| B4 Poison React Mode B full Envenom | `index.js:33776-33782` | `calculateSkillDamage(... 100, ..., 'poison')` + 75 flat ✓ |
| B5 PR envenom limits `floor(Lv/2)` | `index.js:18950` | `Math.floor(learnedLevel / 2)` ✓ |
| B6 Venom Dust radius=50, ground=60s, poison=5×Lv | `index.js:19004,19012,19016` + tick `30967` | All three confirmed ✓ |
| B7 Venom Splasher uses weapon element | `ro_skill_data_2nd.js:206` + `index.js:27473` | `element: 'neutral'` + `skillElement: null` ✓ |
| B8 VS damage split by inner-3×3 count | `index.js:27457-27463` | `vsSplitDivisor = max(1, innerCount)` ✓ |
| B9 Throw Venom Knife SP=15 | `ro_skill_data_2nd.js:207` | `spCost: 15` ✓ |
| B10 VS cooldown 8000-12500ms | `ro_skill_data_2nd.js:206` | `cooldown: 7500+500*(i+1)` ✓ |
| I1 TVK Envenom guard | `index.js:19199` | `if (envenomLv > 0)` before poison roll ✓ |
| I2 TVK weight cache update | `index.js:19153` | `await updatePlayerWeightCache()` ✓ |
| I4 EP weapon switch removal | `inventory:equip` handler | Equip + unequip both clear endow buffs ✓ |
| I5 VS Poison React passive bonus | `index.js:27440-27443` | `vsEffectVal += 30 * prLv` ✓ |

### Intentionally deferred (audit Priority 8)

- **I3 Grimtooth melee/ranged classification** — Lv1-2 should be melee (Safety Wall blocks), Lv3-5 should be ranged (Pneuma blocks). Per the audit's Priority 8: "Wire when Safety Wall/Pneuma blocking is active". Current Grimtooth handler does not pass `isRanged` or check Safety Wall / Pneuma ground effects. Low impact.

### Newly-discovered bugs (FIXED 2026-04-26)

#### N1 — Sonic Acceleration HIT bonus silently dropped

**File:** `server/src/index.js:18674`

The Sonic Blow handler attempted to apply a +50% multiplicative HIT bonus from the Sonic Acceleration quest passive by setting `sbSkillOpts.hitBonus = 1.5`. However, `calculatePhysicalDamage` does NOT recognize a `hitBonus` field — it only accepts `skillHitBonus` (flat additive to HIT) and `hitRatePercent` (multiplicative % to hit rate). The assignment was silently dropped, so Sonic Accel only applied its +50% damage portion (line 18663) but the +50% HIT was a no-op.

**Fix:** Replaced with `sbSkillOpts.hitRatePercent = 50;` so `calculateHitRate` applies `hitRate * (100 + 50) / 100` per the formula at `ro_damage_formulas.js:406-407`.

#### N2 — Poison React Mode B can proc once at Lv1 (envenomLimit=0)

**File:** `server/src/index.js:33770`

At PR Lv1, `envenomLimit = floor(1/2) = 0`. The original Mode B logic ran the 50% proc roll FIRST, then incremented `envenomUsed` to 1 and checked `1 >= 0` → removed the buff. Net effect: Lv1 PR could trigger Mode B exactly once before being removed. rAthena spec: Mode B should NEVER trigger at Lv1.

**Fix:** Gated the proc roll behind `envenomCountersLeft > 0`:
```js
const envenomCountersLeft = (prBuff.envenomLimit || 0) - (prBuff.envenomUsed || 0);
if (envenomCountersLeft > 0 && Math.random() < 0.5 && enemy.health > 0) { ... }
```

Verified per-level: Lv1 = NO Mode B. Lv2-3 = 1 use. Lv4-5 = 2 uses. ... Lv10 = 5 uses.

### Files modified (this pass)

- `server/src/index.js` — Sonic Blow handler (N1 fix), Poison React Mode B handler (N2 fix)
- `docsNew/05_Development/Assassin_Skills_Audit_And_Fix_Plan.md` — header + this resolution log

### Verification

- ✅ `node --check server/src/index.js` passes
- ✅ Per-level PR Mode B counter test: Lv1 NO, Lv2-3 = 1, Lv4-5 = 2, ..., Lv10 = 5 (matches rAthena `floor(Lv/2)`)
- ✅ Sonic Accel HIT scaling: base 80% → 95% (capped); base 40% → 60%
