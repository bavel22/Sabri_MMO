# 08 -- PvP, Guild, War of Emperium & Party Systems

> Ragnarok Online Classic reference document for Sabri_MMO implementation.
> Covers Party, Guild, PvP, War of Emperium (WoE 1 & 2), Battlegrounds, and implementation schemas.

---

## Table of Contents

1. [Party System](#1-party-system)
2. [Guild System](#2-guild-system)
3. [PvP System](#3-pvp-system)
4. [War of Emperium (WoE 1)](#4-war-of-emperium-woe-1)
5. [War of Emperium Second Edition (WoE 2 / SE)](#5-war-of-emperium-second-edition-woe-2--se)
6. [Battlegrounds](#6-battlegrounds)
7. [God Items (Divine Equipment)](#7-god-items-divine-equipment)
8. [Implementation Plan](#8-implementation-plan)

---

## 1. Party System

### 1.1 Creation and Basics

- **Command**: `/organize "Party Name"` creates a party. The creator becomes Party Leader.
- **Requirement**: Novice Basic Skill Level 7 to create a party.
- **Maximum members**: 12 (leader + 11 members).
- **Invite command**: `/invite "CharacterName"` -- can invite remotely (no map proximity needed).
- **Party window**: Alt+Z opens the party window showing member names, HP bars, and map location.

### 1.2 Experience Distribution

Two modes, changeable at any time by the party leader (provided all online members meet the level restriction):

| Mode | Behavior |
|------|----------|
| **Each Take** | Members only gain EXP for monsters they personally dealt damage to. Standard solo-play behavior while in a party. |
| **Even Share** | All EXP from kills by any party member is pooled and divided equally among eligible members on the same map. |

**Even Share Formula**:
```
Per-member EXP = (Monster Base EXP / Number of eligible members on same map)
```

**Party EXP Bonus** (Even Share only):
- Each additional party member on the same map adds a bonus to the total EXP before division.
- Classic formula: approximately +25% bonus EXP per additional member who either dealt damage to the monster or received damage from it.
- The bonus incentivizes group play -- a full party of 12 killing efficiently earns significantly more total EXP than 12 solo players.

**Level Restriction for Even Share**:
- All online party members must be within **15 base levels** of each other to use Even Share. (Some servers use 10 levels; the original iRO Classic specification uses 15.)
- If any online member falls outside this range, Even Share cannot be activated -- the system forces Each Take.
- Members can be any level to *join* the party; the restriction only applies to EXP sharing.

**Same-Map Requirement**:
- Only members on the same map as the monster kill receive shared EXP.
- Members on different maps receive nothing from that kill (even in Even Share mode).

### 1.3 Item Distribution

Set at party creation time. **Cannot be changed** without disbanding and re-creating the party.

| Mode | Behavior |
|------|----------|
| **Each Take** | The player who killed the monster has first-pick priority on dropped items. Other members cannot pick up for a brief window. |
| **Party Share** | Any party member can immediately pick up dropped items with no priority. |
| **Individual** | Whoever picks up the item keeps it -- no sharing logic. |
| **Shared** | Picked-up items are randomly assigned to one party member. |

### 1.4 Party Leader Mechanics

- **Leader Transfer**: Right-click a party member's name in the party window and select "Delegate Leader" to transfer leadership.
- **Automatic Transfer**: If the leader leaves the map and party members remain, leadership automatically delegates to a remaining member.
- **Kick**: Leader can right-click a member name and select "Kick from Party" to remove them.
- **Disband**: If the leader disconnects and no other members are online, the party is automatically disbanded.
- **Leader-only actions**: Changing EXP mode, kicking members, inviting new members.

### 1.5 Party HP Display

- The party window (Alt+Z) shows real-time HP/SP bars for all party members.
- Members on the same map see each other's HP bars above their character sprites.
- HP bar color changes based on health percentage (green > yellow > red).
- Dead members show a skull icon or greyed-out entry.

### 1.6 Party Chat

- `/p <message>` or pressing Enter in party chat mode sends messages to all party members regardless of map.
- Party chat appears in a distinct color (typically green) in the chat window.

---

## 2. Guild System

### 2.1 Guild Creation

- **Requirement**: One Emperium item (consumed on creation; not returned if guild is disbanded).
- **Command**: `/guild "Guild Name"` while possessing an Emperium.
- **Result**: The creating character becomes the **Guild Master** permanently (no built-in transfer in Classic RO; some later patches added `/breakguild` + reform as the only transfer method).
- **Guild name**: Cannot be changed after creation.

### 2.2 Guild Levels (1-50)

- Guilds gain levels through **base experience donations** from members via the tax system.
- Each guild level grants **1 skill point** to spend on guild skills (50 total skill points at max level).
- Higher guild levels unlock more powerful guild skills and increase guild prestige.

**Experience Required Per Level**: Scales exponentially. Early levels require minimal EXP; level 50 requires enormous cumulative donations from all members.

### 2.3 Guild Capacity

| Base Capacity | With Guild Extension (10 levels) |
|---------------|----------------------------------|
| 16 members (including Guild Master) | +6 members per skill level, up to **76 total** at Guild Extension Lv10 |

Note: Some servers cap at 56 total (Extension adds 4 per level). The rAthena/Hercules default is +6 per level for 76 max.

### 2.4 Positions and Ranks

- **20 title/position slots** available for the Guild Master to configure.
- Each position has configurable permissions:
  - **Invite permission**: Can this rank invite new members?
  - **Kick permission**: Can this rank expel members?
  - **Tax rate**: Per-position EXP tax rate (0-50%).
  - **Storage access**: Can this rank use guild storage?
- Positions are purely organizational -- they do not grant stat bonuses.
- Guild Master can rename positions freely (e.g., "Officer", "Recruit", "Elite", etc.).
- Members are sorted by position priority in the guild window.

### 2.5 Guild Tax System

- Only **Base Experience** is taxed; Job Experience is never affected.
- Tax rate: **0% to 50%**, set per position by the Guild Master.
- Taxed EXP goes directly into the guild's level progress bar.
- Members can see their total contributed EXP in the guild window.

### 2.6 Guild Skills (Complete List)

**Passive Skills** (always active once learned):

| Skill | Max Lv | Prereq | Effect |
|-------|--------|--------|--------|
| **Official Guild Approval** | 1 | -- | Enables the guild to participate in WoE and attack Emperiums. Required for castle conquest. |
| **Kafra Contract** | 1 | Approval Lv1 | Enables Kafra services (save, warp, storage) inside guild-owned castles for free. |
| **Guardian Research** | 1 | Kafra Contract Lv1 | Enables hiring of Guardian NPCs in guild-owned castles during WoE. |
| **Strengthen Guardian** | 1 | Guardian Research Lv1 | Guardians gain bonuses to Max HP, ATK, and ASPD. |
| **Guild Extension** | 10 | Approval Lv1 | Each level adds +6 member capacity (up to +60 at Lv10). |
| **Guild's Glory** | 1 | -- | Permits the guild to use a custom guild emblem. |
| **Great Leadership** | 1 | Glory Lv1 | +5 STR to all guild members within 5x5 cells of the Guild Master (WoE only). |
| **Wounds of Glory** | 1 | Glory Lv1 | +5 VIT to all guild members within 5x5 cells of the Guild Master (WoE only). |
| **Soul of Cold** | 1 | Glory Lv1 | +5 AGI to all guild members within 5x5 cells of the Guild Master (WoE only). |
| **Sharp Hawk Eyes** | 1 | Glory Lv1 | +5 DEX to all guild members within 5x5 cells of the Guild Master (WoE only). |
| **Absolute Develop** | 1 | -- | 50% chance to receive a free bonus economy point during castle Commerce investment. |
| **Guild Storage Expansion** | 5 | -- | Unlocks guild storage: 100 slots per level (100/200/300/400/500). |

**Active Skills** (cast by Guild Master only):

| Skill | Max Lv | SP | Cooldown | Effect |
|-------|--------|----|----------|--------|
| **Battle Orders** | 1 | 1 | 5 min (shared) | All guild members visible on Guild Master's screen receive +5 STR, +5 DEX, +5 INT for 1 minute. |
| **Regeneration** | 3 | 1 | 5 min (shared) | Increases HP/SP recovery for visible members. Lv1: 2x HP. Lv2: 2x HP + 2x SP. Lv3: 3x HP + 3x SP. Duration: 60 seconds. |
| **Restore** | 1 | 1 | 5 min (shared) | Instantly restores 90% of Max HP and 90% of Max SP to all guild members visible on screen. |
| **Emergency Call** | 1 | 1 | 5 min (shared) | Teleports ALL online guild members to the Guild Master's location. Cast time: 5 seconds (uninterruptible, fixed). **WoE only.** |
| **Charge Shout Flag** | 1 | -- | 15 min | Places a "Flag of Assault" on the ground during WoE. |
| **Charge Shout Beating** | 1 | -- | 15 min | Teleports nearby guild members to the Flag of Assault location. |
| **Emergency Move** | 1 | -- | -- | Increases movement speed for the Guild Master and nearby members during WoE (siege mode only). |

**Shared Cooldown**: Emergency Call, Restore, Regeneration, and Battle Orders all share the same 5-minute global cooldown. Using any one prevents all four for the duration.

### 2.7 Guild Alliances and Enemies

- **Alliances**: Up to **3 allied guilds** at once. Allied guild members appear as friendly (green names in WoE). Allied guilds cannot attack each other during WoE.
- **Antagonists/Enemies**: Guilds can formally declare another guild as an enemy. Enemy guild members can attack each other **anywhere** (even outside PvP/WoE maps). Enemy names display in red.
- **Commands**: `/ally` to propose alliance, `/enemy` to declare antagonist.

### 2.8 Guild Storage

- Unlocked via the **Guild Storage Expansion** skill (5 levels, 100 slots per level).
- Maximum 500 storage slots at Lv5.
- Accessible through Kafra NPCs in towns or via the Kafra inside guild-owned castles.
- Access permissions can be set per guild position.
- Items in guild storage are shared among all members with storage permission.
- Zeny cannot be stored in guild storage (items only).

### 2.9 Guild Emblem

- **Format**: 24x24 pixels, BMP format, 256 colors.
- **Transparency**: Hot pink (RGB 255, 0, 255) renders as transparent in-game.
- Guild emblem appears above guild members' heads and on the guild window.
- Only the Guild Master can change the emblem.
- Stored client-side in an `Emblem/` folder.

### 2.10 Guild Dissolution

- The Guild Master must **kick all members** first.
- Command: `/breakguild "Guild Name"` dissolves the guild.
- The Emperium used to create the guild is **not** returned.
- All castle ownership is forfeited.
- Guild storage items are lost if not withdrawn before dissolution.
- Guild name becomes available for reuse after dissolution.

### 2.11 Guild Master Transfer (Classic Limitations)

- **Classic RO**: No built-in transfer mechanism. The only way is to disband the guild entirely and recreate it with a new master.
- **Later patches**: Some servers added a transfer feature with a 24-hour cooldown, blocked during WoE hours.
- **For implementation**: Recommend supporting direct transfer with cooldown restrictions.

---

## 3. PvP System

### 3.1 PvP Arenas and Access

- **Location**: Most major cities have an Inn building containing a PvP NPC.
- **Available cities**: Prontera, Izlude, Alberta, Payon, Morroc.
- **Entry fee**: 500 Zeny to enter the PvP Waiting Room.
- **Level requirement**: Base Level 31 minimum.
- **Exit methods**: Relog, Butterfly Wing, death, or another player's Warp Portal. Teleport and Fly Wings are disabled inside PvP maps.

### 3.2 PvP Modes

| Mode | Status | Death Penalty | Item Drop |
|------|--------|---------------|-----------|
| **Yoyo Mode** | Active (primary mode) | No EXP loss | No item drops |
| **Nightmare Mode** | Disabled on most servers | EXP loss on death | 1% chance to drop each equipped/inventory item |

- **Yoyo Mode** is the standard PvP mode. Safe, competitive, no risk.
- **Nightmare Mode** was unpopular due to harsh penalties and has been disabled on iRO and most private servers.

### 3.3 PvP Room Types

- Multiple maps available per arena, typically varying in size.
- No level-specific room restrictions in modern Classic (all levels 31+ share the same maps).
- Some servers offered small/medium/large room variants for different player counts.

### 3.4 Point System and Rankings

| Event | Points |
|-------|--------|
| Enter PvP map | +5 (starting points) |
| Kill another player | +1 |
| Die | -5 |
| Points reach 0 or below | **Kicked from PvP map immediately** (no resurrection allowed) |

**Ranking Display**:
- Current rank shown in the lower-right corner of the screen: "Rank X / Y players".
- **Top 10 players** on a map receive a glowing graphic effect under their character.
- Effect becomes **whiter/brighter** as rank approaches #1.
- Rankings are per-map, per-session (reset when leaving).

### 3.5 PvP Combat Mechanics

- **Spawn invulnerability**: 5 seconds upon entering a PvP map (canceled if the player moves).
- **Damage reduction**: No special PvP damage reduction in Yoyo Mode beyond normal gear/stat calculations. WoE-style reductions do NOT apply in PvP arenas.
- **All skills and items usable** except Teleport and Fly Wings.
- **No knockback immunity** -- players can be pushed around.
- **Resurrection**: Dead players can be resurrected by Priests as long as they have points remaining.

### 3.6 Duel System

- **Command**: `@duel <PlayerName>` to challenge another player to a 1v1 duel.
- **Accept/Reject**: `@accept` or `@reject` the challenge.
- **Invite**: `@invite <PlayerName>` to add a third participant (max 3 players).
- **Leave**: `@leave` to exit the duel.
- **Mechanics**: Duel enables PvP between participants **anywhere** on any map (not restricted to PvP arenas).
- **End conditions**: Leaving the map or dying ends the duel.
- **No penalties**: No EXP loss, no item drops, no point tracking.
- **Note**: The duel system is an @command feature (GM/server command), not present in all Classic implementations. Some servers implement it as an NPC-driven feature instead.

---

## 4. War of Emperium (WoE 1)

### 4.1 Overview

War of Emperium (WoE) is the flagship guild-versus-guild content in Ragnarok Online. Guilds fight to conquer and defend castles during scheduled time windows. Owning a castle grants:
- A physical guild headquarters
- Access to exclusive Guild Dungeons
- Daily Treasure Boxes with rare loot
- Materials for crafting God Items (the rarest equipment in the game)

### 4.2 Schedule

WoE occurs at fixed weekly times set by the server administrator. Typical schedules:

| Server | Day | Time (Server) | Duration |
|--------|-----|---------------|----------|
| Classic (typical) | Wednesday | 7:00 PM - 9:00 PM | 2 hours |
| Classic (typical) | Saturday | 1:00 PM - 3:00 PM | 2 hours |

- Schedule varies by server. Most servers run WoE twice per week.
- During non-WoE hours, castles are inaccessible to attackers. Defenders can enter freely.
- Castle ownership persists between WoE periods until the next session.

### 4.3 Castle Realms and Maps

Four realms with **5 castles each** = **20 castles total**.

**Valkyrie Realm** (north of Prontera):

| Castle | Map ID | God Item Materials |
|--------|--------|--------------------|
| Kriemhild | prtg_cas01 | Sleipnir components |
| Swanhild | prtg_cas02 | Sleipnir components |
| Fadhgrindh | prtg_cas03 | Sleipnir components |
| Skoegul | prtg_cas04 | Sleipnir components |
| Gondul | prtg_cas05 | Sleipnir components |

**Balder Realm / Greenwood Lake** (west of Payon):

| Castle | Map ID | God Item Materials |
|--------|--------|--------------------|
| Bright Arbor | payg_cas01 | Megingjard components |
| Scarlet Palace | payg_cas02 | Megingjard components |
| Holy Shadow | payg_cas03 | Megingjard components |
| Sacred Altar | payg_cas04 | Megingjard components |
| Bamboo Grove Hill | payg_cas05 | Megingjard components |

**Britoniah Realm** (west/south of Geffen):

| Castle | Map ID | God Item Materials |
|--------|--------|--------------------|
| Repherion | gefg_cas01 | Brisingamen components |
| Eeyorbriggar | gefg_cas02 | Brisingamen components |
| Yesnelph | gefg_cas03 | Brisingamen components |
| Bergel | gefg_cas04 | Brisingamen components |
| Mersetzdeitz | gefg_cas05 | Brisingamen components |

**Luina Realm** (west of Al De Baran):

| Castle | Map ID | God Item Materials |
|--------|--------|--------------------|
| Neuschwanstein | aldeg_cas01 | Mjolnir components |
| Hohenschwangau | aldeg_cas02 | Mjolnir components |
| Nuernberg | aldeg_cas03 | Mjolnir components |
| Wuerzberg | aldeg_cas04 | Mjolnir components |
| Rothenburg | aldeg_cas05 | Mjolnir components |

Each castle has a unique multi-room map layout with defensive choke points leading to the Emperium Room.

### 4.4 Emperium Mechanics

The Emperium is a destructible object placed in the innermost room of each castle.

| Property | Value |
|----------|-------|
| HP | 68,430 (base) + bonus from Defense investment |
| Level | 90 |
| Race | Angel |
| Element | Holy 1 |
| Size | Small |
| DEF | 40 + 80 |
| MDEF | 100 + 90 |
| Flee | 107 |

**Damage Rules**:
- **Only normal (auto) attacks** can damage the Emperium.
- **All active skills miss** the Emperium (exception: Gloria Domini and Gravitational Field in some implementations).
- Holy-element weapons and ammo (e.g., Silver Arrows, Aspersio-buffed weapons) **miss** the Emperium because it is Holy element (Holy vs Holy = 0% damage).
- Ideal Emperium breakers use: high ASPD, Shadow/Undead element weapons, small-size damage bonuses.
- The guild must have **Official Guild Approval** skill to damage Emperiums.

**Breaking the Emperium**:
- The character who deals the **final hit** claims the castle for their guild.
- All non-allied characters are immediately warped out.
- A new Emperium spawns for the conquering guild to defend.
- Breaking can happen multiple times during a single WoE session.

### 4.5 Guardian System

- **Prerequisite**: Guild must have **Guardian Research** skill.
- **Hiring**: The Guild Master speaks to the castle's Guardian NPC to summon guardians.
- **Behavior**: Guardians automatically attack all non-allied characters inside the castle during WoE.
- **Stats**: Guardian HP, ATK, and ASPD are boosted by the **Strengthen Guardian** skill and **Defense investment** level.
- **Destruction**: Guardians can be killed by attackers. They can be re-hired or repaired by the Guild Master during WoE.
- **Types**: Two guardian types per castle -- typically a melee knight and a ranged archer.
- **Maximum**: Each castle supports a limited number of guardians (typically 2-4).

### 4.6 Guild Dungeon Access

- Each realm has **3 guild dungeon floors** accessible via hidden NPCs inside each castle.
- **All castles within a single realm share the same dungeon** -- this means enemy guilds who own different castles in the same realm will encounter each other in the dungeon.
- Guild dungeons contain **exclusive monsters** not found elsewhere, with valuable drops.
- Dungeon access is available to **all guild members** (not just the Guild Master).
- Access is available 24/7, not just during WoE.

### 4.7 Treasure Room

- **Spawn time**: Every day at 12:00 AM server time, Treasure Boxes appear in a special room.
- **Access**: Only the **Guild Master** can enter the Treasure Room via the castle's NPC.
- **Base boxes**: 4 Treasure Boxes per castle per day (minimum).
- **Commerce bonus**: Every 5 Commerce investment points = +1 additional Treasure Box.
- **Box types**: Each castle has 2 types of Treasure Boxes, each containing a unique loot table of equipment, consumables, and God Item materials.
- **God Item materials**: Drop rate from Treasure Boxes is approximately **1/250 (0.4%)** per box. These materials are the ONLY way to obtain God Item components.

### 4.8 Castle Economy (Investment System)

Two investment tracks, managed by the Guild Master via NPCs inside the castle:

**Commerce Development (Economy)**:

| Property | Detail |
|----------|--------|
| Purpose | Increases Treasure Box quantity and quality |
| Effect | +1 Treasure Box per 5 economy points |
| Investment limit | 2 investments per day |
| Cost scaling | Each investment costs **2x** the previous one |
| Bonus | **Absolute Develop** guild skill gives 50% chance of a free bonus economy point |

**Defense Development**:

| Property | Detail |
|----------|--------|
| Purpose | Strengthens castle defenses |
| Effect | +1,000 HP to Guardians and Emperium per defense point |
| Investment limit | 2 investments per day |
| Cost scaling | Each investment costs **2x** the previous one |

- Investments are cumulative and persist as long as the guild holds the castle.
- If the castle is lost, the new owner inherits the current investment levels.
- Investment levels slowly decay if not maintained (server-dependent).

### 4.9 WoE Combat Rules

WoE applies significant combat modifiers to create a distinct meta:

**Damage Reductions**:

| Attack Type | Reduction |
|-------------|-----------|
| Skill-based damage (magic and physical) | **-40%** (some servers use -60% or -80%) |
| Long-range normal attacks | **-25%** |
| Short-range normal attacks | No additional reduction |
| Gloria Domini / Gravitational Field | **No reduction** (exceptions) |

**Stat Modifications**:
- **Flee reduced by 20%** for all players.
- **Trap duration increased 4x** (Hunters/trappers are very strong in WoE).

**Disabled Skills** (cannot be used inside castle maps during WoE):
- Teleport, Warp Portal
- Ice Wall
- Basilica
- Assumptio
- Back Slide (some servers)
- Plant Cultivation
- Moonlit Water Mill (Sheltering Bliss)
- Intimidate (Snatch)
- Hocus Pocus
- High Jump (TK classes, in some implementations)

**Other WoE Rules**:
- **No knockback** effects (skills that normally push targets have no displacement).
- **No experience loss** on death.
- **Damage numbers hidden** -- `/mineffect` is automatically activated.
- **No Teleport/Fly Wing** inside castle maps.
- **Reduced healing item effectiveness** (server-dependent; some servers reduce potion effectiveness by 50% in WoE).
- **Cannot use @commands** during WoE (no @duel, @warp, etc.).

### 4.10 WoE Flow

1. **WoE begins** at the scheduled time. An announcement is broadcast server-wide.
2. **Non-owning guild members** inside castles are **warped out**.
3. **Attacking guilds** enter through the castle entrance portal.
4. **Multi-room progression**: Attackers fight through 3-5 rooms of defenders, guardians, and traps to reach the Emperium Room.
5. **Emperium break**: The final-hitting character's guild takes ownership. All others are warped out.
6. **Defense phase**: The new owner must defend until WoE ends or they lose it.
7. **WoE ends**: The guild holding the castle at the moment WoE time expires keeps it until the next WoE session.
8. **Castle disappears** from accessibility (no new attackers can enter until next WoE).

---

## 5. War of Emperium Second Edition (WoE 2 / SE)

### 5.1 Overview and Key Differences from WoE 1

WoE 2 (also called WoE:SE or Second Edition) was introduced as a later expansion to the WoE system with fundamentally different castle designs and defensive mechanics.

| Feature | WoE 1 | WoE 2 |
|---------|-------|-------|
| Castle design | Multiple small maps connected by portals | Single large contiguous map |
| Defensive structures | Pre-existing (always present) | Must be built/installed by guild leader |
| Guardian Stones | Not present | 2 per castle (primary defense) |
| Barricades | Not present | 3 sets of destructible walls |
| Link Flags | Not present | 12 flags for defender fast-travel |
| Pre-casting | Common strategy (buff at portals) | Eliminated (single map, no portal transitions) |
| Emphasis | Offensive rushing | Defensive strategy |
| Castle realms | 4 (Prontera/Payon/Geffen/Al De Baran) | 2 (Juno/Rachel) |

### 5.2 Castle Locations

**Nidhoggur Realm** (west of Juno / Schwartzwald Republic):
- 5 castles with Lighthalzen/Juno architectural style
- Castle names: Himinn, Andlangr, Viblainn, Hljod, Skidbladnir

**Valfreyja Realm** (south of Rachel / Arunafeltz):
- 5 castles with Arunafeltz regional style
- Castle names: Mardol, Cyr, Horn, Gefn, Syr

### 5.3 Guardian Stones

- **2 Guardian Stones** per castle, serving as the primary defensive barrier.
- While both Guardian Stones are intact, an **indestructible barrier** blocks the castle interior entrance. Attackers physically cannot pass.
- Destroying a Guardian Stone removes part of the barrier and kills the guardians it spawned.
- **Both stones must be destroyed** to fully open the path to the interior and barricades.

**Guardian Stone Revival**:
- Materials required: 30 Stone, 1 Oridecon, 1 Elunium, 5 Blue Gemstones, 5 Red Gemstones, 5 Yellow Gemstones.
- **8-minute cooldown** before a destroyed stone can be repaired.
- Any guild member with permission can repair (not just the Guild Master).

**Guardian Spawning**:
- Guardian Stones automatically spawn powerful guardian NPCs.
- These WoE:SE guardians are **significantly stronger** than WoE 1 guardians (comparable to Thor Volcano dungeon monsters).
- If a guardian dies, its stone will respawn it after a delay -- as long as the stone itself is alive.

### 5.4 Barricade System

- **3 sets of barricades** create impassable defensive lines between the Guardian Stones and the Emperium Room.
- Each barricade set consists of **4-8 individual blocks** -- ALL blocks in a set must be destroyed before players can pass.
- Barricades are **NOT automatically present** when a castle is first captured. The Guild Master must install them.

**Installation Cost** (per barricade set):
- 30 Trunk, 10 Steel, 10 Emvertarcon, 5 Oridecon
- Only the Guild Master can install barricades.

**Barricade HP**: Approximately **450,000 HP per block** -- extremely durable.

**Repair Rules**:
- Barricades must be repaired **sequentially from the Emperium room outward** (innermost first).
- Once destroyed during active WoE, barricades **remain destroyed** for the rest of that WoE session. They can only be reinstalled after WoE ends.

### 5.5 Link Flags

- **12 Link Flags** placed throughout the castle for the **defending guild only**.
- Flags allow instant teleportation between any two flag locations inside the castle.
- External flags (just outside the castle entrance) let defenders warp directly into the interior, bypassing attackers at the gate.
- Flags near the Emperium Room allow rapid reinforcement of the final defense.
- **Attackers cannot use Link Flags** -- they must fight through every room.

### 5.6 Additional WoE 2 Skill Restrictions

All WoE 1 skill restrictions apply, plus:
- **High Jump** (TK Ranker skill) is disabled.
- **Leap** (similar movement skill) is disabled.

### 5.7 WoE 2 Treasure and Economy

- Treasure Boxes work the same as WoE 1 (daily spawn at midnight, Guild Master access).
- WoE 2 castles provide materials for **Asprika** and **Brynhild** God Items (God Items Quest 2).
- Castle economy must reach **60+ Commerce** and **30+ Defense** to unlock God Item Quest 2 access.

---

## 6. Battlegrounds

### 6.1 Overview

Battlegrounds (BG) is a team-based PvP system introduced as structured, objective-based combat with matchmaking and unique rewards. It provides an alternative to open PvP and WoE.

**Access**: Maroll Battle Recruiter NPCs in major towns (Prontera, Geffen, Al De Baran, Payon, Morroc, Lighthalzen, Rachel).

**Teams**: Two factions -- **Prince Croix** (South) vs **General Guillaume** (North). Players are randomly assigned.

**Cooldown**: 5-minute delay between queue sign-ups after a match.

### 6.2 Game Modes

#### Tierra Gorge (Capture/Destroy -- Single Round)

| Property | Detail |
|----------|--------|
| Objective | Destroy the opposing team's storehouse |
| Map layout | Two forts (north/south) with a central base |
| Central base | Provides spawn point advantage + 3 guardian NPCs |
| Respawn | 25-second death timer |
| Duration | Until one storehouse is destroyed or time expires |

**Rewards**:
- Winners: 3 Bravery Badges
- Losers: 1 Bravery Badge

#### Flavius (Crystal Destruction -- Best of 3)

| Property | Detail |
|----------|--------|
| Objective | Destroy the enemy team's crystal |
| Crystal protection | 2 guardian NPCs per crystal (must be killed first) |
| Rounds | Best 2 out of 3 rounds |
| Scoring | Destroying crystal = 1 point; first to 2 wins |

**Rewards**:
- Winners: 12 Valor Badges
- Losers: 4 Valor Badges
- Tie: 4 Valor Badges each

#### Krieger Von Midgard / KVM (Deathmatch -- Single Round)

| Property | Detail |
|----------|--------|
| Objective | Reduce opposing team's score to 0 |
| Starting score | 1 point per team member |
| Scoring | Each player death = -1 point for that team |
| Time limit | 5 minutes |
| Tiebreaker | Team with more surviving players wins |
| Level tiers | 80+, 60-80, below 60 (separate queues) |

**Rewards**:
- Level 80+: Winners 5 KVM Points, Losers 1 KVM Point
- Below 80: Winners 2 KVM Points, Losers 0 KVM Points

### 6.3 Level Requirements

| Mode | Minimum Level |
|------|---------------|
| Tierra Gorge | 80 |
| Flavius | 80 |
| KVM (High) | 80+ |
| KVM (Mid) | 60-80 |
| KVM (Low) | Below 60 |

### 6.4 Combat Rules

- All WoE skill restrictions apply (no Ice Wall, Teleport, Warp Portal, etc.).
- WoE-style damage reductions apply.
- Sprint and Leap are prohibited.

### 6.5 Rewards and Equipment

**Currency Types**:
- **Bravery Badges** (from Tierra Gorge)
- **Valor Badges** (from Flavius)
- **KVM Points** (from Krieger Von Midgard)

**Equipment Costs**:

| Slot | Badge Cost | KVM Point Cost |
|------|-----------|---------------|
| Weapons | 100 Badges | 2,000 Points |
| Armor | 80 Badges | 840 Points |
| Garment | 50 Badges | 630 Points |
| Shoes | 50 Badges | 580 Points |
| Accessories | 500 Badges | 1,200 Points |

- All BG equipment is **character-bound** (cannot be traded).
- Consumable rations available for 5-10 Badges.
- BG equipment has unique set bonuses and is competitive with mid-to-high-tier WoE gear.

### 6.6 Classic Availability Note

Battlegrounds was introduced in **Episode 13.2** (2008-2009 era). Whether it qualifies as "Classic" depends on the server's definition. Many "Classic" servers include BG; some Pre-Renewal purists exclude it. For Sabri_MMO, Battlegrounds is recommended as a Phase 2+ feature after core WoE is stable.

---

## 7. God Items (Divine Equipment)

### 7.1 Overview

God Items (also called Divine/Godly Items) are the most powerful and rarest equipment in Ragnarok Online. Their materials can ONLY be obtained from WoE castle Treasure Boxes, making WoE participation mandatory for crafting them.

There are **6 God Items** total, released across two quest series:

### 7.2 God Items Quest 1 (Seal Quests)

The four original God Items are obtained through a server-wide progression system called the "Seal Quests":

| Seal | Item | Type | Source Realm | Level Req |
|------|------|------|-------------|-----------|
| Seal 1 | **Sleipnir** | Footgear (Shoes) | Valkyrie (Prontera) | 70+ |
| Seal 2 | **Megingjard** | Accessory (Belt) | Balder (Payon) | 70+ |
| Seal 3 | **Brisingamen** | Accessory (Necklace) | Britoniah (Geffen) | 60+ |
| Seal 4 | **Mjolnir** | Weapon (Mace) | Luina (Al De Baran) | 70+ |

**Server-Wide Progression**:
- At least **50 players** must complete each seal before the next seal becomes available.
- After **100 players** complete a seal, it permanently closes.
- This creates server-wide community events and extreme competition for limited slots.

### 7.3 God Item Stats

**Sleipnir** (Footgear):
- Max HP +20%, Max SP +20%
- SP Recovery Rate +100%
- MDEF +10
- Increased movement speed
- Cannot be refined; indestructible
- All jobs can equip

**Megingjard** (Accessory -- Belt):
- STR +40, DEF +7, MDEF +7
- Cannot be refined; indestructible
- Unbreakable
- Reduces movement speed (heavy belt flavor)
- Cannot be equipped with Brisingamen simultaneously

**Brisingamen** (Accessory -- Necklace):
- STR +6, AGI +6, VIT +6, INT +6, DEX +6, LUK +6
- MDEF +5
- Increases Heal/Sanctuary effectiveness by 3%
- Cannot be refined; indestructible

**Mjolnir** (Mace -- Weapon):
- ATK +250
- STR +15, DEX +40
- Cannot be refined; indestructible
- Weapon Level 4
- Required Base Level: 95+
- Extremely high ASPD modifier

### 7.4 God Items Quest 2

Two additional God Items from WoE 2 castles:

| Item | Type | Source |
|------|------|--------|
| **Asprika** | Garment | WoE 2 (Nidhoggur/Valfreyja) |
| **Brynhild** | Armor | WoE 2 (Nidhoggur/Valfreyja) |

**Asprika** (Garment):
- 15% damage reduction from all elements
- FLEE +15, MDEF +2
- Cannot be refined; indestructible

**Brynhild** (Armor):
- Max HP +20%, Max SP +20%
- MDEF +10
- All Stats +10
- Cannot be refined; indestructible

### 7.5 Material Drop Rates

- God Item materials drop from WoE Treasure Boxes at approximately **0.4% (1/250)** per box.
- A guild with high Commerce investment (more boxes per day) has better odds.
- Crafting a single God Item requires **multiple unique materials**, each from different castles' treasure pools.
- Realistically, crafting one God Item takes **months of continuous castle ownership**.

---

## 8. Implementation Plan

### 8.1 Database Schema

#### parties Table
```sql
CREATE TABLE parties (
    id              SERIAL PRIMARY KEY,
    name            VARCHAR(24) NOT NULL UNIQUE,
    leader_id       INTEGER NOT NULL REFERENCES characters(id),
    exp_mode        VARCHAR(10) NOT NULL DEFAULT 'each_take',    -- 'each_take' | 'even_share'
    item_mode       VARCHAR(12) NOT NULL DEFAULT 'each_take',    -- 'each_take' | 'party_share' | 'individual' | 'shared'
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_parties_leader ON parties(leader_id);
```

#### party_members Table
```sql
CREATE TABLE party_members (
    id              SERIAL PRIMARY KEY,
    party_id        INTEGER NOT NULL REFERENCES parties(id) ON DELETE CASCADE,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    joined_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(party_id, character_id),
    UNIQUE(character_id)  -- a character can only be in one party
);

CREATE INDEX idx_party_members_party ON party_members(party_id);
CREATE INDEX idx_party_members_char ON party_members(character_id);
```

#### guilds Table
```sql
CREATE TABLE guilds (
    id              SERIAL PRIMARY KEY,
    name            VARCHAR(24) NOT NULL UNIQUE,
    master_id       INTEGER NOT NULL REFERENCES characters(id),
    level           INTEGER NOT NULL DEFAULT 1,
    exp             BIGINT NOT NULL DEFAULT 0,
    exp_next_level  BIGINT NOT NULL DEFAULT 10000,
    max_members     INTEGER NOT NULL DEFAULT 16,
    emblem_data     BYTEA,                          -- 24x24 BMP emblem binary
    emblem_version  INTEGER NOT NULL DEFAULT 0,     -- incremented on emblem change
    notice_title    VARCHAR(60) DEFAULT '',
    notice_body     VARCHAR(240) DEFAULT '',
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_guilds_master ON guilds(master_id);
```

#### guild_members Table
```sql
CREATE TABLE guild_members (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    position_id     INTEGER NOT NULL DEFAULT 0,     -- 0-19 position slot
    exp_donated     BIGINT NOT NULL DEFAULT 0,
    online          BOOLEAN NOT NULL DEFAULT FALSE,
    joined_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(guild_id, character_id),
    UNIQUE(character_id)  -- a character can only be in one guild
);

CREATE INDEX idx_guild_members_guild ON guild_members(guild_id);
CREATE INDEX idx_guild_members_char ON guild_members(character_id);
```

#### guild_positions Table
```sql
CREATE TABLE guild_positions (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    position_index  INTEGER NOT NULL,               -- 0-19
    title           VARCHAR(24) NOT NULL DEFAULT 'Member',
    can_invite      BOOLEAN NOT NULL DEFAULT FALSE,
    can_kick        BOOLEAN NOT NULL DEFAULT FALSE,
    can_storage     BOOLEAN NOT NULL DEFAULT FALSE,
    tax_rate        INTEGER NOT NULL DEFAULT 0,     -- 0-50
    UNIQUE(guild_id, position_index)
);
```

#### guild_skills Table
```sql
CREATE TABLE guild_skills (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    skill_id        INTEGER NOT NULL,               -- matches ro_guild_skill_data IDs
    skill_level     INTEGER NOT NULL DEFAULT 0,
    UNIQUE(guild_id, skill_id)
);

CREATE INDEX idx_guild_skills_guild ON guild_skills(guild_id);
```

#### guild_alliances Table
```sql
CREATE TABLE guild_alliances (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    allied_guild_id INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    relation        VARCHAR(10) NOT NULL DEFAULT 'ally',  -- 'ally' | 'enemy'
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE(guild_id, allied_guild_id)
);

CREATE INDEX idx_guild_alliances_guild ON guild_alliances(guild_id);
```

#### guild_storage Table
```sql
CREATE TABLE guild_storage (
    id              SERIAL PRIMARY KEY,
    guild_id        INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL,
    amount          INTEGER NOT NULL DEFAULT 1,
    slot_index      INTEGER NOT NULL,               -- 0-499
    refine_level    INTEGER NOT NULL DEFAULT 0,
    card_0          INTEGER DEFAULT 0,
    card_1          INTEGER DEFAULT 0,
    card_2          INTEGER DEFAULT 0,
    card_3          INTEGER DEFAULT 0,
    UNIQUE(guild_id, slot_index)
);

CREATE INDEX idx_guild_storage_guild ON guild_storage(guild_id);
```

#### woe_castles Table
```sql
CREATE TABLE woe_castles (
    id                  SERIAL PRIMARY KEY,
    castle_name         VARCHAR(32) NOT NULL UNIQUE,     -- 'Kriemhild', 'Gondul', etc.
    realm               VARCHAR(24) NOT NULL,            -- 'valkyrie', 'balder', 'britoniah', 'luina'
    map_name            VARCHAR(32) NOT NULL,            -- 'prtg_cas01', etc.
    owner_guild_id      INTEGER REFERENCES guilds(id) ON DELETE SET NULL,
    economy_level       INTEGER NOT NULL DEFAULT 0,
    defense_level       INTEGER NOT NULL DEFAULT 0,
    economy_invest_today INTEGER NOT NULL DEFAULT 0,     -- resets daily, max 2
    defense_invest_today INTEGER NOT NULL DEFAULT 0,     -- resets daily, max 2
    last_invest_reset   DATE NOT NULL DEFAULT CURRENT_DATE,
    guardian_1_alive    BOOLEAN NOT NULL DEFAULT TRUE,
    guardian_2_alive    BOOLEAN NOT NULL DEFAULT TRUE,
    emperium_hp         INTEGER NOT NULL DEFAULT 68430,
    captured_at         TIMESTAMPTZ,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_woe_castles_owner ON woe_castles(owner_guild_id);
```

#### woe_treasure_log Table
```sql
CREATE TABLE woe_treasure_log (
    id              SERIAL PRIMARY KEY,
    castle_id       INTEGER NOT NULL REFERENCES woe_castles(id),
    guild_id        INTEGER NOT NULL REFERENCES guilds(id),
    opened_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    items_json      JSONB NOT NULL DEFAULT '[]'      -- array of {itemId, amount} dropped
);
```

#### pvp_stats Table
```sql
CREATE TABLE pvp_stats (
    id              SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id),
    kills           INTEGER NOT NULL DEFAULT 0,
    deaths          INTEGER NOT NULL DEFAULT 0,
    highest_streak  INTEGER NOT NULL DEFAULT 0,
    last_pvp_at     TIMESTAMPTZ,
    UNIQUE(character_id)
);
```

### 8.2 Server-Side WoE Scheduler

```javascript
// ===== WoE Scheduler (server/src/woe_scheduler.js) =====
// Manages WoE time windows, castle state transitions, and tick logic.

const WOE_SCHEDULE = [
    { day: 3, startHour: 19, startMin: 0, durationMin: 120 },  // Wednesday 7-9 PM
    { day: 6, startHour: 13, startMin: 0, durationMin: 120 },  // Saturday 1-3 PM
];

// WoE state machine
let woeActive = false;
let woeStartTime = null;
let woeEndTime = null;

function checkWoeSchedule() {
    const now = new Date();
    const day = now.getDay();
    const hour = now.getHours();
    const min = now.getMinutes();

    for (const slot of WOE_SCHEDULE) {
        if (day === slot.day) {
            const startMin = slot.startHour * 60 + slot.startMin;
            const endMin = startMin + slot.durationMin;
            const nowMin = hour * 60 + min;

            if (nowMin >= startMin && nowMin < endMin && !woeActive) {
                startWoe();
            } else if (nowMin >= endMin && woeActive) {
                endWoe();
            }
        }
    }
}

// Run check every 30 seconds
setInterval(checkWoeSchedule, 30000);

function startWoe() {
    woeActive = true;
    woeStartTime = Date.now();
    // Broadcast to all players
    io.emit('woe:start', { message: 'The War of Emperium has begun!' });
    // Warp non-owners out of castles
    warpNonOwnersFromCastles();
    // Spawn guardians for castle owners
    spawnCastleGuardians();
}

function endWoe() {
    woeActive = false;
    woeEndTime = Date.now();
    // Broadcast end
    io.emit('woe:end', { message: 'The War of Emperium has ended.' });
    // Finalize castle ownership
    finalizeCastleOwnership();
    // Warp all non-owners out
    warpNonOwnersFromCastles();
    // Save castle state to DB
    saveCastleStatesToDB();
}
```

### 8.3 Castle Ownership Change Flow

```javascript
// ===== Emperium Break Logic =====

function onEmperiumDamaged(castleId, attackerId, damage) {
    if (!woeActive) return;

    const castle = activeCastles.get(castleId);
    if (!castle) return;

    const attacker = onlinePlayers.get(attackerId);
    if (!attacker || !attacker.guildId) return;

    // Check guild has Official Approval
    const guild = guilds.get(attacker.guildId);
    if (!guild || !guild.skills.includes('official_approval')) return;

    // Check not same guild or ally
    if (attacker.guildId === castle.ownerGuildId) return;
    if (isAllied(attacker.guildId, castle.ownerGuildId)) return;

    // Apply damage
    castle.emperiumHp -= damage;

    // Broadcast HP update to castle zone
    broadcastToCastle(castleId, 'woe:emperium_hp', {
        castleId,
        hp: castle.emperiumHp,
        maxHp: castle.emperiumMaxHp
    });

    if (castle.emperiumHp <= 0) {
        // Castle conquered!
        onCastleConquered(castleId, attacker.guildId, attackerId);
    }
}

function onCastleConquered(castleId, newGuildId, breakerId) {
    const castle = activeCastles.get(castleId);
    const oldGuildId = castle.ownerGuildId;

    // Update ownership
    castle.ownerGuildId = newGuildId;
    castle.emperiumHp = castle.emperiumMaxHp; // Reset Emperium

    // Warp out all non-allied players
    warpNonAlliedFromCastle(castleId, newGuildId);

    // Broadcast
    io.emit('woe:castle_captured', {
        castleId,
        castleName: castle.name,
        guildId: newGuildId,
        guildName: guilds.get(newGuildId).name,
        breakerName: onlinePlayers.get(breakerId).name
    });

    // Respawn guardians for new owner
    despawnCastleGuardians(castleId);
    spawnCastleGuardians(castleId);
}
```

### 8.4 Socket.io Events

#### Party Events
```
// Client → Server
party:create          { name, expMode, itemMode }
party:invite          { targetCharId }
party:accept_invite   { partyId }
party:reject_invite   { partyId }
party:kick            { targetCharId }
party:leave           {}
party:change_leader   { targetCharId }
party:change_exp_mode { mode }      // 'each_take' | 'even_share'

// Server → Client
party:created         { partyId, name, leaderId }
party:invite_received { partyId, partyName, inviterName }
party:member_joined   { charId, charName, job, level }
party:member_left     { charId, reason }   // 'left' | 'kicked' | 'disconnected'
party:leader_changed  { newLeaderId }
party:exp_mode_changed { mode }
party:dissolved       {}
party:member_hp       { charId, hp, maxHp, sp, maxSp }  // periodic HP sync
party:data            { partyId, name, leaderId, expMode, itemMode, members[] }
```

#### Guild Events
```
// Client → Server
guild:create          { name }                     // requires Emperium in inventory
guild:invite          { targetCharId }
guild:accept_invite   { guildId }
guild:reject_invite   { guildId }
guild:kick            { targetCharId }
guild:leave           {}
guild:change_position { targetCharId, positionId }
guild:set_position    { positionIndex, title, canInvite, canKick, canStorage, taxRate }
guild:set_notice      { title, body }
guild:set_emblem      { emblemBase64 }
guild:use_skill       { skillId }
guild:invest          { castleId, type }           // 'economy' | 'defense'
guild:alliance_request { targetGuildId, relation } // 'ally' | 'enemy'
guild:alliance_accept  { requestId }
guild:alliance_reject  { requestId }
guild:alliance_remove  { targetGuildId }
guild:disband         {}
guild:transfer_leader { targetCharId }
guild:storage_open    {}
guild:storage_move    { fromSlot, toSlot, amount }
guild:storage_deposit { inventorySlot, guildStorageSlot, amount }
guild:storage_withdraw { guildStorageSlot, inventorySlot, amount }
guild:request_info    {}

// Server → Client
guild:created         { guildId, name, masterId }
guild:invite_received { guildId, guildName, inviterName }
guild:member_joined   { charId, charName, job, level }
guild:member_left     { charId, charName, reason }
guild:member_online   { charId, online }
guild:position_changed { charId, positionId, title }
guild:notice_updated  { title, body }
guild:emblem_updated  { guildId, emblemVersion }
guild:skill_used      { skillId, masterId, effectData }
guild:level_up        { newLevel, skillPoints }
guild:disbanded       {}
guild:leader_changed  { newMasterId }
guild:alliance_request_received { requestId, guildName, relation }
guild:alliance_formed { guildId, guildName, relation }
guild:alliance_broken { guildId }
guild:storage_data    { items[] }
guild:data            { guildId, name, level, exp, masterId, notice, positions[], members[], skills[], alliances[] }
guild:invest_result   { castleId, type, newLevel, cost, bonusPoint }
```

#### WoE Events
```
// Server → All Clients
woe:start             { message, endTime }
woe:end               { message, results[] }       // { castleId, guildId, guildName }
woe:castle_captured   { castleId, castleName, guildId, guildName, breakerName }
woe:emperium_hp       { castleId, hp, maxHp }       // to players in castle zone
woe:guardian_spawned  { castleId, guardianId, type, position }
woe:guardian_killed   { castleId, guardianId, killerId }
woe:announcement      { message }                   // global WoE announcements

// Client → Server
woe:attack_emperium   { castleId }                   // auto-attack on Emperium
woe:enter_castle      { castleId }
woe:open_treasure     { castleId }
woe:hire_guardian     { castleId, guardianSlot }
woe:repair_guardian   { castleId, guardianSlot }
```

#### PvP Events
```
// Client → Server
pvp:enter_room        { roomId }
pvp:leave             {}
pvp:duel_request      { targetCharId }
pvp:duel_accept       { requestId }
pvp:duel_reject       { requestId }
pvp:duel_leave        {}

// Server → Client
pvp:entered           { roomId, points, rankings[] }
pvp:kill              { killerId, killerName, victimId, victimName }
pvp:death             { killerId, newPoints }
pvp:kicked            { reason }                    // points <= 0
pvp:rankings_update   { rankings[] }                // { charId, name, points, rank }
pvp:duel_request_received { requestId, challengerName }
pvp:duel_started      { opponentId, opponentName }
pvp:duel_ended        { winnerId, reason }
```

#### Battlegrounds Events
```
// Client → Server
bg:queue              { mode }                      // 'tierra' | 'flavius' | 'kvm'
bg:cancel_queue       {}
bg:ready              {}

// Server → Client
bg:queue_update       { position, estimatedWait }
bg:match_found        { mode, team }                // 'croix' | 'guillaume'
bg:match_start        { mode, teams, objectives }
bg:score_update       { croixScore, guillaumeScore }
bg:round_end          { winner, roundNum }
bg:match_end          { winner, rewards }           // { badges, points }
bg:respawn_timer      { seconds }
```

### 8.5 UE5 Client Implementation

#### New Subsystems Required

| Subsystem | Purpose |
|-----------|---------|
| `PartySubsystem` | Party creation/management, HP bar sync, EXP sharing UI |
| `GuildSubsystem` | Guild window, member list, skills, notices, emblem display |
| `GuildStorageSubsystem` | Guild storage UI (extends existing InventorySubsystem patterns) |
| `PvPSubsystem` | PvP room entry, ranking display, point tracking |
| `WoESubsystem` | WoE timer, castle state, Emperium HP bar, WoE announcements |
| `BattlegroundSubsystem` | BG queue, match state, scoreboard |

#### New Slate Widgets

| Widget | Z-Order | Purpose |
|--------|---------|---------|
| `SPartyWidget` | 11 | Party member list with HP bars, invite/kick buttons |
| `SGuildWidget` | 13 | Full guild window (tabs: Info, Members, Positions, Skills, Alliances) |
| `SGuildStorageWidget` | 14 | Grid-based storage (same pattern as SInventoryWidget) |
| `SPvPRankingWidget` | 18 | Kill/death ranking overlay |
| `SWoEStatusWidget` | 22 | WoE timer, castle ownership map, Emperium HP |
| `SBattlegroundScoreWidget` | 22 | BG score/objective overlay |
| `SDuelWidget` | 17 | Duel request/accept popup |

#### New Blueprint Actors

| Actor | Purpose |
|-------|---------|
| `BP_Emperium` | Destructible Emperium object in castle maps. Implements `BPI_Damageable`. Only accepts normal attacks. |
| `BP_Guardian` | Castle guardian NPC. AI similar to enemy mobs but loyal to owning guild. |
| `BP_CastlePortal` | Entry portal to castle maps. Checks WoE state and guild ownership. |
| `BP_TreasureBox` | Interactable treasure box in castle treasure rooms. |
| `BP_PvPArenaEntrance` | NPC/portal for entering PvP rooms. |
| `BP_BarricadeBlock` | Destructible barricade segment (WoE 2). |
| `BP_GuardianStone` | Destructible/repairable guardian stone (WoE 2). |
| `BP_LinkFlag` | Teleport flag for defenders (WoE 2). |

### 8.6 Scalability Considerations for WoE (100+ Players)

WoE is the most demanding content in RO, routinely involving 100-200+ players on a single castle map. Key scalability strategies:

#### Server-Side
1. **Zone-based Socket Rooms**: Each castle map is its own Socket.io room. Only broadcast to players in that castle zone.
2. **Throttled Position Updates**: During WoE, reduce position broadcast frequency from 100ms to 200ms. Batch position updates.
3. **Damage Aggregation**: Aggregate damage numbers over 100ms windows before broadcasting (prevents packet flood from 100+ players attacking simultaneously).
4. **Guardian AI Tick**: Separate guardian AI from the main enemy AI loop. Guardians use a simplified aggro model (nearest non-allied target).
5. **Emperium Damage Queue**: Queue Emperium damage and process in batches to prevent race conditions on the final hit.
6. **Interest Management**: Only send detailed player data (HP, buffs, animations) for players within render distance. Send simplified data (position only) for distant players.
7. **WoE-Specific Tick Rate**: Consider a dedicated WoE combat tick at 100ms (instead of 50ms) to reduce CPU load with many players.

#### Client-Side (UE5)
1. **LOD for Distant Players**: Reduce mesh complexity and disable particle effects for players beyond 50m.
2. **Batch Rendering**: Use instanced rendering for large groups of similarly-equipped players.
3. **Effect Culling**: Disable non-essential VFX when more than 30 players are on screen.
4. **Optimized Name Plates**: Switch from individual widgets to a single overlay widget that paints all name plates in `OnPaint` (same pattern as `WorldHealthBarSubsystem`).
5. **Network Interpolation**: Increase interpolation buffer during WoE to smooth out the reduced update rate.
6. **Deferred Position Processing**: Queue incoming position updates and process in batches per frame (max 20 per frame).

#### Database
1. **Castle state caching**: Keep castle state in Redis during WoE. Only persist to PostgreSQL at WoE end.
2. **Treasure box generation**: Pre-generate treasure box contents at WoE end, not at access time.
3. **Guild data caching**: Cache guild member lists and skill data in memory. Invalidate on changes.

### 8.7 Implementation Phases

| Phase | Systems | Priority |
|-------|---------|----------|
| **Phase 1** | Party System (creation, EXP sharing, HP display, chat) | High |
| **Phase 2** | Guild System (creation, members, positions, tax, emblem, storage) | High |
| **Phase 3** | Guild Skills (passive skills, active skills, cooldowns) | Medium |
| **Phase 4** | PvP Arenas (Yoyo mode, point system, rankings) | Medium |
| **Phase 5** | WoE 1 (castle maps, Emperium, guardians, ownership) | High |
| **Phase 6** | Castle Economy (investments, treasure rooms, guild dungeons) | Medium |
| **Phase 7** | WoE 2 / SE (Guardian Stones, barricades, Link Flags) | Low |
| **Phase 8** | Battlegrounds (3 modes, matchmaking, rewards) | Low |
| **Phase 9** | God Items (quest system, material tracking, crafting) | Low |
| **Phase 10** | Duel System, PvP Nightmare Mode, Alliance/Enemy system | Low |

---

## Sources

- [Party System - iRO Wiki Classic](https://irowiki.org/classic/Party)
- [Party - iRO Wiki](https://irowiki.org/wiki/Party)
- [Guild System - iRO Wiki](https://irowiki.org/wiki/Guild_System)
- [Guild System - iRO Wiki Classic](https://irowiki.org/classic/Guild_System)
- [Guilds - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Guilds)
- [Guild Skill Database - RateMyServer](https://ratemyserver.net/index.php?page=skill_db&jid=10000)
- [PvP - iRO Wiki Classic](https://irowiki.org/classic/PvP)
- [Player Versus Player - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Player_Versus_Player)
- [War of Emperium - iRO Wiki](https://irowiki.org/wiki/War_of_Emperium)
- [War of Emperium - iRO Wiki Classic](https://irowiki.org/classic/War_of_Emperium)
- [War of Emperium - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/War_of_Emperium)
- [War of Emperium 2 - iRO Wiki](https://irowiki.org/wiki/War_of_Emperium_2)
- [WoE Second Edition Introduction - RateMyServer](https://write.ratemyserver.net/ragnoark-online-character-guides/ragnoark-online-character-woe-guides/an-introduction-to-woe-second-edition-woe-20/)
- [Battlegrounds - iRO Wiki Classic](https://irowiki.org/classic/Battlegrounds)
- [Battlegrounds - iRO Wiki](https://irowiki.org/wiki/Battlegrounds)
- [God Items Quest - iRO Wiki](https://irowiki.org/wiki/God_Items_Quest)
- [God Items & Quests - Criatura Academy](https://old.criatura-academy.com/updates/god-items/)
- [Emperium Monster Stats - RateMyServer](https://ratemyserver.net/index.php?page=mob_db&mob_id=1288)
- [Castle Treasure Drops - iRO Wiki Classic](https://irowiki.org/classic/Castle_Treasure_Drops)
- [Guild Dungeon Investment System - iRO Wiki](https://irowiki.org/wiki/Guild_Dungeon_Investment_System)
- [Castle Economy - Ragnarok Project Zero Wiki](https://wiki.playragnarokzero.com/wiki/Castle_Economy)
- [War of Emperium Guide - GameFAQs](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/28786)
- [WoE Castle Maps - Guiderz](http://mvptracker.net/guides/woe_maps.html)
- [Sleipnir Item Stats - RateMyServer](https://ratemyserver.net/index.php?page=item_db&item_id=2410)
- [Megingjord - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Megingjord)
- [Emergency Call - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Emergency_Call)
- [Battle Orders - Ragnarok Wiki (Fandom)](https://ragnarok.fandom.com/wiki/Battle_Orders)
