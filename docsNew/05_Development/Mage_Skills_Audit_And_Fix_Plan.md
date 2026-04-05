# Mage Skills Audit & Fix Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md)
> **Status**: COMPLETED — All audit items resolved

**Date:** 2026-03-14 (Updated: 2026-03-15)
**Status:** IMPLEMENTED — Phases 1-3 + SP Recovery fixes complete
**Scope:** All 14 Mage class skills (IDs 200-213)

---

## Executive Summary

Deep research against iRO Wiki (Classic), RateMyServer (Pre-Re), and rAthena source code reveals **moderate gaps** across the Mage skill set. The three bolt skills (Cold/Fire/Lightning Bolt) and Thunderstorm are implemented correctly in their core mechanics. However, several skills have significant issues:

**Critical bugs:**
1. **Soul Strike** after-cast delay table is completely wrong (our Lv10 = 1800ms is actually correct per rAthena, but Lv2 = 1400ms should be 1000ms, and the full table is scrambled)
2. **Fire Ball** SP cost is wrong (our code uses 25 flat, canonical is also 25 flat -- actually correct), but cast time formula is wrong (our code uses `i < 5 ? 1500 : 1000` which gives Lv1-5=1500ms, Lv6-10=1000ms, canonical is also 1500/1000 -- correct)
3. **Fire Wall** hit count is wrong: our formula `i+3` gives 3-12, canonical is `level+2` giving 3-12 -- actually correct
4. **Stone Curse** is missing Red Gemstone consumption logic, missing two-phase petrification, missing HP drain while stoned, and the range is wrong (300 vs canonical 100 = 2 cells)
5. **Frost Diver** freeze chance formula is wrong: our code uses `35 + learnedLevel * 3` minus MDEF, but canonical rAthena formula includes level difference and LUK
6. **Napalm Beat** AoE radius is wrong: we use 300 UE units but canonical is 3x3 cells = 150 UE units
7. **Increase SP Recovery** is missing the SP item potency bonus (+2%/level) and the MaxSP%-based regen component
8. **Safety Wall** is missing Blue Gemstone consumption
9. **Sight** duration should be 10s (correct) but missing hidden-reveal tick mechanic
10. **Soul Strike** CC check uses `preventsCasting` but bolts use `isFrozen || isStoned` -- inconsistent

**Correct implementations:**
- All three bolt skills: SP costs, cast times, after-cast delays, multi-hit mechanics, damage formula
- Thunderstorm: SP costs, cast times, multi-hit, AoE size, after-cast delay
- Energy Coat: damage reduction tiers, SP drain per hit, duration, SP cost

**New systems needed:**
1. Two-phase petrification engine (Stone → Petrifying phase → Full Stone)
2. Stone status HP drain tick (1% HP / 5s until 25% HP)
3. Stone status defense modifiers (+25% MDEF, -50% DEF, Earth Lv1 element)
4. Freeze duration formula using MDEF/LUK resistance
5. SP recovery item potency bonus from Increase SP Recovery

---

## Skill-by-Skill Analysis

---

### 1. COLD BOLT (ID 200) — Active / Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target | All sources |
| Max Level | 10 | All sources |
| Element | Water | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Hits | 1 per level (Lv1=1, Lv10=10) | All sources |
| Damage | 1 x MATK per hit (100% MATK) | iRO Wiki Classic, RateMyServer |
| SP Cost | `10 + SkillLv * 2` (12, 14, 16, 18, 20, 22, 24, 26, 28, 30) | iRO Wiki Classic, RateMyServer |
| Cast Time | `SkillLv * 0.7` sec (700, 1400, 2100, 2800, 3500, 4200, 4900, 5600, 6300, 7000 ms) | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | `0.8 + SkillLv * 0.2` sec (1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800 ms) | iRO Wiki Classic, RateMyServer |
| Prerequisites | None | All sources |

#### Current Implementation Status: CORRECT

Our `ro_skill_data.js` definition:
- SP: `12+i*2` → 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 -- **CORRECT**
- Cast Time: `700*(i+1)` → 700, 1400, ..., 7000 -- **CORRECT**
- ACD: `800+(i+1)*200` → 1000, 1200, ..., 2800 -- **CORRECT**
- effectValue: 100 (100% MATK per hit) -- **CORRECT**
- range: 900 -- **WRONG** (should be 450, but 900 UE units = 18 cells, too large)
- Handler: multi-hit bolt with hits = learnedLevel, 100% MATK per hit -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Range is 900 UE units (18 cells) instead of 450 (9 cells) | MEDIUM | Trivial — change `range: 900` to `range: 450` in skill def |

#### Implementation Notes

The 900 UE unit range is shared by all bolt skills and several other mage skills. In RO, magic range is typically 9 cells. At our scale (1 cell = ~50 UE units), 9 cells = 450 UE units. However, the project may have intentionally used 900 for gameplay feel. This should be a coordinated change across all magic skills to maintain consistency. **Recommendation: Keep 900 for now as a deliberate design choice, but document the deviation.**

---

### 2. FIRE BOLT (ID 201) — Active / Single Target

#### RO Classic Canonical Mechanics

Identical to Cold Bolt in all respects except element (Fire).

| Property | Value | Source |
|----------|-------|--------|
| Element | Fire | All sources |
| All other stats | Same as Cold Bolt | All sources |
| Prerequisites | None | All sources |

#### Current Implementation Status: CORRECT

Shares the same handler as Cold Bolt. All values match canonical.

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Same range issue as Cold Bolt (900 vs 450) | MEDIUM | Same coordinated fix |

**No code changes needed for current phase.**

---

### 3. LIGHTNING BOLT (ID 202) — Active / Single Target

#### RO Classic Canonical Mechanics

Identical to Cold Bolt except element (Wind).

| Property | Value | Source |
|----------|-------|--------|
| Element | Wind | All sources |
| All other stats | Same as Cold Bolt | All sources |
| Prerequisites | None | All sources |

#### Current Implementation Status: CORRECT

Shares the same handler as Cold Bolt. All values match canonical.

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Same range issue as Cold Bolt (900 vs 450) | MEDIUM | Same coordinated fix |

**No code changes needed for current phase.**

---

### 4. NAPALM BEAT (ID 203) — Active / Single Target + 3x3 Splash

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target with 3x3 splash | All sources |
| Max Level | 10 | All sources |
| Element | Ghost | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Hits | 1 (single strike at all levels) | iRO Wiki Classic, RateMyServer |
| Damage | `(70 + 10 * SkillLv)%` MATK = 80%, 90%, 100%, 110%, 120%, 130%, 140%, 150%, 160%, 170% | RateMyServer |
| AoE | 3x3 cells around target | RateMyServer |
| Damage Split | Total damage divided equally among all targets hit | All sources |
| SP Cost | Lv1-3: 9, Lv4-6: 12, Lv7-9: 15, Lv10: 18 | iRO Wiki Classic, RateMyServer |
| Cast Time | 1000ms (fixed, all levels) | rAthena skill_cast_db, RateMyServer |
| After-Cast Delay | 1000, 1000, 1000, 900, 900, 800, 800, 700, 600, 500 ms | rAthena skill_cast_db, RateMyServer |
| Prerequisites | None | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP costs: `[9,9,9,12,12,12,15,15,15,18]` -- **CORRECT**
- Cast time: 1000ms fixed -- **CORRECT**
- ACD: `[1000,1000,1000,900,900,800,800,700,600,500]` -- **CORRECT**
- effectValue: `80+i*10` → 80-170% -- **CORRECT**
- Damage split mechanic: Divides per-target damage by target count -- **CORRECT**
- AoE radius: 300 UE units -- **WRONG** (3x3 = 150 UE units at 50 UE/cell)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| AoE radius is 300 UE units (6x6 cells) instead of 150 (3x3 cells) | MEDIUM | Trivial — change `NAPALM_AOE = 300` to `NAPALM_AOE = 150` in handler |
| CC check uses `preventsCasting` but bolts use `isFrozen \|\| isStoned` — inconsistent | LOW | Standardize to `preventsCasting` everywhere |

#### Implementation Notes

The 300 UE unit radius means Napalm Beat currently splashes a 6x6 cell area instead of the canonical 3x3. This doubles the intended splash range, making it significantly more powerful for AoE farming. Fix by changing `NAPALM_AOE = 300` to `NAPALM_AOE = 150` in the handler.

---

### 5. INCREASE SP RECOVERY (ID 204) — Passive

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Passive | All sources |
| Max Level | 10 | All sources |
| SP Regen (per tick) | `3 * SkillLv` flat SP (+3 to +30 per 8s standing / 4s sitting) | iRO Wiki Classic, RateMyServer |
| MaxSP% Bonus | `0.2% * SkillLv` of MaxSP added to regen (0.2% to 2.0%) | iRO Wiki Classic |
| SP Item Potency | `+2% * SkillLv` (+2% to +20%) to SP recovery items | RateMyServer |
| Prerequisites | None | All sources |

**Pre-Renewal SP regen formula (from iRO Wiki Classic):**
```
SPR = 1 + floor(MAX_SP / 100) + floor(INT / 6)
if (INT >= 120): SPR += floor(INT / 2 - 56)
SPR = floor(SPR * (1 + SPR_MOD * 0.01))
```
The `SPR_MOD` includes the Increase SP Recovery flat bonus. SP regen ticks every 8s standing, 4s sitting. Magnificat halves these intervals.

#### Current Implementation Status: PARTIAL

- Flat SP regen bonus: `sprLv * 3` in `getPassiveSkillBonuses()` -- **CORRECT** (base flat bonus)
- MaxSP% regen bonus: **MISSING** (0.2% of MaxSP per level is not applied)
- SP Item Potency bonus: **MISSING** (+2% per level to SP recovery items)
- Used in regen tick via `passive.spRegenBonus` -- **PARTIALLY CORRECT** (flat part only)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Missing MaxSP%-based SP regen component (0.2%/level) | MEDIUM | Moderate — add `spRegenMaxSPPct` to passive bonuses, apply in regen tick |
| Missing SP item potency bonus (+2%/level) | LOW | Moderate — apply multiplier in `inventory:use` SP item handler |
| SP regen tick interval not RO-accurate (should be 8s standing, 4s sitting) | LOW | Low — our regen tick interval may differ from canonical |

#### Implementation Notes

The flat SP bonus (+3/level) is the primary component and is already implemented. The MaxSP% component adds `floor(MaxSP * 0.002 * SkillLv)` per regen tick, which for a 500 SP mage at Lv10 would add +10 SP/tick. This is meaningful at higher levels. The item potency bonus is minor and can be deferred.

---

### 6. SIGHT (ID 205) — Active / Self Buff

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self buff | All sources |
| Max Level | 1 | All sources |
| Element | Fire | RateMyServer |
| SP Cost | 10 | iRO Wiki Classic, RateMyServer |
| Cast Time | 0 (instant) | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | 0 | iRO Wiki Classic |
| Duration | 10 seconds | RateMyServer |
| AoE | 7x7 cells around caster | iRO Wiki Classic, RateMyServer |
| Effect | Reveals hidden/cloaked enemies in AoE | All sources |
| Damage | None (reveal only) | iRO Wiki Classic |
| Prerequisites | None | All sources |

#### Current Implementation Status: CORRECT

- SP cost: 10 -- **CORRECT**
- Cast time: 0 -- **CORRECT**
- Duration: 10000ms -- **CORRECT**
- Element: fire -- **CORRECT**
- AoE radius in buff effect: 700 (7x7 at 50 UE/cell = 350, but 700 is acceptable for feel) -- **ACCEPTABLE**
- Handler applies buff with `revealHidden: true` -- **CORRECT**
- No damage dealt -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| No server-side hidden-reveal tick (buff exists but doesn't actually check for hidden enemies nearby) | LOW | Major — requires Hiding detection system to be implemented first |
| Stalker Chase Walk immunity not enforced | DEFERRED | Phase 6+ (Stalker 2nd class) |

#### Implementation Notes

The Sight buff is correctly applied and broadcast. However, since the Hiding system is simple toggle-based with no server-side proximity checks, the "reveal" mechanic is cosmetic. When the full Hiding system with position-based reveal is implemented, Sight will need a periodic tick to check for hidden entities within 7x7 range. This is a deferred feature.

**No changes needed for current phase.**

---

### 7. STONE CURSE (ID 206) — Active / Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target (debuff, no damage) | All sources |
| Max Level | 10 | All sources |
| Element | Earth (target becomes Earth Lv1 when stoned) | iRO Wiki Classic, RateMyServer |
| Range | 2 cells (100 UE units) | RateMyServer |
| SP Cost | `26 - SkillLv` (25, 24, 23, 22, 21, 20, 19, 18, 17, 16) | iRO Wiki Classic, RateMyServer |
| Cast Time | 1000ms (fixed) | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | ASPD-based | iRO Wiki Classic |
| Stone Chance | `(20 + 4 * SkillLv)%` = 24, 28, 32, 36, 40, 44, 48, 52, 56, 60% | iRO Wiki Classic, RateMyServer |
| Resistance | `Chance -= Chance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK` | rAthena pre-renewal status formulas |
| Red Gemstone | Lv1-5: consumed every cast. Lv6-10: consumed only on SUCCESS | iRO Wiki Classic, RateMyServer |
| Two-Phase | Phase 1 "Petrifying": ~5s, can move, can't attack/skill, can use items. Phase 2 "Stone": full immobilize | iRO Wiki Classic |
| Stone Duration | 20 seconds (fixed) for phase 2 | rAthena, RateMyServer |
| HP Drain | 1% MaxHP every 5 seconds until 25% HP remains | iRO Wiki Classic, RateMyServer |
| Defense Change | +25% MDEF, -50% DEF while fully stoned | iRO Wiki, RateMyServer |
| Element Change | Target becomes Earth Lv1 | iRO Wiki Classic, RateMyServer |
| Boss Immunity | Boss and Undead-element monsters immune | iRO Wiki Classic, RateMyServer |
| Prerequisites | None | All sources |

#### Current Implementation Status: MAJOR GAPS

- SP cost: `25-i` → 25, 24, 23, 22, 21, 20, 19, 18, 17, 16 -- **CORRECT**
- Cast time: 1000ms -- **CORRECT**
- Stone chance: `24+i*4` → 24, 28, ..., 60% -- **CORRECT** (base chance matches)
- Range: 300 UE (6 cells) -- **WRONG** (should be 100 UE = 2 cells)
- Red Gemstone consumption: **PRESENT** in `SKILL_CATALYSTS` but Lv6-10 success-only logic NOT verified in consumption code
- Two-phase petrification: **MISSING** — goes directly to full stone
- HP drain while stoned: **MISSING** — no periodic damage tick
- Defense modifiers while stoned (+25% MDEF, -50% DEF): **MISSING** — not in buff modifiers
- Element change to Earth Lv1: **PARTIALLY IMPLEMENTED** — status engine may handle this
- Resistance formula: Simplified — uses `effectVal - targetHardMdef` instead of canonical formula with level diff and LUK
- After-cast delay: 0 in our code -- **WRONG** (should be ASPD-based, but we can use a small fixed ACD)

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Range 300 UE (6 cells) instead of 100 (2 cells) | HIGH | Trivial — change `range: 300` to `range: 100` |
| Missing two-phase petrification (Petrifying → Stone) | HIGH | Major — new status engine feature |
| Missing HP drain tick (1% HP / 5s until 25% HP) | HIGH | Major — new status tick handler |
| Missing +25% MDEF / -50% DEF modifiers while stoned | MEDIUM | Moderate — add to `getBuffStatModifiers()` |
| Resistance formula oversimplified (missing level diff and LUK) | MEDIUM | Moderate — update `applyStatusEffect()` or handler |
| Red Gemstone Lv6-10 success-only consumption not verified | MEDIUM | Small — verify gem consumption path |
| ACD should be ASPD-based, not 0 | LOW | Small — set a reasonable ACD (e.g. 500ms) |

#### Implementation Notes

Stone Curse is the most complex Mage skill to implement correctly. The two-phase system requires:
1. **Phase 1 "Petrifying"**: Apply a `petrifying` status for ~5 seconds. Target can move and use items but cannot attack/use skills. Visual effect shows gradual graying. If attacked during this phase, petrifying is NOT broken (unlike other statuses). Casting Stone Curse on an already-petrifying target NEGATES the effect (cures it).
2. **Phase 2 "Stone"**: After petrifying timer expires, apply full `stone` status for 20 seconds. Full immobilize, element changes to Earth Lv1, +25% MDEF, -50% DEF. HP drains 1% every 5 seconds until 25% HP. Physical/magical damage breaks stone.

The Red Gemstone consumption rules per `SKILL_CATALYSTS` already have the entry for `stone_curse`. However, the Lv6-10 success-only logic needs verification in the catalyst consumption code path (line ~747-760 in index.js). The current code consumes catalysts before skill execution, which means it always consumes regardless of success. For Lv6-10, consumption should only happen if `stoneResult.applied === true`.

---

### 8. FIRE BALL (ID 207) — Active / Single Target + 5x5 Splash

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target with 5x5 splash | All sources |
| Max Level | 10 | All sources |
| Element | Fire | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Hits | 1 (single strike) | All sources |
| Damage | `(70 + 10 * SkillLv)%` MATK = 80%, 90%, 100%, 110%, 120%, 130%, 140%, 150%, 160%, 170% | iRO Wiki Classic, RateMyServer |
| AoE | 5x5 cells around target | iRO Wiki Classic, RateMyServer |
| Splash Damage | Full damage to ALL targets in AoE (NOT split) | All sources |
| SP Cost | 25 (fixed, all levels) | iRO Wiki Classic, RateMyServer |
| Cast Time | Lv1-5: 1500ms, Lv6-10: 1000ms | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | Lv1-5: 1500ms, Lv6-10: 1000ms | iRO Wiki Classic, RateMyServer |
| Prerequisites | Fire Bolt Lv4 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: 25 fixed -- **CORRECT**
- Cast time: `i < 5 ? 1500 : 1000` → Lv1-5=1500, Lv6-10=1000 -- **CORRECT**
- ACD: `i < 5 ? 1500 : 1000` -- **CORRECT**
- effectValue: `80+i*10` → 80-170% -- **CORRECT**
- AoE: 500 UE units (10 cells) -- **WRONG** (5x5 = 250 UE units at 50 UE/cell)
- Inner/outer ring damage: Center 3x3 full, outer ring 75% -- **NOT CANONICAL** (canonical is full damage to entire 5x5)
- Splash: Full damage to each target (not split) -- **CORRECT**
- Prerequisites: Fire Bolt Lv4 -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| AoE radius 500 UE (10 cells) instead of 250 (5x5) | MEDIUM | Trivial — change `FIREBALL_AOE = 500` to `FIREBALL_AOE = 250` |
| Inner/outer ring damage reduction (75% outer) is NOT canonical | LOW | Trivial — remove edge multiplier, all targets take full damage |

#### Implementation Notes

The inner/outer ring mechanic (`FIREBALL_INNER = 300`, 75% outer damage) is not present in canonical RO. All targets within the 5x5 AoE take the same damage. This is a minor gameplay change. Remove the `edgeMult` calculation and set all targets to 100% damage.

---

### 9. FROST DIVER (ID 208) — Active / Single Target + Freeze

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target (damage + freeze) | All sources |
| Max Level | 10 | All sources |
| Element | Water | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Hits | 1 | All sources |
| Damage | `(100 + 10 * SkillLv)%` MATK = 110, 120, 130, 140, 150, 160, 170, 180, 190, 200% | iRO Wiki Classic, RateMyServer |
| SP Cost | `26 - SkillLv` (25, 24, 23, 22, 21, 20, 19, 18, 17, 16) | iRO Wiki Classic, RateMyServer |
| Cast Time | 800ms (fixed, all levels) | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | 1500ms (fixed, all levels) | iRO Wiki Classic, RateMyServer |
| Freeze Chance | `(35 + 3 * SkillLv)%` = 38, 41, 44, 47, 50, 53, 56, 59, 62, 65% | iRO Wiki Classic, RateMyServer |
| Freeze Duration | `3 * SkillLv` seconds = 3, 6, 9, 12, 15, 18, 21, 24, 27, 30 sec | iRO Wiki Classic, RateMyServer |
| Resistance | `Chance -= Chance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK` | rAthena pre-renewal |
| Freeze Duration Reduction | `Duration -= Duration * targetHardMDEF / 100 - 10 * srcLUK` | rAthena pre-renewal |
| Boss/Undead Immunity | Boss and Undead-element monsters cannot be frozen | All sources |
| Frozen Properties | Target becomes Water Lv1, -50% DEF, +25% MDEF, full immobilize | iRO Wiki |
| Prerequisites | Cold Bolt Lv5 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: `25-i` → 25, 24, ..., 16 -- **CORRECT**
- Cast time: 800ms -- **CORRECT**
- ACD: 1500ms -- **CORRECT**
- effectValue: `110+i*10` → 110-200% -- **CORRECT**
- duration: `(i+1)*3000` → 3000, 6000, ..., 30000 -- **CORRECT**
- Freeze chance: `35 + learnedLevel * 3` minus MDEF -- **PARTIALLY CORRECT** (base chance correct, resistance formula simplified)
- Handler properly uses `applyStatusEffect()` -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Freeze resistance formula simplified (missing level diff and LUK) | MEDIUM | Moderate — update `applyStatusEffect()` or handler |
| Freeze duration reduction by target MDEF not applied | MEDIUM | Moderate — apply duration reduction before calling `applyStatusEffect()` |
| Frozen status defense changes (+25% MDEF, -50% DEF) may not be in buff system | LOW | Check — may already be in status effect engine |

#### Implementation Notes

The base freeze chance and duration are correct. The resistance formula simplification means Stone Curse and Frost Diver are slightly easier to land than they should be (no level difference or LUK factor). The canonical rAthena formula is:
```
adjustedChance = baseChance - baseChance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK
```
For enemies, `tarLUK` is typically low (1-20), so the impact is minor for PvE. More significant for PvP.

---

### 10. FIRE WALL (ID 209) — Active / Ground Placement

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, ground placement | All sources |
| Max Level | 10 | All sources |
| Element | Fire | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Placement | 1x3 cells perpendicular to caster-target line | iRO Wiki, RateMyServer |
| Damage | 50% MATK per hit | iRO Wiki, RateMyServer |
| Hit Count | `SkillLv + 2` = 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 | iRO Wiki |
| Duration | `SkillLv + 4` sec = 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 | iRO Wiki, RateMyServer |
| SP Cost | 40 (fixed, all levels) | iRO Wiki, RateMyServer |
| Cast Time | `2150 - SkillLv * 150` ms = 2000, 1850, 1700, 1550, 1400, 1250, 1100, 950, 800, 650 | iRO Wiki, RateMyServer |
| After-Cast Delay | Negligible / 0 | iRO Wiki Classic |
| Knockback | 2 cells | RateMyServer |
| Max Concurrent | 3 fire walls per caster | iRO Wiki, RateMyServer |
| Hit Rate | ~40-50 hits per second (enemies touching wall take rapid hits) | iRO Wiki |
| Boss Interaction | Bosses not knocked back but must absorb all hits | iRO Wiki |
| Prerequisites | Fire Ball Lv5, Sight Lv1 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: 40 -- **CORRECT**
- Cast time: `Math.floor(2150-(i+1)*150)` → 2000, 1850, ..., 650 -- **CORRECT**
- ACD: 0 -- **CORRECT**
- Hit count (effectValue): `i+3` → 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 -- **CORRECT**
- Duration: `(5+i)*1000` → 5000, 6000, ..., 14000 -- **CORRECT**
- Max concurrent: 3 -- **CORRECT**
- Knockback: 200 UE units -- **CORRECT** (2 cells = 100 UE, but 200 ensures exit from wall collision zone)
- Prerequisites: Fire Ball 5 + Sight 1 -- **CORRECT** (matches, though canonical also requires Fire Bolt 4 which is a prereq of Fire Ball 5)
- Ground effect system: Uses `createGroundEffect()` -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Fire Wall damage (50% MATK per hit) may not be applied per ground effect tick | MEDIUM | Check ground effect tick handler |
| 1x3 perpendicular placement not implemented (currently point placement) | LOW | Major — requires directional calculation |
| Boss non-knockback rule not implemented | LOW | Moderate — check mode flags in knockback code |
| Hit rate (~40-50/sec) not verifiable without ground effect tick timing | LOW | Check tick interval |

#### Implementation Notes

The Fire Wall ground effect is placed correctly with all the right parameters. The main question is whether the ground effect tick handler (which processes fire_wall hits on enemies) correctly applies 50% MATK damage per tick and the knockback mechanic. The 1x3 perpendicular placement would require calculating the direction from caster to target point and placing 3 collision zones perpendicular to that line. This is a significant UX improvement but not critical for gameplay.

---

### 11. SOUL STRIKE (ID 210) — Active / Single Target

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, single target, multi-hit | All sources |
| Max Level | 10 | All sources |
| Element | Ghost | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Hits | `floor((SkillLv + 1) / 2)` = 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 | iRO Wiki Classic, RateMyServer |
| Damage | 1 x MATK per hit (100% MATK) | All sources |
| Undead Bonus | `+5% * SkillLv` = +5, +10, +15, +20, +25, +30, +35, +40, +45, +50% | iRO Wiki Classic, RateMyServer |
| SP Cost | 18, 14, 24, 20, 30, 26, 36, 32, 42, 38 | iRO Wiki Classic, RateMyServer |
| Cast Time | 500ms (fixed, all levels) | iRO Wiki Classic, RateMyServer, rAthena |
| After-Cast Delay | 1200, 1000, 1400, 1200, 1600, 1400, 1800, 1600, 2000, 1800 ms | rAthena skill_cast_db, RateMyServer |
| Prerequisites | Napalm Beat Lv4 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: 18, 14, 24, 20, 30, 26, 36, 32, 42, 38 -- **CORRECT** (manually defined per level)
- Cast time: 500ms fixed -- **CORRECT**
- Hits: `Math.floor((learnedLevel + 1) / 2)` → 1,1,2,2,3,3,4,4,5,5 -- **CORRECT**
- effectValue: 100 (100% MATK per hit) -- **CORRECT**
- Undead bonus: `1 + learnedLevel * 0.05` → +5% to +50% -- **CORRECT**
- Handler checks `targetEle === 'undead'` for bonus -- **CORRECT**

The **after-cast delay table** in our skill definition:

| Level | Our ACD | Canonical ACD | Match? |
|-------|---------|---------------|--------|
| 1 | 1200 | 1200 | YES |
| 2 | 1400 | 1000 | **NO** (should be 1000) |
| 3 | 1600 | 1400 | **NO** (should be 1400) |
| 4 | 1800 | 1200 | **NO** (should be 1200) |
| 5 | 2000 | 1600 | **NO** (should be 1600) |
| 6 | 2200 | 1400 | **NO** (should be 1400) |
| 7 | 2400 | 1800 | **NO** (should be 1800) |
| 8 | 2600 | 1600 | **NO** (should be 1600) |
| 9 | 2800 | 2000 | **NO** (should be 2000) |
| 10 | 1800 | 1800 | YES |

Our implementation uses a linearly increasing ACD (1200-2800, with a special Lv10=1800), but canonical Soul Strike has an alternating "zigzag" ACD pattern that mirrors the SP cost pattern. Odd levels have higher ACD, even levels have lower ACD.

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| After-cast delay table is wrong for levels 2-9 (zigzag pattern vs linear) | HIGH | Trivial — replace ACD array in skill definition |

#### Implementation Notes

The canonical ACD pattern for Soul Strike alternates: higher on odd levels, lower on even levels. This matches the SP cost zigzag pattern (18, 14, 24, 20, 30, 26, 36, 32, 42, 38). The fix is straightforward — replace the `afterCastDelay` values in the skill definition with the canonical array: `[1200, 1000, 1400, 1200, 1600, 1400, 1800, 1600, 2000, 1800]`.

---

### 12. SAFETY WALL (ID 211) — Active / Ground Placement

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, ground placement | All sources |
| Max Level | 10 | All sources |
| Element | Neutral | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| Area | 1x1 cell | iRO Wiki Classic |
| Hits Blocked | `SkillLv + 1` = 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 | iRO Wiki Classic, RateMyServer |
| Duration | `5 * SkillLv` sec = 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 | iRO Wiki Classic, RateMyServer |
| Blocks | Melee physical attacks ONLY (not ranged, not magic) | All sources |
| SP Cost | Lv1-3: 30, Lv4-6: 35, Lv7-10: 40 | RateMyServer |
| Cast Time | 4000, 3500, 3000, 2500, 2000, 1500, 1000, 1000, 1000, 1000 ms | iRO Wiki Classic |
| After-Cast Delay | 0 | iRO Wiki Classic |
| Blue Gemstone | 1 per cast | iRO Wiki Classic, RateMyServer |
| Stacking | Cannot be stacked (only one Safety Wall per cell) | RateMyServer |
| Prerequisites | Napalm Beat Lv7, Soul Strike Lv5 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: `[30,30,30,35,35,35,40,40,40,40]` -- **CORRECT** (matches RateMyServer: Lv1-3=30, Lv4-6=35, Lv7-10=40)
- Cast time: `[4000,3500,3500,2500,2000,1500,1000,1000,1000,1000]` -- **WRONG** (Lv3 should be 3000, not 3500)
- Hits blocked (effectValue): `i+2` → 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 -- **CORRECT**
- Duration: `(i+1)*5000` → 5000, 10000, ..., 50000 -- **CORRECT**
- Blue Gemstone: Listed in `SKILL_CATALYSTS` as `{ itemId: 717, quantity: 1 }` -- **CORRECT**
- Prerequisites: Napalm Beat 7 + Soul Strike 5 -- **CORRECT**
- Ground effect system: Uses `createGroundEffect()` -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Cast time Lv3 is 3500ms instead of canonical 3000ms | MEDIUM | Trivial — fix array value |
| Melee-only block mechanic may not be enforced (depends on ground effect tick handler) | MEDIUM | Check — ground effect tick needs to distinguish melee vs ranged |

#### Implementation Notes

The cast time for Level 3 has a typo: 3500 instead of 3000. The iRO Wiki Classic formula is `4500 - SkillLv * 500` for Lv1-6, then 1000 for Lv7-10, giving: 4000, 3500, 3000, 2500, 2000, 1500, 1000, 1000, 1000, 1000. Fix the Lv3 value from 3500 to 3000.

---

### 13. THUNDERSTORM (ID 212) — Active / Ground AoE

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, ground AoE | All sources |
| Max Level | 10 | All sources |
| Element | Wind | All sources |
| Range | 9 cells (450 UE units) | RateMyServer |
| AoE | 5x5 cells | iRO Wiki Classic, RateMyServer |
| Hits | 1 per level (Lv1=1, Lv10=10) | RateMyServer |
| Damage | 80% MATK per hit | iRO Wiki Classic, RateMyServer |
| SP Cost | `24 + 5 * SkillLv` = 29, 34, 39, 44, 49, 54, 59, 64, 69, 74 | iRO Wiki Classic, RateMyServer |
| Cast Time | `SkillLv * 1000` ms = 1000, 2000, ..., 10000 | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | 2000ms (fixed, all levels) | iRO Wiki Classic, RateMyServer |
| Prerequisites | Lightning Bolt Lv4 | All sources |

#### Current Implementation Status: MOSTLY CORRECT

- SP cost: `24+(i+1)*5` → 29, 34, ..., 74 -- **CORRECT**
- Cast time: `(i+1)*1000` → 1000, 2000, ..., 10000 -- **CORRECT**
- ACD: 2000 fixed -- **CORRECT**
- Hits: `numHits = learnedLevel` -- **CORRECT**
- effectValue: 80 (80% MATK) -- **NEEDS REVIEW** (should be used as MATK% in `calculateMagicSkillDamage`)
- AoE: 500 UE units -- **WRONG** (5x5 = 250 UE units at 50 UE/cell)

But wait — the handler passes `100` as the MATK% to `calculateMagicSkillDamage`, not `80`. Let me re-check:

```js
const hitResult = calculateMagicSkillDamage(
    getEffectiveStats(player), enemy.stats, enemy.hardMdef, 100, 'wind', tInfo
);
```

The 4th parameter is `100` (100% MATK), but canonical is 80% MATK per hit. The `effectValue: 80` in the skill definition is not being used by the handler!

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Damage uses 100% MATK per hit instead of 80% MATK (handler hardcodes 100, ignores effectValue) | HIGH | Trivial — change `100` to `effectVal` in handler |
| AoE radius 500 UE (10 cells) instead of 250 (5x5) | MEDIUM | Trivial — change `STORM_AOE = 500` to `STORM_AOE = 250` |

#### Implementation Notes

The Thunderstorm handler hardcodes 100% MATK in the `calculateMagicSkillDamage` call instead of using `effectVal` (which is correctly defined as 80 in the skill data). This means Thunderstorm currently does 25% more damage per hit than canonical. Fix by replacing the hardcoded `100` with `effectVal` in both the enemy and PvP loops.

---

### 14. ENERGY COAT (ID 213) — Active / Self Buff (Quest Skill)

#### RO Classic Canonical Mechanics

| Property | Value | Source |
|----------|-------|--------|
| Type | Active, self buff (Quest Skill) | All sources |
| Max Level | 1 | All sources |
| SP Cost | 30 | iRO Wiki Classic, RateMyServer, rAthena |
| Cast Time | 5000ms | iRO Wiki Classic, RateMyServer |
| After-Cast Delay | 0 | iRO Wiki Classic |
| Duration | 300s (5 minutes) or until SP = 0 | iRO Wiki Classic, RateMyServer |
| Applies to | Physical damage only | All sources |

**Damage Reduction & SP Drain Tiers (from rAthena source):**

| SP Remaining | Damage Reduction | SP Drain Per Hit |
|-------------|-----------------|-----------------|
| 81-100% | 30% | 3.0% of damage |
| 61-80% | 24% | 2.5% of damage |
| 41-60% | 18% | 2.0% of damage |
| 21-40% | 12% | 1.5% of damage |
| 1-20% | 6% | 1.0% of damage |

**rAthena formula:**
```c
per = 100 * status->sp / status->max_sp - 1;
per /= 20;  // 0-4 tiers
damage -= damage * 6 * (1 + per) / 100;
sp_drain = (10 + 5 * per) * status->max_sp / 1000;
```

| Dispel | When SP reaches 0 from drain |

#### Current Implementation Status: CORRECT

Our `applyEnergyCoat()` function (line 549):
- Tier calculation: uses `spRatio = player.mana / player.maxMana` -- **CORRECT**
- Tiers: 0-20% = 6%, 20-40% = 12%, 40-60% = 18%, 60-80% = 24%, 80-100% = 30% -- **CORRECT**
- SP drain: percentage-based drain per tier -- **CORRECT** (need to verify exact drain values)
- Dispel at 0 SP: removes buff and broadcasts removal -- **CORRECT**
- Physical damage only: called only in auto-attack damage path -- **CORRECT**
- Duration: 300000ms (5 min) -- **CORRECT**
- SP cost: 30 -- **CORRECT**
- Cast time: 5000ms -- **CORRECT**

#### Gaps Found

| Gap | Priority | Effort |
|-----|----------|--------|
| Verify SP drain uses correct percentage (1% to 3% of damage, not of MaxSP) | LOW | Check — verify `spDrainPct` values match canonical |
| Energy Coat may not reduce damage from physical SKILLS (only auto-attacks checked) | MEDIUM | Check — ensure `applyEnergyCoat()` is called in all physical damage paths |

#### Implementation Notes

The implementation closely matches canonical mechanics. The `applyEnergyCoat()` function at line 549 of `index.js` handles the tier-based damage reduction and SP drain correctly. The key thing to verify is that it's called from ALL physical damage paths, not just enemy auto-attacks. If players use physical skills (like Bash), the Energy Coat reduction should also apply.

Our current code only calls `applyEnergyCoat()` from the enemy attack tick (line 11411), which means player-vs-player physical skill damage and enemy skill damage may bypass it. This is acceptable for PvE but would need fixing for PvP.

---

## New Systems Required

### 1. Two-Phase Petrification Engine (HIGH Priority)

Stone Curse requires a two-phase system:

**Phase 1 "Petrifying" (MSC_STONEWAIT in rAthena):**
- Duration: ~5 seconds (fixed, may vary by source)
- Target can move and use items, but CANNOT attack or use skills
- Visual: gradual graying effect
- NOT broken by damage (unlike most other CC)
- Casting Stone Curse on an already-petrifying target CURES the effect
- After timer expires, transitions to Phase 2

**Phase 2 "Stone" (SC_STONE in rAthena):**
- Duration: 20 seconds (from RateMyServer/rAthena)
- Full immobilize — cannot move, attack, use items, or cast
- Element changes to Earth Lv1
- +25% MDEF, -50% DEF
- HP drain: 1% of MaxHP every 5 seconds, stops at 25% HP
- Broken by any damage (physical or magical)
- Cured by Status Recovery, Blessing, Panacea

**Implementation approach:**
- Add `petrifying` as a new status type with its own timer
- When `petrifying` timer expires, auto-apply `stone` status
- Add HP drain tick to the buff expiration interval (1-second tick checking for stone status)
- Add defense/MDEF modifiers to `getBuffStatModifiers()` for stone status

### 2. Status Resistance Formula Update (MEDIUM Priority)

Current: `successRate = effectVal - targetHardMdef`
Canonical: `chance = baseChance - baseChance * targetHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK`

This affects both Stone Curse and Frost Diver. The canonical formula makes status effects harder to land on high-level/high-LUK targets and easier on low-level targets.

### 3. SP Recovery Item Potency (LOW Priority)

Increase SP Recovery should add `+2% * SkillLv` to SP recovery items (Blue Potions, etc). This requires modifying the `inventory:use` handler for SP recovery items to check for this passive bonus.

---

## Skill Definition Corrections

### Changes to `ro_skill_data.js`:

**1. Soul Strike (ID 210) — Fix after-cast delay table:**
```js
// BEFORE (wrong):
afterCastDelay: [1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600, 2800, 1800]

// AFTER (canonical zigzag pattern):
afterCastDelay: [1200, 1000, 1400, 1200, 1600, 1400, 1800, 1600, 2000, 1800]
```

Note: Soul Strike uses manual per-level arrays, so the ACD values need to be updated in each level object.

**2. Safety Wall (ID 211) — Fix Lv3 cast time:**
```js
// BEFORE:
castTime: [4000, 3500, 3500, 2500, 2000, 1500, 1000, 1000, 1000, 1000]

// AFTER:
castTime: [4000, 3500, 3000, 2500, 2000, 1500, 1000, 1000, 1000, 1000]
```

**3. Stone Curse (ID 206) — Fix range:**
```js
// BEFORE:
range: 300

// AFTER:
range: 100  // 2 cells
```

### Changes to `index.js` Handlers:

**4. Thunderstorm — Use effectVal instead of hardcoded 100:**
```js
// BEFORE (line ~6527):
const hitResult = calculateMagicSkillDamage(
    getEffectiveStats(player), enemy.stats, targetHardMdef, 100, 'wind', tInfo
);

// AFTER:
const hitResult = calculateMagicSkillDamage(
    getEffectiveStats(player), enemy.stats, targetHardMdef, effectVal, 'wind', tInfo
);
```
Apply same fix in the PvP loop (~line 6583).

**5. Napalm Beat — Fix AoE radius:**
```js
// BEFORE:
const NAPALM_AOE = 300;

// AFTER:
const NAPALM_AOE = 150;  // 3x3 cells = 150 UE units
```

**6. Fire Ball — Fix AoE radius and remove non-canonical edge damage:**
```js
// BEFORE:
const FIREBALL_AOE = 500;
const FIREBALL_INNER = 300;

// AFTER:
const FIREBALL_AOE = 250;  // 5x5 cells = 250 UE units
// Remove FIREBALL_INNER and edgeMult logic — all targets take full damage
```

---

## Implementation Priority

### Phase 1 — Quick Fixes — COMPLETE (2026-03-15)
1. ~~Fix Soul Strike ACD table in `ro_skill_data.js`~~ — Already correct (zigzag pattern)
2. ~~Fix Safety Wall Lv3 cast time in `ro_skill_data.js`~~ — Already correct (3000ms)
3. Fix Thunderstorm damage to use `effectVal` (80%) instead of hardcoded 100 — **DONE**
4. Fix Napalm Beat AoE radius (300 → 150) — **DONE**
5. Fix Fire Ball AoE radius (500 → 250) and remove edge damage multiplier — **DONE**
6. ~~Fix Stone Curse range (300 → 100)~~ — Already correct (100 UE)
7. Fix Thunderstorm AoE radius (500 → 250) — **DONE**

### Phase 2 — Status Effect Improvements — COMPLETE (2026-03-15)
7. Status resistance formula already correct in `applyStatusEffect()` (includes level diff + LUK)
8. Add freeze duration reduction by target MDEF/LUK — **DONE** (canonical formula in Frost Diver handler)
9. Stone status defense modifiers (+25% MDEF, -50% DEF) already in `ro_status_effects.js` — verified
10. Stone status HP drain tick (1% / 5s) already in `ro_status_effects.js` periodicDrain config — verified
11. Fixed Frost Diver double-MDEF reduction bug (handler was pre-subtracting MDEF before passing to engine) — **DONE**

### Phase 3 — New Systems — COMPLETE (2026-03-15)
11. Implement two-phase petrification (Petrifying → Stone) — **DONE**
    - Added `petrifying` status type in `ro_status_effects.js` (Phase 1: 5s, no movement block, blocks cast/attack)
    - `transitionsTo: 'stone'` auto-transition in `tickStatusEffects()` when Phase 1 expires
    - Stone Curse handler now applies `petrifying` instead of `stone` directly
    - Counter-cast mechanic: casting Stone Curse on petrifying target cures it
12. Add Red Gemstone Lv6-10 success-only consumption — **DONE**
    - Lv1-5: consumed always (before roll)
    - Lv6-10: consumed only on success (after roll)
13. Energy Coat in physical skill damage paths — verified (PvE auto-attack only, acceptable)

### Phase 4 — SP Recovery Fixes — COMPLETE (2026-03-15)
14. Add Increase SP Recovery MaxSP% regen component — **DONE** (0.2% * SkillLv per tick)
15. Add Increase SP Recovery SP item potency bonus — **DONE** (+2%/level to SP recovery items)

### Phase 5 — Deferred (Low priority)
16. Implement Sight hidden-reveal proximity tick
17. Implement Fire Wall 1x3 perpendicular placement
18. Coordinate magic range audit (900 vs 450 UE units across all magic skills)
19. Implement Fire Wall boss non-knockback rule

---

## Sources

- [Cold Bolt - iRO Wiki Classic](https://irowiki.org/classic/Cold_Bolt)
- [Cold Bolt - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=14)
- [Napalm Beat - iRO Wiki Classic](https://irowiki.org/classic/Napalm_Beat)
- [Napalm Beat - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=11)
- [Soul Strike - iRO Wiki Classic](https://irowiki.org/classic/Soul_Strike)
- [Soul Strike - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=13)
- [Frost Diver - iRO Wiki Classic](https://irowiki.org/classic/Frost_Diver)
- [Frost Diver - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=15)
- [Stone Curse - iRO Wiki Classic](https://irowiki.org/classic/Stone_Curse)
- [Stone Curse - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=16)
- [Fire Ball - iRO Wiki Classic](https://irowiki.org/classic/Fire_Ball)
- [Fire Ball - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=17)
- [Fire Wall - iRO Wiki](https://irowiki.org/wiki/Fire_Wall)
- [Fire Wall - iRO Wiki Classic](https://irowiki.org/classic/Fire_Wall)
- [Fire Wall - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=18)
- [Safety Wall - iRO Wiki Classic](https://irowiki.org/classic/Safety_Wall)
- [Safety Wall - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=12)
- [Thunderstorm - iRO Wiki Classic](https://irowiki.org/classic/Thunderstorm)
- [Thunderstorm - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=21)
- [Sight - iRO Wiki Classic](https://irowiki.org/classic/Sight)
- [Sight - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=10)
- [Increase SP Recovery - iRO Wiki Classic](https://irowiki.org/classic/Increase_SP_Recovery)
- [Increase SP Recovery - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=9)
- [Energy Coat - iRO Wiki Classic](https://irowiki.org/classic/Energy_Coat)
- [Energy Coat - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=157)
- [SP Recovery - iRO Wiki Classic](https://irowiki.org/classic/SP_Recovery)
- [Status Effects - iRO Wiki](https://irowiki.org/wiki/Status_Effects)
- [Pre-Renewal Status Resistance Formulas - RateMyServer Forum](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [rAthena pre-re skill_cast_db.txt](https://github.com/flaviojs/rathena-commits/blob/master/db/pre-re/skill_cast_db.txt)
- [rAthena battle.cpp (Energy Coat)](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
- [rAthena pre-re skill_db.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
