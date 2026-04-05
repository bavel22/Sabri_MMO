# Research Report: Water System for Sabri_MMO

## Executive Summary

RO Classic's water is a simple animated plane with alpha blending — no physics, no swimming, no speed reduction. Water walkability is determined by the GAT tile type (0 = walkable, 1 = blocked), NOT by the water plane itself. For Sabri_MMO in UE5, the recommended approach is a **simple Static Mesh plane with a Single Layer Water material** + **trigger volumes for gameplay detection** + **server-side zone data for water areas**. Avoid the UE5 Water plugin — it's overkill and adds swimming mechanics you don't want.

---

## Key Finding 1: How RO Classic Handles Water

**Confidence:** High
**Sources:** [GAT Format Spec](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GAT.MD), [GND Format](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GND.MD), [Ragnarok Research Lab](https://ragnarokresearchlab.github.io/file-formats/rsw/)

### Water Plane Rendering
- Water is a **flat animated plane** at a fixed Y-height per map
- Water level defined in RSW file (moved to GND in later versions)
- Each ground tile corner has a "water vertex" — X/Z pinned to tile coords, Y = water level
- **Wave animation**: Sine wave modulated by RSW parameters, computed per-corner
- **Texture**: Simple texture cycling (frame-based swap timing defined in GND)
- **Alpha blending**: Standard `SrcAlpha + OneMinusSrcAlpha` compositing — no lighting or lightmap on water
- Water is rendered AFTER ground geometry, BEFORE sprites

### Walkability (GAT)
- GAT terrain types determine walkability, NOT the water plane
- Type 0 = walkable (even if underwater)
- Type 1 = blocked (impassable)
- Type 5 = cliff (impassable, snipeable)
- GAT v1.3 added a water flag (`0x0080`) marking tiles intersecting the water plane, but "the gameplay implications are unclear"
- **The water plane is purely visual** — a player walks on underwater tiles the same as dry tiles
- Blocking water = simply setting GAT tiles to type 1 under deep water areas

### Shallow Water in RO
- Shallow water is required for skills: **Aqua Benedicta** and **Water Ball**
- Maps with shallow water include: Payon, Comodo Beach, various field maps
- **No speed reduction** from walking in shallow water (movement speed is unaffected)
- **No splash visual effect** on the base character (some private servers add this)
- The only gameplay effect is enabling water-element skills on those tiles

---

## Key Finding 2: Recommended UE5 Approach

**Confidence:** High
**Sources:** [UE5 Forum - Water System vs Plane](https://forums.unrealengine.com/t/water-system-or-water-plane/672619), [80.lv Stylized Water Tutorial](https://80.lv/articles/how-to-build-stylized-water-shader-design-implementation-for-nimue)

### Three Options Compared

| Approach | Pros | Cons | Recommendation |
|----------|------|------|----------------|
| **A: Simple Plane + SLW Material** | Lightweight, full control, no swimming, ~140 instructions | No built-in interaction | **RECOMMENDED** |
| **B: UE5 Water Plugin** | Built-in buoyancy, waves, splines | Adds swimming mechanics, heavy, hard to disable swimming | AVOID |
| **C: Translucent Plane** | Simplest setup | No depth-based effects, poor sorting with sprites | AVOID |

### Why Option A (Simple Plane + Single Layer Water)

1. **Performance**: Single Layer Water (SLW) shading model is ~140 instructions — much lighter than the Water plugin
2. **No swimming**: SLW doesn't add any physics volumes or swimming states. Players just walk through it.
3. **Stylized control**: Full control over color, opacity, waves, foam — can match RO aesthetic exactly
4. **Depth-based color**: SLW supports depth-aware shallow→deep color transitions natively
5. **Sprite compatibility**: Works with billboard sprites (no complex refraction that would distort sprites)
6. **Caustics**: Can project caustic patterns onto ground beneath water via `ColorScaleBehindWater`

---

## Key Finding 3: Implementation Architecture

**Confidence:** High

### Three Layers

```
Layer 1: VISUAL (Client only)
├── Static Mesh plane at water level Z
├── Single Layer Water material (animated, stylized)
├── Optional: caustic decal on ground beneath
└── Optional: splash particle on player enter/exit

Layer 2: GAMEPLAY DETECTION (Client + Server)
├── Trigger Volume (box) covering water area
├── Client: OnOverlap → visual effects (splash, ripple)
├── Client: emit water:enter/water:exit to server
└── Server: track which players are in water

Layer 3: BLOCKING (Server authoritative)
├── Deep water: NavMesh exclusion (enemy pathfinding)
├── Deep water: GAT-equivalent blocking in ro_zone_data.js
├── Shallow water: walkable, flag for skill eligibility
└── Server rejects movement into blocked deep water tiles
```

### Shallow Water (Walkable)

- **Visual**: Water plane at Z slightly above ground (10-30 UE units)
- **Gameplay**: Player walks normally, server flags player as "in water"
- **Effects**: Optional splash particles on enter, water ripple on movement
- **Skills**: Enables Aqua Benedicta / Water Ball (server checks water flag)
- **NavMesh**: INCLUDED (enemies can walk through shallow water)
- **Speed**: No reduction (matches RO Classic behavior)

### Deep Water (Blocking)

- **Visual**: Water plane at Z well above ground (100+ UE units), darker/deeper color
- **Gameplay**: Player CANNOT enter — NavMesh excluded, server rejects movement
- **Implementation**: Box collision volume with `BlockAll` channel, or simply no walkable ground tiles
- **NavMesh**: EXCLUDED (enemies cannot pathfind through deep water)
- **Alternative**: Instead of collision blocking, just don't have walkable ground under deep water — the player can't walk where there's no floor

---

## Key Finding 4: Stylized Water Material Setup

**Confidence:** High
**Sources:** [80.lv Nimue Tutorial](https://80.lv/articles/how-to-build-stylized-water-shader-design-implementation-for-nimue), [Epic Forums Stylized Water](https://forums.unrealengine.com/t/tutorial-how-we-built-a-stylized-water-shader-in-unreal-engine-5-no-plugins-needed/2622477)

### Material Configuration

```
Shading Model: Single Layer Water
Blend Mode: Opaque (SLW handles transparency internally)
Two-Sided: No
```

### Parameters for RO-Style Water

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Shallow Color** | (0.15, 0.45, 0.35) | Warm teal-green (RO's signature water) |
| **Deep Color** | (0.05, 0.15, 0.25) | Dark blue-green |
| **Opacity** | 0.6-0.8 | Semi-transparent, can see ground below |
| **Roughness** | 0.3-0.5 | Some reflection but not mirror |
| **Wave Speed** | 0.3-0.5 | Gentle, calm animation |
| **Wave Height** | 2-5 UE units | Subtle vertex displacement |
| **Normal Intensity** | 0.3-0.5 | Soft ripples, not aggressive |

### Material Node Graph (Simplified)

```
[Time] → [Sine] → [WPO Z-axis] (gentle wave vertex displacement)

[Normal Map A] → [Panning UV (speed 0.03, dir 1,0)]  ┐
[Normal Map B] → [Panning UV (speed 0.02, dir 0,1)]  ├→ [BlendAngleCorrectNormal] → Normal
                                                       ┘
[SceneDepth] - [PixelDepth] → [Clamp 0-1] → [Lerp(ShallowColor, DeepColor)] → Base Color

[SceneDepth] - [PixelDepth] → [OneMinus] → [Pow, 3] → Foam mask (white foam at edges)

[Voronoi Noise, panning] → ColorScaleBehindWater (caustics on ground)
```

### Caustics (Optional but Nice)

- Generate Voronoi noise pattern
- Pan it slowly in two directions
- Connect to `ColorScaleBehindWater` input on SLW material
- This projects light patterns onto the ground beneath the water surface
- Very RO-appropriate — adds visual interest at zero gameplay cost

---

## Key Finding 5: Server-Side Water Integration

**Confidence:** High

### ro_zone_data.js Integration

Add water area definitions to each zone:

```javascript
prontera: {
    // ...existing zone data...
    waterAreas: [
        {
            id: 'prt_canal_01',
            type: 'shallow',     // 'shallow' | 'deep'
            x: -500, y: -200,    // center position
            width: 200, depth: 400, // area bounds
            waterLevel: 50,       // Z height of water surface
        },
        {
            id: 'prt_moat_01',
            type: 'deep',
            x: -1000, y: -500,
            width: 100, depth: 800,
            waterLevel: 80,
        }
    ]
}
```

### Server Water Tracking

```javascript
// Track players in water
socket.on('water:enter', ({ areaId }) => {
    player.inWater = true;
    player.waterAreaId = areaId;
    // Enable water skills (Aqua Benedicta, Water Ball)
});

socket.on('water:exit', () => {
    player.inWater = false;
    player.waterAreaId = null;
});
```

### NavMesh Impact

- **Shallow water**: Keep NavMesh walkable (enemies wade through)
- **Deep water**: Exclude from NavMesh (natural pathfinding barrier)
- After adding/removing water volumes, re-export NavMesh

---

## Key Finding 6: Client-Side Water Detection

**Confidence:** High

### C++ Water Volume Actor

Create a simple `AWaterArea` actor with a box trigger:

```cpp
UCLASS()
class AWaterArea : public AActor
{
    UPROPERTY(EditAnywhere) UBoxComponent* WaterVolume;
    UPROPERTY(EditAnywhere) FString WaterAreaId;
    UPROPERTY(EditAnywhere) bool bIsShallow = true;
    UPROPERTY(EditAnywhere) float WaterLevel = 50.f;

    // OnOverlapBegin → emit water:enter to server
    // OnOverlapEnd → emit water:exit to server
    // OnOverlapBegin → spawn splash particle
};
```

### Visual Effects on Enter/Exit

- **Splash particle**: Small Niagara burst on enter/exit (optional)
- **Ripple**: Continuous subtle ripple at player feet while in water (optional)
- **Sound**: Water wading SFX loop while in shallow water (future audio system)
- **Sprite tint**: NOT recommended — would require modifying sprite material

---

## Recommendations (Prioritized)

### Phase 1: Basic Water Visual (Do First)
1. Create `M_Water_RO` material using Single Layer Water shading model
2. Create the material in C++ (runtime, like PostProcessSubsystem materials) OR in editor
3. Place a simple Static Mesh plane in L_PrtSouth at a canal/pond location
4. Apply the water material — verify it looks good with the isometric camera

### Phase 2: Gameplay Detection
1. Create `AWaterArea` C++ actor with box trigger
2. Place in levels at water locations
3. Client emits `water:enter`/`water:exit` socket events
4. Server tracks water state for skill eligibility

### Phase 3: Deep Water Blocking
1. Deep water areas: exclude from NavMesh
2. Server rejects pathfinding through deep water tiles
3. No collision volume needed — just no walkable ground

### Phase 4: Polish
1. Caustics on ground beneath water
2. Splash particles on enter/exit
3. Water wading sound effects
4. Foam at shoreline edges

---

## Areas of Disagreement

- **Speed reduction in water**: RO Classic does NOT reduce speed in shallow water. Some private servers and remakes add this as a house rule. Recommendation: match Classic (no speed reduction).
- **UE5 Water plugin**: Some developers prefer it for convenience, but multiple sources recommend against it for stylized games due to complexity and unwanted swimming mechanics.

## Gaps & Limitations

- Exact RO water texture cycling frame count and timing not documented in available sources
- RO water normal map / ripple texture not available (would need to recreate)
- UE5 Single Layer Water interaction with Unlit billboard sprites (our sprites) needs testing — SLW refraction could distort sprites behind water

---

## Sources

1. [GAT Format Specification](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GAT.MD) — Terrain types, walkability flags (5/5)
2. [GND Format Specification](https://github.com/Duckwhale/RagnarokFileFormats/blob/master/GND.MD) — Water vertex, wave parameters (5/5)
3. [RSW Format Specification](https://ragnarokresearchlab.github.io/file-formats/rsw/) — Water level, scene configuration (5/5)
4. [Ragnarok Research Lab](https://ragnarokresearchlab.github.io/rendering/) — Rendering overview (5/5)
5. [80.lv Stylized Water Shader](https://80.lv/articles/how-to-build-stylized-water-shader-design-implementation-for-nimue) — SLW material setup (4/5)
6. [UE5 Forum - Water System vs Plane](https://forums.unrealengine.com/t/water-system-or-water-plane/672619) — Performance comparison (3/5)
7. [UE5 Forum - Stylized Water Tutorial](https://forums.unrealengine.com/t/tutorial-how-we-built-a-stylized-water-shader-in-unreal-engine-5-no-plugins-needed/2622477) — 140 instruction cost (3/5)
8. [iRO Wiki - Water](https://irowiki.org/wiki/Water) — Water property, skills (4/5)
9. [iRO Wiki - Shallow Water Maps](https://irowiki.org/wiki/List_of_Maps_with_shallow_water) — Map list (4/5)
10. [ArtStation - Free Stylized Water](https://www.artstation.com/artwork/zPWQod) — Free shader reference (3/5)
