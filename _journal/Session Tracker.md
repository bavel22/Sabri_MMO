# Claude Code Session Tracker

All Claude Code sessions with their resume IDs, organized by topic. Use `claude --resume <id>` to continue any session.

> **Tip**: After each Claude Code session, add an entry here with the resume ID and a one-line summary of what was accomplished.

---

## Active / Recent

| Date | Resume ID | Topic | Notes |
|------|-----------|-------|-------|
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

## Code Audits

| Resume ID | Topic | Notes |
|-----------|-------|-------|
| `"deep audit"` | Deep code audit | 43 audit files in `_audit/`: code quality, security, performance, integration, monsters |
| `"broad audit"` | Broad code audit | Master reports: `MASTER_AUDIT_REPORT.md`, `MASTER_AUDIT_REPORT_R2.md` |

## Saved Sessions Index

See also: [saved claude sessions](../Dev%20Notes/saved%20claude%20sessions.md) — the original session log file
