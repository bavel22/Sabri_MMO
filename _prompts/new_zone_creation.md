# New Zone Creation — Session Bootstrap Prompt

A self-contained starting prompt for a fresh Claude Code session that builds a new zone using one of two canonical templates:

- **`SewerDungeon01` (Prontera Culvert F1)** — indoor dungeon archetype. Static-mesh modular tiles, AWaterArea canals, no Landscape Actor.
- **`Pryth` (Prontera-style paved capital town)** — outdoor town archetype. Landscape Actor + `M_Landscape_RO_18` paint-driven pavements (Brick/Cobble/Pebble), single perimeter moat, NPC suite. Use this template when the new zone is a paved town.

Pick whichever template matches the new zone's archetype.

## TL;DR

1. Skim the **Starter prompt** block below
2. Fill in the blanks (zone name, RO reference, archetype, fidelity, water/NPC notes)
3. Paste into a new Claude Code session — Claude will read this file in full, load skills, read referenced docs, and guide the build phase by phase

---

## Starter prompt — copy, fill blanks, paste

```
I'm building a new zone for Sabri_MMO. Read `_prompts/new_zone_creation.md` for the
full setup procedure, then before doing anything else: load every skill listed in the
"Skills to load" section that applies to my zone's archetype, read every doc listed
in "Documentation to read", and read the canonical template at
`docsNew/05_Development/Sewer_Dungeon_01_Build_Plan.md` end-to-end.

Zone parameters:
- Zone name (server, used as ZONE_REGISTRY key):  Pryth
- Display name (UI string):                       Pryth
- Level asset name (UE5 .umap, prefix "L_"):      L_Pryth
- RO Classic reference (mp3nametable id):         Prontera
- Archetype:  town - capital
- Fidelity bar: high (1-2 weeks, screenshot-match) }
- Has water?  canals 
- Has NPCs?   Kafra, shop, quest
- Connects from / to:  prt_south and GrassField05 (which existing zones warp here, and where this zone warps to)
- Player level range:  none - this is a non combat zone/town
- Special features:  none

After loading context:
1. Confirm what you loaded back to me in one short summary
2. Ask any clarifying questions BEFORE writing code or files
3. Once aligned, generate a build plan doc at
   `docsNew/05_Development/<MyZone>_Build_Plan.md` modelled on the SewerDungeon01 plan,
   with my zone-specific decisions baked in (palette, GrassType, MIs, decals, BGM,
   ambient, props from the imported asset library, lighting preset, layout sketch)
4. Then walk me through Phase 12.1 (server + scaffold) one step at a time

Don't start editing files until you've confirmed the plan with me.
```

---

## What Claude should do on receipt

The starter prompt above tells future-you to do these steps. Follow them in order.

### Step 1 — Read this entire file first

You're reading it now. Read every section through "Per-archetype style sheets" so you have the full mental model before loading skills.

### Step 2 — Load all relevant skills (parallel `Skill` tool calls)

Use the **Skills to load** matrix below, keyed on the archetype the user picked. Load **more** rather than fewer — false-positive skill loads cost only context, false-negative loads cost correctness (per project standing rule in `CLAUDE.md`).

### Step 3 — Read all relevant docs (parallel `Read` tool calls)

Use the **Documentation to read** matrix. Always include:
- `docsNew/05_Development/Sewer_Dungeon_01_Build_Plan.md` (the template)
- `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` (manual UE5 editor steps)
- Archetype-specific `_meta/` guides

### Step 4 — Read the project journal for current state

```
Read _journal/Dashboard.md
Read _journal/Session Tracker.md  (last few session entries)
```

This catches any system changes since the SewerDungeon01 plan was written (new mechanics, deprecated APIs, fresh asset library entries).

### Step 5 — Brief the user on what you loaded

In ≤ 5 lines. Mention:
- Skill count loaded
- Key docs read
- Anything in the journal that materially changes the plan vs the SewerDungeon01 template
- Whether the imported asset library has changed since 2026-05-07 (run a `Glob` on `3d art/Imported_to_UE5/Final/**/*.glb` to inventory)

### Step 6 — Ask clarifying questions

Use `AskUserQuestion` with 2-4 questions max. Common ones:
- Specific monsters they want spawned (or trust the archetype defaults)
- Whether to reuse existing imported assets or generate new via Hunyuan3D
- Any specific RO Classic visual reference they want screenshot-matched
- Whether neighbouring zones (warp destinations) need updates too

### Step 7 — Generate the per-zone build plan

Write to `docsNew/05_Development/<ZoneName>_Build_Plan.md`. Mirror the SewerDungeon01 plan's structure exactly:

1. Overview
2. Architecture decisions
3. Server changes (with the actual JS code blocks)
4. UE5 level setup
5. Materials & decals
6. Lighting & post-process
7. Audio
8. Water (if applicable)
9. 3D asset generation (only if user chose mid/high fidelity AND library doesn't have what's needed)
10. Layout sketch & room plan
11. NavMesh export
12. Phased schedule
13. Test matrix
14. Risk register
15+. Appendices (asset inventory, decal density formula, light placement guide, glossary)

### Step 8 — Walk through Phase 12.1 step by step

After user approves the plan, start executing. Phase 12.1 (server + scaffold) typically:
1. `ro_zone_data.js` entry
2. `ro_world_map_data.js` entry + grid placement
3. `PostProcessSubsystem.cpp` zone preset block
4. `AudioSubsystem.cpp` BGM + ambient mappings
   - **Convention** (added 2026-05-10): if the new zone is a Sabri-renamed RO Classic field (e.g. `grassfield05` ↔ `prt_fild05`, `prontera_south` ↔ `prt_fild08`), it MUST inherit the original prt_fild##'s BGM **and** ambient stack. Add it under the **"Sabri MMO renamed prt_fild zones"** section in BOTH `BuildBgmZoneMap()` and `BuildAmbientZoneMap()` with `// mirrors prt_fildXX` comments. See `audio-grassfield-prtfild-convention.md` memory and `RO_Audio_System_Research.md` for the canonical RO Classic mp3nametable mappings.
   - Without an explicit BGM mapping, an unmapped zone silently inherits the previous zone's BGM (`PlayZoneBgm` line ~1675), which makes neighboring zones sound identical.
   - Without an explicit ambient mapping, ambient goes silent (`StopAllAmbient` runs at the start of every zone change, line ~1559) — different from BGM behavior. A field where wind/birds vanish = missing `ZoneAmbientMap` entry.
5. Duplicate an existing working level → save as `L_<MyZone>`
6. Verify Level Blueprint, GameMode, DefaultPawnClass=None
7. Compile and test transition

---

## Skills to load (by archetype)

**Always load** (every zone, regardless of type):

```
sabrimmo-zone           ← server registry, warps, level setup, transitions
sabrimmo-3d-world       ← post-process, lighting, RO Classic visual style, Hunyuan3D pipeline
sabrimmo-navmesh        ← pathfinding export, OBJ format, server validation
sabrimmo-audio-player   ← BGM zone mapping, ambient layers, 3D positional sources
sabrimmo-map            ← minimap, world map, loading screen
sabrimmo-persistent-socket  ← subsystem registration pattern (only if adding a new C++ subsystem)
sabrimmo-build-compile  ← editor target build, Live Coding rules, MSVC linker errors
```

**Outdoor zones** (town, field, forest, desert, snow, beach):

```
+ sabrimmo-landscape           ← UE5 Landscape Actor, sculpting, slope rules
+ sabrimmo-ground-textures     ← M_Landscape_RO_17 master, MI variants, texture pipeline
+ sabrimmo-environment-grass   ← Landscape Grass V3, GrassType assets
+ sabrimmo-material-decals     ← DBuffer decals, RO original textures
```

**Indoor / dungeon / cave zones**:

```
+ sabrimmo-material-decals     ← DBuffer decals (still apply on static mesh floors)
```

(Skip landscape + grass — these zones use static-mesh modular tiles, not a Landscape Actor.)

**If the zone has water** (canals, ponds, sea edges, lake interior, sewers):

```
+ sabrimmo-water  ← AWaterArea actor, mixed-mode deep detection, navmesh cuts, skill gating
```

**If the zone has NPCs** (Kafra, shops, quest givers, refiners, guides):

```
+ sabrimmo-npcs               ← AMMONPC, dialogue trees, shops, quests, Kafra
+ sabrimmo-click-interact     ← click detection, line trace, NPC hit resolution
```

**If the zone has unique sprite-rendered enemies not used elsewhere** (rare):

```
+ sabrimmo-enemy             ← enemy spawn, click-target, death/respawn
+ sabrimmo-sprites           ← runtime sprite system
+ sabrimmo-3d-to-2d          ← only if generating new monster sprites
```

**If the user wants high-fidelity custom 3D assets** (mid or high fidelity):

```
+ sabrimmo-art               ← material specs, hand-paint pass, RO Classic look
+ sabrimmo-3d-world          ← Hunyuan3D pipeline (already loaded above, just verify)
```

---

## Documentation to read (by archetype)

**Always read**:

```
docsNew/05_Development/Zone_System_UE5_Setup_Guide.md     ← manual UE5 editor steps
_journal/Dashboard.md                                      ← current project state
```

**Pick the right template for the archetype**:

```
docsNew/05_Development/Sewer_Dungeon_01_Build_Plan.md     ← INDOOR/DUNGEON template
docsNew/05_Development/Pryth_Build_Plan.md                ← OUTDOOR PAVED TOWN template
```

Read the matching one end-to-end. Both files are 60-80 KB and mirror the same 20-section structure.

**Outdoor zones**:

```
_meta/01_Landscape_Guide.md
_meta/02_Materials_Guide.md
_meta/03_Scatter_Objects_Guide.md
_meta/04_Decals_Guide.md
_meta/05_Lighting_PostProcess_Guide.md
docsNew/05_Development/Ground_Texture_System_Research.md  (skim — pipeline reference)
```

**Indoor zones**:

```
_meta/04_Decals_Guide.md
_meta/05_Lighting_PostProcess_Guide.md
```

**Has water**:

```
docsNew/05_Development/Water_System_Research.md
```

**High fidelity (any archetype)**:

```
docsNew/05_Development/RO_Classic_Visual_Style_Research.md  ← critical, NO cel-shading
3d art/3D_World_Asset_Pipeline_2026.md                       ← Hunyuan3D + 3D Coat workflow
```

**Has NPCs / quests**:

```
RagnaCloneDocs/08_NPCs_Quests.md
```

**RO Classic gameplay reference** (skim for design intent):

```
RagnaCloneDocs/06_World_Maps_Zones.md  (RO map flag / zone design conventions)
RagnaCloneDocs/14_Art_Visual_Style.md  (if exists — not all referenced files are present)
```

---

## Memory entries already in `CLAUDE.md` auto-load

These come pre-loaded with every session via the auto-memory system. Do NOT re-read these unless the user explicitly asks — they're already in your context. Just be aware they exist:

| Entry | Relevance to zone work |
|---|---|
| `water-system-mixed-mode.md` | AWaterArea single-actor mixed-mode design |
| `ground-texture-system.md` | Texture pipeline, master materials |
| `landscape-ro-18-paint-pavements.md` | **v18 master** — paint-driven Brick/Cobble/Pebble layers. `MI_Landscape_PavedTown_v1` for paved towns |
| `pryth-zone-state.md` | Pryth Phase A reference (warps, audio, navmesh, MI). Useful when building the next paved capital town |
| `ro-visual-style-research.md` | NO cel-shading, NO outlines, posterized terrain only |
| `navmesh-system.md` | OBJ export → server build, off-by-one path bug |
| `map-system.md` | Minimap, world map, loading screen |
| `feedback-decal-textures.md` | Use ONLY RO original textures for decals |
| `feedback-material-versioning.md` | Increment variant numbers, never overwrite |
| `feedback-seamless-textures.md` | Laplacian blend + UV distortion + irrational ratios |
| `feedback-grass-placement.md` | Random vs Painted methods |
| `feedback-no-cell-bombing.md` | Don't try cell bombing — visible grid |
| `feedback-sprite-generation.md` | SDXL Base for scatter sprites, NOT Illustrious |
| `feedback-savegame-over-gconfig.md` | Use SaveGame for persistence in standalone |
| `feedback-scenecapture-postprocess.md` | Any new SceneCapture2D needs `SetPostProcessMaterial(false)` |
| `feedback-uproperty-loaded-classes.md` | UPROPERTY on runtime-loaded UClass* |
| `feedback-warp-arrival-distance.md` | **CRITICAL FOR ALL ZONES** — `destX/Y/Z` must sit ≥1.5× trigger radius from outbound AWarpPortal or player gets instant re-warp loop |

---

## 12-phase workflow (compressed)

The full version lives in the SewerDungeon01 build plan. Quick reference:

| Phase | What | Time |
|---|---|---|
| **A** Server registration | `ro_zone_data.js` block + reverse warp + `ro_world_map_data.js` ZONE_INFO + grid cell | 1-2 hours |
| **B** UE5 level scaffold | Duplicate existing level, verify Level Blueprint + GameMode, delete placeholder geometry | 30 min |
| **C** Ground | Outdoor: Landscape Actor + sculpt + MI. Indoor: static-mesh modular floor + MI | 30 min - 4 hours |
| **D** Ground variety | 30-100 decals + Landscape Grass setup (outdoor) | 30 min - 2 hours |
| **E** 3D props | Place imported assets (trees/rocks/buildings/pillars/braziers/etc.) | 30 min - 4 hours |
| **F** Special features | AWaterArea actors if water; ESkill volumes etc. | 30 min |
| **G** Lighting + post-process + NavMesh | Per-zone PP preset (`PostProcessSubsystem.cpp`), manual lighting in level, Build Paths, ExportNavMesh, copy OBJ to server | 30 min |
| **H** Finishing | Audio mappings (`AudioSubsystem.cpp`), test matrix, polish iterations | 30 min - 4 hours |

**Recompile is required** after Phase G (PostProcess) and Phase H (Audio) C++ edits. The build skill (`/sabrimmo-build-compile`) has the editor target build command:

```
"C:/UE_5.7/Engine/Build/BatchFiles/Build.bat" SabriMMOEditor Win64 Development "C:/Sabri_MMO/client/SabriMMO/SabriMMO.uproject" -waitmutex
```

---

## Per-archetype style sheets

For each archetype, the canonical picks. Use as defaults — user can override.

### Pastoral PAVED town (Prontera-style — Pryth template)

For capital towns where players walk on cobblestone/brick/pebble pavement (not grass).

| Layer | Pick |
|---|---|
| Ground (Landscape MI) | `MI_Landscape_PavedTown_v1` (parented to `M_Landscape_RO_18`) — has Brick/Cobble/Pebble paint layers |
| Master material | **`M_Landscape_RO_18`** — extends v17 with paint-driven pavements (NOT v17 alone) |
| Pavement textures | Cobblestone primary + Brick (plaza) + Pebble (side paths). Generate brick/pebble via `_tools/generate_pryth_pavements.py` if not yet imported |
| Painter workflow | Shift+2 → Paint → create Layer Infos for `Brick`, `Cobble`, `Pebble` → paint base Cobble first, then Brick on plaza, Pebble on side paths |
| Decals | path/cobble wear + dirt + cracks + dark wagon stains, 40-60 visible. Less flower-heavy than the grassy variant |
| Grass | `GT_V3_Grassland` painted into residential side-yards only (NOT plaza/streets) |
| BGM | `bgm_08` Theme of Prontera |
| Ambient | `PrtMarket` + `PrtBirds` + `PrtBell` |
| Lighting | Outdoor — Directional pitch −50°, Sky 2.5, warm fog |
| Props | `house_prontera_small/medium/tall/wide/corner`, `fountain_basin/stone`, market stalls, awnings, banners, lampposts, fences, carts |
| Reference build | `docsNew/05_Development/Pryth_Build_Plan.md` |

### Pastoral GRASSY town/field (rural, prt_fild-style)

For rural villages or open fields where the dominant surface is meadow grass.

| Layer | Pick |
|---|---|
| Ground | `MI_grassland_*` v3 (parented to `M_Landscape_RO_17`) with warm WarmthTint (1.02, 0.99, 0.94) |
| Decals | dirt + flower scatter, 40-60 visible |
| Grass | `GT_V3_Grassland` (random uniform OR painted clumps) |
| BGM | `bgm_08` Theme of Prontera (or `bgm_12` Streamside for fields) |
| Ambient | `PrtMarket` + `PrtBirds` + `PrtBell` (towns) or `FieldWind1` + `PrtBirds` (fields) |
| Lighting | Outdoor — Directional pitch −50°, Sky 2.5, warm fog |
| Props | `tree_oak/lush/dense/old`, `bush_round/wide/sparse`, `fence_wooden`, `mushroom_brown` |

### Open field (prt_fild)

| Layer | Pick |
|---|---|
| Ground | `MI_grassland_*` neutral tint |
| Decals | dirt + leaf scatter, 30-50 |
| Grass | `GT_V3_Grassland` |
| BGM | `bgm_12` Streamside or `bgm_05` Tread on the Ground |
| Ambient | `FieldWind1` + `PrtBirds` |
| Lighting | Outdoor — same as pastoral town |
| Props | `tree_oak/lush/dense/old`, `bush_round/wide/sparse`, `mushroom_brown`, `fence_wooden`, rocks |

### Forest (Payon Forest)

| Layer | Pick |
|---|---|
| Ground | M_Landscape_RO_17 with green-shifted WarmthTint, lower brightness |
| Decals | leaf scatter + moss + roots, 50-70 |
| Grass | `GT_V3_Forest` |
| BGM | `bgm_03` Peaceful Forest |
| Ambient | `PayGrass` + `PayAnimal` |
| Lighting | Outdoor warm, slightly desaturated |
| Props | `tree_oak_dense/old`, `tree_willow`, `tree_dead`, `mushroom_giant/blue`, `reeds_cattails`, `rock_mossy_large` |

### Desert (Morroc, moc_fild)

| Layer | Pick |
|---|---|
| Ground | `MI_V11..V20_Sand_*` (Morroc), `MI_V17_Sand_GoldenHot` (deep desert) |
| Decals | sand drift + cracked earth + bone, 30-50 |
| Grass | `GT_V3_Desert` |
| BGM | `bgm_11` Theme of Morroc, or `bgm_24` Desert |
| Ambient | `MocWindMid` (town) or `MocWind` (field, strong) |
| Lighting | Outdoor very warm, exposure +1.5 |
| Props | `house_morroc/_arch/_dome`, `tent_morroc`, `dock_wooden` (oasis), boulder_huge, dead trees |

### Snow (Lutie)

| Layer | Pick |
|---|---|
| Ground | Custom MI on M_Landscape_RO_17, BrightnessOffset +0.20, SaturationMult 0.6, cool WarmthTint |
| Decals | ice/frost patterns, 30-50 |
| Grass | `GT_V3_Snow` |
| BGM | (no canonical, pick a soft one) |
| Ambient | quiet wind layer |
| Lighting | Outdoor cold, low saturation |
| Props | `tree_dead_white`, snow-tinted oak, festive lights |

### Cursed/haunted (Glast Heim, Niflheim)

| Layer | Pick |
|---|---|
| Ground | Cool, desat MI on M_Landscape_RO_17 (Niflheim: SaturationMult 0.4, dead-purple tint) |
| Decals | blood + bone + slime + dark stains, 50-80 |
| Grass | `GT_V3_Cursed` |
| BGM | `bgm_42` Curse'n Pain (`gl_*`) |
| Ambient | `DunWind` (light) |
| Lighting | Outdoor dim or indoor — varies by sub-zone |
| Props | `pillar_broken/whole/fluted/carved`, `tomb_stone/cross`, `coffins`, `brazier_iron`, `cobwebs_corner`, `bone_pile`, `statue_warrior` |

### Cave (Byalan, ant_hell)

| Layer | Pick |
|---|---|
| Ground | `MI_RO_DungeonFloor` darker variant |
| Decals | cracks + puddles + dark stains, 40-60 |
| Grass | `GT_V3_Cave` (only if accent — caves are mostly bare) |
| BGM | `bgm_29` Be Nice 'n Easy or `bgm_49` Watery Grave |
| Ambient | `CaveDrip` + `CaveFall` |
| Lighting | Indoor — Point Lights at intersections, no sky |
| Props | `crystal_blue/green/purple`, `rock_mossy_large`, `pillar_broken`, `brazier_iron`, `bone_pile` |

### Sewer (prt_sewb1-4) — see SewerDungeon01 build plan for full reference

| Layer | Pick |
|---|---|
| Ground | Custom `MI_DungeonFloor_Wet` on M_Landscape_RO_09 base |
| Decals | slime green tint + moss + cracks + dark stains, 60-80 |
| BGM | `bgm_19` Under the Ground |
| Ambient | `DunWind` + `CaveDrip` + `CaveFall` |
| Fog | density 0.30, falloff 0.2, moss-green inscatter, FogHeight=500 |
| PostProcess | Bloom 0.15, Vignette 0.5, ExposureBias 0.0, WhiteTemp 5500 |
| Props | brick walls (Hunyuan-generated), pillars, brazier_iron, cobwebs_corner, bone_pile, crates, barrels |

### Volcanic (Veins, Thor)

| Layer | Pick |
|---|---|
| Ground | Custom MI: red cliff, very warm |
| Decals | lava cracks + scorch + sulfur, 40-60 |
| Grass | `GT_V3_Volcanic` |
| BGM | (research §2 for canonical pick) |
| Ambient | `MocWind` (strong wind) |
| Lighting | Outdoor warm orange tint OR indoor dim |
| Props | dark rocks, cracked terrain, brazier_iron, dead trees, `crystal_purple` |

### Beach/tropical (Jawaii, Comodo)

| Layer | Pick |
|---|---|
| Ground | Sand variant with high SaturationMult 1.15 |
| Decals | shells + starfish + coral, 30-50 |
| Grass | `GT_V3_Beach` |
| BGM | (research §2 for canonical pick) |
| Ambient | water drips + birds |
| Lighting | Outdoor very bright, high saturation |
| Props | `dock_wooden`, `tree_bamboo` (palm-feel), sand decals |

---

## Asset inventory (paths)

When user picks "use existing imported assets only", these are the libraries to inventory:

| Path | Contents |
|---|---|
| `3d art/Imported_to_UE5/Final/architecture/` | Houses (Prontera/Payon/Morroc/Alberta), towers, walls, dock, shrine |
| `3d art/Imported_to_UE5/Final/vegetation/` | Trees (oak/willow/sakura/bamboo/dead), bushes, mushrooms, reeds |
| `3d art/Imported_to_UE5/Final/props/` | Barrels, crates, sacks, baskets, market stalls, lampposts, banners, signs, tables, benches, haystacks, fences, carts |
| `3d art/Imported_to_UE5/Final/terrain/` | Rocks (small/medium/large/huge/mossy/split/cluster), bridges, stairs, paths, wells, fountains |
| `3d art/Imported_to_UE5/Final/centerpiece/` | Stone fountain, warrior statue, wooden well |
| `3d art/Imported_to_UE5/Final/dungeon/` | Pillars, tombs, coffins, sarcophagus, brazier_iron, crystals, cobwebs, bone_pile |
| `3d art/Imported_to_UE5/Final/special/` | Anvil, alchemy table, kafra_pad, emperium_pedestal |

Run a fresh Glob on `3d art/Imported_to_UE5/Final/**/*.glb` at session start to get the current inventory — the library grows.

| Path | Contents |
|---|---|
| `client/SabriMMO/Content/SabriMMO/Materials/Environment/` | M_Landscape_RO_* masters, MI_RO_* variants |
| `client/SabriMMO/Content/SabriMMO/Materials/Environment/v3/` | ~920 MI variants for 20 RO zones |
| `client/SabriMMO/Content/SabriMMO/Materials/Environment/Decals/RO_Decals/` | 91 MI_RODecal_* (USE THESE — never AI-generated) |
| `client/SabriMMO/Content/SabriMMO/Audio/BGM/` | 121 bgm_NN tracks (pre-renewal RO Classic) |
| `client/SabriMMO/Content/SabriMMO/Audio/SFX/Ambient/` | se_*_wind, se_subterranean_*, se_pyramid_*, etc. |
| `client/SabriMMO/Content/SabriMMO/Environment/GrassV3/` | 60 V3 sprite scatter assets, 13 zone GrassTypes |

---

## Critical pitfalls (one-line each)

| Pitfall | Fix |
|---|---|
| Created level from scratch | Always **duplicate** an existing working level; never `File > New Level` |
| `DefaultPawnClass` set on GameMode | Must be **None** (`GM_MMOGameMode` Class Defaults) |
| Warp actor location ≠ server `radius * 2` of warp coord | Sync the AWarpPortal to within 400 UE of server `(x, y, z)` |
| **Warp arrival point sits inside outbound trigger sphere** → instant re-warp loop | `destX/Y/Z` must be ≥1.5× trigger radius from the matching outbound AWarpPortal (with `radius: 400`, target ≥800u, prefer ≥1000u). Diagnostic: `LogWarpPortal: Player entered` immediately after `Zone transition complete`. See `feedback-warp-arrival-distance.md` |
| Used `LandscapeLayerSample` for paint layers in v18+ | Use **`LandscapeLayerWeight`** with Base=Constant(0) + Layer=Constant(1) — same pattern as v17. LayerSample sometimes fails to register the layer in Paint UI |
| NPC `NpcId` case mismatch | Server registry key is case-sensitive |
| Forgot to re-export NavMesh after geometry change | Build Paths → `ExportNavMesh <zone>` → copy `client/server/navmesh/<zone>.obj` → `server/navmesh/<zone>.obj` → delete cache → restart server |
| AI-generated decal textures used | Only use `MI_RODecal_*` (RO original textures), tint warm brown, opacity 0.35 |
| Used Illustrious-XL for scatter sprites | Use SDXL Base + "product render" + bottom-align |
| Ground texture prompt has "grass" | Describe **colors**, not plants |
| Material instance overwritten in place | Always increment variant number (`_v2`, `_v3`) |
| LandscapeGrassOutput added via Python | Must be **manual** in Material Editor (Python causes compile fail) |
| Cel-shade or outline post-process added | Don't. RO style is diffuse + warm + flat |
| `mc_algo="dmc"` in Hunyuan3D | Always `mc_algo="mc"` on torch 2.12 / sm_120 |
| Texturing high-poly mesh before decimating | Decimate to 5K **first**, then texture (40s vs 25 min) |
| `SceneCapture2D` without disabling PostProcessMaterial | Add `ShowFlags.SetPostProcessMaterial(false)` |
| Sculpted terrain after placing AWaterArea | Nudge property to retrigger `OnConstruction` |
| `UClass*` member without `UPROPERTY` | GC collects the class → access violation |
| Build target = `SabriMMO` instead of `SabriMMOEditor` | Editor DLL doesn't update — code runs stale. Always build `SabriMMOEditor` |
| Live Coding active blocks UBT rebuild | Close editor or press `Ctrl+Alt+F11` (CPP-only changes only) |
| Header (`.h`) change with Live Coding | Will crash the editor. Close it, do full rebuild, reopen |
| Z=0 for fog on a high-floor zone (sewer at Z=620) | Override `FogHeightFalloff` to 0.2 (default 2.0 = drops to ~0 at altitude) AND lift `FogHeight` (actor Z) to floor level |

---

## Sample dialog from this prompt (so future-Claude knows the cadence)

**User**: pastes starter prompt with blanks filled in for "Geffen Field 04 (gef_fild04, Enchanted Woods), high fidelity, no water, no NPCs, connects from Geffen town"

**Claude**:
1. Reads `_prompts/new_zone_creation.md` (this file)
2. Loads skills: `sabrimmo-zone`, `sabrimmo-3d-world`, `sabrimmo-navmesh`, `sabrimmo-audio-player`, `sabrimmo-map`, `sabrimmo-build-compile`, `sabrimmo-landscape`, `sabrimmo-ground-textures`, `sabrimmo-environment-grass`, `sabrimmo-material-decals`, `sabrimmo-art` (high-fidelity)
3. Reads `Sewer_Dungeon_01_Build_Plan.md` (template), `Zone_System_UE5_Setup_Guide.md`, `_meta/01-05`, `RO_Classic_Visual_Style_Research.md`, `_journal/Dashboard.md`
4. Globs the asset library
5. Reports: "Loaded 11 skills + 9 docs. Asset library has X new GLBs since 2026-05-07. Dashboard says nothing materially changes the plan template."
6. Asks via `AskUserQuestion`:
   - Which Geffen variant — bright daytime or dark cursed-feel? (these dictate WarmthTint + decal mix)
   - Reuse existing tree/rock library, or generate new mossy-stones via Hunyuan3D for high-fidelity?
   - Any specific monsters from RO Classic gef_fild04 (Stainer, Horn, Coco, Dryad)?
7. Once user answers, generates `docsNew/05_Development/Geffen_Field_04_Build_Plan.md`
8. After user reviews/approves, starts Phase 12.1: writes the `gef_fild04` block in `ro_zone_data.js`, etc.

---

## Maintenance

If you (the current session) discover something materially missing from this prompt — a new skill, a new memory file, a deprecated API, a fresh imported asset family, a new pitfall — **edit this file** before ending the session. The next zone build benefits.

Last updated: 2026-05-09 (after `Pryth` Phase A shipped — `M_Landscape_RO_18` paint-driven pavement master + `MI_Landscape_PavedTown_v1` reusable MI; pavement texture generator at `_tools/generate_pryth_pavements.py`; warp arrival distance pitfall added; second canonical template alongside `SewerDungeon01`).

Previous: 2026-05-08 (`SewerDungeon01` foundation — server data, audio, post-process, fog, navmesh, water, spawn volumes).
