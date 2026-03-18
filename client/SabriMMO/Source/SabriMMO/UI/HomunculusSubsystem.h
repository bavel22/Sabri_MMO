// HomunculusSubsystem.h — UWorldSubsystem managing homunculus state and UI.
// Handles homunculus:summoned, homunculus:update, homunculus:died, homunculus:fed,
// homunculus:hunger_tick, homunculus:evolved, homunculus:vaporized events.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "HomunculusSubsystem.generated.h"

class SHomunculusWidget;

UCLASS()
class SABRIMMO_API UHomunculusSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- homunculus state (read by widget) ----
	bool bHasHomunculus = false;
	FString HomunculusName;
	FString HomunculusType;  // lif, amistr, filir, vanilmirth
	int32 HomunculusLevel = 1;
	int32 HP = 0;
	int32 MaxHP = 0;
	int32 SP = 0;
	int32 MaxSP = 0;
	int32 Hunger = 100;
	int32 Intimacy = 250;
	bool bIsEvolved = false;
	bool bIsAlive = true;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- public API ----
	void ToggleWidget();
	void FeedHomunculus();
	void VaporizeHomunculus();

private:
	void HandleSummoned(const TSharedPtr<FJsonValue>& Data);
	void HandleVaporized(const TSharedPtr<FJsonValue>& Data);
	void HandleUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleLeveledUp(const TSharedPtr<FJsonValue>& Data);
	void HandleDied(const TSharedPtr<FJsonValue>& Data);
	void HandleFed(const TSharedPtr<FJsonValue>& Data);
	void HandleHungerTick(const TSharedPtr<FJsonValue>& Data);
	void HandleEvolved(const TSharedPtr<FJsonValue>& Data);
	void HandleResurrected(const TSharedPtr<FJsonValue>& Data);

	void ShowWidget();
	void HideWidget();

	void SpawnHomunculusActor();
	void DespawnHomunculusActor();
	void TickFollowOwner();

	TSharedPtr<SHomunculusWidget> HomWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
	bool bWidgetAdded = false;
	bool bWidgetVisible = false;

	// ---- homunculus actor (placeholder: BP_EnemyCharacter scaled) ----
	UPROPERTY()
	UClass* HomActorClass = nullptr;

	UPROPERTY()
	AActor* HomActor = nullptr;

	FTimerHandle FollowTimerHandle;
};
