// MultiplayerEventSubsystem.h — UWorldSubsystem that bridges Socket.io events
// from the persistent EventRouter to BP_SocketManager's existing handler functions.
// All 14 bridges removed (Phases A-E). Only outbound emit helpers remain.
// chat:receive now handled by ChatSubsystem (Phase E).
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
	// Bridge infrastructure removed in Phase F — all 14 bridges migrated to C++ subsystems.
};
