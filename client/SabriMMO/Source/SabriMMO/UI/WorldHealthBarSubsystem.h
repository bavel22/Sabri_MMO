// WorldHealthBarSubsystem.h — UWorldSubsystem that manages RO-style floating
// HP/SP bars below the local player and enemy characters.
// Wraps Socket.io events to track health data and positions.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "WorldHealthBarSubsystem.generated.h"

class SWorldHealthBarOverlay;

// Per-NPC name label state (screen-space text above NPC actors)
struct FNPCNameData
{
	FString DisplayName;
	TWeakObjectPtr<AActor> Actor;
};

// Per-enemy health bar state
struct FEnemyBarData
{
	int32 EnemyId = 0;
	int32 CurrentHP = 0;
	int32 MaxHP = 1;
	bool bBarVisible = false;   // shown when enemy is in combat / damaged
	bool bIsDead = false;
	FVector WorldPosition = FVector::ZeroVector;

	// Cached reference to the actual enemy actor in the world.
	// Used for real-time position tracking (smooth, no lag).
	TWeakObjectPtr<AActor> CachedActor;
};

UCLASS()
class SABRIMMO_API UWorldHealthBarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- Local player health data ----
	int32 PlayerCurrentHP = 0;
	int32 PlayerMaxHP = 100;
	int32 PlayerCurrentSP = 0;
	int32 PlayerMaxSP = 100;
	bool bPlayerDead = false;

	// ---- Enemy health data (keyed by enemyId) ----
	TMap<int32, FEnemyBarData> EnemyHealthMap;

	// ---- NPC name labels (screen-space text above NPC actors) ----
	TArray<FNPCNameData> NPCNames;

	// ---- Lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- World-to-screen projection (called by overlay widget) ----
	bool ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;

	// Get the local player pawn's world location (feet level)
	bool GetPlayerFeetPosition(FVector& OutPos) const;

	// Get an enemy's feet position from cached actor (smooth) or fallback to socket position
	bool GetEnemyFeetPosition(const FEnemyBarData& Enemy, FVector& OutPos) const;

private:
	// ---- Event handlers ----
	void HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerStats(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyMove(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data);

	void PopulateFromGameInstance();

	// ---- Overlay management ----
	void ShowOverlay();
	void HideOverlay();

	// ---- Actor caching ----
	void CacheEnemyActors();
	void CacheNPCActors();

	// ---- State ----
	bool bOverlayAdded = false;
	int32 LocalCharacterId = 0;

	FTimerHandle ActorCacheTimer;

	TSharedPtr<SWorldHealthBarOverlay> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;
};
