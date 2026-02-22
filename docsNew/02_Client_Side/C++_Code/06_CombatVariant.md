# Variant_Combat — Complete Documentation

## Overview

The Combat variant provides a melee combat system with combo attacks, charged attacks, damage/knockback, death/respawn, and AI enemies. It is located in `Source/SabriMMO/Variant_Combat/`.

**Note**: This is a template combat system from the UE5 template, separate from the MMO's server-authoritative combat. The MMO combat is handled via Socket.io events in Blueprints. This variant provides local/offline combat mechanics.

---

## ACombatCharacter

**File**: `CombatCharacter.h` (335 lines), `CombatCharacter.cpp` (548 lines)  
**Parent**: `ACharacter`  
**Interfaces**: `ICombatAttacker`, `ICombatDamageable`  
**UCLASS**: `abstract`

### Components

| Component | Type | Description |
|-----------|------|-------------|
| `CameraBoom` | USpringArmComponent | Camera arm with lag enabled |
| `FollowCamera` | UCameraComponent | Orbiting camera |
| `LifeBar` | UWidgetComponent | Overhead health bar |

### Combat Properties

| Property | Category | Default | Description |
|----------|----------|---------|-------------|
| `MaxHP` | Damage | 5.0 | Maximum hit points |
| `CurrentHP` | Damage | 0.0 | Current hit points (set in BeginPlay) |
| `LifeBarColor` | Damage | — | Fill color for health bar |
| `PelvisBoneName` | Damage | — | Bone for ragdoll physics |
| `MeleeTraceDistance` | Melee Attack\|Trace | 75 cm | Sphere sweep distance |
| `MeleeTraceRadius` | Melee Attack\|Trace | 75 cm | Sphere sweep radius |
| `DangerTraceDistance` | Melee Attack\|Trace | 300 cm | Danger notification distance |
| `DangerTraceRadius` | Melee Attack\|Trace | 100 cm | Danger notification radius |
| `MeleeDamage` | Melee Attack\|Damage | 1.0 | Damage per hit |
| `MeleeKnockbackImpulse` | Melee Attack\|Damage | 250 cm/s | Knockback force |
| `MeleeLaunchImpulse` | Melee Attack\|Damage | 300 cm/s | Upward launch force |

### Combo Attack Properties

| Property | Category | Default | Description |
|----------|----------|---------|-------------|
| `ComboAttackMontage` | Melee Attack\|Combo | — | AnimMontage for combos |
| `ComboSectionNames` | Melee Attack\|Combo | — | Section names per combo stage |
| `ComboInputCacheTimeTolerance` | Melee Attack\|Combo | 0.45s | Input buffer window for combos |
| `AttackInputCacheTimeTolerance` | Melee Attack | 1.0s | Input buffer for non-combo attacks |

### Charged Attack Properties

| Property | Category | Default | Description |
|----------|----------|---------|-------------|
| `ChargedAttackMontage` | Melee Attack\|Charged | — | AnimMontage for charged attacks |
| `ChargeLoopSection` | Melee Attack\|Charged | — | Loop animation section name |
| `ChargeAttackSection` | Melee Attack\|Charged | — | Release animation section name |

### Camera/Respawn Properties

| Property | Default | Description |
|----------|---------|-------------|
| `DeathCameraDistance` | 400 cm | Camera boom length when dead |
| `DefaultCameraDistance` | 100 cm | Camera boom length normally |
| `RespawnTime` | 3.0s | Time before respawn |

### Key Methods

- **`ComboAttack()`** — Plays combo montage, notifies enemies, sets end delegate
- **`ChargedAttack()`** — Plays charged montage with loop/release sections
- **`DoAttackTrace(FName)`** — Sphere sweep from bone socket, applies damage via `ICombatDamageable`
- **`CheckCombo()`** — Checks input cache, jumps to next combo section
- **`CheckChargedAttack()`** — Jumps to loop or attack section based on hold state
- **`ApplyDamage(...)`** — Reduces HP, applies knockback, enables partial ragdoll
- **`HandleDeath()`** — Disables movement, full ragdoll, hides life bar, schedules respawn
- **`TakeDamage(...)`** — Override of AActor; processes HP reduction, death check
- **`Landed(FHitResult&)`** — Resets ragdoll physics blend weight

### Blueprint Implementable Events
- `BP_ToggleCamera()` — Camera side switch animation
- `DealtDamage(float, FVector&)` — Play damage dealt effects
- `ReceivedDamage(float, FVector&, FVector&)` — Play damage received effects

---

## ACombatEnemy

**File**: `AI/CombatEnemy.h` (233 lines)  
**Parent**: `ACharacter`  
**Interfaces**: `ICombatAttacker`, `ICombatDamageable`  
**UCLASS**: `abstract`

AI-controlled enemy with combo and charged attacks, managed by StateTree via `ACombatAIController`.

### Key Differences from ACombatCharacter
- AI-initiated attacks via `DoAIComboAttack()` / `DoAIChargedAttack()`
- Random combo count (`TargetComboCount`) and charge loops (`TargetChargeLoops`)
- `FOnEnemyAttackCompleted` / `FOnEnemyLanded` delegates for StateTree
- `FOnEnemyDied` BlueprintAssignable delegate
- `DeathRemovalTime` (5s) — removes from level after death
- Danger tracking: `LastDangerLocation`, `LastDangerTime`

---

## Interfaces

### ICombatAttacker
```cpp
virtual void DoAttackTrace(FName DamageSourceBone) = 0;
virtual void CheckCombo() = 0;
virtual void CheckChargedAttack() = 0;
```
Provides animation-driven attack callbacks. Called from AnimNotifies during montages.

### ICombatDamageable
```cpp
virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) = 0;
virtual void HandleDeath() = 0;
virtual void ApplyHealing(float Healing, AActor* Healer) = 0;
virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) = 0;
```
Unified damage/healing/death interface for any damageable actor.

### ICombatActivatable
```cpp
virtual void ToggleInteraction(AActor* ActivationInstigator) = 0;
virtual void ActivateInteraction(AActor* ActivationInstigator) = 0;
virtual void DeactivateInteraction(AActor* ActivationInstigator) = 0;
```
Context-agnostic activation interface for interactive objects.

---

## Animation Notifies

| Class | Called When | Calls |
|-------|------------|-------|
| `AnimNotify_DoAttackTrace` | Attack animation hits | `ICombatAttacker::DoAttackTrace(BoneName)` |
| `AnimNotify_CheckCombo` | Between combo sections | `ICombatAttacker::CheckCombo()` |
| `AnimNotify_CheckChargedAttack` | Charge loop point | `ICombatAttacker::CheckChargedAttack()` |

---

## Gameplay Actors

| Class | Purpose |
|-------|---------|
| `CombatActivationVolume` | Trigger volume that activates `ICombatActivatable` actors |
| `CombatCheckpointVolume` | Updates respawn point when player enters |
| `CombatDamageableBox` | Destructible box implementing `ICombatDamageable` |
| `CombatDummy` | Training dummy (stationary damageable target) |
| `CombatLavaFloor` | Damage-dealing floor hazard |

---

## AI System

### ACombatAIController
- Has `UStateTreeAIComponent` for behavior
- Abstract class — configured in Blueprint

### ACombatEnemySpawner
- Spawns `ACombatEnemy` instances at configurable locations

### Environment Query Contexts
- `EnvQueryContext_Danger` — Query point at last danger location
- `EnvQueryContext_Player` — Query point at nearest player

---

## UCombatLifeBar

**File**: `UI/CombatLifeBar.h` (27 lines)  
**Parent**: `UUserWidget`  
**UCLASS**: `abstract`

```cpp
UFUNCTION(BlueprintImplementableEvent) void SetLifePercentage(float Percent);
UFUNCTION(BlueprintImplementableEvent) void SetBarColor(FLinearColor Color);
```

Implemented in Blueprint widget. Used by both `ACombatCharacter` and `ACombatEnemy`.

---

**Last Updated**: 2026-02-17
