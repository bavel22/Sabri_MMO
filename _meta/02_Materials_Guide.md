# Guide 2: Materials, Textures & Decals

> How ground textures, materials, material variants, and decals work together.

---

## Material System Architecture

```
M_Landscape_RO_17 (Master Material — 23+ parameters)
  ↓ inherits
MI_variant_name (Material Instance — overrides specific parameters)
  ↓ applied to
Landscape Actor (shows the ground texture)
```

One master material, many instances. Each zone uses a different instance with different textures, warmth, saturation, and slope settings.

---

## The Master Material (M_Landscape_RO_17)

Built via Python script: `Scripts/Environment/create_landscape_material.py`

### What It Does Automatically
- **4 texture layers** noise-blended together (ground warm, ground cool, dirt, cliff)
- **Slope detection** — rock appears on slopes >45 degrees (gameplay indicator: unclimbable)
- **UV noise distortion** — warps tile seam lines into wavy curves
- **Irrational tile ratios** — each layer tiles at different size so seams never align
- **Macro variation** — large-scale brightness patches break visible repetition
- **Color processing** — per-zone warmth, saturation, brightness, contrast

### Key Parameters

| Parameter | What it changes | Example |
|-----------|----------------|---------|
| GrassWarmTexture | Primary ground texture | Green meadow for Prontera |
| GrassCoolTexture | Secondary (noise-blended) | Darker green for variety |
| RockTexture | Cliff/slope surface | Purple-gray stone |
| WarmthTint | Color temperature | Warm gold (Prontera) vs cold blue (Rachel) |
| SaturationMult | Color intensity | Near-grayscale (Niflheim) vs vivid (Jawaii) |
| SlopeThreshold | Where rock starts | 0.60 = ~53 degrees |
| DirtAmount | Dirt patches on flat ground | 0.0 = none, 0.25 = moderate |
| GrassVariantBalance | Which ground texture dominates | 0.2 = mostly A, 0.8 = mostly B |

---

## Texture Sources

### AI-Generated (ComfyUI)
- **1000+ textures** in `Ground/`, `Ground_2K/`, `Biomes/`, `ROZones/`
- Generated with Illustrious-XL + SeamlessTile
- Post-processed: Laplacian seamless blend, desaturation, warm shift
- Prompt rule: describe COLORS not plants ("olive green surface" not "grass")

### Original RO (bghq.com)
- **608 textures** in `RO_Original/`
- Actual hand-painted game textures extracted from RO data
- Best for decals and authentic RO zone materials

---

## Material Variants

Pre-made Material Instances organized by zone:

| Folder | Count | Parent | What |
|--------|-------|--------|------|
| `v3/` | ~920 | M_Landscape_RO_12 | 20 RO zones + cross-biome, full 23-param variation |
| `v4/` | ~610 | M_RO_Original | Original RO textures, simple material |
| `ROClassic/` | ~200 | M_Landscape_RO_11 | Warm yellow-green RO classic look |
| `BiomeVariants/` | ~915 | M_Landscape_RO_11 | 15 generic biomes |
| `Variants/` | ~30 | M_Landscape_RO_09 | Early variants |

**To use:** Browse Content Browser → drag any MI_* onto landscape.

---

## Decals {#decals}

DBuffer decals project textures onto terrain for ground variety (dirt patches, cracks, moss, paths).

### Decal Materials
5 parent materials in `/Materials/Environment/Decals/`:
- **M_Decal_Dirt** — warm brown worn patches
- **M_Decal_Cracks** — dark gray fracture lines
- **M_Decal_Moss** — dark green vegetation
- **M_Decal_Path** — sandy worn footpaths
- **M_Decal_DarkStain** — dark stains, blood, shadows

Each has: soft radial falloff + noise edge distortion (no hard rectangles).

### Decal Instances
- **91 instances** in `/Decals/RO_Decals/` using original RO textures
- Tint: warm brown (0.85, 0.80, 0.72), opacity: 0.35
- AI-generated decals looked bad — always use RO originals

### Placing Decals
1. **Drag from Content Browser** — fastest method
2. **Ctrl+D** to duplicate existing decals
3. **E** to rotate, **R** to scale
4. Pitch = -90 to project downward onto ground
5. Budget: <100 visible per camera view

---

## Creating a New Zone Material

1. Pick a parent material (M_Landscape_RO_17 for full features)
2. Create a Material Instance from it
3. Override textures: pick from any texture folder
4. Adjust warmth/saturation to match zone mood
5. Apply to the zone's Landscape Actor
6. Add decals for ground detail
7. Connect GrassTypes for foliage scatter

---

## Texture Generation (for new textures)

```bash
# Start ComfyUI
cd C:/ComfyUI && python main.py

# Generate textures
C:/ComfyUI/venv/Scripts/python.exe _tools/generate_ground_textures_v2.py

# Or zone-specific
C:/ComfyUI/venv/Scripts/python.exe _tools/generate_ro_zones.py

# Post-process for seamless tiling
C:/ComfyUI/venv/Scripts/python.exe _tools/fix_seamless_aggressive.py
```

Settings: SDXL checkpoint, LoRA 0.15, CFG 6.5, 60 steps, SeamlessTile + CircularVAEDecode.
