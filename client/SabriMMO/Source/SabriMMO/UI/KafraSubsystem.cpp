// KafraSubsystem.cpp — Manages Kafra NPC service state, save point + teleport,
// and SKafraWidget overlay lifecycle.

#include "KafraSubsystem.h"
#include "SKafraWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("kafra:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleKafraData(D); });
		Router->RegisterHandler(TEXT("kafra:saved"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleKafraSaved(D); });
		Router->RegisterHandler(TEXT("kafra:teleported"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleKafraTeleported(D); });
		Router->RegisterHandler(TEXT("kafra:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleKafraError(D); });
		Router->RegisterHandler(TEXT("cart:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartData(D); });
		Router->RegisterHandler(TEXT("cart:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartError(D); });
		Router->RegisterHandler(TEXT("cart:equipped"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartEquipped(D); });
	}

	UE_LOG(LogKafra, Log, TEXT("KafraSubsystem started — events registered via EventRouter."));
}

void UKafraSubsystem::Deinitialize()
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

	// Parse cart state
	bool HasCartVal = false;
	if (Obj->TryGetBoolField(TEXT("hasCart"), HasCartVal)) bHasCart = HasCartVal;
	FString JC;
	if (Obj->TryGetStringField(TEXT("jobClass"), JC)) JobClass = JC;
	double PushLv = 0;
	if (Obj->TryGetNumberField(TEXT("pushcartLevel"), PushLv)) PushcartLevel = (int32)PushLv;

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
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("kafraId"), KafraId);

	GI->EmitSocketEvent(TEXT("kafra:open"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting Kafra: %s"), *KafraId);
}

void UKafraSubsystem::RequestSave()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("kafra:save"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting save point."));
}

void UKafraSubsystem::RequestTeleport(const FString& DestZone)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("kafraId"), CurrentKafraId);
	Payload->SetStringField(TEXT("destZone"), DestZone);

	GI->EmitSocketEvent(TEXT("kafra:teleport"), Payload);
	UE_LOG(LogKafra, Log, TEXT("Requesting Kafra teleport to %s"), *DestZone);
}

void UKafraSubsystem::RequestRentCart()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	GI->EmitSocketEvent(TEXT("cart:rent"), TEXT("{}"));
	UE_LOG(LogKafra, Log, TEXT("Requesting cart rental."));
}

void UKafraSubsystem::RequestRemoveCart()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	GI->EmitSocketEvent(TEXT("cart:remove"), TEXT("{}"));
	UE_LOG(LogKafra, Log, TEXT("Requesting cart removal."));
}

bool UKafraSubsystem::CanUseCart() const
{
	static const TSet<FString> CartClasses = {
		TEXT("merchant"), TEXT("blacksmith"), TEXT("alchemist"),
		TEXT("whitesmith"), TEXT("creator"), TEXT("biochemist"),
		TEXT("mastersmith"), TEXT("super_novice")
	};
	return CartClasses.Contains(JobClass.ToLower());
}

void UKafraSubsystem::HandleCartData(const TSharedPtr<FJsonValue>& Data)
{
	// Cart successfully rented — update state
	bHasCart = true;
	StatusMessage = TEXT("Cart equipped!");
	StatusExpireTime = FPlatformTime::Seconds() + 3.0;
	UE_LOG(LogKafra, Log, TEXT("Cart rented successfully."));
}

void UKafraSubsystem::HandleCartEquipped(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	bool HasCartVal = false;
	(*ObjPtr)->TryGetBoolField(TEXT("hasCart"), HasCartVal);
	bHasCart = HasCartVal;

	if (!HasCartVal)
	{
		StatusMessage = TEXT("Cart removed.");
		StatusExpireTime = FPlatformTime::Seconds() + 3.0;
	}
}

void UKafraSubsystem::HandleCartError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	if ((*ObjPtr)->TryGetStringField(TEXT("message"), Msg))
	{
		StatusMessage = Msg;
		StatusExpireTime = FPlatformTime::Seconds() + 4.0;
		UE_LOG(LogKafra, Warning, TEXT("Cart error: %s"), *Msg);
	}
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
