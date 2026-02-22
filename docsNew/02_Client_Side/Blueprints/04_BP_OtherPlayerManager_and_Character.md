# BP_OtherPlayerManager & BP_OtherPlayerCharacter

## BP_OtherPlayerManager

**Path**: `/Game/SabriMMO/Blueprints/BP_OtherPlayerManager`  
**Parent Class**: `Actor`  
**Purpose**: Singleton manager that spawns, updates, and destroys remote player actors. Maintains a `Map<int, BP_OtherPlayerCharacter>` keyed by character ID.

### Components

| Component | Type |
|-----------|------|
| `DefaultSceneRoot` | SceneComponent |

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `OtherPlayerClassActorRef` | class (subclass of BP_OtherPlayerCharacter) | Blueprint class to spawn for remote players |
| `OtherPlayers` | Map<int, object> | Map of characterId → BP_OtherPlayerCharacter actor ref |

### Functions

| Function | Nodes | Params | Returns | Description |
|----------|-------|--------|---------|-------------|
| `SpawnOrUpdatePlayer` | 45 | `CharId (int), Name (string), X/Y/Z (float), Health/MaxHealth (float)` | — | If CharId exists in map → update TargetPosition + PlayerName. If not → SpawnActor(OtherPlayerClassActorRef) at (X,Y,Z), set CharacterId/PlayerName, add to map |
| `RemovePlayer` | 11 | `CharId (int)` | — | Find in map → DestroyActor → Remove from map |
| `GetOtherPlayerActor` | 8 | `CharId (int)` | `Actor` | Find in map → return actor ref (or null) |

### Event Graph (3 nodes)
Empty — all logic is in functions. BeginPlay/Tick/Overlap stubs only.

### Called By
- `BP_SocketManager.OnPlayerMoved` → `SpawnOrUpdatePlayer`
- `BP_SocketManager.OnPlayerLeft` → `RemovePlayer`
- `BP_SocketManager.FindRemotePlayer` → `GetOtherPlayerActor`
- `BP_SocketManager.OnHealthUpdate` → `GetOtherPlayerActor`

---

## BP_OtherPlayerCharacter

**Path**: `/Game/SabriMMO/Blueprints/BP_OtherPlayerCharacter`  
**Parent Class**: `Character`  
**Purpose**: Represents a remote player in the world. Receives server position updates and interpolates smoothly toward the target position on Tick using `AddMovementInput`.

### Components

| Component | Type | Purpose |
|-----------|------|---------|
| `NameTagWidget` | WidgetComponent | World-space WBP_PlayerNameTag showing player name |
| `HoverOverIndicator` | WidgetComponent | Selection indicator shown on hover/target |
| `HealthBarWidget` | WidgetComponent | Overhead WBP_TargetHealthBar |

### Interfaces Implemented

| Interface | Events |
|-----------|--------|
| `BPI_Damageable` | `ReceiveDamageVisual`, `UpdateHealthDisplay` |

### Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `DeltaSeconds` | real | Cached delta time |
| `PlayerName` | string | Display name (set by manager on spawn) |
| `CharacterId` | int | Server character ID |
| `bIsMoving` | bool | Whether currently moving toward target |
| `FlatDirection` | struct (Vector) | XY direction to target (Z zeroed) |
| `TargetPosition` | struct (Vector) | Server-provided target position to interpolate toward |

### Functions

| Function | Nodes | Purpose |
|----------|-------|---------|
| `UpdateHealthBar` | 5 | Updates overhead WBP_TargetHealthBar with current/max health |
| `PlayAttackAnimation` | 4 | Plays attack montage on this remote player's mesh |

### Event Graph (42 nodes)

#### Event BeginPlay
```
Event BeginPlay
    ↓
Set TargetPosition = GetActorLocation() (prevents immediate movement)
    ↓ Delay
Get NameTagWidget → Cast To WBP_PlayerNameTag → Set Player Name (PlayerName as Text)
```

#### Event Tick — Interpolation Movement
```
Event Tick
    ↓
Set DeltaSeconds
    ↓
Branch: bIsMoving?
    ├─ TRUE:
    │   [Calculate flat direction]
    │   FlatDirection = (TargetPosition - GetActorLocation()) with Z = 0
    │   distance = VectorLength(FlatDirection)
    │   ↓
    │   Branch: distance > 10?
    │   ├─ TRUE: Normalize(FlatDirection) → AddMovementInput(FlatDirection, 1.0)
    │   └─ FALSE: Set bIsMoving = false (arrived)
    │
    └─ FALSE: (idle)
```

**Key design**: Movement uses `AddMovementInput` with `CharacterMovementComponent` rather than direct teleportation. This produces smooth, natural-looking movement with proper ground/collision handling.

#### BPI_Damageable Events
```
Event ReceiveDamageVisual (DamageSourceLocation: Vector)
    ↓
Get Actor Location → FindLookAtRotation(ActorLoc, DamageSourceLocation)
    → SetActorRotation (rotate to face attacker)

Event UpdateHealthDisplay (CurrentHealth: float, MaxHealth: float)
    ↓
UpdateHealthBar(CurrentHealth, MaxHealth)
Set HealthBarWidget Visibility = (CurrentHealth < MaxHealth) ? Visible : Hidden
```

### How Remote Players Move

1. Server broadcasts `player:moved` event with `{characterId, x, y, z}`
2. `BP_SocketManager.OnPlayerMoved` calls `OtherPlayerManagerRef.SpawnOrUpdatePlayer(charId, name, x, y, z, health, maxHealth)`
3. `SpawnOrUpdatePlayer` sets `TargetPosition = (x, y, z)` and `bIsMoving = true` on the actor
4. On Tick, actor moves toward `TargetPosition` using `AddMovementInput` at `CharacterMovement.MaxWalkSpeed`
5. When within 10 units of target, sets `bIsMoving = false`

### Connections

| Used By | Function Called |
|---------|----------------|
| `BP_SocketManager.OnCombatDamage` | PlayAttackAnimation, Set TargetPosition |
| `BP_SocketManager.OnHealthUpdate` | UpdateHealthBar |
| `BP_SocketManager.OnCombatRespawn` | Set TargetPosition, UpdateHealthBar |
| `BP_OtherPlayerManager.SpawnOrUpdatePlayer` | Set CharacterId, PlayerName, TargetPosition |
| `AC_TargetingSystem.UpdateHoverDetection` | HoverOverIndicator visibility |

---

**Last Updated**: 2026-02-17  
**Source**: Read via unrealMCP `read_blueprint_content`
