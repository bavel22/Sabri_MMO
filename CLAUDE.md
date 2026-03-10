# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Sabri_MMO** — Class-based action MMORPG inspired by Ragnarok Online Classic.
**Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
**Architecture**: Server-authoritative — all combat, stats, inventory, and positions are validated server-side. The client is presentation + input only.

Full rules and design standards: `docsNew/00_Global_Rules/Global_Rules.md`
Architecture reference: `docsNew/00_Project_Overview.md`
RO Classic game design reference: `RagnaCloneDocs/` (28 files — 14 design + 14 implementation guides)

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
Single monolithic file (~2400 lines). Key sections:
- **REST API** (`/api/auth/*`, `/api/characters/*`, `/api/servers`) — JWT-based auth, character CRUD, server list
- **Socket.io events** — `player:join/position/moved/left`, `combat:*`, `inventory:*`, `chat:*`, `enemy:*`, `stats:*`, `skills:*`
- **Combat tick loop** — 50ms interval, ASPD-based attack timing
- **Enemy AI loop** — 509 RO monster templates, 46 active spawn points (zones 1-3 only, zones 4-9 disabled)
- **Data modules** imported at top: `ro_monster_templates`, `ro_item_mapping`, `ro_exp_tables`, `ro_skill_data`, `ro_monster_ai_codes`, `ro_zone_data`
- **JWT validation** on `player:join` socket event (character ownership check)

### Client C++ (`client/SabriMMO/Source/SabriMMO/`)
| File | Role |
|------|------|
| `CharacterData.h` | `FCharacterData` (30+ fields), `FServerInfo`, `FInventoryItem`, drag-drop structs |
| `MMOGameInstance.*` | Auth state, server selection, character list, persistent socket (ConnectSocket/EmitSocketEvent/EventRouter) |
| `MMOHttpManager.*` | BlueprintFunctionLibrary — REST: login, register, servers, characters CRUD, position save |
| `SabriMMOCharacter.*` | Base player pawn — movement, socket events, stats, BeginPlay ground-snap via `SnapLocationToGround` |
| `SabriMMOPlayerController.*` | Input mapping (click-to-move + WASD) |
| `OtherCharacterMovementComponent.*` | Remote player interpolation + per-tick floor-snap (Z correction via line trace) |
| `UI/LoginFlowSubsystem.*` | Login flow state machine (Login->Server->CharSelect->Create->EnterWorld) |
| `UI/SLoginWidget.*` | Login screen (username/password, remember me, error display) |
| `UI/SServerSelectWidget.*` | Server list with population/status, selection highlighting |
| `UI/SCharacterSelectWidget.*` | 3x3 character card grid + detail panel + delete confirmation |
| `UI/SCharacterCreateWidget.*` | Name, gender, hair style/color pickers |
| `UI/SLoadingOverlayWidget.*` | "Please Wait" fullscreen overlay with progress bar |
| `UI/SBasicInfoWidget.*` | Slate HUD panel (HP/SP/EXP bars, draggable) |
| `UI/BasicInfoSubsystem.*` | UWorldSubsystem bridging server data -> Slate widget |
| `UI/ZoneTransitionSubsystem.*` | Zone transitions, loading overlay, pawn teleport, zone:change/error/teleport events, `SnapLocationToGround()` static helper |
| `UI/KafraSubsystem.*` | Kafra NPC dialog, save point, teleport service |
| `UI/SKafraWidget.*` | Slate Kafra service dialog (save/teleport/cancel) |
| `WarpPortal.*` | Overlap trigger actor for zone warps |
| `KafraNPC.*` | Clickable Kafra NPC actor |
| `SabriMMOGameMode.*` | Base GameMode — sets DefaultPawnClass=nullptr (Level Blueprint spawns pawn) |
| `SocketEventRouter.*` | Multi-handler Socket.io event dispatch — allows multiple subsystems per event |
| `UI/MultiplayerEventSubsystem.*` | Bridge: forwards persistent socket events to BP_SocketManager handler functions via ProcessEvent |
| `UI/PositionBroadcastSubsystem.*` | 30Hz position broadcasting via persistent socket |

### Database (PostgreSQL)
4 core tables: `users`, `characters`, `items` (static definitions), `character_inventory` (per-character).
Character columns include: hair_style, hair_color, gender, delete_date, deleted (soft-delete flag), plus stats added dynamically by the server.
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

**Manager pattern** — One manager per domain (`BP_OtherPlayerManager`, `BP_EnemyManager`). Managers hold Maps/Arrays of managed objects with `Register`/`Unregister`/`Get` functions.

**Interfaces over cast chains** — Use `BPI_Damageable`, `BPI_Interactable`, `BPI_Targetable` instead of `Cast To BP_Enemy` / `Cast To BP_Player` chains.

**Event dispatchers over Tick polling** — Widgets bind to `OnHealthChanged`, `OnStatsUpdated`, etc. Do not poll state on Tick.

**Component-based** — New gameplay features get a dedicated `UActorComponent`. Do not add movement + combat + inventory logic to one Actor.

---

## Persistent Socket Architecture (Phase 4)

**Persistent socket on GameInstance** — `UMMOGameInstance` owns a `TSharedPtr<FSocketIONative>` that survives `OpenLevel()`. No disconnect/reconnect on zone transitions. `USocketEventRouter` provides multi-handler dispatch (multiple subsystems can register for the same event). All C++ subsystems use `Router->RegisterHandler()` in `OnWorldBeginPlay` and `Router->UnregisterAllForOwner(this)` in `Deinitialize`. Blueprint emit calls use `GI->K2_EmitSocketEvent()` (BlueprintCallable). `MultiplayerEventSubsystem` bridges 30 inbound events to BP_SocketManager's existing handler functions via `ProcessEvent`. `PositionBroadcastSubsystem` handles 30Hz position updates. BP_SocketManager still exists in levels as a handler shell — its SocketIO component is no longer connected, but its handler functions (OnCombatDamage, OnEnemySpawn, etc.) are still called by the bridge. All subsystem widgets are gated behind `GI->IsSocketConnected()` so they only show in game levels, not the login screen.

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
| Slate UI panels | `/sabrimmo-ui` | — |
| Skill targeting (click-to-cast) | `/sabrimmo-target-skill` | — |
| Clickable NPCs, interactables | `/sabrimmo-click-interact` | — |
| Zones, maps, warp portals | `/sabrimmo-zone` | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` |
| VFX, particles, Niagara | `/sabrimmo-skills-vfx` | `docsNew/05_Development/VFX_Asset_Reference.md` |
| Stats, leveling, class system | `/sabrimmo-stats` | `RagnaCloneDocs/01_Stats_Leveling_JobSystem.md` |
| Damage pipelines, combat formulas | `/sabrimmo-combat` | `RagnaCloneDocs/02_Combat_System.md` |
| Buffs (Provoke, Blessing, etc.) | `/sabrimmo-buff` | `docsNew/03_Server_Side/Status_Effect_Buff_System.md` |
| Status effects (stun, freeze, etc.) | `/sabrimmo-debuff` | `docsNew/03_Server_Side/Status_Effect_Buff_System.md` |
| Skill trees, cast times, cooldowns | `/sabrimmo-skills` | `RagnaCloneDocs/03_Skills_Magic_System.md` |
| Items, equipment, refining, cards | `/sabrimmo-items` | `RagnaCloneDocs/06_Items_Equipment.md` |
| NPCs, shops, quests, Kafra | `/sabrimmo-npcs` | `RagnaCloneDocs/08_NPCs_Quests.md` |
| Party, guild, social systems | `/sabrimmo-party-guild` | `RagnaCloneDocs/07_Social_Systems.md` |
| PvP, War of Emperium, siege | `/sabrimmo-pvp-woe` | `RagnaCloneDocs/05_PvP_GvG_WoE.md` |
| Trading, vending, storage, zeny | `/sabrimmo-economy` | `RagnaCloneDocs/09_Economy_Trading.md` |
| Pets, homunculus, mercenaries | `/sabrimmo-companions` | `RagnaCloneDocs/12_Pets_Homunculus_Companions.md` |
| Audio, BGM, SFX, sound design | `/sabrimmo-audio` | `RagnaCloneDocs/13_Audio_Sound_Design.md` |
| Art, models, animations, hair | `/sabrimmo-art` | `RagnaCloneDocs/14_Art_Visual_Style.md` |
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
- "Add a new zone with NPCs and warps" -> `/sabrimmo-zone` + `/sabrimmo-npcs` + `/sabrimmo-click-interact`
- "Fix a crash when casting spells" -> `/debugger` + `/sabrimmo-skills` + `/realtime`
- "Build a new HUD panel showing buffs" -> `/sabrimmo-buff` + `/sabrimmo-debuff` + `/sabrimmo-ui`
- "Add a skill that applies poison" -> `/sabrimmo-debuff` + `/sabrimmo-skills` + `/full-stack`
- "Add a buff skill like Blessing" -> `/sabrimmo-buff` + `/sabrimmo-skills` + `/full-stack`
- "Add a new monster with special attacks" -> `/enemy-ai` + `/sabrimmo-combat` + `/full-stack`
- "Implement the inventory system" -> `/sabrimmo-items` + `/sabrimmo-economy` + `/full-stack` + `/sabrimmo-ui`
- "Add party EXP sharing" -> `/sabrimmo-party-guild` + `/sabrimmo-stats` + `/full-stack`
- "Implement pet taming system" -> `/sabrimmo-companions` + `/sabrimmo-items` + `/full-stack`
- "Set up WoE castle sieges" -> `/sabrimmo-pvp-woe` + `/sabrimmo-party-guild` + `/sabrimmo-combat`
- "Add NPC shops and quest givers" -> `/sabrimmo-npcs` + `/sabrimmo-items` + `/sabrimmo-economy`
- "Add background music per zone" -> `/sabrimmo-audio` + `/sabrimmo-zone`
- "Create character hair/costume system" -> `/sabrimmo-art` + `/sabrimmo-ui`
- "Add a new socket event for party invites" -> `/sabrimmo-persistent-socket` + `/sabrimmo-party-guild` + `/full-stack`
- "New subsystem listening to socket events" -> `/sabrimmo-persistent-socket` + `/sabrimmo-ui`
- "Debug socket events not arriving" -> `/debugger` + `/sabrimmo-persistent-socket` + `/realtime`

**Do NOT skip loading skills to save time.** The cost of reloading context is far less than the cost of implementing something wrong and having to redo it.

---

## Personal Skills Available

Invoke with `/skill-name`. Located at `C:/Users/pladr/.claude/skills/`.

### Core Project Skills
| Skill | Use when |
|-------|----------|
| `/full-stack` | `index.js` changes, DB schema, REST endpoints, server logic |
| `/realtime` | Socket.io events, combat tick, position sync, multiplayer |
| `/ui-architect` | UE5 Blueprint / Widget work (unrealMCP first) |
| `/agent-architect` | Enemy AI, stat formulas, RO game systems architecture |
| `/enemy-ai` | Monster aggro, chase, attack, AI state machine, per-monster AI codes |
| `/planner` | Feature planning, development phases, 13-phase roadmap |
| `/project-docs` | Load full project documentation context |

### RO Game System Skills (sabrimmo-*)
| Skill | Use when |
|-------|----------|
| `/sabrimmo-stats` | Base/derived stats, EXP tables, leveling, class/job system, stat allocation |
| `/sabrimmo-combat` | Physical/magical damage pipeline, elements, size/race, critical hits, ASPD |
| `/sabrimmo-buff` | Buff system (Provoke, Endure, Blessing, etc.), stat modifiers, stacking rules, BuffBarWidget |
| `/sabrimmo-debuff` | Status effects (stun, freeze, poison, etc.), resistance formulas, CC, periodic drains, cleanse |
| `/sabrimmo-skills` | Skill trees, cast times, cooldowns, SP costs, 86+ skill definitions |
| `/sabrimmo-items` | Inventory, equipment slots, weapon types, refining, card system, weight |
| `/sabrimmo-npcs` | NPC types, dialogue trees, shops, Kafra, quests, job change |
| `/sabrimmo-party-guild` | Party (12 max), guild (Emperium), EXP sharing, guild skills |
| `/sabrimmo-pvp-woe` | PvP maps, War of Emperium, castle siege, battlegrounds |
| `/sabrimmo-economy` | Zeny, trading, vending, buying stores, storage, mail, anti-duplication |
| `/sabrimmo-companions` | Pets (34 types), homunculus, mercenaries, falcon, cart, mounts |
| `/sabrimmo-audio` | BGM per zone, SFX categories, UE5 Sound Classes, MetaSounds |
| `/sabrimmo-art` | Character models, hair system, animations, monster pipeline, LOD |
| `/sabrimmo-zone` | Zones, maps, warp portals, Kafra NPCs, zone configuration |
| `/sabrimmo-ui` | Slate UI creation, RO brown/gold theme, drag/resize, HUD panels |
| `/sabrimmo-target-skill` | Click-to-cast targeting, skill hotbar integration |
| `/sabrimmo-click-interact` | Left-click interactable actors (NPCs, chests, etc.) |
| `/sabrimmo-skills-vfx` | Skill VFX, Niagara effects, casting circles, warp portal VFX |
| `/sabrimmo-persistent-socket` | Persistent socket, EventRouter, BP event bridge, subsystem registration |

### Utility Skills
| Skill | Use when |
|-------|----------|
| `/debugger` | Crashes, logic errors, failed connections |
| `/opus-45-thinking` | Complex multi-system architecture decisions |
| `/code-quality` | C++/JS coding standards, refactoring, naming, completion audits |
| `/security` | JWT auth, SQL injection, anti-cheat, rate limiting, input validation |
| `/performance` | FPS optimization, tick rate tuning, query optimization, bandwidth |
| `/test` | Quick server tests, Socket.io event validation |
| `/tester` | Full QA, integration testing, cross-layer behavioral tests |
| `/docs` | Documentation sync, changelog, docsNew/ updates |
| `/prompt-engineer` | System prompts, skill definitions, Claude-specific patterns |
| `/deep-research` | Multi-source web research with reasoning chains |

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

### RagnaCloneDocs Reference (RO Classic Game Design)

| Design Doc | Implementation Doc | Covers |
|-----------|-------------------|--------|
| `00_Master_Build_Plan.md` | `00_Master_Implementation_Plan.md` | 13-phase roadmap, priorities, dependencies |
| `01_Stats_Leveling_JobSystem.md` | `02_Stats_Class_System.md` | 6 stats, derived formulas, EXP tables, 30+ classes |
| `02_Combat_System.md` | `03_Combat_System.md` | Physical/magical damage, elements, status effects |
| `03_Skills_Magic_System.md` | `04_Skills_System.md` | 86+ skills, cast times, skill trees, SP costs |
| `04_Monsters_EnemyAI.md` | `05_Enemy_Monster_System.md` | 509 monsters, AI codes, spawn zones, boss mechanics |
| `05_PvP_GvG_WoE.md` | `06_PvP_WoE_System.md` | PvP maps, castle siege, Emperium, battlegrounds |
| `06_Items_Equipment.md` | `07_Items_Equipment.md` | Items, equipment, refining, cards, weight |
| `07_Social_Systems.md` | `08_Social_Systems.md` | Party, guild, friends, chat channels |
| `08_NPCs_Quests.md` | `09_NPCs_Quests.md` | NPC types, quest system, shops, Kafra |
| `09_Economy_Trading.md` | `10_Economy_Trading.md` | Zeny, vending, buying stores, storage |
| `10_World_Zones_Navigation.md` | — | Zone design, world map, navigation |
| `11_Multiplayer_Networking.md` | `01_Core_Architecture.md` | Socket.io, position sync, zone broadcasting |
| `12_Pets_Homunculus_Companions.md` | `12_Companions_System.md` | Pets, homunculus, mercenaries, falcon |
| `13_Audio_Sound_Design.md` | `13_Audio_System.md` | BGM, SFX, audio settings |
| `14_Art_Visual_Style.md` | `14_Art_Pipeline.md` | Models, animations, hair, UI art |
