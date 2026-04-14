# Damage Numbers Slate UI — RO Classic Floating Combat Text

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Combat_System](../../03_Server_Side/Combat_System.md)

**Files**: `UI/SDamageNumberOverlay.h/.cpp`, `UI/DamageNumberSubsystem.h/.cpp`
**Purpose**: Display RO Classic-faithful damage numbers with parabolic sine arc, scale shrink, diagonal drift, and per-type animation curves.
**Status**: Implemented, RO Classic animation overhaul complete
**Last Updated**: 2026-04-05
**Skill**: `/sabrimmo-damage-numbers`

---

## Architecture Overview

### Two-Class Pattern
- **SDamageNumberOverlay** (`SCompoundWidget`): Fullscreen transparent overlay that renders all active damage numbers via `OnPaint` with per-digit rendering and dynamic font scaling
- **UDamageNumberSubsystem** (`UWorldSubsystem`): Manages overlay lifecycle, registers socket events via `EventRouter->RegisterHandler()`, projects world positions to screen space, determines damage type from server events

### Custom Rendering Architecture
- All damage numbers rendered in a single `OnPaint` call using `FSlateDrawElement::MakeText`
- Each digit rendered individually with dynamic font sizing based on animation scale
- Fixed 64-entry circular buffer pool — zero allocation per hit
- Active timer drives repaint every frame

---

## RO Classic Animation System

Based on research from roBrowser's `Damage.js` source code. Three distinct animation paths:

### 1. Damage Numbers — Sine Arc + Drift + Shrink

The signature RO feel — numbers pop in large, arc upward while drifting diagonally, then shrink away to nothing.

```cpp
// Horizontal drift (alternates left/right per entry)
OffsetX = DriftDirection * 60px * t;

// Parabolic sine arc
SineInput = -PI/2 + PI * (0.5 + t * 1.5);
OffsetY = -(30 + sin(SineInput) * 120);  // Peaks at +150px, returns, dips below

// Scale: 2.5x → 0x linear shrink
Scale = max(0.01, (1 - t) * 2.5);

// Alpha: immediate linear fade
Alpha = 1.0 - t;
```

**Duration**: 1.5 seconds

### 2. Miss / Dodge / Block — Straight Rise

```cpp
OffsetY = -(20 + 100 * t);  // Linear vertical rise
Scale = 0.7;                 // Fixed (no shrinking)
Alpha = 1.0 - t;
```

**Duration**: 0.8 seconds

### 3. Heal — Stationary Then Rise

```cpp
// Phase 1 (0-40%): holds still at -30px
// Phase 2 (40-100%): rises upward at 80px/sec
Scale = max(0.8, (1 - t*2) * 2.5);  // Quick shrink to 0.8x then hold
Alpha = 1.0 - t;
```

**Duration**: 1.5 seconds

---

## Color Coding (RO Classic Faithful)

| Type | Enum | Color | RGB | Font Size |
|------|------|-------|-----|-----------|
| Normal damage | `NormalDamage` | **White** | `(1.0, 1.0, 1.0)` | 20pt |
| Critical hit | `CriticalDamage` | **Yellow** | `(0.9, 0.9, 0.15)` | 28pt |
| Player hit | `PlayerHit` | **Red** | `(1.0, 0.0, 0.0)` | 20pt |
| Player crit | `PlayerCritHit` | **Magenta-Red** | `(1.0, 0.35, 0.55)` | 28pt |
| Skill damage | `SkillDamage` | **Orange** | `(1.0, 0.65, 0.15)` | 22pt |
| Miss | `Miss` | **Light Blue** | `(0.5, 0.7, 1.0)` | 18pt |
| Heal | `Heal` | **Green** | `(0.0, 1.0, 0.0)` | 19pt |
| Dodge | `Dodge` | **Green** | `(0.4, 0.9, 0.5)` | 18pt |
| Lucky Dodge | `PerfectDodge` | **Gold** | `(0.95, 0.85, 0.2)` | 18pt |
| Block | `Block` | **Silver** | `(0.85, 0.9, 1.0)` | 18pt |

All colors in `RODamageColors` namespace. Font sizes multiplied by `FontScaleMultiplier` (user option), then by animation `Scale` factor.

### Element Tinting (Enhancement)
Non-neutral elemental attacks blend 40% toward element color via HSV lerp. Applied to damage numbers only (not heals or text labels).

---

## Socket Event Integration

### Events Registered (via EventRouter)

| Event | Handler | Creates |
|-------|---------|---------|
| `combat:damage` | `HandleCombatDamage` | Damage/miss/crit numbers + dual wield |
| `skill:effect_damage` | `HandleCombatDamage` | Skill damage + heal numbers |
| `combat:blocked` | `HandleCombatBlocked` | "Block" text (Auto Guard) |
| `status:tick` | `HandleStatusTick` | Periodic damage numbers (poison/bleed/stone) |
| `status:applied` | `HandleStatusApplied` | Status text ("Poisoned!", "Stunned!") |

### Damage Type Determination

```
hitType "heal"         → Heal (green, uses healAmount)
hitType "miss"         → Miss (blue "Miss" text)
hitType "dodge"        → Dodge (green "Dodge" text)
hitType "perfectDodge" → PerfectDodge (gold "Lucky Dodge" text)
hitType "blocked"      → Block (silver "Block" text)
damage <= 0            → Miss (fallback)
localPlayer + crit     → PlayerCritHit
localPlayer            → PlayerHit
crit                   → CriticalDamage
else                   → NormalDamage
```

---

## Data Flow

```
Server emits combat:damage (broadcast to zone)
  → EventRouter dispatches to DamageNumberSubsystem
    → HandleCombatDamage() extracts all fields
      → SpawnDamagePop()
        ├── If local player target → use pawn position (latency-safe)
        ├── Else → use server targetX/Y/Z
        ├── Add HEAD_OFFSET_Z (+120 world units)
        ├── ProjectWorldLocationToScreen()
        ├── Determine EDamagePopType
        └── OverlayWidget->AddDamagePop()
              ├── Count nearby entries → stacking offset
              ├── Set Lifetime per type
              ├── Set DriftDirection (alternating left/right)
              └── Write to circular buffer pool
                    → OnPaint renders with per-type animation
```

---

## Viewport Integration

### Fullscreen Overlay (No Alignment Wrapper)
```cpp
OverlayWidget = SNew(SDamageNumberOverlay);
ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(OverlayWidget);
ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 20);
```

### Z-Order
| Widget | Z-Order |
|--------|---------|
| BasicInfo panel | 10 |
| SkillTree panel | 15 |
| **Damage numbers** | **20** |
| Targeting overlay | 25 |

Visibility: `EVisibility::SelfHitTestInvisible` — all clicks pass through.

---

## Object Pool

```cpp
static constexpr int32 MAX_ENTRIES = 64;
FDamagePopEntry Entries[MAX_ENTRIES];  // Fixed array, zero heap allocation
int32 NextEntryIndex = 0;             // Circular buffer write head
```

### FDamagePopEntry
```cpp
struct FDamagePopEntry {
    bool bActive = false;
    int32 Value = 0;
    EDamagePopType Type;
    FVector2D ScreenAnchor;
    double SpawnTime = 0.0;
    float RandomXBias = 0.0f;       // +/-12px jitter
    FString Element;                // For element tinting
    FString TextLabel;              // Status text override
    FLinearColor CustomColor;       // Color for TextLabel
    bool bHasCustomColor = false;
    float Lifetime = 1.5f;          // Per-type duration
    float DriftDirection = 1.0f;    // +1 right, -1 left, 0 none
};
```

---

## Options Integration

Set by `OptionsSubsystem`:
- `bDamageNumbersEnabled` — global toggle
- `bShowMissText` — hides miss/dodge/perfect dodge text when false
- `DamageNumberScale` — multiplier (0.75/1.0/1.25) applied via `FontScaleMultiplier`

---

## Research Reference

Full RO Classic damage number research: `docsNew/05_Development/Damage_Number_RO_Classic_Research.md`
Based on roBrowser source (`Damage.js`), roBrowserLegacy, rAthena (`clif.hpp`), Ragnarok Research Lab.
