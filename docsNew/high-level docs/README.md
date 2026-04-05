# Sabri MMO

A class-based action MMORPG inspired by Ragnarok Online Classic, built with Unreal Engine 5.7, Node.js, and PostgreSQL.

## Project Overview

| | |
|---|---|
| **Game Type** | Class-based action MMORPG (Ragnarok Online Classic-style) |
| **Engine** | Unreal Engine 5.7 (C++ + Blueprint) |
| **Backend** | Node.js 18+ / Express 4.18 / Socket.io 4.8 |
| **Database** | PostgreSQL 15.4 + Redis 7.2+ |
| **Architecture** | Server-authoritative — all combat, stats, inventory, positions validated server-side |
| **Team** | Solo developer |
| **Platform** | Windows 11 (development) |

> For the full system inventory and technology breakdown, see [00_Project_Overview.md](../00_Project_Overview.md).

---

## Quick Start

### Prerequisites

- **Node.js 18+** LTS
- **PostgreSQL 15+**
- **Redis 7+**
- **Unreal Engine 5.7** (Epic Games Launcher)
- **Visual Studio 2022** with C++ tools
- **Git**

### 1. Start PostgreSQL & Redis

```bash
# PostgreSQL
net start postgresql-x64-15          # Windows service
psql -U postgres -d sabri_mmo        # Verify

# Redis
redis-server                         # Start Redis
redis-cli ping                       # Should return PONG
```

### 2. Initialize Database

```sql
-- In pgAdmin or psql against sabri_mmo:
\i database/init.sql
\i scripts/output/canonical_items.sql     -- 6,169 rAthena items

-- Run migrations (in order):
\i database/migrations/add_exp_leveling_system.sql
\i database/migrations/add_class_skill_system.sql
-- ... (see database/migrations/ for all 25 migration files)
```

The server auto-creates missing stat columns on startup, so migrations are self-healing.

### 3. Start the Server

```bash
cd server
cp .env.example .env                 # Configure DB_*, JWT_SECRET, REDIS_URL
npm install
npm run dev                          # Development (nodemon auto-restart)
```

### 4. Open UE5 Client

Open `client/SabriMMO/SabriMMO.uproject` in Unreal Engine 5.7 and Play in Editor.

> For detailed setup, see [Setup_Guide.md](../05_Development/Setup_Guide.md).

---

## What's Implemented

### Classes (19 playable)
- **Novice** + 6 first classes (Swordsman, Mage, Archer, Acolyte, Thief, Merchant)
- **13 second classes**: Knight, Crusader, Wizard, Sage, Hunter, Bard, Dancer, Priest, Monk, Assassin, Rogue, Blacksmith, Alchemist

### Combat
- Server-authoritative auto-attack loop (50ms tick), ASPD-based timing (0-190 scale)
- 293 skill definitions (69 first-class + 224 second-class), 180+ active skill handlers
- 10x10x4 element table, 18-weapon size penalties, card modifier stacking
- Physical + magical damage pipelines with HIT/FLEE, Critical, Perfect Dodge
- Dual wield system (Assassin), combo system (Monk), spirit spheres
- Trap system (Hunter), Falcon auto-blitz, performance system (Bard/Dancer)

### Monsters & AI
- 509 RO monster templates, 46 active spawns across 4 zones
- Full AI state machine with CC lock, hidden player detection
- Monster skill system (40+ NPC_ skills), summoning, metamorphosis
- MVP system with slave spawning and HP-threshold skill casting

### Buffs & Status Effects
- 95 buff types in `ro_buff_system.js` (Blessing, Increase AGI, songs, dances, food, potions, etc.)
- 10 status effects (Stun, Freeze, Stone, Sleep, Poison, Blind, Silence, Confusion, Bleeding, Curse)
- Ensemble system (9 Bard/Dancer duet skills)

### Items & Economy
- 6,169 rAthena canonical items, 538 monster cards with full bonus system
- Equipment with refine ATK/DEF, card compounding, weight thresholds
- Cart, Vending (setup/browse), Item Appraisal, Forging, Refining
- NPC shops, Kafra storage/teleport, arrow crafting

### Multiplayer
- Socket.io real-time with persistent connection (survives zone transitions)
- 79 socket event handlers, 11 REST API endpoints
- Party system (EXP share, party chat, HP sync)
- Chat system (Global, Combat, Party, Whisper channels)

### Client (UE5 C++)
- **33 UWorldSubsystems** managing all game domains
- **Pure C++ Slate UI** — all Blueprint widgets replaced (login, HUD, inventory, skills, etc.)
- 3 VFX files (SkillVFXSubsystem with 97+ configs, Niagara-based)
- Persistent socket on GameInstance, SocketEventRouter for multi-handler dispatch

### Zones
- 4 zones: Prontera (town), Prontera South/North (fields), Prontera Dungeon 01
- Warp portals, Kafra NPCs, zone-scoped broadcasting, position persistence

> For the complete feature list see [00_Project_Overview.md](../00_Project_Overview.md).

---

## Project Structure

```
Sabri_MMO/
├── client/SabriMMO/                 # UE5 5.7 project
│   ├── Source/SabriMMO/             # C++ source
│   │   ├── *.h/cpp                  # 13 core files (GameInstance, Character, etc.)
│   │   ├── UI/                      # 33 subsystems + 30+ Slate widgets
│   │   └── VFX/                     # 3 VFX files (Niagara skill effects)
│   └── Content/SabriMMO/            # Blueprint assets (.uasset, not in git)
├── server/
│   ├── src/index.js                 # Monolithic server (~32,200 lines)
│   ├── src/ro_*.js                  # 11 data modules (~6,000 lines)
│   └── .env                         # DB credentials, JWT secret
├── database/
│   ├── init.sql                     # Core schema (users, characters, items, inventory, hotbar)
│   └── migrations/                  # 25 migration files
├── docsNew/                         # Project documentation (see INDEX.md)
├── RagnaCloneDocs/                  # RO Classic reference (28 design + implementation guides)
└── CLAUDE.md                        # Claude Code project instructions
```

---

## Documentation

All project documentation lives in `docsNew/`. See [INDEX.md](DocsNewINDEX.md) for the full navigation.

| Section | What's There |
|---------|-------------|
| [00_Global_Rules/](../00_Global_Rules/) | Design standards, coding rules |
| [01_Architecture/](../01_Architecture/) | System, multiplayer, database architecture |
| [02_Client_Side/](../02_Client_Side/) | C++ code docs, Blueprint docs (legacy) |
| [03_Server_Side/](../03_Server_Side/) | Server systems (combat, skills, buffs, etc.) |
| [04_Integration/](../04_Integration/) | Auth flow, networking protocol |
| [05_Development/](../05_Development/) | Plans, audits, research, migration guides |
| [06_Reference/](../06_Reference/) | API, event, ID references, glossary |
| [06_Audit/](../06_Audit/) | Codebase and VFX audit reports |
| [items/](../items/) | Item database (weapons, armor, cards, etc.) |
| [RagnaCloneDocs/](../../RagnaCloneDocs/) | RO Classic game design reference |

---

## Server Data Modules

| Module | Lines | Purpose |
|--------|-------|---------|
| `ro_buff_system.js` | 1,179 | 95 buff type definitions + apply/expire/modifier logic |
| `ro_damage_formulas.js` | 1,079 | Physical/magical damage, HIT/FLEE, element/size tables |
| `ro_status_effects.js` | 711 | 10 status effects (stun, freeze, etc.) |
| `ro_ground_effects.js` | 622 | Ground AoE zones (traps, songs, dances, ensembles) |
| `ro_monster_skills.js` | 548 | 40+ NPC_ monster skills across 12+ monsters |
| `ro_card_prefix_suffix.js` | 541 | Card naming system (prefix/suffix per card) |
| `ro_item_effects.js` | 520 | Consumable use effects, item scripts |
| `ro_skill_data_2nd.js` | 275 | 224 second-class skill definitions |
| `ro_homunculus_data.js` | 243 | 4 homunculus types, growth tables, derived stats |
| `ro_skill_data.js` | 196 | 69 first-class skill definitions |
| `ro_arrow_crafting.js` | 77 | 45 arrow crafting recipes |

---

**Last Updated**: 2026-03-20
**Engine**: Unreal Engine 5.7
**Server**: Node.js 18+ LTS
