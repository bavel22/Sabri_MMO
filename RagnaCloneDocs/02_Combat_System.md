# 02 -- Combat System (Pre-Renewal)

> Ragnarok Online Classic (pre-Renewal) complete combat mechanics reference.
> All formulas sourced from iRO Wiki Classic, rAthena pre-renewal source, and RateMyServer archives.
> Implementation notes reference the Sabri_MMO server (`server/src/ro_damage_formulas.js`).

---

## Table of Contents

1. [Physical Damage](#1-physical-damage)
2. [Magical Damage](#2-magical-damage)
3. [Defense (DEF / MDEF)](#3-defense)
4. [HIT / FLEE](#4-hit--flee)
5. [ASPD (Attack Speed)](#5-aspd)
6. [Critical Hits](#6-critical-hits)
7. [Element Table](#7-element-table)
8. [Size and Race](#8-size-and-race)
9. [Status Effects](#9-status-effects)
10. [Implementation](#10-implementation)

---

## 1. Physical Damage

Physical damage in pre-Renewal RO is composed of multiple additive and multiplicative stages. Every intermediate calculation is floored (truncated to integer) immediately -- decimals never persist between steps.

### 1.1 StatusATK (Stat-Based Base Damage)

StatusATK is the damage contribution from raw character stats. It differs by weapon class:

**Melee weapons** (Daggers, Swords, Axes, Maces, Spears, Katars, Knuckles, Books):

```
StatusATK = STR + floor(STR / 10)^2 + floor(DEX / 5) + floor(LUK / 3)
```

**Ranged weapons** (Bows, Guns, Instruments, Whips):

```
StatusATK = DEX + floor(DEX / 10)^2 + floor(STR / 5) + floor(LUK / 3)
```

The `floor(X / 10)^2` term creates the "STR/DEX bonus" breakpoints every 10 points:

| STR | floor(STR/10)^2 |
|-----|-----------------|
| 1-9 | 0 |
| 10-19 | 1 |
| 20-29 | 4 |
| 30-39 | 9 |
| 40-49 | 16 |
| 50-59 | 25 |
| 60-69 | 36 |
| 70-79 | 49 |
| 80-89 | 64 |
| 90-99 | 81 |

### 1.2 BaseATK (Status Window ATK)

The BaseATK shown in the status window combines stat contribution with equipment bonuses:

**Melee**:
```
BaseATK = STR + floor(STR / 10)^2 + floor(DEX / 5) + floor(LUK / 5)
        + UpgradeBonus + ImpositioManus + ATK_From_Cards
```

**Ranged**:
```
BaseATK = DEX + floor(DEX / 10)^2 + floor(STR / 5) + floor(LUK / 5)
        + UpgradeBonus + ImpositioManus + ATK_From_Cards
```

Note: Some sources use `floor(LUK / 3)` for StatusATK and `floor(LUK / 5)` for BaseATK. The distinction matters for display vs. damage calculation.

### 1.3 WeaponATK and Variance

WeaponATK is the damage from the weapon itself, subject to random variance based on weapon level and the attacker's DEX:

**Melee (normal hit)**:
```
WeaponATK = random( min(DEX * (0.8 + 0.2 * WeaponLevel), ATK), ATK )
```

Where `ATK` is the weapon's base attack power. The DEX term narrows the damage range -- higher DEX means more consistent damage.

**Melee (critical hit)**:
```
WeaponATK = ATK  (always maximum, no variance)
```

**Ranged (bows)**:
```
WeaponATK = random(ATK/100 * min(ATK, DEX*(0.8+0.2*WeaponLevel)), max(...))
          + random(0, ArrowATK - 1)
```

### 1.4 Damage Variance by Weapon Level

Weapon level determines the variance spread applied to WeaponATK:

| Weapon Level | Variance Range | Example (100 ATK weapon) |
|-------------|----------------|-------------------------|
| Level 1 | +/- 5% | 95 -- 100 |
| Level 2 | +/- 10% | 90 -- 100 |
| Level 3 | +/- 15% | 85 -- 100 |
| Level 4 | +/- 20% | 80 -- 100 |

### 1.5 Refinement (Upgrade) Bonuses

Weapons can be refined at NPCs. Each refinement level adds flat ATK:

| Weapon Level | ATK per Refine | Safe Limit | Over-Upgrade Bonus (per level beyond safe) |
|-------------|---------------|------------|-------------------------------------------|
| Level 1 | +2 ATK | +7 | +3 ATK |
| Level 2 | +3 ATK | +6 | +5 ATK |
| Level 3 | +5 ATK | +5 | +7 ATK |
| Level 4 | +7 ATK | +4 | +13 ATK |

**Example**: A +10 Level 3 weapon gains:
- Safe portion: 5 * 5 = 25 ATK
- Over-upgrade: 5 * (5 + 7) = 60 ATK (5 levels at base + 5 levels at bonus)
- Actually: 10 * 5 = 50 base + (10 - 5) * 7 = 35 over-upgrade = 85 total refinement ATK

### 1.6 Size Modifier

Size penalty applies to the **WeaponATK portion only** (not StatusATK or MasteryATK). See [Section 8](#8-size-and-race) for the full table.

```
SizedWeaponATK = floor(WeaponATK * SizePenalty / 100)
```

### 1.7 Element Modifier

After all ATK components are combined, the attacking element is compared against the target's element using the 10x10x4 element table. See [Section 7](#7-element-table).

```
ElementalDamage = floor(TotalATK * ElementModifier / 100)
```

### 1.8 Card / Equipment Bonuses

Card bonuses are organized by target property. Each category is additive within itself, then the total is applied multiplicatively:

```
CardBonus = RaceBonus% + ElementBonus% + SizeBonus%
DamageAfterCards = floor(TotalATK * (100 + CardBonus) / 100)
```

**Common card bonuses**:
- Hydra Card: +20% vs Demi-Human (race)
- Vadon Card: +20% vs Fire property (element)
- Desert Wolf Card: +15% vs Small size (size)
- Skeleton Worker Card: +15% vs Medium size (size)
- Minorous Card: +15% vs Large size (size)

### 1.9 Mastery ATK

MasteryATK is flat damage from passive skills. It is **not** affected by element modifiers, size penalties, or card bonuses -- it is added after all multipliers:

| Skill | Bonus |
|-------|-------|
| Sword Mastery (Swordsman) | +4 per level (1H swords) |
| Two-Handed Sword Mastery | +4 per level (2H swords) |
| Spear Mastery (Knight) | +4 per level (spears), +5 mounted |
| Katar Mastery (Assassin) | +3 per level (katars) |
| Weaponry Research (Blacksmith) | +2 per level (all weapons) |
| Coin Fling ATK bonus (Rogue) | Varies by skill level |

### 1.10 Buff ATK

BuffATK comes from active skill buffs. Like MasteryATK, it bypasses element/size/card modifiers:

| Buff | Effect |
|------|--------|
| Impositio Manus (Priest) | +5 ATK per level |
| Blessing (Priest) | +STR/DEX/INT directly |
| Provoke (Swordsman) | +ATK% (but also -DEF%) |
| Power Thrust (Blacksmith) | +5% ATK per level (weapon breaks on others) |

### 1.11 Complete Physical Damage Pipeline (Step by Step)

```
1.  Roll Perfect Dodge     → if PD triggers, MISS (criticals bypass PD)
2.  Roll Critical           → CRI = 1 + floor(LUK*0.3) + bonuses
                              CritShield = floor(targetLUK * 0.2)
                              EffectiveCrit = CRI - CritShield
                              if random(100) < EffectiveCrit → CRITICAL
3.  Roll HIT/FLEE           → Skipped on critical
                              HitRate = 80 + AttackerHIT - DefenderFLEE
                              Clamped to 5-95%
                              if random(100) >= HitRate → MISS
4.  Calculate StatusATK     → STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
5.  Calculate WeaponATK     → random(minATK, maxATK) with DEX narrowing
                              Critical: always max ATK
6.  Apply Size Penalty      → SizedWeaponATK = floor(WeaponATK * SizePct / 100)
7.  Apply Damage Variance   → +/- (WeaponLevel * 5)% of weapon ATK
8.  Sum Base Damage         → TotalATK = StatusATK + SizedWeaponATK + PassiveATK
9.  Apply Buff Multiplier   → TotalATK = floor(TotalATK * BuffMultiplier)
10. Apply Skill Multiplier  → TotalATK = floor(TotalATK * SkillMultiplier / 100)
11. Apply Card Bonuses      → TotalATK = floor(TotalATK * (100 + CardBonus) / 100)
12. Apply Element Modifier  → TotalATK = floor(TotalATK * EleModifier / 100)
13. Apply Hard DEF          → TotalATK = floor(TotalATK * (100 - HardDEF) / 100)
14. Apply Soft DEF          → TotalATK = TotalATK - SoftDEF
15. Add MasteryATK          → (already included in step 8 in simplified formula)
16. Floor to minimum 1      → FinalDamage = max(1, TotalATK)
```

### 1.12 Critical Hit Damage

Critical hits modify the pipeline:
- **No variance**: Always use maximum WeaponATK
- **Bypass FLEE**: Skip the HIT/FLEE check entirely
- **Bypass Hard DEF**: Equipment DEF is ignored (In some sources; see Section 6 for nuances)
- **Bypass Soft DEF**: VIT-based DEF is ignored (In some sources)
- **40% damage bonus**: Total damage is multiplied by 1.4x
- **Cannot be Perfect Dodged**: PD check is skipped (except in some implementations)

```
CriticalDamage = floor(NormalDamage * 1.4)
```

Note: The 40% bonus is the most commonly cited figure. Some rAthena configurations use different values. Katar-class weapons double the displayed critical rate but do not inherently change the damage bonus.

---

## 2. Magical Damage

Magical damage uses a separate formula that bypasses FLEE entirely. Magic always hits (unless the target is immune via element).

### 2.1 Status MATK

```
StatusMATK = INT + floor(INT / 7)^2
```

Alternative formulation (some sources):
```
StatusMATK = floor(BaseLevel / 4) + INT + floor(INT / 2) + floor(DEX / 5) + floor(LUK / 3)
```

The simpler `INT + floor(INT/7)^2` is the rAthena pre-renewal formula used in most classic servers and in our implementation.

### 2.2 MATK Min/Max Range

Unlike physical damage which has DEX-narrowed variance, magical damage has a fixed min/max range:

```
MATK_Max = StatusMATK + WeaponMATK
MATK_Min = floor(StatusMATK * 0.7) + floor(WeaponMATK * 0.7)

ActualMATK = random(MATK_Min, MATK_Max)
```

Some sources define min/max differently:
```
MATK_Max = INT + floor(INT / 5)^2
MATK_Min = INT + floor(INT / 7)^2
```

The gap between min and max creates the "magic damage spread" visible in the stat window.

### 2.3 Weapon MATK

Staves and rods contribute WeaponMATK:
```
WeaponMATK = BaseWeaponMATK + RefinementBonus + OverUpgradeBonus
```

Refinement bonuses for MATK follow the same table as physical ATK refinement.

### 2.4 Skill Multiplier

Each magical skill has a damage multiplier:

| Skill | Multiplier per Level |
|-------|---------------------|
| Fire Bolt | 100% per level (Lv10 = 1000%) |
| Cold Bolt | 100% per level (Lv10 = 1000%) |
| Lightning Bolt | 100% per level (Lv10 = 1000%) |
| Soul Strike | 100% per level (multi-hit) |
| Napalm Beat | 70% per level |
| Fire Ball | 170% + 10% per level |
| Fire Wall | 50% per hit |
| Thunderstorm | 100% per level (multi-hit AoE) |
| Jupitel Thunder | 100% per level (knockback) |
| Lord of Vermilion | 100% per level |
| Storm Gust | 100% per level |
| Meteor Storm | 125% per level |
| Heal (vs Undead) | Uses base heal formula |

### 2.5 Complete Magical Damage Pipeline

```
1.  Calculate StatusMATK    → INT + floor(INT/7)^2
2.  Add WeaponMATK          → From equipped staff/rod
3.  Roll damage in range    → random(MATK_Min, MATK_Max)
4.  Apply Skill Multiplier  → floor(MATK * SkillMultiplier / 100)
5.  Apply Buff Modifiers    → floor(damage * BuffMultiplier)
6.  Apply Element Modifier  → floor(damage * EleModifier / 100)
7.  Apply Hard MDEF         → floor(damage * (100 - HardMDEF) / 100)
8.  Subtract Soft MDEF      → damage = damage - SoftMDEF
9.  Floor to minimum 1      → FinalDamage = max(1, damage)
```

### 2.6 Bolt Multi-Hit

Bolt skills (Fire/Cold/Lightning Bolt) deal N hits where N = skill level:
- Each hit is calculated independently
- Each hit rolls its own MATK variance
- Damage per hit = `floor(MATK * 100 / 100)` = 1x MATK per hit
- Total damage = N * (per-hit damage)
- Server sends per-hit `skill:effect_damage` events with 200ms stagger

---

## 3. Defense

### 3.1 Hard DEF (Equipment DEF)

Hard DEF is a **percentage-based reduction** from equipment. In pre-Renewal, the formula depends on the server implementation:

**rAthena pre-renewal (simplified)**:
```
DamageAfterHardDEF = floor(Damage * (100 - HardDEF) / 100)
```

Where HardDEF is capped at 99 (99% reduction max).

**iRO Wiki formula (more precise)**:
```
DamageAfterHardDEF = Damage * (4000 + HardDEF) / (4000 + HardDEF * 10)
```

| HardDEF | Reduction (iRO formula) |
|---------|------------------------|
| 0 | 0% |
| 50 | ~10% |
| 100 | ~18% |
| 200 | ~29% |
| 275 | ~33% |
| 500 | ~50% |

**Armor refinement bonus to Hard DEF**:
- Each +1 refinement adds approximately 0.7 DEF (rounded via `floor((3 + CurrentRefine) / 4)`)

### 3.2 Soft DEF (VIT-based DEF)

Soft DEF is a **flat subtraction** from damage, applied after Hard DEF:

```
SoftDEF = floor(VIT * 0.5) + max(floor(VIT * 0.3), floor(VIT^2 / 150) - 1)
```

At high VIT, the `VIT^2 / 150` term dominates:

| VIT | Soft DEF (approximate) |
|-----|----------------------|
| 1 | 1 |
| 10 | 6 |
| 30 | 18 |
| 50 | 36 |
| 70 | 62 |
| 90 | 89 |
| 99 | 105 |

Soft DEF also has a **random component** in some implementations:
```
SoftDEF = floor(VIT * 0.5) + random(floor(VIT * 0.3), max(floor(VIT * 0.3), floor(VIT^2 / 150) - 1))
```

### 3.3 Hard MDEF (Equipment MDEF)

Hard MDEF works identically to Hard DEF but for magical damage:

**rAthena pre-renewal**:
```
DamageAfterHardMDEF = floor(Damage * (100 - HardMDEF) / 100)
```

**iRO Wiki formula**:
```
DamageAfterHardMDEF = Damage * (1000 + HardMDEF) / (1000 + HardMDEF * 10)
```

| HardMDEF | Reduction (iRO formula) |
|----------|------------------------|
| 0 | 0% |
| 30 | ~23% |
| 60 | ~33% |
| 90 | ~45% |
| 125 | ~50% |

### 3.4 Soft MDEF (INT-based MDEF)

```
SoftMDEF = floor(INT / 2) + max(0, floor((INT * 2 - 1) / 4))
```

Or from iRO Wiki:
```
SoftMDEF = floor(INT + (VIT / 5) + (DEX / 5) + (BaseLv / 4))
```

### 3.5 DEF Application Order

```
1. Start with raw damage
2. Apply Hard DEF (percentage reduction)
3. Apply Soft DEF (flat subtraction)
4. Floor to minimum 1
```

### 3.6 DEF Bypass Mechanics

| Mechanic | Effect |
|----------|--------|
| Critical Hits | Bypass Hard DEF entirely (some sources say both Hard and Soft) |
| Ice Pick weapon | Converts DEF to bonus damage (higher DEF = MORE damage taken) |
| Occult Impaction (Monk) | Same as Ice Pick -- converts DEF to damage |
| Provoke (Swordsman) | Reduces target's Hard DEF by 5-25% (depending on level) |
| Armor Break | Reduces Hard DEF to 0 temporarily |
| Magic Attacks | Ignore physical DEF entirely (use MDEF instead) |

### 3.7 Multi-Monster DEF Penalty

When attacked by more than 2 monsters simultaneously:
```
EffectiveDEF = DEF * (1 - (NumAttackers - 2) * 0.05)
```

Each additional attacker beyond 2 reduces your effective DEF by 5%.

---

## 4. HIT / FLEE

### 4.1 HIT Calculation

```
HIT = 175 + BaseLv + DEX + BonusHIT
```

Where BonusHIT comes from equipment, cards, and skills.

**Monster HIT**:
```
MonsterHIT = MonsterBaseLv + MonsterDEX
```

### 4.2 FLEE Calculation

```
FLEE = 100 + BaseLv + AGI + BonusFLEE
```

**Monster FLEE**:
```
MonsterFLEE = MonsterBaseLv + MonsterAGI
```

### 4.3 Hit Rate (Accuracy)

```
HitRate% = 80 + AttackerHIT - DefenderFLEE
```

Clamped between **5%** (minimum -- you always have at least 5% chance to hit) and **95%** (maximum -- you can never have 100% accuracy via HIT alone).

**Practical example**:
- Player: Level 50, DEX 40, HIT = 175 + 50 + 40 = 265
- Monster: FLEE 200
- Hit Rate = 80 + 265 - 200 = 145% -> capped at 95%

To achieve 95% hit rate: `AttackerHIT >= DefenderFLEE + 15`

To achieve the minimum 100% effective hit rate (95% cap): `HIT = FLEE - 80 + 95 = FLEE + 15`

### 4.4 Multi-Monster FLEE Penalty

When a character is attacked by more than 2 monsters simultaneously, FLEE is reduced:

```
EffectiveFLEE = FLEE - (NumAttackers - 2) * 10% of base FLEE
```

Or equivalently:
```
EffectiveFLEE = SkillBonusFLEE + floor((BaseLv + AGI + EquipFLEE) * (1 - (NumAttackers - 2) * 0.1))
```

Important: Skill-granted FLEE (like Thief's Improve Dodge) is added **after** the penalty, not before.

| Attackers | FLEE Remaining |
|-----------|---------------|
| 1-2 | 100% |
| 3 | 90% |
| 4 | 80% |
| 5 | 70% |
| 6 | 60% |
| 7 | 50% |
| 8 | 40% |
| 9 | 30% |
| 10 | 20% |
| 11+ | 0% (always hit) |

When 11 or more monsters attack simultaneously, effective FLEE drops to 0 and every attack lands.

### 4.5 Perfect Dodge

Perfect Dodge (Lucky Dodge) is a separate evasion check that occurs **before** the normal HIT/FLEE check:

```
PerfectDodge% = 1 + floor(LUK / 10) + BonusPD
```

Each point of PD = 1% chance to completely avoid the attack. PD has **no cap** (100% PD is theoretically possible with enough LUK + equipment).

**Key properties of Perfect Dodge**:
- Checked before HIT/FLEE
- **Not reduced** by the multi-monster FLEE penalty
- Critical hits **bypass** Perfect Dodge
- Skills generally cannot be Perfect Dodged (only auto-attacks)

---

## 5. ASPD

### 5.1 Pre-Renewal ASPD Formula

```
ASPD = 200 - (WD - floor((WD * AGI / 25) + (WD * DEX / 100)) / 10) * (1 - SM)
```

Where:
- **WD** = Weapon Delay = `50 * BTBA` (Base Time Between Attacks in seconds)
- **AGI** = Agility stat
- **DEX** = Dexterity stat
- **SM** = Speed Modifier (sum of all speed buffs)

### 5.2 Weapon Delay (BTBA) by Class and Weapon

Each class has a base attack speed that varies with weapon type. BTBA is measured in seconds:

**Representative BTBA values** (1st classes, common weapons):

| Class | Bare Fist | Dagger | 1H Sword | 2H Sword | Bow | Spear | Mace | Rod |
|-------|-----------|--------|----------|----------|-----|-------|------|-----|
| Novice | 0.50 | 0.65 | 0.70 | -- | -- | -- | -- | -- |
| Swordsman | 0.50 | 0.55 | 0.60 | 0.70 | -- | 0.70 | 0.65 | -- |
| Thief | 0.50 | 0.50 | 0.60 | -- | -- | -- | -- | -- |
| Mage | 0.50 | 0.65 | -- | -- | -- | -- | -- | 0.65 |
| Archer | 0.50 | 0.60 | -- | -- | 0.85 | -- | -- | -- |
| Acolyte | 0.50 | 0.60 | -- | -- | -- | -- | 0.60 | 0.65 |
| Merchant | 0.50 | 0.60 | 0.65 | -- | -- | -- | 0.65 | -- |

**Representative BTBA values** (2nd classes, key weapons):

| Class | Katar | 2H Sword | Bow | Spear (Mounted) | Knuckle | Book |
|-------|-------|----------|-----|-----------------|---------|------|
| Knight | -- | 1.10 | -- | 0.70 | -- | -- |
| Crusader | -- | -- | -- | 0.75 | -- | -- |
| Assassin | 0.70 | -- | -- | -- | -- | -- |
| Hunter | -- | -- | 0.85 | -- | -- | -- |
| Wizard | -- | -- | -- | -- | -- | -- |
| Priest | -- | -- | -- | -- | 0.65 | 0.60 |
| Monk | -- | -- | -- | -- | 0.55 | -- |
| Blacksmith | -- | -- | -- | -- | -- | -- |
| Alchemist | -- | -- | -- | -- | -- | -- |
| Rogue | -- | -- | 0.85 | -- | -- | -- |
| Bard | -- | -- | -- | -- | -- | -- |
| Dancer | -- | -- | -- | -- | -- | -- |

Note: Exact BTBA values vary by server implementation. The above are representative of common pre-renewal configurations. The definitive source is the server's `job_db.txt` or equivalent configuration.

**Shield penalty**: Equipping a shield reduces ASPD by approximately 5-10 depending on class.

### 5.3 Dual Wielding (Assassin)

Assassins can dual-wield daggers. The combined BTBA is:

```
DualBTBA = 0.7 * (BTBA_MainHand + BTBA_OffHand)
```

### 5.4 Speed Modifiers (SM)

Speed modifiers **stack additively** with each other, but potions use the highest value only (no potion stacking):

| Source | SM Value |
|--------|----------|
| Concentration Potion | 0.10 |
| Awakening Potion | 0.15 |
| Berserk Potion | 0.20 |
| Adrenaline Rush (self, Blacksmith) | 0.30 |
| Adrenaline Rush (party) | 0.25 |
| Two-Hand Quicken (Knight) | 0.30 |
| One-Hand Quicken (Crusader) | 0.30 |
| Frenzy / Berserk (Lord Knight) | 0.30 |
| Increase AGI (Priest) | indirect (adds AGI, not SM) |

Total SM is capped at preventing ASPD from exceeding 190.

### 5.5 ASPD to Attack Delay Conversion

```
AttackDelay (seconds) = (200 - ASPD) / 50
Hits per Second = 50 / (200 - ASPD)
```

| ASPD | Delay (seconds) | Hits/Second |
|------|----------------|-------------|
| 150 | 1.00 | 1.0 |
| 160 | 0.80 | 1.25 |
| 170 | 0.60 | 1.67 |
| 175 | 0.50 | 2.0 |
| 180 | 0.40 | 2.5 |
| 185 | 0.30 | 3.33 |
| 189 | 0.22 | 4.55 |
| 190 | 0.20 | 5.0 |

### 5.6 ASPD Cap

The hard cap in pre-Renewal is **190 ASPD** (5 attacks per second, 200ms between attacks). The formula approaches 200 asymptotically but is clamped at 190.

Some servers (including Sabri_MMO) use a softcap at 190 with diminishing returns up to 195.

### 5.7 Amotion and Dmotion

- **Amotion** = Attack animation duration in milliseconds. Determined by ASPD.
- **Dmotion** = Damage motion / hit stun duration. Fixed per monster, causes brief inaction after being hit.

```
Amotion_ms = (200 - ASPD) * 20
```

---

## 6. Critical Hits

### 6.1 Critical Rate

```
StatusCritical = 1 + floor(LUK * 0.3)
EquipCritical = sum of all +CRIT equipment bonuses
TotalCritical = StatusCritical + EquipCritical
```

**Status window display**: `floor(LUK / 3) + EquipBonuses` (slightly different rounding)

### 6.2 Target's Critical Shield

The target's LUK reduces the attacker's effective critical rate:

```
CritShield = floor(TargetLUK / 5)
EffectiveCritRate = TotalCritical - CritShield
```

Each 5 points of target LUK reduces incoming critical chance by 1%.

### 6.3 Katar Critical Bonus

Katar-class weapons **double** the displayed critical rate:
```
KatarCritical = TotalCritical * 2
```

This makes Assassins with Katars the primary critical-build class.

### 6.4 Critical Damage Calculation

Critical hits modify the normal damage calculation in several ways:

1. **Maximum WeaponATK**: No variance roll -- always use the highest possible weapon damage
2. **Bypass FLEE**: The HIT/FLEE accuracy check is skipped entirely
3. **40% damage bonus**: Final damage is multiplied by 1.4x
4. **DEF bypass** (contested): Some sources state criticals ignore both Hard and Soft DEF. Others state only Hard DEF is bypassed. The rAthena pre-renewal default ignores both.

```
CriticalDamage = floor(BaseDamage * 1.4)
```

### 6.5 Skills and Critical

In pre-Renewal, **skills cannot critically hit** (with very few exceptions). Critical hits apply only to auto-attacks. This means:
- Bash, Bowling Bash, etc. cannot crit
- Double Attack (Thief) interacts with crit checks but is not itself a crit
- The only exception is certain 3rd class skills in later patches

### 6.6 Critical vs Perfect Dodge

Critical hits **bypass Perfect Dodge**. This is the only attack type that does so. The check order is:

```
1. Is it a critical? → Skip PD and FLEE checks
2. Perfect Dodge check → LUK-based % chance to avoid
3. FLEE check → HIT vs FLEE accuracy roll
```

---

## 7. Element Table

### 7.1 Overview

Every entity in RO has an element (property) with a level (1-4). Attacks also carry an element. The interaction between attack element and defense element determines a damage multiplier.

**10 elements**: Neutral, Water, Earth, Fire, Wind, Poison, Holy, Shadow, Ghost, Undead

**4 element levels**: Higher levels generally mean stronger resistances and vulnerabilities.

### 7.2 Complete Element Table

Values are **damage percentages** (100 = normal, 0 = immune, negative = heals the target).

#### Level 1 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 25 | 100 |
| **Water** | 100 | 25 | 100 | 150 | 50 | 100 | 75 | 100 | 100 | 100 |
| **Earth** | 100 | 100 | 100 | 50 | 150 | 100 | 75 | 100 | 100 | 100 |
| **Fire** | 100 | 50 | 150 | 25 | 100 | 100 | 75 | 100 | 100 | 125 |
| **Wind** | 100 | 175 | 50 | 100 | 25 | 100 | 75 | 100 | 100 | 100 |
| **Poison** | 100 | 100 | 125 | 125 | 125 | 0 | 75 | 50 | 100 | -25 |
| **Holy** | 100 | 100 | 100 | 100 | 100 | 100 | 0 | 125 | 100 | 150 |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | 50 | 125 | 0 | 100 | -25 |
| **Ghost** | 25 | 100 | 100 | 100 | 100 | 100 | 75 | 75 | 125 | 100 |
| **Undead** | 100 | 100 | 100 | 100 | 100 | 50 | 100 | 0 | 100 | 0 |

#### Level 2 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 25 | 100 |
| **Water** | 100 | 0 | 100 | 175 | 25 | 100 | 50 | 75 | 100 | 100 |
| **Earth** | 100 | 100 | 50 | 25 | 175 | 100 | 50 | 75 | 100 | 100 |
| **Fire** | 100 | 25 | 175 | 0 | 100 | 100 | 50 | 75 | 100 | 150 |
| **Wind** | 100 | 175 | 25 | 100 | 0 | 100 | 50 | 75 | 100 | 100 |
| **Poison** | 100 | 75 | 125 | 125 | 125 | 0 | 50 | 25 | 75 | -50 |
| **Holy** | 100 | 100 | 100 | 100 | 100 | 100 | -25 | 150 | 100 | 175 |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | 25 | 150 | -25 | 100 | -50 |
| **Ghost** | 0 | 75 | 75 | 75 | 75 | 75 | 50 | 50 | 150 | 125 |
| **Undead** | 100 | 75 | 75 | 75 | 75 | 25 | 125 | 0 | 100 | 0 |

#### Level 3 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 0 | 100 |
| **Water** | 100 | -25 | 100 | 200 | 0 | 100 | 25 | 50 | 100 | 125 |
| **Earth** | 100 | 100 | 0 | 0 | 200 | 100 | 25 | 50 | 100 | 75 |
| **Fire** | 100 | 0 | 200 | -25 | 100 | 100 | 25 | 50 | 100 | 175 |
| **Wind** | 100 | 200 | 0 | 100 | -25 | 100 | 25 | 50 | 100 | 100 |
| **Poison** | 100 | 50 | 100 | 100 | 100 | 0 | 25 | 0 | 50 | -75 |
| **Holy** | 100 | 100 | 100 | 100 | 100 | 125 | -50 | 175 | 100 | 200 |
| **Shadow** | 100 | 100 | 100 | 100 | 100 | 0 | 175 | -50 | 100 | -75 |
| **Ghost** | 0 | 50 | 50 | 50 | 50 | 50 | 25 | 25 | 175 | 150 |
| **Undead** | 100 | 50 | 50 | 50 | 50 | 0 | 150 | 0 | 100 | 0 |

#### Level 4 Defenders

| Attack \ Defend | Neutral | Water | Earth | Fire | Wind | Poison | Holy | Shadow | Ghost | Undead |
|----------------|---------|-------|-------|------|------|--------|------|--------|-------|--------|
| **Neutral** | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 100 | 0 | 100 |
| **Water** | 100 | -50 | 100 | 200 | 0 | 75 | 0 | 25 | 100 | 150 |
| **Earth** | 100 | 100 | -25 | 0 | 200 | 75 | 0 | 25 | 100 | 50 |
| **Fire** | 100 | 0 | 200 | -50 | 100 | 75 | 0 | 25 | 100 | 200 |
| **Wind** | 100 | 200 | 0 | 100 | -50 | 75 | 0 | 25 | 100 | 100 |
| **Poison** | 100 | 25 | 75 | 75 | 75 | 0 | 0 | -25 | 25 | -100 |
| **Holy** | 100 | 75 | 75 | 75 | 75 | 125 | -100 | 200 | 100 | 200 |
| **Shadow** | 100 | 75 | 75 | 75 | 75 | -25 | 200 | -100 | 100 | -100 |
| **Ghost** | 0 | 25 | 25 | 25 | 25 | 25 | 0 | 0 | 200 | 175 |
| **Undead** | 100 | 25 | 25 | 25 | 25 | -25 | 175 | 0 | 100 | 0 |

### 7.3 Implementation Note

The server file `ro_damage_formulas.js` contains an `ELEMENT_TABLE` that was sourced from a different reference and has some values that differ from the canonical RateMyServer/iRO Wiki table above. Notable discrepancies in the server code include:
- Several Level 1 values for the four main elements (Water/Earth/Fire/Wind) vs Poison/Holy/Shadow/Ghost/Undead
- Some Level 2-4 values for Ghost and Undead interactions

The tables in this document represent the **canonical pre-Renewal values** from RateMyServer. The server implementation should be audited against these tables and corrected where needed.

### 7.4 Key Element Interactions

**Elemental Cycle** (strong against next in cycle):
```
Water > Fire > Earth > Wind > Water
```

**Holy vs Shadow/Undead**: Holy element is devastating against Shadow and Undead properties, scaling up to 200% damage at high levels.

**Ghost element**: Immune to Neutral attacks at Level 2+. Only Ghost element is super-effective against Ghost.

**Undead element**: Immune to Undead attacks. Healed by Poison and Shadow attacks (negative values).

**Self-element**: Attacking an element with itself deals reduced or zero damage (e.g., Fire vs Fire = 25% at Lv1, 0% at Lv2+).

### 7.5 Player Element

Players default to **Neutral** element. Equipment can change a player's element:
- Swordfish Card (armor) -> Water Lv1
- Sandman Card (armor) -> Earth Lv1
- Pasana Card (armor) -> Fire Lv1
- Dokebi Card (armor) -> Wind Lv1
- Argiope Card (armor) -> Poison Lv1
- Angeling Card (armor) -> Holy Lv1
- Bathory Card (armor) -> Shadow Lv1
- Ghostring Card (armor) -> Ghost Lv1
- Evil Druid Card (armor) -> Undead Lv1

### 7.6 Weapon Element

Weapons default to **Neutral** unless endowed. Element can come from:
- Endow skills (Sage): Fire/Water/Wind/Earth Lv1
- Cards: Drops Card (fire), Mandragora Card (earth), etc.
- Forged weapons: Element imbued during creation
- Aspersio (Priest): Holy element endow

---

## 8. Size and Race

### 8.1 Monster Sizes

All entities have one of three sizes:

| Size | Examples |
|------|---------|
| **Small** | Porings, Lunatics, Fabres, most insects, baby monsters |
| **Medium** | Most humanoid monsters, players (default), thieves, assassins |
| **Large** | MVPs, dragons, large beasts, golems, bosses |

Players are **Medium** by default. Adopted/baby characters are **Small**.

### 8.2 Weapon Size Penalty Table

Size penalty applies only to the **WeaponATK** portion of physical damage:

| Weapon Type | vs Small | vs Medium | vs Large |
|-------------|----------|-----------|----------|
| Bare Fist | 100% | 100% | 100% |
| Dagger | 100% | 75% | 50% |
| 1H Sword | 75% | 100% | 75% |
| 2H Sword | 75% | 75% | 100% |
| Spear (on foot) | 75% | 75% | 100% |
| Spear (mounted) | 75% | 100% | 100% |
| 1H Axe | 50% | 75% | 100% |
| 2H Axe | 50% | 75% | 100% |
| Mace | 75% | 100% | 100% |
| Rod/Staff | 100% | 100% | 100% |
| Bow | 100% | 100% | 75% |
| Katar | 75% | 100% | 75% |
| Book | 100% | 100% | 50% |
| Knuckle/Claw | 100% | 75% | 50% |
| Instrument | 75% | 100% | 75% |
| Whip | 75% | 100% | 50% |
| Gun | 100% | 100% | 100% |
| Huuma Shuriken | 100% | 100% | 100% |

**Key observations**:
- Daggers excel against Small but are terrible against Large (50%)
- 2H Swords and Spears are best against Large
- Rods, Guns, and Bare Fists have no size penalty
- Maces have no penalty against Medium or Large
- Katars are best against Medium (Assassin PvP advantage)

### 8.3 Bypassing Size Penalty

- **Drake Card** (weapon): Nullifies size penalty (treat all sizes as 100%)
- **Magical attacks**: Ignore size penalty entirely
- **Skills**: Some skills have built-in size bypass (e.g., Asura Strike)

### 8.4 Monster Races

There are 10 monster races in Ragnarok Online:

| Race ID | Race Name | Description | Examples |
|---------|-----------|-------------|---------|
| 0 | Formless | Amorphous / mechanical | Drops, Poring, Clock, Alarm, Apocalypse |
| 1 | Undead | Dead/reanimated creatures | Zombie, Skeleton, Wraith, Dracula |
| 2 | Brute (Animal) | Animals / animalistic monsters | Lunatic, Savage, Wolf, Grizzly |
| 3 | Plant | Vegetation / fungi | Mandragora, Flora, Mushroom, Rafflesia |
| 4 | Insect | Bugs / arthropods | Creamy, Hornet, Ant, Scorpion, Spider |
| 5 | Fish | Aquatic creatures | Swordfish, Marse, Obeaune, Marc |
| 6 | Demon (Devil) | Hellspawn / dark entities | Baphomet, Dark Lord, Succubus, Incubus |
| 7 | Demi-Human | Humanoid monsters / players | Kobold, Orc, all Player Characters |
| 8 | Angel | Divine beings | Angeling, Archangeling, Valkyrie |
| 9 | Dragon | Draconic creatures | Petite, Deleter, Ktullanux, Detale |

### 8.5 Race-Specific Card Bonuses

Cards that grant race-specific damage bonuses:

| Card | Bonus | Target Race |
|------|-------|-------------|
| Hydra | +20% | Demi-Human |
| Orc Skeleton | +20% | Undead |
| Desert Wolf | +15% | Small (size, not race) |
| Goblin | +20% | Brute |
| Caramel | +20% | Insect |
| Mandragora | +20% | Plant |
| Marina | +20% | Fish |
| Strouf | +20% | Demon |
| Pecopeco Egg | +20% | Formless |
| Drainliar | +20% | Undead |
| Scorpion | +20% | Plant |
| Vadon | +20% vs Fire (element-based, not race) | N/A |

### 8.6 Race-Specific Damage Reduction

| Card | Reduction | Against Race |
|------|-----------|-------------|
| Thara Frog | -30% | Demi-Human |
| Khalitzburg | -30% | Demon |
| Orc Lady | -30% | Brute |
| Rafflesia | -30% | Fish |
| Cloud Hermit | -30% | Formless |
| Hode | -30% | Dragon |

---

## 9. Status Effects

### 9.1 General Status Resistance Formula

Pre-Renewal status resistance follows a consistent pattern:

```
FinalChance% = BaseChance - (BaseChance * ResistStat / 100) + srcBaseLevel - tarBaseLevel - tarLUK
```

Where `ResistStat` varies by status effect (VIT, INT, etc.). This means:
- Higher resist stat = lower chance to be inflicted
- Level difference matters (higher-level attackers have better infliction)
- LUK provides universal status resistance

### 9.2 Stun

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, use items, use skills, sit down. FLEE negated. |
| **Duration** | 5,000ms (base) |
| **Resist Stat** | VIT (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * VIT/100 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `5000 - 5000 * VIT/100 - 10 * LUK` (ms) |
| **Immunity** | 97 VIT or 300 LUK |
| **Cures** | Status Recovery (Priest), Battle Chant, Dispell |
| **Common Sources** | Bash Lv6+ (5% per level above 5), Shield Charge, Hammer Fall, Storm Gust |

### 9.3 Freeze

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, or use skills. FLEE negated. Item DEF reduced 50%. MDEF +25%. Armor becomes Water Lv1 (vulnerable to Wind). |
| **Duration** | 12,000ms (base) |
| **Resist Stat** | Hard MDEF (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * HardMDEF/100 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `12000 - 12000 * HardMDEF/100 + 10 * srcLUK` (ms) |
| **Immunity** | 300 LUK, or high enough MDEF |
| **Cures** | Being hit (any damage), Status Recovery, Provoke, Battle Chant |
| **Common Sources** | Frost Diver, Storm Gust, Ice Bolt auto-proc, Cold Bolt |
| **Special** | Frozen targets change to Water element -- Wind attacks deal bonus damage |

### 9.4 Stone Curse

| Property | Value |
|----------|-------|
| **Effects** | Two-phase: (1) Petrifying (3s) -- can still move but cannot attack/skill, can use items, vulnerable to Lex Aeterna. (2) Petrified -- cannot do anything, DEF -50%, MDEF +25%, armor becomes Earth Lv1, lose 1% HP every 5s (minimum 25% HP). |
| **Duration** | 20,000ms (always) once fully petrified |
| **Resist Stat** | Hard MDEF (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * HardMDEF/100 + srcLv - tarLv - tarLUK` |
| **Duration** | Always 20,000ms (no stat reduction) |
| **Immunity** | 300 LUK, or high enough MDEF |
| **Cures** | Being hit (breaks stone), Status Recovery, Blessing, Battle Chant |
| **Common Sources** | Stone Curse (Sage), Medusa gaze, Basilisk |
| **Special** | Petrified targets become Earth element -- Fire attacks deal bonus damage |

### 9.5 Sleep

| Property | Value |
|----------|-------|
| **Effects** | Cannot move, attack, use items, or use skills. Enemies always hit sleeping targets. Critical rate against sleeping targets is doubled. |
| **Duration** | 30,000ms (base) |
| **Resist Stat** | INT (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * INT/100 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `30000 - 30000 * INT/100 - 10 * LUK` (ms) |
| **Immunity** | 97 INT or 300 LUK |
| **Cures** | Being hit (any damage), Battle Chant, Dispell |
| **Common Sources** | Lullaby (Bard), monster skills |

### 9.6 Poison

| Property | Value |
|----------|-------|
| **Effects** | DEF reduced 25%. Lose (HP * 1.5% + 2) per second. Cannot reduce HP below 25%. SP regeneration disabled. |
| **Duration** | 30,000ms (monsters) / 60,000ms (players) base |
| **Resist Stat** | VIT (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * VIT/100 + srcLv - tarLv - tarLUK` |
| **Duration (monsters)** | `30000 - 20000 * VIT/100` (ms) |
| **Duration (players)** | `60000 - 45000 * VIT/100 - 100 * LUK` (ms) |
| **Immunity** | 97 VIT or 300 LUK |
| **Cures** | Green Herb, Green Potion, Panacea, Royal Jelly, Detoxify (Thief), Battle Chant |
| **Common Sources** | Envenom (Thief), Poison React, Venom Dust, poison-property monsters |
| **Special** | If attacker is 10+ levels above target, stat resistance is ignored |

### 9.7 Blind (Darkness)

| Property | Value |
|----------|-------|
| **Effects** | HIT reduced by 25%. FLEE reduced by 25%. Screen darkens significantly (reduced visibility). |
| **Duration** | 30,000ms (base), minimum 15,000ms |
| **Resist Stat** | (INT + VIT) / 2 (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * (INT+VIT)/200 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `30000 - 30000 * (INT+VIT)/200 - 10 * LUK` (ms), minimum 15000 |
| **Immunity** | 193 (VIT+INT combined) or 300 LUK |
| **Cures** | Green Potion, Panacea, Royal Jelly, Cure (Priest), Battle Chant |
| **Common Sources** | Blind Attack, Dark element monsters, Sand Attack (Rogue) |

### 9.8 Silence

| Property | Value |
|----------|-------|
| **Effects** | Cannot use any active skills. Can still move, attack (auto-attack), and use items. |
| **Duration** | 30,000ms (base) |
| **Resist Stat** | VIT (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * VIT/100 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `30000 - 30000 * VIT/100 - 10 * LUK` (ms) |
| **Immunity** | 97 VIT or 300 LUK |
| **Cures** | Green Potion, Panacea, Royal Jelly, Cure (Priest), Lex Divina (toggles), Battle Chant |
| **Common Sources** | Lex Divina (Priest), monster skills |

### 9.9 Confusion (Chaos)

| Property | Value |
|----------|-------|
| **Effects** | Movement direction is randomized. Character moves in unintended directions when trying to navigate. |
| **Duration** | 30,000ms (base) |
| **Resist Stat** | (STR + INT) / 2 (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * (STR+INT)/200 - srcLv + tarLv + tarLUK` |
| **Duration Formula** | `30000 - 30000 * (STR+INT)/200 - 10 * LUK` (ms) |
| **Immunity** | 193 (STR+INT combined) or 300 LUK |
| **Cures** | Being hit (any damage), Cure (Priest), Panacea, Royal Jelly, Battle Chant |
| **Common Sources** | Confusion Attack, monster skills |
| **Note** | Confusion formula has inverted level difference compared to other statuses |

### 9.10 Bleeding

| Property | Value |
|----------|-------|
| **Effects** | Lose HP over time (slower than Poison but **can kill**). Natural HP/SP regeneration disabled. Persists through relog. |
| **Duration** | 120,000ms (base) -- the longest base duration of any status |
| **Resist Stat** | VIT (primary), LUK (secondary) |
| **Chance Formula** | `BaseChance - BaseChance * VIT/100 + srcLv - tarLv - tarLUK` |
| **Duration Formula** | `120000 - 120000 * VIT/100 - 10 * LUK` (ms) |
| **Immunity** | 97 VIT or 300 LUK |
| **Cures** | Dying, Battle Chant, Dispell, Compress (Mercenary) -- very few cures |
| **Common Sources** | Clashing Spiral (Lord Knight), certain monster attacks |
| **Special** | Unlike Poison, Bleeding CAN reduce HP to 0 and kill the target |

### 9.11 Curse

| Property | Value |
|----------|-------|
| **Effects** | ATK reduced by 25%. LUK becomes 0. Movement speed drastically reduced. |
| **Duration** | 30,000ms (base) |
| **Resist Stat** | LUK for chance, VIT for duration |
| **Chance Formula** | `BaseChance - BaseChance * LUK/100 + srcLv - tarLUK` |
| **Duration Formula** | `30000 - 30000 * VIT/100 - 10 * LUK` (ms) |
| **Immunity** | 97 LUK (for chance), 100 VIT (for duration) |
| **Cures** | Blessing (Priest), Holy Water, Panacea, Royal Jelly, Battle Chant |
| **Common Sources** | Curse Attack (Undead monsters), Darkness monsters |
| **Special** | If target has 0 LUK, they cannot be cursed at all |

### 9.12 Status Resistance Summary Table

| Status | Primary Resist | Immunity Threshold | LUK Immunity | Duration (base) |
|--------|---------------|-------------------|-------------|-----------------|
| Stun | VIT | 97 VIT | 300 LUK | 5,000ms |
| Freeze | Hard MDEF | High MDEF | 300 LUK | 12,000ms |
| Stone | Hard MDEF | High MDEF | 300 LUK | 20,000ms (fixed) |
| Sleep | INT | 97 INT | 300 LUK | 30,000ms |
| Poison | VIT | 97 VIT | 300 LUK | 30-60,000ms |
| Blind | (INT+VIT)/2 | 193 combined | 300 LUK | 30,000ms |
| Silence | VIT | 97 VIT | 300 LUK | 30,000ms |
| Confusion | (STR+INT)/2 | 193 combined | 300 LUK | 30,000ms |
| Bleeding | VIT | 97 VIT | 300 LUK | 120,000ms |
| Curse | LUK | 97 LUK | -- | 30,000ms |

---

## 10. Implementation

### 10.1 Architecture Overview

Sabri_MMO implements a **server-authoritative** combat system. All damage calculations, hit/miss rolls, and state changes happen on the Node.js server. The UE5 client is presentation-only -- it receives damage events and plays animations/VFX.

```
Server (Node.js + Socket.io)
  |
  +-- ro_damage_formulas.js     <-- Element table, size penalty, all damage functions
  +-- index.js                  <-- Combat tick loop, auto-attack state, skill execution
  +-- ro_monster_templates.js   <-- 509 monster stat templates
  +-- ro_skill_data.js          <-- Skill definitions, prerequisites, SP costs
  +-- ro_zone_data.js           <-- Zone registry, spawn points
  |
Client (UE5 C++)
  +-- DamageNumberSubsystem     <-- Floating damage numbers
  +-- WorldHealthBarSubsystem   <-- HP/SP bars above characters
  +-- BasicInfoSubsystem        <-- Player HP/SP/EXP panel
  +-- CastBarSubsystem          <-- Cast time bars
  +-- SkillVFXSubsystem         <-- Niagara particle effects
```

### 10.2 Server Combat Tick

The server runs a combat tick at **50ms intervals** (`COMBAT.COMBAT_TICK_MS = 50`). Each tick:

1. **Cast completion check**: Iterates `activeCasts` map. If `now >= cast.castEndTime`, executes the completed skill via `executeCastComplete()`.

2. **Auto-attack processing**: Iterates `autoAttackState` map (Map of `attackerCharId -> { targetCharId, isEnemy, startTime }`).
   - Validates attacker is alive and connected
   - Validates target is alive and in range
   - Checks ASPD timing: `now - lastAttackTime >= aspdDelay`
   - Calculates damage via `roPhysicalDamage()`
   - Broadcasts `combat:damage` event to all players in the zone
   - Handles death, experience, loot drops

3. **Enemy AI attacks**: The enemy AI loop (separate 200ms tick) handles monster-initiated attacks using the same damage formulas.

### 10.3 Auto-Attack State Machine

```
Client clicks enemy -> combat:attack event -> Server validates

Server:
  1. Stores in autoAttackState Map
  2. Each combat tick:
     a. Check range (max attack range + tolerance)
     b. Check ASPD delay elapsed
     c. Roll hit/miss/crit via roPhysicalDamage()
     d. Apply damage, broadcast
     e. Update lastAttackTime

Client receives:
  - combat:damage { attackerId, targetId, damage, hitType, isCritical, element, ... }
  - combat:death { killedId, killerId, ... }
  - combat:target_lost { reason }
  - combat:auto_attack_stopped { reason }
```

### 10.4 Damage Calculation Structure (Node.js)

The `ro_damage_formulas.js` module exports:

```javascript
// Tables
ELEMENT_TABLE     // 10x10x4 element effectiveness
SIZE_PENALTY      // 17-entry weapon type vs size table

// Stat calculations
calculateDerivedStats(stats)    // HIT, FLEE, ASPD, ATK, MATK, DEF, MDEF
calculateHitRate(hit, flee, n)  // Hit chance with multi-attacker penalty
calculateCritRate(cri, luk)     // Effective crit rate vs target LUK

// Damage functions
calculatePhysicalDamage(attacker, target, options)
calculateMagicalDamage(attacker, target, options)

// Lookup helpers
getElementModifier(atkEle, defEle, defLv)
getSizePenalty(weaponType, targetSize)
```

**Physical damage function signature**:
```javascript
calculatePhysicalDamage(
  attacker: {
    stats: { str, agi, vit, int, dex, luk, level },
    weaponATK: number,       // Equipment weapon damage
    passiveATK: number,      // Mastery skill bonuses
    weaponType: string,      // For size penalty lookup
    weaponElement: string,   // Weapon element
    weaponLevel: number,     // 1-4, affects variance
    buffMods: { atkMultiplier },
    cardMods: { race_X, ele_X, size_X }
  },
  target: {
    stats: { str, agi, vit, int, dex, luk, level },
    hardDef: number,         // Equipment DEF (0-99)
    element: { type, level },
    size: string,            // 'small' | 'medium' | 'large'
    race: string,            // 'formless' | 'undead' | ... | 'dragon'
    numAttackers: number,    // For FLEE penalty
    buffMods: { defMultiplier }
  },
  options: {
    isSkill: boolean,
    skillMultiplier: number,  // 100 = 1x
    skillHitBonus: number,
    forceHit: boolean,
    forceCrit: boolean,
    skillElement: string
  }
)
// Returns: { damage, hitType, isCritical, isMiss, element, sizePenalty, elementModifier }
```

### 10.5 Socket Events (Combat-Related)

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `combat:attack` | Client -> Server | `{ targetCharacterId, targetEnemyId }` | Start auto-attacking |
| `combat:stop` | Client -> Server | `{}` | Stop auto-attacking |
| `combat:damage` | Server -> Client | `{ attackerId, targetId, damage, hitType, isCritical, element, isEnemy, targetHealth, targetMaxHealth, attackerName, targetName, targetX, targetY, targetZ }` | Auto-attack damage result |
| `combat:death` | Server -> Client | `{ killedId, killedName, killerId, killerName, isEnemy, targetHealth, targetMaxHealth }` | Entity death |
| `combat:auto_attack_stopped` | Server -> Client | `{ reason }` | Auto-attack ended |
| `combat:target_lost` | Server -> Client | `{ reason, isEnemy }` | Target no longer valid |
| `skill:use` | Client -> Server | `{ skillId, targetId, isEnemy, groundX/Y/Z }` | Cast a skill |
| `skill:cast_start` | Server -> Client | `{ characterId, skillId, castTime, targetId }` | Cast bar begins |
| `skill:cast_complete` | Server -> Client | `{ characterId, skillId }` | Cast finished |
| `skill:cast_interrupted` | Server -> Client | `{ characterId, reason }` | Cast was interrupted |
| `skill:effect_damage` | Server -> Client | `{ attackerId, targetId, skillId, damage, hitType, element, isEnemy, targetHealth, targetMaxHealth, hitNumber, totalHits, targetX, targetY, targetZ }` | Skill damage per hit |
| `skill:buff_applied` | Server -> Client | `{ targetId, buffType, duration }` | Buff/debuff applied |
| `skill:buff_removed` | Server -> Client | `{ targetId, buffType }` | Buff/debuff removed |
| `enemy:attack` | Server -> Client | `{ enemyId, targetId, attackMotion }` | Monster attack animation |

### 10.6 UE5 Client Animation/VFX

The client subscribes to combat events via C++ subsystems:

- **DamageNumberSubsystem**: Listens to `combat:damage` and `skill:effect_damage`. Projects floating damage numbers at target world position using `targetX/Y/Z`. Color-coded: white = normal, yellow = critical, gray = miss.

- **WorldHealthBarSubsystem**: Listens to `combat:damage`, `skill:effect_damage`, and `player:stats`. Renders floating HP/SP bars using Slate `OnPaint` with world-to-screen projection. RO Classic colors: navy border, green HP, blue SP, pink enemy HP.

- **CastBarSubsystem**: Listens to `skill:cast_start/complete/interrupted`. Shows world-projected cast bars below casting characters.

- **SkillVFXSubsystem**: Listens to `skill:effect_damage`, `skill:buff_applied`, `skill:buff_removed`. Spawns Niagara/Cascade particle effects. Five VFX patterns: Bolt (sky projectiles), AoE Projectile, Multi-Hit Projectile, Persistent Buff, Ground AoE Rain.

### 10.7 Event Separation

A critical implementation detail: **auto-attack damage and skill damage use separate events**.

- `combat:damage` = auto-attack ONLY. Triggers Blueprint attack animation on the client.
- `skill:effect_damage` = ALL skill damage. Does NOT trigger attack animation (skills have their own VFX).

Both events carry identical damage payload structure. C++ subsystems (DamageNumber, BasicInfo, WorldHealthBar) listen to **both** events to correctly update HP bars and show damage numbers regardless of source.

### 10.8 Cast Time System

Pre-Renewal cast time formula:
```
FinalCastTime = BaseCastTime * (1 - DEX / 150)
```

Instant cast at **150 DEX** (no equipment bonuses needed -- pure stat-based in pre-Renewal).

Server implementation:
1. `skill:use` received -> validate skill, SP, cooldown, range
2. Calculate `actualCastTime` from base cast time and DEX
3. If `actualCastTime > 0`: store in `activeCasts` map with `castEndTime = now + actualCastTime`
4. Broadcast `skill:cast_start` to all players in zone
5. Combat tick checks `activeCasts` -- when `now >= castEndTime`, calls `executeCastComplete()`
6. Cast interruption: movement > 5 UE units or taking damage clears the cast and broadcasts `skill:cast_interrupted`

---

## Sources

- iRO Wiki Classic: https://irowiki.org/classic/
  - [ATK](https://irowiki.org/wiki/ATK)
  - [MATK](https://irowiki.org/wiki/MATK)
  - [DEF](https://irowiki.org/classic/DEF)
  - [MDEF](https://irowiki.org/wiki/MDEF)
  - [FLEE](https://irowiki.org/classic/FLEE)
  - [ASPD](https://irowiki.org/classic/ASPD)
  - [Stats](https://irowiki.org/classic/Stats)
  - [Element](https://irowiki.org/classic/Element)
  - [Size](https://irowiki.org/classic/Size)
  - [Attacks](https://irowiki.org/classic/Attacks)
  - [Status Effects](https://irowiki.org/classic/Status_Effects)
  - [Refinement System](https://irowiki.org/classic/Refinement_System)
  - [Perfect Dodge](https://irowiki.org/classic/Perfect_Dodge)
- RateMyServer: https://ratemyserver.net/
  - [Element Table](https://ratemyserver.net/index.php?page=misc_table_attr)
  - [Size Table](https://ratemyserver.net/index.php?page=misc_table_size)
  - [Status Resistance Formulas (Pre-Renewal)](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- rAthena Pre-Renewal Source: https://github.com/rathena/rathena
- Ragnarok Wiki (Fandom): https://ragnarok.fandom.com/wiki/Stats_(RO)
