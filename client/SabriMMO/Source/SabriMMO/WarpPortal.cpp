// WarpPortal.cpp — Invisible warp trigger at zone edges.

#include "WarpPortal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "UI/ZoneTransitionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogWarpPortal, Log, All);

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerComp = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerComp"));
	TriggerComp->InitSphereRadius(TriggerRadius);
	TriggerComp->SetCollisionProfileName(TEXT("OverlapAll"));
	TriggerComp->SetGenerateOverlapEvents(true);
	RootComponent = TriggerComp;
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();

	// Update radius in case it was edited per-instance
	TriggerComp->SetSphereRadius(TriggerRadius);

	TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);
}

void AWarpPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// Only react to the local player pawn
	if (!OtherActor) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || PC->GetPawn() != OtherActor) return;

	// 1-second spam guard
	const double Now = FPlatformTime::Seconds();
	if (Now - LastWarpTime < 1.0) return;
	LastWarpTime = Now;

	UE_LOG(LogWarpPortal, Log, TEXT("Player entered warp portal: %s"), *WarpId);

	if (UZoneTransitionSubsystem* ZoneSub = World->GetSubsystem<UZoneTransitionSubsystem>())
	{
		ZoneSub->RequestWarp(WarpId);
	}
}
