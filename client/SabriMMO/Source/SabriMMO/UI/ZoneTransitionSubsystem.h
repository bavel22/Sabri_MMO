// ZoneTransitionSubsystem.h — UWorldSubsystem managing zone transitions.
// Wraps zone:change, zone:error, player:teleport socket events.
// Provides RequestWarp() for WarpPortal actors.
// Shows loading overlay during transitions, teleports pawn on arrival.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "ZoneTransitionSubsystem.generated.h"

class USocketIOClientComponent;

UCLASS()
class SABRIMMO_API UZoneTransitionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- zone data (read by other systems) ----
	FString CurrentZoneName;
	FString CurrentDisplayName;
	bool bNoTeleport = false;
	bool bNoReturn = false;
	bool bNoSave = false;
	bool bIsTown = false;

	// ---- public API ----
	void RequestWarp(const FString& WarpId);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- socket event wrapping ----
	void TryWrapSocketEvents();
	void WrapSingleEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
	USocketIOClientComponent* FindSocketIOComponent() const;

	// ---- event handlers ----
	void HandleZoneChange(const TSharedPtr<FJsonValue>& Data);
	void HandleZoneError(const TSharedPtr<FJsonValue>& Data);
	void HandlePlayerTeleport(const TSharedPtr<FJsonValue>& Data);

	// ---- transition management ----
	void CheckTransitionComplete();
	void TeleportPawnToSpawn();

	// ---- loading overlay ----
	void ShowLoadingOverlay(const FString& StatusText);
	void HideLoadingOverlay();

	// ---- transition completion ----
	void ForceCompleteTransition();

	// ---- state ----
	bool bEventsWrapped = false;
	int32 LocalCharacterId = 0;
	int32 TransitionCheckCount = 0;
	FTimerHandle BindCheckTimer;
	FTimerHandle TransitionCheckTimer;

	TSharedPtr<SWidget> LoadingWidget;
	TSharedPtr<SWidget> LoadingOverlay;
	bool bLoadingShown = false;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
