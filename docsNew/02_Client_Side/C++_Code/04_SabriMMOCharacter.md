# ASabriMMOCharacter

**Files**: `Source/SabriMMO/SabriMMOCharacter.h` (97 lines), `SabriMMOCharacter.cpp` (134 lines)  
**Parent**: `ACharacter`  
**UCLASS**: `abstract`  
**Purpose**: Base third-person player character with camera boom, follow camera, and Enhanced Input bindings. Serves as the C++ foundation that Blueprint `BP_MMOCharacter` derives from.

## Log Category

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
```

## Components

| Component | Type | Attachment | Description |
|-----------|------|------------|-------------|
| `CameraBoom` | USpringArmComponent | RootComponent | Camera positioning arm, `bUsePawnControlRotation = true` |
| `FollowCamera` | UCameraComponent | CameraBoom socket | Follow camera, `bUsePawnControlRotation = false` |

## Constructor Defaults

| Setting | Value |
|---------|-------|
| `CapsuleComponent` size | 42 radius, 96 half-height |
| `bUseControllerRotationPitch/Yaw/Roll` | false, false, false |
| `bOrientRotationToMovement` | true |
| `RotationRate` | (0, 500, 0) |
| `JumpZVelocity` | 500 |
| `AirControl` | 0.35 |
| `MaxWalkSpeed` | 500 |
| `MinAnalogWalkSpeed` | 20 |
| `BrakingDecelerationWalking` | 2000 |
| `BrakingDecelerationFalling` | 1500 |
| `CameraBoom.TargetArmLength` | 400 |

## Input Actions (EditAnywhere)

| Property | Type | Binding |
|----------|------|---------|
| `JumpAction` | UInputAction* | Jump / StopJumping |
| `MoveAction` | UInputAction* | Move (Vector2D) |
| `LookAction` | UInputAction* | Look (Vector2D) |
| `MouseLookAction` | UInputAction* | Mouse Look (Vector2D) |

## Functions

### Protected (Input handlers)
- **`Move(FInputActionValue&)`** — Extracts Vector2D → calls `DoMove(X, Y)`
- **`Look(FInputActionValue&)`** — Extracts Vector2D → calls `DoLook(X, Y)`

### Public (BlueprintCallable, virtual)
- **`DoMove(float Right, float Forward)`** — Gets controller yaw rotation, computes forward/right vectors, calls `AddMovementInput`
- **`DoLook(float Yaw, float Pitch)`** — Calls `AddControllerYawInput` / `AddControllerPitchInput`
- **`DoJumpStart()`** — Calls `Jump()`
- **`DoJumpEnd()`** — Calls `StopJumping()`

### Accessors (FORCEINLINE)
- `GetCameraBoom()` → USpringArmComponent*
- `GetFollowCamera()` → UCameraComponent*

## Input Binding Setup

```cpp
void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
    EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Move);
    EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);
    EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);
}
```

---

**Last Updated**: 2026-02-17
