// JobChangeNPC.cpp — Placeable NPC actor for the class change dialog.
// Mirrors AShopNPC pattern: capsule + invisible cylinder fallback + sprite billboard.

#include "JobChangeNPC.h"
#include "UI/NameTagSubsystem.h"
#include "UI/JobChangeSubsystem.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogJobChangeNPC, Log, All);

AJobChangeNPC::AJobChangeNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	// ---- Root capsule (physical collision — blocks player movement) ----
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 96.f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComp->SetSimulatePhysics(false);
	CapsuleComp->SetEnableGravity(false);
	// Ignore Visibility traces so cursor line traces reach the sprite quad
	// instead of stopping at the capsule. Pawn-Pawn collision (player movement
	// blocking) is preserved.
	CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	RootComponent = CapsuleComp;

	// ---- Visual mesh (invisible, sprite handles visual + click target) ----
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(CylinderMesh.Object);
	}
	MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	MeshComp->SetVisibility(false);
}

void AJobChangeNPC::BeginPlay()
{
	Super::BeginPlay();

	// Re-apply at runtime in case existing placed actors have stale serialized
	// collision settings from before the constructor was updated.
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	UWorld* World = GetWorld();

	// Spawn the sprite billboard at the NPC's feet.
	if (World && !SpriteAtlasName.IsEmpty())
	{
		FVector SpriteSpawnLoc = GetActorLocation();
		SpriteSpawnLoc.Z -= 96.f;        // Capsule half-height (down to ground)
		SpriteSpawnLoc.Z += SpriteZAdjust;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpriteActor = World->SpawnActor<ASpriteCharacterActor>(
			ASpriteCharacterActor::StaticClass(),
			SpriteSpawnLoc, FRotator::ZeroRotator,
			SpawnParams);

		if (SpriteActor)
		{
			SpriteActor->SpriteSize = SpriteSize;
			SpriteActor->SetBodyClass(SpriteAtlasName);
			SpriteActor->SetFacingDirection(FVector(-1.f, 0.f, 0.f));
			SpriteActor->EnableClickCollision();
		}
	}

	// Disable cylinder click trace once the sprite owns it.
	if (SpriteActor && MeshComp)
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	// Register name tag with the same canonical pattern as ShopNPC.
	if (World)
	{
		AActor* NameTagTarget = SpriteActor ? (AActor*)SpriteActor : (AActor*)this;
		const float NameTagSpriteHeight = SpriteActor ? SpriteSize.Y : 0.f;
		if (UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>())
		{
			NTS->RegisterEntity(NameTagTarget, NPCDisplayName,
				ENameTagEntityType::NPC, 0, 120.f, NameTagSpriteHeight);
		}
	}
}

void AJobChangeNPC::Interact()
{
	// Spam guard — IA_Attack fires every frame while the mouse button is held.
	const double Now = FPlatformTime::Seconds();
	if (Now - LastInteractTime < 1.0)
	{
		return;
	}
	LastInteractTime = Now;

	UWorld* World = GetWorld();
	if (!World) return;

	// Range check — player must be within InteractionRadius
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC && PC->GetPawn())
	{
		float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), GetActorLocation());
		if (Distance > InteractionRadius)
		{
			UE_LOG(LogJobChangeNPC, Log, TEXT("Player too far from %s (%.0f > %.0f)"),
				*NPCDisplayName, Distance, InteractionRadius);
			return;
		}
	}

	UE_LOG(LogJobChangeNPC, Log, TEXT("Interact() on %s — opening class change dialog"),
		*NPCDisplayName);

	if (UJobChangeSubsystem* Sub = World->GetSubsystem<UJobChangeSubsystem>())
	{
		if (!Sub->IsWidgetVisible())
		{
			Sub->OpenDialog();
		}
	}
}
