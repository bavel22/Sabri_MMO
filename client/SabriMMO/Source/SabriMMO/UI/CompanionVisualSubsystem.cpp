// CompanionVisualSubsystem.cpp — Spawns/despawns cart/mount/falcon geometry placeholders.
// Cart = brown box trailing behind player
// Mount = golden cylinder under player (replaces walking)
// Falcon = small grey sphere near shoulder

#include "CompanionVisualSubsystem.h"
#include "CompanionVisualActor.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCompVis, Log, All);

bool UCompanionVisualSubsystem::ShouldCreateSubsystem(UObject* Outer) const { return true; }

void UCompanionVisualSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("player:stats"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerStats(D); });
	}

	// 10Hz tick for position updates
	TWeakObjectPtr<UCompanionVisualSubsystem> WeakThis(this);
	InWorld.GetTimerManager().SetTimer(VisualTickHandle, [WeakThis]()
	{
		if (WeakThis.IsValid()) WeakThis->TickVisuals();
	}, 0.1f, true);
}

void UCompanionVisualSubsystem::Deinitialize()
{
	DespawnCart();
	DespawnMount();
	DespawnFalcon();

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(VisualTickHandle);
		UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
		if (GI)
		{
			USocketEventRouter* Router = GI->GetEventRouter();
			if (Router) Router->UnregisterAllForOwner(this);
		}
	}
	Super::Deinitialize();
}

void UCompanionVisualSubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool NewMounted = false, NewCart = false, NewFalcon = false;
	Obj->TryGetBoolField(TEXT("isMounted"), NewMounted);

	// hasFalcon comes inside exp or derived sub-objects, or top-level
	Obj->TryGetBoolField(TEXT("hasFalcon"), NewFalcon);
	Obj->TryGetBoolField(TEXT("hasCart"), NewCart);

	// Also check nested — server may put these at top level of stats payload
	// spiritSpheres is at top level, so mount/cart/falcon should be too

	// Mount toggled
	if (NewMounted && !bIsMounted) SpawnMount();
	else if (!NewMounted && bIsMounted) DespawnMount();
	bIsMounted = NewMounted;

	// Cart toggled
	if (NewCart && !bHasCart) SpawnCart();
	else if (!NewCart && bHasCart) DespawnCart();
	bHasCart = NewCart;

	// Falcon toggled
	if (NewFalcon && !bHasFalcon) SpawnFalcon();
	else if (!NewFalcon && bHasFalcon) DespawnFalcon();
	bHasFalcon = NewFalcon;
}

// ============================================================
// Spawn / Despawn
// ============================================================

void UCompanionVisualSubsystem::SpawnCart()
{
	if (CartActor) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CartActor = World->SpawnActor<ACompanionVisualActor>(ACompanionVisualActor::StaticClass(), FTransform::Identity, Params);
	if (CartActor) CartActor->InitShape(ECompanionShape::Cart);
	UE_LOG(LogCompVis, Log, TEXT("Spawned cart placeholder"));
}

void UCompanionVisualSubsystem::DespawnCart()
{
	if (CartActor) { CartActor->Destroy(); CartActor = nullptr; }
}

void UCompanionVisualSubsystem::SpawnMount()
{
	if (MountActor) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MountActor = World->SpawnActor<ACompanionVisualActor>(ACompanionVisualActor::StaticClass(), FTransform::Identity, Params);
	if (MountActor) MountActor->InitShape(ECompanionShape::Mount);
	UE_LOG(LogCompVis, Log, TEXT("Spawned mount placeholder"));
}

void UCompanionVisualSubsystem::DespawnMount()
{
	if (MountActor) { MountActor->Destroy(); MountActor = nullptr; }
}

void UCompanionVisualSubsystem::SpawnFalcon()
{
	if (FalconActor) return;
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FalconActor = World->SpawnActor<ACompanionVisualActor>(ACompanionVisualActor::StaticClass(), FTransform::Identity, Params);
	if (FalconActor) FalconActor->InitShape(ECompanionShape::Falcon);
	UE_LOG(LogCompVis, Log, TEXT("Spawned falcon placeholder"));
}

void UCompanionVisualSubsystem::DespawnFalcon()
{
	if (FalconActor) { FalconActor->Destroy(); FalconActor = nullptr; }
}

// ============================================================
// Position Tick (10Hz)
// ============================================================

void UCompanionVisualSubsystem::TickVisuals()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn) return;

	const FVector OwnerLoc = Pawn->GetActorLocation();
	const FRotator OwnerRot = Pawn->GetActorRotation();

	// Cart: trails behind player (200 units back, centered)
	if (CartActor)
	{
		const FVector CartOffset = OwnerRot.RotateVector(FVector(-200.f, 0.f, -40.f));
		const FVector CartTarget = OwnerLoc + CartOffset;
		const FVector CartCur = CartActor->GetActorLocation();
		CartActor->SetActorLocation(FMath::VInterpTo(CartCur, CartTarget, 0.1f, 6.f));
		CartActor->SetActorRotation(OwnerRot);
	}

	// Mount: directly under player (slight Z offset down)
	if (MountActor)
	{
		const FVector MountTarget = OwnerLoc + FVector(0.f, 0.f, -50.f);
		MountActor->SetActorLocation(MountTarget);
		MountActor->SetActorRotation(OwnerRot);
	}

	// Falcon: near right shoulder (up and to the right)
	if (FalconActor)
	{
		const FVector FalconOffset = OwnerRot.RotateVector(FVector(10.f, 30.f, 90.f));
		const FVector FalconTarget = OwnerLoc + FalconOffset;
		const FVector FalconCur = FalconActor->GetActorLocation();
		// Slight bobbing for liveliness
		const float Bob = FMath::Sin(World->GetTimeSeconds() * 3.f) * 5.f;
		FalconActor->SetActorLocation(FMath::VInterpTo(FalconCur, FalconTarget + FVector(0, 0, Bob), 0.1f, 10.f));
	}
}
