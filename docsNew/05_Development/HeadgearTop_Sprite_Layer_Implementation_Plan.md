# HeadgearTop Sprite Layer Implementation Plan

Complete implementation plan for the HeadgearTop equipment sprite layer, covering the full asset pipeline from 3D model to runtime compositing, hair separation system, and multiplayer sync.

---

## Table of Contents

1. [Current State — What Already Works](#1-current-state--what-already-works)
2. [Architecture Overview](#2-architecture-overview)
3. [How HeadgearTop Differs From Weapons](#3-how-headgeartop-differs-from-weapons)
4. [Phase 1 — Headgear .blend Template Creation](#4-phase-1--headgear-blend-template-creation)
5. [Phase 2 — Full Render](#5-phase-2--full-render)
6. [Phase 3 — Atlas Config and Packing](#6-phase-3--atlas-config-and-packing)
7. [Phase 4 — Database Assignment](#7-phase-4--database-assignment)
8. [Phase 5 — UE5 Import](#8-phase-5--ue5-import)
9. [Phase 6 — Testing](#9-phase-6--testing)
10. [Hair Separation System (Bald Body + Hair Layer)](#10-hair-separation-system-bald-body--hair-layer)
11. [Hair Hiding by Headgear](#11-hair-hiding-by-headgear)
12. [Layer Interaction Summary](#12-layer-interaction-summary)
13. [Adding More Headgear — Repeatable Workflow](#13-adding-more-headgear--repeatable-workflow)
14. [Multi-Slot Headgear Blocking](#14-multi-slot-headgear-blocking)
15. [HeadgearMid and HeadgearLow](#15-headgearmid-and-headgearlow)
16. [Future Enhancements](#16-future-enhancements)
17. [Quick Reference — Commands](#17-quick-reference--commands)

---

## 1. Current State — What Already Works

The entire C++ client pipeline, server broadcast system, and multiplayer sync for HeadgearTop are already implemented. Zero code changes are needed to get headgear sprites rendering. This is purely an asset pipeline and DB assignment task.

| Component | Status | Location |
|-----------|--------|----------|
| `ESpriteLayer::HeadgearTop` (index 5) | Done | `SpriteAtlasData.h` |
| `GetLayerSubDir(HeadgearTop)` returns `"HeadgearTop"` | Done | `SpriteCharacterActor.cpp:1172` |
| `EquipSlotToSpriteLayer("head_top")` returns HeadgearTop | Done | `SpriteCharacterActor.cpp:1184` |
| `LoadEquipmentLayer()` — generic, works for any layer | Done | `SpriteCharacterActor.cpp:1191-1386` |
| Local equip callback includes `head_top` in EquipLayerMap | Done | `AttachToOwnerActor()` lambda |
| Remote player `player:appearance` includes `head_top` | Done | `OtherPlayerSubsystem.cpp:425-445` |
| Server `broadcastEquipAppearance()` includes `head_top` | Done | `index.js:224-253` |
| `zone:ready` sends `head_top` to joining clients | Done | `index.js:6582-6619` |
| ProceduralMesh quad created for layer 5 at BeginPlay | Done | `CreateLayerQuad` loop |
| Depth ordering (+/-5 X offset) in `UpdateQuadUVs` | Done | Shared equipment layer logic |

**What is missing**: Atlas asset files on disk and `view_sprite` values assigned to headgear items in the database.

---

## 2. Architecture Overview

### Runtime Flow (Already Working)

```
Player equips headgear item (head_top slot)
  |
  v
Server: inventory:equip handler
  -> Sets equipped_position = 'head_top' in DB
  -> Calls broadcastEquipAppearance()
  -> Sends player:appearance { equipVisuals: { head_top: view_sprite_id }, weaponMode }
  |
  v
Local Client: EquipmentSubsystem.OnEquipmentChanged
  -> SpriteCharacterActor::LoadEquipmentLayer(HeadgearTop, view_sprite_id)
  -> Searches Content/SabriMMO/Sprites/Atlases/HeadgearTop/ for manifest
  -> Loads 17 per-animation atlases into LayerAtlasRegistry
  -> Resolves active atlas for current weapon mode + animation state
  -> Renders headgear quad with depth ordering
  |
  v
Remote Clients: OtherPlayerSubsystem::HandlePlayerAppearance
  -> Same LoadEquipmentLayer call on remote player's sprite actor
```

### Asset Pipeline (What We Need to Build)

```
3D Headgear Model (GLB/FBX)
  |
  v
Blender .blend Template (headgear parented to mixamorig:Head bone)
  |
  v
Dual-Pass Render (render_blend_all_visible.py --weapon-only)
  -> 17 animations x 8 directions x N frames
  -> depth_map.json (auto-computed head bone vs body center)
  |
  v
Atlas Packing (pack_atlas.py with v2 config)
  -> 17 per-animation atlas PNGs + JSONs + manifest
  -> depth_mode "always_front" (holdout occlusion, no depth arrays)
  |
  v
UE5 Import (UserInterface2D, Nearest, NoMipmaps)
  -> Content/SabriMMO/Sprites/Atlases/HeadgearTop/{type}/
  |
  v
DB Assignment (UPDATE items SET view_sprite = N)
  -> Links item to atlas via view_sprite ID
```

---

## 3. How HeadgearTop Differs From Weapons

| Aspect | Weapon Layer | HeadgearTop Layer |
|--------|-------------|-------------------|
| Bone | `mixamorig:RightHand` or `LeftHand` | `mixamorig:Head` |
| Affects animation set? | Yes (weapon mode changes idle/walk/attack) | No (hat doesn't change which animation plays) |
| Atlas config groups | Per-weapon-mode (`onehand`, `twohand`, `bow`) | Must mirror body's per-mode groups (head position differs per animation) |
| Depth mode | `"always_front"` — holdout occlusion baked from render (same as headgear) | `"always_front"` — holdout occlusion baked from render |
| Unique visuals | ~10 weapon types | ~50-300 unique headgear visuals |
| Render per class? | One .blend per weapon per **gender** (shared across classes via shared armature) | Same — one .blend per headgear per **gender**, shared across classes |
| .blend location | `3d_models/weapon_templates/` | `3d_models/Headgear/Top/` |
| Animation dir | `animations/characters/base_m/` or `base_f/` (shared armature) | `animations/characters/base_m/` or `base_f/` (shared armature) |
| view_sprite IDs | Small numbers (1=dagger, 11=bow) | rAthena canonical IDs (e.g., 101=Egg Shell) |
| Import base model | Mixamo FBX (has armature + textures) | Mixamo FBX (has armature; headgear GLB provides its own textures) |

**Critical**: Headgear CANNOT use `"shared"` group for all animations. The head position changes between `Walking` (unarmed) and `Run With Sword` (onehand). The headgear atlas config must mirror the body/weapon config's 17-animation group mapping exactly so the headgear follows the correct head position per weapon mode.

### 3.1 Depth Mode System (pack_atlas.py)

The `"depth_mode"` field in the atlas config controls how front/behind ordering is computed:

| Value | Behavior | Use For |
|-------|----------|---------|
| `"always_front"` | No `depth_front` array emitted. C++ defaults to +5 (always in front of body). | **ALL equipment** — standard for weapons, headgear, shields, garments |
| `"raw"` | Per-frame per-direction depth from the render camera. Each frame can differ. | Legacy — rarely needed now that holdout occlusion handles visibility |
| `"global"` | Global majority vote per direction across all animations. One value per direction. | Legacy — was used for weapons before holdout occlusion |

**Holdout occlusion (2026-03-27)**: The render script (`render_blend_all_visible.py`) now uses a **Blender Holdout shader** on the body during the equipment pass (Pass 2). The body is transparent but still occludes equipment behind it — only equipment pixels that are actually visible from the render camera appear in the output. This means equipment sprites have physically correct occlusion baked in at render time. Runtime always composites equipment in front (+5 offset), and the baked occlusion ensures correct appearance without needing per-direction depth arrays.

**Why `always_front` is now universal**: With holdout occlusion, there is no need for per-direction or per-frame depth toggling. Equipment that is behind the body is simply not rendered in the sprite (the holdout body blocks those pixels). Equipment that is visible renders normally. The C++ runtime always places equipment at +5 (in front of body), and the pre-baked occlusion makes this correct for all directions.

**Legacy depth modes**: `"global"` and `"raw"` are still supported by the packer and C++ runtime but are deprecated. The old system hid the body during Pass 2 (body invisible, equipment fully visible) and used `depth_front` arrays to toggle front/behind per direction at runtime. This has been replaced by holdout occlusion which is simpler and more accurate. `depth_map.json` is still generated by the render script for backwards compatibility but is ignored when `depth_mode` is `"always_front"`.

**Per-equipment override**: Each equipment config can set its own `depth_mode`. The standard for all new equipment is `"always_front"` with holdout occlusion.

---

## 4. Phase 1 — Headgear .blend Template Creation

### 4.1 Get/Create the Headgear 3D Model

Options:
- **Tripo3D**: Generate from a reference image of the headgear
- **Manual modeling**: Simple geometry in Blender (200-2000 tris)
- **Asset store**: Low-poly hat/helm models

For the first headgear, start with something simple and iconic — a **Hat** (item 2220).

### 4.2 Build the .blend Template

**Important**: Use the shared armature base body **FBX** (not the GLB). The FBX from `animations/characters/base_m/` or `base_f/` has the armature with `mixamorig:Head` bone needed for parenting. With the shared armature system, one headgear .blend per gender serves ALL classes of that gender.

1. Open Blender
2. Import a shared armature base body FBX:
   ```
   File -> Import -> FBX
   Pick any FBX from: 2D animations/3d_models/animations/characters/base_m/
   (e.g., Idle.fbx — any one works, it provides the shared armature)
   For female: use base_f/ instead
   ```
3. Import the headgear model (GLB)
4. Position the headgear on the character's head:
   - Align to the head bone location
   - Scale appropriately
5. Parent the headgear mesh to the armature's Head bone:
   ```
   Select headgear mesh
   Shift+Select armature
   Ctrl+P -> Bone -> mixamorig:Head
   ```
   This sets `parent_bone = "Head"`. The render script detects any mesh with `parent_bone` set as equipment (rendered in Pass 2).
6. Verify: Enter Pose Mode on the armature, rotate the Head bone — headgear should follow.
7. Save as:
   ```
   2D animations/3d_models/Headgear/Top/{headgear_name}_on_base_m.blend  (male)
   2D animations/3d_models/Headgear/Top/{headgear_name}_on_base_f.blend  (female)
   ```

### 4.3 File Organization

With the shared armature system, headgear .blend files are organized per gender (not per class):

```
2D animations/3d_models/Headgear/
  Top/
    egg_shell_hat_on_base_m.blend       <- base_m body FBX + hat GLB on Head bone
    egg_shell_hat_on_base_f.blend       <- base_f body FBX + hat GLB on Head bone
    helm_on_base_m.blend                <- base_m body FBX + helm GLB on Head bone
    egg_shell_hat.blend                 <- standalone headgear model (source)
    ...
  Mid/
    sunglasses_on_base_m.blend          <- future mid-slot headgear
    ...
  Low/
    pipe_on_base_m.blend                <- future low-slot headgear
    ...
```

**Gender subfolder output** (after packing):
```
Content/SabriMMO/Sprites/Atlases/HeadgearTop/
  egg_shell_hat/
    male/                               <- headgeartop_101_manifest.json + atlases
    female/                             <- headgeartop_101_manifest.json + atlases
```

The C++ `LoadEquipmentLayer` gender-aware search will find the correct gender subfolder automatically.

### 4.4 Test Frame

Run a single-frame test render to verify alignment before committing to a full render:

```bash
# Male headgear test (uses base_m shared armature animations)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/egg_shell_hat_on_base_m.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_hat_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_hat_equip" \
  --test-frame --render-size 512
```

**Verify**:
- Body pass: character without hat (hat hidden)
- Equipment pass: ONLY the hat on transparent background
- Hat is positioned correctly on the head
- Hat is at appropriate scale relative to the body

---

## 5. Phase 2 — Full Render

### 5.1 Render Command (Headgear-Only)

```bash
# Male headgear full render (uses base_m shared armature animations)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/egg_shell_hat_on_base_m.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_hat_male" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# Female headgear full render (uses base_f shared armature animations)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/egg_shell_hat_on_base_f.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_hat_female" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7
```

**Key flags**:
- `--weapon-only`: Skips body render (body already exists), renders only the bone-parented headgear mesh
- Body output path is still required (for camera framing calculation) but no body PNGs are written
- `depth_map.json` is auto-generated in the weapon-output directory

### 5.2 Output Structure

```
sprites/render_output/headgear_hat/
  Idle/                                <- 8 dirs x 8 frames = 64 PNGs
  2hand Idle/                          <- 8 dirs x 8 frames
  Walking/                             <- 8 dirs x 12 frames
  Run With Sword/                      <- 8 dirs x 12 frames
  Punching/                            <- 8 dirs x 10 frames
  Stable Sword Inward Slash/           <- 8 dirs x 10 frames
  Great Sword Slash/                   <- 8 dirs x 10 frames
  Standing Draw Arrow/                 <- 8 dirs x 8 frames
  Standing 1H Magic Attack 01/         <- 8 dirs x 8 frames
  Standing 1H Cast Spell 01/           <- 8 dirs x 8 frames
  Standing 2H Magic Area Attack 01/    <- 8 dirs x 8 frames
  Standing 2H Magic Area Attack 02/    <- 8 dirs x 8 frames
  Reaction/                            <- 8 dirs x 6 frames
  Dying/                               <- 8 dirs x 8 frames
  Sitting Idle/                        <- 8 dirs x 4 frames
  Picking Up/                          <- 8 dirs x 8 frames
  Sword And Shield Block/              <- 8 dirs x 6 frames
  depth_map.json                       <- per-frame front/behind data
```

### 5.3 Depth Ordering (Holdout Occlusion)

All equipment (headgear, weapons, shields, garments) uses `"depth_mode": "always_front"` in the atlas config. This means:
- No `depth_front` array is emitted in the atlas JSONs
- C++ always composites equipment at +5 (in front of body)
- Occlusion is physically correct because of the **holdout shader** used during rendering

**How holdout occlusion works**: During the equipment render pass (Pass 2), the body meshes are assigned a Blender Holdout material. The body becomes transparent but still acts as an occluder — equipment pixels behind the body are blocked and do not appear in the rendered sprite. Only equipment pixels that are visible from the camera angle are output. After Pass 2, body materials are restored for the next Pass 1.

This means the equipment sprite already has correct occlusion baked in. At runtime, the C++ code simply places the equipment quad at +5 (in front of body), and the transparent pixels in the sprite naturally show the body underneath. No per-direction or per-frame depth toggling is needed.

The `depth_map.json` is still generated by the render script (legacy), but the packer ignores it when `depth_mode` is `"always_front"`.

**Validated with**: dagger weapon overlay (visible blade tip when behind body, occluded handle) and Egg Shell headgear (correct head occlusion from all 8 directions).

---

## 6. Phase 3 — Atlas Config and Packing

### 6.1 Create Atlas Config

Create `2D animations/atlas_configs/headgeartop_{name}_v2.json`:

```json
{
  "version": 2,
  "character": "headgeartop_101",
  "cell_size": 1024,
  "depth_mode": "always_front",
  "animations": [
    { "folder": "Idle", "state": "idle", "group": "unarmed,onehand" },
    { "folder": "2hand Idle", "state": "idle", "group": "twohand" },
    { "folder": "Walking", "state": "walk", "group": "unarmed" },
    { "folder": "Run With Sword", "state": "walk", "group": "onehand" },
    { "folder": "Punching", "state": "attack", "group": "unarmed" },
    { "folder": "Stable Sword Inward Slash", "state": "attack", "group": "onehand" },
    { "folder": "Great Sword Slash", "state": "attack", "group": "twohand" },
    { "folder": "Standing Draw Arrow", "state": "attack", "group": "bow" },
    { "folder": "Standing 1H Magic Attack 01", "state": "cast_single", "group": "shared" },
    { "folder": "Standing 1H Cast Spell 01", "state": "cast_self", "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 01", "state": "cast_ground", "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 02", "state": "cast_aoe", "group": "shared" },
    { "folder": "Reaction", "state": "hit", "group": "shared" },
    { "folder": "Dying", "state": "death", "group": "shared" },
    { "folder": "Sitting Idle", "state": "sit", "group": "shared" },
    { "folder": "Picking Up", "state": "pickup", "group": "shared" },
    { "folder": "Sword And Shield Block", "state": "block", "group": "onehand" }
  ]
}
```

**Important notes**:
- Same 17-animation layout as `weapon_dagger_v2.json` but with `"depth_mode": "always_front"`
- Groups MUST match the body's animation groups (head position differs per weapon mode animation)
- `"character": "headgeartop_101"` — the number matches the item's `view_sprite` column in the DB (rAthena canonical IDs like 101, not sequential 1/2/3)
- `LoadEquipmentLayer` constructs the lookup name as `GetLayerSubDir("HeadgearTop").ToLower()` + `_` + `ViewSpriteId`
- `"depth_mode": "always_front"` — most headgear uses this. Override to `"raw"` for items that need direction-dependent depth
- Bow attack animation is `"Standing Draw Arrow"` (the official bow attack animation name — verify the actual FBX filename in your `base_m/` or `base_f/` animations folder, as female FBX names may differ)

### 6.2 Naming Convention

| view_sprite | Config character field | Manifest filename |
|-------------|----------------------|-------------------|
| 1 | `headgeartop_1` | `headgeartop_1_manifest.json` |
| 2 | `headgeartop_2` | `headgeartop_2_manifest.json` |
| N | `headgeartop_N` | `headgeartop_N_manifest.json` |

No collision with weapon view_sprite IDs — `LoadEquipmentLayer` uses `GetLayerSubDir()` to search in `HeadgearTop/` subdirectory (weapons search in `Weapon/`).

### 6.3 Pack Command

```bash
"C:/ComfyUI/venv/Scripts/python.exe" \
  "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_hat" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop" \
  --config "2D animations/atlas_configs/headgeartop_hat_v2.json"
```

### 6.4 Output

```
Content/SabriMMO/Sprites/Atlases/HeadgearTop/
  hat/
    male/                                ← gender subfolder (shared armature)
      headgeartop_1_manifest.json
      headgeartop_1_idle.json + .png
      headgeartop_1_2hand_idle.json + .png
      headgeartop_1_walking.json + .png
      ...  (17 per-animation atlas pairs)
    female/                              ← gender subfolder
      headgeartop_1_manifest.json
      headgeartop_1_idle.json + .png
      ...  (17 per-animation atlas pairs)
```

The C++ `LoadEquipmentLayer` gender-aware search checks `hat/male/` or `hat/female/` first, then falls back to `hat/` (gender-neutral). Existing headgear without gender subfolders continues to work.

With `"depth_mode": "always_front"`, no `depth_front` array is emitted. C++ defaults to always-in-front (+5 offset). For headgear using `"depth_mode": "raw"`, each atlas JSON includes a per-frame `depth_front` array.

---

## 7. Phase 4 — Database Assignment

Most headgear items already have `view_sprite` values populated from rAthena canonical data. Check before assigning:

```sql
-- Check existing view_sprite for a headgear item
SELECT item_id, name, view_sprite, equip_slot FROM items WHERE item_id = 5015;
-- Result: Egg Shell, view_sprite = 101, equip_slot = head_top
```

The atlas `character` field must match: `"headgeartop_{view_sprite}"`. For Egg Shell with `view_sprite = 101`, use `"character": "headgeartop_101"`.

Multiple items can share the same `view_sprite` (same visual model, different stats). Only items with matching atlas files will show headgear sprites — items without atlases silently show no headgear (warning logged).

### First Headgear Implemented

| Item | ID | view_sprite | Atlas | Gender Atlases | Status |
|------|----|-------------|-------|----------------|--------|
| Egg Shell | 5015 | 101 | `HeadgearTop/egg_shell_hat/` | male + female | Done |

---

## 8. Phase 5 — UE5 Import

Import all PNGs and JSONs into `Content/SabriMMO/Sprites/Atlases/HeadgearTop/hat/`.

**Texture settings** (same as all sprite atlases):

| Setting | Value |
|---------|-------|
| Compression | UserInterface2D |
| Filter | Nearest |
| Mip Gen Settings | NoMipmaps |
| Never Stream | On |

**Important**: Always delete old `.uasset` files before reimporting — UE5 caches aggressively and may not pick up changes.

---

## 9. Phase 6 — Testing

### 9.1 Local Player Test

1. Start server + client
2. Equip the Hat item
3. Verify: hat appears on character's head
4. Verify animations: idle, walk, attack, cast, sit, death — hat tracks head position in all states
5. Equip a weapon (dagger/bow) — hat still renders correctly with new weapon mode animations
6. Unequip hat — headgear layer disappears cleanly
7. Re-equip hat — layer reappears

### 9.2 Remote Player Test

1. Connect a second client (or use server test tools)
2. Verify: other players see the equipped hat via `player:appearance` event
3. Verify: hat appears correctly on remote player spawn
4. Verify: hat appears when joining a zone where another player already wears one

### 9.3 Edge Cases

- Equip headgear, then zone transition — hat should persist (sent in `zone:ready`)
- Equip headgear, then swap to different headgear — old atlas replaced by new one
- Equip headgear with no atlas files (view_sprite assigned but no manifest) — warning logged, no crash

---

## 10. Hair Separation System (Bald Body + Hair Layer)

This section covers the optional system for separating hair from the body, enabling hair customization (style + color) and proper hair hiding by headgear.

### 10.1 Why Separate Hair?

Currently, hair is baked into the body atlas (part of the Tripo3D model). This means:
- Hair cannot be changed independently (style or color)
- Headgear cannot truly "hide" hair — it can only visually cover it
- All characters of the same class have identical hair

Separating hair enables:
- Hair style selection (24 styles per gender, like RO Classic)
- Hair color selection (9 colors via tint)
- Clean hair hiding when equipping full helms/hoods

### 10.2 Creating a Bald Body Model in Blender

The existing Mixamo animation FBXs do NOT need to be re-downloaded or re-rigged. Only the base mesh needs editing.

**Step 1: Import the base FBX**

```
File -> Import -> FBX
Pick any Mixamo FBX (e.g., Breathing Idle.fbx from swordsman_m)
```

**Step 2: Select the body mesh, enter Edit Mode**

```
Click the mesh object (not the armature) -> Tab (Edit Mode)
```

**Step 3: Select the hair geometry**

Tripo3D models are a single mesh — hair is not a separate object. Use these selection tools:

- Switch to **wireframe mode**: `Z -> Wireframe` (so you select through the mesh)
- **Lasso Select**: `Ctrl+RMB` drag around the hair area from a side/top view
- **Box Select**: `B` — from the side view, box select everything above the forehead line
- **Circle Select**: `C` — paint-select hair vertices (scroll wheel to resize)

Start conservative — select the obvious hair volume first, then refine. The boundary between hair and scalp on Tripo3D models is usually visible as a color/geometry transition.

**Step 4: Delete the selected faces**

```
X -> Faces
```

This leaves a hole where the hair was.

**Step 5: Fill the hole (create bald scalp)**

Select the edge loop around the hole:
```
Alt+Click on one edge of the hole (selects the entire border loop)
```

Fill the hole:
```
Ctrl+F -> Grid Fill
```

If Grid Fill doesn't work cleanly (irregular edge loop), use:
```
F (Fill) -> creates a single n-gon
Then: Ctrl+T (Triangulate Faces)
```

**Step 6: Smooth the new scalp**

With the new faces still selected:
```
Right-click -> Smooth Vertices (repeat 2-3 times)
```

Or switch to **Sculpt Mode** and use the **Smooth** brush to blend the new scalp into the head shape.

**Step 7: Fix UVs for the new faces**

The new scalp faces have no UV mapping. Quick fix:
```
Select the new faces -> U -> Project from View (from a top-down camera angle)
```

In the UV Editor, scale and move the new UVs to a skin-colored region of the existing texture. At sprite resolution with cel-shading, this is invisible — you just need roughly the right skin tone.

**Step 8: Export the bald base FBX**

```
File -> Export -> FBX
- Select: Armature + Mesh
- Apply Scalings: FBX All
- Add Leaf Bones: OFF
Save as: 2D animations/3d_models/characters/swordsman/swordsman_m_bald.fbx
```

**Step 9: Also make a bald GLB for --texture-from**

```
File -> Export -> glTF 2.0 (.glb)
Save as: 2D animations/3d_models/characters/swordsman/male_swordsman_bald.glb
```

### 10.3 Why Existing Animations Still Work

The Mixamo animation FBXs in `--anim-dir` contain **bone transform keyframes** (skeleton data), not mesh data. They say "move this bone to this position at this frame." They don't reference specific vertices.

The render script flow:
1. Loads the **base FBX** (bald version) — provides mesh + armature
2. Imports each **animation FBX** from `--anim-dir` — provides animation curves
3. Animation curves drive the same Mixamo skeleton — bald mesh deforms correctly

Same skeleton, same bone names, same bone weights on all remaining vertices. The new scalp faces auto-inherit bone weights from neighboring vertices (weighted to `mixamorig:Head`), so they follow head movement naturally.

**No re-rigging or re-animating needed.**

### 10.4 Updated File Organization

```
3d_models/characters/swordsman/
  male_swordsman_t_pose.glb           <- original with hair (keep for hair layer render)
  male_swordsman_bald.glb             <- bald (--texture-from for body render)
  swordsman_m_bald.fbx                <- bald base mesh for body render
```

### 10.5 Render Bald Body

```bash
"C:/Blender 5.1/blender.exe" --background \
  --python "C:/Sabri_MMO/2D animations/scripts/blender_sprite_render_v2.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/characters/swordsman/swordsman_m_bald.fbx" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/swordsman_m" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/swordsman_m/" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/swordsman/male_swordsman_bald.glb" \
  --subfolders
```

This replaces the existing body atlas with a bald version. The old body atlas (with hair) is no longer used.

### 10.6 Creating Hair as a Separate Layer

Hair follows the same pipeline as headgear — a separate mesh parented to `mixamorig:Head`, rendered via dual-pass, packed into atlases, loaded at runtime.

**Per hair style**:
1. Model hair mesh in Blender (or extract from original model)
2. Parent to `mixamorig:Head` bone on the character armature
3. Save as `.blend` template (e.g., `hair_01_on_swordsman.blend`)
4. Render hair-only: `--weapon-only` flag
5. Pack atlas: `character = "hair_{style_id}"`
6. Atlas path: `Content/SabriMMO/Sprites/Atlases/Hair/{style}/`

**Hair color via tinting**:
- Render hair in a neutral white/grey base color
- Apply color at runtime via `FSpriteLayerState::TintColor` and `MaterialInst->SetVectorParameterValue("Tint", HairColor)`
- 9 colors from the RO palette: Black, Brown, Blonde, Red, Blue, Purple, Green, Silver, Pink

**Hair loading** (new function, not equipment-based):
```cpp
void ASpriteCharacterActor::SetHairStyle(int32 StyleId, int32 ColorId)
{
    // Load hair atlas into ESpriteLayer::Hair
    LoadEquipmentLayer(ESpriteLayer::Hair, StyleId);

    // Apply tint color
    FSpriteLayerState& HairLayer = Layers[static_cast<int32>(ESpriteLayer::Hair)];
    if (HairLayer.MaterialInst)
    {
        HairLayer.TintColor = GetHairColorFromPalette(ColorId);
        HairLayer.MaterialInst->SetVectorParameterValue(
            TEXT("Tint"), HairLayer.TintColor);
    }
}
```

**Hair atlas config** (same 17-animation structure as headgear):
```json
{
  "version": 2,
  "character": "hair_1",
  "cell_size": 1024,
  "animations": [
    { "folder": "Idle", "state": "idle", "group": "unarmed,onehand" },
    { "folder": "2hand Idle", "state": "idle", "group": "twohand" },
    ...same 17 animations as body/headgear...
  ]
}
```

### 10.7 Time Estimate Per Model

Removing hair from a Tripo3D model in Blender: **10-15 minutes** per character class. The models are low-poly (~4000-5000 verts), so the hair region is maybe 200-500 faces. At sprite scale with cel-shading, a rough bald head looks fine.

---

## 11. Hair Hiding by Headgear

### 11.1 The Flag

Add `"hides_hair"` to the headgear manifest JSON (not the database — hair hiding is per-visual, not per-item):

```json
{
  "version": 2,
  "character": "headgeartop_5",
  "hides_hair": true,
  "cell_size": 1024,
  "atlases": [...]
}
```

No DB migration, no server changes, no network payload changes.

### 11.2 C++ Implementation (~15 lines)

**New member on SpriteCharacterActor.h**:
```cpp
bool bHairHiddenByHeadgear = false;
```

**In LoadEquipmentLayer(), after parsing manifest Root object**:

On equip:
```cpp
if (Layer == ESpriteLayer::HeadgearTop)
{
    bool bHidesHair = false;
    Root->TryGetBoolField(TEXT("hides_hair"), bHidesHair);

    if (bHidesHair)
    {
        SetLayerVisible(ESpriteLayer::Hair, false);
        bHairHiddenByHeadgear = true;
    }
    else if (bHairHiddenByHeadgear)
    {
        // Swapping from a hair-hiding headgear to one that doesn't
        SetLayerVisible(ESpriteLayer::Hair, true);
        bHairHiddenByHeadgear = false;
    }
}
```

On unequip (the existing `ViewSpriteId <= 0` early-return path):
```cpp
if (ViewSpriteId <= 0)
{
    SetLayerVisible(Layer, false);
    L.bUsingLayerV2 = false;
    L.LayerAtlasRegistry.Empty();
    L.ActiveLayerAtlas = nullptr;

    if (Layer == ESpriteLayer::HeadgearTop && bHairHiddenByHeadgear)
    {
        SetLayerVisible(ESpriteLayer::Hair, true);
        bHairHiddenByHeadgear = false;
    }
    return;
}
```

### 11.3 Transition Table

| From | To | Hair Result |
|------|----|-------------|
| No headgear | Hair-hiding headgear equipped | Hair hidden |
| No headgear | Non-hiding headgear equipped | Hair stays visible |
| Hair-hiding headgear | Swap to non-hiding headgear | Hair restored |
| Hair-hiding headgear | Unequip headgear | Hair restored |
| Non-hiding headgear | Swap to hair-hiding headgear | Hair hidden |

### 11.4 Remote Players

Same code path — `OtherPlayerSubsystem` calls the same `LoadEquipmentLayer()`. The manifest `hides_hair` flag is parsed client-side. No server involvement needed.

### 11.5 Which RO Headgear Hides Hair

| Hides Hair | Examples |
|------------|---------|
| Yes | Helm, Beret, Wizard Hat, Magician Hat, Orc Hero Helm, Feather Beret, full-coverage headgear |
| No | Ribbon, Kitty Band, Flower Hairpin, Glasses, Goggles, Angel Wing, small accessories |

General rule: if the headgear completely covers the top of the head, set `"hides_hair": true`.

---

## 12. Layer Interaction Summary

### 12.1 Render Order (Back to Front)

```
Layer 0: Shadow       — reserved, not rendered
Layer 1: Body         — base character (bald if hair separated)
Layer 2: Hair         — separate hair sprite (tinted by color)
Layer 3: HeadgearLow  — mouth items (pipe, mask)
Layer 4: HeadgearMid  — eye items (glasses, goggles)
Layer 5: HeadgearTop  — hats, helms, crowns    <-- THIS IMPLEMENTATION
Layer 6: Garment      — capes, wings
Layer 7: Weapon       — weapon in hand
Layer 8: Shield       — shield in off-hand
```

### 12.2 Depth Offset System (Holdout Occlusion)

All equipment layers are composited at `+5.0` units on X axis (in front of body). This is the only offset used since holdout occlusion was implemented (2026-03-27).

**How it works**: During the Blender render, the body uses a Holdout shader during the equipment pass. The body is transparent but occludes equipment behind it. Only visible equipment pixels are rendered into the sprite. At runtime, equipment is always placed in front (+5), and the baked occlusion in the sprite ensures correct appearance from all directions.

**Deprecated**: The old system used `depth_front` arrays to toggle between `+5.0` (front) and `-5.0` (behind) per frame and per direction. This is no longer needed — `depth_mode: "always_front"` is the standard for all equipment.

### 12.3 No Z-Fighting Between Equipment Layers

Layers at the same depth (+5.0) don't Z-fight because they occupy different screen-space regions:
- Hat pixels are on the head area
- Weapon pixels are in the hand area
- They never overlap in the rendered sprite

### 12.4 Initial Layer X Positions

Each layer's ProceduralMesh starts at `X = layerIndex * 0.3`:
- Body: X = 0.3
- Hair: X = 0.6
- HeadgearTop: X = 1.5
- Weapon: X = 2.1

These initial offsets are overridden by the `+/-5.0` depth system for equipment layers. The initial offset only matters for layers without depth data (Body, Hair before depth system).

---

## 13. Adding More Headgear — Repeatable Workflow

Once the first headgear is working, each additional headgear follows this checklist:

### Per-Headgear Checklist

- [ ] Get/create 3D headgear model (GLB from Tripo3D, manual Blender model, or asset store)
- [ ] Create male .blend: import any FBX from `animations/characters/base_m/` (provides shared armature), import headgear GLB, position on head, parent to `mixamorig:Head` bone. Save as `Headgear/Top/{name}_on_base_m.blend`
- [ ] Create female .blend: same with `base_f/` FBX. Save as `Headgear/Top/{name}_on_base_f.blend`
- [ ] Test frame (male): `--test-frame --render-size 512` with `--anim-dir base_m/`
- [ ] Full render male: `--weapon-only --render-size 1024` with `--anim-dir base_m/`
- [ ] Full render female: `--weapon-only --render-size 1024` with `--anim-dir base_f/`
- [ ] Check item's `view_sprite` in DB: `SELECT view_sprite FROM items WHERE item_id = {id}`
- [ ] Create v2 config: `headgeartop_{name}_v2.json` with `"character": "headgeartop_{view_sprite}"` and `"depth_mode": "always_front"` (create gender-specific config if FBX names differ)
- [ ] Pack male atlas to `HeadgearTop/{name}/male/`
- [ ] Pack female atlas to `HeadgearTop/{name}/female/`
- [ ] Set `"hides_hair": true` in manifest JSON if headgear covers hair (future -- requires hair separation)
- [ ] Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps, Never Stream)
- [ ] Test: equip/unequip, all animation states, weapon mode changes, remote player sync

### Headgear Implementation Status

| Item | ID | view_sprite | depth_mode | Atlas Location | Gender Atlases | Status |
|------|----|-------------|------------|----------------|----------------|--------|
| Egg Shell | 5015 | 101 | always_front | `HeadgearTop/egg_shell_hat/` | male + female | Done |

---

## 14. Multi-Slot Headgear Blocking

**Status: IMPLEMENTED** (server/src/index.js, inventory:equip handler)

In RO Classic, some headgear occupies multiple head slots simultaneously. A full helm occupies head_top + head_mid, blocking the mid slot from being used by glasses or goggles.

### Behavior

| Action | Result |
|--------|--------|
| Equip full helm (top+mid) while wearing glasses (mid) | Glasses auto-unequipped, helm equipped to head_top |
| Try to equip glasses (mid) while wearing full helm (top+mid) | Rejected: "Cannot equip: Full Helm is blocking this slot" |
| Equip hat (top only) while wearing glasses (mid) | Both stay equipped — hat doesn't block mid |
| Unequip full helm | Mid slot freed — glasses can be equipped again |

### Implementation

The server reads `equip_locations` from the items table (rAthena format: `Head_Top,Head_Mid`) and:

1. **Forward blocking**: When equipping a multi-slot headgear, auto-unequips items in ALL secondary slots it occupies (with stat bonus removal via `removeOldBonuses`)
2. **Reverse blocking**: When equipping a single-slot headgear, checks if any existing headgear in other slots has `equip_locations` that includes this slot — rejects with error if blocked

### Data Source

The `equip_locations` column in the `items` table stores the rAthena location string:
- `Head_Top` — single slot
- `Head_Top,Head_Mid` — blocks both slots
- `Head_Low,Head_Mid,Head_Top` — blocks all three

This data was imported by `generate_canonical_migration.js` and is already populated for all 6,169 items.

### Client Impact

None — the client reacts to `inventory:data` which reflects the server's equipment state. Auto-unequipped items disappear from the equipment panel automatically. The sprite system clears layers for unequipped slots via the existing `LoadEquipmentLayer(layer, 0)` path.

---

## 15. HeadgearMid and HeadgearLow

The same pipeline works identically for HeadgearMid (index 4) and HeadgearLow (index 3). The only differences:

| Property | HeadgearTop | HeadgearMid | HeadgearLow |
|----------|-------------|-------------|-------------|
| ESpriteLayer | `HeadgearTop` (5) | `HeadgearMid` (4) | `HeadgearLow` (3) |
| SubDir | `HeadgearTop` | `HeadgearMid` | `HeadgearLow` |
| equip_slot | `head_top` | `head_mid` | `head_low` |
| Bone | `mixamorig:Head` | `mixamorig:Head` | `mixamorig:Head` |
| Character field | `headgearmid_N` | `headgearmid_N` | `headgearlow_N` |
| Atlas path | `HeadgearTop/{type}/` | `HeadgearMid/{type}/` | `HeadgearLow/{type}/` |
| Typical items | Hats, helms, crowns | Glasses, goggles | Mouth items (pipe, mask) |
| Hides hair? | Some items | Never | Never |

All three use the same bone (`mixamorig:Head`) because they're all head accessories. The positioning difference (top of head vs eyes vs mouth) is handled in the .blend template — where you place the 3D model on the head determines where it renders in the sprite.

All three slot mappings are already wired up in C++:
- `EquipSlotToSpriteLayer("head_mid")` returns `HeadgearMid`
- `EquipSlotToSpriteLayer("head_low")` returns `HeadgearLow`
- `GetLayerSubDir(HeadgearMid)` returns `"HeadgearMid"`
- `GetLayerSubDir(HeadgearLow)` returns `"HeadgearLow"`

---

## 16. Future Enhancements

### Per-Class Headgear Rendering

If alignment issues appear between different character body types (swordsman_m vs mage_f), render class-specific headgear atlases. Would add class prefix to the atlas path and require `LoadEquipmentLayer` to incorporate body class in the search path.

### Animated Headgear

Some RO headgear has idle animations (e.g., spinning propeller hat, flapping angel wings). Could be supported by adding frame-based animation to the headgear atlas independent of the body animation. Would need a separate animation timer for the headgear layer.

### Headgear Color Variants

Some headgear comes in multiple colors. Could use the same tint system as hair — render in neutral white/grey, tint at runtime via `MaterialInst->SetVectorParameterValue("Tint", Color)`.

---

## 17. Quick Reference — Commands

### Test Frame (Single Frame, Low Res — male)
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{name}_on_base_m.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_headgear" \
  --test-frame --render-size 512
```

### Full Render (Headgear-Only, male + female)
```bash
# Male
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{name}_on_base_m.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{name}_male" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# Female
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{name}_on_base_f.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{name}_female" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7
```

### Pack Atlas (male + female to gender subfolders)
```bash
# Male
"C:/ComfyUI/venv/Scripts/python.exe" \
  "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{name}_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{name}/male" \
  --config "2D animations/atlas_configs/headgeartop_{name}_v2.json"

# Female (use gender-specific config if FBX names differ)
"C:/ComfyUI/venv/Scripts/python.exe" \
  "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{name}_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{name}/female" \
  --config "2D animations/atlas_configs/headgeartop_{name}_v2.json"
```

### Render Bald Body (After Hair Separation — future)
```bash
"C:/Blender 5.1/blender.exe" --background \
  --python "C:/Sabri_MMO/2D animations/scripts/blender_sprite_render_v2.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/characters/swordsman/swordsman_m_bald.fbx" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/swordsman_m" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/swordsman/male_swordsman_bald.glb" \
  --subfolders
```

### Render Hair Layer (future)
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/hair_templates/hair_01_on_base_m.blend" \
  --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/hair_01_male" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7
```

### DB Assignment
```sql
-- Most headgear already has view_sprite populated from rAthena. Check first:
SELECT item_id, name, view_sprite FROM items WHERE item_id = {id};

-- Only set if not already populated:
UPDATE items SET view_sprite = {N} WHERE item_id IN ({headgear_item_ids});
```
