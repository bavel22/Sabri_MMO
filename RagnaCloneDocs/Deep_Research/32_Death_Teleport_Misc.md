# Death, Teleportation & Misc -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-Renewal) death mechanics, resurrection, teleportation, weight system, auto-attack, and miscellaneous systems. No Renewal-era changes.
> **Sources**: iRO Wiki Classic, RateMyServer (pre-re), rAthena source (pre-re branch, `pc.cpp`, `status.cpp`, `exp.conf`), Hercules source, StrategyWiki, divine-pride.net, Ragnarok Fandom Wiki, GameFAQs, WarpPortal Forums, ROGGH Library.
> **Purpose**: Authoritative reference for implementing death, resurrection, teleportation, weight, auto-attack, and miscellaneous systems in Sabri_MMO.

---

## Table of Contents

1. [Death System](#1-death-system)
2. [Resurrection](#2-resurrection)
3. [Teleportation](#3-teleportation)
4. [Weight System](#4-weight-system)
5. [Auto-Attack System](#5-auto-attack-system)
6. [Miscellaneous Systems](#6-miscellaneous-systems)
7. [Implementation Checklist](#7-implementation-checklist)
8. [Gap Analysis](#8-gap-analysis)

---

## 1. Death System

### 1.1 Death Consequences (What Happens When You Die)

When a character's HP reaches 0, the following sequence occurs:

1. **Fall animation**: The character sprite plays a "collapse" animation and falls to the ground
2. **Lie on ground**: The character remains lying flat on the ground in a prone position. The sprite is rendered translucent/ghostly
3. **Death UI appears**: A dialog box appears with two options:
   - **"Return to Save Point"** -- Immediately respawn at the character's saved Kafra save point
   - **"Wait here"** -- Stay dead on the ground, waiting for a Priest to cast Resurrection or another player to use Yggdrasil Leaf. The Token of Siegfried auto-triggers if present in inventory
4. **EXP penalty applied**: Base and Job EXP are deducted at the moment of death (not on respawn)
5. **Buffs cleared**: Most active buffs are removed on death (see buff survival list below)
6. **Auto-attack canceled**: All attack states and skill states are cleared
7. **Movement stopped**: Character cannot move, attack, use skills, or pick up items while dead

**While dead, the character can:**
- View the game world (camera remains)
- Open chat window and type messages
- Wait indefinitely for resurrection
- Click "Return to Save Point" at any time

**While dead, the character cannot:**
- Move, attack, or use any skills
- Pick up items
- Open inventory, equipment, or other windows (varies by client)
- Be targeted by offensive skills (dead = untargetable by enemies)

### 1.2 Death Penalty Formula (EXP Loss)

The pre-renewal death penalty is configured in rAthena's `conf/battle/exp.conf`:

```
death_penalty_type: 1    // 1 = Lose % of current level's required EXP
death_penalty_base: 100  // 100 = 1% (each 100 units = 1%)
death_penalty_job:  100  // 100 = 1% (each 100 units = 1%)
```

**Official iRO/kRO pre-renewal formula:**

```
Base EXP Lost = floor(EXP_needed_for_next_base_level * 0.01)
Job EXP Lost  = floor(EXP_needed_for_next_job_level * 0.01)
```

This is **1% of the EXP required for the NEXT level**, not 1% of total accumulated EXP.

**Example:**
- Character is Base Level 50, needing 200,000 EXP to reach Level 51
- Character has earned 150,000 / 200,000 EXP toward Level 51
- On death: loses `floor(200,000 * 0.01)` = **2,000 Base EXP**
- New EXP: 148,000 / 200,000
- EXP cannot go below 0 for the current level (you cannot de-level from death)

**The same formula applies to Job EXP:**
- 1% of the Job EXP required for the next Job Level

### 1.3 Death Penalty Exemptions

The following classes and conditions are **exempt** from death penalty:

| Condition | Base EXP Penalty | Job EXP Penalty | Source |
|-----------|-----------------|-----------------|--------|
| **Novice class** (Job ID = Novice) | **0%** (exempt) | **0%** (exempt) | rAthena: `MAPID_NOVICE` check in `pc_dead()` |
| **Baby classes** (all baby variants) | **Halved** (0.5%) | **Halved** (0.5%) | rAthena: `SC_BABY` status halves penalty |
| **Max Base Level** (99 or 99/70) | **0%** (no penalty) | Normal | rAthena: `death_penalty_maxlv: 0` (official default) |
| **Max Job Level** (50/70) | Normal | **0%** (no penalty) | Same — max job level has no next-level EXP |
| **PvP arena death** | **0%** | **0%** | Maps with `pvp` mapflag + `noexppenalty` |
| **WoE death** | **0%** | **0%** | Maps with `gvg` mapflag + `noexppenalty` |
| **Town maps** | **0%** | **0%** | Maps with `town` + `noexppenalty` mapflags |
| **Quest maps** (select few) | **0%** | **0%** | Specific maps with `noexppenalty` mapflag |
| **Token of Siegfried** | **0%** | **0%** | Self-rez consumes token, no penalty |
| **Life Insurance** (`SC_LIFEINSURANCE`) | **0%** | **0%** | rAthena: checked in `pc_dead()` |
| **Kaizel buff** (High Priest) | **0%** | **0%** | Auto-resurrects with no penalty |

### 1.4 Super Novice Guardian Angel ("Never Give Up")

Super Novices have a unique death-save mechanic:

**Trigger conditions:**
- Character is Super Novice class
- Base EXP progress is between **99.0% and 99.9%** of the current level
- HP reaches 0 (would normally die)

**When triggered:**
1. A Guardian Angel appears above the character
2. Character's HP is **fully restored** to max
3. **Mental Strength** (Steel Body) buff is cast on the character
4. Character does **NOT** die -- continues playing normally

**Limitations:**
- Can only trigger **once** per level gained or per relog
- After triggering, will not activate again until the character gains a level or relogs
- The 1% EXP penalty from the "death" is NOT applied (death was prevented)
- Does NOT work if EXP is below 99.0% or at exactly 100%

### 1.5 PvP Death Rules

Deaths in PvP-flagged maps follow special rules:

- **No EXP penalty**: PvP maps have `noexppenalty` mapflag
- **No Job EXP penalty**: Same
- **Zeny penalty** (optional): Configurable via `zeny_penalty` in `exp.conf`. Official default: `0` (no zeny loss). When enabled, it is a percentage of total zeny, applied only on PvP maps
- **Item drop** (optional, Nightmare mode): Via `pvp_nightmaredrop` mapflag, items can be configured to drop from inventory/equipment on death. Syntax: `pvp_nightmaredrop itemID,type,percent` where type 0 = inventory, 1 = equip, 2 = both. Standard PvP does NOT drop items

**PvP respawn:**
- Dead players can respawn at save point or wait for resurrection
- Some PvP maps have special respawn points (arena warps)

### 1.6 WoE Death Rules

Deaths during War of Emperium follow castle/siege rules:

- **No EXP penalty**: GvG maps have `noexppenalty`
- **No item drop**: No nightmare mode in standard WoE
- **Respawn**: Players respawn at their save point (typically the town outside the castle)
- **No Token of Siegfried**: Tokens are blocked in WoE maps (cannot self-resurrect)
- **Resurrection skills**: Priest Resurrection and Yggdrasil Leaf work normally in WoE
- **Barricade death**: Dying to castle guardians or Emperium defense follows same rules

### 1.7 Item Drop on Death

In standard pre-renewal PvE:
- **Players do NOT drop items on death** -- this is not a mechanic in normal gameplay
- Items only drop on death in **Nightmare mode** maps (`pvp_nightmaredrop` mapflag)
- This is a server configuration, not a core game mechanic

**Nightmare mode configuration (rAthena):**
```
pvp_nightmaredrop <item_id>,<type>,<percent>
// type: 0 = random inventory item, 1 = random equipped item, 2 = both
// percent: out of 10000 (10000 = 100%)
```

### 1.8 Respawn Mechanics

**Save Point system:**
- Every character has a **save point** (saved respawn location)
- Save points are set by talking to **Kafra NPCs** and selecting "Save" (free service)
- The save point stores: map name, X coordinate, Y coordinate
- Default save point for new characters: Prontera (varies by server)

**Respawn flow:**
1. Character dies -> death UI appears
2. Player clicks "Return to Save Point" (or timeout/auto-respawn on some servers)
3. Character is warped to their save point map at the saved coordinates
4. Character respawns with:
   - **HP**: Varies by server configuration (typically 50% or 100% of max HP)
   - **SP**: Varies (typically 50% or 100% of max SP)
   - EXP penalty has already been applied at the moment of death
5. Character gains brief **invulnerability** (~5 seconds after spawn, no damage taken)

**@return command (GM/server-specific):**
- The `@return` command warps the player to their save point without dying
- This is a GM command, not available to normal players in official servers
- Some private servers grant this to all players

### 1.9 Buffs Cleared on Death

Most buffs are removed on death. The following buffs **survive** death (rAthena `NoRemoveOnDead` flag):

| Buff | Survives Death | Notes |
|------|---------------|-------|
| Auto Berserk (`SC_AUTOBERSERK`) | Yes | Passive toggle |
| Endure (`SC_ENDURE`) | Yes | Persists through death |
| Shrink (`SC_SHRINK`) | Yes | Passive toggle |
| Songs/Dances (performer's own) | Yes | Performance state persists |
| Kaizel (`SC_KAIZEL`) | Yes | Must survive to auto-rez |
| **All other buffs** | **No** | Cleared on death |

**Cleared on death includes:**
- Blessing, Increase AGI, Angelus, Magnificat
- Two-Hand Quicken, Adrenaline Rush, Weapon Perfection
- All potions (ASPD, stat foods, elemental converters)
- Endow effects (Fire/Water/Wind/Earth Weapon)
- Providence, Kyrie Eleison, Assumptio
- Steel Body, Maximize Power
- All debuffs (Poison, Curse, Silence, etc.)

---

## 2. Resurrection

### 2.1 Resurrection Skill (Priest / High Priest)

**Skill Type**: Supportive (2nd class)
**Max Level**: 4
**SP Cost**: 60 (all levels)
**Catalyst**: 1 Blue Gemstone per use (consumed)
**Target**: Dead players, or Undead-property monsters (non-boss)

**Prerequisites**: Increase SP Recovery Lv4, Status Recovery Lv1

| Level | Cast Time | Skill Delay | HP Restored |
|-------|-----------|-------------|-------------|
| 1 | 6.0 sec | 0 sec | 10% Max HP |
| 2 | 4.0 sec | 1.0 sec | 30% Max HP |
| 3 | 2.0 sec | 2.0 sec | 50% Max HP |
| 4 | 0 sec (instant) | 3.0 sec | 80% Max HP |

**Mechanics:**
- Can only target dead (lying on ground) player characters
- The resurrected player appears standing at their death location
- SP is restored to 0 on resurrection (player must regen SP naturally)
- EXP penalty was already applied at death -- resurrection does NOT refund lost EXP
- Interruptible by damage (cast time can be disrupted if the Priest is hit)
- DEX reduces cast time normally
- When used on Undead-property monsters (non-boss), functions identically to Turn Undead

**Special notes:**
- Cannot resurrect self
- Cannot be used on alive targets
- Blue Gemstone is consumed even if the target clicks "Return to Save Point" during cast
- Works in PvP/WoE maps

### 2.2 Yggdrasil Leaf

**Item ID**: 610 (`Leaf_Of_Yggdrasil`)
**Type**: Delayed-Consumable
**Weight**: 10
**Buy Price**: 4,000z / **Sell**: 2,000z

**Mechanics:**
- Casts **Resurrection Level 1** on a dead player target
- Restores **10% Max HP** (same as Resurrection Lv1)
- Restores 0 SP
- **Consumed on use** (1 leaf per resurrection)
- Does NOT require Blue Gemstone
- **Any class can use it** (not restricted to Priests)
- Must target a dead player character (click on their corpse)
- Has a **use delay** (delayed-consumable type -- cannot spam)
- Does NOT refund EXP lost from death

**Drop sources (notable):**
- Golden Savage: 30%
- Archangeling: 18%
- Wander Man: 6.5%
- Angeling: 5%
- Also from Old Blue Box, Old Violet Box random pools

**Restrictions:**
- Cannot use on self (must target another dead player)
- Cannot use at 90%+ weight (major overweight blocks all item use)
- Some maps may restrict item use

### 2.3 Token of Siegfried

**Item ID**: 7621 (`Token_Of_Siegfried`)
**Type**: Miscellaneous (special consumable)
**Weight**: 1
**Buy Price**: 2z / **Sell**: 1z

**Mechanics:**
- **Self-resurrection item** -- works on the holder when they die
- When the character holding a Token dies, a resurrection dialog appears
- Clicking "Resurrect" consumes the Token and revives the character **on the spot**
- Restores **100% HP and 100% SP**
- **No EXP penalty** is applied (death penalty is waived)
- The Token is deleted from inventory upon use

**Activation flow (rAthena `CZ_STANDING_RESURRECTION`):**
1. Player dies while holding Token of Siegfried in inventory
2. Death UI shows an additional option to self-resurrect
3. Player clicks the resurrection option
4. Server checks inventory for Token (item ID 7621)
5. Server checks for blocking effects (e.g., `SC_HELLPOWER` prevents resurrection)
6. If valid: `status_revive()` is called, Token is deleted
7. Character stands up at death location with 100% HP/SP

**Restrictions:**
- One Token is consumed per death
- Cannot be used if `SC_HELLPOWER` is active (blocks all resurrection)
- Blocked in some WoE maps (server-configurable)
- The item itself is rare -- not dropped by any monster in pre-renewal; obtained from events, quests, or cash shop

### 2.4 Redemptio (Priest Quest Skill)

**Skill Type**: Supportive (Quest Skill)
**Levels**: 1 (fixed)
**SP Cost**: 400
**Cast Time**: 4.0 seconds (**uninterruptible** and **unreducible** by DEX)
**Target**: Self (AoE centered on caster)
**AoE**: 15 x 15 cells

**Prerequisites**: Priest Skill Quest (bring 30 Holy Water + 20 Blue Gemstones to Sister Linus in Prontera Church)

**Mechanics:**
- The caster **sacrifices themselves** to revive all dead party members within the 15x15 area
- All revived party members receive **50% Max HP** restoration
- The caster's HP and SP are reduced to **1** (not killed -- they survive with 1 HP/SP)
- The caster loses **1% of their base EXP** needed for the next level

**EXP cost reduction per person resurrected:**
```
EXP_cost = max(0, 1% * NextLevelEXP - (numResurrected * 0.2% * NextLevelEXP))
```
- Each person resurrected reduces the caster's EXP penalty by 0.2% (200,000 EXP at Lv99)
- With 5+ people resurrected, the EXP penalty is **0** (fully offset)

**At Level 99:**
- NextLevelEXP ~ 100,000,000 (99,999,999)
- 1% = ~1,000,000 EXP
- Each resurrection reduces cost by ~200,000 EXP
- 5 resurrections = 1,000,000 - 1,000,000 = 0 EXP cost

**Restrictions:**
- Caster must have sufficient EXP to pay the cost (skill fails if not enough)
- Uninterruptible -- cannot be canceled by damage during the 4-second cast
- Cast time cannot be reduced by DEX (fixed 4 seconds)
- Only affects **party members** (not guild members or random players)
- Does NOT consume Blue Gemstones or other items

### 2.5 Kaizel (High Priest Skill)

**Note**: While primarily a High Priest (Transcendent) skill, Kaizel is relevant to death mechanics:

- Kaizel is a buff that auto-resurrects the target when they die
- When the buffed target's HP reaches 0, they are immediately resurrected with a percentage of HP
- **No EXP penalty** is applied (death penalty waived)
- The buff is consumed on trigger
- The resurrected character gains brief invulnerability (~3 seconds)

---

## 3. Teleportation

### 3.1 Teleport Skill (Acolyte)

**Skill Type**: Supportive
**Max Level**: 2
**SP Cost**: `11 - SkillLevel` (Lv1 = 10 SP, Lv2 = 9 SP)
**Cast Time**: None (instant)
**Cast Delay**: ASPD-based
**Target**: Self (ground-targeted internally)
**Prerequisite**: Ruwach Lv1

| Level | Effect |
|-------|--------|
| 1 | **Random teleport** -- warp to a random walkable cell on the same map |
| 2 | **Save point OR random** -- opens a menu: "Random" (same as Lv1) or "Save Point" (warp to save point) |

**Restrictions:**
- Disabled on maps with `noteleport` mapflag
- **Disabled in PvP and WoE** maps
- Cannot be used while standing on **Magnetic Earth** (Land Protector) -- the skill is technically ground-targeted
- Disabled on specific free-to-play servers (pRO, idRO, bRO removed it; some servers increase SP cost to 100-500)
- Cannot be used while under certain status effects (Stun, Freeze, etc.)

**Items that grant Teleport:**
- **Creamy Card** (compounded in accessory): Grants Teleport Lv1 (random teleport) as a usable skill
- **Fly Wing** (item 601): Uses Teleport Lv1 effect (random teleport)
- **Butterfly Wing** (item 602): Uses Teleport Lv3 effect (save point warp) -- note: internally `AL_TELEPORT` Lv3 = always save point, no menu

### 3.2 Fly Wing (Random Teleport Item)

**Item ID**: 601 (`Wing_Of_Fly`)
**Type**: Delayed-Consumable
**Weight**: 5
**Buy Price**: 60z / **Sell**: 30z

**Mechanics:**
- Instantly teleports the user to a **random walkable cell** on the same map
- Equivalent to casting Teleport Lv1
- **Consumed on use** (1 per teleport)
- Has a **use delay** (cannot be spammed rapidly -- "delayed-consumable" type)
- Can be used during skill cast (interrupts the cast but teleports)
- **Any class** can use it

**After teleport:**
- Character appears at random location with brief invulnerability (~3-5 seconds)
- Exception: if teleported to a cell near a monster that was already attacking you, the invulnerability may not apply (known quirk)

**Restrictions (maps where Fly Wing does NOT work):**
- Maps with `noteleport` mapflag (e.g., some dungeons, boss rooms)
- WoE castle maps during siege
- Some quest-instance maps
- Maps with `noreturn` mapflag block Butterfly Wing but NOT Fly Wing (Fly Wing is random same-map, not return)

**Related items:**
- **Giant Fly Wing** (item 12212): Party version. When used by the **party leader**, teleports ALL party members on the same map to the leader's random destination. Weight: 1. Only the party leader can activate it.
- **Novice Fly Wing**: Same as Fly Wing but obtained from tutorials

### 3.3 Butterfly Wing (Return to Save Point Item)

**Item ID**: 602 (`Wing_Of_Butterfly`)
**Type**: Delayed-Consumable
**Weight**: 5
**Buy Price**: 300z / **Sell**: 150z

**Mechanics:**
- Instantly teleports the user to their **save point**
- Equivalent to casting Teleport Lv2 and selecting "Save Point" (internally `AL_TELEPORT` Lv3)
- **Consumed on use** (1 per teleport)
- Has a **use delay** (delayed-consumable)
- **Any class** can use it
- Effectively a "town portal" item

**Restrictions:**
- Maps with `noreturn` mapflag block Butterfly Wing use
- Maps with `noteleport` mapflag also block it
- Cannot use at 90%+ weight
- Some WoE/PvP maps restrict it

**Drop sources (notable):**
- Wraith: 13%
- Bloody Butterfly: 12%
- Brilight: 10%

### 3.4 Warp Portal (Acolyte Skill)

**Skill Type**: Supportive
**Max Level**: 4
**SP Cost**: `38 - (SkillLevel * 3)` (Lv1 = 35, Lv2 = 32, Lv3 = 29, Lv4 = 26)
**Cast Time**: ~1.0 second
**Catalyst**: 1 Blue Gemstone per portal (consumed)
**Target**: Ground
**Prerequisite**: Teleport Lv2

| Level | Duration | Available Destinations |
|-------|----------|----------------------|
| 1 | 10 sec | Save Point only |
| 2 | 15 sec | Save Point + 1 Memo Point |
| 3 | 20 sec | Save Point + 2 Memo Points |
| 4 | 25 sec | Save Point + 3 Memo Points |

**Portal mechanics:**
- Creates a blue swirling portal on the ground at the target location
- Any player (not just party members) can enter the portal by stepping on it
- Maximum **8 players** can use a single portal (regardless of skill level)
- Portal closes when: (a) duration expires, (b) 8 players have entered, or (c) caster leaves the map/disconnects
- Maximum **3 portals** can be active simultaneously from one caster

**Memo system (`/memo` command):**
- Players memorize map locations using the `/memo` command (or `/memo1`, `/memo2`, `/memo3`)
- Maximum 3 memo slots available at Warp Portal Lv4
- The number of usable memo slots = `SkillLevel - 1` (Lv1 = 0 memos, Lv4 = 3 memos)
- Each memo stores: map name only (coordinates are fixed to the spot where `/memo` was used, but warp lands at the map's designated warp-in point)
- Typing `/memo` when all slots are full overwrites the **oldest** (bottom of list) memo
- Each map can only be memorized once (re-memoing the same map updates the existing entry)
- Memo points persist across logins (saved to database)

**Restrictions:**
- Maps with `nomemo` mapflag cannot be memorized
- Maps with `nowarp` mapflag cannot be warped FROM (portal creation blocked)
- Maps with `nowarpto` mapflag cannot be warped TO (destination blocked)
- Blue Gemstone is consumed even if the portal is not entered
- The caster must be alive to maintain the portal (dying closes all portals)

**Destination selection UI:**
1. After casting on the ground, a list appears showing available destinations
2. First option is always "Save Point"
3. Additional options are the memorized maps (up to 3)
4. Selecting a destination creates the portal immediately

### 3.5 Kafra Teleport Service

Kafra NPCs in towns offer paid teleportation to other cities:

**Mechanics:**
- Talk to a Kafra NPC and select "Teleport Service"
- A list of available destinations appears with zeny costs
- Select a destination, pay the fee, and instantly warp there
- Available destinations vary by which Kafra NPC you talk to

**Typical Kafra teleport costs (pre-renewal, approximate):**

| From | To | Cost (Zeny) |
|------|----|-------------|
| Prontera | Izlude | 600 |
| Prontera | Geffen | 1,200 |
| Prontera | Payon | 1,200 |
| Prontera | Morroc | 1,200 |
| Prontera | Alberta | 1,800 |
| Prontera | Al De Baran | 1,800 |
| Al De Baran | Juno | 1,800 |
| Geffen | Orc Village | 1,200 |
| Payon | Payon Cave | 600 |

**Notes:**
- Costs vary by server and Kafra location
- **Free Ticket for Kafra Transportation** item negates one teleport cost
- Kafra Points: Every 10 zeny spent on services earns 1 Kafra Point (redeemable in Al De Baran)
- Kafra employees are found in most towns except Hugel, Rachel, and Veins (those use Cool Event Corp.)
- Cat Hand NPCs handle Jotunheim area (Manuk, Splendide)

**Save Point service (Kafra):**
- Free service -- no cost
- Sets the character's respawn point to near the Kafra NPC
- Can be used unlimited times

### 3.6 Map Restriction Flags (Mapflags)

Maps in RO can have various restriction flags that affect teleportation and death:

| Mapflag | Effect |
|---------|--------|
| `noteleport` | Blocks Teleport skill, Fly Wing, and warp-type commands. Cannot teleport out randomly. |
| `noreturn` | Blocks Butterfly Wing, `@return`, and party/guild warp commands. Cannot return to save point via items. |
| `nowarp` | Blocks `@go` and warp commands from this map. GM-level restricted. |
| `nowarpto` | Blocks incoming `@warp` to this map. Cannot warp TO this map. |
| `nomemo` | Blocks `/memo` command. Cannot memorize this map for Warp Portal. |
| `nosave` | Cannot save respawn point here. If character logs off, they return to their previous save point. |
| `noexppenalty` | No EXP loss on death in this map. |
| `nozenypenalty` | No zeny loss on death in this map. |
| `nopenalty` | Combines `noexppenalty` + `nozenypenalty`. |
| `town` | Designates map as a town (safe zone). Mail access, etc. Usually paired with `noexppenalty`. |
| `pvp` | Enables PvP combat. Players can attack each other. |
| `gvg` | Enables Guild vs Guild mode. Guild icons shown, inter-guild attacks. |
| `pvp_nightmaredrop` | Items drop from inventory/equipment on PvP death. Syntax: `id,type,percent`. |
| `nobranch` | Blocks Dead Branch, Bloody Branch, Poring Box, Red Pouch usage. |

**Common map restriction combinations:**
- **Dungeons**: Often `noteleport` + `noreturn` (trap the player, must walk out)
- **Boss rooms**: `noteleport` + `nobranch` + sometimes `nomemo`
- **Towns**: `town` + `noexppenalty` + `nozenypenalty` + `nobranch`
- **WoE castles**: `gvg` + `noexppenalty` + `noreturn` + `noteleport`
- **PvP arenas**: `pvp` + `noexppenalty` + `nozenypenalty`
- **Quest maps**: Often `nosave` + `nomemo`

---

## 4. Weight System

### 4.1 Weight Limit Formula

```
MaxWeight = 2000 + (BaseSTR * 30) + JobBonus + SkillBonus + EquipBonus
```

**Base weight**: 2000 (all characters start with this)
**STR contribution**: +30 weight capacity per point of base STR

**Job-specific weight bonuses:**

| Job Class | Weight Bonus |
|-----------|-------------|
| Novice | 0 |
| Swordsman | +800 |
| Mage | +200 |
| Archer | +400 |
| Merchant | +800 |
| Thief | +400 |
| Acolyte | +400 |
| Knight | +800 |
| Crusader | +800 |
| Wizard | +200 |
| Sage | +400 |
| Hunter | +400 |
| Bard/Dancer | +400 |
| Blacksmith | +1000 |
| Alchemist | +1000 |
| Assassin | +400 |
| Rogue | +400 |
| Priest | +400 |
| Monk | +400 |
| Super Novice | +400 |

**Skill bonuses:**
- **Enlarge Weight Limit** (Merchant skill, Lv1-10): +200 capacity per level (+2000 at Lv10)
- **Gym Pass** (Kafra Shop item): +200 capacity per level (stacks with Enlarge Weight Limit)

**Equipment bonuses:**
- **Peco Peco Ride** (Knight/Crusader mount): +1000 weight capacity while mounted
- Some equipment gives weight capacity bonuses (rare)

**Example calculation:**
- Blacksmith, 80 STR, Enlarge Weight Limit Lv10, no mount
- MaxWeight = 2000 + (80 * 30) + 1000 + 2000 = 7400

### 4.2 50% Overweight (Minor Overweight)

When current weight >= 50% of MaxWeight:

**Penalties:**
- **HP natural regeneration STOPS** completely
- **SP natural regeneration STOPS** completely
- **Increase HP Recovery** skill effect is canceled/blocked
- **Increase SP Recovery** skill effect is canceled/blocked
- **Item-creation skills blocked**: Find Stone (Sage), Arrow Crafting (Archer) are disabled
- Sitting regen bonus is also blocked (since base regen is stopped)

**What still works at 50%+:**
- All attacks (physical, ranged, skill-based)
- All offensive and supportive skills
- Item usage (potions, consumables)
- Movement at normal speed
- Picking up items (until 100% weight)
- Equipment changes

**Note:** Weight does NOT affect movement speed in pre-renewal. Even at 89% weight, the character moves at full speed.

### 4.3 90% Overweight (Major Overweight)

When current weight >= 90% of MaxWeight:

**Penalties (severe):**
- **Cannot attack** (auto-attacks disabled, melee and ranged)
- **Cannot use ANY skills** (offensive, supportive, or self-buff)
- **Cannot use items** (potions, consumables, Fly Wings, etc.)
- **HP/SP regeneration STOPPED** (same as 50%)

**What still works at 90%+:**
- **Movement** at normal speed (can still walk/run)
- **Picking up items** until 100% weight
- **Opening storage** and depositing items to reduce weight
- **Dropping items** to reduce weight
- **Chatting and emotes**
- **NPC interaction** (shops, Kafra, etc.)
- Can still **equip/unequip** items
- Can still be **targeted by enemies** (and will take damage/die without being able to fight back)

### 4.4 100% Weight (Maximum)

When current weight >= 100% of MaxWeight:

- All 90% penalties apply, plus:
- **Cannot pick up items** from the ground
- Must drop, store, or sell items to free up space

### 4.5 Weight Thresholds Summary

```
Weight %    | Regen? | Attack? | Skills? | Items? | Pick Up? | Move? |
------------|--------|---------|---------|--------|----------|-------|
0-49%       | Yes    | Yes     | Yes     | Yes    | Yes      | Yes   |
50-89%      | NO     | Yes     | Yes     | Yes    | Yes      | Yes   |
90-99%      | NO     | NO      | NO      | NO     | Yes      | Yes   |
100%        | NO     | NO      | NO      | NO     | NO       | Yes   |
```

---

## 5. Auto-Attack System

### 5.1 Target Acquisition (No Tab-Targeting)

RO Classic uses a **purely click-based targeting system** -- there is no tab-targeting:

- **Left-click on monster**: Begins auto-attack
- **Ctrl+Click** on passive monster: Required to attack passive mobs (unless `/noctrl` is enabled)
- **`/noctrl` (or `/nc`)**: Removes the Ctrl requirement -- left-click attacks any monster
- Most players enable `/noctrl` immediately

**Target priority when entities overlap (click resolution order):**
1. NPC (highest)
2. Player characters
3. Monsters
4. Ground items
5. Ground/terrain (movement, lowest)

### 5.2 Target Locking

RO Classic does **NOT** have a traditional "target lock" system:

- There is no "selected target" UI indicator (no target frame, no bracket/highlight on the enemy)
- Clicking an enemy starts auto-attack, which continues until interrupted
- The only visual indicator of your current target is your character walking toward / attacking it
- There is no keyboard shortcut to cycle through targets (no tab-targeting)

**Effective target locking behavior:**
- Once auto-attack is engaged, it persists -- your character will continue attacking the same target
- If the target moves out of range, your character **chases** it automatically
- The chase is persistent -- your character follows the target until it dies, you click elsewhere, or you are CC'd

### 5.3 Attack Loop Mechanics

Once auto-attack begins:

```
loop:
  1. Check if target is alive and in attack range
  2. If NOT in range: walk toward target (chase)
  3. If in range:
     a. Face the target
     b. Check if attackDelay has elapsed since last attack
     c. If ready: perform one attack (damage calculation, animation)
     d. Wait for attackDelay ms
  4. Repeat from step 1
  5. Exit loop if:
     - Target dies (character stops, stands idle)
     - Player clicks ground (walk command overrides)
     - Player uses a skill (skill takes priority)
     - Player clicks different target (switch target)
     - Target leaves map (zone change, teleport, death)
     - Player is CC'd (Stun, Freeze, Stone, etc.)
     - Player weight >= 90%
     - Player sits down (Insert key)
```

### 5.4 ASPD and Attack Timing

**ASPD formula (pre-renewal):**

```
ASPD = 200 - (WD - floor((WD * AGI / 25) + (WD * DEX / 100)) / 10) * (1 - SM)
```

Where:
- `WD` = Weapon Delay = `50 * BTBA` (Base Time Between Attacks, class+weapon specific)
- `SM` = Speed Modifier (sum of all ASPD% bonuses from skills, potions, equipment)
- ASPD is **capped at 190**

**Attack delay (time between attacks):**
```
attackDelay_ms = (200 - ASPD) * 10
```

**Hits per second:**
```
hits_per_second = 50 / (200 - ASPD)
```

| ASPD | Attack Delay (ms) | Hits/sec |
|------|-------------------|----------|
| 150 | 500 | 2.0 |
| 160 | 400 | 2.5 |
| 170 | 300 | 3.33 |
| 175 | 250 | 4.0 |
| 180 | 200 | 5.0 |
| 185 | 150 | 6.67 |
| 190 | 100 | 10.0 (cap) |

**Speed Modifier sources (additive, only strongest potion applies):**

| Source | SM Value | Notes |
|--------|----------|-------|
| Concentration Potion | +0.10 | Potion (non-stack with other potions) |
| Awakening Potion | +0.15 | Potion (non-stack) |
| Berserk Potion | +0.20 | Potion (non-stack) |
| Poison Bottle | +0.25 | Assassin Cross only |
| Two-Hand Quicken | +0.30 | Knight (2H sword) |
| Adrenaline Rush | +0.25-0.30 | Blacksmith (axe/mace) |
| Spear Quicken | +0.21-0.30 | Knight/Crusader (spear) |
| Frenzy (Berserk) | +0.30 | Lord Knight, sets ASPD to 190 |
| Doppelganger Card | +0.10 | Equipment card |
| Peco Peco Ride | -0.50 | Penalty while mounted |

**Fractional ASPD:**
- The status window displays a rounded ASPD value, but fractional values (e.g., 170.4 vs 170.8) do have meaningful effects on actual attack timing
- Internal calculations use unrounded values

### 5.5 When Auto-Attack Stops

Auto-attack is interrupted/stopped by:

| Trigger | Result |
|---------|--------|
| Target dies | Character stands idle at current position |
| Click on ground | Walk command overrides attack, character walks to clicked position |
| Click on different monster | Switch target, begin attacking new monster |
| Press skill hotkey | Enter skill targeting mode (attack may resume after skill) |
| Target leaves map | Character stops (zone change, teleport, Fly Wing) |
| Player is CC'd | Stun/Freeze/Stone/Sleep -- character cannot act |
| Weight >= 90% | Attack blocked by overweight |
| Player sits (Insert) | Sitting cancels all attacks |
| Player dies | Obviously stops |
| Equipment change to incompatible weapon | Stops if weapon type changes (some skills) |

### 5.6 Attack Animation and Client Display

- Each attack plays a weapon-specific animation on the character sprite
- The attack animation speed scales with ASPD (higher ASPD = faster animation)
- Damage numbers appear above the target with a slight delay (~100-200ms after animation starts)
- Critical hits show a yellow damage number
- Misses show "MISS" text instead of a number
- The animation is **purely cosmetic** -- damage is calculated server-side at the moment the server processes the attack, regardless of animation state

---

## 6. Miscellaneous Systems

### 6.1 Dead Branch

**Item ID**: 604 (`Branch_Of_Dead_Tree`)
**Type**: Usable Item
**Weight**: 5
**Buy Price**: 50z / **Sell**: 25z

**Mechanics:**
- When used, **summons one random monster** at the user's location
- The summoned monster is drawn from a pool of normal (non-boss) monsters
- The pool includes a wide range of monsters from weak (Poring) to strong (non-MVP/non-boss monsters)
- The summoned monster is **aggressive** -- it will attack nearby players
- Summoned monster gives normal EXP and drops
- Can be used repeatedly (one branch = one monster)

**Restrictions:**
- Blocked on maps with `nobranch` mapflag
- Typically blocked in towns (most towns have `nobranch`)
- Can be used in dungeons and field maps (unless specifically restricted)
- Some servers restrict use to specific "branch rooms"

**Drop sources:**
- Treasure Chests (100% from various chest monsters)
- Various monsters at low rates (0.1-5%): Ghostring, etc.
- Old Blue Box, Old Violet Box random pools

### 6.2 Bloody Branch

**Item ID**: 12103 (`Bloody_Dead_Branch`)
**Type**: Usable Item
**Weight**: 20
**Buy Price**: 10,000z / **Sell**: 5,000z

**Mechanics:**
- When used, **summons one random MVP/Boss monster** at the user's location
- The summoned monster is drawn exclusively from the **boss monster pool** (MVPs)
- The summoned MVP has all its normal abilities, drops, and EXP
- The summoned MVP is **aggressive** and attacks immediately
- MVP drops follow normal rules (MVP reward, tombstone, etc.)

**Restrictions:**
- Blocked on maps with `nobranch` mapflag
- Typically restricted to specific "branch rooms" on many servers
- Same map restrictions as Dead Branch but more heavily controlled

**Acquisition:**
- Extremely rare -- not dropped by any monster in pre-renewal
- Obtained from events, quests, or special NPCs

### 6.3 Old Blue Box

**Item ID**: 603 (`Old_Blue_Box`)
**Type**: Usable Item
**Weight**: 20
**Buy Price**: 10,000z / **Sell**: 5,000z

**Mechanics:**
- When used, **gives the player one random item** from a predefined pool
- The item pool contains hundreds of items: equipment, cards, consumables, crafting materials
- Items in the pool have different weights (probability tiers):
  - Common items have multiple entries in the pool (higher chance)
  - Rare items have single entries (lower chance)
  - The probability of getting any specific item = (number of that item's entries) / (total entries in pool)

**Probability example:**
```
Pool: Apple (1 entry), Knife (2 entries), Jellopy (3 entries)
Total entries: 6
Apple chance: 1/6 = 16.7%
Knife chance: 2/6 = 33.3%
Jellopy chance: 3/6 = 50.0%
```

**Restrictions:**
- **Cannot use at 90%+ weight** (overweight blocks item use)
- The item received is placed in inventory (requires weight capacity)
- If the received item would exceed weight limit, the item may drop to the ground

**Drop sources:**
- Various monsters at low rates
- Quest rewards
- Treasure Chests

### 6.4 Old Purple Box (Old Violet Box)

**Item ID**: 617 (`Old_Violet_Box`)
**Type**: Usable Item
**Weight**: 20

**Mechanics:**
- Similar to Old Blue Box but with a **different (generally better) item pool**
- Contains higher-tier equipment, cards, and rare materials
- Same random selection mechanics as Old Blue Box
- **Cannot use at 90%+ weight**

### 6.5 Old Card Album

**Item ID**: 616 (`Old_Card_Album`)
**Type**: Usable Item
**Weight**: 5

**Mechanics:**
- When used, gives the player **one random card**
- The card pool includes most monster cards in the game
- Each card has equal or weighted probability depending on the server
- **Cannot use at 90%+ weight**
- Extremely valuable and rare item

### 6.6 Treasure Chests (WoE Castle Chests)

**WoE Guild Castle treasure system:**

**Spawn mechanics:**
- Treasure boxes spawn daily at **12:00 AM server time** (Pacific Time on iRO)
- Only spawn in castles currently held by a guild
- Spawn in a **special treasure room** accessible only by the Guild Leader through the castle NPC

**Contents:**
- Each castle has **6 treasure boxes**: 1 common box (shared pool across all castles) + 5 castle-specific boxes
- Contents include: weapons, armor, accessories, crafting materials (Elunium, Oridecon), and special items
- Each castle realm offers different item pools with some exclusive drops (e.g., Goddess Tear, Snow Crystal)

**Access:**
- Only the **Guild Leader** can enter the treasure room and collect boxes
- The Guild Leader controls distribution to guild members
- Boxes respawn daily as long as the guild holds the castle

**Castle investment:**
- Guilds can invest in castles to improve them (Defense, Economy)
- Economy investment increases the number/quality of treasure boxes
- Defense investment improves castle guardian NPCs

### 6.7 Treasure Chest Monsters (Dungeon Chests)

Separate from WoE treasure, some dungeons contain **Treasure Chest monsters**:

**Monster ID**: 1350+ (various types)
**Race**: Formless
**Element**: Neutral

**Mechanics:**
- These are monsters disguised as treasure chests
- They do not move (immobile) and do not attack
- Players attack them like normal monsters
- When killed, they drop items (Dead Branch at 100%, various equipment)
- They have very high DEF and HP for their level
- Respawn time is very long (typically hours)
- Found in deeper dungeon floors

### 6.8 @Commands Reference (Server Commands)

Standard RO does not have player-accessible @commands. These are server/GM commands used by private servers:

| Command | Effect | GM Level |
|---------|--------|----------|
| `@return` | Warp to save point | GM only |
| `@go <city>` | Warp to a specific city | GM only |
| `@warp <map> <x> <y>` | Warp to specific coordinates | GM only |
| `@alive` | Resurrect self | GM only |
| `@heal` | Full HP/SP heal | GM only |
| `@item <id> <amount>` | Create items | GM only |
| `@speed <value>` | Change movement speed | GM only |
| `@die` | Kill self | GM only |
| `@killmonster` | Kill all monsters on map | GM only |
| `@storage` | Open storage anywhere | GM only |

**Player-accessible commands (official):**
| Command | Effect |
|---------|--------|
| `/noctrl` or `/nc` | Toggle Ctrl requirement for attacking passive monsters |
| `/noshift` or `/ns` | Toggle Shift requirement for targeting players with support skills |
| `/sit` | Toggle sitting |
| `/stand` | Stand up |
| `/memo` | Memorize current map for Warp Portal |
| `/where` | Show current map name and coordinates |
| `/who` | Show number of online players |
| `/guild <name>` | Create a guild |
| `/breakguild` | Disband guild |
| `/effect` | Toggle skill effect display |
| `/mineffect` | Minimize skill effects |
| `/miss` | Toggle miss display |
| `/camera` | Toggle free camera mode |

---

## 7. Implementation Checklist

### 7.1 Death System
- [x] HP reaching 0 triggers death state
- [x] Death EXP penalty: 1% of next level's base EXP
- [x] Death EXP penalty: 1% of next level's job EXP
- [x] Novice class exempt from death penalty
- [x] Buff clearing on death (with survival whitelist)
- [ ] Death UI: "Return to Save Point" and "Wait here" buttons
- [ ] Respawn at save point with HP/SP restoration
- [ ] Brief invulnerability after respawn (~5 seconds)
- [ ] Baby class half penalty (0.5%)
- [ ] Max level no penalty (death_penalty_maxlv: 0)
- [ ] Super Novice Guardian Angel mechanic (99.0-99.9% EXP save)
- [ ] PvP: no EXP penalty (noexppenalty mapflag)
- [ ] WoE: no EXP penalty
- [ ] Nightmare mode item drop (pvp_nightmaredrop -- low priority)
- [ ] Dead player untargetable by enemies
- [ ] Death animation (fall and lie on ground)

### 7.2 Resurrection
- [ ] Resurrection skill (Priest): Lv1-4, HP restoration per level
- [ ] Resurrection Blue Gemstone consumption
- [ ] Yggdrasil Leaf: Resurrection Lv1 on dead target
- [x] Token of Siegfried: Self-rez, 100% HP/SP, no EXP penalty
- [x] Redemptio: AoE party mass rez, caster to 1 HP/SP, EXP cost with reduction
- [ ] Kaizel: Auto-rez buff (High Priest transcendent)

### 7.3 Teleportation
- [x] Teleport skill: Lv1 random, Lv2 save point
- [x] Fly Wing item: Random teleport (same map)
- [x] Butterfly Wing item: Save point teleport
- [x] Warp Portal skill: Memo system, Blue Gemstone, portal duration
- [x] Kafra teleport service: Paid city-to-city warps
- [ ] Giant Fly Wing: Party-wide random teleport (leader only)
- [ ] Creamy Card: Grants Teleport Lv1
- [x] Map restriction flags (noteleport, noreturn, nomemo)
- [ ] Magnetic Earth blocks Teleport skill

### 7.4 Weight System
- [x] Weight limit formula: 2000 + STR*30 + job bonus
- [x] 50% overweight: No HP/SP regen
- [x] 90% overweight: No attack, no skills, no items
- [ ] 100% weight: Cannot pick up items
- [x] Enlarge Weight Limit skill bonus
- [ ] Peco Peco mount +1000 weight capacity
- [ ] Gym Pass item bonus
- [ ] Weight check on item pickup (prevent exceeding 100%)
- [ ] 50% blocks Find Stone, Arrow Crafting

### 7.5 Auto-Attack System
- [x] Click-to-attack on monsters
- [x] ASPD-based attack delay loop
- [x] Target chase (follow target if out of range)
- [x] Attack stops on target death
- [x] Attack stops on ground click (movement)
- [x] Attack stops on CC (Stun, Freeze, etc.)
- [x] /noctrl equivalent (attack passive mobs)
- [x] 90% weight blocks attack
- [ ] Fractional ASPD internal calculations

### 7.6 Miscellaneous
- [ ] Dead Branch: Random normal monster summoning
- [ ] Bloody Branch: Random MVP/Boss summoning
- [ ] Old Blue Box: Random item from pool
- [ ] Old Purple Box: Random item from better pool
- [ ] Old Card Album: Random card from pool
- [ ] WoE Treasure Chests (requires WoE system)
- [ ] Treasure Chest dungeon monsters
- [ ] nobranch mapflag enforcement
- [ ] /noctrl, /noshift, /sit, /memo commands

---

## 8. Gap Analysis

### 8.1 Already Implemented in Sabri_MMO

Based on server source analysis (`server/src/index.js`):

| Feature | Status | Notes |
|---------|--------|-------|
| Death state (`DEAD` AI state) | Implemented | Enemy AI states include `DEAD` |
| Death EXP penalty (1% base/job) | Implemented | `clearBuffsOnDeath()` + penalty code at line ~2068 |
| Novice exemption | Implemented | Checked in penalty code |
| Max level no penalty | Implemented | nextLevelExp returns 0 |
| Buff clearing on death | Implemented | `clearBuffsOnDeath()` with `BUFFS_SURVIVE_DEATH` whitelist |
| Redemptio | Implemented | Uninterruptible, EXP cost with per-person reduction |
| Token of Siegfried | Partially | Logic exists but may need UI integration |
| Warp Portal + Memo | Implemented | `/memo1-3` commands, portal creation, zone transitions |
| Kafra Teleport | Implemented | `kafra:teleport` socket handler with zone transitions |
| Teleport skill | Implemented | Lv1 random, Lv2 save point |
| Fly Wing / Butterfly Wing | Implemented | Item effects trigger teleport skill |
| Weight system (50%/90%) | Implemented | Regen blocking, attack/skill blocking |
| ASPD attack loop | Implemented | 50ms combat tick, ASPD-based timing |
| Auto-attack chase | Implemented | Target tracking in combat tick |

### 8.2 Missing / Incomplete

| Feature | Priority | Effort | Notes |
|---------|----------|--------|-------|
| Death UI (Return/Wait buttons) | High | Medium | Client-side UI needed -- Slate widget for death screen |
| Respawn invulnerability (~5 sec) | Medium | Low | Server-side timer after respawn |
| Baby class half penalty | Low | Low | Check class type, halve penalty values |
| Super Novice Guardian Angel | Low | Medium | 99.0-99.9% EXP check, HP restore, Mental Strength buff |
| Resurrection skill (Priest) | High | Medium | Server handler + client target-dead-player flow |
| Yggdrasil Leaf resurrection | High | Low | Item effect that casts Resurrection Lv1 on dead target |
| Kaizel auto-rez buff | Low | Medium | High Priest transcendent skill (future) |
| Giant Fly Wing (party) | Low | Medium | Party leader item, teleports all members |
| Creamy Card Teleport Lv1 | Low | Low | Card effect granting skill |
| Magnetic Earth blocks Teleport | Low | Low | Check for ground effect before allowing cast |
| Dead Branch monster summoning | Medium | Medium | Random monster spawn from pool, aggressive |
| Bloody Branch MVP summoning | Medium | Medium | Random boss spawn from pool |
| Old Blue/Purple/Card Album | Low | Medium | Random item pools, weighted selection |
| WoE Treasure Chests | Low | High | Requires WoE system first |
| Death animation (client) | Medium | Medium | Fall/prone sprite animation |
| 100% weight pickup block | Low | Low | Check weight before allowing item pickup |
| Peco mount +1000 weight | Low | Low | Add to weight calc when mounted |
| nobranch mapflag | Low | Low | Zone flag check before branch use |
| Fractional ASPD | Low | Low | Use unrounded values internally |
| /noshift command | Medium | Low | Toggle for support skill targeting |
| Dead player untargetable | Medium | Low | Skip dead players in enemy AI targeting |

### 8.3 Key Implementation Order (Recommended)

1. **Death UI + Respawn flow** -- Critical for player experience. Need "Return to Save Point" / "Wait here" death screen with proper respawn.
2. **Resurrection skill + Yggdrasil Leaf** -- Priest core functionality. Without these, dead players must always return to save point.
3. **Death animation (client)** -- Visual feedback for death. Character fall/prone.
4. **Respawn invulnerability** -- Prevent spawn-camping.
5. **Dead Branch / Bloody Branch** -- Fun monster summoning items. Requires monster spawn pool.
6. **Old Blue/Purple Box + Card Album** -- Random item reward system.
7. **Giant Fly Wing** -- Party utility item.
8. **Baby class / Super Novice special mechanics** -- Lower priority, class-specific.
9. **WoE Treasure** -- Depends on full WoE implementation.
