# Multiplayer Architecture

## Overview

The MMO uses a client-server architecture with Socket.io for real-time communication. Players connect to a Node.js server, authenticate with JWT tokens, and receive position updates for other players in the same zone.

---

## System Components

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              CLIENT (UE5)                                │
│  ┌─────────────────┐  ┌──────────────────┐  ┌──────────────────────┐   │
│  │  BP_MMOCharacter│  │BP_OtherPlayer    │  │  BP_SocketManager    │   │
│  │  (Local Player) │  │Character         │  │  (Connection Handler)│   │
│  │                 │  │(Remote Players)  │  │                      │   │
│  │ - Player input  │  │                  │  │ - Connect to server  │   │
│  │ - Sends position│  │ - Interpolated   │  │ - Emit position      │   │
│  │ - Local camera  │  │ - Server-driven  │  │ - Receive updates    │   │
│  └────────┬────────┘  └────────┬─────────┘  └──────────┬───────────┘   │
│           │                    │                       │               │
│           │            ┌───────┴───────┐               │               │
│           │            │BP_OtherPlayer │               │               │
│           │            │   Manager     │               │               │
│           │            │               │               │               │
│           │            │ - Spawn/Update│               │               │
│           │            │ - Track all   │               │               │
│           │            │ - Destroy     │               │               │
│           │            └───────┬───────┘               │               │
│           │                    │                       │               │
└───────────┼────────────────────┼───────────────────────┼───────────────┘
            │                    │                       │
            │                    │         WebSocket     │
            │                    │         (Socket.io)   │
            │                    │                       │
└───────────┼────────────────────┼───────────────────────┼───────────────┘
│           │                    │                       │               │
│  ┌────────┴────────────────────┴───────────────────────┴───────────┐   │
│  │                        NODE.JS SERVER                             │   │
│  │  ┌─────────────────────────────────────────────────────────────┐  │   │
│  │  │                      Socket.io                              │  │   │
│  │  │  - player:join      (client → server)                       │  │   │
│  │  │  - player:position  (client → server, 30Hz)                 │  │   │
│  │  │  - player:moved     (server → client, broadcast)            │  │   │
│  │  │  - player:left      (server → client, disconnect)           │  │   │
│  │  └─────────────────────────────────────────────────────────────┘  │   │
│  │                              │                                    │   │
│  │                              ▼                                    │   │
│  │  ┌─────────────────────────────────────────────────────────────┐  │   │
│  │  │                      REDIS CACHE                            │  │   │
│  │  │  - Player positions cached                                  │  │   │
│  │  │  - 5-minute TTL                                             │  │   │
│  │  │  - Zone-based lookup                                        │  │   │
│  │  └─────────────────────────────────────────────────────────────┘  │   │
│  └───────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Data Flow

### 1. Player Joins World

```
UE5 Client                                  Node.js Server
───────────                                 ───────────────
    │                                               │
    │ ─────── Socket.io Connect ─────────────────> │
    │                                               │
    │ ─────── Emit player:join ──────────────────> │
    │       {characterId, token}                    │
    │                                               │
    │ <────── Emit player:joined ───────────────── │
    │       {success: true}                         │
    │                                               │
    │ ◄────── Listen player:moved ───────────────► │
    │                                               │
    │ ◄────── Listen player:left ────────────────► │
```

### 2. Position Update (30Hz)

```
UE5 Client A                                Node.js Server                                UE5 Client B
────────────                                ───────────────                               ────────────
    │                                               │                                           │
    │ ─────── Emit player:position ──────────────> │                                           │
    │       {characterId, x, y, z}                  │                                           │
    │                                               │                                           │
    │                                               │ ─────── Broadcast player:moved ─────────> │
    │                                               │       {characterId, x, y, z, timestamp}   │
    │                                               │                                           │
    │                                               │ <────── Spawn/Update Actor ───────────── │
    │                                               │                                           │
```

### 3. Player Disconnect

```
UE5 Client A                                Node.js Server                                UE5 Client B
────────────                                ───────────────                               ────────────
    │                                               │                                           │
    │ ─────── Disconnect ────────────────────────> │                                           │
    │                                               │                                           │
    │                                               │ ─────── Emit player:left ──────────────> │
    │                                               │       {characterId}                       │
    │                                               │                                           │
    │                                               │ <────── Destroy Actor ────────────────── │
```

---

## Actor Types

### BP_MMOCharacter (Local Player)

| Aspect | Description |
|--------|-------------|
| **Purpose** | Player-controlled character |
| **Input** | Enhanced Input (WASD / Click to move) |
| **Movement** | Direct control via CharacterMovement |
| **Camera** | SpringArm + Camera (local view) |
| **Network** | Emits position at 30Hz |
| **Possession** | PlayerController possessed |

**Components:**
- CapsuleComponent (collision)
- SkeletalMesh (visual)
- SpringArm (camera boom)
- Camera (player view)
- CharacterMovement

---

### BP_OtherPlayerCharacter (Remote Player)

| Aspect | Description |
|--------|-------------|
| **Purpose** | Visual representation of other players |
| **Input** | None (server-controlled) |
| **Movement** | Interpolation toward TargetPosition |
| **Camera** | None |
| **Network** | Receives position updates, interpolates |
| **Possession** | None (unpossessed) |

**Components:**
- CapsuleComponent (collision)
- SkeletalMesh (visual)
- CharacterMovement (minimal use)

**Variables:**
| Variable | Type | Purpose |
|----------|------|---------|
| `TargetPosition` | Vector | Last known server position |
| `InterpolationSpeed` | Float | Smoothing factor (10.0) |

**Tick Logic:**
```
Current = Get Actor Location
New = Lerp(Current, TargetPosition, DeltaTime * Speed)
Set Actor Location(New)
```

---

### BP_OtherPlayerManager (Singleton)

| Aspect | Description |
|--------|-------------|
| **Purpose** | Track and manage all remote players |
| **Quantity** | One per level (singleton) |
| **Location** | Enter World level |

**Variables:**
| Variable | Type | Purpose |
|----------|------|---------|
| `OtherPlayerClass` | Class | BP_OtherPlayerCharacter to spawn |
| `OtherPlayers` | Map | characterId → Actor reference |

**Functions:**
- `SpawnOrUpdatePlayer(id, x, y, z)` - Creates or updates remote player
- `RemovePlayer(id)` - Destroys disconnected player's actor

---

### BP_SocketManager (Singleton)

| Aspect | Description |
|--------|-------------|
| **Purpose** | Handle Socket.io connection |
| **Quantity** | One per level (singleton) |
| **Location** | Enter World level |

**Responsibilities:**
1. Connect to Socket.io server
2. Authenticate with JWT token
3. Emit position updates (30Hz)
4. Receive other player positions
5. Forward to BP_OtherPlayerManager

---

## Event Protocol

### Client → Server

| Event | Frequency | Data | Purpose |
|-------|-----------|------|---------|
| `player:join` | Once | `{characterId, token}` | Authenticate and register |
| `player:position` | 30Hz | `{characterId, x, y, z}` | Send position update |

### Server → Client

| Event | Trigger | Data | Purpose |
|-------|---------|------|---------|
| `player:joined` | On join | `{success: true}` | Join acknowledged |
| `player:moved` | Broadcast | `{characterId, x, y, z, timestamp}` | Other player moved |
| `player:left` | Disconnect | `{characterId}` | Other player left |

---

## State Management

### Client State

```
MMOGameInstance (persistent)
├── AuthToken (JWT from login)
├── UserId
├── Username
└── SelectedCharacter (FCharacterData)
    ├── CharacterId
    ├── Name
    ├── Class
    └── Position (x, y, z)

Level BP (Enter World)
├── BP_SocketManager
│   ├── CharacterId (from GameInstance)
│   ├── AuthToken (from GameInstance)
│   └── IsConnected
└── BP_OtherPlayerManager
    └── OtherPlayers (Map: id → actor)
```

### Server State

```
Node.js Server
├── connectedPlayers (Map: characterId → {socketId, characterId})
└── Redis Cache
    └── player:{id}:position → {x, y, z, timestamp, zone}
```

---

## Performance Considerations

### Update Rate
- **Position updates**: 30Hz (33ms interval)
- **Interpolation**: Every frame (DeltaTime-based)
- **Redis TTL**: 5 minutes

### Network Optimization
- Only position changes broadcast (no full state)
- `socket.broadcast.emit` excludes sender
- JSON string serialization for coordinates

### Client Optimization
- Interpolation smooths network jitter
- No physics for remote players (visual only)
- Local player uses standard CharacterMovement

---

## Scaling Strategy

### Current: Single Zone
- All players see each other
- Broadcast to all connected sockets
- Tested up to 5 players

### Future: Zone System
```
World
├── Zone A (City)
│   └── Players 1-20
├── Zone B (Forest)
│   └── Players 21-40
└── Zone C (Dungeon)
    └── Players 41-50
```

- Only broadcast within same zone
- Redis keys include zone: `player:{id}:position:{zone}`
- Interest management for large worlds

---

## Security

### Authentication Flow
1. Login → Receive JWT token
2. Token stored in MMOGameInstance
3. Socket.io connect includes token
4. Server validates token on player:join

### Validation
- Token expiration checked
- Character ownership verified
- Position sanity checks (future)

---

## Troubleshooting Common Issues

### Issue: Players don't see each other
**Check:**
1. Both clients connected to same server
2. BP_OtherPlayerManager exists in level
3. characterId matches between client and server
4. "Skip self" logic working (characterId != localId)

### Issue: Remote players jittery
**Solutions:**
- Increase InterpolationSpeed (try 15-20)
- Check network latency
- Verify 30Hz update rate maintained

### Issue: Remote players drift away
**Fix:** Ensure InterpolationSpeed is positive (10.0), not negative

### Issue: Disconnect not cleaning up
**Check:**
1. OnPlayerLeft event bound correctly
2. Function input named "Data"
3. io.emit used on server (not socket.broadcast.emit)

---

## Related Documentation

- `docs/SocketIO_RealTime_Multiplayer.md` - Socket.io implementation details
- `docs/BP_OtherPlayerCharacter.md` - Remote player actor
- `docs/BP_OtherPlayerManager.md` - Player manager
- `docs/Daily Progress/MMO_Development_Progress_2026-02-04.md` - Daily progress

---

**Last Updated**: 2026-02-04
**Version**: 1.1
**Status**: Multiplayer Spawning Complete

