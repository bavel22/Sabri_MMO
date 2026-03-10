# 05 -- Items, Equipment & Cards

> Ragnarok Online Classic (pre-Renewal) reference for the Sabri_MMO project.
> All data sourced from iRO Wiki Classic, RateMyServer Pre-RE DB, rAthena documentation, and community references.

---

## Table of Contents

1. [Item Categories](#1-item-categories)
2. [Equipment Slots](#2-equipment-slots)
3. [Weapon Types](#3-weapon-types)
4. [Upgrade / Refine System](#4-upgrade--refine-system)
5. [Card System](#5-card-system)
6. [Consumables](#6-consumables)
7. [Weight System](#7-weight-system)
8. [Inventory & Storage](#8-inventory--storage)
9. [Implementation Plan](#9-implementation-plan)

---

## 1. Item Categories

Every item in RO Classic belongs to exactly one category. The rAthena `item_db` type field defines these:

### 1.1 Usable Items (type = 0, 2, 11)

Items consumed on use. Subtypes:

| Subtype | Examples | Behavior |
|---------|----------|----------|
| **Healing** | Red Potion, White Potion, Yggdrasil Berry | Immediate HP/SP restore |
| **Delayed Consumable** | Elemental Converters, Resistance Potions | Applied after cast/delay |
| **Buff Potions** | Awakening Potion, Berserk Potion, Speed Potion | Temporary stat/speed buff |
| **Cure Items** | Green Potion, Panacea, Royal Jelly | Remove status effects |
| **Teleport Items** | Butterfly Wing, Fly Wing | Instant warp |
| **Gems/Catalysts** | Blue Gemstone, Yellow Gemstone, Red Gemstone | Consumed by skills |
| **Scrolls** | Wind Scroll, Fire Scroll | Cast a spell on use |
| **Stat Foods** | Fried Grasshopper Legs through Dragon Breath Cocktail | +1 to +10 stat for 20 min |
| **Box Items** | Old Blue Box, Old Purple Box, Old Card Album | Open for random loot |

### 1.2 Equipment (type = 4, 5)

Gear worn in specific slots. Two main groups:

- **type 4 (Armor)**: Headgear (upper/mid/lower), Body Armor, Shield, Garment, Footgear, Accessory
- **type 5 (Weapon)**: All weapon subtypes (Dagger, Sword, Bow, etc.)

Equipment has: ATK/DEF, stat bonuses, level/class restrictions, card slots (0-4), weight, and refine level (+0 to +10).

### 1.3 Miscellaneous (type = 3)

Non-usable, non-equippable items. Purpose is selling to NPCs or as crafting materials.

| Subtype | Examples | Use |
|---------|----------|-----|
| **Monster Loot** | Jellopy, Fluff, Shell, Feather, Skel Bone | Sell to NPC for zeny |
| **Crafting Materials** | Iron Ore, Iron, Steel, Star Crumb | Blacksmith forging, brewing |
| **Upgrade Ores** | Phracon, Emveretarcon, Oridecon, Elunium | Weapon/armor refining |
| **Elemental Stones** | Flame Heart, Mystic Frozen, Great Nature, Rough Wind | Weapon forging elements |
| **Quest Items** | Varies per quest | Non-tradeable, quest-specific |
| **Collectibles** | Dolls, Jewels, Treasure Boxes | Trophy items, NPC quests |

### 1.4 Ammunition (type = 10)

Consumed per attack by ranged weapon types. Each ammo type pairs with a specific weapon type.

| Ammo Type | Used By | Notable Variants |
|-----------|---------|-----------------|
| **Arrows** | Bows | Arrow (Neutral), Fire Arrow, Silver Arrow (Holy), Crystal Arrow (Water), Stone Arrow (Earth), Wind Arrow, Immaterial Arrow (Ghost), Shadow Arrow, Cursed Arrow, Rusty Arrow (Poison), Oridecon Arrow, Steel Arrow |
| **Bullets** | Revolvers, Rifles, Shotguns, Gatling Guns | Silver Bullet, Bloody Shell, Full Metal Jacket |
| **Spheres** | Grenade Launchers | Various elemental spheres |
| **Shuriken** | Ninja (with base weapon) | Shuriken, Nimbus Shuriken, Flash Shuriken |
| **Kunai** | Ninja | Kunai, Poisoning Kunai, etc. |
| **Venom Knife** | Assassin Cross (Venom Splasher) | Consumed by skill use |

Arrows are the most common ammo type. Arrow elements override the weapon element for damage calculation. Archers can craft arrows from loot using the **Arrow Crafting** skill.

### 1.5 Cards (type = 6)

Special items installed (compounded) into slotted equipment. Each card is named after a monster (e.g., "Poring Card") and provides a unique bonus. Cards are covered in full detail in [Section 5](#5-card-system).

### 1.6 Pet Items (type = 7, 8)

- **Pet Eggs (type 7)**: Captured monsters that can be hatched as pets
- **Pet Armor (type 8)**: Equipment for pets that grant passive bonuses

---

## 2. Equipment Slots

A character has **10 equipment positions**. Each piece of gear occupies exactly one position (except headgear combos).

### 2.1 Slot Layout

| Position | Slot Name | Card Type | Notes |
|----------|-----------|-----------|-------|
| Head (Upper) | `head_top` | Headgear | Hats, helms, crowns. Most common headgear slot. |
| Head (Middle) | `head_mid` | Headgear | Glasses, goggles, monocles. Some items cover Upper+Mid. |
| Head (Lower) | `head_low` | -- | Mouth items (Pierrot Nose, Gangster Mask). Rarely has card slot. |
| Body Armor | `armor` | Armor | Chest piece. Determines elemental property of character. |
| Weapon (Right Hand) | `weapon` | Weapon | Primary weapon. Defines attack type, element, range. |
| Shield (Left Hand) | `shield` | Shield | Off-hand defense. Cannot equip with two-handed weapons. |
| Garment | `garment` | Garment | Capes, manteaus, hoods. Elemental resist common here. |
| Footgear | `footgear` | Footgear | Shoes, boots, sandals. AGI/movement bonuses common. |
| Accessory 1 | `accessory_1` | Accessory | Rings, clips, rosaries, belts. |
| Accessory 2 | `accessory_2` | Accessory | Second accessory slot. Both can be same item. |

### 2.2 Headgear Combo Positions

Some headgear items occupy multiple head slots simultaneously:

| Combo | Example Items | Blocks |
|-------|---------------|--------|
| Upper only | Hat, Helm, Crown, Beret | Mid and Low remain free |
| Upper + Mid | Blinker, Welding Mask | Low remains free |
| Upper + Mid + Low | Wedding Veil (rare) | All head slots used |
| Mid only | Sunglasses, Glasses | Upper and Low remain free |
| Mid + Low | Gas Mask, Opera Phantom Mask | Upper remains free |
| Lower only | Gangster Mask, Pipe, Pierrot Nose | Upper and Mid remain free |

### 2.3 Two-Handed Weapon Rule

When a character equips a two-handed weapon, the shield slot is forcibly unequipped and locked. The following weapon types are always two-handed:

- Two-Handed Swords
- Two-Handed Spears
- Two-Handed Axes
- Bows
- Katars
- Instruments
- Whips
- Guns (all subtypes)
- Huuma Shuriken

One-handed weapons (Daggers, 1H Swords, 1H Spears, 1H Axes, Maces, Rods/Staves, Knuckles, Books) allow a shield in the off-hand.

### 2.4 Class Restrictions

Every equipment item has a `equip_jobs` bitmask defining which classes can wear it. Common patterns:

| Equipment Type | Typical Class Access |
|---------------|---------------------|
| Heavy Armor (high DEF) | Swordsman, Knight, Crusader, Merchant, Blacksmith |
| Robes / Light Armor | Mage, Wizard, Sage, Acolyte, Priest, Monk |
| Bows | Archer, Hunter, Bard, Dancer, Rogue |
| Two-Handed Swords | Swordsman, Knight, Crusader |
| Katars | Assassin, Assassin Cross |
| Knuckles | Monk, Champion, Priest |
| Books | Sage, Professor, Priest, High Priest |
| Instruments | Bard, Clown |
| Whips | Dancer, Gypsy |
| Guns (all) | Gunslinger |
| Huuma Shuriken | Ninja |

### 2.5 Level Restrictions

Equipment has a `required_level` (base level) that the character must meet. Weapon level also implies a minimum base level:

| Weapon Level | Dagger/Rod | 1H Sword/Mace/Book | Bow/Katar/Spear/2H Sword | Axe |
|-------------|------------|---------------------|--------------------------|-----|
| Level 1 | BaseLv 1 | BaseLv 2 | BaseLv 4 | BaseLv 3 |
| Level 2 | BaseLv 12 | BaseLv 14 | BaseLv 18 | BaseLv 16 |
| Level 3 | BaseLv 24 | BaseLv 27 | BaseLv 33 | BaseLv 30 |
| Level 4 | BaseLv 36 | BaseLv 40 | BaseLv 48 | BaseLv 44 |

---

## 3. Weapon Types

RO Classic has **16 main weapon types** plus support weapons (ammunition). Each type has unique size modifiers, base ASPD, and class restrictions.

### 3.1 Complete Weapon Type Table

| # | Weapon Type | Hands | Usable Classes | Notable Traits |
|---|------------|-------|----------------|----------------|
| 1 | **Dagger** | 1H | Novice, Swordsman, Thief, Mage, Merchant, Ninja, all 2nd classes of these | Fastest ASPD, 100% vs Small |
| 2 | **One-Handed Sword** | 1H | Swordsman, Knight, Crusader, Merchant, Blacksmith, Alchemist | Balanced all-round, 100% vs Medium |
| 3 | **Two-Handed Sword** | 2H | Swordsman, Knight, Crusader | High ATK, no shield, 100% vs Large |
| 4 | **One-Handed Spear** | 1H | Swordsman, Knight, Crusader | Can use shield, 100% vs Large |
| 5 | **Two-Handed Spear** | 2H | Swordsman, Knight, Crusader | Higher ATK spears, 100% vs Large, Peco bonus |
| 6 | **One-Handed Axe** | 1H | Swordsman, Merchant, Blacksmith, Alchemist, Knight | Heavy hitters, 100% vs Large |
| 7 | **Two-Handed Axe** | 2H | Swordsman, Merchant, Blacksmith, Alchemist, Knight | Highest raw ATK, 100% vs Large |
| 8 | **Mace** | 1H | Swordsman, Acolyte, Priest, Monk, Merchant, Blacksmith, Alchemist | 100% vs Medium AND Large |
| 9 | **Rod / Staff** | 1H | Mage, Wizard, Sage, Acolyte, Priest, Monk | MATK bonus, 100% vs all sizes |
| 10 | **Bow** | 2H | Archer, Hunter, Bard, Dancer, Rogue | Ranged, requires arrows, 100% Small+Medium |
| 11 | **Katar** | 2H (both hands) | Assassin, Assassin Cross | Doubles crit rate, 100% vs Medium |
| 12 | **Knuckle / Fist** | 1H | Monk, Champion, Priest | Fast ASPD, 100% vs Small |
| 13 | **Book** | 1H | Sage, Professor, Priest, High Priest | 100% vs Small and Medium |
| 14 | **Musical Instrument** | 2H | Bard, Clown | Ensemble skills, 100% vs Medium |
| 15 | **Whip** | 2H | Dancer, Gypsy | Ensemble skills, 100% vs Medium |
| 16 | **Gun** | 2H | Gunslinger | 5 subtypes (Revolver, Rifle, Shotgun, Gatling, Grenade Launcher), 100% all sizes |
| 17 | **Huuma Shuriken** | 2H | Ninja | Thrown weapon, 100% all sizes |

### 3.2 Size Modifier Table (Pre-Renewal)

Size modifiers are a percentage applied to the weapon's physical ATK component based on the target's size. This is one of the most important damage mechanics in RO.

| Weapon Type | vs Small | vs Medium | vs Large |
|-------------|----------|-----------|----------|
| Bare Fist | 100% | 100% | 100% |
| Dagger | **100%** | 75% | 50% |
| 1H Sword | 75% | **100%** | 75% |
| 2H Sword | 75% | 75% | **100%** |
| 1H Spear | 75% | 75% | **100%** |
| 2H Spear | 75% | 75% | **100%** |
| 1H Axe | 50% | 75% | **100%** |
| 2H Axe | 50% | 75% | **100%** |
| Mace | 75% | **100%** | **100%** |
| Rod / Staff | **100%** | **100%** | **100%** |
| Bow | **100%** | **100%** | 75% |
| Katar | 75% | **100%** | 75% |
| Knuckle / Fist | **100%** | 75% | 50% |
| Instrument | 75% | **100%** | 75% |
| Whip | 75% | **100%** | 50% |
| Book | **100%** | **100%** | 50% |
| Gun (all) | **100%** | **100%** | **100%** |
| Huuma Shuriken | **100%** | **100%** | **100%** |

The Drake Card (MVP card) nullifies size penalties entirely, making it extremely valuable for weapons with poor size coverage.

### 3.3 Weapon Levels (1-4)

Every weapon has a **weapon level** (1-4) that determines:
- Refinement ATK bonus per level
- Safe refinement limit
- Ore required for refining
- Base level requirement

| Property | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 |
|----------|-------------|-------------|-------------|-------------|
| ATK per refine | +2 | +3 | +5 | +7 |
| Over-refine bonus | +3 random | +5 random | +8 random | +13 random |
| Safe limit | +7 | +6 | +5 | +4 |
| Refine ore | Phracon (200z) | Emveretarcon (1,000z) | Oridecon (drop) | Oridecon (drop) |
| Typical ATK range | 10-50 | 30-90 | 60-140 | 100-200+ |
| Examples | Knife, Rod, Club | Cutter, Bow, Mace | Main Gauche, Falchion, Spear | Mysteltainn, Excalibur, Balmung |

### 3.4 ASPD (Attack Speed) by Weapon Type

ASPD (Attack Speed Per-second Delay) determines how fast a character attacks. It ranges from 0-190, where 190 is the hard cap (0.2s between attacks).

**ASPD Formula (Pre-Renewal):**
```
ASPD = 200 - (WD - ([WD * AGI / 25] + [WD * DEX / 100]) / 10) * (1 - SM)
```
Where:
- `WD` = Weapon Delay = `50 * BTBA` (Base Time Between Attacks in seconds)
- `SM` = Speed Modifier (sum of all ASPD% bonuses like Berserk Potion, Twohand Quicken, etc.)
- `[ ]` = Round to nearest integer
- Max ASPD = 190 (0.2s between attacks)

**Hits per second** = `50 / (200 - ASPD)`

Relative ASPD by weapon type (approximate, varies by class):

| Weapon Type | Relative Speed | Notes |
|-------------|---------------|-------|
| Dagger | Fastest | ~0.50-0.65s base swing |
| Knuckle / Fist | Very Fast | ~0.50-0.60s |
| Katar | Fast | ~0.55-0.65s, doubled crit |
| 1H Sword | Medium-Fast | ~0.55-0.70s |
| Mace | Medium | ~0.60-0.70s |
| 1H Axe | Medium-Slow | ~0.60-0.75s |
| 2H Sword | Medium-Slow | ~0.60-0.75s |
| Rod / Staff | Slow | ~0.65-0.80s |
| Bow | Slow | ~0.70-0.85s |
| Spear | Slow | ~0.65-0.80s |
| 2H Axe | Very Slow | ~0.70-0.85s |
| Book | Medium | ~0.60-0.70s |
| Instrument | Slow | ~0.65-0.80s |
| Whip | Slow | ~0.65-0.80s |

### 3.5 Notable Weapons by Type

#### Daggers
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Knife | 17 | 1 | 0 | Starter weapon |
| Cutter | 30 | 1 | 0 | Novice upgrade |
| Main Gauche | 43 | 1 | 3 | 3 card slots -- budget endgame |
| Stiletto | 47 | 2 | 0 | DEX+1 |
| Gladius | 60 | 3 | 0 | -- |
| Ice Falchion | 60 | 3 | 0 | Water property, auto-cast Cold Bolt Lv3 |
| Bazerald | 70 | 4 | 0 | INT+5, MATK+10 |
| Assassin Dagger | 140 | 4 | 0 | Assassin only, ASPD+2 |

#### One-Handed Swords
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Sword | 25 | 1 | 0 | Basic sword |
| Falchion | 49 | 2 | 0 | -- |
| Blade | 53 | 1 | 3 | 3 slots for cards |
| Katana | 60 | 2 | 0 | -- |
| Mysteltainn | 170 | 4 | 0 | Shadow property, chance to curse, DEF+3 |
| Excalibur | 150 | 4 | 0 | Holy property, INT+5, LUK+3 |

#### Two-Handed Swords
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Slayer | 60 | 2 | 2 | -- |
| Bastard Sword | 115 | 2 | 0 | -- |
| Claymore | 180 | 3 | 0 | -- |
| Muramasa | 155 | 4 | 0 | Crit+30, ASPD+8, curse 1% chance |
| Masamune | 200 | 4 | 0 | Flee+30, Perfect Dodge+2, ASPD buff |
| Balmung | 200 | 4 | 0 | INT+20, LUK+20, Indestructible |

#### Spears
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Javelin | 28 | 1 | 0 | Starter spear |
| Spear | 44 | 2 | 0 | -- |
| Pike | 60 | 1 | 3 | 3 card slots |
| Lance | 185 | 3 | 0 | 2H Spear |
| Brocca | 120 | 4 | 0 | 1H, DEF-ignoring |
| Gungnir | 120 | 4 | 0 | Perfect accuracy |

#### Axes
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Axe | 38 | 1 | 0 | Basic 1H axe |
| Battle Axe | 80 | 2 | 0 | 2H |
| Two-Handed Axe | 185 | 3 | 0 | 2H |
| Bloody Axe | 170 | 4 | 0 | STR+10, best 1H axe |

#### Maces
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Club | 23 | 1 | 0 | Starter mace |
| Mace | 37 | 1 | 3 | 3 card slots |
| Smasher | 40 | 1 | 3 | DEX+2, 3 slots |
| Chain | 84 | 2 | 0 | -- |
| Stunner | 140 | 3 | 0 | 5% stun chance |
| Grand Cross | 120 | 4 | 0 | STR+10, INT+4, LUK+10 |

#### Rods / Staves
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Rod | 15 | 1 | 0 | MATK+45, starter staff |
| Wand | 25 | 2 | 0 | MATK+45 |
| Staff | 40 | 2 | 0 | MATK+70 |
| Arc Wand | 60 | 3 | 0 | MATK+95, INT+3 |
| Survivor's Rod | 50 | 3 | 0 | DEX+3, MATK+86 |
| Staff of Piercing | 80 | 4 | 0 | INT+4, MATK+120, 10% MDEF pierce |

#### Bows
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Bow | 15 | 1 | 0 | Basic bow |
| Composite Bow | 29 | 1 | 3 | 3 card slots |
| Cross Bow | 65 | 2 | 0 | DEX+2 |
| Arbalest | 90 | 3 | 0 | DEX+2 |
| Hunter Bow | 125 | 3 | 0 | DEX+5, Hunter only |
| Rudra's Bow | 150 | 4 | 0 | INT+5, Heal Lv1, Cure Lv1 |

#### Katars
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Katar | 148 | 3 | 0 | Basic katar |
| Jamadhar | 165 | 4 | 0 | High ATK |
| Katar of Quaking | 105 | 3 | 3 | 3 card slots, Earth property |
| Sharpened Legbone of Ghoul | 150 | 4 | 0 | Drain HP on hit |

#### Knuckles / Fists
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Waghnak | 30 | 1 | 3 | 3 card slots |
| Knuckle Duster | 50 | 2 | 0 | -- |
| Fist | 115 | 3 | 0 | -- |
| Claw | 86 | 2 | 2 | 2 card slots |
| Finger | 97 | 3 | 0 | Int+1 |

#### Books
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Book | 85 | 2 | 0 | Basic book |
| Bible | 115 | 3 | 0 | Holy property, LUK+2 |
| Tablet | 125 | 3 | 0 | DEX+3, INT+3 |

#### Instruments
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Violin | 50 | 1 | 0 | Starter instrument |
| Guitar of Vast Land | 50 | 3 | 0 | Earth property |
| Mandolin | 50 | 2 | 0 | DEX+3 |

#### Whips
| Weapon | ATK | Lv | Slots | Special |
|--------|-----|----|----|---------|
| Rope | 45 | 1 | 0 | Starter whip |
| Whip of Earth | 55 | 3 | 0 | Earth property |
| Lariat | 45 | 2 | 0 | DEX+3 |

#### Guns
| Weapon | ATK | Lv | Subtype | Special |
|--------|-----|----|----|---------|
| Six Shooter | 30 | 1 | Revolver | Starter gun |
| Western Outlaw | 100 | 3 | Revolver | HIT+10 |
| Garrison | 70 | 2 | Rifle | Long range |
| Gate Keeper | 80 | 3 | Shotgun | Spread shot |
| Butcher | 120 | 3 | Gatling Gun | ASPD bonus |
| Destroyer | 340 | 4 | Grenade Launcher | AoE damage |

---

## 4. Upgrade / Refine System

The refinement system is how players enhance equipment ATK/DEF beyond base values. It is a core zeny sink and risk/reward mechanic.

### 4.1 How Refining Works

1. Find a **Blacksmith NPC** (Hollgrehenn in Prontera, or similar in other towns)
2. Select the item to refine
3. NPC consumes one **ore** + a **zeny fee**
4. Success: item gains +1 refine level (visible as "+X" prefix, e.g., "+7 Blade [3]")
5. Failure: **item is permanently destroyed** (including all compounded cards)

### 4.2 Required Ores and Costs

| Equipment | Ore | Ore Source | Zeny Fee |
|-----------|-----|-----------|----------|
| Weapon Lv 1 | Phracon | NPC (200z each) | 50z |
| Weapon Lv 2 | Emveretarcon | NPC (1,000z each) | 200z |
| Weapon Lv 3 | Oridecon | Monster drop / Oridecon Stone x5 | 5,000z |
| Weapon Lv 4 | Oridecon | Monster drop | 20,000z |
| All Armor | Elunium | Monster drop / Elunium Stone x5 | 2,000z |

### 4.3 Safety Limits

Refining up to the safety limit has **100% success rate**. Above the safety limit, each attempt risks destruction.

| Equipment Type | Safe Limit |
|---------------|------------|
| Weapon Lv 1 | **+7** |
| Weapon Lv 2 | **+6** |
| Weapon Lv 3 | **+5** |
| Weapon Lv 4 | **+4** |
| All Armor | **+4** |

### 4.4 Success Rates (Classic/Subscription Servers)

Refining from +0 to the safe limit is always 100%. Above safe limit:

| Refine Level | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 | Armor |
|-------------|-------------|-------------|-------------|-------------|-------|
| +1 to Safe | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 60% | 60% | 60% |
| +6 | 100% | 60% | 50% | 40% | 40% |
| +7 | 100% | 40% | 20% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 20% | 20% | 20% | 10% | 10% |

**Maximum refine level: +10** for all equipment.

### 4.5 ATK/DEF Bonus Per Refine

#### Weapons: ATK Bonus

| Refine Level | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 |
|-------------|-------------|-------------|-------------|-------------|
| +1 through safe | +2 ATK each | +3 ATK each | +5 ATK each | +7 ATK each |
| Above safe (over-refine) | +2 ATK + random(0 to 3*N) | +3 ATK + random(0 to 5*N) | +5 ATK + random(0 to 8*N) | +7 ATK + random(0 to 13*N) |

Where `N` = number of refine levels above the safe limit.

**Example**: A +10 Weapon Lv 4 (safe = +4) has:
- Base refine ATK: 10 * 7 = +70 ATK (flat)
- Over-refine bonus: random 0 to 13*6 = random 0-78 ATK per hit

This random bonus makes over-refined Lv 4 weapons extremely powerful but wildly inconsistent.

#### Armor: DEF Bonus

Each +1 refine adds approximately **+0.67 hard DEF** (displayed as integer, rounds down). This stacks with the armor's base DEF.

A +10 armor piece gains roughly +7 DEF on top of its base DEF.

### 4.6 Failure Consequences

- The item is **permanently destroyed**
- All compounded cards are **lost**
- The ore is consumed
- The zeny fee is consumed
- There is **no way to recover** the item

This makes refining above safe limit a major gambling mechanic. Players commonly have multiple copies of important items before attempting over-refining.

---

## 5. Card System

The card system is RO's primary equipment customization mechanic. Cards are inserted into slotted equipment to grant permanent bonuses.

### 5.1 Card Basics

- Cards are dropped by monsters at very low rates (typically 0.01% to 0.02%)
- Each card is named "[Monster Name] Card" (e.g., "Hydra Card", "Poring Card")
- Cards belong to a specific **equipment category** (weapon, armor, shield, garment, footgear, headgear, or accessory)
- Cards can only be compounded into their matching equipment type
- **Compounding is permanent** -- once inserted, a card cannot be removed
- If the equipment is destroyed (e.g., refine failure), the card is lost

### 5.2 Card Slots

Equipment can have 0 to 4 card slots. Slots are shown in brackets: `Guard [1]`, `Blade [3]`, `Main Gauche [3]`.

| Equipment Type | Max Slots | Typical Range |
|---------------|-----------|---------------|
| Weapons | 4 | 0-4 (most common: 0-3) |
| Body Armor | 1 | 0-1 |
| Shield | 1 | 0-1 |
| Garment | 1 | 0-1 |
| Footgear | 1 | 0-1 |
| Headgear | 1 | 0-1 |
| Accessories | 1 | 0-1 |

More slots generally means lower base stats. A Blade [3] has lower ATK than a Katana [0], but three card slots make it far more versatile.

**Socket Enchant** (NPC service) can add 1 slot to certain equipment that originally has 0 slots, at a cost of zeny + materials.

### 5.3 Compounding Process

1. Double-click the card in inventory
2. A list of compatible slotted equipment appears
3. Select the target equipment
4. Confirmation dialog warns this is permanent
5. Card is consumed from inventory and embedded in the equipment
6. Equipment name gains a prefix or suffix from the card

### 5.4 Prefix / Suffix System

When a card is compounded, the equipment's display name changes to include the card's prefix or suffix. Each card has a predetermined prefix/suffix that never changes.

- **Prefixes** appear before the item name: "Bloody [card prefix] Guard [1]"
- **Suffixes** start with "of": "Guard of [card suffix] [1]"
- Multiple cards = multiple prefixes/suffixes (weapons with 2+ slots)

Examples:
| Card | Prefix/Suffix | Equipment Example |
|------|--------------|-------------------|
| Hydra Card | "Bloody" (prefix) | "Bloody Blade [3]" |
| Andre Card | "Mighty" (prefix) | "Mighty Blade [3]" |
| Thara Frog Card | "Cranial" (prefix) | "Cranial Guard [1]" |
| Raydric Card | "Immune" (suffix) | "Muffler of Immune [1]" |
| Pasana Card | "of Ifrit" (suffix) | "Saint's Robe of Ifrit [1]" |

### 5.5 Card Categories by Equipment Type

#### Weapon Cards

**Race Damage Cards (+20% damage vs specific race):**

| Card | Race | Prefix |
|------|------|--------|
| Hydra Card | Demi-Human | Bloody |
| Goblin Card | Brute | Clamorous |
| Caramel Card | Insect | Insecticide |
| Strouf Card | Demon | Decussate |
| Flora Card | Fish | Fisher |
| Scorpion Card | Plant | Chemical |
| Earth Petite Card | Dragon | Dragoon |
| Orc Skeleton Card | Undead | Damned |
| Santa Poring Card | Angel | -- |

**Size Damage Cards (+15% damage + 5 ATK vs specific size):**

| Card | Size | Prefix |
|------|------|--------|
| Desert Wolf Card | Small | Gigantic |
| Skeleton Worker Card | Medium | Boned |
| Minorous Card | Large | Titan |

**Element Damage Cards (+20% damage vs specific element):**

| Card | Element | Prefix |
|------|---------|--------|
| Vadon Card | Fire | Flammable |
| Drainliar Card | Water | Saharic |
| Mandragora Card | Wind | Windy |
| Kaho Card | Earth | Underneath |
| Orc Skeleton Card | Holy | Damned |
| Santa Poring Card | Shadow | Hallowed |
| Anacondaq Card | Poison | Envenom |
| Mao Guai Card | Ghost | of Exorcism |

**Status Effect Weapon Cards:**

| Card | Effect | Chance | Prefix |
|------|--------|--------|--------|
| Marina Card | Freeze | 5% | Ice |
| Magnolia Card | Curse | 5% | Cursing |
| Zenorc Card | Poison | 4% | Venomer's |
| Skeleton Card | Stun | 2% | Keen |
| Metaller Card | Silence | 5% | Silence |
| Familiar Card | Blind | 5% | Blink |
| Savage Babe Card | Stun | 5% | -- |

**Flat ATK Weapon Cards:**

| Card | Effect | Prefix |
|------|--------|--------|
| Andre Card | ATK +20 | Mighty |
| Droppable Card | ATK +10 | -- |

#### Armor Cards

**Elemental Property Cards** (change armor element, affecting damage taken):

| Card | Element | Suffix |
|------|---------|--------|
| Pasana Card | Fire 1 | of Ifrit |
| Swordfish Card | Water 1 | Aqua |
| Dokebi Card | Wind 1 | of Zephyrus |
| Sandman Card | Earth 1 | of Gnome |
| Argiope Card | Poison 1 | -- |
| Bathory Card | Shadow 1 | Evil |
| Evil Druid Card | Undead 1 | -- |
| Angeling Card | Holy 1 | Holy |
| Ghostring Card | Ghost 1 | -- |

Elemental armor is crucial for defense. For example, wearing Fire armor (Pasana Card) reduces Fire damage by 25% and makes you immune to Freeze.

**Stat/HP Armor Cards:**

| Card | Effect |
|------|--------|
| Pupa Card | Max HP +700 |
| Pecopeco Card | Max HP +10% |
| Roda Frog Card | Max HP +400, Max SP +50 |
| Poring Card | LUK +2, Perfect Dodge +1 |
| Fabre Card | VIT +1, Max HP +100 |

#### Shield Cards

**Racial Damage Reduction Cards (30% reduction from specific race):**

| Card | Race | Prefix/Suffix |
|------|------|---------------|
| Thara Frog Card | Demi-Human | Cranial |
| Orc Warrior Card | Brute | Brutal |
| Bigfoot Card | Insect | of Gargantua |
| Penomena Card | Formless | Fire-Proof |
| Khalitzburg Card | Demon | from Hell |
| Teddy Bear Card | Undead | of Requiem |
| Rafflesia Card | Fish | Homer's |
| Sky Petite Card | Dragon | of Dragoon |
| Anubis Card | Angel | Satanic |

**Size Damage Reduction Cards (25% reduction from specific size):**

| Card | Size | Suffix |
|------|------|--------|
| Mysteltainn Card | Small | Anti-Small |
| Ogretooth Card | Medium | Anti-Medium |
| Executioner Card | Large | Anti-Large |

#### Garment Cards

| Card | Effect | Suffix |
|------|--------|--------|
| Raydric Card | Neutral damage -20% | Immune |
| Whisper Card | Flee +20, Ghost damage +50% | -- |
| Deviling Card | Neutral damage -50%, other elements +50% | Deviant |
| Jakk Card | Fire damage -30% | Flameguard |
| Marse Card | Water damage -30% | Genie's |
| Dustiness Card | Wind damage -30% | -- |
| Hode Card | Earth damage -30% | -- |
| Nine Tail Card | AGI+2, Flee+20 (at +9 refine) | De Luxe |

#### Footgear Cards

| Card | Effect | Suffix |
|------|--------|--------|
| Verit Card | Max HP +8%, Max SP +8% | -- |
| Matyr Card | AGI +1, Max HP +10% | -- |
| Sohee Card | Max SP +15%, SP Recovery +3% | -- |
| Green Ferus Card | VIT +1, Max HP +10% | -- |
| Firelock Soldier Card | STR +2, Max HP/SP +10% (at +9 refine) | Superior |

#### Headgear Cards

| Card | Effect |
|------|--------|
| Willow Card | Max SP +80 |
| Elder Willow Card | INT +2 |
| Marduk Card | Silence immunity |
| Orc Hero Card | Stun immunity |
| Nightmare Card | Sleep immunity, +1 AGI |
| Deviruchi Card | STR +1, immunity to Blind |
| Stalactic Golem Card | DEF +3, Stun resist +20% |

#### Accessory Cards

| Card | Effect |
|------|--------|
| Zerom Card | DEX +3 |
| Kukre Card | AGI +2 |
| Mantis Card | STR +3 |
| Vitata Card | VIT +3 |
| Smokie Card | Hiding Lv 1 on use |
| Creamy Card | Teleport Lv 1 on use |
| Poporing Card | Detoxify Lv 1 on use |
| Spore Card | VIT +2 |

### 5.6 MVP Cards

MVP cards are the rarest and most powerful cards in the game, dropped only by boss monsters at extremely low rates (0.01% or less). They have game-changing effects.

| Card | Equipment | Effect | Nickname |
|------|-----------|--------|----------|
| **Baphomet Card** | Weapon | Splash damage (3x3 cells) on every attack | "Bapho" |
| **Osiris Card** | Accessory | Full HP/SP restore on resurrection | -- |
| **Pharaoh Card** | Headgear | SP consumption -30% | "Pharaoh" |
| **Orc Lord Card** | Armor | Reflect 30% physical melee damage | "OL" |
| **Tao Gunka Card** | Armor | Max HP +100%, DEF -50, MDEF -50 | "TG" (armor) |
| **Mistress Card** | Headgear | No gemstone requirement, SP cost +25% | -- |
| **Drake Card** | Weapon | Nullify size penalty (100% damage vs all sizes) | "Drake" |
| **Maya Card** | Shield | Reflect single-target magic | "Maya" |
| **GTB (Golden Thief Bug) Card** | Shield | Immune to all magic (including heals!) | "GTB" |
| **Orc Hero Card** | Headgear | Immunity to Stun, VIT +3 | "OH" |
| **Eddga Card** | Footgear | Endure effect (no flinching), Max HP -25% | -- |
| **Moonlight Flower Card** | Footgear | No SP cost for movement skills | -- |
| **Lord of the Dead Card** | Weapon | 1% chance Coma (reduce HP to 1), +50% vs Undead | "LoD" |
| **Turtle General Card** | Weapon | ATK +20%, 5% auto-Magnum Break | "TGK" |
| **Sniper Card (Cecil Damon)** | Weapon | 5% auto-Blitz Beat when attacking | -- |

MVP cards fundamentally alter gameplay. A Baphomet carded weapon turns any melee class into an AoE farmer. GTB makes the wearer immune to all magic but also unable to receive heals.

---

## 6. Consumables

### 6.1 HP Potions

| Item | HP Restored | Weight | NPC Buy | Notes |
|------|-------------|--------|---------|-------|
| Red Potion | 45-65 | 7 | 50z | Starter potion |
| Orange Potion | 105-145 | 10 | 200z | Early game upgrade |
| Yellow Potion | 175-235 | 13 | 550z | Mid-game staple |
| White Potion | 325-405 | 15 | 1,200z | Endgame standard |
| Condensed Red Potion | ~70-110 | 2 | Alchemist-crafted | Light weight |
| Condensed Yellow Potion | ~225-325 | 4 | Alchemist-crafted | Light weight, popular |
| Condensed White Potion | ~425-550 | 3 | Alchemist-crafted | "Slims" -- WoE/MVP staple |
| Mastela Fruit | 400-600 | 3 | 8,500z | Best HP/weight ratio |
| Yggdrasil Berry | Full HP+SP | 30 | Monster drop only | Rarest healing item |
| Yggdrasil Seed | 50% HP+SP | 30 | Monster drop only | Half-Ygg, still rare |
| Fresh Fish | 100-150 | 2 | 250z | Light snack |
| Meat | 70 | 15 | 25z | Monster drop |

**Potion healing formula**: `Actual_Heal = base_heal * (1 + 0.02 * VIT)` -- higher VIT increases potion effectiveness.

### 6.2 SP Potions

| Item | SP Restored | Weight | NPC Buy | Notes |
|------|-------------|--------|---------|-------|
| Blue Potion | 40-60 SP | 15 | Not sold (dropped) | Only direct SP potion |
| Blue Herb | 15-30 SP | 7 | Monster drop | Raw herb |
| Grape | 25 SP | 2 | 100z (from players) | Common SP item |
| Strawberry | 16 SP | 2 | Monster drop | -- |
| Honey | 70 HP + 20 SP | 10 | 100z | Dual recovery |
| Royal Jelly | 100 HP + 40 SP | 15 | Quest/Drop | Also cures status |

### 6.3 Cure / Status Items

| Item | Effect | Weight | Source |
|------|--------|--------|--------|
| Green Potion | Cure Poison, Silence, Blind, Confusion | 7 | Alchemist-crafted |
| Panacea | Cure all negative status effects | 10 | NPC / Drop |
| Royal Jelly | Cure all status + heal HP/SP | 15 | Drop / Quest |
| Holy Water | Cure Cursed status, used in some skills | 3 | NPC / Acolyte skill |
| Karvodailnirol | Cure all status effects | 7 | Monster drop |

### 6.4 ASPD / Speed Potions

| Item | Effect | Duration | Weight | Usable By |
|------|--------|----------|--------|-----------|
| Concentration Potion | ASPD +10% | 30 min | 10 | All classes |
| Awakening Potion | ASPD +15% | 30 min | 15 | 2nd class+ |
| Berserk Potion | ASPD +20% | 30 min | 20 | 2nd class+ (no Mage/Acolyte line) |
| Speed Potion | Move speed +100% | 5 sec | 10 | All classes |

ASPD potions do NOT stack with each other -- only the strongest applies. They DO stack with skill-based ASPD buffs (like Twohand Quicken, Adrenaline Rush).

### 6.5 Stat Food

Crafted via the Cooking system. 60 foods total: 10 levels per stat (6 stats).

| Stat | +1 Food | +5 Food | +10 Food | Duration |
|------|---------|---------|----------|----------|
| STR | Fried Grasshopper Legs | Bomber Steak | Steamed Tongue | 20 min |
| AGI | Frog Egg Squid Ink Soup | Herb-Roasted Trout | Steamed Desert Scorpions | 20 min |
| VIT | Steamed Crab Nippers | Cooked Vegetables | Immortal Stew | 20 min |
| INT | Grape Juice Herbal Tea | Fruit Mix | Dragon Breath Cocktail | 20 min |
| DEX | Honey Grape Juice | Fish Stew | Hwergelmir's Tonic | 20 min |
| LUK | Fried Monkey Tails | Lucky Soup | Cooked Nine Tail's Tails | 20 min |

- All stat foods also restore some HP/SP on use
- Effects stack with buff skills (Blessing, Increase AGI, etc.)
- Only one food per stat at a time -- eating a new one replaces the old
- Recipes for +6 to +10 foods require cookbooks dropped by monsters (not NPC-purchasable)

### 6.6 Elemental Resist Potions

| Item | Effect | Duration | Weight |
|------|--------|----------|--------|
| Fireproof Potion | Fire resist +20%, Water resist -15% | 10 min | 10 |
| Coldproof Potion | Water resist +20%, Wind resist -15% | 10 min | 10 |
| Thunderproof Potion | Wind resist +20%, Earth resist -15% | 10 min | 10 |
| Earthproof Potion | Earth resist +20%, Fire resist -15% | 10 min | 10 |

These are crafted by Alchemists using the Elemental Potion Creation Manual.

### 6.7 Elemental Converters

| Item | Effect | Duration | Weight |
|------|--------|----------|--------|
| Flame Elemental Converter | Weapon becomes Fire element | 20 min | 1 |
| Frost Elemental Converter | Weapon becomes Water element | 20 min | 1 |
| Seismic Elemental Converter | Weapon becomes Earth element | 20 min | 1 |
| Lightning Elemental Converter | Weapon becomes Wind element | 20 min | 1 |

Crafted by Sages using Create Elemental Converter skill. Overwrites existing weapon element.

### 6.8 Other Notable Consumables

| Item | Effect | Weight | Notes |
|------|--------|--------|-------|
| Butterfly Wing | Teleport to save point | 5 | NPC buy 300z |
| Fly Wing | Random teleport on current map | 5 | NPC buy 60z |
| Old Blue Box | Random item (any non-MVP) | 20 | Rare drop |
| Old Purple Box | Random equipment | 20 | Rare drop |
| Old Card Album | Random non-MVP card | 5 | Very rare drop |
| Dead Branch | Summon random monster | 1 | Quest/Drop |
| Bloody Branch | Summon random MVP | 1 | Very rare |
| Token of Siegfried | Resurrect at full HP/SP on death | 1 | Event/Quest |
| Aloevera | Self-cast Provoke Lv1 (+ATK, -DEF) | 2 | NPC buy |
| Box of Sunlight | INT+5 for 10 min | 10 | Cash shop |

---

## 7. Weight System

### 7.1 Weight Limit Formula

```
Max Weight = 2000 + (Base STR * 30) + Job Bonus + Skill Bonus + Mount Bonus
```

| Component | Value |
|-----------|-------|
| Base | 2,000 for all classes |
| STR bonus | +30 per point of base STR |
| Job class bonus | Varies (see table below) |
| Enlarge Weight Limit Lv10 | +2,000 (Merchant skill) |
| Enlarge Weight Limit II Lv10 | +2,000 (Blacksmith/Alchemist) |
| Peco Peco riding | +1,000 |

### 7.2 Job Class Weight Bonuses

| Job Bonus | Classes |
|-----------|---------|
| +0 | Novice, Super Novice |
| +200 | Mage, Wizard, Sage |
| +400 | Archer, Thief, Assassin, Acolyte |
| +600 | Hunter, Bard, Dancer, Rogue, Priest, Monk |
| +800 | Swordsman, Knight, Crusader, Merchant, Gunslinger, Taekwon |
| +1,000 | Blacksmith, Alchemist |

### 7.3 Weight Thresholds

| Threshold | Effect |
|-----------|--------|
| **< 50% capacity** | Normal -- full HP/SP regen, all skills available |
| **50% to 89%** | **Minor Overweight** -- Natural HP/SP regeneration STOPS. HP Recovery and SP Recovery skills stop working. Item-creation skills disabled (Arrow Crafting, etc.). Combat and other skills still work. |
| **90% to 100%** | **Major Overweight** -- ALL of the above PLUS: Cannot attack. Cannot use ANY skills. Essentially immobile except walking. |
| **> 100%** | Cannot pick up any more items (prevented by weight check) |

### 7.4 Common Item Weights

| Item | Weight | Notes |
|------|--------|-------|
| Red Potion | 7 | Very light |
| White Potion | 15 | Standard weight |
| Condensed White Potion | 3 | Why "slims" are preferred |
| Mastela Fruit | 3 | Best HP/weight healing |
| Yggdrasil Berry | 30 | Very heavy for 1 item |
| Blue Potion | 15 | Only SP potion, heavy |
| Butterfly Wing | 5 | Light |
| Fly Wing | 5 | Light |
| Jellopy | 1 | Lightest loot |
| Iron Ore | 15 | Heavy material |
| Elunium | 10 | Upgrade material |
| Oridecon | 20 | Upgrade material |
| Card (any) | 1 | All cards weigh 1 |
| Arrow (any) | 0.1 (per unit) | Nearly weightless |

---

## 8. Inventory & Storage

### 8.1 Player Inventory

| Property | Value |
|----------|-------|
| Slot limit | **Unlimited slots** (weight-limited, not slot-limited) |
| Weight limit | Varies by class/STR (see Section 7) |
| Stacking | Identical usable/etc items stack. Equipment does NOT stack. |
| Max stack | Varies per item (typically 99 for consumables, 999 for loot) |

In Classic RO, inventory is purely weight-limited with no hard slot cap. You can carry 500 different items as long as total weight is under your limit.

> **Sabri_MMO Implementation Note**: Our project uses a hybrid system with both weight AND slot limits for gameplay balance. See Section 9.

### 8.2 Kafra Storage (Personal)

| Property | Value |
|----------|-------|
| Slot limit | **300 slots** (pre-Renewal) |
| Weight limit | None (storage has no weight cap) |
| Access | Via Kafra NPCs in towns |
| Cost | 40-80z per access (varies by town) |
| Account-shared | Yes -- all characters on the same account share storage |
| Stacking | Same as inventory |

### 8.3 Guild Storage

| Property | Value |
|----------|-------|
| Base slots | **100 slots** |
| Max slots | **500 slots** (with Guild Storage Extension skill at max) |
| Access | Guild members only, via Kafra NPC |
| Restrictions | Only one person can access at a time |

### 8.4 Cart / Pushcart (Merchant Classes)

| Property | Value |
|----------|-------|
| Slot limit | **100 slots** |
| Weight limit | **8,000** |
| Access | Merchant, Blacksmith, Alchemist + transcendent versions |
| Activation | Pushcart skill (Merchant Lv5) |
| Side effect | Reduces movement speed by 40% (can be offset by Cart skills) |

### 8.5 Stacking Rules

| Item Type | Stackable? | Max Stack |
|-----------|-----------|-----------|
| Consumables (potions, food, herbs) | Yes | 99-999 (varies) |
| Misc/Etc (loot, materials, ores) | Yes | 999-30,000 |
| Ammunition (arrows, bullets) | Yes | 999 (per type) |
| Cards | No | 1 each (unique per slot) |
| Equipment (weapons, armor) | No | 1 each (even identical items) |

Equipment never stacks because each piece has individual refine level, card composition, and potential enchantments.

---

## 9. Implementation Plan

This section maps the RO Classic systems described above to the Sabri_MMO project's existing architecture.

### 9.1 Current State (Already Implemented)

Based on `database/init.sql` and existing migrations:

**Items table** (`items`):
- Has: item_id, name, description, item_type, equip_slot, weight, price, atk, def, matk, mdef, stat bonuses (str/agi/vit/int/dex/luk), max_hp_bonus, max_sp_bonus, required_level, stackable, max_stack, icon
- Extended with: weapon_type, aspd_modifier, weapon_range (added for RO weapons)
- 126+ items seeded (28 consumables, 50 loot items, 14 weapons, 11 armor pieces, 23 cards)

**Character inventory** (`character_inventory`):
- Has: inventory_id, character_id, item_id, quantity, is_equipped, slot_index, equipped_position
- `equipped_position` column added for dual-accessory support

**Character hotbar** (`character_hotbar`):
- Has: character_id, slot_index, inventory_id, item_id, item_name

### 9.2 Schema Additions Needed

#### 9.2.1 Items Table Enhancements

```sql
-- New columns for items table
ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_level INTEGER DEFAULT 0;      -- 1-4 for weapons
ALTER TABLE items ADD COLUMN IF NOT EXISTS card_slots INTEGER DEFAULT 0;         -- 0-4 card slots
ALTER TABLE items ADD COLUMN IF NOT EXISTS element VARCHAR(10) DEFAULT 'neutral'; -- weapon/armor element
ALTER TABLE items ADD COLUMN IF NOT EXISTS equip_jobs BIGINT DEFAULT -1;         -- bitmask: which classes can equip (-1 = all)
ALTER TABLE items ADD COLUMN IF NOT EXISTS two_handed BOOLEAN DEFAULT false;      -- blocks shield slot
ALTER TABLE items ADD COLUMN IF NOT EXISTS ammo_type VARCHAR(20) DEFAULT NULL;   -- arrow, bullet, sphere, shuriken, kunai
ALTER TABLE items ADD COLUMN IF NOT EXISTS card_type VARCHAR(20) DEFAULT NULL;   -- weapon, armor, shield, garment, footgear, headgear, accessory
ALTER TABLE items ADD COLUMN IF NOT EXISTS card_prefix VARCHAR(50) DEFAULT NULL; -- prefix when compounded
ALTER TABLE items ADD COLUMN IF NOT EXISTS card_suffix VARCHAR(50) DEFAULT NULL; -- suffix when compounded
ALTER TABLE items ADD COLUMN IF NOT EXISTS hit_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS flee_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS crit_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS aspd_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS hp_regen_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS sp_regen_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS perfect_dodge_bonus INTEGER DEFAULT 0;

-- Card effect columns (for race/element/size damage modifiers)
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_race VARCHAR(20) DEFAULT NULL;     -- demi_human, brute, insect, etc.
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_race_pct INTEGER DEFAULT 0;        -- +20 for Hydra
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_element VARCHAR(20) DEFAULT NULL;  -- fire, water, wind, etc.
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_element_pct INTEGER DEFAULT 0;     -- +20 for Vadon
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_size VARCHAR(10) DEFAULT NULL;     -- small, medium, large
ALTER TABLE items ADD COLUMN IF NOT EXISTS damage_size_pct INTEGER DEFAULT 0;        -- +15 for Minorous
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_race VARCHAR(20) DEFAULT NULL;     -- for shield cards
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_race_pct INTEGER DEFAULT 0;        -- -30 for Thara Frog
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_element VARCHAR(20) DEFAULT NULL;  -- for garment cards
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_element_pct INTEGER DEFAULT 0;     -- -20 for Raydric (neutral)
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_size VARCHAR(10) DEFAULT NULL;     -- for shield cards
ALTER TABLE items ADD COLUMN IF NOT EXISTS resist_size_pct INTEGER DEFAULT 0;        -- -25 for Executioner
ALTER TABLE items ADD COLUMN IF NOT EXISTS set_armor_element VARCHAR(20) DEFAULT NULL; -- for armor element cards (Pasana->fire)
ALTER TABLE items ADD COLUMN IF NOT EXISTS status_effect VARCHAR(20) DEFAULT NULL;     -- stun, freeze, poison, etc.
ALTER TABLE items ADD COLUMN IF NOT EXISTS status_chance INTEGER DEFAULT 0;            -- 5 = 5%
ALTER TABLE items ADD COLUMN IF NOT EXISTS special_effect TEXT DEFAULT NULL;            -- JSON for complex card/equipment effects
```

#### 9.2.2 New: Character Equipment Slots Table

```sql
-- Tracks compounded cards per equipment instance
CREATE TABLE IF NOT EXISTS character_equipment_cards (
    equipment_card_id SERIAL PRIMARY KEY,
    inventory_id INTEGER REFERENCES character_inventory(inventory_id) ON DELETE CASCADE,
    slot_index INTEGER NOT NULL CHECK (slot_index >= 0 AND slot_index <= 3),  -- 0-3 for up to 4 card slots
    card_item_id INTEGER REFERENCES items(item_id),
    compounded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(inventory_id, slot_index)
);

CREATE INDEX IF NOT EXISTS idx_equip_cards_inventory ON character_equipment_cards(inventory_id);
```

#### 9.2.3 New: Equipment Refine Tracking

```sql
-- Add refine level to character_inventory (per-instance, not per-item-definition)
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS refine_level INTEGER DEFAULT 0;
```

#### 9.2.4 New: Kafra Storage Table

```sql
CREATE TABLE IF NOT EXISTS character_storage (
    storage_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id) ON DELETE CASCADE,  -- account-shared!
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    refine_level INTEGER DEFAULT 0,
    slot_index INTEGER DEFAULT -1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Card slots for stored equipment
CREATE TABLE IF NOT EXISTS storage_equipment_cards (
    storage_card_id SERIAL PRIMARY KEY,
    storage_id INTEGER REFERENCES character_storage(storage_id) ON DELETE CASCADE,
    slot_index INTEGER NOT NULL CHECK (slot_index >= 0 AND slot_index <= 3),
    card_item_id INTEGER REFERENCES items(item_id),
    UNIQUE(storage_id, slot_index)
);

CREATE INDEX IF NOT EXISTS idx_storage_user ON character_storage(user_id);
```

### 9.3 Server-Side Systems to Build

#### 9.3.1 Equip Stat Recalculation

When equipment changes (equip/unequip/refine/card compound), recalculate all derived stats:

```
1. Start from base stats (STR, AGI, VIT, INT, DEX, LUK from level-up allocation)
2. Add job class bonuses
3. For each equipped item:
   a. Add item stat bonuses (str_bonus, agi_bonus, etc.)
   b. Add refine bonuses (ATK for weapons, DEF for armor)
   c. For each compounded card in the item:
      - Add card stat bonuses
      - Register card passive effects (race/element/size modifiers)
4. Calculate derived stats:
   - Total ATK = base_atk + weapon_atk + refine_atk + card_atk + STR bonus
   - Total DEF = base_def + armor_def + refine_def + card_def + VIT bonus
   - Total MATK = base_matk + weapon_matk + INT bonus
   - HIT = base_hit + DEX + card_hit
   - FLEE = base_flee + AGI + card_flee
   - CRIT = base_crit + LUK + card_crit
   - ASPD = calculate from formula (class + weapon_type + AGI + DEX + aspd_buffs)
   - Weight capacity = 2000 + STR*30 + job_bonus + skill_bonus
5. Apply size modifier based on weapon type vs target size (during damage calc)
6. Apply card race/element/size damage modifiers (during damage calc)
```

#### 9.3.2 Card Effect Pipeline

When dealing or receiving damage, process card effects in order:

```
DAMAGE DEALT:
1. Calculate base physical damage
2. Apply weapon size modifier (Section 3.2 table)
3. Apply weapon element vs target element modifier
4. For each weapon card:
   a. +ATK flat bonuses (Andre: +20)
   b. +% race damage (Hydra: +20% vs Demi-Human)
   c. +% element damage (Vadon: +20% vs Fire)
   d. +% size damage (Minorous: +15% vs Large)
   e. Check status effect chance (Marina: 5% Freeze)
5. Apply over-refine random ATK bonus
6. Sum all modifiers multiplicatively (race * element * size)

DAMAGE RECEIVED:
1. Calculate incoming damage
2. Apply armor element modifier (if armor has element card)
3. Apply shield race reduction (Thara Frog: -30% from Demi-Human)
4. Apply shield size reduction (Executioner: -25% from Large)
5. Apply garment element reduction (Raydric: -20% Neutral)
6. Apply DEF reduction from refines
7. Final damage = max(1, result)
```

#### 9.3.3 Refine System Events

```
Socket Events:
  refine:request     { inventoryId }           -> server validates, attempts refine
  refine:success     { inventoryId, newLevel }  -> client updates UI
  refine:failure     { inventoryId, itemName }  -> client shows destruction animation
  refine:error       { message }                -> invalid attempt

Server Logic:
  1. Validate: item exists, is equipment, player has correct ore + zeny
  2. Check current refine vs safe limit
  3. If <= safe: 100% success
  4. If > safe: roll against success rate table
  5. On success: increment refine_level, deduct ore + zeny
  6. On failure: DELETE item from character_inventory (cascade deletes cards), deduct ore + zeny
  7. Recalculate equipped stats if item was equipped
```

#### 9.3.4 Card Compound Events

```
Socket Events:
  card:compound_request   { cardInventoryId, targetInventoryId }  -> server validates
  card:compound_success   { targetInventoryId, slotIndex, cardId } -> client updates
  card:compound_error     { message }                              -> invalid attempt

Server Logic:
  1. Validate: card exists in inventory, target has empty card slot
  2. Validate: card_type matches target equip_slot
  3. Remove card from character_inventory
  4. INSERT into character_equipment_cards
  5. Recalculate equipped stats if target was equipped
  6. Update item display name with prefix/suffix
```

### 9.4 UE5 Client Systems to Build

#### 9.4.1 Equipment Panel (already exists: EquipmentSubsystem)

Current `SEquipmentWidget` already has slot-based layout. Enhancements needed:
- Show refine level on equipment icons ("+7" overlay text)
- Show card slots as small diamond indicators on each equipment piece
- Card compound drag-drop: drag card from inventory onto equipment slot
- Equipment tooltip showing: base stats + refine bonus + card bonuses (each card listed)

#### 9.4.2 Refine UI

New widget or NPC dialog:
- Select equipment from inventory
- Show: current refine, success rate, required ore, required zeny
- "Refine" button with confirmation ("This may destroy your item!")
- Success/failure animation and sound

#### 9.4.3 Card Compound UI

- Double-click card in inventory opens compound dialog
- Shows list of compatible slotted equipment
- Hover shows which slots are filled vs empty
- Confirm button with warning ("Cards cannot be removed!")
- Visual feedback: card sliding into equipment icon

#### 9.4.4 Weight Display

- Weight bar in inventory panel: `Current Weight / Max Weight`
- Color coding: Green (<50%), Yellow (50-89%), Red (90%+)
- Warning toast when crossing 50% or 90% thresholds
- Block pickup when at 100% with error message

### 9.5 Item ID Ranges (Convention)

Based on existing `init.sql` seed data, maintain these ranges:

| Range | Category | Example |
|-------|----------|---------|
| 1001-1999 | Consumables | 1001=Crimson Vial, 1006=Red Herb |
| 2001-2999 | Misc / Etc / Loot | 2009=Jellopy, 2042=Phracon |
| 3001-3999 | Weapons | 3007=Knife, 3010=Falchion |
| 4001-4999 | Armor / Shield / Headgear / Footgear / Accessories | 4004=Guard, 4005=Hat |
| 5001-5999 | Cards | 5001=Poring Card, 5019=Pecopeco Card |
| 6001-6999 | Ammunition | (not yet seeded) |
| 7001-7999 | Pet Eggs / Pet Armor | (not yet seeded) |

---

## Sources

- [iRO Wiki Classic - Equipment](https://irowiki.org/classic/Equipment)
- [iRO Wiki Classic - Weapons](https://irowiki.org/classic/Weapons)
- [iRO Wiki Classic - Refinement System](https://irowiki.org/classic/Refinement_System)
- [iRO Wiki Classic - Card System](https://irowiki.org/classic/Card_System)
- [iRO Wiki Classic - Card Reference](https://irowiki.org/classic/Card_Reference)
- [iRO Wiki Classic - Healing Items](https://irowiki.org/classic/Healing_Items)
- [iRO Wiki Classic - Weight Limit](https://irowiki.org/classic/Weight_Limit)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [iRO Wiki Classic - Cooking](https://irowiki.org/classic/Cooking)
- [iRO Wiki Classic - Storage](https://irowiki.org/classic/Storage)
- [RateMyServer - Weapon Size Modifier Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer - Refine Success Rates](https://ratemyserver.net/index.php?page=misc_table_refine)
- [RateMyServer - Pre-RE Item Database](https://ratemyserver.net/index.php?page=item_db)
- [rAthena - item_db.txt Documentation](https://github.com/rathena/rathena/blob/master/doc/item_db.txt)
- [iRO Wiki - Resistance Potions](https://irowiki.org/wiki/Resistance_Potions)
- [Ragnarok Wiki - Elements](https://ragnarok.fandom.com/wiki/Elements)
