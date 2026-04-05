# Research Report: Recreating the Ragnarok Online Classic Visual Style in UE5

## Executive Summary

Ragnarok Online's distinctive look comes from **six specific technical choices**, not generic cel-shading. The style is defined by: (1) posterized 8x8 lightmaps with 16-level color quantization on terrain, (2) hand-painted low-poly 3D with NO outlines on geometry, (3) pre-baked ambient occlusion shadows, (4) a very narrow 15-degree FOV creating telephoto compression, (5) warm diffuse lighting with sRGB-direct rendering (no gamma correction), and (6) per-map fog with a 1500-unit far plane. **RO does NOT use cel-shading outlines on 3D geometry** — the posterized look comes entirely from lightmap quantization, not a shader effect.

**Confidence Level**: High — based on roBrowser source code analysis, Ragnarok Research Lab specifications, GND/RSW file format documentation, and community technical breakdowns.

---

## Key Finding 1: RO's "Look" is Lightmap Posterization, NOT Cel-Shading

**Confidence:** High
**Sources:** [Ragnarok Research Lab - Ground Mesh](https://ragnarokresearchlab.github.io/rendering/ground-mesh/), [GND Format Spec](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GND.MD)

The single most distinctive visual feature of RO is the **posterized lightmap** applied to all terrain. This is NOT a shader effect — it's baked into the lightmap textures at map creation time.

### The Algorithm

```
color_component = floor(color_component / 16) * 16
```

This reduces each color channel from 256 levels to 16 levels, creating the characteristic "stepped" shadow transitions on the ground. The posterization applies to the **lightmap only**, not to diffuse textures or 3D models.

### Lightmap Structure

- Each lightmap slice is **8x8 pixels** — intentionally low resolution
- Two channels: **colormap** (RGB lighting) and **shadowmap** (greyscale ambient occlusion)
- The shadowmap is applied AFTER lighting calculation (prevents washed-out shadows)
- One-pixel buffer zone around each slice prevents texture bleeding with linear filtering
- The final look: coarse 8x8 light/shadow patches with hard 16-level color steps

### UE5 Translation

**DO NOT use a post-process cel-shade effect on the whole scene.** Instead:
- Bake lightmaps onto terrain/buildings using UE5's built-in lightmap baker (even with Lumen, you can use baked lightmaps on static meshes)
- OR: Use a **material-level** color quantization on the lightmap/shadow contribution only (not on the base color texture)
- The quantization targets the LIGHTING, not the full scene color
- Keep base color textures smooth — only quantize the light-to-dark transitions

---

## Key Finding 2: NO Outlines on 3D Geometry

**Confidence:** High
**Sources:** [Ragnarok Research Lab - Rendering Overview](https://ragnarokresearchlab.github.io/rendering/), [roBrowser source](https://github.com/vthibault/roBrowser)

RO does NOT draw outlines (Sobel, inverted hull, or otherwise) on any 3D geometry. The clean edges come from:
- **Hand-painted textures** with clear color boundaries baked into the art
- **Low polygon counts** creating naturally sharp silhouettes
- **High color contrast** between adjacent surfaces
- The **posterized lightmaps** creating hard shadow edges that look like outlines

The only "outlines" in RO are on the **2D sprites** themselves (the sprites have painted dark edges in their pixel art).

### UE5 Translation

**Remove the Sobel outline post-process.** It's not part of the RO look. Instead:
- Rely on hand-painted textures with clear edge definition
- Use high-contrast adjacent materials (dark stone next to light grass)
- The posterized lighting on terrain creates natural edge definition

---

## Key Finding 3: The Camera Creates the Look (15° FOV)

**Confidence:** High
**Sources:** [Ragnarok Research Lab - Camera Controls](https://ragnarokresearchlab.github.io/rendering/camera-controls/)

RO uses an **extremely narrow 15-degree vertical FOV** — this is ~4x narrower than typical 3D games (60°). This creates:
- **Telephoto compression**: Objects at different distances appear similar in size (flat, diorama-like)
- **Minimal perspective distortion**: Straight lines stay straight, buildings don't taper
- **Near-orthographic appearance**: The world looks almost isometric despite being perspective
- Combined with the orbital camera at distance 200-250 UU, this creates the distinctive "looking at a miniature" feel

### Camera Parameters
| Parameter | RO Value |
|-----------|----------|
| Vertical FOV | 15° |
| Near plane | 10 world units |
| Far plane | 1500 world units |
| Min zoom | 200-250 world units |
| Max zoom | ~400-750 world units |
| Horizontal rotation | ~1°/pixel |
| Vertical rotation | 5°/scroll step |

### UE5 Translation (Current vs RO)

Your current camera: **-55° pitch, 1200 arm length, default FOV (~90°)**

To match RO: Consider reducing FOV significantly. However, a 15° FOV at 1200 UU distance would show very little of the world. The compromise is:
- **FOV 30-40°** (narrower than default but playable) — gives some telephoto compression
- Keep the -55° pitch (close to RO's typical viewing angle)
- The narrow FOV is a major part of why RO looks like RO — the flatter perspective makes sprites blend into the 3D world

---

## Key Finding 4: Lighting is Simple and Warm

**Confidence:** High
**Sources:** [Ragnarok Research Lab - Scene Lighting](https://ragnarokresearchlab.github.io/rendering/scene-lighting/), [RSW Format](https://ragnarokresearchlab.github.io/file-formats/rsw/)

### Directional Light
- Direction calculated from **longitude/latitude** in spherical coordinates
- Starts at (0, -1, 0), rotated by latitude around X, then longitude around Y
- **Diffuse color**: Per-map RGB floats (typically warm whites: ~(1.0, 0.95, 0.85))
- Sun direction is roughly overhead to slightly angled — never dramatic

### Ambient Light
- Global fill light, separate RGB components
- When lightmaps disabled: ambient amplified by **1.5x** to compensate
- Combined with directional via **screen blending** (prevents underexposure)

### No Gamma Correction
- sRGB textures used directly without linearization
- Screen blending compensates for the resulting darkness
- This gives a slightly warmer, softer look than physically-correct rendering

### UE5 Translation

| RO Lighting | UE5 Equivalent |
|-------------|---------------|
| Directional light (warm white) | Directional Light: Intensity 3.14 lux, Temperature 5500-6500K, slight warm tint |
| Ambient light (screen blend) | Sky Light: Intensity 1.0-2.0, Cubemap or real-time capture |
| No gamma correction | Consider using sRGB workflow OR add slight warmth via color grading |
| Baked lightmaps (posterized) | Baked lightmaps on static geometry OR material-level quantization |

**Key insight**: RO's lighting is intentionally FLAT and WARM. Avoid dramatic shadows, high contrast, or cool color temperatures. The world should feel evenly lit with soft, warm ambient fill.

---

## Key Finding 5: Fog and Atmosphere

**Confidence:** High
**Sources:** [Ragnarok Research Lab - Scene Lighting](https://ragnarokresearchlab.github.io/rendering/scene-lighting/)

### Fog System
- Per-map fog configured in `fogParameterTable.txt`
- Format: `map_id#nearLimit%#farLimit%#fogColorARGB#density%#`
- Fog sphere at 1500 world units (just inside far plane)
- Example (Payon Cave): near 10%, far 90%, color rgb(4,0,154) deep blue, density 30%
- Example (outdoor fields): typically very light fog or no fog

### Key Characteristic
- The **narrow 15° FOV** means fog affects the scene differently — distant objects are more compressed, so fog is more visible
- Indoor/dungeon maps use colored fog for atmosphere (blue for caves, warm for deserts)
- Outdoor maps often have minimal or no fog

### UE5 Translation

| Zone Type | Fog Settings |
|-----------|-------------|
| Outdoor town | Exponential Height Fog: density 0.002-0.005, very light, warm inscattering |
| Outdoor field | Exponential Height Fog: density 0.003-0.008, subtle, neutral |
| Dungeon/cave | Exponential Height Fog: density 0.02-0.05, colored (blue/purple), heavy |

---

## Key Finding 6: Texture and Material Style

**Confidence:** High
**Sources:** [GND Format](https://ragnarokresearchlab.github.io/file-formats/gnd/), [jMonkeyEngine forum](https://hub.jmonkeyengine.org/t/ragnarok-style-graphics/19555)

### Texture Characteristics
- **256-color BMP files** — deliberately limited palette
- **16-bit per pixel color depth** — causes visible color banding (adds to the style)
- **Hand-painted** in a simple, illustrative style — NOT photorealistic
- Tiling textures with clear repeating patterns (intentional, part of the charm)
- **Magenta (254, 0, 254)** used as transparency key

### Material Properties
- **No specularity** — all surfaces are matte/diffuse only
- **No normal maps** — geometry defines all surface detail
- **No metallic** — everything is non-metallic
- **High roughness equivalent** — purely diffuse reflection model

### UE5 Translation

| RO Material | UE5 Material |
|-------------|-------------|
| Diffuse only, no specular | Roughness: 0.95-1.0 (near fully rough) |
| No normal maps | Normal Map: flat (skip entirely) |
| No metallic | Metallic: 0.0 |
| Hand-painted 256-color BMP | Hand-painted textures, 512-1024px, BC7 |
| 16-bit color banding | Optional: slight color quantization in material |

---

## Key Finding 7: Sprite Integration (Billboard System)

**Confidence:** High
**Sources:** [roBrowser source](https://github.com/vthibault/roBrowser), [jMonkeyEngine forum](https://hub.jmonkeyengine.org/t/ragnarok-style-graphics/19555)

### How RO Renders Sprites in 3D
1. Sprite world position converted through model-view/projection matrices
2. **Rotation portion of matrix replaced with identity** (removes all rotation = billboard)
3. Scaled relative to 175-unit base: `scaled = original / 175.0 * cellSize`
4. Z-index layering: `gl_Position.z -= zIndex * 0.01 + depth`
5. **Shadow**: Simple circular dark decal projected on ground (blob shadow)

### Why Sprites "Belong" in the 3D World
- The **narrow FOV** flattens perspective, making flat sprites less obviously flat
- **Posterized lighting** on terrain matches the flat-shaded appearance of sprites
- **No outlines on 3D geometry** — if buildings had outlines, sprites without them would look wrong
- **Consistent color palette** — sprites and textures use the same warm, limited palette
- **Blob shadows** ground the sprites in the scene (connect them to the surface)

### UE5 Translation (Already Implemented)
Your current sprite system (Unlit, Masked, billboard quads) is correct. The key additions:
- Blob shadow decal (already added in Phase 0D)
- Match the world's lighting warmth to the sprite palette
- The narrow FOV is the biggest missing piece for visual integration

---

## Key Finding 8: HD-2D Reference (Octopath Traveler)

**Confidence:** Medium
**Sources:** [Unreal Engine Spotlight - Octopath Traveler](https://www.unrealengine.com/en-US/spotlights/octopath-traveler-s-hd-2d-art-style-and-story-make-for-a-jrpg-dream-come-true), [HD-2D Wikipedia](https://en.wikipedia.org/wiki/HD-2D)

### Octopath's Approach
- **Tilt-shift camera** with wide FOV that flattens the viewpoint while keeping depth
- **Aggressive depth-of-field** — foreground and background heavily blurred
- **Point lights paired with VFX** — light creates character shadows on environment
- **Bloom** — soft, warm bloom on bright surfaces
- The "diorama" look comes from DoF + tilt-shift, NOT from cel-shading

### Tree of Savior's Approach
- **NOT cel-shaded** — uses hand-painted digital art style
- Characters are 3D bodies with 2D hand-drawn heads
- Style is "fairy tale" aesthetic with painterly textures

### Key Difference from RO
Both HD-2D and RO use sprites in 3D worlds, but:
- **RO**: Narrow FOV (telephoto), minimal post-process, posterized lightmaps, no DoF
- **HD-2D**: Wide FOV + tilt-shift, heavy DoF, bloom, dramatic lighting

For your project (RO-style), prioritize the narrow FOV and warm flat lighting over HD-2D's dramatic effects.

---

## Key Finding 9: Color Palette Analysis

**Confidence:** High
**Sources:** [Ragnarok Online Palette - Lospec](https://lospec.com/palette-list/ragnarok-online), [DeviantArt Palette](https://www.deviantart.com/lovipoe/art/Ragnarok-Online-Color-Palette-151651156)

### The RO Palette (210 colors)
The official RO palette is dominated by:
- **Warm earth tones**: Browns, tans, ochres (#826c3e, #a59766, #baa887)
- **Muted purples**: Desaturated lavenders (#887a98, #c2bdd4)
- **Warm reds/pinks**: (#814d5a, #9a616a, #bc8b8e)
- **Soft greens**: Desaturated olive/sage (#59784f, #729762)
- **Warm grays**: (#6c6559, #877f6b, #a09980)

### Characteristic Per-Zone Palettes
| Zone Type | Dominant Colors | Temperature |
|-----------|----------------|-------------|
| Medieval towns (Prontera) | Ochre, cream, warm stone | Warm (~6500-7000K feel) |
| Asian towns (Payon) | Cherry pink, bamboo green | Warm-neutral |
| Deserts (Morroc) | Amber, sienna, burnt orange | Very warm |
| Forests (Payon Forest) | Emerald, moss, dark brown | Neutral-cool |
| Dungeons | Purple, dark gray, deep blue | Cool |
| Snow (Lutie) | Ice blue, white, pale gray | Cool |

### UE5 Translation (Color Grading)

| Setting | Prontera (Town) | Field | Dungeon |
|---------|----------------|-------|---------|
| White Temperature | 6500K (slightly warm) | 6000K (neutral) | 5000K (cool) |
| Tint | Slight magenta (+0.02) | Neutral | Slight blue (+0.05) |
| Saturation | 0.85-0.9 (slightly desaturated) | 0.9-1.0 | 0.6-0.7 |
| Contrast | 0.9-0.95 (LOW - flat lighting) | 0.95 | 1.1 |
| Gain Highlights | (1.02, 1.0, 0.95) warm | (1,1,1) | (0.9, 0.9, 1.0) cool |

**Key insight**: RO's palette is DESATURATED and WARM, not vivid. The colors feel muted and cozy, not bright and flashy.

---

## Recommended UE5 Implementation (Revised from Current)

Based on all research, here's what the post-process chain should actually look like for RO Classic style:

### What to KEEP
1. **Bloom** — RO doesn't have bloom, but subtle bloom (0.3-0.5) adds warmth and softness
2. **Vignette** — Subtle (0.2-0.3) grounds the frame
3. **Per-zone color grading** — Essential for zone identity
4. **Blob shadows** — Correct, matches RO's shadow approach
5. **Fast auto-exposure adaptation** — Correct approach for zone transitions

### What to REMOVE/CHANGE
1. **Remove cel-shade post-process** — RO doesn't posterize the full scene. The posterization is in lightmaps only. The current cel-shade effect darkens the scene and doesn't match RO.
2. **Remove Sobel outline** — RO has NO outlines on 3D geometry. Outlines are only on 2D sprites (baked into the art).
3. **Add warm color grading** — The missing piece. Use `ColorGain`, `WhiteTemp`, and `Saturation` on the PPVolume instead of a custom material.

### What to ADD
1. **Reduce camera FOV** — From default (~90°) to 30-40° for telephoto compression. This is the BIGGEST change for matching RO's look.
2. **Material-level light quantization** (optional) — On the master environment material, quantize the lighting contribution to 8-16 levels. This gives the lightmap posterization effect without a post-process.
3. **Warmer, flatter lighting** — Lower contrast, higher ambient fill, warm directional light.

### Specific UE5 Post-Process Values

```cpp
// Built-in PostProcessSettings — no custom materials needed for base look
S.bOverride_BloomIntensity = true;
S.BloomIntensity = 0.35f;          // subtle warm glow
S.bOverride_BloomThreshold = true;
S.BloomThreshold = 1.5f;           // only bright areas bloom

S.bOverride_VignetteIntensity = true;
S.VignetteIntensity = 0.25f;

// Color grading — warm, slightly desaturated
S.bOverride_WhiteTemp = true;
S.WhiteTemp = 6800.f;              // slightly warm (default 6500)

S.bOverride_ColorSaturation = true;
S.ColorSaturation = FVector4(0.9f, 0.9f, 0.9f, 1.0f);  // slightly desaturated

S.bOverride_ColorContrast = true;
S.ColorContrast = FVector4(0.95f, 0.95f, 0.95f, 1.0f);  // low contrast (flat lighting)

S.bOverride_ColorGainHighlights = true;
S.ColorGainHighlights = FVector4(1.02f, 1.0f, 0.96f, 1.0f); // warm highlights

// Auto-exposure — fast adaptation, per-zone bias
S.bOverride_AutoExposureSpeedUp = true;
S.bOverride_AutoExposureSpeedDown = true;
S.AutoExposureSpeedUp = 20.0f;
S.AutoExposureSpeedDown = 20.0f;
S.bOverride_AutoExposureBias = true;
S.AutoExposureBias = 0.0f;         // adjust per zone
```

### Specific Lighting Setup (per zone)

```
Outdoor Zones:
  Directional Light:
    Intensity: 3.14 lux
    Temperature: 5800-6200K (neutral to slightly warm)
    Rotation: Pitch -50 to -60, Yaw 135 (afternoon feel)
  Sky Light:
    Intensity: 2.0-3.0 (HIGH — RO has bright ambient fill)
    Real Time Capture: On
  Exponential Height Fog:
    Density: 0.003
    Height Falloff: 0.5
    Inscattering Color: warm (0.8, 0.7, 0.5)

Dungeon Zones:
  NO Directional Light, NO Sky Light
  Point Lights only: warm (2700-3500K), 300-800 lumens, radius 400-600 UU
  Exponential Height Fog: density 0.03, colored (zone-specific)
```

---

## Areas of Disagreement

- **Cel-shading vs lightmap posterization**: Multiple community projects (roBrowser clones, fan remakes) add cel-shading outlines to recreate the RO look, but the original game does NOT have screen-space outlines. The posterized lightmaps create hard shadow edges that can be mistaken for outlines.
- **FOV**: Some RO private servers and remakes use wider FOVs for playability. The original 15° FOV is extremely narrow and may feel claustrophobic in a modern game. A compromise of 30-40° is reasonable.

## Gaps & Limitations

- Exact per-map diffuse/ambient light color values from RSW files were not available in the documentation (they vary per map and would need to be extracted from game data)
- The exact lightmap baking algorithm used by BrowEdit is not fully documented
- Water shader implementation details beyond basic alpha blending are sparse

---

## Sources

1. [Ragnarok Research Lab - Rendering Overview](https://ragnarokresearchlab.github.io/rendering/) — Primary technical reference (5/5 credibility)
2. [Ragnarok Research Lab - Camera Controls](https://ragnarokresearchlab.github.io/rendering/camera-controls/) — Exact camera parameters (5/5)
3. [Ragnarok Research Lab - Scene Lighting](https://ragnarokresearchlab.github.io/rendering/scene-lighting/) — Light, fog, water details (5/5)
4. [Ragnarok Research Lab - Ground Mesh](https://ragnarokresearchlab.github.io/rendering/ground-mesh/) — Lightmap posterization algorithm (5/5)
5. [GND Format Specification](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GND.MD) — File format with posterization code (5/5)
6. [RSW Format Specification](https://ragnarokresearchlab.github.io/file-formats/rsw/) — Scene-wide lighting params (5/5)
7. [GND Format - Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/gnd/) — Lightmap structure (5/5)
8. [roBrowser GitHub](https://github.com/vthibault/roBrowser) — Open-source RO client implementation (4/5)
9. [HD-2D - Wikipedia](https://en.wikipedia.org/wiki/HD-2D) — Octopath Traveler technique overview (3/5)
10. [Ragnarok Online Palette - Lospec](https://lospec.com/palette-list/ragnarok-online) — 210-color official palette (4/5)
11. [jMonkeyEngine - RO Style Graphics](https://hub.jmonkeyengine.org/t/ragnarok-style-graphics/19555) — Community recreation discussion (2/5)
12. [80.lv - Stylized UE5 Environment](https://80.lv/articles/a-stylized-ue-5-environment-material-creation-particles-techniques) — UE5 stylized workflow (3/5)
13. [Tree of Savior Forum](https://forum.treeofsavior.com/t/what-is-toss-sprite-art-graphics-style/154277) — ToS art style discussion (2/5)
