# 04 -- Monsters & Enemy AI System

> Ragnarok Online Classic (Pre-Renewal) monster system reference.
> Covers AI behavior, mode flags, state machines, monster database, MVP bosses,
> spawn mechanics, drop rates, monster skills, zone-based monster lists,
> and Sabri_MMO server implementation details.

---

## Table of Contents

1. [Monster AI System](#1-monster-ai-system)
   - [AI Type Codes (01-27)](#11-ai-type-codes-01-27)
   - [Mode Flag Bitmask](#12-mode-flag-bitmask)
   - [State Machine](#13-state-machine-idle--chase--attack--dead)
   - [Aggro Mechanics](#14-aggro-mechanics)
   - [Chase Behavior](#15-chase-behavior)
   - [Wander Behavior](#16-wander-behavior)
   - [Target Selection](#17-target-selection)
2. [Top 100 Monsters](#2-top-100-monsters)
3. [MVP Bosses](#3-mvp-bosses)
4. [Spawn System](#4-spawn-system)
5. [Drop System](#5-drop-system)
6. [Monster Skills](#6-monster-skills)
7. [Monsters by Zone](#7-monsters-by-zone)
8. [Implementation (Sabri_MMO)](#8-implementation-sabri_mmo)

---

## 1. Monster AI System

Ragnarok Online uses an **Aegis-derived AI system** where each monster is assigned an AI type code (01-27) that maps to a hexadecimal mode bitmask. The bitmask encodes 18+ boolean behavior flags that the server-side state machine reads every tick to decide movement, aggro, target switching, and attack behavior.

**Source**: rAthena emulator (`doc/mob_db_mode_list.txt`, `src/map/mob.cpp`), confirmed against official Aegis behavior via rAthena Issue #926.

---

### 1.1 AI Type Codes (01-27)

Each monster has an AI type code from the mob database. The code maps to a hex mode bitmask that controls all behavior. Not all codes 01-27 are used; some are reserved or duplicates.

| Code | Hex Mode | Behavior Summary | Example Monsters |
|------|----------|-----------------|------------------|
| **01** | `0x0081` | **Passive**: Can move + can attack. Flees or retaliates when hit. Does not aggro players. | Fabre, Willow, Chonchon, Spore, Rocker |
| **02** | `0x0083` | **Passive + Looter**: Same as 01 but picks up items on the ground when idle. | Poring, Poporing, Verit |
| **03** | `0x1089` | **Passive + Assist + ChangeTargetMelee**: Retaliates when attacked. Nearby same-type mobs join the fight. Switches target when hit in melee. | Hornet, Condor, Wolf, Peco Peco |
| **04** | `0x3885` | **Angry/Hyper-Active**: Aggressive with assist. Has "follow" and "angry" pre-attack states. Switches target on melee and chase. | Familiar, Zombie, Orc Warrior, Mummy, Ghoul, Hunter Fly |
| **05** | `0x2085` | **Aggressive + ChangeTargetChase**: Actively seeks players. Switches to closer targets while chasing. | Archer Skeleton |
| **06** | `0x0000` | **Immobile/Plant**: Cannot move, cannot attack, no AI. Used for eggs, mushrooms, and objects. | Pupa, Peco Peco Egg, Red/Blue/Yellow Plant |
| **07** | `0x108B` | **Passive + Looter + Assist + ChangeTargetMelee**: Picks up items, assists allies, switches melee target. | Thief Bug, Thief Bug Female, Yoyo, Metaller, Steel Chonchon |
| **08** | `0x7085` | **Aggressive + TargetWeak + All ChangeTarget**: Prioritizes players 5+ levels below the monster. Switches targets on all conditions. | (Rare AI type, used by select monsters) |
| **09** | `0x3095` | **Aggressive + CastSensorIdle + ChangeTarget(Melee/Chase)**: Detects players casting skills while idle and aggros them. Switches targets actively. | Scorpion, Isis, Elder Willow, Side Winder |
| **10** | `0x0084` | **Immobile + Aggressive**: Cannot move but attacks anything in range. Stationary turret behavior. | Mandragora, Flora, Geographer |
| **11** | `0x0084` | **Guardian (Immobile)**: Same bitmask as 10. Used for War of Emperium guardians. | WoE Guardians |
| **12** | `0x2085` | **Guardian (Mobile)**: Aggressive + ChangeTargetChase. Mobile WoE guardians. | WoE Guardian (mobile variant) |
| **13** | `0x308D` | **Aggressive + Assist + ChangeTarget(Melee/Chase)**: Aggressive mob that calls same-type allies when attacked. | Thief Bug Male |
| **17** | `0x0091` | **Passive + CastSensorIdle**: Does not aggro normally, but detects and attacks players who cast skills nearby while idle. | Wormtail, Anacondaq, Smokie, Bigfoot, Golem |
| **19** | `0x3095` | **Aggressive + CastSensor + ChangeTarget(Melee/Chase)**: Same flags as 09 but distinct Aegis state machine behavior. | (Used for specific monsters) |
| **20** | `0x3295` | **Aggressive + CastSensor(Idle/Chase) + ChangeTarget(Melee/Chase)**: Enhanced cast sensor that triggers during both idle and chase states. | (Aggressive ranged with repositioning) |
| **21** | `0x3695` | **Boss/MVP**: Aggressive with CastSensor(Idle/Chase), ChangeTarget(Melee/Chase), ChangeChase. The most active AI type -- switches targets freely, detects casters, and changes chase targets. | Osiris, Baphomet, Doppelganger, Mistress, all MVPs |
| **24** | `0x00A1` | **Slave (Passive)**: CanMove + NoRandomWalk + CanAttack. Summoned by other mobs, stays near master. Does not wander. | Summoned slaves (e.g., Bigfoot slaves from Eddga) |
| **25** | `0x0001` | **Pet**: CanMove only. Cannot attack -- purely cosmetic/companion AI. | Pet versions of monsters |
| **26** | `0xB695` | **Aggressive + RandomTarget + All Sensors**: The most chaotic AI. Aggressive, cast sensor on idle and chase, changes target on melee and chase, AND picks random targets. | (Special aggressive mobs) |
| **27** | `0x8084` | **Immobile + RandomTarget + Aggressive**: Cannot move but attacks aggressively and randomizes targets. | (Special summon/spawn AI) |

**Codes 14-16, 18, 22-23 are unused** in the pre-renewal mob database. Code 09 was historically bugged in rAthena (incorrect assist behavior); the hex values above reflect the corrected Aegis behavior.

---

### 1.2 Mode Flag Bitmask

Each AI type code maps to a hex bitmask. Individual bits control specific behaviors:

| Flag Name | Hex Bit | Decimal | Description |
|-----------|---------|---------|-------------|
| `MD_CANMOVE` | `0x0000001` | 1 | Monster can move and chase players. Without this, the mob is rooted. |
| `MD_LOOTER` | `0x0000002` | 2 | Picks up items on the ground during idle state. |
| `MD_AGGRESSIVE` | `0x0000004` | 4 | Actively scans for nearby players to attack (does not wait to be hit first). |
| `MD_ASSIST` | `0x0000008` | 8 | When a same-type mob nearby is attacked, this mob joins the fight. |
| `MD_CASTSENSORIDLE` | `0x0000010` | 16 | Detects and aggros players who cast skills nearby while mob is idle. |
| `MD_NORANDOMWALK` | `0x0000020` | 32 | Disables random wandering. Mob stands still unless chasing. |
| `MD_NOCAST` | `0x0000040` | 64 | Monster cannot use skills (purely melee auto-attack). |
| `MD_CANATTACK` | `0x0000080` | 128 | Monster can perform attacks. Without this, it is passive (e.g., Pupa). |
| `MD_CASTSENSORCHASE` | `0x0000200` | 512 | Detects casters even while in chase state (not just idle). |
| `MD_CHANGECHASE` | `0x0000400` | 1024 | While chasing one target, switches to a closer in-range target. |
| `MD_ANGRY` | `0x0000800` | 2048 | "Hyper-active" mode. Has extra pre-attack states (follow/angry). Uses different skill sets before and after being attacked. |
| `MD_CHANGETARGETMELEE` | `0x0001000` | 4096 | Switches target when hit by melee attacks (while idle or attacking). |
| `MD_CHANGETARGETCHASE` | `0x0002000` | 8192 | Switches target when hit during chase state. |
| `MD_TARGETWEAK` | `0x0004000` | 16384 | Only aggros players whose level is 5+ below the monster's level. |
| `MD_RANDOMTARGET` | `0x0008000` | 32768 | Picks a random target from its attacker list each swing. |
| `MD_IGNOREMELEE` | `0x0010000` | 65536 | Immune to melee physical damage. |
| `MD_IGNOREMAGIC` | `0x0020000` | 131072 | Immune to magical damage. |
| `MD_IGNORERANGED` | `0x0040000` | 262144 | Immune to ranged physical damage. |
| `MD_MVP` | `0x0080000` | 524288 | MVP protocol: grants MVP rewards, tombstone on death, special EXP distribution. |
| `MD_IGNOREMISC` | `0x0100000` | 1048576 | Immune to misc/trap damage. |
| `MD_KNOCKBACKIMMUNE` | `0x0200000` | 2097152 | Cannot be knocked back by skills (Arrow Repel, Storm Gust, etc.). |
| `MD_TELEPORTBLOCK` | `0x0400000` | 4194304 | Cannot be teleported by skills. |
| `MD_FIXEDITEMDROP` | `0x1000000` | 16777216 | Drop rates are not affected by server rate modifiers. |
| `MD_DETECTOR` | `0x2000000` | 33554432 | Can detect and attack hidden/cloaked players. |
| `MD_STATUSIMMUNE` | `0x4000000` | 67108864 | Immune to all status effects (Stun, Freeze, Stone Curse, etc.). |
| `MD_SKILLIMMUNE` | `0x8000000` | 134217728 | Immune to being affected by skills entirely. |

**Boss Protocol** (automatically applied to `boss` and `mvp` class monsters):
- `MD_KNOCKBACKIMMUNE` -- cannot be displaced
- `MD_STATUSIMMUNE` -- immune to CC effects
- `MD_DETECTOR` -- sees through Hide/Cloak

**Bitmask Calculation Example:**
AI Type 21 (Boss/MVP) = `0x3695`:
- `0x0001` (CanMove) + `0x0004` (Aggressive) + `0x0010` (CastSensorIdle) + `0x0080` (CanAttack) + `0x0200` (CastSensorChase) + `0x0400` (ChangeChase) + `0x1000` (ChangeTargetMelee) + `0x2000` (ChangeTargetChase) = `0x3695`

---

### 1.3 State Machine: IDLE -> CHASE -> ATTACK -> DEAD

The enemy AI runs on a **200ms tick** (5 cycles per second). Each monster is in exactly one state at any time.

```
                  +---------+
                  |  DEAD   |<-------- HP reaches 0
                  +----+----+
                       |
                  respawnMs timer
                       |
                       v
    +------+     +---------+     +---------+
    | IDLE |---->|  CHASE  |---->| ATTACK  |
    +--+---+     +----+----+     +----+----+
       ^              |               |
       |    target    |    target     |
       |    lost      |    moved      |
       +<-------------+    out of     |
       |              |    range      |
       +<-------------+--------------+
```

#### IDLE State
- **Wander**: If `canMove` and NOT `noRandomWalk`, pick a random point within `wanderRadius` of spawn and walk there at 60% move speed. Pause 3-8 seconds between wanders.
- **Aggro Scan**: If `aggressive` flag is set, scan for players within `aggroRange` every 500ms. Closest player becomes target.
- **TargetWeak**: If `targetWeak` flag, only aggro players 5+ levels below the monster.
- **Transition to CHASE**: When a target is found (via aggro scan or damage).
- **Transition to ATTACK**: Immobile aggressive mobs (`canAttack` but NOT `canMove`) go directly to ATTACK.

#### CHASE State
- **Movement**: Move toward target at full `moveSpeed` (calculated from `walkSpeed` field).
- **Range Check**: When distance to target <= `attackRange + tolerance`, transition to ATTACK.
- **Give Up**: If monster travels beyond `chaseRange + 200` UE units from its aggro origin, give up and return to IDLE.
- **Target Validation**: If target disconnects, dies, or leaves zone, pick next target from `inCombatWith` set. If none remain, return to IDLE.
- **ChangeChase**: If `changeChase` flag is set, check if any attacker in `inCombatWith` is closer than current target and within attack range. If so, switch target mid-chase.
- **Hit Stun**: While in hit stun (`damageMotion` ms after taking damage), movement is paused but state checks continue.

#### ATTACK State
- **Auto-Attack**: Deal damage every `attackDelay` ms using the RO physical damage formula.
- **Range Check**: If target moves beyond `attackRange + tolerance + 30` UE units, transition back to CHASE.
- **Target Lost**: If target dies or disconnects, check `inCombatWith` for alternate targets. If none, return to IDLE.
- **RandomTarget**: If `randomTarget` flag is set and multiple combatants exist, pick a random one each swing.
- **Pending Target Switch**: Damage hooks can set `pendingTargetSwitch`; the AI tick consumes it and transitions to CHASE toward the new target.

#### DEAD State
- Entered when HP reaches 0.
- Broadcasts `enemy:death` event to all players in zone.
- Starts a `respawnMs` timer (from monster template).
- On respawn: HP restored to `maxHealth`, position reset to spawn coordinates, AI state reset to IDLE, broadcasts `enemy:spawn`.

---

### 1.4 Aggro Mechanics

**setEnemyAggro(enemy, attackerCharId, hitType)**
The central function called from ALL damage paths (auto-attack, all 9 skill types, ground effects).

1. Skip if enemy is dead or cannot attack (plant-type).
2. Add attacker to `enemy.inCombatWith` set.
3. Record `lastDamageTime` for hit stun calculation.
4. If no current target, OR current state is IDLE, OR `shouldSwitchTarget()` returns true:
   - Set new target to the attacker.
   - If in IDLE, record current position as `aggroOriginX/Y`.
   - If `canMove`, transition to CHASE. Otherwise, transition to ATTACK (immobile mobs).
5. Call `triggerAssist()` to alert nearby same-type allies.

**shouldSwitchTarget(enemy, newAttackerCharId, hitType)**
- No current target? Always switch.
- Same target? Never switch.
- `randomTarget` flag? Always switch.
- In ATTACK or IDLE state: switch if `changeTargetMelee` flag is set AND `hitType === 'melee'`.
- In CHASE state: switch if `changeTargetChase` flag is set.

**triggerAssist(attackedEnemy, attackerCharId)**
- Iterates all enemies in the same zone.
- For each that is: same `templateId`, alive, in IDLE state, has `assist` flag, has `canAttack` + `canMove`.
- If distance <= 550 UE units (11 RO cells), that enemy joins the fight: sets target, transitions to CHASE.

**findAggroTarget(enemy)**
- For aggressive mobs during IDLE aggro scan.
- Iterates all `connectedPlayers` in the same zone.
- Filters: not dead, has position data, within `aggroRange`.
- If `targetWeak`: only players with level >= `monsterLevel - 5` are skipped (targets players 5+ levels below).
- Returns closest qualifying player.

---

### 1.5 Chase Behavior

- **Move Speed**: Calculated from `walkSpeed` field. Formula: `moveSpeed = (50 / walkSpeed) * 1000` UE units/sec.
  - walkSpeed 400 (Poring) -> 125 UE/sec (very slow)
  - walkSpeed 200 (most mobs) -> 250 UE/sec (normal)
  - walkSpeed 100 (fast MVPs) -> 500 UE/sec (very fast)
- **Step Size**: `moveSpeed * (TICK_MS / 1000)` per tick. At 200ms ticks with 250 UE/sec, that is 50 UE units/tick.
- **Chase Range**: Default 600 UE units (12 RO cells). Bosses typically have 800-1200.
- **Leash/Give Up**: Monster gives up chase at `chaseRange + 200` UE units from aggro origin.
- **Return to Spawn**: After giving up chase, the mob returns to IDLE near its current position (not teleported to spawn). It will wander back toward spawn over time via the wander radius clamp.
- **Position Broadcasting**: Enemy position broadcast to zone at `MOVE_BROADCAST_MS` (200ms) intervals to avoid network spam.

---

### 1.6 Wander Behavior

- **Only in IDLE state** when no target exists.
- **Disabled for**: plants (`!canMove`), mobs with `noRandomWalk` flag, slaves (AI code 24).
- **Wander Cycle**:
  1. Wait `WANDER_PAUSE_MIN` to `WANDER_PAUSE_MAX` (3-8 seconds).
  2. Pick a random point within 100-300 UE units of current position.
  3. Clamp target to `wanderRadius` from spawn point (prevents infinite drift).
  4. Walk at **60% of chase speed** (relaxed movement).
  5. Arrive within 10 UE units -> stop, start new pause timer.
- **Wander Speed**: 60% of normal chase speed. A Poring (walkSpeed 400) wanders at ~75 UE/sec.

---

### 1.7 Target Selection

Target selection depends on the situation:

| Situation | Selection Rule |
|-----------|---------------|
| Aggressive aggro scan | Closest player within `aggroRange` |
| Damaged by player (no target) | The attacker becomes target |
| Damaged by new player (has target) | `shouldSwitchTarget()` rules apply |
| Target dies / disconnects | Next player from `inCombatWith` set (first available) |
| `randomTarget` flag | Random player from `inCombatWith` each attack swing |
| `changeChase` flag | While chasing, switch to any combatant closer than current target AND within attack range |
| `targetWeak` flag | Only aggro players 5+ levels below monster |
| Assist trigger | The player who attacked the original mob |

---

## 2. Top 100 Monsters

The following table lists 100 representative monsters from the pre-renewal database, covering the full level range (1-99) and all major zones. Data sourced from rAthena pre-renewal mob_db and RateMyServer.

### Beginner Monsters (Level 1-15)

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Move Spd | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|----------|---------------|-----------------|
| 1002 | Poring | 1 | 50 | 2 | 1 | 7-10 | 0 | 5 | Water 1 | Plant | Medium | 400 | Jellopy, Apple, Poring Card | prt_fild08, prt_fild07 |
| 1078 | Red Plant | 1 | 10 | 0 | 0 | 1-2 | 100 | 99 | Earth 1 | Plant | Small | 1000 | Red Herb, Stem, Apple | Various fields |
| 1007 | Fabre | 2 | 63 | 3 | 2 | 8-11 | 0 | 5 | Earth 1 | Insect | Small | 400 | Fluff, Feather, Fabre Card | prt_fild07, prt_fild08 |
| 1008 | Pupa | 2 | 427 | 2 | 4 | 1-2 | 0 | 0 | Earth 1 | Insect | Small | 1000 | Chrysalis, Shell, Pupa Card | prt_fild07 |
| 1113 | Drops | 3 | 55 | 4 | 2 | 10-13 | 0 | 5 | Fire 1 | Plant | Medium | 400 | Jellopy, Orange Juice, Drops Card | prt_fild08, moc_fild01 |
| 1063 | Lunatic | 3 | 60 | 4 | 3 | 9-12 | 0 | 0 | Neutral 3 | Brute | Small | 200 | Clover, Carrot, Lunatic Card | prt_fild07, prt_fild08 |
| 1010 | Willow | 4 | 95 | 5 | 4 | 9-12 | 0 | 15 | Earth 1 | Plant | Medium | 200 | Tree Root, Trunk, Willow Card | prt_fild07, pay_fild01 |
| 1011 | Chonchon | 4 | 67 | 5 | 4 | 10-13 | 5 | 5 | Wind 1 | Insect | Small | 200 | Shell, Jellopy, Chonchon Card | prt_fild08, mjo_fild01 |
| 1009 | Condor | 5 | 92 | 6 | 5 | 11-14 | 0 | 5 | Wind 1 | Brute | Medium | 150 | Feather, Talon, Condor Card | moc_fild02, moc_fild01 |
| 1012 | Roda Frog | 5 | 133 | 6 | 5 | 12-15 | 0 | 5 | Water 1 | Fish | Medium | 200 | Spawn, Sticky Mucus, Roda Frog Card | prt_fild08 |
| 1049 | Picky | 5 | 80 | 5 | 4 | 9-12 | 0 | 0 | Fire 1 | Brute | Small | 200 | Feather, Red Herb, Picky Card | prt_fild07 |
| 1052 | Rocker | 9 | 198 | 20 | 16 | 24-29 | 5 | 5 | Earth 1 | Insect | Medium | 200 | Grasshopper's Leg, Rocker Card | prt_fild02, prt_fild06 |
| 1004 | Hornet | 8 | 169 | 19 | 15 | 20-27 | 5 | 5 | Wind 1 | Insect | Small | 150 | Bee Sting, Honey, Hornet Card | mjo_fild01, mjo_fild02 |
| 1005 | Familiar | 8 | 155 | 28 | 15 | 20-28 | 0 | 0 | Shadow 1 | Brute | Small | 150 | Tooth of Bat, Hatchling Card | pay_fild03 |
| 1020 | Mandragora | 12 | 405 | 45 | 32 | 30-39 | 0 | 10 | Earth 3 | Plant | Medium | 1000 | Stem, Green Herb, Mandragora Card | mjo_fild01 |
| 1031 | Poporing | 14 | 344 | 81 | 44 | 42-52 | 0 | 5 | Poison 1 | Plant | Medium | 300 | Sticky Mucus, Green Herb, Poporing Card | pay_fild01, prt_fild08 |
| 1024 | Wormtail | 14 | 426 | 59 | 40 | 39-48 | 0 | 5 | Earth 1 | Plant | Medium | 200 | Worm Peeling, Sticky Mucus | pay_fild01 |

### Low-Mid Monsters (Level 15-30)

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Move Spd | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|----------|---------------|-----------------|
| 1015 | Zombie | 15 | 534 | 50 | 33 | 67-79 | 0 | 10 | Undead 1 | Undead | Medium | 400 | Decayed Nail, Horrendous Mouth | pay_dun00, prt_sewb1 |
| 1025 | Boa | 15 | 471 | 72 | 48 | 50-60 | 0 | 5 | Earth 1 | Brute | Medium | 200 | Snake Scale, Boa Card | pay_fild03, pay_fild01 |
| 1014 | Spore | 16 | 510 | 66 | 108 | 24-30 | 0 | 5 | Water 1 | Plant | Medium | 200 | Spore, Mushroom, Spore Card | pay_fild01, mjo_fild02 |
| 1018 | Creamy | 16 | 595 | 105 | 70 | 55-68 | 5 | 5 | Wind 1 | Insect | Small | 150 | Butterfly Wing, Creamy Card | prt_fild01 |
| 1019 | Peco Peco | 19 | 531 | 159 | 72 | 60-72 | 5 | 5 | Fire 1 | Brute | Large | 200 | Peco Peco Feather, Bill of Birds | prt_fild03 |
| 1030 | Anacondaq | 23 | 1,109 | 300 | 149 | 80-96 | 5 | 5 | Poison 1 | Brute | Medium | 200 | Venom Canine, Snake Scale | moc_fild04, moc_fild12 |
| 1001 | Scorpion | 24 | 1,109 | 287 | 176 | 85-100 | 10 | 5 | Fire 1 | Insect | Small | 200 | Scorpion Tail, Venom Canine | moc_fild04, moc_fild02 |
| 1023 | Orc Warrior | 24 | 1,400 | 408 | 160 | 98-115 | 10 | 5 | Earth 1 | Demi-Human | Medium | 200 | Orcish Voucher, Oridecon Stone | gef_fild03, gef_fild10 |
| 1013 | Wolf | 25 | 919 | 329 | 199 | 75-90 | 5 | 0 | Earth 1 | Brute | Medium | 200 | Wolf Claw, Meat, Wolf Card | pay_fild02 |
| 1042 | Steel Chonchon | 26 | 1,003 | 240 | 182 | 80-98 | 15 | 5 | Wind 1 | Insect | Small | 150 | Wind of Verdure, Steel Chonchon Card | mjo_fild02, gef_fild06 |
| 1028 | Soldier Skeleton | 29 | 2,334 | 372 | 226 | 120-145 | 10 | 0 | Undead 1 | Undead | Medium | 200 | Skel-Bone, Gladius | pay_dun01, prt_sewb2 |
| 1026 | Munak | 30 | 2,872 | 601 | 318 | 130-155 | 5 | 10 | Undead 1 | Undead | Medium | 200 | Munak Turban, Munak Card | pay_dun01, pay_dun02 |

### Mid-Level Monsters (Level 30-50)

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Move Spd | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|----------|---------------|-----------------|
| 1016 | Archer Skeleton | 31 | 3,040 | 483 | 283 | 150-180 | 10 | 5 | Undead 1 | Undead | Medium | 300 | Skel-Bone, Fire Arrow | pay_dun02, prt_sewb3 |
| 1041 | Mummy | 33 | 3,843 | 676 | 415 | 175-210 | 10 | 10 | Undead 2 | Undead | Medium | 300 | Rotten Bandage, Mummy Card | moc_pryd01, moc_pryd02 |
| 1091 | Orc Skeleton | 34 | 4,266 | 693 | 480 | 180-218 | 10 | 10 | Undead 1 | Undead | Medium | 200 | Skel-Bone, Orc Skeleton Card | gef_fild05, orcsdun01 |
| 1035 | Hunter Fly | 42 | 5,242 | 2,200 | 810 | 220-270 | 20 | 10 | Wind 2 | Insect | Small | 150 | Zargon, Wing of Fly, Hunter Fly Card | gef_dun01, treasure01 |
| 1143 | Marionette | 41 | 3,222 | 1,510 | 655 | 200-245 | 15 | 10 | Ghost 3 | Demon | Small | 200 | Puppet, Wooden Mail | gef_dun01 |
| 1185 | Orc Lady | 34 | 3,320 | 773 | 512 | 170-205 | 10 | 10 | Earth 2 | Demi-Human | Medium | 200 | Orc Voucher, Heart of Orc Lady | gef_fild10 |
| 1109 | Marse | 35 | 2,689 | 710 | 455 | 165-200 | 5 | 15 | Water 2 | Fish | Small | 300 | Tentacle, Marse Card | iz_dun01 |
| 1132 | Whisper | 34 | 2,150 | 1,030 | 487 | 155-185 | 0 | 45 | Ghost 3 | Demon | Small | 200 | Fabric, Whisper Card | pay_dun03, gef_dun00 |
| 1029 | Isis | 47 | 7,003 | 3,709 | 1,550 | 300-370 | 15 | 25 | Shadow 1 | Demon | Large | 200 | Shining Scale, Isis Card | moc_pryd03, moc_pryd04 |
| 1099 | Goblin | 25 | 1,176 | 297 | 185 | 95-112 | 10 | 5 | Wind 1 | Demi-Human | Medium | 200 | Oridecon Stone, Goblin Card | gef_fild03 |
| 1064 | Metaller | 22 | 1,060 | 210 | 135 | 78-95 | 5 | 5 | Fire 1 | Insect | Medium | 200 | Iron, Iron Ore, Metaller Card | mjo_fild02 |
| 1036 | Ghoul | 40 | 5,418 | 2,308 | 921 | 230-280 | 15 | 10 | Undead 2 | Undead | Medium | 400 | Horrendous Mouth, Ghoul Card | pay_dun03, gef_dun00 |
| 1044 | Obeaune | 35 | 3,152 | 950 | 550 | 160-195 | 10 | 30 | Water 3 | Fish | Medium | 200 | Heart of Mermaid, Obeaune Card | iz_dun01, iz_dun02 |
| 1045 | Marc | 36 | 3,410 | 1,068 | 586 | 170-205 | 15 | 15 | Water 2 | Fish | Medium | 300 | Gill, Marc Card | iz_dun02 |

### Mid-High Monsters (Level 50-70)

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Move Spd | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|----------|---------------|-----------------|
| 1037 | Side Winder | 43 | 5,150 | 2,500 | 1,100 | 240-295 | 15 | 10 | Poison 1 | Brute | Medium | 200 | Venom Canine, Side Winder Card | moc_pryd03 |
| 1192 | Orc Zombie | 31 | 3,520 | 475 | 310 | 158-190 | 5 | 10 | Undead 1 | Undead | Medium | 400 | Rotten Bandage, Orc Zombie Card | orcsdun01 |
| 1191 | Drainliar | 26 | 1,568 | 365 | 250 | 105-130 | 5 | 5 | Shadow 2 | Brute | Small | 250 | Tooth of Bat, Drainliar Card | pay_dun01, gef_dun00 |
| 1179 | Skel Worker | 38 | 4,710 | 1,210 | 750 | 200-240 | 10 | 5 | Undead 1 | Undead | Medium | 200 | Oridecon Stone, Coal | ein_dun01 |
| 1188 | Bongun | 32 | 3,520 | 750 | 420 | 165-195 | 10 | 5 | Undead 1 | Undead | Medium | 200 | Munak Turban, Bongun Card | pay_dun02 |
| 1144 | Bathory | 45 | 6,500 | 2,560 | 1,470 | 250-310 | 15 | 30 | Shadow 1 | Demon | Medium | 200 | Bat Tooth, Bathory Card | alde_dun03 |
| 1205 | Sohee | 40 | 5,000 | 2,200 | 900 | 210-255 | 10 | 30 | Fire 1 | Demon | Medium | 200 | Sohee Card, Tsurugi | pay_dun03 |
| 1193 | Orc Archer | 28 | 2,640 | 425 | 280 | 120-150 | 5 | 10 | Earth 1 | Demi-Human | Medium | 300 | Steel Arrow, Orc Archer Card | orcsdun02 |
| 1126 | Penomena | 50 | 7,513 | 4,070 | 2,120 | 330-410 | 20 | 20 | Poison 3 | Fish | Medium | 200 | Tentacle, Single Cell | alde_dun04 |
| 1211 | Sage Worm | 46 | 5,500 | 3,100 | 1,600 | 270-330 | 10 | 35 | Shadow 1 | Brute | Small | 200 | Bookclip in Memory | gef_dun01 |
| 1219 | Abysmal Knight | 59 | 17,540 | 7,710 | 6,105 | 540-680 | 30 | 30 | Shadow 4 | Demi-Human | Large | 250 | Elunium, Abysmal Knight Card | gl_knt01, gl_knt02 |
| 1253 | Hatii Baby | 55 | 8,980 | 5,200 | 2,800 | 380-470 | 20 | 25 | Water 2 | Brute | Small | 200 | Mystic Frozen, Ice Crystal | xmas_fild01 |
| 1177 | Zenorc | 37 | 4,230 | 1,020 | 612 | 195-240 | 10 | 10 | Poison 1 | Demi-Human | Medium | 200 | Zenorcist's Brace, Zenorc Card | gef_dun01 |
| 1263 | Wind Ghost | 60 | 10,190 | 5,440 | 3,250 | 400-500 | 15 | 40 | Wind 3 | Demon | Medium | 200 | Rough Wind, Wind Ghost Card | gl_cas01 |

### High-Level Monsters (Level 70-99)

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Move Spd | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|----------|---------------|-----------------|
| 1163 | Raydric | 52 | 10,297 | 5,810 | 3,420 | 430-540 | 25 | 15 | Shadow 2 | Demi-Human | Large | 200 | Elunium, Raydric Card | gl_knt01, gl_knt02 |
| 1269 | Clock | 60 | 10,100 | 5,300 | 3,100 | 400-490 | 20 | 30 | Earth 2 | Formless | Medium | 200 | Oridecon, Clock Card | alde_dun04 |
| 1271 | Alarm | 58 | 8,000 | 4,800 | 2,750 | 370-460 | 20 | 30 | Fire 2 | Formless | Medium | 200 | Oridecon, Alarm Card | alde_dun03 |
| 1290 | Joker | 57 | 12,700 | 5,920 | 3,880 | 500-620 | 15 | 35 | Wind 4 | Demi-Human | Large | 200 | Mask of Joker, Joker Card | xmas_dun02 |
| 1262 | Anolian | 61 | 12,180 | 6,000 | 3,510 | 420-530 | 25 | 25 | Water 2 | Fish | Medium | 200 | Crystal Mirror, Anolian Card | cmd_fild03, ama_dun03 |
| 1257 | Injustice | 55 | 9,150 | 5,100 | 2,950 | 390-480 | 20 | 20 | Shadow 2 | Undead | Medium | 200 | Zargon, Injustice Card | gl_sew01, gl_sew02 |
| 1196 | High Orc | 52 | 10,010 | 5,600 | 3,200 | 440-550 | 30 | 20 | Fire 2 | Demi-Human | Large | 200 | Orcish Voucher, High Orc Card | gef_fild05, orcsdun02 |
| 1382 | Dark Priest | 72 | 19,850 | 8,450 | 5,670 | 580-730 | 25 | 50 | Undead 4 | Demon | Medium | 250 | Dead Branch, Dark Priest Card | gl_chyard |
| 1365 | Stalactic Golem | 74 | 22,100 | 9,200 | 6,100 | 620-780 | 45 | 15 | Neutral 4 | Formless | Large | 300 | Elunium, Coal | iz_dun04 |
| 1401 | Shinobi | 68 | 15,200 | 7,200 | 4,800 | 510-640 | 20 | 30 | Shadow 3 | Demi-Human | Medium | 150 | Shinobi Sash, Shinobi Card | ama_dun03 |
| 1380 | Gryphon | 72 | 20,050 | 8,880 | 5,550 | 600-750 | 25 | 35 | Wind 3 | Brute | Large | 150 | Wind of Verdure, Gryphon Card | yuno_fild01 |
| 1613 | Solider | 60 | 11,500 | 5,500 | 3,200 | 430-540 | 20 | 20 | Neutral 2 | Demi-Human | Medium | 200 | Iron Ore, Coal | ein_dun01, ein_dun02 |
| 1651 | Assaulter | 63 | 13,800 | 6,400 | 3,900 | 470-590 | 25 | 25 | Shadow 2 | Demi-Human | Medium | 200 | Oridecon Stone, Assaulter Card | ama_dun03 |

### Elite / Mini-Boss Monsters

| ID | Name | Lv | HP | Base EXP | Job EXP | ATK | DEF | MDEF | Element | Race | Size | Notable Drops | Spawn Locations |
|----|------|----|----|----------|---------|-----|-----|------|---------|------|------|---------------|-----------------|
| 1582 | Angeling | 54 | 41,350 | 3,500 | 2,800 | 350-440 | 20 | 80 | Holy 3 | Angel | Small | Angeling Card, Apple | yuno_fild07 |
| 1584 | Deviling | 62 | 49,000 | 4,500 | 3,600 | 450-560 | 15 | 60 | Shadow 4 | Demon | Small | Deviling Card, Grape | pay_fild04 |
| 1120 | Ghostring | 60 | 32,700 | 3,800 | 3,100 | 380-480 | 10 | 70 | Ghost 4 | Demon | Small | Ghostring Card, Sticky Mucus | pay_dun04 |
| 1096 | Mastering | 25 | 4,990 | 900 | 700 | 100-120 | 10 | 40 | Neutral 3 | Plant | Small | Mastering Card, Crown | prt_fild04 |
| 1583 | Tao Gunka | 70 | 193,000 | 59,175 | 10,445 | 1,450-1,770 | 20 | 20 | Neutral 3 | Demon | Large | Tao Gunka Card, Stone Fragment | beach_dun |
| 1785 | Atroce | 82 | 1,008,420 | 295,550 | 118,895 | 2,526-3,646 | 25 | 25 | Shadow 3 | Brute | Large | Infiltrator, Atroce Card | ra_fild02 |
| 1630 | Bacsojin | 85 | 253,221 | 45,250 | 16,445 | 1,868-6,124 | 20 | 55 | Wind 3 | Demi-Human | Large | Bloody Edge, Bacsojin Card | lou_dun03 |

---

## 3. MVP Bosses

MVP (Most Valuable Player) bosses are the pinnacle of PvE combat in Ragnarok Online. They have massive HP pools, powerful skills, unique drops, and operate under **Boss Protocol** (knockback immune, status immune, detect hidden players).

### MVP Mechanics

- **Boss Protocol**: All MVPs have `MD_KNOCKBACKIMMUNE`, `MD_STATUSIMMUNE`, and `MD_DETECTOR` flags.
- **MVP Reward**: The player dealing the most damage receives the "MVP" award -- 3 bonus MVP drops roll into their inventory, plus bonus MVP EXP.
- **Tombstone**: When killed, an MVP tombstone appears at the death location showing time of death and MVP-awarded player name. The tombstone disappears 5 seconds after respawn.
- **Free-for-All**: Any player can attack an MVP regardless of who found it first. No tagging system.
- **Slave Summoning**: Many MVPs summon "slave" monsters (minions). Slaves share the MVP's aggro table.
- **AI Type**: Almost all MVPs use AI type 21 (`0x3695`) -- aggressive with full target switching, cast sensor, and chase-change.

### Complete MVP Boss Table

| ID | Name | Lv | HP | ATK | DEF | MDEF | Element | Race | Size | Base EXP | Job EXP | MVP EXP | Spawn Map | Respawn (min) |
|----|------|----|----|-----|-----|------|---------|------|------|----------|---------|---------|-----------|--------------|
| 1511 | **Amon Ra** | 88 | 1,214,138 | 1,647-2,576 | 26 | 52 | Earth 3 | Demi-Human | Large | 87,264 | 35,891 | 43,632 | moc_pryd06 (Pyramid B2) | 60-70 |
| 1039 | **Baphomet** | 81 | 668,000 | 3,220-4,040 | 35 | 45 | Shadow 3 | Demon | Large | 107,250 | 37,895 | 53,625 | prt_maze03 (Labyrinth F3) | 120-130 |
| 1874 | **Beelzebub** | 98 | 6,666,666 | 10,000-13,410 | 40 | 40 | Ghost 4 | Demon | Large | 6,666,666 | 6,666,666 | 3,333,333 | abbey03 (Nameless Island) | 720-730 |
| 1272 | **Dark Lord** | 80 | 720,000 | 2,800-3,320 | 30 | 70 | Undead 4 | Demon | Large | 65,780 | 45,045 | 32,890 | gl_chyard (Glast Heim) | 60-70 |
| 1719 | **Detardeurus** | 90 | 960,000 | 4,560-5,548 | 66 | 59 | Shadow 3 | Dragon | Large | 291,850 | 123,304 | 145,925 | abyss_03 (Abyss Lake 3) | 180-190 |
| 1046 | **Doppelganger** | 72 | 249,000 | 1,340-1,590 | 60 | 35 | Shadow 3 | Demon | Medium | 51,480 | 10,725 | 25,740 | gef_dun02 (Geffen Dungeon F3) | 120-130 |
| 1389 | **Dracula** | 85 | 320,096 | 1,625-1,890 | 45 | 76 | Shadow 4 | Demon | Large | 120,157 | 38,870 | 60,078 | gef_dun01 (Geffen Dungeon F2) | 60-70 |
| 1112 | **Drake** | 70 | 326,666 | 1,800-2,100 | 20 | 35 | Undead 1 | Undead | Medium | 28,600 | 22,880 | 14,300 | treasure02 (Sunken Ship F2) | 120-130 |
| 1115 | **Eddga** | 65 | 152,000 | 1,215-1,565 | 15 | 15 | Fire 1 | Brute | Large | 25,025 | 12,870 | 12,512 | pay_fild11 (Payon Field 10) | 120-130 |
| 1086 | **Golden Thief Bug** | 64 | 126,000 | 870-1,145 | 60 | 45 | Fire 2 | Insect | Large | 14,300 | 7,150 | 7,150 | prt_sewb4 (Prontera Culvert F4) | 60-70 |
| 1252 | **Hatii (Garm)** | 73 | 197,000 | 1,700-1,900 | 40 | 45 | Water 4 | Brute | Large | 50,050 | 20,020 | 25,025 | xmas_fild01 (Lutie Field) | 120-130 |
| 1147 | **Maya** | 81 | 169,000 | 1,800-2,070 | 60 | 25 | Earth 4 | Insect | Large | 42,900 | 17,875 | 21,450 | anthell02 (Ant Hell F2) | 120-130 |
| 1059 | **Mistress** | 74 | 212,000 | 880-1,110 | 40 | 60 | Wind 4 | Insect | Small | 39,325 | 27,170 | 19,662 | mjolnir_04 (Mt. Mjolnir) | 120-130 |
| 1150 | **Moonlight Flower** | 67 | 120,000 | 1,200-1,700 | 10 | 55 | Fire 3 | Demon | Medium | 27,500 | 14,300 | 13,750 | pay_dun04 (Payon Cave F5) | 60-70 |
| 1087 | **Orc Hero** | 77 | 585,700 | 2,257-2,542 | 40 | 45 | Earth 2 | Demi-Human | Large | 58,630 | 32,890 | 29,315 | gef_fild03 (Geffen Field) | 1440-1450 |
| 1190 | **Orc Lord** | 74 | 783,000 | 3,700-4,150 | 40 | 5 | Earth 4 | Demi-Human | Large | 62,205 | 8,580 | 31,102 | gef_fild10 (Geffen Field) | 120-130 |
| 1038 | **Osiris** | 78 | 415,400 | 780-2,880 | 10 | 25 | Undead 4 | Undead | Medium | 71,500 | 28,600 | 35,750 | moc_pryd04 (Pyramid F4) | 60-70 |
| 1157 | **Pharaoh** | 93 | 445,997 | 2,267-3,015 | 67 | 70 | Shadow 3 | Demi-Human | Large | 114,990 | 41,899 | 57,495 | in_sphinx5 (Sphinx F5) | 60-70 |
| 1159 | **Phreeoni** | 69 | 188,000 | 880-1,530 | 10 | 20 | Neutral 3 | Brute | Large | 32,175 | 16,445 | 16,087 | moc_fild17 (Sograt Desert) | 120-130 |
| 1583 | **Tao Gunka** | 70 | 193,000 | 1,450-1,770 | 20 | 20 | Neutral 3 | Demon | Large | 59,175 | 10,445 | 29,587 | beach_dun (Beach Dungeon) | 300-310 |
| 1312 | **Turtle General** | 97 | 320,700 | 2,438-3,478 | 50 | 54 | Earth 2 | Brute | Large | 18,202 | 9,800 | 9,101 | tur_dun04 (Turtle Palace F4) | 60-70 |
| 1751 | **Valkyrie Randgris** | 99 | 3,567,200 | 5,560-9,980 | 25 | 42 | Holy 4 | Angel | Large | 2,854,900 | 3,114,520 | 1,427,450 | odin_tem03 (Odin Temple F3) | 480-490 |
| 1647 | **Assassin Cross Eremes** | 99 | 1,411,230 | 4,189-8,289 | 37 | 39 | Poison 4 | Demi-Human | Medium | 4,083,400 | 1,592,380 | 2,041,700 | lhz_dun03 (Bio Lab F3) | 120-130 |
| 1785 | **Atroce** | 82 | 1,008,420 | 2,526-3,646 | 25 | 25 | Shadow 3 | Brute | Large | 295,550 | 118,895 | 147,775 | ra_fild02 (Rachel Field) | 300-310 |
| 2068 | **Boitata** | 93 | 1,283,990 | 3,304-4,266 | 32 | 66 | Fire 3 | Brute | Large | 74,288 | 77,950 | 37,144 | bra_dun02 (Brasilis Dungeon) | 120-130 |

### MVP Detailed Profiles

#### Baphomet (ID: 1039)
- **Stats**: STR 1, AGI 152, VIT 30, INT 85, DEX 120, LUK 95
- **Speed**: Very Fast (attack delay 0.77s)
- **Skills**: Brandish Spear, Dark Breath, Lord of Vermillion, Heal, Teleportation, Maximize Power
- **Regular Drops**: Elunium (54.32%), Oridecon (41.71%), Crescent Scythe (4%), Evil Horn (0.1%), Baphomet Card (0.01%)
- **MVP Drops**: Evil Horn (50%), Baphomet Doll (10%), Baphomet Horn (3%)
- **Slaves**: Summons Dark Illusions

#### Dark Lord (ID: 1272)
- **Stats**: STR 1, AGI 120, VIT 30, INT 118, DEX 99, LUK 60
- **Speed**: Very Fast (attack delay 0.87s)
- **Skills**: Meteor Storm, Fire Wall, Dark Blessing, Hell's Judgement, Agility Up, Break Helm, Teleportation
- **Regular Drops**: Elunium (51.41%), Evil Bone Wand (8%), Grimtooth (3%), Mage Coat (3%), Dark Lord Card (0.01%)
- **MVP Drops**: Skull (60%), Old Purple Box (20%), Coif [1] (5%)
- **Slaves**: Summons Dark Illusions, Wraiths

#### Osiris (ID: 1038)
- **Stats**: STR 1, AGI 75, VIT 30, INT 37, DEX 86, LUK 40
- **Skills**: Curse Attack, Dark Blessing, Dark Strike, Teleportation, Summoning
- **Regular Drops**: Hand of God (10%), Osiris Card (0.01%), various weapons
- **MVP Drops**: Old Blue Box (40%), Yggdrasil Seed (30%), Elunium (10%)
- **Element Weakness**: Fire (200%), Holy (200%), Water (150%)

#### Pharaoh (ID: 1157)
- **Stats**: STR 1, AGI 93, VIT 100, INT 104, DEX 89, LUK 112
- **Skills**: Fire Wall, Meteor Storm, Teleportation, Heal, Power Up, Dark Blessing
- **Regular Drops**: Solar Sword (1%), Bazerald (0.8%), Tablet [1] (3%), Holy Robe (1.5%), Pharaoh Card (0.01%)
- **MVP Drops**: Royal Jelly (50%), Yggdrasil Berry (55%), 3-carat Diamond (50%)
- **Card Effect**: SP consumption -30% (one of the most valuable cards in the game)

#### Mistress (ID: 1059)
- **Stats**: STR 50, AGI 165, VIT 60, INT 95, DEX 70, LUK 130
- **Skills**: Jupitel Thunder, Heal, Lex Divina, Pneuma, Consecutive Attack
- **Regular Drops**: Honey (100%), Old Card Album (10%), Elunium (42.68%), Coronet (2.5%), Gungnir (1.5%), Mistress Card (0.01%)
- **MVP Drops**: Royal Jelly (40%), Pearl (30%), Rough Wind (15%)
- **Card Effect**: Gemstone-less casting (no gemstone consumption for skills)

#### Golden Thief Bug (ID: 1086)
- **Stats**: STR 65, AGI 75, VIT 35, INT 45, DEX 85, LUK 150
- **Skills**: Heal, Fire Attack, Defensive Shield
- **Regular Drops**: Emperium (3%), Oridecon (15%), Elunium (20%), Golden Thief Bug Card (0.01%)
- **MVP Drops**: Gold Ring (20%)
- **Card Effect**: Nullify all magic attacks against wearer (extremely valuable)

#### Eddga (ID: 1115)
- **Stats**: STR 78, AGI 70, VIT 85, INT 66, DEX 90, LUK 85
- **Skills**: Power Up, Summoning (Bigfoot slaves)
- **Regular Drops**: Honey (100%), Katar of Raging Blaze (5%), Tiger's Footskin (2.5%), Eddga Card (0.01%)
- **MVP Drops**: Tiger Skin (50%), Elunium (23%)
- **Card Effect**: Endure effect (cannot be flinched), -25% Max HP

#### Moonlight Flower (ID: 1150)
- **Stats**: STR 55, AGI 99, VIT 55, INT 82, DEX 95, LUK 120
- **Skills**: Mammonite, Heal, Earth Spike, Fire Bolt, Lightning Bolt, Cold Bolt
- **Regular Drops**: Elunium (26%), Silver Knife of Chastity (6.5%), Punisher (5%), Spectral Spear (5%), Moonlight Dagger (1%), Moonlight Flower Card (0.01%)
- **MVP Drops**: Nine Tails (50%), White Potion (15%), Topaz (5%)
- **Card Effect**: No movement speed penalty from equipment

#### Valkyrie Randgris (ID: 1751)
- **Stats**: STR 100, AGI 120, VIT 30, INT 120, DEX 220, LUK 210
- **Skills**: Power Up, Heal, Dispell, Holy Cross, Grand Cross, Magnum Break
- **Regular Drops**: Valkyrian Armor [1] (16%), Valkyrian Shoes [1] (30%), Valkyrian Manteau [1] (30%), Bloody Edge (25%), Randgris Card (0.01%)
- **MVP Drops**: Old Blue Box (50%), Old Card Album (20%), Old Purple Box (55%)
- **Card Effect**: Dispell on attack, +MDEF

#### Drake (ID: 1112)
- **Stats**: STR 85, AGI 80, VIT 49, INT 75, DEX 79, LUK 50
- **Skills**: Water Attack, Brandish Spear
- **Regular Drops**: Elunium (32%), Ring Pommel Saber [3] (9.5%), Saber [3] (6%), Drake Card (0.01%)
- **MVP Drops**: White Potion (50%), Amethyst (5%)
- **Card Effect**: Size penalty removal (100% damage to all sizes)

#### Hatii / Garm (ID: 1252)
- **Stats**: STR 85, AGI 126, VIT 82, INT 65, DEX 95, LUK 60
- **Skills**: Storm Gust, Cold Bolt, Frost Diver, Frost Nova, Adrenaline Rush, Endure, Two-Hand Quicken, Decrease Agility
- **Regular Drops**: Elunium (39.77%), Oridecon (29%), Fang of Hatii (55%), Katar of Frozen Icicle (5%), Hatii Claw [1] (5%), Ice Falchion (1.5%), Hatii Card (0.01%)
- **MVP Drops**: Old Blue Box (30%), Mystic Frozen (30%), Fang of Hatii (10%)

---

## 4. Spawn System

### Respawn Mechanics

- **Normal Monsters**: Respawn on a fixed timer after death. Timer is per-individual-mob, not global. Typical respawn times range from 3,000ms to 10,000ms depending on the monster.
- **MVP Bosses**: Respawn on a longer timer with a **10-minute variance window**. Example: Baphomet respawns "120-130 minutes" means the base timer is 120 minutes plus up to 10 minutes of random variance.
- **Timer Starts**: On the moment the monster dies, not when it is discovered dead.

### Spawn Types

| Type | Description | Examples |
|------|-------------|---------|
| **Fixed Position** | Monster always spawns at the same (x, y) coordinates. | Most dungeon bosses, Kafra area guards |
| **Random Position** | Monster spawns anywhere within the map boundaries or a defined area. | Most field and dungeon normal mobs |
| **Zone-Based** | Spawn coordinates are relative to a defined zone within the map. | Sabri_MMO implementation |
| **Player-Triggered** | Spawns only when certain conditions are met (e.g., quest flags). | Ktullanux (spawned via Ice Scale items) |

### Spawn Counts per Map (Classic RO)

Typical spawn density for popular maps:

| Map | Monster | Count | Respawn (s) |
|-----|---------|-------|-------------|
| prt_fild08 | Poring | 70 | 5 |
| prt_fild08 | Lunatic | 50 | 5 |
| prt_fild08 | Fabre | 40 | 5 |
| pay_dun01 | Zombie | 40 | 5 |
| pay_dun01 | Skeleton | 30 | 5 |
| moc_pryd01 | Mummy | 35 | 5 |
| moc_pryd01 | Verit | 20 | 5 |
| gef_dun01 | Hunter Fly | 25 | 7 |
| gef_dun01 | Marionette | 20 | 7 |
| gl_knt01 | Raydric | 30 | 7 |
| gl_knt01 | Abysmal Knight | 5 | 30 |
| anthell02 | Ant Egg | 40 | 5 |
| anthell02 | Maya (MVP) | 1 | 7200-7800 |
| prt_sewb4 | Golden Thief Bug (MVP) | 1 | 3600-4200 |

### MVP Tombstone System

When an MVP is killed:
1. A **tombstone object** spawns at the death location.
2. Speaking to the tombstone shows:
   - The MVP's name
   - Time of death
   - Name of the player who received the MVP award
3. The tombstone does **NOT** reveal:
   - When the MVP will respawn
   - Where it will respawn
4. The tombstone **disappears 5 seconds after the MVP respawns**.
5. Only applies to naturally spawned MVPs (not player-summoned or instance MVPs).

### Lazy Spawning (Sabri_MMO)

In the Sabri_MMO implementation, enemies are spawned lazily:
- Enemies for a zone are only created when the **first player enters that zone**.
- This is checked in `player:join` and `zone:change` handlers.
- Zone spawn configurations are stored in `ZONE_REGISTRY[zone].enemySpawns`.

---

## 5. Drop System

### Base Drop Rates

Each monster has up to **10 regular drop slots** and **3 MVP drop slots** (MVP bosses only). Each slot has an independent drop rate.

| Drop Category | Rate Range | Notes |
|--------------|------------|-------|
| Common drops | 50-100% | Consumables, cheap materials (Jellopy, herbs) |
| Uncommon drops | 10-49% | Mid-tier materials (Oridecon, Elunium) |
| Rare drops | 1-9.99% | Equipment, valuable materials |
| Very rare drops | 0.01-0.99% | High-end equipment, accessories |
| **Cards** | **0.01%** | Every monster has a card with this rate (1 in 10,000 kills) |
| MVP drops (slot 1) | 30-60% | Usually consumables (potions, boxes) |
| MVP drops (slot 2) | 10-55% | Mix of consumables and materials |
| MVP drops (slot 3) | 5-55% | Usually the rarest MVP reward |

### Card Drop Rate

- **Standard rate**: 0.01% (1 in 10,000)
- Cards are always the **last drop slot** in a monster's drop table.
- Cards are marked as `stealProtected: true` in rAthena (cannot be stolen via Steal skill).
- Some mini-boss cards may have slightly higher rates (0.02-0.05%).

### Bubble Gum Modifier

| Item | Effect | Duration |
|------|--------|----------|
| Bubble Gum | +100% drop rate (doubles all rates) | 30 minutes |
| HE Bubble Gum | +200% drop rate (triples all rates) | 15 minutes |
| Kafra Buff | +50% drop rate | 7 days |

**Rate Cap**: There is a **90% cap** on drop chances boosted by modifiers. A 50% base rate with Bubble Gum becomes 90% (capped), not 100%.

**Stacking**: Bubble Gum stacks multiplicatively with server rate modifiers but the final result is capped at 90%.

### Level Penalty (Optional)

When enabled (typically disabled on iRO):
- Players within 1-5 levels of a monster: 100% base drop rate.
- Greater level disparity: up to 50% penalty at 30+ levels difference.
- This was frequently disabled on official servers and most private servers.

### Party Loot Rules

RO has two party item distribution modes:

| Mode | Behavior |
|------|----------|
| **Each Take** | Items drop on the ground. Anyone in the party can pick them up. |
| **Party Share** | Items are automatically distributed to party members. Players dealing the most damage receive items more often. All party members receive items regardless of who picks up. |

### MVP Drop Distribution

- **MVP Award**: The player dealing the most total damage receives the "MVP" title for that kill.
- **MVP Drops**: 3 MVP-specific items roll independently. They are placed directly into the MVP-awarded player's inventory (not dropped on ground).
- **Regular Drops**: Still drop on the ground as normal. Anyone can pick them up.
- **MVP EXP Bonus**: The MVP-awarded player receives bonus base and job EXP.

---

## 6. Monster Skills

Monsters in RO use a separate skill database (`mob_skill_db`) that defines condition-action rules. Monsters can use both player-accessible skills (at enhanced power levels) AND monster-exclusive NPC skills.

### Skill Database Structure

Each entry in `mob_skill_db` defines:

```
MobID, InfoDummy, State, SkillID, SkillLv, Rate, CastTime, Delay, Cancelable, Target, ConditionType, ConditionValue
```

| Field | Description |
|-------|-------------|
| **State** | When the skill can trigger: `idle`, `walk`, `attack`, `angry`, `chase`, `follow`, `dead`, `loot`, `anytarget` |
| **SkillID** | The skill to cast (can be a player skill or NPC-exclusive skill) |
| **SkillLv** | Level of skill. If `> MAX_SKILL_LEVEL`, uses enhanced monster-only versions |
| **Rate** | Probability per tick (10000 = 100%). Most skills are 500-2000 (5-20%). |
| **CastTime** | Cast time in milliseconds (monsters can have instant casts) |
| **Delay** | Cooldown after use in milliseconds |
| **Target** | `target`, `self`, `friend`, `master`, `randomtarget`, `around1-8` (ground AoE) |
| **ConditionType** | What triggers the skill (see table below) |
| **ConditionValue** | Parameter for the condition |

### Skill Trigger Conditions

| Condition | Description | Value |
|-----------|-------------|-------|
| `always` | No condition, use whenever possible | -- |
| `myhpltmaxrate` | Monster HP below X% | Percentage (e.g., 50 = below 50% HP) |
| `friendhpltmaxrate` | Ally HP below X% | Percentage |
| `attackpcgt` | Number of players attacking > X | Count |
| `slavelt` | Slave count below X | Count |
| `slavele` | Slave count at or below X | Count |
| `closedattacked` | Hit by melee attack | -- |
| `longrangeattacked` | Hit by ranged attack | -- |
| `skillused` | Specific skill ID used on the mob | Skill ID |
| `casttargeted` | Player casting a skill within range | -- |
| `damagedgt` | Single hit damage exceeds X | Damage threshold |
| `rudeattacked` | Player attacks then immediately moves away | -- |

### Common Monster Skills

#### Offensive Skills (Player Skills at Enhanced Levels)

| Skill | Monster Users | Effect |
|-------|--------------|--------|
| **Bash (Lv 10+)** | Most melee mobs | High single-target physical damage |
| **Brandish Spear** | Baphomet, Maya, Drake | AoE physical damage in a cone |
| **Fire Bolt / Cold Bolt / Lightning Bolt** | Moonlight Flower, many caster mobs | Ranged elemental magic damage |
| **Jupitel Thunder** | Mistress, Stormy Knight | Strong Wind-element magic + knockback |
| **Lord of Vermillion** | Baphomet | Large AoE Wind-element damage |
| **Meteor Storm** | Dark Lord, Pharaoh, Amon Ra | Massive AoE Fire-element damage |
| **Storm Gust** | Hatii, Stormy Knight | Large AoE Water-element damage + Freeze |
| **Fire Wall** | Dark Lord, Pharaoh | Ground-placed fire barrier |
| **Earth Spike** | Moonlight Flower | Earth-element magic damage |
| **Grand Cross** | Valkyrie Randgris | Holy-element AoE centered on caster |
| **Mammonite** | Moonlight Flower | Physical + Zeny cost (very high damage) |

#### Offensive Skills (Monster-Exclusive NPC Skills)

| Skill | Effect |
|-------|--------|
| **NPC_COMBOATTACK** | Multi-hit physical attack at 100% ATK per hit |
| **NPC_DARKBREATH** | Shadow-property fixed damage breath attack |
| **NPC_FIREBREATH** | Fire-property breath attack |
| **NPC_ICEBREATH** | Water-property breath attack |
| **NPC_THUNDERBREATH** | Wind-property breath attack |
| **NPC_ACIDBREATH** | Earth-property breath attack |
| **NPC_DARKNESSATTACK** | Shadow-property melee modifier |
| **NPC_POISONATTACK** | Poison-property attack + Poison status |
| **NPC_STUNATTACK** | Physical attack + Stun status |
| **NPC_SLEEPATTACK** | Physical attack + Sleep status |
| **NPC_SILENCEATTACK** | Physical attack + Silence status |
| **NPC_BLINDATTACK** | Physical attack + Blind status |
| **NPC_CURSEATTACK** | Physical attack + Curse status |
| **NPC_BLEEDING** | Physical attack + Bleeding status |
| **NPC_CRITICALSLASH** | Guaranteed critical hit |
| **NPC_GRANDDARKNESS** | Large AoE Shadow-element attack |
| **NPC_DRAGONFEAR** | AoE fear + random status effects |

#### Defensive / Utility Skills

| Skill | Effect |
|-------|--------|
| **Heal** | HP recovery (monsters can heal themselves and allies) |
| **NPC_POWERUP** | Increase ATK for a duration |
| **NPC_AGIUP** | Increase movement and attack speed |
| **NPC_DEFENSE** | Increase DEF for a duration |
| **NPC_BARRIER** | Create a temporary invincible barrier |
| **NPC_INVINCIBLE** | Short-duration full invincibility |
| **NPC_STONESKIN** | Greatly increase physical DEF |
| **Safety Wall** | Block a number of melee hits |
| **Pneuma** | Block a number of ranged hits |
| **Endure** | Prevent flinching from attacks |

#### Summoning / Transformation Skills

| Skill | Effect |
|-------|--------|
| **NPC_SUMMONSLAVE** | Summon specific slave monsters. Slaves follow the master and share its aggro. |
| **NPC_CALLSLAVE** | Teleport all existing slaves back to the master's location, regardless of distance. |
| **NPC_SUMMONMONSTER** | Summon random monsters near the caster. |
| **NPC_METAMORPHOSIS** | Transform into a different monster type. The resulting monster has full HP, stats, EXP, and drops. |
| **NPC_TRANSFORM** | Change appearance (cosmetic only, no stat change). |

#### Utility / Movement Skills

| Skill | Effect |
|-------|--------|
| **NPC_RANDOMMOVE** | Teleport to a random location on the map |
| **NPC_EMOTION** | Display emotion icon (exclamation, heart, etc.) |
| **NPC_REBIRTH** | Resurrect as a different monster on death |
| **NPC_SELFDESTRUCTION** | Deal massive AoE damage centered on self, then die |

### Monster Power Levels

When `SkillLv > MAX_SKILL_LEVEL`, monsters use enhanced versions that exceed player caps:
- **SM_PROVOKE@10**: Reduces target DEF by 100% (no ATK bonus)
- **MG_FIREBALL@43**: Creates a 7x7 AoE with ~1000% damage
- **AL_HEAL@11+**: Heals significantly more than player max level
- **MG_COLDBOLT@20+**: 20-hit Cold Bolt with massive damage

### Example MVP Skill Tables

**Baphomet** (from mob_skill_db):
| State | Skill | Level | Rate | Condition |
|-------|-------|-------|------|-----------|
| attack | Brandish Spear | 20 | 500 | always |
| attack | Lord of Vermillion | 5 | 300 | attackpcgt 2 |
| chase | Dark Breath | 5 | 500 | always |
| attack | Heal | 10 | 200 | myhpltmaxrate 50 |
| idle | NPC_SUMMONSLAVE | 1 | 100 | slavelt 2 |
| attack | Maximize Power | 5 | 300 | always |

**Dark Lord**:
| State | Skill | Level | Rate | Condition |
|-------|-------|-------|------|-----------|
| attack | Meteor Storm | 10 | 400 | always |
| attack | Fire Wall | 10 | 300 | always |
| attack | Hell's Judgement | 5 | 200 | myhpltmaxrate 30 |
| chase | NPC_DARKBREATH | 5 | 500 | always |
| idle | NPC_SUMMONSLAVE | 1 | 100 | slavelt 3 |
| attack | Agility Up | 10 | 200 | always |

---

## 7. Monsters by Zone

### Prontera Region

#### Prontera Fields (prt_fild01 to prt_fild08)
Beginner fields surrounding the capital city. Level range: 1-15.

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Poring | 1 | Water 1 | Jellopy, Poring Card |
| Lunatic | 3 | Neutral 3 | Clover, Carrot, Lunatic Card |
| Fabre | 2 | Earth 1 | Fluff, Fabre Card |
| Pupa | 2 | Earth 1 | Chrysalis, Pupa Card |
| Drops | 3 | Fire 1 | Orange Juice, Drops Card |
| Willow | 4 | Earth 1 | Tree Root, Willow Card |
| Chonchon | 4 | Wind 1 | Shell, Chonchon Card |
| Condor | 5 | Wind 1 | Feather, Condor Card |
| Roda Frog | 5 | Water 1 | Spawn, Roda Frog Card |
| Rocker | 9 | Earth 1 | Grasshopper's Leg, Rocker Card |
| Picky | 5 | Fire 1 | Feather, Picky Card |
| Savage Babe | 7 | Earth 1 | Meat, Savage Babe Card |

#### Prontera Culvert / Sewers (prt_sewb1 to prt_sewb4)
Underground dungeon beneath Prontera. Level range: 15-64 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Thief Bug | 9 | Shadow 1 | Tooth of Bat |
| Thief Bug Female | 16 | Shadow 1 | Insect Feeler |
| Thief Bug Male | 18 | Shadow 1 | Insect Feeler |
| Thief Bug Egg | 5 | Shadow 1 | Shell |
| Familiar | 8 | Shadow 1 | Tooth of Bat |
| Tarou | 11 | Shadow 1 | Rat Tail, Tarou Card |
| Plankton | 17 | Water 1 | Single Cell |
| Drainliar | 26 | Shadow 2 | Tooth of Bat |
| **Golden Thief Bug (MVP)** | 64 | Fire 2 | Emperium, GTB Card |

### Payon Region

#### Payon Fields (pay_fild01 to pay_fild11)
Forested fields east/south of Payon village. Level range: 3-65 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Willow | 4 | Earth 1 | Tree Root |
| Spore | 16 | Water 1 | Spore, Mushroom |
| Poporing | 14 | Poison 1 | Sticky Mucus |
| Wormtail | 14 | Earth 1 | Worm Peeling |
| Boa | 15 | Earth 1 | Snake Scale |
| Bigfoot | 29 | Earth 1 | Bear's Footskin |
| Smokie | 18 | Earth 1 | Raccoon Leaf, Smokie Card |
| Wolf | 25 | Earth 1 | Wolf Claw |
| **Eddga (MVP)** | 65 | Fire 1 | Eddga Card, Tiger's Footskin |

#### Payon Cave / Dungeon (pay_dun00 to pay_dun04)
Five-floor dungeon beneath Payon. Level range: 15-67 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Zombie | 15 | Undead 1 | Decayed Nail |
| Skeleton | 20 | Undead 1 | Skel-Bone |
| Soldier Skeleton | 29 | Undead 1 | Gladius |
| Archer Skeleton | 31 | Undead 1 | Fire Arrow |
| Munak | 30 | Undead 1 | Munak Turban |
| Bongun | 32 | Undead 1 | Munak Turban |
| Sohee | 40 | Fire 1 | Sohee Card |
| Ghoul | 40 | Undead 2 | Horrendous Mouth |
| Ghostring (Mini-Boss) | 60 | Ghost 4 | Ghostring Card |
| **Moonlight Flower (MVP)** | 67 | Fire 3 | Silver Knife, Moonlight Card |

### Geffen Region

#### Geffen Fields (gef_fild00 to gef_fild14)
Diverse fields west of Geffen. Contains Orc territory. Level range: 20-77 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Orc Warrior | 24 | Earth 1 | Orcish Voucher |
| Orc Lady | 34 | Earth 2 | Heart of Orc Lady |
| Goblin | 25 | Wind 1 | Oridecon Stone |
| High Orc | 52 | Fire 2 | High Orc Card |
| **Orc Hero (MVP)** | 77 | Earth 2 | Heroic Emblem, Orc Hero Card |
| **Orc Lord (MVP)** | 74 | Earth 4 | Erde, Orc Lord Card |

#### Geffen Dungeon (gef_dun00 to gef_dun02)
Three-floor magic dungeon. Level range: 30-85 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Whisper | 34 | Ghost 3 | Fabric |
| Hunter Fly | 42 | Wind 2 | Zargon, Hunter Fly Card |
| Marionette | 41 | Ghost 3 | Puppet |
| Sage Worm | 46 | Shadow 1 | Bookclip |
| Jakk | 38 | Fire 2 | Pumpkin Head |
| **Dracula (MVP)** | 85 | Shadow 4 | Yggdrasil Berry, Dracula Card |
| **Doppelganger (MVP)** | 72 | Shadow 3 | Spiky Band, Doppelganger Card |

### Morroc Region

#### Morroc Fields / Sograt Desert (moc_fild01 to moc_fild22)
Desert fields surrounding Morroc. Level range: 5-69 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Scorpion | 24 | Fire 1 | Scorpion Tail |
| Condor | 5 | Wind 1 | Feather |
| Muka | 17 | Earth 1 | Cactus Needle |
| Anacondaq | 23 | Poison 1 | Venom Canine |
| Drops | 3 | Fire 1 | Jellopy |
| Hode | 29 | Earth 2 | Earthworm Peeling |
| **Phreeoni (MVP)** | 69 | Neutral 3 | Fortune Sword, Phreeoni Card |

#### Pyramid Dungeon (moc_pryd01 to moc_pryd06)
Six-floor pyramid dungeon. Level range: 30-88 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Mummy | 33 | Undead 2 | Rotten Bandage |
| Verit | 24 | Undead 1 | Cardinal Jewel |
| Isis | 47 | Shadow 1 | Shining Scale, Isis Card |
| Side Winder | 43 | Poison 1 | Venom Canine |
| Mimic | 35 | Neutral 3 | Old Blue Box |
| **Osiris (MVP)** | 78 | Undead 4 | Hand of God, Osiris Card |
| **Amon Ra (MVP)** | 88 | Earth 3 | Fragment of Rossata Stone |

#### Sphinx Dungeon (in_sphinx1 to in_sphinx5)
Five-floor dungeon south of Morroc. Level range: 40-93 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Marduk | 39 | Fire 1 | Marduk Card |
| Pasana | 45 | Fire 2 | Pasana Card |
| Minorous | 53 | Fire 2 | Hammer of Blacksmith |
| Anubis | 75 | Undead 4 | Anubis Card |
| **Pharaoh (MVP)** | 93 | Shadow 3 | Solar Sword, Pharaoh Card |

### Major Dungeon Zones

#### Glast Heim (gl_knt01, gl_cas01, gl_sew01, gl_chyard, etc.)
Massive multi-zone dungeon. The premier high-level grinding zone. Level range: 50-80 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Raydric | 52 | Shadow 2 | Elunium, Raydric Card |
| Abysmal Knight | 59 | Shadow 4 | Abysmal Knight Card |
| Raydric Archer | 52 | Shadow 2 | Steel Arrow |
| Khalitzburg | 58 | Undead 1 | Khalitzburg Card |
| Dark Priest | 72 | Undead 4 | Dead Branch, Dark Priest Card |
| Wind Ghost | 60 | Wind 3 | Rough Wind |
| Injustice | 55 | Shadow 2 | Zargon |
| Evil Druid | 56 | Undead 4 | Evil Druid Card |
| **Dark Lord (MVP)** | 80 | Undead 4 | Evil Bone Wand, Dark Lord Card |
| **Baphomet (MVP)** | 81 | Shadow 3 | Crescent Scythe, Baphomet Card |

#### Ant Hell (anthell01 to anthell02)
Two-floor insect dungeon. Level range: 20-81 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Andre | 17 | Earth 1 | Worm Peeling |
| Pierre | 17 | Earth 1 | Sticky Mucus |
| Deniro | 17 | Earth 1 | Shell |
| Ant Egg | 3 | Neutral 1 | Shell |
| Vitata | 24 | Earth 1 | Honey |
| **Maya (MVP)** | 81 | Earth 4 | Armlet of Obedience, Maya Card |

#### Sunken Ship (treasure01 to treasure02)
Two-floor undead dungeon. Level range: 30-70 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Whisper | 34 | Ghost 3 | Fabric |
| Pirate Skeleton | 32 | Undead 1 | Skel-Bone |
| Hydra | 18 | Water 2 | Sticky Mucus, Hydra Card |
| **Drake (MVP)** | 70 | Undead 1 | Ring Pommel Saber, Drake Card |

#### Turtle Island (tur_dun01 to tur_dun04)
Four-floor dungeon. Level range: 60-97 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Permeter | 55 | Shadow 2 | Sticky Mucus |
| Freezer | 62 | Water 2 | Mystic Frozen |
| Heater | 62 | Fire 2 | Flame Heart |
| **Turtle General (MVP)** | 97 | Earth 2 | Immaterial Sword, Turtle General Card |

#### Odin Temple (odin_tem01 to odin_tem03)
Three-floor Norse-themed dungeon. Level range: 70-99 (boss).

| Monster | Level | Element | Key Drops |
|---------|-------|---------|-----------|
| Frus | 72 | Neutral 2 | Elunium |
| Skeggiold | 73 | Holy 2 | Holy Water |
| Skogul | 76 | Shadow 3 | Gold |
| Randgris | 88 | Holy 3 | Valkyrie's Shield |
| **Valkyrie Randgris (MVP)** | 99 | Holy 4 | Valkyrian set, Randgris Card |

---

## 8. Implementation (Sabri_MMO)

This section documents how the RO Classic monster system is implemented in the Sabri_MMO project.

### Monster Template Data Structure

File: `server/src/ro_monster_templates.js` (509 monsters, auto-generated from rAthena pre-renewal mob_db.yml)

```javascript
RO_MONSTER_TEMPLATES['poring'] = {
    id: 1002,                          // RO monster ID
    name: 'Poring',                    // Display name
    aegisName: 'PORING',              // rAthena aegis name
    level: 1,                          // Monster level
    maxHealth: 50,                     // HP
    baseExp: 2, jobExp: 1, mvpExp: 0, // Experience rewards
    attack: 7, attack2: 10,           // ATK range (min-max)
    defense: 0, magicDefense: 5,      // DEF and MDEF
    str: 0, agi: 0, vit: 0,          // Base stats
    int: 0, dex: 6, luk: 30,
    attackRange: 50,                   // UE units (1 RO cell = 50 UE units)
    aggroRange: 0,                     // 0 = passive (does not auto-aggro)
    chaseRange: 600,                   // Max chase distance from aggro origin
    aspd: 163,                         // Attack speed
    walkSpeed: 400,                    // Movement speed (ms per RO cell)
    attackDelay: 1872,                 // ms between attacks
    attackMotion: 672,                 // Attack animation duration (ms)
    damageMotion: 480,                 // Hit stun duration (ms)
    size: 'medium',                    // small/medium/large
    race: 'plant',                     // Monster race
    element: { type: 'water', level: 1 }, // Element and level
    monsterClass: 'normal',            // normal/boss/mvp
    aiType: 'passive',                 // Simplified AI type string
    respawnMs: 5500,                   // Respawn timer (ms)
    raceGroups: {},                    // Additional race group flags
    stats: { ... },                    // Duplicate stats for damage formula
    modes: {},                         // Raw mode flags (if any)
    drops: [                           // Drop table (up to 10 slots)
        { itemName: 'Jellopy', rate: 70 },
        { itemName: 'Knife', rate: 1 },
        { itemName: 'Poring Card', rate: 0.01, stealProtected: true },
        // ...
    ],
    mvpDrops: [],                      // MVP-only drops (up to 3 slots)
};
```

### AI Code Lookup

File: `server/src/ro_monster_ai_codes.js` (1,004 monster ID -> AI code mappings)

```javascript
const MONSTER_AI_CODES = {
    1002:  2,   // Poring (Passive + Looter)
    1039: 21,   // Baphomet (Boss/MVP)
    1046: 21,   // Doppelganger (Boss/MVP)
    // ... 1,004 entries
};
```

### Mode Flag System

File: `server/src/index.js` (lines 132-213)

```javascript
// Hex mode bitmask constants
const MD = {
    CANMOVE:            0x0001,
    LOOTER:             0x0002,
    AGGRESSIVE:         0x0004,
    ASSIST:             0x0008,
    CASTSENSORIDLE:     0x0010,
    NORANDOMWALK:       0x0020,
    NOCAST:             0x0040,
    CANATTACK:          0x0080,
    CASTSENSORCHASE:    0x0200,
    CHANGECHASE:        0x0400,
    ANGRY:              0x0800,
    CHANGETARGETMELEE:  0x1000,
    CHANGETARGETCHASE:  0x2000,
    TARGETWEAK:         0x4000,
    RANDOMTARGET:       0x8000,
    MVP:                0x80000,
    KNOCKBACKIMMUNE:    0x200000,
    DETECTOR:           0x2000000,
    STATUSIMMUNE:       0x4000000,
};

// AI code -> hex mode bitmask lookup
const AI_TYPE_MODES = {
    1:  0x0081,   // Passive
    2:  0x0083,   // Passive + Looter
    // ... (all codes 1-27)
    21: 0x3695,   // Boss/MVP
};

// Parse bitmask to boolean flags
function parseModeFlags(hexMode) {
    return {
        canMove:            !!(hexMode & MD.CANMOVE),
        aggressive:         !!(hexMode & MD.AGGRESSIVE),
        assist:             !!(hexMode & MD.ASSIST),
        // ... 18 boolean flags
    };
}
```

### Server AI Tick Loop

File: `server/src/index.js` (lines 6877-7414+)

**Configuration:**
```javascript
const ENEMY_AI = {
    TICK_MS: 200,              // 5 ticks/second
    WANDER_PAUSE_MIN: 3000,    // 3s min between wanders
    WANDER_PAUSE_MAX: 8000,    // 8s max between wanders
    WANDER_DIST_MIN: 100,      // 100 UE units min wander
    WANDER_DIST_MAX: 300,      // 300 UE units max wander
    MOVE_BROADCAST_MS: 200,    // Position broadcast rate
    AGGRO_SCAN_MS: 500,        // Aggro scan interval
    ASSIST_RANGE: 550,         // 11 RO cells assist range
    CHASE_GIVE_UP_EXTRA: 200,  // Chase leash buffer
    IDLE_AFTER_CHASE_MS: 2000, // Idle delay after losing target
};
```

**Tick Loop Structure:**
```
setInterval(200ms) {
    for each enemy in enemies:
        if (isDead || DEAD state) skip
        if (zone has no players) skip  // optimization

        calculate hitStun flag

        switch (aiState):
            IDLE:
                if aggressive: scan for aggro target every 500ms
                if no target: process wander
            CHASE:
                validate target exists
                process pending target switch
                check chase range (give up if too far)
                check attack range (transition to ATTACK)
                move toward target
                check changeChase (switch to closer target)
            ATTACK:
                validate target exists
                process pending target switch
                randomTarget: pick random combatant
                range check (back to CHASE if out of range)
                hit stun check
                attack timing check
                EXECUTE ATTACK (damage formula, broadcast)
}
```

### Spawn Manager

File: `server/src/index.js` (lines 995-1140+)

**Spawn Configuration:**
```javascript
// Legacy: direct ENEMY_SPAWNS array (zones 1-3 only, 46 active spawns)
const ENEMY_SPAWNS = [
    { template: 'poring', x: 300, y: 300, z: 300, wanderRadius: 400 },
    // ... 46 entries for zones 1-3
];

// Modern: zone-based spawns in ZONE_REGISTRY
ZONE_REGISTRY.prontera_south.enemySpawns = [
    { template: 'poring', x: 300, y: 300, z: 300, wanderRadius: 400 },
    // ...
];
```

**spawnEnemy(spawnConfig) function:**
1. Look up template from `ENEMY_TEMPLATES[spawnConfig.template]`
2. Generate unique `enemyId`
3. Look up AI code from `MONSTER_AI_CODES[roId]`
4. Convert AI code to hex mode via `AI_TYPE_MODES[aiCode]`
5. Parse hex mode to boolean flags via `parseModeFlags(hexMode)`
6. Apply boss protocol if `monsterClass === 'boss' || 'mvp'`
7. Calculate movement speed from `walkSpeed` field
8. Create enemy object with all stats, position, AI state
9. Initialize wander state via `initEnemyWanderState()`
10. Broadcast `enemy:spawn` to zone

**Lazy Spawning:**
Enemies are only spawned when the first player enters a zone. The zone tracks whether it has been spawned via presence of enemies in the `enemies` Map for that zone.

### Aggro System

Key functions in `server/src/index.js`:

| Function | Lines | Purpose |
|----------|-------|---------|
| `setEnemyAggro(enemy, charId, hitType)` | 6981-7005 | Central aggro function, called from all damage paths |
| `triggerAssist(attackedEnemy, charId)` | 6956-6978 | Alert same-type allies within 550 UE units |
| `shouldSwitchTarget(enemy, charId, hitType)` | 6941-6953 | Mode-flag-based target switch decision |
| `findAggroTarget(enemy)` | 7008-7041 | Aggressive mob scan for closest player |
| `calculateEnemyDamage(enemy, charId)` | 7044-7076 | Enemy-to-player damage using RO formula |
| `enemyMoveToward(enemy, x, y, now, speed)` | 7079-7103 | Move enemy toward position + broadcast |
| `processWander(enemy, now)` | 7116-7147 | Idle wander subroutine |

### UE5 Client Enemy Actors

The client-side enemy system uses Blueprint-based actors managed by a central manager:

| Asset | Role |
|-------|------|
| `BP_EnemyManager` | Singleton manager. Holds Map of `enemyId -> BP_Enemy`. Handles `enemy:spawn`, `enemy:death`, `enemy:move`, `enemy:attack`, `enemy:health_update` socket events. |
| `BP_Enemy` | Individual enemy actor. Has skeletal mesh, health bar, name plate. Receives position updates and interpolates movement. Plays attack animations on `enemy:attack` events. |
| `BPI_Damageable` | Interface implemented by both BP_Enemy and BP_MMOCharacter. Enables unified damage handling. |
| `BPI_Targetable` | Interface for click-targeting. BP_Enemy implements this for skill targeting. |

**Socket Events (Server -> Client):**

| Event | Payload | Purpose |
|-------|---------|---------|
| `enemy:spawn` | `{ enemyId, templateId, name, level, x, y, z, health, maxHealth }` | Create/show enemy actor |
| `enemy:death` | `{ enemyId, killerName }` | Destroy/hide enemy actor, play death animation |
| `enemy:move` | `{ enemyId, x, y, z, targetX, targetY, isMoving }` | Update enemy position, interpolate movement |
| `enemy:attack` | `{ enemyId, targetId, attackMotion }` | Trigger attack animation on enemy actor |
| `enemy:health_update` | `{ enemyId, health, maxHealth, inCombat }` | Update floating health bar |

**Socket Events (Client -> Server):**

| Event | Payload | Purpose |
|-------|---------|---------|
| `combat:attack` | `{ targetId, isEnemy: true }` | Player starts auto-attacking an enemy |
| `skill:use` | `{ skillId, targetId, isEnemy: true }` | Player casts a targeted skill on enemy |

### Current Implementation Status

| Feature | Status | Notes |
|---------|--------|-------|
| 509 monster templates | Done | Auto-generated from rAthena mob_db |
| 1,004 AI code mappings | Done | Covers all pre-renewal monsters |
| Mode flag bitmask system | Done | 18 boolean flags parsed from hex |
| 4-state AI state machine | Done | IDLE/CHASE/ATTACK/DEAD |
| Aggro + target switching | Done | Full mode-flag-based switching |
| Assist trigger | Done | Same-type mobs within 550 UE units |
| Wander behavior | Done | Radius-clamped, 60% speed |
| Hit stun system | Done | damageMotion ms of inaction |
| Chase leash / give up | Done | chaseRange + 200 UE units |
| RO damage formula (enemy->player) | Done | Uses roPhysicalDamage() |
| Zone-scoped AI | Done | AI only processes in active zones |
| Active zones: 1-3 | Done | ~46 spawn points |
| Zones 4-9, dungeons | Not Yet | Monster spawns defined but disabled |
| Monster skills | Not Yet | No mob_skill_db implementation |
| MVP tombstone system | Not Yet | No client-side tombstone actor |
| Loot/drop on kill | Partial | Drop data in templates, no client integration |
| Boss protocol (knockback/status) | Done | Auto-applied for boss/mvp class |
| Slave summoning | Not Yet | No NPC_SUMMONSLAVE implementation |

---

## References

- [rAthena Monster Mode Flags Documentation](https://github.com/rathena/rathena/blob/master/doc/mob_db_mode_list.txt)
- [rAthena Monster AI Types (Aegis State Machine) -- Issue #926](https://github.com/rathena/rathena/issues/926)
- [rAthena Monster Database Documentation](https://github.com/rathena/rathena/blob/master/doc/mob_db.txt)
- [rAthena Monster Source Code](https://github.com/rathena/rathena/blob/master/src/map/mob.cpp)
- [rAthena Pre-Renewal Monster Database](https://pre.pservero.com/monster/list)
- [RateMyServer Pre-Renewal Monster Database](https://ratemyserver.net/index.php?page=mob_db)
- [RateMyServer MVP Boss Monsters](https://ratemyserver.net/index.php?page=mob_db&mvp=1&mob_search=Search&sort_r=0)
- [iRO Wiki -- MVP System](https://irowiki.org/wiki/MVP)
- [iRO Wiki -- Drop System](https://irowiki.org/wiki/Drop_System)
- [iRO Wiki -- Monster Exclusive Skills](https://irowiki.org/wiki/Category:Monster_Exclusive_Skills)
- [DeepWiki -- rAthena Monster and NPC System](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
- [rAthena Permanent Monster Spawn Wiki](https://github.com/rathena/rathena/wiki/Permanent_Monster_Spawn)
- [Ragnarok MVP Timer](https://ragnarok-mvp-timer.com/)
