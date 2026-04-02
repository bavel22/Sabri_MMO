# Ground Texture System — Comprehensive Research & Implementation Plan

> **Goal**: Achieve the natural, multi-layered ground look seen in Ragnarok Online — splotchy multi-shade grass, automatic rock/cliff on slopes, soft organic blending, hand-painted oil-painting aesthetic.
>
> **Reference screenshots**: `troubleshootin pictures/likethis.png` (modern RO-style), `troubleshootin pictures/rogroundtexture.png` (classic RO)

---

## Table of Contents

1. [Visual Analysis — What Makes the Reference Look Good](#1-visual-analysis)
2. [RO Color Palette — The Foundation](#2-ro-color-palette)
3. [Texture Generation Pipeline (ComfyUI)](#3-texture-generation-pipeline)
4. [Seamless Tiling — Methods Ranked](#4-seamless-tiling)
5. [UE5 Landscape System vs Static Mesh](#5-ue5-landscape-vs-static-mesh)
6. [Slope-Based Auto-Material (Grass→Rock)](#6-slope-based-auto-material)
7. [Multi-Texture Blending for Natural Ground](#7-multi-texture-blending)
8. [Macro Variation — Breaking Tile Repetition](#8-macro-variation)
9. [Triplanar Projection for Cliffs](#9-triplanar-projection)
10. [Distance-Based Detail Scaling](#10-distance-based-detail)
11. [Anti-Repetition Techniques (Stochastic/Hex Tiling)](#11-anti-repetition)
12. [Post-Generation Color Correction](#12-color-correction)
13. [Texture Set — What to Generate](#13-texture-set)
14. [UE5 Material Node Recipes](#14-material-node-recipes)
15. [Performance Budget](#15-performance)
16. [Implementation Checklist](#16-checklist)
17. [Iteration Log — 10 Review Passes](#17-iteration-log)

---

## 1. Visual Analysis

### What the reference screenshots show

**Screenshot 1 — Modern RO-style** (`likethis.png`):
- Rich multi-shade grass: dark green, medium green, yellow-green, brown patches all mixed together
- Individual grass blade hints visible up close
- Rocky cliff faces appear **automatically** where terrain is steep
- The grass-to-cliff transition is soft and organic — follows slope angle, not a hard line
- Small flowers and decorative ground scatter
- Trees with autumn palette (warm yellow/orange foliage)
- The grass has visible color "patches" — some areas are more yellow-green, others deeper green

**Screenshot 2 — Classic RO** (`rogroundtexture.png`):
- Top-down isometric view
- Ground is a mix of yellowy-green and darker green in large, soft splotches
- Very organic, hand-painted look with visible brushstroke texture
- Darker areas near trees and edges (dirt/mud/shadow blending)
- Rock/cliff texture at the top-left corner
- Multiple layers of detail visible: base color, darker patches, lighter patches, small details
- The textures blend together with NO hard edges — completely organic transitions

### The 7 techniques that create this look

| # | Technique | What it does |
|---|-----------|-------------|
| 1 | **Multi-texture slope blending** | Rock on steep slopes, grass on flat — automatic |
| 2 | **Large-scale noise color variation** | Splotchy patches of different greens/yellows across the ground |
| 3 | **Multiple grass texture variants** | 2-3 grass textures with different green/yellow balance, blended by noise |
| 4 | **Height-based blending** | Dirt appears at transitions, lower areas, worn paths |
| 5 | **Hand-painted diffuse textures** | Oil-painting brushstroke quality, NOT photorealistic |
| 6 | **Seamless tiling with anti-repetition** | No visible tile grid even over large areas |
| 7 | **Triplanar projection on cliffs** | Rock texture wraps around steep faces without UV stretching |

---

## 2. RO Color Palette

RO's palette is **desaturated and warm**. The greens are olive/sage tones, never vivid/saturated. Browns dominate. The palette feels muted and cozy.

### Extracted RO Color Families (Lospec — 210 colors)

| Family | Hex Values (dark → light) | Use |
|--------|--------------------------|-----|
| **Earth tones** | `#826c3e` `#a59766` `#baa887` `#cbbda4` `#d6cbb8` `#e2dbc2` | Dominant everywhere |
| **Warm greens** (olive/sage) | `#253826` `#3f5b3c` `#59784f` `#729762` `#91b876` `#c2db95` | Grass — primary |
| **Cool greens** (blue-green) | `#17303c` `#30524f` `#446d5e` `#59896f` `#6da47c` `#93c890` | Forest grass |
| **Warm grays** | `#4d4542` `#6c6559` `#877f6b` `#a09980` `#b6b296` `#d7d6b2` | Paths, stone |
| **Muted purples** | `#24202f` `#373247` `#4e465f` `#645673` `#887a98` `#c2bdd4` | Rocks, shadows |
| **Orange/sienna** | `#743123` `#a85031` `#cc6840` `#e98354` `#f99f69` `#fccb90` | Dirt, cliff |

### Per-Zone Color Targets

| Zone Type | Primary Ground Colors | Temperature |
|-----------|----------------------|-------------|
| Medieval towns (Prontera) | Ochre, cream, warm stone | Warm |
| Fields (Prontera South/North) | Olive green, sage, yellow-brown patches | Warm |
| Forests (Payon) | Emerald, moss, dark brown | Neutral-cool |
| Deserts (Morroc) | Amber, sienna, burnt orange | Very warm |
| Dungeons | Purple, dark gray, deep blue | Cool |
| Snow (Lutie) | Ice blue, white, pale gray | Cool |

### Critical Rule: NO Saturated Colors

- Greens must be olive/sage (`#59784f`, `#729762`), NEVER vivid (`#00FF00`, `#33CC33`)
- Browns must be warm ochre/sienna, not cold gray-brown
- Rocks must be muted purple-gray, not pure gray
- Everything has a warm undertone

---

## 3. Texture Generation Pipeline

### Current Problems with Our Pipeline

Our existing `generate_ro_ground_textures.py` has several issues identified by research:

| Issue | Current Setting | Correct Setting |
|-------|----------------|-----------------|
| **No seamless tiling** | No tiling node | Add `SeamlessTile` node (circular Conv2d padding) |
| **CFG too high** | 7.0 | **6.5** (critical for texture quality per research) |
| **Steps too low** | 25 | **50-60** (sharper detail for textures) |
| **LoRA too strong for textures** | 0.3 | **0.15-0.2** (0.3 ok for sprites, too much for textures) |
| **Prompts use booru tags** | `flat_color, seamless_texture` | Natural language prompts work better for SDXL |
| **No post-processing** | Raw output | Need seamless verification + color correction |
| **Resolution** | 1024x1024 | 1024 is correct for SDXL, stitch 4→2048 for more variation |

### Updated ComfyUI Workflow

**Required custom nodes** (install once):
- `ComfyUI-seamless-tiling` by spinagon — patches Conv2d for circular padding
- `was-node-suite-comfyui` — Image Adjust for color correction

**Node chain:**
```
CheckpointLoaderSimple (Illustrious-XL-v0.1.safetensors)
  → LoraLoader (ROSprites-v2.1C, strength 0.15)
    → SeamlessTile (tiling_x: true, tiling_y: true)
      → CLIPTextEncode (positive)
      → CLIPTextEncode (negative)
      → EmptyLatentImage (1024x1024)
        → KSampler (euler, steps=60, cfg=6.5, denoise=1.0)
          → VAEDecode
            → SaveImage
```

### Prompt Templates (Natural Language, NOT Booru Tags)

**Grass — Warm variant:**
```
hand painted tileable ground grass texture, stylized game art, oil painting
brushstrokes, warm muted olive green with yellow-brown patches, splotchy color
variation, multiple shades of green mixed together, flat lighting, top-down view,
seamless pattern, highly detailed, sharp focus, game texture asset, matte surface,
no shadows, full frame coverage
```

**Grass — Cool variant (forest):**
```
hand painted tileable ground grass texture, stylized game art, oil painting
brushstrokes, cool emerald green with moss and dark brown patches, blue-green
undertones, forest floor feeling, flat lighting, top-down view, seamless pattern,
highly detailed, sharp focus, game texture asset, matte surface, no shadows
```

**Dirt/Earth:**
```
hand painted tileable ground dirt texture, stylized game art, oil painting
brushstrokes, warm ochre sienna umber earth tones, subtle pebble and grain detail,
dry cracked earth feeling, matte flat lighting, top-down view, seamless pattern,
highly detailed, game texture asset, no shadows, full frame
```

**Rock/Cliff face:**
```
hand painted rock cliff face texture, stylized game art, muted purple-gray stone,
visible brushstrokes, cracks and weathering detail, warm desaturated palette,
seamless tileable, flat lighting, no specular highlights, game texture asset,
highly detailed, matte surface
```

**Cobblestone (towns):**
```
hand painted tileable cobblestone road texture, stylized game art, oil painting,
warm gray and ochre medieval stone pavers, irregular stone shapes with mortar lines,
matte flat lighting, top-down view, seamless pattern, game texture asset, no shadows
```

**Sand (desert zones):**
```
hand painted tileable desert sand texture, stylized game art, oil painting
brushstrokes, warm golden sand with subtle ripple patterns, sienna and amber tones,
organic random variation, flat lighting, top-down view, seamless, game texture asset
```

**Dungeon floor:**
```
hand painted tileable dungeon floor texture, stylized game art, dark blue-gray
slate stone, cracked and worn, muted purple undertones, cold and damp feeling,
flat lighting, top-down view, seamless pattern, game texture asset, matte
```

### Negative Prompt (Use for ALL textures)

```
photorealistic, photograph, 3d render, smooth gradient, plastic, glossy, shiny,
specular, metallic, blurry, low quality, worst quality, watermark, signature, text,
shadows, dramatic lighting, high contrast, saturated, neon, vivid colors,
cold blue-green, person, character, face, items, objects, perspective, sky, horizon,
side view, repeating pattern, grid, symmetric
```

### Generation Settings Summary

| Parameter | Value | Why |
|-----------|-------|-----|
| Checkpoint | Illustrious-XL-v0.1 | Best for stylized/hand-painted aesthetic |
| LoRA | ROSprites-v2.1C @ **0.15** | Subtle RO warmth without sprite artifacts |
| Resolution | 1024x1024 | Native SDXL; stitch 4→2048 for variation |
| Sampler | euler | Best for SDXL textures at 60 steps |
| Steps | **60** | Higher = sharper detail for texture work |
| CFG | **6.5** | Critical — NOT 7.0+. Per texture generation experts |
| Tiling | **SeamlessTile node enabled** | Circular Conv2d padding = inherently seamless |
| Batch | Generate 8-16 per type, curate best | Variation via seed increment |

---

## 4. Seamless Tiling

### Methods Ranked (Best → Worst)

| Rank | Method | Quality | Effort | Notes |
|------|--------|---------|--------|-------|
| 1 | **SeamlessTile node (circular padding)** | Excellent | Zero post-processing | Inherently seamless — modify Conv2d during generation |
| 2 | **Multi-band Laplacian pyramid blend** | Excellent | Moderate code | Best post-processing method, preserves detail at all frequencies |
| 3 | **Poisson blending (gradient-domain)** | Very good | Complex math | Preserves local contrast, may shift brightness |
| 4 | **Cross-blend (diamond/cosine mask)** | Good | Simple code | Can cause ghosting on strong directional features |
| 5 | **Offset + inpaint seam** | Good | Requires SD inpainting | Slow, each texture needs a second generation pass |
| 6 | **Manual Photoshop clone stamp** | Variable | Very slow | Depends on artist skill |

### Primary Method: SeamlessTile in ComfyUI

Install `ComfyUI-seamless-tiling` (by spinagon, GPL-3.0, updated March 2025):
- `SeamlessTile` node: patches model Conv2d layers for circular padding
- `Make Circular VAE`: modifies VAE for circular wrapping on decode
- `Offset Image`: verification tool — shifts image to expose seams for inspection

Place `SeamlessTile` between model loader and KSampler. Set `tiling_x: true, tiling_y: true`.

### Fallback: Cross-Blend Post-Processing

For any texture that still has subtle seams after generation:

```python
import numpy as np
from PIL import Image

def make_seamless(path, border_frac=0.2):
    """Cross-blend with cosine ramp for smooth seamless tiling."""
    img = np.array(Image.open(path), dtype=np.float32)
    H, W = img.shape[:2]
    shifted = np.roll(np.roll(img, W//2, axis=1), H//2, axis=0)

    # Create 1D cosine ramps
    bw = int(W * border_frac)
    bh = int(H * border_frac)
    ramp = lambda n: 0.5 * (1 + np.cos(np.linspace(0, np.pi, n)))

    wx = np.ones(W)
    wx[:bw] = ramp(bw)
    wx[-bw:] = ramp(bw)[::-1]

    wy = np.ones(H)
    wy[:bh] = ramp(bh)
    wy[-bh:] = ramp(bh)[::-1]

    mask = (wy[:, None] * wx[None, :])[:, :, np.newaxis]
    result = (img * mask + shifted * (1 - mask)).clip(0, 255).astype(np.uint8)
    Image.fromarray(result).save(path)
```

### Verification Checklist

After generation, verify EVERY texture:

1. **Edge pixel test**: `mean(|left_col - right_col|) < 2.0` per channel (0-255 scale)
2. **Gradient continuity**: `mean(|grad_right_edge - grad_left_edge|) < 3.0`
3. **3x3 tile visual**: Tile the texture 3x3 and check at 100%, 50%, 25% zoom
4. **Squint test**: Squint at the 3x3 grid — reveals low-frequency repetition patterns
5. **Histogram check**: Left/right edge strips should have similar mean brightness (difference < 5)

---

## 5. UE5 Landscape vs Static Mesh

### Recommendation: Use UE5 Landscape Actor

| Factor | Landscape Actor | Static Mesh |
|--------|----------------|-------------|
| Memory | 4 bytes/vertex | 24-28 bytes/vertex |
| Paint layers | Built-in brush tool | Vertex color only (4 channels) |
| Slope-based material | Full support | Manual via material |
| LOD | Automatic continuous | Manual LOD setup |
| Collision | Free (heightfield) | Must configure |
| Sculpting | Built-in editor | External (Blender) |
| Grass spawning | Grass Output node | Manual placement |
| Overhangs/caves | Not possible | Supported |

**For Sabri_MMO's RO-style zones**: Landscape Actor is strongly recommended because:
- RO zones are small (~400x400 game units) — well within Landscape performance range
- Paint layers allow manual artistic control alongside automatic slope blending
- The Grass Output node can auto-spawn ground detail meshes
- Built-in collision is simpler than per-mesh setup
- Top-down camera means LOD streaming matters less, but memory savings help for multiplayer

**Static meshes** should only be used for: buildings, props, cliff face meshes that need overhangs/caves, bridges, and decorative elements — NOT the ground plane.

### Landscape Setup Steps

1. In UE5 Editor: Landscape Mode (Shift+2) → Create new Landscape
2. Set section size to match zone bounds
3. Sculpt terrain with hills and slopes
4. Assign the auto-material (see Section 6)
5. Paint layer overrides where needed (paths, clearings)
6. Add grass/flower scatter via Grass Output node

---

## 6. Slope-Based Auto-Material

### The Core: WorldAlignedBlend

The `WorldAlignedBlend` material function automatically detects surface slope and blends between textures.

**Inputs:**

| Input | Value | Effect |
|-------|-------|--------|
| Blend 0 (Top) | Grass texture | Applied to flat surfaces |
| Blend 1 (Sides) | Rock texture | Applied to steep surfaces |
| In World Vector | `(0, 0, 1)` | Compare against world up |
| Blend Sharpness | `15` (default) | Higher = sharper grass→rock transition |
| Blend Bias | `-5` (default) | More negative = more rock coverage |

**Connection pattern:**
```
GrassTexture --------→ WorldAlignedBlend [Blend 0 (Top)]
RockTexture ---------→ WorldAlignedBlend [Blend 1 (Sides)]
Constant3(0,0,1) ---→ WorldAlignedBlend [In World Vector]
ScalarParam(15) ----→ WorldAlignedBlend [Blend Sharpness]
ScalarParam(-5) ----→ WorldAlignedBlend [Blend Bias]
                       |
                       → Base Color output
```

### Manual DotProduct Method (More Control)

For finer control over the slope threshold:

```
VertexNormalWS → DotProduct(Constant3(0,0,1)) → SlopeValue
                                                    |
SlopeValue → SmoothStep(Min=0.6, Max=0.9) → GrassMask (1=flat, 0=steep)
OneMinus(GrassMask) → RockMask
Lerp(A=GrassColor, B=RockColor, Alpha=RockMask) → BaseColor
```

### Slope Angle Reference

| Surface Angle | DotProduct | Classification |
|--------------|------------|----------------|
| 0 degrees (flat) | 1.0 | Pure grass |
| 15 degrees | 0.966 | Grass |
| 30 degrees | 0.866 | Grass with hint of dirt |
| 45 degrees | 0.707 | Transition zone |
| 60 degrees | 0.5 | Mostly rock |
| 75 degrees | 0.259 | Pure rock |
| 90 degrees (wall) | 0.0 | Pure cliff |

### Adding a Dirt Transition Layer

The reference screenshots show dirt appearing at the grass-to-rock transition. Add a middle layer:

```
SlopeValue → SmoothStep(0.6, 0.85) → GrassMask
SlopeValue → SmoothStep(0.35, 0.55) → MidMask (inverted = dirt zone)
SlopeValue → SmoothStep(0.2, 0.4) → OneMinus → RockMask

Lerp(Grass, Dirt, 1-GrassMask*MidMask) → GrassDirt
Lerp(GrassDirt, Rock, RockMask) → FinalColor
```

---

## 7. Multi-Texture Blending for Natural Ground

### The Complete 4-Layer System

This creates the splotchy, multi-shade look from the screenshots:

```
Layer 1: Grass Warm    (olive/sage green, dominant)
Layer 2: Grass Cool    (yellow-green patches, noise-blended)
Layer 3: Dirt          (ochre/sienna, noise-driven patches)
Layer 4: Rock          (muted purple-gray, slope-driven)
```

### Node Graph (Pseudo-Code)

```
// === LAYER 1: Base Grass (warm olive) ===
AbsoluteWorldPosition → Divide(500) → TextureSample(T_Grass_Warm) → GrassWarm

// === LAYER 2: Alternate Grass (yellow-green patches) ===
AbsoluteWorldPosition → Divide(500) → TextureSample(T_Grass_Cool) → GrassCool

// Noise mask for grass variant patches (large-scale splotches)
AbsoluteWorldPosition.XY → Multiply(0.0005) → Noise(FastGradient, Levels=2) → NoiseMask1
Lerp(A=GrassWarm, B=GrassCool, Alpha=NoiseMask1) → GrassMixed

// === LAYER 3: Dirt patches (noise-driven) ===
AbsoluteWorldPosition → Divide(400) → TextureSample(T_Dirt) → DirtColor

// Separate noise for dirt patches (different frequency)
AbsoluteWorldPosition.XY → Multiply(0.001) → Noise(FastGradient, Levels=3)
  → Saturate → Power(Exponent=2) → DirtMask
Lerp(A=GrassMixed, B=DirtColor, Alpha=DirtMask * 0.3) → GrassDirtMixed

// === LAYER 4: Rock on slopes ===
WorldNormal → DotProduct(0,0,1) → OneMinus → Power(2) → Saturate → SlopeMask
AbsoluteWorldPosition → WorldAlignedTexture(T_Rock, Size=300) → RockColor
Lerp(A=GrassDirtMixed, B=RockColor, Alpha=SlopeMask) → FinalBaseColor
```

### Using LandscapeLayerBlend (Paint + Auto Combined)

For manual artistic control on top of the auto-material:

1. Create `LandscapeLayerBlend` node with 4 layers
2. Set all layers to `LB_HeightBlend` for organic transitions
3. Connect height/displacement maps per layer (controls which material "pokes through" at boundaries)
4. Feed the slope mask as default weight for the Rock layer
5. Feed noise masks as default weights for Grass/Dirt layers
6. Paint manual overrides for paths, clearings, special areas

**Key insight**: `LB_HeightBlend` is critical for natural-looking transitions. It uses a displacement map per layer so that, for example, rocks poke through grass at transitions rather than a uniform gradient blend.

### Making Transitions Soft and Organic

- **Never use Step node** (hard threshold). Always use `Lerp` with noise-driven alpha.
- **Power node** on alpha controls falloff: `Power(2)` = smooth quadratic, `Power(0.5)` = wider/softer
- **SmoothStep** provides S-curve: `SmoothStep(Min=0.3, Max=0.7, Alpha=Noise)` — soft ramp
- **Multiply noise × height map**: Dirt appears where grass height is "short" — gives organic worn-path look
- **Always Saturate** after blend mask math to clamp 0-1

---

## 8. Macro Variation — Breaking Tile Repetition

This is the single most important technique for making tiled textures look natural over large areas. It adds large-scale random color variation that hides the repeating tile pattern.

### Method 1: T_Default_MacroVariation (Built-in UE5)

UE5 ships with `T_Default_MacroVariation` in Engine Content. Sample it 3 times at 3 different world-space UV scales:

```
AbsoluteWorldPosition.XY × 0.002   → TextureSample → Sample1  (500 UU wavelength)
AbsoluteWorldPosition.XY × 0.0007  → TextureSample → Sample2  (1400 UU wavelength)
AbsoluteWorldPosition.XY × 0.00015 → TextureSample → Sample3  (6600 UU wavelength)

Sample1 × Sample2 × Sample3 → VariationMask

Lerp(BaseColor, BaseColor × VariationMask, VariationStrength=0.15) → FinalColor
```

The three frequencies create overlapping variation at different scales — the human eye cannot detect the repeating pattern.

### Method 2: Procedural Noise Tinting

For more control and RO-specific warmth:

```
AbsoluteWorldPosition.XY → Multiply(0.0002) → Noise(FastGradient, Levels=3)
  → Lerp(A=BaseColor, B=WarmTintedColor, Alpha=NoiseOutput × 0.12)
```

- **NoiseScale 0.0002**: ~5000 UU per cycle (very large patches)
- **TintStrength 0.10-0.15**: Subtle — just enough to break repetition
- **TintColor**: Warm ochre shift `(1.05, 1.0, 0.92)` — slightly warmer patches

### Method 3: Dual Grass Variant Blend (Best for RO Look)

This directly creates the splotchy multi-green look from the screenshots:

```
Grass_Variant_A (olive green)  ←→  Grass_Variant_B (yellow-green)
                    ↕
         Noise mask at scale 0.0005
```

Generate 2-3 grass textures with different green/yellow balance. Blend them using large-scale world-position noise. This creates the characteristic RO splotchy grass.

---

## 9. Triplanar Projection for Cliffs

When a texture is applied to a steep cliff face using standard UV mapping, it stretches badly. Triplanar projection fixes this by projecting the texture from all 3 axes and blending based on surface normal.

### UE5 Built-in: WorldAlignedTexture

One-node solution:

```
TextureObject(T_Rock) → WorldAlignedTexture [TextureSize=300]
                          |
                          → Rock BaseColor (no stretching on any surface angle)
```

**TextureSize parameter**: Controls tiling in world units. 300 = texture repeats every 300 UU.

### Performance Note

Triplanar = 3 texture samples instead of 1. Apply ONLY to cliff/rock surfaces, not flat ground. Use the slope mask to conditionally apply triplanar only where needed:

```
// Flat ground: standard UV sampling (1 sample, cheap)
// Steep slopes: triplanar (3 samples, expensive but needed)
SlopeMask > 0.5 → Use triplanar rock
SlopeMask < 0.5 → Use standard UV grass
```

### Normal Map Handling

Normal maps require special treatment in triplanar — RGB channels encode direction vectors, not colors. Use `WorldAlignedNormal` node instead of `WorldAlignedTexture` for normal maps.

**For RO-style materials**: We don't use normal maps (roughness ~1.0, no specular), so this is not a concern. Triplanar is only needed for BaseColor.

---

## 10. Distance-Based Detail Scaling

Prevents visible tiling at distance while keeping detail up close.

### Near/Far Texture Blend

```
// Near: high-frequency tiling (close-up detail)
AbsoluteWorldPosition → Divide(200) → TextureSample → NearTex

// Far: low-frequency tiling (large-scale, no repetition visible)
AbsoluteWorldPosition → Divide(2000) → TextureSample → FarTex

// Distance mask
CameraDepthFade(FadeOffset=500, FadeLength=300) → DistMask

Lerp(A=NearTex, B=FarTex, Alpha=DistMask) → FinalTex
```

**Important**: When sampling the same texture twice, set the second TextureSample's **Sampler Source** to `Shared: Wrap` to avoid consuming an extra sampler slot.

### Practical Values for Top-Down RO Camera

| Parameter | Value | Notes |
|-----------|-------|-------|
| Near tiling scale | 200-500 UU per repeat | Full detail visible |
| Far tiling scale | 1500-3000 UU per repeat | Stretched but no repetition visible |
| Fade start | 500-800 UU from camera | Where near detail starts fading |
| Fade length | 300-500 UU | Width of transition zone |

### For Top-Down Camera

With a top-down camera at fixed distance, the entire ground is roughly the same distance from the camera. Distance-based detail is **less critical** than in a third-person game, but still helps for larger zones where some ground is farther than other.

The **macro variation technique** (Section 8) is more important for top-down than distance blending.

---

## 11. Anti-Repetition Techniques

Even with perfectly seamless textures, large areas show visible repetition. These techniques eliminate it.

### Hex Tiling (Recommended for RO)

Samples the texture using hexagonal cells with random rotation/offset per cell:

1. Convert UV to hex grid coordinates
2. Find the 3 nearest hex centers
3. For each center, compute a random UV offset (hash of hex cell index)
4. Sample texture at each offset UV
5. Blend with smooth weights based on distance to each hex center

**Cost**: 3 texture samples instead of 1. Worth it for the main ground material.

### Stochastic Tiling

Uses triangular grid with random offsets — similar concept to hex tiling but with different sampling pattern. Based on "Procedural Stochastic Textures by Tiling and Blending" (Deliot & Heitz, 2019).

### Macro + Micro Texture Combining (Simplest)

The standard AAA approach:

```
Micro texture: tiles at 4-8x frequency (individual detail)
Macro texture: tiles at 0.3-0.5x frequency (large-scale color variation)

Result = Micro × Macro × 2.0  (overlay blend)
```

The `× 2.0` compensates for darkening from multiplication. This is the simplest anti-repetition method and often sufficient for stylized games.

### Irrational Tiling Ratios

Layer two textures at tiling ratios that are irrational (never align):
- Base grass: tiling at 1.0x
- Detail overlay: tiling at 1.618x (golden ratio) or 3.7x

The two layers never align the same way, preventing periodic pattern perception.

---

## 12. Post-Generation Color Correction

AI models default to vivid, saturated colors. RO needs desaturated, warm, muted tones.

### Step 1: Desaturation

After generation, reduce saturation by 10-20%:
```python
from PIL import Image, ImageEnhance
img = Image.open("texture.png")
enhancer = ImageEnhance.Color(img)
img = enhancer.enhance(0.85)  # 15% desaturation
```

### Step 2: Warm Shift

Apply a warm color overlay:
```python
import numpy as np
arr = np.array(img, dtype=np.float32)
# Warm shift: boost R slightly, reduce B slightly
arr[:, :, 0] *= 1.03  # Red +3%
arr[:, :, 2] *= 0.97  # Blue -3%
arr = arr.clip(0, 255).astype(np.uint8)
img = Image.fromarray(arr)
```

### Step 3: Histogram Edge Normalization

Ensure no brightness gradient across the tile:
```python
# Compare left strip vs right strip
left_mean = arr[:, :32].mean()
right_mean = arr[:, -32:].mean()
if abs(left_mean - right_mean) > 5:
    # Apply gradient correction
    correction = np.linspace(left_mean - right_mean, 0, W)
    arr = arr + correction[None, :, None] * 0.5
```

### In-Engine LUT

For final consistency, apply a color lookup table in the landscape material:
- Create a LUT texture that maps colors toward the RO palette ranges
- Apply via `ColorLookupTable` node in the material
- This ensures ALL textures match regardless of source

---

## 13. Texture Set — What to Generate

### Required Textures (2-3 variants each)

| Texture | Resolution | Description | Variants |
|---------|-----------|-------------|----------|
| **T_Grass_Warm_RO** | 1024 | Olive/sage green, yellow-brown patches | 3 (different patch balance) |
| **T_Grass_Cool_RO** | 1024 | Blue-green emerald, moss undertones | 2 (forest zones) |
| **T_Dirt_RO** | 1024 | Warm ochre-sienna, subtle pebbles | 2 (path vs clearing) |
| **T_Rock_RO** | 1024 | Muted purple-gray, cracks, weathering | 2 (cliff face vs rubble) |
| **T_Cobble_RO** | 1024 | Warm gray-ochre medieval stone | 1 (town roads) |
| **T_Sand_RO** | 1024 | Warm golden sand, ripple patterns | 1 (desert zones) |
| **T_DungeonFloor_RO** | 1024 | Dark blue-gray slate, cracked | 1 |
| **T_DungeonWall_RO** | 1024 | Dark gray-purple bricks | 1 |
| **T_Snow_RO** | 1024 | White with subtle blue shadows | 1 (Lutie zone) |

**Total**: ~15 textures at 1024x1024 = ~15 MB compressed (trivial memory cost).

### Do We Need Normal Maps?

**NO.** For the RO art style:
- RO's original textures have zero normal maps — surfaces are purely diffuse
- The hand-painted look relies on painted depth cues within the diffuse itself
- UE5 materials should use: **Roughness 0.95-1.0, Metallic 0.0, no normal map**
- Adding normal maps would make surfaces respond to dynamic lighting in a way that breaks the flat, hand-painted look

### What Maps We DO Need Per Texture

| Map | Type | Notes |
|-----|------|-------|
| **Base Color (Albedo)** | RGB | The generated texture itself |
| **Roughness** | Scalar | Flat 0.95 (can be constant in material, no texture needed) |
| **Height/Displacement** | Grayscale | Optional — only for LB_HeightBlend transitions |

---

## 14. UE5 Material Node Recipes

### Recipe A: Minimal Auto-Material (3 layers, slope only)

```
INPUTS:
  T_Grass_Warm (TextureObject)
  T_Dirt (TextureObject)
  T_Rock (TextureObject)

GRAPH:
  // Grass (flat)
  TexCoord × 0.25 → TextureSample(T_Grass_Warm) → GrassColor

  // Dirt (moderate slope)
  TexCoord × 0.3 → TextureSample(T_Dirt) → DirtColor

  // Rock (steep) — triplanar to prevent stretching
  WorldAlignedTexture(T_Rock, Size=400) → RockColor

  // Slope detection
  WorldAlignedBlend(Top=GrassColor, Sides=RockColor, Sharpness=15, Bias=-5) → GrassRock

  // Add dirt in transition zone
  VertexNormalWS → DotProduct(0,0,1) → SmoothStep(0.5, 0.75) → OneMinus → DirtMask
  VertexNormalWS → DotProduct(0,0,1) → SmoothStep(0.3, 0.55) → RockMask
  DirtMask × (1 - RockMask) → TransitionMask  // only in the transition band
  Lerp(GrassRock, DirtColor, TransitionMask × 0.6) → FinalBaseColor

  // Material properties
  FinalBaseColor → BaseColor
  Constant(0.95) → Roughness
  Constant(0.0) → Metallic
```

### Recipe B: Full Auto-Material (4 layers + macro variation + distance blend)

```
INPUTS:
  T_Grass_Warm, T_Grass_Cool (TextureObjects)
  T_Dirt, T_Rock (TextureObjects)
  T_Default_MacroVariation (TextureObject, from Engine Content)

GRAPH:
  // === GRASS VARIANT BLENDING ===
  TexCoord × 0.25 → TS(T_Grass_Warm) → GrassWarm
  TexCoord × 0.25 → TS(T_Grass_Cool) → GrassCool
  AbsWorldPos.XY × 0.0005 → Noise(FastGradient, 2 levels) → VariantMask
  Lerp(GrassWarm, GrassCool, VariantMask) → GrassMixed

  // === MACRO VARIATION (anti-repetition) ===
  AbsWorldPos.XY × 0.002  → TS(MacroVar) → MV1
  AbsWorldPos.XY × 0.0007 → TS(MacroVar) → MV2
  AbsWorldPos.XY × 0.00015 → TS(MacroVar) → MV3
  MV1 × MV2 × MV3 → MacroMask
  Lerp(GrassMixed, GrassMixed × MacroMask, 0.12) → GrassVaried

  // === DIRT PATCHES (noise-driven) ===
  TexCoord × 0.3 → TS(T_Dirt) → DirtColor
  AbsWorldPos.XY × 0.001 → Noise(3 levels) → Sat → Power(2) → DirtMask
  Lerp(GrassVaried, DirtColor, DirtMask × 0.25) → GrassDirt

  // === SLOPE BLENDING ===
  WorldAlignedTexture(T_Rock, 400) → RockColor
  VertexNormalWS → Dot(0,0,1) → SlopeVal
  SlopeVal → SmoothStep(0.5, 0.8) → OneMinus → RockMask
  Lerp(GrassDirt, RockColor, RockMask) → FinalColor

  // === DISTANCE BLEND (optional) ===
  TexCoord × 0.05 → TS(T_Grass_Warm) → FarGrass
  CameraDepthFade(600, 400) → DistMask
  Lerp(FinalColor, FarGrass × MacroMask, DistMask × 0.5) → DistanceBlended

  // === OUTPUT ===
  DistanceBlended → BaseColor
  0.95 → Roughness
  0.0 → Metallic
```

### Recipe C: LandscapeLayerBlend (Paintable)

```
LAYERS:
  "Grass"  — LB_HeightBlend — T_Grass_Warm (BaseColor, Height from R channel)
  "Dirt"   — LB_HeightBlend — T_Dirt (BaseColor, Height from R channel)
  "Rock"   — LB_HeightBlend — T_Rock + WorldAlignedTexture (BaseColor)
  "Path"   — LB_HeightBlend — T_Cobble (BaseColor, for manual path painting)

Each layer → MakeMaterialAttributes → LandscapeLayerBlend → Material Output

Use CheapContrast on height maps to control transition sharpness.
Default weights: Slope mask feeds Rock layer, noise feeds Dirt layer.
Manual painting overrides defaults.
```

---

## 15. Performance Budget

### Texture Sampler Limits

- **Hard limit**: 16 texture sampler slots per material (DirectX SM)
- **Landscape overhead**: ~4 samplers reserved
- **Available**: ~12 user samplers
- **Per layer**: 1 sampler (BaseColor only, no normal map for RO style)

**Budget for the full material:**

| Component | Samplers Used |
|-----------|--------------|
| Grass Warm | 1 |
| Grass Cool | 1 |
| Dirt | 1 |
| Rock (triplanar) | 3 (one per axis) |
| MacroVariation (3 scales, shared sampler) | 1 (Shared:Wrap) |
| Distance blend (shared sampler) | 0 (reuses Grass sampler) |
| **Total** | **7 of 12** |

Plenty of headroom. No optimization needed.

### Instruction Count

Target: < 200 base pass instructions. The full Recipe B material should be ~120-150 instructions — well within budget.

### Multiplayer Impact

- Landscape material cost is per-client, not per-player
- All players in the same zone see the same landscape
- A 4-layer auto-material with macro variation typically costs 0.5-1.0ms GPU time
- This is the cheapest part of the scene compared to sprites, UI, and particles

---

## 16. Implementation Checklist

### Phase 1: Texture Generation
- [ ] Install `ComfyUI-seamless-tiling` custom nodes
- [ ] Update `generate_ro_ground_textures.py` with new workflow (SeamlessTile, CFG 6.5, 60 steps, LoRA 0.15)
- [ ] Generate T_Grass_Warm variants (3 seeds)
- [ ] Generate T_Grass_Cool variants (2 seeds)
- [ ] Generate T_Dirt variants (2 seeds)
- [ ] Generate T_Rock variants (2 seeds)
- [ ] Generate T_Cobble
- [ ] Post-process: desaturation + warm shift on all textures
- [ ] Verify seamless tiling: edge test + 3x3 visual for each
- [ ] Curate best variant per type

### Phase 2: UE5 Landscape Setup
- [ ] Create Landscape Actor in Prontera South (test zone)
- [ ] Sculpt basic terrain: flat areas, hills, cliff edges
- [ ] Create Master Landscape Material (Recipe B or C)
- [ ] Test slope-based grass→rock transition
- [ ] Test macro variation (no visible tile grid)
- [ ] Test triplanar on cliff faces (no stretching)

### Phase 3: Layer Painting
- [ ] Create Layer Info assets for each material layer
- [ ] Auto-paint defaults from slope/noise masks
- [ ] Manually paint paths, clearings, special areas
- [ ] Add grass/flower scatter via Grass Output node

### Phase 4: Per-Zone Presets
- [ ] Create Material Instances per zone with parameterized tint/scale
- [ ] Apply appropriate textures per zone (warm grass for fields, cool for forests, etc.)
- [ ] Test zone transitions — no jarring texture changes

### Phase 5: Polish
- [ ] Distance blend testing (if using Recipe B)
- [ ] Performance profiling (should be < 1ms GPU)
- [ ] Compare against reference screenshots
- [ ] Iterate on noise scales, blend sharpness, color tinting

---

## 17. Iteration Log — 10 Review Passes

### Pass 1: Core Architecture Decision
**Question**: Landscape Actor vs Static Mesh for ground?
**Finding**: Landscape Actor is clearly superior — built-in paint layers, slope blending, LOD, collision, grass spawning. Static mesh terrain would require reimplementing all of these manually.
**Decision**: Use Landscape Actor for all zone ground surfaces.

### Pass 2: Texture Generation Quality
**Question**: Are our current textures good enough?
**Finding**: No. Current pipeline has: wrong CFG (7.0 vs 6.5), too few steps (25 vs 60), no seamless tiling node, LoRA too strong (0.3 vs 0.15 for textures), booru-tag prompts instead of natural language.
**Decision**: Complete rewrite of generation script with SeamlessTile node, corrected settings, and natural-language prompts.

### Pass 3: Slope Blending Method
**Question**: WorldAlignedBlend vs manual DotProduct?
**Finding**: WorldAlignedBlend is simpler and handles the common case well. Manual DotProduct gives more control for adding a 3rd transition layer (dirt). Both work.
**Decision**: Use WorldAlignedBlend as the primary slope blend, with a manual DotProduct-based transition zone for the dirt layer.

### Pass 4: Color Variation Approach
**Question**: How to achieve the splotchy multi-green look?
**Finding**: Three complementary techniques needed: (1) Multiple grass texture variants blended by world-space noise, (2) Macro variation overlay (T_Default_MacroVariation at 3 scales), (3) Post-generation color correction to match RO palette.
**Decision**: Implement all three. The grass variant blending is the most impactful for matching the reference screenshots.

### Pass 5: Seamless Tiling Method
**Question**: Generation-time tiling vs post-processing?
**Finding**: SeamlessTile node (circular Conv2d padding) produces inherently seamless textures with zero quality loss. Post-processing methods (cross-blend, Poisson) always lose some detail at edges. Generation-time is clearly better.
**Decision**: Primary: SeamlessTile node. Fallback: cosine cross-blend for any textures that still show subtle seams.

### Pass 6: Anti-Repetition Strategy
**Question**: Single technique or layered approach?
**Finding**: No single technique eliminates all visible repetition. The AAA standard is macro+micro combining. For stylized games, macro variation (3-scale noise overlay) is usually sufficient. Hex/stochastic tiling adds insurance but costs 3x texture samples.
**Decision**: Start with macro variation (3-scale T_Default_MacroVariation). Add hex tiling only if repetition is still visible after testing in-engine.

### Pass 7: Normal Maps
**Question**: Do we need normal maps for the RO art style?
**Finding**: Unanimously no. RO has no normal maps. All surfaces are purely diffuse/matte. Adding normal maps would make surfaces respond to dynamic lighting, breaking the flat hand-painted look. Roughness 0.95, Metallic 0.0, no normal map.
**Decision**: No normal maps. This also saves 1 sampler per layer, keeping us well within the 16-sampler budget.

### Pass 8: Performance Viability
**Question**: Can a 4-layer material with macro variation run at target framerate?
**Finding**: With 7 of 12 available samplers and ~130 estimated instructions, we're well within budget. The material would cost ~0.5-1ms GPU, which is trivial compared to the rest of the scene. No RVT caching needed for this complexity level.
**Decision**: Full Recipe B is viable without any performance compromises.

### Pass 9: Cliff Face Handling
**Question**: Standard UV vs triplanar for steep surfaces?
**Finding**: Standard UV stretches badly on steep surfaces (>45 degrees). Triplanar eliminates stretching but costs 3x samples. Since we only apply triplanar to the rock layer (which only appears on steep slopes), the conditional sampling means most pixels only pay 1x cost.
**Decision**: Triplanar (WorldAlignedTexture) for rock/cliff layer only. Standard UV for grass/dirt.

### Pass 10: Final Architecture Validation
**Question**: Does the complete system match what we see in the reference screenshots?
**Checklist**:
- Multi-shade grass with color patches? **Yes** — dual grass variants + noise blending
- Rock on steep slopes? **Yes** — WorldAlignedBlend slope detection
- Soft organic transitions? **Yes** — SmoothStep + noise masks, LB_HeightBlend
- Hand-painted aesthetic? **Yes** — AI-generated with hand-painted prompts + RO palette
- No visible tile grid? **Yes** — SeamlessTile generation + macro variation + distance blend
- Dirt/earth at transitions? **Yes** — noise-driven dirt patches + slope transition zone
- Triplanar cliff faces? **Yes** — WorldAlignedTexture on rock layer
**Verdict**: The system covers all 7 visual techniques identified in Section 1. Ready for implementation.

---

## Appendix A: ComfyUI Custom Node Installation

```bash
cd C:/ComfyUI/custom_nodes
git clone https://github.com/spinagon/ComfyUI-seamless-tiling.git
# Restart ComfyUI after installation
```

## Appendix B: Existing Texture Files

Current textures at `Content/SabriMMO/Textures/Environment/`:

| File | Size | Status |
|------|------|--------|
| T_Ground_GrassDirt_RO.png | 2048x2048 | Replace with new pipeline |
| T_Ground_Dirt_RO.png | 2048x2048 | Replace with new pipeline |
| T_Ground_Cobble_RO.png | 2048x2048 | Replace with new pipeline |
| T_Ground_Stone_RO.png | 2048x2048 | Replace with new pipeline |
| T_StoneWall_RO.png | 1024x1024 | Keep for buildings |
| T_Grass_RO.png | 1024x1024 | Replace with new pipeline |
| T_WoodPlanks_RO.png | 1024x1024 | Keep for buildings |
| T_DirtPath_RO.png | 1024x1024 | Replace with new pipeline |
| T_Cobblestone_RO.png | 1024x1024 | Replace with new pipeline |
| T_DungeonFloor_RO.png | 1024x1024 | Replace with new pipeline |
| T_DungeonWall_RO.png | 1024x1024 | Keep for dungeons |

## Appendix C: Key Sources

- UE5 Auto Landscape Material — Jenna Soenksen
- World of Level Design — 9 Must-Haves for Auto-Landscape Materials
- 80.lv — Triplanar Deep Dive Parts 1 & 2
- Epic Games — Landscape Materials Documentation (UE5.7)
- Epic Games — Runtime Virtual Texturing Quick Start
- RO Color Palette — Lospec (210 colors)
- RO GND File Format — Ragnarok Research Lab
- Procedural Stochastic Textures — Deliot & Heitz 2019
- ComfyUI-seamless-tiling — spinagon (GitHub)
- Generating 4K PBR Textures Using SDXL — Casey Primozic
- Tiled Diffusion — Madar & Fried (CVPR 2025)

---

## 18. Implementation Results & Lessons Learned (2026-03-30)

### What Was Built

| Component | File | Status |
|-----------|------|--------|
| ComfyUI seamless-tiling nodes | `C:/ComfyUI/custom_nodes/ComfyUI-seamless-tiling/` | Installed |
| 4x-UltraSharp upscaler | `C:/ComfyUI/models/upscale_models/4x-UltraSharp.pth` | Installed |
| Texture generator v2 | `_tools/generate_ground_textures_v2.py` | Working |
| All-inclusive variants | `_tools/generate_allinone_textures.py` | Working |
| Iteration 2 prompts | `_tools/generate_ground_iter2.py` | Working |
| 1K->2K upscaler | `_tools/upscale_and_fix_textures.py` | Working |
| Depth/Normal/AO generator | `_tools/generate_depth_maps.py` | Working |
| Laplacian seamless fix | `_tools/fix_seamless_aggressive.py` | Working |
| UE5 material creator | `Scripts/Environment/create_landscape_material.py` | Working |
| UE5 texture importer | `Scripts/Environment/import_ground_textures.py` | Working |
| UE5 texture assigner | `Scripts/Environment/assign_landscape_textures.py` | Working |
| M_Landscape_RO material | `/Game/SabriMMO/Materials/Environment/M_Landscape_RO` | Working |

### Texture Inventory

| Location | Count | Resolution |
|----------|-------|-----------|
| `Ground/` | 43 diffuse + 43 normal + 43 depth + 43 AO | 1024x1024 |
| `Ground_2K/` | 18 diffuse + 18 normal + 18 depth + 18 AO | 2048x2048 |
| `Ground_Seamless/` | 18 Laplacian-fixed diffuse | 2048x2048 |

### Critical Lessons Learned

#### 1. SeamlessTile Node Is Not Enough
The ComfyUI `SeamlessTile` node (circular Conv2d padding) does NOT produce truly seamless textures with Illustrious-XL. Edge errors remain 5-25 on a 0-255 scale. **MUST** post-process with Laplacian pyramid multi-band blending (`fix_seamless_aggressive.py`).

#### 2. Three Layers of Seam-Hiding Required
No single technique eliminates tile seams. You need all three:
- **Texture level**: Laplacian pyramid blend (fixes edge discontinuities)
- **UV distortion**: Noise-based UV offset warps seam lines into wavy curves (not straight grid)
- **Irrational tile ratios**: Each texture layer tiles at a different size (4000, 5173, 3347, 2891 UU) so seams from different layers never stack

#### 3. Prompt Engineering — Describe Colors, Not Plants
Illustrious-XL generates leaf/blade/clover shapes when you use "grass" as a noun. The winning prompts describe **abstract color fields**:
- GOOD: "overhead view of painted ground surface, oil painting style, olive green and ochre brown color fields"
- BAD: "grass texture, green grass, grass blades"
- Add to negative: "individual leaves, individual grass blades, clover, shamrock, leaf shapes, stems"

#### 4. Generation Settings
| Setting | Value | Why |
|---------|-------|-----|
| CFG | 6.5 | NOT 7.0 — critical for texture quality |
| Steps | 60 | Higher = sharper texture detail |
| LoRA | 0.15 | 0.3+ causes sprite artifacts on textures |
| Sampler | euler | Best for SDXL textures |

#### 5. Tile Size Must Match World Scale
Always check the actual floor/landscape UU dimensions before setting tile sizes. 500 UU on a 40000 UU floor = 80 tiles = visible grid. 4000 UU = 10 tiles = acceptable.

#### 6. Noise Scale Must Match World
Noise scale 0.0005 on a 400 UU area = barely any variation (constant blend). Need 0.004+ for visible splotchy patches on large floors.

#### 7. UE5 Python Material API
- `mat.get_editor_property("expressions")` is **PROTECTED** — cannot iterate expressions
- Set textures during node creation via `node.set_editor_property("texture", tex)`
- For re-runs: `mel.delete_all_material_expressions(mat)` clears the graph safely
- Never `delete_asset` on in-use materials (crashes with native reference errors)
- `MaterialEditingLibrary.set_material_instance_texture_parameter_value()` needs the parameter to exist in parent

#### 8. Best Texture Winners
| Layer | Texture | Prompt group |
|-------|---------|-------------|
| GrassWarm | T_LushGreen_03 | Watercolor green splotches |
| GrassCool | T_LushGreen_02 | Green/brown camo blend |
| Dirt | T_EarthPath_01 | Warm ochre painted bumps |
| Rock | T_StoneCliff_02 | Purple-gray painted boulders |
| Forest | T_MossFloor_01 | Dark moody green |
