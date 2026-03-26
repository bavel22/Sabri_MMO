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
Single monolithic file (~30000 lines). Key sections:
- **REST API** (`/api/auth/*`, `/api/characters/*`, `/api/servers`) — JWT-based auth, character CRUD, server list
- **Socket.io events** — `player:join/position/moved/left`, `combat:*`, `inventory:*`, `chat:*`, `enemy:*`, `stats:*`, `skills:*`
- **Combat tick loop** — 50ms interval, ASPD-based attack timing, dual wield dual-hit per cycle
- **Enemy AI loop** — 509 RO monster templates, 46 active spawn points, monster skill casting
- **Class systems** — Performance (Bard/Dancer), Combo (Monk), Spirit Spheres, Traps (Hunter), Falcon, Cart/Vending, Homunculus, Forging/Refining
- **Ensemble system** — Bard/Dancer 9 ensemble skills, SP drain tick, 9 effect types (stat/resist/regen/speed zones), partner validation, aftermath debuff on end
- **Consumable effects** — `sc_start` handler (ASPD potions, stat foods, cure items), `itemskill` scrolls (Fly Wing, Butterfly Wing, identification), elemental converters (fire/water/wind/earth endow)
- **Abracadabra** — 145 regular + 6 special random skill cast system, weighted selection, Sage skill 1420
- **Monster summoning** — `NPC_SUMMONSLAVE` slave spawning with master/slave lifecycle (slaves die when master dies, slaves leash to master), `NPC_METAMORPHOSIS` transformation (monster replaces itself with a different template)
- **Data modules**: `ro_monster_templates`, `ro_exp_tables`, `ro_skill_data` (includes `ro_skill_data_2nd` internally), `ro_monster_ai_codes`, `ro_zone_data`, `ro_card_effects`, `ro_item_groups`, `ro_ground_effects`, `ro_arrow_crafting`, `ro_monster_skills`, `ro_homunculus_data`, `ro_status_effects`, `ro_buff_system`, `ro_item_effects`, `ro_damage_formulas`
- **Card effect hooks** — `processCardKillHooks`, `processCardDrainEffects`, `processCardStatusProcsOnAttack/WhenHit`, `processCardAutoSpellOnAttack/WhenHit`, `processAutobonusOnAttack/WhenHit`, `processCardDropBonuses`, `knockbackTarget`, `executeAutoSpellEffect`
- **JWT validation** on `player:join` socket event (character ownership check)

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

**30+ UWorldSubsystem files in `UI/`** — each manages one domain (inventory, equipment, buffs, skills, chat, party, etc.) with a paired Slate widget. Key subsystems: `EnemySubsystem` (enemy registry, 5 events), `OtherPlayerSubsystem` (player registry), `CombatActionSubsystem` (10 combat events), `PlayerInputSubsystem` (click-to-move/attack), `LoginFlowSubsystem` (auth flow state machine), `PositionBroadcastSubsystem` (30Hz updates).

**3 VFX files in `VFX/`** — `SkillVFXSubsystem` (97+ configs), `SkillVFXData` (config structs), `CastingCircleActor`.

Load the relevant `/sabrimmo-*` skill for detailed file documentation per subsystem.

### Database (PostgreSQL)
Core tables: `users`, `characters`, `items` (static definitions), `character_inventory` (per-character), `character_hotbar`, `character_cart`, `character_homunculus`, `parties`, `party_members`, `vending_shops`, `vending_items`. 25 migration files in `database/migrations/`.
Character columns include: hair_style, hair_color, gender, delete_date, deleted (soft-delete flag), plagiarized_skill_id/level, plus stats added dynamically by the server.
Characters are never hard-deleted — `DELETE /api/characters/:id` sets `deleted = TRUE`. All queries filter with `AND deleted = FALSE`.

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

**Manager pattern** — One manager per domain. `UEnemySubsystem` and `UOtherPlayerSubsystem` (C++) replaced `BP_EnemyManager` and `BP_OtherPlayerManager` (BP) in Phase 3. Managers hold TMap registries with `GetEnemy()`/`GetPlayer()` lookups.

**Interfaces over cast chains** — Use `BPI_Damageable`, `BPI_Interactable`, `BPI_Targetable` instead of `Cast To BP_Enemy` / `Cast To BP_Player` chains.

**Event dispatchers over Tick polling** — Widgets bind to `OnHealthChanged`, `OnStatsUpdated`, etc. Do not poll state on Tick.

**C++ struct registries over BP property reflection** — Entity subsystems store data in C++ structs (`FEnemyEntry`, `FPlayerEntry`) alongside actor pointers. All reads come from the struct (O(1) TMap lookup). BP actors still receive writes via `SetBPVector`/`SetBPBool` for Tick interpolation. Never use `FindPropertyByName`/`GetPropertyValue_InContainer` to read entity data — use the subsystem's public API (`GetEnemyData()`, `GetPlayerData()`, `GetEnemyIdFromActor()`, `GetPlayerIdFromActor()`).

**Component-based** — New gameplay features get a dedicated `UActorComponent`. Do not add movement + combat + inventory logic to one Actor.

**UPROPERTY on loaded classes** — Any `UClass*` obtained via `LoadClass<>()` or `LoadObject<>()` and stored as a member variable MUST be marked `UPROPERTY()`. Without it, UE5's garbage collector cannot see the reference, will collect the class, and the subsystem crashes with an access violation when it tries to use the stale pointer. This applies to `EnemyBPClass`, `PlayerBPClass`, and any future runtime-loaded class references.

**Zone filtering on all AoE loops** — Every server-side AoE skill handler that iterates `enemies.entries()` or `connectedPlayers.entries()` MUST filter by `enemy.zone !== zone` / `ptarget.zone !== zone`. The `enemies` Map is global (all zones). Without zone filtering, enemies from other zones at overlapping coordinates get included, diluting split damage (Napalm Beat) or dealing invisible damage to unreachable targets.

**Auto-create DB columns at server startup** — Any new column added to a migration file MUST also be added to the server startup auto-creation block (near `// Ensure stat columns exist`). The `getPlayerInventory()` query and similar functions reference these columns — if the migration hasn't been run, the query fails and returns empty data. Auto-creation with `ADD COLUMN IF NOT EXISTS` makes the server self-healing.

**Magic Rod absorption in single-target magic paths** — Every single-target magical skill damage path checks for `magic_rod` buff on the target. If active, the spell is absorbed (target recovers SP equal to the skill's SP cost), damage is nullified, and the buff is consumed. This check must appear before damage application in any new single-target magic handler.

**Ensemble ground effects as stationary AoE zones** — Ensemble skills (Bard+Dancer duet) create stationary ground effects at the midpoint of the two performers. Unlike solo performances which follow the caster, ensembles are fixed-position AoE zones that tick effects on all entities within range. Both performers must remain within range or the ensemble ends.

**Remote visual sync via zone:ready, NOT player:join** — Any multiplayer visual data (weapon sprites, equipment appearance, buff visuals, costumes, mount state) that needs to be received by a client's subsystem handlers MUST be sent in the `zone:ready` handler, NOT during `player:join`. During `player:join`, the client is doing a zone transition (OpenLevel) — subsystems are destroyed and recreated, so socket events are silently dropped. By `zone:ready`, all handlers are registered. The `player:join` early broadcast block also has a JavaScript temporal dead zone issue: variables declared with `let` later in the function (e.g., `jobClass`) cannot be referenced in the early block — use `var` or query the DB row directly.

---

## Persistent Socket Architecture (Phase 4)

**Persistent socket on GameInstance** — `UMMOGameInstance` owns a `TSharedPtr<FSocketIONative>` that survives `OpenLevel()`. No disconnect/reconnect on zone transitions. `USocketEventRouter` provides multi-handler dispatch (multiple subsystems can register for the same event). All C++ subsystems use `Router->RegisterHandler()` in `OnWorldBeginPlay` and `Router->UnregisterAllForOwner(this)` in `Deinitialize`. Blueprint emit calls use `GI->K2_EmitSocketEvent()` (BlueprintCallable). `MultiplayerEventSubsystem` has 0 bridges — all 14 removed in Phases A-E. Only outbound emit helpers remain (EmitCombatAttack, EmitStopAttack, RequestRespawn, EmitChatMessage). `ChatSubsystem` handles `chat:receive` (Phase E). `CombatActionSubsystem` registers for 10 combat events. `EnemySubsystem` registers for 5 enemy events (spawn/move/death/health_update/attack) and owns the enemy actor registry. `OtherPlayerSubsystem` registers for 2 player events (moved/left) and owns the other-player actor registry. `PositionBroadcastSubsystem` handles 30Hz position updates. BP_SocketManager still exists in levels as a handler shell — its SocketIO component is no longer connected. BP_EnemyManager and BP_OtherPlayerManager were removed from levels in Phase 6 (replaced by C++ subsystems in Phase 3). All subsystem widgets are gated behind `GI->IsSocketConnected()` so they only show in game levels, not the login screen.

---

## Dual Wield System (Assassin/Assassin Cross)

Only Assassin/Assassin Cross can dual wield (daggers, 1H swords, 1H axes in left hand). Katars are mutually exclusive. Server helpers: `canDualWield()`, `isDualWielding()`, `isValidLeftHandWeapon()`, `isKatar()`. Both hands hit per attack cycle with per-hand card mods and mastery penalties. Skills use right-hand only. Full details: load `/sabrimmo-skill-assassin` + `/sabrimmo-combat`.

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
| Server code, DB, REST API | `/full-stack` | `docsNew/00_Project_Overview.md` |
| Socket.io events, multiplayer sync | `/realtime` | — |
| Persistent socket, EventRouter, BP bridge | `/sabrimmo-persistent-socket` | `memory/persistent-socket.md` |
| Blueprint / Widget work | `/ui-architect` | unrealMCP first |
| Enemy AI, monster behavior | `/enemy-ai` | `server/src/ro_monster_ai_codes.js` |
| Monster skill casting, NPC_ skills | `/sabrimmo-monster-skills` | `docsNew/03_Server_Side/Monster_Skill_System.md` |
| Slate UI panels | `/sabrimmo-ui` | — |
| Skill targeting (click-to-cast) | `/sabrimmo-target-skill` | — |
| Chat messages, chat UI, channels | `/sabrimmo-chat` | — |
| Combat log, damage/buff/death messages | `/sabrimmo-combat-log` | — |
| Clickable NPCs, interactables | `/sabrimmo-click-interact` | — |
| Zones, maps, warp portals | `/sabrimmo-zone` | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` |
| VFX, particles, Niagara | `/sabrimmo-skills-vfx` | `docsNew/05_Development/VFX_Asset_Reference.md` |
| Skill icon art generation | `/sabrimmo-generate-icons` | — |
| Stats, leveling, class system | `/sabrimmo-stats` | `RagnaCloneDocs/01_Stats_Leveling_JobSystem.md` |
| Death penalty (PvE EXP loss) | `/sabrimmo-death` | `docsNew/05_Development/Phase_7_9_12_Completion_Plan.md` |
| MVP system, monster skills, slaves | `/sabrimmo-mvp` | `docsNew/03_Server_Side/Monster_Skill_System.md` |
| Damage pipelines, combat formulas | `/sabrimmo-combat` | `RagnaCloneDocs/02_Combat_System.md` |
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
| Items, equipment, refining | `/sabrimmo-items` | `RagnaCloneDocs/06_Items_Equipment.md` |
| Refine ATK bonus, overupgrade, armor DEF | `/sabrimmo-refine` | `docsNew/05_Development/Refine_ATK_And_Party_System_Plan.md` |
| Arrows, ammo equip, element override, consumption | `/sabrimmo-ammunition` | — |
| Weight thresholds, overweight penalties | `/sabrimmo-weight` | `docsNew/03_Server_Side/Inventory_System.md` |
| Card compounding, card bonuses | `/sabrimmo-cards` | — |
| Pharmacy crafting, Arrow Crafting, recipes | `/sabrimmo-crafting` | `docsNew/05_Development/Alchemist_Deferred_Skills_Implementation_Plan.md` |
| Summon Flora, Marine Sphere, summon entities | `/sabrimmo-companions` | `docsNew/05_Development/Alchemist_Deferred_Skills_Implementation_Plan.md` |
| NPCs, shops, quests, Kafra | `/sabrimmo-npcs` | `RagnaCloneDocs/08_NPCs_Quests.md` |
| Party system, EXP share, party chat | `/sabrimmo-party` | `docsNew/05_Development/Refine_ATK_And_Party_System_Plan.md` |
| Guild system, ranks, skills, storage | `/sabrimmo-guild` | `RagnaCloneDocs/07_Social_Systems.md` |
| PvP, War of Emperium, siege | `/sabrimmo-pvp-woe` | `RagnaCloneDocs/05_PvP_GvG_WoE.md` |
| Trading, vending, storage, zeny | `/sabrimmo-economy` | `RagnaCloneDocs/09_Economy_Trading.md` |
| Pets, homunculus, mercenaries | `/sabrimmo-companions` | `RagnaCloneDocs/12_Pets_Homunculus_Companions.md` |
| Audio, BGM, SFX, sound design | `/sabrimmo-audio` | `RagnaCloneDocs/13_Audio_Sound_Design.md` |
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
- "Fix Play Dead or Basic Skill job change gate" -> `/sabrimmo-skill-novice` + `/sabrimmo-skills` + `/sabrimmo-buff`
- "Fix Bash stun or Magnum Break fire endow" -> `/sabrimmo-skill-swordsman` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Fix Mage Thunderstorm or Stone Curse petrification" -> `/sabrimmo-skill-mage` + `/sabrimmo-skills` + `/sabrimmo-debuff`
- "Fix Archer Double Strafe or Arrow Shower" -> `/sabrimmo-skill-archer` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Fix Thief Envenom or Hiding SP drain" -> `/sabrimmo-skill-thief` + `/sabrimmo-skills` + `/sabrimmo-debuff`
- "Fix Merchant Mammonite or Cart Revolution" -> `/sabrimmo-skill-merchant` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Add a new zone with NPCs and warps" -> `/sabrimmo-zone` + `/sabrimmo-npcs` + `/sabrimmo-click-interact`
- "Fix a crash when casting spells" -> `/debugger` + `/sabrimmo-skills` + `/realtime`
- "Build a new HUD panel showing buffs" -> `/sabrimmo-buff` + `/sabrimmo-debuff` + `/sabrimmo-ui`
- "Add a skill that applies poison" -> `/sabrimmo-debuff` + `/sabrimmo-skills` + `/full-stack`
- "Add a buff skill like Blessing" -> `/sabrimmo-buff` + `/sabrimmo-skills` + `/full-stack`
- "Add a new monster with special attacks" -> `/enemy-ai` + `/sabrimmo-monster-skills` + `/sabrimmo-combat` + `/full-stack`
- "Implement the inventory system" -> `/sabrimmo-items` + `/sabrimmo-economy` + `/full-stack` + `/sabrimmo-ui`
- "Regen not working / can't attack" -> `/debugger` + `/sabrimmo-weight` + `/sabrimmo-buff`
- "Add party EXP sharing" -> `/sabrimmo-party` + `/sabrimmo-stats` + `/full-stack`
- "Party EXP not working" -> `/sabrimmo-party` + `/debugger`
- "Party chat not showing" -> `/sabrimmo-party` + `/sabrimmo-chat` + `/debugger`
- "Devotion party check failing" -> `/sabrimmo-party` + `/sabrimmo-skill-crusader` + `/debugger`
- "Refine ATK not applying" -> `/sabrimmo-refine` + `/sabrimmo-combat` + `/debugger`
- "Overupgrade bonus wrong" -> `/sabrimmo-refine` + `/sabrimmo-combat`
- "Shield Boomerang refine damage" -> `/sabrimmo-refine` + `/sabrimmo-skill-crusader`
- "Implement pet taming system" -> `/sabrimmo-companions` + `/sabrimmo-items` + `/full-stack`
- "Set up WoE castle sieges" -> `/sabrimmo-pvp-woe` + `/sabrimmo-guild` + `/sabrimmo-combat`
- "Add NPC shops and quest givers" -> `/sabrimmo-npcs` + `/sabrimmo-items` + `/sabrimmo-economy`
- "Add background music per zone" -> `/sabrimmo-audio` + `/sabrimmo-zone`
- "Create character hair/costume system" -> `/sabrimmo-art` + `/sabrimmo-ui`
- "Render character sprites from 3D models" -> `/sabrimmo-3d-to-2d`
- "Convert hero ref to GLB and render sprites" -> `/sabrimmo-3d-to-2d` + `/sabrimmo-art`
- "Add a new socket event for party invites" -> `/sabrimmo-persistent-socket` + `/sabrimmo-party` + `/full-stack`
- "New subsystem listening to socket events" -> `/sabrimmo-persistent-socket` + `/sabrimmo-ui`
- "Debug socket events not arriving" -> `/debugger` + `/sabrimmo-persistent-socket` + `/realtime`
- "Card bonuses not applying in combat" -> `/sabrimmo-cards` + `/sabrimmo-combat` + `/debugger`
- "Arrow element not working" -> `/sabrimmo-ammunition` + `/sabrimmo-combat` + `/debugger`
- "Arrow consumption or crafting issues" -> `/sabrimmo-ammunition` + `/sabrimmo-skill-archer` + `/sabrimmo-items`
- "Add card compound UI" -> `/sabrimmo-cards` + `/sabrimmo-items` + `/sabrimmo-ui`
- "Chat messages not showing" -> `/sabrimmo-chat` + `/debugger` + `/sabrimmo-persistent-socket`
- "Add party/guild chat channels" -> `/sabrimmo-chat` + `/sabrimmo-party` + `/sabrimmo-guild` + `/full-stack`
- "Combat log missing skill damage" -> `/sabrimmo-combat-log` + `/sabrimmo-combat` + `/debugger`
- "Implement Bowling Bash or Brandish Spear" -> `/sabrimmo-skill-knight` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Implement Storm Gust or Meteor Storm" -> `/sabrimmo-skill-wizard` + `/sabrimmo-skills` + `/sabrimmo-combat` + (ground effects: `ro_ground_effects.js`)
- "Implement Sanctuary or Magnus Exorcismus" -> `/sabrimmo-skill-priest` + `/sabrimmo-skills` + `/sabrimmo-combat` + (ground effects)
- "Implement Hunter traps" -> `/sabrimmo-skill-hunter` + `/sabrimmo-skills` + (ground effects: TRAP category)
- "Implement Bard songs or Dancer dances" -> `/sabrimmo-skill-bard` or `/sabrimmo-skill-dancer` + `/sabrimmo-skills` + (ground effects: BUFF_ZONE)
- "Implement Asura Strike or combo system" -> `/sabrimmo-skill-monk` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Implement Sonic Blow or Venom Splasher" -> `/sabrimmo-skill-assassin` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Implement Back Stab or Divest skills" -> `/sabrimmo-skill-rogue` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Implement Adrenaline Rush or forging" -> `/sabrimmo-skill-blacksmith` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Implement Acid Terror or Potion Pitcher" -> `/sabrimmo-skill-alchemist` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Pharmacy crafting not working" -> `/sabrimmo-crafting` + `/sabrimmo-skill-alchemist` + `/debugger`
- "Add new Pharmacy recipe" -> `/sabrimmo-crafting` + `/sabrimmo-items`
- "Arrow Crafting UI not showing" -> `/sabrimmo-crafting` + `/sabrimmo-skill-archer` + `/debugger`
- "Summon Flora plants not attacking" -> `/sabrimmo-companions` + `/sabrimmo-skill-alchemist` + `/debugger`
- "Marine Sphere not detonating" -> `/sabrimmo-companions` + `/sabrimmo-skill-alchemist` + `/debugger`
- "Add client-side summon actors" -> `/sabrimmo-companions` + `/sabrimmo-ui` + `/sabrimmo-persistent-socket`
- "Implement Sage Volcano/Deluge or Land Protector" -> `/sabrimmo-skill-sage` + `/sabrimmo-skills` + (ground effects)
- "Implement Crusader Grand Cross or Devotion" -> `/sabrimmo-skill-crusader` + `/sabrimmo-skills` + `/sabrimmo-combat`
- "Mount system not working" -> `/sabrimmo-skill-knight` or `/sabrimmo-skill-crusader` + `/debugger`
- "Implement Dancer dances" -> `/sabrimmo-skill-dancer` + `/sabrimmo-skills` + (ground effects: BUFF_ZONE)
- "Cart/vending not working" -> `/sabrimmo-economy` + `/sabrimmo-skill-merchant` + `/debugger`
- "Homunculus not attacking" -> `/sabrimmo-companions` + `/sabrimmo-skill-alchemist` + `/debugger`
- "Pet not following" -> `/sabrimmo-companions` + `/debugger`
- "Pet bonuses not applying" -> `/sabrimmo-companions` + `/sabrimmo-stats` + `/debugger`
- "Pet hunger/intimacy issues" -> `/sabrimmo-companions` + `/debugger`
- "Monster skills not casting" -> `/sabrimmo-monster-skills` + `/enemy-ai` + `/debugger`
- "Plagiarism not copying skills" -> `/sabrimmo-skill-rogue` + `/sabrimmo-monster-skills` + `/debugger`
- "Death penalty not working" -> `/sabrimmo-death` + `/debugger`
- "No EXP loss on death" -> `/sabrimmo-death` + `/debugger`
- "MVP rewards not given" -> `/sabrimmo-mvp` + `/debugger`
- "MVP not announcing" -> `/sabrimmo-mvp` + `/sabrimmo-chat`
- "Slave monsters not spawning" -> `/sabrimmo-mvp` + `/enemy-ai` + `/debugger`
- "Whisper not working" -> `/sabrimmo-chat` + `/debugger`
- "Block list not working" -> `/sabrimmo-chat` + `/debugger`

**Do NOT skip loading skills to save time.** The cost of reloading context is far less than the cost of implementing something wrong and having to redo it.

---

## Personal Skills Available

51 sabrimmo-* skills + 10 utility skills. Invoke with `/skill-name`. Located at `C:/Users/pladr/.claude/skills/`.
See `docsNew/00_Global_Rules/Global_Rules.md` → **SKILL INVOCATION** for comprehensive keyword-to-skill mapping, co-load rules, and overlapping skill pairs.

---

## Key Documentation Files

| Document | Path | Contains |
|----------|------|----------|
| Project Overview | `docsNew/00_Project_Overview.md` | Full system inventory, tech stack, all implemented features |
| Global Rules | `docsNew/00_Global_Rules/Global_Rules.md` | Design standards, coding rules |
| Zone Setup Guide | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` | Level Blueprint, zone registry, warp portal placement |
| VFX Asset Reference | `docsNew/05_Development/VFX_Asset_Reference.md` | 1,574 VFX assets cataloged, mapped to RO skills |
| VFX Implementation Plan | `docsNew/05_Development/Skill_VFX_Implementation_Plan.md` | Niagara architecture, per-skill specs, RO visual reference |
| VFX Execution Plan | `docsNew/05_Development/Skill_VFX_Execution_Plan.md` | Step-by-step build status, completion checklist |
| VFX Research Findings | `docsNew/05_Development/Skill_VFX_Research_Findings.md` | 138-source research on RO effects, UE5 Niagara, AI tools |

### RagnaCloneDocs Reference
28 RO Classic reference docs in `RagnaCloneDocs/` (14 design + 14 implementation). Key references are linked in the Skill Selection Guide "Also read" column above.
