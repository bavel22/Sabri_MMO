// MultiplayerEventSubsystem.cpp — All 14 bridges removed (Phases A-E).
// Only outbound emit helpers remain (EmitCombatAttack, EmitStopAttack, RequestRespawn, EmitChatMessage).
// Player/enemy/combat events migrated to C++ subsystems in Phases 2-3.

#include "MultiplayerEventSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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

	// All 14 bridges removed (Phases A-E). C++ subsystems now handle all events:
	//   Phase A: inventory:data/equipped/dropped/error, player:stats, hotbar:alldata, shop:*
	//   Phase B: hotbar:data (HotbarSubsystem)
	//   Phase C: inventory:used (InventorySubsystem)
	//   Phase D: loot:drop (InventorySubsystem)
	//   Phase E: chat:receive (ChatSubsystem)
	//   Phases 2-3: player/enemy/combat events
	// This subsystem now only provides outbound emit helpers for Blueprints.

	UE_LOG(LogMultiplayerBridge, Log, TEXT("MultiplayerEventSubsystem — 0 bridges (all migrated). Outbound emits only."));
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

	Super::Deinitialize();
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
