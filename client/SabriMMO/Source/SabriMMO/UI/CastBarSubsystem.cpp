// CastBarSubsystem.cpp — Implementation of the RO-style cast bar subsystem.
// Registers skill:cast_start/complete/interrupted via EventRouter, maintains
// ActiveCasts map read by SCastBarOverlay for rendering.

#include "CastBarSubsystem.h"
#include "SCastBarOverlay.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogCastBar, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UCastBarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UCastBarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
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
		Router->RegisterHandler(TEXT("skill:cast_start"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCastStart(D); });
		Router->RegisterHandler(TEXT("skill:cast_complete"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCastComplete(D); });
		Router->RegisterHandler(TEXT("skill:cast_interrupted"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterrupted(D); });
		Router->RegisterHandler(TEXT("skill:cast_interrupted_broadcast"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterruptedBroadcast(D); });
		Router->RegisterHandler(TEXT("skill:cast_failed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCastFailed(D); });
	}

	// Only show overlay when socket is connected (game level)
	if (GI->IsSocketConnected())
	{
		ShowOverlay();
	}

	UE_LOG(LogCastBar, Log, TEXT("CastBarSubsystem started — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);
}

void UCastBarSubsystem::Deinitialize()
{
	HideOverlay();
	ActiveCasts.Empty();

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

	UE_LOG(LogCastBar, Log, TEXT("CastBarSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Overlay widget management
// ============================================================

void UCastBarSubsystem::ShowOverlay()
{
	if (bOverlayAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	OverlayWidget = SNew(SCastBarOverlay).Subsystem(this);

	ViewportOverlay =
		SNew(SWeakWidget)
		.PossiblyNullContent(OverlayWidget);

	// Z-order 25 = above DamageNumbers (20), below SkillTargeting overlay (100)
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 25);
	bOverlayAdded = true;

	UE_LOG(LogCastBar, Log, TEXT("Cast bar overlay added to viewport (Z=25)."));
}

void UCastBarSubsystem::HideOverlay()
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
// Event handlers
// ============================================================

void UCastBarSubsystem::HandleCastStart(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CasterIdD = 0;
	Obj->TryGetNumberField(TEXT("casterId"), CasterIdD);
	int32 CasterId = (int32)CasterIdD;

	FString CasterName, SkillName;
	Obj->TryGetStringField(TEXT("casterName"), CasterName);
	Obj->TryGetStringField(TEXT("skillName"), SkillName);

	double SkillIdD = 0;
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);

	double ActualCastTimeD = 0;
	Obj->TryGetNumberField(TEXT("actualCastTime"), ActualCastTimeD);

	FCastBarEntry Entry;
	Entry.CasterId = CasterId;
	Entry.CasterName = CasterName;
	Entry.SkillName = SkillName;
	Entry.SkillId = (int32)SkillIdD;
	Entry.CastStartTime = FPlatformTime::Seconds();
	Entry.CastDuration = (float)(ActualCastTimeD / 1000.0);  // ms -> seconds

	ActiveCasts.Add(CasterId, Entry);

	// Free Cast speed reduction: server sends freeCastSpeedPct (55-100) for Sage with Free Cast
	if (CasterId == LocalCharacterId)
	{
		double FreeCastPct = 0;
		Obj->TryGetNumberField(TEXT("freeCastSpeedPct"), FreeCastPct);
		if (FreeCastPct > 0 && FreeCastPct < 100)
		{
			ApplyFreeCastSpeed((float)FreeCastPct);
		}
	}

	// Play universal cast windup sound (RO Classic ef_beginspell.wav). Falls through
	// to PlaySkillCastSound which uses DefaultSkillCastSoundPath unless this skill ID
	// has a custom override in SkillCastSoundMap. Position from caster's pawn.
	if (UWorld* World = GetWorld())
	{
		if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
		{
			FVector CasterLoc = FVector::ZeroVector;
			// Local player: use controlled pawn directly
			if (CasterId == LocalCharacterId)
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						CasterLoc = Pawn->GetActorLocation();
					}
				}
			}
			// Remote casters: fall back to local pawn location (sound is 3D so it
			// will attenuate appropriately even if not perfectly positioned).
			// Future: look up via OtherPlayerSubsystem / EnemySubsystem.
			if (CasterLoc.IsNearlyZero())
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					if (APawn* Pawn = PC->GetPawn())
					{
						CasterLoc = Pawn->GetActorLocation();
					}
				}
			}
			Audio->PlaySkillCastSound(Entry.SkillId, CasterLoc);
		}
	}

	UE_LOG(LogCastBar, Log, TEXT("Cast started: %s casting %s (%.1fs)"),
		*CasterName, *SkillName, Entry.CastDuration);
}

void UCastBarSubsystem::HandleCastComplete(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CasterIdD = 0;
	Obj->TryGetNumberField(TEXT("casterId"), CasterIdD);
	int32 CasterId = (int32)CasterIdD;

	ActiveCasts.Remove(CasterId);

	if (CasterId == LocalCharacterId)
	{
		RestoreNormalSpeed();
	}

	UE_LOG(LogCastBar, Log, TEXT("Cast complete: caster %d"), CasterId);
}

void UCastBarSubsystem::HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data)
{
	// Caster-only event — always for the local player
	if (LocalCharacterId > 0)
	{
		ActiveCasts.Remove(LocalCharacterId);
		RestoreNormalSpeed();
	}

	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Reason;
	Obj->TryGetStringField(TEXT("reason"), Reason);

	UE_LOG(LogCastBar, Log, TEXT("Cast interrupted (local): reason=%s"), *Reason);
}

void UCastBarSubsystem::HandleCastInterruptedBroadcast(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CasterIdD = 0;
	Obj->TryGetNumberField(TEXT("casterId"), CasterIdD);
	int32 CasterId = (int32)CasterIdD;

	ActiveCasts.Remove(CasterId);

	UE_LOG(LogCastBar, Log, TEXT("Cast interrupted (broadcast): caster %d"), CasterId);
}

void UCastBarSubsystem::HandleCastFailed(const TSharedPtr<FJsonValue>& Data)
{
	// Cast completed timer but execution failed — remove the local player's cast bar
	if (LocalCharacterId > 0)
	{
		ActiveCasts.Remove(LocalCharacterId);
	}

	UE_LOG(LogCastBar, Log, TEXT("Cast failed (local)"));
}

// ============================================================
// Free Cast speed reduction
// ============================================================

void UCastBarSubsystem::ApplyFreeCastSpeed(float SpeedPct)
{
	if (bFreeCastSpeedApplied) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	ACharacter* Char = Cast<ACharacter>(PC->GetPawn());
	if (!Char) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (!CMC) return;

	SavedMaxWalkSpeed = CMC->MaxWalkSpeed;
	CMC->MaxWalkSpeed = SavedMaxWalkSpeed * (SpeedPct / 100.f);
	bFreeCastSpeedApplied = true;

	UE_LOG(LogCastBar, Log, TEXT("Free Cast speed applied: %.0f%% (%.0f -> %.0f)"),
		SpeedPct, SavedMaxWalkSpeed, CMC->MaxWalkSpeed);
}

void UCastBarSubsystem::RestoreNormalSpeed()
{
	if (!bFreeCastSpeedApplied) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	ACharacter* Char = Cast<ACharacter>(PC->GetPawn());
	if (!Char) return;

	UCharacterMovementComponent* CMC = Char->GetCharacterMovement();
	if (!CMC) return;

	CMC->MaxWalkSpeed = SavedMaxWalkSpeed;
	bFreeCastSpeedApplied = false;

	UE_LOG(LogCastBar, Log, TEXT("Free Cast speed restored: %.0f"), SavedMaxWalkSpeed);
}
