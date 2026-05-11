// ShopNPC.cpp — Placeable NPC actor for NPC shops.
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().
// Name text rendered in screen-space by WorldHealthBarSubsystem overlay.

#include "ShopNPC.h"
#include "UI/NameTagSubsystem.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
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
	// IMPORTANT: ignore Visibility traces so cursor line traces reach the sprite quad
	// instead of stopping at the capsule. Pawn-Pawn collision (player movement blocking)
	// is preserved. Without this, hover-only nametags fail in the lower half of the
	// sprite — the trace hits the capsule (resolves to AShopNPC) instead of the sprite,
	// and NameTagSubsystem's hover match (Entry.Actor == SpriteActor) fails.
	CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
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

	// Hide the placeholder cylinder — sprite actor renders the visual.
	// Click collision is disabled in BeginPlay if a sprite spawns successfully.
	MeshComp->SetVisibility(false);
}

void AShopNPC::BeginPlay()
{
	Super::BeginPlay();

	// Re-apply at runtime in case existing placed actors have stale serialized collision
	// settings from before the constructor was updated. Cursor traces must reach the sprite.
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	UWorld* World = GetWorld();

	// Spawn the sprite billboard at the shop NPC's feet.
	// Sprite quad bottom = sprite actor location; capsule center = GetActorLocation(),
	// capsule half-height = 96 (set in constructor). Subtract 96 to land at the ground.
	if (World && !SpriteAtlasName.IsEmpty())
	{
		FVector SpriteSpawnLoc = GetActorLocation();
		SpriteSpawnLoc.Z -= 96.f;        // Capsule half-height (down to ground)
		SpriteSpawnLoc.Z += SpriteZAdjust; // Per-instance fine-tune for atlas framing

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpriteActor = World->SpawnActor<ASpriteCharacterActor>(
			ASpriteCharacterActor::StaticClass(),
			SpriteSpawnLoc, FRotator::ZeroRotator,
			SpawnParams);

		if (SpriteActor)
		{
			// Apply size BEFORE SetBodyClass so the initial quad layout uses our dimensions.
			SpriteActor->SpriteSize = SpriteSize;
			// Loads the V2 manifest. SetBodyClass(FString) checks Body/enemies/{name}/ first,
			// so "knocker" → Body/enemies/knocker/knocker_manifest.json.
			SpriteActor->SetBodyClass(SpriteAtlasName);
			// Face South (front toward the camera) by default. Default FacingDir = +X (North)
			// matches camera forward → would show the back. CalculateDirection picks the
			// right atlas frame as the camera rotates around the NPC (right-click drag).
			SpriteActor->SetFacingDirection(FVector(-1.f, 0.f, 0.f));

			// Sprite quad becomes the click trace target (matches EnemySubsystem pattern).
			SpriteActor->EnableClickCollision();
			// NOTE: Do NOT set bUseServerMovement = true. Its per-tick ground-snap line trace
			// fires from sprite_z+500 downward and hits the AShopNPC capsule (Pawn profile
			// blocks WorldStatic) before the actual ground, snapping the sprite ~192 units up.
			// Stationary NPCs are positioned correctly at spawn (actor_loc - 96 = ground),
			// so the per-tick snap is unnecessary. Without bUseServerMovement and without an
			// OwnerActor, UpdateOwnerTracking is a no-op and the sprite stays at spawn.
		}
	}

	// Sprite is now the click target. Disable the cylinder's ECC_Visibility response
	// so it doesn't intercept the trace before it reaches the sprite. (If sprite spawn
	// failed, the cylinder stays clickable as a fallback.)
	if (SpriteActor && MeshComp)
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	// Register name tag — same canonical pattern as EnemySubsystem and OtherPlayerSubsystem
	// for sprite entities: register the sprite (so the hover-only NPC check matches),
	// pass SpriteHeight for zoom-proportional positioning above the sprite top.
	if (World)
	{
		AActor* NameTagTarget = SpriteActor ? (AActor*)SpriteActor : (AActor*)this;
		const float NameTagSpriteHeight = SpriteActor ? SpriteSize.Y : 0.f;
		if (UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>())
			NTS->RegisterEntity(NameTagTarget, NPCDisplayName,
				ENameTagEntityType::NPC, 0, 120.f, NameTagSpriteHeight);
	}
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
