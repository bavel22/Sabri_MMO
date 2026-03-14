# Phase 4: Entity Management Migration Plan

**Status:** NOT STARTED
**Date:** 2026-03-13
**Scope:** Migrate BP_OtherPlayerManager, BP_EnemyManager, BP_OtherPlayerCharacter, and BP_EnemyCharacter from Blueprint to C++.
**Parent:** `00_Master_Migration_Plan.md` -- Phase 4 of 6
**Dependencies:** Phase 3 (Combat Actions) should be complete before this phase begins. UCombatActionSubsystem references entities from these subsystems for attack animations, facing, and death state.

---

## Table of Contents

1. [Overview](#1-overview)
2. [RO Classic Entity Display Behavior](#2-ro-classic-entity-display-behavior)
3. [Current Blueprint Implementation](#3-current-blueprint-implementation)
4. [Server Event Payloads (Wire Format)](#4-server-event-payloads-wire-format)
5. [New C++ Architecture](#5-new-c-architecture)
6. [Actor Classes](#6-actor-classes)
7. [Subsystem Classes](#7-subsystem-classes)
8. [Integration With Existing Systems](#8-integration-with-existing-systems)
9. [Skeletal Mesh and Animation Strategy](#9-skeletal-mesh-and-animation-strategy)
10. [Zone Transition Lifecycle](#10-zone-transition-lifecycle)
11. [Files to Create](#11-files-to-create)
12. [Files to Modify](#12-files-to-modify)
13. [Blueprint Changes](#13-blueprint-changes)
14. [Migration Sequence](#14-migration-sequence)
15. [Conflict Analysis](#15-conflict-analysis)
16. [Testing Checklist](#16-testing-checklist)

---

## 1. Overview

### What Gets Replaced

| Blueprint Asset | C++ Replacement | Role |
|----------------|-----------------|------|
| `BP_OtherPlayerManager` | `UOtherPlayerSubsystem` | Registry + spawn/update/remove remote players |
| `BP_OtherPlayerCharacter` | `AMMORemotePlayer` | Remote player actor with interpolated movement |
| `BP_EnemyManager` | `UEnemySubsystem` | Registry + spawn/update/remove enemies |
| `BP_EnemyCharacter` | `AMMOEnemyActor` | Enemy actor with interpolated movement, death state |

### What Stays

- **WBP_PlayerNameTag**: Name tags stay as-is during Phase 4. They are replaced in Phase 5 by `UNameTagSubsystem`. During Phase 4, the C++ actors will not have WidgetComponent name tags -- names are temporarily invisible until Phase 5 adds the Slate overlay. This is acceptable because Phase 4 and Phase 5 are developed sequentially.
- **Animation Blueprints**: Animation graphs for character and enemy meshes remain in Blueprint. C++ actors trigger animations via `PlayAnimMontage()` or AnimInstance variable setting.
- **Hover indicator visuals**: Currently WidgetComponents on BP actors. Phase 5 or a later pass handles these. During Phase 4, hover indicators are temporarily absent from C++ entities.

---

## 2. RO Classic Entity Display Behavior

This section documents the expected player-facing behavior that must be preserved exactly.

### 2.1 Other Players

- **Appear** when entering the same zone. The server sends `player:moved` for all players already in the zone when a new player arrives (via `zone:ready`).
- **Position interpolated** between server updates. The server broadcasts `player:moved` at approximately 30Hz (every ~33ms) when a player is moving, driven by the client's `PositionBroadcastSubsystem` emitting `player:position` at 30Hz.
- **Name displayed** above head in white text, always visible (billboard facing camera).
- **Guild name** below character name if the player is in a guild (not yet implemented -- future-proof the data structure).
- **Party members** have green-tinted name text (not yet implemented -- future-proof the data structure).
- **Health bar** only visible for party members (not yet implemented -- `WorldHealthBarSubsystem` already tracks remote player health via `combat:health_update`).
- **Attack animations** play when they attack. Currently driven by `combat:damage` events where the player is the attacker. The remote player's character rotates to face the target and plays the attack montage.
- **Death**: Character falls (death animation), grayed out. Currently `combat:death` hides or disables the actor.
- **Disconnect**: Character actor is destroyed immediately when `player:left` arrives.

### 2.2 Enemies / Monsters

- **Spawn** at predefined spawn points. The server sends `enemy:spawn` for each living enemy in the zone when a player joins.
- **Idle wandering**: Server AI moves enemies randomly within their spawn radius, broadcasting `enemy:move` with `isMoving: true`.
- **Stop wandering**: Server broadcasts `enemy:move` with `isMoving: false` when the enemy reaches its wander destination.
- **Aggro chase**: When an enemy targets a player, the server broadcasts `enemy:move` at a throttled rate (`MOVE_BROADCAST_MS`) as the enemy chases. The client interpolates between positions.
- **Attack**: Server broadcasts `enemy:attack` with `{ enemyId, targetId, attackMotion }` for visual feedback. The client plays the attack animation on the enemy actor.
- **Name + Level** displayed above head as "EnemyName Lv.X" in white text.
- **HP bar** appears when the enemy is in combat (damaged). `WorldHealthBarSubsystem` already handles this via `enemy:health_update` with `inCombat: true`.
- **Death**: Play death animation, disable collision, hide mesh after a delay. The server broadcasts `enemy:death`. The actor is NOT destroyed -- it is reused on respawn (the server sends `enemy:spawn` again with the same `enemyId` for the respawned enemy).
- **Respawn**: Server sends `enemy:spawn` for the same `enemyId`. If the actor still exists in the map, it is re-initialized. If it was destroyed, a new actor is spawned.
- **Size**: Varies by monster (small/medium/large). Currently not visually differentiated -- all enemies use the same mesh. Template-based sizing is a future enhancement.

### 2.3 Entity Interpolation (Both Player and Enemy)

The interpolation model is identical for remote players and enemies:

1. Server sends a position update with `(x, y, z)` coordinates.
2. Client stores the received position as `TargetPosition`.
3. Each tick, the client calculates a flat XY direction from current position to `TargetPosition` (Z is ignored for direction calculation -- floor snap handles Z independently).
4. `AddMovementInput(Direction, 1.0f)` moves the character at its configured `MaxWalkSpeed`.
5. When the 2D distance to `TargetPosition` is less than 10 UE units, movement stops (`bIsMoving = false`).
6. `UOtherCharacterMovementComponent` runs a per-tick floor-snap via downward line trace, correcting Z when the mismatch exceeds 50 UE units.

**Key parameters:**
- Arrival threshold: **10 UE units** (flat XY distance)
- Floor-snap threshold: **50 UE units** (Z mismatch)
- Floor-snap trace range: **200 units above, 2000 units below** actor position
- Movement direction: **flat XY only** (`Direction.Z = 0`)

---

## 3. Current Blueprint Implementation

### 3.1 BP_OtherPlayerManager

- **Type**: Actor placed in every game level (singleton per level).
- **Storage**: `TMap<int, BP_OtherPlayerCharacter>` named `OtherPlayers`.
- **Functions**:
  - `SpawnOrUpdatePlayer(CharId, Name, X, Y, Z, Health, MaxHealth)`: If `CharId` exists in map, update position. Otherwise, spawn a new `BP_OtherPlayerCharacter` at (X, Y, Z), store in map.
  - `RemovePlayer(CharId)`: Destroy the actor, remove from map.
  - `GetEnemyActor(EnemyId)`: Lookup by ID, returns actor reference + bool found.
- **Callers**: `BP_SocketManager.OnPlayerMoved`, `BP_SocketManager.OnPlayerLeft` (both bridged from `MultiplayerEventSubsystem`).
- **Level Blueprint setup**: The Level Blueprint spawns `BP_OtherPlayerManager` and stores a reference on `BP_SocketManager` as `OtherPlayerManagerRef`.

### 3.2 BP_OtherPlayerCharacter

- **Type**: `ACharacter` subclass (Blueprint).
- **Movement component**: `UOtherCharacterMovementComponent` (C++ -- already exists at `client/SabriMMO/Source/SabriMMO/OtherCharacterMovementComponent.h/.cpp`). Provides per-tick floor snap.
- **Components**:
  - `NameTagWidget`: `UWidgetComponent` displaying `WBP_PlayerNameTag` (floating name above head).
  - `HoverOverIndicator`: Visual indicator when mouse hovers over this player.
  - `HealthBarWidget`: HP bar (visible only for party members -- currently unused).
- **Variables**: `PlayerName` (FString), `CharacterId` (int), `TargetPosition` (FVector), `bIsMoving` (bool), `FlatDirection` (FVector).
- **Tick logic**: If `bIsMoving`, calculate flat direction to `TargetPosition`, call `AddMovementInput(Direction, 1.0f)`. Stop when 2D distance < 10 units.
- **Interfaces**: Implements `BPI_Damageable` (Blueprint Interface) with `ReceiveDamageVisual` (rotate to face attacker) and `UpdateHealthDisplay`.
- **Cursor events**: `ActorBeginCursorOver` / `ActorEndCursorOver` toggle `HoverOverIndicator` visibility.

### 3.3 BP_EnemyManager

- **Type**: Actor placed in every game level (singleton per level).
- **Storage**: `TMap<int, BP_EnemyCharacter>` named `EnemyMap`.
- **Functions**:
  - `SpawnOrUpdateEnemy(EnemyId, TemplateId, Name, Level, Health, MaxHealth, X, Y, Z, IsMoving)`: If `EnemyId` exists in map, update position and health. Otherwise, spawn a new `BP_EnemyCharacter`, initialize, and store.
  - `RemoveEnemy(EnemyId)`: Destroy actor, remove from map.
  - `GetEnemyActor(EnemyId)`: Lookup by ID. Returns `(AActor*, bool WasFound)`. **This function is called by `MultiplayerEventSubsystem::HandleEnemyAttack()` via `ProcessEvent` to find enemy actors for attack animations.**
- **Callers**: `BP_SocketManager.OnEnemySpawn`, `BP_SocketManager.OnEnemyMove`, `BP_SocketManager.OnEnemyDeath`, `BP_SocketManager.OnEnemyHealthUpdate` (all bridged from `MultiplayerEventSubsystem`).
- **Level Blueprint setup**: Level Blueprint spawns `BP_EnemyManager` and stores a reference on `BP_SocketManager` as `EnemyManagerRef`. `MultiplayerEventSubsystem` also caches a reference via `FindEnemyManagerActor()`.

### 3.4 BP_EnemyCharacter

- **Type**: `ACharacter` subclass (Blueprint).
- **Components**:
  - `NameTagWidget`: `UWidgetComponent` displaying name + level.
  - `HoverOverIndicator`: Visual indicator on hover.
  - `HealthBarWidget`: HP bar (visibility toggled by `bInCombat`).
- **Variables**: `EnemyId` (int), `EnemyName` (FString), `EnemyLevel` (int), `Health` (float), `MaxHealth` (float), `TargetPosition` (FVector), `bIsMoving` (bool), `bIsDead` (bool), `bIsActiveTarget` (bool).
- **Tick logic**: Same interpolation as `BP_OtherPlayerCharacter`. If `bIsMoving`, calculate flat direction to `TargetPosition`, `AddMovementInput`, stop at 10 units.
- **Functions**:
  - `InitializeEnemy(EnemyId, Name, Level, Health, MaxHealth)`: Set initial state.
  - `UpdateEnemyHealth(Health, MaxHealth)`: Update HP values.
  - `OnEnemyDeath()`: Disable collision, hide skeletal mesh, play death animation.
  - `PlayAttackAnimation()`: Play attack montage. **Called by `MultiplayerEventSubsystem::HandleEnemyAttack()` via `ProcessEvent`.**
- **Cursor events**: `ActorBeginCursorOver` / `ActorEndCursorOver` toggle `HoverOverIndicator`.
- **Interfaces**: Implements `BPI_Damageable` (Blueprint Interface).

### 3.5 Socket Event Flow (Current)

```
Server broadcasts event
        |
        v
FSocketIONative.OnEvent()
        |
        v
USocketEventRouter dispatches to all registered handlers
        |
        +---> MultiplayerEventSubsystem (registered for player:moved, player:left,
        |     enemy:spawn, enemy:move, enemy:death, enemy:health_update, enemy:attack)
        |         |
        |         +---> ForwardToBPHandler("OnPlayerMoved", data)
        |         |        -> BP_SocketManager.OnPlayerMoved (ProcessEvent)
        |         |             -> BP_OtherPlayerManager.SpawnOrUpdatePlayer
        |         |
        |         +---> ForwardToBPHandler("OnEnemySpawn", data)
        |         |        -> BP_SocketManager.OnEnemySpawn (ProcessEvent)
        |         |             -> BP_EnemyManager.SpawnOrUpdateEnemy
        |         |
        |         +---> HandleEnemyAttack(data) [C++ direct handler]
        |                  -> BP_EnemyManager.GetEnemyActor (ProcessEvent)
        |                  -> BP_EnemyCharacter.PlayAttackAnimation (ProcessEvent)
        |
        +---> WorldHealthBarSubsystem (also registered for enemy:spawn, enemy:move,
              enemy:death, enemy:health_update -- independent data tracking)
```

---

## 4. Server Event Payloads (Wire Format)

All payloads are the actual server-emitted JSON. The C++ subsystems must parse these exact field names.

### 4.1 `player:moved`

Broadcast by server when a player moves, or when sending existing players to a newly joined player.

```json
{
    "characterId": 42,
    "characterName": "Sabri",
    "x": 1500.0,
    "y": -2300.0,
    "z": 300.0,
    "health": 450,
    "maxHealth": 500,
    "timestamp": 1710345600000
}
```

**Notes:**
- `characterId` is the unique player identifier. The local player's own ID must be filtered out.
- `characterName` is the display name shown above the character.
- `health` and `maxHealth` are included for party member HP display (future) and `WorldHealthBarSubsystem`.
- `timestamp` is server Unix timestamp in milliseconds.
- The server does NOT send `jobClass`, `level`, `guildName`, `hairStyle`, `hairColor`, or `gender` in this event. Those would need a separate event or payload extension if needed for visual customization.

### 4.2 `player:left`

Broadcast when a player disconnects or changes zone.

```json
{
    "characterId": 42,
    "characterName": "Sabri",
    "reason": "zone_change"
}
```

**Notes:**
- `reason` can be `"zone_change"`, `"kafra_teleport"`, `"respawn"`, `"butterfly_wing"`, or absent (disconnect).
- The subsystem should destroy the remote player actor regardless of reason.

### 4.3 `enemy:spawn`

Sent for each living enemy when a player joins a zone, and when an enemy respawns.

```json
{
    "enemyId": 1001,
    "templateId": 1002,
    "name": "Poring",
    "level": 1,
    "health": 50,
    "maxHealth": 50,
    "x": 3200.0,
    "y": -1800.0,
    "z": 250.0
}
```

**Notes:**
- `enemyId` is the unique runtime instance ID (server-generated, not the template ID).
- `templateId` is the RO monster template ID (e.g., 1002 = Poring). Used for selecting the correct mesh/animation. Currently all enemies use the same placeholder mesh, so this field is stored but not yet visually differentiated.
- `name` is the display name from the monster template.
- `level` is the monster level for display.
- `health`/`maxHealth` represent current state. A respawning enemy arrives with `health === maxHealth`.
- No `isMoving` field on spawn -- enemies spawn idle.

### 4.4 `enemy:move`

Broadcast during AI wander/chase movement and when an enemy stops.

```json
{
    "enemyId": 1001,
    "x": 3250.0,
    "y": -1750.0,
    "z": 250.0,
    "targetX": 3400.0,
    "targetY": -1600.0,
    "isMoving": true
}
```

When the enemy stops:

```json
{
    "enemyId": 1001,
    "x": 3400.0,
    "y": -1600.0,
    "z": 250.0,
    "isMoving": false
}
```

**Notes:**
- `isMoving` determines whether the client should interpolate toward the position or teleport/stop.
- `targetX`/`targetY` are sometimes included (during chase) to indicate the enemy's intended destination. Not always present.
- Movement broadcasts are throttled on the server side (`ENEMY_AI.MOVE_BROADCAST_MS`).

### 4.5 `enemy:moved` (Knockback Only)

A separate event used ONLY for knockback displacement. Not part of normal movement.

```json
{
    "enemyId": 1001,
    "x": 3500.0,
    "y": -1400.0,
    "z": 250.0,
    "isKnockback": true
}
```

**Notes:**
- `isKnockback: true` always present on this event.
- The client should teleport the enemy to this position (not interpolate) since knockback is instantaneous.
- This event is currently NOT handled by the client (no handler registered). The new `UEnemySubsystem` should register for it.

### 4.6 `enemy:death`

Broadcast when an enemy is killed.

```json
{
    "enemyId": 1001,
    "enemyName": "Poring",
    "killerId": 42,
    "killerName": "Sabri",
    "isEnemy": true,
    "isDead": true,
    "baseExp": 4,
    "jobExp": 1,
    "exp": 4,
    "timestamp": 1710345600000
}
```

**Notes:**
- `killerId`/`killerName` identify who killed it (for EXP display, etc.).
- The actor should NOT be destroyed from the map -- it should be disabled (hidden mesh, no collision) and reused on the next `enemy:spawn` with the same `enemyId`.
- EXP fields are consumed by other systems (already handled).

### 4.7 `enemy:health_update`

Broadcast when enemy health changes (damage, regen, status effect drain).

```json
{
    "enemyId": 1001,
    "health": 35,
    "maxHealth": 50,
    "inCombat": true
}
```

**Notes:**
- `inCombat` controls HP bar visibility in `WorldHealthBarSubsystem`.
- Already handled by `WorldHealthBarSubsystem` independently. `UEnemySubsystem` should also update its local HP state for death detection.

### 4.8 `enemy:attack`

Broadcast when an enemy performs an attack (visual feedback only -- damage comes via `combat:damage`).

```json
{
    "enemyId": 1001,
    "targetId": 42,
    "attackMotion": 450
}
```

**Notes:**
- `attackMotion` is the RO attack motion delay in milliseconds. Currently used to time the attack animation duration.
- Currently handled by `MultiplayerEventSubsystem::HandleEnemyAttack()` which finds the enemy actor via `BP_EnemyManager.GetEnemyActor()` ProcessEvent call and triggers `PlayAttackAnimation()`. This must be updated to use `UEnemySubsystem` instead.

---

## 5. New C++ Architecture

### 5.1 Class Diagram

```
USocketEventRouter (on GameInstance, survives level transitions)
        |
        | RegisterHandler()
        |
        +---> UOtherPlayerSubsystem (UWorldSubsystem)
        |        |
        |        | Spawns/manages via TMap<int32, TWeakObjectPtr<AMMORemotePlayer>>
        |        v
        |     AMMORemotePlayer (ACharacter subclass)
        |        - UOtherCharacterMovementComponent (floor snap)
        |        - Tick: interpolation toward TargetPosition
        |        - No input, no camera, no UI components
        |
        +---> UEnemySubsystem (UWorldSubsystem)
                 |
                 | Spawns/manages via TMap<int32, TWeakObjectPtr<AMMOEnemyActor>>
                 v
              AMMOEnemyActor (ACharacter subclass)
                 - UCharacterMovementComponent (standard)
                 - Tick: interpolation toward TargetPosition
                 - Death state: disable collision, hide mesh
                 - Reused on respawn (re-initialize, not re-spawn)

WorldHealthBarSubsystem ---- reads AMMOEnemyActor positions directly
                             (eliminates heuristic actor caching)

UCombatActionSubsystem ----- calls UEnemySubsystem::GetEnemy() for target references
                             calls AMMOEnemyActor::PlayAttackAnimation() directly
```

### 5.2 Data Flow After Migration

```
Server broadcasts "enemy:spawn"
        |
        v
USocketEventRouter dispatches to ALL registered handlers:
        |
        +---> UEnemySubsystem::HandleEnemySpawn()
        |        -> SpawnOrUpdateEnemy() -> creates/reinitializes AMMOEnemyActor
        |
        +---> WorldHealthBarSubsystem::HandleEnemySpawn()
        |        -> Updates EnemyHealthMap (unchanged from today)
        |
        +---> [MultiplayerEventSubsystem bridge REMOVED for this event]

Server broadcasts "enemy:attack"
        |
        v
USocketEventRouter dispatches:
        |
        +---> UEnemySubsystem::HandleEnemyAttack()
                 -> GetEnemy(enemyId) -> AMMOEnemyActor::PlayAttackAnimation()
                 (replaces MultiplayerEventSubsystem::HandleEnemyAttack ProcessEvent chain)
```

---

## 6. Actor Classes

### 6.1 AMMORemotePlayer

Replaces `BP_OtherPlayerCharacter`.

**File locations:**
- `client/SabriMMO/Source/SabriMMO/MMORemotePlayer.h`
- `client/SabriMMO/Source/SabriMMO/MMORemotePlayer.cpp`

**Header:**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MMORemotePlayer.generated.h"

UCLASS()
class SABRIMMO_API AMMORemotePlayer : public ACharacter
{
    GENERATED_BODY()

public:
    AMMORemotePlayer(const FObjectInitializer& ObjectInitializer);

    // ---- Identity ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    int32 CharacterId = 0;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    FString PlayerName;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    FString JobClass;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    FString GuildName;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    int32 Level = 1;

    // ---- Health (for party member display, future) ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    float Health = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    float MaxHealth = 100.f;

    // ---- Appearance (for future visual customization) ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    int32 HairStyle = 1;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    int32 HairColor = 0;

    UPROPERTY(VisibleAnywhere, Category = "MMO Remote Player")
    FString Gender = TEXT("male");

    // ---- Movement interpolation ----
    void SetTargetPosition(const FVector& NewTarget);
    bool IsMoving() const { return bIsMoving; }

    // ---- Visual feedback ----
    void PlayAttackAnimation();
    void PlayDeathAnimation();
    void PlayHitReaction();

    // ---- State ----
    bool IsDead() const { return bIsDead; }
    void SetDead(bool bDead);

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    FVector TargetPosition = FVector::ZeroVector;
    bool bIsMoving = false;
    bool bIsDead = false;

    // Arrival threshold (flat XY distance) -- matches current BP behavior
    static constexpr float ArrivalThreshold = 10.f;
};
```

**Implementation key points:**

**Constructor:**
```cpp
AMMORemotePlayer::AMMORemotePlayer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UOtherCharacterMovementComponent>(
        ACharacter::CharacterMovementComponentName))
{
    // Remote players have no input
    AutoPossessAI = EAutoPossessAI::Disabled;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;

    // Enable cursor interaction for targeting (hover detection)
    // Collision: respond to ECC_Visibility for cursor traces
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
}
```

The constructor uses `SetDefaultSubobjectClass` to swap in `UOtherCharacterMovementComponent` as the character movement component. This is the existing C++ class at `OtherCharacterMovementComponent.h/.cpp` that provides per-tick floor snap via downward line trace.

**Tick interpolation:**
```cpp
void AMMORemotePlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bIsMoving) return;

    const FVector CurrentLocation = GetActorLocation();
    FVector Direction = TargetPosition - CurrentLocation;
    Direction.Z = 0.f; // Flat XY only -- Z handled by floor snap

    const float Distance = Direction.Size2D();

    if (Distance > ArrivalThreshold)
    {
        Direction.Normalize();
        AddMovementInput(Direction, 1.0f);
    }
    else
    {
        bIsMoving = false;
    }
}
```

**SetTargetPosition:**
```cpp
void AMMORemotePlayer::SetTargetPosition(const FVector& NewTarget)
{
    TargetPosition = NewTarget;

    // Only start moving if the target is meaningfully different from current position
    const float Distance = FVector::Dist2D(GetActorLocation(), NewTarget);
    bIsMoving = (Distance > ArrivalThreshold);
}
```

### 6.2 AMMOEnemyActor

Replaces `BP_EnemyCharacter`.

**File locations:**
- `client/SabriMMO/Source/SabriMMO/MMOEnemyActor.h`
- `client/SabriMMO/Source/SabriMMO/MMOEnemyActor.cpp`

**Header:**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MMOEnemyActor.generated.h"

UCLASS()
class SABRIMMO_API AMMOEnemyActor : public ACharacter
{
    GENERATED_BODY()

public:
    AMMOEnemyActor();

    // ---- Identity ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    int32 EnemyId = 0;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    int32 TemplateId = 0;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    FString EnemyName;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    int32 EnemyLevel = 1;

    // ---- Health ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    float Health = 100.f;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    float MaxHealth = 100.f;

    // ---- State ----
    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    bool bIsDead = false;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    bool bIsActiveTarget = false;

    UPROPERTY(VisibleAnywhere, Category = "MMO Enemy")
    bool bInCombat = false;

    // ---- Initialization ----
    void Initialize(int32 InEnemyId, int32 InTemplateId, const FString& InName,
                    int32 InLevel, float InHealth, float InMaxHealth);

    // ---- Position ----
    void SetTargetPosition(const FVector& NewTarget, bool bShouldMove);
    void TeleportToPosition(const FVector& NewPosition); // For knockback
    bool IsMoving() const { return bIsMoving; }

    // ---- Health updates ----
    void UpdateHealth(float NewHealth, float NewMaxHealth, bool InCombat);

    // ---- Death / Respawn ----
    void OnDeath();
    void OnRespawn(int32 InTemplateId, const FString& InName, int32 InLevel,
                   float InHealth, float InMaxHealth);

    // ---- Visual feedback ----
    void PlayAttackAnimation();
    void PlayDeathAnimation();
    void PlayHitReaction();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    FVector TargetPosition = FVector::ZeroVector;
    bool bIsMoving = false;

    // Corpse timer -- hide mesh after delay on death
    FTimerHandle CorpseTimerHandle;
    void HideCorpse();

    // Re-enable actor after respawn
    void EnableActor();

    static constexpr float ArrivalThreshold = 10.f;
    static constexpr float CorpseDisplayTime = 3.0f; // seconds before corpse fades/hides
};
```

**Implementation key points:**

**Constructor:**
```cpp
AMMOEnemyActor::AMMOEnemyActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // No AI possession needed -- movement is driven by server events
    AutoPossessAI = EAutoPossessAI::Disabled;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;

    // Collision: respond to ECC_Visibility for cursor traces (click targeting)
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
}
```

**Initialize (used on first spawn):**
```cpp
void AMMOEnemyActor::Initialize(int32 InEnemyId, int32 InTemplateId,
    const FString& InName, int32 InLevel, float InHealth, float InMaxHealth)
{
    EnemyId = InEnemyId;
    TemplateId = InTemplateId;
    EnemyName = InName;
    EnemyLevel = InLevel;
    Health = InHealth;
    MaxHealth = InMaxHealth;
    bIsDead = false;
    bInCombat = false;
    bIsActiveTarget = false;
    bIsMoving = false;

    EnableActor();
}
```

**OnDeath:**
```cpp
void AMMOEnemyActor::OnDeath()
{
    bIsDead = true;
    bIsMoving = false;
    bInCombat = false;

    // Disable collision immediately so clicks pass through
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    PlayDeathAnimation();

    // Start corpse timer -- hide mesh after delay
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            CorpseTimerHandle, this, &AMMOEnemyActor::HideCorpse,
            CorpseDisplayTime, false);
    }
}
```

**OnRespawn (reuse existing actor):**
```cpp
void AMMOEnemyActor::OnRespawn(int32 InTemplateId, const FString& InName,
    int32 InLevel, float InHealth, float InMaxHealth)
{
    // Clear corpse timer if still pending
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CorpseTimerHandle);
    }

    TemplateId = InTemplateId;
    EnemyName = InName;
    EnemyLevel = InLevel;
    Health = InHealth;
    MaxHealth = InMaxHealth;
    bIsDead = false;
    bInCombat = false;
    bIsActiveTarget = false;
    bIsMoving = false;

    EnableActor();
}
```

**EnableActor / HideCorpse:**
```cpp
void AMMOEnemyActor::EnableActor()
{
    SetActorHiddenInGame(false);

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        Mesh->SetVisibility(true);
    }
}

void AMMOEnemyActor::HideCorpse()
{
    SetActorHiddenInGame(true);

    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        Mesh->SetVisibility(false);
    }
}
```

**TeleportToPosition (knockback):**
```cpp
void AMMOEnemyActor::TeleportToPosition(const FVector& NewPosition)
{
    SetActorLocation(NewPosition, false, nullptr, ETeleportType::TeleportPhysics);
    TargetPosition = NewPosition;
    bIsMoving = false;
}
```

**Tick interpolation:**
```cpp
void AMMOEnemyActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead || !bIsMoving) return;

    const FVector CurrentLocation = GetActorLocation();
    FVector Direction = TargetPosition - CurrentLocation;
    Direction.Z = 0.f;

    const float Distance = Direction.Size2D();

    if (Distance > ArrivalThreshold)
    {
        Direction.Normalize();
        AddMovementInput(Direction, 1.0f);
    }
    else
    {
        bIsMoving = false;
    }
}
```

---

## 7. Subsystem Classes

### 7.1 UOtherPlayerSubsystem

Replaces `BP_OtherPlayerManager` + `BP_SocketManager` player event handlers.

**File locations:**
- `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.h`
- `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.cpp`

**Header:**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "OtherPlayerSubsystem.generated.h"

class AMMORemotePlayer;

UCLASS()
class SABRIMMO_API UOtherPlayerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public API ----
    AMMORemotePlayer* GetPlayer(int32 CharacterId) const;
    AMMORemotePlayer* SpawnOrUpdatePlayer(int32 CharacterId, const FString& Name,
        float X, float Y, float Z, float InHealth, float InMaxHealth);
    void RemovePlayer(int32 CharacterId);
    void RemoveAllPlayers();
    int32 GetPlayerCount() const { return Players.Num(); }

private:
    // ---- Socket event handlers ----
    void HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data);
    void HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data);

    // ---- Entity registry ----
    TMap<int32, TWeakObjectPtr<AMMORemotePlayer>> Players;

    // ---- Local player filter ----
    int32 LocalCharacterId = 0;

    // ---- Spawn class ----
    TSubclassOf<AMMORemotePlayer> RemotePlayerClass;
};
```

**Implementation key points:**

**ShouldCreateSubsystem:**
```cpp
bool UOtherPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}
```

**OnWorldBeginPlay:**
```cpp
void UOtherPlayerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
    if (!GI || !GI->IsSocketConnected()) return;

    // Cache local player ID for filtering
    LocalCharacterId = GI->GetSelectedCharacter().CharacterId;

    // Resolve spawn class (Blueprint subclass or direct C++ class)
    // Option A: Use the C++ class directly
    RemotePlayerClass = AMMORemotePlayer::StaticClass();
    // Option B (recommended): Load a Blueprint subclass that sets mesh/anim
    // static ConstructorHelpers is not available here, so use LoadClass or config

    // Register socket event handlers
    USocketEventRouter* Router = GI->GetEventRouter();
    if (!Router) return;

    Router->RegisterHandler(TEXT("player:moved"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandlePlayerMoved(D); });
    Router->RegisterHandler(TEXT("player:left"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandlePlayerLeft(D); });
}
```

**HandlePlayerMoved (replaces ~38-node BP function):**
```cpp
void UOtherPlayerSubsystem::HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    // Parse fields
    double CharIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
    int32 CharId = (int32)CharIdD;

    // Filter out local player
    if (CharId == LocalCharacterId) return;

    FString Name;
    Obj->TryGetStringField(TEXT("characterName"), Name);

    double X = 0, Y = 0, Z = 0;
    Obj->TryGetNumberField(TEXT("x"), X);
    Obj->TryGetNumberField(TEXT("y"), Y);
    Obj->TryGetNumberField(TEXT("z"), Z);

    double H = 0, MH = 0;
    Obj->TryGetNumberField(TEXT("health"), H);
    Obj->TryGetNumberField(TEXT("maxHealth"), MH);

    SpawnOrUpdatePlayer(CharId, Name, (float)X, (float)Y, (float)Z, (float)H, (float)MH);
}
```

**SpawnOrUpdatePlayer:**
```cpp
AMMORemotePlayer* UOtherPlayerSubsystem::SpawnOrUpdatePlayer(
    int32 CharacterId, const FString& Name,
    float X, float Y, float Z, float InHealth, float InMaxHealth)
{
    UWorld* World = GetWorld();
    if (!World || !RemotePlayerClass) return nullptr;

    // Check if player already exists
    TWeakObjectPtr<AMMORemotePlayer>* Existing = Players.Find(CharacterId);
    if (Existing && Existing->IsValid())
    {
        AMMORemotePlayer* Player = Existing->Get();
        Player->SetTargetPosition(FVector(X, Y, Z));
        Player->Health = InHealth;
        Player->MaxHealth = InMaxHealth;
        return Player;
    }

    // Spawn new remote player
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMMORemotePlayer* NewPlayer = World->SpawnActor<AMMORemotePlayer>(
        RemotePlayerClass, FVector(X, Y, Z), FRotator::ZeroRotator, SpawnParams);

    if (!NewPlayer) return nullptr;

    NewPlayer->CharacterId = CharacterId;
    NewPlayer->PlayerName = Name;
    NewPlayer->Health = InHealth;
    NewPlayer->MaxHealth = InMaxHealth;

    Players.Add(CharacterId, NewPlayer);

    return NewPlayer;
}
```

**HandlePlayerLeft:**
```cpp
void UOtherPlayerSubsystem::HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double CharIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
    int32 CharId = (int32)CharIdD;

    RemovePlayer(CharId);
}
```

**RemovePlayer:**
```cpp
void UOtherPlayerSubsystem::RemovePlayer(int32 CharacterId)
{
    TWeakObjectPtr<AMMORemotePlayer>* Found = Players.Find(CharacterId);
    if (Found && Found->IsValid())
    {
        Found->Get()->Destroy();
    }
    Players.Remove(CharacterId);
}
```

**RemoveAllPlayers (called from Deinitialize):**
```cpp
void UOtherPlayerSubsystem::RemoveAllPlayers()
{
    for (auto& Pair : Players)
    {
        if (Pair.Value.IsValid())
        {
            Pair.Value->Destroy();
        }
    }
    Players.Empty();
}
```

**Deinitialize:**
```cpp
void UOtherPlayerSubsystem::Deinitialize()
{
    RemoveAllPlayers();

    if (UWorld* World = GetWorld())
    {
        if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
        {
            if (USocketEventRouter* Router = GI->GetEventRouter())
            {
                Router->UnregisterAllForOwner(this);
            }
        }
    }

    Super::Deinitialize();
}
```

### 7.2 UEnemySubsystem

Replaces `BP_EnemyManager` + `BP_SocketManager` enemy event handlers + `MultiplayerEventSubsystem::HandleEnemyAttack()`.

**File locations:**
- `client/SabriMMO/Source/SabriMMO/UI/EnemySubsystem.h`
- `client/SabriMMO/Source/SabriMMO/UI/EnemySubsystem.cpp`

**Header:**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "EnemySubsystem.generated.h"

class AMMOEnemyActor;

UCLASS()
class SABRIMMO_API UEnemySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Public API ----
    AMMOEnemyActor* GetEnemy(int32 EnemyId) const;
    AMMOEnemyActor* SpawnOrUpdateEnemy(int32 EnemyId, int32 TemplateId,
        const FString& Name, int32 Level, float Health, float MaxHealth,
        float X, float Y, float Z, bool bIsMoving);
    void RemoveEnemy(int32 EnemyId);
    void RemoveAllEnemies();
    int32 GetEnemyCount() const { return Enemies.Num(); }

    // ---- Iteration (for other subsystems) ----
    const TMap<int32, TWeakObjectPtr<AMMOEnemyActor>>& GetEnemyMap() const { return Enemies; }

private:
    // ---- Socket event handlers ----
    void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyMoved(const TSharedPtr<FJsonValue>& Data);  // knockback
    void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);
    void HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data);

    // ---- Entity registry ----
    TMap<int32, TWeakObjectPtr<AMMOEnemyActor>> Enemies;

    // ---- Spawn class ----
    TSubclassOf<AMMOEnemyActor> EnemyActorClass;
};
```

**Implementation key points:**

**OnWorldBeginPlay:**
```cpp
void UEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
    if (!GI || !GI->IsSocketConnected()) return;

    // Resolve spawn class
    EnemyActorClass = AMMOEnemyActor::StaticClass();

    USocketEventRouter* Router = GI->GetEventRouter();
    if (!Router) return;

    // Register for all enemy events
    Router->RegisterHandler(TEXT("enemy:spawn"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemySpawn(D); });
    Router->RegisterHandler(TEXT("enemy:move"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyMove(D); });
    Router->RegisterHandler(TEXT("enemy:moved"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyMoved(D); });
    Router->RegisterHandler(TEXT("enemy:death"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyDeath(D); });
    Router->RegisterHandler(TEXT("enemy:health_update"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyHealthUpdate(D); });
    Router->RegisterHandler(TEXT("enemy:attack"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyAttack(D); });
}
```

**HandleEnemySpawn (replaces ~41-node BP function):**
```cpp
void UEnemySubsystem::HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    double TemplateIdD = 0;
    Obj->TryGetNumberField(TEXT("templateId"), TemplateIdD);
    int32 TemplateId = (int32)TemplateIdD;

    FString Name;
    Obj->TryGetStringField(TEXT("name"), Name);

    double LevelD = 1;
    Obj->TryGetNumberField(TEXT("level"), LevelD);
    int32 Level = (int32)LevelD;

    double H = 0, MH = 0;
    Obj->TryGetNumberField(TEXT("health"), H);
    Obj->TryGetNumberField(TEXT("maxHealth"), MH);

    double X = 0, Y = 0, Z = 0;
    Obj->TryGetNumberField(TEXT("x"), X);
    Obj->TryGetNumberField(TEXT("y"), Y);
    Obj->TryGetNumberField(TEXT("z"), Z);

    // Check if this enemyId already exists (respawn case)
    TWeakObjectPtr<AMMOEnemyActor>* Existing = Enemies.Find(EnemyId);
    if (Existing && Existing->IsValid())
    {
        AMMOEnemyActor* Enemy = Existing->Get();
        Enemy->SetActorLocation(FVector((float)X, (float)Y, (float)Z),
            false, nullptr, ETeleportType::TeleportPhysics);
        Enemy->OnRespawn(TemplateId, Name, Level, (float)H, (float)MH);
        return;
    }

    // Spawn new enemy
    SpawnOrUpdateEnemy(EnemyId, TemplateId, Name, Level,
        (float)H, (float)MH, (float)X, (float)Y, (float)Z, false);
}
```

**HandleEnemyMove:**
```cpp
void UEnemySubsystem::HandleEnemyMove(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    TWeakObjectPtr<AMMOEnemyActor>* Found = Enemies.Find(EnemyId);
    if (!Found || !Found->IsValid()) return;
    AMMOEnemyActor* Enemy = Found->Get();

    double X = 0, Y = 0, Z = 0;
    Obj->TryGetNumberField(TEXT("x"), X);
    Obj->TryGetNumberField(TEXT("y"), Y);
    Obj->TryGetNumberField(TEXT("z"), Z);

    bool bMoving = false;
    Obj->TryGetBoolField(TEXT("isMoving"), bMoving);

    Enemy->SetTargetPosition(FVector((float)X, (float)Y, (float)Z), bMoving);
}
```

**HandleEnemyMoved (knockback -- currently unhandled by client):**
```cpp
void UEnemySubsystem::HandleEnemyMoved(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    TWeakObjectPtr<AMMOEnemyActor>* Found = Enemies.Find(EnemyId);
    if (!Found || !Found->IsValid()) return;
    AMMOEnemyActor* Enemy = Found->Get();

    double X = 0, Y = 0, Z = 0;
    Obj->TryGetNumberField(TEXT("x"), X);
    Obj->TryGetNumberField(TEXT("y"), Y);
    Obj->TryGetNumberField(TEXT("z"), Z);

    // Knockback: teleport to new position (instantaneous, not interpolated)
    Enemy->TeleportToPosition(FVector((float)X, (float)Y, (float)Z));
}
```

**HandleEnemyDeath:**
```cpp
void UEnemySubsystem::HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    TWeakObjectPtr<AMMOEnemyActor>* Found = Enemies.Find(EnemyId);
    if (!Found || !Found->IsValid()) return;

    Found->Get()->OnDeath();

    // Do NOT remove from map -- actor is reused on next enemy:spawn for same enemyId
}
```

**HandleEnemyHealthUpdate:**
```cpp
void UEnemySubsystem::HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    TWeakObjectPtr<AMMOEnemyActor>* Found = Enemies.Find(EnemyId);
    if (!Found || !Found->IsValid()) return;

    double H = 0, MH = 0;
    Obj->TryGetNumberField(TEXT("health"), H);
    Obj->TryGetNumberField(TEXT("maxHealth"), MH);

    bool bCombat = false;
    Obj->TryGetBoolField(TEXT("inCombat"), bCombat);

    Found->Get()->UpdateHealth((float)H, (float)MH, bCombat);
}
```

**HandleEnemyAttack (replaces MultiplayerEventSubsystem::HandleEnemyAttack):**
```cpp
void UEnemySubsystem::HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    double EnemyIdD = 0;
    if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
    int32 EnemyId = (int32)EnemyIdD;

    TWeakObjectPtr<AMMOEnemyActor>* Found = Enemies.Find(EnemyId);
    if (!Found || !Found->IsValid()) return;

    Found->Get()->PlayAttackAnimation();
}
```

This replaces the entire `MultiplayerEventSubsystem::HandleEnemyAttack()` method which currently does:
1. Parse `enemyId` from JSON
2. Find `BP_EnemyManager` actor in the level (cached or re-found)
3. Call `GetEnemyActor(EnemyId)` via `ProcessEvent` (cross-language call)
4. Call `PlayAttackAnimation()` via `ProcessEvent` (cross-language call)

The new version is 4 lines of actual logic. No cross-language ProcessEvent calls. No actor scanning. Direct map lookup.

**SpawnOrUpdateEnemy:**
```cpp
AMMOEnemyActor* UEnemySubsystem::SpawnOrUpdateEnemy(
    int32 EnemyId, int32 TemplateId, const FString& Name, int32 Level,
    float InHealth, float InMaxHealth, float X, float Y, float Z, bool bIsMoving)
{
    UWorld* World = GetWorld();
    if (!World || !EnemyActorClass) return nullptr;

    // Check existing
    TWeakObjectPtr<AMMOEnemyActor>* Existing = Enemies.Find(EnemyId);
    if (Existing && Existing->IsValid())
    {
        AMMOEnemyActor* Enemy = Existing->Get();
        Enemy->SetTargetPosition(FVector(X, Y, Z), bIsMoving);
        Enemy->UpdateHealth(InHealth, InMaxHealth, Enemy->bInCombat);
        return Enemy;
    }

    // Spawn new
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AMMOEnemyActor* NewEnemy = World->SpawnActor<AMMOEnemyActor>(
        EnemyActorClass, FVector(X, Y, Z), FRotator::ZeroRotator, SpawnParams);

    if (!NewEnemy) return nullptr;

    NewEnemy->Initialize(EnemyId, TemplateId, Name, Level, InHealth, InMaxHealth);
    Enemies.Add(EnemyId, NewEnemy);

    return NewEnemy;
}
```

**RemoveAllEnemies:**
```cpp
void UEnemySubsystem::RemoveAllEnemies()
{
    for (auto& Pair : Enemies)
    {
        if (Pair.Value.IsValid())
        {
            // Clear any pending timers before destroying
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().ClearAllTimersForObject(Pair.Value.Get());
            }
            Pair.Value->Destroy();
        }
    }
    Enemies.Empty();
}
```

---

## 8. Integration With Existing Systems

### 8.1 WorldHealthBarSubsystem

**Current state:** `WorldHealthBarSubsystem` independently registers for `enemy:spawn`, `enemy:move`, `enemy:death`, and `enemy:health_update` via `USocketEventRouter`. It maintains its own `TMap<int32, FEnemyBarData> EnemyHealthMap` and uses a heuristic `CacheEnemyActors()` method to find enemy actors in the world by class name and distance matching.

**After migration:** `WorldHealthBarSubsystem`'s socket event handlers remain unchanged -- they continue to update `EnemyHealthMap` independently. However, the heuristic actor caching (`CacheEnemyActors`) can be improved:

**Option A (minimal change, recommended for Phase 4):** Keep `CacheEnemyActors()` as-is. It searches for actors with "Enemy" in their class name. `AMMOEnemyActor`'s class name contains "Enemy", so the heuristic still works. No changes needed.

**Option B (better, can be done as a follow-up):** Replace heuristic caching with direct lookup via `UEnemySubsystem`. The subsystem provides `GetEnemy(EnemyId)` which returns the exact actor. This eliminates the 2-second polling timer, the distance-matching heuristic, and the "claimed actor" deduplication logic.

```cpp
// In WorldHealthBarSubsystem::GetEnemyFeetPosition (improved version):
if (!Enemy.CachedActor.IsValid())
{
    if (UEnemySubsystem* ES = GetWorld()->GetSubsystem<UEnemySubsystem>())
    {
        Enemy.CachedActor = ES->GetEnemy(Enemy.EnemyId);
    }
}
```

### 8.2 MultiplayerEventSubsystem

**Current state:** Bridges 31 socket events from `USocketEventRouter` to `BP_SocketManager` handler functions via `ProcessEvent`. Among these:

| Event | BP Handler | Action |
|-------|-----------|--------|
| `player:moved` | `OnPlayerMoved` | Calls `BP_OtherPlayerManager.SpawnOrUpdatePlayer` |
| `player:left` | `OnPlayerLeft` | Calls `BP_OtherPlayerManager.RemovePlayer` |
| `enemy:spawn` | `OnEnemySpawn` | Calls `BP_EnemyManager.SpawnOrUpdateEnemy` |
| `enemy:move` | `OnEnemyMove` | Calls `BP_EnemyManager.SpawnOrUpdateEnemy` (position update) |
| `enemy:death` | `OnEnemyDeath` | Calls enemy actor's `OnEnemyDeath()` |
| `enemy:health_update` | `OnEnemyHealthUpdate` | Calls enemy actor's `UpdateEnemyHealth()` |
| `enemy:attack` | (direct C++ handler) | Finds enemy via `BP_EnemyManager`, calls `PlayAttackAnimation()` |

**After migration:** These 7 events are removed from `MultiplayerEventSubsystem`. The bridge count drops from 31 to 24. Specifically:

1. Remove `player:moved` handler registration (line 48-49 of current `.cpp`).
2. Remove `player:left` handler registration (line 50-51).
3. Remove `enemy:spawn` handler registration (line 74-75).
4. Remove `enemy:move` handler registration (line 76-77).
5. Remove `enemy:death` handler registration (line 78-79).
6. Remove `enemy:health_update` handler registration (line 80-81).
7. Remove `enemy:attack` handler registration (line 82-83) AND the entire `HandleEnemyAttack()` method.
8. Remove the `EnemyManagerActor` member variable and `FindEnemyManagerActor()` method (no longer needed).
9. Update the log count from 31 to 24.

**Important: Do NOT remove the `SocketManagerActor` reference or `FindSocketManagerActor()` -- those are still needed for the remaining 24 bridged events.**

### 8.3 DamageNumberSubsystem

**Current state:** Already a C++ subsystem. It spawns floating damage numbers from `combat:damage` and `skill:effect_damage` events. It does not directly interact with entity managers.

**After migration:** No changes needed. `DamageNumberSubsystem` continues to read damage amounts and positions from socket event payloads independently.

### 8.4 UCombatActionSubsystem (Phase 3 -- prerequisite)

**Current state:** Not yet implemented (Phase 3). When implemented, it will handle `combat:damage` processing, including playing attack animations on the local player and facing the target.

**After migration:** `UCombatActionSubsystem` can use `UEnemySubsystem::GetEnemy()` and `UOtherPlayerSubsystem::GetPlayer()` to look up target actors by ID for facing, range checking, and animation triggering. Example:

```cpp
// In UCombatActionSubsystem, when processing combat:damage where local player is attacker:
if (bIsEnemy)
{
    UEnemySubsystem* ES = GetWorld()->GetSubsystem<UEnemySubsystem>();
    if (ES)
    {
        AMMOEnemyActor* Target = ES->GetEnemy(TargetId);
        if (Target)
        {
            // Face the target
            FVector Direction = Target->GetActorLocation() - LocalPawn->GetActorLocation();
            Direction.Z = 0;
            if (!Direction.IsNearlyZero())
            {
                LocalPawn->SetActorRotation(Direction.Rotation());
            }
        }
    }
}
```

### 8.5 BuffBarSubsystem

**Current state:** Already a C++ subsystem. Does not interact with entity managers.

**After migration:** No changes needed.

### 8.6 SkillTreeSubsystem (Targeting Integration)

**Current state:** `SkillTreeSubsystem` handles `skill:use` emission and ground AoE targeting. It uses target information from the Blueprint targeting system.

**After migration:** When `UTargetingSubsystem` is implemented (Phase 2), `SkillTreeSubsystem` can use it for target validation. For Phase 4 specifically, no changes are needed to `SkillTreeSubsystem`.

---

## 9. Skeletal Mesh and Animation Strategy

### 9.1 The Problem

Blueprint actors (BP_OtherPlayerCharacter, BP_EnemyCharacter) have their skeletal meshes, animation blueprints, and materials configured in the Blueprint editor. C++ actors need meshes too, but hardcoding mesh paths in C++ constructors is fragile and inflexible.

### 9.2 Recommended Approach: Blueprint Subclasses for Mesh Configuration

Create minimal Blueprint subclasses of the C++ actor classes. These Blueprints do one thing: set the skeletal mesh and animation blueprint in the editor. All logic stays in C++.

**BP_MMORemotePlayer** (Blueprint inheriting AMMORemotePlayer):
- Set Skeletal Mesh component mesh to the character skeletal mesh
- Set Animation Blueprint class (ABP_MMOCharacter)
- No event graph nodes, no variables, no functions

**BP_MMOEnemy** (Blueprint inheriting AMMOEnemyActor):
- Set Skeletal Mesh component mesh to the enemy skeletal mesh
- Set Animation Blueprint class
- No event graph nodes, no variables, no functions

**Spawn class configuration:**
The subsystems need to spawn the Blueprint subclass, not the raw C++ class. Two options:

**Option A: SoftClassPath in constructor or config**
```cpp
// In UEnemySubsystem::OnWorldBeginPlay
static const TCHAR* EnemyBPPath =
    TEXT("/Game/SabriMMO/Blueprints/BP_MMOEnemy.BP_MMOEnemy_C");
EnemyActorClass = LoadClass<AMMOEnemyActor>(nullptr, EnemyBPPath);
if (!EnemyActorClass)
{
    // Fallback to C++ class (no mesh, but won't crash)
    EnemyActorClass = AMMOEnemyActor::StaticClass();
}
```

**Option B: DefaultSubobjects in C++ constructor**
Set a default mesh in the C++ constructor using `ConstructorHelpers::FObjectFinder`. Less flexible but no Blueprint subclass needed:
```cpp
// In AMMOEnemyActor constructor
static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
    TEXT("/Game/SabriMMO/Characters/Enemy/SK_Enemy"));
if (MeshFinder.Succeeded())
{
    GetMesh()->SetSkeletalMesh(MeshFinder.Object);
}
```

**Recommendation:** Use Option A (Blueprint subclass) for now. This matches how the project already works -- mesh configuration lives in the editor. When the art pipeline produces per-template meshes (e.g., Poring mesh vs. Lunatic mesh), the Blueprint subclass can be extended with a mesh table, or the C++ actor can load meshes dynamically based on `TemplateId`.

### 9.3 Animation Triggering

Both C++ actor classes need to play animations. The approach depends on what animation assets exist:

**If Animation Montages exist:**
```cpp
void AMMOEnemyActor::PlayAttackAnimation()
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        // Load montage from a UPROPERTY or static path
        if (AttackMontage)
        {
            AnimInstance->Montage_Play(AttackMontage);
        }
    }
}
```

**If using AnimInstance variables (current pattern in BP):**
```cpp
void AMMOEnemyActor::PlayAttackAnimation()
{
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        // Set a bool that the Animation Blueprint reads
        FProperty* Prop = AnimInstance->GetClass()->FindPropertyByName(TEXT("bIsAttacking"));
        if (Prop)
        {
            bool bValue = true;
            Prop->SetValue_InContainer(AnimInstance, &bValue);
        }
    }
}
```

**Recommendation:** Define `UPROPERTY()` montage references on the actor class, assignable in the Blueprint subclass. This keeps animation configuration in the editor while triggering stays in C++.

```cpp
// In AMMOEnemyActor header:
UPROPERTY(EditDefaultsOnly, Category = "Animation")
TObjectPtr<UAnimMontage> AttackMontage;

UPROPERTY(EditDefaultsOnly, Category = "Animation")
TObjectPtr<UAnimMontage> DeathMontage;

UPROPERTY(EditDefaultsOnly, Category = "Animation")
TObjectPtr<UAnimMontage> HitReactionMontage;
```

---

## 10. Zone Transition Lifecycle

Zone transitions destroy and recreate `UWorldSubsystem` instances (because the world is destroyed and recreated during `OpenLevel()`). The persistent socket on `UMMOGameInstance` survives the transition.

### 10.1 Zone Exit Sequence

1. `ZoneTransitionSubsystem` triggers `OpenLevel()`.
2. UE5 destroys the current world.
3. All `UWorldSubsystem::Deinitialize()` methods are called:
   - `UOtherPlayerSubsystem::Deinitialize()`: calls `RemoveAllPlayers()` (destroys all `AMMORemotePlayer` actors), unregisters from `USocketEventRouter`.
   - `UEnemySubsystem::Deinitialize()`: calls `RemoveAllEnemies()` (destroys all `AMMOEnemyActor` actors, clears timers), unregisters from `USocketEventRouter`.
4. All actors in the level are destroyed.
5. `USocketEventRouter` continues to exist on `UMMOGameInstance` (survives level transitions). Socket events arriving during the transition are dispatched to any remaining handlers (none, since all subsystems have unregistered). Events are effectively dropped during this window -- this is safe because the server sends a full state dump on `zone:ready`.

### 10.2 Zone Entry Sequence

1. New level loads.
2. Level Blueprint runs: spawns player pawn, configures zone.
3. All `UWorldSubsystem::OnWorldBeginPlay()` methods are called:
   - `UOtherPlayerSubsystem::OnWorldBeginPlay()`: registers handlers for `player:moved` and `player:left` with `USocketEventRouter`.
   - `UEnemySubsystem::OnWorldBeginPlay()`: registers handlers for `enemy:spawn`, `enemy:move`, `enemy:moved`, `enemy:death`, `enemy:health_update`, `enemy:attack`.
4. Client emits `zone:ready` to the server.
5. Server responds by sending:
   - `enemy:spawn` for each living enemy in the zone.
   - `player:moved` for each other player in the zone.
6. Subsystem handlers process these events and spawn the corresponding C++ actors.

### 10.3 Edge Cases

**Events arriving between Deinitialize and OnWorldBeginPlay:** Dropped safely. The `USocketEventRouter` dispatches to handler lists, but all handlers for the old subsystem have been unregistered. No crash, no stale state.

**`zone:ready` fires before subsystems are ready:** This cannot happen because `zone:ready` is emitted by the client after the level has loaded and `BeginPlay` has fired (see `ZoneTransitionSubsystem`).

**Enemy IDs reused across zones:** Enemy IDs are globally unique on the server (monotonic counter). A zone transition destroys all local enemy actors and empties the map. New `enemy:spawn` events for the new zone use new IDs. No collision.

**Player IDs are characters, not sockets:** Character IDs are database-assigned and globally unique. No collision possible.

---

## 11. Files to Create

| File | Type | Lines (est.) |
|------|------|-------------|
| `client/SabriMMO/Source/SabriMMO/MMORemotePlayer.h` | C++ Header | ~80 |
| `client/SabriMMO/Source/SabriMMO/MMORemotePlayer.cpp` | C++ Source | ~120 |
| `client/SabriMMO/Source/SabriMMO/MMOEnemyActor.h` | C++ Header | ~100 |
| `client/SabriMMO/Source/SabriMMO/MMOEnemyActor.cpp` | C++ Source | ~200 |
| `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.h` | C++ Header | ~60 |
| `client/SabriMMO/Source/SabriMMO/UI/OtherPlayerSubsystem.cpp` | C++ Source | ~180 |
| `client/SabriMMO/Source/SabriMMO/UI/EnemySubsystem.h` | C++ Header | ~70 |
| `client/SabriMMO/Source/SabriMMO/UI/EnemySubsystem.cpp` | C++ Source | ~280 |

**Total: 8 new files, ~1,090 estimated lines of code.**

---

## 12. Files to Modify

### 12.1 MultiplayerEventSubsystem.cpp

**Changes:**
1. Remove handler registrations for 7 events (`player:moved`, `player:left`, `enemy:spawn`, `enemy:move`, `enemy:death`, `enemy:health_update`, `enemy:attack`) from `OnWorldBeginPlay()`.
2. Remove the `HandleEnemyAttack()` method entirely (~48 lines).
3. Remove the `EnemyManagerActor` member variable from the header.
4. Remove `FindEnemyManagerActor()` method (~14 lines).
5. Update the log count from 31 to 24.

**Lines removed:** ~70
**Lines added:** 0

### 12.2 MultiplayerEventSubsystem.h

**Changes:**
1. Remove `HandleEnemyAttack()` declaration.
2. Remove `EnemyManagerActor` member variable.
3. Remove `FindEnemyManagerActor()` declaration.

### 12.3 OtherCharacterMovementComponent.h/.cpp

**No changes needed.** `AMMORemotePlayer` uses this component via `SetDefaultSubobjectClass` in its constructor. The component works identically with Blueprint or C++ character owners.

### 12.4 SabriMMO.Build.cs

**Potentially:** Verify that no additional module dependencies are needed. The existing module already includes `SocketIOClient`, `JsonUtilities`, and core UE5 modules. No new dependencies expected for entity management.

---

## 13. Blueprint Changes

### 13.1 New Blueprints to Create (in Unreal Editor)

| Blueprint | Parent Class | Purpose |
|-----------|-------------|---------|
| `BP_MMORemotePlayer` | `AMMORemotePlayer` | Set skeletal mesh, animation blueprint, montage references |
| `BP_MMOEnemy` | `AMMOEnemyActor` | Set skeletal mesh, animation blueprint, montage references |

These Blueprints contain NO event graph logic. They exist solely to configure visual assets that are best set in the editor.

### 13.2 Blueprints to Remove (Phase 6 only -- NOT during Phase 4)

During Phase 4, old Blueprint actors continue to exist in levels but stop receiving events (because their socket events are no longer bridged through `MultiplayerEventSubsystem`). They become inert.

In Phase 6 (cleanup), the following are removed:

| Blueprint | Reason |
|-----------|--------|
| `BP_OtherPlayerManager` (actor in levels) | Replaced by `UOtherPlayerSubsystem` |
| `BP_EnemyManager` (actor in levels) | Replaced by `UEnemySubsystem` |
| `BP_OtherPlayerCharacter` (spawned dynamically) | Replaced by `AMMORemotePlayer` (spawned by `UOtherPlayerSubsystem`) |
| `BP_EnemyCharacter` (spawned dynamically) | Replaced by `AMMOEnemyActor` (spawned by `UEnemySubsystem`) |
| `WBP_PlayerNameTag` (WidgetComponent on each entity) | Replaced by `UNameTagSubsystem` Slate overlay (Phase 5) |

### 13.3 Level Blueprint Changes (Phase 6 only)

Each game level's Level Blueprint spawns `BP_OtherPlayerManager` and `BP_EnemyManager` and stores references on `BP_SocketManager`. In Phase 6, these spawn nodes are removed.

---

## 14. Migration Sequence

### Step 1: Create Actor Classes

1. Create `MMORemotePlayer.h/.cpp` with full implementation.
2. Create `MMOEnemyActor.h/.cpp` with full implementation.
3. Compile. Verify no build errors.
4. Create `BP_MMORemotePlayer` and `BP_MMOEnemy` in the editor. Set meshes and animation blueprints by copying the configuration from `BP_OtherPlayerCharacter` and `BP_EnemyCharacter`.

### Step 2: Create Subsystems (Coexistence Mode)

1. Create `OtherPlayerSubsystem.h/.cpp` with full implementation.
2. Create `EnemySubsystem.h/.cpp` with full implementation.
3. Compile. Verify no build errors.
4. **At this point, both old (BP) and new (C++) systems handle the same events.** `player:moved` fires both `UOtherPlayerSubsystem::HandlePlayerMoved` (spawns `AMMORemotePlayer`) and `MultiplayerEventSubsystem::ForwardToBPHandler("OnPlayerMoved")` (spawns `BP_OtherPlayerCharacter`). This means two actors per remote player/enemy exist simultaneously. This is intentional for testing.

### Step 3: Verify C++ Entities Work

1. Log into a zone with enemies and other players.
2. Verify C++ enemy actors spawn at correct positions (they will overlap with BP enemy actors).
3. Verify C++ remote player actors spawn and interpolate correctly.
4. Verify enemy attack animations trigger on C++ actors.
5. Verify enemy death hides the C++ actor.
6. Verify enemy respawn re-enables the C++ actor.
7. Verify zone transition cleans up all C++ actors without leaks or crashes.

### Step 4: Cut Over (Remove BP Bridge)

1. Remove the 7 event registrations from `MultiplayerEventSubsystem::OnWorldBeginPlay()`.
2. Remove `HandleEnemyAttack()`, `EnemyManagerActor`, and `FindEnemyManagerActor()`.
3. Update the header to match.
4. Compile. Test.
5. BP actors (`BP_OtherPlayerCharacter`, `BP_EnemyCharacter`) are no longer spawned because `BP_SocketManager` no longer receives the events. They sit idle in the level (managers still exist but receive no calls). Old actors are inert.

### Step 5: Verify Full Behavior

Run through the complete testing checklist (Section 16).

---

## 15. Conflict Analysis

### 15.1 Dual Spawning During Coexistence

**Issue:** During Step 2-3, both BP and C++ spawn actors for the same entities. Two copies of each remote player and enemy exist in the world.

**Impact:**
- Visual: Two overlapping meshes per entity (one from BP actor, one from C++ actor). Noticeable but not harmful.
- WorldHealthBarSubsystem actor caching: The heuristic `CacheEnemyActors()` may cache C++ actors instead of BP actors (or vice versa). HP bars may attach to the wrong copy. This is temporary and resolves when the BP bridge is removed.
- Collision: Two capsules per entity. Cursor traces may hit either one. Not harmful during testing.
- Performance: Double the actor count. Acceptable for short testing period.

**Resolution:** Step 4 removes the BP bridge, eliminating the dual spawn. This should be done quickly after Step 3 verification.

### 15.2 WorldHealthBarSubsystem CacheEnemyActors Heuristic

**Issue:** `CacheEnemyActors()` iterates all `ACharacter` actors whose class name contains "Enemy". After migration, `AMMOEnemyActor` matches this filter. The heuristic uses 2D distance matching to associate actors with `FEnemyBarData` entries.

**Resolution:** This works correctly because:
- `AMMOEnemyActor` class name contains "Enemy" (matches the filter).
- `AMMOEnemyActor` is spawned at the same position as the corresponding `FEnemyBarData` entry (because both come from the same socket event).
- After BP bridge removal, only C++ actors exist, so there is no ambiguity.

### 15.3 MultiplayerEventSubsystem EnemyManagerActor After Removal

**Issue:** After removing `enemy:attack` from the bridge, `HandleEnemyAttack()` is deleted. But `FindEnemyManagerActor()` is also used in `OnWorldBeginPlay()` to cache the reference. If the code is partially removed, there could be dangling references.

**Resolution:** Remove all three together: `HandleEnemyAttack()`, `EnemyManagerActor` member, and `FindEnemyManagerActor()`. The `OnWorldBeginPlay()` line `EnemyManagerActor = FindEnemyManagerActor();` is also removed. Clean cut.

### 15.4 BPI_Damageable Interface

**Issue:** BP actors implement `BPI_Damageable` as a Blueprint Interface. C++ actors should implement the same interface for compatibility with systems that use interface-based interaction.

**Resolution:** For Phase 4, `BPI_Damageable` implementation is NOT required on the C++ actors. The systems that use this interface (combat damage visual, health display) already work through other paths:
- WorldHealthBarSubsystem tracks HP via its own socket handlers (no interface call).
- DamageNumberSubsystem spawns numbers from socket event data (no interface call).
- Attack animations are triggered directly by `UEnemySubsystem::HandleEnemyAttack()` (no interface).

If `UCombatActionSubsystem` (Phase 3) needs the interface, it can be added to the C++ actors at that time. For Phase 4, the actors work without it.

### 15.5 enemy:attack Handler Ownership Conflict

**Issue:** If `UEnemySubsystem` registers for `enemy:attack` AND `MultiplayerEventSubsystem` still bridges it, the attack animation would fire twice.

**Resolution:** During coexistence (Step 2-3), both fire. The BP `PlayAttackAnimation` fires on the BP enemy actor, and the C++ one fires on the C++ enemy actor. Since they are different actors, there is no double-play on the same actor. After Step 4 (bridge removed), only the C++ handler remains.

---

## 16. Testing Checklist

### 16.1 Other Player (Remote Player) Tests

- [ ] Other player appears when joining the same zone
- [ ] Other player position updates smoothly (interpolation, not teleporting)
- [ ] Other player stops at correct position when they stop moving
- [ ] Other player disappears immediately on disconnect (`player:left`)
- [ ] Other player disappears when they change zones (`player:left` with reason `zone_change`)
- [ ] Other player has correct floor snap (not floating above or sinking below ground)
  - Verify `UOtherCharacterMovementComponent` is active on the C++ actor
- [ ] Other player health data is stored (for future party HP display)
- [ ] Other player name is stored (for Phase 5 name tag display)
- [ ] Spawn 5+ remote players -- all tracked correctly in the map
- [ ] Local player's own `player:moved` events are filtered out (no self-spawning)

### 16.2 Enemy Tests

- [ ] Enemies spawn at correct positions when entering a zone
- [ ] Enemy wanders (smooth movement when `enemy:move` with `isMoving: true`)
- [ ] Enemy stops wandering (stops at position when `enemy:move` with `isMoving: false`)
- [ ] Enemy chases player (smooth interpolated movement during chase)
- [ ] Enemy death: collision disabled, mesh hidden after corpse delay
- [ ] Enemy death: actor stays in the map (NOT removed from `TMap`)
- [ ] Enemy respawn: existing actor is reused and re-initialized (not a new spawn)
- [ ] Enemy respawn: mesh becomes visible again, collision re-enabled
- [ ] Enemy health update: local `Health`/`MaxHealth` values update correctly
- [ ] Enemy `bInCombat` tracks server `inCombat` flag
- [ ] Enemy attack animation plays when `enemy:attack` arrives
- [ ] Enemy knockback: teleport to new position on `enemy:moved` (no interpolation)
- [ ] Spawn 20+ enemies -- all tracked correctly
- [ ] Enemy with ID that doesn't exist in map: no crash on `enemy:move`, `enemy:death`, etc.

### 16.3 Targeting and Click Detection

- [ ] Click on enemy actor -- targeting works (ECC_Visibility collision response set)
- [ ] Hover over enemy actor -- hover indicator logic can detect the actor
  - Note: hover indicator visuals may not be present yet (Phase 5). Test that the cursor trace hits the actor's capsule.
- [ ] Click on remote player actor -- cursor trace hits (for future party/trade interaction)

### 16.4 WorldHealthBarSubsystem Integration

- [ ] Enemy HP bar appears when enemy is damaged (via `enemy:health_update` with `inCombat: true`)
- [ ] Enemy HP bar position tracks smoothly with the C++ actor (via `CacheEnemyActors` heuristic or direct lookup)
- [ ] Enemy HP bar hides on death
- [ ] HP bar system does not crash when the BP enemy actors are no longer spawned

### 16.5 Zone Transition Tests

- [ ] Zone transition: all remote player actors are destroyed (no leaks)
- [ ] Zone transition: all enemy actors are destroyed (no leaks, timers cleared)
- [ ] Zone transition: `UOtherPlayerSubsystem` and `UEnemySubsystem` unregister from router
- [ ] New zone: subsystems re-register with router
- [ ] New zone: enemies spawn from `enemy:spawn` events sent by server after `zone:ready`
- [ ] New zone: other players spawn from `player:moved` events
- [ ] Rapid zone transitions (warp portal chain): no crash, no actor leaks

### 16.6 MultiplayerEventSubsystem Cleanup

- [ ] `player:moved` no longer bridged to BP_SocketManager (removed from registration)
- [ ] `player:left` no longer bridged
- [ ] `enemy:spawn` no longer bridged
- [ ] `enemy:move` no longer bridged
- [ ] `enemy:death` no longer bridged
- [ ] `enemy:health_update` no longer bridged
- [ ] `enemy:attack` no longer bridged (HandleEnemyAttack removed)
- [ ] Remaining 24 bridged events still work (combat, inventory, chat, shop, etc.)
- [ ] BP_SocketManager still found by `FindSocketManagerActor()` for remaining events
- [ ] `EnemyManagerActor` and `FindEnemyManagerActor()` fully removed from header and source

### 16.7 Performance

- [ ] Tick cost: 50 enemies interpolating simultaneously -- no frame drop
- [ ] Memory: no actor leaks across 10+ zone transitions (monitor actor count)
- [ ] `TMap` lookup: `GetEnemy(EnemyId)` is O(1) -- verify with 100+ enemies

### 16.8 Edge Cases

- [ ] Server sends `enemy:death` for an enemy not in the local map -- no crash
- [ ] Server sends `player:left` for a player not in the local map -- no crash
- [ ] Server sends `enemy:move` for an enemy not in the local map -- no crash
- [ ] Server sends `enemy:spawn` for an enemy that was previously killed (dead in map) -- respawn works
- [ ] Server sends `enemy:spawn` for an enemy that was previously killed and its corpse timer already fired (hidden) -- respawn works
- [ ] Two rapid `enemy:death` events for the same enemy -- no crash or double animation
- [ ] `enemy:attack` for a dead enemy -- no crash (silently ignored or skipped)
- [ ] `player:moved` with `characterId` matching local player -- filtered out, not spawned
