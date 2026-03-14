// MultiplayerEventSubsystem.cpp — Bridges 14 persistent socket events to BP_SocketManager functions.
// Player/enemy/combat events migrated to C++ subsystems in Phases 2-3.

#include "MultiplayerEventSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMultiplayerBridge, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UMultiplayerEventSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UMultiplayerEventSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	// Find BP_SocketManager in the level (it's still placed in levels as a handler shell)
	SocketManagerActor = FindSocketManagerActor();
	if (!SocketManagerActor.IsValid())
	{
		UE_LOG(LogMultiplayerBridge, Warning, TEXT("BP_SocketManager not found in level — BP event bridge disabled."));
		return;
	}

	// Register handlers for ALL BP-exclusive events.
	// Each handler forwards the event data to the corresponding BP function via ProcessEvent.

	// ---- Player Events: REMOVED (Phase 3) ----
	// player:moved + player:left now handled by UOtherPlayerSubsystem.

	// ---- Combat Events: REMOVED (Phase 2) ----
	// All 9 combat events now handled by UCombatActionSubsystem.

	// ---- Enemy Events: REMOVED (Phase 3) ----
	// enemy:spawn, enemy:move, enemy:death, enemy:health_update, enemy:attack
	// now handled by UEnemySubsystem.

	// ---- Inventory Events (BP handles UI updates, equip visuals) ----
	Router->RegisterHandler(TEXT("inventory:data"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnInventoryData"), D); });
	Router->RegisterHandler(TEXT("inventory:used"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnItemUsed"), D); });
	Router->RegisterHandler(TEXT("inventory:equipped"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnItemEquipped"), D); });
	Router->RegisterHandler(TEXT("inventory:dropped"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnItemDropped"), D); });
	Router->RegisterHandler(TEXT("inventory:error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnInventoryError"), D); });

	// ---- Loot Events ----
	Router->RegisterHandler(TEXT("loot:drop"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnLootDrop"), D); });

	// ---- Chat Events ----
	Router->RegisterHandler(TEXT("chat:receive"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnChatReceived"), D); });

	// ---- Stats Events (BP updates HUD manager) ----
	Router->RegisterHandler(TEXT("player:stats"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnPlayerStats"), D); });

	// ---- Hotbar Events ----
	Router->RegisterHandler(TEXT("hotbar:data"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnHotbarData"), D); });
	Router->RegisterHandler(TEXT("hotbar:alldata"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnHotbarAllData"), D); });

	// ---- Shop Events ----
	Router->RegisterHandler(TEXT("shop:data"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnShopData"), D); });
	Router->RegisterHandler(TEXT("shop:bought"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnShopBought"), D); });
	Router->RegisterHandler(TEXT("shop:sold"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnShopSold"), D); });
	Router->RegisterHandler(TEXT("shop:error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnShopError"), D); });

	// Defer event forwarding by one frame — prevents ProcessEvent during PostLoad
	// (UE5 assertion: "Cannot call UnrealScript while PostLoading objects")
	// Events arriving during this gap are safely dropped; zone:ready resends all data.
	bReadyToForward = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToForward = true;
	});

	UE_LOG(LogMultiplayerBridge, Log, TEXT("MultiplayerEventSubsystem — 14 events registered (bridge to %s). Player/enemy/combat events migrated to C++ subsystems."),
		*SocketManagerActor->GetName());
}

void UMultiplayerEventSubsystem::Deinitialize()
{
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

	bReadyToForward = false;
	SocketManagerActor = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Find BP_SocketManager actor
// ============================================================

AActor* UMultiplayerEventSubsystem::FindSocketManagerActor() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetName().Contains(TEXT("SocketManager")))
		{
			return *It;
		}
	}
	return nullptr;
}

// ============================================================
// Bridge: forward event to BP handler function
// ============================================================

void UMultiplayerEventSubsystem::ForwardToBPHandler(const FString& FunctionName, const TSharedPtr<FJsonValue>& Data)
{
	// Skip during PostLoad to prevent "Cannot call UnrealScript while PostLoading" crash
	if (!bReadyToForward) return;

	if (!SocketManagerActor.IsValid())
	{
		// Attempt to re-find (may have been spawned late)
		SocketManagerActor = FindSocketManagerActor();
		if (!SocketManagerActor.IsValid()) return;
	}

	UFunction* Func = SocketManagerActor->FindFunction(*FunctionName);
	if (!Func)
	{
		UE_LOG(LogMultiplayerBridge, Verbose, TEXT("BP function '%s' not found on %s — skipping."),
			*FunctionName, *SocketManagerActor->GetName());
		return;
	}

	// BP handler functions take a single FString "Data" parameter
	// (this matches how SocketIOClientComponent's Bind Event to Function works)
	FString JsonStr = JsonValueToString(Data);

	struct FBPParams
	{
		FString Data;
	};
	FBPParams Params;
	Params.Data = JsonStr;

	SocketManagerActor->ProcessEvent(Func, &Params);
}

// ============================================================
// Utility: serialize FJsonValue to JSON string
// ============================================================

FString UMultiplayerEventSubsystem::JsonValueToString(const TSharedPtr<FJsonValue>& Value)
{
	if (!Value.IsValid()) return TEXT("{}");

	// If it's an object, serialize the object
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (Value->TryGetObject(ObjPtr) && ObjPtr && ObjPtr->IsValid())
	{
		FString Output;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
		FJsonSerializer::Serialize(ObjPtr->ToSharedRef(), Writer);
		return Output;
	}

	// If it's a string already, return it
	FString StrVal;
	if (Value->TryGetString(StrVal))
	{
		return StrVal;
	}

	// Fallback: try to serialize as-is
	return TEXT("{}");
}

// ============================================================
// Public API: Combat event emission (called by BP_MMOCharacter)
// ============================================================

void UMultiplayerEventSubsystem::EmitCombatAttack(int32 TargetId, bool bIsEnemy)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();

	if (bIsEnemy)
	{
		Payload->SetNumberField(TEXT("targetEnemyId"), TargetId);
	}
	else
	{
		Payload->SetStringField(TEXT("targetCharacterId"), FString::FromInt(TargetId));
	}

	GI->EmitSocketEvent(TEXT("combat:attack"), Payload);
}

void UMultiplayerEventSubsystem::EmitStopAttack()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("combat:stop_attack"), TEXT("{}"));
}

void UMultiplayerEventSubsystem::RequestRespawn()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("combat:respawn"), TEXT("{}"));
	UE_LOG(LogMultiplayerBridge, Log, TEXT("RequestRespawn emitted."));
}

void UMultiplayerEventSubsystem::EmitChatMessage(const FString& Message, const FString& Channel)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("message"), Message);
	Payload->SetStringField(TEXT("channel"), Channel);

	GI->EmitSocketEvent(TEXT("chat:message"), Payload);
}
