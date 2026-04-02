# Sabri_MMO — 3D World Building Guide

> How the 3D world is constructed using three layered systems: Landscape, Materials, and Scatter Objects.

---

## Overview

The 3D world in Sabri_MMO is built from three distinct layers that work together:

```
Layer 5: POST-PROCESS (per-zone lighting, fog, color grading)
         ↑ PostProcessSubsystem auto-configures per zone

Layer 4: SCATTER OBJECTS (grass, flowers, rocks, debris)
         ↑ Landscape Grass system projects sprites onto terrain

Layer 3: DECALS (dirt patches, cracks, moss, paths)
         ↑ DBuffer decals projected onto terrain surface

Layer 2: MATERIALS (ground textures, slope blending, color processing)
         ↑ Material Instances with per-zone parameters

Layer 1: LANDSCAPE (terrain geometry — hills, valleys, cliffs)
         ↑ UE5 Landscape Actor sculpted per zone
```

Each layer adds visual richness without depending on the others — you can have just Layer 1 and it looks good, but all three together creates the full RO Classic look.

---

## Quick Reference

| I want to... | Use this | Guide |
|--------------|---------|-------|
| Create terrain with hills and cliffs | Landscape Actor | [01_Landscape_Guide.md](01_Landscape_Guide.md) |
| Change ground texture / zone color | Material Instance variants | [02_Materials_Guide.md](02_Materials_Guide.md) |
| Add grass, flowers, debris to terrain | Landscape Grass system | [03_Scatter_Objects_Guide.md](03_Scatter_Objects_Guide.md) |
| Add dirt patches, cracks, paths | DBuffer Decals | [04_Decals_Guide.md](04_Decals_Guide.md) |
| Change zone lighting, fog, mood | PostProcessSubsystem | [05_Lighting_PostProcess_Guide.md](05_Lighting_PostProcess_Guide.md) |

---

## How They Connect

1. **Build terrain** → UE5 Landscape Actor, sculpt hills/cliffs
2. **Apply material** → M_Landscape_RO_17 auto-blends grass/rock by slope
3. **Add decals** → drag MI_RODecal_* for ground detail variety
4. **Add grass** → connect GT_V3_* to Grass Output, paint or randomize
5. **Polish** → adjust material parameters per zone (warmth, saturation, dirt)
