#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "GroundItemSubsystem.generated.h"

class AGroundItemActor;
class UMMOGameInstance;
class USocketEventRouter;

/** Data for a single ground item. */
USTRUCT()
struct FGroundItemEntry
{
	GENERATED_BODY()

	UPROPERTY() int32 GroundItemId = 0;
	UPROPERTY() int32 ItemId = 0;
	FString ItemName;
	int32 Quantity = 1;
	FString Icon;
	FString TierColor;
	FString ItemType;
	FVector Position = FVector::ZeroVector;
	FVector SourcePosition = FVector::ZeroVector;
	FString SourceType;    // "monster" or "player"
	int32 OwnerCharId = 0;
	bool bIsMvpDrop = false;
	bool bIdentified = true;

	UPROPERTY() TWeakObjectPtr<AGroundItemActor> Actor;
};

/**
 * Manages ground item actors — spawning, despawning, pickup, and socket events.
 * Follows the standard persistent-socket subsystem pattern.
 */
UCLASS()
class SABRIMMO_API UGroundItemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Get a ground item actor by ID (for click detection reverse lookup). */
	AGroundItemActor* GetGroundItemActor(int32 GroundItemId) const;

	/** Reverse lookup: actor -> ground item ID. Returns -1 if not found. */
	int32 GetGroundItemIdFromActor(AActor* Actor) const;

	/** Request the server to pick up a ground item. */
	void RequestPickup(int32 GroundItemId);

private:
	// Socket event handlers
	void HandleItemSpawnedBatch(const TSharedPtr<FJsonValue>& Data);
	void HandleItemPickedUp(const TSharedPtr<FJsonValue>& Data);
	void HandleItemDespawnedBatch(const TSharedPtr<FJsonValue>& Data);
	void HandleItemGroundList(const TSharedPtr<FJsonValue>& Data);
	void HandlePickupSuccess(const TSharedPtr<FJsonValue>& Data);
	void HandlePickupError(const TSharedPtr<FJsonValue>& Data);

	/** Spawn a ground item actor from JSON payload. bPlaySound=false for zone:ready bulk load. */
	void SpawnGroundItemFromJson(const TSharedPtr<FJsonObject>& ItemObj, bool bPlaySound = true);

	/** Remove and destroy a ground item by ID. bFade=true for despawn fade-out. */
	void RemoveGroundItem(int32 GroundItemId, bool bFade = false);

	// Registry
	UPROPERTY() TMap<int32, FGroundItemEntry> GroundItemMap;
	TMap<AActor*, int32> ActorToGroundItemId;

	// Blueprint class for ground items
	UPROPERTY() UClass* GroundItemActorClass = nullptr;
};
