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
Single monolithic file (~2269 lines). Key sections:
- **REST API** (`/api/auth/*`, `/api/characters/*`) — JWT-based auth, character CRUD
- **Socket.io events** — `player:join/position/moved/left`, `combat:*`, `inventory:*`, `chat:*`, `enemy:*`, `stats:*`, `skills:*`
- **Combat tick loop** — 50ms interval, ASPD-based attack timing
- **Enemy AI loop** — 509 RO monster templates, 46 active spawn points (zones 1-3 only, zones 4-9 disabled)
- **Data modules** imported at top: `ro_monster_templates`, `ro_item_mapping`, `ro_exp_tables`, `ro_skill_data`

### Client C++ (`client/SabriMMO/Source/SabriMMO/`)
| File | Role |
|------|------|
| `MMOGameInstance.*` | Persists auth token, character list, selected character across levels |
| `MMOHttpManager.*` | BlueprintFunctionLibrary for REST API calls |
| `SabriMMOCharacter.*` | Base player pawn — movement, socket events, stats |
| `SabriMMOPlayerController.*` | Input mapping (click-to-move + WASD) |
| `OtherCharacterMovementComponent.*` | Remote player interpolation |
| `UI/SBasicInfoWidget.*` | Slate HUD panel (HP/SP/EXP bars, draggable) |
| `UI/BasicInfoSubsystem.*` | UWorldSubsystem bridging server data → Slate widget |

### Database (PostgreSQL)
4 core tables: `users`, `characters`, `items` (static definitions), `character_inventory` (per-character).
Stat columns (`str`, `agi`, `vit`, `int_stat`, `dex`, `luk`, etc.) are added dynamically by the server.

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

## Personal Skills Available

Invoke with `/skill-name`. Located at `C:/Users/pladr/.claude/skills/`.

| Skill | Use when |
|-------|----------|
| `/debugger` | Crashes, logic errors, failed connections |
| `/full-stack` | `index.js` changes, DB schema, REST endpoints |
| `/realtime` | Socket.io events, combat tick, position sync |
| `/ui-architect` | UE5 Blueprint / Widget work (unrealMCP first) |
| `/agent-architect` | Enemy AI, stat formulas, RO game systems |
| `/planner` | Feature planning, development phases |
| `/sabrimmo-ui` | Project-specific UI guidance |
| `/project-docs` | Load full project documentation context |
| `/opus-45-thinking` | Complex multi-system architecture decisions |
