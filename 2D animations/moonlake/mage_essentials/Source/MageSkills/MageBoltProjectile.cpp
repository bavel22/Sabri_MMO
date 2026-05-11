// MageBoltProjectile.cpp
#include "MageBoltProjectile.h"
#include "PaperFlipbookComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AMageBoltProjectile::AMageBoltProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    Flipbook = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Flipbook"));
    SetRootComponent(Flipbook);
    Flipbook->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    NiagaraFx = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraFx"));
    NiagaraFx->SetupAttachment(Flipbook);
    NiagaraFx->bAutoActivate = false;

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->bRotationFollowsVelocity = true;
    Movement->ProjectileGravityScale = 0.f;
    Movement->InitialSpeed = 0.f;
    Movement->MaxSpeed = 0.f;
}

void AMageBoltProjectile::BeginPlay()
{
    Super::BeginPlay();
    SetLifeSpan(MaxLifetime);
}

void AMageBoltProjectile::InitTowardTarget(const FMageSkillRow& InRow, AActor* Target, float Speed)
{
    Row = InRow;
    SeekTarget = Target;
    if (Movement)
    {
        Movement->InitialSpeed = Speed;
        Movement->MaxSpeed = Speed;
        if (Target)
        {
            const FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
            Movement->Velocity = Dir * Speed;
        }
    }
    if (NiagaraFx) NiagaraFx->Activate(true);
}

void AMageBoltProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Elapsed += DeltaSeconds;

    if (!SeekTarget.IsValid())
    {
        Destroy();
        return;
    }

    AActor* Target = SeekTarget.Get();
    const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
    const float Dist = ToTarget.Size();

    // Light homing toward target so bolts always connect.
    if (Movement && Dist > 0.f)
    {
        const FVector Desired = ToTarget.GetSafeNormal() * Movement->MaxSpeed;
        Movement->Velocity = FMath::VInterpTo(Movement->Velocity, Desired, DeltaSeconds, 8.f);
    }

    if (Dist < 60.f) // hit
    {
        BP_OnImpact(Target, GetActorLocation());
        Destroy();
    }
}
