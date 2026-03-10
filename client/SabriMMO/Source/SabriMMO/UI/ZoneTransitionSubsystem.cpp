// ZoneTransitionSubsystem.cpp — Zone transition state machine.
// Handles zone:change (server response to warp), zone:error, player:teleport (Fly Wing).
// Shows fullscreen loading overlay during level transitions.

#include "ZoneTransitionSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogZoneTransition, Log, All);

// ============================================================
// Ground-snap helper
// ============================================================

FVector UZoneTransitionSubsystem::SnapLocationToGround(
	UWorld* World, const FVector& RawLocation, float CapsuleHalfHeight)
{
	if (!World) return RawLocation;

	// Step 1: Try NavMesh projection
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys)
	{
		FNavLocation NavResult;
		const FVector QueryExtent(200.f, 200.f, 500.f);

		if (NavSys->ProjectPointToNavigation(RawLocation, NavResult, QueryExtent))
		{
			FVector Snapped = NavResult.Location;
			Snapped.Z += CapsuleHalfHeight;

			UE_LOG(LogZoneTransition, Log,
				TEXT("SnapToGround: NavMesh hit — raw=(%.0f,%.0f,%.0f) snapped=(%.0f,%.0f,%.0f)"),
				RawLocation.X, RawLocation.Y, RawLocation.Z,
				Snapped.X, Snapped.Y, Snapped.Z);
			return Snapped;
		}
	}

	// Step 2: Fallback — downward line trace
	FHitResult Hit;
	const FVector TraceStart(RawLocation.X, RawLocation.Y, RawLocation.Z + 500.f);
	const FVector TraceEnd(RawLocation.X, RawLocation.Y, RawLocation.Z - 2000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnapToGround), false);

	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
	{
		FVector Snapped = FVector(RawLocation.X, RawLocation.Y, Hit.ImpactPoint.Z + CapsuleHalfHeight);

		UE_LOG(LogZoneTransition, Log,
			TEXT("SnapToGround: Trace hit — raw=(%.0f,%.0f,%.0f) snapped=(%.0f,%.0f,%.0f)"),
			RawLocation.X, RawLocation.Y, RawLocation.Z,
			Snapped.X, Snapped.Y, Snapped.Z);
		return Snapped;
	}

	// Step 3: Both failed — return raw
	UE_LOG(LogZoneTransition, Warning,
		TEXT("SnapToGround: FAILED both NavMesh and trace — using raw location (%.0f,%.0f,%.0f)"),
		RawLocation.X, RawLocation.Y, RawLocation.Z);
	return RawLocation;
}

// ============================================================
// Lifecycle
// ============================================================

bool UZoneTransitionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UZoneTransitionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Seed zone name from GameInstance
	CurrentZoneName = GI->CurrentZoneName;

	// Get local character ID
	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register socket event handlers via persistent EventRouter
	if (USocketEventRouter* Router = GI->GetEventRouter())
	{
		Router->RegisterHandler(TEXT("zone:change"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleZoneChange(D); });
		Router->RegisterHandler(TEXT("zone:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleZoneError(D); });
		Router->RegisterHandler(TEXT("player:teleport"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerTeleport(D); });
	}

	// If we're in the middle of a zone transition (level just loaded after warp),
	// show loading and wait for pawn to spawn
	if (GI->bIsZoneTransitioning)
	{
		ShowLoadingOverlay(FString::Printf(TEXT("Entering %s..."),
			*GI->PendingZoneName));

		// Reset state for this transition
		TransitionCheckCount = 0;
		bPawnTeleported = false;

		// Poll until pawn exists (socket is already connected — persistent)
		InWorld.GetTimerManager().SetTimer(
			TransitionCheckTimer,
			FTimerDelegate::CreateUObject(this, &UZoneTransitionSubsystem::CheckTransitionComplete),
			0.3f, true
		);

		UE_LOG(LogZoneTransition, Log,
			TEXT("Zone transition in progress — waiting for pawn (dest: %s)"),
			*GI->PendingZoneName);
	}

	UE_LOG(LogZoneTransition, Log, TEXT("ZoneTransitionSubsystem started (zone: %s)"), *CurrentZoneName);
}

void UZoneTransitionSubsystem::Deinitialize()
{
	HideLoadingOverlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionCheckTimer);

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
// Event handlers
// ============================================================

void UZoneTransitionSubsystem::HandleZoneChange(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Zone, DisplayName, LevelName;
	Obj->TryGetStringField(TEXT("zone"), Zone);
	Obj->TryGetStringField(TEXT("displayName"), DisplayName);
	Obj->TryGetStringField(TEXT("levelName"), LevelName);

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	UE_LOG(LogZoneTransition, Log,
		TEXT("zone:change received — dest: %s (%s) level: %s pos: (%.0f, %.0f, %.0f)"),
		*Zone, *DisplayName, *LevelName, X, Y, Z);

	// Store transition data in GameInstance (survives level transition)
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	GI->PendingZoneName = Zone;
	GI->PendingLevelName = LevelName;
	GI->PendingSpawnLocation = FVector(X, Y, Z);
	GI->bIsZoneTransitioning = true;

	// Parse zone flags
	const TSharedPtr<FJsonObject>* FlagsPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("flags"), FlagsPtr) && FlagsPtr)
	{
		const TSharedPtr<FJsonObject>& Flags = *FlagsPtr;
		bool bVal = false;
		if (Flags->TryGetBoolField(TEXT("noteleport"), bVal)) bNoTeleport = bVal;
		if (Flags->TryGetBoolField(TEXT("noreturn"), bVal)) bNoReturn = bVal;
		if (Flags->TryGetBoolField(TEXT("nosave"), bVal)) bNoSave = bVal;
		if (Flags->TryGetBoolField(TEXT("town"), bVal)) bIsTown = bVal;
	}

	// Show loading overlay
	ShowLoadingOverlay(FString::Printf(TEXT("Entering %s..."), *DisplayName));

	// Open the new level after a brief delay (let overlay render)
	World->GetTimerManager().SetTimerForNextTick([World, LevelName]()
	{
		if (World && !LevelName.IsEmpty())
		{
			UGameplayStatics::OpenLevel(World, *LevelName);
		}
	});
}

void UZoneTransitionSubsystem::HandleZoneError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Message;
	(*ObjPtr)->TryGetStringField(TEXT("message"), Message);

	UE_LOG(LogZoneTransition, Warning, TEXT("zone:error — %s"), *Message);
}

void UZoneTransitionSubsystem::HandlePlayerTeleport(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	// Only handle local player teleport
	if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	FString TeleportType;
	Obj->TryGetStringField(TEXT("teleportType"), TeleportType);

	UE_LOG(LogZoneTransition, Log,
		TEXT("player:teleport — type: %s pos: (%.0f, %.0f, %.0f)"),
		*TeleportType, X, Y, Z);

	// Snap local pawn to new position
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	// Snap to walkable ground
	FVector RawPos(X, Y, Z);
	float HalfHeight = 96.f;
	if (ACharacter* Char = Cast<ACharacter>(Pawn))
	{
		HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FVector Snapped = SnapLocationToGround(World, RawPos, HalfHeight);
	Pawn->SetActorLocation(Snapped);

	// Micro-adjust for collision overlap
	FVector Adjusted = Snapped;
	if (World->FindTeleportSpot(Pawn, Adjusted, Pawn->GetActorRotation()))
	{
		Pawn->SetActorLocation(Adjusted);
	}
}

// ============================================================
// Public API
// ============================================================

void UZoneTransitionSubsystem::RequestWarp(const FString& WarpId)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected())
	{
		UE_LOG(LogZoneTransition, Warning, TEXT("RequestWarp failed — no socket connection."));
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("warpId"), WarpId);

	GI->EmitSocketEvent(TEXT("zone:warp"), Payload);
	UE_LOG(LogZoneTransition, Log, TEXT("Requesting warp: %s"), *WarpId);
}

// ============================================================
// Transition management
// ============================================================

void UZoneTransitionSubsystem::CheckTransitionComplete()
{
	TransitionCheckCount++;

	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->bIsZoneTransitioning)
	{
		UE_LOG(LogZoneTransition, Error,
			TEXT("CheckTransition #%d: bIsZoneTransitioning is FALSE — something reset it! GI=%s"),
			TransitionCheckCount, GI ? TEXT("valid") : TEXT("null"));
		// Transition was cancelled externally — stop polling
		World->GetTimerManager().ClearTimer(TransitionCheckTimer);
		HideLoadingOverlay();
		return;
	}

	// Wait for player pawn to exist
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		// Log diagnostics every ~3 seconds (every 10th check at 0.3s interval)
		if (TransitionCheckCount % 10 == 1)
		{
			// Gather diagnostic info
			FString GameModeClass = TEXT("None");
			FString DefaultPawnClassName = TEXT("None");
			int32 PlayerStartCount = 0;

			if (AGameModeBase* GM = World->GetAuthGameMode())
			{
				GameModeClass = GM->GetClass()->GetName();
				if (GM->DefaultPawnClass)
				{
					DefaultPawnClassName = GM->DefaultPawnClass->GetName();
				}
			}

			TArray<AActor*> PlayerStarts;
			UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);
			PlayerStartCount = PlayerStarts.Num();

			UE_LOG(LogZoneTransition, Warning,
				TEXT("CheckTransition #%d: NO PAWN — PC=%s, Pawn=%s, GameMode=%s, DefaultPawn=%s, PlayerStarts=%d"),
				TransitionCheckCount,
				PC ? TEXT("valid") : TEXT("NULL"),
				(PC && PC->GetPawn()) ? TEXT("valid") : TEXT("NULL"),
				*GameModeClass,
				*DefaultPawnClassName,
				PlayerStartCount);
		}

		// Timeout: after ~10 seconds (33 checks), force-complete without pawn
		if (TransitionCheckCount >= 33)
		{
			UE_LOG(LogZoneTransition, Error,
				TEXT("CheckTransition TIMEOUT after %d checks (~10s). Force-completing transition. "
				     "FIX: Ensure this level was duplicated from a working level so its Level Blueprint spawns BP_MMOCharacter."),
				TransitionCheckCount);
			ForceCompleteTransition();
		}
		return;
	}

	// Teleport pawn to correct position (only once)
	if (!bPawnTeleported)
	{
		TeleportPawnToSpawn();
		bPawnTeleported = true;
	}

	// Everything ready — complete the transition

	// Update zone state
	GI->CurrentZoneName = GI->PendingZoneName;
	CurrentZoneName = GI->CurrentZoneName;
	CurrentDisplayName = GI->PendingZoneName;
	GI->bIsZoneTransitioning = false;

	// Emit zone:ready to server — triggers sending zone enemies + players to this client
	{
		TSharedPtr<FJsonObject> ReadyPayload = MakeShared<FJsonObject>();
		ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
		GI->EmitSocketEvent(TEXT("zone:ready"), ReadyPayload);
		UE_LOG(LogZoneTransition, Log, TEXT("Emitted zone:ready for zone: %s"), *CurrentZoneName);
	}

	// Clear timer
	World->GetTimerManager().ClearTimer(TransitionCheckTimer);

	// Hide loading after a brief delay (let the world render one frame)
	World->GetTimerManager().SetTimerForNextTick([this]()
	{
		HideLoadingOverlay();
	});

	UE_LOG(LogZoneTransition, Log,
		TEXT("Zone transition complete — now in %s (after %d checks)"), *CurrentZoneName, TransitionCheckCount);
}

void UZoneTransitionSubsystem::ForceCompleteTransition()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	// Update zone state even without a pawn
	GI->CurrentZoneName = GI->PendingZoneName;
	CurrentZoneName = GI->CurrentZoneName;
	CurrentDisplayName = GI->PendingZoneName;
	GI->bIsZoneTransitioning = false;

	// Emit zone:ready so the server sends enemies/players for this zone
	if (GI->IsSocketConnected())
	{
		TSharedPtr<FJsonObject> ReadyPayload = MakeShared<FJsonObject>();
		ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
		GI->EmitSocketEvent(TEXT("zone:ready"), ReadyPayload);
		UE_LOG(LogZoneTransition, Log, TEXT("Emitted zone:ready (forced) for zone: %s"), *CurrentZoneName);
	}

	// Clear timer and hide loading
	World->GetTimerManager().ClearTimer(TransitionCheckTimer);
	World->GetTimerManager().SetTimerForNextTick([this]()
	{
		HideLoadingOverlay();
	});

	UE_LOG(LogZoneTransition, Warning,
		TEXT("Zone transition force-completed — now in %s (no pawn teleport)"), *CurrentZoneName);
}

void UZoneTransitionSubsystem::TeleportPawnToSpawn()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	if (GI->PendingSpawnLocation.IsNearlyZero()) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	// Snap to walkable ground
	float HalfHeight = 96.f;
	if (ACharacter* Char = Cast<ACharacter>(Pawn))
	{
		HalfHeight = Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}
	FVector Snapped = SnapLocationToGround(World, GI->PendingSpawnLocation, HalfHeight);
	Pawn->SetActorLocation(Snapped);

	// Micro-adjust for collision overlap
	FVector Adjusted = Snapped;
	if (World->FindTeleportSpot(Pawn, Adjusted, Pawn->GetActorRotation()))
	{
		Pawn->SetActorLocation(Adjusted);
	}

	UE_LOG(LogZoneTransition, Log,
		TEXT("Teleported pawn to spawn: raw=(%.0f,%.0f,%.0f) final=(%.0f,%.0f,%.0f)"),
		GI->PendingSpawnLocation.X, GI->PendingSpawnLocation.Y, GI->PendingSpawnLocation.Z,
		Adjusted.X, Adjusted.Y, Adjusted.Z);
}

// ============================================================
// Loading overlay
// ============================================================

void UZoneTransitionSubsystem::ShowLoadingOverlay(const FString& StatusText)
{
	if (bLoadingShown) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	LoadingWidget =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.05f, 1.f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(StatusText))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.90f, 0.78f, 1.f)))
			.ShadowOffset(FVector2D(2, 2))
			.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f))
		];

	LoadingOverlay = SNew(SWeakWidget).PossiblyNullContent(LoadingWidget);
	ViewportClient->AddViewportWidgetContent(LoadingOverlay.ToSharedRef(), 50);
	bLoadingShown = true;

	UE_LOG(LogZoneTransition, Log, TEXT("Loading overlay shown: %s"), *StatusText);
}

void UZoneTransitionSubsystem::HideLoadingOverlay()
{
	if (!bLoadingShown) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* ViewportClient = World->GetGameViewport();
		if (ViewportClient && LoadingOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(LoadingOverlay.ToSharedRef());
		}
	}

	LoadingWidget.Reset();
	LoadingOverlay.Reset();
	bLoadingShown = false;

	UE_LOG(LogZoneTransition, Log, TEXT("Loading overlay hidden."));
}
