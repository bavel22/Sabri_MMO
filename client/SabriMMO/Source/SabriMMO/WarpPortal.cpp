// WarpPortal.cpp — Invisible warp trigger at zone edges.

#include "WarpPortal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "UI/ZoneTransitionSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "SkillVFXSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogWarpPortal, Log, All);

AWarpPortal::AWarpPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerComp = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerComp"));
	TriggerComp->InitSphereRadius(TriggerRadius);
	TriggerComp->SetCollisionProfileName(TEXT("OverlapAll"));
	TriggerComp->SetGenerateOverlapEvents(true);
	RootComponent = TriggerComp;

	// Niagara VFX component — visible portal effect (golden ring + spiraling particles)
	PortalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalEffect"));
	PortalEffect->SetupAttachment(RootComponent);
	PortalEffect->SetAutoActivate(false); // Activated in BeginPlay after system is assigned
}

void AWarpPortal::BeginPlay()
{
	Super::BeginPlay();

	// Update radius in case it was edited per-instance
	TriggerComp->SetSphereRadius(TriggerRadius);

	TriggerComp->OnComponentBeginOverlap.AddDynamic(this, &AWarpPortal::OnOverlapBegin);

	// Activate portal VFX if a system is explicitly assigned in the Blueprint
	if (PortalVFXSystem && PortalEffect)
	{
		PortalEffect->SetAsset(PortalVFXSystem);
		PortalEffect->Activate(true);
	}
	else
	{
		// Deferred: wait for SkillVFXSubsystem to load Niagara assets, then spawn portal VFX
		VFXRetryCount = 0;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(VFXRetryTimer,
				FTimerDelegate::CreateUObject(this, &AWarpPortal::TryActivatePortalVFX),
				1.0f, true); // Retry every 1s
		}
	}
}

void AWarpPortal::TryActivatePortalVFX()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Give up after 10 retries (10 seconds)
	if (++VFXRetryCount > 10)
	{
		World->GetTimerManager().ClearTimer(VFXRetryTimer);
		UE_LOG(LogWarpPortal, Warning, TEXT("Warp portal %s: gave up waiting for VFX subsystem"), *WarpId);
		return;
	}

	USkillVFXSubsystem* VFXSub = World->GetSubsystem<USkillVFXSubsystem>();
	if (!VFXSub) return;

	UNiagaraComponent* Comp = VFXSub->SpawnLoopingPortalEffect(GetActorLocation());
	if (Comp)
	{
		SpawnedPortalVFX = Comp;
		World->GetTimerManager().ClearTimer(VFXRetryTimer);
		UE_LOG(LogWarpPortal, Log, TEXT("Warp portal %s: VFX activated"), *WarpId);
	}
}

void AWarpPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up the Niagara portal VFX to prevent orphaned components
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VFXRetryTimer);
	}

	if (SpawnedPortalVFX.IsValid())
	{
		SpawnedPortalVFX->DeactivateImmediate();
		SpawnedPortalVFX->DestroyComponent();
		SpawnedPortalVFX = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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
