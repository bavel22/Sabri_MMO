# Sabri_MMO — 3D World Building Guide

> How the 3D world is constructed using three layered systems: Landscape, Materials, and Scatter Objects.

---

## Overview

The 3D world in Sabri_MMO is built from three distinct layers that work together:

```
Layer 3: SCATTER OBJECTS (grass, flowers, rocks, debris)
         ↑ Landscape Grass system projects sprites onto terrain

Layer 2: DECALS (dirt patches, cracks, moss, paths)
         ↑ DBuffer decals projected onto terrain surface

Layer 1: LANDSCAPE + MATERIALS (terrain geometry + ground textures)
         ↑ UE5 Landscape Actor with auto-blending material
```

Each layer adds visual richness without depending on the others — you can have just Layer 1 and it looks good, but all three together creates the full RO Classic look.

---

## Quick Reference

| I want to... | Use this | Guide |
|--------------|---------|-------|
| Create terrain with hills and cliffs | Landscape Actor | [01_Landscape_Guide.md](01_Landscape_Guide.md) |
| Change ground texture / zone color | Material Instance variants | [02_Materials_Guide.md](02_Materials_Guide.md) |
| Add grass, flowers, debris to terrain | Landscape Grass system | [03_Scatter_Objects_Guide.md](03_Scatter_Objects_Guide.md) |
| Add dirt patches, cracks, paths | DBuffer Decals | [02_Materials_Guide.md](02_Materials_Guide.md#decals) |

---

## How They Connect

1. **Build terrain** → UE5 Landscape Actor, sculpt hills/cliffs
2. **Apply material** → M_Landscape_RO_17 auto-blends grass/rock by slope
3. **Add decals** → drag MI_RODecal_* for ground detail variety
4. **Add grass** → connect GT_V3_* to Grass Output, paint or randomize
5. **Polish** → adjust material parameters per zone (warmth, saturation, dirt)
