# Magical Damage Formula -- Deep Research (Pre-Renewal)

> **Sources**: rAthena pre-renewal source (`battle.cpp`, `status.cpp`), Hercules pre-re (`battle.c`), iRO Wiki Classic, RateMyServer, divine-pride.net, WarpPortal Forums.
> **Scope**: Pre-Renewal (episode 10.x and earlier). All formulas verified against rAthena `#ifndef RENEWAL` code paths.
> **Last updated**: 2026-03-22

---

## Overview

Magical damage in pre-Renewal Ragnarok Online differs fundamentally from physical damage:

1. **No HIT/FLEE check** -- magic always hits (unless element immunity makes damage 0)
2. **No size penalty** -- magic damage is not affected by weapon size modifiers
3. **No critical hits** -- magic cannot crit in pre-Renewal
4. **Uses MDEF instead of DEF** -- Hard MDEF (percentage) + Soft MDEF (flat subtraction)
5. **MATK variance** is INT-based with min/max range, not DEX-narrowed like physical
6. **Spell element** is determined by the skill (not the weapon element)

The pipeline is significantly simpler than physical damage (9 steps vs 16+), but has its own nuances around card modifiers, Mystical Amplification, and multi-hit bundling.

---

## MATK Calculation (INT contribution, weapon MATK, refinement)

### Status MATK (INT-based)

Pre-Renewal uses two INT-based formulas for minimum and maximum MATK:

```
StatusMATK_Min = INT + floor(INT / 7)^2
StatusMATK_Max = INT + floor(INT / 5)^2
```

**Important**: `floor()` is applied to the division BEFORE squaring. For example, with 33 INT:
- Max: 33 + floor(33/5)^2 = 33 + 6^2 = 33 + 36 = **69**
- Min: 33 + floor(33/7)^2 = 33 + 4^2 = 33 + 16 = **49**

This creates a variance spread of 49-69 MATK from stats alone.

#### INT Breakpoints (Status MATK Min/Max)

| INT | MATK Min | MATK Max | Variance |
|-----|----------|----------|----------|
| 1   | 1        | 1        | 0        |
| 10  | 11       | 14       | 3        |
| 20  | 24       | 36       | 12       |
| 30  | 46       | 66       | 20       |
| 40  | 65       | 104      | 39       |
| 50  | 99       | 150      | 51       |
| 60  | 124      | 204      | 80       |
| 70  | 170      | 266      | 96       |
| 80  | 209      | 336      | 127      |
| 90  | 256      | 414      | 158      |
| 99  | 298      | 490      | 192      |

> **rAthena source** (`status.cpp`): `status_base_matk_min()` and `status_base_matk_max()` contain alternative formulas for different entity types. For `BL_PC` (player characters) in pre-renewal, some rAthena builds use the simpler `INT + floor(INT/7)^2` / `INT + floor(INT/5)^2` formulas. Some rAthena forum threads report an alternate formula: `INT + floor(INT/2) + floor(DEX/5) + floor(LUK/3) + floor(BaseLv/4)` -- but this is the **Renewal** formula (`RENEWAL_CAST` defined). The canonical pre-renewal MATK is the `INT/5^2` and `INT/7^2` formulas above.

### Weapon MATK

Staves and rods contribute a flat MATK value from the weapon's database entry, plus refinement bonuses:

```
TotalWeaponMATK = BaseWeaponMATK + RefinementMATK
```

**Weapon MATK Percentage Bonus**: Some weapons (particularly rods/staves) have a `bMatkRate` bonus that applies as a percentage modifier to total MATK. For example:
- Rod: MATK +15%
- Staff: MATK +15%, INT +2
- Hypnotist's Staff: MATK +25%, INT +1
- Survivor's Rod (INT): MATK +15%, INT +3, DEX +3
- Wizardry Staff: MATK +15%, INT +6, DEX +2

These `bMatkRate` percentages are applied multiplicatively to the total MATK (status + weapon) after the base MATK is calculated.

### Refinement Bonus to Weapon MATK

Weapon MATK refinement follows the same table as physical ATK refinement:

| Weapon Level | MATK per Refine | Over-Upgrade Bonus (above safe) |
|-------------|----------------|-------------------------------|
| Level 1     | +2 per refine  | Random 0-3 per hit           |
| Level 2     | +3 per refine  | Random 0-5 per hit           |
| Level 3     | +5 per refine  | Random 0-8 per hit           |
| Level 4     | +7 per refine  | Random 0-14 per hit          |

**Safe refinement limits**: Lv1 = +7, Lv2 = +6, Lv3 = +5, Lv4 = +4. Beyond these limits, over-upgrade bonus applies as a random addition per hit (not a fixed value).

---

## MATK Min/Max and Variance

### Final MATK Range (Status + Weapon)

```
MATK_Max = StatusMATK_Max + WeaponMATK
MATK_Min = StatusMATK_Min + floor(WeaponMATK * 0.7)

ActualMATK = random(MATK_Min, MATK_Max)
```

The weapon MATK minimum is 70% of the base weapon MATK value, creating additional variance from equipment.

### Status Window Display

The status window shows MATK as `A ~ B` where:
- `A` = MATK_Min (with equipment + cards applied)
- `B` = MATK_Max (with equipment + cards applied)

If `bMatkRate` is present (e.g., Rod's +15%):
```
DisplayMATK_Min = floor(MATK_Min * (100 + bMatkRate) / 100)
DisplayMATK_Max = floor(MATK_Max * (100 + bMatkRate) / 100)
```

### Per-Hit Variance

Each magical attack rolls a new random value between MATK_Min and MATK_Max. For multi-hit skills (bolts, Storm Gust), **each hit rolls independently**, creating natural damage variance across hits.

---

## MDEF Reduction (Hard MDEF %, Soft MDEF subtraction)

### Hard MDEF (Equipment MDEF) -- Percentage Reduction

Hard MDEF is a percentage-based reduction applied first. Two formulas exist:

**rAthena pre-renewal (simplified, used in most private servers)**:
```
DamageAfterHardMDEF = floor(Damage * (100 - HardMDEF) / 100)
```
Hard MDEF is capped at 99 (99% max reduction).

**iRO Wiki formula (official servers)**:
```
DamageAfterHardMDEF = floor(Damage * (1000 + HardMDEF) / (1000 + HardMDEF * 10))
```

| HardMDEF | rAthena Reduction | iRO Reduction |
|----------|-------------------|---------------|
| 0        | 0%                | 0%            |
| 10       | 10%               | 8.2%          |
| 30       | 30%               | 22.6%         |
| 60       | 60%               | 33.3%         |
| 90       | 90%               | 44.7%         |
| 99       | 99%               | 47.1%         |
| 125      | 99% (capped)      | 50.0%         |

> **Implementation note**: rAthena's simplified formula provides much more aggressive scaling. Most pre-renewal private servers use the rAthena formula. The iRO formula results in diminishing returns -- even 125 MDEF only halves magic damage.

**Which formula to use**: The rAthena simplified formula (`100 - MDEF%`) is recommended for pre-renewal implementations because:
1. It matches the majority of private server behavior
2. It is simpler to implement and debug
3. The iRO formula was likely a Gravity proprietary variation not widely replicated

### Soft MDEF (INT-based) -- Flat Subtraction

Soft MDEF is a flat number subtracted from damage after Hard MDEF is applied:

```
SoftMDEF = INT + floor(VIT / 5) + floor(DEX / 5) + floor(BaseLv / 4)
```

This is then modified by any additive (`BonusA`) and multiplicative (`BonusB`) bonuses:
```
TotalSoftMDEF = floor((SoftMDEF + BonusA) * (1 + BonusB / 100))
```

| INT | VIT=30 DEX=30 Lv=99 | Approximate SoftMDEF |
|-----|---------------------|---------------------|
| 1   | 30/30/99            | 1 + 6 + 6 + 24 = 37  |
| 50  | 30/30/99            | 50 + 6 + 6 + 24 = 86 |
| 99  | 30/30/99            | 99 + 6 + 6 + 24 = 135|

### MDEF Application Order

```
1. Start with raw spell damage (after skill multiplier)
2. Apply Hard MDEF (percentage reduction)
3. Apply Soft MDEF (flat subtraction)
4. Floor to minimum 1
```

### MDEF Bypass Mechanics

| Mechanic | Effect |
|----------|--------|
| Golden Thief Bug Card | 100% magic immunity (`bNoMagicDamage:100`) |
| Vesper Card | Ignore 30% of boss monster MDEF |
| High Wizard Card | Ignore 10% of normal monster MDEF |
| Strip Helm (Rogue) | Reduces INT, lowering Soft MDEF |
| Steel Body (Monk) | Sets Hard MDEF to 90 (overrides equipment) |
| Freeze/Stone status | MDEF +25% bonus during frozen/petrified state |

---

## Element Modifier (Spell Element vs Target Element)

### How Spell Element is Determined

Unlike physical attacks (where element comes from weapon/endow), magical spell element is determined by the **skill itself**:

| Spell | Element |
|-------|---------|
| Fire Bolt, Fire Ball, Fire Wall, Meteor Storm | Fire |
| Cold Bolt, Frost Diver, Storm Gust, Frost Nova | Water |
| Lightning Bolt, Jupitel Thunder, Lord of Vermilion | Wind |
| Earth Spike, Heaven's Drive | Earth |
| Soul Strike, Napalm Beat | Ghost |
| Holy Light, Magnus Exorcismus, Sanctuary (vs Undead) | Holy |
| Thunderstorm | Wind |
| Stone Curse (damage tick) | Earth |
| Grand Cross | Holy |

### Element Modifier Table (Partial -- Key Magic Interactions)

Values are **damage percentages** (100 = normal, 0 = immune, negative = heals):

#### Level 1 Defenders

| Spell Element | vs Neutral | vs Water | vs Fire | vs Wind | vs Earth | vs Undead | vs Shadow | vs Ghost |
|---------------|-----------|---------|---------|---------|---------|-----------|-----------|----------|
| Fire          | 100       | 50      | 25      | 100     | 150     | 125       | 100       | 100      |
| Water         | 100       | 25      | 150     | 50      | 100     | 100       | 100       | 100      |
| Wind          | 100       | 175     | 100     | 25      | 50      | 100       | 100       | 100      |
| Earth         | 100       | 100     | 50      | 150     | 100     | 100       | 100       | 100      |
| Ghost         | 25        | 100     | 100     | 100     | 100     | 100       | 75        | 125      |
| Holy          | 100       | 100     | 100     | 100     | 100     | 150       | 125       | 100      |

#### Higher Element Levels (Key Differences)

At Level 4 defenders, the same-element immunity reaches -50% (healing the target), opposite elements deal 200%, and Ghost element becomes immune to Neutral attacks (0% at Level 2+).

### Application in Pipeline

```
DamageAfterElement = floor(Damage * ElementModifier / 100)
```

If `ElementModifier <= 0`, the spell deals 0 damage (or heals the target if negative). Element is checked **before** MDEF reduction in the pre-renewal pipeline.

---

## Skill-Specific MATK Multipliers

Each magical skill has a percentage-based damage multiplier applied to the rolled MATK value.

### Bolt Skills (Multi-Hit)

| Skill | Multiplier per Hit | Hits = Skill Level | Total at Lv10 |
|-------|-------------------|-------------------|---------------|
| Fire Bolt | 100% | 10 | 1000% |
| Cold Bolt | 100% | 10 | 1000% |
| Lightning Bolt | 100% | 10 | 1000% |

Each hit rolls its own MATK variance independently. Bolt skills are a "damage bundle" -- Lex Aeterna doubles the entire bundle.

### Single/Multi-Target Skills

| Skill | Multiplier Formula | At Lv10 |
|-------|-------------------|---------|
| Soul Strike | 100% per level (multi-hit, 1+lv/2 hits) | 600% (6 hits) |
| Napalm Beat | 70% + 10% per level (split among targets) | 170% total |
| Fire Ball | 170% + 10% per level | 270% |
| Fire Wall | 50% per hit (max 14 hits) | 700% max |
| Thunderstorm | 100% per level (8 hits AoE) | 800% total |
| Jupitel Thunder | 100% per level + knockback | 1000% (7 hits at Lv10) |
| Lord of Vermilion | 100% per level (AoE, 4s duration) | 1000% (multiple ticks) |
| Storm Gust | 100% per level (AoE, 4.6s) | 1000% (multiple ticks) |
| Meteor Storm | 125% per level (AoE meteors) | 1250% per meteor |
| Earth Spike | 100% per level (multi-hit) | 500% (5 hits at Lv5) |
| Heaven's Drive | 125% MATK (flat, AoE) | 125% |
| Frost Nova | 100-200% (by level, AoE) | 200% at Lv10 |
| Fire Pillar | (auto-hit on walk) | Varies by level |
| Sight Blaster | 100% MATK (reactive) | 100% |

### Priest/Acolyte Offensive Magic

| Skill | Multiplier | Notes |
|-------|-----------|-------|
| Holy Light | 125% MATK | Holy element |
| Magnus Exorcismus | 100% per level per wave | Holy element, multi-wave |
| Turn Undead | Instant kill check, else damage | Based on (BaseLv+INT+LUK)/100 |

### Sage Skills

| Skill | Multiplier | Notes |
|-------|-----------|-------|
| Earth Spike (Sage) | 100% per level | Same as Wizard |
| Heaven's Drive (Sage) | 125% MATK | Same formula |
| Hindsight auto-cast | Uses original skill multiplier | Random bolt at Lv1-3 |

---

## Cast Time Formula (Base Cast, DEX Reduction, Suffragium, etc.)

### Pre-Renewal Cast Time

In pre-Renewal, cast time is **entirely variable** (no fixed cast component):

```
CastTime = BaseCastTime * (1 - DEX / 150) * (1 - SuffragiumReduction) * (1 - EquipReduction / 100)
```

Where:
- `BaseCastTime` = skill-specific base cast time in milliseconds
- `DEX / 150` = DEX reduction factor (0.667% per DEX point)
- `SuffragiumReduction` = 0.15 per Suffragium level (15/30/45%)
- `EquipReduction` = sum of all equipment/card cast time reduction percentages

### Instant Cast

Instant cast occurs when `DEX >= 150` (before any modifiers). With Suffragium or equipment bonuses, the instant cast DEX threshold is lower:

```
Required DEX = 150 * (1 - SuffragiumLv * 0.15) * (1 - EquipReduction / 100)
```

| Suffragium Lv | Required DEX (no equip) |
|---------------|------------------------|
| 0             | 150                    |
| 1             | 127.5 (128)            |
| 2             | 105                    |
| 3             | 82.5 (83)              |

### Cast Time Modifiers

| Source | Reduction | Notes |
|--------|----------|-------|
| DEX stat | 0.667% per point | Linear reduction |
| Suffragium (Priest) | 15% per level (1-3) | Multiplicative with DEX |
| Poem of Bragi (Bard) | Formula-based | Affects both cast time AND after-cast delay |
| Memorize (Sage) | Halves next 5 casts | 50% reduction, consumed per cast |
| Phen Card | +25% cast time | Trade-off for uninterruptible casting |
| Drops Card (weapon) | Varies | Cast time reduction |

### Bragi Cast Time Reduction Formula

Poem of Bragi (Magic Strings) cast time reduction in pre-renewal:
```
CastReduction% = (SkillLevel * 3) + (floor(PerformerINT / 5)) + (MusicLessonsLv * 2) + (floor(SkillLevel / 10) * 20)
```

This applies multiplicatively with the DEX reduction.

---

## Cast Interruption (VIT-based Chance, Phen Card, Endure)

### Interruption Mechanics

When a character takes damage during casting, there is a chance the cast will be interrupted (canceled). In pre-Renewal:

```
InterruptionChance% = 100 - VIT   (clamped to 0-100)
```

This means a character with 100 VIT has 0% chance of being interrupted by damage (effectively immune).

### Anti-Interruption Sources

| Source | Effect | Side Effect |
|--------|--------|-------------|
| Phen Card (accessory) | Casting cannot be interrupted | +25% cast time |
| Bloody Butterfly Card | Casting cannot be interrupted | +25% cast time |
| Endure (Swordsman) | Prevents flinching | Does NOT prevent cast interruption for magic |
| Devotion (Crusader) | Damage redirected to Crusader | Caster is not interrupted since they take no damage |
| Assumptio (Priest) | N/A | Does not prevent interruption, only halves damage |

**Important distinctions**:
- Phen Card grants `bUnbreakableCast` -- 100% uninterruptible regardless of VIT
- VIT only provides a **chance** to resist interruption; it is not a guarantee below 100 VIT
- Skills with `INF2_NOPHEN` flag (e.g., Warp Portal) cannot benefit from Phen Card
- Cast interruption only occurs from **damage**, not from status effects like stun/freeze (those cancel the cast outright)

### Server-Side Implementation

Cast interruption is checked whenever a casting player takes damage:
```javascript
// On damage to a casting player:
if (activeCasts.has(targetId)) {
    const hasUnbreakable = target.unbreakableCast || false;
    if (!hasUnbreakable) {
        const vitResist = Math.min(100, target.effectiveVit || 0);
        const interruptChance = 100 - vitResist;
        if (Math.random() * 100 < interruptChance) {
            activeCasts.delete(targetId);
            // Broadcast cast interruption
        }
    }
}
```

---

## After Cast Delay (Global Cooldown, Bragi Reduction)

### ACD Overview

After Cast Delay (ACD) is a skill-specific cooldown that prevents ANY skill from being used during the delay period. It is separate from cast time and begins when the skill finishes casting.

```
EffectiveACD = BaseACD * (1 - BragiReduction / 100) * (1 - EquipReduction / 100)
```

### Bragi ACD Reduction Formula (Pre-Renewal)

```
ACDReduction% = (SkillLevel * 3) + (floor(PerformerINT / 5)) + (MusicLessonsLv * 2)
```

Note: The `floor(SkillLevel / 10) * 20` bonus that applies to cast time does NOT apply to ACD in most pre-renewal implementations.

### Equipment ACD Reduction

| Source | Reduction | Notes |
|--------|----------|-------|
| Expert Ring | -5% ACD | Accessory |
| Kiel-D-01 Card | -30% ACD | Headgear card |
| Poem of Bragi | Formula-based | Bard performance effect |
| Pharaoh Card | N/A | Reduces SP cost, not ACD |

### ACD Stacking

ACD reductions stack **additively** in pre-renewal:
```
TotalReduction = BragiReduction + ExpertRingReduction + KielReduction + ...
EffectiveACD = BaseACD * (100 - TotalReduction) / 100
```

Minimum ACD is typically clamped to 0 (no negative ACD).

### Typical Skill ACD Values

| Skill | Base ACD (ms) |
|-------|---------------|
| Fire Bolt / Cold Bolt / Lightning Bolt | 200 |
| Storm Gust | 5000 |
| Lord of Vermilion | 5000 |
| Meteor Storm | 2000 |
| Jupitel Thunder | 0 |
| Soul Strike | 1000 |
| Napalm Beat | 500 |
| Fire Ball | 1500 |
| Earth Spike | 0 |
| Heaven's Drive | 1000 |

---

## Magic Rod / Spell Absorption Mechanics

### Magic Rod (Sage Skill)

Magic Rod absorbs incoming **single-target** magic spells, converting the spell's SP cost into SP gained by the caster.

**Mechanics**:
```
SP Gained = floor(SpellSPCost * AbsorptionRate / 100)
```

| Magic Rod Level | Duration (ms) | SP Absorption Rate |
|----------------|---------------|-------------------|
| 1              | 200           | 20%               |
| 2              | 400           | 40%               |
| 3              | 600           | 60%               |
| 4              | 800           | 80%               |
| 5              | 1000          | 100%              |

**Restrictions**:
- Only absorbs **single-target** magic (not AoE spells like Storm Gust, Lord of Vermilion, etc.)
- The timing window is very short -- must be active when the spell hits
- Successfully absorbing a spell nullifies all damage from that spell
- Multiple spells CAN be absorbed during the duration (unlike physical counter skills)
- Does NOT work against ground-targeted AoE spells

### Spell Breaker (Sage Skill)

Separate from Magic Rod, Spell Breaker interrupts an enemy's cast and deals damage + drains SP:
- Cancels the target's current cast
- Deals MATK-based damage
- Drains target SP

### Golden Thief Bug Card

The ultimate magic defense -- `bNoMagicDamage:100` provides **complete magic immunity**:
- All incoming magic damage is reduced to 0
- Check is performed at the very beginning of the magic damage pipeline (before any calculation)
- Trade-off: equipped in Shield slot, losing potential DEF/reduction cards

---

## Lex Aeterna Interaction

### Basic Mechanic

Lex Aeterna (Priest skill) causes the target to take **2x damage** from the next incoming damage instance.

```
DamageWithLA = Damage * 2
```

### Multi-Hit Skill Interaction (Critical for Implementation)

The behavior depends on whether the skill's hits are **bundled** or **individual**:

**Bundled hits** (total displayed as one number) -- LA doubles the ENTIRE damage:
- Cold Bolt (all hits bundled into one yellow number)
- Fire Bolt (all hits bundled)
- Lightning Bolt (all hits bundled)
- Sonic Blow (physical, but same concept)
- Earth Spike (all hits bundled)

**Individual hits** (each hit displays separately) -- LA doubles ONLY the first hit:
- Storm Gust (each tick is separate)
- Lord of Vermilion (each tick is separate)
- Water Ball (each hit is separate)
- Fire Wall (each hit is separate)
- Meteor Storm (each meteor is separate)

### What Does NOT Consume LA

- Healing (Heal does not count as damage)
- Miss / element immunity (0 damage does not consume)
- Status effects without damage (Quagmire, Decrease AGI debuff)
- Shield Reflect retaliation damage

### What Removes LA Without Applying

- Guard (Crusader) -- blocks the attack entirely
- Parry (Lord Knight) -- blocks the attack
- Kaupe (Soul Linker) -- dodge

### Pipeline Position

Lex Aeterna is applied as the **last multiplicative step** in the damage pipeline, after all other modifiers including MDEF:
```
FinalDamage = DamageAfterAllReductions * 2   (if LA active)
```

---

## Safety Wall Blocking

### What Safety Wall Blocks

Safety Wall blocks **melee/close-range physical attacks only**:
- Normal melee auto-attacks
- Close-range physical skills (Bash, Pierce, etc.)
- Monster melee attacks
- Skills defined as `BF_SHORT` (short-range)

### What Safety Wall Does NOT Block

- **All magic damage** -- magic passes through Safety Wall completely
- Ranged physical attacks (arrows, thrown weapons)
- Ranged skills (Double Strafe, Arrow Shower)
- Ground-targeted AoE damage
- Status effects (stun, freeze, etc.)

### Safety Wall Properties

| Level | Duration (s) | Hit Count |
|-------|-------------|-----------|
| 1     | 5           | 1         |
| 5     | 15          | 5         |
| 10    | 50          | 11        |

Each melee hit absorbed decrements the hit counter. When hits reach 0 or duration expires, the wall disappears.

### Interaction with Magic

**Safety Wall has ZERO interaction with magic damage**. A Wizard standing on Safety Wall receives full magical damage from all sources. Safety Wall is purely a melee physical defense.

---

## Pneuma vs Magic

### What Pneuma Blocks

Pneuma creates a 3x3 cell area that blocks **ranged physical attacks**:
- Arrows (auto-attacks from Bow users)
- Ranged physical skills (Double Strafe, Arrow Shower, Blitz Beat)
- Monster ranged attacks (if attack range >= 4 cells)

### What Pneuma Does NOT Block

- **All magic damage** -- Pneuma has no effect on magic whatsoever
- Melee attacks (those are blocked by Safety Wall instead)
- Ground-targeted AoE damage
- Status effects
- Trap damage

### Key Rule

```
Pneuma  = blocks RANGED PHYSICAL only
Safety Wall = blocks MELEE PHYSICAL only
Neither     = blocks MAGIC
```

Magic damage has no positional defense skill in pre-Renewal. The only defenses against magic are:
1. MDEF (equipment + stats)
2. Element resistance (armor element, resist cards)
3. Golden Thief Bug Card (full magic immunity)
4. Magic Rod (Sage -- absorb single-target spells)
5. Land Protector (Sage -- nullifies ground-targeted magic on the cells)

---

## Magic Damage Cards & Equipment

### Card Bonuses That Affect Magic Damage

Cards that modify magic damage fall into several categories:

#### Attacker-Side Cards (Weapon)

**bMatkRate** -- Flat % increase to total MATK:
```
FinalMATK = floor(BaseMATK * (100 + bMatkRate) / 100)
```

**bMagicAddRace** -- % increase to magic damage vs specific race:
- Example: +10% magic damage vs Demi-Human

**bMagicAddEle** -- % increase to magic damage vs specific element:
- Example: +7% magic damage vs Fire element monsters

**bMagicAddSize** -- % increase to magic damage vs specific size:
- Example: +15% magic damage vs Medium size

**bSkillAtk** -- % increase to specific skill damage:
- Example: Fireblend Card: Fire Bolt damage +10%

#### Attacker-Side Cards (Armor/Accessory)

**bMagicAtkEle** -- Increase magic damage of a specific element:
- Example: +20% Fire element magic damage

**bMagicRaceDamage** -- Increase magic damage vs a specific race (armor slot):
- Example: +50% magic damage vs Undead race

#### Defender-Side Cards

**bNoMagicDamage** -- Reduce/nullify incoming magic:
- Golden Thief Bug Card: 100% (full immunity)

**bMagicDefRace** -- Reduce magic damage from specific race:
- Example: -10% magic damage from Demi-Human

**bSubEle** -- Reduce damage from specific element:
- Example: Swordfish Card (armor): -30% Water damage taken

**bIgnoreMdefClassRate** -- Ignore % of target's MDEF:
- Vesper Card: Ignore 30% Boss MDEF
- High Wizard Card: Ignore 10% Normal Monster MDEF

### Card Stacking in Magic Pipeline

Within each category, bonuses are **additive**:
```
4x Hydra Card (bAddRace Demi-Human): 1.00 + 0.20 + 0.20 + 0.20 + 0.20 = 1.80 (80% bonus)
```

Between categories, application is **consecutive with flooring**:
```
damage = floor(damage * (100 + raceBonus) / 100)
damage = floor(damage * (100 + sizeBonus) / 100)
damage = floor(damage * (100 + eleBonus) / 100)
```

The order in rAthena's `battle_calc_cardfix()` for magic (BF_MAGIC):
1. Size bonus
2. Defense element bonus
3. Race bonus
4. Class bonus (normal/boss)

---

## Amp (Mystical Amplification) Modifier

### Overview

Mystical Amplification (Amp) is a High Wizard skill (Skill ID 366) that amplifies the next magical skill's damage.

### Formula

```
AmpBonus = 50 + (SkillLevel * 50)   // percentage
AmplifiedDamage = floor(Damage * (100 + AmpBonus) / 100)
```

| Amp Level | Damage Multiplier |
|-----------|------------------|
| 1         | 100% + 100% = 200% (2.0x) |
| 2         | 100% + 150% = 250% (2.5x) |
| 3         | 100% + 200% = 300% (3.0x) |
| 4         | 100% + 250% = 350% (3.5x) |
| 5         | 100% + 300% = 400% (4.0x) |

Wait -- the above is the **rAthena/Hercules** implementation. Cross-referencing with iRO Wiki Classic and RateMyServer:

**Corrected Pre-Renewal formula**:
```
AmpBonus% = SkillLevel * 50
AmplifiedMATK = floor(MATK * (100 + AmpBonus) / 100)
```

| Amp Level | Total Multiplier |
|-----------|-----------------|
| 1         | 150% (1.5x)    |
| 2         | 200% (2.0x)    |
| 3         | 250% (2.5x)    |
| 4         | 300% (3.0x)    |
| 5         | 350% (3.5x)    |

> **Note**: Some sources show `5% * level` for a much weaker amp (105-125%), but this appears to be Renewal-specific or a misreading. The pre-renewal consensus from iRO Wiki Classic and rAthena is `50% * level`.

### Behavior

- **Duration**: 30 seconds or until the next skill cast
- **Consumption**: Consumed when the next offensive spell **begins casting** (not when it hits)
- **Applies to**: All active magical damage from the amplified skill (including multi-hit, AoE ticks)
- **Does NOT stack** with itself -- re-casting Amp refreshes the timer
- **Pre-renewal special**: Amp also affects non-attack skills like Heal and Energy Coat in some implementations

### Pipeline Position

Amp is applied as a multiplier to the base MATK **before** skill multiplier in some implementations, or **after** skill multiplier in others. rAthena applies it to the MATK value directly:

```
// rAthena pre-renewal:
matk = matk * (100 + ampBonus) / 100   // Applied to base MATK
damage = matk * skillRatio / 100       // Then skill multiplier
```

This means Amp multiplies the entire skill's damage proportionally.

---

## Complete Magic Damage Pipeline (Step-by-Step)

This is the authoritative step-by-step pipeline for pre-Renewal magical damage calculation, synthesized from rAthena source, iRO Wiki Classic, and verified against the existing server implementation.

```
Step  1: GTB Check
         if target.cardNoMagicDamage >= 100 → damage = 0, return IMMUNE

Step  2: Calculate MATK Range
         StatusMATK_Min = INT + floor(INT/7)^2
         StatusMATK_Max = INT + floor(INT/5)^2
         MATK_Min = StatusMATK_Min + floor(WeaponMATK * 0.7)
         MATK_Max = StatusMATK_Max + WeaponMATK

Step  3: Roll MATK
         MATK = random(MATK_Min, MATK_Max)

Step  4: Apply Weapon MATK Rate (bMatkRate)
         if bMatkRate != 0:
             MATK = floor(MATK * (100 + bMatkRate) / 100)

Step  5: Apply Mystical Amplification
         if AmpActive:
             MATK = floor(MATK * (100 + AmpBonus) / 100)
             ConsumeAmpBuff()

Step  6: Apply Skill Multiplier
         damage = floor(MATK * SkillMultiplier / 100)

Step  7: Apply Card MATK Bonuses (consecutive with flooring)
         a. bMagicAddSize:  damage = floor(damage * (100 + sizeBonus) / 100)
         b. bMagicAddEle:   damage = floor(damage * (100 + eleDefBonus) / 100)
         c. bMagicAddRace:  damage = floor(damage * (100 + raceBonus) / 100)
         d. bMagicAtkEle:   damage = floor(damage * (100 + atkEleBonus) / 100)
         e. bSkillAtk:      damage = floor(damage * (100 + skillSpecificBonus) / 100)

Step  8: Apply Element Modifier
         eleModifier = ELEMENT_TABLE[skillElement][targetElement][targetElementLevel]
         if eleModifier <= 0 → damage = 0, return (IMMUNE or HEAL)
         damage = floor(damage * eleModifier / 100)

Step  9: Apply Hard MDEF (Percentage Reduction)
         effectiveHardMDEF = min(99, equipmentMDEF)
         // Apply MDEF ignore cards (Vesper, HW Card)
         if ignorePercent > 0:
             effectiveHardMDEF = floor(effectiveHardMDEF * (100 - ignorePercent) / 100)
         // Steel Body override
         if overrideHardMDEF != null:
             effectiveHardMDEF = overrideHardMDEF
         damage = floor(damage * (100 - effectiveHardMDEF) / 100)

Step 10: Apply Soft MDEF (Flat Subtraction)
         softMDEF = INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)
         // Apply Strip Helm INT reduction
         effectiveSoftMDEF = floor(softMDEF * intMultiplier)
         damage = damage - effectiveSoftMDEF

Step 11: Apply Buff MDEF Bonus (flat subtraction)
         damage = damage - buffBonusMDEF

Step 12: Apply Status MDEF Modifier
         // Freeze/Stone: MDEF multiplier = 1.25 (target takes 125%)
         if mdefMultiplier != 1.0:
             damage = floor(damage * mdefMultiplier)

Step 13: Apply Sage Zone Damage Boost
         // Volcano: +10-50% fire damage
         // Deluge: +10-50% water damage
         // Violent Gale: +10-50% wind damage
         if matchingZoneBoost:
             damage = floor(damage * (100 + zoneBoost) / 100)

Step 14: Apply Passive Race Modifiers
         // Dragonology: +2% MATK per level vs Dragon race
         if passiveRaceMATK:
             damage = floor(damage * (100 + raceBonus) / 100)
         // Defender passive race resist
         if passiveRaceResist:
             damage = floor(damage * (100 - resistBonus) / 100)

Step 15: Apply Raid Debuff
         // Target under Raid: +20% incoming damage (bosses +10%)
         if raidDamageIncrease:
             damage = floor(damage * (1 + raidDamageIncrease))

Step 16: Apply Lex Aeterna
         if target has lex_aeterna buff:
             damage = damage * 2
             RemoveBuff(lex_aeterna)
         // Note: Only for bundled-hit skills. For per-tick skills,
         // LA is checked and consumed on the first tick only.

Step 17: Floor to Minimum
         FinalDamage = max(1, damage)
```

### Pre-Pipeline Checks (Before Step 1)

Before entering the damage pipeline, the following checks occur in the skill handler:

1. **Magic Rod absorption** -- if target has `magic_rod` buff active, absorb the spell (SP gain, 0 damage)
2. **Land Protector** -- if target stands on Land Protector, ground-targeted magic deals 0 damage
3. **Reflect Shield** (Magic) -- some implementations reflect a % of magic damage back

### Multi-Hit Processing

For multi-hit skills, the pipeline is run **per hit**:
- **Bolt skills**: Each hit runs Steps 2-17 independently, but Lex Aeterna applies to the total bundle
- **Storm Gust / LoV**: Each tick runs the full pipeline; Lex Aeterna consumed on first tick
- **Meteor Storm**: Each meteor runs independently; Lex Aeterna consumed on first meteor hit

---

## Implementation Checklist

### Currently Implemented (in `ro_damage_formulas.js`)

- [x] StatusMATK Min/Max calculation (`INT + floor(INT/7)^2` / `INT + floor(INT/5)^2`)
- [x] Weapon MATK integration (0.7x for min)
- [x] MATK variance roll (`random(min, max)`)
- [x] Skill multiplier application
- [x] Card MATK rate (`bMatkRate`)
- [x] Card magic race bonus (`bMagicAddRace`)
- [x] Card skill-specific bonus (`bSkillAtk`)
- [x] Element modifier lookup and application
- [x] Hard MDEF percentage reduction (rAthena formula, capped at 99)
- [x] Soft MDEF flat subtraction
- [x] MDEF ignore by class (Vesper/HW Card)
- [x] Steel Body MDEF override
- [x] Strip Helm INT multiplier on Soft MDEF
- [x] Buff MDEF bonus subtraction
- [x] Status MDEF multiplier (freeze/stone 1.25x)
- [x] Sage zone damage boosts (Volcano/Deluge/Violent Gale)
- [x] Dragonology race MATK bonus
- [x] Passive race resist
- [x] Raid debuff damage increase
- [x] Golden Thief Bug full magic immunity
- [x] Element immunity/healing handling (negative eleModifier)
- [x] Floor to minimum 1

### Currently Implemented (in `index.js` skill handlers)

- [x] Magic Rod absorption (`checkMagicRodAbsorption()`)
- [x] Lex Aeterna 2x damage + consumption
- [x] Per-hit variance for multi-hit skills
- [x] Cast time calculation with DEX reduction
- [x] Cast interruption on damage
- [x] After-cast delay per skill
- [x] Bragi ACD reduction

### Missing / Needs Verification

- [ ] **Mystical Amplification (Amp)**: Verify Amp is applied to MATK before skill multiplier (Step 5). Check if High Wizard class is implemented with Amp skill handler.
- [ ] **bMatkRate application order**: Currently applied after skill multiplier. rAthena applies it to base MATK before skill ratio. Verify correct order.
- [ ] **Card fix order (magic)**: rAthena applies size -> defEle -> race -> class consecutively. Current implementation only has race. Need to add size and element card bonuses for magic.
- [ ] **bMagicAddSize**: Not currently in `calculateMagicalDamage()`. Need to add magic size card bonus.
- [ ] **bMagicAtkEle**: Not currently in pipeline. Cards that boost specific element magic damage.
- [ ] **bMagicDefEle**: Defender-side element magic resistance cards.
- [ ] **Phen Card cast time penalty**: Verify +25% cast time when Phen is equipped with unbreakable cast.
- [ ] **Land Protector blocking**: Verify ground-targeted magic is nullified on LP cells.
- [ ] **Over-upgrade MATK random bonus**: Verify refine over-upgrade adds random MATK per hit (not flat).
- [ ] **Lex Aeterna bundle vs tick distinction**: Verify bolt skills double entire bundle while ground AoE only doubles first tick.
- [ ] **Suffragium cast reduction**: Verify Suffragium multiplicatively stacks with DEX.
- [ ] **Memorize cast reduction**: Sage skill that halves cast time for next 5 casts.

---

## Gap Analysis

### Comparing Server Implementation vs Canonical rAthena Pre-Renewal

| Feature | rAthena Reference | Our Implementation | Status |
|---------|------------------|-------------------|--------|
| MATK Min/Max | `INT+floor(INT/7)^2` / `INT+floor(INT/5)^2` | Same formula | MATCH |
| WeaponMATK 0.7x min | `floor(weaponMATK*0.7)` for min | Same | MATCH |
| bMatkRate | Applied to MATK before skill ratio | Applied after skill multiplier | MISMATCH -- verify order |
| Amp (Mystical Amplification) | `matk *= (100+ampBonus)/100` on MATK | Not visible in `calculateMagicalDamage()` | MISSING from formula -- may be in skill handler |
| Card fix order (magic) | Size -> DefEle -> Race -> Class | Only Race implemented | PARTIAL |
| bMagicAddSize | Consecutive step in cardfix | Not in pipeline | MISSING |
| bMagicAtkEle | Attacker element magic bonus | Not in pipeline | MISSING |
| bMagicDefRace | Defender race magic resist | Not in pipeline | MISSING |
| Hard MDEF formula | `(100-MDEF)/100` pre-renewal | Same | MATCH |
| Hard MDEF cap | 99 | Same | MATCH |
| Soft MDEF | `INT + floor(VIT/5) + floor(DEX/5) + floor(BaseLv/4)` | Same | MATCH |
| Element modifier | 10x10x4 table | Same table | MATCH (verify values) |
| GTB immunity | First check in pipeline | First check | MATCH |
| Lex Aeterna | 2x, consumed on damage | Implemented per skill handler | MATCH |
| Magic Rod | Absorb single-target, SP gain | `checkMagicRodAbsorption()` | MATCH |
| Safety Wall vs magic | No interaction | Not checked in magic pipeline | CORRECT |
| Pneuma vs magic | No interaction | Not checked in magic pipeline | CORRECT |
| Sage zones | Volcano/Deluge/Gale boost | Implemented | MATCH |
| Freeze/Stone MDEF | +25% magic damage taken | `mdefMultiplier: 1.25` | VERIFY -- should be damage INCREASE not MDEF bonus |
| Cast time formula | `base * (1-DEX/150) * modifiers` | Implemented | MATCH |
| ACD reduction (Bragi) | Additive stacking | Implemented | MATCH |
| Over-upgrade MATK | Random 0-N per hit | Needs verification | VERIFY |

### Priority Fixes

1. **bMatkRate order** -- Should be applied to MATK before skill multiplier, not after. Low impact on balance since most staves have 15% bMatkRate and order only matters when skill multiplier != 100%.

2. **Card fix completeness** -- Add `bMagicAddSize` and `bMagicAtkEle` to the magic pipeline for full card compatibility.

3. **Amp integration** -- Ensure Mystical Amplification is properly integrated into the pipeline when High Wizard is implemented.

4. **Freeze/Stone verification** -- The `mdefMultiplier: 1.25` means frozen targets take 25% MORE magic damage. This is applied after MDEF subtraction in our implementation, which matches the rAthena behavior (it is a damage multiplier, not an MDEF change). Verify this is correct.

---

## Sources

- [MATK - iRO Wiki](https://irowiki.org/wiki/MATK)
- [Stats - iRO Wiki Classic](https://irowiki.org/classic/Stats)
- [MDEF - iRO Wiki](https://irowiki.org/wiki/MDEF)
- [rAthena battle.cpp](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
- [rAthena status.cpp](https://github.com/rathena/rathena/blob/master/src/map/status.cpp)
- [Pre-Renewal MATK Calculation Discussion](https://rathena.org/board/topic/143438-pre-renewal-matk-calculation-is-it-working-correctly/)
- [Lex Aeterna - iRO Wiki Classic](https://irowiki.org/classic/Lex_Aeterna)
- [Mystical Amplification - iRO Wiki Classic](https://irowiki.org/classic/Mystical_Amplification)
- [Skills - iRO Wiki Classic (Cast Time)](https://irowiki.org/classic/Skills)
- [Poem of Bragi / Magic Strings - iRO Wiki](https://irowiki.org/wiki/Magic_Strings)
- [Safety Wall - iRO Wiki](https://irowiki.org/wiki/Safety_Wall)
- [Pneuma - iRO Wiki](https://irowiki.org/wiki/Pneuma)
- [Magic Rod - iRO Wiki](https://irowiki.org/wiki/Magic_Rod)
- [Element Table - RateMyServer](https://ratemyserver.net/index.php?page=misc_table_attr)
- [Element - iRO Wiki](https://irowiki.org/wiki/Element)
- [Card Reference - iRO Wiki](https://irowiki.org/wiki/Card_Reference)
- [Refinement System - iRO Wiki](https://irowiki.org/wiki/Refinement_System)
- [After Cast Delay - TalonRO Wiki](https://wiki.talonro.com/After_Cast_Delay)
- [Renewal Calculation Order Issue #8689](https://github.com/rathena/rathena/issues/8689)
- [Card Fix Correction #8727](https://github.com/rathena/rathena/commit/3502c6140a27c8ec8d641b40b3aba734b8c9e802)
- [rAthena item_bonus.txt](https://github.com/rathena/rathena/blob/master/doc/item_bonus.txt)
- [Attacks - iRO Wiki Classic](https://irowiki.org/classic/Attacks)
