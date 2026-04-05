// PostProcessSubsystem.h — RO Classic visual style: auto-lighting, post-process,
// per-zone color grading, and runtime master environment material.
// Spawns DirectionalLight + SkyLight + HeightFog + PPVolume automatically.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PostProcessSubsystem.generated.h"

class APostProcessVolume;
class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class UMaterial;

UCLASS()
class SABRIMMO_API UPostProcessSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Re-apply zone preset (e.g., after zone transition completes) */
	void ApplyZonePreset(const FString& ZoneName);

private:
	void SetupPostProcessVolume();
	void SetupSceneLighting(const FString& ZoneName);
	void CreateEnvironmentMaterial();
	void ApplyEnvironmentMaterial();

	UPROPERTY()
	APostProcessVolume* PPVolume = nullptr;

	UPROPERTY()
	ADirectionalLight* SunLight = nullptr;

	UPROPERTY()
	ASkyLight* AmbientLight = nullptr;

	UPROPERTY()
	AExponentialHeightFog* HeightFog = nullptr;

	/** Runtime master environment material (diffuse-only, fully rough, warm tint) */
	UPROPERTY()
	UMaterial* EnvMaterial = nullptr;

	FString CurrentZone;
};
