# BPI_Damageable

## Overview

Blueprint Interface that defines a unified contract for damage and health display handling across all damageable actors in the game. Replaces class-specific Cast chains with interface message calls, following the Interface design pattern.

## Architecture

```
BP_SocketManager                          BPI_Damageable Interface
        │                                          │
        │  Does Object Implement Interface         │
        │           ↓                              │
        │  UpdateHealthDisplay ────────────────────┼──► BP_EnemyCharacter
        │  ReceiveDamageVisual ────────────────────┼──► BP_OtherPlayerCharacter
        │  GetHealthInfo ──────────────────────────┼──► BP_MMOCharacter (stub)
```

## Interface Functions

### ReceiveDamageVisual
**Type:** Event (with inputs)
**Purpose:** Triggers visual feedback when damage is dealt — rotates the actor toward the attacker.

| Input | Type | Description |
|-------|------|-------------|
| `Damage` | Integer | Amount of damage dealt (for potential damage number spawning) |
| `AttackerLocation` | Vector | World location of the attacker (for rotation calculation) |

**Implementation Notes:**
- Uses `Get Actor Location` → `Find Look at Rotation` → `Set Actor Rotation`
- Set `Teleport Physics = true` on `Set Actor Rotation`
- Optionally guard with `bIsDead` branch before rotating

---

### UpdateHealthDisplay
**Type:** Event (with inputs)
**Purpose:** Updates the health display widget and handles visibility based on combat state.

| Input | Type | Description |
|-------|------|-------------|
| `NewHealth` | Float | Current health value |
| `NewMaxHealth` | Float | Maximum health value |
| `InCombat` | Boolean | Whether the actor is in combat (controls health bar visibility) |

**Implementation Notes:**
- For enemies: `HealthBarWidget → Set Visibility (InCombat ? Visible : Hidden)`
- Calls existing health update function (e.g., `UpdateHealthDisplay` → `UpdateHealth`)
- Passes `NewHealth` and `NewMaxHealth` to the health widget

---

### GetHealthInfo
**Type:** Function (with outputs)
**Purpose:** Returns the health widget component for external access.

| Output | Type | Description |
|--------|------|-------------|
| `HealthWidget` | Widget Component Object Reference | Reference to the health bar widget component |

**Implementation Notes:**
- Simply returns the `HealthBarWidget` component reference
- Used by systems that need to access health display without casting

---

## Blueprint Implementations

### BP_EnemyCharacter

| Function | Implementation |
|----------|----------------|
| `ReceiveDamageVisual` | `Get Actor Location` → `Find Look at Rotation` (Start=ActorLocation, Target=AttackerLocation) → `Set Actor Rotation` (NewRotation=LookAtRotation, TeleportPhysics=true) |
| `UpdateHealthDisplay` | Calls existing `UpdateEnemyHealth(NewHealth, NewMaxHealth, InCombat)` function |
| `GetHealthInfo` | Returns `HealthBarWidget` component |

**Components Used:**
- `HealthBarWidget` (Widget Component, WBP_TargetHealthBar)
- `NameTagWidget` (Widget Component, WBP_PlayerNameTag)

---

### BP_OtherPlayerCharacter

| Function | Implementation |
|----------|----------------|
| `ReceiveDamageVisual` | Same as BP_EnemyCharacter: Get Actor Location → Find Look at Rotation → Set Actor Rotation |
| `UpdateHealthDisplay` | `Select` (Boolean, True=Visible, False=Hidden) → `Set Visibility` on HealthBarWidget → `Update Health Bar` function |
| `GetHealthInfo` | Returns `HealthBarWidget` component |

**Components Used:**
- `HealthBarWidget` (Widget Component, WBP_TargetHealthBar)

---

### BP_MMOCharacter (Local Player)

| Function | Implementation |
|----------|----------------|
| `ReceiveDamageVisual` | Empty (no-op) — HUD updates handled elsewhere via `combat:health_update` |
| `UpdateHealthDisplay` | Empty (no-op) — HUD updates handled elsewhere via `combat:health_update` |
| `GetHealthInfo` | Returns None (no widget) |

**Notes:**
- Local player health is displayed on WBP_GameHud, not on a world-space widget
- Interface implementation is a stub to satisfy the contract

---

## BP_SocketManager Integration

### OnEnemyHealthUpdate (Refactored)

```
OnEnemyHealthUpdate (Data: String)
    ↓
Parse JSON: enemyId, health, maxHealth, inCombat
    ↓
Get Actor of Class → BP_EnemyManager → Cast to BP_EnemyManager
    ↓
GetEnemyActor(EnemyId) → EnemyActor, WasFound
    ↓
Branch: WasFound?
    ├─ TRUE:
    │   Does Object Implement Interface (EnemyActor, BPI_Damageable)
    │       ↓
    │   Branch: Result?
    │       ├─ TRUE:
    │       │   UpdateHealthDisplay (Message)
    │       │       Target: EnemyActor
    │       │       NewHealth: health
    │       │       NewMaxHealth: maxHealth
    │       │       InCombat: inCombat
    │       └─ FALSE: Print String "Enemy doesn't implement BPI_Damageable"
    └─ FALSE: (skip)
    ↓
★ Existing HUD target frame update logic unchanged
```

---

### OnCombatDamage (Refactored)

```
OnCombatDamage (Data: String)
    ↓
Parse JSON: attackerId, targetId, isEnemy, damage, targetHealth, targetMaxHealth, attackerX, attackerY, attackerZ
    ↓
Branch: isEnemy?
    ├─ TRUE (enemy target):
    │   Get Actor of Class → BP_EnemyManager → Cast
    │       ↓
    │   GetEnemyActor(targetId) → EnemyActor, WasFound
    │       ↓
    │   Branch: WasFound?
    │       ├─ TRUE:
    │       │   Does Object Implement Interface (EnemyActor, BPI_Damageable)
    │       │       ↓
    │       │   Branch: Result?
    │       │       ├─ TRUE:
    │       │       │   UpdateHealthDisplay (Message) → EnemyActor
    │       │       │   ReceiveDamageVisual (Message) → EnemyActor
    │       │       │       Damage: damage
    │       │       │       AttackerLocation: Make Vector(attackerX, attackerY, attackerZ)
    │       │       └─ FALSE: (skip)
    │       └─ FALSE: (skip)
    │   ★ Existing HUD UpdateTargetHealth call unchanged
    │
    └─ FALSE (player target):
        Get Game Instance → MMOGameInstance → GetCurrentCharacterId → LocalCharId
            ↓
        Branch: targetId == LocalCharId?
            ├─ TRUE (local player):
            │   Get Player Character → Does Object Implement Interface
            │       ↓
            │   Branch: Result?
            │       ├─ TRUE:
            │       │   UpdateHealthDisplay (Message) → PlayerCharacter
            │       │   ReceiveDamageVisual (Message) → PlayerCharacter
            │       └─ FALSE: (skip)
            │
            └─ FALSE (remote player):
                Get Actor of Class → BP_OtherPlayerManager → Cast
                    ↓
                GetOtherPlayerActor(targetId) → PlayerActor, WasFound
                    ↓
                Branch: WasFound?
                    ├─ TRUE:
                    │   Does Object Implement Interface (PlayerActor, BPI_Damageable)
                    │       ↓
                    │   Branch: Result?
                    │       ├─ TRUE:
                    │       │   UpdateHealthDisplay (Message) → PlayerActor
                    │       │   ReceiveDamageVisual (Message) → PlayerActor
                    │       └─ FALSE: (skip)
                    └─ FALSE: (skip)
```

---

## BP_OtherPlayerManager — GetOtherPlayerActor

Added function to safely retrieve remote player actor references:

```
Function: GetOtherPlayerActor
    Input: CharacterId (Integer)
    Output: PlayerActor (BP_OtherPlayerCharacter Object Reference), WasFound (Boolean)

    ↓
OtherPlayers Map → Find (Key: CharacterId)
    ↓
Branch: Was Found?
    ├─ TRUE:
    │   Cast to BP_OtherPlayerCharacter (Return Value)
    │       ↓
    │   Branch: (Cast Succeeded)
    │       ├─ TRUE: Set PlayerActor = Cast Result, Set WasFound = true
    │       └─ FALSE: Set WasFound = false
    └─ FALSE: Set WasFound = false
```

---

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| **Interface** | `BPI_Damageable` defines contract for damageable objects, replacing Cast chains |
| **Event-Driven** | `ReceiveDamageVisual` and `UpdateHealthDisplay` are events, not queries |
| **Manager** | `BP_EnemyManager` and `BP_OtherPlayerManager` coordinate groups of actors |

---

## Anti-Patterns Addressed

| Anti-Pattern | Before | After |
|--------------|--------|-------|
| Cast Chain | `Cast To BP_EnemyCharacter` → call `UpdateEnemyHealth` | `Does Object Implement Interface` → `UpdateHealthDisplay (Message)` |
| Class-Specific Calls | Direct function calls on specific Blueprint classes | Interface message calls work on any implementing class |

---

## Related Files

| File | Purpose |
|------|---------|
| `Content/Blueprints/BPI_Damageable.uasset` | Blueprint Interface definition |
| `Content/Blueprints/BP_EnemyCharacter.uasset` | Implements interface for enemies |
| `Content/Blueprints/BP_OtherPlayerCharacter.uasset` | Implements interface for remote players |
| `Content/Blueprints/BP_MMOCharacter.uasset` | Implements interface (stub) for local player |
| `Content/Blueprints/BP_SocketManager.uasset` | Uses interface messages instead of Cast |
| `Content/Blueprints/BP_EnemyManager.uasset` | Manages enemy actors |
| `Content/Blueprints/BP_OtherPlayerManager.uasset` | Manages remote player actors, added GetOtherPlayerActor |
| `docs/Enemy_Combat_System.md` | Enemy combat system overview |
| `docs/SocketIO_RealTime_Multiplayer.md` | Socket.io event reference |
| `docs/UI_Widgets.md` | Health bar widget documentation |

---

## Troubleshooting

### Issue: Interface functions not firing
**Check:**
1. Blueprint implements `BPI_Damageable` in Class Settings → Interfaces
2. Blueprint is Compiled after adding interface
3. `Does Object Implement Interface` returns true before calling interface message

### Issue: Health bar not updating
**Check:**
1. `HealthBarWidget` component exists on the actor
2. Widget class is `WBP_TargetHealthBar`
3. `UpdateHealth` function exists in the widget with correct input pins

### Issue: Actor not rotating toward attacker
**Check:**
1. `Find Look at Rotation` has correct Start (actor location) and Target (attacker location) pins
2. `Set Actor Rotation` has `Teleport Physics = true`
3. Actor is not dead (`bIsDead` check if applicable)

---

**Last Updated**: 2026-02-17
**Version**: 1.0
**Status**: Complete
