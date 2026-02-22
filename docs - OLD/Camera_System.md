# Camera System

## Overview

The Camera System provides a fully independent top-down camera with smooth following, rotation, and zoom capabilities. Designed for Diablo-style gameplay where the camera remains fixed relative to the world while the player moves.

## Features

- **Spring Arm Architecture**: Camera boom for adjustable distance and angle
- **Right-Click Rotation**: Independent camera rotation around player
- **Mouse Scroll Zoom**: Adjustable zoom with min/max limits
- **Fixed Camera**: Camera stays stationary when player moves/turns
- **Smooth Following**: Camera lag for polished movement

## Components

### BP_MMOCharacter Components

| Component | Type | Purpose | Settings |
|-----------|------|---------|----------|
| SpringArm | SpringArmComponent | Camera boom/anchor | Target Arm Length: 800 |
| Camera | CameraComponent | Actual camera view | Attached to SpringArm |

### SpringArm Configuration

**Transform Settings:**
- **Target Arm Length**: 800.0 (default distance)
- **Socket Offset**: Z = 100.0 (slightly above character)
- **Relative Rotation**: Pitch = -45.0 (look down angle)
- **Use Pawn Control Rotation**: FALSE (independent of character)

**Lag Settings:**
- **Enable Camera Lag**: TRUE (smooth following)
- **Camera Lag Speed**: 5.0 (responsiveness, higher = snappier)

**Inheritance Settings (CRITICAL):**
- **Inherit Pitch**: FALSE
- **Inherit Yaw**: FALSE
- **Inherit Roll**: FALSE

These prevent the camera from rotating when the character turns.

## Input Actions

### IA_CameraRotate (Input Action)
- **Type**: Digital (bool)
- **Binding**: Right Mouse Button
- **Purpose**: Enable camera rotation mode
- **States**: Triggered (hold) / Completed (release)

### IA_Look (Input Action)
- **Type**: Axis2D
- **Binding**: Mouse X/Y
- **Purpose**: Camera rotation input while holding right-click

### IA_Zoom (Input Action)
- **Type**: Axis 1D
- **Binding**: Mouse Wheel Axis
- **Purpose**: Zoom in/out camera distance

## Blueprint Implementation

> **Phase 4 Note:** Camera rotation and zoom logic has been extracted from BP_MMOCharacter's Event Tick into the **AC_CameraController** Actor Component. The input routing now goes through the component's functions instead of inline Tick logic.

### Camera Rotation (Right-Click) — via AC_CameraController

```
IA_CameraRotate (Started)
    ↓
Get CameraController → SetRotating(true)

IA_CameraRotate (Completed)
    ↓
Get CameraController → SetRotating(false)

IA_Look (Triggered)
    ↓
Get CameraController → SetLookInput(ActionValue as Vector2D)

BP_MMOCharacter Event Tick → Sequence Then 0:
    ↓
Get CameraController → HandleCameraRotation(DeltaSeconds)
```

**Inside AC_CameraController.HandleCameraRotation:**
```
Branch (bIsRotatingCamera?)
    └─ TRUE:
        Get SpringArmRef
            ↓
        Get Relative Rotation → Break Rotator
            ├─ Pitch: (keep current)
            └─ Yaw: CurrentYaw + (LookInput.X * 2.0)
                ↓
        Make Rotator (CurrentRoll, CurrentPitch, NewYaw)
            ↓
        Set Relative Rotation (SpringArmRef)
```

**Key Points:**
- Only Yaw changes (horizontal rotation)
- Pitch stays constant (maintains top-down angle)
- RotationSpeed multiplier (2.0) controls sensitivity
- `bIsRotatingCamera` renamed from `IsRotatingCamera` (Phase 1.3 UE5 convention)

### Camera Zoom (Mouse Scroll) — via AC_CameraController

```
IA_Zoom (Triggered)
    ↓
Get CameraController → HandleZoom(AxisValue)
```

**Inside AC_CameraController.HandleZoom:**
```
Get SpringArmRef → Get Target Arm Length
    ↓
Current + (AxisValue * ZoomSpeed)
    ↓
Clamp (Min: 200.0, Max: 1500.0)
    ↓
Set Target Arm Length (SpringArmRef)
```

**Variables:**
- `MinZoom`: 200.0 (closest zoom)
- `MaxZoom`: 1500.0 (farthest zoom)
- `ZoomSpeed`: 100.0 (scroll sensitivity)

**Zoom Behavior:**
- Scroll up (-1): Zoom in (decrease arm length)
- Scroll down (+1): Zoom out (increase arm length)
- Camera moves along the angled SpringArm direction

## Class Defaults Configuration

### Character Settings

In **BP_MMOCharacter** → Class Defaults → Character Movement:

| Setting | Value | Purpose |
|---------|-------|---------|
| Orient Rotation to Movement | FALSE | Character rotates independently of camera |
| Use Controller Desired Rotation | FALSE | Camera doesn't follow character rotation |

### SpringArm Inheritance (Critical)

In **SpringArm** → Transform:

| Setting | Value | Purpose |
|---------|-------|---------|
| Inherit Pitch | FALSE | Camera pitch independent of character |
| Inherit Yaw | FALSE | Camera yaw independent of character |
| Inherit Roll | FALSE | Camera roll independent of character |

These settings ensure the camera stays fixed in the world while the character moves and turns beneath it.

## Input Mapping Context (IMC_MMOCharacter)

| Action | Binding | Trigger | Purpose |
|--------|---------|---------|---------|
| IA_CameraRotate | Right Mouse Button | Press/Release | Enable rotation mode |
| IA_Look | Mouse X | Axis | Horizontal rotation |
| IA_Zoom | Mouse Wheel Axis | Axis | Zoom in/out |

## Camera Behaviors

### Default State
- Camera is positioned at fixed angle (-45° pitch)
- Distance: 800 units from character
- Character moves, camera stays stationary

### Right-Click Rotation
- Hold right-click to enter rotation mode
- Mouse X rotates camera around character (yaw only)
- Pitch stays constant (no up/down tilt)
- Release right-click to lock camera angle

### Scroll Zoom
- Scroll up: Camera moves closer (min: 200)
- Scroll down: Camera moves farther (max: 1500)
- Movement follows SpringArm angle
- Instant response (no smoothing)

## Common Issues & Solutions

### Issue: Camera rotates with character movement
**Cause**: SpringArm inheritance settings not disabled
**Solution**: Set Inherit Pitch/Yaw/Roll all to FALSE

### Issue: Camera rotation only works once
**Cause**: Input mode changed by click-to-move
**Solution**: Add `Set Input Mode Game and UI` to Event BeginPlay

### Issue: Camera zooms vertically instead of along angle
**Cause**: Expected behavior - this is correct for SpringArm
**Solution**: Adjust Socket Offset if you want different zoom behavior

### Issue: Right-click rotation not working
**Cause**: Missing bIsRotatingCamera boolean variable in AC_CameraController
**Solution**: Ensure AC_CameraController component is added to BP_MMOCharacter and SpringArmRef is set in BeginPlay

### Issue: Camera lags too much behind character
**Cause**: Camera Lag Speed too low
**Solution**: Increase Camera Lag Speed (e.g., 10.0 for snappier follow)

### Issue: Camera uses wrong pawn on first spawn
**Cause**: Level Blueprint has multiple Spawn Actor nodes with different classes
**Solution**: Change ALL Spawn Actor nodes to use BP_MMOCharacter (not BP_ThirdPersonCharacter)

See [Bug Fix Notes](Bug_Fix_Notes.md) for detailed explanation.

## Performance Considerations

- **Event Tick**: Camera rotation runs every frame while right-click held (now via AC_CameraController.HandleCameraRotation)
- **Lag System**: Camera Lag uses interpolation - minimal performance cost
- **Zoom**: Instant value change - no performance impact
- **Component overhead**: AC_CameraController has negligible overhead — no component Tick needed since it's called from owning actor's Tick

## Recommended Values

### Default Camera Setup
- **Target Arm Length**: 800 (good starting point)
- **Pitch**: -45° (classic isometric angle)
- **Zoom Range**: 200-1500 (good flexibility)
- **Lag Speed**: 5.0 (smooth but responsive)

### Rotation Sensitivity
- **Mouse Multiplier**: 2.0 (comfortable rotation speed)
- **Clamp Yaw**: None (full 360° rotation allowed)
- **Clamp Pitch**: Not used (fixed at -45°)

## Integration with Movement System

The Camera System works independently from the [Top-Down Movement System](Top_Down_Movement_System.md):

- Movement uses NavMesh pathfinding
- Camera stays at fixed position/orientation
- Character rotates to face movement direction
- Camera only moves when player manually rotates/zooms

## References

- Related: [Top-Down Movement System](Top_Down_Movement_System.md)
- Related: [Enhanced Input Setup](Enhanced_Input.md)
- Related: [BP_MMOCharacter Blueprint](BP_MMOCharacter.md)
- Related: [AC_CameraController](BP_MMOCharacter.md#ac_cameracontroller) (Phase 4 component docs)

---

**Last Updated**: 2026-02-13  
**Version**: 2.0.0  
**Status**: Refactored — Camera logic extracted to AC_CameraController (Phase 4)
