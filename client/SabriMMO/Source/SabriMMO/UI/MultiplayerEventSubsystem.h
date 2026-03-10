// MultiplayerEventSubsystem.h — UWorldSubsystem that bridges Socket.io events
// from the persistent EventRouter to BP_SocketManager's existing handler functions.
// This preserves all existing Blueprint logic (OnCombatDamage 221 nodes, etc.)
// while the event delivery path uses the persistent socket on GameInstance.

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

	// Direct C++ handler for enemy:attack (no BP function exists for this)
	void HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data);

	// Utility to serialize FJsonValue to a JSON string for BP handler consumption
	static FString JsonValueToString(const TSharedPtr<FJsonValue>& Value);

	// Cached reference to BP_SocketManager actor (found once on begin play)
	TWeakObjectPtr<AActor> SocketManagerActor;

	// Cached reference to BP_EnemyManager actor (for enemy:attack handling)
	TWeakObjectPtr<AActor> EnemyManagerActor;

	// Find BP_SocketManager in the level
	AActor* FindSocketManagerActor() const;

	// Find BP_EnemyManager in the level
	AActor* FindEnemyManagerActor() const;

	// Guard: prevents ProcessEvent calls during PostLoad (crashes UE5).
	// Set to true one frame after OnWorldBeginPlay via SetTimerForNextTick.
	bool bReadyToForward = false;
};
