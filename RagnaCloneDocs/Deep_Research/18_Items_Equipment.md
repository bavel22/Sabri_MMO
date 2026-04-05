# Items & Equipment -- Deep Research (Pre-Renewal)

> Comprehensive reference for replicating the Ragnarok Online Classic (pre-Renewal) items and equipment system.
> Sources: iRO Wiki Classic, RateMyServer Pre-RE DB, rAthena pre-re source (item_db, item_bonus.txt), divine-pride, community references.

---

## Item Types Overview

Every item in RO Classic belongs to exactly one type category. The rAthena `item_db` defines these via the `Type` field.

### Master Category Table

| Type ID | Type Name | rAthena Type | Stackable | Equippable | Examples |
|---------|-----------|-------------|-----------|------------|----------|
| 0 | Healing | Healing | Yes | No | Red Potion, White Potion, Yggdrasil Berry |
| 2 | Usable | Usable | Yes | No | Fly Wing, Butterfly Wing, Dead Branch |
| 3 | Etc / Misc | Etc | Yes | No | Jellopy, Iron Ore, Oridecon, Quest items |
| 4 | Armor | Armor | No | Yes | All armor, headgear, shields, shoes, garments, accessories |
| 5 | Weapon | Weapon | No | Yes | All weapons (daggers, swords, bows, etc.) |
| 6 | Card | Card | No | No | Poring Card, Hydra Card, all monster cards |
| 7 | Pet Egg | PetEgg | No | No | Poring Egg, Lunatic Egg |
| 8 | Pet Armor | PetArmor | No | Yes (pet) | Backpack, Armor of Formless, etc. |
| 10 | Ammo | Ammo | Yes | Yes | Arrows, Bullets, Spheres, Kunai, Shuriken |
| 11 | Delayed Usable | DelayConsume | Yes | No | Elemental Converters, Resistance Potions |
| 18 | Shadow Gear | ShadowGear | No | Yes | (Renewal only -- not applicable) |

### Item Sub-Categories

**Usable Items** break down into:

| Sub-Category | Examples | Behavior |
|-------------|----------|----------|
| Healing | Red Potion, White Potion, Yggdrasil Berry | Immediate HP/SP restore; healing = base * (1 + 0.02 * VIT) |
| Delayed Consumable | Elemental Converters, Resist Potions | Applied after cast/delay |
| Buff Potions | Concentration/Awakening/Berserk Potion | Temporary ASPD buff; do NOT stack with each other |
| Cure Items | Green Potion, Panacea, Royal Jelly | Remove negative status effects |
| Teleport Items | Butterfly Wing (save point), Fly Wing (random) | Instant warp; map flags can block |
| Gems/Catalysts | Blue/Yellow/Red Gemstone | Consumed by skills (Warp Portal, Safety Wall, etc.) |
| Scrolls | Wind Scroll, Fire Scroll | Cast a spell on use |
| Stat Foods | Fried Grasshopper Legs through Dragon Breath Cocktail | +1 to +10 stat for 20 min; one per stat |
| Box Items | Old Blue Box, Old Purple Box, Old Card Album | Open for random loot/equipment/card |

**Misc/Etc Items** break down into:

| Sub-Category | Examples | Use |
|-------------|----------|-----|
| Monster Loot | Jellopy, Fluff, Shell, Skel Bone | Sell to NPC for zeny |
| Crafting Materials | Iron Ore, Iron, Steel, Star Crumb | Blacksmith forging, Alchemist brewing |
| Upgrade Ores | Phracon, Emveretarcon, Oridecon, Elunium | Weapon/armor refining |
| Elemental Stones | Flame Heart, Mystic Frozen, Great Nature, Rough Wind | Weapon forging element |
| Quest Items | Varies per quest | Non-tradeable, quest-specific |
| Collectibles | Dolls, Jewels | Trophy items, NPC quests |

---

## Equipment Slots

A character has **10 equipment positions** (11 counting ammo). Each piece of gear occupies exactly one position, except headgear combo items that span multiple head positions.

### Slot Layout

| # | Position | DB Slot Name | Card Compound Type | Max Card Slots | Notes |
|---|----------|-------------|-------------------|----------------|-------|
| 1 | Head (Upper) | `head_top` | Headgear | 1 | Hats, helms, crowns. Most common headgear slot. |
| 2 | Head (Middle) | `head_mid` | Headgear | 1 | Glasses, goggles, monocles. Some items cover Upper+Mid. |
| 3 | Head (Lower) | `head_low` | -- | 0 | Mouth items (Pierrot Nose, Gangster Mask). Rarely has card slot. |
| 4 | Body Armor | `armor` | Armor | 1 | Chest piece. Determines elemental property of character. |
| 5 | Weapon (Right Hand) | `weapon` | Weapon | 0-4 | Primary weapon. Defines attack type, element, range. |
| 6 | Shield (Left Hand) | `shield` | Shield | 1 | Off-hand defense. Cannot equip with two-handed weapons. |
| 7 | Garment | `garment` | Garment | 1 | Capes, manteaus, hoods. Elemental resist common here. |
| 8 | Footgear | `footgear` | Footgear | 1 | Shoes, boots, sandals. AGI/movement bonuses common. |
| 9 | Accessory 1 | `accessory_1` | Accessory | 1 | Rings, clips, rosaries, belts. |
| 10 | Accessory 2 | `accessory_2` | Accessory | 1 | Second accessory slot. Both can be same item. |
| 11 | Ammo | `ammo` | -- | 0 | Arrows, bullets. Consumed per attack. Not always listed as a "slot." |

### Headgear Combo Positions

Some headgear items occupy multiple head slots simultaneously. The rAthena `Locations` field uses a bitmask to indicate which head positions are occupied.

| rAthena Bitmask | Positions Occupied | Example Items |
|----------------|-------------------|---------------|
| 256 | Upper only | Hat, Helm, Crown, Beret, Sakkat, Apple of Archer |
| 512 | Middle only | Sunglasses, Glasses, Masquerade, Monocle |
| 1 | Lower only | Gangster Mask, Pipe, Pierrot Nose, Cigarette |
| 768 (256+512) | Upper + Middle | Blinker, Welding Mask, Angel Wing Ears |
| 513 (512+1) | Middle + Lower | Gas Mask, Opera Phantom Mask, Alarm Mask |
| 257 (256+1) | Upper + Lower | (Rare, few items) |
| 769 (256+512+1) | Upper + Middle + Lower | Wedding Veil (very rare) |

### Two-Handed Weapon Rule

When a character equips a two-handed weapon, the shield slot is forcibly unequipped and locked. Conversely, equipping a shield while a two-handed weapon is equipped must be rejected.

**Always two-handed weapon types:**
- Two-Handed Swords
- Two-Handed Spears
- Two-Handed Axes
- Bows
- Katars (occupy both hands)
- Musical Instruments
- Whips
- Guns (all subtypes: Revolver, Rifle, Shotgun, Gatling, Grenade Launcher)
- Huuma Shuriken

**Always one-handed weapon types (allow shield):**
- Daggers
- One-Handed Swords
- One-Handed Spears
- One-Handed Axes
- Maces
- Rods / Staves
- Knuckles / Fists
- Books

### Dual Wielding (Assassin Only)

Only Assassin and Assassin Cross can dual-wield weapons. When dual wielding, the left-hand slot holds a second one-handed weapon instead of a shield. Valid left-hand weapons: Daggers, One-Handed Swords, One-Handed Axes. Katars are mutually exclusive with dual wielding (katars already occupy both hands). The left-hand weapon attacks at reduced power (penalty varies by skill level of Righthand Mastery and Lefthand Mastery).

---

## Weapon Types

RO Classic has **17 main weapon types** plus bare fist (unarmed). Each type has unique size modifiers, base ASPD contribution, and class restrictions.

### Complete Weapon Type Table

| # | Weapon Type | Hands | rAthena SubType | Usable Classes (Pre-Renewal) |
|---|------------|-------|----------------|------------------------------|
| 0 | Bare Fist (Unarmed) | -- | -- | All |
| 1 | Dagger | 1H | Dagger | Novice, Swordsman, Thief, Mage, Merchant, Ninja + 2nd classes |
| 2 | One-Handed Sword | 1H | 1hSword | Swordsman, Knight, Crusader, Merchant, Blacksmith, Alchemist |
| 3 | Two-Handed Sword | 2H | 2hSword | Swordsman, Knight, Crusader |
| 4 | One-Handed Spear | 1H | 1hSpear | Swordsman, Knight, Crusader |
| 5 | Two-Handed Spear | 2H | 2hSpear | Swordsman, Knight, Crusader |
| 6 | One-Handed Axe | 1H | 1hAxe | Swordsman, Merchant, Blacksmith, Alchemist, Knight |
| 7 | Two-Handed Axe | 2H | 2hAxe | Swordsman, Merchant, Blacksmith, Alchemist, Knight |
| 8 | Mace | 1H | Mace | Swordsman, Acolyte, Priest, Monk, Merchant, Blacksmith, Alchemist |
| 9 | Rod / Staff | 1H | Staff | Mage, Wizard, Sage, Acolyte, Priest, Monk |
| 10 | Bow | 2H | Bow | Archer, Hunter, Bard, Dancer, Rogue |
| 11 | Katar | 2H | Katar | Assassin, Assassin Cross |
| 12 | Knuckle / Fist | 1H | Knuckle | Monk, Champion, Priest |
| 13 | Book | 1H | Book | Sage, Professor, Priest, High Priest |
| 14 | Musical Instrument | 2H | Instrument | Bard, Clown |
| 15 | Whip | 2H | Whip | Dancer, Gypsy |
| 16 | Gun | 2H | Gun | Gunslinger (all 5 subtypes) |
| 17 | Huuma Shuriken | 2H | Huuma | Ninja |

### Size Modifier Table (Pre-Renewal)

Size modifiers are a percentage applied to the weapon's physical ATK component based on the target's size. This is one of the most important damage mechanics in RO. Only the weapon ATK portion (not StatusATK) is affected.

| Weapon Type | vs Small (S) | vs Medium (M) | vs Large (L) | Best Against |
|-------------|:-----:|:------:|:-----:|-------------|
| Bare Fist | 100% | 100% | 100% | All (no penalty) |
| Dagger | **100%** | 75% | 50% | Small |
| 1H Sword | 75% | **100%** | 75% | Medium |
| 2H Sword | 75% | 75% | **100%** | Large |
| 1H Spear | 75% | 75% | **100%** | Large |
| 2H Spear | 75% | 75% | **100%** | Large |
| 1H Axe | 50% | 75% | **100%** | Large |
| 2H Axe | 50% | 75% | **100%** | Large |
| Mace | 75% | **100%** | **100%** | Medium+Large |
| Rod / Staff | **100%** | **100%** | **100%** | All (no penalty) |
| Bow | **100%** | **100%** | 75% | Small+Medium |
| Katar | 75% | **100%** | 75% | Medium |
| Knuckle / Fist | **100%** | 75% | 50% | Small |
| Book | **100%** | **100%** | 50% | Small+Medium |
| Instrument | 75% | **100%** | 75% | Medium |
| Whip | 75% | **100%** | 50% | Medium |
| Gun (all) | **100%** | **100%** | **100%** | All (no penalty) |
| Huuma Shuriken | **100%** | **100%** | **100%** | All (no penalty) |

The Drake Card (MVP card, weapon slot) nullifies all size penalties entirely (100% damage vs all sizes).

### Weapon Levels (1-4)

Every weapon has a **weapon level** (1-4) that determines refinement bonuses, safe upgrade limits, required ores, and typical ATK ranges.

| Property | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 |
|----------|:-----------:|:-----------:|:-----------:|:-----------:|
| ATK per refine | +2 | +3 | +5 | +7 |
| Over-refine bonus (per level above safe) | random 0-3 | random 0-5 | random 0-8 | random 0-13 |
| Safe limit | +7 | +6 | +5 | +4 |
| Refine ore | Phracon (200z NPC) | Emveretarcon (1,000z NPC) | Oridecon (drop) | Oridecon (drop) |
| Refine zeny cost | 50z | 200z | 5,000z | 20,000z |
| Typical ATK range | 10-60 | 30-100 | 60-160 | 100-200+ |
| Examples | Knife, Rod, Club, Bow[0] | Cutter, Mace, CrossBow | Main Gauche, Falchion, Spear | Mysteltainn, Excalibur, Balmung |

**ATK Range Examples by Weapon Level:**

| Weapon Lv | Low ATK | Mid ATK | High ATK | Highest ATK |
|-----------|---------|---------|----------|-------------|
| Lv 1 | Knife (17) | Composite Bow (29) | Main Gauche (43) | Pike (60) |
| Lv 2 | Wand (25) | Stiletto (47) | Chain (84) | Arbalest (90) |
| Lv 3 | Gladius (60) | Katar (148) | Claymore (180) | Lance (185) |
| Lv 4 | Bazerald (70) | Brocca (120) | Mysteltainn (170) | Balmung (200), Masamune (200) |

### ASPD (Attack Speed) by Weapon Type

Each class has a Base ASPD value (146-158), modified by the equipped weapon type. Weapon delay values differ per class and weapon combination.

**Pre-Renewal ASPD Formula:**
```
ASPD = 200 - (WD - ([WD * AGI / 25] + [WD * DEX / 100]) / 10) * (1 - SM)
```
Where:
- `WD` = Weapon Delay (class-specific base * weapon modifier)
- `SM` = Speed Modifier (sum of ASPD% buffs: Berserk Potion, Twohand Quicken, Adrenaline Rush, etc.)
- Max ASPD = 190 (0.2s between attacks, 5 hits per second)
- Hits per second = `50 / (200 - ASPD)`

**Relative ASPD by Weapon Type (approximate):**

| Weapon Type | Relative Speed | Approx. Base Swing |
|-------------|:-------------:|:-------------------:|
| Dagger | Fastest | 0.50-0.65s |
| Knuckle / Fist | Very Fast | 0.50-0.60s |
| Katar | Fast | 0.55-0.65s |
| 1H Sword | Medium-Fast | 0.55-0.70s |
| Mace | Medium | 0.60-0.70s |
| Book | Medium | 0.60-0.70s |
| 1H Axe | Medium-Slow | 0.60-0.75s |
| 2H Sword | Medium-Slow | 0.60-0.75s |
| Spear (1H/2H) | Slow | 0.65-0.80s |
| Rod / Staff | Slow | 0.65-0.80s |
| Instrument | Slow | 0.65-0.80s |
| Whip | Slow | 0.65-0.80s |
| Bow | Slow | 0.70-0.85s |
| 2H Axe | Very Slow | 0.70-0.85s |

### Class Restrictions per Weapon Type

Every equipment item has an `equip_jobs` field (bitmask in rAthena) defining which classes can equip it. In practice, weapon types naturally restrict by class:

| Weapon Type | 1st Job Classes | 2nd Job Classes |
|-------------|----------------|-----------------|
| Dagger | Novice, Swordsman, Thief, Mage, Merchant | Knight, Crusader, Assassin, Rogue, Wizard, Sage, Blacksmith, Alchemist |
| 1H Sword | Swordsman, Merchant | Knight, Crusader, Blacksmith, Alchemist |
| 2H Sword | Swordsman | Knight, Crusader |
| 1H Spear | Swordsman | Knight, Crusader |
| 2H Spear | Swordsman | Knight, Crusader |
| 1H Axe | Swordsman, Merchant | Knight, Blacksmith, Alchemist |
| 2H Axe | Swordsman, Merchant | Knight, Blacksmith, Alchemist |
| Mace | Swordsman, Acolyte, Merchant | Knight, Crusader, Priest, Monk, Blacksmith, Alchemist |
| Rod / Staff | Mage, Acolyte | Wizard, Sage, Priest, Monk |
| Bow | Archer | Hunter, Bard, Dancer, Rogue |
| Katar | -- | Assassin |
| Knuckle | -- | Monk, Priest |
| Book | -- | Sage, Priest |
| Instrument | -- | Bard |
| Whip | -- | Dancer |

### rAthena Job Bitmask (equip_jobs)

In rAthena YAML format, jobs are listed by name. The legacy TXT format uses a hex bitmask:

| Bit | Hex Value | Job |
|-----|-----------|-----|
| 0 | 0x00000001 | Novice |
| 1 | 0x00000002 | Swordsman |
| 2 | 0x00000004 | Mage |
| 3 | 0x00000008 | Archer |
| 4 | 0x00000010 | Acolyte |
| 5 | 0x00000020 | Merchant |
| 6 | 0x00000040 | Thief |
| 7 | 0x00000080 | Knight |
| 8 | 0x00000100 | Priest |
| 9 | 0x00000200 | Wizard |
| 10 | 0x00000400 | Blacksmith |
| 11 | 0x00000800 | Hunter |
| 12 | 0x00001000 | Assassin |
| 14 | 0x00004000 | Crusader |
| 15 | 0x00008000 | Monk |
| 16 | 0x00010000 | Sage |
| 17 | 0x00020000 | Rogue |
| 18 | 0x00040000 | Alchemist |
| 19 | 0x00080000 | Bard / Dancer |
| 21 | 0x00200000 | Taekwon |
| 23 | 0x00800000 | Gunslinger |
| 24 | 0x01000000 | Ninja |
| 25 | 0x02000000 | Super Novice |

`-1` (0xFFFFFFFF) = All classes can equip. The `equip_upper` field separately restricts by tier (Normal / Transcendent / Baby).

### Weapon Element

All weapons are Neutral element by default unless specified. Weapon element can be changed by:

| Method | Element | Duration | Source |
|--------|---------|----------|--------|
| Inherent weapon property | Varies | Permanent | Weapon's built-in element (e.g., Ice Falchion = Water) |
| Aspersio (Priest) | Holy | Until removed | Buff skill |
| Enchant Poison (Assassin) | Poison | Until removed | Buff skill |
| Endow skills (Sage) | Fire/Water/Wind/Earth | 20 min | Sage Endow skills (Flame Launcher, Frost Weapon, Lightning Loader, Seismic Weapon) |
| Elemental Converters (consumable) | Fire/Water/Wind/Earth | 20 min | Crafted items |
| Cursed Water (consumable) | Shadow | Until removed | Holy Water + item |
| Forged weapons (Blacksmith) | Fire/Water/Wind/Earth | Permanent | Weapon forged with elemental stone |

**Element priority** (in Sabri_MMO): Endow buff > Arrow element (if non-neutral) > Weapon's inherent element.

---

## Armor Types (Properties, DEF, Element)

### Body Armor

Body armor is the primary source of hard DEF and the only equipment slot that can change the character's elemental property.

**Key Body Armors (Pre-Renewal):**

| Armor | DEF | Weight | Slots | Special | Classes |
|-------|:---:|:------:|:-----:|---------|---------|
| Cotton Shirt | 1 | 10 | 0 | Starter armor | All |
| Padded Armor | 4 | 40 | 0 | -- | Swordsman, Merchant, Thief |
| Chain Mail | 5 | 33 | 0 | -- | Swordsman, Merchant, Knight, Blacksmith, Crusader |
| Full Plate | 8 | 45 | 0 | -- | Swordsman, Knight, Crusader |
| Saint's Robe | 6 | 60 | 1 | MDEF+5 | Mage, Acolyte, Priest, Wizard, Sage, Monk |
| Tights | 5 | 10 | 1 | DEX+1 | Archer, Hunter, Bard, Dancer, Rogue |
| Thief Clothes | 4 | 10 | 1 | AGI+1 | Thief, Assassin, Rogue |
| Silk Robe | 3 | 40 | 1 | MDEF+10 | Mage, Wizard, Sage |
| Mink Coat | 5 | 23 | 0 | -- | All |
| Formal Suit | 5 | 30 | 1 | -- | All |
| Lucius's Fierce Armor of Volcano | 4 | 220 | 1 | Fire property, STR+2 | Knight, Crusader |
| Sniping Suit | 5 | 5 | 1 | MDEF+5, CRI+5, Perfect Dodge+5 | Hunter only |

**Armor element:** When an armor card changes the character's element (e.g., Pasana Card = Fire Lv1), all incoming damage is calculated against the character's new elemental property using the element table. This is one of the most impactful mechanics in RO PvP and PvE.

**Armor refine bonus:** Each +1 refine adds `floor((3 + refineLevel) / 4)` hard DEF. A +10 armor gains approximately +3 DEF (on top of base).

### Shields

Shields occupy the left hand and provide hard DEF. They cannot be equipped alongside two-handed weapons.

**Key Shields (Pre-Renewal):**

| Shield | DEF | Weight | Slots | Special | Classes |
|--------|:---:|:------:|:-----:|---------|---------|
| Guard | 3 | 30 | 1 | -- | All (1H weapon users) |
| Buckler | 4 | 60 | 0 | -- | Swordsman, Merchant, Thief classes |
| Shield | 6 | 130 | 0 | -- | Swordsman, Knight, Crusader |
| Mirror Shield | 4 | 40 | 0 | MDEF+5 | All |
| Stone Buckler | 3 | 60 | 0 | 5% less dmg from Large | All |
| Thorny Buckler | 3 | 60 | 0 | MDEF+2 | All |
| Strong Shield | 5 | 250 | 0 | 20% less dmg from Long Range; -25% Fire/Water/Wind/Earth | Swordsman, Knight, Crusader |
| Valkyrja's Shield | 3 | 50 | 1 | MDEF+5, 20% resist Water/Fire/Shadow/Undead | All |
| Cross Shield | 6 | 200 | 0 | DEF+5, MDEF+3, -3% ASPD | Crusader only |

### Garments

Garments (capes, mantles, hoods) are equipped on the back. Commonly used for neutral damage reduction and elemental resist.

**Key Garments (Pre-Renewal):**

| Garment | DEF | Weight | Slots | Special | Classes |
|---------|:---:|:------:|:-----:|---------|---------|
| Hood | 1 | 20 | 1 | -- | All |
| Muffler | 2 | 40 | 1 | -- | All |
| Manteau | 4 | 60 | 1 | -- | Swordsman, Merchant + 2nd classes |
| Undershirt | 2 | 15 | 0 | MDEF+1, combo with Pantie | All |
| Immune Manteau (Manteau + Raydric) | 4 | 60 | 0 | 20% Neutral resist | All |
| Vali's Manteau | 3 | 40 | 0 | MDEF+5 | All |
| Morrigane's Manteau | 3 | 60 | 0 | Perfect Dodge +3, set bonus | All |

### Footgear

Shoes and boots, commonly used for HP/SP bonuses.

**Key Footgear (Pre-Renewal):**

| Footgear | DEF | Weight | Slots | Special | Classes |
|----------|:---:|:------:|:-----:|---------|---------|
| Sandals | 1 | 20 | 1 | -- | All |
| Shoes | 2 | 40 | 1 | -- | All |
| Boots | 4 | 60 | 1 | -- | Swordsman, Merchant, Archer + 2nd |
| Crystal Pumps | 0 | 10 | 0 | LUK+5, MDEF+5 | All (female) |
| Goibne's Greaves | 3 | 70 | 0 | MDEF+3, set bonus | All |
| Variant Shoes | 3 | 50 | 0 | MaxHP +20%, MaxSP -20% OR MaxSP +20%, MaxHP -20% | All |

### Accessories

Two accessory slots available. Provide utility bonuses, stat boosts, or active skill effects.

**Key Accessories (Pre-Renewal):**

| Accessory | DEF | Weight | Slots | Special | Classes |
|-----------|:---:|:------:|:-----:|---------|---------|
| Ring | 0 | 10 | 0 | STR+2 | All |
| Earring | 0 | 10 | 0 | INT+2 | All |
| Glove | 0 | 10 | 0 | DEX+2 | All |
| Brooch | 0 | 10 | 0 | AGI+2 | All |
| Necklace | 0 | 10 | 0 | VIT+2 | All |
| Rosary | 0 | 10 | 0 | LUK+2, MDEF+5 | All |
| Safety Ring | 0 | 10 | 0 | DEF+3, MDEF+3 | All |
| Critical Ring | 0 | 10 | 0 | CRI+5 | All |
| Clip | 0 | 10 | 1 | -- | All |
| Celebrant's Mitten | 0 | 10 | 0 | LUK+1, CRI+3 | All |
| Belt | 0 | 20 | 0 | STR+1 | All |
| Orlean's Gloves | 0 | 10 | 0 | DEX+2, cast time -3% | Mage, Wizard, Sage |

---

## Headgear System (Upper/Mid/Lower Slots, Combo)

### Headgear Position Categories

Headgear is the most diverse equipment category in RO, with hundreds of items spanning three head positions.

**Upper Headgear** (position 256) -- Largest category:

| Headgear | DEF | Weight | Slots | Special | Classes |
|----------|:---:|:------:|:-----:|---------|---------|
| Helm | 6 | 60 | 1 | -- | Swordsman, Knight, Crusader |
| Hat | 2 | 30 | 0 | -- | All |
| Cap | 3 | 40 | 0 | -- | All |
| Crown | 2 | 40 | 0 | INT+1 | All (male) |
| Tiara | 2 | 40 | 0 | INT+1 | All (female) |
| Beret | 2 | 10 | 0 | 10% less dmg from Demi-Human | All |
| Sakkat | 3 | 30 | 0 | AGI+1 | All |
| Apple of Archer | 0 | 20 | 0 | DEX+3 | All |
| Mage Hat | 3 | 40 | 0 | INT+1, MDEF+3 | Mage, Wizard, Sage |
| Feather Beret | 1 | 60 | 0 | 10% less dmg from all | All |
| Ulle's Cap | 2 | 40 | 0 | DEX+2, AGI+1, MDEF+1 | All |
| Wizard Hat | 3 | 30 | 0 | INT+2, MDEF+3 | Mage, Wizard, Sage |
| Crescent Helm | 5 | 80 | 0 | STR+1 | Swordsman, Knight, Crusader |

**Middle Headgear** (position 512):

| Headgear | DEF | Weight | Special | Classes |
|----------|:---:|:------:|---------|---------|
| Sunglasses | 0 | 10 | -- | All |
| Glasses | 0 | 10 | -- | All |
| Masquerade | 0 | 10 | -- | All |
| Monocle | 0 | 10 | -- | All |
| Elder's Branch | 0 | 10 | INT+1 | All |

**Lower Headgear** (position 1):

| Headgear | DEF | Weight | Special | Classes |
|----------|:---:|:------:|---------|---------|
| Gangster Mask | 0 | 10 | -- | All |
| Pipe | 0 | 10 | -- | All |
| Pierrot Nose | 0 | 10 | -- | All |
| Cigarette | 0 | 10 | -- | All |
| Gentleman's Pipe | 0 | 10 | -- | All |

### Headgear with Set Bonuses

Certain headgear pieces trigger combo bonuses when equipped together with specific items. See "Set Bonuses" section below.

---

## Card Slots (0-4 Slots, Compounding Rules)

### Slot Distribution

| Equipment Type | Max Card Slots | Typical Range | Notes |
|---------------|:--------------:|:-------------:|-------|
| Weapons | 4 | 0-4 | Most weapons: 0-3. 4-slot weapons are very rare. |
| Body Armor | 1 | 0-1 | -- |
| Shield | 1 | 0-1 | -- |
| Garment | 1 | 0-1 | -- |
| Footgear | 1 | 0-1 | -- |
| Headgear (Upper) | 1 | 0-1 | -- |
| Headgear (Mid) | 1 | 0-1 | Extremely rare to have a slot |
| Headgear (Lower) | 0 | 0 | Lower headgear never has card slots in Classic |
| Accessory | 1 | 0-1 | Clips are the primary 1-slot accessory |

**Slot trade-off:** More slots generally means lower base stats. A Blade [3] (ATK 53, Lv1) has lower ATK than a Katana [0] (ATK 60, Lv2), but three card slots offer far greater customization.

### Compounding Rules

1. **Card type must match equipment type.** A weapon card can only go into a weapon. An armor card can only go into body armor.
2. **Equipment must have an empty slot.** Cards fill slots sequentially (slot 0, then 1, 2, 3).
3. **Compounding is permanent.** Once inserted, a card cannot be removed without destroying the equipment.
4. **Duplicate cards allowed.** You can put 4x Hydra Card into a 4-slot weapon for +80% vs Demi-Human.
5. **Cards are consumed.** The card item is removed from inventory when compounded.
6. **Equipment name changes.** The item gains the card's prefix or suffix (see below).
7. **If equipment is destroyed** (refine failure, etc.), all compounded cards are lost.

### Card Prefix / Suffix System

Each card has a predetermined prefix (appears before item name) or suffix (appears after, starting with "of"). Multiple cards = multiple prefixes/suffixes on the same item.

| Card | Type | Prefix/Suffix | Example Result |
|------|------|--------------|----------------|
| Hydra Card | Weapon | "Bloody" (prefix) | "Bloody Blade [3]" |
| Andre Card | Weapon | "Mighty" (prefix) | "Mighty Blade [3]" |
| Thara Frog Card | Shield | "Cranial" (prefix) | "Cranial Guard [1]" |
| Raydric Card | Garment | "Immune" (suffix) | "Muffler of Immune [1]" |
| Pasana Card | Armor | "of Ifrit" (suffix) | "Saint's Robe of Ifrit [1]" |
| Poring Card | Armor | -- (no prefix) | "Armor [1]" |
| Matyr Card | Footgear | -- | "Boots [1]" |
| Creamy Card | Accessory | -- | "Clip [1]" |

### Socket Enchant (Slot Addition)

An NPC service that can add 1 card slot to certain equipment that originally has 0 slots.

**NPCs:** Seiyablem (weapons) and Leablem (armor), found in Payon, Morroc, Prontera, Lighthalzen.

**Requirements per attempt:**
- The specific item (must be in inventory)
- Ores/materials (varies by item class: C/B/A/S)
- Zeny fee

**Success/Failure:**
- Success: item gains +1 slot
- Failure: **item and all compounded cards are destroyed**
- Success rate depends on item class (C = highest, S = lowest)

**Important rules:**
- Not all equipment is socket-enchantable -- only items on the NPC's list
- Only one copy of the target item should be in inventory (NPC picks randomly if multiples exist)

---

## Item Properties (Weight, Sell Price, Restrictions)

### Weight

Every item has a weight value. Weight is the primary inventory limiter in RO Classic (not slot count).

**Weight by Category:**

| Category | Typical Weight Range | Examples |
|----------|:-------------------:|----------|
| Cards | 1 | All cards weigh exactly 1 |
| Arrows (per unit) | 0.1 | Nearly weightless |
| Consumable potions | 2-30 | Red Potion (7), White Potion (15), Ygg Berry (30) |
| Fly Wing / Butterfly Wing | 5 | Teleport items |
| Misc loot (light) | 1-5 | Jellopy (1), Fluff (1) |
| Misc loot (heavy) | 5-50 | Iron Ore (15), Oridecon (20) |
| Light armor | 10-40 | Tights (10), Cotton Shirt (10), Thief Clothes (10) |
| Heavy armor | 40-80 | Chain Mail (33), Full Plate (45) |
| Weapons | 30-250 | Knife (60), Claymore (250), Dagger (60) |
| Shields | 30-250 | Guard (30), Strong Shield (250) |
| Headgear | 10-80 | Hat (30), Helm (60) |
| Garments | 15-60 | Hood (20), Manteau (60) |
| Footgear | 20-70 | Sandals (20), Boots (60) |
| Accessories | 10-30 | Ring (10), Safety Ring (10) |

### Sell Price / Buy Price

| Formula | Description |
|---------|-------------|
| NPC Buy Price | The price you pay to buy from an NPC shop. Set per item in the database. |
| NPC Sell Price | `floor(Buy_Price / 2)` -- You always sell to NPCs at half the buy price. |
| Overcharge (Merchant skill) | Sell price * `(105 + 2 * skillLevel - (skillLevel >= 10 ? 1 : 0)) / 100` |
| Discount (Merchant skill) | Buy price * `(100 - (5 + 2 * skillLevel - (skillLevel >= 10 ? 1 : 0))) / 100` |

**Items with no NPC buy price** (buy_price = 0) can only be obtained from drops or player trade. They still have a sell price for NPC selling.

### Level Restrictions

Equipment has a `required_level` (base level minimum):

| Weapon Level | Typical Required Level Range |
|-------------|:---------------------------:|
| Lv 1 | 1-14 |
| Lv 2 | 12-27 |
| Lv 3 | 24-44 |
| Lv 4 | 36-99 |
| Armor | 1-80+ (varies widely) |

### Gender Restrictions

Some equipment is gender-locked:
- Crown = Male only
- Tiara = Female only
- Crystal Pumps = Female only
- Wedding Dress = Female only

### Indestructible Property

Certain items (e.g., Balmung, Lord's Clothes) have the `Indestructible` flag and cannot be broken by skills like Acid Terror or Pressure. In rAthena, this is `Flags: { DropEffect: true }` or script `bonus bUnbreakableWeapon,0;`.

---

## Consumable Items (Potions, Food, Scrolls, Ammunition)

### HP Potions

| Item | HP Restored | Weight | NPC Buy | Notes |
|------|:-----------:|:------:|:-------:|-------|
| Red Potion | 45-65 | 7 | 50z | Starter |
| Orange Potion | 105-145 | 10 | 200z | Early game |
| Yellow Potion | 175-235 | 13 | 550z | Mid-game staple |
| White Potion | 325-405 | 15 | 1,200z | Endgame standard |
| Condensed Red Potion | ~70-110 | 2 | Crafted | Alchemist Pharmacy |
| Condensed Yellow Potion | ~225-325 | 4 | Crafted | Alchemist Pharmacy |
| Condensed White Potion | ~425-550 | 3 | Crafted | "Slims" -- WoE/MVP staple |
| Mastela Fruit | 400-600 | 3 | 8,500z | Best HP/weight ratio |
| Yggdrasil Berry | Full HP+SP | 30 | Drop only | Rarest healing item |
| Yggdrasil Seed | 50% HP+SP | 30 | Drop only | Half-Ygg |
| Fresh Fish | 100-150 | 2 | 250z | Light snack |
| Meat | 70 | 15 | 25z | Monster drop |

**Potion healing formula:** `Actual_Heal = base_heal * (1 + 0.02 * VIT)` -- higher VIT increases potion effectiveness.

### SP Potions

| Item | SP Restored | Weight | Source | Notes |
|------|:-----------:|:------:|:------:|-------|
| Blue Potion | 40-60 | 15 | Drop only | Only direct SP potion |
| Grape | 25 | 2 | Trade/Drop | Common SP item |
| Strawberry | 16 | 2 | Drop | -- |
| Blue Herb | 15-30 | 7 | Drop | Raw herb |
| Honey | 70 HP + 20 SP | 10 | 100z | Dual recovery |
| Royal Jelly | 100 HP + 40 SP | 15 | Quest/Drop | Also cures status |

### Cure / Status Items

| Item | Effect | Weight | Source |
|------|--------|:------:|--------|
| Green Potion | Cure Poison, Silence, Blind, Confusion | 7 | Alchemist-crafted |
| Panacea | Cure all negative status effects | 10 | NPC / Drop |
| Royal Jelly | Cure all status + heal HP/SP | 15 | Drop / Quest |
| Holy Water | Cure Cursed status | 3 | NPC / Acolyte skill |
| Karvodailnirol | Cure all status effects | 7 | Monster drop |

### ASPD / Speed Potions

| Item | Effect | Duration | Weight | Usable By |
|------|--------|:--------:|:------:|-----------|
| Concentration Potion | ASPD +10% | 30 min | 10 | All classes |
| Awakening Potion | ASPD +15% | 30 min | 15 | 2nd class+ |
| Berserk Potion | ASPD +20% | 30 min | 20 | 2nd class+ (not Mage/Acolyte line) |
| Speed Potion | Move speed +100% | 5 sec | 10 | All |

ASPD potions do NOT stack with each other -- only the strongest applies. They DO stack with skill-based ASPD buffs (Twohand Quicken, Adrenaline Rush).

### Stat Food

Crafted via the Cooking system. 60 foods total: 10 levels per stat.

| Stat | +1 Food | +5 Food | +10 Food | Duration |
|------|---------|---------|----------|:--------:|
| STR | Fried Grasshopper Legs | Bomber Steak | Steamed Tongue | 20 min |
| AGI | Frog Egg Squid Ink Soup | Herb-Roasted Trout | Steamed Desert Scorpions | 20 min |
| VIT | Steamed Crab Nippers | Cooked Vegetables | Immortal Stew | 20 min |
| INT | Grape Juice Herbal Tea | Fruit Mix | Dragon Breath Cocktail | 20 min |
| DEX | Honey Grape Juice | Fish Stew | Hwergelmir's Tonic | 20 min |
| LUK | Fried Monkey Tails | Lucky Soup | Cooked Nine Tail's Tails | 20 min |

- All stat foods also restore some HP/SP on use
- Effects stack with buff skills (Blessing, Increase AGI, etc.)
- Only one food per stat at a time -- eating a new one replaces the old
- +6 to +10 recipes require cookbooks from monster drops (not NPC-purchasable)

### Elemental Resist Potions

| Item | Effect | Duration | Weight |
|------|--------|:--------:|:------:|
| Fireproof Potion | Fire resist +20%, Water resist -15% | 10 min | 10 |
| Coldproof Potion | Water resist +20%, Wind resist -15% | 10 min | 10 |
| Thunderproof Potion | Wind resist +20%, Earth resist -15% | 10 min | 10 |
| Earthproof Potion | Earth resist +20%, Fire resist -15% | 10 min | 10 |

### Elemental Converters

| Item | Effect | Duration | Weight |
|------|--------|:--------:|:------:|
| Flame Elemental Converter | Weapon becomes Fire element | 20 min | 1 |
| Frost Elemental Converter | Weapon becomes Water element | 20 min | 1 |
| Seismic Elemental Converter | Weapon becomes Earth element | 20 min | 1 |
| Lightning Elemental Converter | Weapon becomes Wind element | 20 min | 1 |

### Other Notable Consumables

| Item | Effect | Weight | Source |
|------|--------|:------:|--------|
| Butterfly Wing | Teleport to save point | 5 | NPC 300z |
| Fly Wing | Random teleport on current map | 5 | NPC 60z |
| Magnifier | Identify one unidentified item | 2 | NPC 40z |
| Old Blue Box | Random item (any non-MVP) | 20 | Rare drop |
| Old Purple Box | Random equipment | 20 | Rare drop |
| Old Card Album | Random non-MVP card | 5 | Very rare drop |
| Dead Branch | Summon random normal monster | 1 | Quest/Drop |
| Bloody Branch | Summon random MVP monster | 1 | Very rare drop |
| Token of Siegfried | Resurrect at full HP/SP on death | 1 | Event/Quest |
| Aloevera | Self-cast Provoke Lv1 (+ATK, -DEF) | 2 | NPC |

### Ammunition

| Ammo Type | Used By | Examples | Element |
|-----------|---------|----------|---------|
| Arrows | Bow | Arrow (Neutral), Fire Arrow, Silver Arrow (Holy), Crystal Arrow (Water), Stone Arrow (Earth), Wind Arrow, Immaterial Arrow (Ghost), Shadow Arrow, Oridecon Arrow, Steel Arrow | Varies |
| Bullets | Guns (Gunslinger) | Silver Bullet, Bloody Shell, Full Metal Jacket | Varies |
| Spheres | Grenade Launcher | Elemental spheres | Varies |
| Shuriken | Ninja | Shuriken, Nimbus Shuriken, Flash Shuriken | Varies |
| Kunai | Ninja | Kunai, Poisoning Kunai | Varies |

- Arrows override weapon element for damage calculation (if arrow is non-neutral)
- 1 arrow consumed per auto-attack; 1 per skill use (Double Strafe, Arrow Shower, etc.)
- Arrow element priority: Endow > Arrow (non-neutral) > Weapon
- Arrow Crafting skill (Archer/Hunter): converts misc items into arrows (45+ recipes)

---

## Unidentified Items (Magnifier, Identify Skill)

### How Unidentified Items Work

In RO Classic, equipment dropped by monsters is **unidentified by default**. Unidentified items:
- Display a generic name based on weapon/armor type (e.g., "Sword", "Shoes", "Armor")
- Cannot be equipped
- Cannot have their stats viewed (hidden until identified)
- Can be sold to NPCs (but at reduced value)
- Can be stored, traded, and dropped

### Identification Methods

| Method | Source | Cost | Notes |
|--------|--------|:----:|-------|
| Magnifier (Item ID 611) | NPC buy (40z) | 40z | Consumable item. Ctrl+Right-click on unidentified item. |
| Item Appraisal / Identify (Skill) | Merchant class skill (Lv1) | Free | Unlimited use, no magnifier needed. Also available to Super Novice. |

### Generic Name Mapping

When an item is unidentified, its display name is replaced with a generic descriptor:

| Equipment Type | Generic Display Name |
|---------------|---------------------|
| Dagger | "Dagger" |
| 1H Sword | "Sword" |
| 2H Sword | "Two-handed Sword" |
| 1H Spear | "Spear" |
| 2H Spear | "Spear" |
| 1H Axe | "Axe" |
| 2H Axe | "Axe" |
| Mace | "Mace" |
| Rod / Staff | "Rod" |
| Bow | "Bow" |
| Katar | "Katar" |
| Knuckle | "Knuckle" |
| Book | "Book" |
| Instrument | "Instrument" |
| Whip | "Whip" |
| Body Armor | "Armor" |
| Shield | "Shield" |
| Garment | "Garment" |
| Footgear | "Shoes" |
| Headgear (Upper) | "Headgear" |
| Headgear (Mid) | "Headgear" |
| Accessory | "Accessory" |

### Implementation Notes

- The `is_identified` flag is stored per inventory instance (not per item definition)
- Items purchased from NPC shops are always identified
- Items crafted by players (Blacksmith forging, Alchemist brewing) are always identified
- Monster drops default to unidentified for equipment (consumables and misc items are always identified)
- An orange "?" indicator is typically shown on unidentified items in the inventory UI

---

## Item Drops (From Monsters, Rates, Treasure Boxes)

### Monster Drop Mechanics

Each monster has up to 8-10 drop slots, each with an independent drop rate (0.01% to 100%). Card drops occupy a special slot (typically slot 9) at 0.01% base rate.

**Drop rate modifiers:**
- Bubble Gum (consumable): +100% drop rate (doubles all drops)
- Battle Manual: Affects EXP, not drop rate
- Server rate modifier: Multiplied against base rate

### Treasure Box Items

| Box Item | Contents | Drop Source |
|----------|----------|-------------|
| Old Blue Box (603) | Random item from a massive pool (any non-MVP item) | Monster drops, quests |
| Old Purple Box (617) | Random equipment piece | Monster drops, quests |
| Old Card Album (616) | Random non-MVP card | Very rare drops, events |
| Gift Box (644) | Random miscellaneous items | Monster drops |
| Jewelry Box | Random accessories | Special monsters |

### Drop Rate Categories

| Rate Range | Category | Examples |
|:----------:|----------|----------|
| 50-100% | Common | Jellopy, Fluff, basic loot |
| 10-49% | Uncommon | Better loot, some consumables |
| 1-9.99% | Rare | Equipment, rare materials |
| 0.1-0.99% | Very Rare | Good equipment, rare drops |
| 0.01-0.09% | Ultra Rare | Cards, MVP drops, rare equipment |
| 0.01% | Card | Standard card drop rate |

---

## Equipment Bonuses (Stat Bonuses, Script Effects)

### rAthena Item Script Bonus System

Every equipment item can have a `Script` field containing bonus effects. These are applied when the item is equipped and removed when unequipped.

### Basic Stat Bonuses

| Bonus Script | Effect | Example Item |
|-------------|--------|-------------|
| `bonus bStr,n;` | STR + n | Ring (STR+2) |
| `bonus bAgi,n;` | AGI + n | Brooch (AGI+2) |
| `bonus bVit,n;` | VIT + n | Necklace (VIT+2) |
| `bonus bInt,n;` | INT + n | Earring (INT+2) |
| `bonus bDex,n;` | DEX + n | Glove (DEX+2) |
| `bonus bLuk,n;` | LUK + n | Rosary (LUK+2) |
| `bonus bAllStats,n;` | All stats + n | -- |
| `bonus bAgiVit,n;` | AGI+n, VIT+n | -- |
| `bonus bAgiDexStr,n;` | STR+n, AGI+n, DEX+n | -- |

### HP/SP Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus bMaxHP,n;` | MaxHP + n | Pupa Card (+700 HP) |
| `bonus bMaxHPrate,n;` | MaxHP + n% | Pecopeco Card (+10% HP) |
| `bonus bMaxSP,n;` | MaxSP + n | Willow Card (+80 SP) |
| `bonus bMaxSPrate,n;` | MaxSP + n% | Sohee Card (+15% SP) |

### Combat Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus bAtk,n;` | ATK + n | Andre Card (+20 ATK) |
| `bonus bDef,n;` | DEF + n | -- |
| `bonus bMdef,n;` | MDEF + n | Silk Robe (MDEF+10) |
| `bonus bHit,n;` | HIT + n | -- |
| `bonus bFlee,n;` | Flee + n | Whisper Card (+20 Flee) |
| `bonus bCritical,n;` | CRI + n | Critical Ring (+5 CRI) |
| `bonus bPerfectDodge,n;` | Perfect Dodge + n | Poring Card (+1 PD) |
| `bonus bAspd,n;` | ASPD + n | Assassin Dagger (+2 ASPD) |
| `bonus bAspdRate,n;` | ASPD + n% | -- |

### Damage Modifier Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus2 bAddRace,r,n;` | +n% damage vs race r | Hydra Card (+20% vs Demi-Human) |
| `bonus2 bAddEle,e,n;` | +n% damage vs element e | Vadon Card (+20% vs Fire) |
| `bonus2 bAddSize,s,n;` | +n% damage vs size s | Minorous Card (+15% vs Large) |
| `bonus2 bSubRace,r,n;` | -n% damage from race r | Thara Frog Card (-30% from Demi-Human) |
| `bonus2 bSubEle,e,n;` | -n% damage from element e | Raydric Card (-20% from Neutral) |
| `bonus2 bSubSize,s,n;` | -n% damage from size s | -- |

### AutoSpell / AutoCast Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus3 bAutoSpell,id,lv,rate;` | n% chance to cast skill on attack | Marina Card (5% Freeze) |
| `bonus3 bAutoSpellWhenHit,id,lv,rate;` | n% chance to cast skill when hit | -- |
| `bonus bNoSizeFix;` | Nullify size penalties | Drake Card |
| `bonus bNoWeaponDamage;` | Immune to physical damage | -- |
| `bonus bNoMagicDamage,n;` | -n% magic damage | GTB Card (100%) |

### Status Immunity Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus2 bResEff,Eff_Stun,10000;` | Stun immunity (10000 = 100%) | Orc Hero Card |
| `bonus2 bResEff,Eff_Silence,10000;` | Silence immunity | Marduk Card |
| `bonus2 bResEff,Eff_Sleep,10000;` | Sleep immunity | Nightmare Card |
| `bonus2 bResEff,Eff_Blind,10000;` | Blind immunity | Deviruchi Card |
| `bonus2 bResEff,Eff_Freeze,10000;` | Freeze immunity | (Fire armor) |

### Active Skill Bonuses

| Bonus Script | Effect | Example |
|-------------|--------|---------|
| `bonus bSPDrainRate,-30;` | SP cost -30% | Pharaoh Card |
| `bonus bNoGemstone;` | No gemstone requirement | Mistress Card |
| `skill "AL_TELEPORT",1;` | Grants Teleport Lv1 | Creamy Card |
| `skill "TF_HIDING",1;` | Grants Hiding Lv1 | Smokie Card |
| `skill "TF_DETOXIFY",1;` | Grants Detoxify Lv1 | Poporing Card |

---

## Set Bonuses (Specific Item Combinations)

### Equipment Set System

When specific items are equipped together, they trigger additional "combo" bonuses that are applied on top of each item's individual bonuses. The combo check happens during the equip stat recalculation.

### Major Equipment Sets (Pre-Renewal)

**Morpheus's Set** (4 pieces):
| Piece | Slot | Individual Bonus |
|-------|------|-----------------|
| Morpheus's Hood | Upper Headgear | INT+2 |
| Morpheus's Shawl | Garment | MDEF+3, MaxSP+10% |
| Morpheus's Ring | Accessory | INT+1, MaxSP+5% |
| Morpheus's Bracelet | Accessory | INT+1, MaxSP+5% |
| **Full Set Bonus** | -- | **INT+5, MDEF+11, MaxSP+20%, uninterruptible casting, but cast time +25%** |

**Morrigane's Set** (4 pieces):
| Piece | Slot | Individual Bonus |
|-------|------|-----------------|
| Morrigane's Helm | Upper Headgear | STR+2, LUK+2 |
| Morrigane's Manteau | Garment | Perfect Dodge+3, LUK+2 |
| Morrigane's Belt | Accessory | ATK+5, CRI+3 |
| Morrigane's Pendant | Accessory | STR+2, CRI+3 |
| **Full Set Bonus** | -- | **STR+2, LUK+9, CRI+13, ATK+18, Perfect Dodge+13** |

**Goibne's Set** (4 pieces):
| Piece | Slot | Individual Bonus |
|-------|------|-----------------|
| Goibne's Helm | Upper Headgear | DEF+3, MDEF+3 |
| Goibne's Armor | Body Armor | VIT+2, MaxHP+10% |
| Goibne's Spaulders | Garment | VIT+1, DEF+2 |
| Goibne's Greaves | Footgear | MDEF+3, MaxHP+5% |
| **Full Set Bonus** | -- | **VIT+5, MaxHP+15%, MaxSP+5%, DEF+5, MDEF+15** |

### Card Combo Sets

Some cards trigger combo bonuses when compounded on different equipment pieces and equipped together.

**Injustice Card + Rogue Card:**
- Injustice Card (Garment): Flee+3, 2% auto-cast Sonic Blow Lv1 when attacking
- Rogue Card (Accessory): AGI+1
- **Combo Bonus:** AGI+5, Flee+20

**Anolian Card + Alligator Card + Cruiser Card + Merman Card (Swamp Set):**
- Full combo: DEF+2, MDEF+2, Flee+10, HIT+10

**Mantis Card + Creamy Card + Cookie Card + Marin Card:**
- Full combo: ATK+20, CRI+5, Flee+10

### Notable Two-Piece Combos

| Item Combo | Bonus |
|-----------|-------|
| Pantie + Undershirt | AGI+5, Flee+10 |
| Odin's Blessing + Magni's Cap | DEF+2, MDEF+2 |
| Odin's Blessing + Magni's Cap + Stone Buckler | DEF+5, MDEF+5 |
| Odin's Blessing + Frigg's Circlet | DEF+2, MDEF+2, MaxHP+10% |
| Orlean's Gloves + Orlean's Server (Shield) | Reduces variable cast time by 10% |

---

## Refinement System

### How Refining Works

1. Visit a Blacksmith NPC (e.g., Hollgrehenn in Prontera)
2. Select equipment to refine
3. NPC consumes one ore + zeny fee
4. Success: item gains +1 refine level (displayed as "+X" prefix)
5. Failure: item is **permanently destroyed** (including all compounded cards)

### Required Materials

| Equipment | Ore | Ore Source | Zeny Fee |
|-----------|-----|-----------|:--------:|
| Weapon Lv 1 | Phracon | NPC (200z) | 50z |
| Weapon Lv 2 | Emveretarcon | NPC (1,000z) | 200z |
| Weapon Lv 3 | Oridecon | Monster drop / 5x Rough Oridecon | 5,000z |
| Weapon Lv 4 | Oridecon | Monster drop | 20,000z |
| All Armor | Elunium | Monster drop / 5x Rough Elunium | 2,000z |

### Safety Limits and Success Rates

Refining up to the safety limit is 100% success. Above safety, each attempt risks destruction.

| Refine | Weapon Lv1 | Weapon Lv2 | Weapon Lv3 | Weapon Lv4 | Armor |
|:------:|:----------:|:----------:|:----------:|:----------:|:-----:|
| +1 to safe | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 60% | 60% | 60% |
| +6 | 100% | 60% | 50% | 40% | 40% |
| +7 | 100% | 40% | 20% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 20% | 20% | 20% | 10% | 10% |

Safe limits: Lv1=+7, Lv2=+6, Lv3=+5, Lv4=+4, Armor=+4. Maximum refine: **+10**.

### ATK/DEF Bonus Per Refine

**Weapons:**

| Refine | Lv1 (+2/refine) | Lv2 (+3/refine) | Lv3 (+5/refine) | Lv4 (+7/refine) |
|:------:|:---------------:|:---------------:|:---------------:|:---------------:|
| +1 | +2 | +3 | +5 | +7 |
| +4 | +8 | +12 | +20 | +28 |
| +7 | +14 | +18 | +25+rng(0-10) | +28+rng(0-39) |
| +10 | +20+rng(0-9) | +30+rng(0-20) | +50+rng(0-40) | +70+rng(0-78) |

Over-refine formula: For levels above safe limit, add `random(0, overScale * N)` where N = levels above safe, overScale = {Lv1:3, Lv2:5, Lv3:8, Lv4:13}.

**Armor:** Each +1 refine adds approximately `floor((3 + refineLv) / 4)` hard DEF. A +10 armor piece gains roughly +3 DEF.

### Failure Consequences

- Item permanently destroyed (no recovery)
- All compounded cards lost
- Ore consumed
- Zeny fee consumed
- This makes over-refining a high-stakes gambling mechanic

---

## Weight System and Inventory

### Weight Limit Formula

```
Max Weight = 2000 + (Base STR * 30) + Job Bonus + Skill Bonus + Mount Bonus
```

| Component | Value |
|-----------|-------|
| Base | 2,000 for all classes |
| STR bonus | +30 per point of base STR |
| Enlarge Weight Limit Lv10 | +2,000 (Merchant skill) |
| Peco Peco riding | +1,000 |

### Weight Thresholds

| Threshold | Effects |
|:---------:|---------|
| < 50% | Normal -- full HP/SP regen, all skills available |
| 50-89% | No natural HP/SP regeneration. Combat and skills still work. |
| 90-100% | Cannot attack. Cannot use skills. Walking only. |
| > 100% | Cannot pick up items. |

### Inventory Structure

| Property | Value |
|----------|-------|
| Slot limit | Unlimited (weight-limited, not slot-limited) |
| Stacking | Identical usable/etc items stack. Equipment NEVER stacks. |
| Max stack | Typically 99 for consumables, 999 for loot/ammo |

### Kafra Storage

| Property | Value |
|----------|-------|
| Slots | 300 (pre-Renewal) |
| Weight | No weight limit |
| Access | Via Kafra NPCs in towns |
| Cost | 40-80z per access |
| Sharing | Account-shared (all characters on same account) |

### Cart (Merchant Classes)

| Property | Value |
|----------|-------|
| Slots | 100 |
| Weight | 8,000 max |
| Access | Merchant, Blacksmith, Alchemist |
| Speed penalty | -40% movement (offset by Cart skills) |

---

## Implementation Checklist

### Database Schema

- [x] `items` table with basic fields (name, type, weight, price, ATK, DEF, stat bonuses)
- [x] `character_inventory` table with quantity, equipped state, slot_index
- [x] `character_hotbar` table
- [x] Weapon type, ASPD modifier, weapon range columns
- [x] Weapon level, slots (card_slots) columns
- [x] Refine level on character_inventory
- [x] Card slot columns (card_slot_0 through card_slot_3) on character_inventory
- [x] is_identified, is_damaged flags on character_inventory
- [x] class_restrictions, sub_type columns on items
- [x] hit_bonus, flee_bonus, critical_bonus columns
- [x] element column on items (weapon/armor element)
- [x] buy_price, sell_price columns
- [x] Forged attributes (forged_by, forged_element, forged_star_crumbs) on character_inventory
- [ ] equip_jobs bitmask column (currently using JSON class_restrictions -- functional but not matching rAthena format)
- [ ] two_handed boolean flag on items
- [ ] gender restriction field
- [ ] indestructible flag

### Server-Side Systems

- [x] Item definitions loaded into memory Map at startup
- [x] addItemToInventory / removeItemFromInventory
- [x] getPlayerInventory with refine + card data
- [x] Weight system (max weight formula, weight check on pickup)
- [x] Weight thresholds (50%/90% penalties)
- [x] Equipment slot system (10 positions including dual accessories)
- [x] Level requirement validation on equip
- [x] Two-handed weapon / shield mutual exclusion
- [x] Class restriction check on equip
- [x] Stat recalculation on equip/unequip (equipmentBonuses pipeline)
- [x] Refinement system (refine:request handler, success rates, ATK/DEF bonus)
- [x] Card compounding (card:compound handler)
- [x] Card prefix/suffix naming system
- [x] Unidentified item system (identify:item_list/result handlers)
- [x] Inventory operations (use, equip, drop, move, split, merge)
- [x] Cart system (cart:load, cart:move_to/from)
- [x] Arrow/ammo equip slot and consumption
- [x] Arrow element override in damage formula
- [x] Potion healing with VIT scaling
- [x] ASPD potion mutual exclusion (strongest wins)
- [x] Stat food system (one per stat, replace old)
- [x] Consumable items (Fly Wing, Butterfly Wing, etc.)
- [ ] Socket Enchant NPC (add slot to equipment)
- [ ] Treasure box random item generation (OBB, OPB, OCA)
- [ ] NPC shop buy/sell with Discount/Overcharge
- [ ] Guild Storage
- [ ] Equipment visual broadcast to other players
- [ ] Ground item drops (items visible on ground, pick up)
- [ ] Item trading between players

### Client-Side Systems

- [x] InventorySubsystem + SInventoryWidget (grid display, drag/drop, context menu)
- [x] EquipmentSubsystem + SEquipmentWidget (10 slots, stat display, ammo slot)
- [x] KafraSubsystem + SKafraWidget (storage UI)
- [x] CartSubsystem + SCartWidget (cart grid, weight bar)
- [x] ItemTooltipBuilder (stat display, card list, refine display)
- [x] ItemInspectSubsystem + SItemInspectWidget (detailed item view)
- [x] SIdentifyPopup (appraisal UI)
- [x] SCardCompoundPopup (card compounding UI)
- [x] Unidentified item display (generic names, orange "?" overlay, hidden stats)
- [ ] Refine UI (NPC dialog with item selection, success/failure animation)
- [ ] Socket Enchant UI
- [ ] Ground item rendering and pickup
- [ ] Trade window UI
- [ ] Equipment visual on character model (weapon/armor appearance)

---

## Gap Analysis

### Implemented vs. RO Classic Reference

| Feature | RO Classic | Sabri_MMO Status | Priority |
|---------|-----------|:----------------:|:--------:|
| 10 equipment slots | 10 + ammo | Done (11 with ammo) | -- |
| Weapon types (17) | 17 types | Done (all types in DB) | -- |
| Size modifiers | Per weapon type | Done (damage formula) | -- |
| Weapon levels (1-4) | 4 levels | Done (weapon_level column) | -- |
| Refinement (+0 to +10) | Destroy on fail | Done (refine:request) | -- |
| Card slots (0-4) | Permanent compound | Done (card_slot_0-3) | -- |
| Card naming (prefix/suffix) | Display name change | Done (ro_card_prefix_suffix.js) | -- |
| Unidentified items | Magnifier / skill | Done (identify handlers) | -- |
| Weight system | 50%/90% thresholds | Done | -- |
| Dual wield (Assassin) | Left-hand weapon | Done (dual wield system) | -- |
| Potion VIT scaling | heal * (1+0.02*VIT) | Done | -- |
| ASPD potions | Mutual exclusion | Done | -- |
| Ammo system | Arrow element override | Done | -- |
| Cart system | 100 slots, 8000 weight | Done | -- |
| Socket Enchant | NPC slot addition | Not started | Medium |
| Equipment sets/combos | Combo bonuses | Not started | Medium |
| Treasure boxes (OBB/OPB/OCA) | Random item tables | Not started | Low |
| Ground items | Drop + pickup | Not started | Medium |
| NPC shop Discount/Overcharge | Merchant passives | Partial (Overcharge done) | Low |
| Equipment visuals | Weapon/armor on model | Not started | Low |
| Trade window | Player-to-player | Not started | Medium |
| Guild Storage | Guild-shared storage | Not started | Low |

### Known Discrepancies

1. **Job bitmask format:** Sabri_MMO uses JSON array `["swordsman","mage"]` for class_restrictions instead of rAthena's hex bitmask. Functionally equivalent but different format.
2. **Armor refine DEF formula:** Implementation uses `floor((3+refineLv)/4)` per rAthena pre-re. Verified correct.
3. **Over-refine ATK:** Implementation uses cumulative over-scale per level above safe. Some sources show per-hit random; current implementation matches rAthena behavior.
4. **Inventory slots:** RO Classic is purely weight-limited with no slot cap. Sabri_MMO may impose a slot limit for UI reasons -- acceptable divergence.
5. **Two-handed staff:** rAthena has 2H staves as a separate weapon type. Sabri_MMO currently treats all staves as 1H. Could add if needed for specific high-level staves.

---

## Sources

- [iRO Wiki - Equipment](https://irowiki.org/wiki/Equipment)
- [iRO Wiki Classic - Equipment Sets](https://irowiki.org/classic/Equipment_Sets)
- [iRO Wiki Classic - Refinement System](https://irowiki.org/classic/Refinement_System)
- [iRO Wiki - ASPD](https://irowiki.org/wiki/ASPD)
- [iRO Wiki Classic - ASPD](https://irowiki.org/classic/ASPD)
- [iRO Wiki - Card System](https://irowiki.org/wiki/Card_System)
- [iRO Wiki Classic - Card Reference](https://irowiki.org/classic/Card_Reference)
- [iRO Wiki Classic - Socket Enchant](https://irowiki.org/classic/Socket_Enchant)
- [iRO Wiki Classic - Headgears](https://irowiki.org/classic/Headgears)
- [iRO Wiki - Weapons](https://irowiki.org/wiki/Weapons)
- [iRO Wiki - Size](https://irowiki.org/wiki/Size)
- [iRO Wiki - Healing Items](https://irowiki.org/wiki/Healing_Items)
- [iRO Wiki - Dual Wielding](https://irowiki.org/wiki/Dual_Wielding)
- [iRO Wiki - Katar](https://irowiki.org/wiki/Katar)
- [RateMyServer - Pre-RE Item Database](https://ratemyserver.net/index.php?page=item_db)
- [RateMyServer - Pre-RE Weapon Database](https://ratemyserver.net/index.php?page=item_db&item_type=5)
- [RateMyServer - Pre-RE Armor Database](https://ratemyserver.net/index.php?page=item_db&item_type=4&item_class=16)
- [RateMyServer - Pre-RE Shield Database](https://ratemyserver.net/index.php?page=item_db&item_type=4&item_class=32)
- [RateMyServer - Weapon Size Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer - Refine Rates](https://ratemyserver.net/index.php?page=misc_table_refine)
- [RateMyServer - Socket Enchant Guide](https://ratemyserver.net/socket_enchant.php)
- [RateMyServer - Pre-RE Item Combo Database](https://ratemyserver.net/index.php?page=item_combo)
- [rAthena - item_db.txt documentation](https://github.com/rathena/rathena/blob/master/doc/item_db.txt)
- [rAthena - item_bonus.txt documentation](https://github.com/rathena/rathena/blob/master/doc/item_bonus.txt)
- [rAthena - Pre-RE item_db_equip.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/item_db_equip.yml)
- [rAthena - Custom Items Wiki](https://github.com/rathena/rathena/wiki/Custom-Items)
- [Ragnarok Wiki (Fandom) - Equipment](https://ragnarok.fandom.com/wiki/Equipment)
- [Ragnarok Wiki (Fandom) - Socket Enchantment](https://ragnarok.fandom.com/wiki/Socket_Enchantment)
- [Ragnarok Wiki (Fandom) - Elements](https://ragnarok.fandom.com/wiki/Elements)
