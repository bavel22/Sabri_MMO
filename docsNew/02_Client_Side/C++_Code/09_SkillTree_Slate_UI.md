# Skill Tree Slate UI — C++ Documentation

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_System](../../03_Server_Side/Skill_System.md)

## Overview

Pure C++ Slate implementation of the Skill Tree window. Follows the `UWorldSubsystem` + `SCompoundWidget` pattern. RO Classic brown/gold theme, draggable, with class tabs, grid layout, compact skill cells, and rich hover tooltips with per-level data.

**Rewrite v3** (2026-03-10): Replaced SWrapBox flowing layout with fixed grid (treeRow/treeCol positioning), compact 78x88px cells, and `SSkillTooltipWidget` hover tooltips showing full per-level breakdowns.

## Files

| File | Class | Purpose |
|------|-------|---------|
| `UI/SkillTreeSubsystem.h` | `USkillTreeSubsystem` | UWorldSubsystem — data management, Socket.io event binding, `FSkillLevelInfo`/`FSkillEntry` structs |
| `UI/SkillTreeSubsystem.cpp` | | Implementation — event parsing, emit actions, targeting, walk-to-cast |
| `UI/SSkillTreeWidget.h` | `SSkillTreeWidget` | SCompoundWidget — outer window (title bar, tabs, scroll box, bottom bar) |
| `UI/SSkillTreeWidget.cpp` | `SSkillGridPanel` (file-local) | Grid layout, compact cells, tooltip attachment, deferred rebuilds |
| `UI/SSkillTooltipWidget.h` | `SSkillTooltipWidget` | SCompoundWidget — hover tooltip with header, info, prereqs, description, level table |
| `UI/SSkillTooltipWidget.cpp` | | Tooltip implementation |
| `UI/SkillDragDropOperation.h` | `USkillDragDropOperation` | UDragDropOperation subclass for dragging skills to UMG hotbar |

## USkillTreeSubsystem

### Public Data Fields (read by widget)

| Field | Type | Description |
|-------|------|-------------|
| `JobClass` | `FString` | Current job class ID (e.g., "swordsman") |
| `SkillPoints` | `int32` | Available skill points |
| `SkillGroups` | `TArray<FSkillClassGroup>` | Skill data grouped by class |
| `LearnedSkills` | `TMap<int32, int32>` | skillId -> level map |

### Structs

**FSkillLevelInfo** — Per-level data for tooltip display:
- `Level`, `SpCost`, `CastTime`, `Cooldown`, `EffectValue`, `Duration`, `AfterCastDelay`

**FSkillEntry** — Single skill parsed from server data:
- `SkillId`, `Name`, `DisplayName`, `MaxLevel`, `CurrentLevel`
- `Type` (active/passive/toggle), `TargetType`, `Element`, `Range`
- `Description`, `Icon`, `IconPath`, `TreeRow`, `TreeCol`
- `SpCost`, `NextSpCost`, `CastTime`, `Cooldown`, `EffectValue`
- `bCanLearn` — server-computed flag
- `Prerequisites` — `TArray<FSkillPrerequisite>` (each has `RequiredSkillId`, `RequiredLevel`)
- `AllLevels` — `TArray<FSkillLevelInfo>` (all levels for tooltip display)

**FSkillClassGroup** — Group of skills for one class:
- `ClassId`, `ClassDisplayName`, `Skills` array

### Socket Events Bound

| Event | Handler | Action |
|-------|---------|--------|
| `skill:data` | `HandleSkillData` | Parse full skill tree (including `allLevels`), update all fields, rebuild widget |
| `skill:learned` | `HandleSkillLearned` | Update LearnedSkills, request full refresh |
| `skill:refresh` | `HandleSkillRefresh` | Update SkillPoints, request full refresh |
| `skill:reset_complete` | `HandleSkillResetComplete` | Clear LearnedSkills, update SkillPoints |
| `skill:error` | `HandleSkillError` | Log warning, show on-screen debug message, dismiss AoE circle |
| `skill:used` | `HandleSkillUsed` | Log confirmation |
| `skill:effect_damage` | `HandleSkillEffectDamage` | Log skill damage/heal |
| `skill:buff_applied` | `HandleSkillBuffApplied` | Track active buffs |
| `skill:buff_removed` | `HandleSkillBuffRemoved` | Remove expired buffs |
| `skill:cooldown_started` | `HandleSkillCooldownStarted` | Track cooldown expiry |
| `hotbar:alldata` | `HandleHotbarAllData` | Populate HotbarSkillMap |

### Server Payload Change (v3)

The `skill:data` payload now includes an `allLevels` array for each skill:
```javascript
allLevels: skill.levels.map(l => ({
    level: l.level,
    spCost: l.spCost,
    castTime: l.castTime || 0,
    cooldown: l.cooldown || 0,
    effectValue: l.effectValue || 0,
    duration: l.duration || 0,
    afterCastDelay: l.afterCastDelay || 0
}))
```

## SSkillTreeWidget

### Layout Structure (v3)

```
Gold Trim Border (2px)
  +-- Dark Inset (1px)
      +-- Brown Panel (540x560)
          +-- Title Bar (dark bg, "Skill Tree", points count, X close)
          +-- Class Tabs (horizontal, gold active / dark inactive)
          +-- Gold Divider
          +-- Scrollable Grid (SScrollBox)
          |   +-- SSkillGridPanel (file-local SCompoundWidget)
          |       +-- SVerticalBox of rows
          |           +-- SHorizontalBox per row
          |               +-- 78x88px skill cells (or empty placeholders)
          +-- Gold Divider
          +-- Bottom Bar (points remaining, Reset All button)
```

### Grid Layout Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `CELL_WIDTH` | 78px | Individual cell width |
| `CELL_HEIGHT` | 88px | Individual cell height |
| `CELL_HGAP` | 14px | Horizontal gap between cells |
| `CELL_VGAP` | 24px | Vertical gap between rows |
| `GRID_PADDING` | 8px | Padding around entire grid |

### Compact Skill Cell Content

Each 78x88px cell contains:
1. **Icon** (32x32, centered) — draggable for learned active/toggle skills
2. **Skill name** (6pt bold, centered, auto-wrap to cell width)
3. **Level** — `3/10` (green if learned, gold if maxed, dim if locked)
4. **Learn button `[+]`** — only when `canLearn && !maxed`

### Hover Tooltips (SToolTip)

Each cell has an `SToolTip` with `SSkillTooltipWidget` as content. Tooltip appears on hover with:
- Header: icon (28x28) + skill name (gold 10pt) + level display
- Skill info: type, element (color-coded), target, range
- Prerequisites: green (met) / red (unmet) with current/required levels
- Description: auto-wrapping text
- Per-level table: dynamic columns (SP always shown; Cast/CD/Duration only if any level has non-zero values). Current level row highlighted gold, next level row highlighted green.

### Skill Cell Colors

| State | Color | Meaning |
|-------|-------|---------|
| `SkillMaxed` | Gold (0.50, 0.38, 0.15) | Skill at max level |
| `SkillLearned` | Dark Green (0.18, 0.35, 0.18) | Skill partially learned |
| `SkillActive` (canLearn) | Blue-gray (0.28, 0.42, 0.60) | Active skill available to learn |
| `SkillPassive` (canLearn) | Green-gray (0.35, 0.50, 0.30) | Passive skill available to learn |
| `SkillLocked` | Dark brown (0.25, 0.20, 0.15, 0.8) | Prerequisites not met |

### Deferred Rebuild Pattern

Widget mutations are deferred to `Tick()` to avoid Slate crashes:
- `RebuildSkillContent()` sets `bPendingFullRebuild = true` (tabs + grid)
- `RebuildSkillGrid()` sets `bPendingGridRebuild = true` (grid only)
- `Tick()` checks flags and calls `DoRebuildSkillContent()` / `DoRebuildSkillGrid()`
- Tab clicks set `ActiveClassTab` + `bPendingGridRebuild` (never call rebuild directly from event handler)

### SSkillGridPanel (file-local)

Simple `SCompoundWidget` container defined inside `SSkillTreeWidget.cpp`. Holds a `ContentBox` (`SVerticalBox`) that the grid rows are added to. Placed inside the `SScrollBox` for vertical scrolling.

### Drag-to-Hotbar

Learned active/toggle skill icons have `OnMouseButtonDown_Lambda` that initiates drag state:
1. `bSkillDragInitiated = true`, captures mouse
2. `OnMouseMove` checks distance threshold (5px)
3. On threshold: `Sub->StartSkillDrag()`, releases mouse capture
4. Hotbar widget (`SHotbarRowWidget`) detects `SkillSub->bSkillDragging` on click and handles the drop

### Window Drag

Title bar area (top 28px) acts as drag handle. DPI-correct pattern using `DragStartWidgetPos + AbsDelta / DPIScale`.

## SSkillTooltipWidget

### Architecture

Standalone `SCompoundWidget` with no subsystem dependency beyond construction. Receives `const FSkillEntry*` and `USkillTreeSubsystem*` as `SLATE_ARGUMENT`s. All data is read during `Construct()` — the tooltip is static once created (rebuilt on grid rebuild).

### Color Palette (TTColors namespace)

Dark panel (0.12, 0.08, 0.05, 0.95 alpha) with gold border. Element-specific colors for fire (orange), water (blue), wind (green), earth (brown), holy (yellow-white), ghost (purple), poison (purple), undead (grey).

### Level Table Dynamic Columns

The table inspects all levels to determine which columns to show:
- **Always shown**: Lv, SP, Effect
- **Conditional**: Cast (if any level has CastTime > 0), CD (if any Cooldown > 0), Duration (if any Duration > 0)
- Row highlighting: gold tint for current level, green tint for next level

### Helper Functions

- `FormatTime(int32 TimeMs)` — converts ms to "X.Xs" format
- `GetSkillTypeLabel(FString Type)` — "active" -> "Active"
- `GetTargetLabel(FString TargetType)` — "single" -> "Single Target"
- `GetElementColor(FString Element)` — element-specific FLinearColor

---

## Phase 5 Additions

### Ground AoE Indicators (SkillTreeSubsystem.cpp)

`GroundAoE::GetAoEInfo()` switch — 9 total cases:

| Case | Skill | Radius | Color | EffectDuration |
|------|-------|--------|-------|----------------|
| 105 | Magnum Break | 300 | orange | 0.5s |
| 203 | Napalm Beat | 300 | purple | 1.0s |
| 207 | Fire Ball | 500 | orange | 1.0s |
| 209 | Fire Wall | 0 (no circle) | red | 0 |
| 211 | Safety Wall | 100 | white | 10s |
| 212 | Thunderstorm | 500 | yellow-green | level-dependent |
| 304 | Arrow Shower | 400 | neutral | 0 |
| 407 | Signum Crucis | 500 | holy | 0 |
| 608 | Cart Revolution | 300 | neutral | 0 |

### VFX Configs (SkillVFXData.cpp)

16 new configs added in Phase 5 (31 total in BuildSkillVFXConfigs):
- Heal(400/HealFlash), Blessing(402/SelfBuff), IncAGI(403/SelfBuff), DecAGI(404/TargetDebuff)
- Angelus(406/SelfBuff), Ruwach(408/SelfBuff), ImpConc(302/SelfBuff)
- DoubleStrafe(303/Projectile×2), ArrowShower(304/GroundAoERain), ArrowRepel(306/Projectile)
- Envenom(504/AoEImpact), SandAttack(506/AoEImpact)
- Mammonite(603/AoEImpact), CartRev(608/AoEImpact)
- EnergyCo(213/SelfBuff), LoudExc(609/SelfBuff)

---

**Last Updated**: 2026-03-10 (Phase 5: +3 AoE indicator cases, +16 VFX configs, all 6 first classes)
**Previous**: 2026-03-10 (v3 rewrite)
