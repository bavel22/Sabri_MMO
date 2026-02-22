# BPI_Damageable — Blueprint Interface

**Path**: `/Game/SabriMMO/Interfaces/BPI_Damageable`  
**Parent Class**: `Interface`  
**Purpose**: Unified damage/health interface for any actor that can receive damage and display health. Decouples the combat system from specific actor types — BP_SocketManager can call damage visuals on any actor implementing this interface without knowing if it's a player, remote player, or enemy.

## Functions (3)

### ReceiveDamageVisual

```
ReceiveDamageVisual(DamageSourceLocation: Vector)
```

**Purpose**: Called when this actor receives damage. The actor should rotate to face the attacker and play any damage-received visual effects.

**Parameters**:
| Param | Type | Description |
|-------|------|-------------|
| `DamageSourceLocation` | Vector | World position of the attacker |

**Implementations**:

| Blueprint | What It Does |
|-----------|-------------|
| `BP_MMOCharacter` | Event stub (logic handled via BP_SocketManager.OnCombatDamage) |
| `BP_OtherPlayerCharacter` | `GetActorLocation → FindLookAtRotation(Self, Source) → SetActorRotation` — rotates remote player to face attacker |
| `BP_EnemyCharacter` | `GetActorLocation → FindLookAtRotation(Self, Source) → SetActorRotation` — rotates enemy to face attacker |

### UpdateHealthDisplay

```
UpdateHealthDisplay(CurrentHealth: float, MaxHealth: float)
```

**Purpose**: Called when this actor's health changes. The actor should update its overhead health bar widget.

**Parameters**:
| Param | Type | Description |
|-------|------|-------------|
| `CurrentHealth` | float | New current HP |
| `MaxHealth` | float | Maximum HP |

**Implementations**:

| Blueprint | What It Does |
|-----------|-------------|
| `BP_MMOCharacter` | Event stub (health display handled by AC_HUDManager) |
| `BP_OtherPlayerCharacter` | `UpdateHealthBar(Current, Max)` + show/hide HealthBarWidget based on HP < MaxHP |
| `BP_EnemyCharacter` | `UpdateEnemyHealth(Current, Max)` — updates health bar and combat indicator |

### GetHealthInfo

```
GetHealthInfo() → returns (CurrentHealth: float, MaxHealth: float)
```

**Purpose**: Query current health values from the actor.

**Implementations**: Currently minimal (1-2 nodes). Return stubs only — health data primarily flows from server events, not from actor queries.

## How It's Used in BP_SocketManager

The interface is invoked via **Message** nodes (not direct function calls) which safely handle actors that may or may not implement the interface:

```
// In OnCombatDamage (215 nodes):
Does Object Implement Interface (Target, BPI_Damageable)
    ├─ TRUE:
    │   UpdateHealthDisplay (Target, TargetHealth, TargetMaxHealth)
    │   ReceiveDamageVisual (Target, MakeVector(AttackerX, AttackerY, AttackerZ))
    └─ FALSE: (skip — target doesn't support damage visuals)
```

This pattern appears 3 times in `OnCombatDamage`:
1. When target is **local player** (`GetPlayerCharacter`)
2. When target is **enemy** (`TargetEnemyRef`)
3. When target is **remote player** (`OtherPlayerTargetRef`)

## Implementors

| Blueprint | Has ReceiveDamageVisual | Has UpdateHealthDisplay | Has GetHealthInfo |
|-----------|------------------------|------------------------|-------------------|
| `BP_MMOCharacter` | ✅ (stub) | ✅ (stub) | ✅ (stub) |
| `BP_OtherPlayerCharacter` | ✅ (rotate to face) | ✅ (update health bar) | ✅ (stub) |
| `BP_EnemyCharacter` | ✅ (rotate to face) | ✅ (update enemy health) | ✅ (stub) |

## Design Pattern: Interface

This interface replaces what would otherwise be a long Cast chain:
```
// WITHOUT interface (anti-pattern):
Cast To BP_OtherPlayerCharacter → Update Health Bar
Cast To BP_EnemyCharacter → Update Enemy Health
Cast To BP_MMOCharacter → Update Local Health

// WITH interface (correct):
Does Implement BPI_Damageable? → UpdateHealthDisplay(HP, MaxHP)
```

One message call handles all three actor types uniformly.

---

**Last Updated**: 2026-02-17  
**Source**: Read via unrealMCP `read_blueprint_content`
