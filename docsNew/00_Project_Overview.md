# Sabri_MMO — Project Overview

## Executive Summary

**Sabri_MMO** is a class-based action MMORPG built as a solo developer project. It combines an Unreal Engine 5.7 client (C++ + Blueprints) with a Node.js backend (Express + Socket.io), PostgreSQL for persistence, and Redis for real-time caching. The architecture is **server-authoritative** — all combat, stats, inventory, and position logic is validated server-side.

## Technology Stack

| Layer | Technology | Version |
|-------|-----------|---------|
| **Game Client** | Unreal Engine 5.7 | 5.7 (C++ 17/20 + Blueprint) |
| **Web Framework** | Express.js | 4.18.2 |
| **Real-Time** | Socket.io | 4.8.3 |
| **Database** | PostgreSQL | 15.4 |
| **Cache** | Redis | 7.2+ (client v5.10.0) |
| **Auth** | JSON Web Tokens | jsonwebtoken 9.0.3 |
| **Password Hash** | bcrypt | 5.1.1 |
| **Rate Limiting** | express-rate-limit | 8.2.1 |

## Architecture Diagram

```
┌──────────────────────────┐         ┌──────────────────────────┐         ┌─────────────────┐
│     UE5.7 Client         │  HTTP   │     Node.js Server       │  SQL    │   PostgreSQL    │
│                          │◄───────►│                          │◄───────►│                 │
│ • C++ Core Classes       │         │ • Express REST API       │         │ • users         │
│ • Blueprint Game Logic   │ Socket  │ • Socket.io Events       │         │ • characters    │
│ • UMG Widget UI          │◄═══════►│ • Combat Tick Loop       │  Redis  │ • items         │
│ • SocketIOClient Plugin  │  .io    │ • Enemy AI Loop          │◄───────►│ • char_inventory│
│ • HttpModule (REST)      │         │ • JWT Auth Middleware     │  Cache  │                 │
└──────────────────────────┘         └──────────────────────────┘         └─────────────────┘
```

## Implemented Systems

### Authentication & Characters
- JWT-based login/register via REST API
- Character CRUD (create, list, select, delete)
- `FCharacterData` C++ struct for client-side data
- `UMMOGameInstance` persists auth state across level loads

### Real-Time Multiplayer
- Socket.io bidirectional communication
- `player:join` / `player:position` / `player:moved` / `player:left` events
- Redis position caching with 5-minute TTL
- Remote player spawning, interpolation, and name tags
- Tested with 5+ concurrent players

### Combat System (Ragnarok Online-Style)
- Server-authoritative auto-attack loop (50ms tick)
- ASPD-based attack intervals (0–190 scale)
- Player-vs-Player and Player-vs-Enemy targeting
- Range checking with tolerance padding
- Death/respawn cycle with HP restore and teleport
- Kill messages broadcast in COMBAT chat channel

### Stat System (RO-Style)
- 6 base stats: STR, AGI, VIT, INT, DEX, LUK
- Derived stats: statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, ASPD, maxHP, maxSP
- Stat point allocation with server validation
- Stats loaded from DB on join, saved on disconnect

### Enemy/NPC System
- 6 enemy templates: Blobby, Hoplet, Crawlid, Shroomkin, Buzzer, Mosswort
- 12 spawn points with configurable wander radius
- RO-style wandering AI (random movement within spawn radius)
- Server-side health tracking, death, and timed respawn
- Drop tables with chance-based loot rolling

### Items & Inventory
- 16 base items: 5 consumables, 8 loot/etc, 6 weapons, 3 armor
- PostgreSQL `items` (definitions) + `character_inventory` (per-character)
- Socket.io events: load, use consumable, equip/unequip, drop/discard
- Equipped weapon modifies ATK, range, and ASPD
- Stackable items with max stack limits

### Chat System
- Global chat channel via Socket.io
- `chat:message` (client→server) / `chat:receive` (server→all)
- COMBAT channel for kill messages
- Expandable architecture (ZONE, PARTY, GUILD, TELL planned)

### Movement & Camera
- Top-down click-to-move with NavMesh pathfinding
- WASD keyboard controls (cancels click-to-move)
- Spring Arm camera with right-click rotation
- Mouse scroll zoom (200–1500 units)
- Character faces movement direction

### UI Widgets (Blueprint) — 18 Total
- `WBP_LoginScreen` — Login/register
- `WBP_CharacterSelect` / `WBP_CharacterEntry` / `WBP_CreateCharacter` — Character management
- `WBP_GameHUD` — HP/MP bars, target frame
- `WBP_PlayerNameTag` — Floating name above characters
- `WBP_ChatWidget` / `WBP_ChatMessageLine` — Chat interface
- `WBP_StatAllocation` — Stat point distribution
- `WBP_TargetHealthBar` — World-space health bar on enemies/players
- `WBP_InventoryWindow` / `WBP_InventorySlot` — Inventory grid with icons, double-click, right-click, drag-and-drop
- `WBP_ContextMenu` — Right-click context menu (Equip/Use/Drop)
- `WBP_DragVisual` — Ghost widget during inventory drag
- `WBP_ItemTooltip` — Hover tooltip for inventory slots
- `WBP_DeathOverlay` — Death screen with respawn button
- `WBP_LootPopup` — Loot notification popup with auto-fade
- `WBP_DamageNumber` — Floating damage numbers

## Project Directory Structure

```
C:/Sabri_MMO/
├── client/SabriMMO/              # UE5 project
│   ├── Source/SabriMMO/          # C++ source (19 core files + 76 variant files)
│   │   ├── CharacterData.h       # FCharacterData struct
│   │   ├── MMOGameInstance.*     # Auth state, character list, events
│   │   ├── MMOHttpManager.*      # REST API client (BlueprintFunctionLibrary)
│   │   ├── SabriMMOCharacter.*   # Base player character
│   │   ├── SabriMMOPlayerController.*  # Input mapping
│   │   ├── SabriMMOGameMode.*    # Game mode stub
│   │   ├── OtherCharacterMovementComponent.*  # Remote player movement
│   │   ├── SabriMMO.*            # Module definition + log category
│   │   ├── SabriMMO.Build.cs     # Build config (14 module dependencies)
│   │   ├── Variant_Combat/       # Combat system (42 files)
│   │   ├── Variant_Platforming/  # Platforming variant (8 files)
│   │   └── Variant_SideScrolling/ # Side-scrolling variant (26 files)
│   ├── Content/SabriMMO/         # Blueprint assets (.uasset — not in git)
│   └── SabriMMO.uproject         # UE5 5.7 project file
├── server/
│   ├── src/index.js              # Monolithic server (2269 lines)
│   ├── package.json              # 10 dependencies
│   ├── .env                      # DB credentials, JWT secret
│   └── logs/                     # Runtime logs
├── database/
│   ├── init.sql                  # Schema: users, characters, items, character_inventory
│   ├── create_test_users.*       # Test user scripts
│   └── insert_test_user.sql
├── scripts/                      # Utility scripts
├── docs/                         # Legacy documentation (31 files)
├── docsNew/                      # This comprehensive documentation
└── .gitignore
```

## UE5 Plugin Dependencies

| Plugin | Purpose |
|--------|---------|
| **ModelingToolsEditorMode** | In-editor modeling |
| **StateTree** | AI behavior trees |
| **GameplayStateTree** | Gameplay-specific state trees |
| **VisualStudioTools** | VS integration (Win64) |
| **Diversion** | Version control plugin |
| **RemoteControl** | Remote control API |

## C++ Module Dependencies (SabriMMO.Build.cs)

```
Core, CoreUObject, Engine, InputCore, EnhancedInput,
AIModule, StateTreeModule, GameplayStateTreeModule,
UMG, Slate, HTTP, Json, JsonUtilities
```

## Server Dependencies (package.json)

| Package | Version | Purpose |
|---------|---------|---------|
| express | ^4.18.2 | REST API framework |
| socket.io | ^4.8.3 | Real-time WebSocket |
| pg | ^8.11.3 | PostgreSQL client |
| redis | ^5.10.0 | Redis cache client |
| bcrypt | ^5.1.1 | Password hashing |
| jsonwebtoken | ^9.0.3 | JWT authentication |
| cors | ^2.8.5 | Cross-origin requests |
| express-rate-limit | ^8.2.1 | API rate limiting |
| dotenv | ^16.3.1 | Environment variables |
| nodemon | ^3.0.2 | Dev auto-restart |

## Current Status

- **Phase**: Foundation + Multiplayer + Combat + Inventory (server-side complete)
- **Next**: Blueprint UI for inventory, equipment, and loot display
- **Known Issues**: None critical — all resolved (see `docs/Bug_Fix_Notes.md`)

---

**Last Updated**: 2026-02-17
**Engine**: Unreal Engine 5.7
**Server**: Node.js 18+ LTS
