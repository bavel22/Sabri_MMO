// DamageNumberSubsystem.h — UWorldSubsystem that manages the RO-style
// damage number overlay. Registers Socket.io event handlers via the persistent
// EventRouter, projects world positions to screen space, and feeds the
// SDamageNumberOverlay widget.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "DamageNumberSubsystem.generated.h"

class SDamageNumberOverlay;
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

	// ---- options flags (set by OptionsSubsystem) ----
	bool bDamageNumbersEnabled = true;
	bool bShowMissText = true;
	float DamageNumberScale = 1.0f;  // 0.75 = small, 1.0 = normal, 1.25 = large

private:
	// ---- event handlers ----
	void HandleCombatDamage(const TSharedPtr<FJsonValue>& Data);
	void HandleCombatBlocked(const TSharedPtr<FJsonValue>& Data);
	void HandleStatusTick(const TSharedPtr<FJsonValue>& Data);
	void HandleStatusApplied(const TSharedPtr<FJsonValue>& Data);

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
		const FString& Element = TEXT("neutral"),
		const FLinearColor* CustomColor = nullptr);

	// ---- spawn a floating text label (e.g. "Poisoned!", "Stunned!") ----
	void SpawnTextPop(const FString& Text, const FLinearColor& Color,
		bool bIsEnemy, int32 TargetId, const FVector& TargetWorldPos);

	// ---- resolve target world position from ID ----
	bool ResolveTargetPosition(bool bIsEnemy, int32 TargetId, FVector& OutPos) const;

	// ---- map status type to display name + color ----
	static FString GetStatusDisplayName(const FString& StatusType);
	static FLinearColor GetStatusColor(const FString& StatusType);

	// ---- combo total tracking ----
	struct FComboTracker
	{
		int32 TotalDamage = 0;
		int32 ExpectedHits = 0;
		int32 HitsReceived = 0;
		int32 TargetId = 0;
		bool bIsEnemy = false;
		FVector LastTargetPos = FVector::ZeroVector;
	};
	TMap<FString, FComboTracker> ActiveCombos;

	void SpawnComboTotal(int32 TotalDamage, bool bIsEnemy, int32 TargetId, const FVector& Pos);

	// ---- state ----
	UPROPERTY()
	UTexture2D* CritStarburstTexture = nullptr;

	bool bOverlayAdded = false;
	int32 LocalCharacterId = 0;

	// Vertical offset in world units to position numbers above character's head
	static constexpr float HEAD_OFFSET_Z = 120.0f;

	TSharedPtr<SDamageNumberOverlay> OverlayWidget;
	TSharedPtr<SWidget> ViewportOverlay;
};
