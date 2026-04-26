# New Weapon Atlas — Female Sprite (Starter Prompt)

> **Paste the section between the `---START PROMPT---` and `---END PROMPT---` markers into a fresh Claude Code session.** Replace `{WEAPON}` and `{VIEW_SPRITE_ID}` with your actual values.

---START PROMPT---

I need to add a new weapon overlay atlas for the **female swordsman** sprite. The weapon is **{WEAPON}** (e.g., katar, sword_1h, axe_1h, mace, etc.). Its database `view_sprite` ID is **{VIEW_SPRITE_ID}** (e.g., 15 for katar).

**Load these skills first:**
- `/sabrimmo-3d-to-2d`
- `/sabrimmo-sprites`

**MANDATORY READING before starting:**
- `docsNew/05_Development/Weapon_Sprite_Overlay_Pipeline.md` — full pipeline reference
- `memory/weapon-overlay-attempts-2026-03-25.md` — solved approach + holdout occlusion update
- `memory/sprite-rendering-2026-04-06.md` — runtime sprite material + holdout
- `memory/feedback-remote-initial-load.md` — multiplayer sync rules

**Existing reference implementations to copy from:**
- Dagger (female) — view_sprite=1, atlases at `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/dagger/female/weapon_1_*`
- Bow (female) — view_sprite=11, atlases at `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/bow/female/weapon_11_*`
- Templates: `2D animations/3d_models/weapon_templates/dagger/daggerSmall_f.blend` and `2D animations/3d_models/weapon_templates/bow/bow_on_base_f.blend`

**What I'll provide:**
1. A `.blend` file with the {WEAPON} mesh **already bone-parented to `mixamorig:RightHand`** of the female base armature
2. The weapon's `view_sprite` ID (database column)
3. The weapon's RO classification (one-handed, two-handed, bow, katar, etc.)

**Goal**: Produce 17 packed atlases at `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{WEAPON}/female/weapon_{VIEW_SPRITE_ID}_*` using the holdout-occlusion dual-pass render, then verify they load in-game on the female sprite.

**Don't deviate from the proven pipeline.** Use `render_blend_all_visible.py` (current dual-pass + holdout), `pack_atlas.py` v2 with `depth_mode: "always_front"`, and follow the exact 17-animation list from `weapon_dagger_v2.json`. Verify the atlas `character` field is `weapon_{VIEW_SPRITE_ID}` so the C++ `LoadEquipmentLayer` finds the manifest.

**End-to-end verification**: render → pack → reimport in UE5 → log into game with the weapon equipped → confirm sprite shows in 1H/2H/bow animations as appropriate, on both local AND remote players (dual login test).

---END PROMPT---

# Reference: Full Female Weapon Atlas Pipeline

Below is the complete pipeline so any session can follow it precisely.

## Current System State (as of 2026-04-15)

### What's already done
- **Pipeline scripts**: dual-pass render + holdout occlusion + v2 atlas packer
- **C++ runtime**: gender-aware path resolution, group-aware atlas registration, multiplayer sync
- **Female sprites populated**: dagger, bow (both genders)
- **Server logic**: katar forced to OneHand, bow → weaponMode=3, two_handed → TwoHand

### What's NOT done
- 13 of 15 weapon types have empty folders (sword_1h, sword_2h, axe_1h, axe_2h, mace, spear_1h, spear_2h, rod, katar, book, knuckle, instrument, whip)
- Most class/gender body folders are empty placeholders

---

## Pipeline Components — Verified Current State

### Render script
**Path**: `C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py`

**Behavior**:
- Opens a `.blend` template (weapon already bone-parented to `mixamorig:RightHand`)
- Imports all Mixamo FBX animations from `--anim-dir`
- Resets armature to rest pose for consistent centering
- Centers + normalizes using **body bounds only** (equipment hidden during measurement)
- Applies cel-shading + outline to body
- Auto-frames camera from body bounds (frozen for both passes)
- **Pass 1 — Body**: equipment `hide_render=True`, body renders with cel-shade + outline → `body-output/`
- **Pass 2 — Equipment**: body swapped to **Holdout shader** (transparent occluder), equipment renders → `weapon-output/`
- Holdout bakes physical occlusion into the equipment sprite — runtime always composites at +5 (in front)

**CLI**:
```
"C:/Blender 5.1/blender.exe" <template.blend> --background \
  --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir <FBX_directory> \
  --body-output <body_dest> \
  --weapon-output <weapon_dest> \
  --render-size 1024 \
  --camera-angle 10 \
  --camera-target-z 0.7 \
  [--test-frame]      # render 1 frame for alignment verification
  [--weapon-only]     # skip body pass (re-render weapon only)
  [--body-only]       # skip weapon pass
```

### Atlas packer
**Path**: `C:/Sabri_MMO/2D animations/scripts/pack_atlas.py`

**Config format** (v2, weapon):
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

**`character` field is critical** — must be `weapon_{view_sprite_id}` because the C++ `LoadEquipmentLayer` constructs the manifest path as `weapon_{id}_manifest.json`.

**`depth_mode: "always_front"`** tells the packer to omit `depth_front` arrays (holdout already baked occlusion into the sprite).

**Group → weapon mode mapping**:
- `unarmed` → ESpriteWeaponMode::None (0)
- `onehand` → OneHand (1) — also receives katar items via server force
- `twohand` → TwoHand (2)
- `bow` → Bow (3)
- `shared` → registered under ALL modes (used for state animations like hit, death, sit)

### C++ runtime
**File**: `client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.cpp`

**`LoadEquipmentLayer(layer, viewSpriteId)`** — searches for manifest in this priority order:
1. `Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_subdir}/{gender}/weapon_{id}_manifest.json` (e.g., `Weapon/dagger/female/weapon_1_manifest.json`)
2. `Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_subdir}/weapon_{id}_manifest.json`
3. `Content/SabriMMO/Sprites/Atlases/Weapon/weapon_{id}_manifest.json` (flat fallback)

The weapon_subdir comes from the manifest search — the C++ iterates all subdirectories until it finds a matching `weapon_{id}_manifest.json`. So `dagger/`, `bow/`, `katar/` etc. all work as long as the file is named `weapon_{id}`.

**Gender** is determined from the player character data and used as a subdirectory in the search.

### Server logic
**File**: `server/src/index.js`

**Weapon mode formula** (3 places: line ~444, 5951, 6177):
```js
weaponMode = row.weapon_type === 'bow' ? 3
           : (row.weapon_type === 'katar' || !row.two_handed) ? 1
           : 2;
```

**Equipment broadcast**: included in initial `player:moved` (existing players see new player) AND `zone:ready` handler (new player sees existing players).

---

## Step-by-Step: Adding a New Weapon Atlas for Female

### Step 0: Prerequisites you must have
- A `.blend` file with the weapon mesh **already parented to `mixamorig:RightHand`** of the female base Mixamo armature. Naming convention: `{weapon}_on_base_f.blend` or `{weapon}Small_f.blend` (see existing examples).
- The weapon's `view_sprite` integer ID (from the `items` table).
- Knowledge of the weapon's RO classification (so you assign the right `weapon_type` and `two_handed` in the database — this affects `weaponMode` server-side).

### Step 1: Place the .blend in the right directory
```
2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_name}_f.blend
```
Examples that already exist:
- `weapon_templates/dagger/daggerSmall_f.blend`
- `weapon_templates/bow/bow_on_base_f.blend`

### Step 2: Verify .blend setup (optional but recommended)
Run a Blender inspection script to confirm:
- Armature exists with Mixamo bones
- Weapon mesh has `parent_type=BONE` and `parent_bone=mixamorig:RightHand`
- Body mesh has no `parent_bone` (so the script can detect equipment vs body)

The render script auto-detects equipment via `obj.parent_bone` — anything bone-parented is treated as equipment.

### Step 3: Create the atlas config
Copy `2D animations/atlas_configs/weapon_dagger_v2.json` to `weapon_{NAME}_f_v2.json` and change ONLY the `character` field to `weapon_{VIEW_SPRITE_ID}`. Keep all 17 animations and `depth_mode: "always_front"`.

### Step 4: Test render (1 frame, fast)
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/{weapon_type}/{weapon_name}_f.blend" \
  --background --python "C:/Sabri_MMO/2D animations/scripts/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/swordsman_f/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/_test_body_f" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/_test_weapon_{NAME}_f" \
  --render-size 512 \
  --camera-angle 10 \
  --camera-target-z 0.7 \
  --test-frame
```

**Visually verify**: weapon appears at the female swordsman's right hand, no obvious clipping.

### Step 5: Full production render (~15-20 min)
Same command as Step 4 but **remove `--test-frame`** and change `--render-size 512` back to `1024`. Output goes to `sprites/render_output/weapon_{name}_f/{animation_folder}/...`.

### Step 6: Pack atlases
```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_{name}_f" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/female" \
  --config "C:/Sabri_MMO/2D animations/atlas_configs/weapon_{name}_f_v2.json"
```

This produces:
- `weapon_{ID}_idle.png/json`, `weapon_{ID}_2hand_idle.png/json`, ... (17 atlases)
- `weapon_{ID}_manifest.json`

All in `Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/female/`.

### Step 7: Delete stale .uasset files (CRITICAL)
UE5 caches textures in `.uasset` files. If any old ones exist with the same names, UE5 will reuse the old data and ignore the new PNGs.

```bash
rm -f "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon/{weapon_type}/female/"weapon_{ID}_*.uasset
```

### Step 8: Open UE5 and import
- Editor auto-imports the PNGs (or manually drag them into the Content Browser)
- For each new texture, set:
  - **Compression**: BC7
  - **Filter**: Nearest
  - **Mip Gen Settings**: NoMipmaps
  - **Never Stream**: On
  - **sRGB**: Off (linear color, sprites are color-precise)

### Step 9: Database entry
Add the item to the `items` table with:
- `view_sprite = {VIEW_SPRITE_ID}` matching your atlas
- `weapon_type = '{type}'` (one of: `dagger`, `sword_1h`, `sword_2h`, `axe_1h`, `axe_2h`, `mace`, `spear_1h`, `spear_2h`, `rod`, `bow`, `katar`, `book`, `knuckle`, `instrument`, `whip`)
- `two_handed = 0` or `1` per RO classification (server overrides for katars/bows)

### Step 10: Multiplayer verification
1. Restart the Node.js server (`server/start-server.bat`)
2. Open UE5, log in as a female swordsman with the weapon equipped
3. Verify sprite locally
4. Open a SECOND PIE instance, log in as another character
5. Verify both clients see each other's weapon sprite WITHOUT having to unequip/re-equip

If a remote player's weapon doesn't show on initial login, see `feedback-remote-initial-load.md` — never run a sync flow that depends on `player:join` timing alone; always verify the `zone:ready` broadcast path is working.

---

## Proven render settings (don't change without reason)

| Setting | Value |
|---|---|
| Render size | 1024×1024 |
| Camera angle | 10° elevation |
| Camera target Z | 0.7 |
| Cel-shade shadow | 0.45 |
| Cel-shade midtone | 0.78 |
| Outline width | 0.002 (body only) |
| Directions | 8 (S, SW, W, NW, N, NE, E, SE) |
| Engine | EEVEE |
| Camera | Orthographic |
| Background | Transparent (film_transparent) |
| Holdout occlusion | Enabled in Pass 2 (default) |
| `depth_mode` | `"always_front"` |

## Common pitfalls

1. **`character` field doesn't match view_sprite ID** → C++ can't find the manifest, weapon never loads. Always `weapon_{ID}`.
2. **Forgot `depth_mode: "always_front"`** → packer emits depth_front arrays, runtime depth ordering kicks in, weapon may appear behind body randomly.
3. **Stale .uasset files** → UE5 ignores new PNGs. Delete .uasset files first.
4. **Texture sRGB on** → colors look washed out. Set sRGB=Off.
5. **Wrong gender folder** → C++ falls back to flat path or fails. Use `Weapon/{weapon}/female/` exactly.
6. **Server not restarted** → old weaponMode logic, katar/bow exceptions don't apply.
