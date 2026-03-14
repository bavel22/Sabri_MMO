# 01 — Local Player Input & Movement: Blueprint-to-C++ Migration Plan

**Status**: PLANNED
**Priority**: High — this is the first migration in the Blueprint-to-C++ series
**Affected Systems**: Player input, click-to-move, WASD movement, camera control, auto-attack, NPC interaction
**Estimated Scope**: 2 new subsystems (~800-1000 lines total), minor modifications to 2 existing files

---

## Table of Contents

1. [RO Classic Movement Behavior](#1-ro-classic-movement-behavior-pre-renewal-reference)
2. [Current Blueprint Implementation](#2-current-blueprint-implementation-what-gets-replaced)
3. [New C++ Architecture](#3-new-c-architecture)
4. [Detailed Class Specifications](#4-detailed-class-specifications)
5. [Click Routing Logic](#5-click-routing-logic)
6. [Walk-to-Attack System](#6-walk-to-attack-system)
7. [Camera Subsystem](#7-camera-subsystem)
8. [Input System Design](#8-input-system-design)
9. [NavMesh Pathfinding](#9-navmesh-pathfinding)
10. [Socket Events Handled](#10-socket-events-handled)
11. [Integration with Other Subsystems](#11-integration-with-other-subsystems)
12. [Coexistence Strategy](#12-coexistence-strategy-during-migration)
13. [Files to Create](#13-files-to-create)
14. [Files to Modify](#14-files-to-modify)
15. [Testing Checklist](#15-testing-checklist)
16. [Risk Analysis](#16-risk-analysis)

---

## 1. RO Classic Movement Behavior (Pre-Renewal Reference)

### Click-to-Move
- Left-click on ground causes the character to pathfind and walk to that point.
- Original RO uses tile-based A* pathfinding; our UE5 implementation uses NavMesh via `UAIBlueprintHelperLibrary::SimpleMoveToLocation()`.
- Character faces movement direction automatically via `bOrientRotationToMovement = true` (already set in `ASabriMMOCharacter` constructor).
- Movement speed is 150 units/sec base in RO. Our UE5 implementation uses `CharacterMovement->MaxWalkSpeed = 500.f` (set in the C++ constructor at `SabriMMOCharacter.cpp:46`).
- Speed modifiers from buffs/debuffs are applied as multipliers on `MaxWalkSpeed`.
- Click destination could optionally show a small cursor indicator on the ground (not currently implemented; deferred to a later pass).

### WASD Movement
- Not present in original RO Classic, but standard in modern recreations.
- WASD provides direct 8-directional movement relative to camera orientation.
- WASD input cancels any active click-to-move pathfinding immediately.
- Character rotates to face movement direction via `bOrientRotationToMovement` (already configured).
- Movement direction is calculated from controller yaw rotation — forward/right vectors derived from `GetController()->GetControlRotation()`, matching the existing `ASabriMMOCharacter::DoMove()` implementation.

### Click-to-Attack Movement
- When clicking an enemy actor, the character auto-walks toward the enemy until within attack range, then stops and begins auto-attacking.
- If the enemy moves out of range during combat, the character chases automatically (per-tick range re-check).
- This is the "walk-to-attack" behavior. It is conceptually similar to `WalkToCast` in `SkillTreeSubsystem.cpp` but for auto-attacks rather than skills.

### Movement Blocking
- Movement is blocked during Stun, Freeze, and Stone (phase 2 of Stone Curse).
- Movement speed is drastically reduced during Curse.
- Movement interrupts spell casting (if the player moves during a cast bar, the cast is cancelled — handled server-side).
- Note: movement blocking is currently enforced server-side via `getCombinedModifiers()`. The client subsystem should respect `combat:error` responses but does NOT need to duplicate server-side validation. The server is authoritative.

### Movement Speed
- Base 150 speed in RO Classic. Our UE5 base is `MaxWalkSpeed = 500.f`.
- Speed modifiers from Increase AGI, Decrease AGI, Blessing, Curse, etc. are applied as multipliers.
- The server sends speed modifiers via `player:stats` events; the client applies them to `MaxWalkSpeed`.
- Speed modification is out of scope for this migration plan (it will remain in the existing stat update flow).

---

## 2. Current Blueprint Implementation (What Gets Replaced)

All of the following Blueprint logic lives in **BP_MMOCharacter** (the Blueprint child of `ASabriMMOCharacter`). These are the nodes/graphs that will be replaced by C++ and eventually removed from the Blueprint.

### BP_MMOCharacter Event Graph -- IA_ClickToMove

```
EnhancedInputAction IA_ClickToMove [Triggered]
  -> GetHitResultUnderCursorByChannel(ECC_Visibility)
  -> Branch: hit?
     -> YES: SimpleMoveToLocation(hitLocation)
     -> NO: (nothing)
```

Uses NavMesh pathfinding internally via the AIController. The Blueprint input action `IA_ClickToMove` is mapped to Left Mouse Button in the Blueprint's Input Mapping Context (`IMC_MMOCharacter`).

### BP_MMOCharacter Event Graph -- IA_Attack (Left Click on Actor)

```
EnhancedInputAction IA_Attack [Triggered]
  -> GetHitResultUnderCursor(ECC_Visibility)
  -> Branch: hit actor valid?
     -> Cast to BP_EnemyCharacter
        -> Success: EmitCombatAttack(EnemyId, true)
     -> Cast to ShopNPC
        -> Success: ShopNPC.Interact()
     -> Cast to KafraNPC
        -> Success: KafraNPC.Interact()
     -> (fall through to IA_ClickToMove behavior)
```

This is a cast chain that determines what was clicked. `IA_Attack` fires every frame while the left mouse button is held (Triggered event). `EmitCombatAttack` calls `UMultiplayerEventSubsystem::EmitCombatAttack()` which emits `combat:attack` to the server.

### BP_MMOCharacter Event Graph -- IA_Move (WASD)

```
EnhancedInputAction IA_Move [Triggered]
  -> StopMovement() (cancels any active click-to-move)
  -> Then: parent C++ ASabriMMOCharacter::Move() handles direction
```

The `StopMovement()` call is critical: it ensures WASD input immediately cancels any NavMesh pathfinding from click-to-move. The actual directional movement is already handled by the C++ base class `ASabriMMOCharacter::Move()` -> `DoMove()`.

### BP_MMOCharacter Event Graph -- Camera

```
IA_CameraRotate [Started]     -> AC_CameraController.SetRotating(true)
IA_CameraRotate [Completed]   -> AC_CameraController.SetRotating(false)
IA_Look [Triggered]           -> AC_CameraController.SetLookInput(deltaX, deltaY)
IA_Zoom [Triggered]           -> AC_CameraController.HandleZoom(axisValue)
```

`IA_CameraRotate` is bound to Right Mouse Button. It enables/disables camera rotation mode. `IA_Look` provides the mouse delta while rotating. `IA_Zoom` is bound to Mouse Wheel.

### AC_CameraController Component (Blueprint ActorComponent)

This is a Blueprint ActorComponent attached to BP_MMOCharacter. It manages:

- **HandleCameraRotation(DeltaTime)**: While right-click is held, applies accumulated yaw/pitch delta to the SpringArm's world rotation. Pitch is clamped.
- **HandleZoom(AxisValue)**: Adjusts `TargetArmLength` on the SpringArm. Range: 200-1500 UE units.
- **SetLookInput(DeltaX, DeltaY)**: Stores mouse delta for the next Tick.
- **SetRotating(bIsRotating)**: State setter for rotation mode.

### Event Tick Usage in BP_MMOCharacter

The Blueprint Event Tick has two main execution pins:

**Pin 0 — Camera + Hover Detection**:
```
Event Tick
  -> AC_CameraController.HandleCameraRotation(DeltaTime)
  -> BP_TargetingSystem.UpdateHoverDetection()
```

**Pin 1 — Auto-Attack Range Loop**:
```
Event Tick
  -> Branch: bIsAutoAttacking?
     -> YES: Get distance to AttackTarget
        -> Branch: distance > AttackRange?
           -> YES: SimpleMoveToLocation(target location) [chase]
           -> NO: StopMovement() + EmitCombatAttack(targetId, isEnemy)
```

This is the walk-to-attack loop. Every frame, if the player is auto-attacking, it checks distance to the target. If out of range, it pathfinds toward the target. If in range, it stops and emits the attack event. This creates the "chase if enemy moves" behavior.

---

## 3. New C++ Architecture

### Design Principles

1. **UWorldSubsystem pattern** — Consistent with all other subsystems in the project (19 existing subsystems). Auto-created per world, cleaned up on world teardown.
2. **Enhanced Input programmatic creation** — Follow the pattern established in `ASabriMMOCharacter::CreateUIToggleActions()` where input actions and mapping contexts are created in code, not as Blueprint assets.
3. **Socket event integration via EventRouter** — Follow the pattern from `SkillTreeSubsystem`, `InventorySubsystem`, etc.: register handlers in `OnWorldBeginPlay`, unregister in `Deinitialize`.
4. **No duplicate server validation** — The server is authoritative. The client sends `combat:attack` and trusts the server's `combat:auto_attack_started` / `combat:error` responses. Client-side range checking is for UX (walk-to-attack), not validation.
5. **Clean separation** — `UPlayerInputSubsystem` handles all input routing and movement. `UCameraSubsystem` handles camera rotation and zoom. Neither depends on the other.

### System Diagram

```
                    Left Click
                        |
                        v
              UPlayerInputSubsystem
             /        |         \
            v          v          v
      Enemy Actor   NPC Actor   Ground
            |          |          |
            v          v          v
     StartAttacking  Interact  ClickMoveToLocation
            |
            v
     Walk-to-Attack
     (per-tick chase)
            |
            v
     EmitCombatAttack()
     (via UMultiplayerEventSubsystem)


              Right Click + Drag
                     |
                     v
             UCameraSubsystem
                     |
                     v
             SpringArm Yaw Rotation


              Mouse Wheel
                  |
                  v
          UCameraSubsystem
                  |
                  v
          TargetArmLength Zoom


              WASD Keys
                  |
                  v
         UPlayerInputSubsystem
                  |
                  v
         Cancel ClickMove + DoMove()
         (existing ASabriMMOCharacter::DoMove)
```

---

## 4. Detailed Class Specifications

### UPlayerInputSubsystem (UWorldSubsystem)

**Replaces**: IA_ClickToMove, IA_Attack input handling, IA_Move WASD interplay, Event Tick auto-attack loop

**File**: `client/SabriMMO/Source/SabriMMO/UI/PlayerInputSubsystem.h`

```cpp
UCLASS()
class SABRIMMO_API UPlayerInputSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual TStatId GetStatId() const override;

    // ---- Auto-attack state (read by other subsystems) ----
    bool IsAutoAttacking() const { return bIsAutoAttacking; }
    int32 GetAttackTargetId() const { return AttackTargetId; }
    bool IsAttackTargetEnemy() const { return bAttackTargetIsEnemy; }

    // ---- Manual control (called by other subsystems if needed) ----
    void StopAutoAttack();
    void StopClickMove();

private:
    // ---- Input Setup ----
    void SetupInputActions();
    void BindInputActions();

    // ---- Input Handlers ----
    void HandleLeftClick(const FInputActionValue& Value);
    void HandleLeftClickReleased(const FInputActionValue& Value);
    void HandleWASDMove(const FInputActionValue& Value);

    // ---- Click Routing ----
    void RouteLeftClick();

    // ---- Movement ----
    void ClickMoveToLocation(const FVector& Destination);
    void MoveToActor(AActor* Target, float Range);
    void CancelMovement();

    // ---- Auto-Attack ----
    void StartAttacking(AActor* EnemyActor, int32 EnemyId);
    void TickAutoAttack(float DeltaTime);
    void HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data);
    void HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data);
    void HandleTargetLost(const TSharedPtr<FJsonValue>& Data);
    void HandleOutOfRange(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatError(const TSharedPtr<FJsonValue>& Data);

    // ---- NPC Interaction ----
    void TryInteractWithActor(AActor* HitActor);
    void WalkToAndInteract(AActor* NPC, float InteractionRadius);
    void TickWalkToInteract(float DeltaTime);

    // ---- Utility ----
    int32 GetEnemyIdFromActor(AActor* Actor) const;
    bool IsEnemyActor(AActor* Actor) const;
    bool IsNPCActor(AActor* Actor) const;
    APawn* GetLocalPawn() const;
    APlayerController* GetLocalPC() const;

    // ---- Input Action State ----
    UPROPERTY()
    UInputMappingContext* GameplayIMC = nullptr;

    UPROPERTY()
    UInputAction* LeftClickAction = nullptr;

    UPROPERTY()
    UInputAction* WASDMoveAction = nullptr;

    // ---- Click-to-Move State ----
    FVector ClickMoveDestination = FVector::ZeroVector;
    bool bIsClickMoving = false;

    // ---- Auto-Attack State ----
    TWeakObjectPtr<AActor> AttackTarget;
    int32 AttackTargetId = 0;
    bool bAttackTargetIsEnemy = false;
    bool bIsAutoAttacking = false;
    float AttackRange = 200.f; // Updated from server's auto_attack_started

    // ---- Walk-to-Interact State ----
    TWeakObjectPtr<AActor> PendingInteractNPC;
    float PendingInteractRadius = 300.f;
    bool bIsWalkingToInteract = false;

    // ---- Spam Guard ----
    double LastLeftClickTime = 0.0;
    double LastAttackEmitTime = 0.0;
};
```

### UCameraSubsystem (UWorldSubsystem)

**Replaces**: AC_CameraController Blueprint component, IA_CameraRotate / IA_Look / IA_Zoom input handling

**File**: `client/SabriMMO/Source/SabriMMO/UI/CameraSubsystem.h`

```cpp
UCLASS()
class SABRIMMO_API UCameraSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual TStatId GetStatId() const override;

    // ---- Public Queries ----
    bool IsRotatingCamera() const { return bIsRotatingCamera; }
    float GetCurrentYaw() const { return CurrentYaw; }

private:
    // ---- Input Setup ----
    void SetupInputActions();
    void BindInputActions();

    // ---- Input Handlers ----
    void HandleRightClickStart(const FInputActionValue& Value);
    void HandleRightClickEnd(const FInputActionValue& Value);
    void HandleMouseMove(const FInputActionValue& Value);
    void HandleMouseWheel(const FInputActionValue& Value);

    // ---- Camera Update ----
    void UpdateCameraRotation(float DeltaTime);
    void UpdateCameraZoom(float DeltaTime);

    // ---- Utility ----
    USpringArmComponent* FindSpringArm() const;

    // ---- Input Actions ----
    UPROPERTY()
    UInputMappingContext* CameraIMC = nullptr;

    UPROPERTY()
    UInputAction* RightClickAction = nullptr;

    UPROPERTY()
    UInputAction* MouseMoveAction = nullptr;

    UPROPERTY()
    UInputAction* MouseWheelAction = nullptr;

    // ---- Camera State ----
    bool bIsRotatingCamera = false;
    float CurrentYaw = 0.f;
    float PendingYawDelta = 0.f;
    float PendingPitchDelta = 0.f;

    // ---- Zoom State ----
    float CurrentArmLength = 400.f;
    float TargetArmLength = 400.f;

    // ---- Configuration ----
    static constexpr float MinArmLength = 200.f;
    static constexpr float MaxArmLength = 1500.f;
    static constexpr float ZoomSpeed = 100.f;
    static constexpr float ZoomInterpSpeed = 10.f;
    static constexpr float FixedPitch = -55.f;   // RO Classic camera angle
    static constexpr float RotationSensitivity = 1.f;

    // ---- Cached References ----
    TWeakObjectPtr<USpringArmComponent> CachedSpringArm;
};
```

---

## 5. Click Routing Logic

The core of `HandleLeftClick` replaces the Blueprint cast chain in `IA_Attack`. The routing order matters: skill targeting mode takes highest priority, then enemy actors, then NPCs, then ground movement.

```
HandleLeftClick():
  0. Gate: if socket not connected (login screen), early return
  1. If USkillTreeSubsystem::IsInTargetingMode() -> DO NOTHING (let SkillTree handle it)
  2. GetHitResultUnderCursor(ECC_Visibility)
  3. If no blocking hit -> early return

  // --- Determine what was clicked ---
  4. If hit actor has "EnemyId" property (BP_EnemyCharacter) AND EnemyId > 0:
       -> Extract EnemyId via reflection (same as SkillTreeSubsystem::GetEnemyIdFromActor)
       -> StartAttacking(enemyActor, enemyId)
       -> return

  5. If hit actor is AShopNPC:
       -> Check distance. If in range: Interact(). Else: WalkToAndInteract(npc, radius)
       -> return

  6. If hit actor is AKafraNPC:
       -> Check distance. If in range: Interact(). Else: WalkToAndInteract(npc, radius)
       -> return

  7. If hit actor is AMMORemotePlayer (future PvP — placeholder):
       -> Log + ignore for now
       -> return

  8. Otherwise (ground click):
       -> If bIsAutoAttacking: StopAutoAttack()
       -> ClickMoveToLocation(hitResult.ImpactPoint)
```

**Key differences from current Blueprint**:
- Step 1 is new: the Blueprint has no awareness of SkillTreeSubsystem targeting mode, so the targeting overlay handles its own click capture. In the C++ version, we explicitly check and yield.
- Step 5/6 now include walk-to-interact: if the player is too far from an NPC, we walk to them first, then interact. Currently the Blueprint just calls Interact() which has its own internal range check and silently fails if too far.
- Step 8 stops auto-attacking on ground click (same as current Blueprint behavior).

### Enemy Detection via Reflection

Since enemy actors are Blueprint-spawned `BP_EnemyCharacter` instances (not C++ `AMMOEnemyActor`), we must use UE5 property reflection to read the `EnemyId` variable. This is the same approach used by `USkillTreeSubsystem::GetEnemyIdFromActor()` and `USkillVFXSubsystem::FindEnemyActorById()`.

```cpp
int32 UPlayerInputSubsystem::GetEnemyIdFromActor(AActor* Actor) const
{
    if (!Actor) return 0;

    // BP_EnemyCharacter stores "EnemyId" as an int Blueprint variable
    static const FName PrimaryName(TEXT("EnemyId"));
    if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(PrimaryName))
    {
        if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
        {
            return IntProp->GetPropertyValue_InContainer(Actor);
        }
    }
    return 0;
}

bool UPlayerInputSubsystem::IsEnemyActor(AActor* Actor) const
{
    return GetEnemyIdFromActor(Actor) > 0;
}
```

---

## 6. Walk-to-Attack System

This replaces the Event Tick Pin 1 auto-attack loop in the Blueprint. The C++ version runs in the subsystem's `Tick()` override.

### State Machine

```
IDLE
  -> Left-click enemy -> WALKING_TO_ATTACK
  -> Left-click ground -> CLICK_MOVING (separate state)

WALKING_TO_ATTACK
  -> In range -> emit combat:attack -> WAITING_FOR_SERVER
  -> WASD input -> cancel -> IDLE
  -> Click ground -> cancel -> CLICK_MOVING
  -> Click different enemy -> restart with new target
  -> Target destroyed/invalid -> cancel -> IDLE

WAITING_FOR_SERVER
  -> Receive combat:auto_attack_started -> AUTO_ATTACKING
  -> Receive combat:error -> IDLE
  -> Receive combat:out_of_range -> WALKING_TO_ATTACK (walk closer)

AUTO_ATTACKING
  -> Target moves out of range -> CHASING
  -> Click ground -> StopAutoAttack() -> IDLE
  -> Click different enemy -> restart with new target
  -> Receive combat:target_lost -> IDLE
  -> Receive combat:auto_attack_stopped -> IDLE
  -> WASD input -> StopAutoAttack() -> IDLE

CHASING
  -> In range again -> emit combat:attack (server will continue auto-attack loop)
  -> Same cancellation rules as WALKING_TO_ATTACK
```

### Tick Logic (TickAutoAttack)

```cpp
void UPlayerInputSubsystem::TickAutoAttack(float DeltaTime)
{
    if (!bIsAutoAttacking) return;

    // Validate target still exists
    if (!AttackTarget.IsValid())
    {
        StopAutoAttack();
        return;
    }

    APawn* Pawn = GetLocalPawn();
    if (!Pawn) return;

    AActor* Target = AttackTarget.Get();
    float Distance = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());

    if (Distance > AttackRange + 30.f) // 30 UE unit tolerance
    {
        // Out of range -> chase (pathfind toward target)
        MoveToActor(Target, AttackRange);
    }
    else
    {
        // In range -> stop movement, server handles attack ticks
        CancelMovement();
    }
}
```

### Attack Range

The attack range comes from the server's `combat:auto_attack_started` event payload:
```json
{
    "targetId": 2000001,
    "isEnemy": true,
    "attackRange": 200,
    "aspd": 175
}
```

Default client-side `AttackRange` is `200.f` (melee range). Updated when the server confirms the attack. Ranged weapons (bows) will have a larger range sent by the server.

### Interaction with combat:attack Emission

Currently, `EmitCombatAttack()` lives on `UMultiplayerEventSubsystem`. The `PlayerInputSubsystem` will call it:

```cpp
void UPlayerInputSubsystem::StartAttacking(AActor* EnemyActor, int32 EnemyId)
{
    // Cancel any click-to-move
    StopClickMove();

    // Cancel any walk-to-interact
    bIsWalkingToInteract = false;
    PendingInteractNPC.Reset();

    // Set attack state
    AttackTarget = EnemyActor;
    AttackTargetId = EnemyId;
    bAttackTargetIsEnemy = true;
    bIsAutoAttacking = true;

    // Emit combat:attack to server
    if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
    {
        MES->EmitCombatAttack(EnemyId, /*bIsEnemy=*/ true);
    }

    // Walk toward target if out of range (TickAutoAttack will handle chasing)
    APawn* Pawn = GetLocalPawn();
    if (Pawn)
    {
        float Distance = FVector::Dist2D(Pawn->GetActorLocation(), EnemyActor->GetActorLocation());
        if (Distance > AttackRange + 30.f)
        {
            MoveToActor(EnemyActor, AttackRange);
        }
    }
}
```

---

## 7. Camera Subsystem

### Why a Separate Subsystem

Camera control is independent from player input/movement. Separating it:
- Keeps each subsystem focused (single responsibility)
- Allows camera behavior changes without touching movement code
- Makes it easy to add camera modes later (e.g., free-look, cinematic)
- Matches the project pattern of one subsystem per domain

### Camera Rotation (Right-Click Hold)

RO Classic uses a fixed top-down camera angle with yaw rotation. The camera does NOT use controller rotation for yaw (the pawn should not rotate with the camera). Instead, the `SpringArm`'s world rotation is set directly.

**Current behavior** (from AC_CameraController):
- Right-click start: `bIsRotatingCamera = true`
- Mouse move while rotating: accumulate yaw delta
- Per-tick: apply yaw to SpringArm, clamp pitch
- Right-click end: `bIsRotatingCamera = false`

**C++ replacement**:
```cpp
void UCameraSubsystem::UpdateCameraRotation(float DeltaTime)
{
    if (!bIsRotatingCamera) return;

    USpringArmComponent* SpringArm = CachedSpringArm.Get();
    if (!SpringArm) return;

    // Apply accumulated yaw delta
    CurrentYaw += PendingYawDelta * RotationSensitivity;
    PendingYawDelta = 0.f;
    PendingPitchDelta = 0.f; // Consume but ignore pitch (fixed at -55)

    // Set world rotation (yaw only, fixed pitch)
    SpringArm->SetWorldRotation(FRotator(FixedPitch, CurrentYaw, 0.f));
}
```

**Note on `bUsePawnControlRotation`**: The `CameraBoom` SpringArm is currently created with `bUsePawnControlRotation = true` in the C++ constructor (`SabriMMOCharacter.cpp:55`). When we migrate the camera to this subsystem, we need to set `bUsePawnControlRotation = false` so the subsystem has full control. This is a one-line change in `SabriMMOCharacter.cpp`.

### Zoom

```cpp
void UCameraSubsystem::UpdateCameraZoom(float DeltaTime)
{
    USpringArmComponent* SpringArm = CachedSpringArm.Get();
    if (!SpringArm) return;

    // Smooth interpolation toward target
    CurrentArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
    SpringArm->TargetArmLength = CurrentArmLength;
}

void UCameraSubsystem::HandleMouseWheel(const FInputActionValue& Value)
{
    float AxisValue = Value.Get<float>();
    TargetArmLength = FMath::Clamp(TargetArmLength - AxisValue * ZoomSpeed, MinArmLength, MaxArmLength);
}
```

### SpringArm Discovery

The subsystem needs to find the pawn's SpringArm. Since the pawn is spawned by the Level Blueprint (not by the subsystem), we discover it lazily:

```cpp
USpringArmComponent* UCameraSubsystem::FindSpringArm() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return nullptr;

    return Pawn->FindComponentByClass<USpringArmComponent>();
}
```

Called in `Tick()` if the cached reference is stale (e.g., after pawn respawn or zone transition).

---

## 8. Input System Design

### IMC Priority Hierarchy

The project uses Enhanced Input with multiple `UInputMappingContext` objects at different priorities. Higher priority consumes input first.

| Priority | IMC | Owner | Purpose |
|----------|-----|-------|---------|
| 0 | IMC_MMOCharacter (BP asset) | BP_MMOCharacter | Legacy WASD, Look (being replaced) |
| 1 | IMC_UIToggles | ASabriMMOCharacter C++ | F5-F9, H, 1-9 hotbar keys |
| 2 | IMC_GameplayInput | UPlayerInputSubsystem | Left click, WASD |
| 2 | IMC_Camera | UCameraSubsystem | Right click, mouse move, wheel |

Priority 2 for gameplay and camera ensures these consume input before the legacy Blueprint IMC at priority 0. During the migration period, this means the C++ subsystems handle input first. Once the Blueprint IMC entries are removed, priority ordering no longer matters for these inputs.

### Input Action Definitions

All input actions are created programmatically (no Blueprint assets needed), following the pattern from `CreateUIToggleActions()`.

**UPlayerInputSubsystem::SetupInputActions()**:
```cpp
void UPlayerInputSubsystem::SetupInputActions()
{
    GameplayIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_GameplayInput"));

    // Left Click: Triggered (fires each frame while held — matches BP behavior for attack spam)
    // Also bind Started for single-click detection (click-to-move)
    LeftClickAction = NewObject<UInputAction>(this, TEXT("IA_LeftClick"));
    LeftClickAction->ValueType = EInputActionValueType::Boolean;
    GameplayIMC->MapKey(LeftClickAction, EKeys::LeftMouseButton);

    // WASD Movement: Axis2D (X = right, Y = forward)
    WASDMoveAction = NewObject<UInputAction>(this, TEXT("IA_WASDMoveCpp"));
    WASDMoveAction->ValueType = EInputActionValueType::Axis2D;
    // Map WASD keys with swizzle modifiers:
    // W = +Y, S = -Y, D = +X, A = -X
    // (same as the existing IA_Move in IMC_MMOCharacter)
    FEnhancedActionKeyMapping& WMapping = GameplayIMC->MapKey(WASDMoveAction, EKeys::W);
    // Note: UE5 swizzle input modifiers must be added via code:
    // WMapping.Modifiers.Add(SwizzleInputAxisValues(YXZ)) + Negate as needed
    // Full modifier setup follows the same pattern as the existing BP IA_Move
}
```

**WASD Modifier Note**: The existing `IA_Move` in the Blueprint uses Swizzle Input Axis Values and Negate modifiers to map W/A/S/D to a 2D axis. Replicating this in code requires:
```cpp
#include "InputModifiers.h"

// W key: input Y axis (forward), no negate
FEnhancedActionKeyMapping& WMap = GameplayIMC->MapKey(WASDMoveAction, EKeys::W);
UInputModifierSwizzleAxis* WSwizzle = NewObject<UInputModifierSwizzleAxis>(this);
WSwizzle->Order = EInputAxisSwizzle::YXZ;
WMap.Modifiers.Add(WSwizzle);

// S key: input Y axis (forward), negated
FEnhancedActionKeyMapping& SMap = GameplayIMC->MapKey(WASDMoveAction, EKeys::S);
UInputModifierSwizzleAxis* SSwizzle = NewObject<UInputModifierSwizzleAxis>(this);
SSwizzle->Order = EInputAxisSwizzle::YXZ;
SMap.Modifiers.Add(SSwizzle);
UInputModifierNegate* SNegate = NewObject<UInputModifierNegate>(this);
SMap.Modifiers.Add(SNegate);

// D key: input X axis (right), no modifiers needed
GameplayIMC->MapKey(WASDMoveAction, EKeys::D);

// A key: input X axis (right), negated
FEnhancedActionKeyMapping& AMap = GameplayIMC->MapKey(WASDMoveAction, EKeys::A);
UInputModifierNegate* ANegate = NewObject<UInputModifierNegate>(this);
AMap.Modifiers.Add(ANegate);
```

**UCameraSubsystem::SetupInputActions()**:
```cpp
void UCameraSubsystem::SetupInputActions()
{
    CameraIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_Camera"));

    // Right Click: Bool (Started = begin rotate, Completed = end rotate)
    RightClickAction = NewObject<UInputAction>(this, TEXT("IA_RightClickCamera"));
    RightClickAction->ValueType = EInputActionValueType::Boolean;
    CameraIMC->MapKey(RightClickAction, EKeys::RightMouseButton);

    // Mouse XY: Axis2D (delta for camera rotation)
    MouseMoveAction = NewObject<UInputAction>(this, TEXT("IA_MouseDelta"));
    MouseMoveAction->ValueType = EInputActionValueType::Axis2D;
    GameplayIMC->MapKey(MouseMoveAction, EKeys::Mouse2D);

    // Mouse Wheel: Axis1D (zoom)
    MouseWheelAction = NewObject<UInputAction>(this, TEXT("IA_MouseWheelZoom"));
    MouseWheelAction->ValueType = EInputActionValueType::Axis1D;
    CameraIMC->MapKey(MouseWheelAction, EKeys::MouseWheelAxis);
}
```

### Input Binding in Subsystems

Subsystems cannot call `SetupPlayerInputComponent` (that is an Actor method). Instead, they access the Enhanced Input subsystem directly:

```cpp
void UPlayerInputSubsystem::BindInputActions()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    ULocalPlayer* LP = PC->GetLocalPlayer();
    if (!LP) return;

    // Register the IMC
    UEnhancedInputLocalPlayerSubsystem* InputSub =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
    if (!InputSub) return;

    InputSub->AddMappingContext(GameplayIMC, 2); // Priority 2

    // Bind actions via the pawn's UEnhancedInputComponent
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
    if (!EIC) return;

    EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this,
        &UPlayerInputSubsystem::HandleLeftClick);
    EIC->BindAction(WASDMoveAction, ETriggerEvent::Triggered, this,
        &UPlayerInputSubsystem::HandleWASDMove);
}
```

**CRITICAL**: The pawn may not exist when `OnWorldBeginPlay` fires (it is spawned by the Level Blueprint). Use a one-frame timer delay:

```cpp
void UPlayerInputSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Gate: only activate in game levels (not login screen)
    UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
    if (!GI || !GI->IsSocketConnected()) return;

    SetupInputActions();

    // Defer binding by one frame — pawn may not be spawned yet
    InWorld.GetTimerManager().SetTimerForNextTick([this]()
    {
        BindInputActions();
        RegisterSocketHandlers();
    });
}
```

### WASD Movement Handler

```cpp
void UPlayerInputSubsystem::HandleWASDMove(const FInputActionValue& Value)
{
    // Cancel any active click-to-move or walk-to-interact
    StopClickMove();
    bIsWalkingToInteract = false;
    PendingInteractNPC.Reset();

    // Stop auto-attacking if WASD is pressed
    if (bIsAutoAttacking)
    {
        StopAutoAttack();
    }

    // Delegate actual movement to the existing C++ base class
    // ASabriMMOCharacter::DoMove() handles camera-relative direction
    APawn* Pawn = GetLocalPawn();
    if (ASabriMMOCharacter* Char = Cast<ASabriMMOCharacter>(Pawn))
    {
        FVector2D MoveVec = Value.Get<FVector2D>();
        Char->DoMove(MoveVec.X, MoveVec.Y);
    }
}
```

---

## 9. NavMesh Pathfinding

### Which API to Use

The project already uses `UAIBlueprintHelperLibrary::SimpleMoveToLocation()` in `SkillTreeSubsystem.cpp` (WalkToCast). The same function is used by the Blueprint's `SimpleMoveToLocation` node. We will use the same API for consistency.

**Required includes**:
```cpp
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationSystem.h" // Already in SabriMMO.Build.cs
```

### Click-to-Move Implementation

```cpp
void UPlayerInputSubsystem::ClickMoveToLocation(const FVector& Destination)
{
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    bIsClickMoving = true;
    ClickMoveDestination = Destination;

    UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Destination);
}
```

### Cancelling Movement

Following the exact pattern from `WalkToCast::StopMovement()` in `SkillTreeSubsystem.cpp`:

```cpp
void UPlayerInputSubsystem::CancelMovement()
{
    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    // Stop AI pathfinding (SimpleMoveToLocation to current pos)
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Pawn->GetActorLocation());

    // Stop controller-level movement
    PC->StopMovement();

    // Zero out velocity on CharacterMovementComponent
    if (ACharacter* Char = Cast<ACharacter>(Pawn))
    {
        if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
        {
            CMC->StopMovementImmediately();
        }
    }

    bIsClickMoving = false;
}
```

### Move-to-Actor (for Walk-to-Attack and Walk-to-Interact)

```cpp
void UPlayerInputSubsystem::MoveToActor(AActor* Target, float Range)
{
    if (!Target) return;

    APlayerController* PC = GetLocalPC();
    if (!PC) return;

    // SimpleMoveToLocation toward the target's current position
    // TickAutoAttack / TickWalkToInteract will update the destination each tick
    // if the target moves
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Target->GetActorLocation());
}
```

**Note**: We do NOT use `SimpleMoveToActor()` because the target actors are Blueprint-spawned and may not have proper navigation agent interfaces. Instead, we re-issue `SimpleMoveToLocation()` each tick with the target's updated position, which achieves the same chase behavior.

### Ground Snap

The existing `UZoneTransitionSubsystem::SnapLocationToGround()` static helper is available for any position corrections needed. The pawn's initial spawn position is already ground-snapped in `ASabriMMOCharacter::BeginPlay()`. Click-to-move destinations are on the NavMesh by definition (the cursor hit comes from a visibility trace against world geometry), so no additional ground snapping is needed for click-to-move.

---

## 10. Socket Events Handled

The `PlayerInputSubsystem` registers handlers for combat-related socket events via the `EventRouter`. These events inform the subsystem about server-side auto-attack state changes.

| Event | Handler | Purpose | Payload Fields |
|-------|---------|---------|----------------|
| `combat:auto_attack_started` | `HandleAutoAttackStarted` | Server confirmed attack; update attack range | `targetId`, `isEnemy`, `attackRange`, `aspd` |
| `combat:auto_attack_stopped` | `HandleAutoAttackStopped` | Server stopped attack (target switch, death, etc.) | `reason`, `oldTargetId`, `oldIsEnemy` |
| `combat:target_lost` | `HandleTargetLost` | Target died or disconnected | `reason`, `isEnemy` |
| `combat:out_of_range` | `HandleOutOfRange` | Attack failed due to range — walk closer | `targetId`, `isEnemy`, `targetX/Y/Z`, `distance`, `requiredRange` |
| `combat:error` | `HandleCombatError` | Generic combat error (dead, stunned, overweight) | `message` |

### Handler Implementations

```cpp
void UPlayerInputSubsystem::HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    TSharedPtr<FJsonObject> Obj = Data->AsObject();
    if (!Obj.IsValid()) return;

    // Update attack range from server
    double Range = 0;
    if (Obj->TryGetNumberField(TEXT("attackRange"), Range))
    {
        AttackRange = static_cast<float>(Range);
    }

    bIsAutoAttacking = true;
    UE_LOG(LogPlayerInput, Log, TEXT("Auto-attack started. Range: %.0f"), AttackRange);
}

void UPlayerInputSubsystem::HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data)
{
    bIsAutoAttacking = false;
    AttackTarget.Reset();
    AttackTargetId = 0;
    CancelMovement();
    UE_LOG(LogPlayerInput, Log, TEXT("Auto-attack stopped by server."));
}

void UPlayerInputSubsystem::HandleTargetLost(const TSharedPtr<FJsonValue>& Data)
{
    bIsAutoAttacking = false;
    AttackTarget.Reset();
    AttackTargetId = 0;
    CancelMovement();
    UE_LOG(LogPlayerInput, Log, TEXT("Attack target lost."));
}

void UPlayerInputSubsystem::HandleOutOfRange(const TSharedPtr<FJsonValue>& Data)
{
    if (!bIsAutoAttacking || !AttackTarget.IsValid()) return;

    // Server says we're out of range — walk closer
    TSharedPtr<FJsonObject> Obj = Data->AsObject();
    if (Obj.IsValid())
    {
        double Dist = 0, Required = 0;
        Obj->TryGetNumberField(TEXT("distance"), Dist);
        Obj->TryGetNumberField(TEXT("requiredRange"), Required);
        if (Required > 0) AttackRange = static_cast<float>(Required);
    }

    MoveToActor(AttackTarget.Get(), AttackRange);
}
```

### Coexistence with MultiplayerEventSubsystem

`MultiplayerEventSubsystem` already registers handlers for these same events and forwards them to BP_SocketManager via ProcessEvent. The `EventRouter` supports multiple handlers per event, so both subsystems can coexist. The `PlayerInputSubsystem` handlers update local C++ state; the `MultiplayerEventSubsystem` handlers forward to Blueprint for visual updates (targeting indicators, hover effects, etc.).

---

## 11. Integration with Other Subsystems

### USkillTreeSubsystem (Skill Targeting)

**Interaction**: When `SkillTreeSubsystem::IsInTargetingMode()` returns true, `PlayerInputSubsystem` must NOT process left clicks. The SkillTreeSubsystem's `SSkillTargetingOverlay` captures clicks during targeting mode.

**Implementation**: Check `IsInTargetingMode()` at the top of `HandleLeftClick()`:
```cpp
void UPlayerInputSubsystem::HandleLeftClick(const FInputActionValue& Value)
{
    USkillTreeSubsystem* SkillTree = GetWorld()->GetSubsystem<USkillTreeSubsystem>();
    if (SkillTree && SkillTree->IsInTargetingMode())
    {
        // Skill targeting overlay handles this click
        return;
    }
    // ... normal click routing ...
}
```

**WalkToCast interaction**: If `WalkToCast` is active (walking to cast range for a skill), a ground click from `PlayerInputSubsystem` should cancel it. The `WalkToCast::Cancel()` function is cpp-local (file-static namespace), so `PlayerInputSubsystem` cannot call it directly. Instead, the click-to-move will naturally override the `SimpleMoveToLocation` destination. The `WalkToCast` timer will detect the pawn moved away from the expected path and cancel. Alternatively, we could expose a `CancelWalkToCast()` function on `SkillTreeSubsystem`.

### UMultiplayerEventSubsystem (Combat Attack Emission)

**Interaction**: `PlayerInputSubsystem` calls `EmitCombatAttack()` and `EmitStopAttack()` on the `MultiplayerEventSubsystem` to send combat events to the server. It does NOT emit socket events directly.

```cpp
// In StartAttacking():
if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
{
    MES->EmitCombatAttack(EnemyId, /*bIsEnemy=*/ true);
}

// In StopAutoAttack():
if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
{
    MES->EmitStopAttack();
}
```

### UHotbarSubsystem (Hotbar Keys)

**Interaction**: None. Hotbar keys (1-9) are handled by `ASabriMMOCharacter`'s `IMC_UIToggles` at priority 1. These do not conflict with gameplay input at priority 2 because they use different keys.

### UDamageNumberSubsystem, UCombatStatsSubsystem, UBuffBarSubsystem

**Interaction**: None. These subsystems are display-only and do not affect input or movement.

### UInventorySubsystem, UEquipmentSubsystem, UShopSubsystem

**Interaction**: When modal UI panels are open (inventory, equipment, shop), clicks should be consumed by the Slate widgets, not by the gameplay input. Slate widgets at higher Z-order naturally consume mouse events when they have hit-test areas. However, clicking on empty areas of these panels (or outside them) may still trigger `HandleLeftClick`. The subsystem should check if a Slate widget consumed the click:

```cpp
void UPlayerInputSubsystem::HandleLeftClick(const FInputActionValue& Value)
{
    // Check if Slate consumed this click (a UI widget is under the cursor)
    if (FSlateApplication::IsInitialized())
    {
        FWidgetPath WidgetPath;
        FSlateApplication::Get().FindPathToWidget(
            /* ... check if cursor is over a Slate widget ... */);
        // If over a game-world viewport widget only, proceed
        // If over an inventory/shop/equipment widget, return
    }
    // ... rest of click routing ...
}
```

A simpler approach: check if any modal panel is visible and, if the click hit test returns a widget that is NOT the viewport, skip game input. This requires experimentation during implementation to find the cleanest pattern.

### UCastBarSubsystem

**Interaction**: Movement cancels casting (server-side). No special client-side handling needed. The server will send `skill:cast_cancelled` if the player moves during a cast.

### UPositionBroadcastSubsystem

**Interaction**: None. Position broadcasting runs independently at 30Hz.

---

## 12. Coexistence Strategy (During Migration)

The migration can be done incrementally. Both Blueprint and C++ input handling can coexist temporarily.

### Phase 1: Add C++ Subsystems (No Blueprint Changes)

1. Create `PlayerInputSubsystem.h/.cpp` and `CameraSubsystem.h/.cpp`.
2. Register at IMC priority 2 (higher than Blueprint's priority 0).
3. The C++ subsystems handle input first. Since the Blueprint also binds to the same keys, both may fire.
4. **Test**: Verify C++ input works correctly. There may be double-processing (both C++ and BP fire on the same click). This is acceptable for testing.

### Phase 2: Disable Blueprint Input (Keep BP Nodes, Just Disconnect)

1. In the Unreal Editor, open BP_MMOCharacter.
2. In the Event Graph, **disconnect** (but do not delete) the wires from:
   - `IA_ClickToMove` event node
   - `IA_Attack` event node
   - `IA_CameraRotate` event node
   - `IA_Look` event node
   - `IA_Zoom` event node
   - The WASD `StopMovement()` call before `Move()` (the `Move()` call itself stays — it is the C++ parent function)
3. In BP_MMOCharacter's components, optionally disable `AC_CameraController` (uncheck "Auto Activate" or remove the component).
4. In Event Tick, disconnect:
   - Pin 0: `AC_CameraController.HandleCameraRotation` and `BP_TargetingSystem.UpdateHoverDetection`
   - Pin 1: The auto-attack range checking loop
5. **Test**: Verify everything works with only C++ handling input.

### Phase 3: Clean Up Blueprint (Remove Dead Code)

1. Delete the disconnected nodes from BP_MMOCharacter's Event Graph.
2. Remove `AC_CameraController` component from BP_MMOCharacter.
3. Remove `IA_ClickToMove`, `IA_Attack`, `IA_CameraRotate`, `IA_Look`, `IA_Zoom` from the Blueprint's `IMC_MMOCharacter` Input Mapping Context asset.
4. Optionally remove the input action assets themselves if no other Blueprint references them.

### Phase 4: Clean Up C++ Priority

1. Once the Blueprint IMC no longer contains conflicting actions, the C++ IMC priority can be lowered to 0 if desired.
2. Remove any coexistence guards (e.g., `bUseCppInput` flag if one was added).

---

## 13. Files to Create

| File | Purpose |
|------|---------|
| `client/SabriMMO/Source/SabriMMO/UI/PlayerInputSubsystem.h` | Header: UWorldSubsystem for player input and movement |
| `client/SabriMMO/Source/SabriMMO/UI/PlayerInputSubsystem.cpp` | Implementation: click routing, WASD, walk-to-attack, walk-to-interact |
| `client/SabriMMO/Source/SabriMMO/UI/CameraSubsystem.h` | Header: UWorldSubsystem for camera rotation and zoom |
| `client/SabriMMO/Source/SabriMMO/UI/CameraSubsystem.cpp` | Implementation: right-click camera rotation, zoom interpolation |

---

## 14. Files to Modify

### `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.cpp`

**Change**: Set `bUsePawnControlRotation = false` on the SpringArm so the camera subsystem has full control.

```cpp
// Before:
CameraBoom->bUsePawnControlRotation = true;

// After:
CameraBoom->bUsePawnControlRotation = false;
```

**Risk**: This changes camera behavior immediately. Must be done in conjunction with the CameraSubsystem being active. Consider adding a flag or doing this change in the CameraSubsystem's initialization instead (by modifying the SpringArm at runtime).

**Alternative** (safer): Leave the constructor as-is. In `UCameraSubsystem::OnWorldBeginPlay`, after finding the SpringArm, set `bUsePawnControlRotation = false` at runtime:
```cpp
if (USpringArmComponent* SpringArm = FindSpringArm())
{
    SpringArm->bUsePawnControlRotation = false;
    CachedSpringArm = SpringArm;
}
```

### `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs`

**Change**: None. `NavigationSystem` module is already in `PublicDependencyModuleNames` (line 30). `AIModule` is also already present (line 17). `EnhancedInput` is present (line 15). All required modules are available.

### `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.h`

**Change**: No modifications needed. The existing `DoMove()` and `DoLook()` are already `BlueprintCallable` and `virtual`, so the subsystem can call them. The `GetCameraBoom()` and `GetFollowCamera()` accessors are already public.

### Blueprint Assets (Editor-Only, No Code Changes)

- **BP_MMOCharacter**: Disconnect/remove IA_Attack, IA_ClickToMove, IA_CameraRotate, IA_Look, IA_Zoom event nodes. Remove AC_CameraController component. Disconnect Event Tick auto-attack loop.
- **IMC_MMOCharacter**: Remove the input action mappings for the above actions (or delete the entire IMC if nothing else uses it).

---

## 15. Testing Checklist

### Core Movement
- [ ] Click on ground causes the character to walk to the clicked position via NavMesh
- [ ] Click on ground while already moving redirects to new destination
- [ ] Click on non-walkable area (off NavMesh) does nothing or moves to nearest walkable point
- [ ] WASD movement works with correct camera-relative direction
- [ ] WASD cancels any active click-to-move
- [ ] Character faces movement direction during both click-to-move and WASD

### Camera
- [ ] Right-click hold + mouse drag rotates camera (yaw only)
- [ ] Camera pitch stays fixed at -55 degrees
- [ ] Releasing right-click stops camera rotation
- [ ] Mouse wheel zooms in/out
- [ ] Zoom range is clamped to 200-1500 UE units
- [ ] Zoom interpolates smoothly (not instant)
- [ ] Camera survives zone transitions (re-finds SpringArm after level load)

### Combat (Auto-Attack)
- [ ] Click on enemy actor causes the character to walk toward the enemy
- [ ] When within attack range, character stops and `combat:attack` is emitted
- [ ] Server responds with `combat:auto_attack_started` and attack begins
- [ ] If enemy moves out of range, character chases automatically
- [ ] Click on ground while auto-attacking stops the attack (`combat:stop_attack` emitted)
- [ ] WASD while auto-attacking stops the attack
- [ ] Click on a different enemy switches target (old attack stopped, new attack started)
- [ ] `combat:target_lost` clears attack state and stops movement
- [ ] `combat:auto_attack_stopped` clears attack state
- [ ] `combat:error` (dead, stunned, overweight) stops attack attempt

### NPC Interaction
- [ ] Click on ShopNPC within interaction radius opens the shop
- [ ] Click on ShopNPC outside interaction radius causes walk-to-interact, then opens shop
- [ ] Click on KafraNPC within interaction radius opens Kafra dialog
- [ ] Click on KafraNPC outside interaction radius causes walk-to-interact, then opens Kafra dialog
- [ ] Walk-to-interact is cancelled by WASD movement
- [ ] Walk-to-interact is cancelled by clicking elsewhere

### Skill System Interplay
- [ ] Skill targeting mode (from SkillTreeSubsystem) still works — left click in targeting mode is handled by the targeting overlay, not by PlayerInputSubsystem
- [ ] WalkToCast (walking to skill range, then casting) still works
- [ ] Ground AoE indicator (green circle) still follows cursor during ground-target skills
- [ ] Hotbar keys 1-9 still activate skills/items
- [ ] Casting a skill while auto-attacking uses the current target correctly

### UI Panel Interplay
- [ ] F5 (cycle hotbar), F6 (inventory), F7 (equipment), F8 (combat stats), F9 (close shop) still work
- [ ] Clicking on inventory/equipment/shop UI panels does NOT trigger ground movement behind them
- [ ] Dragging items in inventory does NOT trigger click-to-move
- [ ] Right-click item inspect (from InventorySubsystem) still works

### Zone Transitions
- [ ] Walking through a WarpPortal triggers zone transition correctly
- [ ] After zone transition, input subsystems re-initialize (find new pawn, re-bind input)
- [ ] Camera settings (zoom level, yaw) persist or reset gracefully across zone transitions
- [ ] Ground snap on spawn still works after zone transition

### Edge Cases
- [ ] Spam-clicking on an enemy does not send multiple `combat:attack` per frame (spam guard)
- [ ] Clicking during loading overlay (zone transition) does nothing
- [ ] Alt-tab and return: input still works
- [ ] No crashes when pawn is destroyed (e.g., death) while auto-attacking or click-moving
- [ ] No crashes on logout (subsystem Deinitialize cleans up correctly)

---

## 16. Risk Analysis

### Risk: Double Input Processing During Migration

**Problem**: During Phase 1 (coexistence), both the C++ subsystem and the Blueprint event graph may process the same left-click, causing double `combat:attack` emissions or double `SimpleMoveToLocation` calls.

**Mitigation**: The C++ IMC is at priority 2, the Blueprint IMC at priority 0. If the C++ action is set to consume input (`bConsumeInput = true` on the `FEnhancedActionKeyMapping`), the Blueprint action will not fire. Test this during Phase 1.

### Risk: Pawn Not Available on Subsystem Init

**Problem**: `UWorldSubsystem::OnWorldBeginPlay` fires before the Level Blueprint spawns the pawn. The subsystem cannot bind input actions to a nonexistent pawn.

**Mitigation**: Use `SetTimerForNextTick()` to defer input binding, as documented in Section 8. If the pawn is still not available after one tick (unlikely but possible), use a repeating timer that checks every 0.1s until the pawn exists.

### Risk: SpringArm Reference Becomes Stale

**Problem**: Zone transitions destroy and recreate the pawn. The `CameraSubsystem`'s cached `TWeakObjectPtr<USpringArmComponent>` becomes null.

**Mitigation**: Check `CachedSpringArm.IsValid()` every Tick. If stale, call `FindSpringArm()` to re-acquire. This is cheap (one `FindComponentByClass` call).

### Risk: SkillTreeSubsystem Targeting Conflicts

**Problem**: If `PlayerInputSubsystem` processes a left click before `SkillTreeSubsystem`'s targeting overlay, the click might be consumed and the skill never fires.

**Mitigation**: `PlayerInputSubsystem::HandleLeftClick()` checks `SkillTreeSubsystem::IsInTargetingMode()` first and returns early if true. The targeting overlay (Slate widget at Z=25) handles the click directly via Slate event propagation. No conflict.

### Risk: Walk-to-Attack vs Walk-to-Cast Collision

**Problem**: If the player is auto-attacking (walk-to-attack active) and then activates a skill on a different target (walk-to-cast), both systems issue `SimpleMoveToLocation` calls, causing jittery movement.

**Mitigation**: When `SkillTreeSubsystem::BeginTargeting()` is called, or when a hotbar skill is used, `PlayerInputSubsystem::StopAutoAttack()` should be called. The `SkillTreeSubsystem` already calls `WalkToCast::Cancel()` when beginning a new targeting mode. We should add a call to `PlayerInputSubsystem::StopAutoAttack()` as well, either from `SkillTreeSubsystem` or by having `PlayerInputSubsystem` listen for a targeting-mode-changed signal.

### Risk: Slate UI Eating Clicks Incorrectly

**Problem**: With multiple Slate widgets at various Z-orders, a left-click might be consumed by a transparent widget area, preventing ground movement.

**Mitigation**: All current Slate widgets use `SNew(SBox).Visibility(EVisibility::SelfHitTestInvisible)` for their background areas, and only interactive elements (buttons, slots) have hit-test enabled. This pattern should prevent accidental click consumption. Test thoroughly.

### Risk: `bUsePawnControlRotation` Timing

**Problem**: If we change `bUsePawnControlRotation = false` in the constructor but the CameraSubsystem is not yet active (e.g., login screen), the camera may not rotate at all.

**Mitigation**: Use the runtime approach: leave the constructor as-is (`bUsePawnControlRotation = true`) and set it to `false` only when `CameraSubsystem` initializes in a game level. The `ShouldCreateSubsystem` gate (checks `IsSocketConnected`) ensures this only happens in gameplay worlds.

---

## Appendix A: Existing Code References

| File | Path | Relevant Content |
|------|------|-----------------|
| Character base class | `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.h` | `DoMove()`, `DoLook()`, `TryInteractWithNPC()`, `GetCameraBoom()` |
| Character implementation | `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.cpp` | `CreateUIToggleActions()` pattern, `Move()`, SpringArm/Camera setup |
| Build config | `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs` | Module dependencies (NavigationSystem, AIModule, EnhancedInput already present) |
| EventRouter | `client/SabriMMO/Source/SabriMMO/SocketEventRouter.h` | `RegisterHandler()`, `UnregisterAllForOwner()` |
| GameInstance | `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h` | `IsSocketConnected()`, `GetEventRouter()`, `EmitSocketEvent()` |
| Multiplayer bridge | `client/SabriMMO/Source/SabriMMO/UI/MultiplayerEventSubsystem.h` | `EmitCombatAttack()`, `EmitStopAttack()` |
| SkillTree (targeting) | `client/SabriMMO/Source/SabriMMO/UI/SkillTreeSubsystem.h` | `IsInTargetingMode()`, `BeginTargeting()`, `CancelTargeting()` |
| SkillTree (WalkToCast) | `client/SabriMMO/Source/SabriMMO/UI/SkillTreeSubsystem.cpp` | `WalkToCast` namespace, `GroundAoE` namespace, `StopMovement()` |
| Zone transitions | `client/SabriMMO/Source/SabriMMO/UI/ZoneTransitionSubsystem.h` | `SnapLocationToGround()` static helper |
| ShopNPC | `client/SabriMMO/Source/SabriMMO/ShopNPC.h` | `Interact()`, `InteractionRadius` |
| KafraNPC | `client/SabriMMO/Source/SabriMMO/KafraNPC.h` | `Interact()`, `InteractionRadius` |
| Server combat:attack | `server/src/index.js:3897` | `combat:attack` handler, `auto_attack_started/stopped` emissions |

## Appendix B: Server Socket Event Payloads

### `combat:attack` (Client -> Server)
```json
// Enemy target
{ "targetEnemyId": 2000001 }

// Player target (future PvP)
{ "targetCharacterId": "42" }
```

### `combat:auto_attack_started` (Server -> Client)
```json
{
    "targetId": 2000001,
    "isEnemy": true,
    "attackRange": 200,
    "aspd": 175
}
```

### `combat:auto_attack_stopped` (Server -> Client)
```json
{
    "reason": "Player stopped",
    "oldTargetId": 2000001,
    "oldIsEnemy": true
}
```

### `combat:target_lost` (Server -> Client)
```json
{
    "reason": "Target respawned",
    "isEnemy": true
}
```

### `combat:out_of_range` (Server -> Client)
```json
{
    "targetId": 2000001,
    "isEnemy": true,
    "targetX": 1500.0,
    "targetY": 2300.0,
    "targetZ": 100.0,
    "distance": 450.5,
    "requiredRange": 200
}
```

### `combat:error` (Server -> Client)
```json
{ "message": "You are dead" }
{ "message": "Cannot attack while incapacitated" }
{ "message": "Too heavy to attack! Reduce weight below 90%." }
{ "message": "Enemy not found" }
{ "message": "Enemy is already dead" }
```
