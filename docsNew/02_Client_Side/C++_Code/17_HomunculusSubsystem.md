# HomunculusSubsystem — Client C++ Documentation

**Files**: `UI/HomunculusSubsystem.h`, `UI/HomunculusSubsystem.cpp`
**Z-Order**: 22
**Pattern**: UWorldSubsystem + inline SHomunculusWidget (SCompoundWidget)
**Events**: 9 handlers via SocketEventRouter

## Purpose

Manages homunculus state (HP/SP, hunger, intimacy, level, type, evolution) and provides the homunculus management UI panel. Connects to the existing server-side homunculus system (4 types, 12 skills, combat tick, feeding, evolution).

## Widget: SHomunculusWidget (inline in HomunculusSubsystem.cpp)

### Layout
```
┌─ Gold Trim Border ──────────────────┐
│┌─ Dark Border ─────────────────────┐│
││┌─ Brown Panel ───────────────────┐││
│││  [Name] Lv.[Level]              │││
│││  ──────── divider ──────────── │││
│││  Type: [type] | [Active/Dead]   │││
│││  HP   [████████░░] 4500/5000    │││
│││  SP   [██████░░░░] 200/400      │││
│││  Feed [█████░░░░░] 60/100       │││
│││  ──────── divider ──────────── │││
│││  [ Feed ]                       │││
│││  [ Vaporize (Rest) ]            │││
││└─────────────────────────────────┘││
│└───────────────────────────────────┘│
└─────────────────────────────────────┘
```

### Data Binding
All values bound via TAttribute lambdas:
- `bHasHomunculus` — controls visibility
- `HomunculusName`, `HomunculusType`, `HomunculusLevel` — title
- `bIsAlive`, `bIsEvolved` — status display
- `HP/MaxHP` → red bar, `SP/MaxSP` → blue bar, `Hunger` → green bar

## Subsystem: UHomunculusSubsystem

### Public Data Fields
| Field | Type | Description |
|-------|------|-------------|
| bHasHomunculus | bool | Currently summoned |
| HomunculusName | FString | Name (e.g., "Lif") |
| HomunculusType | FString | lif/amistr/filir/vanilmirth |
| HomunculusLevel | int32 | Current level |
| HP / MaxHP | int32 | Hit points |
| SP / MaxSP | int32 | Skill points |
| Hunger | int32 | 0-100 |
| Intimacy | int32 | 0-1000 |
| bIsEvolved | bool | Has evolved |
| bIsAlive | bool | Not dead |

### Event Handlers (9)
| Event | Handler | Updates |
|-------|---------|---------|
| `homunculus:summoned` | HandleSummoned | All fields, shows widget |
| `homunculus:vaporized` | HandleVaporized | Clears state |
| `homunculus:update` | HandleUpdate | HP, SP |
| `homunculus:leveled_up` | HandleLeveledUp | Level |
| `homunculus:died` | HandleDied | bIsAlive, HP=0 |
| `homunculus:fed` | HandleFed | Hunger, Intimacy |
| `homunculus:hunger_tick` | HandleHungerTick | Hunger, Intimacy |
| `homunculus:evolved` | HandleEvolved | bIsEvolved, Intimacy |
| `homunculus:resurrected` | HandleResurrected | bIsAlive, HP |

### Public API
| Method | Emits | Description |
|--------|-------|-------------|
| `ToggleWidget()` | — | Show/hide panel |
| `FeedHomunculus()` | `homunculus:feed` | Feed with type-specific food |
| `VaporizeHomunculus()` | `homunculus:rest` | Return homunculus (requires HP >= 80%) |

### Lifecycle
- `OnWorldBeginPlay`: Register 9 handlers via `Router->RegisterHandler()`
- `Deinitialize`: Unregister all, hide widget
- Widget auto-shows on `homunculus:summoned`
- Widget gated behind `GI->IsSocketConnected()`
