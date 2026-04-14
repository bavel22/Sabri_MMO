# Equipment Sprite Pipeline Reference

Complete step-by-step guide for adding new weapon types and headgear sprites for both male and female characters using the shared armature system.

**Last updated**: 2026-04-09
**Proven weapons**: Dagger (view_sprite=1, male+female), Bow (view_sprite=11, male+female)
**Proven headgear**: Egg Shell Hat (view_sprite=101, male+female)
**Output per equipment per gender**: 17 atlas PNGs + 17 JSONs + 1 manifest = 35 files

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Weapon Sprite Pipeline (10 Steps)](#weapon-sprite-pipeline)
3. [Headgear Sprite Pipeline (10 Steps)](#headgear-sprite-pipeline)
4. [Blender Setup — Detailed Steps](#blender-setup--detailed-steps)
5. [Atlas Config Reference](#atlas-config-reference)
6. [Command Reference](#command-reference)
7. [File Naming Conventions](#file-naming-conventions)
8. [Directory Structure](#directory-structure)
9. [Database Setup](#database-setup)
10. [Multiplayer Sync](#multiplayer-sync)
11. [Troubleshooting](#troubleshooting)

---

## Architecture Overview

### How Equipment Sprites Work

Equipment sprites are rendered as **separate sprite layers** composited on top of the character body sprite. Each equipment layer is its own `UProceduralMeshComponent` with its own atlas texture, updated every frame to match the body's current animation state, weapon mode, direction, and frame.

### Key Concepts

| Concept | Description |
|---------|-------------|
| **Dual-pass rendering** | Body and equipment rendered from the SAME .blend scene. Pass 1 = body only. Pass 2 = equipment with body as holdout occluder. |
| **Holdout occlusion** | During Pass 2, body meshes use a Blender Holdout shader — transparent but blocks equipment pixels behind body. Only visible equipment pixels render. |
| **Shared armature** | Equipment .blend files use `base_m` or `base_f` armature. One render per gender serves ALL classes of that gender. |
| **Gender subfolders** | Equipment atlases live in `{Layer}/{item_name}/male/` and `female/`. Client searches gender subfolder first. |
| **view_sprite ID** | Integer in the `items` DB table that maps an item to its sprite atlas. Atlas files are named `{layer}_{view_sprite}_manifest.json`. |
| **Bone parenting** | Equipment mesh is parented to a specific armature bone (e.g., `mixamorig:RightHand` for weapons, `mixamorig:Head` for headgear). |

### Render Script

All equipment rendering uses `render_blend_all_visible.py` (NOT `blender_sprite_render_v2.py` which is body-only).

### What You Need Before Starting

- `base_m` shared armature ready (17 FBX animations in `animations/characters/base_m/`)
- `base_f` shared armature ready (17 FBX animations in `animations/characters/base_f/`)
- A 3D model of the equipment (GLB from Tripo3D, Blender manual modeling, or asset store)

---

## Weapon Sprite Pipeline

### Step 1: Get or Create the Weapon 3D Model

Option A — **Tripo3D** (for simple weapons):
```bash
cd "C:/Sabri_MMO/2D animations"
python scripts/tripo_image_to_3d.py \
  "path/to/weapon_reference.png" \
  "3d_models/weapon_templates/{weapon_type}/{weapon_type}.glb"
```

Option B — **Manual Blender modeling** or **asset store GLB/FBX**.

Save the weapon model to:
```
2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_type}.glb
```

Examples:
```
weapon_templates/sword/sword.glb
weapon_templates/axe/axe.glb
weapon_templates/spear/spear.glb
weapon_templates/mace/mace.glb
weapon_templates/staff/staff.glb
weapon_templates/katar/katar.glb
weapon_templates/knuckle/knuckle.glb
```

### Step 2: Create Male .blend Template

Open Blender and follow these steps precisely:

#### 2a. Import the base male armature
- File > Import > FBX
- Navigate to: `3d_models/animations/characters/base_m/Idle.fbx`
  - (Any FBX from `base_m/` works — they all contain the shared armature)
- This imports a rigged body mesh + armature

#### 2b. Import the weapon model
- File > Import > glTF 2.0 (for GLB) or FBX
- Navigate to your weapon model: `3d_models/weapon_templates/{weapon_type}/{weapon_type}.glb`

#### 2c. Position the weapon in the character's hand
- Select the weapon mesh
- In Edit Mode or Object Mode, **translate, rotate, and scale** the weapon so it sits correctly in the character's right hand (or left hand for bows/shields)
- The weapon grip should be at/near the hand bone position

**Hand bones:**
| Weapon Type | Target Bone | Notes |
|------------|-------------|-------|
| Dagger, Sword, Axe, Mace, Rod/Staff | `mixamorig:RightHand` | Standard right-hand weapons |
| Spear, 2H Sword | `mixamorig:RightHand` | Two-handed but held from right hand bone |
| Katar, Knuckle | `mixamorig:RightHand` | Fist weapons |
| Bow | `mixamorig:LeftHand` | Held in left hand |
| Shield | `mixamorig:LeftHand` | Off-hand |

#### 2d. Parent weapon to the hand bone
1. Select the **weapon mesh** (click)
2. Then Shift+click the **armature** (so armature has the orange outline)
3. Press **Ctrl+P**
4. Choose **Bone**
5. In the popup that appears, select the correct bone:
   - For right-hand weapons: `mixamorig:RightHand`
   - For bows/shields: `mixamorig:LeftHand`

#### 2e. Verify the parenting
- In the Properties panel (right side), check the weapon mesh:
  - **Parent**: Should show the Armature name
  - **Parent Type**: `BONE`
  - **Parent Bone**: `mixamorig:RightHand` (or LeftHand)
- Enter **Pose Mode** on the armature
- Rotate the hand bone — the weapon should follow perfectly
- Play the Idle animation — weapon should move naturally with the hand

#### 2f. Fine-tune weapon position
- While in Pose Mode, you can see how the weapon looks during animations
- If the grip is off, go back to Object Mode, select the weapon, and adjust
- **Important**: The weapon's position relative to the hand bone is baked into the .blend — get it right here, it applies to all 17 animations

#### 2g. Save the .blend file
- File > Save As
- Save to: `3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_m.blend`

Example:
```
weapon_templates/sword/sword_on_base_m.blend
weapon_templates/axe/axe_on_base_m.blend
weapon_templates/mace/mace_on_base_m.blend
```

### Step 3: Create Female .blend Template

Repeat Step 2 exactly, but use `base_f`:

#### 3a. Start a NEW Blender file (File > New > General)

#### 3b. Import base female armature
- File > Import > FBX
- Navigate to: `3d_models/animations/characters/base_f/Idle.fbx`

#### 3c-3f. Same as steps 2c-2f
- Import weapon GLB, position in hand, parent to bone, verify

#### 3g. Save as female .blend
- Save to: `3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_f.blend`

Example:
```
weapon_templates/sword/sword_on_base_f.blend
weapon_templates/axe/axe_on_base_f.blend
```

**Why two .blend files?** Male and female base bodies have different proportions and hand positions. The weapon needs to be positioned relative to each gender's hand bone separately.

### Step 4: Test Frame Render

Before doing a full render (~10 min per gender), do a quick test to verify alignment:

**Male test:**
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_weapon" \
  --test-frame --render-size 512
```

**Female test:**
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_weapon" \
  --test-frame --render-size 512
```

**What `--test-frame` does:**
- Renders only 1 direction (South), 1 animation (first), 1 frame
- Fast (< 30 seconds) — lets you check weapon position and holdout occlusion
- Output: 2 files in `test_body/` and `test_weapon/`

**Check the output:**
- Open the body PNG — character should look normal, weapon hidden
- Open the weapon PNG — ONLY the weapon visible, body area transparent, parts behind body occluded by holdout

If weapon is mispositioned, go back to Blender and adjust in the .blend file, then re-test.

### Step 5: Full Render (Male)

```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_{weapon_type}_male" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

**Key flags:**
| Flag | Value | Why |
|------|-------|-----|
| `--weapon-only` | (present) | Skips body output (we only need the weapon sprites) |
| `--render-size` | 1024 | Standard cell size |
| `--camera-angle` | 10 | Must match body renders |
| `--camera-target-z` | 0.7 | Must match body renders |
| `--body-output` | (required but unused with --weapon-only) | Still required by argparser |
| `--weapon-output` | `weapon_{type}_male` | Where weapon frames go |

**Output:** 17 animation folders in `sprites/render_output/weapon_{weapon_type}_male/`:
```
weapon_sword_male/
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
  depth_map.json          (legacy, ignored with always_front)
```

### Step 6: Full Render (Female)

```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_type}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_{weapon_type}_female" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

### Step 7: Create Atlas Configs

You need to assign a **view_sprite ID** for this weapon type. Check existing IDs:

| Weapon Type | view_sprite | Status |
|------------|-------------|--------|
| Dagger | 1 | Implemented |
| Bow | 11 | Implemented |

Choose a unique ID that doesn't collide. Common RO view_sprite assignments:

| Weapon Type | Suggested view_sprite |
|------------|----------------------|
| Sword (1H) | 2 |
| 2H Sword | 3 |
| Axe (1H) | 6 |
| 2H Axe | 7 |
| Mace | 8 |
| Rod/Staff | 10 |
| Spear (1H) | 4 |
| 2H Spear | 5 |
| Katar | 16 |
| Knuckle | 12 |
| Book | 13 |
| Whip | 14 |
| Instrument | 15 |

#### Male config

Create: `2D animations/atlas_configs/weapon_{weapon_type}_v2.json`

```json
{
  "version": 2,
  "character": "weapon_{VIEW_SPRITE_ID}",
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

**CRITICAL**: The `"character"` field MUST be `"weapon_{VIEW_SPRITE_ID}"` (e.g., `"weapon_2"` for sword with view_sprite=2). This is what the atlas files are named after.

#### Female config

Create: `2D animations/atlas_configs/weapon_{weapon_type}_f_v2.json`

Same content as male config. Only create a separate female config if the animation FBX filenames differ between `base_m` and `base_f`. If they're identical (which they should be if you downloaded the same Mixamo animations for both), you can reuse the same config for both genders.

### Step 8: Pack Atlases

**Male:**
```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_{weapon_type}_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/male" \
  --config "2D animations/atlas_configs/weapon_{weapon_type}_v2.json"
```

**Female:**
```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_{weapon_type}_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/female" \
  --config "2D animations/atlas_configs/weapon_{weapon_type}_f_v2.json"
```

**Output per gender** (35 files):
```
Weapon/{weapon_type}/male/
  weapon_{ID}_manifest.json
  weapon_{ID}_idle.json + .png
  weapon_{ID}_2hand_idle.json + .png
  weapon_{ID}_walking.json + .png
  weapon_{ID}_run_with_sword.json + .png
  weapon_{ID}_punching.json + .png
  weapon_{ID}_stable_sword_inward_slash.json + .png
  weapon_{ID}_great_sword_slash.json + .png
  weapon_{ID}_standing_draw_arrow.json + .png
  weapon_{ID}_standing_1h_magic_attack_01.json + .png
  weapon_{ID}_standing_1h_cast_spell_01.json + .png
  weapon_{ID}_standing_2h_magic_area_attack_01.json + .png
  weapon_{ID}_standing_2h_magic_area_attack_02.json + .png
  weapon_{ID}_reaction.json + .png
  weapon_{ID}_dying.json + .png
  weapon_{ID}_sitting_idle.json + .png
  weapon_{ID}_picking_up.json + .png
  weapon_{ID}_sword_and_shield_block.json + .png
```

### Step 9: UE5 Import

1. Open Unreal Editor
2. Navigate to `Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/male/`
3. Import all 17 PNG files
4. Set texture properties on each:
   | Property | Value |
   |----------|-------|
   | **Compression** | `UserInterface2D` |
   | **Filter** | `Nearest` |
   | **Mip Gen Settings** | `NoMipmaps` |
   | **Never Stream** | `On` |
5. Repeat for `Weapon/{weapon_type}/female/`
6. Save all
7. **If re-importing**: Delete old `.uasset` files first — UE5 caches aggressively

### Step 10: Database Update

Update the `items` table so all items of this weapon type have the correct `view_sprite`:

```sql
-- Example: assign view_sprite=2 to all one-handed swords
UPDATE items SET view_sprite = 2 WHERE weapon_type = 'one_hand_sword';

-- Or for specific items:
UPDATE items SET view_sprite = 2 WHERE item_id IN (1101, 1102, 1103, 1104);
```

**No C++ code changes needed.** The client's `LoadEquipmentLayer()` scans all subdirectories under `Weapon/` looking for `weapon_{view_sprite}_manifest.json`. When a player equips an item, the server sends `view_sprite` to the client, which loads the matching atlas.

### Step 10b: Test

1. Start server + client
2. Equip a weapon of the new type
3. Verify:
   - Weapon sprite appears on character
   - Correct for all 8 directions
   - Correct during idle, walk, attack, cast, sit, death animations
   - Weapon holdout (parts behind body are occluded)
   - Remote players see the weapon too

---

## Headgear Sprite Pipeline

Headgear uses the **exact same dual-pass render pipeline** as weapons. The only differences are:
- **Bone**: `mixamorig:Head` instead of hand bones
- **Layer**: `HeadgearTop` / `HeadgearMid` / `HeadgearLow` instead of `Weapon`
- **Atlas prefix**: `headgeartop_{ID}` instead of `weapon_{ID}`
- **View sprite IDs**: Use rAthena canonical values (101+) to avoid collision with weapon IDs

### Step 1: Get or Create the Headgear 3D Model

Same options as weapons: Tripo3D, manual Blender modeling, or asset store.

Save to:
```
2D animations/3d_models/Headgear/Top/{headgear_name}/{headgear_name}.glb
```

For Mid/Low headgear:
```
2D animations/3d_models/Headgear/Mid/{headgear_name}/{headgear_name}.glb
2D animations/3d_models/Headgear/Low/{headgear_name}/{headgear_name}.glb
```

Examples:
```
Headgear/Top/iron_helm/iron_helm.glb
Headgear/Top/wizard_hat/wizard_hat.glb
Headgear/Mid/sunglasses/sunglasses.glb
Headgear/Low/pipe/pipe.glb
```

### Step 2: Create Male .blend Template

#### 2a. Open Blender, start new file

#### 2b. Import base male armature
- File > Import > FBX
- Navigate to: `3d_models/animations/characters/base_m/Idle.fbx`

#### 2c. Import headgear model
- File > Import > glTF 2.0
- Navigate to: `3d_models/Headgear/Top/{headgear_name}/{headgear_name}.glb`

#### 2d. Position the headgear on the character's head
- Select the headgear mesh
- In Object Mode, **translate, rotate, and scale** the headgear so it sits correctly on the head
- Top headgear: sits on top of head (hats, helms, crowns)
- Mid headgear: at eye level (glasses, goggles, visors)
- Low headgear: at mouth level (pipes, masks, scarves)

**Tips for positioning:**
- Switch to front view (Numpad 1) and side view (Numpad 3) to align precisely
- The headgear should look correct from all angles
- Enter Pose Mode on the armature and rotate the Head bone to check how the headgear follows

#### 2e. Parent headgear to the Head bone
1. Select the **headgear mesh** (click)
2. Then Shift+click the **armature** (orange outline)
3. Press **Ctrl+P**
4. Choose **Bone**
5. Select: `mixamorig:Head`

#### 2f. Verify the parenting
- Properties panel should show:
  - **Parent**: Armature name
  - **Parent Type**: `BONE`
  - **Parent Bone**: `mixamorig:Head`
- Enter Pose Mode → rotate Head bone → headgear follows
- Play Walking animation → headgear bobs naturally with head movement
- Play Sitting animation → headgear stays on head even in seated position

#### 2g. Save the .blend file
```
3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_m.blend
```

Example:
```
Headgear/Top/iron_helm/iron_helm_on_base_m.blend
Headgear/Top/wizard_hat/wizard_hat_on_base_m.blend
Headgear/Mid/sunglasses/sunglasses_on_base_m.blend
```

### Step 3: Create Female .blend Template

Repeat Step 2 with `base_f`:

#### 3a. Start a NEW Blender file

#### 3b. Import base female armature
- File > Import > FBX → `3d_models/animations/characters/base_f/Idle.fbx`

#### 3c-3f. Same as 2c-2f but on female body
- Import headgear, position on head, parent to `mixamorig:Head`, verify

#### 3g. Save as female .blend
```
3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_f.blend
```

**Why two .blend files?** Male and female bodies have different head sizes/positions. The headgear needs separate positioning for each gender.

### Step 4: Test Frame Render

**Male:**
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_headgear" \
  --test-frame --render-size 512
```

**Female:**
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_headgear" \
  --test-frame --render-size 512
```

Check the test_headgear output — headgear should be visible, body area transparent, parts behind head occluded.

### Step 5: Full Render (Male)

```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_m_body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{headgear_name}_male" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

### Step 6: Full Render (Female)

```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{headgear_name}/{headgear_name}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/base_f_body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{headgear_name}_female" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

### Step 7: Determine view_sprite ID

Check the item's `view_sprite` in the database:
```sql
SELECT item_id, name_english, view_sprite FROM items WHERE item_id = {ITEM_ID};
```

For RO Classic headgear, use the rAthena canonical `view_sprite` values. Examples:

| Headgear | Item ID | view_sprite |
|----------|---------|-------------|
| Egg Shell | 5015 | 101 |
| Hat | 2209 | TBD |
| Helm | 2202 | TBD |
| Cap | 2201 | TBD |
| Ribbon | 2204 | TBD |
| Glasses | 2208 | TBD (head_mid) |

If `view_sprite` is 0 or NULL, assign one. Use the full mapping at `docsNew/items/headgear_sprites.md` or run:
```sql
SELECT DISTINCT view_sprite, name_english FROM items 
WHERE equip_slot LIKE '%head%' AND view_sprite > 0 
ORDER BY view_sprite;
```

### Step 8: Create Atlas Configs

#### Male config

Create: `2D animations/atlas_configs/headgeartop_{headgear_name}_v2.json`

```json
{
  "version": 2,
  "character": "headgeartop_{VIEW_SPRITE_ID}",
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

**CRITICAL**: `"character"` MUST be `"headgeartop_{VIEW_SPRITE_ID}"` (e.g., `"headgeartop_101"`).

For HeadgearMid: use `"headgearmid_{ID}"`. For HeadgearLow: use `"headgearlow_{ID}"`.

#### Female config

Create: `2D animations/atlas_configs/headgeartop_{headgear_name}_f_v2.json`

Same content. Only create separate if FBX animation names differ between genders.

### Step 9: Pack Atlases

**Male:**
```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{headgear_name}_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{headgear_name}/male" \
  --config "2D animations/atlas_configs/headgeartop_{headgear_name}_v2.json"
```

**Female:**
```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{headgear_name}_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{headgear_name}/female" \
  --config "2D animations/atlas_configs/headgeartop_{headgear_name}_f_v2.json"
```

**Output per gender** (35 files):
```
HeadgearTop/{headgear_name}/male/
  headgeartop_{ID}_manifest.json
  headgeartop_{ID}_idle.json + .png
  headgeartop_{ID}_2hand_idle.json + .png
  ... (17 animation pairs)
```

### Step 10: UE5 Import + Database Update

#### Import
1. Import all PNGs for both genders into their respective folders
2. Set texture properties: UserInterface2D, Nearest, NoMipmaps, Never Stream
3. Save all

#### Database
```sql
-- Assign view_sprite to the headgear item(s)
UPDATE items SET view_sprite = 101 WHERE item_id = 5015;  -- Egg Shell example
```

#### Hair hiding (optional)
If the headgear is a full helm/hood that should hide the player's hair:
- Manually edit the generated manifest JSON
- Add `"hides_hair": true` at the top level:
```json
{
  "version": 2,
  "character": "headgeartop_101",
  "cell_size": 1024,
  "hides_hair": true,
  "atlases": [ ... ]
}
```

**No C++ code changes needed.** `LoadEquipmentLayer(HeadgearTop, viewSpriteId)` is already wired.

---

## Blender Setup — Detailed Steps

### Understanding the .blend File Structure

A properly set up equipment .blend file has this hierarchy in Blender's Outliner:

```
Scene Collection
  └── Armature              (from base_m or base_f FBX import)
       ├── Body mesh         (parent=Armature, parent_type=OBJECT)
       └── Equipment mesh    (parent=Armature, parent_type=BONE, parent_bone=mixamorig:RightHand)
```

The render script (`render_blend_all_visible.py`) automatically detects which meshes are equipment vs body by checking `parent_type`:
- `parent_type = 'OBJECT'` → **body mesh** (hidden during equipment pass, holdout applied)
- `parent_type = 'BONE'` → **equipment mesh** (visible during equipment pass)

### How to Verify Correct Setup

1. Select the equipment mesh
2. Open Properties panel > Object Properties > Relations:
   - **Parent**: Should show the Armature name (e.g., "Armature" or "mixamorig:Hips")
   - **Parent Type**: Must be `BONE`
   - **Parent Bone**: Must be the correct bone name (e.g., `mixamorig:RightHand`)

If Parent Type is `OBJECT` instead of `BONE`, the render script will treat it as body and skip it during the equipment pass.

### Common Bone Names (Mixamo Armature)

| Bone | Used For |
|------|----------|
| `mixamorig:RightHand` | Daggers, swords, axes, maces, rods, spears, katars, knuckles |
| `mixamorig:LeftHand` | Bows, shields |
| `mixamorig:Head` | All headgear (top, mid, low) |
| `mixamorig:Spine2` | Garments/capes (back attachment point) |

### Positioning Tips

**Weapons:**
- The weapon's grip point should be at the bone's origin
- For swords: blade points up in T-pose, grip at hand bone
- For bows: bow string faces right in T-pose (archer's left hand holds the bow)
- Scale the weapon to match body proportions — too large looks cartoonish, too small is invisible

**Headgear:**
- Position headgear ABOVE the head mesh, not clipping into it
- Account for the head bone's rotation during animations (especially Sitting, Dying)
- Test with Reaction (hit) animation — head tilts back, headgear should follow naturally

### Texture Recovery for Equipment

If the equipment GLB has textures that don't import correctly:
- The render script's `--texture-from` flag is for BODY textures only (from Tripo3D)
- Equipment textures should be embedded in the GLB directly
- If textures are missing: in Blender, manually assign materials to the equipment mesh
- Ensure equipment materials use the `Principled BSDF` node with the correct texture connected to Base Color

---

## Atlas Config Reference

### Mandatory Fields

| Field | Type | Description |
|-------|------|-------------|
| `version` | `2` | Must be 2 for v2 atlas system |
| `character` | string | `weapon_{ID}` or `headgeartop_{ID}` — matches manifest filename |
| `cell_size` | `1024` | Pixel size per sprite cell |
| `depth_mode` | `"always_front"` | Standard for all equipment. Holdout occlusion bakes visibility. |
| `animations` | array | 17 animation entries (see below) |

### Standard 17 Animations (copy for all equipment)

The animation entries are IDENTICAL for body, weapon, and headgear configs. Equipment must have the same 17 animations as body because the head/hand position varies per animation.

```json
[
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
```

**Why equipment needs all 17 animations:** The hand and head positions differ between Walking (unarmed) and Run With Sword (onehand). If equipment only had one walk atlas, it would misalign during weapon mode switches.

---

## Command Reference

### Test Frame (verify alignment before full render)

```bash
"C:/Blender 5.1/blender.exe" \
  "{BLEND_FILE}" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_{GENDER}/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_equipment" \
  --test-frame --render-size 512
```

### Full Equipment Render (per gender)

```bash
"C:/Blender 5.1/blender.exe" \
  "{BLEND_FILE}" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_{GENDER}/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/{OUTPUT_NAME}" \
  --weapon-only \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98
```

### Atlas Packing (per gender)

```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/{RENDER_OUTPUT_DIR}" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/{LAYER}/{ITEM_NAME}/{GENDER}" \
  --config "2D animations/atlas_configs/{CONFIG_FILE}"
```

### Render Script Flags

| Flag | Default | Description |
|------|---------|-------------|
| `--anim-dir` | (required) | Directory of Mixamo FBX animation files |
| `--body-output` | (required) | Output directory for body sprites (unused with --weapon-only) |
| `--weapon-output` | (required) | Output directory for equipment sprites |
| `--render-size` | 1024 | Cell size in pixels |
| `--camera-angle` | 10.0 | Camera elevation in degrees |
| `--camera-target-z` | 0.7 | Camera look-at Z height |
| `--cel-shadow` | 0.45 | Cel-shade shadow brightness |
| `--cel-mid` | 0.78 | Cel-shade midtone brightness |
| `--no-cel-shade` | false | Disable cel-shading |
| `--no-outline` | false | Disable outline (applied to body only, not equipment) |
| `--outline-width` | 0.002 | Body outline thickness |
| `--directions` | 8 | Render directions (8 or 4) |
| `--texture-from` | None | GLB for body texture recovery (body only, not equipment) |
| `--body-only` | false | Skip equipment pass |
| `--weapon-only` | false | Skip body pass (standard for equipment renders) |
| `--test-frame` | false | Single frame test render |

---

## File Naming Conventions

### Weapon Files

| Component | Pattern | Example (sword, view_sprite=2) |
|-----------|---------|-------------------------------|
| GLB model | `weapon_templates/{type}/{type}.glb` | `weapon_templates/sword/sword.glb` |
| Male .blend | `weapon_templates/{type}/{type}_on_base_m.blend` | `weapon_templates/sword/sword_on_base_m.blend` |
| Female .blend | `weapon_templates/{type}/{type}_on_base_f.blend` | `weapon_templates/sword/sword_on_base_f.blend` |
| Male render output | `render_output/weapon_{type}_male/` | `render_output/weapon_sword_male/` |
| Female render output | `render_output/weapon_{type}_female/` | `render_output/weapon_sword_female/` |
| Male atlas config | `atlas_configs/weapon_{type}_v2.json` | `atlas_configs/weapon_sword_v2.json` |
| Female atlas config | `atlas_configs/weapon_{type}_f_v2.json` | `atlas_configs/weapon_sword_f_v2.json` |
| Config `character` field | `weapon_{view_sprite_id}` | `weapon_2` |
| Male atlas dir | `Weapon/{type}/male/` | `Weapon/sword/male/` |
| Female atlas dir | `Weapon/{type}/female/` | `Weapon/sword/female/` |
| Manifest filename | `weapon_{id}_manifest.json` | `weapon_2_manifest.json` |
| Per-anim atlas | `weapon_{id}_{anim}.json/png` | `weapon_2_idle.json`, `weapon_2_idle.png` |

### Headgear Files

| Component | Pattern | Example (iron_helm, view_sprite=25) |
|-----------|---------|-------------------------------------|
| GLB model | `Headgear/Top/{name}/{name}.glb` | `Headgear/Top/iron_helm/iron_helm.glb` |
| Male .blend | `Headgear/Top/{name}/{name}_on_base_m.blend` | `Headgear/Top/iron_helm/iron_helm_on_base_m.blend` |
| Female .blend | `Headgear/Top/{name}/{name}_on_base_f.blend` | `Headgear/Top/iron_helm/iron_helm_on_base_f.blend` |
| Male render output | `render_output/headgear_{name}_male/` | `render_output/headgear_iron_helm_male/` |
| Female render output | `render_output/headgear_{name}_female/` | `render_output/headgear_iron_helm_female/` |
| Male atlas config | `atlas_configs/headgeartop_{name}_v2.json` | `atlas_configs/headgeartop_iron_helm_v2.json` |
| Female atlas config | `atlas_configs/headgeartop_{name}_f_v2.json` | `atlas_configs/headgeartop_iron_helm_f_v2.json` |
| Config `character` field | `headgeartop_{view_sprite_id}` | `headgeartop_25` |
| Male atlas dir | `HeadgearTop/{name}/male/` | `HeadgearTop/iron_helm/male/` |
| Female atlas dir | `HeadgearTop/{name}/female/` | `HeadgearTop/iron_helm/female/` |
| Manifest filename | `headgeartop_{id}_manifest.json` | `headgeartop_25_manifest.json` |
| Per-anim atlas | `headgeartop_{id}_{anim}.json/png` | `headgeartop_25_idle.json` |

---

## Directory Structure

```
2D animations/
├── scripts/
│   └── render_blend_all_visible.py          # Dual-pass renderer (body + equipment)
│
├── atlas_configs/
│   ├── weapon_dagger_v2.json                # Weapon configs (male)
│   ├── weapon_dagger_f_v2.json              # Weapon configs (female)
│   ├── weapon_bow_v2.json
│   ├── weapon_bow_f_v2.json
│   ├── headgeartop_egg_shell_hat_v2.json    # Headgear configs (male)
│   └── headgeartop_egg_shell_hat_f_v2.json  # Headgear configs (female)
│
├── 3d_models/
│   ├── weapon_templates/                    # Weapon .blend files
│   │   ├── dagger/
│   │   │   ├── dagger.glb                   # Weapon 3D model
│   │   │   ├── dagger_on_base_m.blend       # Male (weapon on RightHand bone)
│   │   │   └── dagger_on_base_f.blend       # Female
│   │   ├── bow/
│   │   │   ├── bow.glb
│   │   │   ├── bow_on_base_m.blend
│   │   │   └── bow_on_base_f.blend
│   │   └── {new_weapon}/                    # New weapon types follow same pattern
│   │       ├── {weapon}.glb
│   │       ├── {weapon}_on_base_m.blend
│   │       └── {weapon}_on_base_f.blend
│   │
│   ├── Headgear/                            # Headgear .blend files
│   │   ├── Top/
│   │   │   ├── egg_shell_hat/
│   │   │   │   ├── egg_shell_hat.glb
│   │   │   │   ├── egg_shell_hat_on_base_m.blend
│   │   │   │   └── egg_shell_hat_on_base_f.blend
│   │   │   └── {new_headgear}/
│   │   │       ├── {headgear}.glb
│   │   │       ├── {headgear}_on_base_m.blend
│   │   │       └── {headgear}_on_base_f.blend
│   │   ├── Mid/                             # Same structure for mid-slot headgear
│   │   └── Low/                             # Same structure for low-slot headgear
│   │
│   └── animations/characters/
│       ├── base_m/                          # 17 shared male animations (used for ALL renders)
│       └── base_f/                          # 17 shared female animations
│
└── sprites/render_output/
    ├── weapon_{type}_male/                  # Weapon render output (per type per gender)
    ├── weapon_{type}_female/
    ├── headgear_{name}_male/                # Headgear render output
    └── headgear_{name}_female/

client/SabriMMO/Content/SabriMMO/Sprites/Atlases/
├── Weapon/
│   ├── dagger/
│   │   ├── male/                            # weapon_1_manifest.json + 17 atlas pairs
│   │   └── female/                          # weapon_1_manifest.json + 17 atlas pairs
│   ├── bow/
│   │   ├── male/                            # weapon_11_manifest.json + 17 atlas pairs
│   │   └── female/
│   └── {new_weapon}/
│       ├── male/                            # weapon_{ID}_manifest.json + 17 atlas pairs
│       └── female/
│
├── HeadgearTop/
│   ├── egg_shell_hat/
│   │   ├── male/                            # headgeartop_101_manifest.json + 17 atlas pairs
│   │   └── female/
│   └── {new_headgear}/
│       ├── male/                            # headgeartop_{ID}_manifest.json + 17 atlas pairs
│       └── female/
│
├── HeadgearMid/                             # Same structure
│   └── {headgear}/
│       ├── male/
│       └── female/
│
└── HeadgearLow/                             # Same structure
    └── {headgear}/
        ├── male/
        └── female/
```

---

## Database Setup

### Items Table — view_sprite Column

The `view_sprite` column in the `items` table is the single link between an item and its sprite atlas. When a player equips an item, the server sends its `view_sprite` to the client, which loads `{layer}_{view_sprite}_manifest.json`.

```sql
-- Check current view_sprite assignments
SELECT item_id, name_english, weapon_type, view_sprite 
FROM items WHERE weapon_type IS NOT NULL AND view_sprite > 0 
ORDER BY weapon_type, view_sprite;

-- Assign view_sprite to a weapon type
UPDATE items SET view_sprite = 2 WHERE weapon_type = 'one_hand_sword';

-- Assign view_sprite to headgear
UPDATE items SET view_sprite = 101 WHERE item_id = 5015;  -- Egg Shell
UPDATE items SET view_sprite = 25 WHERE item_id IN (2228, 2229);  -- Iron Helm variants
```

### Server Broadcasting

The server automatically broadcasts `view_sprite` values via `broadcastEquipAppearance()`:

```javascript
// Server queries equipped items with view_sprite > 0
const ev = { weapon: 0, shield: 0, head_top: 0, head_mid: 0, head_low: 0, garment: 0 };
// Items with view_sprite > 0 populate the matching slot
if (pos && ev.hasOwnProperty(pos) && row.view_sprite > 0) {
    ev[pos] = row.view_sprite;
}
```

This happens automatically on equip/unequip, zone:ready, and player:appearance. **No server code changes needed** when adding new equipment sprites.

### Client Loading

The client's `LoadEquipmentLayer()` searches all subdirectories under the layer folder for a manifest matching `{layer}_{view_sprite}_manifest.json`:

```
LoadEquipmentLayer(HeadgearTop, 101)
  → Searches Content/SabriMMO/Sprites/Atlases/HeadgearTop/
  → Finds egg_shell_hat/female/headgeartop_101_manifest.json  (if player is female)
  → Falls back to egg_shell_hat/headgeartop_101_manifest.json  (gender-neutral)
```

**No C++ code changes needed** when adding new equipment types. The system auto-discovers manifests by scanning subdirectories.

---

## Multiplayer Sync

Equipment visuals for remote players are sent via `player:appearance` events. This happens automatically when:
- A player equips/unequips an item → `broadcastEquipAppearance()` fires
- A player enters a zone → `zone:ready` sends appearance to all players in the zone
- A new player connects → they receive existing players' appearances via `zone:ready`

**CRITICAL rule**: Visual data is sent in `zone:ready`, NOT `player:join`. During `player:join`, the client's subsystems are being destroyed and recreated (OpenLevel). Events during this time are silently dropped.

**No additional server work needed** for new equipment sprites. The existing `broadcastEquipAppearance()` + `zone:ready` pipeline handles it all.

---

## Troubleshooting

### Rendering Issues

| Problem | Cause | Fix |
|---------|-------|-----|
| **Weapon not in equipment pass** | `parent_type` is OBJECT, not BONE | Re-parent: Select weapon → Shift+Select armature → Ctrl+P → Bone → select correct bone |
| **Equipment renders but includes body** | Body mesh also has `parent_type = BONE` | Body must have `parent_type = OBJECT`. Re-import the base FBX. |
| **Holdout not working (body visible in weapon pass)** | Old render script version | Verify using latest `render_blend_all_visible.py` |
| **Equipment floating off character** | Wrong bone selected during parenting | Check Parent Bone in Properties — should be `mixamorig:RightHand/LeftHand/Head` |
| **Equipment jitters during animation** | Transforms not applied before parenting | Select equipment → Ctrl+A → All Transforms, then re-parent |
| **Equipment too large/small** | Scale wrong | Adjust in Object Mode, then Ctrl+A → Scale before parenting |
| **Headgear clips into head** | Positioned too low | Raise headgear Y position in Blender |

### Atlas Packing Issues

| Problem | Cause | Fix |
|---------|-------|-----|
| **"Folder not found" error** | Render output folder names don't match config `"folder"` | Check exact names in render output vs config |
| **Wrong output path** | Packer places files in wrong directory | Second arg to pack_atlas.py is the PARENT of the gender subfolder |
| **Manifest says wrong character name** | Config `"character"` field wrong | Must be `weapon_{ID}` or `headgeartop_{ID}` exactly |

### In-Game Issues

| Problem | Cause | Fix |
|---------|-------|-----|
| **Equipment not showing** | `view_sprite` is 0 or NULL in DB | Set `view_sprite` on the item in the `items` table |
| **Equipment on wrong layer** | Manifest in wrong folder | Check directory: `Weapon/` for weapons, `HeadgearTop/` for top headgear |
| **Wrong gender equipment loading** | Gender subfolder missing | Ensure both `male/` and `female/` subfolders have manifests |
| **Equipment misaligned** | Camera settings don't match body render | Both must use `--camera-angle 10 --camera-target-z 0.7 --render-size 1024` |
| **Equipment visible when should be behind body** | Not using holdout occlusion | Config must have `"depth_mode": "always_front"` |
| **Remote player missing equipment** | view_sprite not sent in zone:ready | Check server `broadcastEquipAppearance` includes the slot |
| **Hair showing through full helm** | Manifest missing `hides_hair` | Add `"hides_hair": true` to manifest JSON |
| **Old sprite showing after re-import** | UE5 cached old .uasset | Delete .uasset files, reimport PNGs |
| **Equipment sprite wrong animation** | Group mismatch with body | Equipment config groups MUST match body config groups exactly |

### Quick Verification Checklist

Before testing in-game, verify:
- [ ] .blend has equipment mesh with `parent_type = BONE` and correct `parent_bone`
- [ ] Test frame renders correctly (equipment only, body area transparent)
- [ ] Full render produced 17 animation folders
- [ ] Atlas config has `"version": 2`, correct `"character"`, and `"depth_mode": "always_front"`
- [ ] Packer produced manifest + 17 JSON/PNG pairs per gender
- [ ] PNGs imported into UE5 with correct texture settings
- [ ] Both male/ and female/ subfolders have complete atlas sets
- [ ] DB `items.view_sprite` is set for the item(s)

---

## Quick Copy-Paste Templates

### New Weapon Type (replace all {placeholders})

```bash
# 1. Create male .blend (MANUAL in Blender — see Step 2)
# Save to: 3d_models/weapon_templates/{WEAPON_TYPE}/{WEAPON_TYPE}_on_base_m.blend

# 2. Create female .blend (MANUAL in Blender — see Step 3)
# Save to: 3d_models/weapon_templates/{WEAPON_TYPE}/{WEAPON_TYPE}_on_base_f.blend

# 3. Test frame (male)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{WEAPON_TYPE}/{WEAPON_TYPE}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_weapon" \
  --test-frame --render-size 512

# 4. Full render (male)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{WEAPON_TYPE}/{WEAPON_TYPE}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_{WEAPON_TYPE}_male" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 5. Full render (female)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{WEAPON_TYPE}/{WEAPON_TYPE}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_{WEAPON_TYPE}_female" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 6. Create atlas config (copy standard, change "character" to "weapon_{VIEW_SPRITE_ID}")
cp "C:/Sabri_MMO/2D animations/atlas_configs/weapon_dagger_v2.json" \
   "C:/Sabri_MMO/2D animations/atlas_configs/weapon_{WEAPON_TYPE}_v2.json"
# Edit: set "character": "weapon_{VIEW_SPRITE_ID}"

cp "C:/Sabri_MMO/2D animations/atlas_configs/weapon_dagger_f_v2.json" \
   "C:/Sabri_MMO/2D animations/atlas_configs/weapon_{WEAPON_TYPE}_f_v2.json"
# Edit: set "character": "weapon_{VIEW_SPRITE_ID}"

# 7. Pack atlases (male)
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_{WEAPON_TYPE}_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{WEAPON_TYPE}/male" \
  --config "2D animations/atlas_configs/weapon_{WEAPON_TYPE}_v2.json"

# 8. Pack atlases (female)
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/weapon_{WEAPON_TYPE}_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{WEAPON_TYPE}/female" \
  --config "2D animations/atlas_configs/weapon_{WEAPON_TYPE}_f_v2.json"

# 9. UE5 import (MANUAL — import PNGs, set texture settings)
# 10. Database: UPDATE items SET view_sprite = {VIEW_SPRITE_ID} WHERE weapon_type = '{WEAPON_TYPE}';
```

### New Headgear (replace all {placeholders})

```bash
# 1. Create male .blend (MANUAL in Blender — see Headgear Step 2)
# Save to: 3d_models/Headgear/Top/{HEADGEAR_NAME}/{HEADGEAR_NAME}_on_base_m.blend

# 2. Create female .blend (MANUAL in Blender — see Headgear Step 3)
# Save to: 3d_models/Headgear/Top/{HEADGEAR_NAME}/{HEADGEAR_NAME}_on_base_f.blend

# 3. Test frame (male)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{HEADGEAR_NAME}/{HEADGEAR_NAME}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_body" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/test_headgear" \
  --test-frame --render-size 512

# 4. Full render (male)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{HEADGEAR_NAME}/{HEADGEAR_NAME}_on_base_m.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{HEADGEAR_NAME}_male" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 5. Full render (female)
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/Headgear/Top/{HEADGEAR_NAME}/{HEADGEAR_NAME}_on_base_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/characters/base_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/body_discard" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/headgear_{HEADGEAR_NAME}_female" \
  --weapon-only --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --cel-shadow 0.92 --cel-mid 0.98

# 6. Create atlas config (copy from egg shell hat, change "character")
cp "C:/Sabri_MMO/2D animations/atlas_configs/headgeartop_egg_shell_hat_v2.json" \
   "C:/Sabri_MMO/2D animations/atlas_configs/headgeartop_{HEADGEAR_NAME}_v2.json"
# Edit: set "character": "headgeartop_{VIEW_SPRITE_ID}"

cp "C:/Sabri_MMO/2D animations/atlas_configs/headgeartop_egg_shell_hat_f_v2.json" \
   "C:/Sabri_MMO/2D animations/atlas_configs/headgeartop_{HEADGEAR_NAME}_f_v2.json"
# Edit: set "character": "headgeartop_{VIEW_SPRITE_ID}"

# 7. Pack atlases (male)
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{HEADGEAR_NAME}_male" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{HEADGEAR_NAME}/male" \
  --config "2D animations/atlas_configs/headgeartop_{HEADGEAR_NAME}_v2.json"

# 8. Pack atlases (female)
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "sprites/render_output/headgear_{HEADGEAR_NAME}_female" \
  "client/SabriMMO/Content/SabriMMO/Sprites/Atlases/HeadgearTop/{HEADGEAR_NAME}/female" \
  --config "2D animations/atlas_configs/headgeartop_{HEADGEAR_NAME}_f_v2.json"

# 9. UE5 import (MANUAL — import PNGs, set texture settings)
# 10. Database: UPDATE items SET view_sprite = {VIEW_SPRITE_ID} WHERE item_id = {ITEM_ID};
# 11. Optional: Add "hides_hair": true to manifest if it's a full helm
```
