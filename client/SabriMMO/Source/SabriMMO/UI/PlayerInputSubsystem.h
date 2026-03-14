// PlayerInputSubsystem.h — Local player click routing.
// Handles: click-to-move (NavMesh), click-to-attack (walk-to-range + emit),
// click-to-interact (NPCs). Uses a timer for walk-to-target polling.
// This subsystem ONLY handles INPUT and EMITTING events.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerInputSubsystem.generated.h"

class ASabriMMOCharacter;

UCLASS()
class SABRIMMO_API UPlayerInputSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// --- Public API ---
	void OnLeftClickFromCharacter(ASabriMMOCharacter* Character);
	void StopAutoAttack();
	bool IsAutoAttacking() const { return bIsAutoAttacking; }
	int32 GetAttackTargetId() const { return AttackTargetId; }
	bool IsAttackTargetEnemy() const { return bAttackTargetIsEnemy; }

private:
	// --- Auto-attack state ---
	bool bIsAutoAttacking = false;
	int32 AttackTargetId = 0;
	bool bAttackTargetIsEnemy = false;
	TWeakObjectPtr<AActor> AttackTargetActor;
	bool bWalkingToAttack = false;
	float AttackRange = 150.f;

	// --- Walk-to-interact state ---
	TWeakObjectPtr<AActor> PendingInteractNPC;
	float InteractRange = 200.f;
	double LastInteractTime = 0.0;

	// --- Click-to-move state ---
	bool bIsClickMoving = false;

	// --- Timer for walk-to polling (replaces FTickableGameObject) ---
	FTimerHandle WalkPollTimer;
	void OnWalkPollTick();

	// --- Click routing ---
	void ProcessClickOnEnemy(AActor* EnemyActor);
	void ProcessClickOnNPC(AActor* NPCActor);
	void ProcessClickOnGround(const FVector& Location);

	// --- Movement helpers ---
	void MoveToLocation(const FVector& Destination);
	void CancelMovement();

	// --- Enemy ID extraction ---
	int32 GetEnemyIdFromActor(AActor* Actor) const;

	// --- Helpers ---
	APawn* GetLocalPawn() const;
	APlayerController* GetLocalPC() const;
};
