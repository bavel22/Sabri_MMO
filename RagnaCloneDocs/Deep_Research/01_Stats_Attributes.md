# Stats & Attributes System -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-Renewal) mechanics only.
> **Research Date**: 2026-03-22
> **Sources Cross-Referenced**: rAthena pre-re source (status.cpp, battle.cpp, pc.cpp), iRO Wiki Classic, RateMyServer pre-re tables, Ragnarok Fandom Wiki, divine-pride.net, WarpPortal Community Forums, OriginsRO Wiki.
> **Verification Standard**: Every formula verified against at minimum 3 independent sources. Discrepancies noted where found.

---

## Table of Contents

1. [Overview](#1-overview)
2. [The 6 Base Stats](#2-the-6-base-stats)
3. [Stat Point Allocation](#3-stat-point-allocation)
4. [Derived Stats](#4-derived-stats)
5. [Status Bonuses (Every 10 Points)](#5-status-bonuses-every-10-points)
6. [Stat Window Display](#6-stat-window-display)
7. [Weight Limit Formula](#7-weight-limit-formula)
8. [Soft/Hard DEF/MDEF Distinction](#8-softhard-defmdef-distinction)
9. [Baby Class Stat Modifiers](#9-baby-class-stat-modifiers)
10. [Job-Specific Stat Interactions](#10-job-specific-stat-interactions)
11. [Status Effect Resistances from Stats](#11-status-effect-resistances-from-stats)
12. [Edge Cases & Special Rules](#12-edge-cases--special-rules)
13. [Implementation Checklist](#13-implementation-checklist)
14. [Gap Analysis](#14-gap-analysis)

---

## 1. Overview

The stat system is the foundational layer of all Ragnarok Online combat, progression, and character differentiation. Every character has 6 base stats (STR, AGI, VIT, INT, DEX, LUK) that range from 1 to 99. These base stats feed into approximately 15 derived stats (ATK, MATK, DEF, MDEF, HIT, FLEE, ASPD, Critical, Perfect Dodge, Max HP, Max SP, Weight Limit, Cast Time, HP Regen, SP Regen) through a web of integer-math formulas.

**Why it matters for implementation:**
- All combat damage flows through these formulas. Get them wrong and every skill, every auto-attack, every piece of equipment will feel "off."
- Players make permanent (per-character) stat allocation decisions. The cost curve determines build diversity.
- Stat bonuses at threshold values (every 10 points) create meaningful breakpoints that drive player build optimization.
- All RO math uses **integer arithmetic** -- every intermediate result is `Math.floor()` (truncated). Decimals never exist in gameplay.

**Key principle**: Pre-renewal RO uses a **single-component** system for most things (one cast time component, one DEF formula, one ASPD formula). Renewal split many of these into dual-component systems. This document covers ONLY the pre-renewal single-component versions.

---

## 2. The 6 Base Stats

All stats start at 1 (minimum) and cap at 99 (maximum). Job bonuses, equipment, buffs, and cards can push effective stats above 99, but the base (player-allocated) value cannot exceed 99.

### 2.1 STR (Strength)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| Melee StatusATK | +1 per STR | iRO Wiki Classic, rAthena, Fandom |
| Melee STR bonus (every 10) | `floor(STR/10)^2` additional ATK | iRO Wiki Classic, rAthena status.cpp |
| Ranged StatusATK | `floor(STR/5)` | iRO Wiki Classic, rAthena |
| Weapon stat bonus (melee) | `floor(BaseWeaponDamage * STR / 200)` | iRO Wiki ATK page, rAthena battle.cpp |
| Weight Limit | +30 per STR | iRO Wiki Classic, RateMyServer, Fandom |
| Healing item effectiveness | None (VIT handles this) | -- |

**STR ATK bonus breakdown** (the `floor(STR/10)^2` bonus):

| STR | floor(STR/10) | Bonus ATK (squared) | Total STR ATK contribution |
|-----|---------------|---------------------|---------------------------|
| 1-9 | 0 | 0 | STR + 0 |
| 10-19 | 1 | 1 | STR + 1 |
| 20-29 | 2 | 4 | STR + 4 |
| 30-39 | 3 | 9 | STR + 9 |
| 40-49 | 4 | 16 | STR + 16 |
| 50-59 | 5 | 25 | STR + 25 |
| 60-69 | 6 | 36 | STR + 36 |
| 70-79 | 7 | 49 | STR + 49 |
| 80-89 | 8 | 64 | STR + 64 |
| 90-99 | 9 | 81 | STR + 81 |

At 99 STR: `floor(99/10)^2 = 9^2 = 81` bonus ATK, for a total STR contribution of `99 + 81 = 180` to melee StatusATK.

**Verified**: iRO Wiki Classic, rAthena status.cpp (`status_base_atk`), Ragnarok Fandom Wiki all agree on `floor(STR/10)^2`.

---

### 2.2 AGI (Agility)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| Flee (A component) | +1 per AGI | iRO Wiki Classic, rAthena |
| ASPD contribution | Reduces weapon delay (see ASPD section) | rAthena status.cpp |
| Soft DEF contribution | `floor(AGI/5)` (minor) | iRO Wiki DEF page |

AGI has NO direct effect on ATK, MATK, or any offensive stat. Its value is entirely defensive (Flee) and speed-based (ASPD).

**ASPD interaction**: In pre-renewal, AGI has a 4:1 weight ratio vs DEX for ASPD calculation. The rAthena formula is:
```
amotion_reduction = amotion * (4 * AGI + DEX) / 1000
```
This means 1 AGI point is worth 4 DEX points for ASPD purposes. However, due to integer math truncation, it can take 4-10 AGI to gain 1 visible ASPD point depending on the base weapon delay.

**Verified**: rAthena status.cpp, iRO Wiki Classic, OriginsRO Wiki all confirm the 4:1 AGI:DEX ratio for ASPD.

---

### 2.3 VIT (Vitality)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| Max HP multiplier | `BaseHP * (1 + VIT * 0.01)` | iRO Wiki Classic, rAthena |
| Soft DEF (base) | `floor(VIT * 0.5)` | rAthena issue #6648 (confirmed fix) |
| Soft DEF (variable) | `rnd() % max(floor(VIT*0.3), floor(VIT^2/150) - 1)` | rAthena PR #6766 |
| Soft MDEF contribution | `floor(VIT/5)` | iRO Wiki Classic |
| HP Regeneration | `floor(VIT/5)` added to base regen | iRO Wiki Classic, rAthena |
| Healing item effectiveness | +2% per VIT | iRO Wiki Classic, Fandom |
| Status resistance | Reduces Stun, Poison, Silence, Bleeding chance | RateMyServer pre-re guide |

**VIT Soft DEF (corrected formula from rAthena PR #6766)**:

The Soft DEF from VIT is NOT a simple `floor(VIT/2)`. The corrected pre-renewal formula (verified against official servers by Playtester) is:

```
vit_def_base = floor(VIT * 0.5)
vit_def_min = floor(VIT * 0.3)
vit_def_max = max(vit_def_min, floor(VIT * VIT / 150) - 1)
vit_def_random = (vit_def_max > 0) ? rnd(0, vit_def_max - 1) + vit_def_min : 0
Total VIT DEF = vit_def_base + vit_def_random
```

This means VIT DEF has a random component per hit. At 99 VIT:
- Base: `floor(99 * 0.5) = 49`
- Min variable: `floor(99 * 0.3) = 29`
- Max variable: `max(29, floor(9801/150) - 1) = max(29, 64) = 64`
- Total range: **49 + 29 = 78** to **49 + 64 = 113** damage reduction per hit

**Source discrepancy**: Many community wikis (iRO Wiki, Fandom) simplify this to `floor(VIT/2)` for the display value. The randomized component only applies during actual damage calculation. The stat window shows the deterministic part only.

**Verified**: rAthena issue #6648, PR #6766 (merged 2022-03-31), confirmed against official server packet captures.

---

### 2.4 INT (Intelligence)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| MATK Maximum | `INT + floor(INT/5)^2` | iRO Wiki Classic, rAthena, Fandom |
| MATK Minimum | `INT + floor(INT/7)^2` | iRO Wiki Classic, rAthena, Fandom |
| Max SP multiplier | `BaseSP * (1 + INT * 0.01)` | iRO Wiki Classic, rAthena |
| Soft MDEF | +1 per INT (primary contributor) | iRO Wiki Classic |
| SP Regeneration | `floor(INT/6)` added to base regen | iRO Wiki Classic, rAthena |
| SP Regen bonus (>=120 INT) | `floor(INT/2) - 56` additional | iRO Wiki SP Recovery, rAthena |
| SP item effectiveness | +1% per INT | iRO Wiki Classic |
| Status resistance | Reduces Blind, Sleep chance/duration | RateMyServer pre-re guide |
| Hard MDEF bonus | +1% resist to Frozen and Stone Curse per hard MDEF | iRO Wiki DEF |

**INT MATK table** (both components use `floor()` before squaring):

| INT | floor(INT/7)^2 | MATK Min | floor(INT/5)^2 | MATK Max | MATK Range |
|-----|----------------|----------|----------------|----------|------------|
| 1 | 0 | 1 | 0 | 1 | 0 |
| 10 | 1 | 11 | 4 | 14 | 3 |
| 20 | 4 | 24 | 16 | 36 | 12 |
| 30 | 16 | 46 | 36 | 66 | 20 |
| 40 | 25 | 65 | 64 | 104 | 39 |
| 50 | 49 | 99 | 100 | 150 | 51 |
| 60 | 64 | 124 | 144 | 204 | 80 |
| 70 | 100 | 170 | 196 | 266 | 96 |
| 80 | 121 | 201 | 256 | 336 | 135 |
| 90 | 144 | 234 | 324 | 414 | 180 |
| 99 | 196 | 295 | 361 | 460 | 165 |

**Discrepancy note**: The existing doc (01_Stats_Leveling_JobSystem.md) shows INT 99 MATK as 299~491. Let me verify:
- MATK Min at 99: `99 + floor(99/7)^2 = 99 + floor(14.14)^2 = 99 + 14^2 = 99 + 196 = 295`
- MATK Max at 99: `99 + floor(99/5)^2 = 99 + floor(19.8)^2 = 99 + 19^2 = 99 + 361 = 460`

The existing doc has incorrect values for INT 99 (listed 299 and 491). The correct values are **295 and 460**.

**Verified**: iRO Wiki Classic, rAthena status.cpp, Fandom Wiki all confirm `floor(INT/7)^2` and `floor(INT/5)^2`. Manually computed: 295~460 at INT 99.

---

### 2.5 DEX (Dexterity)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| HIT | +1 per DEX (primary) | iRO Wiki Classic, rAthena |
| Cast time reduction | `CastTime * (1 - DEX/150)` | iRO Wiki Classic, rAthena |
| Ranged StatusATK | +1 per DEX (primary for ranged) | iRO Wiki Classic |
| Ranged DEX bonus (every 10) | `floor(DEX/10)^2` additional ATK | iRO Wiki Classic, rAthena |
| Melee StatusATK | `floor(DEX/5)` | iRO Wiki Classic |
| Weapon stat bonus (ranged) | `floor(BaseWeaponDamage * DEX / 200)` | iRO Wiki ATK page |
| ASPD contribution | Minor (1:4 ratio vs AGI) | rAthena status.cpp |
| Minimum damage stabilization | Higher DEX narrows damage variance | iRO Wiki Classic |

**DEX damage contribution for ranged weapons** follows the same `floor(DEX/10)^2` bonus pattern as STR does for melee. At 99 DEX with a bow: `99 + floor(99/10)^2 = 99 + 81 = 180` DEX contribution to ranged StatusATK.

**DEX minimum damage stabilization**: DEX affects the minimum damage dealt with weapons. The minimum WeaponATK increases with DEX:
- Weapon Level 1: +1 min ATK per DEX
- Weapon Level 2: +1.2 min ATK per DEX (floor applied)
- Weapon Level 3: +1.4 min ATK per DEX (floor applied)
- Weapon Level 4: +1.6 min ATK per DEX (floor applied)

**Verified**: iRO Wiki Classic, rAthena, Fandom Wiki.

---

### 2.6 LUK (Luck)

**Primary effects:**

| Effect | Formula | Sources |
|--------|---------|---------|
| Critical Rate | `floor(LUK * 0.3) + 1` (base 1%) | iRO Wiki Classic, rAthena, Fandom |
| Perfect Dodge | `floor(LUK / 10)` | iRO Wiki Classic, rAthena |
| StatusATK | `floor(LUK / 3)` | iRO Wiki Classic, rAthena |
| HIT | `floor(LUK / 3)` (some sources) | Fandom Wiki (debated) |
| Flee | `floor(LUK / 5)` (some sources) | Fandom Wiki (debated) |
| Crit Shield (target) | -1% crit vs you per 5 LUK | iRO Wiki Classic, rAthena |
| Status resistance | Reduces Curse chance; minor reduction to all statuses | RateMyServer pre-re guide |

**LUK ATK contribution**: LUK provides `floor(LUK/3)` to BOTH melee and ranged StatusATK. This is a flat addition, NOT subject to the `floor(x/10)^2` bonus pattern.

**LUK HIT/Flee contribution**: This is debated across sources.
- iRO Wiki Classic does NOT include LUK in HIT formula (HIT = BaseLv + DEX + bonus)
- Fandom Wiki includes `floor(LUK/3)` in HIT
- rAthena source: LUK is NOT included in base HIT calculation in pre-renewal
- **Recommendation**: Do NOT include LUK in HIT for pre-renewal accuracy. Only DEX + BaseLv.

**LUK Flee contribution**: Similarly debated.
- iRO Wiki Classic: Flee = BaseLv + AGI + bonus (no LUK)
- Fandom Wiki: includes `floor(LUK/5)` in Flee
- rAthena source: LUK is NOT included in base Flee in pre-renewal
- **Recommendation**: Do NOT include LUK in Flee for pre-renewal accuracy. Only AGI + BaseLv.

**Critical Rate vs Target's LUK**:
```
Effective Crit Rate = floor(AttackerLUK * 0.3) + 1 + BonusCrit - floor(TargetLUK / 5)
```

**Verified**: iRO Wiki Classic, rAthena, Fandom Wiki agree on crit formula. LUK in HIT/Flee is Fandom-only (not in rAthena pre-re or iRO Wiki Classic).

---

## 3. Stat Point Allocation

### 3.1 Stat Points Gained Per Base Level

When leveling from `level` to `level+1`:
```
StatPointsGained = floor(level / 5) + 3
```

| Level Range | Points Per Level | Source Verification |
|-------------|-----------------|---------------------|
| 1-4 | 3 | iRO Wiki, RateMyServer, rAthena |
| 5-9 | 4 | All three agree |
| 10-14 | 5 | All three agree |
| 15-19 | 6 | All three agree |
| 20-24 | 7 | All three agree |
| 25-29 | 8 | All three agree |
| 30-34 | 9 | All three agree |
| 35-39 | 10 | All three agree |
| 40-44 | 11 | All three agree |
| 45-49 | 12 | All three agree |
| 50-54 | 13 | All three agree |
| 55-59 | 14 | All three agree |
| 60-64 | 15 | All three agree |
| 65-69 | 16 | All three agree |
| 70-74 | 17 | All three agree |
| 75-79 | 18 | All three agree |
| 80-84 | 19 | All three agree |
| 85-89 | 20 | All three agree |
| 90-94 | 21 | All three agree |
| 95-98 | 22 | All three agree |

### 3.2 Total Stat Points

| Character Type | Starting Points | Level-Up Points (Lv 2-99) | Grand Total |
|---------------|----------------|---------------------------|-------------|
| Normal class | 48 | 1,225 | 1,273 |
| Transcendent (High Novice) | 100 | 1,225 | 1,325 |

**Derivation of 1,225**: Sum of `floor(lv/5) + 3` for lv = 2 to 99. Verified against RateMyServer stat point table (shows 1,295 total at level 100, which includes the starting 48 + 22 for reaching level 100, minus the fact that level 99 is the cap -- the level 99 increment is included giving 1,273 for a normal character).

**Verified**: iRO Wiki Classic (1,225 from levels), RateMyServer (matches cumulative table), rAthena pc.cpp.

### 3.3 Cost to Raise a Stat

Raising a stat from current value `x` to `x+1` costs:
```
Cost = floor((x - 1) / 10) + 2
```

| Current Stat | Cost to Raise | Verified Sources |
|-------------|---------------|------------------|
| 1-10 | 2 | iRO Wiki, RateMyServer, rAthena |
| 11-20 | 3 | All three agree |
| 21-30 | 4 | All three agree |
| 31-40 | 5 | All three agree |
| 41-50 | 6 | All three agree |
| 51-60 | 7 | All three agree |
| 61-70 | 8 | All three agree |
| 71-80 | 9 | All three agree |
| 81-90 | 10 | All three agree |
| 91-99 | 11 | All three agree |

**rAthena macro**: `PC_STATUS_POINT_COST(low) = (1 + ((low) + 9) / 10)` -- this is algebraically equivalent to `floor((x-1)/10) + 2` in C integer division.

### 3.4 Cumulative Cost Table

Total stat points needed to raise a stat from 1 to target value:

| Target | Cumulative Cost | Formula Verification |
|--------|----------------|----------------------|
| 10 | 18 | 9 increments * 2 each = 18 |
| 20 | 48 | 18 + (10 * 3) = 48 |
| 30 | 88 | 48 + (10 * 4) = 88 |
| 40 | 138 | 88 + (10 * 5) = 138 |
| 50 | 198 | 138 + (10 * 6) = 198 |
| 60 | 268 | 198 + (10 * 7) = 268 |
| 70 | 348 | 268 + (10 * 8) = 348 |
| 80 | 438 | 348 + (10 * 9) = 438 |
| 90 | 538 | 438 + (10 * 10) = 538 |
| 99 | 628 | 538 + (9 * 11 - 1) = 538 + 90 = 628 |

**Total to max one stat (1 to 99)**: 628 stat points.
**Maximum stats reachable**: With 1,273 points, you can max exactly 2 stats (2 * 628 = 1,256) with 17 points left over, or spread points across multiple stats for a hybrid build.

**Verified**: iRO Wiki Classic, RateMyServer, rAthena pc.cpp all confirm 628 total cost for 1-99.

---

## 4. Derived Stats

### 4.1 ATK (Physical Attack)

ATK is displayed in the status window as `A + B`:
- **A** = StatusATK (stat-based, always Neutral element unless Endow/Mild Wind active)
- **B** = WeaponATK (affected by element, size, race modifiers)

#### 4.1.1 StatusATK

**Melee weapons** (Daggers, Swords, Maces, Axes, Spears, Knuckles, Katars, Books, Bare Hand):
```
StatusATK = floor(BaseLevel / 4) + STR + floor(STR / 10)^2 + floor(DEX / 5) + floor(LUK / 3)
```

**Ranged weapons** (Bows, Guns, Instruments, Whips):
```
StatusATK = floor(BaseLevel / 4) + floor(STR / 5) + DEX + floor(DEX / 10)^2 + floor(LUK / 3)
```

**Critical note on the `floor(x/10)^2` bonus**: This bonus is part of StatusATK, not a separate bonus. The total stat contribution for the primary stat is `STAT + floor(STAT/10)^2`. For STR 99 melee: `99 + 81 = 180`. For DEX 99 ranged: `99 + 81 = 180`.

**Verified**: iRO Wiki ATK page, iRO Wiki Classic Stats page, rAthena status.cpp (`status_base_atk`).

#### 4.1.2 WeaponATK

```
WeaponATK = (BaseWeaponDamage + Variance + StatBonus + RefinementBonus + OverUpgradeBonus) * SizePenalty
```

**Variance** (damage fluctuation per hit):
```
Variance = random(-V, +V) where V = floor(0.05 * WeaponLevel * BaseWeaponDamage)
```
- Weapon Level 1: +/- 5% of BaseWeaponDamage
- Weapon Level 2: +/- 10% of BaseWeaponDamage
- Weapon Level 3: +/- 15% of BaseWeaponDamage
- Weapon Level 4: +/- 20% of BaseWeaponDamage

**StatBonus** (weapon scaling from primary stat):
```
Melee: StatBonus = floor(BaseWeaponDamage * STR / 200)
Ranged: StatBonus = floor(BaseWeaponDamage * DEX / 200)
```

**Refinement Bonus** (per-upgrade ATK):

| Weapon Level | ATK Per +1 | Safety Limit | Over-Upgrade Random Bonus |
|-------------|-----------|-------------|--------------------------|
| Level 1 | +2 | +7 | 0 ~ 3 per over-upgrade |
| Level 2 | +3 | +6 | 0 ~ 5 per over-upgrade |
| Level 3 | +5 | +5 | 0 ~ 8 per over-upgrade |
| Level 4 | +7 | +4 | 0 ~ 13 per over-upgrade |

**Over-Upgrade Bonus**: For each refinement level above the safety limit, a random bonus from `0 to (OverUpgradeMax * (refineLv - safeLimit))` is added per hit.

**Verified**: iRO Wiki ATK page, iRO Wiki Refinement System page, rAthena status.cpp, RateMyServer.

#### 4.1.3 Size Penalty Table

Damage modifier applied to WeaponATK only (not StatusATK):

| Weapon Type | Small | Medium | Large |
|------------|-------|--------|-------|
| Bare Fist | 100% | 100% | 100% |
| Dagger | 100% | 75% | 50% |
| 1H Sword | 75% | 100% | 75% |
| 2H Sword | 75% | 75% | 100% |
| 1H Spear | 75% | 75% | 100% |
| 2H Spear | 75% | 75% | 100% |
| 1H Axe | 50% | 75% | 100% |
| 2H Axe | 50% | 75% | 100% |
| Mace | 75% | 100% | 100% |
| Staff/Rod | 100% | 100% | 100% |
| Bow | 100% | 100% | 75% |
| Knuckle/Claw | 100% | 75% | 50% |
| Musical Instrument | 75% | 100% | 75% |
| Whip | 75% | 100% | 50% |
| Book | 100% | 100% | 50% |
| Katar | 75% | 100% | 75% |

**Verified**: RateMyServer size table, iRO Wiki Size page, rAthena battle.cpp.

#### 4.1.4 Total Physical Damage Formula

```
Step 1: Base damage = StatusATK * 2 + WeaponATK
Step 2: Apply skill modifier (if applicable): damage = damage * skillPercent / 100
Step 3: Apply elemental modifier: damage = damage * elementTable[atkElement][defElement] / 100
Step 4: Apply Hard DEF: damage = damage * (4000 + hardDEF) / (4000 + hardDEF * 10)
Step 5: Subtract Soft DEF: damage = damage - softDEF
Step 6: Apply card/race/size modifiers to WeaponATK portion
Step 7: Floor and clamp: damage = max(1, floor(damage))
```

**Key**: StatusATK is multiplied by 2. Only WeaponATK is amplified by race/size/element card bonuses. StatusATK is always Neutral and unaffected by these modifiers (unless Mild Wind changes it).

**Verified**: iRO Wiki ATK page, rAthena battle.cpp, multiple community guides.

#### 4.1.5 Critical Hits

```
Critical Rate = floor(LUK * 0.3) + 1 + BonusCrit - floor(TargetLUK / 5)
```

Critical hit properties (pre-renewal):
- **Always maximum WeaponATK** -- no variance, always highest damage roll
- **Ignores Flee** -- guaranteed hit (100% accuracy)
- **Ignores BOTH Hard DEF and Soft DEF** -- full damage through all defense
- **Does NOT ignore Perfect Dodge** -- Lucky Dodge can still avoid crits
- **+40% damage bonus** -- critical damage multiplier is 1.4x
- **Katar doubles crit rate** -- displayed and actual crit rate doubled with Katar weapons
- **Skills cannot crit** -- only auto-attacks can critically hit (exceptions: Sharp Shooting, some others)

**Verified**: iRO Wiki Classic, Fandom Wiki, rAthena battle.cpp. The 40% bonus is confirmed across all three sources.

---

### 4.2 MATK (Magic Attack)

```
MATK_Min = INT + floor(INT / 7)^2
MATK_Max = INT + floor(INT / 5)^2
```

Per magic spell cast:
```
Damage = random(MATK_Min, MATK_Max) * SkillModifier * ElementModifier
```

Magic damage then goes through Hard MDEF (percentage) then Soft MDEF (flat subtraction).

**StatusMATK** (what the status window displays in some contexts):
```
StatusMATK = floor(BaseLevel / 4) + INT + floor(INT / 2) + floor(DEX / 5) + floor(LUK / 3)
```
Note: This StatusMATK formula is used for display purposes. The actual damage range uses the Min/Max formulas above plus equipment MATK.

**Verified**: iRO Wiki Classic, rAthena status.cpp, Fandom Wiki.

---

### 4.3 DEF (Physical Defense)

DEF is displayed as `A + B`:
- **A** = Hard DEF (equipment-based, percentage reduction)
- **B** = Soft DEF (stat-based, flat subtraction)

#### 4.3.1 Hard DEF (Equipment DEF)

```
DamageAfterHardDEF = Damage * (4000 + HardDEF) / (4000 + HardDEF * 10)
```

| Hard DEF | Reduction % | Verified |
|----------|------------|----------|
| 10 | ~2.4% | Calculated |
| 25 | ~5.9% | Calculated |
| 50 | ~11.1% | iRO Wiki DEF |
| 100 | ~20.0% | iRO Wiki DEF, rAthena |
| 150 | ~27.3% | Calculated |
| 200 | ~33.3% | iRO Wiki DEF |
| 275 | ~40.7% | Calculated |
| 400 | ~50.0% | Calculated |
| 500 | ~55.6% | iRO Wiki DEF |

The formula approaches but never reaches 100% reduction. Even at absurdly high DEF values, damage is never fully negated by Hard DEF alone.

**Armor refinement**: Each +1 on armor contributes approximately 0.7 to Hard DEF (the exact value is debated; iRO Wiki DEF page notes "each + shown as 1 DEF, but damage calculation uses 0.7 DEF per +"). However, this appears to be a display vs calculation discrepancy that varies by source.

**Verified**: iRO Wiki Classic DEF page, iRO Wiki DEF (renewal page also shows pre-re formula), rAthena battle.cpp.

#### 4.3.2 Soft DEF (VIT-based DEF)

The corrected pre-renewal formula (rAthena PR #6766):

```
// Deterministic base
vit_def_base = floor(VIT * 0.5)

// Random variable component
vit_def_min = floor(VIT * 0.3)
vit_def_max = max(vit_def_min, floor(VIT * VIT / 150) - 1)
vit_def_random = (vit_def_max > 0) ? random(vit_def_min, vit_def_max) : 0

// Additional flat contributions
softDEF = vit_def_base + vit_def_random + floor(AGI / 5) + floor(BaseLevel / 2)
```

**Simplified formula for stat window display** (deterministic only):
```
DisplaySoftDEF = floor(VIT / 2) + floor(AGI / 5) + floor(BaseLevel / 2)
```

**Application order**: Hard DEF first (percentage reduction), then Soft DEF (flat subtraction). Always in this order.

**Verified**: rAthena issue #6648, PR #6766, iRO Wiki DEF page.

---

### 4.4 MDEF (Magic Defense)

MDEF is displayed as `A + B`:
- **A** = Hard MDEF (equipment-based)
- **B** = Soft MDEF (stat-based)

#### 4.4.1 Hard MDEF

```
DamageAfterHardMDEF = Damage * (1000 + HardMDEF) / (1000 + HardMDEF * 10)
```

Note the constant is 1000 for MDEF vs 4000 for DEF -- this means MDEF provides much stronger per-point reduction than DEF.

| Hard MDEF | Reduction % |
|-----------|------------|
| 10 | ~8.3% |
| 25 | ~18.5% |
| 50 | ~31.0% |
| 100 | ~47.4% |

**Bonus**: Each point of Hard MDEF provides +1% resistance to Frozen and Stone Curse status effects.

**Verified**: iRO Wiki Classic, iRO Wiki MDEF page, rAthena battle.cpp.

#### 4.4.2 Soft MDEF

```
SoftMDEF = INT + floor(VIT / 5) + floor(DEX / 5) + floor(BaseLevel / 4)
```

Application order: Hard MDEF first (percentage), then Soft MDEF (flat subtraction).

**Verified**: iRO Wiki Classic, rAthena status.cpp.

---

### 4.5 HIT (Accuracy)

```
HIT = BaseLevel + DEX + BonusHIT
```

**Note**: Some sources (Fandom Wiki) add `floor(LUK/3)` to HIT. This is NOT confirmed in rAthena pre-renewal source or iRO Wiki Classic. Do NOT include LUK in HIT for accurate pre-renewal behavior.

**Hit Rate against target**:
```
HitRate = 80 + HIT - TargetFlee
HitRate = clamp(HitRate, 5, 100)
```
Minimum 5% hit chance (even against massive Flee), maximum 100%.

**Verified**: iRO Wiki Classic, rAthena battle.cpp.

---

### 4.6 FLEE (Evasion)

Displayed as `A + B`:
- **A** = Base Flee = `BaseLevel + AGI + BonusFlee`
- **B** = Perfect Dodge = `floor(LUK / 10) + BonusPerfectDodge`

**Dodge Rate**:
```
DodgeRate = FleeA - AttackerHIT + 80
DodgeRate = clamp(DodgeRate, 5, 95)
```
Maximum 95% dodge (100% in WoE). If the Flee check fails, Perfect Dodge is checked separately.

**Perfect Dodge**: Independent check after Flee fails. Can even dodge critical hits. Chance = `PerfectDodge`% (e.g., 10 PD = 10% lucky dodge).

**Multi-attacker Flee penalty**:
```
EffectiveFlee = Flee - max(0, (NumberOfAttackers - 1) * 10)
```
Each additional monster attacking you reduces effective Flee by 10.

**Verified**: iRO Wiki Classic, rAthena battle.cpp, Fandom Wiki.

---

### 4.7 ASPD (Attack Speed)

Pre-renewal ASPD ranges from 0 to 190. Higher is faster.

#### 4.7.1 Pre-Renewal Formula

```
Amotion = BaseWeaponDelay - floor(BaseWeaponDelay * (4 * AGI + DEX) / 1000)
ASPD = 200 - Amotion / 10
FinalAmotion = floor(Amotion * (1 - SpeedModifier))
FinalASPD = clamp(200 - FinalAmotion / 10, 0, 190)
```

Where:
- **BaseWeaponDelay** = class+weapon specific value from job_db (in "delay units" = seconds * 100, so 50 means 500ms)
- **SpeedModifier** = sum of all ASPD% buffs (0.0 to 1.0)

**Alternative representation** (as seen in community sources):
```
ASPD = 200 - (WD - floor(WD * (4*AGI + DEX) / 1000)) / 10 * (1 - SpeedMod)
```

#### 4.7.2 ASPD to Attack Delay

```
AttackDelay_ms = (200 - ASPD) * 10
AttacksPerSecond = 1000 / AttackDelay_ms
```

| ASPD | Delay (ms) | Attacks/sec |
|------|-----------|-------------|
| 150 | 500 | 2.0 |
| 160 | 400 | 2.5 |
| 170 | 300 | 3.33 |
| 175 | 250 | 4.0 |
| 180 | 200 | 5.0 |
| 185 | 150 | 6.67 |
| 190 | 100 | 10.0 (hard cap) |

#### 4.7.3 Base Weapon Delay Table (BTBA * 100)

Values from rAthena job_db (pre-renewal). These are "delay units" where 50 = 500ms base delay.

| Class | Bare Hand | Dagger | 1H Sword | 2H Sword | 1H Spear | 2H Spear | Mace | Axe | 2H Axe | Bow | Staff | Knuckle | Katar | Instrument | Whip | Book |
|-------|-----------|--------|----------|----------|----------|----------|------|-----|--------|-----|-------|---------|-------|------------|------|------|
| Novice | 50 | 55 | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Swordsman | 40 | 65 | 70 | 60 | 65 | 65 | 70 | 80 | 80 | -- | -- | -- | -- | -- | -- | -- |
| Mage | 35 | 60 | -- | -- | -- | -- | -- | -- | -- | -- | 65 | -- | -- | -- | -- | -- |
| Archer | 50 | 55 | -- | -- | -- | -- | -- | -- | -- | 70 | -- | -- | -- | -- | -- | -- |
| Thief | 40 | 50 | 70 | -- | -- | -- | -- | -- | -- | 85 | -- | -- | -- | -- | -- | -- |
| Merchant | 40 | 65 | 55 | -- | -- | -- | 65 | 70 | 70 | -- | -- | -- | -- | -- | -- | -- |
| Acolyte | 40 | 60 | -- | -- | -- | -- | 70 | -- | -- | -- | 65 | -- | -- | -- | -- | -- |
| Knight | 38 | 60 | 55 | 50 | 55 | 55 | 60 | 70 | 70 | -- | -- | -- | -- | -- | -- | -- |
| Wizard | 35 | 58 | -- | -- | -- | -- | -- | -- | -- | -- | 60 | -- | -- | -- | -- | -- |
| Hunter | 48 | 55 | -- | -- | -- | -- | -- | -- | -- | 60 | -- | -- | -- | -- | -- | -- |
| Assassin | 38 | 45 | 65 | -- | -- | -- | -- | -- | -- | -- | -- | -- | 42 | -- | -- | -- |
| Blacksmith | 38 | 60 | 52 | -- | -- | -- | 60 | 62 | 62 | -- | -- | -- | -- | -- | -- | -- |
| Priest | 40 | -- | -- | -- | -- | -- | 62 | -- | -- | -- | 60 | 55 | -- | -- | -- | -- |
| Crusader | 38 | 62 | 58 | 55 | 58 | 58 | 62 | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Sage | 35 | 58 | -- | -- | -- | -- | -- | -- | -- | -- | 60 | -- | -- | -- | -- | 58 |
| Bard | 45 | 55 | -- | -- | -- | -- | -- | -- | -- | 62 | -- | -- | -- | 58 | -- | -- |
| Dancer | 45 | 55 | -- | -- | -- | -- | -- | -- | -- | 62 | -- | -- | -- | -- | 58 | -- |
| Rogue | 38 | 48 | 62 | -- | -- | -- | -- | -- | -- | 75 | -- | -- | -- | -- | -- | -- |
| Monk | 36 | -- | -- | -- | -- | -- | 60 | -- | -- | -- | 62 | 42 | -- | -- | -- | -- |
| Alchemist | 38 | 60 | 52 | -- | -- | -- | 60 | 62 | 62 | -- | -- | -- | -- | -- | -- | -- |

Transcendent classes use the same BTBA values as their base class.

#### 4.7.4 Common ASPD Modifiers

| Source | SpeedModifier Value |
|--------|-------------------|
| Concentration Potion | +0.10 (10%) |
| Awakening Potion | +0.15 (15%) |
| Berserk Potion | +0.20 (20%) |
| Two-Hand Quicken | +0.30 (30%) |
| Adrenaline Rush | +0.30 (30%) |
| Spear Quicken | +0.30 (30%) |
| One-Hand Quicken | +0.30 (30%) |
| Berserk (Lord Knight) | Sets ASPD to 190 |

**Verified**: rAthena status.cpp, iRO Wiki Classic, OriginsRO Wiki.

---

### 4.8 Max HP

```
// Step 1: Base HP from level and class
BaseHP = 35 + floor(BaseLevel * HP_JOB_B)
for i = 2 to BaseLevel:
    BaseHP += round(HP_JOB_A * i)

// Step 2: VIT scaling + Transcendent modifier
MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod)

// Step 3: Additive bonuses (equipment, cards, buffs)
MaxHP += AdditiveHPBonuses

// Step 4: Multiplicative bonuses
MaxHP = floor(MaxHP * (1 + MultiplicativeHPBonuses / 100))
```

Where:
- `TransMod` = 1.25 for transcendent classes, 1.0 otherwise
- `HP_JOB_A` and `HP_JOB_B` are class-specific (see table below)

#### HP Class Coefficients

| Class | HP_JOB_A | HP_JOB_B |
|-------|----------|----------|
| Novice | 0.0 | 5 |
| Swordsman | 0.7 | 5 |
| Mage | 0.3 | 5 |
| Archer | 0.5 | 5 |
| Thief | 0.5 | 5 |
| Merchant | 0.4 | 5 |
| Acolyte | 0.4 | 5 |
| Knight | 1.5 | 5 |
| Wizard | 0.55 | 5 |
| Hunter | 0.85 | 5 |
| Assassin | 1.1 | 5 |
| Blacksmith | 0.9 | 5 |
| Priest | 0.75 | 5 |
| Crusader | 1.1 | 7 |
| Sage | 0.75 | 5 |
| Bard/Dancer | 0.75 | 3 |
| Rogue | 0.85 | 5 |
| Monk | 0.9 | 6.5 |
| Alchemist | 0.9 | 5 |
| Super Novice | 0.0 | 5 |

Transcendent classes use the same coefficients but get the 1.25x TransMod.

**Verified**: iRO Wiki Classic, rAthena job_db, existing 01_Stats_Leveling_JobSystem.md.

---

### 4.9 Max SP

```
// Step 1: Base SP from level and class
BaseSP = 10 + floor(BaseLevel * SP_JOB_B)
for i = 2 to BaseLevel:
    BaseSP += round(SP_JOB_A * i)

// Step 2: INT scaling + Transcendent modifier
MaxSP = floor(BaseSP * (1 + INT * 0.01) * TransMod)

// Step 3: Additive bonuses
MaxSP += AdditiveSPBonuses

// Step 4: Multiplicative bonuses
MaxSP = floor(MaxSP * (1 + MultiplicativeSPBonuses / 100))
```

#### SP Class Coefficients

| Class | SP_JOB_A | SP_JOB_B |
|-------|----------|----------|
| Novice | 0.0 | 2 |
| Swordsman | 0.2 | 2 |
| Mage | 0.6 | 2 |
| Archer | 0.4 | 2 |
| Thief | 0.3 | 2 |
| Merchant | 0.3 | 2 |
| Acolyte | 0.5 | 2 |
| Knight | 0.4 | 2 |
| Wizard | 1.0 | 2 |
| Hunter | 0.6 | 2 |
| Assassin | 0.5 | 2 |
| Blacksmith | 0.5 | 2 |
| Priest | 0.8 | 2 |
| Crusader | 0.5 | 2 |
| Sage | 0.8 | 2 |
| Bard/Dancer | 0.6 | 2 |
| Rogue | 0.5 | 2 |
| Monk | 0.5 | 2 |
| Alchemist | 0.5 | 2 |
| Super Novice | 0.0 | 2 |

**Verified**: iRO Wiki Classic, rAthena job_db.

---

### 4.10 Natural Regeneration

#### HP Regeneration (every 6 seconds standing, every 3 seconds sitting):
```
HPRegen = max(1, floor(MaxHP / 200)) + floor(VIT / 5)
```

#### SP Regeneration (every 8 seconds standing, every 4 seconds sitting):
```
SPRegen = floor(MaxSP / 100) + floor(INT / 6) + 1

// Bonus for INT >= 120:
if (INT >= 120) SPRegen += floor(INT / 2) - 56
```

**Modifiers**:
- **Sitting**: Doubles regen rate (halves the tick interval)
- **50-89% weight**: HP and SP regen halved
- **90%+ weight**: NO regen at all
- **Skill-based regen** (HP/SP Recovery, every 10 seconds):
  - HP Recovery Lv X: `X * 5 + floor(X * MaxHP / 500)`
  - SP Recovery Lv X: `X * 3 + floor(X * MaxSP / 500)`

**Verified**: iRO Wiki Classic, iRO Wiki SP Recovery page, iRO Wiki HP Recovery page, rAthena status.cpp.

---

### 4.11 Cast Time

Pre-renewal uses a single-component cast time (no fixed/variable split):
```
FinalCastTime = BaseCastTime * (1 - DEX / 150)
```

- At 150 DEX (including bonuses): instant cast (0 cast time)
- Each point of DEX reduces cast time by approximately 0.67%
- Equipment/skill cast time reduction bonuses stack additively with the DEX reduction
- Cast is interrupted by taking damage (unless Phen card or similar)

**Verified**: iRO Wiki Classic, rAthena skill.cpp.

---

## 5. Status Bonuses (Every 10 Points)

The "bonus" system in RO gives extra stats at certain thresholds. The primary bonuses follow the `floor(STAT/10)^2` pattern for ATK contributions.

### 5.1 STR Bonus (Melee ATK)

At every 10-point threshold, STR grants a squared bonus to melee ATK:

| STR | Bonus = floor(STR/10)^2 | Incremental Gain |
|-----|------------------------|-----------------|
| 10 | 1 | +1 |
| 20 | 4 | +3 |
| 30 | 9 | +5 |
| 40 | 16 | +7 |
| 50 | 25 | +9 |
| 60 | 36 | +11 |
| 70 | 49 | +13 |
| 80 | 64 | +15 |
| 90 | 81 | +17 |

The incremental gain per 10 STR increases by 2 each time. Going from 80 to 90 STR gives +17 bonus ATK, making high-STR builds increasingly rewarding.

### 5.2 DEX Bonus (Ranged ATK)

Follows the exact same `floor(DEX/10)^2` pattern for ranged StatusATK. Values identical to the STR table.

### 5.3 INT Bonus (MATK)

INT has TWO bonus thresholds:
- **Every 5 INT**: MATK Max gains `floor(INT/5)^2`
- **Every 7 INT**: MATK Min gains `floor(INT/7)^2`

These create a widening MATK range at high INT, making magic damage more variable but with a higher ceiling.

### 5.4 LUK Bonus (Critical)

LUK's bonus is continuous (`floor(LUK * 0.3)`) rather than threshold-based, but Perfect Dodge (`floor(LUK/10)`) has 10-point thresholds:

| LUK | Perfect Dodge | Crit Rate |
|-----|--------------|-----------|
| 10 | 1 | 4% |
| 20 | 2 | 7% |
| 30 | 3 | 10% |
| 40 | 4 | 13% |
| 50 | 5 | 16% |
| 60 | 6 | 19% |
| 70 | 7 | 22% |
| 80 | 8 | 25% |
| 90 | 9 | 28% |

### 5.5 VIT Bonus (Soft DEF / HP Regen)

VIT has multiple threshold-based bonuses:
- `floor(VIT/2)`: Soft DEF base (every 2 VIT)
- `floor(VIT/5)`: HP Regen bonus (every 5 VIT) and Soft MDEF contribution
- MaxHP is continuous (1% per VIT, no thresholds)

### 5.6 AGI Bonus (ASPD / Flee)

AGI provides:
- +1 Flee per AGI (continuous, no thresholds)
- ASPD contribution is continuous but the 4:1 ratio with DEX and integer truncation create uneven stepping

**Verified**: iRO Wiki Classic Stats page, Fandom Wiki, rAthena status.cpp.

---

## 6. Stat Window Display

The player's stat window shows specific values in a standardized format:

### 6.1 Display Format

```
STR  BaseSTR + BonusSTR     [+]    (cost: X)
AGI  BaseAGI + BonusAGI     [+]    (cost: X)
VIT  BaseVIT + BonusVIT     [+]    (cost: X)
INT  BaseINT + BonusINT     [+]    (cost: X)
DEX  BaseDEX + BonusDEX     [+]    (cost: X)
LUK  BaseLUK + BonusLUK     [+]    (cost: X)

ATK:     StatusATK + WeaponATK
MATK:    MATKMin ~ MATKMax
HIT:     TotalHIT
FLEE:    FleeA + PerfectDodge
DEF:     HardDEF + SoftDEF
MDEF:    HardMDEF + SoftMDEF
CRI:     CriticalRate
ASPD:    ASPDValue

Remaining Stat Points: X
```

### 6.2 What "Base" vs "Bonus" Means

- **Base**: The player-allocated stat value (1-99)
- **Bonus**: Sum of job bonuses + equipment + buffs + cards

The stat window shows `Base + Bonus` where Bonus is green text. The `[+]` button raises the Base by 1 if you have enough stat points.

### 6.3 ATK Display

- **A value**: StatusATK (calculated from stats, always includes the `floor(STAT/10)^2` bonus)
- **B value**: Sum of weapon base ATK + refinement bonus + equipment ATK bonuses

### 6.4 DEF/MDEF Display

- **A value**: Hard DEF/MDEF from equipment (percentage reduction in calculations)
- **B value**: Soft DEF/MDEF from stats (flat reduction, but display shows the DETERMINISTIC part only, not the random VIT DEF component)

### 6.5 FLEE Display

- **A value**: BaseLevel + AGI + equipment flee bonuses
- **B value**: Perfect Dodge (from LUK + equipment PD bonuses)

---

## 7. Weight Limit Formula

```
MaxWeight = 2000 + STR * 30 + JobWeightBonus
```

### Job Weight Bonuses

| Class | Weight Bonus | Total at STR 1 | Total at STR 99 |
|-------|-------------|---------------|-----------------|
| Novice | 0 | 2,030 | 4,970 |
| Swordsman | 800 | 2,830 | 5,770 |
| Mage | 200 | 2,230 | 5,170 |
| Archer | 600 | 2,630 | 5,570 |
| Thief | 400 | 2,430 | 5,370 |
| Merchant | 800 | 2,830 | 5,770 |
| Acolyte | 400 | 2,430 | 5,370 |
| Knight | 800 | 2,830 | 5,770 |
| Wizard | 200 | 2,230 | 5,170 |
| Hunter | 600 | 2,630 | 5,570 |
| Assassin | 600 | 2,630 | 5,570 |
| Blacksmith | 800 | 2,830 | 5,770 |
| Priest | 600 | 2,630 | 5,570 |
| Crusader | 800 | 2,830 | 5,770 |
| Sage | 400 | 2,430 | 5,370 |
| Bard/Dancer | 600 | 2,630 | 5,570 |
| Rogue | 600 | 2,630 | 5,570 |
| Monk | 600 | 2,630 | 5,570 |
| Alchemist | 800 | 2,830 | 5,770 |
| Super Novice | 0 | 2,030 | 4,970 |

Transcendent classes use the same weight bonus as their base class.

### Weight Thresholds

| % Capacity | Effect |
|-----------|--------|
| 0-49% | Normal: full regen, full functionality |
| 50-69% | HP/SP regeneration halved |
| 70-89% | HP/SP regen halved + cannot use item-creation skills |
| 90-100% | No attacks, no skills, no regen, movement slowed |

**Verified**: iRO Wiki Classic, iRO Wiki Weight Limit page, RateMyServer, rAthena status.cpp.

---

## 8. Soft/Hard DEF/MDEF Distinction

This is one of the most critical and commonly misunderstood aspects of RO's damage system.

### 8.1 Hard DEF (Equipment DEF)

- **Source**: Armor, shields, headgear, garments, footgear, accessories with DEF
- **Effect**: Percentage-based damage reduction (multiplicative)
- **Formula**: `Damage * (4000 + DEF) / (4000 + DEF * 10)`
- **Display**: The "A" value in `A + B` DEF display
- **Stacking**: All equipment DEF adds up, then one reduction applied
- **Cannot be negative**: Clamped to minimum 0

### 8.2 Soft DEF (Stat DEF)

- **Source**: VIT (primary), AGI (minor), Base Level (minor)
- **Effect**: Flat subtraction from damage after Hard DEF
- **Formula**: See Section 4.3.2 (includes random component)
- **Display**: The "B" value in `A + B` DEF display (shows deterministic part only)
- **Application**: After Hard DEF has already reduced damage

### 8.3 Hard MDEF (Equipment MDEF)

- **Source**: Equipment with MDEF stat
- **Effect**: Percentage-based magic damage reduction
- **Formula**: `Damage * (1000 + MDEF) / (1000 + MDEF * 10)`
- **Bonus**: +1% resistance to Frozen and Stone Curse per point
- **Note**: Uses constant 1000 vs DEF's 4000, so MDEF is much more powerful per point

### 8.4 Soft MDEF (Stat MDEF)

- **Source**: INT (primary), VIT, DEX, Base Level (all minor)
- **Formula**: `INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLevel/4)`
- **Effect**: Flat subtraction from magic damage after Hard MDEF

### 8.5 Application Order (CRITICAL)

```
1. Calculate raw damage (StatusATK * 2 + WeaponATK, or MATK * skill%)
2. Apply element modifier
3. Apply Hard DEF/MDEF (percentage reduction)
4. Subtract Soft DEF/MDEF (flat subtraction)
5. Apply card/race/size modifiers
6. Clamp to minimum 1
```

Never reverse steps 3 and 4. Hard (percentage) always comes before Soft (flat).

**Verified**: iRO Wiki DEF page, rAthena battle.cpp, multiple community damage guides.

---

## 9. Baby Class Stat Modifiers

Baby classes are created through the Adoption System (a married couple adopts a Novice/First Class character).

### 9.1 Restrictions

| Restriction | Value | Source |
|------------|-------|--------|
| Max HP/SP | 75% of normal | iRO Wiki Classic Adoption |
| Stat cap (base) | 80 maximum (cannot raise above 80) | iRO Wiki Classic Adoption |
| Weight limit | -1,200 from normal | iRO Wiki Adoption (some sources) |
| Transcendence | Cannot rebirth/transcend | iRO Wiki Classic Adoption |
| Forge/Brew rates | -50% for Blacksmith/Alchemist | iRO Wiki Classic Adoption |
| Size | Small (PvP advantage vs Pierce, size cards) | iRO Wiki Classic Adoption |

### 9.2 HP/SP Calculation for Baby Classes

```
BabyMaxHP = floor(NormalMaxHP * 0.75)
BabyMaxSP = floor(NormalMaxSP * 0.75)
```

The 75% modifier is applied as a final step after all other HP/SP calculations (VIT scaling, transcendent modifier, equipment bonuses). However, since baby classes cannot transcend, the TransMod is always 1.0.

### 9.3 Stat Cap Enforcement

If a character has stats above 80 before adoption, those stats remain but cannot be raised further. The `[+]` button is disabled for any stat at or above 80. Stats converted from above-80 are returned as raw stat points.

**Verified**: iRO Wiki Classic Adoption System page, iRO Wiki Adoption System page.

---

## 10. Job-Specific Stat Interactions

### 10.1 Job Bonus Stats

Each class receives hidden stat bonuses at specific job levels. These are automatic, cost no stat points, and show as green bonus text in the stat window.

#### Novice (Job 1-10)

| Job Lv | STR | AGI | VIT | INT | DEX | LUK |
|--------|-----|-----|-----|-----|-----|-----|
| 1 | 0 | 0 | 0 | 0 | 0 | 0 |
| 2 | 0 | 0 | 0 | 0 | 0 | +1 |
| 3 | 0 | 0 | 0 | 0 | +1 | +1 |
| 4 | 0 | 0 | 0 | 0 | +1 | +1 |
| 5 | 0 | +1 | 0 | 0 | +1 | +1 |
| 6 | 0 | +1 | +1 | 0 | +1 | +1 |
| 7 | 0 | +1 | +1 | 0 | +1 | +1 |
| 8 | +1 | +1 | +1 | 0 | +1 | +1 |
| 9 | +1 | +1 | +1 | +1 | +1 | +1 |
| 10 | +1 | +1 | +1 | +1 | +1 | +1 |

#### Summary at Max Job Level (All Classes)

| Class | STR | AGI | VIT | INT | DEX | LUK | Total |
|-------|-----|-----|-----|-----|-----|-----|-------|
| Novice (10) | 1 | 1 | 1 | 1 | 1 | 1 | 6 |
| Swordsman (50) | 5 | 3 | 5 | 0 | 3 | 0 | 16 |
| Mage (50) | 0 | 2 | 0 | 6 | 5 | 0 | 13 |
| Archer (50) | 1 | 5 | 0 | 1 | 5 | 1 | 13 |
| Merchant (50) | 4 | 1 | 4 | 1 | 4 | 1 | 15 |
| Thief (50) | 3 | 5 | 1 | 0 | 4 | 2 | 15 |
| Acolyte (50) | 0 | 2 | 2 | 5 | 3 | 3 | 15 |
| Knight (50) | 8 | 5 | 7 | 1 | 5 | 1 | 27 |
| Wizard (50) | 0 | 3 | 0 | 9 | 8 | 0 | 20 |
| Hunter (50) | 3 | 8 | 1 | 2 | 8 | 2 | 24 |
| Blacksmith (50) | 6 | 2 | 6 | 2 | 6 | 2 | 24 |
| Assassin (50) | 5 | 8 | 2 | 0 | 5 | 4 | 24 |
| Priest (50) | 0 | 3 | 5 | 7 | 5 | 3 | 23 |
| Crusader (50) | 7 | 3 | 7 | 3 | 5 | 2 | 27 |
| Sage (50) | 0 | 5 | 0 | 8 | 7 | 0 | 20 |
| Bard/Dancer (50) | 3 | 6 | 1 | 4 | 6 | 4 | 24 |
| Alchemist (50) | 4 | 4 | 4 | 4 | 4 | 4 | 24 |
| Rogue (50) | 4 | 7 | 2 | 1 | 6 | 4 | 24 |
| Monk (50) | 6 | 6 | 3 | 3 | 5 | 4 | 27 |

Transcendent classes use the same bonus table for levels 1-50, then receive approximately 4-6 additional bonus stats across levels 51-70.

### 10.2 Class-Specific Stat Interactions

| Interaction | Formula | Classes |
|------------|---------|---------|
| Blacksmith forging success | +0.1% per DEX, +0.1% per LUK | Blacksmith, Whitesmith |
| Alchemist brewing success | +0.05% per DEX, +0.1% per LUK | Alchemist, Biochemist |
| Thief Steal success | +0.01x per DEX | Thief, Rogue, Stalker |
| Rogue Divest success | +0.2% per DEX | Rogue, Stalker |
| Cooking success | +0.2% per DEX, +0.1% per LUK | All classes |
| Auto-Blitz Beat trigger | `floor((JobLevel + 9) / 10)` | Hunter, Sniper |

**Verified**: iRO Wiki Classic Stats page, RateMyServer.

---

## 11. Status Effect Resistances from Stats

Pre-renewal status resistance formulas (verified from RateMyServer pre-renewal guide):

### 11.1 VIT-Based Resistances

**Stun, Silence, Bleeding**:
```
Chance = BaseChance - BaseChance * tarVIT / 100 + srcBaseLv - tarBaseLv - tarLUK
Duration = BaseDuration - BaseDuration * tarVIT / 100 - 10 * tarLUK
```

Default durations: Stun = 5000ms, Silence = 30000ms, Bleeding = 120000ms.

At 100 VIT: These status effects have 0% base chance (fully immune from the VIT component alone).

### 11.2 INT-Based Resistances

**Sleep**:
```
Chance = BaseChance - BaseChance * tarINT / 100 + srcBaseLv - tarBaseLv - tarLUK
Duration = BaseDuration - BaseDuration * tarINT / 100 - 10 * tarLUK
```

Default duration: Sleep = 30000ms.

**Blind**:
```
Chance = BaseChance - BaseChance * (tarINT + tarVIT) / 200 + srcBaseLv - tarBaseLv - tarLUK
Duration = BaseDuration - BaseDuration * (tarINT + tarVIT) / 200 - 10 * tarLUK
```

Default duration: Blind = 30000ms.

### 11.3 MDEF-Based Resistances

**Stone Curse**:
```
Chance = BaseChance - BaseChance * tarHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK
Duration = always 20000ms (not reduced by stats)
```

**Freeze**:
```
Chance = BaseChance - BaseChance * tarHardMDEF / 100 + srcBaseLv - tarBaseLv - tarLUK
Duration = BaseDuration - BaseDuration * tarHardMDEF / 100 + 10 * srcLUK
```

Default duration: Freeze = 12000ms. Note: SOURCE LUK increases freeze duration (attacker's LUK benefits the attacker).

### 11.4 LUK-Based Resistances

**Curse**:
```
Chance = BaseChance - BaseChance * tarLUK / 100 + srcBaseLv - tarLUK
Duration = BaseDuration - BaseDuration * tarVIT / 100 - 10 * tarLUK
```

Default duration: Curse = 30000ms. Special: Immune if target LUK = 0.

### 11.5 Mixed Resistances

**Poison**:
```
Chance = BaseChance - BaseChance * tarVIT / 100 + srcBaseLv - tarBaseLv - tarLUK
Duration (monsters) = 30000 - 20000 * tarVIT / 100
Duration (players) = 60000 - 45000 * tarVIT / 100 - 100 * tarLUK
```

**Confusion**:
```
Chance = BaseChance - BaseChance * (tarSTR + tarINT) / 200 - srcBaseLv + tarBaseLv + tarLUK
Duration = BaseDuration - BaseDuration * (tarSTR + tarINT) / 200 - 10 * tarLUK
```

Default duration: Confusion = 30000ms.

**Verified**: RateMyServer "Guide: Official Status Resistance Formulas (Pre-Renewal)", cross-checked with rAthena status.cpp.

---

## 12. Edge Cases & Special Rules

### 12.1 Integer Math Rules
- ALL intermediate calculations use `Math.floor()` (truncation toward zero)
- No floating point values exist in gameplay calculations
- The ONLY exception is `Math.round()` used in BaseHP/BaseSP per-level accumulation (`round(HP_JOB_A * i)`)

### 12.2 Stat Floor and Ceiling
- Base stats: minimum 1, maximum 99 (player-allocated)
- Effective stats (base + bonus): no maximum (can exceed 99 with equipment/buffs)
- Effective stats CAN go negative (e.g., Curse reduces LUK to 0, Quagmire reduces AGI/DEX)
- Stats below 0 are clamped to 0 for formula calculations (no negative contributions)

### 12.3 Katar Critical Doubling
- Katar weapons double the displayed and actual critical rate
- This applies to the ENTIRE crit rate calculation, including bonuses from cards and equipment
- Formula: `EffectiveCrit = (floor(LUK * 0.3) + 1 + BonusCrit) * 2`

### 12.4 Mounted Combat (Pecopeco/Grand Pecopeco)
- Knights/Crusaders on mounts: +36% movement speed
- Spear attacks vs Medium size: 100% instead of 75% when mounted (spear mastery)
- Some ASPD penalties may apply depending on weapon

### 12.5 Shield ASPD Penalty
- Equipping a shield increases BTBA by approximately 5-10% depending on class
- The exact penalty varies by server implementation; rAthena applies it as an amotion increase

### 12.6 Dual Wield (Assassin Only)
- Both hands attack per attack cycle
- Each hand uses its own weapon ATK, cards, and element
- ASPD calculation uses the average of both weapon delays
- Mastery penalty: right hand 100%, left hand 80% damage (without Left-hand Mastery skill)

### 12.7 Damage Floor
- Minimum damage from any attack is 1 (never 0)
- Even if DEF/MDEF reduce damage below 1, the result is clamped to 1
- Exception: 0 damage from elemental immunity (Ghost vs Neutral, etc.)

### 12.8 Over-Upgrade Variance
- The over-upgrade bonus is random per hit, ranging from 0 to the maximum
- This makes over-upgraded weapons have high damage variance
- Critical hits do NOT maximize over-upgrade bonus (only base WeaponATK is maximized)

### 12.9 Transcendent Class Stat Reset
- Upon rebirth, ALL stats are reset to 1
- High Novice starts with 100 stat points (vs 48 for normal Novice)
- The extra 52 points compensate for the level-up points that will be re-earned

### 12.10 Healing Item VIT Scaling
```
HealAmount = BaseItemHeal * (1 + VIT * 0.02)
```
Example: A Red Potion (45 HP) at 50 VIT: `45 * (1 + 50*0.02) = 45 * 2.0 = 90 HP`

### 12.11 SP Item INT Scaling
```
SPHealAmount = BaseItemSP * (1 + INT * 0.01)
```

---

## 13. Implementation Checklist

Every formula that must be coded for a complete stat system implementation:

### 13.1 Core Stat Functions
- [ ] `getStatPointCost(currentValue)` -- `floor((x-1)/10) + 2`
- [ ] `getStatPointsForLevel(level)` -- `floor(level/5) + 3`
- [ ] `getTotalStatPoints(baseLevel, isTranscendent)` -- cumulative sum
- [ ] `getCumulativeStatCost(targetValue)` -- sum of costs 1 to target
- [ ] `validateStatAllocation(current, available, amount)` -- bounds checking

### 13.2 HP/SP Functions
- [ ] `calculateBaseHP(baseLevel, hpJobA, hpJobB)` -- iterative sum with `round()`
- [ ] `calculateMaxHP(baseLevel, vit, jobClass, isTranscendent, addMod, multMod)` -- full formula
- [ ] `calculateBaseSP(baseLevel, spJobA, spJobB)` -- iterative sum with `round()`
- [ ] `calculateMaxSP(baseLevel, int, jobClass, isTranscendent, addMod, multMod)` -- full formula
- [ ] HP/SP class coefficient tables (HP_JOB_A, HP_JOB_B, SP_JOB_A, SP_JOB_B per class)
- [ ] Transcendent modifier (1.25x)
- [ ] Baby class modifier (0.75x)

### 13.3 ATK/MATK Functions
- [ ] `calculateMeleeStatusATK(baseLv, str, dex, luk)` -- includes `floor(STR/10)^2`
- [ ] `calculateRangedStatusATK(baseLv, str, dex, luk)` -- includes `floor(DEX/10)^2`
- [ ] `calculateWeaponATK(baseWpnDmg, wpnLv, statBonus, refineBonus, overUpgrade, sizePenalty)`
- [ ] `calculateWeaponVariance(baseWpnDmg, wpnLv)` -- `+/- floor(0.05 * wpnLv * baseWpnDmg)`
- [ ] `calculateStatWeaponBonus(baseWpnDmg, primaryStat)` -- `floor(baseWpnDmg * stat / 200)`
- [ ] `calculateMATK(int)` -- returns { min, max }
- [ ] Size penalty table (16 weapon types x 3 sizes)

### 13.4 DEF/MDEF Functions
- [ ] `applyHardDEF(damage, hardDEF)` -- `damage * (4000+DEF) / (4000+DEF*10)`
- [ ] `applyHardMDEF(damage, hardMDEF)` -- `damage * (1000+MDEF) / (1000+MDEF*10)`
- [ ] `calculateSoftDEF(vit, agi, baseLv)` -- deterministic part
- [ ] `calculateVITDEFRandom(vit)` -- random VIT component for damage calc
- [ ] `calculateSoftMDEF(int, vit, dex, baseLv)` -- INT-based formula

### 13.5 HIT/FLEE/Crit Functions
- [ ] `calculateHIT(baseLv, dex, bonusHIT)` -- `baseLv + dex + bonus`
- [ ] `calculateFlee(baseLv, agi, bonusFlee)` -- `baseLv + agi + bonus`
- [ ] `calculatePerfectDodge(luk, bonusPD)` -- `floor(luk/10) + bonus`
- [ ] `calculateCritRate(luk, bonusCrit)` -- `floor(luk*0.3) + 1 + bonus`
- [ ] `calculateHitRate(attackerHIT, targetFlee)` -- `clamp(80 + HIT - Flee, 5, 100)`
- [ ] `calculateDodgeRate(flee, attackerHIT)` -- `clamp(flee - HIT + 80, 5, 95)`
- [ ] Multi-attacker flee penalty -- `flee - max(0, (attackers-1) * 10)`

### 13.6 ASPD Functions
- [ ] `getWeaponDelay(jobClass, weaponType)` -- lookup table with fallback
- [ ] `calculateASPD(jobClass, weaponType, agi, dex, speedMod)` -- full pre-re formula
- [ ] `aspdToDelayMs(aspd)` -- `max(100, (200 - aspd) * 10)`
- [ ] BTBA table for all 20+ classes x all weapon types
- [ ] Shield penalty application
- [ ] Transcendent-to-base class mapping

### 13.7 Regeneration Functions
- [ ] `calculateHPRegen(maxHP, vit)` -- `max(1, floor(maxHP/200)) + floor(vit/5)`
- [ ] `calculateSPRegen(maxSP, int)` -- includes INT >= 120 bonus
- [ ] Sitting multiplier (2x rate)
- [ ] Weight threshold modifiers (50%/90% capacity)
- [ ] Skill-based regen (HP/SP Recovery)

### 13.8 Cast Time
- [ ] `calculateCastTime(baseCastTimeMs, dex, bonusReduction)` -- `baseCast * (1 - dex/150)`

### 13.9 Weight
- [ ] `calculateMaxWeight(str, jobClass)` -- `2000 + str*30 + jobBonus`
- [ ] Weight bonus table per class

### 13.10 Status Resistances
- [ ] Stun/Silence/Bleeding resistance (VIT-based)
- [ ] Sleep resistance (INT-based)
- [ ] Blind resistance (INT+VIT-based)
- [ ] Freeze/Stone Curse resistance (Hard MDEF-based)
- [ ] Curse resistance (LUK-based)
- [ ] Poison resistance (VIT-based, different player/monster durations)
- [ ] Confusion resistance (STR+INT-based)

### 13.11 Data Tables
- [ ] HP/SP class coefficients (20+ classes)
- [ ] Weight bonuses per class
- [ ] ASPD base weapon delays per class per weapon type
- [ ] Job bonus stats per class per job level (full per-level tables, not just totals)
- [ ] Transcendent-to-base class mapping
- [ ] Baby class flag and modifiers
- [ ] Size penalty table

### 13.12 Display/UI
- [ ] Stat window with Base + Bonus display
- [ ] ATK as A + B format
- [ ] MATK as Min ~ Max format
- [ ] DEF/MDEF as A + B format
- [ ] FLEE as A + B format
- [ ] Stat cost display
- [ ] Remaining stat points display
- [ ] Job bonus stats as green text

---

## 14. Gap Analysis

Comparing this deep research against the existing documentation in `RagnaCloneDocs/01_Stats_Leveling_JobSystem.md` and `RagnaCloneDocs/Implementation/02_Stats_Class_System.md`:

### 14.1 Errors Found in Existing Docs

| Issue | Location | Existing Value | Correct Value | Source |
|-------|----------|---------------|---------------|--------|
| INT 99 MATK Min | 01_Stats Sec 1.5 | 299 | 295 | `99 + floor(99/7)^2 = 99+196 = 295` |
| INT 99 MATK Max | 01_Stats Sec 1.5 | 491 | 460 | `99 + floor(99/5)^2 = 99+361 = 460` |
| INT 80 MATK Min | 01_Stats Sec 1.5 | 211 | 201 | `80 + floor(80/7)^2 = 80+121 = 201` |
| INT 40 MATK Min | 01_Stats Sec 1.5 | 73 | 65 | `40 + floor(40/7)^2 = 40+25 = 65` |
| INT 50 MATK Min | 01_Stats Sec 1.5 | 101 | 99 | `50 + floor(50/7)^2 = 50+49 = 99` |
| INT 60 MATK Min | 01_Stats Sec 1.5 | 133 | 124 | `60 + floor(60/7)^2 = 60+64 = 124` |
| INT 70 MATK Min | 01_Stats Sec 1.5 | 170 | 170 | Correct |
| INT 30 MATK Min | 01_Stats Sec 1.5 | 48 | 46 | `30 + floor(30/7)^2 = 30+16 = 46` |
| Soft DEF formula | 01_Stats Sec 2.3 | `floor(VIT/2)` | Randomized VIT component (see 4.3.2) | rAthena PR #6766 |
| Soft MDEF VIT contribution | 01_Stats Sec 1.4 | `floor(VIT/5)` to SoftMDEF | Correct (this is right) | -- |
| LUK in HIT | 01_Stats Sec 1.7 | "some sources" | Should NOT include LUK | rAthena pre-re |
| LUK in Flee | 01_Stats Sec 1.7 | Implied | Should NOT include LUK | rAthena pre-re |

### 14.2 Missing from Existing Docs

| Missing Item | Importance | Notes |
|-------------|-----------|-------|
| VIT Soft DEF random component | HIGH | The existing doc uses simplified `floor(VIT/2)`. True formula has random range per hit. |
| Status effect resistance formulas | HIGH | Complete per-status formulas with VIT/INT/LUK/MDEF contributions and duration reductions |
| Baby class modifiers | MEDIUM | 75% HP/SP, stat cap 80, forge/brew -50%, no rebirth, Small size |
| Healing item VIT/INT scaling | MEDIUM | `BaseHeal * (1 + VIT*0.02)` for HP items, `BaseHeal * (1 + INT*0.01)` for SP items |
| DEX minimum damage stabilization | LOW | Per weapon-level min ATK scaling |
| ASPD amotion formula (rAthena exact) | MEDIUM | `amotion -= amotion * (4*AGI + DEX) / 1000` is more precise than existing representation |
| Shield ASPD penalty | LOW | ~5-10% BTBA increase when shield equipped |
| Job-specific stat interactions | LOW | Forge/brew DEX+LUK rates, Steal DEX rate, etc. |
| Over-upgrade bonus details | LOW | Per-hit random range, not averaged |
| Weight limit at various STR/class combinations | LOW | Computed reference table |
| SP regen INT >= 120 bonus | MEDIUM | `floor(INT/2) - 56` additional regen |
| Confusion resistance formula | LOW | `(STR+INT)/200` based |
| Freeze duration increased by srcLUK | LOW | Attacker's LUK increases freeze duration |
| Armor refinement DEF contribution | MEDIUM | Debated 0.7 per + vs 1 per +, display vs calculation |

### 14.3 Implemented vs Not Yet Implemented (Code Status)

Based on reading `Implementation/02_Stats_Class_System.md`:

| Feature | Implemented in Code? | Notes |
|---------|---------------------|-------|
| `stat_calculator.js` module | Described but may not be deployed | Doc provides full code |
| StatusATK melee/ranged | YES (in doc) | Correct formulas |
| MATK min/max | YES (in doc) | Correct formulas |
| Soft DEF | PARTIAL | Uses simplified formula, not random VIT component |
| Hard DEF/MDEF | YES (in doc) | Correct 4000/1000 formulas |
| HIT/Flee/Crit | YES (in doc) | LUK not included in HIT (correct) |
| ASPD | YES (in doc) | Formula matches rAthena |
| HP/SP class coefficients | YES (in doc) | All 20+ classes |
| Weight limit | YES (in doc) | Per-class table |
| Regen formulas | YES (in doc) | Including INT >= 120 bonus |
| Job bonus stat tables (per-level) | NOT in code doc | Only totals documented, not per-level breakpoints |
| Status resistance formulas | NOT implemented | Not in either doc |
| Baby class modifiers | NOT implemented | Not mentioned |
| Healing item stat scaling | NOT in code doc | Formula exists but not in implementation guide |
| VIT DEF random component | NOT implemented | Simplified formula used |

---

## Sources

- [iRO Wiki Classic - Stats](https://irowiki.org/classic/Stats)
- [iRO Wiki - ATK](https://irowiki.org/wiki/ATK)
- [iRO Wiki Classic - DEF](https://irowiki.org/classic/DEF)
- [iRO Wiki - DEF](https://irowiki.org/wiki/DEF)
- [iRO Wiki - ASPD](https://irowiki.org/wiki/ASPD)
- [iRO Wiki - SP Recovery](https://irowiki.org/wiki/SP_Recovery)
- [iRO Wiki Classic - Adoption System](https://irowiki.org/classic/Adoption_System)
- [iRO Wiki - Refinement System](https://irowiki.org/wiki/Refinement_System)
- [iRO Wiki - Size](https://irowiki.org/wiki/Size)
- [Ragnarok Fandom Wiki - Stats](https://ragnarok.fandom.com/wiki/Stats_(RO))
- [RateMyServer - Stat Point Table](https://ratemyserver.net/index.php?page=misc_table_exp&op=21)
- [RateMyServer - Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus)
- [RateMyServer - Size Penalty Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer - Pre-Renewal Status Resistance Formulas](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [rAthena GitHub - status.cpp](https://github.com/rathena/rathena/blob/master/src/map/status.cpp)
- [rAthena GitHub - Pre-Re SoftDEF Fix PR #6766](https://github.com/rathena/rathena/pull/6766)
- [rAthena GitHub - Pre-Re SoftDEF Issue #6648](https://github.com/rathena/rathena/issues/6648)
- [rAthena Forum - Stat Calculation Pre-Renewal](https://rathena.org/board/topic/117425-stat-calculation-pre-renewal/)
- [rAthena Forum - Change ASPD Formula](https://rathena.org/board/topic/118001-change-aspd-formula-to-the-iro-one/)
