// CompanionVisualActor.cpp — Simple geometry placeholders for cart/mount/falcon.

#include "CompanionVisualActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

ACompanionVisualActor::ACompanionVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->CastShadow = true;
}

void ACompanionVisualActor::InitShape(ECompanionShape Shape)
{
	CompanionShape = Shape;

	UStaticMesh* PrimitiveMesh = nullptr;
	FVector Scale = FVector(1.f);
	FLinearColor Color = FLinearColor::White;

	switch (Shape)
	{
	case ECompanionShape::Cart:
		// Flat brown box — 80x50x30 units (cart-like)
		PrimitiveMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Scale = FVector(0.8f, 0.5f, 0.3f);
		Color = FLinearColor(0.55f, 0.35f, 0.15f, 1.f); // Brown wood
		break;

	case ECompanionShape::Mount:
		// Large yellow-brown cylinder — Peco Peco body
		PrimitiveMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		Scale = FVector(0.6f, 0.6f, 0.4f);
		Color = FLinearColor(0.85f, 0.65f, 0.25f, 1.f); // Golden-brown (Peco color)
		break;

	case ECompanionShape::Falcon:
		// Small grey sphere — falcon perch
		PrimitiveMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		Scale = FVector(0.15f, 0.15f, 0.15f);
		Color = FLinearColor(0.5f, 0.5f, 0.6f, 1.f); // Blue-grey
		break;
	}

	if (PrimitiveMesh && MeshComp)
	{
		MeshComp->SetStaticMesh(PrimitiveMesh);
		MeshComp->SetWorldScale3D(Scale);

		// Create colored material
		UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (BaseMat)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(TEXT("Color"), Color);
				MeshComp->SetMaterial(0, DynMat);
			}
		}
	}
}
