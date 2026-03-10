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

	/** Snap a teleport location to the nearest walkable ground.
	 *  NavMesh projection first, line trace fallback.
	 *  Adds CapsuleHalfHeight offset so the character stands ON the surface. */
	static FVector SnapLocationToGround(UWorld* World, const FVector& RawLocation, float CapsuleHalfHeight = 96.f);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
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
	bool bPawnTeleported = false;
	int32 LocalCharacterId = 0;
	int32 TransitionCheckCount = 0;
	FTimerHandle TransitionCheckTimer;

	TSharedPtr<SWidget> LoadingWidget;
	TSharedPtr<SWidget> LoadingOverlay;
	bool bLoadingShown = false;
};
