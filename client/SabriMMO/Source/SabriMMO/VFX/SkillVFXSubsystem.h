// SkillVFXSubsystem.h — UWorldSubsystem that spawns Niagara VFX for skill effects.
// Wraps Socket.io events (skill:cast_start, skill:effect_damage, skill:buff_applied/removed)
// and dispatches to parameterised Niagara templates based on SkillVFXData configs.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "SkillVFXData.h"
#include "SkillVFXSubsystem.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class ACastingCircleActor;

UCLASS()
class SABRIMMO_API USkillVFXSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- manual spawning (for testing from console / Blueprint) ----
	UFUNCTION(BlueprintCallable, Category = "SkillVFX")
	void SpawnTestEffect(ESkillVFXTemplate Template, FVector Location, FLinearColor Color);

	/**
	 * Spawn a looping ground effect at a location (used by WarpPortal actors).
	 * Returns the Niagara component so the caller can deactivate it later.
	 * Returns nullptr if no suitable Niagara system is loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "SkillVFX")
	UNiagaraComponent* SpawnLoopingPortalEffect(FVector Location);

	// ---- toggle effects (like RO's /effect command) ----
	UFUNCTION(BlueprintCallable, Category = "SkillVFX")
	void SetEffectsEnabled(bool bEnabled) { bVFXEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "SkillVFX")
	bool AreEffectsEnabled() const { return bVFXEnabled; }

private:
	// ---- event handlers ----
	void HandleCastStart(const TSharedPtr<FJsonValue>& Data);
	void HandleCastComplete(const TSharedPtr<FJsonValue>& Data);
	void HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data);
	void HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleGroundEffectCreated(const TSharedPtr<FJsonValue>& Data);
	void HandleGroundEffectRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatHealthUpdate(const TSharedPtr<FJsonValue>& Data);

	// ---- VFX spawning per template type ----
	void SpawnBoltFromSky(FVector TargetLocation, const FSkillVFXConfig& Config, int32 TotalHits);
	void SpawnProjectileEffect(FVector AttackerLocation, FVector TargetLocation, const FSkillVFXConfig& Config);
	void SpawnMultiHitProjectile(FVector AttackerLocation, FVector TargetLocation, const FSkillVFXConfig& Config, int32 TotalHits);
	void SpawnAoEImpact(FVector Location, const FSkillVFXConfig& Config);
	void SpawnGroundPersistent(FVector Location, const FSkillVFXConfig& Config, int32 SkillId);
	void SpawnGroundAoERain(FVector Location, const FSkillVFXConfig& Config, int32 HitNumber);
	void SpawnSelfBuff(AActor* TargetActor, const FSkillVFXConfig& Config, int32 SkillId, int32 TargetId);
	void SpawnTargetDebuff(AActor* TargetActor, const FSkillVFXConfig& Config, int32 SkillId);
	void SpawnHealFlash(FVector Location, const FSkillVFXConfig& Config);

	// ---- casting circle ----
	void SpawnCastingCircle(int32 CasterId, FVector Location, const FSkillVFXConfig& Config, float CastDuration);
	void RemoveCastingCircle(int32 CasterId);

	// ---- generic Niagara spawn helpers ----
	UNiagaraComponent* SpawnNiagaraAtLocation(UNiagaraSystem* System, FVector Location,
		FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector::OneVector);
	UNiagaraComponent* SpawnNiagaraAttached(UNiagaraSystem* System, USceneComponent* AttachTo);
	void SetNiagaraColor(UNiagaraComponent* Comp, FLinearColor Color);
	void SetNiagaraScale(UNiagaraComponent* Comp, float Scale);

	// ---- actor lookup ----
	AActor* FindEnemyActorById(int32 EnemyId) const;
	AActor* FindPlayerActorById(int32 PlayerId) const;
	FVector GetActorLocationById(int32 Id, bool bIsEnemy) const;

	// ---- Niagara system references ----
	// These are loaded by soft path in Initialize. If the asset doesn't exist yet,
	// the pointer stays null and that template type just doesn't spawn effects.
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_BoltFromSky;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_Projectile;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_AoEImpact;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_GroundPersistent;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_GroundAoERain;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_SelfBuff;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_TargetDebuff;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_HealFlash;

	/** Niagara-based casting circle (from Free_Magic pack). Used instead of decal if available. */
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NS_CastingCircle;

	// ---- casting circle material ----
	UPROPERTY()
	TObjectPtr<UMaterialInterface> MI_CastingCircle;

	// ---- per-skill VFX override cache (avoids repeated LoadObject calls) ----
	UPROPERTY()
	TMap<FString, TObjectPtr<UNiagaraSystem>> NiagaraOverrideCache;
	UPROPERTY()
	TMap<FString, TObjectPtr<UParticleSystem>> CascadeOverrideCache;
	UNiagaraSystem* GetOrLoadNiagaraOverride(const FString& Path);
	UParticleSystem* GetOrLoadCascadeOverride(const FString& Path);

	// ---- unified VFX spawning (handles both Niagara and Cascade) ----
	void SpawnVFXAtLocation(const FSkillVFXConfig& Config, FVector Location,
		FRotator Rotation = FRotator::ZeroRotator, FVector Scale = FVector::OneVector);
	void SpawnVFXAttached(const FSkillVFXConfig& Config, USceneComponent* AttachTo);

	// ---- active effect tracking ----
	TMap<int32, TWeakObjectPtr<ACastingCircleActor>> ActiveCastingCircles; // CasterId → Circle

	// Persistent ground effects keyed by a compound ID (SkillId * 100000 + unique counter)
	TMap<int32, TWeakObjectPtr<UNiagaraComponent>> ActivePersistentEffects;
	int32 PersistentEffectCounter = 0;

	// Buff auras: key = TargetId * 10000 + SkillId
	TMap<int64, TWeakObjectPtr<UNiagaraComponent>> ActiveBuffAuras;

	// Cascade buff auras (legacy particle system): same key scheme
	TMap<int64, TWeakObjectPtr<UParticleSystemComponent>> ActiveCascadeBuffs;

	// Timer that re-activates non-looping Cascade buff particles so they persist until buff removal
	FTimerHandle CascadeLoopTimer;

	// Dedup for bSingleProjectile skills (Fire Ball): key = AttackerId*10000+SkillId → timestamp
	TMap<int64, double> SingleProjectileLastSpawnTime;

	// ---- state ----
	bool bVFXEnabled = true;
	int32 LocalCharacterId = 0;

	// Guard: prevents actor access / component spawning during PostLoad.
	// Set to true one frame after OnWorldBeginPlay via SetTimerForNextTick.
	bool bReadyToProcess = false;
};

