# 06 -- World Maps, Zones, and Dungeons

> Reference document for Sabri_MMO. Covers the entire Ragnarok Online Classic world geography: towns, field maps, dungeons, warp systems, map properties, and UE5 implementation strategy.

---

## Table of Contents

1. [Continental Overview](#1-continental-overview)
2. [Major Towns](#2-major-towns)
3. [Field Maps](#3-field-maps)
4. [Dungeons](#4-dungeons)
5. [Warp System](#5-warp-system)
6. [Map Properties and Flags](#6-map-properties-and-flags)
7. [UE5 Implementation](#7-ue5-implementation)

---

## 1. Continental Overview

The world of Ragnarok Online is set on the continent of **Midgard**, divided into three major nations and several outlying islands.

### 1.1 Rune-Midgarts Kingdom (Southern Continent)

The largest and oldest nation. Capital: **Prontera**. Governed by King Tristan Gaebolg III. Contains the majority of low-to-mid-level content and the original towns from Episode 1-6.

**Major towns:** Prontera, Geffen, Payon, Alberta, Morroc, Izlude, Al De Baran, Comodo, Umbala

**Key geographic features:**
- Prontera Fields (central plains)
- Sograt Desert (southwest)
- Mt. Mjolnir (northwest mountain range)
- Payon Forest (southeast)
- Kokomo Beach / Comodo caves (far south)

### 1.2 Republic of Schwartzvald (Northeast Continent)

An industrialized nation governed by a President. Capital: **Juno (Yuno)**. Contains mid-to-high-level content from Episodes 7-11.

**Major towns:** Juno/Yuno, Einbroch, Lighthalzen, Hugel

**Key geographic features:**
- Yuno Fields (high-altitude plateaus)
- Einbroch Fields (industrial wasteland)
- Hugel Fields (coastal gardens)
- Border with Rune-Midgarts at Al De Baran

### 1.3 Arunafeltz States (Northwest Continent)

A theocratic nation. Capital: **Rachel**. Contains high-level content from Episodes 11.2-11.3.

**Major towns:** Rachel, Veins

**Key geographic features:**
- Rachel Fields (frozen tundra)
- Veins Fields (volcanic desert)

### 1.4 Outlying Islands and Foreign Lands

Accessible by ship from various ports:

| Location | Theme | Access From |
|----------|-------|-------------|
| Amatsu | Feudal Japan | Alberta (ship) |
| Louyang | Ancient China | Alberta (ship) |
| Ayothaya | Ancient Thailand | Alberta (ship) |
| Jawaii | Honeymoon Island (Hawaii) | Alberta (ship, married couples) |
| Moscovia | Medieval Russia | Alberta (ship) |
| Lutie / Xmas | Christmas Village | Al De Baran (warp) |
| Niflheim | Norse Underworld | Umbala (Yggdrasil tree) |
| Turtle Island | Tropical Island | Alberta (ship) |
| Nameless Island | Cursed Monastery | Veins (ship) |

### 1.5 Inter-Continental Travel

| Method | Route |
|--------|-------|
| **Domestic Airship** | Hugel -> Juno -> Einbroch -> Lighthalzen (loops) |
| **International Airship** | Izlude -> Juno -> Rachel (loops) |
| **Kafra Teleport** | Town-to-town within each nation |
| **Ship Routes** | Alberta -> Amatsu / Louyang / Ayothaya / Jawaii / Turtle Island |
| **Warp Portal (Priest skill)** | Any memorized location |
| **Fly Wing** | Random spot on current map |
| **Butterfly Wing** | Return to save point |

---

## 2. Major Towns

### 2.1 Prontera (prontera)

| Property | Value |
|----------|-------|
| **Theme** | Medieval European capital, white stone castle |
| **Nation** | Rune-Midgarts Kingdom (capital) |
| **Map ID** | `prontera` |
| **RO Size** | 312 x 392 cells |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Geffen, Payon, Morroc, Alberta, Orc Dungeon
- **Job Change:** Acolyte Guild, Knight Guild, Crusader Guild, Priest/Monk (Prontera Church)
- **Arena / PvP:** Prontera Arena (north)
- **Tool Dealer, Weapon Dealer, Armor Dealer** -- main shopping street
- **Refiner NPC** -- equipment upgrade
- **Stylist NPC** -- hair color/style change
- **Pet Groomer** -- pet management
- **Bulletin Board** -- quest board

**Connections:**
- South exit -> prt_fild08 (Prontera South Field)
- North exit -> prt_fild01 (Prontera North Field)
- East exit -> prt_fild04 (Prontera East Field)
- West exit -> prt_fild05 (Prontera West Field)
- Prontera Culvert entrance (Knight Guild basement)

---

### 2.2 Geffen (geffen)

| Property | Value |
|----------|-------|
| **Theme** | Magic city, built around a great tower |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `geffen` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Prontera, Payon, Alberta, Morroc
- **Job Change:** Mage Guild, Wizard Guild, Sage Guild
- **Tool Dealer, Weapon/Armor Dealer**
- **Geffen Tower** -- dungeon entrance in center of town

**Connections:**
- North -> gef_fild00 (to Mt. Mjolnir)
- South -> gef_fild07 (Geffen South Field)
- East -> gef_fild04 (to Prontera Fields)
- West -> gef_fild06 (to Orc Village)
- Center -> Geffen Tower Dungeon (underground)

---

### 2.3 Payon (payon)

| Property | Value |
|----------|-------|
| **Theme** | Korean/East Asian village in bamboo forest |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `payon` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Prontera, Alberta, Morroc (1200z each)
- **Job Change:** Archer Guild (Archer Village, north of Payon)
- **Tool Dealer, Weapon/Armor Dealer**
- **Payon Cave entrance** (via Archer Village)

**Connections:**
- South -> pay_fild01 (to Alberta)
- North -> Archer Village (pay_arche) -> Payon Dungeon entrance
- West -> pay_fild04 (to Prontera Fields)
- East -> pay_fild07 (Payon Forest)

---

### 2.4 Alberta (alberta)

| Property | Value |
|----------|-------|
| **Theme** | Maritime port town, cool sea breeze |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `alberta` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Prontera, Payon, Morroc
- **Job Change:** Merchant Guild
- **Ship Captain** -- Travel to Amatsu, Louyang, Ayothaya, Jawaii, Turtle Island, Moscovia
- **Tool Dealer, Weapon/Armor Dealer**

**Connections:**
- North -> alb_fild02 (to Payon)
- West -> alb_fild01 (to Prontera Fields)
- Port -> Ship to overseas destinations

---

### 2.5 Morroc (morocc)

| Property | Value |
|----------|-------|
| **Theme** | Desert oasis town, Arabian architecture |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `morocc` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Prontera, Alberta, Payon
- **Job Change:** Thief Guild (moc_fild11, south pyramid area)
- **Tool Dealer, Weapon/Armor Dealer**
- **Dimensional Gorge entrance** (post-Episode 12, Satan Morroc area)

**Connections:**
- North -> moc_fild01 (to Prontera Fields)
- South -> moc_fild02 (to Ant Hell, Pyramids)
- West -> moc_fild03 (to Sphinx)
- East -> moc_fild12 (to Sograt Desert)
- Pyramids entrance nearby (southeast)
- Sphinx entrance nearby (west fields)

---

### 2.6 Al De Baran (aldebaran)

| Property | Value |
|----------|-------|
| **Theme** | Clockwork city, canals, European-style bridges |
| **Nation** | Border town (Rune-Midgarts / Schwartzvald gateway) |
| **Map ID** | `aldebaran` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra HQ** -- Main Kafra Corporation building (northwest)
- **Alchemist Guild** (southwest) -- Alchemist job change
- **Clock Tower entrance** (center of town)
- **Tool Dealer, Weapon/Armor Dealer**
- **Warp to Lutie** (Christmas village)

**Connections:**
- South -> alde_fild01 (to Geffen/Mjolnir region)
- North -> Juno fields (Schwartzvald border)
- Center -> Clock Tower Dungeon
- Special warp -> Lutie (Christmas map)

---

### 2.7 Izlude (izlude)

| Property | Value |
|----------|-------|
| **Theme** | Satellite town of Prontera, warrior culture |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `izlude` |
| **Type** | Town |

**Key NPCs and Services:**
- **Swordsman Guild** -- Swordsman job change (west side)
- **Arena** -- PvP arena (north)
- **Ship Captains** -- Byalan Island, Alberta (500z), Pharos Lighthouse
- **International Airship** -- To Juno and Rachel
- **Tool Dealer**

**Connections:**
- South -> izlu_fild01 (to Prontera South Field / prt_fild08)
- Port -> Byalan Island (Undersea Tunnel entrance)
- Port -> Alberta
- Airship -> Juno, Rachel

---

### 2.8 Comodo (comodo)

| Property | Value |
|----------|-------|
| **Theme** | Underground cave city, eternal fireworks, entertainment |
| **Nation** | Rune-Midgarts Kingdom |
| **Map ID** | `comodo` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee**
- **Job Change:** Bard Guild, Dancer Guild
- **Casino / Entertainment NPCs**
- **Dungeon entrances** -- Comodo Beach Cave, Hyrule (underwater cave)

**Connections:**
- North -> Kokomo Beach -> cave system -> Morroc fields
- East -> cmd_fild01 (to Umbala)
- South -> Beach Dungeon / Pharos Lighthouse area
- Three cave dungeons accessible from within Comodo

---

### 2.9 Umbala (umbala)

| Property | Value |
|----------|-------|
| **Theme** | Tribal tree-village, Wootan/Utan tribe, jungle |
| **Nation** | Rune-Midgarts Kingdom (remote) |
| **Map ID** | `umbala` |
| **Type** | Town |

**Key NPCs and Services:**
- **Umbala Language NPC** -- Learn tribal language to interact with NPCs
- **Kafra Employee**
- **Yggdrasil Tree entrance** -- leads to Niflheim

**Connections:**
- South -> um_fild01 (to Comodo)
- Deep jungle -> Umbala Dungeon
- Yggdrasil Tree root -> Niflheim

---

### 2.10 Niflheim (niflheim)

| Property | Value |
|----------|-------|
| **Theme** | Norse underworld, realm of the dead, ghostly village |
| **Nation** | None (otherworld) |
| **Map ID** | `niflheim` |
| **Type** | Town (hostile -- monsters on some adjacent maps) |

**Key NPCs and Services:**
- **Ghostly Kafra Employee** -- Limited services
- **Dead Branch Crafter** -- Creates Dead Branches (summon random monsters)
- **Various ghost NPCs** with lore quests

**Connections:**
- Yggdrasil Tree root -> Umbala
- Niflheim Fields (nif_fild01, nif_fild02) contain high-level undead/demon monsters

**Note:** Niflheim has no save point. Dying here returns you to your last save. Getting to Niflheim requires traversing the Umbala Dungeon and descending through the Yggdrasil tree.

---

### 2.11 Lutie / Xmas (xmas)

| Property | Value |
|----------|-------|
| **Theme** | Christmas village, perpetual snow, Santa Claus |
| **Nation** | None (magical place) |
| **Map ID** | `xmas` |
| **Type** | Town |

**Key NPCs and Services:**
- **Santa Claus** -- Seasonal quests
- **Kafra Employee**
- **Toy Factory entrance** (north)
- **Tool Dealer**

**Connections:**
- Warp from Al De Baran
- North -> Toy Factory Dungeon entrance

---

### 2.12 Amatsu (amatsu)

| Property | Value |
|----------|-------|
| **Theme** | Feudal Japan, sakura blossoms, castles |
| **Nation** | Foreign land (Global Project: Japan) |
| **Map ID** | `amatsu` |
| **Type** | Town |

**Key NPCs and Services:**
- **Ninja Guild** (northeast area)
- **Tool Dealer, Weapon Dealer**
- **Amatsu Castle entrance** -- dungeon (requires pass from Emperor quest)

**Connections:**
- Ship from Alberta
- Amatsu Fields (ama_fild01)
- Amatsu Dungeon (3 floors, inside castle)

**Dungeon MVP:** Incantation Samurai (Floor 3)

---

### 2.13 Louyang (louyang)

| Property | Value |
|----------|-------|
| **Theme** | Ancient China, pagodas, dragon motifs |
| **Nation** | Foreign land (Global Project: China) |
| **Map ID** | `louyang` |
| **Type** | Town |

**Key NPCs and Services:**
- **Tool Dealer, Weapon Dealer**
- **Royal Tomb entrance** -- dungeon

**Connections:**
- Ship from Alberta
- Louyang Fields (lou_fild01)
- Royal Tomb Dungeon (underground, multi-floor)

**Dungeon MVP:** White Lady / Bacsojin (based on Madame White Snake legend)

---

### 2.14 Ayothaya (ayothaya)

| Property | Value |
|----------|-------|
| **Theme** | Ancient Thailand, Ayutthaya Kingdom period |
| **Nation** | Foreign land (Global Project: Thailand) |
| **Map ID** | `ayothaya` |
| **Type** | Town |

**Key NPCs and Services:**
- **Tool Dealer**
- **Ancient Shrine entrance** (requires entrance quest)

**Connections:**
- Ship from Alberta
- Ayothaya Fields (ayo_fild01, ayo_fild02)
- Ancient Shrine Dungeon (maze-like, hole-jumping puzzle)

**Dungeon MVP:** Lady Tanee

---

### 2.15 Jawaii (jawaii)

| Property | Value |
|----------|-------|
| **Theme** | Honeymoon Island, tropical paradise (Hawaii inspired) |
| **Nation** | None (vacation destination) |
| **Map ID** | `jawaii` |
| **Type** | Town |

**Key NPCs and Services:**
- **Pub** -- Only area unmarried players can access
- **Marriage-related NPCs**
- **Beach / Resort facilities**

**Connections:**
- Ship from Alberta
- Only married players may explore outside the pub

---

### 2.16 Hugel (hugel)

| Property | Value |
|----------|-------|
| **Theme** | Garden village, coastal, tourist destination |
| **Nation** | Republic of Schwartzvald |
| **Map ID** | `hugel` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee**
- **Hunter Guild / Trapper Guild**
- **Airship terminal** (Domestic route)
- **Tool Dealer**

**Connections:**
- Domestic Airship -> Juno, Einbroch, Lighthalzen
- Hugel Fields (hu_fild01-07)
- Odin's Temple entrance (via field)
- Abyss Lake entrance (via field)
- Thanatos Tower entrance (via field)

---

### 2.17 Lighthalzen (lighthalzen)

| Property | Value |
|----------|-------|
| **Theme** | Industrial city-state of commerce, Rekenber Corporation HQ |
| **Nation** | Republic of Schwartzvald |
| **Map ID** | `lighthalzen` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee**
- **Rekenber Corporation** -- Bio Lab entrance (requires quest)
- **Airship terminal** (Domestic route)
- **Slums district** (poverty area, quest-related)

**Connections:**
- Domestic Airship -> Hugel, Juno, Einbroch
- Lighthalzen Fields (lhz_fild01-03)
- Bio Laboratory Dungeon (basement of Rekenber Corp, quest access)

---

### 2.18 Einbroch (einbroch)

| Property | Value |
|----------|-------|
| **Theme** | Heavy industrial city, factories, pollution, steampunk |
| **Nation** | Republic of Schwartzvald |
| **Map ID** | `einbroch` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee**
- **Airship terminal** (Domestic route)
- **Gunslinger Guild** (job change)
- **Einbech mining village** nearby (sister town)

**Connections:**
- Domestic Airship -> Hugel, Juno, Lighthalzen
- Einbroch Fields (ein_fild01-10)
- Einbech Mine Dungeon (via Einbech village)

---

### 2.19 Juno / Yuno (yuno)

| Property | Value |
|----------|-------|
| **Theme** | Floating island city above the clouds, academic/scholarly |
| **Nation** | Republic of Schwartzvald (capital) |
| **Map ID** | `yuno` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Al De Baran (1200z)
- **Sage Academy** -- Sage job change
- **Airship terminals** (both Domestic and International routes)
- **Juperos Ruins entrance** (via field maps)

**Connections:**
- International Airship -> Izlude, Rachel
- Domestic Airship -> Hugel, Einbroch, Lighthalzen
- South -> yuno_fild01 (to Al De Baran border)
- Juperos Ruins entrance (far west field)
- Thanatos Tower (via Hugel fields)

---

### 2.20 Rachel (rachel)

| Property | Value |
|----------|-------|
| **Theme** | Theocratic holy city, frozen tundra, temples |
| **Nation** | Arunafeltz States (capital) |
| **Map ID** | `rachel` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee** -- Teleport to Veins
- **International Airship terminal**
- **Rachel Sanctuary entrance** (Ice Cave / Ice Dungeon)
- **Temple of Freya** -- quest-related area

**Connections:**
- International Airship -> Juno, Izlude
- Rachel Fields (ra_fild01-13)
- Ice Cave / Rachel Sanctuary (3 floors + boss floor)
- Path to Veins (walking through fields)

---

### 2.21 Veins (veins)

| Property | Value |
|----------|-------|
| **Theme** | Volcanic desert town, extreme heat |
| **Nation** | Arunafeltz States |
| **Map ID** | `veins` |
| **Type** | Town |

**Key NPCs and Services:**
- **Kafra Employee**
- **Tool Dealer**
- **Ship Captain** -- To Nameless Island (quest required)

**Connections:**
- North -> ve_fild01 (to Rachel fields)
- South -> ve_fild07 (to Thor's Volcano entrance)
- Ship -> Nameless Island
- Thor's Volcano Dungeon (via north field)

---

### 2.22 Moscovia (moscovia)

| Property | Value |
|----------|-------|
| **Theme** | Medieval Russian village, rustic island |
| **Nation** | Foreign land (Global Project: Russia) |
| **Map ID** | `moscovia` |
| **Type** | Town |

**Key NPCs and Services:**
- **Tool Dealer**
- **Quest NPCs** (required to access dungeon)
- **Mr. Ibanoff** (docks, dungeon access NPC)

**Connections:**
- Ship from Alberta
- Moscovia Forest Dungeon (3 floors, quest access)

**Notable dungeon monsters:** Wood Goblin, Les, Baba Yaga, Mavka

---

## 3. Field Maps

Field maps are the outdoor connecting zones between towns and dungeon entrances. They contain monster spawns, warp portals to adjacent maps, and occasionally NPCs.

### 3.1 Prontera Fields

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `prt_fild00` | Prontera Field North | 1-15 | Poring, Lunatic, Fabre | prontera (north) |
| `prt_fild01` | Prontera Field 01 | 1-15 | Poring, Lunatic, Fabre, Pupa | prontera (north), Mjolnir fields |
| `prt_fild02` | Prontera Field 02 | 5-20 | Willow, Poring, Roda Frog | Mt. Mjolnir, Geffen fields |
| `prt_fild03` | Prontera Field 03 | 10-25 | Chonchon, Condor, Willow | Prontera, Alberta direction |
| `prt_fild04` | Prontera Field 04 | 10-25 | Chonchon, Rocker, Willow | prontera (east), Payon direction |
| `prt_fild05` | Prontera Field 05 | 5-20 | Poring, Drops, Chonchon | prontera (west), Geffen direction |
| `prt_fild06` | Prontera Field 06 | 10-20 | Savage Babe, Rocker | Prontera south fields |
| `prt_fild07` | Prontera Field 07 | 5-15 | Poring, Fabre, Lunatic | prt_fild08, prontera |
| `prt_fild08` | Prontera Field 08 | 1-10 | Poring (70), Lunatic (40), Pupa (20), Drops (10) | prontera (south), izlude, moc_fild01 |

**Design notes:** Prontera fields are the primary starter area. prt_fild08 is the canonical "first field" with the highest Poring density. Monsters are passive/low-level, making it safe for brand new characters.

### 3.2 Geffen Fields

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `gef_fild00` | Geffen Field 00 | 15-30 | Poison Spore, Poporing | geffen (north), Mjolnir |
| `gef_fild01` | Geffen Field 01 | 15-30 | Creamy, Smokie | Geffen west |
| `gef_fild03` | Geffen Field 03 | 20-40 | Side Winder, Mantis | Mid-level zone |
| `gef_fild04` | Geffen Field 04 | 10-25 | Chonchon, Rocker | Prontera direction |
| `gef_fild06` | Geffen Field 06 | 20-40 | Wolf, Goblin, Kobold | Orc Village direction |
| `gef_fild07` | Geffen Field 07 | 15-30 | Poison Spore, Smokie | geffen (south) |
| `gef_fild10` | Geffen Field 10 | 30-55 | Orc Warrior, Orc Lady, Orc Baby | AoE leveling spot, Orc Village |
| `gef_fild14` | Geffen Field 14 | 40-60 | Petit (Sky), Penomena | High-level Geffen area |

**Design notes:** Geffen fields bridge the gap between Prontera starter area and mid-level content. gef_fild10 (Orc fields) is one of the most popular AoE leveling spots in classic RO.

### 3.3 Payon Fields / Forests

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `pay_fild01` | Payon Field 01 | 10-25 | Willow, Spore | payon (south), Alberta |
| `pay_fild02` | Payon Forest 02 | 15-30 | Elder Willow, Poporing | Payon, Archer Village |
| `pay_fild04` | Payon Field 04 | 20-35 | Smokie, Bigfoot | Prontera fields |
| `pay_fild07` | Payon Forest 07 | 25-40 | Coco, Horn, Elder Willow | East Payon forests |
| `pay_fild09` | Payon Forest 09 | 30-45 | Elder Willow, Coco, Horn | Deep forest |
| `pay_fild10` | Payon Forest 10 | 35-50 | Munak, Bongun | Haunted forest |

**Design notes:** Payon forests become increasingly dark and haunted as you go deeper. The undead-themed monsters in the deeper fields lead thematically into Payon Dungeon.

### 3.4 Morroc / Sograt Desert Fields

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `moc_fild01` | Sograt Desert 01 | 5-15 | Peco Peco, Condor | prt_fild08, morroc direction |
| `moc_fild02` | Sograt Desert 02 | 15-30 | Muka, Piere, Andre | morroc (south), Ant Hell entrance |
| `moc_fild03` | Sograt Desert 03 | 20-35 | Desert Wolf, Golem | morroc (west), Sphinx direction |
| `moc_fild07` | Sograt Desert 07 | 25-40 | Hode, Sand Man | Central desert |
| `moc_fild11` | Sograt Desert 11 | 30-45 | Scorpion, Desert Wolf Baby | Pyramid area |
| `moc_fild12` | Sograt Desert 12 | 25-40 | Magnolia, Muka | morroc (east) |
| `moc_fild16` | Sograt Desert 16 | 35-50 | Sand Man, Hode, Phreeoni(MVP) | Deep desert |
| `moc_fild17` | Sograt Desert 17 | 40-55 | Golem, Pasana | Sphinx approach |

**Design notes:** Sograt Desert fields are vast and open with long walking distances. The Thief Guild is hidden among the pyramid area fields. Phreeoni is a field MVP on moc_fild16.

### 3.5 Mt. Mjolnir Fields

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `mjolnir_01` | Mt. Mjolnir 01 | 15-30 | Creamy, Smokie | Geffen, Prontera fields |
| `mjolnir_02` | Mt. Mjolnir 02 | 20-35 | Smokie, Bigfoot | Mountain paths |
| `mjolnir_03` | Mt. Mjolnir 03 | 25-40 | Side Winder, Mantis | Mid-mountain |
| `mjolnir_04` | Mt. Mjolnir 04 | 30-45 | Dustiness, Flora, Mistress(MVP) | Mistress spawns here |
| `mjolnir_05` | Mt. Mjolnir 05 | 25-40 | Argos, Stem Worm | Mountain pass |
| `mjolnir_06` | Mt. Mjolnir 06 | 20-35 | Poison Spore, Smokie, Creamy | Forest mountain |
| `mjolnir_07` | Mt. Mjolnir 07 | 30-45 | Stainer, Savage | Northern mountain |
| `mjolnir_08` | Mt. Mjolnir 08 | 25-40 | Poporing, Magnolia | To Coal Mine entrance |
| `mjolnir_09` | Mt. Mjolnir 09 | 20-35 | Caramel, Bigfoot | To Al De Baran |
| `mjolnir_12` | Dead Pit area | 25-40 | Steel Chonchon, Skeleton Worker | Coal Mine surface |

**Design notes:** Mt. Mjolnir is the mountain range between Prontera/Geffen and Al De Baran. Mistress MVP spawns on mjolnir_04. The Coal Mine (Dead Pit) entrance is on the mountain paths.

### 3.6 Yuno / Schwartzvald Fields

| Map ID | Name | Level Range | Key Monsters | Connections |
|--------|------|-------------|--------------|-------------|
| `yuno_fild01` | Yuno Field 01 | 50-70 | Grand Peco, Sleeper | Yuno (south), Al De Baran border |
| `yuno_fild02` | Yuno Field 02 | 55-75 | Goat, Harpy | Yuno fields |
| `yuno_fild03` | Yuno Field 03 | 60-80 | Demon Pungus, Sleeper | Juperos approach |
| `yuno_fild04` | Yuno Field 04 | 55-75 | Grand Peco, Goat | Yuno south |
| `yuno_fild09` | Border Checkpoint | 50-70 | Geographer, Sleeper | Border area |
| `hu_fild01-07` | Hugel Fields | 55-80 | Metaling, Beetle King, Demon Pungus | Hugel region |
| `ein_fild01-10` | Einbroch Fields | 50-75 | Mineral, Geographer, Holden | Einbroch region |
| `lhz_fild01-03` | Lighthalzen Fields | 60-85 | Metalings, various | Lighthalzen outskirts |
| `ra_fild01-13` | Rachel Fields | 70-90 | Siroma, Snowier, Ice Titan | Frozen tundra |
| `ve_fild01-07` | Veins Fields | 75-95 | Magmaring, Imp, Kasa | Volcanic desert |

**Design notes:** Schwartzvald and Arunafeltz fields represent a significant level jump from Rune-Midgarts. Players typically arrive at level 50-60 via airship. Rachel and Veins fields are among the highest-level outdoor areas.

---

## 4. Dungeons

### 4.1 Prontera Culvert (Sewers)

| Property | Value |
|----------|-------|
| **Location** | Below Prontera (accessed via Knight Guild) |
| **Map IDs** | `prt_sewb1` through `prt_sewb4` |
| **Floors** | 4 |
| **Level Range** | 15-50 (Pre-Renewal) |
| **Entry** | Quest: talk to Recruiter in Knight Guild |
| **Flags** | noteleport, noreturn, nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| B1 | Thief Bug Egg, Thief Bug, Tarou | Low-level starter dungeon |
| B2 | Thief Bug, Thief Bug Female, Familiar | Slightly harder |
| B3 | Thief Bug Male, Thief Bug Female, Poison Spore | Mid-level |
| B4 | Golden Thief Bug (MVP), Cramp, Thief Bug | Boss floor |

**MVP:** Golden Thief Bug (Level 64, Insect/Wind). Respawn: 60-70 min.
**Notable drops:** Golden Thief Bug Card (reflect magic), Gold

---

### 4.2 Payon Dungeon (Payon Cave)

| Property | Value |
|----------|-------|
| **Location** | North of Payon via Archer Village |
| **Map IDs** | `pay_dun00` through `pay_dun04` |
| **Floors** | 5 (listed as B1-B5) |
| **Level Range** | 15-75 (Pre-Renewal), 25-80 (Renewal) |
| **Entry** | Walk through Archer Village |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| B1 | Skeleton, Zombie, Familiar | Entry level |
| B2 | Skeleton Soldier, Archer Skeleton, Skel Worker | Mid-level undead |
| B3 | Munak, Bongun, Horong, Dokebi | Ghost/demon themed |
| B4 | Archer Skeleton, Nine Tail, Greatest General, Am Mut | High-level |
| B5 | Nine Tail, Moonlight Flower (MVP) | Boss floor |

**MVP:** Moonlight Flower (Level 77, Demon/Fire). Respawn: 60-70 min.
**Notable drops:** Moonlight Flower Card (move speed +50%)

---

### 4.3 Geffen Tower (Geffen Dungeon)

| Property | Value |
|----------|-------|
| **Location** | Center of Geffen (tower basement) |
| **Map IDs** | `gef_dun00` through `gef_dun03` |
| **Floors** | 4 (basement levels) |
| **Level Range** | 20-70 (Pre-Renewal) |
| **Entry** | Walk down stairs in Geffen Tower |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| B1 | Red Plant, Dustiness, Familiar, Poison Spore, Poporing | Low-level, plants |
| B2 | Argos, Dustiness, Jakk, Poison Spore, Whisper | Spider webs |
| B3 | Bathory, Joker, Marionette, Myst, Nightmare, Doppelganger(MVP) | Boss floor |
| B4 | Bathory, Nightmare, Dracula(Mini-boss) | Deeper darkness |

**MVP:** Doppelganger (Level 77, Demon/Dark). Respawn: 120-130 min.
**Notable drops:** Doppelganger Card (+30% ASPD), Balmung

---

### 4.4 Orc Dungeon

| Property | Value |
|----------|-------|
| **Location** | West of Geffen (gef_fild10 area) |
| **Map IDs** | `orcsdun01`, `orcsdun02` |
| **Floors** | 2 |
| **Level Range** | 25-55 (Pre-Renewal), 50-80 (Renewal) |
| **Entry** | Walk in from Orc Village field |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Orc Skeleton, Orc Zombie, Zenorc, Drainliar | Undead orcs |
| F2 | Orc Archer, Orc Warrior, Steel Chonchon, Orc Lord(MVP) | Boss floor |

**MVP:** Orc Lord (Level 73, Demi-Human/Earth). Respawn: 120-130 min.
**Mini-boss:** Orc Hero (Level 77, spawns on fields or via Orc Memory instance)
**Notable drops:** Orc Lord Card (reflect damage), Orc Hero Card (immunity to Stun)

---

### 4.5 Byalan Island (Undersea Tunnel)

| Property | Value |
|----------|-------|
| **Location** | Reached by ship from Izlude |
| **Map IDs** | `iz_dun00` through `iz_dun05` |
| **Floors** | 6 |
| **Level Range** | 15-80 |
| **Entry** | Ship from Izlude |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Marina, Hydra, Plankton, Kukre | Very low-level water |
| F2 | Vadon, Cornutus, Marina | Crabs and shellfish |
| F3 | Merman, Marc, Swordfish | Marc stun is dangerous |
| F4 | Merman, Obeaune, Strouf | Mid-level water |
| F5 | Merman, Penomena, Deviace(Mini-boss) | Ranged attackers |
| F6 | King Dramoh, Sropho, Pot Dofle, Kraken(MVP) | Deep sea boss |

**MVP:** Kraken (Level 93, Water). Respawn: 120-130 min.
**Note:** Drake MVP is NOT in Byalan; Drake is in the Sunken Ship dungeon.

---

### 4.6 Sunken Ship (Treasure Island)

| Property | Value |
|----------|-------|
| **Location** | Accessible from Alberta or certain fields |
| **Map IDs** | `treasure01`, `treasure02` |
| **Floors** | 2 |
| **Level Range** | 30-60 |
| **Entry** | Walk from Alberta fields |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Whisper, Pirate Skeleton, Hydra | Ghostly ship |
| F2 | Drake(MVP), Ghostring, Mimic, Wanderer | Boss floor |

**MVP:** Drake (Level 77, Undead). Respawn: 120-130 min.
**Notable drops:** Drake Card (ignore DEF of normal monsters)

---

### 4.7 Ant Hell

| Property | Value |
|----------|-------|
| **Location** | South of Morroc (Sograt Desert) |
| **Map IDs** | `anthell01`, `anthell02` |
| **Floors** | 2 |
| **Level Range** | 15-40 |
| **Entry** | Walk from moc_fild02 |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| B1 | Ant Egg, Deniro, Andre, Piere, Vitata | Passive eggs, easy grinding |
| B2 | Andre, Piere, Vitata, Maya Purple, Maya(MVP) | Boss floor |

**MVP:** Maya (Level 81, Insect/Earth). Respawn: 120-130 min.
**Notable drops:** Maya Card (reflect single-target magic)

---

### 4.8 Pyramids (Morocc Pyramid)

| Property | Value |
|----------|-------|
| **Location** | Southeast of Morroc |
| **Map IDs** | `moc_pryd01` through `moc_pryd06` |
| **Floors** | 4 basement floors + 2 upper floors |
| **Level Range** | 30-75 (Pre-Renewal) |
| **Entry** | Walk from Morroc area |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Familiar, Poporing | Entry level |
| 2F | Mummy, Verit, Drainliar | Undead themed |
| B1 | Isis, Mummy, Mimic, Matyr | Mid-level |
| B2 | Minorous, Pasana, Ancient Mummy, Osiris(MVP) | Boss floor |

**MVP:** Osiris (Level 78, Undead/Undead). Respawn: 60-70 min.
**Notable drops:** Osiris Card (instant revive once)

---

### 4.9 Sphinx

| Property | Value |
|----------|-------|
| **Location** | West of Morroc (Sograt Desert) |
| **Map IDs** | `in_sphinx1` through `in_sphinx5` |
| **Floors** | 5 basement floors |
| **Level Range** | 30-80 (Pre-Renewal) |
| **Entry** | Walk from moc_fild03 area |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| B1 | Requiem, Marduk, Matyr | Entry, maze-like layout |
| B2 | Marduk, Pasana, Mimic | Mid-level |
| B3 | Anubis, Pasana, Minorous | High-level (popular EXP) |
| B4 | Anubis, Minorous, Osiris(MVP) | Alternate Osiris floor |
| B5 | Pharaoh(MVP), Anubis, Marduk | Boss floor |

**MVP:** Pharaoh (Level 93, Demi-Human). Respawn: 60-70 min.
**Mini-boss:** Amon Ra (Level 80, spawns on B2). Respawn: 60-70 min.
**Notable drops:** Pharaoh Card (SP cost -30%)

---

### 4.10 Clock Tower (Al De Baran)

| Property | Value |
|----------|-------|
| **Location** | Center of Al De Baran |
| **Map IDs** | `c_tower1` through `c_tower4`, `alde_dun01` through `alde_dun04` |
| **Floors** | 4 upper + 4 basement |
| **Level Range** | 40-90 (Pre-Renewal), 75-130 (Renewal) |
| **Entry** | Center of Al De Baran |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F-2F | Punk, Clock, Alarm | Animated clock monsters |
| 3F-4F | Rideword, High Orc, Clock | Upper floors |
| B1-B2 | Bathory, Punk, Penomena | Basement levels |
| B3 | Elder, Rideword, Joker | Deep basement |
| B4 | Owl Duke, Owl Baron, Clock Tower Manager(MVP) | Boss floor |

**MVP:** Clock Tower Manager (Level 74). Respawn: 120-130 min.

**Unknown Basement:** A hidden additional area accessible via special NPC, containing stronger versions of Clock Tower monsters and exclusive weapon drops (Clocktower Weapon Box).

---

### 4.11 Glast Heim

The largest dungeon complex in Ragnarok Online. Multiple wings, each effectively a separate dungeon.

| Property | Value |
|----------|-------|
| **Location** | Northwest of Al De Baran (Glast Heim fields) |
| **Map IDs** | Multiple `gl_*` maps |
| **Wings** | 8+ distinct areas |
| **Level Range** | 50-99 (Pre-Renewal), 70-130 (Renewal) |
| **Entry** | Walk through Glast Heim fields from Al De Baran |
| **Flags** | nosave, indoor (varies by wing) |

#### Glast Heim Castle (gl_cas01, gl_cas02)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Dark Frame, Wanderer, Raydric | Castle exterior |
| 2F | Raydric, Raydric Archer, Dark Lord(MVP) | Boss area |

**MVP:** Dark Lord (Level 90, Demon/Undead). Respawn: 480 min (8 hours).

#### Glast Heim Churchyard (gl_church)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Evil Druid, Wraith, Mimic | Undead priests |

#### Glast Heim Chivalry (gl_knt01, gl_knt02)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Raydric, Abysmal Knight, Khalitzburg | Knight quarters |
| 2F | Abysmal Knight, Khalitzburg, Bloody Knight | High-level undead knights |

**Mini-boss:** Abysmal Knight, Bloody Knight

#### Glast Heim St. Abbey (gl_sew01 and associated maps)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Wraith, Evil Druid, Rybio, Injustice | Cursed monastery |

#### Glast Heim Underprison (gl_prison, gl_prison01)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Zombie Prisoner, Skeleton Prisoner, Hunter Fly | Prison cells |
| 2F | Injustice, Rybio, Cramp | Deeper prison |

#### Glast Heim Culvert (gl_sew03, gl_sew04)
| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| 1F | Gargoyle, Anolian, Sting | Sewer system |
| 2F | Anolian, Sting, Arclouze | Popular leveling |

**Notable about Glast Heim:** This dungeon complex is the single most iconic dungeon in RO. Every wing has distinct monster themes. The entire area tells the story of a fallen capital city. It provides content spanning roughly levels 50-99 and is a rite of passage for all RO players.

---

### 4.12 Turtle Island

| Property | Value |
|----------|-------|
| **Location** | Island reached by ship from Alberta |
| **Map IDs** | `tur_dun01` through `tur_dun04` |
| **Floors** | 4 (inside the giant turtle) |
| **Level Range** | 55-85 |
| **Entry** | Ship from Alberta (quest required) |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Turtle, Spring Rabbit, Permeter | Entry inside turtle |
| F2 | Assaulter, Solider, Freezer | Mid-level turtles |
| F3 | Heater, Assaulter, Permeter | High-level |
| F4 | Turtle General(MVP), all turtle types | Boss floor |

**MVP:** Turtle General (Level 97, Brute/Earth). Respawn: 60-70 min.
**Notable drops:** Turtle General Card (+20% damage to all)

---

### 4.13 Toy Factory (Lutie)

| Property | Value |
|----------|-------|
| **Location** | North of Lutie |
| **Map IDs** | `xmas_dun01`, `xmas_dun02` |
| **Floors** | 2 |
| **Level Range** | 40-70 |
| **Entry** | Walk north from Lutie |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Cookie, Myst Case, Cruiser | Christmas-themed |
| F2 | Stormy Knight(MVP), Cookie, Myst Case | Boss floor |

**MVP:** Stormy Knight (Level 77, Formless/Wind). Respawn: 60-70 min.
**Notable drops:** Stormy Knight Card (chance to freeze attackers)

---

### 4.14 Bio Laboratory (Lighthalzen)

| Property | Value |
|----------|-------|
| **Location** | Beneath Rekenber Corporation, Lighthalzen |
| **Map IDs** | `lhz_dun01` through `lhz_dun04` |
| **Floors** | 4 (Floor 3 and 4 require quest access) |
| **Level Range** | 80-99+ |
| **Entry** | Quest: Biolabs Entrance Quest |
| **Flags** | noteleport, noreturn, nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Remover, Eremes Guile(1st class) | Human experiment subjects |
| F2 | 1st class doppelgangers (all types) | Full set of job class copies |
| F3 | Sniper Cecil, Whitesmith Howard, etc. (transcendent class mini-bosses) | Extremely dangerous |
| F4 | Random transcendent MVP + entourage | Wolchev's lab, hardest floor |

**MVPs (F3/F4):** Transcendent versions of all 2-1 classes:
- Sniper Cecil, Whitesmith Howard, High Wizard Kathryne, High Priest Margaretha, Lord Knight Seyren, Assassin Cross Eremes

**Design note:** Bio Lab is considered the hardest dungeon in classic RO. Floor 3 and 4 monsters have extremely high stats and mimic player skills. The "transcendent MVPs" randomly spawn on F4 with entourages of F3 mini-bosses.

---

### 4.15 Nameless Island / Abbey Dungeon

| Property | Value |
|----------|-------|
| **Location** | Island reached by ship from Veins |
| **Map IDs** | `abbey01` through `abbey03` |
| **Floors** | 3 |
| **Level Range** | 80-99+ |
| **Entry** | Quest: Nameless Island Entrance Quest |
| **Flags** | noteleport, noreturn, nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Ragged Zombie, Zombie Slaughter, Flame Skull | Cursed monastery |
| F2 | Banshee, Necromancer, Fallen Bishop Hibram(MVP) | Bishop boss |
| F3 | Hellhound, Necromancer, Beelzebub(MVP) | Lord of the Flies |

**MVPs:**
- Fallen Bishop Hibram (F2) -- Level 99
- Beelzebub (F3) -- 6,666,666 HP, AOE status effects, Hell Fly summons

**Design note:** Abbey is one of the most challenging end-game dungeons. Beelzebub is among the top 5 hardest MVPs in classic RO.

---

### 4.16 Abyss Lake

| Property | Value |
|----------|-------|
| **Location** | Southwest of Hugel |
| **Map IDs** | `abyss_01` through `abyss_03` (+ `abyss_04` instance) |
| **Floors** | 3 (+ 1 instance) |
| **Level Range** | 70-95 |
| **Entry** | Quest: bring Dragon Canine + Dragon Scale + Dragon Tail to pillar |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Green Ferus, Red Ferus | Dragon-type dungeon |
| F2 | Green/Red Ferus, Blue/Green Acidus | Mixed dragons |
| F3 | Acidus, Hydrolancer, Dragon MVP | Boss floor |

**All monsters are Dragon race.** Equipment from this dungeon can be enchanted with Dragon's Power.

---

### 4.17 Thor's Volcano

| Property | Value |
|----------|-------|
| **Location** | North of Veins |
| **Map IDs** | `thor_v01` through `thor_v03` |
| **Floors** | 3 |
| **Level Range** | 80-99+ |
| **Entry** | Walk from Veins north field |
| **Flags** | noteleport, noreturn, nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Kasa, Salamander, Imp | Fire-element heavy |
| F2 | Bow Guardian, Sword Guardian | LESS dangerous than F1/F3 |
| F3 | Ifrit(MVP), Kasa, Salamander | Boss floor |

**MVP:** Ifrit (Level 99, Fire/Formless). One of the most powerful MVPs.

**Unique design:** Unlike most dungeons, the middle floor is safer than the first and third floors.

---

### 4.18 Rachel Sanctuary (Ice Cave / Ice Dungeon)

| Property | Value |
|----------|-------|
| **Location** | North of Rachel |
| **Map IDs** | `ice_dun01` through `ice_dun03` (+ `ice_dun04`) |
| **Floors** | 3 (+ boss arena) |
| **Level Range** | 70-95 |
| **Entry** | Walk from Rachel fields, quest for deeper floors |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Vanberk, Isilla | Water-element monsters |
| F2 | Snowier, Siroma, Ice Titan | Stronger ice monsters |
| F3 | Ktullanux(MVP, summoned), Gazeti | Boss arena |

**MVP:** Ktullanux (summoned via Ice Necklace Quest -- use 4 Freezing Snow Powder on sacred fires). Respawn: 2 hours after kill.

**Special mechanic:** Defeating Ktullanux temporarily opens portals to F4.

---

### 4.19 Thanatos Tower

| Property | Value |
|----------|-------|
| **Location** | Near Hugel fields |
| **Map IDs** | `tha_t01` through `tha_t12` |
| **Floors** | 12+ (7 in instance version) |
| **Level Range** | 80-99+ |
| **Entry** | Quest: Thanatos Tower Quest (party required) |
| **Flags** | noteleport, noreturn, nosave, indoor |

| Section | Key Monsters | Notes |
|---------|-------------|-------|
| F1-F6 | Observation, Happy Giver, Empathizer, Solace, Retribution | Memory-themed floors |
| F7-F12 | Dolor/Maero/Despero/Odium of Thanatos | Fragment bosses |
| Top | Thanatos(MVP) | Final boss (requires mechanics to weaken) |

**MVP:** Thanatos (Level 99). Must destroy correct memory statues to weaken him. Spawns infinite slave monsters if wrong statue destroyed.

**Fragments required:** Hatred, Agony, Misery, Despair (collected per floor)

---

### 4.20 Juperos Ruins

| Property | Value |
|----------|-------|
| **Location** | Far west Yuno fields |
| **Map IDs** | `jupe_cave`, `jupe_area1`, `jupe_area2`, `jupe_core` |
| **Floors** | 3 + Core |
| **Level Range** | 75-95 |
| **Entry** | F1-F2 open, Core requires quest |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Venatu (multiple colors), Dimik | Robot dungeon |
| F2 | Venatu, Dimik, Archdam | Stronger robots |
| Core | Archdam, Dimik, Vesper(MVP) | Boss area (waves of robots) |

**MVP:** Vesper (Level 97, Formless). Respawn: 120-130 min. Giant mechanical behemoth.

---

### 4.21 Odin's Temple (Shrine)

| Property | Value |
|----------|-------|
| **Location** | Angrboda Island, off Hugel coast |
| **Map IDs** | `odin_tem01` through `odin_tem03` |
| **Floors** | 3 |
| **Level Range** | 80-99+ |
| **Entry** | Walk through Hugel fields |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Skogul, Frus, Plasma | Norse-themed |
| F2 | Skeggiold (Black/Blue), Frus | Valkyrie servants |
| F3 | Valkyrie Randgris(MVP), Valkyrie | Boss floor |

**MVP:** Valkyrie Randgris (Level 99). Respawn: 480 min (8 hours). Extremely powerful.

---

### 4.22 Einbech Mine Dungeon

| Property | Value |
|----------|-------|
| **Location** | Beneath Einbech (sister town of Einbroch) |
| **Map IDs** | `ein_dun01`, `ein_dun02` |
| **Floors** | 2 |
| **Level Range** | 65-90 (Pre-Renewal), 85-100 (Renewal) |
| **Entry** | Walk into Einbech mine |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Waste Stove, Venatus, Porcellio | Mine monsters |
| F2 | RSX-0806(MVP), Venatus, Dimik | Robot boss |

**MVP:** RSX-0806 (Level 90). Respawn: 125-135 min.

---

### 4.23 Coal Mine (Dead Pit)

| Property | Value |
|----------|-------|
| **Location** | Northeast of Geffen (Mt. Mjolnir area) |
| **Map IDs** | `mjolnir_12`, `coal_mine01` through `coal_mine03` |
| **Floors** | 3 |
| **Level Range** | 20-50 |
| **Entry** | Walk from Mt. Mjolnir fields |
| **Flags** | nosave, indoor |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Skeleton Worker, Steel Chonchon | Abandoned mine |
| F2 | Skeleton Worker, Megalodon | Deeper mine |
| F3 | Skeleton General, Skeleton Prisoner | Deepest |

---

### 4.24 Magma Dungeon (Nogg Road)

| Property | Value |
|----------|-------|
| **Location** | Near Yuno / Schwartzvald region |
| **Map IDs** | `mag_dun01`, `mag_dun02`, `mag_dun03` |
| **Floors** | 3 (F3 added in Renewal, requires Level 175) |
| **Level Range** | 60-99+ |
| **Entry** | Walk from Yuno fields |
| **Flags** | nosave |

| Floor | Key Monsters | Notes |
|-------|-------------|-------|
| F1 | Blazer, Explosion, Lava Golem | Fire-element |
| F2 | Nightmare Terror, Knocker | Stronger fire mobs |
| F3 | Enhanced Magma monsters | Renewal-only, high EXP |

---

### 4.25 Additional Notable Dungeons (Summary)

| Dungeon | Location | Floors | MVP | Level Range |
|---------|----------|--------|-----|-------------|
| **Amatsu Dungeon** | Inside Amatsu Castle | 3 | Incantation Samurai | 50-80 |
| **Louyang Royal Tomb** | Below Louyang | 3 | White Lady (Bacsojin) | 50-85 |
| **Ayothaya Ancient Shrine** | Ayothaya fields | 2 | Lady Tanee | 55-80 |
| **Moscovia Forest** | Moscovia island | 3 | None (Baba Yaga mini) | 55-85 |
| **Comodo Beach Dungeon** | South of Comodo | 3 | None | 35-60 |
| **Umbala Dungeon** | Umbala jungle | 2 | None (leads to Niflheim) | 40-65 |
| **Gonryun Dungeon** | Kunlun (floating island) | 3 | Evil Snake Lord | 50-80 |
| **Nidhoggur's Dungeon** | Below Yggdrasil | 2 | Nidhoggur's Shadow | 80-99 |
| **Kiel Hyre's Academy** | Near Juno | 2 | Kiel-D-01 | 75-95 |
| **Endless Tower** | Instance (Misty Island) | 100 | Various (all MVPs) | 50-99+ |

---

## 5. Warp System

### 5.1 Warp Portal (Priest Skill)

| Property | Value |
|----------|-------|
| **Skill ID** | AL_WARP (27) |
| **Class** | Acolyte / Priest / High Priest / Arch Bishop |
| **Max Level** | 4 |
| **Catalyst** | 1 Blue Gemstone per cast |
| **Max Players** | 8 per portal |
| **Active Portals** | Max 3 simultaneously |

**Level progression:**
| Level | Destinations Available |
|-------|----------------------|
| 1 | Save Point only |
| 2 | Save Point + 1 memo point |
| 3 | Save Point + 2 memo points |
| 4 | Save Point + 3 memo points |

**Memo system:**
- Type `/memo` in chat to memorize current map location
- Maximum 3 memo slots
- New memos overwrite the oldest slot if all 3 are full
- Only 1 memo per unique map
- Some maps block memo (nomemo flag)

**Restrictions:**
- Cannot cast on maps with `noteleport` flag
- Cannot memo on maps with `nomemo` flag
- Cannot warp TO maps with certain restrictions
- Blue Gemstone consumed on cast, not on destination selection

---

### 5.2 Kafra Teleport Service

Kafra NPCs are stationed in every major town and provide teleportation for a Zeny fee.

**Standard Kafra destinations by town:**

| Town | Destinations | Cost Range |
|------|-------------|------------|
| **Prontera** | Geffen, Payon, Morroc, Alberta, Orc Dungeon | 600-1800z |
| **Geffen** | Prontera, Payon, Alberta, Morroc | 600-1200z |
| **Payon** | Prontera, Alberta, Morroc | 1200z each |
| **Morroc** | Prontera, Alberta, Payon | 600-1200z |
| **Alberta** | Prontera, Payon, Morroc | 600-1200z |
| **Al De Baran** | Prontera, Geffen | 1200z |
| **Yuno** | Al De Baran | 1200z |
| **Comodo** | Prontera, Umbala | 1200z |

**Schwartzvald Kafra (Cool Event Corp):**
- Separate corporation from Rune-Midgarts Kafra
- Provides storage and teleport in Schwartzvald towns
- Destinations: inter-Schwartzvald town connections

**Additional Kafra services:**
- **Storage** -- Access personal storage (shared across Kafra NPCs)
- **Save Point** -- Set respawn point to current location
- **Cart Rental** -- For Merchant class

**Free Ticket for Kafra Transportation:** A consumable item that waives the Zeny cost for one teleport.

---

### 5.3 Fly Wing (Item ID: 601)

| Property | Value |
|----------|-------|
| **Effect** | Teleport to random location on current map |
| **Buy Price** | 60z |
| **Weight** | 5 |
| **Usable** | Most outdoor maps |

**Blocked by:** `noteleport` map flag (most dungeons)

---

### 5.4 Butterfly Wing (Item ID: 602)

| Property | Value |
|----------|-------|
| **Effect** | Teleport to save point |
| **Buy Price** | 300z |
| **Weight** | 5 |
| **Usable** | Most maps |

**Blocked by:** `noreturn` map flag (many dungeons)

**Colored Butterfly Wings** (special items): Teleport to specific towns.

---

### 5.5 Other Teleportation Methods

| Method | Details |
|--------|---------|
| **Teleport skill (Acolyte)** | Level 1: random spot. Level 2: save point. No gem cost. |
| **@go command** | GM-only town teleport |
| **Airship** | Automatic route between towns (boarding + riding) |
| **Ship routes** | NPC-operated, fixed destinations with Zeny cost |
| **Dead Branch** | Summons random monster (not teleport, but notable) |
| **Giant Fly Wing** | Teleports entire party to random location |

---

## 6. Map Properties and Flags

### 6.1 Map Flag Reference (rAthena Standard)

Map flags control what actions are allowed on each map. Based on the rAthena server emulator standard.

#### Core Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `town` | Marks as safe zone, no monster spawns | All towns |
| `pvp` | Enables Player vs Player combat | PvP arenas |
| `gvg` | Enables Guild vs Guild combat | War of Emperium castles |
| `gvg_castle` | WoE castle map (special rules) | Agit castles |
| `gvg_dungeon` | WoE dungeon map | Castle dungeons |

#### Teleportation Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `noteleport` | Blocks Fly Wing, Teleport skill, @jump | Deep dungeons |
| `noreturn` | Blocks Butterfly Wing, @return | Dungeons, boss rooms |
| `nowarp` | Blocks @go, @warp TO another map | Restricted areas |
| `nowarpto` | Blocks @warp TO this map | Boss rooms, instances |
| `nomemo` | Blocks /memo (Warp Portal memory) | Dungeons, instances |

#### Save/Respawn Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `nosave` | Cannot set save point here | All dungeons, fields |
| `nopenalty` | No EXP/Zeny loss on death | PvP arenas, instances |
| `noexppenalty` | No EXP loss on death | Special areas |
| `nozenypenalty` | No Zeny loss on death | Special areas |

#### Combat/Skill Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `noskill` | All skills disabled | Safe zones, lobbies |
| `restricted` | Certain items/skills restricted | PvP balance maps |
| `noicewall` | Ice Wall skill blocked | Boss rooms |

#### Item Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `nodrop` | Cannot drop items | Town centers |
| `notrade` | Cannot trade items | Instances |
| `novending` | Cannot open vend shop | Dungeon maps |

#### Environment Flags

| Flag | Description | Typical Maps |
|------|-------------|-------------|
| `indoor` | Indoor lighting model | Dungeons, buildings |
| `clouds` | Cloud weather effect | High-altitude maps |
| `fog` | Fog weather effect | Niflheim, swamps |
| `fireworks` | Firework particles | Comodo |
| `leaves` | Falling leaf particles | Payon, forests |
| `snow` | Snow weather | Lutie, Rachel fields |
| `rain` | Rain weather | Various fields |
| `night` | Night-time lighting | Niflheim |

### 6.2 Common Flag Combinations

```
TOWN:       town, nosave(redirect to self)
FIELD:      (no special flags, or) nosave
DUNGEON:    noteleport, noreturn, nosave, indoor
BOSS ROOM:  noteleport, noreturn, nosave, nowarpto, indoor
PVP ARENA:  pvp, nopenalty, noreturn
GVG CASTLE: gvg, gvg_castle, noreturn
INSTANCE:   noteleport, noreturn, nosave, nowarpto, nowarp
```

### 6.3 Map Type Distribution

| Type | Count (Approx.) | Description |
|------|-----------------|-------------|
| Town | ~25 | Safe zones with NPCs |
| Field | ~180 | Outdoor connecting zones |
| Dungeon | ~150+ | Underground/indoor combat zones |
| PvP/GvG | ~30 | Player combat arenas and castles |
| Instance | ~20 | Instanced dungeons (party/solo) |
| Other | ~15 | Airship, special areas |

---

## 7. UE5 Implementation

### 7.1 Level Naming Convention

All UE5 levels use the `L_` prefix. Never use `Map_` or other prefixes.

```
L_Prontera          -- Prontera town
L_PrtSouth          -- Prontera South Field (prt_fild08)
L_PrtNorth          -- Prontera North Field (prt_fild01)
L_PrtDungeon01      -- Prontera Culvert Floor 1
L_Geffen            -- Geffen town
L_GefDungeon01      -- Geffen Tower Floor 1
L_Payon             -- Payon town
L_PayDungeon01      -- Payon Cave Floor 1
L_Morroc            -- Morroc town
L_GlastHeim         -- Glast Heim hub
L_GlastHeimChivalry -- Glast Heim Chivalry wing
```

**Naming pattern:**
- Towns: `L_{TownName}` (e.g., `L_Prontera`, `L_Geffen`)
- Fields: `L_{TownAbbrev}{Direction}` (e.g., `L_PrtSouth`, `L_PrtNorth`)
- Dungeons: `L_{TownAbbrev}Dungeon{Floor}` (e.g., `L_PrtDungeon01`, `L_PrtDungeon02`)
- Multi-wing dungeons: `L_{DungeonName}{Wing}` (e.g., `L_GlastHeimChivalry`)

All levels saved to: `Content/SabriMMO/Levels/`

### 7.2 Level Creation Workflow

**Critical rule: ALWAYS duplicate an existing level.** Never create a new empty level.

1. Right-click an existing working level (e.g., `L_PrtSouth`) in Content Browser
2. **Duplicate** -> Rename to new level name
3. This copies the Level Blueprint, World Settings, and placed actors
4. Modify geometry, spawns, and warp portals as needed

**Why duplication matters:**
- The Level Blueprint handles character spawning, save-position timer, and cleanup
- World Settings must have `GM_MMOGameMode` override with `DefaultPawnClass = None`
- Placed actors (`BP_SocketManager`, `BP_OtherPlayerManager`, `BP_EnemyManager`) are required
- Creating from scratch risks missing any of these critical components

### 7.3 Level Blueprint Structure

Every game level must have a Level Blueprint with three sections:

**A. Character Spawn (Event BeginPlay):**
```
Event BeginPlay
  -> Delay 0.2s
  -> Cast To MMOGameInstance (Get Game Instance)
  -> Get Selected Character -> Break Character Data
  -> Branch: CharacterId > 0?
    -> Branch: Has saved location? (Vector Length != 0)
      -> True:  SpawnActor BP_MMOCharacter at (X, Y, Z) -> Possess
      -> False: SpawnActor BP_MMOCharacter at default (0, 0, 900) -> Possess
  -> Set Timer "SaveCharacterPosition" (5s, Looping)
```

**B. Save Position (Custom Event, every 5s):**
```
SaveCharacterPosition
  -> Cast To MMOGameInstance -> Get Selected Character -> CharacterId
  -> Get Player Character -> Get Actor Location -> X, Y, Z
  -> Save Character Position (CharacterId, X, Y, Z)
```

**C. Cleanup (Event End Play):**
```
Event End Play
  -> Clear Timer "SaveCharacterPosition"
```

### 7.4 Required Blueprint Actors Per Level

Every game level must contain these actors:

| Blueprint | Location | Purpose |
|-----------|----------|---------|
| `BP_SocketManager` | (0, 0, 0) | Socket.io connection hub |
| `BP_OtherPlayerManager` | (0, 0, 0) | Remote player spawning/interpolation |
| `BP_EnemyManager` | (0, 0, 0) | Enemy spawning and management |
| `Nav Mesh Bounds Volume` | Cover playable area | AI pathfinding |
| `Directional Light` | Sky | Sunlight (outdoor) |
| `Sky Atmosphere` + `Sky Light` | Sky | Ambient lighting (outdoor) |

For towns, also add:
| Blueprint | Purpose |
|-----------|---------|
| `AWarpPortal` actors | Zone transition triggers |
| `AKafraNPC` actors | Kafra teleport/save/storage NPCs |
| `AShopNPC` actors | Weapon/armor/tool dealers |

### 7.5 Zone Registry Structure

The server-side zone registry (`server/src/ro_zone_data.js`) defines all zone metadata:

```javascript
const ZONE_REGISTRY = {
    zone_name: {
        name: 'zone_name',              // Unique ID (DB, socket rooms)
        displayName: 'Display Name',     // UI display
        type: 'town' | 'field' | 'dungeon',
        flags: {
            noteleport: false,           // Block Fly Wing
            noreturn: false,             // Block Butterfly Wing
            nosave: false,               // Block save point
            pvp: false,                  // Enable PvP
            town: true,                  // Safe zone flag
            indoor: false                // Indoor lighting
        },
        defaultSpawn: { x, y, z },       // Entry point
        levelName: 'L_LevelName',        // UE5 level file
        warps: [
            {
                id: 'warp_id',
                x, y, z,                 // Portal position
                radius: 200,             // Trigger radius
                destZone: 'target_zone',
                destX, destY, destZ       // Destination coords
            }
        ],
        kafraNpcs: [
            {
                id: 'kafra_id',
                name: 'Kafra Employee',
                x, y, z,
                destinations: [
                    { zone: 'dest_zone', displayName: 'Name', cost: 1200 }
                ]
            }
        ],
        enemySpawns: [
            { template: 'monster_id', x, y, z, wanderRadius: 400 }
        ]
    }
};
```

**Current zones implemented:** `prontera`, `prontera_south`, `prontera_north`, `prt_dungeon_01`

### 7.6 Zone Transition System

The C++ subsystem `ZoneTransitionSubsystem` manages zone changes:

1. Player touches `AWarpPortal` trigger volume
2. Subsystem sends `zone:change` socket event with warp ID
3. Server validates warp, sends back `zone:warp` with destination data
4. Client shows loading overlay, saves state to `MMOGameInstance`
5. `OpenLevel()` loads the target level (`L_*`)
6. New level's Level Blueprint spawns character at destination coordinates
7. Subsystem sends `zone:ready` to server, server joins player to new zone's socket room

**Key GameInstance state during transition:**
```cpp
FString CurrentZoneName;
FString PendingZoneName;
FString PendingLevelName;
FVector PendingSpawnLocation;
bool bIsZoneTransitioning;
```

### 7.7 2D to 3D Translation (RO Cells to UE Units)

RO Classic uses a 2D isometric tile grid. Each tile is called a "cell."

**Conversion factor:** 50 UE units = 1 RO cell

| RO Measurement | UE5 Equivalent |
|---------------|----------------|
| 1 cell | 50 UE units |
| 10 cells | 500 UE units |
| Map 400x400 cells | 20,000 x 20,000 UE units |
| Player movement speed (base) | ~150 UE units/sec |
| Aggro range (typical) | 550 UE units (11 cells) |
| Warp portal trigger | 200 UE units radius (4 cells) |

**Practical map sizes:**
| RO Map | RO Size | UE5 Approximate Size |
|--------|---------|---------------------|
| Prontera town | 312x392 cells | ~5,000 x 6,000 UE units (scaled for playability) |
| Standard field | 400x400 cells | ~20,000 x 20,000 UE units |
| Standard dungeon floor | 200-320 cells | ~10,000-16,000 UE units |

**Height (Z-axis):** RO is essentially 2D. In UE5, terrain height is artistic. Default ground level is approximately Z=300 for fields, Z=500-600 for towns (due to terrain elevation).

### 7.8 Terrain Workflow

For creating new zone geometry:

1. **Town maps:** Use static mesh blockouts (cubes, cylinders) for buildings, walls, gates
2. **Field maps:** Use UE5 Landscape tool with sculpted terrain
3. **Dungeon maps:** Use modular BSP/mesh pieces for corridor-based layouts
4. **All maps:** Place Nav Mesh Bounds Volume covering entire playable area

**Material approach:**
- Starter/prototype: Use basic colored materials per zone theme
- Production: Use material instances with tiling textures

### 7.9 Warp Portal Actors

`AWarpPortal` is a C++ actor with overlap trigger:

```
Properties to set in editor:
  - WarpId: matches zone registry warp ID (e.g., "prt_south_exit")
  - Box extent: collision trigger size
  - Location: matches zone registry coordinates
```

The server resolves the warp ID to destination zone and coordinates. The client does not need to know destination details -- it just sends the warp ID.

### 7.10 Implementation Priority

Based on the existing Sabri_MMO zone system (4 zones implemented), here is a suggested rollout order:

**Phase 1 -- Core Rune-Midgarts (Current + Near-term):**
1. Prontera (done)
2. Prontera South Field (done)
3. Prontera North Field (done)
4. Prontera Culvert F1 (done)
5. Geffen town + Geffen fields
6. Payon town + Payon fields
7. Morroc town + Sograt Desert fields
8. Alberta town
9. Izlude town

**Phase 2 -- Major Dungeons:**
1. Prontera Culvert F2-F4
2. Payon Dungeon (5 floors)
3. Geffen Tower (4 floors)
4. Orc Dungeon (2 floors)
5. Ant Hell (2 floors)
6. Pyramids (6 floors)

**Phase 3 -- Extended Rune-Midgarts:**
1. Al De Baran + Clock Tower
2. Comodo + Umbala + Niflheim
3. Lutie + Toy Factory
4. Byalan Island
5. Sunken Ship
6. Sphinx (5 floors)
7. Glast Heim (all wings)

**Phase 4 -- Schwartzvald:**
1. Juno + Yuno fields
2. Einbroch + Einbech Mine
3. Lighthalzen + Bio Lab
4. Hugel + associated dungeons
5. Abyss Lake, Odin's Temple, Thanatos Tower

**Phase 5 -- Arunafeltz + Foreign Lands:**
1. Rachel + Ice Cave
2. Veins + Thor's Volcano
3. Nameless Island + Abbey
4. Amatsu, Louyang, Ayothaya
5. Moscovia, Jawaii, Turtle Island

---

## References

- [iRO Wiki -- World Map](https://irowiki.org/classic/World_Map)
- [iRO Wiki -- Kafra](https://irowiki.org/wiki/Kafra)
- [iRO Wiki -- Warp Portal](https://irowiki.org/wiki/Warp_Portal)
- [RateMyServer -- Map Database](https://ratemyserver.net/index.php?page=map_db)
- [RateMyServer -- Dungeon Maps](https://ratemyserver.net/dungeonmap.php)
- [RateMyServer -- Interactive World Map](https://ratemyserver.net/worldmap.php)
- [Ragnarok Wiki -- Category: Dungeons](https://ragnarok.fandom.com/wiki/Category:Dungeons)
- [Ragnarok Wiki -- Cities, Towns and Places](https://pro.fandom.com/wiki/Cities,_Towns_and_Places)
- [rAthena -- Mapflag Documentation](https://github.com/rathena/rathena/wiki/Mapflag)
- [rAthena -- Mapflags Source](https://github.com/rathena/rathena/blob/master/doc/mapflags.txt)
- [iRO Wiki -- Leveling Spots](https://irowiki.org/wiki/Leveling_Spots)
- [iRO Wiki -- MVP List](https://irowiki.org/wiki/MVP)
- [StrategyWiki -- Ragnarok Online Dungeons](https://strategywiki.org/wiki/Ragnarok_Online/Dungeons)
