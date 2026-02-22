# BP_MMOCharacter

**Path**: `/Game/SabriMMO/Blueprints/BP_MMOCharacter`  
**Parent Class**: `BP_ThirdPersonCharacter_C` (which inherits `ASabriMMOCharacter` C++)  
**Purpose**: The local player character. Handles top-down click-to-move, WASD keyboard movement, camera rotation/zoom, click-to-attack targeting, and input actions. The player's avatar in the game world.

## Components

| Component | Type | Purpose |
|-----------|------|---------|
| `SpringArm` | SpringArmComponent | Camera boom for RO-style top-down view (Pitch: -55°, Arm Length: 1200, Collision Test: disabled) |
| `Camera` | CameraComponent | Follow camera attached to SpringArm |
| `NameTagWidget` | WidgetComponent | World-space WBP_PlayerNameTag above character |
| `CameraController` | AC_CameraController_C | Actor component handling camera rotation/zoom |
| `TargetingSystem` | AC_TargetingSystem_C | Actor component handling hover detection and range checks |
| `HUDManager` | AC_HUDManager_C | Actor component managing all HUD widgets |

## Interfaces Implemented

| Interface | Events Used |
|-----------|-------------|
| `BPI_Damageable` | `ReceiveDamageVisual`, `UpdateHealthDisplay` |

## RO-Style Camera Configuration

The camera system is configured for Ragnarok Online-style fixed isometric view:

### SpringArm Component Settings
- **Pitch**: -55° (steeper, more top-down angle than default -45°)
- **Target Arm Length**: 1200 (more distant, bird's-eye perspective)
- **bDo Collision Test**: false (disables collision checking for consistent camera positioning)
- **Camera Rotation**: Only rotates on right-click-drag (yaw rotation only)
- **Inherit Pitch**: false (maintains fixed top-down angle)

### Camera Behavior
- Fixed behind-and-above perspective characteristic of RO
- No dynamic pitch adjustment - maintains constant -55° angle
- Longer arm length provides better battlefield overview
- Collision test disabled prevents camera snapping when near walls/objects
- Right-click drag rotates camera around character (yaw only)
- Mouse wheel zoom handled by AC_CameraController component

### Comparison to Previous Settings
| Setting | Previous | RO-Style | Purpose |
|---------|----------|----------|---------|
| Pitch | -45° | -55° | Steeper top-down angle |
| Arm Length | 800 | 1200 | More distant view |
| Collision Test | true | false | Consistent positioning |

## Variables (21)

### Target & Combat

| Variable | Type | Purpose |
|----------|------|---------|
| `CurrentTargetOtherPlayerRef` | object (BP_OtherPlayerCharacter) | Currently targeted remote player |
| `CurrentTargeOtherPlayerId` | int | ID of targeted remote player |
| `TargetEnemyRef` | object (BP_EnemyCharacter) | Currently targeted enemy |
| `TargetEnemyId` | int | ID of targeted enemy |
| `CurrentTargetActor` | object (Actor) | Generic ref to current target (player or enemy) |
| `bIsAutoAttacking` | bool | Whether auto-attack is active |
| `bIsInAutoAttackRangeOfPlayer` | bool | Whether in range of targeted player |
| `bIsInAutoAttackRangeOfEnemy` | bool | Whether in range of targeted enemy |

### Movement & Camera

| Variable | Type | Purpose |
|----------|------|---------|
| `HitResult` | struct (HitResult) | Stored hit result from cursor trace |
| `OldCameraComponentRef` | object | Legacy camera reference (unused, from template) |
| `bIsRotatingCamera` | bool | Whether right-click camera rotation is active |
| `LookInput` | struct (Vector2D) | Current mouse look input |
| `DeltaSeconds` | real | Cached delta time |
| `TargetLocation` | struct (Vector) | Click-to-move target position |
| `MyLocation` | struct (Vector) | Cached actor location |
| `MovementComponentRef` | object | CharacterMovement reference |
| `EnhancedInputComponent` | object | Enhanced Input reference |

### Manager References

| Variable | Type | Purpose |
|----------|------|---------|
| `SocketManagerRef` | object (BP_SocketManager) | Cached reference |
| `EnemyManagerRef` | object (BP_EnemyManager) | Cached reference |
| `OtherPlayerManagerRef` | object (BP_OtherPlayerManager) | Cached reference |
| `HoveredTargetOtherPlayerRef` | object | Currently hovered remote player |

## Functions

| Function | Nodes | Purpose |
|----------|-------|---------|
| `PlayAttackAnimation` | 11 | Plays attack montage on local character |

## Event Graph (231 nodes)

### Event BeginPlay

```
Event BeginPlay
    ↓
[Sequence — "Begin Play"]
    │
    ├─ Pin 0: Enhanced Input Setup
    │   Get Player Controller → Enhanced Input Local Player Subsystem
    │   → Add Mapping Context (IMC_MMOCharacter)
    │
    ├─ Pin 1: Set Manager References
    │   Get Actor Of Class(BP_SocketManager) → Set SocketManagerRef
    │   Get Actor Of Class(BP_EnemyManager) → Set EnemyManagerRef
    │   Get Actor Of Class(BP_OtherPlayerManager) → Set OtherPlayerManagerRef
    │
    ├─ Pin 2: Set Camera Controller Refs
    │   Get CameraController → Set SpringArmRef (from SpringArm component)
    │
    ├─ Pin 3: Set Player Name Tag
    │   Get Game Instance → Cast To MMOGameInstance
    │   → Get Selected Character → Break Character Data → Name
    │   ↓ Delay (for widget to initialize)
    │   Get NameTagWidget → Get User Widget Object → Cast To WBP_PlayerNameTag
    │   → Set Player Name (character name as Text)
    │
    ├─ Pin 4: Input Mode
    │   Get Player Controller → Set Input Mode Game And UI
    │
    └─ Pin 5: Local UI Widgets (delegated to HUDManager via AC_HUDManager)
```

### Event Tick — Core Game Loop

```
Event Tick
    ↓
Set DeltaSeconds
    ↓
[Sequence — "EVENT TICK"]
    │
    ├─ Pin 0: Camera and Movement
    │   ├─ CameraController → Handle Camera Rotation (DeltaSeconds)
    │   └─ TargetingSystem → Update Hover Detection
    │
    ├─ Pin 1: Auto-Attack Range Check
    │   Branch: bIsAutoAttacking?
    │   ├─ TRUE:
    │   │   Set MyLocation = GetActorLocation()
    │   │   ↓
    │   │   [Check Player Target]
    │   │   Branch: CurrentTargetOtherPlayerRef != null?
    │   │   ├─ YES: TargetingSystem.CheckIfInAttackRangeOfTargetPlayer(MyLocation, CurrentTargetOtherPlayerRef)
    │   │   │   Branch: bIsInAutoAttackRangeOfPlayer?
    │   │   │   ├─ TRUE: Stop Movement Immediately
    │   │   │   └─ FALSE: Simple Move To Location (TargetLocation)
    │   │   └─ NO: (skip)
    │   │   ↓
    │   │   [Check Enemy Target]  
    │   │   Branch: TargetEnemyRef != null?
    │   │   ├─ YES: TargetingSystem.CheckIfInAttackRangeOfEnemy(MyLocation, TargetEnemyRef)
    │   │   │   Branch: bIsInAutoAttackRangeOfEnemy?
    │   │   │   ├─ TRUE: Stop Movement Immediately
    │   │   │   └─ FALSE: Simple Move To Location (TargetLocation)
    │   │   └─ NO: (skip)
    │   └─ FALSE: (no auto-attack active)
    │
    └─ Pin 2: HoverOverIndicator Visibility
        (Periodically hide indicators on non-targeted actors)
```

### Input Actions

#### IA_ClickToMove (Left Click — Ground Movement)

```
EnhancedInputAction IA_ClickToMove [Triggered]
    ↓
Get Player Controller → Get Hit Result Under Cursor by Channel
    → Break Hit Result → Set HitResult
    ↓
Branch: Hit successful?
    ├─ TRUE: Simple Move to Location (hit Location)
    │   (Uses NavMesh pathfinding via AIController)
    └─ FALSE: (skip)
```

#### IA_Move (WASD — Keyboard Movement)

```
EnhancedInputAction IA_Move [Triggered]
    ↓
Get Controller → Stop Movement (cancels click-to-move)
    (WASD then handled by parent C++ class ASabriMMOCharacter::Move)
```

#### IA_Attack (Left Click — Attack Target)

```
EnhancedInputAction IA_Attack [Triggered]
    ↓
Get Player Controller → Get Hit Result Under Cursor by Channel
    → Break Hit Result
    ↓
[Step 1: Check if we hit another player]
Cast hit actor To BP_OtherPlayerCharacter
    ├─ SUCCESS → Attack Player:
    │   Set CurrentTargetOtherPlayerRef = cast result
    │   Set CurrentTargeOtherPlayerId = target.CharacterId
    │   Set CurrentTargetActor = target
    │   Set bIsAutoAttacking = true
    │   Set TargetEnemyRef = null (clear enemy target)
    │   SocketManagerRef → Emit Attack(CharacterId, Self)
    │   ↓
    │   Branch: bIsInAutoAttackRangeOfPlayer?
    │   ├─ TRUE: Stop Movement Immediately
    │   └─ FALSE: Simple Move To Location (target location)
    │
    └─ FAIL → [Step 2: Check if we hit an enemy]
        Cast hit actor To BP_EnemyCharacter
        ├─ SUCCESS → Attack Enemy:
        │   Set TargetEnemyRef = cast result
        │   Set TargetEnemyId = enemy.EnemyId
        │   Set CurrentTargetActor = enemy
        │   Set bIsAutoAttacking = true
        │   Set CurrentTargetOtherPlayerRef = null (clear player target)
        │   SocketManagerRef → Emit Attack(EnemyId, Self)
        │   ↓
        │   Branch: bIsInAutoAttackRangeOfEnemy?
        │   ├─ TRUE: Stop Movement Immediately
        │   └─ FALSE: Simple Move To Location (enemy location)
        │
        └─ FAIL → [Step 3: Check if we clicked an NPC Shop]
            Cast hit actor To BP_NPCShop
            ├─ SUCCESS → Open Shop:
            │   Get ShopId from BP_NPCShop
            │   NULL GUARD: IsValid(SocketManagerRef) → FALSE: Return
            │   SocketManagerRef → EmitShop(ShopId)
            │   (No movement, no attack state change)
            │
            └─ FAIL → [Step 4: Clicked on nothing]
                Branch: bIsAutoAttacking?
                ├─ TRUE: Stop Attack
                │   SocketManagerRef → Emit Stop Attack
                │   Set bIsAutoAttacking = false
                │   Clear all target refs
                └─ FALSE: (do nothing — regular click-to-move handles it)
```

#### IA_CameraRotate (Right Click Hold)

```
EnhancedInputAction IA_CameraRotate [Started]
    → CameraController → Set bIsRotatingCamera = true

EnhancedInputAction IA_CameraRotate [Completed]
    → CameraController → Set bIsRotatingCamera = false
```

#### IA_Look (Mouse Move during rotation)

```
EnhancedInputAction IA_Look [Triggered]
    → CameraController → Set Look Input (Vector2D value)
```

#### IA_Zoom (Mouse Wheel)

```
EnhancedInputAction IA_Zoom [Triggered]
    → CameraController → Handle Zoom (axis value)
```

#### IA_ToggleInventory

```
EnhancedInputAction IA_ToggleInventory [Started]
    → HUDManager → Toggle Inventory
```

#### IA_Hotbar_1 through IA_Hotbar_9 (Hotbar Key Slots)

Nine separate `EnhancedInputAction` nodes — one per hotbar key (keys 1–9). Each maps to `AC_HUDManager.UseHotbarSlot` with a hardcoded slot index.

```
EnhancedInputAction IA_Hotbar_1 [Started]
    → HUDManager (AC_HUDManager) → Use Hotbar Slot (SlotIndex = 0)

EnhancedInputAction IA_Hotbar_2 [Started]
    → HUDManager (AC_HUDManager) → Use Hotbar Slot (SlotIndex = 1)

... (pattern continues for IA_Hotbar_3 through IA_Hotbar_9)

EnhancedInputAction IA_Hotbar_9 [Started]
    → HUDManager (AC_HUDManager) → Use Hotbar Slot (SlotIndex = 8)
```

**Comment box in graph**: `"Hot Bar 1 - 9"` groups all 9 hotbar input action nodes together.

**Input Assets**: `IA_Hotbar_1` through `IA_Hotbar_9` are Input Action assets located at `/Game/SabriMMO/Input/`. All 9 are registered in `IMC_MMOCharacter` with keys `1` through `9` respectively.

### BPI_Damageable Interface Events

```
Event ReceiveDamageVisual (from BPI_Damageable)
    → (Currently connected but logic handled via BP_SocketManager.OnCombatDamage)

Event UpdateHealthDisplay (from BPI_Damageable)
    → (Currently connected but logic handled via BP_SocketManager.OnHealthUpdate)
```

## Connections to Other Blueprints

| Blueprint | Stored In | Functions Called |
|-----------|-----------|-----------------|
| `BP_SocketManager` | SocketManagerRef | EmitAttack, EmitStopAttack, EmitShop |
| `BP_OtherPlayerManager` | OtherPlayerManagerRef | (used for reference only) |
| `BP_EnemyManager` | EnemyManagerRef | (used for reference only) |
| `AC_CameraController` | CameraController component | HandleCameraRotation, HandleZoom, SetLookInput, SetRotating |
| `AC_TargetingSystem` | TargetingSystem component | UpdateHoverDetection, CheckIfInAttackRangeOfTargetPlayer, CheckIfInAttackRangeOfEnemy |
| `AC_HUDManager` | HUDManager component | ToggleInventory, ToggleStats, UseHotbarSlot (slot 0–8) |
| `WBP_PlayerNameTag` | NameTagWidget component | SetPlayerName |
| `UMMOGameInstance` (C++) | Via GetGameInstance | GetSelectedCharacter |
| `BPI_Damageable` | Implements interface | ReceiveDamageVisual, UpdateHealthDisplay |

## Design Patterns

| Pattern | Application |
|---------|-------------|
| **Component-Based** | Camera (AC_CameraController), Targeting (AC_TargetingSystem), HUD (AC_HUDManager) are separate components |
| **Interface** | Implements BPI_Damageable for unified damage handling |
| **Event-Driven** | Enhanced Input actions trigger game logic |
| **Emit Abstraction** | All socket emissions go through `BP_SocketManager` named functions (`EmitShop`, `EmitAttack`) — never SocketIO directly |
| **SRP** | Each component handles one responsibility |

---

**Last Updated**: 2026-02-22 (Feature #20 NPC Shop: IA_Attack Step 3 now casts to BP_NPCShop → calls SocketManagerRef.EmitShop(ShopId); EmitShop added to Connections table; Emit Abstraction design pattern added. ⚠️ unrealMCP scan unavailable — verified from user confirmation)

**Previous**: 2026-02-20 (Feature #19 Hotbar: added IA_Hotbar_1–9 input actions section, updated event graph node count 203→231, updated AC_HUDManager called functions; verified via unrealMCP)  
**Source**: Read via unrealMCP `read_blueprint_content`
