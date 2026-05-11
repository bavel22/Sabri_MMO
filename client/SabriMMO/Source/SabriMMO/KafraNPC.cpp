// KafraNPC.cpp — Placeable Kafra NPC actor for save point + teleport service.
// Click detection: BP_MMOCharacter's IA_Attack cast chain calls Interact().
// Name text rendered in screen-space by WorldHealthBarSubsystem overlay.

#include "KafraNPC.h"
#include "UI/NameTagSubsystem.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/KafraSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogKafraNPC, Log, All);

AKafraNPC::AKafraNPC()
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
	// sprite — the trace hits the capsule (resolves to AKafraNPC) instead of the sprite,
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
	MeshComp->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

	// Hide the placeholder cylinder — sprite actor renders the visual.
	// Collision stays enabled so click-trace (ECC_Visibility) still hits.
	MeshComp->SetVisibility(false);
}

void AKafraNPC::BeginPlay()
{
	Super::BeginPlay();

	// Re-apply at runtime in case existing placed actors have stale serialized collision
	// settings from before the constructor was updated. Cursor traces must reach the sprite.
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	UWorld* World = GetWorld();

	// Spawn the sprite billboard at the Kafra's feet.
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
			// Loads the V2 manifest, registers atlases, defaults to (Unarmed, Idle).
			// miyabi_doll_idle is registered under group="unarmed" → matches the default mode.
			SpriteActor->SetBodyClass(SpriteAtlasName);
			// Face South (front toward the camera) by default. Default FacingDir = +X (North)
			// matches camera forward → would show the back. CalculateDirection then picks
			// the right atlas frame as the camera rotates around the Kafra (right-click drag).
			SpriteActor->SetFacingDirection(FVector(-1.f, 0.f, 0.f));

			// Sprite quad becomes the click trace target (matches EnemySubsystem pattern).
			SpriteActor->EnableClickCollision();
			// NOTE: Do NOT set bUseServerMovement = true. Its per-tick ground-snap line trace
			// fires from sprite_z+500 downward and hits the AKafraNPC capsule (Pawn profile
			// blocks WorldStatic) before the actual ground, snapping the sprite ~192 units up.
			// Stationary NPCs are positioned correctly at spawn (actor_loc - 96 = ground),
			// so the per-tick snap is unnecessary. Without bUseServerMovement and without an
			// OwnerActor, UpdateOwnerTracking is a no-op and the sprite stays at spawn.
		}
	}

	// Sprite is now the click target. Disable the cylinder's ECC_Visibility response
	// so it doesn't intercept the trace before it reaches the sprite. (If sprite spawn
	// failed for any reason, the cylinder stays clickable as a fallback.)
	if (SpriteActor && MeshComp)
	{
		MeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	}

	// Register name tag AFTER the sprite spawns. Register the sprite actor itself
	// (not `this`) and pass SpriteHeight so NameTagSubsystem positions the tag
	// proportionally above the sprite top — same pattern as EnemySubsystem and
	// OtherPlayerSubsystem use for sprite-based entities.
	if (World)
	{
		AActor* NameTagTarget = SpriteActor ? (AActor*)SpriteActor : (AActor*)this;
		const float NameTagSpriteHeight = SpriteActor ? SpriteSize.Y : 0.f;
		if (UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>())
			NTS->RegisterEntity(NameTagTarget, NPCDisplayName,
				ENameTagEntityType::NPC, 0, 120.f, NameTagSpriteHeight);
	}
}

void AKafraNPC::Interact()
{
	// Spam guard — IA_Attack fires every frame while mouse is held
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
			UE_LOG(LogKafraNPC, Log, TEXT("Player too far from %s (%.0f > %.0f)"),
				*NPCDisplayName, Distance, InteractionRadius);
			return;
		}
	}

	UE_LOG(LogKafraNPC, Log, TEXT("Interact() called on %s — opening Kafra service %s"),
		*NPCDisplayName, *KafraId);

	if (UKafraSubsystem* KafraSub = World->GetSubsystem<UKafraSubsystem>())
	{
		if (!KafraSub->IsWidgetVisible())
		{
			KafraSub->RequestOpenKafra(KafraId);
		}
	}
}
