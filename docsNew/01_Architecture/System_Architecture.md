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
| **Enemy AI Tick** | 200ms | Full AI state machine (IDLE→CHASE→ATTACK→DEAD), wander, aggro, chase |
| **HP Natural Regen** | 6,000ms | HP regeneration based on VIT |
| **SP Natural Regen** | 8,000ms | SP regeneration based on INT |
| **Skill-Based Regen** | 10,000ms | HP/SP Recovery skill bonuses |
| **Buff Expiry Tick** | 1,000ms | Check/remove expired buffs, Stone Curse HP drain |
| **Ground Effects Tick** | 500ms | Fire Wall damage, Safety Wall blocking, expiry cleanup |
| **Periodic DB Save** | 60,000ms | Save HP/MP, EXP, zone + position for all connected players |

## In-Memory State (Node.js Process)

| Data Structure | Type | Purpose |
|---------------|------|---------|
| `connectedPlayers` | `Map<charId, PlayerData>` | All online players with combat stats |
| `autoAttackState` | `Map<attackerId, AttackState>` | Active auto-attack targets |
| `activeCasts` | `Map<charId, CastData>` | In-progress skill casts |
| `afterCastDelayEnd` | `Map<charId, timestamp>` | After-Cast Delay expiry times |
| `activeGroundEffects` | `Map<effectId, EffectData>` | Fire Wall / Safety Wall effects |
| `enemies` | `Map<enemyId, EnemyData>` | All spawned enemies (509 RO templates) |
| `itemDefinitions` | `Map<itemId, ItemDef>` | Cached item DB rows (148 items) |
| `spawnedZones` | `Set` | Zones with lazy-spawned enemies |

## Client Architecture (UE5)

### C++ Layer
- **`UMMOGameInstance`** — Persists auth token, username, userId, character list across levels. **Owns the persistent Socket.io connection** (`TSharedPtr<FSocketIONative>`) and the `USocketEventRouter` for multi-handler event dispatch. Provides `EmitSocketEvent()`, `ConnectSocket()`, `DisconnectSocket()`, `IsSocketConnected()`.
- **`UHttpManager`** — Static BlueprintFunctionLibrary for REST API calls
- **`ASabriMMOCharacter`** — Base character class with camera boom, input bindings
- **`ASabriMMOPlayerController`** — Dead code (not used at runtime; Blueprint PC inherits from base APlayerController)
- **`FCharacterData`** — USTRUCT with 32 fields (identity, stats, zone, appearance, economy)
- **`FInventoryItem`** / **`FShopItem`** / **`FCartItem`** / **`FServerInfo`** — Additional data structs in CharacterData.h

### C++ Networking Layer (Phase 4 — Persistent Socket)
- **`USocketEventRouter`** — UObject owned by GameInstance. Multi-handler dispatch allowing multiple subsystems to register for the same Socket.io event. Uses `TSharedPtr<FEntry>` for stable lambda captures. API: `RegisterHandler(EventName, Owner, Callback)`, `UnregisterAllForOwner(Owner)`.
- **`UMultiplayerEventSubsystem`** — UWorldSubsystem bridging ~30 inbound Socket.io events to `BP_SocketManager` handler functions via `FindFunction()`/`ProcessEvent()`. Also provides `BlueprintCallable` emit methods: `EmitCombatAttack`, `EmitStopAttack`, `RequestRespawn`, `EmitChatMessage`.
- **`UPositionBroadcastSubsystem`** — UWorldSubsystem running a 30Hz timer broadcasting player position (`characterId, x, y, z, yaw`) via `GI->EmitSocketEvent()`.

### C++ World Subsystems (17)
- **`LoginFlowSubsystem`** — Login→Server→CharSelect→Create→EnterWorld state machine (5 Slate widgets). **Initiates socket connection** in `OnPlayCharacter()`.
- **`BasicInfoSubsystem`** — HP/SP/EXP bars (SBasicInfoWidget, Z=10)
- **`CombatStatsSubsystem`** — Detailed stats panel, F8 toggle (SCombatStatsWidget, Z=12)
- **`DamageNumberSubsystem`** — Floating damage numbers (SDamageNumberOverlay, Z=20)
- **`SkillTreeSubsystem`** — Skill tree + targeting, K toggle (SSkillTreeWidget, Z=20)
- **`CastBarSubsystem`** — Cast bars for all players (SCastBarOverlay, Z=25)
- **`InventorySubsystem`** — Inventory grid, F6 toggle (SInventoryWidget, Z=14)
- **`EquipmentSubsystem`** — Equipment slots, F7 toggle (SEquipmentWidget, Z=15)
- **`HotbarSubsystem`** — 4×9 hotbar, F5 cycle (SHotbarRowWidget×4, Z=16)
- **`WorldHealthBarSubsystem`** — Floating HP/SP bars (SWorldHealthBarOverlay, Z=8)
- **`ZoneTransitionSubsystem`** — Zone warp + loading overlay (Z=50)
- **`KafraSubsystem`** — Kafra NPC dialog (SKafraWidget, Z=19)
- **`ShopSubsystem`** — NPC shop buy/sell (SShopWidget, Z=18)
- **`BuffBarSubsystem`** — Status/buff icons with countdown timers (SBuffBarWidget, Z=11)
- **`SkillVFXSubsystem`** — Niagara/Cascade skill visual effects
- **`MultiplayerEventSubsystem`** — Socket.io event bridge to BP_SocketManager handlers
- **`PositionBroadcastSubsystem`** — 30Hz position broadcasting via GameInstance socket

### Blueprint Layer
- **`BP_MMOCharacter`** — Local player character (derives from C++ base or standalone)
- **`BP_OtherPlayerCharacter`** — Remote player actor with interpolation
- **`BP_OtherPlayerManager`** — Spawns/destroys remote player actors
- **`BP_SocketManager`** — Handler shell for Socket.io events. SocketIO component present but **not connected** — connection lives on GameInstance. BeginPlay sets auth vars, actor refs, and HUD init (no Connect/Bind/Timer calls). Handler functions (`OnCombatDamage`, etc.) called by `MultiplayerEventSubsystem` bridge. Emit functions use `K2_EmitSocketEvent` on GameInstance.
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

## Persistent Socket Architecture (Phase 4)

### Socket Ownership

The Socket.io connection lives on `UMMOGameInstance` as a `TSharedPtr<FSocketIONative>` (from getnamo's SocketIOClient plugin). Because GameInstance survives `OpenLevel()` calls, the socket connection persists across zone transitions without disconnect/reconnect cycles.

```
UMMOGameInstance
  ├── TSharedPtr<FSocketIONative> NativeSocket   ← THE connection
  ├── USocketEventRouter* EventRouter            ← multi-handler dispatch
  ├── ConnectSocket(URL)                         ← called once at login
  ├── DisconnectSocket()                         ← called on logout/shutdown
  ├── EmitSocketEvent(Name, Payload)             ← used by all emitters
  └── IsSocketConnected() → bool                 ← widget visibility gate
```

**Key FSocketIONative settings:**
- `bUnbindEventsOnDisconnect = false` — event handlers survive network blips
- `bCallbackOnGameThread = true` — all callbacks fire on game thread (safe for UObject access)
- Infinite reconnection — plugin auto-reconnects on network loss

### Socket Lifecycle

```
LoginFlowSubsystem::OnPlayCharacter()
  → GI->ConnectSocket("http://server:3001")
  → On connect callback: emit player:join {characterId, token}
  → OpenLevel(TargetMap)

Zone Transition (e.g., warp portal):
  → ZoneTransitionSubsystem::RequestWarp()
  → Socket stays connected (lives on GI, not in level)
  → emit zone:change {characterId, targetZone, warpId}
  → Server moves player between zone rooms (no disconnect)
  → OpenLevel(NewMap)
  → New world subsystems Initialize(), register with EventRouter
  → Old world subsystems Deinitialize(), unregister from EventRouter

Logout / Shutdown:
  → GI->Logout() or GI->Shutdown()
  → emit player:leave
  → GI->DisconnectSocket()
  → NativeSocket.Reset()
```

### SocketEventRouter (Multi-Handler Dispatch)

`USocketEventRouter` is a UObject owned by GameInstance that solves the problem of multiple C++ subsystems needing to listen to the same Socket.io event. The native Socket.io API only supports one callback per event name, so the router multiplexes.

```
USocketEventRouter
  ├── RegisterHandler(EventName, Owner, Callback)
  │     → Adds to TMap<FString, TArray<TSharedPtr<FEntry>>>
  │     → FEntry = { WeakOwner, Callback }
  │     → TSharedPtr ensures lambda captures stay valid
  │
  ├── UnregisterAllForOwner(Owner)
  │     → Removes all entries where WeakOwner matches
  │     → Called by subsystems in Deinitialize()
  │
  └── DispatchEvent(EventName, Payload)
        → Iterates all handlers for EventName
        → Skips entries with expired WeakOwner
        → Calls each valid Callback(Payload)
```

**Example**: Both `BasicInfoSubsystem` and `DamageNumberSubsystem` register for `combat:damage`. When the server sends `combat:damage`, the router calls both handlers.

### MultiplayerEventSubsystem (Blueprint Bridge)

`UMultiplayerEventSubsystem` is a UWorldSubsystem that bridges the gap between the C++ socket layer and the existing Blueprint `BP_SocketManager` handlers. On Initialize, it registers ~30 event handlers with the EventRouter. Each handler finds the corresponding `BP_SocketManager` actor in the level and calls its handler function via `FindFunction()` + `ProcessEvent()`.

```
Socket.io event arrives
  → FSocketIONative callback
  → USocketEventRouter::DispatchEvent()
  → UMultiplayerEventSubsystem handler
  → FindFunction("OnCombatDamage") on BP_SocketManager
  → ProcessEvent(Function, &Params)
  → BP_SocketManager executes existing Blueprint logic
```

It also provides `BlueprintCallable` emit methods for Blueprints that need to send events:
- `EmitCombatAttack(TargetId, IsEnemy)`
- `EmitStopAttack()`
- `RequestRespawn()`
- `EmitChatMessage(Channel, Message)`

### PositionBroadcastSubsystem

`UPositionBroadcastSubsystem` is a UWorldSubsystem that replaces the per-frame position broadcasting that was previously in `BP_SocketManager`'s Event Tick. It runs a 30Hz timer (33.3ms interval) and emits `player:position` with `{characterId, x, y, z, yaw}` via `GI->EmitSocketEvent()`.

### Widget Visibility Gating

All HUD subsystem widgets are gated behind `GI->IsSocketConnected()`. This ensures widgets only appear in game levels (where the socket is connected) and not during the login flow or in editor previews. Each subsystem checks this in its `Initialize()` or widget creation path.

### Blueprint Emit Pattern

**Old pattern** (pre-Phase 4):
```
Get SocketIO Component → Emit(EventName, SIOJsonObject)
```

**New pattern** (Phase 4):
```
Get Game Instance → Cast To MMOGameInstance → Emit Socket Event(EventName, SIOJsonObject*)
```

All Blueprint emit calls go through the GameInstance, which forwards to the persistent `FSocketIONative`. The `BP_SocketManager`'s SocketIO component is present but **not connected** — it exists only as a handler shell.

### Zone Transition Flow (Socket Perspective)

```
Pre-Phase 4:                          Phase 4:
─────────────────                     ──────────────────────
Socket on BP_SocketManager            Socket on GameInstance
  ↓ Warp portal triggered              ↓ Warp portal triggered
  ↓ emit zone:change                   ↓ emit zone:change
  ↓ OpenLevel() destroys level         ↓ OpenLevel() destroys level
  ↓ Socket DESTROYED                   ↓ Socket SURVIVES (on GI)
  ↓ Server sees disconnect             ↓ No disconnect event
  ↓ player:left broadcast              ↓ No player:left spam
  ↓ New level loads                    ↓ New level loads
  ↓ BP_SocketManager connects again    ↓ Subsystems re-register with router
  ↓ player:join re-sent               ↓ zone:ready emitted
  ↓ reconnectBuffCache restore         ↓ Buffs persist in-memory (no cache)
  ↓ Full re-sync from DB              ↓ Seamless transition
```

Benefits:
- No `player:left` / `player:joined` spam on zone change
- Buffs and status effects persist in-memory (no server-side `reconnectBuffCache` needed)
- Other players in the zone see seamless transitions, not disconnect/reconnect flicker
- Faster zone transitions (no TCP handshake + auth round-trip)

## Design Patterns Applied

| Pattern | Application |
|---------|-------------|
| **Server-Authoritative** | All combat, stats, inventory validated server-side |
| **Game Instance** | `UMMOGameInstance` persists auth + socket across level loads |
| **Manager** | `BP_OtherPlayerManager`, `BP_EnemyManager` |
| **Event-Driven** | Delegates in GameInstance; Socket.io events via SocketEventRouter |
| **Multi-Handler Dispatch** | `USocketEventRouter` multiplexes socket events to N subscribers |
| **Blueprint Bridge** | `MultiplayerEventSubsystem` calls BP handler functions via FindFunction/ProcessEvent |
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

**Last Updated**: 2026-03-10
