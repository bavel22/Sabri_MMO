// JobChangeNPC.h — Placeable NPC actor for the class change dialog.
// Place in level (e.g. Prontera spawn), interact opens UJobChangeSubsystem dialog.
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JobChangeNPC.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class ASpriteCharacterActor;

UCLASS()
class SABRIMMO_API AJobChangeNPC : public AActor
{
	GENERATED_BODY()

public:
	AJobChangeNPC();

	/** Display name shown above the NPC's head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job Change NPC")
	FString NPCDisplayName = TEXT("Job Master");

	/** How close (UE units) the player must be to interact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job Change NPC")
	float InteractionRadius = 300.f;

	/** Body atlas name to load — must match a folder under Sprites/Atlases/Body/ or Body/enemies/. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job Change NPC")
	FString SpriteAtlasName = TEXT("antonio");

	/** Sprite billboard size in UE units (width, height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job Change NPC")
	FVector2D SpriteSize = FVector2D(200.f, 200.f);

	/** Vertical adjustment in UE units. Positive = raise the sprite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Job Change NPC")
	float SpriteZAdjust = 0.f;

	/** Called when player clicks this NPC. Opens the class change dialog. */
	UFUNCTION(BlueprintCallable, Category = "Job Change NPC")
	void Interact();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	// Invisible at runtime — sprite renders the visual. Click collision is disabled in
	// BeginPlay if a sprite spawns successfully (sprite handles clicks instead).
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	/** Spawned in BeginPlay. Renders the NPC as a 2D billboard. */
	UPROPERTY()
	TObjectPtr<ASpriteCharacterActor> SpriteActor;

private:
	double LastInteractTime = 0.0;
};
