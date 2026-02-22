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

### 2. Socket.io Connection
```
Client → Connects to ws://localhost:3001 via SocketIOClient plugin
Server → io.on('connection') fires
```

### 3. Player Join
```
Client → socket.emit('player:join', {characterId, token, characterName})
Server → Loads character data from DB (position, health, stats, weapon)
Server → Caches position in Redis
Server → Stores player in connectedPlayers Map
Server → socket.emit('player:joined', {success: true})
Server → socket.emit('combat:health_update', {health, maxHealth, mana, maxMana})
Server → socket.emit('player:stats', {stats, derived})
Server → Sends existing enemies to joining player
Server → socket.broadcast.emit('player:moved', {...}) for initial position
```

### 4. Position Synchronization
```
Client → socket.emit('player:position', {characterId, x, y, z}) at ~30Hz
Server → Cache in Redis (setEx with 300s TTL)
Server → socket.broadcast.emit('player:moved', {characterId, characterName, x, y, z, health, maxHealth, timestamp})
```

### 5. Disconnect
```
Client → Disconnects (browser close, network loss, etc.)
Server → socket.on('disconnect')
Server → Save health/mana to DB
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

## Blueprint Client Architecture

### BP_SocketManager
- Singleton actor spawned in game level
- Holds Socket.io connection reference
- Binds all socket events to Blueprint functions
- Each binding uses `Bind Event to Function` with param named `Data` (String type)
- Parses JSON strings into usable data

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

---

**Last Updated**: 2026-02-17
