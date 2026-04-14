// DamageNumberSubsystem.cpp — Implementation of the RO-style damage number subsystem.
// Registers combat:damage and skill:effect_damage via EventRouter, projects world
// positions to screen, and feeds damage pops into the SDamageNumberOverlay.

#include "DamageNumberSubsystem.h"
#include "SDamageNumberOverlay.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "GameFramework/PlayerController.h"
#include "EnemySubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamageNumbers, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UDamageNumberSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UDamageNumberSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Resolve local character ID from GameInstance
	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;
	UE_LOG(LogDamageNumbers, Log, TEXT("LocalCharacterId resolved: %d"), LocalCharacterId);

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("combat:damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("skill:effect_damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("combat:blocked"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatBlocked(D); });
		Router->RegisterHandler(TEXT("status:tick"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusTick(D); });
		Router->RegisterHandler(TEXT("status:applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStatusApplied(D); });
	}

	// Load critical starburst texture
	CritStarburstTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/SabriMMO/UI/Textures/T_CritStarburst.T_CritStarburst"));

	// Only show overlay when socket is connected (game level)
	if (GI->IsSocketConnected())
	{
		ShowOverlay();
	}

	UE_LOG(LogDamageNumbers, Log, TEXT("DamageNumberSubsystem started — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);
}

void UDamageNumberSubsystem::Deinitialize()
{
	HideOverlay();

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

	UE_LOG(LogDamageNumbers, Log, TEXT("DamageNumberSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Overlay widget management
// ============================================================

void UDamageNumberSubsystem::ShowOverlay()
{
	if (bOverlayAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient)
	{
		UE_LOG(LogDamageNumbers, Warning, TEXT("ShowOverlay: No GameViewportClient available!"));
		return;
	}

	OverlayWidget = SNew(SDamageNumberOverlay)
		.CritStarburstTexture(CritStarburstTexture);

	ViewportOverlay =
		SNew(SWeakWidget)
		.PossiblyNullContent(OverlayWidget);

	// Z-order 20 = above BasicInfo (10) and SkillTree (15), below targeting overlay (25)
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 20);
	bOverlayAdded = true;

	UE_LOG(LogDamageNumbers, Log, TEXT("Damage number overlay added to viewport (Z=20)."));
}

void UDamageNumberSubsystem::HideOverlay()
{
	if (!bOverlayAdded) return;

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

	OverlayWidget.Reset();
	ViewportOverlay.Reset();
	bOverlayAdded = false;
}

// ============================================================
// World-to-screen projection
// ============================================================

bool UDamageNumberSubsystem::ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) return false;

	bool bSuccess = PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);
	return bSuccess;
}

// ============================================================
// Handle combat:damage / skill:effect_damage socket event
// ============================================================

void UDamageNumberSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid())
	{
		UE_LOG(LogDamageNumbers, Warning, TEXT("HandleCombatDamage: Data is invalid!"));
		return;
	}

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr)
	{
		UE_LOG(LogDamageNumbers, Warning, TEXT("HandleCombatDamage: Failed to extract JSON object!"));
		return;
	}
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// ---- Extract event fields ----
	double AttackerIdD = 0, TargetIdD = 0, DamageD = 0, HealAmountD = 0;
	Obj->TryGetNumberField(TEXT("attackerId"), AttackerIdD);
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("damage"), DamageD);
	Obj->TryGetNumberField(TEXT("healAmount"), HealAmountD);

	bool bIsCritical = false;
	Obj->TryGetBoolField(TEXT("isCritical"), bIsCritical);

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	// ---- RO damage system fields ----
	FString HitType = TEXT("normal");
	Obj->TryGetStringField(TEXT("hitType"), HitType);

	FString Element = TEXT("neutral");
	Obj->TryGetStringField(TEXT("element"), Element);

	const int32 AttackerId = (int32)AttackerIdD;
	const int32 TargetId = (int32)TargetIdD;
	const int32 Damage = (int32)DamageD;
	const int32 HealAmount = (int32)HealAmountD;

	// ---- Extract target world position ----
	double TX = 0, TY = 0, TZ = 0;
	Obj->TryGetNumberField(TEXT("targetX"), TX);
	Obj->TryGetNumberField(TEXT("targetY"), TY);
	Obj->TryGetNumberField(TEXT("targetZ"), TZ);

	const FVector TargetWorldPos((float)TX, (float)TY, (float)TZ);

	UE_LOG(LogDamageNumbers, Log, TEXT("HandleCombatDamage: attacker=%d target=%d dmg=%d heal=%d crit=%d isEnemy=%d hitType=%s ele=%s pos=(%.0f,%.0f,%.0f)"),
		AttackerId, TargetId, Damage, HealAmount, bIsCritical ? 1 : 0, bIsEnemy ? 1 : 0, *HitType, *Element, TX, TY, TZ);

	// ---- For heals, use healAmount as the display value ----
	const int32 DisplayValue = (HitType == TEXT("heal")) ? HealAmount : Damage;

	// ---- Spawn damage number for ALL combat in view (RO-style) ----
	SpawnDamagePop(DisplayValue, bIsCritical, bIsEnemy, AttackerId, TargetId, TargetWorldPos, HitType, Element);

	// ---- Combo total tracking for multi-hit skills ----
	double HitNumberD = 0, TotalHitsD = 0, SkillIdD = 0;
	Obj->TryGetNumberField(TEXT("hitNumber"), HitNumberD);
	Obj->TryGetNumberField(TEXT("totalHits"), TotalHitsD);
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	const int32 HitNumber = (int32)HitNumberD;
	const int32 TotalHits = (int32)TotalHitsD;
	const int32 SkillId = (int32)SkillIdD;

	if (TotalHits > 1 && SkillId > 0 && HitType != TEXT("heal"))
	{
		FString ComboKey = FString::Printf(TEXT("%d_%d_%d"), AttackerId, TargetId, SkillId);
		FComboTracker& C = ActiveCombos.FindOrAdd(ComboKey);
		if (HitNumber == 1)
		{
			C = FComboTracker();
			C.ExpectedHits = TotalHits;
		}
		C.TotalDamage += DisplayValue;
		C.HitsReceived++;
		C.TargetId = TargetId;
		C.bIsEnemy = bIsEnemy;
		C.LastTargetPos = TargetWorldPos;

		if (HitNumber >= TotalHits)
		{
			SpawnComboTotal(C.TotalDamage, C.bIsEnemy, C.TargetId, C.LastTargetPos);
			ActiveCombos.Remove(ComboKey);
		}
	}

	// ---- Dual Wield: second damage number for left-hand hit ----
	double Damage2D = 0;
	bool bIsDualWield = false;
	Obj->TryGetNumberField(TEXT("damage2"), Damage2D);
	Obj->TryGetBoolField(TEXT("isDualWield"), bIsDualWield);
	const int32 Damage2 = (int32)Damage2D;
	if (bIsDualWield && Damage2 > 0)
	{
		bool bIsCritical2 = false;
		Obj->TryGetBoolField(TEXT("isCritical2"), bIsCritical2);
		FString Element2 = TEXT("neutral");
		Obj->TryGetStringField(TEXT("element2"), Element2);
		// Slight upward offset so the two numbers don't overlap
		const FVector LeftHandPos = TargetWorldPos + FVector(0.f, 0.f, 30.f);
		SpawnDamagePop(Damage2, bIsCritical2, bIsEnemy, AttackerId, TargetId, LeftHandPos, TEXT("normal"), Element2);
	}
}

// ============================================================
// Handle combat:blocked — Auto Guard shield block
// ============================================================

void UDamageNumberSubsystem::HandleCombatBlocked(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !OverlayWidget.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	const int32 TargetId = (int32)TargetIdD;

	// Get target position from event data
	double TX = 0, TY = 0, TZ = 0;
	Obj->TryGetNumberField(TEXT("targetX"), TX);
	Obj->TryGetNumberField(TEXT("targetY"), TY);
	Obj->TryGetNumberField(TEXT("targetZ"), TZ);

	FVector TargetWorldPos((float)TX, (float)TY, (float)TZ);

	// Fallback: resolve from player character if position is zero
	if (TargetWorldPos.IsNearlyZero())
	{
		if (!ResolveTargetPosition(false, TargetId, TargetWorldPos)) return;
	}

	SpawnDamagePop(0, false, false, 0, TargetId, TargetWorldPos, TEXT("blocked"));
}

// ============================================================
// Resolve target world position from ID (shared by tick + applied)
// ============================================================

bool UDamageNumberSubsystem::ResolveTargetPosition(bool bIsEnemy, int32 TargetId, FVector& OutPos) const
{
	if (bIsEnemy)
	{
		if (UWorld* World = GetWorld())
		{
			if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
			{
				const FEnemyEntry* EnemyData = ES->GetEnemyData(TargetId);
				if (EnemyData && EnemyData->Actor.IsValid())
				{
					OutPos = EnemyData->Actor->GetActorLocation();
					return true;
				}
			}
		}
	}
	else if (TargetId == LocalCharacterId)
	{
		if (UWorld* World = GetWorld())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC && PC->GetPawn())
			{
				OutPos = PC->GetPawn()->GetActorLocation();
				return true;
			}
		}
	}
	return false;
}

// ============================================================
// Status display name + color mapping
// ============================================================

FString UDamageNumberSubsystem::GetStatusDisplayName(const FString& StatusType)
{
	if (StatusType == TEXT("poison"))    return TEXT("Poisoned!");
	if (StatusType == TEXT("stun"))      return TEXT("Stunned!");
	if (StatusType == TEXT("freeze"))    return TEXT("Frozen!");
	if (StatusType == TEXT("blind"))     return TEXT("Blinded!");
	if (StatusType == TEXT("silence"))   return TEXT("Silenced!");
	if (StatusType == TEXT("stone"))     return TEXT("Petrified!");
	if (StatusType == TEXT("petrifying")) return TEXT("Petrifying!");
	if (StatusType == TEXT("sleep"))     return TEXT("Sleep!");
	if (StatusType == TEXT("bleeding"))  return TEXT("Bleeding!");
	if (StatusType == TEXT("confusion")) return TEXT("Confused!");
	if (StatusType == TEXT("curse"))     return TEXT("Cursed!");
	// Fallback: capitalize first letter
	FString Display = StatusType;
	Display.ReplaceInline(TEXT("_"), TEXT(" "));
	if (Display.Len() > 0) Display[0] = FChar::ToUpper(Display[0]);
	return Display + TEXT("!");
}

FLinearColor UDamageNumberSubsystem::GetStatusColor(const FString& StatusType)
{
	if (StatusType == TEXT("poison"))    return FLinearColor(0.70f, 0.30f, 0.80f); // Purple
	if (StatusType == TEXT("stun"))      return FLinearColor(1.00f, 0.92f, 0.23f); // Yellow
	if (StatusType == TEXT("freeze"))    return FLinearColor(0.40f, 0.70f, 1.00f); // Blue
	if (StatusType == TEXT("blind"))     return FLinearColor(0.60f, 0.60f, 0.60f); // Grey
	if (StatusType == TEXT("silence"))   return FLinearColor(0.70f, 0.80f, 1.00f); // Light blue
	if (StatusType == TEXT("stone"))     return FLinearColor(0.70f, 0.55f, 0.30f); // Brown
	if (StatusType == TEXT("petrifying")) return FLinearColor(0.70f, 0.55f, 0.30f); // Brown
	if (StatusType == TEXT("sleep"))     return FLinearColor(0.75f, 0.60f, 0.90f); // Light purple
	if (StatusType == TEXT("bleeding"))  return FLinearColor(1.00f, 0.20f, 0.20f); // Red
	if (StatusType == TEXT("confusion")) return FLinearColor(1.00f, 0.45f, 0.65f); // Pink
	if (StatusType == TEXT("curse"))     return FLinearColor(0.50f, 0.20f, 0.50f); // Dark purple
	return FLinearColor(0.90f, 0.90f, 0.90f); // Default white-ish
}

// ============================================================
// Handle status:tick — periodic damage from poison/bleeding/stone
// ============================================================

void UDamageNumberSubsystem::HandleStatusTick(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !OverlayWidget.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0, DrainD = 0;
	bool bIsEnemy = false;
	FString StatusType;

	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("drain"), DrainD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetStringField(TEXT("statusType"), StatusType);

	const int32 TargetId = (int32)TargetIdD;
	const int32 Drain = (int32)DrainD;
	if (Drain <= 0) return;

	FVector TargetPos;
	if (!ResolveTargetPosition(bIsEnemy, TargetId, TargetPos)) return;

	// Use full status color directly (not subtle element tinting)
	FLinearColor StatusColor = GetStatusColor(StatusType);
	SpawnDamagePop(Drain, false, bIsEnemy, 0, TargetId, TargetPos,
		TEXT("normal"), TEXT("neutral"), &StatusColor);
}

// ============================================================
// Handle status:applied — show floating status name text
// ============================================================

void UDamageNumberSubsystem::HandleStatusApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid() || !OverlayWidget.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	bool bIsEnemy = false;
	FString StatusType;

	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetStringField(TEXT("statusType"), StatusType);

	const int32 TargetId = (int32)TargetIdD;
	if (StatusType.IsEmpty()) return;

	// Only show for: enemies hit by us, or the local player being afflicted
	if (bIsEnemy)
	{
		// For enemies, only show if WE applied it (sourceId == local player)
		double SourceIdD = 0;
		Obj->TryGetNumberField(TEXT("sourceId"), SourceIdD);
		if (LocalCharacterId > 0 && (int32)SourceIdD != LocalCharacterId) return;
	}
	else
	{
		// For players, only show if it's the local player
		if (LocalCharacterId > 0 && TargetId != LocalCharacterId) return;
	}

	FVector TargetPos;
	if (!ResolveTargetPosition(bIsEnemy, TargetId, TargetPos)) return;

	FString DisplayText = GetStatusDisplayName(StatusType);
	FLinearColor Color = GetStatusColor(StatusType);

	SpawnTextPop(DisplayText, Color, bIsEnemy, TargetId, TargetPos);
}

// ============================================================
// Spawn a floating text label at a target's position
// ============================================================

void UDamageNumberSubsystem::SpawnTextPop(
	const FString& Text, const FLinearColor& Color,
	bool bIsEnemy, int32 TargetId, const FVector& TargetWorldPos)
{
	if (!OverlayWidget.IsValid()) return;

	// Offset above head (same as damage numbers)
	FVector HeadPos = TargetWorldPos;
	HeadPos.Z += HEAD_OFFSET_Z;

	// For local player, use actual pawn position
	if (!bIsEnemy && TargetId == LocalCharacterId)
	{
		if (UWorld* World = GetWorld())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC && PC->GetPawn())
			{
				HeadPos = PC->GetPawn()->GetActorLocation();
				HeadPos.Z += HEAD_OFFSET_Z;
			}
		}
	}

	FVector2D ScreenPos;
	if (!ProjectWorldToScreen(HeadPos, ScreenPos)) return;

	// Offset slightly higher than damage numbers so text appears above the drain number
	ScreenPos.Y -= 20.0f;

	OverlayWidget->AddTextPop(Text, Color, ScreenPos);
}

// ============================================================
// Spawn combo total — big yellow number after multi-hit skill
// ============================================================

void UDamageNumberSubsystem::SpawnComboTotal(
	int32 TotalDamage, bool bIsEnemy, int32 TargetId, const FVector& TargetWorldPos)
{
	SpawnDamagePop(TotalDamage, false, bIsEnemy, 0, TargetId, TargetWorldPos,
		TEXT("combo_total"), TEXT("neutral"));
}

// ============================================================
// Spawn a damage pop-up from parsed event data
// ============================================================

void UDamageNumberSubsystem::SpawnDamagePop(
	int32 Damage,
	bool bIsCritical,
	bool bIsEnemy,
	int32 AttackerId,
	int32 TargetId,
	const FVector& TargetWorldPos,
	const FString& HitType,
	const FString& Element,
	const FLinearColor* CustomColor)
{
	if (!bDamageNumbersEnabled) return;

	if (!OverlayWidget.IsValid())
	{
		UE_LOG(LogDamageNumbers, Warning, TEXT("SpawnDamagePop: OverlayWidget is invalid!"));
		return;
	}

	// Offset position above the target's head
	FVector HeadPos = TargetWorldPos;
	HeadPos.Z += HEAD_OFFSET_Z;

	// For local player as target, use the actual pawn position for better accuracy
	const bool bLocalPlayerIsTarget = !bIsEnemy && (TargetId == LocalCharacterId);
	if (bLocalPlayerIsTarget)
	{
		if (UWorld* World = GetWorld())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC && PC->GetPawn())
			{
				HeadPos = PC->GetPawn()->GetActorLocation();
				HeadPos.Z += HEAD_OFFSET_Z;
			}
		}
	}

	// Project to screen
	FVector2D ScreenPos;
	if (!ProjectWorldToScreen(HeadPos, ScreenPos))
	{
		UE_LOG(LogDamageNumbers, Warning, TEXT("SpawnDamagePop: ProjectWorldToScreen failed for pos=(%.0f,%.0f,%.0f)"),
			HeadPos.X, HeadPos.Y, HeadPos.Z);
		return;
	}

	// Determine damage pop type from server hitType
	EDamagePopType PopType;

	if (HitType == TEXT("combo_total"))
	{
		PopType = EDamagePopType::ComboTotal;
	}
	else if (HitType == TEXT("heal"))
	{
		PopType = EDamagePopType::Heal;
	}
	else if (HitType == TEXT("miss"))
	{
		PopType = EDamagePopType::Miss;
	}
	else if (HitType == TEXT("dodge"))
	{
		PopType = EDamagePopType::Dodge;
	}
	else if (HitType == TEXT("perfectDodge"))
	{
		PopType = EDamagePopType::PerfectDodge;
	}
	else if (HitType == TEXT("blocked"))
	{
		PopType = EDamagePopType::Block;
	}
	else if (Damage <= 0)
	{
		// Fallback: zero damage = miss (backward compat for events without hitType)
		PopType = EDamagePopType::Miss;
	}
	else if (bLocalPlayerIsTarget)
	{
		PopType = bIsCritical ? EDamagePopType::PlayerCritHit : EDamagePopType::PlayerHit;
	}
	else if (bIsCritical)
	{
		PopType = EDamagePopType::CriticalDamage;
	}
	else
	{
		PopType = EDamagePopType::NormalDamage;
	}

	// Hide miss/dodge text if option disabled
	if (!bShowMissText && (PopType == EDamagePopType::Miss || PopType == EDamagePopType::Dodge || PopType == EDamagePopType::PerfectDodge))
		return;

	UE_LOG(LogDamageNumbers, Log, TEXT("SpawnDamagePop: dmg=%d type=%d hitType=%s ele=%s screen=(%.0f,%.0f)"),
		Damage, (int32)PopType, *HitType, *Element, ScreenPos.X, ScreenPos.Y);

	OverlayWidget->AddDamagePop(Damage, PopType, ScreenPos, Element, CustomColor);
}
