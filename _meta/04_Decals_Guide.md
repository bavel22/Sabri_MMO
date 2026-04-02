# Guide 4: DBuffer Decals — Ground Detail

> How to add dirt patches, cracks, moss, paths, and zone-specific details to terrain using projected decals.

---

## What Are Decals?

Decals are textures projected onto surfaces — they overlay detail on the terrain without modifying the landscape material. Think of them as stickers on the ground.

```
Without decals: Uniform grass texture everywhere
With decals:    Worn dirt path here, mossy patch there, cracks near cliff base
```

They add a "lived-in, handcrafted" feel that makes each area unique.

---

## Decal Materials (5 Types)

Located at `/Game/SabriMMO/Materials/Environment/Decals/`:

| Material | Default Color | Best For |
|----------|-------------|----------|
| **M_Decal_Dirt** | Warm brown | Worn patches, smudges, general ground variety |
| **M_Decal_Cracks** | Dark gray | Stone cracks, fractures, scuff marks |
| **M_Decal_Moss** | Dark green | Vegetation, leaf scatter, organic growth |
| **M_Decal_Path** | Sandy brown | Footpaths, sand, light-colored detail |
| **M_Decal_DarkStain** | Dark purple-black | Blood stains, shadow pools, puddles, dark marks |

Each has:
- **Soft radial falloff** — fades from center to edges (no hard rectangle)
- **Noise edge distortion** — organic blobby shape (no perfect circle)
- **OpacityStrength** parameter — controls how visible the decal is
- **EdgeSoftness** parameter — controls fade width
- **Two-sided**, translucent blend mode

---

## Decal Instances (91 Ready to Use)

Located at `/Game/SabriMMO/Materials/Environment/Decals/RO_Decals/`:

These use the **original RO textures** as decal sources. Each has:
- Tint: warm brown (0.85, 0.80, 0.72)
- Opacity: 0.35 (subtle)
- Soft organic edges

**DO NOT use AI-generated textures for decals** — they look garish. The original RO textures are already the right muted, hand-painted colors.

---

## Placing Decals

### Method 1: Drag from Content Browser (Fastest)
1. Open Content Browser
2. Navigate to `/Materials/Environment/Decals/RO_Decals/`
3. **Drag any MI_RODecal_*** into the viewport
4. UE5 auto-creates a DecalActor
5. Position, rotate, scale as needed

### Method 2: Duplicate Existing
1. Select any decal actor in the viewport
2. **Ctrl+D** to duplicate
3. **W** to move, **E** to rotate, **R** to scale

### Method 3: Place Actors Panel
1. **Shift+1** to open Place Actors
2. Search **"Decal"**
3. Drag into viewport
4. In Details panel, set Decal Material to any MI_RODecal_*

---

## Positioning Tips

| Setting | Value | Why |
|---------|-------|-----|
| **Rotation Pitch** | -90 | Projects straight down onto ground |
| **Rotation Yaw** | Random 0-360 | Each decal faces different direction |
| **Scale X** | 1.5-5.0 | Controls projection depth |
| **Scale Y/Z** | 1.5-5.0 | Controls width/height on ground |
| **Location Z** | Slightly above ground | Decal projects downward from this height |

### Artistic Tips
- **Overlap** decals slightly for natural coverage
- **Vary rotation** so they don't all face the same direction
- **Mix types** — dirt + moss + cracks in the same area looks rich
- **Keep subtle** — opacity 0.25-0.40 works best. Above 0.5 looks painted-on
- **Place at transitions** — where grass meets cliff, where paths meet grass

---

## Zone-Specific Decal Usage

| Zone | Recommended Decals | Density |
|------|-------------------|---------|
| **Prontera** | Dirt paths, flower scatter, clean stone | 40-60 |
| **Geffen** | Moss, cracks, arcane stains | 30-50 |
| **Payon Forest** | Heavy leaf scatter, moss, roots | 50-70 |
| **Morroc Desert** | Sand drift, cracked earth, sparse | 30-50 |
| **Lutie Snow** | Ice patches, frost patterns | 30-50 |
| **Glast Heim** | Blood stains, bone scatter, slime | 50-80 |
| **Niflheim** | Dark stains, shadow pools, dead patches | 30-50 |
| **Dungeons** | Cracks, puddles, dark stains, rubble | 40-60 |

---

## Creating New Decal Instances

To add more variety using different RO textures:

```python
# In UE5 Python console:
mi = asset_tools.create_asset("MI_RODecal_NewName", "/Game/.../Decals/RO_Decals",
    unreal.MaterialInstanceConstant, mi_factory)
mi.set_editor_property("parent", parent_material)  # M_Decal_Dirt etc.
mel.set_material_instance_texture_parameter_value(mi, "DecalTexture", texture, GLOBAL_PARAMETER)
mel.set_material_instance_vector_parameter_value(mi, "DecalTint",
    unreal.LinearColor(0.85, 0.80, 0.72, 1.0), GLOBAL_PARAMETER)
mel.set_material_instance_scalar_parameter_value(mi, "OpacityStrength", 0.35, GLOBAL_PARAMETER)
```

Or run: `exec(open(r"...\create_decal_instances_v2.py").read())`

---

## Performance

- Budget: **< 100 visible decals** per camera view
- Each decal: ~20 shader instructions (very lightweight)
- Cost scales with visible count, not total placed
- DBuffer decals are cheaper than standard deferred decals
- Enable DBuffer in Project Settings > Rendering (already active with Lumen/VSM)

---

## Prerequisite: DBuffer Enabled

- **Edit > Project Settings > Rendering > Lighting > DBuffer Decals** = checked
- If using Lumen or Virtual Shadow Maps, DBuffer is already active
- Without DBuffer, decals render but with reduced quality
