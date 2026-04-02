# Ground Texture & Material System — Complete Inventory

> Last updated: 2026-03-31
> Reference: `docsNew/05_Development/Ground_Texture_System_Research.md`

---

## Texture Inventory

### Generated Textures (AI via ComfyUI)

| Folder | Count | Resolution | Description |
|--------|-------|-----------|-------------|
| `Ground/` | 43 | 1024x1024 | First-generation ground textures — grass, dirt, rock, cobble, sand, snow, dungeon |
| `Ground/Normals/` | 43 | 1024x1024 | BAE-generated normal maps from diffuse |
| `Ground/Depth/` | 43 | 1024x1024 | MiDaS depth maps from diffuse |
| `Ground/AO/` | 43 | 1024x1024 | Ambient occlusion derived from inverted depth |
| `Ground_2K/` | 18 | 2048x2048 | 4x-UltraSharp upscaled best textures |
| `Ground_2K/Normals/` | 18 | 2048x2048 | Normal maps for 2K textures |
| `Ground_2K/Depth/` | 18 | 2048x2048 | Depth maps for 2K textures |
| `Ground_2K/AO/` | 18 | 2048x2048 | AO maps for 2K textures |
| `Ground_Seamless/` | 18 | 2048x2048 | Laplacian pyramid seamless-fixed 2K textures |
| `Biomes/` (15 subfolders) | 94 | 1024x1024 | Generic biome textures (grassland, forest, desert, snow, volcanic, swamp, beach, mountain, dungeon, cave, jungle, urban, cursed, ruins, industrial) |
| `ROZones/` (16 subfolders) | 96 | 1024x1024 | RO Classic zone-specific textures (prontera, geffen, payon, morroc, lutie, glast_heim, niflheim, veins, amatsu, jawaii, einbroch, yuno, mjolnir, moscovia, comodo, umbala, rachel, louyang, pyramid) |
| `ROClassic/` | 8 | 1024x1024 | Hand-tuned RO Classic reference-matching (warm yellow-green, dark green, brown-green, dark cliff) |
| `Sand/` | 10 | 1024x1024 | Sandy/desert textures (sand warm, sand cool, dry earth, sandstone cliff) |

### Original RO Textures (Downloaded from bghq.com)

| Folder | Count | Resolution | Description |
|--------|-------|-----------|-------------|
| `RO_Original/` | 608 | ~256x256 | Actual hand-painted RO Classic ground textures extracted from game data. Includes grass, stone, cobblestone, sand, snow, dungeon floors, walls, paths — every ground texture type from the original game. |

### Total Texture Count: ~1,061

---

## Material System

### Parent Materials

| Material | Location | Parameters | Purpose |
|----------|----------|-----------|---------|
| **M_Landscape_RO_12** | `/Game/SabriMMO/Materials/Environment/` | 23 | Full-featured landscape material — 4-layer noise blending, slope detection, UV distortion, color processing. Master parent for v1-v3 Material Instances. |
| **M_Landscape_RO_09** | `/Game/SabriMMO/Materials/Environment/` | 13 | Original "best" visual — hard 45-deg slope cutoff, original grass/dirt balance. Parent for early v1 variants. |
| **M_RO_Original** | `/Game/SabriMMO/Materials/Environment/` | 8 | Lightweight material for original RO textures — preserves hand-painted character with minimal processing. Single ground + cliff texture, subtle macro variation, 45-deg slope cutoff. |

### M_Landscape_RO_12 — 23 Parameters

| Category | Parameter | Type | Default | Range | Effect |
|----------|-----------|------|---------|-------|--------|
| **Textures** | GrassWarmTexture | Texture2D | — | — | Primary flat ground texture |
| | GrassCoolTexture | Texture2D | — | — | Secondary variant (noise-blended) |
| | DirtTexture | Texture2D | — | — | Dirt/transition patches |
| | RockTexture | Texture2D | — | — | Cliff/slope texture (XZ projected) |
| | GrassWarmNormal | Texture2D | — | — | Normal map for primary ground |
| | GrassCoolNormal | Texture2D | — | — | Normal map for secondary ground |
| | DirtNormal | Texture2D | — | — | Normal map for dirt |
| | RockNormal | Texture2D | — | — | Normal map for cliff |
| | GrassAO | Texture2D | — | — | Ambient occlusion for grass |
| **Color** | WarmthTint | Vector | (1,0.98,0.94) | warm/cool | Color temperature shift |
| | SaturationMult | Scalar | 1.0 | 0.3-1.2 | Desaturation control |
| | BrightnessOffset | Scalar | 0.0 | -0.2 to 0.3 | Overall brightness shift |
| | ContrastBoost | Scalar | 1.0 | 0.8-1.5 | Contrast power curve |
| **Blending** | GrassVariantBalance | Scalar | 0.5 | 0-1 | Bias ground A vs B |
| | GrassNoiseScale | Scalar | 0.004 | 0.001-0.01 | Splotchy patch size |
| | MacroNoiseScale | Scalar | 0.001 | 0.0003-0.003 | Brightness variation frequency |
| | DirtOnSlopes | Scalar | 0.0 | 0-1 | Dirt in grass-to-rock transition |
| **Slope** | SlopeThreshold | Scalar | 0.60 | 0.45-0.75 | Where rock begins |
| | SlopeTransitionWidth | Scalar | 0.15 | 0.08-0.22 | Transition sharpness |
| | SlopeNoiseAmount | Scalar | 0.0 | 0-0.15 | Wavy/organic slope line |
| | SlopeNoiseFreq | Scalar | 0.01 | 0.005-0.02 | Slope noise jaggedness |
| **Surface** | DirtAmount | Scalar | 0.25 | 0-0.5 | Dirt on flat ground |
| | Roughness | Scalar | 0.95 | 0.65-0.98 | Surface shininess |
| | NormalStrength | Scalar | 0.5 | 0-1 | Bump detail intensity |
| | AOStrength | Scalar | 0.4 | 0-1 | Crevice darkening |
| | UVDistortStrength | Scalar | 60 | 20-100 | Seam line warping |
| | GrassWarmTileSize | Scalar | 4000 | 2000-7000 | Primary ground tile size |
| | GrassCoolTileSize | Scalar | 5173 | 3000-8000 | Secondary ground tile size |
| | DirtTileSize | Scalar | 3347 | 2000-6000 | Dirt tile size |
| | RockTileSize | Scalar | 2891 | 1500-5000 | Rock tile size |

### M_RO_Original — 8 Parameters

| Parameter | Type | Default | Effect |
|-----------|------|---------|--------|
| GroundTexture | Texture2D | — | The RO original texture for flat areas |
| CliffTexture | Texture2D | — | Texture for steep slopes (can be same as ground) |
| TileSize | Scalar | 4000 | Ground texture tile size in UU |
| CliffTileSize | Scalar | 3000 | Cliff texture tile size |
| MacroStrength | Scalar | 0.08 | Subtle brightness variation (0 = none, 0.15 = strong) |
| UVDistortStrength | Scalar | 40 | Seam line warping |
| SlopeThreshold | Scalar | 0.60 | Where cliff texture begins |
| SlopeTransitionWidth | Scalar | 0.15 | Transition sharpness |

---

## Material Instance Variants

| Folder | Count | Parent | Description |
|--------|-------|--------|-------------|
| `Variants/` | ~30 | M_Landscape_RO_09 | v1 — Early variants, textures only, 10 green + 10 sand + 10 mixed |
| `BiomeVariants/` | ~915 | M_Landscape_RO_11 | v2 — 15 biomes x 30 + cross-biome pairs, textures + basic params |
| `ROClassic/` | ~200 | M_Landscape_RO_11 | RO Classic warm yellow-green variants |
| `v3/` | ~920 | M_Landscape_RO_12 | v3 — 20 RO zones x 30 + 40 cross-biome x 8, full 23-param variation |
| `v4/individual/` | 607 | M_RO_Original | Each original RO texture as its own simple material |
| `v4/paired/` | 4+ | M_RO_Original | Ground + different cliff texture combos |
| `v4/tile_sizes/` | varies | M_RO_Original | First 50 textures at different scales |

### Total Material Instance Count: ~2,700+

---

## Material Version History

| Version | Name | Key Feature | Parameters |
|---------|------|------------|-----------|
| v00-v03 | M_Landscape_RO (rebuilt) | Iterating on tiling, UV distortion, seamless textures | 0-13 |
| v04-v08 | M_Landscape_RO_04 through _08 | Slope power, dirt amount, XZ cliff projection | 13 |
| **v09** | **M_Landscape_RO_09** | **Hard 45-deg cutoff — best visual, original balance** | 13 |
| v10-v11 | M_Landscape_RO_10, _11 | All params exposed as MI-overridable (tile sizes, slope, UV) | 17 |
| **v12** | **M_Landscape_RO_12** | **Master parent — color processing, blending control, slope noise** | **23** |

---

## Slope Detection Rule (Gameplay)

| Angle | DotProduct | Material Result | Gameplay |
|-------|-----------|----------------|----------|
| 0-40 degrees | 0.77-1.0 | 100% ground texture | **Walkable** |
| 40-53 degrees | 0.60-0.77 | Ground-to-cliff transition | Edge of walkable area |
| 53-90 degrees | 0.0-0.60 | 100% cliff texture | **Unclimbable** — visual barrier |

Rock/cliff texture = impassable terrain. Matches NavMesh exclusion zones.
