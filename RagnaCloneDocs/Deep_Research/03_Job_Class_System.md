# Job/Class System -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-Renewal) job/class system only. No 3rd classes, no 4th classes, no Renewal-era changes.
> **Sources**: iRO Wiki Classic, RateMyServer (pre-re), rAthena source (pre-re branch), StrategyWiki, divine-pride.net, iRO Wiki, Ragnarok Fandom Wiki, HubPages quest guides, GameFAQs, ROGGH Library.
> **Purpose**: Authoritative reference for implementing the complete job/class system in Sabri_MMO.

---

## Table of Contents

1. [Overview (Class Tree Diagram)](#1-overview)
2. [Complete Class List](#2-complete-class-list)
3. [Novice to 1st Class Paths](#3-novice-to-1st-class-paths)
4. [1st Class to 2nd Class Paths](#4-1st-class-to-2nd-class-paths)
5. [Transcendent/Rebirth System](#5-transcendentrebirth-system)
6. [Super Novice Mechanics](#6-super-novice-mechanics)
7. [Job Change Requirements](#7-job-change-requirements)
8. [Class-Specific Stats (HP/SP Growth Tables)](#8-class-specific-stats)
9. [Job Bonuses (Stat Bonuses Per Job Level)](#9-job-bonuses)
10. [Weapon Restrictions](#10-weapon-restrictions)
11. [Armor Restrictions](#11-armor-restrictions)
12. [Skill Point Allocation](#12-skill-point-allocation)
13. [Equipment Restrictions by Class](#13-equipment-restrictions-by-class)
14. [Baby Classes](#14-baby-classes)
15. [Edge Cases and Special Rules](#15-edge-cases-and-special-rules)
16. [Transcendent-Exclusive Skills](#16-transcendent-exclusive-skills)
17. [Implementation Checklist](#17-implementation-checklist)
18. [Gap Analysis](#18-gap-analysis)

---

## 1. Overview

### 1.1 Class Tree Diagram (Pre-Renewal)

```
                                    ┌─────────┐
                                    │ NOVICE  │
                                    │ Job 1-10│
                                    └────┬────┘
                                         │
              ┌──────────┬──────────┬─────┴─────┬──────────┬──────────┐
              │          │          │           │          │          │
         ┌────┴────┐┌────┴────┐┌───┴───┐ ┌────┴────┐┌───┴───┐┌────┴────┐
         │SWORDMAN ││  MAGE   ││ARCHER │ │MERCHANT ││ THIEF ││ACOLYTE  │
         │Job 1-50 ││Job 1-50 ││Job1-50│ │Job 1-50 ││Job1-50││Job 1-50 │
         └──┬──┬───┘└──┬──┬───┘└──┬──┬─┘ └──┬──┬───┘└──┬──┬┘└──┬──┬───┘
            │  │       │  │       │  │       │  │       │  │     │  │
   ┌────────┘  └───┐  ┌┘  └──┐  ┌┘  └──┐  ┌┘  └──┐  ┌┘  └─┐  ┌┘  └──┐
   │    (2-1) (2-2)│  │      │  │      │  │      │  │      │  │      │
┌──┴──┐ ┌────┴──┐ ┌┴──┴┐┌───┴┐┌┴───┐┌─┴──┐┌──┴──┐┌┴───┐┌─┴──┐┌──┴──┐┌─┴──┐
│KNIGT│ │CRUSDR │ │WIZ ││SAGE││HUNT││B/D ││BLKSM││ALCH││ASSN││ROGUE││PRS │ │MONK│
│Jb50 │ │Job 50 │ │J50 ││J50 ││J50 ││J50 ││J 50 ││J50 ││J50 ││J 50 ││J50 │ │J50 │
└──┬──┘ └───┬───┘ └─┬──┘└──┬─┘└─┬──┘└─┬──┘└──┬──┘└──┬─┘└─┬──┘└──┬──┘└─┬──┘ └─┬──┘
   │        │       │      │     │     │      │      │     │      │     │      │
   ▼        ▼       ▼      ▼     ▼     ▼      ▼      ▼     ▼      ▼     ▼      ▼
  [== Reach Base Lv 99 + Job Lv 50 ==> REBIRTH as High Novice ==]
   │        │       │      │     │     │      │      │     │      │     │      │
┌──┴──┐ ┌───┴───┐ ┌─┴──┐┌──┴─┐┌─┴──┐┌─┴──┐┌──┴──┐┌──┴─┐┌─┴──┐┌──┴──┐┌─┴──┐┌─┴──┐
│ LK  │ │PALDIN │ │ HW ││SCHL││SNPR││MNST││ WS  ││BIOC││ SX ││STLK ││ HP │ │CHMP│
│Jb70 │ │Job 70 │ │J70 ││J70 ││J70 ││GYPS││J 70 ││J70 ││J70 ││J 70 ││J70 │ │J70 │
└─────┘ └───────┘ └────┘└────┘└────┘└────┘└─────┘└────┘└────┘└─────┘└────┘ └────┘
```

**Alternative Path from Novice:**

```
         Novice (Base Lv 45+, Job Lv 10)
              │
         ┌────┴────┐
         │ SUPER   │
         │ NOVICE  │
         │Job 1-99 │
         └─────────┘
```

### 1.2 Class Tier System

| Tier | Name | Job Level Cap | Notes |
|------|------|--------------|-------|
| 0 | Novice | 10 | Starting class |
| 0 | High Novice | 10 | Post-rebirth Novice, 100 stat points |
| 0 | Super Novice | 99 | Special branch from Novice |
| 1 | First Class | 50 | 6 options (Swordman/Mage/Archer/Merchant/Thief/Acolyte) |
| 1 | High First Class | 50 | Post-rebirth 1st class (same skills, different sprite) |
| 2 | Second Class (2-1) | 50 | 6 options (Knight/Wizard/Hunter/Blacksmith/Assassin/Priest) |
| 2 | Second Class (2-2) | 50 | 7 options (Crusader/Sage/Bard/Dancer/Alchemist/Rogue/Monk) |
| 3 | Transcendent (Trans 2nd) | 70 | 13 options, 20 extra job levels + exclusive skills |

---

## 2. Complete Class List

### 2.1 All Classes in Pre-Renewal (Total: 40 classes)

#### Novice Tier (3 classes)

| ID | Class | Display Name | Base Lv Cap | Job Lv Cap | Parent |
|----|-------|-------------|-------------|------------|--------|
| 0 | `novice` | Novice | 99 | 10 | -- |
| 23 | `super_novice` | Super Novice | 99 | 99 | novice |
| 4001 | `high_novice` | High Novice | 99 | 10 | -- (rebirth) |

#### First Class Tier (6 classes)

| ID | Class | Display Name | Job Lv Cap | Parent |
|----|-------|-------------|------------|--------|
| 1 | `swordman` | Swordman | 50 | novice |
| 2 | `mage` | Mage | 50 | novice |
| 3 | `archer` | Archer | 50 | novice |
| 4 | `merchant` | Merchant | 50 | novice |
| 5 | `thief` | Thief | 50 | novice |
| 6 | `acolyte` | Acolyte | 50 | novice |

#### High First Class Tier (6 classes)

| ID | Class | Display Name | Job Lv Cap | Parent |
|----|-------|-------------|------------|--------|
| 4002 | `high_swordman` | High Swordman | 50 | high_novice |
| 4003 | `high_mage` | High Mage | 50 | high_novice |
| 4004 | `high_archer` | High Archer | 50 | high_novice |
| 4005 | `high_merchant` | High Merchant | 50 | high_novice |
| 4006 | `high_thief` | High Thief | 50 | high_novice |
| 4007 | `high_acolyte` | High Acolyte | 50 | high_novice |

#### Second Class Tier -- 2-1 Branch (6 classes)

| ID | Class | Display Name | Job Lv Cap | 1st Class |
|----|-------|-------------|------------|-----------|
| 7 | `knight` | Knight | 50 | swordman |
| 9 | `wizard` | Wizard | 50 | mage |
| 11 | `hunter` | Hunter | 50 | archer |
| 10 | `blacksmith` | Blacksmith | 50 | merchant |
| 12 | `assassin` | Assassin | 50 | thief |
| 8 | `priest` | Priest | 50 | acolyte |

#### Second Class Tier -- 2-2 Branch (7 classes)

| ID | Class | Display Name | Job Lv Cap | 1st Class | Gender |
|----|-------|-------------|------------|-----------|--------|
| 14 | `crusader` | Crusader | 50 | swordman | Any |
| 16 | `sage` | Sage | 50 | mage | Any |
| 19 | `bard` | Bard | 50 | archer | Male only |
| 20 | `dancer` | Dancer | 50 | archer | Female only |
| 18 | `alchemist` | Alchemist | 50 | merchant | Any |
| 17 | `rogue` | Rogue | 50 | thief | Any |
| 15 | `monk` | Monk | 50 | acolyte | Any |

#### Transcendent Class Tier (13 classes)

| ID | Class | Display Name | Job Lv Cap | Base 2nd Class | Alt Name |
|----|-------|-------------|------------|----------------|----------|
| 4008 | `lord_knight` | Lord Knight | 70 | knight | -- |
| 4010 | `high_wizard` | High Wizard | 70 | wizard | -- |
| 4012 | `sniper` | Sniper | 70 | hunter | -- |
| 4011 | `whitesmith` | Whitesmith | 70 | blacksmith | Mastersmith |
| 4013 | `assassin_cross` | Assassin Cross | 70 | assassin | -- |
| 4009 | `high_priest` | High Priest | 70 | priest | -- |
| 4015 | `paladin` | Paladin | 70 | crusader | -- |
| 4017 | `scholar` | Scholar | 70 | sage | Professor |
| 4075 | `minstrel` | Minstrel | 70 | bard | Clown |
| 4076 | `gypsy` | Gypsy | 70 | dancer | -- |
| 4019 | `biochemist` | Biochemist | 70 | alchemist | Creator |
| 4018 | `stalker` | Stalker | 70 | rogue | -- |
| 4016 | `champion` | Champion | 70 | monk | -- |

#### Baby Classes (not in scope for initial implementation -- see Section 14)

13 baby classes mirror the normal classes with restrictions. Not typically part of pre-renewal core gameplay.

---

## 3. Novice to 1st Class Paths

### 3.1 Requirements

All first class transitions require:
- **Novice Job Level 10** (all 9 skill points must be allocated into Basic Skill)
- **No minimum Base Level** (though realistically Base Lv ~10+ to survive the quests)
- **Complete class-specific job change quest** at the appropriate guild NPC

### 3.2 Job Change Locations and Quest Details

| First Class | Guild Location | City | Quest Type |
|-------------|---------------|------|-----------|
| Swordman | Swordman Academy | Izlude | Combat trial -- survive a timed arena with monsters |
| Mage | Magic Academy | Geffen | Knowledge quiz + item collection (solutions/gemstones); Job 50 skips items |
| Archer | Archer Village | Payon | Pay registration fee + collect trunks/willows for points; Job 50 skips item part |
| Merchant | Merchant Guild | Alberta | Delivery quest -- deliver parcels between NPCs for vouchers |
| Thief | Thieves Guild | Morroc (Pyramid) | Item collection (mushroom spores, worm peelings, etc.) + pay 10,000 zeny; set varies |
| Acolyte | Church of Prontera | Prontera | Pilgrimage -- visit multiple shrines around the world, purify undead |

### 3.3 On Job Change

When changing from Novice to 1st Class:
- Job Level resets to **1**
- Job EXP resets to **0**
- All skill points from Novice are **lost** (Novice skills are gone)
- Base Level and Base EXP are **preserved**
- Stats and stat points are **preserved**
- Class is set to the new first class
- Equipment that is class-restricted may need to be unequipped

---

## 4. 1st Class to 2nd Class Paths

### 4.1 Requirements

| Transition | From | To (2-1) | To (2-2) | Min Job Lv | Notes |
|-----------|------|----------|----------|------------|-------|
| Swordman branch | Swordman | Knight | Crusader | 40 | Job 50 may skip item collections in quest |
| Mage branch | Mage | Wizard | Sage | 40 | Job 50 Mage skips item collection for Wizard |
| Archer branch | Archer | Hunter | Bard (M) / Dancer (F) | 40 | Bard/Dancer is gender-locked |
| Merchant branch | Merchant | Blacksmith | Alchemist | 40 | -- |
| Thief branch | Thief | Assassin | Rogue | 40 | -- |
| Acolyte branch | Acolyte | Priest | Monk | 40 | -- |

### 4.2 Why Job 50 Matters

- Job Level 40 gives 39 skill points for 1st class skills (levels 2-40)
- Job Level 50 gives 49 skill points for 1st class skills (levels 2-50)
- Changing at Job 40 permanently loses 10 potential skill points
- Some 2nd class quests grant bonuses or skip steps at Job 50
- **Optimal play**: Always reach Job 50 before changing to 2nd class

### 4.3 2nd Class Quest Locations

| Second Class | Quest Location | Quest Type |
|-------------|---------------|-----------|
| Knight | Knight Guild (Prontera) | Combat trial + personality test |
| Wizard | Wizard Academy (Geffen) | Magic knowledge quiz + item collection |
| Hunter | Hunter Guild (Hugel/Payon) | Interview (10 questions, need 90/100) + combat test |
| Blacksmith | Blacksmith Guild (Einbroch/Geffen) | Forging test + item collection |
| Assassin | Assassin Guild (Morroc) | Stealth test + combat trial |
| Priest | Prontera Church | Combat trial against undead + knowledge test |
| Crusader | Crusader Guild (Prontera) | Faith trial + combat test |
| Sage | Sage Academy (Juno) | Academic test + item collection |
| Bard | Bard Guild (Comodo) | Music performance test |
| Dancer | Dancer Guild (Comodo) | Dance performance test |
| Alchemist | Alchemist Guild (Al De Baran) | Potion brewing test + item collection |
| Rogue | Rogue Guild (Comodo) | Stealth + combat test |
| Monk | Monastery (Prontera/St. Capitolina Abbey) | Meditation + combat trial |

### 4.4 On Job Change (1st to 2nd)

When changing from 1st Class to 2nd Class:
- Job Level resets to **1**
- Job EXP resets to **0**
- **1st class skill points are NOT lost** -- 1st class skills are retained and usable
- All unspent skill points are **lost** (cannot be carried over)
- Base Level and Base EXP are **preserved**
- Stats and stat points are **preserved**
- New skill tree opens for 2nd class skills

### 4.5 Gender-Locked Classes

| Class | Gender | Notes |
|-------|--------|-------|
| Bard | Male only | Male Archers become Bards; they use Instruments |
| Dancer | Female only | Female Archers become Dancers; they use Whips |
| Minstrel | Male only | Transcendent Bard |
| Gypsy | Female only | Transcendent Dancer |

All other classes are available to both genders.

---

## 5. Transcendent/Rebirth System

### 5.1 Overview

The Transcendent (Rebirth) system allows a maxed-out 2nd class character to "reborn" as a more powerful version of themselves. The character goes through the entire leveling process again but with significant bonuses.

### 5.2 Rebirth Requirements

| Requirement | Detail |
|------------|--------|
| Base Level | 99 |
| Job Level | 50 (as 2nd class) |
| Zeny | 1,285,000 (can be waived via quest -- hunt Runaway Book in Juno fields) |
| Skill Points | All must be allocated (0 unspent) |
| Weight | Must be 0% (empty inventory, nothing equipped) |
| Cart | Must not have an active cart |
| Pet | Must not have a Cute Pet active |
| Falcon | Must not have a Falcon equipped |
| Peco Peco | Must not be mounted on a Peco Peco |

### 5.3 Rebirth Process

1. Travel to **Juno** (Yuno)
2. Speak to **Metheus Sylphe** at the Sage Academy
3. Donate 1,285,000 Zeny (or complete the free quest: hunt a Runaway Book in Juno fields)
4. Navigate through the academy to reach **Valhalla**
5. Walk north to meet the **Valkyrie**
6. The Valkyrie transforms the character into a **High Novice** at Base Level 1, Job Level 1

### 5.4 What Changes on Rebirth

| Attribute | Before Rebirth | After Rebirth |
|-----------|---------------|---------------|
| Base Level | 99 | 1 |
| Job Level | 50 | 1 |
| Base EXP | (any) | 0 |
| Job EXP | (any) | 0 |
| Stats (STR/AGI/etc.) | (player-set) | All reset to 1 |
| Stat Points | (spent) | **100** (vs 48 for normal Novice) |
| Skill Points | (spent) | 0 |
| All Skills | (learned) | Lost (must re-learn) |
| Zeny | (any - 1,285,000) | Preserved (minus fee) |
| Items | Must be empty | -- |
| Class | 2nd class | High Novice |
| Quest Skills | (any) | **First Aid + Play Dead pre-learned** |
| Sprite | Normal | Different color (High Novice has unique sprite) |

### 5.5 Transcendent Benefits

| Benefit | Detail |
|---------|--------|
| Starting Stat Points | 100 (vs 48 for normal Novice) -- 52 extra points |
| HP/SP Multiplier | 1.25x permanent (TransMod) on all Max HP/SP calculations |
| Job Level Cap | 70 (vs 50 for normal 2nd class) -- 20 extra job levels |
| Extra Skill Points | 20 more skill points from Job 51-70 |
| Exclusive Skills | Access to transcendent-only skills (varies by class) |
| Quest Skills | First Aid and Play Dead are pre-learned on rebirth |
| EXP Tables | **Harder** -- transcendent base EXP tables are ~1.5x to 3.4x normal |

### 5.6 Rebirth Progression Path

```
2nd Class (Base 99, Job 50)
    │
    ▼ [Rebirth via Valkyrie]
High Novice (Base 1, Job 1)
    │ (100 stat points, plays like normal Novice)
    ▼ [Job Lv 10]
High First Class (Base any, Job 1-50)
    │ (e.g., High Swordman -- same skills as Swordman, different sprite)
    ▼ [Job Lv 40+]
Transcendent Second Class (Base any, Job 1-70)
    │ (e.g., Lord Knight -- all Knight skills + exclusive LK skills)
    └── Max: Base 99, Job 70
```

### 5.7 Transcendent EXP Multipliers (vs Normal)

| Base Level | Normal EXP | Trans EXP | Multiplier |
|-----------|-----------|----------|-----------|
| 1 | 9 | 10 | 1.11x |
| 10 | 320 | 400 | 1.25x |
| 30 | 7,995 | 12,392 | 1.55x |
| 50 | 115,254 | 213,220 | 1.85x |
| 70 | 1,473,746 | 3,389,616 | 2.30x |
| 90 | 9,738,720 | 29,216,160 | 3.00x |
| 98 | 99,999,998 | 343,210,000 | 3.43x |

**Total Base EXP 1-99 (Normal)**: ~405,234,427
**Total Base EXP 1-99 (Transcendent)**: ~1,212,492,549

---

## 6. Super Novice Mechanics

### 6.1 Overview

Super Novice is a special class that branches from Novice. It can learn almost all 1st class skills but retains the Novice's weak HP/SP growth tables.

### 6.2 Requirements

| Requirement | Value |
|------------|-------|
| Current Class | Novice |
| Job Level | 10 (all points in Basic Skill) |
| Base Level | 45 or higher |
| Quest | Super Novice job change quest |

### 6.3 Stats and Caps

| Attribute | Value |
|-----------|-------|
| Base Level Cap | 99 |
| Job Level Cap | 99 |
| HP/SP Coefficients | Same as Novice (HP_A=0.0, HP_B=5.0, SP_A=0.0, SP_B=2.0) |
| Skill Points | 98 (from Job 2-99) |
| Weight Bonus | 0 |

### 6.4 Learnable Skills

Super Novice can learn **almost all First Class skills** from all 6 first class trees:
- Swordman skills (Bash, Provoke, Endure, etc.)
- Mage skills (Firebolt, Cold Bolt, Lightning Bolt, Fire Wall, etc.)
- Archer skills (Owl's Eye, Vulture's Eye, Improve Concentration -- but NOT Double Strafe)
- Merchant skills (Discount, Overcharge, Pushcart, Mammonite, etc.)
- Thief skills (Steal, Hiding, Envenom, Detoxify, etc.)
- Acolyte skills (Heal, Blessing, Increase AGI, Pneuma, etc.)

### 6.5 Skills Super Novice CANNOT Learn

| Excluded Skill | Class | Reason |
|---------------|-------|--------|
| Two-Hand Sword Mastery | Swordman | Cannot equip 2H swords |
| Double Strafe | Archer | Cannot equip bows |
| Arrow Shower | Archer | Cannot equip bows |
| Arrow Crafting | Archer | Cannot equip bows |
| Charge Arrow | Archer | Cannot equip bows |
| Play Dead | Novice | Explicitly excluded |

**General rule**: Any skill that requires a weapon type the Super Novice cannot equip is excluded.

### 6.6 Weapon Restrictions

| Usable Weapons | Cannot Use |
|---------------|-----------|
| Daggers | Bows |
| 1H Swords | 2H Swords |
| Maces | Spears (1H and 2H) |
| Axes (1H) | 2H Axes |
| Rods/Staves | Katars |
| Knuckles | Instruments |
| Books | Whips |

### 6.7 Special Mechanics

#### Guardian Angel (Level-Up Angel)

At every Base Level that is a **multiple of 10** (10, 20, 30, ..., 90), a Guardian Angel appears and casts one random buff on the Super Novice:

| Possible Buff | Effect |
|--------------|--------|
| Kyrie Eleison | Damage-absorbing shield |
| Magnificat | Doubled SP regeneration |
| Gloria | +30 LUK for duration |
| Suffragium | Reduced cast time for next spell |
| Impositio Manus | Increased ATK |

#### Steel Body Angel (Death Protection)

When a Super Novice's HP reaches 0, there is a chance the Guardian Angel will:
1. Fully restore HP to 100%
2. Cast **Mental Strength** (Steel Body) on the Super Novice
3. This effectively prevents death

The chance is believed to be based on intimacy/hidden value. It does not trigger every time.

#### /doridori Command

The `/doridori` chat command causes the character's head to shake back and forth. For Super Novices:
- Doubles natural HP/SP regeneration rate for a period
- No combat effect; purely a regen boost
- Has a cooldown

### 6.8 Super Novice Cannot Transcend

Super Novices **cannot undergo rebirth**. There is no transcendent Super Novice in pre-renewal. (Expanded Super Novice was added later in Renewal.)

---

## 7. Job Change Requirements

### 7.1 Summary Table (All Transitions)

| From | To | Min Job Lv | Min Base Lv | Zeny Cost | Items Required | Quest |
|------|----|-----------|-------------|-----------|----------------|-------|
| (Start) | Novice | -- | -- | 0 | -- | Automatic |
| Novice | Swordman | 10 | -- | 0 | -- | Izlude combat trial |
| Novice | Mage | 10 | -- | 0 | Varies (solutions) | Geffen quiz + items |
| Novice | Archer | 10 | -- | Varies | Trunks/items | Payon collection |
| Novice | Merchant | 10 | -- | 0 | Delivery items | Alberta delivery |
| Novice | Thief | 10 | -- | 10,000 | Mushroom spores, etc. | Morroc collection |
| Novice | Acolyte | 10 | -- | 0 | -- | Prontera pilgrimage |
| Novice | Super Novice | 10 | 45 | 0 | -- | Special quest |
| Swordman | Knight | 40 | -- | 0 | Quest items | Prontera trial |
| Swordman | Crusader | 40 | -- | 0 | Quest items | Prontera faith trial |
| Mage | Wizard | 40 | -- | 0 | Varies (Job 50 skips) | Geffen quiz + items |
| Mage | Sage | 40 | -- | 0 | Quest items | Juno academic test |
| Archer | Hunter | 40 | -- | 0 | Quest items | Hugel interview + combat |
| Archer | Bard (M) | 40 | -- | 0 | Quest items | Comodo music test |
| Archer | Dancer (F) | 40 | -- | 0 | Quest items | Comodo dance test |
| Merchant | Blacksmith | 40 | -- | 0 | Quest items | Einbroch forging test |
| Merchant | Alchemist | 40 | -- | 0 | Quest items | Al De Baran brewing test |
| Thief | Assassin | 40 | -- | 0 | Quest items | Morroc stealth + combat |
| Thief | Rogue | 40 | -- | 0 | Quest items | Comodo stealth + combat |
| Acolyte | Priest | 40 | -- | 0 | Quest items | Prontera undead combat |
| Acolyte | Monk | 40 | -- | 0 | Quest items | St. Capitolina Abbey |
| 2nd Class | Trans. 2nd | 50 | 99 | 1,285,000 | (see Sec 5.2) | Juno Valkyrie rebirth |

### 7.2 Job 50 vs Job 40 Trade-offs

| Aspect | Change at Job 40 | Change at Job 50 |
|--------|-----------------|-----------------|
| Skill Points (1st class) | 39 | 49 |
| Skill Points Lost | 10 permanently | 0 |
| Quest Difficulty | Full quest with all steps | Some quests skip steps |
| Leveling Time | ~60% faster to change | ~40% more grinding |
| **Recommendation** | Only for alts/speed | Always for main characters |

---

## 8. Class-Specific Stats (HP/SP Growth Tables)

### 8.1 HP Growth Formula

```
BaseHP = 35 + (BaseLevel * HP_JOB_B)
for i = 2 to BaseLevel:
    BaseHP += round(HP_JOB_A * i)
MaxHP = floor(BaseHP * (1 + VIT * 0.01) * TransMod)
```

Where `TransMod` = 1.25 for transcendent classes, 1.0 otherwise.

### 8.2 SP Growth Formula

```
BaseSP = 10 + (BaseLevel * SP_JOB_B)
for i = 2 to BaseLevel:
    BaseSP += round(SP_JOB_A * i)
MaxSP = floor(BaseSP * (1 + INT * 0.01) * TransMod)
```

### 8.3 HP/SP Coefficients Per Class

| Class | HP_JOB_A | HP_JOB_B | SP_JOB_A | SP_JOB_B | Weight Bonus |
|-------|----------|----------|----------|----------|-------------|
| Novice | 0.0 | 5.0 | 0.0 | 2.0 | 0 |
| Super Novice | 0.0 | 5.0 | 0.0 | 2.0 | 0 |
| Swordman | 0.7 | 5.0 | 0.2 | 2.0 | 800 |
| Mage | 0.3 | 5.0 | 0.6 | 2.0 | 200 |
| Archer | 0.5 | 5.0 | 0.4 | 2.0 | 600 |
| Merchant | 0.4 | 5.0 | 0.3 | 2.0 | 800 |
| Thief | 0.5 | 5.0 | 0.3 | 2.0 | 400 |
| Acolyte | 0.4 | 5.0 | 0.5 | 2.0 | 400 |
| Knight | 1.5 | 5.0 | 0.4 | 2.0 | 800 |
| Wizard | 0.55 | 5.0 | 1.0 | 2.0 | 200 |
| Hunter | 0.85 | 5.0 | 0.6 | 2.0 | 600 |
| Blacksmith | 0.9 | 5.0 | 0.5 | 2.0 | 800 |
| Assassin | 1.1 | 5.0 | 0.5 | 2.0 | 600 |
| Priest | 0.75 | 5.0 | 0.8 | 2.0 | 600 |
| Crusader | 1.1 | 7.0 | 0.5 | 2.0 | 800 |
| Sage | 0.75 | 5.0 | 0.8 | 2.0 | 400 |
| Bard | 0.75 | 3.0 | 0.6 | 2.0 | 600 |
| Dancer | 0.75 | 3.0 | 0.6 | 2.0 | 600 |
| Alchemist | 0.9 | 5.0 | 0.5 | 2.0 | 800 |
| Rogue | 0.85 | 5.0 | 0.5 | 2.0 | 600 |
| Monk | 0.9 | 6.5 | 0.5 | 2.0 | 600 |

**Transcendent classes** use the same coefficients as their base class but get the 1.25x TransMod multiplier applied.

### 8.4 HP Examples at Base Level 99

Calculations assume 0 VIT (BaseHP only, no VIT scaling):

| Class | BaseHP (Lv99, 0 VIT) | MaxHP (80 VIT, normal) | MaxHP (80 VIT, trans) |
|-------|----------------------|------------------------|----------------------|
| Novice | 530 | 954 | 1,193 |
| Swordman | 3,985 | 7,173 | 8,966 |
| Mage | 2,012 | 3,622 | 4,527 |
| Archer | 2,999 | 5,398 | 6,748 |
| Knight | 7,955 | 14,319 | 17,899 |
| Wizard | 3,302 | 5,944 | 7,430 |
| Hunter | 4,755 | 8,559 | 10,699 |
| Assassin | 5,778 | 10,400 | 13,000 |
| Priest | 4,442 | 7,996 | 9,995 |
| Crusader | 6,471 | 11,648 | 14,560 |
| Monk | 5,288 | 9,518 | 11,898 |

### 8.5 SP Examples at Base Level 99

Calculations assume 0 INT:

| Class | BaseSP (Lv99, 0 INT) | MaxSP (80 INT, normal) | MaxSP (80 INT, trans) |
|-------|----------------------|------------------------|----------------------|
| Novice | 208 | 374 | 468 |
| Mage | 3,166 | 5,699 | 7,124 |
| Wizard | 5,158 | 9,284 | 11,605 |
| Priest | 4,170 | 7,506 | 9,383 |
| Sage | 4,170 | 7,506 | 9,383 |
| Acolyte | 2,678 | 4,820 | 6,025 |

---

## 9. Job Bonuses (Stat Bonuses Per Job Level)

### 9.1 Overview

Each class receives hidden automatic stat bonuses at specific job levels. These:
- Are NOT player-controlled -- they are applied automatically
- Do NOT cost stat points
- Stack with player-allocated stats
- Display as green text in the stat window
- Are cumulative (shown values are total at that job level)

### 9.2 Cumulative Job Bonus Stats at Max Job Level

| Class | STR | AGI | VIT | INT | DEX | LUK | Total |
|-------|-----|-----|-----|-----|-----|-----|-------|
| Novice (Jb10) | 1 | 1 | 1 | 1 | 1 | 1 | 6 |
| Swordman (Jb50) | 5 | 3 | 5 | 0 | 3 | 0 | 16 |
| Mage (Jb50) | 0 | 2 | 0 | 6 | 5 | 0 | 13 |
| Archer (Jb50) | 1 | 5 | 0 | 1 | 5 | 1 | 13 |
| Merchant (Jb50) | 4 | 1 | 4 | 1 | 4 | 1 | 15 |
| Thief (Jb50) | 3 | 5 | 1 | 0 | 4 | 2 | 15 |
| Acolyte (Jb50) | 0 | 2 | 2 | 5 | 3 | 3 | 15 |
| Knight (Jb50) | 8 | 5 | 7 | 1 | 5 | 1 | 27 |
| Wizard (Jb50) | 0 | 3 | 0 | 9 | 8 | 0 | 20 |
| Hunter (Jb50) | 3 | 8 | 1 | 2 | 8 | 2 | 24 |
| Blacksmith (Jb50) | 6 | 2 | 6 | 2 | 6 | 2 | 24 |
| Assassin (Jb50) | 5 | 8 | 2 | 0 | 5 | 4 | 24 |
| Priest (Jb50) | 0 | 3 | 5 | 7 | 5 | 3 | 23 |
| Crusader (Jb50) | 7 | 3 | 7 | 3 | 5 | 2 | 27 |
| Sage (Jb50) | 0 | 5 | 0 | 8 | 7 | 0 | 20 |
| Bard/Dancer (Jb50) | 3 | 6 | 1 | 4 | 6 | 4 | 24 |
| Alchemist (Jb50) | 4 | 4 | 4 | 4 | 4 | 4 | 24 |
| Rogue (Jb50) | 4 | 7 | 2 | 1 | 6 | 4 | 24 |
| Monk (Jb50) | 6 | 6 | 3 | 3 | 5 | 4 | 27 |

### 9.3 Transcendent Job Bonus Extension (Job 51-70)

Transcendent classes use the **same bonus table** as their base class for Job Levels 1-50, then receive additional bonuses for Job Levels 51-70. The extra 20 levels typically add 4-6 more total bonus stats, distributed according to the class's theme.

Approximate additional bonuses (Job 51-70):

| Trans Class | +STR | +AGI | +VIT | +INT | +DEX | +LUK | Extra Total |
|------------|------|------|------|------|------|------|-------------|
| Lord Knight | +2 | +1 | +1 | +0 | +1 | +0 | +5 |
| High Wizard | +0 | +1 | +0 | +2 | +2 | +0 | +5 |
| Sniper | +1 | +2 | +0 | +0 | +2 | +0 | +5 |
| Whitesmith | +1 | +1 | +1 | +0 | +1 | +1 | +5 |
| Assassin Cross | +1 | +2 | +0 | +0 | +1 | +1 | +5 |
| High Priest | +0 | +1 | +1 | +2 | +1 | +1 | +6 |
| Paladin | +1 | +1 | +2 | +1 | +1 | +0 | +6 |
| Scholar | +0 | +1 | +0 | +2 | +2 | +0 | +5 |
| Minstrel/Gypsy | +1 | +1 | +0 | +1 | +1 | +1 | +5 |
| Biochemist | +1 | +1 | +1 | +1 | +1 | +1 | +6 |
| Stalker | +1 | +2 | +0 | +0 | +1 | +1 | +5 |
| Champion | +1 | +1 | +1 | +1 | +1 | +1 | +6 |

### 9.4 Detailed Swordman Job Bonus Table (Job 1-50)

Example of the per-level-milestone distribution pattern:

| Job Lv | STR | AGI | VIT | INT | DEX | LUK | Change |
|--------|-----|-----|-----|-----|-----|-----|--------|
| 1 | 0 | 0 | 0 | 0 | 0 | 0 | -- |
| 2 | 1 | 0 | 0 | 0 | 0 | 0 | +1 STR |
| 6 | 1 | 0 | 1 | 0 | 0 | 0 | +1 VIT |
| 10 | 1 | 0 | 1 | 0 | 1 | 0 | +1 DEX |
| 14 | 2 | 0 | 1 | 0 | 1 | 0 | +1 STR |
| 18 | 2 | 1 | 1 | 0 | 1 | 0 | +1 AGI |
| 22 | 2 | 1 | 2 | 0 | 1 | 0 | +1 VIT |
| 26 | 3 | 1 | 2 | 0 | 1 | 0 | +1 STR |
| 30 | 3 | 2 | 3 | 0 | 2 | 0 | +1 AGI, +1 VIT, +1 DEX |
| 33 | 3 | 2 | 3 | 0 | 2 | 0 | -- |
| 36 | 4 | 2 | 3 | 0 | 2 | 0 | +1 STR |
| 38 | 4 | 2 | 4 | 0 | 2 | 0 | +1 VIT |
| 40 | 4 | 2 | 4 | 0 | 2 | 0 | -- |
| 42 | 4 | 2 | 4 | 0 | 3 | 0 | +1 DEX |
| 46 | 5 | 3 | 5 | 0 | 3 | 0 | +1 STR, +1 AGI, +1 VIT |
| 50 | 5 | 3 | 5 | 0 | 3 | 0 | -- |

*Note: Exact per-level breakpoints vary slightly between sources. The cumulative totals at Job 50 are authoritative. For implementation, use the rAthena `job_stats.yml` / `job_bonus` data as the source of truth.*

### 9.5 Job Bonus Design Pattern

Each class's bonuses reflect its role:
- **Melee DPS** (Swordman, Knight, Assassin, Monk): High STR + AGI
- **Tank** (Crusader, Paladin): High STR + VIT
- **Caster** (Mage, Wizard, Sage): High INT + DEX
- **Ranged** (Archer, Hunter): High AGI + DEX
- **Support** (Acolyte, Priest): High INT + VIT + DEX
- **Hybrid** (Alchemist, Blacksmith, Merchant): Balanced spread
- **Agility** (Thief, Rogue, Assassin): High AGI + DEX

---

## 10. Weapon Restrictions

### 10.1 Weapon Types in Pre-Renewal

| Weapon Type | Code | Handedness | Example |
|------------|------|-----------|---------|
| Dagger | `dagger` | 1H | Knife, Stiletto, Gladius |
| 1H Sword | `one_hand_sword` | 1H | Sword, Falchion, Blade |
| 2H Sword | `two_hand_sword` | 2H | Katana, Zweihander, Claymore |
| 1H Spear | `spear` | 1H (with shield) | Javelin, Pike |
| 2H Spear | `two_hand_spear` | 2H | Lance, Halberd, Trident |
| 1H Axe | `axe` | 1H | Axe, Battle Axe |
| 2H Axe | `two_hand_axe` | 2H | Two-Handed Axe, Berdysz |
| Mace | `mace` | 1H | Mace, Flail, Morning Star |
| Rod/Staff | `staff` | 2H | Rod, Wand, Staff |
| Bow | `bow` | 2H | Bow, Composite Bow, Arbalest |
| Knuckle/Claw | `knuckle` | 1H | Waghnak, Knuckle Duster |
| Musical Instrument | `instrument` | 1H | Guitar, Mandolin, Harp |
| Whip | `whip` | 1H | Whip, Tail, Rope |
| Book | `book` | 1H | Bible, Book, Tablet |
| Katar | `katar` | 2H (dual slot) | Katar, Jamadhar, Kris |

### 10.2 Weapon Usability by Class

| Weapon Type | Usable By |
|------------|-----------|
| Dagger | ALL classes (Novice, all 1st, all 2nd) |
| 1H Sword | Swordman, Knight, Crusader, Merchant, Blacksmith, Alchemist, Thief, Assassin, Rogue |
| 2H Sword | Swordman, Knight (NOT Crusader) |
| 1H Spear | Swordman, Knight, Crusader |
| 2H Spear | Swordman, Knight, Crusader |
| 1H Axe | Swordman, Knight, Merchant, Blacksmith, Alchemist |
| 2H Axe | Swordman, Knight, Merchant, Blacksmith, Alchemist |
| Mace | Swordman, Knight, Crusader, Merchant, Blacksmith, Alchemist, Acolyte, Priest, Monk |
| Rod/Staff | Mage, Wizard, Sage, Acolyte, Priest, Monk |
| Bow | Archer, Hunter, Bard, Dancer, Thief (some), Rogue |
| Knuckle/Claw | Acolyte, Priest, Monk |
| Instrument | Bard only (male Archer 2-2 / Minstrel) |
| Whip | Dancer only (female Archer 2-2 / Gypsy) |
| Book | Sage, Scholar, Priest, High Priest |
| Katar | Assassin, Assassin Cross only |

### 10.3 Novice Weapon Restrictions

Novices and High Novices can **only** equip:
- Daggers (limited selection -- typically only Knife, Cutter, Main Gauche)

### 10.4 Super Novice Weapon Access

Super Novices can equip:
- Daggers, 1H Swords, Maces, 1H Axes, Rods/Staves, Knuckles, Books

Super Novices CANNOT equip:
- Bows, 2H Swords, Spears (1H or 2H), 2H Axes, Katars, Instruments, Whips

### 10.5 Transcendent Weapon Access

Transcendent classes inherit **all weapon permissions** from their base 2nd class. No weapon types are gained or lost on transcendence.

---

## 11. Armor Restrictions

### 11.1 Equipment Slot System

| Slot | Name | Example Items |
|------|------|---------------|
| Upper Headgear | Head (top) | Helm, Hat, Crown, Beret |
| Middle Headgear | Head (mid) | Sunglasses, Monocle, Masquerade |
| Lower Headgear | Head (low) | Pipe, Gangster Mask, Flu Mask |
| Body Armor | Armor | Chain Mail, Mink Coat, Full Plate |
| Shield | Shield | Guard, Buckler, Shield |
| Garment | Garment | Muffler, Manteau, Hood |
| Footgear | Shoes | Sandals, Boots, Shoes |
| Accessory 1 | Accessory | Ring, Clip, Rosary |
| Accessory 2 | Accessory | Ring, Clip, Rosary |

### 11.2 Armor Type Restrictions

#### Shield

| Can Equip Shield | Cannot Equip Shield (when 2H weapon equipped) |
|-----------------|----------------------------------------------|
| All 1H weapon users | Knight (with 2H Sword/Spear) |
| Novice (limited shields) | Assassin (with Katar) |
| -- | Archer/Hunter (with Bow) |

**Note**: Assassins using dual daggers have no shield. Assassins using Katar have no shield. Any class wielding a 2H weapon loses the shield slot.

#### Class-Specific Armor Access

Most armor in RO uses per-item class restrictions rather than blanket class rules. General patterns:

| Armor Category | Typical Class Access |
|---------------|---------------------|
| Plate armor (heavy) | Swordman, Knight, Crusader |
| Robe/Coat (light) | Mage, Wizard, Sage, Priest |
| Leather armor (medium) | Thief, Archer, Merchant and their 2nd classes |
| Universal armor | Most classes can wear |
| Novice-only gear | Cotton Shirt, Novice-exclusive headgears |

**Implementation note**: Armor class restrictions in RO are set **per individual item** in the item database, not by broad armor category. Each item has a class bitmask specifying which classes can equip it.

---

## 12. Skill Point Allocation

### 12.1 Points Per Job Level

| Event | Skill Points Gained |
|-------|-------------------|
| Job Level 1 (initial) | 0 (starting level) |
| Each Job Level 2 and above | +1 |

### 12.2 Total Skill Points by Class Tier

| Class Tier | Job Lv Range | Skill Points | Formula |
|-----------|-------------|-------------|---------|
| Novice | 1-10 | 9 | Levels 2-10 |
| First Class | 1-50 | 49 | Levels 2-50 |
| Second Class (normal) | 1-50 | 49 | Levels 2-50 |
| Transcendent 2nd Class | 1-70 | 69 | Levels 2-70 |
| Super Novice | 1-99 | 98 | Levels 2-99 |

### 12.3 Cumulative Skill Points Throughout Career

| Career Path | Novice SP | 1st Class SP | 2nd Class SP | Total |
|------------|-----------|-------------|-------------|-------|
| Normal (Job 50 -> Job 50) | 9 (lost) | 49 | 49 | 98 active (49+49) |
| Normal (Job 40 -> Job 50) | 9 (lost) | 39 | 49 | 88 active (39+49) |
| Transcendent (Job 70) | 9 (lost, x2) | 49 (re-learned) | 69 | 118 active (49+69) |
| Super Novice | 9 (lost) | -- | -- | 98 (multi-tree) |

### 12.4 Skill Point Rules

1. **Novice skill points are lost** when changing to 1st class (Novice skills become inaccessible)
2. **1st class skills are retained** when changing to 2nd class (points stay invested)
3. **Unspent skill points do NOT carry over** between job changes
4. **Transcendent characters re-learn all skills from scratch** (Novice -> High 1st -> Trans 2nd)
5. **Quest skills** do not cost skill points -- they are learned by completing quests (First Aid, Play Dead, etc.)
6. **Rebirth pre-learns** First Aid and Play Dead for transcendent characters

### 12.5 Skill Tree Constraints

Each skill tree has prerequisite chains. For example:
- To learn Bash Lv10, you need 0 prerequisites
- To learn Magnum Break, you need Bash Lv5
- To learn Bowling Bash (Knight), you need Bash Lv10 + Magnum Break Lv3 + 2H Sword Mastery Lv5

Prerequisite skills from the 1st class tree must be learned during the 1st class phase. A 2nd class character cannot retroactively learn 1st class skills they skipped (they can only spend 2nd class skill points on 2nd class skills).

**Exception**: Transcendent characters re-learn everything, so they can re-allocate 1st class skills differently than their previous life.

---

## 13. Equipment Restrictions by Class

### 13.1 Per-Item Class Bitmask System

In RO (and rAthena), each equipment item has a `Jobs` field that is a bitmask of classes allowed to equip it. This is NOT a simple "class can wear armor type X" system -- it is granular per-item.

rAthena job class bitmask values:

| Bit | Value | Class |
|-----|-------|-------|
| 0 | 0x000001 | Novice |
| 1 | 0x000002 | Swordman |
| 2 | 0x000004 | Mage |
| 3 | 0x000008 | Archer |
| 4 | 0x000010 | Acolyte |
| 5 | 0x000020 | Merchant |
| 6 | 0x000040 | Thief |
| 7 | 0x000080 | Knight |
| 8 | 0x000100 | Priest |
| 9 | 0x000200 | Wizard |
| 10 | 0x000400 | Blacksmith |
| 11 | 0x000800 | Hunter |
| 12 | 0x001000 | Assassin |
| 14 | 0x004000 | Crusader |
| 15 | 0x008000 | Monk |
| 16 | 0x010000 | Sage |
| 17 | 0x020000 | Rogue |
| 18 | 0x040000 | Alchemist |
| 19 | 0x080000 | Bard/Dancer |
| 21 | 0x200000 | Super Novice |

**Transcendent classes** inherit the bitmask of their parent 2nd class. A Lord Knight can equip anything a Knight can equip.

### 13.2 Common Equipment Patterns

| Equipment Category | Typical Bitmask Pattern |
|-------------------|------------------------|
| Universal (most headgear, accessories) | All classes (0xFFFFFF or specific list) |
| Heavy melee gear | Swordman + Knight + Crusader |
| Caster gear | Mage + Wizard + Sage + Priest |
| Archer gear | Archer + Hunter + Bard + Dancer |
| Thief gear | Thief + Assassin + Rogue |
| Merchant gear | Merchant + Blacksmith + Alchemist |
| Novice-only | Novice + Super Novice |

### 13.3 Upper Headgear Class Restrictions

Most upper headgears are wearable by all classes. Class-specific headgears include:
- **Monk's Hat**: Monk/Champion only
- **Mage Hat**: Mage/Wizard/Sage + trans only
- **Crown**: Typically limited to Swordman tree
- **Tiara**: Typically limited to Mage/Acolyte trees

### 13.4 Shield Restrictions

| Shield Type | Classes |
|------------|---------|
| Guard | All melee classes |
| Buckler | Swordman, Merchant, Thief trees + Acolyte tree |
| Shield | Swordman, Knight, Crusader primarily |
| Mirror Shield | Wizard, Sage |

---

## 14. Baby Classes

### 14.1 Overview

Baby classes are created through the **Adoption System**. Two characters who are married can adopt a Novice or First Class character, creating a "Baby" version.

### 14.2 Requirements for Adoption

| Requirement | Detail |
|------------|--------|
| Parents | Must be married (Wedding System) |
| Parent Level | Both parents must be Base Level 70+ |
| Adoptee Class | Must be Novice or First Class |
| Adoptee Level | Any (but cannot be 2nd class already) |

### 14.3 Baby Class Restrictions

| Restriction | Detail |
|------------|--------|
| Stat Cap | Maximum base stat = **80** (vs 99 for normal) |
| Max HP/SP | **75% of normal** Max HP and Max SP |
| Cannot Transcend | Baby classes CANNOT undergo rebirth |
| Skill Points | Same as normal (no change) |
| Equipment | Same access as normal (no change) |
| Appearance | Baby sprite -- smaller, chibi-style |

### 14.4 Baby Class List

| Normal Class | Baby Class | Baby Trans? |
|-------------|-----------|-------------|
| Novice | Baby Novice | No |
| Swordman | Baby Swordman | No |
| Mage | Baby Mage | No |
| Archer | Baby Archer | No |
| Merchant | Baby Merchant | No |
| Thief | Baby Thief | No |
| Acolyte | Baby Acolyte | No |
| Knight | Baby Knight | No |
| Wizard | Baby Wizard | No |
| Hunter | Baby Hunter | No |
| Blacksmith | Baby Blacksmith | No |
| Assassin | Baby Assassin | No |
| Priest | Baby Priest | No |
| Crusader | Baby Crusader | No |
| Sage | Baby Sage | No |
| Bard/Dancer | Baby Bard/Dancer | No |
| Alchemist | Baby Alchemist | No |
| Rogue | Baby Rogue | No |
| Monk | Baby Monk | No |
| Super Novice | Baby Super Novice | No |

### 14.5 Baby-Specific Mechanics

- **Stats over 80 converted to stat points** on adoption if any were above 80
- **Skill "Mama Papa" (Baby skill)**: Warps to parent's location (unique to baby classes)
- **Cannot be adopted again** once already adopted
- **Divorce**: If parents divorce, baby class status is NOT removed (permanent)

### 14.6 Implementation Priority

Baby classes are **LOW PRIORITY** for initial implementation. They are a cosmetic/social system that requires the Marriage system as a prerequisite. Recommended to defer until social systems (marriage, guild, etc.) are implemented.

---

## 15. Edge Cases and Special Rules

### 15.1 Gender-Locked Classes

| Class | Gender | Consequence |
|-------|--------|------------|
| Bard | Male | Female Archers CANNOT become Bards |
| Dancer | Female | Male Archers CANNOT become Dancers |
| Minstrel | Male | Transcendent Bard |
| Gypsy | Female | Transcendent Dancer |

If a male Archer reaches Job 40+, they can choose Knight (2-1 from Swordman -- NO, Archer cannot become Knight) or Hunter/Bard. If female, Hunter/Dancer.

### 15.2 Skill Retention Across Job Changes

| Transition | Skills Retained? | Details |
|-----------|-----------------|---------|
| Novice -> 1st Class | NO | Novice skills are lost |
| 1st Class -> 2nd Class | YES (1st class) | 1st class skills remain usable |
| 2nd Class -> Rebirth (High Novice) | NO | ALL skills are lost |
| High Novice -> High 1st Class | NO | Only quest skills pre-learned |
| High 1st -> Trans 2nd Class | YES (High 1st) | Same as normal 1st->2nd |

### 15.3 Stat Point Carry-Over

| Event | Stat Points |
|-------|------------|
| Create character | 48 (all stats at 1) |
| Job change (1st -> 2nd) | Preserved (no change) |
| Rebirth | Reset to **100** (all stats back to 1, with 100 distributable) |

### 15.4 Equipment on Job Change

When changing jobs:
- Equipment that the new class **cannot use** must be unequipped
- In rAthena, the server auto-unequips invalid items on job change
- Items remain in inventory (not lost)

### 15.5 Cart, Falcon, Peco Peco on Job Change

| Feature | Class | On Job Change |
|---------|-------|--------------|
| Cart | Merchant tree | Lost on rebirth, re-obtainable as Blacksmith/Alchemist |
| Falcon | Hunter/Sniper | Lost on rebirth, re-obtainable as Hunter |
| Peco Peco | Knight/Crusader | Lost on rebirth, re-obtainable as Knight/Crusader |

### 15.6 Death Penalty by Class Tier

| Class Tier | Death Penalty |
|-----------|--------------|
| Novice | **None** (0% EXP loss) |
| First Class | 1% of current level's total base EXP |
| Second Class | 1% of current level's total base EXP |
| Transcendent 1st/2nd | 1% (but trans EXP tables are much larger, so absolute loss is greater) |
| Super Novice | 1% (with Guardian Angel death protection chance) |
| Baby Classes | 1% (same as normal) |

### 15.7 Job Level Does NOT Affect

- Stat points (only base level grants stat points)
- Weight limit
- Base EXP table
- HP/SP (only affects via job bonuses adding to stats)

### 15.8 Multiple Characters Per Account

- Standard RO allows multiple characters per account (typically 3-9 slots)
- Each character is independent (different class, different stats, different level)
- Characters share the same account but do NOT share items, zeny, or progress

---

## 16. Transcendent-Exclusive Skills

### 16.1 Overview

Each transcendent class gains access to a set of skills that are unavailable to the normal 2nd class version. These skills are unlocked via the extra 20 job levels (Job 51-70) worth of skill points.

### 16.2 Exclusive Skills by Class

#### Lord Knight (Trans Knight)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Aura Blade | 5 | Buff | +100-500 non-elemental ATK to weapon for 40-200s |
| Parrying (Parry) | 10 | Buff | Block melee attacks with 2H sword (23-50% chance) |
| Concentration | 5 | Buff | +5-25% ATK/HIT, -5-25% DEF for 25-85s |
| Tension Relax | 1 | Buff | 3x HP regen while sitting (cancelled by movement/attack) |
| Berserk (Frenzy) | 1 | Buff | Set ASPD to 190, 2x ATK, 1 DEF/MDEF, drain SP to 0, cannot use skills |
| Spiral Pierce | 5 | Attack | 150-450% ATK + weight-based damage, 3-hit piercing |
| Head Crush | 5 | Attack | 200-440% ATK, chance to cause bleeding |
| Joint Beat | 10 | Attack | 60-510% ATK, chance to debuff target's stats |

#### High Wizard (Trans Wizard)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Soul Drain | 10 | Passive | Recover SP on killing an enemy with magic |
| Magic Crasher | 1 | Attack | Physical attack using MATK instead of ATK |
| Amplify Magic Power (Mystical Amplification) | 10 | Buff | Next magic skill does +50% MATK |
| Napalm Vulcan | 5 | Attack | 1-5 hits of Ghost element magic AoE |
| Ganbantein | 1 | Special | Destroy ground-targeted AoE skills (traps, ground effects) |
| Gravitation Field | 5 | AoE | Heavy AoE damage + ASPD debuff in area |

#### Sniper (Trans Hunter)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| True Sight | 10 | Buff | +STR/AGI/VIT/INT/DEX/LUK, +HIT, +CRI for 30-300s |
| Wind Walk | 10 | Buff | Party move speed +4-40%, Flee +1-10 |
| Falcon Assault | 5 | Attack | Powerful Falcon strike (INT/DEX/Falcon-based damage) |
| Sharp Shooting | 5 | Attack | 500% ATK piercing ranged attack |
| Phantasmic Arrow | 1 | Attack | Ghost element arrow attack (no arrow consumed) |

#### Whitesmith (Trans Blacksmith)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Cart Boost | 1 | Buff | Increase move speed while cart equipped |
| Maximum Over Thrust | 5 | Buff | +20-100% ATK, weapon break chance on self |
| Full Adrenaline Rush | 5 | Buff | ASPD boost for entire party (all weapons) |
| Meltdown (Shattering Strike) | 10 | Buff | Chance to break enemy weapon/armor on hit |
| Cart Termination | 10 | Attack | ATK+cart weight damage, pushback |

#### Assassin Cross (Trans Assassin)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Advanced Katar Mastery | 5 | Passive | +12-20% Katar ATK |
| Create Deadly Poison | 1 | Special | Create Deadly Poison bottle from ingredients |
| Enchant Deadly Poison (EDP) | 5 | Buff | +100-400% weapon ATK for 40-120s |
| Soul Breaker (Soul Destroyer) | 10 | Attack | Ranged ATK+MATK hybrid damage |
| Meteor Assault | 10 | AoE | ATK + chance for Stun/Bleed/Blind around caster |

#### High Priest (Trans Priest)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Meditatio (Meditation) | 10 | Passive | +1-10% Max SP, +1-3% SP Regen, +1-5% Heal effectiveness |
| Assumptio | 5 | Buff | Halve damage taken for 20-100s |
| Basilica | 5 | AoE | Create holy barrier -- immunity to all attacks inside (cannot attack out either) |
| Mana Recharge | 5 | Passive | Reduce SP cost of skills by 4-20% |

#### Paladin (Trans Crusader)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Battle Chant (Gospel) | 10 | AoE | Random buffs to party, random debuffs to enemies in area |
| Sacrifice (Martyr's Reckoning) | 5 | Attack | Deal 1.1-1.9x MaxHP as damage, consume own HP |
| Pressure (Gloria Domini) | 5 | Attack | Ranged Holy attack, SP damage |

#### Scholar/Professor (Trans Sage)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Double Casting | 5 | Buff | 40-80% chance to auto-cast another bolt on bolt spell |
| Mind Breaker | 5 | Debuff | +20-100% target MATK, -12-60% target MDEF |
| Soul Change | 1 | Special | Swap SP percentage with target |
| Memorize | 1 | Buff | Next 3 casts use 50% cast time |
| Soul Exhale | 1 | Special | Transfer own SP to target |
| Indulge (Soul Siphon) | 5 | Special | Drain SP from target |

#### Minstrel/Clown (Trans Bard)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Tarot Card of Fate | 5 | Special | Random card effect on target (14 possible effects) |
| Arrow Vulcan | 10 | Attack | 3-12 hit ranged attack |
| Marionette Control | 1 | Buff | Transfer half of caster stats to target party member |

#### Gypsy (Trans Dancer)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Tarot Card of Fate | 5 | Special | Same as Minstrel version |
| Arrow Vulcan | 10 | Attack | Same as Minstrel version |
| Marionette Control | 1 | Buff | Same as Minstrel version |

*Note: Minstrel and Gypsy share the same transcendent skills.*

#### Biochemist/Creator (Trans Alchemist)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Acid Demonstration (Acid Bomb) | 10 | Attack | ATK*MATK / target VIT ranged damage |
| Plant Cultivation | 2 | Special | Summon mushrooms or plants |
| Full Chemical Protection | 5 | Buff | Prevent equipment break on target for 120-600s |
| Slim Potion Pitcher | 10 | Heal | Heal party member with slim potion items |

#### Stalker (Trans Rogue)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Chase Walk | 5 | Buff | Cloak while moving (STR bonus on reveal) |
| Preserve | 1 | Buff | Prevent Plagiarism from being overwritten |
| Full Divestment (Full Strip) | 5 | Debuff | Strip all equipment from target for 5-25s |
| Reject Sword | 3 | Buff | Reflect 15-45% melee damage, chance to push back |

#### Champion (Trans Monk)

| Skill | Max Lv | Type | Description |
|-------|--------|------|-------------|
| Zen (Dangerous Soul Collect) | 1 | Buff | Instantly generate 5 spirit spheres |
| Tiger Knuckle Fist | 5 | Attack | 60-260% ATK combo skill |
| Chain Crush Combo | 10 | Attack | 200-1100% ATK combo finisher |
| Raging Palm Strike | 5 | Attack | Ranged spirit sphere attack |

---

## 17. Implementation Checklist

### 17.1 Database

- [ ] `job_classes` table with all 40 class definitions (IDs, names, tiers, parent relationships)
- [ ] `job_bonus_stats` table with per-job-level stat bonuses for all classes
- [ ] HP/SP coefficients stored per class (HP_JOB_A, HP_JOB_B, SP_JOB_A, SP_JOB_B)
- [ ] Weight bonus per class
- [ ] ASPD base delay table per class per weapon type
- [ ] Base EXP tables (normal + transcendent)
- [ ] Job EXP tables (novice, first, second, trans second, super novice)
- [ ] Character table: `class`, `job_class`, `job_level`, `job_exp`, `is_transcendent`, `rebirth_count`
- [ ] Class bitmask field on all equipment items

### 17.2 Server (Node.js)

- [ ] `JOB_CLASS_CONFIG` map with all 40 classes (already partially exists in `ro_exp_tables.js`)
- [ ] `FIRST_CLASSES` array and `SECOND_CLASS_UPGRADES` map
- [ ] `TRANSCENDENT_UPGRADES` map (2nd class -> trans class)
- [ ] `getClassTier(className)` helper function
- [ ] `canEquipWeapon(className, weaponType)` function
- [ ] `canEquipItem(className, itemJobBitmask)` function
- [ ] `getJobBonus(className, jobLevel)` function returning stat bonuses
- [ ] `calculateMaxHP/SP()` using class-specific coefficients
- [ ] Job change socket handler (`job:change`)
- [ ] Rebirth socket handler (with all prerequisite checks)
- [ ] Auto-unequip invalid items on job change
- [ ] Skill point validation (cannot spend 1st class points on 2nd class skills, etc.)
- [ ] Gender validation for Bard/Dancer
- [ ] Super Novice skill tree validation (excluded skills)
- [ ] Death penalty exemption for Novice class

### 17.3 Client (UE5 C++)

- [ ] Job change UI (class selection dialog)
- [ ] Rebirth confirmation dialog
- [ ] Class-specific sprites/meshes
- [ ] Skill tree UI filtering by current class
- [ ] Job bonus display in stat window (green text)
- [ ] Equipment filtering by class
- [ ] Gender-locked class restriction in UI

### 17.4 Content

- [ ] Job change NPC locations per zone
- [ ] Job change quest scripts (or simplified auto-change for MVP)
- [ ] Transcendent rebirth NPC in Juno

---

## 18. Gap Analysis

### 18.1 Currently Implemented (in Sabri_MMO codebase)

Based on `server/src/ro_exp_tables.js` and `server/src/index.js`:

| Feature | Status | Notes |
|---------|--------|-------|
| JOB_CLASS_CONFIG (21 classes) | Partial | Missing High 1st classes, baby classes |
| FIRST_CLASSES array | Exists | 6 first classes |
| SECOND_CLASS_UPGRADES map | Exists | Maps 1st -> [2-1, 2-2] |
| Base EXP table (normal) | Exists | Levels 1-98 |
| Job EXP tables (novice/first/second) | Exists | |
| Trans EXP tables | Missing | Not in ro_exp_tables.js |
| HP/SP coefficients | Exists | In Implementation doc, needs integration |
| ASPD base delays | Exists | In Implementation doc, needs integration |
| Job bonus stats | Partial | Summary exists, full per-level tables missing |
| job:change handler | Exists | Basic functionality |
| Rebirth handler | Missing | Not implemented |
| Weapon restriction checks | Partial | Some checks exist |
| Equipment class bitmask | Missing | Per-item bitmask not implemented |
| Super Novice | Missing | Not implemented |
| Baby classes | Missing | Not implemented |
| Gender-locked class checks | Missing | Bard/Dancer not gender-validated |
| Trans class exclusive skills | Partial | Some skill IDs exist but not all handlers |
| Death penalty exemption (Novice) | Exists | Implemented |

### 18.2 Priority Gaps (Must Fix for Core Gameplay)

1. **Transcendent EXP tables** -- needed before trans classes work properly
2. **Rebirth handler** -- the entire transcendent progression path is blocked
3. **High 1st class definitions** -- needed for rebirth flow (High Swordman, High Mage, etc.)
4. **Full job bonus tables** -- per-job-level granular data from rAthena
5. **Equipment class bitmask validation** -- prevents equipping invalid gear
6. **Weapon type validation per class** -- `canEquipWeapon()` function

### 18.3 Secondary Gaps (Important but Not Blocking)

7. **Super Novice class path** -- alternative progression, lower priority
8. **Gender-locked Bard/Dancer validation** -- correctness issue
9. **Auto-unequip on job change** -- quality of life
10. **Transcendent-exclusive skill handlers** -- some already exist from skill audits
11. **Job change quest system** -- can use simplified NPC auto-change initially

### 18.4 Deferred (Not Needed Yet)

12. **Baby classes** -- requires Marriage system
13. **Full job change quest scripts** -- simplified version acceptable for MVP
14. **Class-specific sprites** -- cosmetic, can use placeholder models

---

## Sources

- [iRO Wiki Classic - Classes](https://irowiki.org/classic/Classes)
- [iRO Wiki - Classes](https://irowiki.org/wiki/Classes)
- [iRO Wiki Classic - Rebirth Walkthrough](https://irowiki.org/classic/Rebirth_Walkthrough)
- [iRO Wiki Classic - Transcendent](https://irowiki.org/classic/Transcendent)
- [iRO Wiki - Super Novice](https://irowiki.org/wiki/Super_Novice)
- [iRO Wiki - Adoption System](https://irowiki.org/wiki/Adoption_System)
- [iRO Wiki Classic - Weapons](https://irowiki.org/classic/Weapons)
- [iRO Wiki - Equipment](https://irowiki.org/wiki/Equipment)
- [iRO Wiki - Lord Knight](https://irowiki.org/wiki/Lord_Knight)
- [iRO Wiki - High Wizard](https://irowiki.org/wiki/High_Wizard)
- [iRO Wiki - Assassin Cross](https://irowiki.org/wiki/Assassin_Cross)
- [iRO Wiki - Biochemist](https://irowiki.org/wiki/Biochemist)
- [iRO Wiki - Stalker](https://irowiki.org/wiki/Stalker)
- [iRO Wiki - Minstrel](https://irowiki.org/wiki/Minstrel)
- [iRO Wiki - Gypsy](https://irowiki.org/wiki/Gypsy)
- [RateMyServer - Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus)
- [RateMyServer - Job Change Quest Guides](https://ratemyserver.net/quest_db.php?type=60000)
- [RateMyServer - Rebirth Quest](https://ratemyserver.net/quest_db.php?type=60000&qid=60026)
- [RateMyServer - Swordman Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus&op=1)
- [RateMyServer - Knight Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus&op=7)
- [RateMyServer - Pre-Re Weapon Database](https://ratemyserver.net/index.php?page=item_db&item_type=5)
- [RateMyServer - Pre-Re Armor Database](https://ratemyserver.net/index.php?page=item_db&item_type=4&item_class=16)
- [rAthena GitHub - job_stats.yml](https://github.com/rathena/rathena/blob/master/db/job_stats.yml)
- [rAthena GitHub - pre-re/job_exp.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/job_exp.yml)
- [rAthena GitHub - pre-re/job_basepoints.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/job_basepoints.yml)
- [rAthena GitHub - pre-re/item_db_equip.yml](https://github.com/rathena/rathena/blob/master/db/pre-re/item_db_equip.yml)
- [rAthena GitHub - HP/SP Table Update Issue #6427](https://github.com/rathena/rathena/issues/6427)
- [rAthena GitHub - ea_job_system.txt](https://github.com/rathena/rathena/blob/master/doc/ea_job_system.txt)
- [StrategyWiki - Ragnarok Online/Transcendence](https://strategywiki.org/wiki/Ragnarok_Online/Transcendence)
- [Ragnarok Fandom Wiki - Adoption System](https://ragnarok.fandom.com/wiki/Adoption_System)
- [ROGGH Library - Super Novice Guide](https://roggh.com/super-novice-class-guide/)
- [RO Classic - Job Change Quests](https://www.ragnarokclassic.com/jobquests/)
- [iRO Wiki - Levels](https://irowiki.org/wiki/Levels)
