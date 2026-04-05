# Alchemist Class Research -- Pre-Renewal Classic

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../03_Server_Side/Skill_System.md) | [Alchemist_Skills_Audit](Alchemist_Skills_Audit_And_Fix_Plan.md)
> **Status**: COMPLETED — All research applied, bugs fixed

**Date:** 2026-03-15
**Status:** RESEARCH COMPLETE
**Scope:** Complete Alchemist class skills, Homunculus system, Potion Creation system, implementation gaps
**Sources:** iRO Wiki, iRO Wiki Classic, RateMyServer, rAthena pre-renewal DB, RagnaCloneDocs/12_Pets_Homunculus_Companions.md

---

## Table of Contents

1. [Class Overview](#1-class-overview)
2. [Skill List Summary](#2-skill-list-summary)
3. [Skill Tree & Prerequisites](#3-skill-tree--prerequisites)
4. [Detailed Skill Specifications](#4-detailed-skill-specifications)
5. [Potion Creation / Pharmacy System](#5-potion-creation--pharmacy-system)
6. [Homunculus System](#6-homunculus-system)
7. [Current Implementation Status](#7-current-implementation-status)
8. [Skill Definition Audit](#8-skill-definition-audit)
9. [New Systems Required](#9-new-systems-required)
10. [Implementation Priority](#10-implementation-priority)
11. [Integration Points](#11-integration-points)
12. [Data Tables Required](#12-data-tables-required)

---

## 1. Class Overview

| Property | Value |
|----------|-------|
| Base Class | Merchant |
| Job Change Level | Merchant Job Lv 40+ |
| Role | Potion crafter, support, Homunculus master, hybrid DPS |
| Weapon Types | Axes (primary), Daggers, 1H Swords, Maces |
| Unique Mechanics | Potion crafting, Homunculus companion, acid/bomb throwing |
| Transcendent Class | Biochemist (Creator) |
| 3rd Class | Geneticist (Renewal only -- out of scope) |

**Alchemist Identity:** The Alchemist is a unique hybrid class that combines potion crafting (Pharmacy), support throwing (Potion Pitcher), offensive bomb/acid skills (Acid Terror, Demonstration), plant/sphere summoning (Bio Cannibalize, Sphere Mine), equipment protection (Chemical Protection x4), and the Homunculus companion system. Unlike most classes, Alchemists rely heavily on consumable catalysts (Acid Bottles, Bottle Grenades, Plant Bottles, etc.) which must be crafted via Pharmacy or purchased.

---

## 2. Skill List Summary

### Combat Skills (17 total, IDs 1800-1815 + inherited Merchant)

| ID | Skill Name | Max Lv | Type | Target | Key Mechanic |
|----|-----------|--------|------|--------|-------------|
| 1804 | Axe Mastery | 10 | Passive | -- | +3 ATK/lv with Axes |
| 1805 | Potion Research | 10 | Passive | -- | +5% potion heal/lv, +1% brew rate/lv |
| 1800 | Pharmacy | 10 | Active | Self | Craft potions/items, +3% rate/lv |
| 1806 | Potion Pitcher | 5 | Active | Single ally | Throw potions to heal allies |
| 1801 | Acid Terror | 5 | Active | Single enemy | Ignores hard DEF, always hits, armor break |
| 1802 | Demonstration | 5 | Active | Ground 3x3 | Fire DoT zone, weapon break |
| 1803 | Summon Flora | 5 | Active | Ground | Summon plant allies |
| 1807 | Summon Marine Sphere | 5 | Active | Ground | Summon exploding sphere |
| 1808 | Chemical Protection Helm | 5 | Active | Single ally | Protect headgear |
| 1809 | Chemical Protection Shield | 5 | Active | Single ally | Protect shield |
| 1810 | Chemical Protection Armor | 5 | Active | Single ally | Protect armor |
| 1811 | Chemical Protection Weapon | 5 | Active | Single ally | Protect weapon |
| 1812 | Bioethics | 1 | Passive (Quest) | -- | Unlock Homunculus tree |
| 1813 | Call Homunculus | 1 | Active | Self | Summon Homunculus |
| 1814 | Rest (Vaporize) | 1 | Active | Self | Store Homunculus |
| 1815 | Resurrect Homunculus | 5 | Active | Self | Revive dead Homunculus |

### Inherited Merchant Skills

All Merchant skills (IDs 600-609) are inherited:
- Enlarge Weight Limit (10), Discount (10), Overcharge (10), Mammonite (10)
- Pushcart (10), Vending (10), Item Appraisal (1)
- Change Cart (1, quest), Cart Revolution (1, quest), Crazy Uproar (1, quest)

---

## 3. Skill Tree & Prerequisites

### Alchemist Skill Tree Layout

```
Row 0: [Axe Mastery]          [---]        [Potion Research]     [---]
Row 1: [---]                   [---]        [Pharmacy]            [Potion Pitcher]
Row 2: [Acid Terror]           [Demonstration] [Summon Flora]     [---]
Row 3: [Bioethics(Q)]          [---]        [Summon Marine Sphere] [CP Helm]
Row 4: [Rest/Vaporize]         [---]        [---]                 [CP Shield]
Row 5: [Call Homunculus]        [---]        [---]                 [CP Armor]
Row 6: [Resurrect Homunculus]   [---]        [---]                 [CP Weapon]
```

### Prerequisite Chains

```
Potion Research Lv5 --> Pharmacy
Pharmacy Lv2 --> Acid Terror
Pharmacy Lv2 --> Demonstration
Pharmacy Lv3 --> Potion Pitcher
Pharmacy Lv2 --> CP Helm
Pharmacy Lv6 --> Summon Flora (Bio Cannibalize)
Pharmacy Lv2 --> Summon Marine Sphere (Sphere Mine)

CP Helm Lv3 --> CP Shield Lv3 --> CP Armor Lv3 --> CP Weapon

Bioethics (Quest) --> Rest/Vaporize
Bioethics + Rest --> Call Homunculus
Call Homunculus --> Resurrect Homunculus
```

**Note:** Axe Mastery, Potion Research, Bioethics, Summon Flora, and Summon Marine Sphere have NO prerequisites within the Alchemist tree.

---

## 4. Detailed Skill Specifications

---

### 4.1 Axe Mastery (ID 1804) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 10 | All sources |
| Type | Passive | All sources |
| Weapons | Axes ONLY (not swords/maces in pre-renewal) | rAthena pre-re, RateMyServer |
| ATK Bonus | +3 per level (+3 to +30) | All sources |
| DEF Bypass | Bonus damage ignores armor DEF and VIT DEF | RateMyServer pre-re |
| Prerequisites | None | All sources |

**ATK Bonus per Level:**

| Lv | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|---|---|---|---|---|---|---|---|---|-----|
| +ATK | 3 | 6 | 9 | 12 | 15 | 18 | 21 | 24 | 27 | 30 |

**Special:** The bonus ATK from Axe Mastery bypasses both hard DEF and soft DEF (VIT defense). It applies to ALL hits in multi-hit attacks. This is the same behavior as Sword Mastery for Swordsman.

---

### 4.2 Potion Research / Learning Potion (ID 1805) -- Passive

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 10 | All sources |
| Type | Passive | All sources |
| Brew Rate | +1% per level (+1% to +10%) | iRO Wiki, RateMyServer |
| Potion Heal | +5% per level (+5% to +50%) | iRO Wiki, RateMyServer |
| Prerequisites | None | All sources |

**Per-Level Scaling:**

| Level | Brew Rate Bonus | Potion Heal Bonus |
|-------|-----------------|-------------------|
| 1 | +1% | +5% |
| 2 | +2% | +10% |
| 3 | +3% | +15% |
| 4 | +4% | +20% |
| 5 | +5% | +25% |
| 6 | +6% | +30% |
| 7 | +7% | +35% |
| 8 | +8% | +40% |
| 9 | +9% | +45% |
| 10 | +10% | +50% |

**Notes:**
- Potion heal bonus applies to potions consumed by the Alchemist AND potions thrown via Potion Pitcher
- Brew rate bonus stacks with Pharmacy skill level bonus
- Also affects potions consumed through Aid Condensed Potion (Biochemist skill)
- The heal bonus also enhances Acid Terror and Demonstration damage (+10 per level for Demonstration, +100 per level for Acid Terror -- from iRO Wiki Renewal data, may not apply in pre-renewal; needs verification during implementation)

---

### 4.3 Pharmacy / Prepare Potion (ID 1800) -- Active (Crafting)

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 10 | All sources |
| Type | Active (crafting) | All sources |
| SP Cost | 5 (all levels) | All sources |
| Target | Self | All sources |
| Cast Time | 0 | rAthena pre-re |
| Catalyst | 1 Medicine Bowl per attempt | All sources |
| Prerequisites | Potion Research Lv5 | All sources |

**Success Rate Formula (Pre-Renewal):**

```
SuccessRate = (Pharmacy_Lv * 3) + (PotionResearch_Lv * 1)
            + (JobLv * 0.2) + (DEX * 0.1) + (LUK * 0.1) + (INT * 0.05)
            + ItemDifficultyModifier
```

**Base Success Rate by Pharmacy Level:**

| Level | Base Rate |
|-------|-----------|
| 1 | 3% |
| 2 | 6% |
| 3 | 9% |
| 4 | 12% |
| 5 | 15% |
| 6 | 18% |
| 7 | 21% |
| 8 | 24% |
| 9 | 27% |
| 10 | 30% |

**Item Difficulty Modifiers:**

| Item Category | Rate Modifier |
|--------------|---------------|
| Red/Yellow/White Potions | +15% to +25% |
| Alcohol | +5% to +15% |
| Acid/Plant/Marine Bottles, Grenade | -5% to +5% |
| Blue Potion, Anodyne, Aloevera, Embryo | -5% |
| Condensed Red Potion | -5% |
| Condensed Yellow Potion | -10% to -5% |
| Condensed White Potion, Glistening Coat | -15% to -5% |

**Required Materials:** Each recipe requires a specific Creation Guide (book/manual) in inventory. The guide is NOT consumed. A Medicine Bowl IS consumed per attempt (success or fail).

**Adopted Alchemist Penalty:** -30% success rate.

**Fame System (optional, deferred):**
- 3 consecutive condensed potion successes = +1 fame point
- 5 consecutive = +3 points
- 7 consecutive = +10 points
- 10 consecutive = +50 points
- Top 10 ranked alchemists' potions get +50% efficacy bonus

See [Section 5](#5-potion-creation--pharmacy-system) for complete recipe list.

---

### 4.4 Acid Terror (ID 1801) -- Active, Ranged Physical

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, ranged physical | All sources |
| Target | Single enemy | All sources |
| Range | 9 cells | All sources |
| Element | Neutral (unaffected by elements) | iRO Wiki Classic |
| SP Cost | 15 (all levels) | All sources |
| Cast Time (Pre-Re) | 1 second | iRO Wiki Classic |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Acid Bottle per cast | All sources |
| Prerequisites | Pharmacy Lv5 | iRO Wiki, rAthena |
| Force Hit | YES -- always hits (ignores FLEE) | iRO Wiki Classic |
| Ignores Hard DEF | YES -- armor DEF is bypassed | All sources |
| VIT DEF | APPLIED (not bypassed) | RateMyServer |
| Boss Damage | Half damage (50%) against boss monsters | iRO Wiki |
| Card Interaction | +ATK cards increase damage; +% and status cards have NO effect | iRO Wiki |
| Blocked By | Pneuma (blocks damage, armor break still applies), Kyrie Eleison (blocks both), Guard (blocks both) | iRO Wiki |

**Pre-Renewal ATK% and Status Effects per Level:**

| Level | ATK% | Armor Break Chance | Bleeding Chance |
|-------|------|-------------------|-----------------|
| 1 | 140% | 3% | 3% |
| 2 | 180% | 7% | 6% |
| 3 | 220% | 10% | 9% |
| 4 | 260% | 12% | 12% |
| 5 | 300% | 13% | 15% |

**Formula:** `ATK% = 100 + 40 * SkillLevel`

**Damage Calculation:**
1. Calculate weapon ATK + flat ATK bonuses (mastery, cards)
2. Apply ATK% multiplier (140-300%)
3. Skip hard DEF (ignore armor)
4. Apply VIT soft DEF reduction
5. Force hit (no HIT/FLEE check)
6. Element table NOT applied (neutral vs neutral always)
7. Against bosses: final damage * 50%

**Armor Break:** On successful hit, chance to break target's armor. Displays `/omg` emote. In PvP, reduces armor to 0 DEF until repaired.

**Bleeding:** Standard bleeding status effect. HP drain over time, blocks natural HP regen.

**Equipment Bonuses (specific items):**
- Erde: +20% Acid Terror damage
- Red Square Bag: +20% Acid Terror damage

---

### 4.5 Demonstration / Bomb (ID 1802) -- Active, Ground AoE DoT

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, ground AoE, DoT | All sources |
| Target | Ground (3x3 area) | All sources |
| Range | 9 cells | All sources |
| Element | Fire property | All sources |
| SP Cost | 10 (all levels) | All sources |
| Cast Time (Pre-Re) | 1 second | iRO Wiki Classic |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Bottle Grenade per cast | All sources |
| Prerequisites | Pharmacy Lv4 | iRO Wiki, rAthena |
| Tick Rate | Damage every 0.5 seconds | iRO Wiki |
| Card Interaction | +ATK cards work; +% and status cards have NO effect | iRO Wiki |
| Stacking | Cannot stack bombs; cannot place adjacent to existing bombs | iRO Wiki |
| Placement | Cannot cast directly under enemies or adjacent to caster | iRO Wiki |

**Per-Level Scaling:**

| Level | ATK% per tick | Duration | Weapon Break Chance |
|-------|-------------|----------|---------------------|
| 1 | 120% | 40 sec | 1% |
| 2 | 140% | 45 sec | 2% |
| 3 | 160% | 50 sec | 3% |
| 4 | 180% | 55 sec | 4% |
| 5 | 200% | 60 sec | 5% |

**Formula:** `ATK% = 100 + 20 * SkillLevel` per tick (iRO Wiki Classic)

**Alternative formula (iRO Wiki Renewal):** `Base_Damage = 60 + 60 * SkillLevel` with `+10 * PotionResearch_Lv` bonus. Pre-renewal uses the simpler version above.

**Duration Formula:** `35 + 5 * SkillLevel` seconds (Classic) OR `40 + 5 * (SkillLevel - 1)` seconds. Sources vary slightly; implementation should use the table above.

**Damage Properties:**
- Fire element damage (applies fire vs defense element table)
- Physical ATK-based but uses only flat ATK (not % cards)
- Each tick checks if enemies are within the 3x3 area
- Enemies can walk into/out of the area
- Weapon break check per tick on each enemy in area

**Weapon Break:** Each tick has a chance to break the weapon of enemies standing in the fire zone.

---

### 4.6 Potion Pitcher / Aid Potion (ID 1806) -- Active, Support

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, supportive | All sources |
| Target | Single ally (party/guild member or self) | All sources |
| Range | 9 cells | All sources |
| SP Cost | 1 (all levels) | All sources |
| Cast Time | 0.5 seconds | RateMyServer |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Prerequisites | Pharmacy Lv3 | All sources |

**Potion Type and Effectiveness by Level:**

| Level | Potion Consumed | Base Heal Value | Effectiveness Multiplier |
|-------|----------------|-----------------|--------------------------|
| 1 | Red Potion | 45-65 HP | 110% |
| 2 | Orange Potion | 105-145 HP | 120% |
| 3 | Yellow Potion | 175-235 HP | 130% |
| 4 | White Potion | 325-405 HP | 140% |
| 5 | Blue Potion | Restores SP (40-60 SP) | 150% |

**Healing Formula (Pre-Renewal best estimate from multiple sources):**

```
HealAmount = floor(BasePotionValue * (SkillLevelMultiplier / 100))
           * (1 + PotionResearch_Lv * 0.05)

Where:
  BasePotionValue = random(min, max) of the potion type
  SkillLevelMultiplier = 100 + 10 * SkillLevel (110/120/130/140/150)
```

**Additional Modifiers:**
- Potion Research increases healing by +5% per level (stacks multiplicatively)
- VIT of the target increases healing received (via potion effectiveness modifiers)
- Increase Recuperative Power (if target has it) can up to double the heal amount
- Homunculus targets receive 3x healing (tripled)
- Consumes 1 potion of the appropriate type from inventory per cast

**Blue Potion (Lv5):** At level 5, throws a Blue Potion which restores SP instead of HP. The SP recovery follows the same effectiveness multiplier.

**Edge Cases:**
- Cannot target enemies
- Cannot target self with Blue Potion for SP recovery (some servers allow it)
- Potion must exist in caster's inventory

---

### 4.7 Summon Flora / Bio Cannibalize (ID 1803) -- Active, Summon

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, summon | All sources |
| Target | Ground (placement) | All sources |
| Range | 4 cells | All sources |
| SP Cost | 20 (all levels) | RateMyServer |
| Cast Time | 2 seconds (1.6 var + 0.4 fixed) | iRO Wiki |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Plant Bottle per cast | All sources |
| Prerequisites | Pharmacy Lv6 | All sources |
| Max Active | 5 total (across all types) | All sources |

**Summoned Plants by Level:**

| Level | Monster | Max Count | Duration | ATK Range | HP Formula |
|-------|---------|-----------|----------|-----------|------------|
| 1 | Mandragora | 5 | 5 minutes | 26-35 | 2430 (2230+200*1) |
| 2 | Hydra | 4 | 4 minutes | 22-28 | 2630 (2230+200*2) |
| 3 | Flora | 3 | 3 minutes | 242-273 | 2830 (2230+200*3) |
| 4 | Parasite | 2 | 2 minutes | 215-430 | 3030 (2230+200*4) |
| 5 | Geographer | 1 | 1 minute | 467-621 | 3230 (2230+200*5) |

**Plant HP Formula:** `2230 + 200 * SkillLevel` per plant

**Plant Behavior:**
- Plants attack enemies within range automatically
- Monsters do NOT attack summoned plants (plants are treated as allies)
- Plant attacks count as the summoner's attacks (trigger autocast effects, aggro goes to summoner)
- Only one plant type active at a time (summoning a different type replaces existing plants)
- Cannot be used in towns

**Geographer Special (Lv5):** Casts Heal every 5 seconds on nearby players/allies with HP below 60% of max, restoring approximately 850-900 HP. Can heal undead-property players (does not deal damage to them).

---

### 4.8 Summon Marine Sphere / Sphere Mine (ID 1807) -- Active, Summon

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, summon | All sources |
| Target | Ground (placement) | All sources |
| Range | 1 cell | All sources |
| SP Cost | 10 (all levels) | All sources |
| Cast Time | 2 seconds (1.6 var + 0.4 fixed) | iRO Wiki |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Duration | 30 seconds (or until detonation) | iRO Wiki |
| Catalyst | 1 Marine Sphere Bottle per cast | All sources |
| Prerequisites | Pharmacy Lv2 | All sources |
| Max Active | 3 | iRO Wiki |

**Sphere HP by Level:**

| Level | HP |
|-------|-----|
| 1 | 2,400 |
| 2 | 2,800 |
| 3 | 3,200 |
| 4 | 3,600 |
| 5 | 4,000 |

**HP Formula:** `2000 + 400 * SkillLevel`

**Self-Destruct Mechanics:**
- Sphere detonates when it receives any damage (hit by enemy or player)
- Can also be "tapped" (clicked) to move ~7 cells in a direction, then detonates
- Detonation damage is Fire property
- Detonation AoE: 11x11 cells around sphere
- Damage scales with sphere's remaining HP (more HP = more damage; healing increases damage)
- In PvP: damages allies and caster
- Does NOT give EXP to the caster for kills from explosion
- Sphere drops normal Marine Sphere loot if killed before exploding

---

### 4.9 Chemical Protection Helm (ID 1808) -- Active, Support

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, supportive | All sources |
| Target | Single ally or self | All sources |
| Range | 1 cell | All sources |
| SP Cost | 20 (all levels) | All sources |
| Cast Time | 2 seconds fixed | All sources |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Glistening Coat per cast | All sources |
| Prerequisites | Pharmacy Lv2 | iRO Wiki |
| Protection | Headgear slot -- protects from break AND strip (Divest) | All sources |

**Duration per Level:**

| Level | Duration |
|-------|----------|
| 1 | 2 minutes (120s) |
| 2 | 4 minutes (240s) |
| 3 | 6 minutes (360s) |
| 4 | 8 minutes (480s) |
| 5 | 10 minutes (600s) |

**Formula:** `Duration = SkillLevel * 120 seconds`

---

### 4.10 Chemical Protection Shield (ID 1809) -- Active, Support

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| SP Cost | 25 (all levels) | All sources |
| Cast Time | 2 seconds fixed | All sources |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Glistening Coat per cast | All sources |
| Prerequisites | CP Helm Lv3 | All sources |
| Protection | Shield slot | All sources |
| Range | 1 cell | All sources |
| Duration | Same as CP Helm (120s * SkillLevel) | All sources |

---

### 4.11 Chemical Protection Armor (ID 1810) -- Active, Support

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| SP Cost | 25 (all levels) | All sources |
| Cast Time | 2 seconds fixed | All sources |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Glistening Coat per cast | All sources |
| Prerequisites | CP Shield Lv3 | All sources |
| Protection | Armor slot (also protects armor element) | All sources |
| Range | 1 cell | All sources |
| Duration | Same formula (120s * SkillLevel) | All sources |

---

### 4.12 Chemical Protection Weapon (ID 1811) -- Active, Support

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| SP Cost | 30 (all levels) | All sources |
| Cast Time | 2 seconds fixed | All sources |
| After-Cast Delay | 0.5 seconds | iRO Wiki |
| Catalyst | 1 Glistening Coat per cast | All sources |
| Prerequisites | CP Armor Lv3 | All sources |
| Protection | Weapon slot | All sources |
| Range | 1 cell | All sources |
| Duration | Same formula (120s * SkillLevel) | All sources |

**Chemical Protection Notes (all 4):**
- Protects equipment from: weapon/armor break mechanics, Rogue's Divest/Strip skills, Whitesmith's Shattering Strike
- Does NOT protect from: unequip mechanics, cursed items
- Only one CP buff per slot at a time (recasting replaces)
- Buff icon displayed in buff bar with countdown timer
- Dispelled by: Dispell (Sage skill)

---

### 4.13 Bioethics (ID 1812) -- Passive (Quest Skill)

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 1 | All sources |
| Type | Passive (quest-learned) | All sources |
| Prerequisites | Complete Bioethics Quest | All sources |
| Effect | Unlocks Homunculus skill tree (Rest, Call Homunculus, Resurrect Homunculus) | All sources |

**Quest:** Speak to specific NPCs (Al de Baran Alchemist Guild). Quest details vary by server. In our implementation, this can be auto-learned or learned from the skill tree (standard simplification).

---

### 4.14 Rest / Vaporize (ID 1814) -- Active

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 1 | All sources |
| SP Cost | 50 | All sources |
| Cast Time | 0 | rAthena pre-re |
| Cooldown | 20 seconds | RateMyServer (Renewal), unclear for pre-re |
| Target | Self (active Homunculus) | All sources |
| Prerequisites | Bioethics Lv1 | All sources |
| Requirement | Homunculus HP must be >= 80% of MaxHP | RagnaCloneDocs |

**Effect:** Stores the active Homunculus. The Homunculus disappears from the field and can be re-summoned later with Call Homunculus (no Embryo needed). Intimacy is NOT reduced. State (HP/SP/stats/skills) is preserved.

---

### 4.15 Call Homunculus (ID 1813) -- Active

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 1 | All sources |
| SP Cost | 10 | All sources |
| Cast Time | 0 | rAthena pre-re |
| Catalyst | 1 Embryo (first summon ONLY) | All sources |
| Prerequisites | Bioethics Lv1, Rest Lv1 | All sources |
| Duration | 1800 seconds (30 min) in Renewal; indefinite in pre-renewal | Sources differ |

**First Summon:**
- Requires an Embryo item in inventory (consumed)
- Homunculus type is determined randomly: 25% chance each (Lif, Amistr, Filir, Vanilmirth)
- 50% chance for each visual variant within a type
- Homunculus is permanently bound to the character

**Subsequent Summons:**
- No Embryo needed (re-summons the stored Homunculus)
- Homunculus retains all state (level, stats, skills, intimacy)

**Delete and Re-roll:**
- Click "del" button in Homunculus info window to permanently delete current Homunculus
- Use Call Homunculus with a new Embryo to get a new random Homunculus

---

### 4.16 Resurrect Homunculus (ID 1815) -- Active

| Property | Value | Source |
|----------|-------|--------|
| Max Level | 5 | All sources |
| Type | Active, supportive | All sources |
| Target | Self (dead Homunculus) | All sources |
| Cast Time | 3 seconds (1 fixed + 2 variable) | iRO Wiki |
| Prerequisites | Call Homunculus Lv1 | All sources |

**Per-Level Scaling:**

| Level | HP Restored | SP Cost | Cooldown |
|-------|------------|---------|----------|
| 1 | 20% | 74 | 140s |
| 2 | 40% | 68 | 110s |
| 3 | 60% | 62 | 80s |
| 4 | 80% | 56 | 50s |
| 5 | 100% | 50 | 20s |

**SP Cost Formula:** `80 - (SkillLevel * 6)` -- Note: this differs from current implementation (`50 + i*6`)
**Cooldown Formula:** `170 - (SkillLevel * 30)` seconds

---

## 5. Potion Creation / Pharmacy System

### 5.1 Overview

The Pharmacy system allows Alchemists to craft potions, bottles, and special items. Each craft attempt requires:
1. The Pharmacy skill (active)
2. A **Creation Guide / Manual** appropriate for the item (NOT consumed, must be in inventory)
3. The recipe **ingredients** (ALL consumed on attempt, success or fail)
4. A **Medicine Bowl** (consumed per attempt)

### 5.2 Creation Guides (Books/Manuals)

| Guide | Items It Unlocks |
|-------|-----------------|
| Potion Creation Guide | Red/Yellow/White Potions, Alcohol, Acid Bottle, Plant Bottle, Marine Sphere Bottle, Bottle Grenade, Glistening Coat, Anodyne, Aloevera, Embryo, Homunculus Tablet |
| Condensed Potion Creation Guide | Condensed Red/Yellow/White Potions |
| Elemental Potion Guide | Fireproof/Coldproof/Thunderproof/Earthproof Potions |

### 5.3 Complete Recipe List

#### Basic Potions

| Item | Ingredients | Guide |
|------|------------|-------|
| Red Potion | 1 Empty Potion Bottle + 1 Red Herb | Potion Creation Guide |
| Yellow Potion | 1 Empty Potion Bottle + 1 Yellow Herb | Potion Creation Guide |
| White Potion | 1 Empty Potion Bottle + 1 White Herb | Potion Creation Guide |
| Blue Potion | 1 Empty Potion Bottle + 1 Blue Herb + 1 Scell | Potion Creation Guide |

#### Utility Potions

| Item | Ingredients | Guide |
|------|------------|-------|
| Anodyne | 1 Empty Bottle + 1 Alcohol + 1 Ment | Potion Creation Guide |
| Aloevera | 1 Empty Bottle + 1 Honey + 1 Aloe | Potion Creation Guide |

#### Condensed Potions

| Item | Ingredients | Guide |
|------|------------|-------|
| Condensed Red Potion | 1 Red Potion + 1 Cactus Needle | Condensed Potion Guide |
| Condensed Yellow Potion | 1 Yellow Potion + 1 Mole Whiskers | Condensed Potion Guide |
| Condensed White Potion | 1 White Potion + 1 Witch Starsand | Condensed Potion Guide |

#### Combat Bottles (Skill Catalysts)

| Item | Ingredients | Guide | Used By |
|------|------------|-------|---------|
| Alcohol | 1 Empty Test Tube + 1 Empty Bottle + 5 Stems + 5 Poison Spores | Potion Creation Guide | Component for Bottle Grenade + Glistening Coat |
| Acid Bottle | 1 Empty Bottle + 1 Immortal Heart | Potion Creation Guide | Acid Terror catalyst |
| Plant Bottle | 1 Empty Bottle + 2 Maneater Blossoms | Potion Creation Guide | Summon Flora catalyst |
| Marine Sphere Bottle | 1 Empty Bottle + 1 Tendon + 1 Detonator | Potion Creation Guide | Summon Marine Sphere catalyst |
| Bottle Grenade | 1 Empty Bottle + 1 Fabric + 1 Alcohol | Potion Creation Guide | Demonstration catalyst |
| Glistening Coat | 1 Empty Bottle + 1 Mermaid's Heart + 1 Zenorc Fang + 1 Alcohol | Potion Creation Guide | Chemical Protection catalyst |

#### Elemental Resistance Potions

| Item | Ingredients | Guide |
|------|------------|-------|
| Fireproof Potion | 1 Empty Potion Bottle + 1 Red Gemstone + 2 Frills | Elemental Potion Guide |
| Coldproof Potion | 1 Empty Potion Bottle + 1 Blue Gemstone + 3 Mermaid's Hearts | Elemental Potion Guide |
| Thunderproof Potion | 1 Blue Gemstone + 3 Moth Dust | Elemental Potion Guide |
| Earthproof Potion | 1 Yellow Gemstone + 2 Large Jellopies | Elemental Potion Guide |

#### Special Items

| Item | Ingredients | Guide | Notes |
|------|------------|-------|-------|
| Embryo | 1 Medicine Bowl + 1 Glass Tube + 1 Morning Dew of Yggdrasil + 1 Seed of Life | Potion Creation Guide | Requires Bioethics skill |
| Homunculus Tablet | 1 Yellow Herb + 1 Seed of Life + 1 Empty Bottle | Potion Creation Guide | Requires Bioethics, restores +50 Homunculus intimacy |

### 5.4 Success Rate Details

**Full Success Rate Calculation:**

```
TotalRate = BaseRate + StatBonus + ItemModifier

Where:
  BaseRate = Pharmacy_Lv * 3 + PotionResearch_Lv * 1
  StatBonus = floor(JobLv * 0.2) + floor(DEX * 0.1) + floor(LUK * 0.1) + floor(INT * 0.05)
  ItemModifier = per-item difficulty (see table above)
```

**Example Calculation (Lv10 Pharmacy, Lv10 Potion Research, Job50, DEX90, LUK50, INT80):**
```
BaseRate = 30 + 10 = 40%
StatBonus = 10 + 9 + 5 + 4 = 28%
Total for White Potion (+20%): 40 + 28 + 20 = 88%
Total for Glistening Coat (-10%): 40 + 28 - 10 = 58%
Total for Embryo (-5%): 40 + 28 - 5 = 63%
```

**Vanilmirth Bonus:** If the Alchemist has a Vanilmirth Homunculus with Instruction Change, add +1% to +5% (per skill level).

---

## 6. Homunculus System

### 6.1 Overview

The Homunculus is a permanent AI companion exclusive to Alchemists. It fights alongside the player, gains EXP, levels up independently, has its own stats and skills, and can evolve.

### 6.2 Obtaining a Homunculus

1. Learn Bioethics (quest skill)
2. Craft an Embryo using Pharmacy skill
3. Use Call Homunculus with Embryo in inventory
4. Random type selected: 25% each (Lif/Amistr/Filir/Vanilmirth)

### 6.3 The Four Base Types

#### 6.3.1 Lif (Support/Healing)

| Property | Value |
|----------|-------|
| Race | Demi Human |
| Element | Neutral |
| Food | Pet Food |
| Role | Healer / Support |

**Starting Stats:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 150 | 40 | 12 | 20 | 15 | 35 | 24 | 12 |

**Average Growth Per Level:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +80 | +6.5 | +0.67 | +0.67 | +0.67 | +0.71 | +0.80 | +0.80 |

**Lv99 Stat Ranges (5th-95th percentile):**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 7,790-8,170 | 650-704 | 71-85 | 79-93 | 74-88 | 95-113 | 94-111 | 82-99 |

**Skills:**

| Skill | Type | Max Lv | SP Cost | Effect |
|-------|------|--------|---------|--------|
| Healing Hands | Active | 5 | 13/16/19/22/25 | Heals owner. Consumes 1 Condensed Red Potion. +0/20/40/60/80% heal at Lv1-5 |
| Urgent Escape | Active | 5 | -- | +10/20/30/40/50% move speed for 40/35/30/25/20s. CD: 60/70/80/90/120s |
| Brain Surgery | Passive | 5 | -- | +1-5% Max SP, +2-10% Healing Hands effectiveness, +3-15% SP Recovery |
| Mental Charge | Active (Evolved) | 3 | -- | Uses MATK instead of ATK for 1/3/5 min. +30/60/90 VIT, +20/40/60 INT |

#### 6.3.2 Amistr (Tank)

| Property | Value |
|----------|-------|
| Race | Brute |
| Element | Neutral |
| Food | Zargon |
| Role | Tank / Damage Soak |

**Starting Stats:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 320 | 10 | 20 | 17 | 35 | 11 | 24 | 12 |

**Average Growth Per Level:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +105 | +2.5 | +0.92 | +0.71 | +0.71 | +0.10 | +0.59 | +0.59 |

**Lv99 Stat Ranges (5th-95th percentile):**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 10,361-10,840 | 250-260 | 104-118 | 77-95 | 95-113 | 16-25 | 74-89 | 62-77 |

**Skills:**

| Skill | Type | Max Lv | SP Cost | Effect |
|-------|------|--------|---------|--------|
| Castling | Active | 5 | 10 | Swap Amistr+owner positions. Success: 20/40/60/80/100% |
| Amistr Bulwark | Active | 5 | -- | +10/15/20/25/30 VIT for 40/35/30/25/20s |
| Adamantium Skin | Passive | 5 | -- | +2/4/6/8/10% MaxHP, +5/10/15/20/25% HP Recovery, +4/8/12/16/20 DEF |
| Blood Lust | Active (Evolved) | 3 | -- | +30/40/50% ATK, 20% lifesteal on each hit |

#### 6.3.3 Filir (Speed/DPS)

| Property | Value |
|----------|-------|
| Race | Brute |
| Element | Neutral |
| Food | Garlet |
| Role | Fast Attacker / DPS |

**Starting Stats:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 90 | 25 | 29 | 35 | 9 | 8 | 30 | 9 |

**Average Growth Per Level:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +60 | +4.5 | +0.71 | +0.92 | +0.10 | +0.59 | +0.71 | +0.59 |

**Lv99 Stat Ranges (5th-95th percentile):**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 5,820-6,110 | 461-471 | 89-107 | 119-133 | 14-23 | 58-73 | 90-108 | 59-74 |

**Stat Growth Probability:**

| Stat | +0 | +1 | +2 |
|------|-----|-----|-----|
| STR | 35.29% | 58.82% | 5.88% |
| AGI | 15.38% | 76.92% | 7.69% |
| VIT | 90.00% | 10.00% | 0% |
| INT | 41.17% | 58.82% | 0% |
| DEX | 35.29% | 58.82% | 5.88% |
| LUK | 41.17% | 58.82% | 0% |

**Skills:**

| Skill | Type | Max Lv | Effect |
|-------|------|--------|--------|
| Moonlight | Offensive | 5 | 1/2/2/2/3 hits, 220/330/440/550/660% ATK |
| Flitting | Active | 5 | +3/6/9/12/15% ASPD, +10/15/20/25/30% ATK for 60/55/50/45/40s. CD: 60/70/80/90/120s |
| Accelerated Flight | Active | 5 | +20/30/40/50/60 FLEE for 60/55/50/45/40s. CD: 60/70/80/90/120s |
| S.B.R.44 | Offensive (Evolved) | 3 | Damage = Intimacy * 100/200/300. Drops intimacy to 2. Requires >= 3 intimacy |

#### 6.3.4 Vanilmirth (Magic/Chaos)

| Property | Value |
|----------|-------|
| Race | Formless |
| Element | Neutral |
| Food | Scell |
| Role | Magic DPS / RNG |

**Starting Stats:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 80 | 11 | 11 | 11 | 11 | 11 | 11 | 11 |

**Average Growth Per Level:**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +90 | +3.5 | +1.1 | +1.1 | +1.1 | +1.1 | +1.1 | +1.1 |

**Stat Growth Probability (identical for all 6 stats):**

| +0 | +1 | +2 | +3 |
|----|-----|-----|-----|
| 30.00% | 33.33% | 33.33% | 3.33% |

**Lv99 Stat Ranges (5th-95th percentile):**

| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 8,321-9,449 | 335-373 | 105-133 | 105-133 | 105-133 | 105-133 | 105-133 | 105-133 |

**Skills:**

| Skill | Type | Max Lv | SP Cost | Effect |
|-------|------|--------|---------|--------|
| Caprice | Offensive | 5 | 22/24/26/28/30 | Random Lv1-5 Fire Bolt/Cold Bolt/Lightning Bolt/Earth Spike |
| Chaotic Blessings | Supportive | 5 | -- | Random Lv1-5 Heal on random target (enemy, self, OR owner) |
| Instruction Change | Passive | 5 | -- | +1/2/3/4/5 INT, +1/1/2/3/4 STR, +1/2/3/4/5% brew rate |
| Self-Destruction | Offensive (Evolved) | 3 | -- | MaxHP * 1/1.5/2 piercing damage in 9x9 AoE. Intimacy drops to 1. Requires >= 450 intimacy |

### 6.4 Homunculus Derived Stats

```
ATK  = floor((STR + DEX + LUK) / 3) + floor(Level / 10)
MATK = Level + INT + floor((INT + DEX + LUK) / 3) + floor(Level / 10) * 2
HIT  = Level + DEX + 150
CRIT = floor(LUK / 3) + 1
DEF  = (VIT + floor(Level / 10)) * 2 + floor((AGI + floor(Level / 10)) / 2) + floor(Level / 2)
FLEE = Level + AGI + floor(Level / 10)
ASPD = 130 (base, 1.4 second attack interval)
```

### 6.5 Intimacy System

**Range:** 0-1000

| Status | Range | Effect |
|--------|-------|--------|
| Hate with Passion | 1-3 | About to abandon |
| Hate | 4-10 | Very unhappy |
| Awkward | 11-100 | Uncomfortable |
| Shy | 101-250 | Timid |
| Neutral | 251-750 | Default |
| Cordial | 751-910 | Friendly |
| Loyal | 911-1000 | Eligible for evolution |

**Feeding Intimacy Changes (based on hunger %):**

| Hunger Range | Intimacy Change |
|--------------|-----------------|
| 1-10% | +0.5 |
| 11-25% (optimal) | +1.0 |
| 26-75% | +0.75 |
| 76-90% | -0.05 |
| 91-100% | -0.5 |
| 0% (starving) | -1.0 per tick |

**Starvation:** Loses 18 intimacy per hour. After 24 hours (432 total loss), Homunculus is permanently abandoned.

### 6.6 Evolution

**Requirements:**
- Intimacy must be Loyal (911+)
- Stone of Sage item (consumed)
- No level requirement

**Evolution Effects:**
- +1 to +10 random bonus for each of the 6 stats
- Increased Max HP and Max SP
- New visual sprite
- Unlocks 4th skill (ultimate)
- Intimacy resets to 10 (Hate)

**Evolved Intimacy Requirements for Ultimate Skills:**
- Lif (Mental Charge): Awkward (50+)
- Amistr (Blood Lust): Awkward (50+)
- Filir (S.B.R.44): 2+ intimacy (almost no requirement)
- Vanilmirth (Self-Destruction): 450+ intimacy

### 6.7 EXP and Leveling

| Property | Value |
|----------|-------|
| Level Cap | 99 |
| Player EXP Share | 90% of Base/Job EXP |
| Homunculus EXP Share | 10% (regardless of damage) |
| Skill Points | 1 per 3 levels (33 total at Lv99) |
| Total EXP to 99 | 203,562,540 |

### 6.8 Homunculus AI

**Default Behavior:**
- Follow owner at 2-3 cell distance
- Attack owner's target when ordered
- Use skills automatically based on cooldowns
- Return to owner if too far (>15 cells: teleport)

**Combat Rules:**
- Hit rate capped at 95%
- Can hit Ghost-property monsters
- FLEE not reduced by mob count
- Base ASPD: 130 (1.4s interval)

**Commands:**
| Action | Control |
|--------|---------|
| Attack target | Alt + Right Click (enemy) |
| Standby toggle | Alt + T |
| Move to location | Alt + Right Click (ground, 15-tile range) |
| Open info | Alt + R |

### 6.9 Homunculus Death

- Homunculus vanishes on death
- Revive with Resurrect Homunculus skill
- Death does NOT reduce intimacy
- Owner death does NOT reduce intimacy
- Vaporize does NOT reduce intimacy

---

## 7. Current Implementation Status

### 7.1 Skill Definitions (ro_skill_data_2nd.js)

All 17 Alchemist skills are defined with IDs 1800-1815. Below is the current state:

| ID | Skill | Status | Issues Found |
|----|-------|--------|-------------|
| 1800 | Pharmacy | Definition exists | Wrong cooldown (500ms, should be 0); no handler |
| 1801 | Acid Terror | Definition exists | Wrong ATK% (140+40i vs correct 140+40i -- matches pre-re Classic); wrong prerequisite (Pharmacy Lv2, should be Lv5); wrong range (450, should be 900); missing castTime (should be 1000ms); wrong cooldown (500, should be afterCastDelay 500); no handler |
| 1802 | Demonstration | Definition exists | Wrong range (450, should be 900); wrong castTime (0, should be 1000ms); wrong cooldown (1000, should be afterCastDelay 500); no handler |
| 1803 | Summon Flora | Definition exists | Wrong SP cost (10, should be 20); missing cast time (should be 2000ms); missing prerequisite (Pharmacy Lv6); wrong range (300, should be 400); wrong cooldown (3000, should be afterCastDelay 500); no handler |
| 1804 | Axe Mastery | Definition exists | CORRECT (passive, +3/lv) |
| 1805 | Potion Research | Definition exists | CORRECT (passive, +5% heal/lv, +1% brew/lv stored as effectValue) |
| 1806 | Potion Pitcher | Definition exists | Wrong effectiveness (20+10i = 20-60%, should be 110-150%); wrong range (450, should be 900); wrong cooldown (500, should be afterCastDelay 500); no handler |
| 1807 | Summon Marine Sphere | Definition exists | Missing prerequisite (Pharmacy Lv2); missing castTime (should be 2000ms); wrong range (300, should be 100); wrong cooldown (3000, should be afterCastDelay 500); no handler |
| 1808 | CP Helm | Definition exists | MOSTLY CORRECT -- SP/duration correct; missing afterCastDelay 500 |
| 1809 | CP Shield | Definition exists | MOSTLY CORRECT; prereq chain correct |
| 1810 | CP Armor | Definition exists | MOSTLY CORRECT; prereq chain correct |
| 1811 | CP Weapon | Definition exists | MOSTLY CORRECT; prereq chain correct |
| 1812 | Bioethics | Definition exists | CORRECT |
| 1813 | Call Homunculus | Definition exists | CORRECT (SP 10, prereqs correct) |
| 1814 | Rest | Definition exists | CORRECT (SP 50) |
| 1815 | Resurrect Homunculus | Definition exists | WRONG SP formula (50+6i gives 50-74, should be 74-50 decreasing: 80-6*lv); missing castTime (should be 3000ms); missing cooldown (170-30*lv seconds) |

### 7.2 Skill Handlers (index.js)

**NO Alchemist skill handlers exist.** Grep for all Alchemist skill names in `index.js` returns zero matches. All 17 skills are definition-only stubs with no server-side execution logic.

### 7.3 Inherited Merchant Skills

All 10 Merchant skills are defined. Mammonite and Cart Revolution have handlers. See `Merchant_Skills_Audit_And_Fix_Plan.md` for details on their issues.

---

## 8. Skill Definition Audit

### 8.1 Corrections Needed

#### Acid Terror (1801) -- MULTIPLE ISSUES

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `prerequisites` | `Pharmacy Lv2` | `Pharmacy Lv5` | iRO Wiki, rAthena |
| `range` | `450` | `900` (9 cells) | All sources |
| `castTime` | `0` | `1000` (1 sec, pre-renewal) | iRO Wiki Classic |
| `cooldown` | `500` | `0` | N/A -- should use afterCastDelay |
| `afterCastDelay` | missing | `500` | iRO Wiki |
| `effectValue` | `140+i*40` (140-300%) | `140+i*40` (140-300%) | CORRECT for pre-renewal |
| `forceHit` | missing | `true` | Always hits |
| `ignoreHardDef` | missing | `true` | Ignores armor DEF |

#### Demonstration (1802) -- MULTIPLE ISSUES

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `range` | `450` | `900` (9 cells) | All sources |
| `castTime` | `0` | `1000` (1 sec) | iRO Wiki Classic |
| `cooldown` | `1000` | `0` | Should use afterCastDelay |
| `afterCastDelay` | missing | `500` | iRO Wiki |
| `effectValue` | `100+i*20` (120-200%) | `100+i*20` (120-200%) | CORRECT |
| `duration` | `40000+i*5000` (40-60s) | `40000+i*5000` (40-60s) | CORRECT |

#### Summon Flora (1803) -- MULTIPLE ISSUES

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `spCost` | `10` | `20` | RateMyServer, all sources |
| `castTime` | `0` | `2000` (2 sec) | iRO Wiki |
| `cooldown` | `3000` | `0` | Should use afterCastDelay |
| `afterCastDelay` | missing | `500` | iRO Wiki |
| `range` | `300` | `400` (4 cells) | All sources |
| `prerequisites` | `[]` (none) | `[{ skillId: 1800, level: 6 }]` (Pharmacy Lv6) | All sources |

#### Potion Pitcher (1806) -- EFFECTIVENESS ISSUE

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `effectValue` | `20+i*10` (20-60%) | Should be `110+i*10` (110-150%) | All sources |
| `range` | `450` | `900` (9 cells) | All sources |
| `castTime` | `0` | `500` (0.5 sec) | RateMyServer |
| `cooldown` | `500` | `0` | Should use afterCastDelay |
| `afterCastDelay` | missing | `500` | iRO Wiki |

#### Summon Marine Sphere (1807) -- MULTIPLE ISSUES

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `prerequisites` | `[]` (none) | `[{ skillId: 1800, level: 2 }]` (Pharmacy Lv2) | All sources |
| `castTime` | `0` | `2000` (2 sec) | iRO Wiki |
| `range` | `300` | `100` (1 cell) | iRO Wiki |
| `cooldown` | `3000` | `0` | Should use afterCastDelay |
| `afterCastDelay` | missing | `500` | iRO Wiki |

#### Pharmacy (1800) -- MINOR

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `cooldown` | `500` | `0` | No cooldown in pre-renewal |

#### Resurrect Homunculus (1815) -- SP FORMULA REVERSED

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `spCost` | `50+i*6` (50-74, increasing) | `80-6*(i+1)` (74-50, decreasing) | iRO Wiki |
| `castTime` | `0` | `3000` (3 sec) | iRO Wiki |
| `cooldown` | `0` | Should be `(170-30*(i+1))*1000` (140s-20s) | iRO Wiki |

#### Chemical Protection (all 4) -- MINOR

| Field | Current | Correct | Source |
|-------|---------|---------|--------|
| `afterCastDelay` | missing | `500` on all 4 | iRO Wiki |

---

## 9. New Systems Required

### 9.1 Potion Crafting System (Priority: HIGH)

**Required for:** Pharmacy skill, all catalyst creation

**Components:**
1. `POTION_RECIPES` data table -- recipe definitions (item IDs, quantities, creation guide required)
2. `pharmacy` skill handler -- consume ingredients, roll success/fail, grant item
3. Creation Guide items in item DB (Potion Creation Guide, Condensed Potion Guide, Elemental Potion Guide)
4. Medicine Bowl item in item DB (catalyst consumed per attempt)
5. Success rate calculator (DEX/LUK/INT/JobLv/Pharmacy_Lv/PotionResearch_Lv)
6. Pharmacy UI on client (show available recipes based on inventory + guides owned)
7. `pharmacy:brew` socket event (attempt), `pharmacy:result` socket event (success/fail)

**Estimated Effort:** Medium-Large (recipe data + handler + client UI)

### 9.2 Catalyst Consumption System (Priority: HIGH)

**Required for:** Acid Terror, Demonstration, Summon Flora, Summon Marine Sphere, Chemical Protection x4

**Components:**
1. Server-side check: verify catalyst item in inventory before skill execution
2. Consume 1 catalyst on cast start (not on hit)
3. Item IDs for catalysts: Acid Bottle, Bottle Grenade, Plant Bottle, Marine Sphere Bottle, Glistening Coat

**Estimated Effort:** Small (add to each handler: check inventory, consume item)

### 9.3 Acid Terror Handler (Priority: HIGH)

**Required for:** Core Alchemist combat skill

**New Mechanics:**
1. Force-hit flag (already needed for Cart Revolution -- see Merchant audit)
2. Ignore hard DEF flag (skip armor DEF, apply only VIT soft DEF)
3. Armor break status effect (new: reduce target armor to 0 DEF, visual indicator)
4. Bleeding status effect (already implemented in buff system)
5. Half damage vs bosses flag
6. Catalyst consumption (1 Acid Bottle)

**Handler Pseudocode:**
```js
// Acid Terror handler
if (skill.name === 'acid_terror') {
    // 1. Check & consume Acid Bottle from inventory
    // 2. Deduct SP
    // 3. Calculate damage:
    //    - Base ATK * (100 + 40 * skillLv) / 100
    //    - Skip hard DEF (ignore equipment DEF)
    //    - Apply VIT soft DEF
    //    - Force hit (skip HIT/FLEE)
    //    - Element: neutral (no element modifiers)
    //    - Halve vs boss: if (target.isBoss) damage = floor(damage / 2)
    // 4. Apply damage
    // 5. Roll armor break: 3/7/10/12/13% per level
    // 6. Roll bleeding: 3/6/9/12/15% per level
    // 7. Broadcast skill:effect_damage
}
```

**Estimated Effort:** Medium

### 9.4 Demonstration Handler (Priority: MEDIUM)

**Required for:** Alchemist ground AoE DoT

**New Mechanics:**
1. Ground zone system (persistent area effect on the map)
2. Tick-based damage (every 500ms for the zone's duration)
3. Fire element damage per tick
4. Weapon break chance per tick
5. Zone collision prevention (no stacking, no adjacent placement)
6. Catalyst consumption (1 Bottle Grenade)

**Ground Zone Architecture:**
```js
// Server-side ground zone object
{
    id: uniqueId,
    type: 'demonstration',
    zone: playerZone,
    x: targetX, y: targetY, z: targetZ,
    radius: 150, // 3x3 cells in UE units
    element: 'fire',
    atkPercent: 100 + 20 * skillLv,
    weaponBreakChance: skillLv * 1, // 1-5%
    duration: 40000 + skillLv * 5000,
    tickInterval: 500,
    casterId: characterId,
    casterStats: getEffectiveStats(player),
    createdAt: Date.now()
}
// Add to zone registry, tick loop processes all active zones
```

**Estimated Effort:** Large (new ground zone system)

### 9.5 Summon System (Priority: MEDIUM)

**Required for:** Summon Flora, Summon Marine Sphere

**Components:**
1. Summoned entity registry on server (separate from enemy registry)
2. Summoned entity spawning (position, HP, stats, AI behavior)
3. Summoned entity death/expiry handling
4. Client-side summoned entity rendering (reuse enemy actor with special flag)
5. Max summon count enforcement
6. Duration timers per summon
7. Plant-specific AI (auto-attack nearby enemies)
8. Marine Sphere-specific AI (self-destruct on damage)
9. Catalyst consumption per summon

**Estimated Effort:** Large (new entity type + AI)

### 9.6 Potion Pitcher Handler (Priority: MEDIUM)

**Required for:** Ally healing support

**Components:**
1. Target selection (ally/self, not enemy)
2. Potion type determination (by skill level)
3. Check potion in caster's inventory, consume it
4. Calculate heal amount (base potion value * effectiveness% * Potion Research bonus)
5. Apply heal to target
6. Broadcast heal effect

**Estimated Effort:** Medium

### 9.7 Chemical Protection Handler (Priority: LOW)

**Required for:** Equipment protection in PvP/strip mechanics

**Components:**
1. Apply CP buff to target's specific equipment slot
2. Buff prevents: weapon/armor break, strip skills (Rogue Divest)
3. Duration based on skill level (120s * lv)
4. Catalyst consumption (1 Glistening Coat per cast)
5. Only meaningful once strip/break mechanics are implemented

**Estimated Effort:** Small (buff application, but depends on strip/break systems)

### 9.8 Homunculus System (Priority: LOW -- Major Feature)

**Required for:** Call Homunculus, Rest, Resurrect Homunculus, all Homunculus gameplay

This is the largest new system required. See Section 6 for complete specifications.

**Components:**
1. **Database:**
   - `homunculus` table (character_id FK, type, level, exp, stats, skill_points, skills, intimacy, hunger, evolved, stat_bonuses)
   - Homunculus creation (random type + stats)
   - Stat growth on level-up (random per growth table)

2. **Server:**
   - Homunculus entity management (spawn, move, attack, die)
   - EXP distribution (90% player / 10% homunculus)
   - Combat AI (attack target, use skills, follow owner)
   - Intimacy/hunger system (feeding, starvation, abandonment)
   - Homunculus skill execution (per-type skills)
   - Evolution logic (Stone of Sage, stat bonuses, skill unlock)
   - Socket events: `homunculus:spawn`, `homunculus:move`, `homunculus:attack`, `homunculus:die`, `homunculus:stats`, `homunculus:feed`, `homunculus:evolve`, `homunculus:skill_use`

3. **Client:**
   - Homunculus actor (similar to other player/enemy actors)
   - Homunculus info panel (stats, skills, intimacy, hunger)
   - Homunculus command UI (attack, standby, move)
   - Homunculus skill bar
   - Evolution animation/effect

**Estimated Effort:** Very Large (entire companion AI + DB + client)

### 9.9 Equipment Break/Strip System (Priority: LOW)

**Required for:** Acid Terror armor break, Demonstration weapon break, Chemical Protection

**Components:**
1. Break state per equipment slot (weapon, armor, shield, helm)
2. Broken equipment: reduced to 0 DEF/ATK (still equipped but non-functional)
3. Repair mechanism (Repairman NPC, Repair skill)
4. Break immunity (Chemical Protection buffs)
5. Break visual indicator (client)

**Estimated Effort:** Medium

---

## 10. Implementation Priority

### Phase 1: Skill Definition Fixes (No new systems, ~1 hour)

Fix all 8 skill definitions with wrong values (see Section 8). This enables correct data display in skill tree UI without any handler work.

| Task | Skills Affected |
|------|----------------|
| Fix Acid Terror prereq, range, castTime, cooldown->afterCastDelay | 1801 |
| Fix Demonstration range, castTime, cooldown->afterCastDelay | 1802 |
| Fix Summon Flora SP, castTime, range, prereq, cooldown->afterCastDelay | 1803 |
| Fix Potion Pitcher effectValue, range, castTime, cooldown->afterCastDelay | 1806 |
| Fix Summon Marine Sphere prereq, castTime, range, cooldown->afterCastDelay | 1807 |
| Fix Pharmacy cooldown | 1800 |
| Fix Resurrect Homunculus SP formula, castTime, cooldown | 1815 |
| Add afterCastDelay to all 4 Chemical Protections | 1808-1811 |

### Phase 2: Core Combat Skills (Acid Terror + Demonstration)

These are the Alchemist's primary damage skills. Requires:
1. Catalyst consumption system (shared infrastructure)
2. Force-hit + ignore-hard-DEF flags in damage calculation
3. Acid Terror handler (ranged physical, armor break, bleeding)
4. Demonstration handler (ground zone DoT system)

### Phase 3: Potion Crafting (Pharmacy System)

The Alchemist's core identity feature:
1. Recipe data table
2. Pharmacy handler + success rate calculator
3. Client UI for recipe selection
4. This enables self-sufficiency (craft own catalysts)

### Phase 4: Support Skills (Potion Pitcher + Chemical Protection)

1. Potion Pitcher handler (ally healing)
2. Chemical Protection handlers (4 skills, buff application)
3. Equipment break/strip system (partial -- CP is only meaningful with break/strip)

### Phase 5: Summon Skills (Summon Flora + Marine Sphere)

1. Summoned entity system
2. Plant AI (auto-attack)
3. Marine Sphere AI (self-destruct)
4. Duration management

### Phase 6: Homunculus System

Major feature requiring dedicated development phase:
1. Database schema
2. Homunculus entity management
3. Combat AI
4. Intimacy/hunger system
5. 4 types with unique skills
6. Evolution system
7. Client UI and actors

---

## 11. Integration Points

### 11.1 With Existing Systems

| System | Integration |
|--------|------------|
| Damage Formulas (`ro_damage_formulas.js`) | Add `forceHit` and `ignoreHardDef` options to `calculateSkillDamage()` |
| Buff System (`ro_buff_system.js`) | Chemical Protection buffs (4 slot types), Bleeding status effect |
| Inventory System | Catalyst consumption (check + remove items), Potion crafting (add crafted items) |
| Weight System | Crafted items add weight, catalyst consumption reduces weight |
| Skill Handler (`index.js`) | New `executeCastComplete()` cases for all 17 skills |
| Item Database | New items: Creation Guides, Medicine Bowl, catalysts, crafted potions |
| Enemy System | Summon Flora/Marine Sphere entities (pseudo-enemies controlled by player) |
| EXP System | Homunculus EXP distribution (90/10 split) |
| Socket Event Router | New events for Pharmacy, Homunculus, and summon entities |

### 11.2 With Merchant Parent Skills

| Merchant Skill | Alchemist Interaction |
|---------------|----------------------|
| Enlarge Weight Limit | Alchemist carries many catalysts (heavy); critical for crafting builds |
| Pushcart | Cart storage for bulk catalyst ingredients |
| Mammonite | Still usable as melee skill |
| Cart Revolution | Still usable with cart |
| Discount | Buy crafting ingredients cheaper |
| Overcharge | Sell potions for more |

### 11.3 With Biochemist (Transcendent Class -- Future)

| Biochemist Skill | Notes |
|-----------------|-------|
| Acid Demonstration / Acid Bomb | Hybrid ATK+MATK, consumes Acid Bottle + Bottle Grenade, ignores DEF. 10 levels. Unique formula. |
| Slim Potion Pitcher | AoE version of Potion Pitcher, uses Condensed Potions, 7x7 area |
| Full Chemical Protection | Casts all 4 CP at once |
| Berserk Pitcher (Soul Linked) | Throw Berserk Potion at any class |
| Twilight Pharmacy I/II/III (Soul Linked) | Mass production of potions |

---

## 12. Data Tables Required

### 12.1 POTION_RECIPES

```js
const POTION_RECIPES = {
    // itemId: { ingredients: [{itemId, qty}], guide: guideItemId, difficultyMod: number }
    501: { // Red Potion
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 507, qty: 1 }], // Empty Potion Bottle + Red Herb
        guide: 7143, // Potion Creation Guide
        difficultyMod: 20 // +20% success
    },
    502: { // Yellow Potion
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 508, qty: 1 }],
        guide: 7143,
        difficultyMod: 20
    },
    504: { // White Potion
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 509, qty: 1 }],
        guide: 7143,
        difficultyMod: 15
    },
    505: { // Blue Potion
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 510, qty: 1 }, { itemId: 911, qty: 1 }],
        guide: 7143,
        difficultyMod: -5
    },
    7135: { // Alcohol
        ingredients: [{ itemId: 7134, qty: 1 }, { itemId: 713, qty: 1 }, { itemId: 905, qty: 5 }, { itemId: 7033, qty: 5 }],
        guide: 7143,
        difficultyMod: 10
    },
    7136: { // Acid Bottle
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 929, qty: 1 }],
        guide: 7143,
        difficultyMod: 0
    },
    7137: { // Plant Bottle
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 1033, qty: 2 }],
        guide: 7143,
        difficultyMod: 0
    },
    7138: { // Marine Sphere Bottle
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 1049, qty: 1 }, { itemId: 1051, qty: 1 }],
        guide: 7143,
        difficultyMod: 0
    },
    7139: { // Bottle Grenade
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 1059, qty: 1 }, { itemId: 7135, qty: 1 }],
        guide: 7143,
        difficultyMod: 0
    },
    7140: { // Glistening Coat
        ingredients: [{ itemId: 713, qty: 1 }, { itemId: 950, qty: 1 }, { itemId: 912, qty: 1 }, { itemId: 7135, qty: 1 }],
        guide: 7143,
        difficultyMod: -10
    },
    // Condensed Potions (separate guide)
    // Elemental Potions (separate guide)
    // Embryo (requires Bioethics)
    7142: { // Embryo
        ingredients: [{ itemId: 7134, qty: 1 }, { itemId: 7133, qty: 1 }, { itemId: 7132, qty: 1 }, { itemId: 7131, qty: 1 }],
        guide: 7143,
        difficultyMod: -5,
        requiresSkill: 'bioethics'
    }
};
```

### 12.2 HOMUNCULUS_BASE_STATS

```js
const HOMUNCULUS_BASE_STATS = {
    lif: {
        race: 'demihuman', element: 'neutral', food: 537, // Pet Food
        baseStats: { hp: 150, sp: 40, str: 12, agi: 20, vit: 15, int: 35, dex: 24, luk: 12 },
        growth: { hp: 80, sp: 6.5, str: 0.67, agi: 0.67, vit: 0.67, int: 0.71, dex: 0.80, luk: 0.80 }
    },
    amistr: {
        race: 'brute', element: 'neutral', food: 912, // Zargon
        baseStats: { hp: 320, sp: 10, str: 20, agi: 17, vit: 35, int: 11, dex: 24, luk: 12 },
        growth: { hp: 105, sp: 2.5, str: 0.92, agi: 0.71, vit: 0.71, int: 0.10, dex: 0.59, luk: 0.59 }
    },
    filir: {
        race: 'brute', element: 'neutral', food: 910, // Garlet
        baseStats: { hp: 90, sp: 25, str: 29, agi: 35, vit: 9, int: 8, dex: 30, luk: 9 },
        growth: { hp: 60, sp: 4.5, str: 0.71, agi: 0.92, vit: 0.10, int: 0.59, dex: 0.71, luk: 0.59 }
    },
    vanilmirth: {
        race: 'formless', element: 'neutral', food: 911, // Scell
        baseStats: { hp: 80, sp: 11, str: 11, agi: 11, vit: 11, int: 11, dex: 11, luk: 11 },
        growth: { hp: 90, sp: 3.5, str: 1.1, agi: 1.1, vit: 1.1, int: 1.1, dex: 1.1, luk: 1.1 }
    }
};
```

### 12.3 HOMUNCULUS_STAT_GROWTH_PROBABILITIES

```js
// Per level-up, for each stat, roll against these probabilities
const HOMUNCULUS_GROWTH_PROBS = {
    filir: {
        str: [{ bonus: 0, chance: 35.29 }, { bonus: 1, chance: 58.82 }, { bonus: 2, chance: 5.88 }],
        agi: [{ bonus: 0, chance: 15.38 }, { bonus: 1, chance: 76.92 }, { bonus: 2, chance: 7.69 }],
        vit: [{ bonus: 0, chance: 90.00 }, { bonus: 1, chance: 10.00 }],
        int: [{ bonus: 0, chance: 41.17 }, { bonus: 1, chance: 58.82 }],
        dex: [{ bonus: 0, chance: 35.29 }, { bonus: 1, chance: 58.82 }, { bonus: 2, chance: 5.88 }],
        luk: [{ bonus: 0, chance: 41.17 }, { bonus: 1, chance: 58.82 }]
    },
    lif: {
        // Most stats: ~66% chance for +1, ~6% for +2
        str: [{ bonus: 0, chance: 33.33 }, { bonus: 1, chance: 60.00 }, { bonus: 2, chance: 6.67 }],
        agi: [{ bonus: 0, chance: 33.33 }, { bonus: 1, chance: 60.00 }, { bonus: 2, chance: 6.67 }],
        vit: [{ bonus: 0, chance: 33.33 }, { bonus: 1, chance: 60.00 }, { bonus: 2, chance: 6.67 }],
        int: [{ bonus: 0, chance: 35.29 }, { bonus: 1, chance: 58.82 }, { bonus: 2, chance: 5.88 }],
        dex: [{ bonus: 0, chance: 26.67 }, { bonus: 1, chance: 66.67 }, { bonus: 2, chance: 6.67 }],
        luk: [{ bonus: 0, chance: 26.67 }, { bonus: 1, chance: 66.67 }, { bonus: 2, chance: 6.67 }]
    },
    amistr: {
        str: [{ bonus: 0, chance: 15.38 }, { bonus: 1, chance: 76.92 }, { bonus: 2, chance: 7.69 }],
        agi: [{ bonus: 0, chance: 35.29 }, { bonus: 1, chance: 58.82 }, { bonus: 2, chance: 5.88 }],
        vit: [{ bonus: 0, chance: 35.29 }, { bonus: 1, chance: 58.82 }, { bonus: 2, chance: 5.88 }],
        int: [{ bonus: 0, chance: 90.00 }, { bonus: 1, chance: 10.00 }],
        dex: [{ bonus: 0, chance: 41.17 }, { bonus: 1, chance: 58.82 }],
        luk: [{ bonus: 0, chance: 41.17 }, { bonus: 1, chance: 58.82 }]
    },
    vanilmirth: {
        // All stats identical
        str: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }],
        agi: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }],
        vit: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }],
        int: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }],
        dex: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }],
        luk: [{ bonus: 0, chance: 30.00 }, { bonus: 1, chance: 33.33 }, { bonus: 2, chance: 33.33 }, { bonus: 3, chance: 3.33 }]
    }
};
```

### 12.4 SUMMON_FLORA_DATA

```js
const SUMMON_FLORA_DATA = {
    1: { monster: 'mandragora', maxCount: 5, duration: 300000, hpBase: 2430, atkMin: 26, atkMax: 35 },
    2: { monster: 'hydra',      maxCount: 4, duration: 240000, hpBase: 2630, atkMin: 22, atkMax: 28 },
    3: { monster: 'flora',      maxCount: 3, duration: 180000, hpBase: 2830, atkMin: 242, atkMax: 273 },
    4: { monster: 'parasite',   maxCount: 2, duration: 120000, hpBase: 3030, atkMin: 215, atkMax: 430 },
    5: { monster: 'geographer', maxCount: 1, duration:  60000, hpBase: 3230, atkMin: 467, atkMax: 621, heals: true, healInterval: 5000, healAmount: 875 }
};
```

### 12.5 ACID_TERROR_STATUS_CHANCES

```js
const ACID_TERROR_STATUS = {
    1: { armorBreak: 3, bleeding: 3 },
    2: { armorBreak: 7, bleeding: 6 },
    3: { armorBreak: 10, bleeding: 9 },
    4: { armorBreak: 12, bleeding: 12 },
    5: { armorBreak: 13, bleeding: 15 }
};
```

### 12.6 DEMONSTRATION_DATA

```js
const DEMONSTRATION_DATA = {
    1: { atkPercent: 120, duration: 40000, weaponBreak: 1 },
    2: { atkPercent: 140, duration: 45000, weaponBreak: 2 },
    3: { atkPercent: 160, duration: 50000, weaponBreak: 3 },
    4: { atkPercent: 180, duration: 55000, weaponBreak: 4 },
    5: { atkPercent: 200, duration: 60000, weaponBreak: 5 }
};
// Tick interval: 500ms for all levels
```

### 12.7 POTION_BASE_HEAL_VALUES

```js
const POTION_BASE_HEAL = {
    // itemId: { minHeal, maxHeal, type: 'hp' or 'sp' }
    501: { min: 45, max: 65, type: 'hp' },   // Red Potion
    502: { min: 105, max: 145, type: 'hp' },  // Orange Potion
    503: { min: 175, max: 235, type: 'hp' },  // Yellow Potion
    504: { min: 325, max: 405, type: 'hp' },  // White Potion
    505: { min: 40, max: 60, type: 'sp' }     // Blue Potion
};
```

---

## Sources

### Primary
- [iRO Wiki - Alchemist](https://irowiki.org/wiki/Alchemist)
- [iRO Wiki Classic - Alchemist](https://irowiki.org/classic/Alchemist)
- [RateMyServer - Alchemist Skills](https://ratemyserver.net/skill_db.php?jid=18)
- [iRO Wiki - Homunculus System](https://irowiki.org/wiki/Homunculus_System)
- [iRO Wiki - Potion Creation](https://irowiki.org/wiki/Potion_Creation)
- RagnaCloneDocs/12_Pets_Homunculus_Companions.md (project internal reference)

### Per-Skill
- [Acid Terror](https://irowiki.org/wiki/Acid_Terror) + [Classic](https://irowiki.org/classic/Acid_Terror)
- [Demonstration/Bomb](https://irowiki.org/wiki/Demonstration) + [Classic](https://irowiki.org/classic/Bomb)
- [Potion Pitcher/Aid Potion](https://irowiki.org/wiki/Aid_Potion) + [Classic](https://irowiki.org/classic/Aid_Potion)
- [Bio Cannibalize/Summon Flora](https://irowiki.org/wiki/Bio_Cannibalize)
- [Sphere Mine/Summon Marine Sphere](https://irowiki.org/wiki/Sphere_Mine)
- [Chemical Protection Helm](https://irowiki.org/wiki/Biochemical_Helm)
- [Chemical Protection Shield](https://irowiki.org/wiki/Synthesized_Shield)
- [Chemical Protection Armor](https://irowiki.org/wiki/Synthetic_Armor)
- [Chemical Protection Weapon](https://irowiki.org/wiki/Alchemical_Weapon)
- [Pharmacy/Prepare Potion](https://irowiki.org/wiki/Pharmacy)
- [Potion Research](https://irowiki.org/wiki/Potion_Research)
- [Axe Mastery](https://irowiki.org/wiki/Axe_Mastery)
- [Bioethics](https://irowiki.org/wiki/Bioethics)
- [Call Homunculus](https://irowiki.org/wiki/Call_Homunculus)
- [Rest/Vaporize](https://irowiki.org/wiki/Rest_(Alchemist))
- [Resurrect Homunculus](https://irowiki.org/wiki/Resurrect_Homunculus)

### Homunculus Types
- [Lif](https://irowiki.org/wiki/Lif)
- [Amistr](https://irowiki.org/wiki/Amistr)
- [Filir](https://irowiki.org/wiki/Filir)
- [Vanilmirth](https://irowiki.org/wiki/Vanilmirth)

### Biochemist (Future Reference)
- [Acid Demonstration/Acid Bomb](https://irowiki.org/wiki/Acid_Demonstration)
- [Slim Potion Pitcher](https://irowiki.org/wiki/Slim_Potion_Pitcher)
