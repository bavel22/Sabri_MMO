// BuffBarSubsystem.cpp — Manages buff/status effect data and Slate widget lifecycle.

#include "BuffBarSubsystem.h"
#include "SBuffBarWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

// File-local helper: get 3-letter abbreviation for a buff/status name
static FString GetBuffAbbrev(const FString& Name)
{
	// Status effects
	if (Name == TEXT("stun"))       return TEXT("STN");
	if (Name == TEXT("freeze"))     return TEXT("FRZ");
	if (Name == TEXT("stone"))      return TEXT("STN");
	if (Name == TEXT("sleep"))      return TEXT("SLP");
	if (Name == TEXT("poison"))     return TEXT("PSN");
	if (Name == TEXT("blind"))      return TEXT("BLD");
	if (Name == TEXT("silence"))    return TEXT("SIL");
	if (Name == TEXT("confusion"))  return TEXT("CNF");
	if (Name == TEXT("bleeding"))   return TEXT("BLE");
	if (Name == TEXT("curse"))      return TEXT("CRS");
	// Buffs
	if (Name == TEXT("provoke"))    return TEXT("PRV");
	if (Name == TEXT("endure"))     return TEXT("END");
	if (Name == TEXT("sight"))      return TEXT("SGT");
	if (Name == TEXT("blessing"))   return TEXT("BLS");
	if (Name == TEXT("increase_agi")) return TEXT("AGI");
	if (Name == TEXT("angelus"))    return TEXT("ANG");
	if (Name == TEXT("decrease_agi")) return TEXT("DAG");
	// Default: first 3 chars uppercase
	FString Upper = Name.ToUpper();
	return Upper.Left(3);
}

// ============================================================================
// Lifecycle
// ============================================================================

bool UBuffBarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UBuffBarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("status:applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusApplied(D); });
		Router->RegisterHandler(TEXT("status:removed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusRemoved(D); });
		Router->RegisterHandler(TEXT("skill:buff_applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleBuffApplied(D); });
		Router->RegisterHandler(TEXT("skill:buff_removed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleBuffRemoved(D); });
		Router->RegisterHandler(TEXT("buff:list"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleBuffList(D); });
	}

	// Only emit and show when socket is connected (game level)
	if (GI->IsSocketConnected())
	{
		// Request current buff/status list from server
		GI->EmitSocketEvent(TEXT("buff:request"), TEXT("{}"));

		// Show the widget
		ShowWidget();
	}

	UE_LOG(LogTemp, Log, TEXT("[BuffBar] Events registered via EventRouter. CharId=%d"), LocalCharacterId);
}

void UBuffBarSubsystem::Deinitialize()
{
	if (bWidgetAdded)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameViewportClient* ViewportClient = World->GetGameViewport();
			if (ViewportClient && ViewportOverlay.IsValid())
			{
				ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
			}
		}
		bWidgetAdded = false;
	}

	Widget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	ActiveBuffs.Empty();
	ActiveStatuses.Empty();

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

// ============================================================================
// Event Handlers
// ============================================================================

void UBuffBarSubsystem::HandleStatusApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only track our own status effects
	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return; // Only track player statuses
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	FString StatusType;
	Obj->TryGetStringField(TEXT("statusType"), StatusType);
	double DurationD = 0;
	Obj->TryGetNumberField(TEXT("duration"), DurationD);

	// Remove existing entry of same type (refresh)
	ActiveStatuses.RemoveAll([&StatusType](const FActiveStatusInfo& S) { return S.Type == StatusType; });

	FActiveStatusInfo Info;
	Info.Type = StatusType;
	Info.DurationMs = (float)DurationD;
	Info.RemainingMs = (float)DurationD;
	Info.ReceivedAt = FPlatformTime::Seconds();
	ActiveStatuses.Add(Info);

	UE_LOG(LogTemp, Log, TEXT("[BuffBar] Status applied: %s (%.1fs)"), *StatusType, DurationD / 1000.0);
}

void UBuffBarSubsystem::HandleStatusRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return;
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	FString StatusType;
	Obj->TryGetStringField(TEXT("statusType"), StatusType);

	ActiveStatuses.RemoveAll([&StatusType](const FActiveStatusInfo& S) { return S.Type == StatusType; });
	UE_LOG(LogTemp, Log, TEXT("[BuffBar] Status removed: %s"), *StatusType);
}

void UBuffBarSubsystem::HandleBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return;
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	FString BuffName;
	Obj->TryGetStringField(TEXT("buffName"), BuffName);
	double DurationD = 0;
	Obj->TryGetNumberField(TEXT("duration"), DurationD);

	// Skip if this is a status effect (Frozen, Stone Curse) — handled by status:applied
	FString BuffNameLower = BuffName.ToLower();
	if (BuffNameLower == TEXT("frozen") || BuffNameLower == TEXT("stone curse"))
		return;

	// Remove existing entry of same name (refresh)
	ActiveBuffs.RemoveAll([&BuffNameLower](const FActiveBuffInfo& B) { return B.Name == BuffNameLower; });

	FActiveBuffInfo Info;
	Info.Name = BuffNameLower;
	Info.DisplayName = BuffName;
	Info.Abbrev = GetBuffAbbrev(BuffNameLower);
	double SkillIdD = 0;
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	Info.SkillId = (int32)SkillIdD;
	Info.DurationMs = (float)DurationD;
	Info.RemainingMs = (float)DurationD;
	Info.ReceivedAt = FPlatformTime::Seconds();
	Info.Category = TEXT("buff"); // Default; server should send category field in future
	ActiveBuffs.Add(Info);

	UE_LOG(LogTemp, Log, TEXT("[BuffBar] Buff applied: %s abbrev=%s (%.1fs) — total buffs: %d, total statuses: %d"),
		*BuffName, *Info.Abbrev, DurationD / 1000.0, ActiveBuffs.Num(), ActiveStatuses.Num());
}

void UBuffBarSubsystem::HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	if (LocalCharacterId > 0 && (int32)TargetIdD != LocalCharacterId) return;

	FString BuffName;
	Obj->TryGetStringField(TEXT("buffName"), BuffName);
	FString BuffNameLower = BuffName.ToLower();

	// Remove from buffs
	ActiveBuffs.RemoveAll([&BuffNameLower](const FActiveBuffInfo& B) { return B.Name == BuffNameLower; });
	// Also remove from statuses (backward compat: server sends buff_removed for expired status effects)
	ActiveStatuses.RemoveAll([&BuffNameLower](const FActiveStatusInfo& S) { return S.Type == BuffNameLower; });
}

void UBuffBarSubsystem::HandleBuffList(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	if (LocalCharacterId > 0 && (int32)CharIdD != LocalCharacterId) return;

	// Clear and rebuild from full list
	ActiveBuffs.Empty();
	ActiveStatuses.Empty();

	const double Now = FPlatformTime::Seconds();

	// Parse buffs array
	const TArray<TSharedPtr<FJsonValue>>* BuffsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("buffs"), BuffsArr))
	{
		for (const auto& Val : *BuffsArr)
		{
			const TSharedPtr<FJsonObject>* BObj = nullptr;
			if (!Val->TryGetObject(BObj) || !BObj) continue;

			FActiveBuffInfo Info;
			(*BObj)->TryGetStringField(TEXT("name"), Info.Name);
			(*BObj)->TryGetStringField(TEXT("displayName"), Info.DisplayName);
			if (Info.DisplayName.IsEmpty()) Info.DisplayName = Info.Name;
			(*BObj)->TryGetStringField(TEXT("abbrev"), Info.Abbrev);
			if (Info.Abbrev.IsEmpty()) Info.Abbrev = GetBuffAbbrev(Info.Name);
			double SkillIdD = 0;
			(*BObj)->TryGetNumberField(TEXT("skillId"), SkillIdD);
			Info.SkillId = (int32)SkillIdD;
			double DurD = 0, RemD = 0;
			(*BObj)->TryGetNumberField(TEXT("duration"), DurD);
			(*BObj)->TryGetNumberField(TEXT("remainingMs"), RemD);
			Info.DurationMs = (float)DurD;
			Info.RemainingMs = (float)RemD;
			Info.ReceivedAt = Now;
			(*BObj)->TryGetStringField(TEXT("category"), Info.Category);
			if (Info.Category.IsEmpty()) Info.Category = TEXT("buff");

			ActiveBuffs.Add(Info);
		}
	}

	// Parse statuses array
	const TArray<TSharedPtr<FJsonValue>>* StatusArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("statuses"), StatusArr))
	{
		for (const auto& Val : *StatusArr)
		{
			const TSharedPtr<FJsonObject>* SObj = nullptr;
			if (!Val->TryGetObject(SObj) || !SObj) continue;

			FActiveStatusInfo Info;
			(*SObj)->TryGetStringField(TEXT("type"), Info.Type);
			double DurD = 0, RemD = 0;
			(*SObj)->TryGetNumberField(TEXT("duration"), DurD);
			(*SObj)->TryGetNumberField(TEXT("remainingMs"), RemD);
			Info.DurationMs = (float)DurD;
			Info.RemainingMs = (float)RemD;
			Info.ReceivedAt = Now;

			ActiveStatuses.Add(Info);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BuffBar] buff:list received — %d buffs, %d statuses"),
		ActiveBuffs.Num(), ActiveStatuses.Num());
}

// ============================================================================
// Widget Management
// ============================================================================

void UBuffBarSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	Widget = SNew(SBuffBarWidget).Subsystem(this);

	// Position below BasicInfoWidget (top-left, offset down)
	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			Widget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 11); // Z=11 (between BasicInfo=10 and CombatStats=12)
	bWidgetAdded = true;
}

void UBuffBarSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (ViewportClient && ViewportOverlay.IsValid())
	{
		ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}
	bWidgetAdded = false;
}

bool UBuffBarSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded;
}

float UBuffBarSubsystem::GetRemainingSeconds(float RemainingMs, double ReceivedAt)
{
	const double ElapsedSec = FPlatformTime::Seconds() - ReceivedAt;
	const float RemainingSec = (RemainingMs / 1000.0f) - (float)ElapsedSec;
	return FMath::Max(0.0f, RemainingSec);
}
