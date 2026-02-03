# Top-Down Movement System

## Overview

The Top-Down Movement System provides Diablo-style controls with multiple input methods:
- **Click-to-Move**: Left-click anywhere on the ground to navigate
- **WASD Movement**: Traditional directional keyboard controls
- **Combined Control**: WASD cancels click-to-move navigation
- **Smart Rotation**: Character smoothly faces movement direction

## Architecture

### Components

| Component | Purpose | Location |
|-----------|---------|----------|
| `BP_MMOCharacter` | Custom character blueprint with movement logic | `Content/Blueprints/` |
| `IA_ClickToMove` | Enhanced Input Action for left-click | Content/Input/ |
| `IA_Move` | Enhanced Input Action for WASD movement | Content/Input/ |
| `IMC_MMOCharacter` | Input Mapping Context combining all actions | Content/Input/ |
| NavMesh Bounds Volume | Pathfinding mesh for ground navigation | Placed in level |

### Input Actions

#### IA_ClickToMove (Input Action)
- **Type**: Digital (bool)
- **Trigger**: Left Mouse Button
- **Purpose**: Initiates click-to-move navigation
- **Usage**: Player left-clicks on ground, character navigates to that location

#### IA_Move (Input Action)
- **Type**: Axis2D
- **Trigger**: W/A/S/D keys
- **Purpose**: Direct character control
- **Usage**: Cancels any active click-to-move navigation

## Blueprint Implementation

### Click-to-Move Logic

```
Input Action IA_ClickToMove (Triggered)
    ↓
Get Player Controller
    ↓
Get Hit Result Under Cursor by Channel
    ├─ Trace Channel: Visibility
    └─ Out Hit: HitResult
    ↓
Break Hit Result
    ↓
Branch (Blocking Hit?)
    ├─ FALSE: [Do nothing - clicked on void]
    └─ TRUE:
        Simple Move to Location
            ├─ Controller: Get Controller
            └─ Goal: Hit Location
        Set Input Mode Game and UI
```

**Key Nodes:**
- **Get Hit Result Under Cursor by Channel**: Performs line trace from mouse position
- **Break Hit Result**: Extracts hit information (Location, Blocking Hit, etc.)
- **Simple Move to Location**: Uses NavMesh to pathfind to destination

### WASD Movement with Cancel

```
Input Action IA_Move (Triggered)
    ↓
Get Controller
    ↓
Stop Movement  ← Cancels any click-to-move
    ↓
[Continue to Add Movement Input for WASD]
```

**Purpose**: Any WASD input immediately cancels click-to-move navigation

### Character Facing Rotation

```
Event Tick
    ↓
Get Velocity → Vector Length
    ↓
Branch (> 10.0?)  [Only rotate when moving]
    └─ TRUE:
        Get Velocity → Rotation From XVector → TargetRotation
        Get Actor Rotation → RInterp To (TargetRotation, DeltaTime, 5.0) → Set Actor Rotation
```

**Purpose**: Character smoothly turns to face movement direction using interpolation

**Nodes:**
- **Rotation From XVector**: Converts velocity vector to rotation
- **RInterp To**: Smooth rotation interpolation (5.0 = turn speed)
- **Delta Time**: Frame-rate independent timing

## NavMesh Configuration

### Setting Up NavMesh

1. **Place NavMesh Bounds Volume**
   - Place tab → Search "NavMesh" → Drag into level
   - Scale to cover playable area (includes stairs/platforms)

2. **Build Navigation**
   - Build → Build Paths (or press P to visualize)
   - Green areas = walkable surfaces

3. **Troubleshooting**
   - If no green areas: Check floor collision settings
   - If gaps: Increase NavMeshBoundsVolume size
   - Adjust NavMesh settings in Project Settings → Navigation

### NavMesh Settings

| Setting | Recommended Value | Purpose |
|---------|-------------------|---------|
| Agent Radius | 34.0 | Character collision radius |
| Agent Height | 144.0 | Character height clearance |
| Agent Step Height | 35.0 | Maximum step height |
| Max Slope | 44.0 degrees | Maximum walkable slope |

## Class Defaults Configuration

In **BP_MMOCharacter** → Class Defaults:

| Setting | Value | Purpose |
|---------|-------|---------|
| Orient Rotation to Movement | FALSE | Prevents auto-rotation affecting camera |
| Use Controller Desired Rotation | FALSE | Character rotation independent of controller |
| Rotation Rate (Yaw) | 360.0 | Max rotation speed |

## Input Settings

### Enhanced Input Configuration

**IMC_MMOCharacter** (Input Mapping Context):

| Action | Binding | Trigger Type |
|--------|---------|--------------|
| IA_ClickToMove | Left Mouse Button | Pressed |
| IA_Move | W/A/S/D | Axis2D |

## Common Issues & Solutions

### Issue: Click-to-move requires double-click
**Cause**: Input mode changes after first click
**Solution**: Add `Set Input Mode Game and UI` after movement

### Issue: Camera rotates with character
**Cause**: SpringArm inherits rotation from parent
**Solution**: In SpringArm → Transform → Set Inherit Yaw/Pitch/Roll to FALSE

### Issue: WASD doesn't cancel click-to-move
**Cause**: Missing Stop Movement node
**Solution**: Add Stop Movement at start of IA_Move event

### Issue: Character doesn't face movement direction
**Cause**: Missing rotation logic or low velocity threshold
**Solution**: Check Event Tick rotation logic and velocity threshold (try 10.0)

## Performance Considerations

- **Event Tick**: Both rotation systems run every frame - keep logic minimal
- **NavMesh**: Large/complex NavMeshes impact pathfinding performance
- **Input Polling**: Enhanced Input is more efficient than legacy input system

## References

- Related: [Camera System](Camera_System.md)
- Related: [Enhanced Input Setup](Enhanced_Input.md)
- Related: [BP_MMOCharacter Blueprint](Character_Blueprint.md)

---

**Last Updated**: 2026-02-03  
**Version**: 1.0.1  
**Status**: Complete
