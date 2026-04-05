# Thief -> Assassin / Rogue Skills -- Deep Research

> **Sources**: rAthena pre-renewal source (authoritative), iRO Wiki Classic, RateMyServer Pre-RE DB, Divine Pride, Hercules pre-re, DarkHikari's guides, WarpPortal forums
> **Scope**: Pre-Renewal (pre-RE) only. All formulas verified against rAthena pre-re source code.
> **Date**: 2026-03-22

---

## Table of Contents

1. [Thief Skills (IDs 500-509)](#1-thief-skills-ids-500-509)
2. [Assassin Skills (IDs 1100-1111)](#2-assassin-skills-ids-1100-1111)
3. [Rogue Skills (IDs 1700-1718)](#3-rogue-skills-ids-1700-1718)
4. [Dual Wield System (Assassin Only)](#4-dual-wield-system-assassin-only)
5. [Hiding/Cloaking System](#5-hidingcloaking-system)
6. [Poison System](#6-poison-system)
7. [Plagiarism System](#7-plagiarism-system)
8. [Strip/Divest System](#8-stripdivest-system)
9. [Steal System](#9-steal-system)
10. [Close Confine Mechanics](#10-close-confine-mechanics)
11. [Skill Trees and Prerequisites](#11-skill-trees-and-prerequisites)
12. [Implementation Checklist](#12-implementation-checklist)
13. [Gap Analysis](#13-gap-analysis)

---

## 1. Thief Skills (IDs 500-509)

### 1.1 Double Attack (ID 500) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 48 (TF_DOUBLE) |
| Type | Passive |
| Max Level | 10 |
| Weapon | Daggers ONLY (not katars at 1st class) |
| Scope | Auto-attack ONLY -- never procs on skills |

**Proc Chance Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Chance (%) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

**Formula**: `procChance = skillLv * 5`

**Mechanics (rAthena verified)**:
- Each proc strikes twice with full auto-attack damage per hit (2 bundled hits, one damage packet)
- Double Attack does NOT crit in pre-renewal (rAthena issue #4460)
- HIT Bonus: +1 HIT per level (passive, always active with daggers equipped)
- Katar Off-Hand Bonus (Assassin only): When using a Katar, Double Attack grants `(1 + 2*SkillLv)%` off-hand damage bonus (effectively adds to Katar attacks)
- At Assassin class, Double Attack also procs on Katar (upgraded from dagger-only at Thief)

---

### 1.2 Improve Dodge (ID 501) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 49 (TF_MISS) |
| Type | Passive |
| Max Level | 10 |

**Effects:**
- 1st class (Thief): +3 FLEE per level (+3 to +30)
- 2nd class (Assassin/Rogue): +4 FLEE per level (+4 to +40)
- Assassin-specific: Slight movement speed increase at Lv10 (1% MSPD bonus)
- NO Perfect Dodge bonus at any level

**Formula**: `bonusFLEE = skillLv * (is2ndClass ? 4 : 3)`

---

### 1.3 Steal (ID 502) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 50 (TF_STEAL) |
| Type | Active, single target |
| Max Level | 10 |
| SP Cost | 10 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 1 second |
| Range | Melee (1 cell) |
| Boss Immunity | Boss monsters CANNOT be stolen from |
| One-Time Lock | Can only steal once per monster (after success) |

**Success Formula (rAthena, two-step)**:

Step 1 -- Overall success roll:
```
overallChance(%) = BaseSuccessRate + (casterDEX - monsterDEX) / 2
```
Where `BaseSuccessRate = 4 + 6 * skillLv` (Lv1=10%, Lv10=64%)

Step 2 -- Item selection (if overall passes):
```
AdjustedDropRatio = DropRatio * (DEX + 3*SkillLv - MonsterDEX + 10) / 100
```
- Iterate through monster's drop list (up to 8 items), top to bottom
- Roll against each item's AdjustedDropRatio
- First item that passes is stolen

**Base Success Rate Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Base % | 10 | 16 | 22 | 28 | 34 | 40 | 46 | 52 | 58 | 64 |

**Key Rules:**
- Stolen item comes from the monster's normal drop table
- Successful steal does NOT affect what drops when monster dies
- Stolen item goes directly to player inventory (subject to weight check)
- If inventory is full or overweight, steal "succeeds" but item is lost

---

### 1.4 Hiding (ID 503) -- Toggle

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 51 (TF_HIDING) |
| Type | Toggle |
| Max Level | 10 |
| SP Cost (activation) | 10 SP |
| Movement | CANNOT move (unless Rogue with Tunnel Drive) |
| Items | CANNOT use items while hidden |
| Attacking | CANNOT attack while hidden |
| Skills | CANNOT use skills (except Hiding toggle to cancel) |
| HP/SP Regen | NO regeneration while hidden |

**Duration and SP Drain Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 30 | 60 | 90 | 120 | 150 | 180 | 210 | 240 | 270 | 300 |
| SP Drain (1 SP every Xs) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |

**Duration Formula**: `30 * skillLv` seconds
**SP Drain Interval**: `(4 + skillLv)` seconds per 1 SP

**Break Conditions:**
1. Manual toggle off (use Hiding again)
2. Duration expires
3. SP reaches 0 from drain
4. Detected by Sight, Ruwach, or Detecting skills (AoE reveal)
5. Detected by Insect race, Demon race, or Boss Protocol monsters
6. Maya Purple card (headgear) on nearby player
7. Bleeding status drains break hiding on HP drain tick
8. Improve Concentration (Hunter) reveals hidden in AoE

**Does NOT break on:**
- Normal attacks from non-detecting enemies (they cannot target you)
- AoE spells that do not specifically reveal (e.g., Thunderstorm damages but does not reveal in all implementations -- server-specific)

See [Section 5: Hiding/Cloaking System](#5-hidingcloaking-system) for full mechanics.

---

### 1.5 Envenom (ID 504) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 52 (TF_POISON) |
| Type | Active, melee single target |
| Max Level | 10 |
| SP Cost | 12 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Range | 2 cells (~100 UE units) |
| Element | Poison |

**Damage Formula (UNIQUE -- hybrid attack)**:
```
Total Damage = NormalMeleeAttack(100% ATK, Poison element) + FlatBonus
FlatBonus = 15 * SkillLv (bypasses DEF entirely, always hits even if main attack misses)
```

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Flat ATK Bonus | +15 | +30 | +45 | +60 | +75 | +90 | +105 | +120 | +135 | +150 |
| Poison Chance (%) | 14 | 18 | 22 | 26 | 30 | 34 | 38 | 42 | 46 | 50 |

**Poison Chance Formula**: `(10 + 4 * SkillLv)%`

**Critical Mechanic**: The flat `15*Lv` bonus is described as "always inflicted, whether your character lands a normal hit or not" (RateMyServer pre-RE). This means:
1. Calculate normal melee attack with Poison element (100% ATK)
2. Add flat bonus of `15 * SkillLv` -- this portion bypasses Armor DEF and VIT softDef
3. If the main attack misses (FLEE dodge), the flat bonus STILL deals damage

**Poison Status:**
- Poison Duration: 60 seconds
- Poison reduces target physical DEF by 25%
- Poison drains 3% MaxHP every 3 seconds (floor at 25% MaxHP)
- Boss and Undead monsters immune to Poison status

---

### 1.6 Detoxify (ID 505) -- Active (Supportive)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 53 (TF_DETOXIFY) |
| Type | Active, supportive |
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 0 (instant) |
| Range | 9 cells (~450 UE units) |
| Target | Self or any friendly player |
| Prerequisites | Envenom Lv3 |

**Effect:** Removes Poison status effect ONLY (does not cure Deadly Poison, which requires Alchemist's Royal Guard/Panacea). Can target self or other players regardless of party membership.

---

### 1.7 Sand Attack (ID 506) -- Active (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 149 (TF_SPRINKLESAND) |
| Type | Active, melee single target |
| Max Level | 1 |
| SP Cost | 9 |
| Cast Time | 0 (instant) |
| Range | Melee (1 cell) |
| Element | Earth |
| Quest Requirement | Job Level 25 |

**Damage**: 130% ATK (rAthena pre-RE verified, RateMyServer confirms)
**Blind Chance**: 20% (rAthena pre-RE)
**Blind Duration**: 30 seconds (standard)

---

### 1.8 Back Slide (ID 507) -- Active (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 150 (TF_BACKSLIDING) |
| Type | Active, self-targeted |
| Max Level | 1 |
| SP Cost | 7 |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0 |
| Distance | 5 cells backward (relative to facing direction) |
| Quest Requirement | Job Level 35 |

**Mechanics:**
- Instant backward movement of 5 cells (250 UE units)
- Direction based on character facing (not movement direction)
- Blocked by unwalkable cells (stops at wall/obstacle)
- Can use `/bingbing` and `/bangbang` commands to change facing before use (direction tricks)
- Server must update player position cache after teleport
- Breaks auto-attack state

---

### 1.9 Pick Stone (ID 508) -- Active (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 151 (TF_PICKSTONE) |
| Type | Active, self-targeted |
| Max Level | 1 |
| SP Cost | 3 |
| Cast Time | 0.5 seconds |
| Quest Requirement | Job Level 15 |

**Mechanics:**
- Adds 1 Stone (Item ID 7049, Weight: 3) to inventory
- Weight restriction: Must be under 50% weight capacity
- Can hold multiple stones (limited by weight/inventory slots)
- Stone is used as ammunition for Throw Stone skill

---

### 1.10 Throw Stone (ID 509) -- Active (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 152 (TF_THROWSTONE) |
| Type | Active, ranged single target |
| Max Level | 1 |
| SP Cost | 2 |
| Cast Time | 0 (instant) |
| Range | 7 cells (~350 UE units) |
| Element | Neutral |
| Quest Requirement | Job Level 20 |

**Damage**: Fixed 50 damage (does not scale with ATK)
**HIT**: Always hits (ignores FLEE)
**DEF**: Does NOT ignore DEF in pre-renewal (damage reduced by target DEF)
**Stun Chance**: 3%
**Stun Duration**: Standard stun (5 seconds, subject to VIT resistance)
**Ammunition**: Consumes 1 Stone (ID 7049) per cast

---

## 2. Assassin Skills (IDs 1100-1111)

### 2.1 Katar Mastery (ID 1100) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 134 (AS_KATAR) |
| Type | Passive |
| Max Level | 10 |
| Weapon | Katar class only |

**Effect**: +3 flat ATK per level when wielding a Katar (added as mastery bonus, bypasses DEF).

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| +ATK | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

---

### 2.2 Sonic Blow (ID 1101) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 136 (AS_SONICBLOW) |
| Type | Active, Offensive |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element |
| Weapon | Katar class ONLY |
| Cast Time | 0 (instant) |
| After-Cast Delay | 2 seconds (fixed animation time) |
| Prerequisites | Katar Mastery Lv4 |
| Hit Count | 8 visual hits, ONE damage calculation |

**Damage Formula (rAthena pre-RE CORRECTED)**:
```
ATK% = 300 + 50 * SkillLv
```

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 350 | 400 | 450 | 500 | 550 | 600 | 650 | 700 | 750 | 800 |
| SP Cost | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 | 32 | 34 |
| Stun% | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |

**SP Formula**: `14 + 2 * SkillLv`
**Stun Formula**: `10 + 2 * SkillLv` %

**Special Mechanics:**
1. Katar required -- skill fails without katar
2. Can miss -- subject to HIT/FLEE check (NOT forceHit)
3. Can be elemental -- uses weapon element (Enchant Poison makes it Poison element)
4. Boss protocol monsters immune to Stun
5. 8 hits visual but damage calculated as single instance
6. 2-second animation cannot be reduced

**Sonic Acceleration Interaction (Quest Skill 1106):**
- +50% damage multiplicatively: `effectiveATK% = ATK% * 1.5`
- +50% to final HIT rate: `effectiveHitRate = hitRate * 1.5`

**EDP Interaction (Pre-Renewal):**
- EDP DOES affect Sonic Blow, but skill modifier is HALVED first
- Formula: `effectiveATK% = (ATK% / 2) * edpMultiplier`
- Example: SB Lv10 + EDP Lv5 = `(800% / 2) * 4.0 = 1600%`

---

### 2.3 Grimtooth (ID 1102) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 137 (AS_GRIMTOOTH) |
| Type | Active, Offensive |
| Target | Single target with 3x3 AoE splash |
| Range | 2+SkillLv cells |
| Element | Weapon element |
| Weapon | Katar class ONLY |
| Required Status | Must be in Hiding or Cloaking |
| Cast Time | 0 (instant) |
| After-Cast Delay | ASPD-based |
| Prerequisites | Sonic Blow Lv5, Cloaking Lv2 |
| Critical Feature | Does NOT break Hiding |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 120 | 140 | 160 | 180 | 200 |
| SP Cost | 3 | 3 | 3 | 3 | 3 |
| Range (cells) | 3 | 4 | 5 | 6 | 7 |

**ATK Formula**: `100 + 20 * SkillLv`
**Range Formula**: `(2 + SkillLv)` cells

**Attack Classification (critical for defensive interactions):**
- Lv1-2: MELEE attack (blocked by Safety Wall, NOT by Pneuma)
- Lv3-5: RANGED attack (blocked by Pneuma, NOT by Safety Wall)

**Special Mechanics:**
1. Must be hidden (Hiding or Cloaking buff active) -- fails otherwise
2. Does NOT break Hiding/Cloaking after use (unique -- Grimtooth stays hidden)
3. 3x3 AoE splash around primary target
4. At Lv3+, status arrows work (poison/stun/etc.), but NOT elemental arrows
5. EDP does NOT affect Grimtooth damage
6. Katar required -- fails without katar

---

### 2.4 Cloaking (ID 1103) -- Toggle

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 135 (AS_CLOAKING) |
| Type | Toggle |
| Target | Self |
| Max Level | 10 |
| SP Cost (activation) | 15 |
| Prerequisites | Hiding Lv2 |
| Duration | Until manually canceled, revealed, or SP depleted |

**Per-Level Data:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Wall Required | Yes | Yes | No | No | No | No | No | No | No | No |
| Speed (on-wall) | 103% | 106% | 109% | 112% | 115% | 118% | 121% | 124% | 127% | 130% |
| Speed (off-wall) | 0% | 0% | 79% | 82% | 85% | 88% | 91% | 94% | 97% | 100% |
| SP Drain (1 SP per Xs) | 0.5 | 1.0 | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 7.0 | 8.0 | 9.0 |

**Key Differences from Hiding:**
1. CAN MOVE while cloaked (Hiding cannot move at all)
2. Skills cancel cloak (using any skill other than Cloaking toggle breaks it)
3. Attacks cancel cloak (auto-attacking breaks Cloaking)
4. Items can be used but cancel Cloaking
5. Lv1-2 require wall adjacency to stay cloaked
6. No HP/SP regen while cloaked
7. AoE damage still hits cloaked players
8. Single-target skills already cast on you still execute (unlike Hiding)

**Detection**: Same as Hiding (Sight, Ruwach, Insect/Demon/Boss, Maya Purple, Improve Concentration, AoE reveal skills like Heaven's Drive, Earth Spike, Arrow Shower)

See [Section 5: Hiding/Cloaking System](#5-hidingcloaking-system) for full comparison.

---

### 2.5 Poison React (ID 1104) -- Active (Self-Buff)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 139 (AS_POISONREACT) |
| Type | Active, Self-Buff |
| Target | Self |
| Max Level | 10 |
| Prerequisites | Enchant Poison Lv3 (ID 1109) |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Counter ATK% | 130 | 160 | 190 | 220 | 250 | 280 | 310 | 340 | 370 | 400 |
| Envenom Counters | 1 | 1 | 2 | 2 | 3 | 3 | 4 | 4 | 5 | 6 |
| Duration (s) | 20 | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 60 |
| SP Cost | 25 | 30 | 35 | 40 | 45 | 50 | 55 | 60 | 45 | 45 |

**Counter ATK Formula**: `100 + 30 * SkillLv`
**Envenom Counter Limit**: `floor((SkillLv + 1) / 2)` for Lv1-9, 6 at Lv10
**Duration**: `15 + 5 * SkillLv` seconds (capped at 60s for Lv9-10)

**Two Modes of Operation (rAthena verified):**

**Mode A -- Poison Element Counter (one-time)**:
- When hit by a Poison-property attack, automatically counterattacks with `(100 + 30*Lv)%` ATK damage
- 50% chance to inflict Poison status on the attacker
- Consumes the buff entirely (one-shot counter)
- Only triggers once per activation

**Mode B -- Non-Poison Envenom Counter (multi-use)**:
- When hit by any NON-poison physical attack, 50% chance to auto-cast Envenom Lv5
- Can trigger up to `envenomCounterLimit` times
- After all counters used, buff expires
- Does NOT consume the poison counter (Mode A still available)

**Buff expires when**: Duration runs out OR Mode A triggers OR all Mode B counters used (whichever first).

---

### 2.6 Venom Dust (ID 1105) -- Active (Ground Effect)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 140 (AS_VENOMDUST) |
| Type | Active, Ground AoE |
| Target | Ground |
| AoE Size | 2x2 cells |
| Range | 2 cells (~100 UE units) |
| Element | Poison |
| Catalyst | 1 Red Gemstone per cast |
| Max Level | 10 |
| SP Cost | 20 (flat, all levels) |
| Prerequisites | Enchant Poison Lv5, Poison React Lv3 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 | 45 | 50 |

**Duration Formula**: `5 * SkillLv` seconds

**Mechanics:**
- Places a poison cloud on the ground
- Enemies standing in the cloud are poisoned (applies Poison status)
- Does NOT deal direct damage -- only applies Poison status effect
- Boss-type monsters immune
- Does NOT refresh poison duration on already-poisoned enemies
- Cloud persists for full duration even if caster moves/dies
- Requires 1 Red Gemstone consumed per cast

---

### 2.7 Sonic Acceleration (ID 1106) -- Passive (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 1003 (AS_SONICACCEL) |
| Type | Passive (Quest Skill) |
| Max Level | 1 |
| Quest Requirement | Assassin Skill Quest, Job Level 30+ |
| Weapon | Katar |

**Effects:**
- +50% damage to Sonic Blow (multiplicatively applied to ATK%)
- +50% to final HIT rate for Sonic Blow (multiplicative, not additive HIT stat)

---

### 2.8 Righthand Mastery (ID 1107) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 132 (AS_RIGHT) |
| Type | Passive |
| Max Level | 5 |

**Right Hand Damage Recovery:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Right Hand ATK% | 60% | 70% | 80% | 90% | 100% |

**Formula**: `rightHandDamage% = 50 + RHM_Lv * 10`

- Base penalty without mastery: right hand deals only 50% damage when dual-wielding
- Lv5 = 100% = full right-hand damage recovery
- Only affects AUTO-ATTACKS, NOT skills (skills always use full weapon damage)
- Has no effect when using a Katar or single weapon

---

### 2.9 Lefthand Mastery (ID 1108) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 133 (AS_LEFT) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Righthand Mastery Lv2 |

**Left Hand Damage Recovery:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Left Hand ATK% | 40% | 50% | 60% | 70% | 80% |

**Formula**: `leftHandDamage% = 30 + LHM_Lv * 10`

- Base penalty without mastery: left hand deals only 30% damage
- Lv5 = 80% = left hand never fully recovers (design intent: off-hand is always weaker)
- Left hand always force-hits (cannot miss) and NEVER crits independently
- Only affects AUTO-ATTACKS, NOT skills

---

### 2.10 Enchant Poison (ID 1109) -- Active (Buff)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 138 (AS_ENCHANTPOISON) |
| Type | Supportive (Buff) |
| Target | Self or Party Member (1 cell range) |
| Max Level | 10 |
| SP Cost | 20 (flat) |
| Cast Time | 0 |
| After-Cast Delay | 1 second |
| Prerequisites | Envenom Lv1 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Duration (s) | 30 | 45 | 60 | 75 | 90 | 105 | 120 | 135 | 150 | 165 |
| Poison Proc (%) | 3.0 | 3.5 | 4.0 | 4.5 | 5.0 | 5.5 | 6.0 | 6.5 | 7.0 | 7.5 |

**Duration Formula**: `(15 + 15 * SkillLv)` seconds
**Poison Proc Formula**: `(2.5 + 0.5 * SkillLv)%`

**Mechanics:**
1. Changes weapon element to Poison for the duration
2. Each physical attack has `(2.5 + 0.5*Lv)%` chance to inflict Poison on target
3. Weapon switch cancels the buff
4. Does NOT stack with other weapon endows (Aspersio, Sage endows) -- last applied wins
5. Overwrite order: remove existing endow buffs (`aspersio`, `enchant_poison`, `endow_fire/water/wind/earth`) then apply new one
6. The poison from this is regular Poison (not Deadly Poison)

---

### 2.11 Venom Splasher (ID 1110) -- Active (Delayed Detonation)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 141 (AS_SPLASHER) |
| Type | Active, Offensive (Delayed) |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| AoE on Detonation | 3x3 cells |
| Element | Poison |
| Catalyst | 1 Red Gemstone (pre-renewal) |
| Max Level | 10 |
| Cast Time | 1 second |
| Prerequisites | Poison React Lv5, Venom Dust Lv5 |

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 550 | 600 | 650 | 700 | 750 | 800 | 850 | 900 | 950 | 1000 |
| SP Cost | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
| Timer (s) | 5.0 | 5.5 | 6.0 | 6.5 | 7.0 | 7.5 | 8.0 | 8.5 | 9.0 | 9.5 |
| Cooldown (s) | 7.5 | 8.0 | 8.5 | 9.0 | 9.5 | 10.0 | 10.5 | 11.0 | 11.5 | 12.0 |

**Formulas:**
- ATK%: `500 + 50 * SkillLv`
- SP Cost: `10 + 2 * SkillLv`
- Explosion Timer: `4.5 + 0.5 * SkillLv` seconds
- Cooldown: `7 + 0.5 * SkillLv` seconds

**Mechanics (pre-renewal):**
1. **Target HP Restriction**: Target must be at or below 75% HP (3/4 of MaxHP)
2. **Timed Explosion**: Target is "marked" and a countdown begins
3. After timer expires, target EXPLODES dealing damage in 3x3 AoE
4. AoE damage is divided among all enemies in the splash area
5. Cannot target Boss monsters (Boss Protocol immunity)
6. Explosion always hits (forceHit on detonation)
7. Consumes 1 Red Gemstone per cast
8. If target dies before explosion: explosion is canceled
9. If target hides before explosion: damage canceled
10. Only one Venom Splasher mark active per caster at a time
11. Per-skill cooldown separate from ACD

---

### 2.12 Throw Venom Knife (ID 1111) -- Active (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 1004 (AS_VENOMKNIFE) |
| Type | Active, Offensive (Quest Skill) |
| Target | Single Enemy |
| Range | 9 cells (~450 UE units) |
| Element | Weapon element (right hand) |
| Max Level | 1 |
| SP Cost | 35 |
| Quest Requirement | Assassin Skill Quest |

**Mechanics:**
- ATK%: 100% (original pre-renewal value; post-balance-patch updated to 500%)
- Ranged physical attack
- Consumes 1 Venom Knife ammunition per use
- Poison chance based on Envenom level: `(10 + 4 * EnvenomLv)%`
- EDP does NOT affect this skill
- Uses weapon element from right hand
- For pre-renewal classic authenticity: use 100% ATK

---

## 3. Rogue Skills (IDs 1700-1718)

### 3.1 Snatcher / Gank (ID 1700) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 210 (RG_SNATCHER) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Steal (502) Lv1 |

**Auto-Steal Trigger Chance:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Chance (%) | 7 | 8 | 10 | 11 | 13 | 14 | 16 | 17 | 19 | 20 |

**Mechanics:**
- On each physical melee AUTO-ATTACK hit, roll `triggerChance`
- If triggered, execute Steal formula (same as Thief's Steal skill)
- Does NOT trigger on ranged attacks (bow) or skills
- Does NOT consume SP
- Uses same `enemy.stolenBy` tracker (one steal per monster)
- Stolen item goes to inventory (weight check applies)

---

### 3.2 Back Stab (ID 1701) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 212 (RG_BACKSTAP) |
| Type | Active, Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element |
| SP Cost | 16 (all levels) |
| Cast Time | 0 (instant) |
| After-Cast Delay | 0.5 seconds (ASPD-based) |
| Prerequisites | Steal Coin Lv4 |
| Always Hits | Yes -- bypasses FLEE and Perfect Dodge (forceHit) |
| Can use from Hiding | Yes -- cancels Hiding on use |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% | 340 | 380 | 420 | 460 | 500 | 540 | 580 | 620 | 660 | 700 |

**ATK Formula**: `300 + 40 * SkillLv`

**Weapon Modifiers (pre-renewal):**
- **Dagger**: Standard 1 hit at full ATK% (2 hits is Renewal-only addition)
- **Bow**: Damage halved (50% penalty)
- **Other weapons (1H sword)**: 1 hit at full damage

**Special Mechanics:**
1. Pre-renewal: must be facing the enemy's back (behind target)
2. Target is turned to face the caster after being hit
3. Teleports caster behind the target (server adjusts position)
4. Cancels Hiding if used while hidden
5. Does NOT crit (skill, not auto-attack)
6. Cards and element modifiers apply normally

---

### 3.3 Tunnel Drive / Stalk (ID 1702) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 213 (RG_TUNNELDRIVE) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Hiding (503) Lv1 |

**Movement Speed While Hidden:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Speed % | 26 | 32 | 38 | 44 | 50 |

**Mechanics:**
- Without Tunnel Drive, Hiding blocks ALL movement
- With Tunnel Drive, movement is allowed at reduced speed
- Hidden status remains active during movement
- SP drain from Hiding still applies during movement

---

### 3.4 Raid / Sightless Mind (ID 1703) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 214 (RG_RAID) |
| Type | Active, Offensive AoE |
| Max Level | 5 |
| Target | Self-centered AoE |
| AoE | 3x3 cells (pre-renewal, NOT 5x5 Renewal) |
| Element | Weapon element |
| SP Cost | 20 (all levels, pre-renewal) |
| Cast Time | 0 (instant) |
| Prerequisites | Back Stab Lv2, Tunnel Drive Lv2 |
| Requirement | Must be in Hiding |
| Effect | Cancels Hiding on use |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| ATK% | 140 | 180 | 220 | 260 | 300 |
| Stun/Blind % | 13 | 16 | 19 | 22 | 25 |

**ATK Formula**: `100 + 40 * SkillLv`
**Status Formula**: `10 + 3 * SkillLv` %

**Mechanics:**
1. Must be in Hiding to use -- fails otherwise
2. Cancels Hiding immediately upon use
3. Physical weapon damage to all enemies in 3x3 around caster
4. Each target independently rolls for Stun AND Blind
5. Boss protocol immune to Stun/Blind

---

### 3.5 Intimidate / Snatch (ID 1704) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 219 (RG_INTIMIDATE) |
| Type | Active, Offensive |
| Max Level | 5 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| Element | Weapon element |
| Prerequisites | Back Stab Lv4, Raid Lv5 |
| Boss Restriction | Cannot target Boss/MVP monsters |

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| SP Cost | 13 | 16 | 19 | 22 | 25 |
| ATK% | 130 | 160 | 190 | 220 | 250 |

**ATK Formula**: `100 + 30 * SkillLv`
**SP Formula**: `10 + 3 * SkillLv`

**Teleport Mechanic:**
1. Deal physical damage to target
2. If target survives and is NOT a Boss: teleport BOTH caster and target to a random walkable position on same map
3. If target is Boss: damage only, NO teleport
4. Restricted maps (GvG, Battleground): damage only, NO teleport
5. PvE: useful for repositioning/fleeing
6. PvP: isolates enemies

---

### 3.6 Sword Mastery -- Rogue (ID 1705) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 2 (SM_SWORD) -- shared skill |
| Type | Passive |
| Max Level | 10 |

**Effect**: +4 ATK per level with Daggers and 1H Swords (mastery ATK, bypasses DEF).

Same as Swordsman's Sword Mastery (ID 100). Non-stacking: if both are somehow learned, use `Math.max(learnedSkills[100], learnedSkills[1705])`.

---

### 3.7 Vulture's Eye -- Rogue (ID 1706) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 44 (AC_VULTURE) -- shared skill |
| Type | Passive |
| Max Level | 10 |

**Effect**: +1 bow attack range per level, +1 HIT per level.

Same as Archer's Vulture's Eye (ID 301). Non-stacking: use `Math.max()`.

---

### 3.8 Double Strafe -- Rogue (ID 1707) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 46 (AC_DOUBLE) -- shared skill |
| Type | Active, Offensive |
| Max Level | 10 |
| Target | Single Enemy |
| Range | 9 cells + Vulture's Eye |
| Element | Weapon element (arrow element) |
| SP Cost | 12 (all levels) |
| Prerequisites | Vulture's Eye (1706) Lv10 |
| Weapon | Bow ONLY, consumes 1 arrow |
| Hits | 2 |

**Damage Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| ATK% per hit | 100 | 110 | 120 | 130 | 140 | 150 | 160 | 170 | 180 | 190 |
| Total ATK% | 200 | 220 | 240 | 260 | 280 | 300 | 320 | 340 | 360 | 380 |

Same handler as Archer's Double Strafe (ID 303).

---

### 3.9 Remove Trap -- Rogue (ID 1708) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 124 (HT_REMOVETRAP) -- shared skill |
| Type | Active, Utility |
| Max Level | 1 |
| Target | Ground (trap) |
| Range | 2 cells |
| SP Cost | 5 |
| Prerequisites | Double Strafe (1707) Lv5 |

Removes a Hunter's ground trap and returns the trap item. Primarily PvP/WoE utility.

---

### 3.10 Steal Coin / Mug (ID 1709) -- Active

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 211 (RG_STEALCOIN) |
| Type | Active |
| Max Level | 10 |
| Target | Single Enemy (monsters only) |
| Range | Melee (1 cell) |
| SP Cost | 15 (all levels) |
| Prerequisites | Snatcher Lv4 |
| Restriction | Cannot target Boss monsters or players |
| One-Time | Can only steal Zeny once per monster |

**Success Rate Formula (rAthena):**
```
rate = (DEX/2 + LUK/2 + 2*(casterLevel - targetLevel) + skillLevel*10) / 10
```

**Zeny Stolen Formula:**
```
Zeny = random(0, 2*monsterLevel + 1) + 8*monsterLevel
```
Example: Monster Level 50 -> `random(0, 101) + 400` -> 400-501 Zeny

---

### 3.11-3.14 Divest / Strip Skills (IDs 1710-1713)

See [Section 8: Strip/Divest System](#8-stripdivest-system) for full mechanics.

| Skill | ID | rA ID | Slot | SP (Lv1-5) | Prerequisites |
|-------|-----|-------|------|-------------|---------------|
| Divest Helm | 1710 | 218 (RG_STRIPHELM) | Head | 12/14/16/18/20 | Steal Coin Lv2 |
| Divest Shield | 1711 | 216 (RG_STRIPSHIELD) | Shield | 12/14/16/18/20 | Divest Helm Lv5 |
| Divest Armor | 1712 | 217 (RG_STRIPARMOR) | Body | 17/19/21/23/25 | Divest Shield Lv5 |
| Divest Weapon | 1713 | 215 (RG_STRIPWEAPON) | Weapon | 17/19/21/23/25 | Divest Armor Lv5 |

All Divest skills share:
- Max Level: 5
- Target: Single Enemy
- Range: Melee (1 cell)
- Cast Time: 1.0 seconds (interruptible)
- After-Cast Delay: 1.0 seconds
- Blocked by: Chemical Protection (Alchemist)

---

### 3.15 Plagiarism (ID 1714) -- Passive

See [Section 7: Plagiarism System](#7-plagiarism-system) for full mechanics.

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 225 (RG_PLAGIARISM) |
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Intimidate Lv5 |
| Copy Limit | 1 skill at a time |
| Level Cap | Copied skill level capped at Plagiarism skill level |
| ASPD Bonus | +1% per skill level (passive, always active) |

---

### 3.16 Gangster's Paradise (ID 1715) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 223 (RG_GANGSTER) |
| Type | Passive |
| Max Level | 1 |
| Prerequisites | Raid Lv1 |
| AoE | 3x3 cells |

**Mechanics:**
- When 2+ Rogues/Stalkers are sitting within 3x3 cells of each other, nearby non-Boss monsters will not attack any player in the group
- Breaks when any player stands up or moves
- Boss monsters NOT affected (will still aggro)
- Requires sitting state tracking + proximity checks in AI tick

---

### 3.17 Compulsion Discount / Haggle (ID 1716) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 224 (RG_COMPULSION) |
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Gangster's Paradise Lv1 |

**Discount Per Level:**

| Level | 1 | 2 | 3 | 4 | 5 |
|-------|---|---|---|---|---|
| Discount % | 9 | 13 | 17 | 21 | 25 |

**Formula**: `discount% = 5 + level * 4`

Does NOT stack with Merchant's Discount -- use the higher value.

---

### 3.18 Scribble / Graffiti (ID 1717) -- Quest Skill

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 220 (RG_GRAFFITI) |
| Type | Active, Utility |
| Max Level | 1 |
| SP Cost | 15 |
| Duration | 180 seconds |
| Catalyst | 1 Red Gemstone |

Purely cosmetic -- places text graffiti on the ground. No gameplay impact.

---

### 3.19 Close Confine (ID 1718) -- Quest Skill

See [Section 10: Close Confine Mechanics](#10-close-confine-mechanics) for full details.

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 1005 (RG_CLOSECONFINE) |
| Type | Active |
| Max Level | 1 |
| Target | Single Enemy |
| Range | Melee (1 cell) |
| SP Cost | 25 |
| Duration | 10 seconds |
| FLEE Bonus | +10 FLEE for caster |
| Boss Restriction | Cannot target Boss monsters |

---

## 4. Dual Wield System (Assassin Only)

### 4.1 Which Weapons Can Go in Left Hand

Only Assassin and Assassin Cross classes can dual wield. Left-hand eligible weapons:

| Weapon Type | Left Hand Allowed |
|-------------|-------------------|
| Dagger | Yes |
| 1H Sword | Yes |
| 1H Axe | Yes |
| Katar | NO (Katar occupies both hands) |
| 2H Sword | NO (two-handed) |
| Bow | NO |
| Mace | NO |
| Staff/Rod | NO |
| Book | NO |
| Spear | NO |

**Katar is mutually exclusive with dual wield.** You cannot equip a Katar and another weapon simultaneously. Katar = both hands occupied.

### 4.2 Damage Penalty Per Hand

When dual-wielding, both hands suffer significant damage penalties on auto-attacks:

**Without Mastery Skills:**
- Right hand: 50% damage
- Left hand: 30% damage

**With Righthand Mastery (ID 1107):**
- Right hand: `(50 + RHM_Lv * 10)%` (Lv5 = 100%)

**With Lefthand Mastery (ID 1108):**
- Left hand: `(30 + LHM_Lv * 10)%` (Lv5 = 80% -- never fully recovers)

### 4.3 Right-Hand Only for Skills

All skills use right-hand weapon ATK ONLY. The left-hand weapon does not contribute to skill damage at all. The left hand only attacks during auto-attacks.

Exception: Soul Destroyer (Assassin Cross) explicitly excludes left-hand ATK: `PhysicalDamage = (TotalATK - LeftHandWeaponATK) * SkillLv`

### 4.4 Left-Hand Attack Properties

- Left hand always FORCE-HITS (cannot miss, bypasses FLEE)
- Left hand NEVER crits independently
- If right-hand crits, the crit bubble shows on left-hand hit as well (visual only)
- Left hand gets its own card modifiers and element
- Both hands attack per combat tick cycle (two separate damage calculations)

### 4.5 ASPD with Dual Wield

```
WeaponDelay = floor((WD_Right + WD_Left) * 7 / 10)
```
- ASPD cap: 190

### 4.6 Katar Alternative (Double Critical Rate)

When wielding a Katar, the **entire CRI value** is doubled -- not just the LUK portion:

```
CRI_base = 1 + floor(LUK * 0.3) + bonusCRI_from_equipment + bonusCRI_from_cards
CRI_effective = CRI_base * 2  (with Katar)
```

Source: rAthena `status.cpp`: `if (sd->status.weapon == W_KATAR) status->cri *= 2;`

This doubling is not shown in the status window (hidden value). It makes Katar + LUK stacking the premier crit build.

**Effective crit in combat:**
```
CritChance = CRI_effective - floor(targetLUK / 5)
```

---

## 5. Hiding/Cloaking System

### 5.1 Comparison Table

| Feature | Hiding | Cloaking |
|---------|--------|----------|
| Movement | CANNOT move (without Tunnel Drive) | CAN move |
| Items | CANNOT use | CAN use (but cancels cloak) |
| Skills | CANNOT use (except toggle) | CANNOT use (cancels cloak) |
| Auto-Attack | CANNOT attack | CANNOT attack (cancels cloak) |
| HP/SP Regen | NONE | NONE |
| Wall Requirement | None | Lv1-2 require wall adjacency |
| SP Drain | 1 SP / (4+Lv) seconds | 1 SP / variable (0.5s-9s) |
| AoE Damage | Some AoEs reveal | AoEs damage but may not reveal |
| Single-Target | Cancels incoming single-target | Incoming single-target still executes |
| Duration | 30*Lv seconds (fixed) | Until SP depleted/revealed |

### 5.2 Detection Methods (Shared)

| Detection Source | Reveals Hidden | Reveals Cloaked |
|------------------|----------------|-----------------|
| Sight (Mage) | Yes | Yes (within range) |
| Ruwach (Acolyte) | Yes + Holy damage | Yes + Holy damage |
| Detect (Hunter) | Yes | Yes |
| Improve Concentration (Hunter) | Yes (AoE) | Yes (AoE) |
| Heaven's Drive | Yes (AoE damage) | Yes (AoE damage) |
| Earth Spike | No (single target) | No (single target) |
| Arrow Shower | Yes (AoE) | Yes (AoE) |
| Insect Race monsters | Auto-detect | Auto-detect |
| Demon Race monsters | Auto-detect | Auto-detect |
| Boss Protocol monsters | Auto-detect | Auto-detect |
| Maya Purple card | Yes (passive reveal) | Yes (passive reveal) |

### 5.3 SP Drain Details

**Hiding SP Drain Intervals:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Interval (s) | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |

**Cloaking SP Drain Intervals:**

| Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------|---|---|---|---|---|---|---|---|---|---|
| Interval (s) | 0.5 | 1.0 | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 7.0 | 8.0 | 9.0 |

### 5.4 Grimtooth Exception

Grimtooth is the ONLY attack skill that does NOT break Hiding/Cloaking. The caster remains hidden after using Grimtooth. All other attacks/skills cancel the hidden state.

### 5.5 Tunnel Drive (Rogue Passive)

Tunnel Drive (ID 1702) enables movement while in Hiding at reduced speed:
- Lv1: 26%, Lv2: 32%, Lv3: 38%, Lv4: 44%, Lv5: 50% of normal movement speed
- SP drain from Hiding continues during movement
- Detection rules still apply during movement

---

## 6. Poison System

### 6.1 Poison Status Effect

| Property | Value |
|----------|-------|
| Duration | 60 seconds (standard) |
| DEF Reduction | -25% physical DEF |
| HP Drain | 3% MaxHP every 3 seconds |
| HP Floor | Cannot reduce HP below 25% MaxHP |
| Immunities | Boss protocol monsters, Undead race/element |
| Cure | Detoxify (Thief), Green Potion, Panacea |

### 6.2 Deadly Poison Status (Assassin Cross only)

| Property | Value |
|----------|-------|
| Duration | Based on EDP level |
| HP Drain | Faster than regular Poison |
| HP Floor | Cannot reduce below 25% MaxHP |
| Cure | Royal Guard (Alchemist), Panacea only (NOT regular antidote, NOT Detoxify) |

### 6.3 Poison Sources

| Source | Poison Chance | Notes |
|--------|---------------|-------|
| Envenom (504) | (10 + 4*Lv)% | Flat bonus ATK always hits |
| Enchant Poison (1109) | (2.5 + 0.5*Lv)% per hit | Weapon endow, auto-attack proc |
| Poison React (1104) Mode A | 50% on counter | One-time counter vs Poison attacks |
| Venom Dust (1105) | 100% in cloud | Ground AoE, no damage |
| EDP (ASC) | Per-hit chance | Deadly Poison (not regular) |
| Status arrows (Poison Arrow) | 10% | Ranged auto-attack proc |

### 6.4 Venom Dust Ground Effect

- 2x2 cell poison cloud placed on ground
- Enemies in cloud are poisoned (100% rate, checked per tick)
- No direct damage
- Duration: 5*Lv seconds
- Does not refresh poison on already-poisoned targets
- Persists after caster death/movement
- Costs 1 Red Gemstone

---

## 7. Plagiarism System

### 7.1 Core Mechanics

- **Trigger**: When any skill damages the Rogue, the last damaging skill is stored as the plagiarized skill
- **Copy Limit**: 1 skill at a time (new copy overwrites old)
- **Level Cap**: Copied skill level = `min(skillLevelUsedAgainstYou, PlagiarismSkillLevel)`
- **ASPD Bonus**: +1% ASPD per Plagiarism level (passive, always active)
- **Persistence**: Copied skill persists across map changes and logouts (stored in DB)

### 7.2 Copyable Skills (Whitelist)

**CAN be copied** (1st and 2nd class skills that deal damage):

| Source Class | Copyable Skills |
|-------------|-----------------|
| Swordsman | Bash, Magnum Break |
| Knight | Bowling Bash, Charge Attack |
| Crusader | Grand Cross, Holy Cross, Shield Boomerang, Shield Charge |
| Mage | Cold Bolt, Fire Ball, Fire Bolt, Fire Wall, Frost Diver, Lightning Bolt, Napalm Beat, Soul Strike, Thunderstorm |
| Wizard | Earth Spike, Fire Pillar, Frost Nova, Heaven's Drive, Jupitel Thunder, Lord of Vermilion, Meteor Storm, Sight Rasher, Storm Gust, Water Ball, Sight Blaster |
| Sage | Earth Spike, Heaven's Drive |
| Archer | Arrow Repel, Arrow Shower, Double Strafe |
| Hunter | Blast Mine, Claymore Trap, Land Mine, Phantasmic Arrow, Freezing Trap |
| Merchant | Mammonite |
| Alchemist | Acid Terror, Demonstration |
| Thief | Envenom, Sand Attack, Throw Stone |
| Assassin | Venom Splasher, Throw Venom Knife |
| Rogue | Back Stab, Double Strafe (Rogue), Raid |
| Acolyte | Holy Light, Ruwach |
| Priest | Aspersio, B.S. Sacramenti, Magnus Exorcismus, Turn Undead |
| Monk | Raging Trifecta Blow, Excruciating Palm |

### 7.3 Skills That CANNOT Be Copied

- All monster-exclusive skills (NPC_ prefix)
- All Transcendent class skills (Lord Knight, Assassin Cross, High Wizard, etc.)
- Non-damaging skills (buffs, heals) -- EXCEPT Aspersio
- Skills requiring weapon types Rogue cannot equip:
  - Sonic Blow / Grimtooth (need Katar -- Rogue cannot equip)
  - Brandish Spear / Pierce / Spear Boomerang / Spear Stab (need Spear)
  - Cart Revolution (need Cart)
- Blitz Beat (falcon mechanic)
- Bard/Dancer performance skills
- Monk combo skills (most, except Trifecta/Excruciating)
- All Ninja and Gunslinger skills

### 7.4 Weapon Requirement Validation

When using a copied skill, weapon requirements still apply:
- Copied Bowling Bash: needs 1H/2H Sword -> Rogue CAN equip -> OK
- Copied Sonic Blow: needs Katar -> Rogue CANNOT equip -> `skill:error`
- Copied Double Strafe: needs Bow -> Rogue CAN equip -> OK

### 7.5 Copy Trigger Implementation

```
When player with Plagiarism learned takes skill damage:
1. Check PLAGIARISM_COPYABLE_SKILLS whitelist
2. If skill is copyable:
   a. usableLevel = min(incomingSkillLevel, plagiarismLevel)
   b. Store: { skillId, usableLevel, copiedAt }
   c. Emit plagiarism:skill_copied to client
3. New copy overwrites any existing copied skill
```

### 7.6 DB Persistence

```sql
ALTER TABLE characters ADD COLUMN plagiarized_skill_id INTEGER DEFAULT NULL;
ALTER TABLE characters ADD COLUMN plagiarized_skill_level INTEGER DEFAULT 0;
```

---

## 8. Strip/Divest System

### 8.1 Success Rate Formula (rAthena official)

```
rate(per 1000) = 50 * (skillLevel + 1) + 2 * (casterDEX - targetDEX)
rate(%) = rate / 10
```

| Level | Base % (equal DEX) |
|-------|-------------------|
| 1 | 10% |
| 2 | 15% |
| 3 | 20% |
| 4 | 25% |
| 5 | 30% |

### 8.2 Duration Formula (rAthena official)

```
duration(ms) = 60000 + 15000 * skillLevel + 500 * (casterDEX - targetDEX)
minimum: 5000ms
```

| Level | Base Duration (equal DEX) |
|-------|--------------------------|
| 1 | 75s (1:15) |
| 2 | 90s (1:30) |
| 3 | 105s (1:45) |
| 4 | 120s (2:00) |
| 5 | 135s (2:15) |

### 8.3 Effects on Monsters

| Strip Type | Monster Effect |
|------------|----------------|
| Strip Weapon (1713) | ATK reduced by 25% |
| Strip Shield (1711) | Hard DEF reduced by 15% |
| Strip Armor (1712) | VIT reduced by 40% (affects soft DEF) |
| Strip Helm (1710) | INT reduced by 40% (affects soft MDEF) |

### 8.4 Effects on Players (PvP)

| Strip Type | Player Effect |
|------------|---------------|
| Strip Weapon | Weapon forcibly unequipped, cannot re-equip for duration |
| Strip Shield | Shield forcibly unequipped, cannot re-equip for duration |
| Strip Armor | Armor forcibly unequipped, cannot re-equip; target becomes Neutral element |
| Strip Helm | Headgear forcibly unequipped, cannot re-equip for duration |

### 8.5 Prevention

- **Chemical Protection** (Alchemist skill): Completely blocks all strip attempts
- **Full Chemical Protection** (Creator/Biochemist): Blocks all four slots at once

### 8.6 Common Properties

All four Divest skills share:
- Cast Time: 1.0 seconds (interruptible by damage)
- After-Cast Delay: 1.0 seconds
- Range: Melee (1 cell)
- Target: Single enemy or player
- Max Level: 5

---

## 9. Steal System

### 9.1 Overall Success Formula

```
overallChance(%) = BaseSuccessRate + (casterDEX - monsterDEX) / 2
BaseSuccessRate = 4 + 6 * skillLv
```

| Level | BaseSuccessRate |
|-------|----------------|
| 1 | 10% |
| 2 | 16% |
| 3 | 22% |
| 4 | 28% |
| 5 | 34% |
| 6 | 40% |
| 7 | 46% |
| 8 | 52% |
| 9 | 58% |
| 10 | 64% |

### 9.2 Item Selection Formula

```
AdjustedDropRatio = DropRatio * (DEX + 3*SkillLv - MonsterDEX + 10) / 100
```

- Each monster has up to 8 items in their drop table
- Iterate top to bottom through the drop list
- For each item: roll against its `AdjustedDropRatio`
- First item that passes is stolen
- If no item passes, steal "succeeds" but gives nothing

### 9.3 Key Constraints

- Boss protocol monsters: CANNOT be stolen from
- One successful steal per monster (tracked by `enemy.stolenBy`)
- Stolen item does NOT reduce death drops
- Stolen item goes to inventory (weight check applies)
- If overweight: steal succeeds but item is lost
- Snatcher (Rogue passive) uses the same formula/tracker

---

## 10. Close Confine Mechanics

### 10.1 Core Effect

- Locks BOTH caster and target in place (neither can move)
- Both can still attack, use skills, and use items
- Caster gains +10 FLEE during the lock
- Duration: 10 seconds
- SP Cost: 25
- Cannot target Boss monsters

### 10.2 Break Conditions

1. Duration expires (10 seconds)
2. Either caster or target dies
3. Either party teleports (Fly Wing, Butterfly Wing, Teleport skill)
4. Target enters Hiding status
5. Knockback occurs to either party (3+ cells separation)
6. Caster or target uses Back Slide
7. Going beyond 2 cells from each other breaks the confine

### 10.3 Implementation

- Apply `close_confine` buff to BOTH caster and target
- Buff includes `isConfiner: true/false` to distinguish initiator
- In `player:position` handler: reject movement for both parties
- Caster gets `bonusFLEE += 10` via buff modifiers
- On any break condition: remove buff from both, broadcast `skill:buff_removed`
- Check `enemy.modeFlags?.boss` before applying

---

## 11. Skill Trees and Prerequisites

### 11.1 Thief Skill Tree

```
No prerequisites:
  Double Attack (500)
  Improve Dodge (501)

Steal (502) -----> Hiding (503) [prereq: Steal Lv5]
                   |
Envenom (504) --> Detoxify (505) [prereq: Envenom Lv3]

Quest Skills (learned from NPC):
  Sand Attack (506) [Job Lv25]
  Back Slide (507)  [Job Lv35]
  Pick Stone (508)  [Job Lv15]
  Throw Stone (509) [Job Lv20]
```

**Skill Points**: 49 total (Job Lv1-49)
**Class Change Requirement**: Base Lv 10 (to become Thief from Novice)

### 11.2 Assassin Skill Tree

```
No prerequisites:
  Righthand Mastery (1107)
  Katar Mastery (1100)

Righthand Mastery (1107) Lv2 -----> Lefthand Mastery (1108)

Katar Mastery (1100) Lv4 -----> Sonic Blow (1101) Lv5 ---+---> Grimtooth (1102) [+Cloaking Lv2]
                                                          |
Hiding (503) Lv2 [Thief] -----> Cloaking (1103) Lv2 -----+

Envenom (504) Lv1 [Thief] -----> Enchant Poison (1109) Lv3 -----> Poison React (1104) Lv3/5 ---+
                                  Enchant Poison (1109) Lv5 ----+                               |
                                                                +-> Venom Dust (1105) Lv5 ------+
                                                                                                |
                                                                                     Venom Splasher (1110)

Quest Skills:
  Sonic Acceleration (1106) [Job Lv30]
  Throw Venom Knife (1111)  [Assassin Skill Quest]
```

**Skill Points**: 49 total (Job Lv1-49)
**Class Change**: Thief Base Lv 40+, Job Lv 40+

### 11.3 Rogue Skill Tree

```
Steal (502) Lv1 -----> Snatcher (1700) Lv4 -----> Steal Coin (1709) Lv2/4 -+
                                                                             |
Hiding (503) Lv1 ----> Tunnel Drive (1702) Lv2 -+                           |
                                                 |                           +-> Divest Helm (1710) Lv5
Steal Coin (1709) Lv4 -+                         |                                |
Back Stab (1701) Lv4 --+-> Back Stab (1701) Lv2 -+-> Raid (1703) Lv5 ---+        +-> Divest Shield (1711) Lv5
                                                  |                       |             |
                                                  +-> Raid (1703) Lv1 ---+             +-> Divest Armor (1712) Lv5
                                                       |                  |                  |
                                                       +-> Gangster's    +-> Intimidate     +-> Divest Weapon (1713)
                                                           Paradise          (1704) Lv5
                                                           (1715) Lv1        |
                                                           |                 +-> Plagiarism (1714)
                                                           +-> Compulsion
                                                               Discount (1716)

No prerequisites:
  Sword Mastery (1705)
  Vulture's Eye (1706) Lv10 -----> Double Strafe (1707) Lv5 -----> Remove Trap (1708)

Quest Skills:
  Scribble (1717) [Quest]
  Close Confine (1718) [Platinum Skills Quest]
```

**Skill Points**: 49 total (Job Lv1-49)
**Class Change**: Thief Base Lv 40+, Job Lv 40+

---

## 12. Implementation Checklist

### 12.1 Thief Skills

| ID | Skill | Handler | Passive | Buff | Ground | Status |
|----|-------|---------|---------|------|--------|--------|
| 500 | Double Attack | N/A (auto-attack proc) | Yes | No | No | IMPLEMENTED |
| 501 | Improve Dodge | N/A | Yes | No | No | IMPLEMENTED |
| 502 | Steal | Yes | No | No | No | IMPLEMENTED (gaps: item acquisition, formula) |
| 503 | Hiding | Yes (toggle) | No | Yes | No | IMPLEMENTED (gaps: SP drain, movement block) |
| 504 | Envenom | Yes (custom) | No | No | No | IMPLEMENTED (gaps: damage formula) |
| 505 | Detoxify | Yes | No | No | No | IMPLEMENTED |
| 506 | Sand Attack | Yes | No | No | No | IMPLEMENTED |
| 507 | Back Slide | Yes | No | No | No | IMPLEMENTED |
| 508 | Pick Stone | Yes | No | No | No | STUB (no item) |
| 509 | Throw Stone | Yes | No | No | No | STUB (no item consumption) |

### 12.2 Assassin Skills

| ID | Skill | Handler | Passive | Buff | Ground | Status |
|----|-------|---------|---------|------|--------|--------|
| 1100 | Katar Mastery | N/A | Yes | No | No | IMPLEMENTED |
| 1101 | Sonic Blow | Yes | No | No | No | IMPLEMENTED (formula corrected) |
| 1102 | Grimtooth | Yes | No | No | No | IMPLEMENTED |
| 1103 | Cloaking | Yes (toggle) | No | Yes | No | IMPLEMENTED |
| 1104 | Poison React | Yes | No | Yes | No | IMPLEMENTED |
| 1105 | Venom Dust | Yes | No | No | Yes | IMPLEMENTED |
| 1106 | Sonic Acceleration | N/A | Yes | No | No | IMPLEMENTED (read in SB handler) |
| 1107 | Righthand Mastery | N/A | Yes | No | No | IMPLEMENTED |
| 1108 | Lefthand Mastery | N/A | Yes | No | No | IMPLEMENTED |
| 1109 | Enchant Poison | Yes | No | Yes | No | IMPLEMENTED |
| 1110 | Venom Splasher | Yes | No | No | No | IMPLEMENTED |
| 1111 | Throw Venom Knife | Yes | No | No | No | IMPLEMENTED |

### 12.3 Rogue Skills

| ID | Skill | Handler | Passive | Buff | Ground | Status |
|----|-------|---------|---------|------|--------|--------|
| 1700 | Snatcher | N/A (auto-attack hook) | Yes | No | No | IMPLEMENTED |
| 1701 | Back Stab | Yes | No | No | No | IMPLEMENTED |
| 1702 | Tunnel Drive | N/A | Yes | No | No | IMPLEMENTED |
| 1703 | Raid | Yes | No | No | No | IMPLEMENTED |
| 1704 | Intimidate | Yes | No | No | No | IMPLEMENTED |
| 1705 | Sword Mastery (R) | N/A | Yes | No | No | IMPLEMENTED |
| 1706 | Vulture's Eye (R) | N/A | Yes | No | No | IMPLEMENTED |
| 1707 | Double Strafe (R) | Yes (shared handler) | No | No | No | IMPLEMENTED |
| 1708 | Remove Trap (R) | Yes | No | No | No | DEFERRED (needs trap system) |
| 1709 | Steal Coin | Yes | No | No | No | IMPLEMENTED |
| 1710 | Divest Helm | Yes | No | Yes (debuff) | No | IMPLEMENTED |
| 1711 | Divest Shield | Yes | No | Yes (debuff) | No | IMPLEMENTED |
| 1712 | Divest Armor | Yes | No | Yes (debuff) | No | IMPLEMENTED |
| 1713 | Divest Weapon | Yes | No | Yes (debuff) | No | IMPLEMENTED |
| 1714 | Plagiarism | N/A (damage hook) | Yes | No | No | IMPLEMENTED |
| 1715 | Gangster's Paradise | N/A | Yes | No | No | DEFERRED (sitting + proximity) |
| 1716 | Compulsion Discount | N/A | Yes | No | No | IMPLEMENTED |
| 1717 | Scribble | Yes | No | No | Yes | DEFERRED (cosmetic) |
| 1718 | Close Confine | Yes | No | Yes (both) | No | IMPLEMENTED |

### 12.4 Assassin Cross Skills (Transcendent -- Future Phase)

| Skill | rAthena ID | Type | Notes |
|-------|-----------|------|-------|
| Advanced Katar Mastery | 376 | Passive | +12-20% Katar ATK% |
| Create Deadly Poison | 407 | Active (Crafting) | 7 materials, DEX+LUK formula |
| Enchant Deadly Poison (EDP) | 378 | Self-Buff | 2x-4x weapon ATK, Poison Bottle catalyst |
| Soul Destroyer (Soul Breaker) | 379 | Offensive, Ranged | Hybrid phys+INT damage, unique formula |
| Meteor Assault | 406 | Self-AoE | 5x5, Stun/Blind/Bleed, EDP immune |

### 12.5 Systems Required

| System | Skills That Use It | Status |
|--------|-------------------|--------|
| Hiding/Cloaking toggle | 503, 1103 | IMPLEMENTED |
| SP drain tick | 503, 1103 | IMPLEMENTED |
| Steal item acquisition | 502, 1700 | IMPLEMENTED |
| Ground effect (Venom Dust) | 1105 | IMPLEMENTED |
| Delayed detonation (Venom Splasher) | 1110 | IMPLEMENTED |
| Plagiarism copy hook | 1714 | IMPLEMENTED |
| Strip/Divest debuff system | 1710-1713 | IMPLEMENTED |
| Close Confine movement lock | 1718 | IMPLEMENTED |
| Dual wield combat | 1107, 1108 | IMPLEMENTED |
| Katar CRI doubling | 1100 | IMPLEMENTED |
| Poison React reactive hook | 1104 | IMPLEMENTED |
| Sitting state (Gangster's Paradise) | 1715 | IMPLEMENTED (basic sitting) |
| Remove Trap | 1708 | DEFERRED (needs trap system) |

---

## 13. Gap Analysis

### 13.1 Resolved Issues (Previously Identified, Now Fixed)

| Issue | Skill | Resolution |
|-------|-------|------------|
| Sonic Blow formula wrong (440+40*Lv) | 1101 | Fixed to 300+50*Lv (rAthena verified) |
| Enchant Poison endow overwrite wrong buff names | 1109 | Fixed endow buff name mismatch |
| Katar CRI doubling not implemented | 1100 | Implemented: `cri *= 2` with Katar |
| Envenom damage formula wrong (skill multiplier vs flat bonus) | 504 | Fixed: 100% ATK + 15*Lv flat (DEF-bypassing) |
| Steal gives no items | 502 | Implemented: monster drop table selection |
| Hiding has no SP drain | 503 | Implemented: periodic SP drain tick |
| Close Confine break conditions incomplete | 1718 | 8 break conditions implemented |
| Plagiarism copy from monster skills | 1714 | `checkPlagiarismCopy()` in `executeMonsterPlayerSkill()` |

### 13.2 Remaining Deferred Items

| Item | Priority | Reason |
|------|----------|--------|
| Remove Trap (1708) | LOW | Requires Hunter trap system to exist first |
| Gangster's Paradise (1715) | LOW | Needs sitting proximity detection in AI tick |
| Scribble (1717) | LOWEST | Purely cosmetic, no gameplay impact |
| Cloaking wall adjacency (Lv1-2) | LOW | Server-side collision detection complexity; high SP drain at Lv1-2 naturally limits |
| Cloaking movement speed modifiers | LOW | Server does not enforce movement speed (trusted client position) |
| Back Stab facing direction check | LOW | PvE simplified (monsters don't have meaningful facing); PvP would need facing system |
| Improve Dodge 2nd class scaling (+4/lv) | MEDIUM | Apply when Assassin/Rogue is detected |
| Improve Dodge Assassin MSPD bonus | LOW | Client-side only, minimal impact |
| Assassin Cross skills (5 skills) | FUTURE | Transcendent class phase |

### 13.3 Cross-System Dependencies

| Dependency | Skills Affected | Status |
|------------|----------------|--------|
| Monster drop tables in `ro_monster_templates.js` | Steal (502), Snatcher (1700) | Available |
| Red Gemstone item tracking | Venom Dust (1105), Venom Splasher (1110), Scribble (1717) | Available |
| Stone item (ID 7049) | Pick Stone (508), Throw Stone (509) | Available |
| Venom Knife ammunition | Throw Venom Knife (1111) | Available |
| Poison Bottle item | EDP (future) | Available |
| Hunter trap system | Remove Trap (1708) | NOT YET IMPLEMENTED |
| Chemical Protection (Alchemist) | Divest (1710-1713) block | IMPLEMENTED |
| Sitting state tracking | Gangster's Paradise (1715) | IMPLEMENTED |
| Buff system (`ro_buff_system.js`) | Cloaking, Enchant Poison, Poison React, Close Confine, Divest debuffs | IMPLEMENTED |

### 13.4 Accuracy Notes

All formulas in this document have been cross-referenced against:
1. **rAthena pre-renewal source code** (authoritative -- `src/map/skill.cpp`, `src/map/battle.cpp`, `src/map/status.cpp`)
2. **iRO Wiki Classic** (community-maintained, generally accurate)
3. **RateMyServer Pre-RE DB** (reliable for pre-renewal data)
4. **Hercules pre-re source** (secondary verification)
5. **WarpPortal community forums** (for edge cases and player-verified behavior)
6. **DarkHikari's guides** (detailed Rogue/Plagiarism reference)

Where sources conflict, rAthena pre-renewal source code is treated as authoritative, with iRO Wiki Classic as secondary reference.
