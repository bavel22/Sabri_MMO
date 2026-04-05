// EnemySubsystem.h — Manages enemy entity lifecycle: spawn, move, death, health, attack.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_EnemyManager.
// Struct refactor: FEnemyEntry holds typed data alongside actor pointer.
// Also manages the Sense skill result popup (skill:sense_result).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Sprite/SpriteAtlasData.h"
#include "Dom/JsonValue.h"
#include "Widgets/SWidget.h"
#include "EnemySubsystem.generated.h"

class SSenseResultPopup;
class ASpriteCharacterActor;

// ============================================================
// Enemy entity data (C++ owned, readable by any subsystem)
// ============================================================

struct FEnemyEntry
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<ASpriteCharacterActor> SpriteActor;
	int32 EnemyId = 0;
	FString EnemyName;
	FString SpriteClass;          // atlas manifest name (e.g. "skeleton")
	int32 WeaponMode = 0;        // 0=unarmed, 1=onehand, 2=twohand, 3=bow
	int32 EnemyLevel = 0;
	double Health = 0.0;
	double MaxHealth = 0.0;
	bool bIsDead = false;
};

UCLASS()
class SABRIMMO_API UEnemySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Public lookups (used by all subsystems) ----

	// Get actor by enemy ID (returns nullptr if not found or stale)
	AActor* GetEnemy(int32 EnemyId) const;

	// Get full typed data by enemy ID (returns nullptr if not found)
	const FEnemyEntry* GetEnemyData(int32 EnemyId) const;

	// Reverse lookup: given an actor, return its enemy ID (0 if not an enemy)
	int32 GetEnemyIdFromActor(AActor* Actor) const;

	// Get the full registry (for iteration by WorldHealthBarSubsystem etc.)
	const TMap<int32, FEnemyEntry>& GetAllEnemies() const { return Enemies; }

	// Sense popup management (called by SSenseResultPopup::DismissPopup)
	void HideSensePopup();

private:
	// Entity registry: server enemy ID -> FEnemyEntry (actor + typed data)
	TMap<int32, FEnemyEntry> Enemies;

	// Reverse lookup cache: actor -> enemy ID (rebuilt on spawn/death)
	TMap<TWeakObjectPtr<AActor>, int32> ActorToEnemyId;

	// Cached BP class for spawning (loaded at runtime)
	// UPROPERTY prevents GC from collecting the loaded UClass while this subsystem holds a reference.
	UPROPERTY()
	UClass* EnemyBPClass = nullptr;

	// Readiness guard (prevents ProcessEvent during PostLoad)
	bool bReadyToProcess = false;

	// Socket event handlers
	void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatKnockback(const TSharedPtr<FJsonValue>& Data);
	void HandleSenseResult(const TSharedPtr<FJsonValue>& Data);

	// Sense popup state
	TSharedPtr<SSenseResultPopup> SensePopup;
	TSharedPtr<SWidget> SenseAlignWrapper;
	TSharedPtr<SWidget> SenseOverlay;
	bool bSensePopupVisible = false;

	void ShowSensePopup(const TSharedPtr<FJsonValue>& Data);
};
