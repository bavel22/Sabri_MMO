# Complete Level Building Guide — Sabri_MMO

> Everything you need to build a full 3D zone from scratch. Every system, every option, every parameter, every script.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Layer 1: Landscape (Terrain Geometry)](#2-layer-1-landscape-terrain-geometry)
3. [Layer 2: Materials (Ground Textures & Blending)](#3-layer-2-materials-ground-textures--blending)
4. [Layer 3: Decals (Ground Detail Overlays)](#4-layer-3-decals-ground-detail-overlays)
5. [Layer 4: Scatter Objects (Grass, Flowers, Debris)](#5-layer-4-scatter-objects-grass-flowers-debris)
6. [Layer 5: Post-Process & Lighting](#6-layer-5-post-process--lighting)
7. [Layer 6: 3D Assets (Trees, Rocks, Buildings)](#7-layer-6-3d-assets-trees-rocks-buildings)
8. [Layer 7: Camera & Occlusion](#8-layer-7-camera--occlusion)
9. [Texture Generation Pipeline (ComfyUI)](#9-texture-generation-pipeline-comfyui)
10. [Complete Script Reference](#10-complete-script-reference)
11. [Zone Color Palettes](#11-zone-color-palettes)
12. [Performance Budgets](#12-performance-budgets)
13. [Step-by-Step: Building a Zone from Nothing](#13-step-by-step-building-a-zone-from-nothing)
14. [Lessons Learned & Pitfalls](#14-lessons-learned--pitfalls)

---

## 1. Architecture Overview

The 3D world is built from 7 independent, composable layers. Each layer adds visual richness without depending on the others. You can start with just a landscape and progressively add layers.

```
Layer 7: Camera & Occlusion      (CameraSubsystem — C++)
Layer 6: 3D Assets                (Trees, rocks, buildings — Blender/TRELLIS FBX)
Layer 5: Post-Process & Lighting  (PostProcessSubsystem — C++, per-zone)
Layer 4: Scatter Objects           (Landscape Grass system — V3 sprites)
Layer 3: Decals                   (DBuffer decals — 91 ready-to-use)
Layer 2: Materials                (M_Landscape_RO_17 — 23+ parameters, 2700+ variants)
Layer 1: Landscape                (UE5 Landscape Actor — sculpted terrain)
```

**Key principle**: The game renders 2D billboarded sprites on 3D terrain. Every visual decision must preserve sprite readability — no cel-shading post-process, no saturation overrides, no outline effects that touch sprites.

---

## 2. Layer 1: Landscape (Terrain Geometry)

The Landscape Actor is the foundation of every outdoor zone. It provides sculpted terrain with built-in collision, LOD, paint layers, and grass spawning support.

### 2.1 Creating a Landscape

1. Open your zone level in UE5
2. **Shift+2** to enter Landscape Mode
3. Click the **Create** tab
4. Configure:

| Setting | Recommended | Notes |
|---------|-------------|-------|
| **Material** | `M_Landscape_RO_17` | Full-featured with paint layers |
| **Section Size** | 63x63 Quads | Best for most zones |
| **Number of Components** | 4x4 to 8x8 | Small zone = 4x4, large zone = 8x8 |
| **Overall Resolution** | Auto-calculated | Based on section size x components |

5. Click **Create**

#### Section Size Reference

| Size | Best For | Total Quads (at 4x4 components) |
|------|----------|------|
| 7x7 | Very small interiors | 784 |
| 15x15 | Small zones | 3,600 |
| **63x63** | **Most zones (recommended)** | **63,504** |
| 127x127 | Large open fields | 258,064 |
| 255x255 | Massive world areas | 1,040,400 |

#### Per-Zone Size Reference

| Zone | Approx Size (UU) | Components | Height Range | Style |
|------|-------------------|------------|--------------|-------|
| Prontera Town | ~10000x5000 | 8x4 | Flat center | Grass + cobblestone |
| Prontera South | ~6000x6000 | 4x4 | Rolling hills | Grass + dirt |
| Prontera North | ~6000x6000 | 4x4 | Steeper, rocky | Grass + rock cliffs |
| Dungeons | N/A | N/A | Flat | Use static mesh floor tiles, not landscape |

### 2.2 Sculpting Tools

Switch to the **Sculpt** tab after creating a landscape:

| Tool | What It Does | When to Use |
|------|-------------|-------------|
| **Sculpt** | Left-click = raise terrain, Shift+click = lower | Main terrain shaping |
| **Smooth** | Softens harsh edges and bumps | After rough sculpting — makes terrain natural |
| **Flatten** | Makes area flat at the height you first click | Town centers, building foundations |
| **Erosion** | Simulates natural weathering | Cliff edges, rocky areas |
| **Noise** | Adds random bumpy detail | Organic surface texture on flat areas |
| **Ramp** | Creates smooth ramp between two points | Paths between elevations |
| **Retopologize** | Rebuilds mesh topology | Fix degenerate triangles after heavy editing |

#### Sculpting Controls

| Control | Action |
|---------|--------|
| **Left-click** | Apply tool (raise/smooth/etc.) |
| **Shift + Left-click** | Inverse (lower/unsculpt) |
| **[ and ]** | Decrease/increase brush size |
| **Brush Falloff** | 0-1 slider, higher = softer edges |
| **Tool Strength** | 0-1 slider, higher = stronger effect per stroke |
| **Ctrl+Z** | Undo (works for sculpting) |

#### Sculpting Strategy for RO Zones

1. **Flat center first** — main walkable area where players move, NPCs stand, combat happens
2. **Gentle rolling hills** — visual interest, helps break up flat monotony, drives grass variant blending
3. **Steep cliff edges** (>45 degrees) — zone boundaries and impassable walls
4. **Gradual slopes for paths** — connecting different elevation areas (keep under 40 degrees for walkable)
5. **Final pass: Smooth tool** everywhere for natural-looking organic terrain
6. **Detail pass: Noise tool** for micro-surface detail

### 2.3 Slope = Gameplay Indicator (Critical Rule)

The material system automatically shows rock/cliff texture on steep slopes. This is a **gameplay indicator** that matches NavMesh exclusion:

| Slope Angle | DotProduct(Normal, Up) | Material Shows | Gameplay |
|-------------|------------------------|----------------|----------|
| 0-40 degrees | 0.77 - 1.0 | 100% grass/dirt | **Walkable** |
| 40-53 degrees | 0.60 - 0.77 | Grass-to-rock transition | Edge of walkable |
| 53-90 degrees | 0.0 - 0.60 | 100% rock/cliff | **Impassable** |

**When sculpting**: Make slopes >45 degrees wherever the player should NOT go. The material and NavMesh both respect this threshold automatically.

### 2.4 Paint Layers (M_Landscape_RO_17 only)

The v17 material supports 3 paintable layers for manual control over grass scatter placement:

#### Setup (one-time per landscape)

1. Assign `M_Landscape_RO_17` to the Landscape Actor
2. **Shift+2** to enter Landscape Mode
3. Switch to the **Paint** tab
4. Three layers appear: **GrassDense**, **FlowerPatch**, **Debris**
5. Click the **+** button next to each layer
6. Choose **Create Layer Info** and save each one (creates `.landscapelayerinfo` assets)

#### Painting

1. Select a layer (e.g., GrassDense)
2. Left-click on the landscape to paint
3. **Shift+Left-click** to erase
4. **[ and ]** to resize brush
5. Adjust **Brush Falloff** and **Tool Strength** as needed

#### What Each Layer Controls

| Layer | Parameter Name | What It Drives | Use For |
|-------|---------------|----------------|---------|
| **GrassDense** | `GrassDense` | Dense grass clump scatter | Thick grass areas, meadows |
| **FlowerPatch** | `FlowerPatch` | Flower cluster scatter | Flower gardens, colorful areas |
| **Debris** | `Debris` | Rocks, leaves, twigs, zone-specific items | Rocky areas, forest floors, dungeon debris |

Each layer is multiplied by the slope mask — grass never appears on cliffs regardless of painting.

### 2.5 After Sculpting: Re-Export NavMesh

After finalizing terrain, the server-side NavMesh must be updated:

1. In UE5: **Build > Build Paths**
2. Open Output Log, run console command: `ExportNavMesh <zone_name>`
3. Copy the exported `.obj` file to `server/navmesh/`
4. Delete the server's navmesh cache file
5. Restart the server

### 2.6 Do NOT Use

- **Nanite Landscape** — Buggy in UE5 5.7 (chunks disappear, displacement breaks, 32GB+ RAM required)
- **Static mesh for terrain** — Landscape Actor has built-in sculpting, paint layers, grass output, LOD, collision, and uses 7x less memory
- **Tessellation/Displacement** — No visual benefit for our flat hand-painted style

---

## 3. Layer 2: Materials (Ground Textures & Blending)

The material system controls what the terrain looks like — which textures appear, how they blend, slope detection, anti-tiling, and per-zone color mood.

### 3.1 Material Hierarchy

```
M_Landscape_RO_17 (Master Material — 23+ parameters, full-featured)
    |
    +-- MI_Prontera_Grassland (Material Instance — overrides textures + colors)
    +-- MI_Payon_Forest (Material Instance — different textures + colors)
    +-- MI_Morroc_Desert (Material Instance — different textures + colors)
    +-- ... (2700+ variants available)
```

You never edit the master material. You create Material Instances that override specific parameters for each zone's look.

### 3.2 Available Master Materials

| Material | Version | Parameters | Features | Best For |
|----------|---------|------------|----------|----------|
| **M_Landscape_RO_17** | v17 | 23+ | Paint layers, slope blend, UV distortion, macro variation, color processing | **Production zones (use this)** |
| **M_Landscape_RO_14** | v14 | 23 | Same as v17 minus paint layers | Zones without painted grass |
| **M_Landscape_RO_12** | v12 | 23 | Original full-param material | Legacy reference |
| **M_Landscape_RO_09** | v09 | ~15 | Hard 45-deg cutoff, original balance | Simple zones |
| **M_RO_Original** | — | 8 | Lightweight, original RO textures | Minimal overhead zones |

### 3.3 M_Landscape_RO_17 Parameters (Complete Reference)

#### Texture Slots (9 parameters)

| Parameter | Type | What It Controls |
|-----------|------|-----------------|
| **GrassWarmTexture** | Texture2D | Primary ground texture (olive/sage green for grass zones) |
| **GrassCoolTexture** | Texture2D | Secondary ground texture (noise-blended with warm) |
| **DirtTexture** | Texture2D | Dirt/earth texture (appears in transition zones and flat patches) |
| **RockTexture** | Texture2D | Cliff/rock texture (appears on slopes >45 degrees) |
| **GrassWarmNormal** | Texture2D | Normal map for warm ground (subtle bump detail) |
| **GrassCoolNormal** | Texture2D | Normal map for cool ground |
| **DirtNormal** | Texture2D | Normal map for dirt |
| **RockNormal** | Texture2D | Normal map for rock |
| **GrassAO** | Texture2D | Ambient occlusion map (crevice darkening) |

#### Color Processing (4 parameters)

| Parameter | Type | Range | Default | What It Controls |
|-----------|------|-------|---------|-----------------|
| **WarmthTint** | Vector (RGB) | — | (1.0, 1.0, 1.0) | Color multiply on entire terrain. Use warm gold (1.02, 0.99, 0.94) for Prontera, cold blue (0.92, 0.96, 1.04) for Rachel |
| **SaturationMult** | Scalar | 0.3 - 1.2 | 1.0 | Desaturation control. Niflheim ~0.35 (nearly grayscale), Jawaii ~1.15 (vivid tropical) |
| **BrightnessOffset** | Scalar | -0.2 to 0.3 | 0.0 | Shifts overall brightness. Dungeons: -0.15 (darker), Snow: +0.20 (brighter) |
| **ContrastBoost** | Scalar | 0.8 - 1.5 | 1.0 | Contrast curve power. Volcanic: 1.4 (harsh), Snow: 0.9 (soft) |

#### Texture Blending (4 parameters)

| Parameter | Type | Range | Default | What It Controls |
|-----------|------|-------|---------|-----------------|
| **GrassVariantBalance** | Scalar | 0 - 1 | 0.5 | Bias between GrassWarm and GrassCool. 0 = all warm, 1 = all cool |
| **GrassNoiseScale** | Scalar | 0.001 - 0.01 | 0.004 | Size of splotchy patches where warm/cool grass blend. Higher = smaller patches |
| **MacroNoiseScale** | Scalar | 0.0003 - 0.003 | 0.001 | Large-scale brightness variation frequency. Prevents monotone terrain |
| **DirtOnSlopes** | Scalar | 0 - 1 | 0.3 | How much dirt appears in the grass-to-rock transition band |

#### Slope Detection (4 parameters)

| Parameter | Type | Range | Default | What It Controls |
|-----------|------|-------|---------|-----------------|
| **SlopeThreshold** | Scalar | 0.5 - 0.8 | 0.60 | DotProduct value where rock begins appearing. Lower = more rock |
| **SlopeTransitionWidth** | Scalar | 0.05 - 0.25 | 0.15 | Width of the grass-to-rock transition. Lower = sharper edge |
| **SlopeNoiseAmount** | Scalar | 0 - 0.15 | 0.05 | Makes the slope line wavy/organic instead of perfectly straight |
| **SlopeNoiseFreq** | Scalar | 0.005 - 0.02 | 0.01 | How jagged/fine the slope noise pattern is |

#### Surface Properties (6+ parameters)

| Parameter | Type | Range | Default | What It Controls |
|-----------|------|-------|---------|-----------------|
| **DirtAmount** | Scalar | 0 - 0.5 | 0.1 | Dirt patches on flat ground (noise-driven) |
| **Roughness** | Scalar | 0.65 - 0.98 | 0.95 | Surface shininess. RO is fully rough (0.95). Lower for wet/polished surfaces |
| **NormalStrength** | Scalar | 0 - 1 | 0.5 | Bump detail intensity from normal maps |
| **AOStrength** | Scalar | 0 - 1 | 0.5 | How dark crevices/ambient occlusion gets |
| **UVDistortStrength** | Scalar | 20 - 100 | 50 | Seam line warping strength. Higher = more distorted tile seams |
| **GrassWarmTileSize** | Scalar | 2000 - 8000 | 4000 | Tile size for warm grass texture in Unreal Units |
| **GrassCoolTileSize** | Scalar | 2000 - 8000 | 5173 | Tile size for cool grass (irrational ratio prevents seam alignment) |
| **DirtTileSize** | Scalar | 2000 - 8000 | 3347 | Tile size for dirt |
| **RockTileSize** | Scalar | 2000 - 8000 | 2891 | Tile size for rock |

#### Paint Layer Weights (3 parameters — v17 only)

| Parameter | Type | What It Controls |
|-----------|------|-----------------|
| **GrassDense** | LandscapeLayerWeight | Thick grass clump regions (painted in editor) |
| **FlowerPatch** | LandscapeLayerWeight | Flower cluster regions (painted in editor) |
| **Debris** | LandscapeLayerWeight | Rock/leaf/twig regions (painted in editor) |

### 3.4 How the Material Blending Works (Node Architecture)

```
WorldPosition
  -> ComponentMask(XY)
  -> UV Noise Distortion (warps seam lines into wavy curves)
  -> Divide by TileSize (per texture)
  -> TextureSample

GrassWarm + GrassCool
  -> Lerp(biased by GrassVariantBalance + Perlin noise at GrassNoiseScale)
  -> Multiply by MacroVariation (3-scale brightness variation at 500/1400/6600 UU wavelengths)
  -> Lerp with Dirt (driven by DirtAmount + noise)
  -> Lerp with Rock (driven by slope DotProduct + SmoothStep threshold)

Rock uses XZ projection (not XY) so cliff faces texture correctly without stretching.

Final color chain:
  -> Multiply by AO (crevice darkening)
  -> Multiply by WarmthTint (zone color)
  -> Desaturation (SaturationMult)
  -> Add BrightnessOffset
  -> Power(ContrastBoost)
  -> Output BaseColor
```

### 3.5 Three Layers of Anti-Tiling

No single technique eliminates visible tile repetition. The material uses all three:

| Layer | Technique | What It Does | Parameter |
|-------|-----------|-------------|-----------|
| **1. Texture-level** | Laplacian pyramid blend (at generation time) | Fixes edge discontinuities in the source texture | N/A (baked into texture) |
| **2. UV distortion** | Noise-based UV offset | Warps straight tile seam lines into wavy curves | `UVDistortStrength` (20-100) |
| **3. Irrational tile ratios** | Each texture tiles at a different size | Seams from different textures never stack/align | `GrassWarmTileSize`, `GrassCoolTileSize`, `DirtTileSize`, `RockTileSize` |

### 3.6 Macro Variation (Anti-Repetition)

The material samples `T_Default_MacroVariation` at 3 different world scales simultaneously:

| Scale | Wavelength | Visual Effect |
|-------|-----------|---------------|
| 0.002 | 500 UU | Small splotchy patches |
| 0.0007 | 1400 UU | Medium landscape-scale variation |
| 0.00015 | 6600 UU | Large gentle brightness drifts |

These multiply together to create organic, non-repeating brightness variation across the entire terrain.

### 3.7 Pre-Made Material Variant Libraries

| Location | Count | Parent Material | Description |
|----------|-------|----------------|-------------|
| `/Materials/Environment/v3/` | ~920 | M_Landscape_RO_12 | 20 RO zones x cross-biome variants |
| `/Materials/Environment/v4/` | ~610 | M_RO_Original | Original RO textures, simple material |
| `/Materials/Environment/ROClassic/` | ~200 | M_Landscape_RO_11 | Warm yellow-green RO classic look |
| `/Materials/Environment/BiomeVariants/` | ~915 | M_Landscape_RO_11 | 15 generic biomes |
| `/Materials/Environment/Variants/` | ~30 | M_Landscape_RO_09 | Early 30 hand-crafted variants (V01-V30) |

### 3.8 Creating a New Material Instance

**Method 1: In Editor**
1. Right-click `M_Landscape_RO_17` in Content Browser
2. Choose **Create Material Instance**
3. Open the new MI, override desired parameters
4. Drag onto your Landscape Actor

**Method 2: Via Python Script**
Run `Scripts/Environment/create_variants_v3.py` in UE5 Python console:
```
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\create_variants_v3.py").read())
```

### 3.9 Texture Sources

| Source | Location | Count | Resolution | Quality | Use For |
|--------|----------|-------|-----------|---------|---------|
| **AI-Generated (ComfyUI)** | `Textures/Environment/Ground/` | 43 | 1024x1024 | Good | Generic ground |
| **AI Upscaled** | `Textures/Environment/Ground_2K/` | 18 | 2048x2048 | Better | Hero zones |
| **AI Seamless-Fixed** | `Textures/Environment/Ground_Seamless/` | 18 | 2048x2048 | Best AI | Primary textures |
| **Per-Zone AI** | `Textures/Environment/ROZones/` (16 folders) | 96 | 1024x1024 | Good | Zone-specific |
| **Per-Biome AI** | `Textures/Environment/Biomes/` (15 folders) | 94 | 1024x1024 | Good | Biome-specific |
| **Original RO** | `Textures/Environment/RO_Original/` | 608 | Various | Authentic | Best for RO feel |
| **Sand/Desert** | `Textures/Environment/Sand/` | 10 | 1024x1024 | Good | Morroc-style |
| **RO Classic Match** | `Textures/Environment/ROClassic/` | 8 | 1024x1024 | Tuned | Reference match |

**Normal Maps**: 43 at `Ground/Normals/`, 18 at `Ground_2K/Normals/`
**Depth Maps**: 43 at `Ground/Depth/`
**AO Maps**: 43 at `Ground/AO/`

### 3.10 Techniques Evaluated But NOT Used

| Technique | Why Not |
|-----------|---------|
| **Cell Bombing** | Creates a visible Voronoi cell grid — tested and rejected. Stick with UV distortion + irrational ratios |
| **Posterized Lighting in Material** | Was recommended in research but not yet implemented. Would add RO's stepped shadow look to terrain only |
| **Hex Tiling** | 3 texture samples per pixel — deferred, cell bombing was tried first (and rejected), UV distortion is sufficient |
| **Material Parameter Collection** | Global zone mood transitions — deferred, using MI parameter overrides per zone instead |
| **Substrate Materials** | UE5.7 replacement for fixed shading — overkill for our pure diffuse terrain |
| **Runtime Virtual Texturing** | Caches complex material shading — our material is only ~150 instructions, below the threshold where RVT helps |
| **Normal maps for bump detail** | Barely visible at our top-down camera angle + 0.95 roughness. Generates but mostly ignored |

---

## 4. Layer 3: Decals (Ground Detail Overlays)

DBuffer decals project textures onto the terrain surface to add visual variety — dirt patches, cracks, moss, paths, bloodstains. They make terrain look "lived-in" rather than monotonously tiled.

### 4.1 What Decals Are

A decal is a texture projected onto existing surfaces (like a projector shining an image onto a wall). In our system, decals project downward onto the terrain to overlay detail without modifying the landscape material.

### 4.2 Five Parent Decal Materials

Each has soft radial falloff and noise edge distortion built in:

| Material | Default Tint | Best For | Typical Use |
|----------|-------------|----------|-------------|
| **M_Decal_Dirt** | Warm brown (0.35, 0.28, 0.18) | Worn patches, smudges, general ground variety | Everywhere |
| **M_Decal_Cracks** | Dark gray (0.20, 0.20, 0.22) | Stone cracks, fractures, scuff marks | Paths, ruins, old stone |
| **M_Decal_Moss** | Dark green (0.15, 0.25, 0.12) | Vegetation, leaf scatter, organic growth | Forests, damp areas |
| **M_Decal_Path** | Sandy brown (0.40, 0.35, 0.25) | Footpaths, sand, light-colored detail | Roads, sandy areas |
| **M_Decal_DarkStain** | Dark purple (0.10, 0.08, 0.12) | Blood, shadow pools, dark stains, puddles | Dungeons, battle sites |

### 4.3 Decal Material Parameters

Every Material Instance exposes these:

| Parameter | Type | Default | Range | What It Controls |
|-----------|------|---------|-------|-----------------|
| **DecalTexture** | Texture2D | — | — | The projected texture image |
| **DecalTint** | Vector (RGB) | Varies by parent | — | Color multiply on the texture. Keep warm/muted |
| **OpacityStrength** | Scalar | 0.35 | 0.25 - 0.50 | Overall visibility. Lower = more subtle. Above 0.5 looks painted-on |
| **EdgeSoftness** | Scalar | 1.5 | 0.5 - 3.0 | Radial falloff power. Lower = wider fade at edges |
| **EdgeNoiseScale** | Scalar | 5.0 | 2.0 - 10.0 | Edge noise frequency. Higher = more jagged/organic boundary |

### 4.4 Decal Node Graph (How It Works)

```
DecalTexture (RO_Original texture)
  |
  +-- RGB --> Multiply(DecalTint) --> BaseColor output
  |
  +-- RGB --> Desaturate(1.0) --> OneMinus --> "texture_opacity"

Radial Gradient (math: 1 - clamp(dot(UV-0.5, UV-0.5) * 4))
  --> Power(EdgeSoftness) --> "radial_mask"

Noise(TexCoord * EdgeNoiseScale, output range 0.5-1.0) --> "edge_noise"

radial_mask * edge_noise --> "organic_mask"
organic_mask * texture_opacity * OpacityStrength --> Opacity output
```

This creates soft, blobby-edged decals that never show hard rectangular boundaries.

### 4.5 Ready-to-Use Decal Instances

| Location | Count | Source Textures | Quality |
|----------|-------|----------------|---------|
| `/Decals/RO_Decals/` | ~76 | Original RO textures | **Best — use these** |
| `/Decals/Instances/` | 15 | Original RO textures | Good (first batch) |
| `/Textures/Environment/Decals/` | 40 | AI-generated | **DO NOT USE** — garish colors |

### 4.6 Placing Decals

#### Method 1: Drag from Content Browser (Fastest)
1. Navigate to `/Game/SabriMMO/Materials/Environment/Decals/RO_Decals/`
2. Drag any `MI_RODecal_*` into the viewport
3. UE5 auto-creates a DecalActor
4. Use **W/E/R** to move, rotate, scale

#### Method 2: Duplicate Existing (Quick Iteration)
1. Select any existing `TerrainDecal_*` in the viewport
2. **Ctrl+D** to duplicate
3. Move to new position, randomize yaw rotation

#### Method 3: Python Spawning (Bulk Placement)
```python
location = unreal.Vector(x, y, z)
rotation = unreal.Rotator(-90.0, random_yaw, 0.0)  # -90 pitch = project downward
actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DecalActor, location, rotation)
actor.set_actor_scale3d(unreal.Vector(scale, scale * 1.5, scale * 1.5))
decal_comp = actor.get_component_by_class(unreal.DecalComponent)
decal_comp.set_decal_material(unreal.load_asset(material_path))
```

### 4.7 Decal Placement Settings

| Setting | Value | Why |
|---------|-------|-----|
| **Rotation Pitch** | -90 | Projects straight down onto terrain |
| **Rotation Yaw** | 0-360 (randomize) | Varies direction per decal for organic look |
| **Scale X** | 1.5 - 5.0 | Controls projection depth (how far into surfaces) |
| **Scale Y/Z** | 1.5 - 5.0 | Controls width/height on ground |
| **Location Z** | Slightly above terrain | Decal projects downward from this height |

### 4.8 Artistic Placement Tips

- **Overlap** decals slightly for natural coverage
- **Randomize yaw** rotation for every decal (no two oriented the same)
- **Mix types** in the same area: dirt + moss + cracks = rich detail
- **Place at transitions**: grass-to-cliff edges, path-to-grass borders, near buildings
- **Cluster** 3-5 decals of varied types per point of interest
- **Keep subtle**: OpacityStrength 0.25-0.40 — above 0.5 looks like painted-on stamps

### 4.9 Per-Zone Decal Recommendations

| Zone | Decal Types to Use | Recommended Count | Artistic Direction |
|------|-------------------|-------------------|-------------------|
| **Prontera Fields** | Dirt paths, flower scatter, stone patches | 40-60 | Warm, welcoming, pastoral |
| **Geffen** | Moss, cracks, arcane stains | 30-50 | Cool, mysterious, old |
| **Payon Forest** | Leaf scatter, moss, root tendrils, mushroom marks | 50-70 | Dense forest floor, organic |
| **Morroc Desert** | Sand drift, cracked earth, bone scatter | 30-50 | Sparse, sun-bleached, hot |
| **Lutie Snow** | Ice patches, frost patterns, snow drifts | 30-50 | Cold, clean, festive |
| **Glast Heim** | Blood stains, bone scatter, claw marks, slime | 50-80 | Horror detail, heavy coverage |
| **Niflheim** | Dark stains, shadow pools, dead patches | 30-50 | Desaturated, lifeless |
| **Veins Volcanic** | Lava cracks, scorch marks, sulfur deposits | 40-60 | Hot, dangerous |
| **Amatsu** | Cherry petals, bamboo leaves, zen rake lines | 30-50 | Serene, cultural detail |
| **Dungeons** | Cracks, puddles, dark stains, rubble | 40-60 | Underground, worn |

### 4.10 Prerequisites

- **DBuffer Decals** must be enabled: Edit > Project Settings > Rendering > Lighting > DBuffer Decals = checked
- Already active if using Lumen/VSM (depth prepass runs automatically)

### 4.11 Critical Rules

1. **Use Original RO textures, NOT AI-generated** — AI decal textures on white backgrounds look garish. The 608 original RO textures are already muted and hand-painted
2. **Tint must be warm brown** — Working tint: (0.85, 0.80, 0.72). Colorful tints clash with terrain
3. **Low opacity** — Default 0.35. Decals are subtle detail, not overpowering overlays
4. **Soft edges required** — Every decal has radial gradient falloff + noise edge distortion built in

---

## 5. Layer 4: Scatter Objects (Grass, Flowers, Debris)

The Landscape Grass system spawns small 2D sprite meshes (billboard crosses) across terrain. This adds grass blades, flowers, pebbles, mushrooms, and zone-specific debris as 3D detail on the ground plane.

### 5.1 System Generations

| Version | Location | Approach | Mesh Type | Status |
|---------|----------|----------|-----------|--------|
| **V1** | `/Environment/Grass/` | Blender geometry + flat colors/RO textures | 3D shapes (cones, spheres) | Working but basic |
| **V2** | `/Environment/GrassV2/` | Billboard cross + AI sprite proof-of-concept | Billboard crosses | Working — proved the approach |
| **V3** | `/Environment/GrassV3/` | Production AI sprites on billboard meshes | Billboard crosses | **Current best — use this** |

**Always use V3 for new work.**

### 5.2 V3 Asset Inventory

**60 sprite types across 13 zone-themed GrassType assets:**

| GrassType Asset | Zone | Varieties | Sprite Types |
|----------------|------|-----------|-------------|
| **GT_V3_Grassland** | Prontera | 11 | Grass clumps (A/B/C, 01/02), Daisies, Pink flowers, Purple flowers, Yellow flowers, Dandelions, Clover, Tall grass |
| **GT_V3_Forest** | Payon | 13 | Ferns (A/B/01), Mushrooms (Brown/Red/White/02), Autumn leaves (A/B), Green leaf, Moss clump, Pine cone, Twig |
| **GT_V3_Desert** | Morroc | 5 | Dry grass (A/B), Small cactus, Desert rock, Dead branch |
| **GT_V3_Snow** | Lutie | 3 | Snow clump, Ice crystal, Frozen plant |
| **GT_V3_Beach** | Jawaii | 5 | Shells (A/B), Starfish, Coral piece, Seaweed |
| **GT_V3_Volcanic** | Veins | 3 | Lava rock, Obsidian shard, Sulfur crystal |
| **GT_V3_Cursed** | Glast Heim | 4 | Bones (A/B), Dead grass, Candle stub |
| **GT_V3_Amatsu** | Amatsu | 5 | Cherry petals (A/B), Bamboo leaf, Grass, Flowers |
| **GT_V3_Moscovia** | Moscovia | 6 | Birch leaf, Pinecone B, Berry, Ferns, Grass, Flowers |
| **GT_V3_Ruins** | Ruins | 3 | Stone fragment, Pottery shard, Dry grass |
| **GT_V3_Industrial** | Einbroch | 2 | Gear piece, Metal scrap |
| **GT_V3_Cave** | Caves | 3 | Blue crystal, Green crystal, Stalagmite |
| **GT_V3_Dungeon** | Dungeons | 3 | Chain piece, Rubble, Bones |

### 5.3 How V3 Works

Each sprite is:
1. An AI-generated PNG (SDXL Base, "product render" style, white-to-alpha converted, bottom-aligned)
2. Applied to a **unique mesh copy** of a billboard cross (two planes at 90 degrees)
3. Material: `BLEND_MASKED`, two-sided, `opacity_mask_clip = 0.33`, roughness 0.95
4. Texture RGB goes to BaseColor, Texture Alpha goes to OpacityMask

### 5.4 Two Placement Methods

#### Method A: Random/Uniform (Automatic — No Painting Required)

Grass appears everywhere on flat terrain automatically.

**Setup** (Manual in Material Editor):
1. Open landscape material (e.g., `M_Landscape_RO_17`)
2. Right-click canvas > search **"Grass Output"** > add `LandscapeGrassOutput` node
3. Click the node, add entries (one per GrassType you want)
4. For each entry, select the appropriate `GT_V3_*` asset
5. Connect the material's **slope Clamp** output to each entry's input pin
6. Apply and Save

**Result**: Scatter appears on all flat ground (slope < 45 degrees), density controlled by the GrassType asset.

#### Method B: Painted Clumps (Manual Brush Control)

Grass appears ONLY where you paint in the editor.

**Setup** (Manual in Material Editor):
1. Use **M_Landscape_RO_17** (has paint layers built in)
2. Open in Material Editor
3. Add `LandscapeGrassOutput` node
4. Connect:
   - **grass_dense_final** output -> grass variety inputs (GT_V3_Grassland, etc.)
   - **flower_final** output -> flower variety inputs
   - **debris_final** output -> debris/rock/leaf variety inputs
5. Apply and Save
6. Assign material to Landscape
7. **Shift+2** > Landscape Mode > **Paint** tab
8. Create Layer Infos for GrassDense, FlowerPatch, Debris
9. Select a layer, left-click to paint on terrain
10. **[ and ]** to resize brush

**Result**: Scatter appears ONLY where you painted, in exact clumps you control.

#### Method C: Combined (Mix Both)

Some varieties on random, others on painted:
- Basic grass clumps -> connect to slope Clamp (always present on flat ground)
- Flower clusters -> connect to flower_final paint layer (only where painted)
- Mushroom patches -> connect to debris_final paint layer (only where painted)

### 5.5 GrassType Variety Properties

Each entry in a GrassType asset has these adjustable properties:

| Property | Type | Range | Default | What It Controls |
|----------|------|-------|---------|-----------------|
| **Grass Mesh** | Static Mesh | — | SM_Billboard_* | The mesh to spawn (unique per sprite) |
| **Grass Density** | Float | 100 - 2000 | 400 | Instances per component section. Higher = more dense |
| **Start Cull Distance** | Int | 1000 - 5000 | 3000 | Distance where instances start fading out |
| **End Cull Distance** | Int | 3000 - 7000 | 5000 | Distance where instances are fully hidden |
| **Min LOD** | Int | 0 - 3 | 0 | Minimum LOD level to use |
| **Scaling** | Enum | Uniform | Uniform | How scaling randomization works |
| **Scale X (Min)** | Float | 0.3 - 1.0 | 0.5 | Minimum random scale |
| **Scale X (Max)** | Float | 1.0 - 3.0 | 2.0 | Maximum random scale |
| **Random Rotation** | Bool | — | true | Randomize yaw rotation per instance |
| **Align to Surface** | Bool | — | true | Tilt mesh to match terrain slope |
| **Use Landscape Lightmap** | Bool | — | true | Use landscape's lightmap for shading |

### 5.6 Adding a New Sprite Type

1. **Generate the sprite** (see Section 9 for ComfyUI pipeline):
   - Use SDXL Base checkpoint (`sd_xl_base_1.0.safetensors`)
   - Prompt: `"ONE single [object description], product render, game asset, isolated on white background, centered, nothing else in image"`
   - Negative: `"multiple objects, many, several, tiling, repeating, grid, collage, panel, frame, border"`
   - Settings: CFG 7.0, 30 steps, euler sampler, 512x512
2. **Post-process**: White-to-alpha (threshold 200), bottom-align, quality check
3. **Copy PNG** to `Content/SabriMMO/Environment/GrassV3/Textures/`
4. **In UE5**: Import texture, duplicate billboard mesh, create alpha-masked material, add to GrassType
5. **Connect** to Grass Output node in landscape material

Or use the automated script:
```
# Add entry to SPRITES dict in _tools/generate_all_sprites_v3.py, then:
C:/ComfyUI/venv/Scripts/python.exe C:/Sabri_MMO/_tools/generate_all_sprites_v3.py

# Then in UE5 Python console:
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\setup_grass_v3_fixed.py").read())
```

### 5.7 Critical Rules

1. **Use SDXL Base, NOT Illustrious-XL** for scatter sprites — Illustrious produces abstract chaos for isolated objects
2. **"product render"** is the magic prompt keyword — makes SDXL treat the object as a product photo
3. **Bottom-align all sprites** — without shifting content to bottom of image, sprites float above ground
4. **One mesh copy per sprite texture** — GrassType varieties share mesh material slot 0, so each sprite needs its own mesh with its own material
5. **Grass Output node is MANUAL ONLY** — cannot be set up via Python, causes material compilation failure
6. **Scale 0.5-2.0** for V3 billboard meshes — they export at ~30-150 UU in engine

---

## 6. Layer 5: Post-Process & Lighting

`PostProcessSubsystem` (C++) auto-spawns and configures all lighting and post-process effects per zone. You don't place lights manually — the subsystem handles everything.

### 6.1 What Gets Spawned Automatically

| Actor | Purpose | Outdoor | Dungeon |
|-------|---------|---------|---------|
| **ADirectionalLight** (Sun) | Primary directional light | 3.14 lux, warm white, pitch -50 | Disabled (intensity 0) |
| **ASkyLight** (Ambient) | Fills shadows with ambient color | 2.5 intensity, warm | 0.3 intensity, cool blue |
| **AExponentialHeightFog** | Atmospheric depth haze | Density 0.004, warm haze | Density 0.03, dark purple |
| **APostProcessVolume** (Global) | Bloom, vignette, exposure, color grading | Per-zone presets | Darker, more vignette |

### 6.2 Outdoor Lighting Defaults

| Component | Property | Value |
|-----------|----------|-------|
| **Sun** | Intensity | 3.14 lux (pi) |
| | Color | (1.0, 0.95, 0.85) — warm white |
| | Rotation | Pitch -50, Yaw 135 (afternoon sun angle) |
| | Cast Shadows | true |
| **Sky** | Intensity | 2.5 |
| | Color | (0.95, 0.92, 0.85) — warm ambient |
| | Source | Captured Scene |
| **Fog** | Density | 0.004 |
| | Height Falloff | 0.5 |
| | Inscattering Color | (0.7, 0.65, 0.5) — warm haze |

### 6.3 Dungeon Lighting

| Component | Property | Value |
|-----------|----------|-------|
| **Sun** | Intensity | 0 (disabled) |
| **Sky** | Intensity | 0.3 |
| | Color | (0.4, 0.4, 0.6) — cool blue |
| **Fog** | Density | 0.03 |
| | Height Falloff | 2.0 |
| | Inscattering Color | (0.1, 0.08, 0.15) — dark purple |

### 6.4 Per-Zone Post-Process Presets

| Zone | Bloom | Vignette | Exposure Bias | White Temp | Color Gain Highlights |
|------|-------|----------|---------------|-----------|----------------------|
| **prontera** | 0.4 | 0.25 | 1.5 | 6800K (warm) | (1.03, 1.0, 0.96) — warm push |
| **prontera_south** | 0.35 | 0.2 | 1.5 | 6500K (neutral) | (1.0, 1.0, 1.0) — neutral |
| **prontera_north** | 0.35 | 0.2 | 1.5 | 6200K (cool) | (0.98, 0.98, 1.02) — cool push |
| **prt_dungeon_01** | 0.2 | 0.4 | 0.0 | 5000K (cold) | (0.92, 0.92, 1.05) — blue push |

#### What Each Parameter Does

| Parameter | What It Controls | Low Value | High Value |
|-----------|-----------------|-----------|------------|
| **Bloom** | Glow around bright areas | 0.0 = no glow | 1.0 = heavy glow |
| **Vignette** | Dark edges around screen | 0.0 = none | 1.0 = heavy darkening |
| **Exposure Bias** | Overall brightness compensation | 0.0 = darker (dungeon) | 1.5+ = brighter (outdoor, compensates for Unlit sprite darkening) |
| **White Temp** | Color temperature of scene | 5000K = cool/blue | 7000K = warm/golden |
| **Color Gain Highlights** | Color tint on bright areas | R > 1.0 = warmer highlights | B > 1.0 = cooler highlights |

### 6.5 Global Post-Process Settings (All Zones)

```
BloomThreshold: 1.5
MotionBlurAmount: 0.0 (disabled — sprite billboards smear)
AutoExposureSpeedUp: 20.0 (near-instant — prevents zone transition brightness bleed)
AutoExposureSpeedDown: 20.0
ColorSaturation: NOT overridden (affects sprites — leave at default)
ColorContrast: NOT overridden (affects sprites — leave at default)
```

### 6.6 Post-Process Cutout Material

A special post-process material that darkens the scene around the player sprite:

- Reads `CustomStencil` buffer (sprites have stencil value = 1)
- 9-tap stencil dilation creates a halo around the sprite
- Non-sprite pixels within the halo are darkened to 35% brightness
- Makes the player sprite always clearly visible against any background
- `FadeAmount` parameter controls intensity (default 0.6)

**Important**: This material affects ALL cameras. `MinimapSubsystem` disables it on its overhead capture with `ShowFlags.SetPostProcessMaterial(false)`. Any new `SceneCaptureComponent2D` must do the same.

### 6.7 User-Adjustable Brightness

`BrightnessMultiplier` (range 0.5-2.0, default 1.0) — scales sun light intensity from 1.57-6.28 lux. Exposed in Options menu.

### 6.8 Adding a New Zone Preset

In `PostProcessSubsystem.cpp`, add an `else if` block in `ApplyZonePreset()`:

```cpp
else if (ZoneName == TEXT("new_zone_name"))
{
    Bloom = 0.35f;
    Vignette = 0.2f;
    ExposureBias = 1.5f;      // outdoor: 1.5, dungeon: 0.0
    WhiteTemp = 6500.f;        // 5000=cool, 6500=neutral, 6800=warm
    GainHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);  // (R, G, B, 1)
}
```

Also add lighting config in `SetupSceneLighting()` if the zone type differs from standard outdoor/dungeon.

### 6.9 Zone Name Resolution During Transitions

During zone transitions, `GI->CurrentZoneName` is stale (still the OLD zone name). The subsystem handles this:
```cpp
FString ZoneName = GI->bIsZoneTransitioning
    ? GI->PendingZoneName
    : GI->CurrentZoneName;
```

### 6.10 What NOT to Do

| Don't | Why |
|-------|-----|
| Add cel-shade post-process | Causes scene darkening and color washing — tested and rejected |
| Add outline post-process | Affects sprites, adds unwanted edges — tested and rejected |
| Override ColorSaturation | Washes out sprite colors — use material WarmthTint instead |
| Override ColorContrast | Affects sprite readability — use material ContrastBoost instead |
| Use post-process posterization | Affects sprites — do posterization in terrain material only (if at all) |
| Add complex PBR specular | RO Classic is fully diffuse/matte. Keep roughness 0.95 |
| Use Lumen bounce lighting | Overkill for our warm flat lighting style |

---

## 7. Layer 6: 3D Assets (Trees, Rocks, Buildings)

Static mesh props placed in the level for visual richness — trees, rocks, buildings, fences, barrels, etc.

### 7.1 Scale Calibration

All assets scale relative to `SM_RO_Tree_01` (Blender Z=145 = correct in-game size):

| Asset Type | Target Z (Blender units) | Proportion to Tree |
|-----------|--------------------------|---------------------|
| Tall tree | 180 | 1.2x |
| Standard tree | 145 | 1.0x (reference) |
| Wide/bushy tree | 110 | 0.75x |
| Building | 130 | 0.9x |
| Large rock | 35 | 0.24x |
| Bush | 25 | 0.17x |
| Small rock cluster | 12 | 0.08x |

### 7.2 Blender FBX Export Settings

```python
bpy.ops.export_scene.fbx(
    filepath=path,
    global_scale=1.0,
    apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_ALL',
    mesh_smooth_type='FACE',
    bake_space_transform=True,
    object_types={'MESH'},
    path_mode='COPY',
    embed_textures=True,
)
```

### 7.3 UE5 Import

FBX textures don't import reliably via UE5's Interchange pipeline. Workaround:
1. Import FBX — mesh imports, textures may be missing
2. **Drag textures from Content Browser directly onto meshes** (auto-creates material)
3. In the auto-created material: add a **TexCoord** node, set UTiling/VTiling = 0.25 to reduce visible tiling

### 7.4 Material Style for 3D Assets

All 3D environment meshes use diffuse-only materials matching RO Classic:

| Property | Value | Why |
|----------|-------|-----|
| Shading Model | DefaultLit | Standard UE5 lit |
| Roughness | 0.95 | Fully rough — no specular highlights |
| Metallic | 0.0 | No metallic surfaces in RO |
| Two-Sided | false | Standard single-sided |
| Tint | Warm earth tones | Match zone color palette |

The runtime `M_Environment_Stylized` material is available at `PostProcessSubsystem::EnvMaterial` with:
- TintColor parameter: default (0.65, 0.55, 0.42) warm stone
- Roughness: 0.95
- Metallic: 0.0

### 7.5 TRELLIS.2 (AI 3D Generation)

Installed at `C:/Sabri_MMO/_tools/TRELLIS/` with all models.

| Use Case | Quality | Status |
|----------|---------|--------|
| Hard-surface objects (buildings, walls, props) | Good | Working |
| Organic shapes (trees, foliage, bushes) | **Bad** | Fails — background removal strips green canopy |

**For trees and bushes**: Use Blender procedural generation scripts instead of TRELLIS.

### 7.6 Available Blender Generation Scripts

| Script | Run Command | What It Creates |
|--------|------------|----------------|
| `blender_make_ro_tree.py` | `"C:/Blender 5.1/blender.exe" --background --python _tools/blender_make_ro_tree.py` | Single RO-style tree (tapered trunk + puffy canopy) |
| `blender_make_ro_assets.py` | `"C:/Blender 5.1/blender.exe" --background --python _tools/blender_make_ro_assets.py` | 6 untextured assets (2 trees, 2 rocks, 1 bush, 1 building) |
| `blender_make_ro_assets_textured.py` | `"C:/Blender 5.1/blender.exe" --background --python _tools/blender_make_ro_assets_textured.py` | 6 textured assets with AI textures applied |

---

## 8. Layer 7: Camera & Occlusion

`CameraSubsystem` (C++) creates and controls the RO-style isometric camera.

### 8.1 Camera Configuration

| Property | Value | Notes |
|----------|-------|-------|
| **Fixed Pitch** | -55 degrees | Isometric-like top-down angle |
| **Default Arm Length** | 1200 UU | Distance from player |
| **Min Arm Length** | 200 UU | Closest zoom |
| **Max Arm Length** | 1500 UU | Farthest zoom |
| **Yaw Control** | Right-click drag | Rotates camera around player |
| **Zoom Control** | Mouse wheel | 80 UU per scroll tick (configurable) |
| **Rotation Sensitivity** | 0.6 deg/pixel | Configurable in Options |
| **Collision Test** | Disabled | Camera passes through walls |
| **Camera Lag** | Disabled | Instant camera follow |

### 8.2 Occlusion Approach

The sprite material uses `BLEND_Translucent + bDisableDepthTest`, so the player character sprite always renders on top of any geometry between camera and player. No world geometry hiding is needed — this matches RO Classic's always-visible player sprite.

The subsystem previously had ray-trace occlusion that hid blocking actors, but this was **removed** in favor of the depth-test-disabled sprite approach.

### 8.3 Sprite Visibility System

Sprites use Custom Stencil (value = 1) which enables:
- Post-process cutout material to darken walls around the player (always-visible halo)
- Future effects can check stencil to exclude sprites

Blob shadow (UDecalComponent on each SpriteCharacterActor):
- Rotated -90 degrees (projects downward)
- DecalSize (64, 48, 48)
- Material: runtime radial gradient opacity
- Located at Z+5 (just above ground)

---

## 9. Texture Generation Pipeline (ComfyUI)

All AI-generated textures use ComfyUI at `http://127.0.0.1:8188`.

### 9.1 Ground Texture Generation

#### Required Setup
- Checkpoint: `Illustrious-XL-v0.1.safetensors`
- LoRA: `ROSprites-v2.1C.safetensors`
- Custom nodes: `ComfyUI-seamless-tiling`, `was-node-suite-comfyui`
- Upscaler: `4x-UltraSharp.pth` (in `models/upscale_models/`)

#### ComfyUI Node Chain
```
CheckpointLoaderSimple (Illustrious-XL-v0.1)
  -> LoraLoader (ROSprites-v2.1C @ 0.12-0.15 strength)
    -> SeamlessTile (tiling="enable", copy_model="Make a copy")
      -> CLIPTextEncode (positive + negative prompts)
      -> EmptyLatentImage (1024x1024)
        -> KSampler (euler, 60 steps, cfg=6.5)
          -> CircularVAEDecode (tiling="enable")
            -> SaveImage
```

#### Critical Generation Settings

| Setting | Value | Common Mistake |
|---------|-------|---------------|
| **CFG** | **6.5** | NOT 7.0 — critical difference |
| **Steps** | **60** | NOT 25 — higher = sharper detail |
| **LoRA Strength** | **0.10-0.15** | NOT 0.3+ — causes artifacts |
| **Sampler** | **euler** | Best for SDXL textures at 60 steps |
| **Resolution** | **1024x1024** | Native SDXL size |

#### Prompt Engineering for Ground Textures

**Describe COLORS, not plants.** Illustrious-XL generates leaf/blade shapes when you say "grass".

Good prompts:
```
"hand painted tileable ground texture, stylized game art, oil painting brushstrokes,
warm muted olive green with yellow-brown patches, flat surface, overhead view,
game texture asset, seamless"
```

Bad prompts:
```
"grass texture, green grass, grass blades"  <- generates individual grass blades
"scenery, landscape, grass field"  <- generates a scene, not a flat texture
```

Negative prompt must include:
```
"individual leaves, individual grass blades, clover, shamrock, leaf shapes, stems,
perspective, depth, horizon, room, scene, landscape"
```

#### Prompt Templates by Texture Type

| Type | Key Words | Color Palette |
|------|-----------|---------------|
| **Grass Warm** | "warm muted olive green with yellow-brown patches" | Sage green, ochre, gold-tan |
| **Grass Cool** | "cool emerald green with moss and dark brown patches, forest floor" | Deep green, moss, dark brown |
| **Dirt/Earth** | "warm ochre sienna umber earth tones, cracked earth" | Sienna, ochre, umber |
| **Rock/Cliff** | "muted purple-gray stone, weathering detail" | Purple-gray, slate, warm gray |
| **Cobblestone** | "warm gray ochre medieval stone pavers, irregular shapes" | Gray-brown, warm stone |
| **Sand** | "warm golden sand with ripple patterns, sienna amber" | Golden, sienna, amber |
| **Dungeon Floor** | "dark blue-gray slate stone, cracked and worn, cold" | Dark blue-gray, cold stone |
| **Snow** | "white snow surface, slight blue shadows, ice crystals" | White, ice blue, faint gray |

### 9.2 Post-Processing Pipeline

After generation, textures go through several processing steps:

#### Step 1: Seamless Verification & Fix
```bash
python _tools/fix_seamless_aggressive.py
```
- Checks left/right and top/bottom edge strips (MAD < 2.0 = seamless)
- If edges bad: Laplacian pyramid multi-band cross-blend with 50% offset copy
- **SeamlessTile node alone is NOT enough** — always post-process

#### Step 2: Color Correction
Applied in the generation scripts:
- **Desaturation**: 10-15% via PIL ImageEnhance (factor 0.85-0.90)
- **Warm shift**: R channel +3%, B channel -3%

#### Step 3: Upscaling (Optional, for hero textures)
```bash
python _tools/upscale_and_fix_textures.py
```
- Uses 4x-UltraSharp model (1024 -> 4096 -> downscale to 2048)
- Re-applies seamless cross-blend after upscale

#### Step 4: Map Generation (Optional)
```bash
python _tools/generate_depth_maps.py
```
- **Normal Maps**: BAE-NormalMapPreprocessor (import as TC_NORMALMAP, sRGB OFF)
- **Depth Maps**: MiDaS-DepthMapPreprocessor
- **AO Maps**: Derived from inverted blurred depth

#### Step 5: 3x3 Tile Preview
Each generation script saves a 3x3 tiled preview image alongside the texture for visual QA.

### 9.3 Scatter Sprite Generation

Different pipeline from ground textures — uses **SDXL Base** (not Illustrious-XL):

| Setting | Value | Why Different from Ground |
|---------|-------|--------------------------|
| **Checkpoint** | `sd_xl_base_1.0.safetensors` | SDXL Base works for isolated objects, Illustrious fails |
| **LoRA** | None | Not needed for product renders |
| **CFG** | 7.0 | Standard for SDXL Base |
| **Steps** | 30 | Sufficient for simple objects |
| **Resolution** | 512x512 | Small sprites don't need 1K |

Prompt pattern:
```
"ONE single [object], product render, game asset,
isolated on white background, centered, nothing else in image"

Negative: "multiple objects, many, several, tiling, repeating,
grid, collage, panel, frame, border, photorealistic, photograph"
```

Alpha conversion:
- Whiteness > 200 -> fully transparent
- Whiteness < 150 -> fully opaque
- Smooth ramp between 150-200

Quality check:
- Reject if > 95% transparent (nothing generated)
- Reject if < 15% transparent (background not removed)
- Auto-retry up to 3 times with different seed

Bottom-align:
- Find last content row, shift down if > 5px from bottom
- Without this, sprites float above ground

### 9.4 Decal Texture Generation (NOT Recommended)

AI-generated decal textures at `Textures/Environment/Decals/` (40 textures) were tested and **look garish**. Use the 608 original RO textures at `Textures/Environment/RO_Original/` instead.

---

## 10. Complete Script Reference

### 10.1 Texture Generation Scripts (`_tools/`)

| Script | Run With | Output | What It Does |
|--------|----------|--------|-------------|
| `generate_ground_textures_v2.py` | `python` (ComfyUI API) | `ground_texture_output/` | Core ground texture generator — SeamlessTile, CFG 6.5, 60 steps |
| `generate_ro_zones.py` | `python` (ComfyUI API) | `ground_texture_output/zones/` | 23 RO zone-specific textures (8 per zone, ~184 total) |
| `generate_all_biomes.py` | `python` (ComfyUI API) | `ground_texture_output/biomes/` | 15 biome types (7 per biome, ~120 total) |
| `generate_ro_classic_ground.py` | `python` (ComfyUI API) | `ground_texture_output/roclassic/` | Warm yellow-green RO reference-matching textures |
| `generate_sand_textures.py` | `python` (ComfyUI API) | `ground_texture_output/sand/` | Sandy/desert textures for Morroc-style zones |
| `generate_allinone_textures.py` | `python` (ComfyUI API) | `ground_texture_output/allinone/` | All-inclusive ground variants |
| `generate_tiling_textures.py` | `python` (ComfyUI API) | `ground_texture_output/` | Hand-painted RO-style tiling textures |
| `generate_all_sprites_v3.py` | `python` (ComfyUI API) | `ground_texture_output/sprites_v3/` | V3 scatter sprites (60 types, 13 zones) |
| `generate_grass_sprites_v2.py` | `python` (ComfyUI API) | `ground_texture_output/sprites_v2/` | V2 scatter sprite proof-of-concept |
| `generate_grass_sprites_v3.py` | `python` (ComfyUI API) | `ground_texture_output/sprites_v3/` | V3 re-generate failed sprites |
| `generate_grass_sprites.py` | `python` (ComfyUI API) | `ground_texture_output/sprites/` | V1 original attempt (uses wrong model) |
| `generate_decal_textures.py` | `python` (ComfyUI API) | `ground_texture_output/decals/` | AI decal textures (NOT recommended — use RO originals) |

### 10.2 Post-Processing Scripts (`_tools/`)

| Script | Run With | Input | Output | What It Does |
|--------|----------|-------|--------|-------------|
| `fix_seamless_aggressive.py` | `python` | `upscaled_2k/` | `seamless_fixed/` | Laplacian pyramid multi-band seamless fix |
| `upscale_and_fix_textures.py` | `python` (ComfyUI API) | 1K textures | `upscaled_2k/` | 4x-UltraSharp upscale + seamless + maps |
| `generate_depth_maps.py` | `python` (ComfyUI API) | Any diffuse | Normal/Depth/AO | Generates all auxiliary maps from diffuse |

### 10.3 Blender Scripts (`_tools/`)

| Script | Run With | What It Creates |
|--------|----------|----------------|
| `blender_make_ro_tree.py` | Blender --background | Single RO-style tree (trunk + canopy) |
| `blender_make_ro_assets.py` | Blender --background | 6 untextured RO assets |
| `blender_make_ro_assets_textured.py` | Blender --background | 6 textured RO assets |
| `blender_make_grass_meshes.py` | Blender --background | V1 grass blade/flower/pebble meshes |
| `blender_make_all_scatter.py` | Blender --background | V1 all 60+ zone scatter meshes |
| `blender_make_sprite_meshes.py` | Blender --background | V2/V3 billboard cross meshes with proper UVs |
| `blender_fix_flat_meshes.py` | Blender --background | Fixes zero-height flat scatter meshes |

Blender path: `"C:/Blender 5.1/blender.exe" --background --python <script>`

### 10.4 UE5 Python Scripts (`Scripts/Environment/`)

Run via UE5 Output Log Python console: `exec(open(r"path").read())`

| Script | What It Does |
|--------|-------------|
| `create_landscape_material.py` | Builds M_Landscape_RO master material (23 params) from scratch via Python |
| `create_variants_v3.py` | Creates ~920 Material Instance variants for 20 RO zones |
| `import_ground_textures.py` | Imports all texture PNGs into UE5 with correct settings |
| `import_biome_textures.py` | Imports biome-specific textures |
| `import_ro_zones.py` | Imports RO zone-specific textures |
| `fix_texture_settings.py` | Fixes normal map compression (TC_NORMALMAP, sRGB OFF) |
| `setup_grass_v3_fixed.py` | V3 grass setup — imports sprites, creates materials + mesh copies + GrassTypes |
| `setup_grass_v2_winners.py` | V2 grass setup (reference) |
| `import_all_scatter.py` | V1 mesh import + GrassType creation |
| `rebuild_all_gt_scale.py` | Fix scale on all GrassType varieties |
| `setup_decals.py` | Creates 5 parent decal materials + spawns decal actors |
| `create_decal_instances_v2.py` | Creates ~76 decal instances from RO original textures |
| `fix_decal_tints.py` | Fixes tints on AI-generated decals |
| `debug_grass_v3.py` | Diagnostic — checks what V3 grass assets exist |
| `debug_grass_v2.py` | Diagnostic — checks what V2 grass assets exist |

### 10.5 UE5 Asset Application Scripts (`_tools/`)

| Script | Run With | What It Does |
|--------|----------|-------------|
| `ue5_import_tree.py` | UE5 Python console | Imports tree FBX + places in level |
| `ue5_fix_materials.py` | UE5 Python console | Creates materials from textures + applies to meshes |
| `ue5_apply_ro_materials.py` | UE5 Python console | Creates MIs from AI textures + applies to level actors |

---

## 11. Zone Color Palettes

Every zone has a distinct color identity. Use these when configuring material WarmthTint, texture selection, and decal choices.

| Zone | Ground Colors | Cliff Colors | Temp | Mood |
|------|--------------|-------------|------|------|
| **Prontera** | Sage green `#6B8E5A`, Gold-tan `#C4A870` | Warm brown `#8B6A42` | Warm | Bright pastoral |
| **Geffen** | Cool blue-gray `#6A6A7E`, Forest green `#4A6E3E` | Dark gray `#5A5A6A` | Cool | Mysterious |
| **Payon** | Dark green `#3A5A2A`, Leaf brown `#8A6A3A` | Mossy brown `#5A4A3A` | Warm | Shaded forest |
| **Morroc** | Golden sand `#DAA520`, Bleached `#E8D8B8` | Sandstone `#C4A060` | Very warm | Hot desert |
| **Lutie** | Snow white `#F0E8E8`, Ice blue `#C0D0E0` | Frozen gray `#C0C0C8` | Cold | Festive winter |
| **Glast Heim** | Gray-green `#4A4A3E`, Purple-gray `#4A3A4E` | Dark corrupt `#3A3A3A` | Cold | Haunted decay |
| **Niflheim** | Ash gray `#696969`, Dead purple `#6A5A6E` | Gray-purple `#4A4A4A` | Dead cold | Lifeless |
| **Veins** | Red rock `#B22222`, Black `#1A1A1A` | Red canyon `#A04A20` | Fire hot | Volcanic hostile |
| **Jawaii** | Bright sand `#F4D8A0`, Ocean blue `#2A8AB0` | Coral pink `#D08A6A` | Tropical | Paradise bright |
| **Einbroch** | Iron gray `#5A5A5A`, Rust `#B7410E` | Sooty brick `#4A3A2A` | Cold gray | Industrial dark |
| **Amatsu** | Dark earth `#5A4A3A`, Cherry `#FFB7C5` | Bamboo rock `#4A6A3A` | Warm | Serene Japanese |
| **Yuno** | White marble `#E8E8E0`, Sky blue `#87CEEB` | Light gray `#9A9A90` | Cool bright | Airy academic |
| **Mjolnir** | Gray-brown `#7A6A5A`, Pine `#2A4A2A` | Dark granite `#6A6A6A` | Neutral | Rugged mountain |
| **Moscovia** | Dark earth `#4A3A2A`, Autumn gold `#C09040` | Forest rock `#5A5A4A` | Warm autumn | Rustic fairy tale |

### RO Color Rules

- Greens are **olive/sage** (`#59784F`, `#729762`), never vivid/saturated
- Browns are **warm ochre/sienna**, never cold
- Rocks are **muted purple-gray**, not flat gray
- Everything has a **warm undertone** — even "cold" zones lean warm-gray, not blue-gray
- **No saturated colors anywhere** — the entire palette is muted

---

## 12. Performance Budgets

### Material Instructions

| Material | Max Budget | Current Actual | Room |
|----------|-----------|---------------|------|
| M_Landscape_RO_17 | 250 instructions | ~150 | Plenty |
| M_RO_Original | 100 instructions | ~50 | Plenty |
| Sprite materials | 50 instructions | ~30 | Fine |
| Decal materials | 50 instructions | ~20 | Fine |

### Texture Memory Per Zone

| Category | Budget | Current |
|----------|--------|---------|
| Landscape textures | 40 MB | ~16 MB |
| Decal textures | 10 MB | ~5 MB |
| Props/foliage | 20 MB | Minimal |
| Sprite atlases | 80 MB | ~60 MB |
| **Total per zone** | **150 MB** | **~76 MB** |

### Scatter Performance

| Metric | Target | Notes |
|--------|--------|-------|
| Visible grass instances | < 5,000 | Total across all varieties |
| Triangles per mesh | 4 | Billboard cross = 2 quads |
| Total grass triangles | < 20,000 | 5000 instances x 4 tris |
| Max cull distance | 5,000 UU (50m) | Aggressive for MMO |
| Max density | 8-10 per 10m^2 (grass), 1-3 (objects) | Not 100+ like single-player |

### Decal Performance

| Metric | Target |
|--------|--------|
| Visible decals per camera view | < 100 |
| Shader instructions per decal | ~20 (very light) |
| Total decals placed per zone | 50-100 |

### Material Sampler Budget

Hard limit: 16 texture sampler slots per material. Landscape reserves ~4.

| Current Usage | Samplers |
|--------------|----------|
| GrassWarm + Normal | 2 |
| GrassCool + Normal | 2 |
| Dirt + Normal | 2 |
| Rock + Normal | 2 |
| AO | 1 |
| MacroVariation | 1 |
| **Total** | **10 of 12 available** |

### Overall Frame Budget

| Component | Target GPU Time |
|-----------|----------------|
| Terrain material | 0.5-1.0 ms |
| Decals | < 0.5 ms |
| Scatter (grass) | < 1.0 ms |
| Sprites | < 0.5 ms |
| Post-process | < 1.0 ms |
| Lighting | < 1.0 ms |
| **Total** | **< 5.0 ms (60+ FPS)** |

---

## 13. Step-by-Step: Building a Zone from Nothing

### Phase 1: Landscape Foundation (30 min)

1. Open zone level in UE5
2. **Shift+2** > Landscape Mode > **Create** tab
3. Set Material to `M_Landscape_RO_17`, Section Size 63x63, Components 4x4
4. Click **Create**
5. Switch to **Sculpt** tab:
   - Flatten tool: establish base height for walkable area
   - Sculpt tool: raise hills, lower valleys
   - Make cliff edges >45 degrees at zone boundaries
   - Smooth tool: final pass for organic shapes
   - Noise tool: subtle surface detail on flat areas
6. Switch to **Paint** tab:
   - Create Layer Infos for GrassDense, FlowerPatch, Debris
   - Paint GrassDense on meadow areas
   - Paint FlowerPatch on colorful spots
   - Paint Debris on rocky/dirty areas

### Phase 2: Material Configuration (15 min)

1. Create a new Material Instance from `M_Landscape_RO_17`
2. Or pick an existing variant from `/Materials/Environment/v3/`
3. Override parameters for your zone:
   - Set textures (GrassWarm, GrassCool, Dirt, Rock) from `Textures/Environment/`
   - Set WarmthTint for zone color mood
   - Set SaturationMult (most zones: 0.8-1.0)
   - Tweak GrassVariantBalance and DirtAmount
   - Verify SlopeThreshold = 0.60 for gameplay-aligned rock
4. Drag the Material Instance onto the Landscape Actor

### Phase 3: Decals (20 min)

1. Browse to `/Game/SabriMMO/Materials/Environment/Decals/RO_Decals/`
2. Drag 40-80 decal instances onto the terrain:
   - Dirt patches along paths and worn areas
   - Cracks near cliff bases and stone surfaces
   - Moss in shaded, damp areas
   - Dark stains near entrances and battle spots
3. For each decal:
   - Randomize yaw rotation (E key to rotate)
   - Scale to 1.5-5.0 (R key to scale)
   - Verify pitch is -90 (projects downward)
4. Mix decal types for rich detail. Overlap slightly.

### Phase 4: Scatter Objects (15 min)

1. Open your landscape material in Material Editor
2. Add `LandscapeGrassOutput` node
3. Add entries for zone-appropriate GrassTypes (e.g., GT_V3_Grassland for Prontera)
4. Choose placement method:
   - **Random**: Connect slope Clamp to input pins
   - **Painted**: Connect paint layer finals (grass_dense_final, flower_final, debris_final)
   - **Mixed**: Some on random, some on painted
5. Apply and Save material
6. If using painted: go to Landscape Mode > Paint tab and paint scatter areas

### Phase 5: Post-Process & Lighting (5 min)

1. Add a new zone preset in `PostProcessSubsystem.cpp` (if not already there):
   ```cpp
   else if (ZoneName == TEXT("your_zone"))
   {
       Bloom = 0.35f;
       Vignette = 0.2f;
       ExposureBias = 1.5f;    // outdoor
       WhiteTemp = 6500.f;      // adjust to zone mood
       GainHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
   }
   ```
2. Add lighting config in `SetupSceneLighting()` if not standard outdoor/dungeon
3. Recompile C++

### Phase 6: 3D Assets (Time varies)

1. Generate or import tree/rock/building FBX files
2. Import into UE5 Content Browser
3. Place in level, configure materials (roughness 0.95, no metallic)
4. Verify they don't block sprite visibility at camera angle

### Phase 7: NavMesh Re-Export (5 min)

1. **Build > Build Paths** in UE5
2. Console: `ExportNavMesh <zone_name>`
3. Copy OBJ to `server/navmesh/`
4. Delete server navmesh cache
5. Restart server

### Phase 8: Verification Checklist

- [ ] Player sprite renders correctly (all 9 layers, animation, direction)
- [ ] NO outlines or posterization on sprites
- [ ] Blob shadow circle under player sprite
- [ ] Enemy sprites spawn and pathfind on new terrain
- [ ] Remote player sprites render
- [ ] Combat works (damage numbers, health bars, death)
- [ ] Slope texture matches NavMesh walkability (rock = impassable)
- [ ] Zone color mood feels right
- [ ] No visible tile grid on terrain
- [ ] Grass scatter looks natural, not floating
- [ ] Decals are subtle, not overpowering
- [ ] 60+ FPS in game

---

## 14. Lessons Learned & Pitfalls

### Texture Generation

| # | Lesson | Detail |
|---|--------|--------|
| 1 | **SeamlessTile node is NOT enough alone** | Must post-process with Laplacian pyramid blend. Edge errors of 5-25 remain without it |
| 2 | **Three layers of seam-hiding required** | Laplacian blend + UV distortion + irrational tile ratios. No single technique works alone |
| 3 | **Describe colors, not plants** | "olive green surface" works. "grass" generates blade shapes. This is the #1 prompt mistake |
| 4 | **CFG 6.5 is critical, not 7.0** | Per texture generation testing. 7.0 produces subtly worse textures |
| 5 | **LoRA at 0.15, not 0.3** | Higher causes sprite-like artifacts on ground textures |
| 6 | **60 steps, not 25** | Higher step count produces sharper texture detail |
| 7 | **Use SDXL Base for sprites, Illustrious-XL for ground** | Illustrious fails for isolated objects; SDXL Base fails for stylized textures |
| 8 | **Tile size must match world scale** | 500 UU = 80 tiles across 40000 UU floor = visible grid. Use 4000+ UU |
| 9 | **Normal maps barely visible** | At our top-down camera angle + roughness 0.95, normal maps contribute minimal visible detail |

### Material System

| # | Lesson | Detail |
|---|--------|--------|
| 10 | **Cell bombing creates visible grid** | Voronoi cells are visible. Stick with UV distortion + irrational ratios |
| 11 | **Material versioning: never overwrite** | Always create numbered variants (_01, _02). Compare side by side |
| 12 | **Python can't iterate expressions** | `mat.get_editor_property("expressions")` is protected. Use node creation directly |
| 13 | **Grass Output node must be manual** | Python-added Grass Output causes material compilation failure. Add in editor |
| 14 | **`decal_blend_mode` is deprecated** | In UE5 5.7, use `blend_mode = BLEND_TRANSLUCENT` instead |

### Scatter System

| # | Lesson | Detail |
|---|--------|--------|
| 15 | **Illustrious-XL cannot generate isolated objects** | Always produces abstract chaos. SDXL Base with "product render" prompt is the solution |
| 16 | **"product render" is the magic keyword** | Makes SDXL treat the subject as a product photo on white background |
| 17 | **Bottom-align all sprites** | Without shifting content to bottom of image, sprites float above ground |
| 18 | **Each sprite needs its own mesh copy** | GrassType varieties share material slot 0. Duplicate mesh per sprite |
| 19 | **V1 flat colored geometry looks bad** | Simple colored cones/spheres don't read as real objects from game camera |
| 20 | **Two different V1 mesh scales exist** | First batch ~800 UU, second batch ~0.07 UU. Always check bounds before setting scale |

### Decals

| # | Lesson | Detail |
|---|--------|--------|
| 21 | **AI-generated decal textures look garish** | Use original RO textures — they're already muted and hand-painted |
| 22 | **Opacity above 0.5 looks painted-on** | Keep at 0.25-0.40 for natural ground detail |
| 23 | **Soft edges are mandatory** | Radial gradient falloff + noise edge distortion. Without these, decals show as ugly rectangles |

### Post-Process & Lighting

| # | Lesson | Detail |
|---|--------|--------|
| 24 | **RO Classic does NOT use cel-shading** | Tested and rejected. Causes scene darkening + color washing |
| 25 | **Don't override ColorSaturation** | Washes out sprite colors. Use material WarmthTint parameter instead |
| 26 | **Don't override ColorContrast** | Affects sprite readability. Use material ContrastBoost instead |
| 27 | **AutoExposure speed must be 20.0** | Lower values cause zone transition brightness bleed |
| 28 | **Use PendingZoneName during transitions** | CurrentZoneName is stale during OpenLevel — reads old zone name |

### 3D Assets

| # | Lesson | Detail |
|---|--------|--------|
| 29 | **TRELLIS.2 fails for organic shapes** | Background removal strips green canopy from trees. Use Blender procedural generation instead |
| 30 | **FBX textures don't import reliably** | Drag textures from Content Browser onto meshes manually. UE5 Interchange pipeline drops textures |
| 31 | **Keep roughness 0.95 on everything** | RO Classic is fully diffuse — no specular highlights anywhere |

### General

| # | Lesson | Detail |
|---|--------|--------|
| 32 | **SceneCapture2D must disable post-process materials** | The global cutout material darkens everything without stencil=1. Minimap, portrait captures must call `ShowFlags.SetPostProcessMaterial(false)` |
| 33 | **Substrate materials are overkill** | Pure diffuse terrain has zero use for physical material layering |
| 34 | **Nanite landscape is buggy** | Chunks disappear, displacement breaks, 32GB+ RAM. Avoid until UE5 5.8+ |
| 35 | **No Lumen bounce needed** | Overkill for our warm flat lighting. Direct + ambient fill is sufficient |

---

## Quick Reference Card

### Key Paths
```
Textures:        Content/SabriMMO/Textures/Environment/
Materials:       Content/SabriMMO/Materials/Environment/
Decals:          Content/SabriMMO/Materials/Environment/Decals/
Grass V3:        Content/SabriMMO/Environment/GrassV3/
RO Original:     Content/SabriMMO/Textures/Environment/RO_Original/ (608 textures)
Gen Scripts:     C:/Sabri_MMO/_tools/
UE5 Scripts:     C:/Sabri_MMO/Scripts/Environment/
PostProcess:     client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp
Camera:          client/SabriMMO/Source/SabriMMO/UI/CameraSubsystem.cpp
```

### Material Quick Picks
```
Full-featured:   M_Landscape_RO_17 (production zones)
Simple:          M_Landscape_RO_09 (basic zones)
Lightweight:     M_RO_Original (original RO textures)
```

### Blender Command
```
"C:/Blender 5.1/blender.exe" --background --python C:/Sabri_MMO/_tools/<script>.py
```

### UE5 Python Command
```
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\<script>.py").read())
```
