// CastBarSubsystem.cpp — Implementation of the RO-style cast bar subsystem.
// Wraps skill:cast_start/complete/interrupted Socket.io events, maintains
// ActiveCasts map read by SCastBarOverlay for rendering.

#include "CastBarSubsystem.h"
#include "SCastBarOverlay.h"
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

	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
		UE_LOG(LogCastBar, Log, TEXT("LocalCharacterId resolved: %d"), LocalCharacterId);
	}

	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &UCastBarSubsystem::TryWrapSocketEvents),
		0.5f,
		true
	);

	UE_LOG(LogCastBar, Log, TEXT("CastBarSubsystem started — waiting for SocketIO bindings..."));
}

void UCastBarSubsystem::Deinitialize()
{
	HideOverlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	ActiveCasts.Empty();
	bEventsWrapped = false;
	CachedSIOComponent = nullptr;

	UE_LOG(LogCastBar, Log, TEXT("CastBarSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Find the SocketIO component on BP_SocketManager
// ============================================================

USocketIOClientComponent* UCastBarSubsystem::FindSocketIOComponent() const
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
// Timer callback — wait for BP bindings, then wrap
// ============================================================

void UCastBarSubsystem::TryWrapSocketEvents()
{
	if (bEventsWrapped) return;

	USocketIOClientComponent* SIOComp = FindSocketIOComponent();
	if (!SIOComp) return;

	TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
	if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

	// Wait until BP has bound key events (same gate as other subsystems)
	if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update")))
		return;

	CachedSIOComponent = SIOComp;

	// Re-resolve character ID
	if (LocalCharacterId == 0)
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
		{
			FCharacterData SelChar = GI->GetSelectedCharacter();
			LocalCharacterId = SelChar.CharacterId;
			UE_LOG(LogCastBar, Log, TEXT("Late-resolved LocalCharacterId: %d"), LocalCharacterId);
		}
	}

	// Wrap cast events
	WrapSingleEvent(TEXT("skill:cast_start"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleCastStart(D); });
	WrapSingleEvent(TEXT("skill:cast_complete"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleCastComplete(D); });
	WrapSingleEvent(TEXT("skill:cast_interrupted"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterrupted(D); });
	WrapSingleEvent(TEXT("skill:cast_interrupted_broadcast"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterruptedBroadcast(D); });
	WrapSingleEvent(TEXT("skill:cast_failed"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleCastFailed(D); });

	bEventsWrapped = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	ShowOverlay();

	UE_LOG(LogCastBar, Log, TEXT("CastBarSubsystem — events wrapped, overlay active. LocalCharId=%d"), LocalCharacterId);
}

// ============================================================
// Wrap a single event: preserve original callback chain
// ============================================================

void UCastBarSubsystem::WrapSingleEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	// Save existing callback chain
	TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
	FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
	if (Existing)
	{
		OriginalCallback = Existing->Function;
	}

	// Replace with combined callback
	NativeClient->OnEvent(EventName,
		[OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (OriginalCallback)
			{
				OriginalCallback(Event, Message);
			}
			if (OurHandler)
			{
				OurHandler(Message);
			}
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);

	UE_LOG(LogCastBar, Log, TEXT("Wrapped event: %s (had original: %s)"),
		*EventName, OriginalCallback ? TEXT("yes") : TEXT("no"));
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
	Entry.CastDuration = (float)(ActualCastTimeD / 1000.0);  // ms → seconds

	ActiveCasts.Add(CasterId, Entry);

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

	UE_LOG(LogCastBar, Log, TEXT("Cast complete: caster %d"), CasterId);
}

void UCastBarSubsystem::HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data)
{
	// Caster-only event — always for the local player
	if (LocalCharacterId > 0)
	{
		ActiveCasts.Remove(LocalCharacterId);
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
