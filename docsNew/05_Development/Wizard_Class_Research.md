# Wizard Class Research -- Pre-Renewal Classic (Comprehensive)

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Wizard_Skills_Audit](Wizard_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE
**Sources:** iRO Wiki Classic, RateMyServer (Pre-Re), rAthena pre-renewal database, project `03_Skills_Complete.md`, existing `ro_skill_data_2nd.js`

---

## Table of Contents

1. [Class Overview](#class-overview)
2. [Cast Time Mechanics](#cast-time-mechanics)
3. [Complete Skill List](#complete-skill-list)
4. [Skill Tree and Prerequisites](#skill-tree-and-prerequisites)
5. [Detailed Skill Specifications](#detailed-skill-specifications)
6. [Existing Implementation Audit](#existing-implementation-audit)
7. [New Systems Required](#new-systems-required)
8. [Ground Effect System Architecture](#ground-effect-system-architecture)
9. [Implementation Priority](#implementation-priority)
10. [Integration Points](#integration-points)
11. [Wizard-Specific Constants](#wizard-specific-constants)

---

## Class Overview

| Property | Value |
|----------|-------|
| Base Class | Mage |
| Job Change Level | Job Level 40+ as Mage |
| Transcendent | High Wizard |
| Weapon Types | Rod (1H), Staff (2H), Book, Dagger |
| Shield | Yes (with 1H weapon) |
| Primary Stats | INT (MATK), DEX (cast time), VIT (survivability) |
| Role | AoE magic DPS, crowd control, ground denial |
| Key Mechanic | Ground-targeted persistent AoE spells |
| Skill Points | 14 skills, requires 101 total skill points (including Mage prerequisites) |

### Class Identity

The Wizard is the primary AoE magic damage dealer in RO Classic. Unlike Mages who rely on single-target bolt spells, Wizards specialize in massive ground-targeted AoEs (Storm Gust, Lord of Vermilion, Meteor Storm) combined with crowd control (freeze, stun, blind) and ground denial (Ice Wall, Quagmire, Fire Pillar). The tradeoff is extremely long cast times (6-15 seconds base) that require high DEX investment or party Suffragium support.

### Wizard vs Mage Skill Access

Wizards inherit all Mage skills. Key Mage skills for Wizard builds:
- **Fire Wall** (Lv10) -- the only Mage ground skill, critical for soloing
- **Sight** (Lv1) -- required for Sightrasher
- **Cold Bolt** (any level) -- Water damage filler during cast downtime
- **Frost Diver** (Lv1+) -- freeze CC, Storm Gust prerequisite
- **Stone Curse** (Lv1) -- Earth Spike/Ice Wall prerequisite
- **Safety Wall** -- defensive ground barrier

---

## Cast Time Mechanics

### Pre-Renewal Cast Time Formula

```
Final Cast Time = Base Cast Time * (1 - DEX / 150) * (1 - Suffragium%) * (1 - Item Cast Reduction%)
```

- At **150 DEX**, all variable cast time becomes **0** (instant cast).
- At 75 DEX, cast time is halved.
- There is NO fixed cast time in pre-renewal. ALL cast time is variable.
- Suffragium (Priest skill): 15% / 30% / 45% cast time reduction.
- Equipment reductions (Phen Card, Orleans Glove, etc.) apply multiplicatively.

### Cast Time Reduction Milestones

| DEX | Cast Reduction | Notes |
|-----|---------------|-------|
| 0 | 0% | Full base cast time |
| 50 | 33% | |
| 75 | 50% | Half cast time |
| 99 | 66% | Practical DEX cap for most builds |
| 110 | 73% | Common high-DEX Wizard target |
| 120 | 80% | |
| 130 | 87% | |
| 140 | 93% | |
| 150 | 100% | Instant cast (unreachable without gear bonuses for most builds) |

### Interruptible Casting

All Wizard cast times are interruptible by damage unless the caster has:
- **Phen Card** (accessory) -- casting cannot be interrupted
- **Endure** status -- prevents flinching
- **Auto Guard** blocking -- prevents interrupt on block

When interrupted, the cast is cancelled and SP is NOT consumed (SP only deducted on successful cast completion).

---

## Complete Skill List

### Wizard Skills (14 total -- IDs 800-813)

| ID | Name | Max Lv | Type | Target | Element | Summary |
|----|------|--------|------|--------|---------|---------|
| 800 | Jupitel Thunder | 10 | Active | Single | Wind | Multi-hit lightning ball + knockback |
| 801 | Lord of Vermilion | 10 | Active | Ground 9x9 | Wind | Massive AoE lightning + blind chance |
| 802 | Meteor Storm | 10 | Active | Ground 7x7 | Fire | Random meteor rain + stun chance |
| 803 | Storm Gust | 10 | Active | Ground 7x7 | Water | Blizzard AoE, 10 hits, freeze on 3rd hit |
| 804 | Earth Spike | 5 | Active | Single | Earth | Multi-hit earth spikes (1-5 hits) |
| 805 | Heaven's Drive | 5 | Active | Ground 5x5 | Earth | AoE earth spikes, hits hidden enemies |
| 806 | Quagmire | 5 | Active | Ground 5x5 | Earth | Debuff: reduce AGI/DEX/move speed |
| 807 | Water Ball | 5 | Active | Single | Water | Multi-hit water, requires water cells |
| 808 | Ice Wall | 10 | Active | Ground | Water | Create 5-cell ice barrier with HP |
| 809 | Sight Rasher | 10 | Active | Self AoE | Fire | Explode Sight for fire AoE + knockback |
| 810 | Fire Pillar | 10 | Active | Ground trap | Fire | Ground trap, ignores MDEF |
| 811 | Frost Nova | 10 | Active | Self AoE | Water | AoE freeze around caster |
| 812 | Sense | 1 | Active | Single | Neutral | Reveal monster stats |
| 813 | Sight Blaster | 1 | Active | Self | Fire | Quest skill. Auto-hit fireball shield |

---

## Skill Tree and Prerequisites

### Mage Prerequisites Required for Wizard Skills

```
Mage Tree:
  Cold Bolt (200) ---------> [No prereqs]
  Fire Bolt (201) ----------> [No prereqs]
  Lightning Bolt (202) -----> [No prereqs]
  Napalm Beat (203) --------> [No prereqs]
  Sight (205) ---------------> [No prereqs]
  Stone Curse (206) ---------> [No prereqs]
  Fire Ball (207) -----------> Fire Bolt Lv4
  Frost Diver (208) ---------> Cold Bolt Lv5
  Fire Wall (209) -----------> Fire Ball Lv5 + Sight Lv1
  Thunderstorm (212) --------> Lightning Bolt Lv4

Wizard Tree:
  Jupitel Thunder (800) -----> Napalm Beat Lv1 + Lightning Bolt Lv1
  Lord of Vermilion (801) ---> Thunderstorm Lv1 + Jupitel Thunder Lv5
  Meteor Storm (802) --------> Thunderstorm Lv1 + Sight Rasher Lv2
  Storm Gust (803) ----------> Frost Diver Lv1 + Jupitel Thunder Lv3
  Earth Spike (804) ---------> Stone Curse Lv1
  Heaven's Drive (805) ------> Earth Spike Lv3
  Quagmire (806) ------------> Heaven's Drive Lv1
  Water Ball (807) ----------> Cold Bolt Lv1 + Lightning Bolt Lv1
  Ice Wall (808) ------------> Stone Curse Lv1 + Frost Diver Lv1
  Sight Rasher (809) --------> Sight Lv1
  Fire Pillar (810) ---------> Fire Wall Lv1
  Frost Nova (811) ----------> Frost Diver Lv1 + Ice Wall Lv1
  Sense (812) ----------------> [No prereqs]
  Sight Blaster (813) -------> [Quest skill, no tree prereqs]
```

### Skill Tree Layout (treeRow, treeCol)

```
Row 0:  [Jupitel Thunder]  [Water Ball]    [Ice Wall]       [Earth Spike]    [Sense]
Row 1:  [Lord of Vermilion][Storm Gust]    [Meteor Storm]   [Heaven's Drive]
Row 2:  [Fire Pillar]      [Frost Nova]    [Sight Rasher]   [Quagmire]
Row 3:  [                                  [Sight Blaster]
```

---

## Detailed Skill Specifications

---

### 1. Jupitel Thunder (ID 800)

**Type:** Active, single target, multi-hit + knockback
**Element:** Wind
**Range:** 9 cells (450 UE units)
**Prerequisites:** Napalm Beat Lv1, Lightning Bolt Lv1

| Level | SP Cost | Cast Time (ms) | Hits | Knockback (cells) | ACD |
|-------|---------|----------------|------|-------------------|-----|
| 1 | 20 | 2500 | 3 | 2 | 0 |
| 2 | 23 | 3000 | 4 | 3 | 0 |
| 3 | 26 | 3500 | 5 | 3 | 0 |
| 4 | 29 | 4000 | 6 | 4 | 0 |
| 5 | 32 | 4500 | 7 | 4 | 0 |
| 6 | 35 | 5000 | 8 | 5 | 0 |
| 7 | 38 | 5500 | 9 | 5 | 0 |
| 8 | 41 | 6000 | 10 | 6 | 0 |
| 9 | 44 | 6500 | 11 | 6 | 0 |
| 10 | 47 | 7000 | 12 | 7 | 0 |

**Damage:** Each hit deals **100% MATK** (1x MATK per hit). Total = hits * MATK.
**Knockback:** Target is pushed back a number of cells equal to `floor((hits + 1) / 2)` (2/3/3/4/4/5/5/6/6/7). Knockback disabled in WoE.
**SP Formula:** `17 + SkillLv * 3` (matches our `20 + i * 3` with i=0 giving 20)
**Cast Time Formula:** `2000 + SkillLv * 500` ms

**Mechanics:**
- All hits land in rapid succession on a single target
- Knockback occurs after all hits
- Cannot knockback boss-protocol monsters (but damage still applies)
- WoE: knockback disabled
- Can hit hidden enemies (but needs target selection)

**Existing Data Audit:**
- SP: `20+i*3` -- **CORRECT** (20,23,26,...,47)
- Cast time: `2500+i*500` -- **CORRECT** (2500,3000,...,7000)
- effectValue: `3+i` (hits) -- **CORRECT** (3,4,...,12)
- **MISSING:** Knockback cells per level not stored (needs custom handler)
- **MISSING:** No handler implemented yet

---

### 2. Lord of Vermilion (ID 801)

**Type:** Active, ground-targeted AoE
**Element:** Wind
**Range:** 9 cells (450 UE units)
**AoE:** 9x9 cells (effective 11x11 with splash) = 450 UE radius base, 550 effective
**Prerequisites:** Thunderstorm Lv1, Jupitel Thunder Lv5
**After-Cast Delay:** 5000ms (fixed)

| Level | SP Cost | Cast Time (ms) | MATK% per wave | Blind % | Total MATK% |
|-------|---------|----------------|----------------|---------|-------------|
| 1 | 60 | 15000 | 100 | 4 | 400 |
| 2 | 64 | 14500 | 120 | 8 | 480 |
| 3 | 68 | 14000 | 140 | 12 | 560 |
| 4 | 72 | 13500 | 160 | 16 | 640 |
| 5 | 76 | 13000 | 180 | 20 | 720 |
| 6 | 80 | 12500 | 200 | 24 | 800 |
| 7 | 84 | 12000 | 220 | 28 | 880 |
| 8 | 88 | 11500 | 240 | 32 | 960 |
| 9 | 92 | 11000 | 260 | 36 | 1040 |
| 10 | 96 | 10500 | 280 | 40 | 1120 |

**Damage:** `(80 + 20 * SkillLv)%` MATK per wave, **4 damage waves** over 4 seconds. The animation shows 10 visual hits per second but only 4 actual damage calculations occur.
**Total Damage:** `4 * (80 + 20 * SkillLv)% MATK` = 400% to 1120% total MATK.
**SP Formula:** `56 + 4 * SkillLv`
**Cast Time Formula:** `15500 - 500 * SkillLv` ms (decreases with level)
**Blind:** `4 * SkillLv` % chance to inflict Blind status per wave.

**Mechanics:**
- Ground-targeted persistent AoE (4-second duration with 4 damage waves)
- Each wave calculates damage independently (separate MATK roll each wave)
- Enemies entering after initial cast still take remaining waves
- Blind check per wave per target
- Boss monsters cannot be Blinded
- Multiple LoV can stack on the same area

**Existing Data Audit:**
- SP: `60+i*4` -- **CORRECT**
- Cast time: `10500+i*500` -- **WRONG** (gives 10500-15000, should be 15000 down to 10500; formula should be `15500-(i+1)*500` or reversed)
- effectValue: `100+i*20` -- **INCORRECT** for total damage (this is per-wave MATK%, need to multiply by 4 waves in handler)
- cooldown: 5000 -- **CORRECT** (this is the ACD)
- **MISSING:** Blind chance not stored
- **MISSING:** No handler implemented

**Fix Required:** Cast time formula is inverted. Currently `10500+i*500` gives Lv1=10500 (should be 15000) and Lv10=15000 (should be 10500). Fix to: `15000-i*500` giving 15000, 14500, 14000, ..., 10500.

---

### 3. Meteor Storm (ID 802)

**Type:** Active, ground-targeted AoE
**Element:** Fire
**Range:** 9 cells (450 UE units)
**AoE:** Each meteor splashes 7x7 cells (350 UE radius); meteors land randomly in 7x7 area around target point
**Prerequisites:** Thunderstorm Lv1, Sight Rasher Lv2
**After-Cast Delay:** `2000 + 500 * SkillLv` ms (2500-7000ms)

| Level | SP Cost | Cast Time (ms) | Meteors | Hits/Meteor | Stun % | Total Hits |
|-------|---------|----------------|---------|-------------|--------|------------|
| 1 | 20 | 15000 | 2 | 1 | 3 | 2 |
| 2 | 24 | 15000 | 2 | 1 | 6 | 2 |
| 3 | 30 | 15000 | 3 | 2 | 9 | 6 |
| 4 | 34 | 15000 | 3 | 2 | 12 | 6 |
| 5 | 40 | 15000 | 4 | 3 | 15 | 12 |
| 6 | 44 | 15000 | 4 | 3 | 18 | 12 |
| 7 | 50 | 15000 | 5 | 4 | 21 | 20 |
| 8 | 54 | 15000 | 5 | 4 | 24 | 20 |
| 9 | 60 | 15000 | 6 | 5 | 27 | 30 |
| 10 | 64 | 15000 | 6 | 5 | 30 | 30 |

**Damage:** Each hit deals **125% MATK**. A single target can be hit by multiple meteors if they overlap.
**SP Formula:** Non-linear pattern -- `[20,24,30,34,40,44,50,54,60,64]`. Pattern: +4, +6, +4, +6, +4, +6, +4, +6, +4.
**Cast Time:** 15000ms fixed (all levels). Pre-renewal has no fixed cast component; this entire 15s is variable (reduced by DEX).
**Meteors:** `floor((SkillLv + 2) / 2)` = 2,2,3,3,4,4,5,5,6,6
**Hits per Meteor:** `floor((SkillLv + 1) / 2)` = 1,1,2,2,3,3,4,4,5,5
**Stun Chance:** `3 * SkillLv` % per hit.

**Mechanics:**
- Meteors fall at random positions within the AoE area over ~2-3 seconds
- Each meteor creates a 7x7 splash damage zone
- A target can be hit by multiple meteors if they stand where splashes overlap
- Maximum theoretical damage at Lv10 with all overlaps: 30 * 125% MATK = 3750% MATK
- Stun is checked per hit (boss immune)
- The randomness means damage is inconsistent -- some targets take more hits than others

**Existing Data Audit:**
- SP: `20+i*4` -- **WRONG** (gives 20,24,28,...,56 -- linear, but canonical is non-linear zigzag)
- Cast time: 15000 -- **CORRECT** (fixed)
- cooldown: 2000 -- **PARTIALLY CORRECT** (canonical ACD = `2000 + 500*SkillLv`, not fixed 2000)
- effectValue: `100+i*20` -- **WRONG** (should encode meteor/hit pattern, not a simple MATK%)
- **MISSING:** Meteor count and hits-per-meteor logic
- **MISSING:** Random meteor landing positions
- **MISSING:** Stun chance
- **MISSING:** No handler implemented

**Fixes Required:**
1. SP cost array: Replace `20+i*4` with `[20,24,30,34,40,44,50,54,60,64]`
2. ACD: Change from fixed `2000` to `2500+i*500` (2500-7000ms)
3. effectValue: Use to encode something useful (e.g., per-hit MATK% = 125 fixed)

---

### 4. Storm Gust (ID 803)

**Type:** Active, ground-targeted AoE
**Element:** Water
**Range:** 9 cells (450 UE units)
**AoE:** 7x7 cells base, each cell splashes 3x3 = effective 11x11 area (550 UE radius)
**SP Cost:** 78 (fixed, all levels)
**Prerequisites:** Frost Diver Lv1, Jupitel Thunder Lv3

| Level | SP | Cast Time (ms) | MATK% per hit | Total MATK% (10 hits) |
|-------|----|----------------|---------------|----------------------|
| 1 | 78 | 6000 | 140 | 1400 |
| 2 | 78 | 7000 | 180 | 1800 |
| 3 | 78 | 8000 | 220 | 2200 |
| 4 | 78 | 9000 | 260 | 2600 |
| 5 | 78 | 10000 | 300 | 3000 |
| 6 | 78 | 11000 | 340 | 3400 |
| 7 | 78 | 12000 | 380 | 3800 |
| 8 | 78 | 13000 | 420 | 4200 |
| 9 | 78 | 14000 | 460 | 4600 |
| 10 | 78 | 15000 | 500 | 5000 |

**Damage:** `(100 + 40 * SkillLv)%` MATK per hit, 10 hits total. Each hit independently rolls MATK.
**Cast Time Formula:** `5000 + 1000 * SkillLv` ms
**ACD:** 5000ms (fixed)

**Freeze Mechanic (CRITICAL):**
- Every 3rd hit on the same target triggers a freeze check
- The freeze check has a **150% base success rate** (meaning it almost always succeeds, reduced by target's freeze resistance)
- Boss-protocol and Undead-element monsters are immune to freeze
- Frozen targets become Water Lv1 element, take +25% MDEF and -50% DEF
- Frozen targets are pushed in a random direction by each non-freezing hit
- Once frozen, subsequent hits break the freeze (damage breaks freeze)
- The "3rd hit" counter is GLOBAL -- if two different Storm Gusts hit the same target, hits from both count toward the 3-hit freeze threshold

**Mechanics:**
- 10 hits over ~4.6 seconds (1 hit every ~460ms)
- Enemies are pushed in random directions by each hit
- Ground-persistent: enemies entering after cast still take remaining hits
- Multiple Storm Gusts can overlap
- The most powerful Wizard skill for sustained AoE damage + CC

**Existing Data Audit:**
- SP: 78 -- **CORRECT**
- Cast time: `6000+i*1000` -- **CORRECT** (6000, 7000, ..., 15000)
- cooldown: 5000 -- **CORRECT** (this is ACD)
- effectValue: `100+i*20` -- **WRONG** (gives 100-280, should be 140-500 per hit; formula should be `100+i*40` for 140,180,...,500)
- **MISSING:** Freeze mechanic (3rd hit counter, 150% chance)
- **MISSING:** Random push direction
- **MISSING:** No handler implemented

**Fix Required:** effectValue formula should be `100+(i+1)*40` or just `140+i*40` to give 140,180,220,...,500.

---

### 5. Earth Spike (ID 804)

**Type:** Active, single target, multi-hit
**Element:** Earth
**Range:** 9 cells (450 UE units)
**Prerequisites:** Stone Curse Lv1

| Level | SP Cost | Cast Time (ms) | Hits | Total MATK% |
|-------|---------|----------------|------|-------------|
| 1 | 12 | 700 | 1 | 100 |
| 2 | 14 | 1400 | 2 | 200 |
| 3 | 16 | 2100 | 3 | 300 |
| 4 | 18 | 2800 | 4 | 400 |
| 5 | 20 | 3500 | 5 | 500 |

**Damage:** Each spike deals **100% MATK** (1x MATK per hit). Hits = SkillLv.
**SP Formula:** `10 + 2 * SkillLv`
**Cast Time Formula:** `700 * SkillLv` ms
**Special:** Can hit Hidden enemies.

**Mechanics:**
- Functionally identical to bolt spells but Earth element
- Same multi-hit pattern as Cold/Fire/Lightning Bolt but capped at Lv5
- Very useful against Wind-element enemies

**Existing Data Audit:**
- SP: `12+i*2` -- **CORRECT**
- Cast time: `700*(i+1)` -- **CORRECT**
- effectValue: `i+1` (hits) -- **CORRECT**
- Follows same bolt handler pattern -- **CORRECT**
- **STATUS:** Can use the existing bolt handler with Earth element

---

### 6. Heaven's Drive (ID 805)

**Type:** Active, ground-targeted AoE
**Element:** Earth
**Range:** 9 cells (450 UE units)
**AoE:** 5x5 cells (250 UE radius)
**Prerequisites:** Earth Spike Lv3

| Level | SP Cost | Cast Time (ms) | Hits | MATK% per hit | Total MATK% |
|-------|---------|----------------|------|---------------|-------------|
| 1 | 28 | 1000 | 1 | 125 | 125 |
| 2 | 32 | 2000 | 2 | 125 | 250 |
| 3 | 36 | 3000 | 3 | 125 | 375 |
| 4 | 40 | 4000 | 4 | 125 | 500 |
| 5 | 44 | 5000 | 5 | 125 | 625 |

**Damage:** Each hit deals **125% MATK**. Hits = SkillLv.
**SP Formula:** `26 + 2 * SkillLv` (note: our data has `26+i*2` = 26,28,30,32,34; canonical from 03_Skills_Complete is 28,32,36,40,44 = `24+4*SkillLv`)
**Cast Time Formula:** `1000 * SkillLv` ms
**Special:** Can hit Hidden enemies (one of the few AoE skills that reveals hidden targets).

**Mechanics:**
- AoE version of Earth Spike
- All targets in the 5x5 area take the same damage (not split)
- Extremely useful for revealing Thieves using Hiding or Assassins with Cloaking
- Can be used as a ground-check in PvP

**Existing Data Audit:**
- SP: `26+i*2` = 26,28,30,32,34 -- **WRONG** (canonical is 28,32,36,40,44 = `24+(i+1)*4`)
- Cast time: `1000*(i+1)` -- **CORRECT**
- effectValue: `i+1` (hits) -- **CORRECT** but damage per hit is 125% MATK (not stored; handler must use 125)
- **MISSING:** 125% MATK per hit not encoded
- **MISSING:** Hidden reveal mechanic

**Fix Required:** SP cost should be `24+(i+1)*4` giving 28,32,36,40,44.

---

### 7. Quagmire (ID 806)

**Type:** Active, ground-targeted debuff zone
**Element:** Earth
**Range:** 9 cells (450 UE units)
**AoE:** 5x5 cells (250 UE radius)
**Prerequisites:** Heaven's Drive Lv1

| Level | SP Cost | AGI/DEX Reduction | Duration (ms) | Move Speed Reduction |
|-------|---------|-------------------|---------------|---------------------|
| 1 | 5 | 5 | 5000 | 50% |
| 2 | 10 | 10 | 10000 | 50% |
| 3 | 15 | 15 | 15000 | 50% |
| 4 | 20 | 20 | 20000 | 50% |
| 5 | 25 | 25 | 25000 | 50% |

**SP Formula:** `5 * SkillLv`
**Stat Reduction:** `5 * SkillLv` AGI and DEX (from project 03_Skills_Complete.md). Note: some sources say 10*SkillLv (10-50), but canonical rAthena uses 5*SkillLv (5-25).
**Duration Formula:** `5000 * SkillLv` ms
**Move Speed:** Flat 50% reduction regardless of level.

**Mechanics:**
- Creates a persistent ground zone that debuffs enemies walking through it
- Maximum 3 Quagmires per caster at a time
- Affects both monsters and players
- Monster stat reduction capped at 50% of their base stats
- Player stat reduction capped at 25% of their base stats
- Removes: Increase AGI, Two-Hand Quicken, Spear Quicken, Wind Walker, Adrenaline Rush, One-Hand Quicken
- Does NOT affect Boss-protocol monsters (bosses and MVPs immune)
- No cast time (instant), no ACD
- The debuff persists for 5 seconds after leaving the area (debuff lingers)

**Existing Data Audit:**
- SP: `5+i*5` = 5,10,15,20,25 -- **CORRECT**
- Cast time: 0 -- **CORRECT**
- effectValue: `10+i*10` = 10,20,30,40,50 -- **DISCREPANCY** (our code says 10-50, but canonical from 03_Skills_Complete says 5-25; rAthena pre-re uses 5*SkillLv for the stat reduction)
- Duration: `5000+i*2000` = 5000,7000,9000,11000,13000 -- **WRONG** (canonical is 5000*SkillLv = 5000,10000,15000,20000,25000)
- **MISSING:** No handler implemented
- **MISSING:** Move speed reduction not stored
- **MISSING:** Boss immunity check
- **MISSING:** Buff-stripping mechanic
- **MISSING:** Stat reduction caps (50% monster, 25% player)
- **MISSING:** Maximum 3 limit

**Fixes Required:**
1. effectValue: Change from `10+i*10` to `5+i*5` (or `5*(i+1)`) for 5,10,15,20,25
2. Duration: Change from `5000+i*2000` to `5000*(i+1)` for 5000,10000,...,25000

---

### 8. Water Ball (ID 807)

**Type:** Active, single target, multi-hit
**Element:** Water
**Range:** 9 cells (450 UE units)
**Prerequisites:** Cold Bolt Lv1, Lightning Bolt Lv1
**Requirement:** Must have water cells nearby (standing in or near water)

| Level | SP Cost | Cast Time (ms) | MATK% per hit | Max Hits | Water Grid |
|-------|---------|----------------|---------------|----------|------------|
| 1 | 15 | 1000 | 130 | 1 | 1x1 |
| 2 | 20 | 2000 | 160 | 4 | 3x3 |
| 3 | 20 | 3000 | 190 | 9 | 3x3 |
| 4 | 25 | 4000 | 220 | 9 | 5x5 |
| 5 | 25 | 5000 | 250 | 25 | 5x5 |

**Damage:** `(100 + 30 * SkillLv)%` MATK per hit. Max hits depends on water cell availability.
**SP Formula:** `[15, 20, 20, 25, 25]`
**Cast Time Formula:** `1000 * SkillLv` ms

**Water Cell Mechanics:**
- The number of actual hits depends on the number of water cells in the specified grid around the caster
- Each water cell consumed = one hit (cells are consumed during the skill)
- If standing on Waterball Lv5 in deep water with 25 cells: 25 hits * 250% MATK = 6250% MATK total (the highest single-target DPS in the game)
- Without enough water cells, the number of hits is reduced to the available cell count
- Deluge (Sage skill) creates water cells, enabling Water Ball usage anywhere
- Ice Wall cells count as water cells for Water Ball

**Implementation Consideration:** For our server, water cell tracking is a significant new system. Two practical approaches:
1. **Simplified:** Assume max hits if player is in a "water zone" (zone metadata flag)
2. **Full:** Track water cell grid per zone (complex, requires terrain data)

**Existing Data Audit:**
- SP: `15+i*5` = 15,20,25,30,35 -- **WRONG** (canonical is [15,20,20,25,25])
- Cast time: 2500 (fixed) -- **WRONG** (canonical is `1000*SkillLv` = 1000-5000)
- effectValue: `130+i*30` = 130,160,190,220,250 -- **CORRECT** (MATK% per hit)
- **MISSING:** Water cell requirement system
- **MISSING:** Max hits per level
- **MISSING:** No handler implemented

**Fixes Required:**
1. SP cost: Replace with `[15,20,20,25,25]`
2. Cast time: Change from fixed 2500 to `1000*(i+1)` = 1000,2000,3000,4000,5000

---

### 9. Ice Wall (ID 808)

**Type:** Active, ground placement (obstacle creation)
**Element:** Water
**Range:** 9 cells (450 UE units)
**SP Cost:** 20 (fixed, all levels)
**Cast Time:** Instant (0ms)
**Prerequisites:** Stone Curse Lv1, Frost Diver Lv1

| Level | HP per cell | Duration (ms) | Total Wall HP |
|-------|------------|---------------|---------------|
| 1 | 400 | 10000 | 2000 |
| 2 | 600 | 15000 | 3000 |
| 3 | 800 | 20000 | 4000 |
| 4 | 1000 | 25000 | 5000 |
| 5 | 1200 | 30000 | 6000 |
| 6 | 1400 | 35000 | 7000 |
| 7 | 1600 | 40000 | 8000 |
| 8 | 1800 | 45000 | 9000 |
| 9 | 2000 | 50000 | 10000 |
| 10 | 2200 | 55000 | 11000 |

**HP Formula:** `200 + 200 * SkillLv` per cell (from 03_Skills_Complete.md)
**Duration Formula:** `5000 + 5000 * SkillLv` ms (from 03_Skills_Complete.md)

**Mechanics:**
- Creates a wall of 5 ice cells perpendicular to the caster's facing direction
- Each cell is an individual obstacle with its own HP
- Cells block movement for both monsters and players
- Cells also block ranged physical attacks (arrows)
- Cells do NOT block magic
- Cells lose 50 HP per second naturally (durability decay)
- Cells can be destroyed by physical/magical attacks (take damage normally)
- Destroyed Ice Wall cells become "water cells" (enables Water Ball!)
- Fire element skills deal extra damage to Ice Wall cells (Water Lv1 element)
- Maximum: Theoretically unlimited but old cells expire
- Ice Wall cells are considered Water Lv1 element for elemental damage

**Special Interactions:**
- Ice Wall + Water Ball combo: Destroy your own Ice Wall to create water cells for Water Ball
- Ice Wall trapping: Place wall on a moving monster to immobilize it
- Safety Wall + Ice Wall: Physical melee can't reach through Ice Wall to Safety Wall user
- Storm Gust + Ice Wall: Freeze enemies trapped behind Ice Wall

**Existing Data Audit:**
- SP: 20 -- **CORRECT**
- Cast time: 0 -- **CORRECT**
- effectValue: `400+i*200` -- **WRONG** (gives 400-2200 for HP, but canonical is `200+200*SkillLv` = 400-2200. Actually matches! 400+(0)*200=400 at i=0 Lv1, 400+9*200=2200 at i=9 Lv10.)
- Duration: `4000+i*4000` -- **WRONG** (gives 4000-40000, canonical is `5000+5000*SkillLv` = 10000-55000)
- **MISSING:** 5-cell wall creation (obstacle system)
- **MISSING:** HP per cell / destructible cells
- **MISSING:** Movement blocking
- **MISSING:** Water cell creation on destruction
- **MISSING:** No handler implemented

**Fix Required:** Duration should be `5000+5000*(i+1)` = 10000,15000,...,55000. Current `4000+i*4000` gives 4000,8000,...,40000.

---

### 10. Sight Rasher (ID 809)

**Type:** Active, self-centered AoE
**Element:** Fire
**Range:** 0 (centered on caster)
**AoE:** ~7x7 cells around caster (350 UE radius)
**Cast Time:** 500ms
**After-Cast Delay:** 2000ms
**Prerequisites:** Sight Lv1
**Requirement:** Sight buff must be active

| Level | SP Cost | MATK% | Knockback (cells) |
|-------|---------|-------|-------------------|
| 1 | 35 | 120 | 5 |
| 2 | 37 | 140 | 5 |
| 3 | 39 | 160 | 5 |
| 4 | 41 | 180 | 5 |
| 5 | 43 | 200 | 5 |
| 6 | 45 | 220 | 5 |
| 7 | 47 | 240 | 5 |
| 8 | 49 | 260 | 5 |
| 9 | 51 | 280 | 5 |
| 10 | 53 | 300 | 5 |

**Damage:** `(100 + 20 * SkillLv)%` MATK as a single hit to all targets in AoE. From 03_Skills_Complete.md: 120-300% MATK.
**SP Formula:** `33 + 2 * SkillLv`
**Knockback:** 5 cells outward from caster (all directions). Disabled in WoE.

**Mechanics:**
- Requires the Sight buff to be active (consumes/cancels the Sight buff on use)
- Fires 8 fireballs in cardinal + diagonal directions
- All enemies within the AoE take one hit regardless of direction
- The fireballs are visual only; the damage is a single AoE check
- Knockback pushes enemies away from caster in 8 directions
- Useful as an emergency defensive skill when surrounded

**Existing Data Audit:**
- SP: `35+i*2` -- **CORRECT** (35,37,...,53)
- Cast time: 500 -- **CORRECT**
- cooldown: 2000 -- **CORRECT** (this is ACD)
- effectValue: `100+i*20` = 100,120,...,280 -- **WRONG** (should be 120,140,...,300 = `100+(i+1)*20`)
- **MISSING:** Sight requirement check
- **MISSING:** Sight consumption on use
- **MISSING:** Knockback (5 cells)
- **MISSING:** No handler implemented

**Fix Required:** effectValue should be `100+(i+1)*20` or `120+i*20` for 120,140,...,300.

---

### 11. Fire Pillar (ID 810)

**Type:** Active, ground trap (delayed activation)
**Element:** Fire
**Range:** 9 cells (450 UE units)
**AoE:** 1x1 cell (the trap cell itself)
**SP Cost:** 75 (fixed, all levels)
**Prerequisites:** Fire Wall Lv1
**Catalyst:** Blue Gemstone required at Lv6+ only

| Level | Cast Time (ms) | Hits | Damage Formula |
|-------|----------------|------|----------------|
| 1 | 3000 | 3 | 3 * (50 + MATK/5) |
| 2 | 2700 | 4 | 4 * (50 + MATK/5) |
| 3 | 2400 | 5 | 5 * (50 + MATK/5) |
| 4 | 2100 | 6 | 6 * (50 + MATK/5) |
| 5 | 1800 | 7 | 7 * (50 + MATK/5) |
| 6 | 1500 | 8 | 8 * (50 + MATK/5) |
| 7 | 1200 | 9 | 9 * (50 + MATK/5) |
| 8 | 900 | 10 | 10 * (50 + MATK/5) |
| 9 | 600 | 11 | 11 * (50 + MATK/5) |
| 10 | 300 | 12 | 12 * (50 + MATK/5) |

**Damage:** Each hit deals `50 + MATK / 5` fixed damage. **Ignores MDEF entirely.**
**Cast Time Formula:** `3300 - 300 * SkillLv` ms (or equivalently `300 + i*300` descending)
**Hits Formula:** `SkillLv + 2`
**Duration:** 30 seconds (trap stays on ground until triggered or expires)
**Max Concurrent:** 5 Fire Pillars per caster

**Mechanics:**
- Acts as a ground trap that activates when an enemy steps on it
- When triggered, the pillar erupts and hits the triggering enemy with all hits at once
- IGNORES MDEF -- this is the key feature; damage is purely `(50 + MATK/5) * hits`
- Cannot be cast on occupied cells (cells with monsters, players, or other ground effects)
- Blue Gemstone required only at Lv6-10
- Lv1-5 have no catalyst cost
- The pillar is invisible to enemies in PvP (trap mechanic)
- Multiple fire pillars can be placed near each other for stacking damage

**Existing Data Audit:**
- SP: 75 -- **CORRECT**
- Cast time: `300+i*300` -- **WRONG** direction (gives 300,600,...,3000 -- should give 3000,2700,...,300 = `3300-(i+1)*300` or `3000-i*300`)
- effectValue: `i+3` = 3,4,...,12 (hits) -- **CORRECT**
- Duration: 30000 -- **CORRECT**
- SKILL_CATALYSTS entry: `{ itemId: 717, quantity: 1, minLevel: 6 }` -- **CORRECT** (Blue Gem Lv6+ only)
- **MISSING:** MDEF ignore in damage formula
- **MISSING:** Max 5 concurrent limit
- **MISSING:** Trap activation mechanic (ground effect that triggers on enemy contact)
- **MISSING:** No handler implemented

**Fix Required:** Cast time formula is inverted. Should be `3000-i*300` for 3000,2700,...,300.

---

### 12. Frost Nova (ID 811)

**Type:** Active, self-centered AoE freeze
**Element:** Water
**Range:** 0 (centered on caster)
**AoE:** 5x5 cells (250 UE radius) -- some sources say 7x7
**Prerequisites:** Frost Diver Lv1, Ice Wall Lv1

| Level | SP Cost | Cast Time (ms) | MATK% | Freeze Chance % |
|-------|---------|----------------|-------|----------------|
| 1 | 45 | 6000 | 73 | 38 |
| 2 | 43 | 5600 | 80 | 43 |
| 3 | 41 | 5200 | 87 | 48 |
| 4 | 39 | 4800 | 94 | 53 |
| 5 | 37 | 4400 | 101 | 58 |
| 6 | 35 | 4000 | 108 | 63 |
| 7 | 33 | 4000 | 115 | 68 |
| 8 | 31 | 4000 | 122 | 73 |
| 9 | 29 | 4000 | 129 | 78 |
| 10 | 27 | 4000 | 136 | 83 |

**Damage:** `(66 + 7 * SkillLv)%` MATK = 73,80,87,...,136%
**SP Formula:** `47 - 2 * SkillLv` = 45,43,41,...,27
**Cast Time:** Lv1-6: `6600 - 400*SkillLv`; Lv7-10: 4000ms fixed
**Freeze Chance:** `33 + 5 * SkillLv` % = 38,43,48,...,83%

**Mechanics:**
- Centered on the caster (cannot be ground-targeted elsewhere)
- Deals Water damage and has a chance to freeze all enemies in range
- Can freeze Water-element enemies (unlike most freeze skills in older implementations)
- Boss/Undead immune to freeze
- The freeze effect follows standard freeze rules (Water Lv1 element change, +25% MDEF, -50% DEF)
- Weaker damage than Storm Gust but faster cast time and easier to use
- Good for emergency CC when surrounded

**Existing Data Audit:**
- SP: `45-i*2` -- **CORRECT** (45,43,...,27)
- Cast time: `4000+i*200` -- **WRONG** (gives 4000,4200,...,5800 -- ascending, should be descending or stepped)
- effectValue: `73+i*6` -- **WRONG** (gives 73,79,85,...,127; should be `66+7*(i+1)` = 73,80,87,...,136 i.e. `73+i*7`)
- **MISSING:** Freeze chance per level
- **MISSING:** No handler implemented

**Fixes Required:**
1. Cast time: Replace with `[6000,5600,5200,4800,4400,4000,4000,4000,4000,4000]`
2. effectValue: Change from `73+i*6` to `73+i*7` for 73,80,87,...,136

---

### 13. Sense / Monster Property (ID 812)

**Type:** Active, single target (information skill)
**Element:** Neutral
**Range:** 9 cells (450 UE units)
**SP Cost:** 10
**Cast Time:** 0 (instant)
**Prerequisites:** None

**Effect:** Opens an information window displaying target monster's stats:
- Current HP / Max HP
- Element and element level
- Race
- Size
- Base Level
- DEF / MDEF
- Base EXP / Job EXP

**Mechanics:**
- Party members also see the information window
- Works only on monsters (not players in PvP)
- No damage, no cooldown
- Purely informational

**Existing Data Audit:**
- SP: 10 -- **CORRECT**
- Cast time: 0 -- **CORRECT**
- All values match canonical
- **MISSING:** No handler implemented (server needs to send monster info to client)

**Implementation:** Simple -- on skill use, server looks up the target monster's stats from the template and sends a `skill:sense_result` event with the data. Client displays in a popup.

---

### 14. Sight Blaster (ID 813) -- Quest Skill

**Type:** Active, self-buff (reactive damage)
**Element:** Fire
**SP Cost:** 40
**Cast Time:** 700ms (some sources say instant)
**Duration:** 120 seconds (2 minutes)
**Prerequisites:** Quest skill -- requires Job Level 40 as Wizard

**Damage:** 100% MATK Fire damage to the first enemy that enters melee range (~2 cells)
**Knockback:** 3 cells backward

**Mechanics:**
- Creates a protective fireball that orbits the caster
- When any enemy enters within 2 cells, the fireball automatically attacks that enemy
- Deals 100% MATK Fire damage
- Knocks the enemy back 3 cells
- The skill then ends (one-time activation)
- If no enemy triggers it within 120 seconds, it expires
- WoE: knockback disabled
- Essentially a single-use "reactive" damage skill -- cast preemptively

**Existing Data Audit:**
- SP: 40 -- **CORRECT**
- Cast time: 700 -- **ACCEPTABLE** (some sources say 0, but 700ms is fine)
- effectValue: 100 (100% MATK) -- **CORRECT**
- Duration: 120000 (2 min) -- **CORRECT**
- questSkill: true -- **CORRECT**
- **MISSING:** Reactive trigger mechanic (proximity check)
- **MISSING:** Knockback 3 cells
- **MISSING:** No handler implemented

---

## Existing Implementation Audit

### Skill Data Corrections Needed (`ro_skill_data_2nd.js`)

| Skill | Field | Current | Canonical | Fix |
|-------|-------|---------|-----------|-----|
| Lord of Vermilion (801) | castTime | `10500+i*500` (ascending) | `15000-i*500` (descending) | Reverse formula |
| Meteor Storm (802) | spCost | `20+i*4` (linear) | `[20,24,30,34,40,44,50,54,60,64]` | Use explicit array |
| Meteor Storm (802) | cooldown (ACD) | 2000 (fixed) | `2500+i*500` (2500-7000) | Variable ACD |
| Storm Gust (803) | effectValue | `100+i*20` (100-280) | `140+i*40` (140-500) | Fix MATK% formula |
| Heaven's Drive (805) | spCost | `26+i*2` (26-34) | `24+(i+1)*4` (28-44) | Fix SP formula |
| Quagmire (806) | effectValue | `10+i*10` (10-50) | `5+i*5` (5-25) | Fix reduction amounts |
| Quagmire (806) | duration | `5000+i*2000` (5k-13k) | `5000*(i+1)` (5k-25k) | Fix duration formula |
| Water Ball (807) | spCost | `15+i*5` (15-35) | `[15,20,20,25,25]` | Use explicit array |
| Water Ball (807) | castTime | 2500 (fixed) | `1000*(i+1)` (1k-5k) | Variable cast time |
| Ice Wall (808) | duration | `4000+i*4000` (4k-40k) | `5000*(i+1)+5000` (10k-55k) | Fix duration |
| Sight Rasher (809) | effectValue | `100+i*20` (100-280) | `120+i*20` (120-300) | Off by 20 at all levels |
| Fire Pillar (810) | castTime | `300+i*300` (300-3000) | `3000-i*300` (3000-300) | Reverse direction |
| Frost Nova (811) | castTime | `4000+i*200` (4k-5.8k) | `[6000,5600,5200,4800,4400,4000,4000,4000,4000,4000]` | Use explicit array |
| Frost Nova (811) | effectValue | `73+i*6` (73-127) | `73+i*7` (73-136) | Fix MATK% formula |
| Frost Nova (811) | spCost | `45-i*2` (45-27) | `47-2*(i+1)` (45-27) | Already correct |

### Handler Implementation Status

| Skill | Handler Exists | Ground Effect | Status |
|-------|---------------|---------------|--------|
| Jupitel Thunder | NO | N/A (single target) | Needs multi-hit + knockback handler |
| Lord of Vermilion | NO | YES (4-wave timed AoE) | Needs ground effect + timed waves |
| Meteor Storm | NO | YES (random meteor drops) | Needs ground effect + random positions |
| Storm Gust | NO | YES (10-hit timed AoE + freeze) | Needs ground effect + freeze counter |
| Earth Spike | NO | N/A (single target) | Can reuse bolt handler |
| Heaven's Drive | NO | N/A (instant AoE) | Similar to Thunderstorm handler |
| Quagmire | NO | YES (persistent debuff zone) | Needs ground effect + debuff application |
| Water Ball | NO | N/A (single target) | Needs water cell check |
| Ice Wall | NO | YES (obstacle creation) | Needs obstacle system |
| Sight Rasher | NO | N/A (instant AoE) | Simple AoE damage + KB |
| Fire Pillar | NO | YES (ground trap) | Needs trap activation mechanic |
| Frost Nova | NO | N/A (instant AoE) | AoE damage + freeze check |
| Sense | NO | N/A | Simple info query |
| Sight Blaster | NO | N/A (self buff) | Needs reactive proximity trigger |

**Total: 0 of 14 Wizard skills have handlers implemented.**

---

## New Systems Required

### System 1: Multi-Wave Ground Effect (HIGH Priority)

**Required for:** Storm Gust, Lord of Vermilion, Meteor Storm

The existing `createGroundEffect()` system supports Fire Wall (hit-on-contact) and Safety Wall (hit-absorbing). The Wizard AoE spells require a new pattern: **timed multi-wave damage** where the ground effect periodically damages all enemies in the area.

**Architecture:**

```javascript
// New ground effect type: 'timed_aoe'
createGroundEffect({
    type: 'storm_gust',           // or 'lord_of_vermilion' or 'meteor_storm'
    casterId: characterId,
    casterName: player.characterName,
    zone: player.zone,
    x, y, z,                      // Center position
    radius: 550,                  // AoE radius in UE units
    duration: 4600,               // Total effect duration
    element: 'water',             // Skill element
    waveCount: 10,                // Number of damage waves
    waveInterval: 460,            // ms between waves
    wavesSent: 0,                 // Counter
    matkPercent: 500,             // MATK% per wave hit
    skillId: 803,
    skillName: 'Storm Gust',
    // Storm Gust specific:
    freezeEveryN: 3,              // Freeze check every Nth hit
    freezeChance: 150,            // 150% base freeze chance
    randomPush: true,             // Push enemies randomly
    // Meteor Storm specific:
    meteorCount: 6,               // Number of individual meteors
    hitsPerMeteor: 5,             // Hits each meteor does
    meteorRadius: 350,            // Individual meteor splash
    stunChance: 30,               // Per-hit stun chance
    // Lord of Vermilion specific:
    blindChance: 40,              // Per-wave blind chance
});
```

**Ground Effect Tick Enhancement (500ms interval):**

```javascript
// In the existing ground effect tick loop:
if (effect.type === 'storm_gust' || effect.type === 'lord_of_vermilion') {
    const elapsed = now - effect.createdAt;
    const wavesDue = Math.floor(elapsed / effect.waveInterval);
    while (effect.wavesSent < wavesDue && effect.wavesSent < effect.waveCount) {
        // Process one wave: damage all enemies in radius
        for (const [eid, enemy] of enemies.entries()) {
            if (enemy.isDead || enemy.zone !== effect.zone) continue;
            const dist = distanceBetween(enemy, effect);
            if (dist > effect.radius) continue;

            // Calculate damage
            const caster = connectedPlayers.get(effect.casterId);
            const dmgResult = calculateMagicSkillDamage(...);
            // Apply damage, broadcast, check status effects
        }
        effect.wavesSent++;
    }
}

if (effect.type === 'meteor_storm') {
    // Each meteor is a sub-effect with random position
    // Process meteors at staggered intervals
    // Each meteor has its own splash radius
}
```

### System 2: Storm Gust Freeze Counter (HIGH Priority)

**Required for:** Storm Gust (and potentially other multi-hit freeze skills)

Storm Gust's freeze mechanic requires tracking how many times each target has been hit by ANY Storm Gust instance. The 3rd hit triggers a freeze check.

```javascript
// Per-target hit counter (global, not per-cast)
// Stored on enemy/player object or in a separate Map
const stormGustHitCounters = new Map(); // targetKey -> hitCount

function processStormGustHit(targetKey, effect) {
    const count = (stormGustHitCounters.get(targetKey) || 0) + 1;
    stormGustHitCounters.set(targetKey, count);

    if (count % 3 === 0) {
        // Freeze check with 150% base chance
        // Boss/Undead immune
        const freezeSuccess = rollFreeze(target, 150);
        if (freezeSuccess) {
            applyStatusEffect(target, 'freeze', ...);
            stormGustHitCounters.delete(targetKey); // Reset on freeze
        }
    }
}

// Clean up counters when targets die or leave zone
```

### System 3: Quagmire Ground Debuff Zone (MEDIUM Priority)

**Required for:** Quagmire

A persistent ground zone that applies a debuff to enemies/players standing in it. Unlike damage zones, this applies stat modifications.

```javascript
createGroundEffect({
    type: 'quagmire',
    casterId: characterId,
    zone: player.zone,
    x, y, z,
    radius: 250,           // 5x5 cells
    duration: duration,     // 5000-25000ms
    agiReduction: reduction,
    dexReduction: reduction,
    moveSpeedReduction: 50, // 50% flat
    maxPerCaster: 3,
});

// In ground effect tick:
if (effect.type === 'quagmire') {
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== effect.zone) continue;
        if (isBossProtocol(enemy)) continue; // Boss immune
        const dist = distanceBetween(enemy, effect);
        if (dist <= effect.radius) {
            // Apply debuff (if not already applied by this quagmire)
            applyQuagmireDebuff(enemy, effect);
        } else {
            // Remove debuff if enemy left area (with 5s linger)
            removeQuagmireDebuff(enemy, effect);
        }
    }
}
```

**Debuff Mechanics:**
- Reduce AGI and DEX by the skill's reduction amount
- Cap reduction: 50% of base for monsters, 25% of base for players
- Reduce movement speed by 50%
- Strip speed-boosting buffs (Increase AGI, Two-Hand Quicken, etc.)
- Debuff lingers 5 seconds after leaving the area

### System 4: Ice Wall Obstacle System (MEDIUM Priority)

**Required for:** Ice Wall

Creates physical obstacles on the map that block movement. This is a fundamentally new system that requires:

1. **Obstacle Registry:** Track which cells have obstacles
2. **Pathfinding Integration:** AI pathfinding must avoid obstacle cells
3. **Destructible HP:** Each cell has HP, takes damage, can be destroyed
4. **Water Cell Conversion:** Destroyed cells become water cells
5. **Element:** Each cell is Water Lv1 (takes extra Fire damage)

```javascript
// Ice Wall cell structure
const iceWallCells = new Map(); // cellKey -> { hp, maxHp, element, ownerId, expiresAt, x, y, z }

function createIceWall(casterId, centerX, centerY, centerZ, facing, level) {
    const hpPerCell = 200 + 200 * level;
    const duration = 5000 + 5000 * level;
    const cells = [];

    // Calculate 5 cells perpendicular to facing direction
    for (let i = -2; i <= 2; i++) {
        const cellX = centerX + perpX * i * CELL_SIZE;
        const cellY = centerY + perpY * i * CELL_SIZE;
        const cellKey = `${Math.floor(cellX/50)}_${Math.floor(cellY/50)}`;

        iceWallCells.set(cellKey, {
            hp: hpPerCell,
            maxHp: hpPerCell,
            element: { type: 'water', level: 1 },
            ownerId: casterId,
            expiresAt: Date.now() + duration,
            x: cellX, y: cellY, z: centerZ,
            decayRate: 50 // HP lost per second
        });
        cells.push({ x: cellX, y: cellY, z: centerZ, hp: hpPerCell });
    }
    return cells;
}
```

**Implementation Priority Note:** Ice Wall is the most complex new system. Consider a simplified version first (just a blocking ground effect without per-cell HP) and iterate.

### System 5: Fire Pillar Trap System (MEDIUM Priority)

**Required for:** Fire Pillar

Ground traps that activate when an enemy walks over them. Extends the existing ground effect system.

```javascript
createGroundEffect({
    type: 'fire_pillar',
    casterId: characterId,
    zone: player.zone,
    x, y, z,
    radius: 50,            // 1x1 cell
    duration: 30000,        // 30 second lifetime
    hits: level + 2,        // 3-12 hits on activation
    activated: false,       // Not yet triggered
    ignoreMDEF: true,       // Key mechanic
    damageFormula: 'fire_pillar', // Special damage calc
});

// In ground effect tick:
if (effect.type === 'fire_pillar' && !effect.activated) {
    // Check if any enemy is standing on the trap
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.zone !== effect.zone) continue;
        const dist = distanceBetween(enemy, effect);
        if (dist <= effect.radius) {
            // ACTIVATE! Deal all hits at once
            effect.activated = true;
            const caster = connectedPlayers.get(effect.casterId);
            const matk = calculateMATK(caster);
            const dmgPerHit = 50 + Math.floor(matk / 5); // MDEF ignored!
            const totalDmg = dmgPerHit * effect.hits;
            // Apply damage, broadcast, remove effect
            break;
        }
    }
}
```

### System 6: Meteor Storm Random Positioning (MEDIUM Priority)

**Required for:** Meteor Storm

Meteors fall at random positions within the AoE area. Each meteor has its own splash zone.

```javascript
function createMeteorStorm(casterId, centerX, centerY, centerZ, level, zone) {
    const meteorCount = Math.floor((level + 2) / 2); // 2-6 meteors
    const hitsPerMeteor = Math.floor((level + 1) / 2); // 1-5 hits
    const aoeRadius = 350; // 7x7 cells
    const meteorSplash = 350; // Each meteor splashes 7x7
    const stunChance = 3 * level;

    // Generate random meteor positions within the AoE
    const meteors = [];
    for (let m = 0; m < meteorCount; m++) {
        const angle = Math.random() * 2 * Math.PI;
        const dist = Math.random() * aoeRadius;
        meteors.push({
            x: centerX + Math.cos(angle) * dist,
            y: centerY + Math.sin(angle) * dist,
            delay: m * 300, // Stagger meteor falls
        });
    }

    // Create ground effect with meteor data
    return createGroundEffect({
        type: 'meteor_storm',
        casterId, zone,
        x: centerX, y: centerY, z: centerZ,
        radius: aoeRadius,
        meteors,
        hitsPerMeteor,
        meteorSplash,
        stunChance,
        matkPercent: 125,
        element: 'fire',
        duration: 3000 + meteorCount * 300, // Total effect time
    });
}
```

### System 7: Sight Blaster Reactive Trigger (LOW Priority)

**Required for:** Sight Blaster

A buff that auto-triggers when an enemy enters proximity.

```javascript
// In the enemy AI tick or a dedicated proximity check:
function checkSightBlasterTriggers() {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (!player.sightBlasterActive) continue;

        for (const [eid, enemy] of enemies.entries()) {
            if (enemy.isDead || enemy.zone !== player.zone) continue;
            const dist = distanceBetween(player, enemy);
            if (dist <= 100) { // 2 cells
                // Trigger Sight Blaster
                const casterStats = getEffectiveStats(player);
                const dmgResult = calculateMagicSkillDamage(casterStats, enemy.stats,
                    enemy.hardMdef, 100, 'fire', getEnemyTargetInfo(enemy));
                // Apply damage, knockback 3 cells, remove buff
                player.sightBlasterActive = false;
                break;
            }
        }
    }
}
```

### System 8: Water Cell Tracking (LOW Priority -- Simplified)

**Required for:** Water Ball (full implementation)

For a simplified initial implementation, add a `hasWater` flag to zone data and assume max hits when in a water zone. Full implementation would track per-cell water presence.

```javascript
// Simplified approach in ro_zone_data.js:
const ZONE_WATER_INFO = {
    'prontera_south': { hasWater: false },
    'payon_cave_1': { hasWater: false },
    'byalan_dungeon_1': { hasWater: true, waterCells: 'full' }, // Underwater dungeon
    // etc.
};

// In Water Ball handler:
const zoneWater = ZONE_WATER_INFO[player.zone];
if (!zoneWater || !zoneWater.hasWater) {
    socket.emit('skill:error', { message: 'No water nearby' });
    return;
}
const maxHits = WATER_BALL_MAX_HITS[learnedLevel - 1]; // [1, 4, 9, 9, 25]
```

---

## Ground Effect System Architecture

### Current Ground Effect Types

| Type | Behavior | Implemented |
|------|----------|-------------|
| `fire_wall` | Hit-on-contact, knockback, charges consumed | YES |
| `safety_wall` | Absorb melee hits, charges consumed | YES |
| `pneuma` | Block ranged attacks | YES |
| `warp_portal` | Teleport on contact | YES |

### New Ground Effect Types Needed

| Type | Behavior | Priority |
|------|----------|----------|
| `storm_gust` | 10 timed waves, freeze counter, random push | HIGH |
| `lord_of_vermilion` | 4 timed waves, blind check | HIGH |
| `meteor_storm` | Random meteor positions, multi-splash, stun | HIGH |
| `quagmire` | Persistent debuff zone (AGI/DEX/speed) | MEDIUM |
| `fire_pillar` | Trap -- activates on contact, all damage at once | MEDIUM |
| `ice_wall` | Obstacle cells with HP, blocks movement | MEDIUM |

### Ground Effect Tick Enhancement

The existing ground effect tick runs at 500ms intervals. For smooth multi-wave skills, this may need adjustment:

- **Storm Gust:** 10 hits over ~4.6s = ~460ms between hits. 500ms tick is close enough (may get 9-10 hits in practice).
- **Lord of Vermilion:** 4 waves over 4s = 1000ms between waves. 500ms tick works perfectly (every other tick).
- **Meteor Storm:** Meteors staggered over ~2-3s. 500ms tick handles this with meteor delay tracking.
- **Quagmire:** Debuff zone, only needs presence check each tick. 500ms fine.
- **Fire Pillar:** Trap activation is instant on contact check. 500ms fine.

**Recommendation:** Keep the 500ms tick but allow sub-tick timing via `lastWaveTime` tracking:

```javascript
if (now - effect.lastWaveTime >= effect.waveInterval) {
    // Process next wave
    effect.lastWaveTime = now;
    effect.wavesSent++;
}
```

---

## Implementation Priority

### Phase 1: Bolt-Pattern Skills (EASY -- 2-3 hours)

Skills that follow the existing multi-hit bolt pattern and need minimal new infrastructure:

1. **Earth Spike (804)** -- Reuse bolt handler with Earth element. Minimal code.
2. **Heaven's Drive (805)** -- Similar to Thunderstorm handler but Earth, 125% MATK, reveal hidden.
3. **Jupitel Thunder (800)** -- Multi-hit bolt + knockback (knockback system exists from Fire Wall).
4. **Sense (812)** -- Simple info query, send monster data to client.

### Phase 2: Self-Centered AoE Skills (MODERATE -- 3-4 hours)

Skills centered on the caster with instant or near-instant effects:

5. **Sight Rasher (809)** -- AoE damage + knockback, requires Sight active check.
6. **Frost Nova (811)** -- AoE damage + freeze chance, similar to Frost Diver but AoE.
7. **Sight Blaster (813)** -- Reactive buff with proximity trigger.

### Phase 3: Ground Effect AoE Skills (HARD -- 8-12 hours)

The signature Wizard skills requiring the multi-wave ground effect system:

8. **Storm Gust (803)** -- Highest priority. 10-wave water AoE with freeze counter.
9. **Lord of Vermilion (801)** -- 4-wave wind AoE with blind.
10. **Meteor Storm (802)** -- Random meteor positioning with stun.

### Phase 4: Ground Utility Skills (HARD -- 6-8 hours)

Utility skills requiring new ground-effect subtypes:

11. **Quagmire (806)** -- Persistent debuff zone.
12. **Fire Pillar (810)** -- Ground trap with MDEF-ignore damage.
13. **Water Ball (807)** -- Single target multi-hit with water cell requirement.
14. **Ice Wall (808)** -- Obstacle creation (most complex, can be deferred).

### Phase 0: Data Fixes (TRIVIAL -- 30 minutes)

Before any handlers, fix the skill data in `ro_skill_data_2nd.js`:
- Lord of Vermilion cast time direction
- Meteor Storm SP costs and ACD
- Storm Gust effectValue MATK%
- Heaven's Drive SP costs
- Quagmire effectValue and duration
- Water Ball SP and cast time
- Ice Wall duration
- Sight Rasher effectValue offset
- Fire Pillar cast time direction
- Frost Nova cast time and effectValue

---

## Integration Points

### Existing Systems to Hook Into

| System | Integration Point | Skills Using It |
|--------|------------------|-----------------|
| `calculateMagicSkillDamage()` | All damage-dealing Wizard skills | All except Sense, Ice Wall, Quagmire |
| `createGroundEffect()` | Persistent ground effects | SG, LoV, MS, Quagmire, Fire Pillar, Ice Wall |
| `applyStatusEffect()` | Freeze, Stun, Blind application | SG, MS, LoV, Frost Nova |
| `broadcastToZone()` | All skill effects | All |
| `applySkillDelays()` | After-cast delay | All |
| `getEffectiveStats()` | MATK calculation | All damage skills |
| `getBuffStatModifiers()` | CC check (preventsCasting) | All active skills |
| `checkDamageBreakStatuses()` | Break freeze/stone on damage | All damage skills |
| `SKILL_CATALYSTS` | Blue Gemstone for Fire Pillar Lv6+ | Fire Pillar |
| `setEnemyAggro()` | Aggro transfer on skill damage | All damage skills vs monsters |

### Socket Events to Add

| Event | Direction | Purpose |
|-------|-----------|---------|
| `skill:ground_effect_created` | Server -> Client | New ground AoE spawned (SG, LoV, MS, Quagmire, etc.) |
| `skill:ground_effect_removed` | Server -> Client | Ground effect expired/destroyed |
| `skill:effect_damage` | Server -> Client | Damage from ground effect wave |
| `skill:sense_result` | Server -> Client | Monster property info window data |
| `skill:ice_wall_created` | Server -> Client | Ice wall cells placed |
| `skill:ice_wall_cell_destroyed` | Server -> Client | Individual ice cell broken |
| `skill:quagmire_debuff` | Server -> Client | Quagmire debuff applied/removed |

Most of these events already exist in the system (ground_effect_created, effect_damage). Only `skill:sense_result` and ice wall-specific events are truly new.

### VFX Requirements

| Skill | VFX Type | Notes |
|-------|----------|-------|
| Jupitel Thunder | Projectile + impact | Lightning ball traveling to target |
| Lord of Vermilion | Ground AoE persistent | Massive lightning field, 4-second loop |
| Meteor Storm | Ground AoE + projectiles | Meteors falling from sky at random positions |
| Storm Gust | Ground AoE persistent | Blizzard/ice storm, 10-hit loop |
| Earth Spike | Impact at target | Earth spikes rising from ground |
| Heaven's Drive | Ground AoE impact | Earth spikes in 5x5 area |
| Quagmire | Ground AoE persistent | Swamp/marsh visual, lingers |
| Water Ball | Projectile multi-hit | Water spheres hitting target |
| Ice Wall | Ground placement | 5 ice blocks appearing |
| Sight Rasher | Self AoE burst | 8 fireballs radiating outward |
| Fire Pillar | Ground trap + eruption | Dormant trap -> fire eruption |
| Frost Nova | Self AoE burst | Ice spikes radiating from caster |
| Sense | UI popup | No world VFX needed |
| Sight Blaster | Self orbiting + burst | Fireball orbiting caster |

---

## Wizard-Specific Constants

### Data Tables Needed

```javascript
// Jupitel Thunder knockback per level
const JUPITEL_KNOCKBACK = [2, 3, 3, 4, 4, 5, 5, 6, 6, 7];

// Meteor Storm SP costs (non-linear)
const METEOR_STORM_SP = [20, 24, 30, 34, 40, 44, 50, 54, 60, 64];

// Meteor Storm meteor count per level
const METEOR_STORM_METEORS = [2, 2, 3, 3, 4, 4, 5, 5, 6, 6];

// Meteor Storm hits per meteor
const METEOR_STORM_HITS_PER = [1, 1, 2, 2, 3, 3, 4, 4, 5, 5];

// Meteor Storm ACD per level
const METEOR_STORM_ACD = [2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000];

// Storm Gust MATK% per hit
const STORM_GUST_MATK = [140, 180, 220, 260, 300, 340, 380, 420, 460, 500];

// Lord of Vermilion MATK% per wave
const LOV_MATK_PER_WAVE = [100, 120, 140, 160, 180, 200, 220, 240, 260, 280];

// Lord of Vermilion blind chance
const LOV_BLIND_CHANCE = [4, 8, 12, 16, 20, 24, 28, 32, 36, 40];

// Frost Nova freeze chance
const FROST_NOVA_FREEZE = [38, 43, 48, 53, 58, 63, 68, 73, 78, 83];

// Frost Nova cast time (stepped)
const FROST_NOVA_CAST = [6000, 5600, 5200, 4800, 4400, 4000, 4000, 4000, 4000, 4000];

// Water Ball max hits
const WATER_BALL_MAX_HITS = [1, 4, 9, 9, 25];

// Water Ball SP costs
const WATER_BALL_SP = [15, 20, 20, 25, 25];

// Sightrasher MATK%
const SIGHTRASHER_MATK = [120, 140, 160, 180, 200, 220, 240, 260, 280, 300];

// Fire Pillar cast time (descending)
const FIRE_PILLAR_CAST = [3000, 2700, 2400, 2100, 1800, 1500, 1200, 900, 600, 300];

// Ice Wall duration
const ICE_WALL_DURATION = [10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 55000];

// Ice Wall HP per cell
const ICE_WALL_HP = [400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000, 2200];
```

### UE Unit Conversions

| RO Cells | UE Units | Used For |
|----------|----------|----------|
| 1x1 | 50 | Fire Pillar trap zone |
| 3x3 | 150 | Napalm Beat AoE |
| 5x5 | 250 | Heaven's Drive, Quagmire, Frost Nova AoE |
| 7x7 | 350 | Meteor Storm splash, Sight Rasher |
| 9x9 | 450 | Lord of Vermilion, Storm Gust base AoE |
| 11x11 | 550 | LoV/SG effective AoE (with splash) |
| 9 cells range | 450 | Standard magic range |

### Key Formulas Summary

| Skill | Damage Formula | Notes |
|-------|---------------|-------|
| Jupitel Thunder | `hits * 100% MATK` | hits = 2 + SkillLv |
| Lord of Vermilion | `4 * (80+20*Lv)% MATK` | 4 waves |
| Meteor Storm | `meteors * hitsPerMeteor * 125% MATK` | Random overlap |
| Storm Gust | `10 * (100+40*Lv)% MATK` | Per-hit MATK roll |
| Earth Spike | `Lv * 100% MATK` | Same as bolts |
| Heaven's Drive | `Lv * 125% MATK` | AoE |
| Quagmire | No damage | Debuff only |
| Water Ball | `maxHits * (100+30*Lv)% MATK` | Water cells limit hits |
| Ice Wall | No damage | Creates obstacles |
| Sight Rasher | `(100+20*Lv)% MATK` | Single AoE hit |
| Fire Pillar | `(Lv+2) * (50 + MATK/5)` | Ignores MDEF |
| Frost Nova | `(66+7*Lv)% MATK` | + freeze check |
| Sight Blaster | `100% MATK` | Single reactive hit |

---

## Sources

- [Wizard - iRO Wiki Classic](https://irowiki.org/classic/Wizard)
- [Storm Gust - iRO Wiki Classic](https://irowiki.org/classic/Storm_Gust)
- [Meteor Storm - iRO Wiki Classic](https://irowiki.org/classic/Meteor_Storm)
- [Jupitel Thunder - iRO Wiki Classic](https://irowiki.org/classic/Jupitel_Thunder)
- [Lord of Vermilion - iRO Wiki Classic](https://irowiki.org/classic/Lord_of_Vermilion)
- [Water Ball - iRO Wiki Classic](https://irowiki.org/classic/Water_Ball)
- [Ice Wall - iRO Wiki Classic](https://irowiki.org/classic/Ice_Wall)
- [Quagmire - iRO Wiki Classic](https://irowiki.org/classic/Quagmire)
- [Frost Nova - iRO Wiki Classic](https://irowiki.org/classic/Frost_Nova)
- [Sightrasher - iRO Wiki Classic](https://irowiki.org/classic/Sightrasher)
- [Fire Pillar - iRO Wiki Classic](https://irowiki.org/classic/Fire_Pillar)
- [Sight Blaster - iRO Wiki](https://irowiki.org/wiki/Sight_Blaster)
- [Earth Spike - iRO Wiki Classic](https://irowiki.org/classic/Earth_Spike)
- [Heaven's Drive - iRO Wiki Classic](https://irowiki.org/classic/Heaven%27s_Drive)
- [Wizard Skill DB - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=9)
- [Jupitel Thunder - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=84)
- [Storm Gust - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=89)
- [Meteor Storm - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=83)
- [Lord of Vermilion - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=85)
- [Fire Pillar - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=80)
- [Frost Nova - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=88)
- [Quagmire - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=92)
- [rAthena pre-re skill_db.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [Mystical Amplification - iRO Wiki](https://irowiki.org/wiki/Mystical_Amplification) (High Wizard only, documented for awareness)
- [Cast Time - iRO Wiki Classic](https://irowiki.org/classic/Skills)
- [Ragnarok Online Classic Calc](https://rocalc.com)
- Project internal: `RagnaCloneDocs/03_Skills_Complete.md` (primary data reference)
- Project internal: `server/src/ro_skill_data_2nd.js` (existing skill definitions)
- Project internal: `server/src/ro_damage_formulas.js` (calculateMagicalDamage infrastructure)
- Project internal: `server/src/index.js` (ground effect system, Fire Wall handler as reference)
