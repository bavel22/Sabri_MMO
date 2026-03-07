// WarpPortal.h — Invisible warp trigger at zone edges.
// When the player overlaps, emits zone:warp to server.
// Place in level, set WarpId per instance (must match ro_zone_data.js).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarpPortal.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class SABRIMMO_API AWarpPortal : public AActor
{
	GENERATED_BODY()

public:
	AWarpPortal();

	/** Must match a warpId in the server's ZONE_REGISTRY for the current zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Portal")
	FString WarpId;

	/** Radius of the overlap trigger (UE units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warp Portal")
	float TriggerRadius = 200.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* TriggerComp;

	/** Niagara particle effect for the portal visual (golden ring + spiraling particles). */
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	UNiagaraComponent* PortalEffect;

	/** Niagara system asset to use for portal VFX. Assign in Blueprint or default. */
	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* PortalVFXSystem;

private:
	double LastWarpTime = 0.0;

	/** Deferred VFX initialization — waits for SkillVFXSubsystem to load Niagara assets. */
	void TryActivatePortalVFX();
	FTimerHandle VFXRetryTimer;
	int32 VFXRetryCount = 0;

	/** Niagara component spawned by the subsystem (not the built-in one). */
	UPROPERTY()
	TWeakObjectPtr<UNiagaraComponent> SpawnedPortalVFX;
};
