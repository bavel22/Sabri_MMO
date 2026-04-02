# Guide 3: Scatter Objects (Grass, Flowers, Debris)

> How 2D sprite and 3D objects are scattered across the landscape using the Landscape Grass system.

---

## Overview

The Landscape Grass system auto-spawns small static meshes (grass blades, flowers, mushrooms, rocks, etc.) across the terrain based on material layer weights. Each mesh shows an AI-generated sprite texture with transparent background.

```
AI Sprite Image (ComfyUI SDXL)
  → White-to-alpha conversion
  → Bottom-align
  → Apply to billboard cross mesh as alpha-masked material
  → Register in LandscapeGrassType asset
  → Connect to Grass Output node in landscape material
  → UE5 auto-spawns instances across the landscape
```

---

## Three Generations

| Version | Location | Approach | Quality |
|---------|----------|----------|---------|
| V1 | `/Environment/Grass/` | Blender geometry + flat colors | Basic — colored shapes |
| V2 | `/Environment/GrassV2/` | Billboard + AI sprite proof-of-concept | Good — proved approach |
| **V3** | `/Environment/GrassV3/` | **Production** — 60 sprites, 13 zones | **Best — use this** |

---

## V3 System (Production)

### 60 Sprite Types Across 13 Zones

| Zone | Objects | Examples |
|------|---------|---------|
| **Grassland** | 11 | Grass clumps, flowers, dandelion, clover |
| **Forest** | 13 | Ferns, mushrooms, autumn leaves, moss, pinecone, twig |
| **Desert** | 5 | Dry grass, cactus, desert rocks, dead branches |
| **Snow** | 3 | Snow clumps, ice crystals, frozen plants |
| **Beach** | 5 | Seashells, starfish, coral, seaweed |
| **Volcanic** | 3 | Lava rock, obsidian, sulfur crystals |
| **Cursed** | 4 | Bones, skulls, dead grass, candle stubs |
| **Amatsu** | 5 | Cherry petals, bamboo leaves, grass, flowers |
| **Moscovia** | 6 | Birch leaves, pinecones, berries, ferns |
| **Ruins** | 3 | Stone fragments, pottery shards |
| **Industrial** | 2 | Metal scraps, gear pieces |
| **Cave** | 3 | Blue/green crystals, stalagmites |
| **Dungeon** | 3 | Chain pieces, rubble, bones |

### How Each Sprite Is Made

1. **Generate** with SDXL Base: `"ONE single [object], product render, isolated on white background"`
2. **Alpha convert**: Whiteness > 200 → transparent, < 150 → opaque
3. **Bottom-align**: Shift content to bottom of image (prevents floating)
4. **Quality check**: Auto-reject if too transparent (>95%) or not enough (<15%)
5. **Auto-retry** up to 3 times with different seeds if quality fails

---

## Two Placement Methods

### Method A: Random (Uniform)

Grass appears everywhere on flat terrain automatically.

**Setup in Material Editor:**
1. Open landscape material
2. Add **Grass Output** node
3. Add GT_V3_* entries
4. Connect **slope Clamp** node to each input pin
5. Apply + Save

**Result:** Uniform scatter across all flat areas. Quick to set up.

### Method B: Painted (Manual Clumps)

You paint exactly where grass appears using a brush.

**Setup:**
1. Use **M_Landscape_RO_17** (has paint layers built in)
2. Open in Material Editor
3. Add **Grass Output** node
4. Connect **grass_dense_final** → grass varieties
5. Connect **flower_final** → flower varieties
6. Connect **debris_final** → debris varieties
7. Apply + Save
8. Assign material to Landscape
9. **Shift+2** → Landscape Mode → **Paint** tab
10. Click **+** next to each layer → **Create Layer Info** → save
11. Select layer, paint with left-click
12. **[ ]** keys to resize brush

**Result:** Grass only where you paint. Full artistic control.

### Combining Both

Some GrassType entries on random (always present), others on painted (clumps):
- Basic grass → slope Clamp (always on flat ground)
- Flower clusters → flower_final (only where painted)
- Mushroom patches → debris_final (only where painted)

---

## Adjusting Scatter Properties

Double-click any `GT_V3_*` in Content Browser:

| Property | What it changes |
|----------|----------------|
| **Scale X (Min/Max)** | Object size range (0.5-2.0 for V3) |
| **Grass Density** | How many per area |
| **Start/End Cull Distance** | Where they fade in/out |
| **Random Rotation** | Random yaw per instance |
| **Align to Surface** | Tilt to match terrain slope |

After changing: re-apply the landscape material (open, Apply, Save) to refresh.

---

## Adding New Scatter Objects

### Generate New Sprites
1. Add entry to `_tools/generate_all_sprites_v3.py`
2. Run: `C:/ComfyUI/venv/Scripts/python.exe _tools/generate_all_sprites_v3.py`
3. Copy new PNG to `Content/SabriMMO/Environment/GrassV3/Textures/`

### Import + Setup in UE5
```
exec(open(r"C:\Sabri_MMO\client\SabriMMO\Scripts\Environment\setup_grass_v3_fixed.py").read())
```

### Add to Grass Output
1. Open landscape material in Material Editor
2. Add new entry in Grass Output node
3. Assign the new GT_V3_* GrassType
4. Connect to slope mask or paint layer
5. Apply + Save

---

## Performance Budget

| Metric | Target |
|--------|--------|
| Visible instances | < 5,000 |
| Triangles per mesh | 4 (billboard cross) |
| Max cull distance | 5,000 UU (50m) |
| Max density per type | 8-10/10m^2 |
| Texture memory | ~15 MB (60 sprites BC3 compressed) |

---

## Sprite Generation Rules

- **Use SDXL Base** (`sd_xl_base_1.0.safetensors`) — NOT Illustrious-XL
- **Prompt**: `"ONE single [object], product render, game asset, isolated on white background, centered, nothing else"`
- **Negative**: must include `multiple objects, tiling, repeating, frame, border`
- **Settings**: CFG 7.0, 30 steps, euler, 512x512
- **Alpha**: threshold 200, ramp over 50
- **Bottom-align**: shift content to bottom of image
- **Quality check**: reject >95% transparent OR <15% transparent
