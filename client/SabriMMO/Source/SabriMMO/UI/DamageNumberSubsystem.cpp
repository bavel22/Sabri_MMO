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
	}

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

	OverlayWidget = SNew(SDamageNumberOverlay);

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
	const FString& Element)
{
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

	if (HitType == TEXT("heal"))
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

	UE_LOG(LogDamageNumbers, Log, TEXT("SpawnDamagePop: dmg=%d type=%d hitType=%s ele=%s screen=(%.0f,%.0f)"),
		Damage, (int32)PopType, *HitType, *Element, ScreenPos.X, ScreenPos.Y);

	OverlayWidget->AddDamagePop(Damage, PopType, ScreenPos, Element);
}
