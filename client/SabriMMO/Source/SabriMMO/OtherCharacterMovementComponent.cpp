// OtherCharacterMovementComponent.cpp — Floor-snap for remote players.
// The Blueprint movement logic (BP_OtherPlayerCharacter Tick) uses AddMovementInput
// with a flat XY direction — it never corrects Z. If a remote player spawns at the
// wrong Z (from stale server data, warp destinations, etc.), they get stuck above or
// below the floor. This override runs a cheap downward line trace each tick and
// teleports the character to the floor when the Z mismatch exceeds a threshold.

#include "OtherCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogOtherCharMove, Log, All);

void UOtherCharacterMovementComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Owner = GetCharacterOwner();
	if (!Owner) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Downward line trace to find the floor beneath the character
	const FVector ActorLoc = Owner->GetActorLocation();
	const float HalfHeight = Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	const FVector TraceStart(ActorLoc.X, ActorLoc.Y, ActorLoc.Z + 200.f);
	const FVector TraceEnd(ActorLoc.X, ActorLoc.Y, ActorLoc.Z - 2000.f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OtherPlayerFloorSnap), false, Owner);

	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		return; // No floor found — nothing to snap to
	}

	// Expected Z = floor surface + capsule half-height
	const float ExpectedZ = Hit.ImpactPoint.Z + HalfHeight;
	const float ZDiff = FMath::Abs(ActorLoc.Z - ExpectedZ);

	// Only snap if the mismatch is significant (> 50 units)
	// Small differences are normal during slopes/stairs
	if (ZDiff > 50.f)
	{
		FVector Corrected = ActorLoc;
		Corrected.Z = ExpectedZ;
		Owner->SetActorLocation(Corrected, false, nullptr, ETeleportType::TeleportPhysics);

		UE_LOG(LogOtherCharMove, Verbose,
			TEXT("Floor-snapped remote player %s: Z %.0f → %.0f (diff %.0f)"),
			*Owner->GetName(), ActorLoc.Z, ExpectedZ, ZDiff);
	}
}
