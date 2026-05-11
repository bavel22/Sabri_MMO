# Claude Code Session Tracker

All Claude Code sessions with their resume IDs, organized by topic. Use `claude --resume <id>` to continue any session.

> **Tip**: After each Claude Code session, add an entry here with the resume ID and a one-line summary of what was accomplished.

---

## Active / Recent

| Date | Resume ID | Topic | Notes |
|------|-----------|-------|-------|
| 2026-05-10 | (canonical compliance) | **Pre-renewal compliance audit (100%) + early-game tuning + character starter kit** | Cloned rAthena master pre-re. Built `_audits/` infrastructure: 5 extract/audit scripts + 5 idempotent fix scripts + JSON canonical references for 1,004 mobs + 6,169 items. **Fixed monster damage formula** (`ro_damage_formulas.js` `isMonsterAttack` flag): mobs were getting player-style StatusATK on top of canonical atk1/atk2, doubling Poring damage. **Stat point off-by-one**: `getStatPointsForLevelUp` was overpaying +19 over 1-99. **Refine fees**: Wlv3 5000z→1000z, Wlv4 20000z→2000z. **New character INSERT** (`POST /api/characters`): atomic transaction seeding `zone_name='prontera'` + Prontera town spawn coords + starter kit (1000z + Knife/Cotton Shirt/Sandals auto-equipped + 25 Red Potions hotbar 0 + 5 Fly Wings + 1 Butterfly Wing). **All 509 monsters: 0 stat findings, 0 mode findings, 0 drop findings** vs rAthena pre-re master. `SERVER_RATES.EXP_RATE_MULTIPLIER` constant (default 1) wired into both EXP-award sites. Drop entries now use explicit `itemId:` to avoid display-name collisions. 4 new memory files (canonical-compliance-audit, monster-damage-formula-fix, drop-itemid-disambiguation, character-starter-kit) + 5 skill updates (combat, stats, enemy, economy, zone, items). Dashboard + this tracker updated. Audit reports in `_audits/AUDIT_REPORT_*.md`. |
| 2026-05-10 | (cont. from 05-09 Job Master) | PlayerInputSubsystem fix + centered/draggable dialog + skills/memory updates | Silent-click root cause: `TryInteractWithNPC` is dead code; live path is `UPlayerInputSubsystem::ProcessClickOnNPC`. Added `AJobChangeNPC` cast in 4 sites of `PlayerInputSubsystem.cpp` (owner-walk break ~L263, filter ~L266, walk-arrival ~L153, in-range ~L541) + `#include`. `SJobChangeWidget` rewrite: removed full-screen modal backdrop, switched `AlignmentWrapper` to `HAlign_Center+VAlign_Center` for natural viewport centering, fixed-size `WidthOverride+HeightOverride` so 3 pages share dimensions, DPI-correct title-bar drag (Phase 7 pattern from `/sabrimmo-ui`), `EMouseCursor::GrabHand`/`GrabHandClosed` cursor on title bar. Skills updated: `/sabrimmo-click-interact` (pipeline rewrite + 4-site cast checklist + new troubleshooting rows), `/sabrimmo-npcs` (Job Master section + actual-vs-aspirational system clarification), `/sabrimmo-stats` (job:change/job:changed payload + state gates), `/sabrimmo-ui` (SJobChangeWidget reference for centered+draggable pattern). Memory: `feedback-npc-click-pipeline.md` + `job-change-npc-system.md`. Dashboard + this tracker updated |
| 2026-05-09 | `"job master npc"` | Job Master NPC + class change in-game | New: `JobChangeNPC.h/.cpp`, `UI/JobChangeSubsystem.h/.cpp` (Z=26, EventRouter for `job:changed`+`job:error`, local sprite respawn via `SpawnSpriteForClass`, `skill:data` re-emit), `UI/SJobChangeWidget.h/.cpp` (3-page `SWidgetSwitcher`: Greeting→Selection→Congrats, RO brown/gold theme). Modified: `SabriMMOCharacter.cpp` (`TryInteractWithNPC` cast chain), `MMOGameInstance.h/.cpp` (`SetSelectedCharacterJobClass`), `UI/OtherPlayerSubsystem.h/.cpp` (`HandleRemoteJobChanged` — peer sprite respawn with `ResetHairHiding`/`EquipVisuals` loop/`ReconcileHairVisibility`). Server `index.js` ~L10299: state gating (dead/sit/combat/overweight 90%+) + HP/SP refill to new max + DB persist + zone broadcast `combat:health_update`. Client tests: `tests/unit/job_change.test.js` 31 tests (data integrity + client mirror parity + eligibility tree + transitions + server gating). Sprite atlas reused: `antonio` (already imported under `Body/enemies/antonio/`). Plan agent caught 3 gaps: own-sprite refresh, skill tree refresh, peer sprite refresh — all addressed. **Latent gap on 05-10 surfaced**: `PlayerInputSubsystem` had hardcoded `AShopNPC`/`AKafraNPC` casts that needed updating, otherwise silent-click. |
| 2026-05-08 | (journal cleanup) | Journal catch-up 04-28 → 05-08 | Filled 11-day gap. 04-28/30/05-01 days off, 04-29 single 02:00 SpriteCharacterActor foliage-occlusion edit, 05-02 Spawn Region System + Hunyuan3D test tree, 05-03/04 Hunyuan3D pipeline build (235 base + ~828 variants), 05-05 v3 regional + RO Classic World Assets Expanded, 05-06 Water System mixed-mode, 05-07 spawnPool integration + grassfield05/07 + Sewer Dungeon prep + respawn ranges, 05-08 SewerDungeon01 build plan + ZoneTransition preload-wait + Inventory/Kafra polish + new_zone_creation prompt. Dashboard + Session Tracker updated |
| 2026-05-08 | `"sewer dungeon build"` (cont. from 05-07) | SewerDungeon01 PostProcess + Build Plan + ZoneTransition preload-wait | `PostProcessSubsystem.cpp` (00:32) — 3 zone presets (grassfield07, grassfield05, SewerDungeon01) + 7000-UU shadow cap + sewer fog override (0.30/0.2/moss-green). `Sewer_Dungeon_01_Build_Plan.md` (01:33, 82 KB, 20 sections + 5 appendices). `SewerDungeon01.obj` + `SewerDungeon01.json` (01:49). `ZoneTransitionSubsystem.h/.cpp` (02:02) — `WaitForPreloadAndHideOverlay` + 700ms stability + 1.5s minimum + 15s timeout. `KafraSubsystem.cpp` + `InventorySubsystem.cpp` (02:02) — `ShowExpectedZoneChange` integration. `_prompts/new_zone_creation.md` (02:06, 22 KB) — canonical bootstrap prompt. `ro_zone_data.js` (02:24) — exit warp radius 200→250 + position adjust |
| 2026-05-07 | `"new zone setup"` | Server `spawnPool` integration + grassfield05/07 + Sewer Dungeon scaffold + respawn ranges + water-state wiring | `index.js` (16:55, +402/-60 lines) — `resolveRespawnMs`, `spawnZoneEnemies`, `isInDeepWater`, water ref-counting Map, snap-and-warn, two-layer movement validation. `ro_monster_templates.js` (16:56) — `respawnMs: [min, max]` for blue_plant / green_plant / black_mushroom / vocal. `Enemy_System.md` (17:00) — "respawnMs format" section. NavMesh: grassfield05 (11:37), prontera_south rebuild (11:45), grassfield07 (16:04). `ro_world_map_data.js` (22:02) — SewerDungeon01 in ZONE_INFO + WORLD_MAP_GRID row 5 col 8. `AudioSubsystem.cpp` (22:07) — prt_sewb1-4 + SewerDungeon01 ambient (DunWind+CaveDrip+CaveFall) + bgm_19. Filed memory `feedback-respawn-range-syntax.md` |
| 2026-05-07 | (cont. Hunyuan thread) | Post-process / alt-mesh follow-up | `_tools/post_alt_mesh_pipeline.py` (09:55, 5,740 bytes) — final-stage cleanup after `hunyuan_alt_meshes_regen.py` swapped in better mesh candidates |
| 2026-05-06 | `"water system mixed mode"` | Mixed-mode `AWaterArea` + raycast deep detection + greedy rect merge + 3-stop gradient | `WaterArea.h/.cpp` (07:07/07:08) — auto-detected per-cell deep, greedy rect-merge nav cuts (`bCanEverAffectNavigation=true` + `UNavArea_Null`), three-stop depth gradient (shallow [0-250 UU] → deep → abyss [900 UU, opacity 1.0]), runtime material via standard `UMaterialExpression*` nodes (NOT custom HLSL — confirmed not to animate at runtime), reference-counted server tracking via `Map<areaId,{isDeep}>`, two-layer movement validation, `spawnEnemy()` snap-and-warn. `Water_System_Research.md` (07:30) — "Implementation Status" addendum. `SabriMMOGameMode.cpp` (08:16) — small wiring. Filed memory `water-system-mixed-mode.md` |
| 2026-05-06 | (cont. from 05-04 Hunyuan thread) | Alt-mesh regen + flatten consolidation | `_tools/hunyuan_alt_meshes_regen.py` (20:57, 11,527 bytes) — re-loads `03_meshes/s*.glb` candidates, recomputes scores, swaps in better alts + re-runs decimate+texture. `flatten_combined.py` (20:57). `flatten_to_final_v3.py` (21:02) — variant-folder handling |
| 2026-05-05 | (cont. from 05-04) | RO Classic regional asset doc + v3 regional Hunyuan generator | `_meta/RO_Classic_World_Assets_Expanded.md` (00:08, 15,705 bytes) — 9 niche regions cataloged (Umbala, Moscovia, Rachel/Veins, Eclage, Brasilis/Dewata/Malaya, OGH/Geffenia, Jawaii, Yuno Sage Academy). `_tools/hunyuan_v3_regional.py` (00:28, 140,537 bytes) — per-region asset definitions + regional palette overrides + lower 5-attempt reroll budget |
| 2026-05-04 | (cont. from 05-03 Hunyuan thread) | Multi-object detection + regen v2 + flatten + Session Record + UE5 import | `Session_Record_2026-05-03.md` (00:27, 24 KB) + `REUSABLE_SESSION_PROMPT.md` (00:27, 7 KB). `find_multi_object_assets.py` (17:54) + `find_multi_object_v2.py` (18:05) — flag assets where 2nd-largest component > 25% of largest. `hunyuan_v2_regen.py` (18:52) — strengthen single-object negative + 8-seed sweep + largest-component-only. `flatten_to_final.py` (16:16) + `flatten_to_final_v2.py` (18:52). `import_hunyuan_assets.py` + `import_hunyuan_v2_assets.py` + tuning helpers. `prontera_south.json` spawn export (22:14) — first real-world ExportSpawnRegions output |
| 2026-05-03 | `"hunyuan asset pipeline"` | Hunyuan3D 2.1 production pipeline build + 135 batch 1 assets + variants + batch 3 queued | Diagnostic sweeps (00:06-00:50). `hunyuan_asset_pipeline.py` (23:04, 48 KB) — master pipeline with multi-input (2 strategies × 2 seeds) + decimate-FIRST + quality-aware scoring (cube_artifact=0.001) + reroll loop with 8 cycle suffixes + backup outside `asset_dir`. `hunyuan_overnight_continue.py` (02:18, 32 KB), `hunyuan_resume.py` (08:18). `comfyui_supervisor.py` (18:15) + UTF-8 logger patch. `hunyuan_cleanup.py` (09:44) — 79%→96% GOOD across 3 passes. `hunyuan_color_variants.py` (17:14, 65 KB) — ~470 variants. `hunyuan_batch3_100.py` (23:23, 44 KB) — 100 gap-filling assets queued. `3D_World_Asset_Pipeline_2026.md` (23:05, ~1,460 lines) |
| 2026-05-02 | `"spawn region system"` | Server + UE5 spawn-area editor | `SpawnRegionVolumes.h/.cpp` (13:51/16:09) — `ASpawnAllowVolume` + `ASpawnDenyVolume` placeable actors with `MonsterFilter` + `RegionTag`. `SpawnRegionExporter.h/.cpp` (13:51/13:55) — `ExportSpawnRegions <Zone>` console command, JSON output. `ro_spawn_regions.js` (13:52, 8,936 bytes) — server loader, weighted random by XY area, deny-box rejection, NavMesh ground-snap (500 UU halfExtent on Z). `Spawn_Region_System.md` (14:08, 16 KB). `Complete_Level_Building_Guide.md` (22:49) — markdown table polish only |
| 2026-05-02 | `"hunyuan3d test tree"` (precursor to 05-03) | Hunyuan3D 2.1 single-asset proof | `_tools/hunyuan_test_tree.py` (23:56, 10 KB), `hunyuan_retry_meshgen.py` (23:59). `3d art/hunyuan_test_output/` — first end-to-end proof: SDXL → rembg BiRefNet → `Hy3D21VAEDecode mc_algo="mc"` → 17 MB raw mesh, depth ratio 0.97. Confirmed `dmc` broken on torch 2.12 / sm_120 |
| 2026-05-01 | — | No Claude Code session | No file activity all day |
| 2026-04-30 | — | No Claude Code session | No file activity all day |
| 2026-04-29 | — | Single 02:00 sprite material edit | One file: `SpriteCharacterActor.cpp` `CreateSpriteMaterial` — added `FoliageOcclusionDist` scalar parameter (default 80 UU) + rewrote opacity custom-node so foliage occluders (small, < 80 UU thick) discard the pixel while walls (≥ 80 UU thick) still trigger the existing line-trace silhouette path. Side trip from the 04-27 main queue |
| 2026-04-28 | — | No Claude Code session | No file activity all day |
| 2026-04-27 | (journal cleanup) | Journal catch-up 04-23 / 04-24 / 04-25 / 04-26 / 04-27 | Filled 5-day gap. 04-23+25 confirmed empty; 04-24 single late-night `ro_monster_templates.js` tweak; 04-26 = triple-stream big day (Priest/Assassin/Monk audits, big commit `56dce91`, Homunculus end-to-end re-audit closing 7/10 gaps, sprite-quality system architecture: ZonePreloadSubsystem + zone:adjacent_classes + 3 subsystem hooks); 04-27 = sprite-quality slider final wiring + project-wide canonical-settings migration (35+ migration scripts in `clear_maxsize_*` and `enable_streaming_*` families) + Path C deferred swap + helmet flicker fix + 11-doc/4-prompt skills sweep + sRGB=True correction after 4 enemies rendered invisible. Dashboard + Session Tracker updated |
| 2026-04-27 | `"sprite quality slider system"` (cont.) | Skills + docs sweep + canonical settings doc | Triggered by `_prompts/1111 update skills and docs.md` (one-line "update all skills and documentation for changes made"). 11 doc files updated (Adding_Humanoid_Enemy_Sprite_Workflow / Shared_Armature_Sprite_Architecture / Enemy_Sprite_Implementation / HeadgearTop_Sprite_Layer_Implementation_Plan / Equipment_Sprite_Pipeline_Reference / Job_Sprite_Pipeline_Reference / Weapon_Sprite_Overlay_Pipeline / 2 prompts) + `_meta/settings for importing sprite atlases.md` (terse human-readable canonical reference). Mass docstring update across 35+ migrate / import scripts (function rename `apply_original_settings → apply_canonical_settings`, MaxTextureSize cap removed, NeverStream flipped, idempotency checks added). Filed `_prompts/sprite_quality_canonical_settings_resume.md` (full state dump for next-session re-entry). sRGB=True correction came from `check_srgb.py` 6-asset diff after ambernite/parasite/plasma/rafflesia rendered invisible |
| 2026-04-27 | `"sprite quality slider system"` (cont.) | Path C deferred equipment swap + helmet flicker | `Sprite/SpriteCharacterActor.h/.cpp`: `FSpriteLayerState::PendingLayerAtlasRegistry` + `PendingSwapTexture` + `PendingSwapTimeoutSeconds`, new `FinalizeEquipmentSwap()`. `LoadEquipmentLayer` defers swap if new texture still streaming; old equipment stays visible until streaming completes (or 1s timeout). Helmet flicker fix: `if (!IsValid(L.MaterialInst))` guard prevents fresh MID creation on every `LoadEquipmentLayer` call (e.g. inventory open) |
| 2026-04-27 | `"sprite quality slider system"` (cont.) | Mass migration — `clear_maxsize_*` + `enable_streaming_*` scripts | `clear_max_texture_size.py` proof-of-concept → `_test_poring.py` smoke test → split into 12 batches (`clear_maxsize_01_players_a` through `clear_maxsize_12b_enemies_08`) because editor hangs on bigger sweeps. Sets `max_texture_size=0` project-wide (fixed caps cause non-square atlases to downscale unevenly: walk 8192×12288 vs idle 8192×8192). Mirror sweep for `never_stream=True → False` in 18 `enable_streaming_*` scripts (test → prt_south → split parts → full project) — without this, slider is cosmetic |
| 2026-04-27 | `"sprite quality slider system"` (cont. from 04-26) | Slider UI wiring (very early hours 01:56-01:57) | `MMOGameInstance.h/.cpp`, `OptionsSubsystem.h/.cpp` (GetSpriteQuality / SetSpriteQuality clamp 0-4, iSpriteQuality=2 Medium default, ApplySpriteQualityToLoadedTextures iterates `/Game/SabriMMO/Sprites/Atlases/`), `SOptionsWidget.cpp` Sprite Quality dropdown. Tier table: Ultra=0 / High=1 / Medium=2 (default) / Low=3 / Very Low=4 |
| 2026-04-26 | `"sprite quality slider system"` (new) | Sprite-quality architecture (late evening) | Created `client/SabriMMO/Source/SabriMMO/UI/ZonePreloadSubsystem.h/.cpp` — per-world subsystem with 3-tier handle system (Pinned / Active / 4 GB LRU), `RequestClassPreload` / `RequestLayerPreload` / `PinClass` / `BeginZone` / `HandleAdjacentClasses` API, async via `FStreamableManager` + `FStreamableHandle`. Hooks: `EnemySubsystem::HandleEnemySpawn` (idempotent batch preload), `OtherPlayerSubsystem::HandlePlayerMoved` (body + hair + 6 equipment slots if `equipVisuals` present), `ZoneTransitionSubsystem` (loading screen waits on preload). Server `zone:adjacent_classes` emit walks `ZONE_REGISTRY[zone].warps`, returns `{destZone: [spriteClass...]}` so client predictively warms neighbour-zone atlases |
| 2026-04-26 | `"homunculus end-to-end re-audit"` | Homunculus completion log | 10 gaps surfaced; 7 fixed (G1/3/4/5/6/7/8/9/10), partial G2 (Lif autocast only), 2 deferred. Fixes: G4 DB persistence on `homunculus:skill_up`, G7 server position-tick (200ms, 600 UE/s + Urgent Escape mult, emits `homunculus:position`), G8 sprite swap to `ASpriteCharacterActor` (Lif=poring, Amistr=poporing, Filir=picky, Vanilmirth=drops), G10 attack-target validation. Client UI: 8 new buttons in SHomunculusWidget (3× Use Skill, 3× Allocate, Mode cycle, Evolve). Remote rendering: 3 new socket handlers + RemoteHomActors map. Lif Healing Hands autocast at HP < 50%, 2s tick. Log appended to `Alchemist_Skills_Audit_And_Fix_Plan.md:506` |
| 2026-04-26 | (commit boundary) | `56dce91` — backlog flush | The 7-day backlog from 04-15 → 04-22 + the morning's audits committed at 11:16. Title: `feat: enemy sprite production sprint (150/509), Mixamo batch downloader, hosting/packaging research, journal catch-up`. ~190 atlas configs, 14 batch import scripts, 14 weapon import scripts, hosting/packaging research docs (4 deep-research files ~153 KB), journal catch-up, .gitignore tweak |
| 2026-04-26 | (new session, class audits) | Priest / Assassin / Monk skill audits + ro_buff_system polish | `docsNew/05_Development/Priest_Skills_Audit_And_Fix_Plan.md` (10:06), `Assassin_Skills_Audit_And_Fix_Plan.md` (10:25), `Monk_Skills_Audit_And_Fix_Plan.md` (10:41). `server/src/ro_skill_data_2nd.js` (09:59), `server/src/ro_buff_system.js` (10:49) — small data + buff-handler corrections folded into the audits. Three of six remaining "Class Skill Testing" audits now done; Rogue / Blacksmith / Alchemist audits remain |
| 2026-04-26 | (continuation, sprite-pipeline) | Knuckle weapon texture diagnosis (00:45) | `client/SabriMMO/Scripts/Environment/diag_weapon_textures.py` + `fix_knuckle_match_working.py` — read-only inspector + matching fix to bring female knuckle weapon atlas in line with working reference. Precursor to the project-wide canonical settings work that lands 04-27 |
| 2026-04-25 | — | No Claude Code session | No file activity all day |
| 2026-04-24 | — | One-line `ro_monster_templates.js` tweak | Single late-night edit at 23:39 — likely small spriteClass / weaponMode / canMove / spriteTint adjustment for an enemy needing hand-tweaking after the 04-22 in-game pass. Type and target unverified (still in working tree) |
| 2026-04-23 | — | No Claude Code session | No file activity all day |
| 2026-04-22 | (journal cleanup) | Journal catch-up 04-19 / 04-20 / 04-21 / 04-22 | Filled 4-day gap. 04-19 (cross-reference + batch 0 stationaries + UE5 bulk import), 04-20 (batches 2-9, C++ subsystem hardening, 195-line spawn grid, --model-z-offset), 04-21 (batches 10-13, plasma tint siblings, `.add_batchN.js` pattern, progress hits 150/509), 04-22 (tail-end imports + `metaller` late render). Dashboard stats updated |
| 2026-04-22 | `"implementing more enemy sprites"` (cont.) | Tail-end imports + metaller render | 6 atlas folders imported (alligator / blazzer / stone_shooter / plasma / metaller). `metaller` late render (Lv 22 insect, biped_insect preset, 17 Mixamo anim folders rendered) |
| 2026-04-21 | `"implementing more enemy sprites"` (cont.) | Batches 10-13 finale + plasma tint siblings | 28 atlas configs / renders / imports (magmaring → kraben). Plasma base atlas + 4 tint variants (plasma_r/b/g/p with [1.5,0.6,0.6] / [0.6,0.6,1.5] / [0.6,1.5,0.6] / [1.3,0.6,1.5]). `.add_batchN.js` pattern formalized (10/11/12/13/blazzer). Late additions: `vocal`, `blazzer`, `orc_baby`. Progress: **150 done / 104 pending / 255 no GLB / 509 total** |
| 2026-04-20 | `"implementing more enemy sprites"` (cont.) | Batches 2-9 production sprint (MASSIVE DAY) | ~90 renders + ~70 UE5 imports across 8 batches. EnemySubsystem C++ hardening (SpriteTint FLinearColor, bCanMove, size→scale parser, spriteScale override, canMove lunge gate, ranged attack billboard swap). Server all-5-emit-path parity (size / canMove / spriteTint / spriteScale). render_monster.py --model-z-offset + default lighting 0.92/0.98. pupa_v2.json walk/attack fallback. 195-line spawn grid (Y=-11000..-3000). Navmesh rebuilt (+5243 lines) |
| 2026-04-19 | `"implementing more enemy sprites"` (cont.) | Cross-reference + batch 0 stationary sprites + UE5 bulk import | `_prompts/lv1_50_model_matching.md` (~550 lines, direct/alias/shared/tint/none tags). Batch 0: 13 stationary sprites (plants/mushrooms/eggs). Mass UE5 import (35 atlas folders catching up 04-13 through 04-15 backlog). `_prompts/resume_sprite_pipeline_session.md` created as re-entry prompt |
| 2026-04-18 | (journal cleanup) | Journal catch-up 04-16 / 04-17 / 04-18 | Filled 04-16 log (host/package/distribute research day, 6 sessions × 4 major docs produced), created 04-17 stub (light day, screenshot only), created this log. Dashboard + Session Tracker updated with new Hosting/Packaging/Distribution section |
| 2026-04-18 | `"implementing more enemy sprites"` (cont.) | Enemy enumeration scripts + full Lv 1-99 sprite roster regen | Built `_prompts/build_enemy_list.js` (142 lines) + `_prompts/build_enemy_markdown.js` (103 lines). Dumped every template, cross-referenced 3d_models/enemies/*.glb + atlas_configs/*_v2.json, produced a 605-row sprite-status table in `_prompts/enemy_sprite_session_resume.md` (now 778 lines). `GLB_ALIAS` + `PRESET_SUGGESTIONS` curated in-script |
| 2026-04-17 | — | No Claude Code session | Only evidence: `Screenshot 2026-04-17 201201.png` at 20:12 (500 KB full-frame). Likely in-game smoke test. No code / doc / config edits |
| 2026-04-16 | `"host package distribute research"` | Hosting / Packaging / Distribution deep research | 4 major docs authored (~153 KB total): `Hosting_Packaging_Distribution_Plan.md` (67 KB master plan), `Server_Deployment_Runbook.md` (38 KB, 4 hardware tracks), `Client_Packaging_Runbook.md` (21 KB, UE5.7 Shipping + Inno Setup), `Client_Endpoint_Config_Design.md` (27 KB, CLI → SaveGame → compile-time resolution + `/health` version handshake). Research prompt `_prompts/host_package_distribute_research.md` (27 KB) |
| 2026-04-16 | (04-15 doc sweep tail) | CLAUDE.md + Global_Rules.md skill-selection polish | New "prefer loading MORE skills" rule in CLAUDE.md Mandatory Context Loading; new SKILL INVOCATION rows in Global_Rules for `agent-architect`, `sabrimmo-audio-enemy`, `sabrimmo-audio-player`, `sabrimmo-rig-animate`, `sabrimmo-item-drop-system` |
| 2026-04-15 | `"implementing more enemy sprites"` (cont.) | Sprite tint system + Eclipse proof | `spriteTint: [r,g,b]` field on monster templates, threaded through the ENEMY_TEMPLATES adapter and all 4 server `enemy:spawn` emit paths. `FEnemyEntry.SpriteTint` + `SetLayerTint(Body, ...)` on client with respawn re-apply. Eclipse = tint-only Lunatic variant sharing one atlas |
| 2026-04-15 | `"implementing more enemy sprites"` (cont.) | Ranged attack → billboard burst | Decal-at-feet replaced with `ASpriteCharacterActor` at target feet (0.6x scale, Attack anim, 0.8s `TWeakObjectPtr` timer destroy). Billboards face camera so the effect reads from any angle |
| 2026-04-15 | `"implementing more enemy sprites"` (cont.) | 8 new atlas configs + Prontera South spawns | `_v2.json` configs for eclipse/thief_bug/thief_bug_f/dragon_fly/desert_wolf_b/plankton/spore/toad; `spriteClass`+`weaponMode` on the 8 templates; "New sprite enemies (testing)" spawn block added to Prontera South; navmesh rebuilt |
| 2026-04-15 | `"implementing more enemy sprites"` | Next-session prompt filed | `_prompts/enemy_sprite_session_resume.md` — sprite tint plan, recolor candidates, 12 animation presets recap, Level 1-30 enemy table (56 entries) |
| 2026-04-15 | (this) | Full doc review + sync | Audited 8 plan docs + 7 core system docs. Added STATUS headers to 6 stale plans (Damage Numbers, Map, Kafra/Trading, Ground Item, Skill VFX, 3D World). Synced line counts (35,281), sprite-monster count (30), UI subsystem count (40), migration count (28), data-module count (21) across Project Overview / Enemy System / Event Reference / Networking Protocol / DocsNewINDEX / Dashboard / this tracker |
| 2026-04-14 | `"implementing more enemy sprites"` (cont.) | Enemy attack lunge + ground strike VFX | Melee lunge (wind-up→lunge→return via ServerTargetPos), ranged vine decal, SpawnGroundStrikeEffect (NS_Dark_Stone_Impact green tint), Mandragora attackRange+aiType fix |
| 2026-04-14 | `"ai generated rig and animations for non humanoids"` (cont.) | Render pipeline improvements | --thicken CLI arg (Solidify for thin geometry), interpolate_lunge(), lunge_offsets in render_all() |
| 2026-04-14 | `f5a4b398` | Journal organization + commit | Backfilled 04-11 through 04-14, updated tracker + dashboard, committed `dea49b4` (172 files, +39,625 lines) |
| 2026-04-13 | `"ai generated rig and animations for non humanoids"` | UniRig pipeline setup + rocker proof | Researched AI rigging tools, installed UniRig (conda env, 4 Windows patches, RTX 5090 cu128). Proved on rocker: 40 bones, 6 procedural Blender animations. Scripts: animate_rocker.py, fix_rocker_legs.py, rebuild_from_fixed.py |
| 2026-04-13 | `"implementing more enemy sprites"` | Enemy sprite batch production | Downloaded ~8 more enemy GLBs (chonchon, creamy, mandragora, pecopeco, poison_spore, poporing, smokie). Added spriteClass/weaponMode to ~20 monster templates. Loaded /sabrimmo-enemy |
| 2026-04-13 | `3c21439f` | Egg/larva enemies — immobile | Made egg enemies (ant egg, dragon egg, pupa, thief bug egg, peco peco egg) not move or attack |
| 2026-04-13 | `9d8e4131` | GLB to FBX conversion | Batch converted enemy GLBs for Blender processing |
| 2026-04-13 | `b0bdd882` | Enemy sprite implementation | 10 new shape key animation presets (~1000 lines in render_monster.py): caterpillar, rabbit, egg, frog, tree, bird, flying insect, bat, quadruped, plant |
| 2026-04-12 | `91745b32` | Enemy list review | Listed all spawned enemies, prioritized Prontera field monsters for sprite batch |
| 2026-04-12 | `fda4dac5` (resumed) | 3D-to-2D pipeline — enemy models | Downloaded ~10 enemy GLBs via Tripo3D (drops, fabre, lunatic, pupa, willow, roda_frog, condor, hornet, savage_babe) |
| 2026-04-11 | `4c2411ea` | Assassin katar attack investigation | Reviewed dual-wield/katar code, confirmed attacks working correctly |
| 2026-04-11 | `a4e1585b` | Blender weight paint correction | Manual weight paint fix for stretched meshes after armature parenting. Updated _meta/pairing base armature.md |
| 2026-04-11 | `965e50ad` | Sprite system review | Loaded /sabrimmo-sprites, reviewed pipeline state — 17-anim standard and shared armature confirmed stable |
| 2026-04-11 | `0ea73525` | Blender crash troubleshooting | Blender hanging on launch — killed zombie processes |
| 2026-04-10 | `"inventory performance"` | Inventory performance analysis + optimization start | Root cause report: 10 cascading bottlenecks (2 DB queries + 164KB transfer + 350 widget rebuilds per potion use). 4-phase optimization plan. Phase 1 started: GetFilteredItems returns const ref, RebuildFilteredCache, ItemDefCache TMap, HandleItemDefs/HandleRefineResult handlers |
| 2026-04-10 | `"ground item client"` | Ground item client implementation | GroundItemSubsystem (368 lines) + GroundItemActor (230 lines). Billboard sprites, tier-color tinting, click-to-pickup, spawn arc animation, socket handlers for spawned_batch/despawned_batch/ground_list/picked_up |
| 2026-04-09 | `"ground item system"` | Ground item / loot drop server system | Full RO Classic drop system: ownership phases (3s/5s/7s normal, 10s/20s/22s MVP), scatter offsets (SE/W/N cycle), 60s despawn, party share, damage ranking priority. 6 socket events, ~207 lines server globals + helpers, drop integration in enemy death handler |
| 2026-04-08 | `"minimap fix"` | Minimap SceneCapture2D + cutout post-process fix | SetPostProcessMaterial(false) in MinimapSubsystem::SetupOverheadCapture. Root cause: global cutout PP material darkened entire capture. Regression protection in skill, CLAUDE.md, docs, inline comment, memory |
| 2026-04-07 | `"enemy audio"` | Enemy/monster audio implementation | Body material sounds (soft/hard/metal/undead), attack/die/move/stand SFX, 60 monsters wired, variant SFX system (TArray + random pick), ROSFX/effect/ folder (626 files) |
| 2026-04-07 | `"player audio"` | Player audio implementation | Per-weapon-type swing/hit sounds, per-class fallback, body material reaction, level up chime, heal sound. Two-tier resolution. Local + remote player support |
| 2026-04-07 | `"audio research"` | BGM zone mapping + audio skill fix | Zone_BGM_Table.md, fixed /sabrimmo-audio misattribution, BGM files sourced (121 tracks), skill SFX wiring (SkillImpactSoundMap), 3 new audio skills created |
| 2026-04-06 | `"character positioning tweaking"` | Sprite-vs-world rendering — final solution | BLEND_Translucent + bDisableDepthTest + binary alpha, two camera→feet/head line traces, per-pixel depth check, post-process cutout via PPI_CustomStencil. CameraSubsystem occlusion REMOVED. Hardcoded 96 capsule half-height. Motion blur disabled. New memory `sprite-rendering-2026-04-06.md` |
| 2026-04-06 | `"Damage Number enhancements"` | Combat polish — close 04-05 carryover blockers | Crit starburst opacity/scale tuned, target flinch verified end-to-end (Hit anim + enemy:move guard firing), `NS_AutoAttackHit` replaced with punchier Niagara from `/Game/Variant_Combat/VFX/`. All 3 04-05 blockers closed |
| 2026-04-06 | `"audio research"` | RO Classic audio — deep research | Full RO audio architecture: GRF/BGM/mp3nametable layout, 121 pre-renewal tracks, SFX inventory, composer credits (SoundTeMP/BlueBlue), filename misspellings to preserve, **legal status (Gravity v. NovaRO $4M 2022 — DO NOT ship RO audio)**, recommended Tier 1-3 sourcing. Output: `docsNew/05_Development/RO_Audio_System_Research.md` + memory `ro-audio-system.md`. Research only, no implementation |
| 2026-04-05 | `"options-menu-expansion"` | Options menu expansion | New OptionsSubsystem + SOptionsWidget, 14 settings (Display/Interface/Camera/Gameplay), `UOptionsSaveGame` SaveGame persistence, FPS overlay, ESC menu integration, 9 target subsystems wired |
| 2026-04-05 | `"character positioning tweaking"` | Character ground positioning fix | LocalSprite TWeakObjectPtr on pawn, name tag re-attached to sprite (smooth Z), OtherCharacterMovementComponent fixed HalfHeight=96, billboard SpriteDepthOffset compensation in ground snap, VFX spawn at visual center |
| 2026-04-05 | `"Damage Number enhancements"` | RO Classic damage numbers + 6-phase hit impact | Sine arc + drift + scale rewrite, RO color swap, status custom-color path, hit flash (3,3,3 overbright), hit particles (NS_AutoAttackHit), crit starburst (FSlateBrush), combo total (FComboTracker), target flinch (Hit anim + enemy:move guard), hit sounds (5 WAVs, pitch jitter). New skill `/sabrimmo-damage-numbers` |
| 2026-04-04 | `"login screen"` | Login screen background + resolution | Deferred texture loading retry, FSlateBrush, ScaleToFill, standalone timing fix, camera aspect ratio constraint |
| 2026-04-04 | `"ESC menu"` | ESC menu + character select return | EscapeMenuSubsystem, player:leave two-phase handler, bReturningToCharSelect, Z=200 background overlay, static FButtonStyle |
| 2026-04-04 | `"right click other players"` | Right-click player context menu | FMenuBuilder popup (Trade/Party Invite/Whisper), bDidRotateThisClick camera drag detection, PlayerInputSubsystem routing, guard conditions |
| 2026-04-04 | `"icons"` | Item icon mapping (all 6169 items) | Subfolder-based IconAssetMap in InventorySubsystem, /Game/SabriMMO/Assets/Item_Icons/{subfolder}/Icon_{name}, underscore fallback |
| 2026-04-03 | `"Storage"` | Kafra Storage — full implementation + QoL | Account-shared 300-slot storage, 10 socket handlers (6 storage + 4 inventory QoL), StorageSubsystem + SStorageWidget, split/sort/auto-stack/search, 40z Kafra fee, JSONB compounded_cards, 4 bugs fixed |
| 2026-04-03 | `"sabrimmo-trading"` | Player-to-Player Trading — full implementation | TradeSession class, 9 socket handlers, two-step confirm (OK→Trade), 13 cancel paths, atomic transfer, trade_logs audit trail, /trade chat command, STradeWidget (Z=22), 4 bugs fixed |
| 2026-04-02 | `"map system research and planning"` | Map system — minimap, world map, loading screen, Guide NPC | All 9 phases. MinimapSubsystem (134x134, 5 zoom, 3 opacity, draggable), SWorldMapWidget (12x8 grid, 62 zones), loading screen (Ken Burns, sparkles), Guide NPC marks, /where command, 5 socket events, ro_world_map_data.js. ~1429 lines, 6 new + 2 modified files. |
| 2026-03-30 to 2026-04-01 | `"ground-texture-and-materials-system"` | Ground textures, materials, decals, grass | Commit `093056d`. 1000+ AI textures, 608 RO originals, 17 material versions, 2700+ variants, DBuffer decals (91 instances), Landscape Grass V3 (60 sprites, 13 zones), paintable grass layers. 3 new skills, 4 new docs, 80 new scripts. |
| 2026-03-29 | — | Hair sprite layer + render pipeline overhaul | Commit `447edea`. Hair layer (9-color tinting, hides_hair, 5 visibility bugs fixed, parallax fix 0.3→0.01). New render standard: render_blend_all_visible.py + .blend scenes + --cel-shadow 0.92 --cel-mid 0.98. Re-rendered merchant_f/swordsman_f/knight_f/archer_f/novice_f + female bow + hair atlases. Fixed 8 missing server broadcast locations for hair data. |
| 2026-03-27 | `"enemy server and client navmesh pathfinding"` | NavMesh pathfinding | Server-side recast-navigation v0.42.1, OBJ export from UE5, coordinate conversion, de-aggro system, all enemy movement patched |
| 2026-03-27 | `"Headgear Sprite System"` | Headgear sprite layer | HeadgearTop layer, holdout occlusion (replaces depth_front), always_front depth mode, multi-slot blocking, Egg Shell Hat first headgear |
| 2026-03-27 | `"hair-sprite-layer-system"` | Hair sprite layer | ESpriteLayer::Hair, 9-color tinting, hides_hair flag, reset-derive-reconcile pattern, female hair style 1 atlas |
| 2026-03-26 | `"state dependent sprite atlases standardization"` | Animation standardization | Unified ALL classes to 17-anim standard, targetType-driven cast, Bow weapon mode, file reorg |
| 2026-03-26 | `"female archer animations"` | Archer + bow sprites | Archer_f body 1136 sprites + bow weapon overlay 1136 sprites, global depth ordering fix |
| 2026-03-26 | `"Skeleton Sprite Animations"` | Skeleton enemy sprite | First humanoid monster sprite, Mixamo-rigged, pure C++ actor (no BP_EnemyCharacter) |
| 2026-03-26 | `"poring sprite animations"` | Poring enemy sprite | First blob monster, shape key anims via render_monster.py, --model-rotation -90 |
| 2026-03-26 | — | NavMesh pathfinding plan | Full plan + implementation prompt. Server-side recast-navigation, client unchanged |
| 2026-03-25 | `"sprite weapon layering system"` | Weapon sprite overlay | SOLVED: dual-pass render + 5 C++ fixes + depth ordering + remote sync. 11 files, +6167 lines |
| 2026-03-25 | `"swordsman and state dependent sprite atlases"` | Swordsman cleanup + mage pack | Simplified 19→14 anims, packed mage_f atlases, validated in UE5 |
| 2026-03-24 | `"mage_f sprite atlases"` | Mage sprite atlases | 34 Mixamo FBX anims, mage_f_v2 config, 2336 sprites rendered (atlases not yet packed) |
| 2026-03-24 | `"swordsman and state dependent sprite atlases"` | Swordsman v2 validation | Validated v2 per-animation atlases in-game, all 19 anims + weapon mode switching confirmed |
| 2026-03-23 | `"swordsman and state dependent sprite atlases"` | Swordsman dual atlas sprites | 768 sprites, 20 anims, 4 weapon group atlases, UE5 SpriteCharacterActor integration |
| 2026-03-23 | `"3d to 2d pipeline"` | 3D-to-2D sprite pipeline | Full pipeline: Tripo3D → Mixamo → Blender render → post-process → atlas packer. Female Mage 168 sprites validated |
| 2026-03-23 | `"comfyUI sprite gen"` | ComfyUI sprite generation | Attempted AI image gen approach — abandoned due to consistency issues |
| 2026-03-23 | `"3d art research"` | 3D art research | Research on 3D art approaches for sprite generation |
| 2026-03-23 | `"deep audit"` | Deep code audit | Deep audit pass — results in `_audit/` (43 files) |
| 2026-03-23 | `"broad audit"` | Broad code audit | Broad audit pass — results in `_audit/` |
| 2026-03-22 | — | Bard/Dancer/Ensemble fixes | Song/dance buff stats, instrument/whip auto-attack, ensemble audit, party system polish |
| 2026-03-21 | `ef5108e4-2783-4088-8ab5-31dd4878b510` | Account creation testing | Tested register, login, character CRUD, JWT auth |
| 2026-03-21 | `e95cef1f-0de4-4a5a-879f-657e1f1319df` | Give items to Sage | Prepped Sage character with gear for testing |
| 2026-03-21 | `"Sage"` | Sage skill testing | All 22 Sage skills (1400-1421) tested and verified |
| 2026-03-21 | `"Hunter skills testing"` | Hunter skill testing | All 18 Hunter skills (900-917) tested and verified |
| 2026-03-21 | `"Obsidian Vault Setup"` | Obsidian vault setup | Created _journal/, _prompts/, _templates/, Dashboard, Workflow Guide |
| 2026-03-20 | `00122bb9-ba62-4e92-8d0f-a15ff48e3549` | Skill icon application | Added icons to all class skills |
| 2026-03-20 | `ad3cfed4-9df5-44fa-b98b-a19ab0ad23d3` | Generic project progress | Misc project work |
| 2026-03-20 | `95d02d40-839c-456c-971d-f7d1a75b70c2` | Documentation overhaul | INDEX.md, cross-links, txt→md conversion, data corrections |

---

## Second Class Implementation (Phases 0-6)

| Resume ID | Phase | Notes File |
|-----------|-------|-----------|
| `0caa9e7c-e6b8-4922-9c23-e3ea27c10277` | Phase 0: Foundation | [2-1 and 2-2 skills implementation P0](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P0.md) |
| `bb6e7ab6-56c5-41a0-91d0-503d6bc87814` | Phase 1: Assassin+Priest+Knight | [P1](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P1.md) |
| `bdd12969-1f29-4c99-8b0c-4ae480bb7634` | Phase 2: Crusader+Wizard+Sage | [P2](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P2.md) |
| `22ce505b-f25d-43ca-b0b4-2ebac7319373` | Phase 3: Monk+Hunter | [P3](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P3.md) |
| `306eb5b5-c8e2-4a9f-af9e-c8a928fdbae4` | Phase 4: Bard+Dancer | [P4](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P4.md) |
| `46fcda86-837e-460d-bc78-92a92b921b35` | Phase 5: Blacksmith+Rogue | [P5](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P5.md) |
| `e6e2dc9c-9795-4c3c-8fb8-73e85553b955` | Phase 6: Alchemist+Homunculus | [P6](../Dev%20Notes/2-1%20and%202-2%20skills%20implementation%20P6.md) |

## Class Skill Audits

| Resume ID                              | Class          | Notes File                                                                                                                                                                  |
| -------------------------------------- | -------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `24afe47d-eb46-41c1-a5d8-6bda6691814b` | Swordsman      | [swordsman skills audit](../Dev%20Notes/class%20skills%20audit/swordsman%20skills%20audit.md)                                                                               |
| `b2a945fb-c7ed-402a-9b60-2b526b368525` | Acolyte+Mage   | [acolyte and mage skills audit](../Dev%20Notes/class%20skills%20audit/acolyte%20and%20mage%20skills%20audit.md)                                                             |
| `3b0c9acb-dc3a-4ab6-b851-cecbbf49ffc4` | Thief (pass 1) | [thief skills audit](../Dev%20Notes/class%20skills%20audit/thief%20skills%20audit.md)                                                                                       |
| `45342877-a0f5-4b2e-a4ba-3fabbf60bc8b` | Thief (pass 2) | [thief skills audit2](../Dev%20Notes/class%20skills%20audit/thief%20skills%20audit2.md)                                                                                     |
| `8aa6b282-85b9-4607-9abe-996c07227b48` | Merchant       | [merchant skills audit](../Dev%20Notes/class%20skills%20audit/merchant%20skills%20audit.md)                                                                                 |
| `a6be6838-bf7f-4bdc-bbc2-c372eb1347f4` | Knight         | [knight skill audit](../Dev%20Notes/class%20skills%20audit/knight%20skill%20audit.md)                                                                                       |
| `8325f9cf-2a2a-4fea-a4ae-77dd494c82f5` | Crusader       | [Crusader Skills Audit](../Dev%20Notes/class%20skills%20audit/Crusader%20Skills%20Audit%20and%20Plan%20Foundation%20Systems.md)                                             |
| `5e7bbe9b-ff37-4722-8718-7e42a413e966` | Wizard         | [wizard skill audit](../Dev%20Notes/class%20skills%20audit/wizard%20skill%20audit.md)                                                                                       |
| `65e95f94-ba6a-4a69-b5bb-d9a2c5517f08` | Sage           | [sage skill audit](../Dev%20Notes/class%20skills%20audit/sage%20skill%20audit.md)                                                                                           |
| `37dc4299-03c7-49b1-b7fb-796eea220d3f` | Hunter         | [hunter skill audit](../Dev%20Notes/class%20skills%20audit/hunter%20skill%20audit.md)                                                                                       |
| `280c1aca-34d3-4da1-bfbc-6139ada43f28` | Bard           | [bard skill audit](../Dev%20Notes/class%20skills%20audit/bard%20skill%20audit.md)                                                                                           |
| `a79a214d-65fc-493c-960a-02b875888cb8` | Dancer         | [dancer skills audit](../Dev%20Notes/class%20skills%20audit/dancer%20skills%20audit.md)                                                                                     |
| `56d65fef-4213-4b81-bd32-004e104da5a6` | Assassin+Monk  | [assasin skill audit](../Dev%20Notes/class%20skills%20audit/assasin%20skill%20audit.md) / [monk skill audit](../Dev%20Notes/class%20skills%20audit/monk%20skill%20audit.md) |
| `702d761a-efa0-4bcb-a343-ccc6bdbb2cc2` | Alchemist      | [alchemist skills audit](../Dev%20Notes/class%20skills%20audit/alchemist%20skills%20audit.md)                                                                               |
| `e70b2228-98c4-48c0-a735-e7d2b228f529` | Blacksmith     | [blacksmith skills audit](../Dev%20Notes/class%20skills%20audit/blacksmith%20skills%20audit.md)                                                                             |
|                                        |                |                                                                                                                                                                             |

## First Class Skill Fixes

| Resume ID | Class | Notes File |
|-----------|-------|-----------|
| `fa97e30b-3e66-4dc1-95c5-67e08b8fbd6b` | Novice | [Novice Fixes](../Dev%20Notes/Novice%20Fixes.md) |
| `099f839c-3136-4c28-adf6-f90b9765edc6` | Swordsman | [Swordsman Fixes](../Dev%20Notes/Swordsman%20Fixes.md) |
| `4e258de1-992a-440f-bbbe-257ad8674198` | Mage | [Mage Fixes](../Dev%20Notes/Mage%20Fixes.md) |
| `fb639c48-1a89-4c02-9d6f-d06760f834f0` | Archer | [Archer Fixes](../Dev%20Notes/Archer%20Fixes.md) |
| `98818c18-e21e-4cb2-ae6e-36229bb93320` | Acolyte | [Acolyte Fixes](../Dev%20Notes/Acolyte%20Fixes.md) |
| `aaf8cae1-9e42-4509-8340-0e5a9fb43e84` | Thief | [Thief Fixes](../Dev%20Notes/Thief%20Fixes.md) |
| `00af7d83-4b2e-4241-b968-338fe4cd7349` | Merchant | [Merchant Fixes](../Dev%20Notes/Merchant%20Fixes.md) |
| `8fbc7cc4-2469-480f-a6e0-0c9439ece817` | Generic cross-class | [first class skills fixes - generic](../Dev%20Notes/first%20class%20skills%20fixes%20-%20generic.md) |

## System Implementation Sessions

| Resume ID | System | Notes File |
|-----------|--------|-----------|
| `4a1da25f-f1c2-4b18-8ab1-04c6a52a50d6` | Card system | [card system implementation](../Dev%20Notes/card%20system%20implementation.md) |
| `74b11a9b-46fd-4753-96e7-9a11b3fb7f8a` | Card support systems | [adding extra systems to support cards](../Dev%20Notes/adding%20extra%20systems%20to%20support%20cards.md) |
| `4d4152cc-5080-429a-b504-9fd1e3e9b3ac` | Card naming | [slotted cards and items name research](../Dev%20Notes/slotted%20cards%20and%20items%20name%20research.md) |
| `c68817f7-6517-4997-bea0-4fcfc6634d47` | Card icons | [applying card icons](../Dev%20Notes/applying%20card%20icons%20to%20all%20cards%20and%20where%20they%20should%20display.md) |
| `ef974f8f-f3e3-48e0-9b36-fec6d3708e3a` | Ammunition | [ammunition system](../Dev%20Notes/ammunition%20system.md) |
| `de9a6613-ed8d-4b7c-ba35-0f2db306dc59` | Weight system | [weight system implementation](../Dev%20Notes/weight%20system%20implementation.md) |
| `8325f9cf-2a2a-4fea-a4ae-77dd494c82f5` | Party + Refine | [party and refine systems](../Dev%20Notes/party%20and%20refine%20systems.md) |
| `8aa6b282-85b9-4607-9abe-996c07227b48` | Vending | [vending system implementation](../Dev%20Notes/vending%20system%20implementation.md) |
| `58a3398e-4bed-458e-a5e0-1a29050885af` | Item inspect | [enhanced item inspect and ui feature](../Dev%20Notes/enhanced%20item%20inspect%20and%20ui%20feature.md) |
| `0f955300-5e35-4cd7-9cf1-8866db973053` | Equipment pipeline | [equipment and stat pipeline fixes](../Dev%20Notes/equipment%20and%20stat%20pipeline%20fixes.md) |
| `5312d21c-2d16-4fec-bd54-9b0ab4410dd4` | Item audit | [item stats and scripts audit](../Dev%20Notes/item%20stats%20and%20scripts%20audit.md) |
| `2e367db1-8178-4f89-b5ca-84edffb780e5` | Item insertion | [inserting cards and items into player inventory](../Dev%20Notes/inserting%20cards%20and%20items%20into%20player%20inventory.md) |
| `b39b5a45-396a-469c-acda-e56500a7f424` | Deferred systems | [Deferred Skills & Systems — Full plan](../Dev%20Notes/Deferred%20Skills%20&%20Systems%20—%20Full%20plan.md) |

## Architecture / Migration Sessions

| Resume ID | Topic | Notes File |
|-----------|-------|-----------|
| `7d482301-0a84-4391-959e-e49f16e6e4d5` | Targeting revamp P1 | [huge targeting revamp](../Dev%20Notes/huge%20targeting%20revamp.md) |
| `d602418a-6c1d-4718-93dd-55c718887e98` | Targeting revamp P2 | [huge targeting revamp phase 2](../Dev%20Notes/huge%20targeting%20revamp%20phase%202.md) |
| `6f800ad7-80a5-4508-8150-fa27b60fb156` | Targeting revamp P3+P5 | [phase 3](../Dev%20Notes/huge%20targeting%20revamp%20phase%203.md) / [phase 5](../Dev%20Notes/huge%20targeting%20revamp%20phase%205%20plus%20research.md) |
| `e1272615-9d2f-487b-824e-a55f2309034f` | BP bridge migration | [BP bridge migration](../Dev%20Notes/BP%20bridge%20migration%20for%20hotbar%20inventory%20loot%20chat.md) |

## Asset Generation Sessions

| Resume ID | Topic | Notes File |
|-----------|-------|-----------|
| `096eb4a7-5587-4873-ae2a-2d23d7cffd4e` | Item icons | [item icon generation](../Dev%20Notes/item%20icon%20generation.md) |
| `bbc39456-ba26-4fe2-9f81-8197c9c66f95` | Card icons | [card icon generation](../Dev%20Notes/card%20icon%20generation.md) |

## 3D-to-2D Sprite Pipeline

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"3d to 2d pipeline"` | Core pipeline build | Tripo3D → Mixamo → Blender render → post-process → atlas packer. Female Mage 168 sprites |
| `"swordsman and state dependent sprite atlases"` | Dual atlas + UE5 integration | 768 sprites, 20 anims, weapon-dependent atlases, SpriteCharacterActor C++ class |
| `"comfyUI sprite gen"` | ComfyUI attempt (abandoned) | AI image gen had consistency issues — 3D pipeline chosen instead |
| `"3d art research"` | Art pipeline research | Research on 3D art approaches for sprite generation |
| `"mage_f sprite atlases"` | Mage v2 atlas build | 34 Mixamo FBX anims, mage_f_v2.json config (many casting variants), 2336 sprites at 1024px. Packed + validated 2026-03-25 |
| `"sprite weapon layering system"` | Weapon overlay system | SOLVED: dual-pass render from same .blend, per-frame depth ordering, 5 C++ fixes, remote sync |
| `"state dependent sprite atlases standardization"` | Animation standardization | 17-anim standard for ALL classes, targetType cast, Bow mode, file reorg |
| `"female archer animations"` | Archer + bow weapon sprites | Archer_f 1136 + bow 1136 sprites, global depth ordering, depth offset 5.0 |
| `"Skeleton Sprite Animations"` | Skeleton enemy sprite | First humanoid monster, Mixamo rig, pure C++ sprite actor |
| `"poring sprite animations"` | Poring enemy sprite | First blob monster, shape key anims, render_monster.py, --model-rotation -90 |
| `"implementing more enemy sprites"` (04-13) | Enemy sprite batch + animation presets | ~20 enemy GLBs downloaded, 10 new shape key presets (~1000 lines in render_monster.py), spriteClass fields added to ~20 monster templates |
| `"ai generated rig and animations for non humanoids"` (04-13) | UniRig AI rigging pipeline | Installed UniRig (conda, 4 patches, RTX 5090 cu128). Proved on rocker: 40 bones, 6 procedural anims. New skill `/sabrimmo-rig-animate` |
| `"implementing more enemy sprites"` (04-19) | Cross-reference + batch 0 stationary sprites | `_prompts/lv1_50_model_matching.md` (~550 lines, direct/alias/shared/tint/none tags). 13 stationary sprites rendered (plants/mushrooms/eggs). Mass UE5 import of 35 atlas folders (04-13..04-15 backlog). `resume_sprite_pipeline_session.md` filed as re-entry prompt |
| `"implementing more enemy sprites"` (04-20) | Batches 2-9 production sprint | ~90 renders + ~70 UE5 imports. EnemySubsystem C++ hardening (SpriteTint, bCanMove, size→scale, spriteScale override, billboard ranged attack). Server all-5-emit-path parity. `render_monster.py --model-z-offset`. 195-line spawn grid. `.add_batchN.js` idempotent template-editor pattern |
| `"implementing more enemy sprites"` (04-21) | Batches 10-13 + plasma tint siblings | 28 atlas configs / renders / imports (magmaring → kraben). Plasma tint system scaled to 4 siblings (r/b/g/p). Late additions: vocal / blazzer / orc_baby. Progress: **150 done / 104 pending (GLB ready) / 255 no GLB / 509 total** |
| `"implementing more enemy sprites"` (04-22) | Tail-end imports + metaller | 6 atlas folders imported (alligator / blazzer / stone_shooter / plasma / metaller). `metaller` late render (Lv 22 insect, biped_insect preset). Closes out sprite-production week |

## Map System

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"map system research and planning"` | Complete map system (9 phases) | Minimap (134x134, SceneCapture, 5 zoom, draggable), World Map (12x8 grid, 62 zones), Loading Screen (Ken Burns, sparkles, progress bar), Guide NPC marks, /where command, preferences persistence. 6 new files, 5 socket events, ~1429 lines |

## Economy Systems

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"Storage"` | Kafra Storage (account-shared) | 10 socket handlers, StorageSubsystem + SStorageWidget, split/sort/auto-stack/search QoL, 40z fee, JSONB, 4 bugs fixed |
| `"sabrimmo-trading"` | Player-to-Player Trading | 9 socket handlers, TradeSession class, two-step confirm, 13 cancel paths, atomic transfer, trade_logs, /trade command, 4 bugs fixed |

## UX / Polish

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"options-menu-expansion"` | Options menu — 14 settings + SaveGame | OptionsSubsystem + SOptionsWidget, UOptionsSaveGame slot `SabriMMO_Options`, 4 sections (Display/Interface/Camera/Gameplay), FPS overlay, 9 target subsystems |
| `"character positioning tweaking"` (04-05) | Ground positioning + sprite snap | LocalSprite TWeakObjectPtr, name tag on sprite, fixed HalfHeight=96 in remote movement, billboard depth offset compensation, VFX spawn at visual center |
| `"character positioning tweaking"` (04-06) | Sprite-vs-world rendering — final solution | BLEND_Translucent + bDisableDepthTest + binary alpha, two camera→feet/head line traces + per-pixel depth, post-process cutout via PPI_CustomStencil, hardcoded 96 capsule, CameraSubsystem occlusion removed, motion blur off |
| `"Damage Number enhancements"` (04-05) | RO Classic numbers + hit impact | Sine arc / drift / scale, RO colors, status custom colors, hit flash, hit particles, crit starburst, combo total, target flinch, hit sounds. 6 phases + Phase A rewrite |
| `"Damage Number enhancements"` (04-06) | Combat polish — close 04-05 carryover | Crit starburst opacity/scale tuned, target flinch verified in-game, hit particle replaced with punchier Niagara |
| `"login screen"` | Login screen background + resolution | Deferred texture loading, FSlateBrush, ScaleToFill, standalone timing, camera 16:9 constraint |
| `"ESC menu"` | ESC menu + character select return | EscapeMenuSubsystem, two-phase player:leave, Z=200 background, bReturningToCharSelect |
| `"right click other players"` | Right-click player context menu | FMenuBuilder (Trade/Party/Whisper), camera drag vs quick-click, PlayerInputSubsystem routing |
| `"icons"` | Item icon mapping (6169 items) | Subfolder IconAssetMap, underscore fallback, all categories mapped |

## 3D World Building

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"ground-texture-and-materials-system"` | Ground textures + materials + decals + grass | 3-day session (3/30-4/01). 1000+ AI textures, 608 RO originals, 17 material versions, 2700+ variants, DBuffer decals (91), Landscape Grass V3 (60 sprites, 13 zones), 80+ scripts, 3 new skills, 5 _meta guides |

## Audio Research / Implementation

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"audio research"` (04-06) | RO Classic audio — deep research | Full architecture (GRF/BGM/mp3nametable/ACT-EffectTable-RSW), 121 pre-renewal tracks, SFX inventory, composer credits, filename misspellings, **legal status (Gravity v. NovaRO $4M 2022)**, recommended Tier 1-3 sourcing. Output: `docsNew/05_Development/RO_Audio_System_Research.md` + memory `ro-audio-system.md` |
| `"enemy audio"` (04-07) | Enemy/monster audio | AudioSubsystem: body material layering, 60 monsters wired, variant SFX (TArray + random), ROSFX/effect/ 626 files |
| `"player audio"` (04-07) | Player audio | Per-weapon swing/hit, per-class fallback, body material reaction, level up chime, heal sound, local + remote |
| `"audio research"` (04-07) | BGM zone mapping + skill fix | Zone_BGM_Table.md, /sabrimmo-audio misattribution fixed, 121 BGM tracks sourced, SkillImpactSoundMap wiring, 3 new skills |

## Ground Item / Loot System

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"ground item system"` (04-09) | Server-side ground items | RO Classic ownership phases, scatter offsets, 60s despawn, party share, damage ranking, 6 socket events, enemy death integration |
| `"ground item client"` (04-10) | Client-side ground items | GroundItemSubsystem (368 lines) + GroundItemActor (230 lines), billboard sprites, tier-color tinting, click-to-pickup, spawn arc |

## Inventory Performance

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"inventory performance"` (04-10) | Performance analysis + Phase 1 start | 10-finding root cause report, 4-phase optimization plan. Started: const ref GetFilteredItems, RebuildFilteredCache, ItemDefCache, HandleItemDefs |

## Minimap / Map Fixes

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"minimap fix"` (04-08) | SceneCapture2D + cutout PP fix | SetPostProcessMaterial(false), regression protection in 5 locations (skill, CLAUDE.md, docs, inline comment, memory) |

## Hosting / Packaging / Distribution

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"host package distribute research"` (04-16) | 3-pillar research: hosting + UE5 packaging + invite-only distribution | 4 deliverable docs (~153 KB) authored in one session via parallel Explore agents. Executive recommendation: self-host on own hardware for Scenarios A (5 friends / Tailscale) and B (20-50 / Cloudflare Tunnel), VPS only at 100+ CCU (Hetzner CCX23 US $36 or AX41 dedicated $55). Rejected: Steam private (Gravity RO IP exposure risk, Dreadmyst 2026 precedent), AWS/Azure/GCP (burstable throttle + egress cliff), Upstash PAYG (~$13K/mo worst case). itch.io Restricted + butler = invite-only distribution answer. Master plan: `Hosting_Packaging_Distribution_Plan.md` |

## World Building / Zones

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"spawn region system"` (05-02) | Visual spawn-area editor | `ASpawnAllowVolume` + `ASpawnDenyVolume` placeable actors with `MonsterFilter` + `RegionTag`. `ExportSpawnRegions <zone>` console command writes `server/spawn_regions/<zone>.json`. Server loader picks weighted-random points (∝ XY area), rejects deny-box overlaps, NavMesh ground-snaps (500 UU halfExtent on Z). `Spawn_Region_System.md` 16 KB. `enemySpawns[]` and `spawnPool[]` coexist |
| `"water system mixed mode"` (05-06) | Mixed-mode `AWaterArea` | Auto-detected per-cell deep via downward raycast grid (default 16x16 = 256 samples). Greedy rect-merge → 1 `UBoxComponent` per rectangle as `NavArea_Null` modifier (cuts navmesh AND server-side OBJ). Three-stop depth gradient: shallow (0-250 UU) → deep → abyss (900 UU, opacity 1.0). Reference-counted server tracking via `Map<areaId,{isDeep}>`. Two-layer movement validation (deep-water AABB fast-path + off-navmesh fallback). `spawnEnemy()` snap-and-warn (>100 UU snap = designer typo warning) |
| `"new zone setup"` (05-07) | Server `spawnPool` integration + grassfield05/07 + Sewer Dungeon scaffold | `index.js` +402/-60 lines: `resolveRespawnMs` (number OR `[min,max]` range), `spawnZoneEnemies` called from all 7 zone-first-load paths, `isInDeepWater` AABB, water:enter/exit ref counting, two-layer movement validation. Per-template respawn ranges (blue_plant/green_plant/black_mushroom/vocal). NavMesh exports for grassfield05 + grassfield07 + prt_south rebuild. `prtsouth_to_dungeon` portal repurposed → `prtsouth_to_sewerdungeon01`. AudioSubsystem zone mappings for prt_sewb1-4 + SewerDungeon01 (DunWind+CaveDrip+CaveFall + bgm_19) |
| `"sewer dungeon build"` (05-08) | Sewer Dungeon F1 build plan + PostProcess + ZoneTransition preload-wait | `Sewer_Dungeon_01_Build_Plan.md` 82 KB / 20 sections / 14 phases — RO Classic prt_sewb1 mimic. PostProcess presets for grassfield07 / grassfield05 / SewerDungeon01 + 7000-UU directional shadow cap (3 cascades, fights 15° FOV cascade seams). SewerDungeon01 fog: density 0.30 / falloff 0.2 / moss-green inscatter / FogHeight=500. ZoneTransition `WaitForPreloadAndHideOverlay` — 700ms stability + 1.5s minimum + 15s timeout, polls `UZonePreloadSubsystem::IsLoadingInProgress` every 200ms. `ShowExpectedZoneChange` / `HideExpectedZoneChange` API for Kafra / Butterfly / RequestWarp optimistic overlay. `_prompts/new_zone_creation.md` 22 KB canonical bootstrap |

## 3D Asset Generation (Hunyuan3D 2.1)

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"hunyuan3d test tree"` (05-02) | Single-asset proof | `_tools/hunyuan_test_tree.py` — first end-to-end SDXL → rembg BiRefNet → `Hy3D21VAEDecode mc_algo="mc"` (NOT dmc — broken on torch 2.12/sm_120). Depth ratio 0.97 |
| `"hunyuan asset pipeline"` (05-03 → 05-06) | Production pipeline + 235 base + ~828 variants | Multi-input mandatory (2 strategies × 2 seeds = 4 candidates). Decimate-FIRST then texture (30× speedup). Quality-aware scoring (cube_artifact=0.001). Backup OUTSIDE `asset_dir`. `comfyui_supervisor.py` auto-restart + UTF-8 logger patch. KJNodes pinned to commit `37a0973`. Inherent failures (thin shapes) accepted after 8 attempts. Multi-object detection v2 (centroid spread + bbox overlap heuristics). `_tools/hunyuan_asset_pipeline.py` 48 KB master + `hunyuan_overnight_continue.py` 32 KB + `hunyuan_cleanup.py` (79%→96% GOOD in 3 passes) + `hunyuan_color_variants.py` 65 KB + `hunyuan_batch3_100.py` 44 KB + `hunyuan_v2_regen.py` 9 KB + `hunyuan_v3_regional.py` 140 KB + `hunyuan_alt_meshes_regen.py` 11 KB + `post_alt_mesh_pipeline.py` 5 KB. `3D_World_Asset_Pipeline_2026.md` ~1,460 lines + `Session_Record_2026-05-03.md` 24 KB + `REUSABLE_SESSION_PROMPT.md` 7 KB. `_meta/RO_Classic_World_Assets_Expanded.md` 15 KB (9 niche regions cataloged) |

## Code Audits

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"deep audit"` | Deep code audit | 43 audit files in `_audit/`: code quality, security, performance, integration, monsters |
| `"broad audit"` | Broad code audit | Master reports: `MASTER_AUDIT_REPORT.md`, `MASTER_AUDIT_REPORT_R2.md` |

## Saved Sessions Index

See also: [saved claude sessions](../Dev%20Notes/saved%20claude%20sessions.md) — the original session log file
