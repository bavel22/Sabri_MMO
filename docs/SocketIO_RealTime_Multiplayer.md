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
| `UpdateInterval` | Float | 0.033 (30Hz) |
| `TimeSinceLastUpdate` | Float | Accumulator for tick |

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

### Server → Client Events

| Event | Data | Description |
|-------|------|-------------|
| `player:joined` | `{success: true}` | Join acknowledged |
| `player:moved` | `{characterId, characterName, x, y, z, timestamp}` | Other player moved |
| `player:left` | `{characterId, characterName}` | Other player disconnected |
| `player:stats` | `{characterId, stats, derived}` | Base stats + derived stats |
| `chat:receive` | `{type, channel, senderId, senderName, message, timestamp}` | Chat message received |
| `combat:health_update` | `{characterId, health, maxHealth, mana, maxMana}` | Health/mana state sync |
| `combat:damage` | `{attackerId, attackerName, targetId, targetName, isEnemy, damage, targetHealth, targetMaxHealth, timestamp}` | Damage dealt |
| `combat:death` | `{killedId, killedName, killerId, killerName, isEnemy, timestamp}` | Player killed |
| `combat:respawn` | `{characterId, characterName, health, maxHealth, mana, maxMana, x, y, z, teleport, timestamp}` | Player respawned (teleport=true) |
| `combat:auto_attack_started` | `{targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs}` | Auto-attack confirmed |
| `combat:auto_attack_stopped` | `{reason}` | Auto-attack ended |
| `combat:target_lost` | `{reason, isEnemy}` | Target died/disconnected/respawned |
| `combat:out_of_range` | `{targetId, isEnemy, targetX, targetY, targetZ, distance, requiredRange}` | Attacker out of melee range |
| `combat:error` | `{message}` | Combat validation error |
| `enemy:spawn` | `{enemyId, templateId, name, level, health, maxHealth, x, y, z}` | Enemy spawned/respawned |
| `enemy:death` | `{enemyId, enemyName, killerId, killerName, isEnemy, exp, timestamp}` | Enemy killed |
| `enemy:health_update` | `{enemyId, health, maxHealth, inCombat}` | Enemy health changed |

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
Print String: "Socket.io Connected!"
Set IsConnected = true
    ↓
Print String: "Emitting player:join for char: " + CharacterId
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
```

**Important**: Field names must be lowercase (`characterId`, not `characterID`)

### Step 3: Event Tick - Send Position Updates

```
Event Tick (Delta Seconds)
    ↓
Branch: IsConnected?
    ↓ (True)
TimeSinceLastUpdate + Delta Seconds → Set TimeSinceLastUpdate
    ↓
Branch: TimeSinceLastUpdate >= UpdateInterval (0.033)
    ↓ (True)
Set TimeSinceLastUpdate = 0
    ↓
Get Actor Location → Break Vector (X, Y, Z)
    ↓
Construct Json Object
    ↓
Set String Field ("characterId", CharacterId.ToString)
Set String Field ("x", X.ToString)
Set String Field ("y", Y.ToString)
Set String Field ("z", Z.ToString)
    ↓
Construct Json Object Value
    ↓
Get SocketIO → Emit ("player:position", Json Value)
```

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

### Step 5: OnPlayerLeft - Remove Disconnected Players

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

1. **Death/Respawn UI**: Death overlay with "Return to Save Point" button
2. **Damage Numbers**: Floating text above targets showing damage dealt
3. **Target Frame on HUD**: Target name + HP bar panel
4. **Attack Animations**: Attack montage during auto-attack cycle
5. **Stat System**: STR/AGI/VIT/INT/DEX/LUK with RO damage formulas
6. **Enemy AI/NPC System**: AI-controlled enemies
7. **Client-Side Prediction**: Make local movement feel more responsive

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

**Last Updated**: 2026-02-06
**Version**: 0.9.0
**Status**: Combat System Tuning Complete, RO-Style Auto-Attack Active
