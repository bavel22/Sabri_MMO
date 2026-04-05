# Transcendent & Rebirth System -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-Renewal) mechanics only.
> **Sources**: iRO Wiki Classic, RateMyServer (pre-re), rAthena source (pre-re branch), Ragnarok Wiki (Fandom), StrategyWiki, divine-pride.net, 99porings.com.
> **Purpose**: Authoritative reference for implementing the transcendent/rebirth system in Sabri_MMO.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Rebirth Process](#2-rebirth-process)
3. [Transcendent Class List](#3-transcendent-class-list)
4. [Transcendent Bonuses](#4-transcendent-bonuses)
5. [Transcendent-Only Skills (Per Class)](#5-transcendent-only-skills-per-class)
6. [Transcendent vs Non-Transcendent Differences](#6-transcendent-vs-non-transcendent-differences)
7. [Implementation Checklist](#7-implementation-checklist)
8. [Gap Analysis](#8-gap-analysis)

---

## 1. Overview

**Rebirth** (also called **Transcendence**) is the process by which a maxed-out second class character (Base Level 99, Job Level 50) resets to level 1 and starts over as a **High Novice**, eventually progressing through a **High First Class** and finally reaching a **Transcendent Second Class** -- a strictly superior version of their original class.

Transcendent characters retain access to ALL skills from their original first and second class, gain exclusive new transcendent-only skills, receive a permanent +25% HP/SP modifier, can reach Job Level 70 (vs 50 for normal), and gain access to transcendent-only equipment.

This system was introduced in Episode 10.1 (Einbroch) and represents the final tier of character progression in pre-Renewal RO. A character can only rebirth once.

---

## 2. Rebirth Process

### 2.1 Requirements

All of the following must be met before the Valkyrie will accept the rebirth:

| Requirement | Detail |
|------------|--------|
| Base Level | 99 (maximum) |
| Job Level | 50 (maximum for 2nd class) |
| Job Class | Must be a second class (Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, Crusader, Sage, Bard, Dancer, Monk, Alchemist, Rogue) |
| Zeny | 1,285,000 zeny donation (waivable -- see quest alternative below) |
| Weight | Under 100% weight capacity (some sources say under 500 weight) |
| Character Add-ons | **NONE** -- must remove Pushcart, Falcon, Cute Pet, Peco Peco, Halter Lead |
| Skill Points | ALL skill points must be allocated (0 unspent) |

**Quest Alternative to Zeny Payment**: The zeny fee can be waived by taking a quest from the NPC to hunt a **Runaway Book** that appears in the fields around Juno. Looting 1 **Captured Book** from it satisfies the requirement.

### 2.2 Quest/NPC Process (Step by Step)

1. **Travel to Juno** -- Go to the Sage Academy (Sage Castle) in the northwest area of Juno (Yuno).
2. **Talk to Metheus Sylphe** -- NPC inside the Sage Academy. Pay 1,285,000 zeny (or present the Captured Book from the alternative quest).
3. **Read the Book of Ymir** -- Interact with the Book of Ymir just north of Metheus Sylphe. Choose "Leave your body to it."
4. **Navigate the maze** -- After reading the book, you enter a maze with portals. Navigate: go south to the bottom of the map, turn left, descend stairs, select random portals until reaching a room with three entrances. Take the bottom-right portal from the bottom-left entrance.
5. **Spiral room** -- Enter the spiral room and proceed to its center, avoiding side portals.
6. **Activate the machine** -- Click the NPC bubble at the machine's bottom (positioning from the bottom-left works best).
7. **Arrive in Valhalla** -- After warping to Valhalla, walk north as far as possible.
8. **Talk to the Valkyrie** -- The Valkyrie checks all requirements. If met, rebirth proceeds.
9. **Rebirth completes** -- Character is reset and teleported to the town associated with their original first class (e.g., Prontera for Swordsman-line).

### 2.3 Reset to High Novice (Level 1/1)

Upon rebirth completion:

| What happens | Detail |
|-------------|--------|
| Class | Becomes **High Novice** (distinct sprite color from normal Novice) |
| Base Level | Reset to **1** |
| Job Level | Reset to **1** |
| All stats | Reset to **1** (all 6 base stats) |
| Stat points | Receive **100 stat points** (vs 48 for normal Novice) |
| All skills | **Removed** (reset to zero) |
| Equipment | Receive a **Knife [4]** and **Cotton Shirt [1]** as starting equipment |
| Location | Teleported to first-class town |

### 2.4 Stats/Skills Reset

- **Stats**: All 6 base stats (STR/AGI/VIT/INT/DEX/LUK) reset to 1. The character receives 100 freely-distributable stat points (52 more than a normal Novice's 48).
- **Skills**: ALL learned skills are removed. However, quest skills (First Aid, Play Dead) are **automatically granted** without needing to complete quests again.
- **Items**: All items in inventory are **retained**. Character-bound items, clothes dye, and quest completion flags are preserved.
- **Quests**: Previously completed quest flags remain (except job change quests which are re-done through the Book of Ymir process).
- **Zeny**: Current zeny is retained (minus the rebirth fee).

### 2.5 Previous Skills Retained?

**NO** -- all skills are wiped. The character must re-learn all first class and second class skills from scratch. However:

- **Quest skills** (First Aid, Play Dead) are automatically granted to High Novice without quests.
- When changing to a High First Class, the first class quest skills are also **automatically granted** without needing to complete the original quest.
- When reaching the transcendent second class, the character gains access to the same skill pool as their non-transcendent counterpart PLUS the new transcendent-exclusive skills.
- Total skill points: 9 (High Novice) + 49 (High First Class) + 69 (Transcendent Second Class, job 1-70) = 127 total skill points (vs 107 for non-transcendent: 9 + 49 + 49).

---

## 3. Transcendent Class List

### 3.1 High Novice

| Field | Value |
|-------|-------|
| Progression from | Rebirth (Base 99 + Job 50 second class) |
| Job Level Cap | 10 |
| Skill Points | 9 |
| Starting Stat Points | 100 (vs 48 for normal Novice) |
| Quest Skills | First Aid, Play Dead (auto-learned) |
| Sprite | Different color from normal Novice |

High Novice is functionally identical to regular Novice except for the bonus stat points, auto-learned quest skills, and the transcendent flag that enables +25% HP/SP and future access to transcendent-only equipment and skills.

### 3.2 High First Classes

High first classes are functionally identical to their normal counterparts. They use the same skills, same HP/SP coefficients, same ASPD tables. The only differences are the transcendent flag (25% HP/SP bonus applies) and a different sprite.

| High First Class | Normal Equivalent | Job Level Cap | Quest Skills |
|-----------------|-------------------|---------------|-------------|
| High Swordsman | Swordsman | 50 | Auto-learned (Moving HP Recovery, Fatal Blow, HP Recovery While Moving) |
| High Mage | Mage | 50 | Auto-learned (Energy Coat) |
| High Archer | Archer | 50 | Auto-learned (Arrow Crafting) |
| High Acolyte | Acolyte | 50 | Auto-learned (Holy Light) |
| High Thief | Thief | 50 | Auto-learned (Throw Sand, Back Slide) |
| High Merchant | Merchant | 50 | Auto-learned (Cart Revolution, Change Cart, Crazy Uproar) |

Job change to High First Class occurs at Job Level 10 (same as normal) by accessing the Book of Ymir in Juno (NOT the original job guild NPCs).

### 3.3 Transcendent Second Classes

Job change to Transcendent Second Class occurs at Job Level 40-50 as a High First Class (optimal at 50 for max skill points) by accessing the Book of Ymir in Juno.

#### 2-1 Transcendent Classes (from 2-1 normal classes)

| Transcendent Class | Normal Equivalent | Base Class | Job Level Cap | rAthena Job ID |
|-------------------|-------------------|-----------|---------------|----------------|
| **Lord Knight** | Knight | Swordsman | 70 | 4008 |
| **High Wizard** | Wizard | Mage | 70 | 4010 |
| **Sniper** | Hunter | Archer | 70 | 4012 |
| **High Priest** | Priest | Acolyte | 70 | 4009 |
| **Assassin Cross** | Assassin | Thief | 70 | 4013 |
| **Whitesmith** (Mastersmith) | Blacksmith | Merchant | 70 | 4011 |

#### 2-2 Transcendent Classes (from 2-2 normal classes)

| Transcendent Class | Normal Equivalent | Base Class | Job Level Cap | rAthena Job ID |
|-------------------|-------------------|-----------|---------------|----------------|
| **Paladin** | Crusader | Swordsman | 70 | 4015 |
| **Scholar** (Professor) | Sage | Mage | 70 | 4017 |
| **Clown** (Minstrel) | Bard | Archer | 70 | 4020 |
| **Gypsy** | Dancer | Archer | 70 | 4021 |
| **Champion** | Monk | Acolyte | 70 | 4016 |
| **Stalker** | Rogue | Thief | 70 | 4018 |
| **Creator** (Biochemist) | Alchemist | Merchant | 70 | 4019 |

---

## 4. Transcendent Bonuses

### 4.1 HP/SP Growth Bonus (+25%)

The single most impactful bonus. A permanent **1.25x multiplier** is applied to Max HP and Max SP calculations:

```
MaxHP = floor(BaseHP * (1 + VIT * 0.01) * 1.25)
MaxSP = floor(BaseSP * (1 + INT * 0.01) * 1.25)
```

This multiplier is applied AFTER the VIT/INT scaling but BEFORE additive and multiplicative equipment/buff modifiers. It applies to ALL transcendent classes (High Novice, High First Class, Transcendent Second Class).

**Example comparison at Level 99:**

| Class | VIT | Normal MaxHP | Trans MaxHP | Difference |
|-------|-----|-------------|-------------|-----------|
| Knight (99 VIT) | 99 | ~15,862 | ~19,828 | +3,966 (+25%) |
| Wizard (1 VIT) | 1 | ~4,467 | ~5,584 | +1,117 (+25%) |
| Priest (50 VIT) | 50 | ~7,924 | ~9,905 | +1,981 (+25%) |

### 4.2 Stat Point Bonus

High Novice starts with **100 stat points** instead of the normal Novice's **48 stat points**. This is a +52 point advantage at level 1.

Since the stat points gained per level-up are identical (formula: `floor(BaseLevelGained / 5) + 3`), the total stat points at level 99 are:

| Character Type | Starting Points | Points from Levels 2-99 | Total at Lv 99 |
|---------------|----------------|------------------------|---------------|
| Normal | 48 | 1,225 | 1,273 |
| Transcendent | 100 | 1,225 | 1,325 |

**Net advantage: +52 stat points**, roughly equivalent to one additional stat raised from 1 to ~30 or a meaningful boost across multiple stats.

### 4.3 Job Level 70 (vs 50 for Non-Trans)

Transcendent second classes can reach Job Level **70** (vs 50 for normal second classes). This provides:

- **+20 additional skill points** (69 total from job levels 1-70 vs 49 from 1-50)
- These extra points are essential for learning transcendent-exclusive skills, which typically require 20-30+ points
- **+20 additional job bonus stats** distributed at job levels 51-70 (see section 4.4)

#### Job EXP Table (Levels 51-70)

Transcendent second classes share the same job EXP table as normal second classes for levels 1-50, then continue:

| Job Level | EXP Required |
|-----------|-------------|
| 51 | 1,000,000 |
| 52 | 1,000,000 |
| 53 | 1,000,000 |
| 54 | 1,200,000 |
| 55 | 1,200,000 |
| 56 | 1,200,000 |
| 57 | 1,400,000 |
| 58 | 1,400,000 |
| 59 | 1,400,000 |
| 60 | 1,600,000 |
| 61 | 1,600,000 |
| 62 | 1,600,000 |
| 63 | 1,800,000 |
| 64 | 1,800,000 |
| 65 | 1,800,000 |
| 66 | 2,000,000 |
| 67 | 2,000,000 |
| 68 | 2,000,000 |
| 69 | 2,200,000 |
| 70 | -- (max) |

### 4.4 Job Bonus Stats (Complete Table, Levels 1-70)

Job bonus stats are free stat points automatically added at specific job levels. Transcendent second classes use the **same** bonus table as their normal counterparts for levels 1-50, then receive additional bonuses at levels 51-70.

**Total Job Bonus Stats per Transcendent Class (cumulative at Job Level 70):**

| Trans Class | STR | AGI | VIT | INT | DEX | LUK | Total |
|------------|-----|-----|-----|-----|-----|-----|-------|
| Lord Knight | +15 | +8 | +8 | +2 | +9 | +3 | 45 |
| High Wizard | +3 | +8 | +5 | +17 | +9 | +3 | 45 |
| Sniper | +4 | +11 | +3 | +5 | +14 | +8 | 45 |
| High Priest | +7 | +8 | +7 | +12 | +9 | +2 | 45 |
| Assassin Cross | +9 | +15 | +3 | +0 | +10 | +8 | 45 |
| Whitesmith | +6 | +7 | +6 | +6 | +12 | +8 | 45 |
| Paladin | +9 | +8 | +10 | +7 | +8 | +3 | 45 |
| Scholar | +6 | +9 | +4 | +13 | +11 | +2 | 45 |
| Clown (Minstrel) | +8 | +12 | +2 | +5 | +14 | +4 | 45 |
| Gypsy | +6 | +14 | +2 | +5 | +16 | +2 | 45 |
| Champion | +9 | +9 | +7 | +7 | +10 | +3 | 45 |
| Stalker | +9 | +11 | +4 | +3 | +12 | +6 | 45 |
| Creator (Biochemist) | +4 | +6 | +3 | +7 | +14 | +11 | 45 |

**Non-transcendent second classes at Job Level 50** receive ~24-27 total job bonus stats. The extra 20 levels provide roughly **+18-21 additional bonus stats**.

#### Lord Knight Job Bonus Levels 51-70 (detailed example)

| Job Level | Stat Gained |
|-----------|------------|
| 52 | +1 STR |
| 53 | +1 AGI |
| 56 | +1 STR |
| 57 | +1 STR |
| 58 | +1 VIT |
| 61 | +1 AGI |
| 62 | +1 DEX |
| 64 | +1 STR |
| 65 | +1 AGI |
| 68 | +1 VIT |
| 70 | +1 STR |

### 4.5 Additional Skill Points

| Character Type | Job Levels | Total Skill Points |
|---------------|------------|-------------------|
| Normal 2nd Class | 1-50 | 49 (no point at job 1) |
| Transcendent 2nd Class | 1-70 | 69 |
| **Extra** | 51-70 | **+20** |

These 20 extra skill points allow transcendent classes to:
1. Learn all their transcendent-exclusive skills (which typically cost 15-31 points total)
2. Have more flexibility in the original skill tree (can max more original skills)

### 4.6 Transcendent EXP Table

Transcendent classes use a **different base EXP table** that requires significantly more EXP per level:

| Base Level | Normal EXP | Trans EXP | Multiplier |
|-----------|-----------|----------|-----------|
| 10 | 2,399 | 4,559 | ~1.9x |
| 20 | 21,643 | 41,125 | ~1.9x |
| 30 | 92,640 | 176,016 | ~1.9x |
| 40 | 300,448 | 570,853 | ~1.9x |
| 50 | 819,200 | 1,556,480 | ~1.9x |
| 60 | 1,986,560 | 3,774,464 | ~1.9x |
| 70 | 4,427,776 | 8,412,775 | ~1.9x |
| 80 | 9,242,624 | 17,560,986 | ~1.9x |
| 90 | 18,389,000 | 34,939,100 | ~1.9x |
| 95 | 24,889,600 | 47,290,240 | ~1.9x |
| 98 | 38,539,000 | 73,224,100 | ~1.9x |

The multiplier is approximately **1.9x** throughout. Total base EXP 1-99 for transcendent: ~770,000,000 (vs ~405,000,000 for normal).

### 4.7 Equipment Access

Transcendent classes gain access to **Transcendent-only equipment** -- items marked with "Transcendent Only" in their equip requirements. Non-transcendent characters cannot wear these items. This includes powerful headgears, armors, and accessories that provide significant stat advantages.

### 4.8 Death Penalty

Transcendent classes have the same **1% death penalty** rate as normal classes (1% of EXP required for current level). However, because the transcendent EXP table requires ~1.9x more EXP per level, the absolute EXP lost per death is significantly higher.

---

## 5. Transcendent-Only Skills (Per Class)

All transcendent classes inherit ALL skills from their first and second class predecessors. The skills listed below are EXCLUSIVELY available to the transcendent version and cannot be learned by the non-transcendent equivalent.

### 5.1 Lord Knight (Trans of Knight)

*Inherits all Swordsman + Knight skills. 8 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Aura Blade** | 355 | 5 | Supportive | Self | Adds +20/40/60/80/100 ATK that bypasses DEF. Duration: 20*Lv seconds. |
| **Parry** (Parrying) | 356 | 10 | Supportive | Self | Block 23-50% of physical attacks. Requires Two-Handed Sword. Duration: 15-60s. SP: 50. |
| **Concentration** (Spear Dynamo) | 357 | 5 | Supportive | Self | +5-25% ATK and HIT, but -5-25% DEF. Duration: 25-85s. No cooldown (unlike Endure). |
| **Tension Relax** (Relax) | 358 | 1 | Supportive | Self | Triples HP regen. Must sit. Canceled by any action. SP: 15. |
| **Berserk** (Frenzy) | 359 | 1 | Supportive | Self | All SP consumed. MaxHP x3. ATK x2. DEF halved. FLEE=1. Cannot use skills/items/chat. HP drains 5% per 15s. Ends when HP<100. Duration: 300s. SP: 200. |
| **Spiral Pierce** (Clashing Spiral) | 397 | 5 | Offensive | Single | 5 hits. 150-350% ATK. Damage affected by spear weight. Range 4 cells. Requires spear. Cast: 0.3-1.5s. ACD: 1-2.5s. |
| **Head Crush** (Traumatic Blow) | 398 | 5 | Offensive | Single | 140-300% ATK. 20-60% Bleeding chance. SP: 23. |
| **Joint Beat** (Vital Strike) | 399 | 10 | Offensive | Single | 60-150% ATK. Random status debuff (ankle/wrist/knee/shoulder/waist/neck break). Requires Cavalier Mastery Lv3 + Head Crush Lv3. |

**Prereqs**: Aura Blade needs Magnum Break Lv5 + Two-Hand Quicken Lv3. Parry needs Two-Hand Quicken Lv3. Concentration needs Riding Lv1 + Aura Blade Lv5. Spiral Pierce needs Pierce Lv5 + Spear Stab Lv5 + Spear Boomerang Lv3.

**Total skill points to max all**: ~46 (exceeds 20 extra points -- must choose)

### 5.2 Paladin (Trans of Crusader)

*Inherits all Swordsman + Crusader skills. 4 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Gloria Domini** (Pressure) | 367 | 5 | Offensive | Single | Fixed damage = (ATK + MATK) * SkillLv. Ignores DEF/MDEF. Target loses 2*Lv% SP. Holy element. Range 9. Cast: 1-3s. |
| **Martyr's Reckoning** (Sacrifice) | 368 | 5 | Supportive | Self | Each attack drains 9% MaxHP but deals massive damage (1.0-1.4x multiplier). Duration: 30s or 5 hits. Needs Devotion Lv3 + Endure Lv1. SP: 100. |
| **Battle Chant** (Gospel) | 369 | 10 | Supportive | AoE (self) | Random positive effects on allies, random negative effects on enemies. Needs Cure Lv1 + Divine Protection Lv3. Duration: 60s. SP: 80-100. |
| **Shield Chain** (Rapid Smiting) | 480 | 5 | Offensive | Single | 500-1300% ATK across 5 hits. Damage based on shield + ATK. Range 4. Needs Shield Boomerang Lv3. |

**Total skill points to max all**: 25

### 5.3 High Wizard (Trans of Wizard)

*Inherits all Mage + Wizard skills. 6 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Soul Drain** | 364 | 10 | Passive | -- | +2% MaxSP per level. Recover SP when killing with single-target magic. |
| **Magic Crasher** (Stave Crasher) | 365 | 1 | Offensive | Single | Physical attack using MATK. Neutral element. Reduced by physical DEF. Range 9. SP: 8. |
| **Mystical Amplification** | 366 | 10 | Supportive | Self | Next spell deals +5-50% MATK. SP: 14-50. |
| **Napalm Vulcan** | 400 | 5 | Offensive | Single (3x3) | Ghost element. 70-350% MATK across 1-5 hits. 25% Curse chance at max. Needs Napalm Beat Lv5. |
| **Ganbantein** | 483 | 1 | Active | Ground (3x3) | 80% chance to cancel all ground-targeted spells in area. Catalyst: 1 Blue Gemstone + 1 Yellow Gemstone. SP: 40. |
| **Gravitational Field** | 484 | 5 | Offensive | Ground (5x5) | Earth element. 100-500% MATK per hit, 2-10 hits. Duration 5-9s. Reduces ASPD. Ignores elemental modifiers. Catalyst: 1 Blue Gem. Needs Mystical Amp Lv10 + Quagmire Lv1. |

**Total skill points to max all**: 32

### 5.4 Scholar / Professor (Trans of Sage)

*Inherits all Mage + Sage skills. 8 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Health Conversion** (Indulge) | 373 | 5 | Supportive | Self | Consume 10% MaxHP, recover 10-50% of consumed HP as SP. |
| **Soul Change** (Soul Exhale) | 374 | 1 | Active | Single (ally/enemy) | Swap your SP with target's SP. SP: 5. |
| **Soul Burn** (Soul Siphon) | 375 | 5 | Offensive | Single | 40-70% chance to drain all of target's SP (you gain it). On fail, you lose double SP. SP: 80-120. |
| **Mind Breaker** | 402 | 5 | Active | Single Enemy | Target gains +20-100% MATK but loses 12-60 Soft MDEF. |
| **Memorize** (Foresight) | 403 | 1 | Supportive | Self | Next 5 spells have -50% cast time. SP: 1. Cast: 5s. |
| **Wall of Fog** (Blinding Mist) | 404 | 1 | Active | Ground (5x3) | Blind enemies. Ranged attacks in fog have -75% damage and 75% miss. Duration: 20s. SP: 25. |
| **Spider Web** (Fiber Lock) | 405 | 1 | Active | Single Enemy | Immobilize target 8s (4s vs players). Halves FLEE. Fire damage breaks web (double damage). SP: 30. |
| **Double Casting** (Double Bolt) | 482 | 5 | Supportive | Self | 40-80% chance bolt spells (Fire/Cold/Lightning Bolt) cast twice. Duration: 90s. SP: 40-60. |

**Total skill points to max all**: 23

### 5.5 Sniper (Trans of Hunter)

*Inherits all Archer + Hunter skills. 4 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **True Sight** (Falcon Eyes) | 380 | 10 | Supportive | Self | +5 all stats. +2-20 ATK/HIT/CRIT. Duration: 30-120s. Needs Improve Concentration Lv1. |
| **Falcon Assault** | 381 | 5 | Offensive | Single | Formula: (DEX*0.8 + INT*0.2) * Lv * 3 + Blitz Beat Dmg * 3. Ignores DEF. Requires Falcon. Range 9. Needs Blitz Beat Lv5 + Steel Crow Lv3 + True Sight Lv5. |
| **Sharp Shooting** (Focused Arrow Strike) | 382 | 5 | Offensive | Single (12x3 line) | 200-400% ATK. Hits all enemies in narrow line toward target. Can critical. Range 9. Cast: 1.5-3.5s. Needs Double Strafe Lv5. |
| **Wind Walker** | 383 | 10 | Supportive | Self (party) | +2-20 FLEE. +4-25% move speed. Duration: 130-400s. Needs Improve Concentration Lv9. |

**Total skill points to max all**: 30

### 5.6 Clown / Minstrel (Trans of Bard)

*Inherits all Archer + Bard skills. 5 solo skills + 1 ensemble skill.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Arrow Vulcan** | 394 | 10 | Offensive | Single | 600-1500% ATK. Multiple hits. Consumes 1 arrow. Range 9. Cast: 3-4.5s. Needs Musical Strike Lv3 + Music Lessons Lv5. |
| **Marionette Control** | 396 | 1 | Active | Party Member | Transfer half of caster's base stats to target (capped at 99 each). Both immobilized. SP: 100. |
| **Tarot Card of Fate** | 489 | 5 | Active | Single Enemy | 8-40% chance. Random 1 of 14 tarot effects (Fool, Magician, High Priestess, Chariot, Strength, Lovers, Wheel, Hanged Man, Death, Temperance, Devil, Tower, Star, Sun). SP: 40. |
| **Longing for Freedom** | 487 | 5 | Active | Self | Move while performing ensemble skills. Speed: 60-100%. SP: 15. |
| **Wand of Hermode** | 488 | 5 | Active | AoE | WoE skill. Blocks magic attacks near warp portals. Strips buffs from affected. SP: 20-60. |
| **Sheltering Bliss** (ensemble) | 395 | 5 | Ensemble | AoE (9x9) | With Gypsy: prevents entry into area. Does not block ranged/magic. SP: 30-70. |

**Total skill points to max all**: 31

### 5.7 Gypsy (Trans of Dancer)

*Inherits all Archer + Dancer skills. Skills are IDENTICAL to Clown's transcendent skills.*

| Skill | rA ID | Notes |
|-------|-------|-------|
| Arrow Vulcan | 394 | Same as Clown |
| Marionette Control | 396 | Same as Clown |
| Tarot Card of Fate | 489 | Same as Clown |
| Longing for Freedom | 487 | Same as Clown |
| Wand of Hermode | 488 | Same as Clown |
| Sheltering Bliss | 395 | Ensemble with Clown (same) |

**Total skill points to max all**: 31

### 5.8 Assassin Cross (Trans of Assassin)

*Inherits all Thief + Assassin skills. 5 exclusive skills (+ 1 unimplemented).*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Advanced Katar Mastery** | 376 | 5 | Passive | -- | +12-20% Katar ATK. Needs Katar Mastery Lv7. |
| **Enchant Deadly Poison** (EDP) | 378 | 5 | Supportive | Self | ATK x2-4. Duration: 40-60s. Chance to inflict deadly poison. Consumes 1 Poison Bottle. Needs Create Deadly Poison Lv1 + Enchant Poison Lv6. SP: 60-100. |
| **Soul Destroyer** (Soul Breaker) | 379 | 10 | Offensive | Single | Formula: ATK*1-10 + INT*5-50 + random(500,1000). Ranged physical + magical hybrid. Range 9. Needs Enchant Poison Lv6 + Cloaking Lv3. Cast: 0.5-1s. |
| **Meteor Assault** | 406 | 10 | Offensive | AoE (5x5 self) | 80-800% ATK. Stun/Blind/Bleed chance scales with level. Needs Sonic Blow Lv10 + Katar Mastery Lv5. Cast: 0.5s. |
| **Create Deadly Poison** | 407 | 1 | Active | Self | Create 1 Poison Bottle. Success ~20% + 0.4*DEX + 0.2*LUK. Failure damages caster. SP: 50. |
| ~~Hallucination Walk~~ | -- | -- | -- | -- | **Unimplemented in pre-Renewal.** |

**Total skill points to max all**: 31

### 5.9 Stalker (Trans of Rogue)

*Inherits all Thief + Rogue skills. 4 exclusive skills (+ 1 unimplemented).*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Chase Walk** (Stealth) | 389 | 5 | Active | Self | Special hide immune to all detection skills (Sight, Ruwach, etc.). Can move while hidden. STR bonus on reveal. Needs Tunnel Drive Lv3 + Hiding Lv5. SP: 10. |
| **Reject Sword** (Counter Instinct) | 390 | 5 | Active | Self | Block up to 3 melee attacks. Deflected damage halved, reflected back at attacker. SP: 10-30. |
| **Preserve** | 475 | 1 | Active | Self | Prevents Plagiarism from overwriting copied skill for 10 minutes. SP: 30. |
| **Full Strip** (Full Divestment) | 476 | 5 | Active | Single Enemy | 12-20% chance to strip ALL equipment simultaneously (weapon, shield, armor, helm). Needs all 4 Strip skills at Lv5. SP: 22-30. |
| ~~Steal Backpack~~ | -- | -- | -- | -- | **Unimplemented in pre-Renewal.** |

**Total skill points to max all**: 16

### 5.10 Whitesmith / Mastersmith (Trans of Blacksmith)

*Inherits all Merchant + Blacksmith skills. 5 exclusive skills (+ 3 unimplemented).*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Melt Down** (Shattering Strike) | 384 | 10 | Supportive | Self | Chance to break enemy weapon (1-10%) and armor (0.7-7%) on each hit. Duration: 15-60s. SP: 50-90. |
| **Cart Boost** | 387 | 1 | Supportive | Self | +100% move speed while pushing cart. Duration: 60s. Requires Pushcart. SP: 20. |
| **Cart Termination** (High Speed Cart Ram) | 485 | 10 | Offensive | Single | Damage scales with cart weight. 5-50% Stun chance. Needs Cart Boost Lv1 + Mammonite Lv10 + Power-Thrust Lv5. SP: 15. |
| **Maximum Over Thrust** (Maximum Power-Thrust) | 486 | 5 | Supportive | Self | +20-100% ATK. Self-only (unlike PT which is party). Slight weapon break chance. Costs 3000-5000 zeny per use. Duration: 60s. Needs Power-Thrust Lv5. SP: 15. |
| **Weapon Refine** (Upgrade Weapon) | 477 | 10 | Active | Item | Refine weapons with higher success rate than NPC. SP: 30. |
| ~~Battle Machine Craft~~ | -- | -- | -- | -- | **Unimplemented in pre-Renewal.** |
| ~~Coin Craft~~ | -- | -- | -- | -- | **Unimplemented in pre-Renewal.** |
| ~~Nugget Craft~~ | -- | -- | -- | -- | **Unimplemented in pre-Renewal.** |

**Total skill points to max all (implemented)**: 26

### 5.11 Creator / Biochemist (Trans of Alchemist)

*Inherits all Merchant + Alchemist skills. 4 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Acid Demonstration** (Acid Bomb) | 490 | 10 | Offensive | Single | Formula: 0.7 * INT^2 * TargetVIT / (SkillLv * 50). Ignores DEF. Always hits. Chance to break weapon/armor. Consumes 1 Acid Bottle + 1 Bottle Grenade. Range 9. Needs Acid Terror Lv5 + Demonstration Lv5. SP: 30-50. |
| **Slim Potion Pitcher** (Aid Condensed Potion) | 478 | 10 | Supportive | Ground (7x7) | Throw condensed potions to heal all party/guild members in area. SP: 30. |
| **Full Chemical Protection** | 479 | 5 | Supportive | Single Ally | Protects all 4 equipment slots from break/strip. Needs 1 Glistening Coat per use. Duration: 120-600s. SP: 40. |
| **Plant Cultivation** | 491 | 2 | Active | Ground | Lv1: Summon random mushroom (50%). Lv2: Summon random plant (50%). Disabled in WoE. SP: 10. |

**Total skill points to max all**: 27

### 5.12 High Priest (Trans of Priest)

*Inherits all Acolyte + Priest skills. 4 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Assumptio** | 361 | 5 | Supportive | Single (self/ally) | Halves all incoming damage (PvM). Reduces by 1/3 (PvP). **Disabled in WoE**. Cast: 1-3s. Duration: 20-100s. Needs Impositio Manus Lv3 + Increase SP Recovery Lv3. SP: 20-60. |
| **Basilica** | 362 | 5 | Supportive | Ground (5x5 self) | Characters inside cannot attack or be attacked. Caster must remain stationary. Not usable in WoE/BG. Catalyst: 1 Blue Gem + 1 Yellow Gem + 1 Holy Water + 1 Red Gem. Needs Gloria Lv2. Duration: 20-40s. SP: 40-80. |
| **Meditatio** | 363 | 10 | Passive | -- | +2-20% Heal effectiveness, +1-10% MaxSP, +3-30% SP regen. |
| **Mana Recharge** (Spiritual Thrift) | 481 | 5 | Passive | -- | -4-20% SP cost for all skills. |

**Total skill points to max all**: 25

### 5.13 Champion (Trans of Monk)

*Inherits all Acolyte + Monk skills. 4 exclusive skills.*

| Skill Name | rA ID | Max Lv | Type | Target | Description |
|-----------|-------|--------|------|--------|-------------|
| **Palm Push Strike** (Raging Palm Strike) | 370 | 5 | Offensive | Single | 300-700% ATK. Knockback 3 cells. Can combo from Raging Thrust. Requires Fury state. Needs Iron Fists Lv7 + Fury Lv5. SP: 2-10. |
| **Tiger Knuckle Fist** (Glacier Fist) | 371 | 5 | Offensive | Single | 140-1020% ATK. Chance to immobilize 1s. Needs Palm Push Strike Lv3 + Chain Combo Lv3. SP: 4-12. |
| **Chain Crush Combo** | 372 | 10 | Offensive | Single | 200-1500% ATK over 5 hits. Costs 1 Spirit Sphere. Must follow Tiger Knuckle Fist or Raging Thrust. Can combo into Guillotine Fist (reduced sphere cost). Needs Tiger Knuckle Fist Lv3. SP: 4-22. |
| **Zen** (Dangerous Soul Collect) | 401 | 1 | Active | Self | Instantly summon 5 Spirit Spheres at once. SP: 20. |

**Total skill points to max all**: 21

---

## 6. Transcendent vs Non-Transcendent Differences

### Summary Comparison Table

| Feature | Normal 2nd Class | Transcendent 2nd Class |
|---------|-----------------|----------------------|
| Max Base Level | 99 | 99 |
| Max Job Level | 50 | **70** |
| Total Skill Points (2nd class) | 49 | **69** (+20) |
| HP/SP Modifier | 1.0x | **1.25x** (+25%) |
| Starting Stat Points (Novice) | 48 | **100** (+52) |
| Total Stat Points at Lv 99 | 1,273 | **1,325** (+52) |
| Job Bonus Stats (total) | ~24-27 | **~45** (~+18-21) |
| EXP Required per Level | Normal table | **~1.9x more** |
| Trans-Only Equipment | No | **Yes** |
| Trans-Only Skills | No | **Yes** (4-8 per class) |
| Quest Skills at Rebirth | Manual quests | **Auto-learned** |
| Sprite | Normal | **Unique color** |
| Base EXP 1-99 Total | ~405M | **~770M** |
| Death Penalty (absolute) | Lower | **Higher** (same %, more EXP) |
| Can equip trans-only gear | No | **Yes** |

### Key Gameplay Implications

1. **Raw Power**: +25% HP/SP + transcendent skills + 52 more stat points makes transcendent classes strictly superior in combat.
2. **Time Investment**: ~1.9x more EXP required means significantly longer leveling time.
3. **Skill Flexibility**: 20 extra skill points allows maxing trans skills AND having a fuller original skill build.
4. **Equipment Access**: Trans-only gear provides additional power that non-trans cannot access.
5. **WoE Impact**: Some trans skills (Assumptio, Basilica, Wand of Hermode) are disabled or limited in WoE to maintain balance.

---

## 7. Implementation Checklist

### 7.1 Database Changes

- [ ] `characters` table: `is_transcendent BOOLEAN DEFAULT FALSE` (already exists)
- [ ] `characters` table: `rebirth_count INTEGER DEFAULT 0` (already exists)
- [ ] `job_classes` table: Add all 13 transcendent class entries (already exists in schema)
- [ ] `job_classes` table: Add all 6 high first class entries
- [ ] `base_exp_table`: Populate transcendent EXP table (class_type = 'transcendent')
- [ ] `job_exp_table`: Add entries for job levels 51-70
- [ ] `job_bonus_stats`: Add bonus stats for job levels 51-70 per trans class
- [ ] `character_skills`: No schema changes needed (same structure)

### 7.2 Server-Side (index.js)

- [ ] **Rebirth Handler**: `rebirth:request` socket event
  - Validate: Base 99, Job 50, second class, weight < 100%, no addons, all skill points spent
  - Charge 1,285,000 zeny (or verify Captured Book quest)
  - Reset character: level 1/1, all stats to 1, 100 stat points, clear all skills
  - Set `is_transcendent = TRUE`, `rebirth_count = 1`
  - Set class to `high_novice`
  - Auto-learn quest skills (First Aid, Play Dead)
  - Relocate to first-class town
  - Emit `rebirth:complete` with new character data

- [ ] **High First Class Job Change**: Modify job change handler
  - At Job 10 as High Novice, allow change to matching High First Class
  - Auto-learn first class quest skills
  - Use Book of Ymir NPC (not original guild NPCs)

- [ ] **Transcendent Second Class Job Change**: Modify job change handler
  - At Job 40-50 as High First Class, allow change to trans second class
  - Use Book of Ymir NPC

- [ ] **HP/SP Calculation**: Already implemented (`TransMod = 1.25`)
  - Verify `calculateMaxHP()` and `calculateMaxSP()` use `is_transcendent`
  - Verify `getEffectiveStats()` passes `is_transcendent` correctly

- [ ] **EXP Table**: `getBaseExpTable(isTranscendent)` -- already implemented
  - Populate actual transcendent EXP values

- [ ] **Stat Points**: `getTotalStatPoints(baseLevel, isTranscendent)` -- already implemented
  - Returns 100 starting points for transcendent

- [ ] **Job Level Cap**: Enforce 70 for trans 2nd classes
  - Verify `processJobLevelUp()` checks class job level cap

- [ ] **Job Bonus Stats**: Add levels 51-70 data for all 13 trans classes
  - Verify `getJobBonusStats()` reads up to level 70

- [ ] **Death Penalty**: Same 1% rate, uses trans EXP table
  - Already works if EXP table is populated correctly

### 7.3 Transcendent Skill Implementation

For each transcendent class, implement the exclusive skills in `ro_skill_data_2nd.js` (or a new `ro_skill_data_trans.js`):

**Suggested ID ranges (continuing from existing scheme):**

| Trans Class | Suggested ID Range | Skill Count |
|------------|-------------------|------------|
| Lord Knight | 2000-2009 | 8 |
| Paladin | 2010-2019 | 4 |
| High Wizard | 2020-2029 | 6 |
| Scholar | 2030-2039 | 8 |
| Sniper | 2040-2049 | 4 |
| Clown | 2050-2059 | 6 |
| Gypsy | 2060-2069 | 6 (shared with Clown) |
| Assassin Cross | 2070-2079 | 5 |
| Stalker | 2080-2089 | 4 |
| Whitesmith | 2090-2099 | 5 |
| Creator | 2100-2109 | 4 |
| High Priest | 2110-2119 | 4 |
| Champion | 2120-2129 | 4 |

**Priority order** (based on player impact and complexity):

1. **High**: Lord Knight, Assassin Cross, High Wizard, Champion (core combat classes)
2. **Medium**: Paladin, High Priest, Sniper, Scholar (support/ranged)
3. **Lower**: Whitesmith, Creator, Stalker, Clown, Gypsy (specialized)

### 7.4 Client-Side (UE5 C++)

- [ ] `FCharacterData`: `bIsTranscendent` already exists
- [ ] Skill tree UI: Show transcendent skills in a separate tab or extended tree
- [ ] Character select: Show transcendent class names and sprites
- [ ] Rebirth UI: NPC interaction flow (Metheus Sylphe dialog, Book of Ymir interaction)
- [ ] Class sprites: Load different sprite sets for trans classes
- [ ] Stat window: Reflect increased job level cap (70)
- [ ] Skill point display: Show correct total available

### 7.5 NPC/Quest Implementation

- [ ] **Metheus Sylphe NPC** in Juno Sage Academy
  - Dialog: rebirth quest or zeny payment
  - Verify all rebirth requirements
- [ ] **Book of Ymir** interactable in Sage Academy
  - Trigger maze/Valhalla transition
- [ ] **Valkyrie NPC** in Valhalla
  - Final rebirth confirmation
  - Execute rebirth process
- [ ] **Runaway Book** monster in Juno fields (quest alternative)
  - Drops: Captured Book (100% on quest active)

---

## 8. Gap Analysis

### Already Implemented in Sabri_MMO

| Feature | Status | Location |
|---------|--------|----------|
| `is_transcendent` DB column | Done | `characters` table |
| `rebirth_count` DB column | Done | `characters` table |
| `job_classes` trans entries | Done | `job_classes` table (13 entries) |
| HP/SP TransMod (1.25x) | Done | `calculateMaxHP()`, `calculateMaxSP()` |
| Trans stat points (100) | Done | `getTotalStatPoints()` |
| Trans EXP table reference | Done | `getBaseExpTable(is_transcendent)` |
| `bIsTranscendent` client field | Done | `FCharacterData` |

### NOT Yet Implemented

| Feature | Priority | Complexity | Notes |
|---------|----------|-----------|-------|
| Rebirth handler (socket event) | HIGH | Medium | Core rebirth flow |
| Trans EXP table data | HIGH | Low | Populate actual values |
| Job level 70 cap enforcement | HIGH | Low | Server-side check |
| Job bonus stats 51-70 | HIGH | Low | Data entry |
| High First Class entries | MEDIUM | Low | DB + job change logic |
| Transcendent skill data | HIGH | High | 64 skills across 13 classes |
| Transcendent skill handlers | HIGH | Very High | Combat handlers per skill |
| Book of Ymir NPC | MEDIUM | Medium | Quest/dialog flow |
| Valkyrie NPC | MEDIUM | Medium | Rebirth execution |
| Trans-only equipment flag | LOW | Low | Item equip check |
| Trans class sprites | LOW | Medium | Art assets |
| Skill tree UI extension | MEDIUM | Medium | Client UI work |
| Job EXP table 51-70 | HIGH | Low | Data entry |
| Auto-learn quest skills on rebirth | MEDIUM | Low | Server logic |

### Estimated Implementation Phases

**Phase 1 -- Foundation** (1-2 sessions):
- Populate trans EXP table, job EXP 51-70, job bonus stats 51-70
- Implement rebirth handler with full validation
- Enforce job level 70 cap
- High first class job change path
- Auto-learn quest skills

**Phase 2 -- Priority Skills** (3-5 sessions):
- Lord Knight skills (Aura Blade, Berserk, Spiral Pierce, Parry, Concentration)
- Assassin Cross skills (EDP, Soul Destroyer, Meteor Assault, Create Deadly Poison)
- High Wizard skills (Mystical Amplification, Soul Drain, Napalm Vulcan)
- Champion skills (Zen, Palm Push Strike, Tiger Knuckle, Chain Crush)

**Phase 3 -- Support/Ranged Skills** (2-3 sessions):
- High Priest skills (Assumptio, Basilica, Meditatio, Mana Recharge)
- Paladin skills (Gloria Domini, Sacrifice, Gospel, Shield Chain)
- Sniper skills (True Sight, Falcon Assault, Sharp Shooting, Wind Walker)
- Scholar skills (Double Casting, Memorize, Mind Breaker, Health Conversion)

**Phase 4 -- Remaining Classes** (2-3 sessions):
- Whitesmith skills (Melt Down, Cart Boost, Cart Termination, Max Over Thrust)
- Creator skills (Acid Demonstration, Full Chemical Protection, Slim Potion Pitcher)
- Stalker skills (Chase Walk, Reject Sword, Preserve, Full Strip)
- Clown/Gypsy skills (Arrow Vulcan, Marionette Control, Tarot Card of Fate)

**Phase 5 -- Polish** (1 session):
- Trans-only equipment flag
- Rebirth NPC dialog flows
- Client UI updates (skill tree, stat window)
- Testing and verification

---

## References

- [iRO Wiki Classic - Transcendent](https://irowiki.org/classic/Transcendent)
- [iRO Wiki - Rebirth](https://irowiki.org/wiki/Rebirth)
- [iRO Wiki Classic - Rebirth Walkthrough](https://irowiki.org/classic/Rebirth_Walkthrough)
- [iRO Wiki Classic - Classes](https://irowiki.org/classic/Classes)
- [iRO Wiki Classic - Lord Knight](https://irowiki.org/classic/Lord_Knight)
- [iRO Wiki Classic - High Wizard](https://irowiki.org/classic/High_Wizard)
- [iRO Wiki Classic - Paladin](https://irowiki.org/classic/Paladin)
- [iRO Wiki Classic - Sniper](https://irowiki.org/classic/Sniper)
- [iRO Wiki Classic - Assassin Cross](https://irowiki.org/classic/Assassin_Cross)
- [iRO Wiki Classic - Stalker](https://irowiki.org/classic/Stalker)
- [iRO Wiki Classic - Mastersmith](https://irowiki.org/classic/Mastersmith)
- [iRO Wiki Classic - Champion](https://irowiki.org/classic/Champion)
- [iRO Wiki Classic - High Priest](https://irowiki.org/classic/High_Priest)
- [iRO Wiki Classic - Scholar](https://irowiki.org/classic/Scholar)
- [iRO Wiki Classic - Minstrel](https://irowiki.org/classic/Minstrel)
- [iRO Wiki Classic - Gypsy](https://irowiki.org/classic/Gypsy)
- [iRO Wiki Classic - Biochemist](https://irowiki.org/classic/Biochemist)
- [iRO Wiki Classic - Max HP](https://irowiki.org/classic/Max_HP)
- [iRO Wiki Classic - Max SP](https://irowiki.org/classic/Max_SP)
- [iRO Wiki - Stats](https://irowiki.org/wiki/Stats)
- [iRO Wiki - Levels](https://irowiki.org/wiki/Levels)
- [RateMyServer - Rebirth Quest](https://ratemyserver.net/quest_db.php?type=60000&qid=60026)
- [RateMyServer - Trans Base EXP Table](https://ratemyserver.net/index.php?page=misc_table_exp&op=2)
- [RateMyServer - Trans Job EXP Table](https://ratemyserver.net/index.php?page=misc_table_exp&op=9)
- [RateMyServer - Job Stat Bonuses](https://ratemyserver.net/index.php?page=misc_table_stbonus)
- [RateMyServer - Skill Database](https://ratemyserver.net/index.php?page=skill_db_class)
- [StrategyWiki - RO Transcendence](https://strategywiki.org/wiki/Ragnarok_Online/Transcendence)
