# Prontera Classic - UE5 Import Guide

A polished, Ragnarok Online classic Prontera-inspired city asset pack for Unreal Engine 5.

## What's in the box

```
prontera_pack/
├── models/                       # 17 GLB meshes (UE5 5.0+ native glTF importer)
│   ├── fountain_central.glb
│   ├── castle_palace.glb
│   ├── house_small.glb
│   ├── house_shop_medium.glb
│   ├── house_inn_large.glb
│   ├── wall_segment.glb
│   ├── wall_corner_tower.glb
│   ├── gate_main.glb
│   ├── tree.glb
│   ├── lamp_post.glb
│   ├── bench.glb
│   ├── shop_sign.glb
│   ├── well.glb
│   ├── flower_planter.glb
│   ├── barrel.glb
│   ├── crates.glb
│   └── market_stall.glb
├── images/                       # Reference preview renders for each mesh
├── textures/                     # 4 tileable 2K PBR albedo textures
│   ├── T_Cobblestone_Albedo.png
│   ├── T_Grass_Albedo.png
│   ├── T_Dirt_Albedo.png
│   └── T_RoofTiles_Albedo.png
├── prontera_layout_reference.png # Top-down concept map
├── prontera_layout.json          # Full scene placement spec (positions, rotations, scales)
├── BuildPronteraLevel.py         # Optional UE5 Python script to auto-build the level
└── UE5_IMPORT_GUIDE.md           # This file
```

All meshes are GLB (binary glTF). Coordinates in `prontera_layout.json` are in **meters**, glTF Y-up. UE5's importer auto-handles axis conversion. UE5 internally uses centimeters, so the Python builder script multiplies positions by 100.

---

## Step 1 - Project setup

1. Create a new Unreal Engine 5 project (5.1+ recommended) - **Games → Blank → No Starter Content** is fine.
2. Enable **glTF Importer** plugin (Edit → Plugins → search "glTF" → enable, restart).
3. (Optional but recommended) Enable **Python Editor Script Plugin** if you plan to run the auto-builder.

---

## Step 2 - Import meshes

1. In the Content Browser, create a folder `Content/Prontera/Meshes`.
2. Drag every `.glb` from `prontera_pack/models/` into that folder.
3. In the import dialog:
   - **Skeletal Mesh**: OFF (these are static)
   - **Generate Lightmap UVs**: ON
   - **Combine Meshes**: OFF (keep individual)
   - **Build Nanite**: ON (recommended for the buildings - massive perf win)
   - **Auto Generate Collision**: ON (`Use Complex As Simple` for walls/castle, default convex hulls fine for props)
4. After import, open each Static Mesh asset and verify scale looks right in the preview. If anything is off, adjust import scale (default 1.0).

---

## Step 3 - Import textures and create materials

1. Create folder `Content/Prontera/Textures` and drag all 4 PNGs in.
2. After import, set **sRGB = ON** for albedo textures (default is correct).
3. Create master material `Content/Prontera/Materials/M_PronteraGround`:
   - Add a **TextureSampleParameter2D** named `Albedo`.
   - Connect to **Base Color**.
   - Add a **TexCoord** node multiplied by a **ScalarParameter** named `Tiling` (default 25), feed into the texture sampler UVs.
   - Set `Roughness = 0.85`, `Metallic = 0`.
4. Right-click the master material → **Create Material Instance**, do this 4 times:
   - `MI_Cobblestone` → assign `T_Cobblestone_Albedo`, Tiling = 25
   - `MI_Grass`       → assign `T_Grass_Albedo`,       Tiling = 8
   - `MI_Dirt`        → assign `T_Dirt_Albedo`,        Tiling = 24
   - `MI_RoofTiles`   → assign `T_RoofTiles_Albedo`,   Tiling = 2 (use as decal/material override on roof faces if desired)

The GLBs already ship with baked PBR textures, so building/prop materials work out-of-the-box. The above ground materials are for the streets and grass plots.

---

## Step 4 - Build the level

You have two options:

### Option A - Automated (recommended)

1. Copy `BuildPronteraLevel.py` into your project's `Content/Python/` folder (create it if missing).
2. Open UE5 → **Window → Output Log** → switch dropdown to **Python**.
3. Run: `py BuildPronteraLevel.py`
4. The script will:
   - Create a 200×200 m ground plane
   - Apply `MI_Cobblestone`
   - Spawn every entry from `prontera_layout.json` (placed under `/Game/Prontera/Meshes/<name>`)
   - Drop in a directional light + sky atmosphere + sky light + exponential fog + post-process volume

The script reads `prontera_layout.json` — keep it next to the .py file or edit `LAYOUT_PATH` at the top.

### Option B - Manual

1. Create a new **Empty Level** (File → New Level).
2. Add **Floor**: Place Actors → Plane, scale to (200, 200, 1). Apply `MI_Cobblestone`.
3. Open `prontera_layout.json` and place each instance:
   - For each entry under `central_plaza`, `castle_district`, `south_residential`, `east_market_district`, `west_residential`, `park_corners`, `perimeter_walls`:
     - Drag the matching mesh from `Content/Prontera/Meshes` into the level.
     - Set its **Location** to `position_xyz` × 100 (UE uses cm; remember to swap axes if needed: glTF X→UE Y, glTF Z→UE X is **already handled** by the importer, so use the values as-is).
     - Set **Rotation Yaw** = `rotation_y` (degrees).
     - Set **Scale** = `scale` (uniform).
4. Add lighting:
   - **Directional Light**: pitch = -45, yaw = 35, intensity 75000 lux, temperature 6000K
   - **Sky Atmosphere** (default settings)
   - **Sky Light**: real-time capture
   - **Exponential Height Fog**: density 0.001, color slightly bluish white
   - **Post Process Volume**: Unbound = true, set bloom 0.6, auto-exposure min 1.0 / max 1.5

---

## Step 5 - Polish pass

- **Nanite**: re-import any building mesh that wasn't Naniteized (right-click in Content Browser → Nanite → Enable).
- **Lumen GI**: should be on by default in UE5 — gives you free dynamic GI on all the cobblestone/walls.
- **Foliage tool**: select the tree mesh, switch to Foliage Mode, paint extra trees in the corner parks for density.
- **Player**: drop in a `ThirdPersonCharacter` Blueprint at `(0, -90, 1)` (just outside the south gate) facing north so you spawn at the iconic entry view.
- **Skybox**: the asset pack does not include an HDRI (the generation tool was offline during build). Use **HDRIBackdrop** with any free daytime HDRI from PolyHaven (e.g., "kloofendal_28d_misty_puresky_4k.exr"), or rely on UE5's built-in Sky Atmosphere which already produces a clean blue sky.

---

## Layout summary (200x200 m)

```
                     N (+Z)
        ┌────────[ N gate ]────────┐
        │   ░░ corner park ░░     │
        │   [ CASTLE PALACE ]     │
        │     trees, fountains    │
        │                         │
[ W gate ]── west residential  east market district ──[ E gate ]
        │       (houses)        (stalls + shops)      │
        │                                             │
        │      ★ CENTRAL PLAZA ★                      │
        │      ( fountain + lamps + benches )         │
        │                                             │
        │  south residential (houses + shops + inn)   │
        │   ░░ corner park ░░    ░░ corner park ░░    │
        └────────[ S gate ]────────┘
                     S (-Z)
```

- **Center (0,0,0)**: classic Prontera fountain
- **North quarter**: large palace at (0,0,70)
- **South gate at (0,0,-100)**: the canonical Prontera entry point
- **Corner towers**: at the four corners (±100, 0, ±100)
- **Walls + 3 mid-side gates**: bordering the city, breaking up the perimeter

---

## Tips for matching the classic look closer

- The original Prontera was rendered with a **fixed isometric camera at ~45° / pitch ~30°**. To recreate that vibe in UE5, set your camera/spring-arm pitch to -30 and arm length 1500-2000 cm.
- Crank **post-process saturation** to ~1.15 and slight **chromatic vignette** off — RO had a flat-but-vivid color profile.
- For the iconic **"Prontera theme"** vibe, drop a 2D ambient music asset and play it as a Sound2D in the level Blueprint's BeginPlay.

Have fun rebuilding the capital!
