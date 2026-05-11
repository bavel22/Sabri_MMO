// MageFireWallActor.h
// One actor per Fire Wall cell. Plays burst -> persistent pillar loop. Tracks hits;
// despawns when HitsRemaining<=0 OR Lifetime expires (whichever comes first), matching
// classic RO behavior.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MageSkillTypes.h"
#include "MageFireWallActor.generated.h"

class UPaperFlipbookComponent;
class UNiagaraComponent;
class UBoxComponent;

UCLASS(Abstract, Blueprintable)
class AMageFireWallActor : public AActor
{
    GENERATED_BODY()

public:
    AMageFireWallActor();

    UFUNCTION(BlueprintCallable, Category="FireWall")
    void InitCell(const FMageSkillRow& InRow);

    /** Trigger volume that calls back into BP/C++ when an enemy enters. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UBoxComponent* TriggerBox = nullptr;

    /** Burst flipbook component (one-shot). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UPaperFlipbookComponent* Burst = nullptr;
    /** Persistent loop pillar flipbook. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UPaperFlipbookComponent* Pillar = nullptr;
    /** Optional Niagara override. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) UNiagaraComponent* PillarNiagara = nullptr;

    /** Called every time a victim is ticked by the wall. Implement to apply damage. */
    UFUNCTION(BlueprintImplementableEvent, Category="FireWall")
    void BP_OnHitVictim(AActor* Victim);

    /** Called once when the cell is despawning. */
    UFUNCTION(BlueprintImplementableEvent, Category="FireWall")
    void BP_OnExpire();

    /** Called by the BP graph when an actor enters or stays in the wall (every tick interval). */
    UFUNCTION(BlueprintCallable, Category="FireWall")
    void RegisterHit(AActor* Victim);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY() FMageSkillRow Row;
    int32 HitsRemaining = 0;
    float TimeRemaining = 0.f;
    float HitTickInterval = 0.4f; // RO default-ish
    float HitTickElapsed  = 0.f;
};
