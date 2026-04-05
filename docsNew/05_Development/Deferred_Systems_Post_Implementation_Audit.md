# Deferred Systems — Post-Implementation Audit

**Date**: 2026-03-18
**Scope**: Verification of all 8 implemented phases against RO Classic pre-renewal
**Method**: 4 parallel audit agents searched actual code and compared against rAthena/iRO Wiki specs
**Result**: 38 issues found (5 critical, 6 high, 12 medium, 15 low)
**Remediation**: All 38 issues FIXED (2026-03-18). Zero remaining.

---

## Critical Issues (5) — Must Fix

### C1. `sc_start` handler is a non-functional stub
**Location**: `index.js` lines ~20776-20779
**Impact**: ALL consumable buffs broken — ASPD potions (645/656/657), stat foods (12041-12100), buff scrolls
**Fix**: Replace stub with full handler that parses `eff.status`, applies buffs via `applyBuff()`, handles class/level restrictions, strongest-wins hierarchy, and stat food mutual exclusion

### C3. `itemskill` handler only recognizes MC_IDENTIFY
**Location**: `index.js` lines ~20781-20784
**Impact**: Elemental Converters (ITEM_ENCHANTARMS), spell scrolls (14512-14594) all non-functional
**Fix**: Add handlers for `ITEM_ENCHANTARMS` (apply endow buff) and spell scroll skills (cast using player stats at scroll level)

### B4+B5. Ensemble buff bonuses (Drum/Nibelungen/Mr. Kim) are dead code
**Location**: `ro_buff_system.js` sets `mods.bonusATK`/`bonusDEF`/`expBonusPct` but `getCombinedModifiers()` in `index.js` never passes these through to the damage/EXP pipelines
**Impact**: Drum ATK/DEF, Nibelungen ATK, Mr. Kim EXP bonus have zero gameplay effect
**Fix**: In `getCombinedModifiers()`, surface `bonusATK`, `bonusDEF`, `expBonusPct` into the combined output. Wire `bonusATK` into damage pipeline, `bonusDEF` into defense pipeline, `expBonusPct` into EXP distribution

### B6. Siegfried element resist is dead code (wrong formula + wrong data type)
**Location**: `index.js` line ~1388 uses `30+level*10` (wrong, should be `55+5*level`); buff sets flat number but damage pipeline reads per-element object
**Impact**: Siegfried element resistance has zero effect
**Fix**: Use `effectValue` from skill data (which correctly stores `55+5*lv`), and set per-element resist object for all 9 non-neutral elements

### G1. `homDerived.int` is undefined — Healing Hands/Chaotic Blessings produce NaN
**Location**: `index.js` lines ~20104, ~20338; `ro_homunculus_data.js` `calculateHomunculusStats()` returns object without `int` key
**Impact**: Both heal skills produce NaN (zero healing)
**Fix**: Add `int: int` to the return object of `calculateHomunculusStats()`

### E1. `UPDATE ... LIMIT 1` is invalid PostgreSQL syntax
**Location**: `index.js` lines ~14882-14884 (Abracadabra gem consumption)
**Impact**: Abracadabra crashes with SQL syntax error when consuming gems
**Fix**: Remove `LIMIT 1` or use subquery pattern `WHERE ctid = (SELECT ctid FROM ... LIMIT 1)`

---

## High Issues (6) — Should Fix

### C2. ASPD potion buff type missing from `ro_buff_system.js`
**Fix**: Add `aspd_potion` case to `getBuffModifiers()` returning `aspdPotionReduction` value. Wire into `calculateASPD()`.

### B7. Siegfried status resist not surfaced
**Fix**: Add `statusResist` to `getCombinedModifiers()` output. Check in `applyStatusEffect()`.

### B2. Lullaby sleep chance is 10000% (guaranteed, no resistance)
**Fix**: Use reasonable base chance (e.g., 30-50%) and pass through standard status resistance formula.

### E2. Abracadabra SA_DEATH awards EXP/drops (should not)
**Fix**: Set enemy's baseExp/jobExp to 0 before calling `processEnemyDeathFromSkill()`, then restore.

### F1. Redemptio has no EXP sufficiency check
**Fix**: Check `currentExp >= expPenalty` before executing. If insufficient, emit `skill:error` and return.

### G3. Flitting ATK bonus off by 5% per level
**Fix**: Change from `100 + skillLevel * 5` to lookup array `[110, 115, 120, 125, 130]`.

---

## Medium Issues (12)

| ID | System | Issue | Fix |
|----|--------|-------|-----|
| A3 | Magic Rod | Cooldown 500ms should be 0 | Set `cooldown: 0` in skill data |
| B1 | Ensemble | Aftermath missing movement penalty | Add speed reduction to aftermath buff |
| B3 | Ensemble | Lullaby only affects enemies | Add `connectedPlayers` iteration for PvP |
| B8 | Ensemble | Loki's Veil doesn't block monster skills | Add check in monster skill execution |
| B9 | Ensemble | Into the Abyss gem bypass incomplete | Add `isInAbyssAoE` check in individual gem consumption paths |
| B11 | Ensemble | Eternal Chaos prereq wrong | Change prereq from Loki's Veil to Drum Lv5 |
| B12 | Ensemble | MWM missing Bard-side skill | Add Bard variant skill ID |
| D1 | Weapon Repair | No equipment update to repaired target | Emit `stats:full_update` or equipment refresh to target |
| E3 | Abracadabra | SA_MONOCELL incomplete transformation | Add DEF/MDEF/element/speed to Poring transform |
| G4 | Homunculus | Bulwark doesn't buff homunculus itself | Apply VIT+DEF to homunculus in addition to owner |
| G7 | Homunculus | Caprice element modifier cosmetic only | Apply element damage table against target element |
| G8 | Homunculus | Moonlight flat DEF instead of % | Use `damage * (100 - hardDef) / 100` like auto-attack |

## Low Issues (15) — ALL FIXED (2026-03-18)

| ID | System | Issue | Status |
|----|--------|-------|--------|
| A1 | Magic Rod | Napalm Beat per-target Magic Rod check in AoE splash | FIXED (2026-03-18) |
| A2 | Magic Rod | Fire Ball per-target Magic Rod check in AoE splash | FIXED (2026-03-18) |
| B10 | Ensemble | MWM movement barrier implemented | FIXED (2026-03-18) |
| B13 | Ensemble | Eternal Chaos SP cost corrected to rAthena 20 | FIXED (2026-03-18) |
| C7 | Consumables | Death clearing verified and working | FIXED (2026-03-18) |
| C8 | Consumables | Stat food `value` field now read correctly | FIXED (2026-03-18) |
| E4 | Abracadabra | SA_MONOCELL Lv6 probability corrected | FIXED (2026-03-18) |
| E5 | Abracadabra | Regular skill pool expanded from 6 to 124 skills | FIXED (2026-03-18) |
| F2 | Redemptio | Now uninterruptible (per-skill flag) | FIXED (2026-03-18) |
| F3 | Redemptio | EXP restoration for revived members implemented | FIXED (2026-03-18) |
| F4 | Redemptio | DB persistence for resurrection added | FIXED (2026-03-18) |
| G5 | Homunculus | Accelerated Flight dead code removed | FIXED (2026-03-18) |
| G6 | Homunculus | Urgent Escape now buffs both owner AND homunculus | FIXED (2026-03-18) |
| G9+G10 | Homunculus | HP/SP regeneration system implemented (10s tick) | FIXED (2026-03-18) |
| G11 | Homunculus | Stone of Sage item ID corrected to 7321 | FIXED (2026-03-18) |

---

**Status: ALL 38 ISSUES RESOLVED — zero remaining**
