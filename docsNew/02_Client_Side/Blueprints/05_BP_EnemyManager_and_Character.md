# BP_EnemyManager & BP_EnemyCharacter

## BP_EnemyManager

**Path**: `/Game/SabriMMO/Blueprints/BP_EnemyManager`  
**Parent Class**: `Actor`  
**Purpose**: Singleton manager that spawns, updates, and destroys enemy actors. Maintains a `Map<int, BP_EnemyCharacter>` keyed by enemy ID (server-assigned, 2000001+).

### Components

| Component | Type |
|-----------|------|
| `DefaultSceneRoot` | SceneComponent |

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `EnemyMap` | Map<int, object> | Map of enemyId → BP_EnemyCharacter actor ref |
| `EnemyCharacterRef` | object | Blueprint class to spawn (BP_EnemyCharacter) |
| `bWasFound` | bool | Temp flag for map lookups |

### Functions

| Function | Nodes | Params | Description |
|----------|-------|--------|-------------|
| `SpawnOrUpdateEnemy` | **101** | `EnemyId (int), TemplateId (string), Name (string), Level (int), Health/MaxHealth (float), X/Y/Z (float), IsMoving (bool)` | If enemyId exists in map → update TargetPosition and bIsMoving. If not → SpawnActor at (X,Y,Z), call `InitializeEnemy()`, add to map |
| `RemoveEnemy` | 11 | `EnemyId (int)` | Find in map → DestroyActor → Remove from map |
| `GetEnemyActor` | 8 | `EnemyId (int)` | Find in map → return actor ref (or null) |

### Event Graph (3 nodes)
Empty — all logic in functions.

### Called By
- `BP_SocketManager.OnEnemySpawn` → `SpawnOrUpdateEnemy`
- `BP_SocketManager.OnEnemyMove` → `SpawnOrUpdateEnemy` (position update)
- `BP_SocketManager.OnEnemyDeath` → via `GetEnemyActor` + `OnEnemyDeath`
- `BP_SocketManager.OnEnemyHealthUpdate` → via `GetEnemyActor` + `UpdateEnemyHealth`

---

## BP_EnemyCharacter

**Path**: `/Game/SabriMMO/Blueprints/BP_EnemyCharacter`  
**Parent Class**: `Character`  
**Purpose**: Client-side enemy actor. Displays name tag, health bar, hover indicator. Interpolates toward server-provided position on Tick. Handles death visuals (disable collision, hide mesh).

### Components

| Component | Type | Purpose |
|-----------|------|---------|
| `NameTagWidget` | WidgetComponent | World-space name tag showing "EnemyName Lv.X" |
| `HoverOverIndicator` | WidgetComponent | Selection indicator shown on mouse hover |
| `HealthBarWidget` | WidgetComponent | Overhead WBP_TargetHealthBar |

### Interfaces Implemented

| Interface | Events |
|-----------|--------|
| `BPI_Damageable` | `ReceiveDamageVisual`, `UpdateHealthDisplay` |

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `EnemyId` | int | Server-assigned enemy ID (2000001+) |
| `EnemyName` | string | Display name (e.g., "Blobby") |
| `EnemyLevel` | int | Enemy level |
| `Health` | real | Current health |
| `MaxHealth` | real | Maximum health |
| `bIsActiveTarget` | bool | Whether this enemy is the local player's current target |
| `bIsDead` | bool | Whether this enemy is dead (prevents movement, shows death state) |
| `DeltaSeconds` | real | Cached delta time |
| `TargetPosition` | struct (Vector) | Server-provided position to interpolate toward |
| `FlatDirection` | struct (Vector) | XY direction to target |
| `bIsMoving` | bool | Whether actively moving toward target |

### Functions

| Function | Nodes | Params | Description |
|----------|-------|--------|-------------|
| `InitializeEnemy` | 23 | `EnemyId, Name, Level, Health, MaxHealth` | Sets all variables, name tag text as "Name Lv.X", initial health bar |
| `UpdateEnemyHealth` | 21 | `Health, MaxHealth, InCombat (bool)` | Updates health/maxHealth vars, health bar widget, shows/hides health bar based on combat state |
| `UpdateHealthBar` | 5 | `Current, Max (float)` | Updates WBP_TargetHealthBar percent |
| `OnEnemyDeath` | 18 | — | Sets bIsDead=true, disables collision, hides mesh/widgets, stops movement |
| `PlayAttackAnimation` | 4 | — | Plays attack montage |

### Event Graph (77 nodes)

#### Event BeginPlay
```
Event BeginPlay
    ↓
[Sequence]
    ├─ Pin 0: Set initial TargetPosition = GetActorLocation()
    │   (Prevents enemy from moving immediately after spawn)
    │
    ├─ Pin 1: Set Name Tag
    │   ↓ Delay
    │   Get NameTagWidget → Cast To WBP_PlayerNameTag
    │   → Set Player Name (EnemyName + " Lv." + EnemyLevel)
    │
    ├─ Pin 2: Set Initial Health Bar
    │   Get HealthBarWidget → Cast To WBP_TargetHealthBar
    │   → UpdateHealth(Health, MaxHealth)
    │   → Set Visibility = Hidden (hidden until combat)
    │
    └─ Pin 3: Set HoverOverIndicator Visibility = Hidden
```

#### Event Tick — Movement & Hover
```
Event Tick
    ↓
Set DeltaSeconds
    ↓
[Sequence]
    ├─ Pin 0: Movement
    │   Branch: bIsDead?
    │   ├─ TRUE: (skip — don't move dead enemies)
    │   └─ FALSE:
    │       Branch: bIsMoving?
    │       ├─ TRUE:
    │       │   FlatDirection = (TargetPosition - ActorLocation) with Z=0
    │       │   distance = VectorLength(FlatDirection)
    │       │   Branch: distance > 10?
    │       │   ├─ TRUE: Normalize → AddMovementInput
    │       │   └─ FALSE: Set bIsMoving = false
    │       └─ FALSE: (idle)
    │
    └─ Pin 1: HoverOverIndicator Visibility
        Branch: bIsActiveTarget?
        ├─ TRUE: HoverOverIndicator → Set Visibility = Visible
        └─ FALSE: HoverOverIndicator → Set Visibility = Hidden
```

#### Cursor Hover Events
```
Event ActorBeginCursorOver → HoverOverIndicator → Set Visibility = Visible
Event ActorEndCursorOver → HoverOverIndicator → Set Visibility = Hidden
```

#### BPI_Damageable Events
```
Event ReceiveDamageVisual (DamageSourceLocation: Vector)
    ↓
GetActorLocation → FindLookAtRotation → SetActorRotation
    (Rotates enemy to face attacker)

Event UpdateHealthDisplay (CurrentHealth: float, MaxHealth: float)
    ↓
UpdateEnemyHealth(CurrentHealth, MaxHealth)
```

### How Enemies Move (Wander)

1. Server enemy AI tick calculates wander positions and broadcasts `enemy:move`
2. `BP_SocketManager.OnEnemyMove` calls `EnemyManagerRef.SpawnOrUpdateEnemy(id, ..., x, y, z, isMoving)`
3. `SpawnOrUpdateEnemy` finds existing actor, sets `TargetPosition` and `bIsMoving`
4. On Tick, enemy moves via `AddMovementInput` (uses CharacterMovementComponent)
5. When `isMoving=false` received, enemy stops at final position

### How Enemies Die

1. Server broadcasts `enemy:death` with `{enemyId, killerId, ...}`
2. `BP_SocketManager.OnEnemyDeath` calls `GetEnemyActor(enemyId)` → `OnEnemyDeath()`
3. `OnEnemyDeath` sets `bIsDead=true`, disables collision (capsule + mesh), hides all widgets
4. Actor remains in EnemyMap (not destroyed — it will be reused on respawn)
5. When `enemy:spawn` arrives for same ID, `SpawnOrUpdateEnemy` reinitializes the actor

### Connections

| Used By | Function Called |
|---------|----------------|
| `BP_SocketManager.OnEnemySpawn` | InitializeEnemy (via SpawnOrUpdateEnemy) |
| `BP_SocketManager.OnEnemyMove` | Set TargetPosition, bIsMoving |
| `BP_SocketManager.OnEnemyDeath` | OnEnemyDeath |
| `BP_SocketManager.OnEnemyHealthUpdate` | UpdateEnemyHealth |
| `BP_SocketManager.OnCombatDamage` | BPI_Damageable messages |
| `BP_MMOCharacter` | IA_Attack → Cast To BP_EnemyCharacter → read EnemyId |
| `AC_TargetingSystem` | UpdateHoverDetection → hover visibility |

---

**Last Updated**: 2026-02-17  
**Source**: Read via unrealMCP `read_blueprint_content`
