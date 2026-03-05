// KafraSubsystem.cpp — Manages Kafra NPC service state, save point + teleport,
// Socket.io event wrapping, and SKafraWidget overlay lifecycle.

#include "KafraSubsystem.h"
#include "SKafraWidget.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogKafra, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UKafraSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UKafraSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &UKafraSubsystem::TryWrapSocketEvents),
		0.5f, true
	);

	UE_LOG(LogKafra, Log, TEXT("KafraSubsystem started — waiting for SocketIO bindings..."));
}

void UKafraSubsystem::Deinitialize()
{
	HideWidget();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	bEventsWrapped = false;
	CachedSIOComponent = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Find SocketIO component
// ============================================================

USocketIOClientComponent* UKafraSubsystem::FindSocketIOComponent() const
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

void UKafraSubsystem::TryWrapSocketEvents()
{
	if (bEventsWrapped) return;

	USocketIOClientComponent* SIOComp = FindSocketIOComponent();
	if (!SIOComp) return;

	TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
	if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

	if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

	CachedSIOComponent = SIOComp;

	WrapSingleEvent(TEXT("kafra:data"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleKafraData(D); });
	WrapSingleEvent(TEXT("kafra:saved"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleKafraSaved(D); });
	WrapSingleEvent(TEXT("kafra:teleported"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleKafraTeleported(D); });
	WrapSingleEvent(TEXT("kafra:error"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleKafraError(D); });

	bEventsWrapped = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	UE_LOG(LogKafra, Log, TEXT("KafraSubsystem — events wrapped."));
}

void UKafraSubsystem::WrapSingleEvent(
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

void UKafraSubsystem::HandleKafraData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Str;
	if (Obj->TryGetStringField(TEXT("kafraId"), Str)) CurrentKafraId = Str;
	if (Obj->TryGetStringField(TEXT("kafraName"), Str)) KafraName = Str;
	if (Obj->TryGetStringField(TEXT("currentSaveMap"), Str)) CurrentSaveMap = Str;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("playerZuzucoin"), Val)) PlayerZuzucoin = (int32)Val;

	// Parse destinations
	Destinations.Empty();
	const TArray<TSharedPtr<FJsonValue>>* DestsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("destinations"), DestsArr) && DestsArr)
	{
		for (const TSharedPtr<FJsonValue>& DestVal : *DestsArr)
		{
			const TSharedPtr<FJsonObject>* DestObjPtr = nullptr;
			if (DestVal.IsValid() && DestVal->TryGetObject(DestObjPtr) && DestObjPtr)
			{
				FKafraDestination Dest;
				(*DestObjPtr)->TryGetStringField(TEXT("zone"), Dest.ZoneName);
				(*DestObjPtr)->TryGetStringField(TEXT("displayName"), Dest.DisplayName);
				double CostD = 0;
				if ((*DestObjPtr)->TryGetNumberField(TEXT("cost"), CostD))
				{
					Dest.Cost = (int32)CostD;
				}
				Destinations.Add(Dest);
			}
		}
	}

	StatusMessage.Empty();
	ShowWidget();

	UE_LOG(LogKafra, Log, TEXT("Kafra opened: %s (%d destinations) Zeny=%d SaveMap=%s"),
		*KafraName, Destinations.Num(), PlayerZuzucoin, *CurrentSaveMap);
}

void UKafraSubsystem::HandleKafraSaved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString SaveMap;
	if (Obj->TryGetStringField(TEXT("saveMap"), SaveMap))
	{
		CurrentSaveMap = SaveMap;
	}

	StatusMessage = TEXT("Save point set!");
	StatusExpireTime = FPlatformTime::Seconds() + 3.0;

	UE_LOG(LogKafra, Log, TEXT("Save point set at %s"), *CurrentSaveMap);
}

void UKafraSubsystem::HandleKafraTeleported(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("remainingZuzucoin"), Val)) PlayerZuzucoin = (int32)Val;

	FString DestZone;
	Obj->TryGetStringField(TEXT("destZone"), DestZone);

	UE_LOG(LogKafra, Log, TEXT("Kafra teleport confirmed — dest: %s, remaining zeny: %d"),
		*DestZone, PlayerZuzucoin);

	// Close the Kafra UI — zone:change event will handle the level transition
	CloseKafra();
}

void UKafraSubsystem::HandleKafraError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	if ((*ObjPtr)->TryGetStringField(TEXT("message"), Msg))
	{
		StatusMessage = Msg;
		StatusExpireTime = FPlatformTime::Seconds() + 4.0;
		UE_LOG(LogKafra, Warning, TEXT("Kafra error: %s"), *Msg);
	}
}

// ============================================================
// Public API
// ============================================================

void UKafraSubsystem::RequestOpenKafra(const FString& KafraId)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetStringField(TEXT("kafraId"), KafraId);

	CachedSIOComponent->EmitNative(TEXT("kafra:open"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting Kafra: %s"), *KafraId);
}

void UKafraSubsystem::RequestSave()
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	CachedSIOComponent->EmitNative(TEXT("kafra:save"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting save point."));
}

void UKafraSubsystem::RequestTeleport(const FString& DestZone)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetStringField(TEXT("kafraId"), CurrentKafraId);
	Payload->SetStringField(TEXT("destZone"), DestZone);

	CachedSIOComponent->EmitNative(TEXT("kafra:teleport"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting Kafra teleport to %s"), *DestZone);
}

void UKafraSubsystem::CloseKafra()
{
	Destinations.Empty();
	StatusMessage.Empty();
	CurrentKafraId.Empty();
	HideWidget();
}

// ============================================================
// Widget lifecycle
// ============================================================

void UKafraSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	KafraWidget = SNew(SKafraWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			KafraWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 19);
	bWidgetAdded = true;

	// Lock player movement while Kafra is open
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ACharacter* Char = PC->GetCharacter())
		{
			if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
			{
				MovComp->DisableMovement();
			}
		}
	}

	UE_LOG(LogKafra, Log, TEXT("Kafra widget shown (Z=19)."));
}

void UKafraSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* ViewportClient = World->GetGameViewport();
		if (ViewportClient && ViewportOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}

	KafraWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;

	// Unlock player movement
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ACharacter* Char = PC->GetCharacter())
			{
				if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
				{
					MovComp->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}

	UE_LOG(LogKafra, Log, TEXT("Kafra widget hidden."));
}

bool UKafraSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded;
}
