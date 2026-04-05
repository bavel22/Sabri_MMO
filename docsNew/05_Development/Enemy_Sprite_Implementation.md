# Enemy Sprite Implementation Guide

How to add animated sprites to enemy monsters using the same `SpriteCharacterActor` system as players.

---

## Overview

Enemies use the same sprite system as players — `ASpriteCharacterActor` with v2 per-animation atlases. The server sends `spriteClass` in `enemy:spawn` events, and `EnemySubsystem` creates sprite actors that play animations based on combat state.

---

## Step-by-Step: Adding Sprites to a Monster

### Step 1: Get the 3D Model

1. Create or find a reference image (T-pose, clean background)
2. Run through **Tripo3D** to get a GLB: `tripo_image_to_3d.py`
3. If Mixamo won't accept GLB, convert to FBX via Blender:
   ```bash
   "C:/Blender 5.1/blender.exe" --background --python-expr "
   import bpy
   bpy.ops.wm.read_factory_settings(use_empty=True)
   bpy.ops.import_scene.gltf(filepath='path/to/model.glb')
   bpy.ops.export_scene.fbx(filepath='path/to/model.fbx', use_selection=False, apply_scale_options='FBX_SCALE_ALL', bake_anim=False)
   "
   ```

### Step 2: Rig + Animate in Mixamo

1. Upload FBX to mixamo.com
2. Download animations as FBX: **With Skin, FPS=30, No Reduction**
3. Save to `2D animations/3d_models/animations/{monster_name}/`

**Monster animation set** (subset of player animations — monsters don't need all 17):

| Animation | State | Required? | Notes |
|-----------|-------|-----------|-------|
| Breathing Idle / Idle | idle | YES | Default standing state |
| Walking | walk | YES | Moving state |
| Punching / attack anim | attack | YES | Auto-attack |
| Reaction | hit | YES | Taking damage |
| Dying | death | YES | Death animation |
| Standing 1H Magic Attack 01 | cast_single | Optional | Monster skill casting |
| Standing 1H Cast Spell 01 | cast_self | Optional | Self-buff casting |
| Standing 2H Magic Area Attack 01 | cast_ground | Optional | Ground AoE casting |
| Standing 2H Magic Area Attack 02 | cast_aoe | Optional | AoE casting |

Minimum: 5 animations (idle, walk, attack, hit, death). Add cast animations for monsters with skills.

### Step 3: Render in Blender

```bash
"C:/Blender 5.1/blender.exe" --background --python "C:/Sabri_MMO/2D animations/scripts/blender_sprite_render_v2.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/animations/{monster_name}/Idle.fbx" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{monster_name}" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/{monster_name}/" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/characters/{monster_name}/{monster_name}.glb" \
  --subfolders
```

**CRITICAL**: `--texture-from` MUST point to the exact GLB that was uploaded to Mixamo. If vertex counts differ, textures will be scrambled.

### Step 4: Create V2 Atlas Config

Create `2D animations/atlas_configs/{monster_name}_v2.json`:

```json
{
  "version": 2,
  "character": "{monster_name}",
  "cell_size": 1024,
  "animations": [
    { "folder": "Breathing Idle", "state": "idle", "group": "shared" },
    { "folder": "Walking", "state": "walk", "group": "shared" },
    { "folder": "Punching", "state": "attack", "group": "shared" },
    { "folder": "Reaction", "state": "hit", "group": "shared" },
    { "folder": "Dying", "state": "death", "group": "shared" }
  ]
}
```

**For monsters**: Use `"group": "shared"` for all animations (monsters don't have weapon modes).

### Step 5: Pack Atlases

```bash
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{monster_name}" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/{monster_name}" \
  --config "2D animations/atlas_configs/{monster_name}_v2.json"
```

**IMPORTANT**: Enemy atlases go in `Body/enemies/{monster_name}/`, NOT the root `Body/` folder.

### Step 6: Import into UE5

Import PNGs at `Content/SabriMMO/Sprites/Atlases/Body/enemies/{monster_name}/`. Texture settings:
- Compression: **UserInterface2D**
- Filter: **Nearest**
- Mip Gen Settings: **NoMipmaps**
- Never Stream: **On**

### Step 7: Update Server — Set spriteClass

In `server/src/ro_monster_templates.js`, add/update the monster's `spriteClass` field:

```javascript
{
    name: 'Poring',
    spriteClass: 'poring',  // matches atlas character name
    // ... other fields
}
```

**CRITICAL**: The `ENEMY_TEMPLATES` adapter in `index.js` (~line 4308) must copy the `spriteClass` field. If the adapter doesn't include it, it won't reach the client.

### Step 8: Verify ALL enemy:spawn emit paths

`enemy:spawn` is emitted from **4+ locations**. ALL must include `spriteClass`:

1. **`spawnEnemy()` broadcast** — initial spawn to zone
2. **Respawn broadcast** — dead enemy respawn after timer
3. **`player:join` zone loop** — sends existing enemies to joining player
4. **`zone:ready` zone loop** — sends enemies AFTER client finishes zone transition

**CRITICAL**: The `zone:ready` loop (#4) is the one clients actually receive. During `player:join` (#3), the client is doing `OpenLevel()` — subsystems are destroyed and recreated, so events are silently dropped.

---

## How EnemySubsystem Uses Sprites (C++)

`EnemySubsystem.cpp` already handles sprite actors:

| Event | Animation | Code Location |
|-------|-----------|---------------|
| `enemy:spawn` | Creates `ASpriteCharacterActor`, calls `SetBodyClass(spriteClass)` | ~line 348 |
| Position update | Walk (moving) / Idle (stopped) based on `bIsMoving` | ~line 446 |
| `enemy:death` | Death animation, hides sprite after timer | ~line 481 |
| `enemy:health_update` | Hit animation when health decreases | ~line 554 |
| `enemy:attack` | Attack animation, or cast animation based on `targetType` | ~line 598 |

### Monster skill cast animations

When `enemy:attack` includes a `targetType` field (from monster skill system), the sprite plays the matching cast animation:

- `single_target` → `ESpriteAnimState::CastSingle`
- `self` → `ESpriteAnimState::CastSelf`
- `ground` → `ESpriteAnimState::CastGround`
- `aoe` → `ESpriteAnimState::CastAoe`
- No targetType → `ESpriteAnimState::Attack` (normal auto-attack)

---

## Checklist

- [ ] 3D model (GLB from Tripo3D or other source)
- [ ] Mixamo FBX animations (minimum: idle, walk, attack, hit, death)
- [ ] Blender render (1024px, 8 directions, cel-shaded)
- [ ] V2 atlas config JSON (use `"shared"` group for all monster anims)
- [ ] Atlas packing → `Body/enemies/{name}/`
- [ ] UE5 import with correct texture settings
- [ ] `spriteClass` set in `ro_monster_templates.js`
- [ ] `ENEMY_TEMPLATES` adapter copies `spriteClass`
- [ ] ALL 4 `enemy:spawn` emit paths include `spriteClass`
- [ ] Test: monster spawns with animated sprite in-game

---

## File Reference

| File | Purpose |
|------|---------|
| `2D animations/scripts/blender_sprite_render_v2.py` | Body-only render (enemies without weapons) |
| `2D animations/scripts/render_blend_all_visible.py` | Dual-pass render (enemies with weapon overlays) |
| `2D animations/scripts/pack_atlas.py` | V2 atlas packer |
| `client/.../UI/EnemySubsystem.cpp` | Enemy sprite actor creation + animation control |
| `client/.../Sprite/SpriteCharacterActor.cpp` | Sprite rendering (shared with players) |
| `server/src/ro_monster_templates.js` | Monster data with `spriteClass` field |
| `server/src/index.js` | `ENEMY_TEMPLATES` adapter + 4 `enemy:spawn` emit paths |
