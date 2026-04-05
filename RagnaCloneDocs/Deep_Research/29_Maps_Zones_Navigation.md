# Maps, Zones & Navigation -- Deep Research (Pre-Renewal)

> Comprehensive reference for the Ragnarok Online Classic (pre-renewal) world map system, zone architecture, map properties, navigation mechanics, and movement systems.
> Sources: rAthena source (mapflags.txt, map.cpp, path.cpp, unit.cpp, npc/warps/, npc/kafras/), iRO Wiki Classic, RateMyServer pre-renewal database, Ragnarok Research Lab, Ragnarok Fandom Wiki, WarpPortal Forums, rAthena GitHub Wiki.

---

## Table of Contents

1. [World Structure](#1-world-structure)
2. [Map Properties / Flags](#2-map-properties--flags)
3. [Navigation & Warping](#3-navigation--warping)
4. [Movement System](#4-movement-system)
5. [Save Points](#5-save-points)
6. [Day/Night Cycle](#6-daynight-cycle)
7. [Weather Effects](#7-weather-effects)
8. [Implementation Checklist](#8-implementation-checklist)
9. [Gap Analysis](#9-gap-analysis)

---

## 1. World Structure

### 1.1 Continental Overview

The world of Ragnarok Online is set on the continent of **Midgard**, divided into three major nations and several outlying islands. Pre-renewal content spans Episodes 1 through 13.2.

| Nation | Capital | Theme | Episode Range |
|--------|---------|-------|---------------|
| **Rune-Midgarts Kingdom** | Prontera | Medieval European, largest nation | Ep 1-6 |
| **Republic of Schwartzvald** | Juno (Yuno) | Industrialized, steampunk | Ep 7-11 |
| **Arunafeltz States** | Rachel | Theocratic, frozen/volcanic | Ep 11.2-11.3 |
| **Outlying Islands** | (various) | Cultural themed (Japan, China, etc.) | Ep 4+ |

---

### 1.2 Towns (All Major Towns)

#### Rune-Midgarts Kingdom (Southern Continent)

| Town | Map ID | Theme | Key Services | Episode |
|------|--------|-------|-------------- |---------|
| **Prontera** | `prontera` | Medieval capital, white stone castle | Kafra, Job Change (Acolyte/Knight/Crusader/Priest/Monk), Arena, Tool/Weapon/Armor Dealers, Refiner, Stylist, Pet Groomer | Ep 1 |
| **Geffen** | `geffen` | Magic city, great tower | Kafra, Job Change (Mage/Wizard/Sage), Geffen Tower dungeon entrance | Ep 1 |
| **Payon** | `payon` | Korean/East Asian village, bamboo forest | Kafra, Job Change (Archer via Archer Village), Payon Cave dungeon | Ep 1 |
| **Alberta** | `alberta` | Maritime port town | Kafra, Job Change (Merchant), Ship Captain (overseas travel) | Ep 1 |
| **Morroc** | `morocc` | Desert oasis, Arabian architecture | Kafra, Job Change (Thief via moc_fild11), Pyramid/Sphinx access | Ep 1 |
| **Izlude** | `izlude` | Satellite town of Prontera | Job Change (Swordsman), Arena, Ship (Byalan Island), International Airship | Ep 1 |
| **Al De Baran** | `aldebaran` | Clockwork city, canals | Kafra HQ, Job Change (Alchemist), Clock Tower dungeon, Warp to Lutie | Ep 4 |
| **Comodo** | `comodo` | Underground cave city, fireworks | Kafra, Job Change (Bard/Dancer), Beach Dungeon | Ep 5 |
| **Umbala** | `umbala` | Tribal tree-village, jungle | Kafra, Yggdrasil Tree (leads to Niflheim) | Ep 6 |

#### Special Towns (Rune-Midgarts)

| Town | Map ID | Theme | Key Services | Episode |
|------|--------|-------|-------------- |---------|
| **Niflheim** | `niflheim` | Norse underworld, realm of the dead | Ghostly Kafra (limited), Dead Branch Crafter. **No save point.** | Ep 6 |
| **Lutie/Xmas** | `xmas` | Christmas village, perpetual snow | Kafra, Santa Claus, Toy Factory dungeon | Ep 3 |
| **Archer Village** | `pay_arche` | Sub-area north of Payon | Archer Guild, Payon Dungeon entrance | Ep 1 |
| **Orc Village** | (gef_fild10 area) | Orc settlement | Orc Dungeon entrance | Ep 1 |

#### Republic of Schwartzvald (Northeast Continent)

| Town | Map ID | Theme | Key Services | Episode |
|------|--------|-------|-------------- |---------|
| **Juno/Yuno** | `yuno` | Floating island city above clouds | Kafra, Job Change (Sage), Airship terminals (Domestic + International), Juperos access | Ep 7 |
| **Einbroch** | `einbroch` | Heavy industrial city, pollution | Kafra, Job Change (Gunslinger), Airship terminal, Einbech Mine | Ep 8 |
| **Lighthalzen** | `lighthalzen` | Commerce city, Rekenber Corp HQ | Kafra, Bio Lab dungeon (quest access), Airship terminal, Slums district | Ep 9 |
| **Hugel** | `hugel` | Garden coastal village | Cool Event Corp NPC, Airship terminal, Odin Temple/Abyss Lake/Thanatos Tower access | Ep 10 |
| **Einbech** | `einbech` | Mining village (sister of Einbroch) | Mine dungeon entrance | Ep 8 |

#### Arunafeltz States (Northwest Continent)

| Town | Map ID | Theme | Key Services | Episode |
|------|--------|-------|-------------- |---------|
| **Rachel** | `rachel` | Theocratic holy city, frozen tundra | Kafra, International Airship, Rachel Sanctuary/Ice Cave, Temple of Freya | Ep 11.2 |
| **Veins** | `veins` | Volcanic desert town | Kafra, Ship to Nameless Island, Thor's Volcano access | Ep 11.3 |

#### Outlying Islands (Foreign Lands)

| Town | Map ID | Theme | Access | Episode |
|------|--------|-------|--------|---------|
| **Amatsu** | `amatsu` | Feudal Japan, sakura | Ship from Alberta (10,000z round trip) | Ep 4 |
| **Louyang** | `louyang` | Ancient China, pagodas | Ship from Alberta (10,000z round trip) | Ep 4 |
| **Ayothaya** | `ayothaya` | Ancient Thailand | Ship from Alberta | Ep 7 |
| **Jawaii** | `jawaii` | Honeymoon Island (Hawaii) | Ship from Alberta (married couples only for full access) | Ep 5 |
| **Moscovia** | `moscovia` | Medieval Russia | Ship from Alberta | Ep 12 |
| **Turtle Island** | (tur_dun area) | Tropical island | Ship from Alberta (quest required) | Ep 4 |
| **Gonryun/Kunlun** | `gonryun` | Floating island, Chinese mythology | Warp from Louyang field | Ep 4 |
| **Nameless Island** | (abbey area) | Cursed monastery | Ship from Veins (quest required) | Ep 11.3 |

**Total towns: ~25 distinct town maps**

---

### 1.3 Field Maps (Connecting Areas Between Towns)

Field maps are outdoor zones connecting towns and dungeon entrances. They contain monster spawns, NPC warp portals, and occasionally quest NPCs. There are approximately **180 field maps** in pre-renewal RO.

#### Prontera Fields (Starter Area)

| Map ID | Level Range | Key Monsters | Connections |
|--------|-------------|--------------|-------------|
| `prt_fild00` | 1-15 | Poring, Lunatic, Fabre | Prontera (north) |
| `prt_fild01` | 1-15 | Poring, Lunatic, Fabre, Pupa | Prontera (north), Mjolnir fields |
| `prt_fild02` | 5-20 | Willow, Poring, Roda Frog | Mt. Mjolnir, Geffen fields |
| `prt_fild03` | 10-25 | Chonchon, Condor, Willow | Prontera, Alberta direction |
| `prt_fild04` | 10-25 | Chonchon, Rocker, Willow | Prontera (east), Payon direction |
| `prt_fild05` | 5-20 | Poring, Drops, Chonchon | Prontera (west), Geffen direction |
| `prt_fild06` | 10-20 | Savage Babe, Rocker | South fields |
| `prt_fild07` | 5-15 | Poring, Fabre, Lunatic | prt_fild08, Prontera |
| `prt_fild08` | 1-10 | Poring (70), Lunatic (40), Pupa (20), Drops (10) | Prontera (south), Izlude, moc_fild01 |

**Design note:** `prt_fild08` is the canonical "first field" with the highest Poring density, making it the starting experience for all new characters.

#### Geffen Fields

| Map ID | Level Range | Key Monsters | Connections |
|--------|-------------|--------------|-------------|
| `gef_fild00` | 15-30 | Poison Spore, Poporing | Geffen (north), Mjolnir |
| `gef_fild01` | 15-30 | Creamy, Smokie | Geffen west |
| `gef_fild03` | 20-40 | Side Winder, Mantis | Mid-level zone |
| `gef_fild04` | 10-25 | Chonchon, Rocker | Prontera direction |
| `gef_fild06` | 20-40 | Wolf, Goblin, Kobold | Orc Village direction |
| `gef_fild07` | 15-30 | Poison Spore, Smokie | Geffen (south) |
| `gef_fild10` | 30-55 | Orc Warrior, Orc Lady, Orc Baby | Popular AoE leveling spot, Orc Village |
| `gef_fild14` | 40-60 | Petit (Sky), Penomena | High-level Geffen area |

#### Payon Fields / Forests

| Map ID | Level Range | Key Monsters | Connections |
|--------|-------------|--------------|-------------|
| `pay_fild01` | 10-25 | Willow, Spore | Payon (south), Alberta |
| `pay_fild02` | 15-30 | Elder Willow, Poporing | Payon, Archer Village |
| `pay_fild04` | 20-35 | Smokie, Bigfoot | Prontera fields |
| `pay_fild07` | 25-40 | Coco, Horn, Elder Willow | East Payon forests |
| `pay_fild09` | 30-45 | Elder Willow, Coco, Horn | Deep forest |
| `pay_fild10` | 35-50 | Munak, Bongun | Haunted forest (leads thematically to Payon Dungeon) |

#### Morroc / Sograt Desert Fields

| Map ID | Level Range | Key Monsters | Connections |
|--------|-------------|--------------|-------------|
| `moc_fild01` | 5-15 | Peco Peco, Condor | prt_fild08, Morroc direction |
| `moc_fild02` | 15-30 | Muka, Piere, Andre | Morroc (south), Ant Hell entrance |
| `moc_fild03` | 20-35 | Desert Wolf, Golem | Morroc (west), Sphinx direction |
| `moc_fild07` | 25-40 | Hode, Sand Man | Central desert |
| `moc_fild11` | 30-45 | Scorpion, Desert Wolf Baby | Pyramid area, Thief Guild |
| `moc_fild12` | 25-40 | Magnolia, Muka | Morroc (east) |
| `moc_fild16` | 35-50 | Sand Man, Hode, **Phreeoni(MVP)** | Deep desert |
| `moc_fild17` | 40-55 | Golem, Pasana | Sphinx approach |

#### Mt. Mjolnir Fields

| Map ID | Level Range | Key Monsters | Connections |
|--------|-------------|--------------|-------------|
| `mjolnir_01` | 15-30 | Creamy, Smokie | Geffen, Prontera fields |
| `mjolnir_02` | 20-35 | Smokie, Bigfoot | Mountain paths |
| `mjolnir_03` | 25-40 | Side Winder, Mantis | Mid-mountain |
| `mjolnir_04` | 30-45 | Dustiness, Flora, **Mistress(MVP)** | Mistress spawns here |
| `mjolnir_05` | 25-40 | Argos, Stem Worm | Mountain pass |
| `mjolnir_06` | 20-35 | Poison Spore, Smokie, Creamy | Forest mountain |
| `mjolnir_07` | 30-45 | Stainer, Savage | Northern mountain |
| `mjolnir_08` | 25-40 | Poporing, Magnolia | Coal Mine entrance |
| `mjolnir_09` | 20-35 | Caramel, Bigfoot | Al De Baran direction |
| `mjolnir_12` | 25-40 | Steel Chonchon, Skeleton Worker | Coal Mine surface |

#### Schwartzvald Fields (Higher Level)

| Region | Map IDs | Level Range | Key Monsters |
|--------|---------|-------------|--------------|
| Yuno Fields | `yuno_fild01-04`, `yuno_fild09` | 50-80 | Grand Peco, Sleeper, Goat, Harpy, Demon Pungus |
| Hugel Fields | `hu_fild01-07` | 55-80 | Metaling, Beetle King, Demon Pungus |
| Einbroch Fields | `ein_fild01-10` | 50-75 | Mineral, Geographer, Holden |
| Lighthalzen Fields | `lhz_fild01-03` | 60-85 | Various Metalings |

#### Arunafeltz Fields (Highest Level Outdoor)

| Region | Map IDs | Level Range | Key Monsters |
|--------|---------|-------------|--------------|
| Rachel Fields | `ra_fild01-13` | 70-90 | Siroma, Snowier, Ice Titan |
| Veins Fields | `ve_fild01-07` | 75-95 | Magmaring, Imp, Kasa |

#### Foreign Land Fields

| Region | Map IDs | Key Monsters |
|--------|---------|--------------|
| Amatsu Fields | `ama_fild01` | Kapha, Miyabi Ningyo |
| Louyang Fields | `lou_fild01` | Mi Gao, Green Maiden |
| Ayothaya Fields | `ayo_fild01-02` | Kraben, Leaf Cat |
| Niflheim Fields | `nif_fild01-02` | High-level undead/demon |
| Moscovia Fields | `mosk_fild01-02` | Wood Goblin, Les |

**Design principles for field maps:**
- Fields get progressively harder as you move further from the starting town
- Each town has 2-8 surrounding field maps forming a regional cluster
- Field maps always connect to adjacent field maps or towns via edge-of-map warp portals
- Some fields contain field MVPs (Phreeoni, Mistress, Golden Bug, etc.)
- Fields are always outdoor (never have `indoor` flag)
- Fields typically have `nosave` but NOT `noteleport` (Fly Wing works)

---

### 1.4 Dungeons (Multi-Floor, Entrance Locations)

Dungeons are underground or enclosed combat zones with higher monster density, stronger enemies, and MVPs. There are approximately **150+ dungeon floor maps** in pre-renewal RO.

#### Rune-Midgarts Dungeons

| Dungeon | Map IDs | Floors | Location | Level Range | MVP | Flags |
|---------|---------|--------|----------|-------------|-----|-------|
| **Prontera Culvert** | `prt_sewb1`-`prt_sewb4` | 4 | Below Prontera (Knight Guild) | 15-50 | Golden Thief Bug (F4) | noteleport, noreturn, nosave, indoor |
| **Payon Dungeon** | `pay_dun00`-`pay_dun04` | 5 | North of Payon via Archer Village | 15-75 | Moonlight Flower (F5) | nosave, indoor |
| **Geffen Tower** | `gef_dun00`-`gef_dun03` | 4 | Center of Geffen (basement) | 20-70 | Doppelganger (F3) | nosave, indoor |
| **Orc Dungeon** | `orcsdun01`-`orcsdun02` | 2 | West of Geffen (gef_fild10) | 25-55 | Orc Lord (F2) | nosave, indoor |
| **Byalan Island** | `iz_dun00`-`iz_dun05` | 6 | Ship from Izlude | 15-80 | Kraken (F6) | nosave |
| **Sunken Ship** | `treasure01`-`treasure02` | 2 | Alberta fields | 30-60 | Drake (F2) | nosave, indoor |
| **Ant Hell** | `anthell01`-`anthell02` | 2 | South of Morroc | 15-40 | Maya (F2) | nosave, indoor |
| **Pyramids** | `moc_pryd01`-`moc_pryd06` | 4+2 | Southeast of Morroc | 30-75 | Osiris (B2) | nosave, indoor |
| **Sphinx** | `in_sphinx1`-`in_sphinx5` | 5 | West of Morroc | 30-80 | Pharaoh (B5) | nosave, indoor |
| **Clock Tower** | `c_tower1`-`c_tower4`, `alde_dun01`-`alde_dun04` | 4+4 | Center of Al De Baran | 40-90 | Clock Tower Manager (B4) | nosave, indoor |
| **Coal Mine** | `coal_mine01`-`coal_mine03` | 3 | Mt. Mjolnir area | 20-50 | None | nosave, indoor |
| **Toy Factory** | `xmas_dun01`-`xmas_dun02` | 2 | North of Lutie | 40-70 | Stormy Knight (F2) | nosave, indoor |
| **Comodo Beach Dungeon** | `beach_dun`, `beach_dun2`, `beach_dun3` | 3 | South of Comodo | 35-60 | None | nosave |
| **Umbala Dungeon** | (umbala dungeon maps) | 2 | Umbala jungle | 40-65 | None (leads to Niflheim) | nosave |
| **Turtle Island** | `tur_dun01`-`tur_dun04` | 4 | Ship from Alberta | 55-85 | Turtle General (F4) | nosave |

#### Glast Heim (Largest Dungeon Complex)

| Wing | Map IDs | Key Monsters | MVP/Mini-Boss |
|------|---------|-------------- |---------------|
| **Castle** | `gl_cas01`, `gl_cas02` | Raydric, Wanderer, Dark Frame | Dark Lord (Lv90, 8hr respawn) |
| **Churchyard** | `gl_church` | Evil Druid, Wraith, Mimic | None |
| **Chivalry** | `gl_knt01`, `gl_knt02` | Raydric, Khalitzburg, Abysmal Knight | Abysmal Knight, Bloody Knight (mini) |
| **St. Abbey** | `gl_sew01` + associated | Wraith, Evil Druid, Rybio, Injustice | None |
| **Underprison** | `gl_prison`, `gl_prison01` | Zombie/Skeleton Prisoner, Hunter Fly | None |
| **Culvert** | `gl_sew03`, `gl_sew04` | Gargoyle, Anolian, Sting | None |

**Design note:** Glast Heim spans levels 50-99 and is the single most iconic dungeon in RO. Every wing has distinct monster themes. Flags: nosave, indoor.

#### Schwartzvald Dungeons

| Dungeon | Map IDs | Floors | Level Range | MVP | Flags |
|---------|---------|--------|-------------|-----|-------|
| **Bio Laboratory** | `lhz_dun01`-`lhz_dun04` | 4 | 80-99+ | Transcendent class MVPs (F3/F4) | noteleport, noreturn, nosave, indoor |
| **Juperos Ruins** | `jupe_cave`, `jupe_area1-2`, `jupe_core` | 3+Core | 75-95 | Vesper (Core) | nosave, indoor |
| **Einbech Mine** | `ein_dun01`-`ein_dun02` | 2 | 65-90 | RSX-0806 (F2) | nosave, indoor |
| **Magma Dungeon** | `mag_dun01`-`mag_dun02` | 2 (pre-re) | 60-99+ | None | nosave |
| **Odin's Temple** | `odin_tem01`-`odin_tem03` | 3 | 80-99+ | Valkyrie Randgris (F3, 8hr) | nosave |
| **Abyss Lake** | `abyss_01`-`abyss_03` | 3 | 70-95 | Dragon MVP (F3) | nosave |
| **Thanatos Tower** | `tha_t01`-`tha_t12` | 12+ | 80-99+ | Thanatos (Top) | noteleport, noreturn, nosave, indoor |
| **Kiel Hyre Academy** | (kiel_dun maps) | 2 | 75-95 | Kiel-D-01 | nosave, indoor |
| **Nidhoggur's Dungeon** | (nyd_dun maps) | 2 | 80-99 | Nidhoggur's Shadow | nosave |

#### Arunafeltz Dungeons

| Dungeon | Map IDs | Floors | Level Range | MVP | Flags |
|---------|---------|--------|-------------|-----|-------|
| **Rachel Sanctuary (Ice Cave)** | `ice_dun01`-`ice_dun03` (+`ice_dun04`) | 3+boss | 70-95 | Ktullanux (summoned) | nosave |
| **Thor's Volcano** | `thor_v01`-`thor_v03` | 3 | 80-99+ | Ifrit (F3) | noteleport, noreturn, nosave |
| **Nameless Island Abbey** | `abbey01`-`abbey03` | 3 | 80-99+ | Fallen Bishop (F2), Beelzebub (F3) | noteleport, noreturn, nosave, indoor |

#### Foreign Land Dungeons

| Dungeon | Map IDs | Floors | Level Range | MVP |
|---------|---------|--------|-------------|-----|
| **Amatsu Dungeon** | (ama_dun maps) | 3 | 50-80 | Incantation Samurai |
| **Louyang Royal Tomb** | (lou_dun maps) | 3 | 50-85 | White Lady (Bacsojin) |
| **Ayothaya Ancient Shrine** | (ayo_dun maps) | 2 | 55-80 | Lady Tanee |
| **Moscovia Forest** | (mosk_dun maps) | 3 | 55-85 | None (Baba Yaga mini) |
| **Gonryun Dungeon** | (gon_dun maps) | 3 | 50-80 | Evil Snake Lord |

**Dungeon design principles:**
- Most dungeons get progressively harder per floor
- MVP floors are typically the deepest/last floor
- Boss rooms often have `noteleport` + `noreturn` flags (prevents easy escape)
- Dungeon floors always have `nosave` (cannot set save point)
- Most dungeons are `indoor` (affects mount dismounting on some servers, weather suppression)
- Some dungeons require quest access (Bio Lab, Thanatos Tower, Nameless Island)

---

### 1.5 Special Maps

#### PvP Maps

| Map Type | Map IDs | Description | Flags |
|----------|---------|-------------|-------|
| **PvP Arenas** | `pvp_y_*`, `pvp_n_*` | Open PvP combat | pvp, nopenalty, noreturn |
| **Prontera Arena** | `arena_room` | Arena lobby/waiting room | town |
| **PvP Rooms** | `pvp_room` | PvP matchmaking area | town |

- PvP arenas have `nopenalty` flag (no EXP/Zeny loss on death)
- Players are automatically hostile to all non-party members
- Some arenas have level-restricted entry (e.g., Lv50-60 only)

#### War of Emperium (GvG) Maps

| Map Type | Map IDs | Description | Flags |
|----------|---------|-------------|-------|
| **Agit Castles (Rune-Midgarts)** | `prtg_cas01`-`05`, `payg_cas01`-`05`, `gefg_cas01`-`05`, `aldeg_cas01`-`05` | WoE 1 castle maps | gvg, gvg_castle, noreturn |
| **Agit2 Castles (Schwartzvald)** | `schg_cas01`-`05`, `arug_cas01`-`05` | WoE 2 castle maps | gvg, gvg_castle, noreturn |
| **Castle Dungeons** | Various `*_cas_dun*` | Treasure room dungeons | gvg_dungeon |

- WoE castles have special rules: reduced damage, no instant-death skills, Emperium target
- Castle ownership determined by destroying the Emperium
- Castle treasures spawn in associated dungeon maps

#### Instance Maps

| Instance | Floors | Cooldown | Party Required | Level Range |
|----------|--------|----------|----------------|-------------|
| **Endless Tower** | 100 | 6d 20h | Yes | 50-99+ |
| **Nidhoggur's Nest** | (varies) | Several days | Yes | 80-99+ |
| **Sealed Shrine** | (varies) | Several days | Yes | 80-99+ |

- Instance maps create a private copy for each party
- Cannot add/remove party members during instance
- Time limit (typically 4 hours) to complete
- Automatic warp to save point when time expires
- Flags: noteleport, noreturn, nosave, nowarpto, nowarp

#### Other Special Maps

| Map | Description |
|-----|-------------|
| **Airship** | Moving vehicle between towns (boarding + riding) |
| **Wedding Chapel** | Marriage ceremony location |
| **Jail/Prison** | Bot/cheater confinement |
| **Tutorial Area** | New player introduction zone |

---

### 1.6 Inter-Continental Travel Routes

#### Airship System

| Route | Type | Stops | Notes |
|-------|------|-------|-------|
| **Domestic Airship** | Schwartzvald loop | Hugel -> Juno -> Einbroch -> Lighthalzen (loops) | Free, NPC-operated |
| **International Airship** | Cross-continental | Izlude -> Juno -> Rachel (loops) | Free, NPC-operated |

Airships are moving maps that physically travel between towns. Players board and ride until reaching their destination, then disembark. Not instant teleportation.

#### Ship Routes (From Alberta)

| Destination | Cost | Quest Required |
|-------------|------|----------------|
| Amatsu | 10,000z (round trip) | No |
| Louyang | 10,000z (round trip) | No |
| Ayothaya | (varies) | No |
| Jawaii | (varies) | Marriage required for full access |
| Turtle Island | (varies) | Quest required |
| Moscovia | (varies) | No |

#### Other Ship Routes

| From | To | Cost |
|------|----|------|
| Izlude | Byalan Island | 500z |
| Veins | Nameless Island | Quest required |

---

## 2. Map Properties / Flags

### 2.1 Complete Map Flag List (rAthena Standard)

Map flags control what actions are allowed on each map. Based on the rAthena server emulator standard (`doc/mapflags.txt` and `npc/mapflag/` directory).

#### Core Type Flags

| Flag | Description | Effect |
|------|-------------|--------|
| `town` | Safe zone designation | No monster spawns. Currently also used to check which maps allow Mail Inbox access. |
| `pvp` | Player vs Player mode | All guilds/players are hostile except party members. Party alliance configurable. |
| `pvp_noparty` | PvP with no party protection | Even party members can attack each other |
| `pvp_noguild` | PvP with no guild protection | Even guild members can attack each other |
| `pvp_nightmaredrop` | PvP with item drop on death | Equipped items and inventory items can drop when killed |
| `pvp_nocalcrank` | PvP without ranking | Kills/deaths not tracked for PvP ranking |
| `gvg` | Guild vs Guild mode | All guilds hostile, guild icons shown, WoE damage reduction applies |
| `gvg_castle` | WoE castle map | Special WoE rules (Emperium target, treasure spawning) |
| `gvg_dungeon` | WoE dungeon map | Castle-associated dungeon |
| `gvg_noparty` | GvG with no party protection | Party members in different guilds can attack each other |
| `battleground` | Battleground mode | Battleground team-based combat |

#### Teleportation / Movement Restriction Flags

| Flag | Description | Blocks |
|------|-------------|--------|
| `noteleport` | No teleporting | Fly Wing, Teleport skill (Acolyte Lv1 random), @jump. **Does NOT block Butterfly Wing.** |
| `noreturn` | No return to save | Butterfly Wing, @return, Teleport Lv2 (save point). **Does NOT block Fly Wing.** |
| `nowarp` | No warping FROM this map | @go, @warp FROM this map (GM commands) |
| `nowarpto` | No warping TO this map | @warp TO this map, marriage warp skills |
| `nomemo` | No memo point | /memo command blocked, Warp Portal memory blocked, marriage warp skills blocked |
| `nobranch` | No Dead Branch use | Dead Branch, Bloody Branch, Poring Box summoning blocked |
| `nojobexp` | No job EXP | Prevents gaining job EXP from monsters on this map |
| `nobaseexp` | No base EXP | Prevents gaining base EXP from monsters on this map |
| `nomobloot` | No monster drops | Prevents monsters from dropping items on this map |
| `nomvploot` | No MVP drops | Prevents MVP bonus drops on this map |

#### Save / Respawn Flags

| Flag | Description | Effect |
|------|-------------|--------|
| `nosave` | No save point setting | Cannot set respawn point here. If char logs off on this map, respawns at specified redirect map or last save. Format: `nosave SavePoint` or `nosave <map>,<x>,<y>` |
| `nopenalty` | No death penalty | No EXP or Zeny loss on death |
| `noexppenalty` | No EXP death penalty | No EXP loss on death (Zeny penalty still applies) |
| `nozenypenalty` | No Zeny death penalty | No Zeny loss on death (EXP penalty still applies) |

#### Combat / Skill Flags

| Flag | Description | Effect |
|------|-------------|--------|
| `noskill` | All skills disabled | No skills can be used. GM exempt. |
| `restricted` | Zone-based restrictions | Certain items/skills restricted based on zone number (1-9). Uses `item_noequip.txt` and `skill_nocast_db.txt` |
| `noicewall` | No Ice Wall | Ice Wall skill blocked (prevents cheese strategies in boss rooms) |
| `monster_noteleport` | Monsters cannot teleport | Prevents monster teleportation (including via RG_INTIMIDATE) |
| `nocommand` | No @commands | Blocks all player @commands below a certain GM level |

#### Item / Trade Flags

| Flag | Description | Effect |
|------|-------------|--------|
| `nodrop` | No item dropping | Cannot drop items on the ground |
| `notrade` | No trading | Cannot open trade window |
| `novending` | No vending | Cannot open vending shop |
| `nochat` | No chat rooms | Cannot create chat rooms |

#### Environment / Visual Flags

| Flag | Description | Effect |
|------|-------------|--------|
| `indoor` | Indoor lighting | Suppresses weather effects. Affects Peco Peco mount (some implementations force dismount). |
| `clouds` | Cloud effect | Cloud weather particles displayed |
| `clouds2` | Dense clouds | Heavier cloud effect |
| `fog` | Fog effect | Fog weather particles displayed |
| `fireworks` | Fireworks effect | Firework particles displayed (Comodo) |
| `sakura` | Sakura petals | Falling sakura petal particles (Amatsu, Payon spring) |
| `leaves` | Falling leaves | Falling leaf particles (forests) |
| `snow` | Snowfall | Snow weather effect (Lutie, Rachel) |
| `rain` | Rainfall | Rain weather effect |
| `night` | Night-time | Night-time lighting override (Niflheim) |

#### Miscellaneous Flags

| Flag | Description |
|------|-------------|
| `loadevent` | Triggers `OnPCLoadMapEvent` NPC event when player enters map |
| `jexp` | Job EXP multiplier (percentage) |
| `bexp` | Base EXP multiplier (percentage) |
| `partylock` | Cannot add/remove party members on this map |
| `guildlock` | Cannot modify guild membership on this map |
| `reset` | Reset player status when entering the map |

---

### 2.2 Indoor vs Outdoor

The `indoor` flag has the following effects:

| Aspect | Indoor (`indoor` flag set) | Outdoor (no `indoor` flag) |
|--------|---------------------------|---------------------------|
| Weather effects | Suppressed (no rain, snow, etc.) | Active based on map weather flags |
| Lighting | Point lights, no directional sun | Directional sunlight + sky |
| Peco Peco mount | Some implementations force dismount | Always allowed |
| Map visual | Dark/enclosed atmosphere | Open sky, horizon |

**Typical indoor maps:** All dungeons, building interiors, some town indoor areas.
**Typical outdoor maps:** All fields, most towns, some dungeon entrances.

---

### 2.3 Common Flag Combinations By Map Type

```
TOWN:           town, nosave(redirect to self)
FIELD:          nosave (or no flags at all)
DUNGEON:        noteleport, noreturn, nosave, indoor
BOSS ROOM:      noteleport, noreturn, nosave, nowarpto, indoor, nobranch
PVP ARENA:      pvp, nopenalty, noreturn
GVG CASTLE:     gvg, gvg_castle, noreturn, nowarpto
GVG DUNGEON:    gvg_dungeon
INSTANCE:       noteleport, noreturn, nosave, nowarpto, nowarp, nomemo, nobranch, partylock
```

**Important distinctions:**
- `noteleport` blocks Fly Wing / Teleport Lv1 (random within map) but NOT Butterfly Wing
- `noreturn` blocks Butterfly Wing / Teleport Lv2 (return to save) but NOT Fly Wing
- Many dungeons set BOTH to prevent any form of instant escape
- Towns always redirect `nosave` to themselves (dying in Prontera respawns you in Prontera)
- Fields with `nosave` use `SavePoint` redirect (player respawns at their last saved town)

---

### 2.4 Map Type Distribution (Pre-Renewal)

| Type | Approximate Count | Description |
|------|-------------------|-------------|
| Town | ~25 | Safe zones with NPCs, Kafra, job change |
| Field | ~180 | Outdoor connecting zones with monster spawns |
| Dungeon | ~150+ | Underground/indoor combat zones (counted per floor) |
| PvP/GvG | ~30 | Player combat arenas and WoE castles |
| Instance | ~20 | Instanced dungeons (Endless Tower, Memorial Dungeons) |
| Other | ~15 | Airship, special areas, wedding chapel, jail |
| **Total** | **~420** | All discrete map files |

---

## 3. Navigation & Warping

### 3.1 Warp Portals (NPC Warps Between Maps)

NPC warp portals are invisible trigger zones placed at map edges and dungeon entrances that automatically transport players to connected maps.

**How NPC warps work:**
1. An invisible NPC warp trigger is defined in rAthena's `npc/warps/` directory
2. When a player walks onto the trigger cell, they are instantly warped to the destination
3. No confirmation dialog -- warping is instant on contact
4. All edge-of-map exits use this system

**rAthena warp definition format:**
```
<source_map>,<source_x>,<source_y>,0	warp	<warp_name>	<trigger_width>,<trigger_height>,<dest_map>,<dest_x>,<dest_y>
```

**Example:**
```
prontera,150,26,0	warp	prt001	2,2,prt_fild08,170,375
// Walking to (150,26) in Prontera warps you to (170,375) in prt_fild08
```

**Key characteristics:**
- Warp triggers are typically 2x2 or 4x4 cells wide
- Placed at map edges (north/south/east/west exits)
- Placed at dungeon entrances (cave mouths, stairwells, etc.)
- Bidirectional by convention (each map has a corresponding return warp)
- Some warps require quest completion (checked by NPC script, not the warp itself)
- Warp portals are NOT visual objects in classic RO (no swirling portal effect for NPC warps)

---

### 3.2 Kafra Teleport Service

Kafra Corporation NPCs are stationed in every major Rune-Midgarts town. The Republic of Schwartzvald uses the competing **Cool Event Corporation** for the same services. Kafra provides three core services:

1. **Teleport Service** -- Instant travel to predefined town destinations for a Zeny fee
2. **Storage Service** -- Access to personal storage (40 Zeny per access)
3. **Save Point Service** -- Set respawn point at current location (Free)

Additional services (town-dependent):
- **Cart Rental** -- For Merchant class (800 Zeny)
- **Kafra Points** -- 1 point per 10 Zeny spent, redeemable in Al De Baran

#### Complete Kafra Teleport Destinations (Pre-Renewal)

**Rune-Midgarts Kafra Corporation:**

| Kafra Location | Destinations | Cost |
|----------------|-------------|------|
| **Prontera** | Geffen, Payon, Morroc, Alberta, Izlude | 600-1,200z |
| **Geffen** | Prontera, Payon, Alberta, Morroc | 600-1,200z |
| **Payon** | Prontera, Alberta, Morroc | 1,200z each |
| **Alberta** | Prontera, Payon, Morroc | 600-1,200z |
| **Morroc** | Prontera, Alberta, Payon | 600-1,200z |
| **Al De Baran** | Prontera, Geffen, Izlude, Mt. Mjolnir Dead Pit | 1,200z each |
| **Izlude** | Prontera | 600z |
| **Comodo** | Prontera, Umbala | 1,200z |
| **Umbala** | Comodo, Prontera | 1,200z |
| **Lutie** | Al De Baran | 1,200z |
| **Niflheim** | (Limited ghostly services) | 1,200z |

**Republic of Schwartzvald (Cool Event Corporation + Kafra):**

| Location | Destinations | Cost |
|----------|-------------|------|
| **Juno/Yuno** | Al De Baran | 1,200z |
| **Einbroch** | Juno, Lighthalzen, Hugel (via airship) | 1,200z |
| **Lighthalzen** | Juno, Einbroch, Hugel (via airship) | 1,200z |
| **Hugel** | Juno, Einbroch, Lighthalzen (via airship) | 1,200z |

**Arunafeltz States:**

| Location | Destinations | Cost |
|----------|-------------|------|
| **Rachel** | Veins | 1,200z |
| **Veins** | Rachel | 1,200z |

**Special items:**
- **Free Ticket for Kafra Transportation** (Item ID 7060): Consumable that waives the Zeny cost for one teleport. Weight: 1. Dropped by various monsters.

---

### 3.3 Fly Wing (Random Teleport)

| Property | Value |
|----------|-------|
| **Item ID** | 601 |
| **Effect** | Teleport to random walkable location on current map |
| **Buy Price** | 60z |
| **Weight** | 5 |

**Rules:**
- Blocked by `noteleport` map flag (most dungeon floors)
- Works on all field maps and most town maps
- The Acolyte skill **Teleport Lv1** does the same thing without consuming an item
- Consumed on use (even if teleport succeeds or fails)
- Cannot control destination -- purely random within walkable cells
- **Giant Fly Wing** (Item ID 12212): Teleports the entire party to a random location

---

### 3.4 Butterfly Wing (Return to Save Point)

| Property | Value |
|----------|-------|
| **Item ID** | 602 |
| **Effect** | Teleport to save point (last Kafra save location) |
| **Buy Price** | 300z |
| **Weight** | 5 |

**Rules:**
- Blocked by `noreturn` map flag (many dungeons, boss rooms)
- The Acolyte skill **Teleport Lv2** does the same thing without consuming an item
- Consumed on use
- Returns to the exact map and coordinates of the player's save point
- **Colored Butterfly Wings**: Special items that teleport to specific towns (not save point)

---

### 3.5 Warp Portal Skill (Acolyte / Priest)

| Property | Value |
|----------|-------|
| **Skill ID** | AL_WARP (27) |
| **Class** | Acolyte, Priest, High Priest, Arch Bishop |
| **Max Level** | 4 |
| **Catalyst** | 1 Blue Gemstone per cast |
| **Portal Capacity** | 8 players per portal |
| **Max Active Portals** | 3 simultaneously |
| **Duration** | (level-dependent, ~15-25 seconds) |

**Level progression:**

| Level | Available Destinations |
|-------|-----------------------|
| 1 | Save Point only |
| 2 | Save Point + Memo 1 |
| 3 | Save Point + Memo 1 + Memo 2 |
| 4 | Save Point + Memo 1 + Memo 2 + Memo 3 |

**Memo System:**
- Type `/memo` in chat to memorize current map location as a warp point
- Maximum 3 memo slots
- New memos overwrite the oldest slot if all 3 are full
- Only 1 memo per unique map (re-memoing same map updates coordinates)
- Blocked by `nomemo` map flag (dungeons, instances)
- Memo stores both the map and exact (x,y) coordinates

**Restrictions:**
- Cannot cast on maps with `noteleport` flag
- Cannot memo on maps with `nomemo` flag
- Blue Gemstone consumed on cast (not on destination selection)
- Portal disappears after 8 players enter or duration expires
- Caster can enter their own portal

---

### 3.6 Dead Branch / Bloody Branch

These items summon random monsters rather than providing teleportation, but their use is regulated by map flags.

| Item | ID | Effect | Restriction |
|------|----|--------|-------------|
| **Dead Branch** | 604 | Summons 1 random non-MVP monster (from pool of ~463) | Blocked by `nobranch` flag |
| **Bloody Branch** | 12103 | Summons 1 random MVP monster | Blocked by `nobranch` flag |
| **Poring Box** | 12109 | Summons multiple Porings | Blocked by `nobranch` flag |

- Many servers restrict Dead/Bloody Branch use to specific "Branch Rooms"
- Most dungeon boss floors have `nobranch` to prevent stacking MVPs
- Towns typically do NOT have `nobranch` but custom servers often add it

---

### 3.7 Other Teleportation Methods

| Method | Description | Restrictions |
|--------|-------------|-------------|
| **Teleport skill (Acolyte)** | Lv1: random spot on current map. Lv2: return to save point. No gem cost. | `noteleport` blocks Lv1, `noreturn` blocks Lv2 |
| **Airship** | Automatic route between towns (boarding + riding) | Must wait for airship arrival/departure |
| **Ship routes** | NPC-operated, fixed destinations with Zeny cost | Specific NPCs in port towns |
| **@go command** | GM-only instant town teleport | Blocked by `nowarp` flag |
| **Marriage skills** | Recall partner to your location | Blocked by `nowarpto` on target map |
| **Al De Baran -> Lutie** | Special NPC warp (Santa Claus event) | Free |
| **Umbala -> Niflheim** | Through Yggdrasil Tree root | Must traverse Umbala Dungeon |

---

## 4. Movement System

### 4.1 Click-to-Move Mechanics

RO Classic uses a **cell-based, click-to-move** system:

1. Player left-clicks on the ground at a destination cell
2. Client determines destination cell coordinates (X, Y) from click position via isometric projection
3. Client sends `CZ_REQUEST_MOVE` packet (0x0437) with destination (X, Y) to server
4. Server calls `path_search()` using **A* algorithm** to validate path from current to destination
5. Server validates each cell via `CELL_CHKNOPASS` (walkability check)
6. If valid: server stores walkpath (array of direction values, max 32 steps)
7. Server moves unit one cell per `walkSpeed` ms along the path
8. Server sends `ZC_NOTIFY_PLAYERMOVE` to the moving client (walk confirmation)
9. Server sends `ZC_NOTIFY_MOVE` to all other clients in view range
10. Client plays walk animation toward destination

**Continuous movement (hold click):**
- Holding the left mouse button causes repeated walk requests toward cursor position
- Creates smooth "drag to move" behavior
- New walk requests sent when character reaches current destination or changes direction

**Path limitations:**
- `MAX_WALKPATH = 32` cells (paths longer than 32 cells are truncated)
- For very long distances, client must send multiple walk requests as character progresses
- Paths cannot cross non-walkable cells
- 8-directional movement (cardinal + diagonal)
- Server re-validates each step during walk

---

### 4.2 Cell System (Grid-Based World)

RO Classic uses an isometric 2D tile grid overlaid on 3D terrain:

**Cell Properties (from GAT/Ground Altitude Table files):**
- Each cell has integer (X, Y) coordinates
- Cells can be: walkable, non-walkable (walls/obstacles), water, shootable-through-but-not-walkable
- "Snipeable" cells allow ranged attacks to pass through but characters cannot walk on them
- Server checks `CELL_CHKNOPASS` before allowing movement

**Map sizes:**
- Maps range from ~100x100 to ~400x400 cells
- Prontera town: 312 x 392 cells
- 1 RO cell = 50 UE units in Sabri_MMO's implementation

**Diagonal movement:**
- Characters CAN move diagonally (8-directional)
- Chebyshev distance for cell counting (diagonal = same cell count as cardinal)
- Diagonal movement takes ~1.41x longer in real time (physical distance is longer)
- Moving 5 cells diagonally: ~1,123 ms vs 5 cells straight: ~827 ms (at 150ms/cell base)

---

### 4.3 Movement Speed

Movement speed is measured as **milliseconds per cell** (time to traverse one adjacent tile).

**Base Speed:**
- All player characters regardless of class or stats: **150 ms per cell** (~6.67 cells/second)
- **AGI does NOT affect movement speed** (AGI only affects ASPD and Flee)
- Movement speed is ONLY modified by specific skills, items, and status effects

**Complete Speed Modifier Table:**

| Source | Speed (ms/cell) | Multiplier | Stack Behavior | Notes |
|--------|----------------|------------|----------------|-------|
| Base (no modifier) | 150 | 1.0x | -- | All classes |
| Increase AGI | 110 | ~1.36x faster | Stacks with Peco, does NOT stack with Moonlight Flower Card | Acolyte skill |
| Peco Peco Ride | 110 | ~1.36x faster | Stacks with Inc AGI, does NOT stack with other speed items | Knight/Crusader |
| Inc AGI + Peco Ride | 70 | ~2.14x faster | These two stack additively | Combined |
| Speed Potion | 60 | ~2.5x faster | **Overrides ALL other speed effects** | 5 second duration |
| Cart Boost | 80 | ~1.88x faster | Whitesmith skill | Cancels cart penalty |
| Moonlight Flower Card | ~110 | ~1.36x faster | Acts like Inc AGI, does not stack with it | Accessory card |
| Pushcart Lv1 penalty | 225 | 0.67x slower | Additive penalty | -50% speed |
| Pushcart Lv2 penalty | 210 | 0.71x slower | | -40% speed |
| Pushcart Lv3 penalty | 195 | 0.77x slower | | -30% speed |
| Pushcart Lv4 penalty | 173 | 0.87x slower | | -15% speed |
| Pushcart Lv5 penalty | 157 | 0.95x slower | | -5% speed |
| Curse status | 225 | 0.67x slower | Speed reduced to 2/3 | Status effect |
| Quagmire | ~250 | 0.60x slower | Significant reduction | Wizard AoE |
| Decrease AGI | ~195 | 0.77x slower | Debuff | Acolyte/monster skill |
| Ankle Snare | 0 (rooted) | Immobile | Trap | Cannot move at all |
| Spider Web | 0 (rooted) | Immobile | Sage skill | Cannot move at all |

**Key speed rules:**
- Speed Potions override ALL other speed effects (both bonuses and penalties)
- Increase AGI and Peco Peco Ride stack additively
- Most other movement speed bonuses do NOT stack -- only the strongest applies
- **Weight does NOT directly affect movement speed** in pre-renewal (only blocks attacks/skills at 90%+)
- There is **no separate "run" state** -- characters have only one movement mode

---

### 4.4 Pathfinding

RO uses the **A* (A-star) pathfinding algorithm** server-side:

- `path_search()` in rAthena's `path.cpp` implements A*
- Searches from current position to destination through walkable cells
- Considers 8-directional movement
- Returns an array of direction values (0-7, one per step)
- Max path length: `MAX_WALKPATH = 32` cells
- Server re-validates walkability per step during traversal
- If destination is unreachable: server ignores the walk request (no error sent to client)

**Client-side rendering of remote players:**
- NOT traditional FPS-style interpolation/extrapolation
- Server tells clients "entity X walks from (x1,y1) to (x2,y2) at speed S"
- Client computes local path and animates walk at the known speed
- This is **path-based animation**, not prediction

**Position updates:**
- Server does NOT send periodic position updates at a fixed rate
- Sends one move packet per walk command (when entity starts walking)
- Stationary entities receive no position updates
- Effective update rate depends on movement speed and path changes

---

### 4.5 Movement Interaction with Skills / Attacks

**Movement cancels:**
- Moving while casting a skill **interrupts the cast** (if movement exceeds small threshold)
- Auto-attack is interrupted by movement (clicking ground stops attacking)
- Some skills have "walk delay" preventing movement for a short time after use

**Movement blocks (cannot move):**
- Stun, Freeze, Stone Curse, Deep Sleep: immobilized
- Ankle Snare (Hunter trap): rooted in place
- Spider Web (Sage): rooted
- Close Confine: both attacker and target rooted
- Root Lock (Blade Stop): both participants immobilized
- Sitting: must stand up to move
- Steel Body (Monk): cannot move while active

**Weight thresholds (movement still works):**
- Weight >= 50%: Natural HP/SP regeneration stops
- Weight >= 90%: Cannot attack or use skills, but CAN still move
- Weight >= 100%: Cannot pick up items, but CAN still move

---

## 5. Save Points

### 5.1 How Save Points Work

A save point is the location where a player respawns after death. It is stored per-character in the database.

**Setting a save point:**
1. Talk to a **Kafra NPC** and select "Save Location" (free service)
2. Talk to an **Inn NPC** (some towns have inns that double as save points)
3. The save point stores both the **map name** and **exact (x, y) coordinates**

**Default save point:**
- New characters start with their save point set to their starting city (typically Prontera for Novice)
- If save point data is corrupted or missing, player respawns at `prontera, 156, 185`

**Save point restrictions:**
- Cannot set save point on maps with `nosave` flag (all dungeons, most fields)
- Towns redirect `nosave` to themselves (dying in a town respawns you in that same town)
- Some maps with `nosave` specify a redirect: `nosave <map>,<x>,<y>`

**Death respawn behavior:**
1. Player dies
2. Death penalty applied (EXP loss, unless `nopenalty` flag)
3. Player shown respawn dialog
4. Player warps to save point (map + coordinates)
5. Player respawns with 1 HP, 0 SP (or percentage based on resurrection method)

**Save point persistence:**
- Saved in the `characters` table: `last_map`, `last_x`, `last_y` columns
- Also `save_map`, `save_x`, `save_y` for the actual save/respawn point
- `last_map/x/y` is the last position (for login), `save_map/x/y` is the respawn point

---

### 5.2 Save Point Locations Per Town

Every town has at least one Kafra NPC that can set save points. Common save point locations:

| Town | Kafra Coordinates (approx.) | Notes |
|------|----------------------------|-------|
| Prontera | Center of town | Most popular save point for new players |
| Geffen | Central area | Near magic shops |
| Payon | Town center | Near tool shops |
| Alberta | Near docks | Convenient for ship travel |
| Morroc | Town center | Desert area |
| Al De Baran | Near Kafra HQ | Multiple Kafra NPCs |
| Izlude | Central | Near arena |
| Comodo | Cave entrance | Underground town |
| Juno | Central plaza | Schwartzvald capital |
| Rachel | Temple district | Arunafeltz capital |

---

## 6. Day/Night Cycle

### 6.1 Historical Status

The day/night cycle was a **partially implemented and later removed feature** in Ragnarok Online.

**Original design (from early kRO):**
- The game was intended to have a full day/night cycle with changing sky colors
- Morning, afternoon, and evening lighting states
- Presented in early development showcases with changing map hues
- Was active on Korean servers (kRO) for some time
- Eventually removed/disabled from the live game -- reasons unknown

**What was planned:**
- Time-of-day linked to visual lighting changes
- Night would reduce visibility (darker maps)
- Some monster spawns would change based on time (night-only monsters)
- Magic effectiveness would vary with time (thunder attacks stronger during rain, etc.)
- Fire magic stronger during heat waves (desert maps)
- Water attacks stronger during rainy periods

### 6.2 What Actually Shipped (Pre-Renewal)

In the final pre-renewal client:
- **No functional day/night cycle** in standard gameplay
- Some maps have permanent "night" atmosphere (Niflheim has `night` flag)
- The `night` map flag forces dark lighting regardless of time
- No time-based monster spawn changes
- No time-based skill effectiveness changes
- The `/lightmap` command exists for adjusting personal brightness preference

**For Sabri_MMO implementation:** The day/night cycle can be implemented as an enhancement feature, but it is NOT part of authentic pre-renewal RO. If implemented, it should be cosmetic-only (no gameplay impact) to maintain pre-renewal balance fidelity.

---

## 7. Weather Effects

### 7.1 Map-Specific Weather (Visual Only)

Weather effects in pre-renewal RO are purely visual and tied to specific maps via map flags. They have **no gameplay impact** (no damage modifiers, no stat changes).

| Weather Flag | Visual Effect | Maps Using It |
|-------------|--------------|---------------|
| `snow` | Falling snowflakes | Lutie/Xmas, Rachel fields, Ice Cave area |
| `rain` | Rainfall + puddle reflections | Various fields (seasonal/regional) |
| `fog` | Dense fog reducing visibility | Niflheim, swamp areas |
| `clouds` | Moving cloud shadows | High-altitude maps (Yuno fields) |
| `clouds2` | Dense overcast | Storm areas |
| `sakura` | Falling sakura petals | Amatsu, Payon (spring season) |
| `leaves` | Falling leaves | Forest maps (Payon forests) |
| `fireworks` | Firework particle explosions | Comodo |
| `night` | Permanent night-time lighting | Niflheim |

### 7.2 Weather Interaction with Indoor Flag

Maps with the `indoor` flag suppress all weather effects. This prevents illogical situations like snow falling inside a dungeon.

### 7.3 Client-Side Weather Rendering

- Weather effects are rendered client-side using particle systems
- The server tells the client which weather flags are active via the map properties packet
- No server-side simulation of weather
- Weather does not change dynamically -- it is fixed per map
- Some private servers implement dynamic weather, but official pre-renewal does not

---

## 8. Implementation Checklist

### 8.1 Server-Side Zone System

```
[x] Zone registry (ro_zone_data.js) with ZONE_REGISTRY structure
[x] Zone-scoped Socket.io rooms (zone:<zoneName>)
[x] Player zone tracking (connectedPlayers.zone + DB characters.zone_name)
[x] Zone transition handler (zone:warp socket event)
[x] Zone ready handler (zone:ready socket event -- broadcasts arrival)
[x] Lazy enemy spawning per zone (spawn on first player entry)
[x] Zone filtering on all AoE loops (enemy/player iteration)
[ ] Complete map flag enforcement per zone:
    [x] noteleport (Fly Wing blocked)
    [x] noreturn (Butterfly Wing blocked)
    [x] nosave (save point blocked)
    [ ] nomemo (Warp Portal memo blocked)
    [ ] nobranch (Dead Branch blocked)
    [ ] noicewall (Ice Wall blocked)
    [ ] pvp (PvP combat enabled)
    [ ] gvg (GvG combat enabled)
    [ ] nopenalty (no death penalty)
    [ ] indoor (visual flag sent to client)
    [ ] nodrop / notrade / novending
[ ] All town Kafra NPC definitions with correct destinations + costs
[ ] All field map connections (bidirectional warps)
[ ] All dungeon floor connections
[ ] Save point system (Kafra save, DB persistence)
[ ] Death respawn to save point
```

### 8.2 Client-Side Zone System

```
[x] ZoneTransitionSubsystem (UWorldSubsystem)
[x] Loading overlay during zone transitions
[x] AWarpPortal actor (trigger volume + socket emit)
[x] AKafraNPC actor (interaction + UI)
[x] SKafraWidget (Kafra service UI)
[x] KafraSubsystem (Kafra domain logic)
[x] GameInstance zone state persistence (CurrentZoneName, PendingZoneName, etc.)
[x] Level Blueprint structure (spawn + save timer + cleanup)
[ ] Map flag display in UI (show zone name, type, restrictions)
[ ] Minimap system
[ ] Weather effect rendering per zone flag
[ ] Indoor/outdoor lighting switch
[ ] Ground cursor highlighting (walkable/non-walkable cell feedback)
```

### 8.3 Content Creation Per Zone

```
For each new zone:
[ ] 1. Zone added to ro_zone_data.js (name, displayName, type, flags, defaultSpawn, levelName)
[ ] 2. Connecting warps added to adjacent zones (bidirectional)
[ ] 3. UE5 level duplicated from existing working level (L_ prefix)
[ ] 4. Level Blueprint verified (BeginPlay spawn + SavePosition timer + EndPlay cleanup)
[ ] 5. World Settings: GameMode Override = GM_MMOGameMode, DefaultPawnClass = None
[ ] 6. BP_SocketManager placed in level
[ ] 7. AWarpPortal actors placed (one per warp, WarpId matching registry)
[ ] 8. AKafraNPC actors placed (one per kafra, KafraId matching registry)
[ ] 9. Nav Mesh Bounds Volume covering playable area
[ ] 10. Lighting setup (outdoor: directional + sky; indoor: point lights)
[ ] 11. Terrain/geometry placed
[ ] 12. Monster spawn definitions in ro_zone_data.js
[ ] 13. Server restarted to pick up new zone data
[ ] 14. Tested: warp in, warp out, Kafra services, enemy spawns, zone flag enforcement
```

---

## 9. Gap Analysis

### 9.1 Currently Implemented (Sabri_MMO)

Based on the current codebase and `ro_zone_data.js`:

| Feature | Status | Notes |
|---------|--------|-------|
| Zone registry system | DONE | `ro_zone_data.js` with ZONE_REGISTRY |
| Zone-scoped socket rooms | DONE | `zone:<zoneName>` rooms |
| Zone transition flow | DONE | `ZoneTransitionSubsystem`, loading overlay, OpenLevel |
| Warp portal actors | DONE | `AWarpPortal` C++ actor |
| Kafra NPC actors | DONE | `AKafraNPC` C++ actor, `SKafraWidget` |
| Persistent socket across zones | DONE | Phase 4 architecture |
| Player zone tracking | DONE | In-memory + DB persistence |

**Current zones implemented:** `prontera`, `prontera_south`, `prontera_north`, `prt_dungeon_01` (4 zones total)

### 9.2 Missing / Not Yet Implemented

| Feature | Priority | Effort | Notes |
|---------|----------|--------|-------|
| Additional zones (~416 remaining) | HIGH | Large | Content creation per zone |
| Complete map flag enforcement | HIGH | Medium | nomemo, nobranch, noicewall, pvp, gvg, nopenalty, indoor, nodrop, notrade, novending |
| Save point system | HIGH | Medium | Kafra save service, death respawn to save point, DB columns |
| Minimap system | MEDIUM | Medium | Zone map overlay, player/party position markers |
| Weather effects per zone | LOW | Medium | Snow, rain, fog, sakura, leaves, fireworks particles |
| Indoor/outdoor lighting switch | MEDIUM | Small | Toggle lighting preset based on zone `indoor` flag |
| PvP arena maps | MEDIUM | Medium | PvP flag enforcement, no death penalty, arena rooms |
| GvG castle maps | LOW | Large | Full WoE system required first |
| Instance dungeon system | LOW | Large | Per-party private map copies, time limits |
| Dead Branch system | LOW | Small | Random monster summoning, `nobranch` flag check |
| Airship travel | LOW | Medium | Moving map, boarding/disembarking NPCs |
| Ship travel (Alberta) | LOW | Small | NPC with warp + Zeny cost |
| Ground cursor highlighting | LOW | Small | Visual walkable/non-walkable cell feedback |
| Day/night cycle | LOWEST | Medium | Not authentic pre-renewal, cosmetic only |

### 9.3 Recommended Implementation Order

**Phase 1 -- Core Towns (5 zones):**
Prontera (done), Geffen, Payon, Alberta, Morroc -- the 5 original Episode 1 towns. Add Kafra NPCs with correct teleport destinations and costs. Implement save point system.

**Phase 2 -- Starter Fields (8-10 zones):**
prt_fild08 (done as prontera_south), surrounding Prontera/Geffen/Payon/Morroc fields. Connect towns with field map chains. Add monster spawns per zone.

**Phase 3 -- First Dungeons (8-12 floors):**
Prontera Culvert (done 1 floor), Payon Dungeon (5 floors), Geffen Tower (4 floors). Implement `noteleport` + `noreturn` + `nosave` + `indoor` flag enforcement.

**Phase 4 -- Map Flag Enforcement:**
Implement remaining map flags: nomemo, nobranch, noicewall, pvp, nopenalty, nodrop, notrade, novending. Add flag display in UI.

**Phase 5 -- Extended Towns:**
Al De Baran, Izlude, Comodo, Umbala, Lutie. Add mid-level connecting fields.

**Phase 6 -- Mid-Level Dungeons:**
Orc Dungeon, Byalan Island, Ant Hell, Pyramids, Sphinx. Add MVP spawns.

**Phase 7 -- Schwartzvald Content:**
Juno, Einbroch, Lighthalzen, Hugel towns. Schwartzvald fields and dungeons.

**Phase 8 -- High-Level Content:**
Glast Heim complex, Clock Tower, Turtle Island. Bio Lab, Thor's Volcano, Thanatos Tower.

**Phase 9 -- Arunafeltz + Foreign Lands:**
Rachel, Veins, Amatsu, Louyang, Ayothaya. Ice Cave, Thor's Volcano, Abbey.

**Phase 10 -- Special Systems:**
PvP arenas, WoE castles, Instance dungeons, Airship/ship travel.

### 9.4 Key Architecture Decisions for Sabri_MMO

1. **One Zone = One UE5 Level** -- Maintain this 1:1 mapping. Do not use level streaming or world partition.

2. **All zone metadata in `ro_zone_data.js`** -- Single source of truth for zone names, flags, warps, Kafra NPCs, enemy spawns. Client levels just provide geometry and placed actors.

3. **Zone flag enforcement is server-side** -- Client reads flags for UI display only. Server validates all teleport/save/item actions against zone flags. Never trust the client.

4. **50 UE units = 1 RO cell** -- Maintain this conversion ratio for all zone geometry and warp coordinates.

5. **Bidirectional warps** -- Every warp connection must exist in both the source and destination zone registries.

6. **Duplicate levels, never create from scratch** -- Level Blueprint structure is critical and cannot be created via code. Always duplicate an existing working level.

7. **Weather effects are client-side only** -- Server sends zone flags. Client renders appropriate particle effects. No server simulation of weather.

---

## Sources

### Primary References
- [rAthena Mapflag Documentation](https://github.com/rathena/rathena/wiki/Mapflag)
- [rAthena mapflags.txt (source)](https://github.com/rathena/rathena/blob/master/doc/mapflags.txt)
- [rAthena noteleport.txt (map list)](https://github.com/rathena/rathena/blob/master/npc/mapflag/noteleport.txt)
- [iRO Wiki Classic -- Kafra](https://irowiki.org/classic/Kafra)
- [iRO Wiki Classic -- Warp Portal](https://irowiki.org/classic/Warp_Portal)
- [iRO Wiki Classic -- World Map](https://irowiki.org/classic/World_Map)
- [iRO Wiki Classic -- Movement Speed](https://irowiki.org/classic/Movement_Speed)
- [iRO Wiki Classic -- Places](https://irowiki.org/classic/Places)
- [iRO Wiki Classic -- Peco Peco Ride](https://irowiki.org/classic/Peco_Peco_Ride)
- [iRO Wiki Classic -- Endless Tower](https://irowiki.org/classic/Endless_Tower)
- [iRO Wiki Classic -- PvP](https://irowiki.org/classic/PvP)

### Secondary References
- [RateMyServer -- Map Database](https://ratemyserver.net/index.php?page=map_db)
- [RateMyServer -- Pre-Renewal Dungeon Maps](https://ratemyserver.net/dungeonmap.php)
- [RateMyServer -- Interactive World Map](https://ratemyserver.net/worldmap.php)
- [RateMyServer -- Map Access Guide](https://write.ratemyserver.net/ragnoark-online-quest-guides/map-access-guide/)
- [rAthena Pre-renewal Database](https://pre.pservero.com/)
- [Ragnarok Research Lab -- Movement and Pathfinding](https://ragnarokresearchlab.github.io/game-mechanics/movement/)
- [Ragnarok Wiki (Fandom) -- Map](https://ragnarok.fandom.com/wiki/Map)
- [Ragnarok Wiki (Fandom) -- Dead Branch](https://ragnarok.fandom.com/wiki/Dead_Branch)
- [Ragnarok Wiki (Fandom) -- Navigation System](https://ragnarok.fandom.com/wiki/Navigation_System)
- [Ragnarok Wiki (Fandom) -- Unimplemented RO Features](https://ragnarok.fandom.com/wiki/Unimplemented_RO_Features)
- [RO Wiki -- Day and Night Cycle](https://www.ragnarok.wiki/Day_and_Night_Cycle_(Ragnarok_Online))
- [iRO Wiki -- Cool Event Corporation](https://irowiki.org/wiki/Cool_Event)
- [iRO Wiki -- Access Quests](https://irowiki.org/wiki/Access_Quests)
- [iRO Wiki Database -- World Map](https://db.irowiki.org/db/world-map/)
- [rAthena Custom Maps Wiki](https://github.com/rathena/rathena/wiki/Custom_Maps)
- [rAthena Kafra NPC Script](https://github.com/rathena/rathena/blob/master/npc/kafras/kafras.txt)
