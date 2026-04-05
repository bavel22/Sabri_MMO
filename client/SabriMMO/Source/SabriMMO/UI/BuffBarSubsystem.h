// BuffBarSubsystem.h — UWorldSubsystem that manages the Buff Bar Slate widget.
// Tracks active buffs (positive stat mods) and status effects (CC/DoT conditions).
// Registers Socket.io event handlers via the persistent EventRouter:
// status:applied, status:removed, buff:list, skill:buff_applied, skill:buff_removed.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "BuffBarSubsystem.generated.h"

class SBuffBarWidget;

/** Info about an active buff (Provoke, Endure, Blessing, etc.) */
USTRUCT()
struct FActiveBuffInfo
{
	GENERATED_BODY()

	FString Name;          // "provoke", "endure", "blessing"
	FString DisplayName;   // "Provoke", "Endure", "Blessing"
	FString Abbrev;        // "PRV", "END", "BLS"
	int32 SkillId = 0;
	float DurationMs = 0;  // Total duration in ms
	float RemainingMs = 0; // Ms remaining at time of event
	double ReceivedAt = 0; // FPlatformTime::Seconds() when received
	FString Category;      // "buff" or "debuff"
};

/** Info about an active status effect (stun, freeze, poison, etc.) */
USTRUCT()
struct FActiveStatusInfo
{
	GENERATED_BODY()

	FString Type;          // "stun", "freeze", "poison"
	float DurationMs = 0;  // Total duration in ms
	float RemainingMs = 0; // Ms remaining at time of event
	double ReceivedAt = 0; // FPlatformTime::Seconds() when received
};

UCLASS()
class SABRIMMO_API UBuffBarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- public data (read by widget) ----
	TArray<FActiveBuffInfo> ActiveBuffs;
	TArray<FActiveStatusInfo> ActiveStatuses;

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

	/** Check if a buff with the given name is currently active (e.g. "hiding", "play_dead") */
	bool HasBuff(const FString& BuffName) const;

	/** Get remaining seconds for a status/buff entry */
	static float GetRemainingSeconds(float RemainingMs, double ReceivedAt);

private:
	// ---- event handlers ----
	void HandleStatusApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffApplied(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data);
	void HandleBuffList(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	bool bWidgetAdded   = false;
	int32 LocalCharacterId = 0;

	TSharedPtr<SBuffBarWidget> Widget;
	TSharedPtr<SWidget>        AlignmentWrapper;
	TSharedPtr<SWidget>        ViewportOverlay;
};
