# Dancer Class — Complete Pre-Renewal Implementation Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Dancer_Skills_Audit](Dancer_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Research Date:** 2026-03-15
**Sources:** iRO Wiki (classic + renewal changelogs), RateMyServer (pre-renewal columns), rAthena pre-renewal DB, iRO Wiki skill histories
**Scope:** All Dancer skills (11 solo + 8 ensemble + 1 quest), class constants, performance system architecture, ensemble system architecture
**Related Docs:**
- `Hunter_Bard_Dancer_Skills_Research.md` — Existing research for all three 2nd Archer classes
- `Archer_Skills_Audit_And_Fix_Plan.md` — Parent class (Archer) skill audit
- `ro_skill_data_2nd.js` — Existing Dancer skill definitions (IDs 1520-1557)

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Class Constants & Data Tables](#2-class-constants--data-tables)
3. [Inherited Archer Skills](#3-inherited-archer-skills)
4. [Solo Skills — Complete Reference](#4-solo-skills--complete-reference)
   - 4.1 Dance Lessons (Passive)
   - 4.2 Adaptation to Circumstances (Amp)
   - 4.3 Encore
   - 4.4 Slinging Arrow (Throw Arrow)
   - 4.5 Ugly Dance (Hip Shaker)
   - 4.6 Scream (Dazzler)
   - 4.7 Humming (Focus Ballet)
   - 4.8 Please Don't Forget Me (Slow Grace)
   - 4.9 Fortune's Kiss (Lady Luck)
   - 4.10 Service for You (Gypsy's Kiss)
   - 4.11 Moonlit Water Mill (Sheltering Bliss)
5. [Quest Skill](#5-quest-skill)
   - 5.1 Charming Wink
6. [Ensemble Skills — Complete Reference](#6-ensemble-skills--complete-reference)
   - 6.1 Lullaby
   - 6.2 Mr. Kim A Rich Man (Mental Sensing)
   - 6.3 Eternal Chaos (Down Tempo)
   - 6.4 A Drum on the Battlefield (Battle Theme)
   - 6.5 The Ring of Nibelungen (Harmonic Lick)
   - 6.6 Loki's Veil (Classical Pluck)
   - 6.7 Into the Abyss (Power Cord)
   - 6.8 Invulnerable Siegfried (Acoustic Rhythm)
7. [Performance System Architecture](#7-performance-system-architecture)
8. [Ensemble System Architecture](#8-ensemble-system-architecture)
9. [Skill Tree & Prerequisites](#9-skill-tree--prerequisites)
10. [Current Implementation Status](#10-current-implementation-status)
11. [New Systems Required](#11-new-systems-required)
12. [Implementation Priority](#12-implementation-priority)
13. [Integration Points with Existing Systems](#13-integration-points-with-existing-systems)

---

## 1. Class Overview

| Property | Value |
|----------|-------|
| **Full Name** | Dancer |
| **Base Class** | Archer |
| **Class Tier** | 2-2 (Second Class) |
| **Gender** | Female only (male counterpart: Bard) |
| **Transcendent** | Gypsy (female) |
| **Job Level Cap** | 50 |
| **Class Change Location** | Comodo |
| **Class Change Requirements** | Item collection, written test, dance test |
| **Quest Job Level for Quest Skill** | Job Level 40 (Charming Wink) |
| **Total Skills** | 19 (11 solo + 8 ensemble) + 1 quest skill |
| **Total Skill Points Needed** | 91 |
| **Primary Weapons** | Whip (dance skills), Bow (archer skills), Dagger (utility/dance cancel) |
| **Role** | Support / AoE debuffer / party buffer |

**Key Class Mechanics:**
- Performance system: AoE ground effects centered on dancer, SP drain while active, movement restricted
- Ensemble system: Combined skills with a Bard in same party, both must be adjacent
- Weapon switching: Equipping a dagger cancels active performance (alternative to Adaptation)
- Can use Slinging Arrow during a performance (unique attack capability while dancing)

---

## 2. Class Constants & Data Tables

### 2.1 Job Stat Bonuses (Per Job Level)

| Stat | Total Bonus | Per-Level Breakdown |
|------|------------|---------------------|
| STR | +2 | Lv 10, 38 |
| AGI | +7 | Lv 2, 6, 14, 22, 30, 42, 46 |
| VIT | +3 | Lv 18, 34, 50 |
| INT | +5 | Lv 3, 11, 23, 31, 43 |
| DEX | +5 | Lv 7, 19, 27, 39, 47 |
| LUK | +8 | Lv 1, 5, 9, 15, 26, 35, 44, 48 |

### 2.2 HP/SP Growth Coefficients

Already in `ro_exp_tables.js`:
```js
dancer: { HP_JOB_A: 0.75, HP_JOB_B: 3, SP_JOB: 6 }
```

### 2.3 ASPD Base Delays

Already in `ro_exp_tables.js`:
```js
dancer: { bare_hand: 45, dagger: 55, bow: 62, whip: 58 }
```

### 2.4 Weapon Size Modifiers

Already in `ro_damage_formulas.js`:
```js
whip: { small: 75, medium: 100, large: 50 }
```
Whips are Medium-optimal weapons. 75% vs Small, 100% vs Medium, 50% vs Large.

### 2.5 Ranged Weapon Classification

Already handled in `ro_damage_formulas.js` — `whip` is classified as ranged:
```js
const isRanged = weaponType === 'bow' || weaponType === 'gun' || weaponType === 'instrument' || weaponType === 'whip';
```

### 2.6 Class Progression

Already in `ro_skill_data.js`:
```js
'dancer': ['novice', 'archer', 'dancer']
```

---

## 3. Inherited Archer Skills

Dancers inherit ALL Archer skills (IDs 300-306):
- **Owl's Eye** (ID 300) — +1 DEX/level (passive)
- **Vulture's Eye** (ID 301) — +1 range/level with bow, +1 HIT/level (passive)
- **Improve Concentration** (ID 302) — +AGI/DEX % buff, reveal hidden
- **Double Strafe** (ID 303) — 2-hit bow attack (requires bow)
- **Arrow Shower** (ID 304) — AoE bow attack with knockback (requires bow)
- **Arrow Crafting** (ID 305) — Convert items to arrows (quest skill)
- **Arrow Repel** (ID 306) — Knockback bow attack (quest skill)

**CRITICAL NOTE:** Bow skills (Double Strafe, Arrow Shower, Arrow Repel) require a **bow** equipped. Dance skills require a **whip** equipped. The Dancer must weapon-swap between bow and whip to use different skill sets. Slinging Arrow is the only offensive skill that uses a whip.

---

## 4. Solo Skills — Complete Reference

### 4.1 Dance Lessons (ID 1520) — Passive

| Field | Value |
|-------|-------|
| **iRO Name** | Dance Lessons |
| **kRO Name** | Dancing Lesson |
| **Max Level** | 10 |
| **Type** | Passive |
| **Target** | None (always active) |
| **Weapon** | Whip only (ATK bonus); MaxSP always applies |
| **Prerequisites** | None |
| **Skill ID** | 1520 |

#### Effects Per Level

| Level | Whip ATK Bonus | MaxSP Bonus | Dance Skill Amplification |
|-------|---------------|-------------|--------------------------|
| 1 | +3 | +1% (Renewal only) | See per-skill formulas |
| 2 | +6 | +2% | |
| 3 | +9 | +3% | |
| 4 | +12 | +4% | |
| 5 | +15 | +5% | |
| 6 | +18 | +6% | |
| 7 | +21 | +7% | |
| 8 | +24 | +8% | |
| 9 | +27 | +9% | |
| 10 | +30 | +10% | |

**ATK Bonus Formula:** `ATK += DanceLessonsLv * 3` (Weapon Mastery type, whip only)

**Dance Skill Amplification:** Dance Lessons level appears as a term in every dance skill formula. The specific amplification differs per skill — see each dance skill's formula section.

**Pre-Renewal Note:** The +Critical and +MaxSP% bonuses are Renewal additions. In pre-renewal, Dance Lessons only provides:
1. +3 ATK per level with whips
2. Amplification of dance effects (via formula terms)
3. Partial reduction of movement speed penalty while performing (undocumented exact formula)

**Current Implementation Status:** Exists at ID 1520 with `effectValue: (i+1)*3`. ATK bonus correct. Dance amplification NOT implemented (no performance system yet).

---

### 4.2 Adaptation to Circumstances / Amp (ID 1523)

| Field | Value |
|-------|-------|
| **iRO Name** | Amp |
| **kRO Name** | Adaptation to Circumstances |
| **Max Level** | 1 |
| **Type** | Active |
| **Target** | Self |
| **SP Cost** | 1 |
| **Cast Time** | 0 (instant) |
| **After-Cast Delay** | 0 |
| **Cooldown** | 0 |
| **Prerequisites** | None |
| **Skill ID** | 1523 |

#### Mechanics (Pre-Renewal)

- **Effect:** Cancels the currently active Performance Skill (dance, song, or ensemble)
- **Restriction:** Cannot be used in the first 5 seconds or last 5 seconds of a performance's duration
- **Ensemble:** Only the caster who initiated the ensemble can use Adaptation
- **After cancellation:** The same performance cannot be immediately recast (5-second lockout on that specific skill)
- **Alternative:** Equipping a dagger weapon also cancels the active performance, but this bypasses the 5-second restriction and has no cast delay

**Current Implementation Status:** Exists at ID 1523. No handler — performance system not yet implemented.

---

### 4.3 Encore (ID 1524)

| Field | Value |
|-------|-------|
| **iRO Name** | Encore |
| **kRO Name** | Encore |
| **Max Level** | 1 |
| **Type** | Active |
| **Target** | Self |
| **SP Cost** | 1 (base) + half the recast skill's SP cost |
| **Cast Time** | 0 (instant) |
| **After-Cast Delay** | 0 |
| **Cooldown** | 0 |
| **Prerequisites** | Adaptation Lv 1 (ID 1523) |
| **Weapon** | Whip required |
| **Skill ID** | 1524 |

#### Mechanics (Pre-Renewal)

- **Effect:** Recast the last Performance Skill (dance, song, or ensemble) at half SP cost
- **Half-SP Calculation:** `actualCost = 1 + floor(lastSkillSPCost / 2)`
- **Failure:** If the prerequisites for the last skill are not met (wrong weapon, no Bard for ensemble, etc.), consumes only 1 SP and fails silently
- **Memory:** Tracks the last performance used by this character across sessions (persists until a new performance is used)
- **Can be used while performing:** No (must cancel first with Adaptation, then Encore)

**Current Implementation Status:** Exists at ID 1524. No handler — performance system not yet implemented.

---

### 4.4 Slinging Arrow / Throw Arrow (ID — needs assignment)

| Field | Value |
|-------|-------|
| **iRO Name** | Slinging Arrow |
| **kRO Name** | Throw Arrow |
| **Max Level** | 5 |
| **Type** | Offensive, Physical |
| **Target** | Single enemy |
| **Element** | Arrow property (inherits equipped arrow element) |
| **Range** | 9 cells |
| **Weapon** | Whip required |
| **Catalyst** | 1 Arrow consumed per cast |
| **Cast Time** | 1,500 ms (1.5 seconds, pre-renewal) |
| **After-Cast Delay** | 0 |
| **Cooldown** | 0 |
| **Prerequisites** | Dance Lessons Lv 3 (ID 1520) |

#### Damage Formula (Pre-Renewal)

| Level | ATK% | SP Cost |
|-------|------|---------|
| 1 | 100% | 1 |
| 2 | 140% | 3 |
| 3 | 180% | 5 |
| 4 | 220% | 7 |
| 5 | 260% | 9 |

**Formula:** `ATK% = 60 + (SkillLv * 40)`
**SP Cost:** `(SkillLv * 2) - 1`

**Special Mechanics:**
- Single hit (pre-renewal). Renewal changed to 2 hits.
- **Can be used DURING a performance.** This is the only offensive skill usable while dancing — the dancer does not need to cancel her performance.
- Arrow element determines attack element (like bow skills)
- Arrow is consumed even if the attack misses

**NOTE:** The RateMyServer pre-renewal column shows different values: 150/175/200/225/250% ATK. The iRO Wiki changelog shows 100/140/180/220/260%. The iRO Wiki changelog is more reliable for pre-renewal formulas. However, the iRO Classic page showed yet another set (150%x2, 190%x2, etc.) which appears to be Renewal data mislabeled. **Use the iRO Wiki changelog formula:** `60 + 40*Lv`.

**Current Implementation Status:** NOT in `ro_skill_data_2nd.js`. Needs to be added. No handler exists.

---

### 4.5 Ugly Dance / Hip Shaker (ID 1525)

| Field | Value |
|-------|-------|
| **iRO Name** | Hip Shaker |
| **kRO Name** | Ugly Dance |
| **Max Level** | 5 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 7x7 cells |
| **SP Cost** | 20 + (SkillLv * 3) = 23, 26, 29, 32, 35 |
| **Cast Time** | 0 (instant) |
| **After-Cast Delay** | 0 |
| **Cooldown** | 0 |
| **Duration** | 30 seconds |
| **SP Drain (maintenance)** | 1 SP every 3 seconds from caster |
| **Weapon** | Whip required |
| **Prerequisites** | Dance Lessons Lv 1 (ID 1520) + Adaptation Lv 1 (ID 1523) |
| **Skill ID** | 1525 |

#### SP Drain Per Tick (Pre-Renewal)

**Formula:** `SP_drained_per_tick = 5 + (5 * SkillLv)` from ENEMIES in the AoE

| Level | SP Drain/Tick | Tick Interval | Max Ticks | Total Max SP Drain |
|-------|--------------|---------------|-----------|-------------------|
| 1 | 10 SP | 3 seconds | 11 | 110 SP |
| 2 | 15 SP | 3 seconds | 11 | 165 SP |
| 3 | 20 SP | 3 seconds | 11 | 220 SP |
| 4 | 25 SP | 3 seconds | 11 | 275 SP |
| 5 | 30 SP | 3 seconds | 11 | 330 SP |

**Mechanics:**
- Drains SP from ALL enemies (and players in PvP) within the 7x7 AoE
- SP drain begins upon entering the area and repeats every 3 seconds
- Maximum 11 ticks per cast (total 33 seconds, but duration is 30s — first tick is immediate on entry)
- Drained SP is NOT recovered by the Dancer
- **PvP Note:** Works on both monsters and players, NOT PvP-only
- **Stacking:** When cast over another Dance skill, Hip Shaker is always treated as skill level 1
- **Ignores Land Protector** (special exception)
- **Monsters have no SP stat** in this project, so against PvE targets this skill has no meaningful effect. It is primarily a PvP utility skill.

**Current Implementation Status:** Exists at ID 1525 with `spCost: 23+i*2` (WRONG — should be `23+i*3`), `effectValue: 5+i*3` (should be `5+i*5` for drain/tick). No handler — performance system not yet implemented.

---

### 4.6 Scream / Dazzler (ID 1526)

| Field | Value |
|-------|-------|
| **iRO Name** | Dazzler |
| **kRO Name** | Scream |
| **Max Level** | 5 |
| **Type** | Active (NOT a performance) |
| **Target** | Screen-wide AoE (all entities in visible range) |
| **AoE** | Entire screen (approximately 15x20 cells) |
| **Cast Time** | 0 (instant) |
| **After-Cast Delay** | 300 ms |
| **Cooldown** | 3,000 ms (3 seconds, pre-renewal) |
| **Weapon** | None required (can use with any weapon) |
| **Prerequisites** | Encore Lv 1 (ID 1524) |
| **Skill ID** | 1526 |

#### Stun Effect Per Level

| Level | Stun Chance | Stun Duration | SP Cost |
|-------|-------------|---------------|---------|
| 1 | 30% | 5 seconds | 12 |
| 2 | 35% | 5 seconds | 14 |
| 3 | 40% | 5 seconds | 16 |
| 4 | 45% | 5 seconds | 18 |
| 5 | 50% | 5 seconds | 20 |

**Formula:** `StunChance = 25 + (SkillLv * 5)%`, **SP Cost:** `10 + (SkillLv * 2)`

**Mechanics:**
- **Affects ALL entities on screen** including party members and allies (friend/foe)
- Stun check is per-target: each target rolls independently
- Standard VIT-based stun resistance applies: `resistChance = VIT * 1%` (approx)
- Boss monsters are immune to stun (standard boss immunity)
- Triggers a random quote flavor text regardless of success
- This is NOT a performance skill — it does NOT require a whip, does NOT restrict movement, and does NOT drain SP
- Counterpart of Bard's Frost Joker (which applies Freeze instead of Stun)

**Current Implementation Status:** Exists at ID 1526 with `spCost: 12+i*2` (correct), `effectValue: 10+i*5` (stun chance — close, should be `25+5*i` for `30-50%`, currently gives `15-35%`). No handler.

---

### 4.7 Humming / Focus Ballet (ID 1522)

| Field | Value |
|-------|-------|
| **iRO Name** | Focus Ballet |
| **kRO Name** | Humming |
| **Max Level** | 10 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 7x7 cells (pre-renewal) |
| **Duration** | 60 seconds (pre-renewal; 180s in renewal) |
| **SP Drain (maintenance)** | 1 SP every 5 seconds from caster |
| **Weapon** | Whip required |
| **Prerequisites** | Ugly Dance Lv 3 (ID 1525) |
| **Skill ID** | 1522 |

#### HIT Bonus Formula (Pre-Renewal)

```
HIT_boost = (SkillLv * 2) + floor(DancerDEX / 10) + DanceLessonsLv
```

| SkillLv | Base HIT (DEX=1, DL=0) | With DL10 | With DL10 + DEX 99 |
|---------|------------------------|-----------|---------------------|
| 1 | 2 | 12 | 21 |
| 2 | 4 | 14 | 23 |
| 3 | 6 | 16 | 25 |
| 4 | 8 | 18 | 27 |
| 5 | 10 | 20 | 29 |
| 6 | 12 | 22 | 31 |
| 7 | 14 | 24 | 33 |
| 8 | 16 | 26 | 35 |
| 9 | 18 | 28 | 37 |
| 10 | 20 | 30 | 39 |

**Maximum Bonus:** At Lv 10, DL 10, DEX 120: `20 + 12 + 10 = 42 HIT`

#### SP Cost Per Level

**Formula:** `SP = 20 + (SkillLv * 2)`

| Level | SP Cost |
|-------|---------|
| 1 | 22 |
| 2 | 24 |
| 3 | 26 |
| 4 | 28 |
| 5 | 30 |
| 6 | 32 |
| 7 | 34 |
| 8 | 36 |
| 9 | 38 |
| 10 | 40 |

**Applies To:** All party members standing within the AoE, including the Dancer herself.
**Does NOT Apply To:** Enemies.

**Current Implementation Status:** Exists at ID 1522 with `spCost: 22+i*4` (WRONG — should be `22+i*2`), `effectValue: 2+i*2` (base HIT only — missing Dance Lessons and DEX scaling). No handler.

---

### 4.8 Please Don't Forget Me / Slow Grace (ID 1527)

| Field | Value |
|-------|-------|
| **iRO Name** | Slow Grace |
| **kRO Name** | Please Don't Forget Me |
| **Max Level** | 10 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 7x7 cells (pre-renewal) |
| **Duration** | 180 seconds |
| **SP Drain (maintenance)** | 1 SP every 10 seconds from caster |
| **Weapon** | Whip required |
| **Prerequisites** | Ugly Dance Lv 3 (ID 1525) |
| **Skill ID** | 1527 |

#### Effect Formulas (Pre-Renewal)

**ASPD Reduction:**
```
ASPD_drop% = (SkillLv * 3) + floor(DancerDEX / 10) + DanceLessonsLv
```

**Movement Speed Reduction:**
```
MoveSpeed_drop% = (SkillLv * 2) + floor(DancerAGI / 10) + ceil(DanceLessonsLv / 2)
```

| SkillLv | ASPD Drop (DEX=1, DL=0) | MoveSpeed Drop (AGI=1, DL=0) | ASPD (DL10, DEX99) | Move (DL10, AGI99) |
|---------|------------------------|------------------------------|--------------------|--------------------|
| 1 | 3% | 2% | 22% | 11% |
| 2 | 6% | 4% | 25% | 13% |
| 3 | 9% | 6% | 28% | 15% |
| 4 | 12% | 8% | 31% | 17% |
| 5 | 15% | 10% | 34% | 19% |
| 6 | 18% | 12% | 37% | 21% |
| 7 | 21% | 14% | 40% | 23% |
| 8 | 24% | 16% | 43% | 25% |
| 9 | 27% | 18% | 46% | 27% |
| 10 | 30% | 20% | 49% | 29% |

**Maximum Values:** At Lv 10, DL 10, DEX 120, AGI 120:
- ASPD Drop: `30 + 12 + 10 = 52%`
- Move Drop: `20 + 12 + 5 = 37%`

#### SP Cost Per Level

**Formula:** `SP = 25 + (SkillLv * 3)`

| Level | SP Cost |
|-------|---------|
| 1 | 28 |
| 2 | 31 |
| 3 | 34 |
| 4 | 37 |
| 5 | 40 |
| 6 | 43 |
| 7 | 46 |
| 8 | 49 |
| 9 | 52 |
| 10 | 55 |

**Applies To:** ALL enemies (monsters and players) within the AoE.
**Does NOT Apply To:** Party members/allies.
**Boss Note:** Full effect applies to boss monsters (ASPD and move speed reduction).
**Cancels:** Removes any active ASPD or move speed buffs on affected targets while they remain in the AoE.

**Current Implementation Status:** Exists at ID 1527 with `spCost: 26+i*4` (WRONG — should be `28+i*3`), `effectValue: 3+i*2` (simplified — missing formulas). No handler.

---

### 4.9 Fortune's Kiss / Lady Luck (ID 1528)

| Field | Value |
|-------|-------|
| **iRO Name** | Lady Luck |
| **kRO Name** | Fortune's Kiss |
| **Max Level** | 10 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 7x7 cells (pre-renewal) |
| **Duration** | 120 seconds (pre-renewal; 180s renewal) |
| **SP Drain (maintenance)** | 1 SP every 4 seconds from caster |
| **Weapon** | Whip required |
| **Prerequisites** | Ugly Dance Lv 3 (ID 1525) |
| **Skill ID** | 1528 |

#### CRIT Bonus Formula (Pre-Renewal)

```
CRIT_boost = SkillLv + floor(DancerLUK / 10) + DanceLessonsLv
```

| SkillLv | Base CRIT (LUK=1, DL=0) | With DL10 | With DL10 + LUK 99 |
|---------|--------------------------|-----------|---------------------|
| 1 | 1 | 11 | 20 |
| 2 | 2 | 12 | 21 |
| 3 | 3 | 13 | 22 |
| 4 | 4 | 14 | 23 |
| 5 | 5 | 15 | 24 |
| 6 | 6 | 16 | 25 |
| 7 | 7 | 17 | 26 |
| 8 | 8 | 18 | 27 |
| 9 | 9 | 19 | 28 |
| 10 | 10 | 20 | 29 |

**Maximum Bonus:** At Lv 10, DL 10, LUK 120: `10 + 12 + 10 = 32 CRIT`

**Pre-Renewal Note:** Fortune's Kiss does NOT provide critical damage bonus in pre-renewal. The +2%/level critical damage is a Renewal addition.

#### SP Cost Per Level

**Formula:** `SP = 40 + (SkillLv * 3)`

| Level | SP Cost |
|-------|---------|
| 1 | 43 |
| 2 | 46 |
| 3 | 49 |
| 4 | 52 |
| 5 | 55 |
| 6 | 58 |
| 7 | 61 |
| 8 | 64 |
| 9 | 67 |
| 10 | 70 |

**Applies To:** All party members in the AoE, including the Dancer.
**Does NOT Apply To:** Enemies.

**Current Implementation Status:** Exists at ID 1528 with `spCost: 30+i*4` (WRONG — should be `43+i*3`), `effectValue: 2+i*2` (simplified — missing LUK and DL scaling). Prerequisites list `1520 lv7` (should be `1525 lv3` = Ugly Dance 3). No handler.

---

### 4.10 Service for You / Gypsy's Kiss (ID 1521)

| Field | Value |
|-------|-------|
| **iRO Name** | Gypsy's Kiss |
| **kRO Name** | Service for You |
| **Max Level** | 10 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 7x7 cells (pre-renewal) |
| **Duration** | 180 seconds |
| **SP Drain (maintenance)** | 1 SP every 5 seconds from caster |
| **Weapon** | Whip required |
| **Prerequisites** | Ugly Dance Lv 3 (ID 1525) |
| **Skill ID** | 1521 |

#### Effect Formulas (Pre-Renewal)

**MaxSP Increase:**
```
MaxSP_boost% = 15 + SkillLv + floor(DancerINT / 10)
```

**SP Consumption Reduction:**
```
SP_reduction% = 20 + (SkillLv * 3) + floor(DancerINT / 10) + ceil(DanceLessonsLv / 2)
```

| SkillLv | MaxSP% (INT=1, DL=0) | SPReduction% (INT=1, DL=0) | MaxSP% (INT99) | SPReduction% (INT99, DL10) |
|---------|-----------------------|---------------------------|----------------|---------------------------|
| 1 | 16% | 23% | 25% | 37% |
| 2 | 17% | 26% | 26% | 40% |
| 3 | 18% | 29% | 27% | 43% |
| 4 | 19% | 32% | 28% | 46% |
| 5 | 20% | 35% | 29% | 49% |
| 6 | 21% | 38% | 30% | 52% |
| 7 | 22% | 41% | 31% | 55% |
| 8 | 23% | 44% | 32% | 58% |
| 9 | 24% | 47% | 33% | 61% |
| 10 | 25% | 50% | 34% | 64% |

**Maximum Values:** At Lv 10, DL 10, INT 120:
- MaxSP: `15 + 10 + 12 = 37%`
- SP Reduction: `20 + 30 + 12 + 5 = 67%` (likely capped at some value in practice)

#### SP Cost Per Level

**Formula:** `SP = 35 + (SkillLv * 5)`

| Level | SP Cost |
|-------|---------|
| 1 | 40 |
| 2 | 45 |
| 3 | 50 |
| 4 | 55 |
| 5 | 60 |
| 6 | 65 |
| 7 | 70 |
| 8 | 75 |
| 9 | 80 |
| 10 | 85 |

**Applies To:** All party members in the AoE, including the Dancer.
**Does NOT Apply To:** Enemies.

**Current Implementation Status:** Exists at ID 1521 with `spCost: 40+i*5` (correct), `effectValue: (i+1)*3` (simplified). Prerequisites list `1520 lv3` (should be `1525 lv3` for full accuracy — wiki says Hip Shaker/Ugly Dance 3 is the prereq for all offensive dances). No handler.

---

### 4.11 Moonlit Water Mill / Sheltering Bliss (ID 1540)

| Field | Value |
|-------|-------|
| **iRO Name** | Sheltering Bliss |
| **kRO Name** | Moonlit Water Mill |
| **Max Level** | 5 |
| **Type** | Performance (Dance) |
| **Target** | Self (ground AoE centered on caster) |
| **AoE** | 9x9 cells |
| **Duration** | 20 + (SkillLv * 4) seconds = 24/28/32/36/40s |
| **SP Drain (maintenance)** | 4-20 SP every 10 seconds (4*SkillLv) |
| **Weapon** | Whip required |
| **Prerequisites** | Adaptation Lv 1 (ID 1523) |
| **Skill ID** | 1540 |

#### Effect & SP Cost Per Level

| Level | Effect | Duration | SP Cost | SP Drain/10s |
|-------|--------|----------|---------|-------------|
| 1 | Block entry/attacks in AoE | 24s | 30 | 4 SP |
| 2 | Block entry/attacks in AoE | 28s | 40 | 8 SP |
| 3 | Block entry/attacks in AoE | 32s | 50 | 12 SP |
| 4 | Block entry/attacks in AoE | 36s | 60 | 16 SP |
| 5 | Block entry/attacks in AoE | 40s | 70 | 20 SP |

**Mechanics:**
- Prevents monsters and players from ENTERING the AoE
- Pushes out entities already inside when cast
- Prevents normal attacks while inside the area
- Does NOT prevent skill use
- Cannot be used in WoE or Endless Tower
- Counterpart of Bard's Unbarring Octave (which prevents skill use instead)

**Current Implementation Status:** Exists at ID 1540 with `spCost: 38+i*2` (WRONG — should be `30+i*10`). No handler.

---

## 5. Quest Skill

### 5.1 Charming Wink (ID 1529)

| Field | Value |
|-------|-------|
| **iRO Name** | Charming Wink |
| **kRO Name** | Wink of Charm |
| **Max Level** | 1 |
| **Type** | Active |
| **Target** | Single enemy |
| **Range** | 9 cells (450 UE units) |
| **SP Cost** | 40 |
| **Cast Time** | 1,000 ms (1 second) |
| **After-Cast Delay** | 1,000 ms |
| **Cooldown** | 3,000 ms |
| **Duration** | 10 seconds (charm/non-aggro) |
| **Prerequisites** | Job Level 40 + Quest completion |
| **Skill ID** | 1529 |

#### Mechanics

**Against Monsters:**
- 70% success chance to charm the target for 10 seconds
- Charmed monsters become non-aggressive (stop attacking, stop chasing)
- Only affects Demi-Human, Angel, and Demon race monsters
- Does NOT affect boss monsters or high-level monsters
- After charm ends, monster resumes normal AI behavior

**Against Players (PvP):**
- Causes Confusion (Chaos) status ailment
- Lower success rate than PvE (exact rate not documented, estimated 30-50%)
- Confusion causes movement/input reversal

**Limitations:**
- Single target only
- Cannot stack (re-applying refreshes duration)
- No damage component

**Current Implementation Status:** Exists at ID 1529 with `spCost: 20` (WRONG — should be 40), `cooldown: 3000` (correct). `questSkill: true` (correct). No handler.

---

## 6. Ensemble Skills — Complete Reference

### Ensemble System Overview

Ensemble skills are unique cooperative skills that require BOTH a Bard AND a Dancer to be:
1. In the **same party**
2. Standing on **adjacent cells** (within 1 cell of each other)
3. Both have the **same ensemble skill learned**
4. Both have their **performance weapon equipped** (instrument for Bard, whip for Dancer)

Either performer can initiate the ensemble. Both performers become locked in place (cannot move) for the duration. Both performers drain SP independently.

The effective skill level used is: `effectiveLv = floor((BardSkillLv + DancerSkillLv) / 2)`

### 6.1 Lullaby (ID 1550 Dancer / 1530 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 20 SP initial |
| **SP Drain** | 1 SP every 4 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Dancer)** | Humming Lv 10 (ID 1522) |
| **Prerequisites (Bard)** | A Whistle Lv 10 (ID 1507) |

#### Effect

- Inflicts Sleep status on all enemies within the 9x9 AoE
- Sleep chance scales with caster INT (exact formula undocumented, estimated: `baseChance + INT/5`)
- Boss monsters are immune to Sleep
- Sleep is broken by taking damage
- Applies every tick (approximately every 4 seconds) to enemies remaining in the AoE

**Current Implementation Status:** Exists at ID 1550 with basic values. No handler.

---

### 6.2 Mr. Kim A Rich Man / Mental Sensing (ID 1551 Dancer / 1531 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 20 + (SkillLv * 4) = 24/28/32/36/40 |
| **SP Drain** | 1 SP every 3 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Both)** | Invulnerable Siegfried Lv 3 (Acoustic Rhythm Lv 3) |

#### EXP Bonus Per Level

| Level | EXP Bonus |
|-------|-----------|
| 1 | +36% |
| 2 | +47% |
| 3 | +58% |
| 4 | +69% |
| 5 | +80% |

**Mechanics:**
- Increases base + job EXP gained by party members within the AoE
- Does NOT increase EXP from boss monster kills
- Stacks multiplicatively with Battle Manuals and EXP-boosting cards
- Formula: `EXP_bonus% = 25 + (SkillLv * 11)` (approximately)

**Current Implementation Status:** Exists at ID 1551 with `effectValue: i+1` (incomplete — should encode EXP bonus). No handler.

---

### 6.3 Eternal Chaos / Down Tempo (ID 1552 Dancer / 1532 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 30 SP initial |
| **SP Drain** | 1 SP every 4 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Both)** | Loki's Veil Lv 1 (Classical Pluck Lv 1) |

#### Effect

- **Reduces all VIT-derived DEF to 0** for all enemies within the AoE
- Equipment/hard DEF is NOT affected
- In pre-renewal: this zeroes `softDef = VIT * 0.8` (or however soft DEF is calculated)

**Mechanics:**
- Extremely powerful against high-VIT enemies
- Does not affect equipment DEF (hard DEF)
- Works on all enemies including boss monsters (the DEF reduction, not a status effect)

**Current Implementation Status:** Exists at ID 1552 with `effectValue: 10+i*5` (should encode the VIT DEF zero effect). No handler.

---

### 6.4 A Drum on the Battlefield / Battle Theme (ID 1553 Dancer / 1533 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 35 + (SkillLv * 5) = 40/45/50/55/60 |
| **SP Drain** | 1 SP every 5 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Dancer)** | Service for You Lv 10 (ID 1521) |
| **Prerequisites (Bard)** | Apple of Idun Lv 10 (ID 1508) |

#### Bonuses Per Level

| Level | ATK Bonus | DEF Bonus |
|-------|-----------|-----------|
| 1 | +50 | +4 |
| 2 | +75 | +6 |
| 3 | +100 | +8 |
| 4 | +125 | +10 |
| 5 | +150 | +12 |

**ATK Formula:** `ATK_bonus = 25 + (SkillLv * 25)`
**DEF Formula:** `DEF_bonus = 2 + (SkillLv * 2)`

**Applies To:** All party members within the AoE.

**Current Implementation Status:** Exists at ID 1553 with `effectValue: 5+i*5` (simplified). No handler.

---

### 6.5 The Ring of Nibelungen / Harmonic Lick (ID 1554 Dancer / 1534 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 35 + (SkillLv * 3) = 38/41/44/47/50 |
| **SP Drain** | 1 SP every 3 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Both)** | Battle Theme Lv 3 (Drum on Battlefield Lv 3) |

#### Armor-Piercing Bonus Per Level

| Level | Bonus Damage (ignores DEF) |
|-------|---------------------------|
| 1 | +75 |
| 2 | +100 |
| 3 | +125 |
| 4 | +150 |
| 5 | +175 |

**Formula:** `bonus_damage = 50 + (SkillLv * 25)` — This damage ignores DEF entirely.

**CRITICAL RESTRICTION:** Only applies to characters wielding **Weapon Level 4** weapons.

**Applies To:** All party members within the AoE who have Weapon Level 4 weapons equipped.

**Current Implementation Status:** Exists at ID 1554 with `effectValue: 15+i*15` (incorrect formula). No handler.

---

### 6.6 Loki's Veil / Classical Pluck (ID 1555 Dancer / 1535 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 15 SP initial |
| **SP Drain** | 1 SP every 4 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Dancer)** | Please Don't Forget Me Lv 10 (ID 1527) |
| **Prerequisites (Bard)** | Assassin Cross of Sunset Lv 10 (ID 1502) |

#### Effect

- **Disables ALL skills** (physical skills, magic spells, item use) for EVERYONE within the AoE
- Exception: "Longing for Freedom" (Gypsy/Minstrel skill) still works
- Boss monsters are NOT affected
- The performers themselves are also affected (cannot use skills while in their own Loki's Veil)
- Only normal attacks remain functional within the AoE

**This is a PvP-focused skill** — extremely powerful zone denial.

**Current Implementation Status:** Exists at ID 1555. No handler.

---

### 6.7 Into the Abyss / Power Cord (ID 1556 Dancer / 1536 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 10 SP initial |
| **SP Drain** | 1 SP every 5 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Both)** | Lullaby Lv 1 (ID 1550/1530) |

#### Effect

- **Removes gemstone requirements** for all skills cast by party members within the AoE
- Affects: Safety Wall (Blue Gem), Stone Curse (Red Gem), Endow skills, Warp Portal (Blue Gem), etc.
- **Exceptions:** Hocus-pocus still requires 1 Yellow Gem, Ganbantein still requires both gems
- Does NOT affect trap item requirements (traps still need the Trap item)

**Current Implementation Status:** Exists at ID 1556. No handler.

---

### 6.8 Invulnerable Siegfried / Acoustic Rhythm (ID 1557 Dancer / 1537 Bard)

| Field | Value |
|-------|-------|
| **Max Level** | 5 |
| **Type** | Ensemble |
| **AoE** | 9x9 cells |
| **Duration** | 60 seconds |
| **SP Cost** | 20 SP initial |
| **SP Drain** | 1 SP every 3 seconds from EACH performer |
| **Cast Time** | 0 |
| **Prerequisites (Dancer)** | Fortune's Kiss Lv 10 (ID 1528) |
| **Prerequisites (Bard)** | A Poem of Bragi Lv 10 (ID 1501) |

#### Resistance Bonuses Per Level

| Level | Elemental Resistance | Status Effect Resistance |
|-------|---------------------|-------------------------|
| 1 | +40% | +10% |
| 2 | +50% | +20% |
| 3 | +60% | +30% |
| 4 | +70% | +40% |
| 5 | +80% | +50% |

**Elemental Resistance Formula:** `elemResist% = 30 + (SkillLv * 10)`
**Status Resistance Formula:** `statusResist% = SkillLv * 10`

**Applies To:** All party members within the AoE.
**Element Coverage:** Fire, Water, Wind, Earth, Shadow, Undead, Holy, Ghost, Poison (all non-Neutral)
**Status Coverage:** All status effects (Stun, Freeze, Sleep, Poison, Curse, Blind, Silence, Confusion, Bleeding, etc.)

**Current Implementation Status:** Exists at ID 1557 with `effectValue: 10+i*10`. No handler.

---

## 7. Performance System Architecture

### 7.1 Overview

The Performance system governs all songs (Bard) and dances (Dancer). It is a ground-placed AoE effect system with the following universal rules:

### 7.2 Core Mechanics

1. **Placement:** Performance creates an AoE zone centered on the performer's current cell when cast
2. **Movement Lock:** The performer CANNOT move while performing (walking cancels the performance)
3. **SP Drain:** Each performance has an ongoing SP maintenance cost. If the performer runs out of SP, the performance ends automatically
4. **One Performance Limit:** Only ONE performance can be active at a time per performer. Casting a new performance replaces the old one
5. **Cancellation Methods:**
   - Adaptation to Circumstances (Amp) skill
   - Equipping a dagger/non-performance weapon
   - Walking/moving
   - Running out of SP
   - Getting hit by certain disabling status effects (Stun, Freeze, etc.)
   - Death
6. **Slinging Arrow Exception:** The performer CAN use Slinging Arrow (Throw Arrow) while performing without canceling the dance
7. **Duration:** Each performance has a fixed maximum duration. It ends when duration expires or the performer cancels it

### 7.3 AoE Behavior

- AoE is **FIXED** at the caster's position when cast — it does NOT move with the performer (but the performer cannot move anyway)
- Effects apply to entities **entering** the AoE and persist while they **remain inside**
- Some effects (like Ugly Dance SP drain) tick periodically
- Some effects (like Humming HIT boost) are persistent while standing in the AoE
- Effects are removed when the entity leaves the AoE or the performance ends

### 7.4 Performance State (Server)

```js
// Per-player performance state
player.activePerformance = {
    skillId: 1522,          // e.g., Humming
    skillLevel: 5,
    startTime: Date.now(),
    duration: 60000,        // ms
    aoeCenter: { x: 100, y: 200 },
    aoeRadius: 175,         // 3.5 cells * 50 UE/cell for 7x7
    spDrainInterval: 5000,  // ms between SP drains
    spDrainAmount: 1,
    lastSpDrain: Date.now(),
    effectType: 'buff_allies', // or 'debuff_enemies', 'drain_sp', 'block_entry'
    lastPerformanceId: null, // for Encore tracking
};
```

### 7.5 Performance Tick Loop

Performances should be checked on the existing combat tick loop (50ms). Each tick:
1. Check if performer has enough SP for next drain tick
2. Apply SP drain from performer at the defined interval
3. Apply AoE effects to entities within range:
   - **Buff performances** (Humming, Fortune's Kiss, Service for You): Apply stat buff to party members in AoE
   - **Debuff performances** (Please Don't Forget Me): Apply stat debuff to enemies in AoE
   - **Drain performances** (Ugly Dance): Drain SP from enemies per tick interval
   - **Block performances** (Moonlit Water Mill): Prevent entry + push out entities
4. Remove effects from entities that leave the AoE
5. End performance if duration expires or SP runs out

### 7.6 Client Events

```
// Server -> Client
performance:start    { characterId, skillId, skillLevel, x, y, aoeRadius, duration }
performance:tick     { characterId, skillId, affectedIds: [...] }  // optional
performance:end      { characterId, skillId, reason: 'expired'|'cancelled'|'no_sp'|'moved' }

// Client -> Server
performance:cancel   { characterId }  // Adaptation skill
```

### 7.7 Performance Visual

Client should render:
- A circular AoE indicator on the ground (like Pneuma/Safety Wall)
- The Dancer in a dancing animation (looping)
- Buff/debuff icons on affected entities within the AoE

---

## 8. Ensemble System Architecture

### 8.1 Overview

Ensemble skills extend the Performance system with two-player cooperative requirements.

### 8.2 Requirements

1. **Party:** Both Bard and Dancer must be in the same party
2. **Adjacent:** Must be standing within 1 cell of each other (8 possible adjacent cells)
3. **Weapons:** Bard must have Instrument equipped, Dancer must have Whip equipped
4. **Skill:** Both must have the ensemble skill learned at some level
5. **Gender:** Requires exactly one male (Bard) and one female (Dancer) performer

### 8.3 Skill Level Calculation

```
effectiveLevel = floor((bardSkillLevel + dancerSkillLevel) / 2)
```

This means if Bard has Lullaby Lv 3 and Dancer has Lullaby Lv 5, the effective level is `floor((3+5)/2) = 4`.

### 8.4 Ensemble State (Server)

```js
// Ensemble extends performance state
player.activeEnsemble = {
    ...performanceState,
    partnerId: 12345,           // The other performer's characterId
    partnerSkillLevel: 3,       // Partner's skill level
    effectiveLevel: 4,          // Calculated combined level
    initiatorId: 11111,         // Who started the ensemble
};
```

### 8.5 Ensemble Restrictions

- **BOTH performers are movement-locked** for the duration
- Only the **initiator** can use Adaptation to cancel the ensemble
- If EITHER performer moves, gets stunned, dies, or runs out of SP, the ensemble ends for BOTH
- Both performers drain SP independently at the ensemble's drain rate
- **Neither performer can cast other skills** while in an ensemble (except the initiator using Adaptation)
- Ensemble AoE is centered between the two performers

### 8.6 Ensemble Validation on Cast

```
1. Check caster has whip/instrument equipped
2. Check caster is in a party
3. Find adjacent party member of opposite class (Bard if caster is Dancer, vice versa)
4. Check partner has the SAME ensemble skill learned
5. Check partner has their performance weapon equipped
6. Check partner is not already performing/in an ensemble
7. Calculate effective level from both skill levels
8. Start ensemble for BOTH performers
9. Place AoE between them
```

### 8.7 Client Events

```
// Server -> Client
ensemble:start    { initiatorId, partnerId, skillId, effectiveLevel, x, y, aoeRadius, duration }
ensemble:end      { initiatorId, partnerId, skillId, reason }

// Client -> Server
// Uses same skill:use event, server detects ensemble requirement
```

---

## 9. Skill Tree & Prerequisites

### 9.1 Dancer Skill Tree Layout

```
Row 0: [Dance Lessons] [Adaptation] [Encore]
Row 1: [PDFM]          [Svc4You]    [Humming]
Row 2: [Ugly Dance]    [Fortunes]   [Scream]
Row 3: [Charm Wink Q]  [Moonlit]
Row 4: [Lullaby]       [Mr. Kim]    [EternalChaos]     [Siegfried]
Row 5: [DrumBattle]    [Nibelungen] [Loki's Veil]      [IntoAbyss]
```

### 9.2 Prerequisite Chains

#### Solo Skill Prerequisites (from Archer)

```
Archer Skills (no Dancer prereqs):
  Owl's Eye (300) -> Vulture's Eye (301) -> Improve Concentration (302)
  Double Strafe (303) -> Arrow Shower (304)

Dancer Exclusive:
  Dance Lessons (1520) -- no prereqs
    -> Ugly Dance (1525) -- requires DL Lv 1 + Adaptation Lv 1
      -> Humming (1522) -- requires Ugly Dance Lv 3
      -> Please Don't Forget Me (1527) -- requires Ugly Dance Lv 3
      -> Fortune's Kiss (1528) -- requires Ugly Dance Lv 3
      -> Service for You (1521) -- requires Ugly Dance Lv 3
    -> Slinging Arrow -- requires DL Lv 3

  Adaptation (1523) -- no prereqs
    -> Encore (1524) -- requires Adaptation Lv 1
      -> Scream (1526) -- requires Encore Lv 1
    -> Ugly Dance (1525) -- requires Adaptation Lv 1 + DL Lv 1
    -> Moonlit Water Mill (1540) -- requires Adaptation Lv 1
```

#### Ensemble Skill Prerequisites (Dancer side)

```
Dance Lessons Lv 10 -> Lullaby (1550)
Dance Lessons Lv 5  -> Mr. Kim A Rich Man (1551)
Dance Lessons Lv 5  -> Eternal Chaos (1552)
Dance Lessons Lv 5  -> A Drum on the Battlefield (1553)
Dance Lessons Lv 10 -> The Ring of Nibelungen (1554)
Dance Lessons Lv 10 -> Loki's Veil (1555)
Dance Lessons Lv 5  -> Into the Abyss (1556)
Dance Lessons Lv 10 -> Invulnerable Siegfried (1557)
```

**NOTE:** The existing `ro_skill_data_2nd.js` only lists `Dance Lessons Lv X` as prerequisites for ensemble skills. The true pre-requisite chain for optimal play involves specific dance skills:
- Lullaby requires Humming Lv 10 (Dancer) / A Whistle Lv 10 (Bard)
- Siegfried requires Fortune's Kiss Lv 10 (Dancer) / Bragi Lv 10 (Bard)
- Battle Theme requires Service for You Lv 10 (Dancer) / Apple of Idun Lv 10 (Bard)
- Classical Pluck requires PDFM Lv 10 (Dancer) / ACoS Lv 10 (Bard)
- Harmonic Lick requires Battle Theme Lv 3
- Into the Abyss requires Lullaby Lv 1
- Eternal Chaos requires Classical Pluck Lv 1 (Down Tempo)
- Mental Sensing requires Acoustic Rhythm Lv 3

The current implementation in `ro_skill_data_2nd.js` simplifies all ensemble prereqs to just `Dance Lessons Lv X`. This should be corrected to match the proper skill tree.

---

## 10. Current Implementation Status

### 10.1 Skill Definitions

All 19 Dancer skills exist in `server/src/ro_skill_data_2nd.js` at IDs 1520-1557. However, they are **definition-only** — no handlers exist, no performance system exists.

### 10.2 Specific Issues Found in Existing Definitions

| Skill | Issue | Current | Correct |
|-------|-------|---------|---------|
| Ugly Dance (1525) | SP cost formula wrong | `23+i*2` | `23+i*3` |
| Ugly Dance (1525) | effectValue wrong | `5+i*3` | `5+i*5` (SP drain/tick) |
| Humming (1522) | SP cost formula wrong | `22+i*4` | `22+i*2` |
| PDFM (1527) | SP cost formula wrong | `26+i*4` | `28+i*3` |
| Fortune's Kiss (1528) | SP cost formula wrong | `30+i*4` | `43+i*3` |
| Fortune's Kiss (1528) | Prerequisites wrong | `DL Lv 7` | `Ugly Dance Lv 3` |
| Scream (1526) | effectValue wrong | `10+i*5` (15-35%) | `25+i*5` (30-50%) |
| Moonlit WM (1540) | SP cost formula wrong | `38+i*2` | `30+i*10` |
| Charming Wink (1529) | SP cost wrong | 20 | 40 |
| Slinging Arrow | MISSING | Not in file | Needs new ID and definition |
| All dances | No `isPerformance` flag | Only some marked | All dances need it |
| All ensembles | Wrong prerequisites | `DL Lv X` only | Proper skill chain |

### 10.3 Missing Skill: Slinging Arrow

Slinging Arrow (Throw Arrow) does NOT exist in `ro_skill_data_2nd.js`. It needs a new skill ID assignment and full definition. Suggested ID: **1541** (after Moonlit Water Mill at 1540).

### 10.4 Missing Systems

- Performance system (AoE ground effects, SP drain, movement lock)
- Ensemble system (Bard+Dancer cooperative casting)
- Performance cancellation logic
- Encore memory tracking
- Slinging Arrow (usable during performance)
- Charming Wink handler (charm/confusion mechanics)
- Scream/Dazzler handler (screen-wide stun)

---

## 11. New Systems Required

### 11.1 Performance System (HIGH PRIORITY)

**What:** Ground-based AoE effect system for songs and dances.

**Server Components:**
1. `player.activePerformance` state object tracking active dance/song
2. Performance tick processing in the main combat loop (every 50ms, but effect ticks at skill-specific intervals)
3. AoE entity tracking (who is inside the performance area)
4. SP drain from performer at defined intervals
5. Effect application/removal for entities entering/leaving AoE
6. Performance cancellation handlers (Adaptation, movement, death, stun, etc.)
7. Movement blocking while performing (reject position updates)
8. Weapon validation on cast (require whip for Dancer, instrument for Bard)

**Events:**
- `performance:start` — broadcast to zone when a dance/song begins
- `performance:end` — broadcast when it stops
- `performance:effect` — periodic effect notifications (optional)

**Effort:** LARGE — this is a new subsystem affecting movement, combat tick, and skill use.

### 11.2 Ensemble System (MEDIUM PRIORITY, depends on 11.1)

**What:** Cooperative two-player extension of the Performance system.

**Server Components:**
1. Adjacent cell detection (Bard and Dancer within 1 cell)
2. Party membership validation
3. Partner skill level lookup for effective level calculation
4. Dual movement lock (both performers)
5. Dual SP drain
6. Ensemble termination on either partner's disruption
7. AoE centered between the two performers

**Effort:** MEDIUM — extends Performance system, does not require new tick loops.

### 11.3 Slinging Arrow Handler (LOW PRIORITY)

**What:** Physical ranged attack usable with whip during performances.

**Server Components:**
1. Standard skill damage calculation at ATK% multiplier
2. Arrow consumption (deferred until arrow system)
3. Check for whip weapon
4. Allow use during active performance (bypass performance movement lock for this skill only)

**Effort:** SMALL — standard physical skill handler, similar to Double Strafe.

### 11.4 Scream/Dazzler Handler (LOW PRIORITY)

**What:** Screen-wide AoE stun skill.

**Server Components:**
1. Find all entities on screen (approximately 15x20 cell area around caster)
2. Apply stun chance per entity
3. VIT-based stun resistance
4. Boss immunity check
5. Note: affects allies too (party members can be stunned)

**Effort:** SMALL — similar to Frost Joker (Bard counterpart).

### 11.5 Charming Wink Handler (LOW PRIORITY)

**What:** Targeted charm/confusion skill.

**Server Components:**
1. Race check (Demi-Human, Angel, Demon only)
2. Boss immunity check
3. 70% success rate vs PvE
4. Apply charm state (set non-aggressive flag on monster AI) for 10 seconds
5. PvP: apply Confusion status instead

**Effort:** SMALL — single-target status application.

### 11.6 Dance Effect Integration (MEDIUM PRIORITY)

**What:** Each dance skill needs its specific effect integrated with the game's stat/combat systems.

| Dance | Integration Required |
|-------|---------------------|
| Humming | Add temporary HIT bonus to `getEffectiveStats()` for entities in AoE |
| PDFM | Add ASPD penalty + move speed penalty to entities in AoE |
| Fortune's Kiss | Add CRIT bonus to `getEffectiveStats()` for entities in AoE |
| Service for You | Add MaxSP bonus + SP cost multiplier for entities in AoE |
| Ugly Dance | SP subtraction per tick on entities in AoE |
| Moonlit WM | Block movement into AoE + push entities out |

Each integration needs the dancer's stats (DEX/AGI/LUK/INT) and Dance Lessons level for formula calculation.

**Effort:** MEDIUM — requires touching stat calculation, ASPD, and SP cost pipelines.

---

## 12. Implementation Priority

### Phase 1: Foundation (Required for ALL Dancer skills)
1. **Performance System Core** — AoE placement, movement lock, SP drain, cancellation, tick loop
2. **Fix skill definitions** — Correct SP costs, effectValues, prerequisites in `ro_skill_data_2nd.js`
3. **Add Slinging Arrow** — Skill definition + physical attack handler

### Phase 2: Solo Dances (Core Dancer gameplay)
4. **Humming** — HIT bonus AoE (party buff)
5. **Fortune's Kiss** — CRIT bonus AoE (party buff)
6. **Service for You** — MaxSP + SP cost reduction AoE (party buff)
7. **Please Don't Forget Me** — ASPD + move speed reduction (enemy debuff)
8. **Ugly Dance** — SP drain (enemy debuff)
9. **Moonlit Water Mill** — Entry blocking (defensive)

### Phase 3: Utility Skills
10. **Scream/Dazzler** — Screen-wide stun
11. **Charming Wink** — Targeted charm
12. **Adaptation fixes** — 5-second restriction, weapon-swap cancel
13. **Encore** — Half-SP replay of last performance

### Phase 4: Ensemble System
14. **Ensemble system core** — Adjacent detection, dual lock, effective level calc
15. **Lullaby** — AoE sleep
16. **A Drum on the Battlefield** — ATK/DEF party buff
17. **Invulnerable Siegfried** — Elemental + status resistance
18. **Into the Abyss** — Remove gemstone requirements
19. **The Ring of Nibelungen** — Lv4 weapon damage bonus
20. **Mr. Kim A Rich Man** — EXP bonus
21. **Loki's Veil** — Skill disable zone
22. **Eternal Chaos** — VIT DEF zero

---

## 13. Integration Points with Existing Systems

### 13.1 Stat System (`getEffectiveStats()` / `calculateDerivedStats()`)

- Humming: Needs to add HIT bonus from performance AoE
- Fortune's Kiss: Needs to add CRIT bonus from performance AoE
- Service for You: Needs to modify MaxSP calculation
- PDFM: Needs to modify ASPD calculation
- Dance Lessons: +3 ATK/level with whip (weapon mastery type)

### 13.2 Combat Tick Loop (`index.js` main loop)

- Performance tick processing needs to run alongside combat ticks
- SP drain from performers
- Effect application/removal for entities entering/leaving AoE
- Performance expiration checks

### 13.3 Buff/Debuff System (`ro_buff_system.js`)

- PDFM creates a debuff on enemies (ASPD reduction, move speed reduction)
- Humming/Fortune's Kiss/Service for You create buffs on allies
- These are NOT traditional buffs — they only persist while in the AoE
- May need a new "performance effect" category distinct from standard buffs

### 13.4 Movement System (position broadcasting, `PlayerInputSubsystem`)

- Performance movement lock: server must reject position updates while performing
- PDFM move speed reduction: needs to affect movement speed calculation
- Moonlit Water Mill: server must block movement into the AoE

### 13.5 Skill Use System (skill handlers in `index.js`)

- Skills cannot be used while performing (except Slinging Arrow and Adaptation)
- Loki's Veil disables ALL skills for entities in AoE
- Into the Abyss removes gemstone requirements
- Service for You reduces SP costs for skills

### 13.6 Enemy AI System (monster AI loop)

- Charming Wink: needs to set a "charmed" flag that overrides aggressive AI
- PDFM: monsters in AoE have reduced attack speed (affects attack timing)
- Lullaby: Sleep status pauses AI

### 13.7 Weight System

- Standard weight checks apply to all Dancer skills
- >= 90% weight blocks skill use (including dance activation)

### 13.8 Client-Side (C++ Subsystems)

- New `PerformanceSubsystem` (or extend `SkillVFXSubsystem`) for AoE visuals
- Performance ground indicator rendering (like Pneuma circles)
- Dancing animation triggering on character model
- Buff/debuff icon display for entities in performance AoE
- Slinging Arrow targeting while movement-locked

### 13.9 Existing Data Already in Place

| Data | Location | Status |
|------|----------|--------|
| ASPD base delays (dancer) | `ro_exp_tables.js` | Complete |
| HP/SP growth coefficients | `ro_exp_tables.js` | Complete |
| Class progression chain | `ro_skill_data.js` | Complete |
| Whip size modifiers | `ro_damage_formulas.js` | Complete |
| Whip ranged classification | `ro_damage_formulas.js` | Complete |
| Transcendent mapping (Gypsy->Dancer) | `ro_exp_tables.js` | Complete |
| Skill definitions (19 skills) | `ro_skill_data_2nd.js` | Need corrections |
| Bard counterpart skills (19 skills) | `ro_skill_data_2nd.js` | Exist (need same corrections) |

---

## Appendix A: Dancer-Specific Constants

```js
// Performance system constants
const PERFORMANCE_CONSTANTS = {
    // AoE sizes in cells (radius from center)
    STANDARD_DANCE_AOE: 3,    // 7x7 cells = 3-cell radius
    LARGE_DANCE_AOE: 4,       // 9x9 cells = 4-cell radius

    // UE unit conversion (1 cell = 50 UE units)
    STANDARD_DANCE_RADIUS_UE: 175,  // 3.5 cells * 50
    LARGE_DANCE_RADIUS_UE: 225,     // 4.5 cells * 50

    // Performance cannot be cancelled in first/last 5 seconds
    ADAPTATION_LOCKOUT_MS: 5000,

    // Scream/Dazzler screen range approximation
    SCREEN_RANGE_UE: 750,  // ~15 cells

    // Charming Wink
    CHARM_SUCCESS_RATE: 0.70,
    CHARM_DURATION_MS: 10000,
    CHARM_VALID_RACES: ['demi_human', 'angel', 'demon'],
};

// SP drain rates per performance (ms between drains)
const PERFORMANCE_SP_DRAIN = {
    ugly_dance: { interval: 3000, amount: 1 },
    humming: { interval: 5000, amount: 1 },
    please_dont_forget_me: { interval: 10000, amount: 1 },
    fortunes_kiss: { interval: 4000, amount: 1 },
    service_for_you: { interval: 5000, amount: 1 },
    moonlit_water_mill: { interval: 10000, amountPerLv: 4 },  // 4*SkillLv per tick
    // Ensembles: drain from EACH performer
    lullaby: { interval: 4000, amount: 1 },
    mr_kim: { interval: 3000, amount: 1 },
    eternal_chaos: { interval: 4000, amount: 1 },
    drum_battlefield: { interval: 5000, amount: 1 },
    nibelungen: { interval: 3000, amount: 1 },
    lokis_veil: { interval: 4000, amount: 1 },
    into_the_abyss: { interval: 5000, amount: 1 },
    siegfried: { interval: 3000, amount: 1 },
};
```

---

## Appendix B: Skill Name Cross-Reference

| Skill ID | kRO / Internal Name | iRO Name | In-Game Display |
|----------|---------------------|----------|-----------------|
| 1520 | Dance Lessons | Dance Lessons | Dance Lessons |
| 1521 | Service for You | Gypsy's Kiss | Gypsy's Kiss |
| 1522 | Humming | Focus Ballet | Focus Ballet |
| 1523 | Adaptation | Amp | Amp |
| 1524 | Encore | Encore | Encore |
| 1525 | Ugly Dance | Hip Shaker | Hip Shaker |
| 1526 | Scream | Dazzler | Dazzler |
| 1527 | Please Don't Forget Me | Slow Grace | Slow Grace |
| 1528 | Fortune's Kiss | Lady Luck | Lady Luck |
| 1529 | Charming Wink | Charming Wink | Charming Wink |
| 1540 | Moonlit Water Mill | Sheltering Bliss | Sheltering Bliss |
| (NEW) | Throw Arrow / Slinging Arrow | Slinging Arrow | Slinging Arrow |
| 1550 | Lullaby (D) | Lullaby | Lullaby |
| 1551 | Mr. Kim A Rich Man (D) | Mental Sensing | Mental Sensing |
| 1552 | Eternal Chaos (D) | Down Tempo | Down Tempo |
| 1553 | Drum on Battlefield (D) | Battle Theme | Battle Theme |
| 1554 | Ring of Nibelungen (D) | Harmonic Lick | Harmonic Lick |
| 1555 | Loki's Veil (D) | Classical Pluck | Classical Pluck |
| 1556 | Into the Abyss (D) | Power Cord | Power Cord |
| 1557 | Invulnerable Siegfried (D) | Acoustic Rhythm | Acoustic Rhythm |

---

## Appendix C: Corrected Skill Definitions

Below are the corrected definitions for `ro_skill_data_2nd.js`. Only showing skills that need changes:

```js
// Slinging Arrow (NEW — needs ID assignment, suggested 1541)
{ id: 1541, name: 'slinging_arrow', displayName: 'Slinging Arrow', classId: 'dancer',
  maxLevel: 5, type: 'active', targetType: 'single', element: 'neutral', range: 450,
  description: 'Ranged whip attack using arrows. Usable during performances.',
  icon: 'slinging_arrow', treeRow: 1, treeCol: 3,
  prerequisites: [{ skillId: 1520, level: 3 }],
  levels: genLevels(5, i => ({
      level: i+1,
      spCost: (i+1)*2 - 1,   // 1, 3, 5, 7, 9
      castTime: 1500,          // 1.5 seconds
      afterCastDelay: 0,
      cooldown: 0,
      effectValue: 60 + (i+1)*40,  // 100, 140, 180, 220, 260%
      duration: 0
  }))
},

// Ugly Dance — fix SP cost and effectValue
{ id: 1525, /* ... */, levels: genLevels(5, i => ({
    level: i+1,
    spCost: 20 + (i+1)*3,     // 23, 26, 29, 32, 35 (was 23+i*2)
    castTime: 0, cooldown: 0,
    effectValue: 5 + (i+1)*5,  // 10, 15, 20, 25, 30 SP drain/tick (was 5+i*3)
    duration: 30000
}))},

// Humming — fix SP cost
{ id: 1522, /* ... */, levels: genLevels(10, i => ({
    level: i+1,
    spCost: 20 + (i+1)*2,     // 22, 24, 26, ..., 40 (was 22+i*4)
    castTime: 0, cooldown: 0,
    effectValue: (i+1)*2,      // Base HIT: 2, 4, ..., 20 (+ DL + DEX/10 at runtime)
    duration: 60000             // 60s pre-renewal (was 180000)
}))},

// Please Don't Forget Me — fix SP cost
{ id: 1527, /* ... */, levels: genLevels(10, i => ({
    level: i+1,
    spCost: 25 + (i+1)*3,     // 28, 31, 34, ..., 55 (was 26+i*4)
    castTime: 0, cooldown: 0,
    effectValue: (i+1)*3,      // Base ASPD drop: 3, 6, ..., 30% (+ DL + DEX/10 at runtime)
    duration: 180000
}))},

// Fortune's Kiss — fix SP cost and prerequisites
{ id: 1528, /* ... */,
  prerequisites: [{ skillId: 1525, level: 3 }],  // Ugly Dance Lv 3 (was DL Lv 7)
  levels: genLevels(10, i => ({
    level: i+1,
    spCost: 40 + (i+1)*3,     // 43, 46, 49, ..., 70 (was 30+i*4)
    castTime: 0, cooldown: 0,
    effectValue: i+1,          // Base CRIT: 1, 2, ..., 10 (+ DL + LUK/10 at runtime)
    duration: 120000            // 120s pre-renewal (was 180000)
}))},

// Service for You — SP cost already correct (40+i*5), fix prerequisites
{ id: 1521, /* ... */,
  prerequisites: [{ skillId: 1525, level: 3 }],  // Ugly Dance Lv 3 (was DL Lv 3)
},

// Scream — fix effectValue
{ id: 1526, /* ... */, levels: genLevels(5, i => ({
    level: i+1,
    spCost: 10 + (i+1)*2,     // 12, 14, 16, 18, 20 (correct)
    castTime: 0,
    afterCastDelay: 300,
    cooldown: 3000,
    effectValue: 25 + (i+1)*5,  // 30, 35, 40, 45, 50% stun (was 10+i*5 = 15-35%)
    duration: 5000              // Stun duration 5 seconds
}))},

// Moonlit Water Mill — fix SP cost
{ id: 1540, /* ... */, levels: genLevels(5, i => ({
    level: i+1,
    spCost: 20 + (i+1)*10,    // 30, 40, 50, 60, 70 (was 38+i*2)
    castTime: 0, cooldown: 0,
    effectValue: 0,
    duration: 20000 + (i+1)*4000  // 24, 28, 32, 36, 40 seconds
}))},

// Charming Wink — fix SP cost
{ id: 1529, /* ... */,
  levels: [{ level: 1, spCost: 40, castTime: 1000, afterCastDelay: 1000,
             cooldown: 3000, effectValue: 70, duration: 10000 }]
},
```

---

## Appendix D: Summary of All Pre-Renewal Dance Formulas

| Skill | Formula | Variables |
|-------|---------|-----------|
| **Dance Lessons** | `ATK += DL * 3` | DL = Dance Lessons level |
| **Slinging Arrow** | `ATK% = 60 + (Lv * 40)` | Lv = skill level |
| **Ugly Dance SP drain** | `drain = 5 + (Lv * 5)` per 3s tick | Lv = skill level |
| **Humming HIT** | `HIT = (Lv * 2) + floor(DEX/10) + DL` | DEX = dancer's DEX |
| **PDFM ASPD drop** | `ASPD% = (Lv * 3) + floor(DEX/10) + DL` | DEX = dancer's DEX |
| **PDFM Move drop** | `Move% = (Lv * 2) + floor(AGI/10) + ceil(DL/2)` | AGI = dancer's AGI |
| **Fortune's Kiss CRIT** | `CRIT = Lv + floor(LUK/10) + DL` | LUK = dancer's LUK |
| **Service MaxSP** | `MaxSP% = 15 + Lv + floor(INT/10)` | INT = dancer's INT |
| **Service SP cost** | `SPred% = 20 + (Lv*3) + floor(INT/10) + ceil(DL/2)` | INT = dancer's INT |
| **Scream stun** | `Chance = 25 + (Lv * 5)%` | Modified by target VIT |
| **Charming Wink** | `70% success rate` | Only vs Demi/Angel/Demon |
| **Drum ATK** | `ATK = 25 + (Lv * 25)` | Effective ensemble level |
| **Drum DEF** | `DEF = 2 + (Lv * 2)` | Effective ensemble level |
| **Nibelungen** | `DMG = 50 + (Lv * 25)` (ignores DEF) | Lv4 weapons only |
| **Siegfried Elem** | `ElemResist% = 30 + (Lv * 10)` | All non-neutral elements |
| **Siegfried Status** | `StatusResist% = Lv * 10` | All status effects |
| **Mr. Kim EXP** | `EXP% = 25 + (Lv * 11)` (approx) | No boss EXP |

Where DL = Dance Lessons level, Lv = skill level, stats = dancer's stats at time of cast.
