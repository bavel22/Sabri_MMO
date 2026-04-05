// TargetingSubsystem.h — Per-tick hover detection, cursor switching, hover indicator.
// Phase 2 of Blueprint-to-C++ migration.
// Does NOT handle clicks or own auto-attack state (C1 audit fix).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TargetingSubsystem.generated.h"

UENUM()
enum class ETargetType : uint8
{
	None,
	Enemy,
	Player,
	NPC,
	GroundItem
};

UCLASS()
class SABRIMMO_API UTargetingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- Hover state getters ----
	AActor* GetHoveredActor() const { return HoveredActor.Get(); }
	ETargetType GetHoveredTargetType() const { return HoveredTargetType; }
	int32 GetHoveredTargetId() const { return HoveredTargetId; }

	// ---- Actor classification (shared utility, uses property reflection — M2 audit fix) ----
	static int32 GetEnemyIdFromActor(AActor* Actor);

private:
	// ---- Hover state ----
	TWeakObjectPtr<AActor> HoveredActor;
	ETargetType HoveredTargetType = ETargetType::None;
	int32 HoveredTargetId = 0;

	// ---- Tick ----
	FTimerHandle HoverTraceTimer;
	void PerformHoverTrace();

	// ---- Classification ----
	ETargetType ClassifyActor(AActor* Actor, int32& OutId) const;

	// ---- Cursor + indicator ----
	void UpdateCursor(ETargetType Type);
	void ShowHoverIndicator(AActor* Actor);
	void HideHoverIndicator(AActor* Actor);

	// ---- Helpers ----
	APlayerController* GetLocalPC() const;
};
