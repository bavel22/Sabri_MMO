# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Sabri_MMO** — Class-based action MMORPG inspired by Ragnarok Online Classic.
**Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
**Architecture**: Server-authoritative — all combat, stats, inventory, and positions are validated server-side. The client is presentation + input only.

Full rules and design standards: `docsNew/00_Global_Rules/Global_Rules.md`
Architecture reference: `docsNew/00_Project_Overview.md`
RO Classic game design reference: `RagnaCloneDocs/` (28 files — 14 design + 14 implementation guides)
Documentation index: `docsNew/INDEX.md`
Dashboard: `_journal/Dashboard.md`
Session tracker: `_journal/Session Tracker.md`
Prompt library: `_prompts/README.md`

---

## Session Workflow

**At the start of each session**, check `_journal/` for the most recent daily note. If the user has written goals, blockers, or context there, use it to inform your work.

**At the end of a session** (when the user says they're done, or after completing a major task), offer to:
1. Update `_journal/Session Tracker.md` with the current session's resume ID and summary
2. Update `_journal/Dashboard.md` if completed items or next steps changed
3. Move any reusable prompts to `_prompts/`
4. Update `docsNew/` if any system documentation is now outdated

---

## Commands

### Server
```bash
cd server
npm run dev        # Development (nodemon auto-restart)
npm start          # Production
```
Requires `.env` at `server/.env` with `DB_*`, `JWT_SECRET`, `REDIS_URL`.

### Database
```sql
-- Initial setup (run in pgAdmin against sabri_mmo DB)
\i database/init.sql

-- Migrations (run in order as needed)
\i database/migrations/<migration>.sql
```
The server auto-creates missing stat columns on startup.

### UE5 Client
Build via Unreal Editor or:
```
UnrealBuildTool SabriMMO Win64 Development "C:/Sabri_MMO/client/SabriMMO/SabriMMO.uproject"
```
`NetworkPrediction` module is intentionally excluded due to a UE5.7 compiler bug.

---

## Architecture

### Server (`server/src/index.js`)
Single monolithic file (~30000 lines). Key sections: REST API (JWT auth, character CRUD), Socket.io events (`player:*`, `combat:*`, `inventory:*`, `chat:*`, `enemy:*`, `stats:*`, `skills:*`), combat tick loop (50ms, ASPD-based), enemy AI loop (509 templates, 46 spawn points), class systems (20 classes), consumable/card/ensemble/ground effect handlers. 17 data modules (`ro_monster_templates`, `ro_skill_data`, `ro_card_effects`, `ro_damage_formulas`, etc.). Load `/full-stack` for details.

### Client C++ (`client/SabriMMO/Source/SabriMMO/`)

**Core infrastructure:**

| File | Role |
|------|------|
| `CharacterData.h` | `FCharacterData`, `FServerInfo`, `FInventoryItem`, drag-drop structs |
| `MMOGameInstance.*` | Auth state, character list, persistent socket (ConnectSocket/EmitSocketEvent/EventRouter) |
| `MMOHttpManager.*` | BlueprintFunctionLibrary — REST API calls |
| `SabriMMOCharacter.*` | Local player pawn — movement, stats, BeginPlay ground-snap |
| `SabriMMOPlayerController.*` | Input mapping (click-to-move + WASD) |
| `SocketEventRouter.*` | Multi-handler Socket.io event dispatch for subsystems |
| `OtherCharacterMovementComponent.*` | Remote player interpolation + per-tick floor-snap |

**30+ UWorldSubsystem files in `UI/`** — each manages one domain with a paired Slate widget. **3 VFX files in `VFX/`**. Load the relevant `/sabrimmo-*` skill for per-subsystem details.

### Database (PostgreSQL)
Core tables: `users`, `characters`, `items` (static definitions), `character_inventory` (per-character), `character_hotbar`, `character_cart`, `character_homunculus`, `parties`, `party_members`, `vending_shops`, `vending_items`. 25 migration files in `database/migrations/`.
Characters are soft-deleted (`deleted = TRUE`). All queries filter `AND deleted = FALSE`.

---

## Mandatory Blueprint Workflow

**ALWAYS use unrealMCP before any Blueprint work — no exceptions.**

```
1. mcp__unrealMCP__read_blueprint_content({ blueprint_path: "/Game/..." })
2. Base all analysis/changes on the actual unrealMCP output
3. Never assume variable names, types, or component structure
```

Blueprint assets live in `client/SabriMMO/Content/SabriMMO/` (not tracked in git as binary `.uasset` files).
Widget prefix: `WBP_`. Blueprint prefix: `BP_`. Interface prefix: `BPI_`.

---

## Key Design Patterns

**GameInstance for persistence** — `UMMOGameInstance` holds auth state, character data, and stats. Never store cross-level state in PlayerController or GameMode.

**Manager pattern** — One `UWorldSubsystem` per domain. TMap registries with `Get*()`/`Register*()`/`Unregister*()` lookups.

**Interfaces over cast chains** — Use `BPI_Damageable`, `BPI_Interactable`, `BPI_Targetable` instead of `Cast To BP_Enemy` / `Cast To BP_Player` chains.

**Event dispatchers over Tick polling** — Widgets bind to `OnHealthChanged`, `OnStatsUpdated`, etc. Do not poll state on Tick.

**C++ struct registries over BP property reflection** — Entity data lives in C++ structs (`FEnemyEntry`, `FPlayerEntry`), read via subsystem API (`GetEnemyData()`, `GetPlayerData()`). Never use `FindPropertyByName`.

**Component-based** — New gameplay features get a dedicated `UActorComponent`. Do not add movement + combat + inventory logic to one Actor.

**UPROPERTY on loaded classes** — Any `UClass*` obtained via `LoadClass<>()` or `LoadObject<>()` and stored as a member variable MUST be marked `UPROPERTY()`. Without it, UE5's garbage collector cannot see the reference, will collect the class, and the subsystem crashes with an access violation when it tries to use the stale pointer. This applies to `EnemyBPClass`, `PlayerBPClass`, and any future runtime-loaded class references.

**Zone filtering on all AoE loops** — Every server-side AoE skill handler that iterates `enemies.entries()` or `connectedPlayers.entries()` MUST filter by `enemy.zone !== zone` / `ptarget.zone !== zone`. The `enemies` Map is global (all zones). Without zone filtering, enemies from other zones at overlapping coordinates get included, diluting split damage (Napalm Beat) or dealing invisible damage to unreachable targets.

**Auto-create DB columns at server startup** — Any new column added to a migration file MUST also be added to the server startup auto-creation block (near `// Ensure stat columns exist`). The `getPlayerInventory()` query and similar functions reference these columns — if the migration hasn't been run, the query fails and returns empty data. Auto-creation with `ADD COLUMN IF NOT EXISTS` makes the server self-healing.

**Magic Rod absorption in single-target magic paths** — Every single-target magical damage path must check for `magic_rod` buff before applying damage. If active, absorb SP and nullify.

**Ensemble ground effects as stationary AoE zones** — Ensembles create fixed-position ground effects at the two performers' midpoint. Both must stay in range or the ensemble ends.

**Remote visual sync via zone:ready, NOT player:join** — Visual data (weapon sprites, equipment, buffs, mounts) MUST be sent in `zone:ready`, NOT `player:join`. During `player:join` the client is doing OpenLevel — subsystems are destroyed/recreated, events dropped silently. Also beware JS temporal dead zone with `let` in the early broadcast block.

**Shared armature + gender-aware equipment** — Two base bodies (`base_m`, `base_f`), all classes share the armature. Equipment renders per gender, not per class. `LoadEquipmentLayer` searches: `{item_subdir}/{GenderSubDir}/` → `{item_subdir}/` → root. Full doc: `docsNew/05_Development/Shared_Armature_Sprite_Architecture.md`.

**Deferred widget creation in standalone mode** — Standalone viewport reports 0x0 and textures load as 32x32 during `OnWorldBeginPlay`. Widgets depending on viewport size or textures MUST use deferred retry (see `LoginFlowSubsystem::TryLoadBackgroundTexture()`).

**FSlateBrush over FSlateDynamicImageBrush** — `FSlateDynamicImageBrush` is deprecated in UE5.7. Use `FSlateBrush` + `SetResourceObject()`. Keep `UTexture2D` alive via `UPROPERTY()`.

**SceneCapture2D must disable post-process materials** — Any `USceneCaptureComponent2D` using `SCS_FinalColorLDR` MUST call `ShowFlags.SetPostProcessMaterial(false)`. The global cutout post-process material darkens everything without stencil=1, which breaks non-main-camera captures (minimap, portraits).

---

## Persistent Socket Architecture

`UMMOGameInstance` owns a `TSharedPtr<FSocketIONative>` that survives `OpenLevel()` — no reconnect on zone transitions. `USocketEventRouter` provides multi-handler dispatch. Subsystems register via `Router->RegisterHandler()` in `OnWorldBeginPlay` and `Router->UnregisterAllForOwner(this)` in `Deinitialize`. BP emit: `GI->K2_EmitSocketEvent()`. All subsystem widgets gated behind `GI->IsSocketConnected()`. Load `/sabrimmo-persistent-socket` for full details.

---

## Naming Conventions

- Variables: `camelCase` locals, `PascalCase` class/component names
- Booleans: `IsConnected`, `HasWeapon`, `CanAttack`
- Constants: `MAX_PLAYERS`, `BASE_ATTACK_SPEED`
- Functions: verb phrases in PascalCase — `SpawnPlayer`, `GetCharacterStats`
- No abbreviations: `Initialize` not `Init`, `Calculate` not `Calc`

---

## Mandatory Context Loading

**BEFORE starting any task, ALWAYS load the relevant skills and documentation for that task.** This project is large and growing — context from skills and docs prevents regressions, missed patterns, and architectural drift.

### How to Load Context

1. **Identify which systems the task touches** (UI? Server? VFX? Zones? Combat? Stats? Items? etc.)
2. **Load the matching skill(s)** using the Skill tool — skills contain architectural rules, file locations, naming conventions, and common patterns that MUST be followed
3. **Read relevant source files** before modifying them — never assume code structure
4. **Check `docsNew/`** for system-specific documentation when the skill references it
5. **Check `RagnaCloneDocs/`** for RO Classic game design reference when implementing new game systems

### Skill Selection Guide

| If the task involves... | Load these skills | Also read these docs |
|------------------------|-------------------|---------------------|
| Crashes, errors, bugs | `/debugger` | — |
| Build, compile, UBT, linker error, DLL, Live Coding | `/sabrimmo-build-compile` | — |
| Server code, DB, REST API | `/full-stack` | `docsNew/00_Project_Overview.md` |
| Socket.io events, multiplayer sync | `/realtime` | — |
| Persistent socket, EventRouter, BP bridge | `/sabrimmo-persistent-socket` | `memory/persistent-socket.md` |
| Blueprint / Widget work | `/ui-architect` | unrealMCP first |
| NavMesh pathfinding, enemy movement | `/sabrimmo-navmesh` | `docsNew/05_Development/NavMesh_Pathfinding_Implementation_Plan.md` |
| Enemy AI, monster behavior | `/enemy-ai` | `server/src/ro_monster_ai_codes.js` |
| Enemy sprites, spawn, targeting, death | `/sabrimmo-enemy` | `docsNew/03_Server_Side/Enemy_System.md` |
| Enemy sprite rendering pipeline | `/sabrimmo-3d-to-2d` | `docsNew/03_Server_Side/Enemy_System.md` |
| Non-humanoid rigging + animation (UniRig pipeline) | `/sabrimmo-rig-animate` | — |
| Sprite system (atlas, layers, animation states) | `/sabrimmo-sprites` | `docsNew/05_Development/Enemy_Sprite_Implementation.md` |
| Sprite weapon overlays, depth ordering | `/sabrimmo-sprites` | `docsNew/05_Development/Weapon_Sprite_Overlay_Pipeline.md` |
| Monster skill casting, NPC_ skills | `/sabrimmo-monster-skills` | `docsNew/03_Server_Side/Monster_Skill_System.md` |
| Slate UI panels | `/sabrimmo-ui` | — |
| Skill targeting (click-to-cast) | `/sabrimmo-target-skill` | — |
| Chat messages, chat UI, channels | `/sabrimmo-chat` | — |
| Combat log, damage/buff/death messages | `/sabrimmo-combat-log` | — |
| Clickable NPCs, interactables | `/sabrimmo-click-interact` | — |
| Right-click player context menu | `/sabrimmo-right-click-player-context` | — |
| 3D world, post-process, lighting, exposure | `/sabrimmo-3d-world` | `docsNew/05_Development/3D_World_Implementation_Plan.md` |
| Water areas, water material, water skills | `/sabrimmo-water` | `docsNew/05_Development/Water_System_Research.md` |
| Landscape Actor, terrain sculpting, zone terrain setup | `/sabrimmo-landscape` | `_meta/01_Landscape_Guide.md` |
| Ground textures, materials, biome variants | `/sabrimmo-ground-textures` | `docsNew/05_Development/Ground_Texture_System_Research.md`, `docsNew/05_Development/Material_Variant_Tracker.md` |
| Terrain decals, ground detail, dirt/moss/crack overlays | `/sabrimmo-material-decals` | `_meta/04_Decals_Guide.md` |
| Landscape grass, flowers, pebbles, detail mesh scatter | `/sabrimmo-environment-grass` | `_meta/03_Scatter_Objects_Guide.md` |
| ESC menu, character select return, logout | `/sabrimmo-esc-menu` | — |
| Options menu, game settings, FPS counter, SaveGame persistence | `/sabrimmo-options` | — |
| Login screen, login background, character select UI | `/sabrimmo-login-screen` | — |
| Resolution, aspect ratio, DPI, standalone squishing | `/sabrimmo-resolution` | — |
| Zones, maps, warp portals | `/sabrimmo-zone` | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` |
| Minimap, world map, loading screens, guide NPC marks | `/sabrimmo-map` | `docsNew/05_Development/Map_System_Implementation_Plan.md` |
| VFX, particles, Niagara | `/sabrimmo-skills-vfx` | `docsNew/05_Development/VFX_Asset_Reference.md` |
| Skill icon art generation | `/sabrimmo-generate-icons` | — |
| Stats, leveling, class system | `/sabrimmo-stats` | `RagnaCloneDocs/01_Stats_Leveling_JobSystem.md` |
| Death penalty (PvE EXP loss) | `/sabrimmo-death` | `docsNew/05_Development/Phase_7_9_12_Completion_Plan.md` |
| MVP system, monster skills, slaves | `/sabrimmo-mvp` | `docsNew/03_Server_Side/Monster_Skill_System.md` |
| Damage pipelines, combat formulas | `/sabrimmo-combat` | `RagnaCloneDocs/02_Combat_System.md` |
| Damage numbers, floating combat text | `/sabrimmo-damage-numbers` | `docsNew/05_Development/Damage_Number_RO_Classic_Research.md` |
| Buffs (Provoke, Blessing, etc.) | `/sabrimmo-buff` | `docsNew/03_Server_Side/Status_Effect_Buff_System.md` |
| Status effects (stun, freeze, etc.) | `/sabrimmo-debuff` | `docsNew/03_Server_Side/Status_Effect_Buff_System.md` |
| Skill trees, cast times, cooldowns | `/sabrimmo-skills` | `RagnaCloneDocs/03_Skills_Magic_System.md` |
| Novice skills (IDs 1-3) | `/sabrimmo-skill-novice` | `docsNew/05_Development/Novice_Skills_Audit_And_Fix_Plan.md` |
| Swordsman skills (IDs 100-109) | `/sabrimmo-skill-swordsman` | `docsNew/05_Development/Swordsman_Skills_Audit_And_Fix_Plan.md` |
| Mage skills (IDs 200-213) | `/sabrimmo-skill-mage` | `docsNew/05_Development/Mage_Skills_Audit_And_Fix_Plan.md` |
| Archer skills (IDs 300-306) | `/sabrimmo-skill-archer` | `docsNew/05_Development/Archer_Skills_Audit_And_Fix_Plan.md` |
| Acolyte skills (IDs 400-414) | `/sabrimmo-skill-acolyte` | `docsNew/05_Development/Acolyte_Skills_Audit_And_Fix_Plan.md` |
| Thief skills (IDs 500-509) | `/sabrimmo-skill-thief` | `docsNew/05_Development/Thief_Skills_Audit_And_Fix_Plan.md` |
| Merchant skills (IDs 600-609) | `/sabrimmo-skill-merchant` | `docsNew/05_Development/Merchant_Skills_Audit_And_Fix_Plan.md` |
| Knight skills (IDs 700-710) | `/sabrimmo-skill-knight` | `docsNew/05_Development/Knight_Class_Research.md` |
| Crusader skills (IDs 1300-1313) | `/sabrimmo-skill-crusader` | `docsNew/05_Development/Crusader_Class_Research.md` |
| Wizard skills (IDs 800-813) | `/sabrimmo-skill-wizard` | `docsNew/05_Development/Wizard_Class_Research.md` |
| Sage skills (IDs 1400-1421) | `/sabrimmo-skill-sage` | `docsNew/05_Development/Sage_Class_Research.md` |
| Hunter skills (IDs 900-917) | `/sabrimmo-skill-hunter` | `docsNew/05_Development/Hunter_Class_Research.md` |
| Bard skills (IDs 1500-1537) | `/sabrimmo-skill-bard` | `docsNew/05_Development/Bard_Class_Research.md` |
| Dancer skills (IDs 1520-1557) | `/sabrimmo-skill-dancer` | `docsNew/05_Development/Dancer_Skills_Audit_And_Fix_Plan.md` |
| Priest skills (IDs 1000-1018) | `/sabrimmo-skill-priest` | `docsNew/05_Development/Priest_Class_Research.md` |
| Monk skills (IDs 1600-1615) | `/sabrimmo-skill-monk` | `docsNew/05_Development/Monk_Class_Research.md` |
| Assassin skills (IDs 1100-1111) | `/sabrimmo-skill-assassin` | `docsNew/05_Development/Assassin_Class_Research.md` |
| Rogue skills (IDs 1700-1718) | `/sabrimmo-skill-rogue` | `docsNew/05_Development/Rogue_Class_Research.md` |
| Blacksmith skills (IDs 1200-1230) | `/sabrimmo-skill-blacksmith` | `docsNew/05_Development/Blacksmith_Class_Research.md` |
| Alchemist skills (IDs 1800-1815) | `/sabrimmo-skill-alchemist` | `docsNew/05_Development/Alchemist_Class_Research.md` |
| Ground items, item drops, pickup, loot | `/sabrimmo-item-drop-system` | `docsNew/05_Development/Ground_Item_And_Drop_System_Research.md` |
| Items, equipment, refining | `/sabrimmo-items` | `RagnaCloneDocs/06_Items_Equipment.md` |
| Refine ATK bonus, overupgrade, armor DEF | `/sabrimmo-refine` | `docsNew/05_Development/Refine_ATK_And_Party_System_Plan.md` |
| Arrows, ammo equip, element override, consumption | `/sabrimmo-ammunition` | — |
| Weight thresholds, overweight penalties | `/sabrimmo-weight` | `docsNew/03_Server_Side/Inventory_System.md` |
| Card compounding, card bonuses | `/sabrimmo-cards` | — |
| Pharmacy crafting, Arrow Crafting, recipes | `/sabrimmo-crafting` | `docsNew/05_Development/Alchemist_Deferred_Skills_Implementation_Plan.md` |
| Summon Flora, Marine Sphere, summon entities | `/sabrimmo-companions` | `docsNew/05_Development/Alchemist_Deferred_Skills_Implementation_Plan.md` |
| NPCs, shops, quests, Kafra | `/sabrimmo-npcs` | `RagnaCloneDocs/08_NPCs_Quests.md` |
| Kafra storage, deposit, withdraw, account storage | `/sabrimmo-storage` | `docsNew/05_Development/Kafra_Storage_And_Trading_Implementation_Plan.md` |
| Party system, EXP share, party chat | `/sabrimmo-party` | `docsNew/05_Development/Refine_ATK_And_Party_System_Plan.md` |
| Guild system, ranks, skills, storage | `/sabrimmo-guild` | `RagnaCloneDocs/07_Social_Systems.md` |
| PvP, War of Emperium, siege | `/sabrimmo-pvp-woe` | `RagnaCloneDocs/05_PvP_GvG_WoE.md` |
| Trading, vending, storage, zeny | `/sabrimmo-economy` | `RagnaCloneDocs/09_Economy_Trading.md` |
| Player-to-player trade window | `/sabrimmo-trading` | `docsNew/05_Development/Kafra_Storage_And_Trading_Implementation_Plan.md` |
| Pets, homunculus, mercenaries | `/sabrimmo-companions` | `RagnaCloneDocs/12_Pets_Homunculus_Companions.md` |
| Audio, BGM, SFX, sound design (parent — BGM/ambient/zones/options) | `/sabrimmo-audio` | `RagnaCloneDocs/13_Audio_Sound_Design.md`, `docsNew/05_Development/RO_Audio_System_Research.md` |
| Monster sounds, body material layering, frame-sync move SFX, status sounds | `/sabrimmo-audio-enemy` | `docsNew/05_Development/RO_Audio_System_Research.md` |
| Player sounds (local + remote), weapon-type swing/hit, per-class fallback, body reaction, level up, heal | `/sabrimmo-audio-player` | `docsNew/05_Development/RO_Audio_System_Research.md` |
| Legacy combat hit sound thunks, supplementary hit audio | `/sabrimmo-audio-combat` | — |
| Art, models, animations, hair | `/sabrimmo-art` | `RagnaCloneDocs/14_Art_Visual_Style.md` |
| 3D-to-2D sprite pipeline, Blender render, Tripo3D | `/sabrimmo-3d-to-2d` | `2D animations/SESSION_CONTEXT.md` |
| New feature planning | `/planner` | `RagnaCloneDocs/00_Master_Build_Plan.md` |
| Complex architecture decisions | `/opus-45-thinking` | `/project-docs` |
| Full project context dump | `/project-docs` | All of `docsNew/` |
| Code style, refactoring, audits | `/code-quality` | — |
| Security, auth, anti-cheat | `/security` | — |
| Performance, FPS, tick rate | `/performance` | — |
| Server tests, quick validation | `/test` | — |
| Full QA, integration testing | `/tester` | — |
| Documentation updates | `/docs` | `docsNew/` |
| Prompt engineering, skill writing | `/prompt-engineer` | — |
| Deep web research | `/deep-research` | — |

### Multi-System Tasks

Many tasks touch multiple systems. **Load ALL relevant skills.** Examples:
- "Add a new skill with VFX" -> `/sabrimmo-skills` + `/sabrimmo-combat` + `/sabrimmo-skills-vfx` + `/full-stack`
- "Fix any class skill" -> `/sabrimmo-skill-{class}` + `/sabrimmo-skills` + `/sabrimmo-combat` (if damage) or `/sabrimmo-debuff` (if status) or `/sabrimmo-buff` (if buff)
- "Add a new zone with NPCs and warps" -> `/sabrimmo-zone` + `/sabrimmo-npcs` + `/sabrimmo-click-interact` + `/sabrimmo-3d-world`
- "Scene/lighting/post-process issues" -> `/sabrimmo-3d-world` + `/debugger` (+ `/sabrimmo-zone` if per-zone preset, + `/sabrimmo-persistent-socket` if zone transition)
- "Ground textures/tiling" -> `/sabrimmo-ground-textures` (+ `/sabrimmo-zone` + `/sabrimmo-navmesh` if building full zone)
- "Water issues" -> `/sabrimmo-water` + `/sabrimmo-zone` (+ `/sabrimmo-skill-acolyte` if Aqua Benedicta)
- "Damage number issues" -> `/sabrimmo-damage-numbers` + `/debugger` (+ `/sabrimmo-combat` if new type, + `/sabrimmo-persistent-socket` if not showing)
- "Fix a crash when casting spells" -> `/debugger` + `/sabrimmo-skills` + `/realtime`
- "Build a new HUD panel showing buffs" -> `/sabrimmo-buff` + `/sabrimmo-debuff` + `/sabrimmo-ui`
- "Add a skill that applies poison" -> `/sabrimmo-debuff` + `/sabrimmo-skills` + `/full-stack`
- "Add a buff skill like Blessing" -> `/sabrimmo-buff` + `/sabrimmo-skills` + `/full-stack`
- "Add a new monster with special attacks" -> `/enemy-ai` + `/sabrimmo-monster-skills` + `/sabrimmo-combat` + `/full-stack`
- "Sprite issues (not showing, floating, wrong anim, weapon Z-fighting)" -> `/sabrimmo-sprites` + `/sabrimmo-enemy` + `/debugger` (+ `/sabrimmo-3d-to-2d` if render pipeline, + `/sabrimmo-persistent-socket` if remote player)
- "Add new sprites (monster, class, weapon)" -> `/sabrimmo-sprites` + `/sabrimmo-3d-to-2d` + `/sabrimmo-enemy` + `/sabrimmo-art`
- "Rig and animate a non-humanoid enemy" -> `/sabrimmo-rig-animate` + `/sabrimmo-enemy` + `/sabrimmo-sprites` (+ `/sabrimmo-3d-to-2d` if sprite rendering)
- "Enemy issues (click/target, respawn)" -> `/sabrimmo-enemy` + `/debugger`
- "Implement the inventory system" -> `/sabrimmo-items` + `/sabrimmo-economy` + `/full-stack` + `/sabrimmo-ui`
- "Regen not working / can't attack" -> `/debugger` + `/sabrimmo-weight` + `/sabrimmo-buff`
- "Party issues (EXP, chat, Devotion)" -> `/sabrimmo-party` + `/debugger` (+ `/sabrimmo-stats` if EXP, + `/sabrimmo-chat` if chat, + `/sabrimmo-skill-crusader` if Devotion)
- "Refine issues (ATK, overupgrade, Shield Boomerang)" -> `/sabrimmo-refine` + `/sabrimmo-combat` + `/debugger`
- "Implement pet taming system" -> `/sabrimmo-companions` + `/sabrimmo-items` + `/full-stack`
- "Set up WoE castle sieges" -> `/sabrimmo-pvp-woe` + `/sabrimmo-guild` + `/sabrimmo-combat`
- "Add NPC shops and quest givers" -> `/sabrimmo-npcs` + `/sabrimmo-items` + `/sabrimmo-economy`
- "Storage issues" -> `/sabrimmo-storage` + `/debugger` (+ `/sabrimmo-items` if deposit/withdraw, + `/sabrimmo-npcs` if Kafra UI, + `/sabrimmo-economy` if cart transfers)
- "BGM issues (not playing, wrong track, no loop, zone music)" -> `/sabrimmo-audio-player` + `/debugger` (+ `/sabrimmo-zone` if per-zone, + `/sabrimmo-login-screen` if login)
- "Volume/mute/options audio" -> `/sabrimmo-audio-player` + `/sabrimmo-options`
- "Player SFX (swing, hit, level up, heal, equip, UI click, potion)" -> `/sabrimmo-audio-player` + target system skill (combat/items/stats/ui)
- "Monster SFX (attack, die, move, body material, status)" -> `/sabrimmo-audio-enemy` + `/sabrimmo-enemy` + `/debugger` (+ `/sabrimmo-debuff` if status sounds)
- "Combat hit sounds / skill impact SFX" -> `/sabrimmo-audio-combat` + `/sabrimmo-audio-player` (+ `/sabrimmo-skills-vfx` if skill impact)
- "Ambient sounds (zone water/wind/birds)" -> `/sabrimmo-audio-player` + `/sabrimmo-zone`
- "Create character hair/costume system" -> `/sabrimmo-art` + `/sabrimmo-ui`
- "Render character sprites from 3D models" -> `/sabrimmo-3d-to-2d`
- "Convert hero ref to GLB and render sprites" -> `/sabrimmo-3d-to-2d` + `/sabrimmo-art`
- "Socket event issues (not arriving, new event, new subsystem)" -> `/sabrimmo-persistent-socket` + `/realtime` + `/debugger`
- "Card issues" -> `/sabrimmo-cards` + `/sabrimmo-combat` + `/debugger` (+ `/sabrimmo-ui` if compound UI)
- "Arrow/ammo issues" -> `/sabrimmo-ammunition` + `/sabrimmo-combat` + `/debugger` (+ `/sabrimmo-skill-archer` if crafting)
- "Chat issues (not showing, channels, combat log)" -> `/sabrimmo-chat` + `/debugger` (+ `/sabrimmo-party`/`/sabrimmo-guild` if channels, + `/sabrimmo-combat-log` if combat tab)
- "Implement 2nd class skill" -> `/sabrimmo-skill-{class}` + `/sabrimmo-skills` + `/sabrimmo-combat` (+ ground effects if AoE/trap/performance)
- "Crafting issues (Pharmacy, Arrow Crafting)" -> `/sabrimmo-crafting` + `/debugger` (+ `/sabrimmo-skill-alchemist` or `/sabrimmo-skill-archer`)
- "Summon issues (Flora, Marine Sphere)" -> `/sabrimmo-companions` + `/sabrimmo-skill-alchemist` + `/debugger`
- "Cart/vending not working" -> `/sabrimmo-economy` + `/sabrimmo-skill-merchant` + `/debugger`
- "Trade issues" -> `/sabrimmo-trading` + `/debugger` (+ `/sabrimmo-items` if transfer, + `/sabrimmo-economy` if vending conflict)
- "Right-click menu issues" -> `/sabrimmo-right-click-player-context` + `/debugger` + target system skill
- "Pet/Homunculus issues" -> `/sabrimmo-companions` + `/debugger` (+ `/sabrimmo-skill-alchemist` if homunculus, + `/sabrimmo-stats` if bonuses)
- "Monster skill issues (not casting, Plagiarism)" -> `/sabrimmo-monster-skills` + `/enemy-ai` + `/debugger` (+ `/sabrimmo-skill-rogue` if Plagiarism)
- "Death/respawn issues" -> `/sabrimmo-death` + `/debugger`
- "MVP issues (rewards, announce, slaves)" -> `/sabrimmo-mvp` + `/enemy-ai` + `/debugger` (+ `/sabrimmo-chat` if announce)
- "NavMesh/pathfinding issues" -> `/sabrimmo-navmesh` + `/enemy-ai` + `/debugger` (+ `/sabrimmo-zone` if new zone, + `/sabrimmo-build-compile` if export)
- "Ground effect not ticking" -> `/sabrimmo-skills` + `/sabrimmo-combat` + class-specific skill + `/debugger`
- "Map issues (minimap, world map, loading screen, Guide NPC marks)" -> `/sabrimmo-map` + `/debugger` (+ `/sabrimmo-zone` if zone grid, + `/sabrimmo-npcs` if Guide NPC, + `/sabrimmo-party` if party dots, + `/sabrimmo-persistent-socket` if zone change)
- "Options issues (not saving, FPS, brightness)" -> `/sabrimmo-options` + `/debugger` (+ `/sabrimmo-3d-world` if brightness, + target subsystem skill)
- "ESC menu issues (not opening, char select, respawn)" -> `/sabrimmo-esc-menu` + `/debugger` (+ `/sabrimmo-persistent-socket` if char switch, + `/sabrimmo-death` if respawn)
- "Login screen issues (background, squished)" -> `/sabrimmo-login-screen` + `/sabrimmo-resolution` + `/debugger`
- "Resolution/standalone issues (widget size, 32x32 texture, squished UI)" -> `/sabrimmo-resolution` + `/sabrimmo-ui` + `/debugger`
- "Ground item issues (not dropping, pickup fail, icon missing, floating)" -> `/sabrimmo-item-drop-system` + `/debugger` (+ `/sabrimmo-party` if loot modes, + `/sabrimmo-buff` if drop rate, + `/sabrimmo-enemy` if death drops, + `/sabrimmo-options` if drop sounds)

**Do NOT skip loading skills to save time.** The cost of reloading context is far less than the cost of implementing something wrong and having to redo it.

---

## Personal Skills Available

74 sabrimmo-* skills (20 class + 54 system) + 17 utility skills. Invoke with `/skill-name`. Located at `C:/Users/pladr/.claude/skills/`.
See `docsNew/00_Global_Rules/Global_Rules.md` → **SKILL INVOCATION** for comprehensive keyword-to-skill mapping, co-load rules, and overlapping skill pairs.

---

## Key Documentation Files

| Document | Path |
|----------|------|
| Project Overview | `docsNew/00_Project_Overview.md` |
| Global Rules | `docsNew/00_Global_Rules/Global_Rules.md` |
| RagnaCloneDocs | `RagnaCloneDocs/` (28 files — 14 design + 14 implementation) |

Additional doc paths are linked in the Skill Selection Guide "Also read" column above.
