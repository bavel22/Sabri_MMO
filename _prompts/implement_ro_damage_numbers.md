# Prompt: Implement RO Classic Damage Numbers & Hit Impact

## Context

Load these skills before starting: `/sabrimmo-combat`, `/sabrimmo-ui`, `/sabrimmo-sprites`

Research document: `docsNew/05_Development/Damage_Number_RO_Classic_Research.md`

## Goal

Rewrite the damage number animation system in `SDamageNumberOverlay` and `DamageNumberSubsystem` to faithfully recreate Ragnarok Online Classic's iconic damage number feel. The current system uses a simple ease-out vertical rise with fixed font sizes. RO Classic uses a parabolic sine arc with diagonal drift, 4x→0x scale shrinking, and immediate alpha fade — making numbers feel like they "pop in big then fall away into nothing."

## Files to Modify

| File | Changes |
|------|---------|
| `client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.h` | Update FDamagePopEntry struct, add new constants, add starburst texture |
| `client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.cpp` | Rewrite OnPaint animation curves, add per-type animation, add starburst rendering |
| `client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.h` | Update spawn API if needed, add starburst texture UPROPERTY |
| `client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.cpp` | Update color assignments, spawn logic adjustments |

## Detailed Implementation Steps

### Step 1: Replace Animation Constants

Remove these current constants:
```cpp
// REMOVE
static constexpr float LIFETIME = 1.3f;
static constexpr float RISE_DISTANCE = 85.0f;
static constexpr float FADE_START = 0.65f;
static constexpr float SPREAD_PER_DIGIT = 4.5f;
```

Add per-type timing constants:
```cpp
// RO Classic durations
static constexpr float LIFETIME_DAMAGE = 1.5f;     // Normal/crit damage
static constexpr float LIFETIME_MISS = 0.8f;        // Miss/dodge/lucky dodge/block
static constexpr float LIFETIME_HEAL = 1.5f;        // Heal numbers
static constexpr float LIFETIME_COMBO = 3.0f;        // Combo total (future)

// Arc parameters (converted from RO world units to screen pixels)
static constexpr float ARC_AMPLITUDE = 120.0f;       // Height of sine arc in pixels (RO: 5 world units)
static constexpr float ARC_BASE_OFFSET = 30.0f;      // Starting offset above spawn point (RO: 2 world units)
static constexpr float DRIFT_DISTANCE = 60.0f;       // Horizontal drift in pixels (RO: 4 world units)

// Scale parameters
static constexpr float SCALE_START = 3.0f;           // Starting scale multiplier (RO: 4.0, tuned for screen)
static constexpr float SCALE_END = 0.0f;             // Ending scale (shrinks to nothing)
static constexpr float MISS_SCALE = 0.7f;            // Fixed scale for miss/dodge text
static constexpr float MISS_RISE_DISTANCE = 100.0f;  // How far miss text rises straight up

// Heal-specific
static constexpr float HEAL_STATIONARY_PCT = 0.4f;   // Heal holds still for first 40%
static constexpr float HEAL_SCALE_START = 2.5f;       // Heal starts at 2.5x
static constexpr float HEAL_SCALE_FLOOR = 0.8f;       // Heal shrinks to 0.8x then holds
static constexpr float HEAL_RISE_SPEED = 80.0f;       // Pixels per second during rise phase
```

### Step 2: Add Per-Entry Duration to FDamagePopEntry

```cpp
struct FDamagePopEntry {
    // ... existing fields ...
    float Lifetime;           // Per-entry duration (type-dependent)
    float DriftDirection;     // +1.0 or -1.0 for left/right drift variation
};
```

Set `Lifetime` based on type at spawn time:
- `NormalDamage`, `CriticalDamage`, `PlayerHit`, `PlayerCritHit`, `SkillDamage` → `LIFETIME_DAMAGE`
- `Miss`, `Dodge`, `PerfectDodge`, `Block` → `LIFETIME_MISS`
- `Heal` → `LIFETIME_HEAL`

Alternate `DriftDirection` for visual variety: `+1.0` for odd entries, `-1.0` for even, or random.

### Step 3: Rewrite the OnPaint Animation (Core Change)

This is the most important change. Replace the current animation in `OnPaint()` with per-type animation curves.

#### Normal/Crit/Skill Damage — Sine Arc + Drift + Shrink

```cpp
// t = normalized time [0.0 → 1.0]
float t = ElapsedTime / Entry.Lifetime;

// -- Horizontal drift (diagonal feel) --
float DriftX = Entry.DriftDirection * DRIFT_DISTANCE * t;

// -- Vertical sine arc (THE signature RO curve) --
// Z = BaseOffset + sin(-PI/2 + PI * (0.5 + t * 1.5)) * Amplitude
float SineInput = -PI / 2.0f + PI * (0.5f + t * 1.5f);
float ArcY = -(ARC_BASE_OFFSET + FMath::Sin(SineInput) * ARC_AMPLITUDE);
// Negative because screen Y is inverted (up = negative)

// -- Scale: linear shrink from SCALE_START to 0 --
float Scale = FMath::Max(0.0f, (1.0f - t) * SCALE_START);

// -- Alpha: immediate linear fade --
float Alpha = FMath::Max(0.0f, 1.0f - t);

// Apply scale to font size
float ScaledFontSize = BaseFontSize * Scale;

// Final screen position
float FinalX = Entry.ScreenAnchor.X + DriftX;
float FinalY = Entry.ScreenAnchor.Y + ArcY;
```

#### Miss/Dodge/Lucky Dodge/Block — Straight Rise, Fixed Scale

```cpp
float t = ElapsedTime / Entry.Lifetime;

// No horizontal drift
float DriftX = 0.0f;

// Linear vertical rise (no arc)
float RiseY = -(30.0f + MISS_RISE_DISTANCE * t);

// Fixed scale (no shrinking)
float Scale = MISS_SCALE;

// Linear alpha fade
float Alpha = FMath::Max(0.0f, 1.0f - t);

float FinalX = Entry.ScreenAnchor.X;
float FinalY = Entry.ScreenAnchor.Y + RiseY;
```

#### Heal — Stationary Then Rise, Quick Shrink Then Hold

```cpp
float t = ElapsedTime / Entry.Lifetime;

// No horizontal drift
float DriftX = 0.0f;

// Phase 1 (0-40%): stationary. Phase 2 (40-100%): rise
float RiseY;
if (t < HEAL_STATIONARY_PCT) {
    RiseY = -ARC_BASE_OFFSET;  // Just base offset, no movement
} else {
    float RiseT = (t - HEAL_STATIONARY_PCT) / (1.0f - HEAL_STATIONARY_PCT);
    RiseY = -(ARC_BASE_OFFSET + HEAL_RISE_SPEED * Entry.Lifetime * RiseT);
}

// Quick shrink from 2.5x to 0.8x, then hold at 0.8x
float HealScale = FMath::Max(HEAL_SCALE_FLOOR, (1.0f - t * 2.0f) * HEAL_SCALE_START);

float Alpha = FMath::Max(0.0f, 1.0f - t);

float FinalX = Entry.ScreenAnchor.X;
float FinalY = Entry.ScreenAnchor.Y + RiseY;
```

### Step 4: Fix Color Scheme (Swap Normal ↔ Crit)

In `DamageNumberSubsystem.cpp`, swap the color assignments:

```cpp
// BEFORE (wrong — inverted from RO)
// NormalDamage = Yellow, CriticalDamage = White

// AFTER (matches RO Classic)
static const FLinearColor NormalWhite(1.0f, 1.0f, 1.0f);          // Normal damage = WHITE
static const FLinearColor CritYellow(0.9f, 0.9f, 0.15f);          // Crit damage = YELLOW
static const FLinearColor PlayerRed(1.0f, 0.0f, 0.0f);            // Player-received = pure RED (was 0.2 before)
static const FLinearColor PlayerCritRed(1.0f, 0.35f, 0.55f);      // Player crit = bright magenta-red (keep)
static const FLinearColor HealGreen(0.0f, 1.0f, 0.0f);            // Heal = pure GREEN (was 0.3/1.0/0.4)
static const FLinearColor MissBlue(0.5f, 0.7f, 1.0f);             // Miss = keep
static const FLinearColor DodgeGreen(0.4f, 0.9f, 0.5f);           // Dodge = keep
static const FLinearColor PerfDodgeGold(0.95f, 0.85f, 0.2f);      // Lucky Dodge = keep
static const FLinearColor BlockSilver(0.85f, 0.9f, 1.0f);         // Block = keep
```

### Step 5: Add Critical Starburst Background

Add a grey starburst texture that renders BEHIND critical hit numbers.

#### In SDamageNumberOverlay.h:
```cpp
// Starburst texture for crits (loaded from a UTexture2D asset)
UPROPERTY()
UTexture2D* CritStarburstTexture;

FSlateBrush CritStarburstBrush;
```

#### In SDamageNumberOverlay.cpp OnPaint:
```cpp
// For CriticalDamage and PlayerCritHit types, draw starburst BEFORE number
if (Entry.Type == EDamagePopType::CriticalDamage || Entry.Type == EDamagePopType::PlayerCritHit) {
    if (CritStarburstTexture) {
        // Size at 60% of number bounding box, offset -6px up
        float BurstSize = ScaledFontSize * 3.0f;  // Approximate bounding size
        FVector2D BurstPos(FinalX - BurstSize * 0.5f, FinalY - BurstSize * 0.5f - 6.0f);
        
        FLinearColor BurstColor(0.66f, 0.66f, 0.66f, Alpha);  // Grey tint
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(FVector2D(BurstSize, BurstSize), FSlateLayoutTransform(BurstPos)),
            &CritStarburstBrush,
            ESlateDrawEffect::None,
            BurstColor
        );
    }
}
// Then draw the number digits on LayerId + 1 (above starburst)
```

#### Starburst Texture Asset:
Create a simple starburst/spike-burst texture (white, ~128x128, transparent background). Can be created in any image editor — a radial spike pattern. Import to `Content/SabriMMO/UI/Textures/T_CritStarburst.uasset`.

Alternatively, render the starburst procedurally using Slate draw primitives (lines radiating from center), avoiding the need for a texture asset.

### Step 6: Scale Font Size by Animation Scale

The current system uses fixed font sizes per type. With the RO scaling, the font size must change every frame based on the `Scale` value.

Current approach uses cached `FSlateFontInfo`. With dynamic scaling, either:

**Option A: Scale the paint geometry transform** (preferred — less font cache churn)
```cpp
// Apply scale via FSlateLayoutTransform on the paint geometry
// Draw at base font size but scale the geometry
FSlateLayoutTransform ScaleTransform(Scale, FinalPosition);
```

**Option B: Rebuild font each frame with scaled size** (simpler but more cache pressure)
```cpp
float ScaledSize = FMath::Max(8.0f, BaseFontSize * Scale * FontScaleMultiplier);
FSlateFontInfo ScaledFont = FCoreStyle::GetDefaultFontStyle("Bold", FMath::RoundToInt(ScaledSize));
```

Option A is better for performance. The `MakeText` calls use the base font, and the entire digit group is drawn into a scaled geometry transform.

### Step 7: Update Digit Rendering for Scale

The per-digit rendering loop in OnPaint needs to account for the dynamic scale:

```cpp
// Calculate digit positions at BASE scale, then apply scale transform to the group
float BaseDigitWidth = CachedDigitWidth;  // Measured at base font size
float BaseGap = DIGIT_BASE_GAP;
float TotalBaseWidth = NumDigits * BaseDigitWidth + (NumDigits - 1) * BaseGap;
float StartX = -TotalBaseWidth * 0.5f;  // Center around origin

// Draw each digit relative to origin
for (int32 i = 0; i < NumDigits; i++) {
    float DigitX = StartX + i * (BaseDigitWidth + BaseGap);
    // ... MakeText at (DigitX, 0) relative to group origin ...
}

// Apply group transform: translate to FinalPos + scale around center
```

### Step 8: Adjust Stacking Logic

The current stacking system (checking for nearby recent entries and offsetting upward) should be adjusted:

- Since numbers now drift horizontally, stacking is less of an issue — numbers naturally separate
- Reduce `STACK_CHECK_RADIUS` or remove the stacking offset entirely
- Keep the `RandomXBias` for slight variation, but the drift handles separation
- Consider alternating drift direction (left/right) for consecutive hits to spread them

### Step 9: (Optional) Add Hit Impact Effects

These are additive improvements beyond the damage numbers themselves:

#### 9a. Target Hit Flash
When a target takes damage, briefly flash the sprite material:
```cpp
// In EnemySubsystem or SpriteCharacterActor:
void PlayHitFlash() {
    // Set a "flash" material parameter to 1.0
    // Lerp back to 0.0 over 100ms
    // This creates a brief white flash on the sprite
}
```

#### 9b. Hit Impact Particle
Spawn a small Niagara particle burst at the target position:
- Small white/yellow sparks or slash marks
- 3-5 particles, 200ms lifetime
- Spread radially from impact point
- Different effects per weapon type (slash for swords, sparks for maces)

#### 9c. Positional Hit Sound
Play a weapon-type-specific hit sound at the target's location:
```cpp
// USoundBase* per weapon type, loaded in subsystem constructor
// On combat:damage received, play the appropriate sound
UGameplayStatics::PlaySoundAtLocation(World, HitSound, TargetLocation);
```

#### 9d. Target Recoil/Flinch
Brief position offset on the target sprite to simulate flinch:
```cpp
// Offset sprite actor 5-10 units away from attacker for 100ms
// Then lerp back to original position over 100ms
// Skip if target has Endure buff
```

### Step 10: Test and Tune

After implementation, tune these values in-game:

1. **ARC_AMPLITUDE** — if arc feels too high/low, adjust (try 80-150px range)
2. **DRIFT_DISTANCE** — if numbers drift too far/little (try 40-80px range)
3. **SCALE_START** — if initial pop is too big/small (try 2.5-4.0 range)
4. **Timing** — 1500ms is the RO value, but may need 1200-1800ms tuning for our camera distance
5. **Font base size** — the scaled font needs a good base size that looks right at 1x scale (mid-animation)
6. **Starburst size** — relative to the digit group bounding box

### Reference: RO Classic Arc Visualization

```
Screen position over time (approximate):

t=0.0   **** (4x scale, full alpha, at spawn point)
         \
t=0.1     *** (3.6x, drift right, rising)
            \
t=0.2       ** (3.2x, still rising)
              \
t=0.33         * (2.7x, PEAK of arc)  ← highest point
              /
t=0.5       * (2.0x, descending, 50% alpha)
           /
t=0.67   . (1.3x, back to spawn height, 33% alpha)
        /
t=0.8  . (0.8x, below spawn, 20% alpha, barely visible)
      /
t=1.0 (gone — 0x scale, 0 alpha)
```

The combination of shrinking + fading + arcing downward creates the "number falls away into the distance" feel that defines RO Classic combat.

## What NOT to Change

- Keep element tinting (our enhancement, RO doesn't have it)
- Keep status effect floating text ("Poisoned!" etc.)
- Keep the circular pool architecture (64 entries, efficient)
- Keep options integration (toggle, scale multiplier)
- Keep dual-wield damage number separation
- Keep per-digit rendering approach (just add scale transform)
- Keep DPI-aware scaling logic
- Keep the event registration pattern (both combat:damage and skill:effect_damage)

## Files to Read Before Starting

1. `client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.h` — current struct/constants
2. `client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.cpp` — current OnPaint rendering
3. `client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.h` — spawn API
4. `client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.cpp` — color/type assignment, event handlers
5. `docsNew/05_Development/Damage_Number_RO_Classic_Research.md` — full research reference
