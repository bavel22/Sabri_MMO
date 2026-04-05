# Hunter Class — Complete Implementation Research

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Hunter_Skills_Audit](Hunter_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE -- Ready for implementation
**Class Progression:** Novice -> Archer -> **Hunter** -> Sniper (transcendent)
**Base Research:** `docsNew/05_Development/Hunter_Bard_Dancer_Skills_Research.md` (raw skill data)
**Parent Audit:** `docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md` (Archer skill gaps)
**Sources:** iRO Wiki, iRO Wiki Classic, rAthena pre-renewal DB (pre.pservero.com), RateMyServer, rAthena GitHub, GameFAQs trap guides

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Complete Skill List (18 skills)](#2-complete-skill-list)
3. [Skill Tree & Prerequisites](#3-skill-tree--prerequisites)
4. [Damage Formulas (All Skills)](#4-damage-formulas)
5. [Trap System Architecture](#5-trap-system-architecture)
6. [Falcon System Architecture](#6-falcon-system-architecture)
7. [Passive Skills & getPassiveSkillBonuses](#7-passive-skills--getpassiveskillbonuses)
8. [Existing Skill Data Audit](#8-existing-skill-data-audit)
9. [New Systems Required](#9-new-systems-required)
10. [Implementation Priority](#10-implementation-priority)
11. [Integration Points](#11-integration-points)
12. [Constants & Data Tables](#12-constants--data-tables)

---

## 1. Class Overview

### Hunter Identity

The Hunter is Archer's 2nd class specialization, defined by two unique mechanics that no other class possesses:

1. **Trap System** -- 12 trap skills that place ground objects which trigger when enemies step on them. Traps deal MISC-type damage (ignores DEF/MDEF, bypasses FLEE, unaffected by cards), inflict status effects (stun, freeze, blind, sleep), drain SP, or control positioning (knockback, immobilize).

2. **Falcon System** -- A companion falcon that enables Blitz Beat (manual and auto-proc), Detect (reveal hidden), and Spring Trap (remote trap removal). Falcon damage is MISC-type and scales with INT/DEX/Steel Crow, not ATK.

### Inherited Skills (from Archer)

Hunters inherit and can continue to use all 7 Archer skills:
- Owl's Eye (300), Vulture's Eye (301), Improve Concentration (302)
- Double Strafe (303), Arrow Shower (304)
- Arrow Crafting (305), Arrow Repel (306) -- quest skills

### Hunter Skill Count

| Category | Count | Skills |
|----------|-------|--------|
| Damage Traps | 4 | Land Mine, Blast Mine, Claymore Trap, Freezing Trap |
| Status Traps | 3 | Ankle Snare, Sandman, Flasher |
| Utility Traps | 3 | Skid Trap, Shockwave Trap, Talkie Box |
| Trap Management | 2 | Remove Trap, Spring Trap |
| Falcon Skills | 2 | Blitz Beat, Detect |
| Passives | 3 | Beast Bane, Falconry Mastery, Steel Crow |
| Quest Skill | 1 | Phantasmic Arrow |
| **Total** | **18** | |

### Unique Mechanics Summary

| Mechanic | Details |
|----------|---------|
| MISC Damage | Traps and Blitz Beat ignore DEF, MDEF, FLEE, cards, size penalty |
| Trap Items | Each trap placement consumes 1-2 Trap items (Item ID 1065, 100z from NPC) |
| Trap Ground State | Traps are ground objects that persist until triggered or expire |
| Falcon Companion | Rented from Hunter Guild (100z), enables falcon skills, exclusive with Cart |
| Auto-Blitz Beat | LUK/3 % chance to trigger on every normal attack (no SP cost) |

---

## 2. Complete Skill List

### 2.1 Trap Skills (12 skills)

---

#### Skid Trap (ID: 908, rA: 115)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Utility) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | None |

| Level | Knockback (cells) | Trap Lifetime (sec) |
|-------|--------------------|---------------------|
| 1 | 6 | 300 |
| 2 | 7 | 240 |
| 3 | 8 | 180 |
| 4 | 9 | 120 |
| 5 | 10 | 60 |

**Formulas:**
- Knockback = 5 + SkillLv cells
- Trap Lifetime = (6 - SkillLv) * 60 seconds
- Damage: 0 (no damage)

**Special Mechanics:**
- Knockback direction: pushes in direction the trap was facing when placed
- Does NOT deal damage
- Trap becomes visible after placement
- Boss monsters immune to knockback effect (still triggers trap)

---

#### Land Mine (ID: 904, rA: 116)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Offensive) |
| Target | Ground |
| Element | Earth |
| Range | 3 cells (150 UE units) |
| AoE | 1x1 (triggering cell only) |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | None |

| Level | Damage Formula | Stun Chance | Stun Duration | Trap Lifetime (sec) |
|-------|----------------|-------------|---------------|---------------------|
| 1 | (DEX+75) * (1+INT/100) * 1 | 35% | 5s | 200 |
| 2 | (DEX+75) * (1+INT/100) * 2 | 40% | 5s | 160 |
| 3 | (DEX+75) * (1+INT/100) * 3 | 45% | 5s | 120 |
| 4 | (DEX+75) * (1+INT/100) * 4 | 50% | 5s | 80 |
| 5 | (DEX+75) * (1+INT/100) * 5 | 55% | 5s | 40 |

**Formulas:**
- Damage = SkillLv * (DEX + 75) * (1 + INT / 100)
- Stun Chance = 30 + 5 * SkillLv (%)
- Damage is **MISC type** -- ignores DEF, MDEF, FLEE, cards, size penalty
- Affected by Earth element modifier on target (ELEMENT_TABLE lookup)
- Boss monsters immune to stun (damage still applies)

---

#### Ankle Snare (ID: 903, rA: 117)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Control) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| AoE | 1x1 (triggering cell only) |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | Skid Trap Lv 1 |

| Level | Snare Duration (sec) | Trap Lifetime (sec) |
|-------|----------------------|---------------------|
| 1 | 4 | 250 |
| 2 | 8 | 200 |
| 3 | 12 | 150 |
| 4 | 16 | 100 |
| 5 | 20 | 50 |

**Formulas:**
- Base snare duration = 4 * SkillLv seconds
- Player snare duration = 5 * SkillLv / (TargetAGI * 0.1) seconds, minimum 3 seconds
- Boss monster snare duration = SkillLv * 3 seconds (shorter, capped)
- Trap Lifetime = (6 - SkillLv) * 50 seconds
- Damage: 0 (no damage)

**Special Mechanics:**
- Target cannot move but CAN still attack and use skills
- Trap becomes visible when triggered
- Breaking free: duration ends naturally (no stat-based early break)
- Trapped enemies are valid targets for auto-attack and skills

---

#### Shockwave Trap (ID: 906, rA: 118)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Debuff) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| AoE | 3x3 cells |
| SP Cost | 45 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 2 Traps |
| Prerequisites | Ankle Snare Lv 1 |

| Level | SP Drain % | Trap Lifetime (sec) |
|-------|-----------|---------------------|
| 1 | 20% | 200 |
| 2 | 35% | 160 |
| 3 | 50% | 120 |
| 4 | 65% | 80 |
| 5 | 80% | 40 |

**Formulas:**
- SP drained = floor(TargetCurrentSP * DrainPercent / 100)
- Drain percent = 5 + 15 * SkillLv
- Damage: 0 (drains SP only)

**Special Mechanics:**
- Does NOT affect Boss monsters (no SP drain)
- Does NOT deal HP damage
- Only affects targets with SP (monsters with 0 SP are unaffected)
- Hits only the target that activated the trap (not the full 3x3)
- Consumes 2 Trap items per placement

---

#### Sandman (ID: 909, rA: 119)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Status) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| AoE | 5x5 cells |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | Flasher Lv 1 |

| Level | Sleep Chance | Sleep Duration (sec) | Trap Lifetime (sec) |
|-------|-------------|----------------------|---------------------|
| 1 | 50% | 30 | 150 |
| 2 | 60% | 30 | 120 |
| 3 | 70% | 30 | 90 |
| 4 | 80% | 30 | 60 |
| 5 | 90% | 30 | 30 |

**Formulas:**
- Sleep chance = 40 + 10 * SkillLv (%)
- Sleep duration = 30 seconds (fixed, all levels)
- Damage: 0 (no damage)

**Special Mechanics:**
- Sleep is breakable by damage
- Boss monsters immune to sleep (still triggers trap)
- All targets in 5x5 AoE are checked independently

---

#### Flasher (ID: 910, rA: 120)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Status) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| AoE | 3x3 cells |
| SP Cost | 12 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 2 Traps |
| Prerequisites | Skid Trap Lv 1 |

| Level | Blind Duration (sec) | Trap Lifetime (sec) |
|-------|---------------------|---------------------|
| 1 | 30 | 150 |
| 2 | 30 | 120 |
| 3 | 30 | 90 |
| 4 | 30 | 60 |
| 5 | 30 | 30 |

**Formulas:**
- Blind duration = 30 seconds (fixed, all levels)
- Level only affects trap lifetime (not blind duration)
- Damage: 0 (no damage)

**Special Mechanics:**
- Blind reduces HIT by 25% and FLEE by 25%
- Boss monsters immune to blind
- Hits only the target that activated the trap
- Consumes 2 Trap items

---

#### Freezing Trap (ID: 911, rA: 121)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Offensive + Status) |
| Target | Ground |
| Element | Water |
| Range | 3 cells (150 UE units) |
| AoE | 3x3 cells |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 2 Traps |
| Prerequisites | Flasher Lv 1 |

| Level | Damage Formula | Freeze Duration (sec) | Trap Lifetime (sec) |
|-------|----------------|----------------------|---------------------|
| 1 | (DEX+75) * (1+INT/100) * 1 | 3 | 150 |
| 2 | (DEX+75) * (1+INT/100) * 2 | 6 | 120 |
| 3 | (DEX+75) * (1+INT/100) * 3 | 9 | 90 |
| 4 | (DEX+75) * (1+INT/100) * 4 | 12 | 60 |
| 5 | (DEX+75) * (1+INT/100) * 5 | 15 | 30 |

**Formulas:**
- Damage = SkillLv * (DEX + 75) * (1 + INT / 100) -- same base as Land Mine
- Freeze duration = 3 * SkillLv seconds
- Damage is MISC type (ignores DEF/MDEF)
- Damage IS affected by Water element modifier (ELEMENT_TABLE lookup)

**Special Mechanics:**
- Frozen targets become Water Lv1 element (vulnerable to Wind)
- Boss monsters immune to freeze (damage still applies)
- Freeze is breakable by damage
- Consumes 2 Trap items

---

#### Blast Mine (ID: 912, rA: 122)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Offensive) |
| Target | Ground |
| Element | Wind |
| Range | 3 cells (150 UE units) |
| AoE | 3x3 cells |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | Land Mine Lv 1, Sandman Lv 1, Freezing Trap Lv 1 |

| Level | Damage Formula | Auto-Detonate Timer (sec) |
|-------|----------------|---------------------------|
| 1 | (50+DEX/2) * (1+INT/100) * 1 | 25 |
| 2 | (50+DEX/2) * (1+INT/100) * 2 | 20 |
| 3 | (50+DEX/2) * (1+INT/100) * 3 | 15 |
| 4 | (50+DEX/2) * (1+INT/100) * 4 | 10 |
| 5 | (50+DEX/2) * (1+INT/100) * 5 | 5 |

**Formulas:**
- Damage = SkillLv * (50 + floor(DEX / 2)) * (1 + INT / 100)
- Auto-detonate timer = 30 - 5 * SkillLv (seconds)
- Damage is MISC type
- Wind element modifier applies

**Special Mechanics:**
- AUTO-DETONATES after timer expires (even with no target)
- Can also trigger by enemy stepping on it before timer
- Auto-detonation damages all enemies in 3x3 AoE
- Timer starts when trap is placed

---

#### Claymore Trap (ID: 907, rA: 123)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Trap, Offensive) |
| Target | Ground |
| Element | Fire |
| Range | 3 cells (150 UE units) |
| AoE | 5x5 cells |
| SP Cost | 15 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 2 Traps |
| Prerequisites | Shockwave Trap Lv 1, Blast Mine Lv 1 |

| Level | Damage Formula | Trap Lifetime (sec) |
|-------|----------------|---------------------|
| 1 | (75+DEX/2) * (1+INT/100) * 1 | 20 |
| 2 | (75+DEX/2) * (1+INT/100) * 2 | 40 |
| 3 | (75+DEX/2) * (1+INT/100) * 3 | 60 |
| 4 | (75+DEX/2) * (1+INT/100) * 4 | 80 |
| 5 | (75+DEX/2) * (1+INT/100) * 5 | 100 |

**Formulas:**
- Damage = SkillLv * (75 + floor(DEX / 2)) * (1 + INT / 100)
- Trap Lifetime = 20 * SkillLv seconds (INCREASES with level, opposite of most traps)
- Damage is MISC type
- Fire element modifier applies

**Special Mechanics:**
- Largest AoE trap (5x5)
- Highest trap damage
- Duration increases with level (unusual)
- Consumes 2 Trap items
- Ignores DEF, FLEE, cards, size penalties
- Bypasses Guard, Parry, and Weapon Blocking

---

#### Remove Trap (ID: 905, rA: 124)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| Type | Active (Utility) |
| Target | Trap on ground |
| Element | Neutral |
| Range | 2 cells (100 UE units) |
| SP Cost | 5 |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | None |
| Prerequisites | Land Mine Lv 1 |

**Special Mechanics:**
- Can only remove YOUR OWN traps
- Returns 1 Trap item to inventory
- Cannot remove traps that are already triggered/activated
- Short range (must stand adjacent)

---

#### Talkie Box (ID: 914, rA: 125)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| Type | Active (Trap, Cosmetic) |
| Target | Ground |
| Element | Neutral |
| Range | 3 cells (150 UE units) |
| SP Cost | 1 |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | 1 Trap |
| Prerequisites | Shockwave Trap Lv 1, Remove Trap Lv 1 |

**Special Mechanics:**
- Player inputs a text message at cast time
- Trap displays the message when any player steps on it
- Trap lasts 600 seconds (10 minutes)
- Purely cosmetic -- no damage, no status effects
- Implementation note: requires text input UI on client

---

#### Spring Trap (ID: 913, rA: 131)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Utility) |
| Target | Trap on ground |
| Element | Neutral |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | None |
| Prerequisites | Remove Trap Lv 1, Falconry Mastery Lv 1 |
| Requires | Falcon equipped |

| Level | Range (cells) |
|-------|---------------|
| 1 | 4 (200 UE) |
| 2 | 5 (250 UE) |
| 3 | 6 (300 UE) |
| 4 | 7 (350 UE) |
| 5 | 8 (400 UE) |

**Special Mechanics:**
- Can target ANY visible trap (enemy or own)
- Does NOT return Trap item (unlike Remove Trap)
- Requires falcon companion
- Range increases with level
- Useful for WoE/PvP counter-trapping

---

### 2.2 Falcon Skills (2 skills)

---

#### Blitz Beat (ID: 900, rA: 129)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Type | Active (Offensive, MISC) |
| Target | Single (3x3 splash) |
| Element | **Neutral** (always) |
| Range | 5 + Vulture's Eye level (cells) |
| Cast Time | 1500 ms (1200 variable + 300 fixed in some sources; pre-renewal may be all-variable) |
| After-Cast Delay | 1000 ms |
| Cooldown | 0 |
| Required Item | None |
| Prerequisites | Falconry Mastery Lv 1 |
| Requires | Falcon equipped |

| Level | Hits | SP Cost |
|-------|------|---------|
| 1 | 1 | 10 |
| 2 | 2 | 13 |
| 3 | 3 | 16 |
| 4 | 4 | 19 |
| 5 | 5 | 22 |

**SP Cost Formula:** 7 + 3 * SkillLv

**Damage Formula (Pre-Renewal):**
```
Per-hit damage = floor(DEX/10) * 2 + floor(INT/2) * 2 + SteelCrowLv * 6 + 80
Total damage (manual) = Per-hit * NumberOfHits
```

Simplified:
```
PerHit = 80 + SteelCrowBonus + INT + floor(DEX/5)
       where SteelCrowBonus = SteelCrowLv * 6
TotalDamage = PerHit * Hits
```

**MISC Damage Properties:**
- Ignores DEF and MDEF completely
- Ignores FLEE (always hits, cannot miss)
- Ignores card race/element/size modifiers (NOT affected by Hydra, Vadon, etc.)
- Ignores size penalty
- Always Neutral element (element table still applies to target's element)
- NOT affected by ATK or MATK
- NOT affected by weapon element or endow skills

**Manual Cast vs Auto-Blitz:**

| Property | Manual Blitz Beat | Auto-Blitz Beat |
|----------|-------------------|-----------------|
| SP Cost | 10-22 | 0 (free) |
| Hits | 1-5 (skill level) | 1-5 (job level formula) |
| Targeting | Player selects target | Triggers on current auto-attack target |
| 3x3 Damage | Full damage to all in AoE | Damage SPLIT among targets in AoE |
| Cast Time | Yes (1500ms base) | None (instant) |
| After-Cast Delay | Yes (1000ms) | None |
| Trigger | Player activates | LUK/3 % on every normal attack |

**Auto-Blitz Beat Details:**
- Trigger chance: floor(LUK / 3) % on every normal bow attack
- Hit count formula: min(BlitzBeatLv, floor((JobLevel + 9) / 10))
- Auto-Blitz can trigger even on MISSED attacks
- Auto-Blitz triggers DURING after-cast delay (independent of skill cooldowns)
- Damage is SPLIT among all targets in 3x3: each target gets TotalDamage / NumTargets

**Auto-Blitz Hit Count by Job Level:**

| Job Level | Auto-Blitz Hits |
|-----------|-----------------|
| 1-9 | 1 |
| 10-19 | 2 |
| 20-29 | 3 |
| 30-39 | 4 |
| 40-50 | 5 |

Capped at learned Blitz Beat level (e.g., Blitz Beat Lv3 caps at 3 hits regardless of job level).

---

#### Detect (ID: 902, rA: 130)

| Property | Value |
|----------|-------|
| Max Level | 4 |
| Type | Active (Utility) |
| Target | Ground |
| Element | Neutral |
| AoE | 7x7 cells centered on target cell |
| SP Cost | 8 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | None |
| Prerequisites | Improve Concentration Lv 1, Falconry Mastery Lv 1 |
| Requires | Falcon equipped |

| Level | Range (cells) |
|-------|---------------|
| 1 | 3 (150 UE) |
| 2 | 5 (250 UE) |
| 3 | 7 (350 UE) |
| 4 | 9 (450 UE) |

**Special Mechanics:**
- Reveals Hiding, Cloaking, and Chase Walk
- Also reveals invisible enemy traps
- Detection area is always 7x7 cells centered on the targeted ground position
- Range to target position increases with level
- One-time reveal (not persistent)

---

### 2.3 Passive Skills (3 skills)

---

#### Beast Bane (ID: 915, rA: 126)

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Type | Passive |
| Prerequisites | None |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|-----|
| ATK Bonus | +4 | +8 | +12 | +16 | +20 | +24 | +28 | +32 | +36 | +40 |

**Formula:** +4 * SkillLv flat ATK vs Brute and Insect race monsters

**Implementation:** This is a race-based passive ATK bonus, identical in pattern to Demon Bane (Acolyte).
- Add to `getPassiveSkillBonuses()`: `raceATK.brute += lv * 4; raceATK.insect += lv * 4`
- Applied at step 11b of the physical damage pipeline (after card mods, before element modifier)
- iRO Wiki Classic states this bonus "ignores HardDef and Soft Def" -- meaning it is flat ATK added AFTER all DEF reductions, just like Demon Bane

---

#### Falconry Mastery (ID: 916, rA: 127)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| Type | Passive |
| Prerequisites | Beast Bane Lv 1 |

**Implementation:** This enables the falcon companion. The server tracks a boolean `player.hasFalcon` (or `player.falconEquipped`).

**Falcon Rules:**
- Rented from Hunter Guild NPC for 100 Zeny
- Cannot have falcon and Pushcart at the same time
- Required for: Blitz Beat, Detect, Spring Trap, Auto-Blitz Beat
- Falconry Mastery Lv1 is the gate -- if the player has this skill and rents a falcon, `player.hasFalcon = true`

---

#### Steel Crow (ID: 901, rA: 128)

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Type | Passive |
| Prerequisites | Blitz Beat Lv 5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|-----|
| Falcon ATK | +6 | +12 | +18 | +24 | +30 | +36 | +42 | +48 | +54 | +60 |

**Formula:** +6 * SkillLv per Blitz Beat hit

**Implementation:** Read in the Blitz Beat damage calculation:
```js
const steelCrowLv = learned[901] || 0;
const steelCrowBonus = steelCrowLv * 6;
const perHitDamage = 80 + steelCrowBonus + Math.floor(INT / 2) * 2 + Math.floor(DEX / 10) * 2;
```

---

### 2.4 Quest Skill (1 skill)

---

#### Phantasmic Arrow (ID: 917, rA: 1009)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| Type | Active (Offensive, Physical) |
| Target | Single |
| Element | Neutral (or arrow element if equipped) |
| Range | 9 cells (450 UE units) |
| SP Cost | 10 |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 |
| Required Item | None (does NOT consume arrows) |
| Prerequisites | Quest completion (Hunter Platinum Skill Quest, Job Lv 40+) |
| Requires | Bow equipped |

**Damage:** 150% ATK (standard physical damage pipeline)
**Knockback:** 3 cells away from caster

**Special Mechanics:**
- Does NOT consume arrows (creates a phantom arrow)
- Uses arrow element if arrows are equipped, otherwise Neutral
- Standard physical skill (affected by DEF, cards, size penalty, etc.)
- Boss monsters immune to knockback (damage still applies)

---

### 2.5 Soul Link Skill (deferred)

#### Beast Strafing / Beast Charge (rA: 499)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 12 |
| Damage | (50 + STR * 8)% ATK |
| Prerequisites | Double Strafe Lv 10, Beast Bane Lv 10, active Hunter Spirit buff |
| Target Restriction | Brute or Insect race ONLY |
| Must use | Immediately after Double Strafe |

**Status: DEFERRED** -- Requires Soul Linker class to cast Hunter Spirit. Not part of base Hunter implementation.

---

## 3. Skill Tree & Prerequisites

### Visual Skill Tree Layout

```
Row 0:  [Blitz Beat]--->[Falconry Mastery]   [Detect]   [Beast Bane]
             |                   ^               ^
             v                   |               |
Row 1:  [Steel Crow]    [Land Mine]<--+   [Shockwave Trap]
                              |       |         |
                              v       |         v
Row 2:  [Blast Mine]<--+  [Remove Trap]   [Claymore Trap]
                        |      |
                        |      v
Row 3:  [Skid Trap]  [Sandman]  [Flasher]--->[Freezing Trap]

Row 4:  [Phantasmic Arrow*]  [Spring Trap]  [Talkie Box]
```

### Complete Prerequisite Chain (Project IDs)

| Skill (Project ID) | Prerequisites |
|---------------------|---------------|
| Blitz Beat (900) | Falconry Mastery (916) Lv 1 |
| Steel Crow (901) | Blitz Beat (900) Lv 5 |
| Detect (902) | Improve Concentration (302) Lv 1, Falconry Mastery (916) Lv 1 |
| Ankle Snare (903) | Skid Trap (908) Lv 1 |
| Land Mine (904) | None |
| Remove Trap (905) | Land Mine (904) Lv 1 |
| Shockwave Trap (906) | Ankle Snare (903) Lv 1 |
| Claymore Trap (907) | Shockwave Trap (906) Lv 1, Blast Mine (912) Lv 1 |
| Skid Trap (908) | None |
| Sandman (909) | Flasher (910) Lv 1 |
| Flasher (910) | Skid Trap (908) Lv 1 |
| Freezing Trap (911) | Flasher (910) Lv 1 |
| Blast Mine (912) | Land Mine (904) Lv 1, Sandman (909) Lv 1, Freezing Trap (911) Lv 1 |
| Spring Trap (913) | Remove Trap (905) Lv 1, Falconry Mastery (916) Lv 1 |
| Talkie Box (914) | Shockwave Trap (906) Lv 1, Remove Trap (905) Lv 1 |
| Beast Bane (915) | None |
| Falconry Mastery (916) | Beast Bane (915) Lv 1 |
| Phantasmic Arrow (917) | Quest completion (questSkill: true) |

### Prerequisite Chains to Reach Key Skills

**To get Claymore Trap (907):**
```
Skid Trap 1 -> Ankle Snare 1 -> Shockwave Trap 1 -> Claymore Trap
  and
Skid Trap 1 -> Flasher 1 -> Freezing Trap 1 -> (for Blast Mine)
  and
Land Mine 1 -> (for Blast Mine)
  and
Flasher 1 -> Sandman 1 -> (for Blast Mine)
  and
Blast Mine 1 -> Claymore Trap
  TOTAL: Skid 1 + Flasher 1 + Sandman 1 + Freezing 1 + Land Mine 1 + Ankle 1 + Shockwave 1 + Blast 1 = 8 skill points minimum
```

**To get Blitz Beat + Steel Crow 10:**
```
Beast Bane 1 -> Falconry Mastery 1 -> Blitz Beat 5 -> Steel Crow 10
  TOTAL: 17 skill points
```

---

## 4. Damage Formulas

### 4.1 MISC Damage Type (Traps + Falcon)

MISC damage is a third damage category alongside Physical and Magical. It has the following properties:

| Property | MISC | Physical | Magical |
|----------|------|----------|---------|
| Ignores DEF | YES | No | N/A |
| Ignores MDEF | YES | N/A | No |
| Ignores FLEE | YES | No | Yes |
| Ignores Cards | YES | No | No |
| Ignores Size Penalty | YES | No | Yes |
| Element Table | YES | YES | YES |
| Critical | No | Yes | No |
| HIT/FLEE Check | No | Yes | No |
| Can Miss | No | Yes | No |

**Implementation Pattern:**
```js
function calculateMiscDamage(baseDamage, skillElement, targetElement, targetElementLevel) {
    // MISC damage: apply element modifier only, nothing else
    const eleMod = getElementModifier(skillElement, targetElement, targetElementLevel);
    if (eleMod <= 0) return { damage: 0, hitType: 'elementImmune', element: skillElement };

    // Apply +/- 10% variance (standard for traps)
    const variance = 0.9 + Math.random() * 0.2; // 0.9 to 1.1
    const finalDamage = Math.max(1, Math.floor(baseDamage * eleMod / 100 * variance));

    return { damage: finalDamage, hitType: 'misc', isMiss: false, element: skillElement };
}
```

### 4.2 Trap Damage Formulas (Consolidated)

All trap damage skills use this general formula:
```
BaseDamage = SkillLv * (BaseValue + floor(DEX / 2)) * (1 + INT / 100)
FinalDamage = floor(BaseDamage * ElementModifier / 100) +/- 10%
```

| Trap | BaseValue | Element | AoE |
|------|-----------|---------|-----|
| Land Mine | DEX + 75 (note: full DEX, not DEX/2) | Earth | 1x1 |
| Blast Mine | 50 + floor(DEX/2) | Wind | 3x3 |
| Claymore Trap | 75 + floor(DEX/2) | Fire | 5x5 |
| Freezing Trap | DEX + 75 (same as Land Mine) | Water | 3x3 |

**Example calculation (DEX=90, INT=50, SkillLv=5):**
- Land Mine: 5 * (90+75) * (1+50/100) = 5 * 165 * 1.5 = **1,237 base damage**
- Blast Mine: 5 * (50+45) * (1+50/100) = 5 * 95 * 1.5 = **712 base damage**
- Claymore: 5 * (75+45) * (1+50/100) = 5 * 120 * 1.5 = **900 base damage**
- Freezing: 5 * (90+75) * (1+50/100) = same as Land Mine = **1,237 base damage**

### 4.3 Blitz Beat Damage Formula

```
PerHitDamage = 80 + (SteelCrowLv * 6) + (floor(INT/2) * 2) + (floor(DEX/10) * 2)
TotalDamage = PerHitDamage * NumberOfHits
```

Simplified (equivalent):
```
PerHitDamage = 80 + SteelCrowBonus + INT + floor(DEX/5)
```

**Example (INT=70, DEX=99, Steel Crow 10):**
```
PerHit = 80 + 60 + 70 + floor(99/5) = 80 + 60 + 70 + 19 = 229
Blitz Beat Lv5 (5 hits) = 229 * 5 = 1,145 total damage
```

Auto-Blitz with multiple targets: damage is split.
- 3 targets in 3x3: each takes 1,145 / 3 = 381 damage

### 4.4 Phantasmic Arrow Damage

Standard physical damage pipeline at 150% ATK multiplier:
```js
const result = calculatePhysicalDamage(attackerStats, targetStats, targetHardDef, 150, ...);
```
Not MISC -- affected by DEF, cards, size penalty, FLEE, etc.

---

## 5. Trap System Architecture

### 5.1 Server-Side Trap State

The trap system requires a new ground entity registry, similar to how enemies and players are tracked.

```js
// New Map in index.js
const activeTraps = new Map(); // trapId -> TrapEntry

// TrapEntry structure
{
    trapId: Number,           // Unique auto-increment ID
    ownerId: Number,          // Character ID of the hunter who placed it
    ownerName: String,        // Character name
    skillId: Number,          // Which trap skill (904, 907, etc.)
    skillName: String,        // 'land_mine', 'claymore_trap', etc.
    skillLevel: Number,       // Learned level when placed
    x: Number,                // Ground position X (UE units)
    y: Number,                // Ground position Y (UE units)
    z: Number,                // Ground position Z (UE units)
    zone: String,             // Zone identifier
    element: String,          // 'earth', 'fire', 'wind', 'water', 'neutral'
    placedAt: Number,         // Date.now() when placed
    lifetime: Number,         // Milliseconds until auto-expire
    expiresAt: Number,        // placedAt + lifetime
    triggerRadius: Number,    // UE units -- how close an entity must be to trigger
    aoeRadius: Number,        // UE units -- damage/effect AoE radius
    isTriggered: Boolean,     // Has it been stepped on?
    autoDetonateAt: Number,   // For Blast Mine: time when it auto-detonates (0 = never)
    trapItemCost: Number,     // 1 or 2 Trap items consumed
    // Cached damage parameters (from owner stats at placement time)
    ownerDEX: Number,
    ownerINT: Number,
    ownerBaseLv: Number,
    steelCrowLv: Number,
}
```

### 5.2 Trap Placement Flow

```
1. Client sends skill:use { skillId, groundX, groundY, groundZ }
2. Server validates:
   a. Player is hunter class (or sniper)
   b. Player has learned the trap skill
   c. Player has Trap items in inventory (1 or 2 depending on skill)
   d. Ground position is within range (3 cells = 150 UE)
   e. No existing trap at the exact position (2-cell minimum spacing)
   f. Not in after-cast delay or on cooldown
3. Server deducts Trap item(s) from inventory
4. Server deducts SP
5. Server creates TrapEntry in activeTraps
6. Server broadcasts trap:placed to zone
   { trapId, ownerId, skillId, skillName, x, y, z, element }
7. Client spawns trap visual at position
```

### 5.3 Trap Trigger Flow

```
On every enemy position update (enemy AI 200ms tick) and player position update:
  For each active trap in the same zone:
    If entity position within triggerRadius of trap position:
      If trap.isTriggered === false:
        Mark trap as triggered
        Execute trap effect (damage, status, knockback, etc.)
        Broadcast trap:triggered to zone
        Remove trap from activeTraps
```

**Trigger radius:** 1 cell = 50 UE units. Traps trigger when center-to-center distance is within 50 UE units (1 cell).

### 5.4 Trap Expiry / Auto-Detonate

A 1-second interval checks all active traps:
```js
setInterval(() => {
    const now = Date.now();
    for (const [trapId, trap] of activeTraps.entries()) {
        // Auto-detonate (Blast Mine)
        if (trap.autoDetonateAt > 0 && now >= trap.autoDetonateAt) {
            executeTrapDetonation(trap);
            activeTraps.delete(trapId);
            broadcastToZone(trap.zone, 'trap:detonated', { trapId, x: trap.x, y: trap.y, z: trap.z });
            continue;
        }
        // Natural expiry
        if (now >= trap.expiresAt) {
            activeTraps.delete(trapId);
            broadcastToZone(trap.zone, 'trap:expired', { trapId });
            // Return Trap item if expired naturally (rAthena behavior)
        }
    }
}, 1000);
```

### 5.5 Trap Item Consumption

| Trap Skill | Trap Items Consumed |
|-----------|---------------------|
| Skid Trap | 1 |
| Land Mine | 1 |
| Ankle Snare | 1 |
| Sandman | 1 |
| Blast Mine | 1 |
| Phantasmic Arrow | 0 |
| Flasher | 2 |
| Freezing Trap | 2 |
| Shockwave Trap | 2 |
| Claymore Trap | 2 |
| Remove Trap | 0 (recovers 1) |
| Spring Trap | 0 |
| Talkie Box | 1 |

**Trap item:** Item ID 1065, weight 10, buy price 100z from NPC. Must be in player inventory.

### 5.6 Trap Placement Rules

1. **Maximum active traps:** In pre-renewal, a Hunter can have **3 traps** placed at a time (some sources say unlimited, but rAthena defaults to 3 via `MAX_SUMMONS`). If a 4th trap is placed, the oldest trap is destroyed.
2. **Minimum spacing:** Cannot place a trap within 2 cells of another trap, player, or monster.
3. **Trap visibility:** All traps are visible to the caster and party members. Enemy players see traps only after they trigger.
4. **Zone transitions:** All traps are destroyed when the owner changes zones.

### 5.7 Socket.io Events (Traps)

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `trap:placed` | Server -> Zone | `{ trapId, ownerId, skillId, skillName, x, y, z, element, lifetime }` | Trap spawned on ground |
| `trap:triggered` | Server -> Zone | `{ trapId, skillName, x, y, z, triggeredById, triggeredByName }` | Trap activated |
| `trap:detonated` | Server -> Zone | `{ trapId, skillName, x, y, z }` | Blast Mine auto-exploded |
| `trap:expired` | Server -> Zone | `{ trapId }` | Trap timed out |
| `trap:removed` | Server -> Zone | `{ trapId, removedById }` | Remove Trap / Spring Trap |

Trap damage is broadcast via the standard `skill:effect_damage` event with `hitType: 'misc'`.

---

## 6. Falcon System Architecture

### 6.1 Falcon State

```js
// On player object
player.hasFalcon = false; // Set true when falcon rented from Hunter Guild NPC

// Checks required before falcon skills
function requiresFalcon(player) {
    return player.hasFalcon === true;
}
```

**Falcon rental:**
- NPC in Hunter Guild area
- Costs 100 Zeny
- Requires Falconry Mastery Lv 1
- Falcon is lost on death in some versions (optional for our implementation)
- Cannot have falcon and pushcart simultaneously

### 6.2 Auto-Blitz Beat Integration

Auto-Blitz Beat triggers in the auto-attack combat tick. This is the most complex part of the falcon system.

```js
// In processEnemyAutoAttack() or processPlayerAutoAttack(), AFTER the normal attack damage:

// Auto-Blitz Beat check (Hunter/Sniper with falcon + bow)
if (player.hasFalcon && attacker.weaponType === 'bow') {
    const blitzLv = (attacker.learnedSkills || {})[900] || 0; // Blitz Beat skill level
    if (blitzLv > 0) {
        const luk = getEffectiveStats(attacker).luk;
        const blitzChance = Math.floor(luk / 3); // LUK/3 %

        if (Math.random() * 100 < blitzChance) {
            // Determine number of hits from job level
            const jobLvHits = Math.floor((attacker.jobLevel + 9) / 10);
            const autoBlitzHits = Math.min(blitzLv, jobLvHits);

            // Calculate per-hit damage
            const steelCrowLv = (attacker.learnedSkills || {})[901] || 0;
            const stats = getEffectiveStats(attacker);
            const perHitDmg = 80 + (steelCrowLv * 6) + Math.floor(stats.int / 2) * 2 + Math.floor(stats.dex / 10) * 2;

            // Find targets in 3x3 (125 UE radius)
            const targets = [];
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead || enemy.zone !== attacker.zone) continue;
                const dist = calculateDistance(target, enemy); // target is the auto-attack target
                if (dist <= 125) targets.push(enemy);
            }

            // Auto-blitz damage is SPLIT among targets
            const totalDamage = perHitDmg * autoBlitzHits;
            const perTargetDamage = Math.floor(totalDamage / targets.length);

            for (const t of targets) {
                // Apply element modifier
                const eleMod = getElementModifier('neutral', t.element?.type || 'neutral', t.element?.level || 1);
                const finalDmg = Math.max(1, Math.floor(perTargetDamage * eleMod / 100));

                t.health = Math.max(0, t.health - finalDmg);
                broadcastToZone(zone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: attacker.characterName,
                    targetId: eid, targetName: t.name, isEnemy: true,
                    skillId: 900, skillName: 'Blitz Beat', skillLevel: blitzLv,
                    element: 'neutral', damage: finalDmg, hits: autoBlitzHits,
                    hitType: 'misc', isCritical: false, isMiss: false,
                    targetX: t.x, targetY: t.y, targetZ: t.z,
                    targetHealth: t.health, targetMaxHealth: t.maxHealth,
                    timestamp: Date.now()
                });

                if (t.health <= 0) {
                    await processEnemyDeathFromSkill(t, attacker, characterId, io);
                }
            }
        }
    }
}
```

### 6.3 Manual Blitz Beat Handler

```js
if (skill.name === 'blitz_beat') {
    if (!player.hasFalcon) {
        socket.emit('skill:error', { message: 'Requires falcon companion' });
        return;
    }
    if (!targetId) {
        socket.emit('skill:error', { message: 'No target selected' });
        return;
    }

    // Resolve target
    // ... standard target resolution ...

    // Calculate damage
    const steelCrowLv = (player.learnedSkills || {})[901] || 0;
    const stats = getEffectiveStats(player);
    const perHitDmg = 80 + (steelCrowLv * 6) + Math.floor(stats.int / 2) * 2 + Math.floor(stats.dex / 10) * 2;
    const numHits = learnedLevel; // Skill level = number of hits
    const totalDamage = perHitDmg * numHits;

    // Element modifier (MISC damage, always neutral attack)
    const eleMod = getElementModifier('neutral', targetElement, targetElementLevel);
    const finalDmg = Math.max(1, Math.floor(totalDamage * eleMod / 100));

    // Apply damage
    player.mana = Math.max(0, player.mana - spCost);
    applySkillDelays(characterId, player, skillId, levelData, socket);

    target.health = Math.max(0, target.health - finalDmg);

    // Manual Blitz Beat: FULL damage to all in 3x3 (NOT split)
    // Find splash targets
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy === target || enemy.isDead || enemy.zone !== zone) continue;
        const dist = calculateDistance(target, enemy);
        if (dist <= 125) { // 3x3 = 125 UE radius
            const splashEleMod = getElementModifier('neutral', enemy.element?.type || 'neutral', enemy.element?.level || 1);
            const splashDmg = Math.max(1, Math.floor(totalDamage * splashEleMod / 100));
            enemy.health = Math.max(0, enemy.health - splashDmg);
            // Broadcast splash damage
        }
    }

    // ... broadcast, death check, skill:used ...
}
```

---

## 7. Passive Skills & getPassiveSkillBonuses

### Required Additions

The following must be added to `getPassiveSkillBonuses()` in `index.js`:

```js
// Beast Bane (915): +4 ATK/level vs Brute and Insect race
const bbLv = learned[915] || 0;
if (bbLv > 0) {
    bonuses.raceATK.brute = (bonuses.raceATK.brute || 0) + bbLv * 4;
    bonuses.raceATK.insect = (bonuses.raceATK.insect || 0) + bbLv * 4;
}
```

This uses the same `raceATK` object as Demon Bane. The physical damage pipeline step 11b already supports arbitrary race keys:
```js
// Step 11b in roPhysicalDamage():
if (attacker.passiveRaceATK && attacker.passiveRaceATK[target.race]) {
    totalATK += attacker.passiveRaceATK[target.race];
}
```

**Steel Crow (901)** and **Falconry Mastery (916)** are NOT added to `getPassiveSkillBonuses()`. They are read directly in the Blitz Beat damage calculation:
- Steel Crow: `(learned[901] || 0) * 6` in the Blitz Beat formula
- Falconry Mastery: checked as `learned[916] >= 1` for falcon prerequisite

**No bonuses struct changes needed.** The existing `raceATK` object handles Beast Bane. No new fields required.

---

## 8. Existing Skill Data Audit

The Hunter skills are already defined in `server/src/ro_skill_data_2nd.js` (IDs 900-917). Here is an audit of each definition against the canonical data.

### 8.1 Blitz Beat (900) -- ISSUES FOUND

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| maxLevel | 5 | 5 | CORRECT |
| SP formula | `10+i*3` (10,13,16,19,22) | 7+3*SkillLv (10,13,16,19,22) | CORRECT |
| castTime | 0 | 1500ms (1200 variable + 300 fixed) | **WRONG -- should have cast time** |
| cooldown | 1000 | 0 (afterCastDelay: 1000) | **WRONG -- should be ACD, not cooldown** |
| effectValue | `i+1` (1-5 = hit count) | Hits 1-5 | CORRECT (used as hit count) |
| prerequisites | Falconry Mastery (916) Lv 1 | Falconry Mastery Lv 1 | CORRECT |
| range | 800 | 5 + VultureEye cells | **NOTE: should be 250 + bonusRange** |

**Fixes needed:**
1. Add `castTime: 1500` (1.5 second base cast time, reduced by DEX)
2. Change `cooldown: 1000` to `afterCastDelay: 1000, cooldown: 0`
3. Consider adjusting range to 250 (5 cells base) + Vulture's Eye bonus applied in handler

### 8.2 Steel Crow (901) -- ISSUES FOUND

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| maxLevel | 10 | 10 | CORRECT |
| prerequisites | Blitz Beat (900) Lv 1 | Blitz Beat (900) **Lv 5** | **WRONG -- should require BB Lv 5** |
| effectValue | `(i+1)*6` | +6/lv | CORRECT |

**Fixes needed:**
1. Change prerequisite from `{ skillId: 900, level: 1 }` to `{ skillId: 900, level: 5 }`

### 8.3 Detect (902) -- ISSUES FOUND

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| maxLevel | 4 | 4 | CORRECT |
| SP Cost | `8+i*2` (8,10,12,14) | 8 (flat, all levels) | **WRONG -- SP should be 8 flat** |
| prerequisites | None | Improve Concentration Lv 1, Falconry Mastery Lv 1 | **WRONG -- missing prerequisites** |
| cooldown | 1000 | 0 | **WRONG -- no cooldown** |
| range | 600 | Level-dependent (3-9 cells = 150-450 UE) | **WRONG -- range should scale** |

**Fixes needed:**
1. Change SP to flat 8: `spCost: 8`
2. Add prerequisites: `[{ skillId: 302, level: 1 }, { skillId: 916, level: 1 }]`
3. Remove cooldown: `cooldown: 0`
4. Range should be level-dependent. Use `range: 150 + i * 100` (150, 250, 350, 450) or handle in handler with per-level range lookup.

### 8.4 Ankle Snare (903) -- MINOR ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Land Mine (904) Lv 1 | **Skid Trap** Lv 1 | **WRONG -- wrong prerequisite** |
| duration | `4000+i*4000` (4-20s) | Snare: 4*SkillLv sec, Trap Life: (6-Lv)*50 sec | PARTIALLY CORRECT (snare time OK) |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Change prerequisite from `{ skillId: 904, level: 1 }` to `{ skillId: 908, level: 1 }` (Skid Trap)
2. Remove cooldown: `cooldown: 0`

### 8.5 Land Mine (904) -- MINOR ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | None | None | CORRECT |
| effectValue | `50+i*50` | Damage formula based on DEX/INT | **WRONG INTERPRETATION -- effectValue not used for trap damage** |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Remove cooldown: `cooldown: 0`
2. effectValue interpretation: should be stored for reference but actual damage calculated via DEX/INT formula in handler

### 8.6 Remove Trap (905) -- OK

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| Everything | Matches | Matches | CORRECT |

### 8.7 Shockwave Trap (906) -- MINOR ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Ankle Snare (903) Lv 1 | Ankle Snare Lv 1 | CORRECT |
| effectValue | `20+i*15` (20,35,50,65,80) | 5+15*SkillLv (20,35,50,65,80) | CORRECT |
| cooldown | 1000 | 0 | **WRONG** |

**Fix:** Remove cooldown

### 8.8 Claymore Trap (907) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Shockwave (906) Lv 1, Land Mine (904) Lv 3 | Shockwave Lv 1, **Blast Mine (912)** Lv 1 | **WRONG prerequisite** |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Change prerequisite from `{ skillId: 904, level: 3 }` to `{ skillId: 912, level: 1 }` (Blast Mine)
2. Remove cooldown

### 8.9 Skid Trap (908) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Land Mine (904) Lv 1 | **None** | **WRONG -- Skid Trap has no prerequisites** |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Remove prerequisites: `prerequisites: []`
2. Remove cooldown

### 8.10 Sandman (909) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Ankle Snare (903) Lv 1 | **Flasher (910)** Lv 1 | **WRONG** |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Change prerequisite to `{ skillId: 910, level: 1 }` (Flasher)
2. Remove cooldown

### 8.11 Flasher (910) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Skid Trap (908) Lv 1 | Skid Trap Lv 1 | CORRECT |
| cooldown | 1000 | 0 | **WRONG** |

**Fix:** Remove cooldown

### 8.12 Freezing Trap (911) -- OK (minor cooldown)

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Flasher (910) Lv 1 | Flasher Lv 1 | CORRECT |
| cooldown | 1000 | 0 | **WRONG** |

**Fix:** Remove cooldown

### 8.13 Blast Mine (912) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Land Mine (904) Lv 5 | Land Mine Lv 1, Sandman Lv 1, Freezing Trap Lv 1 | **WRONG -- missing 2 prereqs, wrong level** |
| cooldown | 1000 | 0 | **WRONG** |

**Fixes needed:**
1. Change prerequisites to: `[{ skillId: 904, level: 1 }, { skillId: 909, level: 1 }, { skillId: 911, level: 1 }]`
2. Remove cooldown

### 8.14 Spring Trap (913) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Remove Trap (905) Lv 1 | Remove Trap Lv 1, **Falconry Mastery (916) Lv 1** | **MISSING prerequisite** |

**Fix:** Add prerequisite: `[{ skillId: 905, level: 1 }, { skillId: 916, level: 1 }]`

### 8.15 Talkie Box (914) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | Remove Trap (905) Lv 1 | Shockwave Trap (906) Lv 1, Remove Trap Lv 1 | **MISSING prerequisite** |

**Fix:** Add prerequisite: `[{ skillId: 906, level: 1 }, { skillId: 905, level: 1 }]`

### 8.16 Beast Bane (915) -- CORRECT

All fields match canonical data.

### 8.17 Falconry Mastery (916) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| prerequisites | None | **Beast Bane (915) Lv 1** | **MISSING prerequisite** |

**Fix:** Add prerequisite: `[{ skillId: 915, level: 1 }]`

### 8.18 Phantasmic Arrow (917) -- ISSUES

| Field | Current Value | Canonical Value | Status |
|-------|---------------|-----------------|--------|
| SP Cost | 10 | **10** (some sources say 10, RateMyServer says 10) | CORRECT |
| range | 800 | 9 cells = 450 UE | **WRONG -- too high** |

**Note:** The SP cost discrepancy between sources (10 vs some claiming 50) appears to be version-dependent. The rAthena pre-renewal database and RateMyServer both show 10 SP. We use 10.

**Fix:** Adjust range to 450 UE (9 cells)

### 8.19 Audit Summary

| Severity | Count | Skills |
|----------|-------|--------|
| Wrong prerequisites | 8 | Steel Crow, Detect, Ankle Snare, Claymore Trap, Skid Trap, Sandman, Blast Mine, Spring Trap, Talkie Box, Falconry Mastery |
| Wrong cooldown (should be 0) | 11 | All trap skills + Blitz Beat + Detect |
| Missing cast time | 1 | Blitz Beat (needs 1500ms) |
| Missing ACD | 1 | Blitz Beat (needs 1000ms) |
| SP formula wrong | 1 | Detect (should be flat 8) |
| Range wrong | 2 | Blitz Beat, Phantasmic Arrow |

---

## 9. New Systems Required

### 9.1 Trap Ground Entity System (HIGH PRIORITY)

**Scope:** Server-side ground entity registry for traps with placement, trigger, expiry, and removal logic.

**Key components:**
1. `activeTraps` Map on server -- tracks all placed traps per zone
2. Trap trigger check in enemy AI tick (200ms) -- check if any enemy position overlaps a trap
3. Trap expiry/auto-detonate interval (1000ms)
4. Trap placement validation (range, spacing, item cost, max trap limit)
5. Client-side trap visual actors (spawned on `trap:placed`, removed on trigger/expire)

**New Socket.io events:** `trap:placed`, `trap:triggered`, `trap:detonated`, `trap:expired`, `trap:removed`

**Estimated effort:** Large (new subsystem)

### 9.2 MISC Damage Calculator (MEDIUM PRIORITY)

**Scope:** New damage function that bypasses DEF/MDEF/FLEE/cards/size, applying only element modifiers.

```js
function calculateMiscDamage(baseDamage, skillElement, targetElement, targetElementLevel) {
    const eleMod = getElementModifier(skillElement, targetElement, targetElementLevel);
    if (eleMod <= 0) return { damage: 0, hitType: 'elementImmune', element: skillElement };
    const variance = 0.9 + Math.random() * 0.2;
    return {
        damage: Math.max(1, Math.floor(baseDamage * eleMod / 100 * variance)),
        hitType: 'misc',
        isMiss: false,
        isCritical: false,
        element: skillElement
    };
}
```

**Location:** Add to `ro_damage_formulas.js` alongside `calculatePhysicalDamage` and `calculateMagicalDamage`.

**Estimated effort:** Small (single function)

### 9.3 Falcon Companion State (MEDIUM PRIORITY)

**Scope:** Track `player.hasFalcon` boolean. Gate falcon skills behind it.

**Components:**
1. `player.hasFalcon` field on player object
2. Falcon rental NPC interaction (100z cost, Falconry Mastery required)
3. Falcon loss conditions (optional: lose on death, or persistent until zone change)
4. Auto-Blitz Beat trigger in auto-attack combat tick
5. Falcon skill prerequisite checks (Blitz Beat, Detect, Spring Trap)

**Estimated effort:** Medium

### 9.4 Trap Item Consumption (MEDIUM PRIORITY)

**Scope:** Check and deduct Trap items (ID 1065) from inventory when placing traps.

**Components:**
1. Check `character_inventory` for Trap items with quantity >= cost (1 or 2)
2. Deduct quantity on placement
3. Return 1 Trap item on Remove Trap
4. Add Trap items to NPC shop inventory

**Estimated effort:** Small (uses existing inventory:use pattern)

### 9.5 Client Trap Subsystem (MEDIUM PRIORITY)

**Scope:** New UWorldSubsystem for trap visuals on the client.

**Components:**
1. `UTrapSubsystem` -- listens to `trap:placed`, `trap:triggered`, `trap:expired`, `trap:removed`
2. Spawn/destroy trap visual actors at ground positions
3. Trap visual could be a simple mesh (box/sphere) with element-colored material
4. Auto-hide enemy traps (only show own + party traps)

**Estimated effort:** Medium (new subsystem, but follows established patterns)

### 9.6 Immobilize Status Effect (LOW PRIORITY)

**Scope:** Ankle Snare's immobilization is distinct from Stun/Freeze -- target can attack and cast but cannot move.

**Implementation options:**
A. New status effect type `'ankle_snare'` with `canMove: false, canAttack: true, canSkill: true`
B. Modify position validation to reject movement from snared entities

**Estimated effort:** Small-Medium

---

## 10. Implementation Priority

### Phase 1: Foundation (Prerequisites + Passives)

1. **Fix all skill data definitions** -- correct prerequisites, remove incorrect cooldowns, add missing ACD/cast times
2. **Add Beast Bane to getPassiveSkillBonuses()** -- race ATK bonus for brute/insect
3. **Add MISC damage calculator** -- `calculateMiscDamage()` in `ro_damage_formulas.js`
4. **Add `player.hasFalcon` state** -- boolean on player object

### Phase 2: Core Trap System

5. **Build trap ground entity registry** -- `activeTraps` Map, placement validation, trigger detection
6. **Trap trigger in enemy AI tick** -- check enemy positions against active traps each tick
7. **Trap expiry interval** -- 1-second check for lifetime expiry and Blast Mine auto-detonate
8. **Implement damage trap handlers** -- Land Mine, Blast Mine, Claymore Trap, Freezing Trap
9. **Implement status trap handlers** -- Ankle Snare, Sandman, Flasher
10. **Implement utility trap handlers** -- Skid Trap, Shockwave Trap, Remove Trap, Spring Trap

### Phase 3: Falcon System

11. **Implement Blitz Beat handler** -- manual cast, MISC damage, 3x3 splash (not split)
12. **Implement Auto-Blitz Beat** -- LUK/3% trigger in auto-attack tick, damage split among targets
13. **Implement Detect handler** -- reveal hidden entities in 7x7 area
14. **Implement Phantasmic Arrow handler** -- 150% ATK, 3-cell knockback, no arrow consumption

### Phase 4: Client Integration

15. **Client trap subsystem** -- visual actors for placed traps
16. **Trap VFX configs** -- explosion effects for damage traps, status effects for others
17. **Blitz Beat VFX** -- falcon strike animation/particle

### Phase 5: Polish

18. **Talkie Box** -- text input UI + message display
19. **Trap item shop integration** -- ensure NPC shops sell Trap items
20. **Falcon rental NPC** -- Kafra-style interaction for falcon rental

---

## 11. Integration Points

### 11.1 Existing Systems Touched

| System | Change Required |
|--------|----------------|
| `index.js` combat tick | Add Auto-Blitz Beat check after auto-attack damage |
| `index.js` skill:use handler | Add handlers for all 15 active Hunter skills |
| `index.js` getPassiveSkillBonuses() | Add Beast Bane race ATK bonus |
| `ro_damage_formulas.js` | Add `calculateMiscDamage()` function |
| `ro_skill_data_2nd.js` | Fix 15+ field errors across 18 skill definitions |
| Enemy AI tick (`index.js`) | Add trap trigger detection for enemy movement |
| `ro_status_effects.js` | Add `ankle_snare` immobilize effect (if not using existing) |
| VFX/SkillVFXData.cpp | Add VFX configs for 15+ Hunter skills |
| Client subsystem | New `TrapSubsystem` for trap visuals |

### 11.2 Status Effects Used by Hunter

| Skill | Status Effect | Existing? |
|-------|---------------|-----------|
| Land Mine | Stun | YES -- `ro_status_effects.js` |
| Sandman | Sleep | YES -- `ro_status_effects.js` |
| Flasher | Blind | YES -- `ro_status_effects.js` |
| Freezing Trap | Freeze | YES -- `ro_status_effects.js` |
| Ankle Snare | Immobilize | **NO -- new status type needed** |
| Shockwave Trap | SP Drain | **NO -- direct SP modification, not a status** |

### 11.3 Inventory Integration

- Trap items (ID 1065) must be in `items` table with `buy_price: 100, weight: 10`
- NPC shops in hunter-accessible zones should sell Trap items
- `inventory:update` event after Trap consumption
- Remove Trap returns 1 Trap to inventory

---

## 12. Constants & Data Tables

### 12.1 Hunter-Specific Constants

```js
const HUNTER_CONSTANTS = {
    // Trap system
    MAX_ACTIVE_TRAPS: 3,            // Maximum traps placed simultaneously
    TRAP_PLACEMENT_RANGE: 150,      // 3 cells in UE units
    TRAP_TRIGGER_RADIUS: 50,        // 1 cell -- entity must be within 1 cell to trigger
    TRAP_MIN_SPACING: 100,          // 2 cells minimum between traps
    TRAP_ITEM_ID: 1065,             // Item ID for Trap consumable

    // Falcon
    FALCON_RENTAL_COST: 100,        // Zeny to rent falcon
    AUTO_BLITZ_SPLASH_RADIUS: 125,  // 3x3 cells = 2.5 cell radius
    BLITZ_BEAT_SPLASH_RADIUS: 125,  // Same 3x3 splash

    // Damage variance
    TRAP_DAMAGE_VARIANCE: 0.1,      // +/- 10% on trap damage
};
```

### 12.2 Trap Item Cost Table

```js
const TRAP_ITEM_COSTS = {
    'skid_trap': 1,
    'land_mine': 1,
    'ankle_snare': 1,
    'sandman': 1,
    'blast_mine': 1,
    'flasher': 2,
    'freezing_trap': 2,
    'shockwave_trap': 2,
    'claymore_trap': 2,
    'talkie_box': 1,
    'remove_trap': 0,   // Returns 1 instead
    'spring_trap': 0,    // No cost
};
```

### 12.3 Trap Lifetime Table (in seconds)

```js
// Trap lifetimes by skill and level (in seconds)
// Most traps: lifetime DECREASES with level (forces higher level play)
const TRAP_LIFETIMES = {
    'skid_trap':      [300, 240, 180, 120, 60],    // (6-Lv) * 60
    'land_mine':      [200, 160, 120,  80, 40],    // (5-Lv+1) * 40
    'ankle_snare':    [250, 200, 150, 100, 50],    // (6-Lv) * 50
    'shockwave_trap': [200, 160, 120,  80, 40],    // Same as land mine
    'sandman':        [150, 120,  90,  60, 30],    // (6-Lv) * 30
    'flasher':        [150, 120,  90,  60, 30],    // Same as sandman
    'freezing_trap':  [150, 120,  90,  60, 30],    // Same as sandman
    'blast_mine':     [ 25,  20,  15,  10,  5],    // Auto-detonate timer (30-5*Lv)
    'claymore_trap':  [ 20,  40,  60,  80, 100],   // INCREASES with level (20*Lv)
    'talkie_box':     [600],                        // Fixed 10 minutes
};
```

### 12.4 Auto-Blitz Hit Count Table

```js
function getAutoBlitzHits(blitzBeatLv, jobLevel) {
    const jobLvHits = Math.floor((jobLevel + 9) / 10);
    return Math.min(blitzBeatLv, jobLvHits);
}
```

---

## Sources

- [Hunter - iRO Wiki](https://irowiki.org/wiki/Hunter)
- [Hunter - iRO Wiki Classic](https://irowiki.org/classic/Hunter)
- [Hunter Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=11)
- [Blitz Beat - iRO Wiki](https://irowiki.org/wiki/Blitz_Beat)
- [Blitz Beat - iRO Wiki Classic](https://irowiki.org/classic/Blitz_Beat)
- [Ankle Snare - iRO Wiki](https://irowiki.org/wiki/Ankle_Snare)
- [Ankle Snare - rAthena Pre-Re](https://pre.pservero.com/skill/ht_anklesnare)
- [Claymore Trap - iRO Wiki](https://irowiki.org/wiki/Claymore_Trap)
- [Claymore Trap - iRO Wiki Classic](https://irowiki.org/classic/Claymore_Trap)
- [Freezing Trap - iRO Wiki](https://irowiki.org/wiki/Freezing_Trap)
- [Blast Mine - iRO Wiki](https://irowiki.org/wiki/Blast_Mine)
- [Blast Mine - rAthena Pre-Re](https://pre.pservero.com/skill/HT_BLASTMINE)
- [Land Mine - iRO Wiki](https://irowiki.org/wiki/Land_Mine)
- [Skid Trap - iRO Wiki](https://irowiki.org/wiki/Skid_Trap)
- [Sandman - iRO Wiki](https://irowiki.org/wiki/Sandman)
- [Flasher - iRO Wiki](https://irowiki.org/wiki/Flasher)
- [Shockwave Trap - rAthena Pre-Re](https://pre.pservero.com/skill/HT_SHOCKWAVE)
- [Phantasmic Arrow - iRO Wiki](https://irowiki.org/wiki/Phantasmic_Arrow)
- [Phantasmic Arrow - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=1009)
- [Beast Bane - iRO Wiki Classic](https://irowiki.org/classic/Beast_Bane)
- [Beast Charge - iRO Wiki](https://irowiki.org/wiki/Beast_Charge)
- [Falcon Assault - iRO Wiki](https://irowiki.org/wiki/Falcon_Assault)
- [Trap Hunter Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/25790)
- [Trap Item - RateMyServer](https://ratemyserver.net/index.php?page=item_db&item_id=1065)
- [Hunter Skill Quest - iRO Wiki](https://irowiki.org/wiki/Hunter_Skill_Quest)
- [Existing Research: Hunter_Bard_Dancer_Skills_Research.md](Hunter_Bard_Dancer_Skills_Research.md)
- [Archer Audit: Archer_Skills_Audit_And_Fix_Plan.md](Archer_Skills_Audit_And_Fix_Plan.md)
