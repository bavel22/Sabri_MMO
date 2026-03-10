// BasicInfoSubsystem.cpp — Implementation of the Basic Info HUD subsystem

#include "BasicInfoSubsystem.h"
#include "SBasicInfoWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"

DEFINE_LOG_CATEGORY_STATIC(LogBasicInfo, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UBasicInfoSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UBasicInfoSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	PopulateNameFromGameInstance();

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("combat:health_update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleHealthUpdate(D); });
		Router->RegisterHandler(TEXT("combat:damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("skill:effect_damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("combat:death"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDeath(D); });
		Router->RegisterHandler(TEXT("combat:respawn"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatRespawn(D); });
		Router->RegisterHandler(TEXT("player:stats"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerStats(D); });
		Router->RegisterHandler(TEXT("exp:gain"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleExpGain(D); });
		Router->RegisterHandler(TEXT("exp:level_up"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleExpLevelUp(D); });
		Router->RegisterHandler(TEXT("player:joined"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerJoined(D); });
		Router->RegisterHandler(TEXT("shop:bought"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopTransaction(D); });
		Router->RegisterHandler(TEXT("shop:sold"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopTransaction(D); });
		Router->RegisterHandler(TEXT("inventory:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryData(D); });
	}

	// Only show widget and request data if socket is connected (game level, not login)
	if (GI->IsSocketConnected())
	{
		GI->EmitSocketEvent(TEXT("player:request_stats"), TEXT("{}"));
		GI->EmitSocketEvent(TEXT("inventory:load"), TEXT("{}"));
		ShowWidget();
	}

	UE_LOG(LogBasicInfo, Log, TEXT("BasicInfoSubsystem started — events registered via EventRouter."));
}

void UBasicInfoSubsystem::Deinitialize()
{
	HideWidget();

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
// Widget visibility
// ============================================================

void UBasicInfoSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	BasicInfoWidget = SNew(SBasicInfoWidget).Subsystem(this);

	// Wrap in an alignment box pinned to top-left.
	// SelfHitTestInvisible lets clicks pass through the empty viewport area.
	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			BasicInfoWidget.ToSharedRef()
		];

	ViewportOverlay =
		SNew(SWeakWidget)
		.PossiblyNullContent(AlignmentWrapper);

	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 10);
	bWidgetAdded = true;

	UE_LOG(LogBasicInfo, Log, TEXT("BasicInfo widget added to viewport."));
}

void UBasicInfoSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (ViewportOverlay.IsValid())
			{
				VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
			}
		}
	}

	BasicInfoWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
}

bool UBasicInfoSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded;
}

// ============================================================
// Event Handlers
// ============================================================

void UBasicInfoSubsystem::HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only update for our local character
	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;
	if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

	double H = 0, MH = 0, M = 0, MM = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);
	Obj->TryGetNumberField(TEXT("mana"),      M);
	Obj->TryGetNumberField(TEXT("maxMana"),   MM);

	CurrentHP = (int32)H;
	MaxHP     = FMath::Max((int32)MH, 1);
	CurrentSP = (int32)M;
	MaxSP     = FMath::Max((int32)MM, 1);
}

void UBasicInfoSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only update if WE are the target
	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	int32 TargetId = (int32)TargetIdD;
	if (LocalCharacterId > 0 && TargetId != LocalCharacterId) return;

	// combat:damage uses isEnemy — skip if this is an enemy target
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return;

	double TH = 0, TMH = 0;
	Obj->TryGetNumberField(TEXT("targetHealth"),    TH);
	Obj->TryGetNumberField(TEXT("targetMaxHealth"), TMH);

	CurrentHP = FMath::Max((int32)TH, 0);
	if ((int32)TMH > 0) MaxHP = (int32)TMH;

	UE_LOG(LogBasicInfo, Verbose, TEXT("HandleCombatDamage — HP: %d/%d"), CurrentHP, MaxHP);
}

void UBasicInfoSubsystem::HandleCombatDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only update if WE are the killed player
	double KilledIdD = 0;
	Obj->TryGetNumberField(TEXT("killedId"), KilledIdD);
	int32 KilledId = (int32)KilledIdD;
	if (LocalCharacterId > 0 && KilledId != LocalCharacterId) return;

	CurrentHP = 0;

	double TMH = 0;
	Obj->TryGetNumberField(TEXT("targetMaxHealth"), TMH);
	if ((int32)TMH > 0) MaxHP = (int32)TMH;

	UE_LOG(LogBasicInfo, Log, TEXT("HandleCombatDeath — player died, HP set to 0"));
}

void UBasicInfoSubsystem::HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only update if WE are the respawning player
	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;
	if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

	double H = 0, MH = 0, M = 0, MM = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);
	Obj->TryGetNumberField(TEXT("mana"),      M);
	Obj->TryGetNumberField(TEXT("maxMana"),   MM);

	CurrentHP = (int32)H;
	MaxHP     = FMath::Max((int32)MH, 1);
	CurrentSP = (int32)M;
	MaxSP     = FMath::Max((int32)MM, 1);

	UE_LOG(LogBasicInfo, Log, TEXT("HandleCombatRespawn — HP: %d/%d, SP: %d/%d"), CurrentHP, MaxHP, CurrentSP, MaxSP);
}

void UBasicInfoSubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Filter: only accept stats for our local character
	double CharIdD = 0;
	if (Obj->TryGetNumberField(TEXT("characterId"), CharIdD))
	{
		int32 CharId = (int32)CharIdD;
		if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;
	}

	// Parse derived stats for MaxHP/MaxSP
	const TSharedPtr<FJsonObject>* DerivedPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("derived"), DerivedPtr) && DerivedPtr)
	{
		double MH = 0, MS = 0;
		(*DerivedPtr)->TryGetNumberField(TEXT("maxHP"), MH);
		(*DerivedPtr)->TryGetNumberField(TEXT("maxSP"), MS);
		if (MH > 0) MaxHP = (int32)MH;
		if (MS > 0) MaxSP = (int32)MS;
	}

	// Parse base stats for STR (used in weight calc)
	const TSharedPtr<FJsonObject>* StatsPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("stats"), StatsPtr) && StatsPtr)
	{
		double StrVal = 0;
		(*StatsPtr)->TryGetNumberField(TEXT("str"), StrVal);
		if (StrVal > 0) STR = (int32)StrVal;
		RecalcMaxWeight();
	}

	// Parse EXP payload
	const TSharedPtr<FJsonObject>* ExpPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("exp"), ExpPtr) && ExpPtr)
	{
		ParseExpPayload(*ExpPtr);
	}
}

void UBasicInfoSubsystem::HandleExpGain(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TSharedPtr<FJsonObject>* ExpPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("exp"), ExpPtr) && ExpPtr)
	{
		ParseExpPayload(*ExpPtr);
	}
}

void UBasicInfoSubsystem::HandleExpLevelUp(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Filter: exp:level_up is broadcast — only accept our own
	double CharIdD = 0;
	if (Obj->TryGetNumberField(TEXT("characterId"), CharIdD))
	{
		int32 CharId = (int32)CharIdD;
		if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;
	}

	const TSharedPtr<FJsonObject>* ExpPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("exp"), ExpPtr) && ExpPtr)
	{
		ParseExpPayload(*ExpPtr);
	}
}

void UBasicInfoSubsystem::HandlePlayerJoined(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Z = 0;
	Obj->TryGetNumberField(TEXT("zuzucoin"), Z);
	Zuzucoin = (int32)Z;

	UE_LOG(LogBasicInfo, Log, TEXT("Player joined — Zuzucoin: %d"), Zuzucoin);
}

void UBasicInfoSubsystem::HandleShopTransaction(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double NewZ = 0;
	if (Obj->TryGetNumberField(TEXT("newZuzucoin"), NewZ))
	{
		Zuzucoin = (int32)NewZ;
	}
}

void UBasicInfoSubsystem::HandleInventoryData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Update zuzucoin if present in inventory:data payload
	double ZI = 0;
	if (Obj->TryGetNumberField(TEXT("zuzucoin"), ZI))
	{
		Zuzucoin = (int32)ZI;
	}

	// Calculate total weight from items array
	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		int32 TotalWeight = 0;
		for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObj) && ItemObj)
			{
				double W = 0, Q = 1;
				(*ItemObj)->TryGetNumberField(TEXT("weight"), W);
				(*ItemObj)->TryGetNumberField(TEXT("quantity"), Q);
				TotalWeight += (int32)W * FMath::Max((int32)Q, 1);
			}
		}
		CurrentWeight = TotalWeight;
	}
}

// ============================================================
// Shared EXP payload parser
// ============================================================

void UBasicInfoSubsystem::ParseExpPayload(const TSharedPtr<FJsonObject>& ExpObj)
{
	if (!ExpObj.IsValid()) return;

	double BL = 0, JL = 0;
	ExpObj->TryGetNumberField(TEXT("baseLevel"), BL);
	ExpObj->TryGetNumberField(TEXT("jobLevel"),  JL);
	if (BL > 0) BaseLevel = (int32)BL;
	if (JL > 0) JobLevel  = (int32)JL;

	double BE = 0, BEN = 0, JE = 0, JEN = 0;
	ExpObj->TryGetNumberField(TEXT("baseExp"),     BE);
	ExpObj->TryGetNumberField(TEXT("baseExpNext"), BEN);
	ExpObj->TryGetNumberField(TEXT("jobExp"),      JE);
	ExpObj->TryGetNumberField(TEXT("jobExpNext"),  JEN);

	BaseExp     = (int64)BE;
	BaseExpNext = FMath::Max((int64)BEN, (int64)1);
	JobExp      = (int64)JE;
	JobExpNext  = FMath::Max((int64)JEN, (int64)1);

	// Job class display name
	FString JCDisplay;
	if (ExpObj->TryGetStringField(TEXT("jobClassDisplayName"), JCDisplay))
	{
		JobClassDisplayName = JCDisplay;
	}
}

// ============================================================
// Populate player name from GameInstance
// ============================================================

void UBasicInfoSubsystem::PopulateNameFromGameInstance()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	if (!SelChar.Name.IsEmpty())
	{
		PlayerName = SelChar.Name;
	}
	if (!SelChar.JobClass.IsEmpty())
	{
		JobClassDisplayName = SelChar.JobClass;
	}
	LocalCharacterId = SelChar.CharacterId;

	// Seed initial values from character data (may be overwritten by socket events)
	if (SelChar.Health > 0)
	{
		CurrentHP = SelChar.Health;
		MaxHP     = SelChar.Health;  // Assume full HP on login until server corrects
	}
	if (SelChar.Mana > 0)
	{
		CurrentSP = SelChar.Mana;
		MaxSP     = SelChar.Mana;   // Assume full SP on login until server corrects
	}
	if (SelChar.Level > 0)  BaseLevel = SelChar.Level;
	if (SelChar.JobLevel > 0) JobLevel = SelChar.JobLevel;
	BaseExp = SelChar.BaseExp;
	JobExp  = SelChar.JobExp;
}

// ============================================================
// Weight formula — RO-style: 2000 + STR*30
// ============================================================

void UBasicInfoSubsystem::RecalcMaxWeight()
{
	MaxWeight = 2000 + STR * 30;
}
