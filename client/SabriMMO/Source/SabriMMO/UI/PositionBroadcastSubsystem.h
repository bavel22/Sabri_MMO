// PositionBroadcastSubsystem.h — UWorldSubsystem that broadcasts the local player's
// position to the server at 30Hz via the persistent socket on GameInstance.
// Replaces the position timer that was previously on BP_SocketManager.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PositionBroadcastSubsystem.generated.h"

UCLASS()
class SABRIMMO_API UPositionBroadcastSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void BroadcastPosition();

	FTimerHandle PositionTimer;
};
