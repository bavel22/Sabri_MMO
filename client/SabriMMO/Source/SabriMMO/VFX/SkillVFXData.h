// SkillVFXData.h — Data structures mapping skill IDs to VFX configurations.
// Each implemented skill has a FSkillVFXConfig defining which Niagara template
// to use, what colors to apply, and behavioral parameters.

#pragma once

#include "CoreMinimal.h"
#include "SkillVFXData.generated.h"

/** Which parameterised Niagara template to spawn for a skill. */
UENUM(BlueprintType)
enum class ESkillVFXTemplate : uint8
{
	None            UMETA(DisplayName = "None"),
	BoltFromSky     UMETA(DisplayName = "Bolt From Sky"),       // Cold/Fire/Lightning Bolt
	Projectile      UMETA(DisplayName = "Projectile"),           // Soul Strike, Fire Ball, Frost Diver
	AoEImpact       UMETA(DisplayName = "AoE Impact"),           // Magnum Break, Fire Ball explosion, Napalm Beat
	GroundPersistent UMETA(DisplayName = "Ground Persistent"),   // Fire Wall, Safety Wall
	GroundAoERain   UMETA(DisplayName = "Ground AoE Rain"),      // Thunderstorm
	SelfBuff        UMETA(DisplayName = "Self Buff"),             // Endure, Energy Coat, Sight
	TargetDebuff    UMETA(DisplayName = "Target Debuff"),         // Provoke, Stone Curse, Frost Diver freeze
	HealFlash       UMETA(DisplayName = "Heal Flash"),            // First Aid
	WarpPortal      UMETA(DisplayName = "Warp Portal")            // Warp portal effect
};

/** Per-skill VFX configuration. */
USTRUCT(BlueprintType)
struct FSkillVFXConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SkillId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ESkillVFXTemplate Template = ESkillVFXTemplate::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor SecondaryColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Scale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Duration = 0.5f;

	/** Whether to show a casting circle during cast time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bUseCastingCircle = false;

	/** Casting circle color (element-based). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor CastingCircleColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);

	/** Casting circle radius in UE units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float CastingCircleRadius = 200.0f;

	/** For bolt skills: number of bolts per cast (= skill level at runtime). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 BoltCount = 1;

	/** Seconds between bolt hits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BoltInterval = 0.15f;

	/** For projectile skills: travel speed in UE units/sec. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ProjectileSpeed = 2000.f;

	/** Whether the effect loops (Fire Wall, Safety Wall, Sight). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLooping = false;

	/** Element name from server (fire, water, wind, earth, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Element;

	/** AoE radius for ground effects (Thunderstorm, Magnum Break). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AoERadius = 0.f;

	/** Spawn height above target for bolt effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BoltSpawnHeight = 500.f;

	/** Per-skill VFX override path (Niagara NS_ or Cascade P_). */
	FString VFXOverridePath;

	/** If true, the override is a Cascade (UParticleSystem) effect, not Niagara. */
	bool bIsCascade = false;

	/** If true, AoE effect spawns at caster position, not target (e.g. Magnum Break). */
	bool bSelfCentered = false;

	/** How long Cascade effects should persist before forced cleanup (seconds).
	 *  Cascade effects with looping emitters never auto-destroy, so this is required.
	 *  0 = use default (0.5s for one-shot, 10s for persistent). */
	float CascadeLifetime = 0.f;
};

/**
 * Static helper to get VFX config for a skill ID.
 * Returns a config with Template=None for unknown skills.
 */
namespace SkillVFXDataHelper
{
	inline FLinearColor GetElementColor(const FString& Element)
	{
		if (Element == TEXT("fire"))    return FLinearColor(1.0f, 0.3f, 0.05f, 1.0f);
		if (Element == TEXT("water"))   return FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
		if (Element == TEXT("wind"))    return FLinearColor(0.9f, 1.0f, 0.3f, 1.0f);
		if (Element == TEXT("earth"))   return FLinearColor(0.6f, 0.4f, 0.2f, 1.0f);
		if (Element == TEXT("holy"))    return FLinearColor(1.0f, 1.0f, 0.8f, 1.0f);
		if (Element == TEXT("dark"))    return FLinearColor(0.3f, 0.0f, 0.5f, 1.0f);
		if (Element == TEXT("ghost"))   return FLinearColor(0.6f, 0.3f, 0.9f, 1.0f);
		if (Element == TEXT("undead"))  return FLinearColor(0.2f, 0.0f, 0.3f, 1.0f);
		if (Element == TEXT("poison"))  return FLinearColor(0.3f, 0.8f, 0.1f, 1.0f);
		return FLinearColor(0.3f, 0.8f, 1.0f, 1.0f); // neutral / default cyan
	}

	/** Declared in header, defined in SkillVFXData.cpp to avoid Live Coding issues with inline statics. */
	const FSkillVFXConfig& GetSkillVFXConfig(int32 SkillId);
}
