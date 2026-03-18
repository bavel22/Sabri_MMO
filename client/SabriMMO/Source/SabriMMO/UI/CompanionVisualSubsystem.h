// CompanionVisualSubsystem.h — Manages visual placeholders for cart, mount, and falcon.
// Listens to player:stats for isMounted/hasCart/hasFalcon flags and spawns/despawns
// simple geometry actors (ACompanionVisualActor) accordingly.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "CompanionVisualSubsystem.generated.h"

class ACompanionVisualActor;

UCLASS()
class SABRIMMO_API UCompanionVisualSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void HandlePlayerStats(const TSharedPtr<FJsonValue>& Data);
	void TickVisuals();

	void SpawnCart();
	void DespawnCart();
	void SpawnMount();
	void DespawnMount();
	void SpawnFalcon();
	void DespawnFalcon();

	bool bHasCart = false;
	bool bIsMounted = false;
	bool bHasFalcon = false;

	UPROPERTY()
	ACompanionVisualActor* CartActor = nullptr;

	UPROPERTY()
	ACompanionVisualActor* MountActor = nullptr;

	UPROPERTY()
	ACompanionVisualActor* FalconActor = nullptr;

	FTimerHandle VisualTickHandle;
};
