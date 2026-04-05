# BP_MMOCharacter

## Overview

`BP_MMOCharacter` is the local player-controlled character in the MMO. It handles player input, camera control, and network position synchronization.

---

## File Location

```
Content/Blueprints/BP_MMOCharacter.uasset
```

---

## Parent Class

`Character` (from Engine)

---

## Purpose

- Local player avatar controlled by the player
- Sends position updates to server at 30Hz
- Displays player name tag above head
- Handles all player input (movement, combat)

---

## Components

| Component | Type | Purpose |
|-----------|------|---------||
| `CapsuleComponent` | CapsuleComponent | Collision and movement base |
| `SkeletalMesh` | SkeletalMeshComponent | Visual character mesh |
| `SpringArm` | SpringArmComponent | Camera boom for top-down view |
| `Camera` | CameraComponent | Player view camera |
| `CharacterMovement` | CharacterMovementComponent | Movement physics |
| `NameTagWidget` | WidgetComponent | Player name display above head |
| `CameraController` | AC_CameraController | Handles camera rotation and zoom (extracted Phase 4) |
| `TargetingSystem` | AC_TargetingSystem | Handles hover detection and target selection (extracted Phase 4) |
| `HUDManager` | AC_HUDManager | Centralized widget management (extracted Phase 5) |

---

## Variables

| Variable | Type | Category | Purpose |
|----------|------|----------|---------||
| `PlayerName` | String | Name Tag | Character name from GameInstance |
| `DeltaSeconds` | Float | Tick | Cached delta time (renamed from `Delta Seconds`) |
| `ClampedLength` | Float | Camera | Clamped arm length value (renamed from `ClampedLenght`) |
| `HoveredTarget` | Actor (Object Ref) | Targeting | Currently hovered actor (renamed, removed trailing space) |
| `InventoryWindowRef` | Widget (Object Ref) | UI | Reference to inventory window widget (renamed from `InventorywindowRef`) |
| `bIsAutoAttacking` | Boolean | Combat | Whether auto-attack loop is active (renamed from `IsAutoAttacking`) |
| `bIsRotatingCamera` | Boolean | Camera | Whether right-click camera rotation is active (renamed from `IsRotatingCamera`) — **now lives in AC_CameraController** |
| `bIsMovingToTarget` | Boolean | Movement | Moving toward a combat target (renamed from `IsMovingToTarget`) |
| `bHasTarget` | Boolean | Targeting | Whether a target is selected (renamed from `HasTarget`) — **now lives in AC_TargetingSystem** |
| `bIsStatWindowOpen` | Boolean | UI | Stat allocation window open state (renamed from `IsStatWindowOpen`) |
| `bIsInventoryOpen` | Boolean | UI | Inventory window open state (renamed from `IsInventoryOpen`) |

> **Note (Phase 1.3 Refactor):** All boolean variables now use the UE5 `b` prefix convention. Several camera/targeting variables have been moved into their respective Actor Components (see below).

---

## Name Tag Widget Setup

### Component Configuration

**Details Panel - User Interface:**
| Property | Value |
|----------|-------|
| Widget Class | `WBP_PlayerNameTag` |
| Space | Screen |
| Draw at Desired Size | ✓ Checked |
| Draw Size | X=200, Y=50 |

**Details Panel - Transform:**
| Property | Value | Description |
|----------|-------|-------------|
| Location X | 0 | Centered on character |
| Location Y | 0 | Centered on character |
| Location Z | 250 | Above character head |
| Rotation | 0, 0, 0 | Default |
| Scale | 1, 1, 1 | Default size |

---

## Event Graph

### Event BeginPlay - Set Name Tag

```
Event BeginPlay
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Get SelectedCharacter → Break FCharacterData
    ↓
Get Name → Set PlayerName (variable)
    ↓
Delay (0.1 seconds)  ← Wait for variable replication
    ↓
Get NameTagWidget (component) → Get User Widget Object
    ↓
Cast to WBP_PlayerNameTag
    ↓
Set Player Name (function)
    ↓
PlayerName variable → NewName input
```

**Purpose**: Retrieves character name from GameInstance and sets it on the name tag widget.

---

## Actor Components (Phase 4 Extraction)

### AC_CameraController
Extracted from BP_MMOCharacter's Event Tick camera logic. Handles all camera rotation and zoom.

| Variable | Type | Default | Purpose |
|----------|------|---------|---------||
| `SpringArmRef` | SpringArmComponent (Ref) | Set in BeginPlay | Reference to SpringArm component |
| `bIsRotatingCamera` | Boolean | false | Right-click rotation active |
| `CurrentYaw` | Float | 0 | Current camera yaw |
| `CurrentPitch` | Float | 0 | Current camera pitch |
| `CurrentRoll` | Float | 0 | Current camera roll |
| `LookInput` | Vector2D | (0,0) | Mouse look input |
| `ZoomSpeed` | Float | 100.0 | Scroll zoom sensitivity |
| `NewArmLength` | Float | 0 | Calculated arm length |
| `ClampedLength` | Float | 0 | Clamped arm length |

| Function | Inputs | Purpose |
|----------|--------|---------||
| `HandleCameraRotation` | DeltaSeconds (Float) | Applies yaw rotation to SpringArm when bIsRotatingCamera is true |
| `HandleZoom` | AxisValue (Float) | Adjusts SpringArm Target Arm Length with clamping |
| `SetLookInput` | Input (Vector2D) | Stores mouse look input for rotation |
| `SetRotating` | bRotating (Boolean) | Toggles bIsRotatingCamera state |

### AC_TargetingSystem
Extracted from BP_MMOCharacter's hover detection logic. Handles cursor traces and target selection.

| Variable | Type | Default | Purpose |
|----------|------|---------|---------||
| `HoveredTarget` | Actor (Object Ref) | None | Currently hovered actor |
| `HoveredEnemy` | BP_EnemyCharacter (Ref) | None | Currently hovered enemy |
| `CurrentTarget` | Actor (Object Ref) | None | Actively selected target |
| `CurrentTargetId` | Integer | 0 | Selected target's character/enemy ID |
| `TargetEnemyId` | Integer | 0 | Selected enemy's ID |
| `bHasTarget` | Boolean | false | Whether a target is selected |
| `bIsAutoAttacking` | Boolean | false | Auto-attack active |
| `AttackRange` | Float | 100.0 | Current attack range |
| `LastCursorScreenPos` | Vector2D | (0,0) | Cached cursor position for optimization |

| Function | Purpose |
|----------|---------||
| `UpdateHoverDetection` | Traces cursor, casts to enemy/player, shows/hides hover indicators |
| `OnTargetSelected` | Event Dispatcher — fires when a new target is selected (TargetActor, TargetId) |

---

## Input Mapping

**Enhanced Input (Default):**
- **W/A/S/D** - Movement (forward/left/backward/right)
- **Left Click** - Interact / Attack
- **Shift** - Sprint (optional)
- **Mouse** - Camera rotation (if enabled)

---

## Event Tick — Component Integration (Phase 4)

```
Event Tick (DeltaSeconds)
    ↓
Sequence
    ├─ Then 0: Get CameraController → HandleCameraRotation(DeltaSeconds)
    ├─ Then 1: [Auto-attack chase logic — calls TargetingSystem for range checking]
    └─ Then 2: Get TargetingSystem → UpdateHoverDetection()
```

**Input Routing (Phase 4):**
- `IA_CameraRotate Started` → `CameraController → SetRotating(true)`
- `IA_CameraRotate Completed` → `CameraController → SetRotating(false)`
- `IA_Look` → `CameraController → SetLookInput(ActionValue as Vector2D)`
- `IA_Zoom` → `CameraController → HandleZoom(AxisValue)`

**BeginPlay Addition:**
- After existing setup: `Get CameraController → Set SpringArmRef` to SpringArm component

---

## Network Position Sync

**Position updates sent via BP_SocketManager:**
- Event Tick checks `UpdateInterval` (0.033s = 30Hz)
- When interval elapsed: Get Actor Location → Emit `player:position`
- Format: `{characterId, x, y, z}`

**Not handled in BP_MMOCharacter directly** - BP_SocketManager manages all Socket.io communication.

---

## Comparison with BP_OtherPlayerCharacter

| Feature | BP_MMOCharacter (Local) | BP_OtherPlayerCharacter (Remote) |
|---------|------------------------|----------------------------------|
| Input | Yes (WASD / Click) | No (server-driven) |
| Camera | Yes (SpringArm) | No |
| Position Source | Player input | Server broadcasts |
| Network | Sends position (30Hz) | Receives position |
| Interpolation | No (direct control) | Yes (smooth follow) |
| Name Tag | Yes (WBP_PlayerNameTag) | Yes (WBP_PlayerNameTag) |

---

## BPI_Damageable Interface Implementation (Phase 3)

Implements `BPI_Damageable` Blueprint Interface as a stub (no-op). Local player health is displayed on WBP_GameHud, not on a world-space widget.

### Interface Configuration
- **Class Settings** → Interfaces → Add `BPI_Damageable`

### ReceiveDamageVisual Implementation
```
ReceiveDamageVisual (Event)
    ↓
    (Empty - no-op)
```
**Note:** Local player rotation is handled by the player controller, not needed here.

### UpdateHealthDisplay Implementation
```
UpdateHealthDisplay (Event)
    ↓
    (Empty - no-op)
```
**Note:** Local player health is updated via `combat:health_update` event → WBP_GameHud, not through world-space widgets.

### GetHealthInfo Implementation
```
GetHealthInfo (Function)
    ↓
Return Node.HealthWidget: (returns None)
```
**Note:** Local player has no HealthBarWidget component - health is displayed on the HUD.

---

## Related Files

| File | Purpose |
|------|---------||
| `BP_OtherPlayerCharacter` | Remote player representation |
| `BP_SocketManager` | Handles position emit/receive |
| `WBP_PlayerNameTag` | Name display widget |
| `MMOGameInstance` | Stores SelectedCharacter data |
| `ABP_unarmed` | Animation Blueprint (shared) |
| `AC_CameraController` | Camera rotation/zoom component (Phase 4) |
| `AC_TargetingSystem` | Hover detection/target selection component (Phase 4) |
| `AC_HUDManager` | Widget management component (Phase 5) |
| `Camera_System.md` | Camera system documentation |
| `docs/BPI_Damageable.md` | Interface for unified damage handling |
| `docs/AC_HUDManager.md` | Centralized HUD management |

---

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| **Interface** | `BPI_Damageable` implemented as stub to satisfy contract |
| **Event-Driven** | Local player health updates via `combat:health_update` event |

---

## Troubleshooting

### Issue: Name tag not visible

**Check:**
1. NameTagWidget component exists
2. Widget Class is WBP_PlayerNameTag
3. Space is set to Screen
4. Delay in BeginPlay is long enough (try 0.2s)

### Issue: Name shows "Unknown" or empty

**Check:**
1. PlayerName variable set correctly
2. GameInstance.SelectedCharacter.Name has value
3. Cast to MMOGameInstance succeeds

### Issue: Position not syncing to server

**Check:**
1. BP_SocketManager exists in level
2. IsConnected is true
3. CharacterId is set from GameInstance

---

## Performance Notes

- Local player uses full CharacterMovement physics
- No interpolation (direct player control)
- Camera attached via SpringArm (smooth follow optional)

---

**Last Updated**: 2026-02-17
**Version**: 2.2
**Status**: Complete — Phase 1.3 variable renames + Phase 4 component extraction + Phase 3 BPI_Damageable interface + Phase 5 AC_HUDManager
