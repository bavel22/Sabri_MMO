# Swordsman -> Knight / Crusader Skills -- Deep Research

> **Sources**: iRO Wiki Classic, iRO Wiki (combined), RateMyServer Skill DB, rAthena pre-renewal source (skill_db.yml, skill_tree.yml, skill.cpp), Divine Pride, Hercules pre-re source
> **Target**: Pre-Renewal (Episode 10.4 era, before June 2010 Renewal patch)
> **Date**: 2026-03-22
> **Scope**: 10 Swordsman skills (IDs 100-109) + 12 Knight skills (IDs 700-711) + 14 Crusader skills (IDs 1300-1313) + 3 shared skills

---

## Table of Contents

1. [Swordsman Skills (IDs 100-109)](#1-swordsman-skills-ids-100-109)
2. [Knight Skills (IDs 700-711)](#2-knight-skills-ids-700-711)
3. [Crusader Skills (IDs 1300-1313)](#3-crusader-skills-ids-1300-1313)
4. [Skill Trees and Prerequisites](#4-skill-trees-and-prerequisites)
5. [Shared Skills between Knight and Crusader](#5-shared-skills-between-knight-and-crusader)
6. [Special Mechanics](#6-special-mechanics)
7. [Passive Skills Detailed Mechanics](#7-passive-skills-detailed-mechanics)
8. [Implementation Checklist](#8-implementation-checklist)
9. [Gap Analysis vs Current Codebase](#9-gap-analysis-vs-current-codebase)

---

## 1. Swordsman Skills (IDs 100-109)

### 1.1 Sword Mastery (ID 100)

| Field | Value |
|-------|-------|
| rAthena ID | SM_SWORD (2) |
| Type | Passive |
| Max Level | 10 |
| Target | None |
| Element | N/A |
| Weapon Types | One-Handed Swords AND Daggers |
| Classes | Swordsman, Knight, Crusader, Lord Knight, Paladin, Rogue, Stalker, Super Novice |
| Prerequisites | None |

**ATK Bonus Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

**Formula**: `+4 * SkillLv` ATK (mastery bonus -- bypasses DEF, added to final damage before elemental modifier)

**Mechanics**:
- Mastery ATK is a flat damage bonus added after DEF reduction
- Applies only when wielding `dagger` or `one_hand_sword` weapon types
- Does NOT apply to two-handed swords, spears, maces, or axes
- Stacks with other mastery bonuses (e.g., Rogue's Sword Mastery copy uses `Math.max()` -- non-stacking)

---

### 1.2 Two-Handed Sword Mastery (ID 101)

| Field | Value |
|-------|-------|
| rAthena ID | SM_TWOHAND (3) |
| Type | Passive |
| Max Level | 10 |
| Target | None |
| Weapon Types | Two-Handed Swords only |
| Prerequisites | Sword Mastery Lv1 |

**ATK Bonus Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 4 | 8 | 12 | 16 | 20 | 24 | 28 | 32 | 36 | 40 |

**Formula**: `+4 * SkillLv` ATK (mastery bonus -- bypasses DEF)

**Mechanics**:
- Identical scaling to Sword Mastery but for `two_hand_sword` only
- Key prerequisite for Two-Hand Quicken (Knight) and Bowling Bash

---

### 1.3 Increase HP Recovery (ID 102)

| Field | Value |
|-------|-------|
| rAthena ID | SM_RECOVERY (4) |
| Type | Passive |
| Max Level | 10 |
| Target | None |
| Prerequisites | None |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +HP/10s (flat) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |
| +MaxHP% / tick | 0.2% | 0.4% | 0.6% | 0.8% | 1.0% | 1.2% | 1.4% | 1.6% | 1.8% | 2.0% |
| Item Heal +% | 10% | 20% | 30% | 40% | 50% | 60% | 70% | 80% | 90% | 100% |

**HP Regen Formula** (per 10-second tick):
```
hpRegen = max(1, floor(MaxHP / 200)) + floor(VIT / 5) + (SkillLv * 5) + floor(MaxHP * SkillLv * 0.002)
```

**Item Heal Formula**:
```
healAmount = baseHealAmount * (1 + SkillLv * 0.10) * (1 + VIT_item_bonus)
```
- Item heal bonus is +10% per level (Lv10 = +100% = double healing)
- Stacks multiplicatively with VIT-based item heal bonus
- Applies to Red Potion, Yellow Potion, White Potion, etc.

**Restrictions**:
- No HP regen while overweight (>50% weight) unless specific items override
- No HP regen while moving (unless Moving HP Recovery is learned)
- Sitting provides 2x base regen rate

---

### 1.4 Bash (ID 103)

| Field | Value |
|-------|-------|
| rAthena ID | SM_BASH (5) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element (neutral default) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 (ASPD-limited) |
| Cooldown | 0 |
| Prerequisites | None |
| Weapon Restriction | Cannot use with bows |

**Damage and SP Table (rAthena authoritative):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 370 | 400 |
| SP Cost | 8 | 8 | 8 | 8 | 8 | 15 | 15 | 15 | 15 | 15 |
| HIT Bonus | +5% | +10% | +15% | +20% | +25% | +30% | +35% | +40% | +45% | +50% |

**Damage formula (rAthena)**: `skillratio += 30 * skill_lv` (base 100% + 30% per level)

**HIT Bonus**: Multiplicative accuracy bonus. Formula: `effectiveHIT = baseHIT * (1 + bashLevel * 5 / 100)`. At Lv10 with 67 HIT, effective HIT becomes `67 * 1.5 = 100`.

**Stun (Fatal Blow integration)**: When Fatal Blow (ID 109) is learned and Bash is Lv6+:
- **rAthena formula**: `stunChance = (BashLevel - 5) * BaseLevel / 10` percent (on 0-100% scale)
- Derived from rAthena: `status_change_start(... SC_STUN, (skill_lv - 5) * sd->status.base_level * 10, ...)` where rate is on 0-10000 scale
- **Stun duration**: 5 seconds (rAthena `skill_get_time2`)
- Stun chance is reduced by target's VIT resistance

**Example stun chances (before VIT resistance):**

| Bash Lv | Base Lv 50 | Base Lv 75 | Base Lv 99 |
|---------|-----------|-----------|-----------|
| 6 | 5.0% | 7.5% | 9.9% |
| 7 | 10.0% | 15.0% | 19.8% |
| 8 | 15.0% | 22.5% | 29.7% |
| 9 | 20.0% | 30.0% | 39.6% |
| 10 | 25.0% | 37.5% | 49.5% |

---

### 1.5 Provoke (ID 104)

| Field | Value |
|-------|-------|
| rAthena ID | SM_PROVOKE (6) |
| Type | Active (debuff) |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells (~450 UE units) |
| Element | Neutral |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 1000ms |
| Duration | 30 seconds |
| Prerequisites | None |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP Cost | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
| Target ATK+ | 5% | 8% | 11% | 14% | 17% | 20% | 23% | 26% | 29% | 32% |
| Target DEF- | 10% | 15% | 20% | 25% | 30% | 35% | 40% | 45% | 50% | 55% |
| Success Rate | 53% | 56% | 59% | 62% | 65% | 68% | 71% | 74% | 77% | 80% |

**Formulas**:
- ATK increase: `(2 + 3 * level)` percent
- DEF decrease: `(5 + 5 * level)` percent (VIT-based soft DEF only on players)
- Success rate: `(50 + 3 * level)` percent
- SP cost: `(3 + level)`

**Immunities**:
- **Undead element** monsters: completely immune
- **Boss-type** monsters (BOSS mode flag): completely immune
- SP should NOT be deducted when target is immune (check before deduction)

**On Enemy**: Forces aggro to the caster. Enemy gains ATK% boost but loses DEF%.

**On Player (PvP/WoE)**: Target gains ATK but loses VIT-based DEF only.

---

### 1.6 Magnum Break (ID 105)

| Field | Value |
|-------|-------|
| rAthena ID | SM_MAGNUM (7) |
| Type | Offensive (self-centered AoE) |
| Max Level | 10 |
| Target | Self-centered 5x5 AoE |
| Range | 0 (self-centered, NOT ground-targeted) |
| Element | **Fire** |
| Knockback | 2 cells |
| Cast Time | 0 (instant) |
| After-Cast Delay | 2000ms |
| Cooldown | 0 |
| SP Cost | 30 (all levels) |
| Prerequisites | Bash Lv5 |

**Damage, HP Cost, and HIT Bonus Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |
| HP Cost | 20 | 20 | 19 | 19 | 18 | 18 | 17 | 17 | 16 | 16 |
| HIT Bonus | +10 | +20 | +30 | +40 | +50 | +60 | +70 | +80 | +90 | +100 |

**Damage formula**: `(100 + 20 * SkillLv)%` ATK -- Fire element

**HP Cost formula**: `21 - ceil(level / 2)` -- produces 20,20,19,19,18,18,17,17,16,16
- HP cost CANNOT kill the caster (minimum 1 HP)
- HP cost is deducted before damage is dealt

**Fire Endow After-Effect (CRITICAL MECHANIC)**:
- For 10 seconds after Magnum Break, ALL normal auto-attacks gain an additional **20% Fire property bonus damage**
- This is NOT a weapon element endow -- it adds supplementary fire damage ON TOP of normal attacks
- The 20% fire damage bypasses DEF (it is applied as additional fire-element damage)
- If the user attacks with a Neutral weapon, only the 20% bonus portion is affected by the target's Fire element modifier
- The buff is called `magnum_break_fire` in the codebase
- Magnum Break's own damage also includes this 20% bonus, so at Lv10: `(300% + 20% * 300%)` = effectively 300% with 1/6 of total being fire bonus

**HIT Bonus**: `+10 * SkillLv` flat HIT bonus (additive, not multiplicative)

**Knockback**: 2 cells away from caster for all enemies hit

**targetType**: Must be `'aoe'` (self-centered), NOT `'ground'`

---

### 1.7 Endure (ID 106)

| Field | Value |
|-------|-------|
| rAthena ID | SM_ENDURE (8) |
| Type | Supportive (self buff) |
| Max Level | 10 |
| Target | Self |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 10000ms (10 seconds) |
| Prerequisites | Provoke Lv5 |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 10 | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 |
| +MDEF | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
| Max Hits | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 | 7 |

**Duration formula**: `(7 + SkillLv * 3)` seconds
**MDEF formula**: `+SkillLv` flat MDEF bonus

**7-Hit Mechanic**:
- Anti-flinch effect cancels after receiving **7 hits from monsters**
- **Player attacks do NOT count** toward this limit (the skill persists for full duration against player hits)
- Counter decremented ONLY by monster/enemy physical hits
- When counter reaches 0, buff is removed early with reason `'hit_limit'`

**Other Mechanics**:
- Prevents flinch (movement/attack delay on being hit) while active
- Sitting characters remain seated when hit while Endure is active
- Survives death (in `BUFFS_SURVIVE_DEATH` whitelist)
- Disabled in War of Emperium

---

### 1.8 Moving HP Recovery (ID 107) -- Quest Skill

| Field | Value |
|-------|-------|
| rAthena ID | SM_MOVINGRECOVERY (144) |
| Type | Passive (quest skill) |
| Max Level | 1 |
| Quest Requirement | Job Level 25+ |
| Effect | Allows HP regen while moving at **50% of standing rate** |

**Pre-Renewal**: Moving regen = 50% of standing rate
**Renewal**: Moving regen = 25% of standing rate (NOT our target)

**Does NOT affect**:
- Increase HP Recovery bonus (only base regen)
- Sitting rate (always 4x standing rate)

---

### 1.9 Auto Berserk (ID 108) -- Quest Skill

| Field | Value |
|-------|-------|
| rAthena ID | SM_AUTOBERSERK (146) |
| Type | Toggle (quest skill) |
| Max Level | 1 |
| SP Cost | 1 (to toggle on) |
| Quest Requirement | Job Level 35+ |

**Effect**: When HP drops below 25% of MaxHP, automatically applies self-Provoke Level 10:
- +32% ATK (same as Provoke Lv10)
- **-55% VIT-based soft DEF** (critical defensive penalty)

**Activation/Deactivation**:
- Auto-activates when `HP < 0.25 * MaxHP`
- Auto-deactivates when `HP >= 0.25 * MaxHP` (after healing)
- Can be manually toggled off
- Works even at 0 SP
- Survives death (in `BUFFS_SURVIVE_DEATH` whitelist)

---

### 1.10 Fatal Blow (ID 109) -- Quest Skill

| Field | Value |
|-------|-------|
| rAthena ID | SM_FATALBLOW (145) |
| Type | Passive (quest skill) |
| Max Level | 1 |
| Quest Requirement | Job Level 30+, Bash Lv6+ |

**Effect**: Enables stun chance on Bash at Lv6+

**Stun Formula (rAthena)**:
```
stunChance = (BashLevel - 5) * BaseLevel / 10   (0-100% scale)
```
- Derived from rAthena: `status_change_start(... SC_STUN, (skill_lv - 5) * sd->status.base_level * 10 ...)` on 0-10000 scale
- **Stun duration**: 5000ms (5 seconds)
- Reduced by target VIT resistance

---

## 2. Knight Skills (IDs 700-711)

### Class Overview

| Property | Value |
|----------|-------|
| Base Class | Swordsman |
| Transcendent | Lord Knight |
| Max Job Level | 50 |
| Total Skill Points | 76 (49 from swordsman + 49 from knight, but only 49 job levels = 49 points for knight tree) |
| Quest Skills | 1 (Charge Attack, Job Lv 40) |
| Soul Link Skill | 1 (One-Hand Quicken, requires Knight Spirit) |
| Weapons | 1H Swords, 2H Swords, Daggers, Spears, Maces, Axes |
| Mount | Peco Peco (via Riding skill) |

### 2.1 Spear Mastery (ID 700)

| Field | Value |
|-------|-------|
| rAthena ID | KN_SPEARMASTERY (60) |
| Type | Passive |
| Max Level | 10 |
| Weapon Types | All Spears (1H and 2H) |
| Prerequisites | None |
| Classes | Knight, Lord Knight, Crusader, Paladin |

**ATK Bonus Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Dismounted | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |
| Mounted | +5 | +10 | +15 | +20 | +25 | +30 | +35 | +40 | +45 | +50 |

**Dismounted formula**: `+4 * SkillLv` ATK
**Mounted formula**: `+5 * SkillLv` ATK (extra +1 per level from Riding synergy)

**Implementation**: Check `player.isMounted` -- if mounted, use `+5 * level`, otherwise `+4 * level`. The mounted bonus is intrinsic to Spear Mastery, not a separate effect from Riding.

---

### 2.2 Pierce (ID 701)

| Field | Value |
|-------|-------|
| rAthena ID | KN_PIERCE (56) |
| Type | Offensive (Physical, Melee) |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee (2-3 cells with spear) |
| Element | Weapon element |
| Weapon Requirement | **Spear only** (1H or 2H) |
| Cast Time | 0 (instant, ASPD-based) |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| SP Cost | 7 (flat, all levels) |
| Prerequisites | Spear Mastery Lv1 |

**Damage Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% per hit | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 | 200 |
| HIT Bonus | +5% | +10% | +15% | +20% | +25% | +30% | +35% | +40% | +45% | +50% |

**Damage formula**: `(100 + 10 * SkillLv)%` ATK per hit

**Multi-Hit by Target Size (CRITICAL MECHANIC)**:

| Target Size | Number of Hits |
|-------------|---------------|
| Small | 1 hit |
| Medium | 2 hits |
| Large | 3 hits |

- Each hit deals the FULL ATK% independently (not split)
- Against Large at Lv10: `200% * 3 = 600%` total ATK
- Damage is applied as a **single bundle** server-side (one transaction)
- rAthena: HitCount = -3 (negative = multi-hit without increasing per-hit damage), then reduced based on size
- Lex Aeterna doubles the ENTIRE bundle

**Accuracy Bonus**: `+5 * SkillLv` percent multiplicative HIT bonus

---

### 2.3 Spear Stab (ID 702)

| Field | Value |
|-------|-------|
| rAthena ID | KN_SPEARSTAB (57) |
| Type | Offensive (Physical) |
| Max Level | 10 |
| Target | Single enemy (with LINE AoE) |
| Range | 4 cells with spear |
| Element | Weapon element |
| Weapon Requirement | **Spear only** |
| Cast Time | 0 (ASPD-based) |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| SP Cost | 9 (flat, all levels) |
| Knockback | 6 cells |
| Prerequisites | Pierce Lv5 |

**Damage Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

**Damage formula**: `(100 + 20 * SkillLv)%` ATK

**Line AoE Mechanic (KEY FEATURE)**:
- Hits ALL enemies in a **straight line** between caster and targeted enemy
- The line is defined by caster position to target position
- All enemies within the line path take the SAME damage
- All hit enemies are knocked back 6 cells in the direction away from caster
- Knockback disabled in WoE

**Implementation**: Calculate line from caster to target, find all enemies along that line within a narrow width tolerance (~50 UE units), deal damage + knockback to all.

---

### 2.4 Brandish Spear (ID 703)

| Field | Value |
|-------|-------|
| rAthena ID | KN_BRANDISHSPEAR (58) |
| Type | Offensive (Physical, Directional AoE) |
| Max Level | 10 |
| Target | Directional AoE (frontal cone from caster) |
| Range | Melee |
| Element | Weapon element |
| Weapon Requirement | **Spear** (1H or 2H) |
| Mount Requirement | **MUST be mounted on Peco Peco** |
| Cast Time | 700ms (uninterruptible, reduced by DEX) |
| After-Cast Delay | 1000ms |
| Cooldown | 0 |
| SP Cost | 12 (flat, all levels) |
| Knockback | 2 cells |
| Prerequisites | Spear Stab Lv3, Riding Lv1 |

**Damage Table (Pre-Renewal):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 | 220 | 240 | 260 | 280 | 300 |

**Damage formula**: `(100 + 20 * SkillLv)%` ATK
**Note**: Renewal changed to `(400 + 100 * SkillLv)%` -- NOT our target.

**AoE Pattern (Directional, Level-Dependent)**:

The AoE extends forward from caster in the direction of the targeted enemy. It expands at levels 4, 7, and 10:

```
Direction: North (rotates based on target direction)

Level 10 only:       .DDD.
Levels 7-10:         CCCCC
Levels 4-10:         BBBBB
All levels:          AAAAA
Caster position:     AAXAA

X = caster (Knight)
A = Zone 1: always affected (all levels)
B = Zone 2: added at Lv4+
C = Zone 3: added at Lv7+
D = Zone 4: added at Lv10 only (narrower, 3 cells)
```

**Zone damage multipliers (Pre-Renewal)**:
- Zone A (closest to caster): Full damage (100% of ATK%)
- Zone B: ~75% damage
- Zone C: ~50% damage
- Zone D: ~25% damage

At Lv10 inner zone = approximately 562% total, outer zone = 300% (zone-based multipliers from rAthena).

| Level Range | AoE Depth | Width |
|-------------|-----------|-------|
| 1-3 | 2 rows | 5 cells |
| 4-6 | 3 rows | 5 cells |
| 7-9 | 4 rows | 5 cells |
| 10 | 5 rows | 5 wide (row D = 3 cells) |

**Special Mechanics**:
1. **Mount required**: Cannot be used while dismounted. Emit error if `!player.isMounted`
2. **Directional**: AoE direction determined by caster facing toward targeted enemy
3. **DEF reduction during cast**: Caster DEF reduced to 2/3 during the 700ms cast time
4. **Knockback**: 2 cells (disabled in WoE)

---

### 2.5 Spear Boomerang (ID 704)

| Field | Value |
|-------|-------|
| rAthena ID | KN_SPEARBOOMERANG (59) |
| Type | Offensive (Physical, **Ranged**) |
| Max Level | 5 |
| Target | Single Enemy |
| Element | Weapon element |
| Weapon Requirement | **Spear** (1H or 2H) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 1000ms |
| Cooldown | 0 |
| SP Cost | 10 (flat, all levels) |
| Prerequisites | Pierce Lv3 |

**Damage and Range Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 150 | 200 | 250 | 300 | 350 |
| Range (cells) | 3 | 5 | 7 | 9 | 11 |
| Range (UE units) | 150 | 250 | 350 | 450 | 550 |

**Damage formula**: `(100 + 50 * SkillLv)%` ATK
**Range formula**: `(1 + 2 * SkillLv)` cells

**Key mechanic**: This is a **RANGED physical attack** despite being a spear skill:
- Can be blocked by **Pneuma** (Safety Wall does NOT block)
- Can trigger ranged-specific card effects
- Range scales with skill level (level-dependent range)

---

### 2.6 Two-Hand Quicken (ID 705)

| Field | Value |
|-------|-------|
| rAthena ID | KN_TWOHANDQUICKEN (60) |
| Type | Supportive (Self Buff) |
| Max Level | 10 |
| Target | Self |
| Weapon Requirement | **Two-Handed Sword MUST be equipped** |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Prerequisites | Two-Handed Sword Mastery Lv1 (ID 101) |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP Cost | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |
| Duration (s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

**SP formula**: `10 + SkillLv * 4`
**Duration formula**: `SkillLv * 30` seconds

**ASPD Bonus (Pre-Renewal ONLY)**:
- **+30% ASPD** (applied as IAS 0.3 in ASPD formula)
- Pre-renewal: `ASPD = 200 - (WD - [WD*AGI/25 + WD*DEX/100]) / 10 * (1 - SM)`
- Where SM = sum of speed modifiers (THQ = 0.3, Berserk Potion = 0.2, etc.)
- In renewal, additionally provides CRI and HIT bonuses -- these DO NOT exist in pre-renewal

**ASPD mutual exclusion**: Only the strongest ASPD buff applies among:
- Two-Hand Quicken (THQ)
- Spear Quicken (SQ)
- Adrenaline Rush (AR)
- A Poem of Bragi
- Concentration (Assassin)

**Cancellation Conditions**:
1. Decrease AGI (Acolyte skill)
2. Quagmire (Wizard ground skill)
3. Unequipping the 2H Sword
4. Manual cancelation
5. Dispel removes THQ (it is NOT in UNDISPELLABLE set)

---

### 2.7 Auto Counter (ID 706)

| Field | Value |
|-------|-------|
| rAthena ID | KN_AUTOCOUNTER (61) |
| Type | Active (Self Stance/Counter) |
| Max Level | 5 |
| Target | Self (enters guard stance) |
| Weapon Requirement | Melee weapon (cannot use with bows) |
| Cast Time | 0 |
| After-Cast Delay | ASPD-based |
| Cooldown | 0 |
| SP Cost | 3 (flat, all levels) |
| Prerequisites | Two-Handed Sword Mastery Lv1 (ID 101) |

**Stance Duration Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Duration (ms) | 400 | 800 | 1200 | 1600 | 2000 |

**Duration formula**: `SkillLv * 400` ms

**Passive Counter (auto-trigger):**
1. When a **melee physical auto-attack** hits the caster during stance:
   - The incoming attack is **blocked** (no damage taken)
   - Caster performs a **critical hit** counter-attack on the attacker
   - Critical counter **ignores DEF**
   - Stance ends immediately
   - Only ONE counter per stance activation
2. **Facing requirement**: Caster must be facing the attacker for counter to trigger
3. **Skills cannot be countered** -- only normal auto-attacks
4. All monsters including bosses can have their attacks blocked and countered

**Active Counter (click-to-attack during stance):**
- If player clicks a monster while in Auto Counter stance:
  - **2x critical chance** (double normal crit rate)
  - **+20 accuracy** bonus
  - Stance ends after the attack
  - Counter-attack still has ASPD-based delay

**Implementation Notes**:
- Requires a time-limited "stance" state on the player object
- Combat tick must check if target has Auto Counter stance AND is facing attacker before applying damage
- Counter-attack should use `forceCrit: true, forceHit: true` in damage calculation

---

### 2.8 Bowling Bash (ID 707)

| Field | Value |
|-------|-------|
| rAthena ID | KN_BOWLINGBASH (62) |
| Type | Offensive (Physical, AoE) |
| Max Level | 10 |
| Target | Single enemy (with 3x3 AoE splash) |
| Range | Melee (2 cells) |
| Element | Weapon element |
| Weapon Requirement | None officially (works with swords, spears, any melee) |
| Cast Time | 700ms (uninterruptible, reduced by DEX) |
| After-Cast Delay | ASPD-based |
| Cooldown | 0 |
| Knockback | 1 cell (in direction caster last faced) |
| Prerequisites | Bash Lv10, Magnum Break Lv3, 2H Sword Mastery Lv5, Two-Hand Quicken Lv10, Auto Counter Lv5 |

**Damage Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 | 340 | 380 | 420 | 460 | 500 |
| SP Cost | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 |

**Damage formula**: `(100 + 40 * SkillLv)%` ATK
**SP formula**: `(12 + SkillLv)`

**Two-Hit Mechanic (CRITICAL -- Pre-Renewal)**:
- Bowling Bash always hits **TWICE** in pre-renewal
- Each hit is calculated independently with the full ATK% multiplier
- At Lv10: effective total = `500% * 2 = 1000%` ATK
- **Lex Aeterna**: Only doubles the FIRST hit (second hit is normal)
- **Card effects**: Apply to BOTH hits independently
- **Critical**: Each hit has its own critical chance roll
- **DEF**: Each hit is reduced by DEF independently

**AoE / Chain Reaction Mechanics (Pre-Renewal)**:
1. Primary target takes 2 hits and is knocked back 1 cell
2. Knockback direction = caster's last faced direction (applied BEFORE damage)
3. If knocked-back target collides with or lands near other enemies, those secondary enemies take **splash damage** in a 3x3 cell area
4. Splash damage uses same ATK% but may hit only ONCE for secondary targets
5. Chain reaction: if secondary targets are knocked into more enemies, the chain continues
6. **Max chain depth** = skill level (prevents infinite cascading)

**Gutter Line (Pre-Renewal exploit)**:
- In pre-renewal RO, an invisible grid exists: when `X % 40 == 0` or `Y % 40 == 0`, Bowling Bash behaves differently with altered knockback/AoE
- **Recommendation**: Do NOT implement gutter lines -- this was a bug/unintended behavior
- Use standard 2-hit + 3x3 splash + 1-cell knockback consistently

**DEF Reduction During Cast**: Caster DEF reduced to 2/3 during 700ms cast time

---

### 2.9 Riding / Peco Peco Ride (ID 708)

| Field | Value |
|-------|-------|
| rAthena ID | KN_RIDING (63) |
| Type | Passive |
| Max Level | 1 |
| Prerequisites | Endure Lv1 (Swordsman, ID 106) |

**Effects When Mounted:**

| Effect | Value | Source |
|--------|-------|--------|
| Movement Speed | +25% base move speed (equivalent to Increase AGI) | iRO Wiki Classic |
| Weight Capacity | +1000 weight limit | iRO Wiki Classic |
| ASPD Penalty | ASPD set to **50% of normal** (halved) | iRO Wiki Classic |
| Spear vs Medium | Spear weapons deal **100% damage** to Medium size (normally 75%) | iRO Wiki Classic |
| Spear Mastery | Enables higher mounted ATK bonus (+5/lv instead of +4/lv) | iRO Wiki Classic |
| Sprite | Character model changes to mounted sprite | Visual |

**Mount/Dismount**:
- Player toggles mount on/off (via `/mount` command or skill use)
- Cannot mount indoors in certain zones (may defer this restriction)
- Dismounting restores normal ASPD, removes weight bonus, removes mounted Spear Mastery bonus
- Mount is purely cosmetic + stat modifier (no separate entity)
- Required for Brandish Spear

---

### 2.10 Cavalier Mastery (ID 709)

| Field | Value |
|-------|-------|
| rAthena ID | KN_CAVALIERMASTERY (64) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Riding Lv1 (ID 708) |

**ASPD Recovery Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Effective ASPD | 60% | 70% | 80% | 90% | 100% |

**Formula**: Without Cavalier Mastery, mounted ASPD = 50% of normal. With skill:
```
mountedASPD = normalASPD * (0.5 + CavalierMasteryLv * 0.1)
```
- Lv0: 50%, Lv1: 60%, Lv2: 70%, Lv3: 80%, Lv4: 90%, **Lv5: 100%** (full restoration)

---

### 2.11 Charge Attack (ID 710) -- Quest Skill

| Field | Value |
|-------|-------|
| rAthena ID | KN_CHARGEATK (1001) |
| Type | Offensive (Physical, **Ranged**) |
| Max Level | 1 |
| Target | Single Enemy |
| Range | 14 cells (~700 UE units) |
| Element | Weapon element |
| Weapon Requirement | None (any weapon) |
| SP Cost | 40 |
| Quest Requirement | Knight Platinum Skills Quest, Job Lv 40 |
| Knockback | 1 cell |

**Distance-Based Damage (Pre-Renewal):**

| Distance | Damage | Cast Time |
|----------|--------|-----------|
| 0-2 cells | 100% ATK | 500ms |
| 3-5 cells | 200% ATK | 700ms |
| 6-8 cells | 300% ATK | 1000ms |
| 9-11 cells | 400% ATK | 1200ms |
| 12-14 cells | 500% ATK | 1500ms |

**Special Mechanics**:
1. **Distance-based scaling**: Both damage AND cast time increase with distance to target
2. **Caster movement**: The caster rushes/teleports to the target position regardless of hit/miss
3. **Ranged physical**: Despite moving to target, damage is classified as RANGED (blocked by Pneuma, NOT blocked by Safety Wall)
4. **Trap escape**: Can escape Ankle Snare, Fiber Lock, and Close Confine if an enemy is in range
5. **1-cell knockback**: Target pushed in random direction on hit

---

### 2.12 One-Hand Quicken (ID 711) -- Soul Link

| Field | Value |
|-------|-------|
| rAthena ID | KN_ONEHAND (1001 variant) |
| Type | Supportive (Self Buff) |
| Max Level | 1 |
| Target | Self |
| Weapon Requirement | **One-Handed Sword** |
| SP Cost | 100 |
| Duration | 300 seconds (5 minutes) |
| Prerequisite | Two-Hand Quicken Lv10, Knight Spirit soul link active |
| ASPD Bonus | +30% (same as Two-Hand Quicken) |

**Implementation Priority**: LOW (requires Soul Linker class, defer until Trans classes)

**Cancellation**: Same as THQ + Soul Link expiring

---

## 3. Crusader Skills (IDs 1300-1313)

### Class Overview

| Property | Value |
|----------|-------|
| rAthena Job ID | 14 (Crusader), 4015 (Paladin) |
| Base Class | Swordsman |
| Job Levels | 50 |
| Skill Points | 49 |
| Total Crusader Skills | 14 (+ 1 quest skill = Shrink) |
| Transcendent | Paladin |
| Shield Dependency | Auto Guard, Shield Charge, Shield Boomerang, Reflect Shield, Defender, Shrink ALL require shield |

### 3.1 Faith (ID 1300)

| Field | Value |
|-------|-------|
| rAthena ID | CR_TRUST (248) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Max HP Bonus | +200 | +400 | +600 | +800 | +1000 | +1200 | +1400 | +1600 | +1800 | +2000 |
| Holy Resist | 5% | 10% | 15% | 20% | 25% | 30% | 35% | 40% | 45% | 50% |

**MaxHP formula**: `+200 * SkillLv` (flat, added AFTER VIT calculation)
```
MaxHP = floor(BaseHP * (1 + VIT * 0.01)) + faithBonus
```

**Holy Resistance**: `holyDamage = holyDamage * (100 - SkillLv * 5) / 100`
- Reduces damage from Holy element attacks
- Important for Grand Cross self-damage reduction (with Faith Lv10, self-damage is halved)

---

### 3.2 Auto Guard (ID 1301)

| Field | Value |
|-------|-------|
| rAthena ID | CR_AUTOGUARD (249) |
| Type | Supportive (Toggle) |
| Max Level | 10 |
| Target | Self |
| Duration | 300 seconds (5 minutes) |
| Requirements | **Shield equipped** |
| Prerequisites | None |

**Block Chance and SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Block% | 5 | 10 | 14 | 18 | 21 | 24 | 26 | 28 | 29 | 30 |
| SP Cost | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Move Delay (ms) | 300 | 300 | 300 | 300 | 300 | 200 | 200 | 200 | 200 | 100 |

**Mechanics**:
- **Blocks both melee AND ranged physical attacks** (does NOT block magic)
- When block occurs, player briefly immobilized (movement delay above)
- Toggle skill: on/off during 5-minute window
- Shield must remain equipped -- removing shield cancels effect
- When blocked: attack deals 0 damage, emit `combat:blocked` event
- Card on-hit effects may still trigger on block (source-dependent)

**Interaction with Shrink**: When Auto Guard blocks AND Shrink is active, 50% chance to knockback attacker 2 cells.

**Checked in ALL damage paths**: auto-attack, `executeMonsterPlayerSkill()`, `elemental_melee`, `status_melee`

---

### 3.3 Holy Cross (ID 1302)

| Field | Value |
|-------|-------|
| rAthena ID | CR_HOLYCROSS (253) |
| Type | Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 2 cells (melee) |
| Element | **Holy** (forced, ignores weapon element) |
| Hits | 2 (displayed as one bundle) |
| SP Cost | 11-20 (10 + SkillLv) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Prerequisites | Faith Lv3 |

**Damage and SP per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 135 | 170 | 205 | 240 | 275 | 310 | 345 | 380 | 415 | 450 |
| SP Cost | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
| Blind% | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

**Damage formula**: `(100 + 35 * SkillLv)%` ATK

**Two-Handed Spear Bonus**: Damage is **DOUBLED** when using a two-handed spear. Total ATK% becomes 270-900%.

**Element**: ALWAYS Holy -- ignores weapon element and endows.

**Blind**: `3 * SkillLv`% chance to apply blind status.

**Ignores size penalty**: No weapon size penalty applies.

---

### 3.4 Grand Cross (ID 1303)

| Field | Value |
|-------|-------|
| rAthena ID | CR_GRANDCROSS (254) |
| Type | Offensive (Hybrid Physical+Magical) |
| Max Level | 10 |
| Target | Self-centered AoE |
| AoE Shape | **Cross-shaped** (not square -- 9 cells in + pattern, 2 cells each direction) |
| Element | **Holy** |
| Cast Time | 3000ms (3 seconds, **uninterruptible**) |
| After-Cast Delay | 1500ms |
| Cooldown | 1000ms |
| HP Cost | **20% of current HP** per cast |
| Prerequisites | Holy Cross Lv6, Faith Lv10 |

**SP Cost per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| SP Cost | 37 | 44 | 51 | 58 | 65 | 72 | 79 | 86 | 93 | 100 |

**SP formula**: `30 + 7 * SkillLv`

**Damage Formula (Pre-Renewal)**:
```
Per-Hit Damage = floor((ATK + MATK) * (100 + 40 * SkillLv) / 100) * HolyElementMod
```

| Level | (ATK+MATK)% |
|-------|-------------|
| 1-10 | 140, 180, 220, 260, 300, 340, 380, 420, 460, 500 |

**ATK component**: Uses only WeaponATK (from STR/DEX/LUK/weapon base). Does NOT benefit from:
- Weapon Masteries
- Demon Bane
- Percentage damage cards
- Flat ATK bonus cards/items

**MATK component**: Full MATK (random between MATKmin and MATKmax, scales with INT^2)

**Number of Hits**: 1-5 times depending on enemy position relative to caster. When multiple monsters share a cell, hits per monster decrease (minimum 1).

**Self-Damage Formula**:
```
SelfDamage = floor(GrandCrossDamage / 2) * HolyElementModOnCaster
```
- Caster takes approximately HALF the damage dealt
- Faith Holy Resistance reduces self-damage: with Faith Lv10 (50% holy resist), self-damage is halved again to ~25% of dealt damage
- HP cost (20% current HP) applied BEFORE skill activates
- Self-damage applied AFTER skill effect resolves
- Self-damage is Holy element AND Demi-Human race (can reduce with Holy/Demi-Human resist gear)

**Cross-Shaped AoE Pattern**:
```
    X
    X
  X X X     (caster at center)
    X
    X
```
Affected cells (caster at 0,0): `(0,-2), (0,-1), (-2,0), (-1,0), (0,0), (1,0), (2,0), (0,1), (0,2)` = 9 cells in cross pattern. NOT a 5x5 square.

**Grand Cross misses** targets positioned diagonally from the caster.

**Special Mechanics**:
- Cast is **uninterruptible** (immune to flinch/knockback during cast)
- Caster is **immobilized** during the 0.9-second effect animation
- Affected by long-range modifiers
- Blind chance on Undead/Demon monsters: `3 * SkillLv`%
- Golden Thief Bug Card on shield DISABLES Grand Cross entirely

---

### 3.5 Shield Charge / Smite (ID 1304)

| Field | Value |
|-------|-------|
| rAthena ID | CR_SHIELDCHARGE (250) |
| Type | Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | 4 cells |
| Element | Neutral (weapon element) |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Requirements | **Shield equipped** |
| Prerequisites | Auto Guard Lv5 |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |
| Stun Chance | 20% | 25% | 30% | 35% | 40% |
| Knockback | 5 | 6 | 7 | 8 | 9 cells |

**Damage formula**: `(100 + 20 * SkillLv)%` ATK
**Stun formula**: `(15 + 5 * SkillLv)%` -- stun duration 5000ms
**Knockback formula**: `(4 + SkillLv)` cells (disabled in WoE)

---

### 3.6 Shield Boomerang (ID 1305)

| Field | Value |
|-------|-------|
| rAthena ID | CR_SHIELDBOOMERANG (251) |
| Type | Offensive (**Ranged Physical**) |
| Max Level | 5 |
| Target | Single Enemy |
| Element | **Always Neutral** |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 700ms |
| Requirements | **Shield equipped** |
| Prerequisites | Shield Charge Lv3 |

**Pre-Renewal Damage Formula**:

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 130 | 160 | 190 | 220 | 250 |
| Range (cells) | 3 | 5 | 7 | 9 | 11 |

**Damage formula**: `ATK * (100 + 30 * SkillLv) / 100`
**Range formula**: `(1 + SkillLv * 2)` cells

**Pre-Renewal Damage Composition**:
The damage comes from:
- **Base ATK from STR, DEX, LUK** (StatusATK)
- **Shield upgrades**: `+5 * shieldRefine` ATK
- **Shield weight**: `shieldWeight / 10` ATK (heavier shields = more damage)
- Element is ALWAYS Neutral regardless of weapon element

**Does NOT benefit from**:
- Base weapon ATK
- Weapon upgrades/refine
- Mastery Skills (Sword Mastery, Spear Mastery)
- Spirit Spheres
- Provoke ATK bonus
- Magnum Break fire bonus
- Impositio Manus

**Ignores size penalty**: No weapon-type size penalty applies.

**Ranged**: Blocked by Pneuma, NOT blocked by Safety Wall.

---

### 3.7 Devotion (ID 1306)

| Field | Value |
|-------|-------|
| rAthena ID | CR_DEVOTION (255) |
| Type | Supportive |
| Max Level | 5 |
| Target | Single **Party Member** |
| SP Cost | 25 (all levels) |
| Cast Time | 3000ms (1.5s variable + 1.5s fixed -- pre-renewal may be all variable) |
| After-Cast Delay | 3000ms |
| Prerequisites | Reflect Shield Lv5, Grand Cross Lv4 |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Max Targets | 1 | 2 | 3 | 4 | 5 |
| Range (cells) | 7 | 8 | 9 | 10 | 11 |
| Duration (s) | 30 | 45 | 60 | 75 | 90 |

**Core Mechanic**: Creates a damage-redirect link from party member to Crusader.
- **ALL physical AND magical damage** intended for the protected ally is transferred to the Crusader
- Damage uses the **protected target's DEF/MDEF** for reduction (NOT the Crusader's)
- If redirected damage kills the Crusader, excess damage is NOT passed back
- Protected target CANNOT have their skills interrupted by damage (since they receive no damage)

**Restrictions**:
- **Level restriction**: Target must be within **10 base levels** of the Crusader
- **Crusaders/Paladins CANNOT be targets** (both as casters and receivers)
- Cannot target self
- Must be in same party

**Breaking Conditions**:
1. Duration expires
2. Protected target moves out of skill range (7-11 cells based on level)
3. Crusader's HP drops below **25%** of MaxHP
4. Crusader or target dies
5. Crusader is stunned, frozen, or otherwise CC'd
6. Close Confine on either party breaks the link

**Interactions**:
- Auto Guard: Crusader CAN block redirected damage with Auto Guard
- Reflect Shield: Crusader reflects redirected melee damage back to original attacker
- Dispel removes Devotion link

---

### 3.8 Reflect Shield (ID 1307)

| Field | Value |
|-------|-------|
| rAthena ID | CR_REFLECTSHIELD (252) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Duration | 300 seconds (5 minutes) |
| Requirements | **Shield equipped** |
| Prerequisites | **Auto Guard Lv3 AND Shield Boomerang Lv3** |

**Reflect % and SP Cost:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Reflect% | 13 | 16 | 19 | 22 | 25 | 28 | 31 | 34 | 37 | 40 |
| SP Cost | 35 | 40 | 45 | 50 | 55 | 60 | 65 | 70 | 75 | 80 |

**Reflect formula**: `(10 + 3 * SkillLv)%`

**Mechanics**:
- Reflects a percentage of **melee physical damage** back to attacker
- Does NOT reflect ranged attacks or magic
- Reflected damage classified as "Reflect Type Damage"
- Reflected damage does NOT trigger auto-cast cards on the Crusader
- Reflected damage does NOT trigger enemy Kaahi
- Ignores weapon size modifications on reflected damage
- Shield must remain equipped
- Stacks with card-based reflect effects (Maya Card)
- Formula: `reflectedDamage = floor(incomingMeleeDamage * reflectPercent / 100)`

---

### 3.9 Providence / Resistant Souls (ID 1308)

| Field | Value |
|-------|-------|
| rAthena ID | CR_PROVIDENCE (256) |
| Type | Supportive |
| Max Level | 5 |
| Target | Single Player (party member, NOT Crusader/Paladin) |
| Range | 9 cells |
| SP Cost | 30 (all levels) |
| Cast Time | 1000ms |
| After-Cast Delay | 3000ms |
| Duration | 180 seconds (3 minutes) |
| Prerequisites | **Divine Protection Lv5 (Acolyte, ID 413), Heal Lv5 (Crusader, ID 1311)** |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Demon Race Resist | 5% | 10% | 15% | 20% | 25% |
| Holy Element Resist | 5% | 10% | 15% | 20% | 25% |

**Mechanics**:
- Provides resistance to Demon race attacks AND Holy element attacks
- **Crusaders/Paladins CANNOT be targeted** (including self-cast)
- Only works on non-Crusader party members
- Stacks multiplicatively with Faith's Holy Resistance

---

### 3.10 Defender / Defending Aura (ID 1309)

| Field | Value |
|-------|-------|
| rAthena ID | CR_DEFENDER (257) |
| Type | Supportive (Toggle) |
| Max Level | 5 |
| Target | Self |
| SP Cost | 30 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 800ms |
| Duration | 180 seconds (3 minutes) |
| Requirements | **Shield equipped** |
| Prerequisites | **Shield Boomerang Lv1** |

**Per-Level Table:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Ranged Damage Reduction | 20% | 35% | 50% | 65% | 80% |
| ASPD Penalty | -20% | -15% | -10% | -5% | 0% |
| Movement Speed | -33% | -33% | -33% | -33% | -33% |

**Mechanics**:
- Toggle skill with duration
- Reduces ALL ranged physical damage by listed percentage
- Movement speed reduced by ~33% at all levels
- ASPD penalty decreases with level; at Lv5 no ASPD penalty
- Shield must remain equipped
- Devotion targets also receive the ranged reduction (but also movement penalty)
- Stacks with Auto Guard (guard blocks entirely, Defender reduces if not blocked)

---

### 3.11 Spear Quicken (ID 1310)

| Field | Value |
|-------|-------|
| rAthena ID | CR_SPEARQUICKEN (258) |
| Type | Supportive |
| Max Level | 10 |
| Target | Self |
| Requirements | **Two-handed spear equipped** |
| Prerequisites | Spear Mastery Lv10 (Knight/Crusader shared, ID 700) |

**Per-Level Table (Pre-Renewal):**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ASPD Bonus | +21% | +22% | +23% | +24% | +25% | +26% | +27% | +28% | +29% | +30% |
| SP Cost | 24 | 28 | 32 | 36 | 40 | 44 | 48 | 52 | 56 | 60 |
| Duration (s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |

**ASPD formula**: `+(20 + SkillLv)%` (scales 21-30% unlike THQ's flat 30%)

**Mechanics**:
- Functionally similar to Two-Hand Quicken but for two-handed spears
- Cancelled by Decrease AGI, Quagmire, and weapon swap to non-2H-spear
- ASPD mutual exclusion with THQ, AR, etc. (strongest wins)
- In pre-renewal: PURE ASPD bonus (no crit/flee bonuses)

---

### 3.12 Heal (Crusader version, ID 1311)

| Field | Value |
|-------|-------|
| rAthena ID | AL_HEAL (28, shared with Acolyte) |
| Type | Supportive |
| Max Level | 10 |
| Target | Single (ally or undead enemy) |
| Range | 9 cells |
| Element | Holy |
| Cast Time | 0 (instant for Crusaders in pre-renewal) |
| After-Cast Delay | 1000ms |
| Prerequisites | **Demon Bane Lv5 (Acolyte, ID 413), Faith Lv10** |

**Heal Formula (Pre-Renewal)**:
```
HealAmount = floor((BaseLv + INT) / 8) * (4 + 8 * SkillLv)
```

**SP Cost**: `3 * SkillLv + 10` = 13, 16, 19, 22, 25, 28, 31, 34, 37, 40

**Undead Targeting**: Damages Undead property monsters at **50% effectiveness**: `damage = floor(healAmount / 2)`, Holy element

---

### 3.13 Cure (Crusader version, ID 1312)

| Field | Value |
|-------|-------|
| rAthena ID | AL_CURE (29, shared with Acolyte) |
| Type | Supportive |
| Max Level | 1 |
| Target | Single (ally) |
| Range | 9 cells |
| SP Cost | 15 |
| Cast Time | 0 |
| After-Cast Delay | 1000ms |
| Prerequisites | Faith Lv5 |

**Effect**: Removes **Silence**, **Blind**, and **Confusion** status effects from target.

---

### 3.14 Shrink (ID 1313) -- Quest Skill

| Field | Value |
|-------|-------|
| rAthena ID | SN_SHRINK (1002) |
| Type | Toggle |
| Max Level | 1 |
| Target | Self |
| SP Cost | 15 |
| Duration | 300 seconds (5 minutes) |
| Requirements | **Shield equipped, Auto Guard active** |
| Prerequisites | Quest completion |

**Mechanics (Pre-Renewal)**:
- When Auto Guard successfully blocks an attack, **50% chance** to knockback the attacker **2 cells**
- Knockback chance is flat 50% (NOT scaled by AG level -- per-AG-level scaling was a Renewal change)
- Requires Auto Guard to be active; Shrink does nothing without Auto Guard
- Toggle on/off

---

## 4. Skill Trees and Prerequisites

### 4.1 Swordsman Skill Tree

```
Sword Mastery (100) ---Lv1---> Two-Handed Sword Mastery (101)
                                         |
                                [Used by Knight: THQ(705), AC(706)]
                                [Used by Knight: BB(707) requires Lv5]

Bash (103) ---Lv5---> Magnum Break (105)
                         |
               [Used by Knight: BB(707) requires MB Lv3]

Provoke (104) ---Lv5---> Endure (106)
                              |
                    [Used by Knight/Crusader: Riding(708) requires Endure Lv1]

Increase HP Recovery (102): No dependencies

Quest Skills (no tree prereqs):
  - Moving HP Recovery (107): Job Lv 25
  - Fatal Blow (109): Job Lv 30, Bash Lv6
  - Auto Berserk (108): Job Lv 35
```

### 4.2 Knight Skill Tree

```
Swordsman Tree (inherited)
==========================
Sword Mastery(100) --> 2H Sword Mastery(101) --> [Two-Hand Quicken(705)]
Bash(103) --> Magnum Break(105)
Provoke(104) --> Endure(106) --> [Riding(708)]

Knight Tree
===========
Spear Mastery(700) --> Pierce(701) --> Spear Stab(702) --> [Brandish Spear(703)]
                   |                |                         (also needs Riding Lv1)
                   |                └--> Spear Boomerang(704)

2H Sword Mastery(101) --> Two-Hand Quicken(705)
                     └--> Auto Counter(706)

Bowling Bash(707) requires ALL of:
  - Bash(103) Lv10
  - Magnum Break(105) Lv3
  - 2H Sword Mastery(101) Lv5
  - Two-Hand Quicken(705) Lv10
  - Auto Counter(706) Lv5

Endure(106) Lv1 --> Riding(708) --> Cavalier Mastery(709)
                               └--> [Brandish Spear(703)]

Charge Attack(710): Quest skill (no tree prereqs, Job Lv 40 quest)
One-Hand Quicken(711): Requires THQ Lv10 + Knight Spirit soul link
```

### 4.3 Crusader Skill Tree

```
Row 0: Faith(1300)    Auto Guard(1301)    Devotion(1306)    Heal(1311)
Row 1: Holy Cross(1302)  Shield Charge(1304)  Reflect Shield(1307)  Cure(1312)
Row 2: Grand Cross(1303)  Shield Boomerang(1305)  Providence(1308)
Row 3: Spear Quicken(1310)  Defender(1309)  Shrink(1313)

Prerequisite Chains:
====================
Faith(1300) ---Lv3---> Holy Cross(1302) ---Lv6---> Grand Cross(1303)
Faith(1300) ---Lv10--> Grand Cross(1303)
Faith(1300) ---Lv5---> Cure(1312)
Faith(1300) ---Lv10--> Heal(1311)

Auto Guard(1301) ---Lv5---> Shield Charge(1304) ---Lv3---> Shield Boomerang(1305)
Auto Guard(1301) ---Lv3---> Reflect Shield(1307) [ALSO needs SB Lv3]

Shield Boomerang(1305) ---Lv3---> Reflect Shield(1307)
Shield Boomerang(1305) ---Lv1---> Defender(1309)

Reflect Shield(1307) ---Lv5---> Devotion(1306)
Grand Cross(1303) ---Lv4---> Devotion(1306)

Demon Bane(413, Acolyte) ---Lv5---> Heal(1311) [cross-class prereq]
Heal(1311) ---Lv5---> Providence(1308)
Auto Guard(1301) ---Lv5---> Providence(1308)

Spear Mastery(700, shared) ---Lv10---> Spear Quicken(1310)
Endure(106, Swordsman) ---Lv1---> Riding(708, shared)
Riding(708) ---Lv1---> Cavalry Mastery(709, shared)
```

### 4.4 Full Prerequisite Map (All Three Classes)

| Skill | ID | Prerequisites |
|-------|-----|--------------|
| **Swordsman** | | |
| Sword Mastery | 100 | None |
| Two-Handed Sword Mastery | 101 | Sword Mastery Lv1 |
| Increase HP Recovery | 102 | None |
| Bash | 103 | None |
| Provoke | 104 | None |
| Magnum Break | 105 | Bash Lv5 |
| Endure | 106 | Provoke Lv5 |
| Moving HP Recovery | 107 | Quest (Job Lv 25) |
| Auto Berserk | 108 | Quest (Job Lv 35) |
| Fatal Blow | 109 | Quest (Job Lv 30, Bash Lv6) |
| **Knight** | | |
| Spear Mastery | 700 | None |
| Pierce | 701 | Spear Mastery Lv1 |
| Spear Stab | 702 | Pierce Lv5 |
| Brandish Spear | 703 | Spear Stab Lv3, Riding Lv1 |
| Spear Boomerang | 704 | Pierce Lv3 |
| Two-Hand Quicken | 705 | 2H Sword Mastery Lv1 |
| Auto Counter | 706 | 2H Sword Mastery Lv1 |
| Bowling Bash | 707 | Bash Lv10, Magnum Break Lv3, 2HSM Lv5, THQ Lv10, AC Lv5 |
| Riding | 708 | Endure Lv1 |
| Cavalier Mastery | 709 | Riding Lv1 |
| Charge Attack | 710 | Quest (Job Lv 40) |
| One-Hand Quicken | 711 | THQ Lv10, Knight Spirit soul link |
| **Crusader** | | |
| Faith | 1300 | None |
| Auto Guard | 1301 | None |
| Holy Cross | 1302 | Faith Lv3 |
| Grand Cross | 1303 | Holy Cross Lv6, Faith Lv10 |
| Shield Charge | 1304 | Auto Guard Lv5 |
| Shield Boomerang | 1305 | Shield Charge Lv3 |
| Devotion | 1306 | Reflect Shield Lv5, Grand Cross Lv4 |
| Reflect Shield | 1307 | Auto Guard Lv3, Shield Boomerang Lv3 |
| Providence | 1308 | Divine Protection Lv5 (Acolyte), Heal Lv5 (Crusader 1311) |
| Defender | 1309 | Shield Boomerang Lv1 |
| Spear Quicken | 1310 | Spear Mastery Lv10 (shared, ID 700) |
| Heal | 1311 | Demon Bane Lv5 (Acolyte, ID 413), Faith Lv10 |
| Cure | 1312 | Faith Lv5 |
| Shrink | 1313 | Quest (Auto Guard active required) |

---

## 5. Shared Skills between Knight and Crusader

Three Knight skills are shared with the Crusader class:

| Skill | ID | Shared | Notes |
|-------|-----|--------|-------|
| Spear Mastery | 700 | Knight + Crusader | Same skill ID, same mechanics |
| Riding (Peco Peco Ride) | 708 | Knight + Crusader | Same mount system, Crusader gets "Grand Peco" visual |
| Cavalier Mastery | 709 | Knight + Crusader | Same ASPD recovery mechanics |

**Implementation Requirement**: These skills have `classId: 'knight'` in the skill data. Crusaders need access via a `sharedWith: ['crusader']` field or equivalent shared skill mechanism in `getAvailableSkills()`.

**Crusader Skill Tree Positions**: These shared skills need `sharedTreePos` entries for the Crusader skill tree UI, and `iconClassId` for icon resolution.

---

## 6. Special Mechanics

### 6.1 Peco Peco Riding System

**Mount Effects Summary:**

| Effect | Mounted | Dismounted |
|--------|---------|------------|
| Movement Speed | +25% (like Increase AGI) | Normal |
| Weight Capacity | +1000 | Normal |
| ASPD | 50% * (1 + CM_Lv*0.2) | 100% |
| Spear vs Medium | 100% damage | 75% damage (size penalty) |
| Spear Mastery | +5 ATK/lv | +4 ATK/lv |
| Brandish Spear | Available | Unavailable |

**ASPD Formula While Mounted (Pre-Renewal)**:
```
mountedASPD_modifier = 0.5 + CavalierMasteryLv * 0.1
effective_ASPD = baseASPD * mountedASPD_modifier
```
At Cavalier Mastery Lv5: `1.0` (full ASPD restored).

**Implementation**: `player.isMounted` boolean flag. Toggle via `/mount` command or Riding skill activation. Affects: movement speed, weight capacity, ASPD calculation, Spear Mastery bonus, spear size penalty override, Brandish Spear availability.

### 6.2 Auto Counter Facing System

**Facing Requirement**:
- Caster must be facing the attacker for counter to trigger
- If attacked from behind or side, counter does NOT trigger and caster takes damage normally
- In practice, facing is determined by the direction the player last moved or attacked

**Implementation Options**:
1. **Full directional**: Track player facing (8 directions), calculate angle between player facing and attacker direction, allow counter within ~90 degree cone
2. **Simplified**: Always allow counter (skip facing check for v1), add facing later
3. **Recommended**: Start with simplified (always counter), add facing as polish

### 6.3 Bowling Bash Chain Reaction

**Chain Reaction Flow**:
1. Caster uses Bowling Bash on primary target
2. Primary target is knocked back 1 cell in caster's facing direction
3. Primary target takes 2 hits at full ATK%
4. After knockback, check 3x3 area around primary target's new position
5. Any enemies in that 3x3 area take splash damage (1 hit at full ATK%)
6. Splash targets are also knocked back 1 cell
7. After EACH splash knockback, check 3x3 again for MORE enemies (recursive)
8. Max chain depth = skill level (prevents infinite cascading)
9. Already-hit enemies are NOT hit again in the same chain

**Gutter Lines (Pre-Renewal Exploit)**:
- Map cells where `X % 40 == 0` or `Y % 40 == 0` had different BB behavior
- This was an unintended mechanic/exploit
- **Do NOT implement** -- use consistent behavior throughout

### 6.4 Spear Skills and Size Penalty

In RO Classic, weapon types have size penalties:

| Weapon Type | vs Small | vs Medium | vs Large |
|-------------|----------|-----------|----------|
| Dagger | 100% | 75% | 50% |
| 1H Sword | 75% | 100% | 75% |
| 2H Sword | 75% | 75% | 100% |
| Spear (Dismounted) | 75% | 75% | 100% |
| **Spear (Mounted)** | 75% | **100%** | 100% |
| Mace | 75% | 100% | 100% |
| Axe | 50% | 75% | 100% |

When mounted, spears deal 100% to Medium size (normally 75%). This is critical for Pierce effectiveness since Medium is the most common monster size.

### 6.5 Grand Cross Hybrid Damage

Grand Cross is unique as a **hybrid ATK+MATK** skill:
- The ATK portion is reduced by enemy DEF
- The MATK portion is reduced by enemy MDEF
- Both are combined, then multiplied by skill%, then by Holy element modifier
- Requires a specialized damage calculation function
- Self-damage is also hybrid and affected by caster's own Holy/Demi-Human resistances

### 6.6 Devotion Damage Redirect

**Damage Pipeline with Devotion**:
1. Attack targets protected player
2. Calculate damage using **protected player's DEF/MDEF** (not Crusader's)
3. Before damage is applied to protected player, redirect to Crusader
4. Crusader takes the calculated damage instead
5. Check if Crusader's Auto Guard triggers (can block redirected attacks)
6. Check if Crusader's Reflect Shield triggers (reflects back to original attacker)
7. If Crusader HP < 25% MaxHP after damage, Devotion breaks
8. If redirected damage would kill Crusader, Crusader dies; excess NOT passed to protected player

### 6.7 Weapon Skills Cannot Critical

In pre-renewal RO, weapon-based offensive skills (Bash, Pierce, Bowling Bash, etc.) **cannot critical hit**. The `!isSkill` guard prevents critical rolls on skill damage.

Exceptions:
- Auto Counter's counter-attack (forced critical)
- Specific skills marked with `forceCrit` flag

---

## 7. Passive Skills Detailed Mechanics

### 7.1 Mastery ATK Application Order

Mastery ATK (from Sword Mastery, 2H Sword Mastery, Spear Mastery) is added to the damage calculation AFTER DEF reduction but BEFORE elemental modifier:

```
1. Calculate base damage (StatusATK + WeaponATK + random variance)
2. Apply skill multiplier
3. Subtract hard DEF (equipment DEF)
4. Subtract soft DEF (VIT-based)
5. ADD mastery ATK (bypasses DEF)
6. ADD refine ATK bonus
7. Apply elemental modifier
8. Apply size modifier
9. Apply card modifiers
10. Final damage
```

### 7.2 HP Regen System

**Base HP Regen** (per 10-second tick):
```
baseRegen = max(1, floor(MaxHP / 200)) + floor(VIT / 5)
```

**With Increase HP Recovery** (ID 102):
```
totalRegen = baseRegen + (SkillLv * 5) + floor(MaxHP * SkillLv * 0.002)
```

**Modifiers**:
- Standing: 100% regen rate
- Sitting: 200% (2x) regen rate
- Moving (without Moving HP Recovery): 0% (no regen)
- Moving (with Moving HP Recovery): 50% regen rate
- Overweight (>50% weight): 0% (no regen)
- Overweight (>90% weight): 0% regen AND 0% SP regen

### 7.3 Faith MaxHP Integration

Faith's MaxHP bonus must be added in `getEffectiveStats()`:
```
effectiveMaxHP = baseMaxHP + faithBonus
```
Where `faithBonus = faithLevel * 200` (Lv10 = +2000 MaxHP).

This must be applied BEFORE any percentage-based MaxHP modifications.

---

## 8. Implementation Checklist

### 8.1 Swordsman Skills

| ID | Skill | Handler | Passive | Buff | Status |
|----|-------|---------|---------|------|--------|
| 100 | Sword Mastery | N/A | DONE | N/A | COMPLETE |
| 101 | 2H Sword Mastery | N/A | DONE | N/A | COMPLETE |
| 102 | Increase HP Recovery | N/A | PARTIAL | N/A | Needs: regen tick integration, item heal bonus |
| 103 | Bash | DONE | N/A | N/A | Needs: HIT bonus, stun formula fix |
| 104 | Provoke | DONE | N/A | DONE | Needs: Undead/Boss immunity check |
| 105 | Magnum Break | DONE | N/A | PARTIAL | Needs: targetType fix, HP cost, fire endow buff |
| 106 | Endure | DONE | N/A | DONE | Needs: 7-hit counter |
| 107 | Moving HP Recovery | N/A | DONE | N/A | Needs: 50% moving regen rate |
| 108 | Auto Berserk | DONE | N/A | DONE | Needs: -55% DEF penalty verification |
| 109 | Fatal Blow | N/A | DONE | N/A | Needs: baseLevel scaling formula |

### 8.2 Knight Skills

| ID | Skill | Handler | Passive | Buff | Priority |
|----|-------|---------|---------|------|----------|
| 700 | Spear Mastery | N/A | PARTIAL | N/A | HIGH (mounted bonus) |
| 701 | Pierce | DONE (rewritten) | N/A | N/A | DONE |
| 702 | Spear Stab | DONE (rewritten) | N/A | N/A | DONE |
| 703 | Brandish Spear | DONE (rewritten) | N/A | N/A | DONE |
| 704 | Spear Boomerang | DONE | N/A | N/A | Needs: range scaling, ranged flag |
| 705 | Two-Hand Quicken | DONE | N/A | DONE | DONE |
| 706 | Auto Counter | DONE | N/A | N/A | Needs: facing (deferred) |
| 707 | Bowling Bash | DONE (rewritten) | N/A | N/A | DONE |
| 708 | Riding | N/A | DEF ONLY | N/A | MEDIUM (mount system) |
| 709 | Cavalier Mastery | N/A | DEF ONLY | N/A | MEDIUM (ASPD interaction) |
| 710 | Charge Attack | DONE (rewritten) | N/A | N/A | DONE |
| 711 | One-Hand Quicken | NOT IMPL | N/A | N/A | LOW (Soul Link) |

### 8.3 Crusader Skills

| ID | Skill | Handler | Passive | Buff | Priority |
|----|-------|---------|---------|------|----------|
| 1300 | Faith | N/A | DONE | N/A | DONE |
| 1301 | Auto Guard | DONE | N/A | DONE | DONE |
| 1302 | Holy Cross | DONE | N/A | N/A | DONE |
| 1303 | Grand Cross | DONE (rewritten) | N/A | N/A | DONE |
| 1304 | Shield Charge | DONE | N/A | N/A | DONE |
| 1305 | Shield Boomerang | DONE | N/A | N/A | DONE |
| 1306 | Devotion | DONE | N/A | DONE | DONE |
| 1307 | Reflect Shield | DONE | N/A | DONE | DONE |
| 1308 | Providence | DONE | N/A | DONE | DONE |
| 1309 | Defender | DONE | N/A | DONE | DONE |
| 1310 | Spear Quicken | DONE | N/A | DONE | DONE |
| 1311 | Heal (Crusader) | DONE (alias) | N/A | N/A | DONE |
| 1312 | Cure (Crusader) | DONE (alias) | N/A | N/A | DONE |
| 1313 | Shrink | DONE | N/A | DONE | DONE |

### 8.4 Systems Checklist

| System | Status | Notes |
|--------|--------|-------|
| Peco Peco mount system | PARTIAL | `/mount` command works, `isMounted` flag exists. Full ASPD/weight/size integration pending |
| Spear Mastery mounted bonus | PENDING | Need `isMounted` check in `getPassiveSkillBonuses()` |
| Auto Counter facing | DEFERRED | Simplified (always counter) implemented, facing check deferred |
| Bowling Bash chain reaction | DONE | Recursive knockback + 3x3 check, max depth = skill level |
| Grand Cross hybrid damage | DONE | Combined ATK+MATK calculation with self-damage |
| Devotion damage redirect | DONE | Full redirect in enemy damage pipeline |
| Shield Boomerang custom damage | DONE | Uses batk + shieldWeight/10 + refine*5, always Neutral |
| Shared skill access (Crusader) | DONE | `sharedTreePos` + `iconClassId` mechanism |
| ASPD mutual exclusion | DONE | THQ/SQ/AR only strongest wins |
| Equipment buff cancel | DONE | Unequip 2H sword cancels THQ, unequip shield cancels AG/RS/Defender |

---

## 9. Gap Analysis vs Current Codebase

### 9.1 Data Definition Gaps (ro_skill_data.js / ro_skill_data_2nd.js)

| ID | Field | Current | Correct | Priority |
|----|-------|---------|---------|----------|
| 103 | cooldown | 700 | 0 | MEDIUM |
| 105 | targetType | 'ground' | 'aoe' | HIGH |
| 105 | afterCastDelay/cooldown | ACD:0, CD:2000 | ACD:2000, CD:0 | MEDIUM |
| 701 | cooldown | 500 | 0 | MEDIUM |
| 702 | cooldown | 500 | 0 | MEDIUM |
| 703 | cooldown | 1000 | 0 (use ACD:1000) | MEDIUM |
| 704 | range | 600 (flat) | level-dependent: 150-550 | MEDIUM |
| 704 | cooldown | 1000 | 0 (use ACD:1000) | MEDIUM |
| 706 | cooldown | 500 | 0 | MEDIUM |
| 707 | cooldown | 500 | 0 | MEDIUM |
| 707 | castTime | missing | 700 | HIGH |
| 707 | prerequisites | missing MB+2HSM | add {105,3} and {101,5} | HIGH |
| 710 | effectValue | 300 (flat) | distance-based 100-500% | CRITICAL |
| 710 | range | 900 | 700 | MEDIUM |
| 1301 | effectValue | WRONG formula | [5,10,14,18,21,24,26,28,29,30] | HIGH |
| 1303 | SP cost | 37+i*3 | 30+7*lv | HIGH |
| 1304 | effectValue | 120+i*30 | 120+i*20 | HIGH |
| 1305 | effectValue | 130+i*20 | 100+30*lv | HIGH |
| 1307 | prerequisites | AG Lv3 only | AG Lv3 + SB Lv3 | HIGH |
| 1308 | prerequisites | AG Lv5 + Acolyte Heal Lv5 | DP Lv5 + Crusader Heal Lv5 | HIGH |
| 1309 | prerequisites | Shield Charge Lv5 | Shield Boomerang Lv1 | HIGH |
| 1310 | effectValue | flat 30 | scales 21-30% | MEDIUM |

### 9.2 Missing Handler Functionality

| Feature | Description | Priority |
|---------|-------------|----------|
| Increase HP Recovery regen integration | `hpRegenBonus` not added to HP regen tick | HIGH |
| Increase HP Recovery item heal bonus | +10%/lv not applied in inventory:use | HIGH |
| Bash HIT bonus | Multiplicative HIT bonus not in damage calc | LOW |
| Fatal Blow baseLevel scaling | Uses flat formula instead of `(lv-5)*baseLv/10` | LOW |
| Magnum Break HP cost | HP drain not implemented | HIGH |
| Magnum Break fire endow | 20% fire bonus buff not applied | HIGH |
| Endure 7-hit counter | Hit counter tracking not implemented | MEDIUM |
| Moving HP Recovery 50% | Currently 100% instead of 50% while moving | MEDIUM |
| Spear Mastery mounted bonus | Only applies +4/lv, missing +5/lv when mounted | HIGH |
| Spear Boomerang range scaling | Flat range instead of level-dependent | MEDIUM |
| Mount system full integration | ASPD penalty, weight bonus, size override | MEDIUM |
| One-Hand Quicken | Not implemented (Soul Link, deferred) | LOW |

### 9.3 Resolved Issues (Previously Fixed)

The following items were identified in previous audits and have been fixed:

- Knight: Spear Stab/Bowling Bash/Brandish Spear AoE rewrites (2026-03-20)
- Knight: Charge Attack distance-based damage + crash fixes (2026-03-20)
- Knight: THQ/SQ/AR ASPD stats window display (2026-03-20)
- Crusader: Grand Cross full rewrite (WeaponATK only, correct MATK, 41-cell diamond) (2026-03-20b)
- Crusader: Faith MaxHP in getEffectiveStats (2026-03-20b)
- Crusader: Auto Guard in all 3 enemy damage paths (2026-03-20b)
- Crusader: Shrink flat 50% knockback (2026-03-20b)
- Crusader: Shared Knight skills in Crusader tab (sharedTreePos/iconClassId) (2026-03-20b)
- Crusader: Heal/Providence prereq fixes (2026-03-20b)
- Crusader: Spear subType detection fix (2026-03-20b)
- Crusader: SUPPORTIVE_SKILLS whitelist for friendly targeting (2026-03-20b)
- Crusader: Shield Boomerang custom damage (batk + shieldWeight/10 + refine*5) (2026-03-20b)
- Crusader: Devotion full damage redirect + CC break + out-of-range break (2026-03-20b)
- Equipment buff cancel on unequip (2026-03-20b)
- ASPD mutual exclusion (strongest wins among THQ/SQ/AR) (2026-03-20b)

---

## Sources

- [iRO Wiki Classic - Swordman](https://irowiki.org/classic/Swordman)
- [iRO Wiki Classic - Knight](https://irowiki.org/classic/Knight)
- [iRO Wiki Classic - Crusader](https://irowiki.org/classic/Crusader)
- [iRO Wiki - Bowling Bash](https://irowiki.org/wiki/Bowling_Bash)
- [iRO Wiki Classic - Bowling Bash](https://irowiki.org/classic/Bowling_Bash)
- [iRO Wiki - Brandish Spear](https://irowiki.org/wiki/Brandish_Spear)
- [iRO Wiki Classic - Brandish Spear](https://irowiki.org/classic/Brandish_Spear)
- [iRO Wiki - Grand Cross](https://irowiki.org/wiki/Grand_Cross)
- [iRO Wiki Classic - Grand Cross](https://irowiki.org/classic/Grand_Cross)
- [iRO Wiki - Shield Boomerang](https://irowiki.org/wiki/Shield_Boomerang)
- [iRO Wiki Classic - Shield Boomerang](https://irowiki.org/classic/Shield_Boomerang)
- [iRO Wiki - Counter Attack](https://irowiki.org/wiki/Counter_Attack)
- [iRO Wiki Classic - Counter Attack](https://irowiki.org/classic/Counter_Attack)
- [iRO Wiki - Magnum Break](https://irowiki.org/wiki/Magnum_Break)
- [iRO Wiki Classic - Magnum Break](https://irowiki.org/classic/Magnum_Break)
- [iRO Wiki - Peco Peco Ride](https://irowiki.org/wiki/Peco_Peco_Ride)
- [iRO Wiki Classic - Peco Peco Ride](https://irowiki.org/classic/Peco_Peco_Ride)
- [iRO Wiki Classic - Cavalier Mastery](https://irowiki.org/classic/Cavalier_Mastery)
- [iRO Wiki - Twohand Quicken](https://irowiki.org/wiki/Twohand_Quicken)
- [iRO Wiki - Spear Quicken](https://irowiki.org/wiki/Spear_Quicken)
- [iRO Wiki - Charge Attack](https://irowiki.org/wiki/Charge_Attack)
- [iRO Wiki - Holy Cross](https://irowiki.org/wiki/Holy_Cross)
- [iRO Wiki - Provoke](https://irowiki.org/wiki/Provoke)
- [iRO Wiki - Endure](https://irowiki.org/wiki/Endure)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [RateMyServer - Swordman Skills](https://ratemyserver.net/index.php?page=skill_db&jid=1)
- [RateMyServer - Knight Skills](https://ratemyserver.net/index.php?page=skill_db&jid=7)
- [RateMyServer - Crusader Skills](https://ratemyserver.net/index.php?page=skill_db&jid=14)
- [RateMyServer - Bowling Bash](https://ratemyserver.net/index.php?page=skill_db&skid=62)
- [RateMyServer - Grand Cross](https://ratemyserver.net/index.php?page=skill_db&skid=254)
- [RateMyServer - Shield Boomerang](https://ratemyserver.net/index.php?page=skill_db&skid=251)
- [RateMyServer - Auto Guard](https://ratemyserver.net/index.php?page=skill_db&skid=249)
- [RateMyServer - Shield Reflect](https://ratemyserver.net/index.php?page=skill_db&skid=252)
- [RateMyServer - Spear Quicken](https://ratemyserver.net/index.php?page=skill_db&skid=258)
- [RateMyServer - Charge Attack](https://ratemyserver.net/index.php?page=skill_db&skid=1001)
- [RateMyServer - Official Bowling Bash Guide](https://forum.ratemyserver.net/guides/guide-official-bowling-bash/)
- [RateMyServer - Magnum Break Elements](https://forum.ratemyserver.net/swordsman/(non-renewal)-magnum-break-elements/)
- [RateMyServer - Pre-Renewal Status Resistance Formulas](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [rAthena - Pre-Renewal Skill Tree (skill_tree.yml)](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_tree.yml)
- [rAthena - Pre-Renewal Skill DB (skill_db.yml)](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [rAthena - Skill Config (skill.conf)](https://github.com/rathena/rathena/blob/master/conf/battle/skill.conf)
- [rAthena - Bowling Bash Fix PR #5598](https://github.com/rathena/rathena/pull/5598/files/195dad0e168f975274990273a97a248940ccfc16)
- [rAthena - Bowling Bash Knockback Issue #8435](https://github.com/rathena/rathena/issues/8435)
- [rAthena - Spear Stab Issue #932](https://github.com/rathena/rathena/issues/932)
- [rAthena - Grand Cross Issue #1140](https://github.com/rathena/rathena/issues/1140)
- [Paladin Guide: Striving To Perfectness](https://write.ratemyserver.net/ragnoark-online-character-guides/paladin-guide-striving-to-perfectness/)
- [Lord Knight Comprehensive Guide](https://write.ratemyserver.net/ragnoark-online-character-guides/lord-knight-a-comprehensive-guide-for-pvmwoepvp/)
