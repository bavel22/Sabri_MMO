// CastBarSubsystem.h — UWorldSubsystem that manages RO-style cast bars.
// Registers Socket.io cast event handlers via the persistent EventRouter,
// tracks active casts for all visible players, and feeds the SCastBarOverlay
// widget for world-projected rendering.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CastBarSubsystem.generated.h"

class SCastBarOverlay;

// Active cast entry — one per caster currently casting a skill
struct FCastBarEntry
{
	int32 CasterId = 0;
	FString CasterName;
	FString SkillName;
	int32 SkillId = 0;
	double CastStartTime = 0.0;
	float CastDuration = 0.0f;  // seconds
};

UCLASS()
class SABRIMMO_API UCastBarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- public data (read by SCastBarOverlay on each paint frame) ----
	TMap<int32, FCastBarEntry> ActiveCasts;  // casterId → cast data
	int32 LocalCharacterId = 0;

private:
	// ---- event handlers ----
	void HandleCastStart(const TSharedPtr<FJsonValue>& Data);
	void HandleCastComplete(const TSharedPtr<FJsonValue>& Data);
	void HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data);
	void HandleCastInterruptedBroadcast(const TSharedPtr<FJsonValue>& Data);
	void HandleCastFailed(const TSharedPtr<FJsonValue>& Data);

	// ---- overlay management ----
	void ShowOverlay();
	void HideOverlay();

	// ---- state ----
	bool bOverlayAdded = false;

	TSharedPtr<SCastBarOverlay> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;
};
