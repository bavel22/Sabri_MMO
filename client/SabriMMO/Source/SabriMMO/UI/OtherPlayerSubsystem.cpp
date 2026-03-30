// OtherPlayerSubsystem.cpp — Other player entity management: spawn, move, leave.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_OtherPlayerManager.

#include "OtherPlayerSubsystem.h"
#include "NameTagSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Sprite/SpriteCharacterActor.h"
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
	Router->RegisterHandler(TEXT("player:appearance"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePlayerAppearance(D); });
	Router->RegisterHandler(TEXT("vending:shop_opened"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingShopOpened(D); });
	Router->RegisterHandler(TEXT("vending:shop_closed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingShopClosed(D); });
	Router->RegisterHandler(TEXT("skill:buff_applied"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleBuffApplied(D); });
	Router->RegisterHandler(TEXT("skill:buff_removed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleBuffRemoved(D); });
	// Some hiding removal paths (SP depletion, detection) emit buff:removed instead of skill:buff_removed
	Router->RegisterHandler(TEXT("buff:removed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleBuffRemoved(D); });

	// --- Remote sprite animation events ---
	// Helper: safely set anim state on a remote sprite (skip if not ready)
	auto SafeSetAnim = [](FPlayerEntry* E, ESpriteAnimState State) {
		if (!E || !E->SpriteActor.IsValid() || !IsValid(E->SpriteActor.Get()))
			return;
		if (!E->SpriteActor->IsBodyReady())
			return;
		E->SpriteActor->SetAnimState(State);
	};

	// Attack animation: when another player deals damage
	Router->RegisterHandler(TEXT("combat:damage"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double AtkId = 0;
			Obj->TryGetNumberField(TEXT("attackerId"), AtkId);
			SafeSetAnim(Players.Find((int32)AtkId), ESpriteAnimState::Attack);
		});

	// Death animation
	Router->RegisterHandler(TEXT("combat:death"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double KId = 0;
			Obj->TryGetNumberField(TEXT("killedId"), KId);
			SafeSetAnim(Players.Find((int32)KId), ESpriteAnimState::Death);
		});

	// Respawn: back to idle
	Router->RegisterHandler(TEXT("combat:respawn"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("characterId"), CId);
			SafeSetAnim(Players.Find((int32)CId), ESpriteAnimState::Idle);
		});

	// Hit reaction: when enemy attacks another player
	Router->RegisterHandler(TEXT("enemy:attack"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double TId = 0;
			Obj->TryGetNumberField(TEXT("targetId"), TId);
			FPlayerEntry* E = Players.Find((int32)TId);
			if (E && E->SpriteActor.IsValid() && E->SpriteActor->IsBodyReady())
			{
				auto State = E->SpriteActor->GetAnimState();
				if (State != ESpriteAnimState::Death && State != ESpriteAnimState::Attack)
					E->SpriteActor->SetAnimState(ESpriteAnimState::Hit);
			}
		});

	// Cast animation: when another player starts casting — pick cast anim by targetType
	Router->RegisterHandler(TEXT("skill:cast_start"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("casterId"), CId);
			FPlayerEntry* E = Players.Find((int32)CId);
			if (E && E->SpriteActor.IsValid() && E->SpriteActor->IsBodyReady())
			{
				if (E->SpriteActor->GetAnimState() != ESpriteAnimState::Death)
				{
					FString TargetType;
					Obj->TryGetStringField(TEXT("targetType"), TargetType);
					ESpriteAnimState CastState = ESpriteAnimState::CastSingle;
					if (TargetType == TEXT("self"))        CastState = ESpriteAnimState::CastSelf;
					else if (TargetType == TEXT("ground")) CastState = ESpriteAnimState::CastGround;
					else if (TargetType == TEXT("aoe"))    CastState = ESpriteAnimState::CastAoe;
					E->SpriteActor->SetAnimState(CastState);
				}
			}
		});

	// Sit/stand for other players
	Router->RegisterHandler(TEXT("player:sit_state"), this,
		[this, SafeSetAnim](const TSharedPtr<FJsonValue>& D) {
			if (!D.IsValid()) return;
			const TSharedPtr<FJsonObject>& Obj = D->AsObject();
			if (!Obj.IsValid()) return;
			double CId = 0;
			Obj->TryGetNumberField(TEXT("characterId"), CId);
			FPlayerEntry* E = Players.Find((int32)CId);
			if (E && E->SpriteActor.IsValid() && E->SpriteActor->IsBodyReady())
			{
				bool bSitting = false;
				Obj->TryGetBoolField(TEXT("isSitting"), bSitting);
				E->SpriteActor->SetAnimState(bSitting ? ESpriteAnimState::Sit : ESpriteAnimState::Idle);
			}
		});

	// Defer readiness by one frame (prevents ProcessEvent during PostLoad)
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("OtherPlayerSubsystem — 5 player events registered (localId=%d)."),
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
		if (Pair.Value.Actor.IsValid())
		{
			Pair.Value.Actor->Destroy();
		}
	}
	Players.Empty();
	ActorToPlayerId.Empty();
	HiddenPlayerIds.Empty();

	bReadyToProcess = false;
	PlayerBPClass = nullptr;

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

AActor* UOtherPlayerSubsystem::GetPlayer(int32 CharacterId) const
{
	const FPlayerEntry* Found = Players.Find(CharacterId);
	if (Found && Found->Actor.IsValid())
		return Found->Actor.Get();
	return nullptr;
}

const FPlayerEntry* UOtherPlayerSubsystem::GetPlayerData(int32 CharacterId) const
{
	return Players.Find(CharacterId);
}

int32 UOtherPlayerSubsystem::GetPlayerIdFromActor(AActor* Actor) const
{
	if (!Actor) return 0;
	const int32* Found = ActorToPlayerId.Find(Actor);
	return Found ? *Found : 0;
}

FString UOtherPlayerSubsystem::GetPlayerNameFromActor(AActor* Actor) const
{
	if (!Actor) return FString();
	const int32* FoundId = ActorToPlayerId.Find(Actor);
	if (!FoundId) return FString();
	const FPlayerEntry* Entry = Players.Find(*FoundId);
	return Entry ? Entry->PlayerName : FString();
}

bool UOtherPlayerSubsystem::IsPlayerHidden(int32 CharacterId) const
{
	return HiddenPlayerIds.Contains(CharacterId);
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
	FPlayerEntry* Existing = Players.Find(CharId);
	if (Existing && Existing->Actor.IsValid())
	{
		AActor* Player = Existing->Actor.Get();

		// Large distance = zone transition, Fly Wing, or teleport — snap instead of interpolate.
		float Dist = FVector::Dist(Player->GetActorLocation(), Pos);
		if (Dist > 200.f)
		{
			Player->SetActorLocation(Pos);
		}

		SetBPVector(Player, TEXT("TargetPosition"), Pos);
		SetBPBool(Player, TEXT("bIsMoving"), true);

		// Update weapon mode on every position tick (equipment changes propagate via player:moved)
		if (Existing->SpriteActor.IsValid())
		{
			double WMD = 0;
			Obj->TryGetNumberField(TEXT("weaponMode"), WMD);
			int32 WM = (int32)WMD;
			ESpriteWeaponMode NewMode = ESpriteWeaponMode::None;
			if (WM == 1) NewMode = ESpriteWeaponMode::OneHand;
			else if (WM == 2) NewMode = ESpriteWeaponMode::TwoHand;
			else if (WM == 3) NewMode = ESpriteWeaponMode::Bow;
			if (NewMode != Existing->SpriteActor->GetWeaponMode())
			{
				Existing->SpriteActor->SetWeaponMode(NewMode);
			}
		}

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
	FString JobClass;
	Obj->TryGetStringField(TEXT("jobClass"), JobClass);
	FString Gender;
	Obj->TryGetStringField(TEXT("gender"), Gender);
	double HairStyleD = 0, HairColorD = 0;
	Obj->TryGetNumberField(TEXT("hairStyle"), HairStyleD);
	Obj->TryGetNumberField(TEXT("hairColor"), HairColorD);

	SetBPVector(NewPlayer, TEXT("TargetPosition"), Pos);

	// Spawn sprite for this other player with their class/gender
	int32 SpriteClassId = ASpriteCharacterActor::JobClassToId(JobClass);
	int32 SpriteGender = Gender.ToLower() == TEXT("female") ? 1 : 0;
	ASpriteCharacterActor* Sprite = ASpriteCharacterActor::SpawnSpriteForClass(
		World, Pos, SpriteClassId, SpriteGender);
	// Hide the 3D mesh on the other player BP (show sprite instead)
	if (USkeletalMeshComponent* OtherMesh = NewPlayer->FindComponentByClass<USkeletalMeshComponent>())
	{
		OtherMesh->SetVisibility(false);
	}

	// Store entry in registry FIRST — must exist before equipment loading
	// so HandlePlayerAppearance can find it if it fires during this frame
	// Preserve any cached EquipVisuals from a placeholder entry (player:appearance arrived first)
	TMap<FString, int32> CachedEquipVisuals;
	FPlayerEntry* Placeholder = Players.Find(CharId);
	if (Placeholder && Placeholder->EquipVisuals.Num() > 0)
	{
		CachedEquipVisuals = Placeholder->EquipVisuals;
	}

	FPlayerEntry Entry;
	Entry.Actor = NewPlayer;
	Entry.SpriteActor = Sprite;
	Entry.CharacterId = CharId;
	Entry.PlayerName = PlayerName;
	Entry.JobClass = JobClass;
	Entry.Gender = Gender;
	Entry.HairStyle = (int32)HairStyleD;
	Entry.HairColor = (int32)HairColorD;
	Entry.EquipVisuals = CachedEquipVisuals;
	Players.Add(CharId, Entry);
	ActorToPlayerId.Add(NewPlayer, CharId);

	if (Sprite)
	{
		Sprite->AttachToOwnerActor(NewPlayer, false, CharId);

		// Set weapon mode immediately from player:moved data
		double WMD = 0;
		Obj->TryGetNumberField(TEXT("weaponMode"), WMD);
		int32 WM = (int32)WMD;
		UE_LOG(LogOtherPlayerSubsystem, Warning,
			TEXT("Remote sprite spawn: charId=%d weaponMode=%d bodyReady=%d"),
			CharId, WM, Sprite->IsBodyReady() ? 1 : 0);
		if (WM == 1) Sprite->SetWeaponMode(ESpriteWeaponMode::OneHand);
		else if (WM == 2) Sprite->SetWeaponMode(ESpriteWeaponMode::TwoHand);
		else if (WM == 3) Sprite->SetWeaponMode(ESpriteWeaponMode::Bow);

		// Load equipment visuals — try 3 sources in order:
		// 1. equipVisuals in player:moved data (server must be restarted for this)
		// 2. Cached from player:appearance that arrived before this handler
		// 3. HandlePlayerAppearance will load them when it fires next
		bool bEquipLoaded = false;

		const TSharedPtr<FJsonObject>* EquipObj;
		if (Obj->TryGetObjectField(TEXT("equipVisuals"), EquipObj))
		{
			static const TArray<FString> Slots = {
				TEXT("weapon"), TEXT("shield"),
				TEXT("head_top"), TEXT("head_mid"), TEXT("head_low"),
				TEXT("garment")
			};
			for (const FString& SlotName : Slots)
			{
				double ViewSpriteD = 0;
				(*EquipObj)->TryGetNumberField(SlotName, ViewSpriteD);
				int32 ViewSpriteId = (int32)ViewSpriteD;
				if (ViewSpriteId > 0)
				{
					ESpriteLayer Layer = ASpriteCharacterActor::EquipSlotToSpriteLayer(SlotName);
					if (Layer != ESpriteLayer::MAX)
						Sprite->LoadEquipmentLayer(Layer, ViewSpriteId);
					bEquipLoaded = true;
				}
			}
		}

		if (!bEquipLoaded && CachedEquipVisuals.Num() > 0)
		{
			for (const auto& Pair : CachedEquipVisuals)
			{
				if (Pair.Value > 0)
				{
					ESpriteLayer Layer = ASpriteCharacterActor::EquipSlotToSpriteLayer(Pair.Key);
					if (Layer != ESpriteLayer::MAX)
						Sprite->LoadEquipmentLayer(Layer, Pair.Value);
					bEquipLoaded = true;
				}
			}
		}

		UE_LOG(LogOtherPlayerSubsystem, Log,
			TEXT("Remote sprite %d: equipLoaded=%d cachedVisuals=%d"),
			CharId, bEquipLoaded ? 1 : 0, CachedEquipVisuals.Num());

		// Load hair style
		if (Entry.HairStyle > 0)
		{
			Sprite->SetHairStyle(Entry.HairStyle, Entry.HairColor);
		}
	}

	// Register name tag on the SPRITE actor (not the BP actor) so it tracks
	// the sprite exactly with no parallax/rotation mismatch
	// SpriteHeight=150 enables zoom-proportional positioning
	AActor* NameTagTarget = Sprite ? (AActor*)Sprite : NewPlayer;
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->RegisterEntity(NameTagTarget, PlayerName, ENameTagEntityType::Player, 0, 120.f, 150.f);

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

	FPlayerEntry* Found = Players.Find(CharId);
	if (Found)
	{
		if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		{
			// Unregister name tag (might be on sprite or BP actor)
			if (Found->SpriteActor.IsValid())
				NTS->UnregisterEntity(Found->SpriteActor.Get());
			if (Found->Actor.IsValid())
				NTS->UnregisterEntity(Found->Actor.Get());
		}

		// Destroy sprite actor
		if (Found->SpriteActor.IsValid())
			Found->SpriteActor.Get()->Destroy();

		if (Found->Actor.IsValid())
		{
			ActorToPlayerId.Remove(Found->Actor);
			Found->Actor.Get()->Destroy();
		}
	}
	Players.Remove(CharId);
	HiddenPlayerIds.Remove(CharId);

	UE_LOG(LogOtherPlayerSubsystem, Verbose, TEXT("Player %d left."), CharId);
}

// ============================================================
// HandleVendingShopOpened — show shop sign above vending player
// ============================================================

void UOtherPlayerSubsystem::HandleVendingShopOpened(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
	int32 CharId = (int32)CharIdD;

	FString Title;
	Obj->TryGetStringField(TEXT("title"), Title);

	FPlayerEntry* Found = Players.Find(CharId);
	if (Found)
	{
		Found->bIsVending = true;
		// Show shop sign via NameTagSubsystem
		if (Found->Actor.IsValid())
		{
			if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
			{
				NTS->SetVendingTitle(Found->Actor.Get(), Title);
			}
		}
		UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("Player %d opened shop: %s"), CharId, *Title);
	}
}

void UOtherPlayerSubsystem::HandleVendingShopClosed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
	int32 CharId = (int32)CharIdD;

	FPlayerEntry* Found = Players.Find(CharId);
	if (Found)
	{
		Found->bIsVending = false;
		// Remove shop sign
		if (Found->Actor.IsValid())
		{
			if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
			{
				NTS->SetVendingTitle(Found->Actor.Get(), FString());
			}
		}
		UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("Player %d closed shop"), CharId);
	}
}

// ============================================================
// HandleBuffApplied — hide other players when they enter Hiding/Cloaking
// ============================================================

void UOtherPlayerSubsystem::HandleBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only care about player buffs (isEnemy == false)
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return;

	// Check if effects contain isHidden: true
	const TSharedPtr<FJsonObject>* EffectsPtr = nullptr;
	if (!Obj->TryGetObjectField(TEXT("effects"), EffectsPtr) || !EffectsPtr) return;
	bool bIsHidden = false;
	(*EffectsPtr)->TryGetBoolField(TEXT("isHidden"), bIsHidden);
	if (!bIsHidden) return;

	double TargetIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("targetId"), TargetIdD)) return;
	int32 TargetId = (int32)TargetIdD;

	// Skip self (local player handles own hiding via BuffBar)
	if (TargetId == LocalCharacterId) return;

	SetPlayerVisibility(TargetId, false);
}

// ============================================================
// HandleBuffRemoved — show other players when they leave Hiding/Cloaking
// ============================================================

void UOtherPlayerSubsystem::HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (bIsEnemy) return;

	FString BuffName;
	Obj->TryGetStringField(TEXT("buffName"), BuffName);
	if (BuffName != TEXT("hiding") && BuffName != TEXT("cloaking")) return;

	double TargetIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("targetId"), TargetIdD)) return;
	int32 TargetId = (int32)TargetIdD;

	if (TargetId == LocalCharacterId) return;

	SetPlayerVisibility(TargetId, true);
}

// ============================================================
// SetPlayerVisibility — toggle actor + name tag for a player
// ============================================================

void UOtherPlayerSubsystem::SetPlayerVisibility(int32 CharacterId, bool bVisible)
{
	if (bVisible)
	{
		HiddenPlayerIds.Remove(CharacterId);
	}
	else
	{
		HiddenPlayerIds.Add(CharacterId);
	}

	FPlayerEntry* Entry = Players.Find(CharacterId);
	if (!Entry || !Entry->Actor.IsValid()) return;

	AActor* PlayerActor = Entry->Actor.Get();
	PlayerActor->SetActorHiddenInGame(!bVisible);

	// Also toggle name tag visibility
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
	{
		NTS->SetVisible(PlayerActor, bVisible);
	}

	UE_LOG(LogOtherPlayerSubsystem, Log, TEXT("Player %d (%s) %s"),
		CharacterId, *Entry->PlayerName, bVisible ? TEXT("revealed") : TEXT("hidden"));
}

void UOtherPlayerSubsystem::HandlePlayerAppearance(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>& Obj = Data->AsObject();
	if (!Obj.IsValid()) return;

	double CharIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("characterId"), CharIdD)) return;
	int32 CharId = (int32)CharIdD;

	// Cache equipment visuals even if sprite doesn't exist yet
	// (player:appearance may arrive before player:moved spawns the sprite)
	FPlayerEntry* Entry = Players.Find(CharId);
	if (!Entry)
	{
		// No entry yet — create a placeholder to cache the data
		FPlayerEntry NewEntry;
		NewEntry.CharacterId = CharId;
		Players.Add(CharId, NewEntry);
		Entry = Players.Find(CharId);
	}

	// Parse hair data
	double HairStyleD = 0, HairColorD = 0;
	Obj->TryGetNumberField(TEXT("hairStyle"), HairStyleD);
	Obj->TryGetNumberField(TEXT("hairColor"), HairColorD);
	if (HairStyleD > 0)
	{
		Entry->HairStyle = (int32)HairStyleD;
		Entry->HairColor = (int32)HairColorD;
	}

	// Parse and cache equipVisuals
	const TSharedPtr<FJsonObject>* EquipObj;
	if (Obj->TryGetObjectField(TEXT("equipVisuals"), EquipObj))
	{
		static const TArray<FString> Slots = {
			TEXT("weapon"), TEXT("shield"),
			TEXT("head_top"), TEXT("head_mid"), TEXT("head_low"),
			TEXT("garment")
		};

		for (const FString& SlotName : Slots)
		{
			double ViewSpriteD = 0;
			(*EquipObj)->TryGetNumberField(SlotName, ViewSpriteD);
			Entry->EquipVisuals.Add(SlotName, (int32)ViewSpriteD);
		}
	}

	// If sprite exists, apply immediately
	if (Entry->SpriteActor.IsValid())
	{
		ASpriteCharacterActor* Sprite = Entry->SpriteActor.Get();

		// Update weapon mode
		double WeaponModeD = 0;
		Obj->TryGetNumberField(TEXT("weaponMode"), WeaponModeD);
		int32 WM = (int32)WeaponModeD;
		ESpriteWeaponMode NewMode = ESpriteWeaponMode::None;
		if (WM == 1) NewMode = ESpriteWeaponMode::OneHand;
		else if (WM == 2) NewMode = ESpriteWeaponMode::TwoHand;
		else if (WM == 3) NewMode = ESpriteWeaponMode::Bow;
		Sprite->SetWeaponMode(NewMode);

		// Reset + load equipment + reconcile hair (same pattern as local player)
		Sprite->ResetHairHiding();

		for (const auto& Pair : Entry->EquipVisuals)
		{
			ESpriteLayer Layer = ASpriteCharacterActor::EquipSlotToSpriteLayer(Pair.Key);
			if (Layer != ESpriteLayer::MAX)
			{
				Sprite->LoadEquipmentLayer(Layer, Pair.Value);
			}
		}

		// Apply hair style
		if (Entry->HairStyle > 0)
		{
			Sprite->SetHairStyle(Entry->HairStyle, Entry->HairColor);
		}

		Sprite->ReconcileHairVisibility();
	}
}
