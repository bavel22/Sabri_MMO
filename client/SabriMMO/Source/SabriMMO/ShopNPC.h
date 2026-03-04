// ShopNPC.h — Placeable NPC actor for NPC shops.
// Place in level, set ShopId and NPCDisplayName per instance.
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().
// Billboard text above NPC always faces camera.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopNPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

UCLASS()
class SABRIMMO_API AShopNPC : public AActor
{
	GENERATED_BODY()

public:
	AShopNPC();

	/** Which server-side shop this NPC opens (1=Tool, 2=Weapon, 3=Armor, 4=General). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	int32 ShopId = 1;

	/** Display name shown above the NPC's head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	FString NPCDisplayName = TEXT("Shop NPC");

	/** How close (UE units) the player must be to interact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	float InteractionRadius = 300.f;

	/** Called from Blueprint when player clicks this NPC. Opens the shop UI. */
	UFUNCTION(BlueprintCallable, Category = "Shop NPC")
	void Interact();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

private:
	double LastInteractTime = 0.0;
};
