// CombatStatsSubsystem.cpp — Tracks RO pre-renewal derived combat stats
// and manages the SCombatStatsWidget overlay.
// F8 key toggle is handled by SabriMMOPlayerController via Enhanced Input.

#include "CombatStatsSubsystem.h"
#include "SCombatStatsWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatStats, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UCombatStatsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UCombatStatsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register socket event handlers via persistent EventRouter
	if (USocketEventRouter* Router = GI->GetEventRouter())
	{
		Router->RegisterHandler(TEXT("player:stats"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerStats(D); });
	}

	// Request current stats so the widget is populated immediately (only when connected)
	if (GI->IsSocketConnected())
	{
		GI->EmitSocketEvent(TEXT("player:request_stats"), TEXT("{}"));
	}

	UE_LOG(LogCombatStats, Log, TEXT("CombatStatsSubsystem started — events registered. LocalCharId=%d."), LocalCharacterId);
}

void UCombatStatsSubsystem::Deinitialize()
{
	if (bWidgetVisible)
	{
		ToggleWidget(); // Remove widget
	}

	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	Super::Deinitialize();
}

// ============================================================
// Handle player:stats event — extract base + derived stats
// ============================================================

void UCombatStatsSubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Check if this is for our character
	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	if ((int32)CharIdD != LocalCharacterId && LocalCharacterId != 0) return;

	// ── Parse effective stats (base + equipment) ──
	const TSharedPtr<FJsonObject>* StatsObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("stats"), StatsObj) && StatsObj)
	{
		double Val = 0;
		if ((*StatsObj)->TryGetNumberField(TEXT("str"), Val)) STR = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("agi"), Val)) AGI = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("vit"), Val)) VIT = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("int"), Val)) INT_Stat = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("dex"), Val)) DEX = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("luk"), Val)) LUK = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("level"), Val)) BaseLevel = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("weaponATK"), Val)) WeaponATK = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("passiveATK"), Val)) PassiveATK = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("hardDef"), Val)) HardDEF = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("hardMdef"), Val)) HardMDEF = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("weaponMATK"), Val)) WeaponMATK = (int32)Val;
		if ((*StatsObj)->TryGetNumberField(TEXT("statPoints"), Val)) StatPoints = (int32)Val;
	}

	// ── Parse base stats (without equipment bonuses) ──
	const TSharedPtr<FJsonObject>* BaseStatsObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("baseStats"), BaseStatsObj) && BaseStatsObj)
	{
		double Val = 0;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("str"), Val)) BaseSTR = (int32)Val;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("agi"), Val)) BaseAGI = (int32)Val;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("vit"), Val)) BaseVIT = (int32)Val;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("int"), Val)) BaseINT = (int32)Val;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("dex"), Val)) BaseDEX = (int32)Val;
		if ((*BaseStatsObj)->TryGetNumberField(TEXT("luk"), Val)) BaseLUK = (int32)Val;
	}

	// ── Parse stat allocation costs ──
	const TSharedPtr<FJsonObject>* CostsObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("statCosts"), CostsObj) && CostsObj)
	{
		double Val = 0;
		if ((*CostsObj)->TryGetNumberField(TEXT("str"), Val)) CostSTR = (int32)Val;
		if ((*CostsObj)->TryGetNumberField(TEXT("agi"), Val)) CostAGI = (int32)Val;
		if ((*CostsObj)->TryGetNumberField(TEXT("vit"), Val)) CostVIT = (int32)Val;
		if ((*CostsObj)->TryGetNumberField(TEXT("int"), Val)) CostINT = (int32)Val;
		if ((*CostsObj)->TryGetNumberField(TEXT("dex"), Val)) CostDEX = (int32)Val;
		if ((*CostsObj)->TryGetNumberField(TEXT("luk"), Val)) CostLUK = (int32)Val;
	}

	// ── Parse derived stats ──
	const TSharedPtr<FJsonObject>* DerivedObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("derived"), DerivedObj) && DerivedObj)
	{
		double Val = 0;
		if ((*DerivedObj)->TryGetNumberField(TEXT("statusATK"), Val)) StatusATK = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("statusMATK"), Val)) StatusMATK = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("hit"), Val)) HIT = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("flee"), Val)) FLEE = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("critical"), Val)) Critical = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("perfectDodge"), Val)) PerfectDodge = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("softDEF"), Val)) SoftDEF = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("softMDEF"), Val)) SoftMDEF = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("matkMin"), Val)) MATKMin = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("matkMax"), Val)) MATKMax = (int32)Val;
		if ((*DerivedObj)->TryGetNumberField(TEXT("aspd"), Val)) ASPD = (int32)Val;
	}

	// ── Parse dual wield info ──
	const TSharedPtr<FJsonObject>* DualWieldObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("dualWield"), DualWieldObj) && DualWieldObj)
	{
		bool bDW = false;
		if ((*DualWieldObj)->TryGetBoolField(TEXT("isDualWielding"), bDW)) bIsDualWielding = bDW;
		double Val = 0;
		if ((*DualWieldObj)->TryGetNumberField(TEXT("weaponATK_right"), Val)) WeaponATK_Right = (int32)Val;
		if ((*DualWieldObj)->TryGetNumberField(TEXT("weaponATK_left"), Val)) WeaponATK_Left = (int32)Val;
		if ((*DualWieldObj)->TryGetNumberField(TEXT("rightHandDamagePercent"), Val)) RightHandDamagePercent = (int32)Val;
		if ((*DualWieldObj)->TryGetNumberField(TEXT("leftHandDamagePercent"), Val)) LeftHandDamagePercent = (int32)Val;
		FString Str;
		if ((*DualWieldObj)->TryGetStringField(TEXT("weaponTypeRight"), Str)) WeaponTypeRight = Str;
		if ((*DualWieldObj)->TryGetStringField(TEXT("weaponTypeLeft"), Str)) WeaponTypeLeft = Str;
		if ((*DualWieldObj)->TryGetStringField(TEXT("elementRight"), Str)) ElementRight = Str;
		if ((*DualWieldObj)->TryGetStringField(TEXT("elementLeft"), Str)) ElementLeft = Str;
	}
	else
	{
		bIsDualWielding = false;
	}

	UE_LOG(LogCombatStats, Log, TEXT("Stats updated: ATK=%d+%d(+%d passive) MATK=%d~%d HIT=%d FLEE=%d CRI=%d PD=%d DEF=%d+%d MDEF=%d+%d ASPD=%d StatPts=%d%s"),
		StatusATK, WeaponATK, PassiveATK, MATKMin, MATKMax, HIT, FLEE, Critical, PerfectDodge, HardDEF, SoftDEF, HardMDEF, SoftMDEF, ASPD, StatPoints,
		bIsDualWielding ? *FString::Printf(TEXT(" DW:R=%d(%d%%) L=%d(%d%%)"), WeaponATK_Right, RightHandDamagePercent, WeaponATK_Left, LeftHandDamagePercent) : TEXT(""));
}

// ============================================================
// Widget toggle
// ============================================================

void UCombatStatsSubsystem::ToggleWidget()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	if (bWidgetVisible)
	{
		// Hide
		if (ViewportOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
		StatsWidget.Reset();
		ViewportOverlay.Reset();
		bWidgetVisible = false;
		UE_LOG(LogCombatStats, Log, TEXT("Combat stats widget hidden."));
	}
	else
	{
		// Show
		StatsWidget = SNew(SCombatStatsWidget).Subsystem(this);

		ViewportOverlay =
			SNew(SWeakWidget)
			.PossiblyNullContent(StatsWidget);

		// Z-order 12 = between BasicInfo (10) and SkillTree (15)
		ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 12);
		bWidgetVisible = true;
		UE_LOG(LogCombatStats, Log, TEXT("Combat stats widget shown (Z=12)."));
	}
}

bool UCombatStatsSubsystem::IsWidgetVisible() const
{
	return bWidgetVisible;
}

// ============================================================
// Allocate a stat point via SocketIO (sends player:allocate_stat)
// ============================================================

void UCombatStatsSubsystem::AllocateStat(const FString& StatName)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("statName"), StatName);

	GI->EmitSocketEvent(TEXT("player:allocate_stat"), Payload);
	UE_LOG(LogCombatStats, Log, TEXT("Sent player:allocate_stat for %s"), *StatName);
}
