# Multiplayer Architecture

## Overview

Sabri_MMO uses a **server-authoritative** multiplayer model where the Node.js server is the single source of truth for all game state. Clients send inputs and requests; the server validates, processes, and broadcasts results to all relevant clients via Socket.io.

## Network Topology

```
                    ┌──────────────────────┐
                    │    Node.js Server     │
                    │    (Port 3001)        │
                    │                       │
                    │  ┌─────────────────┐  │
                    │  │   Socket.io     │  │
                    │  │   Event Hub     │  │
                    │  └────────┬────────┘  │
                    │           │            │
                    │  ┌────────┴────────┐  │
                    │  │ connectedPlayers│  │
                    │  │   Map<id,data>  │  │
                    │  └─────────────────┘  │
                    └───┬───────┬───────┬───┘
                        │       │       │
              WebSocket │       │       │ WebSocket
                        │       │       │
                   ┌────┴──┐ ┌──┴───┐ ┌─┴─────┐
                   │Client1│ │Client2│ │ClientN│
                   │ UE5   │ │ UE5  │ │ UE5   │
                   └───────┘ └──────┘ └───────┘
```

## Connection Lifecycle

### 1. Authentication (HTTP REST)
```
Client → POST /api/auth/login {username, password}
Server → Validates credentials, returns JWT token
Client → Stores token in UMMOGameInstance
```

### 2. Socket.io Connection (Phase 4 — Persistent Socket)

The socket connection is established **once** during login and persists for the entire play session. It lives on `UMMOGameInstance` as a `TSharedPtr<FSocketIONative>` (getnamo's SocketIOClient plugin), which survives `OpenLevel()` calls.

```
LoginFlowSubsystem::OnPlayCharacter()
  → GI->ConnectSocket("http://server:3001")
  → FSocketIONative settings:
      bUnbindEventsOnDisconnect = false  (handlers survive network blips)
      bCallbackOnGameThread = true       (safe for UObject access)
      Infinite reconnection              (auto-reconnects on network loss)
  → On connect callback: emit player:join
  → OpenLevel(TargetMap)
```

**Pre-Phase 4 (Legacy)**: Socket lived on `BP_SocketManager`'s SocketIO component in each level. Every `OpenLevel()` destroyed the socket, causing disconnect/reconnect cycles, `player:left`/`player:joined` spam, and requiring `reconnectBuffCache` on the server.

### 3. Player Join
```
Client → socket.emit('player:join', {characterId, token, characterName})
Server → Validates JWT token, checks character ownership
Server → Loads character data from DB (position, health, stats, weapon)
Server → Caches position in Redis
Server → Stores player in connectedPlayers Map
Server → socket.emit('player:joined', {success: true})
Server → socket.emit('combat:health_update', {health, maxHealth, mana, maxMana})
Server → socket.emit('player:stats', {stats, derived})
Server → socket.emit('buff:list', {buffs: [...]})
Server → Sends existing enemies to joining player
Server → socket.broadcast.emit('player:moved', {...}) for initial position
```

### 4. Position Synchronization
```
PositionBroadcastSubsystem (30Hz timer, UWorldSubsystem)
  → Every 33.3ms: GI->EmitSocketEvent('player:position', {characterId, x, y, z, yaw})
Server → Cache in Redis (setEx with 300s TTL)
Server → socket.broadcast.emit('player:moved', {characterId, characterName, x, y, z, health, maxHealth, timestamp})
```

### 5. Zone Transition (Socket Stays Connected)
```
Client → Warp portal / Kafra teleport / Fly Wing triggered
Client → GI->EmitSocketEvent('zone:change', {characterId, targetZone, warpId})
Server → Moves player between zone Socket.io rooms (no disconnect)
Client → OpenLevel(NewMap)
         ↓ Old level destroyed — old UWorldSubsystems Deinitialize()
         ↓ Socket SURVIVES on GameInstance
         ↓ New level loads — new UWorldSubsystems Initialize()
         ↓ MultiplayerEventSubsystem re-registers with SocketEventRouter
         ↓ PositionBroadcastSubsystem restarts 30Hz timer
Client → emit zone:ready
Server → Sends zone-specific data (enemies, ground effects, players)
```

No `player:left`/`player:joined` spam. Buffs persist in-memory. No `reconnectBuffCache` needed.

### 6. Disconnect (Logout or Shutdown)
```
Client → GI->Logout() or GI->Shutdown()
Client → emit player:leave
Client → GI->DisconnectSocket()
Client → NativeSocket.Reset()

Server → socket.on('disconnect')
Server → Save health/mana/zone/position to DB
Server → Save stats to DB
Server → Clear auto-attack state
Server → Clear from enemy combat sets
Server → Remove from connectedPlayers
Server → io.emit('player:left', {characterId, characterName})
```

## Socket.io Event Catalog

### Player Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `player:join` | C→S | `{characterId, token, characterName}` | Join game world |
| `player:joined` | S→C | `{success: true}` | Join confirmed |
| `player:position` | C→S | `{characterId, x, y, z}` | Position update (~30Hz) |
| `player:moved` | S→C* | `{characterId, characterName, x, y, z, health, maxHealth, timestamp}` | Broadcast position |
| `player:left` | S→All | `{characterId, characterName}` | Player disconnected |
| `player:stats` | S→C | `{characterId, stats, derived}` | Full stat snapshot |
| `player:request_stats` | C→S | _(empty)_ | Request current stats |
| `player:allocate_stat` | C→S | `{statName, amount}` | Allocate stat point |

### Combat Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `combat:attack` | C→S | `{targetCharacterId?}` or `{targetEnemyId?}` | Start auto-attack |
| `combat:stop_attack` | C→S | _(empty)_ | Stop auto-attack |
| `combat:auto_attack_started` | S→C | `{targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs}` | Confirm attack started |
| `combat:auto_attack_stopped` | S→C | `{reason, oldTargetId?, oldIsEnemy?}` | Attack stopped |
| `combat:damage` | S→All | `{attackerId, attackerName, targetId, targetName, isEnemy, damage, targetHealth, targetMaxHealth, attackerX/Y/Z, targetX/Y/Z, timestamp}` | Damage dealt |
| `combat:health_update` | S→All | `{characterId, health, maxHealth, mana, maxMana}` | Health sync |
| `combat:out_of_range` | S→C | `{targetId, isEnemy, targetX/Y/Z, distance, requiredRange}` | Move closer |
| `combat:target_lost` | S→C | `{reason, isEnemy}` | Target unavailable |
| `combat:death` | S→All | `{killedId, killedName, killerId, killerName, isEnemy, targetHealth, targetMaxHealth, timestamp}` | Player killed |
| `combat:respawn` | C→S / S→All | `{characterId, characterName, health, maxHealth, mana, maxMana, x, y, z, teleport, timestamp}` | Respawn |
| `combat:error` | S→C | `{message}` | Combat error |

### Enemy Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `enemy:spawn` | S→All | `{enemyId, templateId, name, level, health, maxHealth, x, y, z}` | Enemy spawned |
| `enemy:move` | S→All | `{enemyId, x, y, z, targetX?, targetY?, isMoving}` | Enemy position |
| `enemy:death` | S→All | `{enemyId, enemyName, killerId, killerName, isEnemy, isDead, exp, timestamp}` | Enemy killed |
| `enemy:health_update` | S→All | `{enemyId, health, maxHealth, inCombat}` | Enemy HP |

### Chat Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `chat:message` | C→S | `{channel, message}` | Send chat message |
| `chat:receive` | S→All | `{type, channel, senderId, senderName, message, timestamp}` | Receive message |

### Inventory Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `inventory:load` | C→S | _(empty)_ | Request full inventory |
| `inventory:data` | S→C | `{items: [...]}` | Full inventory data |
| `inventory:use` | C→S | `{inventoryId}` | Use consumable |
| `inventory:used` | S→C | `{inventoryId, itemId, itemName, healed, spRestored, health, maxHealth, mana, maxMana}` | Item used |
| `inventory:equip` | C→S | `{inventoryId, equip}` | Equip/unequip |
| `inventory:equipped` | S→C | `{inventoryId, itemId, itemName, equipped, slot, weaponType, attackRange, aspd, attackIntervalMs}` | Equip result |
| `inventory:drop` | C→S | `{inventoryId, quantity?}` | Drop/discard item |
| `inventory:dropped` | S→C | `{inventoryId, itemId, itemName, quantity}` | Drop confirmed |
| `inventory:error` | S→C | `{message}` | Inventory error |

### Loot Events

| Event | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `loot:drop` | S→C | `{enemyId, enemyName, items: [{itemId, itemName, quantity, icon, itemType}]}` | Loot from kill |

## Position Caching (Redis)

- **Key Format**: `player:{characterId}:position`
- **Value**: JSON `{x, y, z, zone, timestamp}`
- **TTL**: 300 seconds (5 minutes)
- **Used By**: Combat tick loop for range checks, zone queries

```javascript
// Write
await redisClient.setEx(`player:${charId}:position`, 300, JSON.stringify({x, y, z, zone, timestamp}));

// Read
const data = await redisClient.get(`player:${charId}:position`);
const pos = JSON.parse(data);
```

## Combat Tick Architecture

The combat tick loop runs every 50ms (`COMBAT.COMBAT_TICK_MS`) and processes all active auto-attack entries:

```
For each (attackerId, attackState) in autoAttackState:
  1. Skip if attacker dead or disconnected
  2. Check ASPD timing (now - lastAttackTime >= attackInterval)
  3. Get positions from Redis
  4. Check range (distance <= attackRange)
     - If out of range → emit 'combat:out_of_range'
     - If in range → execute attack:
       a. Calculate damage
       b. Reduce target HP
       c. Broadcast 'combat:damage' to all
       d. If target HP <= 0 → process death
```

## Enemy AI Architecture

The enemy AI tick runs every 500ms (`ENEMY_AI.WANDER_TICK_MS`):

```
For each enemy:
  Skip if dead or in combat (inCombatWith.size > 0)
  
  If not wandering:
    If time >= nextWanderTime:
      Pick random point within wanderRadius of spawn
      Set isWandering = true
  
  If wandering:
    Move toward target at WANDER_SPEED (60 units/sec)
    Broadcast 'enemy:move' every 200ms
    If arrived (distance < 10):
      Stop, schedule next wander (3-8s pause)
      Broadcast final position with isMoving=false
```

## Client Socket Architecture (Phase 4 — Persistent Socket)

### Architecture Overview

```
┌───────────────────────────────────────────────────────────────────┐
│                     UMMOGameInstance                               │
│                     (Survives OpenLevel)                          │
│                                                                   │
│  TSharedPtr<FSocketIONative> NativeSocket ←── THE connection     │
│  USocketEventRouter* EventRouter ←── multi-handler dispatch       │
│  ConnectSocket() / DisconnectSocket() / EmitSocketEvent()        │
│  IsSocketConnected() → bool                                      │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────────┐  │
│  │                  USocketEventRouter                         │  │
│  │                                                             │  │
│  │  TMap<EventName, TArray<TSharedPtr<FEntry>>>                │  │
│  │    "combat:damage" → [BasicInfoSub, DmgNumSub, VFXSub]    │  │
│  │    "player:moved"  → [MultiplayerEventSub]                 │  │
│  │    "buff:list"     → [BuffBarSub, BasicInfoSub]            │  │
│  │                                                             │  │
│  │  FEntry = { TWeakObjectPtr Owner, Callback }               │  │
│  │  TSharedPtr<FEntry> for stable lambda captures              │  │
│  └─────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘
         │                    │                    │
    RegisterHandler     DispatchEvent        UnregisterAll
         │                    │                    │
┌────────┴────────────────────┴────────────────────┴────────────────┐
│                    UWorldSubsystems (per-level)                    │
│                    (Destroyed/recreated on OpenLevel)              │
│                                                                    │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │MultiplayerEvent  │  │PositionBroadcast │  │ BasicInfoSub    │  │
│  │  Subsystem       │  │  Subsystem       │  │ DamageNumSub    │  │
│  │                  │  │                  │  │ CombatStatsSub  │  │
│  │ Bridges ~30      │  │ 30Hz timer       │  │ BuffBarSub      │  │
│  │ events to        │  │ emits position   │  │ SkillVFXSub     │  │
│  │ BP_SocketManager │  │ via GI socket    │  │ CastBarSub      │  │
│  │ via FindFunction │  │                  │  │ etc.            │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                    BP_SocketManager                           │  │
│  │    (Handler Shell — SocketIO component NOT connected)        │  │
│  │                                                              │  │
│  │    BeginPlay: Set auth vars + actor refs + HUD init          │  │
│  │    NO Connect / NO Bind / NO Timer                           │  │
│  │    Handler functions called by MultiplayerEventSubsystem     │  │
│  │    Emit functions use K2_EmitSocketEvent on GameInstance     │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────┘
```

### USocketEventRouter

Solves the problem of multiple C++ subsystems needing to listen to the same Socket.io event. The native Socket.io API only supports one callback per event name, so the router multiplexes.

```cpp
// Registration (in subsystem Initialize)
Router->RegisterHandler("combat:damage", this, [this](auto Payload) {
    // Handle damage event
});

// Unregistration (in subsystem Deinitialize)
Router->UnregisterAllForOwner(this);

// Internal dispatch (called by FSocketIONative callback)
Router->DispatchEvent("combat:damage", Payload);
// → Iterates all handlers, skips expired WeakOwner entries
```

### MultiplayerEventSubsystem (Blueprint Bridge)

Bridges ~30 inbound Socket.io events to `BP_SocketManager` handler functions. On `Initialize()`, registers with the `SocketEventRouter` for all events that `BP_SocketManager` previously bound directly. Each handler finds the BP_SocketManager actor and calls its handler function via `FindFunction()` + `ProcessEvent()`.

**BlueprintCallable emit methods** for Blueprints that need to send events:
- `EmitCombatAttack(TargetId, IsEnemy)` — starts auto-attack
- `EmitStopAttack()` — stops auto-attack
- `RequestRespawn()` — respawn after death
- `EmitChatMessage(Channel, Message)` — send chat

### PositionBroadcastSubsystem

Replaces the per-frame position broadcasting that was previously in `BP_SocketManager`'s Event Tick. Runs a 30Hz timer (33.3ms interval) and emits `player:position` with `{characterId, x, y, z, yaw}` via `GI->EmitSocketEvent()`.

### Widget Visibility Gating

All HUD subsystem widgets are gated behind `GI->IsSocketConnected()`. This prevents widgets from appearing during the login flow or in editor previews. Each subsystem checks this in its `Initialize()` or widget creation path. Only when the socket is connected (i.e., player is in a game level) do widgets get added to the viewport.

### Blueprint Emit Pattern

**Old pattern** (pre-Phase 4):
```
Get SocketIO Component → Emit(EventName, SIOJsonObject)
```

**New pattern** (Phase 4):
```
Get Game Instance → Cast To MMOGameInstance → Emit Socket Event(EventName, SIOJsonObject*)
```

### BP_SocketManager (Handler Shell Status)

`BP_SocketManager` still exists as an actor in game levels but its role has changed:

| Aspect | Pre-Phase 4 | Phase 4 |
|--------|-------------|---------|
| **SocketIO Component** | Connected, binds events | Present but NOT connected |
| **BeginPlay** | Connect + Bind + Start Timer | Set auth vars + actor refs + HUD init |
| **Event Binding** | Direct Blueprint bindings | Handlers called by MultiplayerEventSubsystem |
| **Event Emit** | Via SocketIO component | Via `K2_EmitSocketEvent` on GameInstance |
| **Position Timer** | Event Tick at variable rate | Removed (PositionBroadcastSubsystem handles) |
| **Destroyed on OpenLevel** | Yes (and socket dies) | Yes (but socket survives on GI) |

### BP_OtherPlayerManager
- Maintains `Map<characterId, BP_OtherPlayerCharacter>`
- `SpawnOrUpdatePlayer(charId, name, x, y, z)` — creates or updates remote actors
- `RemovePlayer(charId)` — destroys remote player actor
- Called from BP_SocketManager on `player:moved` and `player:left` events

### BP_OtherPlayerCharacter
- Represents a remote player in the world
- Uses `CharacterMovementComponent` for smooth interpolation
- Sets `TargetPosition` from server data
- Interpolates toward target on Tick
- Displays `WBP_PlayerNameTag` widget component

### BP_EnemyManager
- Maintains `Map<enemyId, BP_EnemyCharacter>`
- Spawns enemies from `enemy:spawn` events
- Updates positions from `enemy:move` events
- Handles death/respawn lifecycle

## Zone Transition Flow (Socket Perspective)

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

**Server-side simplification**: The `reconnectBuffCache` (which cached buffs/statuses on disconnect for 30s TTL and restored them on `player:join`) is no longer needed. Since the socket never disconnects during zone transitions, buffs remain in the server's in-memory `connectedPlayers` data structures throughout.

## Server Authority Rules

| Action | Client Does | Server Validates |
|--------|------------|-----------------|
| **Attack** | Sends target ID | Checks target exists, not dead, not self |
| **Damage** | Displays result | Calculates damage, applies to HP |
| **Movement** | Moves locally | Caches position, broadcasts to others |
| **Use Item** | Sends inventory ID | Verifies ownership, type, applies effect |
| **Equip** | Sends inventory ID | Checks level requirement, slot, updates stats |
| **Stat Alloc** | Sends stat name | Checks available points, updates DB |
| **Chat** | Sends message | Validates non-empty, routes to channel |
| **Zone Change** | Sends target zone | Validates zone exists, player not dead |

---

**Last Updated**: 2026-03-10
