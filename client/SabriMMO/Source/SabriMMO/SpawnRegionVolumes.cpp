// SpawnRegionVolumes.cpp — see header for usage.

#include "SpawnRegionVolumes.h"
#include "Components/BoxComponent.h"

ASpawnRegionVolume::ASpawnRegionVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	BoxComp->InitBoxExtent(FVector(500.f, 500.f, 250.f));
	BoxComp->SetCollisionProfileName(TEXT("NoCollision"));
	BoxComp->SetGenerateOverlapEvents(false);
	BoxComp->SetHiddenInGame(true);
	BoxComp->SetLineThickness(3.f);
	RootComponent = BoxComp;
}

ASpawnAllowVolume::ASpawnAllowVolume()
{
	if (BoxComp)
	{
		BoxComp->ShapeColor = FColor(50, 200, 50);
	}
}

ASpawnDenyVolume::ASpawnDenyVolume()
{
	if (BoxComp)
	{
		BoxComp->ShapeColor = FColor(200, 50, 50);
	}
}
