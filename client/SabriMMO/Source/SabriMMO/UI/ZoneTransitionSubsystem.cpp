// ZoneTransitionSubsystem.cpp — Zone transition state machine.
// Handles zone:change (server response to warp), zone:error, player:teleport (Fly Wing).
// Shows fullscreen loading overlay during level transitions.

#include "ZoneTransitionSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogZoneTransition, Log, All);

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

	// Seed zone name from GameInstance
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		CurrentZoneName = GI->CurrentZoneName;
	}

	// Start polling for socket events
	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &UZoneTransitionSubsystem::TryWrapSocketEvents),
		0.5f, true
	);

	// If we're in the middle of a zone transition (level just loaded after warp),
	// show loading and wait for socket to reconnect
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		if (GI->bIsZoneTransitioning)
		{
			ShowLoadingOverlay(FString::Printf(TEXT("Entering %s..."),
				*GI->PendingZoneName));

			// Reset state for this transition
			TransitionCheckCount = 0;
			bPawnTeleported = false;

			// Poll until socket is ready and pawn exists
			InWorld.GetTimerManager().SetTimer(
				TransitionCheckTimer,
				FTimerDelegate::CreateUObject(this, &UZoneTransitionSubsystem::CheckTransitionComplete),
				0.3f, true
			);

			UE_LOG(LogZoneTransition, Log,
				TEXT("Zone transition in progress — waiting for socket + pawn (dest: %s)"),
				*GI->PendingZoneName);
		}
	}

	UE_LOG(LogZoneTransition, Log, TEXT("ZoneTransitionSubsystem started (zone: %s)"), *CurrentZoneName);
}

void UZoneTransitionSubsystem::Deinitialize()
{
	HideLoadingOverlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
		World->GetTimerManager().ClearTimer(TransitionCheckTimer);
	}

	bEventsWrapped = false;
	CachedSIOComponent = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Find SocketIO component
// ============================================================

USocketIOClientComponent* UZoneTransitionSubsystem::FindSocketIOComponent() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
		{
			return Comp;
		}
	}
	return nullptr;
}

// ============================================================
// Event wrapping
// ============================================================

void UZoneTransitionSubsystem::TryWrapSocketEvents()
{
	if (bEventsWrapped) return;

	USocketIOClientComponent* SIOComp = FindSocketIOComponent();
	if (!SIOComp) return;

	TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
	if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

	// Wait for BP to bind events first
	if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

	CachedSIOComponent = SIOComp;

	// Get local character ID
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
	}

	// Wrap zone events
	WrapSingleEvent(TEXT("zone:change"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleZoneChange(D); });
	WrapSingleEvent(TEXT("zone:error"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleZoneError(D); });
	WrapSingleEvent(TEXT("player:teleport"),
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerTeleport(D); });

	bEventsWrapped = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	UE_LOG(LogZoneTransition, Log, TEXT("ZoneTransitionSubsystem — events wrapped."));
}

void UZoneTransitionSubsystem::WrapSingleEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
	FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
	if (Existing)
	{
		OriginalCallback = Existing->Function;
	}

	NativeClient->OnEvent(EventName,
		[OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (OriginalCallback) OriginalCallback(Event, Message);
			if (OurHandler) OurHandler(Message);
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);
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

	Pawn->SetActorLocation(FVector(X, Y, Z));
}

// ============================================================
// Public API
// ============================================================

void UZoneTransitionSubsystem::RequestWarp(const FString& WarpId)
{
	if (!CachedSIOComponent.IsValid())
	{
		UE_LOG(LogZoneTransition, Warning, TEXT("RequestWarp failed — no socket connection."));
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetStringField(TEXT("warpId"), WarpId);

	CachedSIOComponent->EmitNative(TEXT("zone:warp"), Payload);
	UE_LOG(LogZoneTransition, Log, TEXT("Requesting warp: %s"), *WarpId);
}

// ============================================================
// Transition management
// ============================================================

void UZoneTransitionSubsystem::CheckTransitionComplete()
{
	TransitionCheckCount++;

	// Wait until socket events are wrapped (connection is alive)
	if (!bEventsWrapped)
	{
		if (TransitionCheckCount % 10 == 1)
		{
			UE_LOG(LogZoneTransition, Warning, TEXT("CheckTransition #%d: waiting for socket events to wrap..."), TransitionCheckCount);
		}
		return;
	}

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
				     "FIX: Ensure this level has a PlayerStart actor and the GameMode has a valid DefaultPawnClass."),
				TransitionCheckCount);
			ForceCompleteTransition();
		}
		return;
	}

	// Teleport pawn to spawn immediately when found (before waiting for socket)
	// so it's already in the right position while the loading overlay is still visible.
	if (!bPawnTeleported)
	{
		TeleportPawnToSpawn();
		bPawnTeleported = true;
	}

	// Still waiting for socket events — stay on loading screen
	if (!bEventsWrapped)
	{
		if (TransitionCheckCount % 10 == 1)
		{
			UE_LOG(LogZoneTransition, Log,
				TEXT("CheckTransition #%d: pawn placed, waiting for socket events..."), TransitionCheckCount);
		}
		return;
	}

	// Everything ready — complete the transition

	// Update zone state
	GI->CurrentZoneName = GI->PendingZoneName;
	CurrentZoneName = GI->CurrentZoneName;
	CurrentDisplayName = GI->PendingZoneName;
	GI->bIsZoneTransitioning = false;

	// Emit zone:ready to server — triggers sending zone enemies + players to this client
	if (CachedSIOComponent.IsValid())
	{
		TSharedPtr<FJsonObject> ReadyPayload = MakeShareable(new FJsonObject());
		ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
		CachedSIOComponent->EmitNative(TEXT("zone:ready"), ReadyPayload);
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
	if (CachedSIOComponent.IsValid())
	{
		TSharedPtr<FJsonObject> ReadyPayload = MakeShareable(new FJsonObject());
		ReadyPayload->SetStringField(TEXT("zone"), CurrentZoneName);
		CachedSIOComponent->EmitNative(TEXT("zone:ready"), ReadyPayload);
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

	Pawn->SetActorLocation(GI->PendingSpawnLocation);
	UE_LOG(LogZoneTransition, Log,
		TEXT("Teleported pawn to spawn: (%.0f, %.0f, %.0f)"),
		GI->PendingSpawnLocation.X, GI->PendingSpawnLocation.Y, GI->PendingSpawnLocation.Z);
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
