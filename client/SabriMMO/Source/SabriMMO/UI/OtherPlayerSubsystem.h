// OtherPlayerSubsystem.h — Manages other player entity lifecycle: spawn, move, leave.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_OtherPlayerManager.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "OtherPlayerSubsystem.generated.h"

UCLASS()
class SABRIMMO_API UOtherPlayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Public lookup — returns nullptr if not found or stale.
	// Used by CombatActionSubsystem for actor resolution.
	AActor* GetPlayer(int32 CharacterId) const;

private:
	// Entity registry: server character ID -> spawned BP_OtherPlayerCharacter actor
	TMap<int32, TWeakObjectPtr<AActor>> Players;

	// Cached BP class for spawning (loaded at runtime)
	UClass* PlayerBPClass = nullptr;

	// Local player ID — filtered from player:moved to avoid spawning self
	int32 LocalCharacterId = 0;

	// Readiness guard (prevents ProcessEvent during PostLoad)
	bool bReadyToProcess = false;

	// Socket event handlers
	void HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data);
};
