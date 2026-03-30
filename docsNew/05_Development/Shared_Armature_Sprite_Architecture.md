# Shared Armature Sprite Architecture

Architecture for the shared armature system that eliminates equipment alignment issues across classes and enables the full 9-layer sprite compositing system.

**Status**: Fully proven and working (C++ gender-aware search live, two classes validated: knight_f + merchant_f on base_f armature, gender equipment for dagger + egg shell hat)
**Date**: 2026-03-28
**Prerequisite for**: Hair layer, hair hiding by headgear
**Proven classes**: knight_f, merchant_f (both on base_f shared armature)
**Proven equipment**: Dagger (male + female), Egg Shell Hat (male + female)
**Key fact**: Adding a new class = just body atlas render (~5 min). All existing equipment of that gender works automatically.

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Current Architecture (Per-Class Rigs)](#2-current-architecture-per-class-rigs)
3. [Target Architecture (Shared Armature)](#3-target-architecture-shared-armature)
4. [Implementation Steps](#4-implementation-steps)
5. [Ensuring Consistent Proportions](#5-ensuring-consistent-proportions)
6. [Hair Separation System](#6-hair-separation-system)
7. [Equipment Rendering (One Atlas Per Gender)](#7-equipment-rendering-one-atlas-per-gender)
8. [Layer Compositing Order](#8-layer-compositing-order)
9. [C++ Changes Required](#9-c-changes-required)
10. [Migration Path From Current System](#10-migration-path-from-current-system)
11. [What This Enables](#11-what-this-enables)
12. [Known Tradeoffs](#12-known-tradeoffs)

---

## 1. Problem Statement

Each character class (swordsman_m, archer_f, mage_f) is currently a separate Tripo3D model uploaded to Mixamo independently. Mixamo creates a unique skeleton for each model, adapting bone positions to the mesh proportions. This means:

- `mixamorig:RightHand` is at different world coordinates per class (measured 12-53 pixel offset)
- `mixamorig:Head` is at different world coordinates per class (measured 33-40 pixel offset)
- Equipment (weapons, headgear) rendered on one class doesn't align with other classes
- Every equipment item needs per-class renders to align (N classes × M equipment = N×M atlases)

### Measured Bone Position Differences (Current System)

**RightHand Z (height):**
| Class | Z Position | Offset from swordsman |
|-------|-----------|----------------------|
| swordsman_m | 0.1205 | — |
| archer_f | 0.0892 | ~12 pixels lower |
| mage_f | -0.0119 | ~53 pixels lower |

**Head Z (height):**
| Class | Z Position | Offset from swordsman |
|-------|-----------|----------------------|
| swordsman_m | 0.2255 | — |
| archer_f | 0.1435 | ~33 pixels lower |
| mage_f | 0.1261 | ~40 pixels lower |

Left-right alignment (Y axis) is within ~4 pixels — acceptable. Height differences (Z axis) are clearly visible and unacceptable for shared equipment atlases.

---

## 2. Current Architecture (Per-Class Rigs)

```
swordsman_m → Tripo3D → Mixamo (skeleton A) → render body atlas
archer_f    → Tripo3D → Mixamo (skeleton B) → render body atlas
mage_f      → Tripo3D → Mixamo (skeleton C) → render body atlas

dagger rendered on skeleton A → misaligns with skeleton B and C
egg shell hat rendered on skeleton A → misaligns with skeleton B and C
```

Each class has its own skeleton with different bone positions. Equipment must be rendered per-class to align.

---

## 3. Target Architecture (Shared Armature)

```
male_base   → Tripo3D → Mixamo → ONE male skeleton + 17 animations
female_base → Tripo3D → Mixamo → ONE female skeleton + 17 animations

swordsman_m → Tripo3D → rig to male_base skeleton → render body atlas
knight_m    → Tripo3D → rig to male_base skeleton → render body atlas
archer_f    → Tripo3D → rig to female_base skeleton → render body atlas
mage_f      → Tripo3D → rig to female_base skeleton → render body atlas

dagger rendered on male_base skeleton → aligns with ALL male classes ✓
dagger rendered on female_base skeleton → aligns with ALL female classes ✓
egg shell hat → same — 2 atlases (male + female) serve unlimited classes ✓
```

Two base skeletons (male, female). All classes of the same gender share the same bone positions. Equipment renders twice (once per gender), works for all classes.

---

## 4. Implementation Steps

### Step 1: Generate Base Bodies (Once Per Gender)

Generate a plain male and female character in Tripo3D:
- Clean T-pose reference image
- Simple/minimal clothing (or nude)
- No class-specific accessories, hair optional (will be removed later for bald version)
- Save the reference image — this becomes the template for ALL class variants

Upload to Mixamo:
- Download all 17 standard animations as FBX (With Skin, FPS=30, No Reduction)
- This is the ONE skeleton that all classes of that gender will share

File organization:
```
3d_models/base_bodies/
  male/
    base_m_t_pose.glb                  ← Tripo3D output
    base_m_t_pose.fbx                  ← Blender FBX export for Mixamo
    base_m_bald.glb                    ← bald version (hair removed, for holdout)
    base_m_bald.fbx                    ← bald FBX for holdout renders
  female/
    base_f_t_pose.glb
    base_f_t_pose.fbx
    base_f_bald.glb
    base_f_bald.fbx

3d_models/animations/
  characters/
    base_m/                            ← 17 Mixamo FBX animations (male skeleton)
      Idle.fbx
      Walking.fbx
      ...
    base_f/                            ← 17 Mixamo FBX animations (female skeleton)
      Idle.fbx
      Walking.fbx
      ...
```

**Animation name note**: Female FBX filenames from Mixamo may differ from male (e.g., different animation search results). When creating atlas configs for female equipment, verify the actual FBX filenames in `base_f/` and create gender-specific configs if needed (e.g., `weapon_dagger_f_v2.json`).

### Step 2: Generate Class Models

For each class, generate a Tripo3D model from a reference image that was **edited from the base body reference** (see Section 5 for details on ensuring consistent proportions).

```
base_male.png              → edit to add swordsman armor → base_male_swordsman.png
base_male.png              → edit to add knight armor    → base_male_knight.png
base_female.png            → edit to add archer leather  → base_female_archer.png
base_female.png            → edit to add mage robes      → base_female_mage.png
```

Run each through Tripo3D → GLB output.

### Step 3: Rig Class Models to Shared Armature

**Proven workflow** (validated with knight_f and merchant_f on base_f armature, 2026-03-28):

For each class model, in Blender:

1. **Import base body Mixamo T-pose FBX** (provides the shared armature):
   ```
   File → Import → FBX
   Pick: 3d_models/animations/characters/base_f/Idle.fbx  (any FBX — provides armature)
   ```

2. **Import class model GLB** (provides the outfit appearance):
   ```
   File → Import → glTF 2.0
   Pick: the class GLB from Tripo3D (e.g., knight_f from edited base_f reference)
   ```

3. **(Optional) Import class Mixamo T-pose FBX** for alignment reference:
   ```
   File → Import → FBX
   Pick: the class's own Mixamo T-pose FBX (if you had previously rigged it independently)
   Use this purely for visual alignment — overlap the class mesh to match proportions
   Delete this reference FBX mesh+armature after positioning
   ```

4. **Scale and position the class mesh** to overlap with the base body:
   ```
   Select class mesh → S (scale) → match height
   G (grab) → position feet, hips, shoulders to overlap
   ```

5. **Clean up the class mesh for parenting** (CRITICAL — all 3 sub-steps required):
   ```
   a. If the class mesh has an Armature modifier (from a previous Mixamo rig):
      Select class mesh → Properties → Modifiers → delete the Armature modifier

   b. If the class mesh is parented to another armature:
      Select class mesh → Alt+P → Clear Parent (Keep Transform)

   c. Apply all transforms:
      Select class mesh → Ctrl+A → All Transforms
   ```
   These steps ensure the mesh is a clean, unparented, zero-transform object ready for Automatic Weights.

6. **Delete the base body mesh** (keep the armature):
   ```
   Select base body mesh → X → Delete
   The shared armature remains in the scene
   ```

7. **Parent class mesh to the shared armature**:
   ```
   Select class mesh → Shift+Select armature → Ctrl+P → Armature Deform → With Automatic Weights
   ```

8. **Verify**: Enter Pose Mode, move bones — class mesh should deform correctly.

9. **Save** as the class .blend:
   ```
   Save as: 3d_models/characters/{class}_{gender}_shared.blend
   ```

### Step 4: Render Class Body Atlases

**STANDARD RENDER COMMAND (2026-03-29)**: ALL body renders use `render_blend_all_visible.py` with the shared armature `.blend` loaded as the Blender scene. `blender_sprite_render_v2.py` is OBSOLETE for body renders.

```bash
# Example: knight_f body on shared base_f armature
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/characters/knight_f/knightFBXTPOSE_on_baseFBXTPOSE.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/knight_f" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/knight_f_equip_ignore" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/knight_f/knight_f.glb" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

**Critical flags**:
- `.blend` is the FIRST arg to `blender.exe` (loaded as scene), NOT the script's positional arg
- `--texture-from` is ALWAYS required — .blend has Mixamo-stripped textures
- `--cel-shadow 0.92 --cel-mid 0.98` — standard lighting (nearly flat, all detail visible)
- `--anim-dir` points to `base_f/` or `base_m/` (shared armature anims)
- `--weapon-output` to a throwaway dir (no equipment in body render)

### Step 5: Render Equipment (Once Per Gender)

Equipment .blend files use the bald base body as the holdout occluder:

```bash
# Dagger for female classes (first proven — base_f armature)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/dagger/dagger_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_dagger_female" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# Dagger for male classes (base_m armature)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/dagger/dagger_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_dagger_male" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7
```

Equipment atlas organization (gender subfolders):
```
Sprites/Atlases/
  Body/
    swordsman_m/    ← male class body atlases (legacy per-class rig)
    knight_f/       ← female class body atlases (shared armature base_f)
    merchant_f/     ← female class body atlases (shared armature base_f)
  Weapon/
    dagger/
      male/         ← weapon_1_manifest.json + 17 atlases (rendered on base_m armature)
      female/       ← weapon_1_manifest.json + 17 atlases (rendered on base_f armature)
  HeadgearTop/
    egg_shell_hat/
      male/         ← headgeartop_101_manifest.json + 17 atlases
      female/       ← headgeartop_101_manifest.json + 17 atlases
  Hair/
    style_01/
      male/         ← hair_1_manifest.json + atlases (future)
      female/       ← (future)
```

---

## 5. Ensuring Consistent Proportions

### The Key Rule

**All class reference images for Tripo3D must start from the same base body silhouette.**

Tripo3D generates 3D models that closely follow the input image's proportions. Same body silhouette in → same body proportions out. Class differences should only be in clothing/surface details, not body shape.

### Workflow

1. **Create base reference images** (one male, one female):
   - Clean T-pose, front-facing
   - Simple neutral clothing
   - Consistent proportions (anime ~1:5.5 head-to-body ratio)
   - Save as `base_male.png`, `base_female.png`

2. **For each class**, edit the base image to add outfit details:
   - Use Photoshop, GIMP, or AI inpainting
   - Paint armor/robes/leather ONTO the same body silhouette
   - Keep the pose, proportions, and body outline identical
   - Only change the surface appearance (clothing, accessories)
   - Save as `base_male_swordsman.png`, `base_female_archer.png`, etc.

3. **Run each edited image through Tripo3D** → GLB
   - The output models will have very similar proportions (same silhouette)
   - Only the clothing surface differs

### What To Avoid

- Do NOT generate class models from completely different reference images
- Do NOT use refs from different artists with different body proportions
- Do NOT mix bulky armored knight refs with thin elf mage refs
- Do NOT change the pose or body outline between class variants
- Keep the same camera angle, same framing, same body proportions across all class reference images

---

## 6. Hair Separation System

With the shared armature, hair becomes a proper layer:

### Creating Bald Base Bodies

1. Open the base body GLB in Blender
2. Edit Mode → select hair geometry → delete faces
3. Fill the hole with Grid Fill → smooth
4. Fix UVs (Project from View, map to skin color area)
5. Export as `male_base_bald.glb` and `male_base_bald.fbx`

The bald body is used for:
- **Holdout occluder** in all equipment .blend files (weapons, headgear, shield, garment)
- **Body layer atlas** (the nude/base body visible at edges where outfit doesn't cover — optional)

### Creating Hair Meshes

Option A — Extract from existing Tripo3D models:
1. Open a Tripo3D model with the desired hair style
2. Edit Mode → select hair faces
3. `P` → Separate by Selection
4. Parent the hair mesh to `mixamorig:Head` bone
5. One operation gives you both the bald body AND the hair mesh

Option B — Generate hair-only from Tripo3D reference images (results vary)

Option C — Use free/paid 3D hair model packs (may need art style adjustment)

### Hair Rendering

Same pipeline as headgear:
```bash
# Hair style 1 for male
render_blend_all_visible.py \
  hair_01_on_male_base.blend \      ← bald base body + hair mesh on Head bone
  --weapon-only \                   ← holdout on bald body
  --anim-dir base_male/ \
  --render-size 1024
```

### Hair Color

- Render hair in neutral white/grey
- Tint at runtime via `FSpriteLayerState::TintColor`
- 9 RO colors: Black, Brown, Blonde, Red, Blue, Purple, Green, Silver, Pink

### Hair Hiding by Headgear

- `"hides_hair": true` in headgear manifest JSON
- C++: ~15 lines in `LoadEquipmentLayer()` — `SetLayerVisible(ESpriteLayer::Hair, false)`
- Restores hair on unequip or swap to non-hiding headgear

---

## 7. Equipment Rendering (One Atlas Per Gender)

With the shared armature, equipment scales cleanly:

| Equipment Type | Atlases Needed | Shared Across |
|----------------|---------------|---------------|
| Dagger | 2 (male + female) | All male classes / all female classes |
| Bow | 2 | All classes per gender |
| Egg Shell Hat | 2 | All classes per gender |
| Hair Style 1 | 2 | All classes per gender |
| Each new weapon | 2 | All classes per gender |
| Each new headgear | 2 | All classes per gender |

**Scaling**: 10 weapons × 2 genders = 20 weapon atlases total (not 10 × N classes).

### Equipment .blend Files

Each equipment .blend contains:
- The **bald base body** (as holdout occluder — not rendered, just occludes)
- The **equipment mesh** (weapon/headgear/hair parented to the correct bone)
- Both on the **shared armature**

```
3d_models/weapon_templates/
  dagger/
    dagger_on_male_base.blend       ← bald male body + dagger on RightHand
    dagger_on_female_base.blend     ← bald female body + dagger on RightHand
  bow/
    bow_on_male_base.blend
    bow_on_female_base.blend

3d_models/Headgear/Top/
  egg_shell_hat_on_male_base.blend
  egg_shell_hat_on_female_base.blend

3d_models/hair_templates/
  hair_01_on_male_base.blend
  hair_01_on_female_base.blend
```

### Holdout Occlusion

All equipment renders use the holdout system:
- Body mesh uses Blender Holdout shader during equipment pass (Pass 2)
- Body is transparent but occludes equipment behind it
- Only visible equipment pixels render
- Runtime always composites equipment at +5 (in front)
- No depth arrays needed — occlusion baked from render

The bald body as holdout also prevents hair clipping through helmets.

---

## 8. Layer Compositing Order

```
Layer 0: Shadow        ← reserved (not rendered)
Layer 1: Body          ← class outfit (swordsman armor, mage robe, etc.)
Layer 2: Hair          ← separate hair mesh on Head bone (hideable by headgear)
Layer 3: HeadgearLow   ← mouth items (pipe, mask) on Head bone
Layer 4: HeadgearMid   ← eye items (glasses, goggles) on Head bone
Layer 5: HeadgearTop   ← hats, helms, crowns on Head bone
Layer 6: Garment       ← capes, wings, back items
Layer 7: Weapon        ← weapon on Hand bone
Layer 8: Shield        ← shield on Hand bone

All equipment layers composite at +5 (always in front of body).
Holdout occlusion baked from render handles front/behind naturally.
```

---

## 9. C++ Changes (Implemented)

### GenderSubDir Member Variable

`ASpriteCharacterActor` stores `GenderSubDir` as an `FString` member ("male" or "female"), set during `SetBodyClass(int32 JobClass, int32 Gender)`:

```cpp
// In SetBodyClass:
GenderSubDir = (Gender == 1) ? TEXT("female") : TEXT("male");
```

### LoadEquipmentLayer — Gender-Aware Search (IMPLEMENTED)

Three-tier search priority:
```
1. {LayerRoot}/{item_subdir}/{GenderSubDir}/{manifest}
   e.g., Weapon/dagger/female/weapon_1_manifest.json

2. {LayerRoot}/{item_subdir}/{manifest}
   e.g., Weapon/dagger/weapon_1_manifest.json  (gender-neutral fallback)

3. {LayerRoot}/{manifest}
   e.g., Weapon/weapon_1_manifest.json  (flat fallback)
```

This allows gradual migration: existing gender-neutral equipment atlases still work, while new gender-specific atlases take priority when present.

### SetHairStyle — New Function

```cpp
void ASpriteCharacterActor::SetHairStyle(int32 StyleId, int32 ColorId)
{
    LoadEquipmentLayer(ESpriteLayer::Hair, StyleId);

    FSpriteLayerState& HairLayer = Layers[static_cast<int32>(ESpriteLayer::Hair)];
    if (HairLayer.MaterialInst)
    {
        HairLayer.TintColor = GetHairColorFromPalette(ColorId);
        HairLayer.MaterialInst->SetVectorParameterValue(TEXT("Tint"), HairLayer.TintColor);
    }
}
```

### Hair Hiding — hides_hair Flag

~15 lines in `LoadEquipmentLayer()` — check `"hides_hair"` in headgear manifest, toggle Hair layer visibility. Already designed (see HeadgearTop Implementation Plan Section 11).

---

## 10. Migration Path From Current System

### Phase 1: Shared Armature Foundation
- Generate male_base and female_base in Tripo3D
- Upload to Mixamo → download 17 animations each
- Create bald variants in Blender
- Re-rig existing class models (swordsman_m, archer_f, mage_f) to shared armatures
- Re-render all body atlases

### Phase 2: Re-render Equipment
- Create equipment .blend files using bald base bodies
- Re-render dagger, bow, egg shell hat (once per gender)
- Update C++ `LoadEquipmentLayer` for gender-aware search
- Delete per-class equipment atlases (replaced by per-gender)

### Phase 3: Hair Layer
- Remove hair from class models (bald body atlas)
- Extract hair meshes from existing Tripo3D models
- Render hair atlases (once per gender per style)
- Implement `SetHairStyle()` and `hides_hair` in C++
- Connect to character creation (hair style + color selection)

---

## 11. What This Enables

| Feature | Current System | Shared Armature |
|---------|---------------|-----------------|
| Equipment alignment across classes | Broken (12-53px offset) | Perfect (same bones) |
| Equipment atlases per weapon | 1 per class per weapon | 1 per gender per weapon |
| Hair customization | Baked into body, can't change | Separate layer, style + color selectable |
| Hair hiding by headgear | Not possible | `hides_hair` manifest flag |
| Adding a new class | New Mixamo rig + re-render all equipment | Just a new body atlas |
| Adding a new weapon | Render per class | Render once per gender |
| Headgear clipping through hair | Hair baked in, clips through | Bald holdout body, no clipping |

---

## 12. Known Tradeoffs

### Mesh Deformation

Class models rigged to the shared armature via Automatic Weights may have slight deformation differences from the original Mixamo rig. Arms might stretch slightly longer/shorter, torso might compress slightly. At 1024px cel-shaded sprites, this is invisible.

### Proportion Consistency

Requires discipline in Tripo3D reference image creation — all class refs must start from the same base body silhouette. Different proportions → visible misalignment despite shared armature.

### Two Genders = Two Equipment Sets

Every equipment item renders twice (male + female). This is the minimum — you can't avoid it because male and female bodies have genuinely different proportions. But it's 2×N instead of Classes×N.

### Hair Model Availability

Need 3D hair meshes per style. Extracting from existing Tripo3D models provides 3-5 styles. More styles require additional modeling or generation.

---

## Quick Reference: Adding Content (Copy-Paste Commands)

### Add a New Class (e.g., priest_f on base_f)

```bash
# 1. Render body (shared armature .blend loaded as Blender scene)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/characters/priest_f/priestFBXTPOSE_on_baseFBXTPOSE.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/priest_f" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/priest_f_equip_ignore" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/priest_f/priest_f.glb" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 2. Pack atlas (copy standard_template_v2.json, set "character": "priest_f")
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/priest_f" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" \
  --config "2D animations/atlas_configs/priest_f_v2.json"

# 3. Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps)
# 4. Done — all existing female equipment works automatically
```

### Add a New Weapon Type (e.g., sword, view_sprite=2)

```bash
# 1. Male render
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/sword/sword_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_sword_male" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# 2. Female render
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/sword/sword_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_sword_female" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# 3. Pack male (config: "character": "weapon_2", "depth_mode": "always_front")
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_sword_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/sword/male" \
  --config "2D animations/atlas_configs/weapon_sword_v2.json"

# 4. Pack female (use gender-specific config if FBX names differ)
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_sword_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/sword/female" \
  --config "2D animations/atlas_configs/weapon_sword_v2.json"

# 5. DB: UPDATE items SET view_sprite = 2 WHERE weapon_type = 'sword'
# 6. Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps)
# 7. Done — works for all classes of each gender
```

### Add a New Headgear (e.g., helm, view_sprite from DB)

```bash
# 1. Male render
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/helm_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_helm_male" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# 2. Female render
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/helm_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_helm_female" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7

# 3. Pack male (config: "character": "headgeartop_N", "depth_mode": "always_front")
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_helm_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/helm/male" \
  --config "2D animations/atlas_configs/headgeartop_helm_v2.json"

# 4. Pack female
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_helm_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/helm/female" \
  --config "2D animations/atlas_configs/headgeartop_helm_v2.json"

# 5. Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps)
# 6. Done — works for all classes of each gender
```

### Key Technical Facts

| Fact | Value |
|------|-------|
| base_f ortho_scale | 2.55 |
| swordsman_m ortho_scale | 2.56 |
| Female animation dir | `animations/characters/base_f/` |
| Male animation dir | `animations/characters/base_m/` |
| Female bow attack FBX | "Standing Draw Arrow" |
| Gender-specific atlas config needed when | FBX filenames differ between base_m and base_f |
| C++ GenderSubDir | Gender==1 -> "female", else -> "male" |
| Equipment depth_mode | "always_front" for all equipment |
| Depth offset | +5.0 units (holdout occlusion handles visibility) |
