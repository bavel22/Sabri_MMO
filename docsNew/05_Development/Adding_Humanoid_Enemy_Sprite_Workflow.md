# Adding a Humanoid Enemy Sprite — Repeatable Workflow

> **Purpose**: Add a new humanoid enemy with full 8-direction sprite rendering, 16 standard Mixamo animations, and server-side spawn integration. Proven on 40+ enemies during the 2026-04-24/25 session.

**Skills to load when doing this work**:
- `/sabrimmo-3d-to-2d` — render pipeline, atlas configs, render flags
- `/sabrimmo-enemy` — server emit paths, spriteClass field flow
- `/sabrimmo-sprites` — runtime sprite system

---

## Prerequisites

The user provides:
1. **Enemy name** (e.g. "smoking_orc")
2. **Animations folder** at `C:/Sabri_MMO/2D animations/3d_models/animations/enemies/{name}/` containing 16 standard Mixamo FBX files
3. **GLB + FBX model files** at `C:/Sabri_MMO/2D animations/3d_models/enemies/{name}.glb` (and `.fbx`)
4. **Spawn location** in the game world (typically `prt_south, x=-2150, y=Y`)

If the GLB is missing, the render will produce purple/magenta sprites. Ask the user to provide the original Tripo3D output.

---

## Step-by-step workflow

### 1. Verify files exist

```bash
ls "C:/Sabri_MMO/2D animations/3d_models/animations/enemies/{name}/"
ls "C:/Sabri_MMO/2D animations/3d_models/enemies/" | grep -i {name}
```

**Common naming gotchas** (pay attention!):
- GLB filename may differ from animation folder (e.g. anims at `sand_man/`, GLB at `sandman.glb`)
- Some GLBs use a different stem entirely (anims at `bon_gun/`, GLB at `bogun.glb`)
- Pass the actual filename verbatim to `--texture-from`

### 2. Find the template key

```bash
grep -n "{display_name}\|{NAME_UPPER}" server/src/ro_monster_templates.js
```

**Template key pitfalls** — keys often differ from display names:

| Display Name | Template Key |
|-------------|-------------|
| Orc Warrior | `ork_warrior` (with K) |
| Christmas Goblin | `gobline_xmas` (with E) |
| Miyabi Doll | `miyabi_ningyo` |
| Baphomet Jr. | `baphomet_` (trailing underscore) |
| Jing Guai | `li_me_mang_ryang` |
| Bongun | `bogun` (renamed from `bon_gun` to match GLB) |

Always grep for `RO_MONSTER_TEMPLATES['` followed by likely keys.

### 3. Render in Blender (background, ~13-15 min solo)

```bash
"C:/Blender 5.1/blender.exe" --background \
  --python "C:/Sabri_MMO/2D animations/scripts/blender_sprite_render_v2.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/animations/enemies/{name}/Idle.fbx" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{name}" \
  --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --anim-dir "C:/Sabri_MMO/2D animations/3d_models/animations/enemies/{name}/" \
  --texture-from "C:/Sabri_MMO/2D animations/3d_models/enemies/{name}.glb" \
  --subfolders
```

**Optional flags**:
- `--model-rotation 180` — rotate model 180° around Z. Use when the rendered sprite faces the wrong direction (e.g. megalith was facing north when player wanted south). Re-applied per-frame so Mixamo actions don't override it.

**Concurrency cap**: 7-8 parallel Blender instances max. More causes PNG corruption from disk contention.

### 4. Create v2 atlas config

Path: `C:/Sabri_MMO/2D animations/atlas_configs/{name}_v2.json`

Standard 16-animation layout (just change `"character"`):

```json
{
  "version": 2,
  "character": "{name}",
  "cell_size": 1024,
  "animations": [
    { "folder": "Idle", "state": "idle", "group": "unarmed" },
    { "folder": "2hand Idle", "state": "idle", "group": "twohand" },
    { "folder": "Walking", "state": "walk", "group": "unarmed" },
    { "folder": "Run With Sword", "state": "walk", "group": "onehand,twohand" },
    { "folder": "Mutant Punch", "state": "attack", "group": "unarmed" },
    { "folder": "Stable Sword Inward Slash", "state": "attack", "group": "onehand" },
    { "folder": "Great Sword Slash", "state": "attack", "group": "twohand" },
    { "folder": "Standing Draw Arrow", "state": "attack", "group": "bow" },
    { "folder": "Standing 1H Cast Spell 01", "state": "cast_single", "group": "shared" },
    { "folder": "Standing 1H Magic Attack 01", "state": "cast_self", "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 01", "state": "cast_ground", "group": "shared" },
    { "folder": "Standing 2H Magic Area Attack 02", "state": "cast_aoe", "group": "shared" },
    { "folder": "Reaction", "state": "hit", "group": "shared" },
    { "folder": "Dying", "state": "death", "group": "shared" },
    { "folder": "Sword And Shield Block", "state": "block", "group": "onehand" },
    { "folder": "Sitting Idle", "state": "sit", "group": "shared" }
  ]
}
```

**If the monster has fewer animations**, fall back to Idle:

```json
{ "folder": "Idle", "state": "attack", "group": "unarmed" },
{ "folder": "Idle", "state": "hit", "group": "shared" },
{ "folder": "Idle", "state": "death", "group": "shared" }
```

(All entries with the same `folder` will produce the same atlas — pack script de-duplicates by output filename.)

### 5. Pack atlases

```bash
mkdir -p "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}" && \
"C:/ComfyUI/venv/Scripts/python.exe" "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{name}/Idle" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}" \
  --config "C:/Sabri_MMO/2D animations/atlas_configs/{name}_v2.json"
```

**CRITICAL**: input path is `render_output/{name}/Idle/` (NOT `render_output/{name}/`). The `--subfolders` flag plus the base FBX filename ("Idle.fbx" → "Idle") create an extra nesting level.

Output: `Body/enemies/{name}/` directory with **16 PNGs + 16 JSONs + 1 manifest = 33 files, 1072 sprite frames total**.

### 6. Add `spriteClass` to template

In `server/src/ro_monster_templates.js`, add a new line right after `aegisName:`:

```javascript
RO_MONSTER_TEMPLATES['{template_key}'] = {
    id: ..., name: '...', aegisName: '...',
    spriteClass: '{name}', weaponMode: 0,    // ← ADD THIS LINE
    level: ..., maxHealth: ...,
```

**weaponMode mapping**:
| Value | Mode | Use For |
|-------|------|---------|
| 0 | Unarmed | Default, melee enemies (Mutant Punch attack anim) |
| 1 | OneHand | 1H sword (Stable Sword Inward Slash) |
| 2 | TwoHand | 2H sword (Great Sword Slash) |
| 3 | Bow | Ranged enemies (`attackRange >= 300`) — uses Standing Draw Arrow |

**Sibling variants share sprites**: e.g. `kobold_1`, `kobold_2`, `kobold_3` all use `spriteClass: 'kobold'`. Add to ALL siblings.

### 7. Add spawn point

In `server/src/ro_zone_data.js`, add to the `prontera_south.enemySpawns` array:

```javascript
{ template: '{template_key}', x: -2150, y: -{Y}, z: 90.151, wanderRadius: 50 },
```

Use the actual TEMPLATE KEY (not spriteClass). E.g. `'baphomet_'` for Baphomet Jr., `'li_me_mang_ryang'` for Jing Guai.

### 8. Add to import script

Edit `client/SabriMMO/Scripts/Environment/import_enemy_batch_{N}.py` (or the master `import_enemy_batch.py`) — add `"{name}"` to the `FOLDERS` list.

### 9. Import into UE5

In UE5 Python console (open via Tools → Python Console):

```
py "C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/import_enemy_batch_{N}.py"
```

This imports new PNGs and applies sprite settings (BC7, TEXTUREGROUP_UI, TF_NEAREST, NoMipmaps, never_stream). It SKIPS already-imported assets.

**For RE-import after re-render** (e.g. after changing rotation or fixing animations): use a force-reimport script that DELETES existing `.uasset` files first. UE5 caches imported textures aggressively, and `replace_existing=True` alone often doesn't update the data. Template: `client/SabriMMO/Scripts/Environment/reimport_requiem_megalith.py` — copy and edit `FOLDERS` to target other enemies.

### 10. Restart server

```
C:/Sabri_MMO/server/start-server.bat
```

The new enemy will spawn at the configured location with the sprite. No client recompile needed.

---

## Server emit paths — ALL 5 must include sprite fields

When debugging "enemy spawns wrong / shows default mesh", check ALL 5 `enemy:spawn` emit paths in `server/src/index.js`:

| # | Location | Lines | Triggers |
|---|----------|-------|----------|
| 1 | Combat-death respawn | ~28578 | **Player kills enemy → respawn** |
| 2 | Status-event respawn | ~2816 | Enemy dies via status (poison, etc.) |
| 3 | `spawnEnemy()` broadcast | ~4813 | Initial spawn at server startup |
| 4 | `player:join` zone loop | ~6628 | New player joins (DROPPED during OpenLevel) |
| 5 | `zone:ready` zone loop | ~7212 | After zone transition (THE ONE that matters) |

**Also**: `ENEMY_TEMPLATES` adapter (~line 4590) must copy `spriteClass`/`weaponMode`/`spriteTint`/`spriteScale` from the template.

The combat-death respawn (path #1) was the most-commonly-missed path historically. Symptom: "enemy works initially, but after player kills it and it respawns, it shows the default capsule mesh." Fixed 2026-04-25.

---

## Common issues & fixes

| Issue | Cause | Fix |
|-------|-------|-----|
| Sprite renders purple/magenta | GLB missing or wrong vertex count | Verify GLB is the EXACT one uploaded to Mixamo. If missing, ask user to provide. |
| Pack produces only 4 atlases | Config has all entries pointing to "Idle" folder (Idle-fallback) | Use standard 16-anim config with unique folder names per state |
| Render produces 6 anim folders not 16 | Render started before all FBX files were synced | Re-run render — animations folder now has all 16 FBX |
| Sprite faces wrong direction | Default model orientation in FBX | Add `--model-rotation 180` (or 90/-90 as needed) |
| Megalith faces north instead of south | Rotation only applied once before render loop, not per-frame | Updated `blender_sprite_render_v2.py` line ~907 to re-apply rotation per-frame |
| Enemy works initially but shows default mesh on respawn | Combat-death respawn payload missing sprite fields | Add `spriteClass`/`weaponMode` to combat-death emit (`index.js` ~line 28578) |
| UE5 doesn't pick up re-rendered textures | UE5 caches imported texture data | Use force-reimport script that deletes uassets first |
| `[WARN] No sprites in '{anim}/' — skipping` during pack | Stale config with old anim names | Rewrite config to standard 16-anim layout |
| Goblin Archer plays Mutant Punch instead of Draw Arrow | weaponMode is 0 (unarmed) | Set `weaponMode: 3` for ranged enemies (`attackRange >= 300`) |

---

## Reference files

- **Render script**: `2D animations/scripts/blender_sprite_render_v2.py`
- **Pack script**: `2D animations/scripts/pack_atlas.py`
- **Standard atlas config**: `2D animations/atlas_configs/standard_template_v2.json`
- **Server template registry**: `server/src/ro_monster_templates.js`
- **Server zone spawn config**: `server/src/ro_zone_data.js`
- **Server emit paths**: `server/src/index.js` (lines 2816, 4813, 6628, 7212, 28578)
- **UE5 import scripts**: `client/SabriMMO/Scripts/Environment/import_enemy_batch_*.py`
- **Force-reimport template**: `client/SabriMMO/Scripts/Environment/reimport_requiem_megalith.py`
- **Atlas output**: `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}/`
