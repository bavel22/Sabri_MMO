# Sewer Dungeon F1 (`SewerDungeon01`) — Build Plan

| Field | Value |
|---|---|
| Zone name (server) | `SewerDungeon01` |
| Display name | `Sewer Dungeon F1` |
| Level asset | `L_SewerDungeon01` (in `Content/SabriMMO/Levels/`) |
| Type | `dungeon` |
| RO Classic reference | `prt_sewb1` (Prontera Culvert F1) |
| Fidelity target | High (1-2 weeks, screenshot-match) |
| Status | Phase A foundation complete (server, audio, post-process, fog, navmesh, water, spawns, warps) — visual polish pending. Last updated 2026-05-09 |
| Owner | pladr |

## Implementation status (2026-05-09)

| Done | Item | Where |
|---|---|---|
| ✓ | `SewerDungeon01` registered in `ZONE_REGISTRY` | `server/src/ro_zone_data.js` |
| ✓ | `prtsouth_to_dungeon` warp repurposed → `prtsouth_to_sewerdungeon01` (radius 200) | `server/src/ro_zone_data.js` `prontera_south.warps[]` |
| ✓ | `sewerdungeon01_to_prtsouth` exit warp (radius 250) | `server/src/ro_zone_data.js` `SewerDungeon01.warps[]` |
| ✓ | `SewerDungeon01` added to `ZONE_INFO` + `WORLD_MAP_GRID` row 5 col 8 | `server/src/ro_world_map_data.js` |
| ✓ | Pre-existing `buildWorldMapData` bugs fixed (spawnPool ignored, ENEMY_TEMPLATES wrong import name, element object handling) | `server/src/ro_world_map_data.js` |
| ✓ | PostProcess zone preset block (Bloom 0.15, Vignette 0.5, WhiteTemp 5500K) | `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp` `ApplyZonePreset()` |
| ✓ | Fog override — density 0.12, falloff 0.2, moss-green inscatter, FogHeight Z=500 | `PostProcessSubsystem.cpp` `SetupSceneLighting()` |
| ✓ | Volumetric fog enabled — scatter 0.7, albedo (80,110,80), extinction 1.0, start 0 | Same block — provides god rays through Point Lights |
| ✓ | Water canals placed in `L_SewerDungeon01` (`AWaterArea` actors) | UE5 level |
| ✓ | `SpawnAllowVolume` + `SpawnDenyVolume` placed + `ExportSpawnRegions SewerDungeon01` | `server/spawn_regions/SewerDungeon01.json` (1 allow + 1 deny) |
| ✓ | NavMesh exported + copied to `server/navmesh/SewerDungeon01.obj` | 1699 vertices, 1132 triangles |
| ✓ | Audio mappings — BGM `bgm_19` + ambient `DunWind+CaveDrip+CaveFall` (also added for `prt_sewb1-4`) | `AudioSubsystem.cpp` |
| ✓ | AWarpPortal placed + WarpId synced both sides | UE5 level + server registry |
| ✓ | C++ rebuilt and `UnrealEditor-SabriMMO.dll` stamped 2026-05-09 | Editor target build |
| ✓ | `r.LocalFogVolume.GlobalStartDistance=0` set permanently | `client/SabriMMO/Config/DefaultEngine.ini` `[SystemSettings]` — fixes UE5.7 hardcoded 20m near-fade |
| pending | `L_SewerDungeon01` greybox geometry replaced with sewer brick assets | UE5 level |
| pending | Hunyuan3D pipeline for 10 missing sewer assets | `_tools/hunyuan_asset_pipeline.py` — see Appendix B |
| pending | `MI_DungeonFloor_Wet`, `MI_DungeonWall_Mossy` material instances | `Scripts/Environment/create_sewer_dungeon_mis.py` |
| pending | Sewer slime decal MIs (×6) | `Scripts/Environment/create_sewer_decal_instances.py` |
| pending | Brazier Point Lights + `Volumetric Scattering Intensity = 3-4` per light | UE5 level (this is what creates god rays — without per-light scatter the volumetric flag alone produces uniform fog) |
| pending | Ceiling-grate Spotlights + `Volumetric Scattering Intensity = 5-8` | UE5 level |
| pending | 60-80 decals from `MI_RODecal_*` (DarkStain/Moss/Cracks/Path mix) | UE5 level |
| optional | `LocalFogVolume` actors for uneven density pockets | UE5 level (now render up close — cvar fix above) |

## TL;DR

A high-fidelity mimic of RO Classic Prontera Culvert F1 — a moss-grown brick sewer with stone canals of murky sewage running between corridors and rooms, lit by warm iron braziers and shafts of cool light dropping through ceiling grates. Static-mesh modular floor and walls (no Landscape Actor — this is an indoor dungeon archetype). Heavy fog, deep vignette, claustrophobic mood. **200 random spawns** via `spawnPool` distributed across `SpawnAllowVolume` actors (50 Thief Bug + 120 Thief Bug Egg + 15 Familiar + 15 Tarou). Water canals use `AWaterArea` for visual + auto-detected deep cells that cut the navmesh and gate Aqua Benedicta / Water Ball skills. Targets `bgm_19 "Under the Ground"` for BGM and stacks dungeon-wind + water-drop + waterfall ambient layers.

## Table of contents

1. [Overview](#1-overview)
2. [Architecture decisions](#2-architecture-decisions)
3. [Server changes](#3-server-changes)
4. [UE5 level setup](#4-ue5-level-setup)
5. [Materials & decals](#5-materials--decals)
6. [Lighting & post-process](#6-lighting--post-process)
7. [Audio](#7-audio)
8. [Water canals](#8-water-canals)
9. [3D asset generation (Hunyuan3D)](#9-3d-asset-generation-hunyuan3d)
10. [Layout sketch & room plan](#10-layout-sketch--room-plan)
11. [NavMesh export](#11-navmesh-export)
12. [Phased schedule](#12-phased-schedule)
13. [Test matrix](#13-test-matrix)
14. [Risk register](#14-risk-register)
15. [Appendix A — Existing imported asset inventory](#appendix-a--existing-imported-asset-inventory)
16. [Appendix B — Hunyuan3D per-asset prompts](#appendix-b--hunyuan3d-per-asset-prompts)
17. [Appendix C — Decal type → density formula](#appendix-c--decal-type--density-formula)
18. [Appendix D — Point Light placement guide](#appendix-d--point-light-placement-guide)
19. [Appendix E — Cross-references](#appendix-e--cross-references)
20. [Glossary](#glossary)

---

## 1. Overview

### What this zone is

A faithful mimic of Ragnarok Online Classic's `prt_sewb1` — the first floor of the Prontera Culvert system. In the RO universe, this is the underground sewer beneath Prontera, accessed via a stair in the Inn. It's a low-to-mid-level dungeon (level 8-25) where Thieves cut their teeth and the Bug Hunter sub-quest takes place. Visually it's defined by:

- Moss-thick brown brick walls
- Wet gray cobblestone floor with slime puddles and worn footpaths
- Stone canals carrying slow-moving murky sewage
- Narrow stone bridges spanning the canals
- Iron braziers casting warm pools at corridor intersections
- Shafts of cool light dropping through ceiling grates
- Drainage pipes weeping into the canals
- Cobwebs in dark corners, bone piles in dead-ends
- Heavy mossy-green fog layered over everything

### Why this zone first

It exercises every dungeon-specific system in the project (no Landscape, AWaterArea mixed-mode, Point Light pools, dungeon ambient stack, modular static-mesh construction, dungeon BGM map) without needing the giant scope of a town zone. A successful build here is the template for `SewerDungeon02/03/04`, all dungeons in `gef_dun*` and `gl_*`, and the Pyramids. It's also the smallest zone that can flex the high-fidelity asset pipeline (10 new Hunyuan3D meshes hand-painted in 3D Coat) so we validate that workflow on a manageable scope before scaling to 20+ assets per zone.

### Naming

| Item | Value |
|---|---|
| Zone name (server `ro_zone_data.js` key) | `SewerDungeon01` |
| Display name (UI strings) | `Sewer Dungeon F1` |
| UE5 level filename | `L_SewerDungeon01.umap` |
| UE5 level path | `Content/SabriMMO/Levels/L_SewerDungeon01` |
| Warp ID (entrance from Prontera) | `sewerdungeon01_to_prt` |
| Warp ID (descent to F2) | `sewerdungeon01_to_sewerdungeon02` |
| Reverse warp ID (Prontera to here) | `prt_to_sewerdungeon01` |
| Water area IDs | `sewerdungeon01_canal_main`, `sewerdungeon01_canal_east`, etc. |

### Fidelity target

**High** — 1-2 week build, side-by-side screenshot validation against RO Classic references. Acceptance criteria:

- Visually reads as "this is clearly a copy of prt_sewb1" to anyone who's played RO
- Decal placement is per-camera-pass (walked the level from player POV, refined where the eye lingers)
- All Hunyuan3D-generated assets pass through 3D Coat hand-paint cohesion (no photoreal-leaning textures)
- Lighting tuned per RO atmospheric reference (warm pools + cool grate shafts + heavy moss-green fog)
- Sewage water palette tuned darker than the default `AWaterArea` (greenish-black, not blue)
- Custom material instances for wet floor + mossy wall (not just stock dungeon MIs)

---

## 2. Architecture decisions

### Decision matrix

| Decision | Choice | Rationale |
|---|---|---|
| Ground actor | **Static mesh modular tiles** (NOT Landscape Actor) | Dungeon archetype — no slope, no grass, no sky. Hand-laid corridors. Use `M_Landscape_*` is wrong here — Landscape's slope detection and grass scatter add nothing for an enclosed sewer. |
| Wall actor | **Static mesh modular wall pieces** | Same reason. Walls need to be hand-laid for the maze layout. Procedural slope-rock won't produce sewer brick. |
| Ceiling | **Static mesh slab** above corridors | Indoor archetype — must occlude sky. Without a ceiling, ambient skylight bleeds in. |
| Water | **`AWaterArea` actor with custom palette** | Auto-detects deep cells from raycast → spawns `NavArea_Null` boxes that cut the UE5 navmesh AND the server-side navmesh OBJ. Gates Aqua Benedicta + Water Ball skills automatically. The shallow-canal use case is a perfect fit for the mixed-mode design. |
| Lighting | **Point Lights only — NO Directional Light, NO Sky Light** | Indoor dungeon. `PostProcessSubsystem::SetupSceneLighting` already handles this when `flags.indoor === true` is reflected in the per-zone preset. |
| Post-process | **Per-zone preset in `PostProcessSubsystem.cpp`** | Heavy vignette (0.5), low bloom (0.15), neutral exposure, slight cool tint. Unique to this zone. |
| Fog | **Heavy Exponential Height Fog** | Density 0.05 (10x outdoor), dark moss-green inscatter. Critical for atmosphere. |
| BGM | `bgm_19 "Under the Ground"` | Canonical RO sewer track per the audio research doc. |
| Ambient | `se_dun_wind01` + `se_subterranean_waterdrop_01` + `se_subterranean_waterfall_01` | Dungeon-wind base layer + per-area drips and falls. |
| Map flag — `indoor` | **`true`** | Skips outdoor lighting in `PostProcessSubsystem`. |
| Map flag — `nosave` | **`true`** | Kafra cannot save here — RO Classic standard for sewers. |
| Map flag — `noteleport` | **`false`** | Fly Wing IS allowed in RO Classic prt_sewb1 — players use it for monster aggro escape. |
| Map flag — `noreturn` | **`false`** | Butterfly Wing works, returns to save point. |
| Map flag — `pvp` | **`false`** | Standard. |
| Map flag — `town` | **`false`** | Not a town — enemies spawn. |
| NPCs | **None for F1** | The sewer is hostile, no shops/Kafras. The optional Bug Hunter quest NPC lives in Prontera, not here. Defer NPC additions to F2/F3 if needed. |

### Why NOT use a Landscape Actor

The `sabrimmo-landscape` skill is explicit: Landscape Actor is for outdoor zones with sculpted terrain, grass scatter, and slope-based gameplay (rock at >45° = unclimbable). A sewer has none of those — it's a hand-laid maze. Forcing a Landscape under a sewer floor would:

- Waste memory on a heightfield we never sculpt
- Confuse the per-zone material system (no grass-warm/grass-cool/dirt/rock blend makes sense here)
- Trigger the auto-grass scatter (would be empty but still cost runtime)
- Make it harder to author the floor (every variation requires sculpting + paint layers vs just placing a different mesh)

The correct approach for dungeons is a single floor mesh per tile, walls placed by hand, and `MI_RO_DungeonFloor` / `MI_RO_DungeonWall` (or new variants) applied directly.

### Why use `AWaterArea` and not just translucent planes

- Auto-detects deep cells via downward raycast grid (16x16 default)
- Spawns `UBoxComponent` nav-modifier boxes per deep rectangle → cuts UE5 navmesh
- Works with the server-side `findClosestNavMeshPoint` validation backstop (rejects player movement into deep cells)
- Provides depth-graded color/opacity (shallow → deep → abyss) without writing a custom material
- Hooks into `water:enter` / `water:exit` socket events for skill gating (Aqua Benedicta, Water Ball)
- Mixed-mode (per-cell deep detection) lets us have shallow walkable canal edges with deep middles — perfect for sewers

---

## 3. Server changes

### 3.1 `SewerDungeon01` registry entry — committed 2026-05-07

The actual block now in `server/src/ro_zone_data.js`. The user placed the AWaterArea actors in `L_SewerDungeon01` directly, so `waterAreas[]` is omitted from the registry — the AWaterArea actors carry their own visual + nav-cut behavior, and players in the level still get correct `water:enter` / `water:exit` events. (`waterAreas[]` in the registry is only required if you also want the server-side `isInDeepWater()` AABB check against zone-data deep entries; for mixed-mode actors the navmesh-validation backstop handles deep blocking on its own.)

```javascript
SewerDungeon01: {
    name: 'SewerDungeon01',
    displayName: 'Sewer Dungeon F1',
    type: 'dungeon',
    flags: {
        noteleport: false,    // Fly Wing allowed (RO Classic prt_sewb1 behavior)
        noreturn: false,      // Butterfly Wing works (back to last save point)
        nosave: true,         // Kafra cannot save here
        pvp: false,
        town: false,
        indoor: true          // skips outdoor lighting in PostProcessSubsystem
    },
    defaultSpawn: { x: 3370, y: -1390, z: 620 },  // matches the prtsouth → here warp destination
    levelName: 'L_SewerDungeon01',
    warps: [
        // Exit upward to Prontera South Field
        {
            id: 'sewerdungeon01_to_prtsouth',
            x: 3990, y: -1390, z: 620,
            radius: 200,
            destZone: 'prontera_south',
            destX: -650, destY: 10290, destZ: 30
        }
        // F2 descent warp — uncomment when SewerDungeon02 ships.
        // AWarpPortal in L_SewerDungeon01 at (3990, 1350, 290) should currently be a
        // closed-gate static mesh (or AWarpPortal with a WarpId that doesn't match any
        // registered warp — server will emit zone:error). Players returning from F2
        // will spawn at (3680, 1350, 340) per the planned reverse warp.
        // {
        //     id: 'sewerdungeon01_to_sewerdungeon02',
        //     x: 3990, y: 1350, z: 290,
        //     radius: 200,
        //     destZone: 'SewerDungeon02',
        //     destX: 0, destY: 0, destZ: 0   // set when F2 is built
        // }
    ],
    kafraNpcs: [],
    enemySpawns: [],  // cleared — using spawnPool below.
    // ── Random spawnPool — distributed across SpawnAllowVolume actors ─────
    // Setup steps in UE5:
    //   1. Place SpawnAllowVolume actors covering walkable rooms/corridors in L_SewerDungeon01.
    //   2. (Optional) Place SpawnDenyVolume actors over canal water / unreachable nooks.
    //   3. Editor console: ExportSpawnRegions SewerDungeon01
    //      → writes server/spawn_regions/SewerDungeon01.json
    //   4. Restart server. Points are NavMesh-snapped to ground at runtime.
    // Total: 200 spawns (50 + 120 + 15 + 15) distributed evenly across the allow volumes.
    spawnPool: [
        { template: 'thief_bug',     count: 50,  wanderRadius: 350 },   // lvl 6  (aggressive)
        { template: 'thief_bug_egg', count: 120, wanderRadius: 80  },   // lvl 4  (stationary)
        { template: 'farmiliar',     count: 15,  wanderRadius: 400 },   // lvl 24 (flying bat, fast melee)
        { template: 'tarou',         count: 15,  wanderRadius: 350 }    // lvl 11 (aggressive rat)
    ]
}
```

**Total spawns: 200** (50 Thief Bugs + 120 Eggs + 15 Familiars + 15 Tarous). The `spawnPool` mechanism distributes them evenly across the `SpawnAllowVolume` actors in `L_SewerDungeon01`, NavMesh-snapped to ground at runtime. Adjust counts here to scale density up or down without re-exporting the level. **Pending**: place `SpawnAllowVolume` actors in the level and run `ExportSpawnRegions SewerDungeon01` in the editor console, otherwise no spawns will occur (the server logs a warning at startup if `server/spawn_regions/SewerDungeon01.json` is missing).

### 3.2 Reverse warp in `prontera_south` — committed 2026-05-07

The existing `prtsouth_to_dungeon` warp at `(-650, 9560, -20)` was **repurposed** to point to `SewerDungeon01` instead of `prt_dungeon_01`. The physical `AWarpPortal` actor in `L_PrtSouth` keeps the same world position. **You must update the actor's `WarpId` property** in the editor:

| Was | Now |
|---|---|
| `WarpId = "prtsouth_to_dungeon"` | `WarpId = "prtsouth_to_sewerdungeon01"` |

`prt_dungeon_01` remains in the registry and is still reachable via the Prontera Kafra teleport menu (entry: `kafra_prontera_1.destinations` in `prontera.kafraNpcs[]`). If you want a separate world-space warp to it, add a new `AWarpPortal` somewhere else in `L_PrtSouth` with its own `WarpId` and add a matching warp definition.

The committed block in `prontera_south.warps[]`:

```javascript
{
    // Repurposed: was prtsouth_to_dungeon → prt_dungeon_01.
    // The AWarpPortal actor in L_PrtSouth at (-650, 9560, -20) now points to SewerDungeon01.
    // prt_dungeon_01 is still reachable via Prontera Kafra teleport.
    id: 'prtsouth_to_sewerdungeon01',
    x: -650, y: 9560, z: -20,
    radius: 200,
    destZone: 'SewerDungeon01',
    destX: 3370, destY: -1390, destZ: 620
}
```

### 3.3 `server/src/ro_world_map_data.js` — committed 2026-05-07

Added to `ZONE_INFO` (note the actual schema uses `category` + `levelRange` + `biome`, not `type` + `level` + `monsters[]` — the monsters list is auto-generated from `ZONE_REGISTRY` spawns at runtime by `buildWorldMapData`):

```javascript
SewerDungeon01: { displayName: 'Sewer Dungeon F1', category: 'dungeon', levelRange: '15-25', biome: 'underground' },
```

Placed on `WORLD_MAP_GRID` at row 5 col 8 (replaced a `null` ocean cell south-east of the Prontera region). Verified no duplicates:

```bash
$ node -e "const d = require('./server/src/ro_world_map_data'); const flat = d.WORLD_MAP_GRID.flat().filter(Boolean); const dupes = flat.filter((z,i) => flat.indexOf(z) !== i); console.log(dupes.length ? 'DUPES: ' + dupes : 'OK')"
OK: no duplicates
```

### 3.4 Database — no migration needed

`zone_name` is a free-text VARCHAR column on `characters` — new zone names are supported automatically. `level_name` is computed on-the-fly from `getZone(zone_name).levelName` in `GET /api/characters` — no DB column required.

### 3.5 Map flags rationale

| Flag                | Value   | RO Classic source                                                      |
| ------------------- | ------- | ---------------------------------------------------------------------- |
| `indoor: true`      | `true`  | All sewer floors disable outdoor lighting.                             |
| `nosave: true`      | `true`  | Kafra reject save attempts in dungeons.                                |
| `noteleport: false` | `false` | iRO confirmed: Fly Wing usable in prt_sewb1 (monster escape mechanic). |
| `noreturn: false`   | `false` | Butterfly Wing works — returns to last save point in nearest town.     |
| `pvp: false`        | `false` | No PvP in PvE dungeons.                                                |
| `town: false`       | `false` | Enemies spawn here.                                                    |

---

## 4. UE5 level setup

### 4.1 Duplicate `L_PrtDungeon01` → `L_SewerDungeon01`

**Never create a level from `File > New Level`** — the Level Blueprint that spawns and possesses `BP_MMOCharacter` is the easiest critical piece to forget, and missing it causes a 10-second hang on the loading screen with no pawn appearing. Always duplicate an existing working level.

Steps:

1. In UE5 Content Browser, navigate to `Content/SabriMMO/Levels/`
2. Right-click `L_PrtDungeon01` → Duplicate
3. Rename the copy to `L_SewerDungeon01`
4. Open `L_SewerDungeon01`
5. Select all geometry actors (cubes, walls, etc. from PrtDungeon01) — delete them
6. Keep these actors intact:
   - Level Blueprint (verify it's present — see 4.3)
   - Existing `NavMeshBoundsVolume` (resize later)
   - Existing lighting actors (delete and re-place per Section 6)
   - World Settings GameMode override

### 4.2 World Settings — verify

Window → World Settings:

| Setting | Value |
|---|---|
| GameMode Override | `GM_MMOGameMode` |
| Default Pawn Class (in GM_MMOGameMode → Class Defaults) | **None** (NOT BP_MMOCharacter) |

If `Default Pawn Class` is set, the GameMode will spawn a duplicate pawn at world origin alongside the Level Blueprint's spawn. Critical: leave it None. The C++ base class `ASabriMMOGameMode` already sets it to `nullptr` — check the BP doesn't override.

### 4.3 Level Blueprint — verify (CRITICAL)

Open the Level Blueprint (Window → Level Blueprint) and verify the 3 sections from `L_PrtDungeon01` were copied:

**A. Spawn and Possess Character (Event BeginPlay):**

1. `Event BeginPlay` → `Delay 0.2s` → `Cast To MMOGameInstance` (from `Get Game Instance`)
2. `Get Selected Character` → `Break Character Data` → extract `CharacterId`, `X`, `Y`, `Z`
3. Branch: `CharacterId > 0`
4. Branch: `Vector Length Squared(Make Vector(X,Y,Z)) != 0.0`
   - True: SpawnActor `BP_MMOCharacter` at `(X, Y, Z)` → Possess
   - False: SpawnActor `BP_MMOCharacter` at default `(0, 0, 100)` → Possess
5. `Set Timer by Function Name` ("SaveCharacterPosition", 5.0s, Looping=true)

**B. SaveCharacterPosition (Custom Event, every 5s):**

- Cast → Get Selected Character → CharacterId, then Get Player Character(0) → Get Actor Location → Save Character Position

**C. Cleanup (Event End Play):**

- Clear Timer by Function Name "SaveCharacterPosition"

If any of the 3 sections is missing, abort — re-duplicate from `L_PrtDungeon01`. Don't try to rebuild the BP from scratch.

### 4.4 Required actors checklist

Per `Zone_System_UE5_Setup_Guide.md`:

- [ ] **Level Blueprint** with the 3 sections above (verified)
- [ ] **GameMode Override** = `GM_MMOGameMode`, **DefaultPawnClass = None** (verified)
- [ ] **NavMeshBoundsVolume** sized to cover playable area (~6500 × 6500 × 1000 UU, scaled later to match the maze)
- [ ] Lighting per Section 6 (Point Lights + Spotlights at ceiling grates; NO Directional Light, NO Sky Light, NO Sky Atmosphere)
- [ ] **AWarpPortal** at the exit-up-to-Prontera location (matches `sewerdungeon01_to_prt` server warp coords within 400 UE units)
- [ ] **AWarpPortal** at the descent-to-F2 location (closed gate mesh until F2 ships)
- [ ] **AWaterArea** for each canal, IDs matching server `waterAreas[]` entries

Do **NOT** place these (removed in Phase 6, replaced by C++ subsystems):

- `BP_OtherPlayerManager`
- `BP_EnemyManager`

`BP_SocketManager` is dead code, kept in levels for compatibility — do not place a new one.

### 4.5 Greybox layout strategy (Days 1-2)

Block out the maze with simple cubes scaled to:
- Corridor walls: 400×100×400 UU (W × thickness × H)
- Corridor floor segments: 400×400×30 UU
- Room floors: 1500×1500×30 UU
- Ceilings: same footprint as floors, placed at Z=400 above floor

Use `M_Cube_BlockGreen` (or any visible debug material) for greybox. Don't apply final materials yet — greybox is for layout iteration only.

Goal of greybox: validate the layout walks well, doors line up, water canals route through corridors logically, room sizes feel right. Iterate before placing real assets — moving 50 brick wall meshes is more painful than moving 50 cubes.

---

## 5. Materials & decals

### 5.1 Floor materials

| Surface                                  | Material                             | Notes                                                                          |
| ---------------------------------------- | ------------------------------------ | ------------------------------------------------------------------------------ |
| Floor (dry, away from water)             | `MI_RO_DungeonFloor` (existing)      | Acceptable for blockout. Already in `Content/SabriMMO/Materials/Environment/`. |
| Floor (wet, near canals + after bridges) | `MI_DungeonFloor_Wet` (NEW — create) | See 5.4 for creation.                                                          |

### 5.2 Wall materials

| Surface                                               | Material                              | Notes                    |
| ----------------------------------------------------- | ------------------------------------- | ------------------------ |
| Wall (clean section, near entrance and central rooms) | `MI_RO_DungeonWall` (existing)        | Acceptable for blockout. |
| Wall (mossy section, deeper rooms and dead-ends)      | `MI_DungeonWall_Mossy` (NEW — create) | See 5.4 for creation.    |

### 5.3 Decal selection — sewer-specific palette

Drop **60-80 decals total** across the level. Distribution:

| Decal MI base                                  | Count | Tint                                                   | Use                                             |
| ---------------------------------------------- | ----- | ------------------------------------------------------ | ----------------------------------------------- |
| `M_Decal_DarkStain` (slime green tint variant) | 30-40 | Override DecalTint to `(0.15, 0.30, 0.10)` slime green | Slime puddles around water canals, room edges   |
| `MI_RODecal_Moss_*`                            | 12-16 | Default warm brown tint `(0.85, 0.80, 0.72)`           | Wall bases, around drainage pipes, room corners |
| `MI_RODecal_Cracks_*`                          | 9-12  | Default                                                | Broken floor sections, cracked walls            |
| `MI_RODecal_DarkStain_*`                       | 6-8   | Default                                                | Blood/grime stains, old stains in dead-ends     |
| `MI_RODecal_Path_*`                            | 3-4   | Default                                                | Worn floor in heavily-trafficked main corridor  |

**Rules** (from `_meta/04_Decals_Guide.md` and `feedback-decal-textures`):

- Pitch = **−90°** (projects straight down onto floor)
- Yaw = random 0-360°
- Scale: X = 1.5-5.0 (projection depth), Y/Z = 1.5-5.0 (footprint)
- Place slightly above floor surface
- Opacity stays at 0.35 (default — never crank above 0.5 or it looks painted-on)
- Slight overlap is good — natural coverage
- Mix 3+ types in the same area for richness
- **NEVER use AI-generated decal textures** (`Textures/Environment/Decals/`) — they look garish. Use only `MI_RODecal_*` (RO original textures from `RO_Original/`).

### 5.4 New material instance creation

Create two new MIs via UE5 Python (run in Editor → Execute Python Script). Save scripts at `client/SabriMMO/Scripts/Environment/`:

#### `create_sewer_dungeon_mis.py`

```python
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

# Path to save the new MIs
TARGET_PATH = "/Game/SabriMMO/Materials/Environment/SewerDungeon"

# 1. MI_DungeonFloor_Wet — wet/glossy variant of MI_RO_DungeonFloor
parent_floor = unreal.load_asset("/Game/SabriMMO/Materials/Environment/M_Landscape_RO_09")
factory = unreal.MaterialInstanceConstantFactoryNew()

mi_wet = asset_tools.create_asset("MI_DungeonFloor_Wet", TARGET_PATH,
    unreal.MaterialInstanceConstant, factory)
mi_wet.set_editor_property("parent", parent_floor)

# Override the wet floor look — increase glossiness, slight cool tint
mel.set_material_instance_scalar_parameter_value(mi_wet, "Roughness", 0.30, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_vector_parameter_value(mi_wet, "WarmthTint",
    unreal.LinearColor(0.85, 0.88, 0.92, 1.0),  # cool tint
    unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi_wet, "BrightnessOffset", -0.10, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi_wet, "SaturationMult", 0.85, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

unreal.EditorAssetLibrary.save_asset(mi_wet.get_path_name())

# 2. MI_DungeonWall_Mossy — mossy variant of MI_RO_DungeonWall
mi_mossy = asset_tools.create_asset("MI_DungeonWall_Mossy", TARGET_PATH,
    unreal.MaterialInstanceConstant, factory)
mi_mossy.set_editor_property("parent", parent_floor)

# Override with mossy texture (use a green-shifted ground texture)
mossy_tex = unreal.load_asset("/Game/SabriMMO/Textures/Environment/Biomes/forest/T_forest_001")
mel.set_material_instance_texture_parameter_value(mi_mossy, "GrassWarmTexture", mossy_tex,
    unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_vector_parameter_value(mi_mossy, "WarmthTint",
    unreal.LinearColor(0.65, 0.78, 0.55, 1.0),  # green-shifted
    unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi_mossy, "SaturationMult", 1.10, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi_mossy, "BrightnessOffset", -0.15, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi_mossy, "Roughness", 0.92, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

unreal.EditorAssetLibrary.save_asset(mi_mossy.get_path_name())

print("Created MI_DungeonFloor_Wet and MI_DungeonWall_Mossy")
```

Run via UE5 Editor → File → Execute Python Script → select this file.

**Versioning:** if you re-tune these values, save as `MI_DungeonFloor_Wet_v2`, never overwrite (per `feedback-material-versioning`).

### 5.5 Decal tint override script

Create slime-green variants of `M_Decal_DarkStain` for sewer slime puddles. Save at `client/SabriMMO/Scripts/Environment/create_sewer_decal_instances.py`:

```python
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

TARGET = "/Game/SabriMMO/Materials/Environment/Decals/SewerDungeon"
parent = unreal.load_asset("/Game/SabriMMO/Materials/Environment/Decals/M_Decal_DarkStain")
factory = unreal.MaterialInstanceConstantFactoryNew()

# Use 6 different RO original textures as decal sources for variation
SOURCE_TEXTURES = [
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_001",  # adjust to actual filenames
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_005",
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_012",
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_018",
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_023",
    "/Game/SabriMMO/Textures/Environment/RO_Original/T_RO_029"
]

for i, tex_path in enumerate(SOURCE_TEXTURES, start=1):
    mi_name = f"MI_RODecal_SewerSlime_{i:02d}"
    mi = asset_tools.create_asset(mi_name, TARGET, unreal.MaterialInstanceConstant, factory)
    mi.set_editor_property("parent", parent)

    tex = unreal.load_asset(tex_path)
    if tex:
        mel.set_material_instance_texture_parameter_value(mi, "DecalTexture", tex,
            unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    # Slime-green tint instead of warm brown
    mel.set_material_instance_vector_parameter_value(mi, "DecalTint",
        unreal.LinearColor(0.15, 0.30, 0.10, 1.0),
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)
    mel.set_material_instance_scalar_parameter_value(mi, "OpacityStrength", 0.35,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER)

    unreal.EditorAssetLibrary.save_asset(mi.get_path_name())

print(f"Created {len(SOURCE_TEXTURES)} sewer slime decal instances")
```

---

## 6. Lighting & post-process

### 6.1 PostProcessSubsystem zone preset (REQUIRED)

Add to `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp` in `ApplyZonePreset()`. Find the section with `else if (ZoneName == TEXT("prt_dungeon_01"))` and add this block after it:

```cpp
else if (ZoneName == TEXT("SewerDungeon01"))
{
    Bloom = 0.15f;            // minimal bloom — no bright sun
    Vignette = 0.5f;          // heavy — claustrophobic
    ExposureBias = 0.0f;      // dungeon, no boost
    WhiteTemp = 5500.f;       // slightly warm to balance cool fog
    GainHighlights = FVector4(0.95f, 0.92f, 0.98f, 1.0f);  // subtle cool blue-green tint
}
```

This requires a recompile (`./Build.bat SabriMMO Win64 Development -Project=...` or compile from inside UE5 editor with Live Coding).

### 6.2 Auto-lighting block

In `SetupSceneLighting()`, the existing dungeon block handles `flags.indoor === true` for `prt_dungeon_01`. Same code applies to `SewerDungeon01` automatically because the zone is type `dungeon` with `indoor: true`. No new code needed in this method.

Auto-lighting result:
- DirectionalLight intensity = 0 (off)
- SkyLight intensity = 0.3, color (0.4, 0.4, 0.6) cool blue (existing dungeon default)
- HeightFog density = 0.03, falloff 2.0, inscatter (0.1, 0.08, 0.15) dark purple (existing dungeon default)

### 6.3 Per-zone fog override + volumetric fog (REQUIRED — actual implementation 2026-05-09)

The default dungeon fog (density 0.03, falloff 2.0) renders as **invisible** in this zone because the sewer floor sits at Z=620 — falloff 2.0 makes density drop to ~0 at altitude. Three fixes layered together:

1. **Shallow falloff** so fog reaches the player altitude
2. **Lift FogHeight** (actor Z) close to the floor plane
3. **Volumetric fog ON** with forward scattering — creates god rays through Point Lights/Spotlights

The actual block now in `PostProcessSubsystem::SetupSceneLighting()` (after the `if (HeightFog)` block):

```cpp
// Per-zone fog density overrides (high-fidelity zones)
if (ZoneName == TEXT("SewerDungeon01") && HeightFog)
{
    if (UExponentialHeightFogComponent* FC = HeightFog->GetComponent())
    {
        // ── Base ExponentialHeightFog (screen-space tonemap layer) ──
        FC->SetFogDensity(0.12f);                                       // light atmospheric base layer
        FC->SetFogHeightFalloff(0.2f);                                  // 10× shallower than dungeon default — fog reaches Z=620
        FC->SetFogInscatteringColor(FLinearColor(0.15f, 0.20f, 0.15f)); // moss green
        FC->SetStartDistance(0.f);

        // ── Volumetric fog (true 3D voxel grid, lights scatter through it) ──
        // Forward scatter at 0.7 = strong "Mie" phase function: light bright near the
        // source, falls off to ambient outward → god rays from braziers, soft halo
        // around ceiling-grate Spotlights, depth visible through the volume.
        FC->SetVolumetricFog(true);
        FC->SetVolumetricFogScatteringDistribution(0.7f);
        FC->SetVolumetricFogAlbedo(FColor(80, 110, 80));                // greenish tint on scattered light
        FC->SetVolumetricFogExtinctionScale(1.0f);                       // moderate — leaves headroom for LocalFogVolumes
        FC->SetVolumetricFogStartDistance(0.f);
    }
    HeightFog->SetActorLocation(FVector(0.f, 0.f, 500.f));              // FogHeight lifted near floor plane
}
```

**For visible god rays you MUST also bump per-light `Volumetric Scattering Intensity` in the editor** — see Section 6.4. Without that, the volumetric flag alone produces uniform fog that looks identical to the screen-space layer.

See `feedback-volumetric-fog-altitude.md` (memory) for the general pattern any high-floor zone should follow.

### 6.3.1 LocalFogVolume hardcoded near-fade fix (REQUIRED if using LocalFogVolume actors)

UE5.7 hardcodes `r.LocalFogVolume.GlobalStartDistance = 2000` (cm = 20 m). LocalFogVolume actors only render past 20 m and fade as you approach — they vanish completely when the camera is inside or near them. This is engine source — `Engine/Source/Runtime/Renderer/Private/LocalFogVolumeRendering.cpp:65`.

To make local volumes render up close (required for atmospheric pockets in interior zones), add to `client/SabriMMO/Config/DefaultEngine.ini`:

```ini
[SystemSettings]
r.LocalFogVolume.GlobalStartDistance=0
```

Survives shipping builds (the file is staged into the packaged game's Config folder). No C++ rebuild needed — engine reads `[SystemSettings]` at startup. Editor needs a restart to apply, OR run `r.LocalFogVolume.GlobalStartDistance 0` once in console for the current session.

See `feedback-localfog-near-fade.md` (memory) for full reference.

### 6.4 Manual lighting in the level (Point Lights + Spotlights)

The auto-lighting only handles the global ambient. The atmosphere comes from hand-placed lights in the level itself.

**Point Lights at brazier_iron meshes** (warm pools):

| Setting | Value |
|---|---|
| Light Color | RGB (1.0, 0.55, 0.20) — warm orange |
| Intensity | 800 cd |
| Attenuation Radius | 400 UU |
| Source Radius | 5 |
| Source Length | 0 |
| Cast Shadows | **true** (required — shadows are what create the dark gaps between fog rays) |
| **Volumetric Scattering Intensity** (Light → Advanced) | **3.0 to 4.0** (default 1.0 is too subtle — this is where 80% of the god-ray effect comes from) |
| Cast Volumetric Shadow | true (default) |
| Use IES Profile | optional (for flicker-style projection) |

Place one Point Light at every `brazier_iron.glb` mesh, ~10 cm above the brazier basin.

**Spotlights through ceiling grates** (cool light shafts):

| Setting | Value |
|---|---|
| Light Color | RGB (0.5, 0.7, 1.0) — cool blue |
| Intensity | 1500 cd |
| Inner Cone Angle | 25° |
| Outer Cone Angle | 35° |
| Attenuation Radius | 800 UU |
| Cast Shadows | **true** |
| **Volumetric Scattering Intensity** | **5.0 to 8.0** (Spotlights cone shape produces dramatic shafts at high values) |
| Cast Volumetric Shadow | true |

Place one Spotlight per ceiling grate, pointed straight down, ~50 UU above the ceiling slab. These simulate daylight from above and break up the gloom.

**Critical**: without `Volumetric Scattering Intensity > 1.0` per light, the volumetric fog from Section 6.3 looks identical to the cheap screen-space tonemap. The volumetric flag enables the 3D voxel grid; the per-light scatter is what writes density INTO that grid. Both are required.

**Optional: Point Lights at crystal meshes** (phosphorescent accent):

| Setting | Value |
|---|---|
| Light Color | RGB (0.4, 0.7, 0.9) cyan / (0.4, 0.9, 0.5) green |
| Intensity | 200 cd |
| Attenuation Radius | 250 UU |
| Cast Shadows | false (cheap, no shadow cost) |

For dim back-rooms with `crystal_blue` or `crystal_green` clusters.

**Light count budget**: aim for 15-25 lights total in the level. Past 30, performance degrades on lower-spec GPUs even with unbatched Lumen. Each light costs ~0.5 ms in lumen evaluation.

See [Appendix D — Point Light placement guide](#appendix-d--point-light-placement-guide) for room-by-room counts.

### 6.5 What NOT to do

Per `RO_Classic_Visual_Style_Research.md`:

- **Do NOT enable cel-shading post-process**
- **Do NOT enable outline post-process**
- **Do NOT override `ColorSaturation` or `ColorContrast`** (affects sprites — use material parameters instead)
- **Do NOT enable any custom PP material in `WeightedBlendables`** (the global cutout PP is already there for sprite-vs-world rendering — adding more darkens the scene)

---

## 7. Audio

### 7.1 BGM mapping

Add to `client/SabriMMO/Source/SabriMMO/Audio/AudioSubsystem.cpp` in `OnWorldBeginPlay`, in the BGM mapping section:

```cpp
ZoneToBgmMap.Add(TEXT("SewerDungeon01"), BgmRoot + TEXT("bgm_19"));   // Under the Ground
```

Track 19 ("Under the Ground") is the canonical RO Classic sewer track per the audio research doc — used for `prt_sewb1-4` in the original game.

### 7.2 Ambient layer mapping

Add to the ambient mapping section:

```cpp
ZoneAmbientMap.Add(TEXT("SewerDungeon01"), {
    DungeonWind1,         // se_dun_wind01 — base dungeon wind
    SubterraneanDrop,     // se_subterranean_waterdrop_01 — cave drips
    SubterraneanFall      // se_subterranean_waterfall_01 — for sewage runoff areas
});
```

The `DungeonWind1`, `SubterraneanDrop`, `SubterraneanFall` constants are already declared at the top of the ambient block. If not, add them:

```cpp
const FString DungeonWind1   = AmbientRoot + TEXT("se_dun_wind01");
const FString SubterraneanDrop = AmbientRoot + TEXT("se_subterranean_waterdrop_01");
const FString SubterraneanFall = AmbientRoot + TEXT("se_subterranean_waterfall_01");
```

### 7.3 3D positional ambient sources (HIGH-FIDELITY OPTION)

For per-canal water gurgle that gets louder as you approach, add to `Zone3DAmbientMap`:

```cpp
Zone3DAmbientMap.Add(TEXT("SewerDungeon01"), {
    // Water gurgle at the main canal junction
    { TEXT("se_subterranean_waterdrop_01"), FVector(0, 0, 70), 500.f, 2000.f, 0.7f },
    // Waterfall sound where canal pours into the deep cell
    { TEXT("se_subterranean_waterfall_01"), FVector(1200, -1500, 70), 800.f, 3000.f, 0.6f },
    // Brazier crackle at central intersection
    { TEXT("se_brazier_crackle"), FVector(0, 800, 100), 200.f, 600.f, 0.5f }
});
```

Adjust positions to match the actual canal/brazier locations after the level is built.

### 7.4 Skill SFX

No zone-specific work needed — `SkillImpactSoundMap` already handles all skill IDs. The water-gating on Aqua Benedicta (413) and Water Ball (412) is automatic via the server's `player.inWater` state.

---

## 8. Water canals

### 8.1 Place AWaterArea actors

For each canal, place an `AWaterArea` actor:

1. Place Actors → search "WaterArea" → drag into level
2. Position the actor at the canal center (Z slightly below floor surface, e.g. Z = 50 if floor is at Z = 100)
3. Set properties in Details panel:
   - `WaterAreaId`: must match a `waterAreas[]` entry in `ro_zone_data.js` (e.g., `sewerdungeon01_canal_main`)
   - `WaterExtent`: half-extent matching the canal footprint, e.g., (200, 1500) for a 400 × 3000 UU canal
   - `bAutoDetectDeep`: **true** (default — let the actor raycast and detect deep cells per cell)
   - `DeepDepthThreshold`: 200 (default — vertical drop in UE units to mark a cell deep)
   - `DepthSampleResolution`: 16 (default — 16×16 raycast grid)
4. Visual properties for sewage:
   - `ShallowColor`: `(0.18, 0.22, 0.10)` — murky algae green
   - `DeepColor`: `(0.08, 0.10, 0.04)` — algae black
   - `AbyssColor`: `(0.02, 0.03, 0.01)` — near-black
   - `AbyssOpacity`: `1.0` — fully opaque at depth
   - `AbyssDepth`: `900` — depth where opacity reaches 1.0
   - `WaveSpeed`: `0.2` — slow, stagnant sewage feel

### 8.2 Save → Build Navigation → verify cuts

After placing AWaterArea actors:

1. **Save the level** (the actor's `OnConstruction` raycasts on save and spawns the deep-cell nav-modifier boxes)
2. Build → Build Navigation
3. Press **P** to visualize the green nav overlay — verify there are HOLES where deep canal cells were detected
4. If no holes appear, the floor mesh under the canal is missing or too far down — adjust the AWaterArea Z position or the floor mesh

### 8.3 Mixed-mode: shallow edges + deep middles

The AWaterArea raycast grid will detect:
- Shallow cells where the floor is just below the water level (shore, bridges) → no nav cut, walkable
- Deep cells where the floor is missing or far below (canal middle) → nav cut, blocked

This is what we want — the player can step into the shallow shore but can't walk across the canal. Bridges (placed as static meshes spanning the canal) sit on the navmesh and are walkable.

### 8.4 NavMesh validation backstop

The server-side `findClosestNavMeshPoint` validation (in `index.js`, `player:position` handler) catches any client-side movement attempts into deep cells (>100 UU off-navmesh) and emits `player:position_rejected` with `reason: 'off_navmesh'`. This is automatic for any zone that has a navmesh OBJ.

### 8.5 Skill gating

Aqua Benedicta (skill ID 413) and Water Ball (412) require `player.inWater === true`. The `water:enter` socket event sets this when the player enters any AWaterArea. The server's `player.activeWaterAreas` Map is reference-counted — overlapping AWaterAreas are handled correctly.

For sewer F1, players can:
- Stand in shallow canal edges → can cast Aqua Benedicta + Water Ball
- Walk on bridges over canals → out of water, skills disabled
- Try to walk into deep middles → blocked by navmesh

### 8.6 If a canal needs to be uniformly deep (no walkable edges)

Set `bAutoDetectDeep = false` and `bIsDeep = true` on the AWaterArea actor. This treats the entire footprint as deep — one merged nav-modifier box covers the full extent, no raycast.

---

## 9. 3D asset generation (Hunyuan3D)

### 9.1 Asset gap analysis

**Available** in `3d art/Imported_to_UE5/Final/` — use directly:

| Asset                                                                                                                    | Use in sewer                                    |
| ------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------- |
| `pillar_broken`, `pillar_whole`, `pillar_fluted`, `pillar_carved`                                                        | Sewer support columns at corridor intersections |
| `brazier_iron`                                                                                                           | Light sources                                   |
| `cobwebs_corner`                                                                                                         | Wall corners                                    |
| `bone_pile`                                                                                                              | Refuse heaps                                    |
| `crystal_blue`, `crystal_green`                                                                                          | Optional phosphorescent accent                  |
| `crate_wooden`, `crate_small`, `crate_large`, `barrel_wooden`, `barrel_large`, `barrel_open`, `sack_burlap`, `sack_open` | Smuggler stash                                  |
| `rock_small`, `rock_medium`, `rock_large`, `rocks_cluster`                                                               | Rubble                                          |
| `bridge_wooden`, `bridge_stone_arch`                                                                                     | Bridges over canals                             |
| `stairs_wooden`                                                                                                          | Stairs to surface + descent to F2               |
| `tomb_stone`                                                                                                             | Forgotten crypt sections (sparingly)            |
| `well_stone`                                                                                                             | Larger water features                           |
| `fountain_basin`                                                                                                         | Central canal feature                           |

**Missing** — must generate via Hunyuan3D pipeline (10 assets, ~1 day):

| Priority | Asset name | Purpose |
|---|---|---|
| Critical | `wall_sewer_brick_module` | Modular wall section |
| Critical | `wall_sewer_brick_corner` | 90° wall corner |
| Critical | `floor_sewer_cobble_module` | Floor tile |
| Critical | `pipe_drainage_large` | Wall-protruding stone drainage pipe |
| High | `wall_sewer_brick_window` | Wall variant with vertical slit/grate |
| High | `grate_iron_floor` | Iron grate floor cover |
| High | `arch_sewer_doorway` | Stone archway between rooms |
| Medium | `sconce_wall_torch` | Wall-mounted torch (alt to brazier) |
| Medium | `stairs_stone_wet` | Wet stone steps into water |
| Low | `pipe_wall_run` | Smaller pipes along walls |

See [Appendix B](#appendix-b--hunyuan3d-per-asset-prompts) for full per-asset prompts.

### 9.2 Pipeline (per asset)

Per the canonical procedure in `/sabrimmo-3d-world` skill:

```
1. SDXL multi-input generation (rotated + 3quarter strategies × 2 seeds = 4 candidates)  ~12s × 2 strategies = ~24s gen
2. rembg BiRefNet pre-mask                                                                 ~30s
3. Validate masked input (multi-object check, edge-cutoff, coverage)                       ~1s
4. Hunyuan3D mesh-gen with mc_algo="mc"                                                    ~35s × 4 = ~140s
5. Quality-aware scoring (cube artifacts get 0.001 — always lose)                          ~1s
6. DECIMATE FIRST to 5K faces                                                              ~5s
7. Texture decimated mesh (6-camera bake)                                                  ~40s
8. 3D Coat hand-paint pass (HIGH-FIDELITY GATE)                                            ~1 hour (manual)
9. Re-export GLB → import to UE5
```

Total per asset: **~5 min auto + ~1 hour 3D Coat hand-paint** = ~1.25 hours per asset.

10 assets × 1.25 hours = ~13 hours, spread across 2 days.

### 9.3 Critical pipeline rules

- `mc_algo="mc"` (NOT `"dmc"` — broken on torch 2.12 / sm_120)
- Decimate BEFORE textering (5K faces takes 40s to texture; 1.4M faces takes 25 min)
- Validate masked inputs to catch cube artifacts (multi-object check, edge-cutoff check)
- Use quality-aware scoring (`if volume > 5.0: score = 0.001` to exclude cube artifacts)
- 3D Coat hand-paint pass is REQUIRED for high fidelity — without it, AI assets look photoreal-leaning and clash with the RO style. Per the skill: "After Hunyuan generation + decimation, every asset must pass through a 3D Coat hand-paint pass before UE5 import."

### 9.4 3D Coat hand-paint workflow (per asset, ~1 hour)

1. Import GLB to 3D Coat
2. Bake AO + curvature reference maps
3. Hand-paint base diffuse with stylized smart materials, warm earth tones (RO palette)
4. Export 1K or 2K diffuse PNG (no normal map, no roughness — RO is roughness=0.95 flat)
5. Re-import to Blender, embed diffuse in FBX/GLB
6. UE5: BaseColor = diffuse, Roughness = 0.95 constant, Specular = 0, no normal map

### 9.5 Output paths

| Stage | Path |
|---|---|
| Pipeline output | `3d art/generated_assets/sewer/<asset_name>/06_final.glb` |
| Hand-painted final | `3d art/Imported_to_UE5/Final/sewer/<asset_name>.glb` |
| UE5 import target | `Content/SabriMMO/Meshes/Environment/SewerDungeon/SM_<AssetName>` |

### 9.6 Reusable scripts (call from `_tools/`)

- `_tools/hunyuan_asset_pipeline.py` — master batch pipeline. Add a `SEWER_ASSETS` config with the 10 prompts.
- `_tools/comfyui_supervisor.py` — auto-restart supervisor (always run with this for batch jobs)
- `_tools/hunyuan_cleanup.py` — iterative cleanup pass after the initial gen

---

## 10. Layout sketch & room plan

### 10.1 ASCII layout diagram

```
                    [stairs UP to Prontera Inn]
                          ↑ Z=400 (warp to prt)
              ┌───────────┴──────────────────────────────────┐
              │                                              │
              │   ENTRY ROOM (1500x1500 UU)                  │
              │   ┌──┐  ┌──┐                                 │
              │   │  │  │  │   ░░░░░░░░░░░░░ canal main ░░░░│
              │   │  │  │  │   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
              │   └──┘  └──┘   ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
              │                ▓ stone bridge ▓               │
              │ ┌─ corridor ──────┐  ░░░░░░░░░░░░             │
              │ │                 │  ░░░░░░░░░░░░             │
              │ │  bug_nest       │  ░░░░░░░░░░░░             │
              │ │  [3 thief_bug]  │   canal continues →       │
              │ │  [2 eggs]       │                            │
              │ │                 │                            │
              │ └─────────────────┘                            │
              │                                                │
              │     ┌──────────────────┐                       │
              │     │ CENTRAL HUB      │ ░░ canal junction ░░  │
              │     │ (2000x2000 UU)   │ ░░ E + S branches ░░  │
              │     │ brazier ×4 corners│ ░░ waterfall down ░░ │
              │     │ pillar_carved ×2 │                       │
              │     └──────────────────┘                       │
              │                                                │
              │     ┌── corridor ──┐                           │
              │     │              │                           │
              │     │ TAROU WARREN │   ░░ canal east ░░       │
              │     │ (1200x1200) │   ░░ extentX=800 ░░       │
              │     │ [4 tarou]   │   ░░ extentY=200 ░░       │
              │     │ haystack ×3 │                            │
              │     │ rubble piles│                            │
              │     └─────────────┘                            │
              │                                                │
              │  ┌────────────┐                                │
              │  │            │                                │
              │  │ DEEP ROOM  │ — disused, mossy walls,        │
              │  │ (1800x1800)│   bone_pile, cobwebs,          │
              │  │ [boss?]    │   tomb_stone (forgotten crypt)│
              │  │ crystal ×3 │                                │
              │  │ pillar ×4  │                                │
              │  └────────────┘                                │
              │                                                │
              └──────────── [stairs DOWN to F2] ──────────────┘
                              ↓ Z=-300 (warp to SewerDungeon02 — closed for now)
```

Footprint: ~6500 × 6500 UU. Floor at Z=100. Ceiling at Z=400. Canal water surface at Z=70 (30 UU below floor surface, simulating water that's cut into the floor).

### 10.2 Room dimensions and content

| Room | Dimensions | Lighting | Decoration |
|---|---|---|---|
| Entry room | 1500×1500 UU | 1 brazier on each of 2 internal pillars + 1 ceiling grate spotlight | 2 internal stone pillars (`pillar_whole`), worn floor (path decals), entry stairs warp |
| Bug nest corridor | 400×800 UU | 1 brazier at midpoint | Crates stacked along one wall, 2 thief_bug_egg meshes (use scaled `bone_pile`?), cobwebs corners |
| Central hub | 2000×2000 UU | 4 corner braziers + 2 ceiling grate spotlights + 2 pillar braziers | 4 corner pillars (`pillar_carved`), central canal junction with stone bridge, fountain_basin or well_stone as canal source feature |
| Tarou warren | 1200×1200 UU | 1 brazier + 2 small accent crystal lights | Haystacks, scattered rocks, fence segments separating off areas, sack piles |
| Deep room (forgotten crypt) | 1800×1800 UU | 3 brazier pools dim, 3 crystal accents (cyan glow) | 4 carved pillars, tomb_stone, sarcophagus, cobwebs heavy, bone piles, mossy walls (use `MI_DungeonWall_Mossy`) |
| Connecting corridors | 400 UU wide × variable | 1 brazier per ~600 UU stretch | Drainage pipes weeping water decals, occasional moss decals, slime puddle decals |

### 10.3 Enemy spawn distribution — `spawnPool` (200 total)

The hand-coded `enemySpawns[]` from earlier drafts was replaced with `spawnPool` (random spawn distribution across `SpawnAllowVolume` actors, NavMesh-snapped). Counts per template:

| Template | Count | wanderRadius | Notes |
|---|---:|---:|---|
| `thief_bug` | 50 | 350 | Lvl 6, aggressive — main encounter density |
| `thief_bug_egg` | 120 | 80 | Lvl 4, stationary — clusters in nest rooms (small wander = effectively pinned) |
| `farmiliar` | 15 | 400 | Lvl 8, flying bat (shadow-element) — wider wander for corridors |
| `tarou` | 15 | 350 | Lvl 11, aggressive rat |

**How "evenly across the spawn area" works**: place `SpawnAllowVolume` actors covering the rooms and corridors you want monsters in (e.g., one per room + corridor). Optionally place `SpawnDenyVolume` actors over canal water and unreachable nooks. Run `ExportSpawnRegions SewerDungeon01` in the editor console — it writes `server/spawn_regions/SewerDungeon01.json` containing the merged allow-minus-deny region. The server picks 200 random points inside that region (weighted by per-volume area for even distribution) and snaps each to the nearest navmesh point.

To bias densities (e.g., concentrate eggs in a "nest room"), add a second smaller `SpawnAllowVolume` covering just that room, then re-export — the area-weighted random distribution will drop more spawns there.

### 10.4 Warp portal positions — committed 2026-05-07

| Warp ID | Local coords (in SewerDungeon01) | Destination |
|---|---|---|
| `sewerdungeon01_to_prtsouth` | (3990, -1390, 620) | `prontera_south` at (-650, 10290, 30) |
| `sewerdungeon01_to_sewerdungeon02` (commented out) | (3990, 1350, 290) | `SewerDungeon02` — TBD when F2 ships, players from F2 will spawn at (3680, 1350, 340) |

Reverse warp in `prontera_south.warps[]`:

| Warp ID | Local coords (in prontera_south) | Destination |
|---|---|---|
| `prtsouth_to_sewerdungeon01` (was `prtsouth_to_dungeon`) | (-650, 9560, -20) | `SewerDungeon01` at (3370, -1390, 620) |

Future reverse warp (when F2 ships): `sewerdungeon02_to_sewerdungeon01` from F2's local position to `SewerDungeon01` at (3680, 1350, 340).

**Sync rule**: server validates player proximity to warps within `radius * 2 = 400 UE units`. The `AWarpPortal` actors in `L_SewerDungeon01` and `L_PrtSouth` must be within 400 UU of the registered coords above. Use `unrealMCP find_actors_by_name("WarpPortal")` to verify.

### 10.5 Water canals — placed by user 2026-05-07

The user has placed `AWaterArea` actors directly in `L_SewerDungeon01`. Each actor's `bAutoDetectDeep` raycast handles per-cell deep detection at `OnConstruction` time, spawning `NavArea_Null` boxes that cut the UE5 navmesh. The server-side navmesh validation backstop (`findClosestNavMeshPoint` rejecting >100u off-nav movement) catches any client attempts to enter deep cells.

Because the AWaterArea actors carry their own visual + nav-cut behavior, the `waterAreas[]` array in `ro_zone_data.js` is **not required** — it only adds a server-side AABB fast-path for `isInDeepWater()`, which is redundant with the navmesh check for mixed-mode actors. If you want the redundant safety check, add a `waterAreas[]` entry per AWaterArea actor with matching `id`, `extentX`, `extentY`, position, and `waterLevel` — see Section 8 for the schema.

**Pending**: re-export the navmesh after the user finishes placing AWaterArea actors so the deep-cell cuts make it into `server/navmesh/SewerDungeon01.obj` (see Section 11).

---

## 11. NavMesh export

After the level is greyboxed and water canals placed:

### 11.1 In UE5

1. Build → Build Navigation
2. Press **P** to visualize the green nav overlay
3. Verify:
   - Walkable corridors and rooms have continuous green coverage
   - Water deep cells have HOLES (no green)
   - Bridges over water DO have green (walkable)
4. If holes are missing where they should be:
   - Open the AWaterArea actor → toggle `bAutoDetectDeep` off and on (re-triggers `OnConstruction`)
   - Or nudge `WaterExtent` X by 1 UU to force re-scan
   - Re-run Build Navigation

### 11.2 Export OBJ

Open editor console (backtick `` ` ``):
```
ExportNavMesh SewerDungeon01
```

OBJ writes to `client/server/navmesh/SewerDungeon01.obj` (note: path is off-by-one due to `GetDefaultOutputDirectory` quirk).

### 11.3 Copy to server

```bash
cp client/server/navmesh/SewerDungeon01.obj server/navmesh/SewerDungeon01.obj
```

### 11.4 Delete cached navmesh

```bash
rm -f server/navmesh/.cache/SewerDungeon01.navmesh
```

(Skip if the cache directory is empty or this zone has never been built.)

### 11.5 Restart server

```bash
cd server
npm run dev
```

Startup logs should show:
```
[NavMesh] Building 'SewerDungeon01' from server/navmesh/SewerDungeon01.obj
[NavMesh] Built 'SewerDungeon01' in 412ms (1247 vertices, 2380 triangles)
[NavMesh] Cached to server/navmesh/.cache/SewerDungeon01.navmesh
```

### 11.6 Validate

1. Connect a client and warp into `SewerDungeon01`
2. Click-to-move into a deep canal cell
3. Server should emit `player:position_rejected` with `reason: 'off_navmesh'`
4. Client should snap the player back to the last valid position

If movement doesn't reject, the OBJ may not have the deep-cell holes — re-run Build Navigation in UE5 with the AWaterArea visible, re-export, re-copy, restart.

---

## 12. Phased schedule

### 12.1 Days 1-2 — Server + scaffold (12-16 hours)

- [ ] Add `SewerDungeon01` to `ro_zone_data.js` (1 hour, includes reverse warp in `prontera`)
- [ ] Add `SewerDungeon01` to `ro_world_map_data.js` (`ZONE_INFO` + grid placement) (30 min)
- [ ] Verify no duplicate zone names in grid (5 min, see 3.3)
- [ ] Add `PostProcessSubsystem.cpp` zone preset block (15 min — requires C++ recompile)
- [ ] Add fog density override (5 min)
- [ ] Add `AudioSubsystem.cpp` BGM + ambient mappings (30 min — requires C++ recompile)
- [ ] Compile (~5-10 min via Live Coding or full rebuild)
- [ ] Duplicate `L_PrtDungeon01` → `L_SewerDungeon01` and save (5 min)
- [ ] Verify Level Blueprint, GameMode override, DefaultPawnClass=None (15 min)
- [ ] Delete old PrtDungeon geometry, keep lighting + nav volume placeholders (15 min)
- [ ] Greybox the maze layout (4-8 hours): cubes scaled to corridor widths, room sizes, ceiling slabs

End of day 2: walkable greybox, server registers zone, client can warp into the empty maze.

### 12.2 Days 3-5 — Asset generation (24 hours, mostly overnight)

- [ ] Add `SEWER_ASSETS` config to `_tools/hunyuan_asset_pipeline.py` with 10 prompts (1 hour)
- [ ] Run pipeline overnight under `comfyui_supervisor.py` (~1 hour gen × 10 + cleanup retries = ~13 hours, parallelized)
- [ ] Quality cleanup pass next morning (1 hour)
- [ ] 3D Coat hand-paint pass on each asset (~1 hour each × 10 = 10 hours, spread across 1-2 days)
- [ ] Re-export GLBs → import to UE5 (~30 min)
- [ ] Run `create_sewer_dungeon_mis.py` to create wet-floor and mossy-wall MIs (5 min)
- [ ] Run `create_sewer_decal_instances.py` to create slime decal MIs (5 min)
- [ ] Verify all assets in Content Browser (15 min)

End of day 5: all 10 missing assets imported and hand-painted, MIs ready, can start dressing the level.

### 12.3 Days 6-9 — Build the level (24-32 hours)

- [ ] Replace greybox walls with `wall_sewer_brick_module` + corners (6 hours)
- [ ] Replace greybox floors with `floor_sewer_cobble_module` (4 hours)
- [ ] Apply `MI_DungeonWall_Mossy` to deep-room walls (15 min)
- [ ] Apply `MI_DungeonFloor_Wet` to canal-adjacent floor tiles (1 hour)
- [ ] Place `bridge_stone_arch` over canals (1 hour)
- [ ] Place `stairs_wooden` at entry + `stairs_stone_wet` at descent points (30 min)
- [ ] Place `arch_sewer_doorway` at room transitions (1 hour)
- [ ] Place `pipe_drainage_large` along walls (with `M_Decal_DarkStain` underneath each — water trail) (2 hours)
- [ ] Place `pillar_carved`, `pillar_whole`, `pillar_broken`, `pillar_fluted` per room plan (2 hours)
- [ ] Place AWaterArea actors at canal positions, tune palette per Section 8 (1 hour)
- [ ] Build Navigation → verify deep cell cuts (15 min)
- [ ] Place AWarpPortals at entry stairs + descent, sync coords to server (30 min)
- [ ] Place all decals (~70 total, see Section 5.3) (3 hours)
- [ ] Place props (crates, barrels, sacks, baskets, bones, cobwebs, haystacks, fences) (4 hours)
- [ ] Place `brazier_iron` meshes at light positions (30 min)
- [ ] Place Point Lights on every brazier with correct settings (1 hour)
- [ ] Place ceiling grates (use `grate_iron_floor` flipped) + Spotlights (1 hour)
- [ ] Optional: Place `crystal_blue/green` clusters in deep room with accent Point Lights (30 min)

End of day 9: visually complete level, atmospheric, all rooms dressed, lighting placed.

### 12.4 Days 10-12 — Polish (16-24 hours)

- [ ] Side-by-side screenshot comparison vs RO Classic prt_sewb1 references (2 hours)
- [ ] Adjust decal density/placement based on what looks busy vs sparse (3-4 hours)
- [ ] Fine-tune Point Light intensity + radius per room (1-2 hours)
- [ ] Adjust fog density / inscatter to match references (30 min, may require recompile if changing in C++)
- [ ] Adjust AWaterArea depth gradient to match RO sewage look (30 min)
- [ ] Per-camera decal pass — walk the level from player POV, add/remove decals where the eye lingers (2-4 hours)
- [ ] Place final enemy spawn coordinates in `enemySpawns[]` based on actual room positions (1 hour)
- [ ] Build Navigation → ExportNavMesh → copy to `server/navmesh/` → delete cache → restart server (30 min)
- [ ] Test matrix pass (4-6 hours, see Section 13)
- [ ] Bug fixes from test (2-4 hours)

End of day 12: zone is ship-ready.

### 12.5 Days 13-14 — Buffer

Reserved for:
- Re-rolls of any Hunyuan3D assets that didn't survive 3D Coat
- Regression fixes if combat/spawn/water-skill tests fail
- Loading screen art generation if you want a sewer-themed `T_Loading_NN` for the next slot
- World-map dot placement tuning
- Optional bonus polish

---

## 13. Test matrix

### 13.1 Smoke tests

- [ ] Server starts without errors when `SewerDungeon01` is added to registry
- [ ] Server logs show `[NavMesh] Built 'SewerDungeon01'` on startup
- [ ] Client builds without errors after C++ changes (PostProcess + Audio)
- [ ] `L_SewerDungeon01.umap` opens in editor without missing-asset warnings

### 13.2 Zone transition

- [ ] Login → walk into Prontera → walk to `prt_to_sewerdungeon01` warp portal at Inn area
- [ ] Loading screen displays
- [ ] `L_SewerDungeon01` loads
- [ ] Player spawns at `sewerdungeon01_to_prt` local coords (top of stairs)
- [ ] No duplicate character at world origin (verify `DefaultPawnClass = None`)
- [ ] Walk back through warp → returns to Prontera at `prt_to_sewerdungeon01` destination coords

### 13.3 Lighting / visual

- [ ] Heavy fog visible, mossy-green tint
- [ ] Vignette heavy at screen edges
- [ ] Brazier Point Lights cast warm orange pools, ~400 UU radius
- [ ] Ceiling grate Spotlights cast cool blue light shafts
- [ ] Walls show brick + moss texture (not default checkerboard)
- [ ] Floor shows cobble texture, wet variant near canals
- [ ] Decals render with soft edges (no rectangles), organic placement

### 13.4 Water canals

- [ ] AWaterArea visual plane visible, murky green sewage palette
- [ ] Walking on shallow shore is allowed
- [ ] Walking onto bridges crosses canals
- [ ] Click-to-move into deep canal middle is rejected — server emits `player:position_rejected` with `reason: 'off_navmesh'`
- [ ] WASD into deep canal middle is rejected too
- [ ] `water:enter` socket event fires when stepping into shore
- [ ] `water:exit` socket event fires when stepping out
- [ ] Aqua Benedicta (skill 413) usable while in shallow water; rejected when out of water with `Must be standing in water`
- [ ] Water Ball (skill 412) same gating

### 13.5 Combat / enemies

- [ ] All 4 templates spawn (server log shows `[SPAWN] SewerDungeon01 thief_bug ×50, thief_bug_egg ×120, farmiliar ×15, tarou ×15` or similar) — pending `ExportSpawnRegions SewerDungeon01` first
- [ ] Thief Bugs (×50) wander within their 350 UU radius
- [ ] Thief Bug Eggs (×120) are effectively stationary (80 UU radius)
- [ ] Familiars (×15) fly through corridors and aggro fast
- [ ] Tarous (×15) territorial in their cluster areas
- [ ] No spawn warnings in server log (`[ENEMY] Spawn '...' snapped Xu to navmesh ...`) — if any appear, the SpawnAllowVolume overlaps deep water or unwalkable area; tighten the volume or add a SpawnDenyVolume
- [ ] Enemies pathfind around pillars and water canals (NavMesh OBJ correctly cut by AWaterArea actors)
- [ ] Combat works (basic attack, skills land, enemies die, drop items)

### 13.6 Audio

- [ ] BGM `bgm_19 "Under the Ground"` starts on entry, loops, fades out on warp
- [ ] Ambient layer 1 (dungeon wind) audible, looping
- [ ] Ambient layer 2 (water drips) audible, looping
- [ ] Ambient layer 3 (waterfall) audible near canal junctions
- [ ] 3D positional water gurgle louder near canals (if added)
- [ ] Brazier crackle audible near each brazier (if added)
- [ ] Player sounds work (footsteps if added, hit/swing audio)
- [ ] Skill SFX work (Aqua Benedicta cast sound, etc.)

### 13.7 Map system

- [ ] Minimap renders the dungeon overhead correctly (no black square — verify `SetPostProcessMaterial(false)` on minimap SceneCapture is intact)
- [ ] Player position dot moves on minimap
- [ ] Enemy dots show on minimap
- [ ] World map (M key) shows `SewerDungeon01` cell with red dungeon tint
- [ ] Hover tooltip shows "Sewer Dungeon F1 — Dungeon — Lv 15-25 — Thief Bug, Thief Bug Female, ..."

### 13.8 ESC menu / save / death

- [ ] Try Kafra Save (Butterfly Wing or via dialog if added) → rejected with `nosave` flag
- [ ] Use Fly Wing (item 1029) → random teleport within zone (allowed since `noteleport: false`)
- [ ] Use Butterfly Wing (item 1028) → teleports to save point (last town save, e.g. Prontera)
- [ ] Die in zone → respawns at save point (Prontera)
- [ ] Disconnect mid-zone → reconnect → spawns at last position (within sewer)

### 13.9 Multiplayer

- [ ] Two PIE clients in `SewerDungeon01` → see each other's sprites
- [ ] One client in `SewerDungeon01`, one in `prontera` → cannot see each other (zone-scoped broadcasting)
- [ ] Party member dot shows on world map for cross-zone party member
- [ ] Combat events broadcast correctly (damage numbers visible to all in same zone)

---

## 14. Risk register

| Risk | Mitigation |
|---|---|
| Level Blueprint missing after duplicate | Re-duplicate from `L_PrtDungeon01`, never create from scratch. Verify the 3 Level BP sections before proceeding. |
| `DefaultPawnClass` set somewhere → duplicate character | Verify `GM_MMOGameMode` Class Defaults explicitly set to None. |
| Warp position desync (server xyz vs UE actor location) | Use `unrealMCP find_actors_by_name("WarpPortal")` to verify actor positions match server `x, y, z` within 400 UE. |
| AWaterArea raycast misses floor → all cells deep | Verify floor mesh is below AWaterArea Z position. Show Collision in editor to confirm. Check the depth-scan log output at OnConstruction. |
| Hunyuan3D asset is cube artifact | Quality-aware scoring rejects cubes (`volume > 5.0` → score 0.001). If still bad, reroll with new SDXL seed (8 attempts max via cleanup loop). |
| 3D Coat hand-paint takes too long | High-fidelity bar requires it. If timeline slips, drop to mid-fidelity (skip 3D Coat, use Hunyuan-textured assets directly) for first ship, polish later. |
| NavMesh OBJ doesn't reflect water cuts | Re-Build Navigation in UE5 with AWaterArea visible. Re-export. Delete `.cache/SewerDungeon01.navmesh`. Restart server. |
| Server can't find `SewerDungeon02` for descent warp | Comment out the F2 warp until F2 ships. Place a closed-gate static mesh at the descent location instead of an AWarpPortal. |
| Decals look like rectangles | Use ONLY `MI_RODecal_*` (RO original textures), tint stays warm brown, opacity stays 0.35. Never use AI-generated decals. |
| Cel-shading gets enabled by accident | The visual-style research file is explicit: do NOT add cel-shade or outlines. They were tested and washed out the scene. |
| Minimap goes black | Verify `SetPostProcessMaterial(false)` is still in `MinimapSubsystem::SetupOverheadCapture()`. Without it, the global cutout PP darkens the entire capture. |
| `mc_algo="dmc"` accidentally used in Hunyuan | Pipeline crashes with `'NoneType' object has no attribute 'mesh_f'`. Always `mc_algo="mc"` on torch 2.12 / sm_120. |
| Texturing 1.4M-face mesh takes 25+ minutes | DECIMATE FIRST to 5K faces (40s texture). The skill is explicit on this. |
| `UClass*` member without UPROPERTY | If any new C++ subsystems are added during this work, mark all runtime-loaded class refs with `UPROPERTY()` or GC will collect them and crash. |
| Sewer feels too dark | Increase Point Light intensity from 800 to 1200, add 1-2 ceiling grate Spotlights, lower fog density from 0.05 to 0.04. |
| Sewer feels too bright | Reduce ceiling Spotlight intensity, lower SkyLight to 0.15, raise fog density to 0.06. |
| Performance drops in dense rooms | Light count budget is 15-25 per zone. Reduce Spotlights or add aggressive cull distances. Disable shadows on accent crystal Point Lights (intensity 200). |

---

## Appendix A — Existing imported asset inventory

From `3d art/Imported_to_UE5/Final/`:

### Vegetation (12)
`tree_oak`, `tree_oak_old`, `tree_oak_dense`, `tree_oak_yellow`, `tree_oak_lush`, `tree_dead`, `tree_dead_white`, `tree_willow`, `tree_sakura_pink`, `tree_sakura_full`, `tree_bamboo`, `bush_round`, `bush_wide`, `bush_sparse`, `mushroom_giant`, `mushroom_brown`, `mushroom_blue`, `reeds_cattails`

### Architecture (15)
**Prontera:** `house_prontera_small`, `house_prontera_medium`, `house_prontera_tall`, `house_prontera_wide`, `house_prontera_corner`
**Payon:** `hut_payon`, `hut_payon_long`, `hut_payon_small`
**Morroc:** `house_morroc`, `house_morroc_arch`, `house_morroc_dome`, `tent_morroc`
**Other:** `house_alberta_wood`, `dock_wooden`, `tower_geffen`, `tower_gate`, `wall_stone_battlement`, `shrine_small`

### Town props (~25)
Barrels (wooden, large, open), crates (small, large, wooden), sacks (burlap, open), baskets (woven, apple), market stalls (red, blue, awning_market, awning_striped), lampposts (`lamppost_amatsu`), banners (horizontal), signs (planted), tables (wooden), benches (wooden), haystacks, fences (wooden, long, stone), carts (wooden, kafra, merchant)

### Terrain (12)
Rocks (small, medium, large, mossy_large, split, cluster), `boulder_huge`, `cliff_face_low`, bridges (wooden, stone arch), `stairs_wooden`, `path_stone`, wells (wooden, stone), fountains (basin, stone), `statue_pedestal`

### Centerpieces (3)
`fountain_stone`, `statue_warrior`, `well_wooden`

### Dungeon (13)
Pillars (broken, whole, fluted, carved), tombs (stone, cross), coffins (wooden, stone), `sarcophagus`, `brazier_iron`, crystals (blue, purple, green), `cobwebs_corner`, `bone_pile`

### Special (4)
`anvil_blacksmith`, `table_alchemy`, `kafra_pad`, `emperium_pedestal`

---

## Appendix B — Hunyuan3D per-asset prompts

### B.1 `wall_sewer_brick_module` (CRITICAL)

**Subject:** "wall section of old brown brick with mossy green grime"

**Rotated strategy prompt:**
```
ONE single wall section of old brown brick with mossy green grime, slightly rotated to show depth, looking down from 30 degrees above, you can see the top curving away from the viewer, voxel game asset 3D rendered, octane render with ambient occlusion, isolated on plain white background, centered, low-poly stylized fantasy game asset, Ragnarok Online style, hand-painted warm earth tones, no shadow, fully contained inside the frame with clear empty white space around all edges
```

**3quarter strategy prompt:**
```
ONE single wall section of old brown brick with mossy green grime, viewed from a 3/4 perspective angle, dramatic side lighting from upper left, strong shadow on opposite side showing volume, you can see both the front and the side of the object, full 3D shape with clear depth, sculpted form, isolated on plain white background, centered, low-poly stylized fantasy game asset, photogrammetry render, Ragnarok Online style, hand-painted warm earth tones, no shadow on ground, small enough to leave at least 10 percent margin around all sides
```

**Negative prompt (universal, used for all assets):**
```
front view, side view, profile, orthographic, flat, 2D illustration, 2D drawing, sketch, painting, multiple objects, several, many, tiling, repeating, grid, collage, panel, frame, border, photorealistic, photograph, perspective lines, horizon, scene, foreground objects, background scenery, ground, floor, surface beneath, cut off, clipped, cropped, extends beyond frame, touches edges, fills entire frame, no margin, no padding
```

**Target:** modular wall section, ~400 UU wide × ~100 UU thick × ~400 UU tall after import. Should tile horizontally (left edge meshes with right edge of next instance).

**Decimation target:** 5000 faces.

**3D Coat hand-paint focus:** Warm brown base brick (HSV 30°, S 35%, V 45%), green moss highlights at brick edges and mortar lines (HSV 100°, S 45%, V 35%), darker stains in lower third where water has reached.

### B.2 `wall_sewer_brick_corner` (CRITICAL)

**Subject:** "90 degree wall corner of old brown brick with mossy green grime, two faces meeting"

Same prompt template as B.1, with subject substituted. Target: same dimensions as B.1 but L-shaped footprint.

### B.3 `floor_sewer_cobble_module` (CRITICAL)

**Subject:** "dark wet cobblestone floor tile with mossy edges, square shape"

Same prompt template. Target: ~400×400 UU floor tile, ~30 UU thick. Should tile in a 4-way pattern.

**3D Coat focus:** Wet stone look (slightly glossy in 3D Coat, but final material is roughness 0.95 — gloss comes from the wet MI variant), darker mortar gaps, occasional moss patches at tile edges.

### B.4 `pipe_drainage_large` (CRITICAL)

**Subject:** "large stone drainage pipe protruding from a wall, with dripping moss, cylindrical opening"

**Target:** ~150 UU diameter pipe, ~300 UU long, attaches to a wall surface.

**3D Coat focus:** Mossy stone exterior, dark interior (suggests depth into wall), water stain trail running down from the lower lip.

### B.5 `wall_sewer_brick_window` (HIGH)

**Subject:** "old brown brick wall section with vertical iron-grate window, mossy"

**Target:** same dimensions as B.1, with a 50×200 UU vertical slit window covered by 4 iron bars.

### B.6 `grate_iron_floor` (HIGH)

**Subject:** "rusted iron floor grate with square holes, sewer drain cover, top-down view"

**Target:** ~300×300 UU square, ~10 UU thick. Sits flush in the floor or on the ceiling (used both ways).

### B.7 `arch_sewer_doorway` (HIGH)

**Subject:** "stone archway doorway with keystone, mossy bricks, semi-circular top"

**Target:** ~400 UU wide × ~50 UU thick × ~500 UU tall. Spans between two `wall_sewer_brick_module` segments.

### B.8 `sconce_wall_torch` (MEDIUM)

**Subject:** "iron wall sconce with flame holder, fantasy medieval, attached to wall"

**Target:** ~80 UU wide × ~30 UU deep × ~120 UU tall. Place above eye level on wall sections, alternates with `brazier_iron` for variety.

### B.9 `stairs_stone_wet` (MEDIUM)

**Subject:** "stone steps descending into shallow water, wet and mossy, three steps"

**Target:** ~400 UU wide × ~600 UU deep × ~300 UU tall. Used at canal entry points where the floor steps down into the water.

### B.10 `pipe_wall_run` (LOW)

**Subject:** "small clay pipes running horizontally along a stone wall, drainage system, multiple pipe segments"

**Target:** ~400 UU long × ~30 UU diameter. Decorative — runs along walls at various heights to break up flat surfaces.

---

## Appendix C — Decal type → density formula

For a 6500×6500 UU sewer with ~5 rooms + connecting corridors, the high-fidelity decal budget is **70-80 visible decals**. Distribution per room:

```
Room footprint (UU²) ÷ 80,000 = base decal count for that room
```

Multiplied by:
- **Mood multiplier**: 1.5× for deep room (mossy), 1.2× for tarou warren (refuse), 1.0× standard, 0.8× for entry room (cleaner)
- **Decal type ratio per room**:
  - Entry room: 50% path/dirt, 30% slime, 15% moss, 5% cracks
  - Bug nest corridor: 40% slime, 30% dirt, 15% cracks, 15% moss
  - Central hub: 35% slime, 25% path, 25% moss, 15% cracks
  - Tarou warren: 30% dirt, 25% slime, 25% cracks, 20% moss
  - Deep room: 40% moss, 25% cracks, 20% slime, 15% dark stains (blood)
  - Corridors: 50% slime (water trails from pipes), 25% moss, 15% cracks, 10% dirt

Example:
- Deep room: 1800×1800 = 3,240,000 UU² ÷ 80,000 = 40 decals × 1.5 mood = **60 decals** in deep room alone
- Wait, that's too many. Cap at 25-30 per room.

**Practical cap per room: 25 decals max**, with smaller rooms getting fewer. Total target is 70-80 across the whole zone.

Per `_meta/04_Decals_Guide.md` and `feedback-decal-textures`:
- Place at floor surface + slight Z offset
- Pitch −90° (downward)
- Random Yaw 0-360°
- Scale 1.5-5.0
- Opacity stays 0.35 (default)
- Slight overlap = natural
- Mix 3+ types per area

---

## Appendix D — Point Light placement guide

**Total light budget: 15-25 lights** for the zone.

| Room | Brazier Point Lights | Ceiling Grate Spotlights | Crystal Accent Point Lights | Subtotal |
|---|---|---|---|---|
| Entry room | 2 (on internal pillars) | 1 | 0 | 3 |
| Bug nest corridor | 1 | 0 | 0 | 1 |
| Central hub | 4 (corners) + 2 (pillars) | 2 | 0 | 8 |
| Tarou warren | 1 | 0 | 0 | 1 |
| Deep room | 3 (sparse) | 0 | 3 (cyan crystals) | 6 |
| Corridors | ~3 (one per ~600 UU stretch) | 0 | 0 | 3 |
| **Total** | **14** | **3** | **3** | **22** |

Settings (repeated from Section 6.4):

| Light type | Color | Intensity | Radius | Cone | Shadows |
|---|---|---|---|---|---|
| Brazier Point | (1.0, 0.55, 0.20) warm orange | 800 | 400 UU | n/a | Yes |
| Ceiling Spotlight | (0.5, 0.7, 1.0) cool blue | 1500 | 800 UU | inner 25° / outer 35° | Yes |
| Crystal Accent | (0.4, 0.7, 0.9) cyan | 200 | 250 UU | n/a | **No** (perf) |

**Light tuning iteration order:**
1. Place all lights at default values
2. Walk the level, identify dark spots → bump nearest brazier intensity by 200
3. Identify bright spots → reduce intensity by 200 or pull radius in
4. Confirm fog density still feels heavy after lighting tune (light + thick fog = haze, but light pools should still read as pools)

---

## Appendix E — Cross-references

### Skills (load these when working on this zone)
- `/sabrimmo-zone` — server registry, warps, level setup
- `/sabrimmo-3d-world` — Hunyuan3D pipeline, post-process, lighting
- `/sabrimmo-water` — AWaterArea actor, depth gradient, navmesh interaction
- `/sabrimmo-navmesh` — pathfinding export, OBJ format, server validation
- `/sabrimmo-material-decals` — decal materials, placement, RO original textures rule
- `/sabrimmo-map` — minimap, world map, loading screen
- `/sabrimmo-audio-player` — BGM zone mapping, ambient layers, 3D positional sources
- `/sabrimmo-persistent-socket` — subsystem registration pattern
- `/sabrimmo-npcs` — if NPCs are added to F2/F3

### Documentation
- `_meta/01_Landscape_Guide.md` — landscape rules (read to understand WHY we're not using one here)
- `_meta/02_Materials_Guide.md` — material instance system
- `_meta/04_Decals_Guide.md` — decal placement
- `_meta/05_Lighting_PostProcess_Guide.md` — per-zone presets
- `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` — manual editor steps
- `docsNew/05_Development/RO_Classic_Visual_Style_Research.md` — what NOT to do (no cel-shading, no outlines)
- `docsNew/05_Development/Water_System_Research.md` — water design rationale
- `3d art/3D_World_Asset_Pipeline_2026.md` — Hunyuan3D pipeline reference

### Memory entries (in `C:/Users/pladr/.claude/projects/C--Sabri-MMO/memory/`)
- `water-system-mixed-mode.md` — AWaterArea single-actor mixed-mode design
- `ground-texture-system.md` — texture pipeline
- `ro-visual-style-research.md` — RO Classic rendering research
- `feedback-decal-textures.md` — RO original textures only for decals
- `feedback-material-versioning.md` — increment variant numbers
- `feedback-scenecapture-postprocess.md` — minimap PostProcessMaterial fix

### Server files
- `server/src/ro_zone_data.js` — zone registry
- `server/src/ro_world_map_data.js` — world map grid
- `server/src/ro_npc_data.js` — NPC registry (deferred for F1)
- `server/src/ro_navmesh.js` — navmesh module

### Client C++ files
- `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp` — zone preset add
- `client/SabriMMO/Source/SabriMMO/Audio/AudioSubsystem.cpp` — BGM + ambient mappings
- `client/SabriMMO/Source/SabriMMO/WaterArea.cpp` — AWaterArea actor (no edits needed, just placement)
- `client/SabriMMO/Source/SabriMMO/MMONPC.cpp` — generic NPC actor (if NPCs added later)

### Scripts to author
- `client/SabriMMO/Scripts/Environment/create_sewer_dungeon_mis.py` — wet floor + mossy wall MIs (Section 5.4)
- `client/SabriMMO/Scripts/Environment/create_sewer_decal_instances.py` — slime decal MIs (Section 5.5)
- `_tools/hunyuan_asset_pipeline.py` — add `SEWER_ASSETS` config (Section 9, Appendix B)

---

## Glossary

| Term | Definition |
|---|---|
| **AWaterArea** | C++ actor that renders water visually + raycasts depth + spawns nav-modifier boxes that cut UE5 navmesh + emits `water:enter`/`water:exit` socket events. |
| **DBuffer Decal** | UE5 decal type that writes to the deferred buffer — cheaper than standard deferred decals. Required to be enabled in Project Settings → Rendering → Lighting. |
| **GrassType (V3)** | UE5 `LandscapeGrassType` asset that drives auto-scattering of small meshes via the Landscape's Grass Output material node. Not used for sewer (no Landscape). |
| **Hunyuan3D** | AI mesh generator we use for new 3D assets (replaces TRELLIS.2). Pipeline: SDXL prompt → mask → Hunyuan mesh-gen → decimate → texture → 3D Coat hand-paint. |
| **Material Instance (MI)** | A child of a master Material that overrides parameters (textures, colors, scalars) without rebuilding the node graph. We use these to make zone-specific looks. |
| **mc_algo** | Hunyuan3D's mesh generation algorithm. Use `"mc"` (marching cubes); `"dmc"` is broken on torch 2.12 / sm_120. |
| **mixed-mode** | AWaterArea's per-cell deep detection — raycasts a 16×16 grid downward, marks each cell deep/shallow individually, merges deep cells into rectangles for nav cuts. |
| **NavMesh OBJ** | Wavefront OBJ file that the server loads at startup to build a navmesh for pathfinding. Exported from UE5 via `ExportNavMesh <zone>` console command. |
| **PostProcessSubsystem** | C++ UWorldSubsystem that auto-spawns Directional/Sky lights, Height Fog, and Post Process Volume per zone. Configured per zone via `ApplyZonePreset` and `SetupSceneLighting`. |
| **Sewer brick module** | Modular static mesh (~400×100×400 UU) that tiles horizontally to form wall sections. Generated via Hunyuan3D, hand-painted in 3D Coat. |
| **`spawnedZones` Set** | Server-side set tracking which zones have had their initial enemy spawn pass. First player to enter a zone triggers the spawn, subsequent players see the same enemy state. |
| **Zone preset** | Per-zone block in `PostProcessSubsystem.cpp::ApplyZonePreset` setting Bloom, Vignette, ExposureBias, WhiteTemp, GainHighlights. Required for new zones. |

---

**Last updated**: 2026-05-07
**Status**: Planning — ready to begin Phase 12.1 (server + scaffold).
