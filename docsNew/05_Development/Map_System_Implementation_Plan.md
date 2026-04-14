# Map System Implementation Plan — Minimap, World Map & Navigation

> Full implementation plan for RO Classic-style map UI in Sabri_MMO.
> Research source: `RagnaCloneDocs/16_Map_Minimap_WorldMap_Complete_Research.md`
> Existing zone system: 4 zones (prontera, prontera_south, prontera_north, prt_dungeon_01) with full warp/Kafra/transition support.

---

## Table of Contents
1. [Architecture Overview](#1-architecture-overview)
2. [Phase 1: Map Data Infrastructure](#phase-1-map-data-infrastructure)
3. [Phase 2: Minimap Subsystem & Widget](#phase-2-minimap-subsystem--widget)
4. [Phase 3: World Map Subsystem & Widget](#phase-3-world-map-subsystem--widget)
5. [Phase 4: Map Indicators & Markers](#phase-4-map-indicators--markers)
6. [Phase 5: Loading Screen System](#phase-5-loading-screen-system)
7. [Phase 6: Guide NPC Mark System](#phase-6-guide-npc-mark-system)
8. [Phase 7: /where Command & Coordinate Display](#phase-7-where-command--coordinate-display)
9. [Phase 8: Monster Map Overlay](#phase-8-monster-map-overlay)
10. [Phase 9: Polish & Integration](#phase-9-polish--integration)
11. [File Inventory](#file-inventory)
12. [Test Checklist](#test-checklist)

---

## 1. Architecture Overview

### System Components

```
┌─────────────────────────────────────────────────────┐
│                    Server (Node.js)                   │
│  ro_zone_data.js ← zone registry, warps, spawns      │
│  ro_world_map_data.js ← NEW: zone layout, connections │
│  index.js ← map:info, map:markers socket events       │
└──────────────────────┬──────────────────────────────┘
                       │ Socket.io
┌──────────────────────▼──────────────────────────────┐
│              Client (UE5 C++ Slate)                   │
│                                                       │
│  MapDataSubsystem (UWorldSubsystem)                   │
│  ├── Zone registry (name, display name, connections)  │
│  ├── Minimap texture cache (UTexture2D per zone)      │
│  └── World map layout data                            │
│                                                       │
│  MinimapSubsystem (UWorldSubsystem)                   │
│  ├── SMinimapWidget (128x128 Slate, top-right)        │
│  ├── Player arrow (rotates with facing)               │
│  ├── Party/guild member dots                          │
│  ├── Warp portal markers (red dots)                   │
│  ├── Guide NPC marks (blinking crosses)               │
│  ├── 3 opacity states (Ctrl+Tab)                      │
│  └── 5 zoom levels                                    │
│                                                       │
│  WorldMapSubsystem (UWorldSubsystem)                  │
│  ├── SWorldMapWidget (fullscreen overlay)              │
│  ├── Hand-painted continent background                │
│  ├── Hoverable zone rectangles with tooltips          │
│  ├── Party/guild member positions (cross-zone)        │
│  ├── Current location indicator                       │
│  ├── Zone name toggle (Alt key)                       │
│  └── Monster info toggle (Tab key)                    │
│                                                       │
│  MapLoadingSubsystem (UWorldSubsystem)                │
│  ├── Full-screen loading overlay                      │
│  ├── Random loading screen art                        │
│  └── Progress bar                                     │
└───────────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **Minimap BMP equivalent → UTexture2D**: Pre-render overhead minimap images for each zone as PNG/BMP textures. Import into UE5 as `UTexture2D`. Load on zone entry.

2. **World map background → Single large UTexture2D**: Hand-painted continent illustration. Zone bounding boxes defined in data.

3. **All UI in pure C++ Slate** — consistent with existing subsystem pattern.

4. **Server sends zone layout data** via `zone:ready` — zone connections, display names, party member positions. The client builds the world map from this data.

5. **Minimap rendering via Slate `FSlateDrawElement`** — draw minimap texture, then overlay indicators (arrow, dots, marks) using Slate paint calls. No UMG/UCanvas.

---

## Phase 1: Map Data Infrastructure
> Server-side zone layout data + client data subsystem

### Tasks

- [x] **1.1** Create `server/src/ro_world_map_data.js` — zone layout registry
  - Zone display names, recommended levels, map category (town/field/dungeon/indoor)
  - Zone connections graph (which zones connect to which, via which warps)
  - World map bounding boxes (x1, y1, x2, y2 pixel coords on world map image)
  - Monster info per zone (name, level range — pulled from existing spawn data)
  - Zone flags summary (town, pvp, dungeon, etc.)

- [x] **1.2** Add `map:world_data` socket event to `server/src/index.js`
  - Sent once on `zone:ready` (alongside buff:list, etc.)
  - Payload: full zone registry (all zones, not just current), party member zones
  - Lightweight — only names, bounds, connections, flags. No cell data.

- [x] **1.3** Add `map:party_positions` socket event (6 zone-change paths)
  - Sent whenever a party member changes zones
  - Payload: `{ memberId, memberName, zoneName, zoneDisplayName }`
  - Allows world map to show party dots on correct zones

- [x] **1.4** Create `client/SabriMMO/Source/SabriMMO/UI/MinimapSubsystem.h/.cpp` (combined MapData + Minimap)
  - `UWorldSubsystem` — zone data cache
  - Struct `FZoneMapData`: zoneName, displayName, levelRange, category, bIsTown, bIsDungeon, connections (TArray<FString>), worldMapBounds (FBox2D), monsters (TArray<FMonsterInfo>)
  - `TMap<FString, FZoneMapData> ZoneRegistry`
  - Registers for `map:world_data` via EventRouter
  - Provides `GetZoneData(zoneName)`, `GetAllZones()`, `GetCurrentZone()`
  - Caches party member zone positions for world map

- [x] **1.5** Create minimap texture assets (N/A — replaced by SceneCapture2D)
  - For each existing zone (prontera, prontera_south, prontera_north, prt_dungeon_01):
    - Create a 512x512 PNG overhead view of the zone
    - Walkable areas = colored terrain, non-walkable = transparent
    - Style: simplified, colorized top-down view (matches RO Classic minimaps)
  - Import as `UTexture2D` in `Content/SabriMMO/Textures/Minimaps/`
  - Naming: `T_Minimap_{zoneName}.png` (e.g., `T_Minimap_prontera.png`)

- [x] **1.6** Create world map background texture (T_WorldMap_SabriMMO.png)
  - Hand-painted fantasy continent illustration (matches RO Classic style)
  - Resolution: 2048x2048 or 4096x2048 (depends on aspect ratio)
  - Shows: terrain types (green fields, brown desert, white snow, mountains, ocean)
  - Import as `UTexture2D` at `Content/SabriMMO/Textures/UI/T_WorldMap_Midgard.png`

- [x] **1.7** Define zone bounding boxes for world map (in ro_world_map_data.js)
  - In `ro_world_map_data.js`, define pixel coordinates of each zone's rectangle on the world map image
  - Match RO's `worldviewdata_table.lua` format: `{ zoneName, x1, y1, x2, y2, displayName, levelRange }`

---

## Phase 2: Minimap Subsystem & Widget
> Core minimap rendering, opacity cycling, zoom

### Tasks

- [x] **2.1** Create `client/SabriMMO/Source/SabriMMO/UI/MinimapSubsystem.h/.cpp`
  - `UWorldSubsystem` — manages minimap state and input
  - Properties:
    - `CurrentMinimapTexture` (UTexture2D*, UPROPERTY)
    - `OpacityState` (0=hidden, 1=50%, 2=100%)
    - `ZoomLevel` (0-4, factors: [1, 10, 6, 3, 2])
    - `bIsWorldMapOpen` (prevents minimap input while world map open)
  - On `OnWorldBeginPlay`: load minimap texture for current zone, create widget
  - On zone change: swap minimap texture
  - Register for `map:world_data`, `map:party_positions` via EventRouter
  - Gate widget behind `GI->IsSocketConnected()` (don't show on login screen)

- [x] **2.2** Create `client/SabriMMO/Source/SabriMMO/UI/SMinimapWidget.h/.cpp`
  - `SCompoundWidget` — 128x128 fixed-size Slate widget
  - **Position**: Top-right corner of viewport (`FAnchors(1.0, 0.0)`, offset `-146, 18`)
  - **Rendering** (in `OnPaint`):
    1. Draw minimap texture (scaled/cropped based on zoom level, centered on player)
    2. Draw player arrow (white, rotated by facing direction — 8 directions, 45° each)
    3. Draw party member squares (6x6 white border, 4x4 colored fill)
    4. Draw guild member triangles (pink `rgb(245,175,200)`, white stroke)
    5. Draw warp portal dots (red, small circles)
    6. Draw Guide NPC marks (colored + crosses, blinking 500ms)
  - Apply `globalAlpha` based on opacity state (0.0, 0.5, 1.0)
  - **Map name text**: Above minimap canvas, white text, zone display name
  - **Zoom buttons**: Two small buttons at top-right of minimap (+ and -)

- [x] **2.3** Implement Tab opacity cycling (mapped to Tab key)
  - In `PlayerInputSubsystem` (existing), add Ctrl+Tab binding
  - Cycle: hidden(0) → transparent(1) → opaque(2) → hidden(0)
  - Call `MinimapSubsystem->CycleOpacity()`
  - Save preference to GameInstance for persistence

- [x] **2.4** Implement zoom in/out
  - Zoom button click handlers in SMinimapWidget
  - 5 levels: `[1, 10, 6, 3, 2]` (full map, then progressively closer)
  - At level 0: entire minimap texture scaled to 128x128
  - At levels 1-4: crop texture around player position, scale factor applied

- [x] **2.5** Implement minimap coordinate projection
  - Convert world position (UE5 coordinates) to minimap pixel coordinates
  - Account for zone bounds, minimap texture size, current zoom level
  - Function: `FVector2D WorldToMinimapPos(FVector WorldPos)`
  - Must handle UE5 → RO coordinate differences (Z=up vs Y=up)

- [x] **2.6** Implement player arrow rotation
  - Get player facing direction from character rotation
  - Quantize to 8 directions (N, NE, E, SE, S, SW, W, NW)
  - Draw white arrow rotated by `direction * 45°`
  - Arrow texture: small white directional indicator (create as asset or draw via Slate)

- [x] **2.7** Test minimap rendering
  - Verify correct position (top-right), correct size (128x128)
  - Verify player arrow tracks character position and rotation
  - Verify opacity cycling works (3 states)
  - Verify zoom levels work (5 levels)
  - Verify minimap hidden on login screen

---

## Phase 3: World Map Subsystem & Widget
> Full-screen world map overlay with hoverable zones

### Tasks

- [x] **3.1** Create world map subsystem (merged into MinimapSubsystem)
  - `UWorldSubsystem` — manages world map state
  - Properties:
    - `bIsOpen` (toggle state)
    - `bShowZoneNames` (Alt toggle)
    - `bShowMonsterInfo` (Tab toggle)
    - `HoveredZone` (FString — currently hovered zone name)
  - Toggle with Ctrl+` (backtick)
  - When open: pause game input (but not game simulation), show overlay
  - Get zone data from MapDataSubsystem

- [x] **3.2** Create `client/SabriMMO/Source/SabriMMO/UI/SWorldMapWidget.h/.cpp`
  - `SCompoundWidget` — full-screen overlay
  - **Rendering layers** (in OnPaint):
    1. **Background**: Draw world map texture (T_WorldMap_Midgard) scaled to screen
    2. **Zone rectangles**: For each zone in registry, draw invisible hover region at bounding box coords
    3. **Hovered zone highlight**: If mouse over a zone, draw semi-transparent highlight rectangle + tooltip
    4. **Zone names**: If `bShowZoneNames`, draw display name text on each zone (white text, small font)
    5. **Monster info**: If `bShowMonsterInfo`, overlay monster names/levels per zone (green text)
    6. **Current location**: White arrow on the zone the player is currently in
    7. **Party member dots**: Pink dots on zones where party members are (cross-zone!)
    8. **Guild member triangles**: Pink triangles on zones where guild members are
    9. **Dungeon markers**: Red dots at dungeon entrance locations
  - **Tooltip on hover**: Show zone display name, level range, minimap thumbnail preview
  - RO Classic brown/gold UI theming for borders and text backgrounds

- [x] **3.3** Implement Tilde (backtick) toggle
  - In `PlayerInputSubsystem`, add Ctrl+Backtick binding
  - Toggle `WorldMapSubsystem->ToggleWorldMap()`
  - When open: consume all game input except Esc and toggle shortcuts
  - Esc also closes world map

- [x] **3.4** Implement Alt key zone name toggle
  - While world map is open, Alt key toggles `bShowZoneNames`
  - Zone names drawn as white text labels centered on each zone rectangle

- [x] **3.5** Implement Tab key monster info toggle
  - While world map is open, Tab key toggles `bShowMonsterInfo`
  - Pull monster data from MapDataSubsystem (populated from server zone data)
  - Draw monster names (green) and level ranges (white) on each zone

- [x] **3.6** Implement zone hover interaction
  - Mouse tracking over zone bounding boxes
  - On hover: highlight zone rectangle with semi-transparent overlay (dark red border `rgb(149,15,15)`)
  - Show tooltip: zone name, level range, minimap thumbnail
  - Minimap thumbnail: small version of the zone's minimap texture

- [x] **3.7** Implement party member cross-zone tracking
  - MapDataSubsystem maintains `TMap<int32, FString> PartyMemberZones`
  - Updated via `map:party_positions` socket event
  - WorldMapWidget reads this to draw pink dots on correct zone rectangles
  - Hover over dot → show party member name

- [x] **3.8** Test world map
  - Verify Ctrl+` opens/closes
  - Verify zone hover highlights and tooltips
  - Verify Alt toggles zone names
  - Verify Tab toggles monster info
  - Verify party member dots appear on correct zones
  - Verify current location arrow is on correct zone
  - Verify Esc closes world map
  - Verify game input paused while world map open

---

## Phase 4: Map Indicators & Markers
> Party/guild dots, warp markers, position tracking

### Tasks

- [x] **4.1** Implement party member minimap indicators
  - In MinimapSubsystem: listen for party member position updates (from PartySubsystem)
  - For each party member on same zone: draw colored square on minimap
  - Colors: assign random RGB per member (consistent across session)
  - Render: 6x6px white border, 4x4px colored fill

- [ ] **4.2** Implement guild member minimap indicators
  - Listen for guild member data (when guild system exists)
  - Draw upward-pointing pink triangles (`rgb(245,175,200)`) with white stroke
  - Same-zone only

- [x] **4.3** Implement warp portal minimap markers
  - From zone data (warps array in ro_zone_data.js), get warp positions
  - Server sends warp positions as part of `map:world_data`
  - Draw small red dots at warp locations on minimap
  - These are static per-zone — load once on zone entry

- [x] **4.4** Implement player position to minimap coordinate mapping
  - Zone bounds (server-defined min/max coords or UE5 level bounds)
  - Map player world position → normalized 0..1 → minimap pixel
  - Handle zoom: at higher zoom, only show portion of map around player
  - Update every frame in `Tick()`

- [x] **4.5** Server: include warp positions in zone data
  - In `zone:ready` handler, include warp portal positions for the current zone
  - Format: `warps: [{ id, x, y, z, destZone }]`
  - Client uses these for red dot placement on minimap

---

## Phase 5: Loading Screen System
> RO-style loading screens during zone transitions

### Tasks

- [x] **5.1** Upgrade `ZoneTransitionSubsystem` loading overlay
  - Currently: plain fullscreen opaque overlay
  - Upgrade to: loading screen illustration + progress bar
  - On zone change: randomly select one loading screen image
  - Display full-screen, maintain until level fully loaded

- [x] **5.2** Create loading screen art assets
  - Create 6-12 loading screen illustrations
  - Resolution: 1920x1080 (16:9) or auto-scale
  - Style: Anime/fantasy character art (consistent with RO aesthetic)
  - Import as `UTexture2D` at `Content/SabriMMO/Textures/LoadingScreens/`
  - Naming: `T_Loading_00.png` through `T_Loading_11.png`

- [x] **5.3** Implement progress bar
  - Horizontal bar at bottom of loading screen
  - RO Classic style: simple bar, 0% → 100%
  - Track level streaming progress via `GetAsyncLoadPercentage` or timer-based estimate
  - Optional: show "Loading..." or zone display name text

- [x] **5.4** Random loading screen selection
  - On zone transition start: randomly pick from available loading textures
  - Ensure different image from last transition (if possible)

---

## Phase 6: Guide NPC Mark System
> Blinking cross marks on minimap from Guide NPCs

### Tasks

- [x] **6.1** Add `map:mark` socket event to server
  - Sent by Guide NPC interaction scripts
  - Payload: `{ type, x, y, id, color, duration }`
  - Type 0: timed mark, Type 1: persistent until death/teleport, Type 2: remove by ID

- [x] **6.2** Implement mark rendering in SMinimapWidget (blinking crosses)
  - Store marks as `TArray<FMinimapMark>` in MinimapSubsystem
  - Struct: `{ int32 Id; float X, Y; FColor Color; float RemainingTime; bool bPersistent; }`
  - Render as cross/plus (+): two rectangles (2x8 and 8x2 pixels)
  - **Blinking**: toggle visibility every 500ms (via accumulated delta time)

- [x] **6.3** Implement mark lifetime management
  - Timed marks: decrement timer each tick, remove when expired
  - Persistent marks: remove on death or zone transition
  - Remove-by-ID: find and remove specific mark

- [x] **6.4** Create Guide NPC logic (server-side)
  - Guide NPC in Prontera: offers list of facilities
  - On selection: emit `map:mark` events for selected facility locations
  - "Clear all marks" option

---

## Phase 7: /where Command & Coordinate Display
> Chat command to show current map coordinates

### Tasks

- [x] **7.1** Implement `/where` chat command (client-side)
  - In ChatSubsystem: detect `/where` input
  - Get current zone name from ZoneTransitionSubsystem
  - Get player position in cell coordinates (convert UE5 → game coords)
  - Display in chat: `zoneName (X, Y)` format
  - System message (not broadcast to other players)

- [x] **7.2** Coordinate conversion helper (WorldToMinimapUV + /where cell conversion)
  - UE5 world position → cell coordinates
  - Using the zone's coordinate system (origin, scale)
  - Function in MapDataSubsystem: `FIntPoint WorldToCell(FVector WorldPos)`
  - Consider: 50 UE units ≈ 1 RO cell (existing convention from CLAUDE.md)

---

## Phase 8: Monster Map Overlay
> Monster info shown on world map when Tab is pressed

### Tasks

- [x] **8.1** Populate monster data per zone in `ro_world_map_data.js`
  - For each zone, list: monster names, level ranges, element types
  - Source: existing spawn data in `ro_zone_data.js` + `ro_monster_templates.js`
  - Auto-generate from spawn definitions (no manual entry)

- [x] **8.2** Send monster data to client (included in map:world_data)
  - Include in `map:world_data` payload
  - Per zone: `monsters: [{ name, level, element }]`

- [x] **8.3** Render monster overlay on world map
  - When Tab toggled: draw monster info text on each zone
  - Monster names: green text (`rgb(128, 255, 128)`)
  - Monster levels: white text
  - Position: centered within zone bounding box
  - Truncate if too many monsters (show top 3-5 by level)

---

## Phase 9: Polish & Integration
> Cross-system integration, edge cases, UX polish

### Tasks

- [x] **9.1** Minimap auto-hides on login screen
  - Gate behind `GI->IsSocketConnected()` (matches existing pattern)
  - Create widget only after socket connected

- [x] **9.2** Minimap texture swap on zone transition (N/A -- SceneCapture2D auto-updates)
  - SceneCapture2D renders the live 3D world, so the minimap automatically reflects the new zone after transition
  - No texture swapping needed

- [x] **9.3** World map closes on zone transition
  - If world map is open when zone change begins, close it automatically
  - Subsystem listens for zone:change and calls CloseWorldMap()

- [x] **9.4** Persist minimap preferences
  - Save opacity state and zoom level to GameInstance
  - Restore on world begin play

- [x] **9.5** RO Classic UI theming
  - Minimap frame: subtle dark border consistent with RO brown/gold theme
  - World map: dark parchment-style border, RO Classic font for zone names
  - Loading screen: no special frame, full-screen bleed
  - Use `FLinearColor` constants from existing UI theme

- [x] **9.6** Performance optimization
  - Minimap: only update indicators at 10Hz (not every frame)
  - World map: only render when open (no background updates)
  - Minimap texture: GPU-resident, no CPU reads
  - Zone data: cached on first load, not re-requested

- [x] **9.7** Input handling priority
  - World map open: consume all gameplay input (movement, attack, skills)
  - Minimap: never consumes input (display only in Classic)
  - Tab/M/N work in all states; SWorldMapWidget::OnKeyDown handles Tab/N/M/Esc directly

- [x] **9.8** Documentation updates
  - Update `docsNew/02_Client_Side/C++_Code/00_Module_Overview.md` with new subsystems
  - Update `docsNew/00_Project_Overview.md` with map UI feature
  - Update `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` with minimap texture creation
  - Update `docsNew/06_Reference/Event_Reference.md` with new socket events

---

## File Inventory

### New C++ Files (Client)
| File | Type | Purpose |
|------|------|---------|
| `UI/MapDataSubsystem.h/.cpp` | UWorldSubsystem | Zone registry, map data cache |
| `UI/MinimapSubsystem.h/.cpp` | UWorldSubsystem | Minimap state, input, preferences |
| `UI/SMinimapWidget.h/.cpp` | SCompoundWidget | 128x128 minimap rendering |
| `UI/WorldMapSubsystem.h/.cpp` | UWorldSubsystem | World map state, toggles |
| `UI/SWorldMapWidget.h/.cpp` | SCompoundWidget | Full-screen world map rendering |

### New Server Files
| File | Purpose |
|------|---------|
| `server/src/ro_world_map_data.js` | Zone layout registry, world map bounds, connections |

### Modified Files
| File | Changes |
|------|---------|
| `server/src/index.js` | Add `map:world_data`, `map:party_positions`, `map:mark` events |
| `client/.../UI/PlayerInputSubsystem.h/.cpp` | Add Ctrl+Tab, Ctrl+` bindings |
| `client/.../UI/ChatSubsystem.h/.cpp` | Add `/where` command handling |
| `client/.../UI/ZoneTransitionSubsystem.h/.cpp` | Upgrade loading overlay |
| `client/.../SabriMMO.Build.cs` | (No changes expected — all Slate, no new modules) |

### New Art Assets
| Asset | Location | Format | Notes |
|-------|----------|--------|-------|
| Minimap textures (4) | `Content/SabriMMO/Textures/Minimaps/` | PNG 512x512 | One per zone |
| World map background | `Content/SabriMMO/Textures/UI/` | PNG 2048x2048+ | Continent illustration |
| Loading screens (6-12) | `Content/SabriMMO/Textures/LoadingScreens/` | PNG 1920x1080 | Fantasy character art |
| Player arrow | `Content/SabriMMO/Textures/UI/` | PNG 16x16 | White directional arrow |
| Zoom buttons (4) | `Content/SabriMMO/Textures/UI/` | PNG 12x12 | +/- normal and hover |

---

## Test Checklist

### Minimap Tests
- [x] Minimap appears at top-right corner after logging in
- [x] Minimap NOT visible on login/character select screen
- [x] Player arrow tracks character position correctly
- [x] Player arrow rotates with character facing direction
- [x] Tab cycles: opaque → transparent → hidden → opaque
- [x] Zoom +/- buttons work (5 levels)
  - [x] Level 0: entire map visible
  - [x] Level 4: close-up around player
- [x] Zone display name shown inside minimap (semi-transparent strip)
- [x] Warp portals shown as red dots at correct positions
- [x] Party members shown as colored squares (same zone only)
- [x] Party member NOT shown when on different zone
- [x] Minimap auto-updates on zone transition (SceneCapture2D)
- [x] Minimap works in all 4 zones
- [x] Minimap draggable via left-click drag (DPI-correct)
- [x] Minimap preferences persist across zone transitions (GameInstance)
- [x] Compact 134x134 layout with double gold/dark frame

### World Map Tests
- [x] M opens world map as full-screen overlay
- [x] M again or Esc closes world map
- [x] Zone hover highlights and shows tooltip
- [x] Tooltip shows: zone name, level range, monster list
- [x] N toggles zone name labels on/off
- [x] Tab toggles monster info overlay on/off
- [x] Current location indicator on correct zone
- [x] Party member pink dots on correct zones (cross-zone)
- [x] Game input paused while world map open
- [x] World map auto-closes on zone transition
- [x] World map works correctly for all 4 zones

### Loading Screen Tests
- [x] Loading screen shows during zone transition
- [x] Random illustration displayed (not same every time)
- [x] Progress bar visible and animates 0% → 95%
- [x] Loading screen fills entire viewport with Ken Burns effect
- [x] Loading clears after level fully loaded
- [x] 30 golden sparkle particles with twinkle animation
- [x] Fade-in from black over 0.8 seconds
- [x] Bottom strip with zone name text

### Guide NPC Tests
- [x] Guide NPC in Prontera marks minimap on interaction
- [x] Marks appear as colored + crosses
- [x] Marks blink (500ms on/off cycle)
- [x] Timed marks disappear after duration
- [x] "Clear marks" option removes all marks
- [x] Marks removed on zone transition

### /where Command Tests
- [x] `/where` typed in chat shows coordinates
- [x] Format: `zoneName (X, Y)` in system message
- [x] Coordinates are reasonable/accurate
- [x] Works in all zones

### Integration Tests
- [x] Minimap and world map don't conflict
- [x] Tab works while world map is open (toggles monster info)
- [x] Zone transition: minimap updates, world map closes, loading screen shows
- [x] Party dots update in real-time on both minimap and world map
- [x] No FPS impact from minimap (16 FPS SceneCapture)
- [x] All new subsystems clean up properly on Deinitialize
- [x] All new subsystems use EventRouter pattern (RegisterHandler/UnregisterAllForOwner)

---

## Known Gotchas

### Minimap SceneCapture must disable post-process materials

`MinimapSubsystem::SetupOverheadCapture()` sets `CaptureComponent->ShowFlags.SetPostProcessMaterial(false)`. This is **required**, not optional.

`PostProcessSubsystem` pushes a cutout post-process material into the unbound global `APostProcessVolume`. That material darkens every pixel where `CustomStencil != 1` (see `docsNew/05_Development/RO_Classic_Visual_Style_Research.md` and `memory/sprite-rendering-2026-04-06.md`). The minimap's SceneCapture uses `SCS_FinalColorLDR`, which runs the full post-process chain — including the cutout.

The minimap camera looks straight down, and the player sprite is a billboard facing the main camera, so from the overhead view the sprite is edge-on and writes no stencil into the capture. Result: the cutout darkens the entire captured frame to ~61% brightness, which visually reads as "the minimap texture isn't showing" (the dim capture blends into the dark frame background).

**If you add another SceneCapture2D anywhere in the project**, repeat the same `SetPostProcessMaterial(false)` call, or switch the capture source to `SCS_SceneColorHDR`. See `memory/feedback-scenecapture-postprocess.md`.

---

## Implementation Order (Recommended)

| Order | Phase | Priority | Depends On |
|-------|-------|----------|------------|
| 1 | Phase 1 (Data Infrastructure) | HIGH | — |
| 2 | Phase 2 (Minimap) | HIGH | Phase 1 |
| 3 | Phase 7 (/where) | MEDIUM | Phase 1 (coordinate conversion) |
| 4 | Phase 4 (Indicators) | HIGH | Phase 2 |
| 5 | Phase 3 (World Map) | HIGH | Phase 1 |
| 6 | Phase 5 (Loading Screens) | MEDIUM | — (independent) |
| 7 | Phase 8 (Monster Map) | LOW | Phase 3 |
| 8 | Phase 6 (Guide NPC) | LOW | Phase 2 + Phase 4 |
| 9 | Phase 9 (Polish) | MEDIUM | All above |

**Total estimated new files**: 10 C++ files (5 h/cpp pairs) + 1 server JS file + ~22 art assets
**Key dependencies**: Phase 1 must be done first. Phases 2 and 3 are the core deliverables.
