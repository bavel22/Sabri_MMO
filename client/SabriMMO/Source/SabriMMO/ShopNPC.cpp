// ShopNPC.cpp — Placeable NPC actor for NPC shops.
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().
// Name text rendered in screen-space by WorldHealthBarSubsystem overlay.

#include "ShopNPC.h"
#include "UI/NameTagSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UI/ShopSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogShopNPC, Log, All);

AShopNPC::AShopNPC()
{
	PrimaryActorTick.bCanEverTick = false;

	// ---- Root capsule (physical collision — blocks player movement) ----
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 96.f);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));
	CapsuleComp->SetSimulatePhysics(false);
	CapsuleComp->SetEnableGravity(false);
	RootComponent = CapsuleComp;

	// ---- Visual mesh (editable per instance, default cylinder placeholder) ----
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	// Trace-only collision: responds to ECC_Visibility so GetHitResultUnderCursor
	// detects this actor on click. Does NOT block player movement.
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(CylinderMesh.Object);
	}
	// Scale to roughly human proportions.
	// Centered on capsule (actor origin sits at ground surface).
	MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay();

	// Register name tag (RO Classic: NPC names are hover-only, light blue)
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->RegisterEntity(this, NPCDisplayName, ENameTagEntityType::NPC);
}

void AShopNPC::Interact()
{
	// Spam guard — ignore rapid-fire calls from Blueprint IA_Attack (fires every frame while held)
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
			UE_LOG(LogShopNPC, Log, TEXT("Player too far from %s (%.0f > %.0f)"),
				*NPCDisplayName, Distance, InteractionRadius);
			return;
		}
	}

	UE_LOG(LogShopNPC, Log, TEXT("Interact() called on %s — opening shop %d"),
		*NPCDisplayName, ShopId);

	if (UShopSubsystem* ShopSub = World->GetSubsystem<UShopSubsystem>())
	{
		if (!ShopSub->IsWidgetVisible())
		{
			ShopSub->RequestOpenShop(ShopId);
		}
	}
}

#if WITH_EDITOR
void AShopNPC::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
