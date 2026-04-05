# Physical Damage Formula -- Deep Research (Pre-Renewal)

> Comprehensive reference for Ragnarok Online Classic (pre-Renewal) physical damage calculation.
> Cross-referenced from: iRO Wiki Classic, rAthena pre-re battle.cpp, Hercules pre-re, RateMyServer, WarpPortal forums, ROClassicGuide, divine-pride.net.
> All values are **pre-Renewal only**. Renewal changed nearly every formula listed here.

---

## Table of Contents

1. [Overview](#overview)
2. [StatusATK Calculation](#statusatk-calculation)
3. [WeaponATK Calculation](#weaponatk-calculation)
4. [Total ATK = StatusATK + WeaponATK](#total-atk--statusatk--weaponatk)
5. [DEF Reduction](#def-reduction)
6. [Size Modifier](#size-modifier)
7. [Element Modifier](#element-modifier)
8. [Race Modifier](#race-modifier)
9. [Critical Hits](#critical-hits)
10. [Skill Damage Modifiers](#skill-damage-modifiers)
11. [Card and Equipment Modifiers](#card-and-equipment-modifiers)
12. [Mastery Bonuses](#mastery-bonuses)
13. [Damage Variance](#damage-variance)
14. [Minimum Damage Rules](#minimum-damage-rules)
15. [Star Crumb and Spirit Sphere Bonuses](#star-crumb-and-spirit-sphere-bonuses)
16. [Dual Wield Penalties](#dual-wield-penalties)
17. [Perfect Dodge vs Normal Dodge](#perfect-dodge-vs-normal-dodge)
18. [Full Damage Pipeline](#full-damage-pipeline)
19. [Implementation Checklist](#implementation-checklist)
20. [Gap Analysis](#gap-analysis)

---

## Overview

Physical damage in pre-Renewal RO is calculated through a multi-step pipeline. Every intermediate result is **floored** (truncated to integer) immediately -- decimals never persist between steps.

The five components of ATK (per iRO Wiki):

| Component | Description | Element-affected? | DEF-reduced? | Card-amplified? |
|-----------|-------------|-------------------|--------------|-----------------|
| **StatusATK** | Stat-derived base damage (STR/DEX/LUK/BaseLv) | Yes (weapon element) | Yes | No |
| **WeaponATK** | Weapon base ATK with variance | Yes (weapon element) | Yes | Yes |
| **ExtraATK** | Equipment ATK, consumable ATK, ammo ATK | Yes | Yes | Yes |
| **MasteryATK** | Passive skill bonuses (Sword Mastery, etc.) + Star Crumbs | **No** (no element) | **No** (bypasses DEF) | **No** |
| **BuffATK** | Active buff bonuses (Impositio Manus, etc.) | **No** (no element) | **No** (bypasses DEF) | **No** |

**Critical insight**: MasteryATK and BuffATK are **not** subject to element modifiers, size penalties, card percentage bonuses, or DEF reductions. They are flat damage added at the end of the pipeline. This is why weapon mastery skills and Star Crumb bonuses are described as "seeking damage" -- they always deal their full value regardless of target properties.

**Pipeline summary** (detailed in [Full Damage Pipeline](#full-damage-pipeline)):
```
1. Perfect Dodge check
2. Critical check
3. HIT/FLEE check
4. Calculate StatusATK
5. Calculate WeaponATK (with variance)
6. Apply size penalty to WeaponATK
7. Calculate ExtraATK
8. Sum: StatusATK + SizedWeaponATK + ExtraATK
9. Apply skill multiplier
10. Apply card modifiers (race + element + size)
11. Apply element modifier
12. Apply hard DEF (percentage reduction)
13. Subtract soft DEF (flat subtraction)
14. Add MasteryATK + BuffATK
15. Apply critical multiplier (if critical)
16. Floor to minimum 1
```

---

## StatusATK Calculation

StatusATK is the damage contribution from raw character stats. The formula differs by weapon class.

### Melee Weapons
Daggers, Swords, Axes, Maces, Spears, Katars, Knuckles, Books:

```
StatusATK = floor(BaseLv / 4) + STR + floor(STR / 10)^2 + floor(DEX / 5) + floor(LUK / 3)
```

**Note**: Some sources separate the `STR + floor(STR/10)^2` term as the "STR bonus" and present it as:
```
StatusATK = floor(BaseLv / 4) + StatusSTR + floor(DEX / 5) + floor(LUK / 3)
where StatusSTR = STR + floor(STR / 10)^2
```

### Ranged Weapons
Bows, Guns, Instruments, Whips:

```
StatusATK = floor(BaseLv / 4) + DEX + floor(DEX / 10)^2 + floor(STR / 5) + floor(LUK / 3)
```

### STR/DEX Bonus Breakpoints

The `floor(X/10)^2` term creates breakpoints every 10 points of the primary stat:

| Stat Value | floor(X/10)^2 | Marginal Gain |
|-----------|---------------|---------------|
| 1-9 | 0 | -- |
| 10-19 | 1 | +1 at 10 |
| 20-29 | 4 | +3 at 20 |
| 30-39 | 9 | +5 at 30 |
| 40-49 | 16 | +7 at 40 |
| 50-59 | 25 | +9 at 50 |
| 60-69 | 36 | +11 at 60 |
| 70-79 | 49 | +13 at 70 |
| 80-89 | 64 | +15 at 80 |
| 90-99 | 81 | +17 at 90 |

This means the last few points of STR before a breakpoint (e.g., 89->90) provide a large damage spike from the bonus term alone.

### BaseLv / 4 Component

The `floor(BaseLv / 4)` term adds a small amount of StatusATK from character level alone:

| BaseLv | Contribution |
|--------|-------------|
| 1 | 0 |
| 20 | 5 |
| 50 | 12 |
| 80 | 20 |
| 99 | 24 |

### Element Behavior of StatusATK

StatusATK is subject to the weapon's element modifier. If the weapon is Neutral, StatusATK damage is Neutral. If endowed with Fire (via Aspersio, Sage endow, etc.), StatusATK is treated as Fire element for the element table lookup.

**Exception**: In rAthena source, StatusATK was historically treated as having a flag `NK_NO_ELEFIX` in some codepaths, meaning it would NOT be affected by elemental modifiers. This was corrected in later rAthena commits. The canonical iRO Classic behavior is that StatusATK IS affected by element.

---

## WeaponATK Calculation

WeaponATK is the damage from the equipped weapon, subject to random variance based on weapon level and DEX.

### Base Weapon ATK

The weapon's base ATK is the value listed in the item database (e.g., a Stiletto has 73 ATK, a Katana has 60 ATK).

### WeaponATK Variance (DEX Narrowing)

**Melee (normal hit)**:
```
MinWeaponATK = floor(WeaponBaseATK * (0.8 + 0.2 * WeaponLevel) * DEX / MaxDEX_for_weapon)
```

More precisely, the iRO Wiki formula:
```
WeaponATK = rnd(floor(min(DEX * (0.8 + 0.2 * WeaponLevel), WeaponBaseATK)), WeaponBaseATK)
```

The minimum ATK is `DEX * (0.8 + 0.2 * WeaponLevel)` clamped to the weapon's base ATK. Higher DEX narrows the range between min and max, meaning more consistent damage. When DEX is high enough that the minimum equals the maximum, there is no variance.

**Melee (critical hit)**:
```
WeaponATK = WeaponBaseATK   (always maximum, no variance)
```

**Ranged (bows)**:
```
WeaponATK = rnd(floor(WeaponBaseATK * min(WeaponBaseATK, DEX * (0.8 + 0.2 * WeaponLevel)) / 100),
                WeaponBaseATK)
           + rnd(0, ArrowATK - 1)
```

Arrow ATK is added as a separate random term with its own variance.

### Refinement (Upgrade) Bonuses

Each refinement level adds flat ATK to the weapon:

| Weapon Level | ATK per Refine | Safe Limit |
|-------------|---------------|------------|
| Level 1 | +2 | +7 |
| Level 2 | +3 | +6 |
| Level 3 | +5 | +5 |
| Level 4 | +7 | +4 |

**Example**: A +10 Level 4 weapon gains:
- Base refinement: 10 * 7 = 70 ATK
- This is the guaranteed flat bonus shown in the status window

### Overupgrade Bonus (Random ATK Beyond Safe Limit)

When a weapon is refined past the safe limit, each level beyond safe adds a **random** ATK bonus per hit:

| Weapon Level | Safe Limit | Random ATK per Over-Upgrade Level | Max at +10 |
|-------------|------------|-----------------------------------|------------|
| Level 1 | +7 | 0 ~ 3 per level | 3 levels * 0~3 = 0~9 |
| Level 2 | +6 | 0 ~ 5 per level | 4 levels * 0~5 = 0~20 |
| Level 3 | +5 | 0 ~ 8 per level | 5 levels * 0~8 = 0~40 |
| Level 4 | +4 | 0 ~ 14 per level | 6 levels * 0~14 = 0~84 |

**Key properties of over-upgrade bonus**:
- Random for every hit (rolled independently each attack)
- Minimum is 0 (can add nothing)
- NOT shown in the status window
- NOT affected by Maximize Power or critical hit max-ATK behavior
- NOT affected by size penalty or element modifiers

The over-upgrade bonus is part of MasteryATK in rAthena's internal classification, meaning it bypasses all damage modifiers. It is pure flat damage added at the end.

### ExtraATK (Equipment ATK)

ExtraATK combines several sub-components:
- **EquipATK**: Flat +ATK from cards and equipment bonuses (e.g., Andre Card +20 ATK)
- **ConsumableATK**: +ATK from active consumables
- **AmmunitionATK**: ATK from equipped arrows/ammunition
- **PseudoBuffATK**: ATK from certain pseudo-buff effects

ExtraATK IS affected by element modifiers, size penalty, and card percentage bonuses, unlike MasteryATK.

---

## Total ATK = StatusATK + WeaponATK

The pre-Renewal ATK formula (simplified, before modifiers):

```
RawATK = StatusATK + WeaponATK + ExtraATK
```

This raw ATK is then run through the modifier pipeline: skill multiplier, card bonuses, element modifier, DEF reduction, and finally MasteryATK/BuffATK are added.

### Status Window Display

The status window shows ATK as two numbers: `A + B`

- **A** (left number): StatusATK + EquipATK + refine bonus + card flat ATK. This is the "guaranteed minimum" portion.
- **B** (right number): WeaponATK variance range. This represents the random damage spread.

```
DisplayATK_Left = StatusATK + FlatEquipBonuses + RefineATK
DisplayATK_Right = WeaponBaseATK
```

---

## DEF Reduction

Physical damage is reduced by two layers of defense, applied in order.

### Hard DEF (Equipment DEF) -- Percentage Reduction

Hard DEF is a **percentage-based** reduction from equipment (armor, shield, helm, garment, footgear, accessories). It is applied first.

**iRO Wiki Classic formula** (canonical):
```
DamageAfterHardDEF = floor(Damage * (4000 + HardDEF) / (4000 + HardDEF * 10))
```

| HardDEF | Effective Reduction |
|---------|-------------------|
| 0 | 0.0% |
| 10 | ~2.4% |
| 25 | ~5.7% |
| 50 | ~11.1% |
| 100 | ~18.5% |
| 150 | ~24.1% |
| 200 | ~28.6% |
| 300 | ~35.0% |
| 500 | ~50.0% |
| 600 | ~50.0% |

**rAthena simplified formula** (used by many private servers):
```
DamageAfterHardDEF = floor(Damage * (100 - HardDEF) / 100)
```
Where HardDEF is capped at 99 (99% max reduction). This is a simpler linear model.

**Which to use**: The iRO Wiki (4000+DEF) formula is the official pre-Renewal behavior. The simplified formula is a common private server approximation. For accurate classic emulation, use the 4000-based formula.

### Armor Refinement DEF Bonus

Each +1 refinement on armor adds approximately:
```
ArmorRefineDEF = floor((3 + RefineLv) / 4)
```

| Refine Level | DEF Bonus |
|-------------|-----------|
| +1 | 1 |
| +2 | 1 |
| +3 | 1 |
| +4 | 1 |
| +5 | 2 |
| +6 | 2 |
| +7 | 2 |
| +8 | 2 |
| +9 | 3 |
| +10 | 3 |

### Soft DEF (VIT-based DEF) -- Flat Subtraction

Soft DEF is a **flat subtraction** from damage, applied after Hard DEF.

**Primary formula (iRO Wiki Classic)**:
```
SoftDEF = floor(VIT * 0.5) + max(floor(VIT * 0.3), floor(VIT^2 / 150) - 1)
```

The `VIT^2 / 150` term becomes dominant at high VIT, creating accelerating returns:

| VIT | floor(VIT*0.5) | max(floor(VIT*0.3), floor(VIT^2/150)-1) | Total SoftDEF |
|-----|---------------|----------------------------------------|---------------|
| 1 | 0 | 0 | 0 |
| 10 | 5 | 3 | 8 |
| 20 | 10 | 6 | 16 |
| 30 | 15 | 9 | 24 |
| 50 | 25 | 15 | 40 |
| 70 | 35 | 31 | 66 |
| 80 | 40 | 41 | 81 |
| 90 | 45 | 53 | 98 |
| 99 | 49 | 64 | 113 |

**Random component** (some implementations):
```
SoftDEF = floor(VIT * 0.5) + rnd(floor(VIT * 0.3), max(floor(VIT * 0.3), floor(VIT^2 / 150) - 1))
```

In this variant, the second term is randomized between the VIT*0.3 floor and the VIT^2/150 ceiling, adding damage variance from the defender's side.

### DEF Application Order

```
1. Start with raw damage after all offensive modifiers
2. Apply Hard DEF (percentage reduction): damage = floor(damage * (4000 + hardDEF) / (4000 + hardDEF * 10))
3. Subtract Soft DEF (flat): damage = damage - softDEF
4. Floor to minimum 1
```

### DEF Bypass Mechanics

| Mechanic | Hard DEF | Soft DEF |
|----------|----------|----------|
| Critical hits (pre-Renewal) | **BYPASSED** | **BYPASSED** |
| Ice Pick weapon | Inverts (more DEF = more damage) | Inverts |
| Occult Impaction / Investigate (Monk) | Inverts (like Ice Pick) | Inverts |
| Provoke (on target) | Reduced by 5-25% | Unaffected |
| Armor Break / Full Strip | Reduced to 0 | Unaffected |
| Steel Body (on self) | Set to 90 hard DEF equivalent | Set to very high |
| Magic attacks | Ignored (uses MDEF) | Ignored (uses MDEF) |

### Multi-Monster DEF Penalty

When attacked by more than 2 monsters simultaneously:
```
EffectiveDEF = DEF * (1 - (NumAttackers - 2) * 0.05)
```

Each additional attacker beyond 2 reduces effective DEF by 5%.

---

## Size Modifier

Size penalty applies only to the **WeaponATK** portion of physical damage. StatusATK and MasteryATK are NOT affected.

```
SizedWeaponATK = floor(WeaponATK * SizePenalty% / 100)
```

### Complete Weapon-vs-Size Penalty Table

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
| Rod / Staff | 100% | 100% | 100% |
| Bow | 100% | 100% | 75% |
| Katar | 75% | 100% | 75% |
| Book | 100% | 100% | 50% |
| Knuckle / Claw | 100% | 75% | 50% |
| Instrument | 75% | 100% | 75% |
| Whip | 75% | 100% | 50% |
| Gun | 100% | 100% | 100% |
| Huuma Shuriken | 100% | 100% | 100% |

### Size Penalty Notes

- Players are always **Medium** size (baby/adopted characters are **Small**)
- Size penalty is one of the biggest sources of damage loss -- a dagger vs Large monster deals only 50% WeaponATK
- **Drake Card** (weapon card): Nullifies size penalty entirely (all sizes treated as 100%)
- **Magical attacks**: Completely ignore size penalty
- **Some skills**: Built-in size bypass (e.g., Maximize Power forces 100% weapon damage)

---

## Element Modifier

The attacking element is compared against the defending element using a 10x10x4 lookup table. All damage components except MasteryATK and BuffATK are affected.

```
ElementalDamage = floor(TotalATK * ElementModifier% / 100)
```

### 10 Elements
Neutral, Water, Earth, Fire, Wind, Poison, Holy, Shadow, Ghost, Undead

### 4 Element Levels
Higher levels create stronger resistances and vulnerabilities. Most players are Neutral Lv1. Monsters range from Lv1 to Lv4.

### Key Element Interactions

**Elemental cycle** (strong against next):
```
Water > Fire > Earth > Wind > Water
```

**Critical interactions**:
- Ghost element (Lv2+): Immune to Neutral attacks (0% damage)
- Holy vs Undead: Up to 200% damage at Lv4
- Attacking same element: Reduced/zero damage (Fire vs Fire Lv1 = 25%, Lv2+ = 0%)
- Poison/Shadow vs Undead: Negative values = **heals** the target

### Weapon Element Sources

Priority order for determining attack element:
1. **Endow skills** (Sage/Priest): Fire/Water/Wind/Earth/Holy
2. **Arrow element** (ranged only): Non-Neutral arrow overrides weapon
3. **Card element** (weapon card): Drops Card = Fire, Mandragora = Earth, etc.
4. **Forged element** (Blacksmith): Element stone imbued during creation
5. **Default**: Neutral (if none of the above)

See `RagnaCloneDocs/02_Combat_System.md` Section 7 for the complete 10x10x4 element table.

---

## Race Modifier

Race-specific damage bonuses from cards and equipment. There are 10 races:

| Race ID | Name | Card Example | Bonus |
|---------|------|-------------|-------|
| 0 | Formless | Peco Peco Egg Card | +20% |
| 1 | Undead | Orc Skeleton Card | +20% |
| 2 | Brute | Goblin Card | +20% |
| 3 | Plant | Flora Card | +20% |
| 4 | Insect | Caramel Card | +20% |
| 5 | Fish | Hydra Card (wrong -- see below) | -- |
| 6 | Demon | Strouf Card | +20% |
| 7 | Demi-Human | Hydra Card | +20% |
| 8 | Angel | Orc Lady Card | +20% |
| 9 | Dragon | Dragonfly Card | +20% |

Race modifiers stack additively with each other, then multiply with other modifier types (element, size).

---

## Critical Hits

### Critical Rate Calculation

```
StatusCritical = 1 + floor(LUK * 0.3)
EquipCritical = sum of all +CRIT equipment/card bonuses
TotalCritical = StatusCritical + EquipCritical
```

### Critical Shield (Target's Crit Reduction)

```
CritShield = floor(TargetLUK / 5)
EffectiveCritRate = TotalCritical - CritShield
```

Each 5 LUK on the target reduces incoming crit rate by 1%.

### Katar Critical Bonus

Katar weapons **double** the displayed critical rate:
```
KatarCritical = TotalCritical * 2
EffectiveCritRate = KatarCritical - CritShield
```

### Critical Hit Properties (Pre-Renewal)

In pre-Renewal, critical hits have these properties:

1. **Maximum WeaponATK**: No variance -- always use highest weapon damage
2. **Bypass FLEE**: HIT/FLEE check is skipped entirely
3. **Bypass Perfect Dodge**: PD check is skipped (critical always lands)
4. **Bypass Hard DEF**: Equipment DEF is **ignored** (iRO Wiki Classic confirms this)
5. **Bypass Soft DEF**: VIT-based DEF is **ignored** (iRO Wiki Classic confirms this)
6. **40% damage bonus**: Final damage is multiplied by 1.4x

**Source confirmation**: iRO Wiki Classic Stats page explicitly states: "Critical Hit Rate does damage that fully ignores enemy DEF, both % and pure value part." This was changed in Renewal, where criticals no longer bypass DEF and instead deal +40% damage with DEF still applied.

### Critical Damage Formula (Pre-Renewal)

```
CritDamage = floor((StatusATK + MaxWeaponATK + ExtraATK) * SkillMul * CardMod * EleMod * 1.4)
           + MasteryATK + BuffATK
```

Because Hard DEF and Soft DEF are both bypassed, the damage is significantly higher than a normal hit, especially against high-DEF targets.

### Skills and Criticals

In pre-Renewal, **skills cannot critically hit** with very few exceptions:
- Auto-attacks can crit
- Double Attack (Thief passive) can proc alongside crits
- Some later skills in 3rd class patches may crit
- Katar Auto Counter (Assassin Cross) is a forced-crit counter-attack

---

## Skill Damage Modifiers

Skills apply a percentage multiplier to the base damage:

```
SkillDamage = floor(RawATK * SkillMultiplier% / 100)
```

### Per-Skill Multiplier Examples

| Skill | Multiplier Formula | Lv10 Value |
|-------|--------------------|------------|
| Bash | 100% + 30% * Lv | 400% |
| Magnum Break | 100% + 20% * Lv | 300% |
| Pierce | (100% + 10% * Lv) * SizeMul | varies |
| Bowling Bash | 100% + 40% * Lv | 500% (x2 hits) |
| Brandish Spear | varies by zone | 562% inner at Lv10 |
| Sonic Blow | 100% + 40% * Lv | 500% (x8 hits) |
| Spiral Pierce | Based on weapon weight | varies |
| Asura Strike | Based on SP, STR, ATK | varies |
| Double Strafe | 100% + 90% * Lv (2 hits) | 190% x2 = 380% |
| Arrow Shower | 75% + 5% * Lv | 125% |

### Skill Multiplier Application Point

The skill multiplier is applied **after** StatusATK + SizedWeaponATK + ExtraATK are combined, but **before** card bonuses, element modifier, and DEF reduction. This means the skill multiplier amplifies the base damage which is then further modified by all subsequent steps.

---

## Card and Equipment Modifiers

### Offensive Card Modifier Categories

Cards provide percentage-based damage bonuses organized by target property. The fundamental stacking rule:

**Same category = additive. Different categories = multiplicative.**

The categories:
1. **Race bonus** (e.g., Hydra +20% vs Demi-Human)
2. **Element bonus** (e.g., Vadon +20% vs Fire)
3. **Size bonus** (e.g., Minorous +15% vs Large)
4. **Class bonus** (e.g., Abysmal Knight +25% vs Boss)
5. **Specific monster** (e.g., Turtle General +20% vs all)

### Stacking Formula

```
TotalCardMultiplier = (1 + SumRaceBonus) * (1 + SumEleBonus) * (1 + SumSizeBonus) * (1 + SumClassBonus)
CardDamage = floor(Damage * TotalCardMultiplier)
```

### Examples

**4x Hydra Card** (all race, same category):
```
(1.00 + 0.20 + 0.20 + 0.20 + 0.20) = 1.80x = 80% more damage vs Demi-Human
```

**2x Hydra + 2x Skeleton Worker** (race + size, different categories):
```
(1.00 + 0.20 + 0.20) * (1.00 + 0.15 + 0.15) = 1.40 * 1.30 = 1.82x
```

**1 Hydra + 1 Vadon + 1 Skeleton Worker** (race + element + size):
```
(1.00 + 0.20) * (1.00 + 0.20) * (1.00 + 0.15) = 1.20 * 1.20 * 1.15 = 1.656x
```

### Defensive Card Modifiers

Defensive cards reduce incoming damage, also organized by category:

| Slot | Type | Examples |
|------|------|---------|
| Shield | Race reduction | Thara Frog: -30% from Demi-Human |
| Garment | Element reduction | Raydric: -20% from Neutral |
| Armor | Element property change | Pasana: Fire Lv1 armor |
| Footgear | Stat bonuses | Verit: +8% MaxHP |

Defensive reductions are applied multiplicatively:
```
DamageReceived = floor(Damage * (1 - ShieldReduction) * (1 - GarmentReduction) * ...)
```

### Application Order

Card bonuses (offensive) are applied after the skill multiplier but before element modifier and DEF:
```
... -> Skill Multiplier -> Card Bonuses -> Element Modifier -> DEF -> Mastery ...
```

---

## Mastery Bonuses

MasteryATK is flat damage from passive skills. It is **NOT** affected by element modifiers, size penalties, card percentage bonuses, or DEF reductions. It is added after all multiplicative steps.

### Weapon Mastery Skills

| Skill | Class | Bonus per Level | Max Bonus | Applies To |
|-------|-------|-----------------|-----------|------------|
| Sword Mastery | Swordsman | +4 ATK | +40 (Lv10) | 1H Swords, Daggers |
| Two-Handed Sword Mastery | Swordsman | +4 ATK | +40 (Lv10) | 2H Swords |
| Spear Mastery | Knight/Crusader | +4 ATK (foot) / +5 ATK (mounted) | +40/+50 (Lv10) | Spears |
| Katar Mastery | Assassin | +3 ATK | +30 (Lv10) | Katars |
| Axe Mastery | Alchemist | +3 ATK | +30 (Lv10) | Axes |
| Advanced Book | Sage | +3 ATK (also ASPD) | +30 (Lv10) | Books |
| Weaponry Research | Blacksmith | +2 ATK | +20 (Lv10) | All weapons |
| Hilt Binding | Blacksmith | +1 ATK | +1 (Lv1) | All weapons |
| Beast Bane | Swordsman | +4 ATK | +40 (Lv10) | vs Brute and Insect only |

### Other Mastery Sources

| Source | Bonus |
|--------|-------|
| Star Crumb (forged weapon) | +5 / +10 / +40 (see below) |
| Spirit Spheres (Monk) | +3 ATK per sphere |
| Overupgrade bonus | 0~random per hit (see WeaponATK section) |

### Mastery Non-Stacking Rule

When a character has multiple mastery passives from different classes (e.g., Rogue with Sword Mastery from Swordsman and its own passive), the game uses `Math.max()` -- only the highest value applies, they do not stack.

---

## Damage Variance

In addition to the WeaponATK variance from DEX narrowing, there is a final +-5% variance applied to the entire damage:

### Weapon Level Variance

```
VarianceRange = WeaponLevel * 5%
MinMul = 1.0 - (WeaponLevel * 0.05)
MaxMul = 1.0
ActualMul = rnd(MinMul, MaxMul)
VariedDamage = floor(Damage * ActualMul)
```

| Weapon Level | Variance Range | Example (100 base) |
|-------------|----------------|-------------------|
| Level 1 | 95% -- 100% | 95 -- 100 |
| Level 2 | 90% -- 100% | 90 -- 100 |
| Level 3 | 85% -- 100% | 85 -- 100 |
| Level 4 | 80% -- 100% | 80 -- 100 |

### Critical Hit Variance

Critical hits **skip** the damage variance -- they always deal maximum weapon damage (no random roll).

### Skill Variance

Most skills still apply weapon variance. Some skills with fixed damage (e.g., Asura Strike) may have reduced or no variance.

---

## Minimum Damage Rules

### Standard Minimum

The final damage after all reductions is floored to **minimum 1**:
```
FinalDamage = max(1, CalculatedDamage)
```

This means even against extremely high-DEF targets, at least 1 damage is dealt per hit.

### Exception: Element Immunity

When the element modifier is 0% or negative (e.g., Fire vs Fire Lv2 = 0%, Neutral vs Ghost Lv2+ = 0%), the damage is truly **0** -- the minimum 1 rule does NOT apply. The attack shows as a MISS or 0 damage.

### Exception: Perfect Dodge / MISS

When the attack is dodged (Perfect Dodge or FLEE miss), no damage is dealt at all.

---

## Star Crumb and Spirit Sphere Bonuses

### Star Crumbs (Forged Weapons)

Weapons forged by a Blacksmith can include Star Crumbs. The damage bonus is:

| Star Crumbs Used | Flat Damage Bonus |
|-----------------|-------------------|
| 1 | +5 per hit |
| 2 | +10 per hit |
| 3 | +40 per hit |

**Properties**:
- Cannot miss (always deals this damage on hit)
- Ignores DEF (not reduced by Hard or Soft DEF)
- Ignores element modifiers (no elemental interaction)
- Ignores size penalty
- Classified as MasteryATK in rAthena
- Stacks with regular weapon damage
- The +40 for 3 crumbs (not +15) is intentional -- it is a special bonus for using the maximum number

### Spirit Spheres (Monk)

Each Spirit Sphere adds +3 flat ATK:
```
SpiritSphereATK = NumSpheres * 3
```

| Spheres | Bonus ATK |
|---------|-----------|
| 1 | +3 |
| 2 | +6 |
| 3 | +9 |
| 4 | +12 |
| 5 | +15 |

**Properties**:
- Added as part of passiveATK/MasteryATK
- Bypasses DEF and element (same as mastery bonuses)
- Spheres are consumed by certain skills (Asura Strike uses all 5, Investigate uses 1)
- Spheres expire after 600 seconds (10 minutes)
- Maximum 5 spheres at a time

---

## Dual Wield Penalties

Only Assassin and Assassin Cross classes can dual-wield weapons.

### Base Dual Wield Penalty

When dual-wielding without any mastery skills:
- **Right hand**: 50% of normal damage
- **Left hand**: 30% of normal damage

### Righthand Mastery (Passive)

Restores right-hand damage:

| Level | Right Hand Damage |
|-------|-------------------|
| 1 | 60% |
| 2 | 70% |
| 3 | 80% |
| 4 | 90% |
| 5 | 100% |

### Lefthand Mastery (Passive)

Restores left-hand damage:

| Level | Left Hand Damage |
|-------|------------------|
| 1 | 40% |
| 2 | 50% |
| 3 | 60% |
| 4 | 70% |
| 5 | 80% |

Note: Even at max Lefthand Mastery (Lv5), the left hand only deals 80% damage, never reaching 100%.

### Dual Wield Mechanics

- Both weapons swing per auto-attack cycle (two hits per attack)
- Each hand uses its own weapon ATK, cards, and element
- The left-hand attack is always counted as a **skill attack** (even though it looks like a normal attack)
- This means the left-hand attack cannot critically hit independently
- If the right hand crits, the crit display shows on both hits, but only the right hand actually gets crit bonuses
- ASPD for dual wield: `DualBTBA = 0.7 * (BTBA_MainHand + BTBA_OffHand)`
- Skills always use the **right hand** weapon only (left hand is ignored for skill damage)

### Dual Wield Damage Formula

```
RightHandDamage = NormalDamage * (RighthandMasteryPct / 100)
LeftHandDamage = NormalDamage * (LefthandMasteryPct / 100)
TotalDamage = RightHandDamage + LeftHandDamage
```

Each hand calculates damage independently with its own weapon, then applies the mastery percentage.

---

## Perfect Dodge vs Normal Dodge

### Check Order

```
1. Is it a critical hit? -> Skip BOTH PD and FLEE checks (always hits)
2. Is it a skill attack? -> Skip PD check (skills cannot be Perfect Dodged)
3. Perfect Dodge check -> LUK-based chance
4. FLEE check -> HIT vs FLEE accuracy roll
```

### Perfect Dodge (Lucky Dodge)

```
PerfectDodge = 1 + floor(LUK / 10) + BonusPD_from_equipment
```

Each point of PD = 1% chance to completely avoid the attack.

**Properties**:
- Checked before normal FLEE
- **Not reduced** by multi-monster penalty
- Critical hits **bypass** PD
- Skill attacks **bypass** PD (only auto-attacks can be PD'd)
- No theoretical cap (100% PD is possible with enough LUK + equipment)
- Triggers a "Lucky" display on the client

### Normal Dodge (FLEE)

```
FLEE = 100 + BaseLv + AGI + BonusFLEE_from_equipment_and_skills
HitRate = 80 + AttackerHIT - DefenderFLEE
HitRate = clamp(HitRate, 5, 95)
```

**Properties**:
- Checked after PD
- Clamped between 5% (always at least 5% chance to hit) and 95% (never 100% accuracy from HIT alone)
- **Reduced** by multi-monster penalty: -10% base FLEE per attacker beyond 2
- Critical hits bypass FLEE entirely
- ForceHit skills (e.g., Bash) bypass FLEE

### Multi-Monster FLEE Penalty

```
FLEEPenalty = (NumAttackers - 2) * 10%   (only if NumAttackers > 2)
EffectiveFLEE = BaseFLEE * (1 - FLEEPenalty)
```

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

Skill-granted FLEE (e.g., Improve Dodge) is added **after** the penalty, not before.

### HIT Calculation

```
PlayerHIT = 175 + BaseLv + DEX + BonusHIT
MonsterHIT = BaseLv + DEX (monsters use simpler formula)
```

To achieve 95% hit rate: `AttackerHIT >= DefenderFLEE + 15`

---

## Full Damage Pipeline

The complete step-by-step order of operations for a physical attack in pre-Renewal Ragnarok Online:

```
=== PHASE 1: HIT DETERMINATION ===

Step 1: Perfect Dodge Check
   - Skip if: critical hit, skill attack, or forceHit
   - PD = 1 + floor(LUK / 10) + equipPD
   - if rnd(100) < PD -> MISS (Lucky Dodge), return 0

Step 2: Critical Hit Check
   - Skip if: skill attack (skills cannot crit in pre-re)
   - CRI = 1 + floor(LUK * 0.3) + equipCRI
   - If katar: CRI = CRI * 2
   - CritShield = floor(targetLUK / 5)
   - EffectiveCrit = CRI - CritShield
   - if rnd(100) < EffectiveCrit -> CRITICAL (set isCritical = true)

Step 3: HIT/FLEE Check
   - Skip if: critical hit, forceHit skill
   - HIT = 175 + BaseLv + DEX + bonusHIT
   - FLEE = 100 + targetBaseLv + targetAGI + targetBonusFLEE
   - If numAttackers > 2: FLEE = FLEE * (1 - (numAttackers-2)*0.1)
   - HitRate = clamp(80 + HIT - FLEE, 5, 95)
   - if rnd(100) >= HitRate -> MISS, return 0

=== PHASE 2: DAMAGE CALCULATION ===

Step 4: Calculate StatusATK
   - Melee:  StatusATK = floor(BaseLv/4) + STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
   - Ranged: StatusATK = floor(BaseLv/4) + DEX + floor(DEX/10)^2 + floor(STR/5) + floor(LUK/3)

Step 5: Calculate WeaponATK
   - Normal: WeaponATK = rnd(floor(min(DEX*(0.8+0.2*WLv), BaseATK)), BaseATK)
   - Critical: WeaponATK = BaseATK (always max, no variance)
   - Add refinement: WeaponATK += RefineLv * RefineBonus[WLv]

Step 6: Apply Size Penalty (to WeaponATK only)
   - SizedWeaponATK = floor(WeaponATK * SizePenalty% / 100)
   - StatusATK is NOT affected by size penalty

Step 7: Apply Weapon Variance
   - Skip if: critical hit
   - VarianceMul = rnd(1.0 - WLv*0.05, 1.0)
   - SizedWeaponATK = floor(SizedWeaponATK * VarianceMul)

Step 8: Calculate ExtraATK
   - ExtraATK = EquipATK + ConsumableATK + AmmoATK
   - (flat +ATK bonuses from cards, consumables, arrows)

Step 9: Sum Pre-Modifier Damage
   - BaseDamage = StatusATK + SizedWeaponATK + ExtraATK

Step 10: Apply Buff Multipliers
   - BuffMul = product of all active buff multipliers
   - Example: Provoke Lv10 = 1.32x ATK, Power Thrust Lv5 = 1.25x
   - BaseDamage = floor(BaseDamage * BuffMul)

Step 11: Apply Skill Multiplier
   - If skill: BaseDamage = floor(BaseDamage * SkillMultiplier% / 100)
   - Example: Bash Lv10 = 400%, Bowling Bash Lv10 = 500%

Step 12: Apply Card/Equipment Damage Bonuses
   - RaceBonus = sum of race% cards
   - EleBonus = sum of element% cards
   - SizeBonus = sum of size% cards
   - ClassBonus = sum of class% cards (boss/normal)
   - CardMul = (1 + RaceBonus/100) * (1 + EleBonus/100) * (1 + SizeBonus/100) * (1 + ClassBonus/100)
   - BaseDamage = floor(BaseDamage * CardMul)

Step 13: Apply Element Modifier
   - EleMod = ELEMENT_TABLE[atkElement][defElement][defElementLevel - 1]
   - If EleMod <= 0: return 0 (immune / absorbed)
   - BaseDamage = floor(BaseDamage * EleMod / 100)

=== PHASE 3: DEF REDUCTION ===

Step 14: Apply Hard DEF (percentage reduction)
   - Skip if: critical hit (crits bypass DEF in pre-re)
   - DamageAfterHardDEF = floor(BaseDamage * (4000 + HardDEF) / (4000 + HardDEF * 10))

Step 15: Apply Soft DEF (flat subtraction)
   - Skip if: critical hit (crits bypass soft DEF in pre-re)
   - SoftDEF = floor(VIT*0.5) + max(floor(VIT*0.3), floor(VIT^2/150) - 1)
   - DamageAfterDEF = DamageAfterHardDEF - SoftDEF

=== PHASE 4: POST-DEF ADDITIONS ===

Step 16: Add MasteryATK
   - MasteryATK = WeaponMastery + StarCrumbATK + SpiritSphereATK + OverupgradeRandom
   - DamageAfterDEF += MasteryATK

Step 17: Add BuffATK
   - BuffATK = ImpositioManus + other flat buff ATK
   - DamageAfterDEF += BuffATK

Step 18: Apply Critical Multiplier
   - If critical: DamageAfterDEF = floor(DamageAfterDEF * 1.4)

Step 19: Apply Defensive Card Reductions
   - DamageAfterDEF = floor(DamageAfterDEF * (1 - DefensiveRaceReduction))
   - DamageAfterDEF = floor(DamageAfterDEF * (1 - DefensiveEleReduction))
   - etc.

Step 20: Floor to Minimum
   - FinalDamage = max(1, DamageAfterDEF)

Step 21: Apply Dual Wield Penalty (if applicable)
   - RightHandDamage = floor(FinalDamage * RighthandMasteryPct / 100)
   - LeftHandDamage = floor(FinalDamage * LefthandMasteryPct / 100)

=== RETURN FinalDamage ===
```

### Important Notes on the Pipeline

1. **Every intermediate floor()**: The game floors every calculation result. No decimal values carry between steps.

2. **Element modifier before DEF**: Element is applied before DEF reduction. This means Fire vs Water (150%) amplified damage is then reduced by DEF, while Fire vs Fire (25%) reduced damage gets further reduced by DEF.

3. **Card bonuses before element**: Card percentage bonuses are applied before element modifier. This is significant because the card-amplified damage is then element-modified.

4. **MasteryATK after DEF**: Mastery bonuses bypass DEF entirely. This is why they are often called "seeking damage" or "true damage."

5. **Critical bypass**: In pre-Renewal, criticals skip Steps 14 and 15 entirely (no DEF at all), making them devastating against high-DEF targets.

6. **Provoke is a buff multiplier**: Provoke's ATK bonus (+2% + 3% per level, Lv10 = +32%) is applied in Step 10 as a multiplier to BaseDamage. The DEF reduction it causes is applied to the target's HardDEF in Step 14.

---

## Implementation Checklist

For implementing the pre-Renewal physical damage pipeline in a game server:

### Core Formula
- [x] StatusATK calculation (melee vs ranged variants)
- [x] WeaponATK with DEX-narrowed variance
- [x] Size penalty table (18 weapon types x 3 sizes)
- [x] Weapon level variance (+/- WLv * 5%)
- [x] Element modifier (10x10x4 table)
- [x] Hard DEF percentage reduction
- [x] Soft DEF flat subtraction
- [x] MasteryATK (post-DEF flat addition)
- [x] BuffATK (post-DEF flat addition)
- [x] Minimum damage floor (1)

### Hit Determination
- [x] Perfect Dodge check (LUK-based)
- [x] Critical hit check (LUK + equip + katar double)
- [x] Critical shield (target LUK / 5)
- [x] HIT/FLEE check with 5-95% clamp
- [x] Multi-monster FLEE penalty

### Modifiers
- [x] Card bonuses (race/element/size -- additive within category, multiplicative between)
- [x] Skill multipliers (per-skill % tables)
- [x] Buff multipliers (Provoke, Power Thrust, etc.)
- [x] Refinement ATK bonuses
- [x] Overupgrade random bonus

### Special Systems
- [x] Dual wield damage (Righthand/Lefthand Mastery)
- [x] Star Crumb seeking damage
- [x] Spirit Sphere ATK bonus
- [x] Critical hit DEF bypass (pre-Renewal specific)
- [x] Element priority (Endow > Arrow > Card > Forge > Default)
- [x] Defensive card reductions
- [x] Drake Card (size penalty nullification)
- [ ] Ice Pick / Investigate DEF inversion
- [ ] Maximize Power (always max weapon ATK)
- [ ] Lex Aeterna (double damage, consumed on first hit)
- [ ] Auto Guard / Parrying (chance to block physical)

### Edge Cases
- [ ] Element immunity -> 0 damage (not floored to 1)
- [ ] Negative element values (heals target)
- [ ] Multi-monster DEF penalty
- [ ] Pneuma (blocks all ranged physical)
- [ ] Safety Wall (blocks all melee physical)
- [ ] Reflecting (Mirror Image, Kaite)
- [ ] Endure (no flinch on hit, limited hits)

---

## Gap Analysis

Comparing the documented pre-Renewal formula against common server implementations, the following areas require special attention:

### 1. Hard DEF Formula Discrepancy
**Issue**: Many implementations use `(100 - DEF) / 100` instead of the canonical `(4000 + DEF) / (4000 + DEF * 10)`. The simplified formula gives a linear reduction while the canonical formula gives diminishing returns.
**Impact**: Significant. At 100 DEF, simplified = 0% remaining, canonical = ~81.5% remaining. The simplified formula makes high DEF far more powerful.
**Resolution**: Use the 4000-based formula for accurate pre-Renewal behavior.

### 2. Critical DEF Bypass
**Issue**: Some implementations have criticals NOT bypass DEF (following Renewal behavior). In pre-Renewal, criticals should bypass **both** Hard and Soft DEF.
**Impact**: Critical builds become significantly weaker if DEF is not bypassed.
**Resolution**: In pre-Renewal mode, skip DEF entirely for critical hits.

### 3. StatusATK Element Application
**Issue**: rAthena historically had StatusATK flagged as `NK_NO_ELEFIX` (not affected by element), but iRO Classic confirms StatusATK IS element-affected.
**Impact**: Moderate. Ghostring armor (Ghost element) should reduce StatusATK from Neutral weapons to 25% damage, not just WeaponATK.
**Resolution**: Apply element modifier to StatusATK as well.

### 4. Card Stacking Model
**Issue**: Some implementations add all card bonuses into a single pool. The correct behavior is additive within same category, multiplicative between different categories.
**Impact**: Significant. 4x Hydra = 1.80x, but 2 Hydra + 2 Skeleton Worker = 1.82x (multiplicative is better when mixing types).
**Resolution**: Separate card bonuses into race/element/size/class pools, add within each, then multiply the pools together.

### 5. Overupgrade Bonus Classification
**Issue**: The overupgrade random bonus should be classified as MasteryATK (bypasses DEF/element/size). Some implementations add it to WeaponATK where it gets reduced by DEF.
**Impact**: Moderate at high refine levels (+10 Lv4 weapon can add 0-84 random ATK).
**Resolution**: Add overupgrade bonus after DEF as part of MasteryATK.

### 6. BaseLv / 4 in StatusATK
**Issue**: Some implementations omit the `floor(BaseLv / 4)` term from StatusATK. iRO Wiki Classic includes it.
**Impact**: Low (24 ATK at level 99), but accumulates over many hits.
**Resolution**: Include BaseLv/4 in StatusATK calculation.

### 7. LUK Contribution Discrepancy
**Issue**: Some sources use `floor(LUK / 3)` for damage calculation and `floor(LUK / 5)` for status window display. Others use `floor(LUK / 5)` consistently.
**Impact**: Low (difference of ~6 ATK at 99 LUK).
**Resolution**: Use `floor(LUK / 3)` for actual damage, `floor(LUK / 5)` for display.

### 8. Soft DEF Random Component
**Issue**: Whether soft DEF has a random component varies by implementation. The formula `rnd(floor(VIT*0.3), max(...))` vs fixed `max(...)` changes damage consistency.
**Impact**: Low-moderate. Adds or removes defender-side damage variance.
**Resolution**: Use the fixed version for simpler implementation; document if using random version.

---

## Sources

- [ATK - iRO Wiki](https://irowiki.org/wiki/ATK)
- [Attacks - iRO Wiki Classic](https://irowiki.org/classic/Attacks)
- [Stats - iRO Wiki Classic](https://irowiki.org/classic/Stats)
- [DEF - iRO Wiki Classic](https://irowiki.org/classic/DEF)
- [DEF - iRO Wiki](https://irowiki.org/wiki/DEF)
- [Size - iRO Wiki Classic](https://irowiki.org/classic/Size)
- [Perfect Dodge - iRO Wiki Classic](https://irowiki.org/classic/Perfect_Dodge)
- [Dual Wielding - iRO Wiki](https://irowiki.org/wiki/Dual_Wielding)
- [Righthand Mastery - iRO Wiki Classic](https://irowiki.org/classic/Righthand_Mastery)
- [Left Hand Mastery - iRO Wiki](https://irowiki.org/wiki/Left_Hand_Mastery)
- [Refinement System - iRO Wiki Classic](https://irowiki.org/classic/Refinement_System)
- [Refinement System - iRO Wiki](https://irowiki.org/wiki/Refinement_System)
- [Forging - iRO Wiki Classic](https://irowiki.org/classic/Forging)
- [Weapon Mastery - iRO Wiki Classic](https://irowiki.org/classic/Weapon_Mastery)
- [Occult Impaction - iRO Wiki Classic](https://irowiki.org/classic/Occult_Impaction)
- [rAthena battle.cpp](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
- [rAthena Pre-Renewal Damage Rework Issue #8193](https://github.com/rathena/rathena/issues/8193)
- [rAthena Damage Pre-Renewal Forum Thread](https://rathena.org/board/topic/118071-damage-pre-renewal/)
- [RateMyServer Size Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer DEF Guide](https://forum.ratemyserver.net/guides/def-guide/)
- [WarpPortal Damage Calculation Mechanics](https://forums.warpportal.com/index.php?/topic/186118-r-inactive-damage-calculation-mechanics-wip/)
- [WarpPortal Critical Damage Modifiers](https://forums.warpportal.com/index.php?/topic/249572-solved-critical-damage-modifiers/)
- [ROClassicGuide Card Stacking](https://roclassic-guide.com/hydra-cards-elementals-substats-updates/)
- [TalonRO Card Stacking Wiki](https://wiki.talonro.com/Stacking_Of_Cards)
- [Card Modifier Guide - Ragnarok Job Guides](http://ragnarokjobguides.blogspot.com/2009/05/ragnarok-job-guides-weapon-card.html)
- [Weapon Size Modifiers - MuhRO](https://wiki.muhro.eu/Weapon_Size_Modifiers)
- [Size Penalty - NovaRO Wiki](https://www.novaragnarok.com/wiki/Size_Penalty)
- [Impositio Manus - iRO Wiki](https://irowiki.org/wiki/Impositio_Manus)
