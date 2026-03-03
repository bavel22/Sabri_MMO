# World Health Bar System (Floating HP/SP Bars)

## Overview

RO-style floating health and mana bars rendered below player and enemy characters in world space. Uses screen-space projection to position bars at entity feet, with OnPaint rendering for maximum performance.

**Player**: Green HP bar + Blue SP bar, always visible (hidden on death)
**Enemy**: Pink/Magenta HP bar only, hidden by default, appears when damaged

## Architecture

```
Server (Socket.io events)
    │
    ├── combat:health_update ──┐
    ├── combat:damage ─────────┤
    ├── combat:death ──────────┤
    ├── combat:respawn ────────┤
    ├── player:stats ──────────┤    ┌──────────────────────┐
    ├── enemy:health_update ───┼───>│ WorldHealthBarSubsystem│
    ├── enemy:spawn ───────────┤    │  (UWorldSubsystem)    │
    ├── enemy:move ────────────┤    │  Wraps socket events  │
    └── enemy:death ───────────┘    │  Tracks HP/SP data    │
                                    │  Enemy position map   │
                                    └──────────┬───────────┘
                                               │
                                    ┌──────────▼───────────┐
                                    │ SWorldHealthBarOverlay │
                                    │  (SCompoundWidget)     │
                                    │  OnPaint rendering     │
                                    │  World-to-screen proj  │
                                    │  Z-order: 8            │
                                    └────────────────────────┘
```

## Client Implementation (C++)

### Files

| File | Class | Purpose |
|------|-------|---------|
| `UI/WorldHealthBarSubsystem.h` | `UWorldHealthBarSubsystem` | Data management, socket event wrapping |
| `UI/WorldHealthBarSubsystem.cpp` | — | All event handlers, overlay lifecycle |
| `UI/SWorldHealthBarOverlay.h` | `SWorldHealthBarOverlay` | Widget definition, bar constants |
| `UI/SWorldHealthBarOverlay.cpp` | — | OnPaint rendering, world-to-screen projection |

### UWorldHealthBarSubsystem

**Public Data Fields** (read by overlay each frame):
- `PlayerCurrentHP`, `PlayerMaxHP` — Local player HP
- `PlayerCurrentSP`, `PlayerMaxSP` — Local player SP
- `bPlayerDead` — Hide player bars on death
- `EnemyHealthMap` — `TMap<int32, FEnemyBarData>` keyed by enemyId

**FEnemyBarData struct:**
- `EnemyId`, `CurrentHP`, `MaxHP`
- `bBarVisible` — True when enemy is in combat (damaged)
- `bIsDead` — True after enemy:death
- `WorldPosition` — Tracked from enemy:spawn, enemy:move, combat:damage

**Socket Events Wrapped:**

| Event | Data Extracted | Purpose |
|-------|---------------|---------|
| `combat:health_update` | characterId, health, maxHealth, mana, maxMana | Player HP/SP updates |
| `combat:damage` | targetId, isEnemy, targetHealth, targetMaxHealth, targetX/Y/Z | Player+enemy HP, enemy position |
| `combat:death` | killedId | Player death → hide bar |
| `combat:respawn` | characterId, health, maxHealth, mana, maxMana | Player respawn → show bar |
| `player:stats` | derived.maxHP, derived.maxSP | Max HP/SP from stat recalc |
| `enemy:health_update` | enemyId, health, maxHealth, inCombat | Enemy HP + visibility toggle |
| `enemy:spawn` | enemyId, health, maxHealth, x, y, z | Initial enemy state + position |
| `enemy:move` | enemyId, x, y, z | Enemy position tracking |
| `enemy:death` | enemyId | Enemy death → hide bar |

**Position Tracking:**
- **Local player**: Uses actual pawn position via `GetPlayerFeetPosition()` — accurate, capsule-half-height offset
- **Enemies**: Position from socket events (enemy:spawn, enemy:move, combat:damage targetX/Y/Z)

### SWorldHealthBarOverlay

**Rendering**: Full-viewport overlay with `EVisibility::HitTestInvisible`. Uses active timer to invalidate each frame. All bars rendered in `OnPaint` via `FSlateDrawElement::MakeBox`.

**Bar Dimensions:**
- Width: 80px
- HP fill height: 5px
- SP fill height: 5px
- Border: 1px navy
- Divider: 1px navy (between HP and SP)
- Player total height: 13px (1+5+1+5+1)
- Enemy total height: 7px (1+5+1)

**Color Palette (RO Classic):**

| Element | Color | FLinearColor |
|---------|-------|-------------|
| Border | Navy (#10189C) | (0.063, 0.094, 0.612) |
| Background | Dark gray (#424242) | (0.259, 0.259, 0.259) |
| Player HP | Green (#10EF21) | (0.063, 0.937, 0.129) |
| Player HP (critical ≤25%) | Red (#FF0000) | (1.0, 0.0, 0.0) |
| Player SP | Blue (#1863DE) | (0.094, 0.388, 0.871) |
| Enemy HP | Pink (#FF00E7) | (1.0, 0.0, 0.906) |
| Enemy HP (critical ≤25%) | Yellow (#FFFF00) | (1.0, 1.0, 0.0) |

**DPI Handling:**
```
GeometryScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale()
InvScale = 1.0 / GeometryScale
SlateLocalPos = ScreenPixelPos * InvScale
```

## Behavior Specification

### Player Bars
- Always visible below local player character
- Green HP bar on top, Blue SP bar on bottom
- HP bar turns red when HP ≤ 25%
- Hidden when player is dead (`bPlayerDead = true`)
- Reappears immediately on respawn
- Bar snaps to new value instantly (no animation, RO-faithful)

### Enemy Bars
- Hidden by default on spawn (`bBarVisible = false`)
- Shown when enemy enters combat (`inCombat: true` from `enemy:health_update`)
- Pink/Magenta HP fill, turns yellow at ≤ 25%
- Hidden on enemy death (`enemy:death` event)
- Hidden on enemy respawn (fresh `enemy:spawn` resets state)
- AoE damage: each enemy hit receives its own events → all damaged enemies show bars

### Edge Cases
- Enemy position unknown (zero vector): bar not drawn, position fills in from next `enemy:move` or `combat:damage`
- Initial enemy spawn events missed (fired before subsystem wraps): positions update from continuous `enemy:move` broadcasts within seconds
- Player at full HP: bar shows 100% green fill (no special handling)
- Enemy at full HP in combat: bar shows 100% pink fill

## Design Patterns Used

| Pattern | How Applied |
|---------|-------------|
| UWorldSubsystem | `UWorldHealthBarSubsystem` — auto-created, world-lifetime, no actor dependency |
| Event-Driven | Socket event wrapping, no Tick polling for data |
| OnPaint Overlay | Single widget renders all bars — efficient for many enemies |
| World-to-Screen Projection | `ProjectWorldLocationToScreen` each frame per entity |
| Event Chaining | `WrapSingleEvent` preserves Blueprint handlers, adds C++ handler on top |

## Related Files

| File | Purpose |
|------|---------|
| `UI/BasicInfoSubsystem.*` | Reference: same WrapSingleEvent pattern, HP/SP event handling |
| `UI/DamageNumberSubsystem.*` | Reference: overlay pattern, world-to-screen projection |
| `UI/SDamageNumberOverlay.*` | Reference: OnPaint rendering, active timer, DPI scaling |
| `server/src/index.js` | Socket event sources (combat:*, enemy:*, player:stats) |
| `docsNew/06_Reference/Event_Reference.md` | Complete event payload documentation |
| `docsNew/03_Server_Side/Enemy_System.md` | Enemy spawning, combat, death flow |

---

**Last Updated**: 2026-03-03
**Version**: 1.0
**Status**: Complete
