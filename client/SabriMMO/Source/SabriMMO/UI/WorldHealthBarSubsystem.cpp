// WorldHealthBarSubsystem.cpp — Implementation of the RO-style floating HP/SP
// bar subsystem. Wraps socket events, tracks player + enemy health data,
// and manages the SWorldHealthBarOverlay Slate widget.

#include "WorldHealthBarSubsystem.h"
#include "SWorldHealthBarOverlay.h"
#include "EnemySubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "ShopNPC.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorldHealthBar, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UWorldHealthBarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UWorldHealthBarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	PopulateFromGameInstance();

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("combat:health_update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleHealthUpdate(D); });
		Router->RegisterHandler(TEXT("combat:damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("skill:effect_damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
		Router->RegisterHandler(TEXT("combat:death"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDeath(D); });
		Router->RegisterHandler(TEXT("combat:respawn"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCombatRespawn(D); });
		Router->RegisterHandler(TEXT("player:stats"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerStats(D); });
		Router->RegisterHandler(TEXT("enemy:health_update"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyHealthUpdate(D); });
		Router->RegisterHandler(TEXT("enemy:spawn"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleEnemySpawn(D); });
		Router->RegisterHandler(TEXT("enemy:move"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyMove(D); });
		Router->RegisterHandler(TEXT("enemy:death"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyDeath(D); });
	}

	// Only show overlay and start caching when socket is connected (game level)
	if (GI->IsSocketConnected())
	{
		ShowOverlay();

		// Start periodic actor caching (match enemy data to actual world actors for smooth position tracking)
		InWorld.GetTimerManager().SetTimer(
			ActorCacheTimer,
			FTimerDelegate::CreateLambda([this]()
			{
				CacheEnemyActors();
				CacheNPCActors();
			}),
			2.0f,
			true,
			0.5f  // Initial delay: let actors spawn first
		);
	}

	UE_LOG(LogWorldHealthBar, Log, TEXT("WorldHealthBarSubsystem started — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);
}

void UWorldHealthBarSubsystem::Deinitialize()
{
	HideOverlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActorCacheTimer);

		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	EnemyHealthMap.Empty();

	UE_LOG(LogWorldHealthBar, Log, TEXT("WorldHealthBarSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Overlay management
// ============================================================

void UWorldHealthBarSubsystem::ShowOverlay()
{
	if (bOverlayAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	OverlayWidget = SNew(SWorldHealthBarOverlay).Subsystem(this);

	ViewportOverlay =
		SNew(SWeakWidget)
		.PossiblyNullContent(OverlayWidget);

	// Z-order 8 = below BasicInfo (10), below DamageNumbers (20)
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 8);
	bOverlayAdded = true;

	UE_LOG(LogWorldHealthBar, Log, TEXT("World health bar overlay added to viewport (Z=8)."));
}

void UWorldHealthBarSubsystem::HideOverlay()
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

bool UWorldHealthBarSubsystem::ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) return false;

	return PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);
}

// ============================================================
// Get the local player pawn's feet position
// ============================================================

bool UWorldHealthBarSubsystem::GetPlayerFeetPosition(FVector& OutPos) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC || !PC->GetPawn()) return false;

	OutPos = PC->GetPawn()->GetActorLocation();

	// Offset down to feet level using capsule half-height if available
	if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
	{
		if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
		{
			OutPos.Z -= Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	return true;
}

// ============================================================
// Populate initial data from GameInstance
// ============================================================

void UWorldHealthBarSubsystem::PopulateFromGameInstance()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Seed initial HP/SP (assume full until server corrects)
	if (SelChar.Health > 0)
	{
		PlayerCurrentHP = SelChar.Health;
		PlayerMaxHP = SelChar.Health;
	}
	if (SelChar.Mana > 0)
	{
		PlayerCurrentSP = SelChar.Mana;
		PlayerMaxSP = SelChar.Mana;
	}
}

// ============================================================
// Player Health Events
// ============================================================

void UWorldHealthBarSubsystem::HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Filter by local character ID
	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;
	if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

	double H = 0, MH = 0, M = 0, MM = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);
	Obj->TryGetNumberField(TEXT("mana"),      M);
	Obj->TryGetNumberField(TEXT("maxMana"),   MM);

	PlayerCurrentHP = (int32)H;
	PlayerMaxHP     = FMath::Max((int32)MH, 1);
	PlayerCurrentSP = (int32)M;
	PlayerMaxSP     = FMath::Max((int32)MM, 1);
	bPlayerDead     = (PlayerCurrentHP <= 0);
}

void UWorldHealthBarSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	int32 TargetId = (int32)TargetIdD;

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	double TH = 0, TMH = 0;
	Obj->TryGetNumberField(TEXT("targetHealth"),    TH);
	Obj->TryGetNumberField(TEXT("targetMaxHealth"), TMH);

	// Enemy target — update enemy health + position
	if (bIsEnemy)
	{
		FEnemyBarData& Enemy = EnemyHealthMap.FindOrAdd(TargetId);
		Enemy.EnemyId = TargetId;
		Enemy.CurrentHP = FMath::Max((int32)TH, 0);
		if ((int32)TMH > 0) Enemy.MaxHP = (int32)TMH;
		const bool bWasVisible = Enemy.bBarVisible;
		Enemy.bBarVisible = true;
		Enemy.bIsDead = (Enemy.CurrentHP <= 0);

		// Update position from damage event (MUST happen before eager cache)
		double TX = 0, TY = 0, TZ = 0;
		Obj->TryGetNumberField(TEXT("targetX"), TX);
		Obj->TryGetNumberField(TEXT("targetY"), TY);
		Obj->TryGetNumberField(TEXT("targetZ"), TZ);
		if (TX != 0.0 || TY != 0.0 || TZ != 0.0)
		{
			Enemy.WorldPosition = FVector((float)TX, (float)TY, (float)TZ);
		}

		// Eagerly cache actor on first visibility so we skip the fallback position path.
		// WorldPosition must be set first — CacheEnemyActors skips entries with zero position.
		if (!bWasVisible && !Enemy.CachedActor.IsValid())
		{
			CacheEnemyActors();
		}
	}
	// Player target — update player HP
	else if (LocalCharacterId > 0 && TargetId == LocalCharacterId)
	{
		PlayerCurrentHP = FMath::Max((int32)TH, 0);
		if ((int32)TMH > 0) PlayerMaxHP = (int32)TMH;
		bPlayerDead = (PlayerCurrentHP <= 0);
	}
}

void UWorldHealthBarSubsystem::HandleCombatDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double KilledIdD = 0;
	Obj->TryGetNumberField(TEXT("killedId"), KilledIdD);
	int32 KilledId = (int32)KilledIdD;

	if (LocalCharacterId > 0 && KilledId == LocalCharacterId)
	{
		PlayerCurrentHP = 0;
		bPlayerDead = true;
	}
}

void UWorldHealthBarSubsystem::HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;
	if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

	double H = 0, MH = 0, M = 0, MM = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);
	Obj->TryGetNumberField(TEXT("mana"),      M);
	Obj->TryGetNumberField(TEXT("maxMana"),   MM);

	PlayerCurrentHP = (int32)H;
	PlayerMaxHP     = FMath::Max((int32)MH, 1);
	PlayerCurrentSP = (int32)M;
	PlayerMaxSP     = FMath::Max((int32)MM, 1);
	bPlayerDead     = false;
}

void UWorldHealthBarSubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Filter by characterId if present
	double CharIdD = 0;
	if (Obj->TryGetNumberField(TEXT("characterId"), CharIdD))
	{
		int32 CharId = (int32)CharIdD;
		if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;
	}

	// Parse derived stats for MaxHP/MaxSP
	const TSharedPtr<FJsonObject>* DerivedPtr = nullptr;
	if (Obj->TryGetObjectField(TEXT("derived"), DerivedPtr) && DerivedPtr)
	{
		double MH = 0, MS = 0;
		(*DerivedPtr)->TryGetNumberField(TEXT("maxHP"), MH);
		(*DerivedPtr)->TryGetNumberField(TEXT("maxSP"), MS);
		if (MH > 0) PlayerMaxHP = (int32)MH;
		if (MS > 0) PlayerMaxSP = (int32)MS;
	}
}

// ============================================================
// Enemy Health Events
// ============================================================

void UWorldHealthBarSubsystem::HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD);
	int32 EnemyId = (int32)EnemyIdD;
	if (EnemyId <= 0) return;

	double H = 0, MH = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);

	bool bInCombat = false;
	Obj->TryGetBoolField(TEXT("inCombat"), bInCombat);

	FEnemyBarData& Enemy = EnemyHealthMap.FindOrAdd(EnemyId);
	Enemy.EnemyId = EnemyId;
	Enemy.CurrentHP = FMath::Max((int32)H, 0);
	if ((int32)MH > 0) Enemy.MaxHP = (int32)MH;
	Enemy.bBarVisible = bInCombat;
	Enemy.bIsDead = (Enemy.CurrentHP <= 0);
}

void UWorldHealthBarSubsystem::HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD);
	int32 EnemyId = (int32)EnemyIdD;
	if (EnemyId <= 0) return;

	double H = 0, MH = 0;
	Obj->TryGetNumberField(TEXT("health"),    H);
	Obj->TryGetNumberField(TEXT("maxHealth"), MH);

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	FEnemyBarData& Enemy = EnemyHealthMap.FindOrAdd(EnemyId);
	Enemy.EnemyId = EnemyId;
	Enemy.CurrentHP = (int32)H;
	Enemy.MaxHP = FMath::Max((int32)MH, 1);
	Enemy.WorldPosition = FVector((float)X, (float)Y, (float)Z);
	Enemy.bBarVisible = false;  // Hidden by default on spawn
	Enemy.bIsDead = false;
	Enemy.CachedActor = nullptr; // Clear cached actor — new spawn needs fresh lookup
}

void UWorldHealthBarSubsystem::HandleEnemyMove(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD);
	int32 EnemyId = (int32)EnemyIdD;
	if (EnemyId <= 0) return;

	FEnemyBarData* Enemy = EnemyHealthMap.Find(EnemyId);
	if (!Enemy) return;

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	Enemy->WorldPosition = FVector((float)X, (float)Y, (float)Z);
}

void UWorldHealthBarSubsystem::HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD);
	int32 EnemyId = (int32)EnemyIdD;
	if (EnemyId <= 0) return;

	FEnemyBarData* Enemy = EnemyHealthMap.Find(EnemyId);
	if (Enemy)
	{
		Enemy->CurrentHP = 0;
		Enemy->bIsDead = true;
		Enemy->bBarVisible = false;
		Enemy->CachedActor = nullptr; // Clear cached actor on death
	}
}

// ============================================================
// Actor Caching — match enemy data entries to actual world actors
// for smooth real-time position tracking (no socket lag)
// ============================================================

void UWorldHealthBarSubsystem::CacheEnemyActors()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Collect enemy actors from EnemySubsystem registry (replaces class name heuristic)
	TArray<AActor*> CandidateActors;
	if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
	{
		for (const auto& Pair : ES->GetAllEnemies())
		{
			const FEnemyEntry& Entry = Pair.Value;
			if (Entry.bIsDead) continue;
			AActor* Actor = Entry.Actor.Get();
			if (Actor) CandidateActors.Add(Actor);
		}
	}

	// For each enemy data entry that needs a cached actor, find the closest matching world actor
	for (auto& Pair : EnemyHealthMap)
	{
		FEnemyBarData& Enemy = Pair.Value;

		// Skip if already cached and still valid
		if (Enemy.CachedActor.IsValid()) continue;

		// Skip if we have no socket position to match against
		if (Enemy.WorldPosition.IsZero()) continue;

		// Find the closest candidate actor (2D distance, ignore Z differences from capsule offset)
		AActor* BestMatch = nullptr;
		float BestDistSq = FLT_MAX;
		const float MaxMatchDistSq = 500.f * 500.f; // 500 UE units max match distance

		for (AActor* Candidate : CandidateActors)
		{
			// Skip actors already claimed by another enemy entry
			bool bAlreadyClaimed = false;
			for (const auto& OtherPair : EnemyHealthMap)
			{
				if (OtherPair.Key != Pair.Key && OtherPair.Value.CachedActor.Get() == Candidate)
				{
					bAlreadyClaimed = true;
					break;
				}
			}
			if (bAlreadyClaimed) continue;

			FVector ActorPos = Candidate->GetActorLocation();
			// 2D distance (XY only) for matching — Z can differ due to capsule half-height
			float DistSq = FVector::DistSquaredXY(ActorPos, Enemy.WorldPosition);

			if (DistSq < BestDistSq && DistSq < MaxMatchDistSq)
			{
				BestDistSq = DistSq;
				BestMatch = Candidate;
			}
		}

		if (BestMatch)
		{
			Enemy.CachedActor = BestMatch;
		}
	}
}

// ============================================================
// Cache NPC actors — collect all AShopNPC actors for name rendering
// ============================================================

void UWorldHealthBarSubsystem::CacheNPCActors()
{
	UWorld* World = GetWorld();
	if (!World) return;

	NPCNames.Empty();

	for (TActorIterator<AShopNPC> It(World); It; ++It)
	{
		AShopNPC* NPC = *It;
		if (!NPC) continue;

		FNPCNameData Data;
		Data.DisplayName = NPC->NPCDisplayName;
		Data.Actor = NPC;
		NPCNames.Add(Data);
	}
}

// ============================================================
// Get enemy feet position — use cached actor for smooth tracking,
// fallback to socket position if actor not cached
// ============================================================

bool UWorldHealthBarSubsystem::GetEnemyFeetPosition(const FEnemyBarData& Enemy, FVector& OutPos) const
{
	// Prefer cached actor position (smooth, frame-accurate)
	if (Enemy.CachedActor.IsValid())
	{
		AActor* Actor = Enemy.CachedActor.Get();
		OutPos = Actor->GetActorLocation();

		// Offset down to feet level using capsule half-height (same as player)
		if (ACharacter* Char = Cast<ACharacter>(Actor))
		{
			if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
			{
				OutPos.Z -= Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		return true;
	}

	// Fallback: use socket-reported position (may be laggy)
	// Server positions are at capsule-center level (spawn Z).
	// Offset down by a default capsule half-height to approximate feet level,
	// matching what the CachedActor path does with the actual capsule component.
	if (!Enemy.WorldPosition.IsZero())
	{
		OutPos = Enemy.WorldPosition;
		static constexpr float DefaultCapsuleHalfHeight = 96.0f;
		OutPos.Z -= DefaultCapsuleHalfHeight;
		return true;
	}

	return false;
}
