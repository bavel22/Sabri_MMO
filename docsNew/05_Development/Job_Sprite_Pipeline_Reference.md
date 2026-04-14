# Job Sprite Pipeline Reference

Complete step-by-step guide for rendering new character class sprites using the shared armature system.

**Last updated**: 2026-04-09
**Proven with**: knight_f, merchant_f (shared armature on base_f)
**Output per class**: 17 atlas PNGs + 17 JSONs + 1 manifest = 35 files

---

## Table of Contents

1. [Current State](#current-state)
2. [Prerequisites](#prerequisites)
3. [Per-Class Pipeline (7 Steps)](#per-class-pipeline)
4. [Batch Workflow](#batch-workflow)
5. [Standard Template Config](#standard-template-config)
6. [Mixamo Animation Downloads](#mixamo-animation-downloads)
7. [Class Checklist](#class-checklist)
8. [Troubleshooting](#troubleshooting)

---

## Current State

### Shared Armatures

| Gender | Armature | Animations | Status |
|--------|----------|------------|--------|
| Female | `base_f` | 17 FBX in `animations/characters/base_f/` | Ready |
| Male | `base_m` | `animations/characters/base_m/` | **MISSING — build first** |

### Rendered Classes (shipped with body atlases)

| Class | Gender | Armature | Atlas Dir |
|-------|--------|----------|-----------|
| Swordsman | M | per-class (legacy) | `Body/swordsman_m/` |
| Swordsman | F | shared (base_f) | `Body/swordsman_f/` |
| Novice | F | shared (base_f) | `Body/novice_f/` |
| Mage | F | shared (base_f) | `Body/mage_f/` |
| Merchant | F | shared (base_f) | `Body/merchant_f/` |
| Knight | F | shared (base_f) | `Body/knight_f/` |
| Archer | F | per-class (legacy) | `Body/archer_f/` |

### Hero Reference Images

Located in `2D animations/sprites/references/hero_refs/`. Available for all classes except `monk_f`.

---

## Prerequisites

### One-Time: Build `base_m` Shared Armature

This unlocks ALL male class rendering. Only needs to be done once.

1. **Get a male base body reference image** (plain humanoid, T-pose, clean background)

2. **Tripo3D** — convert reference to 3D model:
   ```bash
   cd "C:/Sabri_MMO/2D animations"
   python scripts/tripo_image_to_3d.py \
     sprites/references/hero_refs/base_m.png \
     3d_models/characters/base_m/base_m.glb
   ```

3. **Convert GLB to FBX** (Blender, if Mixamo rejects the GLB):
   - Open Blender
   - File > Import > glTF 2.0 > `base_m.glb`
   - File > Export > FBX > `3d_models/characters/base_m/base_m.fbx`

4. **Mixamo** — upload, auto-rig, download 17 animations:
   - Go to mixamo.com, upload the FBX
   - Download each animation individually (see [Mixamo Animation Downloads](#mixamo-animation-downloads))
   - Settings for EVERY download: **"With Skin"**, **FPS=30**, **Keyframe Reduction=None**
   - Save all to: `3d_models/animations/characters/base_m/`

5. **Verify** — folder should contain these 17 FBX + 1 T-Pose:
   ```
   animations/characters/base_m/
     2hand Idle.fbx
     Dying.fbx
     Great Sword Slash.fbx
     Idle.fbx
     Picking Up.fbx
     Punching.fbx
     Reaction.fbx
     Run With Sword.fbx
     Sitting Idle.fbx
     Stable Sword Inward Slash.fbx
     Standing 1H Cast Spell 01.fbx
     Standing 1H Magic Attack 01.fbx
     Standing 2H Magic Area Attack 01.fbx
     Standing 2H Magic Area Attack 02.fbx
     Standing Draw Arrow.fbx
     Sword And Shield Block.fbx
     T-Pose.fbx
     Walking.fbx
   ```

### Required Tools

| Tool | Location | Purpose |
|------|----------|---------|
| Blender 5.1 | `C:/Blender 5.1/blender.exe` | 3D rendering |
| Python (ComfyUI venv) | `C:/ComfyUI/venv/Scripts/python.exe` | Atlas packing |
| Tripo3D API | `2D animations/scripts/tripo_image_to_3d.py` | Image → GLB |
| Render script | `2D animations/scripts/blender_sprite_render_v2.py` | 8-dir cel-shade render |
| Atlas packer | `2D animations/scripts/pack_atlas.py` | V2 per-animation packer |

---

## Per-Class Pipeline

Repeat these 7 steps for each class/gender combo. ~5-15 min per class.

Throughout this guide, replace `{class}` with the class name (e.g., `knight`) and `{gender}` with `m` or `f`.

### Step 1: Tripo3D — Hero Ref to GLB

```bash
cd "C:/Sabri_MMO/2D animations"
python scripts/tripo_image_to_3d.py \
  "sprites/references/hero_refs/{class}_{gender}.png" \
  "3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb"
```

**Requirements for the hero ref image:**
- Character in **T-pose** (arms out to sides)
- **Clean background** (white or transparent)
- Full body visible (head to toe)
- Front-facing

If the hero ref is not T-pose, edit the image first or Mixamo rigging will fail.

**Output**: `3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb`

### Step 2: Blender — Rig to Shared Armature

This is a manual Blender step. Each sub-step is described in detail.

#### 2a. Start fresh
- Open Blender
- File > New > General (start with the default scene)
- Delete the default cube, camera, and light (select each → X key)
- You should have an empty scene

#### 2b. Import the base armature
- File > Import > FBX
- Navigate to: `C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_{gender}/T-Pose.fbx`
  - Use `base_m/` for male classes, `base_f/` for female classes
  - Any FBX from the folder works (they all contain the shared armature), but `T-Pose.fbx` is cleanest
- **Import settings**: Leave defaults (especially "Armature" checked)
- After import you should see: a **body mesh** (the base body) + an **Armature** object in the Outliner

#### 2c. Import the class model
- File > Import > glTF 2.0
- Navigate to: `C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb`
- After import you should see the class outfit mesh appear, likely at a different scale or offset from the base body

#### 2d. Align the class mesh to the base body
- Select the class mesh (left-click)
- Press **S** to scale, **G** to grab/translate — match the class mesh to the base body as closely as possible
- Use front view (**Numpad 1**) and side view (**Numpad 3**) to check alignment
- Key alignment points: **feet on the ground plane**, **shoulders at same height**, **arms matching T-pose angle**
- The closer the overlap, the better the automatic weights will be

**Tip**: If the class mesh is much larger/smaller than the base, check the FBX import scale. Mixamo exports at different scales depending on the original model. You may need to scale the class mesh by 0.01 or 100x.

#### 2e. Clean the class mesh (remove old rig data)

The Tripo3D GLB may have its own armature/modifiers that will conflict. Remove them:

1. **Select the class mesh** (left-click in viewport or Outliner)
2. **Check for Armature modifier**: Open Properties panel (right side) > Modifiers tab (wrench icon)
   - If there's an "Armature" modifier → click the **X** to delete it
   - If there are other modifiers, leave them unless they're armature-related
3. **Clear parent**: With class mesh selected → **Alt+P** → **Clear and Keep Transform**
   - This removes any parent relationship from the old GLB import while keeping the mesh in place
4. **Apply all transforms**: With class mesh selected → **Ctrl+A** → **All Transforms**
   - This bakes the current position/rotation/scale into the mesh data
   - After this, the mesh's transform should read: Location (0,0,0), Rotation (0,0,0), Scale (1,1,1)
   - **This step is critical** — if you skip it, the automatic weights will be computed at the wrong scale

**If the class GLB imported its own armature object**: Select that armature in the Outliner and delete it (X key). You only want the base armature from step 2b.

#### 2f. Delete the base body mesh (keep the armature!)

The base body mesh was only needed for alignment reference. Now remove it:

1. **Identify which mesh is the base body**: In the Outliner, it's the mesh that was imported with the FBX in step 2b. It's usually named something like "Beta_Surface" or "mixamorig:Mesh" or similar.
2. **Select ONLY the body mesh** (NOT the armature) — click it in the Outliner
3. **Delete it**: Press **X** → Delete
4. **Verify**: The Outliner should now show:
   - The **Armature** (with its bone hierarchy intact)
   - The **class mesh** (your outfit model)
   - Nothing else (no extra meshes or armatures)

**WARNING**: Do NOT delete the armature. If you accidentally delete it, undo (Ctrl+Z) immediately.

#### 2g. Parent the class mesh to the base armature

This is the core step that binds the class outfit to the shared skeleton:

1. **Select the class mesh FIRST** (left-click in viewport or Outliner)
2. **Then Shift+click the Armature** (the armature should now have the orange highlight, class mesh has dark orange)
   - Order matters: mesh first, then armature
3. Press **Ctrl+P** → choose **Armature Deform** → **With Automatic Weights**
4. Blender will compute vertex weights automatically based on proximity to each bone

**If you get "Bone Heat Weighting: failed to find solution for one or more bones"**:
- The mesh has non-manifold geometry or overlapping vertices
- Fix: Select the class mesh → Edit Mode → Mesh > Clean Up > Merge by Distance (threshold 0.001)
- Then try Ctrl+P again
- If it still fails: try **With Empty Groups** instead, then manually weight paint

#### 2h. Verify the rigging

1. **Scrub the timeline**: The T-Pose FBX imported an animation. Press Play (Space) or drag the timeline slider. The class mesh should deform with the armature.
2. **Check key body parts**:
   - Arms: Should bend at elbows and shoulders
   - Legs: Should bend at knees and hips
   - Head: Should tilt/rotate
   - Hands: Fingers should curl (if the mesh has them)
3. **Enter Pose Mode** (select armature → Ctrl+Tab or dropdown): Rotate individual bones to check deformation
4. **Check for weight issues**:
   - If a limb doesn't move → that area has zero weight (needs manual weight painting)
   - If parts stretch weirdly → weights are bleeding to the wrong bone (fix in Weight Paint mode)
   - Minor imperfections are OK — sprites are small (1024px) and viewed from 8 directions

**To fix weights** (if needed):
- Select the class mesh → switch to **Weight Paint Mode** (Ctrl+Tab)
- Select the problematic bone in the armature (Ctrl+click in Weight Paint mode)
- Paint weights: Red = full influence, Blue = no influence
- Use a soft brush to smooth transitions

#### 2i. Save the .blend file

- File > Save As
- Save to: `C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_shared.blend`

Example paths:
```
3d_models/characters/thief_m/thief_m_shared.blend
3d_models/characters/priest_f/priest_f_shared.blend
3d_models/characters/wizard_m/wizard_m_shared.blend
```

**Reference**: See `3d_models/characters/knight_f/` for proven examples — multiple .blend attempts are normal (the final one is the `_done.blend` variant).

#### Blender Outliner — What Correct Setup Looks Like

After completing all steps, your Outliner should look like this:
```
Scene Collection
  └── Armature                    ← Shared base skeleton (from base_m or base_f FBX)
       └── {class mesh name}      ← Class outfit mesh (parented with Automatic Weights)
```

The class mesh's Properties panel should show:
- **Parent**: Armature name
- **Parent Type**: `OBJECT` (not BONE — this is different from equipment!)
- **Modifiers**: One "Armature" modifier pointing to the base armature

### Step 3: Blender Render — 8 Directions x 17 Animations

```bash
"C:/Blender 5.1/blender.exe" --background \
  --python "C:/Sabri_MMO/2D animations/scripts/blender_sprite_render_v2.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_shared.blend" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{class}_{gender}" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98 \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_{gender}/" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb" \
  --subfolders
```

**CRITICAL flags — do NOT change these:**

| Flag | Value | Why |
|------|-------|-----|
| `--render-size` | `1024` | Standard cell size for v2 atlases |
| `--camera-angle` | `10` | Near eye-level, RO Classic style |
| `--camera-target-z` | `0.7` | Feet visible, character centered |
| `--cel-shadow` | `0.92` | Finalized lighting — old 0.45 is too dark |
| `--cel-mid` | `0.98` | Finalized lighting — old 0.78 is too dark |
| `--anim-dir` | `base_m/` or `base_f/` | Shared armature animations — NEVER per-class |
| `--texture-from` | The original Tripo3D GLB | Recovers textures stripped by Mixamo |
| `--subfolders` | (flag) | Creates per-animation output folders |

**CRITICAL**: `--texture-from` MUST point to the **exact same GLB** that was uploaded to Mixamo for rigging. If the vertex count doesn't match, textures will be scrambled.

**Output**: 17 folders in `sprites/render_output/{class}_{gender}/`, each containing 8 directions x N frames as individual PNGs:
```
sprites/render_output/{class}_{gender}/
  Idle/
  2hand Idle/
  Walking/
  Run With Sword/
  Punching/
  Stable Sword Inward Slash/
  Great Sword Slash/
  Standing Draw Arrow/
  Standing 1H Magic Attack 01/
  Standing 1H Cast Spell 01/
  Standing 2H Magic Area Attack 01/
  Standing 2H Magic Area Attack 02/
  Reaction/
  Dying/
  Sitting Idle/
  Picking Up/
  Sword And Shield Block/
  T-Pose/        (skip — not used in atlas)
```

**Time**: ~5-10 min per class depending on hardware.

### Step 4: Create Atlas Config

Copy the standard template and change the character name:

```bash
cp "C:/Sabri_MMO/2D animations/atlas_configs/standard_template_v2.json" \
   "C:/Sabri_MMO/2D animations/atlas_configs/{class}_{gender}_v2.json"
```

Edit the new file — **only change the `"character"` field**:

```json
{
  "version": 2,
  "character": "{class}_{gender}",
  "cell_size": 1024,
  "animations": [
    ... (leave the 17 standard animations unchanged)
  ]
}
```

**When to modify animation entries**: Only if Mixamo gave different FBX filenames for `base_m` vs `base_f`. In that case, update the `"folder"` values to match the actual subfolder names in `sprites/render_output/{class}_{gender}/`.

### Step 5: Pack Atlases

```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/{class}_{gender}" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" \
  --config "2D animations/atlas_configs/{class}_{gender}_v2.json"
```

**Output**: 35 files in `Sprites/Atlases/Body/{class}_{gender}/`:
- 17 atlas PNGs (`{class}_{gender}_{sanitized_anim_name}.png`)
- 17 atlas JSONs (`{class}_{gender}_{sanitized_anim_name}.json`)
- 1 manifest (`{class}_{gender}_manifest.json`)

`sanitized_anim_name` = lowercase, spaces to underscores, stripped parentheses.

### Step 6: UE5 Import

1. Open Unreal Editor
2. Navigate to `Content/SabriMMO/Sprites/Atlases/Body/{class}_{gender}/`
3. Drag all 17 `.png` files into the folder (or use Import)
4. For EACH imported texture, set these properties:
   | Property | Value |
   |----------|-------|
   | **Compression** | `UserInterface2D` |
   | **Filter** | `Nearest` |
   | **Mip Gen Settings** | `NoMipmaps` |
   | **Never Stream** | `On` (checked) |
5. Save all
6. **Important**: The `.json` files must also be in the same directory on disk (they're loaded at runtime via FPaths, not as assets)

**Tip**: If re-importing over existing sprites, **delete the old `.uasset` files first**. UE5 caches aggressively and may not pick up the new PNGs.

### Step 7: Test

1. Start the server and client
2. Log in as a character with the matching job class
3. The sprite system auto-detects: `SetBodyClass()` finds `{class}_{gender}_manifest.json` → loads v2 atlases
4. Verify:
   - Idle animation plays
   - Walking animation plays when moving
   - Attack animation plays when attacking
   - Sit animation works (F2)
   - Death animation plays
   - Weapon mode switches work (equip dagger → 1H anims, equip two-hander → 2H anims)

**No C++ code changes needed.** The v2 manifest auto-detection handles everything.

---

## Batch Workflow

When rendering many classes in sequence, optimize by batching steps:

### Phase A: All Tripo3D conversions
Run all hero refs through Tripo3D first. This is the slowest step (API wait times).
```bash
cd "C:/Sabri_MMO/2D animations"
python scripts/tripo_image_to_3d.py sprites/references/hero_refs/thief_m.png 3d_models/characters/thief_m/thief_m_t_pose.glb
python scripts/tripo_image_to_3d.py sprites/references/hero_refs/thief_f.png 3d_models/characters/thief_f/thief_f_t_pose.glb
# ... repeat for all classes
```

### Phase B: All Blender rigging
Open each GLB in Blender, rig to shared armature, save .blend. This is manual but fast (~2-5 min each).

### Phase C: All renders (can run overnight)
Chain all render commands:
```bash
# Female classes (all use base_f)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/characters/thief_f/thief_f_shared.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/thief_f" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/thief_f_weapon_discard" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/thief_f/thief_f_t_pose.glb" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# Male classes (all use base_m)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/characters/thief_m/thief_m_shared.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/thief_m" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/thief_m_weapon_discard" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/thief_m/thief_m_t_pose.glb" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# ... repeat for all classes
```

### Phase D: All atlas packing
```bash
# One command per class
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/thief_f" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" \
  --config "2D animations/atlas_configs/thief_f_v2.json"

# ... repeat for all classes
```

### Phase E: UE5 import all at once
Import all PNGs, set texture properties, save.

---

## Standard Template Config

All classes use the same 17-animation template. Copy `atlas_configs/standard_template_v2.json` and only change `"character"`.

```json
{
  "version": 2,
  "character": "{class}_{gender}",
  "cell_size": 1024,
  "animations": [
    { "folder": "Idle",                              "state": "idle",         "group": "unarmed,onehand" },
    { "folder": "2hand Idle",                         "state": "idle",         "group": "twohand" },
    { "folder": "Walking",                            "state": "walk",         "group": "unarmed" },
    { "folder": "Run With Sword",                     "state": "walk",         "group": "onehand" },
    { "folder": "Punching",                           "state": "attack",       "group": "unarmed" },
    { "folder": "Stable Sword Inward Slash",          "state": "attack",       "group": "onehand" },
    { "folder": "Great Sword Slash",                  "state": "attack",       "group": "twohand" },
    { "folder": "Standing Draw Arrow",                "state": "attack",       "group": "bow" },
    { "folder": "Standing 1H Magic Attack 01",        "state": "cast_single",  "group": "shared" },
    { "folder": "Standing 1H Cast Spell 01",          "state": "cast_self",    "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 01",   "state": "cast_ground",  "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 02",   "state": "cast_aoe",     "group": "shared" },
    { "folder": "Reaction",                           "state": "hit",          "group": "shared" },
    { "folder": "Dying",                              "state": "death",        "group": "shared" },
    { "folder": "Sitting Idle",                       "state": "sit",          "group": "shared" },
    { "folder": "Picking Up",                         "state": "pickup",       "group": "shared" },
    { "folder": "Sword And Shield Block",             "state": "block",        "group": "onehand" }
  ]
}
```

### Group Rules Quick Reference

| Group | Weapon Modes | Example Animations |
|-------|-------------|-------------------|
| `shared` | ALL (None, OneHand, TwoHand, Bow) | cast, hit, death, sit, pickup |
| `unarmed` | None only | idle (no weapon), walk (no weapon), attack (punch) |
| `unarmed,onehand` | None + OneHand | idle (shared between unarmed and 1H) |
| `onehand` | OneHand only | attack (sword slash), block, walk (run with sword) |
| `twohand` | TwoHand only | idle (2H), attack (great sword slash) |
| `bow` | Bow only | attack (draw arrow) |

### Animations NOT Included (removed from pipeline)

| Animation | Why Removed |
|-----------|-------------|
| Fighting Idle | Attack animation frames handle combat idle visual |
| Breathing Idle | Merged into Idle (use Idle for all idle states) |
| Idle (1) / Walking (1) / Mutant Punch | Variants removed from standard set |
| Stable Sword Outward Slash | Second 1H attack variant removed |
| Standing Taunt Chest Thump | Replaced by CastAoe animation |

---

## Mixamo Animation Downloads

Download these 17 animations for each base armature. Use the EXACT search names shown below.

**Settings for ALL downloads**: "With Skin", FPS=30, Keyframe Reduction=None

| # | Mixamo Search | FBX Filename | Frames |
|---|--------------|-------------|--------|
| 1 | Idle | `Idle.fbx` | 8 |
| 2 | 2hand Idle | `2hand Idle.fbx` | 8 |
| 3 | Walking | `Walking.fbx` | 12 |
| 4 | Run With Sword | `Run With Sword.fbx` | 12 |
| 5 | Punching | `Punching.fbx` | 10 |
| 6 | Stable Sword Inward Slash | `Stable Sword Inward Slash.fbx` | 10 |
| 7 | Great Sword Slash | `Great Sword Slash.fbx` | 10 |
| 8 | Standing Draw Arrow | `Standing Draw Arrow.fbx` | 8 |
| 9 | Standing 1H Magic Attack 01 | `Standing 1H Magic Attack 01.fbx` | 8 |
| 10 | Standing 1H Cast Spell 01 | `Standing 1H Cast Spell 01.fbx` | 8 |
| 11 | Standing 2H Magic Area Attack 01 | `Standing 2H Magic Area Attack 01.fbx` | 8 |
| 12 | Standing 2H Magic Area Attack 02 | `Standing 2H Magic Area Attack 02.fbx` | 8 |
| 13 | Reaction | `Reaction.fbx` | 6 |
| 14 | Dying | `Dying.fbx` | 8 |
| 15 | Sitting Idle | `Sitting Idle.fbx` | 4 |
| 16 | Picking Up | `Picking Up.fbx` | 8 |
| 17 | Sword And Shield Block | `Sword And Shield Block.fbx` | 6 |

Also download: `T-Pose.fbx` (used as base import in Blender rigging step, not rendered).

---

## Class Checklist

### First Classes (1st job — base classes)

| Class | Male | Female | Hero Ref M | Hero Ref F | Notes |
|-------|------|--------|-----------|-----------|-------|
| Novice | TODO | DONE | novice_m.png | novice_f.png | |
| Swordsman | DONE (legacy) | DONE | swordsman_m.png | swordsman_f.png | M uses per-class rig |
| Mage | TODO | DONE | mage_m.png | mage_f.png | |
| Archer | TODO | DONE (legacy) | archer_m.png | archer_f.png | F uses per-class rig |
| Acolyte | TODO | TODO | acolyte_m.png | acolyte_f.png | |
| Thief | TODO | TODO | thief_m.png | thief_f.png | |
| Merchant | TODO | DONE | merchant_m.png | merchant_f.png | |

### Second Classes (2nd job)

| Class | Male | Female | Hero Ref M | Hero Ref F | Notes |
|-------|------|--------|-----------|-----------|-------|
| Knight | TODO | DONE | knight_m.png | knight_f.png | |
| Crusader | TODO | TODO | crusader_m.png | crusader_f.png | |
| Wizard | TODO | TODO | wizard_m.png | wizard_f.png | |
| Sage | TODO | TODO | sage_m.png | sage_f.png | |
| Hunter | TODO | TODO | hunter_m.png | hunter_f.png | |
| Bard | TODO | N/A | bard_m.png | — | Male-only class |
| Dancer | N/A | TODO | — | dancer_f.png | Female-only class |
| Priest | TODO | TODO | priest_m.png | priest_f.png | |
| Monk | TODO | TODO | monk_m.png | **MISSING** | Need monk_f hero ref |
| Assassin | TODO | TODO | assassin_m.png | assassin_f.png | |
| Rogue | TODO | TODO | rogue_m.png | rogue_f.png | |
| Blacksmith | TODO | TODO | blacksmith_m.png | blacksmith_f.png | |
| Alchemist | TODO | TODO | alchemist_m.png | alchemist_f.png | |

### Count Summary

| Category | Done | TODO | Total |
|----------|------|------|-------|
| Female 1st class | 5 | 2 | 7 |
| Male 1st class | 1 | 6 | 7 |
| Female 2nd class | 1 | 12 | 13 |
| Male 2nd class | 0 | 12 | 12 |
| **Total** | **7** | **32** | **39** |

*Bard=male only, Dancer=female only, monk_f needs hero ref*

---

## Troubleshooting

### Rendering

| Problem | Cause | Fix |
|---------|-------|-----|
| **Purple/magenta sprites** | --texture-from GLB missing or wrong | Use the exact Tripo3D GLB for that class |
| **Scrambled/garbled textures** | Vertex count mismatch between GLB and rigged mesh | Verify: render GLB alone — if it looks fine, the GLB matches. If not, re-generate. |
| **Feet cut off** | Camera target too high | Use `--camera-target-z 0.7` |
| **Sprites too dark** | Cel-shade shadow too deep | Default is fine (0.45 shadow). Check Blender scene lighting. |
| **Deformed limbs** | Bad automatic weights | Use Blender Weight Paint mode to fix problem areas |
| **T-pose in output** | .blend not properly rigged | Verify: scrub timeline in Blender — mesh should animate |
| **Missing animation folders** | --anim-dir points to wrong directory | Must point to `animations/characters/base_m/` or `base_f/` |

### Atlas Packing

| Problem | Cause | Fix |
|---------|-------|-----|
| **"Folder not found"** | Anim folder name doesn't match config | Check exact folder names in `render_output/{class}/` vs config `"folder"` values |
| **Wrong output path** | Packer output goes to wrong dir | Second arg is the PARENT dir (`Body/`), packer creates subdirectory |
| **No manifest generated** | Config missing `"version": 2` | Ensure config has `"version": 2` |

### In-Game

| Problem | Cause | Fix |
|---------|-------|-----|
| **No sprite shows** | Missing manifest.json or PNGs not imported | Verify files exist in `Body/{class}_{gender}/` and PNGs are imported as .uasset |
| **Blurry sprites** | Wrong texture filter | Set Filter=Nearest on all atlas textures |
| **DXT compression artifacts** | Wrong compression | Set Compression=UserInterface2D |
| **Old sprite still showing** | UE5 cached old .uasset | Delete old .uasset files before reimporting |
| **Wrong animation plays** | Config group mapping wrong | Verify group field matches expected weapon mode |
| **Weapon sprites misaligned** | Class uses per-class rig, weapon uses shared | Both MUST use same armature (shared). Migrate class to shared armature. |

### Mixamo

| Problem | Cause | Fix |
|---------|-------|-----|
| **Upload rejected** | GLB format not accepted | Convert GLB to FBX via Blender first |
| **Bad auto-rig** | Model not in T-pose or has extra geometry | Clean up model, ensure T-pose before upload |
| **Animation doesn't match name** | Mixamo search returned wrong animation | Download, preview in Blender, re-download if wrong |

---

## Directory Structure Reference

```
2D animations/
├── scripts/
│   ├── blender_sprite_render_v2.py     # Core renderer
│   ├── render_blend_all_visible.py     # Dual-pass (weapon overlays)
│   ├── pack_atlas.py                   # Atlas packer
│   ├── tripo_image_to_3d.py           # Tripo3D API
│   └── post_process_sprites.py         # Trim + downscale
│
├── atlas_configs/
│   ├── standard_template_v2.json       # COPY THIS for new classes
│   ├── knight_f_v2.json               # Example: proven config
│   └── {class}_{gender}_v2.json       # One per class
│
├── 3d_models/
│   ├── characters/
│   │   ├── base_f/                     # Shared female armature source
│   │   │   ├── base_f.glb
│   │   │   └── base_f.fbx
│   │   ├── base_m/                     # Shared male armature source (TODO)
│   │   └── {class}_{gender}/           # Per-class model + .blend
│   │       ├── {class}_{gender}_t_pose.glb     # Tripo3D output
│   │       └── {class}_{gender}_shared.blend   # Rigged to shared armature
│   │
│   └── animations/characters/
│       ├── base_f/                     # 17 FBX + T-Pose (shared female)
│       └── base_m/                     # 17 FBX + T-Pose (shared male, TODO)
│
├── sprites/
│   ├── references/hero_refs/           # Input: 2D class art
│   └── render_output/{class}_{gender}/ # Output: per-anim folders of frames
│
client/SabriMMO/Content/SabriMMO/Sprites/Atlases/
└── Body/
    └── {class}_{gender}/               # Final: 17 PNGs + 17 JSONs + manifest
        ├── {class}_{gender}_manifest.json
        ├── {class}_{gender}_idle.json
        ├── {class}_{gender}_idle.png   # (.uasset after import)
        └── ...
```

---

## Quick Command Reference

### Full pipeline for one class (copy-paste, replace {class}_{gender} and base_{gender})

```bash
# 1. Tripo3D
cd "C:/Sabri_MMO/2D animations"
python scripts/tripo_image_to_3d.py \
  "sprites/references/hero_refs/{class}_{gender}.png" \
  "3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb"

# 2. (MANUAL) Blender rigging — see Step 2 above

# 3. Render
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_shared.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_{gender}/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/{class}_{gender}" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/{class}_{gender}_weapon_discard" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/{class}_{gender}/{class}_{gender}_t_pose.glb" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 4. Atlas config
cp "2D animations/atlas_configs/standard_template_v2.json" \
   "2D animations/atlas_configs/{class}_{gender}_v2.json"
# Edit: change "character" to "{class}_{gender}"

# 5. Pack
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/{class}_{gender}" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" \
  --config "2D animations/atlas_configs/{class}_{gender}_v2.json"

# 6. (MANUAL) UE5 import — see Step 6 above
# 7. Test in-game
```
