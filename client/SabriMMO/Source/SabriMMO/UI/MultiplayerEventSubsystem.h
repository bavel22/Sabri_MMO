// MultiplayerEventSubsystem.h — UWorldSubsystem that bridges Socket.io events
// from the persistent EventRouter to BP_SocketManager's existing handler functions.
// Bridges 14 events to BP (inventory, loot, chat, stats, hotbar, shop).
// Player/enemy/combat events migrated to C++ subsystems in Phases 2-3.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "MultiplayerEventSubsystem.generated.h"

UCLASS()
class SABRIMMO_API UMultiplayerEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Public API for Blueprints to emit events via the persistent socket
	UFUNCTION(BlueprintCallable, Category = "MMO Multiplayer")
	void EmitCombatAttack(int32 TargetId, bool bIsEnemy);

	UFUNCTION(BlueprintCallable, Category = "MMO Multiplayer")
	void EmitStopAttack();

	UFUNCTION(BlueprintCallable, Category = "MMO Multiplayer")
	void RequestRespawn();

	UFUNCTION(BlueprintCallable, Category = "MMO Multiplayer")
	void EmitChatMessage(const FString& Message, const FString& Channel);

private:
	// Bridge: converts FJsonValue to FString and calls BP handler via ProcessEvent
	void ForwardToBPHandler(const FString& FunctionName, const TSharedPtr<FJsonValue>& Data);

	// Utility to serialize FJsonValue to a JSON string for BP handler consumption
	static FString JsonValueToString(const TSharedPtr<FJsonValue>& Value);

	// Cached reference to BP_SocketManager actor (found once on begin play)
	TWeakObjectPtr<AActor> SocketManagerActor;

	// Find BP_SocketManager in the level
	AActor* FindSocketManagerActor() const;

	// Guard: prevents ProcessEvent calls during PostLoad (crashes UE5).
	// Set to true one frame after OnWorldBeginPlay via SetTimerForNextTick.
	bool bReadyToForward = false;
};
