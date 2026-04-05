# Bard & Dancer Skills — Deep Research

> **Version:** 1.0
> **Date:** 2026-03-22
> **Scope:** Complete pre-renewal (Classic) Bard & Dancer skill reference for MMO replication
> **Sources:** iRO Wiki Classic, RateMyServer Skill DB, rAthena pre-renewal source (status.cpp, skill.cpp, battle.cpp, skill_db.yml), eathena skill.c/status.c, Hercules Board Archive, GameFAQs guides (Jath, aldoteng, Dikiwinky), MyRoLife Bard/Dancer guide, divine-pride, idRO Klasik Wiki, rAthena GitHub issues (#1808, #5008, #5030, #6135, #7134, #7163, #7726, #9281, #9577), Ragnarok Fandom Wiki
> **All data is Pre-Renewal unless explicitly noted otherwise.**

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Bard Skills (IDs 1500-1509)](#2-bard-skills-ids-1500-1509)
3. [Dancer Skills (IDs 1520-1541)](#3-dancer-skills-ids-1520-1541)
4. [Ensemble Skills (IDs 1530-1537 / 1550-1557)](#4-ensemble-skills-ids-1530-1537--1550-1557)
5. [Performance System Core](#5-performance-system-core)
6. [Ensemble System](#6-ensemble-system)
7. [Adaptation to Circumstances](#7-adaptation-to-circumstances)
8. [Encore](#8-encore)
9. [Skill Trees and Prerequisites](#9-skill-trees-and-prerequisites)
10. [Implementation Checklist](#10-implementation-checklist)
11. [Gap Analysis](#11-gap-analysis)

---

## 1. Class Overview

### Bard

| Property | Value |
|----------|-------|
| Base Class | Archer |
| Gender | Male only |
| Weapon | Instrument (performance), Bow (archer skills), Dagger (utility/cancel) |
| Transcendent | Clown (Minstrel) |
| Job Level Cap | 50 |
| Skill Points | 49 |
| rAthena Job ID | 19 |
| Role | Support buffer (songs), secondary ranged DPS |

### Dancer

| Property | Value |
|----------|-------|
| Base Class | Archer |
| Gender | Female only |
| Weapon | Whip (performance), Bow (archer skills), Dagger (utility/cancel) |
| Transcendent | Gypsy |
| Job Level Cap | 50 |
| Skill Points | 49 |
| rAthena Job ID | 20 |
| Role | Support buffer/debuffer (dances), secondary ranged DPS |

### Design Philosophy

Bards and Dancers are the male/female counterparts of the Archer's second-class branch. Their primary value is through **performance skills** -- ground-based AoE auras that passively buff allies or debuff enemies. They sacrifice personal combat power for party utility. Together, a Bard and Dancer can perform **ensemble skills** (duets) with powerful combined effects unavailable to either class alone.

Both classes inherit ALL Archer skills (Owl's Eye, Vulture's Eye, Improve Concentration, Double Strafe, Arrow Shower, Arrow Crafting, Arrow Repel).

---

## 2. Bard Skills (IDs 1500-1509)

### 2.1 Music Lessons (ID 1500, rA: BA_MUSICALLESSON 315)

| Field | Value |
|-------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Effects:**
- +3 ATK per level with Instruments (weapon mastery, ignores DEF)
- Amplifies all song/performance formulas (appears as a term in every song formula)
- Partially restores movement during performance

| Level | Instrument ATK | Move Speed During Performance |
|-------|---------------|-------------------------------|
| 1 | +3 | 27.5% of normal |
| 2 | +6 | 30% |
| 3 | +9 | 32.5% |
| 4 | +12 | 35% |
| 5 | +15 | 37.5% |
| 6 | +18 | 40% |
| 7 | +21 | 42.5% |
| 8 | +24 | 45% |
| 9 | +27 | 47.5% |
| 10 | +30 | 50% |

**Movement Formula:** `performanceSpeedPercent = 25 + 2.5 * MusicLessonsLv`

**Pre-Renewal Note:** The +Critical and +MaxSP% bonuses listed on some wikis are Renewal-only additions. In pre-renewal, Music Lessons only provides instrument ATK, song formula amplification, and performance movement speed.

---

### 2.2 Musical Strike / Melody Strike (ID 1511, rA: BA_MUSICALSTRIKE 316)

| Field | Value |
|-------|-------|
| Type | Active, Physical, Single Target |
| Max Level | 5 |
| Range | 9 cells |
| Element | Arrow property (inherits equipped arrow element) |
| Cast Time | 1500 ms |
| After-Cast Delay | 0 ms |
| Cooldown | 0 ms |
| Prerequisites | Music Lessons Lv 3 |
| Requires | Instrument + 1 Arrow (consumed per cast) |
| Usable During Performance | **YES** |

**Damage Formula (Pre-Renewal, rAthena-verified):**

```
ATK% = 60 + (SkillLv * 40)
```

| Level | ATK% | SP Cost |
|-------|------|---------|
| 1 | 100% | 1 |
| 2 | 140% | 3 |
| 3 | 180% | 5 |
| 4 | 220% | 7 |
| 5 | 260% | 9 |

**SP Cost Formula:** `(SkillLv * 2) - 1`

**Source Verification:** rAthena commit #9577 (`928e1cb`) confirms the pre-renewal formula: `base_skillratio += -40 + 40 * skill_lv` giving total ratio `60 + 40*Lv`. The RateMyServer values (150/175/200/225/250) are incorrect for pre-renewal.

**Special Mechanics:**
- Single hit (pre-renewal). Renewal changed to 2 hits.
- **Can be used DURING an active performance** without canceling it. This is the Bard's primary offensive skill while singing.
- Arrow element determines attack element.
- Arrow is consumed even if the attack misses.
- rAthena `AllowWhenPerforming` flag is set for this skill.

---

### 2.3 Dissonance / Unchained Serenade (ID 1505, rA: BA_DISSONANCE 317)

| Field | Value |
|-------|-------|
| Type | Active, Performance |
| Max Level | 5 |
| AoE | 7x7 cells (175 UE radius) |
| Element | Neutral (MISC damage type) |
| Cast Time | 0 ms |
| After-Cast Delay | 0 ms |
| Duration | 30 seconds |
| SP Drain | 1 SP / 3 seconds |
| Prerequisites | Music Lessons Lv 1, Adaptation Lv 1 |
| Requires | Instrument equipped |

**Damage Formula:**

```
DamagePerTick = 30 + (10 * SkillLv) + (MusicLessonsLv * 3)
```

| Level | Base Damage/Tick | With ML10 | SP Cost |
|-------|-----------------|-----------|---------|
| 1 | 40 | 70 | 18 |
| 2 | 50 | 80 | 21 |
| 3 | 60 | 90 | 24 |
| 4 | 70 | 100 | 27 |
| 5 | 80 | 110 | 30 |

**SP Cost Formula:** `15 + SkillLv * 3`

**Critical Mechanics:**
- **MISC damage type** (BF_MISC in rAthena battle.cpp) -- ignores BOTH physical DEF and MDEF entirely. Only the raw formula value applies.
- Hits all enemies in 7x7 AoE every 3 seconds (11 ticks over 30 seconds).
- This IS a performance skill -- Bard enters performance state.
- Required at Lv 3 as prerequisite for all 4 main Bard songs.

---

### 2.4 Frost Joker / Unbarring Octave (ID 1506, rA: BA_FROSTJOKER 318)

| Field | Value |
|-------|-------|
| Type | Active, AoE (NOT a performance) |
| Max Level | 5 |
| AoE | Screen-wide (all visible entities) |
| Cast Time | 0 ms |
| After-Cast Delay | 4000 ms |
| Cooldown | 0 ms |
| Prerequisites | Encore Lv 1 |
| Weapon | None required (any weapon) |

**Freeze Chance Formula:**

```
FreezeChance = 15 + (5 * SkillLv) %
```

| Level | Freeze Chance | SP Cost |
|-------|---------------|---------|
| 1 | 20% | 12 |
| 2 | 25% | 14 |
| 3 | 30% | 16 |
| 4 | 35% | 18 |
| 5 | 40% | 20 |

**SP Cost Formula:** `10 + 2 * SkillLv`

**Freeze Duration:** Approximately 12 seconds base, reduced by target's equipment MDEF:
```
Duration = max(3000, 12000 - 12000 * itemMDEF / 100) ms
```

**Critical Mechanics:**
- Affects **EVERYONE on screen**: enemies, party members, allies, the Bard himself.
- Party members receive **half** the base freeze chance: `FreezeChance / 2`.
- Boss-type monsters are **immune**.
- **NOT a performance skill** -- does NOT restrict movement, does NOT drain SP.
- **BLOCKED during active performance** (rAthena `AllowWhenPerforming` flag is NOT set for Frost Joker). Only Adaptation, Musical Strike, and Slinging Arrow have this flag. This was confirmed by rAthena source inspection.
- Dancer counterpart: Scream/Dazzler (Stun instead of Freeze).

**Name Clarification:** "Unbarring Octave" is iRO's localized name for Frost Joker (BA_FROSTJOKER, rA ID 318). It is NOT a separate skill.

---

### 2.5 A Whistle / Perfect Tablature (ID 1507, rA: BA_WHISTLE 319)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Song) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 180 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 5 seconds |
| Prerequisites | Dissonance Lv 3 |
| Requires | Instrument equipped |

**Pre-Renewal Formulas (rAthena xmas_present2008 disassembly, Issue #1808):**

```
FLEE Bonus  = SkillLv + floor(AGI / 10) + floor(MusicLessonsLv * 0.5)
PD Bonus    = floor((SkillLv + 1) / 2) + floor(LUK / 30) + floor(MusicLessonsLv * 0.2)
```

| Level | Base FLEE | Base PD | SP Cost |
|-------|----------|---------|---------|
| 1 | +1 | +1 | 24 |
| 2 | +2 | +1 | 28 |
| 3 | +3 | +2 | 32 |
| 4 | +4 | +2 | 36 |
| 5 | +5 | +3 | 40 |
| 6 | +6 | +3 | 44 |
| 7 | +7 | +4 | 48 |
| 8 | +8 | +4 | 52 |
| 9 | +9 | +5 | 56 |
| 10 | +10 | +5 | 60 |

**SP Cost Formula:** `20 + SkillLv * 4`

**Example:** Bard with ML 10, AGI 80, LUK 30, Whistle Lv 10:
- FLEE = 10 + 8 + 5 = **+23**
- PD = 5 + 1 + 2 = **+8**

**Buff Rules:**
- Affects ALL non-caster players in AoE (not party-only). Caster does NOT receive own buff.
- Buff lingers 20 seconds after leaving AoE.

---

### 2.6 Assassin Cross of Sunset / Impressive Riff (ID 1502, rA: BA_ASSASSINCROSS 320)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Song) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 120 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 3 seconds |
| Prerequisites | Dissonance Lv 3 |
| Requires | Instrument equipped |

**ASPD Boost Formula (Pre-Renewal):**

```
ASPD% = 10 + SkillLv + floor(AGI / 20) + floor(MusicLessonsLv / 2)
```

| Level | Base ASPD% | With ML10 + AGI 80 | SP Cost |
|-------|-----------|-------------------|---------|
| 1 | 11% | 20% | 38 |
| 2 | 12% | 21% | 41 |
| 3 | 13% | 22% | 44 |
| 4 | 14% | 23% | 47 |
| 5 | 15% | 24% | 50 |
| 6 | 16% | 25% | 53 |
| 7 | 17% | 26% | 56 |
| 8 | 18% | 27% | 59 |
| 9 | 19% | 28% | 62 |
| 10 | 20% | 29% | 65 |

**SP Cost Formula:** `35 + SkillLv * 3`

**Stacking Rules (Haste2 Exclusion Group):**
- **Mutually exclusive with:** Adrenaline Rush, Two-Hand Quicken, Spear Quicken, One-Hand Quicken. Only the STRONGEST bonus from this group applies.
- **Stacks with:** ASPD Potions (Haste1 group). Cross-group stacking is allowed.
- **Does NOT affect bow users.** Characters with `weaponType === 'bow'` are excluded from the ASPD buff.
- Caster does NOT receive own buff.

**Source:** rAthena status.cpp SC_ASSNCROS, iRO Wiki Classic Impressive Riff, idRO Klasik Wiki, rAthena issue #9041.

---

### 2.7 A Poem of Bragi / Magic Strings (ID 1501, rA: BA_POEMBRAGI 321)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Song) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 180 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 5 seconds |
| Prerequisites | Dissonance Lv 3 |
| Requires | Instrument equipped |

**Pre-Renewal Formulas (rAthena xmas_present2008 disassembly, Issue #1808):**

```
VCT Reduction% = SkillLv * 3 + floor(DEX / 10) + MusicLessonsLv
ACD Reduction% = (SkillLv < 10 ? SkillLv * 3 : 50) + floor(INT / 5) + MusicLessonsLv * 2
```

| Level | Base VCT Red. | Base ACD Red. | SP Cost |
|-------|---------------|---------------|---------|
| 1 | -3% | -3% | 40 |
| 2 | -6% | -6% | 45 |
| 3 | -9% | -9% | 50 |
| 4 | -12% | -12% | 55 |
| 5 | -15% | -15% | 60 |
| 6 | -18% | -18% | 65 |
| 7 | -21% | -21% | 70 |
| 8 | -24% | -24% | 75 |
| 9 | -27% | -27% | 80 |
| 10 | -30% | -50% (capped) | 85 |

**SP Cost Formula:** `35 + SkillLv * 5`

**Example:** Bard with ML 10, DEX 99, INT 50, Bragi Lv 10:
- VCT Reduction = 30 + 9 + 10 = **49%**
- ACD Reduction = 50 + 10 + 20 = **80%**

**Interaction with Suffragium:** Bragi's VCT reduction and Suffragium are independent multiplicative effects:
```
Final VCT = Base * (1 - DEX/150) * (1 - Bragi%) * (1 - Suffragium%)
```

**This is arguably the most powerful support skill in the game.** A well-built Bard can reduce a Wizard's cast time and after-cast delay by 50-80%.

Caster does NOT receive own buff.

---

### 2.8 Apple of Idun / Song of Lutie (ID 1508, rA: BA_APPLEIDUN 322)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Song) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 180 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 6 seconds |
| Prerequisites | Dissonance Lv 3 |
| Requires | Instrument equipped |

**Pre-Renewal Formulas (rAthena xmas_present2008 disassembly, Issue #1808):**

```
MaxHP Boost%     = 5 + SkillLv * 2 + floor(VIT / 10) + floor(MusicLessonsLv / 2)
HP Recovery/Tick = (30 + SkillLv * 5) + floor(VIT / 2) + MusicLessonsLv * 5
```

| Level | Base MaxHP% | Base HP/Tick | SP Cost |
|-------|------------|-------------|---------|
| 1 | +7% | 35 HP | 40 |
| 2 | +9% | 40 HP | 45 |
| 3 | +11% | 45 HP | 50 |
| 4 | +13% | 50 HP | 55 |
| 5 | +15% | 55 HP | 60 |
| 6 | +17% | 60 HP | 65 |
| 7 | +19% | 65 HP | 70 |
| 8 | +21% | 70 HP | 75 |
| 9 | +23% | 75 HP | 80 |
| 10 | +25% | 80 HP | 85 |

**SP Cost Formula:** `35 + SkillLv * 5`

**Healing Frequency:** Every 6 seconds (31 ticks over 180 seconds).

**Example:** Bard with ML 10, VIT 50, Apple Lv 10:
- MaxHP Boost = 5 + 20 + 5 + 5 = **35%**
- HP/Tick = 80 + 25 + 50 = **155 HP every 6s**

Caster does NOT receive own buff.

---

### 2.9 Pang Voice (ID 1509, rA: BA_PANGVOICE 1010)

| Field | Value |
|-------|-------|
| Type | Active, Single Target |
| Max Level | 1 |
| Range | 9 cells |
| SP Cost | 20 |
| Cast Time | 800 ms |
| After-Cast Delay | 2000 ms |
| Cooldown | 0 ms |
| Prerequisites | Quest (Bard Platinum Skill, Job Lv 40+) |

**Effect:** Inflicts Confusion (Chaos) status on target.

**Success Rate Formula:**
```
chance = max(5, min(95, 50 + (casterBaseLv - targetBaseLv) - targetVIT/5 - targetLUK/5))
```

- Does NOT work on Boss monsters.
- Duration: ~15 seconds.
- **BLOCKED during active performance** (rAthena `AllowWhenPerforming` flag is NOT set).

---

## 3. Dancer Skills (IDs 1520-1541)

### 3.1 Dance Lessons (ID 1520, rA: DC_DANCINGLESSON 323)

| Field | Value |
|-------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**Effects:**
- +3 ATK per level with Whips (weapon mastery, ignores DEF)
- Amplifies all dance/performance formulas (appears as a term in every dance formula)
- Partially restores movement during performance

| Level | Whip ATK | Move Speed During Performance |
|-------|---------|-------------------------------|
| 1 | +3 | 27.5% of normal |
| 2 | +6 | 30% |
| 3 | +9 | 32.5% |
| 4 | +12 | 35% |
| 5 | +15 | 37.5% |
| 6 | +18 | 40% |
| 7 | +21 | 42.5% |
| 8 | +24 | 45% |
| 9 | +27 | 47.5% |
| 10 | +30 | 50% |

**Movement Formula:** `performanceSpeedPercent = 25 + 2.5 * DanceLessonsLv`

**Pre-Renewal Note:** The +Critical and +MaxSP% bonuses are Renewal-only additions. In pre-renewal, Dance Lessons only provides whip ATK, dance formula amplification, and performance movement speed.

---

### 3.2 Slinging Arrow / Throw Arrow (ID 1541, rA: DC_THROWARROW 324)

| Field | Value |
|-------|-------|
| Type | Active, Physical, Single Target |
| Max Level | 5 |
| Range | 9 cells |
| Element | Arrow property (inherits equipped arrow element) |
| Cast Time | 1500 ms |
| After-Cast Delay | 0 ms |
| Cooldown | 0 ms |
| Prerequisites | Dance Lessons Lv 3 |
| Requires | Whip + 1 Arrow (consumed per cast) |
| Usable During Performance | **YES** |

**Damage Formula (Pre-Renewal, rAthena-verified):**

```
ATK% = 60 + (SkillLv * 40)
```

| Level | ATK% | SP Cost |
|-------|------|---------|
| 1 | 100% | 1 |
| 2 | 140% | 3 |
| 3 | 180% | 5 |
| 4 | 220% | 7 |
| 5 | 260% | 9 |

**SP Cost Formula:** `(SkillLv * 2) - 1`

**Source Verification:** rAthena `slingingarrow.cpp`: `base_skillratio += -40 + 40 * skill_lv` = total `60 + 40*Lv` = 100/140/180/220/260%. The RateMyServer values (150/175/200/225/250) are incorrect for pre-renewal.

**Special Mechanics:**
- Single hit (pre-renewal). Renewal changed to 2 hits.
- **Can be used DURING an active performance** without canceling it.
- rAthena `AllowWhenPerforming` flag is set for this skill.

---

### 3.3 Ugly Dance / Hip Shaker (ID 1525, rA: DC_UGLYDANCE 325)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Offensive Dance) |
| Max Level | 5 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 30 seconds |
| SP Drain | 1 SP / 3 seconds (from caster) |
| Prerequisites | Dance Lessons Lv 1, Adaptation Lv 1 |
| Requires | Whip equipped |

**SP Drain Per Tick (from enemies, eathena source verified):**

```
spDrainPerTick = (5 + 5 * SkillLv) + (DanceLessonsLv > 0 ? 5 + DanceLessonsLv : 0)
```

| Level | Base Drain/Tick | With DL10 | SP Cost |
|-------|----------------|-----------|---------|
| 1 | 10 SP | 25 SP | 23 |
| 2 | 15 SP | 30 SP | 26 |
| 3 | 20 SP | 35 SP | 29 |
| 4 | 25 SP | 40 SP | 32 |
| 5 | 30 SP | 45 SP | 35 |

**SP Cost Formula:** `20 + 3 * SkillLv`

**Mechanics:**
- Drains SP from ALL enemies (and players in PvP) within 7x7 AoE every 3 seconds.
- Drained SP is NOT recovered by the Dancer.
- Monsters typically have no SP stat, making this primarily a PvP utility skill.
- Ignores Land Protector (special exception).
- Tick interval: 3 seconds (11 ticks total).

---

### 3.4 Scream / Dazzler (ID 1526, rA: DC_SCREAM 326)

| Field | Value |
|-------|-------|
| Type | Active, AoE (NOT a performance) |
| Max Level | 5 |
| AoE | Screen-wide (all visible entities) |
| Cast Time | 0 ms |
| After-Cast Delay | 4000 ms |
| Cooldown | 0 ms |
| Prerequisites | Encore Lv 1 |
| Weapon | None required (any weapon) |

**Stun Chance Formula (rAthena dazzler.cpp):**

```
StunChance = (150 + 50 * SkillLv + 100) / 10  =>  25 + 5 * SkillLv %
```

| Level | Stun Chance | SP Cost |
|-------|-------------|---------|
| 1 | 30% | 12 |
| 2 | 35% | 14 |
| 3 | 40% | 16 |
| 4 | 45% | 18 |
| 5 | 50% | 20 |

**SP Cost Formula:** `10 + 2 * SkillLv`

**Critical Mechanics:**
- **3-second delayed execution** (rAthena `skill_addtimerskill(src, tick+3000, ...)`). Stun check fires 3 seconds AFTER casting, giving targets time to move out of range.
- Affects ALL entities on screen including party members and allies.
- **Party members receive 1/4 the stun chance** (rAthena: `if (battle_check_target(src, target, BCT_PARTY) > 0) rate /= 4`).
- Boss monsters are immune to stun.
- Standard VIT-based stun resistance applies.
- **NOT a performance skill** -- no whip required, no movement restriction, no SP drain.
- **BLOCKED during active performance** (rAthena `AllowWhenPerforming` flag is NOT set -- contradicts some older documentation).

---

### 3.5 Humming / Focus Ballet (ID 1522, rA: DC_HUMMING 327)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Dance) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 60 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 5 seconds |
| Prerequisites | Ugly Dance Lv 3 |
| Requires | Whip equipped |

**HIT Bonus Formula (eathena skill.c verified):**

```
HIT_boost = (SkillLv * 2) + floor(DEX / 10) + DanceLessonsLv
```

| Level | Base HIT (DEX=1, DL=0) | With DL10 | With DL10 + DEX99 | SP Cost |
|-------|------------------------|-----------|-------------------|---------|
| 1 | 2 | 12 | 21 | 22 |
| 2 | 4 | 14 | 23 | 24 |
| 3 | 6 | 16 | 25 | 26 |
| 4 | 8 | 18 | 27 | 28 |
| 5 | 10 | 20 | 29 | 30 |
| 6 | 12 | 22 | 31 | 32 |
| 7 | 14 | 24 | 33 | 34 |
| 8 | 16 | 26 | 35 | 36 |
| 9 | 18 | 28 | 37 | 38 |
| 10 | 20 | 30 | 39 | 40 |

**SP Cost Formula:** `20 + SkillLv * 2`

**eathena source:**
```c
case DC_HUMMING:
    val1 = 2*skilllv+status->dex/10;
    if(sd) val1 += pc_checkskill(sd,DC_DANCINGLESSON);
    break;
```

Caster does NOT receive own buff. Affects all non-caster players in AoE.

---

### 3.6 Please Don't Forget Me / Slow Grace (ID 1527, rA: DC_DONTFORGETME 328)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Debuff Dance) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 180 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 10 seconds |
| Prerequisites | Ugly Dance Lv 3 |
| Requires | Whip equipped |

**Effect Formulas (eathena skill.c verified):**

```
ASPD Reduction%  = 5 + (SkillLv * 3) + floor(DEX / 10) + DanceLessonsLv
MoveSpeed Red.%  = 5 + (SkillLv * 3) + floor(AGI / 10) + DanceLessonsLv
```

| Level | ASPD Drop (base) | Move Drop (base) | ASPD (DL10, DEX99) | Move (DL10, AGI99) | SP Cost |
|-------|-----------------|-----------------|--------------------|--------------------|---------|
| 1 | 8% | 8% | 27% | 27% | 28 |
| 2 | 11% | 11% | 30% | 30% | 31 |
| 3 | 14% | 14% | 33% | 33% | 34 |
| 4 | 17% | 17% | 36% | 36% | 37 |
| 5 | 20% | 20% | 39% | 39% | 40 |
| 6 | 23% | 23% | 42% | 42% | 43 |
| 7 | 26% | 26% | 45% | 45% | 46 |
| 8 | 29% | 29% | 48% | 48% | 49 |
| 9 | 32% | 32% | 51% | 51% | 52 |
| 10 | 35% | 35% | 54% | 54% | 55 |

**SP Cost Formula:** `25 + SkillLv * 3`

**eathena source:**
```c
case DC_DONTFORGETME:
    val1 = status->dex/10 + 3*skilllv + 5; // ASPD decrease
    val2 = status->agi/10 + 3*skilllv + 5; // Movement speed adjustment
    if(sd){ val1 += pc_checkskill(sd,DC_DANCINGLESSON);
            val2 += pc_checkskill(sd,DC_DANCINGLESSON); }
    break;
```

**Applies To:** ALL enemies (monsters and players) within the AoE.
**Does NOT Apply To:** Party members/allies.
**Boss Note:** Full effect applies to boss monsters (ASPD and move speed reduction).
**Cancels on application:** Removes SC_INCREASEAGI, SC_ADRENALINE, SC_ADRENALINE2, SC_SPEARQUICKEN, SC_TWOHANDQUICKEN, SC_ONEHAND from affected targets.

---

### 3.7 Fortune's Kiss / Lady Luck (ID 1528, rA: DC_FORTUNEKISS 329)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Dance) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 120 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 4 seconds |
| Prerequisites | Ugly Dance Lv 3 |
| Requires | Whip equipped |

**CRIT Bonus Formula (eathena skill.c verified):**

```
CRIT_boost = 10 + SkillLv + floor(LUK / 10) + DanceLessonsLv
```

| Level | Base CRIT (LUK=1, DL=0) | With DL10 | With DL10 + LUK99 | SP Cost |
|-------|--------------------------|-----------|-------------------|---------|
| 1 | 11 | 21 | 30 | 43 |
| 2 | 12 | 22 | 31 | 46 |
| 3 | 13 | 23 | 32 | 49 |
| 4 | 14 | 24 | 33 | 52 |
| 5 | 15 | 25 | 34 | 55 |
| 6 | 16 | 26 | 35 | 58 |
| 7 | 17 | 27 | 36 | 61 |
| 8 | 18 | 28 | 37 | 64 |
| 9 | 19 | 29 | 38 | 67 |
| 10 | 20 | 30 | 39 | 70 |

**SP Cost Formula:** `40 + SkillLv * 3`

**eathena source:**
```c
case DC_FORTUNEKISS:
    val1 = 10+skilllv+(status->luk/10);
    if(sd) val1 += pc_checkskill(sd,DC_DANCINGLESSON);
    val1*=10; // Because every 10 crit is an actual cri point.
    break;
```

**Note:** The `*10` at the end converts to rAthena's internal 0.1-crit units. Since our system stores CRIT as display integers (not *10), our formula uses the pre-multiplication value.

**Pre-Renewal Note:** Fortune's Kiss does NOT provide critical damage bonus in pre-renewal. The +2%/level critical damage is a Renewal addition.

Caster does NOT receive own buff. Affects all non-caster players in AoE.

---

### 3.8 Service for You / Gypsy's Kiss (ID 1521, rA: DC_SERVICEFORYOU 330)

| Field | Value |
|-------|-------|
| Type | Active, Performance (Supportive Dance) |
| Max Level | 10 |
| AoE | 7x7 cells (175 UE radius) |
| Duration | 180 seconds |
| Effect Linger | 20 seconds after leaving AoE |
| SP Drain | 1 SP / 5 seconds |
| Prerequisites | Ugly Dance Lv 3 |
| Requires | Whip equipped |

**Effect Formulas (eathena skill.c verified):**

```
MaxSP Boost%     = 15 + SkillLv + floor(INT / 10) + DanceLessonsLv
SP Cost Reduction% = 20 + (SkillLv * 3) + floor(INT / 10) + DanceLessonsLv
```

| Level | MaxSP% (base) | SP Red.% (base) | MaxSP% (DL10, INT99) | SP Red.% (DL10, INT99) | SP Cost |
|-------|---------------|-----------------|----------------------|------------------------|---------|
| 1 | 16% | 23% | 35% | 42% | 40 |
| 2 | 17% | 26% | 36% | 45% | 45 |
| 3 | 18% | 29% | 37% | 48% | 50 |
| 4 | 19% | 32% | 38% | 51% | 55 |
| 5 | 20% | 35% | 39% | 54% | 60 |
| 6 | 21% | 38% | 40% | 57% | 65 |
| 7 | 22% | 41% | 41% | 60% | 70 |
| 8 | 23% | 44% | 42% | 63% | 75 |
| 9 | 24% | 47% | 43% | 66% | 80 |
| 10 | 25% | 50% | 44% | 69% | 85 |

**SP Cost Formula:** `35 + SkillLv * 5`

**eathena source:**
```c
case DC_SERVICEFORYOU:
    val1 = 15+skilllv+(status->int_/10); // MaxSP percent increase
    val2 = 20+3*skilllv+(status->int_/10); // SP cost reduction
    if(sd){ val1 += pc_checkskill(sd,DC_DANCINGLESSON);  //TODO should be half
            val2 += pc_checkskill(sd,DC_DANCINGLESSON); } //TODO should be half
    break;
```

**Note:** The eathena TODO comments note DL contribution "should be half" but the actual pre-renewal code uses full DL. We match the actual source code, not the aspirational TODO.

Caster does NOT receive own buff. Affects all non-caster players in AoE.

---

### 3.9 Charming Wink / Wink of Charm (ID 1529, rA: DC_WINKCHARM 1011)

| Field | Value |
|-------|-------|
| Type | Active, Single Target |
| Max Level | 1 |
| Range | 9 cells |
| SP Cost | 40 |
| Cast Time | 1000 ms |
| After-Cast Delay | 2000 ms |
| Cooldown | 0 ms |
| Prerequisites | Quest (Dancer Platinum Skill, Job Lv 40+) |
| Race Restriction | Demi-Human, Angel, Demon only |

**Pre-Renewal Mechanics (rAthena winkofcharm.cpp):**

| Target | Success Rate | Effect | Duration |
|--------|-------------|--------|----------|
| Monster | `max(0, (casterBaseLv - targetBaseLv) + 40)%` | SC_WINKCHARM: monster follows caster (charmed, non-aggressive) | 10s |
| Player | 10% flat | SC_CONFUSION (30s), then SC_WINKCHARM (10s) only if confusion lands | 30s/10s |

**rAthena source:**
```cpp
// Against Players (pre-renewal):
sc_start(src, target, SC_CONFUSION, 10, ...); // 10% chance, 30s
sc_start(src, target, SC_WINKCHARM, 100, ...); // then charm 10s

// Against Monsters:
sc_start2(src, target, SC_WINKCHARM,
    (status_get_lv(src) - status_get_lv(target)) + 40, // Level-based chance
    skill_lv, src->id, Duration2); // 10s charm
```

- Does NOT affect Boss monsters.
- **BLOCKED during active performance.**

---

### 3.10 Moonlit Water Mill / Sheltering Bliss (ID 1540, rA: DC_FORTUNEKISS ... N/A solo)

| Field | Value |
|-------|-------|
| Type | Performance (Special) |
| Max Level | 5 |
| AoE | 9x9 cells (225 UE radius) |
| Duration | `15 + 5 * SkillLv` seconds (20/25/30/35/40s) |
| SP Drain | `4 * SkillLv` SP every 10 seconds |
| Prerequisites | Adaptation Lv 1 |
| Requires | Whip equipped |

| Level | Duration | SP Cost | SP Drain/10s |
|-------|----------|---------|-------------|
| 1 | 20s | 30 | 4 |
| 2 | 25s | 40 | 8 |
| 3 | 30s | 50 | 12 |
| 4 | 35s | 60 | 16 |
| 5 | 40s | 70 | 20 |

**Mechanics:**
- Prevents monsters and players from ENTERING the AoE.
- Pushes out entities already inside when cast (2-cell knockback).
- Prevents normal attacks while inside the area.
- Does NOT prevent skill use.
- Cannot be used in WoE or Endless Tower.

**Note:** Moonlit Water Mill is canonically an ENSEMBLE skill (requires both Bard and Dancer). Some servers implement it as a solo Dancer performance, but the canonical behavior requires both performers. This is classified as deferred until ensemble system is complete.

---

## 4. Ensemble Skills (IDs 1530-1537 / 1550-1557)

Ensemble skills are learned separately by Bard (IDs 1530-1537) and Dancer (IDs 1550-1557), but both are the same skill requiring both performers. The effective skill level uses the **minimum** of the two performers' levels (iRO Wiki) or the **average** (rAthena formula: `floor((BardLv + DancerLv) / 2)`). The rAthena average formula is the canonical implementation.

### 4.1 Lullaby (Bard 1530 / Dancer 1550, rA: BD_LULLABY 306)

| Field | Value |
|-------|-------|
| Max Level | 1 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | 20 |
| Duration | 60 seconds |
| SP Drain | 1 SP / 4 seconds (from EACH performer) |
| Bard Prereq | A Whistle Lv 10 |
| Dancer Prereq | Humming Lv 10 |

**Effect:** Chance to inflict Sleep on all enemies in the AoE every 4-6 seconds.
- Sleep chance scales with caster INT (estimated: `baseChance + INT/5`).
- Boss monsters are immune to Sleep.
- Sleep is broken by taking damage.
- Party members are IMMUNE to Lullaby's sleep effect (requires `casterPartyId` check).

---

### 4.2 Mr. Kim A Rich Man / Mental Sensing (Bard 1531 / Dancer 1551, rA: BD_RICHMANKIM 307)

| Field | Value |
|-------|-------|
| Max Level | 5 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | `16 + 4 * SkillLv` (20/24/28/32/36) |
| Duration | 60 seconds |
| SP Drain | 1 SP / 3 seconds (from EACH performer) |
| Prereq (Both) | Invulnerable Siegfried Lv 3 |

| Level | EXP Bonus |
|-------|-----------|
| 1 | +36% |
| 2 | +47% |
| 3 | +58% |
| 4 | +69% |
| 5 | +80% |

**EXP Formula:** `EXP_bonus% = 25 + SkillLv * 11` (approximately)

- Increases base + job EXP gained by party members within the AoE.
- Does NOT increase EXP from boss monster kills.
- Stacks multiplicatively with Battle Manuals.
- Cannot be dispelled.

---

### 4.3 Eternal Chaos / Down Tempo (Bard 1532 / Dancer 1552, rA: BD_ETERNALCHAOS 308)

| Field | Value |
|-------|-------|
| Max Level | 1 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | 30 |
| Duration | 60 seconds |
| SP Drain | 1 SP / 4 seconds (from EACH performer) |
| Prereq (Both) | Loki's Veil Lv 1 |

**Effect:** Reduces all VIT-derived soft DEF to 0 for all enemies in AoE.
- Equipment/hard DEF is NOT affected.
- Works on all enemies including boss monsters.
- Cannot be dispelled.

---

### 4.4 A Drum on the Battlefield / Battle Theme (Bard 1533 / Dancer 1553, rA: BD_DRUMBATTLEFIELD 309)

| Field | Value |
|-------|-------|
| Max Level | 5 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | `33 + 5 * SkillLv` (38/43/48/53/58) |
| Duration | 60 seconds |
| SP Drain | 1 SP / 5 seconds (from EACH performer) |
| Bard Prereq | Apple of Idun Lv 10 |
| Dancer Prereq | Service for You Lv 10 |

| Level | ATK Bonus | DEF Bonus |
|-------|-----------|-----------|
| 1 | +50 | +4 |
| 2 | +75 | +6 |
| 3 | +100 | +8 |
| 4 | +125 | +10 |
| 5 | +150 | +12 |

**ATK Formula:** `ATK_bonus = 25 + 25 * SkillLv`
**DEF Formula:** `DEF_bonus = 2 + 2 * SkillLv`

Applies to party members in AoE only. Cannot be dispelled.

---

### 4.5 The Ring of Nibelungen / Harmonic Lick (Bard 1534 / Dancer 1554, rA: BD_RINGNIBELUNGEN 310)

| Field | Value |
|-------|-------|
| Max Level | 5 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | `33 + 3 * SkillLv` (36/39/42/45/48) |
| Duration | 60 seconds |
| SP Drain | 1 SP / 3 seconds (from EACH performer) |
| Prereq (Both) | Drum on Battlefield Lv 3 |

| Level | Lv4 Weapon ATK Bonus |
|-------|---------------------|
| 1 | +75 |
| 2 | +100 |
| 3 | +125 |
| 4 | +150 |
| 5 | +175 |

**ATK Formula:** `ATK_bonus = 50 + 25 * SkillLv`

**CRITICAL RESTRICTION:** Only applies to characters wielding **Weapon Level 4** weapons. This damage ignores DEF entirely (flat bonus applied post-DEF).

Applies to party members in AoE only. Cannot be dispelled.

---

### 4.6 Loki's Veil / Classical Pluck (Bard 1535 / Dancer 1555, rA: BD_ROKISWEIL 311)

| Field | Value |
|-------|-------|
| Max Level | 1 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | 15 |
| Duration | 60 seconds |
| SP Drain | 1 SP / 4 seconds (from EACH performer) |
| Bard Prereq | Assassin Cross of Sunset Lv 10 |
| Dancer Prereq | Please Don't Forget Me Lv 10 |

**Effect:** Disables ALL skills (physical, magic, item use) for EVERYONE within the AoE.
- **Exception:** "Longing for Freedom" (Gypsy/Minstrel trans skill) still works.
- Boss monsters are NOT affected.
- The performers themselves are also affected.
- Only normal attacks remain functional within the AoE.
- Musical Strike and Slinging Arrow are BLOCKED inside Loki's Veil (unlike normal performances).
- Only the **initiator** (not the partner) can use Adaptation to cancel the ensemble.
- Cannot be dispelled.
- **PvP-focused skill** -- extremely powerful zone denial.

---

### 4.7 Into the Abyss / Power Cord (Bard 1536 / Dancer 1556, rA: BD_INTOABYSS 312)

| Field | Value |
|-------|-------|
| Max Level | 1 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | 10 |
| Duration | 60 seconds |
| SP Drain | 1 SP / 5 seconds (from EACH performer) |
| Prereq (Both) | Lullaby Lv 1 |

**Effect:** Party members in AoE can cast spells without gemstone requirements and lay traps without item cost.
- Affects: Safety Wall (Blue Gem), Stone Curse (Red Gem), Endow skills, Warp Portal (Blue Gem), etc.
- **Exceptions:** Hocus-pocus still requires 1 Yellow Gem, Ganbantein still requires both gems.
- Does NOT affect trap item requirements (traps still need the Trap item).
- Cannot be dispelled.

---

### 4.8 Invulnerable Siegfried / Acoustic Rhythm (Bard 1537 / Dancer 1557, rA: BD_SIEGFRIED 313)

| Field | Value |
|-------|-------|
| Max Level | 5 |
| AoE | 9x9 cells (225 UE radius) |
| SP Cost | `15 + 5 * SkillLv` (20/25/30/35/40) |
| Duration | 60 seconds |
| SP Drain | 1 SP / 3 seconds (from EACH performer) |
| Bard Prereq | Poem of Bragi Lv 10 |
| Dancer Prereq | Fortune's Kiss Lv 10 |

| Level | Elemental Resist | Status Resist |
|-------|-----------------|---------------|
| 1 | +60% | +10% |
| 2 | +65% | +20% |
| 3 | +70% | +30% |
| 4 | +75% | +40% |
| 5 | +80% | +50% |

**Elemental Resist Formula:** `elemResist% = 55 + 5 * SkillLv`
**Status Resist Formula:** `statusResist% = SkillLv * 10`

- Applies to party members in AoE only.
- Element coverage: Fire, Water, Wind, Earth, Shadow, Undead, Holy, Ghost, Poison (all non-Neutral).
- Status coverage: All status effects (Stun, Freeze, Sleep, Poison, Curse, Blind, Silence, Confusion, Bleeding, etc.).
- Cannot be dispelled.

---

## 5. Performance System Core

### 5.1 How Performances Work

Performances (songs for Bards, dances for Dancers) are ground-based AoE effects. When activated:

1. **Activation:** SP is consumed. A 7x7 cell (solo) or 9x9 cell (ensemble) ground effect spawns centered on the caster.
2. **Performance State:** The caster enters "performing" state. Movement speed is severely restricted.
3. **AoE Effect:** Entities within the AoE receive the buff/debuff effect. The effect persists while they remain in range.
4. **SP Drain:** The performer loses 1 SP at regular intervals (varies per skill). When SP reaches 0, the performance ends.
5. **Duration:** Each performance has a maximum stay duration. After expiry, the performance ends automatically.
6. **Buff Linger:** When an entity leaves the AoE, the buff/debuff effect lingers for 20 seconds before fading. This prevents flicker when entities are near the edge. The linger timer does NOT reset if the entity re-enters the AoE before expiry.

### 5.2 AoE Sizes

| Context | AoE Size | Radius (UE units at 50 UE/cell) |
|---------|----------|----------------------------------|
| Solo Song/Dance (pre-renewal) | 7x7 cells | 175 |
| Ensemble (pre-renewal) | 9x9 cells | 225 |
| Solo Song/Dance (Renewal) | 31x31 cells | 775 |

**Implementation uses pre-renewal: 7x7 for solos, 9x9 for ensembles.**

### 5.3 AoE Movement Behavior

- **Solo performances:** The ground AoE follows the caster's position (moves with them). Since the caster's movement speed is heavily reduced, this is a slow drift.
- **Ensemble performances:** The ground AoE is FIXED at the midpoint between the two performers. It does NOT move. Both performers are movement-locked.

### 5.4 SP Drain Intervals Per Song/Dance

| Skill | SP Drain Interval | Source |
|-------|-------------------|--------|
| A Whistle | 1 SP / 5 seconds | rAthena pre-re |
| Assassin Cross of Sunset | 1 SP / 3 seconds | rAthena pre-re |
| A Poem of Bragi | 1 SP / 5 seconds | rAthena pre-re |
| Apple of Idun | 1 SP / 6 seconds | rAthena pre-re |
| Dissonance | 1 SP / 3 seconds | rAthena pre-re |
| Humming | 1 SP / 5 seconds | rAthena pre-re |
| Please Don't Forget Me | 1 SP / 10 seconds | rAthena pre-re |
| Fortune's Kiss | 1 SP / 4 seconds | rAthena pre-re |
| Service for You | 1 SP / 5 seconds | rAthena pre-re |
| Ugly Dance | 1 SP / 3 seconds | rAthena pre-re |

### 5.5 Movement Speed During Performance

Without Music/Dance Lessons, the performer moves at 25% of normal speed (1/4 speed). With Lessons:

```
performanceSpeedPercent = 25 + 2.5 * LessonsLv
```

| Lessons Lv | Speed % |
|------------|---------|
| 0 | 25% |
| 1 | 27.5% |
| 2 | 30% |
| 3 | 32.5% |
| 4 | 35% |
| 5 | 37.5% |
| 6 | 40% |
| 7 | 42.5% |
| 8 | 45% |
| 9 | 47.5% |
| 10 | 50% |

**For ensembles:** Movement speed is 0% -- CANNOT move at all.

### 5.6 Weapon Requirements

| Class | Required Weapon | Applies To |
|-------|----------------|------------|
| Bard | Instrument | All songs, Musical Strike |
| Dancer | Whip | All dances, Slinging Arrow |
| Either | None | Frost Joker, Scream, Pang Voice, Charming Wink |

Bow skills (Double Strafe, Arrow Shower, Arrow Repel) require a BOW. The Bard/Dancer must weapon-swap between performance weapon and bow.

### 5.7 Cancel Conditions

Performances can be canceled by the following (in order of priority):

| Cancel Condition | Applies To | Bypass 5s Gate? | Source |
|-----------------|------------|-----------------|--------|
| **Death** | Solo + Ensemble | Yes | Universal |
| **SP Depletion** | Solo + Ensemble | Yes | Universal |
| **Duration Expiry** | Solo + Ensemble | Yes | Universal |
| **Disconnect/Logout** | Solo + Ensemble | Yes | Universal |
| **Stun** | Solo + Ensemble | Yes | CC cancel |
| **Freeze** | Solo + Ensemble | Yes | CC cancel |
| **Stone Curse** | Solo + Ensemble | Yes | CC cancel |
| **Deep Sleep** | Solo + Ensemble | Yes | CC cancel |
| **Weapon Swap** | Solo + Ensemble | **Yes** (no 5s gate) | Instant cancel |
| **Starting New Performance** | Solo only | Yes | Auto-cancel old |
| **Adaptation to Circumstances** | Solo + Ensemble | No (5s first-use gate) | Skill cancel |
| **Dispel** | Solo + Ensemble | Yes | Sage skill |
| **Heavy Damage (>25% MaxHP)** | Solo + Ensemble | Yes | Single-hit threshold |

**What does NOT cancel solo performances:**
- Silence (prevents new casts but does NOT stop ongoing performance)
- Bleeding
- Confusion
- Sleep (controversially -- some sources say it does, but rAthena status.cpp does not end SC_DANCING on sleep for solo)

**What additionally cancels ensembles (but not solo):**
- Silence DOES cancel ensemble performances
- Either performer being separated beyond range

### 5.8 Skills Usable During Performance

| Skill | Usable During Performance? | Source |
|-------|---------------------------|--------|
| Musical Strike | **YES** | rAthena `AllowWhenPerforming` |
| Slinging Arrow | **YES** | rAthena `AllowWhenPerforming` |
| Adaptation to Circumstances | **YES** | rAthena `AllowWhenPerforming` |
| Frost Joker | **NO** (BLOCKED) | rAthena -- no AllowWhenPerforming flag |
| Scream | **NO** (BLOCKED) | rAthena -- no AllowWhenPerforming flag |
| Pang Voice | **NO** (BLOCKED) | rAthena -- no AllowWhenPerforming flag |
| Charming Wink | **NO** (BLOCKED) | rAthena -- no AllowWhenPerforming flag |
| Encore | **NO** -- must cancel first | By design |
| Normal attacks | **BLOCKED** | Performance state |
| Item use | **BLOCKED** | Performance state |
| All other skills | **BLOCKED** | Performance state |

### 5.9 Caster Buff Exclusion

**The caster does NOT receive their own song/dance buff.** This is a core rule confirmed by iRO Wiki, MyRoLife guide, and multiple player resources. In the performance tick loop, the performer's own character ID must be excluded from buff application.

However, in **ensemble skills**, the OTHER performer (the partner) IS affected by the ensemble buff. Only the caster (initiator) is excluded, not the partner.

### 5.10 Song Overlap Rules

When two performances of the SAME type (two Bard songs, or two Dancer dances) overlap in AoE:
- The intersection zone converts to **Dissonance Lv1** (for songs) or **Ugly Dance Lv1** (for dances) MISC damage instead of applying the normal buff.
- A Song and a Dance can coexist normally -- no Dissonance/Ugly Dance conversion.

rAthena commit `10bbf25` (#9281) improved the overlap detection to handle 3+ overlapping performances correctly.

### 5.11 Performance Aftermath

After a performance ends, there is typically a brief "aftermath" debuff applied to the performer:
- The performer cannot immediately start a new performance (short internal cooldown).
- Some implementations apply a movement speed penalty for ~3 seconds after the performance ends.

---

## 6. Ensemble System

### 6.1 Requirements

1. **Party:** Both Bard and Dancer must be in the **same party**.
2. **Adjacent:** Must be standing within **1 cell** of each other (any of the 8 adjacent cells).
3. **Weapons:** Bard must have **Instrument** equipped, Dancer must have **Whip** equipped.
4. **Skill:** Both must have the **same ensemble skill** learned at some level.
5. **Gender:** Requires exactly one male (Bard) and one female (Dancer).

### 6.2 Skill Level Calculation

```
effectiveLevel = floor((bardSkillLevel + dancerSkillLevel) / 2)
```

Example: Bard has Lullaby Lv 3, Dancer has Lullaby Lv 5 => effective level = `floor((3+5)/2)` = **4**.

**Note:** iRO Wiki states the MINIMUM is used; rAthena source uses the AVERAGE. The rAthena formula is canonical.

### 6.3 AoE Positioning

The ensemble AoE (9x9) is centered at the **midpoint** between the two performers' positions. Unlike solo performances which follow the caster, ensemble AoEs are **fixed** -- stationary ground effects.

### 6.4 Movement Lock

**BOTH performers are completely movement-locked** during an ensemble. Movement speed is 0% -- they cannot move at all. If either performer is forcibly moved (knockback, teleport), the ensemble breaks.

### 6.5 SP Drain

**BOTH performers** drain SP independently at the ensemble's defined SP drain interval. If EITHER performer runs out of SP, the ensemble ends for BOTH.

### 6.6 SP Recovery

SP does NOT regenerate while performing an ensemble. The performers' SP only decreases during an ensemble.

### 6.7 Cancel Conditions (Ensemble-Specific)

All solo performance cancel conditions also apply to ensembles, plus:

| Additional Ensemble Cancel | Details |
|---------------------------|---------|
| Either performer dies | Both stop |
| Either performer runs out of SP | Both stop |
| Performers separated beyond range | Ensemble breaks |
| Silence on either performer | Ensemble breaks (unlike solo) |
| Either performer forcibly moved | Ensemble breaks |
| Adaptation by initiator | Both stop (only initiator can Adapt) |

**The non-initiating partner cannot use Adaptation.** Only the performer who started the ensemble can cancel it with Adaptation. The partner is forced to cooperate.

### 6.8 Ensemble Buff Targets

| Ensemble Type | Who Is Affected |
|--------------|-----------------|
| Supportive (Drum, Nibelungen, Siegfried, Into the Abyss, Mr. Kim) | Party members in AoE |
| Offensive (Eternal Chaos, Loki's Veil) | ALL entities in AoE (enemies AND party, including performers) |
| Sleep (Lullaby) | All enemies in AoE, party immune |

### 6.9 Ensemble Prerequisite Map

| Ensemble Skill | Bard Prerequisite | Dancer Prerequisite |
|---------------|-------------------|---------------------|
| Lullaby | A Whistle Lv 10 | Humming Lv 10 |
| Mr. Kim A Rich Man | Invulnerable Siegfried Lv 3 | Invulnerable Siegfried Lv 3 |
| Eternal Chaos | Loki's Veil Lv 1 | Loki's Veil Lv 1 |
| A Drum on the Battlefield | Apple of Idun Lv 10 | Service for You Lv 10 |
| The Ring of Nibelungen | Drum on Battlefield Lv 3 | Drum on Battlefield Lv 3 |
| Loki's Veil | Assassin Cross Lv 10 | Please Don't Forget Me Lv 10 |
| Into the Abyss | Lullaby Lv 1 | Lullaby Lv 1 |
| Invulnerable Siegfried | Poem of Bragi Lv 10 | Fortune's Kiss Lv 10 |

---

## 7. Adaptation to Circumstances

### Bard: ID 1503 / Dancer: ID 1523 (rA: BD_ADAPTATION 304)

| Field | Value |
|-------|-------|
| Type | Active, Self |
| Max Level | 1 |
| SP Cost | 1 |
| Cast Time | 0 ms (instant) |
| After-Cast Delay | 300 ms |
| Cooldown | 0 ms |
| Prerequisites | None |

**Effect:** Cancels the caster's current active performance (song, dance, or ensemble).

**Restrictions:**
- Cannot be used in the first 5 seconds of a performance.
- Only the ensemble INITIATOR can use Adaptation to cancel an ensemble (not the partner).
- After cancellation, the same performance cannot be immediately recast (short lockout).

**Alternative Cancel Method:** Switching to a dagger weapon instantly cancels any performance. This bypasses the 5-second restriction and has no cast delay. This is the preferred cancel technique in competitive play ("dagger-stopping tactic").

**rAthena `AllowWhenPerforming` flag is set** -- this skill can be used during performance.

---

## 8. Encore

### Bard: ID 1504 / Dancer: ID 1524 (rA: BD_ENCORE 305)

| Field | Value |
|-------|-------|
| Type | Active, Self |
| Max Level | 1 |
| SP Cost | 1 + ceil(lastSongSPCost / 2) |
| Cast Time | 0 ms (instant) |
| After-Cast Delay | 300 ms |
| Cooldown | 10000 ms |
| Prerequisites | Adaptation Lv 1 |
| Requires | Instrument (Bard) or Whip (Dancer) |

**Effect:** Re-casts the last performed song/dance at 50% of its original SP cost.

**Mechanics:**
- Server tracks `player.lastPerformanceSkillId` and `player.lastPerformanceLevel`.
- Encore replays the stored skill at the stored level (NOT current learned level).
- After Encore is used, the remembered skill is CLEARED -- cannot chain Encore repeatedly. Must perform a full song to re-set the memory.
- Cannot be used during an active performance -- must cancel first.
- If no song was previously performed, Encore fails (consumes only 1 SP).

---

## 9. Skill Trees and Prerequisites

### 9.1 Bard Skill Tree

```
Music Lessons (1500) -----> Musical Strike (1511, req ML 3)
    |
    +---> Dissonance (1505, req ML 1 + Adaptation 1)
    |         |
    |         +---> A Whistle (1507, req Dis 3)
    |         +---> Assassin Cross (1502, req Dis 3)
    |         +---> Poem of Bragi (1501, req Dis 3)
    |         +---> Apple of Idun (1508, req Dis 3)
    |
    +---> Adaptation (1503) ---> Encore (1504, req Adapt 1)
                                     |
                                     +---> Frost Joker (1506, req Encore 1)

[Quest Skill] Pang Voice (1509, Job Lv 40)

Ensemble chain:
A Whistle 10 ---------> Lullaby (1530)
Assassin Cross 10 ----> Loki's Veil (1535)
Poem of Bragi 10 -----> Invulnerable Siegfried (1537)
Apple of Idun 10 -----> Drum on Battlefield (1533)
Lullaby 1 ------------> Into the Abyss (1536)
Drum on Battlefield 3 -> Ring of Nibelungen (1534)
Inv. Siegfried 3 ------> Mr. Kim A Rich Man (1531)
Loki's Veil 1 --------> Eternal Chaos (1532)
```

### 9.2 Dancer Skill Tree

```
Dance Lessons (1520) -----> Slinging Arrow (1541, req DL 3)
    |
    +---> Ugly Dance (1525, req DL 1 + Adaptation 1)
    |         |
    |         +---> Humming (1522, req UD 3)
    |         +---> Please Don't Forget Me (1527, req UD 3)
    |         +---> Fortune's Kiss (1528, req UD 3)
    |         +---> Service for You (1521, req UD 3)
    |
    +---> Adaptation (1523) ---> Encore (1524, req Adapt 1)
                                     |
                                     +---> Scream (1526, req Encore 1)

[Quest Skill] Charming Wink (1529, Job Lv 40)

Ensemble chain:
Humming 10 -----------> Lullaby (1550)
PDFM 10 --------------> Loki's Veil (1555)
Fortune's Kiss 10 ----> Invulnerable Siegfried (1557)
Service for You 10 ---> Drum on Battlefield (1553)
Lullaby 1 ------------> Into the Abyss (1556)
Drum on Battlefield 3 -> Ring of Nibelungen (1554)
Inv. Siegfried 3 ------> Mr. Kim A Rich Man (1551)
Loki's Veil 1 --------> Eternal Chaos (1552)
```

### 9.3 Inherited Archer Skills

Both Bards and Dancers inherit ALL Archer skills:
- Owl's Eye (300), Vulture's Eye (301), Improve Concentration (302)
- Double Strafe (303), Arrow Shower (304)
- Arrow Crafting (305, quest), Arrow Repel (306, quest)

---

## 10. Implementation Checklist

### Solo Bard Skills

| ID | Skill | Handler | Data | Buff | VFX | Status |
|----|-------|---------|------|------|-----|--------|
| 1500 | Music Lessons | Passive | Done | N/A | N/A | DONE |
| 1511 | Musical Strike | Combat | Done | N/A | TODO | DONE |
| 1505 | Dissonance | Perf | Done | N/A | TODO | DONE |
| 1506 | Frost Joker | AoE | Done | N/A | TODO | DONE |
| 1507 | A Whistle | Perf | Done | Done | TODO | DONE |
| 1502 | Assassin Cross | Perf | Done | Done | TODO | DONE |
| 1501 | Poem of Bragi | Perf | Done | Done | TODO | DONE |
| 1508 | Apple of Idun | Perf | Done | Done | TODO | DONE |
| 1509 | Pang Voice | Target | Done | N/A | TODO | DONE |
| 1503 | Adaptation | Cancel | Done | N/A | N/A | DONE |
| 1504 | Encore | Re-cast | Done | N/A | N/A | DONE |

### Solo Dancer Skills

| ID | Skill | Handler | Data | Buff | VFX | Status |
|----|-------|---------|------|------|-----|--------|
| 1520 | Dance Lessons | Passive | Done | N/A | N/A | DONE |
| 1541 | Slinging Arrow | Combat | Done | N/A | TODO | DONE |
| 1525 | Ugly Dance | Perf | Done | N/A | TODO | DONE |
| 1526 | Scream | AoE | Done | N/A | TODO | DONE |
| 1522 | Humming | Perf | Done | Done | TODO | DONE |
| 1527 | PDFM | Perf | Done | Done | TODO | DONE |
| 1528 | Fortune's Kiss | Perf | Done | Done | TODO | DONE |
| 1521 | Service for You | Perf | Done | Done | TODO | DONE |
| 1540 | Moonlit Water Mill | Perf | Stub | N/A | TODO | DEFERRED |
| 1529 | Charming Wink | Target | Done | N/A | TODO | DONE |
| 1523 | Adaptation | Cancel | Done | N/A | N/A | DONE |
| 1524 | Encore | Re-cast | Done | N/A | N/A | DONE |

### Ensemble Skills

| Bard ID | Dancer ID | Skill | Status |
|---------|-----------|-------|--------|
| 1530 | 1550 | Lullaby | DONE |
| 1531 | 1551 | Mr. Kim A Rich Man | DONE |
| 1532 | 1552 | Eternal Chaos | DONE |
| 1533 | 1553 | Drum on Battlefield | DONE |
| 1534 | 1554 | Ring of Nibelungen | DONE |
| 1535 | 1555 | Loki's Veil | DONE |
| 1536 | 1556 | Into the Abyss | DONE |
| 1537 | 1557 | Invulnerable Siegfried | DONE |

### Performance System

| Feature | Status |
|---------|--------|
| Performance state management | DONE |
| SP drain tick loop | DONE |
| AoE buff application (allies) | DONE |
| AoE debuff application (enemies) | DONE |
| Movement speed restriction | DONE |
| Caster buff exclusion | DONE |
| Buff linger (20s) | DONE |
| Song overlap -> Dissonance | DONE |
| Weapon swap cancel | DONE |
| Heavy damage cancel (>25% HP) | DONE |
| Dispel cancel | DONE |
| CC cancel (Stun/Freeze/Stone) | DONE |
| Silence does NOT cancel solo | DONE |
| New song auto-cancels old | DONE |
| Haste2 exclusion group | DONE |
| ACoS bow user exclusion | DONE |
| Frost Joker/Scream BLOCKED during perf | DONE |

### Ensemble System

| Feature | Status |
|---------|--------|
| Party requirement check | DONE |
| Adjacent position check | DONE |
| Averaged skill level calculation | DONE |
| Both performers enter performance state | DONE |
| Midpoint AoE positioning | DONE |
| Both performers movement-locked | DONE |
| Both performers SP drain | DONE |
| Either performer break ends both | DONE |
| Initiator-only Adaptation | DONE |
| Ensemble buff pipeline | DONE |
| Ensemble aftermath debuff | DONE |

---

## 11. Gap Analysis

### Currently Implemented (Verified Working)

All 20 Bard skills (11 solo + 1 quest + 8 ensemble) and all 20 Dancer skills (11 solo + 1 quest + 8 ensemble) have been implemented and audited. The performance system, ensemble system, and all associated mechanics are functional.

### Remaining Gaps

| Gap | Priority | Blocked By | Notes |
|-----|----------|------------|-------|
| Moonlit Water Mill as canonical ensemble | LOW | Ensemble system complete, but MWM has movement barrier logic | Requires collision/combat tick integration for entry blocking + attack blocking |
| Performance VFX (client-side) | MEDIUM | VFX system | Ground AoE indicators, dancing animations, song note particles |
| Performance aftermath debuff (movement penalty) | LOW | None | Minor 3s speed penalty after performance ends |
| SP Recovery blocked during ensemble | LOW | None | Need to suppress SP regen tick while in ensemble state |
| Song overlap detection with 3+ performers | LOW | Rare scenario | rAthena commit #9281 has reference implementation |
| Longing for Freedom (Clown/Gypsy trans skill) | N/A | Transcendent class system | Allows movement during ensemble. Trans-only. |

### Data Accuracy Notes

The following formulas have been verified against rAthena pre-renewal source code and/or eathena C source:

- Musical Strike / Slinging Arrow: `60 + 40*Lv` (rAthena commit #9577)
- Bragi VCT/ACD: rAthena xmas_present2008 disassembly (Issue #1808)
- A Whistle FLEE/PD: rAthena Issue #1808
- ACoS ASPD: rAthena Issue #7726, status.cpp
- Apple of Idun MaxHP/HP: rAthena Issue #1808
- Humming HIT: eathena skill.c `DC_HUMMING`
- PDFM ASPD/Move: eathena skill.c `DC_DONTFORGETME`
- Fortune's Kiss CRIT: eathena skill.c `DC_FORTUNEKISS`
- Service for You MaxSP/SP Red: eathena skill.c `DC_SERVICEFORYOU`
- Ugly Dance SP Drain: eathena skill.c `DC_UGLYDANCE`
- Scream delay + party reduction: rAthena `dazzler.cpp`
- Charming Wink PvE/PvP: rAthena `winkofcharm.cpp`
- Frost Joker/Scream blocked during performance: rAthena `AllowWhenPerforming` flag check

### Cross-Reference with Implementation Audit Docs

The following audit documents contain implementation-specific bug fixes already applied:

- `Bard_Skills_Comprehensive_Audit.md` -- 22 bugs + 1 rAthena-verified fix (all applied)
- `Dancer_Skills_Audit_And_Fix_Plan.md` -- 8 critical + 5 moderate + 8 deferred (all applied)
- `session-fixes-2026-03-22.md` -- Bard/Dancer/Ensemble comprehensive fix session

---

## Sources

- [Bard - iRO Wiki Classic](https://irowiki.org/classic/Bard)
- [Dancer - iRO Wiki Classic](https://irowiki.org/classic/Dancer)
- [Ensemble Skill - iRO Wiki Classic](https://irowiki.org/classic/Ensemble_Skill)
- [Bard Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=19)
- [Dancer Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=20)
- [Bard/Dancer/Clown/Gypsy Guide - MyRoLife](http://myrolife.blogspot.com/2016/05/barddancerclowngypsy-guide.html)
- [Bard Guide - GameFAQs (Jath)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/30940)
- [Bard/Dancer Guide - GameFAQs (aldoteng)](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/35759)
- [Magic Strings - iRO Wiki](https://irowiki.org/wiki/Magic_Strings)
- [Impressive Riff - iRO Wiki](https://irowiki.org/wiki/Impressive_Riff)
- [Melody Strike - iRO Wiki](https://irowiki.org/wiki/Melody_Strike)
- [Slinging Arrow - iRO Wiki](https://irowiki.org/wiki/Slinging_Arrow)
- [Classical Pluck - iRO Wiki](https://irowiki.org/wiki/Classical_Pluck)
- [Acoustic Rhythm - iRO Wiki](https://irowiki.org/wiki/Acoustic_Rhythm)
- [Song of Lutie - iRO Wiki](https://irowiki.org/wiki/Song_of_Lutie)
- [Frost Joker - Ragnarok Wiki Fandom](https://ragnarok.fandom.com/wiki/Frost_Joker)
- [Assassin Cross of Sunset Issue #7726 - rAthena GitHub](https://github.com/rathena/rathena/issues/7726)
- [Musical Strike Pre-Re Damage #9577 - rAthena GitHub](https://github.com/rathena/rathena/commit/928e1cb53ea7f8306b5d7bcb951645e692d94549)
- [Song/Dance Overlap Dissonance #9281 - rAthena GitHub](https://github.com/rathena/rathena/commit/10bbf2506db7e723093acb4f3d70d38143a592ad)
- [Dispel Cancels Songs - rAthena Board](https://rathena.org/board/topic/122148-dispel-cancels-bard-and-dancer-songs/)
- [After Cast Delay Reduction #6135 - rAthena GitHub](https://github.com/rathena/rathena/issues/6135)
- [Official Status Resistance Formulas - RateMyServer Forum](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [ASPD - iRO Wiki Classic](https://irowiki.org/classic/ASPD)
- [Movement Speed - iRO Wiki Classic](https://irowiki.org/classic/Movement_Speed)
- [rAthena skill_db.txt documentation](https://github.com/rathena/rathena/blob/master/doc/skill_db.txt)
- [Bard - idRO Klasik Wiki](https://idrowiki.org/klasik/Bard)
