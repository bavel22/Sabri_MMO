# Implementation Plan: Damage Number & Hit Impact Remaining Features

## Context

Load these skills: `/sabrimmo-damage-numbers`, `/sabrimmo-combat`, `/sabrimmo-ui`, `/sabrimmo-sprites`
Research: `docsNew/05_Development/Damage_Number_RO_Classic_Research.md`

## Overview

Six features remain to complete the RO Classic combat feel. Ordered by implementation dependency:

```
Phase 1: Hit Flash (sprite material)           — standalone, no dependencies
Phase 2: Hit Impact Particles (Niagara)         — standalone, no dependencies  
Phase 3: Critical Starburst (Slate OnPaint)     — standalone, no dependencies
Phase 4: Combo Total Display (DamageNumber)     — extends existing system
Phase 5: Target Flinch/Recoil (sprite actor)    — depends on Phase 1 material work
Phase 6: Hit Sounds (audio)                     — standalone, first audio in project
```

Phases 1-3 are independent and can be implemented in any order or in parallel.

---

## Phase 1: Hit Flash on Target Sprite

**Goal**: When a target takes damage, briefly flash the sprite white for ~150ms.

### Architecture

SpriteCharacterActor already has:
- `UMaterialInstanceDynamic` per layer (body, hair, weapon, etc.) created in `CreateSpriteMaterial()`
- `SetLayerTint(ESpriteLayer, FLinearColor)` that calls `SetVectorParameterValue("TintColor", Color)`
- `TintColor` parameter multiplies against texture RGB in the material
- A `Tick()` function and frame-based animation system

**Problem**: TintColor is multiplicative — `texture * tint`. For layers already tinted white (body, weapon), setting white has no visible effect. We need an **additive** flash parameter.

### Implementation

#### 1a. Add "HitFlash" scalar parameter to sprite material

In `SpriteCharacterActor.cpp`, `CreateSpriteMaterial()` creates the material procedurally. Add a new scalar parameter that controls a lerp-to-white:

```cpp
// In CreateSpriteMaterial() — after existing TintColor multiply node:

// Add HitFlash parameter: 0.0 = normal, 1.0 = pure white
UMaterialExpressionScalarParameter* FlashParam = NewObject<UMaterialExpressionScalarParameter>(Material);
FlashParam->ParameterName = TEXT("HitFlash");
FlashParam->DefaultValue = 0.0f;
Material->GetExpressionCollection().AddExpression(FlashParam);

// Lerp between tinted texture and white based on HitFlash
UMaterialExpressionLinearInterpolate* FlashLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
FlashLerp->A.Connect(/* existing tinted texture output */);
FlashLerp->ConstB = FVector4(1.0f, 1.0f, 1.0f, 1.0f);  // White
FlashLerp->Alpha.Connect(FlashParam);
// Connect FlashLerp output to material's BaseColor
```

**Alternative (simpler, if material expression graphs are complex)**:
Instead of modifying the material graph, use a dedicated overlay approach:
- Add a separate `UProceduralMeshComponent` per sprite that renders a white quad on top
- Set its opacity to 0 normally, flash to 0.7 on hit, fade back to 0

**Simplest viable approach**: Since the material is created programmatically and may be complex to modify, use the **emissive channel** instead:

```cpp
// The material is MSM_Unlit, so EmissiveColor IS the output color.
// We can add an emissive additive term controlled by HitFlash.
// At HitFlash=0: normal texture output
// At HitFlash=1: texture + white additive = washed out white
```

Actually, the **simplest approach** that works with the existing material:

```cpp
// SpriteCharacterActor.h — add flash state
float HitFlashTimer = 0.0f;
bool bHitFlashing = false;
static constexpr float HIT_FLASH_DURATION = 0.15f;  // 150ms
TMap<int32, FLinearColor> OriginalLayerTints;  // Saved before flash

// SpriteCharacterActor.h — public API
void PlayHitFlash();
```

```cpp
// SpriteCharacterActor.cpp — PlayHitFlash()
void ASpriteCharacterActor::PlayHitFlash()
{
    if (bHitFlashing) return;  // Already flashing
    bHitFlashing = true;
    HitFlashTimer = 0.0f;
    
    // Save original tints and set all layers to bright white
    OriginalLayerTints.Empty();
    for (int32 i = 0; i < Layers.Num(); ++i)
    {
        if (Layers[i].MaterialInst)
        {
            OriginalLayerTints.Add(i, Layers[i].TintColor);
            // Boost to bright white — even colored layers become bright
            Layers[i].MaterialInst->SetVectorParameterValue(
                TEXT("TintColor"), FLinearColor(3.0f, 3.0f, 3.0f, 1.0f));
        }
    }
}

// In Tick() — update flash
if (bHitFlashing)
{
    HitFlashTimer += DeltaTime;
    if (HitFlashTimer >= HIT_FLASH_DURATION)
    {
        // Restore original tints
        for (auto& Pair : OriginalLayerTints)
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

**Why (3,3,3) works**: The material is `MSM_Unlit` so the emissive color IS the final output. With HDR rendering, values > 1.0 are allowed and produce a bloom-like brightness. Even without bloom, dark texture pixels (0.3 brown leather) become (0.9 near-white) when multiplied by 3. The visual result is a brief "whiteout" flash.

#### 1b. Trigger flash from combat events

In `CombatActionSubsystem::HandleCombatDamage()` or `DamageNumberSubsystem::HandleCombatDamage()`:

```cpp
// After determining the target actor:
if (ASpriteCharacterActor* SpriteActor = Cast<ASpriteCharacterActor>(TargetActor))
{
    SpriteActor->PlayHitFlash();
}
```

**Best location**: `CombatActionSubsystem` since it already resolves attacker/target actors for animation. Add the flash call right after `PlayAttackAnimationOnActor()`.

#### Files to modify
| File | Changes |
|------|---------|
| `Sprite/SpriteCharacterActor.h` | Add `PlayHitFlash()`, flash timer, saved tints |
| `Sprite/SpriteCharacterActor.cpp` | Implement flash + tick update |
| `UI/CombatActionSubsystem.cpp` | Call `PlayHitFlash()` on target actor |

---

## Phase 2: Hit Impact Particles

**Goal**: Spawn a brief white sparkle burst at the target position on auto-attack hit. RO Classic uses effect ID 0 (EF_HIT1): 4 white particles scattering outward, ~300ms.

### Architecture

SkillVFXSubsystem already has:
- `SpawnNiagaraAtLocation(UNiagaraSystem*, FVector, FRotator, FVector)` — auto-destroy, auto-activate
- `SetNiagaraColor(UNiagaraComponent*, FLinearColor)` — per-spawn color override
- Pre-loaded Niagara assets at construction time
- Available particle assets: `NS_Free_Magic_Hit1`, `NS_Spark_Burst`, `NS_Free_Magic_Slash`

### Implementation

#### 2a. Add hit impact Niagara asset reference to SkillVFXSubsystem

```cpp
// SkillVFXSubsystem.h — add UPROPERTY
UPROPERTY()
UNiagaraSystem* NS_AutoAttackHit = nullptr;

// SkillVFXSubsystem.cpp — load in OnWorldBeginPlay
NS_AutoAttackHit = LoadObject<UNiagaraSystem>(nullptr,
    TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));
```

#### 2b. Add public spawn function

```cpp
// SkillVFXSubsystem.h
void SpawnAutoAttackHitEffect(FVector Location, bool bIsCritical = false);

// SkillVFXSubsystem.cpp
void USkillVFXSubsystem::SpawnAutoAttackHitEffect(FVector Location, bool bIsCritical)
{
    if (!NS_AutoAttackHit) return;
    UWorld* World = GetWorld();
    if (!World) return;
    
    // Scale up slightly for crits
    FVector Scale = bIsCritical ? FVector(1.5f) : FVector(0.8f);
    UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_AutoAttackHit, Location, FRotator::ZeroRotator, Scale);
    if (Comp)
    {
        // White for normal, yellow-gold for crit (matches RO)
        FLinearColor Color = bIsCritical 
            ? FLinearColor(0.9f, 0.9f, 0.15f)   // Yellow (crit)
            : FLinearColor(1.0f, 1.0f, 1.0f);   // White (normal)
        SetNiagaraColor(Comp, Color);
    }
}
```

#### 2c. Trigger from CombatActionSubsystem

```cpp
// In HandleCombatDamage(), after resolving target position:
if (USkillVFXSubsystem* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
{
    VFX->SpawnAutoAttackHitEffect(TargetPos, bIsCritical);
}
```

Only trigger for `combat:damage` (auto-attacks), NOT for `skill:effect_damage` (skills have their own VFX).

#### Files to modify
| File | Changes |
|------|---------|
| `VFX/SkillVFXSubsystem.h` | Add `NS_AutoAttackHit` UPROPERTY, `SpawnAutoAttackHitEffect()` |
| `VFX/SkillVFXSubsystem.cpp` | Load asset, implement spawn function |
| `UI/CombatActionSubsystem.cpp` | Call spawn on `combat:damage` hit |

---

## Phase 3: Critical Starburst Background

**Goal**: Render a grey starburst sprite behind critical hit damage numbers. RO Classic uses a red/orange spike burst at 60% size, tinted grey (0.66, 0.66, 0.66).

### Architecture

Two approaches:
1. **Texture-based**: Import a starburst texture, render via `FSlateDrawElement::MakeBox` in OnPaint
2. **Procedural**: Draw radial lines using `FSlateDrawElement::MakeLines` in OnPaint

Approach 1 is faithful to RO (which uses a bitmap sprite). Approach 2 avoids needing a texture asset.

### Implementation (Texture-Based, recommended)

#### 3a. Create starburst texture

Create a 128x128 white starburst sprite with transparent background:
- 8-12 radial spikes emanating from center
- Spikes are triangular (wide at base, pointed at tip)
- White on transparent (color tinting applied at render time)
- Import to: `Content/SabriMMO/UI/Textures/T_CritStarburst.png`
- UE5 import settings: BC7, NoMipmaps, UI texture group, NeverStream

Can be generated via Python/PIL script or created in any image editor. The RO original is ~32x32 pixels upscaled.

#### 3b. Load texture in DamageNumberSubsystem

```cpp
// DamageNumberSubsystem.h — add UPROPERTY
UPROPERTY()
UTexture2D* CritStarburstTexture = nullptr;

// DamageNumberSubsystem.cpp — in OnWorldBeginPlay
CritStarburstTexture = LoadObject<UTexture2D>(nullptr,
    TEXT("/Game/SabriMMO/UI/Textures/T_CritStarburst.T_CritStarburst"));
```

Pass to overlay widget:
```cpp
OverlayWidget = SNew(SDamageNumberOverlay)
    .CritStarburstTexture(CritStarburstTexture);
```

#### 3c. Add starburst rendering to SDamageNumberOverlay

```cpp
// SDamageNumberOverlay.h — add members
SLATE_BEGIN_ARGS(SDamageNumberOverlay)
    : _CritStarburstTexture(nullptr)
    {}
    SLATE_ARGUMENT(UTexture2D*, CritStarburstTexture)
SLATE_END_ARGS()

FSlateBrush CritStarburstBrush;

// SDamageNumberOverlay.cpp — in Construct
if (InArgs._CritStarburstTexture)
{
    CritStarburstBrush.SetResourceObject(InArgs._CritStarburstTexture);
    CritStarburstBrush.ImageSize = FVector2D(128, 128);
    CritStarburstBrush.DrawAs = ESlateBrushDrawType::Image;
}
```

#### 3d. Render starburst behind crit numbers in OnPaint

```cpp
// In OnPaint, BEFORE rendering digits for crit types:
const bool bIsCrit = (Entry.Type == EDamagePopType::CriticalDamage || 
                      Entry.Type == EDamagePopType::PlayerCritHit);
if (bIsCrit && CritStarburstBrush.GetResourceObject())
{
    // Size at 60% of the scaled digit group bounding box (RO Classic spec)
    const float BurstSize = ScaledFontSz * 3.5f;  // Approximate digit group extent
    const FVector2f BurstPos(
        (float)(BasePos.X - BurstSize * 0.5f),
        (float)(BasePos.Y - BurstSize * 0.5f - 4.0f)  // Slight upward offset
    );
    
    // Grey tint matching RO Classic (0.66, 0.66, 0.66) with current alpha
    FLinearColor BurstColor(0.66f, 0.66f, 0.66f, Alpha * 0.8f);
    
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        OutLayerId,  // Below text layer
        AllottedGeometry.ToPaintGeometry(
            FVector2D(BurstSize, BurstSize),
            FSlateLayoutTransform(BurstPos)
        ),
        &CritStarburstBrush,
        DrawEffects,
        BurstColor
    );
}
// Then draw digits on TextLayerId (above starburst)
```

#### Files to modify
| File | Changes |
|------|---------|
| `UI/SDamageNumberOverlay.h` | Add `CritStarburstTexture` SLATE_ARGUMENT, FSlateBrush member |
| `UI/SDamageNumberOverlay.cpp` | Load brush in Construct, render in OnPaint before digits |
| `UI/DamageNumberSubsystem.h` | Add `CritStarburstTexture` UPROPERTY |
| `UI/DamageNumberSubsystem.cpp` | LoadObject texture, pass to SNew() |

---

## Phase 4: Combo Total Display

**Goal**: After multi-hit skills (bolts, Double Attack, Triple Attack), show a big yellow total damage number that stays visible for 3 seconds.

### Architecture

Server sends `skill:effect_damage` events with `hitNumber` (1-indexed) and `totalHits` fields. Each hit arrives with a 200ms stagger. The last hit has `hitNumber === totalHits`.

**Approach**: Buffer per-skill damage in DamageNumberSubsystem. On the last hit, spawn a combo total pop with a new `EDamagePopType::ComboTotal` type.

### Implementation

#### 4a. Add combo tracking to DamageNumberSubsystem

```cpp
// DamageNumberSubsystem.h — combo tracking
struct FComboTracker
{
    int32 TotalDamage = 0;
    int32 TotalHits = 0;
    int32 HitsReceived = 0;
    FVector LastTargetPos = FVector::ZeroVector;
    int32 TargetId = 0;
    bool bIsEnemy = false;
    double FirstHitTime = 0.0;
};

// Key: "{attackerId}_{targetId}_{skillId}"
TMap<FString, FComboTracker> ActiveCombos;
```

#### 4b. Track hits in HandleCombatDamage

```cpp
// In HandleCombatDamage(), after extracting fields:
double HitNumberD = 0, TotalHitsD = 0;
int32 SkillIdI = 0;
Obj->TryGetNumberField(TEXT("hitNumber"), HitNumberD);
Obj->TryGetNumberField(TEXT("totalHits"), TotalHitsD);
double SkillIdD = 0;
Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
SkillIdI = (int32)SkillIdD;

const int32 HitNumber = (int32)HitNumberD;
const int32 TotalHits = (int32)TotalHitsD;

// Only track multi-hit skills (totalHits > 1)
if (TotalHits > 1 && SkillIdI > 0)
{
    FString ComboKey = FString::Printf(TEXT("%d_%d_%d"), AttackerId, TargetId, SkillIdI);
    
    FComboTracker& Combo = ActiveCombos.FindOrAdd(ComboKey);
    if (HitNumber == 1)
    {
        // First hit — reset tracker
        Combo.TotalDamage = 0;
        Combo.TotalHits = TotalHits;
        Combo.HitsReceived = 0;
        Combo.TargetId = TargetId;
        Combo.bIsEnemy = bIsEnemy;
        Combo.FirstHitTime = FPlatformTime::Seconds();
    }
    
    Combo.TotalDamage += DisplayValue;
    Combo.HitsReceived++;
    Combo.LastTargetPos = TargetWorldPos;
    
    // Last hit — spawn combo total
    if (HitNumber >= TotalHits)
    {
        // Spawn big yellow combo total
        SpawnComboTotal(Combo.TotalDamage, Combo.bIsEnemy, Combo.TargetId, Combo.LastTargetPos);
        ActiveCombos.Remove(ComboKey);
    }
}
```

#### 4c. Add ComboTotal damage type

```cpp
// SDamageNumberOverlay.h — add to enum
enum class EDamagePopType : uint8
{
    // ... existing types ...
    ComboTotal   // Yellow, big — multi-hit skill total
};
```

```cpp
// Color: same yellow as CritYellow (0.9, 0.9, 0.15) — matches RO
// Font: CRIT_FONT_SIZE (28pt base) — large and impactful
```

#### 4d. Add ComboTotal animation path in OnPaint

RO Classic combo total behavior:
- Scale: rapid pop-in from 0 to 3.5x in first 50ms, then holds
- Position: higher above entity (+7 base offset), slow rise
- Duration: 3000ms (double normal)
- No drift

```cpp
// In OnPaint animation section:
else if (Entry.Type == EDamagePopType::ComboTotal)
{
    // Rapid pop-in: 0 to 3.5x in 50ms (t=0.017 at 3000ms lifetime)
    Scale = FMath::Min(t / 0.017f, 1.0f) * 3.5f;  // 0→3.5x in ~50ms
    if (t > 0.017f) Scale = 3.5f;  // Hold at 3.5x
    
    // Slow rise from higher position
    OffsetY = -(50.0f + 20.0f * t);  // Higher start, barely moves
    OffsetX = 0.0f;  // No drift
    
    // Slower fade
    Alpha = FMath::Max(0.0f, 1.0f - t * 0.8f);  // Slower fade than normal
}
```

#### 4e. Set lifetime in AddDamagePop

```cpp
// In the lifetime switch:
else if (Type == EDamagePopType::ComboTotal)
{
    Entry.Lifetime = LIFETIME_COMBO;  // 3.0s
    Entry.DriftDirection = 0.0f;
}
```

Add constant: `static constexpr float LIFETIME_COMBO = 3.0f;`

#### Files to modify
| File | Changes |
|------|---------|
| `UI/SDamageNumberOverlay.h` | Add `ComboTotal` to enum, `LIFETIME_COMBO` constant |
| `UI/SDamageNumberOverlay.cpp` | Add ComboTotal color/font/animation, lifetime handling |
| `UI/DamageNumberSubsystem.h` | Add `FComboTracker`, `ActiveCombos` map, `SpawnComboTotal()` |
| `UI/DamageNumberSubsystem.cpp` | Track hits, spawn combo total on last hit |

---

## Phase 5: Target Flinch / Recoil

**Goal**: Brief recoil on the target sprite when hit. RO Classic plays a "HURT" animation frame for `dMotion` milliseconds (~288-576ms for most monsters). No position change. Suppressed by Endure buff.

### Architecture

SpriteCharacterActor already has animation states including a `Hit` state that auto-reverts to Idle. The flinch is simply triggering this animation.

For position-based recoil (enhanced over RO), we could add a brief backward offset, but RO Classic doesn't actually move the sprite — it just plays the hurt frame. Let's match RO faithfully.

### Implementation

#### 5a. Trigger Hit animation state on damage

```cpp
// In CombatActionSubsystem::HandleCombatDamage()
// After resolving target actor:

// Play flinch animation (unless target has Endure)
if (ASpriteCharacterActor* TargetSprite = Cast<ASpriteCharacterActor>(TargetActor))
{
    // Check for Endure buff (skip flinch)
    // The server would need to send an "endure" flag, or we check client-side buff state
    // For now, always play flinch on non-miss hits
    if (HitType != TEXT("miss") && HitType != TEXT("dodge") && HitType != TEXT("perfectDodge"))
    {
        TargetSprite->PlayHitAnimation();  // Existing one-shot Hit state
    }
}
```

#### 5b. Ensure Hit animation exists and works

Check that `SpriteCharacterActor::SetAnimationState()` handles the `Hit` state:
- Sets current animation to the "hit" atlas sequence
- One-shot: plays once then reverts to Idle
- Duration: controlled by atlas frame count * frame duration (~200-300ms)

If the atlas doesn't have a "hit" animation for all classes, fall back to a brief pause/freeze:
```cpp
void ASpriteCharacterActor::PlayHitAnimation()
{
    if (HasAnimationForState(ESpriteAnimState::Hit))
    {
        SetAnimationState(ESpriteAnimState::Hit);
    }
    // If no hit animation, the PlayHitFlash() from Phase 1 provides visual feedback
}
```

#### 5c. Endure buff integration (future)

When the server implements `damageType` in the `combat:damage` event:
```cpp
// Server sends: damageType: 'endure' when target has Endure active
FString DamageType;
Obj->TryGetStringField(TEXT("damageType"), DamageType);
if (DamageType == TEXT("endure"))
{
    // No flinch, no hit animation
    // Optionally play an Endure visual (brief glow effect)
}
else
{
    TargetSprite->PlayHitAnimation();
}
```

#### Files to modify
| File | Changes |
|------|---------|
| `Sprite/SpriteCharacterActor.h` | Add `PlayHitAnimation()` if not existing |
| `Sprite/SpriteCharacterActor.cpp` | Implement hit animation trigger |
| `UI/CombatActionSubsystem.cpp` | Call flinch on target after damage |

---

## Phase 6: Hit Sounds (First Audio System)

**Goal**: Play weapon-type-specific hit sounds on auto-attack damage. This is the FIRST audio system in the project.

### Architecture

No audio infrastructure exists. We need:
1. Sound assets (wav files) organized by weapon type
2. A way to play positional sounds at the target location
3. Random variant selection (3-5 variants per weapon type)
4. Optional pitch variation (+/-5%)

### Implementation

#### 6a. Organize sound assets

```
Content/SabriMMO/Audio/HitSounds/
  Normal/
    Hit_Normal_01.wav
    Hit_Normal_02.wav
    Hit_Normal_03.wav
  Critical/
    Hit_Crit_01.wav
    Hit_Crit_02.wav
  WeaponSwing/  (optional, for attacker swing sounds)
    Swing_Sword_01.wav
    Swing_Dagger_01.wav
    ...
```

For MVP implementation, start with just 3 normal hit variants and 2 crit variants. Weapon-specific sounds can be added later.

#### 6b. Add hit sound playback to CombatActionSubsystem

```cpp
// CombatActionSubsystem.h — add sound references
UPROPERTY()
TArray<USoundBase*> NormalHitSounds;

UPROPERTY()
USoundBase* CritHitSound = nullptr;

// CombatActionSubsystem.cpp — load in OnWorldBeginPlay
NormalHitSounds.Add(LoadObject<USoundBase>(nullptr,
    TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_01.Hit_Normal_01")));
NormalHitSounds.Add(LoadObject<USoundBase>(nullptr,
    TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_02.Hit_Normal_02")));
NormalHitSounds.Add(LoadObject<USoundBase>(nullptr,
    TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_03.Hit_Normal_03")));
CritHitSound = LoadObject<USoundBase>(nullptr,
    TEXT("/Game/SabriMMO/Audio/HitSounds/Critical/Hit_Crit_01.Hit_Crit_01"));
```

#### 6c. Play sound on hit

```cpp
// In HandleCombatDamage(), after resolving target position:
if (HitType != TEXT("miss") && HitType != TEXT("dodge") && Damage > 0)
{
    USoundBase* Sound = nullptr;
    if (bIsCritical && CritHitSound)
    {
        Sound = CritHitSound;
    }
    else if (NormalHitSounds.Num() > 0)
    {
        Sound = NormalHitSounds[FMath::RandRange(0, NormalHitSounds.Num() - 1)];
    }
    
    if (Sound)
    {
        // Positional audio at target location
        // Pitch variation: +/-5% for organic feel (RO Classic behavior)
        float Pitch = FMath::FRandRange(0.95f, 1.05f);
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(), Sound, TargetPos,
            1.0f,   // Volume
            Pitch,  // Pitch
            0.0f    // Start time
        );
    }
}
```

#### 6d. Future: weapon-specific sounds

Later, expand to per-weapon-type sound sets using the server's `weaponType` field (if added to `combat:damage`):

```cpp
TMap<FString, TArray<USoundBase*>> WeaponHitSounds;
// "sword" → [sword_hit_01, sword_hit_02, ...]
// "dagger" → [dagger_hit_01, dagger_hit_02, ...]
// "mace" → [mace_hit_01, mace_hit_02, ...]
```

#### 6e. Sound assets

For initial implementation, use royalty-free RPG hit sounds or generate them:
- Normal hits: short percussive impacts (~80-150ms), "thud" character
- Critical hits: higher-pitched, more dramatic "crunch" (~100-150ms)
- Total: 5 wav files minimum (3 normal + 2 crit)

Sources for free RPG hit sounds:
- freesound.org (search "sword hit", "melee impact", "rpg hit")
- opengameart.org (search "combat sounds")
- kenney.nl/assets (game audio packs)

Import settings in UE5: PCM quality, no compression for short sounds.

#### Files to modify
| File | Changes |
|------|---------|
| `UI/CombatActionSubsystem.h` | Add sound UPROPERTYs |
| `UI/CombatActionSubsystem.cpp` | Load sounds, play on hit |

---

## Summary: All Files Modified Per Phase

| Phase | Feature | Files Modified | New Files |
|-------|---------|---------------|-----------|
| 1 | Hit Flash | SpriteCharacterActor.h/.cpp, CombatActionSubsystem.cpp | None |
| 2 | Hit Particles | SkillVFXSubsystem.h/.cpp, CombatActionSubsystem.cpp | None |
| 3 | Crit Starburst | SDamageNumberOverlay.h/.cpp, DamageNumberSubsystem.h/.cpp | T_CritStarburst.png |
| 4 | Combo Total | SDamageNumberOverlay.h/.cpp, DamageNumberSubsystem.h/.cpp | None |
| 5 | Flinch | SpriteCharacterActor.h/.cpp, CombatActionSubsystem.cpp | None |
| 6 | Hit Sounds | CombatActionSubsystem.h/.cpp | 5 wav files |

## Implementation Order Recommendation

1. **Phase 3 (Crit Starburst)** — Fastest to implement, pure Slate, extends existing OnPaint
2. **Phase 4 (Combo Total)** — Medium effort, extends DamageNumberSubsystem
3. **Phase 1 (Hit Flash)** — Material parameter work, need to test TintColor approach
4. **Phase 2 (Hit Particles)** — Uses existing VFX infrastructure
5. **Phase 5 (Flinch)** — Depends on verifying Hit animation state exists in atlases
6. **Phase 6 (Hit Sounds)** — Needs sound assets sourced/created first

## What NOT to Implement

- **Bitmap digit font** — The current TTF Bold font with outline looks good. Bitmap sprites would require a texture atlas, custom UV rendering, and significant refactoring for marginal visual improvement. Skip unless specifically requested.
- **Screen shake on crit** — RO Classic does NOT shake on crits. Shake is reserved for AoE skills (Meteor Storm, LoV). Don't add it.
- **Endure visual glow** — Low priority, requires Endure buff state to be available client-side. Plan for later.
