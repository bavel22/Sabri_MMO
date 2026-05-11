// ZoneTransitionSubsystem.h — UWorldSubsystem managing zone transitions.
// Wraps zone:change, zone:error, player:teleport socket events.
// Provides RequestWarp() for WarpPortal actors.
// Shows loading overlay during transitions, teleports pawn on arrival.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "Styling/SlateBrush.h"
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

	/** Show the loading overlay optimistically. Used by client-side triggers that
	 *  expect a zone change but won't get the zone:change response for ~60-250ms
	 *  (Kafra teleport, Butterfly Wing). Hidden automatically by HandleZoneError
	 *  on rejection, or re-applied with the destination name by HandleZoneChange. */
	void ShowExpectedZoneChange(const FString& StatusText = TEXT("Loading..."));

	/** Hide an optimistic overlay shown by ShowExpectedZoneChange when the expected
	 *  zone change was rejected by a non-zone:error path (e.g., kafra:error). */
	void HideExpectedZoneChange();

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

	// Polls UZonePreloadSubsystem until all atlas async loads complete (or timeout),
	// then hides the loading overlay. Called after zone:ready is emitted.
	void WaitForPreloadAndHideOverlay();

	// ---- loading overlay ----
	void ShowLoadingOverlay(const FString& StatusText);
	void HideLoadingOverlay();
	void LoadLoadingScreenTextures();

	// ---- transition completion ----
	void ForceCompleteTransition();

	// ---- state ----
	bool bPawnTeleported = false;
	int32 LocalCharacterId = 0;
	int32 TransitionCheckCount = 0;
	FTimerHandle TransitionCheckTimer;
	FTimerHandle PreloadWaitTimer;
	double PreloadStableSinceTime = 0.0;
	double PreloadWaitStartTime = 0.0;

	TSharedPtr<SWidget> LoadingWidget;
	TSharedPtr<SWidget> LoadingOverlay;
	bool bLoadingShown = false;

	// Loading screen textures (randomly selected per transition)
	UPROPERTY()
	TArray<UTexture2D*> LoadingScreenTextures;
	int32 LastLoadingScreenIndex = -1;
	bool bLoadingTexturesLoaded = false;

	// Active loading screen brush (must stay alive while overlay is shown)
	TSharedPtr<FSlateBrush> ActiveLoadingBrush;

	// Ken Burns animation — random per transition
	float KenBurnsStartScale = 1.0f;
	float KenBurnsEndScale = 1.08f;
	FVector2D KenBurnsDrift = FVector2D::ZeroVector;  // normalized drift direction

	// Loading start time (drives all animations)
	mutable double LoadingStartTime = 0.0;
};
