# Guide 1: UE5 Landscape System

> How to create, sculpt, and configure terrain for game zones.

---

## What Is a Landscape?

A UE5 Landscape Actor is heightfield-based terrain that supports:
- Sculpting (hills, valleys, cliffs) with brush tools
- Automatic texture blending (grass on flat, rock on slopes)
- Paint layers for manual control
- Grass/foliage spawning from the material
- Built-in LOD and collision

---

## Creating a Landscape

1. **Shift+2** to enter Landscape Mode
2. Click **Create** tab
3. Settings:
   - **Material**: `M_Landscape_RO_17` (latest with paint layers)
   - **Section Size**: 63x63 Quads
   - **Number of Components**: 4x4 to 8x8
4. Click **Create**

---

## Sculpting Terrain

Switch to **Sculpt** tab in Landscape Mode:

| Tool | Shortcut | What it does |
|------|----------|-------------|
| **Sculpt** | Default | Left-click raises, Shift+click lowers |
| **Smooth** | — | Softens harsh edges |
| **Flatten** | — | Makes area flat at clicked height |
| **Erosion** | — | Natural weathering look |
| **Noise** | — | Random bumpy detail |

### Brush Settings
- **Brush Size**: `[` and `]` to resize
- **Brush Falloff**: 0-1, higher = softer edges
- **Tool Strength**: 0-1, higher = stronger effect

### Sculpting for Gameplay
- **0-40 degrees** = walkable (grass texture shows)
- **40-53 degrees** = transition zone
- **53-90 degrees** = impassable cliff (rock texture shows automatically)
- Sculpt steep slopes (>45 deg) wherever players should NOT go
- This matches the NavMesh exclusion zones on the server

---

## Applying Materials

Drag any landscape material from Content Browser onto the Landscape:

| Material | What it does |
|----------|-------------|
| `M_Landscape_RO_17` | Full-featured: 23 params, slope detection, paint layers, noise blending |
| `M_Landscape_RO_09` | Simple: good visual, hard 45-deg cutoff, no paint layers |
| `M_RO_Original` | Lightweight: single texture + cliff, for original RO textures |
| Any `MI_*` variant | Pre-configured zone look (Prontera, Morroc, Lutie, etc.) |

### Changing Zone Look
Browse `/Game/SabriMMO/Materials/Environment/v3/` for zone-specific variants.
Each has unique warmth, saturation, and textures matching an RO zone.

---

## Paint Layers (M_Landscape_RO_17)

With v17 material applied:

1. **Shift+2** → Landscape Mode → **Paint** tab
2. Three layers available:
   - **GrassDense** — where thick grass grows
   - **FlowerPatch** — where flowers appear
   - **Debris** — where rocks/leaves scatter
3. Click **+** next to each → **Create Layer Info** → save
4. Select layer, paint on landscape
5. Grass appears where you paint (connected via Grass Output node)

---

## Per-Zone Configuration

Each zone should have:
1. A **Landscape Actor** with sculpted terrain
2. A **Material Instance** from the zone's variant set (v3 folder)
3. **Decals** placed for ground variety
4. **GrassTypes** connected for foliage scatter
5. **PostProcessSubsystem** handles lighting/fog per zone automatically
