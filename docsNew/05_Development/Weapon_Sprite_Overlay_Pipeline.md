# Weapon Sprite Overlay Pipeline

Complete, verified pipeline for rendering weapon sprites that overlay pixel-perfectly on body sprites. This document captures the exact working state as of 2026-03-25.

---

## Overview

Weapons are rendered as **transparent overlay sprites** that composite on top of body sprites at runtime. Both body and weapon are rendered from the **same Blender scene** in a dual-pass approach, guaranteeing pixel-perfect alignment.

**Key principle**: Body and weapon sprites MUST be rendered from the SAME .blend file, with the SAME camera, centering, and ortho_scale. Any difference — even fractional pixels — causes visible misalignment.

---

## Pipeline Summary

```
1. Create .blend template (weapon positioned on hand bone)
2. Dual-pass render (body + weapon from same scene)
3. Pack atlases (body + weapon, matching configs)
4. Import into UE5 (delete old .uasset first)
5. C++ equipment layer composites at runtime
```

---

## Step 1: Create the .blend Template

### What you need
- **Mixamo FBX**: The character's rigged FBX (has Mixamo skeleton, no textures). This is the same FBX used for body sprite rendering.
- **Weapon GLB**: The weapon model from Tripo3D (has textures, no skeleton).

### Procedure (in Blender)

1. **Import the Mixamo FBX**: File → Import → FBX. This gives you the armature + body mesh.
2. **Import the weapon GLB**: File → Import → glTF. This gives you the weapon mesh with textures.
3. **Parent weapon to hand bone**:
   - Select the weapon mesh
   - Shift-select the Armature
   - Switch to Pose Mode
   - Select the `mixamorig:RightHand` bone
   - Ctrl+P → Bone
4. **Position the weapon**: In Object Mode, select the weapon and adjust Location/Rotation/Scale in the Properties panel until it looks correct in the character's hand.
5. **Save as .blend**: e.g., `dagger_on_swordsman.blend`

### Verification checklist
Run this inspection script (or check in Blender's Outliner):
- Armature exists with Mixamo bones (`mixamorig:RightHand` etc.)
- Body mesh: `parent=Armature`, `parent_type=OBJECT`, `parent_bone=""` (empty)
- Weapon mesh: `parent=Armature`, `parent_type=BONE`, `parent_bone=mixamorig:RightHand`
- The `parent_bone` field is how the render script distinguishes body vs equipment

### Important notes
- The weapon is positioned **once**. It follows the hand bone through ALL animations automatically via bone parenting.
- You do NOT need a separate .blend per animation or per direction.
- The .blend can be saved in any pose — the render script resets to rest pose before centering.
- One .blend per weapon model per character class (e.g., `dagger_on_swordsman.blend`, `sword_on_swordsman.blend`).

### File location
```
2D animations/3d_models/weapon_templates/{weapon}_on_{class}.blend
```

---

## Step 2: Dual-Pass Render

### Script
`2D animations/render_blend_all_visible.py`

### What it does
1. Opens the .blend file (weapon already positioned on hand bone)
2. Removes existing cameras/lights from .blend
3. Imports all Mixamo FBX animations from `--anim-dir` (with Blender 5.x `action_slot` fix)
4. Strips root motion (XY translation on Hips/Root bones)
5. **Resets armature to rest pose** (clears all bone transforms → T-pose for consistent centering)
6. Optionally recovers body textures from GLB (`--texture-from`)
7. **Centers + normalizes using BODY mesh bounds only** (equipment hidden during measurement, height → 2.0)
8. Applies **cel-shading** to ALL meshes (body + equipment) for consistent style
9. Applies **outline** to body meshes only (solidify modifier — too thick on small weapons)
10. **Auto-frames camera from body bounds only** (equipment hidden during measurement)
11. Sets up 3-point lighting (matches blender_sprite_render_v2.py exactly)
12. **Freezes** camera position, ortho_scale, centering — never recomputed
13. For each (direction, animation, frame), renders **TWO passes**:
    - **Pass 1**: Body visible, equipment `hide_render=True` → body output directory
    - **Pass 2**: Body uses **Holdout shader** (transparent occluder), equipment visible → weapon output directory. Only equipment pixels visible from the camera are rendered; equipment behind the body is occluded by the holdout body.
14. Both passes share the IDENTICAL frozen camera state → pixel-perfect alignment guaranteed

### Body vs Equipment detection
- **Body mesh**: `obj.parent_bone` is empty/falsy (parented to armature as OBJECT)
- **Equipment mesh**: `obj.parent_bone` is set (e.g., `mixamorig:RightHand`)

### Blender 5.x action_slot fix (CRITICAL)
When importing FBX animations into Blender 5.x, simply setting `armature.animation_data.action = new_action` does NOT play the animation. You MUST also set:
```python
armature.animation_data.action_slot = action.slots[0]
```
Without this, the armature stays frozen in the .blend file's saved pose.

### Production command
```bash
"C:/Blender 5.1/blender.exe" \
  "C:/Sabri_MMO/2D animations/3d_models/weapon_templates/dagger_on_swordsman.blend" \
  --background --python "C:/Sabri_MMO/2D animations/render_blend_all_visible.py" -- \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/swordsman_m/" \
  --body-output "C:/Sabri_MMO/2D animations/sprites/render_output/swordsman_m" \
  --weapon-output "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_dagger" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7
```

### Available flags
| Flag | Default | Description |
|------|---------|-------------|
| `--anim-dir` | required | Directory of Mixamo FBX animation files |
| `--body-output` | required | Output directory for body sprites |
| `--weapon-output` | required | Output directory for weapon overlay sprites |
| `--render-size` | 1024 | Render resolution per cell |
| `--camera-angle` | 10.0 | Camera elevation degrees |
| `--camera-target-z` | 0.7 | Camera look-at Z height |
| `--cel-shadow` | 0.92 | Cel-shade shadow brightness (standard 2026-03-29) |
| `--cel-mid` | 0.78 | Cel-shade midtone brightness |
| `--no-cel-shade` | false | Disable cel-shading |
| `--no-outline` | false | Disable body outline |
| `--outline-width` | 0.002 | Outline thickness |
| `--directions` | 8 | Number of directions (4 or 8) |
| `--texture-from` | none | GLB to recover body textures from |
| `--body-only` | false | Skip weapon pass |
| `--weapon-only` | false | Skip body pass |
| `--test-frame` | false | Render 1 frame only (S dir, first anim) for alignment test |

### Expected output
- ~1392 body sprites in `--body-output` (20 anims × ~8 avg frames × 8 dirs)
- ~1392 weapon sprites in `--weapon-output` (same layout)
- Per-animation subfolders with naming: `{AnimName}/{AnimName}_{Direction}_f{Frame:02d}.png`
- Total render time: ~15-20 minutes at 1024px

### Test before full render
Always run `--test-frame` first to verify alignment:
```bash
... --test-frame --render-size 512
```
This renders 1 body + 1 weapon frame in seconds. Visually compare to confirm the dagger is at the hand position.

---

## Step 3: Pack Atlases

### Body atlas config
Use the existing v2 config: `2D animations/atlas_configs/swordsman_m_v2.json`

```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/swordsman_m" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body" \
  --config "C:/Sabri_MMO/2D animations/atlas_configs/swordsman_m_v2.json"
```

### Weapon atlas config

**CRITICAL**: The `character` field MUST be `weapon_{view_sprite_id}` where `view_sprite_id` is the integer from the server's `items.view_sprite` database column. The C++ code constructs the manifest path as `weapon_{id}_manifest.json`.

Config file: `2D animations/atlas_configs/weapon_dagger_v2.json`
```json
{
  "version": 2,
  "character": "weapon_1",
  "cell_size": 1024,
  "animations": [
    ... (must mirror the body config exactly — same folders, states, groups)
  ]
}
```

The weapon config MUST have the **same animation entries** (folder, state, group) as the body config. This ensures body and weapon atlases have matching frame counts and direction columns per animation.

```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/weapon_dagger" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Weapon" \
  --config "C:/Sabri_MMO/2D animations/atlas_configs/weapon_dagger_v2.json"
```

### Output
- Body: 19 atlases → `Content/SabriMMO/Sprites/Atlases/Body/swordsman_m_*.{png,json}` + manifest
- Weapon: 19 atlases → `Content/SabriMMO/Sprites/Atlases/Weapon/weapon_1_*.{png,json}` + manifest

---

## Step 4: Import into UE5

### CRITICAL: Delete old .uasset files first
UE5 caches textures in `.uasset` files. If you replace the source PNGs on disk, UE5 does NOT automatically update the `.uasset`. You MUST:

1. **Close the UE5 editor**
2. Delete all old `.uasset` files for the affected atlases:
   ```bash
   rm -f Content/SabriMMO/Sprites/Atlases/Body/swordsman_m_*.uasset
   rm -f Content/SabriMMO/Sprites/Atlases/Weapon/weapon_1_*.uasset
   ```
3. Also delete any stale files from old render pipelines:
   - `swordsman_m_onehand.*` (v1 weapon-group atlas)
   - `swordsman_m_twohand.*` (v1 weapon-group atlas)
   - `swordsman_m_unarmed.*` (v1 weapon-group atlas)
   - `swordsman_m_shared.*` (v1 weapon-group atlas)
   - `swordsman_m_fighting_idle.*` (removed animation)
   - `weapon_dagger_*.*` (wrong naming — must be weapon_1)
4. **Open UE5** — it will auto-import the PNGs as new textures
5. **Set texture properties** on all new textures (REQUIRED — all six settings):
   - Compression: **BC7** (`TC_BC7`)
   - Filter: **Nearest** (`TF_NEAREST`) — no blur
   - Mip Gen Settings: **NoMipmaps** (`TMGS_NO_MIPMAPS`)
   - Never Stream: **On**
   - sRGB: **Off** (linear color so cel-shade ramp stays accurate)
   - **Texture Group: UI** (`TEXTUREGROUP_UI`) — sprite atlases are UI textures, not world textures

   The proven Python block (reusable in all import scripts):
   ```python
   texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
   texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
   texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
   texture.set_editor_property("never_stream", True)
   texture.set_editor_property("srgb", False)
   texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
   ```

---

## Step 5: C++ Runtime Compositing

The equipment layer system is fully implemented in `SpriteCharacterActor.cpp`. No code changes needed for new weapons — just add atlas files.

### How it works
1. Server sends `player:appearance` with `view_sprite` ID per equipment slot and `weaponMode` (0=unarmed, 1=onehand, 2=twohand, 3=bow)
2. `LoadEquipmentLayer(ESpriteLayer::Weapon, viewSpriteId)` loads `weapon_{id}_manifest.json`
3. Equipment atlas entries are registered with **group-aware** weapon mode mapping (shared/unarmed/onehand/twohand/bow)
4. `ResolveLayerAtlas` picks the same variant index as the body (`ActiveVariantIndex`)
5. `UpdateQuadUVs` renders the weapon quad with UV flip compensation at the same position as the body quad
6. Equipment is always composited at `+5` (in front of body). **Holdout occlusion** (2026-03-27) bakes correct visibility into the sprite at render time — the body uses a Holdout shader during Pass 2, so equipment behind the body is already occluded in the sprite. No per-direction depth toggling needed.
7. Legacy `depth_front` arrays and global majority vote system are deprecated. `depth_mode: "always_front"` is the standard for all equipment.

### Four critical C++ fixes (all in SpriteCharacterActor.cpp)

These fixes are required for correct weapon overlay display. They are already applied in the working codebase.

#### Fix 1: Group-aware equipment registration (LoadEquipmentLayer)
Equipment atlas entries must respect the `group` field (shared/unarmed/onehand/twohand) — same logic as body's `LoadV2AtlasManifest`. Without this, ALL animations register under ALL weapon modes, causing the weapon to show the wrong animation (e.g., "punching" dagger overlay during "sword slash" body animation).

```cpp
// CORRECT — parses group field, registers under matching mode(s)
TArray<FString> Groups;
GroupName.ParseIntoArray(Groups, TEXT(","));
for (const FString& G : Groups) { ... }

// WRONG — ignores group, registers under ALL modes
for (int32 m = 0; m < MAX; m++) { ... }
```

#### Fix 2: Variant index sync (ResolveLayerAtlas)
Equipment must use `ActiveVariantIndex` (same as body) instead of hardcoded 0. Without this, body might show "Stable Sword Inward Slash" (variant 0) while weapon shows "Stable Sword Outward Slash" (variant 1) — different arm positions.

```cpp
// CORRECT
int32 Idx = FMath::Clamp(ActiveVariantIndex, 0, Variants->Num() - 1);
Layer.ActiveLayerAtlas = &(*Variants)[Idx];

// WRONG
Layer.ActiveLayerAtlas = &(*Variants)[0];
```

#### Fix 3: UV horizontal flip (UpdateQuadUVs + UpdateBodyQuadUVs)
The billboard rotation (`(-CamFwd).Rotation()`) negates the local Y axis when Yaw ≈ 180°, which mirrors all sprites horizontally. Swapping U0/U1 in the UV mapping compensates for this mirror.

```cpp
// CORRECT — U1 first, then U0 (flipped)
NewUVs.Add(FVector2D(U1, V1));  // bottom-left vertex
NewUVs.Add(FVector2D(U0, V1));  // bottom-right vertex
NewUVs.Add(FVector2D(U0, V0));  // top-right vertex
NewUVs.Add(FVector2D(U1, V0));  // top-left vertex

// WRONG — standard mapping (mirrored by billboard)
NewUVs.Add(FVector2D(U0, V1));
NewUVs.Add(FVector2D(U1, V1));
NewUVs.Add(FVector2D(U1, V0));
NewUVs.Add(FVector2D(U0, V0));
```

This fix must be applied in BOTH `UpdateQuadUVs` (equipment layers) AND `UpdateBodyQuadUVs` (body layer).

#### Fix 4: Direction calculation (CalculateDirection)
The original direction calc negated both Cross and Dot: `Atan2(-Cross, -Dot)`. The `-Cross` compensated for the billboard horizontal mirror. With Fix 3 handling the mirror via UV flip, the `-Cross` is no longer needed and causes directions to be inverted (East shows West, etc.).

```cpp
// CORRECT — only Dot negated (handles front/back mapping)
float Angle = FMath::Atan2(Cross, -Dot);

// WRONG with UV flip active — double-flips left/right
float Angle = FMath::Atan2(-Cross, -Dot);
```

#### Fix 5: Holdout Occlusion (replaces per-frame depth ordering)

**Current method (2026-03-27)**: The render script uses a **Blender Holdout shader** on the body during Pass 2. The body is transparent but still occludes equipment behind it. Only equipment pixels visible from the render camera appear in the output sprite. At runtime, equipment is always composited at +5 (in front of body) — no per-direction depth toggling needed.

```cpp
// CURRENT — always in front, holdout occlusion baked into sprite
Layer.MeshComp->SetRelativeLocation(FVector(5.f, 0.f, 0.f));
```

The C++ runtime still supports `depth_front` arrays for backwards compatibility. If `DepthFront.Num() == 0` (which is the case with `depth_mode: "always_front"`), it defaults to +5.

**Deprecated method**: The old system hid the body during Pass 2 (body fully invisible), rendering equipment completely including parts behind the body. It then used `depth_front` arrays with a global majority vote to toggle equipment between +5 (front) and -5 (behind) per direction at runtime. This caused edge cases with partially-visible equipment and depth popping during animations.

The `depth_map.json` is still generated by the render script for legacy compatibility but is ignored by the packer when `depth_mode` is `"always_front"`.

**These four fixes are interdependent.** Fix 3 (UV flip) requires Fix 4 (direction). Fix 1 (groups) and Fix 2 (variant) are independent but both required for correct animation matching. Fix 5 (holdout) is handled entirely by the render script — no C++ changes needed beyond the default +5 offset.

---

## Adding a New Weapon

To add a new weapon (e.g., sword, axe, mace):

1. **Get the weapon GLB** from Tripo3D (or any 3D model source)
2. **Create a .blend template**: Import Mixamo FBX + weapon GLB, parent weapon to the correct hand bone (`mixamorig:RightHand` for melee, `mixamorig:LeftHand` for bows), position it, save
3. **Determine the view_sprite ID**: Check/assign in the server's `items` table `view_sprite` column. All items sharing the same visual use the same ID (e.g., all daggers = 1, all swords = 2, all bows = 11)
4. **Create weapon atlas config**: Copy `weapon_dagger_v2.json`, change `character` to `weapon_{id}` (e.g., `weapon_11` for bows)
5. **Run dual-pass render**: Use the production command with the new .blend and output dirs
6. **Pack weapon atlases**: Use pack_atlas.py with the new config (`depth_mode: "always_front"` — holdout occlusion baked from render)
7. **Import into UE5**: Import PNGs in correct subfolder (`Weapon/bow/`), set texture properties
8. **No C++ changes needed** — the equipment layer system auto-loads from manifest

### Existing weapon types

| Weapon | view_sprite | Hand bone | Config | Atlas dir |
|--------|-------------|-----------|--------|-----------|
| Dagger | 1 | RightHand | `weapon_dagger_v2.json` | `Weapon/dagger/` |
| Bow | 11 | LeftHand | `weapon_bow_v2.json` | `Weapon/bow/` |

### Weapon mode mapping (server → client)

| Server weaponMode | C++ ESpriteWeaponMode | Trigger |
|-------------------|----------------------|---------|
| 0 | None (unarmed) | No weapon equipped |
| 1 | OneHand | Non-bow, non-two-handed weapon |
| 2 | TwoHand | `two_handed=true` and NOT bow |
| 3 | Bow | `weapon_type='bow'` |

Server computes: `weapon_type === 'bow' ? 3 : two_handed ? 2 : 1` (3 locations in index.js: `broadcastEquipAppearance`, `zone:ready` early broadcast, `player:join` equip query)

---

## Multiplayer Remote Sync (Critical)

Weapon sprites must load on remote players immediately when they appear — not just after equip/unequip. This requires correct timing on BOTH directions:

### Direction 1: Existing player sees NEW player's weapon

**Server** (`player:join` early broadcast block, ~line 5417): The initial `player:moved` broadcast includes `equipVisuals` so existing players can load the weapon sprite in the same handler that creates the remote sprite.

**CRITICAL**: This broadcast runs BEFORE `let jobClass` is declared. Use `earlyJobClass` (from DB row via `var`) instead. JavaScript's temporal dead zone makes `let` variables inaccessible before declaration — this caused silent failures that aborted the entire broadcast block.

**Client** (`HandlePlayerMoved` in OtherPlayerSubsystem.cpp): When creating a new remote sprite, reads `equipVisuals` from the `player:moved` JSON and calls `LoadEquipmentLayer` for each slot.

### Direction 2: NEW player sees EXISTING players' weapons

**Server** (`zone:ready` handler, NOT `player:join`): Sends `player:appearance` for all existing players in the zone to the joining player.

**WHY zone:ready**: During `player:join`, the client does a zone transition (OpenLevel). The OtherPlayerSubsystem is destroyed and recreated during this transition. Any `player:appearance` events sent during `player:join` arrive when no handler is listening — they're silently dropped. By `zone:ready`, the client has finished loading, all subsystems are registered, and handlers are active.

**Client** (`HandlePlayerAppearance`): Caches equipment visuals on the FPlayerEntry even if the sprite doesn't exist yet. When the sprite IS created (from 30Hz position updates), cached visuals are applied immediately.

### The Zone:Ready Rule (applies to ALL multiplayer visual sync)

> **ANY visual/state data that needs to be received by a client's subsystem handlers MUST be sent in or after `zone:ready`, NOT in `player:join`.** The `player:join` events arrive during the zone transition when subsystems don't exist yet.

This pattern applies to: weapon sprites, shield sprites, headgear sprites, costume overlays, mount visuals, buff visual effects, guild emblems, party indicators, vending shop signs — anything that requires a client-side subsystem to process.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Weapon on wrong hand | UV flip not applied (Fix 3) | Apply U0/U1 swap in both UV functions |
| Directions inverted (E↔W) | Direction calc still has -Cross (Fix 4) | Change to `Atan2(Cross, -Dot)` |
| Weapon shows wrong animation | Group registration ignores groups (Fix 1) | Apply group-aware parsing in LoadEquipmentLayer |
| Weapon variant doesn't match body | ResolveLayerAtlas uses index 0 (Fix 2) | Use ActiveVariantIndex |
| Weapon not loading | Manifest name mismatch | Must be `weapon_{view_sprite_id}_manifest.json` |
| Weapon misaligned with body | Body .uasset is from old render | Delete .uasset files, reimport new PNGs |
| Animation frozen in .blend pose | Missing action_slot assignment | Add `ad.action_slot = action.slots[0]` |
| Test frame looks correct but full render doesn't | Stale .uasset cache | Delete ALL .uasset for both Body and Weapon |
| Weapon always in front (expected behavior) | Holdout occlusion bakes visibility into sprite | This is correct — equipment behind body is occluded at render time, runtime always uses +5 |
| Equipment visible where it should be hidden | Holdout shader not applied during Pass 2 | Check render_blend_all_visible.py creates Holdout material and swaps body meshes in Pass 2 |
| Legacy: Weapon depth not changing per frame | DepthFront array empty in C++ | Expected with depth_mode "always_front" — holdout handles occlusion |
| Sit animation shows Cast instead | Server `buff:applied` with skillId=0 (sitting) triggers Cast | Guard `skill:buff_applied`: skip if `CurrentAnimState==Sit` or `skillId==0` |
| Remote weapon not shown on initial login (either direction) | Server `player:join` broadcast crashes silently | Check server log for `Cannot access 'X' before initialization` — use `var`/early query vars |
| Remote weapon not shown (joining player doesn't see existing) | `player:appearance` sent in `player:join` before zone transition | Move appearance broadcast to `zone:ready` handler |
| Remote equipLoaded=0 cachedVisuals=0 | Entry not in Players map when equipment code runs | Create entry BEFORE equipment loading, preserve cached EquipVisuals from placeholder |

---

## File Reference

| File | Purpose |
|------|---------|
| `2D animations/render_blend_all_visible.py` | Dual-pass render script (body + weapon) |
| `2D animations/blender_sprite_render_v2.py` | Original body-only render (still used for classes without weapons) |
| `2D animations/pack_atlas.py` | V2 atlas packer |
| `2D animations/atlas_configs/swordsman_m_v2.json` | Body atlas config |
| `2D animations/atlas_configs/weapon_dagger_v2.json` | Weapon atlas config (character="weapon_1") |
| `2D animations/3d_models/weapon_templates/*.blend` | .blend templates (weapon positioned on hand bone) |
| `client/.../Sprite/SpriteCharacterActor.cpp` | C++ sprite actor (all 4 fixes) |
| `client/.../Sprite/SpriteCharacterActor.h` | Sprite actor header (FSpriteLayerState, ESpriteLayer) |
| `client/.../Sprite/SpriteAtlasData.h` | Atlas data structs (FSingleAnimAtlasInfo, FSpriteAtlasKey) |

---

## Proven Render Settings

| Setting | Value |
|---------|-------|
| Render size | 1024x1024 |
| Camera angle | 10° elevation |
| Camera target Z | 0.7 |
| Cel-shade shadow | 0.92 |
| Cel-shade mid | 0.78 |
| Outline width | 0.002 (body only) |
| Directions | 8 (S, SW, W, NW, N, NE, E, SE) |
| Engine | EEVEE |
| Camera | Orthographic |
| Background | Transparent (film_transparent) |
