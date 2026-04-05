# Merchant -> Blacksmith / Alchemist Skills -- Deep Research

> **Sources**: iRO Wiki Classic, iRO Wiki (Renewal cross-ref), RateMyServer Skill DB, rAthena pre-renewal source (pre.pservero.com), Hercules pre-re skill.c, Ragnarok Fandom Wiki, Divine Pride DB, rAthena GitHub Issues, RateMyServer Forge/Brew Calculators
> **Scope**: Pre-Renewal Classic only. All formulas verified against rAthena pre-re unless noted.
> **Date**: 2026-03-22

---

## Table of Contents

1. [Merchant Skills (IDs 600-609)](#1-merchant-skills-ids-600-609)
2. [Blacksmith Skills (IDs 1200-1230)](#2-blacksmith-skills-ids-1200-1230)
3. [Alchemist Skills (IDs 1800-1815)](#3-alchemist-skills-ids-1800-1815)
4. [Cart System](#4-cart-system)
5. [Forging System](#5-forging-system)
6. [Refining Integration](#6-refining-integration)
7. [Pharmacy / Brewing System](#7-pharmacy--brewing-system)
8. [Weapon Break / Armor Break Mechanics](#8-weapon-break--armor-break-mechanics)
9. [ASPD Buffs (Adrenaline Rush, Weapon Perfection, Maximize Power)](#9-aspd-buffs)
10. [Chemical Protection Skills](#10-chemical-protection-skills)
11. [Acid Terror / Demonstration Formulas](#11-acid-terror--demonstration-formulas)
12. [Homunculus Skills (Call, Rest, Resurrect)](#12-homunculus-skills)
13. [Vending Skill Mechanics](#13-vending-skill-mechanics)
14. [Discount / Overcharge Formulas](#14-discount--overcharge-formulas)
15. [Skill Trees and Prerequisites](#15-skill-trees-and-prerequisites)
16. [Whitesmith / Mastersmith Transcendent Skills](#16-whitesmith--mastersmith-transcendent-skills)
17. [Implementation Checklist](#17-implementation-checklist)
18. [Gap Analysis](#18-gap-analysis)

---

## 1. Merchant Skills (IDs 600-609)

### 1.1 Enlarge Weight Limit (ID 600, rA: MC_INCCARRY / 36)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |
| Unlocks | Discount (Lv3), Pushcart (Lv5) |

**Effect**: `MaxWeight += 200 * SkillLevel` (cumulative: +200 to +2000)

**Per-Level Table**:

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| +Weight | 200 | 400 | 600 | 800 | 1000 | 1200 | 1400 | 1600 | 1800 | 2000 |

**Implementation**: Applied in `getPlayerMaxWeight()`. Base weight is `(STR + BaseLv) * 30 + 2000` (pre-renewal). Enlarge Weight Limit adds on top.

---

### 1.2 Discount (ID 601, rA: MC_DISCOUNT / 37)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Enlarge Weight Limit Lv3 |
| Scope | NPC shop BUY prices only (not player vending, not deals) |

**Formula**: `FinalPrice = floor(BasePrice * (100 - DiscountPct) / 100)`

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| -% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |

**Pattern**: +2% per level (Lv1-9), +1% at Lv10 (diminishing return at cap).
**Floor**: Minimum price = 1 zeny (prevents free items).
**Rogue Compulsion Discount**: Uses `Math.max(merchantDiscount, rogueDiscount)` -- does NOT stack.

---

### 1.3 Overcharge (ID 602, rA: MC_OVERCHARGE / 38)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Discount Lv3 |
| Scope | NPC shop SELL prices only (not deals) |

**Formula**: `FinalPrice = floor(BasePrice * (100 + OverchargePct) / 100)`

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| +% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |

**Pattern**: Identical to Discount. +2% per level (Lv1-9), +1% at Lv10.

---

### 1.4 Mammonite (ID 603, rA: MC_MAMMONITE / 42)

| Property | Value |
|----------|-------|
| Type | Active, Offensive, Melee |
| Max Level | 10 |
| Target | Single enemy |
| Range | 1 cell (melee) |
| Element | Weapon property |
| SP Cost | 5 (all levels) |
| Cast Time | 0 |
| After-Cast Delay | 0 |
| Cooldown | 0 (ASPD-based entirely) |
| Prerequisites | None |

**Damage Formula**: `ATK% = 100 + 50 * SkillLevel`
**Zeny Cost**: `ZenyCost = 100 * SkillLevel`

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| ATK% | 150 | 200 | 250 | 300 | 350 | 400 | 450 | 500 | 550 | 600 |
| Zeny | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | 1000 |

**Key mechanics**:
- Zeny cost deducted BEFORE damage (if insufficient zeny, skill fails)
- Dubious Salesmanship (Blacksmith quest skill 1211) reduces cost by 10%: `floor(zenyCost * 0.9)`
- Uses weapon element, affected by size penalty, affected by cards
- No cooldown, no ACD -- attack rate is purely ASPD-based (can be spammed)
- Standard physical damage pipeline (applies DEF, element table, etc.)

---

### 1.5 Pushcart (ID 604, rA: MC_PUSHCART / 39)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Enlarge Weight Limit Lv5 |
| Unlocks | Vending (Lv3), Change Cart (quest) |

**Effect**: Enables renting a cart from Kafra NPCs. Cart has 100 item slots and 8000 max weight.

**Speed Penalty**: Move speed = `(50 + 5 * SkillLevel)` % of base speed.

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| Speed% | 55% | 60% | 65% | 70% | 75% | 80% | 85% | 90% | 95% | 100% |
| Penalty | -45% | -40% | -35% | -30% | -25% | -20% | -15% | -10% | -5% | 0% |

**Speed application**: Multiplicative with other speed buffs. If player has 120% base speed (e.g., from AGI Up), Pushcart Lv1 = `0.55 * 120% = 66%` effective speed.

**Cart rental**: Must rent from Kafra NPC (free or small fee, server-configurable). Cart persists through logout but not through class change.

---

### 1.6 Vending (ID 605, rA: MC_VENDING / 41)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 10 |
| SP Cost | 30 (all levels) |
| Prerequisites | Pushcart Lv3 |
| Required State | Must have Pushcart equipped |
| NPC Distance | Must be 4+ cells from any NPC |

**Slots per Level**: `Slots = 2 + SkillLevel`

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| Slots | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

See [Section 13](#13-vending-skill-mechanics) for full vending system details.

---

### 1.7 Item Appraisal (ID 606, rA: MC_IDENTIFY / 40)

| Property | Value |
|----------|-------|
| Type | Active |
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 0 |
| ACD | 0 |
| Prerequisites | None |

**Effect**: Identifies one unidentified item in inventory. Opens item selection UI. Functionally identical to using a Magnifier consumable.

**Unidentified items**: Equipment dropped by monsters may be unidentified. Shows generic name (e.g., "One-Handed Sword" instead of "Blade [3]"), cannot be equipped until identified.

---

### 1.8 Change Cart (ID 607, rA: MC_CHANGECART / 154)

| Property | Value |
|----------|-------|
| Type | Active (Quest Skill) |
| Max Level | 1 |
| SP Cost | 40 |
| Prerequisites | Quest completion |
| Required State | Must have Pushcart |

**Cart appearance by base level**:

| Base Lv | Cart Design |
|---------|-------------|
| 1-40 | Default wooden cart |
| 41-65 | Wooden cart (variant) |
| 66-80 | Flower/fern covered cart |
| 81-90 | Panda doll cart |
| 91-99 | Big wheels + roof + banner |

**Quest NPC**: Charlron in Alberta (119, 221). Items: 20 Animal Skin, 10 Iron, 50 Trunk.

---

### 1.9 Cart Revolution (ID 608, rA: MC_CARTREVOLUTION / 153)

| Property | Value |
|----------|-------|
| Type | Active, Offensive, Melee (Quest Skill) |
| Max Level | 1 |
| Target | Single enemy (with 3x3 splash) |
| Range | 1 cell (melee) |
| Element | Forced Neutral (element does NOT change with weapon) |
| SP Cost | 12 |
| Cast Time | 0 |
| ACD | 0 |
| Cooldown | 0 (ASPD-based) |
| Knockback | 2 cells |
| Force Hit | YES (ignores FLEE/accuracy check) |
| Required State | Must have Pushcart equipped |

**Damage Formula**:
```
DamageATK% = 150 + floor(100 * CurrentCartWeight / MaxCartWeight)
```
- MaxCartWeight = 8000 (constant)
- Empty cart (0 weight): 150% ATK
- Half cart (4000 weight): 200% ATK
- Full cart (8000 weight): 250% ATK
- "1% per 80 weight" (8000 / 80 = 100% max bonus)

**Critical mechanics**:
- **Forced Neutral element**: The skill itself is Neutral regardless of weapon element. This means it CANNOT hit Ghost-property targets (Ghost vs Neutral = 0% or 25% depending on level).
- **Force hit**: Ignores FLEE entirely -- always connects.
- **Splash**: Hits all enemies in 3x3 area around the primary target. Splash targets also take full damage and knockback.
- **Knockback**: All hit targets pushed 2 cells away from the attacker.
- The damage IS affected by +ATK bonuses and weapon cards.

**Quest NPC**: Gershaun in Alberta (232, 106).

---

### 1.10 Crazy Uproar / Loud Exclamation (ID 609, rA: MC_LOUD / 155)

| Property | Value |
|----------|-------|
| Type | Active, Self Buff (Quest Skill) |
| Max Level | 1 |
| SP Cost | 8 |
| Cast Time | 0 |
| Duration | 300 seconds (5 minutes) |
| Target | Self only (pre-renewal) |

**Pre-Renewal Effect**: +4 STR (self only).

**Renewal additions (NOT in pre-renewal)**:
- +30 ATK (Renewal only)
- Party-wide effect (Renewal only)

**Cancelled by**: Quagmire.

**Quest NPC**: Necko in Alberta (83, 96). Items: 7 Pearl, 50 Mushroom Spore, 1 Banana Juice.

---

## 2. Blacksmith Skills (IDs 1200-1230)

### Combat Skills (IDs 1200-1211)

### 2.1 Adrenaline Rush (ID 1200, rA: BS_ADRENALINE / 111)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| Cast Time | 0 |
| ACD | 0 |
| Cooldown | 0 |
| Weapon Req | Axe or Mace ONLY |
| Prerequisites | Hammer Fall Lv2 |
| Unlocks | Weapon Perfection (Lv2 + WR Lv2), Power Thrust (Lv3) |

**SP Cost**: `17 + 3 * SkillLevel`
**Duration**: `30 * SkillLevel` seconds

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| SP | 20 | 23 | 26 | 29 | 32 |
| Duration | 30s | 60s | 90s | 120s | 150s |

**ASPD Mechanics (Pre-Renewal)**:
- **Caster** (Blacksmith/Whitesmith): ASPD delay reduced by 30% (attacks 30% faster)
- **Party members** (any class with Axe/Mace equipped): ASPD delay reduced by 20%
- Party members WITHOUT Axe/Mace get NO benefit
- Implementation: `aspdMultiplier = 1.3` (caster) or `1.2` (party)

**Mutual exclusion**: Does NOT stack with Two-Hand Quicken (705), Spear Quicken (708), One-Hand Quicken (1103). Strongest wins (max-wins rule).

**Hilt Binding interaction**: +10% duration if caster has Hilt Binding Lv1.
```
finalDuration = baseDuration * (hasHiltBinding ? 1.1 : 1.0)
```

**Weapon check**: Fails silently if caster is not using Axe or Mace. Party members' benefit is also gated on their equipped weapon type.

---

### 2.2 Weapon Perfection (ID 1201, rA: BS_WEAPONPERFECT / 112)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| Cast Time | 0 |
| ACD | 0 |
| Cooldown | 0 |
| Prerequisites | Adrenaline Rush Lv2 AND Weaponry Research Lv2 |
| Unlocks | Maximize Power (Lv3) |

**SP Cost**: `20 - 2 * SkillLevel`
**Duration**: `10 * SkillLevel` seconds

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| SP | 18 | 16 | 14 | 12 | 10 |
| Duration | 10s | 20s | 30s | 40s | 50s |

**Effect**: Removes ALL weapon size penalties. All physical attacks deal 100% damage regardless of target size (Small/Medium/Large).

**Size penalty table (when NOT active)**:

| Weapon Type | Small | Medium | Large |
|-------------|-------|--------|-------|
| Dagger | 100% | 75% | 50% |
| 1H Sword | 75% | 100% | 75% |
| 2H Sword | 75% | 75% | 100% |
| Axe | 50% | 75% | 100% |
| Mace | 75% | 100% | 75% |
| Spear (unmounted) | 75% | 75% | 100% |
| Spear (mounted) | 75% | 75% | 100% |

**Party-wide**: All party members on screen benefit (no weapon type restriction, unlike Adrenaline Rush).

**Drake Card comparison**: Drake Card provides permanent size penalty removal for the wearer only. Weapon Perfection provides temporary party-wide removal.

**Hilt Binding**: +10% duration.

---

### 2.3 Power Thrust / Over Thrust (ID 1202, rA: BS_OVERTHRUST / 113)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Self (party-wide) |
| Cast Time | 0 |
| ACD | 0 |
| Cooldown | 0 |
| Prerequisites | Adrenaline Rush Lv3 |
| Unlocks | Maximize Power (Lv2) |

**SP Cost**: `20 - 2 * SkillLevel`
**Duration**: `20 * SkillLevel` seconds

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| SP | 18 | 16 | 14 | 12 | 10 |
| Duration | 20s | 40s | 60s | 80s | 100s |
| Self ATK% | +5% | +10% | +15% | +20% | +25% |
| Party ATK% | +5% | +5% | +10% | +10% | +15% |

**Self ATK Formula**: `5 * SkillLevel` % bonus ATK (multiplicative with base ATK)
**Party ATK Formula**: Lv1-2: +5%, Lv3-4: +10%, Lv5: +15%

**Weapon Break Chance**: 0.1% chance per physical attack while active to break the caster's own weapon.
```
if (hasOverThrust && Math.random() < 0.001) {
    // Break own weapon -- set weaponBroken = true
}
```

**Hilt Binding**: +10% duration.

**Stacking with Maximum Over Thrust (Whitesmith)**: The two stack multiplicatively.
- PT Lv5 (+25%) + MOT Lv5 (+100%) = `1.25 * 2.0 = 2.50` (250% total)

---

### 2.4 Maximize Power (ID 1203, rA: BS_MAXIMIZE / 114)

| Property | Value |
|----------|-------|
| Type | Toggle (Active) |
| Max Level | 5 |
| Target | Self |
| Initial SP | 10 SP (on activation) |
| Cast Time | 0 |
| ACD | 0 |
| Prerequisites | Weapon Perfection Lv3 AND Power Thrust Lv2 |

**SP Drain**: 1 SP per `SkillLevel` seconds

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| SP/tick | 1 SP / 1s | 1 SP / 2s | 1 SP / 3s | 1 SP / 4s | 1 SP / 5s |

**Effect**: While active, all physical attacks deal MAXIMUM weapon damage (no weapon variance roll).

**Weapon variance formula (pre-renewal)**:
```
Variance = +/- (0.05 * WeaponLevel * BaseWeaponATK)
```
- Lv1 weapon: +/- 5% variance
- Lv2 weapon: +/- 10% variance
- Lv3 weapon: +/- 15% variance
- Lv4 weapon: +/- 20% variance

With Maximize Power active, the variance always resolves to the MAXIMUM:
```
varianceBonus = 0.05 * weaponLevel * baseWeaponATK  // always max
```

**Does NOT affect**: Overupgrade variance (random 1 to max bonus). Does NOT affect MATK.

**Toggle behavior**: Cast again to deactivate. Auto-deactivates when SP reaches 0 or on death.

---

### 2.5 Weaponry Research (ID 1204, rA: BS_WEAPONRESEARCH / 107)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | Hilt Binding Lv1 |
| Unlocks | Weapon Repair (Lv1), Weapon Perfection (Lv2 with AR Lv2) |

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| +ATK | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| +HIT | 2 | 4 | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 |
| Forge Rate | +1% | +2% | +3% | +4% | +5% | +6% | +7% | +8% | +9% | +10% |

**ATK**: `+2 * SkillLevel` flat mastery ATK (ignores element/size/card modifiers, added post-DEF)
**HIT**: `+2 * SkillLevel` flat HIT bonus
**Forge Rate**: `+1 * SkillLevel` % added to weapon forging success rate

**Note**: Unlike Sword Mastery (swords only), Weaponry Research applies to ALL weapon types.

---

### 2.6 Skin Tempering (ID 1205, rA: BS_SKINTEMPER / 109)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | None |

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| Fire Resist | +4% | +8% | +12% | +16% | +20% |
| Neutral Resist | +1% | +2% | +3% | +4% | +5% |

**Fire Resist**: `4 * SkillLevel` % reduction to Fire element damage
**Neutral Resist**: `1 * SkillLevel` % reduction to Neutral element damage

**Note**: In-game description incorrectly states 5% Fire per level. Actual is 4% per level (confirmed rAthena source).

---

### 2.7 Hammer Fall (ID 1206, rA: BS_HAMMERFALL / 110)

| Property | Value |
|----------|-------|
| Type | Active, Status Only (NO DAMAGE) |
| Max Level | 5 |
| Target | Ground (5x5 AoE) |
| Range | 1 cell (melee) |
| SP Cost | 10 (all levels) |
| Cast Time | 0 |
| ACD | 0 |
| Cooldown | 0 |
| Weapon Req | Axe, Mace, Dagger, or 1H Sword |
| Prerequisites | None |
| Unlocks | Adrenaline Rush (Lv2) |

**Stun Chance**: `20 + 10 * SkillLevel` %

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| Stun% | 30% | 40% | 50% | 60% | 70% |

**Stun duration**: 5 seconds (reduced by target VIT)
**Stun resistance formula**:
```
FinalChance = BaseChance - (BaseChance * TargetVIT / 100) + srcBaseLv - tarBaseLv - tarLUK
```

**CRITICAL**: Hammer Fall deals ZERO damage. It ONLY inflicts Stun status on enemies in the 5x5 AoE. This is unusual for an offensive-looking skill.

---

### 2.8 Hilt Binding (ID 1207, rA: BS_HILTBINDING / 105)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 1 |
| Prerequisites | None |
| Unlocks | Weaponry Research (Lv1), Ore Discovery (with Steel Tempering Lv1) |

**Effects**:
- +1 STR (permanent stat bonus)
- +4 ATK (flat mastery ATK, ignores multipliers)
- +10% duration to Adrenaline Rush, Power Thrust, and Weapon Perfection

---

### 2.9 Ore Discovery (ID 1208, rA: BS_FINDINGORE / 106)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 1 |
| Prerequisites | Steel Tempering Lv1 AND Hilt Binding Lv1 |

**Effect**: Chance to find additional ore items when killing monsters. Triggers on monster kill and drops an ore from the ore group (Iron Ore, Coal, etc.). Estimated ~5% chance per kill (exact rate not precisely documented in rAthena).

---

### 2.10 Weapon Repair (ID 1209, rA: BS_REPAIRWEAPON / 108)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 1 |
| Target | Single ally |
| Range | 2 cells |
| SP Cost | 30 |
| Cast Time | 5000ms |
| ACD | 0 |
| Prerequisites | Weaponry Research Lv1 |

**Materials consumed (from Blacksmith's inventory)**:

| Equipment Type | Material Required |
|---------------|-------------------|
| Weapon Lv1 | 1 Iron Ore |
| Weapon Lv2 | 1 Iron |
| Weapon Lv3 | 1 Steel |
| Weapon Lv4 | 1 Rough Oridecon |
| Armor (any) | 1 Steel |

**Effect**: Repairs a broken weapon or armor on the target. Restores weapon stats, element, card bonuses.

---

### 2.11 Greed (ID 1210, rA: BS_GREED / 1013)

| Property | Value |
|----------|-------|
| Type | Active (Quest Skill) |
| Max Level | 1 |
| AoE | 5x5 cells |
| SP Cost | 10 |
| ACD | 1000ms |
| Quest Req | Job Level 30+ |

**Effect**: Automatically pick up ALL item drops within 2 cells (5x5 area). Cannot be used in towns, PvP, or WoE maps.

---

### 2.12 Dubious Salesmanship (ID 1211, rA: BS_UNFAIRLYTRICK / 1012)

| Property | Value |
|----------|-------|
| Type | Passive (Quest Skill) |
| Max Level | 1 |
| Quest Req | Job Level 25+ |

**Effect**: Reduces Mammonite Zeny cost by 10%.
```
finalZenyCost = floor(baseCost * 0.9)
```

---

### Forging Skills (IDs 1220-1230)

### 2.13 Iron Tempering (ID 1220, rA: BS_IRON / 94)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | None |
| Unlocks | Steel Tempering (Lv1), Enchanted Stone Craft (Lv1) |
| Recipe | 1 Iron Ore + Mini Furnace -> 1 Iron |

**Success Rate**: `40 + 5 * SkillLevel` % base + stat bonuses

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| Base | 45% | 50% | 55% | 60% | 65% |

**Full formula (rAthena)**:
```
make_per = 4000 + (SkillLevel * 500) + (JobLv * 20) + (DEX * 10) + (LUK * 10)
// make_per / 100 = percentage
```

---

### 2.14 Steel Tempering (ID 1221, rA: BS_STEEL / 95)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Iron Tempering Lv1 |
| Unlocks | Ore Discovery (with Hilt Binding Lv1) |
| Recipe | 5 Iron + 1 Coal -> 1 Steel |

**Success Rate**: `30 + 5 * SkillLevel` % base + stat bonuses

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| Base | 35% | 40% | 45% | 50% | 55% |

**Full formula**:
```
make_per = 3000 + (SkillLevel * 500) + (JobLv * 20) + (DEX * 10) + (LUK * 10)
```

---

### 2.15 Enchanted Stone Craft (ID 1222, rA: BS_ENCHANTEDSTONE / 96)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Iron Tempering Lv1 |

**Recipes** (10 elemental ores -> 1 elemental stone):

| Input (10x) | Output |
|-------------|--------|
| Flame Heart Ore | 1 Flame Heart (Fire Stone) |
| Mystic Frozen Ore | 1 Mystic Frozen (Water Stone) |
| Rough Wind Ore | 1 Rough Wind (Wind Stone) |
| Great Nature Ore | 1 Great Nature (Earth Stone) |

**Success Rate**: `10 + 5 * SkillLevel` % base

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| Base | 15% | 20% | 25% | 30% | 35% |

**Full formula**:
```
make_per = 1000 + (SkillLevel * 500) + (JobLv * 20) + (DEX * 10) + (LUK * 10)
```

---

### 2.16 Research Oridecon (ID 1223, rA: BS_ORIDEOCON / 97)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 5 |
| Prerequisites | Enchanted Stone Craft Lv1 |

**Effect**: Increases forging success rate for Level 3 weapons ONLY. +1% per level (+100 to `make_per`).

---

### 2.17-2.23 Smith Skills (IDs 1224-1230)

Seven weapon forging skills, each with 3 levels. Each level adds +5% (500 to `make_per`) to forging success rate for that weapon category.

| ID | Skill | rA ID | Prerequisites | Weapons |
|----|-------|-------|---------------|---------|
| 1224 | Smith Dagger | 98 | None | Knife, Main Gauche, Dirk, Dagger, Stiletto, Gladius, Damascus |
| 1225 | Smith Sword | 99 | Smith Dagger Lv1 | Sword, Falchion, Blade, Rapier, Scimitar, Ring Pommel Saber, Saber, Haedongeum, Tsurugi, Flamberge |
| 1226 | Smith 2H Sword | 100 | Smith Sword Lv1 | Katana, Slayer, Bastard Sword, Two-Handed Sword, Broad Sword, Claymore |
| 1227 | Smith Axe | 101 | Smith Sword Lv2 | Axe, Battle Axe, Hammer, Buster, Two-Handed Axe |
| 1228 | Smith Mace | 102 | Smith Knucklebrace Lv1 | Club, Mace, Smasher, Flail, Chain, Morning Star |
| 1229 | Smith Knucklebrace | 103 | Smith Dagger Lv1 | Knuckle Duster, Brass Knuckle, Waghnak, Hora, Fist, Claw |
| 1230 | Smith Spear | 104 | Smith Dagger Lv2 | Javelin, Spear, Pike, Guisarme, Glaive, Partizan, Trident |

**Per-level bonus**: +10% / +20% / +30% forge rate for that weapon type.

**Unforgeable weapon types**: Rods, Staves, Bows, Instruments, Books, Katars, Level 4 weapons, Guns.

---

## 3. Alchemist Skills (IDs 1800-1815)

### 3.1 Pharmacy / Prepare Potion (ID 1800, rA: AM_PHARMACY / 228)

| Property | Value |
|----------|-------|
| Type | Active (Crafting) |
| Max Level | 10 |
| SP Cost | 5 (all levels) |
| Cast Time | 0 |
| Target | Self |
| Catalyst | 1 Medicine Bowl per attempt |
| Prerequisites | Potion Research Lv5 |
| Unlocks | Acid Terror (Lv5), Demonstration (Lv4), Potion Pitcher (Lv3), CP Helm (Lv2), Summon Flora (Lv6), Summon Marine Sphere (Lv2) |

See [Section 7](#7-pharmacy--brewing-system) for complete crafting system.

---

### 3.2 Acid Terror (ID 1801, rA: AM_ACIDTERROR / 230)

| Property | Value |
|----------|-------|
| Type | Active, Ranged Physical |
| Max Level | 5 |
| Target | Single enemy |
| Range | 9 cells |
| Element | Neutral (NOT forced -- uses element table) |
| SP Cost | 15 (all levels) |
| Cast Time | 1 second (pre-renewal) |
| ACD | 0.5 seconds |
| Catalyst | 1 Acid Bottle per cast |
| Prerequisites | Pharmacy Lv5 |

See [Section 11](#11-acid-terror--demonstration-formulas) for full damage mechanics.

---

### 3.3 Demonstration / Bomb (ID 1802, rA: AM_DEMONSTRATION / 229)

| Property | Value |
|----------|-------|
| Type | Active, Ground AoE DoT |
| Max Level | 5 |
| Target | Ground (3x3) |
| Range | 9 cells |
| Element | Fire |
| SP Cost | 10 (all levels) |
| Cast Time | 1 second (pre-renewal) |
| ACD | 0.5 seconds |
| Catalyst | 1 Bottle Grenade per cast |
| Prerequisites | Pharmacy Lv4 |

See [Section 11](#11-acid-terror--demonstration-formulas) for full damage mechanics.

---

### 3.4 Summon Flora / Bio Cannibalize (ID 1803, rA: AM_CANNIBALIZE / 232)

| Property | Value |
|----------|-------|
| Type | Active, Summon |
| Max Level | 5 |
| Target | Ground |
| Range | 4 cells |
| SP Cost | 20 (all levels) |
| Cast Time | 2 seconds (1.6 var + 0.4 fixed) |
| ACD | 0.5 seconds |
| Catalyst | 1 Plant Bottle per cast |
| Prerequisites | Pharmacy Lv6 |
| Max Active | 5 total (across all types) |

**Summoned plants by level**:

| Lv | Monster | Max | Duration | ATK Range | HP Formula |
|----|---------|-----|----------|-----------|------------|
| 1 | Mandragora | 5 | 5 min | 26-35 | 2430 |
| 2 | Hydra | 4 | 4 min | 22-28 | 2630 |
| 3 | Flora | 3 | 3 min | 242-273 | 2830 |
| 4 | Parasite | 2 | 2 min | 215-430 | 3030 |
| 5 | Geographer | 1 | 1 min | 467-621 | 3230 |

**HP Formula**: `2230 + 200 * SkillLevel`

**Geographer Special** (Lv5): Casts Heal every 5 seconds on nearby players/allies with HP below 60% of max, restoring ~850-900 HP.

**Behavior**: Plants attack enemies within range automatically. Monsters do NOT attack summoned plants. Plant attacks count as summoner's attacks (trigger autocast effects, aggro goes to summoner). Only one plant type active at a time.

---

### 3.5 Axe Mastery (ID 1804, rA: AM_AXEMASTERY / 226)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

**ATK Bonus**: `+3 * SkillLevel` with Axes (mastery ATK -- ignores DEF, all weapon types with "axe" in subType)

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| +ATK | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

---

### 3.6 Potion Research / Learning Potion (ID 1805, rA: AM_LEARNINGPOTION / 227)

| Property | Value |
|----------|-------|
| Type | Passive |
| Max Level | 10 |
| Prerequisites | None |

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| Brew Rate | +1% | +2% | +3% | +4% | +5% | +6% | +7% | +8% | +9% | +10% |
| Potion Heal | +5% | +10% | +15% | +20% | +25% | +30% | +35% | +40% | +45% | +50% |

**Brew Rate**: Stacks with Pharmacy skill level bonus in the brewing formula.
**Potion Heal**: Applies to potions consumed by the Alchemist AND potions thrown via Potion Pitcher.

---

### 3.7 Potion Pitcher / Aid Potion (ID 1806, rA: AM_POTIONPITCHER / 231)

| Property | Value |
|----------|-------|
| Type | Active, Supportive |
| Max Level | 5 |
| Target | Single ally (party/guild or self) |
| Range | 9 cells |
| SP Cost | 1 (all levels) |
| Cast Time | 0.5 seconds |
| ACD | 0.5 seconds |
| Prerequisites | Pharmacy Lv3 |

**Potion type by level**:

| Lv | Potion Consumed | Base Heal (HP) | Multiplier |
|----|----------------|----------------|------------|
| 1 | Red Potion | 45-65 | 110% |
| 2 | Orange Potion | 105-145 | 120% |
| 3 | Yellow Potion | 175-235 | 130% |
| 4 | White Potion | 325-405 | 140% |
| 5 | Blue Potion | 40-60 (SP) | 150% |

**Healing Formula (Pre-Renewal, rAthena skill.c)**:
```
HealAmount = floor(BasePotionValue * (SkillLevelMultiplier / 100))
           * (1 + PotionResearch_Lv * 0.05)

Where:
  BasePotionValue = random(min, max) of potion
  SkillLevelMultiplier = 100 + 10 * SkillLevel (110/120/130/140/150)
```

**Additional modifiers**:
- White Potion (Lv4): `BasePotionValue + VIT * 2` (Hercules pre-re skill.c:8516-8522)
- Blue Potion (Lv5): `BasePotionValue + INT * 2` for SP recovery (same source)
- Increase HP Recovery (target passive): step-based increase (Hercules)
- Homunculus targets: healing is TRIPLED (3x)
- Potion must exist in caster's inventory (consumed per cast)
- Cannot target enemies

---

### 3.8 Summon Marine Sphere / Sphere Mine (ID 1807, rA: AM_SPHEREMINE / 233)

| Property | Value |
|----------|-------|
| Type | Active, Summon |
| Max Level | 5 |
| Target | Ground |
| Range | 1 cell |
| SP Cost | 10 (all levels) |
| Cast Time | 2 seconds (1.6 var + 0.4 fixed) |
| ACD | 0.5 seconds |
| Duration | 30 seconds (or until detonation) |
| Catalyst | 1 Marine Sphere Bottle per cast |
| Prerequisites | Pharmacy Lv2 |
| Max Active | 3 |

**Sphere HP by level**: `2000 + 400 * SkillLevel`

| Lv | 1 | 2 | 3 | 4 | 5 |
|----|---|---|---|---|---|
| HP | 2400 | 2800 | 3200 | 3600 | 4000 |

**Self-destruct mechanics**:
- Detonates when hit by any damage (enemy or player)
- Can be "tapped" to move ~7 cells, then detonates
- Detonation: Fire property, 11x11 AoE
- Damage scales with sphere's remaining HP
- In PvP: damages allies and caster
- No EXP for kills from explosion

---

### 3.9-3.12 Chemical Protection Skills (IDs 1808-1811)

See [Section 10](#10-chemical-protection-skills) for full details.

---

### 3.13 Bioethics (ID 1812, rA: AM_BIOETHICS / 238)

| Property | Value |
|----------|-------|
| Type | Passive (Quest Skill) |
| Max Level | 1 |
| Prerequisites | Complete Bioethics Quest |

**Effect**: Unlocks Homunculus skill tree (Rest, Call Homunculus, Resurrect Homunculus).

---

### 3.14 Call Homunculus (ID 1813, rA: AM_CALLHOMUN / 243)

See [Section 12](#12-homunculus-skills) for full details.

---

### 3.15 Rest / Vaporize (ID 1814, rA: AM_REST / 244)

See [Section 12](#12-homunculus-skills) for full details.

---

### 3.16 Resurrect Homunculus (ID 1815, rA: AM_RESURRECTHOMUN / 247)

See [Section 12](#12-homunculus-skills) for full details.

---

## 4. Cart System

### 4.1 Overview

The cart system is shared between Merchant, Blacksmith, and Alchemist classes. It provides:
- Extra storage (100 item slots, 8000 max weight)
- Cart-based attack skills (Cart Revolution, Cart Termination)
- Movement speed penalty (offset by Pushcart skill level)
- Vending (player shops)

### 4.2 Cart Properties

| Property | Value |
|----------|-------|
| Max Slots | 100 items |
| Max Weight | 8000 |
| Rental | From Kafra NPC (free or small fee) |
| Speed Penalty | `(50 + 5 * PushcartLevel)` % of normal speed |
| At Pushcart Lv10 | 100% normal speed (no penalty) |

### 4.3 Cart Weight and Skill Damage

**Cart Revolution (Merchant):**
```
DamageATK% = 150 + floor(100 * CartWeight / 8000)
```
- Empty cart: 150% ATK
- Half cart (4000): 200% ATK
- Full cart (8000): 250% ATK

**Cart Termination / High Speed Cart Ram (Whitesmith):**
```
DamageATK% = floor(CartWeight / DamageMod) + 100
Where DamageMod = 16 - SkillLevel
```
- At Lv10: DamageMod = 6, Full cart: `floor(8000 / 6) + 100 = 1433%`
- With Power Thrust Lv5 (+25%) + Max Over Thrust Lv5 (+100%): `1433 * 1.25 * 2.0 = 3582%`

| CT Lv | DamageMod | Empty (0) | Half (4000) | Full (8000) | Zeny | Stun% |
|-------|-----------|-----------|-------------|-------------|------|-------|
| 1 | 15 | 100% | 367% | 633% | 600 | 5% |
| 5 | 11 | 100% | 464% | 827% | 1000 | 25% |
| 10 | 6 | 100% | 767% | 1433% | 1500 | 50% |

### 4.4 Cart Storage Implementation

- Separate inventory from player inventory
- Stored as `character_cart` DB table
- Transfer items between inventory and cart
- Cart items only available for vending
- Cart weight tracked as `player.cartWeight` (sum of all items)
- Cart destroyed / inaccessible on class change away from Merchant tree

### 4.5 Cart Visual Types

| Base Level | Appearance | Change Cart Required |
|-----------|------------|---------------------|
| 1-40 | Default wooden | No (auto) |
| 41-65 | Wooden variant | Yes |
| 66-80 | Flower/fern | Yes |
| 81-90 | Panda doll | Yes |
| 91-99 | Big wheels/roof | Yes |

---

## 5. Forging System

### 5.1 Overview

Blacksmiths can forge weapons from raw materials. Forged weapons have special properties:
- Display crafter's name: "XXX's [Weapon Name]"
- Can have elemental property (if elemental stone used)
- Can have Star Crumb bonuses (+ATK)
- Have 0 card slots (slots replaced by element/star crumbs)

### 5.2 Forging Success Rate Formula (rAthena Source)

```js
// Base rate (out of 10000 = 100.00%)
let make_per = 5000;  // 50% base

// Stat bonuses
make_per += jobLevel * 20;                     // +0.2% per job level
make_per += DEX * 10;                          // +0.1% per DEX
make_per += LUK * 10;                          // +0.1% per LUK

// Smith skill bonus (per weapon type)
make_per += smithSkillLevel * 500;             // +5% per level (max +15%)

// Weaponry Research bonus
make_per += weaponryResearchLevel * 100;       // +1% per level (max +10%)

// Oridecon Research bonus (Lv3 weapons ONLY)
if (weaponLevel === 3) {
    make_per += orideconResearchLevel * 100;   // +1% per level (max +5%)
}

// Anvil bonus
make_per += anvilBonus;
// Standard Anvil:   +0
// Oridecon Anvil:    +300  (+3%)
// Golden Anvil:      +500  (+5%)
// Emperium Anvil:    +1000 (+10%)

// Penalties
if (hasElementStone) make_per -= 2000;         // -20% for elemental weapon
make_per -= numStarCrumbs * 1500;              // -15% per Star Crumb (max 3)
make_per -= (weaponLevel - 1) * 1000;          // -10% per weapon level above 1

// Baby class penalty
if (isBabyClass) make_per = floor(make_per * 0.7);  // 30% reduction

// Clamp
make_per = Math.max(0, Math.min(10000, make_per));
```

### 5.3 Example Calculations

**Lv1 Weapon, Smith Lv3, no extras** (JLv50, DEX99, LUK99):
```
5000 + 1000 + 990 + 990 + 1500 + 1000 = 10480 -> capped at 100%
```

**Lv3 Weapon + Fire + 3 Stars, Smith Lv3** (JLv50, DEX99, LUK99, WR10, OR5):
```
5000 + 1000 + 990 + 990 + 1500 + 1000 + 500 + 0 - 2000 - 4500 - 2000 = 1480 -> 14.8%
```

### 5.4 Star Crumb Mastery ATK Values

| Star Crumbs Used | Mastery ATK Bonus |
|-----------------|-------------------|
| 1 | +5 |
| 2 | +10 |
| 3 | +40 |

The +40 for 3 stars is intentional -- a large jump to reward the extreme difficulty.

### 5.5 Element Stones for Forging

| Stone | Element Applied | Damage vs Weakness |
|-------|----------------|-------------------|
| Flame Heart | Fire Lv1 | 150% vs Earth |
| Mystic Frozen | Water Lv1 | 150% vs Fire |
| Rough Wind | Wind Lv1 | 150% vs Water |
| Great Nature | Earth Lv1 | 150% vs Wind |

### 5.6 Anvil Types

| Anvil | Forge Rate Bonus | Obtainable From |
|-------|-----------------|-----------------|
| Standard Anvil | +0% | NPC shop |
| Oridecon Anvil | +3% | Quest/Drop |
| Golden Anvil | +5% | Quest/Drop |
| Emperium Anvil | +10% | Rare drop/Quest |

### 5.7 Complete Forge Recipes

See the Blacksmith Class Research doc (`docsNew/05_Development/Blacksmith_Class_Research.md`) for the full weapon-by-weapon recipe list (7 weapon categories, ~45 individual weapons).

### 5.8 Ranked Blacksmith System

Top 10 Blacksmiths per server earn "ranked" status:
- Ranked BS's forged weapons deal +10 Mastery ATK bonus
- Points: +1 (Lv1 wep +10 refine), +10 (Lv3 wep + 3 components), +25 (Lv2 wep +10), +1000 (Lv3 wep +10)

---

## 6. Refining Integration

### 6.1 Refine Materials

| Equipment | Material | NPC Fee |
|-----------|----------|---------|
| Weapon Lv1 | 1 Phracon | 200z + 50z |
| Weapon Lv2 | 1 Emveretarcon | 1000z + 200z |
| Weapon Lv3 | 1 Oridecon | Drop + 5000z |
| Weapon Lv4 | 1 Oridecon | Drop + 20000z |
| All Armor | 1 Elunium | Drop + 2000z |

### 6.2 Safe Limits

| Equipment | Safe Limit |
|-----------|-----------|
| Weapon Lv1 | +7 |
| Weapon Lv2 | +6 |
| Weapon Lv3 | +5 |
| Weapon Lv4 | +4 |
| Armor | +4 |

### 6.3 Success Rates (Normal Ores, Pre-Renewal)

| Refine | Wep Lv1 | Wep Lv2 | Wep Lv3 | Wep Lv4 | Armor |
|--------|---------|---------|---------|---------|-------|
| +1 | 100% | 100% | 100% | 100% | 100% |
| +2 | 100% | 100% | 100% | 100% | 100% |
| +3 | 100% | 100% | 100% | 100% | 100% |
| +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 19% | 19% | 19% | 9% | 9% |

### 6.4 ATK/DEF Bonus per Refine

| Weapon Level | ATK per Refine | Over-Safe Bonus (per level beyond safe) |
|-------------|---------------|----------------------------------------|
| Level 1 | +2 | +3 |
| Level 2 | +3 | +5 |
| Level 3 | +5 | +7 |
| Level 4 | +7 | +13 |
| Armor | floor((3+refineLv)/4) DEF | N/A |

**Overupgrade random variance**: Above safe limit, each hit adds random `1 ~ maxOverupgradeBonus` ATK. The max bonus accumulates: e.g., Lv4 weapon at +10 (safe +4) = 6 levels over safe * 13 = max 78 random variance per hit.

### 6.5 Failure Consequence

Equipment is permanently destroyed. All cards, enchantments, and refine levels are lost. Ore and zeny are consumed. No downgrade -- only destruction.

### 6.6 Whitesmith Upgrade Weapon (rA: 477)

Whitesmith-exclusive NPC substitute. Higher success rates than normal NPC refiners (exact rates server-configurable, typically +5-10% above normal).

---

## 7. Pharmacy / Brewing System

### 7.1 Overview

The Pharmacy system allows Alchemists to craft potions, bottles, and special items. Requirements per attempt:
1. Pharmacy skill (active)
2. A Creation Guide (appropriate for the item, NOT consumed)
3. Recipe ingredients (ALL consumed on attempt, success or fail)
4. 1 Medicine Bowl (consumed per attempt)

### 7.2 Success Rate Formula (Pre-Renewal)

```
SuccessRate% = (Pharmacy_Lv * 3) + (PotionResearch_Lv * 1)
             + (InstructionChange_Lv * 1)           // Vanilmirth Homunculus passive
             + floor(JobLv * 0.2) + floor(DEX * 0.1)
             + floor(LUK * 0.1) + floor(INT * 0.05)
             + ItemDifficultyModifier
```

### 7.3 Item Difficulty Modifiers

| Item Category | Rate Modifier |
|--------------|---------------|
| Red Potion | +25% |
| Yellow Potion | +20% |
| White Potion | +15% |
| Blue Potion | -5% |
| Alcohol | +10% |
| Acid Bottle | +0% |
| Plant Bottle | +5% |
| Marine Sphere Bottle | +0% |
| Bottle Grenade | +0% |
| Glistening Coat | -10% |
| Anodyne | -5% |
| Aloevera | -5% |
| Embryo | -5% |
| Condensed Red Potion | -5% |
| Condensed Yellow Potion | -7% |
| Condensed White Potion | -10% |
| Fireproof/Coldproof/Thunderproof/Earthproof | -10% |

### 7.4 Creation Guides

| Guide | Required For |
|-------|-------------|
| Potion Creation Guide | Red/Yellow/White/Blue Potions, Alcohol, Acid/Plant/Marine Bottles, Bottle Grenade, Glistening Coat, Anodyne, Aloevera, Embryo |
| Condensed Potion Guide | Condensed Red/Yellow/White Potions |
| Elemental Potion Guide | Fireproof/Coldproof/Thunderproof/Earthproof Potions |

### 7.5 Complete Recipe List

#### Basic Potions

| Item | Ingredients |
|------|------------|
| Red Potion | 1 Empty Potion Bottle + 1 Red Herb |
| Yellow Potion | 1 Empty Potion Bottle + 1 Yellow Herb |
| White Potion | 1 Empty Potion Bottle + 1 White Herb |
| Blue Potion | 1 Empty Potion Bottle + 1 Blue Herb + 1 Scell |

#### Utility Potions

| Item | Ingredients |
|------|------------|
| Anodyne | 1 Empty Bottle + 1 Alcohol + 1 Ment |
| Aloevera | 1 Empty Bottle + 1 Honey + 1 Aloe |

#### Condensed Potions

| Item | Ingredients |
|------|------------|
| Condensed Red | 1 Red Potion + 1 Cactus Needle |
| Condensed Yellow | 1 Yellow Potion + 1 Mole Whiskers |
| Condensed White | 1 White Potion + 1 Witch Starsand |

#### Combat Bottles (Skill Catalysts)

| Item | Ingredients | Used By |
|------|------------|---------|
| Alcohol | 1 Empty Test Tube + 1 Empty Bottle + 5 Stems + 5 Poison Spore | Component |
| Acid Bottle | 1 Empty Bottle + 1 Immortal Heart | Acid Terror |
| Plant Bottle | 1 Empty Bottle + 2 Maneater Blossom | Summon Flora |
| Marine Sphere Bottle | 1 Empty Bottle + 1 Tendon + 1 Detonator | Summon Marine Sphere |
| Bottle Grenade | 1 Empty Bottle + 1 Fabric + 1 Alcohol | Demonstration |
| Glistening Coat | 1 Empty Bottle + 1 Mermaid's Heart + 1 Zenorc Fang + 1 Alcohol | Chemical Protection |

#### Elemental Resistance Potions

| Item | Ingredients |
|------|------------|
| Fireproof Potion | 1 Empty Potion Bottle + 1 Red Gemstone + 2 Frills |
| Coldproof Potion | 1 Empty Potion Bottle + 1 Blue Gemstone + 3 Mermaid's Heart |
| Thunderproof Potion | 1 Blue Gemstone + 3 Moth Dust |
| Earthproof Potion | 1 Yellow Gemstone + 2 Large Jelloopy |

#### Special Items

| Item | Ingredients | Notes |
|------|------------|-------|
| Embryo | 1 Medicine Bowl + 1 Glass Tube + 1 Dew of Yggdrasil + 1 Seed of Life | Req Bioethics |
| Homunculus Tablet | 1 Yellow Herb + 1 Seed of Life + 1 Empty Bottle | +50 intimacy |

### 7.6 Example Calculation

**Stats**: Pharmacy Lv10, Potion Research Lv10, JLv50, DEX90, LUK50, INT80
```
Base = 30 + 10 = 40%
Stats = floor(50*0.2) + floor(90*0.1) + floor(50*0.1) + floor(80*0.05) = 10 + 9 + 5 + 4 = 28%
White Potion (+15%): 40 + 28 + 15 = 83%
Glistening Coat (-10%): 40 + 28 - 10 = 58%
Embryo (-5%): 40 + 28 - 5 = 63%
```

### 7.7 Adopted Alchemist Penalty

Adopted (Baby) Alchemists suffer -30% success rate penalty to ALL brewing.

### 7.8 Fame System (Optional / Deferred)

- 3 consecutive condensed potion successes = +1 fame
- 5 consecutive = +3 fame
- 7 consecutive = +10 fame
- 10 consecutive = +50 fame
- Top 10 ranked Alchemists' potions get +50% efficacy bonus

---

## 8. Weapon Break / Armor Break Mechanics

### 8.1 Weapon Break Sources

| Source | Break Chance | Target | Condition |
|--------|-------------|--------|-----------|
| Power Thrust (1202) | 0.1% per attack | Caster's own weapon | While buff is active |
| Demonstration (1802) | 1-5% per tick | Enemy weapons | Standing in fire zone |
| Melt Down / Shattering Strike (Whitesmith) | Level-based | Enemy weapon + armor | Per-hit chance |
| Maximum Over Thrust (Whitesmith) | 0.1% per attack | Caster's own weapon | While buff is active |

### 8.2 Armor Break Sources

| Source | Break Chance | Target |
|--------|-------------|--------|
| Acid Terror (1801) | 3/7/10/12/13% | Enemy armor |
| Melt Down (Whitesmith) | Level-based | Enemy armor |

### 8.3 Weapon Break Effects

When a weapon breaks:
- Weapon ATK = 0 (no weapon damage)
- Weapon element reverts to Neutral
- All weapon card bonuses removed
- Weapon still equipped but non-functional
- Displays special broken weapon icon
- Repaired by: Weapon Repair (Blacksmith), NPC repair service

### 8.4 Armor Break Effects

When armor breaks:
- Armor DEF = 0
- Armor element reverts to Neutral
- All armor card bonuses removed
- Still equipped but non-functional
- Repaired by: Weapon Repair, NPC repair

### 8.5 Protection Against Break

- Chemical Protection (Alchemist): Prevents break AND strip for specific slot
- Cranial/Horn-type cards: Some reduce break chance
- Boss monsters: immune to weapon/armor break from standard sources

---

## 9. ASPD Buffs

### 9.1 Adrenaline Rush (1200)

- **Caster**: ASPD delay * 0.7 (30% faster attacks)
- **Party**: ASPD delay * 0.8 (20% faster attacks, Axe/Mace only)
- **Duration**: 30-150s
- **Mutual exclusion**: THQ, SQ, OHQ (strongest wins)
- **Hilt Binding**: +10% duration

### 9.2 Weapon Perfection (1201)

- **Effect**: Removes size penalty (all attacks = 100% vs all sizes)
- **Party-wide**: Yes (no weapon restriction)
- **Duration**: 10-50s
- **Hilt Binding**: +10% duration

### 9.3 Power Thrust / Over Thrust (1202)

- **Caster**: +5-25% ATK (multiplicative)
- **Party**: +5-15% ATK
- **Weapon break**: 0.1% per attack on caster's weapon
- **Duration**: 20-100s
- **Hilt Binding**: +10% duration

### 9.4 Maximize Power (1203)

- **Effect**: Always max weapon variance (no random roll)
- **SP Drain**: 1 SP per SkillLevel seconds
- **Toggle**: Cast again to deactivate
- **Does NOT affect**: MATK, overupgrade variance

### 9.5 ASPD Mutual Exclusion Groups

| Group | Skills | Rule |
|-------|--------|------|
| ASPD Haste | Adrenaline Rush, THQ, SQ, OHQ, Full AR | Strongest wins |
| Size Penalty | Weapon Perfection, Drake Card | Coexist (both set noSizePenalty) |
| ATK Boost | Power Thrust, Max Over Thrust | Stack multiplicatively |

### 9.6 Buff Application Order

When casting a new ASPD buff:
1. Check mutual exclusion group
2. If new buff is stronger (higher ASPD multiplier), remove old, apply new
3. If new buff is weaker, reject with message "A stronger buff is already active"
4. Apply Hilt Binding duration extension
5. Broadcast buff_applied event

---

## 10. Chemical Protection Skills

### 10.1 CP Helm (ID 1808, rA: AM_CP_HELM / 237)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 20 |
| Cast Time | 2s (fixed) |
| ACD | 0.5s |
| Range | 1 cell |
| Catalyst | 1 Glistening Coat |
| Prerequisites | Pharmacy Lv2 |
| Protects | Headgear slot (break + strip) |

### 10.2 CP Shield (ID 1809, rA: AM_CP_SHIELD / 235)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 25 |
| Cast Time | 2s (fixed) |
| ACD | 0.5s |
| Range | 1 cell |
| Catalyst | 1 Glistening Coat |
| Prerequisites | CP Helm Lv3 |
| Protects | Shield slot |

### 10.3 CP Armor (ID 1810, rA: AM_CP_ARMOR / 236)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 25 |
| Cast Time | 2s (fixed) |
| ACD | 0.5s |
| Range | 1 cell |
| Catalyst | 1 Glistening Coat |
| Prerequisites | CP Shield Lv3 |
| Protects | Armor slot (also protects armor element) |

### 10.4 CP Weapon (ID 1811, rA: AM_CP_WEAPON / 234)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 30 |
| Cast Time | 2s (fixed) |
| ACD | 0.5s |
| Range | 1 cell |
| Catalyst | 1 Glistening Coat |
| Prerequisites | CP Armor Lv3 |
| Protects | Weapon slot |

### 10.5 Duration (All 4 CP Skills)

Formula: `Duration = SkillLevel * 120 seconds`

| Lv | Duration |
|----|----------|
| 1 | 2 min (120s) |
| 2 | 4 min (240s) |
| 3 | 6 min (360s) |
| 4 | 8 min (480s) |
| 5 | 10 min (600s) |

### 10.6 What CP Protects Against

- Weapon/armor break from: Power Thrust self-break, Demonstration weapon break, Acid Terror armor break, Melt Down, monster attacks
- Equipment strip from: Rogue's Divest Helm/Shield/Armor/Weapon, Full Divestment (Stalker)
- Full Chemical Protection (Biochemist/Creator): All 4 slots at once (single cast)

### 10.7 What CP Does NOT Protect Against

- Forced unequip effects (certain game mechanics)
- Equipment cursing effects
- Dispell (Sage skill) removes the CP buff itself

---

## 11. Acid Terror / Demonstration Formulas

### 11.1 Acid Terror (ID 1801) -- Full Damage Pipeline

| Lv | ATK% | Armor Break% | Bleeding% |
|----|------|-------------|-----------|
| 1 | 140% | 3% | 3% |
| 2 | 180% | 7% | 6% |
| 3 | 220% | 10% | 9% |
| 4 | 260% | 12% | 12% |
| 5 | 300% | 13% | 15% |

**ATK Formula**: `ATK% = 100 + 40 * SkillLevel`

**Damage calculation pipeline**:
1. Calculate weapon ATK + flat ATK bonuses (mastery, +ATK cards)
2. Apply ATK% multiplier (140-300%)
3. **SKIP hard DEF** (ignores armor DEF entirely)
4. Apply VIT soft DEF reduction (NOT bypassed)
5. **Force hit** (no HIT/FLEE check -- always connects)
6. Element table IS applied (Neutral element, but affected by defender element)
7. Against boss monsters: final damage * 50%

**Catalyst**: 1 Acid Bottle consumed at START of cast (lost even if interrupted)

**Blocked by**:
- Pneuma: blocks damage, but armor break/bleeding still apply
- Kyrie Eleison: blocks both damage and status effects
- Auto Guard: blocks both

**Card interactions**: +ATK cards increase damage. +% cards and status proc cards have NO effect.

### 11.2 Demonstration / Bomb (ID 1802) -- Full Mechanics

| Lv | ATK% per tick | Duration | Weapon Break% |
|----|-------------|----------|---------------|
| 1 | 120% | 40s | 1% (rAthena: 3%) |
| 2 | 140% | 45s | 2% (rAthena: 6%) |
| 3 | 160% | 50s | 3% (rAthena: 9%) |
| 4 | 180% | 55s | 4% (rAthena: 12%) |
| 5 | 200% | 60s | 5% (rAthena: 15%) |

**ATK Formula**: `ATK% = 100 + 20 * SkillLevel` per tick

**Note on weapon break %**: Sources disagree. iRO Wiki shows 1-5%, rAthena source shows 3*Lv (3-15%). The rAthena values are more commonly used in private servers.

**Tick rate**: Damage every 0.5 seconds (500ms)

**Duration formula**: `35 + 5 * SkillLevel` seconds

**Properties**:
- Fire element damage (uses fire vs defender element table)
- Physical ATK-based but only uses flat ATK (no +% cards)
- Each tick checks if enemies are within the 3x3 area
- Enemies can walk into/out of the AoE
- Weapon break check per tick per enemy
- Cannot stack bombs; cannot place adjacent to existing bombs
- Cannot cast directly under enemies

**Catalyst**: 1 Bottle Grenade per cast

---

## 12. Homunculus Skills

### 12.1 Bioethics (ID 1812)

Passive quest skill. Unlocks Rest -> Call Homunculus -> Resurrect Homunculus.

### 12.2 Rest / Vaporize (ID 1814, rA: AM_REST / 244)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 50 |
| Cast Time | 0 |
| Target | Self (active Homunculus) |
| Prerequisites | Bioethics Lv1 |
| Requirement | Homunculus HP >= 80% of MaxHP |

**Effect**: Stores the active Homunculus. It disappears from the field and can be re-summoned later with Call Homunculus (no Embryo needed). Intimacy is NOT reduced. Full state preserved (HP/SP/stats/skills/intimacy/hunger).

### 12.3 Call Homunculus (ID 1813, rA: AM_CALLHOMUN / 243)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 10 |
| Cast Time | 0 |
| Prerequisites | Bioethics Lv1, Rest Lv1 |

**First summon**:
- Requires 1 Embryo in inventory (consumed)
- Random type: 25% each (Lif, Amistr, Filir, Vanilmirth)
- 50% chance per visual variant within type
- Permanently bound to character

**Subsequent summons**: No Embryo needed. Re-summons stored Homunculus with full state.

**Delete and re-roll**: Delete current Homunculus permanently, then use Call with new Embryo for new random type.

### 12.4 Resurrect Homunculus (ID 1815, rA: AM_RESURRECTHOMUN / 247)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| Cast Time | 3 seconds (1 fixed + 2 variable) |
| Prerequisites | Call Homunculus Lv1 |

| Lv | HP Restored | SP Cost | Cooldown |
|----|------------|---------|----------|
| 1 | 20% | 74 | 140s |
| 2 | 40% | 68 | 110s |
| 3 | 60% | 62 | 80s |
| 4 | 80% | 56 | 50s |
| 5 | 100% | 50 | 20s |

**SP Cost Formula**: `80 - (SkillLevel * 6)`
**Cooldown Formula**: `170 - (SkillLevel * 30)` seconds

### 12.5 Homunculus Types Summary

| Type | Role | Food | Race | Key Skill |
|------|------|------|------|-----------|
| Lif | Support/Heal | Pet Food | Demi Human | Healing Hands |
| Amistr | Tank | Zargon | Brute | Castling + Bulwark |
| Filir | DPS | Garlet | Brute | Moonlight + Flitting |
| Vanilmirth | Magic/RNG | Scell | Formless | Caprice + Instruction Change |

### 12.6 Homunculus Derived Stats

```
ATK  = floor((STR + DEX + LUK) / 3) + floor(Level / 10)
MATK = Level + INT + floor((INT + DEX + LUK) / 3) + floor(Level / 10) * 2
HIT  = Level + DEX + 150
CRIT = floor(LUK / 3) + 1
DEF  = (VIT + floor(Level / 10)) * 2 + floor((AGI + floor(Level / 10)) / 2) + floor(Level / 2)
FLEE = Level + AGI + floor(Level / 10)
ASPD = 130 (base, 1.4s attack interval)
```

### 12.7 Intimacy System

| Status | Range | Behavior |
|--------|-------|----------|
| Hate with Passion | 1-3 | About to abandon |
| Hate | 4-10 | Very unhappy |
| Awkward | 11-100 | Uncomfortable |
| Shy | 101-250 | Timid |
| Neutral | 251-750 | Default |
| Cordial | 751-910 | Friendly |
| Loyal | 911-1000 | Eligible for evolution |

**Feeding rules** (based on hunger%):

| Hunger | Intimacy Change |
|--------|-----------------|
| 1-10% | +0.5 |
| 11-25% (optimal) | +1.0 |
| 26-75% | +0.75 |
| 76-90% | -0.05 |
| 91-100% | -0.5 |
| 0% (starving) | -1.0 per tick |

**Starvation**: -18 intimacy per hour. After 24 hours (432 total), Homunculus permanently abandoned.

### 12.8 Evolution

- Requirements: Intimacy 911+ (Loyal), Stone of Sage item (consumed)
- Effects: +1 to +10 random bonus per stat, increased MaxHP/MaxSP, new visual sprite, unlocks 4th skill
- Intimacy resets to 10 (Hate)

---

## 13. Vending Skill Mechanics

### 13.1 Setup Requirements

1. Must be Merchant, Blacksmith, Alchemist (or trans)
2. Must have Pushcart equipped
3. Must have Vending skill learned
4. Must be 4+ cells from any NPC
5. Must be in a vend-allowed map (not PvP, not WoE)

### 13.2 Slots

Formula: `Slots = 2 + SkillLevel`

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| Slots | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |

### 13.3 Pricing Rules

- Only items in Pushcart can be vended (not player inventory)
- Max price per item: 1,000,000,000 zeny
- 5% commission on items priced above 10,000,000z (vendor receives 95%)
- Price listed is per-unit price (buyers can select quantity from a stack)
- Minimum price: 1 zeny

### 13.4 Shop Behavior

- Shop title: max 80 characters (player-entered)
- Character sits down and displays shop sign above head
- Cannot move, attack, or use skills while vending
- Shop closes when: all items sold, character dies, player manually closes, disconnects
- Buyers click on vending character to open shop window
- Revenue goes directly to character's zeny (minus commission)
- SP Cost: 30 (deducted on shop open)

### 13.5 Buyer Flow

1. Click on vending character
2. Shop window opens showing items, prices, quantities
3. Select item, choose quantity
4. Total calculated as price * quantity
5. Confirm purchase
6. Zeny deducted from buyer, items added to buyer inventory
7. Items removed from vendor's cart

---

## 14. Discount / Overcharge Formulas

### 14.1 Discount (ID 601)

```
BuyPrice = floor(NPCBasePrice * (100 - DiscountPct) / 100)
BuyPrice = max(1, BuyPrice)  // minimum 1 zeny
```

### 14.2 Overcharge (ID 602)

```
SellPrice = floor(NPCBaseSellPrice * (100 + OverchargePct) / 100)
```

**Base NPC Sell Price**: Typically 50% of buy price (item_db configured).

### 14.3 Percentage Table

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|---|
| Discount -% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |
| Overcharge +% | 7 | 9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 24 |

### 14.4 Non-Stacking with Rogue

Rogue's Compulsion Discount (1716, same as 601) shares identical percentages. They do NOT stack -- `Math.max(merchantDiscount, rogueDiscount)` is used.

---

## 15. Skill Trees and Prerequisites

### 15.1 Merchant Skill Tree

```
Enlarge Weight Limit (600) -- Lv3 --> Discount (601) -- Lv3 --> Overcharge (602)
Enlarge Weight Limit (600) -- Lv5 --> Pushcart (604) -- Lv3 --> Vending (605)
Item Appraisal (606): No prereqs
Mammonite (603): No prereqs
Quest Skills: Cart Revolution (608), Change Cart (607), Crazy Uproar (609)
```

### 15.2 Blacksmith Skill Tree

```
Combat Branch:
  Hammer Fall (1206) -- Lv2 --> Adrenaline Rush (1200)
  Hilt Binding (1207) --> Weaponry Research (1204) -- Lv1 --> Weapon Repair (1209)
  AR (1200) Lv2 + WR (1204) Lv2 --> Weapon Perfection (1201)
  AR (1200) Lv3 --> Power Thrust (1202)
  WP (1201) Lv3 + PT (1202) Lv2 --> Maximize Power (1203)
  Skin Tempering (1205): No prereqs

Forging Branch:
  Iron Tempering (1220) -- Lv1 --> Steel Tempering (1221)
  Iron Tempering (1220) -- Lv1 --> Enchanted Stone Craft (1222) -- Lv1 --> Research Oridecon (1223)
  Steel Tempering (1221) Lv1 + Hilt Binding (1207) Lv1 --> Ore Discovery (1208)

Smith Branch:
  Smith Dagger (1224): No prereqs
  Smith Dagger (1224) Lv1 --> Smith Sword (1225) -- Lv1 --> Smith 2H Sword (1226)
  Smith Sword (1225) Lv2 --> Smith Axe (1227)
  Smith Dagger (1224) Lv1 --> Smith Knucklebrace (1229) -- Lv1 --> Smith Mace (1228)
  Smith Dagger (1224) Lv2 --> Smith Spear (1230)

Quest Skills: Greed (1210), Dubious Salesmanship (1211)
```

### 15.3 Alchemist Skill Tree

```
Potion Research (1805) -- Lv5 --> Pharmacy (1800)
Pharmacy (1800) -- Lv2 --> Acid Terror (1801)
Pharmacy (1800) -- Lv4 --> Demonstration (1802)
Pharmacy (1800) -- Lv3 --> Potion Pitcher (1806)
Pharmacy (1800) -- Lv2 --> CP Helm (1808)
Pharmacy (1800) -- Lv6 --> Summon Flora (1803)
Pharmacy (1800) -- Lv2 --> Summon Marine Sphere (1807)

CP Helm (1808) Lv3 --> CP Shield (1809) Lv3 --> CP Armor (1810) Lv3 --> CP Weapon (1811)

Axe Mastery (1804): No prereqs

Bioethics (1812, Quest) --> Rest (1814)
Bioethics (1812) + Rest (1814) --> Call Homunculus (1813)
Call Homunculus (1813) --> Resurrect Homunculus (1815)
```

### 15.4 Known Prerequisite Bugs (in Sabri_MMO codebase)

| Skill | Current Code | Correct (rAthena) |
|-------|-------------|-------------------|
| Adrenaline Rush (1200) | No prereqs | Hammer Fall Lv2 |
| Weapon Perfection (1201) | AR Lv2 only | AR Lv2 + WR Lv2 |
| Power Thrust (1202) | AR Lv5 | AR Lv3 |
| Maximize Power (1203) | PT Lv5 + WP Lv3 | PT Lv2 + WP Lv3 |
| Smith Dagger (1224) | WR Lv1 | None |
| Smith Axe (1227) | SD Lv1 | SS Lv2 |
| Smith Mace (1228) | SD Lv1 | SKB Lv1 |
| Smith Knucklebrace (1229) | SM Lv1 | SD Lv1 |
| Smith Spear (1230) | SD Lv1 | SD Lv2 |
| Research Oridecon (1223) | IT Lv1 | ESC Lv1 |

**Total prerequisite bugs: 10** (9 from Blacksmith + 0 from Merchant + 1 corrected from prior, if not already fixed).

---

## 16. Whitesmith / Mastersmith Transcendent Skills

### 16.1 Maximum Over Thrust (rA: WS_OVERTHRUST / 484)

| Property | Value |
|----------|-------|
| Max Level | 5 |
| SP Cost | 15 (all levels) |
| Zeny Cost | 3000 * SkillLevel |
| Duration | 60/90/120/150/180s |
| Self Only | Yes (does NOT affect party) |
| Prerequisites | Power Thrust Lv5 |

**ATK Bonus**: +20/40/60/80/100% (self only)

**Weapon break**: 0.1% per attack (same as Power Thrust)

**Stacks with Power Thrust**: Multiplicatively.
```
PT Lv5 (+25%) + MOT Lv5 (+100%): 1.25 * 2.0 = 2.50 (250% total ATK)
```

**Cancelled by**: Gear change (unequip/equip weapon cancels the buff).

### 16.2 Cart Boost (rA: WS_CARTBOOST / 387)

| Property | Value |
|----------|-------|
| Max Level | 1 |
| SP Cost | 20 |
| Duration | 60 seconds |
| Required State | Must have Pushcart |
| Prerequisites | Cart Revolution Lv1, Hilt Binding Lv1, Pushcart Lv2 |

**Effect**: Increases movement speed significantly (similar to Increase AGI Lv10 speed). Required for Cart Termination.

### 16.3 Cart Termination / High Speed Cart Ram (rA: WS_CARTTERMINATION / 485)

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Target | Single enemy |
| Range | 1 cell (melee) |
| Required State | Cart Boost must be active |
| Prerequisites | Cart Boost Lv1 |

**Damage Formula**:
```
DamageATK% = floor(CartWeight / (16 - SkillLevel)) + 100
```

| Lv | DamageMod | Full Cart (8000) Damage | Zeny | Stun% |
|----|-----------|------------------------|------|-------|
| 1 | 15 | 633% | 600 | 5% |
| 2 | 14 | 671% | 700 | 10% |
| 3 | 13 | 715% | 800 | 15% |
| 4 | 12 | 767% | 900 | 20% |
| 5 | 11 | 827% | 1000 | 25% |
| 6 | 10 | 900% | 1100 | 30% |
| 7 | 9 | 989% | 1200 | 35% |
| 8 | 8 | 1100% | 1300 | 40% |
| 9 | 7 | 1243% | 1400 | 45% |
| 10 | 6 | 1433% | 1500 | 50% |

**Key**: Power Thrust and Max Over Thrust stack additively with this skill's damage.

### 16.4 Melt Down / Shattering Strike (rA: WS_MELTDOWN / 384)

| Property | Value |
|----------|-------|
| Max Level | 10 |
| Target | Self buff |
| SP Cost | 50 (all levels) |
| Duration | 15/20/25/30/35/40/45/50/55/60s |
| Prerequisites | Skin Tempering Lv3, Weaponry Research Lv5 |

**Effect**: Adds chance to break target's weapon or armor on each physical hit.

| Lv | Weapon Break% | Armor Break% |
|----|-------------|-------------|
| 1 | 1% | 1% |
| 5 | 5% | 5% |
| 10 | 10% | 10% |

**Against monsters**: Reduces ATK or DEF (equivalent to breaking weapon/armor).

**Cannot affect**: Boss monsters.

### 16.5 Upgrade Weapon (rA: WS_WEAPONREFINE / 477)

| Property | Value |
|----------|-------|
| Max Level | 10 |
| SP Cost | 5-50 (varies) |
| Prerequisites | Weaponry Research Lv10, Iron Tempering Lv5 |

**Effect**: Refine a weapon using the Whitesmith's own materials instead of an NPC. Success rates are higher than NPC refiners at higher skill levels.

### 16.6 Unfair Trick (rA: BS_UNFAIRLYTRICK / 1012)

Already covered in Section 2.12 (Dubious Salesmanship).

---

## 17. Implementation Checklist

### Merchant Skills

| ID | Skill | Status | Notes |
|----|-------|--------|-------|
| 600 | Enlarge Weight Limit | COMPLETE | Applied in getPlayerMaxWeight |
| 601 | Discount | COMPLETE | Applied in shop:buy |
| 602 | Overcharge | COMPLETE | Applied in shop:sell |
| 603 | Mammonite | COMPLETE | Fixed cooldown (was 800, now 0) |
| 604 | Pushcart | COMPLETE (def only) | Cart system implemented |
| 605 | Vending | COMPLETE | Full vending system implemented |
| 606 | Item Appraisal | COMPLETE | Identify system implemented |
| 607 | Change Cart | STUB | Cosmetic, low priority |
| 608 | Cart Revolution | COMPLETE | Weight scaling + force hit |
| 609 | Crazy Uproar | COMPLETE | +4 STR buff |

### Blacksmith Combat Skills

| ID | Skill | Status | Notes |
|----|-------|--------|-------|
| 1200 | Adrenaline Rush | COMPLETE | ASPD buff, weapon check, party |
| 1201 | Weapon Perfection | COMPLETE | noSizePenalty flag, party |
| 1202 | Power Thrust | COMPLETE | ATK%, weapon break 0.1% |
| 1203 | Maximize Power | COMPLETE | Toggle, SP drain, max variance |
| 1204 | Weaponry Research | COMPLETE | Passive ATK/HIT/forge rate |
| 1205 | Skin Tempering | COMPLETE | Fire/Neutral resist |
| 1206 | Hammer Fall | COMPLETE | Stun AoE, no damage |
| 1207 | Hilt Binding | COMPLETE | +1 STR, +4 ATK, +10% duration |
| 1208 | Ore Discovery | COMPLETE | Ore drop on kill |
| 1209 | Weapon Repair | COMPLETE | Repairs broken equipment |
| 1210 | Greed | DEFERRED | Requires ground loot system |
| 1211 | Dubious Sales | COMPLETE | -10% Mammonite cost |

### Blacksmith Forging Skills

| ID | Skill | Status | Notes |
|----|-------|--------|-------|
| 1220 | Iron Tempering | COMPLETE | Forge system implemented |
| 1221 | Steel Tempering | COMPLETE | Forge system implemented |
| 1222 | Enchanted Stone | COMPLETE | Forge system implemented |
| 1223 | Research Oridecon | COMPLETE | Lv3 weapon bonus |
| 1224-1230 | Smith (7 types) | COMPLETE | All forge recipes |

### Alchemist Skills

| ID | Skill | Status | Notes |
|----|-------|--------|-------|
| 1800 | Pharmacy | COMPLETE | Full brewing system |
| 1801 | Acid Terror | COMPLETE | Ignore hard DEF, force hit, armor break |
| 1802 | Demonstration | COMPLETE | Fire ground DoT, weapon break |
| 1803 | Summon Flora | COMPLETE | 5 plant types, auto-attack |
| 1804 | Axe Mastery | COMPLETE | Passive +3/lv |
| 1805 | Potion Research | COMPLETE | Brew rate + heal bonus |
| 1806 | Potion Pitcher | COMPLETE | Throw potions, VIT*2/INT*2 |
| 1807 | Summon Marine Sphere | COMPLETE | Fire AoE self-destruct |
| 1808-1811 | CP Helm/Shield/Armor/Weapon | COMPLETE | preventBreak/Strip buffs |
| 1812 | Bioethics | COMPLETE | Quest skill, unlocks homunculus |
| 1813 | Call Homunculus | COMPLETE | Create/resummon |
| 1814 | Rest | COMPLETE | Vaporize, HP>=80% |
| 1815 | Resurrect Homunculus | COMPLETE | Revive with HP% |

### Systems

| System | Status | Notes |
|--------|--------|-------|
| Cart Storage | COMPLETE | CartSubsystem, SCartWidget |
| Vending | COMPLETE | Full buyer/vendor UI |
| Item Identification | COMPLETE | SIdentifyPopup |
| Forging | COMPLETE | forge:request handler |
| Refining | COMPLETE | refine:request handler |
| Pharmacy/Brewing | COMPLETE | pharmacy:craft handler |
| Weapon/Armor Break | COMPLETE | weaponBroken/armorBroken flags |
| Chemical Protection | COMPLETE | preventBreak/preventStrip buffs |
| Homunculus | PARTIALLY COMPLETE | Combat tick, feeding, persistence done; evolution/skills deferred |
| Ground Loot | DEFERRED | Needed for Greed skill |

---

## 18. Gap Analysis

### 18.1 Known Remaining Gaps

| Gap | Priority | Effort | Blocked By |
|-----|----------|--------|------------|
| Greed skill (1210) | LOW | Medium | Ground loot system |
| Change Cart (607) | VERY LOW | Medium | Cart visual assets |
| Homunculus evolution | MEDIUM | Medium | Stone of Sage item, stat bonus logic |
| Homunculus skill casting | MEDIUM | Large | AI system for skill selection |
| Homunculus position broadcast | MEDIUM | Medium | Client-side actor |
| Whitesmith skills (trans) | DEFERRED | Large | Transcendent class system |
| Ranked Blacksmith system | LOW | Medium | Ranking/leaderboard system |
| Fame system (Alchemist) | LOW | Medium | Ranking/leaderboard system |
| Cart visual attachment | LOW | Medium | Client-side cart mesh |

### 18.2 Prerequisite Bugs (if not already fixed)

See Section 15.4 for the 10 known prerequisite bugs in the Blacksmith skill tree. All should be verified against the current codebase.

### 18.3 Formula Discrepancies Between Sources

| Skill/System | Discrepancy | Resolution |
|-------------|-------------|------------|
| Demonstration weapon break % | iRO Wiki: 1-5%, rAthena: 3-15% | Use rAthena values (3*SkillLevel) |
| Acid Terror ATK% | Alchemist Research shows 140-300%, Skills Complete shows 120-300% | Use 100+40*Lv = 140-300% (iRO Wiki Classic) |
| Power Thrust party ATK | rAthena pre-re: 5*Lv for both, iRO Wiki: tiered (5/5/10/10/15) | Use iRO Wiki tiered values for Classic |
| Potion Pitcher White Potion VIT bonus | Not in all sources | Confirmed in Hercules pre-re skill.c (VIT*2) |
| Adrenaline Rush party % | Some sources say 20%, rAthena says 25% | Use 20% (iRO Wiki Classic confirmed) |

### 18.4 Verification Checklist

- [ ] All Merchant skill cooldowns verified as 0 (except where noted)
- [ ] Cart Revolution targetType is 'single' with splash (not 'ground')
- [ ] Cart Revolution is Forced Neutral element
- [ ] Mammonite has no cooldown (ASPD-based)
- [ ] All 10 Blacksmith prerequisite bugs fixed
- [ ] Hammer Fall deals NO damage
- [ ] Weapon Perfection applies to party (no weapon restriction)
- [ ] Power Thrust weapon break 0.1% implemented
- [ ] Maximize Power SP drain interval correct per level
- [ ] Acid Terror ignores hard DEF but not VIT soft DEF
- [ ] Acid Terror consumes Acid Bottle at cast START
- [ ] Demonstration tick rate is 500ms
- [ ] CP blocks both break AND strip
- [ ] Potion Pitcher VIT*2 bonus for White Potion
- [ ] Potion Pitcher INT*2 bonus for Blue Potion
- [ ] Potion Pitcher 3x healing on Homunculus
- [ ] Forge success formula matches rAthena source
- [ ] Brew success formula matches rAthena source
- [ ] Refine success rates match pre-renewal table
- [ ] Resurrect Homunculus SP cost formula: 80 - 6*Lv (not 50 + 6*Lv)

---

*Document generated from deep web research across 14+ sources. Cross-referenced with rAthena pre-renewal source, iRO Wiki Classic, RateMyServer, Hercules pre-re, and existing project documentation.*
