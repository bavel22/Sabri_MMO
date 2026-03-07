# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**Sabri_MMO** — Class-based action MMORPG inspired by Ragnarok Online.
**Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
**Architecture**: Server-authoritative — all combat, stats, inventory, and positions are validated server-side. The client is presentation + input only.

Full rules and design standards: `docsNew/00_Global_Rules/Global_Rules.md`
Architecture reference: `docsNew/00_Project_Overview.md`

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
- **Data modules** imported at top: `ro_monster_templates`, `ro_item_mapping`, `ro_exp_tables`, `ro_skill_data`
- **JWT validation** on `player:join` socket event (character ownership check)

### Client C++ (`client/SabriMMO/Source/SabriMMO/`)
| File | Role |
|------|------|
| `CharacterData.h` | `FCharacterData` (30+ fields), `FServerInfo`, `FInventoryItem`, drag-drop structs |
| `MMOGameInstance.*` | Auth state, server selection, character list, remembered username, configurable URL |
| `MMOHttpManager.*` | BlueprintFunctionLibrary — REST: login, register, servers, characters CRUD, position save |
| `SabriMMOCharacter.*` | Base player pawn — movement, socket events, stats |
| `SabriMMOPlayerController.*` | Input mapping (click-to-move + WASD) |
| `OtherCharacterMovementComponent.*` | Remote player interpolation |
| `UI/LoginFlowSubsystem.*` | Login flow state machine (Login→Server→CharSelect→Create→EnterWorld) |
| `UI/SLoginWidget.*` | Login screen (username/password, remember me, error display) |
| `UI/SServerSelectWidget.*` | Server list with population/status, selection highlighting |
| `UI/SCharacterSelectWidget.*` | 3x3 character card grid + detail panel + delete confirmation |
| `UI/SCharacterCreateWidget.*` | Name, gender, hair style/color pickers |
| `UI/SLoadingOverlayWidget.*` | "Please Wait" fullscreen overlay with progress bar |
| `UI/SBasicInfoWidget.*` | Slate HUD panel (HP/SP/EXP bars, draggable) |
| `UI/BasicInfoSubsystem.*` | UWorldSubsystem bridging server data → Slate widget |
| `UI/ZoneTransitionSubsystem.*` | Zone transitions, loading overlay, pawn teleport, zone:change/error/teleport events |
| `UI/KafraSubsystem.*` | Kafra NPC dialog, save point, teleport service |
| `UI/SKafraWidget.*` | Slate Kafra service dialog (save/teleport/cancel) |
| `WarpPortal.*` | Overlap trigger actor for zone warps |
| `KafraNPC.*` | Clickable Kafra NPC actor |
| `SabriMMOGameMode.*` | Base GameMode — sets DefaultPawnClass=nullptr (Level Blueprint spawns pawn) |

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

1. **Identify which systems the task touches** (UI? Server? VFX? Zones? Combat? etc.)
2. **Load the matching skill(s)** using the Skill tool — skills contain architectural rules, file locations, naming conventions, and common patterns that MUST be followed
3. **Read relevant source files** before modifying them — never assume code structure
4. **Check `docsNew/`** for system-specific documentation when the skill references it

### Skill Selection Guide

| If the task involves... | Load these skills | Also read these docs |
|------------------------|-------------------|---------------------|
| Crashes, errors, bugs | `/debugger` | — |
| Server code, DB, REST API | `/full-stack` | `docsNew/00_Project_Overview.md` |
| Socket.io events, multiplayer sync | `/realtime` | — |
| Blueprint / Widget work | `/ui-architect` | unrealMCP first |
| Enemy AI, monster behavior | `/enemy-ai` | `server/src/ro_monster_ai_codes.js` |
| Slate UI panels | `/sabrimmo-ui` | — |
| Skill targeting (click-to-cast) | `/sabrimmo-target-skill` | — |
| Clickable NPCs, interactables | `/sabrimmo-click-interact` | — |
| Zones, maps, warp portals | `/sabrimmo-zone` | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` |
| VFX, particles, Niagara | `/sabrimmo-vfx` | `docsNew/05_Development/VFX_Asset_Reference.md` |
| New feature planning | `/planner` | `docsNew/00_Project_Overview.md` |
| Complex architecture decisions | `/opus-45-thinking` | `/project-docs` |
| Full project context dump | `/project-docs` | All of `docsNew/` |

### Multi-System Tasks

Many tasks touch multiple systems. **Load ALL relevant skills.** Examples:
- "Add a new skill with VFX" → `/sabrimmo-target-skill` + `/sabrimmo-vfx` + `/full-stack`
- "Add a new zone with NPCs and warps" → `/sabrimmo-zone` + `/sabrimmo-click-interact` + `/sabrimmo-vfx`
- "Fix a crash when casting spells" → `/debugger` + `/sabrimmo-vfx` + `/realtime`
- "Build a new HUD panel showing buffs" → `/sabrimmo-ui` + `/realtime`
- "Add a new monster with special attacks" → `/enemy-ai` + `/agent-architect` + `/full-stack`

**Do NOT skip loading skills to save time.** The cost of reloading context is far less than the cost of implementing something wrong and having to redo it.

---

## Personal Skills Available

Invoke with `/skill-name`. Located at `C:/Users/pladr/.claude/skills/`.

| Skill | Use when |
|-------|----------|
| `/debugger` | Crashes, logic errors, failed connections |
| `/full-stack` | `index.js` changes, DB schema, REST endpoints |
| `/realtime` | Socket.io events, combat tick, position sync |
| `/ui-architect` | UE5 Blueprint / Widget work (unrealMCP first) |
| `/agent-architect` | Enemy AI, stat formulas, RO game systems |
| `/enemy-ai` | Monster aggro, chase, attack, AI state machine, mode flags, per-monster AI codes |
| `/planner` | Feature planning, development phases |
| `/sabrimmo-ui` | Project-specific UI guidance |
| `/sabrimmo-target-skill` | Set up RO-style click-to-cast targeting for a skill |
| `/sabrimmo-click-interact` | Add new left-click interactable actors to the world (NPCs, chests, etc.) |
| `/sabrimmo-zone` | Add new zones/levels/maps, warp portals, Kafra NPCs, zone configuration |
| `/sabrimmo-vfx` | Skill VFX system — Niagara effects, casting circles, warp portal VFX, adding VFX to new skills |
| `/project-docs` | Load full project documentation context |
| `/opus-45-thinking` | Complex multi-system architecture decisions |

### Key Documentation Files

| Document | Path | Contains |
|----------|------|----------|
| Project Overview | `docsNew/00_Project_Overview.md` | Full system inventory, tech stack, all implemented features |
| Global Rules | `docsNew/00_Global_Rules/Global_Rules.md` | Design standards, coding rules |
| Zone Setup Guide | `docsNew/05_Development/Zone_System_UE5_Setup_Guide.md` | Level Blueprint, zone registry, warp portal placement |
| VFX Asset Reference | `docsNew/05_Development/VFX_Asset_Reference.md` | 1,574 VFX assets cataloged, mapped to RO skills |
| VFX Implementation Plan | `docsNew/05_Development/Skill_VFX_Implementation_Plan.md` | Niagara architecture, per-skill specs, RO visual reference |
| VFX Execution Plan | `docsNew/05_Development/Skill_VFX_Execution_Plan.md` | Step-by-step build status, completion checklist |
| VFX Research Findings | `docsNew/05_Development/Skill_VFX_Research_Findings.md` | 138-source research on RO effects, UE5 Niagara, AI tools |
