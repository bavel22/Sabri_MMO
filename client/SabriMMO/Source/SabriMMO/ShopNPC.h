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
class ASpriteCharacterActor;

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

	/** Body atlas name to load — must match a folder under Sprites/Atlases/Body/ or Body/enemies/. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	FString SpriteAtlasName = TEXT("knocker");

	/** Sprite billboard size in UE units (width, height). Default sized for humanoid NPC. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	FVector2D SpriteSize = FVector2D(200.f, 200.f);

	/** Vertical adjustment in UE units. Positive = raise the sprite, negative = lower it.
	 * Use to compensate for atlases where the visible character isn't at the cell bottom.
	 * Default 0 matches the Kafra/miyabi_doll setup that works correctly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop NPC")
	float SpriteZAdjust = 0.f;

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

	// Invisible at runtime — sprite renders the visual. Click collision is disabled in
	// BeginPlay if a sprite spawns successfully (sprite handles clicks instead).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	/** Spawned in BeginPlay, parented via Owner. Renders the shop NPC as a 2D billboard. */
	UPROPERTY()
	TObjectPtr<ASpriteCharacterActor> SpriteActor;

private:
	double LastInteractTime = 0.0;
};
