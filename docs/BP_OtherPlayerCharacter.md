# BP_OtherPlayerCharacter

## Overview
Blueprint representing remote players in the multiplayer world. This actor is spawned by `BP_OtherPlayerManager` when other players connect and is updated via Socket.io position broadcasts.

---

## File Location
```
Content/Blueprints/BP_OtherPlayerCharacter.uasset
```

---

## Parent Class
- **Character** (reparented from Actor to Character for CharacterMovement support)

---

## Components

| Component | Type | Purpose |
|-----------|------|----------|
| `CapsuleComponent` | Capsule Collision | Character collision and root component |
| `Mesh` | SkeletalMeshComponent | Visual representation (inherited from Character class) |
| `CharacterMovement` | CharacterMovementComponent | Drives movement with proper velocity/acceleration for animations |

### Mesh Settings
- **Skeletal Mesh Asset**: Same as BP_MMOCharacter (for visual consistency)
- **Animation Mode**: Use Animation Blueprint
- **Anim Class**: ABP_unarmed (same as local player)
- **Location**: (0, 0, -90) relative to capsule

### CharacterMovement Settings
| Setting | Value | Section |
|---------|-------|---------|
| **Max Walk Speed** | `600` (matches local player) | Character Movement: Walking |
| **Max Acceleration** | `2048` | Character Movement: Walking |
| **Orient Rotation to Movement** | ✓ Checked | Rotation Settings |

### Class Defaults
| Setting | Value |
|---------|-------|
| **Auto Possess AI** | Spawned |
| **AI Controller Class** | AIController |
| **Use Controller Rotation Yaw** | ✗ Unchecked |

---

## Variables

### TargetPosition
| Property | Value |
|----------|-------|
| **Name** | `TargetPosition` |
| **Type** | Vector |
| **Default** | Initialized to spawn location in BeginPlay |
| **Purpose** | Last known position from server - where to move toward |

---

## Event Graph

### Event BeginPlay - Initialize Target Position

```
Event BeginPlay
    ↓
Get Actor Location → Set TargetPosition
```

**Logic**: Initializes TargetPosition to the spawn location so the character stays put until the first real position update arrives from the server.

### Event Tick - CharacterMovement-Based Movement

```
Event Tick
    ↓
Get Actor Location → CurrentPos
    ↓
TargetPosition - CurrentPos → Direction
    ↓
Break Vector → Make Vector(X, Y, 0.0) → FlatDirection (Z zeroed for ground plane)
    ↓
Vector Length(FlatDirection) → Distance
    ↓
Branch: Distance > 10.0?
    ├─ TRUE:  Normalize(FlatDirection) → Add Movement Input (WorldDirection, Scale=1.0)
    └─ FALSE: (nothing - CharacterMovement decelerates naturally)
```

**Logic Explanation:**
1. Calculate direction vector from current position to target
2. Zero out Z component to keep movement on ground plane
3. If distance > 10 units, feed normalized direction into CharacterMovement via Add Movement Input
4. If close enough, stop adding input — CharacterMovement decelerates naturally
5. CharacterMovement computes proper velocity and acceleration
6. ABP_unarmed reads velocity/acceleration → ShouldMove = true → walk/run animation plays
7. Orient Rotation to Movement makes the character face travel direction

---

## Usage

### Spawned By
- **BP_OtherPlayerManager** - When `player:moved` event received for new characterId

### Updated By
- **BP_OtherPlayerManager** - Calling `Set TargetPosition` on each position update

### Destroyed By
- **BP_OtherPlayerManager** - When `player:left` event received

---

## Differences from BP_MMOCharacter

| Feature | BP_MMOCharacter | BP_OtherPlayerCharacter |
|---------|-----------------|-------------------------|
| **Input** | Player-controlled (keyboard/mouse) | Server-controlled (Add Movement Input) |
| **Movement** | Direct input via CharacterMovement | Add Movement Input toward TargetPosition |
| **Possession** | PlayerController | AIController (Auto Possess AI: Spawned) |
| **Camera** | SpringArm + Camera | None |
| **Updates** | Input-driven | 30Hz position updates from server |
| **Animations** | Velocity-driven via CharacterMovement | Same — CharacterMovement drives ABP_unarmed |

---

## Troubleshooting

### Issue: Remote player slides without animating
**Cause**: CharacterMovement not driving movement (using SetActorLocation instead)
**Fix**: Use Add Movement Input in Event Tick so CharacterMovement computes velocity/acceleration for ABP_unarmed

### Issue: Remote player not moving at all
**Cause**: Missing AI Controller — CharacterMovement requires a controller to process input
**Fix**: Set Auto Possess AI = "Spawned" and AI Controller Class = "AIController" in Class Defaults

### Issue: Remote player walks from origin on spawn
**Cause**: SpawnActor transform not using the received (x,y,z) coordinates, or TargetPosition not initialized
**Fix**:
1. In BP_OtherPlayerManager, wire SpawnActor's Spawn Transform Location to the incoming (x,y,z)
2. In BP_OtherPlayerCharacter BeginPlay, set TargetPosition = GetActorLocation()

### Issue: Other player not visible
**Cause**: Mesh not assigned after reparenting to Character class
**Fix**: 
1. Select Mesh component (inherited from Character)
2. Set Skeletal Mesh Asset to match BP_MMOCharacter
3. Set Anim Class to ABP_unarmed
4. Check Mesh Location Z = -90
5. Delete any old manually-added mesh components

---

## Related Blueprints

- **BP_MMOCharacter** - Local player character (possessed)
- **BP_OtherPlayerManager** - Spawns and manages instances of this actor
- **BP_SocketManager** - Receives position updates and forwards to manager

---

**Last Updated**: 2026-02-05
