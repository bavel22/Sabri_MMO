// KafraNPC.h — Placeable Kafra NPC actor for save point + teleport service.
// Place in level, set KafraId per instance (must match ro_zone_data.js).
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KafraNPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

UCLASS()
class SABRIMMO_API AKafraNPC : public AActor
{
	GENERATED_BODY()

public:
	AKafraNPC();

	/** Must match a kafraId in the server's ZONE_REGISTRY for the current zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	FString KafraId;

	/** Display name shown above the NPC's head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	FString NPCDisplayName = TEXT("Kafra Employee");

	/** How close (UE units) the player must be to interact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	float InteractionRadius = 300.f;

	/** Called from C++ when player clicks this NPC. Opens the Kafra UI. */
	UFUNCTION(BlueprintCallable, Category = "Kafra NPC")
	void Interact();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

private:
	double LastInteractTime = 0.0;
};
