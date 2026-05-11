// MageBoltProjectile.h
// Generic bolt projectile actor. Visuals are driven by a Paper2D Flipbook
// (FlipbookComponent) OR a Niagara System component depending on what your
// Blueprint subclass attaches. Spawn from BP_SpawnBoltProjectile in
// UMageSkillComponent and call InitTowardTarget(). The actor handles seek,
// impact, and self-destruction.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MageSkillTypes.h"
#include "MageBoltProjectile.generated.h"

class UPaperFlipbookComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;

UCLASS(Abstract, Blueprintable)
class AMageBoltProjectile : public AActor
{
    GENERATED_BODY()

public:
    AMageBoltProjectile();

    UFUNCTION(BlueprintCallable, Category="MageBolt")
    void InitTowardTarget(const FMageSkillRow& InRow, AActor* Target, float Speed = 1500.f);

    /** True for hitscan-style logic (damage already applied on spawn) so we just play VFX. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageBolt")
    bool bHitscan = true;

    /** Lifetime fail-safe in case target dies/teleports. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MageBolt")
    float MaxLifetime = 3.0f;

    /** Optional Paper2D flipbook (assign in BP child). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageBolt")
    UPaperFlipbookComponent* Flipbook = nullptr;

    /** Optional Niagara visual (assign in BP child). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageBolt")
    UNiagaraComponent* NiagaraFx = nullptr;

    /** Movement component for clean tracking. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MageBolt")
    UProjectileMovementComponent* Movement = nullptr;

    /** Called on impact — implement to spawn impact VFX and/or apply damage. */
    UFUNCTION(BlueprintImplementableEvent, Category="MageBolt")
    void BP_OnImpact(AActor* Target, FVector HitLocation);

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    UPROPERTY() FMageSkillRow Row;
    UPROPERTY() TWeakObjectPtr<AActor> SeekTarget;

    float Elapsed = 0.f;
};
