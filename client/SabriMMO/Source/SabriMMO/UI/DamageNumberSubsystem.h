// DamageNumberSubsystem.h — UWorldSubsystem that manages the RO-style
// damage number overlay. Wraps Socket.io combat events, projects world
// positions to screen space, and feeds the SDamageNumberOverlay widget.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "DamageNumberSubsystem.generated.h"

class USocketIOClientComponent;
class SDamageNumberOverlay;
struct FSIOBoundEvent;
enum class EDamagePopType : uint8;

UCLASS()
class SABRIMMO_API UDamageNumberSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	// ---- socket event wrapping ----
	void TryWrapSocketEvents();
	void WrapSingleEvent(const FString& EventName,
		TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);

	USocketIOClientComponent* FindSocketIOComponent() const;

	// ---- event handlers ----
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);

	// ---- overlay management ----
	void ShowOverlay();
	void HideOverlay();

	// ---- world-to-screen projection ----
	bool ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;

	// ---- spawn a damage pop-up from parsed event data ----
	void SpawnDamagePop(int32 Damage, bool bIsCritical, bool bIsEnemy,
		int32 AttackerId, int32 TargetId,
		const FVector& TargetWorldPos,
		const FString& HitType = TEXT("normal"),
		const FString& Element = TEXT("neutral"));

	// ---- state ----
	bool bEventsWrapped = false;
	bool bOverlayAdded = false;
	int32 LocalCharacterId = 0;

	// Vertical offset in world units to position numbers above character's head
	static constexpr float HEAD_OFFSET_Z = 120.0f;

	FTimerHandle BindCheckTimer;

	TSharedPtr<SDamageNumberOverlay> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;

	TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
