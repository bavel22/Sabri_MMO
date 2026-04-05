// OtherPlayerSubsystem.h — Manages other player entity lifecycle: spawn, move, leave.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_OtherPlayerManager.
// Struct refactor: FPlayerEntry holds typed data alongside actor pointer.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "OtherPlayerSubsystem.generated.h"

// ============================================================
// Other player entity data (C++ owned, readable by any subsystem)
// ============================================================

class ASpriteCharacterActor;

struct FPlayerEntry
{
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<ASpriteCharacterActor> SpriteActor;
	int32 CharacterId = 0;
	FString PlayerName;
	FString JobClass;
	FString Gender;
	int32 HairStyle = 1;
	int32 HairColor = 0;
	bool bIsVending = false;
	TMap<FString, int32> EquipVisuals;  // slot → view_sprite for equipment layers
};

UCLASS()
class SABRIMMO_API UOtherPlayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Public lookups (used by all subsystems) ----

	// Get actor by character ID (returns nullptr if not found or stale)
	AActor* GetPlayer(int32 CharacterId) const;

	// Get full typed data by character ID (returns nullptr if not found)
	const FPlayerEntry* GetPlayerData(int32 CharacterId) const;

	// Reverse lookup: given an actor, return its character ID (0 if not an other-player)
	int32 GetPlayerIdFromActor(AActor* Actor) const;

	// Get player name for an actor (empty string if not found)
	FString GetPlayerNameFromActor(AActor* Actor) const;

	// Check if a player is currently hidden (Hiding/Cloaking)
	bool IsPlayerHidden(int32 CharacterId) const;

	// Get all players (for minimap iteration)
	const TMap<int32, FPlayerEntry>& GetAllPlayers() const { return Players; }

private:
	// Entity registry: server character ID -> FPlayerEntry (actor + typed data)
	TMap<int32, FPlayerEntry> Players;

	// Reverse lookup cache: actor -> character ID
	TMap<TWeakObjectPtr<AActor>, int32> ActorToPlayerId;

	// Players currently in Hiding/Cloaking (hidden from view)
	TSet<int32> HiddenPlayerIds;

	// Cached BP class for spawning (loaded at runtime)
	// UPROPERTY prevents GC from collecting the loaded UClass while this subsystem holds a reference.
	UPROPERTY()
	UClass* PlayerBPClass = nullptr;

	// Local player ID — filtered from player:moved to avoid spawning self
	int32 LocalCharacterId = 0;

	// Readiness guard (prevents ProcessEvent during PostLoad)
	bool bReadyToProcess = false;

	// Socket event handlers
	void HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerAppearance(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingShopOpened(const TSharedPtr<FJsonValue>& Data);
	void HandleVendingShopClosed(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data);

	// Toggle actor + name tag visibility for a player
	void SetPlayerVisibility(int32 CharacterId, bool bVisible);
};
