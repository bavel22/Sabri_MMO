# Enemy Sprite Implementation Session

## Context
We've built a complete non-humanoid enemy sprite pipeline using shape key animations in `render_monster.py`. This session continues implementing sprites for more enemies.

## Load These Skills First
```
/sabrimmo-enemy
/sabrimmo-3d-to-2d  
/sabrimmo-sprites
```

## Read These Memory Files
```
memory/monster-shape-key-system.md
memory/enemy-sprite-system.md
memory/feedback-attack-lunge-ortho.md
memory/feedback-thin-geometry-rotation.md
```

## FIRST TASK: Implement Sprite Tint System + Eclipse

Before implementing any new sprites, implement a **runtime sprite tinting system** so recolored enemies can share atlas files. Then use it to create Eclipse (dark Lunatic) as a proof of concept.

### Implementation Steps:

1. **Server template**: Add optional `spriteTint` field to monster templates. Format: `spriteTint: [r, g, b]` where values are 0-1 multipliers. Example for Eclipse: `spriteTint: [0.3, 0.3, 0.45]`

2. **ENEMY_TEMPLATES adapter** (~line 4590 in index.js): Copy `spriteTint` from RO template to the adapter output.

3. **All 4 enemy:spawn emit paths**: Include `spriteTint` in the payload (spawnEnemy broadcast, respawnEnemy, player:join loop, zone:ready loop).

4. **FEnemyEntry struct** (EnemySubsystem.h): Add `FLinearColor SpriteTint = FLinearColor::White;`

5. **HandleEnemySpawn** (EnemySubsystem.cpp): Parse `spriteTint` array from JSON, store in entry, and call `SpriteActor->SetBodyTint(Color)` or apply via `SetVectorParameterValue("TintColor", Color)` on the body layer material.

6. **SpriteCharacterActor**: May need a `SetBodyTint(FLinearColor)` method that sets the tint on the Body layer's `UMaterialInstanceDynamic`. Check if the material already has a TintColor parameter (hair system uses this). If not, add one to `CreateSpriteMaterial()`.

7. **Test with Eclipse**: Set Eclipse template to `spriteClass: 'lunatic', spriteTint: [0.3, 0.3, 0.45]`. Eclipse should appear as a dark-tinted Lunatic using the exact same atlas files.

### Recolor Candidates (share atlas, just tint differently)
Once the tint system works, these enemies can reuse existing sprites:
- Eclipse → tinted Lunatic
- Thara Frog → tinted Roda Frog  
- Steel Chonchon → metallic Chonchon
- Elder Willow → darker Willow
- Andre/Deniro/Piere/Vitata → tinted ant (one base ant model)
- Vagabond Wolf → tinted Wolf
- Desert Wolf / Baby Desert Wolf → tinted Wolf variants
- Drainliar → tinted Familiar (bat)
- Marina → tinted Marin (blob)

## What's Already Built

### render_monster.py — 12 Animation Presets
Located at `2D animations/scripts/render_monster.py`. Uses 6 shape key types:
- **Deformation**: Squash, Stretch, Flatten (scale from z_min anchor)
- **Translation**: HopUp (vertical offset)
- **Wing-targeted**: WingUp/WingDown (only affects wing-region vertices)
- **Diagonal tilt**: TiltOver (X+Y diagonal for death falls)
- **Forward lean**: LeanForward (upper body tilts, feet planted — bipeds)
- **Arm reach**: ArmReach (arm-region vertex extension)

| Preset        | Best For           | Example Enemies          |
| ------------- | ------------------ | ------------------------ |
| blob          | Round amorphous    | Poring, Drops, Poporing  |
| caterpillar   | Worms/larvae       | Fabre                    |
| rabbit        | Small hoppers      | Lunatic, Smokie          |
| egg           | Stationary cocoons | Pupa, Thief Bug Egg      |
| frog          | Amphibians         | Roda Frog                |
| tree          | Walking plants     | Willow, Poison Spore     |
| bird          | Ground birds       | Condor, Peco Peco        |
| flying_insect | Airborne bugs      | Hornet, Creamy, Chonchon |
| bat           | Flying mammals     | Familiar                 |
| quadruped     | 4-legged animals   | Savage Babe, Yoyo        |
| plant         | Stationary plants  | Mandragora               |
| biped_insect  | Upright insects    | Rocker                   |

### 30 Monsters with Sprites Complete

Unique rendered atlases in `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/`. Every one also covers its `meta_` / `provoke_` / same-name `_` variants via inheritance (6 extra templates auto-inherit), for **36 total template entries served** by these 30 atlases.

**Lv 1-10 (21):** Poring, Fabre, Pupa, Lunatic, Drops, Willow, Chonchon, Condor, Roda Frog, Thief Bug, Eclipse, Savage Babe, Hornet, Familiar, Dragon Fly, Rocker, Baby Desert Wolf, Thief Bug Female, Skeleton, Toad, Plankton

**Lv 11-20 (8):** Mandragora, Poporing, Zombie, Spore, Creamy, Smokie, Peco Peco, Poison Spore

**Lv 21+ (1):** Yoyo

**Variant inheritors (6 — no extra work, just set `spriteClass`):** `fabre_`, `meta_fabre`, `meta_pupa`, `chonchon_`, `meta_creamy`, `provoke_yoyo`

### C++ Attack System (EnemySubsystem.cpp)
- **Melee lunge**: 3-phase wind-up (20u back, 0.08s) → snap forward (60u) → return (0.15s). Clamped by MinClearance=40u, MeleeThreshold=250u.
- **Ranged attack**: Spawns a temporary sprite actor at player's feet (billboard, auto-destroys). Currently uses attacker's own sprite class.
- **Key rule**: Ortho camera makes render-side XY translation invisible. All movement effects must be client-side C++.

### Pipeline for Each New Monster
1. Place GLB in `2D animations/3d_models/enemies/`
2. Choose preset type (or create new one in render_monster.py)
3. Create atlas config: `2D animations/atlas_configs/{name}_v2.json`
4. Add `spriteClass: '{name}', weaponMode: 0` to server template in `ro_monster_templates.js`
5. Render: `blender.exe --background --python render_monster.py -- {model}.glb {output} --monster-type {preset} --render-size 1024 --camera-angle 10 --camera-target-z 0.7 --model-rotation -90`
6. Pack: `pack_atlas.py {render_output} {atlas_dir} --config {config}.json`
7. Move files to `Body/enemies/{name}/` subfolder
8. Delete old .uasset files, import PNGs in UE5 (UserInterface2D, Nearest, NoMipmaps, Never Stream)

### Important Rules
- Standard enemies: `--model-rotation -90`
- Stationary enemies (egg/plant): `--model-rotation 90` (face south)
- Thin geometry (wings/fins): offset rotation by 15° (e.g., `-75` instead of `-90`)
- `--thicken` flag exists but DISTORTS models — use rotation offset instead
- CELL_SIZE = 50 UE units. MELEE_RANGE = 150. Ranged = attackRange > 200.

## Enemies Still Needing Sprites (Level 1-30)

### Priority Unique Enemies (need GLB models from Tripo3D)

| Level | Key                       | Name             | Suggested Preset                       |     |
| ----- | ------------------------- | ---------------- | -------------------------------------- | --- |
| 6     | eclipse                   | Eclipse          | rabbit (large lunatic variant)         |     |
| 6     | thief_bug                 | Thief Bug        | caterpillar or biped_insect            |     |
| 8     | dragon_fly                | Dragon Fly       | flying_insect                          |     |
| 9     | desert_wolf_b             | Baby Desert Wolf | quadruped                              |     |
| 10    | plankton                  | Plankton         | blob                                   |     |
| 10    | thief_bug_                | Thief Bug Female | biped_insect                           |     |
| 10    | toad                      | Toad             | frog                                   |     |
| 11    | kukre                     | Kukre            | frog or fish                           |     |
| 11    | tarou                     | Tarou            | rabbit (rat)                           |     |
| 13    | ambernite                 | Ambernite        | egg (snail)                            |     |
| 14    | hydra                     | Hydra            | plant (ranged, attackRange 350)        |     |
| 14    | worm_tail                 | Wormtail         | caterpillar                            |     |
| 15    | snake                     | Boa              | caterpillar or new snake preset        |     |
| 15    | marin                     | Marin            | blob                                   |     |
| 15    | shellfish                 | Shellfish        | egg                                    |     |
| 16    | spore                     | Spore            | tree or blob                           |     |
| 16    | stainer                   | Stainer          | flying_insect                          |     |
| 17    | andre                     | Andre            | biped_insect (ant)                     |     |
| 17    | coco                      | Coco             | rabbit                                 |     |
| 17    | muka                      | Muka             | plant (cactus)                         |     |
| 17    | steel_chonchon            | Steel Chonchon   | flying_insect                          |     |
| 18    | horn                      | Horn             | biped_insect                           |     |
| 18    | martin                    | Martin           | rabbit                                 |     |
| 18    | piere                     | Piere            | biped_insect (ant)                     |     |
| 18    | vocal                     | Vocal            | biped_insect (cricket, like Rocker)    |     |
| 19    | deniro                    | Deniro           | biped_insect (ant)                     |     |
| 19    | vadon                     | Vadon            | frog or crab                           |     |
| 20    | elder_wilow               | Elder Willow     | tree                                   |     |
| 20    | vitata                    | Vitata           | biped_insect (ant)                     |     |
| 21    | dustiness                 | Dustiness        | flying_insect                          |     |
| 21    | marina                    | Marina           | blob                                   |     |
| 21    | raggler                   | Raggler          | quadruped                              |     |
| 22    | goblin_5                  | Goblin           | biped_insect or new humanoid           |     |
| 22    | metaller                  | Metaller         | biped_insect                           |     |
| 22    | thara_frog                | Thara Frog       | frog                                   |     |
| 23    | anacondaq                 | Anacondaq        | caterpillar (snake)                    |     |
| 23    | caramel                   | Caramel          | quadruped                              |     |
| 23    | cornutus                  | Cornutus         | egg (hermit crab)                      |     |
| 23    | zerom                     | Zerom            | biped_insect or humanoid               |     |
| 24    | drainliar                 | Drainliar        | bat                                    |     |
| 24    | eggyra                    | Eggyra           | blob or egg                            |     |
| 24    | orc_warrior / ork_warrior | Orc Warrior      | biped_insect or new humanoid           |     |
| 24    | orc_zombie                | Orc Zombie       | biped_insect                           |     |
| 24    | scorpion                  | Scorpion         | biped_insect or new arachnid           |     |
| 24    | vagabond_wolf             | Vagabond Wolf    | quadruped                              |     |
| 25    | argos                     | Argos            | biped_insect (spider)                  |     |
| 25    | bigfoot                   | Bigfoot          | quadruped                              |     |
| 25    | golem                     | Golem            | tree (stone variant)                   |     |
| 25    | wolf                      | Wolf             | quadruped                              |     |
| 26    | flora                     | Flora            | plant (ranged like Mandragora)         |     |
| 26    | mantis                    | Mantis           | biped_insect                           |     |
| 26    | savage                    | Savage           | quadruped                              |     |
| 26    | hode                      | Hode             | caterpillar                            |     |
| 27    | desert_wolf               | Desert Wolf      | quadruped                              |     |
| 28    | goblin_archer             | Goblin Archer    | biped_insect (ranged, attackRange 450) |     |
| 28    | orc_skeleton              | Orc Skeleton     | biped_insect                           |     |
| 29    | soldier_skeleton          | Soldier Skeleton | biped_insect                           |     |
| 30    | munak                     | Munak            | biped_insect or humanoid               |     |
| 30    | sasquatch                 | Sasquatch        | quadruped                              |     |
| 30    | skel_worker               | Skeleton Worker  | biped_insect                           |     |

### Egg/Cocoon Variants (use egg preset, low priority)


---

## MASTER ENEMY LIST (Generated 2026-04-18)

Cross-referenced with `server/src/ro_monster_templates.js` (509 templates), `2D animations/3d_models/enemies/` (GLBs), and `client/SabriMMO/Content/SabriMMO/Sprites/Atlases/Body/enemies/` (atlases).

### Summary
- **150** enemies rendered or inheriting a rendered sprite (`done`)
- **104** enemies with GLB ready or inheriting a GLB, sprite pending (`pending`)
- **255** enemies with no GLB yet (`no GLB`)
- 1 of these inherit sprite from a base variant (`shares X` in the Sprite column)
- **509** total monster templates

### Sprite Column Legend
- `done` — atlas exists in `Body/enemies/{name}/`
- `done (shares X)` — auto-inherits X's atlas via variant rule (meta_X / provoke_X / same-name X_ suffix). NO extra work needed; just set `spriteClass: 'X'` in the template
- `done (tinted)` — uses another monster's atlas with `spriteTint` recolor
- `pending (GLB ready)` — GLB file exists; just needs render + pack
- `pending (shares X GLB)` — no own GLB, but base variant has one; when X is rendered, this entry becomes done automatically
- `no GLB` — needs Tripo3D model first

### Variant-Inheritance Rules
The matcher auto-detects these sprite-sharing patterns:
- `meta_X`, `provoke_X` — always share X's sprite (metamorphosis/provoked forms are visually identical)
- `X_` (single trailing `_`) — shares X's sprite ONLY when the display name is identical (e.g. `fabre_` = "Fabre" shares `fabre`; `poring_` = "Santa Poring" does NOT share `poring`)
- Chains cascade: `meta_picky_` → `picky_` → `picky` (stops at the first ancestor with a sprite)

### Preset Column
Suggested `render_monster.py` preset. `?` means unknown — pick based on the model when you have it. Available presets: blob, caterpillar, rabbit, egg, frog, tree, bird, flying_insect, bat, quadruped, plant, biped_insect.

### Class Tags
**BOSS** = boss-protocol (knockback/status immune). **MVP** = MVP fanfare on death.

### Level 1-10 (50 total — 50 done, 0 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|   1 | 1002 | `poring` | Poring | plant | water1 | medium | blob | done |
|   1 | 1078 | `red_plant` | Red Plant | plant | earth1 | small | tree | done |
|   1 | 1079 | `blue_plant` | Blue Plant | plant | earth1 | small | tree | done |
|   1 | 1080 | `green_plant` | Green Plant | plant | earth1 | small | tree | done |
|   1 | 1081 | `yellow_plant` | Yellow Plant | plant | earth1 | small | tree | done |
|   1 | 1082 | `white_plant` | White Plant | plant | earth1 | small | tree | done |
|   1 | 1083 | `shining_plant` | Shining Plant | plant | holy1 | small | tree | done |
|   1 | 1084 | `black_mushroom` | Black Mushroom | plant | earth1 | small | tree | done |
|   1 | 1085 | `red_mushroom` | Red Mushroom | plant | earth1 | small | tree | done |
|   1 | 1182 | `thief_mushroom` | Thief Mushroom | plant | earth1 | small | tree | done |
|   1 | 1184 | `fabre_` | Fabre | insect | earth1 | small | biped_insect | done |
|   1 | 1395 | `crystal_1` | Wind Crystal **BOSS** | formless | neutral1 | small | ? | done |
|   1 | 1396 | `crystal_2` | Earth Crystal **BOSS** | formless | neutral1 | small | ? | done |
|   1 | 1397 | `crystal_3` | Fire Crystal **BOSS** | formless | neutral1 | small | ? | done |
|   1 | 1398 | `crystal_4` | Water Crystal **BOSS** | formless | neutral1 | small | ? | done |
|   2 | 1007 | `fabre` | Fabre | insect | earth1 | small | caterpillar | done |
|   2 | 1008 | `pupa` | Pupa | insect | earth1 | small | egg | done |
|   2 | 1090 | `mastering` | Mastering **BOSS** | plant | water1 | medium | tree | done |
|   2 | 1229 | `meta_fabre` | Fabre | insect | earth1 | small | biped_insect | done |
|   2 | 1230 | `meta_pupa` | Pupa | insect | earth1 | small | biped_insect | done |
|   3 | 1047 | `pecopeco_egg` | Peco Peco Egg | formless | neutral3 | small | egg | done |
|   3 | 1049 | `picky` | Picky | brute | fire1 | small | egg | done |
|   3 | 1062 | `poring_` | Santa Poring | plant | holy1 | medium | blob | done |
|   3 | 1063 | `lunatic` | Lunatic | brute | neutral3 | small | rabbit | done |
|   3 | 1113 | `drops` | Drops | plant | fire1 | medium | blob | done |
|   3 | 1232 | `meta_pecopeco_egg` | Peco Peco Egg | formless | neutral3 | small | egg | done |
|   3 | 1240 | `meta_picky` | Picky | brute | fire1 | small | quadruped | done |
|   4 | 1010 | `wilow` | Willow | plant | earth1 | medium | tree | done |
|   4 | 1011 | `chonchon` | Chonchon | insect | wind1 | small | flying_insect | done |
|   4 | 1048 | `thief_bug_egg` | Thief Bug Egg | insect | shadow1 | small | egg | done |
|   4 | 1050 | `picky_` | Picky | brute | fire1 | small | egg | done |
|   4 | 1097 | `ant_egg` | Ant Egg | formless | neutral3 | small | egg | done |
|   4 | 1183 | `chonchon_` | Chonchon | insect | wind1 | small | biped_insect | done |
|   4 | 1236 | `meta_ant_egg` | Ant Egg | formless | neutral3 | small | egg | done |
|   4 | 1241 | `meta_picky_` | Picky | brute | fire1 | small | quadruped | done |
|   5 | 1009 | `condor` | Condor | brute | wind1 | medium | bird | done |
|   5 | 1012 | `roda_frog` | Roda Frog | fish | water1 | medium | frog | done |
|   6 | 1051 | `thief_bug` | Thief Bug | insect | neutral3 | small | biped_insect | done |
|   6 | 1093 | `eclipse` | Eclipse **BOSS** | brute | neutral3 | medium | rabbit | done |
|   7 | 1167 | `savage_babe` | Savage Babe | brute | earth1 | small | quadruped | done |
|   8 | 1004 | `hornet` | Hornet | insect | wind1 | small | flying_insect | done |
|   8 | 1005 | `farmiliar` | Familiar | brute | shadow1 | small | bat | done |
|   8 | 1091 | `dragon_fly` | Dragon Fly **BOSS** | insect | wind1 | small | flying_insect | done |
|   9 | 1052 | `rocker` | Rocker | insect | earth1 | medium | biped_insect | done |
|   9 | 1107 | `desert_wolf_b` | Baby Desert Wolf | brute | fire1 | small | quadruped | done |
|  10 | 1053 | `thief_bug_` | Thief Bug Female | insect | shadow1 | medium | biped_insect | done |
|  10 | 1076 | `skeleton` | Skeleton | undead | undead1 | medium | biped_insect | done |
|  10 | 1089 | `toad` | Toad **BOSS** | fish | water1 | medium | frog | done |
|  10 | 1161 | `plankton` | Plankton | plant | water3 | small | blob | done |
|  10 | 1247 | `antonio` | Antonio | plant | holy3 | medium | tree | done |

### Level 11-20 (41 total — 39 done, 2 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  11 | 1070 | `kukre` | Kukre | fish | water1 | small | frog | done |
|  11 | 1175 | `tarou` | Tarou | brute | shadow1 | small | rabbit | done |
|  12 | 1020 | `mandragora` | Mandragora | plant | earth3 | medium | plant | done |
|  13 | 1094 | `ambernite` | Ambernite | insect | water1 | large | egg | done |
|  14 | 1024 | `worm_tail` | Wormtail | plant | earth1 | medium | caterpillar | done |
|  14 | 1031 | `poporing` | Poporing | plant | poison1 | medium | blob | done |
|  14 | 1068 | `hydra` | Hydra | plant | water2 | small | plant | done |
|  15 | 1015 | `zombie` | Zombie | undead | undead1 | medium | biped_insect | done |
|  15 | 1025 | `snake` | Boa | brute | earth1 | medium | caterpillar | done |
|  15 | 1074 | `shellfish` | Shellfish | fish | water1 | small | egg | done |
|  15 | 1242 | `marin` | Marin | plant | water2 | medium | blob | pending (GLB ready) |
|  15 | 1520 | `boiled_rice` | Boiled Rice | plant | water1 | medium | tree | done |
|  16 | 1014 | `spore` | Spore | plant | water1 | medium | tree | done |
|  16 | 1018 | `creamy` | Creamy | insect | wind1 | small | flying_insect | done |
|  16 | 1174 | `stainer` | Stainer | insect | wind1 | small | flying_insect | done |
|  16 | 1231 | `meta_creamy` | Creamy | insect | wind1 | small | biped_insect | done |
|  17 | 1042 | `steel_chonchon` | Steel Chonchon | insect | wind1 | small | flying_insect | done |
|  17 | 1055 | `muka` | Muka | plant | earth1 | large | plant | done |
|  17 | 1095 | `andre` | Andre | insect | earth1 | small | biped_insect | done |
|  17 | 1104 | `coco` | Coco | brute | earth1 | small | rabbit | done |
|  17 | 1162 | `rafflesia` | Rafflesia | plant | earth1 | small | tree | done |
|  17 | 1237 | `meta_andre` | Andre | insect | earth1 | small | biped_insect | done |
|  18 | 1056 | `smokie` | Smokie | brute | earth1 | small | rabbit | done |
|  18 | 1088 | `vocal` | Vocal | insect | earth1 | medium | biped_insect | pending (GLB ready) |
|  18 | 1120 | `ghostring` | Ghostring **BOSS** | demon | ghost4 | medium | ? | done |
|  18 | 1128 | `horn` | Horn | insect | earth1 | medium | biped_insect | done |
|  18 | 1145 | `martin` | Martin | brute | earth2 | small | rabbit | done |
|  18 | 1160 | `piere` | Piere | insect | earth1 | small | biped_insect | done |
|  18 | 1238 | `meta_piere` | Piere | insect | earth1 | small | biped_insect | done |
|  18 | 1266 | `aster` | Aster | fish | earth1 | small | frog | done |
|  19 | 1019 | `pecopeco` | Peco Peco | brute | fire1 | large | bird | done |
|  19 | 1054 | `thief_bug__` | Thief Bug Male | insect | shadow1 | medium | biped_insect | done |
|  19 | 1066 | `vadon` | Vadon | fish | water1 | small | frog | done |
|  19 | 1077 | `poison_spore` | Poison Spore | plant | poison1 | medium | tree | done |
|  19 | 1105 | `deniro` | Deniro | insect | earth1 | small | biped_insect | done |
|  19 | 1234 | `provoke_yoyo` | Yoyo | brute | earth1 | small | quadruped | done |
|  19 | 1239 | `meta_deniro` | Deniro | insect | earth1 | small | biped_insect | done |
|  20 | 1033 | `elder_wilow` | Elder Willow | plant | fire2 | medium | tree | done |
|  20 | 1073 | `crab` | Crab | fish | water1 | small | frog | done |
|  20 | 1096 | `angeling` | Angeling **BOSS** | angel | holy4 | medium | ? | done |
|  20 | 1176 | `vitata` | Vitata | insect | earth1 | small | biped_insect | done |

### Level 21-30 (59 total — 35 done, 24 GLB ready)

| Lv  | ID   | Key                | Name                   | Race      | Element  | Size   | Preset        | Sprite              |
| --- | ---- | ------------------ | ---------------------- | --------- | -------- | ------ | ------------- | ------------------- |
| 21  | 1057 | `yoyo`             | Yoyo                   | brute     | earth1   | small  | quadruped     | done                |
| 21  | 1114 | `dustiness`        | Dustiness              | insect    | wind2    | small  | flying_insect | done                |
| 21  | 1141 | `marina`           | Marina                 | plant     | water2   | small  | blob          | done                |
| 21  | 1254 | `raggler`          | Raggler                | brute     | wind1    | small  | quadruped     | done                |
| 21  | 1686 | `orc_baby`         | Orc Baby               | demihuman | earth1   | small  | biped_insect  | pending (GLB ready) |
| 22  | 1034 | `thara_frog`       | Thara Frog             | fish      | water2   | medium | frog          | done                |
| 22  | 1058 | `metaller`         | Metaller               | insect    | fire1    | medium | biped_insect  | pending (GLB ready) |
| 22  | 1126 | `goblin_5`         | Goblin                 | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 23  | 1030 | `anacondaq`        | Anacondaq              | brute     | poison1  | medium | caterpillar   | done                |
| 23  | 1067 | `cornutus`         | Cornutus               | fish      | water1   | small  | egg           | done                |
| 23  | 1103 | `caramel`          | Caramel                | brute     | earth1   | small  | quadruped     | done                |
| 23  | 1125 | `goblin_4`         | Goblin                 | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 23  | 1178 | `zerom`            | Zerom                  | demihuman | fire1    | medium | biped_insect  | done                |
| 23  | 1627 | `anopheles`        | Anopheles              | insect    | wind3    | small  | flying_insect | done                |
| 23  | 1784 | `stapo`            | Stapo                  | formless  | earth2   | small  | blob          | done                |
| 24  | 1001 | `scorpion`         | Scorpion               | insect    | fire1    | small  | biped_insect  | done                |
| 24  | 1023 | `ork_warrior`      | Orc Warrior            | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 24  | 1064 | `megalodon`        | Megalodon              | undead    | undead1  | medium | biped_insect  | done                |
| 24  | 1092 | `vagabond_wolf`    | Vagabond Wolf **BOSS** | brute     | earth1   | medium | quadruped     | done                |
| 24  | 1111 | `drainliar`        | Drainliar              | brute     | shadow2  | small  | bat           | done                |
| 24  | 1116 | `eggyra`           | Eggyra                 | formless  | ghost2   | medium | blob          | done                |
| 24  | 1123 | `goblin_2`         | Goblin                 | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 24  | 1124 | `goblin_3`         | Goblin                 | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 24  | 1153 | `orc_zombie`       | Orc Zombie             | undead    | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 24  | 1235 | `smoking_orc`      | Smoking Orc            | demihuman | earth1   | medium | biped_insect  | pending (GLB ready) |
| 24  | 1588 | `orc_xmas`         | Christmas Orc          | demihuman | earth1   | medium | biped_insect  | pending (GLB ready) |
| 25  | 1013 | `wolf`             | Wolf                   | brute     | earth1   | medium | quadruped     | done                |
| 25  | 1040 | `golem`            | Golem                  | formless  | neutral1 | large  | tree          | pending (GLB ready) |
| 25  | 1060 | `bigfoot`          | Bigfoot                | brute     | earth1   | large  | quadruped     | pending (GLB ready) |
| 25  | 1071 | `pirate_skel`      | Pirate Skeleton        | undead    | undead1  | medium | biped_insect  | pending (GLB ready) |
| 25  | 1100 | `argos`            | Argos                  | insect    | poison1  | large  | biped_insect  | done                |
| 25  | 1122 | `goblin_1`         | Goblin                 | demihuman | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 25  | 1245 | `gobline_xmas`     | Christmas Goblin       | demihuman | wind1    | medium | biped_insect  | pending (GLB ready) |
| 25  | 1265 | `cookie`           | Cookie                 | demihuman | neutral3 | small  | biped_insect  | done                |
| 25  | 1392 | `rotar_zairo`      | Rotar Zairo            | formless  | wind2    | large  | ?             | pending (GLB ready) |
| 26  | 1118 | `flora`            | Flora                  | plant     | earth1   | large  | plant         | done                |
| 26  | 1127 | `hode`             | Hode                   | brute     | earth2   | medium | caterpillar   | done                |
| 26  | 1138 | `magnolia`         | Magnolia               | demon     | water1   | small  | ?             | pending (GLB ready) |
| 26  | 1139 | `mantis`           | Mantis                 | insect    | earth1   | medium | biped_insect  | done                |
| 26  | 1158 | `phen`             | Phen                   | fish      | water2   | medium | frog          | done                |
| 26  | 1166 | `savage`           | Savage                 | brute     | earth2   | large  | quadruped     | done                |
| 26  | 1221 | `m_savage`         | Savage                 | brute     | earth2   | large  | ?             | done                |
| 26  | 1613 | `metaling`         | Metaling               | formless  | neutral1 | small  | ?             | done                |
| 27  | 1106 | `desert_wolf`      | Desert Wolf            | brute     | fire1    | medium | quadruped     | done                |
| 27  | 1220 | `m_desert_wolf`    | Desert Wolf            | brute     | fire1    | medium | quadruped     | done                |
| 27  | 1409 | `rice_cake_boy`    | Dumpling Child         | demihuman | neutral1 | small  | biped_insect  | pending (GLB ready) |
| 28  | 1142 | `marine_sphere`    | Marine Sphere          | plant     | water2   | small  | tree          | done                |
| 28  | 1152 | `orc_skeleton`     | Orc Skeleton           | undead    | neutral1 | medium | biped_insect  | pending (GLB ready) |
| 28  | 1246 | `cookie_xmas`      | Christmas Cookie       | demihuman | holy2    | small  | ?             | done                |
| 28  | 1258 | `goblin_archer`    | Goblin Archer          | demihuman | neutral1 | small  | biped_insect  | pending (GLB ready) |
| 28  | 1619 | `porcellio`        | Porcellio              | insect    | earth3   | small  | biped_insect  | done                |
| 29  | 1028 | `soldier_skeleton` | Soldier Skeleton       | undead    | undead1  | medium | biped_insect  | pending (GLB ready) |
| 29  | 1121 | `giearth`          | Giearth                | demon     | earth1   | small  | ?             | done                |
| 30  | 1026 | `munak`            | Munak                  | undead    | undead1  | medium | biped_insect  | pending (GLB ready) |
| 30  | 1069 | `sword_fish`       | Swordfish              | fish      | water2   | large  | frog          | done                |
| 30  | 1119 | `frilldora`        | Frilldora              | brute     | fire1    | medium | quadruped     | done                |
| 30  | 1169 | `skel_worker`      | Skeleton Worker        | undead    | undead1  | medium | biped_insect  | pending (GLB ready) |
| 30  | 1243 | `sasquatch`        | Sasquatch              | brute     | neutral3 | large  | quadruped     | pending (GLB ready) |
| 30  | 1400 | `karakasa`         | Karakasa               | formless  | neutral3 | medium | ?             | done                |

### Level 31-40 (51 total — 24 done, 27 GLB ready)

| Lv  | ID   | Key                | Name              | Race      | Element  | Size   | Preset       | Sprite              |
| --- | ---- | ------------------ | ----------------- | --------- | -------- | ------ | ------------ | ------------------- |
| 31  | 1016 | `archer_skeleton`  | Archer Skeleton   | undead    | undead1  | medium | biped_insect | pending (GLB ready) |
| 31  | 1044 | `obeaune`          | Obeaune           | fish      | water2   | medium | frog         | done                |
| 31  | 1134 | `kobold_2`         | Kobold            | demihuman | neutral1 | medium | biped_insect | pending (GLB ready) |
| 31  | 1135 | `kobold_3`         | Kobold            | demihuman | neutral1 | medium | biped_insect | pending (GLB ready) |
| 31  | 1144 | `marse`            | Marse             | fish      | water2   | small  | frog         | done                |
| 31  | 1146 | `matyr`            | Matyr             | brute     | shadow1  | medium | quadruped    | done                |
| 31  | 1177 | `zenorc`           | Zenorc            | demihuman | shadow1  | medium | biped_insect | pending (GLB ready) |
| 31  | 1273 | `orc_lady`         | Orc Lady          | demihuman | neutral1 | medium | biped_insect | pending (GLB ready) |
| 31  | 1582 | `deviling`         | Deviling **BOSS** | demon     | shadow4  | medium | ?            | done                |
| 31  | 1782 | `roween`           | Roween            | brute     | wind1    | medium | quadruped    | done                |
| 32  | 1188 | `bon_gun`          | Bongun            | undead    | undead1  | medium | biped_insect | pending (GLB ready) |
| 32  | 1279 | `tri_joint`        | Tri Joint         | insect    | earth1   | small  | biped_insect | done                |
| 32  | 1415 | `baby_leopard`     | Baby Leopard      | brute     | ghost1   | small  | quadruped    | done                |
| 33  | 1110 | `dokebi`           | Dokebi            | demon     | shadow1  | small  | ?            | pending (GLB ready) |
| 33  | 1170 | `sohee`            | Sohee             | demon     | water1   | medium | ?            | pending (GLB ready) |
| 33  | 1282 | `kobold_archer`    | Kobold Archer     | demihuman | neutral1 | small  | biped_insect | pending (GLB ready) |
| 33  | 1404 | `miyabi_ningyo`    | Miyabi Doll       | demon     | shadow1  | medium | ?            | pending (GLB ready) |
| 34  | 1129 | `horong`           | Horong            | formless  | fire4    | small  | ?            | done                |
| 34  | 1165 | `sand_man`         | Sandman           | formless  | earth3   | medium | ?            | pending (GLB ready) |
| 34  | 1179 | `whisper`          | Whisper           | demon     | ghost3   | small  | ?            | done                |
| 34  | 1185 | `whisper_`         | Whisper           | undead    | ghost1   | small  | biped_insect | done                |
| 34  | 1186 | `whisper_boss`     | Giant Whisper     | demon     | ghost2   | small  | ?            | done                |
| 34  | 1494 | `kind_of_beetle`   | Beetle King       | insect    | earth1   | small  | biped_insect | pending (GLB ready) |
| 35  | 1164 | `requiem`          | Requiem           | demihuman | shadow1  | medium | biped_insect | pending (GLB ready) |
| 35  | 1248 | `cruiser`          | Cruiser           | formless  | neutral3 | medium | ?            | pending (GLB ready) |
| 35  | 1280 | `steam_goblin`     | Goblin Steamrider | demihuman | wind2    | medium | biped_insect | pending (GLB ready) |
| 35  | 1417 | `zipper_bear`      | Zipper Bear       | brute     | shadow1  | medium | quadruped    | pending (GLB ready) |
| 35  | 1620 | `noxious`          | Noxious           | formless  | ghost3   | medium | ?            | done                |
| 36  | 1045 | `marc`             | Marc              | fish      | water2   | medium | frog         | done                |
| 36  | 1133 | `kobold_1`         | Kobold            | demihuman | neutral1 | medium | biped_insect | pending (GLB ready) |
| 36  | 1509 | `lude`             | Lude              | undead    | undead1  | small  | biped_insect | pending (GLB ready) |
| 36  | 1628 | `mole`             | Holden            | brute     | earth2   | small  | quadruped    | done                |
| 37  | 1041 | `mummy`            | Mummy             | undead    | undead2  | medium | biped_insect | pending (GLB ready) |
| 38  | 1032 | `verit`            | Verit             | undead    | undead1  | medium | biped_insect | pending (GLB ready) |
| 38  | 1130 | `jakk`             | Jakk              | formless  | fire2    | medium | ?            | pending (GLB ready) |
| 38  | 1151 | `myst`             | Myst              | formless  | poison1  | large  | ?            | done                |
| 38  | 1244 | `jakk_xmas`        | Christmas Jakk    | formless  | fire2    | medium | ?            | pending (GLB ready) |
| 38  | 1249 | `mystcase`         | Myst Case         | formless  | neutral3 | medium | ?            | done                |
| 38  | 1261 | `wild_rose`        | Wild Rose         | brute     | wind1    | small  | quadruped    | done                |
| 38  | 1586 | `leaf_cat`         | Leaf Cat          | brute     | earth1   | small  | quadruped    | done                |
| 38  | 1789 | `iceicle`          | Iceicle           | formless  | water2   | small  | ?            | done                |
| 39  | 1498 | `wootan_shooter`   | Wootan Shooter    | demihuman | earth2   | medium | biped_insect | pending (GLB ready) |
| 40  | 1036 | `ghoul`            | Ghoul             | undead    | undead2  | medium | biped_insect | pending (GLB ready) |
| 40  | 1140 | `marduk`           | Marduk            | demihuman | fire1    | large  | biped_insect | pending (GLB ready) |
| 40  | 1215 | `stem_worm`        | Stem Worm         | plant     | wind1    | medium | caterpillar  | done                |
| 40  | 1255 | `neraid`           | Nereid            | brute     | earth1   | small  | quadruped    | done                |
| 40  | 1256 | `pest`             | Pest              | brute     | shadow2  | small  | quadruped    | done                |
| 40  | 1277 | `greatest_general` | Greatest General  | formless  | fire2    | medium | ?            | done                |
| 40  | 1508 | `quve`             | Quve              | undead    | undead1  | small  | biped_insect | done                |
| 40  | 1585 | `mime_monkey`      | Mime Monkey       | plant     | water1   | medium | tree         | pending (GLB ready) |
| 40  | 1836 | `magmaring`        | Magmaring         | formless  | fire2    | small  | ?            | pending (GLB ready) |

### Level 41-50 (52 total — 2 done, 50 GLB ready)

| Lv  | ID   | Key                | Name             | Race      | Element  | Size   | Preset       | Sprite              |
| --- | ---- | ------------------ | ---------------- | --------- | -------- | ------ | ------------ | ------------------- |
| 41  | 1099 | `argiope`          | Argiope          | insect    | poison1  | large  | biped_insect | pending (GLB ready) |
| 41  | 1143 | `marionette`       | Marionette       | demon     | ghost3   | small  | ?            | pending (GLB ready) |
| 41  | 1406 | `kapha`            | Kapha            | fish      | water1   | medium | frog         | pending (GLB ready) |
| 41  | 1499 | `wootan_fighter`   | Wootan Fighter   | demihuman | fire2    | medium | biped_insect | pending (GLB ready) |
| 42  | 1035 | `hunter_fly`       | Hunter Fly       | insect    | wind2    | small  | biped_insect | pending (GLB ready) |
| 42  | 1250 | `chepet`           | Chepet           | demihuman | fire1    | medium | biped_insect | pending (GLB ready) |
| 42  | 1271 | `alligator`        | Alligator        | brute     | water1   | medium | quadruped    | pending (GLB ready) |
| 42  | 1495 | `stone_shooter`    | Stone Shooter    | plant     | fire3    | medium | tree         | pending (GLB ready) |
| 42  | 1621 | `venomous`         | Venomous         | formless  | poison1  | medium | ?            | pending (GLB ready) |
| 42  | 1715 | `novus`            | Novus            | dragon    | neutral1 | small  | ?            | pending (GLB ready) |
| 42  | 1776 | `siroma`           | Siroma           | formless  | water3   | small  | ?            | pending (GLB ready) |
| 43  | 1037 | `side_winder`      | Side Winder      | brute     | poison1  | medium | quadruped    | pending (GLB ready) |
| 43  | 1199 | `punk`             | Punk             | plant     | wind1    | small  | tree         | pending (GLB ready) |
| 43  | 1214 | `choco`            | Choco            | brute     | fire1    | small  | quadruped    | pending (GLB ready) |
| 43  | 1281 | `sageworm`         | Sage Worm        | brute     | neutral3 | small  | caterpillar  | pending (GLB ready) |
| 43  | 1367 | `blazzer`          | Blazer           | demon     | fire2    | medium | ?            | pending (GLB ready) |
| 43  | 1616 | `pitman`           | Pitman           | undead    | earth2   | large  | biped_insect | pending (GLB ready) |
| 43  | 1629 | `hill_wind`        | Hill Wind        | brute     | wind3    | medium | quadruped    | pending (GLB ready) |
| 43  | 1694 | `plasma_r`         | Plasma           | formless  | fire4    | small  | ?            | pending (GLB ready) |
| 43  | 1718 | `novus_`           | Novus            | dragon    | neutral1 | small  | ?            | pending (GLB ready) |
| 43  | 1721 | `dragon_egg`       | Dragon Egg       | dragon    | neutral2 | medium | egg          | done                |
| 44  | 1102 | `bathory`          | Bathory          | demihuman | shadow1  | medium | bat          | pending (GLB ready) |
| 44  | 1155 | `petit`            | Petite           | dragon    | earth1   | medium | ?            | pending (GLB ready) |
| 44  | 1697 | `plasma_b`         | Plasma           | formless  | water4   | small  | ?            | pending (GLB ready) |
| 44  | 1783 | `galion`           | Galion **BOSS**  | brute     | wind2    | medium | quadruped    | pending (GLB ready) |
| 45  | 1156 | `petit_`           | Petite           | dragon    | wind1    | medium | ?            | pending (GLB ready) |
| 45  | 1274 | `megalith`         | Megalith         | formless  | neutral4 | large  | ?            | pending (GLB ready) |
| 45  | 1680 | `hill_wind_1`      | Hill Wind        | brute     | wind3    | medium | quadruped    | pending (GLB ready) |
| 46  | 1109 | `deviruchi`        | Deviruchi        | demon     | shadow1  | small  | ?            | pending (GLB ready) |
| 46  | 1211 | `brilight`         | Brilight         | insect    | fire1    | small  | biped_insect | pending (GLB ready) |
| 46  | 1383 | `explosion`        | Explosion        | brute     | fire3    | small  | quadruped    | pending (GLB ready) |
| 46  | 1402 | `poison_toad`      | Poison Toad      | brute     | poison2  | medium | frog         | pending (GLB ready) |
| 46  | 1413 | `wild_ginseng`     | Hermit Plant     | plant     | fire2    | small  | tree         | done                |
| 46  | 1781 | `drosera`          | Drosera          | plant     | earth1   | medium | tree         | pending (GLB ready) |
| 47  | 1029 | `isis`             | Isis             | demon     | shadow1  | large  | ?            | pending (GLB ready) |
| 47  | 1108 | `deviace`          | Deviace          | fish      | water4   | medium | frog         | pending (GLB ready) |
| 47  | 1212 | `iron_fist`        | Iron Fist        | insect    | neutral3 | medium | biped_insect | pending (GLB ready) |
| 47  | 1403 | `antique_firelock` | Firelock Soldier | undead    | undead2  | medium | biped_insect | pending (GLB ready) |
| 47  | 1695 | `plasma_g`         | Plasma           | formless  | earth4   | small  | ?            | pending (GLB ready) |
| 48  | 1065 | `strouf`           | Strouf           | fish      | water3   | large  | frog         | pending (GLB ready) |
| 48  | 1253 | `gargoyle`         | Gargoyle         | demon     | wind3    | medium | ?            | pending (GLB ready) |
| 48  | 1517 | `li_me_mang_ryang` | Jing Guai        | demon     | earth3   | medium | ?            | pending (GLB ready) |
| 49  | 1061 | `nightmare`        | Nightmare        | demon     | ghost3   | large  | ?            | pending (GLB ready) |
| 49  | 1189 | `orc_archer`       | Orc Archer       | demihuman | neutral1 | medium | biped_insect | pending (GLB ready) |
| 49  | 1500 | `parasite`         | Parasite         | plant     | wind2    | medium | tree         | pending (GLB ready) |
| 49  | 1519 | `chung_e`          | Green Maiden     | demihuman | neutral2 | medium | biped_insect | pending (GLB ready) |
| 49  | 1696 | `plasma_p`         | Plasma           | formless  | shadow4  | small  | ?            | pending (GLB ready) |
| 50  | 1101 | `baphomet_`        | Baphomet Jr.     | demon     | shadow1  | small  | ?            | pending (GLB ready) |
| 50  | 1493 | `dryad`            | Dryad            | plant     | earth4   | medium | tree         | pending (GLB ready) |
| 50  | 1587 | `kraben`           | Kraben           | formless  | ghost2   | medium | ?            | pending (GLB ready) |
| 50  | 1615 | `obsidian`         | Obsidian         | formless  | earth2   | small  | ?            | pending (GLB ready) |
| 50  | 1838 | `knocker`          | Knocker          | demon     | earth1   | small  | ?            | pending (GLB ready) |

### Level 51-60 (67 total — 0 done, 1 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  51 | 1180 | `nine_tail` | Nine Tail | brute | fire3 | medium | quadruped | no GLB |
|  51 | 1191 | `mimic` | Mimic | formless | neutral3 | medium | ? | no GLB |
|  51 | 1257 | `injustice` | Injustice | undead | shadow2 | medium | biped_insect | no GLB |
|  51 | 1263 | `wind_ghost` | Wind Ghost | demon | wind3 | medium | ? | no GLB |
|  51 | 1267 | `carat` | Carat | demon | wind2 | medium | ? | no GLB |
|  51 | 1497 | `wooden_golem` | Wooden Golem | plant | neutral1 | large | tree | no GLB |
|  51 | 1510 | `hylozoist` | Heirozoist | demon | shadow2 | small | ? | no GLB |
|  51 | 1516 | `increase_soil` | Mi Gao | formless | earth3 | medium | ? | no GLB |
|  52 | 1149 | `minorous` | Minorous | brute | fire2 | large | ? | no GLB |
|  52 | 1163 | `raydric` | Raydric | demihuman | shadow2 | large | biped_insect | no GLB |
|  52 | 1196 | `skel_prisoner` | Skeleton Prisoner | undead | undead3 | medium | biped_insect | no GLB |
|  52 | 1213 | `high_orc` | High Orc | demihuman | neutral1 | large | biped_insect | no GLB |
|  52 | 1276 | `raydric_archer` | Raydric Archer | demon | shadow2 | medium | ? | no GLB |
|  52 | 1380 | `driller` | Driller | brute | earth1 | medium | quadruped | no GLB |
|  52 | 1584 | `tamruan` | Tamruan | demon | shadow3 | large | ? | no GLB |
|  53 | 1192 | `wraith` | Wraith | undead | undead4 | large | ? | no GLB |
|  53 | 1197 | `zombie_prisoner` | Zombie Prisoner | undead | undead3 | medium | biped_insect | no GLB |
|  53 | 1264 | `merman` | Merman | demihuman | water3 | medium | biped_insect | no GLB |
|  53 | 1410 | `live_peach_tree` | Enchanted Peach Tree | plant | earth2 | medium | tree | no GLB |
|  53 | 1632 | `gremlin` | Gremlin | demon | shadow2 | large | ? | no GLB |
|  54 | 1514 | `dancing_dragon` | Zhu Po Long | dragon | wind2 | medium | ? | no GLB |
|  54 | 1687 | `green_iguana` | Grove | brute | earth2 | medium | quadruped | no GLB |
|  55 | 1304 | `giant_spider` | Giant Spider | insect | poison1 | large | biped_insect | no GLB |
|  55 | 1408 | `blood_butterfly` | Bloody Butterfly | insect | wind2 | medium | flying_insect | no GLB |
|  55 | 1506 | `disguise` | Disguise | demon | earth4 | medium | ? | no GLB |
|  55 | 1682 | `removal` | Remover | undead | undead2 | medium | biped_insect | no GLB |
|  55 | 1738 | `constant` | Constant | formless | shadow3 | small | ? | no GLB |
|  55 | 1778 | `gazeti` | Gazeti | demon | water1 | medium | ? | no GLB |
|  56 | 1209 | `cramp` | Cramp | brute | poison2 | small | quadruped | no GLB |
|  56 | 1294 | `killer_mantis` | Killer Mantis | insect | earth1 | medium | biped_insect | no GLB |
|  56 | 1303 | `giant_honet` | Giant Hornet | insect | wind1 | small | biped_insect | no GLB |
|  56 | 1368 | `geographer` | Geographer | plant | earth3 | medium | tree | no GLB |
|  56 | 1375 | `the_paper` | The Paper | formless | neutral3 | medium | ? | no GLB |
|  56 | 1378 | `demon_pungus` | Demon Pungus | demon | poison3 | small | ? | no GLB |
|  56 | 1412 | `evil_cloud_hermit` | Taoist Hermit | formless | neutral2 | large | ? | no GLB |
|  56 | 1512 | `hyegun` | Yao Jun | undead | undead2 | medium | biped_insect | no GLB |
|  56 | 1614 | `mineral` | Mineral | formless | neutral2 | small | ? | no GLB |
|  56 | 1633 | `beholder` | Beholder | formless | wind2 | small | ? | no GLB |
|  56 | 1692 | `breeze` | Breeze | formless | wind3 | medium | ? | no GLB |
|  56 | 1693 | `plasma_y` | Plasma | formless | ghost4 | small | ? | no GLB |
|  57 | 1131 | `joker` | Joker | demihuman | wind4 | large | biped_insect | no GLB |
|  57 | 1216 | `penomena` | Penomena | fish | poison1 | medium | frog | no GLB |
|  57 | 1780 | `muscipular` | Muscipular | plant | earth1 | medium | tree | no GLB |
|  58 | 1117 | `evil_druid` | Evil Druid | undead | undead4 | large | biped_insect | no GLB |
|  58 | 1193 | `alarm` | Alarm | formless | neutral3 | medium | ? | no GLB |
|  58 | 1306 | `leib_olmai` | Leib Olmai | brute | earth1 | large | ? | no GLB |
|  58 | 1322 | `spring_rabbit` | Spring Rabbit | brute | earth2 | medium | quadruped | no GLB |
|  58 | 1369 | `grand_peco` | Grand Peco | brute | fire2 | large | ? | no GLB |
|  58 | 1503 | `gibbet` | Gibbet | demon | shadow1 | large | ? | no GLB |
|  58 | 1652 | `ygnizem` | Egnigem Cenia | demihuman | fire2 | medium | biped_insect | no GLB |
|  59 | 1194 | `arclouse` | Arclouze | insect | earth2 | medium | biped_insect | no GLB |
|  59 | 1195 | `rideword` | Rideword | formless | neutral3 | small | ? | no GLB |
|  59 | 1260 | `dark_frame` | Dark Frame | demon | shadow3 | medium | ? | no GLB |
|  59 | 1308 | `panzer_goblin` | Panzer Goblin | demihuman | wind2 | medium | biped_insect | no GLB |
|  59 | 1323 | `see_otter` | Sea Otter | brute | water3 | medium | quadruped | no GLB |
|  59 | 1631 | `chung_e_` | Green Maiden | demihuman | wind2 | medium | biped_insect | pending (shares `chung_e` GLB) |
|  59 | 1655 | `erend` | Errende Ebecee | demihuman | holy2 | medium | biped_insect | no GLB |
|  59 | 1771 | `vanberk` | Vanberk | demihuman | neutral4 | medium | biped_insect | no GLB |
|  60 | 1072 | `kaho` | Kaho | demon | fire4 | medium | ? | no GLB |
|  60 | 1269 | `clock` | Clock | formless | earth2 | medium | ? | no GLB |
|  60 | 1278 | `stalactic_golem` | Stalactic Golem | formless | neutral1 | large | ? | no GLB |
|  60 | 1387 | `gig` | Gig | brute | fire2 | small | quadruped | no GLB |
|  60 | 1388 | `archangeling` | Arc Angeling **BOSS** | angel | holy3 | medium | ? | no GLB |
|  60 | 1656 | `kavac` | Kavach Icarus | demihuman | wind2 | medium | biped_insect | no GLB |
|  60 | 1699 | `ancient_mimic` | Ancient Mimic | formless | neutral3 | large | ? | no GLB |
|  60 | 1775 | `snowier` | Snowier | formless | water2 | large | ? | no GLB |
|  60 | 1777 | `ice_titan` | Ice Titan | formless | water3 | large | ? | no GLB |

### Level 61-70 (72 total — 0 done, 0 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  61 | 1154 | `pasana` | Pasana | demihuman | fire2 | medium | biped_insect | no GLB |
|  61 | 1206 | `anolian` | Anolian | fish | water2 | medium | frog | no GLB |
|  61 | 1207 | `sting` | Sting | formless | earth3 | medium | ? | no GLB |
|  61 | 1301 | `am_mut` | Am Mut | demon | shadow1 | small | ? | no GLB |
|  61 | 1313 | `mobster` | Mobster | demihuman | neutral1 | medium | biped_insect | no GLB |
|  61 | 1321 | `dragon_tail` | Dragon Tail | insect | wind2 | medium | biped_insect | no GLB |
|  61 | 1391 | `galapago` | Galapago | brute | earth1 | small | quadruped | no GLB |
|  61 | 1515 | `garm_baby` | Baby Hatii | brute | water2 | medium | quadruped | no GLB |
|  61 | 1657 | `rawrel` | Laurell Weinder | demihuman | ghost2 | medium | biped_insect | no GLB |
|  61 | 1773 | `hodremlin` | Hodremlin | demon | shadow3 | medium | ? | no GLB |
|  62 | 1275 | `alice` | Alice | demihuman | neutral3 | medium | biped_insect | no GLB |
|  62 | 1293 | `cremy_fear` | Creamy Fear | insect | wind1 | small | biped_insect | no GLB |
|  62 | 1298 | `zombie_master` | Zombie Master | undead | undead1 | medium | biped_insect | no GLB |
|  62 | 1311 | `gullinbursti` | Gullinbursti | brute | earth2 | large | ? | no GLB |
|  62 | 1504 | `dullahan` | Dullahan | undead | undead2 | medium | biped_insect | no GLB |
|  62 | 1513 | `civil_servant` | Mao Guai | brute | wind2 | medium | quadruped | no GLB |
|  62 | 1653 | `whikebain` | Wickebine Tres | demihuman | poison3 | medium | biped_insect | no GLB |
|  62 | 1772 | `isilla` | Isilla | demihuman | neutral4 | medium | biped_insect | no GLB |
|  62 | 1796 | `aunoe` | Aunoe | demihuman | neutral4 | medium | biped_insect | no GLB |
|  62 | 1797 | `fanat` | Fanat | demihuman | neutral4 | medium | biped_insect | no GLB |
|  63 | 1132 | `khalitzburg` | Khalitzburg | undead | undead1 | large | biped_insect | no GLB |
|  63 | 1200 | `zherlthsh` | Zealotus | demihuman | neutral3 | medium | biped_insect | no GLB |
|  63 | 1270 | `c_tower_manager` | Clock Tower Manager | formless | neutral4 | large | ? | no GLB |
|  63 | 1309 | `gajomart` | Gajomart | formless | fire4 | small | ? | no GLB |
|  63 | 1314 | `permeter` | Permeter | brute | neutral2 | medium | quadruped | no GLB |
|  63 | 1317 | `fur_seal` | Seal | brute | water1 | medium | quadruped | no GLB |
|  63 | 1416 | `wicked_nymph` | Evil Nymph | demon | shadow3 | medium | ? | no GLB |
|  64 | 1086 | `golden_bug` | Golden Thief Bug **MVP** | insect | fire2 | large | biped_insect | no GLB |
|  64 | 1297 | `ancient_mummy` | Ancient Mummy | undead | undead2 | medium | biped_insect | no GLB |
|  64 | 1299 | `goblin_leader` | Goblin Leader | demihuman | neutral1 | medium | biped_insect | no GLB |
|  64 | 1300 | `caterpillar` | Caterpillar | insect | earth1 | small | biped_insect | no GLB |
|  64 | 1377 | `elder` | Elder | demihuman | neutral4 | large | biped_insect | no GLB |
|  64 | 1666 | `poton_canon_2` | Photon Cannon | formless | neutral2 | medium | ? | no GLB |
|  65 | 1115 | `eddga` | Eddga **MVP** | brute | fire1 | large | ? | no GLB |
|  65 | 1205 | `executioner` | Executioner **BOSS** | formless | shadow2 | large | ? | no GLB |
|  65 | 1262 | `mutant_dragon` | Mutant Dragonoid **BOSS** | dragon | fire2 | large | ? | no GLB |
|  65 | 1296 | `kobold_leader` | Kobold Leader | demihuman | neutral1 | medium | biped_insect | no GLB |
|  65 | 1371 | `fake_angel` | False Angel | angel | holy3 | small | ? | no GLB |
|  65 | 1385 | `deleter_` | Deleter | dragon | fire2 | medium | ? | no GLB |
|  65 | 1405 | `tengu` | Tengu | demon | earth2 | large | ? | no GLB |
|  65 | 1667 | `poton_canon_3` | Photon Cannon | formless | neutral2 | medium | ? | no GLB |
|  65 | 1698 | `deathword` | Death Word | formless | neutral3 | medium | ? | no GLB |
|  65 | 1774 | `seeker` | Seeker | formless | wind3 | small | ? | no GLB |
|  66 | 1310 | `majoruros` | Majoruros | brute | fire2 | large | ? | no GLB |
|  66 | 1365 | `apocalips` | Apocalypse | formless | neutral3 | large | ? | no GLB |
|  66 | 1384 | `deleter` | Deleter | dragon | fire2 | medium | ? | no GLB |
|  66 | 1654 | `armaia` | Armeyer Dinze | demihuman | earth3 | medium | biped_insect | no GLB |
|  66 | 1664 | `poton_canon` | Photon Cannon | formless | neutral2 | medium | ? | no GLB |
|  67 | 1150 | `moonlight` | Moonlight Flower **MVP** | demon | fire3 | medium | ? | no GLB |
|  67 | 1305 | `ancient_worm` | Ancient Worm | insect | poison1 | large | caterpillar | no GLB |
|  67 | 1382 | `diabolic` | Diabolic | demon | shadow2 | small | ? | no GLB |
|  67 | 1386 | `sleeper` | Sleeper | formless | earth2 | medium | ? | no GLB |
|  67 | 1665 | `poton_canon_1` | Photon Cannon | formless | neutral2 | medium | ? | no GLB |
|  68 | 1292 | `mini_demon` | Mini Demon | demon | shadow1 | small | ? | no GLB |
|  68 | 1318 | `heater` | Heater | brute | fire2 | medium | quadruped | no GLB |
|  68 | 1381 | `grizzly` | Grizzly | brute | fire3 | large | ? | no GLB |
|  68 | 1617 | `waste_stove` | Old Stove | formless | neutral1 | large | ? | no GLB |
|  69 | 1159 | `phreeoni` | Phreeoni **MVP** | brute | neutral3 | large | ? | no GLB |
|  69 | 1372 | `goat` | Goat | brute | fire3 | medium | quadruped | no GLB |
|  69 | 1401 | `shinobi` | Shinobi | demihuman | neutral1 | medium | biped_insect | no GLB |
|  69 | 1618 | `ungoliant` | Ungoliant | insect | poison2 | large | biped_insect | no GLB |
|  69 | 1717 | `ferus_` | Ferus | dragon | earth2 | large | ? | no GLB |
|  69 | 1737 | `aliza` | Aliza | demihuman | neutral3 | medium | biped_insect | no GLB |
|  69 | 1753 | `frus` | Frus | demon | shadow3 | medium | ? | no GLB |
|  69 | 1770 | `echio` | Echio | demihuman | neutral4 | medium | biped_insect | no GLB |
|  70 | 1112 | `drake` | Drake **MVP** | undead | undead1 | medium | biped_insect | no GLB |
|  70 | 1283 | `chimera` | Chimera **BOSS** | brute | fire3 | large | ? | no GLB |
|  70 | 1316 | `solider` | Solider | brute | earth2 | medium | quadruped | no GLB |
|  70 | 1376 | `harpy` | Harpy | demon | wind3 | medium | ? | no GLB |
|  70 | 1583 | `tao_gunka` | Tao Gunka **MVP** | demon | neutral3 | large | ? | no GLB |
|  70 | 1714 | `ferus` | Ferus | dragon | fire2 | large | ? | no GLB |
|  70 | 1752 | `skogul` | Skogul | demon | shadow3 | medium | ? | no GLB |

### Level 71-80 (57 total — 0 done, 0 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  71 | 1201 | `rybio` | Rybio | demon | neutral2 | large | ? | no GLB |
|  71 | 1204 | `tirfing` | Ogretooth **BOSS** | formless | shadow3 | medium | ? | no GLB |
|  71 | 1315 | `assulter` | Assaulter | demihuman | neutral1 | medium | biped_insect | no GLB |
|  71 | 1492 | `incantation_samurai` | Samurai Specter **MVP** | demihuman | shadow3 | large | biped_insect | no GLB |
|  71 | 1505 | `loli_ruri` | Loli Ruri | demon | shadow4 | large | ? | no GLB |
|  71 | 1622 | `teddy_bear` | Teddy Bear | formless | neutral3 | small | ? | no GLB |
|  72 | 1046 | `doppelganger` | Doppelganger **MVP** | demon | shadow3 | medium | ? | no GLB |
|  72 | 1259 | `gryphon` | Gryphon **BOSS** | brute | wind4 | large | ? | no GLB |
|  72 | 1319 | `freezer` | Freezer | brute | water2 | medium | quadruped | no GLB |
|  72 | 1507 | `bloody_murderer` | Bloody Murderer | demihuman | shadow3 | large | biped_insect | no GLB |
|  72 | 1518 | `bacsojin` | White Lady | demihuman | water2 | large | biped_insect | no GLB |
|  72 | 1676 | `venatu_1` | Venatu | formless | neutral2 | medium | ? | no GLB |
|  72 | 1681 | `gemini` | Gemini-S58 **BOSS** | formless | water1 | medium | ? | no GLB |
|  73 | 1202 | `phendark` | Phendark | demihuman | neutral2 | large | biped_insect | no GLB |
|  73 | 1252 | `garm` | Hatii **MVP** | brute | water4 | large | ? | no GLB |
|  73 | 1290 | `skeleton_general` | Skeleton General | undead | undead1 | medium | biped_insect | no GLB |
|  73 | 1418 | `dark_snake_lord` | Evil Snake Lord **MVP** | brute | ghost3 | large | caterpillar | no GLB |
|  73 | 1769 | `agav` | Agav | demihuman | neutral4 | medium | biped_insect | no GLB |
|  74 | 1059 | `mistress` | Mistress **MVP** | insect | wind4 | small | biped_insect | no GLB |
|  74 | 1190 | `orc_lord` | Orc Lord **MVP** | demihuman | earth4 | large | biped_insect | no GLB |
|  74 | 1208 | `wander_man` | Wanderer | demon | wind1 | medium | ? | no GLB |
|  74 | 1291 | `wraith_dead` | Wraith Dead | undead | undead4 | large | ? | no GLB |
|  75 | 1098 | `anubis` | Anubis | demihuman | undead2 | large | biped_insect | no GLB |
|  75 | 1295 | `owl_baron` | Owl Baron **BOSS** | demon | neutral3 | large | ? | no GLB |
|  75 | 1320 | `owl_duke` | Owl Duke **BOSS** | demon | neutral3 | large | ? | no GLB |
|  75 | 1374 | `incubus` | Incubus | demon | shadow3 | medium | ? | no GLB |
|  75 | 1390 | `violy` | Violy | demihuman | neutral2 | medium | biped_insect | no GLB |
|  75 | 1679 | `venatu_4` | Venatu | formless | water2 | medium | ? | no GLB |
|  75 | 1735 | `alicel` | Alicel | demon | neutral3 | medium | ? | no GLB |
|  75 | 1736 | `aliot` | Aliot | demon | neutral3 | medium | ? | no GLB |
|  76 | 1203 | `mysteltainn` | Mysteltainn **BOSS** | formless | shadow4 | large | ? | no GLB |
|  76 | 1307 | `cat_o_nine_tail` | Cat o' Nine Tails **BOSS** | demon | fire3 | medium | ? | no GLB |
|  76 | 1716 | `acidus_` | Acidus | dragon | wind2 | large | ? | no GLB |
|  76 | 1837 | `imp` | Fire Imp | demon | fire3 | small | ? | no GLB |
|  77 | 1087 | `ork_hero` | Orc Hero **MVP** | demihuman | earth2 | large | biped_insect | no GLB |
|  77 | 1251 | `knight_of_windstorm` | Stormy Knight **MVP** | formless | wind4 | large | ? | no GLB |
|  77 | 1302 | `dark_illusion` | Dark Illusion **BOSS** | demon | undead4 | large | ? | no GLB |
|  77 | 1366 | `lava_golem` | Lava Golem | formless | neutral1 | large | ? | no GLB |
|  77 | 1669 | `dimik` | Dimik | formless | neutral2 | medium | ? | no GLB |
|  77 | 1675 | `venatu` | Venatu | formless | fire2 | medium | ? | no GLB |
|  77 | 1703 | `solace` | Lady Solace **BOSS** | angel | holy3 | medium | ? | no GLB |
|  78 | 1038 | `osiris` | Osiris **MVP** | undead | undead4 | medium | biped_insect | no GLB |
|  78 | 1379 | `nightmare_terror` | Nightmare Terror | demon | shadow3 | large | ? | no GLB |
|  78 | 1678 | `venatu_3` | Venatu | formless | earth2 | medium | ? | no GLB |
|  79 | 1148 | `medusa` | Medusa | demon | neutral2 | medium | ? | no GLB |
|  79 | 1198 | `dark_priest` | Dark Priest **BOSS** | demon | undead4 | medium | ? | no GLB |
|  79 | 1219 | `knight_of_abyss` | Abysmal Knight | demihuman | shadow4 | large | biped_insect | no GLB |
|  79 | 1658 | `b_ygnizem` | Egnigem Cenia **MVP** | demihuman | fire2 | medium | biped_insect | no GLB |
|  79 | 1668 | `archdam` | Archdam | demihuman | neutral3 | large | biped_insect | no GLB |
|  79 | 1670 | `dimik_1` | Dimik | formless | wind2 | medium | ? | no GLB |
|  79 | 1702 | `retribution` | Baroness of Retribution **BOSS** | angel | shadow3 | medium | ? | no GLB |
|  80 | 1272 | `dark_lord` | Dark Lord **MVP** | demon | undead4 | large | ? | no GLB |
|  80 | 1672 | `dimik_3` | Dimik | formless | earth2 | medium | ? | no GLB |
|  80 | 1677 | `venatu_2` | Venatu | formless | wind2 | medium | ? | no GLB |
|  80 | 1701 | `shelter` | Mistress of Shelter **BOSS** | angel | holy3 | medium | ? | no GLB |
|  80 | 1713 | `acidus` | Acidus | dragon | holy2 | large | ? | no GLB |
|  80 | 1830 | `bow_guardian` | Bow Master **BOSS** | demihuman | neutral4 | large | biped_insect | no GLB |

### Level 81-90 (34 total — 0 done, 0 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  81 | 1039 | `baphomet` | Baphomet **MVP** | demon | shadow3 | large | ? | no GLB |
|  81 | 1147 | `maya` | Maya **MVP** | insect | earth4 | large | biped_insect | no GLB |
|  81 | 1289 | `maya_puple` | Maya Purple **BOSS** | insect | earth4 | large | biped_insect | no GLB |
|  81 | 1700 | `observation` | Dame of Sentinel **BOSS** | angel | neutral4 | medium | ? | no GLB |
|  81 | 1754 | `skeggiold` | Skeggiold **BOSS** | angel | holy2 | small | egg | no GLB |
|  82 | 1268 | `bloody_knight` | Bloody Knight | formless | shadow4 | large | ? | no GLB |
|  82 | 1638 | `shecil` | Cecil Damon | demihuman | wind3 | medium | biped_insect | no GLB |
|  82 | 1673 | `dimik_4` | Dimik | formless | fire2 | medium | ? | no GLB |
|  82 | 1785 | `atroce` | Atroce **MVP** | brute | shadow3 | large | ? | no GLB |
|  82 | 1795 | `bloody_knight_` | Bloody Knight **BOSS** | angel | ghost1 | large | ? | no GLB |
|  83 | 1636 | `harword` | Howard Alt-Eisen | demihuman | water4 | medium | biped_insect | no GLB |
|  83 | 1706 | `tha_maero` | Maero of Thanatos **BOSS** | undead | ghost4 | medium | biped_insect | no GLB |
|  83 | 1707 | `tha_dolor` | Dolor of Thanatos **BOSS** | undead | ghost4 | small | biped_insect | no GLB |
|  83 | 1755 | `skeggiold_` | Skeggiold **BOSS** | angel | holy2 | small | egg | no GLB |
|  85 | 1370 | `succubus` | Succubus | demon | shadow3 | medium | ? | no GLB |
|  85 | 1389 | `dracula` | Dracula **MVP** | demon | shadow4 | large | ? | no GLB |
|  85 | 1630 | `bacsojin_` | White Lady **MVP** | demihuman | wind3 | large | biped_insect | no GLB |
|  85 | 1833 | `kasa` | Kasa **BOSS** | formless | fire3 | large | ? | no GLB |
|  86 | 1623 | `rsx_0806` | RSX-0806 **MVP** | formless | neutral3 | large | ? | no GLB |
|  86 | 1829 | `sword_guardian` | Sword Master **BOSS** | demihuman | neutral4 | large | biped_insect | no GLB |
|  86 | 1839 | `byorgue` | Byorgue **BOSS** | demihuman | neutral1 | medium | biped_insect | no GLB |
|  87 | 1635 | `eremes` | Eremes Guile | demon | poison4 | medium | ? | no GLB |
|  88 | 1511 | `amon_ra` | Amon Ra **MVP** | demihuman | earth3 | large | biped_insect | no GLB |
|  88 | 1674 | `monemus` | Monemus **BOSS** | formless | fire3 | large | ? | no GLB |
|  88 | 1705 | `tha_despero` | Despero of Thanatos **BOSS** | undead | ghost4 | large | biped_insect | no GLB |
|  89 | 1671 | `dimik_2` | Dimik | formless | water2 | medium | ? | no GLB |
|  89 | 1688 | `lady_tanee` | Lady Tanee **MVP** | plant | wind3 | large | tree | no GLB |
|  89 | 1720 | `hydro` | Hydrolancer **BOSS** | dragon | shadow2 | large | ? | no GLB |
|  89 | 1768 | `gloomundernight` | Gloom Under Night **MVP** | formless | ghost3 | large | ? | no GLB |
|  90 | 1637 | `magaleta` | Margaretha Sorin | demihuman | holy3 | medium | biped_insect | no GLB |
|  90 | 1719 | `detale` | Detardeurus **MVP** | dragon | shadow3 | large | ? | no GLB |
|  90 | 1733 | `kiel` | Kiehl **BOSS** | formless | shadow2 | medium | ? | no GLB |
|  90 | 1734 | `kiel_` | Kiel D-01 **MVP** | formless | shadow2 | medium | ? | no GLB |
|  90 | 1846 | `dreammetal` | Dream Metal **BOSS** | formless | holy1 | small | ? | no GLB |

### Level 91-100 (26 total — 0 done, 0 GLB ready)

| Lv  | ID   | Key | Name | Race | Element | Size | Preset | Sprite |
| --- | ---- | --- | ---- | ---- | ------- | ---- | ------ | ------ |
|  91 | 1634 | `seyren` | Seyren Windsor | demon | fire3 | medium | ? | no GLB |
|  91 | 1831 | `salamander` | Salamander **BOSS** | formless | fire3 | large | ? | no GLB |
|  92 | 1639 | `katrinn` | Kathryne Keyron | demihuman | ghost3 | medium | biped_insect | no GLB |
|  92 | 1704 | `tha_odium` | Odium of Thanatos **BOSS** | undead | ghost4 | large | biped_insect | no GLB |
|  93 | 1157 | `pharaoh` | Pharaoh **MVP** | demihuman | shadow3 | large | biped_insect | no GLB |
|  94 | 1373 | `lord_of_death` | Lord of the Dead **MVP** | demon | shadow3 | large | ? | no GLB |
|  97 | 1312 | `turtle_general` | Turtle General **MVP** | brute | earth2 | large | ? | no GLB |
|  97 | 1685 | `apocalips_h` | Vesper **MVP** | brute | holy2 | large | ? | no GLB |
|  98 | 1779 | `ktullanux` | Ktullanux **MVP** | brute | water4 | large | ? | no GLB |
|  99 | 1502 | `poring_v` | Bring it on! **MVP** | plant | poison1 | medium | tree | no GLB |
|  99 | 1646 | `b_seyren` | Lord Knight Seyren **MVP** | demihuman | fire4 | medium | biped_insect | no GLB |
|  99 | 1647 | `b_eremes` | Assassin Cross Eremes **MVP** | demihuman | poison4 | medium | biped_insect | no GLB |
|  99 | 1648 | `b_harword` | Whitesmith Howard **MVP** | demihuman | earth4 | medium | biped_insect | no GLB |
|  99 | 1649 | `b_magaleta` | High Priest Margaretha **MVP** | demihuman | holy4 | medium | biped_insect | no GLB |
|  99 | 1650 | `b_shecil` | Sniper Cecil **MVP** | demihuman | wind4 | medium | biped_insect | no GLB |
|  99 | 1651 | `b_katrinn` | High Wizard Kathryne **MVP** | demihuman | ghost3 | medium | biped_insect | no GLB |
|  99 | 1708 | `thanatos` | Memory of Thanatos **MVP** | demon | ghost4 | large | ? | no GLB |
|  99 | 1751 | `randgris` | Valkyrie Randgris **MVP** | angel | holy4 | large | ? | no GLB |
|  99 | 1805 | `b_seyren_` | Lord Knight Seyren **BOSS** | demihuman | fire4 | medium | biped_insect | no GLB |
|  99 | 1806 | `b_eremes_` | Assassin Cross Eremes **BOSS** | demihuman | poison4 | medium | biped_insect | no GLB |
|  99 | 1807 | `b_harword_` | Mastersmith Howard **BOSS** | demihuman | earth4 | medium | biped_insect | no GLB |
|  99 | 1808 | `b_magaleta_` | High Priest Margaretha **BOSS** | demihuman | holy4 | medium | biped_insect | no GLB |
|  99 | 1809 | `b_shecil_` | Sniper Cecil **BOSS** | demihuman | wind4 | medium | biped_insect | no GLB |
|  99 | 1810 | `b_katrinn_` | High Wizard Kathryne **BOSS** | demihuman | ghost3 | medium | biped_insect | no GLB |
|  99 | 1832 | `ifrit` | Ifrit **MVP** | formless | fire4 | large | ? | no GLB |
|  99 | 1840 | `golden_savage` | Golden Savage **BOSS** | brute | earth2 | large | ? | no GLB |

### Note on Unmapped GLB Files
- `rocker-fixed.glb`, `rocker_rigged.glb` — render-experiment variants of `rocker.glb`, ignored
- All other GLBs are now bound to a template (via direct name match, spriteClass, or explicit alias in `build_enemy_list.js`)

### How To Use This List
1. Pick the next `pending (GLB ready)` row (top-most / lowest level).
2. Run the render command using the suggested preset (see top of this file).
3. Pack with `pack_atlas.py` to `Body/enemies/{name}/`.
4. Add `spriteClass: '{name}', weaponMode: 0` to the template in `ro_monster_templates.js`.
5. Any `pending (shares {name} GLB)` entries will auto-update to `done (shares {name})` on next regen.
6. Import PNGs into UE5 (UserInterface2D, Nearest, NoMipmaps, Never Stream). Restart server.

### Regenerating This List
After rendering more sprites or adding more GLBs, run:
```bash
cd /c/Sabri_MMO
node _prompts/build_enemy_list.js       # → _prompts/.enemy_dump.json
node _prompts/build_enemy_markdown.js   # → _prompts/.enemy_table.md
```
Then replace this section of `enemy_sprite_session_resume.md` (everything after the top-level separator that introduces "MASTER ENEMY LIST") with the contents of `.enemy_table.md`.
