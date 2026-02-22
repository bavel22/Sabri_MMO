# System Architecture

## Overview

Sabri_MMO follows a **three-tier architecture**: UE5 client ↔ Node.js server ↔ PostgreSQL/Redis. The server is the single source of truth for all game state. Clients send input/requests; the server validates, computes, and broadcasts results.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           GAME CLIENTS (UE5.7)                         │
│                                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │
│  │  Player 1   │  │  Player 2   │  │  Player 3   │  │  Player N   │   │
│  │ C++ + BP    │  │ C++ + BP    │  │ C++ + BP    │  │ C++ + BP    │   │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘   │
│         │                │                │                │           │
│         └────────────────┼────────────────┼────────────────┘           │
│                          │                │                             │
└──────────────────────────┼────────────────┼─────────────────────────────┘
                           │                │
              HTTP REST    │    Socket.io   │
              (Auth, CRUD) │    (Real-time) │
                           │                │
┌──────────────────────────┼────────────────┼─────────────────────────────┐
│                     NODE.JS SERVER (index.js)                           │
│                                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                  │
│  │  Express     │  │  Socket.io   │  │  Game Logic  │                  │
│  │  REST API    │  │  Event Hub   │  │  Tick Loops  │                  │
│  │              │  │              │  │              │                  │
│  │ • /health    │  │ • player:*   │  │ • Combat     │                  │
│  │ • /api/auth  │  │ • combat:*   │  │   (50ms)     │                  │
│  │ • /api/chars │  │ • enemy:*    │  │ • Enemy AI   │                  │
│  │ • /api/pos   │  │ • chat:*     │  │   (500ms)    │                  │
│  │              │  │ • inventory:* │  │ • HP Save    │                  │
│  │              │  │ • loot:*     │  │   (60s)      │                  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                  │
│         │                │                │                             │
└─────────┼────────────────┼────────────────┼─────────────────────────────┘
          │                │                │
          ▼                ▼                ▼
┌──────────────────┐  ┌──────────────────┐
│   PostgreSQL     │  │     Redis        │
│                  │  │                  │
│ • users          │  │ • Position cache │
│ • characters     │  │   (5min TTL)     │
│ • items          │  │ • player:ID:pos  │
│ • char_inventory │  │                  │
└──────────────────┘  └──────────────────┘
```

## Communication Protocols

### HTTP REST (Authentication & CRUD)
- **Transport**: HTTP/1.1 over TCP port 3001
- **Format**: JSON request/response bodies
- **Auth**: JWT Bearer tokens in `Authorization` header
- **Rate Limit**: 100 requests per 15 minutes per IP on `/api/*`
- **Used For**: Login, register, character CRUD, position save

### Socket.io (Real-Time Game Events)
- **Transport**: WebSocket upgrade from HTTP (port 3001, same server)
- **Format**: JSON payloads with event names
- **Patterns**: `emit` (single client), `broadcast.emit` (all except sender), `io.emit` (all clients)
- **Used For**: Position sync, combat, chat, inventory, enemy updates

## Server Tick Loops

| Loop | Interval | Purpose |
|------|----------|---------|
| **Combat Tick** | 50ms | Process auto-attack queue, range checks, damage, death |
| **Enemy AI Tick** | 500ms | Enemy wandering, movement toward wander targets |
| **Health Save** | 60,000ms | Periodic DB save of all connected players' HP/MP |

## In-Memory State (Node.js Process)

| Data Structure | Type | Purpose |
|---------------|------|---------|
| `connectedPlayers` | `Map<charId, PlayerData>` | All online players with combat stats |
| `autoAttackState` | `Map<attackerId, AttackState>` | Active auto-attack targets |
| `enemies` | `Map<enemyId, EnemyData>` | All spawned enemies |
| `itemDefinitions` | `Map<itemId, ItemDef>` | Cached item DB rows |

## Client Architecture (UE5)

### C++ Layer
- **`UMMOGameInstance`** — Persists auth token, username, userId, character list across levels
- **`UHttpManager`** — Static BlueprintFunctionLibrary for REST API calls
- **`ASabriMMOCharacter`** — Base character class with camera boom, input bindings
- **`ASabriMMOPlayerController`** — Input mapping context setup
- **`FCharacterData`** — USTRUCT for character data transfer

### Blueprint Layer
- **`BP_MMOCharacter`** — Local player character (derives from C++ base or standalone)
- **`BP_OtherPlayerCharacter`** — Remote player actor with interpolation
- **`BP_OtherPlayerManager`** — Spawns/destroys remote player actors
- **`BP_SocketManager`** — Socket.io connection and event binding hub
- **`BP_EnemyCharacter`** — Client-side enemy actor
- **`BP_EnemyManager`** — Enemy lifecycle management

### Widget Layer (UMG) — 15 Widgets
- **`WBP_LoginScreen`** — Login/register UI
- **`WBP_CharacterSelect`** / **`WBP_CharacterEntry`** / **`WBP_CreateCharacter`** — Character management
- **`WBP_GameHUD`** — In-game HP/MP bars, target frame
- **`WBP_ChatWidget`** / **`WBP_ChatMessageLine`** — Chat interface
- **`WBP_StatAllocation`** — Stat point distribution
- **`WBP_PlayerNameTag`** — Floating name tag
- **`WBP_TargetHealthBar`** — World-space enemy/player health bar
- **`WBP_InventoryWindow`** / **`WBP_InventorySlot`** — Inventory grid
- **`WBP_DeathOverlay`** — Death screen with respawn button
- **`WBP_LootPopup`** — Loot notification
- **`WBP_DamageNumber`** — Floating damage numbers

## Design Patterns Applied

| Pattern | Application |
|---------|-------------|
| **Server-Authoritative** | All combat, stats, inventory validated server-side |
| **Game Instance** | `UMMOGameInstance` persists auth across level loads |
| **Manager** | `BP_OtherPlayerManager`, `BP_EnemyManager` |
| **Event-Driven** | Delegates in GameInstance; Socket.io events |
| **Component-Based** | `AC_HUDManager`, `AC_CameraController`, `AC_TargetingSystem` on BP_MMOCharacter |
| **Interface** | `BPI_Damageable` (Blueprint), `ICombatAttacker`, `ICombatDamageable`, `ICombatActivatable` (C++ variant) |
| **Blueprint Function Library** | `UHttpManager` as static callable functions |

## Data Flow Examples

### Login Flow
```
Client                          Server                         Database
  │                               │                               │
  │ POST /api/auth/login          │                               │
  │ {username, password} ────────►│                               │
  │                               │ SELECT ... FROM users ───────►│
  │                               │◄─────────── user row ─────────│
  │                               │ bcrypt.compare(password)      │
  │                               │ jwt.sign({user_id, username}) │
  │◄─── {token, user} ───────────│                               │
  │                               │                               │
  │ SetAuthData(token, user, id)  │                               │
  │ → UMMOGameInstance            │                               │
  │ → OnLoginSuccess.Broadcast()  │                               │
```

### Combat Auto-Attack Flow
```
Client                          Server                          All Clients
  │                               │                               │
  │ combat:attack                 │                               │
  │ {targetEnemyId} ─────────────►│                               │
  │                               │ Validate, set autoAttackState │
  │◄─ combat:auto_attack_started ─│                               │
  │                               │                               │
  │                               │ ── Combat Tick (50ms) ──      │
  │                               │ Check range (Redis pos)       │
  │                               │ Check ASPD interval           │
  │                               │ Calculate damage              │
  │                               │ enemy.health -= damage        │
  │                               │                               │
  │                               │ combat:damage ───────────────►│
  │                               │ enemy:health_update ─────────►│
  │                               │                               │
  │                               │ [If enemy.health <= 0]        │
  │                               │ enemy:death ─────────────────►│
  │                               │ loot:drop (to killer only) ──►│
  │                               │ setTimeout(respawn)           │
```

## Scalability Considerations

- **Current**: Single Node.js process, single PostgreSQL instance, single Redis
- **Bottleneck**: `connectedPlayers` Map in memory limits to one process
- **Future**: Redis pub/sub for multi-process Socket.io, connection pooling, read replicas
- **Target**: 1000–5000 concurrent players (per README roadmap)

---

**Last Updated**: 2026-02-17
