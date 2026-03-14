// OtherPlayerSubsystem.cpp — Other player entity management: spawn, move, leave.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_OtherPlayerManager.

#include "OtherPlayerSubsystem.h"
#include "NameTagSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOtherPlayerSubsystem, Log, All);

// ============================================================
// BP Reflection Helpers
// ============================================================

namespace
{
	void SetBPInt(AActor* A, FName N, int32 V)
	{
		if (!A) return;
		if (FIntProperty* P = CastField<FIntProperty>(A->GetClass()->FindPropertyByName(N)))
			*P->ContainerPtrToValuePtr<int32>(A) = V;
	}

	void SetBPBool(AActor* A, FName N, bool V)
	{
		if (!A) return;
		if (FBoolProperty* P = CastField<FBoolProperty>(A->GetClass()->FindPropertyByName(N)))
			P->SetPropertyValue_InContainer(A, V);
	}

	void SetBPString(AActor* A, FName N, const FString& V)
	{
		if (!A) return;
		if (FStrProperty* P = CastField<FStrProperty>(A->GetClass()->FindPropertyByName(N)))
			*P->ContainerPtrToValuePtr<FString>(A) = V;
	}

	void SetBPVector(AActor* A, FName N, const FVector& V)
	{
		if (!A) return;
		if (FStructProperty* P = CastField<FStructProperty>(A->GetClass()->FindPropertyByName(N)))
		{
			if (P->Struct == TBaseStructure<FVector>::Get())
				*P->ContainerPtrToValuePtr<FVector>(A) = V;
		}
	}
}

// ============================================================
// Lifecycle
// ============================================================

bool UOtherPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UOtherPlayerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	// Get local character ID (filter self from player:moved)
	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Load BP class at runtime
	PlayerBPClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/SabriMMO/Blueprints/BP_OtherPlayerCharacter.BP_OtherPlayerCharacter_C"));
	if (!PlayerBPClass)
	{
		UE_LOG(LogOtherPlayerSubsystem, Error, TEXT("Failed to load BP_OtherPlayerCharacter — other players disabled."));
		return;
	}

	// Register event handlers
	Router->RegisterHandler(TEXT("player:moved"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerMoved(D); });
	Router->RegisterHandler(TEXT("player:left"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerLeft(D); });

	// Defer readiness by one frame (prevents ProcessEvent during PostLoad)
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("OtherPlayerSubsystem — 2 player events registered (localId=%d)."),
		LocalCharacterId);
}

void UOtherPlayerSubsystem::Deinitialize()
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

	// Destroy all spawned other-player actors
	for (auto& Pair : Players)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Destroy();
		}
	}
	Players.Empty();

	bReadyToProcess = false;
	PlayerBPClass = nullptr;

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

AActor* UOtherPlayerSubsystem::GetPlayer(int32 CharacterId) const
{
	const TWeakObjectPtr<AActor>* Found = Players.Find(CharacterId);
	if (Found && Found->IsValid())
		return Found->Get();
	return nullptr;
}

// ============================================================
// HandlePlayerMoved — filter local, spawn new or update existing
// ============================================================

void UOtherPlayerSubsystem::HandlePlayerMoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
	int32 CharId = (int32)CharIdD;

	// Filter local player — server broadcasts to everyone except sender,
	// but zone-join batch sends all players including self
	if (CharId == LocalCharacterId) return;

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);
	FVector Pos((float)X, (float)Y, (float)Z);

	// ---- Existing player? ----
	TWeakObjectPtr<AActor>* Existing = Players.Find(CharId);
	if (Existing && Existing->IsValid())
	{
		AActor* Player = Existing->Get();

		// Large distance = zone transition, Fly Wing, or teleport — snap instead of interpolate.
		// Normal 30Hz position updates are ~20 units; 200 catches all teleports.
		float Dist = FVector::Dist(Player->GetActorLocation(), Pos);
		if (Dist > 200.f)
		{
			Player->SetActorLocation(Pos);
		}

		SetBPVector(Player, TEXT("TargetPosition"), Pos);
		SetBPBool(Player, TEXT("bIsMoving"), true);
		return;
	}

	// ---- New player: spawn ----
	UWorld* World = GetWorld();
	if (!World || !PlayerBPClass) return;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Pos);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewPlayer = World->SpawnActor(PlayerBPClass, &SpawnTransform, SpawnParams);
	if (!NewPlayer)
	{
		UE_LOG(LogOtherPlayerSubsystem, Warning, TEXT("Failed to spawn other player %d"), CharId);
		return;
	}

	FString PlayerName;
	Obj->TryGetStringField(TEXT("characterName"), PlayerName);

	SetBPInt(NewPlayer, TEXT("CharacterId"), CharId);
	SetBPString(NewPlayer, TEXT("PlayerName"), PlayerName);
	SetBPVector(NewPlayer, TEXT("TargetPosition"), Pos);

	Players.Add(CharId, NewPlayer);

	// Register name tag (RO Classic: player names always visible)
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->RegisterEntity(NewPlayer, PlayerName, ENameTagEntityType::Player);

	UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("Spawned other player %d (%s) at (%.0f, %.0f, %.0f)"),
		CharId, *PlayerName, X, Y, Z);
}

// ============================================================
// HandlePlayerLeft — destroy and remove from registry
// ============================================================

void UOtherPlayerSubsystem::HandlePlayerLeft(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
	int32 CharId = (int32)CharIdD;

	TWeakObjectPtr<AActor>* Found = Players.Find(CharId);
	if (Found && Found->IsValid())
	{
		// Unregister name tag before destroying actor
		if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
			NTS->UnregisterEntity(Found->Get());

		Found->Get()->Destroy();
	}
	Players.Remove(CharId);

	UE_LOG(LogOtherPlayerSubsystem, Verbose, TEXT("Player %d left."), CharId);
}
