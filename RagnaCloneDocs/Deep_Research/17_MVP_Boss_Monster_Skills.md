# MVP, Boss & Monster Skill System -- Deep Research (Pre-Renewal)

> **Scope**: Ragnarok Online Classic (pre-renewal, Episodes 1-13.2) MVP system, boss monster mechanics, and the complete monster skill (NPC_) framework. Verified against rAthena pre-re source, Hercules, iRO Wiki Classic, RateMyServer pre-re database, and official kRO/iRO behavior.
>
> **Purpose**: Implementation reference for Sabri_MMO server and client.

---

## Table of Contents

1. [MVP System](#1-mvp-system)
2. [Boss Immunities](#2-boss-immunities)
3. [Monster Skill System](#3-monster-skill-system)
4. [Mini-Boss System](#4-mini-boss-system)
5. [Complete MVP List](#5-complete-mvp-list)
6. [Implementation Checklist](#6-implementation-checklist)
7. [Gap Analysis](#7-gap-analysis)

---

## 1. MVP System

### 1.1 MVP Flag vs Boss Flag (Differences and Immunities)

Ragnarok Online has two distinct boss-tier classifications that are often conflated. Understanding the difference is critical for correct implementation.

#### Monster Class Hierarchy

| Class | rAthena Field | Boss Protocol | MVP Protocol | Loot Rule | Examples |
|-------|---------------|---------------|--------------|-----------|---------|
| **Normal** | `Class: Normal` | No | No | First-come-first-served | Poring, Zombie, Orc Warrior |
| **Boss (Mini-Boss)** | `Class: Boss` | YES | No | First-hit priority (NOT FFA) | Angeling, Deviling, Ghostring, Tao Gunka, Mastering |
| **MVP** | `Class: Boss` + `MvpExp > 0` | YES | YES | Free-for-all (FFA) | Baphomet, Osiris, Dark Lord, Pharaoh |

#### Boss Protocol (Shared by Both Boss and MVP)

All monsters with `Class: Boss` or `MvpExp > 0` automatically receive these flags:

| Flag | Hex | Effect |
|------|-----|--------|
| `MD_KNOCKBACKIMMUNE` | `0x200000` | Cannot be displaced by any knockback skill (Arrow Repel, Storm Gust, Bowling Bash, etc.) |
| `MD_STATUSIMMUNE` | `0x4000000` | Immune to most status effects (Stun, Freeze, Stone Curse, Poison, Silence, Sleep, Blind, Curse, Bleeding, Hallucination, etc.) |
| `MD_DETECTOR` | `0x2000000` | Can detect and attack players in Hide, Cloak, Chase Walk, and Tunnel Drive |

#### MVP Protocol (MVP Only -- On Top of Boss Protocol)

| Feature | Description |
|---------|-------------|
| `MD_MVP` flag (`0x80000`) | Marks the monster as an MVP for reward processing |
| MVP EXP bonus | `MvpExp` field in mob_db -- bonus base EXP awarded to MVP winner |
| MVP drops (3 slots) | Up to 3 additional item drops rolled exclusively for the MVP winner |
| MVP announcement | Server-wide broadcast: `"[MVP] {PlayerName} has defeated {MonsterName}!"` |
| MVP tombstone | Death marker NPC spawned at kill location |
| FFA loot | Any player can attack regardless of who engaged first |
| Coma immunity | Immune to Coma (Thanatos Card effect) |

#### Key Distinction: Mini-Boss vs MVP

- **Mini-Boss**: Has Boss Protocol but does NOT have MVP Protocol. Does NOT give MVP rewards, does NOT create a tombstone, does NOT have FFA loot rules. First player to engage has loot priority.
- **MVP**: Has BOTH Boss Protocol AND MVP Protocol. Gives MVP rewards, creates tombstone, has FFA loot, server-wide announcement.

**How rAthena Identifies MVPs**: In `mob_db.yml`, any monster with `MvpExp` greater than 0 is treated as an MVP. The `Class: Boss` field alone makes it a mini-boss. There is no separate `Class: Mvp` -- MVPs are identified by the presence of MVP EXP and MVP drops.

---

### 1.2 MVP Reward System (MVP Drops, EXP Bonus, Announcement)

#### MVP Winner Determination

The player who contributes the most to killing the MVP is awarded the "MVP" title. rAthena's `mob.cpp` tracks damage via a damage log (`DAMAGELOG_SIZE = 20`):

```
MVP Score = totalDamageDealt + totalDamageTanked
```

- **totalDamageDealt**: Sum of all damage the player dealt to the MVP (auto-attacks, skills, traps, ground effects, pet damage)
- **totalDamageTanked**: Sum of all damage the MVP's auto-attacks dealt TO the player (records the monster's normal attack damage against each player)
- The player with the highest combined score wins MVP
- Pets, homunculus, and mercenary damage is credited to the owner
- If the top-damage player is dead or disconnected at the moment of death, the reward goes to the next highest scorer
- Party member damage is NOT combined -- each player is scored individually

#### MVP EXP Reward

| Component | Description |
|-----------|-------------|
| Base EXP from kill | Distributed to all attackers proportionally based on damage dealt |
| Job EXP from kill | Distributed identically to base EXP |
| **MVP Bonus EXP** | `MvpExp` value awarded exclusively to the MVP winner as bonus base EXP |

The MVP bonus EXP is in addition to the winner's proportional share of the kill EXP. It is pure base EXP (no job EXP component).

#### MVP Drop System

Each MVP has up to 3 MVP drop slots defined in `MvpDrops`:

```yaml
MvpDrops:
  - Item: Old_Blue_Box
    Rate: 5000          # 50.00%
  - Item: Old_Violet_Box
    Rate: 3000          # 30.00%
  - Item: Yggdrasilberry
    Rate: 1000          # 10.00%
```

Rules:
- Each slot rolls independently (player can receive 0-3 MVP drops)
- Rates are out of 10000 (10000 = 100%)
- MVP drops go directly into the MVP winner's inventory (not dropped on ground)
- MVP drops CANNOT be stolen (Steal skill cannot take them)
- If the MVP winner's inventory is full, the items drop to the ground
- MVP drop rates are NOT affected by Bubble Gum or server rate multipliers by default (configurable in `drops.conf`)
- Regular drops from the MVP still drop on the ground as normal and can be picked up by anyone

#### MVP Announcement

When an MVP dies, the server broadcasts to all players in the zone (some servers broadcast globally):

```
[MVP] {CharacterName} has defeated {MonsterName}!
```

This is a yellow system message displayed in the chat window. The announcement includes only the MVP winner's name, not the party or guild.

---

### 1.3 MVP Tombstone (Death Marker, Respawn Timer Display)

#### Tombstone Mechanics

When a naturally-spawned MVP is killed, a **Tomb** NPC object spawns at the exact death coordinates:

| Property | Value |
|----------|-------|
| Appearance | A gravestone/tombstone sprite (NPC sprite) |
| Spawn location | Exact (x, y) coordinates where the MVP died |
| Interactable | Yes -- clicking the tombstone displays information |
| Duration | Exists from MVP death until 5 seconds after the MVP respawns |
| Despawn | Automatically removed ~5 seconds after the MVP has respawned |

#### Tombstone Information Display

When a player clicks on the tombstone, it shows:

```
Here lies {Monster Name}
Killed at: {Time of Death}
MVP: {Character Name}
```

- **Monster Name**: The MVP's display name (e.g., "Baphomet")
- **Time of Death**: Server time when the MVP was killed (format: HH:MM)
- **MVP Winner**: The character name of the player who received the MVP award

#### What the Tombstone Does NOT Reveal

- When the MVP will respawn (players must track this themselves)
- Where the MVP will respawn (if it has random spawn positions)
- How much time remains until respawn
- Who else participated in the fight

#### Tombstone Exceptions

| Condition | Tombstone Created? |
|-----------|--------------------|
| Natural MVP spawn killed | YES |
| MVP summoned by Dead Branch | NO |
| MVP summoned by Bloody Branch | NO |
| MVP in instances (Endless Tower, etc.) | NO |
| Bio Lab MVPs (Seyren, Eremes, etc.) | Server-dependent (some servers: NO) |
| Unholy Path bosses | NO |

---

### 1.4 MVP Respawn Timers (Fixed + Variable Window)

#### Respawn Formula

```
respawnTime = baseTimer + random(0, varianceWindow)
```

- **baseTimer**: Fixed minimum time after death before the MVP can respawn
- **varianceWindow**: Random additional time added to baseTimer (typically 10 minutes / 600,000ms)
- Timer starts at the moment of death, NOT when the tombstone is clicked or when a player enters the map

#### Respawn Timer Table (All Pre-Renewal MVPs)

| Respawn Category | Base Timer | Variance | Effective Range | Examples |
|-----------------|-----------|----------|-----------------|---------|
| 1-hour MVPs | 60 min | +0-10 min | 60-70 min | Amon Ra, Dark Lord, Dracula, Golden Thief Bug, Moonlight Flower, Osiris, Pharaoh, Turtle General |
| 2-hour MVPs | 120 min | +0-10 min | 120-130 min | Baphomet, Doppelganger, Drake, Eddga, Hatii, Maya, Mistress, Orc Lord, Stormy Knight |
| 3-hour MVPs | 180 min | +0-10 min | 180-190 min | Detardeurus |
| 5-hour MVPs | 300 min | +0-10 min | 300-310 min | Atroce, Tao Gunka |
| 8-hour MVPs | 480 min | +0-10 min | 480-490 min | Valkyrie Randgris |
| 12-hour MVPs | 720 min | +0-10 min | 720-730 min | Beelzebub |
| 24-hour MVPs | 1440 min | +0-10 min | 1440-1450 min | Orc Hero |
| Special spawn | N/A | N/A | Quest-triggered | Ktullanux, Memory of Thanatos |

#### Multiple Spawn Locations

Some MVPs can spawn on multiple maps, each with independent timers:

| MVP | Map 1 | Respawn 1 | Map 2 | Respawn 2 | Additional |
|-----|-------|-----------|-------|-----------|------------|
| Eddga | pay_fild11 | 2 hours | Balder Guild Dungeon | 8 hours | -- |
| Atroce | ra_fild02 | 5 hours | ra_fild03 | 3 hours | ra_fild04 (5h), ve_fild01 (5h), ve_fild02 (6h) |
| Mistress | mjolnir_04 | 2 hours | -- | -- | Single spawn |

Each map's timer is independent -- killing Eddga on pay_fild11 does not affect Eddga's timer on the guild dungeon map.

---

### 1.5 Most Damage vs Last Hit (MVP Reward Attribution)

#### Pre-Renewal Attribution Rule

**Most total contribution wins** -- NOT last hit.

The MVP award is given to the player with the highest `damage_dealt + damage_tanked` score. This is fundamentally different from many other MMOs that use "last hit" or "tagging" systems.

Key implications:
- A player who deals 90% of the damage but dies before the MVP is killed can still win MVP (if they had the highest score and there is no higher scorer)
- Tank characters that absorb significant damage from the MVP gain contribution credit even if their DPS is lower
- Healing does NOT contribute to the score (healing is not tracked in the damage log)
- Buff-only support characters receive no MVP credit
- If the MVP winner disconnects before the death event, the next-highest scorer receives the award

#### EXP Distribution (Non-MVP Portion)

For the regular base/job EXP (not the MVP bonus):
- EXP is distributed proportionally to all attackers based on damage dealt to the MVP's total HP
- Each additional attacker increases total EXP by 25% (tap bonus, up to a cap)
- Example: MVP with 100,000 HP and 1000 base EXP. Player A deals 65,000 damage, Player B deals 35,000 damage. Total EXP = 1000 * 1.25 = 1250. Player A gets 812 EXP, Player B gets 437 EXP.
- Party Even Share mode overrides this with equal distribution among party members in range

---

## 2. Boss Immunities

### 2.1 Status Effect Immunity List

All monsters with Boss Protocol (both mini-bosses and MVPs) are immune to the following status effects:

#### Fully Immune (No Effect Whatsoever)

| Status Effect | Skill Source | Notes |
|---------------|-------------|-------|
| Stun | Bash, Shield Charge, Hammer Fall, NPC attacks | Complete immunity |
| Freeze | Frost Diver, Storm Gust, Frost Nova | Complete immunity |
| Stone Curse | Stone Curse | Complete immunity (including the "Stone" state) |
| Sleep | Lullaby, NPC attacks | Complete immunity |
| Poison | Envenom, Enchant Poison, Venom Dust | Complete immunity |
| Deadly Poison | Enchant Deadly Poison | Complete immunity |
| Curse | NPC attacks, Cursed Water | Complete immunity |
| Blind | Blind attack, NPC attacks | Complete immunity |
| Silence | Lex Divina (1 exception -- see below) | Complete immunity |
| Bleeding | NPC attacks | Complete immunity |
| Confusion/Hallucination | NPC_HALLUCINATION | Complete immunity |
| Deep Sleep | (Renewal only) | Complete immunity |
| Fear | NPC_DRAGONFEAR | Complete immunity |
| Burning | (Renewal only) | Complete immunity |
| Freezing | (Renewal only) | Complete immunity |
| Crystallization | (Renewal only) | Complete immunity |
| Coma | Thanatos Card | Complete immunity |

#### Partially Effective (Reduced Duration/Effect)

| Skill/Effect | Interaction with Boss | Details |
|-------------|----------------------|---------|
| **Ankle Snare** | 1/5 duration | Boss monsters are held for only 20% of normal duration. Duration formula: `(3000 + 30 * srcBaseLv) / 5` ms. Post-2018 patch: MVPs fully immune. |
| **Quagmire** | Applies normally | Reduces AGI and DEX. One of the few debuffs that affects bosses. Also reduces movement speed. Works on bosses because it is classified as a ground effect, not a status effect on the target. |
| **Decrease AGI** | Does NOT apply | Status immunity blocks it |
| **Provoke** | Does NOT apply | Status immunity blocks it |

#### Skills That Still Affect Bosses

| Skill | Effect on Boss | Why It Works |
|-------|---------------|--------------|
| **Divest / Strip skills** (Rogue) | Removes equipment, reduces stats | Classified as equipment manipulation, not status effect |
| **Quagmire** (Sage) | AGI/DEX reduction, speed reduction | Ground effect, not direct status |
| **Dispell** (Sage) | Removes buffs from boss | Buff removal, not status application |
| **Safety Wall** | Blocks boss melee hits | Protective ground effect |
| **Pneuma** | Blocks boss ranged hits | Protective ground effect |
| **Land Protector** | Cancels boss ground skills | Ground effect interaction |
| **Magnetic Earth** | Same as Land Protector | Ground effect |
| **Lex Aeterna** | Doubles next damage | Applies as a debuff; some implementations allow on boss |
| **Frost Joker / Scream** | Does NOT freeze bosses | Status immunity prevents the freeze |

---

### 2.2 Knockback Immunity

All Boss Protocol monsters are immune to position displacement:

| Skill | Normal Monsters | Boss Monsters |
|-------|----------------|---------------|
| Arrow Repel (Archer) | 6 cells knockback | No effect |
| Storm Gust (Wizard) | 2 cells knockback | No effect |
| Bowling Bash (Knight) | Knockback + chain | No knockback, still takes damage |
| Jupitel Thunder (Wizard) | 2+ cells knockback | No effect on position |
| Spear Boomerang (Knight) | 3 cells knockback | No effect |
| Brandish Spear (Knight) | 3 cells knockback | No effect |
| Magnum Break (Swordsman) | 2 cells knockback | No effect |
| Charge Attack (Knight) | 1 cell knockback | No effect |
| Fire Wall (Wizard) | Pushback through fire cells | No effect on position |
| Grand Cross (Crusader) | No knockback normally | N/A |

**Implementation note**: The knockback check should be:
```javascript
if (enemy.modeFlags.knockbackImmune) return; // Skip all knockback logic
```

---

### 2.3 Skill-Specific Interactions (Detailed)

#### Ankle Snare (Hunter Trap)

| Target Type | Duration | Details |
|------------|----------|---------|
| Normal monster | `(3000 + 30 * srcBaseLv)` ms, reduced by target AGI | Full duration, movement-only lock |
| Boss/MVP (pre-2018) | 1/5 of normal duration | Example: Lv99 Hunter = (3000 + 2970) / 5 = 1194ms |
| MVP (post-2018) | 0 / Fully immune | Changed in a later kRO patch; pre-renewal servers vary |

#### Magnetic Earth / Land Protector (Sage)

- Removes AND prevents ground effects (Safety Wall, Pneuma, Fire Wall, Storm Gust ground, etc.)
- Boss monsters' own ground-target skills are also blocked
- Land Protector vs Land Protector: mutual destruction (both removed)

#### Investigate (Monk)

- Works normally on bosses -- ignores DEF based on target's DEF value
- Higher boss DEF = more damage from Investigate

#### Asura Strike (Monk)

- Works normally on bosses -- full damage, no immunity
- Often the primary MVP-killing skill due to its massive single-hit damage

---

## 3. Monster Skill System

### 3.1 NPC_ Skills Complete List

Monster-exclusive skills use the `NPC_` prefix. These cannot be learned or copied by players (except via Plagiarism for player-class skills that monsters also use). All skill IDs below are from the rAthena pre-renewal skill database.

#### Elemental Attack Skills

| Skill ID | Name | Type | Element | Effect |
|----------|------|------|---------|--------|
| 183 | NPC_FIREATTACK | Offensive | Fire | Single-target Fire property ranged physical damage |
| 184 | NPC_WATERATTACK | Offensive | Water | Single-target Water property ranged physical damage |
| 185 | NPC_GROUNDATTACK | Offensive | Earth | Single-target Earth property ranged physical damage |
| 186 | NPC_WINDATTACK | Offensive | Wind | Single-target Wind property ranged physical damage |
| 187 | NPC_POISONATTACK | Offensive | Poison | Single-target Poison property attack + Poison chance |
| 188 | NPC_HOLYATTACK | Offensive | Holy | Single-target Holy property ranged physical damage |
| 189 | NPC_DARKNESSATTACK | Offensive | Shadow | Single-target Shadow property ranged physical damage |
| 190 | NPC_UNDEADATTACK | Offensive | Undead | Single-target Undead property ranged physical damage |
| 191 | NPC_GHOSTATTACK | Offensive | Ghost | Single-target Ghost property ranged physical damage |
| 192 | NPC_MAGICALATTACK | Offensive | Varies | Single-target magical damage using monster's element |

#### Status-Inflicting Attack Skills

| Skill ID | Name | Status Inflicted | Base Chance | Notes |
|----------|------|-----------------|-------------|-------|
| 176 | NPC_STUNATTACK | Stun | 10-50% by level | Physical ATK + Stun |
| 177 | NPC_PETRIFYATTACK | Stone Curse | 10-50% | Physical ATK + Petrification |
| 178 | NPC_CURSEATTACK | Curse | 10-50% | Physical ATK + Curse |
| 179 | NPC_SLEEPATTACK | Sleep | 10-50% | Physical ATK + Sleep |
| 180 | NPC_POISON | Poison | 10-50% | Poison property attack + Poison status |
| 181 | NPC_BLINDATTACK | Blind | 10-50% | Physical ATK + Blind |
| 182 | NPC_SILENCEATTACK | Silence | 10-50% | Physical ATK + Silence |
| 660 | NPC_BLEEDING | Bleeding | Varies | Physical ATK + Bleeding |
| 661 | NPC_DEADLYPOISON | Deadly Poison | Varies | Extremely high poison damage |

#### Multi-Hit and AoE Attack Skills

| Skill ID | Name | Effect | Damage |
|----------|------|--------|--------|
| 171 | NPC_COMBOATTACK | Multi-hit physical (2-8 hits) | 100% ATK per hit |
| 172 | NPC_CRITICALSLASH | Guaranteed critical hit | 200% ATK, ignores Flee |
| 339 | NPC_GRANDDARKNESS | AoE Shadow-element Grand Cross | Large AoE around caster |
| 656 | NPC_EARTHQUAKE | 3-tick AoE Neutral magic | Physical ATK-based, ignores FLEE, 3 separate damage instances |
| 659 | NPC_DRAGONFEAR | AoE fear + random statuses | Causes Stun/Blind/Silence/Bleeding randomly in AoE |
| 662 | NPC_PULSESTRIKE | AoE knockback damage | Physical damage + knockback in area |
| 663 | NPC_HELLJUDGEMENT | Massive AoE damage | Shadow property, used by Dark Lord |

#### Breath Attack Skills

| Skill ID | Name | Element | Damage Type |
|----------|------|---------|-------------|
| 654 | NPC_FIREBREATH | Fire | Breath attack, ATK-based |
| 655 | NPC_ICEBREATH | Water | Breath attack, ATK-based |
| 657 | NPC_THUNDERBREATH | Wind | Breath attack, ATK-based |
| 658 | NPC_ACIDBREATH | Earth | Breath attack, ATK-based |
| 174 | NPC_DARKBREATH | Shadow | Shadow-property breath |

#### Self-Buff Skills

| Skill ID | Name | Effect | Duration |
|----------|------|--------|----------|
| 195 | NPC_POWERUP | ATK x3, HIT boost (based on DEX) | Temporary |
| 196 | NPC_AGIUP | Movement speed and ASPD increase | Temporary |
| 198 | NPC_DEFENSE | DEF increase | Temporary |
| 199 | NPC_LICK | Remove target's equipment temporarily + stun | Instant |
| 201 | NPC_KEEPING | Set DEF to 90, cannot move or attack | Duration-based |
| 202 | NPC_DARKBLESSING | Inflict Curse on target (monster version) | -- |
| 203 | NPC_BARRIER | Invincible barrier (no damage taken) | Short duration |
| 204 | NPC_DEFENDER | Reduce incoming ranged damage | Duration-based |
| 205 | NPC_INVINCIBLE | Full invincibility (immune to everything) | Very short |
| 206 | NPC_INVINCIBLEOFF | Cancel invincibility state | Instant |
| 670 | NPC_STONESKIN | +DEF%, -MDEF% | Duration (increases physical defense at cost of magic defense) |
| 671 | NPC_ANTIMAGIC | +MDEF%, -DEF% | Duration (increases magic defense at cost of physical defense) |

#### Summoning / Transformation Skills

| Skill ID | Name | Effect | Parameters |
|----------|------|--------|------------|
| 196 | NPC_SUMMONSLAVE | Summon slave monsters bound to caster | val1-val5 = slave monster IDs |
| 352 | NPC_CALLSLAVE | Teleport all slaves to master's location | Instant recall |
| 303 | NPC_SUMMONMONSTER | Summon random monsters near caster | Not bound as slaves |
| 304 | NPC_METAMORPHOSIS | Transform into a different monster | val1-val5 = possible new monster IDs |
| 342 | NPC_TRANSFORM | Change appearance only (cosmetic) | No stat change |
| 207 | NPC_REBIRTH | Respawn at same location on death | Level 1-3: different HP% on rebirth (50%) |

#### Utility / Movement Skills

| Skill ID | Name | Effect |
|----------|------|--------|
| 173 | NPC_SELFDESTRUCTION | Massive AoE neutral damage centered on self, then die. 400% ATK multiplier. |
| 197 | NPC_EMOTION | Display emotion icon (exclamation, heart, anger, etc.) |
| 200 | NPC_HALLUCINATION | Cause hallucination status on target (screen distortion) |
| 331 | NPC_RANDOMMOVE | Teleport to random location on map |
| 342 | NPC_CHANGEUNDEAD | Change target's element to Undead |
| 667 | NPC_WIDEBLEEDING | AoE Bleeding |
| 668 | NPC_WIDESILENCE | AoE Silence |
| 669 | NPC_WIDESTUN | AoE Stun |
| 664 | NPC_WIDECONFUSE | AoE Confusion |
| 665 | NPC_WIDESLEEP | AoE Sleep |
| 666 | NPC_WIDESIGHT | AoE hidden/cloak detection (sight) |
| 672 | NPC_WIDECURSE | AoE Curse |
| 673 | NPC_WIDEFREEZE | AoE Freeze |
| 674 | NPC_WIDESTONE | AoE Stone Curse |

#### HP Drain Skills

| Skill ID | Name | Effect |
|----------|------|--------|
| 199 | NPC_BLOODDRAIN | Drain HP from target, heal self for amount drained |
| 200 | NPC_ENERGYDRAIN | Drain SP from target |

---

### 3.2 Skill Casting Conditions (HP%, Idle, Attack, Target Distance)

#### mob_skill_db Format

Each entry in rAthena's `mob_skill_db.txt` follows this format:

```
MobID,Info@SkillName,State,SkillID,SkillLv,Rate,CastTime,Delay,Cancelable,Target,Condition,ConditionValue,val1,val2,val3,val4,val5,Emotion
```

#### State Field (When the Skill Can Trigger)

| State | Description | AI Context |
|-------|-------------|------------|
| `any` | Any state -- always eligible | Checked every AI tick regardless of state |
| `idle` | Monster is in IDLE state (not chasing, not attacking) | Wandering or standing still |
| `walk` | Monster is moving (but not chasing a target) | During wander movement |
| `chase` | Monster is chasing a target | Moving toward target in CHASE state |
| `attack` | Monster is in ATTACK state (in range, auto-attacking) | Actively fighting a target |
| `angry` | "Hyper-active" pre-attack state | Only for AI types with MD_ANGRY flag |
| `follow` | Following target before engaging | Pre-attack approach state |
| `dead` | Monster just died | Triggers on death (for NPC_REBIRTH, etc.) |
| `loot` | Monster is picking up items | Only for looter AI types |
| `anytarget` | Like `any` but requires a valid target | Must have targetPlayerId set |

#### Condition Types

| Condition | conditionValue | Description |
|-----------|---------------|-------------|
| `always` | -- | No condition beyond state and rate check |
| `onspawn` | -- | Triggers once when the monster first spawns |
| `myhpltmaxrate` | X (percentage) | Monster's current HP is below X% of maxHP |
| `myhpinrate` | X (min%), val1 = Y (max%) | Monster's HP is between X% and Y% of maxHP |
| `mystatuson` | status_id | Monster currently has the specified status effect |
| `mystatusoff` | status_id | Monster does NOT have the specified status effect |
| `friendhpltmaxrate` | X (percentage) | A nearby ally's HP is below X% |
| `friendstatuson` | status_id | A nearby ally has the specified status |
| `friendstatusoff` | status_id | A nearby ally does NOT have the specified status |
| `attackpcgt` | X (count) | Number of players attacking this monster is greater than X |
| `attackpcge` | X (count) | Number of players attacking >= X |
| `slavelt` | X (count) | Current slave count is less than X |
| `slavele` | X (count) | Current slave count is <= X |
| `closedattacked` | -- | Monster was just hit by a melee attack |
| `longrangeattacked` | -- | Monster was just hit by a ranged attack |
| `skillused` | skill_id | A specific skill was just used on the monster |
| `casttargeted` | -- | A player is currently casting a skill targeting this monster |
| `rudeattacked` | -- | Player attacks then immediately runs away (kiting detection) |
| `masterhpltmaxrate` | X (percentage) | Master's HP below X% (for slave monsters) |
| `masterattacked` | -- | Master is being attacked |
| `alchemist` | -- | Monster has Alchemist AI (homunculus/summon) |
| `groundattacked` | -- | Hit by a ground-targeted skill |
| `damagedgt` | X (damage) | A single attack dealt more than X damage |

#### Target Types

| Target | Description |
|--------|-------------|
| `target` | The monster's current attack target |
| `self` | The monster itself |
| `friend` | A nearby same-type ally |
| `master` | The monster's master (for slaves) |
| `randomtarget` | A random player in range (ignoring current target) |
| `around1` through `around8` | Ground-targeted AoE at positions around the monster |

---

### 3.3 Skill Cast Timing and Cooldowns

#### Timing Fields

| Field | Description | Range |
|-------|-------------|-------|
| `CastTime` | Time in ms before skill activates (cast bar) | 0 (instant) to 10000+ |
| `Delay` | Cooldown after use before the same skill can be used again | 0 (no cooldown) to 300000+ |
| `Cancelable` | Whether the cast can be interrupted by taking damage | `yes` / `no` |

#### Cast Time Behavior

- `CastTime = 0`: Skill fires instantly, no cast bar displayed
- `CastTime > 0`: Monster enters casting state; `enemy:casting` event emitted
- If `Cancelable = yes` and monster takes damage during cast: cast is interrupted, `enemy:cast_interrupted` event emitted
- If `Cancelable = no`: cast always completes regardless of damage taken (typical for boss abilities)

#### Cooldown Behavior

- `Delay = 0`: Skill can be used again on the next AI tick (200ms)
- `Delay > 0`: Skill is on cooldown for that many milliseconds
- Each skill entry has its own independent cooldown timer
- Multiple entries of the same skill CAN have different delays (rAthena `monster_ai` flag 0x200 controls per-entry vs per-skill delays)

#### Rate Behavior

- Rate is checked each AI tick (200ms) when the state and condition match
- Rate is out of 10000 (10000 = 100%, 5000 = 50%, 500 = 5%)
- Skills are checked top-to-bottom in the mob_skill_db entry list
- First skill that passes state + condition + rate check is selected
- If a skill is selected, the monster skips its auto-attack for that tick
- Typical rates: 500-2000 (5-20%) for regular skills, 100-500 (1-5%) for powerful skills, 10000 (100%) for conditional triggers (like SUMMONSLAVE when slavelt condition met)

---

### 3.4 Slave Spawning Mechanics (Master/Slave Lifecycle)

#### NPC_SUMMONSLAVE

When a monster uses NPC_SUMMONSLAVE, it creates "slave" monsters bound to the caster:

**mob_skill_db format**:
```
1039,Baphomet@NPC_SUMMONSLAVE,idle,196,5,10000,700,30000,no,self,slavelt,2,1101,0,0,0,0,0
```
- val1 through val5 contain monster IDs of the slaves to summon
- Up to 5 different slave types per skill entry
- The `slavelt` / `slavele` condition controls the maximum slave count

#### Slave Lifecycle Rules

| Rule | Behavior |
|------|----------|
| **Spawn location** | Slaves spawn at the master's current position |
| **Movement** | Slaves follow the master when it moves; movement speed matches master's speed at summon time |
| **Master moves out of range** | Slaves stand still until master returns to their vicinity |
| **Target selection** | Slaves attack whoever the master is targeting |
| **Target change** | When master changes targets, slaves continue attacking their current target until it dies or leaves range |
| **Slave attacked** | If attacked while idle (master not fighting), slaves retaliate against the attacker |
| **Master teleport** | Slaves teleport with the master to its new location |
| **Master death** | ALL slaves die immediately; slaves yield NO EXP and NO drops when dying this way |
| **Slave death** | Does not affect master; master may re-summon via `slavelt` condition |
| **EXP from slaves** | Slaves killed individually (before master dies) DO give EXP and drops normally |
| **Slave AI** | Typically AI type 24 (Passive Slave: CanMove + NoRandomWalk + CanAttack) |
| **Slave count limit** | Controlled by `slavelt` / `slavele` condition value in mob_skill_db |

#### NPC_CALLSLAVE (Skill ID 352)

Teleports all living slaves back to the master's current position instantly:
- Used when slaves have wandered too far from the master
- Does not create new slaves, only recalls existing ones
- Instant effect, no cast time
- Common on MVPs that move frequently (Baphomet, Eddga)

#### Common Master/Slave Combinations

| Master (MVP) | Slave Monster(s) | Max Slaves | Condition |
|-------------|-----------------|------------|-----------|
| Baphomet (1039) | Dark Illusion (1101) | 2 | slavelt 2 |
| Dark Lord (1272) | Dark Illusion (1101), Wraith (1024) | 3 | slavelt 3 |
| Eddga (1115) | Bigfoot (1060) | 3 | slavelt 3 |
| Orc Lord (1190) | High Orc (1196), Orc Archer (1193) | 4 | slavelt 4 |
| Osiris (1038) | Ancient Mummy (1297) | 2 | slavelt 2 |
| Maya (1147) | Maya Purple (1289), Ant Egg (1097) | 3 | slavelt 3 |
| Moonlight Flower (1150) | Ninetail (1180) | 2 | slavelt 2 |
| Drake (1112) | Pirate Skeleton (1196) | 2 | slavelt 2 |
| Hatii (1252) | Hatii Baby (1253) | 3 | slavelt 3 |
| Pharaoh (1157) | Mummy (1041), Isis (1029) | 3 | slavelt 3 |
| Turtle General (1312) | Assaulter (1306), Permeter (1307) | 4 | slavelt 4 |

---

### 3.5 Metamorphosis (Monster Transformation)

#### NPC_METAMORPHOSIS (Skill ID 304)

When used, the monster transforms into a completely different monster type:

**mob_skill_db format**:
```
1068,Thief Bug@NPC_METAMORPHOSIS,attack,304,1,500,0,5000,no,self,myhpltmaxrate,30,1057,1058,0,0,0,0
```
- val1 through val5 contain possible target monster IDs
- One is chosen randomly from the non-zero values
- The transformation replaces the original monster entirely

#### Transformation Rules

| Rule | Behavior |
|------|----------|
| **HP** | New monster spawns at FULL HP (maxHP of the new template) |
| **Stats** | Completely replaced with the new monster's stats, DEF, MDEF, element, etc. |
| **EXP** | The NEW monster's EXP values are used (not the original) |
| **Drops** | The NEW monster's drop table is used (not the original) |
| **Position** | Stays at the same position; does not respawn elsewhere |
| **Aggro** | inCombatWith is preserved; continues fighting the same target |
| **Visual** | Client receives `enemy:transform` event to swap the model |
| **Enemy ID** | Remains the same (same enemy object, updated stats) |
| **Respawn** | When the transformed monster dies, the ORIGINAL monster respawns (not the transformed version) |
| **Slaves** | If the original had slaves, behavior varies (typically slaves persist) |

#### Implementation Concern

A known rAthena issue (#1395): If NPC_METAMORPHOSIS is not implemented correctly, the original monster can respawn while the transformed version still exists, creating "infinite mobs." The fix is to ensure the original enemy ID is reused (not creating a new entity) and that the respawn timer only activates when the transformed version dies.

---

## 4. Mini-Boss System

### 4.1 Differences from MVP

Mini-bosses (also called "Boss-type" monsters by the community) have Boss Protocol but lack MVP Protocol. The differences are significant:

| Feature | Mini-Boss | MVP |
|---------|-----------|-----|
| Boss Protocol (immune/detect/knockback) | YES | YES |
| MVP Protocol (rewards/tombstone) | NO | YES |
| MVP EXP bonus | NO | YES |
| MVP drops (3 slots) | NO | YES |
| Tombstone on death | NO | YES |
| Server-wide announcement | NO | YES |
| FFA loot | NO (first-hit priority) | YES |
| Respawn timer | Shorter (1-5 hours typical) | Longer (1-24 hours) |
| Typical HP pool | 5,000 - 200,000 | 100,000 - 6,700,000 |
| Appears on MVP tracker | Usually no | YES |
| Can be summoned by Dead/Bloody Branch | Some can | YES |
| Slave summoning | Rare | Common |

### 4.2 Complete Mini-Boss List (Pre-Renewal)

| ID | Name | Level | HP | Element | Race | Size | Spawn Location | Respawn |
|----|------|-------|-----|---------|------|------|----------------|---------|
| 1096 | Mastering | 25 | 4,990 | Neutral 3 | Plant | Small | prt_fild04 | ~30 min |
| 1582 | Angeling | 54 | 41,350 | Holy 3 | Angel | Small | yuno_fild07 | ~60 min |
| 1584 | Deviling | 62 | 49,000 | Shadow 4 | Demon | Small | pay_fild04 | ~120 min |
| 1120 | Ghostring | 60 | 32,700 | Ghost 4 | Demon | Small | pay_dun04 | ~60 min |
| 1583 | Tao Gunka | 70 | 193,000 | Neutral 3 | Demon | Large | beach_dun | ~120 min |
| 1785 | Atroce | 82 | 1,008,420 | Shadow 3 | Brute | Large | ra_fild02-04 | ~300 min |
| 1630 | Bacsojin | 85 | 253,221 | Wind 3 | Demi-Human | Large | lou_dun03 | ~120 min |
| 1388 | Arc Angeling | 68 | 45,100 | Holy 3 | Angel | Medium | yuno_fild01 | ~60 min |
| 1092 | Vagabond Wolf | 40 | 8,302 | Earth 1 | Brute | Medium | prt_fild06 | ~60 min |
| 1091 | Eclipse | 25 | 6,050 | Neutral 3 | Brute | Small | pay_fild01 | ~60 min |
| 1089 | Toad | 18 | 2,005 | Water 1 | Fish | Medium | prt_fild00 | ~60 min |

**Note**: Some sources classify Tao Gunka, Atroce, and Bacsojin as MVPs (they have MvpExp in some databases). Classification varies by server version and episode.

---

## 5. Complete MVP List

### 5.1 All Pre-Renewal MVPs with Stats

| ID | Name | Lv | HP | ATK Range | DEF | MDEF | Element | Race | Size | Base EXP | Job EXP | MVP EXP | Spawn Map | Respawn (min) |
|----|------|----|----|-----------|-----|------|---------|------|------|----------|---------|---------|-----------|---------------|
| 1511 | Amon Ra | 88 | 1,214,138 | 1,647-2,576 | 26 | 52 | Earth 3 | Demi-Human | Large | 87,264 | 35,891 | 43,632 | moc_pryd06 | 60-70 |
| 1039 | Baphomet | 81 | 668,000 | 3,220-4,040 | 35 | 45 | Shadow 3 | Demon | Large | 107,250 | 37,895 | 53,625 | prt_maze03 | 120-130 |
| 1874 | Beelzebub | 98 | 6,666,666 | 10,000-13,410 | 40 | 40 | Ghost 4 | Demon | Large | 6,666,666 | 6,666,666 | 3,333,333 | abbey03 | 720-730 |
| 1272 | Dark Lord | 80 | 720,000 | 2,800-3,320 | 30 | 70 | Undead 4 | Demon | Large | 65,780 | 45,045 | 32,890 | gl_chyard | 60-70 |
| 1719 | Detardeurus | 90 | 960,000 | 4,560-5,548 | 66 | 59 | Shadow 3 | Dragon | Large | 291,850 | 123,304 | 145,925 | abyss_03 | 180-190 |
| 1046 | Doppelganger | 72 | 249,000 | 1,340-1,590 | 60 | 35 | Shadow 3 | Demon | Medium | 51,480 | 10,725 | 25,740 | gef_dun02 | 120-130 |
| 1389 | Dracula | 85 | 320,096 | 1,625-1,890 | 45 | 76 | Shadow 4 | Demon | Large | 120,157 | 38,870 | 60,078 | gef_dun01 | 60-70 |
| 1112 | Drake | 70 | 326,666 | 1,800-2,100 | 20 | 35 | Undead 1 | Undead | Medium | 28,600 | 22,880 | 14,300 | treasure02 | 120-130 |
| 1115 | Eddga | 65 | 152,000 | 1,215-1,565 | 15 | 15 | Fire 1 | Brute | Large | 25,025 | 12,870 | 12,512 | pay_fild11 | 120-130 |
| 1086 | Golden Thief Bug | 64 | 126,000 | 870-1,145 | 60 | 45 | Fire 2 | Insect | Large | 14,300 | 7,150 | 7,150 | prt_sewb4 | 60-70 |
| 1252 | Hatii (Garm) | 73 | 197,000 | 1,700-1,900 | 40 | 45 | Water 4 | Brute | Large | 50,050 | 20,020 | 25,025 | xmas_fild01 | 120-130 |
| 1147 | Maya | 81 | 169,000 | 1,800-2,070 | 60 | 25 | Earth 4 | Insect | Large | 42,900 | 17,875 | 21,450 | anthell02 | 120-130 |
| 1059 | Mistress | 74 | 212,000 | 880-1,110 | 40 | 60 | Wind 4 | Insect | Small | 39,325 | 27,170 | 19,662 | mjolnir_04 | 120-130 |
| 1150 | Moonlight Flower | 67 | 120,000 | 1,200-1,700 | 10 | 55 | Fire 3 | Demon | Medium | 27,500 | 14,300 | 13,750 | pay_dun04 | 60-70 |
| 1087 | Orc Hero | 77 | 585,700 | 2,257-2,542 | 40 | 45 | Earth 2 | Demi-Human | Large | 58,630 | 32,890 | 29,315 | gef_fild03 | 1440-1450 |
| 1190 | Orc Lord | 74 | 783,000 | 3,700-4,150 | 40 | 5 | Earth 4 | Demi-Human | Large | 62,205 | 8,580 | 31,102 | gef_fild10 | 120-130 |
| 1038 | Osiris | 78 | 415,400 | 780-2,880 | 10 | 25 | Undead 4 | Undead | Medium | 71,500 | 28,600 | 35,750 | moc_pryd04 | 60-70 |
| 1157 | Pharaoh | 93 | 445,997 | 2,267-3,015 | 67 | 70 | Shadow 3 | Demi-Human | Large | 114,990 | 41,899 | 57,495 | in_sphinx5 | 60-70 |
| 1159 | Phreeoni | 69 | 188,000 | 880-1,530 | 10 | 20 | Neutral 3 | Brute | Large | 32,175 | 16,445 | 16,087 | moc_fild17 | 120-130 |
| 1312 | Turtle General | 97 | 320,700 | 2,438-3,478 | 50 | 54 | Earth 2 | Brute | Large | 18,202 | 9,800 | 9,101 | tur_dun04 | 60-70 |
| 1751 | Valkyrie Randgris | 99 | 3,567,200 | 5,560-9,980 | 25 | 42 | Holy 4 | Angel | Large | 2,854,900 | 3,114,520 | 1,427,450 | odin_tem03 | 480-490 |
| 1768 | Gloom Under Night | 89 | 1,190,000 | 3,746-8,296 | 30 | 45 | Ghost 3 | Formless | Large | 330,830 | 167,890 | 165,415 | ra_san05 | 300-310 |
| 1785 | Atroce | 82 | 1,008,420 | 2,526-3,646 | 25 | 25 | Shadow 3 | Brute | Large | 295,550 | 118,895 | 147,775 | ra_fild02 | 300-310 |
| 1734 | Kiel D-01 | 90 | 1,523,000 | 4,238-5,040 | 30 | 70 | Neutral 3 | Demi-Human | Medium | 600,000 | 500,000 | 300,000 | kh_dun02 | 120-130 |
| 1647 | Eremes Guile (AC) | 99 | 1,411,230 | 4,189-8,289 | 37 | 39 | Poison 4 | Demi-Human | Medium | 4,083,400 | 1,592,380 | 2,041,700 | lhz_dun03 | 120-130 |
| 1648 | Howard Alt-Eisen (WS) | 99 | 1,349,040 | 3,893-5,765 | 66 | 22 | Earth 4 | Demi-Human | Medium | 4,030,000 | 1,562,830 | 2,015,000 | lhz_dun03 | 120-130 |
| 1649 | Margaretha Sorin (HP) | 99 | 1,268,250 | 2,822-4,540 | 40 | 79 | Holy 4 | Demi-Human | Medium | 3,913,200 | 1,502,450 | 1,956,600 | lhz_dun03 | 120-130 |
| 1650 | Seyren Windsor (LK) | 99 | 1,669,533 | 5,700-10,890 | 52 | 35 | Fire 4 | Demi-Human | Medium | 4,324,380 | 1,658,450 | 2,162,190 | lhz_dun03 | 120-130 |
| 1651 | Cecil Damon (Sniper) | 99 | 1,349,040 | 3,850-8,800 | 26 | 51 | Wind 4 | Demi-Human | Medium | 4,083,400 | 1,592,380 | 2,041,700 | lhz_dun03 | 120-130 |
| 1652 | Kathryne Keyron (HW) | 99 | 1,190,880 | 3,290-6,680 | 21 | 82 | Ghost 4 | Demi-Human | Medium | 3,736,830 | 1,432,910 | 1,868,415 | lhz_dun03 | 120-130 |
| 2068 | Boitata | 93 | 1,283,990 | 3,304-4,266 | 32 | 66 | Fire 3 | Brute | Large | 74,288 | 77,950 | 37,144 | bra_dun02 | 120-130 |
| 1583 | Tao Gunka | 70 | 193,000 | 1,450-1,770 | 20 | 20 | Neutral 3 | Demon | Large | 59,175 | 10,445 | 29,587 | beach_dun | 300-310 |
| 1688 | Lady Tanee | 89 | 720,000 | 2,148-5,765 | 26 | 48 | Wind 3 | Demi-Human | Large | 100,000 | 100,000 | 50,000 | ayo_dun02 | 420-430 |
| 1658 | Stormy Knight | 73 | 375,000 | 2,780-3,260 | 20 | 56 | Wind 4 | Formless | Large | 53,060 | 21,880 | 26,530 | xmas_dun02 | 120-130 |

### 5.2 MVP Key Stats Summary

| MVP | Key Stat Highlights | Signature Skills | Signature Drops |
|-----|-------------------|-----------------|-----------------|
| Amon Ra | STR 1, INT 85, DEX 120 | Meteor Storm, Heal, Summoning | Fragment of Rossata Stone |
| Baphomet | AGI 152, DEX 120, LUK 95 | Brandish Spear, LoV, Heal, SUMMONSLAVE | Crescent Scythe, Baphomet Doll |
| Beelzebub | Highest HP (6.67M), ATK 10k+ | (Episode 12+ content) | Staff of Soul, Beelzebub Card |
| Dark Lord | INT 118, MDEF 70 | Meteor Storm, Fire Wall, Hell's Judgement | Evil Bone Wand, Grimtooth |
| Doppelganger | DEF 60 (highest phys DEF) | Fast attack, Summoning | Spiky Band, Doppelganger Card |
| Drake | INT 75, Water Ball Lv9 | MG_WATERBALL (copyable!), Brandish Spear | Ring Pommel Saber, Drake Card |
| Eddga | VIT 85, Fire 1 | Power Up, SUMMONSLAVE (Bigfoot) | Tiger's Footskin, Eddga Card |
| Golden Thief Bug | DEF 60, LUK 150 | Heal, Fire Attack | Emperium, GTB Card |
| Hatii | AGI 126 | Storm Gust, Frost Diver, Frost Nova | Fang of Hatii, Hatii Card |
| Maya | DEF 60, Earth 4 | Heal, Summoning (Maya Purple, Ant Egg) | Maya Card (reflect single-target magic) |
| Mistress | AGI 165 (fastest), LUK 130 | Jupitel Thunder, Heal, Pneuma | Mistress Card (no gemstones) |
| Moonlight Flower | LUK 120 | Mammonite, Heal, elem Bolts, Earth Spike | Moonlight Dagger, MLF Card (no speed penalty) |
| Orc Hero | 24-hour respawn | Power Up, massive HP (585k) | Heroic Emblem, Orc Hero Card |
| Orc Lord | HP 783k, Earth 4 | SUMMONSLAVE (High Orc, Orc Archer) | Erde, Orc Lord Card |
| Osiris | Undead 4, wide ATK range | Curse Attack, Dark Blessing, Summoning | Hand of God, Osiris Card |
| Pharaoh | VIT 100, INT 104, LUK 112 | Fire Wall, Meteor Storm, Heal | Solar Sword, Pharaoh Card (SP -30%) |
| Phreeoni | Neutral 3 | Moderate skills | Fortune Sword, Phreeoni Card (HIT +100) |
| Turtle General | Lv 97 | Multiple skill sets | Turtle General Card (+20% melee to all) |
| Valkyrie Randgris | DEX 220, LUK 210, 3.57M HP | Dispell, Grand Cross, Holy Cross | Valkyrian set, Randgris Card |

### 5.3 MVP Skill Tables (Representative Entries from mob_skill_db)

#### Baphomet (1039)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | idle | NPC_SUMMONSLAVE | 5 | 10000 | slavelt 2 | 30000 |
| 2 | attack | Brandish Spear | 20 | 500 | always | 3000 |
| 3 | attack | Lord of Vermillion | 5 | 300 | attackpcgt 2 | 10000 |
| 4 | chase | NPC_DARKBREATH | 5 | 500 | always | 5000 |
| 5 | attack | AL_HEAL | 10 | 200 | myhpltmaxrate 50 | 10000 |
| 6 | attack | NPC_POWERUP | 5 | 300 | always | 30000 |
| 7 | idle | NPC_CALLSLAVE | 1 | 10000 | slavelt 1 | 30000 |

#### Dark Lord (1272)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | idle | NPC_SUMMONSLAVE | 1 | 10000 | slavelt 3 | 30000 |
| 2 | attack | MG_METEORSTORM | 10 | 400 | always | 8000 |
| 3 | attack | MG_FIREWALL | 10 | 300 | always | 5000 |
| 4 | attack | NPC_HELLJUDGEMENT | 5 | 200 | myhpltmaxrate 30 | 15000 |
| 5 | chase | NPC_DARKBREATH | 5 | 500 | always | 5000 |
| 6 | attack | AL_INCAGI | 10 | 200 | always | 30000 |

#### Osiris (1038)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | idle | NPC_SUMMONSLAVE | 1 | 10000 | slavelt 2 | 30000 |
| 2 | attack | NPC_CURSEATTACK | 5 | 500 | always | 5000 |
| 3 | attack | NPC_DARKNESSATTACK | 5 | 500 | always | 3000 |
| 4 | attack | AL_HEAL | 11 | 300 | myhpltmaxrate 50 | 10000 |
| 5 | idle | AL_TELEPORT | 1 | 500 | rudeattacked | 5000 |

#### Pharaoh (1157)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | idle | NPC_SUMMONSLAVE | 1 | 10000 | slavelt 3 | 30000 |
| 2 | attack | MG_FIREWALL | 10 | 400 | always | 5000 |
| 3 | attack | MG_METEORSTORM | 10 | 300 | always | 10000 |
| 4 | attack | AL_HEAL | 11 | 300 | myhpltmaxrate 40 | 10000 |
| 5 | attack | NPC_POWERUP | 5 | 200 | always | 30000 |
| 6 | idle | AL_TELEPORT | 1 | 500 | rudeattacked | 5000 |

#### Mistress (1059)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | attack | MG_JUPITEL | 10 | 400 | always | 3000 |
| 2 | attack | AL_HEAL | 9 | 300 | myhpltmaxrate 50 | 10000 |
| 3 | attack | PR_LEXDIVINA | 3 | 200 | always | 10000 |
| 4 | attack | AL_PNEUMA | 1 | 300 | longrangeattacked | 5000 |
| 5 | attack | NPC_COMBOATTACK | 3 | 300 | always | 3000 |

#### Golden Thief Bug (1086)

| Priority | State | Skill | Level | Rate | Condition | Delay |
|----------|-------|-------|-------|------|-----------|-------|
| 1 | attack | AL_HEAL | 9 | 300 | myhpltmaxrate 50 | 10000 |
| 2 | attack | NPC_FIREATTACK | 3 | 500 | always | 3000 |
| 3 | attack | NPC_DEFENSE | 3 | 200 | myhpltmaxrate 30 | 30000 |

---

## 6. Implementation Checklist

### 6.1 Server-Side Implementation Status

| Feature | Status | File(s) | Notes |
|---------|--------|---------|-------|
| Monster template system (509 monsters) | DONE | `ro_monster_templates.js` | All pre-renewal monsters imported |
| Boss Protocol auto-apply | DONE | `index.js` (spawnEnemy) | knockbackImmune, statusImmune, detector |
| MVP flag on mode flags | DONE | `index.js` (parseModeFlags) | MD_MVP = 0x80000 |
| AI state machine (IDLE/CHASE/ATTACK/DEAD) | DONE | `index.js` | 200ms tick, full state transitions |
| Mode flag system (18 flags) | DONE | `index.js` | AI types 1-27 mapped |
| Monster skill database | PARTIAL | `ro_monster_skills.js` | 23 monsters with skills, needs expansion to 100+ |
| NPC_ skill execution | PARTIAL | `index.js` (executeNPCSkill) | Elemental attacks, status attacks, combo attack done |
| Player skill execution by monsters | DONE | `index.js` (executeMonsterPlayerSkill) | Plagiarism integration working |
| Skill casting system | DONE | `index.js` (processMonsterCasting) | Cast bars, interruptible/uninterruptible |
| Skill conditions (HP%, slavelt, etc.) | PARTIAL | `index.js` | `always`, `myhpltmaxrate`, `rudeattacked`, `slavelt` done |
| NPC_SUMMONSLAVE | DONE | `index.js` | Master/slave lifecycle, slaves die with master |
| NPC_CALLSLAVE | NOT DONE | -- | Needs implementation |
| NPC_METAMORPHOSIS | DONE | `index.js` | Template swap, stats replaced |
| NPC_SELFDESTRUCTION | DONE | `ro_monster_skills.js` | 400% ATK AoE, self-kill |
| NPC_REBIRTH | NOT DONE | -- | Needs death-trigger skill system |
| NPC_POWERUP | PARTIAL | `ro_monster_skills.js` | Defined but buff application needs work |
| NPC_AGIUP | PARTIAL | `ro_monster_skills.js` | Defined but buff application needs work |
| NPC_DEFENSE | NOT DONE | -- | DEF buff for monsters |
| NPC_BARRIER | NOT DONE | -- | Invincibility barrier |
| NPC_KEEPING | NOT DONE | -- | DEF 90, cannot move/attack |
| NPC_STONESKIN | NOT DONE | -- | +DEF%, -MDEF% |
| NPC_ANTIMAGIC | NOT DONE | -- | +MDEF%, -DEF% |
| NPC_EARTHQUAKE | NOT DONE | -- | 3-tick AoE, critical for Baphomet |
| NPC_HELLJUDGEMENT | NOT DONE | -- | Dark Lord signature skill |
| NPC_DRAGONFEAR | NOT DONE | -- | AoE random status |
| NPC_GRANDDARKNESS | NOT DONE | -- | AoE Shadow Grand Cross |
| Breath attacks (Fire/Ice/Thunder/Acid) | NOT DONE | -- | Elemental breath skills |
| Wide-area status skills | NOT DONE | -- | WIDEBLEEDING, WIDESILENCE, WIDESTUN, etc. |
| NPC_BLOODDRAIN / ENERGYDRAIN | NOT DONE | -- | HP/SP drain skills |
| NPC_HALLUCINATION | NOT DONE | -- | Screen distortion effect |
| NPC_RANDOMMOVE | NOT DONE | -- | Monster teleport |
| MVP reward system | DONE | `index.js` | Damage tracking, MVP winner, MVP EXP |
| MVP drop rolls (3 slots) | DONE | `index.js` | Independent rolls, inventory placement |
| MVP announcement | DONE | `index.js` | Server-wide broadcast |
| MVP tombstone | PARTIAL | `index.js` | `mvp:tombstone` event emitted, client display needed |
| MVP respawn variance | DONE | `index.js` | Base timer + 0-10 min random |
| Mini-boss differentiation | DONE | `ro_monster_templates.js` | monsterClass: 'boss' vs 'mvp' |
| Damage-based MVP determination | DONE | `index.js` | Highest totalDamage wins |
| Slave death on master death | DONE | `index.js` | No EXP/drops from slave death via master |
| Slave following master | PARTIAL | `index.js` | Basic following, needs teleport-with-master |

### 6.2 Client-Side Implementation Status

| Feature | Status | File(s) | Notes |
|---------|--------|---------|-------|
| Enemy spawn/despawn actors | DONE | `EnemySubsystem.*` | Registry with actor management |
| Enemy health bars | DONE | `WorldHealthBarSubsystem.*` | Overhead health bars |
| Enemy death handling | DONE | `EnemySubsystem.*` | Death event, actor removal |
| Enemy skill cast bar | NOT DONE | -- | `enemy:casting` event exists, no UI |
| Enemy skill VFX | NOT DONE | -- | `enemy:skill_used` event exists, no VFX mapping |
| MVP tombstone display | NOT DONE | -- | `mvp:tombstone` event exists, no client handler |
| MVP announcement UI | PARTIAL | `ChatSubsystem.*` | Uses chat system, may need special formatting |
| MVP reward notification | PARTIAL | -- | EXP gain shown, MVP title display needed |
| Boss/MVP special health bar | NOT DONE | -- | No visual distinction for boss-class enemies |
| Slave spawn/death sync | DONE | `EnemySubsystem.*` | Slaves spawn/die as normal enemies |

---

## 7. Gap Analysis

### 7.1 Critical Gaps (Block MVP Content)

| Gap | Impact | Priority | Effort |
|-----|--------|----------|--------|
| **NPC_EARTHQUAKE implementation** | Baphomet's signature AoE; many MVPs use it | P0 | Medium |
| **NPC_HELLJUDGEMENT implementation** | Dark Lord's signature skill | P0 | Medium |
| **Breath attack skills (5 types)** | Used by dragons, MVPs with breath abilities | P0 | Medium |
| **NPC_CALLSLAVE** | MVPs need to recall slaves after teleport | P1 | Low |
| **Monster skill DB expansion** | Only 23/509 monsters have skills; need 100+ for zones 4-9 | P0 | High |
| **Skill condition expansion** | Missing: `attackpcgt`, `attackpcge`, `closedattacked`, `longrangeattacked`, `masterattacked`, `damagedgt`, `friendhpltmaxrate`, `casttargeted` | P1 | Medium |
| **Monster self-buff system** | NPC_POWERUP/AGIUP/DEFENSE/BARRIER/KEEPING/STONESKIN/ANTIMAGIC need buff application | P1 | Medium |

### 7.2 Important Gaps (Enhance MVP Experience)

| Gap | Impact | Priority | Effort |
|-----|--------|----------|--------|
| **Client MVP tombstone display** | Players cannot see tombstones or track MVPs | P2 | Medium |
| **Client enemy cast bar** | No visual indicator when monsters are casting | P2 | Medium |
| **Client enemy skill VFX** | Monster skills have no visual effects | P2 | High |
| **Boss/MVP distinct health bar** | No visual distinction between normal and boss enemies | P2 | Low |
| **Wide-area status skills (8 types)** | Used by high-level dungeon monsters | P2 | Medium |
| **NPC_BLOODDRAIN / ENERGYDRAIN** | HP/SP drain used by several monsters | P2 | Low |
| **NPC_GRANDDARKNESS** | AoE Shadow Grand Cross, used by Lord of Death | P2 | Medium |
| **NPC_DRAGONFEAR** | AoE random status, used by dragons | P2 | Medium |
| **NPC_REBIRTH** | Monster respawn on death, used by special monsters | P3 | Medium |
| **NPC_RANDOMMOVE** | Monster teleport, used for kite-prevention | P3 | Low |

### 7.3 Nice-to-Have Gaps

| Gap | Impact | Priority | Effort |
|-----|--------|----------|--------|
| **NPC_HALLUCINATION** | Screen distortion effect (client-only visual) | P3 | Low |
| **NPC_EMOTION** | Emotion bubbles over monsters | P3 | Low |
| **MVP damage-tanked tracking** | Currently only tracks damage dealt, not tanked | P2 | Low |
| **Monster enhanced skill levels** | Monsters using player skills above max level (Heal Lv11, Cold Bolt Lv20) | P2 | Low |
| **Looter behavior** | Monsters picking up dropped items | P3 | Medium |
| **Angry/Follow AI states** | Pre-attack states for AI type 4 (MD_ANGRY) | P3 | Medium |
| **Cast sensor idle/chase** | Monsters detecting players casting nearby | P2 | Low |

### 7.4 Implementation Priority Order

1. **Phase A**: Expand monster skill DB to 50+ monsters (focus on zones 4-9 monsters)
2. **Phase B**: Implement NPC_EARTHQUAKE, NPC_HELLJUDGEMENT, breath attacks, NPC_GRANDDARKNESS
3. **Phase C**: Monster self-buff system (POWERUP, AGIUP, DEFENSE, BARRIER, STONESKIN, ANTIMAGIC)
4. **Phase D**: Expand skill conditions (attackpcgt, closedattacked, longrangeattacked, damagedgt)
5. **Phase E**: NPC_CALLSLAVE, NPC_REBIRTH, NPC_RANDOMMOVE
6. **Phase F**: Client -- enemy cast bar, MVP tombstone display, boss health bar distinction
7. **Phase G**: Client -- enemy skill VFX mapping, monster emotion display
8. **Phase H**: Wide-area status skills, drain skills, remaining NPC_ skills
9. **Phase I**: Looter behavior, angry/follow states, cast sensor, enhanced skill levels

---

## Sources

- [MVP - iRO Wiki Classic](https://irowiki.org/classic/MVP)
- [MVP - iRO Wiki](https://irowiki.org/wiki/MVP)
- [Boss Protocol - iRO Wiki](https://irowiki.org/wiki/Boss_Protocol)
- [Category: Monster Exclusive Skills - iRO Wiki](https://irowiki.org/wiki/Category:Monster_Exclusive_Skills)
- [NPC_SUMMONSLAVE - iRO Wiki](https://irowiki.org/wiki/NPC_SUMMONSLAVE)
- [NPC_EARTHQUAKE - iRO Wiki](https://irowiki.org/wiki/NPC_EARTHQUAKE)
- [NPC_POWERUP - iRO Wiki](https://irowiki.org/wiki/NPC_POWERUP)
- [NPC_STONESKIN - iRO Wiki](https://irowiki.org/wiki/NPC_STONESKIN)
- [NPC_ANTIMAGIC - iRO Wiki](https://irowiki.org/wiki/NPC_ANTIMAGIC)
- [NPC_KEEPING - iRO Wiki](https://irowiki.org/wiki/NPC_KEEPING)
- [NPC_DRAGONFEAR - iRO Wiki](https://irowiki.org/wiki/NPC_DRAGONFEAR)
- [Ankle Snare - iRO Wiki Classic](https://irowiki.org/classic/Ankle_Snare)
- [Ankle Snare Minimum Duration - rAthena Issue #8253](https://github.com/rathena/rathena/issues/8253)
- [MVP Boss Monsters - RO Monster Database (Pre-re)](https://ratemyserver.net/index.php?page=mob_db&mvp=1&mob_search=Search&sort_r=0)
- [rAthena mob.cpp source](https://github.com/rathena/rathena/blob/master/src/map/mob.cpp)
- [rAthena mob.hpp source](https://github.com/rathena/rathena/blob/master/src/map/mob.hpp)
- [rAthena mob_db.txt documentation](https://github.com/rathena/rathena/blob/master/doc/mob_db.txt)
- [rAthena mob_db_mode_list.txt](https://github.com/rathena/rathena/blob/master/doc/mob_db_mode_list.txt)
- [rAthena pre-renewal mob_skill_db.txt](https://github.com/rathena/rathena/blob/master/db/pre-re/mob_skill_db.txt)
- [Monster and NPC System - rAthena DeepWiki](https://deepwiki.com/rathena/rathena/6-monster-and-npc-system)
- [Monster Database - rAthena DeepWiki](https://deepwiki.com/rathena/rathena/6.1-monster-database)
- [rAthena MVP Bug Report - Issue #4817](https://github.com/rathena/rathena/issues/4817)
- [rAthena drops.conf](https://github.com/rathena/rathena/blob/master/conf/battle/drops.conf)
- [List of MVPs & MVP Card IDs - rAthena Wiki](https://github.com/rathena/rathena/wiki/List-of-MVPs-&-MVP-Card-IDs)
- [NPC_METAMORPHOSIS infinite mobs - rAthena Issue #1395](https://github.com/rathena/rathena/issues/1395)
- [Monster Damage Log Improvements - rAthena commit](https://github.com/rathena/rathena/commit/2a7c96b8732a17528e88fbc1df3367bd6f30046c)
- [Quagmire vs NPC_AGIUP - rAthena Issue #904](https://github.com/rathena/rathena/issues/904)
- [Understanding mob_skill_db SUMMONSLAVE - rAthena Forum](https://rathena.org/board/topic/130569-understanding-mob_skill_db-summonslave-skill/)
- [MvP Tombstones - WarpPortal Forums](https://forums.warpportal.com/index.php?/topic/173543-mvp-tombstones/)
- [Guide: Official Status Resistance Formulas (Pre-Renewal)](https://forum.ratemyserver.net/guides/guide-official-status-resistance-formulas-(pre-renewal)/)
- [Ragnarok Online - Monster Skills FAQ - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/29608)
