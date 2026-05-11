// MageFireWallActor.cpp
#include "MageFireWallActor.h"
#include "PaperFlipbookComponent.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"

AMageFireWallActor::AMageFireWallActor()
{
    PrimaryActorTick.bCanEverTick = true;

    TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    TriggerBox->SetBoxExtent(FVector(50.f, 50.f, 96.f));
    TriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SetRootComponent(TriggerBox);

    Burst  = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Burst"));
    Burst->SetupAttachment(TriggerBox);
    Burst->SetLooping(false);

    Pillar = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Pillar"));
    Pillar->SetupAttachment(TriggerBox);
    Pillar->SetLooping(true);

    PillarNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PillarNiagara"));
    PillarNiagara->SetupAttachment(TriggerBox);
    PillarNiagara->bAutoActivate = false;
}

void AMageFireWallActor::InitCell(const FMageSkillRow& InRow)
{
    Row = InRow;
    HitsRemaining = FMath::Max(1, InRow.HitsPerCell);
    TimeRemaining = FMath::Max(0.5f, InRow.LifetimeSeconds);
}

void AMageFireWallActor::BeginPlay()
{
    Super::BeginPlay();
    if (Burst)  Burst->PlayFromStart();
    if (Pillar) Pillar->Play();
    if (PillarNiagara) PillarNiagara->Activate(true);
}

void AMageFireWallActor::RegisterHit(AActor* Victim)
{
    if (!Victim || HitsRemaining <= 0) return;
    BP_OnHitVictim(Victim);
    HitsRemaining--;
    if (HitsRemaining <= 0)
    {
        BP_OnExpire();
        Destroy();
    }
}

void AMageFireWallActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    TimeRemaining -= DeltaSeconds;
    if (TimeRemaining <= 0.f)
    {
        BP_OnExpire();
        Destroy();
        return;
    }

    HitTickElapsed += DeltaSeconds;
    if (HitTickElapsed < HitTickInterval) return;
    HitTickElapsed = 0.f;

    // Damage everything currently overlapping the trigger box.
    TArray<AActor*> Overlaps;
    TriggerBox->GetOverlappingActors(Overlaps);
    for (AActor* A : Overlaps)
    {
        if (A && A != GetOwner())
        {
            RegisterHit(A);
            if (HitsRemaining <= 0) break;
        }
    }
}
