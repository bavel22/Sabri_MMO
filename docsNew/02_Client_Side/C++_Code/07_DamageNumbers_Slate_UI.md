# Damage Numbers Slate UI — RO-Style Floating Combat Text

**Files**: `UI/SDamageNumberOverlay.h/.cpp`, `UI/DamageNumberSubsystem.h/.cpp`
**Purpose**: Display Ragnarok Online-style damage numbers with per-digit fan-out spread, rise animation, outlined text, and color-coded damage types.
**Status**: Implemented, pending first in-game test
**Last Updated**: 2026-02-26

---

## Architecture Overview

### Component-Based Architecture
- **SDamageNumberOverlay** (`SLeafWidget`): Fullscreen transparent overlay that renders all active damage numbers via custom `OnPaint` — no child widgets, no UMG
- **UDamageNumberSubsystem** (`UWorldSubsystem`): Manages overlay lifecycle, wraps `combat:damage` Socket.io events, projects world positions to screen space, feeds damage pops into the overlay
- **Event Wrapping**: Preserves existing Blueprint + BasicInfoSubsystem handler chain while adding damage number rendering

### Custom Rendering Architecture
- All damage numbers rendered in a single `OnPaint` call using `FSlateDrawElement::MakeText`
- Each digit rendered individually for RO-style horizontal fan-out spread
- No widget creation/destruction per hit — uses a fixed-size circular buffer pool
- Active timer drives repaint only when entries exist (zero cost when idle)

---

## RO Visual Characteristics Recreated

### Per-Digit Fan-Out Spread (Signature RO Visual)
The defining visual feature of Ragnarok Online's damage numbers. In RO, multi-digit damage values display each digit as a separate sprite that fans outward from center as the number rises.

**Implementation**: Each character in the damage value is rendered as a separate `FSlateDrawElement::MakeText` call with individually calculated positions. As animation progresses, extra horizontal spacing is added between digits using an ease-out curve.

```
Time 0.0s:  "1234" → [1][2][3][4]  (compact, normal text spacing)
Time 0.3s:  "1234" → [1] [2] [3] [4]  (digits spreading outward)
Time 0.6s:  "1234" → [1]  [2]  [3]  [4]  (max spread reached)
Time 1.3s:  "1234" → faded out completely
```

### Bold Outlined Digits
Uses `FSlateFontInfo::OutlineSettings` for native GPU-accelerated outlined text:
- 2px black outline for normal damage (high contrast against any background)
- 3px black outline for critical hits (thicker for emphasis at larger size)
- Outline color: near-black `(0.02, 0.02, 0.02)` for maximum readability

### Rising Motion with Deceleration
Numbers rise 85px upward with a cubic ease-out curve — fast at start, decelerating naturally like gravity:
```cpp
RiseProgress = FMath::InterpEaseOut(0.0f, 1.0f, LifeAlpha, 3.0f);  // Exponent 3
RiseY = -RISE_DISTANCE * RiseProgress;  // Negative Y = upward in screen space
```

### Alpha Fade
Full opacity for the first 65% of the 1.3s lifetime, then linear fade to transparent:
```cpp
if (LifeAlpha > 0.65f)
    Alpha = 1.0f - (LifeAlpha - 0.65f) / 0.35f;
```

### Vertical Stacking
When multiple hits land on the same target in quick succession, numbers offset upward to prevent overlap:
- Detection: entries within 60px and 0.8s of each other are considered "stacked"
- Offset: -26px Y per stacked entry (upward in screen space)

### Random Horizontal Bias
Each damage pop gets a random X offset of ±12px for organic, non-rigid appearance.

---

## Color Coding (RO-Faithful)

| Damage Type | Enum Value | Color | RGB | Font Size | When |
|-------------|-----------|-------|-----|-----------|------|
| Normal damage | `NormalDamage` | Yellow/Gold | `1.00, 0.92, 0.23` | 20pt | Auto-attack damage dealt to enemies |
| Critical hit | `CriticalDamage` | Warm White | `1.00, 1.00, 0.95` | 28pt | Critical hit on enemy (1.4x larger) |
| Player hit | `PlayerHit` | Red | `1.00, 0.20, 0.20` | 20pt | Local player receiving damage |
| Player crit hit | `PlayerCritHit` | Magenta-Red | `1.00, 0.35, 0.55` | 28pt | Local player receiving critical hit |
| Skill damage | `SkillDamage` | Orange | `1.00, 0.65, 0.15` | 22pt | Skill damage (future use) |
| Miss | `Miss` | Light Blue | `0.50, 0.70, 1.00` | 18pt | When damage == 0 |
| Heal | `Heal` | Green | `0.30, 1.00, 0.40` | 19pt | Healing received (future use) |

All colors defined in the `RODamageColors` namespace in `SDamageNumberOverlay.cpp`.

---

## Animation Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `LIFETIME` | 1.3s | Total pop-up duration |
| `RISE_DISTANCE` | 85px | Total pixels to rise upward |
| `SPREAD_PER_DIGIT` | 4.5px | Max extra spacing per digit at full spread |
| `DIGIT_BASE_GAP` | 1.0px | Base gap between digits (always present) |
| `FADE_START` | 0.65 | Alpha fade begins at 65% of lifetime |
| `RANDOM_X_RANGE` | ±12px | Random horizontal bias per pop |
| `STACK_CHECK_RADIUS` | 60px | Distance threshold for stacking detection |
| `STACK_CHECK_TIME` | 0.8s | Time window for stacking detection |
| `STACK_OFFSET_Y` | -26px | Vertical offset per stacked number |
| `HEAD_OFFSET_Z` | 120 units | World-space Z offset above target position |

### Animation Curves

**Rise** — Cubic ease-out (`exponent = 3.0`):
```
Position rises quickly at the start, then decelerates smoothly.
At 25% lifetime: ~58% of total rise distance covered
At 50% lifetime: ~87% of total rise distance covered
At 75% lifetime: ~97% of total rise distance covered
```

**Digit Spread** — Quadratic ease-out (`exponent = 2.0`), reaches max at 50% lifetime:
```
SpreadT = Clamp(LifeAlpha * 2.0, 0.0, 1.0)
SpreadFactor = InterpEaseOut(0.0, 1.0, SpreadT, 2.0)
```

**Alpha Fade** — Linear ramp from 1.0 to 0.0 over the final 35% of lifetime.

---

## Socket.io Event Integration

### Wrapped Event

| Event | Direction | Data Used | Purpose |
|-------|-----------|-----------|---------|
| `combat:damage` | Server → All | attackerId, targetId, damage, isCritical, isEnemy, targetX/Y/Z | All auto-attack and skill damage |

The `combat:damage` event is emitted by the server for **both** auto-attacks and skill hits. This single event drives all damage number display.

### Event Wrapping Strategy
Follows the same pattern as `BasicInfoSubsystem`:
1. Polls every 0.5s for `BP_SocketManager` + bound `combat:health_update` event
2. Saves existing callback chain (BP handler + BasicInfoSubsystem handler)
3. Replaces with combined callback: original chain first, then damage number handler
4. All existing functionality preserved

### Event Payload (combat:damage)
```json
{
    "attackerId": 42,
    "attackerName": "PlayerOne",
    "targetId": 101,
    "targetName": "Poring",
    "isEnemy": true,
    "damage": 156,
    "isCritical": false,
    "targetHealth": 344,
    "targetMaxHealth": 500,
    "attackerX": 1200.0,
    "attackerY": 800.0,
    "attackerZ": 0.0,
    "targetX": 1350.0,
    "targetY": 750.0,
    "targetZ": 0.0,
    "timestamp": 1740600000000
}
```

### Damage Type Determination Logic
```
if damage <= 0           → Miss (light blue "Miss" text)
else if target == local player:
    if isCritical        → PlayerCritHit (magenta-red, 28pt)
    else                 → PlayerHit (red, 20pt)
else if isCritical       → CriticalDamage (white, 28pt)
else                     → NormalDamage (yellow, 20pt)
```

### Visibility Rule
Shows damage for **ALL combat in view** (RO-style: you see all nearby combat, not just your own). Every `combat:damage` event creates a damage pop at the target's position.

---

## Data Flow

```
Server emits combat:damage (broadcast to all clients)
  → Socket.io plugin receives on game thread
    → BP_SocketManager original handler (existing Blueprint damage numbers)
    → BasicInfoSubsystem handler (HP bar updates)
    → DamageNumberSubsystem.HandleCombatDamage()
      ↓
      Extract: damage, isCritical, isEnemy, attackerId, targetId, targetX/Y/Z
      ↓
      SpawnDamagePop()
        ├── If local player is target → use actual pawn position (more accurate)
        ├── Else → use server-sent targetX/Y/Z
        ↓
        Add HEAD_OFFSET_Z (+120 world units) for above-head positioning
        ↓
        ProjectWorldLocationToScreen() → screen-space anchor
        ↓
        Determine EDamagePopType from damage/crit/isEnemy/localPlayer
        ↓
        SDamageNumberOverlay::AddDamagePop()
          ├── Count nearby recent entries for stacking offset
          ├── Apply stack offset + random X bias
          └── Write to circular buffer pool
              ↓
              OnAnimationTick (every frame)
                ├── Clean expired entries
                └── Invalidate paint if active entries exist
                    ↓
                    OnPaint (Slate render thread)
                      └── For each active entry:
                          ├── Compute rise, spread, alpha from elapsed time
                          ├── Build FSlateFontInfo with outline settings
                          └── For each digit: MakeText at calculated position
```

---

## Viewport Integration

### Fullscreen Overlay (No Alignment Wrapper)
Unlike `SBasicInfoWidget`, the damage overlay needs to fill the entire viewport for absolute positioning of numbers anywhere on screen:

```cpp
OverlayWidget = SNew(SDamageNumberOverlay);
ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(OverlayWidget);
ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 20);
```

### Z-Order
| Widget | Z-Order | Purpose |
|--------|---------|---------|
| BasicInfo panel | 10 | HP/SP/EXP HUD |
| SkillTree panel | 15 | Skill tree window |
| **Damage numbers** | **20** | **Floating combat text** |
| Targeting overlay | 25 | Skill targeting cursor |

### Hit Testing
`SetVisibility(EVisibility::HitTestInvisible)` — the overlay is completely invisible to mouse input. All clicks pass through to the game world.

---

## Object Pool Design

### Pool Structure
```cpp
static constexpr int32 MAX_ENTRIES = 64;
FDamagePopEntry Entries[MAX_ENTRIES];  // Fixed-size array, no heap allocation
int32 NextEntryIndex = 0;             // Circular buffer write head
int32 ActiveCount = 0;                // Rough count for paint skip optimization
```

### Entry Lifecycle
1. **Spawn**: `AddDamagePop()` writes to `Entries[NextEntryIndex]`, increments index mod 64
2. **Active**: Entry has `bActive = true` and `(Now - SpawnTime) < LIFETIME`
3. **Expire**: `OnAnimationTick` sets `bActive = false` when lifetime exceeded
4. **Reuse**: Oldest entry silently overwritten when pool wraps (circular buffer)

### Performance Characteristics
- **Zero allocation per hit**: All entries pre-allocated in a fixed array
- **Zero widget churn**: No Slate widgets created/destroyed during combat
- **Idle optimization**: `Invalidate(Paint)` only called when `ActiveCount > 0`
- **Max concurrent**: 64 simultaneous damage pops (covers intense AoE scenarios)
- **Draw calls per digit**: 1 `MakeText` call (outline handled by GPU font renderer)
- **Worst case per frame**: 64 entries × ~5 digits × 1 draw call = ~320 text draw elements

---

## Structs and Enums

### EDamagePopType
```cpp
enum class EDamagePopType : uint8
{
    NormalDamage,    // Yellow/gold — auto-attack damage dealt to enemies
    CriticalDamage,  // White, larger — critical hit on enemy
    PlayerHit,       // Red — local player receiving damage
    PlayerCritHit,   // Bright red, larger — local player receiving a critical hit
    SkillDamage,     // Orange — skill damage dealt to enemies
    Miss,            // Light blue — "Miss" text
    Heal             // Green — healing received
};
```

### FDamagePopEntry
```cpp
struct FDamagePopEntry
{
    bool bActive = false;
    int32 Value = 0;                           // Damage amount (0 = miss)
    EDamagePopType Type = NormalDamage;
    FVector2D ScreenAnchor = FVector2D::Zero;  // Screen-space anchor at spawn time
    double SpawnTime = 0.0;                    // FPlatformTime::Seconds()
    float RandomXBias = 0.0f;                  // ±12px horizontal randomization
};
```

---

## World-to-Screen Projection

### Position Pipeline
```
Server: target world position (targetX, targetY, targetZ)
  → Add HEAD_OFFSET_Z (+120 units on Z axis) for above-head placement
  → Special case: if target is local player, use actual pawn position instead
  → APlayerController::ProjectWorldLocationToScreen()
  → Result: screen-space FVector2D anchor
```

### Local Player Accuracy
When the local player is the damage target, the subsystem uses the actual pawn position (`PC->GetPawn()->GetActorLocation()`) instead of the server-sent position. This avoids latency-induced position mismatch where server coordinates may be slightly behind the client's predicted position.

### Off-Screen Handling
If `ProjectWorldLocationToScreen` returns false (target behind camera or off-screen), the damage pop is silently discarded. No damage numbers appear for off-screen combat.

---

## Technical Details

### Dependencies (Already in Build.cs)
```cpp
"Slate",          // SLeafWidget base class
"SlateCore",      // FSlateDrawElement, FSlateFontInfo, FSlateFontMeasure
"SocketIOClient", // Socket.io client plugin
"SIOJson",        // JSON utilities for Socket.io
"Json",           // JSON parsing
```

### Key Includes (Overlay)
```cpp
#include "Rendering/DrawElements.h"           // FSlateDrawElement::MakeText
#include "Framework/Application/SlateApplication.h"  // Font measure service
#include "Fonts/FontMeasure.h"                // FSlateFontMeasure
#include "Styling/CoreStyle.h"                // FCoreStyle::GetDefaultFontStyle
```

### Key Includes (Subsystem)
```cpp
#include "SocketIOClientComponent.h"          // Socket.io component
#include "SocketIONative.h"                   // Native client + EventFunctionMap
#include "MMOGameInstance.h"                   // Character ID resolution
#include "GameFramework/PlayerController.h"   // World-to-screen projection
```

### Font Rendering
Uses UE5's default Roboto font in Bold weight with GPU-rendered outlines:
```cpp
FSlateFontInfo DigitFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
DigitFont.OutlineSettings.OutlineSize = (int32)OutlineSize;  // 2 or 3px
DigitFont.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);
```

### Per-Digit Positioning Math
```cpp
// EffectiveDigitSpacing grows with spread animation
float EffectiveDigitSpacing = DigitWidth + DIGIT_BASE_GAP + SpreadFactor * SPREAD_PER_DIGIT;

// Center the digit group symmetrically around the anchor X
for (int32 c = 0; c < NumChars; ++c)
{
    float DigitCenterOffset = (c - (NumChars - 1) * 0.5f) * EffectiveDigitSpacing;
    FVector2f DrawPos(BasePos.X + DigitCenterOffset - DigitW * 0.5f, BasePos.Y - DigitH * 0.5f);
    // ... MakeText at DrawPos
}
```

---

## Coexistence with Blueprint System

The new C++ Slate damage numbers run **alongside** the existing Blueprint `WBP_DamageNumber` system. Both display damage numbers simultaneously during the transition period.

| System | Technology | Triggered By | Widget Lifecycle |
|--------|-----------|-------------|-----------------|
| **Old** (Blueprint) | UMG `WBP_DamageNumber` | `AC_HUDManager.ShowDamageNumbers` via BP event | Create → Animate → Remove from Parent |
| **New** (C++ Slate) | `SDamageNumberOverlay` OnPaint | `DamageNumberSubsystem` via event wrapping | Persistent overlay, pool entries recycled |

To disable the old system: remove or bypass the `ShowDamageNumbers` call in `AC_HUDManager`'s `OnCombatDamage` event handler in Blueprint.

---

## Usage

### Automatic
The subsystem activates automatically:
1. Game world begins play → resolve `LocalCharacterId` from `UMMOGameInstance`
2. Timer polls 0.5s for `BP_SocketManager` + socket bindings
3. Wraps `combat:damage` event (preserving full handler chain)
4. Overlay added to viewport at Z-order 20
5. All combat in view generates floating damage numbers

### Manual Control (if needed)
```cpp
UDamageNumberSubsystem* Sub = GetWorld()->GetSubsystem<UDamageNumberSubsystem>();
// Subsystem is fully automatic — no manual calls needed
// Overlay shows/hides with subsystem lifecycle
```

### Direct Pop-Up Spawning (for custom use)
```cpp
// Access the overlay widget from the subsystem (would need to expose getter)
// OverlayWidget->AddDamagePop(Damage, EDamagePopType::NormalDamage, ScreenPosition);
```

---

## Design Patterns Used

- **Object Pool**: Fixed 64-entry circular buffer, zero allocation per hit
- **Custom OnPaint Rendering**: Full pixel-level control over digit positions and animation
- **Event Wrapping**: Preserves existing handler chain (BP + BasicInfoSubsystem)
- **Component-Based**: Separate overlay widget and data/event subsystem
- **Active Timer Animation**: Frame-accurate animation with idle optimization
- **World-to-Screen Projection**: Server world coordinates mapped to screen-space anchors

---

## Future Enhancements

- **Element-Based Colors**: Use `skill:effect_damage` event for fire (red), water (blue), earth (brown), wind (green) skill coloring
- **Healing Numbers**: Hook into `combat:health_update` to detect HP increases and show green numbers
- **Custom Digit Textures**: Replace font rendering with imported RO-style bitmap digit sprites via `FSlateBrush`
- **Critical Hit Pop Animation**: Scale overshoot (1.8x → 1.0x) during the first 150ms for crits
- **Combo Counter**: Track rapid successive hits and display combo count
- **Configurable Toggle**: Runtime toggle between old Blueprint and new Slate damage numbers
- **DPI Scaling**: Adjust font sizes and animation distances based on viewport DPI scale
