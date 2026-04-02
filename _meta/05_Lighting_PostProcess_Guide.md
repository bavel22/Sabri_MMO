# Guide 5: Lighting & Post-Process

> How per-zone lighting, color grading, fog, and exposure are handled automatically by the PostProcessSubsystem.

---

## Overview

Each zone has a distinct visual mood — Prontera is warm and bright, Lutie is cold and snowy, Glast Heim is dark and desaturated. This is handled by two systems:

1. **PostProcessSubsystem** (C++) — auto-spawns/configures lights and post-process per zone
2. **Material color parameters** — WarmthTint, SaturationMult, BrightnessOffset per material instance

---

## PostProcessSubsystem

File: `client/SabriMMO/Source/SabriMMO/UI/PostProcessSubsystem.cpp`

Automatically spawns and configures per zone:
- **DirectionalLight** (sun) — intensity, color, angle
- **SkyLight** (ambient fill) — intensity, color
- **ExponentialHeightFog** — density, color
- **PostProcessVolume** — bloom, vignette, exposure, color grading

### Outdoor Zone Lighting
| Component | Settings |
|-----------|---------|
| Sun | 3.14 lux, color (1.0, 0.95, 0.85) warm, pitch -50 deg |
| Sky | 2.5 intensity, color (0.95, 0.92, 0.85) warm |
| Fog | Density 0.004, warm inscattering (0.7, 0.65, 0.5) |

### Dungeon Zone Lighting
| Component | Settings |
|-----------|---------|
| Sun | Disabled (intensity 0) |
| Sky | 0.3 intensity, cool blue (0.4, 0.4, 0.6) |
| Fog | Density 0.03, dark purple inscattering (0.1, 0.08, 0.15) |

### Per-Zone Post-Process Presets

| Zone | Bloom | Vignette | Exposure Bias | White Temp | Color Gain |
|------|-------|----------|---------------|-----------|------------|
| Prontera | 0.4 | 0.25 | 1.5 | 6800K (warm) | Warm |
| Prontera South | 0.35 | 0.2 | 1.5 | 6500K (neutral) | Neutral |
| Prontera North | 0.35 | 0.2 | 1.5 | 6200K (cool) | Cool |
| Dungeon | 0.2 | 0.4 | 0.0 | 5000K (cool) | Blue |

---

## Material-Level Zone Mood (M_Landscape_RO_17)

These parameters override per Material Instance to shift the entire terrain color:

| Parameter | Effect | Example Values |
|-----------|--------|---------------|
| **WarmthTint** | Color multiply on terrain | Prontera: (1.02, 0.99, 0.94), Rachel: (0.92, 0.96, 1.04) |
| **SaturationMult** | Desaturation | Niflheim: 0.35, Jawaii: 1.15 |
| **BrightnessOffset** | Overall brightness | Dungeon: -0.15, Snow: +0.20 |
| **ContrastBoost** | Contrast curve | Volcanic: 1.4, Snow: 0.9 |

These affect ONLY terrain — sprites are unaffected (they have their own pre-baked shading).

---

## Zone Name Resolution During Transitions

During zone transitions, `CurrentZoneName` is stale. The subsystem uses:
```cpp
FString ZoneName = GI->bIsZoneTransitioning
    ? GI->PendingZoneName
    : GI->CurrentZoneName;
```

Auto-exposure speed is set to 20.0 (near-instant) to prevent brightness bleed between zones.

---

## What NOT To Do

- **No cel-shading post-process** — tested and caused scene darkening + color washing
- **No ColorSaturation/ColorContrast overrides** — affects sprites, use material parameters instead
- **No post-process posterization** — would affect sprites. Do posterization in terrain material only
- **Keep roughness 0.95** on terrain — no PBR specular, RO is fully diffuse

---

## RO Classic Visual Style (Research Finding)

RO's look comes from 6 specific techniques, NONE of which are screen-space effects:
1. Posterized 8x8 lightmaps (terrain-only, not post-process)
2. No outlines on 3D geometry
3. 15-degree vertical FOV (we use ~90 for playability)
4. Warm flat lighting
5. Diffuse-only materials (roughness ~1.0)
6. sRGB-direct rendering

Full research: `docsNew/05_Development/RO_Classic_Visual_Style_Research.md`

---

## Adding a New Zone Preset

In `PostProcessSubsystem.cpp`, add an `else if` block in `ApplyZonePreset()`:

```cpp
else if (ZoneName == TEXT("new_zone"))
{
    Bloom = 0.35f;
    Vignette = 0.2f;
    ExposureBias = 1.5f;      // outdoor: 1.5, dungeon: 0.0
    WhiteTemp = 6500.f;        // 5000=cool, 6500=neutral, 6800=warm
    GainHighlights = FVector4(1.0f, 1.0f, 1.0f, 1.0f);
}
```

Also add lighting config in `SetupSceneLighting()` if zone type differs.
