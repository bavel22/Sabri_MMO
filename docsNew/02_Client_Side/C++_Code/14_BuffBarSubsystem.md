# UBuffBarSubsystem

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Status_Effect_Buff_System](../../03_Server_Side/Status_Effect_Buff_System.md)

**Files**: `Source/SabriMMO/UI/BuffBarSubsystem.h` (90 lines), `BuffBarSubsystem.cpp` (446 lines)
**Parent**: `UWorldSubsystem`
**Purpose**: Tracks active buffs (positive stat modifiers) and status effects (CC/DoT conditions) for the local player. Renders a horizontal icon row via Slate.

## Widget

`SBuffBarWidget` at Z-order 11 (between BasicInfo at Z=10 and CombatStats at Z=12). Always-on once events are wrapped (no toggle key). Positioned top-left at offset `(10, 175)` -- directly below BasicInfoWidget.

**Files**: `Source/SabriMMO/UI/SBuffBarWidget.h` (40 lines), `SBuffBarWidget.cpp` (295 lines)

## Architecture

Standard `UWorldSubsystem` + `SCompoundWidget` pattern:

1. `ShouldCreateSubsystem` -- creates in any game world
2. `OnWorldBeginPlay` -- registers 5 event handlers with `USocketEventRouter`, calls `ShowWidget` if socket is connected
3. `ShowWidget` -- creates `SBuffBarWidget`, adds to viewport via `AddViewportWidgetContent` at Z=11
4. `Deinitialize` -- removes widget from viewport, unregisters from EventRouter, clears all state

> **Historical note:** This subsystem previously used `TryWrapSocketEvents` / `WrapSingleEvent` / `FindSocketIOComponent` patterns. These were replaced by the `EventRouter->RegisterHandler()` pattern in Phase 4.

The widget reads directly from `ActiveBuffs` and `ActiveStatuses` public arrays on the subsystem. The subsystem populates these arrays from socket events; the widget polls them on Tick.

## Data Structures

### FActiveBuffInfo
```
Name           FString   "provoke", "endure", "blessing"
DisplayName    FString   "Provoke", "Endure", "Blessing"
Abbrev         FString   "PRV", "END", "BLS"
SkillId        int32     Originating skill ID
DurationMs     float     Total duration in milliseconds
RemainingMs    float     Remaining ms at time of event receipt
ReceivedAt     double    FPlatformTime::Seconds() when received
Category       FString   "buff" or "debuff"
```

### FActiveStatusInfo
```
Type           FString   "stun", "freeze", "poison", etc.
DurationMs     float     Total duration in milliseconds
RemainingMs    float     Remaining ms at time of event receipt
ReceivedAt     double    FPlatformTime::Seconds() when received
```

### Remaining Time Calculation

`UBuffBarSubsystem::GetRemainingSeconds(RemainingMs, ReceivedAt)` computes:
```
elapsed = FPlatformTime::Seconds() - ReceivedAt
remaining = (RemainingMs / 1000) - elapsed
return max(0, remaining)
```

## Socket Events

All events are registered via `EventRouter->RegisterHandler()` (multi-handler dispatch).

### Incoming (Server to Client)
| Event | Handler | Payload Fields | Description |
|-------|---------|----------------|-------------|
| `status:applied` | `HandleStatusApplied` | `targetId, isEnemy, statusType, duration` | Adds/refreshes a status effect entry |
| `status:removed` | `HandleStatusRemoved` | `targetId, isEnemy, statusType` | Removes a status effect entry |
| `skill:buff_applied` | `HandleBuffApplied` | `targetId, isEnemy, buffName, duration, skillId` | Adds/refreshes a buff entry |
| `skill:buff_removed` | `HandleBuffRemoved` | `targetId, buffName` | Removes a buff entry (also clears matching statuses for backward compat) |
| `buff:list` | `HandleBuffList` | `characterId, buffs[], statuses[]` | Full rebuild from server snapshot |

No outgoing events.

### Filtering

All handlers filter to local player only:
- `isEnemy == true` events are ignored (enemy buffs/statuses not tracked)
- `targetId` / `characterId` must match `LocalCharacterId` (resolved from `UMMOGameInstance::GetSelectedCharacter()`)

### Special Cases

- `skill:buff_applied` skips entries named "Frozen" or "Stone Curse" -- these are handled by `status:applied` instead
- `skill:buff_removed` also removes matching `ActiveStatuses` entries (server may send `buff_removed` for expired status effects)
- `buff:list` clears both arrays and rebuilds from scratch

## Widget Layout

Each icon is a 28x36 pixel box in a horizontal `SHorizontalBox` row:

```
+---------------------------+
| [colored bar 3px]         |   <- Status/category color indicator
|                           |
|          ABR              |   <- 3-letter abbreviation (Bold, 7pt, white)
|                           |
|          12s              |   <- Countdown timer (Regular, 6pt, gold)
+---------------------------+
```

- Outer border: `GoldDark` (0.50, 0.38, 0.15)
- Inner background: `PanelDark` (0.22, 0.14, 0.08, 0.90)
- Icons have 1px horizontal padding between them
- Status effects render first (more urgent), then buffs
- Maximum 20 icons total
- Icons rebuild when the total count changes (tracked by `LastIconCount`)
- Timer shows `Xs` for seconds, `Xm` for 60+ seconds
- Client-side expiry cleanup runs on Tick (removes entries with remaining time <= 0)

## Status Abbreviations and Colors

### Status Effects
| Status | Abbreviation | Color | RGBA |
|--------|-------------|-------|------|
| Stun | STN | Yellow | (0.90, 0.85, 0.20) |
| Freeze | FRZ | Cyan | (0.30, 0.85, 0.95) |
| Stone | PTR | Gray | (0.60, 0.60, 0.60) |
| Sleep | SLP | Purple | (0.65, 0.40, 0.85) |
| Poison | PSN | Green | (0.30, 0.80, 0.30) |
| Blind | BLD | Dark Gray | (0.40, 0.40, 0.40) |
| Silence | SIL | Blue | (0.35, 0.55, 0.90) |
| Confusion | CNF | Pink | (0.90, 0.50, 0.70) |
| Bleeding | BLE | Dark Red | (0.70, 0.15, 0.15) |
| Curse | CRS | Dark Purple | (0.50, 0.20, 0.60) |

### Buff Categories
| Category | Color | RGBA |
|----------|-------|------|
| Buff | Green | (0.25, 0.75, 0.30) |
| Debuff | Orange | (0.90, 0.55, 0.15) |

### Buff Abbreviations (from `GetBuffAbbrev`)
| Buff Name | Abbreviation |
|-----------|-------------|
| provoke | PRV |
| endure | END |
| sight | SGT |
| blessing | BLS |
| increase_agi | AGI |
| angelus | ANG |
| decrease_agi | DAG |

Unknown buffs default to the first 3 characters of the name, uppercased.

## Stability Delay

The subsystem uses a **4-tick stability delay** (2 seconds total, at 0.5s per tick).

This places it in the wrapping order as follows:
1. BasicInfoSubsystem, WorldHealthBarSubsystem, SkillTreeSubsystem, CastBarSubsystem -- wrap immediately (0 ticks)
2. **BuffBarSubsystem -- 4 ticks (2s)**
3. SkillVFXSubsystem -- 6 ticks (3s)
4. DamageNumberSubsystem -- 10 ticks (5s, wraps LAST)

All subsystems register with the EventRouter in `OnWorldBeginPlay`. The EventRouter dispatches to all registered handlers for a given event -- no ordering or chaining is required.

---

**Last Updated**: 2026-03-09
