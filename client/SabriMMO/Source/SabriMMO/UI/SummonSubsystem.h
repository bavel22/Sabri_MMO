// SummonSubsystem.h — UWorldSubsystem that tracks summoned Flora plants
// and Marine Spheres via socket events. Renders visual indicators via
// SSummonOverlay (OnPaint overlay — no Blueprint actors needed).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "SummonSubsystem.generated.h"

class SSummonOverlay;

// Runtime data for a summoned plant
struct FSummonedPlant
{
	int32 PlantId = 0;
	int32 OwnerId = 0;
	FString OwnerName;
	FString Type;           // "Mandragora", "Hydra", etc.
	int32 MonsterId = 0;
	FVector Position = FVector::ZeroVector;
	int32 HP = 0;
	int32 MaxHP = 0;
	float Duration = 0.f;  // seconds
	double SpawnTime = 0.0;
};

// Runtime data for a summoned marine sphere
struct FSummonedSphere
{
	int32 SphereId = 0;
	int32 OwnerId = 0;
	FString OwnerName;
	FVector Position = FVector::ZeroVector;
	int32 HP = 0;
	int32 MaxHP = 0;
	double SpawnTime = 0.0;
};

UCLASS()
class SABRIMMO_API USummonSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Public data (read by SSummonOverlay in OnPaint)
	TMap<int32, FSummonedPlant> Plants;     // plantId -> plant data
	TMap<int32, FSummonedSphere> Spheres;   // sphereId -> sphere data
	int32 LocalCharacterId = 0;

	// Emit manual sphere detonation
	void DetonateSphere(int32 SphereId);

	// Project a world position to screen (used by overlay)
	bool ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;

private:
	// Socket event handlers
	void HandlePlantSpawned(const TSharedPtr<FJsonValue>& Data);
	void HandlePlantRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandlePlantAttack(const TSharedPtr<FJsonValue>& Data);
	void HandleSphereSpawned(const TSharedPtr<FJsonValue>& Data);
	void HandleSphereExploded(const TSharedPtr<FJsonValue>& Data);
	void HandleSphereRemoved(const TSharedPtr<FJsonValue>& Data);

	// Overlay widget (full-viewport OnPaint)
	TSharedPtr<SSummonOverlay> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;
	bool bOverlayAdded = false;
	void ShowOverlay();
};
