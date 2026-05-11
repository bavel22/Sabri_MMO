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

	/** When true, raycasts downward from a sample grid auto-determine which cells are deep.
	 *  When false, falls back to bIsDeep applied to the entire area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Detection")
	bool bAutoDetectDeep = true;

	/** Manual override / fallback when bAutoDetectDeep is false: treat the whole area as deep. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Detection",
		meta = (EditCondition = "!bAutoDetectDeep"))
	bool bIsDeep = false;

	/** Depth (UE units) below the water surface to mark a sample as deep. ~50 UE = 1 RO cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Detection",
		meta = (EditCondition = "bAutoDetectDeep", ClampMin = "50"))
	float DeepDepthThreshold = 200.f;

	/** Grid resolution for the depth raycast scan (NxN). Higher = finer deep-cell granularity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Detection",
		meta = (EditCondition = "bAutoDetectDeep", ClampMin = "4", ClampMax = "64"))
	int32 DepthSampleResolution = 16;

	/** Shallow water color — darker blue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	FLinearColor ShallowColor = FLinearColor(0.08f, 0.18f, 0.5f, 0.3f);

	/** Deep water color — deep blue (mid-depth, around DepthFalloff distance) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	FLinearColor DeepColor = FLinearColor(0.03f, 0.08f, 0.35f, 0.5f);

	/** Abyssal color — extreme depth, near-black navy. Final tier of the depth gradient. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	FLinearColor AbyssColor = FLinearColor(0.005f, 0.015f, 0.05f, 1.f);

	/** Opacity at abyss depth. 1.0 = fully opaque (no see-through). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual",
		meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float AbyssOpacity = 1.f;

	/** Depth (UE units) at which water reaches AbyssColor + AbyssOpacity. Must be > DepthFalloff (250u default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual",
		meta = (ClampMin = "300"))
	float AbyssDepth = 900.f;

	/** Wave animation speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water|Visual")
	float WaveSpeed = 0.5f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

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

	/** Per-deep-cell nav modifier boxes (each cuts a rectangular region out of the navmesh) */
	UPROPERTY()
	TArray<UBoxComponent*> DeepNavBoxes;

	UPROPERTY()
	UMaterialInstanceDynamic* WaterMID = nullptr;

	/** Bool grid (DepthSampleResolution^2) — true if cell is deep. Indexed [j*N + i]. */
	TArray<bool> DeepCells;

	void CreateWaterMaterial();
	void ApplyExtentToComponents();
	void PerformDepthScan();
	void SpawnDeepNavBoxes();
	void ClearDeepNavBoxes();

	double LastWaterEventTime = 0.0;
	bool bPlayerInWater = false;
};
