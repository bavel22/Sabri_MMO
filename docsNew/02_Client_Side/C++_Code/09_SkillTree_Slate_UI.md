# Skill Tree Slate UI — C++ Documentation

## Overview

Pure C++ Slate implementation of the Skill Tree window. Follows the same `UWorldSubsystem` + `SCompoundWidget` pattern as the Basic Info panel. RO Classic brown/gold theme, draggable, with class tabs and scrollable skill grid.

## Files

| File | Class | Purpose |
|------|-------|---------|
| `UI/SkillTreeSubsystem.h` | `USkillTreeSubsystem` | UWorldSubsystem — data management, Socket.io event binding |
| `UI/SkillTreeSubsystem.cpp` | | Implementation — event parsing, emit actions |
| `UI/SSkillTreeWidget.h` | `SSkillTreeWidget` | SCompoundWidget — Slate UI layout |
| `UI/SSkillTreeWidget.cpp` | | Implementation — title bar, class tabs, skill grid, learn buttons, hotbar quick-assign row |
| `UI/SkillDragDropOperation.h` | `USkillDragDropOperation` | UDragDropOperation subclass for dragging skills to UMG hotbar |

## USkillTreeSubsystem

### Public Data Fields (read by widget)

| Field | Type | Description |
|-------|------|-------------|
| `JobClass` | `FString` | Current job class ID (e.g., "swordsman") |
| `SkillPoints` | `int32` | Available skill points |
| `SkillGroups` | `TArray<FSkillClassGroup>` | Skill data grouped by class |
| `LearnedSkills` | `TMap<int32, int32>` | skillId → level map |

### Structs

**FSkillEntry** — Single skill parsed from server data:
- `SkillId`, `Name`, `DisplayName`, `MaxLevel`, `CurrentLevel`
- `Type` (active/passive/toggle), `TargetType`, `Element`, `Range`
- `Description`, `Icon`, `TreeRow`, `TreeCol`
- `SpCost`, `NextSpCost`, `CastTime`, `Cooldown`, `EffectValue`
- `bCanLearn` — server-computed flag
- `Prerequisites` — `TArray<FSkillPrerequisite>` (each has `RequiredSkillId`, `RequiredLevel`)

**FSkillClassGroup** — Group of skills for one class:
- `ClassId`, `ClassDisplayName`, `Skills` array

### Public Methods

| Method | Description |
|--------|-------------|
| `ToggleWidget()` | Show/hide the skill tree window |
| `ShowWidget()` | Add widget to viewport at z-order 20 |
| `HideWidget()` | Remove widget from viewport |
| `RequestSkillData()` | Emit `skill:data` to server |
| `LearnSkill(int32)` | Emit `skill:learn` with skillId |
| `ResetAllSkills()` | Emit `skill:reset` |
| `AssignSkillToHotbar(int32, FString, int32)` | Emit `hotbar:save_skill` with skillId, displayName, slotIndex (1–9) |

### Delegate

`FOnSkillDataUpdated OnSkillDataUpdated` — Broadcast when `HandleSkillData` finishes parsing. The widget subscribes to this to rebuild its content.

### Socket Events Bound

| Event | Handler | Action |
|-------|---------|--------|
| `skill:data` | `HandleSkillData` | Parse full skill tree, update all fields, broadcast delegate |
| `skill:learned` | `HandleSkillLearned` | Update LearnedSkills, request full refresh |
| `skill:refresh` | `HandleSkillRefresh` | Update SkillPoints, request full refresh |
| `skill:reset_complete` | `HandleSkillResetComplete` | Clear LearnedSkills, update SkillPoints |
| `skill:error` | `HandleSkillError` | Log warning, show on-screen debug message |

**Note**: `hotbar:data` is intentionally NOT bound from C++. The Blueprint `BP_SocketManager` binds `hotbar:alldata` → `OnHotbarAllData` instead. Any C++ `NativeClient->OnEvent()` call overwrites the BP handler (SocketIO plugin uses `TMap::Add`). See Troubleshooting below.

### Delayed Hotbar Request

After `TryWrapSocketEvents` succeeds, a 3-second `FTimerHandle` fires `hotbar:request` to the server. This ensures the Blueprint's `AC_HUDManager` (which has a 0.2s Delay in its EventGraph) is fully initialized before hotbar data arrives.

### Event Binding Pattern

For **skill-specific events** (`skill:data`, `skill:learned`, etc.), `SkillTreeSubsystem` uses `BindNewEvent()` to register C++-only handlers directly on the native Socket.io client. This is safe because skill events have no pre-existing Blueprint bindings.

For **hotbar events**, the subsystem does NOT bind `hotbar:data` (now `hotbar:alldata`) from C++. The Blueprint `BP_SocketManager` already has a `BindEventToFunction` for this event. C++ `NativeClient->OnEvent()` uses `TMap::Add` which **replaces** any existing handler for the same event name, destroying the BP binding. This was the root cause of the hotbar-not-loading bug (2026-02-24).

## SSkillTreeWidget

### Layout Structure

```
Gold Trim Border (2px)
  └─ Dark Inset (1px)
      └─ Brown Panel
          ├─ Title Bar (dark bg, "Skill Tree", points count, X close)
          ├─ Class Tabs (horizontal, gold active / dark inactive)
          ├─ Gold Divider
          ├─ Scrollable Skill Content (SScrollBox)
          │   └─ SWrapBox of skill slots
          ├─ Gold Divider
          └─ Bottom Bar (points remaining, Reset All button)
```

### Skill Slot Colors

| State | Color | Meaning |
|-------|-------|---------|
| `SkillMaxed` | Gold | Skill at max level |
| `SkillLearned` | Dark Green | Skill partially learned |
| `SkillActive` (canLearn) | Blue-gray | Active skill available to learn |
| `SkillPassive` (canLearn) | Green-gray | Passive skill available to learn |
| `SkillLocked` | Dark brown | Prerequisites not met |

### Skill Slot Content

Each slot (130px wide) displays:
1. **Type indicator** — `[A]` active, `[P]` passive, `[T]` toggle
2. **Skill name** (bold, auto-wrap)
3. **Level** — `Lv 3/10` + SP cost
4. **Description** (dim, auto-wrap)
5. **Learn/Level Up button** (green, only when `canLearn && !maxed`)
6. **Hotbar quick-assign row** (9 numbered buttons [1]-[9], only for learned active/toggle skills)

### Drag Behavior

Title bar area (top 28px) acts as drag handle. Same pattern as `SBasicInfoWidget`:
- `OnMouseButtonDown` → capture if in title area
- `OnMouseMove` → update position via `SetRenderTransform`
- `OnMouseButtonUp` → release capture

### Dynamic Rebuild

When `OnSkillDataUpdated` fires, `RebuildSkillContent()` clears and rebuilds the `SkillContentBox`. Class tabs are rebuilt from `SkillGroups` array. Active tab index is clamped to valid range.

## Integration with Blueprint

To open the skill tree from Blueprint:
1. Get `USkillTreeSubsystem` from the World: `GetWorld()->GetSubsystem<USkillTreeSubsystem>()`
2. Call `ToggleWidget()` or `ShowWidget()`
3. The subsystem automatically requests skill data from server on show

Alternatively, bind a key input in the PlayerController to call `ToggleWidget()`.

## Troubleshooting

### Issue: Hotbar data not loading (OnHotbarData/OnHotbarAllData not firing)
**Root Cause**: C++ `NativeClient->OnEvent("hotbar:data", ...)` in `TryWrapSocketEvents` replaced the Blueprint handler. The SocketIO plugin's `FSocketIONative::OnEvent` uses `EventFunctionMap.Add()` (`TMap::Add` = replace, not append).
**Fix**: Remove all C++ bindings for events that have pre-existing Blueprint `BindEventToFunction`. Server was changed to emit `hotbar:alldata` instead of `hotbar:data`, with a new BP binding `OnHotbarAllData`.
**Prevention**: Never bind the same event name from both C++ and Blueprint.

### Issue: Quick-assign buttons off by 1
**Root Cause**: `BuildHotbarAssignRow` iterated `Slot = 0..8` and passed `SlotIndex = Slot` (0-indexed) but BP hotbar slots are 1-indexed.
**Fix**: Changed to `SlotIndex = Slot + 1`. Server validation for `hotbar:save_skill` updated to accept 1–9.

---

**Last Updated**: 2026-02-25 (Added hotbar quick-assign, AssignSkillToHotbar, delayed hotbar:request, hotbar:data binding removal, troubleshooting)  
**Previous**: 2026-02-23
