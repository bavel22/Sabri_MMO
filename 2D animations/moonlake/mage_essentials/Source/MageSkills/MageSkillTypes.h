// MageSkillTypes.h
// Mage Essentials - common types and DataTable rows.
// Drop into your Game module (e.g. MyGame/Public/Skills/), and add Engine, CoreUObject,
// GameplayTags, Niagara, Paper2D to your .Build.cs PublicDependencyModuleNames if needed.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "PaperFlipbook.h"
#include "NiagaraSystem.h"
#include "MageSkillTypes.generated.h"

UENUM(BlueprintType)
enum class EMageElement : uint8
{
    Neutral, Fire, Water, Wind, Earth
};

UENUM(BlueprintType)
enum class EMageSkillCategory : uint8
{
    Targeted,   // Bolts: needs an actor target
    Ground      // Fire Wall: needs a tile/ground location
};

/**
 * One row per (SkillId, Level). The DataTable RowName convention is:
 *   FIRE_BOLT_01, FIRE_BOLT_02, ..., COLD_BOLT_10, FIRE_WALL_05, etc.
 * This matches MageSkills.csv shipped with the pack.
 */
USTRUCT(BlueprintType)
struct FMageSkillRow : public FTableRowBase
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMageElement Element = EMageElement::Neutral;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMageSkillCategory Category = EMageSkillCategory::Targeted;

    // Cost / cadence
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SP = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float CastTime = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float GlobalCooldown = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DelayAfter = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Range = 9.f; // tiles

    // Targeted-bolt fields
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BoltCount = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoltInterval = 0.5f;

    // Ground-AoE fields (Fire Wall)
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WidthCells = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HitsPerCell = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float LifetimeSeconds = 5.f;

    // Damage
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageMultiplier = 1.0f;

    // VFX assets (assign in editor; sheets live under Content/MageEssentials/SpriteSheets/)
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperFlipbook> CastGlowFlipbook;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperFlipbook> ProjectileFlipbook;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UPaperFlipbook> ImpactFlipbook;

    // Optional Niagara variants (used instead of Paper2D when set)
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UNiagaraSystem> CastGlowNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UNiagaraSystem> ProjectileNiagara;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UNiagaraSystem> ImpactNiagara;
};
