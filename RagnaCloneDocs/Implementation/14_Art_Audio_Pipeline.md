# 14 - Art, Animation, VFX & Audio Pipeline: UE5 C++ Implementation Guide

> **Scope**: Character models, animation state machines, VFX patterns, post-processing, audio subsystem, UI art theme, and asset organization.
> **Stack**: UE5.7 (C++ + Blueprints) | Niagara + Cascade VFX | Pure Slate UI
> **Prerequisite**: Read `07_UI_HUD_System.md` for ROColors namespace, `SkillVFXData.h/cpp` for existing VFX config pattern.

---

## Table of Contents

1. [Character Model Pipeline](#1-character-model-pipeline)
2. [Animation System](#2-animation-system)
3. [VFX System](#3-vfx-system)
4. [Post-Processing](#4-post-processing)
5. [Audio System](#5-audio-system)
6. [UI Art Theme](#6-ui-art-theme)
7. [Asset Organization](#7-asset-organization)

---

## 1. Character Model Pipeline

### 1.1 Skeletal Mesh Requirements

| Requirement | Specification |
|-------------|---------------|
| Bone root | `Root` -> `Pelvis` (UE5 Mannequin compatible) |
| Bone count | 65-80 bones (body), 10-15 (face), 20-30 (cloth/hair physics) |
| Poly budget | LOD0: 15k tris, LOD1: 8k tris, LOD2: 3k tris |
| LOD distances | LOD0: 0-2000, LOD1: 2000-5000, LOD2: 5000+ |
| Texture size | Body: 2048x2048, Hair: 1024x1024, Equipment: 512-1024 |
| Material slots | Body, Hair, Eyes, UpperArmor, LowerArmor, Weapon |

### 1.2 LOD Setup in C++

```cpp
// In constructor or PostInitializeComponents
USkeletalMeshComponent* MeshComp = GetMesh();
if (MeshComp && MeshComp->GetSkeletalMeshAsset())
{
    MeshComp->SetForcedLodModel(0); // 0 = auto LOD
    MeshComp->bEnablePerPolyCollision = false;

    // Screen-size thresholds for LOD transitions
    FSkeletalMeshLODInfo& LOD1 = MeshComp->GetSkeletalMeshAsset()->GetLODInfo(1);
    LOD1.ScreenSize = FPerPlatformFloat(0.4f);
    FSkeletalMeshLODInfo& LOD2 = MeshComp->GetSkeletalMeshAsset()->GetLODInfo(2);
    LOD2.ScreenSize = FPerPlatformFloat(0.15f);
}
```

### 1.3 Master Pose Component Pattern (Modular Character)

The player character uses a Master Pose setup: one leader skeleton drives body, outfit, and hair child meshes. Equipment swaps only change the child mesh, not the leader.

```cpp
// CharacterAppearanceComponent.h
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SABRIMMO_API UCharacterAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void InitializeAppearance(USkeletalMeshComponent* InLeaderMesh);
    void SetOutfitMesh(USkeletalMesh* NewMesh);
    void SetHairMesh(USkeletalMesh* NewMesh, int32 HairStyle);
    void SetHairColor(int32 ColorIndex);
    void SetWeaponMesh(UStaticMesh* NewMesh, FName SocketName = TEXT("weapon_r"));
    void ClearWeapon();

private:
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> LeaderMesh;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> OutfitMesh;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> HairMesh;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> HairMaterialInstance;

    // Hair color palette (RO Classic: 9 colors, indices 0-8)
    static const TArray<FLinearColor>& GetHairColorPalette();
};
```

```cpp
// CharacterAppearanceComponent.cpp
void UCharacterAppearanceComponent::InitializeAppearance(USkeletalMeshComponent* InLeaderMesh)
{
    LeaderMesh = InLeaderMesh;
    AActor* Owner = GetOwner();
    if (!Owner || !LeaderMesh) return;

    // Outfit child mesh — follows leader skeleton
    OutfitMesh = NewObject<USkeletalMeshComponent>(Owner, TEXT("OutfitMesh"));
    OutfitMesh->SetupAttachment(LeaderMesh);
    OutfitMesh->SetLeaderPoseComponent(LeaderMesh);
    OutfitMesh->RegisterComponent();

    // Hair child mesh — follows leader skeleton
    HairMesh = NewObject<USkeletalMeshComponent>(Owner, TEXT("HairMesh"));
    HairMesh->SetupAttachment(LeaderMesh);
    HairMesh->SetLeaderPoseComponent(LeaderMesh);
    HairMesh->RegisterComponent();

    // Weapon — static mesh attached to socket
    WeaponMesh = NewObject<UStaticMeshComponent>(Owner, TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(LeaderMesh, TEXT("weapon_r"));
    WeaponMesh->SetVisibility(false);
    WeaponMesh->RegisterComponent();
}

void UCharacterAppearanceComponent::SetOutfitMesh(USkeletalMesh* NewMesh)
{
    if (OutfitMesh && NewMesh)
    {
        OutfitMesh->SetSkeletalMesh(NewMesh);
        OutfitMesh->SetLeaderPoseComponent(LeaderMesh);
    }
}

void UCharacterAppearanceComponent::SetHairMesh(USkeletalMesh* NewMesh, int32 HairStyle)
{
    if (!HairMesh || !NewMesh) return;
    HairMesh->SetSkeletalMesh(NewMesh);
    HairMesh->SetLeaderPoseComponent(LeaderMesh);
}

void UCharacterAppearanceComponent::SetHairColor(int32 ColorIndex)
{
    if (!HairMesh) return;
    UMaterialInterface* BaseMat = HairMesh->GetMaterial(0);
    if (!BaseMat) return;

    if (!HairMaterialInstance)
    {
        HairMaterialInstance = HairMesh->CreateDynamicMaterialInstance(0, BaseMat);
    }

    const TArray<FLinearColor>& Palette = GetHairColorPalette();
    int32 Idx = FMath::Clamp(ColorIndex, 0, Palette.Num() - 1);
    HairMaterialInstance->SetVectorParameterValue(TEXT("HairColor"), Palette[Idx]);
}

void UCharacterAppearanceComponent::SetWeaponMesh(UStaticMesh* NewMesh, FName SocketName)
{
    if (!WeaponMesh || !NewMesh) return;
    WeaponMesh->SetStaticMesh(NewMesh);
    WeaponMesh->AttachToComponent(LeaderMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    WeaponMesh->SetVisibility(true);
}

void UCharacterAppearanceComponent::ClearWeapon()
{
    if (WeaponMesh)
    {
        WeaponMesh->SetVisibility(false);
        WeaponMesh->SetStaticMesh(nullptr);
    }
}

const TArray<FLinearColor>& UCharacterAppearanceComponent::GetHairColorPalette()
{
    // RO Classic 9-color palette (indices 0-8 matching server hair_color)
    static TArray<FLinearColor> Palette = {
        FLinearColor(0.10f, 0.07f, 0.05f, 1.f),  // 0: Black
        FLinearColor(0.55f, 0.25f, 0.10f, 1.f),  // 1: Brown
        FLinearColor(0.80f, 0.60f, 0.20f, 1.f),  // 2: Blonde
        FLinearColor(0.85f, 0.15f, 0.10f, 1.f),  // 3: Red
        FLinearColor(0.35f, 0.35f, 0.65f, 1.f),  // 4: Blue
        FLinearColor(0.50f, 0.20f, 0.55f, 1.f),  // 5: Purple
        FLinearColor(0.20f, 0.50f, 0.25f, 1.f),  // 6: Green
        FLinearColor(0.75f, 0.75f, 0.75f, 1.f),  // 7: Silver
        FLinearColor(0.90f, 0.40f, 0.55f, 1.f),  // 8: Pink
    };
    return Palette;
}
```

### 1.4 Equipment Mesh Swap on Server Event

When `inventory:equipped` fires, look up the item's mesh path and call the appearance component:

```cpp
// In EquipmentSubsystem or inventory handler
void HandleEquipChanged(const FInventoryItem& Item, bool bEquipped)
{
    APawn* Pawn = GetWorld()->GetFirstPlayerController()->GetPawn();
    if (!Pawn) return;

    auto* Appearance = Pawn->FindComponentByClass<UCharacterAppearanceComponent>();
    if (!Appearance) return;

    if (Item.EquipSlot == TEXT("weapon"))
    {
        if (bEquipped)
        {
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
                *FString::Printf(TEXT("/Game/SabriMMO/Meshes/Weapons/SM_%s"), *Item.WeaponType));
            FName Socket = TEXT("weapon_r");
            if (Item.WeaponType == TEXT("bow")) Socket = TEXT("weapon_l");
            Appearance->SetWeaponMesh(Mesh, Socket);
        }
        else
        {
            Appearance->ClearWeapon();
        }
    }
    // Similar blocks for armor, headgear, etc.
}
```

### 1.5 Hair Style Asset Mapping

Hair style (1-19) maps to `SK_Hair_{Gender}_{02d}` in `/Game/SabriMMO/Meshes/Hair/`. Loaded via `LoadObject<USkeletalMesh>` with `FMath::Clamp(HairStyle, 1, 19)`.

---

## 2. Animation System

### 2.1 Animation Blueprint State Machine

The player AnimBP uses a layered state machine with two layers:
- **Base Layer**: Locomotion (Idle/Walk/Run) + Death
- **Upper Body Layer**: Attack/Cast/Hit (blended from chest up)

State machine states and transitions:

| State | Entry Condition | Exit Condition |
|-------|----------------|----------------|
| Idle | Speed < 5 | Speed >= 5, OR attack/hit/death |
| Walk | 5 <= Speed < 300 | Speed < 5 OR Speed >= 300 |
| Run | Speed >= 300 | Speed < 300 |
| Attack | `bIsAttacking == true` | Montage finished OR interrupted |
| Hit | `bIsHit == true` | HitReact montage finished (0.3s) |
| Cast | `bIsCasting == true` | Cast complete/interrupted |
| Die | `bIsDead == true` | Respawn event |

### 2.2 Anim Instance C++ Base Class

```cpp
// MMOAnimInstance.h
UCLASS()
class SABRIMMO_API UMMOAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // ---- State variables (read by AnimBP state machine) ----
    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
    float Direction = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsHit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsCasting = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bIsDead = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    float AttackPlayRate = 1.0f;

    // ---- Montage triggers (called from gameplay code) ----
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PlayAttackMontage(UAnimMontage* Montage, float ServerASPD);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PlayHitReact();

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PlayDeathMontage(UAnimMontage* Montage);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void PlayCastMontage(UAnimMontage* Montage, float CastDuration);

private:
    UPROPERTY()
    TObjectPtr<APawn> OwnerPawn;
};
```

```cpp
// MMOAnimInstance.cpp
void UMMOAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    OwnerPawn = TryGetPawnOwner();
}

void UMMOAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    if (!OwnerPawn) return;

    FVector Velocity = OwnerPawn->GetVelocity();
    Speed = Velocity.Size2D();

    // Direction for strafing blend space (-180 to 180)
    if (Speed > 5.f)
    {
        FRotator ActorRot = OwnerPawn->GetActorRotation();
        Direction = CalculateDirection(Velocity, ActorRot);
    }
}
```

### 2.3 ASPD-Based Attack Playback Rate

RO Classic ASPD determines attack speed. The server sends `attackMotion` (ms) per attack. The client adjusts montage play rate so the animation fits within that window.

```
PlayRate = BaseAnimLength / (ServerAttackMotion / 1000.0)
```

```cpp
void UMMOAnimInstance::PlayAttackMontage(UAnimMontage* Montage, float ServerASPD)
{
    if (!Montage) return;

    // ServerASPD = attackMotion in milliseconds from server
    float BaseLength = Montage->GetPlayLength();  // seconds
    float TargetLength = ServerASPD / 1000.0f;     // convert ms to seconds

    // Clamp to reasonable range (0.3x to 4.0x speed)
    AttackPlayRate = FMath::Clamp(BaseLength / TargetLength, 0.3f, 4.0f);

    bIsAttacking = true;
    Montage_Play(Montage, AttackPlayRate);

    // Listen for montage end to reset flag
    FOnMontageEnded EndDelegate;
    EndDelegate.BindLambda([this](UAnimMontage*, bool) {
        bIsAttacking = false;
    });
    Montage_SetEndDelegate(EndDelegate, Montage);
}

void UMMOAnimInstance::PlayHitReact()
{
    bIsHit = true;
    // Reset after damageMotion (server value) or fixed 0.3s
    GetWorld()->GetTimerManager().SetTimer(
        HitResetTimer, [this]() { bIsHit = false; }, 0.3f, false);
}

void UMMOAnimInstance::PlayDeathMontage(UAnimMontage* Montage)
{
    bIsDead = true;
    if (Montage)
    {
        Montage_Play(Montage, 1.0f);
    }
}

void UMMOAnimInstance::PlayCastMontage(UAnimMontage* Montage, float CastDuration)
{
    if (!Montage) return;

    bIsCasting = true;
    float BaseLength = Montage->GetPlayLength();
    float Rate = (CastDuration > 0.f) ? BaseLength / CastDuration : 1.0f;
    Montage_Play(Montage, FMath::Clamp(Rate, 0.1f, 3.0f));

    FOnMontageEnded EndDelegate;
    EndDelegate.BindLambda([this](UAnimMontage*, bool) {
        bIsCasting = false;
    });
    Montage_SetEndDelegate(EndDelegate, Montage);
}
```

### 2.4 Attack Montages Per Weapon Type

```cpp
// WeaponAnimData.h — Maps weapon types to attack montage assets
struct FWeaponAnimSet
{
    TSoftObjectPtr<UAnimMontage> AttackMontage1;  // Primary combo hit
    TSoftObjectPtr<UAnimMontage> AttackMontage2;  // Secondary combo hit
    TSoftObjectPtr<UAnimMontage> CriticalMontage; // Critical hit override
    float BaseAnimLength = 0.6f;                  // Base montage length (seconds)
};

// Weapon type -> montage path mapping table
// Pattern: /Game/SabriMMO/Anims/AM_Atk_{Type}_{01|02|Crit}
static const TMap<FString, FWeaponAnimSet>& GetWeaponAnimSets()
{
    static TMap<FString, FWeaponAnimSet> Sets;
    if (Sets.Num() == 0)
    {
        auto Add = [&](const FString& Type, const TCHAR* Atk1, const TCHAR* Atk2,
                        const TCHAR* Crit, float BaseLen) {
            FWeaponAnimSet S;
            S.AttackMontage1 = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(Atk1));
            S.AttackMontage2 = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(Atk2));
            S.CriticalMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(Crit));
            S.BaseAnimLength = BaseLen;
            Sets.Add(Type, S);
        };
        // WeaponType       Attack1                          Attack2                          Critical                       BaseLen
        Add(TEXT("dagger"),         TEXT("/Game/SabriMMO/Anims/AM_Atk_Dagger_01"),   TEXT("...02"), TEXT("...Crit"), 0.5f);
        Add(TEXT("one_hand_sword"), TEXT("/Game/SabriMMO/Anims/AM_Atk_1HSword_01"),  TEXT("...02"), TEXT("...Crit"), 0.6f);
        Add(TEXT("staff"),          TEXT("/Game/SabriMMO/Anims/AM_Atk_Staff_01"),     TEXT("...02"), TEXT("...Crit"), 0.7f);
        Add(TEXT("bow"),            TEXT("/Game/SabriMMO/Anims/AM_Atk_Bow_01"),       TEXT("...02"), TEXT("...Crit"), 0.8f);
        Add(TEXT("mace"),           TEXT("/Game/SabriMMO/Anims/AM_Atk_Mace_01"),      TEXT("...02"), TEXT("...Crit"), 0.65f);
        Add(TEXT("spear"),          TEXT("/Game/SabriMMO/Anims/AM_Atk_Spear_01"),     TEXT("...02"), TEXT("...Crit"), 0.75f);
        Add(TEXT("unarmed"),        TEXT("/Game/SabriMMO/Anims/AM_Atk_Unarmed_01"),   TEXT("...02"), TEXT("...Crit"), 0.5f);
    }
    return Sets;
}
```

### 2.5 Triggering Attacks from Socket Events

When `combat:damage` arrives (auto-attack only), trigger the appropriate animation:

```cpp
// In combat handler (e.g. BP_MMOCharacter or a CombatAnimSubsystem)
void HandleCombatDamage(int32 AttackerId, int32 TargetId, float AttackMotion,
                        const FString& WeaponType, bool bIsCritical)
{
    AActor* AttackerActor = FindPlayerOrEnemyActor(AttackerId);
    if (!AttackerActor) return;

    USkeletalMeshComponent* MeshComp = AttackerActor->FindComponentByClass<USkeletalMeshComponent>();
    if (!MeshComp) return;

    UMMOAnimInstance* AnimInst = Cast<UMMOAnimInstance>(MeshComp->GetAnimInstance());
    if (!AnimInst) return;

    const auto& Sets = GetWeaponAnimSets();
    const FWeaponAnimSet* Set = Sets.Find(WeaponType);
    if (!Set) Set = Sets.Find(TEXT("unarmed"));
    if (!Set) return;

    // Alternate between combo hits
    static TMap<int32, int32> ComboCounters;
    int32& Counter = ComboCounters.FindOrAdd(AttackerId, 0);
    Counter = (Counter + 1) % 2;

    TSoftObjectPtr<UAnimMontage> MontageRef = bIsCritical ? Set->CriticalMontage
        : (Counter == 0 ? Set->AttackMontage1 : Set->AttackMontage2);

    UAnimMontage* Montage = MontageRef.LoadSynchronous();
    if (Montage)
    {
        AnimInst->PlayAttackMontage(Montage, AttackMotion);
    }
}
```

### 2.6 Anim Notify Events from C++

Create notify classes to fire gameplay events at specific animation frames:

```cpp
// AnimNotify_AttackHit.h
UCLASS()
class SABRIMMO_API UAnimNotify_AttackHit : public UAnimNotify
{
    GENERATED_BODY()
public:
    virtual void Notify(USkeletalMeshComponent* MeshComp,
                        UAnimSequenceBase* Animation,
                        const FAnimNotifyEventReference& EventReference) override
    {
        AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
        if (!Owner) return;

        // Fire weapon trail VFX at hit frame
        // Actual damage is server-authoritative, this is cosmetic only
        UE_LOG(LogTemp, Verbose, TEXT("AttackHit notify on %s"), *Owner->GetName());
    }
};

// AnimNotify_FootStep.h
UCLASS()
class SABRIMMO_API UAnimNotify_FootStep : public UAnimNotify
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Sound")
    USoundBase* FootstepSound = nullptr;

    virtual void Notify(USkeletalMeshComponent* MeshComp,
                        UAnimSequenceBase* Animation,
                        const FAnimNotifyEventReference& EventReference) override
    {
        AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
        if (!Owner || !FootstepSound) return;
        UGameplayStatics::PlaySoundAtLocation(Owner, FootstepSound, Owner->GetActorLocation());
    }
};
```

### 2.7 Monster Animation Blueprint (Base Class)

Monsters reuse the same `UMMOAnimInstance` with a simplified state machine (no weapon variation, no casting).

```cpp
// MonsterAnimInstance.h
UCLASS()
class SABRIMMO_API UMonsterAnimInstance : public UMMOAnimInstance
{
    GENERATED_BODY()
public:
    // Monster-specific: walk speed ratio for blend space
    UPROPERTY(BlueprintReadOnly, Category = "Monster")
    float WalkSpeedRatio = 0.6f;  // wander at 60%, chase at 100%

    virtual void NativeUpdateAnimation(float DeltaSeconds) override
    {
        Super::NativeUpdateAnimation(DeltaSeconds);
        // Monsters use a single attack montage per template
        // Selected by BP_EnemyManager based on monster_id
    }
};
```

All monster AnimBPs inherit from `ABP_Monster_Base` which uses:
- Idle/Walk blend space (Speed 0-500)
- Attack montage slot (upper body)
- Death montage (full body, play-once)
- Hit react additive layer

---

## 3. VFX System

### 3.1 Existing 5 VFX Patterns (Recap)

All VFX is managed by `USkillVFXSubsystem` (WorldSubsystem). Configs live in `SkillVFXData.cpp`.

| Pattern | Template Enum | Description | Examples |
|---------|---------------|-------------|----------|
| A: Bolt | `BoltFromSky` | N bolts strike from sky, staggered 0.15s | Cold/Fire/Lightning Bolt |
| B: AoE Projectile | `Projectile` + `bSingleProjectile` | 1 projectile + impact explosion at target | Fire Ball |
| C: Multi-Hit | `Projectile` | N projectiles player->enemy, staggered 200ms | Soul Strike |
| D: Persistent Buff | `SelfBuff` / `TargetDebuff` | Spawned on `skill:buff_applied`, destroyed on `skill:buff_removed` | Frost Diver, Provoke, Endure |
| E: Ground Rain | `GroundAoERain` | N strikes at random AoE positions, staggered 300ms | Thunderstorm |

Additional templates: `GroundPersistent` (Fire Wall, Safety Wall), `HealFlash` (First Aid), `AoEImpact` (Bash, Magnum Break).

### 3.2 Adding VFX for a New Skill (4-Step Process)

**Step 1: Identify the pattern.** Match the skill to one of the 5 patterns above (or `AoEImpact`/`HealFlash`).

**Step 2: Find or import the VFX asset.** Check `docsNew/05_Development/VFX_Asset_Reference.md` for 1,574 cataloged assets. Prefer Niagara (`NS_`) over Cascade (`P_`) unless you need scale-aware world-space particles.

**Step 3: Add config entry to `SkillVFXData.cpp`.**

```cpp
// In BuildSkillVFXConfigs(), add:
Add(SKILL_ID, ESkillVFXTemplate::Projectile,
    FLinearColor(R, G, B, 1.f),        // PrimaryColor
    TEXT("element"),                     // Element string
    /*bCastCircle=*/ true,
    /*AoERadius=*/ 0.f,
    /*bLooping=*/ false,
    /*ProjSpeed=*/ 1500.f,
    /*BoltHeight=*/ 0.f,
    TEXT("/Game/PackName/VFX/NS_Asset.NS_Asset"),  // VFXOverridePath
    /*bCascade=*/ false,
    /*Scale=*/ 1.0f);

// If the skill needs special flags:
Configs[SKILL_ID].bSingleProjectile = true;
Configs[SKILL_ID].ImpactOverridePath = TEXT("/Game/.../NS_Impact.NS_Impact");
```

**Step 4: Verify server events.** Ensure the server emits the correct events for the pattern:
- Bolt/Projectile/AoE: `skill:effect_damage` with `attackerId, targetId, skillId, targetX/Y/Z`
- Buff: `skill:buff_applied` with `targetId, skillId, duration` and `skill:buff_removed`
- Ground: `skill:effect_damage` with `groundX/Y/Z` for storm center

No C++ changes needed in `SkillVFXSubsystem` -- all routing is automatic based on `FSkillVFXConfig.Template`.

### 3.3 Environmental VFX

Environmental effects are spawned by zone-specific actors, not by `SkillVFXSubsystem`. Use a `UZoneEnvironmentComponent` with `TSoftObjectPtr<UNiagaraSystem>` properties for Rain/Snow/Fog, a `WeatherType` string, and spawn via `UNiagaraFunctionLibrary::SpawnSystemAttached()` in `BeginPlay()`.

### 3.4 Performance Budget

| Metric | Budget |
|--------|--------|
| Max simultaneous Niagara systems | 30 (all players + enemies in view) |
| Max particles per system | 500 (use GPU sim for > 100) |
| Distance cull threshold | 5000 UE units (beyond this, skip spawn) |
| Cascade systems (legacy) | Max 10 simultaneous (buff auras only) |
| LOD for particles | Enable `NiagaraScalabilitySettings` per system |

Distance culling in `SkillVFXSubsystem`:

```cpp
// Before any SpawnNiagaraAtLocation call
APlayerController* PC = GetWorld()->GetFirstPlayerController();
if (PC && PC->GetPawn())
{
    float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), Location);
    if (Distance > 5000.f) return; // Skip VFX beyond view distance
}
```

---

## 4. Post-Processing

### 4.1 Cel-Shading Approach

Use a post-process material with custom stencil to apply a toon-shading pass. This preserves the RO aesthetic (flat-lit, bold colors) without modifying individual material shaders.

```
Post-Process Volume (Unbound)
  -> Post Process Material: M_PP_CelShade
       -> Reads SceneColor, SceneDepth, CustomDepth
       -> Quantizes lighting to 3 bands (shadow, mid, highlight)
       -> Blending: Before Tonemapping
       -> Priority: 0
```

Material pseudo-logic (Material Editor):

```
// Quantize scene luminance to N bands
float Lum = dot(SceneColor.rgb, float3(0.299, 0.587, 0.114));
float Bands = 3.0;
float Quantized = floor(Lum * Bands) / Bands;
float3 Result = SceneColor.rgb * (Quantized / max(Lum, 0.001));
```

### 4.2 Outline Shader

Sobel edge detection on scene depth and normals, drawn as a full-screen post-process pass:

```
Post-Process Volume (Unbound)
  -> Post Process Material: M_PP_Outline
       -> Reads SceneDepth (3x3 Sobel kernel)
       -> Reads WorldNormal (edge detection on normal discontinuities)
       -> Combines: max(DepthEdge, NormalEdge) > Threshold -> OutlineColor
       -> OutlineColor: (0.05, 0.03, 0.01, 1.0)  // Near-black brown
       -> OutlineWidth: controlled by pixel offset (1-2 pixels)
       -> Blending: After Tonemapping
       -> Priority: 1
```

### 4.3 Color Grading Per Zone

Each zone has a Post-Process Volume with custom color grading:

| Zone Type | Temperature | Saturation | Contrast | Bloom |
|-----------|-------------|------------|----------|-------|
| Town (Prontera) | Warm (+0.1) | 1.1 | 1.0 | 0.4 |
| Forest | Green shift | 1.2 | 1.05 | 0.5 |
| Dungeon | Cool (-0.15) | 0.7 | 1.2 | 0.2 |
| Desert | Warm (+0.2) | 0.9 | 1.1 | 0.6 |
| Snow | Cool (-0.1) | 0.85 | 0.95 | 0.7 |

Set via Level Blueprint or zone actor `BeginPlay`:

```cpp
void SetZonePostProcess(APostProcessVolume* Volume, const FString& ZoneType)
{
    FPostProcessSettings& S = Volume->Settings;

    if (ZoneType == TEXT("town"))
    {
        S.bOverride_WhiteTemp = true;
        S.WhiteTemp = 7000.f;  // warmer
        S.bOverride_ColorSaturation = true;
        S.ColorSaturation = FVector4(1.1f, 1.1f, 1.1f, 1.0f);
        S.bOverride_BloomIntensity = true;
        S.BloomIntensity = 0.4f;
    }
    else if (ZoneType == TEXT("dungeon"))
    {
        S.bOverride_WhiteTemp = true;
        S.WhiteTemp = 5500.f;  // cooler
        S.bOverride_ColorSaturation = true;
        S.ColorSaturation = FVector4(0.7f, 0.7f, 0.7f, 1.0f);
        S.bOverride_BloomIntensity = true;
        S.BloomIntensity = 0.2f;
    }
    // ... other zone types
}
```

---

## 5. Audio System

### 5.1 AudioSubsystem Header

```cpp
// AudioSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "AudioSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class USoundAttenuation;
class USocketIOClientComponent;

UENUM()
enum class ESFXCategory : uint8
{
    Combat,
    Skill,
    UI,
    Ambient,
    Footstep
};

USTRUCT()
struct FBGMEntry
{
    GENERATED_BODY()

    FString ZoneName;
    TSoftObjectPtr<USoundBase> BGMAsset;
    float Volume = 1.0f;
};

UCLASS()
class SABRIMMO_API UAudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- BGM ----
    UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
    void PlayBGMForZone(const FString& ZoneName);

    UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
    void CrossfadeBGM(USoundBase* NewBGM, float FadeDuration = 2.0f);

    UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
    void StopBGM(float FadeOut = 1.0f);

    // ---- SFX ----
    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlayCombatSFX(const FString& SFXName, FVector Location);

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlaySkillSFX(int32 SkillId, FVector Location);

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlayUISFX(const FString& SFXName);

    UFUNCTION(BlueprintCallable, Category = "Audio|SFX")
    void PlayAmbientSFX(USoundBase* Sound, FVector Location, float VolumeMultiplier = 1.0f);

    // ---- Volume Controls ----
    UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
    void SetBGMVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
    void SetSFXVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
    void SetAmbientVolume(float Volume);

    UFUNCTION(BlueprintPure, Category = "Audio|Volume")
    float GetBGMVolume() const { return BGMVolume; }

    UFUNCTION(BlueprintPure, Category = "Audio|Volume")
    float GetSFXVolume() const { return SFXVolume; }

    UFUNCTION(BlueprintPure, Category = "Audio|Volume")
    float GetAmbientVolume() const { return AmbientVolume; }

    void SaveVolumeSettings();
    void LoadVolumeSettings();

private:
    // ---- Socket wrapping ----
    void TryWrapSocketEvents();
    USocketIOClientComponent* FindSocketIOComponent() const;

    void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
    void HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data);
    void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);

    USoundBase* GetBGMForZone(const FString& ZoneName) const;
    USoundBase* GetCombatSFX(const FString& SFXName) const;
    USoundBase* GetSkillSFX(int32 SkillId) const;
    USoundBase* GetUISFX(const FString& SFXName) const;

    // ---- BGM dual-component crossfade ----
    UPROPERTY()
    TObjectPtr<UAudioComponent> BGMComponentA;

    UPROPERTY()
    TObjectPtr<UAudioComponent> BGMComponentB;

    int32 ActiveBGMIndex = 0;  // 0 = A is active, 1 = B is active

    UAudioComponent* GetActiveBGMComponent() const;
    UAudioComponent* GetInactiveBGMComponent() const;

    // ---- Attenuation ----
    UPROPERTY()
    TObjectPtr<USoundAttenuation> CombatAttenuation;

    UPROPERTY()
    TObjectPtr<USoundAttenuation> AmbientAttenuation;

    // ---- Volume state ----
    float BGMVolume = 0.7f;
    float SFXVolume = 1.0f;
    float AmbientVolume = 0.5f;

    // ---- State ----
    bool bEventsWrapped = false;
    FString CurrentBGMZone;
    FTimerHandle BindCheckTimer;
    FTimerHandle CrossfadeTimer;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;

    // ---- Asset caches ----
    TMap<FString, TObjectPtr<USoundBase>> BGMCache;
    TMap<FString, TObjectPtr<USoundBase>> CombatSFXCache;
    TMap<int32, TObjectPtr<USoundBase>>   SkillSFXCache;
    TMap<FString, TObjectPtr<USoundBase>> UISFXCache;
};
```

### 5.2 AudioSubsystem Implementation (Key Methods)

```cpp
// AudioSubsystem.cpp
#include "AudioSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/ConfigCacheIni.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMOAudio, Log, All);

// ============================================================
// Zone -> BGM mapping table
// ============================================================
static const TMap<FString, FString>& GetZoneBGMMap()
{
    static TMap<FString, FString> Map;
    if (Map.Num() == 0)
    {
        // Zone name -> BGM asset path
        Map.Add(TEXT("prt_south"),   TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Prontera"));
        Map.Add(TEXT("prt_north"),   TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Prontera"));
        Map.Add(TEXT("prt_fild01"),  TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Field01"));
        Map.Add(TEXT("prt_fild02"),  TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Field02"));
        Map.Add(TEXT("mjo_dun01"),   TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Dungeon01"));
        Map.Add(TEXT("prt_castle"),  TEXT("/Game/SabriMMO/Audio/BGM/SB_BGM_Castle"));
        // Add more zones as implemented
    }
    return Map;
}

// ============================================================
// Skill -> SFX mapping table
// ============================================================
static const TMap<int32, FString>& GetSkillSFXMap()
{
    static TMap<int32, FString> Map;
    if (Map.Num() == 0)
    {
        Map.Add(2,   TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_Heal"));          // First Aid
        Map.Add(103, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_Bash"));          // Bash
        Map.Add(104, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_Provoke"));       // Provoke
        Map.Add(105, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_MagnumBreak"));   // Magnum Break
        Map.Add(200, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_ColdBolt"));      // Cold Bolt
        Map.Add(201, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_FireBolt"));      // Fire Bolt
        Map.Add(202, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_LightningBolt")); // Lightning Bolt
        Map.Add(207, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_FireBall"));      // Fire Ball
        Map.Add(208, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_FrostDiver"));    // Frost Diver
        Map.Add(210, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_SoulStrike"));    // Soul Strike
        Map.Add(212, TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_Thunderstorm")); // Thunderstorm
    }
    return Map;
}

// ============================================================
// Lifecycle
// ============================================================

bool UAudioSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    LoadVolumeSettings();

    // Create dual BGM audio components for crossfade
    AActor* WorldSettings = InWorld.GetWorldSettings();
    if (WorldSettings)
    {
        BGMComponentA = NewObject<UAudioComponent>(WorldSettings, TEXT("BGM_A"));
        BGMComponentA->bAutoActivate = false;
        BGMComponentA->bIsUISound = true;  // Not positional
        BGMComponentA->RegisterComponent();

        BGMComponentB = NewObject<UAudioComponent>(WorldSettings, TEXT("BGM_B"));
        BGMComponentB->bAutoActivate = false;
        BGMComponentB->bIsUISound = true;
        BGMComponentB->RegisterComponent();
    }

    // Create attenuation settings for 3D sounds
    CombatAttenuation = NewObject<USoundAttenuation>(this, TEXT("CombatAttenuation"));
    CombatAttenuation->Attenuation.bAttenuate = true;
    CombatAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
    CombatAttenuation->Attenuation.AttenuationShapeExtents = FVector(200.f);
    CombatAttenuation->Attenuation.FalloffDistance = 2000.f;

    AmbientAttenuation = NewObject<USoundAttenuation>(this, TEXT("AmbientAttenuation"));
    AmbientAttenuation->Attenuation.bAttenuate = true;
    AmbientAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
    AmbientAttenuation->Attenuation.AttenuationShapeExtents = FVector(500.f);
    AmbientAttenuation->Attenuation.FalloffDistance = 5000.f;

    // Start polling for socket events
    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UAudioSubsystem::TryWrapSocketEvents),
        0.5f, true);

    UE_LOG(LogMMOAudio, Log, TEXT("AudioSubsystem started."));
}

void UAudioSubsystem::Deinitialize()
{
    StopBGM(0.f);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
        World->GetTimerManager().ClearTimer(CrossfadeTimer);
    }

    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}

// ============================================================
// BGM Crossfade
// ============================================================

UAudioComponent* UAudioSubsystem::GetActiveBGMComponent() const
{
    return (ActiveBGMIndex == 0) ? BGMComponentA.Get() : BGMComponentB.Get();
}

UAudioComponent* UAudioSubsystem::GetInactiveBGMComponent() const
{
    return (ActiveBGMIndex == 0) ? BGMComponentB.Get() : BGMComponentA.Get();
}

void UAudioSubsystem::PlayBGMForZone(const FString& ZoneName)
{
    if (ZoneName == CurrentBGMZone) return;  // Already playing this zone's BGM

    USoundBase* NewBGM = GetBGMForZone(ZoneName);
    if (!NewBGM)
    {
        UE_LOG(LogMMOAudio, Warning, TEXT("No BGM found for zone: %s"), *ZoneName);
        return;
    }

    CurrentBGMZone = ZoneName;
    CrossfadeBGM(NewBGM, 2.0f);
}

void UAudioSubsystem::CrossfadeBGM(USoundBase* NewBGM, float FadeDuration)
{
    if (!NewBGM) return;

    UAudioComponent* OldComp = GetActiveBGMComponent();
    UAudioComponent* NewComp = GetInactiveBGMComponent();

    if (!OldComp || !NewComp) return;

    // Fade out old
    if (OldComp->IsPlaying())
    {
        OldComp->FadeOut(FadeDuration, 0.f);
    }

    // Set up new and fade in
    NewComp->SetSound(NewBGM);
    NewComp->SetVolumeMultiplier(0.f);
    NewComp->Play();
    NewComp->FadeIn(FadeDuration, BGMVolume);

    // Swap active index
    ActiveBGMIndex = (ActiveBGMIndex == 0) ? 1 : 0;

    UE_LOG(LogMMOAudio, Log, TEXT("Crossfade BGM -> %s (%.1fs)"),
        *NewBGM->GetName(), FadeDuration);
}

void UAudioSubsystem::StopBGM(float FadeOut)
{
    if (BGMComponentA && BGMComponentA->IsPlaying())
        BGMComponentA->FadeOut(FadeOut, 0.f);
    if (BGMComponentB && BGMComponentB->IsPlaying())
        BGMComponentB->FadeOut(FadeOut, 0.f);
    CurrentBGMZone.Empty();
}

// ============================================================
// SFX Playback
// ============================================================

void UAudioSubsystem::PlayCombatSFX(const FString& SFXName, FVector Location)
{
    USoundBase* Sound = GetCombatSFX(SFXName);
    if (!Sound) return;

    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(), Sound, Location,
        SFXVolume, 1.0f, 0.f,
        CombatAttenuation);
}

void UAudioSubsystem::PlaySkillSFX(int32 SkillId, FVector Location)
{
    USoundBase* Sound = GetSkillSFX(SkillId);
    if (!Sound) return;

    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(), Sound, Location,
        SFXVolume, 1.0f, 0.f,
        CombatAttenuation);
}

void UAudioSubsystem::PlayUISFX(const FString& SFXName)
{
    USoundBase* Sound = GetUISFX(SFXName);
    if (!Sound) return;

    // UI sounds are non-positional
    UGameplayStatics::PlaySound2D(GetWorld(), Sound, SFXVolume);
}

void UAudioSubsystem::PlayAmbientSFX(USoundBase* Sound, FVector Location, float VolumeMultiplier)
{
    if (!Sound) return;

    UGameplayStatics::PlaySoundAtLocation(
        GetWorld(), Sound, Location,
        AmbientVolume * VolumeMultiplier, 1.0f, 0.f,
        AmbientAttenuation);
}

// ============================================================
// Volume Controls (saved to GameUserSettings.ini)
// ============================================================

void UAudioSubsystem::SetBGMVolume(float Volume)
{
    BGMVolume = FMath::Clamp(Volume, 0.f, 1.f);
    UAudioComponent* Active = GetActiveBGMComponent();
    if (Active && Active->IsPlaying())
    {
        Active->SetVolumeMultiplier(BGMVolume);
    }
    SaveVolumeSettings();
}

void UAudioSubsystem::SetSFXVolume(float Volume)
{
    SFXVolume = FMath::Clamp(Volume, 0.f, 1.f);
    SaveVolumeSettings();
}

void UAudioSubsystem::SetAmbientVolume(float Volume)
{
    AmbientVolume = FMath::Clamp(Volume, 0.f, 1.f);
    SaveVolumeSettings();
}

void UAudioSubsystem::SaveVolumeSettings()
{
    GConfig->SetFloat(TEXT("AudioSettings"), TEXT("BGMVolume"),
        BGMVolume, GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AudioSettings"), TEXT("SFXVolume"),
        SFXVolume, GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("AudioSettings"), TEXT("AmbientVolume"),
        AmbientVolume, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

void UAudioSubsystem::LoadVolumeSettings()
{
    GConfig->GetFloat(TEXT("AudioSettings"), TEXT("BGMVolume"),
        BGMVolume, GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AudioSettings"), TEXT("SFXVolume"),
        SFXVolume, GGameUserSettingsIni);
    GConfig->GetFloat(TEXT("AudioSettings"), TEXT("AmbientVolume"),
        AmbientVolume, GGameUserSettingsIni);
}

// ============================================================
// Asset Loading (lazy cache)
// ============================================================

USoundBase* UAudioSubsystem::GetBGMForZone(const FString& ZoneName) const
{
    const auto& Map = GetZoneBGMMap();
    const FString* Path = Map.Find(ZoneName);
    if (!Path) return nullptr;

    // Check cache first (mutable cast for lazy loading pattern)
    UAudioSubsystem* MutableThis = const_cast<UAudioSubsystem*>(this);
    TObjectPtr<USoundBase>* Cached = MutableThis->BGMCache.Find(ZoneName);
    if (Cached && Cached->Get()) return Cached->Get();

    USoundBase* Loaded = LoadObject<USoundBase>(nullptr, **Path);
    if (Loaded)
    {
        MutableThis->BGMCache.Add(ZoneName, Loaded);
    }
    return Loaded;
}

USoundBase* UAudioSubsystem::GetCombatSFX(const FString& SFXName) const
{
    UAudioSubsystem* MutableThis = const_cast<UAudioSubsystem*>(this);
    TObjectPtr<USoundBase>* Cached = MutableThis->CombatSFXCache.Find(SFXName);
    if (Cached && Cached->Get()) return Cached->Get();

    FString Path = FString::Printf(TEXT("/Game/SabriMMO/Audio/SFX/SB_SFX_%s"), *SFXName);
    USoundBase* Loaded = LoadObject<USoundBase>(nullptr, *Path);
    if (Loaded) MutableThis->CombatSFXCache.Add(SFXName, Loaded);
    return Loaded;
}

USoundBase* UAudioSubsystem::GetSkillSFX(int32 SkillId) const
{
    UAudioSubsystem* MutableThis = const_cast<UAudioSubsystem*>(this);
    TObjectPtr<USoundBase>* Cached = MutableThis->SkillSFXCache.Find(SkillId);
    if (Cached && Cached->Get()) return Cached->Get();

    const auto& Map = GetSkillSFXMap();
    const FString* Path = Map.Find(SkillId);
    if (!Path) return nullptr;

    USoundBase* Loaded = LoadObject<USoundBase>(nullptr, **Path);
    if (Loaded) MutableThis->SkillSFXCache.Add(SkillId, Loaded);
    return Loaded;
}

USoundBase* UAudioSubsystem::GetUISFX(const FString& SFXName) const
{
    UAudioSubsystem* MutableThis = const_cast<UAudioSubsystem*>(this);
    TObjectPtr<USoundBase>* Cached = MutableThis->UISFXCache.Find(SFXName);
    if (Cached && Cached->Get()) return Cached->Get();

    FString Path = FString::Printf(TEXT("/Game/SabriMMO/Audio/UI/SB_UI_%s"), *SFXName);
    USoundBase* Loaded = LoadObject<USoundBase>(nullptr, *Path);
    if (Loaded) MutableThis->UISFXCache.Add(SFXName, Loaded);
    return Loaded;
}

// ============================================================
// Socket Event Wrapping (follows BasicInfoSubsystem pattern)
// ============================================================

void UAudioSubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;

    TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
    if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

    if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update")))
        return;

    CachedSIOComponent = SIOComp;
    bEventsWrapped = true;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    // Start BGM for current zone
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
    {
        FString ZoneName = GI->CurrentZoneName;
        if (!ZoneName.IsEmpty())
        {
            PlayBGMForZone(ZoneName);
        }
    }

    UE_LOG(LogMMOAudio, Log, TEXT("AudioSubsystem events wrapped."));
}

USocketIOClientComponent* UAudioSubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
            return Comp;
    }
    return nullptr;
}
```

### 5.3 Integration Points

Call audio from existing subsystems:

```cpp
// In SkillVFXSubsystem::HandleSkillEffectDamage (after spawning VFX)
if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
{
    Audio->PlaySkillSFX(SkillId, TargetLocation);
}

// In ZoneTransitionSubsystem (after zone load complete)
if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
{
    Audio->PlayBGMForZone(NewZoneName);
}

// UI button click (from any Slate widget)
if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
{
    Audio->PlayUISFX(TEXT("Click"));
}
```

### 5.4 Sound Attenuation Settings Summary

| Category | Inner Radius | Falloff Distance | Spatialization |
|----------|-------------|------------------|----------------|
| Combat SFX | 200 UE units | 2000 UE units | 3D (Sphere) |
| Skill SFX | 200 UE units | 2000 UE units | 3D (Sphere) |
| Ambient SFX | 500 UE units | 5000 UE units | 3D (Sphere) |
| BGM | N/A | N/A | 2D (bIsUISound) |
| UI SFX | N/A | N/A | 2D (PlaySound2D) |

---

## 6. UI Art Theme

### 6.1 ROColors Namespace (Full Palette)

All Slate widgets use this shared namespace. Defined per-file (not a shared header currently). See `SBasicInfoWidget.cpp` for the canonical copy.

```cpp
// Use FLinearColor -- NOT FColor (FColor applies sRGB->linear conversion, too dark)
namespace ROColors
{
    // Panel backgrounds
    static const FLinearColor PanelBrown     (0.43f, 0.29f, 0.17f, 1.0f);  // Main window fill
    static const FLinearColor PanelDark      (0.22f, 0.14f, 0.08f, 1.0f);  // Title bar, insets
    static const FLinearColor PanelMedium    (0.33f, 0.22f, 0.13f, 1.0f);  // Secondary areas
    static const FLinearColor PanelLight     (0.50f, 0.36f, 0.22f, 1.0f);  // Hover highlight

    // Gold trim + highlights
    static const FLinearColor GoldTrim       (0.72f, 0.58f, 0.28f, 1.0f);  // Outer border
    static const FLinearColor GoldDark       (0.50f, 0.38f, 0.15f, 1.0f);  // Bar borders
    static const FLinearColor GoldHighlight  (0.92f, 0.80f, 0.45f, 1.0f);  // Active labels
    static const FLinearColor GoldDivider    (0.60f, 0.48f, 0.22f, 1.0f);  // Separators

    // Bar fills
    static const FLinearColor HPRed          (0.85f, 0.15f, 0.15f, 1.0f);
    static const FLinearColor SPBlue         (0.20f, 0.45f, 0.90f, 1.0f);
    static const FLinearColor EXPYellow      (0.90f, 0.75f, 0.10f, 1.0f);
    static const FLinearColor JobExpOrange   (0.90f, 0.55f, 0.10f, 1.0f);
    static const FLinearColor WeightGreen    (0.25f, 0.75f, 0.20f, 1.0f);
    static const FLinearColor BarBg          (0.10f, 0.07f, 0.04f, 1.0f);

    // Buttons
    static const FLinearColor ButtonNormal   (0.36f, 0.25f, 0.13f, 1.0f);
    static const FLinearColor ButtonHover    (0.48f, 0.36f, 0.23f, 1.0f);
    static const FLinearColor ButtonPressed  (0.24f, 0.18f, 0.12f, 1.0f);
    static const FLinearColor ButtonDisabled (0.30f, 0.25f, 0.20f, 0.6f);

    // Text
    static const FLinearColor TextPrimary    (0.96f, 0.90f, 0.78f, 1.0f);
    static const FLinearColor TextBright     (1.00f, 1.00f, 1.00f, 1.0f);
    static const FLinearColor TextGold       (0.95f, 0.82f, 0.48f, 1.0f);
    static const FLinearColor TextDisabled   (0.50f, 0.50f, 0.50f, 1.0f);
    static const FLinearColor TextShadow     (0.00f, 0.00f, 0.00f, 0.85f);

    // Slots, tabs, tooltips, close button
    static const FLinearColor SlotBg         (0.10f, 0.10f, 0.10f, 1.0f);
    static const FLinearColor SlotBorder     (0.33f, 0.33f, 0.33f, 1.0f);
    static const FLinearColor SlotHover      (0.50f, 0.40f, 0.20f, 0.5f);
    static const FLinearColor TabActive      (0.36f, 0.25f, 0.13f, 1.0f);
    static const FLinearColor TabInactive    (0.22f, 0.14f, 0.08f, 1.0f);
    static const FLinearColor TooltipBg      (0.00f, 0.00f, 0.00f, 0.85f);
    static const FLinearColor TooltipBorder  (0.50f, 0.40f, 0.20f, 1.0f);
    static const FLinearColor CloseNormal    (0.60f, 0.20f, 0.20f, 1.0f);
    static const FLinearColor CloseHover     (0.90f, 0.25f, 0.25f, 1.0f);
}
```

### 6.2 Standard Panel Border Pattern

Every RO-style window uses this 3-layer SBorder nesting:

```cpp
// Outer gold trim -> inner dark inset -> main brown panel
SNew(SBorder)
    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
    .BorderBackgroundColor(ROColors::GoldTrim)
    .Padding(FMargin(2.f))
    [
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(ROColors::PanelDark)
        .Padding(FMargin(1.f))
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(ROColors::PanelBrown)
            .Padding(FMargin(6.f))
            [
                // ... content ...
            ]
        ]
    ]
```

### 6.3 Standard Button Pattern

```cpp
SNew(SBorder)
    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
    .BorderBackgroundColor(ROColors::ButtonNormal)
    .HAlign(HAlign_Center).VAlign(VAlign_Center)
    .Padding(FMargin(12.f, 2.f))
    [
        SNew(STextBlock)
        .Text(FText::FromString(TEXT("Button Label")))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
        .ColorAndOpacity(FSlateColor(ROColors::GoldHighlight))
        .ShadowOffset(FVector2D(1, 1))
        .ShadowColorAndOpacity(ROColors::TextShadow)
    ]
```

### 6.4 Standard Text Style

| Use | Font Style | Size | Color |
|-----|-----------|------|-------|
| Title | Bold | 9 | `GoldHighlight` |
| Body | Regular | 8 | `TextPrimary` |
| Value/number | Bold | 7-8 | `TextBright` |
| Disabled | Regular | 8 | `TextDisabled` |
| Tooltip description | Italic | 8 | `TextPrimary` |
| All text shadow | -- | -- | `TextShadow` at offset (1,1) |

---

## 7. Asset Organization

### 7.1 Naming Convention Table

| Prefix | Asset Type | Example |
|--------|-----------|---------|
| `SK_` | Skeletal Mesh | `SK_Player_Male`, `SK_Poring` |
| `SM_` | Static Mesh | `SM_Sword_Dagger_01`, `SM_Tree_Oak` |
| `M_` | Material | `M_Player_Body`, `M_CelShade_Base` |
| `MI_` | Material Instance | `MI_Hair_Red`, `MI_CastingCircle` |
| `T_` | Texture | `T_Player_Body_D`, `T_Player_Body_N` |
| `NS_` | Niagara System | `NS_Lightning_Strike`, `NS_Magma_Shot` |
| `P_` | Cascade Particle (legacy) | `P_Ice_Proj_charge_01` |
| `NE_` | Niagara Emitter | `NE_attack01` |
| `A_` | Animation Sequence | `A_Idle_Unarmed`, `A_Walk_Forward` |
| `AM_` | Animation Montage | `AM_Atk_Dagger_01`, `AM_Death_01` |
| `ABP_` | Animation Blueprint | `ABP_Player`, `ABP_Monster_Base` |
| `BS_` | Blend Space | `BS_Locomotion_2D` |
| `SB_` | Sound Base (Cue/Wave) | `SB_BGM_Prontera`, `SB_SFX_Bash` |
| `SA_` | Sound Attenuation | `SA_Combat`, `SA_Ambient` |
| `SC_` | Sound Concurrency | `SC_BGM`, `SC_SFX_Combat` |
| `BP_` | Blueprint Actor | `BP_MMOCharacter`, `BP_EnemyManager` |
| `WBP_` | Widget Blueprint | `WBP_MainMenu` |
| `BPI_` | Blueprint Interface | `BPI_Damageable`, `BPI_Targetable` |
| `L_` | Level / Map | `L_PrtSouth`, `L_MjoDun01` |
| `DT_` | Data Table | `DT_MonsterTemplates`, `DT_ItemData` |
| `E_` | Enum (Blueprint) | `E_WeaponType`, `E_Element` |
| `S_` | Struct (Blueprint) | `S_SkillData` |

Texture suffixes: `_D` (Diffuse/Base Color), `_N` (Normal), `_R` (Roughness), `_M` (Metallic), `_E` (Emissive), `_AO` (Ambient Occlusion).

### 7.2 Content Browser Folder Structure

```
Content/SabriMMO/
    Anims/
        Player/           # A_, AM_, ABP_, BS_ for player character
        Monsters/         # Per-monster-family subfolders
            Poring/
            Lunatic/
        Shared/           # Shared hit reacts, death anims
    Audio/
        BGM/              # SB_BGM_* (one per zone or zone group)
        SFX/              # SB_SFX_* (combat, skill sounds)
        UI/               # SB_UI_* (click, hover, equip, level up)
        Ambient/          # SB_AMB_* (wind, rain, crowd)
    Blueprints/
        Characters/       # BP_MMOCharacter, BP_OtherPlayer
        Enemies/          # BP_Enemy, BP_EnemyManager
        NPCs/             # BP_KafraNPC, BP_ShopNPC
        Core/             # BP_SocketManager, GM_MMOGameMode
        Interfaces/       # BPI_Damageable, BPI_Interactable, BPI_Targetable
    Levels/
        L_PrtSouth.umap
        L_PrtNorth.umap
        L_Login.umap
    Materials/
        Characters/       # M_Player_Body, M_Hair_Base, MI_Hair_*
        Environment/      # M_Terrain_Grass, M_Water
        PostProcess/      # M_PP_CelShade, M_PP_Outline
        UI/               # MI_CastingCircle
    Meshes/
        Characters/       # SK_Player_Male, SK_Player_Female
        Hair/             # SK_Hair_Male_01 through SK_Hair_Male_19
        Weapons/          # SM_Dagger_01, SM_Sword_01
        Equipment/        # SK_Armor_Novice, SK_Headgear_*
        Environment/      # SM_Tree_*, SM_Rock_*, SM_Building_*
    Textures/
        Characters/       # T_Player_Body_D, T_Player_Body_N
        Environment/
        UI/               # T_Icon_Potion, T_Slot_Empty
    VFX/
        Niagara/          # NS_*, NE_* (Niagara systems and emitters)
        Cascade/          # P_* (legacy Cascade only for scale-aware effects)
    Widgets/              # WBP_* (if any UMG used for non-HUD menus)
```

### 7.3 Asset References from C++ (Loading Patterns)

| Pattern | When to Use | Code |
|---------|-------------|------|
| Hard reference | Small, always-needed assets (< 5 per class) | `UPROPERTY() TObjectPtr<USkeletalMesh> Mesh;` |
| Soft reference | Large assets loaded on demand | `TSoftObjectPtr<UNiagaraSystem> Effect;` then `Effect.LoadSynchronous()` |
| Runtime LoadObject | Data-driven paths from config tables | `LoadObject<USoundBase>(nullptr, *Path)` |
| Async loading | BGM, large VFX (avoid hitches) | `UAssetManager::GetStreamableManager().RequestAsyncLoad(...)` |

**Rule**: BGM and large VFX use soft references or async loading. SFX and small meshes can use `LoadObject`. Never hard-reference more than 5 assets per class to avoid bloating cook times.
