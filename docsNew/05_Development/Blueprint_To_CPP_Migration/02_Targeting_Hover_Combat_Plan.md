# 02 - Targeting, Hover Detection, Auto-Attack & Combat Actions Migration Plan

**Status**: PLANNED
**Priority**: HIGH (core gameplay loop)
**Dependencies**: Persistent socket (Phase 4) COMPLETE, EventRouter COMPLETE, DamageNumberSubsystem COMPLETE
**Estimated Effort**: 3-4 sessions

---

## Table of Contents

1. [Scope & Goals](#1-scope--goals)
2. [RO Classic Targeting Behavior](#2-ro-classic-targeting-behavior-pre-renewal)
3. [Current Blueprint Implementation](#3-current-blueprint-implementation-what-gets-replaced)
4. [New C++ Architecture](#4-new-c-architecture)
5. [UTargetingSubsystem Specification](#5-utargetingsubsystem-specification)
6. [UCombatActionSubsystem Specification](#6-ucombatactionsubsystem-specification)
7. [Server Event Payloads](#7-server-event-payloads-exact-format)
8. [Integration with Existing C++ Subsystems](#8-integration-with-existing-c-subsystems)
9. [Event Deduplication & MultiplayerEventSubsystem Changes](#9-event-deduplication--multiplayereventsubsystem-changes)
10. [Files to Create](#10-files-to-create)
11. [Files to Modify](#11-files-to-modify)
12. [Implementation Steps](#12-implementation-steps)
13. [Testing Checklist](#13-testing-checklist)
14. [Risk Analysis & Rollback](#14-risk-analysis--rollback)

---

## 1. Scope & Goals

### What This Migration Covers

- **Hover detection** (AC_TargetingSystem's 59-node `UpdateHoverDetection` graph)
- **Cursor type switching** based on what the mouse hovers over
- **Click-to-attack** initiation (BP_MMOCharacter's `IA_Attack` event graph)
- **Auto-attack range loop** (BP_MMOCharacter Event Tick)
- **Combat socket event handling** (BP_SocketManager's `OnCombatDamage` 215 nodes, `OnAutoAttackStarted` 32 nodes, `OnAutoAttackStopped` 27 nodes, `OnCombatOutOfRange` 30 nodes, `OnTargetLost` 27 nodes, `OnCombatDeath` 19 nodes, `OnCombatRespawn` 72 nodes)
- **Attack animation playback** and **actor rotation** on damage events

### What This Migration Does NOT Cover

- DamageNumberSubsystem (already C++, already handles `combat:damage` and `skill:effect_damage`)
- WorldHealthBarSubsystem (already C++, already handles HP bar updates)
- BasicInfoSubsystem (already C++, already handles local HP/SP/EXP)
- BuffBarSubsystem (already C++)
- CastBarSubsystem (already C++)
- SkillVFXSubsystem (already C++)
- Enemy spawn/move/death (remains in BP_EnemyManager for now)
- Player spawn/move/leave (remains bridged to BP for now)

### Goals

1. Eliminate the 215-node `OnCombatDamage` Blueprint function (largest single BP function in the project)
2. Replace per-tick Blueprint hover detection with optimized C++ trace
3. Move all targeting state into a single `UTargetingSubsystem` (currently scattered across AC_TargetingSystem, BP_MMOCharacter variables, and BP_SocketManager)
4. Reduce ProcessEvent bridge calls by removing 8 combat events from `MultiplayerEventSubsystem`
5. Make combat response deterministic and easier to debug (C++ breakpoints vs BP node-by-node stepping)

---

## 2. RO Classic Targeting Behavior (Pre-Renewal)

### Click-to-Target

RO Classic uses a purely click-based targeting system. There is no tab-targeting.

| Input | Action | Server Event |
|-------|--------|-------------|
| Left-click monster | Start auto-attacking immediately | Client emits `combat:attack` with `targetEnemyId` |
| Left-click NPC | Walk to NPC, initiate dialog | No server event (client-side interaction) |
| Left-click ground | Walk to position | No server event (client-side movement) |
| Left-click ground item | Walk to item, pick up | Client emits `loot:pickup` |
| Right-click player | Context menu (whisper, trade, party) | No server event until menu choice |
| Click ground while attacking | Stop attacking | Client emits `combat:stop_attack` |

**Key behavior**: Clicking a monster is a single action -- there is no separate "select target" then "press attack button" flow. The click IS the attack command.

### Cursor Types

| Cursor State | Trigger Condition | UE5 `EMouseCursor` |
|-------------|-------------------|---------------------|
| Normal arrow | Over ground or nothing | `EMouseCursor::Default` |
| Attack sword | Over hostile monster | `EMouseCursor::Crosshairs` |
| Speech bubble | Over NPC | `EMouseCursor::TextEditBeam` (custom texture later) |
| Grab hand | Over ground item | `EMouseCursor::GrabHand` |
| Crosshair | Active skill targeting mode | `EMouseCursor::Crosshairs` |

**Note**: Custom cursor textures can be added later via `FSlateApplication::Get().GetPlatformCursor()->SetCustomCursor()`. Phase 1 uses built-in cursors.

### Target Priority (Overlapping Entities)

When entities overlap under the cursor, priority determines which one gets interacted with:

1. **NPC** (highest -- always interact, never attack)
2. **Players** (context menu or PvP if enabled)
3. **Monsters** (attack)
4. **Ground items** (pickup)
5. **Ground** (movement -- lowest)

This is enforced by collision channel ordering in the line trace, not by Z-sorting.

### Hover Effects

- **Enemy hover**: Name text highlighted, selection circle/indicator widget appears at feet
- **NPC hover**: Name highlighted, cursor changes to talk icon
- **Player hover**: Name always visible (RO shows names persistently)
- **Cursor leave**: Indicator hidden immediately, cursor reverts to default

### Auto-Attack Loop (Full Client-Server Flow)

```
1. Player clicks enemy
2. Client: TargetingSubsystem sets LockedTarget
3. Client: CombatActionSubsystem emits combat:attack { targetEnemyId }
4. Server: Validates target alive, attacker alive, not CC'd, not overweight
5. Server: Emits combat:auto_attack_started { targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs }
6. Client: CombatActionSubsystem receives confirmation, updates targeting state
7. Server: Combat tick loop (50ms) checks ASPD timing per attacker
8. Server: When ASPD timer fires, checks range -> if out of range, emits combat:out_of_range
9. Client: Receives out_of_range -> SimpleMoveToLocation toward target
10. Server: When in range, calculates damage (HIT/FLEE, crit, element, size, DEF)
11. Server: Broadcasts combat:damage { attackerId, targetId, damage, isCritical, hitType, ... }
12. Client: CombatActionSubsystem rotates attacker toward target, plays attack animation
13. Client: DamageNumberSubsystem (ALREADY C++) spawns floating number
14. Client: WorldHealthBarSubsystem (ALREADY C++) updates HP bar
15. If target dies: Server emits combat:target_lost { reason: "Enemy died" } to attacker
16. Client: CombatActionSubsystem clears target, stops animations
17. If player clicks elsewhere: Client emits combat:stop_attack
18. Server: Emits combat:auto_attack_stopped { reason: "Player stopped" }
```

### Attack Animations

- Character rotates to face target (Yaw only) before each attack swing
- Animation speed scales with ASPD (higher ASPD = faster playback rate)
- Dual wield: two damage numbers per attack cycle (DamageNumberSubsystem already handles this)
- Character stays facing target while auto-attacking (does not rotate with camera or movement input)

---

## 3. Current Blueprint Implementation (What Gets Replaced)

### AC_TargetingSystem (ActorComponent on BP_MMOCharacter)

**Node count**: 59 nodes in `UpdateHoverDetection`
**Tick**: Runs every frame (Event Tick)

**Current logic**:
1. `GetHitResultUnderCursorByChannel(Visibility)` per frame
2. Cast chain: `Cast to BP_OtherPlayerCharacter` -> `Cast to BP_EnemyCharacter` -> fallthrough
3. On hover enter: Shows `HoverOverIndicator` WidgetComponent on hovered actor
4. On hover exit: Hides indicator from previous actor
5. `CheckIfInAttackRangeOfTargetPlayer`: 3D distance vs `AttackRange`
6. `CheckIfInAttackRangeOfEnemy`: 3D distance vs `AttackRange`

**Variables exposed**:
- `HoveredTargetPlayerRef` (ACharacter reference)
- `HoeveredTargetEnemyRef` (note: typo in original BP, "Hoevered")
- `AttackRange` (float, default 150)
- `bHasTarget` (bool)

**Problems**:
- Cast chain is O(N) per cast type per tick
- WidgetComponent indicator is expensive (creates a full widget per hover)
- State is on an ActorComponent, not globally accessible
- Typo in variable name (`HoeveredTargetEnemyRef`) propagated to multiple reference points

### BP_MMOCharacter IA_Attack (Event Graph)

**Node count**: ~40 nodes
**Trigger**: Enhanced Input action `IA_Attack` (left-click)

**Current logic**:
1. Get targeting component reference
2. Cast chain: `HoveredTargetPlayerRef` -> `Cast to BP_OtherPlayerCharacter` -> `Cast to BP_EnemyCharacter` -> `Cast to BP_NPCShop` -> `TryInteractWithNPC`
3. If enemy: Set `TargetEnemyRef`, set `bIsAutoAttacking = true`, call `BP_SocketManager->EmitCombatAttack(enemyId, true)`
4. If player: Set `TargetPlayerRef`, emit PvP attack (disabled)
5. If NPC: Call `TryInteractWithNPC()` (already migrated to C++)
6. If nothing (click ground while attacking): Emit `combat:stop_attack`, clear target, set `bIsAutoAttacking = false`

### BP_MMOCharacter Event Tick (Auto-Attack Range Loop)

**Node count**: ~25 nodes in the auto-attack branch
**Runs**: Every frame when `bIsAutoAttacking == true`

**Current logic**:
```
if bIsAutoAttacking:
    if TargetPlayerRef is valid:
        dist = VectorLength(PlayerLoc - TargetPlayerLoc)
        if dist <= AttackRange: StopMovement()
        else: SimpleMoveToLocation(TargetPlayerLoc)
    elif TargetEnemyRef is valid:
        dist = VectorLength(PlayerLoc - TargetEnemyLoc)
        if dist <= AttackRange: StopMovement()
        else: SimpleMoveToLocation(TargetEnemyLoc)
```

**Problem**: Runs every frame, mixes targeting logic with character tick, does 3D distance per frame even when not needed.

### BP_SocketManager Combat Handlers

All BP_SocketManager handler functions receive a single `FString Data` parameter (JSON string), parse it manually in Blueprint, and execute game logic.

| Handler | Node Count | What It Does |
|---------|-----------|--------------|
| `OnCombatDamage` | **215** | Parse damage JSON, identify attacker/target actors, play attack animation, rotate actors, update health (partially duplicated by existing C++ subsystems) |
| `OnAutoAttackStarted` | 32 | Set `bIsAutoAttacking`, show target frame UI, update attack range |
| `OnAutoAttackStopped` | 27 | Clear attack state, hide target frame |
| `OnCombatOutOfRange` | 30 | Parse target position, `SimpleMoveToLocation` toward target |
| `OnTargetLost` | 27 | Stop attack, clear target refs, hide target frame |
| `OnCombatDeath` | 19 | Show death overlay or play death VFX |
| `OnCombatRespawn` | 72 | Teleport to respawn position, restore health, hide death overlay |
| `OnCombatError` | ~10 | Display error message in chat |

**Total**: ~432 Blueprint nodes replaced by this migration.

---

## 4. New C++ Architecture

### Subsystem Overview

```
+-------------------------+       +---------------------------+
| UTargetingSubsystem     |       | UCombatActionSubsystem    |
| (UWorldSubsystem)       |       | (UWorldSubsystem)         |
|                         |       |                           |
| - Hover detection       |       | - combat:damage handler   |
| - Cursor switching      |       | - auto_attack_started     |
| - Target locking        |       | - auto_attack_stopped     |
| - Range checking        |       | - target_lost             |
| - Hover indicator       |       | - out_of_range            |
| - Attack initiation     |       | - death / respawn         |
|   (emits combat:attack) |       | - Attack animation play   |
|                         |       | - Actor rotation          |
+----------+--------------+       +----------+----------------+
           |                                 |
           |  reads LockedTarget             |  reads LockedTarget
           |                                 |  calls SetLockedTarget/ClearTarget
           +----------+---------------------++
                      |
          +-----------v-----------+
          | EventRouter           |
          | (on GameInstance)      |
          +-----------------------+
                      |
          +-----------v-----------+
          | Existing C++ Systems  |
          | (unchanged)           |
          |                       |
          | - DamageNumberSubsystem (combat:damage -> floating numbers)
          | - WorldHealthBarSubsystem (-> HP bars)
          | - BasicInfoSubsystem (-> local HP/SP)
          | - BuffBarSubsystem (-> buff icons)
          | - CastBarSubsystem (-> cast bars)
          | - SkillVFXSubsystem (-> particle effects)
          +-----------------------+
```

### Key Design Decisions

1. **Two subsystems, not one** -- Targeting (per-frame trace, cursor, hover state) is separate from combat action handling (event-driven socket responses). Different tick patterns, different lifecycles.

2. **UWorldSubsystem, not UActorComponent** -- Follows the established pattern from all 19 existing subsystems. Globally accessible via `GetWorld()->GetSubsystem<UTargetingSubsystem>()`. Survives actor destruction/recreation.

3. **Weak object pointers for targets** -- `TWeakObjectPtr<AActor>` prevents dangling references when enemies are destroyed. The BP had crashes from accessing destroyed actor references.

4. **EventRouter multi-handler** -- `UCombatActionSubsystem` registers for `combat:damage` as an ADDITIONAL handler alongside `DamageNumberSubsystem`, `WorldHealthBarSubsystem`, and `BasicInfoSubsystem`. Each subsystem handles its own concern (animation vs numbers vs HP bars vs local stats).

5. **No Slate widget for targeting** -- The hover indicator is implemented as a `UWidgetComponent` already attached to enemy/NPC actors. The subsystem shows/hides it, not create/destroy it. If no WidgetComponent exists, the subsystem still changes the cursor.

6. **Attack initiation uses existing EmitCombatAttack** -- The `UMultiplayerEventSubsystem::EmitCombatAttack(TargetId, bIsEnemy)` BlueprintCallable function already exists and correctly emits `combat:attack`. The new `UTargetingSubsystem` calls this directly.

---

## 5. UTargetingSubsystem Specification

**File**: `client/SabriMMO/Source/SabriMMO/UI/TargetingSubsystem.h/.cpp`
**Replaces**: AC_TargetingSystem, BP_MMOCharacter auto-attack range loop, BP_MMOCharacter IA_Attack target resolution

### Enums

```cpp
UENUM()
enum class ETargetType : uint8
{
    None,
    Enemy,
    Player,
    NPC,
    GroundItem
};
```

### State

```cpp
// ---- Hover state (updated per tick) ----
TWeakObjectPtr<AActor> HoveredActor;
ETargetType HoveredTargetType = ETargetType::None;
int32 HoveredTargetId = 0;

// ---- Locked target (set on click, cleared on target_lost/stop_attack) ----
TWeakObjectPtr<AActor> LockedTarget;
int32 LockedTargetId = 0;
bool bLockedTargetIsEnemy = false;
ETargetType LockedTargetType = ETargetType::None;

// ---- Auto-attack state ----
bool bIsAutoAttacking = false;
float AttackRange = 150.f;        // Updated by combat:auto_attack_started
float RangeTolerance = 50.f;      // Matches COMBAT.RANGE_TOLERANCE on server

// ---- Tick gating ----
float HoverTraceInterval = 0.033f;  // 30Hz hover traces (not every frame)
float TimeSinceLastTrace = 0.f;

// ---- Locally cached character ID ----
int32 LocalCharacterId = 0;
```

### Lifecycle

```cpp
bool ShouldCreateSubsystem(UObject* Outer) const override;
    // Same pattern as all other subsystems: return World->IsGameWorld()

void OnWorldBeginPlay(UWorld& InWorld) override;
    // 1. Resolve LocalCharacterId from GameInstance
    // 2. Register Tick function via FTickerDelegate (30Hz, not every frame)
    // 3. Gate behind GI->IsSocketConnected() (don't tick on login screen)

void Deinitialize() override;
    // 1. Remove tick delegate
    // 2. Clear all target state
    // 3. Reset cursor to default
```

### Tick Logic (30Hz)

```
PerformHoverTrace():
    1. Get APlayerController via World->GetFirstPlayerController()
    2. If no PC or no Pawn: return (not yet spawned)
    3. FHitResult HitResult
    4. PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult)
    5. AActor* HitActor = HitResult.GetActor()
    6. Classify HitActor:
       a. Cast to AShopNPC or AKafraNPC -> ETargetType::NPC
       b. Check actor name contains "EnemyCharacter" -> ETargetType::Enemy
          (or cast to known enemy base class if one exists)
       c. Check actor name contains "OtherPlayerCharacter" -> ETargetType::Player
       d. Check for ground item actor -> ETargetType::GroundItem
       e. Otherwise -> ETargetType::None
    7. If HoveredActor changed:
       a. Hide indicator on old HoveredActor (if valid)
       b. Show indicator on new HitActor (if has WidgetComponent named "HoverOverIndicator")
       c. Update HoveredActor, HoveredTargetType, HoveredTargetId
    8. UpdateCursor(HoveredTargetType)
```

**Actor classification approach**: Since BP_EnemyCharacter and BP_OtherPlayerCharacter are Blueprint classes (not C++ classes), we identify them by checking for specific components or using actor tags. The recommended approach:

- **Enemy actors**: Check for a tag `"Enemy"` set in the Blueprint, OR check if the actor has a specific component (like `EnemyHealthBar`), OR use `GetClass()->GetName().Contains("Enemy")`.
- **Player actors**: Same approach with tag `"OtherPlayer"` or component check.
- **NPCs**: Cast to `AShopNPC` or `AKafraNPC` (these ARE C++ classes).

**Preferred method**: Add an Actor Tag in the Blueprint class defaults. Tags are cheap to check and don't require casting.

| Blueprint Class | Tag to Add | ETargetType |
|----------------|------------|-------------|
| BP_EnemyCharacter | `"Enemy"` | Enemy |
| BP_OtherPlayerCharacter | `"OtherPlayer"` | Player |
| BP_NPCShop (C++) | `"NPC"` (or cast) | NPC |
| BP_KafraNPC (C++) | `"NPC"` (or cast) | NPC |
| BP_GroundItem | `"GroundItem"` | GroundItem |

### Cursor Management

```cpp
void UpdateCursor(ETargetType Type)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    switch (Type)
    {
    case ETargetType::Enemy:
        PC->CurrentMouseCursor = EMouseCursor::Crosshairs;
        break;
    case ETargetType::NPC:
        PC->CurrentMouseCursor = EMouseCursor::TextEditBeam;
        break;
    case ETargetType::GroundItem:
        PC->CurrentMouseCursor = EMouseCursor::GrabHand;
        break;
    default:
        PC->CurrentMouseCursor = EMouseCursor::Default;
        break;
    }
}
```

### Hover Indicator Management

```cpp
void ShowHoverIndicator(AActor* Actor)
{
    if (!Actor) return;
    UWidgetComponent* Indicator = Actor->FindComponentByClass<UWidgetComponent>();
    if (Indicator && Indicator->GetName().Contains(TEXT("HoverOver")))
    {
        Indicator->SetVisibility(true);
    }
}

void HideHoverIndicator(AActor* Actor)
{
    if (!Actor) return;
    UWidgetComponent* Indicator = Actor->FindComponentByClass<UWidgetComponent>();
    if (Indicator && Indicator->GetName().Contains(TEXT("HoverOver")))
    {
        Indicator->SetVisibility(false);
    }
}
```

### Range Checking

```cpp
bool IsInAttackRange(AActor* Target) const
{
    if (!Target) return false;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return false;

    FVector PawnLoc = PC->GetPawn()->GetActorLocation();
    FVector TargetLoc = Target->GetActorLocation();

    // Use 2D distance (XY only) to match server behavior
    // Server calculates: sqrt(dx*dx + dy*dy) without Z
    float Dist2D = FVector::Dist2D(PawnLoc, TargetLoc);
    return Dist2D <= (AttackRange + RangeTolerance);
}

bool IsInAttackRange(const FVector& TargetLocation) const
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return false;

    float Dist2D = FVector::Dist2D(PC->GetPawn()->GetActorLocation(), TargetLocation);
    return Dist2D <= (AttackRange + RangeTolerance);
}
```

**Important**: The server calculates range using 2D distance (`sqrt(dx*dx + dy*dy)` -- no Z component). The client MUST match this. Using `FVector::Dist()` (3D) would cause mismatches where the client thinks it is in range but the server disagrees.

### Attack Initiation (Replaces BP_MMOCharacter IA_Attack)

```cpp
void HandleLeftClick()
{
    // Called from ASabriMMOCharacter::SetupPlayerInputComponent
    // via a new Enhanced Input binding for IA_Attack

    switch (HoveredTargetType)
    {
    case ETargetType::NPC:
        // NPC interaction (highest priority, already migrated to C++)
        if (HoveredActor.IsValid())
        {
            ASabriMMOCharacter* Pawn = Cast<ASabriMMOCharacter>(
                GetWorld()->GetFirstPlayerController()->GetPawn());
            if (Pawn)
            {
                Pawn->TryInteractWithNPC(HoveredActor.Get());
            }
        }
        break;

    case ETargetType::Enemy:
        // Attack enemy (emit combat:attack)
        StartAutoAttack(HoveredActor.Get(), HoveredTargetId, true);
        break;

    case ETargetType::Player:
        // PvP: currently disabled on server
        // Future: context menu (whisper, trade, party invite)
        break;

    case ETargetType::GroundItem:
        // Future: walk to item, emit loot:pickup
        break;

    case ETargetType::None:
        // Clicked empty ground
        if (bIsAutoAttacking)
        {
            StopAutoAttack();
        }
        break;
    }
}

void StartAutoAttack(AActor* Target, int32 TargetId, bool bIsEnemy)
{
    SetLockedTarget(Target, TargetId, bIsEnemy);

    // Emit combat:attack via existing MultiplayerEventSubsystem
    UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>();
    if (MES)
    {
        MES->EmitCombatAttack(TargetId, bIsEnemy);
    }

    // Don't set bIsAutoAttacking yet -- wait for server confirmation
    // (combat:auto_attack_started)
}

void StopAutoAttack()
{
    bIsAutoAttacking = false;
    ClearTarget();

    UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>();
    if (MES)
    {
        MES->EmitStopAttack();
    }
}
```

### Auto-Attack Range Loop (Replaces BP_MMOCharacter Event Tick)

```cpp
void TickAutoAttackRange(float DeltaTime)
{
    if (!bIsAutoAttacking) return;
    if (!LockedTarget.IsValid())
    {
        bIsAutoAttacking = false;
        return;
    }

    // Range is checked by the SERVER in the combat tick loop.
    // The server emits combat:out_of_range when the player is too far.
    // CombatActionSubsystem handles that event and calls MoveTowardTarget().
    // This tick only handles the case where the target actor moves.
    //
    // We do NOT need a per-tick client-side range check because:
    // 1. Server already does it every 50ms (COMBAT_TICK_MS)
    // 2. Server emits combat:out_of_range with target position
    // 3. CombatActionSubsystem calls SimpleMoveToLocation
    // 4. Client-side range check would duplicate server authority
}
```

### Public API

```cpp
// ---- Hover state ----
AActor* GetHoveredActor() const;
ETargetType GetHoveredTargetType() const;
int32 GetHoveredTargetId() const;

// ---- Locked target ----
AActor* GetLockedTarget() const;
int32 GetLockedTargetId() const;
bool IsLockedTargetEnemy() const;
ETargetType GetLockedTargetType() const;
void SetLockedTarget(AActor* Actor, int32 Id, bool bIsEnemy);
void ClearTarget();

// ---- Attack state ----
bool IsAutoAttacking() const;
void SetAutoAttacking(bool bValue);
float GetAttackRange() const;
void SetAttackRange(float Range);

// ---- Range checks ----
bool IsInAttackRange(AActor* Target) const;
bool IsInAttackRange(const FVector& Location) const;

// ---- Actions ----
void HandleLeftClick();      // Called by Enhanced Input
void StartAutoAttack(AActor* Target, int32 TargetId, bool bIsEnemy);
void StopAutoAttack();
void MoveTowardTarget(const FVector& TargetLocation);
```

---

## 6. UCombatActionSubsystem Specification

**File**: `client/SabriMMO/Source/SabriMMO/UI/CombatActionSubsystem.h/.cpp`
**Replaces**: BP_SocketManager combat handlers (OnCombatDamage animation/rotation, OnAutoAttackStarted, OnAutoAttackStopped, OnCombatOutOfRange, OnTargetLost, OnCombatDeath, OnCombatRespawn, OnCombatError)

### Socket Events Registered

| Event | Handler | What It Does |
|-------|---------|--------------|
| `combat:damage` | `HandleCombatDamage` | Play attack animation, rotate attacker toward target |
| `combat:auto_attack_started` | `HandleAutoAttackStarted` | Confirm attack state, set range, lock target |
| `combat:auto_attack_stopped` | `HandleAutoAttackStopped` | Clear attack state |
| `combat:target_lost` | `HandleTargetLost` | Clear target, stop attack |
| `combat:out_of_range` | `HandleOutOfRange` | Move player toward target |
| `combat:death` | `HandleCombatDeath` | Show death overlay or play death VFX |
| `combat:respawn` | `HandleCombatRespawn` | Teleport, restore health, hide death overlay |
| `combat:error` | `HandleCombatError` | Log error, optionally display in chat |
| `combat:health_update` | `HandleHealthUpdate` | Forward to targeting subsystem for target HP tracking |

### State

```cpp
// Local player info
int32 LocalCharacterId = 0;
FString LocalCharacterName;

// Death overlay
bool bDeathOverlayVisible = false;
TSharedPtr<SDeathOverlayWidget> DeathOverlayWidget;  // Simple "You died" + respawn button
TSharedPtr<SWidget> DeathOverlayWrapper;

// Target frame (shows locked target's name, level, HP bar)
bool bTargetFrameVisible = false;
TSharedPtr<STargetFrameWidget> TargetFrameWidget;    // Slate widget showing target info
TSharedPtr<SWidget> TargetFrameWrapper;
int32 TargetFrameTargetId = 0;
FString TargetFrameTargetName;
float TargetFrameHP = 0.f;
float TargetFrameMaxHP = 0.f;
bool bTargetFrameIsEnemy = false;

// Deferred ready flag (same pattern as MultiplayerEventSubsystem)
bool bReadyToProcess = false;
```

### Lifecycle

```cpp
bool ShouldCreateSubsystem(UObject* Outer) const override;
    // return World->IsGameWorld()

void OnWorldBeginPlay(UWorld& InWorld) override;
    // 1. Get GI, Router
    // 2. Resolve LocalCharacterId, LocalCharacterName
    // 3. Register all 9 combat event handlers via Router->RegisterHandler()
    // 4. Defer readiness by one frame (SetTimerForNextTick)
    //    to avoid ProcessEvent-during-PostLoad crash
    // 5. Gate behind GI->IsSocketConnected()

void Deinitialize() override;
    // 1. Router->UnregisterAllForOwner(this)
    // 2. Hide death overlay
    // 3. Clear state
```

### HandleCombatDamage (Replaces 215-Node Blueprint)

This handler is responsible ONLY for attack animations and actor rotation. Damage numbers, HP bars, and stat updates are handled by their respective existing C++ subsystems.

```
HandleCombatDamage(Data):
    1. Parse JSON fields:
       - attackerId (int32)
       - targetId (int32)
       - isEnemy (bool)
       - hitType (string: "normal", "critical", "miss", "dodge", "perfectDodge", "heal")
       - damage (int32)
       - isCritical (bool)
       - isMiss (bool)
       - attackerX/Y/Z (float)
       - targetX/Y/Z (float)
       - isDualWield (bool)
       - damage2 (int32)

    2. If hitType == "miss" or hitType == "dodge" or hitType == "perfectDodge":
       Skip animation (missed attacks don't show swing in RO)
       Return early

    3. Resolve attacker actor:
       a. If attackerId == LocalCharacterId:
          attacker = PC->GetPawn()
       b. Else if attackerId >= 2000000 (enemy ID range):
          attacker = FindEnemyActor(attackerId)  // via BP_EnemyManager
       c. Else:
          attacker = FindPlayerActor(attackerId)  // via BP_OtherPlayerManager

    4. Resolve target actor (same logic with targetId + isEnemy flag)

    5. Rotate attacker to face target:
       if (attacker && target):
           FVector Dir = target->GetActorLocation() - attacker->GetActorLocation()
           FRotator LookRot = Dir.Rotation()
           attacker->SetActorRotation(FRotator(0, LookRot.Yaw, 0))  // Yaw only

    6. Play attack animation on attacker:
       if (ACharacter* CharAttacker = Cast<ACharacter>(attacker)):
           UAnimInstance* AnimInst = CharAttacker->GetMesh()->GetAnimInstance()
           if (AnimInst):
               // Try to find and play attack montage
               // Animation Blueprint handles the actual montage selection
               UFunction* PlayAttackFunc = attacker->FindFunction("PlayAttackAnimation")
               if (PlayAttackFunc):
                   attacker->ProcessEvent(PlayAttackFunc, nullptr)

    7. (damage numbers, HP bars, local stats: handled by other subsystems -- do nothing here)
```

**Actor resolution helpers**:

```cpp
AActor* FindEnemyActor(int32 EnemyId) const
{
    // Find BP_EnemyManager and call GetEnemyActor(EnemyId)
    // Same pattern as MultiplayerEventSubsystem::HandleEnemyAttack
    // Uses ProcessEvent with FGetEnemyParams struct
}

AActor* FindPlayerActor(int32 CharacterId) const
{
    // Find BP_OtherPlayerManager and call GetPlayerActor(CharacterId)
    // Same pattern as enemy lookup
}
```

### HandleAutoAttackStarted

```
HandleAutoAttackStarted(Data):
    1. Parse: targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs
    2. Get UTargetingSubsystem
    3. Set AttackRange on targeting subsystem
    4. Set bIsAutoAttacking = true on targeting subsystem
    5. If LockedTarget is not already set:
       Resolve target actor and call SetLockedTarget()
    6. Show target frame:
       TargetFrameTargetId = targetId
       TargetFrameTargetName = targetName
       bTargetFrameIsEnemy = isEnemy
       ShowTargetFrame()  // creates STargetFrameWidget if not exists, adds to viewport at Z=9
    7. Log: "Auto-attack started on [targetName] (range: [attackRange], ASPD: [aspd])"
```

### HandleAutoAttackStopped

```
HandleAutoAttackStopped(Data):
    1. Parse: reason (string)
    2. Get UTargetingSubsystem
    3. Set bIsAutoAttacking = false
    4. Clear locked target
    5. HideTargetFrame()  // remove STargetFrameWidget from viewport
    6. Log: "Auto-attack stopped: [reason]"
```

### HandleTargetLost

```
HandleTargetLost(Data):
    1. Parse: reason (string), isEnemy (bool)
    2. Get UTargetingSubsystem
    3. Set bIsAutoAttacking = false
    4. ClearTarget()
    5. HideTargetFrame()  // remove STargetFrameWidget from viewport
    6. Log: "Target lost: [reason]"
```

### HandleOutOfRange

```
HandleOutOfRange(Data):
    1. Parse: targetId, isEnemy, targetX, targetY, targetZ, distance, requiredRange
    2. If targetX/Y/Z are present:
       Get UTargetingSubsystem
       Call MoveTowardTarget(FVector(targetX, targetY, targetZ))
    3. MoveTowardTarget implementation:
       APlayerController* PC = GetWorld()->GetFirstPlayerController()
       APawn* Pawn = PC->GetPawn()
       UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, TargetLocation)
```

### HandleCombatDeath

```
HandleCombatDeath(Data):
    1. Parse: killedId, killedName, killerId, killerName, isEnemy,
             targetHealth, targetMaxHealth, timestamp
    2. If killedId == LocalCharacterId:
       a. Show death overlay (Slate widget at Z=40)
       b. Death overlay contains:
          - "You have been defeated" text
          - Timer countdown (5 seconds, matching RESPAWN_DELAY_MS)
          - "Respawn" button (calls RequestRespawn on MultiplayerEventSubsystem)
       c. Stop auto-attack via TargetingSubsystem
       d. Disable movement input (optional, server already blocks dead players)
    3. If killedId is a remote player (not local, not enemy):
       // Future: play death animation on AMMORemotePlayer
       // For now: BP_OtherPlayerManager handles this
    4. If isEnemy:
       // Enemy death is handled by BP_EnemyManager via enemy:death event
       // This handler does nothing for enemy deaths
```

### HandleCombatRespawn

```
HandleCombatRespawn(Data):
    1. Parse: characterId, characterName, health, maxHealth, mana, maxMana,
             x, y, z, teleport (bool), timestamp
    2. If characterId == LocalCharacterId:
       a. Hide death overlay
       b. Teleport pawn to (x, y, z):
          APawn* Pawn = PC->GetPawn()
          Pawn->SetActorLocation(FVector(x, y, z))
       c. Apply ground snap:
          ZoneTransitionSubsystem::SnapLocationToGround(Pawn)
       d. Update health/mana on GameInstance character data
       e. BasicInfoSubsystem will auto-update from combat:health_update
    3. If characterId is remote player:
       // Future: teleport AMMORemotePlayer actor
       // For now: handled by BP_OtherPlayerManager
```

### HandleCombatError

```
HandleCombatError(Data):
    1. Parse: message (string)
    2. Log warning: "Combat error: [message]"
    3. Future: display in chat widget
```

### HandleHealthUpdate

```
HandleHealthUpdate(Data):
    1. Parse: characterId, health, maxHealth, mana, maxMana
    2. BasicInfoSubsystem handles local player HP/SP display (already in C++)
    3. WorldHealthBarSubsystem handles floating enemy HP bars (already in C++)
    4. CombatActionSubsystem updates the TARGET FRAME if this is the locked target:
       If bTargetFrameVisible AND characterId == TargetFrameTargetId:
           TargetFrameHP = health
           TargetFrameMaxHP = maxHealth
           // Widget reads these via TAttribute lambdas -- auto-updates
```

### Target Frame Widget (STargetFrameWidget)

Appears when auto-attack starts on a target. Shows the target's name, level, and HP bar.
Disappears when attack stops, target lost, or target dies. Z-order: 9 (above WorldHealthBar Z=8, below BasicInfo Z=10).

```cpp
// RO Classic style target frame — compact panel above health bars
class STargetFrameWidget : public SCompoundWidget
{
    SLATE_BEGIN_ARGS(STargetFrameWidget) : _Subsystem(nullptr) {}
        SLATE_ARGUMENT(UCombatActionSubsystem*, Subsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        // 3-layer gold/dark/brown frame (same pattern as all RO widgets)
        // Layout:
        //   [Target Name]          (gold text, bold, 9pt)
        //   [====HP Bar=====]      (red fill, dark bg, 14px height)
        //     "1234 / 5678"        (white text centered on bar)
        //
        // Width: 180px fixed
        // Position: top-center of screen, offset down 40px
        // Data binding via TAttribute lambdas reading from Subsystem
    }
};

// Public API on UCombatActionSubsystem:
void ShowTargetFrame();
    // 1. Create STargetFrameWidget if not exists
    // 2. Wrap in alignment SBox (HAlign_Center, VAlign_Top, padding top 40px)
    // 3. AddViewportWidgetContent at Z=9
    // 4. bTargetFrameVisible = true

void HideTargetFrame();
    // 1. Remove from viewport
    // 2. bTargetFrameVisible = false
    // 3. Clear target data

void UpdateTargetFrameHealth(float HP, float MaxHP);
    // Called from HandleHealthUpdate when characterId matches target
    // Widget auto-updates via TAttribute lambdas
```

**Target frame shows:**
- Target name (from `combat:auto_attack_started` payload `targetName`)
- HP bar with numeric overlay (from `combat:health_update` / `enemy:health_update`)
- RO Classic brown/gold theme (matches existing widgets)

**Target frame hides on:**
- `combat:auto_attack_stopped`
- `combat:target_lost`
- `combat:death` (if target died)
- Player clicks ground (stop attack)

### Death Overlay Widget

A simple Slate widget, not a full subsystem widget. Created lazily on first death.

```cpp
// Minimal death overlay — centered text with respawn button
class SDeathOverlayWidget : public SCompoundWidget
{
    SLATE_BEGIN_ARGS(SDeathOverlayWidget) {}
        SLATE_EVENT(FOnClicked, OnRespawnClicked)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetCountdown(float SecondsRemaining);

private:
    TSharedPtr<STextBlock> CountdownText;
    FOnClicked RespawnCallback;
};
```

The death overlay is added/removed from the viewport at Z=40 (between skill tree Z=20 and loading overlay Z=50).

---

## 7. Server Event Payloads (Exact Format)

All payloads documented from actual server code in `server/src/index.js`.

### combat:attack (Client -> Server)

```json
{
    "targetEnemyId": 2000001,
    // OR
    "targetCharacterId": "12345"
}
```

Emitted by `MultiplayerEventSubsystem::EmitCombatAttack()` (already exists).

### combat:auto_attack_started (Server -> Client, unicast)

```json
{
    "targetId": 2000001,
    "targetName": "Poring",
    "isEnemy": true,
    "attackRange": 150,
    "aspd": 175,
    "attackIntervalMs": 873
}
```

### combat:auto_attack_stopped (Server -> Client, unicast)

```json
{
    "reason": "Player stopped"
}
```

Possible `reason` values:
- `"Player stopped"` -- player sent `combat:stop_attack`
- `"Switched target"` -- player attacked a different target
- `"You died"` -- attacker died
- `"Overweight"` -- weight >= 90%

When reason is `"Switched target"`, additional fields:
```json
{
    "reason": "Switched target",
    "oldTargetId": 2000003,
    "oldIsEnemy": true
}
```

### combat:damage (Server -> Zone broadcast)

```json
{
    "attackerId": 12345,
    "attackerName": "PlayerName",
    "targetId": 2000001,
    "targetName": "Poring",
    "isEnemy": true,
    "damage": 150,
    "isCritical": false,
    "isMiss": false,
    "hitType": "normal",
    "element": "neutral",
    "damage2": 0,
    "isDualWield": false,
    "isCritical2": false,
    "element2": "neutral",
    "targetHealth": 850,
    "targetMaxHealth": 1000,
    "attackerX": 100.0, "attackerY": 200.0, "attackerZ": 300.0,
    "targetX": 150.0, "targetY": 250.0, "targetZ": 300.0,
    "timestamp": 1710000000000
}
```

`hitType` values: `"normal"`, `"critical"`, `"miss"`, `"dodge"`, `"perfectDodge"`, `"heal"`

### combat:target_lost (Server -> Client, unicast)

```json
{
    "reason": "Enemy died",
    "isEnemy": true
}
```

Possible `reason` values:
- `"Enemy died"` -- target HP reached 0
- `"Enemy gone"` -- target despawned
- `"Target died"` -- PvP target died
- `"Target disconnected"` -- PvP target logged out
- `"Target respawned"` -- PvP target respawned
- `"PvP is currently disabled"` -- server blocked PvP

### combat:out_of_range (Server -> Client, unicast)

```json
{
    "targetId": 2000001,
    "isEnemy": true,
    "targetX": 500.0,
    "targetY": 600.0,
    "targetZ": 300.0,
    "distance": 350.5,
    "requiredRange": 100
}
```

Also sent for skill range failures with different fields:
```json
{
    "skillId": "bash",
    "targetId": 2000001,
    "distance": 400,
    "maxRange": 150
}
```

### combat:death (Server -> Zone broadcast)

```json
{
    "killedId": 12345,
    "killedName": "PlayerName",
    "killerId": 2000001,
    "killerName": "Poring",
    "isEnemy": false,
    "targetHealth": 0,
    "targetMaxHealth": 1500,
    "timestamp": 1710000000000
}
```

**Note**: `isEnemy` refers to whether the KILLED entity is an enemy. For player death, `isEnemy: false`. For enemy kills by players, enemy death is handled by `enemy:death` event, NOT `combat:death`.

### combat:respawn (Server -> Zone broadcast)

```json
{
    "characterId": 12345,
    "characterName": "PlayerName",
    "health": 1500,
    "maxHealth": 1500,
    "mana": 200,
    "maxMana": 200,
    "x": 0.0,
    "y": 0.0,
    "z": 300.0,
    "teleport": true,
    "timestamp": 1710000000000
}
```

### combat:error (Server -> Client, unicast)

```json
{
    "message": "Target not found"
}
```

### combat:stop_attack (Client -> Server)

```json
{}
```

Emitted by `MultiplayerEventSubsystem::EmitStopAttack()` (already exists).

---

## 8. Integration with Existing C++ Subsystems

### Subsystems That Also Handle combat:damage

The `EventRouter` supports multiple handlers per event. After this migration, `combat:damage` will have FOUR registered handlers:

| Subsystem | Registered For | What It Handles |
|-----------|---------------|-----------------|
| `DamageNumberSubsystem` | `combat:damage`, `skill:effect_damage` | Floating damage numbers (spawn, position, color, crit text) |
| `WorldHealthBarSubsystem` | `combat:damage` | Enemy/player HP bar updates |
| `BasicInfoSubsystem` | `combat:damage`, `combat:health_update` | Local player HP/SP/EXP bar |
| **`CombatActionSubsystem` (NEW)** | `combat:damage` | Attack animation, actor rotation |

Each handler independently parses the JSON and handles its own concern. There is no ordering dependency between them. The EventRouter invokes all handlers for a given event in registration order, but all four are idempotent with respect to each other.

### Subsystems That Already Handle Other Combat Events

| Event | Already Handled By | Still Needed After Migration? |
|-------|-------------------|------------------------------|
| `combat:health_update` | BasicInfoSubsystem | Yes (local HP/SP update) |
| `skill:effect_damage` | DamageNumberSubsystem, SkillVFXSubsystem | Yes (skill damage numbers, VFX) |
| `skill:used` | SkillTreeSubsystem | Yes (skill cooldown/feedback) |
| `buff:applied` | BuffBarSubsystem | Yes (buff icon display) |

### Subsystems the New Code Reads From

| Subsystem | What CombatActionSubsystem Reads |
|-----------|----------------------------------|
| `UTargetingSubsystem` | LockedTarget, IsAutoAttacking |
| `UMultiplayerEventSubsystem` | EmitCombatAttack(), EmitStopAttack(), RequestRespawn() |
| `UMMOGameInstance` | LocalCharacterId, IsSocketConnected() |
| `USocketEventRouter` | RegisterHandler(), UnregisterAllForOwner() |

---

## 9. Event Deduplication & MultiplayerEventSubsystem Changes

### Current State

`MultiplayerEventSubsystem.cpp` currently bridges these combat events from EventRouter to BP_SocketManager via ProcessEvent:

```cpp
// Lines 53-71 in MultiplayerEventSubsystem.cpp
Router->RegisterHandler(TEXT("combat:damage"), this, ...);         // -> OnCombatDamage (BP)
Router->RegisterHandler(TEXT("combat:health_update"), this, ...);  // -> OnHealthUpdate (BP)
Router->RegisterHandler(TEXT("combat:death"), this, ...);          // -> OnCombatDeath (BP)
Router->RegisterHandler(TEXT("combat:respawn"), this, ...);        // -> OnCombatRespawn (BP)
Router->RegisterHandler(TEXT("combat:auto_attack_started"), this, ...);  // -> OnAutoAttackStarted (BP)
Router->RegisterHandler(TEXT("combat:auto_attack_stopped"), this, ...);  // -> OnAutoAttackStopped (BP)
Router->RegisterHandler(TEXT("combat:target_lost"), this, ...);    // -> OnTargetLost (BP)
Router->RegisterHandler(TEXT("combat:out_of_range"), this, ...);   // -> OnCombatOutOfRange (BP)
Router->RegisterHandler(TEXT("combat:error"), this, ...);          // -> OnCombatError (BP)
```

### After Migration

Remove these 9 registrations from `MultiplayerEventSubsystem.cpp`. The new `UCombatActionSubsystem` handles all of them directly in C++.

```cpp
// REMOVE from MultiplayerEventSubsystem::OnWorldBeginPlay:
// - combat:damage bridge        (now handled by CombatActionSubsystem + DamageNumberSubsystem + WorldHealthBarSubsystem + BasicInfoSubsystem)
// - combat:health_update bridge (still handled by BasicInfoSubsystem, CombatActionSubsystem handles target frame)
// - combat:death bridge         (now handled by CombatActionSubsystem)
// - combat:respawn bridge       (now handled by CombatActionSubsystem)
// - combat:auto_attack_started  (now handled by CombatActionSubsystem)
// - combat:auto_attack_stopped  (now handled by CombatActionSubsystem)
// - combat:target_lost          (now handled by CombatActionSubsystem)
// - combat:out_of_range         (now handled by CombatActionSubsystem)
// - combat:error                (now handled by CombatActionSubsystem)
```

The corresponding BP_SocketManager functions become dead code:
- `OnCombatDamage` (215 nodes)
- `OnAutoAttackStarted` (32 nodes)
- `OnAutoAttackStopped` (27 nodes)
- `OnCombatOutOfRange` (30 nodes)
- `OnTargetLost` (27 nodes)
- `OnCombatDeath` (19 nodes)
- `OnCombatRespawn` (72 nodes)
- `OnCombatError` (~10 nodes)
- `OnHealthUpdate` (handled by BasicInfoSubsystem)

These BP functions should NOT be deleted yet. They remain as dead code until all combat functionality is verified working in C++. Delete them in a later cleanup pass.

### Events That Stay Bridged

These events remain in `MultiplayerEventSubsystem` because they are NOT part of this migration:

```
player:moved          -> OnPlayerMoved (BP)
player:left           -> OnPlayerLeft (BP)
enemy:spawn           -> OnEnemySpawn (BP)
enemy:move            -> OnEnemyMove (BP)
enemy:death           -> OnEnemyDeath (BP)
enemy:health_update   -> OnEnemyHealthUpdate (BP)
enemy:attack          -> HandleEnemyAttack (C++ in MultiplayerEventSubsystem)
inventory:data        -> OnInventoryData (BP)
inventory:used        -> OnItemUsed (BP)
inventory:equipped    -> OnItemEquipped (BP)
inventory:dropped     -> OnItemDropped (BP)
inventory:error       -> OnInventoryError (BP)
loot:drop             -> OnLootDrop (BP)
chat:receive          -> OnChatReceived (BP)
player:stats          -> OnPlayerStats (BP)
hotbar:data           -> OnHotbarData (BP)
hotbar:alldata        -> OnHotbarAllData (BP)
shop:data             -> OnShopData (BP)
shop:bought           -> OnShopBought (BP)
shop:sold             -> OnShopSold (BP)
shop:error            -> OnShopError (BP)
```

After this migration, `MultiplayerEventSubsystem` bridges 22 events (down from 31).

---

## 10. Files to Create

| File | Type | Purpose |
|------|------|---------|
| `client/SabriMMO/Source/SabriMMO/UI/TargetingSubsystem.h` | Header | UTargetingSubsystem class declaration |
| `client/SabriMMO/Source/SabriMMO/UI/TargetingSubsystem.cpp` | Impl | Hover trace, cursor, target locking, attack initiation |
| `client/SabriMMO/Source/SabriMMO/UI/CombatActionSubsystem.h` | Header | UCombatActionSubsystem class declaration |
| `client/SabriMMO/Source/SabriMMO/UI/CombatActionSubsystem.cpp` | Impl | Socket event handlers, animations, death overlay |

**Estimated LOC**: ~400 lines header, ~900 lines implementation (total across both subsystems).

---

## 11. Files to Modify

| File | Change |
|------|--------|
| `client/SabriMMO/Source/SabriMMO/UI/MultiplayerEventSubsystem.cpp` | Remove 9 combat event registrations (lines 53-71), reduce bridge count from 31 to 22 |
| `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.h` | Add `IA_Attack` input action UPROPERTY (if not already there from BP migration), add `HandleAttackInput()` declaration |
| `client/SabriMMO/Source/SabriMMO/SabriMMOCharacter.cpp` | Add `HandleAttackInput()` implementation that delegates to `UTargetingSubsystem::HandleLeftClick()`, bind in `SetupPlayerInputComponent()` |
| `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs` | Verify `AIModule` is in dependency list (needed for `UAIBlueprintHelperLibrary::SimpleMoveToLocation`) |

### Blueprint Changes (Phase 6 -- NOT Part of This Migration)

These Blueprint changes happen AFTER C++ is verified working:

| Blueprint | Change |
|-----------|--------|
| BP_MMOCharacter | Remove `AC_TargetingSystem` component |
| BP_MMOCharacter | Remove `IA_Attack` event graph nodes (replaced by C++) |
| BP_MMOCharacter | Remove `Event Tick` auto-attack range loop |
| BP_MMOCharacter | Remove `bIsAutoAttacking`, `TargetEnemyRef`, `TargetPlayerRef` variables |
| BP_EnemyCharacter | Ensure `"Enemy"` actor tag is set in Class Defaults |
| BP_OtherPlayerCharacter | Ensure `"OtherPlayer"` actor tag is set in Class Defaults |
| BP_SocketManager | (Dead code) OnCombatDamage, OnAutoAttackStarted, etc. can be deleted |

---

## 12. Implementation Steps

### Step 1: Create UTargetingSubsystem (Header + Stub)

1. Create `TargetingSubsystem.h` with full API declaration
2. Create `TargetingSubsystem.cpp` with lifecycle stubs
3. Implement `ShouldCreateSubsystem`, `OnWorldBeginPlay`, `Deinitialize`
4. Verify subsystem creates and logs on level load
5. **Test**: Launch game, check log for "TargetingSubsystem started"

### Step 2: Implement Hover Detection

1. Implement `PerformHoverTrace()` with 30Hz tick
2. Implement actor classification (tag-based or name-based)
3. Implement `UpdateCursor()` for cursor type switching
4. Implement `ShowHoverIndicator()` / `HideHoverIndicator()`
5. **Test**: Hover over enemy -> cursor changes to crosshair. Hover over NPC -> cursor changes. Move away -> cursor resets.

### Step 3: Implement Click-to-Attack

1. Add `IA_Attack` Enhanced Input action to `ASabriMMOCharacter` (or use existing one)
2. Bind to `HandleAttackInput()` which calls `UTargetingSubsystem::HandleLeftClick()`
3. Implement `HandleLeftClick()` with target type switch
4. Implement `StartAutoAttack()` calling `EmitCombatAttack()`
5. Implement `StopAutoAttack()` calling `EmitStopAttack()`
6. **Test**: Click enemy -> `combat:attack` emitted (check server log)

### Step 4: Create UCombatActionSubsystem (Header + Stub)

1. Create `CombatActionSubsystem.h` with full API declaration
2. Create `CombatActionSubsystem.cpp` with lifecycle stubs
3. Register all 9 event handlers via EventRouter
4. Implement `bReadyToProcess` one-frame deferral
5. **Test**: Launch game, check log for event registration

### Step 5: Implement HandleAutoAttackStarted/Stopped/TargetLost

1. Implement `HandleAutoAttackStarted` -> sets attack range, confirms attack
2. Implement `HandleAutoAttackStopped` -> clears attack state
3. Implement `HandleTargetLost` -> clears target
4. **Test**: Attack enemy, confirm attack starts. Kill enemy, confirm target clears.

### Step 6: Implement HandleCombatDamage (Animation + Rotation)

1. Implement actor resolution (FindEnemyActor, FindPlayerActor)
2. Implement attacker rotation toward target
3. Implement attack animation playback via ProcessEvent("PlayAttackAnimation")
4. **Test**: Attack enemy, confirm character rotates and plays animation on each hit

### Step 7: Implement HandleOutOfRange

1. Implement `MoveTowardTarget()` using `SimpleMoveToLocation`
2. Implement `HandleOutOfRange` -> parse target position, move toward it
3. **Test**: Start attack, walk away from enemy, confirm character walks back

### Step 8: Implement HandleCombatDeath + HandleCombatRespawn

1. Create minimal `SDeathOverlayWidget` Slate widget
2. Implement `HandleCombatDeath` -> show overlay for local player death
3. Implement `HandleCombatRespawn` -> teleport, ground snap, hide overlay
4. **Test**: Die to enemy, confirm death overlay shows. Click respawn, confirm teleport.

### Step 9: Remove BP Bridge Events

1. Remove 9 combat event registrations from `MultiplayerEventSubsystem.cpp`
2. Update bridge count comment (31 -> 22)
3. **Test**: Full combat flow with BP handlers no longer called. Verify no regressions.

### Step 10: Verification & Cleanup

1. Run full testing checklist (section 13)
2. Verify no Blueprint combat handlers are being called (add UE_LOG to BP functions)
3. Document any edge cases found
4. Update `CLAUDE.md` with new subsystem entries

---

## 13. Testing Checklist

### Hover Detection

- [ ] Hover over enemy -> cursor changes to crosshair
- [ ] Hover over NPC -> cursor changes to talk icon
- [ ] Hover over ground item -> cursor changes to grab hand
- [ ] Hover over nothing -> cursor is default arrow
- [ ] Hover over enemy -> hover indicator appears at enemy feet
- [ ] Move cursor away from enemy -> hover indicator disappears
- [ ] Rapidly move cursor between enemies -> no indicator flicker/leak

### Click-to-Attack

- [ ] Click enemy -> `combat:attack` emitted to server (check server log)
- [ ] Click NPC -> NPC dialog opens (TryInteractWithNPC still works)
- [ ] Click ground while attacking -> `combat:stop_attack` emitted, attack stops
- [ ] Click different enemy while attacking -> target switches (old attack stopped, new started)

### Auto-Attack Flow

- [ ] Server confirms attack -> `combat:auto_attack_started` received
- [ ] Attack range updated from server value
- [ ] Each `combat:damage` -> attacker rotates toward target
- [ ] Each `combat:damage` -> attack animation plays on attacker
- [ ] Target dies -> `combat:target_lost` received, target cleared
- [ ] Target moves out of range -> `combat:out_of_range` received, player walks closer
- [ ] Player walks back in range -> attacks resume automatically (server handles this)

### Dual Wield

- [ ] Dual wield `combat:damage` with `damage2` -> second damage number spawns (DamageNumberSubsystem)
- [ ] Dual wield attack animation plays normally (single animation, two numbers)

### Death & Respawn

- [ ] Local player killed -> death overlay appears (Z=40)
- [ ] Death overlay shows respawn countdown
- [ ] Click respawn -> `combat:respawn` emitted
- [ ] Server confirms respawn -> pawn teleported to save point
- [ ] Death overlay hidden after respawn
- [ ] HP/SP restored after respawn (BasicInfoSubsystem updates)
- [ ] Ground snap applied at respawn location

### Event Deduplication

- [ ] `DamageNumberSubsystem` still spawns damage numbers (unaffected by migration)
- [ ] `WorldHealthBarSubsystem` still updates HP bars (unaffected)
- [ ] `BasicInfoSubsystem` still updates local HP/SP bar (unaffected)
- [ ] `BuffBarSubsystem` still shows buffs (unaffected)
- [ ] `CastBarSubsystem` still shows cast bars (unaffected)
- [ ] BP_SocketManager's `OnCombatDamage` is NOT called (add test log to verify)

### Edge Cases

- [ ] Enemy destroyed before `combat:damage` arrives -> no crash (WeakObjectPtr)
- [ ] Player disconnects mid-attack -> no crash
- [ ] Zone transition during auto-attack -> attack stops, subsystems reinitialize in new zone
- [ ] Attack during cast bar -> server rejects (client doesn't need to check)
- [ ] Attack while CC'd (stunned) -> server rejects
- [ ] Attack while overweight (>=90%) -> server sends `auto_attack_stopped` with reason "Overweight"

### Performance

- [ ] Hover trace at 30Hz, not 60Hz (check with stat unit)
- [ ] No per-frame Blueprint event graph execution for combat
- [ ] ProcessEvent calls reduced by 9 events (no more BP bridge for combat)

---

## 14. Risk Analysis & Rollback

### Risk: BP_MMOCharacter IA_Attack Conflict

**Risk**: Both the existing BP `IA_Attack` event and the new C++ `HandleAttackInput()` fire on the same input.

**Mitigation**: During development, the C++ handler calls `UTargetingSubsystem::HandleLeftClick()`. The BP `IA_Attack` still runs but its logic is gated behind `bIsAutoAttacking` which is no longer set by the BP. Once verified, the BP event graph nodes are removed (Phase 6).

**Alternative**: Add a bool `bUseCPPTargeting` on GameInstance. C++ handler checks this before acting. BP handler checks the inverse. Allows toggling between old and new systems.

### Risk: Actor Classification Fails

**Risk**: Tag-based or name-based actor classification misidentifies actors.

**Mitigation**: Start with conservative classification (check multiple signals: tag + class name + component presence). Log every classification decision at Verbose level. Add fallthrough to `ETargetType::None` for unrecognized actors.

### Risk: Animation Playback Differences

**Risk**: C++ `ProcessEvent("PlayAttackAnimation")` behaves differently than BP direct function call.

**Mitigation**: The `PlayAttackAnimation` function on BP_EnemyCharacter and BP_MMOCharacter is a simple wrapper around `PlayAnimMontage`. If ProcessEvent fails, fall back to `Cast<ACharacter>` and call `PlayAnimMontage` directly with a default attack montage.

### Risk: Death Overlay Interaction with Existing UI

**Risk**: Death overlay at Z=40 blocks input to other widgets.

**Mitigation**: Death overlay should be hit-test transparent except for the respawn button. Use `EVisibility::SelfHitTestInvisible` on the background, `EVisibility::Visible` on the button.

### Rollback Plan

If the migration causes critical issues:

1. **Instant rollback**: Re-add the 9 combat events to `MultiplayerEventSubsystem.cpp` (restore lines 53-71)
2. **Disable new subsystems**: Add `return false` to `ShouldCreateSubsystem` in both new subsystems
3. **Remove C++ input binding**: Comment out `HandleAttackInput` binding in `SetupPlayerInputComponent`
4. **Result**: System reverts to Blueprint-only combat handling with no code deletion

The rollback does NOT require deleting any new files. Both systems can coexist temporarily because EventRouter supports multiple handlers for the same event. The only conflict is input handling (IA_Attack), which is resolved by the `bUseCPPTargeting` gate.

---

## Appendix A: File Structure After Migration

```
client/SabriMMO/Source/SabriMMO/
    UI/
        TargetingSubsystem.h           NEW
        TargetingSubsystem.cpp         NEW
        CombatActionSubsystem.h        NEW
        CombatActionSubsystem.cpp      NEW
        MultiplayerEventSubsystem.h    (unchanged)
        MultiplayerEventSubsystem.cpp  MODIFIED (remove 9 event bridges)
        DamageNumberSubsystem.h/.cpp   (unchanged, still handles combat:damage numbers)
        WorldHealthBarSubsystem.h/.cpp (unchanged, still handles HP bars)
        BasicInfoSubsystem.h/.cpp      (unchanged, still handles local stats)
        CombatStatsSubsystem.h/.cpp    (unchanged)
        ... (all other subsystems unchanged)
    SabriMMOCharacter.h               MODIFIED (add HandleAttackInput)
    SabriMMOCharacter.cpp             MODIFIED (bind HandleAttackInput to IA_Attack)
```

## Appendix B: Subsystem Z-Order Registry (Updated)

| Z | Subsystem | Notes |
|---|-----------|-------|
| 5 | LoginFlowSubsystem | Login screen widgets |
| 8 | WorldHealthBarSubsystem | Floating HP/SP bars |
| 10 | BasicInfoSubsystem | HP/SP/EXP bars |
| 11 | BuffBarSubsystem | Status/buff icons |
| 12 | CombatStatsSubsystem | F8 stat panel |
| 14 | InventorySubsystem | F6 inventory |
| 15 | EquipmentSubsystem | F7 equipment |
| 16 | HotbarSubsystem | F5 hotbar (keybind Z=30) |
| 18 | ShopSubsystem | NPC shop |
| 19 | KafraSubsystem | Kafra dialog |
| 20 | SkillTreeSubsystem | Skill tree |
| 20 | DamageNumberSubsystem | Damage numbers overlay |
| 22 | ItemInspectSubsystem | Item inspect popup |
| 23 | SCardCompoundPopup | Card compound dialog |
| 25 | CastBarSubsystem | Cast bar |
| **40** | **CombatActionSubsystem (death overlay)** | **NEW -- death screen** |
| 50 | ZoneTransitionSubsystem / LoginFlowSubsystem (loading) | Loading overlay |

## Appendix C: Enhanced Input Action Binding Pattern

The `IA_Attack` input action binding in `ASabriMMOCharacter::SetupPlayerInputComponent` follows the exact same pattern as existing UI toggle keys:

```cpp
// In CreateUIToggleActions():
IA_AttackAction = NewObject<UInputAction>(this, TEXT("IA_AttackAction"));
IA_AttackAction->ValueType = EInputActionValueType::Boolean;
UIToggleIMC->MapKey(IA_AttackAction, EKeys::LeftMouseButton);

// In SetupPlayerInputComponent():
if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
{
    EIC->BindAction(IA_AttackAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleAttackInput);
}
```

**Important**: The existing BP_MMOCharacter already has an `IA_Attack` input action defined in the Blueprint's Input Mapping Context. The C++ action must use a different name (`IA_AttackAction`) or replace the BP one entirely. During the transition period, both can coexist if they use different IMC priorities. Set the C++ IMC priority higher to consume the input before the BP IMC sees it.
