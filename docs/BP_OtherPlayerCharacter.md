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
- **Character** (same as BP_MMOCharacter for consistency)

---

## Components

| Component | Type | Purpose |
|-----------|------|---------|
| `CapsuleComponent` | Capsule Collision | Character collision and root component |
| `SkeletalMesh` | SkeletalMesh | Visual representation of the player character |
| `CharacterMovement` | CharacterMovementComponent | Handles movement interpolation |

### SkeletalMesh Settings
- **Mesh**: Same as BP_MMOCharacter (for visual consistency)
- **Animation Mode**: Use Animation Blueprint
- **Anim Class**: ABP_unarmed (same as local player)
- **Location**: (0, 0, -90) relative to capsule

---

## Variables

### TargetPosition
| Property | Value |
|----------|-------|
| **Name** | `TargetPosition` |
| **Type** | Vector |
| **Default** | (0, 0, 0) |
| **Purpose** | Last known position from server - where to interpolate toward |

### InterpolationSpeed
| Property | Value |
|----------|-------|
| **Name** | `InterpolationSpeed` |
| **Type** | Float |
| **Default** | 10.0 |
| **Purpose** | Speed of movement smoothing (higher = faster catch-up) |

**Important**: Must be positive! Negative values cause drift away from target.

---

## Event Graph

### Event Tick - Position Interpolation

```
Event Tick
    ↓
Get Actor Location → CurrentLocation
    ↓
Lerp Vector
    A: CurrentLocation
    B: TargetPosition  
    Alpha: Get World Delta Seconds * InterpolationSpeed
    ↓
Set Actor Location (Result)
```

**Logic Explanation:**
1. Get current actor position
2. Linearly interpolate between current and target position
3. DeltaTime multiplication ensures frame-rate independent movement
4. Set new smoothed position

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
| **Input** | Player-controlled | Server-controlled |
| **Movement** | Direct input | Interpolated from server |
| **Possession** | PlayerController | None (unpossessed) |
| **Camera** | SpringArm + Camera | None |
| **Updates** | Input-driven | 30Hz position updates |

---

## Troubleshooting

### Issue: Other player drifts away
**Cause**: InterpolationSpeed is negative
**Fix**: Set InterpolationSpeed to positive value (10.0 recommended)

### Issue: Other player jitters/stutters
**Cause**: InterpolationSpeed too high or network latency
**Fix**: Reduce InterpolationSpeed (try 5.0-8.0)

### Issue: Other player snaps to position
**Cause**: InterpolationSpeed too low
**Fix**: Increase InterpolationSpeed (try 15.0-20.0)

### Issue: Other player not visible
**Cause**: Mesh underground or hidden
**Fix**: 
1. Check SkeletalMesh Location Z = -90
2. Verify mesh asset is assigned
3. Check Visibility = True in Rendering section

---

## Related Blueprints

- **BP_MMOCharacter** - Local player character (possessed)
- **BP_OtherPlayerManager** - Spawns and manages instances of this actor
- **BP_SocketManager** - Receives position updates and forwards to manager

---

**Last Updated**: 2026-02-04
