// PetSubsystem.h — UWorldSubsystem managing pet state and SPetWidget lifecycle.
// Handles pet:hatched, pet:fed, pet:hunger_update, pet:ran_away, pet:returned events.
// RO Classic pre-renewal: 56 tameable pets, hunger/intimacy system, stat bonuses at Cordial+.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "PetSubsystem.generated.h"

class SPetWidget;

UCLASS()
class SABRIMMO_API UPetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- pet state (read by SPetWidget) ----
	bool bHasPet = false;
	int32 PetId = 0;
	int32 MobId = 0;
	FString PetName;
	int32 Hunger = 100;       // 0-100
	int32 Intimacy = 250;     // 0-1000
	FString HungerLevel;      // very_hungry/hungry/neutral/satisfied/stuffed
	FString IntimacyLevel;    // awkward/shy/neutral/cordial/loyal
	int32 EquipItemId = 0;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- public API ----
	void ToggleWidget();
	void FeedPet();
	void ReturnPetToEgg();
	void RenamePet(const FString& NewName);
	void RequestPetList();

private:
	void HandlePetHatched(const TSharedPtr<FJsonValue>& Data);
	void HandlePetFed(const TSharedPtr<FJsonValue>& Data);
	void HandlePetHungerUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandlePetRanAway(const TSharedPtr<FJsonValue>& Data);
	void HandlePetReturned(const TSharedPtr<FJsonValue>& Data);
	void HandlePetRenamed(const TSharedPtr<FJsonValue>& Data);
	void HandlePetTamed(const TSharedPtr<FJsonValue>& Data);
	void HandlePetError(const TSharedPtr<FJsonValue>& Data);

	void ShowWidget();
	void HideWidget();
	void SpawnPetActor();
	void DespawnPetActor();
	void TickFollowOwner();

	TSharedPtr<SPetWidget> PetWidget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
	bool bWidgetAdded = false;
	bool bWidgetVisible = false;

	// ---- pet actor (placeholder: BP_EnemyCharacter scaled down) ----
	UPROPERTY()
	UClass* PetActorClass = nullptr;

	UPROPERTY()
	AActor* PetActor = nullptr;

	FTimerHandle FollowTimerHandle;
};
