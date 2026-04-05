# Monster AI & Behavior -- Deep Research (Pre-Renewal)

> Comprehensive reference for replicating Ragnarok Online Classic (pre-renewal) monster AI in an MMO game server.
> Covers the AI tick loop, state machine, mode flags, AI type codes, aggro system, movement, spawning, monster stats, and combat formulas.
> Verified against rAthena pre-renewal source (`src/map/mob.cpp`, `doc/mob_db_mode_list.txt`, `doc/mob_db.txt`), Aegis server behavior (rAthena Issue #926), iRO Wiki Classic, RateMyServer pre-re database, and DeepWiki rAthena analysis.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Monster Modes/Flags](#2-monster-modesflags)
3. [AI Types (Codes 01-27)](#3-ai-types-codes-01-27)
4. [State Machine](#4-state-machine)
5. [Aggro System](#5-aggro-system)
6. [Movement](#6-movement)
7. [Spawn System](#7-spawn-system)
8. [Monster Stats](#8-monster-stats)
9. [Monster Skills](#9-monster-skills)
10. [Implementation Checklist](#10-implementation-checklist)
11. [Gap Analysis](#11-gap-analysis)

---

## 1. Overview

### Core Architecture

Ragnarok Online Classic uses an **Aegis-derived AI system** where each monster is assigned:
1. An **AI type code** (01-27) that maps to a hexadecimal mode bitmask
2. A **mode bitmask** (18+ boolean behavior flags) that the server-side state machine reads every tick
3. A **skill database** (`mob_skill_db`) that defines condition-action rules for skill usage per AI state

The server runs all monster AI centrally. There is no client-side AI -- the client only receives position updates, attack events, and death/spawn notifications. All decisions (target selection, movement, attack timing, skill usage) happen server-side.

### AI Tick Loop

The AI processes every monster on the server on a fixed interval:

| Parameter | Aegis/rAthena Value | Notes |
|-----------|-------------------|-------|
| **Tick interval** | 100ms (10 ticks/sec) | rAthena `MIN_MOBTHINKTIME = 100`. Aegis uses the same 100ms base. |
| **Active AI range** | `AREA_SIZE + 2` cells | Monsters only run full AI when players are within this range. Outside it, monsters are "lazy" (reduced processing). |
| **Idle skill interval** | Every 10 ticks (1 sec) | Active idle skills trigger every 1000ms (`IDLE_SKILL_INTERVAL = 10`). |
| **Walk delay** | Per-monster `walkSpeed` ms/cell | Controls movement speed between cells. |
| **Attack delay** | Per-monster `attackDelay` ms | Controls time between auto-attacks. |

**Lazy AI optimization**: Monsters far from any player (outside `AREA_SIZE + ACTIVE_AI_RANGE` cells) skip most AI processing. They do not scan for targets, do not wander, and do not process skills. This is critical for performance -- maps can have hundreds of spawned monsters, but only those near players need full AI.

### Tick Processing Order (per monster per tick)

1. Check if monster is in active AI range of any player
2. If dead, skip (respawn timer handled separately)
3. Process hit stun (`damageMotion` delay after taking damage)
4. Process walk delay (`canmove_tick` -- cannot move until walk animation completes)
5. Process action delay (`canact_tick` -- cannot act/attack until action animation completes)
6. Run state-specific logic (IDLE / CHASE / ATTACK)
7. Process skill usage (check `mob_skill_db` conditions)
8. Broadcast position updates if moved

---

## 2. Monster Modes/Flags

### Mode Flag Bitmask (Complete Reference)

Each monster has a mode field that is a bitmask of behavior flags. These flags are the atomic building blocks of all monster behavior. The AI type code simply maps to a pre-defined combination of these flags.

**Source**: `rathena/doc/mob_db_mode_list.txt`, confirmed against Aegis behavior.

| Flag Name | Hex Bit | Decimal | Description |
|-----------|---------|---------|-------------|
| `MD_CANMOVE` | `0x0001` | 1 | Monster can move and chase players. Without this, the mob is rooted in place (e.g., Mandragora, Flora). |
| `MD_LOOTER` | `0x0002` | 2 | Picks up items dropped on the ground when in idle state. When killed, looted items drop from the monster. |
| `MD_AGGRESSIVE` | `0x0004` | 4 | Actively scans for nearby players to attack. Does not wait to be hit first. |
| `MD_ASSIST` | `0x0008` | 8 | When a nearby mob of the same class (same `templateId`) is attacked, this mob joins the fight against the attacker. |
| `MD_CASTSENSORIDLE` | `0x0010` | 16 | Detects and aggros players who begin casting a skill while the monster is in IDLE state, even if the skill does not target the monster. |
| `MD_NORANDOMWALK` | `0x0020` | 32 | Disables random wandering. Monster stands still during IDLE unless it has a target. Used for slaves (AI 24), guardians. |
| `MD_NOCAST` | `0x0040` | 64 | Monster cannot use skills. Purely melee auto-attack only. |
| `MD_CANATTACK` | `0x0080` | 128 | Monster can perform attacks. Without this flag, the monster is completely passive (e.g., Pupa, Eggs, Plants with no ATK). |
| `MD_CASTSENSORCHASE` | `0x0200` | 512 | Detects players casting skills even while the monster is in CHASE state (not just idle). Extends cast sensor to be active during pursuit. |
| `MD_CHANGECHASE` | `0x0400` | 1024 | While chasing one target, if a different player in `inCombatWith` is closer AND within attack range, switch to that player. Active target-of-opportunity during pursuit. |
| `MD_ANGRY` | `0x0800` | 2048 | "Hyper-active" mode. Monster has extra pre-attack states ("follow" and "angry") in addition to normal "chase" and "attack". Before being attacked, uses a different skill set. After being hit, reverts to normal states. While in "follow" state, automatically switches to the closest character. |
| `MD_CHANGETARGETMELEE` | `0x1000` | 4096 | Switches target when hit by a **melee attack** (not skills, not ranged). Only triggers during IDLE or ATTACK states. On Aegis, the distinction is between "ATTACKED" (any damage) and "MELEE ATTACKED" (physical melee only -- skills do NOT trigger this, even melee skills). |
| `MD_CHANGETARGETCHASE` | `0x2000` | 8192 | Switches target when hit by any attack during CHASE state. Unlike melee target change, this triggers on any damage type while the monster is pursuing a target. |
| `MD_TARGETWEAK` | `0x4000` | 16384 | Only aggros players whose base level is 5+ levels below the monster's level. Players at or above `monsterLevel - 5` are ignored by the aggro scan. |
| `MD_RANDOMTARGET` | `0x8000` | 32768 | Picks a random target from its attacker list (`inCombatWith`) each auto-attack swing. Completely ignores proximity or damage dealt. |
| `MD_IGNOREMELEE` | `0x10000` | 65536 | Immune to melee physical damage. |
| `MD_IGNOREMAGIC` | `0x20000` | 131072 | Immune to magical damage. |
| `MD_IGNORERANGED` | `0x40000` | 262144 | Immune to ranged physical damage. |
| `MD_MVP` | `0x80000` | 524288 | MVP protocol: grants MVP rewards on kill, tombstone on death, special EXP distribution. Makes mob resistant to Coma. |
| `MD_IGNOREMISC` | `0x100000` | 1048576 | Immune to misc/trap damage. |
| `MD_KNOCKBACKIMMUNE` | `0x200000` | 2097152 | Cannot be knocked back by any skill (Arrow Repel, Storm Gust, Bowling Bash, etc.). |
| `MD_TELEPORTBLOCK` | `0x400000` | 4194304 | Cannot be teleported by skills. |
| `MD_FIXEDITEMDROP` | `0x1000000` | 16777216 | Drop rates are not affected by server rate modifiers. Always uses base rates. |
| `MD_DETECTOR` | `0x2000000` | 33554432 | Can detect and attack hidden/cloaked players (Hiding, Cloaking, Chase Walk). |
| `MD_STATUSIMMUNE` | `0x4000000` | 67108864 | Immune to all status effects (Stun, Freeze, Stone Curse, Blind, Sleep, etc.). |
| `MD_SKILLIMMUNE` | `0x8000000` | 134217728 | Immune to being affected by skills entirely. |

### Detailed Flag Behavior

#### Aggressive (MD_AGGRESSIVE)

- Monster scans for the closest player within its `aggroRange` every tick (or at a configured scan interval).
- Scan occurs only during IDLE state.
- Target selection: closest qualifying player wins. No threat/hate table for initial aggro.
- If `MD_TARGETWEAK` is also set, only players 5+ levels below the monster qualify.
- Once a target is found, the monster records its current position as "aggro origin" and transitions to CHASE (or ATTACK if immobile).
- Aggressive monsters are the most common enemy type in dungeons and higher-level fields.
- **Detection range**: The `aggroRange` field in the monster template determines how far the monster can "see" players. Typical values: 300-500 UE units (6-10 RO cells). Boss/MVP monsters often have larger ranges.

#### Assist (MD_ASSIST)

- When a monster with `MD_ASSIST` is in IDLE state and a nearby monster of the **same class** (`templateId`) takes damage, the assisting monster joins the fight.
- **Assist range**: Approximately 11 RO cells (550 UE units in Sabri_MMO). rAthena uses a cell-based range check.
- The assisting monster targets the **player who attacked the original mob**, not the closest player.
- Assist only triggers for monsters that are: same `templateId`, alive, in IDLE state, have both `canAttack` and `canMove` flags.
- Assist does NOT chain -- if monster B assists monster A, nearby monster C does not also assist due to B's state change (C would need to be triggered by a direct attack on another C-type mob, or by an attack on a B-type mob if C has the same templateId as B).
- **Common assist monsters**: Hornet (AI 03), Condor, Wolf, Peco Peco, Orc Warrior, Steel Chonchon.

#### Looter (MD_LOOTER)

- Monster picks up items dropped on the ground when in IDLE state.
- Looted items are stored internally on the monster.
- When the looter monster is killed, all looted items drop from the monster's corpse in addition to its normal drop table.
- Looter scan occurs at idle intervals, checking for nearby ground items within a small range.
- **Common looter monsters**: Poring (AI 02), Poporing, Verit, Thief Bug (AI 07), Yoyo, Metaller.

#### Cast Sensor Idle (MD_CASTSENSORIDLE)

- When a player begins casting a skill (cast bar starts) and the monster is in IDLE state, the monster detects the cast and aggros the caster.
- The detection range is the monster's `aggroRange` (same as aggressive scan range).
- This triggers even if the skill does not target the monster -- simply casting near a cast-sensor monster is enough.
- **Key distinction from Aggressive**: Aggressive monsters scan for proximity alone. Cast Sensor Idle monsters are passive (no aggro scan) UNTIL a player casts a skill. Then they react.
- **Common cast sensor idle monsters**: Wormtail (AI 17), Anacondaq, Smokie, Bigfoot, Golem.

#### Cast Sensor Chase (MD_CASTSENSORCHASE)

- Extends cast sensing to the CHASE state. Normally cast sensor only activates during IDLE.
- With this flag, if the monster is chasing Player A and Player B starts casting a skill nearby, the monster may switch to Player B.
- Used by boss-type AI (AI 20, 21) and special aggressive mobs.

#### Angry (MD_ANGRY)

- "Hyper-active" mode that adds two extra states to the state machine: **Follow** and **Angry**.
- **Before being attacked**: The monster uses "follow" and "angry" states, which have a separate skill set in `mob_skill_db`.
- **After being attacked**: The monster reverts to normal "chase" and "attack" states with the standard skill set.
- While in "follow" state, the monster automatically switches to whichever character is closest -- it is highly mobile and reactive.
- The transition from angry/follow to normal chase/attack happens on the first damage received.
- **Common angry monsters**: Familiar (AI 04), Zombie, Orc Warrior, Mummy, Ghoul, Hunter Fly.

#### Change Target Melee (MD_CHANGETARGETMELEE)

- When a player deals a **melee auto-attack** (not a skill, even if melee range) to the monster while it is in ATTACK or IDLE state, the monster switches its target to the new attacker.
- **Aegis distinction**: There are two separate events on Aegis -- "ATTACKED" (any damage source) and "MELEE ATTACKED" (melee auto-attack only). The "MELEE ATTACKED" event does NOT trigger for skills, not even melee-range skills like Bash or Magnum Break.
- This flag is commonly paired with `MD_CHANGETARGETCHASE` for mobs that switch targets freely.

#### Change Target Chase (MD_CHANGETARGETCHASE)

- When the monster takes any damage while in CHASE state, it switches its target to the new attacker.
- Unlike melee target change, this triggers on ANY damage type (melee, ranged, magic, misc).
- Combined with `MD_CHANGETARGETMELEE`, creates a monster that switches targets very actively.

#### Change Chase (MD_CHANGECHASE)

- While chasing one target, the monster checks if any other player in its `inCombatWith` set is currently within attack range.
- If a closer-in-range player is found, the monster immediately switches to attacking that player instead of continuing the chase.
- This is a target-of-opportunity behavior -- the monster does not go out of its way, but if someone is already in reach while it is chasing someone else, it takes the easier target.
- Primarily used by boss/MVP AI (AI 21).

#### Target Weak (MD_TARGETWEAK)

- Modifies the aggressive scan to only target players whose base level is at least 5 levels below the monster's level.
- Formula: player must have `playerLevel < monsterLevel - 5` to be eligible for aggro.
- Players at or above `monsterLevel - 5` are invisible to the aggro scan.
- This creates "bully" monsters that only pick on weaker players.
- Rare AI type (AI 08).

#### Random Target (MD_RANDOMTARGET)

- Each auto-attack swing, the monster picks a completely random target from its `inCombatWith` set.
- Ignores proximity, damage dealt, and all other targeting criteria.
- Creates unpredictable behavior where the monster constantly switches between attackers.
- Used by special aggressive mobs (AI 26, 27).

### Boss Protocol (Automatic Flags)

Monsters with `monsterClass === 'boss'` or `monsterClass === 'mvp'` automatically receive:

| Auto-Flag | Effect |
|-----------|--------|
| `MD_KNOCKBACKIMMUNE` | Cannot be displaced by any skill |
| `MD_STATUSIMMUNE` | Immune to all status effects (Stun, Freeze, Stone Curse, etc.) |
| `MD_DETECTOR` | Can detect and attack hidden/cloaked players |

Additionally, MVP-class monsters receive `MD_MVP` for the MVP reward protocol.

Boss Protocol also grants practical immunities:
- **Rude Attack immunity**: Regular monsters teleport when hit by "rude attacks" (attacks from positions the monster cannot reach, e.g., behind Ice Wall). Boss monsters do NOT teleport from rude attacks.
- **Coma resistance**: The MVP flag makes the monster resistant to Coma (instant-kill to 1 HP).
- **Gravity/Death resistance**: Boss-type monsters are immune to instant-death effects.

---

## 3. AI Types (Codes 01-27)

Each monster has an AI type code from the mob database. The code maps to a hex mode bitmask that the server parses into boolean flags. Not all codes 01-27 are actively used; some are reserved, duplicates, or unused in the pre-renewal database.

**Source**: rAthena Issue #926 (Aegis state machine verification), `doc/mob_db_mode_list.txt`.

### Complete AI Type Code Table

| Code | Hex Mode | Active Flags | Behavior Summary | Example Monsters |
|------|----------|-------------|------------------|------------------|
| **01** | `0x0081` | CanMove, CanAttack | **Passive**: Wanders and retaliates when hit. Does not aggro. The most basic AI. | Fabre, Willow, Chonchon, Spore, Rocker |
| **02** | `0x0083` | CanMove, Looter, CanAttack | **Passive + Looter**: Same as 01 but picks up items on the ground during idle. | Poring, Poporing, Verit |
| **03** | `0x1089` | CanMove, Assist, CanAttack, ChangeTargetMelee | **Passive + Assist**: Retaliates when hit. Nearby same-type mobs join the fight. Switches target on melee hit. | Hornet, Condor, Wolf, Peco Peco |
| **04** | `0x3885` | CanMove, Aggressive, CanAttack, Angry, ChangeTargetMelee, ChangeTargetChase | **Angry/Hyper-Active**: Aggressive with angry pre-attack states and full target switching. Uses different skill sets before/after being attacked. | Familiar, Zombie, Orc Warrior, Mummy, Ghoul, Hunter Fly |
| **05** | `0x2085` | CanMove, Aggressive, CanAttack, ChangeTargetChase | **Aggressive + Chase Switch**: Actively seeks players. Switches to closer targets while chasing. | Archer Skeleton |
| **06** | `0x0000` | (none) | **Immobile/Plant**: Cannot move, cannot attack, no AI. Used for eggs, mushrooms, and objects. Drops items when killed but has no behavior. | Pupa, Peco Peco Egg, Red/Blue/Yellow/White Plant, Shining Plant |
| **07** | `0x108B` | CanMove, Looter, Assist, CanAttack, ChangeTargetMelee | **Passive + Looter + Assist**: Picks up items, assists allies, switches melee target. Most complex passive AI. | Thief Bug, Thief Bug Female, Yoyo, Metaller, Steel Chonchon |
| **08** | `0x7085` | CanMove, Aggressive, CanAttack, TargetWeak, ChangeTargetMelee, ChangeTargetChase | **Aggressive + Target Weak**: Prioritizes players 5+ levels below the monster. Full target switching. | Rare -- select monsters only |
| **09** | `0x3095` | CanMove, Aggressive, CastSensorIdle, CanAttack, ChangeTargetMelee, ChangeTargetChase | **Aggressive + Cast Sensor**: Detects players casting skills while idle. Switches targets actively on both melee hit and chase. | Scorpion, Isis, Elder Willow, Side Winder |
| **10** | `0x0084` | Aggressive, CanAttack | **Immobile Turret**: Cannot move but attacks anything in range. Aggressive scan within its (short) attack range. | Mandragora, Flora, Geographer, Hydra |
| **11** | `0x0084` | Aggressive, CanAttack | **WoE Guardian (Immobile)**: Same bitmask as 10. Functionally identical but semantically separated for WoE guardians. | War of Emperium Guardians (stationary) |
| **12** | `0x2085` | CanMove, Aggressive, CanAttack, ChangeTargetChase | **WoE Guardian (Mobile)**: Same bitmask as 05. Mobile WoE guardians that chase and switch chase targets. | War of Emperium Guardians (mobile) |
| **13** | `0x308D` | CanMove, Aggressive, Assist, CanAttack, ChangeTargetMelee, ChangeTargetChase | **Aggressive + Assist**: Aggressive mob that also calls same-type allies when attacked. Full target switching. | Thief Bug Male |
| **17** | `0x0091` | CanMove, CastSensorIdle, CanAttack | **Passive + Cast Sensor**: Does not aggro normally. Detects and attacks players who cast skills nearby while idle. Effectively passive until magic is used. | Wormtail, Anacondaq, Smokie, Bigfoot, Golem |
| **19** | `0x3095` | CanMove, Aggressive, CastSensorIdle, CanAttack, ChangeTargetMelee, ChangeTargetChase | **Aggressive + Cast Sensor (variant)**: Same flag set as 09 but distinct Aegis state machine behavior internally. | Select monsters |
| **20** | `0x3295` | CanMove, Aggressive, CastSensorIdle, CastSensorChase, CanAttack, ChangeTargetMelee, ChangeTargetChase | **Enhanced Cast Sensor**: Cast sensor active during both idle AND chase states. Aggressive with full target switching. | Aggressive ranged monsters with repositioning |
| **21** | `0x3695` | CanMove, Aggressive, CastSensorIdle, CastSensorChase, ChangeChase, CanAttack, ChangeTargetMelee, ChangeTargetChase | **Boss/MVP AI**: The most active AI type. Aggressive, full target switching on all conditions, cast sensor in all states, chase-change (switches to closer in-range targets during chase). | Osiris, Baphomet, Doppelganger, Mistress, all MVPs |
| **24** | `0x00A1` | CanMove, NoRandomWalk, CanAttack | **Slave (Passive)**: Summoned by other mobs via NPC_SUMMONSLAVE. Stays near master, no random wandering. Attacks master's target. | Summoned slaves (e.g., Bigfoot slaves from Eddga, Dark Illusions from Baphomet) |
| **25** | `0x0001` | CanMove | **Pet**: Can move only. Cannot attack -- purely cosmetic/companion AI. Follows owner. | Pet versions of monsters |
| **26** | `0xB695` | CanMove, Aggressive, CastSensorIdle, CastSensorChase, ChangeChase, CanAttack, ChangeTargetMelee, ChangeTargetChase, RandomTarget | **Chaotic Aggressive**: Like AI 21 (Boss) but with RandomTarget -- picks random targets each swing. The most unpredictable AI. | Special aggressive mobs |
| **27** | `0x8084` | Aggressive, CanAttack, RandomTarget | **Immobile + Random**: Cannot move but attacks aggressively and randomizes targets. | Special summon/spawn AI |

**Unused codes**: 14, 15, 16, 18, 22, 23 are unused in the pre-renewal mob database.

### AI Type Distribution

Based on the 509-monster pre-renewal database:

| AI Type | Count | Percentage | Description |
|---------|-------|-----------|-------------|
| 01 | ~100 | ~20% | Passive (basic) |
| 02 | ~30 | ~6% | Passive + Looter |
| 03 | ~80 | ~16% | Passive + Assist |
| 04 | ~90 | ~18% | Angry/Hyper-Active |
| 05 | ~25 | ~5% | Aggressive + Chase Switch |
| 06 | ~20 | ~4% | Plants/Immobile |
| 07 | ~40 | ~8% | Passive + Looter + Assist |
| 09 | ~40 | ~8% | Aggressive + Cast Sensor |
| 10 | ~15 | ~3% | Immobile Turret |
| 17 | ~25 | ~5% | Passive + Cast Sensor |
| 21 | ~30 | ~6% | Boss/MVP AI |
| Other | ~14 | ~3% | Slaves, pets, specials |

---

## 4. State Machine

### States

The monster AI state machine has **4 primary states** and, for `MD_ANGRY` monsters, 2 additional sub-states:

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

    Additional for MD_ANGRY mobs:
    +--------+     +-------+
    | FOLLOW |---->| ANGRY |    (before first damage)
    +--------+     +-------+
         |              |
         +---- hit ---->+---> CHASE (normal states)
```

#### IDLE State

The default state. Monster has no target and is either standing still or wandering.

**Processing order each tick**:

1. **Aggressive scan** (if `MD_AGGRESSIVE`): Every scan interval (default 500ms in Sabri_MMO, every tick in rAthena), iterate all players in the zone. Find the closest player within `aggroRange`. If `MD_TARGETWEAK`, filter out players at or above `monsterLevel - 5`. If a target is found, record current position as "aggro origin", transition to CHASE (or ATTACK if `!MD_CANMOVE`).

2. **Cast sensor check** (if `MD_CASTSENSORIDLE`): If any player within `aggroRange` is currently casting a skill (has an active cast bar), aggro that player. Transition to CHASE.

3. **Assist check**: Handled externally via `triggerAssist()` -- when a same-type mob nearby is attacked, this mob transitions to CHASE targeting the attacker.

4. **Wander** (if no target found): If `MD_CANMOVE` and NOT `MD_NORANDOMWALK`, pick a random point within wander range and walk toward it at reduced speed. Pause 3-8 seconds between wanders.

5. **Idle skill usage**: Check `mob_skill_db` for skills with `state: idle`. Roll against skill rate.

#### CHASE State

Monster is pursuing a target player.

**Processing order each tick**:

1. **Target validation**: Check if target is still alive, connected, and in the same zone. If not, pick the next target from `inCombatWith`. If no valid targets remain, return to IDLE.

2. **Pending target switch**: If a damage hook set `pendingTargetSwitch`, consume it and update the target.

3. **Chase range check**: Calculate distance from monster's current position to its "aggro origin" (where it first detected the target). If distance exceeds `chaseRange + giveUpExtra`, the monster gives up and returns to IDLE. It does NOT teleport back -- it simply stops chasing and begins wandering back naturally.

4. **Attack range check**: If distance to target is within `attackRange + tolerance`, stop moving and transition to ATTACK.

5. **Movement**: If not in hit stun (`damageMotion` ms after taking damage), move toward target at full `moveSpeed`. Movement is calculated as `moveSpeed * (tickMs / 1000)` UE units per tick.

6. **Change Chase** (if `MD_CHANGECHASE`): While chasing, check if any other player in `inCombatWith` is closer AND within attack range. If so, switch to that player immediately (they are already in reach, so skip straight to ATTACK).

7. **Cast sensor chase** (if `MD_CASTSENSORCHASE`): If any player within range is casting a skill, potentially switch to that player.

8. **Chase skill usage**: Check `mob_skill_db` for skills with `state: chase`. Roll against skill rate.

#### ATTACK State

Monster is within attack range and auto-attacking its target.

**Processing order each tick**:

1. **Target validation**: Same as CHASE -- check target alive/connected/in-zone.

2. **Pending target switch**: Consume any pending switch from damage hooks.

3. **Random target** (if `MD_RANDOMTARGET`): If multiple players in `inCombatWith`, pick one at random for this swing. Filter out dead/disconnected players first.

4. **Range check**: If target has moved beyond `attackRange + tolerance + buffer` (small buffer to prevent oscillation), transition back to CHASE.

5. **Hit stun check**: If in hit stun, skip attack but continue state checks.

6. **Attack timing**: If `now - lastAttackTime < attackDelay`, skip (not enough time has passed since last attack).

7. **Execute attack**: Set `lastAttackTime = now`. Calculate damage using the physical damage formula. Apply damage to target. Broadcast combat events. Check for player death.

8. **Attack skill usage**: Check `mob_skill_db` for skills with `state: attack`. Roll against skill rate. Skills can replace the auto-attack for that tick.

9. **Target change on melee** (if `MD_CHANGETARGETMELEE`): If hit by a melee auto-attack from a different player, switch target to the new attacker. Note: this is triggered from the damage hook (`setEnemyAggro`), not from within the ATTACK state logic itself.

#### DEAD State

Monster HP has reached 0.

1. Mark as dead (`isDead = true`).
2. Stop all combat interactions. Clear `inCombatWith`.
3. Cancel any active auto-attackers targeting this monster.
4. Award EXP to the killer (and party members if applicable).
5. Roll and distribute loot drops.
6. Broadcast `enemy:death` to all players in zone.
7. Start respawn timer (`setTimeout` with `respawnMs`).
8. On respawn: restore HP to max, reset position to spawn coordinates, reset AI state to IDLE, broadcast `enemy:spawn`.

#### Follow/Angry Sub-States (MD_ANGRY only)

For monsters with the `MD_ANGRY` flag (AI type 04):

- **Follow**: Before being attacked, the monster uses "follow" behavior. It automatically switches to whichever character is closest. It uses a different skill set (`state: follow` in `mob_skill_db`). Movement is aggressive but reactive.
- **Angry**: A transitional state before the first damage is received. The monster uses `state: angry` skills.
- **Transition**: On first damage received, the monster permanently transitions to normal CHASE/ATTACK states for the remainder of that combat engagement. The angry/follow states reset when the monster returns to IDLE (all targets lost).

### State Transition Summary

| From | To | Trigger |
|------|----|---------|
| IDLE | CHASE | Aggressive scan finds target, cast sensor detects caster, assist triggered, damaged by player |
| IDLE | ATTACK | Same triggers as CHASE, but monster has `!MD_CANMOVE` (immobile) |
| CHASE | ATTACK | Distance to target <= `attackRange + tolerance` |
| CHASE | IDLE | Target lost (dead/disconnected/left zone), all `inCombatWith` empty, chase range exceeded |
| ATTACK | CHASE | Target moved beyond `attackRange + tolerance + buffer` |
| ATTACK | IDLE | Target lost, all `inCombatWith` empty |
| Any | DEAD | HP reaches 0 |
| DEAD | IDLE | Respawn timer completes |

---

## 5. Aggro System

### Overview

Ragnarok Online Classic does NOT use a threat/hate table for aggro. The aggro system is rule-based, driven by mode flags. Target selection follows simple priority rules without accumulating threat values.

This is fundamentally different from games like World of Warcraft where healers generate threat and tanks need to maintain aggro through damage output. In RO:
- The **first player to attack** generally becomes the permanent target (for mobs without target-switching flags).
- Mobs WITH target-switching flags change based on recent hits, not cumulative damage.
- There is no way to "out-threat" another player through healing or damage.

### Core Aggro Function: `setEnemyAggro()`

Called from ALL damage paths -- auto-attack, all skill types, ground effects, DoT ticks.

**Logic**:
1. Skip if monster is dead or cannot attack (plant-type).
2. Add attacker to `inCombatWith` set.
3. Record `lastDamageTime` for hit stun calculation.
4. Determine if target should switch (see `shouldSwitchTarget()` below).
5. If switching: set new target. If previously in IDLE, record aggro origin position. Transition to CHASE (or ATTACK if immobile).
6. Call `triggerAssist()` to alert nearby same-type allies.

### Target Switching Decision: `shouldSwitchTarget()`

| Condition | Result |
|-----------|--------|
| No current target | Always switch |
| Same attacker as current target | Never switch (no-op) |
| `MD_RANDOMTARGET` flag set | Always switch |
| In ATTACK or IDLE state + `MD_CHANGETARGETMELEE` + `hitType === 'melee'` | Switch |
| In CHASE state + `MD_CHANGETARGETCHASE` | Switch |
| None of the above | Do NOT switch |

**Key insight**: Most monsters (AI types 01, 02, 06, 10, 17) have NO target-switching flags. Once they lock onto a target, they stay locked until that target dies, disconnects, or leaves the zone. Only then do they pick the next target from `inCombatWith`.

### Assist Mechanic: `triggerAssist()`

When a monster is damaged, `triggerAssist()` alerts nearby same-type allies:

1. Iterate all monsters in the same zone.
2. For each candidate: must be same `templateId`, alive, in IDLE state, have `MD_ASSIST` + `MD_CANATTACK` + `MD_CANMOVE`.
3. If distance between the attacked monster and the candidate is within **assist range** (11 RO cells / 550 UE units), the candidate joins the fight.
4. The assisting monster targets the player who attacked the original mob.
5. The assisting monster transitions from IDLE to CHASE.

**Assist does NOT chain**: If Monster A is attacked, triggering Monster B to assist, Monster B's state change to CHASE does not trigger Monster C (even if C has `MD_ASSIST` and is the same type). Only direct attacks on a mob trigger assist checks.

### Aggro Scan: `findAggroTarget()`

For aggressive monsters during IDLE state:

1. Iterate all connected players in the same zone.
2. Filter: not dead, has valid position data, within `aggroRange`.
3. If `MD_TARGETWEAK`: skip players at or above `monsterLevel - 5`.
4. If `MD_DETECTOR`: can target hidden/cloaked players.
5. Return the **closest** qualifying player.

### Target Selection Priority (by situation)

| Situation | Rule |
|-----------|------|
| Aggressive idle scan | Closest player within `aggroRange` |
| Cast sensor (idle/chase) | The casting player |
| First damage received (no target) | The attacker |
| Damaged by new player (has target) | `shouldSwitchTarget()` rules |
| Target dies/disconnects | Next player from `inCombatWith` (first available) |
| `MD_RANDOMTARGET` | Random player from `inCombatWith` each swing |
| `MD_CHANGECHASE` (during chase) | Any combatant closer than current target AND in attack range |
| `MD_TARGETWEAK` | Only players 5+ levels below monster |
| Assist trigger | The player who attacked the allied mob |

### Rude Attack Behavior

A "rude attack" occurs when a player attacks a monster from a position the monster cannot reach (behind Ice Wall, across impassable terrain, etc.):

- **Normal monsters**: Execute `NPC_RANDOMMOVE` (teleport to a random position on the map) after receiving a rude attack. This prevents exploitation of unreachable positions.
- **Boss/MVP monsters**: Do NOT teleport from rude attacks. They are immune to this mechanic. They will continue attempting to path to the attacker or switch targets.
- **Configuration**: rAthena `monster_ai` flag `0x0004` controls whether mobs can only change target within attack range (preventing ranged rude attack target switching).

---

## 6. Movement

### Walk Speed System

Monster movement speed is defined by the `walkSpeed` field in the monster database. This value represents **milliseconds per RO cell** (1 cell = approximately 50 UE units in Sabri_MMO).

| walkSpeed Value | Speed Description | Cells/Second | UE Units/Second | Example Monsters |
|----------------|-------------------|-------------|----------------|------------------|
| 100 | Very Fast | 10 | 500 | Fast MVPs (Baphomet) |
| 150 | Fast | 6.67 | 333 | Hornet, Condor, Creamy, Hunter Fly |
| 200 | Normal | 5 | 250 | Most monsters (Wolf, Zombie, Scorpion, Isis) |
| 250 | Slightly Slow | 4 | 200 | Drainliar, Side Winder |
| 300 | Slow | 3.33 | 167 | Archer Skeleton, Mummy, Poporing, Marc |
| 400 | Very Slow | 2.5 | 125 | Poring, Willow, Ghoul |
| 1000 | Immobile* | 1 | 50 | Red Plant, Pupa, Mandragora |

*walkSpeed 1000 is used for "plants" that technically have `MD_CANMOVE` but move so slowly they are effectively stationary. True immobile monsters (AI 06) have no `MD_CANMOVE` flag.

**Speed formula for server implementation**:
```
moveSpeed (UE units/sec) = (50 / walkSpeed) * 1000
stepPerTick (UE units) = moveSpeed * (tickMs / 1000)
```

Example: Poring (walkSpeed 400) at 200ms tick:
- moveSpeed = (50/400) * 1000 = 125 UE/sec
- stepPerTick = 125 * 0.2 = 25 UE units per tick

### Player Movement Speed Reference

For comparison, a player with no speed modifiers takes **150ms per cell** (6.67 cells/sec, 333 UE/sec). This means:
- Most monsters (walkSpeed 200) move at 75% of player speed.
- Fast monsters (walkSpeed 150) move at player speed.
- Very fast monsters (walkSpeed 100) move at 150% of player speed.
- Slow monsters (walkSpeed 400) move at 37.5% of player speed.

### Wander Movement

During IDLE state, monsters that can wander move at **60% of their chase speed**:

| Parameter | Value | Notes |
|-----------|-------|-------|
| Wander speed | 60% of chase speed | Relaxed movement |
| Wander pause | 3-8 seconds | Random delay between wander movements |
| Wander distance | 100-300 UE units (2-6 cells) | Random distance from current position |
| Wander radius clamp | Per-spawn `wanderRadius` | Prevents infinite drift from spawn point |

**Wander cycle**:
1. Wait a random pause (3-8 seconds).
2. Pick a random point within 100-300 UE units of current position.
3. Clamp target to `wanderRadius` from original spawn point.
4. Walk toward target at 60% chase speed.
5. Arrive (within 10 UE units) -> stop, start new pause timer.
6. Repeat.

**Wander disabled for**: Monsters without `MD_CANMOVE`, monsters with `MD_NORANDOMWALK` (slaves, guardians), dead monsters.

### Chase Movement

During CHASE state, monsters move at full speed toward their target:

| Parameter | Value | Notes |
|-----------|-------|-------|
| Chase speed | 100% of `moveSpeed` | Full speed pursuit |
| Chase range | Per-monster `chaseRange` (default 12 cells / 600 UE units) | Max distance from aggro origin |
| Give-up distance | `chaseRange + 200` UE units (4 extra cells) | Distance from aggro origin where monster gives up |
| Return behavior | Walk back naturally | Monster does NOT teleport to spawn; it wanders back via normal idle wander |

**Chase range typical values**:
- Normal monsters: 600 UE units (12 cells)
- Boss/MVP monsters: 800-1200 UE units (16-24 cells)
- Immobile monsters: 0 (cannot chase)

### Hit Stun and Walk Delay

| Timer | Source | Effect |
|-------|--------|--------|
| **damageMotion** | Per-monster field (typically 200-480ms) | After taking damage, monster is in "hit stun" -- movement paused, attacks paused, but state checks continue. |
| **walkDelay / canmove_tick** | After each movement step | Monster cannot begin a new move until walk animation completes. Prevents "sliding" where the monster appears to move without animation. |
| **attackMotion / canact_tick** | After each attack | Monster cannot attack or use skills until attack animation completes. |

### Position Broadcasting

Monster positions are broadcast to clients at a rate-limited interval to prevent network spam:

| Parameter | Value | Notes |
|-----------|-------|-------|
| Broadcast interval | 200ms | Position only sent if monster moved since last broadcast |
| Format | `enemy:move` event with `{enemyId, x, y, z, targetX, targetY}` | Client interpolates between current and target positions |

---

## 7. Spawn System

### Spawn Types

| Type | Description | Implementation |
|------|-------------|----------------|
| **Fixed Position** | Monster always spawns at exact (x, y) coordinates. | Used for dungeon bosses, special NPCs, specific story monsters. The spawn point has explicit x/y values. |
| **Random Position** | Monster spawns at a random position within the map or within a defined rectangular area. | x=0, y=0 means random anywhere on the map. x1,y1,x2,y2 defines a rectangular spawn area. |
| **Zone-Based** | Spawn coordinates are relative to a defined sub-area within the map. | Sabri_MMO implementation -- each zone has a `wanderRadius` that implicitly defines the spawn area. |

### Spawn Syntax (rAthena)

```
<map>,<x>,<y>,<xs>,<ys>  monster  <name>  <mobID>,<amount>,<delay1>{,<delay2>,<event>,<size>,<ai>}
```

| Field | Description |
|-------|-------------|
| `map` | Map name (e.g., `prt_fild08`, `pay_dun01`) |
| `x, y` | Center spawn coordinates. Both 0 = random position on map. |
| `xs, ys` | Spawn area spread (cells from center). 0,0 = exact point. |
| `amount` | Number of this monster to spawn simultaneously |
| `delay1` | Base respawn delay in milliseconds after death |
| `delay2` | Random variance added to base delay (ms). Actual respawn = `delay1 + random(0, delay2)` |
| `event` | Optional NPC script event triggered on kill (`NPCName::OnEventName`) |
| `size` | Optional size override (0=normal, 1=small, 2=big) |
| `ai` | Optional AI override |

### Respawn Mechanics

**Timer start**: The respawn timer begins at the moment of death, not when the corpse disappears or is discovered.

**Minimum respawn**: The server enforces a minimum respawn delay of 5 seconds (5000ms) regardless of the configured value.

**Respawn position**: On respawn, each monster's center cell is set to a random cell within its original spawn area (unless fixed position). Each respawn may place the monster in a slightly different location within the defined area.

| Monster Class | Typical `respawnMs` | Notes |
|---------------|-------------------|-------|
| Normal (field) | 0 - 5,000ms | Many field mobs have instant or near-instant respawn (0ms = 5s minimum enforced) |
| Normal (dungeon) | 5,000 - 10,000ms | Dungeon mobs take slightly longer |
| Mini-Boss | 300,000 - 600,000ms | 5-10 minutes |
| MVP Boss | 3,600,000 - 7,200,000ms | 60-120 minutes base + 0-600,000ms variance (up to 10 min random) |

### Spawn Density Reference

Typical spawn counts for popular classic RO maps:

| Map | Monster | Count | Respawn (sec) |
|-----|---------|-------|---------------|
| prt_fild08 | Poring | 70 | 5 |
| prt_fild08 | Lunatic | 50 | 5 |
| prt_fild08 | Fabre | 40 | 5 |
| pay_dun01 | Zombie | 40 | 5 |
| pay_dun01 | Skeleton | 30 | 5 |
| moc_pryd01 | Mummy | 35 | 5 |
| gef_dun01 | Hunter Fly | 25 | 7 |
| gl_knt01 | Raydric | 30 | 7 |
| gl_knt01 | Abysmal Knight | 5 | 30 |
| anthell02 | Ant Egg | 40 | 5 |
| anthell02 | Maya (MVP) | 1 | 7200-7800 |
| prt_sewb4 | Golden Thief Bug (MVP) | 1 | 3600-4200 |
| prt_maze03 | Baphomet (MVP) | 1 | 7200-7800 |

### Slave Spawn Mechanics

Slaves are monsters summoned by other monsters via `NPC_SUMMONSLAVE`:

| Property | Behavior |
|----------|----------|
| **Spawn position** | At or near the master's current position |
| **Movement** | Follow the master. Do not wander randomly (`MD_NORANDOMWALK`). If master is out of range, stand still. |
| **Movement speed** | Matches master's movement speed at time of summoning |
| **Targeting** | Attack whatever the master is targeting. If master changes target, slaves continue attacking the original target until it dies or leaves. If master has no target, slaves stand idle (unless attacked directly). |
| **Master death** | When the master dies, all slaves die immediately. Slaves killed this way yield NO experience and NO drops. |
| **Master teleport** | When the master teleports (via NPC_RANDOMMOVE or similar), all slaves teleport to the master's new position. |
| **AI type** | Usually AI code 24 (`0x00A1`: CanMove + NoRandomWalk + CanAttack). |
| **Summoning conditions** | Controlled by `mob_skill_db` conditions. Typically `slavelt X` (summon if slave count below X) or `slavele X`. |
| **Independent slaves** | Some monsters spawn WITH slaves already (defined in spawn scripts). These "pre-existing" slaves may have independent AI rather than strict master-follow behavior. |

### Lazy Spawning (Server Optimization)

Enemies for a zone are only created when the **first player enters that zone**. This prevents wasting resources on zones nobody is visiting.

```
Player enters zone -> Check if enemies exist for zone -> If not, spawn all from ZONE_REGISTRY
```

### MVP Tombstone System

When an MVP is killed:
1. A tombstone object spawns at the death location.
2. The tombstone displays: MVP name, time of death, MVP-awarded player name.
3. The tombstone does NOT reveal respawn time or respawn location.
4. The tombstone disappears 5 seconds after the MVP respawns.
5. Only applies to naturally spawned MVPs (not summoned or instanced).

---

## 8. Monster Stats

### Monster Database Fields

Each monster in the database has the following combat-relevant stats:

| Field | Type | Description | Used In |
|-------|------|-------------|---------|
| `level` | int | Monster level (1-99) | EXP penalty, TargetWeak check, damage formulas |
| `maxHealth` (HP) | int | Maximum hit points | Combat, respawn |
| `attack` (ATK1) | int | Minimum physical ATK | Damage formula (min roll) |
| `attack2` (ATK2) | int | Maximum physical ATK | Damage formula (max roll) |
| `defense` (DEF) | int | Hard physical DEF | Damage reduction (percentage-based in pre-renewal) |
| `magicDefense` (MDEF) | int | Hard magic DEF | Magic damage reduction |
| `str` | int | Strength stat | Physical damage bonus |
| `agi` | int | Agility stat | FLEE, ASPD |
| `vit` | int | Vitality stat | Soft DEF (VIT DEF) |
| `int` | int | Intelligence stat | Soft MDEF (INT MDEF), magic damage |
| `dex` | int | Dexterity stat | HIT accuracy |
| `luk` | int | Luck stat | Critical rate, Perfect Dodge |
| `attackRange` | int | Attack range in cells (1 = melee, 2+ = ranged) | Determines melee vs ranged, Pneuma interaction |
| `aggroRange` | int | Sight/detection range in cells | Aggressive scan distance |
| `chaseRange` | int | Max pursuit distance in cells | Chase state give-up distance |
| `aspd` | int | Attack speed value (0-200 scale) | (Informational -- `attackDelay` is the actual timing) |
| `walkSpeed` | int | MS per cell movement | Movement speed calculation |
| `attackDelay` | int | MS between auto-attacks | Attack timing |
| `attackMotion` | int | Attack animation duration (ms) | Action delay (canact_tick) |
| `damageMotion` | int | Hit stun duration (ms) | Hit stun timer |
| `size` | string | `small` / `medium` / `large` | Size modifier in damage formula |
| `race` | string | formless / undead / brute / plant / insect / fish / demon / demi-human / angel / dragon | Race modifier in damage formula, card effects |
| `element` | object | `{ type, level }` (element type + level 1-4) | Elemental damage multiplier |
| `monsterClass` | string | `normal` / `boss` / `mvp` | Boss protocol, MVP rewards |

### How Monster ATK Works (Pre-Renewal)

In pre-renewal, monster ATK is a simple min-max range:
- `attack` = minimum damage roll
- `attack2` = maximum damage roll
- Actual base damage = `random(attack, attack2)`

This is different from player ATK, which has StatusATK + WeaponATK components. Monsters have a single ATK value that is used directly.

### Monster HIT Calculation

```
Monster HIT = monster.level + monster.dex
```

Against player FLEE:
```
Player FLEE = player.level + player.agi + (bonus from skills/items)
Hit Rate = 80 + Monster_HIT - Player_FLEE
Clamped to range [5%, 95%]
```

### Monster FLEE Calculation

```
Monster FLEE = monster.level + monster.agi
```

Against player HIT:
```
Player HIT = player.level + player.dex + (bonus from skills/items)
Hit Rate = 80 + Player_HIT - Monster_FLEE
Clamped to range [5%, 95%]
```

### Monster DEF in Damage Calculation (Pre-Renewal)

Pre-renewal uses a two-part DEF system for both players and monsters:

**Hard DEF** (from `defense` field):
```
hardDefReduction = hardDef%  (percentage reduction)
damageAfterHardDef = baseDamage * (100 - hardDef) / 100
```
- Capped at 100 (100% reduction). Very few monsters have 100 DEF (Red Plant).
- Boss monsters typically have 15-60 hard DEF.

**Soft DEF / VIT DEF** (from `vit` stat):
```
softDef = vit (for monsters, soft DEF = VIT stat directly)
damageAfterSoftDef = damageAfterHardDef - softDef
```
- Flat reduction after percentage reduction.
- Minimum final damage is 1.

### Monster MDEF in Damage Calculation (Pre-Renewal)

Similar two-part system:

**Hard MDEF** (from `magicDefense` field):
```
hardMDefReduction = hardMDef%
magicDamageAfterHardMDef = baseMagicDamage * (100 - hardMDef) / 100
```

**Soft MDEF / INT MDEF** (from `int` stat):
```
softMDef = int (for monsters, soft MDEF = INT stat directly)
finalMagicDamage = magicDamageAfterHardMDef - softMDef
```

### Monster Critical Hit

Monsters can land critical hits on players:
```
Monster Crit Rate = monster.luk / 3 + 1  (percentage)
Critical hits ignore FLEE (always hit) and ignore hard DEF
```

Boss monsters specifically can critically hit -- their LUK stat directly influences this.

### Attack Speed (ASPD / Attack Delay)

The `attackDelay` field directly controls the time between auto-attacks in milliseconds:

| Delay Range | Speed Category | Examples |
|-------------|---------------|----------|
| 700-1000ms | Very Fast | Baphomet (768), Doppelganger (868) |
| 1000-1500ms | Fast | Most dungeon mobs |
| 1500-2000ms | Normal | Most field mobs (Poring: 1872) |
| 2000-2500ms | Slow | Heavy/tanky mobs |
| 2500ms+ | Very Slow | Immobile turrets |

The `attackMotion` field defines how long the attack animation takes. During this time, the monster cannot take other actions (walk, use skills). The `damageMotion` field defines the hit stun duration after receiving damage.

---

## 9. Monster Skills

### Skill Database Structure

Monster skills are defined in `mob_skill_db` with condition-action rules. Each entry specifies when and how a monster uses a specific skill.

```
MobID, Info, State, SkillID, SkillLv, Rate, CastTime, Delay, Cancelable, Target, Condition, ConditionValue, val1-5, Emotion, Chat
```

| Field | Description |
|-------|-------------|
| **State** | AI state requirement: `idle`, `walk`, `attack`, `angry`, `follow`, `chase`, `dead`, `loot`, `anytarget` |
| **SkillID** | Skill to use (player skill ID or NPC-exclusive skill ID) |
| **SkillLv** | Skill level. Can exceed player max levels for enhanced monster versions. |
| **Rate** | Probability per tick out of 10000. 500 = 5%, 2000 = 20%, 10000 = 100% guaranteed. |
| **CastTime** | Cast time in ms (monsters can have 0 for instant cast) |
| **Delay** | Cooldown after use (ms). Monster cannot use this skill again until delay expires. |
| **Cancelable** | Whether the cast can be interrupted by damage |
| **Target** | `target` (current target), `self`, `friend` (ally mob), `master`, `randomtarget`, `around1-8` (ground AoE at varying ranges) |

### Trigger Conditions

| Condition | Description | Value |
|-----------|-------------|-------|
| `always` | No condition -- use whenever rate rolls | -- |
| `myhpltmaxrate` | Monster HP below X% of max | Percentage (50 = below 50%) |
| `friendhpltmaxrate` | Nearby ally HP below X% | Percentage |
| `attackpcgt` | Number of attacking players > X | Count |
| `slavelt` | Current slave count < X | Count |
| `slavele` | Current slave count <= X | Count |
| `closedattacked` | Monster was hit by melee attack this tick | -- |
| `longrangeattacked` | Monster was hit by ranged attack this tick | -- |
| `skillused` | Specific skill was used on the monster | Skill ID |
| `casttargeted` | A player is casting a skill targeting the monster | -- |
| `damagedgt` | Single hit damage exceeds threshold | Damage value |
| `rudeattacked` | Player attacked from unreachable position | -- |

### Monster Skill Categories

**Offensive (Player Skills at Enhanced Levels)**:
- Bolt spells (Fire/Cold/Lightning Bolt, Jupitel Thunder) at levels 10-20+
- AoE spells (Lord of Vermillion, Meteor Storm, Storm Gust)
- Physical skills (Bash, Brandish Spear, Mammonite)
- Heal used offensively on undead players

**Monster-Exclusive NPC Skills**:
- `NPC_COMBOATTACK`: Multi-hit physical at 100% ATK per hit
- `NPC_xxxBREATH`: Elemental breath attacks (Dark, Fire, Ice, Thunder, Acid)
- `NPC_xxxATTACK`: Element-typed melee modifiers with status chance
- `NPC_CRITICALSLASH`: Guaranteed critical hit
- `NPC_STUNATTACK/SLEEPATTACK/SILENCEATTACK/BLINDATTACK/CURSEATTACK/BLEEDING`: Status-inflicting attacks
- `NPC_POWERUP/AGIUP/DEFENSE/BARRIER/STONESKIN`: Self-buffs
- `NPC_SELFDESTRUCTION`: AoE suicide bomb

**Summoning/Transformation**:
- `NPC_SUMMONSLAVE`: Summon specific slave monsters bound to master
- `NPC_CALLSLAVE`: Teleport all slaves back to master position
- `NPC_SUMMONMONSTER`: Summon random monsters (not bound)
- `NPC_METAMORPHOSIS`: Transform into a different monster (full stat/drop replacement)
- `NPC_TRANSFORM`: Cosmetic appearance change only
- `NPC_REBIRTH`: Resurrect as a different monster on death

**Utility/Movement**:
- `NPC_RANDOMMOVE`: Teleport to random map position (used on rude attack)
- `NPC_EMOTION`: Display emotion icon
- `NPC_INVINCIBLE`: Short full invincibility

### Enhanced Monster Skill Levels

When `SkillLv > MAX_PLAYER_LEVEL`, monsters use enhanced versions that exceed player caps:
- Heal Lv 11+: Heals significantly more than player max
- Cold Bolt Lv 20+: 20-hit bolt with massive damage
- Fireball Lv 43: 7x7 AoE with ~1000% damage
- Provoke Lv 10: Reduces target DEF by 100% (no ATK bonus)

---

## 10. Implementation Checklist

### Core Systems

- [x] Monster template database (509 monsters from rAthena pre-re)
- [x] AI type code to hex mode mapping (AI_TYPE_MODES lookup table)
- [x] Mode flag bitmask parser (parseModeFlags function)
- [x] Boss protocol auto-application (knockback immune, status immune, detector)
- [x] 4-state machine (IDLE, CHASE, ATTACK, DEAD)
- [x] AI tick loop (200ms in Sabri_MMO, 100ms in rAthena)
- [x] Spawn system with per-zone spawn configs
- [x] Lazy spawning (first player triggers zone spawn)
- [x] Respawn timer (setTimeout with respawnMs)

### Aggro System

- [x] setEnemyAggro() -- central aggro function from all damage paths
- [x] shouldSwitchTarget() -- mode-flag-based target switching
- [x] triggerAssist() -- same-type mob assist within 550 UE units
- [x] findAggroTarget() -- aggressive scan for closest player
- [x] inCombatWith set -- tracks all attacking players
- [x] Aggro origin tracking -- position where mob first aggro'd
- [x] Target validation -- dead/disconnected/left-zone checks

### Movement

- [x] Walk speed calculation from walkSpeed field
- [x] Chase movement at full speed
- [x] Wander movement at 60% speed
- [x] Wander radius clamping to spawn area
- [x] Chase range give-up mechanic
- [x] Hit stun pause (damageMotion)
- [x] Position broadcasting at rate-limited intervals

### Mode Flags

- [x] MD_CANMOVE -- movement enabled
- [x] MD_AGGRESSIVE -- aggro scan
- [x] MD_ASSIST -- same-type assist
- [x] MD_CANATTACK -- attack enabled
- [x] MD_CHANGETARGETMELEE -- melee target switch
- [x] MD_CHANGETARGETCHASE -- chase target switch
- [x] MD_CHANGECHASE -- in-chase target-of-opportunity
- [x] MD_RANDOMTARGET -- random target per swing
- [x] MD_TARGETWEAK -- level-based aggro filter
- [x] MD_CASTSENSORIDLE -- cast sensor (idle)
- [x] MD_NORANDOMWALK -- disable wandering
- [x] MD_KNOCKBACKIMMUNE -- auto-set for boss/mvp
- [x] MD_STATUSIMMUNE -- auto-set for boss/mvp
- [x] MD_DETECTOR -- auto-set for boss/mvp
- [x] MD_MVP -- MVP rewards protocol
- [ ] MD_LOOTER -- item pickup from ground (not yet implemented)
- [ ] MD_CASTSENSORCHASE -- cast sensor during chase (not fully wired)
- [ ] MD_ANGRY -- follow/angry pre-attack states (not implemented)
- [ ] MD_NOCAST -- skill suppression flag (not needed until monster skills fully wired)
- [ ] MD_IGNOREMELEE/MAGIC/RANGED/MISC -- damage type immunities (not implemented)
- [ ] MD_TELEPORTBLOCK -- teleport immunity (not implemented)
- [ ] MD_FIXEDITEMDROP -- fixed drop rates (not implemented)
- [ ] MD_SKILLIMMUNE -- full skill immunity (not implemented)

### Monster Skills

- [x] Monster skill database (ro_monster_skills.js -- 12 monsters, 40+ NPC_ skills)
- [x] Skill execution functions (7 types: melee, targeted, self-buff, AoE, heal, summon, transform)
- [x] AI hooks for skill selection (IDLE and ATTACK states)
- [x] Condition evaluation (myhpltmaxrate, slavelt, attackpcgt, always)
- [x] NPC_SUMMONSLAVE -- slave spawning with master/slave lifecycle
- [x] NPC_METAMORPHOSIS -- monster transformation
- [ ] Full mob_skill_db coverage (only 12 of 509 monsters have skill entries)
- [ ] Chase state skill usage
- [ ] Angry/Follow state skill sets
- [ ] NPC_CALLSLAVE -- teleport slaves to master
- [ ] NPC_RANDOMMOVE -- rude attack teleport
- [ ] NPC_REBIRTH -- resurrect as different monster
- [ ] NPC_SELFDESTRUCTION -- suicide AoE

### Spawn System

- [x] Fixed position spawns
- [x] Zone-based spawns with wanderRadius
- [x] Respawn timer per-individual-mob
- [x] Slave spawn via NPC_SUMMONSLAVE
- [x] Slave death on master death (no EXP/drops)
- [x] MVP tombstone system
- [ ] Random position spawns (anywhere on map)
- [ ] MVP respawn variance window (base + random 0-10 min)
- [ ] Spawn event scripts (on-kill NPC events)
- [ ] Pre-existing slaves (spawned with master at zone init)

### Combat

- [x] Monster auto-attack damage calculation (roPhysicalDamage)
- [x] Hit/Miss calculation (HIT vs FLEE)
- [x] Critical hit system (LUK-based)
- [x] Hard DEF + Soft DEF (VIT) reduction
- [x] Hard MDEF + Soft MDEF (INT) reduction
- [x] Elemental damage multipliers
- [x] Size penalty modifiers
- [x] Race card modifiers
- [x] EXP distribution on kill
- [x] Drop system with roll rates
- [x] MVP EXP and drop distribution

---

## 11. Gap Analysis

### Critical Gaps (Affect gameplay correctness)

| Gap | Impact | Priority |
|-----|--------|----------|
| **MD_ANGRY (Follow/Angry states)** | AI type 04 monsters (18% of all mobs -- Zombie, Orc Warrior, Mummy, Ghoul, Hunter Fly, Familiar) behave identically to simple aggressive mobs instead of having pre-attack hyper-active behavior. Missing different skill sets for before/after being attacked. | HIGH |
| **MD_CASTSENSORCHASE** | Boss/MVP AI (AI 21) and aggressive caster-hunters (AI 20) do not detect casters during chase. Slightly reduces boss difficulty. | MEDIUM |
| **MD_LOOTER (item pickup)** | Poring, Poporing, Yoyo, Thief Bug etc. do not pick up ground items. Missing a classic RO behavior. Requires ground item system. | MEDIUM |
| **Rude attack teleport (NPC_RANDOMMOVE)** | Normal monsters can be attacked from unreachable positions without consequence. Players can exploit terrain/Ice Wall to safely kill mobs. | MEDIUM |
| **Damage type immunities (IGNOREMELEE/MAGIC/RANGED/MISC)** | Some monsters should be immune to specific damage types (e.g., Ghost-element monsters immune to normal physical). Currently handled by elemental table but not by mode flags. | LOW |

### Moderate Gaps (Affect completeness)

| Gap | Impact | Priority |
|-----|--------|----------|
| **Monster skill coverage** | Only 12/509 monsters have skill entries. Most dungeon and boss monsters should have multiple skills. | HIGH (for boss content) |
| **NPC_CALLSLAVE** | Master cannot recall scattered slaves. Slaves may get stuck behind terrain. | LOW |
| **NPC_SELFDESTRUCTION** | Monsters like Marine Sphere cannot self-destruct. | LOW |
| **NPC_REBIRTH** | Monsters cannot resurrect as different types on death. | LOW |
| **MVP respawn variance** | MVPs respawn on fixed timer without the 0-10 minute random variance window. Makes MVP hunting predictable. | MEDIUM |
| **Random map-wide spawns** | All spawns are fixed-position zone-based. No support for random spawn anywhere on map. | LOW |
| **Lazy AI range** | All monsters in a zone run full AI regardless of distance from players. Performance concern for large zones. | LOW (optimization) |

### Implemented Beyond Reference

These features in the current implementation go beyond or differ from the rAthena reference:

| Feature | Status | Notes |
|---------|--------|-------|
| AI tick at 200ms vs 100ms | Intentional | Lower CPU cost, acceptable accuracy |
| Aggro scan at 500ms interval | Intentional | Reduces scan overhead vs every-tick |
| UE unit coordinate system | Adaptation | 1 RO cell = 50 UE units. All ranges converted. |
| Zone-based spawn system | Enhancement | Cleaner than rAthena's per-map scripts. wanderRadius provides implicit spawn area. |
| Monster skill system (ro_monster_skills.js) | Partial | 12 monsters with 40+ NPC_ skills. 7 execution functions. AI hooks for IDLE and ATTACK. |

---

## Sources

- [rAthena Monster AI Types (Aegis State Machine) -- Issue #926](https://github.com/rathena/rathena/issues/926)
- [rAthena mob_db_mode_list.txt](https://github.com/rathena/rathena/blob/master/doc/mob_db_mode_list.txt)
- [rAthena mob_db.txt](https://github.com/rathena/rathena/blob/master/doc/mob_db.txt)
- [rAthena mob.cpp (AI implementation)](https://github.com/rathena/rathena/blob/master/src/map/mob.cpp)
- [rAthena monster.conf (battle configuration)](https://github.com/rathena/rathena/blob/master/conf/battle/monster.conf)
- [rAthena Permanent Monster Spawn wiki](https://github.com/rathena/rathena/wiki/Permanent_Monster_Spawn)
- [rAthena Pre-Renewal Monster Class/AI Database Fix](https://github.com/rathena/rathena/commit/af2615091cc5deb3ebd9817f95d75b2f0cae600d)
- [DeepWiki -- rAthena Monster and NPC System](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
- [DeepWiki -- rAthena Monster Database](https://deepwiki.com/rathena/rathena/6.1-monster-database)
- [iRO Wiki -- Monster](https://irowiki.org/wiki/Monster)
- [iRO Wiki Classic -- Monster](https://irowiki.org/classic/Monster)
- [iRO Wiki -- Movement Speed](https://irowiki.org/wiki/Movement_Speed)
- [iRO Wiki -- NPC_SUMMONSLAVE](https://irowiki.org/wiki/NPC_SUMMONSLAVE)
- [iRO Wiki -- MVP](https://irowiki.org/wiki/MVP)
- [RateMyServer -- Pre-Re Monster Database](https://ratemyserver.net/index.php?page=mob_db)
- [RateMyServer -- Cast Sensor Monsters](https://ratemyserver.net/index.php?page=mob_db&sense=1&mob_search=Search&sort_r=0)
- [RateMyServer -- Assist Monsters](https://ratemyserver.net/index.php?page=mob_db&assi=1&mob_search=Search&sort_r=0)
- [WarpPortal Forums -- Aggro System Discussion](https://forums.warpportal.com/index.php?%2Ftopic%2F89360-how-does-the-aggro-system-work-in-ro%2F=)
- [rAthena Forums -- Monster Aggro System](https://rathena.org/board/topic/81546-monster-aggro-system/)
- [rAthena Forums -- Boss Monster Behavior (Issue #1697)](https://github.com/rathena/rathena/issues/1697)
- [rAthena Forums -- Slave Monster Behavior (Issue #1676)](https://github.com/rathena/rathena/issues/1676)
- [Boss Protocol Reference](https://ragnarokonlinefanfictionv0.fandom.com/wiki/Ragnarok_Ground_Zero/Monsters/Boss_Protocol)
- [Ragnarok MVP Timer](https://ragnarok-mvp-timer.com/)
