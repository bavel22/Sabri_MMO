# Configuration Reference

## Server Environment (.env)

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `PORT` | number | 3000 | HTTP/Socket.io server port (typically set to 3001) |
| `DB_HOST` | string | — | PostgreSQL hostname |
| `DB_PORT` | number | — | PostgreSQL port |
| `DB_NAME` | string | — | PostgreSQL database name |
| `DB_USER` | string | — | PostgreSQL username |
| `DB_PASSWORD` | string | — | PostgreSQL password |
| `JWT_SECRET` | string | — | Secret key for JWT signing |
| `LOG_LEVEL` | string | INFO | Logging level: DEBUG, INFO, WARN, ERROR |
| `REDIS_HOST` | string | localhost | Redis hostname |
| `REDIS_PORT` | number | 6379 | Redis port |

## Server Constants

### Combat (COMBAT object)
| Constant | Value | Description |
|----------|-------|-------------|
| `BASE_DAMAGE` | 1 | Default damage per hit (before stat formulas) |
| `DAMAGE_VARIANCE` | 0 | Random variance added to base damage |
| `MELEE_RANGE` | 150 | Default melee attack range (UE units) |
| `RANGED_RANGE` | 800 | Default ranged attack range |
| `DEFAULT_ASPD` | 180 | Default attack speed (0–190 scale) |
| `ASPD_CAP` | 190 | Maximum ASPD value |
| `RANGE_TOLERANCE` | 50 | Padding for range checks |
| `COMBAT_TICK_MS` | 50 | Combat loop interval (ms) |
| `RESPAWN_DELAY_MS` | 5000 | Respawn delay (ms) |
| `SPAWN_POSITION` | {0, 0, 300} | Respawn world position |

### Enemy AI (ENEMY_AI object)
| Constant | Value | Description |
|----------|-------|-------------|
| `WANDER_TICK_MS` | 500 | AI tick interval |
| `WANDER_PAUSE_MIN` | 3000 | Min pause between wanders (ms) |
| `WANDER_PAUSE_MAX` | 8000 | Max pause between wanders (ms) |
| `WANDER_SPEED` | 60 | Movement speed (units/second) |
| `WANDER_DIST_MIN` | 100 | Min wander distance per axis |
| `WANDER_DIST_MAX` | 300 | Max wander distance per axis |
| `MOVE_BROADCAST_MS` | 200 | Position broadcast frequency (ms) |

### Inventory (INVENTORY object)
| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_SLOTS` | 100 | Max inventory slots per character |
| `MAX_WEIGHT` | 2000 | Max carry weight (future use) |

### Rate Limiting
| Setting | Value | Description |
|---------|-------|-------------|
| `windowMs` | 900,000 (15 min) | Rate limit window |
| `max` | 100 | Max requests per window per IP |
| Scope | `/api/*` | Applied to all API routes |

## UE5 Project Configuration

### SabriMMO.uproject
| Setting | Value |
|---------|-------|
| `EngineAssociation` | "5.7" |
| `FileVersion` | 3 |
| Module `SabriMMO` | Runtime, Default loading phase |

### Enabled Plugins
| Plugin | Purpose |
|--------|---------|
| ModelingToolsEditorMode | In-editor modeling |
| StateTree | AI behavior trees |
| GameplayStateTree | Gameplay state trees |
| VisualStudioTools | VS integration (Win64) |
| Diversion | Version control |
| RemoteControl | Remote control API |

### Build Dependencies (SabriMMO.Build.cs)
```
Public: Core, CoreUObject, Engine, InputCore, EnhancedInput,
        AIModule, StateTreeModule, GameplayStateTreeModule,
        UMG, Slate, HTTP, Json, JsonUtilities
```

## C++ Character Defaults (ASabriMMOCharacter constructor)

| Property | Value |
|----------|-------|
| CapsuleComponent radius | 42 |
| CapsuleComponent half-height | 96 |
| bOrientRotationToMovement | true |
| RotationRate | (0, 500, 0) |
| JumpZVelocity | 500 |
| AirControl | 0.35 |
| MaxWalkSpeed | 500 |
| BrakingDecelerationWalking | 2000 |
| CameraBoom TargetArmLength | 400 |
| bUsePawnControlRotation (boom) | true |
| bUsePawnControlRotation (camera) | false |

## C++ Combat Character Defaults (ACombatCharacter constructor)

| Property | Value |
|----------|-------|
| CapsuleComponent radius | 35 |
| CapsuleComponent half-height | 90 |
| MaxWalkSpeed | 400 |
| CameraBoom lag | enabled |
| CameraBoom rotation lag | enabled |
| PrimaryActorTick.bCanEverTick | true |
| Tags | "Player" |

## Database Defaults

### New Character
| Field | Default |
|-------|---------|
| class | 'warrior' |
| level | 1 |
| experience | 0 |
| x, y, z | 0, 0, 0 |
| health / max_health | 100 / 100 |
| mana / max_mana | 100 / 100 |
| All stats (str-luk) | 1 |
| stat_points | 48 |

### New User
| Field | Default |
|-------|---------|
| created_at | CURRENT_TIMESTAMP |
| last_login | NULL |

---

**Last Updated**: 2026-02-17
