# Enhanced Input System

## Overview

The Enhanced Input System (EIS) is UE5's modern input handling framework, replacing the legacy Input system. It provides more flexibility, better performance, and cleaner Blueprint integration for action-based inputs.

## Architecture

### Core Concepts

| Concept | Description | Analogy |
|---------|-------------|---------|
| **Input Action** | Defines an input (e.g., "Jump", "Fire") | "What" can the player do |
| **Input Mapping Context** | Binds actions to keys/buttons | "How" to trigger the action |
| **Modifiers** | Adjust input values (e.g., negate, scale) | Transform the input |
| **Triggers** | Conditions for when input fires (e.g., pressed, held) | "When" to trigger |

## Input Actions Created

### IA_ClickToMove
- **Purpose**: Left-click to move character
- **Type**: Digital (bool)
- **Trigger**: Pressed
- **Binding**: Left Mouse Button

### IA_Move
- **Purpose**: WASD character movement
- **Type**: Axis2D (X: Left/Right, Y: Forward/Backward)
- **Binding**: W/A/S/D keys

### IA_CameraRotate
- **Purpose**: Enable camera rotation mode
- **Type**: Digital (bool)
- **Trigger**: Pressed/Released
- **Binding**: Right Mouse Button

### IA_Look
- **Purpose**: Mouse look for camera rotation
- **Type**: Axis2D (X: horizontal, Y: vertical)
- **Binding**: Mouse X/Y axes

### IA_Zoom
- **Purpose**: Camera zoom in/out
- **Type**: Axis1D
- **Binding**: Mouse Wheel Axis

## Input Mapping Context (IMC_MMOCharacter)

### Action Bindings

| Action | Key/Button | Modifier | Trigger |
|--------|------------|----------|---------|
| IA_ClickToMove | Left Mouse Button | None | Pressed |
| IA_Move | W | Swizzle Axis (Y positive) | Ongoing |
| IA_Move | S | Swizzle Axis + Negate (Y negative) | Ongoing |
| IA_Move | A | Swizzle Axis + Negate (X negative) | Ongoing |
| IA_Move | D | Swizzle Axis (X positive) | Ongoing |
| IA_CameraRotate | Right Mouse Button | None | Pressed/Released |
| IA_Look | Mouse X | None | Ongoing |
| IA_Zoom | Mouse Wheel Axis | None | Ongoing |

### Modifiers Explained

**Swizzle Axis**: Reorders input axes
- For W/S: Converts Y-axis to Forward/Backward
- For A/D: Converts X-axis to Left/Right

**Negate**: Inverts the input value
- Used for S (backward) and A (left) directions

## Blueprint Integration

### Setup in BeginPlay

```
Event BeginPlay
    ↓
Get Player Controller
    ↓
Get Enhanced Input Local Player Subsystem
    ↓
Add Mapping Context (IMC_MMOCharacter, Priority: 1)
    ↓
Set Input Mode Game and UI
```

**Purpose**: Registers the input mappings when character spawns.

### Event-Based Input Handling

Unlike legacy input, Enhanced Input creates automatic events:

```
Input Action IA_ClickToMove (Triggered)
    ↓
[Your logic here]

Input Action IA_CameraRotate (Triggered)
    ↓
Set IsRotatingCamera = TRUE

Input Action IA_CameraRotate (Completed)
    ↓
Set IsRotatingCamera = FALSE
```

**Trigger Types:**
- **Started**: First frame action becomes true
- **Triggered**: Every frame while active
- **Ongoing**: Continuous while active (for axis inputs)
- **Canceled**: Action interrupted
- **Completed**: Action finished

### Axis Value Access

For axis inputs (movement, look, zoom):

```
Input Action IA_Move (Triggered)
    ↓
Get Action Value (Vector2D)
    ↓
Break Vector2D
    ├─ X: Left/Right input
    └─ Y: Forward/Backward input
```

```
Input Action IA_Zoom (Triggered)
    ↓
Get Action Value (Float)
    ├─ +1.0: Scroll down (zoom out)
    └─ -1.0: Scroll up (zoom in)
```

## Creating Input Actions

### Step-by-Step

1. **Content Browser** → Right-click
2. **Input** → **Input Action**
3. Name it (e.g., `IA_MyAction`)
4. Double-click to open
5. Set **Value Type**:
   - **Digital (bool)**: On/off actions (jump, fire)
   - **Axis1D (float)**: Single axis (zoom, throttle)
   - **Axis2D (Vector2D)**: Two axes (move, look)
   - **Axis3D (Vector)**: Three axes (flight controls)

### Value Type Use Cases

| Value Type | Use For | Example |
|------------|---------|---------|
| Digital | Binary actions | Jump, Fire, Interact |
| Axis1D | Single axis control | Zoom, Lean, Throttle |
| Axis2D | Dual axis control | Move, Look, Navigate |
| Axis3D | 3D navigation | Flight, Free camera |

## Creating Input Mapping Context

### Step-by-Step

1. **Content Browser** → Right-click
2. **Input** → **Input Mapping Context**
3. Name it (e.g., `IMC_MyContext`)
4. Double-click to open
5. Click **+ Mappings**
6. Select your Input Action
7. Click **+** to add key binding
8. Select key/axis from dropdown
9. (Optional) Add modifiers
10. Set trigger type

### Priority System

When multiple contexts are active, higher priority wins:

```cpp
// Example: Menu overrides gameplay
AddMappingContext(IMC_Gameplay, 0);  // Base priority
AddMappingContext(IMC_Menu, 1);      // Higher priority (menu open)
```

## Migration from Legacy Input

### Legacy vs Enhanced

| Legacy | Enhanced |
|--------|----------|
| Project Settings → Input | Content assets |
| Hardcoded in INI files | Blueprint-friendly |
| Limited modifiers | Rich modifier system |
| Global mappings | Context-aware |

### Why Migrate?

1. **Performance**: Better runtime performance
2. **Flexibility**: Runtime context switching
3. **Organization**: Self-contained assets
4. **Modifiers**: Built-in input transformations
5. **Debugging**: Better visualization tools

## Troubleshooting

### Issue: Input events not firing
**Check:**
- Is IMC added in BeginPlay?
- Is input mode set correctly?
- Is the action bound in IMC?

### Issue: Axis values are wrong
**Check:**
- Are modifiers applied correctly?
- Is swizzle axis needed?
- Check negation on inverse directions

### Issue: Multiple contexts conflicting
**Check:**
- Priority values (higher = wins)
- Remove unused contexts
- Context order matters

### Issue: Mouse input not working
**Check:**
- Input mode (Game Only vs Game and UI)
- Mouse capture settings
- Widget focus issues

## Best Practices

1. **Naming Convention**: Use IA_/IMC_ prefixes for clarity
2. **Organization**: Keep related inputs in same IMC
3. **Priorities**: Plan context priority levels
4. **Cleanup**: Remove contexts when not needed
5. **Testing**: Test all trigger types (Started vs Triggered)

## References

- Related: [Top-Down Movement System](Top_Down_Movement_System.md)
- Related: [Camera System](Camera_System.md)
- UE5 Docs: Enhanced Input System

---

**Last Updated**: 2026-02-03  
**Version**: 1.0.0  
**Status**: Complete
