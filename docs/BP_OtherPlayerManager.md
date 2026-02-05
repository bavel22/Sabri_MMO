# BP_OtherPlayerManager

## Overview
Singleton manager that tracks and controls all remote player actors in the world. Spawns `BP_OtherPlayerCharacter` instances when other players join and destroys them when they disconnect.

---

## File Location
```
Content/Blueprints/BP_OtherPlayerManager.uasset
```

---

## Placement
- **Level**: Enter World (same level as BP_SocketManager)
- **Quantity**: Single instance (singleton pattern)
- **Spawn**: Manually place in level

---

## Variables

### OtherPlayerClass
| Property | Value |
|----------|-------|
| **Name** | `OtherPlayerClass` |
| **Type** | Class (Actor) |
| **Default** | BP_OtherPlayerCharacter |
| **Purpose** | Class to spawn for remote players |

### OtherPlayers
| Property | Value |
|----------|-------|
| **Name** | `OtherPlayers` |
| **Type** | Map (Integer → Actor) |
| **Default** | Empty |
| **Purpose** | Maps characterId to spawned actor instance |

**Example Map Entry:**
```
Key: 24 (characterId)
Value: BP_OtherPlayerCharacter_C_0 (spawned actor reference)
```

---

## Functions

### SpawnOrUpdatePlayer

**Purpose**: Creates or updates a remote player actor

**Inputs:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `characterId` | Integer | Unique ID of the remote player |
| `x` | Float | Target X position from server |
| `y` | Float | Target Y position from server |
| `z` | Float | Target Z position from server |

**Logic Flow:**
```
SpawnOrUpdatePlayer(characterId, x, y, z)
    ↓
Find in Map (OtherPlayers, characterId)
    ↓
Branch: Was Found?
    ↓ (True - Player exists)
    Get Return Value → Actor
    Cast to BP_OtherPlayerCharacter
    Set TargetPosition (x, y, z)
    ↓ (False - New player)
    Spawn Actor from Class
        Class: OtherPlayerClass
        Spawn Transform: Make Transform (Location: x,y,z)
    Set Return Value Map
        Map: OtherPlayers
        Key: characterId
        Value: Spawned Actor
    Set TargetPosition on spawned actor
```

**Called By:**
- `BP_SocketManager.OnPlayerMoved` - When position update received

---

### RemovePlayer

**Purpose**: Destroys a remote player actor when they disconnect

**Inputs:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `characterId` | Integer | ID of player to remove |

**Logic Flow:**
```
RemovePlayer(characterId)
    ↓
Find in Map (OtherPlayers, characterId)
    ↓
Branch: Was Found?
    ↓ (True)
    Get Return Value → Actor to Destroy
    Destroy Actor (Actor to Destroy)
    Remove from Map (OtherPlayers, characterId)
    Print: "Player removed: [characterId]"
    ↓ (False)
    Print: "Player not found: [characterId]"
```

**Called By:**
- `BP_SocketManager.OnPlayerLeft` - When disconnect event received

---

## Event Graph

### Event BeginPlay (Optional Debug)
```
Event BeginPlay
    ↓
Print String: "OtherPlayerManager spawned"
    ↓
Print String: "Tracking remote players..."
```

---

## Usage Example

### Spawning a New Remote Player
```
// BP_SocketManager receives player:moved for character 25
SpawnOrUpdatePlayer(
    characterId: 25,
    x: 1000.0,
    y: 500.0,
    z: 92.0
)

// Result: New BP_OtherPlayerCharacter spawned at (1000, 500, 92)
// Added to OtherPlayers map: {25 → Actor_C_5}
```

### Updating Existing Player Position
```
// Next position update for character 25
SpawnOrUpdatePlayer(
    characterId: 25,
    x: 1010.0,
    y: 510.0,
    z: 92.0
)

// Result: Finds existing actor in map
// Updates TargetPosition to (1010, 510, 92)
// Actor interpolates to new position
```

### Removing Disconnected Player
```
// BP_SocketManager receives player:left for character 25
RemovePlayer(characterId: 25)

// Result: Finds actor in map
// Destroys the actor
// Removes entry from map
```

---

## Troubleshooting

### Issue: Multiple actors spawned for same player
**Cause**: characterId lookup failing (type mismatch)
**Fix**: Ensure characterId is Integer type, not String or Float

### Issue: Actors not destroyed on disconnect
**Cause**: RemovePlayer not being called or characterId mismatch
**Fix**: 
1. Check OnPlayerLeft event is bound
2. Verify JSON parsing gives correct characterId
3. Check Map lookup uses same type as Set

### Issue: Cannot find BP_OtherPlayerManager in level
**Cause**: Not placed in level or wrong level loaded
**Fix**: 
1. Place BP_OtherPlayerManager in Enter World level
2. Use "Get Actor of Class" with BP_OtherPlayerManager class

### Issue: Null reference when spawning
**Cause**: OtherPlayerClass not set
**Fix**: Set OtherPlayerClass variable to BP_OtherPlayerCharacter in Defaults

---

## Related Blueprints

| Blueprint | Relationship |
|-----------|--------------|
| **BP_OtherPlayerCharacter** | Class that gets spawned/instances |
| **BP_SocketManager** | Calls SpawnOrUpdatePlayer and RemovePlayer |
| **BP_MMOCharacter** | Local player (never managed by this) |

---

## Architecture Notes

### Why a Manager?
- Centralizes remote player tracking
- Prevents duplicate spawns
- Easy cleanup on disconnect
- Single point for debugging

### Map vs Array
- **Map** chosen for O(1) lookup by characterId
- Array would require iteration to find player
- characterId is unique (database primary key)

### Singleton Pattern
- Only one manager needed per level
- Multiple managers would cause duplicate actors
- Place manually in level (not spawned at runtime)

---

**Last Updated**: 2026-02-04
