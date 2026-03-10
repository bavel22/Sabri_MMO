# 12. Pets, Homunculus, and Companion Systems

> Comprehensive reference for implementing RO Classic (pre-Renewal) companion systems in Sabri_MMO.
> All formulas, stats, and mechanics sourced from iRO Wiki Classic, iRO Wiki, and RateMyServer.

---

## Table of Contents

1. [Pet System](#1-pet-system)
2. [Homunculus System](#2-homunculus-system)
3. [Mercenary System](#3-mercenary-system)
4. [Falcon System](#4-falcon-system)
5. [Cart System](#5-cart-system)
6. [Mount System](#6-mount-system)
7. [Implementation Notes](#7-implementation-notes)

---

## 1. Pet System

### 1.1 Overview

The Cute Pet System allows players to tame monsters and keep them as companions. Pets follow the player, provide stat bonuses at high intimacy, can wear accessories, and respond to commands. Pets exist as egg items in inventory when not summoned.

### 1.2 Capturing Pets

**Requirements:**
- A **Taming Item** specific to the target monster
- A **Pet Incubator** (purchased from Pet NPCs in major towns, ~3,000z)

**Capture Process:**
1. Player targets a monster visible on screen
2. Double-click the taming item in inventory
3. A slot-machine mini-game appears on screen
4. If the slot machine lands on "Success", the monster is captured as a Pet Egg item
5. The taming item is consumed regardless of success or failure

**Taming Success Rate Formula:**

```
CaptureChance = BaseRate * (2.0 - (CurrentHP / MaxHP))
```

- `BaseRate` varies per pet (typically 2%-20%)
- At 100% HP: `BaseRate * 1.0` (lowest chance)
- At 50% HP: `BaseRate * 1.5`
- At 1% HP: `BaseRate * ~1.99` (approximately 2x chance, confirmed ~1.67x multiplicative)

**Refined formula (from rAthena source):**

```
CaptureChance = (BaseRate + (PlayerBaseLv - MonsterLv) * 30 + LUK * 20) * (200 - MonsterHPPercent) / 100
```

> **Implementation Note:** The slot machine is purely visual. The success/fail outcome is determined server-side at the moment the taming item is used. The client just displays a slot machine animation for drama.

### 1.3 Pet Egg System

- Captured pets become **Pet Egg** items in inventory
- Each egg is a unique item instance (stores pet data: name, intimacy, hunger, accessory)
- Use a **Pet Incubator** on a Pet Egg to hatch/summon the pet
- Only ONE pet can be active at a time
- Returning a pet stores it back as an egg (preserving all state)
- Pet eggs are tradeable (but intimacy resets on trade in most implementations)
- Pet eggs are NOT stackable (each is unique)

### 1.4 Hunger System

**Hunger Range:** 0-100

| Status | Range | Effect |
|--------|-------|--------|
| Stuffed | 91-100 | Overfed -- intimacy decreases |
| Satisfied | 76-90 | Well fed |
| Neutral | 26-75 | Normal state |
| Hungry | 11-25 | Needs feeding -- optimal feed timing |
| Very Hungry | 0-10 | Starving -- intimacy drops rapidly |

**Hunger Mechanics:**
- Each feeding increases hunger by **20 points**
- Hunger decreases passively over time (rate varies per pet species, typically 1-8 points per minute)
- If hunger reaches 0 (starving), intimacy decreases by **20 every 20 seconds**
- Overfeeding triggers escalating emotes:
  - First overfeed: `/ok` emote (normal)
  - Second overfeed: `/hmm` or `/pif` emote (warning)
  - Third overfeed: `/omg` emote -- **pet runs away permanently**

**Feeding Rules:**
- Only feed ONE item at a time
- Each pet has a **specific food item** (not interchangeable)
- "Pet Food" (generic item) is different from monster-specific food despite identical icons

### 1.5 Intimacy System

**Intimacy Range:** 0-1000

| Status | Range | Behavior |
|--------|-------|----------|
| Awkward | 0-99 | Ready to abandon owner; may flee |
| Shy | 100-249 | Uses unhappy emotes |
| Neutral | 250-749 | Uses friendly emotes |
| Cordial | 750-909 | Uses affectionate emotes; **stat bonuses active** |
| Loyal | 910-1000 | Talks often; **full stat bonuses active** |

**Initial Intimacy:** Random between **100 and 399** when first hatched (higher-level monsters start lower).

**Intimacy Gain:**
- Feeding at **Hungry** (11-25): Maximum intimacy gain per feeding
- Feeding at **Neutral** (26-75): Moderate intimacy gain, but faster overall due to shorter wait
- Feeding at **Satisfied+**: Minimal or no gain; risk of overfeeding
- Optimal strategy: Feed the moment the pet enters Hungry status

**Intimacy Loss:**
- Owner death (unevolved pet): **-20 intimacy**
- Owner death (evolved pet): **-1 intimacy**
- Overfeeding (per extra feed): **-100 intimacy**
- Starvation (hunger = 0): **-20 every 20 seconds**
- Returning pet to egg: **No intimacy loss**

### 1.6 Pet Commands

| Command | Effect |
|---------|--------|
| **Feed** | Give the pet its specific food item. Hunger +20 |
| **Performance** | Pet performs a trick/emote. Small intimacy gain at some intimacy levels |
| **Return to Egg** | Unsummon the pet, store as egg in inventory. No intimacy loss |
| **Unequip Accessory** | Remove the pet's equipped accessory |
| **Rename** | Change the pet's display name (one-time in some versions) |

### 1.7 Pet Bonuses (Cordial/Loyal)

Bonuses only activate when intimacy is **Cordial (750+)** or **Loyal (910+)**. Some pets grant partial bonuses at Cordial and full bonuses at Loyal.

### 1.8 Pet Accessories

- Each pet has a unique equippable accessory
- Accessories are obtained from monster drops, quests, or NPCs
- Accessories change the pet's visual appearance
- Some accessories grant additional bonuses beyond the base pet bonus
- Accessories stay equipped when pet is returned to egg

### 1.9 Pet Following AI

- Pets follow the owner with a simple follow behavior
- Follow distance: approximately 2-3 cells behind the owner
- If the pet gets too far away (>15 cells), it teleports to the owner
- Pets do NOT attack monsters (they are purely cosmetic companions with stat bonuses)
- Pets display idle animations when the owner is stationary
- Pets display emotes periodically based on intimacy level

### 1.10 Complete Pet List

#### Core Pets (Original / Pre-Renewal)

| Monster | Level | Taming Item | Food | Accessory | Loyal Bonus |
|---------|-------|-------------|------|-----------|-------------|
| Poring | 1 | Unripe Apple | Apple Juice | Backpack | LUK+2, CRIT+1 |
| Drops | 2 | Orange Juice | Yellow Herb | Backpack | HIT+3, ATK+3 |
| Poporing | 16 | Bitter Herb | Green Herb | Backpack | LUK+2, Poison Resist+10% |
| Lunatic | 3 | Rainbow Carrot | Carrot Juice | Silk Ribbon | CRIT+2, ATK+2 |
| Picky | 3 | Earthworm the Dude | Red Herb | Tiny Egg Shell | STR+1, ATK+5 |
| Chonchon | 7 | Rotten Fish | Pet Food | Monster Oxygen Mask | AGI+1, FLEE+2 |
| Steel Chonchon | 22 | Rusty Iron | Iron Ore | Monster Oxygen Mask | FLEE+6, AGI-1 |
| Hunter Fly | 42 | Monster Juice | Red Gemstone | Monster Oxygen Mask | FLEE-5, Perfect Dodge+2 |
| Spore | 5 | Dew Laden Moss | Pet Food | Bark Shorts | HIT+5, ATK-2 |
| Poison Spore | 19 | Deadly Noxious Herb | Pet Food | Bark Shorts | STR+1, INT+1 |
| Rocker | 15 | Singing Flower | Pet Food | Rocker Glasses | HP Recovery+5%, HP+25 |
| Savage Bebe | 10 | Sweet Milk | Pet Food | Green Lace | VIT+1, HP+50 |
| Baby Desert Wolf | 17 | Well-Dried Bone | Pet Food | Transparent Head Protector | INT+1, SP+20 |
| Smokie | 22 | Sweet Potato | Pet Food | Red Scarf | AGI+1, Perfect Dodge+1 |
| Yoyo | 29 | Tropical Banana | Banana Juice | Monkey Circlet | CRIT+3, LUK-1 |
| Orc Warrior | 36 | Orc Trophy | Pet Food | Wild Flower | ATK+10, DEF-3 |
| Munak | 33 | No Recipient | Pet Food | Punisher | INT+1, DEF+1 |
| Bongun | 32 | Her Heart | Pet Food | Grave Keeper's Sword | VIT+1, Stun Resist+1% |
| Dokebi | 68 | Old Broom | Pet Food | Wig | MATK+1%, ATK-1% |
| Sohee | 44 | Silver Knife of Chastity | Pet Food | Golden Bell | STR+1, DEX+1 |
| Isis | 47 | Armlet of Obedience | Pet Food | Queen's Hair Ornament | MATK-1%, ATK+1% |
| Peco Peco | 24 | Fat Chubby Earthworm | Pet Food | Battered Pot | HP+150, SP-10 |
| Earth Petite (Green) | 40 | Shining Stone | Pet Food | Stellar Hairpin | DEF/MDEF-2, ASPD+1% |
| Deviruchi | 56 | Contract in Shadow | Shoot | Pacifier | ATK/MATK+1%, HP/SP-3% |
| Baphomet Jr. | 57 | Book of the Devil | Honey | Skull Helm | DEF/MDEF+1, Stun Resist-1% |
| Alice | 100 | Soft Apron | White Potion | None | MDEF+1, Demi Human Resist+1% |
| Green Maiden | 80 | Tantan Noodles | Bun | None | DEF+1, Demi Human Resist+1% |
| Zealotus | 82 | Forbidden Red Candle | Immortal Heart | None | ATK+2%, MATK vs Demi Human+2% |
| Succubus | 85 | Boy's Pure Heart | Vital Flower | Black Butterfly Mask | 2% chance to drain HP on attack |
| Imp | 78 | Ice Fireworks | Flame Gemstone | Horn Barrier | Fire Resist+2%, Fire Monster Damage+1% |
| Hot Rice Cake | 35 | Chewy Rice Powder | Green Herb | None | Neutral Element Resist+1%, HP-1% |
| Wanderer | 75 | Vagabond's Skull | Spirit Liquor | None | (No bonus) |
| Santa Goblin | 36 | Sweet Candy Cane | Scell | None | HP+30, Water Resist+1% |
| New Year Doll | -- | Event Quest | Mojji | None | (Event pet) |

> **Note:** This list covers ~34 core pre-Renewal pets. Some servers add additional pets. The server implementation should store pet definitions in a data file similar to `ro_monster_templates.js`.

### 1.11 Pet Evolution (Renewal Feature -- Document for Future)

Pet evolution is a **Renewal** feature (not Classic), but documented here for future reference:

- Pets must reach **Loyal** intimacy to evolve
- Evolution requires specific items (3 taming items + crafting materials + monster card)
- Evolved pets have reduced intimacy decay (-1 on owner death vs -20)
- Evolved pets gain auto-feeding capability
- Evolved pets provide enhanced bonuses
- Evolution resets intimacy (must rebuild to Loyal again)

**Example evolution requirements:**
- Poring -> Mastering: 3 Unripe Apple, 3 Leaf of Yggdrasil, 100 Sticky Mucus, Poring Card
- Lunatic -> Leaf Lunatic: 3 Rainbow Carrot, 3 Leaf of Yggdrasil, 100 Clover, Lunatic Card

---

## 2. Homunculus System

### 2.1 Overview

The Homunculus System is exclusive to **Alchemist** and **Biochemist** (Creator) classes. It allows creating and summoning a living companion that fights alongside the player, gains experience, levels up, has its own stats and skills, and can evolve.

### 2.2 Prerequisites

- Must be Alchemist or higher (Biochemist/Creator/Geneticist)
- Must learn the **Bioethics** skill (quest-based, unlocks all homunculus skills)
- Must craft an **Embryo** item

### 2.3 Embryo Creation

**Required Materials** (purchased from Al de Baran Alchemist Guild):
| Material | Notes |
|----------|-------|
| Medicine Bowl | Consumed |
| Glass Tube | Consumed |
| Morning Dew of Yggdrasil | Consumed |
| Seed of Life | Consumed |
| Potion Creation Guide | **NOT consumed** (reusable) |

**Crafting Method:**
- Use the **Prepare Potion** skill with all materials in inventory
- Success rate depends on creator's **INT**, **DEX**, and **LUK**
- Failed attempts consume materials

### 2.4 Summoning

**Call Homunculus** skill:
- SP Cost: **10**
- Prerequisite: Bioethics Lv 1
- First use requires an Embryo item (consumed)
- Subsequent uses do NOT require an Embryo (the homunculus is permanently bound)
- 25% chance to get each of the 4 types; 50% chance for each sprite variant within a type

### 2.5 Homunculus Management Skills

| Skill | SP Cost | Effect |
|-------|---------|--------|
| Call Homunculus | 10 | Summon the homunculus |
| Vaporize | 50 | Store homunculus (requires HP >= 80% of MaxHP) |
| Homunculus Resurrection | 74 | Revive dead homunculus at (Level * 20)% HP (Lv5 = 99% HP) |

**Homunculus Resurrection by Level:**

| Level | HP Restored |
|-------|-------------|
| 1 | 20% |
| 2 | 40% |
| 3 | 60% |
| 4 | 80% |
| 5 | 99% |

### 2.6 Homunculus Death

- When the homunculus dies, it vanishes from the field
- It can be revived with **Homunculus Resurrection** skill
- Homunculus death does **NOT** reduce intimacy
- Owner death does **NOT** reduce intimacy
- Vaporizing does **NOT** reduce intimacy
- Starvation (unfed for 24 hours = 432 intimacy loss) can eventually cause permanent abandonment

### 2.7 The Four Base Homunculus Types

#### 2.7.1 Lif (Support Type)

**Race:** Demi Human | **Element:** Neutral | **Food:** Pet Food

**Starting Stats:**
| Stat | Value |
|------|-------|
| HP | 150 |
| SP | 40 |
| STR | 12 |
| AGI | 20 |
| VIT | 15 |
| INT | 35 |
| DEX | 24 |
| LUK | 12 |

**Average Growth Per Level:**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +80 | +6.5 | +0.67 | +0.67 | +0.67 | +0.71 | +0.80 | +0.80 |

**Stat Ranges at Level 99 (5th-95th percentile):**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 7,790-8,170 | 650-704 | 71-85 | 79-93 | 74-88 | 95-113 | 94-111 | 82-99 |

**Skills:**

| Skill | Type | Max Lv | Description |
|-------|------|--------|-------------|
| Healing Hands | Active | 5 | Heals owner's HP. Consumes a Condensed Red Potion. Works like Acolyte's Heal |
| Urgent Escape | Active | 5 | Increases Lif's and owner's movement speed by 10-50% for 40-20 seconds |
| Brain Surgery | Passive | 5 | +1-5% Max SP, +2-10% Healing Hands effectiveness, +3-15% SP Recovery |
| Mental Charge (Evolved) | Active | 3 | Uses MATK instead of ATK for regular attacks for 1-5 minutes. +30-90 VIT, +20-60 INT |

**Healing Hands by Level:**

| Level | Heal Amount | SP Cost |
|-------|-------------|---------|
| 1 | Base heal | 13 |
| 2 | +20% heal | 16 |
| 3 | +40% heal | 19 |
| 4 | +60% heal | 22 |
| 5 | +80% heal | 25 |

**Urgent Escape by Level:**

| Level | Speed Bonus | Duration | Cooldown |
|-------|-------------|----------|----------|
| 1 | +10% | 40s | 60s |
| 2 | +20% | 35s | 70s |
| 3 | +30% | 30s | 80s |
| 4 | +40% | 25s | 90s |
| 5 | +50% | 20s | 120s |

---

#### 2.7.2 Amistr (Tank Type)

**Race:** Brute | **Element:** Neutral | **Food:** Zargon

**Starting Stats:**
| Stat | Value |
|------|-------|
| HP | 320 |
| SP | 10 |
| STR | 20 |
| AGI | 17 |
| VIT | 35 |
| INT | 11 |
| DEX | 24 |
| LUK | 12 |

**Average Growth Per Level:**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +105 | +2.5 | +0.92 | +0.71 | +0.71 | +0.10 | +0.59 | +0.59 |

**Stat Ranges at Level 99 (5th-95th percentile):**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 10,361-10,840 | 250-260 | 104-118 | 77-95 | 95-113 | 16-25 | 74-89 | 62-77 |

**Skills:**

| Skill | Type | Max Lv | Description |
|-------|------|--------|-------------|
| Castling | Active | 5 | Instantly swaps Amistr and owner's position. Success: 20/40/60/80/100% |
| Amistr Bulwark | Active | 5 | +10-30 VIT for 40-20 seconds |
| Adamantium Skin | Passive | 5 | +2-10% MaxHP, +5-25% HP Recovery, +4-20 DEF |
| Blood Lust (Evolved) | Active | 3 | +30-50% ATK, 20% lifesteal on each hit |

**Castling by Level:**

| Level | Success Rate | SP Cost |
|-------|-------------|---------|
| 1 | 20% | 10 |
| 2 | 40% | 10 |
| 3 | 60% | 10 |
| 4 | 80% | 10 |
| 5 | 100% | 10 |

**Amistr Bulwark by Level:**

| Level | VIT Bonus | Duration |
|-------|-----------|----------|
| 1 | +10 | 40s |
| 2 | +15 | 35s |
| 3 | +20 | 30s |
| 4 | +25 | 25s |
| 5 | +30 | 20s |

**Adamantium Skin by Level:**

| Level | MaxHP | HP Recovery | DEF |
|-------|-------|-------------|-----|
| 1 | +2% | +5% | +4 |
| 2 | +4% | +10% | +8 |
| 3 | +6% | +15% | +12 |
| 4 | +8% | +20% | +16 |
| 5 | +10% | +25% | +20 |

---

#### 2.7.3 Filir (Speed/Attack Type)

**Race:** Brute | **Element:** Neutral | **Food:** Garlet

**Starting Stats:**
| Stat | Value |
|------|-------|
| HP | 90 |
| SP | 25 |
| STR | 29 |
| AGI | 35 |
| VIT | 9 |
| INT | 8 |
| DEX | 30 |
| LUK | 9 |

**Average Growth Per Level:**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +60 | +4.5 | +0.71 | +0.92 | +0.10 | +0.59 | +0.71 | +0.59 |

**Stat Growth Probability (Per Level-Up):**

| Stat | +0 Chance | +1 Chance | +2 Chance |
|------|-----------|-----------|-----------|
| STR | 35.29% | 58.82% | 5.88% |
| AGI | 15.38% | 76.92% | 7.69% |
| VIT | 90.00% | 10.00% | 0% |
| INT | 41.17% | 58.82% | 0% |
| DEX | 35.29% | 58.82% | 5.88% |
| LUK | 41.17% | 58.82% | 0% |

**Stat Ranges at Level 99 (5th-95th percentile):**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 5,820-6,110 | 461-471 | 89-107 | 119-133 | 14-23 | 58-73 | 90-108 | 59-74 |

**Skills:**

| Skill | Type | Max Lv | Description |
|-------|------|--------|-------------|
| Moonlight | Offensive | 5 | Pecks target: 220-660% ATK damage, 1-3 hits |
| Flitting (Fleeting Move) | Active | 5 | +3-15% ASPD, +10-30% ATK for 60-40 seconds |
| Accelerated Flight | Active | 5 | +20-60 FLEE for 60-40 seconds |
| S.B.R.44 (Evolved) | Offensive | 3 | Deals Intimacy * 100-300 damage. Drops intimacy to 2. Requires >= 3 intimacy |

**Moonlight by Level:**

| Level | Hits | Damage |
|-------|------|--------|
| 1 | 1 | 220% |
| 2 | 2 | 330% |
| 3 | 2 | 440% |
| 4 | 2 | 550% |
| 5 | 3 | 660% |

**Flitting by Level:**

| Level | ASPD Bonus | ATK Bonus | Duration | Cooldown |
|-------|------------|-----------|----------|----------|
| 1 | +3% | +10% | 60s | 60s |
| 2 | +6% | +15% | 55s | 70s |
| 3 | +9% | +20% | 50s | 80s |
| 4 | +12% | +25% | 45s | 90s |
| 5 | +15% | +30% | 40s | 120s |

**Accelerated Flight by Level:**

| Level | FLEE Bonus | Duration | Cooldown |
|-------|------------|----------|----------|
| 1 | +20 | 60s | 60s |
| 2 | +30 | 55s | 70s |
| 3 | +40 | 50s | 80s |
| 4 | +50 | 45s | 90s |
| 5 | +60 | 40s | 120s |

---

#### 2.7.4 Vanilmirth (Magic Type)

**Race:** Formless | **Element:** Neutral | **Food:** Scell

**Starting Stats:**
| Stat | Value |
|------|-------|
| HP | 80 |
| SP | 11 |
| STR | 11 |
| AGI | 11 |
| VIT | 11 |
| INT | 11 |
| DEX | 11 |
| LUK | 11 |

**Average Growth Per Level:**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| +90 | +3.5 | +1.1 | +1.1 | +1.1 | +1.1 | +1.1 | +1.1 |

**Stat Growth Probability (Per Level-Up -- identical for all 6 stats):**

| Bonus | Chance |
|-------|--------|
| +0 | 30.00% |
| +1 | 33.33% |
| +2 | 33.33% |
| +3 | 3.33% |

**Stat Ranges at Level 99 (5th-95th percentile):**
| HP | SP | STR | AGI | VIT | INT | DEX | LUK |
|----|----|----|-----|-----|-----|-----|-----|
| 8,321-9,449 | 335-373 | 105-133 | 105-133 | 105-133 | 105-133 | 105-133 | 105-133 |

**Skills:**

| Skill | Type | Max Lv | Description |
|-------|------|--------|-------------|
| Caprice | Offensive | 5 | Randomly casts Lv1-5 Fire Bolt, Cold Bolt, Lightning Bolt, or Earth Spike on target |
| Chaotic Blessings | Supportive | 5 | Randomly casts Lv1-5 Heal on random target (enemy, Vanilmirth, or owner) |
| Instruction Change | Passive | 5 | +1-5 INT, +1-4 STR, +1-5% Potion Creation rate |
| Self-Destruction (Evolved) | Offensive | 3 | Deals MaxHP * 1-2 piercing damage in 9x9 AoE. Drops intimacy to 1. Requires >= 450 intimacy |

**Caprice by Level:**

| Level | SP Cost | Bolt Level Range |
|-------|---------|-----------------|
| 1 | 22 | Lv 1 random bolt |
| 2 | 24 | Lv 1-2 random bolt |
| 3 | 26 | Lv 1-3 random bolt |
| 4 | 28 | Lv 1-4 random bolt |
| 5 | 30 | Lv 1-5 random bolt |

**Instruction Change by Level:**

| Level | INT | STR | Potion Rate |
|-------|-----|-----|-------------|
| 1 | +1 | +1 | +1% |
| 2 | +2 | +1 | +2% |
| 3 | +3 | +2 | +3% |
| 4 | +4 | +3 | +4% |
| 5 | +5 | +4 | +5% |

### 2.8 Homunculus Stat Formulas

```
ATK  = Floor((STR + DEX + LUK) / 3) + Floor(Level / 10)
MATK = Level + INT + Floor((INT + DEX + LUK) / 3) + Floor(Level / 10) * 2
HIT  = Level + DEX + 150
CRIT = Floor(LUK / 3) + 1
DEF  = (VIT + Floor(Level / 10)) * 2 + Floor((AGI + Floor(Level / 10)) / 2) + Floor(Level / 2)
FLEE = Level + AGI + Floor(Level / 10)
ASPD = 130 (base, 1.4 second attack interval)
```

**Combat Rules:**
- Hit rate capped at 95% (5% guaranteed miss)
- Can hit Ghost-property monsters
- FLEE is NOT reduced by mob count (unlike players)
- Base attack speed: ASPD 130 (1.4 seconds per attack)

### 2.9 Homunculus Intimacy (Feeding)

**Intimacy Range:** 0-1000

| Status | Range |
|--------|-------|
| Hate with Passion | 1-3 |
| Hate | 4-10 |
| Awkward | 11-100 |
| Shy | 101-250 |
| Neutral | 251-750 |
| Cordial | 751-910 |
| Loyal | 911-1000 |

**Feeding Intimacy Gain (based on hunger percentage):**

| Hunger Range | Intimacy Change |
|--------------|-----------------|
| 1-10% | +0.5 |
| 11-25% (optimal) | +1.0 |
| 26-75% | +0.75 |
| 76-90% | -0.05 |
| 91-100% | -0.5 |
| 0% (starving) | -1.0 per tick |

**Starvation Penalty:**
- Loses **18 intimacy per hour** of starvation
- After 24 hours of starvation: **432 intimacy lost** -- homunculus is **permanently abandoned**

### 2.10 Homunculus Evolution

**Requirements:**
- Intimacy must be **Loyal** (911+)
- Must have a **Stone of Sage** item
- Double-click Stone of Sage to evolve
- No level requirement

**Evolution Effects:**
- Random bonus of **+1 to +10** for each stat (STR/AGI/VIT/INT/DEX/LUK)
- Increased Max HP and Max SP
- New visual sprite (2 variants per evolved type)
- Unlocks the **4th skill** (ultimate skill)
- **Intimacy resets to 10** (Hate status) -- must rebuild to Loyal again

**Evolved Form Names:**

| Base Form | Evolved Form |
|-----------|-------------|
| Lif | H-Lif (Eira) |
| Amistr | H-Amistr |
| Filir | H-Filir |
| Vanilmirth | H-Vanilmirth |

**Evolved Skill Intimacy Requirements:**
- Lif (Mental Charge): Requires **Awkward** (50+ intimacy)
- Amistr (Blood Lust): Requires **Awkward** (50+ intimacy)
- Filir (S.B.R.44): Requires only **2 intimacy** (minimum)
- Vanilmirth (Self-Destruction): Requires **450 intimacy** (half)

### 2.11 Homunculus Experience and Leveling

**Level Cap:** 99 (Alchemist/Biochemist)

**EXP Distribution:**
- Player receives **90%** of Base/Job EXP from kills
- Homunculus receives **10%** regardless of damage contribution
- Level 99 total EXP: **203,562,540**

**Skill Points:**
- One skill point per **3 levels** (no job level equivalent)
- Total skill points at Lv99: **33**

### 2.12 Homunculus Commands

| Command | Hotkey | Effect |
|---------|--------|--------|
| Attack target | Alt + Right Click (monster) | Orders homunculus to attack specific target |
| Attack (aggressive) | Alt + Double Right Click | Attack order with priority |
| Follow / Standby toggle | Alt + T | Toggle between follow mode and stationary mode |
| Move to location | Alt + Right Click (ground) | Move to specific location (15-tile range) |
| Open info window | Alt + R | Display homunculus stats/skills |

### 2.13 Homunculus AI

Homunculi can be controlled via:
1. **Manual commands** (attack, move, standby)
2. **Custom AI scripts** (Lua-based in original RO)
3. **Default AI** (auto-attack nearby enemies, follow owner)

**Default AI Behavior:**
- Idle: Follow owner at 2-3 cell distance
- Combat: Attack owner's target
- Flee: Return to owner if HP is low (configurable)
- Use skills automatically based on cooldowns

### 2.14 Homunculus S (Mutation -- Renewal Feature, Future Reference)

After evolving to the H-form, Geneticists can further mutate their homunculus into one of 5 Homunculus S types:

| H-S Type | Specialty | Learns First Skill At |
|----------|-----------|----------------------|
| Bayeri | Defense/Support | Lv 105 |
| Sera | Status/AoE | Lv 105 |
| Dieter | Ground AoE/Fire | Lv 109 |
| Eleanor | Melee Combos | Lv 100 |
| Eira | Magic/Silence | Lv 106 |

> **Note:** Homunculus S is a Renewal/3rd class feature. Document here for future planning only.

---

## 3. Mercenary System

### 3.1 Overview

The Mercenary System allows **any player** (not class-restricted) to hire NPC fighters that battle alongside them. Unlike homunculi, mercenaries have **fixed stats**, do NOT level up, and expire after a set duration.

### 3.2 Mercenary Guilds

| Guild | Type | Location | Coordinates |
|-------|------|----------|-------------|
| Fencer Guild | Sword mercenaries | Izlude | (47, 139) |
| Bowman Guild | Bow mercenaries | Payon | (99, 167) |
| Spearman Guild | Spear mercenaries | Prontera | (41, 337) |

### 3.3 Contract System

**Purchasing Scrolls:**
- Mercenary scrolls are purchased with Zeny from guild NPCs
- Higher-level scrolls require higher base levels and loyalty points
- Each scroll summons a specific mercenary when used
- Contract duration: **30 minutes** (fixed)

**Contract Ending Conditions:**
1. Contract time expires (30 minutes)
2. Mercenary dies
3. Summoner (contractor) dies
4. Manual cancellation

**Scroll Costs:**

| Level | Base Level Req | Loyalty Req | Zeny Cost |
|-------|---------------|-------------|-----------|
| 1 | 15+ | -- | 7,000z |
| 2 | 25+ | -- | 14,000z |
| 3 | 35+ | -- | 21,000z |
| 4 | 45+ | -- | 28,000z |
| 5 | 55+ | -- | 35,000z |
| 6 | 65+ | -- | 42,000z |
| 7 | 75+ | 50 LP | 49,000z |
| 8 | 85+ | 100 LP | 56,000z |
| 9 | 90+ | 300 LP | 63,000z |
| 10 | 90+ | 500 LP | 400 LP (no zeny) |

### 3.4 Loyalty Points System

Loyalty Points (LP) are tracked **per guild** (Fencer/Bowman/Spearman separately).

**Earning LP:**
- Every **50 monster kills** by mercenary: **+1 LP**
- Completing a full 30-minute contract: **+1 LP**

**Losing LP:**
- Mercenary death: **-1 LP**

**LP Uses:**
- Unlock higher-level mercenary scrolls (Lv 7-10)
- Lv 10 scroll costs 400 LP instead of zeny
- Higher LP increases killcount bonus trigger rate

### 3.5 Killcount Stat Bonuses

Mercenaries gain temporary stat bonuses based on kills during the contract. Higher loyalty reduces the kills needed per bonus tier.

| Bonus Tier | FLEE | HIT | ATK | HP | SP |
|------------|------|-----|-----|----|----|
| 1 | +15 | +15 | +15 | +5% | +5% |
| 2 | +30 | +30 | +30 | +10% | +10% |
| 3 | +45 | +45 | +45 | +15% | +15% |
| 4 | +60 | +60 | +60 | +20% | +20% |
| 5 | +75 | +75 | +75 | +25% | +25% |

### 3.6 Fencer Mercenary Stats

| Lv | Name | HP | SP | ATK | HIT | FLEE | DEF | MDEF |
|----|------|----|----|-----|-----|------|-----|------|
| 1 | David | 502 | 70 | 174-261 | 50 | 47 | 26+5 | 0+3 |
| 2 | Ellen | 979 | 99 | 258-387 | 65 | 68 | 31+6 | 0+9 |
| 3 | Luise | 1,555 | 134 | 326-489 | 80 | 85 | 36+7 | 3+9 |
| 4 | Frank | 2,397 | 162 | 382-573 | 90 | 109 | 39+8 | 5+10 |
| 5 | Ryan | 3,387 | 195 | 406-609 | 105 | 128 | 42+9 | 7+10 |
| 6 | Paolo | 4,495 | 241 | 436-654 | 120 | 145 | 45+10 | 12+17 |
| 7 | Jens | 5,889 | 279 | 468-702 | 135 | 168 | 47+11 | 15+17 |
| 8 | Thierry | 7,520 | 325 | 500-750 | 150 | 185 | 49+12 | 18+24 |
| 9 | Steven | 9,052 | 348 | 524-786 | 155 | 190 | 51+12 | 22+27 |
| 10 | Wayne | 12,355 | 451 | 760-1,040 | 159 | 204 | 64+30 | 30+45 |

**Fencer Skills by Level:**
- Lv 1-3: Bash, Provoke
- Lv 4-6: Bash, Magnum Break, Provoke
- Lv 7-9: Bash, Bowling Bash, Parry, Shield Reflect
- Lv 10: Bash Lv10, Bowling Bash Lv10, Frenzy Lv1

### 3.7 Bowman Mercenary Stats

| Lv | Name | HP | SP | ATK | HIT | FLEE | DEF | MDEF |
|----|------|----|----|-----|-----|------|-----|------|
| 1 | Mina | 256 | 200 | 170-255 | 48 | 36 | 7+5 | 5+3 |
| 2 | Dororu | 457 | 70 | 228-342 | 70 | 48 | 11+8 | 7+5 |
| 3 | Nami | 732 | 93 | 260-390 | 92 | 61 | 15+12 | 9+10 |
| 4 | Elfin | 1,092 | 116 | 310-465 | 110 | 83 | 18+17 | 11+14 |
| 5 | Clara | 2,212 | 280 | 360-540 | 143 | 101 | 20+17 | 13+20 |
| 6 | Dali | 3,098 | 353 | 424-636 | 153 | 116 | 21+24 | 15+33 |
| 7 | Karaya | 4,051 | 415 | 468-702 | 171 | 135 | 24+27 | 16+39 |
| 8 | Hiyori | 5,039 | 469 | 481-723 | 193 | 155 | 24+27 | 18+40 |
| 9 | Kero | 5,572 | 499 | 500-750 | 205 | 166 | 25+27 | 20+41 |
| 10 | Sukye | 7,381 | 642 | 816-1,124 | 222 | 182 | 49+27 | 49+41 |

**Bowman Skills by Level:**
- Lv 1-3: Double Strafe, Arrow Shower
- Lv 4-6: Double Strafe, Arrow Shower, Skid Trap, Land Mine
- Lv 7-9: Focused Arrow Strike, Arrow Repel, Freezing Trap, Remove Trap, Sandman
- Lv 10: Arrow Repel Lv1, Berserk Lv1, Focused Arrow Strike Lv10, Weapon Quicken Lv5

### 3.8 Spearman Mercenary Stats

| Lv | Name | HP | SP | ATK | HIT | FLEE | DEF | MDEF |
|----|------|----|----|-----|-----|------|-----|------|
| 1 | Rodin | 2,071 | 100 | 168-252 | 52 | 23 | 30+4 | 1+3 |
| 2 | Lancer | 2,523 | 131 | 208-312 | 71 | 35 | 33+4 | 1+3 |
| 3 | Nathan | 3,397 | 161 | 248-372 | 81 | 42 | 36+22 | 55+12 |
| 4 | Roan | 4,580 | 191 | 300-450 | 105 | 57 | 39+35 | 5+17 |
| 5 | Orizaro | 5,899 | 221 | 350-510 | 120 | 62 | 42+39 | 10+19 |
| 6 | Thyla | 7,874 | 252 | 370-555 | 133 | 75 | 46+15 | 10+84 |
| 7 | Ben | 10,260 | 330 | 380-570 | 142 | 84 | 50+63 | 15+51 |
| 8 | Pinaka | 13,167 | 366 | 400-600 | 153 | 110 | 55+74 | 20+57 |
| 9 | Kuhlmann | 14,648 | 398 | 440-660 | 158 | 120 | 60+77 | 25+63 |
| 10 | Roux | 18,000 | 413 | 700-950 | 169 | 129 | 70+90 | 30+75 |

**Spearman Skills by Level:**
- Lv 1-3: Pierce, Provoke, Guard
- Lv 4-6: Pierce, Brandish Spear, Guard, Defending Aura
- Lv 7-9: Pierce, Clashing Spiral, Guard, Sacrifice
- Lv 10: Clashing Spiral Lv5, Guard Lv10, Sacrifice Lv3, Weapon Quicken Lv5

### 3.9 Monster Mercenaries

| Mercenary | Level | Type | Notes |
|-----------|-------|------|-------|
| Wild Rose | 38 | Special | Special contract |
| Doppelganger | 72 | Special | Rare contract |
| Egnigem Cenia | 78 | Special | Rare contract |

### 3.10 Mercenary-Only Skills

| Skill | Effect |
|-------|--------|
| Benediction | Removes Curse and Blind |
| Compress | Removes Bleeding |
| Crash | 3-hit attack with Stun chance |
| Mental Cure | Removes Hallucination and Chaos |
| Recuperate | Removes Poison and Silence |
| Regain | Removes Sleep and Stun |
| Scapegoat | Transfers remaining HP to summoner, mercenary dies |
| Tender | Removes Frozen and Stone |
| Weapon Quicken | Temporary ASPD boost |

### 3.11 Mercenary Commands

| Command | Hotkey | Effect |
|---------|--------|--------|
| Target enemy | Alt + Left Click (monster) | Order attack |
| Move | Alt + Left Click (ground) | Move to position |
| Standby | Ctrl + T | Toggle standby mode |
| Status window | Ctrl + R | Open mercenary stats |
| Custom AI | /merai | Configure AI behavior |

### 3.12 Mercenary Mechanics

- **ASPD:** Fixed at **156** regardless of type or level
- **Teleport range:** If mercenary moves **>15 cells** from summoner, teleports to summoner
- **Heal effectiveness:** Heal on mercenaries restores only **50%** of normal amount
- **Buff restrictions:** Endow skills only work from summoner, not from party members
- **Trap limit:** Maximum **10 traps** active at once
- **PvP/WoE:** Mercenaries function in both but **cannot damage the Emperium**
- **Support items:** Special mercenary potions purchased from guild NPCs

**Mercenary Support Items:**

| Item | Cost | Effect |
|------|------|--------|
| Mercenary Red Potion | 1,750z | Restores ~1,000 HP |
| Mercenary Blue Potion | 3,500z | Restores ~100 SP |
| Mercenary Concentration Potion | 560z | +10% ASPD |
| Mercenary Awakening Potion | 1,050z | +15% ASPD (Merc Lv40+ required) |
| Mercenary Berserk Potion | 2,100z | +20% ASPD (Merc Lv80+ required) |

---

## 4. Falcon System

### 4.1 Overview

Hunters and Snipers can rent a **Falcon** companion that sits on their shoulder and automatically attacks enemies. The falcon is not a separate entity with its own HP -- it is a passive companion that triggers attacks based on the player's LUK stat.

### 4.2 Obtaining a Falcon

- **Rental Location:** Hunter Guild (hu_in01 for Hunters)
- **Cost:** Free (quest-based or NPC rental)
- **Requirement:** Must be Hunter, Sniper, or Ranger class
- **Restriction:** Cannot have a falcon AND a cart at the same time (Hunters only -- this restriction does not apply to Rangers)

### 4.3 Falcon Skills

#### 4.3.1 Blitz Beat (Hunter Skill)

**Type:** Offensive | **Max Level:** 5 | **Target:** Single

**Damage Formula (per hit):**
```
DamagePerHit = (SkillLevel * 20) + (SteelCrowLv * 6) + Floor(AGI / 2) * 2 + Floor(DEX / 10) * 2
```

> **Note:** Originally INT-based; changed to AGI-based in a May 2020 update. Classic implementations should use INT.

**Classic (INT-based) Damage Formula:**
```
DamagePerHit = (SkillLevel * 20) + (SteelCrowLv * 6) + Floor(INT / 2) * 2 + Floor(DEX / 10) * 2
```

**Skill Levels:**

| Level | Hits | SP Cost | Total Damage |
|-------|------|---------|-------------|
| 1 | 1 | 10 | 1x formula |
| 2 | 2 | 13 | 2x formula |
| 3 | 3 | 16 | 3x formula |
| 4 | 4 | 19 | 4x formula |
| 5 | 5 | 22 | 5x formula |

**Auto-Cast (Passive Trigger):**

```
AutoBlitzChance = LUK / 3  (percentage)
```

- 3 LUK = 1% chance per normal attack
- 30 LUK = 10% chance
- 75 LUK = 25% chance
- 99 LUK = 33% chance

**Auto-Cast Rules:**
- Triggers on each **normal attack** (not skills)
- Triggers even on missed or blocked attacks
- No cast time, but has **1-second cast delay**
- Hit count depends on Job Level:
  - Job 1-9: max 1 hit
  - Job 10-19: max 2 hits
  - Job 20-29: max 3 hits
  - Job 30-39: max 4 hits
  - Job 40+: max 5 hits
- Auto-cast Blitz Beat always targets the monster being attacked
- Auto-cast damage is divided by number of targets in AoE (manual cast is not divided)

#### 4.3.2 Steel Crow (Hunter Passive)

**Type:** Passive | **Max Level:** 10 | **Prerequisite:** Blitz Beat Lv 5

| Level | Bonus Damage |
|-------|-------------|
| 1 | +6 |
| 2 | +12 |
| 3 | +18 |
| 4 | +24 |
| 5 | +30 |
| 6 | +36 |
| 7 | +42 |
| 8 | +48 |
| 9 | +54 |
| 10 | +60 |

#### 4.3.3 Falcon Assault (Sniper Skill)

**Type:** Offensive | **Max Level:** 5 | **Target:** Single | **Prerequisite:** Blitz Beat Lv 5, Steel Crow Lv 3

**Damage Formula:**
```
Damage = ((Floor(AGI/2)*2 + Floor(DEX/10)*2 + BlitzBeatLv*20 + SteelCrowLv*6) * SkillLevel + SteelCrowLv*6) * (SteelCrowLv/20 + SkillLevel + BaseLv/50)
```

**SP Cost:** 26 + (SkillLevel * 4)

| Level | SP Cost |
|-------|---------|
| 1 | 30 |
| 2 | 34 |
| 3 | 38 |
| 4 | 42 |
| 5 | 46 |

**Notes:**
- Single hit (not multi-hit like Blitz Beat)
- Requires a falcon
- Has cast time (variable, reducible with DEX)

### 4.4 Falcon Visibility

- The falcon is visible perched on the player's right shoulder
- When Blitz Beat triggers, the falcon flies toward the target and returns
- Other players can see the falcon on the character
- The falcon sprite changes with class advancement (Hunter vs Sniper vs Ranger)

### 4.5 Falcon Restrictions

- Cannot rent a falcon while a cart is equipped (Merchant/Blacksmith dual-class note: N/A for pure Hunters)
- Falcon cannot be used while mounted on a Peco Peco (classic; changed in Renewal)
- Only 1 falcon per character

---

## 5. Cart System

### 5.1 Overview

The Cart (Pushcart) system is exclusive to the **Merchant** job tree (Merchant, Blacksmith, Whitesmith/Mastersmith, Alchemist, Biochemist/Creator, Geneticist). It provides additional inventory storage and enables cart-based combat skills.

### 5.2 Pushcart Skill

**Type:** Passive | **Max Level:** 10 | **Prerequisite:** Enlarge Weight Limit Lv 5

**Cart Specifications:**
- **Weight Capacity:** 8,000
- **Inventory Slots:** 100
- **Hotkey:** Alt+W to open cart storage

**Movement Speed by Pushcart Level:**

| Level | Speed Penalty | Effective Speed |
|-------|--------------|-----------------|
| 1 | -45% | 55% |
| 2 | -40% | 60% |
| 3 | -35% | 65% |
| 4 | -30% | 70% |
| 5 | -25% | 75% |
| 6 | -20% | 80% |
| 7 | -15% | 85% |
| 8 | -10% | 90% |
| 9 | -5% | 95% |
| 10 | 0% | 100% (normal speed) |

### 5.3 Cart Rental

- Rented from **Kafra Employees** in most major towns
- Cost varies by Kafra location (typically ~800z)
- Super Novices can only rent from specific Al de Baran Kafra (aldebaran 54/238)
- Cart persists until manually returned or character changes class (in some implementations)

### 5.4 Cart Revolution (Merchant Skill)

**Type:** Offensive | **Max Level:** 1 | **Target:** 3x3 AoE around target | **SP Cost:** 12

**Damage Formula:**
```
Damage% = 150 + (100 * CurrentCartWeight / MaxCartWeight)
```

- Empty cart: **150%** ATK
- Full cart (8,000 weight): **250%** ATK
- Weight bonus: **1% per 80 weight units**
- Knockback: 2 cells

**Strategy:** Fill cart with cheap heavy items (Steel, Flower) to maximize damage.

### 5.5 Change Cart (Merchant Platinum Skill)

**Type:** Active | **Max Level:** 1 | **SP Cost:** 40 | **Obtained via:** Quest

Changes the cart's visual appearance based on character base level.

**Cart Models by Base Level:**

| Base Level | Cart Style |
|-----------|------------|
| 1+ | Cart 1: Plain Metal (default) |
| 41+ | Cart 2: Wooden |
| 66+ | Cart 3: Flower/Salad |
| 81+ | Cart 4: Panda |
| 91+ | Cart 5: Ice Cream |
| 101+ (Trans) | Cart 6: Umbrella |
| 111+ (Trans) | Cart 7: Adventurer |
| 121+ (Trans) | Cart 8: Hodrem |
| 131+ (Trans) | Cart 9: Imperial |

> **Note:** Carts 6-9 require transcendent/rebirth classes to reach those base levels.

### 5.6 Cart Boost (Whitesmith/Mastersmith Skill)

**Type:** Supportive | **Max Level:** 1 | **SP Cost:** 20 | **Duration:** 60 seconds

**Prerequisites:** Pushcart Lv 5, Cart Revolution Lv 1, Change Cart Lv 1, Hilt Binding Lv 1

**Effect:**
- Boosts movement speed while carrying a pushcart (approximately equivalent to Increase AGI speed)
- **Does NOT stack** with Increase AGI
- **Mutually cancels** with Decrease AGI
- Cannot be dispelled by Dispell or Slow Grace
- ASPD-based cast delay

### 5.7 High Speed Cart Ram / Cart Termination (Whitesmith Skill)

**Type:** Offensive | **Max Level:** 10 | **Target:** Single | **Prerequisite:** Cart Boost active

**Damage Formula:**
```
Damage% = Floor(CartWeight / DamageModifier) + 100
```

**Damage Modifier by Level:**

| Level | Modifier | Max Damage (8000 wt) | Stun Chance | Zeny Cost |
|-------|----------|---------------------|-------------|-----------|
| 1 | 16 | 600% | 5% | 600z |
| 2 | 14 | 671% | 10% | 700z |
| 3 | 12 | 767% | 15% | 800z |
| 4 | 10 | 900% | 20% | 900z |
| 5 | 8 | 1,100% | 25% | 1,000z |
| 6 | 7.5 | 1,167% | 30% | 1,100z |
| 7 | 7 | 1,243% | 35% | 1,200z |
| 8 | 6.5 | 1,331% | 40% | 1,300z |
| 9 | 6.25 | 1,380% | 45% | 1,400z |
| 10 | 6 | 1,433% | 50% | 1,500z |

**Notes:**
- Can ONLY be used while Cart Boost is active
- Costs Zeny per use (not SP)
- Power-Thrust and Maximum Power-Thrust stack additively
- With Lv10 HSCR + Lv5 Maximum Power-Thrust = **1,433% max damage**
- Ignores size penalties
- Cannot be reflected

### 5.8 Cart Storage Interface

- Accessed via Alt+W hotkey
- Drag items between inventory and cart
- Cart acts as secondary storage (items remain through map changes)
- Items in cart are NOT accessible for equipment or skill use (must move to inventory first)
- NPC shops can buy/sell directly from/to cart
- Vending skill uses cart inventory (not player inventory)

---

## 6. Mount System

### 6.1 Overview

The mount system allows **Knight** and **Crusader** class characters to ride Peco Peco birds, providing increased movement speed at the cost of reduced attack speed.

### 6.2 Peco Peco Ride Skill

**Type:** Passive | **Max Level:** 1

**Prerequisites:**
- Knight: Endure Lv 1
- Crusader: Faith Lv 1

**Effects:**
- Increases movement speed (approximately equal to Increase AGI effect)
- Increases weight limit by **+1,000**
- Allows Spear weapons to deal **100% damage to Medium size** enemies (normally 75%)
- Enhances Spear Mastery by **+1 ATK per skill level** of Spear Mastery

**Penalty:**
- **ASPD reduced to 50%** of normal (halved attack speed)
- This penalty is mitigated by the Cavalier Mastery skill

### 6.3 Cavalier Mastery Skill

**Type:** Passive | **Max Level:** 5 | **Prerequisite:** Peco Peco Ride Lv 1

Recovers ASPD lost from mounting:

| Level | ASPD Recovery | Effective ASPD |
|-------|-------------|---------------|
| 0 (no skill) | 0% | 50% of normal |
| 1 | +10% | 60% of normal |
| 2 | +20% | 70% of normal |
| 3 | +30% | 80% of normal |
| 4 | +40% | 90% of normal |
| 5 | +50% | 100% (normal speed) |

> **Important:** Cavalier Mastery Lv 5 fully restores normal ASPD while mounted.

### 6.4 Mount Locations

| Class | Mount Type | Rental Location | Coordinates |
|-------|-----------|----------------|-------------|
| Knight | Peco Peco | Prontera Chivalry | (55, 348) |
| Lord Knight | Peco Peco | Prontera Chivalry | (55, 348) |
| Crusader | Grand Peco | Prontera Church | (232, 318) |
| Paladin | Grand Peco | Prontera Church | (232, 318) |

### 6.5 Mount/Dismount Mechanics

- **Mounting:** Talk to the Peco Peco rental NPC
- **Dismounting:** Talk to the same NPC or use a specific command
- **Weight Warning:** Dismounting removes the +1,000 weight bonus; ensure inventory can handle the reduction
- **Free rental:** No Zeny cost in most implementations
- **Persistent:** Mount persists through map changes and relog

### 6.6 Visual Changes

- Character sprite changes significantly when mounted (taller, riding animation)
- Peco Peco (Knight) and Grand Peco (Crusader) have different sprites but identical mechanics
- Movement animation changes to riding animation
- Attack animation changes to mounted attack animation
- All equipped weapons/armor remain visible on mounted sprite

### 6.7 Skill Interactions While Mounted

| Skill | Mounted Effect |
|-------|---------------|
| Brandish Spear | Range/AoE changes when mounted |
| Spear Boomerang | Usable while mounted |
| Charge Attack | Usable while mounted |
| Grand Cross | Usable while mounted (no special interaction) |
| Bowling Bash | ASPD penalty affects usage speed |
| All melee skills | Affected by ASPD penalty unless Cavalier Mastery Lv5 |

### 6.8 Renewal Mounts (Future Reference)

| Class | Mount | Prerequisite |
|-------|-------|-------------|
| Rune Knight | Dragon | Dragon Training skill |
| Royal Guard | Gryphon | Gryphon Riding skill |

> These are Renewal features, documented for future planning.

---

## 7. Implementation Notes

### 7.1 Database Schema

#### character_pets Table

```sql
CREATE TABLE character_pets (
    pet_id          SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    monster_id      INTEGER NOT NULL,           -- Monster template ID
    pet_name        VARCHAR(24) NOT NULL,       -- Player-chosen name
    intimacy        INTEGER NOT NULL DEFAULT 250,  -- 0-1000
    hunger          INTEGER NOT NULL DEFAULT 50,   -- 0-100
    accessory_id    INTEGER DEFAULT NULL,       -- Equipped accessory item ID
    is_summoned     BOOLEAN NOT NULL DEFAULT FALSE,
    renamed         BOOLEAN NOT NULL DEFAULT FALSE,
    egg_item_id     INTEGER,                    -- Reference to egg in inventory
    created_at      TIMESTAMP DEFAULT NOW(),
    updated_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT fk_pet_character FOREIGN KEY (character_id)
        REFERENCES characters(id) ON DELETE CASCADE,
    CONSTRAINT chk_intimacy CHECK (intimacy >= 0 AND intimacy <= 1000),
    CONSTRAINT chk_hunger CHECK (hunger >= 0 AND hunger <= 100)
);

CREATE INDEX idx_pet_character ON character_pets(character_id);
CREATE INDEX idx_pet_summoned ON character_pets(character_id, is_summoned);
```

#### character_homunculus Table

```sql
CREATE TABLE character_homunculus (
    homunculus_id   SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    type            VARCHAR(20) NOT NULL,       -- 'lif', 'amistr', 'filir', 'vanilmirth'
    sprite_variant  INTEGER NOT NULL DEFAULT 1, -- 1 or 2 (visual variant)
    name            VARCHAR(24) NOT NULL,
    level           INTEGER NOT NULL DEFAULT 1,
    experience      BIGINT NOT NULL DEFAULT 0,
    intimacy        INTEGER NOT NULL DEFAULT 250,
    hunger          INTEGER NOT NULL DEFAULT 50,

    -- Stats (randomized growth per level)
    hp_current      INTEGER NOT NULL DEFAULT 0,
    hp_max          INTEGER NOT NULL DEFAULT 0,
    sp_current      INTEGER NOT NULL DEFAULT 0,
    sp_max          INTEGER NOT NULL DEFAULT 0,
    str             INTEGER NOT NULL DEFAULT 0,
    agi             INTEGER NOT NULL DEFAULT 0,
    vit             INTEGER NOT NULL DEFAULT 0,
    int_stat        INTEGER NOT NULL DEFAULT 0,  -- 'int' is reserved word
    dex             INTEGER NOT NULL DEFAULT 0,
    luk             INTEGER NOT NULL DEFAULT 0,

    -- Skill levels
    skill_1_level   INTEGER NOT NULL DEFAULT 0,  -- Type-specific skill 1
    skill_2_level   INTEGER NOT NULL DEFAULT 0,  -- Type-specific skill 2
    skill_3_level   INTEGER NOT NULL DEFAULT 0,  -- Type-specific skill 3
    skill_4_level   INTEGER NOT NULL DEFAULT 0,  -- Evolved ultimate skill
    skill_points    INTEGER NOT NULL DEFAULT 0,  -- Unspent skill points

    -- Evolution
    is_evolved      BOOLEAN NOT NULL DEFAULT FALSE,
    evolution_bonus_str INTEGER DEFAULT 0,
    evolution_bonus_agi INTEGER DEFAULT 0,
    evolution_bonus_vit INTEGER DEFAULT 0,
    evolution_bonus_int INTEGER DEFAULT 0,
    evolution_bonus_dex INTEGER DEFAULT 0,
    evolution_bonus_luk INTEGER DEFAULT 0,

    -- State
    is_alive        BOOLEAN NOT NULL DEFAULT TRUE,
    is_summoned     BOOLEAN NOT NULL DEFAULT FALSE,
    is_vaporized    BOOLEAN NOT NULL DEFAULT FALSE,

    -- Position (when summoned)
    x               FLOAT DEFAULT 0,
    y               FLOAT DEFAULT 0,
    z               FLOAT DEFAULT 0,

    created_at      TIMESTAMP DEFAULT NOW(),
    updated_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT fk_homun_character FOREIGN KEY (character_id)
        REFERENCES characters(id) ON DELETE CASCADE,
    CONSTRAINT chk_homun_type CHECK (type IN ('lif', 'amistr', 'filir', 'vanilmirth')),
    CONSTRAINT chk_homun_intimacy CHECK (intimacy >= 0 AND intimacy <= 1000),
    CONSTRAINT chk_homun_hunger CHECK (hunger >= 0 AND hunger <= 100)
);

CREATE UNIQUE INDEX idx_homun_character ON character_homunculus(character_id);
```

#### character_mercenary Table

```sql
CREATE TABLE character_mercenary (
    mercenary_id    SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    merc_type       VARCHAR(10) NOT NULL,       -- 'fencer', 'bowman', 'spearman'
    merc_level      INTEGER NOT NULL,            -- 1-10

    -- Contract state
    hp_current      INTEGER NOT NULL,
    hp_max          INTEGER NOT NULL,
    sp_current      INTEGER NOT NULL,
    sp_max          INTEGER NOT NULL,
    contract_start  TIMESTAMP NOT NULL DEFAULT NOW(),
    contract_end    TIMESTAMP NOT NULL,          -- start + 30 minutes
    kill_count      INTEGER NOT NULL DEFAULT 0,

    -- Position
    x               FLOAT DEFAULT 0,
    y               FLOAT DEFAULT 0,
    z               FLOAT DEFAULT 0,

    created_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT fk_merc_character FOREIGN KEY (character_id)
        REFERENCES characters(id) ON DELETE CASCADE,
    CONSTRAINT chk_merc_type CHECK (merc_type IN ('fencer', 'bowman', 'spearman'))
);

-- Loyalty points tracked per guild per character
CREATE TABLE character_mercenary_loyalty (
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    guild_type      VARCHAR(10) NOT NULL,       -- 'fencer', 'bowman', 'spearman'
    loyalty_points  INTEGER NOT NULL DEFAULT 0,

    PRIMARY KEY (character_id, guild_type),
    CONSTRAINT fk_loyalty_character FOREIGN KEY (character_id)
        REFERENCES characters(id) ON DELETE CASCADE
);
```

### 7.2 Server Data Files

#### ro_pet_data.js

```javascript
// Pet definitions: taming items, food, bonuses, hunger rates
const PET_DATA = {
    1002: { // Poring
        monsterId: 1002,
        name: 'Poring',
        tamingItem: 619,        // Unripe Apple
        food: 531,              // Apple Juice
        accessory: 10013,       // Backpack
        hungerRate: 3,          // Points lost per minute
        tamingChance: 20,       // Base % chance
        intimacyGainPerFeed: 5, // At optimal hunger
        bonusCordial: { luk: 1, crit: 1 },
        bonusLoyal: { luk: 2, crit: 1 }
    },
    // ... 30+ more entries
};
```

#### ro_homunculus_data.js

```javascript
// Homunculus type definitions: base stats, growth ranges, skills
const HOMUNCULUS_TYPES = {
    lif: {
        race: 'demihuman',
        element: 'neutral',
        food: 537, // Pet Food
        baseStats: { hp: 150, sp: 40, str: 12, agi: 20, vit: 15, int: 35, dex: 24, luk: 12 },
        growthAvg: { hp: 80, sp: 6.5, str: 0.67, agi: 0.67, vit: 0.67, int: 0.71, dex: 0.80, luk: 0.80 },
        skills: {
            healing_hands: { maxLv: 5, type: 'active' },
            urgent_escape: { maxLv: 5, type: 'active' },
            brain_surgery: { maxLv: 5, type: 'passive' },
            mental_charge: { maxLv: 3, type: 'active', evolved: true, intimacyReq: 50 }
        }
    },
    // amistr, filir, vanilmirth ...
};
```

### 7.3 Socket.io Events

#### Pet Events

```
pet:summon          { petId }                       -- Hatch/summon pet from egg
pet:return          { petId }                       -- Return pet to egg
pet:feed            { petId }                       -- Feed the pet its food
pet:performance     { petId }                       -- Pet performance command
pet:rename          { petId, newName }              -- Rename pet
pet:equip_accessory { petId, accessoryId }          -- Equip pet accessory
pet:unequip_accessory { petId }                     -- Remove pet accessory
pet:tame            { targetMonsterId, tamingItemId } -- Attempt taming

-- Server -> Client
pet:summoned        { petId, monsterId, name, intimacy, hunger, accessoryId, x, y, z }
pet:returned        { petId }
pet:fed             { petId, hunger, intimacy, emote }
pet:tamed           { success, petId?, eggItemId? }
pet:intimacy_changed { petId, intimacy, level }     -- Level = 'awkward'|'shy'|etc.
pet:hunger_tick     { petId, hunger }               -- Periodic hunger update
pet:fled            { petId }                       -- Pet ran away (starvation/overfeed)
pet:position        { petId, x, y, z }              -- Pet position update for other players
```

#### Homunculus Events

```
homunculus:summon    { }                             -- Call Homunculus (first time needs Embryo)
homunculus:vaporize  { }                             -- Store homunculus
homunculus:resurrect { }                             -- Revive dead homunculus
homunculus:feed      { }                             -- Feed homunculus
homunculus:command   { command, targetId?, x?, y?, z? } -- 'attack'|'move'|'standby'|'follow'
homunculus:skill     { skillId, targetId?, x?, y?, z? } -- Use homunculus skill
homunculus:evolve    { }                             -- Use Stone of Sage
homunculus:allocate_skill { skillId }                -- Spend skill point

-- Server -> Client
homunculus:summoned  { type, name, level, hp, sp, stats, skills, intimacy, evolved, x, y, z }
homunculus:vaporized { }
homunculus:died      { }
homunculus:resurrected { hp }
homunculus:fed       { hunger, intimacy }
homunculus:exp_gained { currentExp, expToNext, level }
homunculus:leveled_up { level, stats, skillPoints }
homunculus:evolved   { bonusStats, newSprite }
homunculus:skill_used { skillId, targetId?, x?, y?, z?, damage? }
homunculus:position  { x, y, z }                    -- Position sync
homunculus:hp_changed { hp, maxHp }
homunculus:sp_changed { sp, maxSp }
homunculus:intimacy_changed { intimacy, level }
homunculus:stat_update { stats }                     -- Full stat refresh
```

#### Mercenary Events

```
mercenary:summon     { scrollItemId }                -- Use mercenary scroll
mercenary:dismiss    { }                             -- Dismiss mercenary
mercenary:command    { command, targetId?, x?, y?, z? } -- 'attack'|'move'|'standby'
mercenary:skill      { skillId, targetId? }          -- Use mercenary skill
mercenary:use_item   { itemId }                      -- Use mercenary potion

-- Server -> Client
mercenary:summoned   { mercType, mercLevel, name, hp, sp, stats, skills, contractEnd, x, y, z }
mercenary:dismissed  { }
mercenary:died       { }
mercenary:expired    { }                             -- Contract expired
mercenary:hp_changed { hp, maxHp }
mercenary:sp_changed { sp, maxSp }
mercenary:skill_used { skillId, targetId?, damage? }
mercenary:position   { x, y, z }
mercenary:kill_count { kills, bonusTier }
mercenary:loyalty    { guildType, points }
```

#### Falcon Events (Integrated into Combat)

```
-- Falcon is not a separate entity; integrated into player attack system
combat:blitz_beat    { playerId, targetId, hits, totalDamage, x, y, z } -- Auto-proc notification
skill:falcon_assault { playerId, targetId, damage }                     -- Manual falcon skill
```

#### Cart Events

```
cart:open            { }                             -- Open cart storage window
cart:move_to_cart    { itemId, amount }               -- Move item from inventory to cart
cart:move_from_cart  { itemId, amount }               -- Move item from cart to inventory
cart:data            { items, currentWeight, maxWeight } -- Cart contents sync

-- Cart boost handled as a buff via existing buff system
```

#### Mount Events

```
mount:ride           { }                             -- Mount Peco Peco
mount:dismount       { }                             -- Dismount

-- Server -> Client
mount:mounted        { mountType }                   -- 'pecopeco'|'grandpeco'
mount:dismounted     { }
```

### 7.4 Server Implementation Notes

#### Pet Server Logic

```
Pet Hunger Tick (every 60 seconds per summoned pet):
  - Decrease hunger by pet-specific rate (1-8 per minute)
  - If hunger <= 0:
    - Decrease intimacy by 20 every 20 seconds
    - If intimacy <= 0: pet flees (delete permanently)
  - Broadcast pet:hunger_tick to owner

Pet Following:
  - Pets follow owner with simple offset (2-3 cells behind)
  - Update pet position when owner moves
  - If distance > 15 cells, teleport pet to owner
  - Broadcast pet:position to zone players (throttled, every 500ms)

Pet Intimacy Bonuses:
  - Check intimacy level on every change
  - If >= 750 (Cordial): apply partial bonuses to owner stats
  - If >= 910 (Loyal): apply full bonuses to owner stats
  - Recalculate owner stats when bonuses change
```

#### Homunculus Server Logic

```
Homunculus Combat Tick (integrated with existing 50ms combat loop):
  - If homunculus is summoned and has a target:
    - Check attack range and ASPD timer
    - Calculate damage using homunculus ATK formula
    - Apply damage to target
    - Grant 10% of monster EXP to homunculus
    - Grant 90% of monster EXP to owner
  - If homunculus is following: update position toward owner
  - If homunculus is on standby: hold position

Homunculus Feeding Tick (every 60 seconds):
  - Decrease hunger by type-specific rate
  - If hunger reaches 0: begin intimacy decay

Homunculus Level-Up:
  - Roll random stat gains per growth probability table
  - Award 1 skill point every 3 levels
  - Update all derived stats (ATK, MATK, HIT, DEF, FLEE, CRIT)
```

#### Mercenary Server Logic

```
Mercenary Contract Timer:
  - Set 30-minute timer on summon
  - On expiry: dismiss mercenary, award +1 loyalty point
  - On death: dismiss mercenary, deduct -1 loyalty point

Mercenary Combat:
  - Fixed ASPD of 156
  - Use mercenary's stats for damage calculation
  - Track kill count for bonus tiers
  - Apply killcount bonuses when thresholds reached

Mercenary Position:
  - Follow owner similar to pets/homunculus
  - Teleport if >15 cells away
  - Broadcast position to zone players
```

### 7.5 UE5 Client Implementation

#### Companion Actor Architecture

```
ACompanionActor (Base Class)
  |-- APetActor          (cosmetic follower, no combat)
  |-- AHomunculusActor   (combat companion, separate stats/HP bar)
  |-- AMercenaryActor    (combat companion, fixed stats)
  |-- AFalconActor       (shoulder attachment, VFX only)

All companions:
  - Follow owner with simple pathfinding (NavMesh or offset-based)
  - Interpolated movement for remote players (like OtherCharacterMovementComponent)
  - Visible to all players in the zone
  - Display name plate with companion name
  - Play idle/walk/attack animations
```

#### Companion Subsystems

```
UPetSubsystem : UWorldSubsystem
  - Manages pet summoning/return/feeding UI
  - Handles pet:* socket events
  - Spawns/despawns APetActor
  - Updates pet stat bonuses on owner

UHomunculusSubsystem : UWorldSubsystem
  - Manages homunculus summoning/commands/skills/evolution
  - Handles homunculus:* socket events
  - Spawns/despawns AHomunculusActor
  - HP/SP bars on homunculus
  - Skill cooldown tracking
  - EXP bar in homunculus info window

UMercenarySubsystem : UWorldSubsystem
  - Manages mercenary summoning/commands
  - Handles mercenary:* socket events
  - Contract timer display
  - Spawns/despawns AMercenaryActor

UFalconSubsystem : UWorldSubsystem (or integrated into SkillVFX)
  - Handles falcon VFX (shoulder perch, Blitz Beat flight animation)
  - Triggered by combat:blitz_beat events
  - No separate actor -- VFX only
```

#### Companion Visibility to Other Players

All companions must be visible to other players in the zone:

```
Spawn flow:
1. Owner summons companion -> server validates -> broadcasts to zone
2. Zone players receive companion:summoned with ownerId, type, position
3. Each client spawns the companion actor attached to the owner's player actor
4. Position updates broadcast via companion:position (throttled 500ms)

Despawn flow:
1. Owner returns/dismisses companion -> server validates -> broadcasts
2. Zone players receive companion:returned/dismissed
3. Each client despawns the companion actor
```

### 7.6 Priority Implementation Order

For Sabri_MMO, the recommended implementation order based on complexity and player value:

| Priority | System | Complexity | Player Value | Notes |
|----------|--------|-----------|-------------|-------|
| 1 | Mount System | Low | High | Visual only + speed buff + ASPD mod. No separate entity. |
| 2 | Cart System | Medium | High | Storage + cart skills. UI + inventory extension. |
| 3 | Falcon System | Low | Medium | VFX + auto-proc damage. No separate entity. |
| 4 | Pet System | Medium | High | Full companion entity + hunger/intimacy ticks + stat bonuses. |
| 5 | Mercenary System | High | Medium | Combat AI + contract timer + loyalty system. |
| 6 | Homunculus System | Very High | Medium | Full combat companion + leveling + skills + evolution. |

### 7.7 Performance Considerations

- **Pet position updates:** Throttle to 500ms for zone broadcasts (pets don't need 50ms precision)
- **Hunger/intimacy ticks:** Use a single server interval (e.g., 60s) that processes all active pets/homunculi in batch
- **Homunculus combat:** Integrate into existing combat tick loop (50ms) but only process for summoned homunculi
- **Mercenary contract timer:** Use setTimeout per contract, not a polling loop
- **Max companions per zone:** Consider limiting total visible companions to prevent performance issues (e.g., 50 pets + 20 homunculi + 20 mercenaries per zone)
- **Companion interpolation:** Use the same OtherCharacterMovementComponent pattern for smooth remote companion movement

---

## Sources

- [Pet System - iRO Wiki Classic](https://irowiki.org/classic/Pet_System)
- [Cute Pet System - iRO Wiki](https://irowiki.org/wiki/Cute_Pet_System)
- [Homunculus System - iRO Wiki](https://irowiki.org/wiki/Homunculus_System)
- [Lif - iRO Wiki](https://irowiki.org/wiki/Lif)
- [Amistr - iRO Wiki](https://irowiki.org/wiki/Amistr)
- [Filir - iRO Wiki](https://irowiki.org/wiki/Filir)
- [Vanilmirth - iRO Wiki](https://irowiki.org/wiki/Vanilmirth)
- [Mercenary System - iRO Wiki Classic](https://irowiki.org/classic/Mercenary_System)
- [Mercenary System - iRO Wiki](https://irowiki.org/wiki/Mercenary_System)
- [Blitz Beat - iRO Wiki](https://irowiki.org/wiki/Blitz_Beat)
- [Steel Crow - iRO Wiki](https://irowiki.org/wiki/Steel_Crow)
- [Falcon Assault - iRO Wiki](https://irowiki.org/wiki/Falcon_Assault)
- [Pushcart - iRO Wiki](https://irowiki.org/wiki/Pushcart)
- [Cart Revolution - iRO Wiki](https://irowiki.org/wiki/Cart_Revolution)
- [Cart Boost - iRO Wiki](https://irowiki.org/wiki/Cart_Boost)
- [High Speed Cart Ram - iRO Wiki](https://irowiki.org/wiki/High_Speed_Cart_Ram)
- [Change Cart - iRO Wiki](https://irowiki.org/wiki/Change_Cart)
- [Peco Peco Ride - iRO Wiki](https://irowiki.org/wiki/Peco_Peco_Ride)
- [Cavalier Mastery - iRO Wiki](https://irowiki.org/wiki/Cavalier_Mastery)
- [Homunculus S - iRO Wiki](https://irowiki.org/wiki/Homunculus_S)
- [Homunculus and Homunculus S Guide - ROGGH Library](https://roggh.com/homunculus-and-homunculus-s-guide/)
- [Pet Guide - RateMyServer](https://write.ratemyserver.net/ragnoark-online-how-to/pet-guides/)
- [The Big Fat Homunculus Bible - RateMyServer](https://write.ratemyserver.net/ragnoark-online-tips-and-tricks/the-big-fat-homunculus-bible/)
