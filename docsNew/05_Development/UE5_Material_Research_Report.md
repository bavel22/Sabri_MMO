# UE5 Material & Visual Techniques — Research Report for Sabri_MMO

> Deep research on UE5 5.7 material techniques for a 2D-sprite-in-3D-world game inspired by Ragnarok Online Classic.
> Conducted 2026-03-31. All recommendations verified against our game's architecture: sprite characters, isometric camera, hand-painted aesthetic, MMO performance requirements.

---

## Executive Summary

Our game has a unique visual challenge: 2D billboarded sprites on 3D terrain, viewed from an isometric/top-down camera, with Ragnarok Online Classic's hand-painted aesthetic. This report evaluates every relevant UE5 5.7 technique and recommends what to use, what to skip, and what to defer.

**Top 5 immediate recommendations:**
1. Add **Cell Bombing** to our landscape material (single Voronoi texture = eliminates remaining tiling)
2. Create **Material Parameter Collection** for zone mood (global warmth/saturation that transitions smoothly)
3. Add **DBuffer decals** for terrain variety (dirt patches, cracks, paths — 50-100 per zone)
4. Implement **posterized lighting in material** (RO's quantized lightmap look, terrain-only, skips sprites)
5. Add **Landscape Grass Type** for detail mesh scattering (small rocks, flowers with aggressive cull distance)

**Do NOT pursue:**
- Substrate materials (overkill for our diffuse-only terrain)
- Nanite landscape (buggy in 5.7, our zones are small enough without it)
- Post-process posterization (would affect sprites — do it in terrain material instead)
- Complex PBR specular on terrain (keep roughness 0.95)

### Benefits at a Glance — What the Player Sees

| Technique                         | What changes visually                                                                                                                                     | Before                                                           | After                                                                                            |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ |
| **Cell Bombing**                  | Ground texture never repeats the same way twice. No visible tile grid even when zoomed out over huge areas.                                               | You can spot a repeating square pattern in the grass if you look | Ground looks like one continuous hand-painted surface                                            |
| **Posterized Lighting**           | Shadows on the ground have hard stair-step edges like RO Classic instead of smooth gradients. Gives the terrain that distinctive "painted lightmap" look. | Smooth modern shadows that clash with the 2D sprite aesthetic    | Stepped, hand-painted-looking shadows that match RO Classic's visual identity                    |
| **Material Parameter Collection** | When walking from Prontera (warm, bright) to a dungeon (dark, desaturated), the ground color smoothly shifts over 1-2 seconds instead of snapping.        | Hard visual cut at zone boundary, or same mood everywhere        | Smooth, atmospheric zone transitions — warm fields, cold mountains, gray wastelands              |
| **DBuffer Decals**                | Random dirt patches, worn paths, cracks, and moss appear on the ground without changing the base material. Each zone feels handcrafted.                   | Uniform ground texture everywhere                                | Lived-in world with footpaths, erosion, scattered debris                                         |
| **Landscape Grass Type**          | Small 3D grass blades, flowers, pebbles, and debris scatter across the ground, fading at distance. Ground feels alive.                                    | Flat painted surface                                             | Ground has visible 3D detail meshes growing from it (like the reference screenshot likethis.png) |
| **Blob Shadow Polish**            | Character shadows stretch toward the sun direction and change color per zone, making sprites feel more grounded in the 3D world.                          | Simple circular dark spot under character                        | Directional, zone-tinted shadow that connects sprite to terrain                                  |
| **Stencil Masking**               | Terrain effects (posterization, color grading) only affect the ground — sprite characters keep their clean, pre-rendered look.                            | Post-process effects wash out sprites or make them look wrong    | Sprites stay crisp, terrain gets stylized — best of both worlds                                  |
| **Gooch Shading (dungeons)**      | Dungeon shadows become colored (blue-purple) instead of pure black, keeping enemies and items visible even in dark areas.                                 | Black shadows hide gameplay in dungeons                          | Colored shadows maintain visibility while feeling dark and moody                                 |

---

## 1. Anti-Tiling: Cell Bombing (RECOMMENDED — Immediate)

### Visual Benefit
**The ground never looks like a repeating tile again.** Right now if you zoom out, you can still spot the rectangular tile pattern in some areas. Cell bombing makes every section of ground show a slightly different part of the texture, so it reads as one continuous painted surface — exactly like the RO Classic screenshots.

### What it does
Adds a Voronoi noise texture that randomly offsets UV coordinates per cell area, breaking visible repetition. Unlike our current UV noise distortion (which warps seam LINES), cell bombing randomizes the POSITION of each tile copy so no two adjacent areas show the same part of the texture.

### How it works in material
```
TextureCoordinate (base UVs)
  -> Sample Voronoi noise at CellSize scale
  -> Multiply Voronoi output by OffsetStrength
  -> Add offset to base UVs
  -> Sample main texture with offset UVs
  -> Lerp between normal and offset texture using Voronoi alpha
```

### Settings
- Cell size: 10x larger than texture tile size (if tile = 4000 UU, cell = 40000 UU)
- Offset strength: 0.3-0.7 (higher = more randomization)
- Voronoi texture: 256x256 is sufficient (small, reusable)
- Set Sampler Source to "Shared: Wrap" to avoid sampler limit

### Performance
- +1 extra Voronoi texture sample + minimal math
- Combined with our existing UV distortion + irrational tile ratios = virtually zero visible repetition

### Verdict: **USE** — single highest-impact improvement we can make to tiling quality.

---

## 2. Posterized Lighting in Material (RECOMMENDED — Medium-term)

### Visual Benefit
**This is THE single most important visual technique for matching RO Classic.** The stepped, banded shadows on terrain are what make RO instantly recognizable. Without it, our terrain has smooth modern shadows that clash with the 2D sprite aesthetic. With it, the ground looks like it belongs in Ragnarok Online — painted, stylized, and cohesive with the sprite characters.

### What it does
Reproduces RO Classic's distinctive stepped shadow look by quantizing light-to-surface angles into discrete bands.

### Why in material, NOT post-process
Post-process posterization would affect our 2D sprites (which have their own pre-baked shading). By doing it in the terrain material only, sprites remain untouched.

### Material node chain
```
DotProduct(SunVector, PixelNormalWS)   // light angle
  -> Multiply(StepCount)               // e.g., 8 bands
  -> Floor                             // quantize
  -> Divide(StepCount)                 // normalize back to 0-1
  -> Clamp(0, 1)
  -> Lerp(ShadowColor, LitColor, result)  // apply stepped lighting
```

### Parameters to expose
- `LightStepCount` (4-16, default 8 — matches RO's 16-level quantization)
- `PosterizeStrength` (0-1, default 0.5 — blend between smooth and stepped)
- `ShadowWarmth` (color — warm brown shadows, not black)

### Verdict: **USE** — this is THE defining visual of RO Classic terrain. Should be added to M_Landscape_RO as an optional parameter.

---

## 3. Material Parameter Collection for Zone Mood (RECOMMENDED — Immediate)

### Visual Benefit
**Each zone feels completely different without needing separate materials.** Walking from warm golden Prontera fields into cold blue Lutie snow, the ground color temperature shifts smoothly over 1-2 seconds. Walking into Glast Heim, everything desaturates to near-grayscale. Walking into Morroc, everything warms to golden amber. One Blueprint call per zone transition — every terrain surface updates together.

### What it does
A single shared parameter set that ALL materials reference. When the player enters a new zone, one Blueprint call updates the global mood — every terrain material updates instantly.

### Setup
Create MPC asset with:
- `ZoneWarmth` (scalar, 0.5-1.5) — warm = Prontera, cold = Rachel
- `ZoneSaturation` (scalar, 0.3-1.2) — grayscale = Niflheim, vivid = Jawaii
- `ZoneBrightness` (scalar, -0.2 to 0.3) — dark = dungeons, bright = snow
- `ZoneFogDensity` (scalar) — per-zone fog
- `ZoneTintColor` (vector) — overall color overlay

### In terrain material
Replace our per-MI WarmthTint/SaturationMult with MPC references. This means zone transitions are instant and smooth (Timeline lerp) instead of requiring different Material Instances per zone.

### In Blueprint (zone transition)
```
OnZoneEnter:
  SetScalarParameterValue(MPC, "ZoneWarmth", 1.05)  // warm Prontera
  SetScalarParameterValue(MPC, "ZoneSaturation", 0.95)
```

### Verdict: **USE** — eliminates need for separate MI per zone for color grading. Simpler architecture, smoother transitions.

---

## 4. DBuffer Decals for Terrain Variety (RECOMMENDED — Medium-term)

### Visual Benefit
**The world feels lived-in and handcrafted.** Instead of uniform grass everywhere, you see worn dirt paths where NPCs walk, cracks around cliff bases, moss patches near water, scattered leaves under trees. Each zone gets unique ground detail without changing the base material. This is how AAA stylized games add "personality" to their terrain.

### What it does
Projects textures (dirt patches, cracks, moss, paths) onto terrain without modifying the landscape material. Each decal is a separate actor placed in the level.

### Setup
- Enable DBuffer in Project Settings (already active if using Lumen/VSM)
- Create Decal Material: Domain = Deferred Decal, Blend Mode = Translucent
- Only enable DBuffer channels you modify (A = Color, B = Normal, C = Roughness)

### Budget per zone
| Decal Type | Texture Size | Count | Purpose |
|------------|-------------|-------|---------|
| Dirt patches | 512x512 | 30-50 | Break grass uniformity |
| Cracks/erosion | 256x256 | 20-30 | Rocky area detail |
| Worn paths | 512x512 | 10-20 | Player trail feeling |
| Moss/leaves | 256x256 | 20-30 | Forest floor detail |
| **Total** | — | **80-130** | — |

### Verdict: **USE** — adds tremendous visual variety with minimal performance cost. Independent of material complexity.

---

## 5. Landscape Grass Type (RECOMMENDED — Medium-term)

### Visual Benefit
**The ground comes alive with 3D detail.** Small grass blades, tiny flowers, pebbles, and debris scatter across the landscape — visible up close but fading at distance for performance. This is what makes the reference screenshot (likethis.png) look so rich — the ground isn't just a flat texture, it has physical objects growing from it. Combined with our painted textures, this creates the lush, detailed ground that RO-inspired games are known for.

### What it does
Spawns small static mesh instances (grass blades, flowers, small rocks, pebbles) on the landscape based on material layer weights.

### Setup
1. Create `LandscapeGrassType` asset
2. Add Grass Variety entries (static mesh + density + cull distance + scale range)
3. In landscape material, connect to `Grass Output` node
4. UE5 automatically spawns instances based on landscape paint data

### Performance settings
- **Start Cull Distance**: 3000-5000 UU (30-50 meters) — aggressive for MMO
- **End Cull Distance**: 5000-7000 UU
- **Density**: 10-30 per sq meter (not 100+ like a single-player game)
- **Use LODs** on grass mesh (3-triangle at distance)
- **Random rotation + scale** for variety (scale 0.8-1.2)

### Verdict: **USE** — adds life to the ground plane. Essential for matching the reference screenshots (likethis.png has visible grass detail). Set aggressive cull distances for MMO performance.

---

## 6. Hex Tiling (CONSIDER — Future)

### What it does
Samples textures using hexagonal grid with random rotation/offset per hex cell. Eliminates visible tiling for ANY texture. More sophisticated than our UV distortion + irrational ratios.

### Availability
- Pure UE5 material node implementation exists: [github.com/engelmanna/HexTileUnrealEngine](https://github.com/engelmanna/HexTileUnrealEngine)
- HLSL implementation by Morten Mikkelsen (original paper author)

### Performance
- 3 texture samples per pixel (vs 1 normal) — same as our current multi-layer system
- Works best for organic textures (grass, dirt, rock)

### Verdict: **DEFER** — Cell bombing is simpler and may be sufficient. Evaluate hex tiling only if cell bombing doesn't eliminate all visible repetition.

---

## 7. Stencil Masking for Sprites (RECOMMENDED — Already Partially Done)

### What we already have
- `r.CustomDepth=3` enabled in DefaultEngine.ini
- All sprite ProceduralMeshComponent layers set to `CustomDepthStencilValue = 1`
- Occlusion transparency uses stencil to skip sprites in ray trace

### What to add
- In any future post-process material (posterization, color grading), sample `SceneTexture:CustomStencil`
- Where stencil == 1, skip the effect (preserve sprite appearance)
- This lets us add terrain-only effects without affecting sprites

### Verdict: **USE** — infrastructure is already in place, just needs to be leveraged.

---

## 8. Substrate Material System (DO NOT USE)

### What it is
UE5 5.7's replacement for the fixed shading model system. Composes physical layers (slabs) instead of choosing DefaultLit/ClearCoat/etc.

### Why NOT for us
- Our terrain is pure diffuse (roughness 0.95, metallic 0.0, no specular)
- Substrate's value is in multi-layer physical materials (wet rock, car paint, skin)
- We have zero use cases for Vertical Layer, Thin Film, Anisotropy, or Haziness
- Legacy materials work identically for our needs
- Substrate adds complexity without visual benefit

### Verdict: **DO NOT USE** — no benefit for our art style.

---

## 9. Nanite Landscape (DO NOT USE — Yet)

### Current state (5.7)
- Eliminates landscape HLODs
- Supports displacement/tessellation (experimental)

### Known issues
- Displacement disappears in standalone builds
- Random chunk disappearing in editor
- 32GB+ RAM required for >2K landscapes
- Material layer blend compatibility issues

### Why not for us
- Our zones are small (~40000 UU)
- Standard landscape LOD is sufficient
- Our isometric camera doesn't need micro-polygon detail
- The bugs are too risky for production

### Verdict: **AVOID until UE 5.8-5.9** — reassess when stabilized.

---

## 10. Runtime Virtual Texturing (CONSIDER — If Needed)

### What it does
Caches complex material shading into a virtual texture. The GPU computes the full material once, then reads cached results for subsequent frames.

### When useful
- When landscape material exceeds 200 shader instructions
- When many paint layers per landscape component (5+)
- Our M_Landscape_RO_12 is ~150 instructions — below the threshold

### Performance trade-off
- Saves per-frame shader cost
- Adds GPU memory for virtual texture tiles
- Adds latency when tiles need updating (camera movement)

### Verdict: **DEFER** — our material is not complex enough to benefit yet. If we add Cell Bombing + posterized lighting and exceed 200 instructions, then evaluate.

---

## 11. Gooch Shading (CONSIDER — For Dungeons)

### What it does
Replaces black shadows with warm/cool color tones. Shadows become blue-purple instead of black, lit areas become warm yellow-orange.

### Why consider
- RO dungeons and indoor areas have colored lighting (torch orange, ghost blue)
- Gooch shading prevents pitch-black shadows that hide gameplay
- Can be zone-specific via Material Parameter Collection

### Verdict: **CONSIDER** for dungeon zones only. Not needed for outdoor zones (standard warm shadows work fine).

---

## 12. Blob Shadow Improvement (RECOMMENDED — Quick Win)

### Current state
- SpriteCharacterActor has UDecalComponent for blob shadow
- Uses runtime-created deferred decal with radial gradient

### Improvement
- Add slight **squash** toward sun direction (elongated shadow)
- Modulate opacity by **distance to ground** (weaker when jumping/elevated)
- Match blob shadow **color** to zone mood (warm brown for Prontera, cool blue for Rachel)

### Verdict: **USE** — small visual polish that helps sprites feel grounded on terrain.

---

## 13. PCG (Procedural Content Generation) for Props (CONSIDER — Future)

### What it does (UE5 5.7)
- Scatters rocks, bushes, flowers, debris procedurally across terrain
- Rule-based: density by slope, altitude, paint layer
- Non-destructive: regenerates on terrain edit

### Why consider
- Reduces manual prop placement per zone
- Ensures consistent visual density
- Works well with Landscape Grass Type (grass mesh) + PCG (larger props)

### Verdict: **CONSIDER** for future zone building. Not urgent — manual placement works for current zone count.

---

## 14. Performance Budget Summary

### Material Instruction Targets
| Material | Max Instructions | Current |
|----------|-----------------|---------|
| M_Landscape_RO_12 | 250 | ~150 |
| M_RO_Original | 100 | ~50 |
| Sprite materials | 50 | ~30 |
| Decal materials | 50 | ~20 |

### Texture Memory Budget (per zone)
| Category | Budget | Current |
|----------|--------|---------|
| Landscape textures | 40 MB | ~16 MB (4 x 2K BC1 + normals) |
| Decal textures | 10 MB | 0 (not yet implemented) |
| Props/foliage | 20 MB | minimal |
| Sprite atlases | 80 MB | ~60 MB |
| **Total per zone** | **150 MB** | **~76 MB** |

### Key Limits
- 16 texture sampler slots per material (12 usable after landscape overhead)
- 16 vertex interpolators per material
- Current M_Landscape_RO_12 uses 7 of 12 samplers — room for Cell Bombing Voronoi + posterization
- Keep total visible decals < 100 per camera view

---

## 15. Implementation Priority

| Priority | Technique | Effort | Impact | When |
|----------|-----------|--------|--------|------|
| 1 | Cell Bombing in material | Low | High | Next session |
| 2 | Material Parameter Collection | Low | High | Next session |
| 3 | Posterized lighting in material | Medium | Very High (defines RO look) | Next session |
| 4 | DBuffer decals | Medium | High | Next week |
| 5 | Landscape Grass Type | Medium | Medium | Next week |
| 6 | Blob shadow polish | Low | Low | Anytime |
| 7 | Hex tiling | Medium | Medium | If cell bombing insufficient |
| 8 | Gooch shading (dungeons) | Low | Medium | When building dungeons |
| 9 | RVT caching | Medium | Low (not needed yet) | If material > 200 instructions |
| 10 | PCG prop scattering | High | Medium | Future zone building |

---

## 16. What We Should NOT Do

| Technique | Why Not |
|-----------|---------|
| **Substrate materials** | Our terrain is pure diffuse — no physical layering needed |
| **Nanite landscape** | Buggy in 5.7, our zones are small enough without it |
| **Post-process posterization** | Would affect sprites — do in terrain material instead |
| **Complex PBR specular** | RO has zero specular on terrain — roughness stays 0.95 |
| **Normal maps on flat ground** | Barely visible from top-down camera at 0.95 roughness |
| **Lumen bounce lighting** | Overkill for our warm flat lighting style |
| **Ray-traced shadows for sprites** | Sprites are 2D billboards — blob shadow decal is sufficient |
| **Tessellation/displacement** | Stylized look doesn't benefit from geometric detail |
| **Multiple separate materials per zone** | Use Material Instances from single parent — shared shader compilation |
