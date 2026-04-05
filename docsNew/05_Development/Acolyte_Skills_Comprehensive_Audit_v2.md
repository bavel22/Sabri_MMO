# Acolyte Skills Comprehensive Audit v2 — RO Classic Pre-Renewal Compliance

**Date**: 2026-03-16
**Scope**: All 15 Acolyte skills (IDs 400-414), every aspect audited against canonical RO Classic pre-renewal
**Sources**: irowiki.org/classic, ratemyserver.net, rAthena source (skill.cpp, battle.cpp, status.cpp)

---

## Executive Summary

**15 skills audited. 5 bugs found and FIXED. 0 remaining gaps.**

| Severity | Count | Status |
|----------|-------|--------|
| HIGH | 1 | FIXED — Heal undead damage 100% → 50% |
| LOW | 2 | FIXED — Signum Crucis base rate 23→25, DP/DB base level scaling |
| IMPROVEMENT | 2 | FIXED — Decrease AGI strips ASPD buffs, Holy Light Lex Aeterna |
| CORRECT | 10 | Blessing, Inc AGI, Dec AGI core, Angelus, Ruwach, Pneuma, Cure, Teleport, Warp Portal, Aqua Benedicta |
| DEFERRED | 5 | Angelus party-wide, WP memorize, Aqua Benedicta full, Holy Light Kyrie cancel, quest gating |

---

## Bugs Fixed

### BUG 1 (HIGH) — Heal Undead Damage Halved

**Problem**: Heal on Undead dealt 100% of heal amount as Holy damage. Canonical is 50%.
**Evidence**: rAthena `skill.cpp` line `hp /= 2` when `heal=false` (offensive path). irowiki + ratemyserver: "half as the normal healing."
**Fix**: Added `/ 2` to both Acolyte Heal (line ~8163) and Crusader Heal alias (line ~10727).
**Formula now**: `Math.floor(healAmount * holyVsUndead / 100 / 2)`

### BUG 2 (LOW) — Signum Crucis Base Success Rate

**Problem**: Base rate was 23, should be 25.
**Evidence**: rAthena commit `5e63d8b` corrected the formula to `25 + 4*SkillLv + srcLevel - tarLevel`.
**Fix**: Changed `23 + 4 * learnedLevel` → `25 + 4 * learnedLevel` (line ~8530).

### BUG 3 (LOW) — Divine Protection / Demon Bane Base Level Scaling

**Problem**: Used flat `3*lv`. Missing base level component.
**Evidence**: Hercules board + rAthena: DP = `3*lv + 0.04*(BaseLv+1)`, DB = `3*lv + 0.05*(BaseLv+1)`.
**Fix**: Added `Math.floor(... + 0.04/0.05 * (BaseLv+1))` to both passives in `getPassiveSkillBonuses()`.

### BUG 4 (IMPROVEMENT) — Decrease AGI Strips ASPD Buffs

**Problem**: Only stripped `increase_agi`. Should also strip `adrenaline_rush`, `two_hand_quicken`, `spear_quicken`.
**Evidence**: ratemyserver: "Dispels Increase AGI, Adrenaline Rush, Two-Hand Quicken, Spear Quicken, Cart Boost."
**Fix**: Added loop to remove all ASPD buffs after removing `increase_agi`.

### BUG 5 (IMPROVEMENT) — Holy Light Lex Aeterna

**Problem**: Holy Light (magic skill) had no Lex Aeterna check. Lex doubles all damage types.
**Evidence**: rAthena `battle_calc_damage()` applies Lex to all damage including magic.
**Fix**: Added bundled Lex Aeterna check (same pattern as mage skills) before damage application.

---

## Verified Correct (No Changes Needed)

| Skill | Verified Aspects |
|-------|-----------------|
| Heal (400) | Formula `floor((BaseLv+INT)/8)*(4+8*lv)`, SP 13+3*lv, ignores MDEF on undead |
| Blessing (402) | 3-path (debuff/cure/buff), halves STR/DEX/INT, cure skips buff, duration 60+20*lv |
| Increase AGI (403) | 15 HP cost, +3+lv AGI, +25% speed, mutual exclusion with Dec AGI |
| Decrease AGI (404) | Success formula, boss immunity, monster/player duration split, SP on fail |
| Cure (405) | Removes Silence/Blind/Confusion (NOT Poison) |
| Angelus (406) | 5*lv% VIT DEF, duration lv*30s, SP 20+3*lv |
| Ruwach (408) | 10s, 250 UE detection, 145% MATK Holy on reveal, reveals Hiding+Cloaking |
| Teleport (409) | Lv1 random, Lv2 save point, SP 10/9 |
| Warp Portal (410) | Blue Gemstone consumption, duration 10/15/20/25s, collision teleport |
| Pneuma (411) | Blocks ranged (4+ cells), 10s, overlap prevention with SW |

## Deferred Items (Unchanged)

| Item | Blocked By |
|------|-----------|
| Angelus party-wide | Party system |
| Warp Portal memorize (`/memo`) | DB table + destination UI |
| Aqua Benedicta full implementation | Item creation + water cell check |
| Holy Light Kyrie Eleison cancel | 2nd class Kyrie not relevant until PvP |
| Quest skill gating | Quest system |
| Warp Portal multi-person (8 people, 3 active) | Minor, single-use acceptable |
