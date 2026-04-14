# Prompt: Implement Hit Impact & Remaining Damage Number Features

## Context

Load these skills: `/sabrimmo-damage-numbers`, `/sabrimmo-combat`, `/sabrimmo-ui`, `/sabrimmo-sprites`

Research: `docsNew/05_Development/Damage_Number_RO_Classic_Research.md`
Plan: `docsNew/05_Development/Damage_Number_Hit_Impact_Implementation_Plan.md`

## Goal

Implement the remaining 6 features to complete the RO Classic combat feel, plus fix status effect damage coloring. Read the full implementation plan before starting.

## Pre-Implementation: Read These Files First

```
client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.h
client/SabriMMO/Source/SabriMMO/UI/SDamageNumberOverlay.cpp
client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.h
client/SabriMMO/Source/SabriMMO/UI/DamageNumberSubsystem.cpp
client/SabriMMO/Source/SabriMMO/UI/CombatActionSubsystem.h
client/SabriMMO/Source/SabriMMO/UI/CombatActionSubsystem.cpp
client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.h
client/SabriMMO/Source/SabriMMO/Sprite/SpriteCharacterActor.cpp
client/SabriMMO/Source/SabriMMO/VFX/SkillVFXSubsystem.h
client/SabriMMO/Source/SabriMMO/VFX/SkillVFXSubsystem.cpp
```

## Implementation Order

### Step 0: Fix Status Effect Damage Colors

**Problem**: Status tick damage (poison/bleeding/stone) currently uses element tinting at 40% blend on a white base — too subtle.

**Fix**: Add optional `CustomColor` parameter to `SpawnDamagePop()` and `AddDamagePop()`.

In `SDamageNumberOverlay.h`, update `AddDamagePop` signature:
```cpp
void AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition,
    const FString& Element = TEXT(""), const FLinearColor* CustomColor = nullptr);
```

In `SDamageNumberOverlay.cpp`, `AddDamagePop()`, after setting existing fields and before the lifetime block:
```cpp
if (CustomColor)
{
    Entry.CustomColor = *CustomColor;
    Entry.bHasCustomColor = true;
}
```

In `DamageNumberSubsystem.h`, update `SpawnDamagePop` signature:
```cpp
void SpawnDamagePop(int32 Damage, bool bIsCritical, bool bIsEnemy,
    int32 AttackerId, int32 TargetId, const FVector& TargetWorldPos,
    const FString& HitType = TEXT("normal"), const FString& Element = TEXT("neutral"),
    const FLinearColor* CustomColor = nullptr);
```

Pass through to `AddDamagePop` in `SpawnDamagePop()`:
```cpp
OverlayWidget->AddDamagePop(Damage, PopType, ScreenPos, Element, CustomColor);
```

In `HandleStatusTick()`, use full status colors instead of element tinting:
```cpp
FLinearColor StatusColor = GetStatusColor(StatusType);
SpawnDamagePop(Drain, false, bIsEnemy, 0, TargetId, TargetPos,
    TEXT("normal"), TEXT("neutral"), &StatusColor);
```

**Result**: Poison drain = **purple** (0.7, 0.3, 0.8). Bleeding = **red** (1.0, 0.2, 0.2). Stone = **brown** (0.7, 0.55, 0.3). Matches the "Poisoned!" text colors.

**Verify**: Status text ("Poisoned!") already uses the miss animation path (straight rise, fixed 0.7x scale, 0.8s) because `!Entry.TextLabel.IsEmpty()` → `bIsMissType = true`. No change needed for status text.

Build and test after this step before proceeding.

---

### Step 1: Hit Flash on Target Sprite (Phase 1)

**Files**: `SpriteCharacterActor.h/.cpp`, `CombatActionSubsystem.cpp`

Add to `SpriteCharacterActor.h`:
```cpp
public:
    void PlayHitFlash();

private:
    float HitFlashTimer = 0.0f;
    bool bHitFlashing = false;
    static constexpr float HIT_FLASH_DURATION = 0.15f;
    TMap<int32, FLinearColor> SavedLayerTints;
```

Implement in `SpriteCharacterActor.cpp`:
- `PlayHitFlash()`: Save all layer TintColors, set all to `FLinearColor(3.0f, 3.0f, 3.0f, 1.0f)`
- In `Tick()`: accumulate timer, restore tints when `HIT_FLASH_DURATION` reached

Trigger in `CombatActionSubsystem::HandleCombatDamage()`:
- After resolving target actor, cast to `ASpriteCharacterActor`, call `PlayHitFlash()`
- For enemy targets: resolve via `UEnemySubsystem::GetEnemyData()` → `Actor`
- Skip for miss/dodge/perfectDodge hit types

Build and test.

---

### Step 2: Hit Impact Particles (Phase 2)

**Files**: `SkillVFXSubsystem.h/.cpp`, `CombatActionSubsystem.cpp`

Add to `SkillVFXSubsystem.h`:
```cpp
UPROPERTY()
UNiagaraSystem* NS_AutoAttackHit = nullptr;

void SpawnAutoAttackHitEffect(FVector Location, bool bIsCritical = false);
```

In `OnWorldBeginPlay()`: load `NS_Free_Magic_Hit1`:
```cpp
NS_AutoAttackHit = LoadObject<UNiagaraSystem>(nullptr,
    TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));
```

Implement `SpawnAutoAttackHitEffect()`:
- Scale: 0.8 normal, 1.5 crit
- Color: white normal, yellow (0.9, 0.9, 0.15) crit
- Use existing `SpawnNiagaraAtLocation()` + `SetNiagaraColor()`

Trigger in `CombatActionSubsystem::HandleCombatDamage()`:
- ONLY for `combat:damage` events (auto-attacks), NOT `skill:effect_damage`
- After resolving target position, call `SpawnAutoAttackHitEffect(TargetPos, bIsCritical)`
- Skip for miss/dodge

Build and test.

---

### Step 3: Critical Starburst (Phase 3)

**Files**: `SDamageNumberOverlay.h/.cpp`, `DamageNumberSubsystem.h/.cpp`
**New asset**: Create `Content/SabriMMO/UI/Textures/T_CritStarburst.png` (128x128 white starburst, transparent bg, 8-12 spikes)

In `DamageNumberSubsystem.h`: add `UPROPERTY() UTexture2D* CritStarburstTexture`
In `DamageNumberSubsystem.cpp`: LoadObject in OnWorldBeginPlay, pass to SNew

In `SDamageNumberOverlay.h`: add `SLATE_ARGUMENT(UTexture2D*, CritTexture)`, add `FSlateBrush CritBrush` member
In `SDamageNumberOverlay.cpp` Construct: init brush with `SetResourceObject()`
In OnPaint: for CriticalDamage/PlayerCritHit types, render `FSlateDrawElement::MakeBox` with `CritBrush` on `OutLayerId` (below text on `TextLayerId`):
- Size: `ScaledFontSz * 3.5f`
- Position: centered on BasePos, offset -4px up
- Color: `(0.66, 0.66, 0.66, Alpha * 0.8)` — grey, fades with number
- Render BEFORE the digit loop

**IMPORTANT**: The starburst must use `FSlateBrush` (NOT `FSlateDynamicImageBrush` which is deprecated in UE5.7). Keep the source `UTexture2D*` alive via UPROPERTY on the subsystem.

Build and test.

---

### Step 4: Combo Total Display (Phase 4)

**Files**: `SDamageNumberOverlay.h/.cpp`, `DamageNumberSubsystem.h/.cpp`

Add `ComboTotal` to `EDamagePopType` enum.
Add `LIFETIME_COMBO = 3.0f` constant.

In `DamageNumberSubsystem.h`:
```cpp
struct FComboTracker {
    int32 TotalDamage = 0;
    int32 ExpectedHits = 0;
    int32 HitsReceived = 0;
    int32 TargetId = 0;
    bool bIsEnemy = false;
    FVector LastTargetPos = FVector::ZeroVector;
};
TMap<FString, FComboTracker> ActiveCombos;
void SpawnComboTotal(int32 TotalDamage, bool bIsEnemy, int32 TargetId, const FVector& Pos);
```

In `HandleCombatDamage()`: parse `hitNumber`, `totalHits`, `skillId`. If `totalHits > 1`, buffer damage per `"{attackerId}_{targetId}_{skillId}"` key. On `hitNumber >= totalHits`, call `SpawnComboTotal()` then remove tracker.

`SpawnComboTotal()` calls `SpawnDamagePop()` with a new hitType like `"combo_total"`, which maps to `EDamagePopType::ComboTotal`.

In OnPaint, ComboTotal animation:
```cpp
// Rapid pop-in: 0→3.5x in 50ms
float PopT = FMath::Clamp(t / 0.017f, 0.0f, 1.0f);
Scale = PopT * 3.5f;
OffsetY = -(50.0f + 20.0f * t);  // High, slow rise
OffsetX = 0.0f;
Alpha = FMath::Max(0.0f, 1.0f - t * 0.8f);
```

In `AddDamagePop()` lifetime switch: ComboTotal → `LIFETIME_COMBO`, DriftDirection 0.
In `GetFillColor()`: ComboTotal → `CritYellow` (0.9, 0.9, 0.15).
In `GetFontSize()`: ComboTotal → `CRIT_FONT_SIZE` (28).

Build and test with bolt skills (Fire Bolt Lv5+ = 5+ hits → yellow total).

---

### Step 5: Target Flinch (Phase 5)

**Files**: `SpriteCharacterActor.h/.cpp`, `CombatActionSubsystem.cpp`

Check if SpriteCharacterActor already has a `Hit` or `Hurt` animation state in its `ESpriteAnimState` enum. If so, add a `PlayHitAnimation()` function that calls `SetAnimationState(ESpriteAnimState::Hit)`.

If no Hit animation exists in the atlas system, skip this step — the hit flash (Phase 1) provides sufficient visual feedback.

Trigger in `CombatActionSubsystem::HandleCombatDamage()`:
```cpp
if (HitType != "miss" && HitType != "dodge" && HitType != "perfectDodge")
    Sprite->PlayHitAnimation();
```

Future Endure integration: when server sends `damageType: "endure"`, skip flinch.

Build and test.

---

### Step 6: Hit Sounds (Phase 6)

**Files**: `CombatActionSubsystem.h/.cpp`
**New assets**: 5 wav files in `Content/SabriMMO/Audio/HitSounds/`

This is the FIRST audio in the project. Source 3 normal hit sounds + 2 crit hit sounds (short percussive impacts, 80-150ms duration). Import to UE5 as SoundWave assets.

In `CombatActionSubsystem.h`:
```cpp
UPROPERTY() TArray<USoundBase*> NormalHitSounds;
UPROPERTY() USoundBase* CritHitSound = nullptr;
```

Load in `OnWorldBeginPlay()` via `LoadObject<USoundBase>()`.

Play in `HandleCombatDamage()`:
- Only for non-miss hits with damage > 0
- Random variant selection for normal hits
- +/-5% pitch variation: `FMath::FRandRange(0.95f, 1.05f)`
- Positional: `UGameplayStatics::PlaySoundAtLocation(World, Sound, TargetPos, 1.0f, Pitch)`

Build and test.

---

## What NOT to Change

- Keep element tinting for normal combat damage (white + 40% element blend)
- Keep status text using miss animation path (already works correctly)
- Don't add screen shake on crits (RO Classic doesn't have it)
- Don't implement bitmap font (TTF Bold + outline looks good enough)
- Don't modify the sine arc / drift / scale system from the base implementation
- Keep the 64-entry circular pool architecture
- Keep DPI-aware scaling
- Keep dual-wield damage number separation

## Build Command

```bash
"C:/UE_5.7/Engine/Build/BatchFiles/Build.bat" SabriMMOEditor Win64 Development "C:/Sabri_MMO/client/SabriMMO/SabriMMO.uproject" -waitmutex
```

Close editor before building (header changes in Phases 1, 3, 4, 5, 6).
