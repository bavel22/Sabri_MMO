// WarpPortal.h — Invisible warp trigger at zone edges.
// When the player overlaps, emits zone:warp to server.
// Place in level, set WarpId per instance (must match ro_zone_data.js).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarpPortal.generated.h"

class USphereComponent;

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

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* TriggerComp;

private:
	double LastWarpTime = 0.0;
};
