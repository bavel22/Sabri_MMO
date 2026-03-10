# 01 - Stats, Leveling, and Job/Class System

> **Scope**: Ragnarok Online Classic (pre-Renewal) mechanics only.
> **Sources**: iRO Wiki Classic, RateMyServer (pre-re), rAthena source (pre-re branch), Ragnarok Wiki (Fandom).
> **Purpose**: Authoritative reference for implementing the stat, leveling, and class systems in Sabri_MMO.

---

## Table of Contents

1. [Base Stats (STR/AGI/VIT/INT/DEX/LUK)](#1-base-stats)
2. [Derived Stats](#2-derived-stats)
3. [Leveling System](#3-leveling-system)
4. [Job/Class System](#4-jobclass-system)
5. [DB Schema (PostgreSQL)](#5-db-schema-postgresql)
6. [UE5 C++ Implementation](#6-ue5-c-implementation)

---

## 1. Base Stats

Every character has six primary stats. All start at 1 (minimum) and can be raised to a maximum of 99. Every calculation in RO uses integer math -- all intermediate results are **floored** (truncated) immediately. Decimals do not exist in-game.

### 1.1 Stat Point Allocation

#### Points Gained Per Base Level

When a character gains a base level, the number of stat points awarded is:

```
StatPointsGained = floor(BaseLevelGained / 5) + 3
```

| Level Range | Points Per Level |
|------------|-----------------|
| 1-4        | 3               |
| 5-9        | 4               |
| 10-14      | 5               |
| 15-19      | 6               |
| 20-24      | 7               |
| 25-29      | 8               |
| 30-34      | 9               |
| 35-39      | 10              |
| 40-44      | 11              |
| 45-49      | 12              |
| 50-54      | 13              |
| 55-59      | 14              |
| 60-64      | 15              |
| 65-69      | 16              |
| 70-74      | 17              |
| 75-79      | 18              |
| 80-84      | 19              |
| 85-89      | 20              |
| 90-94      | 21              |
| 95-98      | 22              |

**Total stat points from levels 1-99**: 1,225
**Character creation bonus**: 48 points (6 stats start at 1, effectively 48 distributable points on top)
**Grand total**: 1,273 stat points for a normal class character at level 99
**Transcendent (High) Novice**: starts with 100 stat points (instead of 48)

#### Cost to Raise a Stat

Raising a stat from value `x` to `x+1` costs:

```
Cost = floor((x - 1) / 10) + 2
```

| Stat Range | Cost Per Point |
|-----------|---------------|
| 1 -> 2 through 10 -> 11  | 2  |
| 11 -> 12 through 20 -> 21 | 3  |
| 21 -> 22 through 30 -> 31 | 4  |
| 31 -> 32 through 40 -> 41 | 5  |
| 41 -> 42 through 50 -> 51 | 6  |
| 51 -> 52 through 60 -> 61 | 7  |
| 61 -> 62 through 70 -> 71 | 8  |
| 71 -> 72 through 80 -> 81 | 9  |
| 81 -> 82 through 90 -> 91 | 10 |
| 91 -> 92 through 98 -> 99 | 11 |

**Total cost to max one stat (1 to 99)**: 628 stat points
**Maximum stats achievable**: You cannot max all 6 stats. With 1,273 points you can max ~2 stats with some left over.

#### Cumulative Cost Table (1 to Target)

| Target | Cumulative Cost |
|--------|----------------|
| 10     | 18             |
| 20     | 48             |
| 30     | 88             |
| 40     | 138            |
| 50     | 198            |
| 60     | 268            |
| 70     | 348            |
| 80     | 438            |
| 90     | 538            |
| 99     | 628            |

---

### 1.2 STR (Strength)

**Primary effects:**
- Melee StatusATK: +1 per STR
- Melee StatusATK bonus: every 10 STR grants `floor(STR/10)^2` additional StatusATK (the "STR bonus")
- Ranged StatusATK: +1 per 5 STR (i.e., `floor(STR/5)`)
- Weight Limit: +30 per STR
- Weapon stat bonus (melee): `floor(BaseWeaponDamage * STR / 200)`

**STR bonus table (every 10 STR milestone):**

| STR | Bonus ATK |
|-----|-----------|
| 10  | 1         |
| 20  | 4         |
| 30  | 9         |
| 40  | 16        |
| 50  | 25        |
| 60  | 36        |
| 70  | 49        |
| 80  | 64        |
| 90  | 81        |
| 99  | 81 (floor(99/10)^2 = 9^2 = 81) |

---

### 1.3 AGI (Agility)

**Primary effects:**
- Flee Rate: +1 per AGI
- ASPD: reduces attack delay (see ASPD formula in Section 2)
- No direct effect on ATK, DEF, or MATK

AGI's ASPD contribution varies by class and weapon. Roughly, 4-10 AGI = 1 ASPD point depending on context.

---

### 1.4 VIT (Vitality)

**Primary effects:**
- Max HP: +1% of Base HP per VIT (multiplicative: `BaseHP * (1 + VIT * 0.01)`)
- Soft DEF: `floor(VIT/2)` (contributes to SoftDEF formula)
- Soft MDEF: contributes `floor(VIT/5)` to SoftMDEF
- Natural HP Regeneration: `floor(MaxHP / 200) + floor(VIT / 5)`
- Healing item effectiveness: +2% per VIT
- Status resistance: higher VIT reduces duration of Stun, Poison, etc.

---

### 1.5 INT (Intelligence)

**Primary effects:**
- MATK Maximum: `INT + floor(INT/5)^2`
- MATK Minimum: `INT + floor(INT/7)^2`
- Max SP: +1% of Base SP per INT (multiplicative: `BaseSP * (1 + INT * 0.01)`)
- Soft MDEF: +1 per INT (primary contributor)
- Natural SP Regeneration: `floor(MaxSP / 100) + floor(INT / 6) + 1`
- Healing item SP effectiveness: +1% per INT

**INT MATK bonus table:**

| INT | Min MATK | Max MATK |
|-----|----------|----------|
| 10  | 11       | 14       |
| 20  | 28       | 36       |
| 30  | 48       | 66       |
| 40  | 73       | 104      |
| 50  | 101      | 150      |
| 60  | 133      | 204      |
| 70  | 170      | 266      |
| 80  | 211      | 336      |
| 90  | 256      | 414      |
| 99  | 299      | 491      |

Formula derivation:
- Min MATK = `INT + floor(INT / 7)^2`
- Max MATK = `INT + floor(INT / 5)^2`

---

### 1.6 DEX (Dexterity)

**Primary effects:**
- HIT: +1 per DEX
- Cast time reduction: `CastTime = BaseCastTime * (1 - DEX / 150)`
  - At 150 DEX: instant cast (0 cast time)
- Ranged StatusATK: +1 per DEX (primary stat for ranged weapons)
- Melee StatusATK: +1 per 5 DEX (i.e., `floor(DEX/5)`)
- Minimum damage stabilization: higher DEX narrows damage variance
- Weapon stat bonus (ranged): `floor(BaseWeaponDamage * DEX / 200)`
- ASPD: minor contribution (see ASPD formula)

**DEX bonus for ranged (every 10 DEX milestone):**
Same structure as STR bonus: `floor(DEX/10)^2`

---

### 1.7 LUK (Luck)

**Primary effects:**
- Critical Rate: `floor(LUK * 0.3) + 1` percent (base 1% crit for all characters)
- Perfect Dodge: `floor(LUK / 10)` (B component of Flee display)
- StatusATK: +1 per 3 LUK (`floor(LUK/3)`)
- HIT: +1 per 3 LUK (`floor(LUK/3)`) -- note: some sources only list DEX for HIT
- Flee: +1 per 5 LUK (`floor(LUK/5)`)
- Crit Shield (target's defense against crits): every 5 LUK the target has reduces attacker's effective crit by 1%

---

## 2. Derived Stats

### 2.1 ATK (Physical Attack)

ATK is displayed in the status window as `A + B` where:
- **A** = StatusATK (stat-based, always Neutral element unless using Mild Wind)
- **B** = WeaponATK + ExtraATK (affected by element, size, race modifiers)

#### StatusATK

**For melee weapons** (Daggers, Swords, Maces, Axes, Spears, Knuckles, Katars, Books, Bare Hand):
```
StatusATK = floor(BaseLevel / 4) + STR + floor(DEX / 5) + floor(LUK / 3)
```

**For ranged weapons** (Bows, Guns, Instruments, Whips):
```
StatusATK = floor(BaseLevel / 4) + floor(STR / 5) + DEX + floor(LUK / 3)
```

#### WeaponATK

```
WeaponATK = (BaseWeaponDamage + Variance + StatBonus + RefinementBonus + OverUpgradeBonus) * SizePenalty
```

Where:
- **Variance**: `random(0, floor(0.05 * WeaponLevel * BaseWeaponDamage) * 2)` centered on BaseWeaponDamage
  - Effectively: `BaseWeaponDamage +/- floor(0.05 * WeaponLevel * BaseWeaponDamage)`
- **StatBonus (melee)**: `floor(BaseWeaponDamage * STR / 200)`
- **StatBonus (ranged)**: `floor(BaseWeaponDamage * DEX / 200)`
- **RefinementBonus**: See refinement table below
- **OverUpgradeBonus**: See refinement table below
- **SizePenalty**: Depends on weapon type vs monster size (see Size Penalty table)

#### Refinement Bonus

| Weapon Level | ATK Per +1 | Safety Limit | Over-Upgrade Bonus (random) |
|-------------|-----------|-------------|---------------------------|
| Level 1     | +2 ATK    | +7          | 0 ~ 3 ATK per over-upgrade |
| Level 2     | +3 ATK    | +6          | 0 ~ 5 ATK per over-upgrade |
| Level 3     | +5 ATK    | +5          | 0 ~ 8 ATK per over-upgrade |
| Level 4     | +7 ATK    | +4          | 0 ~ 13 ATK per over-upgrade |

Armor refinement: +1 Hard DEF per +1, safety limit +4 for all armor.

#### Size Penalty Table

Damage modifier applied to WeaponATK only (not StatusATK):

| Weapon Type         | Small | Medium | Large |
|--------------------|-------|--------|-------|
| Bare Fist          | 100%  | 100%   | 100%  |
| Dagger             | 100%  | 75%    | 50%   |
| One-handed Sword   | 75%   | 100%   | 75%   |
| Two-handed Sword   | 75%   | 75%    | 100%  |
| One-handed Spear   | 75%   | 75%    | 100%  |
| Two-handed Spear   | 75%   | 75%    | 100%  |
| One-handed Axe     | 50%   | 75%    | 100%  |
| Two-handed Axe     | 50%   | 75%    | 100%  |
| Mace               | 75%   | 100%   | 100%  |
| Staff/Rod          | 100%  | 100%   | 100%  |
| Bow                | 100%  | 100%   | 75%   |
| Knuckle/Claw       | 100%  | 75%    | 50%   |
| Musical Instrument | 75%   | 100%   | 75%   |
| Whip               | 75%   | 100%   | 50%   |
| Book               | 100%  | 100%   | 50%   |
| Katar              | 75%   | 100%   | 75%   |

#### Total Physical Damage Formula

```
Damage = {StatusATK * 2 + WeaponATK + ExtraATK} * ElementModifier * SkillModifier
Damage = ApplyHardDEF(Damage)
Damage = Damage - SoftDEF
Damage = max(1, Damage)
```

For skills, the `SkillModifier` is the skill's damage percentage (e.g., Bash Lv10 = 400%).

#### Critical Hits

- Critical rate: `floor(LUK * 0.3) + 1 + BonusCrit`
- Target's crit resistance: `floor(TargetLUK / 5)`
- Effective crit: `CritRate - TargetCritShield`
- **Critical hits always deal maximum WeaponATK** (no variance, always max damage)
- **Critical hits ignore Flee** (guaranteed hit)
- **Critical hits ignore Hard DEF and Soft DEF** -- full damage through defense
- **Critical hits do NOT ignore Perfect Dodge** (Lucky Dodge can still avoid crits)
- **Critical damage bonus**: +40% damage

---

### 2.2 MATK (Magic Attack)

```
MATK_Min = INT + floor(INT / 7)^2
MATK_Max = INT + floor(INT / 5)^2
```

Actual magic damage per hit is:
```
random(MATK_Min, MATK_Max) * SkillModifier * ElementModifier
```

**StatusMATK** (full formula):
```
StatusMATK = floor(BaseLevel / 4) + INT + floor(INT / 2) + floor(DEX / 5) + floor(LUK / 3)
```

Note: The `floor(INT/5)^2` and `floor(INT/7)^2` formulas apply to the variance range of raw MATK. StatusMATK is the base upon which equipment and buffs add.

---

### 2.3 DEF (Physical Defense)

DEF is displayed as `A + B`:
- **A** = Hard DEF (equipment-based, percentage reduction)
- **B** = Soft DEF (stat-based, flat subtraction)

#### Hard DEF

```
DamageAfterHardDEF = Damage * (4000 + HardDEF) / (4000 + HardDEF * 10)
```

| Hard DEF | Reduction |
|----------|-----------|
| 10       | ~2.4%     |
| 50       | ~11.1%    |
| 100      | ~20%      |
| 200      | ~33.3%    |
| 275      | ~40%      |
| 500      | ~55.6%    |

#### Soft DEF

```
SoftDEF = floor(VIT / 2) + floor(AGI / 5) + floor(BaseLevel / 2)
TotalSoftDEF = (SoftDEF + AdditiveBonuses) * (1 + MultiplicativeBonuses / 100)
```

**Application order**: Hard DEF first (percentage), then Soft DEF (flat subtraction).

---

### 2.4 MDEF (Magic Defense)

MDEF is also displayed as `A + B`:
- **A** = Hard MDEF (equipment-based)
- **B** = Soft MDEF (stat-based)

#### Hard MDEF

```
DamageAfterHardMDEF = Damage * (1000 + HardMDEF) / (1000 + HardMDEF * 10)
```

#### Soft MDEF

```
SoftMDEF = INT + floor(VIT / 5) + floor(DEX / 5) + floor(BaseLevel / 4)
TotalSoftMDEF = (SoftMDEF + AdditiveBonuses) * (1 + MultiplicativeBonuses / 100)
```

**Application order**: Hard MDEF first (percentage), then Soft MDEF (flat subtraction).

---

### 2.5 HIT (Accuracy)

```
HIT = BaseLevel + DEX + BonusHIT
```

Some sources also add `floor(LUK/3)` to HIT.

**Hit chance against target**:
```
HitRate = 80 + HIT - TargetFlee
HitRate = clamp(HitRate, 5, 100)  // minimum 5% hit, maximum 100%
```

---

### 2.6 FLEE (Evasion)

Displayed as `A + B`:
- **A** = Base Flee = `BaseLevel + AGI + BonusFlee`
- **B** = Perfect Dodge = `floor(LUK / 10) + BonusPerfectDodge`

**Dodge chance**:
```
DodgeRate = FleeA - AttackerHIT + 80
DodgeRate = clamp(DodgeRate, 5, 95)  // maximum 95% dodge
```

**Perfect Dodge**: Checked separately. If the 95% flee check fails, Perfect Dodge still has a chance to avoid the hit. Perfect Dodge even works against Critical Hits.

**Multi-hit penalty**: When being attacked by multiple monsters simultaneously, Flee drops. Each additional attacker beyond the first reduces effective Flee:
```
EffectiveFlee = Flee - max(0, (NumberOfAttackers - 1) * 10)
```

---

### 2.7 ASPD (Attack Speed)

Pre-renewal ASPD ranges from 0 to 190. Higher is faster. 190 is the hard cap (5 attacks per second).

#### Formula

```
ASPD = 200 - (WeaponDelay - floor((WeaponDelay * AGI / 25) + (WeaponDelay * DEX / 100)) / 10) * (1 - SpeedModifier)
```

Where:
- **WeaponDelay** = `50 * BTBA` (Base Time Between Attacks in seconds, class+weapon dependent)
- **SpeedModifier** = sum of all ASPD% bonuses (Berserk Potion = 0.20, Two-Hand Quicken = 0.30, etc.)

#### Delay to Attacks-Per-Second

```
AttackDelay_ms = (200 - ASPD) * 10   // in milliseconds
AttacksPerSecond = 1000 / AttackDelay_ms
```

| ASPD | Delay (ms) | Attacks/sec |
|------|-----------|-------------|
| 150  | 500       | 2.0         |
| 160  | 400       | 2.5         |
| 170  | 300       | 3.3         |
| 180  | 200       | 5.0         |
| 185  | 150       | 6.7         |
| 190  | 100       | 10.0 (cap)  |

Note: The game hard-caps at 190 ASPD. Some sources say the effective cap is 5 hits/second (which would be 180 ASPD), as 190 may not be achievable in pre-renewal.

#### Base Weapon Delay Table (BTBA in seconds)

Approximate values from early RO data. These vary by class and weapon:

| Class     | Bare Hand | Dagger | 1H Sword | 2H Sword | Spear | Mace | Axe  | Bow  | Staff |
|-----------|-----------|--------|----------|----------|-------|------|------|------|-------|
| Novice    | 0.50      | 0.55   | --       | --       | --    | --   | --   | --   | --    |
| Swordman  | 0.40      | 0.65   | 0.70     | 0.60     | 0.65  | 0.70 | 0.80 | --   | --    |
| Mage      | 0.35      | --     | --       | --       | --    | --   | --   | --   | 0.65  |
| Archer    | 0.50      | --     | --       | --       | --    | --   | --   | 0.70 | --    |
| Thief     | 0.40      | 0.50   | 0.70     | --       | --    | --   | --   | 0.85 | --    |
| Merchant  | 0.40      | 0.65   | 0.55     | --       | --    | 0.65 | 0.70 | --   | --    |
| Acolyte   | 0.40      | 0.60   | 0.70     | --       | --    | 0.70 | --   | --   | --    |

Higher-tier classes (Knight, Wizard, etc.) have their own base delays, generally faster than their first-class counterparts.

**Shield penalty**: Equipping a shield increases BTBA by approximately 5-10% depending on class.

#### Common ASPD Modifiers

| Source              | Modifier |
|--------------------|----------|
| Concentration Potion | +0.10   |
| Awakening Potion    | +0.15   |
| Berserk Potion      | +0.20   |
| Two-Hand Quicken    | +0.30   |
| Adrenaline Rush     | +0.30   |
| Berserk (LK)        | sets to 190 |

---

### 2.8 Max HP

#### Formula

```
// Step 1: Calculate Base HP from level and class
BaseHP = 35 + (BaseLevel * HP_JOB_B)
for i = 2 to BaseLevel:
    BaseHP += round(HP_JOB_A * i)

// Step 2: Apply VIT and transcendent modifier
MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod)

// Step 3: Apply additive then multiplicative bonuses
MaxHP += AdditiveHPBonuses
MaxHP = floor(MaxHP * (1 + MultiplicativeHPBonuses * 0.01))
```

Where:
- **TransMod** = 1.25 for transcendent classes, 1.0 otherwise
- **HP_JOB_A** = class-specific HP growth coefficient per level
- **HP_JOB_B** = class-specific base HP per level (default 5 if not specified)

#### HP Class Coefficients

| Class           | HP_JOB_A | HP_JOB_B | Notes |
|----------------|----------|----------|-------|
| Novice          | 0.0      | 5        |       |
| Super Novice    | 0.0      | 5        |       |
| Swordman        | 0.7      | 5        |       |
| Mage            | 0.3      | 5        |       |
| Archer          | 0.5      | 5        |       |
| Thief           | 0.5      | 5        |       |
| Merchant        | 0.4      | 5        |       |
| Acolyte         | 0.4      | 5        |       |
| Knight          | 1.5      | 5        |       |
| Wizard          | 0.55     | 5        |       |
| Hunter          | 0.85     | 5        |       |
| Assassin        | 1.1      | 5        |       |
| Blacksmith      | 0.9      | 5        |       |
| Priest          | 0.75     | 5        |       |
| Crusader        | 1.1      | 7        | HP_JOB_B = 7 |
| Sage            | 0.75     | 5        |       |
| Bard / Dancer   | 0.75     | 3        | HP_JOB_B = 3 |
| Rogue           | 0.85     | 5        |       |
| Monk            | 0.9      | 6.5      | HP_JOB_B = 6.5 |
| Alchemist       | 0.9      | 5        |       |

Transcendent classes use the same coefficients as their non-transcendent counterparts, but get the 1.25x TransMod multiplier.

#### Example HP Calculations

**Swordman, Level 50, 50 VIT**:
```
BaseHP = 35 + (50 * 5) = 285
For i=2..50: += round(0.7 * i) => sum of round(0.7*2) + round(0.7*3) + ... + round(0.7*50)
             = 1 + 2 + 3 + 4 + 4 + 4 + 5 + 6 + 6 + 7 + ... + 35 = ~893
BaseHP = 285 + 893 = 1178
MaxHP = floor(1178 * 1.50) = 1767
```

**Knight, Level 99, 80 VIT (Transcendent = Lord Knight)**:
```
BaseHP = 35 + (99 * 5) = 530
For i=2..99: += round(1.5 * i) => ~7,425
BaseHP = 530 + 7425 = 7955
MaxHP = floor(7955 * 1.80 * 1.25) = floor(17,899) = 17,899
```

---

### 2.9 Max SP

#### Formula

```
// Step 1: Calculate Base SP from level and class
BaseSP = 10 + (BaseLevel * SP_JOB_B)
for i = 2 to BaseLevel:
    BaseSP += round(SP_JOB_A * i)

// Step 2: Apply INT and transcendent modifier
MaxSP = floor(BaseSP * (1 + INT * 0.01) * TransMod)

// Step 3: Apply additive then multiplicative bonuses
MaxSP += AdditiveSPBonuses
MaxSP = floor(MaxSP * (1 + MultiplicativeSPBonuses * 0.01))
```

#### SP Class Coefficients

| Class           | SP_JOB_A | SP_JOB_B |
|----------------|----------|----------|
| Novice          | 0.0      | 2        |
| Super Novice    | 0.0      | 2        |
| Swordman        | 0.2      | 2        |
| Mage            | 0.6      | 2        |
| Archer          | 0.4      | 2        |
| Thief           | 0.3      | 2        |
| Merchant        | 0.3      | 2        |
| Acolyte         | 0.5      | 2        |
| Knight          | 0.4      | 2        |
| Wizard          | 1.0      | 2        |
| Hunter          | 0.6      | 2        |
| Assassin        | 0.5      | 2        |
| Blacksmith      | 0.5      | 2        |
| Priest          | 0.8      | 2        |
| Crusader        | 0.5      | 2        |
| Sage            | 0.8      | 2        |
| Bard / Dancer   | 0.6      | 2        |
| Rogue           | 0.5      | 2        |
| Monk            | 0.5      | 2        |
| Alchemist       | 0.5      | 2        |

---

### 2.10 Natural Regeneration

**HP Regeneration** (every 6 seconds while sitting/idle):
```
HPRegen = max(1, floor(MaxHP / 200)) + floor(VIT / 5)
```
Sitting doubles the rate. Overweight (>50%) halves it. Overweight (>90%) stops it.

**SP Regeneration** (every 8 seconds):
```
SPRegen = floor(MaxSP / 100) + floor(INT / 6) + 1
```
If INT >= 120: additional bonus of `floor(INT / 2) - 56`.
Sitting doubles the rate.

**Skill-based Regeneration** (every 10 seconds, from HP/SP Recovery skills):
```
HP Recovery Lv X: SkillHPRegen = X * 5 + floor(X * MaxHP / 500)
SP Recovery Lv X: SkillSPRegen = X * 3 + floor(X * MaxSP / 500)
```

---

### 2.11 Cast Time

Pre-renewal uses a single-component cast time (no fixed/variable split):

```
FinalCastTime = BaseCastTime * (1 - DEX / 150)
```

- At 150 DEX (or equivalent with bonuses): instant cast
- Each point of DEX reduces cast time by ~0.67%
- Equipment and skills can add flat cast time reduction percentages that stack additively
- Cast time is interrupted by taking damage or moving (> 5 UE units)

---

### 2.12 Weight Limit

```
MaxWeight = 2000 + STR * 30 + JobWeightBonus
```

#### Job Weight Bonuses

| Class           | Weight Bonus |
|----------------|-------------|
| Novice          | 0           |
| Swordman        | 800         |
| Mage            | 200         |
| Archer          | 600         |
| Thief           | 400         |
| Merchant        | 800         |
| Acolyte         | 400         |
| Knight          | 800         |
| Wizard          | 200         |
| Hunter          | 600         |
| Assassin        | 600         |
| Blacksmith      | 800         |
| Priest          | 600         |
| Crusader        | 800         |
| Sage            | 400         |
| Bard / Dancer   | 600         |
| Rogue           | 600         |
| Monk            | 600         |
| Alchemist       | 800         |
| Super Novice    | 0           |

**Weight thresholds**:
- 50% capacity: Normal, full regen
- 50-89% capacity: HP/SP regeneration halved
- 70%+ capacity: Cannot use item-creation skills
- 90%+ capacity: No attacks, no skills, no regen, movement slowed

---

## 3. Leveling System

### 3.1 Base Level

- **Range**: 1 to 99 (all classes in pre-renewal)
- **Effect**: Each base level grants stat points (formula above) and contributes to derived stats (HIT, Flee, DEF, MDEF)
- **Death penalty**: Lose 1% of current base EXP required for next level (cannot delevel)
- **Transcendent death penalty**: Lose 1% of current base EXP (same rate, but EXP tables are higher)

### 3.2 Job Level

- **Novice**: Job Level 1-10
- **First Class**: Job Level 1-50
- **Second Class**: Job Level 1-50
- **High Novice**: Job Level 1-10
- **High First Class**: Job Level 1-50
- **Transcendent Second Class**: Job Level 1-70
- **Super Novice**: Job Level 1-99

Each job level grants exactly **1 skill point**.

Job levels also grant hidden **Job Bonus Stats** -- small stat bonuses automatically applied per job level (class-specific, see Section 4).

### 3.3 Base EXP Table (Normal Classes, Levels 1-99)

| Level | EXP to Next | Level | EXP to Next | Level | EXP to Next |
|-------|------------|-------|------------|-------|------------|
| 1     | 9          | 34    | 13,967     | 67    | 687,211    |
| 2     | 16         | 35    | 15,775     | 68    | 740,988    |
| 3     | 25         | 36    | 17,678     | 69    | 925,400    |
| 4     | 36         | 37    | 19,677     | 70    | 1,473,746  |
| 5     | 77         | 38    | 21,773     | 71    | 1,594,058  |
| 6     | 112        | 39    | 30,543     | 72    | 1,718,928  |
| 7     | 153        | 40    | 34,212     | 73    | 1,848,355  |
| 8     | 200        | 41    | 38,065     | 74    | 1,982,340  |
| 9     | 253        | 42    | 42,102     | 75    | 2,230,113  |
| 10    | 320        | 43    | 46,323     | 76    | 2,386,162  |
| 11    | 385        | 44    | 53,026     | 77    | 2,547,417  |
| 12    | 490        | 45    | 58,419     | 78    | 2,713,878  |
| 13    | 585        | 46    | 64,041     | 79    | 3,206,160  |
| 14    | 700        | 47    | 69,892     | 80    | 3,681,024  |
| 15    | 830        | 48    | 75,973     | 81    | 4,022,472  |
| 16    | 970        | 49    | 102,468    | 82    | 4,377,024  |
| 17    | 1,120      | 50    | 115,254    | 83    | 4,744,680  |
| 18    | 1,260      | 51    | 128,692    | 84    | 5,125,440  |
| 19    | 1,420      | 52    | 142,784    | 85    | 5,767,272  |
| 20    | 1,620      | 53    | 157,528    | 86    | 6,204,000  |
| 21    | 1,860      | 54    | 178,184    | 87    | 6,655,464  |
| 22    | 1,990      | 55    | 196,300    | 88    | 7,121,664  |
| 23    | 2,240      | 56    | 215,198    | 89    | 7,602,600  |
| 24    | 2,504      | 57    | 234,879    | 90    | 9,738,720  |
| 25    | 2,950      | 58    | 255,341    | 91    | 11,649,960 |
| 26    | 3,426      | 59    | 330,188    | 92    | 13,643,520 |
| 27    | 3,934      | 60    | 365,914    | 93    | 18,339,300 |
| 28    | 4,474      | 61    | 403,224    | 94    | 23,836,800 |
| 29    | 6,889      | 62    | 442,116    | 95    | 35,658,000 |
| 30    | 7,995      | 63    | 482,590    | 96    | 48,687,000 |
| 31    | 9,174      | 64    | 536,948    | 97    | 58,135,000 |
| 32    | 10,425     | 65    | 585,191    | 98    | 99,999,998 |
| 33    | 11,748     | 66    | 635,278    |       |            |

**Total base EXP 1-99 (normal)**: ~405,234,427

### 3.4 Transcendent Base EXP Table (Selected Milestones)

Transcendent classes require significantly more EXP per level:

| Level | Normal EXP | Trans. EXP | Multiplier |
|-------|-----------|-----------|-----------|
| 1     | 9         | 10        | ~1.1x     |
| 10    | 320       | 400       | 1.25x     |
| 30    | 7,995     | 12,392    | 1.55x     |
| 50    | 115,254   | 213,220   | 1.85x     |
| 70    | 1,473,746 | 3,389,616 | 2.3x      |
| 90    | 9,738,720 | 29,216,160| 3.0x      |
| 98    | 99,999,998| 343,210,000| 3.4x     |

**Total trans. base EXP 1-99**: ~1,212,492,549

### 3.5 Job EXP Tables

#### Novice (Job Level 1-10)

| Job Level | EXP to Next |
|-----------|------------|
| 1         | 10         |
| 2         | 18         |
| 3         | 28         |
| 4         | 40         |
| 5         | 91         |
| 6         | 151        |
| 7         | 205        |
| 8         | 268        |
| 9         | 340        |

**Total to Job 10**: ~1,151

#### First Class (Job Level 1-50)

| Job Lv | EXP     | Job Lv | EXP     | Job Lv | EXP     |
|--------|---------|--------|---------|--------|---------|
| 1      | 43      | 18     | 2,226   | 35     | 78,160  |
| 2      | 58      | 19     | 3,040   | 36     | 84,175  |
| 3      | 76      | 20     | 3,988   | 37     | 90,404  |
| 4      | 116     | 21     | 5,564   | 38     | 107,611 |
| 5      | 180     | 22     | 6,272   | 39     | 125,915 |
| 6      | 220     | 23     | 7,021   | 40     | 153,941 |
| 7      | 272     | 24     | 9,114   | 41     | 191,781 |
| 8      | 336     | 25     | 11,473  | 42     | 204,351 |
| 9      | 520     | 26     | 15,290  | 43     | 248,352 |
| 10     | 604     | 27     | 16,891  | 44     | 286,212 |
| 11     | 699     | 28     | 18,570  | 45     | 386,371 |
| 12     | 802     | 29     | 23,229  | 46     | 409,795 |
| 13     | 948     | 30     | 28,359  | 47     | 482,092 |
| 14     | 1,125   | 31     | 36,478  | 48     | 509,596 |
| 15     | 1,668   | 32     | 39,716  | 49     | 519,787 |
| 16     | 1,937   | 33     | 43,088  |        |         |
| 17     | 2,226   | 34     | 52,417  |        |         |

**Total to Job 50 (First Class)**: ~3,753,621

#### Second Class (Job Level 1-50)

| Job Lv | EXP       | Job Lv | EXP       | Job Lv | EXP       |
|--------|-----------|--------|-----------|--------|-----------|
| 1      | 184       | 18     | 20,642    | 35     | 358,435   |
| 2      | 284       | 19     | 27,434    | 36     | 380,376   |
| 3      | 348       | 20     | 35,108    | 37     | 447,685   |
| 4      | 603       | 21     | 38,577    | 38     | 526,989   |
| 5      | 887       | 22     | 42,206    | 39     | 610,246   |
| 6      | 1,096     | 23     | 52,708    | 40     | 644,736   |
| 7      | 1,598     | 24     | 66,971    | 41     | 793,535   |
| 8      | 2,540     | 25     | 82,688    | 42     | 921,810   |
| 9      | 3,676     | 26     | 89,544    | 43     | 1,106,758 |
| 10     | 4,290     | 27     | 96,669    | 44     | 1,260,955 |
| 11     | 4,946     | 28     | 117,821   | 45     | 1,487,304 |
| 12     | 6,679     | 29     | 144,921   | 46     | 1,557,657 |
| 13     | 9,492     | 30     | 174,201   | 47     | 1,990,632 |
| 14     | 12,770    | 31     | 186,677   | 48     | 2,083,386 |
| 15     | 14,344    | 32     | 199,584   | 49     | 2,125,053 |
| 16     | 16,005    | 33     | 238,617   |        |           |
| 17     | 20,642    | 34     | 286,366   |        |           |

**Total to Job 50 (Second Class)**: ~16,488,271

#### Transcendent Second Class (Job Level 51-70)

Transcendent second classes share the same job EXP table as normal second classes for levels 1-50, then continue to 70:

| Job Lv | EXP         |
|--------|-------------|
| 50     | 2,125,053   |
| 51     | 4,488,362   |
| 52     | 5,765,950   |
| 53     | 7,246,770   |
| 54     | 9,498,810   |
| 55     | 12,658,800  |
| 56     | 17,127,600  |
| 57     | 22,275,900  |
| 58     | 29,586,000  |
| 59     | 37,170,000  |
| 60     | 52,050,000  |
| 61     | 58,821,000  |
| 62     | 68,280,000  |
| 63     | 78,750,000  |
| 64     | 89,250,000  |
| 65     | 99,750,000  |
| 66     | 110,250,000 |
| 67     | 120,750,000 |
| 68     | 131,250,000 |
| 69     | 141,750,000 |

**Total to Job 70 (Trans.)**: ~1,084,674,386

### 3.6 Party EXP Sharing

#### Two Modes

1. **Each Take**: Each party member only gains EXP from monsters they personally damaged. No sharing.
2. **Even Share**: All EXP from party kills is pooled and split equally. Requires all members to be within 15 base levels of each other.

#### Party Bonus (Even Share)

| Party Size | Total EXP Pool | Per Person |
|-----------|---------------|------------|
| 1         | 100%          | 100%       |
| 2         | 120%          | 60%        |
| 3         | 140%          | ~47%       |
| 4         | 160%          | 40%        |
| 5         | 180%          | 36%        |
| 6         | 200%          | ~33%       |
| 7         | 220%          | ~31%       |
| 8         | 240%          | 30%        |
| 9         | 260%          | ~29%       |
| 10        | 280%          | 28%        |
| 11        | 300%          | ~27%       |
| 12        | 320%          | ~27%       |

Formula: `TotalEXP = BaseMonsterEXP * (1 + (PartySize - 1) * 0.20)`

Note: Both Base and Job EXP are shared using the same formula.

### 3.7 Monster Level Difference Modifiers

EXP gained varies based on the level difference between player and monster:

| Monster Level vs Player | EXP Modifier |
|------------------------|-------------|
| Monster 10+ levels above | 140%       |
| Monster 6-10 levels above | 120%      |
| Monster 1-5 levels above | 110%       |
| Same level              | 100%        |
| Monster 1-5 below       | 95%        |
| Monster 6-10 below      | 80%        |
| Monster 11-15 below     | 60%        |
| Monster 16-20 below     | 35%        |
| Monster 21-25 below     | 20%        |
| Monster 26-30 below     | 15%        |
| Monster 31+ below       | 10%        |

**MVP monsters** are exempt and always give 100% of their EXP.

### 3.8 Death Penalty

**Pre-renewal**:
- On death: lose 1% of the **total base EXP required for the current level** (i.e., 1% of `EXPToNextLevel`)
- Job EXP is NOT lost on death
- Cannot delevel (EXP cannot go below 0 for current level)
- **Transcendent** classes: same 1% penalty, but their EXP tables are much larger, so absolute loss is greater
- **Novice** class: no death penalty
- **PvP / WoE**: different rules may apply (typically reduced or no penalty)

---

## 4. Job/Class System

### 4.1 Class Hierarchy Overview

```
                          Novice (Job 1-10)
                              |
            +---------+-------+-------+---------+--------+
            |         |       |       |         |        |
        Swordman    Mage   Archer  Merchant   Thief   Acolyte
        (Job 1-50)                                    (Job 1-50)
            |         |       |       |         |        |
         +--+--+   +--+--+ +--+--+ +--+--+  +--+--+  +--+--+
         |     |   |     | |     | |     |  |     |  |     |
       Knight Crus Wiz Sage Hunt B/D BS Alch Asn Rog Prs Monk
       (Job 1-50)                                    (Job 1-50)
            |
      [Reach Base 99, Job 50]
            |
      High Novice (Job 1-10, 100 stat pts)
            |
      High First Class (Job 1-50)
            |
      Transcendent Second Class (Job 1-70)
```

Additionally:
- **Super Novice**: Base 1-99, Job 1-99 (branches from Novice at Base 45+)

### 4.2 All Classes (Pre-Renewal)

#### Novice Tier

| Class | Base Level Cap | Job Level Cap | Requirements |
|-------|---------------|---------------|-------------|
| Novice | 99 | 10 | Starting class |
| High Novice | 99 | 10 | Rebirth from 2nd class (Base 99, Job 50) |
| Super Novice | 99 | 99 | Novice at Base Level 45+ |

#### First Job Classes

| Class | Job Level Cap | Job Change Requirement | Weapons |
|-------|--------------|----------------------|---------|
| Swordman | 50 | Novice Job 10 | Daggers, 1H Swords, 2H Swords, 1H Spears, 2H Spears |
| Mage | 50 | Novice Job 10 | Rods/Staves, Daggers |
| Archer | 50 | Novice Job 10 | Bows |
| Merchant | 50 | Novice Job 10 | Daggers, 1H Swords, Axes, Maces |
| Thief | 50 | Novice Job 10 | Daggers, 1H Swords, Bows (some) |
| Acolyte | 50 | Novice Job 10 | Maces, Rods/Staves |

#### Second Job Classes (2-1 Branch)

| Class | Base Class | Job Level Cap | Change Requirement |
|-------|-----------|--------------|-------------------|
| Knight | Swordman | 50 | Job 40+ as Swordman |
| Wizard | Mage | 50 | Job 40+ as Mage |
| Hunter | Archer | 50 | Job 40+ as Archer |
| Blacksmith | Merchant | 50 | Job 40+ as Merchant |
| Assassin | Thief | 50 | Job 40+ as Thief |
| Priest | Acolyte | 50 | Job 40+ as Acolyte |

#### Second Job Classes (2-2 Branch)

| Class | Base Class | Job Level Cap | Change Requirement |
|-------|-----------|--------------|-------------------|
| Crusader | Swordman | 50 | Job 40+ as Swordman |
| Sage | Mage | 50 | Job 40+ as Mage |
| Bard (M) / Dancer (F) | Archer | 50 | Job 40+ as Archer |
| Alchemist | Merchant | 50 | Job 40+ as Merchant |
| Rogue | Thief | 50 | Job 40+ as Thief |
| Monk | Acolyte | 50 | Job 40+ as Acolyte |

#### Transcendent Classes

| Transcendent Class | Normal Equivalent | Job Level Cap |
|-------------------|------------------|--------------|
| Lord Knight | Knight | 70 |
| High Wizard | Wizard | 70 |
| Sniper | Hunter | 70 |
| Whitesmith (Mastersmith) | Blacksmith | 70 |
| Assassin Cross | Assassin | 70 |
| High Priest | Priest | 70 |
| Paladin | Crusader | 70 |
| Scholar (Professor) | Sage | 70 |
| Minstrel (M) / Gypsy (F) | Bard / Dancer | 70 |
| Biochemist (Creator) | Alchemist | 70 |
| Stalker | Rogue | 70 |
| Champion | Monk | 70 |

### 4.3 Job Change Requirements

#### Novice -> First Class
- **Required**: Novice Job Level 10
- **Process**: Complete a class-specific job change quest in the appropriate city
  - Swordman: Izlude Swordman Academy
  - Mage: Geffen Magic Academy
  - Archer: Payon Archer Village
  - Merchant: Alberta Merchant Guild
  - Thief: Morroc Pyramid (Thieves Guild)
  - Acolyte: Prontera Church

#### First Class -> Second Class
- **Required**: First Class Job Level 40 minimum (recommended: Job 50 for maximum skill points)
- **Process**: Complete the respective guild test quest
- **Important**: Job Level 40 means 40 skill points. Job Level 50 means 49 skill points (Job 1 gives no point). Going at Job 40 means losing 9 potential skill points forever.

#### Second Class -> Transcendent
- **Required**: Base Level 99 AND Job Level 50
- **Cost**: 1,285,000 Zeny
- **Additional Requirements**:
  - Weight below 100%
  - No Cart, Pet, or Falcon/Peco Peco
  - All skill points must be allocated
- **Process**: Speak to the Valkyrie in Juno. Character is reborn as High Novice Level 1/1
- **Benefits**:
  - 100 starting stat points (vs 48 for normal)
  - 25% permanent HP/SP bonus (TransMod = 1.25)
  - Access to transcendent-exclusive skills (20 additional job levels worth)
  - Quest skills (First Aid, Play Dead) are pre-learned

#### Novice -> Super Novice
- **Required**: Novice Job Level 10, Base Level 45+
- **Unique Features**:
  - Can learn (almost) all First Class skills from every tree
  - Uses Novice HP/SP tables (very low)
  - Guardian Angel: at level multiples of 10, angel appears with random buffs
  - /doridori command: doubles natural HP/SP regen for a time
  - Steel Body angel: at 0 HP, angel may revive with full HP + Mental Strength buff
  - Cannot use 2-Handed Swords, Spears, or Bows
  - Job Level cap: 99

### 4.4 Stat Points and Skill Points Per Job Level

**Stat Points**: Gained only from Base Levels (see Section 1.1). Job Levels do NOT grant stat points directly.

**Skill Points**: 1 skill point per Job Level gained (starting from Job Level 2).
- Novice Job 10: 9 skill points (levels 2-10)
- First Class Job 50: 49 skill points (levels 2-50)
- Second Class Job 50: 49 skill points (levels 2-50)
- Trans Second Class Job 70: 69 skill points (levels 2-70)
- Super Novice Job 99: 98 skill points (levels 2-99)

### 4.5 Job Bonus Stats

Each class receives hidden stat bonuses at specific job levels. These are automatic and do not cost stat points. They show as green text in the stat window.

#### Novice Job Bonus Stats (Job 1-10)

| Job Lv | STR | AGI | VIT | INT | DEX | LUK |
|--------|-----|-----|-----|-----|-----|-----|
| 1      | 0   | 0   | 0   | 0   | 0   | 0   |
| 2      | 0   | 0   | 0   | 0   | 0   | 1   |
| 6      | 0   | 1   | 0   | 0   | 1   | 1   |
| 8      | 1   | 1   | 1   | 0   | 1   | 1   |
| 10     | 1   | 1   | 1   | 1   | 1   | 1   |

#### Swordman Job Bonus Stats (Job 1-50)

Swordman receives primarily STR and VIT bonuses:
- By Job 50: STR +5, AGI +3, VIT +5, INT +0, DEX +3, LUK +0

#### Mage Job Bonus Stats (Job 1-50)

Mage receives primarily INT and DEX bonuses:
- By Job 50: STR +0, AGI +2, VIT +0, INT +6, DEX +5, LUK +0

#### Full Job Bonus Summary (at Max Job Level)

| Class       | STR | AGI | VIT | INT | DEX | LUK | Total |
|------------|-----|-----|-----|-----|-----|-----|-------|
| Novice      | 1   | 1   | 1   | 1   | 1   | 1   | 6     |
| Swordman    | 5   | 3   | 5   | 0   | 3   | 0   | 16    |
| Mage        | 0   | 2   | 0   | 6   | 5   | 0   | 13    |
| Archer      | 1   | 5   | 0   | 1   | 5   | 1   | 13    |
| Merchant    | 4   | 1   | 4   | 1   | 4   | 1   | 15    |
| Thief       | 3   | 5   | 1   | 0   | 4   | 2   | 15    |
| Acolyte     | 0   | 2   | 2   | 5   | 3   | 3   | 15    |
| Knight      | 8   | 5   | 7   | 1   | 5   | 1   | 27    |
| Wizard      | 0   | 3   | 0   | 9   | 8   | 0   | 20    |
| Hunter      | 3   | 8   | 1   | 2   | 8   | 2   | 24    |
| Blacksmith  | 6   | 2   | 6   | 2   | 6   | 2   | 24    |
| Assassin    | 5   | 8   | 2   | 0   | 5   | 4   | 24    |
| Priest      | 0   | 3   | 5   | 7   | 5   | 3   | 23    |
| Crusader    | 7   | 3   | 7   | 3   | 5   | 2   | 27    |
| Sage        | 0   | 5   | 0   | 8   | 7   | 0   | 20    |
| Bard/Dancer | 3   | 6   | 1   | 4   | 6   | 4   | 24    |
| Alchemist   | 4   | 4   | 4   | 4   | 4   | 4   | 24    |
| Rogue       | 4   | 7   | 2   | 1   | 6   | 4   | 24    |
| Monk        | 6   | 6   | 3   | 3   | 5   | 4   | 27    |

Transcendent classes use the same bonus table as their non-transcendent counterparts for levels 1-50, then receive additional bonuses for levels 51-70 (roughly 4-6 more total bonus stats).

---

## 5. DB Schema (PostgreSQL)

### 5.1 Characters Table

This extends the existing `characters` table with the full stat and job system columns:

```sql
-- ============================================================
-- Core character table (extends existing schema)
-- ============================================================
CREATE TABLE IF NOT EXISTS characters (
    character_id    SERIAL PRIMARY KEY,
    user_id         INTEGER NOT NULL REFERENCES users(user_id),
    name            VARCHAR(24) NOT NULL UNIQUE,

    -- Visual / identity
    class           VARCHAR(30) DEFAULT 'novice',       -- display class (novice, swordman, knight, etc.)
    gender          VARCHAR(10) DEFAULT 'male',
    hair_style      INTEGER DEFAULT 1,
    hair_color      INTEGER DEFAULT 0,

    -- Base Level
    level           INTEGER DEFAULT 1 CHECK (level BETWEEN 1 AND 99),
    base_exp        BIGINT DEFAULT 0,

    -- Job Level & Class
    job_level       INTEGER DEFAULT 1 CHECK (job_level BETWEEN 1 AND 99),
    job_exp         BIGINT DEFAULT 0,
    job_class       VARCHAR(30) DEFAULT 'novice',       -- internal class id for job tree

    -- Primary Stats (player-allocated)
    str             INTEGER DEFAULT 1 CHECK (str BETWEEN 1 AND 99),
    agi             INTEGER DEFAULT 1 CHECK (agi BETWEEN 1 AND 99),
    vit             INTEGER DEFAULT 1 CHECK (vit BETWEEN 1 AND 99),
    int_stat        INTEGER DEFAULT 1 CHECK (int_stat BETWEEN 1 AND 99),  -- "int" is reserved
    dex             INTEGER DEFAULT 1 CHECK (dex BETWEEN 1 AND 99),
    luk             INTEGER DEFAULT 1 CHECK (luk BETWEEN 1 AND 99),

    -- Point pools
    stat_points     INTEGER DEFAULT 48,     -- unspent stat points
    skill_points    INTEGER DEFAULT 0,      -- unspent skill points

    -- HP/SP (current values, recalculated on login)
    current_health  INTEGER DEFAULT 0,
    max_health      INTEGER DEFAULT 0,
    current_mana    INTEGER DEFAULT 0,
    max_mana        INTEGER DEFAULT 0,

    -- Position
    x               REAL DEFAULT 0,
    y               REAL DEFAULT 0,
    z               REAL DEFAULT 580,
    zone_name       VARCHAR(50) DEFAULT 'prt_fild08',
    level_name      VARCHAR(100) DEFAULT 'L_PrtSouth',

    -- Rebirth tracking
    is_transcendent BOOLEAN DEFAULT FALSE,
    rebirth_count   INTEGER DEFAULT 0,       -- 0 = normal, 1 = transcendent

    -- Weight
    current_weight  INTEGER DEFAULT 0,

    -- Soft delete
    deleted         BOOLEAN DEFAULT FALSE,
    delete_date     TIMESTAMPTZ,

    -- Timestamps
    created_at      TIMESTAMPTZ DEFAULT NOW(),
    last_login      TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_characters_user ON characters(user_id);
CREATE INDEX IF NOT EXISTS idx_characters_job_class ON characters(job_class);
CREATE INDEX IF NOT EXISTS idx_characters_zone ON characters(zone_name);
```

### 5.2 Job Class Definitions Table

```sql
-- ============================================================
-- Static job class definitions
-- ============================================================
CREATE TABLE IF NOT EXISTS job_classes (
    class_id        VARCHAR(30) PRIMARY KEY,      -- 'novice', 'swordman', 'knight', etc.
    display_name    VARCHAR(50) NOT NULL,
    class_tier      SMALLINT NOT NULL DEFAULT 0,   -- 0=novice, 1=first, 2=second, 3=transcendent
    parent_class_id VARCHAR(30) REFERENCES job_classes(class_id),

    -- Level caps
    base_level_cap  INTEGER NOT NULL DEFAULT 99,
    job_level_cap   INTEGER NOT NULL DEFAULT 50,

    -- HP/SP coefficients
    hp_job_a        REAL NOT NULL DEFAULT 0.0,
    hp_job_b        REAL NOT NULL DEFAULT 5.0,
    sp_job_a        REAL NOT NULL DEFAULT 0.0,
    sp_job_b        REAL NOT NULL DEFAULT 2.0,

    -- Weight bonus
    weight_bonus    INTEGER NOT NULL DEFAULT 0,

    -- ASPD base delays (JSON object: weapon_type -> BTBA in seconds)
    aspd_base       JSONB DEFAULT '{}',

    -- Is transcendent variant?
    is_transcendent BOOLEAN DEFAULT FALSE,

    -- Job change requirements
    required_job_level  INTEGER DEFAULT 10,         -- min job level of parent class
    required_base_level INTEGER DEFAULT 0,          -- min base level
    required_zeny       INTEGER DEFAULT 0            -- cost in zeny
);

-- Seed data
INSERT INTO job_classes (class_id, display_name, class_tier, parent_class_id, job_level_cap,
    hp_job_a, hp_job_b, sp_job_a, sp_job_b, weight_bonus,
    required_job_level, required_base_level) VALUES
-- Novice Tier
('novice',          'Novice',           0, NULL,        10, 0.0, 5.0, 0.0, 2.0,   0,  0,  0),
('high_novice',     'High Novice',      0, NULL,        10, 0.0, 5.0, 0.0, 2.0,   0,  0,  0),
('super_novice',    'Super Novice',     0, 'novice',    99, 0.0, 5.0, 0.0, 2.0,   0, 10, 45),

-- First Job Classes
('swordman',        'Swordman',         1, 'novice',    50, 0.7, 5.0, 0.2, 2.0, 800, 10,  0),
('mage',            'Mage',             1, 'novice',    50, 0.3, 5.0, 0.6, 2.0, 200, 10,  0),
('archer',          'Archer',           1, 'novice',    50, 0.5, 5.0, 0.4, 2.0, 600, 10,  0),
('merchant',        'Merchant',         1, 'novice',    50, 0.4, 5.0, 0.3, 2.0, 800, 10,  0),
('thief',           'Thief',            1, 'novice',    50, 0.5, 5.0, 0.3, 2.0, 400, 10,  0),
('acolyte',         'Acolyte',          1, 'novice',    50, 0.4, 5.0, 0.5, 2.0, 400, 10,  0),

-- Second Job Classes (2-1)
('knight',          'Knight',           2, 'swordman',  50, 1.5, 5.0, 0.4, 2.0, 800, 40,  0),
('wizard',          'Wizard',           2, 'mage',      50, 0.55,5.0, 1.0, 2.0, 200, 40,  0),
('hunter',          'Hunter',           2, 'archer',    50, 0.85,5.0, 0.6, 2.0, 600, 40,  0),
('blacksmith',      'Blacksmith',       2, 'merchant',  50, 0.9, 5.0, 0.5, 2.0, 800, 40,  0),
('assassin',        'Assassin',         2, 'thief',     50, 1.1, 5.0, 0.5, 2.0, 600, 40,  0),
('priest',          'Priest',           2, 'acolyte',   50, 0.75,5.0, 0.8, 2.0, 600, 40,  0),

-- Second Job Classes (2-2)
('crusader',        'Crusader',         2, 'swordman',  50, 1.1, 7.0, 0.5, 2.0, 800, 40,  0),
('sage',            'Sage',             2, 'mage',      50, 0.75,5.0, 0.8, 2.0, 400, 40,  0),
('bard',            'Bard',             2, 'archer',    50, 0.75,3.0, 0.6, 2.0, 600, 40,  0),
('dancer',          'Dancer',           2, 'archer',    50, 0.75,3.0, 0.6, 2.0, 600, 40,  0),
('alchemist',       'Alchemist',        2, 'merchant',  50, 0.9, 5.0, 0.5, 2.0, 800, 40,  0),
('rogue',           'Rogue',            2, 'thief',     50, 0.85,5.0, 0.5, 2.0, 600, 40,  0),
('monk',            'Monk',             2, 'acolyte',   50, 0.9, 6.5, 0.5, 2.0, 600, 40,  0),

-- Transcendent Classes
('lord_knight',     'Lord Knight',      3, 'knight',    70, 1.5, 5.0, 0.4, 2.0, 800, 50, 99),
('high_wizard',     'High Wizard',      3, 'wizard',    70, 0.55,5.0, 1.0, 2.0, 200, 50, 99),
('sniper',          'Sniper',           3, 'hunter',    70, 0.85,5.0, 0.6, 2.0, 600, 50, 99),
('whitesmith',      'Whitesmith',       3, 'blacksmith',70, 0.9, 5.0, 0.5, 2.0, 800, 50, 99),
('assassin_cross',  'Assassin Cross',   3, 'assassin',  70, 1.1, 5.0, 0.5, 2.0, 600, 50, 99),
('high_priest',     'High Priest',      3, 'priest',    70, 0.75,5.0, 0.8, 2.0, 600, 50, 99),
('paladin',         'Paladin',          3, 'crusader',  70, 1.1, 7.0, 0.5, 2.0, 800, 50, 99),
('scholar',         'Scholar',          3, 'sage',      70, 0.75,5.0, 0.8, 2.0, 400, 50, 99),
('minstrel',        'Minstrel',         3, 'bard',      70, 0.75,3.0, 0.6, 2.0, 600, 50, 99),
('gypsy',           'Gypsy',            3, 'dancer',    70, 0.75,3.0, 0.6, 2.0, 600, 50, 99),
('biochemist',      'Biochemist',       3, 'alchemist', 70, 0.9, 5.0, 0.5, 2.0, 800, 50, 99),
('stalker',         'Stalker',          3, 'rogue',     70, 0.85,5.0, 0.5, 2.0, 600, 50, 99),
('champion',        'Champion',         3, 'monk',      70, 0.9, 6.5, 0.5, 2.0, 600, 50, 99)
ON CONFLICT (class_id) DO NOTHING;

-- Mark transcendent classes
UPDATE job_classes SET is_transcendent = TRUE WHERE class_tier = 3;
UPDATE job_classes SET is_transcendent = TRUE WHERE class_id = 'high_novice';
```

### 5.3 Job Bonus Stats Table

```sql
-- ============================================================
-- Job bonus stats per job level per class
-- ============================================================
CREATE TABLE IF NOT EXISTS job_bonus_stats (
    class_id    VARCHAR(30) NOT NULL REFERENCES job_classes(class_id),
    job_level   INTEGER NOT NULL CHECK (job_level BETWEEN 1 AND 99),
    str_bonus   INTEGER DEFAULT 0,
    agi_bonus   INTEGER DEFAULT 0,
    vit_bonus   INTEGER DEFAULT 0,
    int_bonus   INTEGER DEFAULT 0,
    dex_bonus   INTEGER DEFAULT 0,
    luk_bonus   INTEGER DEFAULT 0,
    PRIMARY KEY (class_id, job_level)
);

-- Example: Swordman bonuses (cumulative at each job level milestone)
-- Full data would be seeded from rAthena job_db
INSERT INTO job_bonus_stats (class_id, job_level, str_bonus, agi_bonus, vit_bonus, int_bonus, dex_bonus, luk_bonus) VALUES
('swordman', 1,  0, 0, 0, 0, 0, 0),
('swordman', 10, 1, 1, 1, 0, 1, 0),
('swordman', 20, 2, 1, 2, 0, 1, 0),
('swordman', 30, 3, 2, 3, 0, 2, 0),
('swordman', 40, 4, 2, 4, 0, 2, 0),
('swordman', 50, 5, 3, 5, 0, 3, 0)
ON CONFLICT DO NOTHING;
```

### 5.4 EXP Tables

```sql
-- ============================================================
-- Base experience table (level -> EXP required for next level)
-- ============================================================
CREATE TABLE IF NOT EXISTS base_exp_table (
    level           INTEGER NOT NULL,
    class_type      VARCHAR(20) NOT NULL DEFAULT 'normal',  -- 'normal' or 'transcendent'
    exp_to_next     BIGINT NOT NULL,
    PRIMARY KEY (level, class_type)
);

-- ============================================================
-- Job experience table (job_level -> EXP required for next level)
-- ============================================================
CREATE TABLE IF NOT EXISTS job_exp_table (
    job_level       INTEGER NOT NULL,
    class_tier      VARCHAR(20) NOT NULL DEFAULT 'first',   -- 'novice', 'first', 'second', 'trans_second', 'super_novice'
    exp_to_next     BIGINT NOT NULL,
    PRIMARY KEY (job_level, class_tier)
);
```

### 5.5 Character Skills Table (Already Exists)

```sql
-- Already in server schema:
CREATE TABLE IF NOT EXISTS character_skills (
    character_id    INTEGER NOT NULL REFERENCES characters(character_id),
    skill_id        INTEGER NOT NULL,
    skill_level     INTEGER DEFAULT 0,
    PRIMARY KEY (character_id, skill_id)
);
```

### 5.6 Migration Script

```sql
-- ============================================================
-- Migration: add_stat_job_system.sql
-- Run after existing migrations
-- ============================================================

-- Add columns to characters if missing
ALTER TABLE characters
    ADD COLUMN IF NOT EXISTS is_transcendent BOOLEAN DEFAULT FALSE,
    ADD COLUMN IF NOT EXISTS rebirth_count INTEGER DEFAULT 0,
    ADD COLUMN IF NOT EXISTS current_weight INTEGER DEFAULT 0;

-- Ensure constraints
ALTER TABLE characters
    ALTER COLUMN str SET DEFAULT 1,
    ALTER COLUMN agi SET DEFAULT 1,
    ALTER COLUMN vit SET DEFAULT 1,
    ALTER COLUMN int_stat SET DEFAULT 1,
    ALTER COLUMN dex SET DEFAULT 1,
    ALTER COLUMN luk SET DEFAULT 1;

-- Create job_classes and job_bonus_stats tables (see above for full CREATE + INSERT)
-- Create base_exp_table and job_exp_table (see above)
```

---

## 6. UE5 C++ Implementation

### 6.1 Stat Struct (C++ Header)

```cpp
// CharacterStats.h
#pragma once
#include "CoreMinimal.h"
#include "CharacterStats.generated.h"

/**
 * Primary stats (player-allocated).
 * All values 1-99. Server-authoritative.
 */
USTRUCT(BlueprintType)
struct FPrimaryStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 STR = 1;
    UPROPERTY(BlueprintReadOnly) int32 AGI = 1;
    UPROPERTY(BlueprintReadOnly) int32 VIT = 1;
    UPROPERTY(BlueprintReadOnly) int32 INT = 1;   // Note: "INT" is not reserved in UE C++
    UPROPERTY(BlueprintReadOnly) int32 DEX = 1;
    UPROPERTY(BlueprintReadOnly) int32 LUK = 1;
};

/**
 * Job bonus stats (auto-applied per job level, class-specific).
 * Added on top of primary stats; not player-controlled.
 */
USTRUCT(BlueprintType)
struct FJobBonusStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 STR = 0;
    UPROPERTY(BlueprintReadOnly) int32 AGI = 0;
    UPROPERTY(BlueprintReadOnly) int32 VIT = 0;
    UPROPERTY(BlueprintReadOnly) int32 INT = 0;
    UPROPERTY(BlueprintReadOnly) int32 DEX = 0;
    UPROPERTY(BlueprintReadOnly) int32 LUK = 0;
};

/**
 * Derived (computed) stats. Recalculated server-side whenever
 * primary stats, equipment, or buffs change. Client receives
 * final values via socket events.
 */
USTRUCT(BlueprintType)
struct FDerivedStats
{
    GENERATED_BODY()

    // Attack
    UPROPERTY(BlueprintReadOnly) int32 StatusATK = 0;   // A component
    UPROPERTY(BlueprintReadOnly) int32 WeaponATK = 0;   // B component
    UPROPERTY(BlueprintReadOnly) int32 MATK_Min = 0;
    UPROPERTY(BlueprintReadOnly) int32 MATK_Max = 0;

    // Defense
    UPROPERTY(BlueprintReadOnly) int32 HardDEF = 0;     // Equipment DEF
    UPROPERTY(BlueprintReadOnly) int32 SoftDEF = 0;     // VIT DEF
    UPROPERTY(BlueprintReadOnly) int32 HardMDEF = 0;    // Equipment MDEF
    UPROPERTY(BlueprintReadOnly) int32 SoftMDEF = 0;    // INT MDEF

    // Accuracy / Evasion
    UPROPERTY(BlueprintReadOnly) int32 HIT = 0;
    UPROPERTY(BlueprintReadOnly) int32 Flee = 0;        // A component
    UPROPERTY(BlueprintReadOnly) int32 PerfectDodge = 0; // B component

    // Speed
    UPROPERTY(BlueprintReadOnly) int32 ASPD = 0;        // 0-190
    UPROPERTY(BlueprintReadOnly) float AttackDelay = 0;  // ms between attacks

    // Critical
    UPROPERTY(BlueprintReadOnly) float CritRate = 0;     // percentage
    UPROPERTY(BlueprintReadOnly) float CritShield = 0;   // target's crit reduction

    // Resources
    UPROPERTY(BlueprintReadOnly) int32 MaxHP = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxSP = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxWeight = 0;

    // Regeneration
    UPROPERTY(BlueprintReadOnly) int32 HPRegenPerTick = 0;
    UPROPERTY(BlueprintReadOnly) int32 SPRegenPerTick = 0;

    // Cast time
    UPROPERTY(BlueprintReadOnly) float CastTimeReduction = 0;  // 0.0 to 1.0
};

/**
 * Full character stat block sent from server to client.
 */
USTRUCT(BlueprintType)
struct FCharacterStatBlock
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(BlueprintReadOnly) int32 CharacterId = 0;
    UPROPERTY(BlueprintReadOnly) FString ClassName;        // "knight", "wizard", etc.
    UPROPERTY(BlueprintReadOnly) bool bIsTranscendent = false;

    // Levels
    UPROPERTY(BlueprintReadOnly) int32 BaseLevel = 1;
    UPROPERTY(BlueprintReadOnly) int64 BaseEXP = 0;
    UPROPERTY(BlueprintReadOnly) int64 BaseEXPToNext = 0;
    UPROPERTY(BlueprintReadOnly) int32 JobLevel = 1;
    UPROPERTY(BlueprintReadOnly) int64 JobEXP = 0;
    UPROPERTY(BlueprintReadOnly) int64 JobEXPToNext = 0;

    // Points
    UPROPERTY(BlueprintReadOnly) int32 StatPoints = 48;
    UPROPERTY(BlueprintReadOnly) int32 SkillPoints = 0;

    // Stats
    UPROPERTY(BlueprintReadOnly) FPrimaryStats Primary;
    UPROPERTY(BlueprintReadOnly) FJobBonusStats JobBonus;
    UPROPERTY(BlueprintReadOnly) FDerivedStats Derived;

    // Current resources
    UPROPERTY(BlueprintReadOnly) int32 CurrentHP = 0;
    UPROPERTY(BlueprintReadOnly) int32 CurrentSP = 0;
    UPROPERTY(BlueprintReadOnly) int32 CurrentWeight = 0;
};
```

### 6.2 Stat Recalculation (Server-Side, JavaScript)

All stat recalculation happens on the server. The client only receives final computed values. This is the reference implementation matching the formulas in this document.

```javascript
// ============================================================
// server/src/stat_calculator.js
// ============================================================

/**
 * Calculate the cost to raise a stat from currentValue to currentValue+1
 */
function getStatPointCost(currentValue) {
    return Math.floor((currentValue - 1) / 10) + 2;
}

/**
 * Calculate stat points gained for reaching a specific base level
 */
function getStatPointsForLevel(level) {
    return Math.floor(level / 5) + 3;
}

/**
 * Calculate total stat points available at a given base level
 * @param {number} baseLevel - Current base level
 * @param {boolean} isTranscendent - Whether character is transcendent
 * @returns {number} Total stat points
 */
function getTotalStatPoints(baseLevel, isTranscendent) {
    let total = isTranscendent ? 100 : 48; // starting points
    for (let lv = 2; lv <= baseLevel; lv++) {
        total += getStatPointsForLevel(lv);
    }
    return total;
}

/**
 * Calculate Base HP for a class at a given level
 */
function calculateBaseHP(baseLevel, hpJobA, hpJobB) {
    let baseHP = 35 + (baseLevel * hpJobB);
    for (let i = 2; i <= baseLevel; i++) {
        baseHP += Math.round(hpJobA * i);
    }
    return baseHP;
}

/**
 * Calculate Max HP
 */
function calculateMaxHP(baseLevel, vit, hpJobA, hpJobB, isTranscendent, additiveMod, multiplicativeMod) {
    const baseHP = calculateBaseHP(baseLevel, hpJobA, hpJobB);
    const transMod = isTranscendent ? 1.25 : 1.0;
    let maxHP = Math.floor(baseHP * (1 + vit * 0.01) * transMod);
    maxHP += (additiveMod || 0);
    maxHP = Math.floor(maxHP * (1 + (multiplicativeMod || 0) * 0.01));
    return Math.max(1, maxHP);
}

/**
 * Calculate Base SP for a class at a given level
 */
function calculateBaseSP(baseLevel, spJobA, spJobB) {
    let baseSP = 10 + (baseLevel * spJobB);
    for (let i = 2; i <= baseLevel; i++) {
        baseSP += Math.round(spJobA * i);
    }
    return baseSP;
}

/**
 * Calculate Max SP
 */
function calculateMaxSP(baseLevel, intStat, spJobA, spJobB, isTranscendent, additiveMod, multiplicativeMod) {
    const baseSP = calculateBaseSP(baseLevel, spJobA, spJobB);
    const transMod = isTranscendent ? 1.25 : 1.0;
    let maxSP = Math.floor(baseSP * (1 + intStat * 0.01) * transMod);
    maxSP += (additiveMod || 0);
    maxSP = Math.floor(maxSP * (1 + (multiplicativeMod || 0) * 0.01));
    return Math.max(1, maxSP);
}

/**
 * Calculate StatusATK
 */
function calculateStatusATK(baseLevel, str, dex, luk, isRangedWeapon) {
    const baseLevelBonus = Math.floor(baseLevel / 4);
    if (isRangedWeapon) {
        return baseLevelBonus + Math.floor(str / 5) + dex + Math.floor(luk / 3);
    }
    return baseLevelBonus + str + Math.floor(dex / 5) + Math.floor(luk / 3);
}

/**
 * Calculate WeaponATK (single roll)
 */
function calculateWeaponATK(baseWeaponDmg, weaponLevel, mainStat, refinement, sizePenalty) {
    // Variance
    const varianceRange = Math.floor(0.05 * weaponLevel * baseWeaponDmg);
    const variance = Math.floor(Math.random() * (varianceRange * 2 + 1)) - varianceRange;

    // Stat bonus
    const statBonus = Math.floor(baseWeaponDmg * mainStat / 200);

    // Refinement bonus
    const refineATK = [0, 2, 3, 5, 7]; // per weapon level
    const refinementBonus = refinement * (refineATK[weaponLevel] || 0);

    // Over-upgrade bonus
    const safetyLimits = [0, 7, 6, 5, 4];
    const overUpgradeBonusMax = [0, 3, 5, 8, 13];
    let overUpgradeBonus = 0;
    const safeLimit = safetyLimits[weaponLevel] || 4;
    if (refinement > safeLimit) {
        const overCount = refinement - safeLimit;
        overUpgradeBonus = Math.floor(Math.random() * (overUpgradeBonusMax[weaponLevel] + 1)) * overCount;
    }

    const rawWeaponATK = baseWeaponDmg + variance + statBonus + refinementBonus + overUpgradeBonus;
    return Math.max(0, Math.floor(rawWeaponATK * sizePenalty / 100));
}

/**
 * Calculate HIT
 */
function calculateHIT(baseLevel, dex, bonusHIT) {
    return baseLevel + dex + (bonusHIT || 0);
}

/**
 * Calculate Flee
 */
function calculateFlee(baseLevel, agi, bonusFlee) {
    return baseLevel + agi + (bonusFlee || 0);
}

/**
 * Calculate Perfect Dodge
 */
function calculatePerfectDodge(luk, bonusPD) {
    return Math.floor(luk / 10) + (bonusPD || 0);
}

/**
 * Calculate Soft DEF
 */
function calculateSoftDEF(vit, agi, baseLevel) {
    return Math.floor(vit / 2) + Math.floor(agi / 5) + Math.floor(baseLevel / 2);
}

/**
 * Calculate Soft MDEF
 */
function calculateSoftMDEF(intStat, vit, dex, baseLevel) {
    return intStat + Math.floor(vit / 5) + Math.floor(dex / 5) + Math.floor(baseLevel / 4);
}

/**
 * Apply Hard DEF to damage
 */
function applyHardDEF(damage, hardDEF) {
    return Math.floor(damage * (4000 + hardDEF) / (4000 + hardDEF * 10));
}

/**
 * Apply Hard MDEF to damage
 */
function applyHardMDEF(damage, hardMDEF) {
    return Math.floor(damage * (1000 + hardMDEF) / (1000 + hardMDEF * 10));
}

/**
 * Calculate ASPD
 */
function calculateASPD(weaponDelay, agi, dex, speedModifier) {
    const agiReduction = Math.floor(weaponDelay * agi / 25);
    const dexReduction = Math.floor(weaponDelay * dex / 100);
    const totalReduction = Math.floor((agiReduction + dexReduction) / 10);
    const baseASPD = 200 - (weaponDelay - totalReduction) * (1 - (speedModifier || 0));
    return Math.min(190, Math.max(0, Math.floor(baseASPD)));
}

/**
 * Calculate MATK range
 */
function calculateMATK(intStat) {
    return {
        min: intStat + Math.floor(intStat / 7) ** 2,
        max: intStat + Math.floor(intStat / 5) ** 2
    };
}

/**
 * Calculate Critical Rate (percentage)
 */
function calculateCritRate(luk, bonusCrit) {
    return Math.floor(luk * 0.3) + 1 + (bonusCrit || 0);
}

/**
 * Calculate cast time
 */
function calculateCastTime(baseCastTime, dex, bonusReduction) {
    const dexReduction = Math.min(1.0, dex / 150);
    const finalReduction = Math.min(1.0, dexReduction + (bonusReduction || 0));
    return Math.max(0, baseCastTime * (1 - finalReduction));
}

/**
 * Calculate HP regeneration per tick (6 second tick)
 */
function calculateHPRegen(maxHP, vit) {
    return Math.max(1, Math.floor(maxHP / 200)) + Math.floor(vit / 5);
}

/**
 * Calculate SP regeneration per tick (8 second tick)
 */
function calculateSPRegen(maxSP, intStat) {
    let regen = Math.floor(maxSP / 100) + Math.floor(intStat / 6) + 1;
    if (intStat >= 120) {
        regen += Math.floor(intStat / 2) - 56;
    }
    return regen;
}

/**
 * Calculate Weight Limit
 */
function calculateMaxWeight(str, jobWeightBonus) {
    return 2000 + str * 30 + (jobWeightBonus || 0);
}

/**
 * Full stat recalculation for a character
 * Called whenever stats, equipment, or buffs change
 */
function recalculateAllStats(character, jobClassDef, equipment, buffs) {
    const c = character;
    const jc = jobClassDef;
    const isRanged = equipment && ['bow', 'gun', 'instrument', 'whip'].includes(equipment.weaponType);
    const mainStat = isRanged ? c.dex : c.str;

    // Effective stats = base + job bonus + equipment bonus + buff bonus
    const totalSTR = c.str + (c.jobBonusSTR || 0) + (equipment?.strBonus || 0) + (buffs?.strBonus || 0);
    const totalAGI = c.agi + (c.jobBonusAGI || 0) + (equipment?.agiBonus || 0) + (buffs?.agiBonus || 0);
    const totalVIT = c.vit + (c.jobBonusVIT || 0) + (equipment?.vitBonus || 0) + (buffs?.vitBonus || 0);
    const totalINT = c.int_stat + (c.jobBonusINT || 0) + (equipment?.intBonus || 0) + (buffs?.intBonus || 0);
    const totalDEX = c.dex + (c.jobBonusDEX || 0) + (equipment?.dexBonus || 0) + (buffs?.dexBonus || 0);
    const totalLUK = c.luk + (c.jobBonusLUK || 0) + (equipment?.lukBonus || 0) + (buffs?.lukBonus || 0);

    const result = {};

    // ATK
    result.statusATK = calculateStatusATK(c.level, totalSTR, totalDEX, totalLUK, isRanged);
    result.matk = calculateMATK(totalINT);

    // DEF
    result.softDEF = calculateSoftDEF(totalVIT, totalAGI, c.level);
    result.hardDEF = equipment?.totalDEF || 0;
    result.softMDEF = calculateSoftMDEF(totalINT, totalVIT, totalDEX, c.level);
    result.hardMDEF = equipment?.totalMDEF || 0;

    // HIT / Flee
    result.hit = calculateHIT(c.level, totalDEX, equipment?.hitBonus || 0);
    result.flee = calculateFlee(c.level, totalAGI, equipment?.fleeBonus || 0);
    result.perfectDodge = calculatePerfectDodge(totalLUK, equipment?.pdBonus || 0);

    // ASPD
    const weaponDelay = getWeaponDelay(c.job_class, equipment?.weaponType || 'bare_hand');
    result.aspd = calculateASPD(weaponDelay, totalAGI, totalDEX, buffs?.speedModifier || 0);
    result.attackDelay = (200 - result.aspd) * 10; // ms

    // Critical
    result.critRate = calculateCritRate(totalLUK, equipment?.critBonus || 0);

    // HP / SP
    result.maxHP = calculateMaxHP(c.level, totalVIT, jc.hp_job_a, jc.hp_job_b,
        c.is_transcendent, equipment?.maxHPBonus || 0, buffs?.hpMultiplier || 0);
    result.maxSP = calculateMaxSP(c.level, totalINT, jc.sp_job_a, jc.sp_job_b,
        c.is_transcendent, equipment?.maxSPBonus || 0, buffs?.spMultiplier || 0);

    // Regeneration
    result.hpRegen = calculateHPRegen(result.maxHP, totalVIT);
    result.spRegen = calculateSPRegen(result.maxSP, totalINT);

    // Weight
    result.maxWeight = calculateMaxWeight(totalSTR, jc.weight_bonus);

    return result;
}

module.exports = {
    getStatPointCost,
    getStatPointsForLevel,
    getTotalStatPoints,
    calculateBaseHP,
    calculateMaxHP,
    calculateBaseSP,
    calculateMaxSP,
    calculateStatusATK,
    calculateWeaponATK,
    calculateHIT,
    calculateFlee,
    calculatePerfectDodge,
    calculateSoftDEF,
    calculateSoftMDEF,
    applyHardDEF,
    applyHardMDEF,
    calculateASPD,
    calculateMATK,
    calculateCritRate,
    calculateCastTime,
    calculateHPRegen,
    calculateSPRegen,
    calculateMaxWeight,
    recalculateAllStats
};
```

### 6.3 Server-Side Stat Validation

The server must validate every stat allocation request:

```javascript
// In socket event handler: stats:allocate
socket.on('stats:allocate', async (data) => {
    const { statName, amount } = data;
    const player = players.get(socket.characterId);
    if (!player) return;

    // Validate stat name
    const validStats = ['str', 'agi', 'vit', 'int_stat', 'dex', 'luk'];
    if (!validStats.includes(statName)) return;

    const currentValue = player[statName];
    if (currentValue >= 99) return; // already maxed

    // Calculate cost
    let totalCost = 0;
    let newValue = currentValue;
    for (let i = 0; i < amount; i++) {
        if (newValue >= 99) break;
        totalCost += getStatPointCost(newValue);
        newValue++;
    }

    if (totalCost > player.stat_points) {
        socket.emit('stats:error', { message: 'Not enough stat points' });
        return;
    }

    // Apply
    player[statName] = newValue;
    player.stat_points -= totalCost;

    // Recalculate all derived stats
    const derived = recalculateAllStats(player, jobClassDef, equipment, buffs);
    Object.assign(player, derived);

    // Persist to DB
    await pool.query(
        `UPDATE characters SET ${statName} = $1, stat_points = $2 WHERE character_id = $3`,
        [newValue, player.stat_points, player.character_id]
    );

    // Broadcast updated stats
    socket.emit('player:stats', buildStatsPayload(player));
});
```

### 6.4 Level-Up Processing

```javascript
/**
 * Process base level up
 */
function processBaseLevelUp(player) {
    const expTable = getBaseExpTable(player.is_transcendent);
    while (player.level < 99) {
        const expNeeded = expTable[player.level];
        if (!expNeeded || player.base_exp < expNeeded) break;

        player.base_exp -= expNeeded;
        player.level++;

        // Grant stat points
        const pointsGained = getStatPointsForLevel(player.level);
        player.stat_points += pointsGained;

        // Recalculate MaxHP/MaxSP (level affects base HP)
        // ... recalculateAllStats() ...
    }
}

/**
 * Process job level up
 */
function processJobLevelUp(player, jobClassDef) {
    const expTable = getJobExpTable(jobClassDef.class_tier);
    while (player.job_level < jobClassDef.job_level_cap) {
        const expNeeded = expTable[player.job_level];
        if (!expNeeded || player.job_exp < expNeeded) break;

        player.job_exp -= expNeeded;
        player.job_level++;

        // Grant 1 skill point
        player.skill_points++;

        // Update job bonus stats
        // ... lookup job_bonus_stats for new job_level ...
    }
}
```

### 6.5 Client-Side Stat Display (UE5)

The client never recalculates stats -- it displays whatever the server sends. The `player:stats` socket event payload includes all values needed for the UI.

```cpp
// In SabriMMOCharacter.cpp or BasicInfoSubsystem.cpp:
// Parse the player:stats socket event and populate the stat block

void UBasicInfoSubsystem::OnStatsReceived(const FString& JsonPayload)
{
    // Parse JSON into FCharacterStatBlock
    // Update widget display
    // Stat allocation UI shows:
    //   - Current stat value + job bonus (e.g., "STR: 50 + 5")
    //   - Cost to raise next point
    //   - Available stat points
    //   - Derived stat totals
}
```

The stat window should display:
- Primary stats with job bonus: `STR  50 + 5` (where 5 is from job bonus)
- StatusATK + WeaponATK: `ATK  120 + 85`
- StatusDEF + EquipDEF: `DEF  40 + 65`
- Flee + PerfectDodge: `FLEE  180 + 7`
- MATK range: `MATK  150 ~ 250`
- Cost for next point: shows how many stat points the next raise costs
- Remaining stat points available

---

## Appendix A: Element Table

Pre-renewal element advantage multipliers (attacking element vs defending element, Level 1):

| Attack \ Defend | Neutral | Water | Fire | Earth | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|------|-------|------|--------|------|--------|-------|--------|
| Neutral        | 100     | 100   | 100  | 100   | 100  | 100    | 100  | 100    | 25    | 100    |
| Water          | 100     | 25    | 150  | 100   | 90   | 100    | 75   | 100    | 100   | 100    |
| Fire           | 100     | 150   | 25   | 90    | 100  | 100    | 75   | 100    | 100   | 125    |
| Earth          | 100     | 100   | 90   | 25    | 150  | 100    | 75   | 100    | 100   | 100    |
| Wind           | 100     | 90    | 100  | 150   | 25   | 100    | 75   | 100    | 100   | 100    |
| Poison         | 100     | 100   | 100  | 100   | 100  | 0      | 75   | 50     | 100   | -25    |
| Holy           | 100     | 100   | 100  | 100   | 100  | 100    | 0    | 125    | 100   | 150    |
| Shadow         | 100     | 100   | 100  | 100   | 100  | 50     | 125  | 0      | 100   | -25    |
| Ghost          | 0       | 100   | 100  | 100   | 100  | 100    | 100  | 100    | 125   | 100    |
| Undead         | 100     | 100   | 100  | 100   | 100  | -25    | 150  | -25    | 100   | 0      |

Values are percentages. Negative values mean healing the target. 0 = immune.
Higher element levels (2, 3, 4) shift these values further. Level 4 elements have extreme multipliers (e.g., Fire Lv4 vs Water Lv1 = 175%).

---

## Appendix B: Melee Attack Classification

An attack is classified as **melee** if the attacker is fewer than 4 cells (tiles) from the target. An attack is classified as **ranged** if the attacker is 4 or more cells away. This affects:
- Which ATK formula is used (STR-primary vs DEX-primary)
- Whether certain skills/cards/buffs apply
- In UE5 terms: 1 RO cell = ~50 UE units, so melee range < 200 UE units

---

## Appendix C: Quick Reference — All Derived Stat Formulas

| Stat | Formula |
|------|---------|
| StatusATK (melee) | `floor(BaseLv/4) + STR + floor(DEX/5) + floor(LUK/3)` |
| StatusATK (ranged) | `floor(BaseLv/4) + floor(STR/5) + DEX + floor(LUK/3)` |
| MATK Min | `INT + floor(INT/7)^2` |
| MATK Max | `INT + floor(INT/5)^2` |
| HIT | `BaseLv + DEX + Bonus` |
| Flee | `BaseLv + AGI + Bonus` |
| Perfect Dodge | `floor(LUK/10) + Bonus` |
| Soft DEF | `floor(VIT/2) + floor(AGI/5) + floor(BaseLv/2)` |
| Hard DEF reduction | `Dmg * (4000+DEF) / (4000+DEF*10)` |
| Soft MDEF | `INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)` |
| Hard MDEF reduction | `Dmg * (1000+MDEF) / (1000+MDEF*10)` |
| ASPD | `200 - (WD - floor((WD*AGI/25 + WD*DEX/100)/10)) * (1-SM)` |
| Crit Rate | `floor(LUK*0.3) + 1 + Bonus` |
| Max HP | `floor(BaseHP * (1+VIT*0.01) * TransMod) + AddMod` |
| Max SP | `floor(BaseSP * (1+INT*0.01) * TransMod) + AddMod` |
| Cast Time | `Base * (1 - DEX/150)` |
| HP Regen | `max(1, floor(MaxHP/200)) + floor(VIT/5)` |
| SP Regen | `floor(MaxSP/100) + floor(INT/6) + 1` |
| Weight Limit | `2000 + STR*30 + JobBonus` |
| Stat Cost | `floor((current-1)/10) + 2` |
| Stat Pts/Level | `floor(level/5) + 3` |

---

## Sources

- [iRO Wiki Classic - Stats](https://irowiki.org/classic/Stats)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [iRO Wiki Classic - Max HP](https://irowiki.org/classic/Max_HP)
- [iRO Wiki Classic - Base EXP Chart](https://irowiki.org/classic/Base_EXP_Chart)
- [iRO Wiki Classic - Job EXP Chart](https://irowiki.org/classic/Job_EXP_Chart)
- [iRO Wiki Classic - Classes](https://irowiki.org/classic/Classes)
- [iRO Wiki Classic - DEF](https://irowiki.org/classic/DEF)
- [iRO Wiki Classic - Size](https://irowiki.org/classic/Size)
- [iRO Wiki - ATK](https://irowiki.org/wiki/ATK)
- [iRO Wiki - MATK](https://irowiki.org/wiki/MATK)
- [iRO Wiki - MDEF](https://irowiki.org/wiki/MDEF)
- [iRO Wiki - Experience](https://irowiki.org/wiki/Experience)
- [iRO Wiki - Weight Limit](https://irowiki.org/wiki/Weight_Limit)
- [iRO Wiki - Rebirth](https://irowiki.org/wiki/Rebirth)
- [iRO Wiki - Super Novice](https://irowiki.org/wiki/Super_Novice)
- [RateMyServer - Base EXP Table (Normal)](https://ratemyserver.net/index.php?page=misc_table_exp&op=1)
- [RateMyServer - Base EXP Table (Transcendent)](https://ratemyserver.net/index.php?page=misc_table_exp&op=2)
- [RateMyServer - Job EXP Table (First Class)](https://ratemyserver.net/index.php?page=misc_table_exp&op=4)
- [RateMyServer - Job EXP Table (Second Class)](https://ratemyserver.net/index.php?page=misc_table_exp&op=5)
- [RateMyServer - Weapon Size Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer - Stat Point Table](https://ratemyserver.net/index.php?page=misc_table_exp&op=21)
- [RateMyServer - Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus)
- [rAthena GitHub - Pre-Renewal DB](https://github.com/rathena/rathena/tree/master/db/pre-re)
- [rAthena GitHub - Pre-Renewal Job EXP](https://github.com/rathena/rathena/blob/master/db/pre-re/job_exp.yml)
- [Ragnarok Wiki (Fandom) - Stats](https://ragnarok.fandom.com/wiki/Stats_(RO))
- [Ragnarok Wiki (Fandom) - Elements](https://ragnarok.fandom.com/wiki/Elements)
