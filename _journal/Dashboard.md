# Sabri_MMO Dashboard

## Quick Links
- [Session Tracker](Session%20Tracker.md) — All Claude Code sessions with resume IDs
- [Workflow Guide](Workflow%20Guide.md) — How to use Obsidian + Claude Code together
- [Prompt Library](../_prompts/README.md) — Reusable prompts that work
- [Documentation Index](DocsNewINDEX.md) — Master doc navigation
- [Project Overview](../docsNew/00_Project_Overview.md) — Full system inventory
- [Global Rules](../docsNew/00_Global_Rules/Global_Rules.md) — Design standards
- [CLAUDE.md](../CLAUDE.md) — Claude Code project instructions

## Current Stats (2026-05-10)
| Metric | Count |
|--------|-------|
| Server lines | ~35,940 (index.js — +402 from 05-07 + ~40 from 05-09 `job:change` gating/HP-SP refill/DB persist) + 12 data modules (~6,400 — +`ro_spawn_regions.js` 8.9 KB) |
| Socket handlers | 107 `socket.on(...)` in index.js (water:enter/exit handlers expanded with ref counting + ZoneTransition `ShowExpectedZoneChange`/`HideExpectedZoneChange` API) |
| REST endpoints | 11 |
| C++ UI Subsystems | **42** (+JobChangeSubsystem 05-09) |
| In-world NPC actor types | **3** — `AShopNPC`, `AKafraNPC`, `AJobChangeNPC` (05-09). Live click handler: `UPlayerInputSubsystem::ProcessClickOnNPC` (4 cast sites must include each new type) |
| Skill definitions | 293 (69 first + 224 second) |
| Buff types | 95 |
| Status effects | 10 |
| Items in DB | 6,169 |
| Cards | 538 |
| Monster templates | 509 (**~183 now have spriteClass**, ~150 sprites rendered, 4 with `[min,max]` respawn ranges) |
| Active spawns | **`spawnPool` system live** — prt_south migrated off 195-line hand grid (10 templates × proportional count). grassfield05 (6 templates), grassfield07 (4 templates), SewerDungeon01 (4 templates × 200 total). High-level prt_south enemies still commented out 04-27 (restorable) |
| Zones | **7** (prontera, prontera_south, prt_north, prt_dungeon_01, **+grassfield05**, **+grassfield07**, **+SewerDungeon01** as of 05-07/08) |
| DB migrations | 28 |
| VFX configs | 97 |
| Classes | 19 (6 first + 13 second) |
| Textures | 1,061 (AI + RO originals) + **~1,063 Hunyuan3D 2.1 GLBs** (235 base × ~828 color variants across 7 categories: vegetation/architecture/props/terrain/dungeon/special/centerpiece) |
| Material variants | 2,700+ |
| Decal instances | 91 |
| Scatter meshes | 135 (75 V1 + 60 V3) |
| Environment scripts | **150+** (+35 sprite-quality migration 04-27 + ~15 hunyuan import + multi-object detect + flatten + tune + verify + collision + spawn region exporter) |
| `_tools/` Hunyuan3D scripts | **13** master/orchestrator/cleanup/variants/regen + supervisor + 5 diagnostic |
| Audio files | 121 BGM tracks + 626 ROSFX |
| Enemy 3D models | ~205 GLBs (Tripo3D) |
| Enemy atlas configs | 191 `_v2.json` files |
| Enemy atlas folders (in UE5) | 162 in `Body/enemies/` |
| UE5 import batch scripts | 13 (`import_batch3.py` through `import_batch13.py`) |
| Monster animation presets | 12 (blob + 10 new body types + humanoid) |
| Sprite Quality slider tiers | 5 (Ultra=0 / High=1 / Medium=2 / Low=3 / Very Low=4) |
| Class Skill Audit docs done | 13/19 (+Priest +Monk +Assassin 04-26) — Rogue / Blacksmith / Alchemist still pending |
| C++ Spawn-region actor classes | 2 (`ASpawnAllowVolume` + `ASpawnDenyVolume`) + `SpawnRegionExporter` console command |
| `AWaterArea` deployments | 1 zone (SewerDungeon01 canals — first end-to-end use of mixed-mode auto-detected deep cells) |
| NavMesh OBJ exports | 7 (prontera, prontera_north, prontera_south rebuilt 05-07, prt_dungeon_01, **+grassfield05**, **+grassfield07**, **+SewerDungeon01**) |

## What's Implemented
- [x] All 6 first classes + 13 second classes
- [x] Full combat pipeline (physical + magical)
- [x] 95 buff types + 10 status effects
- [x] Party system (21/21 RO features)
- [x] Card system (538/538, all deferred systems)
- [x] Dual wield (Assassin)
- [x] Combo system (Monk)
- [x] Performance system (Bard/Dancer, 9 ensembles)
- [x] Trap system (Hunter)
- [x] Pet + Homunculus + Companion visuals
- [x] Cart + Vending + Item Appraisal
- [x] Forging + Refining
- [x] Monster skill system (40+ NPC_ skills)
- [x] Persistent socket (survives zone transitions)
- [x] 33 C++ Slate subsystems (all BPs replaced)
- [x] Abracadabra (145 skills + 6 special effects)
- [x] Whisper, MVP, death penalty
- [x] 3D-to-2D sprite pipeline (Tripo3D+Mixamo+Blender, validated Mage+Swordsman)
- [x] SpriteCharacterActor UE5 integration (dual atlas, weapon modes, combat hooks)
- [x] Code audit pass (43 audit files across security, performance, integration, monsters)
- [x] Weapon sprite overlay system (dual-pass render, per-frame depth ordering, remote sync)
- [x] Animation standardization (17-anim standard for ALL classes, targetType-driven cast)
- [x] Archer_f + bow weapon overlay sprites (2272 total)
- [x] Skeleton enemy sprite (humanoid, Mixamo-rigged, pure C++ actor)
- [x] Poring enemy sprite (blob, shape key anims, render_monster.py)
- [x] NavMesh pathfinding implemented (server-side recast-navigation, enemy AI movement)
- [x] Hair sprite layer system (separate layer, 9-color tinting, hides_hair headgear flag, female atlas done)
- [x] Headgear sprite layer (holdout occlusion, always_front depth, multi-slot blocking, Egg Shell Hat)
- [x] Render pipeline standardized (render_blend_all_visible.py + .blend + --cel-shadow 0.92/0.98)
- [x] Shared armature system (base_m/base_f for all classes, ~5 min per new class)
- [x] Ground texture system (1061 textures, 17 material versions, 2700+ variants, ComfyUI pipeline)
- [x] DBuffer decal system (5 parent materials, 91 RO-texture instances)
- [x] Landscape Grass V3 (60 AI sprites, 13 zone GrassTypes, paintable + random placement)
- [x] 3D world building guides (5 _meta docs: landscape, materials, scatter, decals, lighting)
- [x] Map system — minimap (134x134, 5 zoom, draggable, SceneCapture), world map (12x8 grid, 62 zones), loading screen (Ken Burns, sparkles), Guide NPC marks, /where command, preferences persistence
- [x] Kafra Storage — account-shared 300-slot storage, 10 socket handlers, split/sort/auto-stack/search QoL, 40z fee
- [x] Player-to-Player Trading — 10 items + zeny, two-step confirm (OK/Trade), 13 cancel paths, atomic transfer, /trade command, trade logs
- [x] Right-click player context menu — Trade/Party Invite/Whisper, camera drag vs quick-click detection, FMenuBuilder popup
- [x] Login screen — deferred texture loading, FSlateBrush, ScaleToFill, standalone timing fix, camera 16:9
- [x] ESC menu — character select return, player:leave two-phase handler, Z=200 overlay, dead/alive states
- [x] Item icons — all 6169 items mapped to subfolder-based icon assets in InventorySubsystem
- [x] Options menu — 14 settings (Display/Interface/Camera/Gameplay), `UOptionsSaveGame` persistence, FPS overlay, ESC menu integration, 9 target subsystems wired
- [x] Character ground positioning fix — LocalSprite TWeakObjectPtr, name tag tracks sprite, fixed remote HalfHeight=96, billboard SpriteDepthOffset compensation, VFX spawn at visual center
- [x] RO Classic damage numbers — Sine arc + horizontal drift + 3.0x→0 scale shrink, per-type curves (damage/miss/heal), RO color spec (white/yellow/red/green), element tinting, status custom-color path
- [x] Hit impact system — Hit flash (3,3,3 overbright 150ms), hit particles (NS_AutoAttackHit at visual center), crit starburst (FSlateBrush), combo total (FComboTracker, multi-hit buffer), target flinch (Hit anim + enemy:move guard)
- [x] Hit sound system — First audio in project: 5 WAVs, random variant + ±5% pitch jitter, positional `PlaySoundAtLocation`
- [x] Sprite-vs-world rendering (final) — BLEND_Translucent + bDisableDepthTest + binary alpha, two camera→feet/head line traces + per-pixel depth, post-process cutout via PPI_CustomStencil, hardcoded 96 capsule, motion blur off
- [x] RO Classic audio research — full architecture (GRF/BGM/SFX trigger sources), 121 pre-renewal tracks, composer credits, legal status ($4M Gravity v. NovaRO 2022), Tier 1-3 sourcing path documented
- [x] Audio system implementation — AudioSubsystem (2847 lines), enemy SFX (60 monsters, body material layering), player SFX (per-weapon swing/hit, per-class fallback), BGM zone mapping, skill SFX (SkillImpactSoundMap), 3 audio skills created
- [x] Minimap SceneCapture2D fix — SetPostProcessMaterial(false) to exclude cutout PP from minimap capture, regression protection in 5 locations
- [x] Ground item / loot drop system — server: RO Classic ownership phases (3s/5s/7s normal, 10s/20s/22s MVP), scatter offsets, 60s despawn, party share, 6 socket events. Client: GroundItemSubsystem + GroundItemActor (733 lines), billboard sprites, tier-color tinting, click-to-pickup, spawn arc
- [x] Inventory performance analysis — 10-finding root cause report, Phase 1 optimizations started (const ref GetFilteredItems, RebuildFilteredCache, ItemDefCache TMap)
- [x] Enemy 3D model downloads — ~20 GLBs from Tripo3D (fabre, lunatic, drops, pupa, willow, condor, hornet, roda_frog, savage_babe, chonchon, creamy, mandragora, pecopeco, poison_spore, poporing, smokie, rocker, farmiliar, yoyo)
- [x] Monster animation presets — 10 new shape key presets (~1000 lines in render_monster.py): caterpillar, rabbit, egg, frog, tree, bird, flying insect, bat, quadruped, plant
- [x] Monster template spriteClass fields — ~20 templates updated with spriteClass + weaponMode
- [x] UniRig pipeline — AI rigging for non-humanoid enemies, installed + proved on rocker (40 bones, 6 anims). Skill: `/sabrimmo-rig-animate`
- [x] Blender 5.x FBX import fix — monkey-patched cast_shadow deprecation crash in blender_sprite_render_v2.py
- [x] Egg/larva enemies — made immobile (ant egg, dragon egg, pupa, thief bug egg, peco peco egg)
- [x] Enemy attack lunge system — melee wind-up→lunge→return (ServerTargetPos + timers), ranged vine decal at target feet, safe TWeakObjectPtr guards
- [x] Ground strike VFX — SpawnGroundStrikeEffect (NS_Dark_Stone_Impact, green earth tint) for ranged ground attacks like Mandragora
- [x] Render pipeline: thicken modifier (--thicken for thin geometry), lunge offset interpolation in render_all()
- [x] Mandragora fix — attackRange 200→450, aiType passive→aggressive
- [x] `spriteTint` field end-to-end — monster template → server adapter → 4 enemy:spawn broadcast paths → client parser + SetLayerTint (enables recolored variants)
- [x] Tint system scaled — plasma validates with 4 siblings (plasma_r/b/g/p), Eclipse (Lunatic variant) implemented and spawned
- [x] **150/509 enemies have sprites** — 13 production batches (04-13 through 04-21), grid spawn layout in Prontera South (Y=-11000..+1500), variant inheritance (meta_X / provoke_X / X_) auto-covers 35+ additional templates for free
- [x] EnemySubsystem client-side field plumbing — `FEnemyEntry.SpriteTint` (FLinearColor), `bCanMove` (stationary gate), `size`→sprite scale mapping (0.5x/1.0x/1.5x), `spriteScale` per-template override. Respawn re-applies tint
- [x] Server all-5-emit-path parity — `size` / `canMove` / `spriteTint` / `spriteScale` now broadcast on spawnEnemy, respawn, player:join, zone:ready, and via the `ENEMY_TEMPLATES` adapter
- [x] `.add_batchN.js` template-edit pattern — idempotent (skip-if-spriteClass-exists) JS scripts replace hand-editing 30KB of monster templates per batch. 14 batch scripts now in `_prompts/`
- [x] `render_monster.py --model-z-offset` flag — lifts mesh before render for flying creatures; preserves across lunge interpolation
- [x] Lighting defaults bumped — `--cel-shadow 0.92 --cel-mid 0.98` is now the default for both render_monster.py and blender_sprite_render_v2.py (matches RO Classic posterized look)
- [x] Ranged attack visual swap — EnemySubsystem ranged attacks now spawn a scaled-down billboard sprite burst (using attacker's sprite class) instead of the vine decal, so the effect is visible from any camera angle
- [x] **Runtime Sprite Quality slider** (2026-04-26 → 04-27) — 5 tiers (Ultra/High/Medium/Low/Very Low), `iSpriteQuality = 2` Medium default, `OptionsSubsystem::ApplySpriteQualityToLoadedTextures` iterates `/Game/SabriMMO/Sprites/Atlases/` and sets `LODBias = GlobalLODBias` on every loaded `UTexture2D`. Slider entry under Video options
- [x] **`UZonePreloadSubsystem`** (2026-04-26) — per-world subsystem, 3-tier handle system (Pinned / Active / 4 GB LRU), `RequestClassPreload` / `RequestLayerPreload` / `PinClass` / `BeginZone` / `HandleAdjacentClasses` API, async via `FStreamableManager` + `FStreamableHandle`. Hooks in `EnemySubsystem::HandleEnemySpawn` (idempotent batch preload) + `OtherPlayerSubsystem::HandlePlayerMoved` (body + hair + 6 equipment slots) + `ZoneTransitionSubsystem` (loading screen waits on preload)
- [x] **Server `zone:adjacent_classes` predictive emit** (2026-04-26) — after `zone:ready`, walks `ZONE_REGISTRY[zone].warps` and emits `{destZone: [spriteClass...]}` so client warms neighbour-zone atlases before warp transitions
- [x] **Path C deferred equipment swap** (2026-04-27) — `FSpriteLayerState::PendingLayerAtlasRegistry` + `FinalizeEquipmentSwap()`. New equipment held in pending registry until atlas streaming completes (or 1s timeout); old equipment stays visible. Eliminates pop-in on weapon/hair/headgear changes
- [x] **Helmet flicker fix** (2026-04-27) — `if (!IsValid(L.MaterialInst))` guard in `LoadEquipmentLayer`. Without it, fresh `UMaterialInstanceDynamic*` was created on every call (e.g. inventory open) causing one-frame flashes
- [x] **Canonical sprite atlas settings** (2026-04-27) — single source of truth at `_meta/settings for importing sprite atlases.md` + memory `feedback-sprite-texture-group-ui.md`: BC7 + TEXTUREGROUP_UI + TF_NEAREST + TMGS_SIMPLE_AVERAGE + use_new_mip_filter + do_scale_mips_for_alpha_coverage + alpha_coverage W=0.5 + max_texture_size=0 + never_stream=False + **srgb=True** (sprite material samples as Color/sRGB; False renders invisible). All 35+ migrate / import scripts + 11 docs + 4 prompts synced. `migrate_test_verify.py` enforces canonical settings + multi-mip presence
- [x] **Homunculus end-to-end re-audit** (2026-04-26) — 7/10 gaps fixed, partial G2 (Lif autocast only), 2 deferred. DB persistence on skill_up, server position tick (200ms, 600 UE/s + Urgent Escape mult), per-type sprite swap (Lif=poring / Amistr=poporing / Filir=picky / Vanilmirth=drops), attack-target validation, 8 new client widget buttons, remote rendering via `homunculus:other_summoned/dismissed/position` + `RemoteHomActors` map
- [x] **Priest / Monk / Assassin skill audit docs** (2026-04-26) — `docsNew/05_Development/Priest_Skills_Audit_And_Fix_Plan.md` + `Monk_Skills_Audit_And_Fix_Plan.md` + `Assassin_Skills_Audit_And_Fix_Plan.md` + small `ro_skill_data_2nd.js` + `ro_buff_system.js` corrections
- [x] **Sprite material foliage occlusion** (2026-04-29) — `FoliageOcclusionDist` scalar parameter (default 80 UU) on the sprite material; small occluders (grass < 80 UU thick) discard sprite pixels, walls (≥ 80 UU thick) still trigger the line-trace silhouette path
- [x] **Spawn Region System** (2026-05-02) — `ASpawnAllowVolume` + `ASpawnDenyVolume` placeable actors with `MonsterFilter` whitelist + `RegionTag`. `ExportSpawnRegions <Zone>` console command writes `server/spawn_regions/<zone>.json`. Server `ro_spawn_regions.js` picks weighted-random points (∝ XY area), rejects deny-box overlaps, NavMesh ground-snaps. `enemySpawns[]` and `spawnPool[]` coexist
- [x] **Hunyuan3D 2.1 production asset pipeline** (2026-05-03 → 05-06) — 235 base + ~828 color variants = ~1,063 game-ready GLBs across 7 categories (vegetation 54, architecture 38, props 61, terrain 34, dungeon 29, special 12, centerpiece 7). Multi-input mandatory (2 strategies × 2 seeds), decimate-FIRST then texture (30× speedup), quality-aware scoring (`cube_artifact = 0.001`), 8-attempt reroll loop, backup OUTSIDE asset_dir, ComfyUI supervisor + UTF-8 logger patch + KJNodes pinned to commit `37a0973`. 96% GOOD after 3 cleanup passes. Master pipeline `_tools/hunyuan_asset_pipeline.py` 48 KB, `3D_World_Asset_Pipeline_2026.md` 1,460 lines, `Session_Record_2026-05-03.md` + `REUSABLE_SESSION_PROMPT.md`
- [x] **AWaterArea mixed-mode water system** (2026-05-06) — auto-detected per-cell deep via downward raycast grid (default 16x16 = 256 samples), greedy rect-merge → 1 `UBoxComponent` per rectangle as `NavArea_Null` modifier (cuts navmesh AND server-side OBJ), three-stop depth gradient (shallow [0-250 UU] → deep → abyss [900 UU, opacity 1.0]), runtime material via standard `UMaterialExpression*` nodes (NOT custom HLSL — confirmed not to animate), reference-counted server tracking via `Map<areaId,{isDeep}>`, two-layer movement validation (deep AABB + off-navmesh), `spawnEnemy()` snap-and-warn
- [x] **Server `spawnPool` integration** (2026-05-07) — `index.js` `spawnZoneEnemies(zoneData, zoneName)` called from all 7 zone-first-load paths. prt_south migrated off the 195-line hand-placed grid → proportional spawnPool. Two new field zones grassfield05 (6 templates) + grassfield07 (4 templates incl. `vocal × 1` rare named with 15-25 min respawn). `_journal/Session Tracker.md` "World Building / Zones" section
- [x] **`respawnMs` per-template ranges** (2026-05-07) — `respawnMs: [min, max]` array form for non-MVP rare named / farming spawns. `resolveRespawnMs()` helper at `index.js:~100`, used at status-event respawn (~L2793) + combat-death respawn (~L28799). 4 templates migrated: blue_plant + green_plant `[300000, 600000]` (5-10 min), black_mushroom `[120000, 360000]` (2-6 min), vocal `[900000, 1500000]` (15-25 min). MVP/boss variance untouched (still hardcoded in death handler). Memory `feedback-respawn-range-syntax.md`
- [x] **Sewer Dungeon F1 server-side scaffold + build plan** (2026-05-07/08) — `SewerDungeon01` registered in `ZONE_REGISTRY` with full RO Classic flag set (indoor:true, nosave:true, noteleport:false, noreturn:false). 200-spawn pool (thief_bug × 50 + thief_bug_egg × 120 + farmiliar × 15 + tarou × 15). `prtsouth_to_dungeon` portal repurposed → `prtsouth_to_sewerdungeon01`. `ZONE_INFO` + `WORLD_MAP_GRID` row 5 col 8. AudioSubsystem ambient stack (DunWind+CaveDrip+CaveFall) + `bgm_19 "Under the Ground"`. PostProcessSubsystem zone preset (vignette 0.5, bloom 0.15, exposure 0, white temp 5500K, cool blue-green tint) + fog override (density 0.30, falloff 0.2, moss-green inscatter, FogHeight=500). NavMesh + spawn region exported. `Sewer_Dungeon_01_Build_Plan.md` 82 KB / 20 sections / 14 phases / 5 appendices
- [x] **ZoneTransition preload-wait** (2026-05-08) — `WaitForPreloadAndHideOverlay` polls `UZonePreloadSubsystem::IsLoadingInProgress` every 200ms, dismisses overlay after 700ms of zero in-flight + 1.5s minimum + 15s hard timeout. `ShowExpectedZoneChange` / `HideExpectedZoneChange` API for Kafra / Butterfly / RequestWarp optimistic overlay. Fixes the 04-26 sprite-quality work — without preload-wait, loading screen dismissed mid-streaming → pop-in storm
- [x] **Directional shadow cap** (2026-05-08) — `SetDynamicShadowDistanceMovableLight(7000.f)` + 3 cascades + tuned distribution (exponent 3.0, transition 0.2, fadeout 0.25) to fight 15° top-down FOV cascade seams in all outdoor zones
- [x] **Per-zone PostProcess presets for grassfield07 / grassfield05** (2026-05-08) — outdoor field tuning (warmer / cooler highlight tints, 6700/6400 K white temp)
- [x] **Canonical bootstrap prompt for new zones** (2026-05-08) — `_prompts/new_zone_creation.md` 22 KB, self-contained starter that loads skills + reads SewerDungeon01 plan + reads journal + asks 2-4 clarifying questions before generating per-zone build plan
- [x] **Job Master NPC + in-game class change** (2026-05-09 → 05-10) — `AJobChangeNPC` (antonio sprite atlas) + `UJobChangeSubsystem` (Z=26) + `SJobChangeWidget` (centered + draggable, RO brown/gold, 3-page `SWidgetSwitcher`: Greeting / Selection / Congrats). Server `index.js` `job:change` handler now gates on dead/sit/combat/overweight + refills HP/SP to new max + persists. Client respawns local sprite (`SpawnSpriteForClass`) + emits `skill:data` to refresh skill tree. `UOtherPlayerSubsystem::HandleRemoteJobChanged` respawns peer sprites in zone (`ResetHairHiding` → `EquipVisuals` reload → `SetHairStyle` → `ReconcileHairVisibility`). `MMOGameInstance::SetSelectedCharacterJobClass` keeps cached character data in sync. `tests/unit/job_change.test.js` — 31 tests (data integrity, client mirror parity, eligibility tree, transitions, server gating). 05-10 follow-up fix: live click handler is `UPlayerInputSubsystem::ProcessClickOnNPC` (4 cast sites), NOT `TryInteractWithNPC` (dead code) — silent-click gotcha now in `feedback-npc-click-pipeline.md`

## What's Next

### In-Game Testing Pass
**Class skills to test:**
- [x] Sage (IDs 1400-1421)
- [x] Hunter (IDs 900-917)
- [x] Bard (IDs 1500-1537)
- [x] Dancer (IDs 1520-1557)
- [x] Priest (IDs 1000-1018) — audit doc landed 2026-04-26
- [x] Monk (IDs 1600-1615) — audit doc landed 2026-04-26
- [x] Assassin (IDs 1100-1111) — audit doc landed 2026-04-26
- [ ] Rogue (IDs 1700-1718)
- [ ] Blacksmith (IDs 1200-1230)
- [ ] Alchemist (IDs 1800-1815) — Homunculus re-audit landed 2026-04-26 (appended to existing plan), broader Alchemist class audit still pending

**Sprite-Quality Slider — Validation (NEW 2026-04-27, still open)**
- [ ] UE5 rebuild and end-to-end slider test at every tier (Ultra / High / Medium / Low / Very Low) — measure VRAM curve + visual quality on busy zones
- [ ] Re-import the 4 sRGB-broken enemy atlases (ambernite / parasite / plasma / rafflesia) — currently invisible until re-imported with `srgb=True`
- [ ] `migrate_test_verify.py` post-rebuild — should report all-green
- [ ] `zone:adjacent_classes` end-to-end test — verify warp-portal transitions are faster than cold zone enter
- [ ] `ZonePreloadSubsystem` 4 GB LRU budget calibration — measure on 16 GB GPU at every quality tier
- [ ] Path C / helmet flicker validation — equip / unequip / open inventory rapidly, confirm no flashes
- [ ] Restore high-level enemies in `prt_south` after testing wraps (currently commented out in `ro_zone_data.js`)
- [ ] **Big commit triage** — ~98 files in working tree since `56dce91` (04-26). Splits: `feat(sprite-quality)` + `chore(scripts)` + `docs(canonical-settings)` + `feat(spawn-region)` + `feat(water-system)` + `feat(zones)` (grassfield05/07/SewerDungeon01) + `feat(hunyuan3d)` (large — own commit) + `chore(journal)` (12 daily logs)

**Sewer Dungeon F1 — Build Phases (NEW 2026-05-08)**
- [x] Phase 12.1 — server scaffold (registry / world map / audio mappings) — committed 05-07
- [x] PostProcess preset + fog override — landed 05-08
- [x] AWaterArea canals placed in `L_SewerDungeon01` — confirmed by user 05-08
- [x] NavMesh + spawn region exported — 05-08 01:49
- [ ] UE5 rebuild and first walkthrough — confirm all per-zone settings work end-to-end
- [ ] Greybox geometry replacement — sewer brick modular meshes
- [ ] `MI_DungeonFloor_Wet` + `MI_DungeonWall_Mossy` material instances (`Scripts/Environment/create_sewer_dungeon_mis.py`)
- [ ] 6 sewer slime decal MIs (`Scripts/Environment/create_sewer_decal_instances.py`)
- [ ] 10 missing Hunyuan3D assets per `Sewer_Dungeon_01_Build_Plan.md` Appendix B
- [ ] Brazier Point Lights + ceiling Spotlight grate shafts placed in level
- [ ] AWarpPortal `WarpId` rename in `L_PrtSouth`: `prtsouth_to_dungeon` → `prtsouth_to_sewerdungeon01`
- [ ] 28-test test matrix (`Sewer_Dungeon_01_Build_Plan.md` Section 13)

**Spawn Pool Validation (NEW 2026-05-07)**
- [ ] Place SpawnAllowVolume actors in `L_GrassField05` + `L_GrassField07` + run `ExportSpawnRegions <zone>` for each (JSONs don't exist yet, server warns "spawnPool but no spawn_regions/<zone>.json — random spawns skipped")
- [ ] `vocal` rare-named spawn rate test (15-25 min respawn, single instance at a time)
- [ ] Black mushroom + plant farmability test (verify sparse spawns are still findable)
- [ ] prt_south enemy density tuning — random vs old hand-placed grid play-test

**Hunyuan3D Assets (NEW 2026-05-03 → 05)**
- [ ] Verify batch 3 final asset count + variant count after queue completes (~20 hours from 05-03 23:23)
- [ ] Triage v3 regional output (Moscovia / Umbala / Eclage prompts) — first quality pass on regional outputs
- [ ] Decide regional priority order (Moscovia and Eclage most visually distinct)
- [ ] Diversion VCS check-in for the ~6-8 GB asset library
- [ ] Place Hunyuan asset waves in `L_PrtSouth` + `L_GrassField05/07` (most of the library is unused so far — only SewerDungeon01 will draw from it directly)
- [ ] 4 stubborn multi-object assets — keep with `largest_component_only` artifact, or drop entirely?
- [ ] OGH/Geffenia palette safety filter test (demonic prompts may trip SDXL filters)

**AWaterArea Follow-ups (NEW 2026-05-06)**
- [ ] First end-to-end test in `L_SewerDungeon01` — verify auto-detect deep + nav cuts + 3-stop gradient
- [ ] OBB server-side check for rotated water actors (currently AABB only — looser than visual + navmesh shape for rotated actors)
- [ ] Splash particles, wading sound, caustics, vertex displacement (all explicitly deferred)
- [ ] Per-zone default water tint (sewer = greenish-black, ocean = blue, swamp = muddy — currently every AWaterArea defaults to clean blue)

**Sprite Material Validation (NEW 2026-04-29)**
- [ ] Validate foliage occlusion threshold in-game — walk through prt_south V3 grass, confirm sprites disappear behind clumps but still get the silhouette behind walls. Tune `FoliageOcclusionDist` if grass blades are too tall to hide at 80 UU

**Homunculus follow-ups (NEW 2026-04-26)**
- [ ] Skill-point persistence regression test — connect, allocate, hard-kill server, restart, verify
- [ ] Remote homunculus rendering test — two clients, verify other player sees the trailing pet
- [ ] Lif autocast smoke test — drop owner HP < 50%, confirm 2s heal tick + cooldown gate
- [ ] Per-type homunculus AI design pass (Amistr / Filir / Vanilmirth)
- [ ] Per-type homunculus dedicated sprites (currently using poring/poporing/picky/drops as placeholders)

**Systems to test:**
- [x] Account creation (register, login, character create/delete, JWT auth)
- [x] Party (EXP share, chat, HP sync, invites)
- [ ] Dual wield (Assassin left-hand, per-hand cards, mastery penalties)
- [ ] MVP (announcements, rewards, slave spawning)
- [ ] Pets (taming, feeding, hunger/intimacy, bonuses)
- [ ] Homunculus (combat, skills, evolution, feeding)
- [ ] Refining (success rates, overupgrade bonus, safe limits)
- [ ] ASPD potions (delay reduction, stacking, class restrictions)

### VFX Bugs & Remaining Work
See [Skill_VFX_Execution_Plan](../docsNew/05_Development/Skill_VFX_Execution_Plan.md) for full plan (Phases 6/7/11 still pending).

**Bugs to fix:**
- [ ] Frost Diver VFX never wears off (persists after debuff expires)
- [ ] Frozen/Stone Cursed enemies keep walking to previous target instead of stopping immediately
- [ ] Frost Bolt and Fire Bolt missing impact VFX on hit
- [ ] Fireball not visible to other clients (should show projectile from caster to enemy on all screens)
- [ ] Soul Strike projectiles don't travel all the way to the enemy
- [ ] Stone Curse reuses Frost Diver's Cascade effect (need different VFX, can't control color)

**Previously fixed (done):**
- [x] Provoke uses P_Enrage_Base, 1s flash above enemy head
- [x] Cold Bolt/Fire Bolt: projectile destroyed on arrival, 1 projectile per spell level, spawns even if enemy dies
- [x] Lightning Bolt persists longer (was disappearing too fast)
- [x] Frost Diver VFX lasts full debuff duration
- [x] Fire Ball: single projectile → target → single explosion
- [x] Soul Strike: 1 projectile per level, starts at player, stops at enemy (bolt+fireball hybrid)
- [x] Thunderstorm loops for full spell cycle, scales with skill level
- [x] Stone Curse VFX shows (behaves like Frost Diver)
- [x] Fire Wall displays at ground-click location (like Safety Wall targeting)
- [x] Skills don't trigger auto-attacks (separate trigger system)
- [x] Fire Wall damage/knockback matches RO Classic (not instant kill)
- [x] Safety Wall and Fire Wall properly expire after duration
- [x] Magnum Break ground-targeted VFX even with no enemies nearby
- [x] Endure VFX visible to other players
- [x] Casting circles shown to other players
- [x] AoE ground target indicator matches RO Classic
- [x] Magnum Break target circle sized correctly
- [x] First Aid shows green healing numbers
- [x] Buff system persists across zone transitions

### Sprite Pipeline — Next Steps
- [x] Validate all Swordsman atlas animations working in-game
- [x] Standardize animation set per class — **17-anim standard for ALL classes** (replaces old fighter/mage split)
- [x] **Pack mage_f atlases** (2336 sprites → per-animation PNGs, validated in UE5)
- [x] **Weapon sprite overlay system** (dual-pass render, depth ordering, dagger on swordsman validated)
- [x] **Clean up mage animations** → unified to 17-anim standard
- [x] **Clean up variants logic** in C++ (simplified for 17-anim standard)
- [x] **Archer_f + bow weapon overlay** (2272 sprites, bow .blend template, Bow weapon mode in C++/server)
- [x] **Monster sprite — humanoid** (Skeleton: Mixamo-rigged, pure C++ sprite actor)
- [x] **Monster sprite — non-humanoid** (Poring: shape key anims via render_monster.py, --model-rotation -90)
- [x] **Enemy sprite integration** (EnemySubsystem: sprite enemies as pure C++ actors, no BP_EnemyCharacter)
- [x] **Re-render swordsman_m + mage_f** with new 17-animation standard set
- [x] **UniRig pipeline** — AI rigging for non-humanoid enemies (proved on rocker, 40 bones, 6 anims)
- [x] **10 monster animation presets** — caterpillar, rabbit, egg, frog, tree, bird, flying insect, bat, quadruped, plant (~1000 lines in render_monster.py)
- [x] **~20 enemy 3D models** downloaded from Tripo3D (fabre through yoyo)
- [x] **spriteClass + weaponMode** added to ~20 monster templates
- [x] **Render enemy sprite atlases** — rocker + 149 others complete; **150 / 509 monsters have sprites** (13 production batches through 04-21). 104 more have GLBs ready for render
- [ ] **Drain the 104 `pending (GLB ready)` pool** — see `_prompts/enemy_sprite_session_resume.md` for the queue. Next candidates: Lv 22-28 goblin bloc (goblin_1..5, orc_warrior, orc_zombie, smoking_orc, orc_xmas, munak, zenorc), then Lv 31-40 (archer_skeleton, kobold_1..3, kobold_archer, mummy, verit, jakk, bon_gun, wootan_shooter/fighter, ghoul, marduk, steam_goblin, zipper_bear, etc.)
- [ ] **More weapon .blend templates** (sword, staff, rod, katar, spear, mace, axe — dagger + bow done)
  - [x] Figure out weapon positioning across different classes/genders
- [x] **NavMesh pathfinding** — implemented server-side (recast-navigation, de-aggro, all movement patched)
- [ ] Equipment layer system — headgear + hair DONE, shield + garment remaining
- [ ] Scale production: batch remaining classes through Tripo3D+Mixamo+render
- [x] Hair color tint system (9 RO colors, material TintColor multiply)
- [x] Headgear sprite layer (holdout occlusion, Egg Shell Hat)
- [ ] Male hair style 1 atlas (female done, male not yet)
- [x] `spriteScale` override + `canMove` gate + billboard ranged attack — enemy visual polish systems complete

### 3D World — Next Steps
- [ ] Apply materials and decals to Prontera zone (first complete zone)
- [ ] Posterized lighting (top improvement from UE5 material research)
- [ ] Material Parameter Collection for global tint/time-of-day
- [ ] Test grass density at scale (performance check)
- [ ] Build additional zone landscapes (Payon, Geffen, Morroc, etc.)

### Audit Follow-ups
- [ ] Review audit results in `_audit/` and triage fixes (43 files, 2 master reports)

### Map System — Next Steps
- [x] Commit map system code — committed in `60eca6c`
- [ ] Generate world map illustration (prompt ready: `_prompts/world_map_generation_prompt.md`)
- [ ] Generate loading screen art (12 prompts ready: `_prompts/loading_screen_generation_prompts.md`)

### Economy Systems
- [x] Kafra Storage — implemented 2026-04-03 (prompt: `_prompts/implement_kafra_storage.md`)
- [x] Player-to-Player Trading — implemented 2026-04-03 (prompt: `_prompts/implement_player_trading.md`)
- [x] Right-click context menu on other players — implemented 2026-04-04

### UX / Polish
- [x] ESC → character select screen — implemented 2026-04-04
- [x] Login screen background + resolution sizing — implemented 2026-04-04
- [x] Item icon mapping (6169 items) — implemented 2026-04-04
- [x] Right-click context menu on other players — implemented 2026-04-04
- [x] Fix character floating off ground / terrain clipping when moving north — implemented 2026-04-05
- [x] Attack impact effects / damage number improvements — implemented 2026-04-05 (polish needed: starburst visibility, target flinch verification, better hit particle)
- [x] Options menu (14 settings, SaveGame persistence, FPS overlay) — implemented 2026-04-05
- [x] Crit starburst — opacity/scale/saturation tuned for readability — 2026-04-06
- [x] Target flinch — verified end-to-end (Hit anim atlas confirmed, PlayHitAnimation called, enemy:move guard firing) — 2026-04-06
- [x] Replace auto-attack hit particle — `NS_AutoAttackHit` swapped for punchier Niagara from `/Game/Variant_Combat/VFX/` — 2026-04-06
- [x] Sprite-vs-world rendering (silhouette behind walls, no clipping into nearby geometry) — implemented 2026-04-06

### Audio System (implemented 2026-04-07)
- [x] Deep research on RO Classic audio (architecture, BGM tracklist, SFX inventory, legal status, sourcing tiers) — `docsNew/05_Development/RO_Audio_System_Research.md`
- [x] AudioSubsystem built (2847 lines) — enemy body material SFX, player per-weapon swing/hit, per-class fallback, BGM zone mapping, skill SFX (SkillImpactSoundMap), 60 monsters wired, 121 BGM tracks sourced
- [x] Fix `/sabrimmo-audio` skill — removed Tobias Marberger misattribution. 3 new audio skills created (enemy/player/combat)
- [ ] Audio testing — verify all SFX (enemy, player, BGM, skill) in-game
- [ ] Source Tier 1 placeholder audio (Sonniss GDC 2026 + Kenney CC0 + OpenGameArt) ahead of commissioned tracks

### Ground Item / Loot Drop System (implemented 2026-04-09 to 2026-04-10)
- [x] Server: RO Classic ownership phases, scatter offsets, 60s despawn, party share, damage ranking, 6 socket events
- [x] Client: GroundItemSubsystem + GroundItemActor (733 lines), billboard sprites, tier-color tinting, click-to-pickup, spawn arc
- [ ] End-to-end testing (kill monster → items drop on ground → pick up → inventory)
- [ ] Drop rate table verification against rAthena

### Inventory Performance (analysis done 2026-04-10, optimization in progress)
- [x] Root cause report: 10 cascading bottlenecks (2 DB queries + 164KB transfer + 350 widget rebuilds per potion use)
- [x] Phase 1 started: const ref GetFilteredItems, RebuildFilteredCache, ItemDefCache TMap
- [ ] Phase 1 remaining: fix GetItemAtSlot lambdas, add TMap<InventoryId, Index>
- [ ] Phase 2: Delta widget update instead of full rebuild
- [ ] Phase 3: Strip descriptions from server payload (164KB → ~20KB)
- [ ] Phase 4: Incremental weight + itemDef Map on server

### Hosting / Packaging / Distribution (research complete 2026-04-16, implementation pending)
- [x] Deep research pass — 4 deliverable docs authored (~153 KB total). See `docsNew/05_Development/Hosting_Packaging_Distribution_Plan.md` for the master plan + 3 companion docs
- [ ] Implement endpoint config per `Client_Endpoint_Config_Design.md` — `SABRI_DEFAULT_MASTER_URL` define in Target.cs, CLI parse in `UMMOGameInstance::Init`, `CustomServerUrl` UPROPERTY on `UOptionsSaveGame`, advanced panel entry in `SOptionsWidget`
- [ ] Swap `http://` → `https://` and `ws://` → `wss://` in URL assembly for Shipping, keep `http://localhost` dev fallback
- [ ] Add `version` field to server `/health` response (currently only `status`/`timestamp`/`message`); client fetches + compares before socket connect; mismatch UX blocks login
- [ ] Verify UE5.7 SocketIOClient plugin speaks WSS, or rebuild with WSS enabled
- [ ] First local Shipping build validation (package → run with `-server=localhost:3001` → verify login + zone + combat)
- [ ] Register domain (check `sabrimmo.com` at Cloudflare Registrar, $10.46/yr)
- [ ] Create Tailscale account + tailnet (Scenario A — 5 friends prereq)
- [ ] Add `server/.env.example` with every required variable (currently absent; runbooks assume it)
- [ ] Set up itch.io Restricted page + install `butler` CLI for bulk-key delivery
- [ ] **NEVER** use: Steam (RO IP exposure, Dreadmyst 2026 NCsoft precedent), AWS/Azure/GCP (egress cliff + burstable throttle), Upstash PAYG (potential $13K/mo bill on 20 Hz tick × 100 CCU), Neon scale-to-zero (cold-start freezes combat)

### Remaining Features
- [ ] Client-side homunculus actor + position broadcast
- [ ] PvP system
- [ ] War of Emperium / Guild system
- [ ] Quest system
- [ ] Additional zones (Payon, Geffen, Alberta, etc.)
- [ ] Niagara VFX build (Phases 6/7/11 pending — 97 configs defined, assets mapped)
- [ ] Guild storage + trading
- [ ] More NPC shops per zone

## Recent Sessions
<!-- Add links to session logs here, newest first -->
- **2026-05-10** — **Canonical pre-renewal compliance audit (100%) + character starter kit + early-game tuning**. Cloned rAthena master, built `_audits/` infrastructure (5 extract/audit scripts, 5 fix scripts, JSON canonical references for 1,004 mobs + 6,169 items). Fixed monster damage formula (`isMonsterAttack` flag — was doubling Poring damage from 8→17 vs Novices). Stat point off-by-one (was overpaying +19 over 1-99). Refine fees Wlv3 5000z→1000z, Wlv4 20000z→2000z. New character INSERT now seeds Prontera town spawn + starter kit (1000z + Knife/Cotton Shirt/Sandals equipped + 25 Red Potions hotbar 0 + 5 Fly Wings + 1 Butterfly Wing). All 509 monsters: 0 stat findings, 0 mode findings, 0 drop findings vs canonical. Server `SERVER_RATES.EXP_RATE_MULTIPLIER` constant (default 1) wired into both award sites for easy rate scaling. Drop entries now use explicit `itemId:` to avoid display-name collisions (Knife=1201/1202/1203). 4 new memory files + 5 skill updates.
- **2026-05-08** — **Sewer Dungeon F1 build plan + ZoneTransition preload-wait + new_zone_creation prompt**. `Sewer_Dungeon_01_Build_Plan.md` 82 KB / 20 sections / 14 phases. PostProcessSubsystem: SewerDungeon01 zone preset (vignette 0.5, bloom 0.15, cool blue-green) + fog override (density 0.30, falloff 0.2, moss-green) + grassfield07/05 presets + 7000-UU directional shadow cap to fight 15° FOV cascade seams. ZoneTransitionSubsystem: `WaitForPreloadAndHideOverlay` polls `UZonePreloadSubsystem::IsLoadingInProgress` every 200ms (700ms stability + 1.5s minimum + 15s timeout) — fixes 04-26 sprite-quality work that dismissed loading screen mid-streaming. `ShowExpectedZoneChange`/`HideExpectedZoneChange` API for Kafra/Butterfly/RequestWarp optimistic overlay. NavMesh + spawn region exported for SewerDungeon01. Exit warp radius 200→250 + position adjusted for cramped sewer geometry. `_prompts/new_zone_creation.md` 22 KB canonical bootstrap. Journal catch-up for 04-28 → 05-08 (11-day gap). ([daily note](2026-05-08.md))
- **2026-05-07** — **Server `spawnPool` integration** + grassfield05/07 + Sewer Dungeon scaffold + respawn ranges + water-state wiring. `index.js` +402/-60 lines: `resolveRespawnMs` (number OR `[min,max]` range), `spawnZoneEnemies` from all 7 zone-first-load paths, `isInDeepWater` AABB, `water:enter`/`water:exit` ref counting via `Map<areaId,{isDeep}>`, two-layer movement validation (deep-water AABB + off-navmesh), `spawnEnemy()` snap-and-warn (>100 UU snap = designer typo warning). Per-template respawn ranges (blue_plant/green_plant `[300000, 600000]`, black_mushroom `[120000, 360000]`, vocal `[900000, 1500000]`) — MVP/boss variance untouched. NavMesh exports for grassfield05 (11:37) + prt_south rebuild (11:45) + grassfield07 (16:04). prt_south migrated off 195-line hand-grid → proportional `spawnPool`. `prtsouth_to_dungeon` repurposed → `prtsouth_to_sewerdungeon01`. AudioSubsystem zone mappings for prt_sewb1-4 + SewerDungeon01 (DunWind+CaveDrip+CaveFall + bgm_19). Filed memory `feedback-respawn-range-syntax.md`. ([daily note](2026-05-07.md))
- **2026-05-06** — **`AWaterArea` mixed-mode water system**. Auto-detected per-cell deep via downward raycast grid (default 16x16 = 256 samples), greedy rect-merge → 1 `UBoxComponent` per rectangle as `NavArea_Null` modifier (cuts navmesh + server OBJ). Three-stop depth gradient: shallow (0-250 UU) → deep → abyss (900 UU, opacity 1.0). Material via standard `UMaterialExpression*` nodes (NOT custom HLSL — confirmed not to animate at runtime). `Water_System_Research.md` "Implementation Status" addendum documenting deviations + extensions vs original research. Hunyuan alt-mesh regen (`hunyuan_alt_meshes_regen.py` 11 KB) + flatten v3 + flatten_combined. Memory `water-system-mixed-mode.md`. ([daily note](2026-05-06.md))
- **2026-05-05** — **RO Classic World Assets Expanded** + **v3 regional Hunyuan generator**. `_meta/RO_Classic_World_Assets_Expanded.md` 15 KB cataloging 9 niche regions (Umbala / Moscovia / Rachel-Veins / Eclage / Brasilis-Dewata-Malaya / Old Glast Heim-Geffenia / Jawaii / Yuno Sage Academy) with per-region asset checklists. `_tools/hunyuan_v3_regional.py` 140 KB — per-region asset definitions + regional palette overrides + lower 5-attempt reroll budget (vs 8). Light day, mostly autonomous queued runs. ([daily note](2026-05-05.md))
- **2026-05-04** — **Hunyuan continuation: multi-object detection + regen v2 + UE5 import**. `Session_Record_2026-05-03.md` 24 KB (the canonical multi-day record) + `REUSABLE_SESSION_PROMPT.md` 7 KB filed at 00:27. `find_multi_object_assets.py` + `find_multi_object_v2.py` (centroid spread + bbox overlap heuristics). `hunyuan_v2_regen.py` strengthens single-object negatives + 8-seed sweep + largest-component-only. `flatten_to_final_v2.py` for UE5 import layout. `import_hunyuan_assets.py` + `import_hunyuan_v2_assets.py` + tuning helpers. `prontera_south.json` first real-world `ExportSpawnRegions` output (22:14). ([daily note](2026-05-04.md))
- **2026-05-03** — **Hunyuan3D 2.1 production pipeline build + 135 batch 1 assets**. Master pipeline `hunyuan_asset_pipeline.py` 48 KB: multi-input mandatory (2 strategies × 2 seeds), decimate-FIRST then texture (30× speedup), quality-aware scoring (cube_artifact=0.001), reroll loop with 8 cycle suffixes, backup OUTSIDE asset_dir. `comfyui_supervisor.py` + UTF-8 logger patch (prevents cp1252 crash on custom node logs). KJNodes pinned to commit `37a0973`. Batch 1: 135 assets across 7 categories, **96% GOOD** after 3 cleanup passes. Color variants: ~470 (`hunyuan_color_variants.py` 65 KB). Batch 3 queued: 100 gap-filling assets (`hunyuan_batch3_100.py` 44 KB). 6 inherent-bad assets accepted (thin shapes the AI fundamentally can't make volumetric). `3D_World_Asset_Pipeline_2026.md` ~1,460 lines. ([daily note](2026-05-03.md))
- **2026-05-02** — **Spawn Region System** (visual spawn-area editor). `ASpawnAllowVolume` + `ASpawnDenyVolume` placeable actors (green/red wireframe boxes), `MonsterFilter: TArray<FString>` whitelist + `RegionTag` label. `ExportSpawnRegions <Zone>` console command writes world-space AABBs to `server/spawn_regions/<zone>.json`. Server `ro_spawn_regions.js` 8.9 KB: weighted-random by XY area, deny-box rejection, NavMesh ground-snap (500 UU halfExtent on Z so vertical box positioning doesn't matter). `Spawn_Region_System.md` 16 KB. Late evening: Hunyuan3D 2.1 single-asset proof (test tree, depth ratio 0.97, confirmed `mc_algo="mc"` mandatory). ([daily note](2026-05-02.md))
- **2026-05-01** — Day off. ([daily note](2026-05-01.md))
- **2026-04-30** — Day off. ([daily note](2026-04-30.md))
- **2026-04-29** — Single 02:00 sprite material edit. `SpriteCharacterActor::CreateSpriteMaterial` adds `FoliageOcclusionDist` scalar parameter (default 80 UU): small occluders < 80 UU thick discard sprite pixels; walls ≥ 80 UU still trigger the line-trace silhouette path. Side trip from the 04-27 main queue. ([daily note](2026-04-29.md))
- **2026-04-28** — Day off. ([daily note](2026-04-28.md))
- **2026-04-27** — **Sprite-Quality Slider final wiring** + project-wide canonical-settings migration. Slider lands end-to-end (Ultra/High/Medium/Low/Very Low, LODBias 0-4, `OptionsSubsystem::ApplySpriteQualityToLoadedTextures`). Mass migration: `MaxTextureSize=0` (12 `clear_maxsize_*` scripts to fix non-square atlas downscale), `NeverStream=False` (18 `enable_streaming_*` scripts so slider can actually free VRAM). Path C deferred equipment swap + helmet flicker `IsValid(MaterialInst)` guard. **sRGB=True is mandatory** (4 enemies invisible until corrected via `check_srgb.py` diff). 11-doc + 4-prompt skills sweep + canonical settings doc at `_meta/settings for importing sprite atlases.md`. Filed `_prompts/sprite_quality_canonical_settings_resume.md` as next-session re-entry. Journal catch-up for 04-23 / 04-24 / 04-25 / 04-26 / 04-27 (5-day gap). ([daily note](2026-04-27.md))
- **2026-04-26** — TRIPLE-STREAM big day. (1) Knuckle weapon texture diagnosis (00:45 — `diag_weapon_textures.py` + `fix_knuckle_match_working.py`, precursor to canonical-settings work). (2) Class skill audit closures: Priest / Assassin / Monk audit docs + `ro_skill_data_2nd.js` + `ro_buff_system.js` corrections (3/6 remaining now done). (3) Big git commit `56dce91` — flushes 7-day backlog (~190 atlas configs, 14 batch scripts, 14 weapon scripts, hosting/packaging research, journal catch-up 04-15 → 04-22). (4) **Homunculus end-to-end re-audit** — 7/10 gaps fixed (DB persistence, server position-tick, sprite swap, target validation, 8 widget buttons, remote rendering), partial G2 (Lif autocast only), 2 deferred. (5) **Sprite-Quality system architecture** late evening — `ZonePreloadSubsystem.h/.cpp` (3-tier handles, 4 GB LRU, async via `FStreamableManager`), hooks in EnemySubsystem + OtherPlayerSubsystem + ZoneTransitionSubsystem, server `zone:adjacent_classes` predictive emit. ([daily note](2026-04-26.md))
- **2026-04-25** — Day off. ([daily note](2026-04-25.md))
- **2026-04-24** — Single one-line `ro_monster_templates.js` tweak at 23:39. No Claude session traces. ([daily note](2026-04-24.md))
- **2026-04-23** — Day off. ([daily note](2026-04-23.md))
- **2026-04-22** — Tail-end imports (alligator / blazzer / stone_shooter / plasma / metaller — 6 atlas folders into `Body/enemies/`). `metaller` late render (Lv 22 insect, biped_insect preset). Journal catch-up for 04-19 / 04-20 / 04-21 / 04-22 — filled 4-day gap, Dashboard + Session Tracker updated. Closes out the sprite-production week. ([daily note](2026-04-22.md))
- **2026-04-21** — Batches 10-13 finale: 28 new atlas configs / renders / imports across `magmaring` → `kraben` range. **Plasma tint siblings** — plasma base atlas + 4 tint variants (plasma_r `[1.5,0.6,0.6]`, plasma_b `[0.6,0.6,1.5]`, plasma_g `[0.6,1.5,0.6]`, plasma_p `[1.3,0.6,1.5]`) validates tint system beyond Eclipse. `.add_batchN.js` pattern formalized (5 batch scripts: 10 / 11 / 12 / 13 / blazzer). Late additions: `vocal`, `blazzer` (GLB arrived mid-day), `orc_baby`. Progress now **150 done / 104 pending / 255 no GLB / 509 total**. ([daily note](2026-04-21.md))
- **2026-04-20** — MASSIVE production day. Batches 2 through 9 — ~90 renders, ~70 UE5 imports. **EnemySubsystem C++ hardening**: `FEnemyEntry.SpriteTint` + `bCanMove`, size→scale parser (small/medium/large = 0.5x/1.0x/1.5x), `spriteScale` per-template override, `canMove` gate on attack lunge (stationary mobs thrash in place), ranged attack swap from vine decal → billboard sprite burst. Server: `size` / `canMove` / `spriteTint` / `spriteScale` added to all 5 emit paths (spawnEnemy / respawn / player:join / zone:ready / adapter). `render_monster.py --model-z-offset` for flying creatures + default lighting 0.92/0.98. 195-line spawn grid in `ro_zone_data.js` (Y=-11000..-3000). Navmesh rebuilt (+5243 lines). ([daily note](2026-04-20.md))
- **2026-04-19** — Sprite production organization day. `_prompts/lv1_50_model_matching.md` (~550 lines) — every GLB cross-referenced against Lv 1-50 roster with direct/alias/shared/tint/none tags. **Batch 0**: stationary sprites (plants / mushrooms / eggs, 13 atlases rendered, egg + tree preset). Mass UE5 import (35 atlas folders) catching up 04-13 through 04-15 backlog. `_prompts/resume_sprite_pipeline_session.md` created as sprite-session re-entry prompt. ([daily note](2026-04-19.md))
- **2026-04-18** — Enemy enumeration scripts (`_prompts/build_enemy_list.js` + `build_enemy_markdown.js`) — dumps every monster template, cross-references `3d_models/enemies/*.glb` and `atlas_configs/*_v2.json`, produces 605-row Level 1-99+ sprite-status table (status column: done / done (tinted) / pending (GLB ready) / no GLB). Regenerated `_prompts/enemy_sprite_session_resume.md` to 778 lines. Journal catch-up for 04-16/17/18 + Dashboard + Session Tracker updates. ([daily note](2026-04-18.md))
- **2026-04-17** — Light day. No commits, no code / doc / config edits. Only artifact: `Screenshot 2026-04-17 201201.png` at 20:12 (500 KB full-frame, likely in-game smoke test). Screenshot purpose undocumented — flagged for manual triage. ([daily note](2026-04-17.md))
- **2026-04-16** — Hosting / Packaging / Distribution **deep research day**. 4 major docs authored (~153 KB) in one session: `Hosting_Packaging_Distribution_Plan.md` (67 KB, 3-scenario decision matrix + executive summary + Scenario A end-to-end runbook), `Server_Deployment_Runbook.md` (38 KB, 4 hardware tracks), `Client_Packaging_Runbook.md` (21 KB, UE5.7 Shipping + Inno Setup), `Client_Endpoint_Config_Design.md` (27 KB, CLI → SaveGame → compile-time resolution + `/health` version handshake w/ C++ diffs). Research prompt filed. CLAUDE.md / Global_Rules.md skill-selection polish ("prefer loading MORE skills" rule + 5 new SKILL INVOCATION rows). Nothing implemented yet — plan-only. ([daily note](2026-04-16.md))
- **2026-04-15** — Two threads: (1) sprite tint system — `spriteTint:[r,g,b]` threaded end-to-end (template → adapter → 4 enemy:spawn emit paths → client `SetLayerTint` with respawn re-apply), Eclipse proves tint-only Lunatic variant. Ranged-attack decal replaced with a billboard `ASpriteCharacterActor` burst (attacker's sprite class, 0.6x, Attack anim). 8 new atlas configs (eclipse/thief_bug/thief_bug_f/dragon_fly/desert_wolf_b/plankton/spore/toad) + matching `spriteClass` in templates + Prontera South test spawn block. Navmesh rebuilt. (2) Full doc review — audited 8 plans + 7 core system docs for drift; added STATUS headers to 6 stale plans (Damage Numbers, Map, Kafra/Trading, Ground Item, Skill VFX, 3D World); synced stats across Project Overview / Enemy System / Event Reference / Networking Protocol / DocsNewINDEX / Dashboard / Session Tracker. ([daily note](2026-04-15.md))
- **2026-04-14** — Enemy attack polish: melee lunge system (wind-up→lunge→return via ServerTargetPos), ranged vine decal for Mandragora-style attacks, SpawnGroundStrikeEffect VFX, render_monster --thicken + lunge_offsets, Mandragora attackRange/aiType fix. Journal backfill + major commit `dea49b4` (172 files). ([daily note](2026-04-14.md))
- **2026-04-13** — Enemy sprite batch production: 10 new shape key animation presets (~1000 lines), ~20 spriteClass fields added to monster templates, egg/larva enemies made immobile, UniRig AI rigging pipeline installed + proved on rocker (40 bones, 6 anims), Blender 5.x FBX import fix, ~8 more enemy GLBs downloaded. ([daily note](2026-04-13.md))
- **2026-04-12** — Asset generation day: ~10 enemy 3D models downloaded from Tripo3D (drops, fabre, lunatic, pupa, willow, condor, hornet, roda_frog, savage_babe), enemy spawn priority reviewed. ([daily note](2026-04-12.md))
- **2026-04-11** — Sprite system review, assassin katar attack investigation (confirmed correct), Blender weight paint fix documented, egg shell hat item distribution, Blender crash troubleshooting. ([daily note](2026-04-11.md))
- **2026-04-10** — Inventory performance analysis (10-finding root cause report, 4-phase optimization plan). Ground item client implementation (GroundItemSubsystem + GroundItemActor, 733 lines). Phase 1 inventory optimizations started (const ref, cache, ItemDefCache). ([daily note](2026-04-10.md))
- **2026-04-09** — Ground item / loot drop server system: RO Classic ownership phases, scatter offsets, 60s despawn, party share, damage ranking, 6 socket events, enemy death integration. Research doc created. ([daily note](2026-04-09.md))
- **2026-04-08** — Minimap SceneCapture2D fix (SetPostProcessMaterial(false) to exclude cutout PP). Regression protection in 5 locations. ([daily note](2026-04-08.md))
- **2026-04-07** — Full audio system implementation: AudioSubsystem (2847 lines), enemy SFX (60 monsters, body material layering), player SFX (per-weapon swing/hit, per-class), BGM zone mapping (121 tracks), skill SFX (SkillImpactSoundMap), 3 new audio skills. Options menu drop sound toggles. ([daily note](2026-04-07.md))
- **2026-04-06** — Sprite-vs-world rendering FINAL solution (BLEND_Translucent + bDisableDepthTest + binary alpha, two camera→feet/head line traces + per-pixel depth, post-process cutout via PPI_CustomStencil, hardcoded 96 capsule, motion blur off, CameraSubsystem occlusion removed). Damage number polish — closed all 3 carryover blockers from 04-05 (crit starburst readable, target flinch verified, hit particle replaced). RO Classic audio deep research (GRF/BGM/SFX architecture, 121 pre-renewal tracks, composer credits, **Gravity v. NovaRO $4M legal status**, Tier 1-3 sourcing recommendations) — research only, output to `RO_Audio_System_Research.md`. ([daily note](2026-04-06.md))
- **2026-04-05** — Options menu expansion (14 settings, SaveGame, FPS overlay), character ground positioning fix (LocalSprite, name tag on sprite, fixed remote HalfHeight, billboard depth comp), RO Classic damage number rewrite + 6-phase hit impact (sine arc, status colors, hit flash, hit particles, crit starburst, combo total, target flinch, hit sounds). 3 new skills, 2 new docs, 3 new prompts. ([daily note](2026-04-05.md))
- **2026-04-04** — Login screen (deferred texture, resolution fix), ESC menu (char select return, Z=200 overlay), right-click player context (Trade/Party/Whisper), item icons (6169 mapped to subfolders). Major commit `60eca6c`. ([daily note](2026-04-04.md))
- **2026-04-03** — Kafra Storage (account-shared 300-slot, 10 handlers, split/sort/search QoL) + Player-to-Player Trading (two-step confirm, 13 cancel paths, atomic transfer, /trade command). 8 bugs found and fixed. ([daily note](2026-04-03.md))
- **2026-04-02** — Complete map system: minimap (134x134, 5 zoom, draggable), world map (12x8 grid, 62 zones), loading screen (Ken Burns, sparkles), Guide NPC marks, /where command. Planned Kafra Storage + Player Trading (prompts created). ([daily note](2026-04-02.md))
- **2026-03-30 to 2026-04-01** — Ground texture & material system: 1000+ AI textures, 608 RO originals, 17 material versions, 2700+ variants, DBuffer decals (91), Landscape Grass V3 (60 sprites, 13 zones), 80+ scripts, 3 new skills, 5 _meta guides ([daily note](2026-03-30.md))
- **2026-03-29** — Hair sprite layer committed, render pipeline overhaul (--cel-shadow 0.92), parallax fix (0.3→0.01), 5 hair visibility bugs fixed, 5 female classes re-rendered ([daily note](2026-03-29.md))
- **2026-03-27** — NavMesh pathfinding implemented (recast-navigation), headgear sprite layer (holdout occlusion, Egg Shell Hat), hair sprite layer started, 5 female class + weapon re-renders, shared armature system ([daily note](2026-03-27.md))
- **2026-03-26** — Animation standardization (17-anim standard for ALL classes), archer_f + bow weapon sprites, skeleton + poring enemy sprites (pure C++ actors), NavMesh pathfinding planned ([daily note](2026-03-26.md))
- **2026-03-25** — Weapon sprite overlay system SOLVED (dual-pass render + 5 C++ fixes + depth ordering + remote sync), swordsman anims simplified 19→14, mage atlases packed & validated ([daily note](2026-03-25.md))
- **2026-03-24** — Swordsman v2 atlases validated in-game, Mage v2 config built (34 anims, 2336 sprites rendered), standardized fighter vs mage animation sets ([daily note](2026-03-24.md))
- **2026-03-23** — 3D-to-2D sprite pipeline (Mage 168 sprites + Swordsman 768 sprites), UE5 SpriteCharacterActor, ComfyUI abandoned, deep+broad code audits (43 files) ([daily note](2026-03-23.md))
- **2026-03-22** — Bard/Dancer/Ensemble comprehensive fix, party system polish ([daily note](2026-03-22.md))
- **2026-03-21** — In-game testing: Sage (22 skills) + Hunter (18 skills) passed, account creation tested, Obsidian vault setup ([daily note](2026-03-21.md))
- **2026-03-20** — Skill icon application (all class skills), generic project progress, documentation overhaul ([daily note](2026-03-20.md))

## Useful Claude Code Commands
```bash
# Resume a previous session
claude --resume <session-id>

# Start with specific context
claude "load /sabrimmo-skills /sabrimmo-combat then fix X"

# Quick server test
cd server && npm run dev
```
