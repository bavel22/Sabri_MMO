// MageSkillComponent.h
// Attach to your player/NPC pawn. Handles cast pipeline, SP cost, GCD,
// bolt sequencing, and Fire Wall placement. Delegates VFX spawning to your
// Blueprint child via BlueprintImplementableEvent so you can swap Paper2D
// for Niagara without touching C++.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MageSkillTypes.h"
#include "MageSkillComponent.generated.h"

class UDataTable;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillCastStart, FName, SkillId, int32, Level);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCastFinish, FName, SkillId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillCastInterrupt, FName, SkillId);

UCLASS(ClassGroup=(MageSkills), meta=(BlueprintSpawnableComponent))
class UMageSkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMageSkillComponent();

    /** DataTable of FMageSkillRow. Assign the asset built from MageSkills.csv. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageSkills")
    UDataTable* SkillTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageSkills")
    int32 CurrentSP = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageSkills")
    int32 MaxSP = 100;

    // ---- Public API ----------------------------------------------------------------

    /** Begin casting a Targeted bolt (Fire/Cold/Lightning) at TargetActor. */
    UFUNCTION(BlueprintCallable, Category="MageSkills")
    bool BeginCastBolt(FName SkillId, int32 Level, AActor* TargetActor);

    /** Begin casting Fire Wall at the given world location, oriented along Forward. */
    UFUNCTION(BlueprintCallable, Category="MageSkills")
    bool BeginCastFireWall(int32 Level, const FVector& GroundLocation, const FVector& Forward);

    /** Cancel the current cast (movement, stagger, etc.). */
    UFUNCTION(BlueprintCallable, Category="MageSkills")
    void InterruptCast();

    UFUNCTION(BlueprintPure, Category="MageSkills")
    bool IsCasting() const { return bCasting; }

    UFUNCTION(BlueprintPure, Category="MageSkills")
    bool IsOnGlobalCooldown() const;

    // ---- Events --------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable) FOnSkillCastStart OnSkillCastStart;
    UPROPERTY(BlueprintAssignable) FOnSkillCastFinish OnSkillCastFinish;
    UPROPERTY(BlueprintAssignable) FOnSkillCastInterrupt OnSkillCastInterrupt;

    // ---- BP hooks for VFX/SFX ------------------------------------------------------

    /** Show/hide the cast glow VFX on the caster's hand/staff. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|VFX")
    void BP_ShowCastGlow(const FMageSkillRow& Row);
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|VFX")
    void BP_HideCastGlow(const FMageSkillRow& Row);

    /** Spawn a single bolt projectile from MuzzleLocation toward Target. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|VFX")
    void BP_SpawnBoltProjectile(const FMageSkillRow& Row, FVector MuzzleLocation, AActor* Target, int32 BoltIndex);

    /** Spawn a one-shot impact at HitLocation. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|VFX")
    void BP_SpawnImpact(const FMageSkillRow& Row, FVector HitLocation, AActor* Target);

    /** Spawn the ignition burst + persistent pillars for a Fire Wall cell. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|VFX")
    void BP_SpawnFireWallCell(const FMageSkillRow& Row, FVector CellLocation, int32 CellIndex);

    /** Apply damage. Implement in BP/C++ to feed your damage pipeline. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|Combat")
    void BP_ApplyBoltDamage(const FMageSkillRow& Row, AActor* Target, int32 BoltIndex);
    UFUNCTION(BlueprintImplementableEvent, Category="MageSkills|Combat")
    void BP_ApplyFireWallTick(const FMageSkillRow& Row, AActor* Victim, FVector CellLocation);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool LookupRow(FName SkillId, int32 Level, FMageSkillRow& OutRow) const;
    void FinishCast();
    void FireOneBolt(int32 BoltIndex);

    // State
    bool   bCasting        = false;
    float  CastElapsed     = 0.f;
    float  GcdRemaining    = 0.f;
    FMageSkillRow CurrentRow;
    TWeakObjectPtr<AActor> CurrentTarget;
    FVector CurrentGroundLocation = FVector::ZeroVector;
    FVector CurrentForward        = FVector::ForwardVector;

    // Bolt sequencing (after cast finishes)
    bool   bSequencingBolts = false;
    int32  BoltsFired       = 0;
    float  NextBoltIn       = 0.f;
};
