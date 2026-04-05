# PvP System -- Deep Research (Pre-Renewal)

> Comprehensive reference for Ragnarok Online Classic (pre-renewal) Player vs Player combat systems.
> Sources: iRO Wiki Classic, rAthena source code, RateMyServer, Ragnarok Fandom Wiki, community guides, rAthena forums.
> Cross-reference: `RagnaCloneDocs/08_PvP_Guild_WoE.md`, `RagnaCloneDocs/Implementation/11_PvP_WoE.md`

---

## Table of Contents

1. [Overview](#overview)
2. [PvP Map Types](#pvp-map-types)
3. [PvP Mechanics](#pvp-mechanics)
4. [PvP Restrictions](#pvp-restrictions)
5. [Death and Respawn in PvP](#death-and-respawn-in-pvp)
6. [PvP Rankings](#pvp-rankings)
7. [Player Killing (pk_mode)](#player-killing-pk_mode)
8. [Duel System](#duel-system)
9. [PvP-Specific Cards and Items](#pvp-specific-cards-and-items)
10. [PvP Meta and Build Considerations](#pvp-meta-and-build-considerations)
11. [Implementation Checklist](#implementation-checklist)
12. [Gap Analysis](#gap-analysis)

---

## Overview

Ragnarok Online's pre-renewal PvP system provides structured player-versus-player combat in dedicated arena maps. Unlike modern MMOs with instanced battlegrounds or matchmaking, RO Classic PvP is a persistent free-for-all environment accessed through town NPCs.

**Key characteristics of RO Classic PvP:**
- **Opt-in via NPC**: Players enter PvP arenas through Inn NPCs in major cities
- **Free-for-all**: All players in a PvP map can attack each other (no teams, no matchmaking)
- **Points-based ejection**: A scoring system ejects players who die too many times
- **No PvP-specific damage reduction**: Standard damage formulas apply (unlike WoE which has explicit reductions)
- **Ranking with visual feedback**: Top players receive glowing aura effects
- **Safe mode (Yoyo)**: The primary mode has zero death penalties
- **Separate from WoE**: PvP arenas are independent of the guild-based War of Emperium system

**PvP is NOT the same as WoE.** WoE has its own damage reductions (-40% skill damage, -25% long-range), disabled skills, and no-knockback rules. PvP arenas are a simpler free-for-all with almost no restrictions beyond Fly Wing/Teleport being disabled.

---

## PvP Map Types

### PvP Rooms (Yoyo Mode -- Free-For-All)

Yoyo Mode is the standard, "safe" PvP mode. It is the only active PvP mode on official iRO servers (Nightmare Mode was disabled due to unpopularity).

**Access:**
- Available in most major cities: **Prontera, Izlude, Alberta, Payon, Morroc**
- Located inside **Inn buildings** containing a PvP NPC (typically called "PvP Arena Guide" or similar)
- **Entry fee**: 500 Zeny
- **Level requirement**: Base Level 31 minimum
- One NPC in the waiting room (northeast) serves as the gateway to multiple PvP maps

**Map naming convention (rAthena):**
| Map Name Pattern | Description |
|-----------------|-------------|
| `pvp_y_room` | Yoyo Mode waiting room |
| `pvp_y_1-1` through `pvp_y_2-5` | Yoyo Mode arena maps (multiple sizes) |
| `pvp_n_room` | Nightmare Mode waiting room |
| `pvp_n_1-1` through `pvp_n_8-5` | Nightmare Mode arena maps |

**Room selection:**
- Players talk to the NPC and select from a list of available maps
- Maps vary in size (small, medium, large variants)
- No level-specific room restrictions in modern Classic -- all levels 31+ share the same maps
- Some older implementations offered level-tiered rooms, but these were consolidated
- Upon selection, the player is teleported to a **random spawn point** on the chosen map

**Yoyo Mode rules:**
| Property | Value |
|----------|-------|
| Death EXP loss | **None** |
| Item drop on death | **None** |
| Starting points | 5 |
| Points per kill | +1 |
| Points per death | -5 |
| Kicked at | Points <= 0 |
| Fly Wing | **Disabled** |
| Teleport skill | **Disabled** |
| Butterfly Wing | **Allowed** (exit method) |
| Resurrection by Priest | **Allowed** (if victim has points remaining) |
| Spawn invulnerability | 5 seconds (canceled on movement, attack, or skill use) |

**Exit methods:**
1. Relog (disconnect and reconnect)
2. Butterfly Wing (warps to save point)
3. Another player's Warp Portal skill
4. Death when points reach 0 (auto-kicked to save point)
5. Being resurrected does NOT exit -- player stays on map if points > 0

### Nightmare Mode

Nightmare Mode was the "hardcore" PvP mode with real penalties. It was disabled on iRO and most private servers due to extreme unpopularity.

**Nightmare Mode rules:**
| Property | Value |
|----------|-------|
| Death EXP loss | **Yes** (standard PvE death penalty applies) |
| Item drop on death | **1% chance per equipped/inventory item** |
| Starting points | 5 |
| Points per kill | +1 |
| Points per death | -5 |
| Kicked at | Points <= 0 |
| Other rules | Same as Yoyo Mode |

**rAthena mapflag configuration:**
```
// Nightmare mode drops
map_name	mapflag	pvp_nightmaredrop	id,type,percent
// id: item ID or 0 for random
// type: 1=inventory, 2=equip, 3=both
// percent: drop chance per item (100 = 1%)
```

**Why it was disabled:** Players could grief by entering with valuable gear and losing it, or high-level players could farm lower-level players for their drops. The risk-reward ratio was too punishing for casual players, and the mode became a ghost town.

### GvG Maps

Guild-vs-Guild maps are separate from PvP arenas and follow WoE rules. They are NOT part of the PvP arena system but are sometimes confused with it.

**Key differences from PvP arenas:**
| Property | PvP Arena | GvG/WoE Map |
|----------|-----------|-------------|
| Damage reduction | None | -40% skill damage, -25% long-range |
| Knockback | Enabled | **Disabled** |
| Flee | Normal | -20% |
| Trap duration | Normal | 4x duration |
| Party/guild alliance | Party/guild members cannot attack each other | Guild alliance rules apply |
| Skill restrictions | Fly Wing/Teleport only | Extensive list (Teleport, Warp Portal, Ice Wall, Basilica, Assumptio, etc.) |
| Death penalty | None (Yoyo) | None |
| Scheduling | 24/7 access | Scheduled time windows only |

**GvG map flags (rAthena):**
```
map_name	mapflag	gvg
map_name	mapflag	gvg_noparty      // Party members can attack each other
```

### pk_mode Maps

pk_mode is a **server-wide configuration** (not a per-map setting) that turns the entire server into open-world PvP outside of towns. This is NOT part of the standard Classic PvP system -- it is a server administrator option for "PK servers."

**pk_mode configuration (rAthena `conf/battle/misc.conf`):**
```
// PK Server Mode
// 0 = disabled (default)
// 1 = enabled (PvP on all non-town maps)
// 2 = enabled + manner penalty on kill
pk_mode: 0
```

**pk_mode = 1 behavior:**
- All non-town maps become PvP-enabled
- Novices (Job Class 0) cannot be attacked and cannot attack
- No special entry requirements
- No point system
- Standard death penalties apply (EXP loss)
- Towns remain safe zones (via `nopvp` mapflag)

**pk_mode = 2 behavior:**
- Same as pk_mode 1, plus:
- Killing another player inflicts a **manner penalty of -5** on the killer
- Accumulating negative manner points triggers restrictions based on `manner_system` config:
  - Bit 1 (value 1): Disable chatting (whispers, party/guild messages)
  - Bit 2 (value 2): Disable skill usage
  - Bit 3 (value 4): Disable commands usage
  - Bit 4 (value 8): Disable item usage/picking/dropping
  - Bit 5 (value 16): Disable room creation (chatrooms, vending shops)

**Important:** pk_mode is NOT canon Classic RO behavior. It is a private server feature. Official iRO never had server-wide PK mode. For Sabri_MMO, pk_mode should be treated as an optional admin configuration, not a core feature.

---

## PvP Mechanics

### Damage Calculation Differences

**Critical finding: PvP arenas (Yoyo Mode) have NO special damage reduction formula.**

Unlike WoE, which applies explicit damage reductions, PvP arena combat uses the exact same damage formulas as PvE. The only mechanical difference is that the target is another player instead of a monster.

**What changes in PvP damage calculation:**

| Factor | PvE Value | PvP Value | Notes |
|--------|-----------|-----------|-------|
| Target race | Monster's race | **Demi-Human** (all players) | Hydra Card (+20% vs DH) works |
| Target element | Monster's element | **Neutral 1** (default player element) | Unless changed by armor card/endow |
| Target size | Monster's size | **Medium** (all players) | Desert Wolf Card etc. |
| DEF/MDEF | Monster stats | Player's gear + stat DEF | Full equipment DEF applies |
| Flee | Monster AGI | Player AGI-based Flee | Full Flee calculation |
| Critical | Works normally | **Works normally** | Unlike WoE where crits are less relevant |
| Card effects | vs monster race/ele | vs Demi-Human race | Thara Frog -30%, Hydra +20% |
| Skill damage | Normal | Normal | No reduction in PvP arenas |

**Player race in pre-renewal:** All player characters are classified as **Demi-Human** race. This is the single most important factor in PvP itemization -- cards that modify Demi-Human damage (Hydra, Skeleton Worker for offense; Thara Frog for defense) are the foundation of all PvP builds.

**Player element:** Players default to **Neutral Level 1** element. This can be changed by:
- Armor cards: Dokebi (Wind 1), Swordfish (Water 1), Pasana (Fire 1), Sandman (Earth 1), Bathory (Dark 1), Angeling (Holy 1), Ghostring (Ghost 1), Evil Druid (Undead 1)
- Endow skills: Aspersio (Holy), Frost Weapon (Water), Flame Launcher (Fire), Lightning Loader (Wind), Sage endows

**Player size:** All players are **Medium** size. Size modifiers from weapon types apply:
- Daggers: 100% vs Medium (no penalty)
- Swords: 100% vs Medium
- Spears: 75% vs Medium (unless mounted -- Peco/Grand Peco removes size penalty)
- Axes: 75% vs Medium
- Maces: 100% vs Medium
- Bows: 100% vs Medium

### Skill Interactions in PvP

**Almost all skills work normally in PvP arenas.** The only disabled interactions are:
- **Teleport** (skill ID 26): Cannot be used in PvP maps
- **Fly Wing** (item): Cannot be used in PvP maps
- **Intimidate/Snatch** (Rogue skill 1704): The teleport component is disabled; the skill still deals damage but does not warp the target or caster. This also applies to **Wander Man Card** garments that grant Intimidate on attack.

**Skills that work differently due to PvP context:**
| Skill | PvE Behavior | PvP Behavior |
|-------|-------------|-------------|
| Heal | Heals allies | Can heal enemy players (targets all players) |
| Resurrection | Revives dead party members | Can revive any dead player (if they have PvP points) |
| Pneuma | Blocks ranged attacks | Blocks other players' ranged attacks |
| Safety Wall | Blocks melee attacks | Blocks other players' melee attacks |
| Ice Wall | Creates obstacle | Creates obstacle that blocks players too |
| Warp Portal | Creates portal | **Works** -- can be used as an exit method |
| Butterfly Wing | Returns to save point | **Works** -- primary exit method |
| Status effects | Applied normally | Applied normally (Freeze, Stone, Stun, etc.) |
| Knockback | Applied normally | **Applied normally** (unlike WoE where knockback is disabled) |

**Supportive skills on enemies in PvP:**
- Heal targets any player (party member or not) -- cannot be used offensively on Undead-element players (would damage them)
- Blessing/Increase AGI can be cast on any player
- Buffs respect normal targeting rules

### Healing in PvP

**No healing reduction in PvP arenas.** All healing works at full effectiveness:
- Red Potions, White Potions, Mastela Fruit: Full heal amount
- Heal skill: Full heal amount
- Sanctuary: Full heal (ground effect, heals everyone)
- Slim Potions (Alchemist): Full effectiveness
- Recovery items: Full effectiveness

**In WoE (NOT PvP arenas):** Some servers reduce potion effectiveness by 50%, but this is a WoE-specific rule, not a PvP arena rule.

**Healing-related interactions:**
- **Critical Wounds** status effect (from certain skills like Acid Demonstration in Renewal) reduces healing effectiveness by 20% per level. In pre-renewal, this status is not commonly available.
- **Potion Research** (Alchemist passive) bonus applies to all potions used by the Alchemist in PvP
- **Increase HP Recovery** (Acolyte passive) bonus applies in PvP

### Buff/Debuff Interactions

All buffs and debuffs function normally in PvP arenas. There are no PvP-specific modifications to status effects.

**Key PvP-relevant status effects:**
| Status | Duration | Effect in PvP | Counterplay |
|--------|----------|---------------|-------------|
| Freeze | Varies by skill | Target cannot move/act, Water element | Marc Card (immunity), Thawing Potion |
| Stone Curse | 5s + petrify | Target cannot move/act, Earth element | Soft body can be attacked for extra damage |
| Stun | Varies | Target cannot move/act | VIT-based resistance, Orc Hero Card |
| Silence | Varies | Cannot cast skills | Green Potion, Panacea |
| Blind | Varies | Reduced HIT/Flee | Green Potion |
| Poison | Varies | HP drain, reduced DEF | Green Potion, Panacea |
| Curse | Varies | LUK = 0, reduced movement | Holy Water |
| Sleep | Varies | Cannot act, wakes on damage | Alarm Card |
| Bleeding | Varies | HP drain, no natural regen | Vital Flower (Renewal item) |
| Coma | Instant | HP = 1, SP = 1 | No prevention (Coma cards proc) |
| Strip (Divest) | Varies | Removes equipment piece | Full Chemical Protection |

**Dispel in PvP:** Sage's Dispel skill works normally and removes all dispellable buffs. This is extremely powerful in PvP.

**Devotion in PvP:** Crusader's Devotion skill redirects damage from party members to the Crusader. Works in PvP arenas (requires party).

---

## PvP Restrictions

### Disabled/Modified Skills

PvP arenas have **very few** skill restrictions compared to WoE. The complete list of disabled skills in PvP arenas:

| Skill | Reason Disabled |
|-------|----------------|
| **Teleport** (Lv1 and Lv2) | Prevents escaping combat; Fly Wing is the item equivalent and is also blocked |
| **Intimidate/Snatch** (teleport component only) | The teleport effect is disabled; damage still applies. Wander Man Card's Intimidate effect is also blocked. |

**Everything else works.** This includes:
- Ice Wall (creates barriers -- works in PvP)
- Warp Portal (usable -- serves as an exit method for other players)
- Basilica (creates invulnerable zone -- works in PvP)
- Assumptio (2x effective HP -- works in PvP)
- Back Slide (movement skill -- works in PvP)
- All ground effects (Storm Gust, Meteor Storm, Lord of Vermilion, etc.)
- All AoE skills
- All knockback skills (Arrow Repel, Bowling Bash, etc.)

**Contrast with WoE disabled skills (for reference -- these restrictions do NOT apply in PvP arenas):**
- Teleport, Warp Portal
- Ice Wall
- Basilica
- Assumptio
- Plant Cultivation
- Moonlit Water Mill (Sheltering Bliss)
- Intimidate (Snatch) -- full disable
- Hocus Pocus (Abracadabra)
- High Jump

### Item Restrictions

**Almost no item restrictions in PvP arenas:**
- **Fly Wing**: Disabled (cannot be used)
- All other items work normally including:
  - All potions (Red, White, Blue, Mastela Fruit)
  - Yggdrasil Berry and Yggdrasil Leaf
  - Speed Potions
  - All consumable buffs
  - Equipment with any cards

**Butterfly Wing is NOT restricted** -- it is one of the intended exit methods.

### Level Restrictions

| Restriction | Value |
|-------------|-------|
| Minimum Base Level to enter | **31** |
| Maximum Base Level | **None** (no cap) |
| Level-based matchmaking | **None** |

**Important note:** There is no level balancing in PvP arenas. A level 31 player can be matched against a level 99 player. The original system had level-tiered rooms, but these were removed in favor of single mixed-level arenas. Some private servers re-implement level tiers (e.g., 31-60, 61-80, 81-99).

---

## Death and Respawn in PvP

### Death Penalty

| PvP Mode | EXP Loss | Item Drop | Notes |
|----------|----------|-----------|-------|
| **Yoyo Mode** | **None** | **None** | Safe PvP, no consequences beyond point loss |
| **Nightmare Mode** | **Yes** (standard death penalty ~1% base EXP) | **1% chance per item** | Disabled on most servers |
| **WoE** (for reference) | **None** | **None** | Castle combat is penalty-free |
| **pk_mode** | **Yes** (standard death penalty) | Server-configurable | Not standard Classic |

### Point System and Ejection

The PvP point system is the primary consequence of death in Yoyo Mode:

```
Starting points: 5
Kill reward:     +1 point
Death penalty:   -5 points
Ejection:        Automatic when points <= 0
```

**Ejection mechanics:**
- When a player's points reach 0 or below, they are **immediately** warped out of the PvP map
- No chance to be Resurrected -- the warp happens instantly upon death
- Player is warped to their **save point** (last Kafra save location)
- PvP session data (points, kills, deaths) is cleared
- Player must pay the 500 Zeny entry fee again to re-enter

**Resurrection mechanics (points > 0):**
- If a player dies but still has points > 0, they remain as a dead body on the map
- A Priest can cast Resurrection on them
- They can also use a Yggdrasil Leaf on themselves (if they have one -- but they are dead, so another player must use it)
- Upon resurrection, they continue playing with their remaining points
- **No respawn timer** -- resurrection is immediate upon receiving the skill

### Respawn Location

- Dead players with 0 points are warped to their **last save point** (town Kafra save)
- If the player has no save point, they are sent to the **default spawn** (Prontera for most races, Morroc for some)
- There is no "respawn inside the PvP map" mechanic -- once kicked, you must re-enter through the NPC

---

## PvP Rankings

### Per-Map Ranking System

PvP rankings are calculated **per map, per session**. They are not persistent lifetime rankings.

**Ranking calculation:**
- Players are ranked by **points** (descending)
- Ties are broken by **kill count** (descending)
- Rankings update in real-time whenever a kill occurs

**Ranking display:**
- The player's current rank is shown in the **lower-right corner** of the screen
- Format: `Rank X / Y players` (where X is the player's rank and Y is total players on the map)
- This display updates dynamically

### Visual Ranking Effects (Top 10)

Players ranked in the **top 10** on a PvP map receive a visual effect:

| Rank | Visual Effect |
|------|---------------|
| Ranks 2-10 | Glowing graphic effect **under the character's feet** |
| Rank 1 | **Brightest white** glow effect |
| Approaching Rank 1 | Glow progressively becomes **whiter/brighter** |
| Outside top 10 | No visual effect |

**The glow is visible to all players** on the map, making top-ranked players identifiable targets.

### TIME ATTACK!! System

When a player reaches **Rank 1**, a special system activates:

1. The message **"TIME ATTACK!!"** appears on the Rank 1 player's screen only
2. A timer begins recording how long the player maintains Rank 1
3. This time data is saved server-side
4. Originally, this data was posted on the **official website** as a leaderboard
5. The timer resets if the player loses Rank 1 and regains it

**For Sabri_MMO implementation:** The TIME ATTACK system can be adapted as a persistent "longest Rank 1 streak" stat saved to the `pvp_stats` database table.

### Kill/Death Tracking

**Per-session (in-memory):**
- Points, kills, deaths, current streak tracked per player
- Data cleared when leaving PvP map

**Persistent (database):**
- Lifetime kills, deaths, and highest streak can be persisted to `pvp_stats` table
- Not an original iRO Classic feature (the official server only tracked TIME ATTACK)
- Recommended for modern implementation as players expect lifetime stats

---

## Player Killing (pk_mode)

### Overview

pk_mode is a **server configuration option** (not a standard Classic feature) that enables open-world PvP across all non-town maps. It was never active on official iRO servers.

### How pk_mode Works

**Server configuration (`conf/battle/misc.conf`):**
```
pk_mode: 0    // 0=disabled, 1=enabled, 2=enabled+manner penalty
```

**When pk_mode = 1:**
- All maps without the `nopvp` mapflag become PvP-enabled
- Towns and cities are protected via `nopvp` mapflag
- Novices (base class, Job ID 0) are immune -- cannot attack or be attacked
- Rebirth Novices (High Novice) ARE vulnerable
- No point system, no ranking, no special PvP UI
- Standard death penalties apply (EXP loss, no item drop unless `causedrop` mapflag is set)
- No spawn invulnerability
- Party/guild alliance rules apply (cannot attack allies)

**When pk_mode = 2:**
- Everything from pk_mode 1, plus:
- Each player kill inflicts a **manner penalty of -5** on the killer
- Manner penalties are cumulative
- When manner drops below 0, the `manner_system` config determines restrictions

### Karma System

RO has a vestigial **karma** system in the code, but it was **never fully implemented or functional** in official servers.

**rAthena karma facts:**
- Karma is stored as a character variable but has no gameplay effect in pre-renewal
- No official commands or NPCs interact with the karma value
- Some private servers implement custom karma systems via scripting
- The `manner` system (described above) is the closest thing to a karma penalty for PK

**For Sabri_MMO:** If implementing pk_mode, use the manner penalty system (pk_mode = 2) as the basis. The karma system should be considered a deferred/optional feature with no official specification to reference.

---

## Duel System

The duel system is an **@command-based feature** (not part of the original Classic PvP design). It enables consensual 1v1 (or up to 3-player) PvP anywhere.

### Duel Commands

| Command | Effect |
|---------|--------|
| `@duel <PlayerName>` | Challenge a player to a duel |
| `@accept` | Accept a pending duel challenge |
| `@reject` | Decline a duel challenge |
| `@invite <PlayerName>` | Add a player to an existing duel (max 3 participants) |
| `@leave` | Leave the current duel |

### Duel Rules

| Property | Value |
|----------|-------|
| Maximum participants | **3** |
| Location restriction | **None** (works anywhere except WoE/PvP/BG maps) |
| Same-map requirement | **Yes** (challenger and target must be on same map) |
| Death penalty | **None** |
| Item drop | **None** |
| Point tracking | **None** |
| End conditions | Death, leaving the map, `@leave` command |
| Duel while in PvP | **Not allowed** (redundant) |
| Duel during WoE | **Not allowed** |

### Implementation Notes

- The duel system is a private server / @command feature, not present in vanilla Classic RO
- Many private servers implement it as a convenience feature
- For Sabri_MMO, implement as a socket event (`pvp:duel_request`, `pvp:duel_accept`, etc.) rather than @commands
- Consider adding a chat command equivalent (`/duel`, `/accept`, `/reject`)

---

## PvP-Specific Cards and Items

### Offensive Cards (Weapon)

Cards that increase damage against other players (Demi-Human race):

| Card | Slot | Bonus | Notes |
|------|------|-------|-------|
| **Hydra Card** | Weapon | +20% damage vs Demi-Human | Core PvP offensive card |
| **Skeleton Worker Card** | Weapon | +15% damage vs Medium size | Stacks with Hydra |
| **Turtle General Card** | Weapon | +20% damage to all targets | More expensive alternative |
| **Phreeoni Card** | Weapon | +100 HIT | For ASPD builds needing HIT |
| **Incantation Samurai Card** | Weapon | +50% critical damage, -1 SP per attack | For critical builds |
| **Abysmal Knight Card** | Weapon | +25% damage vs Boss monsters | NOT for PvP (players are not Boss) |

**Optimal PvP weapon carding (pre-renewal):**
- **3x Hydra + 1x Skeleton Worker** in a 4-slot weapon = +60% race + 15% size = best average DPS vs players
- **4x Hydra** = +80% race bonus but no size bonus
- **2x Hydra + 2x Skeleton Worker** = +40% race + 30% size = slightly different breakpoint

### Defensive Cards (Shield)

| Card | Slot | Bonus | Notes |
|------|------|-------|-------|
| **Thara Frog Card** | Shield | -30% damage from Demi-Human | **THE** core PvP defensive card |
| **Horn Card** | Shield | -35% damage from Long Range attacks | Alternative vs Hunters/ranged |
| **Khalitzburg Card** | Shield | -30% damage from Demon race + Undead element | Niche vs Evil Druid users |

**"Cranial" prefix:** A shield with Thara Frog Card is called a "Cranial" shield. This is the most important single piece of PvP equipment.

### Status Immunity Cards (Armor/Accessory)

| Card | Slot | Immunity | Notes |
|------|------|----------|-------|
| **Marc Card** | Armor | Freeze immunity | **Essential** for PvP (Storm Gust, Frost Driver) |
| **Evil Druid Card** | Armor | Freeze + Stone immunity, Undead element | Changes player to Undead 1 -- Heal damages, Resurrection kills |
| **Orc Hero Card** | Headgear | Stun immunity | Extremely rare, highly valued |
| **Marduk Card** | Headgear | Silence immunity | Important vs Lex Divina, Frost Joker |
| **Nightmare Card** | Headgear | Sleep immunity | vs Lullaby, some monster skills |
| **Orc Lord Card** | Armor | Reflect 30% melee damage | Powerful in PvP melee |
| **Ghostring Card** | Armor | Ghost element 1 | Reduces Neutral damage by 25% but weak to other elements |
| **Angeling Card** | Armor | Holy element 1 | Immune to Holy damage (Heal, Grand Cross) but weak to Dark |

### PvP-Relevant Equipment

| Equipment | Effect | PvP Relevance |
|-----------|--------|---------------|
| **Poo Poo Hat** | -10% damage from Demi-Human | Stacks with Thara Frog |
| **Feather Beret** | -10% damage from Demi-Human | Same effect as Poo Poo Hat |
| **Valkyrja's Shield** | +20% Fire/Water/Shadow/Undead resist | Best shield base for PvP |
| **Immune Manteau** (Raydric) | -20% Neutral damage | Pairs with Ghostring armor |
| **Variant Shoes** | +20% MaxHP, -20% MaxSP if no VIT | HP pool increase for survivability |
| **Tidal Shoes + Falcon Muffler** | Combo: MaxHP +10%, MaxSP +10% | Common PvP footgear combo |
| **Combat Knife** | +10% resist Demi-Human, ignore DEF of Demi-Human race | PvP dagger option |
| **Main Gauche [4]** | 4 card slots | Platform for 4x Hydra or 3H+1SW |
| **Sword Breaker / Mail Breaker** | Chance to break weapon/armor | PvP disruption tools |

### Yggdrasil Items in PvP

| Item | Effect | PvP Notes |
|------|--------|-----------|
| **Yggdrasil Berry** | Full HP + SP restore | **Usable in PvP arenas** -- very powerful |
| **Yggdrasil Leaf** | Resurrects dead player | Can revive dead players in PvP |
| **Yggdrasil Seed** | Heals 50% HP/SP | Commonly used |

These items are **NOT restricted** in PvP arenas (unlike some private servers that ban them). Their availability significantly affects PvP balance -- players with large Yggdrasil Berry supplies have a major advantage.

---

## PvP Meta and Build Considerations

### Common PvP Class Archetypes (Pre-Renewal)

| Role | Classes | Strategy |
|------|---------|----------|
| **Burst damage** | Assassin Cross (EDP+SB), Champion (Asura Strike) | One-shot kills through high burst |
| **AoE control** | Wizard (Storm Gust), High Wizard (AoE lockdown) | Freeze/stun groups, area denial |
| **Support** | High Priest (heal, buffs, Resurrection) | Keep party alive, Lex Aeterna debuff |
| **Tank/disrupt** | Paladin (Devotion, Gospel/Battle Chant) | Absorb damage, random powerful buffs |
| **Strip/disable** | Stalker (Divest skills, Backstab) | Remove enemy equipment |
| **Ranged DPS** | Sniper (Double Strafe, Sharp Shooting) | Kiting, long-range damage |
| **Utility** | Professor (Dispel, Soul Burn, Land Protector) | Counter-magic, SP drain |

### Key PvP Interactions

**Ghostring + Raydric combo:** Player becomes Ghost element (Neutral attacks -25%) with Raydric manteau (-20% Neutral). Total Neutral damage reduction: ~40%. But vulnerable to Holy, Dark, and other elements.

**Marc vs Evil Druid:** Marc gives freeze immunity while staying Neutral element. Evil Druid gives freeze AND stone immunity but makes the player Undead (Heal damages, Resurrection kills, weak to Holy and Fire). Evil Druid is higher risk/higher reward.

**Strip builds:** Rogues/Stalkers with Divest skills can remove a player's weapon, shield, helm, or armor for 60+ seconds. Full Chemical Protection (Alchemist) prevents this. Strip removes card bonuses from the stripped piece.

---

## Implementation Checklist

### Server-Side Requirements

- [ ] **Zone flags**: Add `pvp`, `pvpNightmare`, `pvp_noparty`, `pvp_noguild`, `pvp_nocalcrank` flags to zone registry
- [ ] **PvP damage gate**: `isPvPAllowed()` function checking zone flags before allowing player-vs-player damage
- [ ] **PvP session state**: In-memory `Map<characterId, { points, kills, deaths, streak, zone, spawnInvulnerableUntil }>`
- [ ] **Point system**: +5 on enter, +1 on kill, -5 on death, kick at <= 0
- [ ] **Spawn invulnerability**: 5-second timer, canceled on movement/attack/skill
- [ ] **PvP kill handler**: `onPvPKill()` -- update points, broadcast kill event, check ejection, update rankings
- [ ] **PvP rankings**: Per-zone sorted ranking list, broadcast on every kill
- [ ] **PvP ejection**: Warp player to save point when points <= 0
- [ ] **Entry validation**: Level >= 31, 500 Zeny fee, not already in PvP, room not full
- [ ] **Room capacity**: `maxPlayers` per PvP zone (default 40)
- [ ] **Fly Wing/Teleport block**: Check `pvp` flag in Fly Wing handler and Teleport skill handler
- [ ] **Butterfly Wing allow**: Ensure Butterfly Wing still works in PvP zones
- [ ] **Player race**: Set all players to `race: 'demihuman'` for card effect calculations
- [ ] **Player element**: Track `playerElement` (default Neutral 1), modify via armor cards and endow skills
- [ ] **Player size**: Set all players to `size: 'medium'`
- [ ] **Persistent stats**: `pvp_stats` table (lifetime kills, deaths, highest_streak, last_pvp_at)
- [ ] **Duel system**: Challenge, accept, reject, leave, end-on-death, max 3 participants
- [ ] **PvP entry NPC**: NPC handler for `pvp:enter_room` socket event

### Client-Side Requirements

- [ ] **PvP rank display**: HUD element showing "Rank X / Y" in lower-right corner
- [ ] **PvP point display**: Current points shown somewhere on screen
- [ ] **Kill/death feed**: Chat messages for kills and deaths in PvP
- [ ] **Top 10 aura effect**: Niagara VFX under character feet, progressively whiter for higher ranks
- [ ] **TIME ATTACK message**: Full-screen flash text when reaching Rank 1
- [ ] **PvP entry popup**: NPC interaction dialog showing entry fee, level requirement, room selection
- [ ] **Spawn invulnerability indicator**: Visual feedback during 5s invulnerability
- [ ] **Duel UI**: Challenge popup, accept/reject buttons, duel status indicator
- [ ] **PvP zone indicator**: HUD element showing "PvP Zone" when in a PvP map

### Database Requirements

- [ ] **pvp_stats table**: `character_id (UNIQUE), kills, deaths, highest_streak, time_attack_best, last_pvp_at`
- [ ] **Zone registry updates**: PvP arena zones added to `ro_zone_data.js`

### Socket Events

```
// Client -> Server
pvp:enter_room        { roomId }
pvp:leave             {}
pvp:duel_request      { targetCharId }
pvp:duel_accept       { requestId }
pvp:duel_reject       { requestId }
pvp:duel_leave        {}

// Server -> Client
pvp:entered           { roomId, points, rankings[] }
pvp:kill              { killerId, killerName, victimId, victimName, killerPoints, victimPoints }
pvp:death             { killerId, newPoints }
pvp:kicked            { reason }
pvp:rankings_update   { rankings[] }
pvp:duel_request_received { requestId, challengerName }
pvp:duel_started      { opponentId, opponentName }
pvp:duel_ended        { winnerId, reason }
pvp:error             { message }
pvp:invulnerable_end  {}
pvp:time_attack       { active: boolean, elapsed: number }
```

---

## Gap Analysis

### What Our Existing Documentation Covers Well
- PvP arena access and entry (08_PvP_Guild_WoE.md Section 3)
- Point system basics (08_PvP_Guild_WoE.md Section 3.4)
- Duel system commands (08_PvP_Guild_WoE.md Section 3.6)
- Server-side implementation code stubs (Implementation/11_PvP_WoE.md Sections 1.1-1.7)
- Socket event definitions (Implementation/11_PvP_WoE.md Section 1.7)
- Database schema for pvp_stats (Implementation/11_PvP_WoE.md Section 7)

### Gaps Filled by This Deep Research

| Gap | Section in This Document | Priority |
|-----|-------------------------|----------|
| **No PvP damage reduction in arenas** (clarified -- existing docs were ambiguous) | [Damage Calculation Differences](#damage-calculation-differences) | Critical |
| **Player race/element/size for PvP calculations** | [Damage Calculation Differences](#damage-calculation-differences) | Critical |
| **Complete list of disabled/allowed skills** | [Disabled/Modified Skills](#disabledmodified-skills) | High |
| **PvP vs WoE rule differences** (explicit comparison) | [GvG Maps](#gvg-maps) | High |
| **Ranking visual effects (top 10 aura, progressive whitening)** | [Visual Ranking Effects](#visual-ranking-effects-top-10) | Medium |
| **TIME ATTACK!! system** | [TIME ATTACK System](#time-attack-system) | Medium |
| **pk_mode mechanics and manner penalties** | [Player Killing](#player-killing-pk_mode) | Low (optional feature) |
| **Karma system status** (not implemented in Classic) | [Karma System](#karma-system) | Low |
| **PvP card/item meta** | [PvP-Specific Cards and Items](#pvp-specific-cards-and-items) | Medium |
| **Nightmare Mode complete rules** | [Nightmare Mode](#nightmare-mode) | Low (disabled feature) |
| **Resurrection in PvP** (allowed if points > 0) | [Point System and Ejection](#point-system-and-ejection) | Medium |
| **Yggdrasil items usable in PvP** | [Yggdrasil Items in PvP](#yggdrasil-items-in-pvp) | Medium |
| **Spawn invulnerability cancellation rules** | [PvP Rooms](#pvp-rooms-yoyo-mode----free-for-all) | High |
| **Map naming conventions** (pvp_y_*, pvp_n_*) | [PvP Rooms](#pvp-rooms-yoyo-mode----free-for-all) | Low |

### Remaining Unknowns / Needs Verification

1. **Exact Nightmare Mode EXP penalty**: Is it the standard 1% base EXP loss, or a different rate? Most sources say "standard death penalty" but some private servers modified this.
2. **Warp Portal behavior in PvP**: Can any player enter a Warp Portal placed in a PvP arena, or only party members? Standard behavior is all players can enter.
3. **PvP room historical level tiers**: The original iRO had level-tiered rooms that were later consolidated. If implementing level tiers, what were the original brackets? Research suggests 31-60, 61-80, 81-99 but this varies by source.
4. **Official TIME ATTACK leaderboard format**: The exact data posted to the official website is not well-documented. For implementation, tracking best time-at-rank-1 in the database is sufficient.
5. **Invulnerability interaction with skills cast on self**: Does casting a self-buff (like Increase AGI) during invulnerability cancel it? Most sources say only movement, attacking, and offensive skills cancel it.

---

## Sources

- [PvP - iRO Wiki Classic](https://irowiki.org/classic/PvP)
- [PvP - iRO Wiki](https://irowiki.org/wiki/PvP)
- [PvP Overview - iRO Wiki](https://irowiki.org/wiki/PvP_Overview)
- [Player Versus Player - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Player_Versus_Player)
- [Mapflag - rAthena Wiki (GitHub)](https://github.com/rathena/rathena/wiki/Mapflag)
- [rAthena mapflags.txt](https://github.com/rathena/rathena/blob/master/doc/mapflags.txt)
- [rAthena nightmare.txt](https://github.com/rathena/rathena/blob/master/npc/mapflag/nightmare.txt)
- [rAthena pvp.txt (NPC scripts)](https://github.com/rathena/rathena/blob/master/npc/other/pvp.txt)
- [rAthena nopvp.txt](https://github.com/rathena/rathena/blob/master/npc/mapflag/nopvp.txt)
- [rAthena battle.conf (player settings)](https://github.com/rathena/rathena/blob/master/conf/battle/player.conf)
- [rAthena battle.cpp (damage calc source)](https://github.com/rathena/rathena/blob/master/src/map/battle.cpp)
- [PK Servers - iRO Wiki](https://irowiki.org/wiki/PK_Servers)
- [Ragnarok's Karma System - rAthena Forums](https://rathena.org/board/topic/62278-ragnaroks-karma-system/)
- [Ragnarok Online PVP System (Blog)](https://playragnarokonline.blogspot.com/2013/04/pvp-system-player-versus-player.html)
- [Ragnarok Online Official PVP Guide](https://renewal.playragnarok.com/gameguide/features_pvp.aspx)
- [Thara Frog Card - RateMyServer (Pre-Re)](https://ratemyserver.net/index.php?page=item_db&item_id=4058)
- [Demi Human - iRO Wiki](https://irowiki.org/wiki/Demi_Human)
- [War of Emperium - iRO Wiki](https://irowiki.org/wiki/War_of_Emperium)
- [PvP - Origins Online Wiki](https://wiki.originsro.org/wiki/PvP)
- [PvP - Ragnagoats Wiki](https://ragnagoats.net/wiki/index.php?title=PvP)
