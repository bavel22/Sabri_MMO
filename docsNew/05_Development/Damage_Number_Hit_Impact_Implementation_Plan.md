# Damage Number & Hit Impact — Remaining Features Implementation Plan

**Date**: 2026-04-05
**Depends on**: RO Classic damage number overhaul (completed 2026-04-05)
**Research**: `Damage_Number_RO_Classic_Research.md`, roBrowser source, rAthena source
**Skill**: `/sabrimmo-damage-numbers`

---

## Table of Contents

1. [Status Effect Coloring Analysis](#status-effect-coloring-analysis)
2. [Phase 1: Hit Flash on Target Sprite](#phase-1-hit-flash-on-target-sprite)
3. [Phase 2: Hit Impact Particles](#phase-2-hit-impact-particles)
4. [Phase 3: Critical Starburst Background](#phase-3-critical-starburst-background)
5. [Phase 4: Combo Total Display](#phase-4-combo-total-display)
6. [Phase 5: Target Flinch / Recoil](#phase-5-target-flinch--recoil)
7. [Phase 6: Hit Sounds](#phase-6-hit-sounds)
8. [File Change Matrix](#file-change-matrix)
9. [Research Sources](#research-sources)

---

## Status Effect Coloring Analysis

### Current Behavior

**Status tick damage** (poison/bleeding/stone periodic drain via `status:tick`):
- `HandleStatusTick()` maps status type to an element: poison→"poison", stone→"earth", bleeding→"fire"
- Calls `SpawnDamagePop()` with `hitType="normal"` and the mapped element
- `SpawnDamagePop()` determines type as `NormalDamage` (white) or `PlayerHit` (red)
- OnPaint applies element tinting: 40% HSV blend toward element color

**Result**: Poison drain on enemies = white blended 40% toward purple = **light lavender** (subtle).
Bleeding drain = white blended 40% toward fire orange = **light salmon** (subtle).
These are too subtle against busy backgrounds.

**Status applied text** ("Poisoned!", "Stunned!", etc. via `status:applied`):
- `HandleStatusApplied()` calls `SpawnTextPop()` with `GetStatusColor()` and `GetStatusDisplayName()`
- `AddTextPop()` sets `bHasCustomColor = true` and `Entry.Type = EDamagePopType::Miss`
- OnPaint: `!Entry.TextLabel.IsEmpty()` evaluates true → `bIsMissType = true`
- **Uses miss animation path**: straight vertical rise, fixed 0.7x scale, 0.8s duration, no drift
- This already matches the user's request ("should appear like the miss text")

### Recommended Fix for Status Damage Colors

**Problem**: Status tick damage uses element tinting (40% blend) which is too subtle on a white base color.

**Solution**: Use the full status color directly for status tick damage instead of element tinting. Add an optional custom color parameter to `SpawnDamagePop()` → `AddDamagePop()`:

```cpp
// DamageNumberSubsystem — modified SpawnDamagePop signature:
void SpawnDamagePop(..., const FLinearColor* CustomColor = nullptr);

// SDamageNumberOverlay — modified AddDamagePop signature:
void AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition,
    const FString& Element = TEXT(""), const FLinearColor* CustomColor = nullptr);
```

In `HandleStatusTick()`:
```cpp
FLinearColor StatusColor = GetStatusColor(StatusType);
SpawnDamagePop(Drain, false, bIsEnemy, 0, TargetId, TargetPos,
    TEXT("normal"), TEXT("neutral"), &StatusColor);
```

In `AddDamagePop()`:
```cpp
if (CustomColor)
{
    Entry.CustomColor = *CustomColor;
    Entry.bHasCustomColor = true;
}
```

This gives full-saturation colors:
- Poison drain: **purple** (0.7, 0.3, 0.8) — clearly visible
- Bleeding drain: **red** (1.0, 0.2, 0.2) — clearly visible
- Stone drain: **brown** (0.7, 0.55, 0.3) — clearly visible

The status text ("Poisoned!") already uses these exact colors via `bHasCustomColor`, so they match.

### Summary Table

| Display | Current Animation | Current Color | Needs Change? |
|---------|------------------|---------------|---------------|
| Status text ("Poisoned!") | Miss path (straight rise, 0.7x, 0.8s) | Full status color (purple/yellow/blue) | No — already correct |
| Status drain number (5, 12, etc.) | Damage arc (sine + drift + shrink) | White with 40% element tint (subtle) | **Yes** — use full status color |

---

## Phase 1: Hit Flash on Target Sprite

**Goal**: Brief white flash on target sprite when taking damage (~150ms). RO Classic plays a HURT animation frame; we enhance with a visual flash.

### Research Findings

- `SpriteCharacterActor` uses `UProceduralMeshComponent` per layer with `UMaterialInstanceDynamic`
- Material is `MSM_Unlit` + `BLEND_Masked` + `TwoSided`, created in `CreateSpriteMaterial()`
- Existing `TintColor` vector parameter (multiplied against texture RGB)
- `SetLayerTint(ESpriteLayer, FLinearColor)` sets TintColor per layer
- Body/weapon layers default to white (1,1,1) — multiplying by white has no visible effect
- Hair layers use colored tint (e.g., brown, blonde) — these DO flash visibly
- Tick() already runs every frame with animation updates

### Approach: TintColor Override to (3,3,3)

Since the material is Unlit and UE5 allows HDR values > 1.0 in material parameters:
- Setting TintColor to (3, 3, 3) multiplies all texture pixels by 3x
- Dark pixels (0.3 leather) become (0.9 near-white) — visually "washed out"
- Light pixels (0.8 skin) become (1.0+ clamped to white) — fully white
- Combined effect: brief "whiteout" flash across all layers

```cpp
// SpriteCharacterActor.h
void PlayHitFlash();

private:
    float HitFlashTimer = 0.0f;
    bool bHitFlashing = false;
    static constexpr float HIT_FLASH_DURATION = 0.15f;  // 150ms
    TMap<int32, FLinearColor> SavedLayerTints;  // Original tints before flash

// SpriteCharacterActor.cpp
void ASpriteCharacterActor::PlayHitFlash()
{
    if (bHitFlashing) return;
    bHitFlashing = true;
    HitFlashTimer = 0.0f;
    SavedLayerTints.Empty();
    
    for (int32 i = 0; i < Layers.Num(); ++i)
    {
        if (Layers[i].MaterialInst)
        {
            SavedLayerTints.Add(i, Layers[i].TintColor);
            Layers[i].MaterialInst->SetVectorParameterValue(
                TEXT("TintColor"), FLinearColor(3.0f, 3.0f, 3.0f, 1.0f));
        }
    }
}

// In Tick():
if (bHitFlashing)
{
    HitFlashTimer += DeltaTime;
    if (HitFlashTimer >= HIT_FLASH_DURATION)
    {
        for (auto& Pair : SavedLayerTints)
        {
            if (Layers.IsValidIndex(Pair.Key) && Layers[Pair.Key].MaterialInst)
            {
                Layers[Pair.Key].MaterialInst->SetVectorParameterValue(
                    TEXT("TintColor"), Pair.Value);
                Layers[Pair.Key].TintColor = Pair.Value;
            }
        }
        bHitFlashing = false;
    }
}
```

### Trigger Location

In `CombatActionSubsystem::HandleCombatDamage()`, after resolving target actor:
```cpp
if (ASpriteCharacterActor* Sprite = Cast<ASpriteCharacterActor>(TargetActor))
{
    Sprite->PlayHitFlash();
}
```

Also call for enemies hit by player (resolve enemy actor from `EnemySubsystem`).

### Files to Modify

| File | Changes |
|------|---------|
| `Sprite/SpriteCharacterActor.h` | Add `PlayHitFlash()`, flash timer, saved tints map |
| `Sprite/SpriteCharacterActor.cpp` | Implement flash + tick update |
| `UI/CombatActionSubsystem.cpp` | Call `PlayHitFlash()` on target |

---

## Phase 2: Hit Impact Particles

**Goal**: Small white spark burst at the target's position on auto-attack hit. RO Classic effect ID 0 (EF_HIT1): 4 white sparkle particles, ~300ms.

### Research Findings

- `SkillVFXSubsystem` has `SpawnNiagaraAtLocation()` helper (auto-destroy, auto-activate, pooled)
- `SetNiagaraColor()` applies per-spawn color override
- Assets pre-loaded at construction via `LoadObject<UNiagaraSystem>()`
- Available assets for hit impact:
  - `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1` — impact burst (best match)
  - `/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash` — slash effect
  - `/Game/NiagaraExamples/FX_Sparks/NS_Spark_Burst` — spark burst

### Implementation

```cpp
// SkillVFXSubsystem.h
UPROPERTY()
UNiagaraSystem* NS_AutoAttackHit = nullptr;

void SpawnAutoAttackHitEffect(FVector Location, bool bIsCritical = false);

// SkillVFXSubsystem.cpp — load in OnWorldBeginPlay
NS_AutoAttackHit = LoadObject<UNiagaraSystem>(nullptr,
    TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));

// Spawn function
void USkillVFXSubsystem::SpawnAutoAttackHitEffect(FVector Location, bool bIsCritical)
{
    if (!NS_AutoAttackHit) return;
    FVector Scale = bIsCritical ? FVector(1.5f) : FVector(0.8f);
    UNiagaraComponent* Comp = SpawnNiagaraAtLocation(
        NS_AutoAttackHit, Location, FRotator::ZeroRotator, Scale);
    if (Comp)
    {
        FLinearColor Color = bIsCritical
            ? FLinearColor(0.9f, 0.9f, 0.15f)   // Yellow for crit
            : FLinearColor(1.0f, 1.0f, 1.0f);   // White for normal
        SetNiagaraColor(Comp, Color);
    }
}
```

Trigger from `CombatActionSubsystem` on `combat:damage` only (not `skill:effect_damage` — skills have their own VFX).

### Files to Modify

| File | Changes |
|------|---------|
| `VFX/SkillVFXSubsystem.h` | Add `NS_AutoAttackHit` UPROPERTY, `SpawnAutoAttackHitEffect()` |
| `VFX/SkillVFXSubsystem.cpp` | Load asset, implement spawn |
| `UI/CombatActionSubsystem.cpp` | Call spawn on combat:damage hit |

---

## Phase 3: Critical Starburst Background

**Goal**: Grey starburst sprite rendered behind yellow crit damage numbers. RO Classic uses `critbg` from `msg.spr`: red/orange spike burst, 60% size, tinted grey (0.66, 0.66, 0.66), static (no animation), fades with the number.

### Research Findings

- `critbg` is sprite frame 3 in RO's `msg.spr` file
- Rendered at 60% of original sprite dimensions
- Offset (0, -6) pixels vertically
- Tinted grey: `[0.66, 0.66, 0.66, 1.0]`
- Same animation as the damage number (moves with it, fades together)
- Does NOT rotate or pulse — purely static

### Implementation

#### Texture Asset

Create `Content/SabriMMO/UI/Textures/T_CritStarburst.png`:
- 128x128 white starburst on transparent background
- 8-12 triangular spikes radiating from center
- Import settings: BC7, NoMipmaps, UI group, NeverStream

#### Code

```cpp
// DamageNumberSubsystem.h
UPROPERTY()
UTexture2D* CritStarburstTexture = nullptr;

// DamageNumberSubsystem.cpp — OnWorldBeginPlay
CritStarburstTexture = LoadObject<UTexture2D>(nullptr,
    TEXT("/Game/SabriMMO/UI/Textures/T_CritStarburst.T_CritStarburst"));

// Pass to overlay
OverlayWidget = SNew(SDamageNumberOverlay).CritTexture(CritStarburstTexture);
```

```cpp
// SDamageNumberOverlay.h — add SLATE_ARGUMENT
SLATE_ARGUMENT(UTexture2D*, CritTexture)

// Members
FSlateBrush CritBrush;

// SDamageNumberOverlay.cpp — Construct
if (InArgs._CritTexture)
{
    CritBrush.SetResourceObject(InArgs._CritTexture);
    CritBrush.ImageSize = FVector2D(128, 128);
    CritBrush.DrawAs = ESlateBrushDrawType::Image;
}

// OnPaint — render BEFORE digits for crit types
if ((Entry.Type == EDamagePopType::CriticalDamage ||
     Entry.Type == EDamagePopType::PlayerCritHit) && CritBrush.GetResourceObject())
{
    float BurstSize = ScaledFontSz * 3.5f;
    FVector2f BurstPos(
        (float)(BasePos.X - BurstSize * 0.5f),
        (float)(BasePos.Y - BurstSize * 0.5f - 4.0f));
    FLinearColor BurstColor(0.66f, 0.66f, 0.66f, Alpha * 0.8f);
    FSlateDrawElement::MakeBox(OutDrawElements, OutLayerId,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(BurstSize, BurstSize), FSlateLayoutTransform(BurstPos)),
        &CritBrush, DrawEffects, BurstColor);
}
```

### Files to Modify

| File | Changes |
|------|---------|
| `UI/SDamageNumberOverlay.h` | Add CritTexture SLATE_ARGUMENT, FSlateBrush member |
| `UI/SDamageNumberOverlay.cpp` | Init brush in Construct, render in OnPaint |
| `UI/DamageNumberSubsystem.h` | Add CritStarburstTexture UPROPERTY |
| `UI/DamageNumberSubsystem.cpp` | LoadObject, pass to SNew |
| **New asset** | `Content/SabriMMO/UI/Textures/T_CritStarburst.png` |

---

## Phase 4: Combo Total Display

**Goal**: After multi-hit skills, show a big yellow total damage number. RO Classic: 3s duration, rapid pop-in (0→3.5x in 50ms), holds at 3.5x, slow rise from high position.

### Research Findings

- Server sends `skill:effect_damage` with `hitNumber` (1-indexed) and `totalHits` per event
- Last hit: `hitNumber === totalHits`
- Bolt skills: `totalHits = skillLevel` (Fire Bolt Lv10 = 10 hits, 200ms stagger)
- Holy Cross: 2 hits, 150ms stagger
- `DamageNumberSubsystem` already parses both events but doesn't read hitNumber/totalHits
- `SkillVFXSubsystem` already reads hitNumber/totalHits (line 327)

### Implementation

#### New enum value + constant
```cpp
// SDamageNumberOverlay.h
ComboTotal  // Add to EDamagePopType enum

static constexpr float LIFETIME_COMBO = 3.0f;
```

#### Combo tracking in DamageNumberSubsystem
```cpp
// DamageNumberSubsystem.h
struct FComboTracker
{
    int32 TotalDamage = 0;
    int32 ExpectedHits = 0;
    int32 HitsReceived = 0;
    int32 TargetId = 0;
    bool bIsEnemy = false;
    FVector LastTargetPos = FVector::ZeroVector;
    double FirstHitTime = 0.0;
};

TMap<FString, FComboTracker> ActiveCombos;  // Key: "attackerId_targetId_skillId"

void SpawnComboTotal(int32 TotalDamage, bool bIsEnemy, int32 TargetId, const FVector& Pos);
```

#### Track in HandleCombatDamage
```cpp
// Parse hitNumber/totalHits from skill:effect_damage events
double HitNumberD = 0, TotalHitsD = 0, SkillIdD = 0;
Obj->TryGetNumberField(TEXT("hitNumber"), HitNumberD);
Obj->TryGetNumberField(TEXT("totalHits"), TotalHitsD);
Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
int32 HitNumber = (int32)HitNumberD;
int32 TotalHits = (int32)TotalHitsD;
int32 SkillId = (int32)SkillIdD;

if (TotalHits > 1 && SkillId > 0)
{
    FString Key = FString::Printf(TEXT("%d_%d_%d"), AttackerId, TargetId, SkillId);
    FComboTracker& C = ActiveCombos.FindOrAdd(Key);
    if (HitNumber == 1) { C = FComboTracker(); C.ExpectedHits = TotalHits; C.FirstHitTime = FPlatformTime::Seconds(); }
    C.TotalDamage += DisplayValue;
    C.HitsReceived++;
    C.TargetId = TargetId;
    C.bIsEnemy = bIsEnemy;
    C.LastTargetPos = TargetWorldPos;
    
    if (HitNumber >= TotalHits)
    {
        SpawnComboTotal(C.TotalDamage, C.bIsEnemy, C.TargetId, C.LastTargetPos);
        ActiveCombos.Remove(Key);
    }
}
```

#### Combo total animation in OnPaint
```cpp
else if (Entry.Type == EDamagePopType::ComboTotal)
{
    // Rapid pop-in: 0→3.5x in ~50ms, then hold
    float PopT = FMath::Clamp(t / 0.017f, 0.0f, 1.0f);  // 0.017 = 50ms/3000ms
    Scale = PopT * 3.5f;
    OffsetY = -(50.0f + 20.0f * t);  // High start, slow rise
    OffsetX = 0.0f;
    Alpha = FMath::Max(0.0f, 1.0f - t * 0.8f);  // Slower fade
}
```

Color: CritYellow `(0.9, 0.9, 0.15)`. Font: CRIT_FONT_SIZE (28pt base).

### Files to Modify

| File | Changes |
|------|---------|
| `UI/SDamageNumberOverlay.h` | Add `ComboTotal` enum, `LIFETIME_COMBO` |
| `UI/SDamageNumberOverlay.cpp` | Color, font, animation path, lifetime |
| `UI/DamageNumberSubsystem.h` | Add `FComboTracker`, `ActiveCombos`, `SpawnComboTotal()` |
| `UI/DamageNumberSubsystem.cpp` | Track hits, spawn total on last hit |

---

## Phase 5: Target Flinch / Recoil

**Goal**: Target plays brief recoil animation when hit. RO Classic: HURT animation frame plays for `dMotion` ms (~288-576ms), no position change, suppressed by Endure.

### Research Findings

- `SpriteCharacterActor` has animation states including `Hit` (one-shot, auto-reverts to Idle)
- Monster dMotion values: Poring 480ms, Zombie 288ms, Wolf 432ms, Orc 288ms
- Player dMotion: ~230-480ms depending on job class
- Endure buff: suppresses flinch entirely, damage type changes to `DMG_ENDURE`
- Target does NOT move position — only the animation plays

### Implementation

```cpp
// In CombatActionSubsystem::HandleCombatDamage(), after resolving target:
if (HitType != TEXT("miss") && HitType != TEXT("dodge") && HitType != TEXT("perfectDodge"))
{
    if (ASpriteCharacterActor* Sprite = Cast<ASpriteCharacterActor>(TargetActor))
    {
        Sprite->PlayHitAnimation();  // Existing one-shot Hit state
    }
}
```

If the Hit animation doesn't exist in all atlases, `PlayHitFlash()` (Phase 1) provides the visual feedback fallback.

Future: When the server sends `damageType: "endure"`, skip flinch:
```cpp
FString DamageType;
Obj->TryGetStringField(TEXT("damageType"), DamageType);
if (DamageType != TEXT("endure")) { /* play flinch */ }
```

### Files to Modify

| File | Changes |
|------|---------|
| `Sprite/SpriteCharacterActor.h` | Add `PlayHitAnimation()` if missing |
| `Sprite/SpriteCharacterActor.cpp` | Implement (trigger Hit anim state) |
| `UI/CombatActionSubsystem.cpp` | Call on target after damage |

---

## Phase 6: Hit Sounds

**Goal**: Positional audio at target location on auto-attack hit. RO Classic: 3-5 variants per weapon type, random selection, +/-5% pitch variation. This is the FIRST audio system in the project.

### Research Findings

- No audio playback code exists in the project (no `PlaySoundAtLocation`, no `USoundBase` refs)
- RO Classic weapon hit sounds: short percussive (~80-150ms), weapon-type-specific
- Critical hits: higher-pitched, more dramatic "crunch" variant
- RO cycles through 3-5 variants to prevent repetition
- +/-5% pitch shift per playback for organic feel
- Monster type does NOT affect hit sound (same sound regardless of target)

### Implementation

#### Asset Organization
```
Content/SabriMMO/Audio/HitSounds/
  Normal/Hit_Normal_01.wav, _02.wav, _03.wav
  Critical/Hit_Crit_01.wav, _02.wav
```

Start with 5 wav files total. Weapon-specific sets can be added later via `TMap<FString, TArray<USoundBase*>>`.

#### Code
```cpp
// CombatActionSubsystem.h
UPROPERTY() TArray<USoundBase*> NormalHitSounds;
UPROPERTY() USoundBase* CritHitSound = nullptr;

// CombatActionSubsystem.cpp — OnWorldBeginPlay
NormalHitSounds.Add(LoadObject<USoundBase>(nullptr, TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_01")));
// ... load all variants

// In HandleCombatDamage():
if (HitType != TEXT("miss") && Damage > 0)
{
    USoundBase* Sound = bIsCritical && CritHitSound
        ? CritHitSound
        : NormalHitSounds[FMath::RandRange(0, NormalHitSounds.Num() - 1)];
    if (Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, TargetPos,
            1.0f, FMath::FRandRange(0.95f, 1.05f));
    }
}
```

#### Sound Asset Sources

Free RPG hit sounds:
- freesound.org ("sword hit", "melee impact")
- opengameart.org ("combat sounds")
- kenney.nl/assets (game audio packs)

Import: PCM quality, no compression for short sounds.

### Files to Modify

| File | Changes |
|------|---------|
| `UI/CombatActionSubsystem.h` | Add sound UPROPERTYs |
| `UI/CombatActionSubsystem.cpp` | Load sounds, play on hit |
| **New assets** | 5 wav files in `Content/SabriMMO/Audio/HitSounds/` |

---

## File Change Matrix

| File | Ph1 | Ph2 | Ph3 | Ph4 | Ph5 | Ph6 | Status Fix |
|------|-----|-----|-----|-----|-----|-----|------------|
| `Sprite/SpriteCharacterActor.h` | X | | | | X | | |
| `Sprite/SpriteCharacterActor.cpp` | X | | | | X | | |
| `VFX/SkillVFXSubsystem.h` | | X | | | | | |
| `VFX/SkillVFXSubsystem.cpp` | | X | | | | | |
| `UI/SDamageNumberOverlay.h` | | | X | X | | | |
| `UI/SDamageNumberOverlay.cpp` | | | X | X | | | |
| `UI/DamageNumberSubsystem.h` | | | X | X | | | X |
| `UI/DamageNumberSubsystem.cpp` | | | X | X | | | X |
| `UI/CombatActionSubsystem.h` | | | | | | X | |
| `UI/CombatActionSubsystem.cpp` | X | X | | | X | X | |

### New Assets Required

| Asset | Phase | Notes |
|-------|-------|-------|
| `T_CritStarburst.png` | 3 | 128x128 white starburst, transparent bg |
| `Hit_Normal_01/02/03.wav` | 6 | Short percussive hit sounds |
| `Hit_Crit_01/02.wav` | 6 | Higher-pitched crit hit sounds |

---

## Recommended Implementation Order

1. **Status Fix** — Quickest win, improves existing poison/bleeding drain visibility
2. **Phase 3 (Crit Starburst)** — Pure Slate, no dependencies, extends OnPaint
3. **Phase 4 (Combo Total)** — Extends DamageNumberSubsystem, moderate effort
4. **Phase 1 (Hit Flash)** — Material parameter work, needs testing
5. **Phase 2 (Hit Particles)** — Uses existing VFX infrastructure
6. **Phase 5 (Flinch)** — Needs Hit animation verified in atlases
7. **Phase 6 (Hit Sounds)** — Needs wav assets sourced first

---

## Research Sources

### roBrowser / roBrowserLegacy (Client Source)
- `Damage.js` — damage number animation, colors, critbg rendering
- `EffectConst.js` — EF_HIT1-EF_HIT6 effect IDs and descriptions
- `EffectTable.js` — hit effect parameters (particle count, textures, durations)
- `EffectManager.js` — effect spawning patterns

### rAthena (Server Source)
- `clif.hpp` — `e_damage_type` enum (DMG_NORMAL, DMG_ENDURE, DMG_CRITICAL, etc.)
- `mob_db.txt` — dMotion values per monster
- `battle.hpp` — damage type definitions

### Project Files (Existing Infrastructure)
- `SpriteCharacterActor.h/.cpp` — ProceduralMeshComponent, TintColor parameter, SetLayerTint()
- `SkillVFXSubsystem.h/.cpp` — SpawnNiagaraAtLocation(), SetNiagaraColor(), asset loading
- `CombatActionSubsystem.h/.cpp` — 9 combat events, attack animation, actor resolution
- `DamageNumberSubsystem.h/.cpp` — 5 events, damage type determination, world-to-screen
