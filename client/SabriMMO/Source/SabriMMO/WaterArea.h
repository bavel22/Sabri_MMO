// WaterArea.h — RO-style water area: visual animated plane + trigger for gameplay detection.
// Place in levels at canals, ponds, rivers. Shallow water is walkable; deep water blocks via NavMesh.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterArea.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class SABRIMMO_API AWaterArea : public AActor
{
	GENERATED_BODY()

public:
	AWaterArea();

	/** Unique ID matching server zone data (e.g., "prt_canal_01") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FString WaterAreaId;

	/** Half-extent of the water area box (X, Y in world units). Z is ignored — water is flat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	FVector2D WaterExtent = FVector2D(500.f, 500.f);

	/** Whether this is deep (blocking) water. Deep water should exclude NavMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
	bool bIsDeep = false;

	/** Shallow water color — darker blue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	FLinearColor ShallowColor = FLinearColor(0.08f, 0.18f, 0.5f, 0.3f);

	/** Deep water color — deep blue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	FLinearColor DeepColor = FLinearColor(0.03f, 0.08f, 0.35f, 0.5f);

	/** Wave animation speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	float WaveSpeed = 0.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* TriggerComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* WaterPlaneMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* WaterMID = nullptr;

	void CreateWaterMaterial();
	void SetupWaterPlane();

	double LastWaterEventTime = 0.0;
	bool bPlayerInWater = false;
};
