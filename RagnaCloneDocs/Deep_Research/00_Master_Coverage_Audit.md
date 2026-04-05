# Master Coverage Audit -- RO Classic Pre-Renewal

**Document Version**: 1.0
**Date**: 2026-03-22
**Purpose**: Definitive reference of ALL Ragnarok Online Classic (pre-Renewal) game systems, with coverage status tracking against existing RagnaCloneDocs research documents.
**Scope**: Pre-Renewal only (Episodes 1 through 13.2, before the June 2009 Renewal patch). Excludes Renewal-only features (3rd classes, fixed cast time, level penalty EXP/drops, instance dungeons, etc.).

---

## Table of Contents

1. [Coverage Legend](#coverage-legend)
2. [Core Mechanics](#1-core-mechanics)
3. [Combat Systems](#2-combat-systems)
4. [Class/Skill Systems](#3-classskill-systems)
5. [Item Systems](#4-item-systems)
6. [Social Systems](#5-social-systems)
7. [World/Navigation](#6-worldnavigation)
8. [Economy](#7-economy)
9. [Monster/Enemy Systems](#8-monsterenemy-systems)
10. [Status Effect Systems](#9-status-effect-systems)
11. [UI/UX Systems](#10-uiux-systems)
12. [Audio/Visual Systems](#11-audiovisual-systems)
13. [Miscellaneous Systems](#12-miscellaneous-systems)
14. [Coverage Matrix](#coverage-matrix)
15. [Missing/Overlooked Features](#missingoverlooked-features)
16. [Implementation Priority Order](#implementation-priority-order)
17. [Complete Feature Count](#complete-feature-count)

---

## Coverage Legend

| Symbol | Meaning |
|--------|---------|
| **FULL** | System fully documented in RagnaCloneDocs with formulas, data tables, and implementation notes |
| **PARTIAL** | System mentioned/outlined but missing key details (formulas, data, or edge cases) |
| **MENTIONED** | Referenced in passing but not researched in depth |
| **MISSING** | Not covered in any existing RagnaCloneDocs document |
| **N/A** | Not applicable to pre-Renewal or intentionally excluded from scope |

Doc references use shorthand: `01` = `01_Stats_Leveling_JobSystem.md`, `02` = `02_Combat_System.md`, etc.

---

## 1. Core Mechanics

### 1.1 Stat System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 6 base stats (STR/AGI/VIT/INT/DEX/LUK) | **FULL** | 01 | All formulas, bonus tables, derived effects |
| Stat point allocation formula | **FULL** | 01 | Cost = floor((x-1)/10)+2, cumulative tables |
| Stat points per base level | **FULL** | 01 | floor(BaseLevelGained/5)+3 |
| Total stat points at 99 (1,273) | **FULL** | 01 | Including 48 creation bonus |
| Transcendent bonus stat points (100 vs 48) | **PARTIAL** | 01 | Mentioned but trans classes not in scope |
| Stat bonuses (STR bonus every 10 STR, etc.) | **FULL** | 01, 02 | floor(STR/10)^2 tables |
| Integer-only math (all floors) | **FULL** | 01, 02 | Stated as universal rule |

### 1.2 Leveling System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Base level 1-99 | **FULL** | 01 | EXP tables referenced |
| Job level (Novice 1-10, 1st 1-50, 2nd 1-50) | **FULL** | 01 | Per-class configs |
| EXP tables (base and job) | **FULL** | 01, Master Plan | All tiers defined |
| Skill points per job level (1 per level) | **FULL** | 01, 03 | 49 total per tier |
| Death EXP penalty | **PARTIAL** | Master Plan | Mentioned; formula not detailed in research docs |
| Novice no-death-penalty | **MISSING** | -- | Novices lose 0 EXP on death; not documented |
| Super Novice 1% death penalty | **MISSING** | -- | SN loses only 1% on death |
| "Guardian angel" mechanic (SN at 99.0% EXP) | **MISSING** | -- | SN gets auto-buffs at 99.0% EXP milestones |

### 1.3 Job/Class System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Novice -> 1st class (Job Lv 10) | **FULL** | 01, Master Plan | 6 first classes |
| 1st -> 2nd class (Job Lv 40+) | **FULL** | 01, Master Plan | 12 second classes (2-1 and 2-2) |
| Class-specific HP/SP formulas | **PARTIAL** | 01, Master Plan | Per-class modifiers listed but incomplete |
| Class-specific ASPD tables | **PARTIAL** | 02 | Representative BTBA values; not all class/weapon combos |
| Weapon class restrictions | **FULL** | 05 | equip_jobs bitmask system |
| Job change quests | **PARTIAL** | 07 | NPC locations listed; quest details sparse |
| Rebirth/Transcendence system | **PARTIAL** | 01, Master Plan | Mentioned as post-launch; no implementation detail |
| Transcendent classes (13 trans classes) | **MENTIONED** | 03, Master Plan | Skill IDs listed; no formulas or implementation |
| Super Novice class | **MISSING** | -- | Access to all 1st-class skills; unique mechanics |
| Extended classes (Taekwon/Star Gladiator/Soul Linker) | **MISSING** | -- | Episode 10.3 classes; Feeling/Hatred systems |
| Extended classes (Gunslinger) | **MISSING** | -- | Episode 11.3; guns, bullets, coin skills |
| Extended classes (Ninja) | **MISSING** | -- | Episode 11.3; ninjutsu, kunai, shadow skills |
| Baby classes (adoption system) | **MISSING** | -- | Half stats, married parent requirement |

### 1.4 Movement System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Grid-based movement (cells) | **FULL** | 15 | 1 RO cell = 50 UE units |
| Base movement speed (150ms/cell) | **FULL** | 15 | All classes same base speed |
| Speed modifiers (IncAGI, Peco, potions) | **FULL** | 15 | Full table with stacking rules |
| Diagonal movement (8-direction) | **FULL** | 15 | Chebyshev distance noted |
| Sitting mechanic | **FULL** | 15 | Insert key, 2x regen, blocks actions |
| Pushcart speed penalty | **FULL** | 15 | Lv1 -50% through Lv5 -5% |
| Walking cancels casting | **FULL** | 15 | Movement threshold for interruption |
| Walk delay after skills | **PARTIAL** | 15 | Mentioned but per-skill data not compiled |

---

## 2. Combat Systems

### 2.1 Physical Damage

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| StatusATK formula (melee vs ranged) | **FULL** | 01, 02 | STR-based melee, DEX-based ranged |
| WeaponATK and variance | **FULL** | 02 | Per weapon level variance tables |
| Size penalty table (17 weapon types x 3 sizes) | **FULL** | 01, 02, 05 | Full table in all docs |
| Element modifier (10x10x4 table) | **FULL** | 02, 03 | All 10 elements, 4 levels |
| Refinement/upgrade ATK bonus | **FULL** | 02, 05 | +2/+3/+5/+7 per weapon level |
| Over-upgrade random bonus | **FULL** | 02, 05 | Beyond safe limit formula |
| Card/equipment bonuses (race/element/size) | **FULL** | 02, 05 | Additive within category, multiplicative across |
| Mastery ATK (passive skill flat damage) | **FULL** | 02 | Post-modifier, flat addition |
| Buff ATK (Impositio, Provoke, etc.) | **FULL** | 02 | Listed per buff |
| Complete damage pipeline (16 steps) | **FULL** | 02 | Step-by-step with formulas |
| Dual wield system (Assassin) | **PARTIAL** | 02 | BTBA formula; per-hand damage in CLAUDE.md |
| Weapon element endow (elemental arrows, converters) | **PARTIAL** | 02, 05 | Priority: Endow > Arrow > Weapon |
| Arrow ATK contribution | **PARTIAL** | 05 | Mentioned; detailed in ammunition skill |

### 2.2 Magical Damage

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| StatusMATK formula | **FULL** | 02 | INT + floor(INT/7)^2 |
| MATK min/max range | **FULL** | 02 | Two formula variants documented |
| Weapon MATK (staff/rod bonus) | **FULL** | 02 | Refinement bonus included |
| Skill multipliers per spell | **FULL** | 02, 03 | All bolt/AoE/multi-hit multipliers |
| Complete magical damage pipeline | **FULL** | 02 | 9-step pipeline |
| Bolt multi-hit (N = skill level) | **FULL** | 02 | Independent rolls per hit |
| Lex Aeterna (double next damage) | **PARTIAL** | 03 | Skill documented; pipeline integration noted |

### 2.3 Defense System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Hard DEF (percentage reduction from equipment) | **FULL** | 02 | Both rAthena and iRO formulas |
| Soft DEF (flat reduction from VIT) | **FULL** | 02 | VIT-based formula with tables |
| Hard MDEF (equipment MDEF) | **FULL** | 02 | Percentage-based |
| Soft MDEF (INT-based MDEF) | **FULL** | 02 | Multiple formula variants |
| Armor refinement DEF bonus | **FULL** | 02, 05 | floor((3+refine)/4) |
| DEF bypass mechanics (crit, Ice Pick, etc.) | **FULL** | 02 | Full table of bypass sources |
| Multi-monster DEF penalty | **FULL** | 02 | -5% per attacker beyond 2 |
| Armor element (body armor determines player element) | **PARTIAL** | 02, 05 | Mentioned but not deeply detailed |

### 2.4 HIT/FLEE System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| HIT formula (175 + BaseLv + DEX + bonus) | **FULL** | 02 | Player and monster formulas |
| FLEE formula (100 + BaseLv + AGI + bonus) | **FULL** | 02 | Player and monster formulas |
| Hit rate (80 + HIT - FLEE, clamped 5-95%) | **FULL** | 02 | With practical examples |
| Multi-monster FLEE penalty (-10% per attacker >2) | **FULL** | 02 | Full table 1-11+ attackers |
| Perfect Dodge (1 + floor(LUK/10) + bonus) | **FULL** | 02 | Pre-HIT/FLEE check, no cap |

### 2.5 Critical Hit System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Critical rate formula | **FULL** | 02 | floor(LUK*0.3)+1+bonus |
| Crit shield (target LUK defense) | **FULL** | 02 | floor(targetLUK/5) or /0.2 |
| Critical damage (1.4x, max ATK, bypass DEF) | **FULL** | 02 | Details on what is bypassed |
| Katar double crit rate | **PARTIAL** | 02, 05 | Display vs actual |
| Skills cannot crit (general rule) | **PARTIAL** | 02 | Rule stated; exceptions not listed |

### 2.6 ASPD System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Pre-renewal ASPD formula | **FULL** | 02 | WD-based formula with AGI/DEX |
| BTBA (Base Time Between Attacks) per class | **PARTIAL** | 02 | Representative values; not every combo |
| Speed modifiers (potions, skills) | **FULL** | 02 | Full SM table with stacking rules |
| Dual wield ASPD (Assassin) | **PARTIAL** | 02 | 0.7 * (BTBA_Main + BTBA_Off) |
| Shield ASPD penalty | **PARTIAL** | 02 | Mentioned (-5 to -10) |
| ASPD display (0-190 scale, 190=fastest pre-re) | **PARTIAL** | 02 | Scale mentioned but cap details sparse |

### 2.7 Auto-Attack System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Auto-attack targeting (click enemy) | **FULL** | 15 | Click-to-attack documented |
| Attack delay from ASPD | **FULL** | 02 | Formula for ms between attacks |
| Attack animation | **PARTIAL** | 15 | Mentioned; not visually specified |
| Auto-attack element (weapon/endow/arrow) | **FULL** | 02, 05 | Priority system documented |
| Ranged auto-attack (bow) with arrow consumption | **PARTIAL** | 05 | Arrow consumption noted |

### 2.8 Skill Targeting and Casting

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Cast time formula (1 - DEX/150) | **FULL** | 03 | No fixed cast in pre-renewal |
| After-cast delay (ACD) | **FULL** | 03 | Not reduced by DEX; Bragi reduces |
| Skill interruption on damage | **FULL** | 03 | Endure/Phen prevent |
| Target types (self/single/ground/aoe/passive) | **FULL** | 03 | Full taxonomy |
| Skill tree prerequisites | **FULL** | 03 | Per-skill prereqs documented |
| Gem/catalyst consumption | **PARTIAL** | 03, 05 | Some skills list gem cost |
| Skill cooldowns vs global ACD | **PARTIAL** | 03 | Distinction stated but per-skill data varies |

---

## 3. Class/Skill Systems

### 3.1 First Classes (6 classes, ~56 skills total)

| Class | ID Range | Skill Count | Coverage | Doc |
|-------|----------|-------------|----------|-----|
| **Novice** | 1-3 | 3 (Basic Skill, First Aid, Play Dead) | **FULL** | 03 |
| **Swordsman** | 100-109 | 10 (7 regular + 3 quest) | **FULL** | 03 |
| **Mage** | 200-213 | 14 | **FULL** | 03 |
| **Archer** | 300-306 | 7 (6 regular + Arrow Crafting) | **FULL** | 03 |
| **Thief** | 500-509 | 10 (6 regular + 4 quest: Throw Sand, Back Slide, Pick Stone, Throw Stone) | **FULL** | 03 |
| **Merchant** | 600-609 | 10 (8 regular + 2 quest: Cart Revolution, Change Cart) | **FULL** | 03 |
| **Acolyte** | 400-414 | 15 (13 regular + 2 quest: Holy Light, B.S. Sacramenti) | **FULL** | 03 |

### 3.2 Second Classes 2-1 (6 classes)

| Class | ID Range | Skill Count | Coverage | Doc |
|-------|----------|-------------|----------|-----|
| **Knight** | 700-710 | 11 + quest skills (Charge Attack) | **FULL** | 03 |
| **Wizard** | 800-813 | 14 | **FULL** | 03 |
| **Hunter** | 900-917 | 18 | **FULL** | 03 |
| **Priest** | 1000-1018 | 19 | **FULL** | 03 |
| **Assassin** | 1100-1111 | 12 | **FULL** | 03 |
| **Blacksmith** | 1200-1230 | ~20 (including forging sub-skills) | **FULL** | 03 |

### 3.3 Second Classes 2-2 (6 classes)

| Class | ID Range | Skill Count | Coverage | Doc |
|-------|----------|-------------|----------|-----|
| **Crusader** | 1300-1313 | 14 | **FULL** | 03 |
| **Sage** | 1400-1421 | 22 | **FULL** | 03 |
| **Bard** | 1500-1537 | ~20 (solo + ensemble) | **FULL** | 03 |
| **Dancer** | 1520-1557 | ~20 (solo + ensemble) | **FULL** | 03 |
| **Rogue** | 1700-1718 | 19 | **FULL** | 03 |
| **Alchemist** | 1800-1815 | 16 | **FULL** | 03 |
| **Monk** | 1600-1615 | 16 | **FULL** | 03 |

### 3.4 Transcendent Classes (13 classes)

| Class | Base Class | New Skills | Coverage | Doc |
|-------|-----------|------------|----------|-----|
| **High Novice** | Novice | -- | **MENTIONED** | 03 |
| **Lord Knight** | Knight | ~5 (Aura Blade, Parrying, Berserk, Spiral Pierce, Joint Beat) | **PARTIAL** | 03 |
| **High Wizard** | Wizard | ~4 (Ganbantein, Gravitation, Napalm Vulcan, Soul Drain) | **PARTIAL** | 03 |
| **Sniper** | Hunter | ~5 (True Sight, Falcon Assault, Sharp Shooting, Wind Walk, etc.) | **PARTIAL** | 03 |
| **High Priest** | Priest | ~5 (Assumptio, Basilica, Meditatio, etc.) | **PARTIAL** | 03 |
| **Assassin Cross** | Assassin | ~5 (Meteor Assault, Soul Destroyer, Create Deadly Poison, Enchant Deadly Poison, etc.) | **PARTIAL** | 03 |
| **Whitesmith** | Blacksmith | ~5 (Cart Boost, High Speed Cart Ram, Cart Termination, etc.) | **PARTIAL** | 03 |
| **Paladin** | Crusader | ~5 (Pressure, Sacrifice, Gospel, Shield Chain, Defender) | **PARTIAL** | 03 |
| **Scholar/Professor** | Sage | ~5 (Soul Change, Spider Web, Mind Breaker, Memo, etc.) | **PARTIAL** | 03 |
| **Clown/Minstrel** | Bard | ~3 (Tarot Card, Hermode, Wand of Hermode) | **PARTIAL** | 03 |
| **Gypsy** | Dancer | ~3 (Tarot Card, Closing Ceremony, Wand of Hermode) | **PARTIAL** | 03 |
| **Stalker** | Rogue | ~5 (Full Strip, Preserve, Chase Walk, Stealth, etc.) | **PARTIAL** | 03 |
| **Creator/Biochemist** | Alchemist | ~5 (Acid Demonstration, Slim Potion Pitcher, Plant Cultivation, etc.) | **PARTIAL** | 03 |
| **Champion** | Monk | ~5 (Zen, Root, Chain Crush Combo, Glacier Fist, etc.) | **PARTIAL** | 03 |

### 3.5 Extended Classes (Pre-Renewal, Episode 10.3-11.3)

| Class | Skill Count | Coverage | Doc |
|-------|-------------|----------|-----|
| **Super Novice** | Access to all 1st-class skills | **MISSING** | -- |
| **Taekwon Kid** | ~16 (kicks, stances, running) | **MISSING** | -- |
| **Star Gladiator** | ~15 (Feeling, Hatred, Warmth, Comfort, Union) | **MISSING** | -- |
| **Soul Linker** | ~30+ (Spirit Link buffs for every class, Ka- spells, Es- spells) | **MISSING** | -- |
| **Gunslinger** | ~25 (5 gun types, Coin system, Desperado, etc.) | **MISSING** | -- |
| **Ninja** | ~20 (Ninjutsu, Kunai, Shadow clones, etc.) | **MISSING** | -- |

### 3.6 Skill Mechanics Not Tied to Specific Classes

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Quest skills (platinum skills) system | **PARTIAL** | 03, 07 | Some quests listed; NPC locations provided |
| Skill level selection (per-use) | **PARTIAL** | 03 | Mentioned; implementation in Sabri noted |
| Ground-targeted skills | **FULL** | 03 | Target type documented |
| Self-centered AoE skills | **FULL** | 03 | Target type documented |
| Ensemble skills (Bard+Dancer duets) | **FULL** | 03 | 8 ensemble skills documented |
| Performance system (songs/dances) | **FULL** | 03 | Movement, overlap, cancellation rules |
| Trap system (Hunter) | **FULL** | 03 | Placement, trigger, duration, damage |
| Spirit sphere system (Monk) | **FULL** | 03 | Counter resource, consumption |
| Combo system (Monk) | **FULL** | 03 | Chain timing, skill whitelist |
| Plagiarism (Rogue) | **FULL** | 03 | Copy mechanics, whitelist |
| Forging/crafting (Blacksmith) | **FULL** | 03 | Recipes, success formula |
| Pharmacy/brewing (Alchemist) | **FULL** | 03 | Recipes, success formula |

---

## 4. Item Systems

### 4.1 Item Categories

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Usable items (type 0, 2, 11) | **FULL** | 05 | HP/SP potions, cures, scrolls, teleport |
| Equipment (type 4 = armor, 5 = weapon) | **FULL** | 05 | All subtypes |
| Miscellaneous/Etc items (type 3) | **FULL** | 05 | Loot, crafting mats, ores, quest items |
| Ammunition (type 10) | **FULL** | 05 | Arrows, bullets, spheres, shuriken, kunai |
| Cards (type 6) | **FULL** | 05 | Compounding, effects |
| Pet items (type 7 eggs, type 8 armor) | **PARTIAL** | 05, 12 | Types mentioned; not all items listed |
| Unidentified items (Magnifier system) | **PARTIAL** | 05, 07 | Magnifier sold; identification behavior sparse |

### 4.2 Equipment System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 10 equipment slots | **FULL** | 05, 09 | Head top/mid/low, armor, weapon, shield, garment, footgear, acc x2 |
| Headgear combo positions (upper+mid, etc.) | **FULL** | 05 | All 6 combos listed |
| Two-handed weapon shield lock | **FULL** | 05 | Full list of 2H weapon types |
| Class restrictions (equip_jobs bitmask) | **FULL** | 05 | Per-type patterns |
| Level restrictions | **FULL** | 05 | Weapon level to base level mapping |
| Equipment visual on character | **PARTIAL** | Master Plan | Art pipeline dependency |
| Equipment sets/combos (set bonuses) | **MISSING** | -- | Some equipment grants bonuses when worn together |
| Slotted vs unslotted variants | **PARTIAL** | 05 | Card slots 0-4 mentioned; variant list not compiled |
| Costume/shadow equipment | **N/A** | -- | Renewal-only feature |

### 4.3 Weapon System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 17 weapon types (including guns, huuma) | **FULL** | 05 | Full table with traits |
| Weapon levels (1-4) | **FULL** | 05 | Variance, refinement rates |
| Size modifier table | **FULL** | 05 | 17 types x 3 sizes |
| Weapon element | **PARTIAL** | 02, 05 | System documented; not all weapons listed |
| Named/unique weapons | **MISSING** | -- | Special quest/MVP weapons with unique effects |

### 4.4 Refine/Upgrade System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Refinement levels (+0 to +10) | **FULL** | 05, 07 | ATK/DEF bonuses per level |
| Safety limits per weapon level | **FULL** | 05, 07 | +7/+6/+5/+4 for WLv 1/2/3/4 |
| Success rates beyond safety | **FULL** | 07 | Full table +5 through +10 |
| Failure consequence (destruction) | **FULL** | 07 | Cards lost with item |
| Refinement materials (ores) | **FULL** | 07 | Phracon/Emveretarcon/Oridecon/Elunium |
| Refine NPC locations | **FULL** | 07 | Hollegren/Suhnbi |
| Enriched ores (improved rates) | **PARTIAL** | 07 | Mentioned; source not detailed |
| HD ores (prevent destruction) | **PARTIAL** | 07 | Mentioned; late pre-renewal addition |
| Ore processing (5 rough -> 1 pure) | **FULL** | 07 | NPC documented |
| Visual glow at high refine (+7, +8, +9, +10) | **MENTIONED** | Master Plan | Art requirement only |

### 4.5 Card System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Card compounding (insert into slotted equipment) | **FULL** | 05 | Right-click flow |
| Card slot types (weapon/armor/garment/footgear/accessory/headgear/shield) | **FULL** | 05 | Per-slot card categories |
| Card effect types (stat/race/element/size/proc/drain/auto-cast/skill grant) | **FULL** | 05 | 8 effect types with examples |
| Card removal (NPC only) | **FULL** | 05 | No player self-removal |
| Stacking rules (same card in multiple slots) | **PARTIAL** | 05 | Basic rule stated; exceptions not listed |
| Card naming system (prefix/suffix on equipment) | **PARTIAL** | 05 | System exists; full naming data sparse |
| MVP cards (boss-exclusive drops) | **PARTIAL** | 04, 05 | Drop system mentioned; card list incomplete |

### 4.6 Consumable System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| HP restoration (Red/Orange/Yellow/White Potion, etc.) | **FULL** | 05, 13 | Fixed amounts |
| SP restoration (Grape Juice, Blue Potion) | **FULL** | 05, 13 | Not NPC sold; economy detail |
| Status cure items (Green Potion, Panacea, Royal Jelly) | **FULL** | 05 | Per-status cure |
| Buff potions (Awakening, Berserk, Speed) | **FULL** | 05, 02 | ASPD modifiers in combat doc |
| Stat food (+stat for 20-30 minutes) | **FULL** | 05 | +1 to +10 stat types |
| Teleport items (Fly Wing, Butterfly Wing) | **FULL** | 05 | Zone flag interaction |
| Gem/catalyst items (Blue/Yellow/Red Gemstone) | **FULL** | 05 | Consumed by skills |
| Yggdrasil Berry/Seed (full HP/SP restore) | **PARTIAL** | 05 | Mentioned; WoE restrictions noted |
| Box items (Old Blue Box, Old Card Album) | **PARTIAL** | 05 | Mentioned; random loot tables not detailed |
| Elemental converters (Fire/Water/Wind/Earth endow) | **PARTIAL** | 05 | Mentioned; detailed in server implementation |
| Scroll items (cast spell on use) | **PARTIAL** | 05 | Category listed |
| Dead Branch / Bloody Branch (summon random monster) | **MISSING** | -- | Summons random monster from mob DB |

### 4.7 Weight System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Weight formula (2000 + STR * 30) | **FULL** | 05, 09 | Class modifiers noted |
| 50% overweight (blocks HP/SP regen items) | **FULL** | 05, 09 | Threshold documented |
| 70% overweight (blocks natural regen, item creation) | **PARTIAL** | 09 | UI doc has 70%; combat doc has 50% |
| 90% overweight (blocks attacks/skills) | **FULL** | 05, 09 | Threshold documented |
| Item weight values | **PARTIAL** | 05 | Some items listed; no complete table |

---

## 5. Social Systems

### 5.1 Party System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Party creation (/organize) | **FULL** | 08 | Leader + 11 max members |
| Party invite/accept/reject | **FULL** | 08 | Remote invite (no proximity) |
| EXP distribution (Each Take / Even Share) | **FULL** | 08 | Formula, bonus per member |
| Level restriction for Even Share (15 levels) | **FULL** | 08 | All online members must be within range |
| Same-map requirement for shared EXP | **FULL** | 08 | Detailed |
| Party EXP bonus (+25% per member) | **FULL** | 08 | Even Share only |
| Item distribution modes | **FULL** | 08 | 4 modes documented |
| Party leader mechanics (transfer, kick, disband) | **FULL** | 08 | All leader actions |
| Party HP/SP display | **FULL** | 08 | Real-time bars |
| Party chat (/p) | **FULL** | 08 | Cross-map, green color |
| Party minimap dots | **PARTIAL** | 08, 09 | Mentioned; minimap not implemented |
| Basic Skill Lv6 party requirement | **FULL** | 03, 08 | Prerequisite documented |

### 5.2 Guild System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Guild creation (Emperium + /guild) | **FULL** | 08 | Permanent Guild Master |
| Guild levels (1-50) | **FULL** | 08 | EXP donation system |
| Guild capacity (16 base + Extension) | **FULL** | 08 | +6/level up to 76 |
| 20 configurable positions/ranks | **FULL** | 08 | Invite/kick/tax/storage permissions |
| Guild tax system (base EXP only, 0-50%) | **FULL** | 08 | Per-position rates |
| Guild skills (12 total: passive + active) | **FULL** | 08 | Complete list with effects |
| Emergency Call (WoE only) | **FULL** | 08 | 5s uninterruptible cast |
| Battle Orders / Regeneration / Restore | **FULL** | 08 | Shared 5-min cooldown |
| Guild alliances (max 3) | **FULL** | 08 | Green names in WoE |
| Guild enemies/antagonists | **FULL** | 08 | Red names, attack anywhere |
| Guild storage (100 slots/level, max 500) | **FULL** | 08 | Permission-based access |
| Guild emblem (24x24 BMP) | **FULL** | 08 | Hot pink transparency |
| Guild dissolution (/breakguild) | **FULL** | 08 | All members must be kicked first |
| Guild notice/announcement | **PARTIAL** | 08 | Mentioned; not detailed |
| Guild chat (/g or %) | **PARTIAL** | 08 | Channel exists; format not detailed |

### 5.3 Chat System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Normal/local chat (visible to nearby players) | **FULL** | 09, Master Plan | Default channel |
| Global/shout chat | **PARTIAL** | Master Plan | Mentioned as working |
| Zone chat (same map) | **FULL** | Master Plan | Channel listed |
| Party chat (green) | **FULL** | 08, Master Plan | Cross-map |
| Guild chat (yellow/green) | **PARTIAL** | 08, Master Plan | Channel listed |
| Whisper/PM (pink) | **PARTIAL** | Master Plan | Channel listed; block system partial |
| Trade chat | **PARTIAL** | Master Plan | Channel listed |
| System announcements (red) | **PARTIAL** | Master Plan | Channel listed |
| Chat rooms (Alt+C, player-created) | **MISSING** | -- | Visible chat bubbles above head |
| Chat commands (/commands) | **PARTIAL** | 15 | Some commands listed |
| Chat log filtering (tabs) | **PARTIAL** | 09 | UI layout described |

### 5.4 Friend List System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Add/remove friends | **PARTIAL** | Master Plan | Listed as feature |
| Online/offline status | **PARTIAL** | Master Plan | Listed as feature |
| Current zone display | **PARTIAL** | Master Plan | Listed as feature |
| Quick whisper from friend list | **PARTIAL** | Master Plan | Listed as feature |

### 5.5 Block/Ignore System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Block whispers from player | **PARTIAL** | Master Plan | Listed as feature |
| Unblock player | **PARTIAL** | Master Plan | Listed as feature |
| /ex and /exall commands | **MISSING** | -- | Block all PMs command not documented |

### 5.6 Marriage System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Marriage ceremony (Prontera Church NPC) | **MISSING** | -- | Requires Base Lv 45, different genders |
| Wedding Ring (summoning skill) | **MISSING** | -- | Spouse summon while ring equipped |
| Marriage skills (2 couple skills) | **MISSING** | -- | Happy Break, Undying Love |
| Divorce mechanic | **MISSING** | -- | Ring removal |
| Jawaii honeymoon island access | **MISSING** | -- | Married couples only |

### 5.7 Adoption System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Baby class transformation | **MISSING** | -- | Married couple adopts 1st class/Novice |
| Parent requirements (Lv 70+, ring equipped) | **MISSING** | -- | Both parents online |
| Baby class stat reduction (halved) | **MISSING** | -- | Skills at half effectiveness |
| Baby skill (Tantrum) | **MISSING** | -- | Unique baby-only skill |

### 5.8 Emote/Emotion System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 71+ emotes via /emotion commands | **MISSING** | -- | /an, /heh, /ok, /no, etc. |
| Alt+L emote menu | **MISSING** | -- | Visual emote selector |
| Macro system (Alt+M) for emote shortcuts | **MISSING** | -- | ALT+1 through ALT+0 |
| Rock/Paper/Scissors emotes | **MISSING** | -- | Ctrl+-, Ctrl+=, Ctrl+\ |
| Basic Skill Lv3 enables emotes | **FULL** | 03 | Documented in skill tree |

### 5.9 Duel System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| /duel command to challenge player | **MISSING** | -- | 1v1 PvP outside PvP maps |
| Duel acceptance/rejection | **MISSING** | -- | Mutual consent required |
| Duel area restriction | **MISSING** | -- | Cannot leave certain range |

---

## 6. World/Navigation

### 6.1 Towns

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Prontera (capital) | **FULL** | 06 | NPCs, connections, services |
| Geffen (magic city) | **FULL** | 06 | Magic Academy, Tower |
| Payon (eastern village) | **FULL** | 06 | Archer Village, Cave |
| Alberta (port town) | **FULL** | 06 | Merchant Guild, ships |
| Morroc (desert oasis) | **FULL** | 06 | Thief Guild, Pyramids |
| Izlude (island town) | **PARTIAL** | 06 | Swordsman Guild, arena |
| Al De Baran (clock tower) | **PARTIAL** | 06 | Listed; minimal detail |
| Comodo (beach town) | **PARTIAL** | 06 | Listed; Bard/Dancer guild |
| Umbala (tree village) | **PARTIAL** | 06 | Listed; Niflheim access |
| Amatsu (Japanese island) | **PARTIAL** | 06 | Ship from Alberta |
| Louyang (Chinese city) | **PARTIAL** | 06 | Ship from Alberta |
| Ayothaya (Thai city) | **PARTIAL** | 06 | Ship from Alberta |
| Jawaii (honeymoon island) | **PARTIAL** | 06 | Married couples only |
| Moscovia (Russian village) | **PARTIAL** | 06 | Ship from Alberta |
| Lutie/Xmas (Christmas village) | **PARTIAL** | 06 | From Al De Baran |
| Niflheim (underworld) | **PARTIAL** | 06 | From Umbala |
| Juno/Yuno (sky city, Schwartzvald capital) | **PARTIAL** | 06 | Sage Academy |
| Einbroch (industrial city) | **PARTIAL** | 06 | Blacksmith Guild |
| Lighthalzen (Rekenber HQ) | **PARTIAL** | 06 | Biolab dungeon |
| Hugel (garden city) | **PARTIAL** | 06 | Hunter Guild |
| Rachel (temple city, Arunafeltz capital) | **PARTIAL** | 06 | Frozen fields |
| Veins (volcanic town) | **PARTIAL** | 06 | Thor Dungeon |

### 6.2 Field Maps

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Prontera fields (8+) | **PARTIAL** | 06 | Some connections listed |
| Geffen fields (10+) | **PARTIAL** | 06 | Some connections listed |
| Payon fields (8+) | **PARTIAL** | 06 | Some connections listed |
| Morroc/Sograt desert fields (15+) | **PARTIAL** | 06 | Some connections listed |
| Mt. Mjolnir fields (8+) | **PARTIAL** | 06 | Some connections listed |
| Yuno fields | **PARTIAL** | 06 | Mentioned |
| Einbroch fields | **PARTIAL** | 06 | Mentioned |
| Hugel fields | **PARTIAL** | 06 | Mentioned |
| Rachel fields | **PARTIAL** | 06 | Mentioned |

### 6.3 Dungeons

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Prontera Culvert/Sewers (4 floors) | **PARTIAL** | 06 | Listed |
| Geffen Tower (3 floors) | **PARTIAL** | 06 | Listed |
| Geffen Dungeon (2 floors) | **PARTIAL** | 06 | Listed |
| Payon Cave (5 floors) | **PARTIAL** | 06 | Listed |
| Morroc Pyramids (4 floors) | **PARTIAL** | 06 | Listed |
| Sphinx (5 floors) | **PARTIAL** | 06 | Listed |
| Orc Dungeon (2 floors) | **MENTIONED** | 06 | Mentioned in warp network |
| Glast Heim (10+ sub-maps) | **MISSING** | -- | Massive dungeon complex, not detailed |
| Clock Tower (Al De Baran, 4+ floors) | **MENTIONED** | 06 | Town listed but dungeon not detailed |
| Sunken Ship (2 floors) | **MISSING** | -- | Drake MVP location |
| Ant Hell (2 floors) | **MISSING** | -- | Near Morroc |
| Coal Mine/Mine Dungeon | **MISSING** | -- | Near Einbroch |
| Turtle Island (3 floors) | **MISSING** | -- | Ship from Alberta |
| Magma Dungeon (2 floors) | **MISSING** | -- | Near Yuno |
| Abyss Lake (3 floors) | **MISSING** | -- | Dragon-themed |
| Ice Dungeon (3 floors + Rachel) | **MISSING** | -- | Near Rachel |
| Thor Volcano (3 floors) | **MISSING** | -- | Near Veins |
| Biolab (3 floors) | **MISSING** | -- | Near Lighthalzen, end-game |
| Nameless Island (3 floors) | **MISSING** | -- | From Veins |
| Toy Factory (2 floors) | **MISSING** | -- | Lutie/Xmas |
| Niffleheim dungeon | **MISSING** | -- | In Niflheim |
| Treasure rooms (various) | **MISSING** | -- | WoE castle treasures |

### 6.4 Map Properties and Flags

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| noteleport (blocks Fly Wing/Teleport) | **FULL** | 06 | Zone flag system |
| noreturn (blocks Butterfly Wing) | **FULL** | 06 | Zone flag |
| nosave (overrides save point) | **FULL** | 06 | Zone flag |
| pvp (enables player killing) | **FULL** | 06, 08 | Zone flag |
| gvg (War of Emperium zone) | **FULL** | 06, 08 | Zone flag |
| nomemo (blocks Warp Portal memo) | **PARTIAL** | 06 | Zone flag mentioned |
| nowarp (blocks Warp Portal entry) | **PARTIAL** | 06 | Zone flag mentioned |
| nowarpto (blocks being warped to) | **PARTIAL** | 06 | Zone flag mentioned |
| noicewall | **MISSING** | -- | Blocks Ice Wall placement |
| Cell walkability (GAT files) | **FULL** | 15 | Walkable/non-walkable/water |

### 6.5 Navigation

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Warp portals (NPC-placed, bidirectional) | **FULL** | 06 | WarpPortal actors |
| Kafra teleport service (town-to-town) | **FULL** | 06, 07 | Zeny costs documented |
| Priest Warp Portal skill | **FULL** | 03 | Memo system |
| Minimap | **PARTIAL** | 09 | Layout described; not implemented |
| World map (Ctrl+~) | **PARTIAL** | 09 | Described; not implemented |
| Navigation system (NPC pathing) | **PARTIAL** | 09 | Post-renewal addition noted |
| Inter-continental airships | **FULL** | 06 | Domestic + international routes |
| Ship travel to islands | **FULL** | 06 | Alberta departure |

---

## 7. Economy

### 7.1 Currency

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Zeny (sole currency) | **FULL** | 13 | Max 2^31-1, overflow protection |
| Zeny sources (NPC sales, drops, vending) | **FULL** | 13 | Full table |
| Zeny sinks (shop, Kafra, skills, refine) | **FULL** | 13 | Full table with cost ranges |

### 7.2 NPC Shops

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Buy/sell price formula | **FULL** | 13 | Sell = floor(Buy/2) |
| Discount skill (Merchant, up to 24%) | **FULL** | 13 | 10-level table |
| Overcharge skill (Merchant, up to 24%) | **FULL** | 13 | 10-level table |
| Tool Dealers (per-town inventory) | **FULL** | 07, 13 | Prontera as reference |
| Weapon Dealers (per-town inventory) | **FULL** | 07, 13 | Class-themed by town |
| Armor Dealers (per-town inventory) | **FULL** | 07, 13 | Per-town |
| Pet NPCs (Incubator, food) | **PARTIAL** | 12 | Pet food listed |
| Arrow Dealers | **PARTIAL** | 07 | Mentioned |

### 7.3 Player-to-Player Trading

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Trade window (items + zeny) | **FULL** | 13, Master Plan | Lock/confirm flow |
| Anti-scam (re-lock on change) | **FULL** | 13 | Documented |
| Max zeny per trade (999,999,999) | **FULL** | 13 | Overflow protection |

### 7.4 Vending System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Merchant Vending skill (player shops) | **FULL** | 13, Master Plan | Max items = Vending Lv + 2 |
| Vendor sign above character | **FULL** | 13 | Shop name display |
| Browse/buy from vendor | **FULL** | 13 | Socket events documented |
| Vending tax (5% over 10M zeny items) | **FULL** | 13 | Zeny sink |
| Overweight restriction while vending | **PARTIAL** | 13 | Cannot move while vending |

### 7.5 Buying Store

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Reverse vending (post wanted items) | **PARTIAL** | 13, Master Plan | Listed as feature |
| Bulk Buyer Shop License (200z) | **PARTIAL** | 13 | Cost documented |

### 7.6 Storage System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Kafra storage (600 slots per account) | **FULL** | 13, Master Plan | DB schema provided |
| Guild storage (100-500 slots) | **FULL** | 08, 13 | Permission-based |
| Kafra access fee (40z) | **FULL** | 13 | Per-access cost |
| Store/retrieve items and zeny | **FULL** | 13, Master Plan | Drag interface |

### 7.7 Mail System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| RODEX mail system | **PARTIAL** | 13 | Mentioned; fees documented |
| Item attachments (2,500z per item) | **PARTIAL** | 13 | Fee noted |
| Zeny sending (2% fee) | **PARTIAL** | 13 | Fee noted |
| Auction Hall integration | **PARTIAL** | 13 | Mentioned; noted as later removed |

### 7.8 Auction System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Item auctioning via NPC | **PARTIAL** | 13 | Mentioned; 12,000z/hr listing fee |
| Category browsing (Armor/Weapon/Card/Misc) | **MISSING** | -- | UI not detailed |
| System removed due to bugs | **PARTIAL** | 13 | Historical note |

---

## 8. Monster/Enemy Systems

### 8.1 Monster AI

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| AI type codes (01-27) | **FULL** | 04 | All codes with hex bitmasks |
| 18+ mode flag bitmask | **FULL** | 04 | Each flag documented |
| State machine (IDLE/CHASE/ATTACK/DEAD) | **FULL** | 04 | Per-state behavior |
| Aggro mechanics (setEnemyAggro) | **FULL** | 04 | 5-step process |
| Assist trigger (same-type nearby) | **FULL** | 04 | Distance, conditions |
| Target selection rules (8 situations) | **FULL** | 04 | Full table |
| Chase behavior (speed, range, leash) | **FULL** | 04 | Formulas with examples |
| Wander behavior (IDLE random walk) | **FULL** | 04 | 60% speed, pause timers |
| Cast sensor (detect casters) | **FULL** | 04 | Idle and chase variants |
| Boss protocol (knockback/status immune, detector) | **FULL** | 04 | Auto-applied flags |
| Random target selection | **FULL** | 04 | From inCombatWith set |
| Hit stun (damageMotion pause) | **FULL** | 04 | Per-monster delay |
| Looter behavior (pick up items) | **FULL** | 04 | Idle state only |

### 8.2 Monster Database

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 509 monster templates | **FULL** | 04, Master Plan | From rAthena pre-re |
| Top 100 monsters (level 1-99) | **FULL** | 04 | Stats, drops, zones |
| Monster elements (10 elements x 4 levels) | **FULL** | 04 | Per-template |
| Monster races (20 types) | **FULL** | 04 | Per-template |
| Monster sizes (Small/Medium/Large) | **FULL** | 04 | Per-template |
| Monster stats (HP, ATK, DEF, MDEF, etc.) | **FULL** | 04 | Per-template |

### 8.3 MVP/Boss System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| MVP spawn timers (1-2 hour respawn) | **FULL** | 04, Master Plan | Variable timing |
| MVP announcement (zone/server) | **FULL** | 04 | Broadcast on spawn |
| MVP tombstone (death location marker) | **FULL** | 04 | Killer name + time |
| MVP drops (separate from normal) | **FULL** | 04 | Killer-only reward |
| MVP EXP distribution | **PARTIAL** | 04 | Most damage = MVP credit |
| Phase mechanics (HP threshold skills) | **PARTIAL** | 04 | Mentioned; per-boss data sparse |
| 10 initial MVPs listed | **FULL** | Master Plan | Level, zone, mechanics |
| Mini-bosses | **PARTIAL** | Master Plan | 5 listed |

### 8.4 Monster Skills

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Monster skill database | **FULL** | 04 | ro_monster_skills.js documented |
| Skill conditions (HP%, target count, random) | **FULL** | 04 | Condition types |
| NPC_ skill types (summonslave, metamorphosis, etc.) | **FULL** | CLAUDE.md | 40+ NPC_ skills |
| Monster skill execution functions | **FULL** | CLAUDE.md | 7 execution functions |
| Slave spawning (master/slave lifecycle) | **FULL** | CLAUDE.md | Die with master, leash |

### 8.5 Drop System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Chance-based loot rolling | **FULL** | 04 | Per-monster drop tables |
| Guaranteed drops (100% rate) | **FULL** | 04 | Poring -> Jellopy |
| Card drops (0.01% base rate) | **FULL** | 04 | LUK modifier |
| Drop rate server multiplier (2x/3x events) | **PARTIAL** | 04 | Mentioned |
| Steal interaction with drop tables | **PARTIAL** | 04 | Modified rates for Steal |
| Looter recovery (kill looter to get items) | **MISSING** | -- | Items held by looter drop on death |

### 8.6 Spawn System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Spawn points per zone | **FULL** | 04 | Count, coordinates |
| Respawn timers (per-monster) | **FULL** | 04 | From template |
| Spawn area (radius around point) | **PARTIAL** | 04 | Mentioned |
| Dead Branch spawning (item use) | **MISSING** | -- | Summons random monster |
| Bloody Branch spawning (MVP version) | **MISSING** | -- | Summons random MVP |

---

## 9. Status Effect Systems

### 9.1 Negative Status Effects

| Status | Coverage | Doc | Notes |
|--------|----------|-----|-------|
| Poison (DoT, -HP regen) | **FULL** | 02 | Duration, tick, visual |
| Stun (cannot act, 2-5s) | **FULL** | 02 | VIT resistance |
| Freeze (cannot act, Water 1 property) | **FULL** | 02 | Frost Diver documented |
| Stone Curse/Petrify (2-phase, Earth 1) | **FULL** | 02 | Stone Curse documented |
| Sleep (cannot act, wake on damage) | **FULL** | 02 | Duration, visual |
| Curse (LUK=0, -25% move speed) | **FULL** | 02 | 30s duration |
| Silence (cannot use skills) | **FULL** | 02 | 30s duration |
| Blind (-25 HIT, -25% FLEE) | **FULL** | 02 | Reduced vision |
| Bleeding (DoT, no natural regen) | **FULL** | 02 | 30s duration |
| Coma (HP=1, SP=1 instant) | **FULL** | 02 | Instant effect |
| Confusion (reversed controls) | **MISSING** | -- | Movement direction swapped |
| Chaos (item in inventory randomly dropped) | **MISSING** | -- | From Strip skills |
| Hallucination (screen distortion) | **MISSING** | -- | From some monster skills |
| Fear (cannot act, similar to stun) | **MISSING** | -- | Late pre-renewal addition |
| Burning (fire DoT, movement speed reduction) | **MISSING** | -- | From fire skills |
| Crystallization/Deep Freeze | **MISSING** | -- | Enhanced freeze variant |

### 9.2 Positive Status Effects (Buffs)

| Buff | Coverage | Doc | Notes |
|------|----------|-----|-------|
| Provoke (+ATK%, -DEF%) | **FULL** | 02, 03 | Bidirectional |
| Blessing (+STR/DEX/INT) | **FULL** | 03 | Or curse Undead/Demon |
| Increase AGI (+AGI, +move speed) | **FULL** | 03 | Cancels Decrease AGI |
| Angelus (+VIT DEF) | **FULL** | 03 | Party buff |
| Kyrie Eleison (damage barrier) | **FULL** | 03 | Absorbs X hits/damage |
| Magnificat (SP regen x2) | **FULL** | 03 | Party range |
| Gloria (+LUK 30) | **FULL** | 03 | Party range |
| Endure (flinch immunity) | **FULL** | 03 | 7 hits or duration |
| Two-Hand Quicken (+ASPD) | **FULL** | 03 | 2H sword only |
| Adrenaline Rush (+ASPD) | **FULL** | 03 | Axe/mace |
| Weapon Perfection (no size penalty) | **FULL** | 03 | 15-60s |
| Maximize Power (max weapon ATK) | **FULL** | 03 | SP drain |
| All performance buffs (songs/dances) | **FULL** | 03 | 7 supportive + 8 ensemble |
| Autoguard (block chance) | **FULL** | 03 | Shield required |
| Defender (ranged damage reduction) | **FULL** | 03 | Shield required |
| Devotion (damage redirect) | **FULL** | 03 | Party member protection |
| EDP (Enchant Deadly Poison) | **PARTIAL** | 03 | Trans-class buff |
| Assumptio (+DEF/MDEF) | **PARTIAL** | 03 | Trans-class buff |
| Gospel/Battle Chant (random buffs) | **PARTIAL** | 03 | Trans-class |

### 9.3 Status Resistance

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| VIT-based stun resistance | **PARTIAL** | 02 | Mentioned |
| INT-based sleep/blind resistance | **PARTIAL** | 02 | Mentioned |
| LUK-based curse/stone resistance | **PARTIAL** | 02 | Mentioned |
| Boss immunity to all CC | **FULL** | 04 | MD_STATUSIMMUNE flag |
| Status resistance formulas per effect | **MISSING** | -- | Per-status VIT/INT/LUK thresholds not compiled |

---

## 10. UI/UX Systems

### 10.1 Main HUD

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Basic Info window (Alt+V) | **FULL** | 09 | HP/SP/EXP bars, weight, zeny |
| Status window (Alt+A) | **FULL** | 09 | 6 stats + derived stats |
| Equipment window (Alt+Q) | **FULL** | 09 | 10 slots with drag-drop |
| Inventory window (Alt+E) | **FULL** | 09 | Tabs: Item/Equip/Etc |
| Skill tree window (Alt+S) | **FULL** | 09 | Prerequisites, + buttons |
| Hotbar/shortcut bar (F9) | **FULL** | 09 | Items + skills on number keys |
| Chat window | **FULL** | 09 | Bottom-left, resizable |
| Minimap (Ctrl+Tab) | **PARTIAL** | 09 | Described but not implemented |
| World map (Ctrl+~) | **PARTIAL** | 09 | Described but not implemented |
| Damage numbers (floating) | **FULL** | 09 | Miss, crit, heal, skill damage |
| Cast bar (progress bar during casting) | **FULL** | 09 | World-projected |
| HP/SP bars above characters | **FULL** | 09 | Party members + enemies |
| Name tags above characters | **FULL** | 15 | Guild name, title |
| Death overlay (respawn prompt) | **PARTIAL** | 09 | Mentioned |
| Loading screen (zone transition) | **FULL** | 09 | Full overlay |

### 10.2 Windows and Panels

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Party window (Alt+Z) | **FULL** | 08, 09 | Member list, HP bars |
| Guild window | **PARTIAL** | 08 | Features described; UI layout not detailed |
| Quest log | **PARTIAL** | 07, 09 | System described; UI sparse |
| Shop window (buy/sell NPC) | **FULL** | 07 | Batch operations |
| Trade window | **PARTIAL** | 13 | Lock/confirm flow |
| Kafra window (save/storage/teleport) | **FULL** | 07, 09 | Tab-based |
| Refine window | **PARTIAL** | 07 | NPC interaction |
| Vending setup window | **PARTIAL** | 13 | Merchant skill activation |
| Vending browse window | **PARTIAL** | 13 | Buyer view |
| Pet window | **PARTIAL** | 12 | Pet commands/stats |
| Homunculus window | **PARTIAL** | 12 | Commands/stats |
| Friends list window | **MISSING** | -- | UI for friend management |
| Configuration/options window | **MISSING** | -- | Graphics, sound, controls |
| Macro window (Alt+M) | **MISSING** | -- | Emote/command shortcuts |
| Tip box / helper window | **MISSING** | -- | First-login guidance |

### 10.3 Item Tooltips and Popups

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Item tooltip (hover) | **FULL** | 09 | Name, type, weight, description, stats |
| Equipment comparison tooltip | **PARTIAL** | 09 | Mentioned |
| Item rarity colors | **PARTIAL** | 09 | White/green/blue/purple/orange |
| Right-click context menus | **PARTIAL** | 09 | Use/equip/drop options |
| Confirmation dialogs (drop, delete, etc.) | **PARTIAL** | 09 | Mentioned |

### 10.4 Targeting

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Click-to-target (enemy/NPC/player) | **FULL** | 15 | Cursor change system |
| Target info display (name, HP bar) | **FULL** | 15 | Above selected target |
| Auto-attack on target click | **FULL** | 15 | Click + hold |
| Cursor types (normal/attack/talk/pickup) | **FULL** | 15 | 4 cursor states |
| Tab targeting | **MISSING** | -- | Some implementations have tab-cycle |
| Ctrl+click (force attack on players) | **MISSING** | -- | PvP attack without PvP zone |

---

## 11. Audio/Visual Systems

### 11.1 Audio

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| BGM per zone/town/dungeon | **FULL** | 14 | Per-zone assignment |
| BGM fade on zone transition | **PARTIAL** | 14 | Mentioned |
| SFX per skill/action | **PARTIAL** | 14 | Categories listed |
| Attack hit sounds | **PARTIAL** | 14 | Per-weapon type |
| Monster death sounds | **PARTIAL** | 14 | Per-monster |
| UI sounds (button click, equip, level up) | **PARTIAL** | 14 | Categories listed |
| Ambient sounds per zone type | **PARTIAL** | 14 | Town/field/dungeon |
| Volume controls (BGM/SFX separate) | **PARTIAL** | 14 | Settings mentioned |

### 11.2 Visual/Art

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Character models (per class, per gender) | **PARTIAL** | 10 | Art pipeline outlined |
| Monster models/sprites | **PARTIAL** | 10 | Pipeline outlined |
| Skill VFX (particles, projectiles) | **FULL** | Master Plan | 97+ configs in SkillVFXSubsystem |
| Hair style system (1-19 styles) | **PARTIAL** | 10, Master Plan | Customization documented |
| Hair color system (0-8 colors) | **PARTIAL** | 10, Master Plan | Customization documented |
| Equipment visual changes | **PARTIAL** | Master Plan | Art dependency |
| Headgear visuals | **PARTIAL** | 10 | Mentioned |
| Refine glow effects (+7/+8/+9/+10) | **MENTIONED** | Master Plan | Art requirement |
| Status effect visuals (stun stars, freeze ice) | **PARTIAL** | 02 | Visual column in status table |
| Day/night cycle | **MISSING** | -- | Was on kRO briefly; some servers implement |
| Weather effects (rain, snow) | **MISSING** | -- | Seasonal/map-specific |
| Cloth dye/palette system | **PARTIAL** | 07 | Stylist NPC mentioned |

---

## 12. Miscellaneous Systems

### 12.1 Pet System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Pet taming (taming items + success rate) | **FULL** | 12 | Formula with HP factor |
| Pet egg system (inventory storage) | **FULL** | 12 | Unique instances |
| Hunger system (0-100) | **FULL** | 12 | Feeding, decay, starvation |
| Intimacy system (0-1000) | **FULL** | 12 | 5 tiers with behaviors |
| Pet stat bonuses (Cordial/Loyal) | **FULL** | 12 | Per-pet bonus table |
| Pet accessories (visual + bonus) | **FULL** | 12 | Per-pet accessory |
| Pet following AI | **FULL** | 12 | Follow distance, teleport |
| Pet commands (feed, perform, return) | **FULL** | 12 | 5 commands |
| 34 core pre-renewal pets | **FULL** | 12 | Full table |
| Pet evolution | **N/A** | 12 | Renewal feature, documented for future |
| Overfeed penalty (pet runs away) | **FULL** | 12 | 3 overfeed = escape |

### 12.2 Homunculus System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| 4 homunculus types (Lif/Amistr/Filir/Vanilmirth) | **FULL** | 12 | Stats, growth |
| Homunculus creation (Embryo item) | **FULL** | 12 | Alchemist skill |
| Homunculus feeding (type-specific food) | **FULL** | 12 | Hunger/intimacy |
| Homunculus skills (4 per type) | **PARTIAL** | 12 | Skills listed; not all detailed |
| Homunculus evolution (Loyal intimacy) | **PARTIAL** | 12 | Requirements listed; post-evolution sparse |
| Homunculus auto-attack | **PARTIAL** | 12 | Combat tick documented in CLAUDE.md |
| Homunculus EXP sharing | **PARTIAL** | 12 | 10% to homunculus |

### 12.3 Falcon System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Falcon rental (Hunter class) | **FULL** | 12 | From Hunter Guild NPC |
| Blitz Beat (auto/manual) | **FULL** | 03, 12 | Damage formula |
| Auto-Blitz chance | **FULL** | 03 | floor((jobLv+9)/10) |
| Detect (reveal hidden) | **FULL** | 03 | With Falcon |
| Falcon Assault (Sniper) | **PARTIAL** | 03 | Trans-class skill |
| Falcon exclusivity with Peco | **PARTIAL** | 12 | Cannot have both |

### 12.4 Cart System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Cart rental (Kafra, 600-1200z) | **FULL** | 12 | Merchant class only |
| Cart inventory (100 slots) | **FULL** | 12 | Separate from inventory |
| Cart skills (Cart Revolution, Cart Termination) | **FULL** | 03 | Damage formulas |
| Change Cart (visual change skill) | **FULL** | 03 | 5 cart appearances |
| Cart speed penalty (Pushcart skill reduces) | **FULL** | 12, 15 | Lv1 -50% to Lv5 -5% |
| Cart weight limit | **PARTIAL** | 12 | Weight = player max weight * 0.8 |

### 12.5 Mount System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Peco Peco (Knight/Crusader) | **FULL** | 12 | +36% speed, Riding skill |
| Dragon (Rune Knight, Renewal) | **N/A** | -- | Renewal only |
| Grand Peco (Paladin) | **PARTIAL** | 12 | Trans-class mount |
| Mount + Falcon exclusivity | **PARTIAL** | 12 | Cannot have both |

### 12.6 Mercenary System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Hired mercenaries (NPC rental) | **PARTIAL** | 12 | System described |
| Mercenary types (Archer/Lancer/Swordsman) | **PARTIAL** | 12 | 3 types, 10 grades each |
| Mercenary duration (30 minutes) | **PARTIAL** | 12 | Time-limited |
| Mercenary loyalty system | **PARTIAL** | 12 | Grade requirements |
| Mercenary skills | **MISSING** | -- | Per-type skills not listed |
| Mercenary commands (attack/follow/standby) | **PARTIAL** | 12 | Basic commands |

### 12.7 PvP System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| PvP maps (zone flags) | **FULL** | 08 | pvp zone type |
| PvP damage rules (reduced damage) | **FULL** | 08 | 60-70% of PvE |
| No EXP loss on PvP death | **FULL** | 08 | Respawn at save point |
| PvP rankings (kills/deaths) | **PARTIAL** | 08 | Tracking mentioned |
| PvP arena (structured) | **PARTIAL** | 08 | Mentioned |
| Free-for-all PvP mode | **FULL** | 08 | Open world flagged zones |
| PvP room types (Yoyo, Nightmare) | **PARTIAL** | 08 | Modes listed |

### 12.8 War of Emperium (WoE)

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Castle zones (5 per realm) | **FULL** | 08 | WoE 1 detailed |
| Emperium (destructible crystal) | **FULL** | 08 | HP, rules, element |
| Schedule system (set days/times) | **FULL** | 08 | Configurable |
| Castle ownership | **FULL** | 08 | Transfers on Emperium break |
| Treasure boxes (daily for owner) | **FULL** | 08 | Economy reward |
| Guardian NPCs (Knight/Soldier) | **FULL** | 08 | Castle defense |
| WoE rules (no Teleport, restricted items) | **FULL** | 08 | Full rule set |
| Guardian stones | **PARTIAL** | 08 | Mentioned |
| Barricades | **PARTIAL** | 08 | Mentioned |
| WoE SE (Second Edition) | **PARTIAL** | 08 | Section header; less detail |
| God items/divine equipment (Mjolnir, etc.) | **PARTIAL** | 08 | Section mentioned |

### 12.9 Battlegrounds

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Team-based PvP instances | **PARTIAL** | 08 | Section mentioned |
| Tierra Valley / Flavius modes | **MISSING** | -- | Specific modes not detailed |
| Battleground rewards (badges, equipment) | **MISSING** | -- | Reward system not documented |
| Queue system | **MISSING** | -- | Matchmaking not documented |

### 12.10 Quest System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Quest database (objectives, rewards) | **FULL** | 07 | JSON schema defined |
| Quest states (not started/in progress/complete) | **FULL** | 07 | State machine |
| Objective types (kill/collect/talk/reach) | **FULL** | 07 | 5 types |
| Quest rewards (EXP, zeny, items, skills) | **FULL** | 07 | Multiple reward types |
| Quest log UI | **PARTIAL** | 07, 09 | Described; not implemented |
| Quest markers (! and ? above NPCs) | **PARTIAL** | 07 | Mentioned |
| Job change quests (6 first-class) | **PARTIAL** | 07 | Locations listed; details sparse |
| Access quests (dungeon unlock) | **PARTIAL** | 07 | 5 access quests listed |
| Daily quests | **MISSING** | -- | Some daily repeatables existed |
| Turn-in quests (collection) | **MISSING** | -- | Collect X items type |

### 12.11 NPC Dialogue System

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| Dialogue trees (branching choices) | **FULL** | 07 | JSON format defined |
| NPC sprites and facing | **FULL** | 07 | Fixed position |
| Conditional branching (level, class, items, quest) | **FULL** | 07 | Per-node conditions |
| NPC actions (give/take items, warp, etc.) | **FULL** | 07 | Action types |
| Stylist NPC (hair change) | **PARTIAL** | 07 | Mentioned |
| Refiner NPC | **FULL** | 07 | Full mechanics |

### 12.12 Miscellaneous Forgotten Features

| Feature | Coverage | Doc | Notes |
|---------|----------|-----|-------|
| /commands system (slash commands) | **PARTIAL** | 15 | Some listed |
| /where (show current coordinates) | **MISSING** | -- | Debug/utility command |
| /who (player count) | **MISSING** | -- | Server population command |
| /memo (save warp portal location) | **PARTIAL** | 03 | Priest skill interaction |
| /noctrl (auto-attack without holding click) | **MISSING** | -- | Control mode toggle |
| /noshift (remove Shift requirement for skills on self) | **MISSING** | -- | QoL command |
| /showname (toggle name display) | **MISSING** | -- | Display setting |
| /effect (toggle skill effects) | **MISSING** | -- | Performance command |
| /mineffect (reduce effect intensity) | **MISSING** | -- | Performance command |
| /bgm and /sound (toggle audio) | **MISSING** | -- | Audio control commands |
| /snap (auto-target nearest enemy) | **MISSING** | -- | Targeting QoL |
| Screenshot system (PrintScreen) | **MISSING** | -- | Client feature |
| Character rename (NPC or cash shop) | **MISSING** | -- | Premium service |
| GM commands (/hide, /item, /warp, /kick, etc.) | **MISSING** | -- | Server administration |
| Anti-bot measures (Captcha) | **MISSING** | -- | Security system |
| Client-side settings persistence | **MISSING** | -- | Config files |
| Dead Branch / Bloody Branch monster summoning | **MISSING** | -- | Item-based monster spawn |
| Treasure chest system (WoE castles) | **PARTIAL** | 08 | Mentioned in WoE section |

---

## Coverage Matrix

### By Document

| Document | Systems Covered | Coverage Level |
|----------|----------------|----------------|
| `01_Stats_Leveling_JobSystem.md` | Stats, leveling, class progression | **FULL** |
| `02_Combat_System.md` | Physical/magical damage, DEF, HIT/FLEE, ASPD, crits, status effects | **FULL** |
| `03_Skills_Complete.md` | All 1st/2nd class skills, trans class skill lists | **FULL** (1st/2nd), **PARTIAL** (trans) |
| `04_Monsters_EnemyAI.md` | AI system, 100 monsters, MVPs, drops, spawn, monster skills | **FULL** |
| `05_Items_Equipment_Cards.md` | Item categories, equipment, weapons, refine, cards, consumables, weight | **FULL** |
| `06_World_Maps_Zones.md` | Continental overview, 22 towns, field maps, dungeons, warp, flags | **PARTIAL** (many dungeons missing) |
| `07_NPCs_Quests_Shops.md` | NPC types, shops, dialogue, quests, Kafra, refine NPCs | **FULL** |
| `08_PvP_Guild_WoE.md` | Party, guild, PvP, WoE 1, WoE SE, Battlegrounds, God items | **FULL** (Party/Guild/WoE1), **PARTIAL** (rest) |
| `09_UI_UX_System.md` | HUD, windows, tooltips, targeting, damage numbers | **FULL** |
| `10_Art_Animation_VFX_Pipeline.md` | Art pipeline, models, VFX | **PARTIAL** |
| `11_Multiplayer_Networking.md` | Networking architecture | **FULL** |
| `12_Pets_Homunculus_Companions.md` | Pets, homunculus, mercenary, falcon, cart, mount | **FULL** (pets), **PARTIAL** (rest) |
| `13_Economy_Trading_Vending.md` | Zeny, NPC shops, trading, vending, buying store, storage, mail, auction | **FULL** |
| `14_Audio_Music_SFX.md` | BGM, SFX, audio pipeline | **PARTIAL** |
| `15_Movement_Targeting_Interaction.md` | Movement, multiplayer sync, targeting, pathfinding, name tags | **FULL** |

### By System Category

| Category | Total Features | FULL | PARTIAL | MENTIONED | MISSING |
|----------|---------------|------|---------|-----------|---------|
| Core Mechanics | 28 | 21 | 4 | 0 | 3 |
| Combat Systems | 62 | 52 | 8 | 0 | 2 |
| Class/Skill Systems | 32 | 19 | 7 | 1 | 5 |
| Item Systems | 48 | 35 | 10 | 1 | 2 |
| Social Systems | 45 | 18 | 11 | 0 | 16 |
| World/Navigation | 55 | 12 | 25 | 2 | 16 |
| Economy | 24 | 18 | 5 | 0 | 1 |
| Monster/Enemy | 36 | 30 | 4 | 0 | 2 |
| Status Effects | 26 | 14 | 4 | 0 | 8 |
| UI/UX | 38 | 22 | 12 | 0 | 4 |
| Audio/Visual | 16 | 2 | 10 | 1 | 3 |
| Miscellaneous | 95 | 42 | 30 | 0 | 23 |
| **TOTAL** | **505** | **285** | **130** | **5** | **85** |

**Overall Coverage: 285/505 = 56.4% FULL, 130/505 = 25.7% PARTIAL, 85/505 = 16.8% MISSING**

---

## Missing/Overlooked Features

The following features are completely absent from existing RagnaCloneDocs and need research documents:

### HIGH Priority (Core gameplay, affects many players)

1. **Marriage System** -- Ceremony, rings, couple skills, divorce, Jawaii access
2. **Adoption System** -- Baby classes, parent requirements, stat halving
3. **Super Novice Class** -- All 1st-class skills, unique death penalty, guardian angel at 99.0% EXP
4. **Emote/Emotion System** -- 71+ emotes, Alt+L menu, macro window, /commands
5. **Chat Rooms** -- Alt+C player-created chat bubbles, used for trading/socializing
6. **Duel System** -- 1v1 PvP challenges outside PvP zones
7. **Status Resistance Formulas** -- Per-status VIT/INT/LUK thresholds for immunity
8. **Death EXP Penalty** -- Per-class penalty rates (1% Novice, varies by class)
9. **Dead Branch / Bloody Branch** -- Item-based monster summoning (major gameplay feature)
10. **Slash Commands** -- /noctrl, /noshift, /where, /who, /effect, /mineffect, /snap, etc.

### MEDIUM Priority (Significant content, fewer players affected)

11. **Extended Classes: Taekwon Kid** -- Kick-based class, stances, running system
12. **Extended Classes: Star Gladiator** -- Feeling/Hatred/Warmth/Comfort systems
13. **Extended Classes: Soul Linker** -- Spirit Link buffs for every class, Ka-/Es- spells
14. **Extended Classes: Gunslinger** -- 5 gun types, Coin system, Desperado, Full Buster
15. **Extended Classes: Ninja** -- Ninjutsu (fire/water/wind/earth), Kunai, Shadow skills
16. **Battlegrounds** -- Tierra Valley, Flavius, badge rewards, queue system
17. **Equipment Set Bonuses** -- Certain equipment combinations grant bonus effects
18. **Glast Heim dungeon complex** -- 10+ sub-maps, major mid-to-high content
19. **Additional dungeon details** -- Sunken Ship, Ant Hell, Coal Mine, Magma, Abyss, Ice, Thor, Biolab, Toy Factory
20. **Confusion/Chaos/Hallucination status effects** -- Additional negative statuses

### LOW Priority (Niche features, polish items)

21. **Day/Night Cycle** -- Was briefly on kRO; optional aesthetic feature
22. **Weather Effects** -- Rain, snow, map-specific visuals
23. **Mercenary Skills** -- Per-type skill lists for hired mercenaries
24. **Battleground Rewards** -- Badge exchange items and equipment
25. **Options/Configuration Window** -- Graphics, sound, control settings UI
26. **GM Commands** -- Server administration tools
27. **Anti-bot/Captcha System** -- Security measures
28. **Looter Item Recovery** -- Items held by looter monsters drop on death
29. **Named/Unique Weapons** -- Quest/MVP weapons with special effects
30. **Daily Quests** -- Repeatable quest content

---

## Implementation Priority Order

Based on gameplay impact, player engagement, and dependency chains:

### Tier 1: Foundation (Must Have for Playable Game)
1. Stats, Leveling, EXP System -- **DONE**
2. Combat (Physical + Magical + Defense) -- **DONE**
3. First Class Skills (6 classes) -- **DONE**
4. Inventory, Equipment, Weight -- **DONE**
5. Monster AI + Spawning -- **DONE**
6. Zone System + Warp Portals -- **DONE**
7. NPC Shops (buy/sell) -- **DONE**
8. Chat System (basic channels) -- **DONE**
9. Hotbar System -- **DONE**
10. Job Change System -- **DONE**

### Tier 2: Core Content (Must Have for MMO Feel)
11. Second Class Skills (12 classes) -- **DONE**
12. Status Effects (all 10 core) -- **DONE**
13. Card System (compound + effects) -- **DONE**
14. Refine/Upgrade System -- **DONE**
15. Party System -- **DONE**
16. Death Penalty + Respawn -- **DONE**
17. Pet System -- **DONE** (basic)
18. Homunculus System -- **DONE** (basic)
19. Cart/Vending System -- **DONE**
20. Weight Enforcement -- **DONE**

### Tier 3: Extended Content (Needed for Full Experience)
21. Guild System (create, ranks, skills, storage)
22. Whisper/PM System + Block List -- **DONE** (basic)
23. Player-to-Player Trading
24. PvP System (zones, rules, rankings)
25. Quest System (dialogue engine, quest log, job change quests)
26. Kafra Storage
27. More Zones (15+ towns, 30+ fields, 20+ dungeons)
28. More Monsters (100+ spawn points, MVP bosses)
29. Emote/Emotion System
30. Minimap + World Map

### Tier 4: Completeness (Full RO Classic Replica)
31. War of Emperium (castles, Emperium, schedule, guilds)
32. Transcendent Classes (13 trans classes, rebirth system)
33. Marriage + Adoption System
34. Super Novice Class
35. Extended Classes (Taekwon/Star Gladiator/Soul Linker/Gunslinger/Ninja)
36. Battlegrounds
37. Duel System
38. Chat Rooms
39. Friend List System
40. Mail/Auction System

### Tier 5: Polish (Nice to Have)
41. Dead Branch / Bloody Branch summoning
42. Equipment Set Bonuses
43. Day/Night Cycle
44. Weather Effects
45. Slash Command System (/noctrl, /noshift, etc.)
46. Options/Configuration Window
47. Screenshot System
48. Character Rename Service
49. GM Administration Tools
50. Anti-bot Measures

---

## Complete Feature Count

### Summary by Category

| Category | Discrete Features |
|----------|-------------------|
| Core Mechanics (stats, leveling, classes, movement) | 28 |
| Combat Systems (physical, magical, defense, HIT/FLEE, ASPD, crit, targeting) | 62 |
| Class/Skill Systems (27 classes, 400+ skills, special mechanics) | 32 major systems |
| Item Systems (categories, equipment, weapons, refine, cards, consumables, weight) | 48 |
| Social Systems (party, guild, chat, friends, marriage, adoption, emotes, duel) | 45 |
| World/Navigation (towns, fields, dungeons, warps, maps, flags) | 55 |
| Economy (zeny, shops, trading, vending, storage, mail, auction) | 24 |
| Monster/Enemy Systems (AI, database, MVPs, skills, drops, spawns) | 36 |
| Status Effect Systems (10+ negative, 20+ positive, resistance) | 26 |
| UI/UX Systems (HUD, windows, tooltips, targeting) | 38 |
| Audio/Visual Systems (BGM, SFX, art, VFX) | 16 |
| Miscellaneous (pets, homunculus, falcon, cart, mount, PvP, WoE, quests, NPCs) | 95 |
| **GRAND TOTAL** | **505 discrete features** |

### Skill Count by Class

| Class Tier | Classes | Approx. Skills |
|-----------|---------|---------------|
| Novice | 1 | 3 |
| First Classes | 6 | ~56 (+ ~15 quest skills) |
| Second Classes (2-1) | 6 | ~95 |
| Second Classes (2-2) | 7 | ~115 (including ensemble) |
| Transcendent | 13 | ~65 new trans-only skills |
| Extended (Taekwon/SG/SL) | 3 | ~60 |
| Extended (Gunslinger) | 1 | ~25 |
| Extended (Ninja) | 1 | ~20 |
| Super Novice | 1 | Uses 1st class skills (~56) |
| Guild Skills | -- | 12 |
| Marriage Skills | -- | 2 |
| **TOTAL** | **39 playable classes** | **~470 unique skills** |

### Monster Count

| Category | Count |
|----------|-------|
| Normal Monsters | ~450 |
| Mini-Bosses | ~30 |
| MVP Bosses | ~30 |
| Plants/Objects | ~15 |
| WoE Guardians | ~10 |
| Pets (tameable) | ~34 |
| **TOTAL** | **~570 monster templates** |

### Item Count (Estimated Pre-Renewal Total)

| Category | Count |
|----------|-------|
| Weapons | ~400 |
| Armor/Shield/Garment/Footgear | ~300 |
| Headgear (top/mid/low) | ~400 |
| Accessories | ~150 |
| Cards | ~500 |
| Consumables (potions, food, scrolls) | ~200 |
| Ammunition (arrows, bullets) | ~50 |
| Miscellaneous/Etc (loot, ores, materials) | ~500 |
| Pet Items (eggs, food, accessories) | ~100 |
| Quest Items | ~200 |
| **TOTAL** | **~2,800 unique items** |

### Zone Count (Estimated Pre-Renewal)

| Category | Count |
|----------|-------|
| Towns | ~22 |
| Field Maps | ~120 |
| Dungeon Floors | ~80 |
| PvP/WoE Maps | ~30 |
| Special/Event Maps | ~15 |
| **TOTAL** | **~270 unique maps** |

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-22 | Initial comprehensive audit |
