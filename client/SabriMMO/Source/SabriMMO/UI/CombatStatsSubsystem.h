// CombatStatsSubsystem.h — UWorldSubsystem that tracks and exposes
// all RO pre-renewal derived combat stats from the server's player:stats event.
// Feeds data into SCombatStatsWidget for display.
// F8 key toggle is handled by SabriMMOPlayerController via Enhanced Input.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "CombatStatsSubsystem.generated.h"

class SCombatStatsWidget;

UCLASS()
class SABRIMMO_API UCombatStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- public data fields (read by SCombatStatsWidget) ----
	// Effective stats (base + equipment, displayed in stat window)
	int32 STR = 1;
	int32 AGI = 1;
	int32 VIT = 1;
	int32 INT_Stat = 1;
	int32 DEX = 1;
	int32 LUK = 1;
	int32 BaseLevel = 1;

	// Base stats (without equipment, used for stat cost calc display)
	int32 BaseSTR = 1;
	int32 BaseAGI = 1;
	int32 BaseVIT = 1;
	int32 BaseINT = 1;
	int32 BaseDEX = 1;
	int32 BaseLUK = 1;

	// Stat allocation (RO Classic)
	int32 StatPoints = 0;
	int32 CostSTR = 2;
	int32 CostAGI = 2;
	int32 CostVIT = 2;
	int32 CostINT = 2;
	int32 CostDEX = 2;
	int32 CostLUK = 2;

	// Derived combat stats (from server)
	int32 StatusATK = 0;
	int32 WeaponATK = 0;
	int32 PassiveATK = 0;
	int32 ArrowATK = 0;
	int32 StatusMATK = 0;
	int32 MATKMin = 0;
	int32 MATKMax = 0;
	int32 WeaponMATK = 0;
	int32 HIT = 0;
	int32 FLEE = 0;
	int32 Critical = 0;
	int32 PerfectDodge = 0;
	int32 SoftDEF = 0;
	int32 SoftMDEF = 0;
	int32 HardDEF = 0;
	int32 HardMDEF = 0;
	int32 ASPD = 0;

	// Dual Wield (Assassin/Assassin Cross)
	bool bIsDualWielding = false;
	int32 WeaponATK_Right = 0;
	int32 WeaponATK_Left = 0;
	int32 RightHandDamagePercent = 50;
	int32 LeftHandDamagePercent = 30;
	FString WeaponTypeRight;
	FString WeaponTypeLeft;
	FString ElementRight;
	FString ElementLeft;

	// Elemental Resistances (% reduction per element)
	int32 ResistNeutral = 0;
	int32 ResistFire = 0;
	int32 ResistWater = 0;
	int32 ResistEarth = 0;
	int32 ResistWind = 0;
	int32 ResistPoison = 0;
	int32 ResistHoly = 0;
	int32 ResistDark = 0;
	int32 ResistGhost = 0;
	int32 ResistUndead = 0;

	// Block chance (Auto Guard)
	int32 BlockChance = 0;

	// Race ATK/DEF (passive + cards + buffs)
	TMap<FString, int32> RaceAtk;
	TMap<FString, int32> RaceDef;

	// Size ATK/DEF (cards)
	TMap<FString, int32> SizeAtk;
	TMap<FString, int32> SizeDef;

	// Element ATK (cards)
	TMap<FString, int32> EleAtk;

	// ---- stat allocation ----
	void AllocateStat(const FString& StatName);

	// ---- lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ---- widget visibility ----
	void ToggleWidget();
	bool IsWidgetVisible() const;
	void ToggleAdvancedWidget();
	bool IsAdvancedWidgetVisible() const;

private:
	// ---- event handlers ----
	void HandlePlayerStats(const TSharedPtr<FJsonValue>& Data);

	// ---- state ----
	bool bWidgetVisible = false;
	bool bAdvancedWidgetVisible = false;
	int32 LocalCharacterId = 0;

	TSharedPtr<SCombatStatsWidget> StatsWidget;
	TSharedPtr<SWidget> ViewportOverlay;
	TSharedPtr<class SAdvancedStatsWidget> AdvancedWidget;
	TSharedPtr<SWidget> AdvancedOverlay;
};
