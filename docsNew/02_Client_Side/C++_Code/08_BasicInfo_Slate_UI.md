# BasicInfo Slate UI — Draggable & Resizable HUD Panel

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [UI_Style_Guide](../../UI/ro_classic_ui_slate_style_guide.md)

**Files**: `UI/SBasicInfoWidget.h/.cpp`, `UI/BasicInfoSubsystem.h/.cpp`  
**Purpose**: Display local player's basic information (name, job, HP/SP, EXP, weight, zuzucoin) in a draggable, resizable Slate widget.  
**Status**: ✅ Implemented, compiling, and fully functional  
**Last Updated**: 2026-02-24

---

## Architecture Overview

### Component-Based Architecture
- **SBasicInfoWidget** (`SCompoundWidget`): Pure Slate widget with drag + edge/corner resize, RO Classic brown/gold theme
- **UBasicInfoSubsystem** (`UWorldSubsystem`): Manages widget lifecycle, wraps Socket.io event callbacks, holds data fields
- **Event Wrapping**: Preserves existing Blueprint handlers while adding C++ data processing

### Event-Driven Architecture
- No polling — all updates driven by Socket.io events
- Widget reads data via `TAttribute` lambdas that bind to subsystem fields
- Subsystem updates fields when socket events arrive; Slate re-reads each frame

---

## Features Implemented

### UI Elements
- **Title Bar** (draggable): Player name (left) + "JobClass Lv.X / Job Lv.Y" (right)
- **HP Bar**: Red fill with "current / max" overlay text, gold border
- **SP Bar**: Blue fill with "current / max" overlay text, gold border
- **Gold Divider Lines**: Between HP/SP section and EXP section
- **Base EXP Bar**: Yellow fill with percentage overlay
- **Job EXP Bar**: Orange fill with percentage overlay
- **Bottom Row**: Weight label + Zuzucoin count on medium-brown background

### Visual Design — RO Classic Brown/Gold Theme
- **Color System**: All colors use `FLinearColor()` constructor directly (NOT `FColor()` which applies sRGB→linear conversion making colors too dark)
- **Panel Frame**: 3-layer border — gold outer (2px) → dark inset (1px) → brown panel body
- **Bars**: Gold-bordered (`GoldDark`) dark background (`BarBg`) with bright saturated fill colors
- **Text**: Warm cream primary text with black drop shadows, gold highlight for labels/currency
- **Gold Dividers**: 1px lines between sections for visual separation
- **Default Size**: 200×155px, min 100×60px, positioned at top-left (10, 10)

### Color Palette (FLinearColor — linear space)
| Name | R | G | B | Usage |
|------|---|---|---|-------|
| PanelBrown | 0.43 | 0.29 | 0.17 | Main panel background |
| PanelDark | 0.22 | 0.14 | 0.08 | Title bar, inset border |
| PanelMedium | 0.33 | 0.22 | 0.13 | Bottom row background |
| GoldTrim | 0.72 | 0.58 | 0.28 | Outer border |
| GoldDark | 0.50 | 0.38 | 0.15 | Bar borders |
| GoldHighlight | 0.92 | 0.80 | 0.45 | Label text |
| GoldDivider | 0.60 | 0.48 | 0.22 | Divider lines |
| HPRed | 0.85 | 0.15 | 0.15 | HP bar fill |
| SPBlue | 0.20 | 0.45 | 0.90 | SP bar fill |
| EXPYellow | 0.90 | 0.75 | 0.10 | Base EXP fill |
| JobExpOrange | 0.90 | 0.55 | 0.10 | Job EXP fill |
| BarBg | 0.10 | 0.07 | 0.04 | Bar dark background |
| TextPrimary | 0.96 | 0.90 | 0.78 | Body text |
| TextBright | 1.00 | 1.00 | 1.00 | Overlay on bars |
| ZuzucoinGold | 0.95 | 0.82 | 0.48 | Currency value |

### Drag Functionality
- Drag from title bar area (top 20px of widget)
- Uses `FVector2D` for mouse math, converts to `FVector2f` for `SetRenderTransform`
- Position persists during gameplay

### Resize Functionality
- **Edge/corner resize**: 6px grab zone on all 4 edges + 4 corners
- **Cursor feedback**: `OnCursorQuery` returns appropriate resize cursors (↔ ↕ ↗ ↘)
- **Min size**: 100×60px, no maximum limit
- **Bitflag edge system**: `EResizeEdge` enum with `ENUM_CLASS_FLAGS` for combining edges

---

## Socket.io Event Integration

### Event Wrapping Strategy
The subsystem **wraps existing Blueprint callbacks** without breaking them:

1. **Polls** every 0.5s for BP_SocketManager + bound `combat:health_update` event
2. **Saves** the original Blueprint callback from `FSocketIONative::EventFunctionMap`
3. **Replaces** with a combined callback that calls:
   - Original Blueprint handler first
   - Our C++ handler second
4. **Preserves** all existing Blueprint functionality

### Character ID Filtering
All event handlers filter by `LocalCharacterId` to prevent other players' broadcast data from overwriting the local player's UI:
- `HandleHealthUpdate`: Checks `characterId` field
- `HandleCombatDamage`: Checks `targetId` + skips `isEnemy=true`
- `HandleCombatDeath`: Checks `killedId`
- `HandleCombatRespawn`: Checks `characterId`
- `HandlePlayerStats`: Checks `characterId` (if present)
- `HandleExpLevelUp`: Checks `characterId` (**critical** — this event is broadcast to all players)

### Wrapped Events
| Event | Data Source | Updated Fields | Filter |
|-------|------------|---------------|--------|
| `combat:health_update` | HP/SP values | CurrentHP, MaxHP, CurrentSP, MaxSP | characterId |
| `combat:damage` | PvP damage | CurrentHP, MaxHP | targetId + !isEnemy |
| `combat:death` | Player killed | CurrentHP → 0 | killedId |
| `combat:respawn` | Player respawn | CurrentHP, MaxHP, CurrentSP, MaxSP | characterId |
| `player:stats` | Derived stats + EXP | MaxHP, MaxSP, STR, levels, EXP | characterId |
| `exp:gain` | EXP payload | Levels, EXP | — (socket-targeted) |
| `exp:level_up` | EXP payload | Levels, EXP, JobClass | characterId |
| `player:joined` | Initial zuzucoin | Zuzucoin | — |
| `shop:bought`/`shop:sold` | Updated zuzucoin | Zuzucoin | — |
| `inventory:data` | Items + zuzucoin | CurrentWeight, Zuzucoin | — |

### Data Refresh (Initial Load Fix)
After wrapping events, subsystem emits:
- `player:request_stats` → triggers `player:stats` + `combat:health_update` response (server re-sends current health)
- `inventory:load` → triggers `inventory:data` response

Server-side fix in `player:request_stats` handler: also emits `combat:health_update` with current HP/SP values, ensuring late-binding UIs get correct initial health.

### Initial Seeding
`PopulateNameFromGameInstance()` seeds from `FCharacterData`:
- `CurrentHP = MaxHP = SelChar.Health` (assume full HP until server corrects)
- `CurrentSP = MaxSP = SelChar.Mana` (assume full SP until server corrects)
- PlayerName, JobClassDisplayName, BaseLevel, JobLevel, BaseExp, JobExp

---

## Data Flow

```
Socket.io Event → Wrapped Callback → Original BP Handler + C++ Handler
                                                    ↓
                                     Subsystem Public Fields Updated
                                                    ↓
                                     Slate Widget TAttribute Lambdas
                                                    ↓
                                     UI Re-renders (no explicit invalidation needed)
```

---

## Viewport Integration

### Alignment Wrapper (Critical)
The widget is wrapped in an alignment `SBox` to prevent it from filling the entire viewport:

```cpp
AlignmentWrapper = SNew(SBox)
    .HAlign(HAlign_Left)
    .VAlign(VAlign_Top)
    .Visibility(EVisibility::SelfHitTestInvisible)  // clicks pass through empty area
    [
        BasicInfoWidget.ToSharedRef()
    ];

ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 10);
```

**Why**: `AddViewportWidgetContent` makes widgets fill the entire viewport. Without the alignment wrapper:
- The `SCompoundWidget` geometry = full viewport → edge resize grab zones at viewport edges (broken)
- Mouse events intercepted across entire screen (blocks gameplay interaction)

With the wrapper:
- `SelfHitTestInvisible` on outer SBox → clicks pass through empty space
- Widget geometry = natural content size (200×155) → edge detection works correctly
- `HAlign_Left + VAlign_Top` → widget starts at top-left, `SetRenderTransform` offsets from there

---

## Technical Details

### Dependencies (Build.cs)
```cpp
"SlateCore",      // Core Slate framework
"SocketIOClient", // Socket.io client plugin
"SIOJson",        // JSON utilities for Socket.io
```

### Key Includes (Widget)
```cpp
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
```

### SocketIOClient Plugin Modification
Added public getter to `USocketIOClientComponent`:
```cpp
TSharedPtr<FSocketIONative> GetNativeClient() const { return NativeClient; }
```

### Bar Row Construction Pattern
Each bar uses a layered overlay:
```
SHorizontalBox
  ├── SBox (48px label) → STextBlock "HP" in gold
  └── SBox (fill, 14px height)
      └── SBorder (GoldDark, 1px padding — bar border)
          └── SOverlay
              ├── SBorder (BarBg — dark background)
              ├── SProgressBar (.Percent, .FillColorAndOpacity)
              └── STextBlock (value overlay, white + shadow)
```

---

## Usage

### Automatic
The subsystem activates automatically when:
1. Game world begins play → `PopulateNameFromGameInstance()` seeds data
2. Timer polls 0.5s for BP_SocketManager + socket bindings
3. Events wrapped + `player:request_stats` / `inventory:load` emitted
4. Widget appears at top-left corner

### Manual Control
```cpp
UBasicInfoSubsystem* Sub = GetWorld()->GetSubsystem<UBasicInfoSubsystem>();
Sub->ShowWidget();
Sub->HideWidget();
bool bVisible = Sub->IsWidgetVisible();
```

---

## Design Patterns Used

- **Component-Based**: Separate widget (SBasicInfoWidget) and data manager (UBasicInfoSubsystem)
- **Event-Driven**: No polling, pure event-driven updates via Socket.io wrapping
- **Dependency Injection**: Subsystem injected into widget via `SLATE_ARGUMENT`
- **Single Responsibility**: Widget = UI rendering + mouse interaction, Subsystem = data + events + lifecycle
- **Manager Pattern**: Subsystem manages widget lifecycle, event wrapping, and data fields

---

## Testing Notes

- **Compilation**: ✅ Successful with UE5.7
- **Socket Integration**: Wraps existing BP events without breaking them
- **Performance**: No per-frame costs, only updates on socket events
- **Memory**: Widget kept alive via `AlignmentWrapper` → `SWeakWidget` chain
- **Thread Safety**: All handlers run on game thread (`ESIOThreadOverrideOption::USE_GAME_THREAD`)
- **Character Filtering**: All broadcast events filtered by `LocalCharacterId`

---

## Common Pitfalls & Lessons Learned

1. **FColor → FLinearColor makes colors too dark**: `FColor(0x9E,0x1C,0x1C)` applies sRGB→linear conversion. Always use `FLinearColor(R, G, B, A)` directly for UI colors.
2. **AddViewportWidgetContent fills viewport**: Must wrap in alignment `SBox` with `SelfHitTestInvisible` to prevent full-screen overlay.
3. **Broadcast events overwrite local data**: `exp:level_up` is broadcast to all players. Always filter by `characterId`.
4. **Initial health missed**: `combat:health_update` fires before event wrapping is ready. Server must re-send health when `player:request_stats` is called.
5. **MaxHP not seeded**: `FCharacterData` has no MaxHealth field. Seed `MaxHP = CurrentHP` initially so bars show full until server corrects.
6. **SProgressBar default style**: The default fill image is tinted by `FillColorAndOpacity`. Combined with sRGB→linear conversion, colors appeared black.

---

## Future Enhancements

- **Save/Load Position**: Persist widget position in player settings
- **Custom Textures**: Ornamental frame, rounded bars, character portrait (requires PNG assets + FSlateBrush)
- **Tooltip Support**: Hover tooltips for EXP values showing time-to-level
- **Animation**: Smooth bar transitions when values change
- **Weight Bar**: Visual progress bar for weight instead of text-only
