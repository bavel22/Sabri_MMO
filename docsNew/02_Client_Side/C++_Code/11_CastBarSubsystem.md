# UCastBarSubsystem

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../../03_Server_Side/Skill_System.md)

**Files**: `Source/SabriMMO/UI/CastBarSubsystem.h` (69 lines), `CastBarSubsystem.cpp` (336 lines)
**Parent**: `UWorldSubsystem`
**Purpose**: Tracks skill casts for all visible players and renders cast bars via an always-on overlay.

## Widget

`SCastBarOverlay` at Z-order 25. Always-on (no toggle key). Projects world-space cast bars below each casting character.

## Socket Events

### Incoming (Server → Client)
| Event | Handler | Description |
|-------|---------|-------------|
| `skill:cast_start` | `HandleCastStart` | Adds entry to `ActiveCasts` (casterId, skillName, duration) |
| `skill:cast_complete` | `HandleCastComplete` | Removes cast for local player |
| `skill:cast_interrupted` | `HandleCastInterrupted` | Removes local player's cast (caster-only) |
| `skill:cast_interrupted_broadcast` | `HandleCastInterruptedBroadcast` | Removes any caster by ID (zone-wide) |
| `skill:cast_failed` | `HandleCastFailed` | Removes local player's cast (execution failed) |

No outgoing events.

## Data

### FCastBarEntry
```
CasterId, CasterName, SkillName, SkillId, CastStartTime (FPlatformTime::Seconds), CastDuration (seconds)
```

### ActiveCasts
`TMap<int32, FCastBarEntry>` — keyed by caster character ID. Tracks casts for ALL visible players, not just the local player.

### LocalCharacterId
Resolved from `UMMOGameInstance::GetSelectedCharacter()` at BeginPlay.

## Implementation Notes

- Converts server `actualCastTime` from milliseconds to seconds
- `cast_interrupted` only removes local player's cast
- `cast_interrupted_broadcast` removes any caster by ID (zone broadcast)
- Widget renders progress bars via OnPaint, using `FPlatformTime::Seconds()` delta

---

**Last Updated**: 2026-03-09
