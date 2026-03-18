// CompanionVisualActor.h — Simple geometry placeholder for cart/mount/falcon visuals.
// Uses UStaticMeshComponent with engine primitives (cube, cylinder, sphere).
// Pure C++ — no Blueprint dependencies.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompanionVisualActor.generated.h"

UENUM()
enum class ECompanionShape : uint8
{
	Cart,    // Flat box trailing behind player
	Mount,   // Large cylinder under player
	Falcon,  // Small sphere near shoulder
};

UCLASS()
class SABRIMMO_API ACompanionVisualActor : public AActor
{
	GENERATED_BODY()

public:
	ACompanionVisualActor();

	void InitShape(ECompanionShape Shape);

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	ECompanionShape CompanionShape = ECompanionShape::Cart;
};
