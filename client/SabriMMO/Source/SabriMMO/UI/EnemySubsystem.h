// EnemySubsystem.h — Manages enemy entity lifecycle: spawn, move, death, health, attack.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_EnemyManager.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "EnemySubsystem.generated.h"

UCLASS()
class SABRIMMO_API UEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Public lookup — returns nullptr if not found or stale.
	// Used by CombatActionSubsystem for actor resolution.
	AActor* GetEnemy(int32 EnemyId) const;

private:
	// Entity registry: server enemy ID -> spawned BP_EnemyCharacter actor
	TMap<int32, TWeakObjectPtr<AActor>> Enemies;

	// Cached BP class for spawning (loaded at runtime)
	UClass* EnemyBPClass = nullptr;

	// Readiness guard (prevents ProcessEvent during PostLoad)
	bool bReadyToProcess = false;

	// Socket event handlers
	void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data);
};
