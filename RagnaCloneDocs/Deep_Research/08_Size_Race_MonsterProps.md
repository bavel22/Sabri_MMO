# Size, Race & Monster Properties -- Deep Research (Pre-Renewal)

> Comprehensive reference for Ragnarok Online Classic (pre-Renewal) size modifiers, race system,
> and monster property flags. Sourced from rAthena pre-renewal source code, iRO Wiki Classic,
> RateMyServer archives, Hercules documentation, and community-verified data.

---

## Table of Contents

1. [Size System](#1-size-system)
2. [Race System](#2-race-system)
3. [Monster Properties](#3-monster-properties)
4. [Damage Modifier Stack Order](#4-damage-modifier-stack-order)
5. [Implementation Checklist](#5-implementation-checklist)
6. [Gap Analysis](#6-gap-analysis)

---

## 1. Size System

### 1.1 Three Sizes

Every entity in Ragnarok Online (players, monsters, summons) has exactly one of three sizes:

| Size ID | Size Name | Description | Examples |
|---------|-----------|-------------|---------|
| 0 | **Small** | Small-bodied creatures, juveniles, insects, baby characters | Lunatic, Fabre, Hornet, Chonchon, Picky, Creamy, Hunter Fly, Mistress, Angeling, Ghostring, Deviling |
| 1 | **Medium** | Standard humanoids, mid-size creatures, all player characters | Poring, Zombie, Orc Warrior, Raydric, Clock, Doppelganger, Drake, Osiris, Moonlight Flower |
| 2 | **Large** | Massive creatures, most MVPs, dragons, golems | Peco Peco, Isis, High Orc, Stalactic Golem, Baphomet, Orc Lord, Valkyrie Randgris, Turtle General |

**Player size**: All player characters are **Medium** by default. Adopted/baby characters are **Small**. No player is ever Large.

**Monster size distribution** (approximate across the full pre-renewal mob_db):
- Small: ~30% (insects, baby monsters, bats, small animals, some mini-bosses like Mistress/Angeling)
- Medium: ~45% (humanoid monsters, undead, most fish, mid-size brutes)
- Large: ~25% (bosses, dragons, large beasts, golems, most MVPs)

---

### 1.2 Weapon Type vs Size Penalty Table (Complete)

Size penalty applies **only to the WeaponATK portion** of physical damage. StatusATK, MasteryATK, BuffATK, and refinement bonuses are NOT affected by size penalty.

```
SizedWeaponATK = floor(WeaponATK * SizePenaltyPct / 100)
```

**Complete table** (verified against iRO Wiki Classic and rAthena `db/pre-re/size_fix.txt`):

| Weapon Type | vs Small | vs Medium | vs Large | Best Against | Worst Against |
|-------------|----------|-----------|----------|-------------|---------------|
| **Bare Fist** | 100% | 100% | 100% | All equal | None |
| **Dagger** | 100% | 75% | 50% | Small | Large |
| **1H Sword** | 75% | 100% | 75% | Medium | Small/Large |
| **2H Sword** | 75% | 75% | 100% | Large | Small/Medium |
| **Spear (on foot)** | 75% | 75% | 100% | Large | Small/Medium |
| **Spear (mounted/Peco)** | 75% | 100% | 100% | Medium/Large | Small |
| **1H Axe** | 50% | 75% | 100% | Large | Small |
| **2H Axe** | 50% | 75% | 100% | Large | Small |
| **Mace** | 75% | 100% | 100% | Medium/Large | Small |
| **Rod / Staff** | 100% | 100% | 100% | All equal | None |
| **Bow** | 100% | 100% | 75% | Small/Medium | Large |
| **Katar** | 75% | 100% | 75% | Medium | Small/Large |
| **Book** | 100% | 100% | 50% | Small/Medium | Large |
| **Knuckle / Claw** | 100% | 75% | 50% | Small | Large |
| **Instrument** | 75% | 100% | 75% | Medium | Small/Large |
| **Whip** | 75% | 100% | 50% | Medium | Large |
| **Gun** | 100% | 100% | 100% | All equal | None |
| **Huuma Shuriken** | 100% | 100% | 100% | All equal | None |

**Key strategic observations**:
- **Daggers** excel against Small (100%) but suffer heavily against Large (50%) -- Assassins must use Katar for Large targets
- **2H Swords / Spears** are anti-Large weapons (100% vs Large)
- **Mounted Spears** gain a unique bonus: Medium penalty removed (75% -> 100%), making Knights/Crusaders on Peco effective against both Medium and Large
- **Axes** are the worst against Small (50%) but best against Large (100%)
- **Rods, Guns, Bare Fists, Huuma Shurikens** have zero size penalty (always 100%)
- **Maces** have no penalty against Medium or Large (good for Acolyte/Priest PvE)
- **Katars** are optimized for Medium (PvP) but suffer against both Small and Large (75%)
- **Books** and **Knuckles** suffer heavily against Large (50%)

---

### 1.3 Skills That Bypass Size Penalty

Certain skills have their own damage formulas that either ignore size penalty entirely or use a fixed formula unaffected by weapon size modifiers:

| Skill | Class | Bypass Type | Notes |
|-------|-------|-------------|-------|
| **Investigate** (Occult Impaction) | Monk | Full bypass | Uses DEF-to-damage conversion; own formula ignores size |
| **Asura Strike** (Extremity Fist) | Monk/Champion | Full bypass | Fixed formula: `(250 + 150*Lv + statusATK + SP*factor)`; not subject to size penalty |
| **Grand Cross** | Crusader | Full bypass | Uses WeaponATK-only formula with custom calculation |
| **Shield Boomerang** | Crusader | Full bypass | Custom damage = batk + shieldWeight/10 + refine*5 |
| **Acid Terror** | Alchemist | Full bypass | Uses own damage formula; ignores weapon ATK |
| **Cart Revolution** | Merchant | Partial | Uses cart weight in formula, but weapon portion still has size penalty |

**Important**: Magical skills (all bolt spells, AoE magic, Heal vs Undead, etc.) inherently bypass size penalty because size modifier only applies to the WeaponATK portion of **physical** damage. MATK-based damage is never subject to size penalty.

---

### 1.4 noSizePenalty Effects

These effects override the size penalty table, treating all targets as if they were the weapon's optimal size (100% modifier):

| Source | Type | Effect |
|--------|------|--------|
| **Drake Card** (weapon) | Card | `bNoSizeFix` -- Nullifies size penalty for ALL attacks with this weapon |
| **Maximize Power** (Blacksmith Lv5) | Buff | `noSizePenalty` flag -- While active, size penalty is removed |

In the damage formula, these are checked before the size penalty lookup:
```javascript
const sizePenaltyPct = (attacker.cardNoSizeFix || attacker.buffMods.noSizePenalty)
    ? 100
    : getSizePenalty(weaponType, targetSize);
```

**Drake Card** is one of the most valuable MVP cards in the game because it removes a fundamental constraint on weapon choice.

---

### 1.5 Size-Specific Cards

**Offensive cards** (weapon slot, increase damage dealt to targets of a specific size):

| Card | Size Target | Bonus | Additional Effect |
|------|------------|-------|-------------------|
| **Desert Wolf Card** | Small | +15% damage | +5 ATK |
| **Skeleton Worker Card** | Medium | +15% damage | +5 ATK |
| **Minorous Card** | Large | +15% damage | +5 ATK |

These cards stack additively with each other within the same category, and the total size bonus is applied multiplicatively with other card bonus categories (race, element).

**Defensive cards** (shield/armor slot, reduce damage received based on attacker size):

| Card | Size Source | Reduction |
|------|-----------|-----------|
| **Pecopeco Card** | All sizes (MaxHP) | +10% MaxHP (indirect defense) |
| **Peco Peco Egg Card** | -- | DEF +2, MDEF +1 (general, not size-specific) |

Note: Unlike race-based defensive cards (which have clear -30% reductions per race), size-based defensive reductions are less common and less standardized. The primary defensive tool against specific sizes is proper equipment and positioning rather than shield cards.

---

## 2. Race System

### 2.1 Complete Race List (10 Races)

Every monster and player in RO Classic belongs to exactly one of 10 races. The `Player` race exists in Renewal but in pre-renewal is treated as Demi-Human (Race 7).

| Race ID | Race Name | Alternative Names | Description | Example Monsters |
|---------|-----------|-------------------|-------------|-----------------|
| 0 | **Formless** | Amorphous | Amorphous beings, mechanical constructs, animated objects | Poring, Drops, Poporing, Clock, Alarm, Apocalypse, Mastering |
| 1 | **Undead** | -- | Reanimated dead, skeletal creatures, spirits | Zombie, Skeleton, Mummy, Ghoul, Wraith, Dracula, Osiris, Dark Priest |
| 2 | **Brute** | Animal | Animals, beasts, animalistic creatures | Lunatic, Wolf, Savage, Grizzly, Peco Peco, Condor, Familiar, Atroce, Hatii, Phreeoni |
| 3 | **Plant** | -- | Vegetation, fungi, and plant-like organisms | Mandragora, Flora, Geographer, Rafflesia, Red/Blue/Yellow/Shining Plant, Willow, Poring* |
| 4 | **Insect** | Bug | Arthropods, arachnids, bugs | Creamy, Hornet, Fabre, Chonchon, Scorpion, Hunter Fly, Maya, Golden Thief Bug, Mistress |
| 5 | **Fish** | Aquatic | Water-dwelling and aquatic creatures | Swordfish, Marse, Obeaune, Marc, Penomena, Anolian, Roda Frog |
| 6 | **Demon** | Devil | Hellspawn, dark magical entities, evil spirits | Baphomet, Dark Lord, Succubus, Incubus, Whisper, Marionette, Bathory, Sohee, Tao Gunka, Moonlight Flower |
| 7 | **Demi-Human** | Humanoid | Humanoid monsters AND all Player Characters | Orc Warrior, Orc Archer, Kobold, Goblin, Raydric, Abysmal Knight, High Orc, Pharaoh, all PCs |
| 8 | **Angel** | -- | Divine beings, holy creatures | Angeling, Archangeling, Valkyrie Randgris |
| 9 | **Dragon** | Draconic | Draconic creatures, wyrms, serpents | Petite, Deleter, Ktullanux, Detardeurus, Baby Dragon, Mutant Dragon |

*Note: Poring is Race 3 (Plant) despite looking amorphous. Race is determined by the official mob_db, not visual appearance.*

**Player race**: All player characters are **Demi-Human** (Race 7) in pre-renewal. This is why Hydra Card (+20% vs Demi-Human) is the premier PvP damage card.

---

### 2.2 Race-Specific Offensive Cards (Weapon Slot)

These cards increase physical damage dealt to monsters/players of the specified race. They are among the most important cards in the game for PvE optimization.

| Card | Target Race | Bonus | Card ID |
|------|------------|-------|---------|
| **Pecopeco Egg Card** | Formless (0) | +20% damage | 4052 |
| **Orc Skeleton Card** | Undead (1) | +20% damage | 4072 |
| **Goblin Card** | Brute (2) | +20% damage | 4060 |
| **Mandragora Card** | Plant (3) | +20% damage | 4100 |
| **Caramel Card** | Insect (4) | +20% damage | 4045 |
| **Vadon Card** | Fish (5)* | +20% damage | 4051 |
| **Strouf Card** | Demon (6) | +20% damage | 4108 |
| **Hydra Card** | Demi-Human (7) | +20% damage | 4035 |
| **Goblin Leader Card** | Angel (8) | +20% damage | 4128 |
| **Earth Petite Card** | Dragon (9) | +20% damage | 4172 |

*Note: Vadon Card is actually +20% vs **Fire element**, not Fish race. The Fish race offensive card is **Marina Card** in some contexts, but it is actually +5% Freeze chance. For pure race-based +20% vs Fish, use **Orc Skeleton Card** (Undead) + element strategy. The canonical Fish race +20% offensive card from rAthena is the **Kaho Card** (in some databases). Cross-reference with your card database implementation.*

**Boss-type offensive card** (special):

| Card | Target | Bonus |
|------|--------|-------|
| **Abysmal Knight Card** | Boss-type monsters | +25% damage |

This card targets monster **class** (boss protocol), not race -- it stacks multiplicatively with race cards.

---

### 2.3 Race-Specific Defensive Cards (Shield Slot)

These cards reduce ALL damage received (physical AND magical) from monsters/players of the specified race. They even reduce damage from attacks that ignore DEF/MDEF.

| Card | Against Race | Reduction | Card ID |
|------|-------------|-----------|---------|
| **Cloud Hermit Card** | Formless (0) | -30% damage received | 4097 |
| **Orc Lady Card** | Brute (2)* | -30% damage received | 4098 |
| **Rafflesia Card** | Fish (5) | -30% damage received | 4159 |
| **Khalitzburg Card** | Demon (6) | -30% damage received | 4141 |
| **Thara Frog Card** | Demi-Human (7) | -30% damage received | 4028 |
| **Hode Card** | Dragon (9) | -30% damage received | -- |
| **Alice Card** | Boss-class monsters | -40% damage received | 4191 |

*Note: The complete set for all 10 races may vary slightly by server/database. The six listed above are the canonical pre-renewal shield race reduction cards confirmed across multiple sources.*

**Alice Card** is special -- it targets boss **class** (boss protocol monsters), not a specific race. It stacks with race reduction cards.

---

### 2.4 Race-Specific Resistance

Beyond shield cards, several other sources provide race-based damage reduction:

| Source | Effect |
|--------|--------|
| **Angelus** (Priest) | Does NOT provide race-specific reduction (VIT-DEF only) |
| **Assumptio** (Priest, not pre-renewal) | General damage halving |
| **Thara Frog Card** | -30% from Demi-Human (the PvP/WoE staple) |
| **Race-specific armor cards** | Vary by implementation |
| **Dragonology** (Sage passive) | Dragon element resistance + MATK bonus |
| **Invulnerable Siegfried** (Bard ensemble) | Element resist (not race-based) |

---

## 3. Monster Properties

### 3.1 Monster Class (Normal / Boss / MVP)

Every monster in the mob_db has a **class** field that determines fundamental behavior:

| Class | Description | Boss Protocol | MVP Rewards | Examples |
|-------|-------------|--------------|-------------|---------|
| **Normal** | Standard monsters | No | No | Poring, Zombie, Orc Warrior |
| **Boss** (Mini-Boss) | Rare spawns with boss protocol | **Yes** | No | Angeling, Deviling, Ghostring, Tao Gunka |
| **MVP** | Most Valuable Player bosses | **Yes** | **Yes** | Baphomet, Dark Lord, Osiris, Pharaoh |
| **Guardian** | WoE castle defenders | **Yes** | No | WoE Guardian (immobile/mobile) |

In rAthena's mob_db, `Class` is encoded as:
- `Normal` (default)
- `Boss` (has boss protocol but no MVP rewards)
- `Guardian` (WoE-specific)

The `MD_MVP` mode flag (`0x0080000`) separately enables MVP rewards (3 bonus drops, MVP EXP, tombstone).

---

### 3.2 Boss Protocol Flag

**Boss Protocol** is a set of immunities and special properties automatically applied to all monsters with `boss` or `mvp` class. It consists of three core mode flags:

```
MD_KNOCKBACKIMMUNE  (0x0200000)  — Cannot be displaced by knockback skills
MD_STATUSIMMUNE     (0x4000000)  — Immune to most status effects
MD_DETECTOR         (0x2000000)  — Can detect and attack hidden/cloaked players
```

#### 3.2.1 Status Effects Bosses ARE Immune To

Boss protocol monsters are immune to the following status effects:

| Status Effect | Normally Inflicted By |
|--------------|-----------------------|
| **Stun** | Bash Lv6+, Shield Charge, Hammer Fall, Storm Gust |
| **Freeze** | Frost Diver, Storm Gust, Frost Nova, Cold Bolt procs |
| **Stone Curse** | Stone Curse skill, Medusa Card |
| **Sleep** | Lullaby (Bard ensemble) |
| **Blind** | Curse/Blind skills, Dark element procs |
| **Silence** | Lex Divina (Priest) |
| **Curse** | Curse-inflicting attacks |
| **Poison** | Envenom, Venom Dust, Poison React |
| **Bleeding** | Various skill procs |
| **Burning** | Fire-based skill procs |
| **Confusion** | Confusion-inflicting skills |
| **Coma** | Gloria Domini, Tarot Card of Fate* |
| **Fear** | Various |
| **Deep Sleep** | Various |

*Exception: The Priest's Coma skill CAN affect Undead-element bosses specifically.*

#### 3.2.2 Effects That WORK on Bosses (Boss Protocol Exceptions)

Despite boss protocol, certain effects can still affect boss monsters:

| Effect | Why It Works | Notes |
|--------|-------------|-------|
| **Quagmire** (Wizard) | Reduces AGI/DEX/MoveSpeed | Cannot reduce stats by more than 50% on bosses |
| **Divest / Strip** skills (Rogue) | Equipment strip effects | Works on bosses (strip weapon/armor/helm/shield) |
| **Decrease AGI** (Acolyte) | Speed reduction | Applies but may have reduced duration |
| **Lex Aeterna** (Priest) | Double damage on next hit | Works on bosses (consumed on hit) |
| **Ankle Snare** (Hunter) | Movement trap | Duration reduced to 1/5 on bosses |
| **Spider Web** (Sage) | Movement restriction | Works but reduced duration |
| **Provoke** | ATK up / DEF down | **Fails** on bosses (boss protocol blocks it) |
| **Steal** | Item theft | **Fails** on bosses |
| **Mental Sensing / Sense** | Info reveal | **Fails** on bosses |
| **Classical Pluck** (Bard) | Cast interruption | **Cannot stop** boss casting |
| **Spell Breaker** (Sage) | Cast cancellation | Success rate reduced to **10%** on bosses |

#### 3.2.3 Additional Boss Properties

- **Knockback immunity**: Arrow Repel, Storm Gust knockback, Bowling Bash chain splash, and all other displacement effects are nullified
- **Hidden player detection**: Bosses can see and target players using Hide, Cloak, Tunnel Drive, Chasewalk
- **Card targeting**: Abysmal Knight Card (+25% ATK vs boss-class), Alice Card (-40% damage from boss-class)
- **Drop rate**: Boss drops are NOT affected by server drop rate multipliers (fixed drop rates)

---

### 3.3 MVP Flag vs Boss Flag Distinction

| Property | Boss (Mini-Boss) | MVP |
|----------|-----------------|-----|
| **Boss Protocol** | Yes | Yes |
| **Status Immune** | Yes | Yes |
| **Knockback Immune** | Yes | Yes |
| **Detect Hidden** | Yes | Yes |
| **MVP Rewards (3 bonus drops)** | **No** | **Yes** |
| **MVP EXP bonus** | **No** | **Yes** |
| **Tombstone on death** | **No** | **Yes** |
| **FFA (Free-for-All) loot** | **No** (standard loot rules) | **Yes** (anyone can attack/loot) |
| **Spawn timer** | 10 min - 2 hours | 60 min - 12 hours |
| **Typical HP range** | 4,000 - 200,000 | 100,000 - 7,000,000 |
| **AI Type** | Varies (often 09, 04) | Almost always 21 (`0x3695`) |
| **Server announcement** | No | Yes (some servers broadcast MVP kills) |

**Mini-Boss examples**: Angeling (54), Deviling (62), Ghostring (60), Mastering (25), Tao Gunka (70), Eclipse, Vagabond Wolf, Eddga

**MVP examples**: Baphomet (81), Dark Lord (80), Osiris (78), Pharaoh (93), Valkyrie Randgris (99), Beelzebub (98)

*Note: "Mini-Boss" is a player-coined term. In the mob_db, they are simply monsters with `Class: Boss` but without the `MD_MVP` mode flag.*

---

### 3.4 Monster Modes (Behavior Flags)

Each monster has a hexadecimal mode bitmask that controls AI behavior. The flags are:

| Flag | Hex Bit | Description |
|------|---------|-------------|
| `MD_CANMOVE` | `0x0000001` | Monster can walk and chase |
| `MD_LOOTER` | `0x0000002` | Picks up items on ground during idle |
| `MD_AGGRESSIVE` | `0x0000004` | Actively scans for and attacks nearby players |
| `MD_ASSIST` | `0x0000008` | Joins fight when same-type ally is attacked nearby |
| `MD_CASTSENSORIDLE` | `0x0000010` | Aggros on players casting skills while mob is idle |
| `MD_NORANDOMWALK` | `0x0000020` | Stands still unless chasing (no random wander) |
| `MD_NOCAST` | `0x0000040` | Cannot use skills (melee auto-attack only) |
| `MD_CANATTACK` | `0x0000080` | Can perform attacks (without this, mob is passive like Pupa) |
| `MD_CASTSENSORCHASE` | `0x0000200` | Detects casters even while in chase state |
| `MD_CHANGECHASE` | `0x0000400` | Switches to closer target while chasing |
| `MD_ANGRY` | `0x0000800` | Hyper-active mode with extra pre-attack states |
| `MD_CHANGETARGETMELEE` | `0x0001000` | Switches target when hit by melee |
| `MD_CHANGETARGETCHASE` | `0x0002000` | Switches target when hit during chase |
| `MD_TARGETWEAK` | `0x0004000` | Only aggros players 5+ levels below monster |
| `MD_RANDOMTARGET` | `0x0008000` | Picks random target from attacker list each swing |
| `MD_IGNOREMELEE` | `0x0010000` | Immune to melee physical damage |
| `MD_IGNOREMAGIC` | `0x0020000` | Immune to magical damage |
| `MD_IGNORERANGED` | `0x0040000` | Immune to ranged physical damage |
| `MD_MVP` | `0x0080000` | MVP protocol (rewards, tombstone, special EXP) |
| `MD_IGNOREMISC` | `0x0100000` | Immune to misc/trap damage |
| `MD_KNOCKBACKIMMUNE` | `0x0200000` | Cannot be knocked back |
| `MD_TELEPORTBLOCK` | `0x0400000` | Cannot be teleported by skills |
| `MD_FIXEDITEMDROP` | `0x1000000` | Drop rates ignore server rate modifiers |
| `MD_DETECTOR` | `0x2000000` | Can detect hidden/cloaked players |
| `MD_STATUSIMMUNE` | `0x4000000` | Immune to all status effects |
| `MD_SKILLIMMUNE` | `0x8000000` | Immune to being affected by skills entirely |

**Common AI type -> Mode mappings**:

| AI Type | Hex Mode | Behavior | Typical Monsters |
|---------|----------|----------|-----------------|
| 01 | `0x0081` | Passive (move + attack, no aggro) | Fabre, Willow, Spore |
| 02 | `0x0083` | Passive + Looter | Poring, Poporing |
| 03 | `0x1089` | Passive + Assist + ChangeTargetMelee | Hornet, Wolf, Peco Peco |
| 04 | `0x3885` | Angry/Hyper-Active + Assist | Zombie, Orc Warrior, Mummy |
| 05 | `0x2085` | Aggressive + ChangeTargetChase | Archer Skeleton |
| 06 | `0x0000` | Immobile/Plant (no AI) | Pupa, Plant-type |
| 09 | `0x3095` | Aggressive + CastSensor + ChangeTarget | Scorpion, Isis, Side Winder |
| 10 | `0x0084` | Immobile + Aggressive (turret) | Mandragora, Flora, Geographer |
| 17 | `0x0091` | Passive + CastSensorIdle | Smokie, Golem, Bigfoot |
| 21 | `0x3695` | Boss/MVP (full target switching + cast sensor) | All MVPs |
| 24 | `0x00A1` | Slave (follows master, no wander) | Summoned slaves |

---

### 3.5 Monster Stats Formulas

#### 3.5.1 Monster ATK

Monster ATK is defined directly in the mob_db as a min-max range (e.g., `ATK: 7-10` for Poring). The actual damage per swing is:

```
MonsterDamage = random(ATK_min, ATK_max)
```

Unlike players, monsters do NOT have StatusATK / WeaponATK separation. Their ATK is a flat range defined by the database. Monster auto-attacks use the standard damage pipeline with this flat ATK value.

#### 3.5.2 Monster HIT (Pre-Renewal)

```
MonsterHIT = MonsterLevel + MonsterDEX
```

Source: rAthena `status.cpp` -- monsters use `level + dex` (no additional constants). Players use `175 + level + DEX + bonus` in pre-renewal.

#### 3.5.3 Monster FLEE (Pre-Renewal)

```
MonsterFLEE = MonsterLevel + MonsterAGI
```

Source: rAthena `status.cpp` -- monsters use `level + agi`. Players use `100 + level + AGI + bonus` in pre-renewal.

#### 3.5.4 Monster DEF (Hard DEF / Equipment DEF)

Monster Hard DEF is defined directly in the mob_db as a single integer. It functions identically to player Hard DEF as a percentage reduction:

```
DamageAfterHardDEF = floor(Damage * (100 - MonsterHardDEF) / 100)
```

Hard DEF is capped at 99 (99% reduction). Some monsters have very high Hard DEF (Red Plant = 100, which is effectively capped to 99).

#### 3.5.5 Monster Soft DEF (VIT-based DEF)

Monster Soft DEF uses a simpler formula than players:

```
MonsterSoftDEF = floor(MonsterVIT / 2)
```

This is a flat subtraction applied after Hard DEF. Players have a more complex formula involving `VIT/2 + max(VIT*0.3, VIT^2/150 - 1)`.

#### 3.5.6 Monster MDEF (Hard MDEF / Equipment MDEF)

Defined directly in mob_db. Applied identically to Hard DEF but for magical damage:

```
DamageAfterHardMDEF = floor(Damage * (100 - MonsterHardMDEF) / 100)
```

#### 3.5.7 Monster Soft MDEF (INT-based MDEF)

```
MonsterSoftMDEF = floor((MonsterINT + MonsterLevel) / 4)
```

Source: rAthena `status.cpp` -- monsters combine INT and level, then divide by 4. Players use `INT/2 + max(0, (INT*2-1)/4)`.

#### 3.5.8 Monster Critical Rate

Monsters have a base critical rate derived from LUK:

```
MonsterCRI = 1 + floor(MonsterLUK / 3)
```

---

### 3.6 Monster Element + Level

Every monster has an element type AND an element level (1-4). Higher element levels intensify elemental interactions:

**Element Types** (10 total):
| ID | Element |
|----|---------|
| 0 | Neutral |
| 1 | Water |
| 2 | Earth |
| 3 | Fire |
| 4 | Wind |
| 5 | Poison |
| 6 | Holy |
| 7 | Shadow (Dark) |
| 8 | Ghost |
| 9 | Undead |

**Element Levels** (1-4):
- Level 1: Standard elemental properties
- Level 2: Enhanced elemental reactions
- Level 3: Strong elemental affinity
- Level 4: Maximum elemental intensity

The element table is a 10x10x4 matrix. Each cell contains the damage percentage when element X attacks element Y at level Z. For example:
- Fire Lv1 attacking Water Lv1 = 50% damage (water resists fire)
- Fire Lv1 attacking Earth Lv1 = 150% damage (fire beats earth)
- Holy Lv1 attacking Shadow Lv4 = 200% damage (maximum holy vs dark)
- Neutral attacking Ghost Lv1 = 25% damage (neutral is weak vs ghost)

**Common monster element distributions**:
- Neutral: Most beginner and field monsters
- Undead: Undead-race monsters (but not always -- some undead-race are Shadow element)
- Earth: Many Plant-race and ground-dwelling monsters
- Water: Fish-race and aquatic monsters
- Fire: Some insects, volcanic monsters
- Shadow: Demons, dark creatures, thieves
- Holy: Angels (rare)
- Ghost: Ghosts, spirits (rare, often Demon race)
- Poison: Snakes, toxic creatures
- Wind: Flying insects, wind creatures

---

### 3.7 Monster Size Distribution by Race

General patterns (not absolute rules):

| Race | Typical Size | Exceptions |
|------|-------------|------------|
| Formless (0) | Medium | Clock (Medium), Alarm (Medium) |
| Undead (1) | Medium | Some Large (Dracula) |
| Brute (2) | Small-Medium | Peco Peco (Large), Grizzly (Large), Hatii (Large) |
| Plant (3) | Small-Medium | Mandragora (Medium, immobile) |
| Insect (4) | Small | Maya (Large), Golden Thief Bug (Large) |
| Fish (5) | Medium | Marse (Small) |
| Demon (6) | Small-Medium | Baphomet (Large), Dark Lord (Large) |
| Demi-Human (7) | Medium-Large | Raydric (Large), High Orc (Large) |
| Angel (8) | Small-Large | Angeling (Small), Valkyrie Randgris (Large) |
| Dragon (9) | Large | Petite (Small) |

---

## 4. Damage Modifier Stack Order

### 4.1 Pre-Renewal Physical Damage Pipeline (Complete Order)

The damage modifiers are applied in a specific order. The key principle is:
- **Within the same category**, bonuses are **additive** (e.g., two Hydra Cards = +40% vs Demi-Human)
- **Between different categories**, bonuses are **multiplicative** (e.g., Hydra + Skeleton Worker = 1.20 * 1.15 = 1.38x)

```
Step-by-step pipeline:

1.  Calculate StatusATK       -> STR + floor(STR/10)^2 + floor(DEX/5) + floor(LUK/3)
2.  Calculate WeaponATK       -> random(min, max) with DEX narrowing; crit = max
3.  Apply SIZE PENALTY        -> SizedWeaponATK = floor(WeaponATK * SizePenaltyPct / 100)
4.  Apply Damage Variance     -> +/- (WeaponLevel * 5)%
5.  Sum Base Damage           -> TotalATK = StatusATK + SizedWeaponATK + PassiveATK
6.  Apply Buff Multipliers    -> floor(TotalATK * BuffMult)  [Provoke, Power Thrust, EDP]
7.  Apply Skill Multiplier    -> floor(TotalATK * SkillMult / 100)
8.  Apply CARD BONUSES        -> Multiplicative between categories:
     a. RaceBonus   -> floor(TotalATK * (100 + raceBonus) / 100)
     b. SizeBonus   -> floor(TotalATK * (100 + sizeBonus) / 100)
     c. EleBonus    -> floor(TotalATK * (100 + eleBonus)  / 100)
     d. ClassBonus  -> floor(TotalATK * (100 + classBonus) / 100)  [boss/normal]
9.  Apply ELEMENT MODIFIER    -> floor(TotalATK * EleTable[atkEle][defEle][defEleLv] / 100)
10. Apply Hard DEF            -> floor(TotalATK * (100 - HardDEF) / 100)
11. Subtract Soft DEF         -> TotalATK = TotalATK - SoftDEF
12. Add Refinement ATK        -> TotalATK += refineATK + overUpgradeATK
13. Add MasteryATK            -> TotalATK += masteryATK  (flat, bypasses all multipliers)
14. Floor to minimum 1        -> FinalDamage = max(1, TotalATK)
```

### 4.2 Card Bonus Calculation (cardfix)

From rAthena source (pre-renewal):

```javascript
// rAthena cardfix: bonuses within same type ADD, between types MULTIPLY
cardfix = (100 + raceBonus) * (100 + eleBonus) * (100 + sizeBonus) / 10000;
// Then: damage = floor(damage * cardfix / 100)
```

**Example**: Player with 2x Hydra (+40% vs Demi-Human) + 1x Skeleton Worker (+15% vs Medium) + 1x Vadon (+20% vs Fire) attacking a Fire Medium Demi-Human:

```
Race:    100 + 40 = 140%
Size:    100 + 15 = 115%
Element: 100 + 20 = 120%
Total:   140 * 115 * 120 / 10000 = 193.2% (floored per step)
```

### 4.3 Defensive Card Modifier Order

Defensive reductions are also applied multiplicatively between categories, in this specific order:

```
1. Size Reduction     -> floor(damage * (100 - sizeRed) / 100)
2. Race Reduction     -> floor(damage * (100 - raceRed) / 100)
3. Class Reduction    -> floor(damage * (100 - classRed) / 100)   [boss/normal]
4. Property Reduction -> floor(damage * (100 - eleRed) / 100)
```

**Example**: Player with Thara Frog Shield (-30% vs Demi-Human) + Raydric Garment (-20% vs Neutral element) being attacked by a Neutral Demi-Human:

```
Race:    100 - 30 = 70%
Element: 100 - 20 = 80%
Total:   70 * 80 / 100 = 56% of original damage taken
```

### 4.4 Skills That Bypass Certain Modifiers

Some skills skip parts of the standard damage pipeline:

| Skill | Bypasses |
|-------|----------|
| **Grand Cross** | Does not use ATK% cards; custom WeaponATK-only formula |
| **Acid Terror** | Ignores ATK%; own formula (ignores Hard DEF) |
| **Investigate** (Occult Impaction) | Converts DEF to damage; ignores size penalty |
| **Asura Strike** | Fixed formula; not subject to standard ATK% or size penalty |
| **Shield Boomerang** | Custom damage (batk + weight + refine); always Neutral |
| **Shield Chain** | Not subject to ATK% |
| **Dragon Breath** | Not subject to ATK% |

---

## 5. Implementation Checklist

### 5.1 Size System

- [x] SIZE_PENALTY lookup table with all 18 weapon types (implemented in `ro_damage_formulas.js`)
- [x] `getSizePenalty(weaponType, targetSize)` function
- [x] Size penalty applied only to WeaponATK portion
- [x] Mounted spear override (Medium: 75% -> 100%)
- [x] `noSizePenalty` bypass (Drake Card `cardNoSizeFix` + Maximize Power buff)
- [x] Monster templates include `size` field (small/medium/large)
- [x] Size-specific card bonuses applied in damage pipeline (`sizeBonus`)
- [x] Size-specific defensive reductions applied (`sizeRed`)
- [ ] Gun weapon type in SIZE_PENALTY table (present but verify usage)
- [ ] Huuma Shuriken weapon type (present but Ninja class not implemented)

### 5.2 Race System

- [x] Monster templates include `race` field
- [x] 10 race types used across monster templates (formless, undead, brute, plant, insect, fish, demon, demi-human, angel, dragon)
- [x] Race-specific card bonuses applied in damage pipeline (`raceBonus`)
- [x] Race-specific defensive reductions applied (`raceRed`)
- [x] Player characters treated as Demi-Human (Race 7)
- [x] Abysmal Knight Card (+25% vs Boss class) implemented
- [x] Alice Card (-40% from Boss class) implemented
- [x] Card bonus stacking: additive within category, multiplicative between categories

### 5.3 Monster Properties

- [x] `monsterClass` field on templates (normal/boss/mvp)
- [x] `modeFlags` parsed from hex mode bitmask
- [x] Boss Protocol: `knockbackImmune`, `statusImmune`, `detector` flags
- [x] Status immunity check (`modeFlags.statusImmune`) in `applyStatusEffect()`
- [x] Knockback immunity check in `knockbackTarget()`
- [x] MVP flag (`MD_MVP`) for MVP reward distribution
- [x] Monster element + level stored in templates
- [x] Monster DEF (hardDEF) applied as percentage reduction
- [x] Monster MDEF (hardMDEF) applied as percentage reduction
- [x] AI type codes mapped to behavior patterns
- [x] Aggressive, Assist, Looter, CastSensor modes implemented
- [x] Monster HIT = Level + DEX
- [x] Monster FLEE = Level + AGI
- [ ] Monster Soft DEF formula (VIT/2) -- verify implementation matches pre-renewal
- [ ] Monster Soft MDEF formula ((INT+Level)/4) -- verify implementation
- [ ] `MD_IGNOREMELEE`, `MD_IGNOREMAGIC`, `MD_IGNORERANGED`, `MD_IGNOREMISC` -- verify implementation
- [ ] `MD_FIXEDITEMDROP` -- drops ignore server rate multipliers
- [ ] `MD_SKILLIMMUNE` -- full skill immunity (rare flag)

### 5.4 Damage Pipeline

- [x] Size penalty before element/card modifiers
- [x] Card bonuses multiplicative between categories (race * size * element * class)
- [x] Element modifier from 10x10x4 element table
- [x] Hard DEF percentage reduction
- [x] Soft DEF flat subtraction
- [x] Refinement ATK added post-DEF
- [x] Mastery ATK added last (bypasses all multipliers)

---

## 6. Gap Analysis

### 6.1 Current Implementation Status (Sabri_MMO)

Based on code inspection of `server/src/ro_damage_formulas.js` and `server/src/index.js`:

**Fully Implemented**:
- SIZE_PENALTY table with 18 weapon types + default fallback
- `getSizePenalty()` with mounted spear override
- `cardNoSizeFix` (Drake Card) and `noSizePenalty` (Maximize Power) bypass
- Monster templates with `size`, `race`, `element`, `monsterClass` fields
- Card bonus pipeline: race, element, size bonuses applied multiplicatively
- Boss protocol mode flags (`knockbackImmune`, `statusImmune`, `detector`)
- Monster HIT/FLEE formulas
- Element table (10x10x4)
- Full damage pipeline order

**Partially Implemented / Needs Verification**:

| Item | Status | Action Needed |
|------|--------|---------------|
| Monster Soft DEF | Implemented but formula may differ | Verify `VIT/2` is used for monsters (not player formula) |
| Monster Soft MDEF | Implemented but formula may differ | Verify `(INT+Level)/4` is used for monsters |
| `MD_IGNOREMELEE/MAGIC/RANGED/MISC` | Possibly missing | Check if damage type immunity flags are enforced |
| `MD_FIXEDITEMDROP` | Unknown | Verify drop rate server multipliers respect this flag |
| Quagmire on bosses | Implemented | Verify 50% stat reduction cap on bosses |
| Ankle Snare duration on bosses | Implemented | Verify 1/5 duration factor |
| Spell Breaker 10% on bosses | Implemented | Verify reduced success rate |
| Size distribution accuracy | Partially | Verify all 509 monster templates have correct sizes vs rAthena mob_db |
| Race distribution accuracy | Partially | Verify all templates have correct races vs rAthena mob_db |

### 6.2 Not Yet Needed (Future Classes/Systems)

| Feature | Dependency | Priority |
|---------|-----------|----------|
| Gun weapon type damage | Gunslinger class (not planned) | N/A |
| Huuma Shuriken damage | Ninja class (not planned) | N/A |
| Player size changes | Baby/Adopted character system | Low |
| `MD_TELEPORTBLOCK` | Teleport-targeting skills | Low |
| `MD_SKILLIMMUNE` | Rare MVP flag | Low |

### 6.3 Recommended Audit Actions

1. **Verify monster softDEF/softMDEF formulas** in `ro_damage_formulas.js` match the pre-renewal formulas documented above (VIT/2 for DEF, (INT+Level)/4 for MDEF)
2. **Spot-check 20 monster templates** against rAthena's `db/pre-re/mob_db.yml` to confirm size, race, and element accuracy
3. **Test Quagmire on boss** to confirm 50% stat reduction cap is enforced
4. **Test Ankle Snare on boss** to confirm 1/5 duration
5. **Verify `MD_IGNOREMELEE/MAGIC/RANGED/MISC`** flags are checked in damage paths (these affect rare monsters like Ghostring)
6. **Verify card bonus order** matches rAthena: race -> size -> element -> class (within offensive), and size -> race -> class -> element (within defensive)

---

## Sources

- [iRO Wiki Classic - Size](https://irowiki.org/classic/Size)
- [iRO Wiki Classic - Boss Protocol](https://irowiki.org/classic/Boss_Protocol)
- [iRO Wiki - ATK (Damage Formula)](https://irowiki.org/wiki/ATK)
- [iRO Wiki - DEF](https://irowiki.org/classic/DEF)
- [iRO Wiki - Race](https://irowiki.org/wiki/Race)
- [RateMyServer - Weapon Type vs Monster Size Table](https://ratemyserver.net/index.php?page=misc_table_size)
- [RateMyServer - Pre-Renewal Monster Database](https://ratemyserver.net/index.php?page=mob_db)
- [rAthena - mob_db.txt Documentation](https://github.com/rathena/rathena/blob/master/doc/mob_db.txt)
- [rAthena - mob_db_mode_list.txt](https://github.com/rathena/rathena/blob/master/doc/mob_db_mode_list.txt)
- [rAthena - Monster Soft MDEF Issue #4185](https://github.com/rathena/rathena/issues/4185)
- [rAthena - Pre-Re SoftDEF Formula PR #6766](https://github.com/rathena/rathena/pull/6766)
- [rAthena - Hit and Flee Pre-Renewal Discussion](https://rathena.org/board/topic/133367-hit-and-flee-in-pre-renewal/)
- [rAthena - Pre-Renewal Damage Rework Issue #8193](https://github.com/rathena/rathena/issues/8193)
- [NovaRO Wiki - Size Penalty](https://www.novaragnarok.com/wiki/Size_Penalty)
- [MuhRO Wiki - Weapon Size Modifiers](https://wiki.muhro.eu/Weapon_Size_Modifiers)
