# Novice & Super Novice -- Deep Research (Pre-Renewal)

> **Sources**: iRO Wiki Classic, iRO Wiki, RateMyServer (pre-re), rAthena pre-re (`skill_db.yml`, `skill_tree.yml`, `status.cpp`), StrategyWiki, JellyRO Wiki, Divine Pride, GameFAQs, RO Fandom Wiki, ROGGH Library, various private server wikis
> **Scope**: Pre-Renewal mechanics only. Renewal/Expanded Super Novice content is explicitly excluded unless noted for contrast.
> **Date**: 2026-03-22

---

## Table of Contents

1. [Novice Class](#novice-class)
2. [Super Novice](#super-novice)
3. [Implementation Checklist](#implementation-checklist)
4. [Gap Analysis](#gap-analysis)

---

## NOVICE CLASS

The Novice is the starting class for all characters in Ragnarok Online. Every player begins as a Novice and must progress through this class before job changing to a 1st class (or Super Novice). Novices are intentionally weak, serving as a tutorial phase that introduces players to the game's core systems.

---

### Starting Stats and Equipment

#### Initial Character State

| Property | Value |
|----------|-------|
| Base Level | 1 |
| Job Level | 1 |
| Base Stats | All stats start at 1 (STR/AGI/VIT/INT/DEX/LUK = 1) |
| Starting Stat Points | 48 distributable points |
| Starting Skill Points | 0 (earned 1 per job level starting at Job Level 2) |
| Starting Zeny | 0 |
| Starting HP | ~40 (formula: `35 + (1 * 5) = 40`, before VIT modifier) |
| Starting SP | ~12 (formula: `10 + (1 * 2) = 12`, before INT modifier) |
| Weight Limit | 2030 (`2000 + 1 * 30 + 0 job bonus`) |
| Walk Speed | Standard (same as all classes, no modifier) |

#### Starting Equipment

Every new Novice receives two items from the King:

| Item | ID | Type | Stats | Slots | Weight | Notes |
|------|----|------|-------|-------|--------|-------|
| Knife | 1201 | Dagger (Weapon Lv 1) | ATK 17 | 3 | 40 | Equippable by all classes |
| Cotton Shirt | 2301 | Armor | DEF 1 | 0 | 10 | Equippable by all classes |

The Knife[3] has 3 card slots, making it theoretically powerful for card builds despite its low ATK. The Cotton Shirt has 0 slots in its base form (a slotted version Cotton Shirt[1] exists as a separate item, ID 2330).

#### Training Grounds Items

Players who go through the Training Grounds receive additional free items:

| Item | Quantity | Source |
|------|----------|--------|
| Red Potion | 100 | NPC Brade in Training Grounds |
| Novice Potion | Various | Training quest rewards |
| Butterfly Wing | 1 | Training completion (return to town) |

**Novice Potion** (ID 12000): Heals 325 HP. Usable only by Novice and Super Novice classes (class-restricted consumable). Weight: 3. NPC price: 7z.

---

### Novice Skills

The Novice class has exactly 3 skills: Basic Skill (passive), First Aid (active/quest), and Play Dead (toggle/quest).

#### Basic Skill (ID 1 / rAthena NV_BASIC)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 1 (NV_BASIC) | rAthena `db/pre-re/skill_db.yml` |
| Type | Passive | All sources |
| Max Level | 9 | All sources |
| SP Cost | 0 (passive) | All sources |
| Cast Time | N/A | Passive |
| Learned Via | Skill point allocation (1 point per level) | All sources |
| Prerequisites | None | rAthena skill tree |

Basic Skill is the only skill Novices learn via skill points (not quest). Since Novices earn 9 skill points (Job Levels 2-10), all 9 points go into Basic Skill to unlock the job change gate.

##### Basic Skill Unlock Progression

| Level | Feature Unlocked | Description |
|-------|-----------------|-------------|
| Lv 1 | Trading | Enables trading items/zeny with other players |
| Lv 2 | Emotes | Enables Alt+Number emoticon system |
| Lv 3 | Sitting | Enables sitting command; sitting doubles HP/SP natural regen rate |
| Lv 4 | Chat Room | Enables creating chat rooms (Alt+C) |
| Lv 5 | Party Join | Enables accepting party invitations |
| Lv 6 | Kafra Storage | Enables using Kafra NPC storage services |
| Lv 7 | Party Create | Enables creating parties (/organize command) |
| Lv 8 | (Removed) | Originally alignment system; level still exists for progression |
| Lv 9 | Job Change | Enables job changing to any 1st class (required alongside Job Level 10) |

**Important**: Basic Skill Lv 9 is a hard requirement for job change. A Novice with Job Level 10 but only Basic Skill Lv 8 cannot change class. In practice, since Job Level 10 gives exactly 9 skill points and Basic Skill is the only skill available for point allocation, every Novice at Job Level 10 will have Basic Skill Lv 9.

#### First Aid (ID 2 / rAthena NV_FIRSTAID, ID 142)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 142 (NV_FIRSTAID) | rAthena `db/pre-re/skill_db.yml` |
| Type | Active / Supportive | All sources |
| Max Level | 1 | All sources |
| Target | Self | All sources |
| SP Cost | 3 | rAthena YAML, Divine Pride |
| HP Restored | **5 HP** (flat, not affected by healing modifiers) | All sources |
| Cast Time | 0 ms (instant) | rAthena YAML |
| After-Cast Delay | 0 ms | rAthena YAML |
| Cooldown | 0 ms | rAthena YAML |
| Element | Neutral | rAthena YAML |
| Quest Skill | Yes | rAthena `IsQuest: true` |
| Interruptible | Yes | pservero |

**Heal Formula**: Fixed 5 HP regardless of stats, level, or equipment. No scaling.

**Quest to Learn**:
- **NPC**: Nami (Prontera Inn, `prt_in` 234, 133)
- **Requirements**: Base Level 4+, Job Level 3+
- **Quest Items**: 3 Red Herbs + 3 Clovers + 1 Sterilized Bandage
- **Alternative**: Instructor in Training Grounds teaches it directly

**Class Inheritance**: All classes inherit First Aid upon job change from Novice. It persists through all class changes including transcendence.

#### Play Dead (ID 3 / rAthena NV_TRICKDEAD, ID 143)

| Property | Value | Source |
|----------|-------|--------|
| rAthena ID | 143 (NV_TRICKDEAD) | rAthena `db/pre-re/skill_db.yml` |
| Type | Toggle | rAthena `Toggleable: true` |
| Max Level | 1 | All sources |
| Target | Self | All sources |
| SP Cost | **5** | rAthena YAML, Divine Pride (NOT 1 as some wikis claim) |
| Cast Time | 0 ms (instant) | rAthena YAML |
| After-Cast Delay | 0 ms | rAthena YAML |
| Duration | **Infinite** (until toggled off or broken) | rAthena `tick = INFINITE_TICK` |
| Status Applied | SC_TRICKDEAD (status ID 47) | rAthena `status.hpp` |
| Quest Skill | Yes | rAthena `IsQuest: true` |

**Quest to Learn**:
- **NPC**: Prontera Chivalry Member (`prt_in` 73, 87) or Instructor Argos (Training Grounds)
- **Requirements**: Job Level 7+, must know First Aid
- **Mechanic**: Player must "hold breath" for 20 seconds (wait timer)

**Status Effect Mechanics (SC_TRICKDEAD)**:

| Behavior | Detail | Source |
|----------|--------|--------|
| Visual | Corpse lying-down animation (`dead_sit = 1`) | rAthena `status.cpp` |
| Movement | **BLOCKED** (`sc->cant.move`) | rAthena `status.cpp`, `unit.cpp` |
| Auto-Attack | **BLOCKED** (cannot be targeted by normal attacks) | rAthena `status.cpp` |
| Skill Use | **BLOCKED** (all skills except Play Dead itself) | rAthena `status.cpp` |
| Item Use | **BLOCKED** | rAthena client behavior |
| HP Regen | **BLOCKED** (`RGN_NONE`) | rAthena `status.cpp` |
| SP Regen | **BLOCKED** (`RGN_NONE`) | rAthena `status.cpp` |
| Monster Aggro | **IMMUNE** -- ALL monsters (including bosses/MVPs) ignore the player | iRO Wiki |
| PvP Damage | **IMMUNE** -- cannot be targeted in PvP | iRO Wiki |
| Toggle Off | Recasting Play Dead cancels the status (no SP cost to toggle off) | All sources |

**Break Conditions** (things that remove Play Dead):
- Recasting Play Dead (toggle off)
- Sage's Dispell
- Swordsman's Provoke
- Bleeding HP drain tick (each tick removes Play Dead)
- AoE status effects (Hammer Fall, Venom Dust) can still affect the player
- Tarot Card of Fate can damage/kill through Play Dead

**Class Restriction**: Play Dead is marked `Exclude: true` in rAthena's skill tree, meaning it is NOT inherited by 1st class or higher. Only Novice (and High Novice after rebirth, where quest skills are pre-learned) can use it. **Super Novice explicitly loses access to Play Dead.**

---

### Equipment Restrictions

Novices have the most restricted equipment access in the game.

#### Weapons

| Weapon Type | Equippable | Notes |
|-------------|-----------|-------|
| Dagger | Yes | Primary weapon type (Knife is a dagger) |
| 1H Sword | No | Requires Swordsman+ |
| 2H Sword | No | Requires Swordsman+ |
| 1H Spear | No | Requires Swordsman+ |
| 2H Spear | No | Requires Swordsman+ |
| 1H Axe | No | Requires Merchant+ |
| 2H Axe | No | Requires Merchant+ |
| Mace | No | Requires Acolyte/Merchant+ |
| Rod/Staff | No | Requires Mage/Acolyte+ |
| Bow | No | Requires Archer+ |
| Knuckle | No | Requires Monk+ |
| Katar | No | Requires Assassin+ |
| Instrument | No | Requires Bard+ |
| Whip | No | Requires Dancer+ |
| Book | No | Requires Sage+ |

**Note**: Some daggers are class-restricted (e.g., Combat Knife is Thief-only). Novices can only equip daggers that have `Novice` or `All` in their class restriction field.

#### Armor

| Armor Type | Equippable | Notes |
|------------|-----------|-------|
| Headgear (Upper) | Limited | Most headgears have level or class requirements; Novices can wear a few low-level ones |
| Headgear (Mid/Lower) | Limited | Same restrictions |
| Body Armor | Limited | Cotton Shirt, Novice Breastplate, and a few others |
| Shield | Limited | Guard, Novice Shield |
| Garment | Limited | Hood, Muffler |
| Shoes | Limited | Sandals, Shoes |
| Accessory | Limited | Clips, Novice Armlet |

#### ASPD Values (Weapon Delay)

| Weapon | Base Delay (seconds) | Notes |
|--------|---------------------|-------|
| Bare Hand | 0.50 | Slowest bare-hand in the game |
| Dagger | 0.55 | Only weapon type available |

Novice has the slowest ASPD modifiers of any class.

---

### Job Change Requirements to Each 1st Class

| Target Class | Requirements | Location |
|-------------|-------------|----------|
| Swordsman | Novice Job Level 10, Basic Skill Lv 9 | Izlude Swordman Academy |
| Mage | Novice Job Level 10, Basic Skill Lv 9 | Geffen Magic Academy |
| Archer | Novice Job Level 10, Basic Skill Lv 9 | Payon Archer Village |
| Merchant | Novice Job Level 10, Basic Skill Lv 9 | Alberta Merchant Guild |
| Thief | Novice Job Level 10, Basic Skill Lv 9 | Morroc Pyramid (Thieves Guild) |
| Acolyte | Novice Job Level 10, Basic Skill Lv 9 | Prontera Church |
| **Super Novice** | Novice Job Level 10, **Base Level 45+** | Al De Baran (Tzerero NPC) |

Each 1st class job change involves a unique quest line with the respective guild. The quests are flavor content -- the hard requirements are Job Level 10 and Basic Skill Lv 9.

**Important**: Job changing resets Job Level to 1. All 9 Novice skill points (Basic Skill Lv 9) are retained. Learned quest skills (First Aid, Play Dead) are also retained (except Play Dead becomes unusable for non-Novice classes after relog/map change).

---

### Novice-Specific Items

| Item | ID | Type | Stats | Weight | Effect |
|------|----|------|-------|--------|--------|
| Knife | 1201 | Dagger (Lv 1) | ATK 17, 3 slots | 40 | Starting weapon |
| Cotton Shirt | 2301 | Armor | DEF 1, 0 slots | 10 | Starting armor |
| Novice Potion | 12000 | Consumable | Heals 325 HP | 3 | Novice/Super Novice only |
| Novice Breastplate | 2340 | Armor | DEF 4, 1 slot | 0 | Novice/Super Novice only, purchasable in Juno (89,000z) |
| Novice Shield (Guard) | 2113 | Shield | DEF 3 | 40 | Novice-equippable guard |
| Novice Armlet | 2880 | Accessory | Various | 10 | Novice-tier accessory |

#### Angel Equipment Set (Super Novice Standard)

| Item | Slot | Stats | Notes |
|------|------|-------|-------|
| Angelic Guard [1] | Shield | DEF 3, MDEF +3 | Dropped by mini-boss Protocol |
| Angelic Protection [1] | Armor | DEF 4, MDEF +3 | Dropped by mini-boss Protocol |
| Angelic Cardigan [1] | Garment | DEF 1, MDEF +3 | Set bonus with full Angel set |
| Angel's Kiss [1] | Shoes | DEF 1, MaxHP +50 | Part of Angel set |
| Angel's Reincarnation [1] | Accessory | MDEF +2 | Part of Angel set |

#### Light Gear Set (NPC-Purchasable in Lighthalzen)

Affordable alternative gear for Super Novices, all purchasable from NPCs (except Novice Breastplate which is from Juno).

---

### Novice Class Properties

| Property | Value |
|----------|-------|
| Base Level Cap | 99 |
| Job Level Cap | 10 |
| HP Growth (HP_JOB_A) | 0.0 (flat growth only from base formula) |
| HP Growth (HP_JOB_B) | 5 |
| SP Growth (SP_JOB_A) | 0.0 |
| SP Growth (SP_JOB_B) | 2 |
| Weight Bonus | 0 |
| Death Penalty | **NONE** (Novices do not lose EXP on death) |
| Respawn HP | 50% of Max HP |

#### Job Bonus Stats (Novice, Job 1-10)

| Job Lv | STR | AGI | VIT | INT | DEX | LUK | Total |
|--------|-----|-----|-----|-----|-----|-----|-------|
| 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| 2 | 0 | 0 | 0 | 0 | 0 | +1 | 1 |
| 6 | 0 | +1 | 0 | 0 | +1 | +1 | 3 |
| 8 | +1 | +1 | +1 | 0 | +1 | +1 | 5 |
| 10 | +1 | +1 | +1 | +1 | +1 | +1 | 6 |

At max Job Level 10, Novice receives +1 to all six stats.

---

## SUPER NOVICE

The Super Novice is a unique "expanded" class that branches from Novice. It retains the Novice's fragile HP/SP pools but gains access to nearly every 1st class skill in the game, plus several unique mechanics (Guardian Angel, Fury, Steel Body resurrection). It is considered the most versatile class in terms of skill freedom, but one of the most difficult and expensive to play effectively.

---

### Job Change Requirements

| Requirement | Value | Source |
|-------------|-------|--------|
| Current Class | Novice | All sources |
| Job Level | 10 | All sources |
| Base Level | **45+** | All sources |
| Quest NPC | Tzerero | Al De Baran (`aldebaran` 116, 63), inside the Windmill building |
| Quest Items | 30 Sticky Mucus + 30 Resin | Quest items for combat test |
| Reward | Panties (armor) | Given upon successful job change |

**Important Quest Note**: During dialogue with the NPC, selecting "Huh?" is required. Selecting "What is that?" wastes the quest items and forces re-collection.

The Base Level 45 requirement is significantly higher than the 1st class requirement (just Job 10), making Super Novice a deliberate late-game choice that requires substantial leveling as a Novice first.

---

### Skill Access (All 1st Class Skills from All 6 Classes)

Super Novice can learn skills from **all six 1st class skill trees**: Swordsman, Mage, Archer, Acolyte, Thief, and Merchant. This is the class's defining feature.

#### Complete Accessible Skill List (Pre-Renewal)

**From Swordsman tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Sword Mastery | 100 | 10 | +ATK with 1H Swords and Daggers |
| ~~Two-Handed Sword Mastery~~ | 101 | -- | **EXCLUDED** (cannot equip 2H Swords) |
| Increase HP Recovery | 102 | 10 | |
| Bash | 103 | 10 | |
| Provoke | 104 | 10 | |
| Magnum Break | 105 | 10 | |
| Endure | 106 | 10 | |

**From Mage tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Increase SP Recovery | 200 | 10 | |
| Sight | 201 | 1 | |
| Napalm Beat | 202 | 10 | |
| Safety Wall | 203 | 10 | |
| Soul Strike | 204 | 10 | |
| Cold Bolt | 205 | 10 | |
| Frost Diver | 206 | 10 | |
| Stone Curse | 207 | 10 | |
| Fire Ball | 208 | 5 | |
| Fire Wall | 209 | 10 | |
| Fire Bolt | 210 | 10 | |
| Lightning Bolt | 211 | 10 | |
| Thunder Storm | 212 | 10 | |

**From Archer tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Owl's Eye | 300 | 10 | +DEX |
| Vulture's Eye | 301 | 10 | +Range, +HIT (for bow -- limited use since SN cannot equip bows) |
| Attention Concentrate | 302 | 10 | +DEX/AGI buff |
| ~~Double Strafe~~ | 303 | -- | **EXCLUDED** (requires bow) |
| Arrow Shower | 304 | 10 | Listed in some sources; requires bow to actually use |

**Note on Archer skills**: While Super Novice has access to some Archer tree skills in the skill tree data, the ones requiring bow equipment (Double Strafe, Arrow Shower) are functionally unusable since Super Novice cannot equip bows. Owl's Eye, Vulture's Eye, and Attention Concentrate work without bows.

**From Acolyte tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Divine Protection | 400 | 10 | |
| Demon Bane | 401 | 10 | |
| Ruwach | 402 | 1 | |
| Pneuma | 403 | 1 | |
| Teleportation | 404 | 2 | |
| Warp Portal | 405 | 4 | |
| Heal | 406 | 10 | |
| Increase Agility | 407 | 10 | |
| Decrease Agility | 408 | 10 | |
| Aqua Benedicta | 409 | 1 | |
| Signum Crucis | 410 | 10 | |
| Angelus | 411 | 10 | |
| Blessing | 412 | 10 | |
| Cure | 413 | 1 | |

**From Thief tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Double Attack | 500 | 10 | Works with daggers (very useful for Super Novice) |
| Increase Dodge | 501 | 10 | +Flee |
| Steal | 502 | 10 | |
| Hiding | 503 | 10 | |
| Envenom | 504 | 10 | |
| Detoxify | 505 | 1 | |

**From Merchant tree**:
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Enlarge Weight Limit | 600 | 10 | Critical for Super Novice's 0 weight bonus |
| Discount | 601 | 10 | |
| Overcharge | 602 | 10 | |
| Pushcart | 603 | 10 | |
| Identify | 604 | 1 | |
| Vending | 605 | 10 | |
| Mammonite | 606 | 10 | |

**From Novice tree** (retained):
| Skill | ID | Max Lv | Notes |
|-------|----|--------|-------|
| Basic Skill | 1 | 9 | Retained from Novice |
| First Aid | 2 | 1 | Retained from Novice |
| ~~Play Dead~~ | 3 | -- | **EXCLUDED** (loses access upon becoming Super Novice) |

#### Excluded Skills Summary

| Skill | Reason |
|-------|--------|
| Play Dead (NV_TRICKDEAD) | Explicitly excluded in rAthena skill tree (`Exclude: true`). Super Novice loses this upon job change. |
| Two-Handed Sword Mastery | Cannot equip 2H Swords |
| Double Strafe | Cannot equip Bows |
| Arrow Shower | Cannot equip Bows |

#### Total Available Skill Points

- **Job Level Cap**: 99
- **Skill Points from Job Levels 2-99**: **98 skill points**
- **Basic Skill**: 9 points (carried over from Novice)
- **Effective Points for 1st Class Skills**: 89 (98 minus 9 for Basic Skill, though Basic Skill is likely already maxed before job change)

With 98 total skill points and access to ~80+ skills across 6 trees, Super Novice has enormous build flexibility but cannot max everything. Strategic point allocation is critical.

---

### Skill Point Limits and Allocation

| Property | Value |
|----------|-------|
| Total Skill Points | 98 (from Job Levels 2-99) |
| Basic Skill Points | 9 (pre-allocated from Novice phase) |
| Skills Available | ~80+ from 6 first-class trees |
| Max Points in Any One Tree | Unlimited (can dump all 98 into one tree if desired) |

**Build Archetypes**:
- **Magic Super Novice**: Heavy Mage tree (Bolts, Soul Strike) + Acolyte support (Heal, Blessing, Inc AGI)
- **Melee Super Novice**: Swordsman combat (Bash, Magnum Break) + Thief passives (Double Attack, Dodge)
- **Support Super Novice**: Acolyte tree (Heal, Blessing, Cure, Angelus) + Merchant utility (Discount, Overcharge)
- **Hybrid**: Mix of combat, magic, and utility

**Critical constraint**: Unlike 1st class characters who get 49 points for one tree, Super Novice gets 98 points spread across six trees. This sounds generous but many builds require 50+ points just for core skills in two trees, leaving little for utility.

---

### Equipment Restrictions

Super Novice equipment restrictions are the same as a regular Novice -- they can only equip items that a Novice is allowed to wear, plus Novice-exclusive items.

#### Weapons

| Weapon Type | Equippable | ASPD Penalty | Notes |
|-------------|-----------|-------------|-------|
| Dagger | Yes | -15 | Primary melee weapon |
| 1H Sword | Yes | -17 | Available despite being a Swordsman weapon |
| 1H Axe | Yes | -10 | |
| Mace | Yes | -10 | Good for Heal-based builds |
| Rod/Staff (1H) | Yes | -25 | For magic builds, MATK bonus |
| Shield | Yes | -10 | Guard, Novice Shield, Angelic Guard |
| Bow | **No** | -- | Cannot equip |
| 2H Sword | **No** | -- | Cannot equip |
| 2H Axe | **No** | -- | Cannot equip (pre-renewal) |
| 2H Staff | **No** | -- | Cannot equip (pre-renewal) |
| 1H Spear | **No** | -- | Cannot equip |
| 2H Spear | **No** | -- | Cannot equip |
| Knuckle | **No** | -- | Cannot equip |
| Katar | **No** | -- | Cannot equip |
| Instrument | **No** | -- | Cannot equip |
| Whip | **No** | -- | Cannot equip |
| Book | **No** | -- | Cannot equip |

**Equipment vs. Skill Access**: Super Novice can learn Sword Mastery (which boosts daggers AND 1H swords), but the inability to equip bows or 2H weapons is the main limiting factor. Skills that require specific weapon types the Super Novice cannot equip are technically learnable but functionally useless.

#### Armor

Same restrictions as regular Novice:
- Body: Cotton Shirt, Novice Breastplate, Angelic Protection, and items with `All` class equip
- Shield: Guard, Novice Shield, Angelic Guard
- Garment: Hood, Muffler, Angelic Cardigan
- Shoes: Sandals, Shoes, Angel's Kiss
- Headgear: Limited selection (most headgears have class requirements)
- Accessories: Clips, Novice Armlet, Angel's Reincarnation

---

### Fury Mode / Spirit System

Super Novice has a unique **Fury** system activated through a chat chant mechanic, separate from the Soul Linker **Super Novice Spirit** buff.

#### Fury Mode (Self-Activated)

| Property | Value |
|----------|-------|
| Effect | **CRIT +50** |
| Duration | Until map change or relog |
| Activation Condition | Base EXP must be at an exact multiple of 10.0% (10.0%, 20.0%, 30.0%, ... 90.0%) |
| Required Chat Lines | Must have typed at least 7 lines of chat on the current map |
| Chant Text | Must type specific prayer text after the 7 chat lines |

**Chant Activation Sequence**:
1. Ensure your Base EXP is exactly at a 10% increment (10.0%, 20.0%, etc.)
2. Type at least 7 lines of chat text on the current map
3. Type the prayer: `"Guardian Angel, can you hear my voice?"`
4. Type: `"My name is [character name], and I'm a Super Novice~"`
5. Type: `"Please help me~"`
6. (Sometimes one additional line is needed)
7. Fury status activates (+50 CRI)

**Key Details**:
- Changing maps or relogging removes Fury and requires restarting the entire process
- The EXP percentage must be at exact 10% increments (e.g., 10.0% works, 10.5% does not)
- The +50 CRI bonus is enormous (equivalent to ~167 LUK for crit rate)

#### Super Novice Spirit (Soul Linker Buff)

| Property | Value | Source |
|----------|-------|--------|
| Skill ID | SL_SNOVICE / 451 | rAthena |
| Duration | 150/200/250/300/350 seconds (by level) | RateMyServer |
| Caster | Soul Linker class only | All sources |

**Effects vary by target's Job Level**:

| Job Level Threshold | Effect |
|---------------------|--------|
| Job Lv 70+ | Each Spirit Link cast has a 1% chance to clear the Super Novice's death counter, restoring the +10 all-stat bonus that was lost upon dying |
| Base Lv 90+ | Allows equipping any headgear in the game regardless of class restriction |

The death counter mechanic: Super Novices who have never died receive +10 to all stats as a hidden bonus. Dying removes this bonus permanently -- unless a Soul Linker's Spirit Link triggers the 1% chance to reset the counter.

---

### Death Angel Mechanic (EXP Protection)

Super Novice has two unique death-related mechanics:

#### 1. Guardian Angel Level-Up Buff

| Trigger | When a Super Novice levels up |
|---------|------------------------------|
| Effect | Guardian Angel appears and casts a random buff |
| Possible Buffs | Kyrie Eleison, Magnificat, Gloria, Suffragium, Impositio Manus |
| Duration | Standard skill duration for the chosen buff |

#### 2. Steel Body Death Protection (99% EXP Angel)

| Property | Value |
|----------|-------|
| Trigger | Super Novice's HP reaches 0 while Base EXP is between 99.0% and 99.9% |
| Effect | Guardian Angel appears, fully heals HP, and casts **Steel Body (Mental Strength)** on the Super Novice |
| Duration | Steel Body lasts its normal duration |
| Repeatable | **No** -- cannot trigger again until the Super Novice relogs |

**Steel Body** (from Monk skill tree): Grants extreme DEF/MDEF bonuses but prevents movement, attacking, skill use, and item use (similar to Play Dead but with defensive stats). The angel-granted version still imposes these restrictions.

**At Level 99**: Once a Super Novice reaches 99 million EXP past level 99 (effectively 99.9% at the EXP cap), Steel Body triggers each time they die. However, this can only happen once per login session -- they must relog for it to activate again.

**EXP Penalty on Death**: Unlike regular Novice (which has no death penalty), **Super Novice does suffer the standard 1% Base EXP loss on death**. The Steel Body mechanic prevents the death itself but does not prevent EXP loss if the Super Novice does actually die.

---

### Level Cap and Stats

| Property | Value |
|----------|-------|
| Base Level Cap | 99 |
| Job Level Cap | 99 |
| Total Stat Points (Levels 1-99) | 1,273 (same as all pre-renewal classes) |
| HP Growth (HP_JOB_A) | **0.0** (same as regular Novice -- very low HP) |
| HP Growth (HP_JOB_B) | 5 |
| SP Growth (SP_JOB_A) | **0.0** (same as regular Novice -- very low SP) |
| SP Growth (SP_JOB_B) | 2 |
| Weight Bonus | **0** (same as regular Novice -- no weight bonus) |
| Death Penalty | 1% Base EXP loss (NOT exempt like regular Novice) |
| Base Level 99 HP Bonus | **+2000 Max HP** (flat, permanent, affected by cards but NOT VIT) |

#### HP Comparison at Level 99

| Class | VIT 1 | VIT 50 | VIT 99 |
|-------|-------|--------|--------|
| Novice/Super Novice (base) | ~530 | ~795 | ~1,054 |
| Super Novice (with Lv99 +2000) | ~2,530 | ~2,795 | ~3,054 |
| Swordsman (for comparison) | ~4,700 | ~7,050 | ~9,353 |
| Knight (for comparison) | ~7,955 | ~11,933 | ~15,831 |

The +2000 HP bonus at Level 99 is critical for Super Novice survival but still leaves them far below other classes.

#### Job Bonus Stats (Super Novice, Job 1-99)

The Super Novice receives balanced job bonuses across all stats, totaling approximately +5 to each stat by Job Level 99. The exact per-level distribution can be found in rAthena's `job_db.yml` but follows an even spread pattern.

| Stat | Total at Job 99 |
|------|----------------|
| STR | +5 |
| AGI | +5 |
| VIT | +5 |
| INT | +5 |
| DEX | +5 |
| LUK | +5 |
| **Total** | **+30** |

#### Hidden Death Counter Bonus

| Condition | Bonus |
|-----------|-------|
| Super Novice has NEVER died | +10 to ALL stats (STR/AGI/VIT/INT/DEX/LUK) |
| After first death | Bonus is lost permanently |
| Soul Linker Spirit (Job Lv 70+) | 1% chance per cast to reset death counter and restore +10 bonus |

This +10 all-stats bonus is significant and incentivizes careful play. Combined with the Fury +50 CRI, a deathless Super Novice with Fury active is surprisingly powerful for its class.

---

### Super Novice-Exclusive Features

| Feature | Description |
|---------|-------------|
| Fury Mode | +50 CRI via chat chant at 10% EXP increments |
| Guardian Angel (Level-Up) | Random Acolyte buff on each level-up |
| Steel Body Angel | Death protection at 99.0-99.9% EXP |
| Deathless +10 Stats | +10 all stats while death count = 0 |
| +2000 HP at Lv 99 | Flat permanent HP bonus |
| /doridori Command | Head-shake animation that doubles HP/SP natural regen rate temporarily |
| Novice Potion Access | Can use Novice Potions (class-restricted consumable, 325 HP) |
| 98 Skill Points | More skill points than any other class |
| 6-Tree Skill Access | Can learn from all 1st class skill trees |
| Pushcart/Vending | Can use Merchant's cart and vending skills |

#### /doridori Command

- Super Novice exclusive command
- Character shakes head rapidly (~6 times in 1.5-3 seconds)
- Doubles natural HP/SP regeneration rate for a period
- Different from sitting (which also doubles regen)

---

### Advantages and Disadvantages vs Regular Classes

#### Advantages

| Advantage | Detail |
|-----------|--------|
| Unmatched Skill Versatility | Access to ~80+ skills from all 6 first-class trees |
| Self-Sufficient | Can heal (Heal), buff (Blessing, Inc AGI), steal, identify, teleport, and fight -- all on one character |
| Fury +50 CRI | When activated, one of the highest CRI bonuses in the game |
| Deathless +10 Stats | +10 to all stats is equivalent to ~60 stat points worth of bonuses |
| Lv 99 HP Bonus | +2000 HP partially compensates for low base HP |
| 98 Skill Points | Most skill points of any class (regular 1st class gets 49, 2nd class gets 49) |
| No Class-Specific Quests | Does not need to complete any class guild tests |
| Vending + Pushcart | Can run a shop like a Merchant |
| Double Attack + Dagger | Thief's Double Attack works with daggers, giving good DPS |
| Heal + Magic | Can switch between physical and magical playstyles |

#### Disadvantages

| Disadvantage | Detail |
|--------------|--------|
| Extremely Low HP | HP_JOB_A = 0.0, lowest in the game. Even with Lv99 bonus, much lower than melee classes |
| Extremely Low SP | SP_JOB_A = 0.0, lowest in the game. Magic builds are SP-starved |
| No 2nd Class Skills | Cannot access any 2nd class skills (Knight, Wizard, Hunter, etc.) |
| Equipment Limitations | Cannot equip bows, 2H swords, spears, knuckles, katars, instruments, whips, books |
| No Weight Bonus | 0 weight bonus from class, making inventory management difficult |
| Expensive to Gear | Novice-equippable gear is limited and some is expensive (Novice Breastplate 89,000z) |
| Death Counter Fragile | Losing the +10 all-stats bonus on first death is devastating |
| Fury Requires Setup | Must be at exact 10% EXP, type 7+ chat lines, and type specific chant text |
| Low ASPD | Novice weapon delay values are the slowest |
| No Play Dead | Loses the Novice survival tool upon job change |
| Difficult for New Players | Build complexity and fragility make this a poor choice for beginners |

---

## Implementation Checklist

### Already Implemented in Server

| Item | Status | Location |
|------|--------|----------|
| Basic Skill definition (ID 1) | Done | `ro_skill_data.js` line 18 |
| First Aid definition (ID 2) | Done | `ro_skill_data.js` line 19 |
| Play Dead definition (ID 3) | Done | `ro_skill_data.js` line 20 |
| First Aid handler | Done | `index.js` (heals 5 HP, deducts 3 SP) |
| Play Dead handler | Done | `index.js` (toggle buff with movement/attack/skill/regen blocking, enemy deaggro) |
| Basic Skill Lv 9 job change gate | Done | `index.js` job:change handler |
| Play Dead break conditions (Dispel, Provoke, Bleeding) | Done | `index.js` |
| isQuestSkill flag on First Aid/Play Dead | Done | `ro_skill_data.js` |
| Super Novice in class system | Partial | `index.js` line 3084 (class mapping), line 4604+ (skill class access) |
| Super Novice Pushcart access | Done | `index.js` line 6707 (`CART_CLASSES` includes `super_novice`) |

### Needs Implementation

| Item | Priority | Effort | Notes |
|------|----------|--------|-------|
| Super Novice job change handler | HIGH | Medium | Base Level 45+ check, Job Level 10, special quest flow |
| Super Novice skill tree (combined 1st class trees) | HIGH | Medium | Need to populate skill tree with correct prereqs from all 6 classes |
| Super Novice equipment restrictions | HIGH | Medium | Must enforce Novice-level equip restrictions (no bow, no 2H sword, no spear, etc.) |
| Play Dead exclusion for Super Novice | HIGH | Small | Remove Play Dead from Super Novice's available skills |
| Super Novice HP/SP tables | HIGH | Small | Verify HP_JOB_A=0.0, SP_JOB_A=0.0 match Novice tables |
| Super Novice ASPD values | HIGH | Small | Set weapon delay values (same as Novice) |
| Basic Skill per-level feature gating | MEDIUM | Medium | Gate trading (Lv1), emotes (Lv2), sitting (Lv3), chat rooms (Lv4), party join (Lv5), Kafra (Lv6), party create (Lv7) |
| Fury Mode (CRIT +50 chant system) | MEDIUM | Large | EXP percentage check, chat line counter, chant text matching, map-scoped state |
| Guardian Angel level-up buff | MEDIUM | Medium | Random buff selection (Kyrie/Magnificat/Gloria/Suffragium/Impositio) on level-up |
| Steel Body death protection | MEDIUM | Medium | Trigger at 99.0-99.9% EXP, full heal + Steel Body buff, once per login |
| Deathless +10 all-stats bonus | MEDIUM | Small | Track death count, apply +10 bonus if count = 0, remove on death |
| Level 99 +2000 HP bonus | MEDIUM | Small | Flat +2000 MaxHP on reaching Base Level 99 |
| /doridori command | LOW | Small | Head-shake animation, double HP/SP regen temporarily |
| Novice Potion class restriction | LOW | Small | Restrict Novice Potion (12000) to Novice/Super Novice |
| Soul Linker Super Novice Spirit | LOW | Large | Requires Soul Linker class (not yet in game) |
| Quest skill infrastructure | LOW | Large | Learn First Aid/Play Dead via NPC quest instead of skill points |
| Sitting regen mechanic (Basic Skill Lv 3) | LOW | Medium | Sitting doubles HP/SP regen; requires Basic Skill Lv 3 |

---

## Gap Analysis

### Current State vs. Complete Implementation

#### Novice Class

| Feature | Current State | Canonical State | Gap |
|---------|--------------|----------------|-----|
| Basic Skill | Definition correct, no per-level gating | Should gate 7 features per level | MEDIUM gap -- features themselves (trading, chat rooms) not all implemented yet |
| First Aid | Fully implemented, correct SP/HP values | Should be quest-learned | LOW gap -- functionally correct, quest delivery deferred |
| Play Dead | Fully implemented with break conditions | Should be quest-learned, loses access after job change | LOW gap -- quest delivery deferred, class restriction after job change not enforced |
| Starting Equipment | Not explicitly handled | Knife + Cotton Shirt on character creation | LOW gap -- training system not implemented |
| Death Penalty Exemption | Need to verify | Novices should have 0% EXP loss on death | Verify `deathPenalty` check for novice class |
| Novice Potion | Not class-restricted | Should only be usable by Novice/Super Novice | SMALL gap |

#### Super Novice Class

| Feature | Current State | Canonical State | Gap |
|---------|--------------|----------------|-----|
| Class Exists | Present in class mapping and skill access | Full implementation | LARGE gap -- class exists in data but lacks unique mechanics |
| Skill Tree | Included in skill access maps (SuperNovice) | Should show combined 6-tree with correct prereqs | MEDIUM gap |
| Play Dead Exclusion | Not enforced | SN should not have Play Dead | SMALL gap |
| Equipment Restrictions | Not specifically enforced for SN | Novice-level equip only (no bow, no 2H, no spear, etc.) | MEDIUM gap |
| HP/SP Growth | Uses Novice tables (correct) | HP_JOB_A=0.0, SP_JOB_A=0.0 | Verify |
| Fury Mode | Not implemented | +50 CRI via chat chant at 10% EXP milestones | LARGE gap |
| Guardian Angel | Not implemented | Random buff on level-up | MEDIUM gap |
| Steel Body Angel | Not implemented | Death protection at 99% EXP | MEDIUM gap |
| Deathless Bonus | Not implemented | +10 all stats while death count = 0 | MEDIUM gap |
| Lv 99 HP Bonus | Not implemented | +2000 flat MaxHP at Base Level 99 | SMALL gap |
| /doridori | Not implemented | Double regen command | SMALL gap |
| Job Bonus Stats | Not verified | +5 each stat by Job 99, total +30 | Verify in server |
| Weight Bonus | Set to 0 (correct) | 0 weight bonus | Confirmed correct |
| Death Penalty | Not verified for SN | 1% Base EXP loss (same as other non-Novice classes) | Verify |

### Priority Order for Super Novice Implementation

1. **P1 (Required)**: Job change handler (Base Lv 45+), skill tree population, equipment restrictions, Play Dead exclusion
2. **P2 (Core Identity)**: Fury Mode, Guardian Angel buffs, deathless +10 stats bonus, Steel Body death protection, Lv 99 +2000 HP
3. **P3 (Polish)**: /doridori command, Novice Potion restriction, Basic Skill per-level gating
4. **P4 (Future)**: Soul Linker Spirit, quest skill infrastructure, sitting mechanic

---

## Sources

### Primary (Authoritative)
- [rAthena `db/pre-re/skill_db.yml`](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_db.yml)
- [rAthena `db/pre-re/skill_tree.yml`](https://github.com/rathena/rathena/blob/master/db/pre-re/skill_tree.yml)
- [rAthena `src/map/status.cpp`](https://github.com/rathena/rathena/blob/master/src/map/status.cpp)
- [rAthena `npc/pre-re/jobs/novice/novice.txt`](https://github.com/rathena/rathena/blob/master/npc/pre-re/jobs/novice/novice.txt)

### Secondary (Community Wikis)
- [iRO Wiki - Novice](https://irowiki.org/wiki/Novice)
- [iRO Wiki - Super Novice](https://irowiki.org/wiki/Super_Novice)
- [iRO Wiki Classic - Super Novice](https://irowiki.org/classic/Super_Novice)
- [iRO Wiki - Basic Skill](https://irowiki.org/wiki/Basic_Skill)
- [iRO Wiki - First Aid](https://irowiki.org/wiki/First_Aid)
- [iRO Wiki - Play Dead](https://irowiki.org/wiki/Play_Dead)
- [iRO Wiki - Super Novice Job Change Guide](https://irowiki.org/wiki/Super_Novice_Job_Change_Guide)
- [iRO Wiki - Super Novice Spirit](https://irowiki.org/wiki/Super_Novice_Spirit)
- [iRO Wiki - Novice Skill Quest (Classic)](https://irowiki.org/wiki/Novice_Skill_Quest(Classic))
- [iRO Wiki - Classes](https://irowiki.org/wiki/Classes)
- [iRO Wiki - ASPD](https://irowiki.org/wiki/ASPD)

### Tertiary (Guides and Cross-Reference)
- [RateMyServer - Super Novice Skills](https://ratemyserver.net/index.php?page=skill_db&jid=23)
- [RateMyServer - Novice Skills](https://ratemyserver.net/index.php?jid=0&page=skill_db)
- [RateMyServer - Super Novice Job Bonus Stats](https://ratemyserver.net/index.php?page=misc_table_stbonus&op=23)
- [RateMyServer - Super Novice Guide](https://write.ratemyserver.net/ragnoark-online-character-guides/super-novice-guide/)
- [RateMyServer - Novice Platinum Skill Quest](https://ratemyserver.net/quest_db.php?type=50000&qid=50001)
- [RateMyServer - Super Novice Job Quest](https://ratemyserver.net/quest_db.php?type=60000&qid=60025)
- [RateMyServer - Knife Item](https://ratemyserver.net/index.php?page=item_db&item_id=1202)
- [RateMyServer - Novice Breastplate](https://ratemyserver.net/index.php?page=item_db&item_id=2340)
- [StrategyWiki - Ragnarok Online Supernovice](https://strategywiki.org/wiki/Ragnarok_Online/Supernovice)
- [JellyRO Wiki - Super Novice](https://wiki.jellyro.com/index.php/Super_Novice)
- [JellyRO Wiki - SuperNovice Secrets/FAQs](https://wiki.jellyro.com/index.php/SuperNovice_Secret's/Faq's)
- [Ragnarok Fandom Wiki - Novice](https://ragnarok.fandom.com/wiki/Novice)
- [Ragnarok Fandom Wiki - Super Novice](https://ragnarok.fandom.com/wiki/Super_Novice)
- [ROGGH Library - Super Novice Class Guide](https://roggh.com/super-novice-class-guide/)
- [HubPages - Super Novice Job Change Quest Guide](https://discover.hubpages.com/games-hobbies/Ragnarok-Online-Super-Novice-Job-Change-Quest-Guide)
- [Gamers Portal 101 - 10 Super Novice Secrets](https://gamersportal101.blogspot.com/2014/09/raganarok-online-10-super-novice-secrets.html)
- [GameFAQs - Perma-Novice Guide](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/40612)
- [GameFAQs - Death Penalties](https://gamefaqs.gamespot.com/boards/561051-ragnarok-online/41885231)
- [Divine Pride - First Aid (142)](https://www.divine-pride.net/database/skill/142)
- [Divine Pride - Play Dead (143)](https://www.divine-pride.net/database/skill/143)
- [rAthena Forum - Super Novice Chant](https://rathena.org/board/topic/122474-super-novice-chant-furyexplosion/)
- [rAthena GitHub Issue #5392 - Death Penalty Config](https://github.com/rathena/rathena/issues/5392)
- [Spirit of the Supernovice - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&skid=451)
