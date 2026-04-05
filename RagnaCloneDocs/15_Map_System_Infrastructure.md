# Ragnarok Online Classic Map System Infrastructure

## Executive Summary

This document covers the complete map infrastructure of Ragnarok Online Classic (pre-renewal), including binary file formats (.GAT, .GND, .RSW, .RSM), the map grid/coordinate system, cell types, map loading and transitions, naming conventions, zone connections, and world topology. All information is confirmed against rAthena, Hercules, OpenKore, and Ragnarok Research Lab documentation.

---

## 1. Map File Formats

Every RO map consists of three mandatory files sharing the same base name, plus referenced .RSM model files:

| File | Magic | Purpose | Used By |
|------|-------|---------|---------|
| .GAT | `GRAT` | Ground altitude + cell types (walkability) | Server + Client |
| .GND | `GRGN` | Ground mesh geometry, textures, lightmaps, water | Client only |
| .RSW | `GRSW` | Scene definition: 3D objects, lights, sounds, effects, water | Client only |
| .RSM | `GRSM` | 3D model data (buildings, trees, props) | Client only |

All binary values are **little-endian**. The server only needs .GAT files (for walkability and altitude). The client uses all four types.

Files are stored flat in the `data\` directory of the GRF archive:
```
data\prontera.gat
data\prontera.gnd
data\prontera.rsw
data\model\prontera\building01.rsm   (3D models in subdirectories)
data\texture\...                      (textures in subdirectories)
```

The `resnametable.txt` in the GRF allows aliasing map files without duplicating data:
```
nguild_pay.rsw#payg_cas01.rsw#
nguild_pay.gat#payg_cas01.gat#
nguild_pay.gnd#payg_cas01.gnd#
```

---

## 2. GAT File Format (Ground Altitude/Type)

### 2.1 Binary Structure

**Header (14 bytes):**

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | char[4] | Magic | ASCII `"GRAT"` |
| 4 | 1 | uint8 | MajorVersion | Usually 1 |
| 5 | 1 | uint8 | MinorVersion | 2 or 3 |
| 6 | 4 | int32 | Width | Map width in tiles (cells) |
| 10 | 4 | int32 | Height | Map height in tiles (cells) |

**Tile Data (20 bytes per tile, Width x Height tiles):**

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | float | SouthWestAltitude | Height at (0,0) corner |
| 4 | 4 | float | SouthEastAltitude | Height at (1,0) corner |
| 8 | 4 | float | NorthWestAltitude | Height at (0,1) corner |
| 12 | 4 | float | NorthEastAltitude | Height at (1,1) corner |
| 16 | 4 | uint32 | TerrainType | Cell type flags |

**Total file size:** 14 + (Width x Height x 20) bytes

### 2.2 Cell/Terrain Types

**GAT Type Values (from rAthena/Hercules `map_gat2cell`):**

| GAT Value | Walkable | Shootable | Water | Description |
|-----------|----------|-----------|-------|-------------|
| 0 | YES | YES | NO | Standard walkable ground |
| 1 | NO | NO | NO | Wall / impassable obstacle |
| 2 | YES | YES | NO | (Undocumented, treated as walkable) |
| 3 | YES | YES | YES | Walkable water (shallow) |
| 4 | YES | YES | NO | (Undocumented, treated as walkable) |
| 5 | NO | YES | NO | Cliff / gap (snipeable but impassable) |
| 6 | YES | YES | NO | (Undocumented, treated as walkable) |

**OpenKore FLD Format (expanded cell types with bitflags):**

| Value | Constant | Description |
|-------|----------|-------------|
| 0 | TILE_NOWALK | Non-walkable |
| 1 | TILE_WALK | Walkable ground |
| 2 | TILE_SNIPE | Snipeable (ranged attacks pass through) |
| 4 | TILE_WATER | Water cell |
| 8 | TILE_CLIFF | Cliff/elevation |

**Combined FLD mappings from GAT types:**

| GAT Type | FLD Value | Meaning |
|----------|-----------|---------|
| 0 | TILE_WALK (1) | Walkable ground |
| 1 | TILE_NOWALK (0) | Non-walkable obstacle |
| 2 | TILE_WATER (4) | Non-walkable water, not snipeable |
| 3 | TILE_WALK\|TILE_WATER (5) | Walkable water |
| 4 | TILE_WATER\|TILE_SNIPE (6) | Non-walkable water, snipeable |
| 5 | TILE_CLIFF\|TILE_SNIPE (10) | Cliff, snipeable |
| 6 | TILE_CLIFF (8) | Cliff, not snipeable |

### 2.3 Version 1.3 Water Bit

GAT v1.3 embeds a water flag in the TerrainType field using bitmask `0x80` (128) in the high byte. When the high byte is `0x0080`, the tile is marked as a water tile regardless of the low byte value.

### 2.4 Altitude Interpretation

Heights are stored as **negative values** scaled by the global GAT zoom level. The values represent position on the **negative Y axis** multiplied by the zoom factor. Each tile is **5 world units** in size (hardcoded in the client). Heights must be divided by the scale factor to get actual world-space altitude.

### 2.5 Water Level Determination

The water level is stored in the .RSW file at **byte offset 166** as a 4-byte float. To determine if a cell is underwater:

```
averageDepth = (SWalt + SEalt + NWalt + NEalt) / 4
if averageDepth > waterLevel:
    cell is underwater (add WATER flag)
```

---

## 3. GND File Format (Ground Mesh/Textures)

### 3.1 Binary Structure (Version 1.7, most common)

**Header:**

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| 0 | 4 | char[4] | Magic | ASCII `"GRGN"` |
| 4 | 1 | uint8 | MajorVersion | |
| 5 | 1 | uint8 | MinorVersion | |
| 6 | 4 | int32 | Width | Cube grid width (= GAT width / 2) |
| 10 | 4 | int32 | Height | Cube grid height (= GAT height / 2) |
| 14 | 4 | float | Scale | Geometry scale factor (always 10) |

**Texture Section:**

| Field | Size | Description |
|-------|------|-------------|
| TextureCount | 4 (int32) | Number of diffuse textures |
| TexturePathLength | 4 (int32) | Always 80 |
| TexturePaths | 80 x TextureCount | Null-terminated paths, 80 bytes each |

**Lightmap Section:**

| Field | Size | Description |
|-------|------|-------------|
| SliceCount | 4 (int32) | Number of lightmap slices |
| Per slice: | | |
| - PixelFormat | 4 (int32) | Always 1 (8-bit RGBA as ARGB) |
| - Width | 4 (int32) | Always 8 |
| - Height | 4 (int32) | Always 8 |
| - ShadowmapPixels | 64 (byte[]) | 8x8 ambient occlusion texture |
| - LightmapPixels | 192 (byte[]) | 8x8 x 3-channel color/lighting |

Each slice = 268 bytes total.

**Surface Section:**

| Field | Size | Type | Description |
|-------|------|------|-------------|
| SurfaceCount | 4 | int32 | Number of surface definitions |
| Per surface (56 bytes): | | | |
| - TextureCoords | 32 | float x 8 | U/V for 4 corners (BL, BR, TL, TR) |
| - TextureID | 2 | int16 | Diffuse texture index (-1 = none) |
| - LightmapSlice | 2 | uint16 | Lightmap reference |
| - VertexColor | 16 | BGRA x 4 | Per-corner vertex colors |

**Cube Grid (28 bytes per cube, Width x Height):**

| Field | Size | Type | Description |
|-------|------|------|-------------|
| BottomLeftHeight | 4 | float | SW corner altitude |
| BottomRightHeight | 4 | float | SE corner altitude |
| TopLeftHeight | 4 | float | NW corner altitude |
| TopRightHeight | 4 | float | NE corner altitude |
| TopSurfaceID | 4 | int32 | Upward-facing surface index |
| NorthWallSurfaceID | 4 | int32 | Northern wall surface index |
| EastWallSurfaceID | 4 | int32 | Eastern wall surface index |

### 3.2 GND-GAT Relationship

- GND cubes are **10 world units** (the Scale factor)
- GAT tiles are **5 world units** (hardcoded)
- Each GND cube contains exactly **2x2 = 4 GAT tiles**
- GND dimensions = GAT dimensions / 2
- Normalizing scale factor = (GND dimensions / GAT dimensions) x Scale = 1/2 x 10 = **5**

### 3.3 GND Version Differences

| Version | Changes |
|---------|---------|
| 1.5 | Same as 1.7 except lightmap format stored globally |
| 1.7 | Primary format (described above) |
| 1.8 | Adds single water plane config (level, type, wave params) |
| 1.9 | Adds multiple water planes (U x V grid) |

**GND v1.8 Water Configuration (appended after cube grid):**

| Field | Size | Type |
|-------|------|------|
| WaterLevel | 4 | float |
| WaterType | 4 | int32 (texture ID) |
| WaveHeight | 4 | float |
| WaveSpeed | 4 | float |
| WavePitch | 4 | float |
| TextureCyclingInterval | 4 | int32 |
| NumWaterPlanesU | 4 | int32 (always 1) |
| NumWaterPlanesV | 4 | int32 (always 1) |

---

## 4. RSW File Format (Resource/Scene World)

### 4.1 Binary Structure (Version 2.1, reference)

**Header (6 bytes):**

| Offset | Size | Type | Field |
|--------|------|------|-------|
| 0 | 4 | char[4] | Magic: `"GRSW"` |
| 4 | 1 | uint8 | MajorVersion |
| 5 | 1 | uint8 | MinorVersion |

**Water Configuration (24 bytes, v2.1-2.5):**

Same structure as GND v1.8 single water plane. Removed in v2.6 (moved entirely to GND).

**Lighting Parameters (36 bytes):**

| Field | Size | Type |
|-------|------|------|
| Longitude | 4 | uint32 |
| Latitude | 4 | uint32 |
| DiffuseRed | 4 | float |
| DiffuseGreen | 4 | float |
| DiffuseBlue | 4 | float |
| AmbientRed | 4 | float |
| AmbientGreen | 4 | float |
| AmbientBlue | 4 | float |
| ShadowmapAlpha | 4 | float |

**Map Boundaries (16 bytes):** Four uint32 values (Top, Bottom, Left, Right). Purpose uncertain.

### 4.2 Scene Objects

Prefixed by a 4-byte object count (int32). Each object starts with a 4-byte ObjectTypeID:

**Type 1 -- Animated Props (RSM model instances):**

| Field | Size | Type |
|-------|------|------|
| Name | 40 | string |
| AnimationTypeID | 4 | int32 |
| AnimationSpeed | 4 | float |
| CollisionFlags | 4 | int32 |
| ModelFile | 80 | string (.rsm path) |
| NodeName | 80 | string |
| PositionX/Y/Z | 12 | float x 3 |
| RotationX/Y/Z | 12 | float x 3 |
| ScaleX/Y/Z | 12 | float x 3 |

**Type 2 -- Dynamic Light Sources:**

| Field | Size | Type |
|-------|------|------|
| Name | 80 | string |
| PositionX/Y/Z | 12 | float x 3 |
| DiffuseR/G/B | 12 | float x 3 |
| Range | 4 | float |

**Type 3 -- Spatialized Audio Sources:**

| Field | Size | Type |
|-------|------|------|
| Name | 80 | string |
| SoundFile | 80 | string (.wav path) |
| PositionX/Y/Z | 12 | float x 3 |
| VolumeGain | 4 | float |
| Width | 4 | uint32 |
| Height | 4 | uint32 |
| Range | 4 | float |
| CycleInterval | 4 | float (default 4s in v1.9) |

**Type 4 -- Particle Effect Emitters:**

| Field | Size | Type |
|-------|------|------|
| Name | 80 | string |
| PositionX/Y/Z | 12 | float x 3 |
| PresetEffectID | 4 | uint32 |
| EmissionDelay | 4 | float |
| LaunchParamA/B/C/D | 16 | float x 4 |

### 4.3 Quad Tree (v2.1+)

Fixed 5-level hierarchical structure for frustum culling. Each level has 4 sub-ranges (48 bytes each):

| Field | Size | Type |
|-------|------|------|
| BottomX/Y/Z | 12 | float x 3 |
| TopX/Y/Z | 12 | float x 3 |
| DiameterX/Y/Z | 12 | float x 3 |
| CenterX/Y/Z | 12 | float x 3 |

Total quad tree: ~64KB (fixed size regardless of map).

### 4.4 RSW Version Differences

| Version | Changes |
|---------|---------|
| 1.9 | No quad tree; audio sources lack CycleInterval |
| 2.1 | Standard format (described above) |
| 2.2 | Adds BuildNumber (uint8) after MinorVersion |
| 2.5 | BuildNumber becomes uint32; adds UnknownRenderFlag (uint8) |
| 2.6 | Water config removed (moved to GND) |
| 2.6.162+ | Animated props gain additional uint8 after CollisionFlags |

### 4.5 Water Rendering

- 32 animation frames per water type, cycling at 3 frames/display cycle
- Full cycle: 96 frames / 60fps = ~1.6 seconds
- Wave simulation: sine curve with amplitude, phase offset, surface curvature
- Wind direction: implied (1,1) = northeast
- Transparency: alpha approximately 144/255 (0.565)
- Standard water textures: 128x128 mapped to single GND surface
- Lava types (4 & 6): 256x256 mapped to four surfaces

---

## 5. RSM File Format (3D Models)

### 5.1 Overview

RSM files contain 3D model data for buildings, trees, props, and other decorative objects placed via RSW scene definitions. They are **purely visual** and have no gameplay collision.

**Header:** Magic bytes `"GRSM"`, followed by version (2 bytes).

### 5.2 Core Structure (v1.x)

| Section | Description |
|---------|-------------|
| Header | Magic + version + animation length + shade type + alpha + texture count |
| Textures | Array of texture path strings (40 bytes each) |
| Root Node Name | String identifying the root mesh node |
| Node Count | Number of mesh nodes |
| Nodes | Hierarchical mesh definitions |

**Per-Node Structure:**
- Name (40 bytes string)
- Parent Name (40 bytes string, empty for root)
- Texture references (count + array of texture IDs)
- Offset Matrix (3x3 float matrix = 36 bytes)
- Position (float x 3)
- Rotation Axis (float x 3)
- Rotation Angle (float)
- Scale (float x 3)
- Vertex Count + Vertices (float x 3 each)
- Texture Vertex Count + Texture Vertices (color + UV coords)
- Face Count + Faces (vertex indices, texture vertex indices, texture ID, smoothing group)
- Scale Keyframe Count + Scale Keyframes (frame + float x 3)
- Rotation Keyframe Count + Rotation Keyframes (frame + quaternion)

### 5.3 Version Differences

| Version | Key Changes |
|---------|-------------|
| 1.2 | Color parameters for texture vertices; smoothing groups |
| 1.3 | Bounding box format altered with unknown boolean |
| 1.4 | Transparency support (alpha field, reportedly unused) |
| 1.5 | Scale keyframes per node (unused) |
| 2.2 (.rsm2) | FPS-based animation; multiple root nodes; translation keyframes; length-prefixed strings |
| 2.3 (.rsm2) | Per-mesh texture references; texture animations |

### 5.4 Node Hierarchy

Models use a tree hierarchy where transformations cascade from root to children. Example: a windmill root = building base, child nodes = individual wheels that animate independently. Original RSM supports single root; RSM2 supports multiple independent root nodes.

---

## 6. Map Grid/Coordinate System

### 6.1 Cell Size and Scale

| Measurement | Value |
|-------------|-------|
| **GAT tile/cell size** | 5 world units (hardcoded) |
| **GND cube size** | 10 world units (Scale factor) |
| **GAT tiles per GND cube** | 2x2 = 4 |
| **1 game cell** | 1 GAT tile = 5x5 world units |

### 6.2 Coordinate Origin

- Origin **(0,0)** is at the **bottom-left (southwest)** corner of the map
- **X** increases from west to east (left to right)
- **Y** increases from south to north (bottom to top)
- Tiles are arranged **bottom-left to top-right** in the GAT data array
- Tile at position (x, y) is stored at index: `y * width + x`

### 6.3 Typical Map Dimensions

| Map | Width (cells) | Height (cells) | Total Cells |
|-----|--------------|----------------|-------------|
| Prontera (prontera) | 312 | 392 | 122,304 |
| Typical field map | ~300 | ~300 | ~90,000 |
| Small maps (training) | ~50-100 | ~50-100 | ~2,500-10,000 |
| Large dungeons | Variable | Variable | Up to ~150,000+ |

Maps range from very small (50x50) to large (400x400+). The server enforces a maximum map size constant of **512x512 = 262,144 cells** (`MAX_MAP_SIZE` in rAthena).

### 6.4 Server Block System

The server divides maps into **blocks** of `BLOCK_SIZE = 8` cells for efficient spatial queries. Entity lookup functions (`map_foreachinrange`, `map_foreachinarea`) iterate over blocks rather than individual cells.

### 6.5 Cell Stacking

- Default `official_cell_stack_limit = 1` (one character per cell)
- When a character stops on a cell exceeding the stack limit, they auto-walk to the nearest free cell
- Setting to 0 disables stacking checks (free movement)

---

## 7. Server-Side Cell System (rAthena/Hercules)

### 7.1 mapcell Structure

```c
struct mapcell {
    // Terrain properties (from GAT)
    unsigned char walkable : 1;
    unsigned char shootable : 1;
    unsigned char water : 1;
    // Dynamic flags (set at runtime by skills/NPCs)
    unsigned char npc : 1;          // OnTouch NPC present
    unsigned char basilica : 1;     // Basilica active
    unsigned char landprotector : 1; // Land Protector active
    unsigned char novending : 1;    // Vending prohibited
    unsigned char nochat : 1;       // Chat rooms prohibited
    unsigned char maelstrom : 1;    // Maelstrom active
    unsigned char icewall : 1;      // Ice Wall active
    unsigned char nobuyingstore : 1; // Buying store prohibited
};
```

### 7.2 Cell Check Enum

```c
enum cell_chk : uint8 {
    CELL_GETTYPE,        // Get raw GAT type
    CELL_CHKWALL,        // Is wall? (GAT type 1)
    CELL_CHKWATER,       // Is water? (GAT type 3)
    CELL_CHKCLIFF,       // Is cliff? (GAT type 5)
    CELL_CHKPASS,        // Is passable? (not wall or cliff)
    CELL_CHKREACH,       // Passable ignoring stack limit
    CELL_CHKNOPASS,      // Is impassable?
    CELL_CHKNOREACH,     // Impassable ignoring stack limit
    CELL_CHKSTACK,       // Cell at stack limit?
    CELL_CHKNPC,         // Has OnTouch NPC?
    CELL_CHKBASILICA,    // Has Basilica?
    CELL_CHKLANDPROTECTOR, // Has Land Protector?
    CELL_CHKNOVENDING,   // Vending prohibited?
    CELL_CHKNOCHAT,      // Chat prohibited?
    CELL_CHKMAELSTROM,   // Has Maelstrom?
    CELL_CHKICEWALL,     // Has Ice Wall?
    CELL_CHKNOBUYINGSTORE, // Buying store prohibited?
};
```

### 7.3 Map Cache Format (Server)

The server does NOT read GRF files directly. Instead, maps are pre-processed into a **map cache** binary file:

**Header (6 bytes):**
- FileSize (uint32)
- MapCount (uint16)

**Per-map entry:**
- MapName (12-byte string)
- XDimension (int16)
- YDimension (int16)
- CompressedLength (int32)
- CellData (variable, compressed)

Cell data is stored compressed (zlib). All values are little-endian for cross-platform compatibility.

---

## 8. Map Naming Conventions

### 8.1 Naming Pattern

```
{region_prefix}[_{type}][{number}]
```

### 8.2 Region Prefixes (3-4 characters)

| Prefix | Region | Capital |
|--------|--------|---------|
| `prt` | Prontera (Rune-Midgarts) | Prontera |
| `gef` | Geffen | Geffen |
| `pay` | Payon | Payon |
| `moc` | Morocc / Sograt Desert | Morocc |
| `alde` | Aldebaran | Aldebaran |
| `izlude` | Izlude | Izlude |
| `alberta` | Alberta | Alberta |
| `comodo` | Comodo | Comodo |
| `yuno` | Juno (Schwartzvald) | Juno |
| `lhz` | Lighthalzen | Lighthalzen |
| `ein` | Einbroch / Einbech | Einbroch |
| `hug` | Hugel | Hugel |
| `rachel` | Rachel (Arunafeltz) | Rachel |
| `veins` | Veins | Veins |
| `amatsu` | Amatsu (Japan) | Amatsu |
| `gonryun` | Gonryun (China) | Gonryun |
| `louyang` | Louyang (China) | Louyang |
| `ayothaya` | Ayothaya (Thailand) | Ayothaya |
| `umbala` | Umbala | Umbala |
| `niflheim` | Niflheim | Niflheim |
| `brasilis` | Brasilis (Brazil) | Brasilis |
| `moscovia` | Moscovia (Russia) | Moscovia |
| `malaya` | Malaya (Philippines) | Malaya |
| `mjolnir` | Mt. Mjolnir mountain range | -- |
| `gl` | Glast Heim | -- |

### 8.3 Type Suffixes

| Suffix | Type | Example |
|--------|------|---------|
| (none) | Town/city center | `prontera`, `geffen`, `payon` |
| `_fild` | Field (outdoor) | `prt_fild01`, `gef_fild00` |
| `_dun` | Dungeon | `gef_dun00`, `alde_dun01` |
| `_in` | Indoor / building interior | `prt_in`, `gef_tower` |
| `_castle` / `_cstl` | Castle | `prt_castle` |
| `_church` | Church | `prt_church` |
| `_sewb` | Sewer/culvert (Prontera) | `prt_sewb1` |
| `_pryd` | Pyramid (Morocc) | `moc_pryd01` |
| `_gld` | Guild territory | `prt_gld`, `pay_gld` |
| `g_cas` | Guild castle (WoE) | `prtg_cas01`, `aldeg_cas05` |
| `_vilg` | Village | -- |

### 8.4 Numbering

- Fields and dungeons are numbered sequentially: `00`, `01`, `02`, etc.
- Dungeon numbers typically represent floor/level depth
- Guild castles have 5 per region: `01` through `05`
- PvP arenas: `pvp_n_1-1` through `pvp_n_8-5`
- Instance maps: prefix with `1@`, `2@`: e.g., `1@tower`, `2@orcs`

### 8.5 Special Map Types

| Pattern | Type | Example |
|---------|------|---------|
| `job_*` | Job change quest maps | `job_sword1`, `job_thief1` |
| `pvp_*` | PvP arena maps | `pvp_n_1-1`, `pvp_y_1-1` |
| `guild_vs*` | Guild training arena | `guild_vs1` |
| `quiz_*` | Quiz/event maps | `quiz_00` |
| `monk_in` | Specific building interior | `monk_in` |

### 8.6 Map Name Constraints

- Maximum **11 characters** for the map name
- No spaces allowed (use underscores)
- No `.rsw` extension in server config (deprecated)
- Case-sensitive in most implementations

---

## 9. Map Loading and Transitions

### 9.1 Client Loading Process

When the client receives a map change command:

1. Server sends map change packet with target map name and coordinates
2. Client shows a **loading screen** (the classic blue/gradient screen)
3. Client loads map files from GRF:
   - Reads `.rsw` to get scene definition and file references
   - Reads `.gnd` for terrain geometry, textures, lightmaps
   - Reads `.gat` for altitude data and cell types
   - Loads all referenced `.rsm` models
   - Loads textures, audio, and effects
4. Client builds the 3D scene (ground mesh, objects, lights, water plane)
5. Client places the player at the destination coordinates
6. Loading screen disappears, gameplay resumes

### 9.2 Server Map Change Process

When a player warps (via portal, NPC, skill, or GM command):

1. `pc_setpos()` is called with target map, x, y coordinates
2. Server validates the target map exists and coordinates are valid
3. If same map server: player is removed from old map block lists, added to new map
4. If different map server: player data is saved, connection handed to new map server
5. Server sends `clif_changemap` packet to client
6. Using coordinates (0,0) = random valid placement on the target map

### 9.3 Server-Side Map Registration

Maps must be registered in multiple places:
- `conf/maps_athena.conf` -- list of maps this map server handles
- `db/map_index.txt` -- global map ID registry (name + numeric ID)
- `map_cache.dat` -- compressed cell data for all registered maps

The server auto-generates map cache from .gat files in GRF archives using `mapcache.exe`.

---

## 10. Zone Connections and World Topology

### 10.1 Connection Types

Maps connect to each other through three mechanisms:

**1. Invisible Warp NPCs (Edge Warps)**
The primary connection method. Invisible warp NPCs are placed at map edges, creating seamless field-to-field transitions:

```
// Format: source_map,x,y,facing  warp  name  spanX,spanY,dest_map,dest_x,dest_y
prt_fild00,159,383,0  warp  prtf01  2,6,mjolnir_07,156,19
prt_fild00,18,129,0   warp  prtf03  6,2,gef_fild00,376,140
```

- `spanX, spanY` define the trigger area (cells in each direction from center)
- Source coordinates are placed at map boundaries (near 0, ~16, or max-width)
- Destination coordinates place the player at the opposite edge of the target map

**2. NPC Warps (Interactive)**
NPCs that players talk to for transport:
- Kafra employees (teleport service for zeny)
- Airship NPCs
- Dungeon entrance NPCs
- Quest-specific teleporters

**3. Skill Warps (Warp Portal)**
Player-created portals using the Acolyte skill:
- Consumes Blue Gemstone
- Max 8 people, 3 active portals
- Memorized destinations via `/memo` command (3 slots)
- Portal disappears if caster leaves the map

### 10.2 Warp NPC Script Format

```
source_map,x,y,facing<TAB>warp<TAB>warp_name<TAB>spanX,spanY,dest_map,dest_x,dest_y
```

| Field | Description |
|-------|-------------|
| source_map | Map where warp NPC is placed |
| x, y | NPC coordinates on source map |
| facing | Direction (0 = neutral, unused for warps) |
| warp_name | Unique identifier |
| spanX, spanY | Trigger area radius in cells |
| dest_map | Target map name |
| dest_x, dest_y | Landing coordinates on target map |

### 10.3 Warp File Organization (rAthena)

```
npc/
  warps/
    cities/          # Town entrance/exit warps
      prontera.txt   # Prontera city <-> castle, indoor maps
      geffen.txt
      payon.txt
      ...
    fields/          # Field-to-field edge connections
      prontera_fild.txt
      geffen_fild.txt
      morroc_fild.txt
      mjolnir.txt
      ...
    dungeons/        # Dungeon floor connections
      gef_dun.txt    # Geffen Tower floors
      alde_dun.txt   # Clock Tower floors
      moc_pryd.txt   # Pyramid floors
      ...
    other/           # Special area connections
      ...
  pre-re/
    warps/           # Pre-renewal specific overrides
      cities/
      fields/
      dungeons/
```

### 10.4 World Topology

The Ragnarok Online world consists of:

**Rune-Midgarts Kingdom (Southern Midgard continent)**
- Capital: Prontera
- Towns: Izlude, Geffen, Payon, Morocc, Alberta, Aldebaran, Comodo
- Connected by: Prontera Fields, Geffen Fields, Payon Forest, Sograt Desert, Mt. Mjolnir

**Republic of Schwartzvald (Northeast Midgard)**
- Capital: Juno (Yuno)
- Towns: Aldebaran (gateway), Einbroch, Einbech, Lighthalzen, Hugel
- Connected via: Aldebaran (border city between kingdoms)

**Arunafeltz States (Northwest Midgard)**
- Capital: Rachel
- Towns: Veins
- Connected via: Schwartzvald region

**Islands and Foreign Lands**
- Amatsu (Japanese theme) -- ship from Alberta
- Gonryun (Chinese theme) -- airship/warp
- Louyang (Chinese theme) -- ship from Alberta
- Ayothaya (Thai theme) -- airship
- Umbala (tribal theme) -- southern forests
- Niflheim (realm of the dead) -- portal from Umbala
- Brasilis (Brazilian theme) -- airship
- Moscovia (Russian theme) -- ship

**Field Map Topology**
Fields form a **grid-like network** where each map connects to adjacent maps via edge warps. For example, Prontera's field network:

```
                   mjolnir_07
                      |
                  prt_fild00 --- gef_fild00 (to Geffen)
                      |
      gef_fild01 --- prt_fild04 --- prt_fild05
                      |
                  prt_fild08 --- prt_fild09
                      |
                  (to Morocc fields)
```

### 10.5 Dungeon Structure

Dungeons are **linear chains** of floors connected by warps:

```
Field map entrance
  |
  v
dungeon_01 (Floor 1)
  |
  v
dungeon_02 (Floor 2)
  |
  v
dungeon_03 (Floor 3, boss floor)
```

Example (Geffen Tower): `gef_dun00` -> `gef_dun01` -> `gef_dun02` -> `gef_dun03`
Example (Pyramid): `moc_pryd01` -> `moc_pryd02` -> `moc_pryd03` -> `moc_pryd04` -> `moc_pryd05` -> `moc_pryd06`

### 10.6 Indoor Maps

Town buildings lead to **shared indoor maps** that combine multiple building interiors:
- `prt_in` -- all Prontera building interiors in one map
- `gef_tower` -- Geffen Tower interior
- `prt_church` -- Prontera church interior

Players warp to specific coordinates within the indoor map to appear in the correct building.

---

## 11. Map Index System

The `map_index.txt` file assigns a unique numeric ID to every map for inter-server communication:

```
prontera    1
prt_fild01  2
prt_fild02  3
...
// Custom maps start at 1250+
custom_map  1250
```

- IDs must **never change** once assigned (used for cross-server references)
- New maps are appended at the end
- ~1,300+ maps in the complete rAthena index
- Both pre-renewal and renewal maps share the same index

---

## 12. Key Technical Constants

| Constant | Value | Description |
|----------|-------|-------------|
| GAT tile size | 5 world units | Hardcoded in client |
| GND cube size | 10 world units | Scale factor in GND header |
| GAT tiles per GND cube | 4 (2x2) | Fixed ratio |
| BLOCK_SIZE | 8 cells | Server spatial partitioning |
| MAX_MAP_SIZE | 512 x 512 | Maximum map dimensions |
| Map name max length | 11 characters | Server limitation |
| Cell stack limit | 1 (official) | Characters per cell |
| Water alpha | ~0.565 (144/255) | Water transparency |
| Water texture frames | 32 | Animation frames per type |
| Water cycle speed | ~1.6 seconds | Full texture cycle |
| Quad tree levels | 5 | RSW spatial hierarchy |
| GAT magic bytes | `GRAT` | File identification |
| GND magic bytes | `GRGN` | File identification |
| RSW magic bytes | `GRSW` | File identification |
| RSM magic bytes | `GRSM` | File identification |

---

## Sources

1. [Ragnarok Research Lab - GAT](https://ragnarokresearchlab.github.io/file-formats/gat/) -- Binary format specification (5/5 credibility)
2. [Ragnarok Research Lab - GND](https://ragnarokresearchlab.github.io/file-formats/gnd/) -- Binary format specification (5/5 credibility)
3. [Ragnarok Research Lab - RSW](https://ragnarokresearchlab.github.io/file-formats/rsw/) -- Binary format specification (5/5 credibility)
4. [RagnarokFileFormats/GAT.MD](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GAT.MD) -- Community format research (4/5 credibility)
5. [RagnarokFileFormats/GND.MD](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GND.MD) -- Community format research (4/5 credibility)
6. [RagnarokFileFormats/RSW.MD](https://github.com/rdw-archive/RagnarokFileFormats/blob/master/RSW.MD) -- Community format research (4/5 credibility)
7. [RagnarokFileFormats/RSM.MD](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/RSM.MD) -- Community format research (4/5 credibility)
8. [rAthena src/map/map.hpp](https://github.com/rathena/rathena/blob/master/src/map/map.hpp) -- Cell enums/structs (5/5 credibility)
9. [rAthena src/map/map.cpp](https://github.com/rathena/rathena/blob/master/src/map/map.cpp) -- Map loading implementation (5/5 credibility)
10. [rAthena Wiki - Map_Gat2Cell](https://github.com/rathena/rathena/wiki/Map_Gat2Cell) -- GAT type conversion (5/5 credibility)
11. [rAthena Wiki - Custom_Maps](https://github.com/rathena/rathena/wiki/Custom_Maps) -- Map configuration (5/5 credibility)
12. [rAthena Wiki - Resnametable](https://github.com/rathena/rathena/wiki/Resnametable) -- File aliasing (5/5 credibility)
13. [rAthena Wiki - Defining_Warp_Points](https://github.com/rathena/rathena/wiki/Defining_Warp_Points) -- Warp NPC format (5/5 credibility)
14. [rAthena db/map_index.txt](https://github.com/rathena/rathena/blob/master/db/map_index.txt) -- Complete map registry (5/5 credibility)
15. [rAthena doc/map_cache.txt](https://github.com/rathena/rathena/blob/master/doc/map_cache.txt) -- Map cache format (5/5 credibility)
16. [rAthena npc/pre-re/warps](https://github.com/rathena/rathena/tree/master/npc/pre-re/warps) -- Pre-renewal warp scripts (5/5 credibility)
17. [Hercules src/map/map.c](https://github.com/HerculesWS/Hercules/blob/stable/src/map/map.c) -- Alternative implementation (5/5 credibility)
18. [OpenKore Wiki - Field file format](https://openkore.com/wiki/Field_file_format) -- FLD format spec (4/5 credibility)
19. [OpenKore gat_to_fld2.pl](https://github.com/OpenKore/openkore/blob/master/fields/tools/gat_to_fld2.pl) -- GAT conversion source (4/5 credibility)
20. [iRO Wiki Classic - World Map](https://irowiki.org/classic/World_Map) -- Map topology reference (3/5 credibility)
21. [RateMyServer - Map Database](https://ratemyserver.net/index.php?page=map_db) -- Map dimensions reference (3/5 credibility)
22. [WarpPortal Forums - Map Sizes](https://forums.warpportal.com/index.php?/topic/195189-resolved-map-sizes/) -- Map dimensions (2/5 credibility)
