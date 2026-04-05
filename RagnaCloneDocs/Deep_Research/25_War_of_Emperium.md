# War of Emperium -- Deep Research (Pre-Renewal)

> Comprehensive deep research document covering the War of Emperium (WoE) system in Ragnarok Online Classic (pre-renewal).
> Sources: iRO Wiki Classic, rAthena pre-re source, RateMyServer, divine-pride, Neoseeker WoE Guide, Ragnarok Fandom Wiki, GameFAQs WoE Guide, StrategyWiki, ROGGH Library, RODE2.
> Cross-referenced with existing project docs: `RagnaCloneDocs/08_PvP_Guild_WoE.md` and `RagnaCloneDocs/Implementation/11_PvP_WoE.md`.

---

## Table of Contents

1. [Overview](#1-overview)
2. [WoE Schedule & Timing](#2-woe-schedule--timing)
3. [Castle System](#3-castle-system)
4. [Emperium](#4-emperium)
5. [Guardian System](#5-guardian-system)
6. [Castle Economy](#6-castle-economy)
7. [WoE Restrictions](#7-woe-restrictions)
8. [WoE-Specific Mechanics](#8-woe-specific-mechanics)
9. [WoE Second Edition (WoE 2 / SE)](#9-woe-second-edition-woe-2--se)
10. [God Items](#10-god-items)
11. [Implementation Checklist](#11-implementation-checklist)
12. [Gap Analysis](#12-gap-analysis)

---

## 1. Overview

### What WoE Is

War of Emperium (WoE) is the flagship guild-versus-guild (GvG) endgame content in Ragnarok Online. During scheduled time windows, guilds fight to conquer and defend castles by destroying the Emperium -- a destructible Holy Angel object placed in each castle's innermost room. The guild whose member deals the killing blow to the Emperium claims ownership of that castle.

### Purpose and Rewards

Castle ownership grants:

| Benefit | Description |
|---------|-------------|
| **Guild Headquarters** | A physical castle that serves as the guild's base of operations |
| **Guild Dungeons** | Access to exclusive 3-floor dungeons with unique monsters, high EXP, and MVPs |
| **Daily Treasure Boxes** | 4+ treasure chests spawning daily with rare loot, equipment, and God Item materials |
| **God Item Materials** | The ONLY source for materials to craft the 4 (later 6) most powerful items in the game |
| **Prestige** | Server-wide castle ownership announcements; guilds are ranked by castle count |
| **Kafra Services** | Free Kafra (save, warp, storage) inside owned castles via the Kafra Contract guild skill |

### Prerequisites

- The guild must have the **Official Guild Approval** guild skill (1 skill point) to participate in WoE and damage Emperiums.
- Any guild member can enter castle maps during WoE, but only members of guilds with Official Guild Approval can attack the Emperium.
- No minimum character level is required to enter WoE, though effectiveness varies greatly by level and gear.

---

## 2. WoE Schedule & Timing

### WoE Times and Duration

WoE occurs at fixed weekly times set by the server administrator. The schedule is entirely server-configurable.

**Typical iRO Classic Schedule:**

| Day | Time (Server Time) | Duration |
|-----|-------------------|----------|
| Wednesday | 7:00 PM -- 9:00 PM | 2 hours |
| Saturday | 1:00 PM -- 3:00 PM | 2 hours |

**Key timing rules:**
- WoE runs twice per week on most servers (some run once, some run three times).
- Each session is typically 2 hours, though some servers use 1-hour or 90-minute sessions.
- During non-WoE hours, castles are inaccessible to non-owning guild members. The owning guild can enter freely at any time.
- Castle ownership persists between WoE periods. The guild holding a castle when WoE ends retains ownership until the next WoE session (or until another guild breaks their Emperium in a future session).

**WoE State Machine:**

| State | Duration | Description |
|-------|----------|-------------|
| **INACTIVE** | Between WoE sessions | Castles locked to non-owners. Normal gameplay. |
| **PRE_WOE** | 5 minutes before start | Server-wide announcement. Castles prepare. |
| **ACTIVE** | 2 hours (configurable) | Full WoE combat. Castles open to all guilds. Emperiums vulnerable. |
| **POST_WOE** | 5 minutes after end | Cleanup. Non-owners warped out. Treasure boxes generated. Castle state saved. |

**Announcements:**
- 5 minutes before WoE: "The War of Emperium begins in 5 minutes!"
- WoE start: "The War of Emperium has begun!" (server-wide)
- Castle captured: "[Guild Name] has conquered [Castle Name]!" (server-wide)
- WoE end: "The War of Emperium has ended." with ownership results (server-wide)

### Castle Rotation

In standard pre-renewal RO, all 20 castles across all 4 realms are open simultaneously during WoE. There is no rotation -- every castle is contestable every WoE session.

Some private servers implement rotation systems where only certain realms are open on specific WoE days (e.g., Prontera + Payon on Wednesday, Geffen + Al De Baran on Saturday). This is a server-configurable option, not a core mechanic.

**Recommendation for Sabri_MMO:** Start with all castles open simultaneously. Add optional realm rotation as a configuration option once the base system is stable.

---

## 3. Castle System

### Complete Castle List

**4 realms x 5 castles = 20 WoE 1 castles total.**

#### Valkyrie Realm (north of Prontera)

| # | Castle Name | Map ID | God Item Realm |
|---|------------|--------|----------------|
| 1 | Kriemhild | prtg_cas01 | Sleipnir |
| 2 | Swanhild | prtg_cas02 | Sleipnir |
| 3 | Fadhgrindh | prtg_cas03 | Sleipnir |
| 4 | Skoegul | prtg_cas04 | Sleipnir |
| 5 | Gondul | prtg_cas05 | Sleipnir |

#### Balder Realm / Greenwood Lake (west of Payon)

| # | Castle Name | Map ID | God Item Realm |
|---|------------|--------|----------------|
| 1 | Bright Arbor | payg_cas01 | Megingjard |
| 2 | Scarlet Palace | payg_cas02 | Megingjard |
| 3 | Holy Shadow | payg_cas03 | Megingjard |
| 4 | Sacred Altar | payg_cas04 | Megingjard |
| 5 | Bamboo Grove Hill | payg_cas05 | Megingjard |

#### Britoniah Realm (west/south of Geffen)

| # | Castle Name | Map ID | God Item Realm |
|---|------------|--------|----------------|
| 1 | Repherion | gefg_cas01 | Brisingamen |
| 2 | Eeyorbriggar | gefg_cas02 | Brisingamen |
| 3 | Yesnelph | gefg_cas03 | Brisingamen |
| 4 | Bergel | gefg_cas04 | Brisingamen |
| 5 | Mersetzdeitz | gefg_cas05 | Brisingamen |

#### Luina Realm (west of Al De Baran)

| # | Castle Name | Map ID | God Item Realm |
|---|------------|--------|----------------|
| 1 | Neuschwanstein | aldeg_cas01 | Mjolnir |
| 2 | Hohenschwangau | aldeg_cas02 | Mjolnir |
| 3 | Wuerzberg | aldeg_cas03 | Mjolnir |
| 4 | Nuernberg | aldeg_cas04 | Mjolnir |
| 5 | Rothenburg | aldeg_cas05 | Mjolnir |

### Castle Layout and Structure

Each WoE 1 castle consists of **multiple interconnected maps** (rooms) linked by portals. Players progress from the outer entrance through defensive rooms to reach the Emperium Room.

**Standard WoE 1 Castle Structure:**

```
Castle Entrance (Outer Gate)
    |
    v
Room 1 (First Defense -- wide open area)
    |
    v
Room 2 (Second Defense -- narrower, choke points)
    |
    v
Room 3 (Third Defense -- often a corridor or staircase)
    |
    v
[Optional Room 4 in some castles]
    |
    v
Emperium Room (Final Defense -- Emperium object at center/rear)
```

**Layout characteristics:**
- Each castle has a unique layout. No two castles are identical.
- Castles have 3-5 rooms total, connected by warp portals.
- Each portal transition grants **5 seconds of invulnerability** to the player passing through (to prevent portal camping from being absolutely dominant).
- Rooms decrease in size as you go deeper, creating natural funnels and choke points.
- The Emperium Room is always the last room, typically smaller than other rooms.
- Some castles have alternate paths or side rooms, but all paths eventually lead to the Emperium Room.
- Terrain varies per castle: some have open courtyards, others have tight corridors, bridges, or staircases.

### Choke Points

Choke points are the defining strategic element of castle defense. They are narrow passages (5 cells wide or less) that force attackers into a concentrated area.

**Common choke point locations:**
- Portal exits (where attackers enter a new room)
- Narrow hallways and corridors
- Bridge crossings
- Staircase entrances
- Doorways into the Emperium Room

**Defender strategy at choke points:**
- Wizards cast Firewall/Storm Gust at narrow passages
- Hunters place traps (which last 4x longer in WoE)
- Priests cast Pneuma/Safety Wall on the choke
- Crusaders use Devotion to protect key players
- High Priests use Sanctuary for area healing

### Castle Features

**Flags (WoE 1):**
- Each castle has **flag NPCs** placed outside the castle entrance.
- Clicking a flag allows guild members of the owning guild (and allied guilds) to teleport to a specific location inside the castle.
- Flags are a defensive mechanic: they allow defenders to quickly reinforce interior positions without fighting through their own castle.
- Attackers cannot use flags.
- Flag placement is fixed per castle (not player-configurable in WoE 1).
- Each flag NPC has a dialogue with numbered destination options (e.g., "Go to Flag 1", "Go to Flag 2").

**Castle NPCs:**
- **Kafra NPC**: Provides save/warp/storage services (requires Kafra Contract guild skill).
- **Guardian NPC**: The Guild Master speaks to this NPC to hire/manage guardians (requires Guardian Research guild skill).
- **Investment NPC**: The Guild Master speaks to this NPC to invest in Commerce or Defense.
- **Treasure Room NPC**: Only the Guild Master can access the treasure room through this NPC.
- **Guild Dungeon NPC**: Any guild member can use this NPC to access the guild dungeon.

---

## 4. Emperium

### Stats and Properties

The Emperium is treated as a special monster entity (Monster ID: 1288) with the following stats:

| Property | Value | Notes |
|----------|-------|-------|
| **Monster ID** | 1288 | `EMPELIUM` in mob_db |
| **Level** | 90 | |
| **HP** | 100 (base mob_db) | See "Effective HP" below |
| **Race** | Angel | Racial modifiers apply |
| **Element** | Holy Lv1 | Holy vs Holy = 0% damage |
| **Size** | Small | Size modifiers apply |
| **Class** | Boss | Boss protocol immunities |
| **DEF** | 64 (hard) + 43 (soft) | From rAthena pre-re mob_db |
| **MDEF** | 50 (hard) + 47 (soft) | |
| **Flee** | 207 | High flee for its level |
| **STR** | 1 | |
| **AGI** | 17 | |
| **VIT** | 80 | High VIT |
| **INT** | 50 | |
| **DEX** | 31 | |
| **LUK** | 20 | |
| **ATK** | 143 | Self-defense (irrelevant) |
| **Base EXP** | 0 | No EXP for breaking |
| **Job EXP** | 0 | No EXP for breaking |

**Effective HP in WoE:**

The Emperium's actual HP during WoE is **not** the 100 HP listed in mob_db. The true HP depends on the implementation:

- **rAthena default behavior**: The Emperium HP is set by the guild castle configuration. Typical base HP is **68,430** (this value comes from the castle config, not mob_db).
- **Defense investment bonus**: Each Defense investment point adds **+1,000 HP** to the Emperium.
- **Effective formula**: `Emperium HP = 68,430 + (defense_level * 1,000)`
- A well-invested castle can have an Emperium with 100,000+ HP.

**Note on mob_db HP value**: The 100 HP in mob_db is essentially a placeholder. The actual Emperium HP is managed by the WoE castle system, not the monster database. Some implementations use 68,430 directly in code; others use different base values. The mob_db entry exists primarily to define the Emperium's race, element, size, and DEF/MDEF.

### Immunities

The Emperium has extensive immunities that make it a unique target:

**Skill Immunity:**
- **ALL active skills miss the Emperium.** Only normal (auto) attacks can deal damage.
- This includes all physical skills (Bash, Bowling Bash, Sonic Blow, etc.) and all magical skills (Storm Gust, Lord of Vermilion, etc.).
- **Exceptions** (may damage the Emperium on some servers):
  - **Gloria Domini** (Paladin) -- deals damage based on target's current SP. Some servers allow this; most pre-renewal servers do not.
  - **Gravitational Field** (High Wizard) -- some implementations allow this; not standard in classic pre-renewal.
- Items that activate skills (e.g., scrolls) also fail against the Emperium.
- **Autocast equipment CAN trigger skills** inside castles, but those autocasted skills will still miss the Emperium.

**Elemental Immunity:**
- Holy element attacks deal **0% damage** to the Emperium (Holy vs Holy = 0% in the RO element table).
- This means:
  - **Silver Arrows** (Holy element ammo) deal 0 damage.
  - **Aspersio-buffed weapons** (Holy endow) deal 0 damage.
  - **Holy-element weapons** deal 0 damage.
- **Effective elements against the Emperium:**
  - **Shadow/Dark**: 125% damage (best)
  - **Undead**: 0% damage (Holy vs Undead = 0% per RO element table -- note: this is the same as Holy vs Holy)
  - **Neutral**: 100% damage (standard)
  - **Poison**: 75% damage (reduced but still works)
  - **Ghost**: 90% damage (slightly reduced)
  - All other elements: 100% damage

**Status Immunity:**
- The Emperium is Boss-class, granting immunity to most status effects.
- Stun, Freeze, Stone Curse, Sleep, Silence, Blind, Bleeding, Poison -- all resisted.
- The Emperium cannot be knocked back.

### Breaking Mechanics

**Who can attack the Emperium:**
- Only members of guilds with the **Official Guild Approval** skill.
- The attacker's guild must NOT be the current owner or an allied guild of the owner.
- The attacker must be on the same map as the Emperium.

**How to break the Emperium:**
1. Only **normal auto-attacks** deal damage. No skills.
2. The optimal Emperium breaker uses:
   - **High ASPD**: Maximum attack speed to deal as many hits as possible.
   - **Shadow/Dark element weapon**: Cursed Water (temporary Shadow endow) is the most common method. +25% damage vs Holy.
   - **Small-size damage bonus**: The Emperium is Small size. Daggers deal 100% to Small (no penalty). Weapons with "ignores size" modifiers are ideal.
   - **Anti-Angel racial bonus**: Cards/effects that boost damage vs Angel race.
   - **High HIT**: The Emperium has 207 Flee, requiring decent HIT to land hits.
   - **Critical attacks**: Criticals bypass Flee entirely. The Emperium has "very little crit shield" (low LUK, minimal critical defense). High-crit Assassins using Katar weapons (double crit rate) are classic Emperium breakers.
3. The character who deals the **killing blow** (final hit reducing HP to 0) claims the castle for their guild.

**What happens when the Emperium breaks:**
1. The breaker's guild immediately becomes the new castle owner.
2. **All non-allied characters are warped out** of the castle to their save points.
3. A **new Emperium spawns** for the conquering guild to defend (full HP).
4. A **server-wide announcement** broadcasts: "[Guild Name] conquered [Castle Name]!"
5. New guardians are spawned for the new owner (if they have the Guardian Research skill).
6. The castle can be contested again immediately -- other guilds can re-enter and attempt to break the new Emperium.
7. This can happen **multiple times per WoE session**. Ownership changes frequently in active WoE.

---

## 5. Guardian System

### Guardian Overview

Guardians are powerful NPC monsters that defend the castle on behalf of the owning guild. They automatically attack all non-allied characters (enemies and neutral players) inside the castle during WoE.

### Prerequisites

- The guild must have the **Guardian Research** guild skill to hire guardians.
- The **Strengthen Guardian** guild skill provides bonuses to guardian stats.
- The **Guild Master** must speak to the Guardian NPC inside the castle's Emperium Room to initiate guardian spawning.

### Guardian Types (WoE 1)

Three types of guardians exist in WoE 1:

| Type | Monster ID | HP | ATK | DEF | MDEF | Range | Skills | Notes |
|------|-----------|-----|-----|-----|------|-------|--------|-------|
| **Soldier Guardian** | 1287 | Lowest | Lowest | Medium | Medium | Melee | Bash Lv10, Stun Attack Lv5 (50% stun chance) | Weakest overall but has stun |
| **Knight Guardian** | 1286 | High | High | High | Average | Short melee | None | Pure physical brute, no skills |
| **Archer Guardian** | 1285 | Medium | High | Low | High | 12 cells (ranged) | Arrow Shower Lv10, Double Strafe | Ranged; can interrupt casts |

**Guardian properties (all types):**
- Boss protocol (immune to status effects)
- Can detect hidden/cloaked players
- Neutral element attacks
- Walk toward and attack nearest non-allied player
- Return to spawn position if target moves too far (leash range)

### Guardian Stats and Defense Investment

Guardian stats are boosted by two factors:

1. **Strengthen Guardian guild skill**: Each level provides:
   - +10% Max HP per level
   - +5% ATK per level

2. **Defense Investment level**: Each defense point adds:
   - +1,000 HP to all guardians
   - General stat improvements

**Guardian spawn count:**
- Each castle supports a limited number of guardians (typically 2-4 in WoE 1).
- The exact number available depends on the castle and server configuration.
- The Guild Master must speak to the Guardian NPC to hire each guardian individually.

### Guardian Behavior

- **Idle**: Guardians stand at their assigned spawn position when no enemies are nearby.
- **Chase**: When a non-allied player enters the guardian's detection range, the guardian begins chasing.
- **Attack**: The guardian attacks the nearest non-allied target within attack range.
- **Respawn**: When a guardian is killed, it can be re-hired by the Guild Master during the same WoE session (requires speaking to the Guardian NPC again). There is a **30-second respawn delay** before re-hiring is possible.
- **Ownership change**: When a castle changes hands, old guardians are despawned and the new owner can hire their own.

---

## 6. Castle Economy

### Investment System

Two investment tracks are managed by the Guild Master via NPCs inside the castle. Both are available **outside of WoE hours** (investments cannot be made during active WoE on most servers, though some allow it).

#### Commerce Development (Economy)

| Property | Detail |
|----------|--------|
| **Purpose** | Increases Treasure Box quantity and quality |
| **Effect** | +1 Treasure Box per 5 Commerce investment points |
| **Investment limit** | 2 investments per day |
| **Cost scaling** | Each investment costs **2x** the previous one. Base cost: ~5,000 Zeny |
| **Guild skill bonus** | **Absolute Develop** guild skill: 50% chance of a free bonus economy point per investment |
| **Persistence** | Cumulative; persists as long as the guild holds the castle |
| **On castle loss** | The new owner inherits the current investment levels |

#### Defense Development

| Property | Detail |
|----------|--------|
| **Purpose** | Strengthens castle defenses |
| **Effect** | +1,000 HP to Guardians AND Emperium per defense point |
| **Investment limit** | 2 investments per day |
| **Cost scaling** | Each investment costs **2x** the previous one. Base cost: ~5,000 Zeny |
| **Guardian bonus** | Increased guardian count capacity |
| **Persistence** | Cumulative; persists as long as the guild holds the castle |
| **On castle loss** | The new owner inherits the current investment levels |

**Investment decay:** On some servers, investment levels slowly decay if not maintained. This is server-configurable. Standard behavior is no decay.

**Investment reset:** Daily investment counters (the 2/day limit) reset at midnight server time.

### Treasure Chests

Treasure chests are the primary economic reward for castle ownership.

**Spawn mechanics:**
- Treasure Boxes appear every day at **12:00 AM server time** (midnight) in the castle's Treasure Room.
- Only the **Guild Master** can enter the Treasure Room via the castle's Treasure Room NPC.
- Base count: **4 Treasure Boxes** per castle per day (minimum).
- Commerce bonus: **+1 additional Treasure Box** per 5 Commerce investment points.
- Maximum with high investment: 10-15+ boxes per day.

**Treasure Box types:**
- Each castle has **2 types** of Treasure Boxes:
  1. A **Common Treasure Box** shared by all castles in the realm.
  2. A **Castle-specific Treasure Box** unique to that particular castle.

**Treasure Box contents (typical):**

| Category | Examples | Drop Rate |
|----------|----------|-----------|
| **Equipment** | Mid-tier weapons and armor, WoE-relevant gear | Common |
| **Consumables** | Potions, scrolls, stat foods | Common |
| **Crafting materials** | Oridecon, Elunium, gemstones | Uncommon |
| **Rare equipment** | High-tier weapons, headgear | Rare |
| **God Item materials** | Realm-specific God Item components | ~0.4% (1/250) per box |

**God Item material distribution by realm:**

| Realm | Castles | God Item | Material Examples |
|-------|---------|----------|-------------------|
| Valkyrie (Prontera) | prtg_cas01-05 | **Sleipnir** (Footgear) | Sleipnir components |
| Balder (Payon) | payg_cas01-05 | **Megingjard** (Accessory) | Megingjard components |
| Britoniah (Geffen) | gefg_cas01-05 | **Brisingamen** (Accessory) | Brisingamen components |
| Luina (Al De Baran) | aldeg_cas01-05 | **Mjolnir** (Mace) | Mjolnir components |

### Castle Dungeons (Guild Dungeons)

Each realm has guild dungeons accessible to all members of the castle-owning guild.

**Access:**
- Enter via the Guild Dungeon NPC inside the castle.
- Any guild member can enter (not just the Guild Master).
- Access is available **24/7**, not just during WoE hours.
- **All 5 castles in a realm share the same guild dungeon.** Enemy guilds who own different castles in the same realm will encounter each other in the dungeon.

**Dungeon structure:**
- 3 floors per realm.
- Floor 1 (GD1): PvE only. Normal monsters.
- Floor 2 (GD2): PvE only. Stronger monsters.
- Floor 3 (Hall of Abyss): **GvG enabled** -- players from different guilds can attack each other. Strongest monsters and MVPs.

**Death in guild dungeons:** Unlike castle WoE maps, **death in guild dungeons DOES cause EXP loss** (standard PvE death penalty applies). This is true even if killed by another player in the Hall of Abyss.

**Guild Dungeon Monsters by Realm:**

| Realm | Notable Monsters | MVP |
|-------|-----------------|-----|
| **Prontera (Valkyrie)** | Maya Purple, Creamy Fear, Caterpillar, Leib Olmai, Gullinbursti | Maya, Baphomet |
| **Payon (Balder)** | Vagabond Wolf, Skeleton General, Am Mut, Cat o' Nine Tails, Gajomart | Eddga |
| **Geffen (Britoniah)** | Dark Hammer/Mace/Axe Kobold, Dark Kobold Archer, Kobold Leader | Atroce |
| **Al De Baran (Luina)** | Dark Kobold variants, Old Treasure Box, Laura | Angry Student Pyuriel |

**Guild dungeon characteristics:**
- Very fast monster respawn rate
- High EXP per monster compared to normal dungeons
- Valuable monster drops (exclusive or rare items)
- MVPs have long respawn timers (typically 8 hours)

---

## 7. WoE Restrictions

### Disabled Skills During WoE

The following skills **cannot be used** inside castle maps during WoE:

| Skill | ID | Reason |
|-------|----|--------|
| **Teleport** | 26 (AL_TELEPORT) | Prevents escape; forces fighting or death |
| **Warp Portal** | 27 (AL_WARP) | Prevents strategic warping inside castles |
| **Ice Wall** | 14 (WZ_ICEWALL) | Would create impassable barriers in choke points |
| **Basilica** | 316 (HP_BASILICA) | Creates an invulnerable zone; too powerful for defense |
| **Assumptio** | 353 (HP_ASSUMPTIO) | Halves damage taken; too powerful for WoE (pre-renewal) |
| **Intimidate / Snatch** | 1707 (RG_INTIMIDATE) | Forced warp of target; broken in castle context |
| **Plant Cultivation** | 491 (CR_CULTIVATION) | Spawns plants that block movement |
| **Sheltering Bliss / Moonlit Water Mill** | 1535 (CG_MOONLIT) | Ensemble skill; prevents passage |
| **Hocus Pocus / Abracadabra** | 1420 (SA_ABRACADABRA) | Random skill effects; unpredictable in WoE (iRO) |

**Additional notes on skill restrictions:**
- Using **items that activate disabled skills** also fails (e.g., Fly Wing for Teleport).
- **Autocast equipment** CAN grant disabled skills inside castles. If a weapon autocasts Ice Wall, it will work. However, autocasted skills still miss the Emperium.
- **Endure** does not work inside castles (its flinch immunity is suppressed).
- **Equipment preventing cast interruption** (Phen Card and similar effects) does NOT function inside WoE castles. All casts can be interrupted.
- **Back Slide** (331): Disabled on some servers. Not universally disabled in pre-renewal.

### Item Restrictions

| Restriction | Detail |
|-------------|--------|
| **Fly Wing** | Blocked (activates Teleport, which is disabled) |
| **Butterfly Wing** | Allowed (returns to save point -- acts as retreat) |
| **Potions** | Some servers reduce potion effectiveness by 50% in WoE. Server-configurable. |
| **Yggdrasil Berry** | Typically allowed but some servers restrict instant full heals |
| **Dead Branch / Bloody Branch** | Blocked (monster summoning in castles) |

### Combat Modifications

| Modification | Value | Notes |
|-------------|-------|-------|
| **Skill damage reduction** | **-40%** | ALL skill-based damage (physical + magical) except Gloria Domini and Gravitational Field |
| **Long-range normal attack reduction** | **-25%** | Applies to bows, guns, and other ranged normal attacks |
| **Short-range normal attack reduction** | **0%** | No reduction for melee auto-attacks |
| **Flee reduction** | **-20%** | All players' Flee is reduced by 20% inside castles |
| **Trap duration multiplier** | **4x** | Hunter/Rogue traps last 4 times longer (makes trappers very powerful) |
| **Knockback** | **Disabled** | ALL knockback effects have no displacement. Skills that push (e.g., Arrow Shower, Bowling Bash) deal damage but do not move the target. |

### Teleportation Rules

| Method | Allowed? | Notes |
|--------|----------|-------|
| Teleport (skill) | NO | Disabled in castle maps |
| Fly Wing (item) | NO | Activates Teleport |
| Butterfly Wing | YES | Returns to save point (escape method) |
| Warp Portal (skill) | NO | Disabled in castle maps |
| @commands (@warp, @go) | NO | GM commands disabled during WoE |
| Emergency Call | YES | Guild Master skill, WoE only |
| Castle Flags | YES | Defenders only; fixed positions |
| Portal transitions | YES | Moving between castle rooms |

---

## 8. WoE-Specific Mechanics

### Guild Leader Recall / Emergency Call

**Emergency Call** (also known as **Urgent Call**) is an active guild skill usable only by the Guild Master.

| Property | Value |
|----------|-------|
| **Skill name** | Emergency Call / Urgent Call |
| **Skill ID** | 10013 |
| **Effect** | Teleports ALL online guild members to the Guild Master's location |
| **Cast time** | 5 seconds (fixed, uninterruptible) |
| **Cooldown** | 5 minutes (**shared** with Battle Orders, Regeneration, and Restore) |
| **SP cost** | 1 SP |
| **Restriction** | WoE only -- cannot be used outside WoE period or outside castle maps |
| **Exception** | If the Guild Master is a Taekwon/Star Gladiator class with Leap, cast time is doubled to 10 seconds |

**Emergency Call mechanics:**
- Teleports ALL online guild members regardless of their current map (even if on a different continent).
- Members who are **dead** at the time of casting are NOT teleported.
- Members who are on a **loading screen** (zone transitioning) are NOT teleported.
- Members are teleported to the Guild Master's exact position, causing a "stack" of players.
- This is the primary defensive recall tool: the Guild Master calls all members to reinforce the Emperium Room.

**Shared cooldown:** Using Emergency Call, Battle Orders, Regeneration, OR Restore puts ALL four skills on a 5-minute cooldown. The Guild Master must choose which skill to use each 5-minute window.

### Guild Flag System (WoE 1)

Castle flags are NPC objects placed at fixed positions outside and inside each castle.

**Flag mechanics:**
- **Exterior flags**: Placed just outside the castle entrance. Owning guild members can click them to warp inside the castle, bypassing the entrance entirely.
- **Interior flags**: Placed at strategic positions throughout the castle (near choke points, near the Emperium Room).
- **Usage**: Click a flag NPC to see a list of numbered destinations. Select a destination to instantly warp there.
- **Defender only**: Only members of the owning guild (and allied guilds) can use flags.
- **Attackers cannot use flags** -- they must fight through every room.
- **Flag placement is fixed** per castle (determined by the castle's NPC script, not configurable by players in WoE 1).

### Charge Shout Flag / Charge Shout Beating (Guild Skills)

| Skill | Effect | Cooldown |
|-------|--------|----------|
| **Charge Shout Flag** | Guild Master places a "Flag of Assault" on the ground during WoE | 15 minutes |
| **Charge Shout Beating** | Teleports nearby guild members to the Flag of Assault location | 15 minutes |

These skills are less commonly used than Emergency Call but provide an additional tactical warp option.

### Death Mechanics in WoE

| Aspect | Detail |
|--------|--------|
| **EXP loss** | **None.** No base or job EXP is lost when dying inside castle maps during WoE. |
| **Item drop** | **None.** No items or equipment are dropped on death in WoE. |
| **Respawn location** | Player respawns at their **save point** (the last Kafra save they registered). |
| **Resurrection** | Priests CAN resurrect dead players inside the castle (no EXP loss on resurrect either). |
| **Buffs on death** | Most buffs are lost on death. Players must rebuff after respawning. |
| **Return to battle** | After respawning at the save point, players must manually walk/warp back to the castle entrance and fight through again. Emergency Call from the Guild Master is the fastest way back. |

**Death in Guild Dungeons**: Unlike castle WoE maps, death in guild dungeons causes normal EXP loss (standard death penalty applies).

### Pre-Cast / Buff Stacking at Portals

A major WoE 1 strategy is "pre-casting" or "pre-buffing" at portal transitions:

- Defenders stack buffs and place AoE skills (Storm Gust, Lord of Vermilion, Meteor Storm) at the exact position where attackers will appear when transitioning through a portal.
- Attackers receive 5 seconds of invulnerability when transitioning through a portal, giving them time to move out of the kill zone.
- This creates a specific WoE meta: defenders must time their casts to land just as invulnerability expires, while attackers must spread out immediately upon entering.

### Damage Numbers and Visual Effects

- **`/mineffect`** is automatically activated in WoE castles: damage numbers and visual effects are hidden to reduce visual clutter and improve client performance with large numbers of players.
- Players can see their own damage numbers but not others'.

### Alliance Rules in Castles

- Guild mates and allied guilds are treated as **allies** inside castles. They cannot attack each other.
- Members **cannot be expelled** from the guild while inside a castle.
- Guild **alliances cannot be broken** while any alliance member is inside a castle.
- Up to **3 allied guilds** at once. Allied members appear as friendly (green names).

---

## 9. WoE Second Edition (WoE 2 / SE)

WoE 2 (also called WoE:SE or Second Edition) was introduced as a later expansion with fundamentally different castle designs and mechanics.

### Key Differences from WoE 1

| Feature | WoE 1 | WoE 2 |
|---------|-------|-------|
| Castle design | Multiple maps connected by portals | Single large contiguous map |
| Defensive structures | None (terrain only) | Guardian Stones + Barricades (must be built) |
| Guardian spawning | Guild Master hires manually | Automatic from Guardian Stones |
| Link Flags | Outside castle only | 12 flags throughout castle interior |
| Pre-casting meta | Dominant strategy | Eliminated (no portal transitions) |
| Emphasis | Offensive rushing | Defensive investment + strategy |
| Castle realms | 4 (Prontera/Payon/Geffen/Al De Baran) | 2 (Juno/Rachel) |
| Total castles | 20 | 10 |

### WoE 2 Castle Locations

**Nidhoggur Realm** (Schwartzwald Republic / Juno):

| # | Castle Name | Map ID |
|---|------------|--------|
| 1 | Himinn | schg_cas01 |
| 2 | Andlangr | schg_cas02 |
| 3 | Viblainn | schg_cas03 |
| 4 | Hljod | schg_cas04 |
| 5 | Skidbladnir | schg_cas05 |

**Valfreyja Realm** (Arunafeltz / Rachel):

| # | Castle Name | Map ID |
|---|------------|--------|
| 1 | Mardol | arug_cas01 |
| 2 | Cyr | arug_cas02 |
| 3 | Horn | arug_cas03 |
| 4 | Gefn | arug_cas04 |
| 5 | Syr | arug_cas05 |

### Guardian Stones (WoE 2)

- **2 Guardian Stones** per castle -- the primary defensive barrier.
- While BOTH Guardian Stones are intact, an **indestructible barrier** blocks the castle interior entrance. Attackers physically cannot pass.
- Destroying a Guardian Stone removes part of the barrier and kills the guardians it spawned.
- **Both stones must be destroyed** to fully open the path to barricades and the Emperium.

**Guardian Stone stats:**
- HP: ~200,000 per stone
- Can be attacked by any non-allied player

**Guardian Stone repair:**
- Materials required: 30 Stone, 1 Oridecon, 1 Elunium, 5 Blue Gemstones, 5 Red Gemstones, 5 Yellow Gemstones
- **8-minute cooldown** after destruction before repair is possible
- Any guild member with repair permission can repair (not just Guild Master)
- Repairing a stone immediately re-spawns its associated guardians

**Guardian spawning from stones:**
- When a Guardian Stone is constructed, an initial number of Guardians spawn immediately (depends on Castle Defense Level).
- Additional guardians spawn at intervals: 5 minutes (1st), 15 minutes (2nd), 30 minutes (3rd), 45 minutes (4th), 60 minutes (5th).
- WoE 2 guardians are **approximately 5x stronger** than WoE 1 guardians.
- WoE 2 guardians are Boss protocol, immune to status effects, and can detect hidden players.
- They use close-range neutral melee attacks (Sword Guardian type).

### Barricade System (WoE 2)

- **3 sets of barricades** create impassable defensive lines between the Guardian Stones area and the Emperium Room.
- Each barricade set consists of **4-8 individual blocks** -- ALL blocks in a set must be destroyed before players can pass.
- Barricades are **NOT automatically present** when a castle is first captured. The Guild Master must install them.

**Installation cost per barricade set:**
- 30 Trunk, 10 Steel, 10 Emvertarcon, 5 Oridecon

**Barricade stats:**
- HP per block: approximately **450,000 HP** -- extremely durable
- Only damageable if both Guardian Stones are destroyed

**Repair rules:**
- Must be repaired **sequentially from the Emperium Room outward** (innermost first)
- Once destroyed during active WoE, barricades **remain destroyed** for the rest of that WoE session
- Can only be reinstalled after WoE ends

### Link Flags (WoE 2)

- **12 Link Flags** placed throughout the castle for the **defending guild only**.
- Flags allow instant teleportation between any two flag locations inside the castle.
- External flags allow defenders to warp directly into the interior, bypassing attackers at the gate.
- Flags near the Emperium Room allow rapid reinforcement of the final defense.
- **Attackers cannot use Link Flags.**

### WoE 2 Treasure and Economy

- Treasure Boxes work the same as WoE 1 (daily spawn at midnight, Guild Master access).
- WoE 2 castles provide materials for **Asprika** and **Brynhild** God Items (God Items Quest 2).
- Castle economy must reach **60+ Commerce** and **30+ Defense** to unlock God Item Quest 2 access.

### Additional WoE 2 Skill Restrictions

All WoE 1 skill restrictions apply, plus:
- **High Jump** (TK Ranker skill) is disabled
- **Leap** (similar movement skill) is disabled

---

## 10. God Items

### Overview

God Items (Divine Equipment) are the most powerful and rarest equipment in Ragnarok Online. Their materials can ONLY be obtained from WoE castle Treasure Boxes, making WoE participation mandatory for crafting them.

### God Items Quest 1 (Seal Quests) -- WoE 1

| Seal # | Item | Type | Source Realm | Level Req |
|--------|------|------|-------------|-----------|
| 1 | **Sleipnir** | Footgear (Shoes) | Valkyrie (Prontera) | 70+ |
| 2 | **Megingjard** | Accessory (Belt) | Balder (Payon) | 70+ |
| 3 | **Brisingamen** | Accessory (Necklace) | Britoniah (Geffen) | 60+ |
| 4 | **Mjolnir** | Weapon (Mace) | Luina (Al De Baran) | 70+ |

**Server-wide progression:**
- At least **50 players** must complete each seal before the next seal becomes available.
- After **100 players** complete a seal, it permanently closes.
- This creates server-wide community events and extreme competition for limited slots.
- A Guild Master with guild dungeon access brings assembled materials to the **Grunburti NPC** in the guild dungeons to craft the God Item.

**God Item stats:**

| Item | Key Stats | Notes |
|------|-----------|-------|
| **Sleipnir** | Max HP +20%, Max SP +20%, SP Recovery +100%, MDEF +10, increased move speed | All jobs, indestructible, cannot be refined |
| **Megingjard** | STR +40, DEF +7, MDEF +7, reduces move speed | Cannot equip with Brisingamen, indestructible |
| **Brisingamen** | All stats +6, MDEF +5, +3% Heal/Sanctuary effectiveness | Indestructible, cannot be refined |
| **Mjolnir** | ATK +250, STR +15, DEX +40, Weapon Level 4, extreme ASPD modifier | Requires Base Level 95+, indestructible |

### God Items Quest 2 -- WoE 2

| Item | Type | Source | Key Stats |
|------|------|--------|-----------|
| **Asprika** | Garment | Nidhoggur/Valfreyja (WoE 2) | 15% damage reduction (all elements), FLEE +15, MDEF +2 |
| **Brynhild** | Armor | Nidhoggur/Valfreyja (WoE 2) | Max HP +20%, Max SP +20%, MDEF +10, All Stats +10 |

### Material Drop Rates

- God Item materials drop from Treasure Boxes at approximately **0.4% (1/250)** per box.
- Each God Item requires **multiple unique materials** from different castles' treasure pools.
- Realistically, crafting one God Item takes **months of continuous castle ownership** with high Commerce investment.

---

## 11. Implementation Checklist

### Phase 1: Core Infrastructure (Required)

- [ ] **Guild System** -- Create/join/leave guild, guild skills, positions, tax, alliances, emblem
- [ ] **Guild Skills** -- Official Guild Approval, Kafra Contract, Guardian Research, Strengthen Guardian, Guild Extension, Guild's Glory, Battle Orders, Regeneration, Restore, Emergency Call, Charge Shout Flag/Beating
- [ ] **Guild Database** -- `guilds`, `guild_members`, `guild_positions`, `guild_skills`, `guild_alliances`, `guild_storage` tables
- [ ] **Castle Database** -- `woe_castles` table with all 20 castles seeded
- [ ] **Zone flags** -- Add `woe`, `woeSE`, `guildZone`, `noKnockback`, `noDuel` to zone registry

### Phase 2: WoE Scheduler

- [ ] **WoE Scheduler** -- `setInterval` check, configurable schedule, PRE_WOE/ACTIVE/POST_WOE/INACTIVE state machine
- [ ] **Server-wide announcements** -- `woe:announcement`, `woe:start`, `woe:end`, `woe:castle_captured`
- [ ] **Castle state loading** -- Load from DB at PRE_WOE, persist at POST_WOE
- [ ] **Warp non-owners** -- On WoE start and end, warp all non-owning guild members out of castles

### Phase 3: Emperium and Castle Conquest

- [ ] **Emperium entity** -- Spawn per castle with correct stats (HP based on defense investment)
- [ ] **Emperium damage rules** -- Only auto-attacks, skill miss, Holy immunity, guild approval check
- [ ] **Castle conquest** -- On Emperium break: ownership change, warp non-allies, respawn Emperium, server-wide announcement
- [ ] **Multiple breaks per session** -- Allow re-entry and re-break during active WoE
- [ ] **Emperium HP broadcast** -- `woe:emperium_hp` events for health bar display

### Phase 4: Combat Modifications

- [ ] **Skill damage reduction** -- -40% for all skill damage in WoE zones
- [ ] **Long-range reduction** -- -25% for long-range normal attacks
- [ ] **Flee reduction** -- -20% for all players in WoE zones
- [ ] **Trap duration** -- 4x multiplier in WoE zones
- [ ] **Knockback disabled** -- No displacement from knockback skills
- [ ] **Disabled skills** -- Block Teleport, Warp Portal, Ice Wall, Basilica, Assumptio, etc.
- [ ] **Cast interruption** -- Phen Card effects disabled in WoE
- [ ] **Portal invulnerability** -- 5 seconds on room transition

### Phase 5: Guardian System

- [ ] **Guardian NPC** -- Castle NPC for Guild Master to hire guardians
- [ ] **3 guardian types** -- Soldier (stun), Knight (melee brute), Archer (ranged)
- [ ] **Guardian AI** -- Chase, attack, leash behavior (reuse enemy AI system)
- [ ] **Guardian stats** -- Base stats + Strengthen Guardian bonuses + Defense investment bonuses
- [ ] **Guardian death/respawn** -- 30s cooldown before re-hire
- [ ] **Guardian ownership change** -- Despawn old, spawn new on castle capture

### Phase 6: Castle Economy

- [ ] **Investment NPCs** -- Commerce and Defense investment handlers
- [ ] **Investment limits** -- 2/day per type, 2x cost scaling
- [ ] **Absolute Develop** -- 50% bonus economy point chance
- [ ] **Daily reset** -- Investment counters reset at midnight
- [ ] **Treasure box generation** -- Daily at midnight, 4 base + Commerce bonus
- [ ] **Treasure Room NPC** -- Guild Master access only
- [ ] **Treasure loot tables** -- Per-realm loot tables with God Item material chance (0.4%)

### Phase 7: Castle Flags and Navigation

- [ ] **Flag NPCs** -- Exterior and interior flags per castle
- [ ] **Flag teleportation** -- Defenders only, numbered destinations
- [ ] **Castle room portals** -- Multi-room navigation with 5s invulnerability

### Phase 8: Guild Dungeons

- [ ] **Guild Dungeon access NPC** -- Inside each castle
- [ ] **Dungeon zones** -- 3 floors per realm (12 total zone definitions)
- [ ] **Dungeon monsters** -- Realm-specific monster spawns with MVPs
- [ ] **GvG on Floor 3** -- Hall of Abyss PvP-enabled
- [ ] **Shared dungeon** -- All castles in a realm share the same dungeon

### Phase 9: Client-Side

- [ ] **WoESubsystem** -- UWorldSubsystem for WoE state, castle info, Emperium HP
- [ ] **WoE Status Widget** -- Timer, castle list, ownership display
- [ ] **Emperium HP Bar** -- Health bar for the Emperium entity (visible to attackers)
- [ ] **Castle Map Widget** -- Shows current castle room and layout
- [ ] **WoE announcements** -- Chat messages for start/end/capture events
- [ ] **Guardian actors** -- Reuse enemy actor system for guardian NPCs
- [ ] **Flag interaction** -- Click-to-warp NPC interaction

### Phase 10: WoE 2 Extensions (Optional)

- [ ] **Guardian Stones** -- 2 per castle, destructible, 8-min repair cooldown
- [ ] **Barricades** -- 3 sets per castle, 450k HP blocks, installation cost
- [ ] **WoE 2 guardians** -- 5x stronger, Sword Guardian type
- [ ] **12 Link Flags** -- Interior warp network for defenders
- [ ] **WoE 2 castles** -- 10 additional castles in Nidhoggur/Valfreyja realms

### Phase 11: God Items (Optional, Late-Game)

- [ ] **Seal Quest NPCs** -- Server-wide progression tracking
- [ ] **God Item materials** -- Treasure Box drop integration
- [ ] **Grunburti NPC** -- Crafting interface in guild dungeons
- [ ] **God Item stats** -- 6 items with unique properties
- [ ] **Server-wide seal tracking** -- 50-player threshold, 100-player cap

---

## 12. Gap Analysis

### What the existing project docs cover well:

- **08_PvP_Guild_WoE.md**: Complete guild system (creation, skills, positions, tax, alliances, storage, emblem), PvP system (arenas, modes, point system, duel), WoE 1 overview (castles, Emperium, guardians, treasure), WoE 2 overview, Battlegrounds, God Items, full database schema.
- **Implementation/11_PvP_WoE.md**: PvP server architecture (damage, rankings, duels, invulnerability), WoE scheduler (state machine, castle loading/saving), Emperium damage + break logic, castle economy (investment handler), guardian spawning, treasure generation, WoE combat rules, Guardian Stone + Barricade (WoE 2), WoESubsystem client header.

### Gaps and corrections identified through deep research:

| # | Gap/Correction | Severity | Detail |
|---|---------------|----------|--------|
| G1 | **Emperium HP discrepancy** | Medium | Existing docs say 68,430 HP base. rAthena mob_db says 100 HP. The 68,430 comes from castle config, not mob_db. Both docs should clarify this is castle-config HP, not monster HP. |
| G2 | **Emperium DEF values differ** | Low | Existing doc says "DEF 40 + 80". rAthena pre-re mob_db shows DEF 64 + 43 (soft + hard). Implementation should use rAthena values. |
| G3 | **Emperium MDEF values differ** | Low | Existing doc says "MDEF 100 + 90". rAthena pre-re mob_db shows MDEF 50 + 47. Implementation should use rAthena values. |
| G4 | **Emperium Flee discrepancy** | Low | Existing doc says Flee 107. rAthena mob_db shows Flee 207. Use 207. |
| G5 | **Guardian types incomplete** | Medium | Existing docs mention "melee knight and ranged archer" (2 types). RO Classic actually has 3 types: Soldier (1287, stun), Knight (1286, brute), Archer (1285, ranged). All three should be implemented. |
| G6 | **Guardian skills missing** | Medium | Existing docs list no guardian skills. Soldier has Bash Lv10 + Stun Attack Lv5. Archer has Arrow Shower Lv10 + Double Strafe. These need implementation. |
| G7 | **Portal invulnerability missing** | Medium | 5 seconds of invulnerability on portal transition is a critical WoE mechanic not mentioned in existing implementation docs. |
| G8 | **Phen Card suppression missing** | Low | Cast-interrupt immunity from Phen Card (and similar) is suppressed in WoE. Not mentioned in existing docs. |
| G9 | **Endure suppression missing** | Low | Endure skill flinch immunity is suppressed in WoE. Not in existing docs. |
| G10 | **Alliance restrictions in castle** | Low | Cannot break alliances or expel guild members while inside a castle. Not in implementation docs. |
| G11 | **Guild dungeon GvG floor** | Medium | Floor 3 (Hall of Abyss) is GvG-enabled. Existing docs mention guild dungeons but not the PvP floor. |
| G12 | **Guild dungeon EXP loss** | Low | Death in guild dungeons causes normal EXP loss (unlike WoE castle maps). Not explicitly stated in implementation docs. |
| G13 | **Autocast exception** | Low | Autocast equipment CAN trigger disabled skills inside castles (but skills still miss Emperium). Not mentioned. |
| G14 | **Treasure Box types** | Low | Each castle has 2 types (common + castle-specific). Existing docs say "2 types" but implementation's `rollTreasureBoxLoot()` doesn't distinguish. |
| G15 | **WoE 2 guardian spawn intervals** | Medium | WoE 2 guardians spawn at 5/15/30/45/60 minute intervals from Guardian Stones. Not in implementation docs. |
| G16 | **Emergency Call dead/loading exclusion** | Low | Dead or loading-screen members are not teleported by Emergency Call. Not in implementation docs. |
| G17 | **Taekwon Emergency Call penalty** | Low | Guild Master with TK/Star Gladiator Leap skill has doubled Emergency Call cast time (5s to 10s). Edge case. |
| G18 | **Investment inherits on castle loss** | Low | New owner inherits Commerce/Defense levels. Stated in design doc but not explicitly in implementation code. |
| G19 | **God Item server-wide progression** | High | Seal Quest system (50-player threshold, 100-player cap) is not in any implementation code. Major feature for late-game. |
| G20 | **/mineffect auto-activation** | Low | Damage numbers automatically hidden in WoE. Client-side cosmetic feature. |
| G21 | **Shared guild skill cooldown** | Medium | Emergency Call, Battle Orders, Regeneration, Restore share a single 5-minute cooldown. Implementation must enforce shared cooldown across all 4 skills. |

### Priority Ranking for Implementation:

1. **Phase 1-3** (Critical): Guild system + WoE scheduler + Emperium + Castle conquest -- the minimum viable WoE.
2. **Phase 4** (High): Combat modifications -- WoE is unplayable without damage reductions, knockback disable, and skill restrictions.
3. **Phase 5** (High): Guardian system -- essential for castle defense.
4. **Phase 6** (Medium): Castle economy -- treasure boxes are the primary reward loop.
5. **Phase 7** (Medium): Flags and navigation -- important for defender strategy.
6. **Phase 8** (Medium): Guild dungeons -- secondary reward loop, 24/7 content.
7. **Phase 9** (High): Client-side UI -- players need to see WoE state and Emperium HP.
8. **Phase 10** (Low): WoE 2 -- separate castle set with different mechanics. Can be deferred.
9. **Phase 11** (Low): God Items -- late-game content requiring months of WoE ownership. Defer until WoE is stable.

---

## Sources

- [iRO Wiki Classic - War of Emperium](https://irowiki.org/classic/War_of_Emperium)
- [iRO Wiki - War of Emperium](https://irowiki.org/wiki/War_of_Emperium)
- [iRO Wiki Classic - War of Emperium 2](https://irowiki.org/classic/War_of_Emperium_2)
- [iRO Wiki Classic - Castle Treasure Drops](https://irowiki.org/classic/Castle_Treasure_Drops)
- [iRO Wiki - God Items Quest](https://irowiki.org/wiki/God_Items_Quest)
- [iRO Wiki Classic - War of Emperium for Newbies](https://irowiki.org/classic/War_of_Emperium_for_Newbies)
- [Ragnarok Fandom Wiki - War of Emperium](https://ragnarok.fandom.com/wiki/War_of_Emperium)
- [RateMyServer - Emperium (Pre-Re Monster #1288)](https://ratemyserver.net/index.php?page=mob_db&mob_id=1288)
- [RateMyServer - WoE 2.0 Introduction Guide](https://write.ratemyserver.net/ragnoark-online-character-guides/ragnoark-online-character-woe-guides/an-introduction-to-woe-second-edition-woe-20/)
- [RateMyServer - Paradox WoE Guide](https://write.ratemyserver.net/ragnoark-online-character-guides/ragnoark-online-character-woe-guides/paradox-war-of-emperium-guide/)
- [RateMyServer - Guild Dungeon Maps](https://ratemyserver.net/index.php?page=areainfo&area=4999)
- [divine-pride - Emperium Monster](https://www.divine-pride.net/database/monster/1288/emperium)
- [rAthena GitHub - guild.conf](https://github.com/rathena/rathena/blob/master/conf/battle/guild.conf)
- [rAthena GitHub - agit_main.txt](https://github.com/rathena/rathena/blob/master/npc/guild/agit_main.txt)
- [rAthena GitHub - skill_nocast_db.txt](https://github.com/rathena/rathena/blob/master/db/re/skill_nocast_db.txt)
- [rAthena Wiki - War of Emperium](https://github.com/rathena/rathena/wiki/War_of_Emperium)
- [Neoseeker - WoE Guide v1.1](https://www.neoseeker.com/ragnarok-online/faqs/139851-war-of-emperium.html)
- [GameFAQs - War of Emperium Guide by Arctic_Breeze](https://gamefaqs.gamespot.com/pc/561051-ragnarok-online/faqs/28786)
- [StrategyWiki - Ragnarok Online / War of Emperium](https://strategywiki.org/wiki/Ragnarok_Online/War_of_Emperium)
- [ROGGH Library - Guild Dungeon Access](https://roggh.com/roggh-guild-dungeon-access-and-woe-exclusive-gears/)
- [iRO Wiki - Urgent Call (Guild Skill)](https://irowiki.org/wiki/Urgent_Call)
- [RateMyServer - Guild Skills](https://ratemyserver.net/index.php?page=skill_db&jid=10000)
- [RODE2 - WoE:SE Mechanics](http://rode2.doddlercon.com/guides/ep113.php?id=9)
- [iRO Wiki - Guild Dungeon Investment System](https://irowiki.org/wiki/Guild_Dungeon_Investment_System)
- [Ragnarok Project Zero Wiki - Castle Economy](https://wiki.playragnarokzero.com/wiki/Castle_Economy)
