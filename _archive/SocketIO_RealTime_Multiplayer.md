# Socket.io Real-Time Multiplayer System

Complete documentation for the Socket.io integration between UE5 client and Node.js server.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Server Implementation](#server-implementation)
4. [Client Implementation](#client-implementation)
5. [Event Reference](#event-reference)
6. [Blueprint Setup](#blueprint-setup)
7. [Troubleshooting](#troubleshooting)

---

## Overview

The Socket.io system enables real-time bidirectional communication between the UE5 client and Node.js server for multiplayer functionality.

### Features
- Real-time position updates (30Hz)
- Player join/leave notifications
- Remote player spawning with CharacterMovement-based movement
- Initial position broadcast from database on player join
- Player disconnect cleanup
- JWT token authentication
- Redis position caching
- Broadcast to all connected players
- 5+ player scale tested
- Player name tags above characters
- Proper walk/run animations on remote players via CharacterMovement

### Technology Stack
- **Server**: Node.js + Socket.io 4.7.4
- **Client**: UE5 + SocketIOClient Plugin (getnamo)
- **Cache**: Redis 7.2.3
- **Protocol**: WebSocket with HTTP fallback

---

## Architecture

```
┌─────────────────┐         WebSocket         ┌─────────────────┐
│   UE5 Client    │ ◄───────────────────────► │  Node.js Server │
│                 │                           │                 │
│ BP_SocketManager│                           │ Socket.io       │
│  - Connect      │                           │  - Events       │
│  - Emit         │                           │  - Broadcast    │
│  - Receive      │                           │                 │
│  - Spawn Players│                           │                 │
└────────┬────────┘                           └────────┬────────┘
         │                                             │
         │         ┌──────────────────┐               │
         │         │ BP_OtherPlayer   │               │
         │         │ Manager          │               │
         │         │ - Spawn Actors   │               │
         │         │ - Track Players  │               │
         │         └────────┬─────────┘               │
         │                  │                         │
         ▼                  ▼                         ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│ BP_OtherPlayer  │  │ BP_OtherPlayer  │  │     Redis       │
│ Character       │  │ Character       │  │  Position Cache │
│ (Remote P1)     │  │ (Remote P2)     │  │                 │
└─────────────────┘  └─────────────────┘  └─────────────────┘
```

### Data Flow

1. **Player Join**: Client emits `player:join` with characterId and token
2. **Initial Position**: Server fetches player's DB position, broadcasts to other clients immediately
3. **Position Update**: Client emits `player:position` at 30Hz
4. **Broadcast**: Server broadcasts `player:moved` to all other clients
5. **Spawn/Update**: Client spawns or updates remote player actors at correct position
6. **Disconnect**: Server broadcasts `player:left`, client destroys actor
7. **Cache**: Server stores position in Redis with 5-minute TTL

---

## Server Implementation

### File: `server/src/index.js`

#### Socket.io Server Setup

```javascript
const { Server } = require('socket.io');
const http = require('http');

// Create HTTP server from Express app
const server = http.createServer(app);

// Initialize Socket.io with CORS
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

// Start server
server.listen(PORT, async () => {
    logger.info(`MMO Server running on port ${PORT}`);
    logger.info(`Socket.io ready`);
});
```

#### Connected Players Tracking

```javascript
const connectedPlayers = new Map();
// Key: characterId
// Value: { socketId, characterId }
```

#### Event Handlers

**Connection Event**
```javascript
io.on('connection', (socket) => {
    logger.info(`Socket connected: ${socket.id}`);
    
    // Handle player join
    socket.on('player:join', async (data) => {
        const { characterId, token, characterName } = data;
        
        // Store in connected players map with name
        connectedPlayers.set(characterId, {
            socketId: socket.id,
            characterId: characterId,
            characterName: characterName || 'Unknown'
        });
        
        logger.info(`Player joined: ${characterName || 'Unknown'} (Character ${characterId})`);
        socket.emit('player:joined', { success: true });
        
        // Fetch player's position from database and broadcast to others
        try {
            const charResult = await pool.query(
                'SELECT x, y, z FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (charResult.rows.length > 0) {
                const pos = charResult.rows[0];
                await setPlayerPosition(characterId, pos.x, pos.y, pos.z);
                socket.broadcast.emit('player:moved', {
                    characterId, characterName,
                    x: pos.x, y: pos.y, z: pos.z,
                    timestamp: Date.now()
                });
                logger.info(`Broadcasted initial position for ${characterName} (Character ${characterId}) at (${pos.x}, ${pos.y}, ${pos.z})`);
            }
        } catch (err) {
            logger.error(`Failed to fetch/broadcast initial position for character ${characterId}:`, err.message);
        }
    });
    
    // Handle position updates
    socket.on('player:position', async (data) => {
        const { characterId, x, y, z } = data;
        const player = connectedPlayers.get(characterId);
        const characterName = player ? player.characterName : 'Unknown';
        
        // Cache in Redis
        await setPlayerPosition(characterId, x, y, z);
        
        // Broadcast to other players with name
        socket.broadcast.emit('player:moved', {
            characterId,
            characterName,
            x, y, z,
            timestamp: Date.now()
        });
        logger.info(`Broadcasted player:moved for ${characterName} (Character ${characterId})`);
    });
    
    // Handle disconnect
    socket.on('disconnect', () => {
        logger.info(`Socket disconnected: ${socket.id}`);
        
        // Remove from connected players
        for (const [charId, player] of connectedPlayers.entries()) {
            if (player.socketId === socket.id) {
                connectedPlayers.delete(charId);
                logger.info(`Player left: ${player.characterName || 'Unknown'} (Character ${charId})`);
                
                // Broadcast to other players using io (socket already disconnected)
                io.emit('player:left', { 
                    characterId: charId,
                    characterName: player.characterName || 'Unknown'
                });
                break;
            }
        }
    });
});
```

#### Redis Helper Functions

```javascript
// Cache player position (5-minute TTL)
async function setPlayerPosition(characterId, x, y, z, zone = 'default') {
    const key = `player:${characterId}:position`;
    const position = JSON.stringify({ x, y, z, zone, timestamp: Date.now() });
    await redisClient.setEx(key, 300, position);
    logger.debug(`Cached position for character ${characterId}`);
}

// Retrieve cached position
async function getPlayerPosition(characterId) {
    const key = `player:${characterId}:position`;
    const data = await redisClient.get(key);
    return data ? JSON.parse(data) : null;
}

// Get all players in a zone
async function getPlayersInZone(zone = 'default') {
    const keys = await redisClient.keys('player:*:position');
    const players = [];
    for (const key of keys) {
        const data = await redisClient.get(key);
        if (data) {
            const pos = JSON.parse(data);
            if (pos.zone === zone) {
                const characterId = key.split(':')[1];
                players.push({ characterId: parseInt(characterId), ...pos });
            }
        }
    }
    return players;
}
```

### Dependencies

```json
{
    "socket.io": "^4.7.4",
    "redis": "^4.6.12"
}
```

---

## Client Implementation

### Plugin: SocketIOClient-Unreal

**Source**: https://github.com/getnamo/SocketIOClient-Unreal

**Installation**:
1. Clone plugin to `client/SabriMMO/Plugins/SocketIOClient/`
2. Enable plugin in `.uproject` file
3. Regenerate project files
4. Rebuild project

### BP_SocketManager Actor

**Location**: `Content/Blueprints/BP_SocketManager`

**Components**:
- `SocketIOClientComponent` (auto-attached by plugin)

**Variables**:
| Variable | Type | Description |
|----------|------|-------------|
| `ServerURL` | String | `http://localhost:3001` |
| `IsConnected` | Boolean | Connection state |
| `CharacterId` | Integer | Current character ID |
| `AuthToken` | String | JWT token from login |
| `PositionTimerHandle` | Timer Handle | Timer for 30Hz position emission (replaces Tick accumulator) |
| `bIsAutoAttacking` | Boolean | Auto-attack loop active |
| `TargetEnemyId` | Integer | Currently targeted enemy's ID |
| `IsTargetingEnemy` | Boolean | True if target is an enemy |
| `bHUDManagerReady` | Boolean | Whether AC_HUDManager is ready to receive updates |
| `BufferedHealthUpdates` | String Array | Array of buffered health update JSON strings |

---

## Event Reference

### Client → Server Events

| Event | Data | Description |
|-------|------|-------------|
| `player:join` | `{characterId, token, characterName}` | Player enters world |
| `player:position` | `{characterId, x, y, z}` | Position update (30Hz) |
| `chat:message` | `{channel, message}` | Chat message |
| `combat:attack` | `{targetCharacterId?, targetEnemyId?}` | Start auto-attacking player or enemy |
| `combat:stop_attack` | `{}` | Stop auto-attacking |
| `combat:respawn` | `{}` (optional data) | Request respawn after death |
| `player:allocate_stat` | `{statName, amount}` | Allocate stat points (str/agi/vit/int/dex/luk) |
| `player:request_stats` | (none) | Request current stats (used when opening stat window) |
| `inventory:load` | (none) | Request full inventory load |
| `inventory:use` | `{inventoryId}` | Use a consumable item |
| `inventory:equip` | `{inventoryId, equip}` | Equip or unequip an item |
| `inventory:drop` | `{inventoryId, quantity?}` | Drop/discard an item |

### Server → Client Events

| Event | Data | Description |
|-------|------|-------------|
| `player:joined` | `{success: true}` | Join acknowledged |
| `player:moved` | `{characterId, characterName, x, y, z, health, maxHealth, timestamp}` | Other player moved (includes health) |
| `player:left` | `{characterId, characterName}` | Other player disconnected |
| `player:stats` | `{characterId, stats, derived}` | Base stats + derived stats |
| `chat:receive` | `{type, channel, senderId, senderName, message, timestamp}` | Chat message received |
| `combat:health_update` | `{characterId, health, maxHealth, mana, maxMana}` | Health/mana state sync |
| `combat:damage` | `{attackerId, attackerName, targetId, targetName, isEnemy, damage, targetHealth, targetMaxHealth, attackerX, attackerY, attackerZ, targetX, targetY, targetZ, timestamp}` | Damage dealt (includes positions for remote rotation) |
| `combat:death` | `{killedId, killedName, killerId, killerName, isEnemy, targetHealth, targetMaxHealth, timestamp}` | Player killed (killer does NOT receive combat:target_lost) |
| `combat:respawn` | `{characterId, characterName, health, maxHealth, mana, maxMana, x, y, z, teleport, timestamp}` | Player respawned (teleport=true) |
| `combat:auto_attack_started` | `{targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs}` | Auto-attack confirmed |
| `combat:auto_attack_stopped` | `{reason}` | Auto-attack ended |
| `combat:target_lost` | `{reason, isEnemy}` | Target died/disconnected/respawned |
| `combat:out_of_range` | `{targetId, isEnemy, targetX, targetY, targetZ, distance, requiredRange}` | Attacker out of range (requiredRange = attackRange - RANGE_TOLERANCE) |
| `combat:error` | `{message}` | Combat validation error |
| `enemy:spawn` | `{enemyId, templateId, name, level, health, maxHealth, x, y, z}` | Enemy spawned/respawned |
| `enemy:death` | `{enemyId, enemyName, killerId, killerName, isEnemy, exp, timestamp}` | Enemy killed |
| `enemy:health_update` | `{enemyId, health, maxHealth, inCombat}` | Enemy health changed |
| `enemy:move` | `{enemyId, x, y, z, targetX?, targetY?, isMoving}` | Enemy wandering position update |
| `inventory:data` | `{items[]}` | Full inventory contents |
| `inventory:used` | `{inventoryId, itemId, itemName, healed, spRestored, health, maxHealth, mana, maxMana}` | Consumable used successfully |
| `inventory:equipped` | `{inventoryId, itemId, itemName, equipped, slot, weaponType, attackRange, aspd, attackIntervalMs}` | Item equipped/unequipped |
| `inventory:dropped` | `{inventoryId, itemId, itemName, quantity}` | Item dropped/discarded |
| `inventory:error` | `{message}` | Inventory operation failed |
| `loot:drop` | `{enemyId, enemyName, items[{itemId, itemName, quantity, icon, itemType}]}` | Loot dropped from enemy kill |

### Server Logs

All socket events are logged with directional prefixes:

| Prefix | Meaning | Level |
|--------|---------|-------|
| `[RECV]` | Event received from client | INFO (combat/chat), DEBUG (position) |
| `[SEND]` | Event sent to specific client | INFO |
| `[BROADCAST]` | Event sent to all clients | INFO (combat), DEBUG (position) |
| `[COMBAT]` | Combat system action | INFO |

```
[INFO] Socket connected: [socket-id]
[INFO] [RECV] player:join from [socket-id]: {characterId, token, characterName}
[INFO] [SEND] player:joined to [socket-id]: {success: true}
[INFO] [SEND] combat:health_update to [socket-id]: {characterId, health, maxHealth, mana, maxMana}
[DEBUG] [RECV] player:position from [socket-id]: {characterId, x, y, z}
[DEBUG] [BROADCAST] player:moved for [name] (Character [id])
[INFO] [RECV] combat:attack from [socket-id]: {targetCharacterId}
[INFO] [SEND] combat:auto_attack_started to [socket-id]: {targetId, targetName, attackRange, aspd, attackIntervalMs}
[INFO] [COMBAT] [attacker] hit [target] for [damage] damage (HP: [remaining]/[max])
[INFO] [BROADCAST] combat:damage: {attackerId, targetId, damage, targetHealth, ...}
[INFO] [SEND] combat:target_lost to [socket-id]: {reason}
[INFO] [BROADCAST] combat:death: {killedId, killedName, killerId, killerName}
[INFO] [BROADCAST] combat:respawn: {characterId, health, maxHealth, x, y, z}
[INFO] [BROADCAST] player:left: {characterId, characterName}
```

---

## Blueprint Setup

### Step 1: Event BeginPlay - Connect to Server

```
Event BeginPlay
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Is Authenticated? (Branch)
    ↓ (True)
Get AuthToken → Set AuthToken variable
Get SelectedCharacterId → Set CharacterId variable
    ↓
Print String: "Attempting SocketIO Connect..."
    ↓
Get SocketIO → Connect (URL: ServerURL)
    ↓
Bind Event to OnConnected (custom event: OnSocketConnected)
```

### Step 2: OnSocketConnected - Emit player:join

```
OnSocketConnected (Custom Event)
    ↓
Set IsConnected = true
    ↓
Construct Json Object
    ↓
Set String Field (FieldName: "characterId", StringValue: CharacterId → To String)
    ↓
Set String Field (FieldName: "token", StringValue: AuthToken)
    ↓
Get Game Instance → Cast to MMOGameInstance → Get SelectedCharacter → Break → Get Name
    ↓
Set String Field (FieldName: "characterName", StringValue: Name)
    ↓
Construct Json Object Value (from Json Object)
    ↓
Get SocketIO → Emit (Event Name: "player:join", Message: Json Value)
    ↓
Bind Event to Function (EventName: "player:moved", FunctionName: "OnPlayerMoved", Target: Self)
Bind Event to Function (EventName: "player:left", FunctionName: "OnPlayerLeft", Target: Self)
... (combat, enemy, chat bindings)
    ↓
★ Inventory Event Bindings (Phase 1.1 Fix):
Bind Event to Function (EventName: "inventory:data", FunctionName: "OnInventoryData")
Bind Event to Function (EventName: "inventory:used", FunctionName: "OnItemUsed")
Bind Event to Function (EventName: "inventory:equipped", FunctionName: "OnItemEquipped")  ← WAS "inventory:used" (BUG)
Bind Event to Function (EventName: "inventory:error", FunctionName: "OnInventoryError")  ← WAS "inventory:used" (BUG)
Bind Event to Function (EventName: "inventory:dropped", FunctionName: "OnItemDropped")
Bind Event to Function (EventName: "loot:drop", FunctionName: "OnLootDrop")
    ↓
★ Loot Event Forwarding:
OnLootDrop → Parse JSON → Forward to AC_HUDManager.ShowLootPopup()
    ↓
★ Health Update Binding (with buffering):
Bind Event to Function (EventName: "combat:health_update", FunctionName: "OnCombatHealthUpdate", Target: Self)
```

> **Phase 1.1 Bug Fix:** Previously, `OnItemEquipped` and `OnInventoryError` were both bound to `inventory:used` instead of their correct event names `inventory:equipped` and `inventory:error`. This caused only one handler to fire (last binding wins) or all three to fire on the same event.

> **Health Update Buffering**: The `combat:health_update` event is bound to `OnCombatHealthUpdate` which implements client-side buffering to handle the 0.5-second delay before AC_HUDManager is ready.

**Important**: Field names must be lowercase (`characterId`, not `characterID`)

---

### Client-Side Health Buffering

**Why Buffering is Needed:**
The server sends `combat:health_update` immediately when a player joins, but the client's AC_HUDManager component (which creates WBP_GameHud) isn't ready until 0.5 seconds later due to player spawn requirements. Without buffering, the initial health update is lost and health bars appear empty.

**Buffer Variables in BP_SocketManager:**
| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `bHUDManagerReady` | Boolean | false | Whether AC_HUDManager is ready to receive updates |
| `BufferedHealthUpdates` | String Array | [] | Array of buffered health update JSON strings |

**Buffering Logic:**
1. **When `combat:health_update` received:**
   - If `bHUDManagerReady` is true → Forward to AC_HUDManager immediately
   - If `bHUDManagerReady` is false → Add to `BufferedHealthUpdates` array

2. **When AC_HUDManager is created (after 0.5s delay):**
   - Set `bHUDManagerReady = true`
   - For each item in `BufferedHealthUpdates` array → Flush to AC_HUDManager
   - Clear the `BufferedHealthUpdates` array

**Why Array Buffering is Needed:**
The server sends 3 separate `combat:health_update` events for each player join:
1. Self health to joining player
2. Broadcast new player's health to existing players  
3. Send existing players' health to joining player

Without array buffering, only the last health update would be stored, causing incorrect health displays for subsequent players joining the game.

**This prevents race conditions** and ensures health bars appear correctly for all players, regardless of join order or component initialization timing. This is a standard multiplayer pattern for handling multiple client-side component readiness events.

---

### Step 3: Timer-Based Position Updates (Replaces Tick)

As of the refactor, BP_SocketManager uses a Timer Handle instead of Event Tick for position updates. This is cleaner and avoids unnecessary Tick calls when the socket isn't connected.

**Variables Added:**
| Variable | Type | Default | Purpose |
|----------|------|---------|---------|
| `PositionTimerHandle` | Timer Handle | Empty | Timer for 30Hz position emission |

**Step 3.1: Create EmitPositionUpdate Function**
```
Function: EmitPositionUpdate
    (No inputs, No outputs)

    ↓
Branch: IsConnected?
    ├─ FALSE: Return (safety check)
    └─ TRUE:
        Get Player Character (Index 0) → Get Actor Location → Break Vector (X, Y, Z)
            ↓
        Construct Json Object
            ↓
        Set String Field ("characterId", CharacterId → ToString)
        Set String Field ("x", X → ToString)
        Set String Field ("y", Y → ToString)
        Set String Field ("z", Z → ToString)
            ↓
        Construct Json Object Value (from Json Object)
            ↓
        Get SocketIOClient → Emit ("player:position", Json Value)
```

**Step 3.2: Start Timer in OnSocketConnected (NOT BeginPlay)**
In `OnSocketConnected`, AFTER the player:join emit and event bindings:
```
... (existing: Emit player:join, Bind events)
    ↓
Set Timer by Function Name
    Function Name: "EmitPositionUpdate"  ← String, exact match
    Time: 0.033  ← (30Hz, or use 0.1 for 10Hz)
    Looping: ✓ Checked
    → Return Value → Set PositionTimerHandle
```

**Step 3.3: Clear Timer on Disconnect**
In Event EndPlay:
```
Event EndPlay
    ↓
Clear Timer by Handle → Handle: PositionTimerHandle
```

**Step 3.4: Remove Old Tick Logic**
- Delete the Event Tick node (or just the position emission section)
- Delete variables: `TimeSinceLastUpdate`, `UpdateInterval`
- Keep Tick only if it handles other things beyond position emission

> **Important**: Timer must start in OnSocketConnected, NOT BeginPlay. Socket connection is async — starting the timer in BeginPlay would emit player:position before the connection is established.

### Step 4: OnPlayerMoved - Spawn/Update Other Players

```
Function: OnPlayerMoved
    Input: Data (String)
    ↓
Value From Json String (Data)
    ↓
As Object
    ↓
Try Get String Field ("characterId") → String → To Integer → MovedCharacterId
Try Get String Field ("characterName") → String → Store as PlayerName
Try Get String Field ("x") → String → To Float → X
Try Get String Field ("y") → String → To Float → Y
Try Get String Field ("z") → String → To Float → Z
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Get SelectedCharacter → Break FCharacterData
    ↓
Get CharacterId → LocalCharacterId
    ↓
Branch: MovedCharacterId != LocalCharacterId (skip self)
    ↓ (True)
Get Actor of Class (BP_OtherPlayerManager)
    ↓
Cast to BP_OtherPlayerManager
    ↓
SpawnOrUpdatePlayer(MovedCharacterId, X, Y, Z)
    ↓
Set PlayerName variable on spawned actor
```

### OnLootDrop - Forward to HUD Manager

```
Function: OnLootDrop
    Input: Data (String)  [Must be named "Data" exactly]
    ↓
Value From Json String (Data)
    ↓
As Object
    ↓
Try Get String Field ("enemyName") → EnemyName
Try Get Array Field ("items") → ItemsArray
    ↓
Branch: Is Valid (HUDManager)?
    ├─ TRUE:
    │   └─ Call ShowLootPopup (HUDManager, EnemyName, ItemsArray)
    └─ FALSE:
        └─ Print String: "HUDManager not available for loot display"
```

**Note**: BP_SocketManager no longer creates loot popups directly. It forwards the parsed data to AC_HUDManager which manages the popup lifecycle.

```
Function: OnPlayerLeft
    Input: Data (String)  [Must be named "Data" exactly]
    ↓
Value From Json String (Data)
    ↓
As Object
    ↓
Try Get String Field ("characterId") → String → To Integer → DisconnectedId
    ↓
Get Actor of Class (BP_OtherPlayerManager)
    ↓
Cast to BP_OtherPlayerManager
    ↓
RemovePlayer(DisconnectedId)
```

**Critical**: Function input parameter must be named "Data" (case-sensitive)

### Node Reference

| Node | Category | Purpose |
|------|----------|---------|
| `Construct Json Object` | SIOJ\|Json | Create empty JSON object |
| `Set String Field` | SIOJ\|Json | Add string field to object |
| `Construct Json Object Value` | SIOJ\|Json | Convert object to value for Emit |
| `Bind Event to Function` | SocketIO Functions | Bind server event to BP function |
| `Emit` | SocketIO Functions | Send event to server |
| `Connect` | SocketIO Functions | Connect to server URL |

---

## BPI_Damageable Interface Integration (Phase 3)

As of Phase 3, `BP_SocketManager` uses the `BPI_Damageable` Blueprint Interface for unified damage and health display handling, replacing class-specific Cast chains.

### Interface Functions Used

| Function | Purpose |
|----------|---------|
| `UpdateHealthDisplay` | Updates health bar widget and visibility based on combat state |
| `ReceiveDamageVisual` | Rotates target actor toward attacker location |
| `GetHealthInfo` | Returns health widget component reference |

### OnEnemyHealthUpdate (Interface-Based)

```
OnEnemyHealthUpdate (Data: String)
    ↓
Parse JSON: enemyId, health, maxHealth, inCombat
    ↓
Get Actor of Class → BP_EnemyManager → Cast to BP_EnemyManager
    ↓
GetEnemyActor(EnemyId) → EnemyActor, WasFound
    ↓
Branch: WasFound?
    ├─ TRUE:
    │   Does Object Implement Interface (EnemyActor, BPI_Damageable)
    │       ↓
    │   Branch: Result?
    │       ├─ TRUE:
    │       │   UpdateHealthDisplay (Message)
    │       │       Target: EnemyActor
    │       │       NewHealth: health
    │       │       NewMaxHealth: maxHealth
    │       │       InCombat: inCombat
    │       └─ FALSE: Print String warning
    └─ FALSE: (skip)
    ↓
★ Existing HUD target frame update logic unchanged
```

### OnCombatDamage (Interface-Based)

```
OnCombatDamage (Data: String)
    ↓
Parse JSON: attackerId, targetId, isEnemy, damage, targetHealth, targetMaxHealth, attackerX/Y/Z
    ↓
Branch: isEnemy?
    ├─ TRUE (enemy target):
    │   GetEnemyActor(targetId) → EnemyActor, WasFound
    │       ↓
    │   Branch: WasFound?
    │       ├─ TRUE:
    │       │   Does Object Implement Interface (EnemyActor, BPI_Damageable)
    │       │       ↓
    │       │   Branch: Result?
    │       │       ├─ TRUE:
    │       │       │   UpdateHealthDisplay (Message) → EnemyActor
    │       │       │   ReceiveDamageVisual (Message) → EnemyActor
    │       │       │       Damage: damage
    │       │       │       AttackerLocation: Make Vector(attackerX, attackerY, attackerZ)
    │       │       └─ FALSE: (skip)
    │       └─ FALSE: (skip)
    │   ★ Existing HUD UpdateTargetHealth call unchanged
    │
    └─ FALSE (player target):
        Get Game Instance → MMOGameInstance → GetCurrentCharacterId → LocalCharId
            ↓
        Branch: targetId == LocalCharId?
            ├─ TRUE (local player):
            │   Get Player Character → Does Object Implement Interface
            │       ↓
            │   Branch: Result?
            │       ├─ TRUE:
            │       │   UpdateHealthDisplay (Message) → PlayerCharacter
            │       │   ReceiveDamageVisual (Message) → PlayerCharacter
            │       └─ FALSE: (skip)
            │
            └─ FALSE (remote player):
                Get Actor of Class → BP_OtherPlayerManager → Cast
                    ↓
                GetOtherPlayerActor(targetId) → PlayerActor, WasFound
                    ↓
                Branch: WasFound?
                    ├─ TRUE:
                    │   Does Object Implement Interface (PlayerActor, BPI_Damageable)
                    │       ↓
                    │   Branch: Result?
                    │       ├─ TRUE:
                    │       │   UpdateHealthDisplay (Message) → PlayerActor
                    │       │   ReceiveDamageVisual (Message) → PlayerActor
                    │       └─ FALSE: (skip)
                    └─ FALSE: (skip)
```

### GetOtherPlayerActor Function

Added to `BP_OtherPlayerManager` for safe remote player actor retrieval:

```
Function: GetOtherPlayerActor
    Input: CharacterId (Integer)
    Output: PlayerActor (BP_OtherPlayerCharacter Object Reference), WasFound (Boolean)

    ↓
OtherPlayers Map → Find (Key: CharacterId)
    ↓
Branch: Was Found?
    ├─ TRUE:
    │   Cast to BP_OtherPlayerCharacter (Return Value)
    │       ↓
    │   Branch: (Cast Succeeded)
    │       ├─ TRUE: Set PlayerActor = Cast Result, Set WasFound = true
    │       └─ FALSE: Set WasFound = false
    └─ FALSE: Set WasFound = false
```

### Design Patterns Applied

| Pattern | Before | After |
|---------|--------|-------|
| Cast Chain | `Cast To BP_EnemyCharacter` → call `UpdateEnemyHealth` | `Does Object Implement Interface` → `UpdateHealthDisplay (Message)` |
| Class-Specific | Direct function calls on specific Blueprint classes | Interface message calls work on any implementing class |

### Related Files

| File | Purpose |
|------|---------|
| `docs/BPI_Damageable.md` | Full interface documentation |
| `Content/Blueprints/BP_EnemyCharacter.uasset` | Implements interface |
| `Content/Blueprints/BP_OtherPlayerCharacter.uasset` | Implements interface |
| `Content/Blueprints/BP_MMOCharacter.uasset` | Implements interface (stub) |
| `Content/Blueprints/BP_OtherPlayerManager.uasset` | Added GetOtherPlayerActor |

---

## Troubleshooting

### Issue: "Emitting player:join for char: 0"

**Cause**: BP_SocketManager spawned before character selected

**Fix**: Place BP_SocketManager in "Enter World" level, not login level

---

### Issue: "Player joined: Character undefined"

**Cause**: Field name case mismatch in JSON

**Fix**: Use `characterId` (lowercase 'd'), not `characterID`

---

### Issue: No position updates in server logs

**Cause**: Server handler had no logging

**Fix**: Add logging to `player:position` handler in server/src/index.js

---

### Issue: Cannot find SIOJson nodes

**Solution**: 
1. Ensure SocketIOClient plugin is enabled
2. Right-click and search for "Construct Json Object" (category: SIOJ\|Json)
3. Search for "Set String Field" (category: SIOJ\|Json)

---

### Issue: OnPlayerLeft not firing

**Cause**: Function input parameter named incorrectly

**Fix**: Rename input parameter from "Arg" or "InData" to exactly "Data"

---

### Issue: Remote player slides without animating

**Cause**: Using SetActorLocation instead of CharacterMovement

**Fix**: Use Add Movement Input in Event Tick so CharacterMovement drives velocity/acceleration for ABP_unarmed

---

### Issue: Remote player walks from origin on spawn

**Cause**: SpawnActor not using correct (x,y,z) coordinates, or server not broadcasting initial position

**Fix**: 
1. Wire SpawnActor's Spawn Transform Location to incoming (x,y,z) in BP_OtherPlayerManager
2. Initialize TargetPosition = GetActorLocation() in BP_OtherPlayerCharacter BeginPlay
3. Server broadcasts DB position immediately on player:join

---

### Issue: Local player character invisible

**Cause**: Mesh underground after implementing other player system

**Fix**: Restart UE5 (viewport/rendering glitch)

---

## Testing

### Single Player Test

1. Start server: `npm run dev`
2. Play in UE5
3. Login and select character
4. Check server logs:
   ```
   [INFO] Player joined: Character [id]
   [INFO] Position update received: Character [id] at (x, y, z)
   [INFO] Broadcasted player:moved for Character [id]
   ```

### Two Player Test

1. Play in Editor with 2 players (PIE)
2. Move Player 1
3. Check Player 2's world - should see Player 1's character
4. Move Player 2
5. Check Player 1's world - should see Player 2's character

### Five Player Scale Test

1. Create test users (run `node database/create_test_users.js`)
2. Open 5 client windows (PIE or packaged builds)
3. Login with different accounts (1, 2, 3, 4, 5)
4. Verify all players see each other
5. Move around and verify smooth interpolation
6. Disconnect one player - verify character disappears from others

**Expected Results:**
- ✓ All players visible to each other
- ✓ Position updates at 30Hz
- ✓ Smooth interpolation (no jitter)
- ✓ Disconnect cleanup working

---

## Performance Notes

- **Update Rate**: 30Hz (33ms interval) for position updates
- **Redis TTL**: 5 minutes for position cache
- **Broadcast**: `socket.broadcast.emit` excludes sender for position updates
- **Disconnect**: `io.emit` used for player:left (socket already disconnected)
- **Movement**: CharacterMovement-based Add Movement Input in BP_OtherPlayerCharacter
- **Scale Tested**: 5+ concurrent players on localhost
- **Connection**: WebSocket with automatic fallback

---

## Next Steps

1. **Skills System**: Skill data structure, target skills, skill hotbar
2. **Client-Side Prediction**: Make local movement feel more responsive
3. **Multiple Maps**: Zone transitions and warps
4. **Equipment Visuals**: Show equipped items on character mesh

---

## Files Modified

### Server
- `server/src/index.js` - Socket.io events, player:left broadcast, Redis integration, initial position broadcast on join
- `server/package.json` - Added socket.io, redis dependencies

### Client
- `Content/Blueprints/BP_SocketManager.uasset` - Socket manager with OnPlayerLeft and characterName handling
- `Content/Blueprints/BP_OtherPlayerCharacter.uasset` - Remote player actor with CharacterMovement-based movement and name tag widget
- `Content/Blueprints/BP_OtherPlayerManager.uasset` - Player spawning/management singleton
- `Content/Blueprints/BP_MMOCharacter.uasset` - Local player with name tag widget
- `Content/Blueprints/Widgets/WBP_PlayerNameTag.uasset` - Name tag display widget
- `Source/SabriMMO/MMOGameInstance.h/.cpp` - Auth token storage
- `Plugins/SocketIOClient/` - Socket.io plugin

### Database
- `database/create_test_users.js` - Test user creation script

### Documentation
- `docs/SocketIO_RealTime_Multiplayer.md` - This file
- `docs/BP_OtherPlayerCharacter.md` - Remote player character documentation
- `docs/BP_OtherPlayerManager.md` - Player manager documentation
- `docs/WBP_PlayerNameTag.md` - Name tag widget documentation
- `docs/JSON_Communication_Protocol.md` - JSON event formats
- `README.md` - Phase 2 progress update
- `RUNNING.md` - Redis startup instructions

---

**Last Updated**: 2026-02-16
**Version**: 0.13.1
**Status**: Client-side health buffering updated to array-based approach for multi-player support
