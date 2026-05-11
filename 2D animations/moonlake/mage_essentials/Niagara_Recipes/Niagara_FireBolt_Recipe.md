# Niagara Recipe — Fire Bolt projectile (SubUV)

This is a one-emitter Niagara System that uses the Paper2D sprite sheet as a SubUV
texture. Use this if you want fewer draw calls than Paper2D Flipbooks and easier
GPU scaling for high bolt counts.

## Sheet info (matches all sheets in this pack)
- File: `SpriteSheets/vfx_fire_bolt_proj_spritesheet.png`
- Frame size: 256 x 256
- Grid: 5 columns x 5 rows
- Frames: 25
- Frame rate target: ~24 fps for projectile loop, 30 fps for impacts

## System: NS_FireBolt_Projectile

1. **Create** Niagara System -> Empty.
2. **Add Emitter** -> "Sprite (CPU)" template.
3. **Properties**
   - Sim Target: CPU (switch to GPU later for huge bolt counts).
4. **Emitter Update**
   - Spawn Rate: 0
   - Spawn Burst Instantaneous: Count = 1, Time = 0.0
5. **Particle Spawn**
   - Initialize Particle: Lifetime = 1.5
   - Sphere Location: Radius = 0 (we're driving translation via the actor's movement component)
6. **Particle Update**
   - Add module: **Sub UV Animation**
     - Mode: Linear
     - Rows: 5
     - Columns: 5
     - Start Frame: 0
     - End Frame: 24
     - Loop: true (for projectile/cast/pillar) or false (for impacts/burst)
     - Animation Time: 1.0 (full lifetime)
7. **Renderer**
   - **Sprite Renderer**:
     - Material: M_VFX_Translucent_SubUV (described below)
     - SubUV: enabled
     - SubImage Size: (5, 5)
     - SubUV Blending Enabled: true (smoother frame blends; optional)
     - Alignment: Velocity Aligned (for bolts) or Camera Plane (for impacts/cast glow/pillar)

## Material: M_VFX_Translucent_SubUV (used by all skills)

Create a Material with these settings:
- Domain: Surface
- Blend Mode: Translucent
- Shading Model: Unlit
- Used with Niagara Sprites: enabled
- Used with Particle Sprites: enabled

Graph:
```
ParticleSubUV (Texture parameter "SubUVTex")
    .RGB --> Multiply (B = Particle Color.RGB) --> Multiply (B = Emissive Strength scalar, default 2.0) --> Emissive Color
    .A   --> Multiply (B = Particle Color.A)   --> Opacity
```

Texture parameter `SubUVTex` is overridden per skill via a Material Instance:
- `MI_FireBolt_Proj`     -> `vfx_fire_bolt_proj_spritesheet`
- `MI_FireBolt_Impact`   -> `vfx_fire_impact_spritesheet`
- `MI_ColdBolt_Proj`     -> `vfx_cold_bolt_proj_spritesheet`
- ...etc for all 11 sheets.

## Per-skill systems to create
| System | Sheet | Loop |
|---|---|---|
| NS_FireCastGlow | vfx_fire_cast_glow_spritesheet | Yes |
| NS_FireBolt_Projectile | vfx_fire_bolt_proj_spritesheet | Yes |
| NS_FireImpact | vfx_fire_impact_spritesheet | No (one-shot) |
| NS_ColdCastGlow | vfx_cold_cast_glow_spritesheet | Yes |
| NS_ColdBolt_Projectile | vfx_cold_bolt_proj_spritesheet | Yes |
| NS_ColdImpact | vfx_cold_impact_spritesheet | No |
| NS_LightningCastGlow | vfx_lightning_cast_glow_spritesheet | Yes |
| NS_LightningBolt_Projectile | vfx_lightning_bolt_proj_spritesheet | Yes |
| NS_LightningImpact | vfx_lightning_impact_spritesheet | No |
| NS_FireWallBurst | vfx_fire_wall_burst_spritesheet | No |
| NS_FireWallPillar | vfx_fire_wall_pillar_spritesheet | Yes |

## Texture import settings (apply to ALL 11 sprite sheets)
- Compression Settings: **UserInterface2D (RGBA)** (preserves alpha cleanly)
- Mip Gen Settings: **NoMipmaps**
- sRGB: **true**
- Filter: **Bilinear**
- Texture Group: **UI** or **Effects**

## Tips
- Crank `Particle Color` alpha up/down to fade in/out at lifetime ends.
- For bolts, set `Initialize Particle.Sprite Size` to (60, 60) world units; tune to taste.
- For Fire Wall Pillar, anchor the system at the cell's ground center; use
  `Alignment = Camera Plane` and `Sprite Size` (180, 360) — tall billboards.
