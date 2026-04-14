// CombatActionSubsystem.h — Handles ALL server combat response events.
// Phase 2 of Blueprint-to-C++ migration. Replaces BP_SocketManager combat handlers (~432 nodes).
// Owns confirmed auto-attack state (C1 audit fix). Target frame (Z=9). Death overlay (Z=40).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CombatActionSubsystem.generated.h"

UCLASS()
class SABRIMMO_API UCombatActionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Public state (C1: this subsystem owns confirmed auto-attack state) ----
	bool IsAutoAttacking() const { return bIsAutoAttacking; }
	bool IsDead() const { return bDeathOverlayVisible; }
	float GetAttackRange() const { return AttackRange; }
	int32 GetLockedTargetId() const { return LockedTargetId; }

	// Re-enable bOrientRotationToMovement. Called by PlayerInputSubsystem when
	// the player clicks ground to stop attacking (before server round-trip).
	void RestoreOrientToMovement();

	// ---- Target frame data (read by STargetFrameWidget via TAttribute lambdas) ----
	FString TargetFrameName;
	float TargetFrameHP = 0.f;
	float TargetFrameMaxHP = 0.f;
	bool bTargetFrameIsEnemy = false;

private:
	// ---- Local player info ----
	int32 LocalCharacterId = 0;

	// ---- Confirmed auto-attack state (set by server events, not clicks) ----
	bool bIsAutoAttacking = false;
	float AttackRange = 150.f;
	int32 LockedTargetId = 0;

	// ---- Widget state ----
	bool bTargetFrameVisible = false;
	bool bDeathOverlayVisible = false;
	TSharedPtr<SWidget> TargetFrameWidget;
	TSharedPtr<SWidget> TargetFrameWrapper;
	TSharedPtr<SWidget> TargetFrameViewportOverlay;  // SWeakWidget passed to AddViewportWidgetContent
	TSharedPtr<SWidget> DeathOverlayWidget;
	TSharedPtr<SWidget> DeathOverlayWrapper;
	TSharedPtr<SWidget> DeathOverlayViewportOverlay;  // SWeakWidget passed to AddViewportWidgetContent

	// ---- Hit sound assets (first audio system in project) ----
	UPROPERTY()
	TArray<TObjectPtr<USoundBase>> NormalHitSounds;

	UPROPERTY()
	TObjectPtr<USoundBase> CritHitSound = nullptr;

	// ---- Readiness guard (prevents ProcessEvent during PostLoad) ----
	bool bReadyToProcess = false;

	// ---- Socket event handlers ----
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data);
	void HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data);
	void HandleTargetLost(const TSharedPtr<FJsonValue>& Data);
	void HandleOutOfRange(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatDeath(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatError(const TSharedPtr<FJsonValue>& Data);
	void HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data);
	void HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data);

	// ---- Actor resolution (Phase 3: direct subsystem lookup) ----
	AActor* FindEnemyActor(int32 EnemyId);
	AActor* FindPlayerActor(int32 CharacterId);

	// ---- Animation ----
	void PlayAttackAnimationOnActor(AActor* Actor, const FVector& TargetPosition);

	// ---- Widget management ----
	void ShowTargetFrame();
	void HideTargetFrame();
	void ShowDeathOverlay();
	void HideDeathOverlay();

	// ---- State helpers ----
	void ClearAutoAttackState();
	void StopPawnMovement();
	void RotatePawnToward(AActor* Target);
	void SetOrientToMovement(bool bEnable);
	APlayerController* GetLocalPC() const;
};
