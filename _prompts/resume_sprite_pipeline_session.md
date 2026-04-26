# Resume Enemy Sprite Pipeline Session

## Start with these commands

```
/sabrimmo-enemy
/sabrimmo-3d-to-2d
/sabrimmo-sprites
```

## Read these memories

```
memory/sprite-pipeline-v2.md     # current state of the sprite system (primary)
memory/enemy-sprite-system.md    # original sprite pipeline docs
memory/monster-shape-key-system.md   # render_monster.py 12 presets
memory/feedback-thin-geometry-rotation.md
memory/feedback-attack-lunge-ortho.md
memory/feedback-render-pipeline-standard.md
```

## Read these reference files

```
_prompts/enemy_sprite_session_resume.md   # master enemy table (150 done / 104 GLB ready / 255 no-GLB)
```

## Current state at end of last session

**150 / 509 enemies have sprites.** 9 batches completed:
- Stationary (plants/mushrooms/eggs)
- Batch 1 (eclipse, thief_bug, dragon_fly, desert_wolf_b, plankton, thief_bug_f, toad)
- Antonio + Spore fills
- Batch 3 (kukre..shellfish)
- Batch 4 (boiled_rice..muka)
- Batch 5 (ghostring..vadon)
- Batch 6 (yoyo..eggyra, shifted -4000 X)
- Batch 7 (wolf..frilldora)
- Batch 8 (karakasa..whisper)
- Batch 9 (noxious..quve)

Each batch is spawned in prontera_south with Y shifting in +/-2000 increments.

Spawn layout (Y/X coords in prontera_south `enemySpawns`):
- Default spawn area: various beginner enemies
- Y=-3000: stationary plants/mushrooms
- Y=-5000: batch 9 (high-level)
- Y=-7000: batch 3 + batch 8 (X=-4800)
- Y=-9000: batch 4 + batch 7 (X=-4800)
- Y=-11000: batch 5 + batch 6 (X=-4800)

## Key systems you set up

1. **Size-based scaling** (`small`=0.5x, `medium`=1.0x, `large`=1.5x) in `EnemySubsystem::HandleEnemySpawn`
2. **spriteScale override** — optional per-template field bypasses size (e.g. whisper_boss: 0.75 = 1.5x of small whisper)
3. **canMove flag** — server sends, client gates attack lunge so stationary enemies thrash in place
4. **--model-z-offset** flag in render_monster.py for flying creatures (bats, ghosts, floating blobs)
5. **Walk+attack state fallback** in manifests for plant/egg preset (idle animation reused)
6. **Variant sprite inheritance** — `meta_X`, `provoke_X`, same-name `X_` auto-share base spriteClass (propagated in server templates)
7. **HP bar at top of sprite** — repositioned for sprite enemies so large sprites don't cover it
8. **Navmesh dead-zone fix** — `enemyMoveToward` final waypoint no longer skipped at 20u (fixed stuck-at-12u wander bug)
9. **Lighting defaults updated** — render_monster.py and blender_sprite_render_v2.py now default `--cel-shadow 0.92 --cel-mid 0.98`

## Helper scripts

In `C:/Sabri_MMO/_prompts/`:
- `build_enemy_list.js` + `build_enemy_markdown.js` — regenerate master table
- Run both then replace everything after line 180 of `enemy_sprite_session_resume.md` with `.enemy_table.md`

In `C:/Sabri_MMO/client/SabriMMO/Scripts/Environment/`:
- `import_enemy_sprites.py` — generic (scan all `Body/enemies/`)
- `import_batchN.py` (N=3..9) — per-batch importers with BC7, UI group, NoMipmaps, Nearest, Never Stream
- `reimport_hermit_plant.py` — template for force-reimport (replaces existing uassets)

## Pipeline cheatsheet

**Render command template:**
```bash
"C:/Blender 5.1/blender.exe" --background --python "C:/Sabri_MMO/2D animations/scripts/render_monster.py" -- \
  "C:/Sabri_MMO/2D animations/3d_models/enemies/{name}.glb" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{name}" \
  --monster-type {preset} --render-size 1024 --camera-angle 10 --camera-target-z 0.7 \
  --model-rotation {rot}
  # Optional flags:
  # --model-z-offset 0.5   (for flying creatures)
  # --cel-shadow 0.92 --cel-mid 0.98 (already default, just documenting)
```

**Rotation quick reference:**
- Standard moving enemies: `--model-rotation -90`
- Stationary enemies (egg/plant preset): `--model-rotation 90`
- Thin geometry (bats/flying insects/cookies): `--model-rotation -75` (15° offset avoids edge-on)
- If sprite faces wrong direction: rotate by ±90° increments until correct

**Parallel render cap: 7-8 Blender instances max** (15 caused PNG corruption last time)

**Pack command template:**
```bash
"C:/Users/pladr/AppData/Local/Programs/Python/Python312/python.exe" \
  "C:/Sabri_MMO/2D animations/scripts/pack_atlas.py" \
  "C:/Sabri_MMO/2D animations/sprites/render_output/{name}" \
  "C:/Sabri_MMO/client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/{name}" \
  --config "C:/Sabri_MMO/2D animations/atlas_configs/{name}_v2.json"
```

## Standard atlas configs

**Blob preset:**
```json
{
  "animations": [
    { "folder": "Idle Bounce", "state": "idle", "group": "shared" },
    { "folder": "Hop Forward", "state": "walk", "group": "shared" },
    { "folder": "Lunge Attack", "state": "attack", "group": "shared" },
    { "folder": "Hit Squish", "state": "hit", "group": "shared" },
    { "folder": "Flatten Death", "state": "death", "group": "shared" }
  ]
}
```

**Animation folder names per preset:**
- blob: Idle Bounce / Hop Forward / Lunge Attack / Hit Squish / Flatten Death
- caterpillar: Idle Wiggle / Crawl Forward / Bite Attack / Hit Recoil / Curl Death
- rabbit: Idle Twitch / Bunny Hop / Headbutt Attack / Hit Flinch / Topple Death
- egg: Idle Wobble / Hit Jiggle / Crack Death (no walk/attack — use idle_wobble fallback)
- frog: Idle Croak / Frog Leap / Tongue Snap / Hit Flinch / Belly Up Death
- tree: Idle Sway / Root Waddle / Branch Swipe / Trunk Shudder / Timber Death
- bird: (same walk/attack — don't recall exactly, check render_monster.py)
- flying_insect: Hover Buzz / Dart Forward / Sting Attack / Swat Flinch / Swatted Death
- bat: Wing Beat Hover / Swoop Flight / Dive Bite / Wing Recoil / Plummet Death
- quadruped: Idle Sniff / Trot Forward / Charge Attack / Stumble Flinch / Collapse Death
- plant: Idle Sway / Tentacle Thrash / Recoil Flinch / Wilt Death (no walk — use idle_sway fallback)
- biped_insect: Idle Fidget / Skitter Walk / Claw Swipe / Flinch Reel / Crumple Death

## Typical batch workflow

For each new batch:
1. Verify GLBs exist, identify presets + rotations
2. Create `{name}_v2.json` atlas configs (use template above)
3. Kick off 7-8 parallel renders (wait for first wave before second)
4. Pack all atlases
5. Inject `spriteClass` into server templates via a one-off node script (template pattern in `_prompts/.add_batch*.js` history)
6. For meta_/provoke_ variants: propagate base's `spriteClass`
7. Add spawns to `ro_zone_data.js` prontera_south enemySpawns array
8. Create `import_batch{N}.py` in `Scripts/Environment/` (copy template from batch 9)
9. User restarts server + runs import script in UE5 editor

## Pending items (from session end)

- **whisper_boss 1.5× scale** — server-side `spriteScale: 0.75` added. **Need C++ rebuild to take effect** (and editor has been hanging on close — may need force-kill via `taskkill //F //PID`)
- **savage HP bar fix** — C++ change to position bar at top of sprite for sprite enemies. Same C++ rebuild.
- **drainliar test spawn** added at (1200, 0) near default spawn for easy finding
- **flora + marine_sphere 180° rotation** — re-rendered, uassets deleted, user must re-run `import_batch7.py`
- **swordfish rotation 90** — re-rendered, uassets deleted, user must re-run `import_batch7.py`

## What the user might ask next

- "implement the next set of enemies" — same pipeline, user will paste a list + spawn offset. Use the typical batch workflow above.
- "fix {enemy} rotation/scale/position" — targeted re-render, delete uassets, user reimports.
- "update master enemy table" — re-run build_enemy_list.js + build_enemy_markdown.js.
- "add new special behavior" — likely means new server flag, all 5 emit paths + client parse + store + use.

## Critical gotchas

1. **All 5 enemy:spawn emit paths + ENEMY_TEMPLATES adapter** must be updated when adding server fields (spriteClass, weaponMode, size, spriteTint, spriteScale, canMove)
2. **UE5 editor frequently hangs on close** — force-kill with `taskkill //F //PID <pid>` if build fails with "Live Coding is active"
3. **Re-rendered sprites need uasset cleanup** before reimporting (`rm -f .../enemies/{name}/*.uasset` before running import script)
4. **BC7 import takes minutes** — user shouldn't play during encoding (causes crash from texture compile manager race)
5. **Render ≤8 in parallel** (15 caused PNG corruption last time from resource contention)

---

Pick up where we left off. The user will give you the next set of enemies or a fix request. If they ask about the current state, reference `memory/sprite-pipeline-v2.md` and `_prompts/enemy_sprite_session_resume.md` (master table).
