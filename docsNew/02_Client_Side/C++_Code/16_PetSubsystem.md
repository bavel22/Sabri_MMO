# PetSubsystem — Client C++ Documentation

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Pet_System](../../03_Server_Side/Pet_System.md)

**Files**: `UI/PetSubsystem.h`, `UI/PetSubsystem.cpp`
**Z-Order**: 21
**Pattern**: UWorldSubsystem + inline SPetWidget (SCompoundWidget)
**Events**: 8 handlers via SocketEventRouter

## Purpose

Manages pet state (hunger, intimacy, name, type) and provides the pet management UI panel. RO Classic brown/gold themed Slate widget with hunger/intimacy bars and action buttons.

## Widget: SPetWidget (inline in PetSubsystem.cpp)

### Layout
```
┌─ Gold Trim Border ──────────────────┐
│┌─ Dark Border ─────────────────────┐│
││┌─ Brown Panel ───────────────────┐││
│││  Pet - [PetName]                │││
│││  ──────── divider ──────────── │││
│││  Mood: [IntimacyLevel]          │││
│││  Hunger  [████████░░] 80/100    │││
│││  Intimacy [██████░░░░] 600/1000 │││
│││  ──────── divider ──────────── │││
│││  [ Feed ]                       │││
│││  [ Return to Egg ]              │││
││└─────────────────────────────────┘││
│└───────────────────────────────────┘│
└─────────────────────────────────────┘
```

### Data Binding
All text and bar values bound via TAttribute lambdas reading `UPetSubsystem` fields:
- `bHasPet` — controls visibility of pet info vs "No pet" message
- `PetName`, `IntimacyLevel` — text display
- `Hunger` (0-100) → green progress bar
- `Intimacy` (0-1000) → pink progress bar

## Subsystem: UPetSubsystem

### Public Data Fields
| Field | Type | Description |
|-------|------|-------------|
| bHasPet | bool | Whether a pet is currently active |
| PetId | int32 | DB row ID |
| MobId | int32 | Monster template ID |
| PetName | FString | Custom or default name |
| Hunger | int32 | 0-100 |
| Intimacy | int32 | 0-1000 |
| HungerLevel | FString | very_hungry/hungry/neutral/satisfied/stuffed |
| IntimacyLevel | FString | awkward/shy/neutral/cordial/loyal |
| EquipItemId | int32 | Equipped accessory (0 = none) |

### Event Handlers (8)
| Event | Handler | Updates |
|-------|---------|---------|
| `pet:hatched` | HandlePetHatched | All fields, shows widget |
| `pet:fed` | HandlePetFed | Hunger, Intimacy, IntimacyLevel |
| `pet:hunger_update` | HandlePetHungerUpdate | Hunger, Intimacy, levels |
| `pet:ran_away` | HandlePetRanAway | Clears all fields |
| `pet:returned` | HandlePetReturned | Clears all fields |
| `pet:renamed` | HandlePetRenamed | PetName |
| `pet:tamed` | HandlePetTamed | Log only |
| `pet:error` | HandlePetError | Log warning |

### Public API
| Method | Emits | Description |
|--------|-------|-------------|
| `ToggleWidget()` | — | Show/hide pet panel |
| `FeedPet()` | `pet:feed` | Feed active pet |
| `ReturnPetToEgg()` | `pet:return_to_egg` | Deactivate pet |
| `RenamePet(name)` | `pet:rename` | Set custom name |
| `RequestPetList()` | `pet:list` | Get all owned pets |

### Lifecycle
- `OnWorldBeginPlay`: Register 8 handlers via `Router->RegisterHandler()`
- `Deinitialize`: Unregister all, hide widget
- Widget auto-shows on `pet:hatched`
- Widget gated behind `GI->IsSocketConnected()` (game levels only)
