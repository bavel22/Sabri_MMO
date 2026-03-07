// CastingCircleActor.h — Spawnable actor that displays a rotating magical
// circle on the ground via a DecalComponent. Used for RO-style cast indicators.
// Color and radius are driven at runtime through a dynamic material instance.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CastingCircleActor.generated.h"

class UDecalComponent;

UCLASS()
class SABRIMMO_API ACastingCircleActor : public AActor
{
	GENERATED_BODY()

public:
	ACastingCircleActor();

	/**
	 * Configure the circle after spawn.
	 * @param ElementColor  Tint colour (element-based).
	 * @param Radius        Decal half-size in UE units.
	 * @param Duration      Expected cast time in seconds (auto-destroys after this + fade).
	 */
	void Initialize(FLinearColor ElementColor, float Radius, float Duration);

	/** Start fading out and destroy when done. */
	void FadeOut(float FadeTime = 0.3f);

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	UDecalComponent* CircleDecal;

	/** Base material to create dynamic instance from. Set in editor or via subsystem. */
	UPROPERTY(EditAnywhere, Category = "VFX")
	UMaterialInterface* BaseMaterial;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* DynMaterial;

	FTimerHandle FadeTimerHandle;
	FTimerHandle AutoDestroyHandle;

	float FadeAlpha = 0.f;
	float FadeDirection = 1.f;   // +1 = fading in, -1 = fading out
	float FadeSpeed = 4.f;      // alpha units per second

	void TickFade();
};
