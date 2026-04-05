# Refining, Forging & Crafting -- Deep Research (Pre-Renewal)

> **Sources**: rAthena pre-re source (skill.cpp, battle.cpp, status.cpp, refine.yml, produce_db.yml), iRO Wiki Classic, RateMyServer Pre-RE DB, Ragnarok Wiki (Fandom), GameFAQs forging guides, WarpPortal Forums, ROClassicGuide
> **Date**: 2026-03-22
> **Scope**: Pre-Renewal (Classic) only. Renewal mechanics differ significantly and are excluded.

---

## Table of Contents

1. [Refining System](#1-refining-system)
2. [Forging System (Blacksmith)](#2-forging-system-blacksmith)
3. [Pharmacy / Brewing System (Alchemist)](#3-pharmacy--brewing-system-alchemist)
4. [Arrow Crafting](#4-arrow-crafting)
5. [Cooking System](#5-cooking-system)
6. [Implementation Checklist](#6-implementation-checklist)
7. [Gap Analysis](#7-gap-analysis)

---

## 1. Refining System

The refinement system is the primary equipment enhancement mechanic in Ragnarok Online. Players upgrade weapons and armor at Refine NPCs (e.g., Hollgrehenn in Prontera) for increased ATK/DEF. It is the game's most significant zeny sink and risk/reward gamble.

### 1.1 Materials per Weapon Level

Each refine attempt consumes exactly **1 ore** + a **zeny fee**. The ore type depends on the equipment being refined.

| Equipment Type | Required Ore | Ore Source | Zeny Fee Per Attempt |
|----------------|-------------|------------|---------------------|
| Weapon Level 1 | Phracon (ID 1010) | NPC shop, 200z each | 50z |
| Weapon Level 2 | Emveretarcon (ID 1011) | NPC shop, 1,000z each | 200z |
| Weapon Level 3 | Oridecon (ID 984) | Monster drop, or 5 Rough Oridecon -> 1 Oridecon | 5,000z |
| Weapon Level 4 | Oridecon (ID 984) | Monster drop | 20,000z |
| All Armor | Elunium (ID 985) | Monster drop, or 5 Rough Elunium -> 1 Elunium | 2,000z |

**Ore Conversion NPCs**: Players can trade 5 Rough Oridecon (ID 756) to an NPC to receive 1 Oridecon, and 5 Rough Elunium (ID 757) for 1 Elunium.

### 1.2 Safe Limits

Refining up to the safe limit has a **100% success rate**. Above the safe limit, each attempt risks destruction of the item.

| Equipment Type | Safe Limit |
|----------------|-----------|
| Weapon Level 1 | **+7** |
| Weapon Level 2 | **+6** |
| Weapon Level 3 | **+5** |
| Weapon Level 4 | **+4** |
| All Armor | **+4** |

**Maximum refine level**: **+10** for all equipment in pre-renewal.

### 1.3 Success Rates Table (Pre-Renewal / Classic)

Complete success rate table for normal ores. Rates verified against rAthena `db/pre-re/refine.yml` and RateMyServer.

#### Normal Ore Success Rates

| Refine | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 | Armor |
|--------|-------------|-------------|-------------|-------------|-------|
| +1 | 100% | 100% | 100% | 100% | 100% |
| +2 | 100% | 100% | 100% | 100% | 100% |
| +3 | 100% | 100% | 100% | 100% | 100% |
| +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 60% | 60% |
| +6 | 100% | 100% | 60% | 40% | 40% |
| +7 | 100% | 60% | 50% | 40% | 40% |
| +8 | 60% | 40% | 20% | 20% | 20% |
| +9 | 40% | 20% | 20% | 20% | 20% |
| +10 | 20% | 20% | 20% | 10% | 10% |

#### Enriched Ore Success Rates (Pre-Renewal Classic)

Enriched Oridecon (ID 7620) and Enriched Elunium (ID 7619) provide higher success rates. These are typically obtained from Cash Shop or special events.

| Refine | Weapon Lv 1 | Weapon Lv 2 | Weapon Lv 3 | Weapon Lv 4 | Armor |
|--------|-------------|-------------|-------------|-------------|-------|
| +1 | 100% | 100% | 100% | 100% | 100% |
| +2 | 100% | 100% | 100% | 100% | 100% |
| +3 | 100% | 100% | 100% | 100% | 100% |
| +4 | 100% | 100% | 100% | 100% | 100% |
| +5 | 100% | 100% | 100% | 90% | 90% |
| +6 | 100% | 100% | 90% | 70% | 70% |
| +7 | 100% | 90% | 80% | 70% | 70% |
| +8 | 90% | 70% | 60% | 40% | 40% |
| +9 | 70% | 40% | 40% | 40% | 40% |
| +10 | 30% | 30% | 30% | 20% | 20% |

**Note**: Enriched ore rates on Classic servers differ from Renewal. The above table reflects pre-renewal Classic rates from iRO Wiki Classic and rAthena forums. Some private servers may use slightly different values.

**HD (High-Density) Ores**: HD Oridecon and HD Elunium have the same success rates as Enriched ores, but on failure the item **loses 1 refine level instead of being destroyed**. These are typically only available during Refine Events and are not part of the standard pre-renewal game.

### 1.4 Failure Consequences

When refining fails (above safe limit):
- The item is **permanently destroyed**
- All compounded **cards are lost**
- The ore is consumed
- The zeny fee is consumed
- There is **no way to recover** the item

This makes refining above the safe limit a high-stakes gamble. Players typically prepare multiple copies of important items before attempting over-refining.

### 1.5 Refine ATK Bonus per Weapon Level

Each successful refine adds a flat ATK bonus that depends on the weapon level. This bonus is displayed in the status window as the right-side ATK value (e.g., "ATK 150 + 35").

| Weapon Level | ATK Per Refine | Safe Limit | Total Flat ATK at +10 |
|-------------|---------------|-----------|----------------------|
| Level 1 | **+2** ATK | +7 | +20 ATK |
| Level 2 | **+3** ATK | +6 | +30 ATK |
| Level 3 | **+5** ATK | +5 | +50 ATK |
| Level 4 | **+7** ATK | +4 | +70 ATK |

**Damage pipeline placement (rAthena battle.cpp)**: Flat refine ATK (`rhw.atk2`) is added **after DEF reduction** (post-defense). It bypasses both hard DEF% and soft DEF flat, is NOT affected by size penalty, is NOT affected by skill ratios, but IS affected by elemental modifier.

### 1.6 Overupgrade ATK Bonus Formula

For refines **above the safe limit**, each hit deals an additional random bonus ATK on top of the flat refine ATK. This random value changes every attack.

**Formula**: `randomBonus = rnd(1, overrefineMax)`

Where `overrefineMax` depends on weapon level and how many refines exceed the safe limit.

#### Overupgrade Random Bonus Maximums

| Weapon Lv | +5 | +6 | +7 | +8 | +9 | +10 |
|-----------|-----|-----|-----|-----|-----|------|
| Lv 1 (safe +7) | -- | -- | -- | 1~3 | 1~6 | 1~9 |
| Lv 2 (safe +6) | -- | -- | 1~5 | 1~10 | 1~15 | 1~20 |
| Lv 3 (safe +5) | -- | 1~8 | 1~16 | 1~24 | 1~32 | 1~40 |
| Lv 4 (safe +4) | 1~13 | 1~26 | 1~39 | 1~52 | 1~65 | 1~78 |

**Per-level over-refine bonus constants** (from rAthena `refine.yml`):
- Weapon Lv 1: **3** per overupgrade level
- Weapon Lv 2: **5** per overupgrade level
- Weapon Lv 3: **8** per overupgrade level
- Weapon Lv 4: **13** per overupgrade level

**Formula**: `overrefineMax = overBonusPerLevel * (currentRefine - safeLimit)`

**Damage pipeline placement**: The overupgrade bonus is added to **base weapon damage** (before DEF), after size penalty is applied. Unlike flat refine ATK, it IS subject to DEF reduction and IS affected by skill ratios.

**Example**: A +10 Weapon Level 4 (safe +4, 6 overupgrades):
- Flat refine ATK: 10 * 7 = **+70 ATK** (post-DEF, every hit)
- Overupgrade bonus: random **1~78 ATK** per hit (pre-DEF, variable)

**Properties comparison**:

| Property | Flat Refine ATK | Overupgrade Random |
|----------|----------------|-------------------|
| When added | After DEF (post-defense) | Inside base damage (before DEF) |
| Bypasses hard DEF? | YES | NO |
| Bypasses soft DEF? | YES | NO |
| Affected by size penalty? | NO | NO (added after size fix) |
| Affected by skill ratio? | NO | YES |
| Affected by element modifier? | YES | YES |
| Per-hit random? | NO (flat) | YES (1 to max) |
| Shown in status window? | YES (right side "+X") | NO |
| Affected by Maximize Power? | NO | YES (uses max instead of random) |

### 1.7 Armor Refine DEF Bonus

Each refine adds hard DEF to armor pieces. The formula applies per-piece.

**Formula**: `DEF bonus per piece = floor((3 + refineLevel) / 4)` per each +1 refine

**Cumulative DEF bonus table** (total added DEF at each refine level):

| Refine | +1 | +2 | +3 | +4 | +5 | +6 | +7 | +8 | +9 | +10 |
|--------|----|----|----|----|----|----|----|----|----|----|
| DEF added at this level | +1 | +1 | +1 | +1 | +2 | +2 | +2 | +2 | +3 | +3 |
| Cumulative total DEF | 1 | 2 | 3 | 4 | 6 | 8 | 10 | 12 | 15 | 18 |

**Example**: A +10 Buckler (base 40 DEF) would have 40 + 18 = **58 DEF** total.

This applies to all armor-type equipment: body armor, shield, headgear, garment, and footgear. Each refined piece independently adds its DEF bonus.

### 1.8 Enriched Materials

| Item | ID | Used For | Effect |
|------|-----|----------|--------|
| Enriched Oridecon | 7620 | Weapon Lv 3-4 | Higher refine success rate |
| Enriched Elunium | 7619 | All Armor | Higher refine success rate |
| HD Oridecon | 6240 | Weapon Lv 3-4 | Same rate as Enriched; on fail, -1 refine instead of destruction |
| HD Elunium | 6241 | All Armor | Same rate as Enriched; on fail, -1 refine instead of destruction |

**Obtaining**: Enriched ores come from Cash Shop, events, or special quest rewards. They are NOT normally obtainable through gameplay in pre-renewal Classic.

### 1.9 Skills That Exclude Refine Bonus

Certain skills use custom damage formulas that do not include flat refine ATK:

| Skill | ID | Reason |
|-------|----|--------|
| Shield Boomerang | 1305 | Custom damage: bATK + shieldWeight/10 + refine*5 (uses own refine calc) |
| Acid Terror | 1809 | Custom damage ignoring weapon |
| Investigate | 1606 | Custom damage based on target DEF |
| Asura Strike | 1610 | Custom damage based on SP |
| Sacrifice / Martyr's Reckoning | (Paladin) | Custom damage based on HP |

All other physical skills (Bash, Bowling Bash, Sonic Blow, Pierce, Double Strafe, etc.) DO receive the flat refine ATK bonus.

### 1.10 Blacksmith Weapon Refine Skill (WS_WEAPONREFINE)

Blacksmiths have a unique skill that lets them refine weapons themselves, without visiting a Refine NPC. Key differences from NPC refining:

- Uses the Blacksmith's own DEX and job level to modify success rate
- Each skill level allows refining up to a certain weapon level (Lv 1-4 corresponding to skill levels 1-4)
- Only works on weapons, not armor
- No zeny fee (still consumes ore)
- **Does NOT use enriched ores** -- always uses standard rates

---

## 2. Forging System (Blacksmith)

Blacksmiths can create weapons from raw materials using the Smith skills. Forged weapons bear the crafter's name and can include elemental properties and Star Crumb bonuses.

### 2.1 Forgeable Weapon Types

Only specific weapon categories can be forged. **Bows, Rods, Instruments, Whips, Books, Katars, and Level 4 weapons cannot be forged.**

| Smith Skill | Skill ID | Weapon Type | Max Forgeable Level |
|------------|----------|-------------|-------------------|
| Smith Dagger | 1201 | Daggers | Lv 3 (Gladius) |
| Smith Sword | 1202 | One-Handed Swords | Lv 3 (Katana) |
| Smith Two-Handed Sword | 1203 | Two-Handed Swords | Lv 3 (Bastard Sword) |
| Smith Axe | 1204 | Axes (1H and 2H) | Lv 3 (Two-Handed Axe) |
| Smith Knucklebrace | 1205 | Knuckles / Fists | Lv 3 (Fist) |
| Smith Mace | 1206 | Maces | Lv 3 (Chain) |
| Smith Spear | 1207 | Spears (1H and 2H) | Lv 3 (Lance) |

Each Smith skill has 3 levels. **You cannot forge a weapon whose weapon level exceeds your Smith skill level** (e.g., Smith Dagger Lv 1 can only forge Lv 1 daggers like Knife and Cutter).

### 2.2 Complete Forgeable Weapons & Materials

#### Daggers (Smith Dagger)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Knife | 1 | 17 | 1 Iron, 10 Jellopy | Iron Hammer |
| Cutter | 1 | 30 | 25 Iron | Iron Hammer |
| Main Gauche | 1 | 43 | 50 Iron | Iron Hammer |
| Dirk | 2 | 59 | 17 Steel | Golden Hammer |
| Stiletto | 2 | 47 | 40 Steel | Golden Hammer |
| Gladius | 3 | 60 | 40 Steel, 4 Oridecon, 1 Sapphire | Oridecon Hammer |

#### One-Handed Swords (Smith Sword)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Sword | 1 | 25 | 2 Iron | Iron Hammer |
| Falchion | 2 | 49 | 25 Steel | Golden Hammer |
| Blade | 1 | 53 | 45 Iron | Iron Hammer |
| Rapier | 2 | 70 | 20 Steel | Golden Hammer |
| Scimitar | 2 | 85 | 35 Steel | Golden Hammer |
| Katana | 3 | 60 | 50 Steel, 15 Oridecon | Oridecon Hammer |

#### Two-Handed Swords (Smith Two-Handed Sword)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Slayer | 2 | 60 | 25 Steel | Golden Hammer |
| Bastard Sword | 2 | 115 | 45 Steel | Golden Hammer |
| Two-Handed Sword | 3 | 160 | 12 Oridecon, 10 Steel | Oridecon Hammer |

#### Axes (Smith Axe)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Axe | 1 | 38 | 10 Iron | Iron Hammer |
| Battle Axe | 2 | 80 | 110 Iron, 25 Steel | Golden Hammer |
| Hammer (weapon) | 2 | 120 | 30 Steel | Golden Hammer |
| Buster | 3 | 155 | 25 Oridecon, 10 Steel | Oridecon Hammer |
| Two-Handed Axe | 3 | 185 | 25 Oridecon, 10 Steel | Oridecon Hammer |

#### Knuckles (Smith Knucklebrace)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Waghnak | 1 | 30 | 20 Iron, 15 Claw of Wolves | Iron Hammer |
| Knuckle Duster | 2 | 50 | 30 Steel | Golden Hammer |
| Hora (Finger) | 3 | 115 | 20 Oridecon, 10 Steel | Oridecon Hammer |
| Fist | 3 | 115 | 20 Oridecon, 10 Steel, 1 Ruby | Oridecon Hammer |

#### Maces (Smith Mace)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Club | 1 | 23 | 3 Iron | Iron Hammer |
| Mace | 1 | 37 | 30 Iron | Iron Hammer |
| Smasher | 1 | 40 | 40 Iron | Iron Hammer |
| Chain | 2 | 84 | 30 Steel, 1 Sapphire | Golden Hammer |

#### Spears (Smith Spear)

| Weapon | Weapon Lv | ATK | Materials | Hammer |
|--------|----------|-----|-----------|--------|
| Javelin | 1 | 28 | 3 Iron | Iron Hammer |
| Spear | 2 | 44 | 15 Steel | Golden Hammer |
| Pike | 1 | 60 | 45 Iron | Iron Hammer |
| Guisarme | 2 | 84 | 25 Steel | Golden Hammer |
| Glaive | 2 | 94 | 30 Steel | Golden Hammer |
| Partizan | 2 | 104 | 40 Steel | Golden Hammer |
| Trident | 2 | 114 | 45 Steel | Golden Hammer |
| Halberd | 3 | 165 | 25 Oridecon, 10 Steel | Oridecon Hammer |
| Lance | 3 | 185 | 30 Oridecon, 10 Steel | Oridecon Hammer |

**Note**: All forged weapons have **no card slots**. The trade-off is that they can carry elemental properties, Star Crumb bonuses, and the crafter's name.

### 2.3 Base Success Formula

The forging success rate is calculated as follows (from rAthena `skill.cpp`):

```
base_make_per = 5000 + (JobLevel * 20) + (DEX * 10) + (LUK * 10)
```

This base rate (out of 10000 = 100%) is then modified by:

#### Skill Bonuses

```
+ Smith [Weapon] Skill Level * 500    (e.g., Smith Sword Lv 3 = +1500)
+ Weaponry Research Level * 100       (passive, Lv 1-10 = +100 to +1000)
```

#### Weapon Level Penalty

```
- (Weapon Level - 1) * 1000
  Lv 1 weapon: -0
  Lv 2 weapon: -1000
  Lv 3 weapon: -2000
```

#### Full Formula

```
success_rate = 5000
             + (JobLevel * 20)
             + (DEX * 10)
             + (LUK * 10)
             + (SmithSkillLevel * 500)
             + (WeaponryResearchLevel * 100)
             - ((WeaponLevel - 1) * 1000)
             + AnvilBonus
             - ElementPenalty
             - (StarCrumbCount * 1500)
```

Divided by 100 to get percentage. Capped at 0% minimum (negative rates just mean failure).

**Example**: Job Lv 50, 99 DEX, 60 LUK, Smith Sword Lv 3, Weaponry Research Lv 10, forging a Lv 3 Katana with Golden Anvil:
```
= 5000 + (50*20) + (99*10) + (60*10) + (3*500) + (10*100) - (2*1000) + 500
= 5000 + 1000 + 990 + 600 + 1500 + 1000 - 2000 + 500
= 8590 / 10000 = 85.9%
```

### 2.4 Anvil Types and Bonuses

An Anvil is required in inventory to forge. Different anvil types give different success bonuses.

| Anvil | ID | Success Bonus | Where to Get |
|-------|-----|--------------|-------------|
| Anvil | 986 | +0 (base) | NPC shop |
| Oridecon Anvil | 987 | +300 (+3%) | NPC shop (expensive) |
| Golden Anvil | 988 | +500 (+5%) | Rare drop / event |
| Emperium Anvil | 989 | +1000 (+10%) | Very rare drop |

**Hammer requirement**: The correct hammer must also be in inventory:
- **Iron Hammer** (ID 1005) -- for weapon Lv 1
- **Golden Hammer** (ID 1006) -- for weapon Lv 2
- **Oridecon Hammer** (ID 1007) -- for weapon Lv 3

Both the anvil and hammer are consumed on each forge attempt (success or failure).

### 2.5 Element Stones (Modify Weapon Element)

One of the 3 forging material slots can hold an elemental stone to imbue the forged weapon with an element.

| Element Stone | ID | Weapon Element | Source |
|--------------|-----|---------------|--------|
| Flame Heart | 994 | Fire | 10 Red Blood -> 1 Flame Heart (NPC) |
| Mystic Frozen | 995 | Water | 10 Crystal Blue -> 1 Mystic Frozen (NPC) |
| Rough Wind | 996 | Wind | 10 Wind of Verdure -> 1 Rough Wind (NPC) |
| Great Nature | 997 | Earth | 10 Green Live -> 1 Great Nature (NPC) |

**Rules**:
- Only ONE elemental stone can be used per forge
- Using more than one elemental stone causes automatic failure
- Adds a **-2000 penalty** (-20%) to success rate
- The forged weapon becomes permanently that element (cannot be changed)
- Element is reflected in the weapon name (e.g., "Fire Main Gauche")

### 2.6 Star Crumbs (Bonus ATK)

Star Crumbs add permanent bonus mastery ATK to the forged weapon. Up to 3 can be used per forge.

| Star Crumbs Used | Bonus Mastery ATK | Success Rate Penalty | Name Prefix |
|-----------------|-------------------|---------------------|-------------|
| 1 | **+5** ATK | -1500 (-15%) | "Very Strong" |
| 2 | **+10** ATK | -3000 (-30%) | "Very Very Strong" |
| 3 | **+40** ATK | -4500 (-45%) | "Very Very Very Strong" |

**Important**: The mastery ATK from Star Crumbs is flat bonus damage added **after DEF** (like mastery bonuses), making it particularly valuable. The jump from 2 to 3 Star Crumbs (+10 to +40) makes 3-Star Crumb weapons disproportionately powerful.

**Star Crumb crafting**: 10 Star Dust (monster drop) -> 1 Star Crumb (NPC conversion).

### 2.7 "Very Very Very Strong" Naming System

Forged weapons have a special naming convention that reflects their properties:

**Full name format**: `[Star Crumb Prefix] [Crafter Name]'s [Element] [Weapon Name]`

| Components | Example Name |
|-----------|-------------|
| No additions | "CrafterName's Blade" |
| 1 Star Crumb | "Very Strong CrafterName's Blade" |
| 2 Star Crumbs | "Very Very Strong CrafterName's Blade" |
| 3 Star Crumbs | "Very Very Very Strong CrafterName's Blade" |
| Fire element | "CrafterName's Fire Blade" |
| 2 Stars + Fire | "Very Very Strong CrafterName's Fire Blade" |
| 1 Star + Fire | "Very Strong CrafterName's Fire Blade" |

**Note**: 3 Star Crumbs + 1 Element Stone is NOT possible (only 3 material slots exist, and 3 Stars fill all 3).

**Maximum possible combinations per weapon**:
- 0 Stars + 0 Element (neutral, crafter name only)
- 1 Star + 0 Element (Very Strong neutral)
- 2 Stars + 0 Element (Very Very Strong neutral)
- 3 Stars + 0 Element (Very Very Very Strong neutral)
- 0 Stars + 1 Element (elemental)
- 1 Star + 1 Element (Very Strong elemental)
- 2 Stars + 1 Element (Very Very Strong elemental)

= **7 possible configurations** per forgeable weapon.

### 2.8 Forging Failure

When forging fails:
- All materials are **consumed** (iron/steel/oridecon, element stone, star crumbs, hammer, anvil)
- No weapon is produced
- The Blacksmith receives a failure message
- No penalty to the Blacksmith character

---

## 3. Pharmacy / Brewing System (Alchemist)

Alchemists use the **Pharmacy** skill (ID 228) to brew potions, bombs, and other consumables. A corresponding **manual/guide book** must be in inventory (not consumed).

### 3.1 Required Manuals

| Manual | ID | Recipes Unlocked |
|--------|-----|-----------------|
| Potion Creation Guide | 7144 | Red Potion, Yellow Potion, White Potion, Blue Potion, Alcohol |
| Bottle Grenade Creation Guide | 7134 | Acid Bottle, Bottle Grenade |
| Condensed Potion Creation Guide / Slim Potion Create Book | 7133 | Condensed Red Potion, Condensed Yellow Potion, Condensed White Potion |
| Embryo Creation Guide | 7135 | Embryo |
| Coat of Arms | 7136 | Glistening Coat |
| Elemental Converter Guide | (varies) | Elemental converters (Fire/Water/Wind/Earth) |

All manuals are quest rewards or NPC purchases. They are NOT consumed during crafting.

### 3.2 Complete Recipe List

#### Potions (require Potion Creation Guide)

| Product | ID | Ingredients | Manual Required |
|---------|-----|------------|----------------|
| Red Potion | 501 | 1 Red Herb, 1 Empty Potion Bottle | Potion Creation Guide |
| Yellow Potion | 503 | 1 Yellow Herb, 1 Empty Potion Bottle | Potion Creation Guide |
| White Potion | 504 | 1 White Herb, 1 Empty Potion Bottle | Potion Creation Guide |
| Blue Potion | 505 | 1 Blue Herb, 1 Scell, 1 Empty Potion Bottle | Potion Creation Guide |
| Alcohol | 970 | 5 Stem, 5 Poison Spore, 1 Empty Bottle, 1 Empty Test Tube | Potion Creation Guide |

**All brewing attempts also consume 1 Medicine Bowl (ID 7134).**

#### Bombs and Bottles (require Bottle Grenade Creation Guide)

| Product | ID | Ingredients |
|---------|-----|------------|
| Acid Bottle | 7135 | 1 Immortal Heart, 1 Empty Bottle |
| Bottle Grenade | 7136 | 1 Alcohol, 1 Fabric, 1 Empty Bottle |

#### Condensed/Slim Potions (require Condensed Potion Creation Guide)

| Product | ID | Ingredients |
|---------|-----|------------|
| Condensed Red Potion | 545 | 1 Red Potion, 1 Empty Test Tube, 1 Witch Starsand |
| Condensed Yellow Potion | 546 | 1 Yellow Potion, 1 Empty Test Tube, 1 Witch Starsand |
| Condensed White Potion | 547 | 1 White Potion, 1 Empty Test Tube, 1 Witch Starsand |

#### Misc Brews

| Product | ID | Ingredients | Manual |
|---------|-----|------------|--------|
| Anodyne | 605 | 1 Alcohol, 1 Ment, 1 Empty Bottle | Potion Creation Guide |
| Aloevera | 606 | 1 Alcohol, 1 Aloe, 1 Empty Bottle | Potion Creation Guide |
| Embryo | 7142 | 1 Seed of Life, 1 Morning Dew of Yggdrasil, 1 Glass Tube | Embryo Creation Guide |
| Glistening Coat | 7210 | 1 Zenorc's Fang, 1 Alcohol, 1 Empty Bottle | Coat of Arms |

#### Elemental Converters

| Product | ID | Ingredients |
|---------|-----|------------|
| Fire Converter | 12114 | 3 Scorpion Tail |
| Water Converter | 12115 | 3 Snail's Shell |
| Wind Converter | 12116 | 3 Horn |
| Earth Converter | 12117 | 3 Rainbow Shell |

### 3.3 Success Rate Formula

The brewing success formula (from rAthena `skill.cpp`):

```
base_make_per = (Pharmacy Skill Level * 300)
              + (Learning Potion / Potion Research Skill Level * 50)
              + (Job Level * 20)
              + ((INT / 2) * 10)
              + (DEX * 10)
              + (LUK * 10)
```

Then modified per potion type:

| Potion Category | Rate Modifier |
|----------------|---------------|
| Red/Yellow/White Potions | base + 2000 + rnd(10, 1000) |
| Blue Potion | base + 1000 + rnd(10, 1000) |
| Alcohol | base + 1000 + rnd(10, 1000) |
| Acid Bottle, Bottle Grenade | base + rnd(10, 1000) |
| Condensed Red/Yellow Potion | base + rnd(10, 500) |
| Condensed White Potion | base - rnd(10, 1000) |
| Embryo | base - rnd(10, 2000) |

Rate is out of 10000 (divide by 100 for percentage).

**Potion Research (Learning Potion) passive** (skill ID 228): Each level adds +50 to base rate AND +5% to all potion healing effects as a separate bonus.

### 3.4 Potion Creation Guide Book Bonus

Having the **Potion Creation Guide** in inventory is required for basic potions. The guide itself provides no additional success bonus beyond unlocking the recipes. However, the **Potion Research / Learning Potion** passive skill (separate from the guide) does increase success rates.

### 3.5 Homunculus Bonus

If the Alchemist has a Vanilmirth-type homunculus with the **Change Instruction** skill, it adds:

```
+ (Change Instruction Level * 100) to base_make_per
```

This can provide up to +500 (+5%) at max level.

### 3.6 Brewing Failure

When brewing fails:
- All ingredient materials are consumed
- The Medicine Bowl is consumed
- No product is created
- Manual is NOT consumed (reusable)

---

## 4. Arrow Crafting

Arrow Crafting (Skill ID 305) is an Archer quest skill that converts items in inventory into arrows. It is NOT level-based -- there is only 1 level.

### 4.1 Rules

- **Weight limit**: Cannot be used if the character is at **50% weight or more**
- **No success rate**: Arrow Crafting always succeeds (100%)
- **No level requirement**: Available to all Archer-class characters after completing the quest
- **Converts 1 item** at a time into a specific arrow type and quantity

### 4.2 Complete Arrow Crafting Recipe List

#### Basic Arrows (Arrow, ID 1750)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Jellopy | 909 | 4 |
| Worm Peeling | 939 | 3 |
| Resin | 907 | 3 |
| Feather | 940 | 2 |
| Tooth of Bat | 7001 | 2 |
| Chonchon Doll | 904 | 3 |
| Tree Root | 902 | 7 |
| Claw of Wolves | 916 | 5 |
| Insect Feeler | 914 | 5 |
| Shell | 935 | 10 |
| Log (Wooden Block) | 1020 | 20 |
| Trunk | 1019 | 40 |
| Solid Trunk | 7063 | 15 |

#### Iron Arrows (Iron Arrow, ID 1770)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Iron | 998 | 100 |
| Phracon | 1010 | 50 |

#### Steel Arrows (Steel Arrow, ID 1753)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Steel | 999 | 100 |
| Emveretarcon | 1011 | 50 |
| Elunium | 985 | 1000 |

#### Oridecon Arrows (Oridecon Arrow, ID 1765)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Oridecon | 984 | 250 |

#### Fire Arrows (Fire Arrow, ID 1752)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Red Blood | 990 | 600 |
| Flame Heart | 994 | 1800 |
| Burning Heart | 7097 | 30 |

#### Crystal / Water Arrows (Crystal Arrow, ID 1754)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Crystal Blue | 991 | 150 |
| Mystic Frozen | 995 | 450 |

#### Wind Arrows (Wind Arrow, ID 1755)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Wind of Verdure | 992 | 150 |
| Rough Wind | 996 | 450 |

#### Stone / Earth Arrows (Stone Arrow, ID 1756)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Green Live | 993 | 150 |
| Great Nature | 997 | 450 |

#### Silver / Holy Arrows (Silver Arrow, ID 1751)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Holy Water | 912 | 3 |

#### Holy Arrows (Holy Arrow, ID 1772)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Valhala's Flower | 7510 | 600 |

#### Arrow of Counter Evil (ID 1766)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Rosary in Mouth | 7340 | 150 |

#### Immaterial / Ghost Arrows (Immaterial Arrow, ID 1757)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Emperium | 714 | 600 |

#### Shadow Arrows (Shadow Arrow, ID 1767)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Dark Crystal Fragment | 7015 | 100 |

#### Rusty / Poison Arrows (Rusty Arrow, ID 1762)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Venom Canine | 937 | 30 |
| Poison Spore | 936 | 10 |
| Bee Sting | 7033 | 100 |

#### Status Effect Arrows

| Arrow Type | Arrow ID | Source Item | Source ID | Qty |
|-----------|---------|------------|----------|-----|
| Stun Arrow | 1758 | Phracon Fragment | 7014 | 150 |
| Frozen Arrow | 1759 | Snowy Fragment | 7066 | 150 |
| Flash Arrow (blind) | 1760 | Light Granule | 7031 | 100 |
| Cursed Arrow | 1761 | Dark Granule | 7030 | 100 |
| Sleep Arrow | 1768 | Moth Dust | 7018 | 150 |
| Mute Arrow (silence) | 1769 | Mute Fragment | 7032 | 100 |

#### Sharp Arrows (CRIT boost, ID 1764)

| Source Item | ID | Arrows Produced |
|------------|-----|----------------|
| Horrendous Mouth | 947 | 100 |
| Talon of Griffon | 7053 | 150 |

### 4.3 Arrow Properties Summary

| Arrow | ID | Element | Special Effect |
|-------|-----|---------|---------------|
| Arrow | 1750 | Neutral | Basic arrow |
| Silver Arrow | 1751 | Holy | Effective vs Undead/Demon |
| Fire Arrow | 1752 | Fire | Effective vs Earth/Undead |
| Steel Arrow | 1753 | Neutral | Higher ATK |
| Crystal Arrow | 1754 | Water | Effective vs Fire |
| Wind Arrow | 1755 | Wind | Effective vs Water |
| Stone Arrow | 1756 | Earth | Effective vs Wind |
| Immaterial Arrow | 1757 | Ghost | Effective vs Ghost |
| Stun Arrow | 1758 | Neutral | Chance to stun |
| Frozen Arrow | 1759 | Water | Chance to freeze |
| Flash Arrow | 1760 | Neutral | Chance to blind |
| Cursed Arrow | 1761 | Neutral | Chance to curse |
| Rusty Arrow | 1762 | Poison | Chance to poison |
| Sharp Arrow | 1764 | Neutral | CRI +20 |
| Oridecon Arrow | 1765 | Neutral | High ATK |
| Arrow of Counter Evil | 1766 | Holy | Anti-undead |
| Shadow Arrow | 1767 | Shadow | Effective vs Holy |
| Sleep Arrow | 1768 | Neutral | Chance to sleep |
| Mute Arrow | 1769 | Neutral | Chance to silence |
| Iron Arrow | 1770 | Neutral | Medium ATK |
| Holy Arrow | 1772 | Holy | Anti-undead |

---

## 5. Cooking System

The Cooking system exists in pre-renewal RO but is relatively late content. Players craft stat-boosting food items using gathered ingredients and Cookbooks.

### 5.1 Prerequisites

- Must be **Base Level 50** or higher
- Must complete the **Cooking Quest** (NPC: Madeleine Chu and Charles Orleans in Prontera Castle)
- Must have a **Cooking Kit** (purchased from Madeleine Chu after quest completion)
- Must have the correct **level Cookbook** in inventory

### 5.2 Cookbooks

| Cookbook | Stat Food Level | How to Obtain |
|---------|----------------|--------------|
| Level 1 Cookbook | +1 to +2 foods | Charles Orleans NPC trade |
| Level 2 Cookbook | +2 to +3 foods | Charles Orleans NPC trade |
| Level 3 Cookbook | +3 to +4 foods | Charles Orleans NPC trade |
| Level 4 Cookbook | +4 to +5 foods | Charles Orleans NPC trade |
| Level 5 Cookbook | +5 to +6 foods | Charles Orleans NPC trade |
| Level 6 Cookbook | +6 to +7 foods | Monster drop only |
| Level 7 Cookbook | +7 to +8 foods | Monster drop only |
| Level 8 Cookbook | +8 to +9 foods | Monster drop only |
| Level 9 Cookbook | +9 foods | Monster drop only |
| Level 10 Cookbook | +10 foods | Monster drop only |

Cookbooks Lv 1-5 are obtained by trading food items to Charles Orleans. Cookbooks Lv 6-10 drop from monsters.

### 5.3 Stat Food Effects

Each cooked food provides a temporary stat boost. Duration is typically **20 minutes** for all stat foods.

| Stat | Food Names (by level) |
|------|----------------------|
| STR | Fried Grasshopper Legs (+1) through Dragon Breath Cocktail (+10) |
| AGI | Grape Juice Herbal Tea (+1) through Nine Tail Dish (+10) |
| VIT | Steamed Crab Nippers (+1) through Immortal Stew (+10) |
| INT | Spicy Fried Bao (+1) through Hwergelmir's Elixir (+10) |
| DEX | Frog Egg and Squid Ink Soup (+1) through Drosera Herb Salad (+10) |
| LUK | Fried Monkey Tails (+1) through Cooked Nine Tail Dish (+10) |

**Stacking rules**: If several food items of the **same stat** are consumed, only the **highest level** one takes effect. Consuming a lower-level food while a higher-level one is active has no effect.

### 5.4 Cooking Success

Cooking success depends on:
- Character's **DEX** and **LUK** stats
- The **level of the food** being cooked (higher-level foods are harder)
- Having the correct Cookbook in inventory

On failure, ingredients are consumed but no food is produced.

### 5.5 Implementation Priority

Cooking is a **lower priority** system for the Sabri_MMO project. It requires:
- A quest chain (Cooking Quest)
- 10 cookbook items (5 from NPC, 5 from monster drops)
- 60 food recipes (6 stats x 10 levels)
- Ingredient sourcing from various monsters

This can be deferred until core systems are stable.

---

## 6. Implementation Checklist

### 6.1 Refining System

| Feature | Status | Notes |
|---------|--------|-------|
| Refine NPC interaction | IMPLEMENTED | Server handler `refine:request` |
| Normal ore success rates | IMPLEMENTED | REFINE_RATES table in index.js |
| Safe limits (+4/+5/+6/+7 / +4 armor) | IMPLEMENTED | SAFE_LIMITS in index.js |
| Failure = item destruction | IMPLEMENTED | Item removed from inventory on fail |
| Flat refine ATK per weapon level | IMPLEMENTED | +2/+3/+5/+7 in damage pipeline (post-DEF) |
| Overupgrade random bonus | IMPLEMENTED | OVER_BONUS table, rnd(1, max) per hit |
| Armor refine DEF bonus | IMPLEMENTED | floor((3+rl)/4) per piece |
| Refine ATK shown in status window | IMPLEMENTED | Right-side ATK in stats payload |
| Enriched Oridecon/Elunium | NOT IMPLEMENTED | Higher success rates, Cash Shop item |
| HD Oridecon/Elunium | NOT IMPLEMENTED | Safe fail (-1 refine), event-only |
| Blacksmith Weapon Refine skill | NOT IMPLEMENTED | Self-refine with own stats |
| Card loss on destruction | IMPLEMENTED | Cards destroyed with item |
| Zeny fee per attempt | IMPLEMENTED | Fee deducted per attempt |
| Dual-wield left-hand refine | IMPLEMENTED | Separate refineATKLeft for Assassin |

### 6.2 Forging System

| Feature | Status | Notes |
|---------|--------|-------|
| Forge handler (forge:request) | IMPLEMENTED | Full forge flow in index.js |
| Forge success formula | IMPLEMENTED | DEX + LUK + JobLv + skill bonuses |
| Forgeable weapon recipes | IMPLEMENTED | FORGE_RECIPES in index.js |
| Element stones | IMPLEMENTED | Flame Heart / Mystic Frozen / Rough Wind / Great Nature |
| Star Crumbs (+5/+10/+40 mastery ATK) | IMPLEMENTED | Star crumb bonus stored on weapon |
| Element stone penalty (-20%) | IMPLEMENTED | -2000 to make_per |
| Star Crumb penalty (-15% each) | IMPLEMENTED | -1500 per crumb |
| Weapon level penalty | IMPLEMENTED | -(wlv-1)*1000 |
| Anvil types and bonuses | PARTIALLY | Basic anvil; check if all 4 types work |
| Hammer consumption | IMPLEMENTED | Correct hammer type required |
| Crafter name on weapon | IMPLEMENTED | forged_by column in DB |
| Very/Very Very/Very Very Very Strong naming | IMPLEMENTED | Client displays prefix based on star count |
| Smith skill level gate | IMPLEMENTED | Cannot forge above skill level |

### 6.3 Pharmacy / Brewing

| Feature | Status | Notes |
|---------|--------|-------|
| Pharmacy skill handler | PARTIALLY | Basic potions work, check full recipe list |
| Potion Creation Guide requirement | CHECK | Verify manual-in-inventory check |
| Success rate formula | IMPLEMENTED | INT + DEX + LUK + JobLv based |
| Red/Yellow/White/Blue Potion recipes | IMPLEMENTED | Basic potions |
| Alcohol, Acid Bottle, Bottle Grenade | IMPLEMENTED | Bomb ingredients |
| Condensed Potions | CHECK | May not be fully implemented |
| Embryo creation | IMPLEMENTED | For homunculus system |
| Anodyne / Aloevera | CHECK | Verify recipes exist |
| Elemental converters | CHECK | Verify Pharmacy-based creation |
| Potion Research passive bonus | IMPLEMENTED | +5% heal bonus per level |
| Homunculus (Vanilmirth) bonus | NOT IMPLEMENTED | Change Instruction bonus to brew rate |
| Medicine Bowl consumption | CHECK | Verify consumed per attempt |

### 6.4 Arrow Crafting

| Feature | Status | Notes |
|---------|--------|-------|
| Arrow Crafting skill handler | IMPLEMENTED | `ro_arrow_crafting.js` |
| Complete recipe list | IMPLEMENTED | 45 recipes in ARROW_CRAFTING_RECIPES |
| Weight limit check (50%) | IMPLEMENTED | Server validates weight |
| 100% success rate | IMPLEMENTED | Always succeeds |
| Inventory refresh after crafting | IMPLEMENTED | Client UI updates |

### 6.5 Cooking System

| Feature | Status | Notes |
|---------|--------|-------|
| Cooking Quest | NOT IMPLEMENTED | Quest chain for prerequisite |
| Cookbook items (1-10) | NOT IMPLEMENTED | 10 cookbook items |
| Cooking Kit | NOT IMPLEMENTED | Required consumable |
| Stat food recipes (60) | NOT IMPLEMENTED | 6 stats x 10 levels |
| Success rate formula | NOT IMPLEMENTED | DEX + LUK based |
| Stat food buff application | PARTIALLY | sc_start handler can apply stat buffs; food items need to trigger it |
| Food stacking rules | NOT IMPLEMENTED | Highest level wins per stat |

---

## 7. Gap Analysis

### 7.1 Critical Gaps (Core Gameplay Impact)

| Gap | Priority | Effort | Description |
|-----|----------|--------|-------------|
| Enriched Oridecon/Elunium | MEDIUM | Small | Add enriched ore items + separate success rate table. Only matters if Cash Shop exists. |
| Blacksmith Weapon Refine | LOW | Medium | Self-refine skill. Low priority since NPC refine covers same functionality. |
| Condensed Potion verification | HIGH | Small | Verify Slim/Condensed potion recipes are in the pharmacy handler. These are core Alchemist products used in WoE. |

### 7.2 Completeness Gaps

| Gap | Priority | Effort | Description |
|-----|----------|--------|-------------|
| Anvil type verification | LOW | Tiny | Verify all 4 anvil types (Anvil, Oridecon, Golden, Emperium) with correct bonuses |
| Forge recipe completeness | MEDIUM | Small | Cross-check all weapon recipes against rAthena produce_db |
| Pharmacy recipe completeness | MEDIUM | Small | Verify Anodyne, Aloevera, Glistening Coat, elemental converters |
| Medicine Bowl consumption | LOW | Tiny | Verify Medicine Bowl consumed per pharmacy attempt |
| Vanilmirth brew bonus | LOW | Small | Change Instruction skill bonus to pharmacy success |

### 7.3 Deferred Systems (Future Content)

| System | Priority | Effort | Description |
|--------|----------|--------|-------------|
| Cooking system | LOW | Large | Full cooking quest, 10 cookbooks, 60 recipes, ingredients. Defer until core systems stable. |
| HD Oridecon/Elunium | VERY LOW | Small | Safe-fail ores. Event-only content, not needed for core game. |
| Socket Enchant NPC | MEDIUM | Medium | NPC that adds card slots to certain equipment. Separate system from refining. |

### 7.4 Verified Working (No Gaps)

- Refine success rates (normal ores, all weapon levels + armor)
- Safe limits per weapon level
- Flat refine ATK (+2/+3/+5/+7) post-DEF in damage pipeline
- Overupgrade random bonus pre-DEF
- Armor refine DEF bonus formula
- Refine exclude skills list (Shield Boomerang, Acid Terror, Investigate, Asura Strike)
- Forge success formula with all modifiers
- Element stone + Star Crumb forging
- Arrow Crafting (45 recipes, weight check, 100% success)
- Dual-wield left-hand refine ATK

---

## Sources

- [RateMyServer - Weapon and Armor Refine Success Rates](https://ratemyserver.net/index.php?page=misc_table_refine)
- [iRO Wiki Classic - Refinement System](https://irowiki.org/classic/Refinement_System)
- [iRO Wiki - Refinement System](https://irowiki.org/wiki/Refinement_System)
- [rAthena Pre-Renewal refine.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/refine.yml)
- [rAthena skill.cpp (forging/pharmacy formulas)](https://github.com/rathena/rathena/blob/master/src/map/skill.cpp)
- [iRO Wiki Classic - Forging](https://irowiki.org/classic/Forging)
- [iRO Wiki - Forging](https://irowiki.org/wiki/Forging)
- [RateMyServer - Forging Calculator](https://ratemyserver.net/forge_calc.php)
- [RateMyServer - Blacksmith Weapon Forging Guide](https://ratemyserver.net/index.php?page=creation_db&op=3)
- [Revival-RO Wiki - Weapon Forging Guide](https://revivalro.fandom.com/wiki/Weapon_Forging_Guide)
- [iRO Wiki - Potion Creation](https://irowiki.org/wiki/Potion_Creation)
- [RateMyServer - Brewing Calculator](https://ratemyserver.net/brew_calc.php)
- [RateMyServer - Alchemist Potion Making Guide](https://ratemyserver.net/index.php?page=creation_db&op=4)
- [Ragnarok Wiki (Fandom) - Alchemy](https://ragnarok.fandom.com/wiki/Alchemy)
- [iRO Wiki Classic - Arrow Crafting](https://irowiki.org/classic/Arrow_Crafting)
- [RateMyServer - Arrow Making Guide](https://ratemyserver.net/index.php?op=1&page=creation_db)
- [iRO Wiki Classic - Cooking](https://irowiki.org/classic/Cooking)
- [RateMyServer - Cooking & Recipe Guide](https://ratemyserver.net/index.php?page=creation_db&op=2)
- [ROClassicGuide - Forging Guide: How Strong are Very Very Strong Weapons?](https://roclassic-guide.com/forging-guide-strong-strong-weapons/)
- [GameFAQs - Forging Merchant/Blacksmith Guide](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/28512)
- [WarpPortal Forums - Enriched Refinement Rates Discussion](https://forums.warpportal.com/index.php?/topic/218424-enriched-refinement-rates-classic-or-renewal/)
- [rAthena Forums - Refine Rate for Pre-Renewal](https://rathena.org/board/topic/96609-refine-rate-for-pre-renewal/)
- [iRO Wiki - ATK](https://irowiki.org/wiki/ATK)
- [Ragnarok Wiki (Fandom) - Blacksmithing](https://ragnarok.fandom.com/wiki/Blacksmithing)
