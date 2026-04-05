# RO Classic Map, Minimap & World Map — Complete Research

> Compiled from 8 parallel deep-research agents covering rAthena source, Hercules, roBrowser, Ragnarok Research Lab, iRO Wiki, and 40+ additional sources. All findings confirmed pre-renewal (Classic) unless explicitly noted.

---

## Table of Contents
1. [Minimap System](#1-minimap-system)
2. [World Map System](#2-world-map-system)
3. [Map Grid & Coordinate System](#3-map-grid--coordinate-system)
4. [Cell Types & Terrain](#4-cell-types--terrain)
5. [Map Flags](#5-map-flags)
6. [Map Naming Conventions](#6-map-naming-conventions)
7. [Zone Connections & Warp System](#7-zone-connections--warp-system)
8. [World Topology & Complete Map List](#8-world-topology--complete-map-list)
9. [Map Loading & Transitions](#9-map-loading--transitions)
10. [Navigation & UI Commands](#10-navigation--ui-commands)
11. [Map-Related Status Effects](#11-map-related-status-effects)
12. [Client Configuration Files](#12-client-configuration-files)
13. [Pre-Renewal vs Renewal Feature Matrix](#13-pre-renewal-vs-renewal-feature-matrix)

---

## 1. Minimap System

### 1.1 Position, Shape & Size
- **Position**: Upper-right corner of screen (`top: 18px, right: 18px` — roBrowser CSS)
- **Shape**: Rectangular with sharp corners (NOT circular)
- **Canvas size**: 128x128 pixels (fixed, cannot be resized or moved)
- **No visible border or frame** around the minimap
- **Two small zoom buttons** (12x12px each) stacked vertically at top-right of minimap:
  - Plus: `map/map_plus0.bmp` (normal), `map/map_plus1.bmp` (hover)
  - Minus: `map/map_minus0.bmp` (normal), `map/map_minus1.bmp` (hover)

### 1.2 Display Modes (Ctrl+Tab)
Three states cycled with **Ctrl+Tab**:
| State | Opacity | Description |
|-------|---------|-------------|
| 0 | Hidden | Minimap completely invisible |
| 1 | 50% | Semi-transparent (see game world through map) |
| 2 | 100% | Fully opaque (default on startup) |

Preference saved across sessions: `{ zoom: 0, opacity: 2 }`

### 1.3 Zoom Levels
5 zoom factors: `[1, 10, 6, 3, 2]` (from roBrowser source)
- Level 0 (factor 1): Shows **entire map** scaled to fit 128x128 canvas
- Levels 1-4: Progressive zoom centered on player position
- Controlled via +/- buttons (no scroll wheel on minimap)

### 1.4 Minimap Source Images (BMP Files)
- **Format**: BMP, 24-bit color (256-color indexed also works)
- **Typical resolution**: 512x512 pixels (some maps may differ)
- **Transparency color**: Magenta (#FF00FF) — R and B > 0xF0, G < 0x10 = transparent
- **Walkable areas**: Colored pixels representing terrain shape
- **Non-walkable areas**: Magenta (transparent)
- **File path**: `data/texture/유저인터페이스/map/{mapname}.bmp`
  - `유저인터페이스` = Korean for "user interface" (CP949 encoding)
- **Pre-rendered assets** — NOT generated at runtime. Created during map development.
- **resnametable.txt** provides aliasing: custom maps can reuse existing minimap BMPs

### 1.5 Minimap Indicators (What Is Shown)

| Element | Visual | Color | Size | Notes |
|---------|--------|-------|------|-------|
| **Player** | Directional arrow | White | Arrow sprite | Rotates with facing direction (8 dirs, 45° each) |
| **Party members** | Square dot | Random RGB per member | 6x6px white border, 4x4px colored fill | Same map only |
| **Guild members** | Upward triangle | Pink `rgb(245,175,200)` | ~8px with white stroke (lineWidth: 2) | Same map only |
| **Warp portals** | Dot | Red | Small | Map exits to other zones |
| **Guide NPC marks** | Cross/plus (+) | Custom color per mark | 2x8 + 8x2 px rectangles | Blink 500ms on/off cycle |
| **Other players** | NOT shown | — | — | Only party/guild visible |
| **Monsters** | NOT shown | — | — | Never on Classic minimap |
| **NPCs** | NOT shown | — | — | Only Guide NPC marks (manual) |
| **Items on ground** | NOT shown | — | — | — |

**Arrow rendering**: `ctx.rotate((direction + 4) * 45 * Math.PI / 180)` — loaded from `map_arrow.bmp`

### 1.6 Guide NPC Mark System (Pre-Renewal Only Way to Mark Minimap)
- Talk to a **Guide NPC** in any town
- Guide places colored **+ (cross) marks** at facility locations (weapon shop, inn, tool dealer, etc.)
- Each facility type gets a different color
- Marks persist until: player walks to marked location, Guide clears them, or timer expires
- Server script command: `viewpoint <type>, <x>, <y>, <id>, <color>;`
  - Type 0: Display for 15 seconds
  - Type 1: Display until death/teleport
  - Type 2: Remove mark by ID
  - Color: `0x00RRGGBB` hex

### 1.7 Map Name Display
- Current map's **display name** shown at top of minimap frame
- Simple white/light text, small sans-serif font
- Display name (e.g., "Prontera") NOT the internal ID (e.g., `prontera`)

### 1.8 Minimap Rotation
- **Fixed north-up** — does NOT rotate with camera
- Only the player arrow rotates to show facing direction
- "North" on minimap = upper-right direction in the isometric game world

### 1.9 Minimap Interaction
- **NOT clickable** for navigation in Classic
- **No drag/move** — fixed position
- **No resize** — fixed 128x128
- **No fog of war** — entire map visible immediately on entry
- **No exploration tracking** — complete map shown at all times

### 1.10 Indoor/Dungeon Maps
- Handled **identically** to outdoor maps
- Each map has its own unique minimap BMP
- No special rendering mode or visual distinction (purely art choice in BMP)

---

## 2. World Map System

### 2.1 Availability
- **NOT in original RO Classic** — added with **Episode 12: Nightmare of Midgard** (December 5, 2007 on kRO)
- Still pre-renewal (Renewal came 2010), so it IS part of late Classic era
- Before Episode 12: players relied on third-party fan maps and community wikis

### 2.2 Opening & Controls
| Shortcut | Action |
|----------|--------|
| **Ctrl + ` (backtick)** | Open/close world map |
| **Alt** (while open) | Toggle field/zone name labels |
| **Tab** (while open) | Toggle monster map overlay |
| **Esc** | Close world map |

### 2.3 Appearance
- **Full-screen overlay** covering entire game window
- **Hand-painted fantasy cartographic illustration** (`worldmap.bmp`)
- NOT a schematic grid — illustrated overhead continent map like a fantasy atlas page
- Shows: green grasslands, brown deserts, white snow areas, mountain ranges, blue ocean, rivers
- Style consistent with RO's anime/manhwa aesthetic

### 2.4 Zone Overlay System
- Individual field zones defined as **invisible rectangular bounding boxes** on the background image
- Hovering over a zone: highlights it and shows minimap thumbnail preview popup
- Each zone rectangle: `(x1, y1, x2, y2)` pixel coordinates on worldmap.bmp
- **Cannot click to teleport** — informational only

### 2.5 World Map Information
**Per zone (with overlays):**
- Zone/field display name (toggle with Alt)
- Minimap preview popup on hover
- Monster names (with monster map enabled via Tab)
- Monster levels and recommended level ranges

**Always visible:**
- **Your current location** — white arrow
- **Party members** — pink/magenta dots (ANY map, cross-zone visible)
- **Guild members** — pink triangles
- **Town locations** — marked on painted background
- **Dungeon entrance markers** — small indicators on world map

**NOT shown:**
- Individual NPC locations (Classic)
- Kafra routes
- Warp Portal memo points
- Saved locations

### 2.6 Zoom/Pan
- **No zoom, no pan** in Classic world map
- Single fixed-size full-screen image — see entire continent at once

### 2.7 Regions
The world map shows the **Midgard continent** as one continuous landmass:
- **Rune-Midgarts Kingdom** (south) — capital Prontera, 50+ areas
- **Republic of Schwarzwald** (northeast) — capital Juno, ~36 areas
- **Arunafeltz States** (northwest) — capital Rachel
- **Island nations**: Amatsu, Louyang, Kunlun, Ayothaya, Niflheim, Lutie, Turtle Island, etc.

**No tabs to switch continents** — all on one image in Classic. A "Dimension" tab was added later for expansion areas.

### 2.8 Client Data Structure

**Files** (in GRF at `data/luafiles514/lua files/worldviewdata/`):
| File | Purpose |
|------|---------|
| `worldviewdata_list.lua` | Region list (Midgard, Dimension, etc.) |
| `worldviewdata_table.lua` | Zone bounding boxes per region (140+ field/town entries) |
| `worldviewdata_language.lua` | Display names for zones (365 entries) |
| `worldviewdata_info.lua` | UI styling (colors, fonts, alpha) |
| `worldviewdata_f.lua` | Accessor functions |
| `worldmap.bmp` | Main Midgard background image (in texture folder) |

**Zone definition format** (from `worldviewdata_table.lua`):
```lua
{ RegionID, "mapname.rsw", x1, y1, x2, y2, WORLD_MSGID.MSI_ID, "levelRange" }
```

**UI styling** (from `worldviewdata_info.lua`):
- Dungeon markers: Red (255, 0, 0)
- Monster names: Green (128, 255, 128)
- Monster levels: White (255, 255, 255)
- Selection box: Dark red (149, 15, 15)

---

## 3. Map Grid & Coordinate System

### 3.1 Cell System
- 1 cell = 1 GAT tile = **5 world units** (hardcoded)
- GND cubes = 10 world units = 2x2 GAT tiles
- Origin **(0,0) at bottom-left (southwest)**
- X increases east, Y increases north
- Maximum: `MAX_MAP_SIZE = 512 x 512 = 262,144 cells`

### 3.2 Typical Map Dimensions
| Map Type | Typical Size | Example |
|----------|-------------|---------|
| Small indoor | 30x30 to 100x100 | `in_prontera` |
| Town | 200x200 to 400x400 | `prontera` (312x392) |
| Field | 300x300 to 400x400 | `prt_fild08` |
| Dungeon | 150x150 to 300x300 | Varies widely |

### 3.3 Spatial Partitioning
- Server uses `BLOCK_SIZE = 8` cells for spatial hash grid
- Block position: `x/8 + (y/8) * bxs`
- Separate block grids for mobs (`block_mob[]`) and non-mobs (`block[]`)

### 3.4 Boundary Handling
- `battle_config.map_edge_size` = 15 cells (default edge margin for spawns)
- Last row/column treated as walls
- Out-of-bounds → `CELL_CHKNOPASS`

---

## 4. Cell Types & Terrain

### 4.1 GAT Terrain Types
| GAT Type | Walkable | Snipeable | Water | Description |
|----------|----------|-----------|-------|-------------|
| 0 | Yes | Yes | No | Standard walkable ground |
| 1 | No | No | No | Wall/obstruction |
| 2 | No | No | Yes | Non-walkable water (not snipeable) |
| 3 | Yes | Yes | Yes | Walkable water |
| 4 | No | Yes | Yes | Non-walkable water (snipeable) |
| 5 | No | Yes | No | Cliff (ranged attacks pass, can't walk) |
| 6 | No | No | No | Cliff (not snipeable) |

### 4.2 Dynamic Cell Flags (Set at Runtime)
| Flag | Set By | Purpose |
|------|--------|---------|
| `CELL_NPC` | Warp/NPC scripts | NPC touch-trigger area |
| `CELL_BASILICA` | Basilica skill | Basilica zone |
| `CELL_LANDPROTECTOR` | Land Protector skill | Ground skill blocker |
| `CELL_ICEWALL` | Ice Wall skill | Temporary wall |
| `CELL_NOVENDING` | Script/mapflag | Block vending |
| `CELL_NOCHAT` | Script/mapflag | Block chat rooms |
| `CELL_MAELSTROM` | Maelstrom skill | Maelstrom zone |
| `CELL_NOBUYINGSTORE` | Script/mapflag | Block buying stores |

### 4.3 Water Detection
- Water level stored in RSW file (byte offset 166)
- Cells are underwater when average of 4 corner altitudes > water level
- GAT v1.3+: bit flag `0x08` in TerrainType marks water tiles

---

## 5. Map Flags

### 5.1 Key Map Flags (81+ total in rAthena)

**Movement/Teleportation:**
- `MF_NOMEMO` — Cannot use /memo
- `MF_NOTELEPORT` — Disables Fly Wing, Teleport skill, TK_HIGHJUMP
- `MF_NOWARP` — Cannot warp TO this map
- `MF_NOWARPTO` — Cannot warp FROM this map
- `MF_NORETURN` — Disables Butterfly Wing, return items
- `MF_NOGO` — @go command disabled

**PvP/GvG:**
- `MF_PVP` — PvP mode
- `MF_GVG` — Guild vs Guild mode
- `MF_GVG_CASTLE` — WoE castle map

**EXP/Penalties:**
- `MF_NOPENALTY` — No death penalty
- `MF_NOBASEEXP` / `MF_NOJOBEXP` — No EXP gain
- `MF_NOMOBLOOT` / `MF_NOMVPLOOT` — No drops

**Weather/Visual:**
- `MF_SNOW` / `MF_FOG` / `MF_SAKURA` / `MF_LEAVES` / `MF_CLOUDS` / `MF_FIREWORKS`
- `MF_NIGHTENABLED` — Day/night cycle (303 maps)

**Administrative:**
- `MF_TOWN` — Town map
- `MF_NOSAVE` — No save point (warp to specified map on relog)
- `MF_NOSKILL` — All skills disabled
- `MF_RESTRICTED` — Zone-based item/skill restrictions

### 5.2 Flag Definition Syntax (NPC scripts)
```
prontera    mapflag    town
pay_dun00   mapflag    noteleport
aldeg_cas01 mapflag    gvg_castle
```

---

## 6. Map Naming Conventions

### 6.1 Pattern
`{region_prefix}_{type}{number}` — max 11 characters

### 6.2 Region Prefixes
| Prefix | Region | Examples |
|--------|--------|----------|
| `prt` | Prontera | `prt_fild08`, `prt_sewb1` |
| `gef` | Geffen | `gef_fild07`, `gef_dun02` |
| `pay` | Payon | `pay_fild04`, `pay_dun03` |
| `moc` | Morroc | `moc_fild01`, `moc_pryd06` |
| `alde` | Aldebaran | `alde_dun04` |
| `mjolnir` | Mt. Mjolnir | `mjolnir_06` |
| `cmd` | Comodo | `cmd_fild03` |
| `yuno` | Juno/Yuno | `yuno_fild03` |
| `ein` | Einbroch | `ein_fild05` |
| `lhz` | Lighthalzen | `lhz_fild01` |
| `ra` | Rachel | `ra_fild08` |
| `hu` | Hugel | `hu_fild04` |

### 6.3 Type Suffixes
| Suffix | Type | Example |
|--------|------|---------|
| `_fild` | Field (outdoor) | `prt_fild08` |
| `_dun` | Dungeon | `pay_dun03` |
| `_in` | Indoor | `prt_in` |
| `_castle` | Castle | `prt_castle` |
| `_gld` | Guild | `prt_gld` |
| `g_cas` | WoE castle | `aldeg_cas01` |
| `_sewb` | Sewer/Culvert | `prt_sewb1` |

---

## 7. Zone Connections & Warp System

### 7.1 Warp NPC Definition
Warps are NPC entities defined in server scripts (`npc/warps/`):
```
{src_map},{x},{y},{facing}    warp    {name}    {spanX},{spanY},{dest_map},{dest_x},{dest_y}
```

Example:
```
prontera,156,22,0    warp    prt001    1,1,prt_fild08,170,375
```

### 7.2 Server-Side Processing
1. Parser creates `npc_data` with `subtype = NPCTYPE_WARP`
2. Stores destination: mapindex, x, y
3. Stores trigger area: xs, ys
4. Calls `npc_setcells()` to mark cells with `CELL_NPC`
5. When player steps on warp cell, server sends `PACKET_ZC_NPCACK_MAPMOVE` (0x0091)

### 7.3 Connection Topology
- **Fields form a grid**: adjacent field maps connect at edges
- **Dungeons are linear chains**: floor 1 → floor 2 → floor 3
- **Towns are hubs**: multiple warps to surrounding fields
- **Inter-region connections**: airships (Izlude↔Juno), ships (Alberta↔Amatsu/Louyang)
- **Special portals**: Niflheim (via Umbala), Lutie (via Aldebaran), etc.

### 7.4 Warp Script Organization
```
npc/warps/
  cities/     (26 files: prontera.txt, geffen.txt, etc.)
  dungeons/   (34 files: abbey.txt, gef_dun.txt, etc.)
  fields/     (13 files: gefenia.txt, mtmjolnir.txt, etc.)
  other/      (6 files: airplane.txt, arena.txt, etc.)
```

---

## 8. World Topology & Complete Map List

### 8.1 Rune-Midgarts Kingdom (South)

| City | Position | Notable |
|------|----------|---------|
| **Prontera** | Center | Capital, Knight/Crusader/Acolyte guilds |
| **Izlude** | SE of Prontera | Swordsman guild, port, arena |
| **Geffen** | West | City of Magic, Mage/Wizard guilds |
| **Payon** | Southeast | Archer guild, Korean-style |
| **Morroc** | Southwest | Thief guild, desert, Pyramids |
| **Alberta** | South | Merchant guild, port city |
| **Aldebaran** | North | Alchemist guild, Clock Tower |
| **Comodo** | Far south | Bard/Dancer guilds, underground |
| **Umbala** | Southwest jungle | Tribal village, treetops |

**Fields**: `prt_fild00-11`, `gef_fild00-14`, `pay_fild01-11`, `moc_fild01-22`, `mjolnir_01-12`, `cmd_fild01-09`

### 8.2 Schwarzwald Republic (Northeast)
| City | Notable |
|------|---------|
| **Juno (Yuno)** | Capital, floating city, Sage guild |
| **Einbroch** | Industrial, Gunslinger guild |
| **Einbech** | Mining suburb |
| **Lighthalzen** | Commerce, Biolab dungeon |
| **Hugel** | Coastal village |

**Fields**: `yuno_fild01-12`, `ein_fild01-10`, `lhz_fild01-03`, `hu_fild01-07`

### 8.3 Arunafeltz States (Northwest)
| City | Notable |
|------|---------|
| **Rachel** | Capital, Temple of Freya |
| **Veins** | Canyon town, Thor Volcano |

**Fields**: `ra_fild01-13`, `ve_fild01-07`

### 8.4 Islands
| Location | Theme | Access |
|----------|-------|--------|
| Amatsu | Japanese | Ship from Alberta |
| Louyang | Chinese | Ship from Alberta |
| Kunlun (Gonryun) | Tibetan/floating | Ship from Alberta |
| Ayothaya | Thai | Ship from Alberta |
| Niflheim | Realm of Dead | Portal from Umbala |
| Lutie | Christmas/Snow | Warp from Aldebaran |
| Turtle Island | Dungeon island | Ship from Alberta |
| Moscovia | Russian | Ship from Alberta |

### 8.5 Major Dungeons (34 dungeon groups)
Prontera Culvert (prt_sewb1-4), Geffen Dungeon (gef_dun00-03), Payon Cave (pay_dun00-04), Pyramids (moc_pryd01-06), Sphinx (in_sphinx1-5), Glast Heim (15+ maps), Clock Tower (c_tower1-4, alde_dun01-04), Byalan (iz_dun00-05), Orc Dungeon (orcsdun01-02), Ant Hell (anthell01-02), Turtle Dungeon (tur_dun01-04), Magma Dungeon (mag_dun01-02), Coal Mines, Sunken Ship (treasure01-02), Thanatos Tower (tha_t01-12), Thor Volcano (thor_v01-03), Biolab (lhz_dun01-04), Rachel Sanctuary (ra_san01-05), Abbey (abbey01-03)

### 8.6 Total Map Count
- **~1,247 entries** in rAthena map_index.txt (includes renewal)
- **~600-700 pre-renewal maps** estimated
- Categories: ~198 fields, ~156 dungeons, ~89 town interiors, ~74 guild/castle, ~126 PvP arenas, ~32 job quest maps, ~28 towns

---

## 9. Map Loading & Transitions

### 9.1 Loading Screen
1. "Please Wait" dialog appears
2. Screen goes **black**
3. **Random loading screen illustration** displayed (full-screen)
4. **Progress bar** at bottom: 0% → 100%
5. New map fades in

**Loading screen files:**
- Format: JPEG (`loadingXX.jpg`, zero-based index)
- Path: `data/texture/유저인터페이스/`
- Typically 6-12 images, randomly selected
- Optimal: 4:3 aspect ratio (800x600 or 1024x768)
- No map-specific loading screens — always random
- No gameplay tips displayed

### 9.2 Server Processing (Map Change)
1. Server calls `pc_setpos()` for destination
2. Cancel active combat, casting
3. Remove player from enemy combat sets
4. Broadcast `player:left` to old zone
5. Switch rooms, update zone
6. Save to database
7. Lazy spawn enemies in new zone (if first player)
8. Send map change packet to client
9. Client loads GAT+GND+RSW+RSM from GRF
10. Client sends "map loaded" confirmation
11. Server sends surrounding entity data

---

## 10. Navigation & UI Commands

### 10.1 Pre-Renewal Commands
| Command | Function |
|---------|----------|
| `/where` | Display `mapname (X, Y)` in chat |
| `/memo` | Save current location as Warp Portal destination (3 slots max) |
| `Ctrl+Tab` | Cycle minimap: hidden → transparent → opaque |
| `Ctrl+~` | Open/close world map |

### 10.2 /memo (Warp Portal System)
- **3 memo slots** maximum
- New memo pushes older ones down, oldest overwritten when full
- Same map updates existing entry (no duplicates)
- Blocked by `nomemo` map flag
- Memo points NOT shown on any map display

### 10.3 Fly Wing / Butterfly Wing
| Item | Effect | Blocked By |
|------|--------|------------|
| Fly Wing (601) | Random teleport within current map | `noteleport` flag |
| Butterfly Wing (602) | Teleport to save point | `noreturn` flag |
| Giant Fly Wing (12212) | Random teleport, entire party | `noteleport` flag |

### 10.4 Navigation System (/navi) — NOT PRE-RENEWAL
- Added ~2010-2012, part of Renewal
- `/navi <mapname> <X/Y>` — sets destination
- **Yellow arrows** appear on ground (manual follow, NO auto-walk)
- NPC icons appear on minimap (15+ icon types)
- Can be retrofitted to pre-renewal servers via Hercules plugin

### 10.5 Party Member Tracking
- **Minimap (same map)**: Pink squares for party, pink triangles for guild
- **World map (any map)**: Pink dots for party, pink triangles for guild
- **Party window (Alt+Z)**: Shows each member's current map name as text
- **Cannot see cross-map party on minimap** — requires world map or party window

### 10.6 Coordinates
- No persistent on-screen coordinate display
- `/where` is the only way to check coordinates
- Format: `mapname (X, Y)` — shows internal map ID, not display name

### 10.7 No Pinpoint/Ping System
- No Alt+click marking in Classic
- Party leaders cannot mark locations for members
- Coordination via party chat + `/where`

---

## 11. Map-Related Status Effects

| Skill/Effect | Range | Reveals Hidden? | Minimap Impact |
|-------------|-------|-----------------|----------------|
| Sight (Mage) | 7x7 | Yes (not Chase Walk) | None |
| Ruwach (Acolyte) | 5x5 | Yes + damage | None |
| Intravision (Maya Purple Card) | Visual range | Yes (passive) | None |
| Detect (Hunter) | 5-13 cells | Yes (targeted area) | None |

None of these show anything on the minimap — they reveal game-world sprites only.

---

## 12. Client Configuration Files

| File | Purpose | Entries |
|------|---------|---------|
| `resnametable.txt` | File path aliasing | Varies |
| `mapnametable.txt` | Display names for maps | ~1,242 |
| `mp3nametable.txt` | BGM track per map | ~1,155 |
| `fogparametertable.txt` | Fog params (near, far, color, density) | ~1,520 |
| `indoorrswtable.txt` | Indoor camera mode maps | ~152 |
| `etcinfo.txt` | Weather effects per map | Varies |
| `exceptionminimapnametable.txt` | Minimap exceptions | Varies |

**BGM format**: `{mapname}.rsw#bgm\{trackname}.mp3#`
**Fog format**: `{mapname}.rsw# near# far# color_hex# density#`

---

## 13. Pre-Renewal vs Renewal Feature Matrix

| Feature | Pre-Renewal Classic | Renewal (2012+) |
|---------|-------------------|-----------------|
| Minimap (top-right, 128x128) | Yes | Yes |
| Ctrl+Tab 3-state cycle | Yes | Yes |
| World Map (Ctrl+~) | Yes (Episode 12+, 2007) | Yes |
| Alt toggle field names | Yes | Yes |
| Tab toggle monster map | Yes (2009 patch) | Yes |
| Party dots on world map | Pink dots | Same |
| Party squares on minimap | Pink squares (same map) | Same |
| Guild triangles on minimap | Pink triangles | Same |
| Map name on minimap | Yes | Yes |
| `/where` command | Yes | Yes |
| `/memo` command | Yes (3 slots) | Yes |
| Navigation System (/navi) | **NO** | Yes |
| NPC icons on minimap | **NO** (Guide NPC + marks only) | Yes (15+ icon types) |
| Auto-walk | **NO** | **NO** (arrows only) |
| Minimap click-to-navigate | **NO** | Yes (click NPC icon) |
| Pinpoint/ping system | **NO** | **NO** |
| Fog of war | **NO** | **NO** |
| Coordinate HUD display | **NO** (only /where) | **NO** |
| Loading screens | Random illustration + progress bar | Same |
| Zoom/pan world map | **NO** | **NO** |

---

## Sources (50+ Unique)

### Primary (5/5 credibility)
- rAthena source code (map.hpp, map.cpp, path.cpp, clif.cpp, npc.cpp)
- Ragnarok Research Lab (GAT, GND, RSW, GRF file format specs)
- roBrowser source code (MiniMap.js, MiniMap.css, MiniMap.html)
- rAthena map_index.txt, npc/warps/, npc/mapflag/

### Secondary (4/5 credibility)
- iRO Wiki (Classic + main)
- rAthena Wiki (Custom_Maps, GRF, Resnametable, Mapflag, Weather)
- ROenglishRE client data files (mapnametable, mp3nametable, fogparametertable)
- Ragnarok-Client-Scripts (worldviewdata Lua files, navigation Lua files)
- OpenKore Wiki (Field file format)
- BrowEdit3 (map editor)

### Tertiary (3/5 credibility)
- StrategyWiki, Fandom Wiki, PlayRagnarok official guide
- RateMyServer (interactive world map, forum posts)
- DefKey (keyboard shortcuts), WarpPortal support docs
