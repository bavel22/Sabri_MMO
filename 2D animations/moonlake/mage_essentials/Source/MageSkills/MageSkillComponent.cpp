// MageSkillComponent.cpp
#include "MageSkillComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UMageSkillComponent::UMageSkillComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

bool UMageSkillComponent::IsOnGlobalCooldown() const
{
    return GcdRemaining > 0.f;
}

bool UMageSkillComponent::LookupRow(FName SkillId, int32 Level, FMageSkillRow& OutRow) const
{
    if (!SkillTable) return false;
    // Convention: row name = "FIRE_BOLT_03"
    const FString RowKey = FString::Printf(TEXT("%s_%02d"), *SkillId.ToString(), Level);
    if (FMageSkillRow* Row = SkillTable->FindRow<FMageSkillRow>(FName(*RowKey), TEXT("MageSkills")))
    {
        OutRow = *Row;
        return true;
    }
    return false;
}

bool UMageSkillComponent::BeginCastBolt(FName SkillId, int32 Level, AActor* TargetActor)
{
    if (bCasting || IsOnGlobalCooldown() || !TargetActor) return false;

    FMageSkillRow Row;
    if (!LookupRow(SkillId, Level, Row)) return false;
    if (Row.Category != EMageSkillCategory::Targeted) return false;
    if (CurrentSP < Row.SP) return false;

    CurrentSP    -= Row.SP;
    CurrentRow    = Row;
    CurrentTarget = TargetActor;

    bCasting    = true;
    CastElapsed = 0.f;

    BP_ShowCastGlow(CurrentRow);
    OnSkillCastStart.Broadcast(SkillId, Level);
    return true;
}

bool UMageSkillComponent::BeginCastFireWall(int32 Level, const FVector& GroundLocation, const FVector& Forward)
{
    if (bCasting || IsOnGlobalCooldown()) return false;

    FMageSkillRow Row;
    if (!LookupRow(TEXT("FIRE_WALL"), Level, Row)) return false;
    if (Row.Category != EMageSkillCategory::Ground) return false;
    if (CurrentSP < Row.SP) return false;

    CurrentSP    -= Row.SP;
    CurrentRow    = Row;
    CurrentGroundLocation = GroundLocation;
    CurrentForward        = Forward.IsNearlyZero() ? FVector::ForwardVector : Forward.GetSafeNormal2D();

    bCasting    = true;
    CastElapsed = 0.f;

    BP_ShowCastGlow(CurrentRow);
    OnSkillCastStart.Broadcast(TEXT("FIRE_WALL"), Level);
    return true;
}

void UMageSkillComponent::InterruptCast()
{
    if (!bCasting && !bSequencingBolts) return;
    BP_HideCastGlow(CurrentRow);
    OnSkillCastInterrupt.Broadcast(CurrentRow.SkillId);

    bCasting         = false;
    bSequencingBolts = false;
    CastElapsed      = 0.f;
    BoltsFired       = 0;
    NextBoltIn       = 0.f;
}

void UMageSkillComponent::FinishCast()
{
    BP_HideCastGlow(CurrentRow);
    GcdRemaining = CurrentRow.GlobalCooldown;

    if (CurrentRow.Category == EMageSkillCategory::Targeted)
    {
        // Sequence bolts at BoltInterval. The motion-cast time is over,
        // but bolts visually launch one after another.
        bSequencingBolts = true;
        BoltsFired = 0;
        NextBoltIn = 0.f; // first bolt fires immediately
    }
    else // Ground (Fire Wall): spawn the cells right now.
    {
        const FVector Right = FVector::CrossProduct(FVector::UpVector, CurrentForward).GetSafeNormal();
        const float CellSize = 100.f; // 1 tile = 100 cm; tune to your grid
        const int32 N = FMath::Max(1, CurrentRow.WidthCells);
        const int32 Half = N / 2;
        for (int32 i = 0; i < N; ++i)
        {
            const int32 Offset = i - Half;
            const FVector Loc = CurrentGroundLocation + Right * (Offset * CellSize);
            BP_SpawnFireWallCell(CurrentRow, Loc, i);
        }
        bCasting = false;
        OnSkillCastFinish.Broadcast(CurrentRow.SkillId);
    }
}

void UMageSkillComponent::FireOneBolt(int32 BoltIndex)
{
    AActor* Target = CurrentTarget.Get();
    if (!Target) return;

    const FVector Muzzle = GetOwner() ? GetOwner()->GetActorLocation() + FVector(0,0,80.f) : FVector::ZeroVector;
    BP_SpawnBoltProjectile(CurrentRow, Muzzle, Target, BoltIndex);
    // Damage applies on impact in BP_SpawnBoltProjectile's BP graph (when projectile hits),
    // OR you can apply now for hitscan-style:
    BP_ApplyBoltDamage(CurrentRow, Target, BoltIndex);
}

void UMageSkillComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (GcdRemaining > 0.f)
    {
        GcdRemaining = FMath::Max(0.f, GcdRemaining - DeltaTime);
    }

    if (bCasting)
    {
        CastElapsed += DeltaTime;
        if (CastElapsed >= CurrentRow.CastTime)
        {
            bCasting = false;
            FinishCast();
        }
    }
    else if (bSequencingBolts)
    {
        NextBoltIn -= DeltaTime;
        if (NextBoltIn <= 0.f)
        {
            FireOneBolt(BoltsFired);
            BoltsFired++;
            if (BoltsFired >= CurrentRow.BoltCount)
            {
                bSequencingBolts = false;
                OnSkillCastFinish.Broadcast(CurrentRow.SkillId);
            }
            else
            {
                NextBoltIn = CurrentRow.BoltInterval;
            }
        }
    }
}
