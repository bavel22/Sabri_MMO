// KafraNPC.h — Placeable Kafra NPC actor for save point + teleport service.
// Place in level, set KafraId per instance (must match ro_zone_data.js).
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KafraNPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class ASpriteCharacterActor;

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

	/** Body atlas name to load — must match a folder under Sprites/Atlases/Body/ or Body/enemies/. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	FString SpriteAtlasName = TEXT("miyabi_doll");

	/** Sprite billboard size in UE units (width, height). Default sized for humanoid NPC. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	FVector2D SpriteSize = FVector2D(200.f, 200.f);

	/** Vertical adjustment in UE units. Positive = raise the sprite, negative = lower it.
	 * Use to compensate for atlases where the visible character isn't at the cell bottom
	 * (e.g., short / centered characters). Default 0 works for tall humanoids like miyabi_doll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kafra NPC")
	float SpriteZAdjust = 0.f;

	/** Called from C++ when player clicks this NPC. Opens the Kafra UI. */
	UFUNCTION(BlueprintCallable, Category = "Kafra NPC")
	void Interact();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	// Invisible at runtime — sprite renders the visual. Kept for click-trace collision.
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	/** Spawned in BeginPlay, parented via Owner. Renders the Kafra as a 2D billboard. */
	UPROPERTY()
	TObjectPtr<ASpriteCharacterActor> SpriteActor;

private:
	double LastInteractTime = 0.0;
};
