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

### bIsMoving
| Property | Value |
|----------|-------|
| **Name** | `bIsMoving` |
| **Type** | Boolean |
| **Default** | false |
| **Purpose** | Optimization flag — skips Tick movement logic when no new position update received (Phase 2.2) |

---

## Event Graph

### Event BeginPlay - Initialize Target Position

```
Event BeginPlay
    ↓
Get Actor Location → Set TargetPosition
```

**Logic**: Initializes TargetPosition to the spawn location so the character stays put until the first real position update arrives from the server.

### Event Tick - CharacterMovement-Based Movement (with bIsMoving Optimization)

```
Event Tick
    ↓
Branch: bIsMoving?
    ├─ FALSE: (skip all movement logic — CharacterMovement decelerates naturally)
    └─ TRUE:
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
            └─ FALSE: Set bIsMoving = false (arrived at target)
```

> **Phase 1.2 Note:** Debug PrintString nodes that printed the distance check result every frame were removed from this Tick.

**Logic Explanation:**
1. **bIsMoving check** (Phase 2.2 optimization): Skip entire movement block when no new position has been received
2. Calculate direction vector from current position to target
3. Zero out Z component to keep movement on ground plane
4. If distance > 10 units, feed normalized direction into CharacterMovement via Add Movement Input
5. If close enough (≤ 10), set `bIsMoving = false` — CharacterMovement decelerates naturally
6. CharacterMovement computes proper velocity and acceleration
7. ABP_unarmed reads velocity/acceleration → ShouldMove = true → walk/run animation plays
8. Orient Rotation to Movement makes the character face travel direction

**bIsMoving is set to `true` by:** `BP_SocketManager.OnPlayerMoved` (or `BP_OtherPlayerManager.SpawnOrUpdatePlayer`) when a new TargetPosition is received from the server.

---

## Usage

### Spawned By
- **BP_OtherPlayerManager** - When `player:moved` event received for new characterId

### Updated By
- **BP_OtherPlayerManager** - Calling `Set TargetPosition` + `Set bIsMoving = true` on each position update

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

## BPI_Damageable Interface Implementation (Phase 3)

Implements `BPI_Damageable` Blueprint Interface for unified damage handling.

### Interface Configuration
- **Class Settings** → Interfaces → Add `BPI_Damageable`

### ReceiveDamageVisual Implementation
```
ReceiveDamageVisual (Event)
    ↓
Get Actor Location → Return Value
    ↓
Find Look at Rotation
    ├─ Start: Actor Location
    └─ Target: AttackerLocation (input pin)
    ↓
Set Actor Rotation
    ├─ New Rotation: Find Look at Rotation Return Value
    └─ Teleport Physics: true
```

### UpdateHealthDisplay Implementation
```
UpdateHealthDisplay (Event)
    ↓
Select (Boolean)
    ├─ Index: InCombat (input pin)
    ├─ False: Hidden
    └─ True: Visible
    ↓
Set Visibility (HealthBarWidget)
    └─ New Visibility: Select Return Value
    ↓
Update Health Bar (existing function)
    ├─ Health: NewHealth (input pin)
    └─ Maxhealth: NewMaxHealth (input pin)
```

### GetHealthInfo Implementation
```
GetHealthInfo (Function)
    ↓
Return Node.HealthWidget: HealthBarWidget (component reference)
```

---

## Related Blueprints

- **BP_MMOCharacter** - Local player character (possessed)
- **BP_OtherPlayerManager** - Spawns and manages instances of this actor
- **BP_SocketManager** - Receives position updates and forwards to manager
- **BPI_Damageable** - Interface for unified damage handling
- **docs/BPI_Damageable.md** - Full interface documentation

---

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| **Interface** | `BPI_Damageable` replaces Cast chains for damage/health updates |
| **Event-Driven** | `ReceiveDamageVisual` and `UpdateHealthDisplay` are events |

---

**Last Updated**: 2026-02-17
**Version**: 1.2
**Status**: Complete — Phase 1.2 PrintStrings removed, Phase 2.2 bIsMoving optimization, Phase 3 BPI_Damageable interface
