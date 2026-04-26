# 3D World Implementation Plan — Ragnarok Online Classic Style

> **STATUS (as of 2026-04-15)**
> - **Phase 0 — Post-Process Foundation**: **IMPLEMENTED** in C++ (PostProcessSubsystem, CastingCircleActor, blob shadow material, custom stencil). Runtime materials auto-create if editor assets are missing.
> - **Ground/Material system — SHIPPED (out-of-plan, 2026-03-30 → 2026-04-01)**: 1,061 AI + RO textures, 17 material versions, 2,700+ variants, DBuffer decals (91), Landscape Grass V3 (60 sprites, 13 zones), 80+ scripts. See `_meta/01–05_*` guides and `/sabrimmo-landscape`, `/sabrimmo-ground-textures`, `/sabrimmo-material-decals`, `/sabrimmo-environment-grass` skills.
> - **Phases 1–7 — NOT STARTED**: terrain sculpting / AI-generated prop assembly / per-zone build-out. Blocked on editor work (Prontera first, then Payon/Geffen/Morroc). C++ foundation is ready to consume assets as they land.
> - **Rendering decision (2026-04-06)**: sprite-vs-world uses BLEND_Translucent + bDisableDepthTest + binary alpha + per-pixel depth occlusion (post-process cutout via PPI_CustomStencil). See `memory/sprite-rendering-2026-04-06.md`.

## Context

Sabri_MMO has a fully working 2D sprite system (characters, enemies, equipment layers) rendered as billboards in a 3D UE5 world. The 3D world itself is currently placeholder geometry (basic cubes/planes from LevelPrototyping). This plan adds the visual 3D environment — cel-shaded post-processing, stylized materials, terrain, buildings, props, and dungeon kits — to make the world look like Ragnarok Online Classic with an "HD-2D Plus" aesthetic.

**Goal**: 2D billboard sprites in a cel-shaded 3D world with per-zone color palettes, Sobel outlines on 3D geometry (but NOT on sprites), and AI-generated 3D props.

---

## Current State

| Component | Status |
|-----------|--------|
| 4 game zones (server registry) | Complete |
| 7 UE5 level files | Exist (L_Prontera 2.2MB, others ~170KB placeholder) |
| 2D sprite system (18+ classes, 776 atlas files) | Complete |
| Sprite material (Unlit, Masked, TwoSided ProceduralMesh) | Complete |
| Camera (-55 pitch, 1200 arm length) | Complete |
| NavMesh (all 4 zones) | Complete |
| Engine (Lumen, VSM, Ray Tracing, Substrate) | Enabled but unused |
| Post-process chain | **COMPLETE** — built-in UE5 color grading, no custom PP materials |
| Auto-lighting (sun + sky + fog) | **COMPLETE** — per-zone DirectionalLight + SkyLight + HeightFog |
| Occlusion transparency | **COMPLETE** — camera ray trace hides blocking actors |
| Master environment material | **COMPLETE** (C++ runtime, not auto-applied yet) |
| RO-style textures (12) | **COMPLETE** — 7 surface + 5 ground (2048x2048 seamless) via ComfyUI |
| TRELLIS.2 3D generation | **INSTALLED + MODELS DOWNLOADED** — ready for use |
| Blender asset pipeline | **WORKING** — trees, rocks, bush, building generated + textured |
| Blob shadows for sprites | **COMPLETE** (material created in C++) |
| Custom stencil on sprites | **COMPLETE** |
| 3D environment meshes | **NOT STARTED** (blocked on TRELLIS models) |
| Terrain/Landscape | **NOT STARTED** (editor work) |

---

## Phase 0: Post-Process Foundation (Highest Visual Impact)

### Phase 0A: PostProcessSubsystem — DONE

Created `UPostProcessSubsystem` (UWorldSubsystem) that:
- Auto-spawns unbound `APostProcessVolume` on BeginPlay
- Loads 3 PP materials from `/Game/SabriMMO/Materials/PostProcess/` (graceful skip if missing)
- Creates MIDs, adds to volume's `WeightedBlendables`
- Applies per-zone presets (bloom, vignette, saturation, tint, contrast)
- Locks auto-exposure for dungeons

**Files created:**
- [x] `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.h`
- [x] `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp`
- [x] `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs` — added `"RenderCore"` module

**Zone presets:**

| Zone | Bloom | Vignette | Saturation | ZoneTint | Contrast |
|------|-------|----------|------------|----------|----------|
| `prontera` | 0.4 | 0.25 | 1.1 | (1.05, 1.02, 0.95) | 1.0 |
| `prontera_south` | 0.4 | 0.2 | 1.0 | (1.0, 1.0, 1.0) | 1.0 |
| `prontera_north` | 0.4 | 0.2 | 1.0 | (0.98, 0.98, 1.02) | 1.0 |
| `prt_dungeon_01` | 0.2 | 0.4 | 0.7 | (0.85, 0.85, 1.0) | 1.2 |

### Phase 0B: Post-Process Materials — DONE (runtime C++)

All 3 PP materials are created programmatically at runtime using `UMaterialExpressionCustom` (same pattern as sprite materials). Editor assets at `Content/SabriMMO/Materials/PostProcess/` override runtime materials if present.

#### PP_CelShade (Post Process domain, Before Tonemapping)

Posterizes luminance to 2-3 bands while preserving hue:

```hlsl
float3 Color = SceneTexture(PostProcessInput0).rgb;
float Luminance = dot(Color, float3(0.2126, 0.7152, 0.0722));
float Bands = BandCount;  // ScalarParam, default 3.0
float Quantized = floor(Luminance * Bands) / Bands;
float FinalLum = lerp(Quantized, Luminance, Smoothing);  // ScalarParam, default 0.1
float3 Result = Color * (FinalLum / max(Luminance, 0.001));
return Result;
```

Parameters: `BandCount` (3.0), `Smoothing` (0.1)

#### PP_Outline (Post Process domain, After Tonemapping)

Sobel edge detection on depth + normals with Custom Stencil exclusion for sprites:

```hlsl
float2 TexelSize = OutlineThickness / ViewSize;
float dN = SceneDepth(UV + float2(0, -TexelSize.y));
float dS = SceneDepth(UV + float2(0,  TexelSize.y));
float dE = SceneDepth(UV + float2( TexelSize.x, 0));
float dW = SceneDepth(UV + float2(-TexelSize.x, 0));
float depthEdge = saturate((abs(dN - dS) + abs(dE - dW)) * DepthSensitivity);

float3 nN = WorldNormal(UV + float2(0, -TexelSize.y));
float3 nS = WorldNormal(UV + float2(0,  TexelSize.y));
float normalEdge = saturate((1.0 - dot(nN, nS)) * NormalSensitivity);

float edge = max(depthEdge, normalEdge);
float stencil = CustomStencil(UV);
edge *= (stencil < 0.5) ? 1.0 : 0.0;  // skip sprites (stencil=1)

float3 Scene = SceneTexture(PostProcessInput0).rgb;
return lerp(Scene, OutlineColor, edge);
```

Parameters: `OutlineThickness` (1.5), `DepthSensitivity` (50.0), `NormalSensitivity` (5.0), `OutlineColor` ((0.15, 0.10, 0.05) dark brown)

#### PP_ColorGrade (Post Process domain)

```hlsl
float3 Color = SceneTexture(PostProcessInput0).rgb;
float Lum = dot(Color, float3(0.2126, 0.7152, 0.0722));
float3 Grey = float3(Lum, Lum, Lum);
float3 Saturated = lerp(Grey, Color, Saturation);
float3 Contrasted = ((Saturated - 0.5) * Contrast) + 0.5;
return Contrasted * ZoneTint;
```

Parameters: `ZoneTint` (White), `Saturation` (1.0), `Contrast` (1.0)

### Phase 0C: Custom Stencil on Sprites — DONE

Modified `SpriteCharacterActor.cpp` `CreateLayerQuad()` to set `CustomDepthStencilValue = 1` on all ProceduralMeshComponents. Added `r.CustomDepth=3` to DefaultEngine.ini.

### Phase 0D: Blob Shadow — DONE (C++ code)

Added `UDecalComponent* BlobShadow` to `SpriteCharacterActor`. Projects downward, loads `M_BlobShadow` material. Gracefully hides if material not yet created.

**M_BlobShadow material** — created at runtime in C++ (Deferred Decal, Translucent, radial gradient via Custom HLSL). Editor asset at `Content/SabriMMO/Materials/M_BlobShadow` overrides if present.

### Phase 0 Verification

- [ ] Open L_PrtSouth in PIE
- [ ] Cel-shading on placeholder cubes (once PP materials created)
- [ ] Outlines at geometry edges (once PP materials created)
- [ ] Sprites have NO outlines (stencil exclusion)
- [ ] Blob shadow under player sprite (once M_BlobShadow created)
- [ ] Zone tint changes per zone

---

## Phase 1: Master Environment Material — NOT STARTED

### Phase 1A: M_Environment_Stylized (UE5 Editor)

Path: `Content/SabriMMO/Materials/M_Environment_Stylized`

**Texture Parameters:** BaseColor, NormalMap, ORM (Occlusion R, Roughness G, Metallic B)

**Scalar Parameters:** RoughnessMin (0.6), RoughnessMax (0.9), MetallicOverride (0.0), SaturationMult (1.0), UVScale (1.0)

**Vector Parameters:** TintColor (White), EmissiveTint (Black)

**Rules:** Roughness 0.6-0.9, metallic binary 0 or 1, hand-painted textures only.

### Phase 1B: Material Instances

| Instance | Use | Roughness | Metallic |
|----------|-----|-----------|----------|
| MI_Env_Stone | Walls, bridges | 0.8 | 0 |
| MI_Env_Wood | Buildings, fences | 0.7 | 0 |
| MI_Env_Grass | Landscape grass | 0.9 | 0 |
| MI_Env_Dirt | Landscape dirt | 0.85 | 0 |
| MI_Env_Rock | Landscape steep | 0.85 | 0 |
| MI_Env_Cobblestone | Town roads | 0.75 | 0 |
| MI_Env_DungeonFloor | Dungeon floors | 0.5 | 0 |
| MI_Env_DungeonWall | Dungeon walls | 0.7 | 0 |
| MI_Env_Metal | Gates, grates | 0.4 | 1 |

---

## Phase 2: Terrain — NOT STARTED

### Landscape Auto-Material

Path: `Content/SabriMMO/Materials/M_Landscape_AutoSlope`
- Flat (0-20 deg): Grass
- Medium (20-45 deg): Dirt
- Steep (45-90 deg): Rock

### Per-Zone Specs

| Zone | Size | Height | Style |
|------|------|--------|-------|
| prontera | ~10000x5000 UU | Flat center | Grass + cobblestone |
| prontera_south | ~6000x6000 UU | Rolling hills | Grass + dirt |
| prontera_north | ~6000x6000 UU | Steeper, rocky | Grass + rock |
| prt_dungeon_01 | N/A | Modular floor tiles | No landscape |

### NavMesh Re-Export (after each zone's terrain)

1. UE5: Build → Build Paths
2. Console: `ExportNavMesh <zone_name>`
3. Copy OBJ to `server/navmesh/`
4. Delete cache, restart server

---

## Phase 3: Prontera Town — NOT STARTED

### Asset List (AI-generated 3D)

**Structures:** Castle (15-20K), Church (10-15K), 2x Town Gate (3-5K ea), 8-12x Residential (2-3K ea), 3-4x Shop (2-3K ea), Fountain (3-5K), 2x Bridge (1-2K ea), 8-12x Wall Segments (500-1K ea), 2-4x Watchtower (2-3K ea)

**Props:** 6-10x Street Lamps, 4-8x Barrels, 4-8x Crates, 3-5x Market Stalls, 4-6x Benches, 3-5x Signposts, 1-2x Wells

**Foliage (NO Nanite):** 10-20x Stylized Trees, 8-15x Bushes, 4-8x Flower Patches

### Lighting

Directional Light: Pitch -45, Yaw 135, 7000K warm, 3.14 lux
Sky Atmosphere + Sky Light (Real Time Capture)
Exponential Height Fog: density 0.01, warm inscattering

---

## Phase 4: Prontera South Field — NOT STARTED

Natural assets: trees, rocks, grass, cave entrance near dungeon warp, signpost near town warp. Neutral lighting.

---

## Phase 5: Prontera Dungeon 1F — NOT STARTED

### Modular Kit Pieces

Floor Tile (200x200 UU), Wall variants (straight, corner, T, doorway), Pillars, Stairs, Torch Holders, Sewer Grates, Water Channels

### Dungeon Lighting

NO sky/directional. Point lights at torches: 2700K, 500 lumens, 400 UU radius. PostProcessSubsystem dungeon preset auto-applies.

---

## Phase 6: Prontera North Field — NOT STARTED

Rockier variant of Phase 4, slightly cooler palette.

---

## Phase 7: Polish — NOT STARTED

- 7A: Occlusion transparency (camera → player ray trace, fade occluding buildings)
- 7B: Foliage wind animation (WPO sine wave)
- 7C: Water planes (translucent, UV scroll)
- 7D: Ambient audio hooks

---

## Dependency Graph

```
Phase 0A (C++ subsystem) ✓
    ↓
Phase 0B (PP materials — editor) + 0C (Stencil) ✓ + 0D (Blob shadow) ✓
    ↓
[GATE: Sprites OK, cel-shade + outlines on grey-box]
    ↓
Phase 1 (Master material + instances)
    ↓
Phase 2 (Terrain) ←→ Phase 3A-3B (Asset generation)
    ↓                       ↓
    └──→ Phase 3C-3E (Assembly + NavMesh)
              ↓
    Phase 4 + Phase 5 (parallel)
              ↓
           Phase 6
              ↓
           Phase 7 (Polish)
```

---

## Sprite Interaction Test Protocol (run after EACH phase)

1. Player sprite renders (all 9 layers, animation, direction)
2. NO outlines on sprite edges (stencil exclusion)
3. NO posterization on sprite colors (Unlit)
4. Blob shadow circle on ground below sprite
5. Billboard rotation works with 3D environment
6. Enemy sprites spawn and pathfind on terrain
7. Remote player sprites render correctly
8. Combat works (damage numbers, health bars, death)
9. 60+ FPS with post-process chain

---

## Critical Files

| File | Phase | Change |
|------|-------|--------|
| `UI/PostProcessSubsystem.h/.cpp` | 0A | NEW — done |
| `Sprite/SpriteCharacterActor.h/.cpp` | 0C, 0D | Stencil + blob shadow — done |
| `SabriMMO.Build.cs` | 0A | Added RenderCore — done |
| `Config/DefaultEngine.ini` | 0C | r.CustomDepth=3 — done |
| `UI/CameraSubsystem.cpp` | 7A | Occlusion transparency — pending |
| `server/src/ro_zone_data.js` | 2C | Spawn Z updates if terrain changes — pending |
