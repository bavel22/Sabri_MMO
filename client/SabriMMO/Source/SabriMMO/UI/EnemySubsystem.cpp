// EnemySubsystem.cpp — Enemy entity management: spawn, move, death, health, attack.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_EnemyManager.

#include "EnemySubsystem.h"
#include "SSenseResultPopup.h"
#include "NameTagSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "VFX/SkillVFXSubsystem.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogEnemySubsystem, Log, All);

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

	void SetBPDouble(AActor* A, FName N, double V)
	{
		if (!A) return;
		UClass* C = A->GetClass();
		if (FDoubleProperty* P = CastField<FDoubleProperty>(C->FindPropertyByName(N)))
			*P->ContainerPtrToValuePtr<double>(A) = V;
		else if (FFloatProperty* P2 = CastField<FFloatProperty>(C->FindPropertyByName(N)))
			*P2->ContainerPtrToValuePtr<float>(A) = (float)V;
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

	bool GetBPBool(AActor* A, FName N)
	{
		if (!A) return false;
		if (FBoolProperty* P = CastField<FBoolProperty>(A->GetClass()->FindPropertyByName(N)))
			return P->GetPropertyValue_InContainer(A);
		return false;
	}

	// Call a parameterless BP function
	void CallBPFunction(AActor* Actor, FName FuncName)
	{
		if (!Actor) return;
		UFunction* Func = Actor->FindFunction(FuncName);
		if (Func) Actor->ProcessEvent(Func, nullptr);
	}

	// Call a BP function with params. Uses Memzero + explicit FString lifecycle
	// (NOT InitializeStruct/DestroyStruct which can overflow the alloca'd buffer
	// due to GetStructureSize() > ParmsSize alignment differences — causes 0xc0000409).
	void CallBPFunction(AActor* Actor, FName FuncName,
		TFunctionRef<void(UFunction*, uint8*)> SetParams)
	{
		if (!Actor) return;
		UFunction* Func = Actor->FindFunction(FuncName);
		if (!Func) return;

		if (Func->ParmsSize == 0)
		{
			Actor->ProcessEvent(Func, nullptr);
			return;
		}

		uint8* Params = (uint8*)FMemory_Alloca(Func->ParmsSize);
		FMemory::Memzero(Params, Func->ParmsSize);

		// Construct FString properties in-place (POD types are fine with Memzero)
		TArray<FStrProperty*, TInlineAllocator<4>> StringProps;
		for (TFieldIterator<FStrProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			new (It->ContainerPtrToValuePtr<FString>(Params)) FString();
			StringProps.Add(*It);
		}

		SetParams(Func, Params);
		Actor->ProcessEvent(Func, Params);

		// Destroy FStrings
		for (FStrProperty* P : StringProps)
		{
			P->ContainerPtrToValuePtr<FString>(Params)->~FString();
		}
	}
}

// ============================================================
// Lifecycle
// ============================================================

bool UEnemySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	// Load BP class at runtime
	EnemyBPClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/SabriMMO/Blueprints/BP_EnemyCharacter.BP_EnemyCharacter_C"));
	if (!EnemyBPClass)
	{
		UE_LOG(LogEnemySubsystem, Error, TEXT("Failed to load BP_EnemyCharacter — enemies disabled."));
		return;
	}

	// Register event handlers
	Router->RegisterHandler(TEXT("enemy:spawn"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemySpawn(D); });
	Router->RegisterHandler(TEXT("enemy:move"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyMove(D); });
	Router->RegisterHandler(TEXT("enemy:death"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyDeath(D); });
	Router->RegisterHandler(TEXT("enemy:health_update"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyHealthUpdate(D); });
	Router->RegisterHandler(TEXT("enemy:attack"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyAttack(D); });
	Router->RegisterHandler(TEXT("combat:knockback"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatKnockback(D); });
	Router->RegisterHandler(TEXT("skill:sense_result"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleSenseResult(D); });

	// Defer readiness by one frame (prevents ProcessEvent during PostLoad)
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	UE_LOG(LogEnemySubsystem, Log, TEXT("EnemySubsystem — 7 enemy events registered (incl. sense)."));
}

void UEnemySubsystem::Deinitialize()
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

	HideSensePopup();

	// Clear stand sound timers for all entities. Timer manager belongs to the world and
	// would clean up on world teardown, but explicit clearing is safer.
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TM = World->GetTimerManager();
		for (auto& Pair : Enemies)
		{
			if (Pair.Value.StandSoundTimer.IsValid())
				TM.ClearTimer(Pair.Value.StandSoundTimer);
		}
	}

	// Destroy all spawned enemy + sprite actors
	for (auto& Pair : Enemies)
	{
		// For sprite-only enemies, Actor == SpriteActor — only destroy once
		if (Pair.Value.SpriteActor.IsValid() && Pair.Value.SpriteActor != Pair.Value.Actor)
			Pair.Value.SpriteActor->Destroy();
		if (Pair.Value.Actor.IsValid())
			Pair.Value.Actor->Destroy();
	}
	Enemies.Empty();
	ActorToEnemyId.Empty();

	bReadyToProcess = false;
	EnemyBPClass = nullptr;

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

AActor* UEnemySubsystem::GetEnemy(int32 EnemyId) const
{
	const FEnemyEntry* Found = Enemies.Find(EnemyId);
	if (Found && Found->Actor.IsValid())
		return Found->Actor.Get();
	return nullptr;
}

const FEnemyEntry* UEnemySubsystem::GetEnemyData(int32 EnemyId) const
{
	return Enemies.Find(EnemyId);
}

int32 UEnemySubsystem::GetEnemyIdFromActor(AActor* Actor) const
{
	if (!Actor) return 0;
	const int32* Found = ActorToEnemyId.Find(Actor);
	return Found ? *Found : 0;
}

// ============================================================
// HandleEnemySpawn — spawn new, respawn dead, or update alive
// ============================================================

void UEnemySubsystem::HandleEnemySpawn(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
	int32 EnemyId = (int32)EnemyIdD;

	FString Name;
	Obj->TryGetStringField(TEXT("name"), Name);

	double LevelD = 0, HealthD = 0, MaxHealthD = 0;
	Obj->TryGetNumberField(TEXT("level"), LevelD);
	Obj->TryGetNumberField(TEXT("health"), HealthD);
	Obj->TryGetNumberField(TEXT("maxHealth"), MaxHealthD);
	int32 Level = (int32)LevelD;

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);
	FVector Pos((float)X, (float)Y, (float)Z);

	// Lambda: call InitializeEnemy with 5 params via ProcessEvent
	auto InitEnemy = [&](AActor* Enemy)
	{
		CallBPFunction(Enemy, TEXT("InitializeEnemy"),
			[&](UFunction* Func, uint8* Params)
			{
				if (auto* P = CastField<FIntProperty>(Func->FindPropertyByName(TEXT("InEnemyId"))))
					*P->ContainerPtrToValuePtr<int32>(Params) = EnemyId;
				if (auto* P = CastField<FStrProperty>(Func->FindPropertyByName(TEXT("InName"))))
					*P->ContainerPtrToValuePtr<FString>(Params) = Name;
				if (auto* P = CastField<FIntProperty>(Func->FindPropertyByName(TEXT("InLevel"))))
					*P->ContainerPtrToValuePtr<int32>(Params) = Level;
				if (auto* P = CastField<FDoubleProperty>(Func->FindPropertyByName(TEXT("InHealth"))))
					*P->ContainerPtrToValuePtr<double>(Params) = HealthD;
				if (auto* P = CastField<FDoubleProperty>(Func->FindPropertyByName(TEXT("InMaxHealth"))))
					*P->ContainerPtrToValuePtr<double>(Params) = MaxHealthD;
			});
	};

	// ---- Existing enemy? ----
	FEnemyEntry* Existing = Enemies.Find(EnemyId);
	if (Existing && Existing->Actor.IsValid())
	{
		AActor* Enemy = Existing->Actor.Get();

		if (Existing->bIsDead || Enemy->IsHidden())
		{
			// Dead respawn: re-show, re-enable, re-initialize
			if (Existing->SpriteActor.IsValid())
			{
				// Sprite enemy: sprite IS the actor
				Existing->SpriteActor->ServerTargetPos = Pos;
				Existing->SpriteActor->SetActorLocation(Pos);
				Existing->SpriteActor->SetActorHiddenInGame(false);
				Existing->SpriteActor->EnableClickCollision();
				if (Existing->SpriteActor->IsBodyReady())
					Existing->SpriteActor->SetAnimState(ESpriteAnimState::Idle);
			}

			if (!Existing->SpriteClass.IsEmpty())
			{
				// Sprite-only enemy: no BP actor to re-init
			}
			else
			{
				// BP enemy: re-show and re-init
				Enemy->SetActorHiddenInGame(false);
				Enemy->SetActorEnableCollision(true);
				SetBPBool(Enemy, TEXT("bIsDead"), false);
				Enemy->SetActorLocation(Pos);
				InitEnemy(Enemy);
				SetBPVector(Enemy, TEXT("TargetPosition"), Pos);
			}

			// Update struct data
			Existing->EnemyName = Name;
			Existing->EnemyLevel = Level;
			Existing->Health = HealthD;
			Existing->MaxHealth = MaxHealthD;
			Existing->bIsDead = false;

			// Re-show name tag on respawn
			AActor* TagTarget = Existing->SpriteActor.IsValid()
				? (AActor*)Existing->SpriteActor.Get() : Enemy;
			if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
				NTS->SetVisible(TagTarget, true);

			UE_LOG(LogEnemySubsystem, Log, TEXT("Respawned enemy %d (%s)"), EnemyId, *Name);
		}
		else
		{
			// Alive: just update position
			SetBPVector(Enemy, TEXT("TargetPosition"), Pos);
		}
		return;
	}

	// ---- New enemy: spawn ----
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FString SpriteClass;
	Obj->TryGetStringField(TEXT("spriteClass"), SpriteClass);
	double WeaponModeD = 0;
	Obj->TryGetNumberField(TEXT("weaponMode"), WeaponModeD);
	int32 WeaponMode = (int32)WeaponModeD;

	UE_LOG(LogEnemySubsystem, Warning, TEXT("Enemy %d (%s) spriteClass='%s' weaponMode=%d"),
		EnemyId, *Name, *SpriteClass, WeaponMode);

	AActor* PrimaryActor = nullptr;
	ASpriteCharacterActor* Sprite = nullptr;

	if (!SpriteClass.IsEmpty())
	{
		// ---- Sprite enemy: C++ only, no BP actor ----
		Sprite = World->SpawnActor<ASpriteCharacterActor>(
			Pos, FRotator::ZeroRotator, SpawnParams);
		if (!Sprite)
		{
			UE_LOG(LogEnemySubsystem, Warning, TEXT("Failed to spawn sprite enemy %d (%s)"), EnemyId, *Name);
			return;
		}

		Sprite->SetBodyClass(SpriteClass);
		Sprite->EnableClickCollision();
		Sprite->ServerTargetPos = Pos;
		Sprite->bUseServerMovement = true;

		ESpriteWeaponMode Mode = ESpriteWeaponMode::None;
		if (WeaponMode == 1) Mode = ESpriteWeaponMode::OneHand;
		else if (WeaponMode == 2) Mode = ESpriteWeaponMode::TwoHand;
		else if (WeaponMode == 3) Mode = ESpriteWeaponMode::Bow;
		Sprite->SetWeaponMode(Mode);

		PrimaryActor = Sprite;

		UE_LOG(LogEnemySubsystem, Log, TEXT("Enemy %d sprite-only: class=%s weaponMode=%d"),
			EnemyId, *SpriteClass, WeaponMode);
	}
	else
	{
		// ---- Non-sprite enemy: BP actor (existing path) ----
		if (!EnemyBPClass) return;

		FTransform SpawnTransform;
		SpawnTransform.SetLocation(Pos);

		AActor* NewEnemy = World->SpawnActor(EnemyBPClass, &SpawnTransform, SpawnParams);
		if (!NewEnemy)
		{
			UE_LOG(LogEnemySubsystem, Warning, TEXT("Failed to spawn enemy %d (%s)"), EnemyId, *Name);
			return;
		}

		InitEnemy(NewEnemy);
		SetBPVector(NewEnemy, TEXT("TargetPosition"), Pos);
		PrimaryActor = NewEnemy;
	}

	// Store in struct registry
	FEnemyEntry Entry;
	Entry.Actor = PrimaryActor;
	Entry.SpriteActor = Sprite;
	Entry.EnemyId = EnemyId;
	Entry.EnemyName = Name;
	Entry.SpriteClass = SpriteClass;
	Entry.WeaponMode = WeaponMode;
	Entry.EnemyLevel = Level;
	Entry.Health = HealthD;
	Entry.MaxHealth = MaxHealthD;
	Entry.bIsDead = false;
	Enemies.Add(EnemyId, Entry);
	ActorToEnemyId.Add(PrimaryActor, EnemyId);

	// ---- Audio bindings (sprite enemies only) ----
	if (Sprite)
	{
		UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>();

		// Frame-sync the move sound to the Walk animation cycle. Each time the sprite's
		// Walk animation wraps from last frame to first, we play the monster move sound.
		// This is the authentic RO Classic ACT-style "boing" cadence — perfectly synced
		// to the visual hop, not approximated by a fixed cooldown.
		// Lambda captures TWeakObjectPtr<this> so it safely no-ops if subsystem is destroyed.
		TWeakObjectPtr<UEnemySubsystem> WeakThis(this);
		Sprite->OnAnimCycleComplete.AddLambda([WeakThis, EnemyId](ESpriteAnimState State)
		{
			if (State != ESpriteAnimState::Walk) return;
			if (!WeakThis.IsValid()) return;

			const FEnemyEntry* E = WeakThis->Enemies.Find(EnemyId);
			if (!E || E->bIsDead || !E->Actor.IsValid()) return;

			if (UAudioSubsystem* A = WeakThis->GetWorld()->GetSubsystem<UAudioSubsystem>())
				A->PlayMonsterSound(E->SpriteClass, EMonsterSoundType::Move, E->Actor->GetActorLocation());
		});

		// Stand sound timer (RO Classic monsters with idle ambient like Pharaoh, Baphomet).
		// No-op for Poring/Skeleton — HasStandSound returns false and the timer is never set.
		// Initial offset is randomized so all entities don't fire on the same tick.
		if (Audio && Audio->HasStandSound(SpriteClass))
		{
			const float Interval = Audio->GetStandInterval(SpriteClass);
			const float InitialDelay = FMath::FRandRange(0.f, Interval);

			GetWorld()->GetTimerManager().SetTimer(
				Enemies[EnemyId].StandSoundTimer,
				[WeakThis, EnemyId]()
				{
					if (!WeakThis.IsValid()) return;
					const FEnemyEntry* E = WeakThis->Enemies.Find(EnemyId);
					if (!E || E->bIsDead || !E->Actor.IsValid()) return;
					if (UAudioSubsystem* A = WeakThis->GetWorld()->GetSubsystem<UAudioSubsystem>())
						A->PlayMonsterSound(E->SpriteClass, EMonsterSoundType::Stand, E->Actor->GetActorLocation());
				},
				Interval,
				true,           // looping
				InitialDelay    // first fire delay
			);
		}
	}

	// Register name tag
	float SpriteHeight = Sprite ? 150.f : 0.f;
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->RegisterEntity(PrimaryActor, Name, ENameTagEntityType::Monster, Level, 120.f, SpriteHeight);

	UE_LOG(LogEnemySubsystem, Verbose, TEXT("Spawned enemy %d (%s) at (%.0f, %.0f, %.0f)"),
		EnemyId, *Name, X, Y, Z);
}

// ============================================================
// HandleEnemyMove — set TargetPosition + bIsMoving
// ============================================================

void UEnemySubsystem::HandleEnemyMove(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
	int32 EnemyId = (int32)EnemyIdD;

	FEnemyEntry* Entry = Enemies.Find(EnemyId);
	if (!Entry || !Entry->Actor.IsValid()) return;
	AActor* Enemy = Entry->Actor.Get();

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	FVector NewPos((float)X, (float)Y, (float)Z);

	bool bIsMoving = false;
	Obj->TryGetBoolField(TEXT("isMoving"), bIsMoving);

	// Sprite enemies: C++ handles movement interpolation directly (bypasses BP Tick + CMC)
	if (Entry->SpriteActor.IsValid())
	{
		// Read walk speed from CharacterMovementComponent (once-ish, cached on sprite)
		float WalkSpeed = Entry->SpriteActor->ServerMoveSpeed;
		if (UCharacterMovementComponent* CMC = Enemy->FindComponentByClass<UCharacterMovementComponent>())
			WalkSpeed = CMC->MaxWalkSpeed;

		Entry->SpriteActor->SetServerTargetPosition(NewPos, bIsMoving, WalkSpeed);

		// Move SFX is no longer throttled here — it is fired by the SpriteCharacterActor's
		// OnAnimCycleComplete delegate (frame-locked to the Walk animation cycle for
		// authentic RO Classic Poring "boing" timing). The binding is set up once in
		// HandleEnemySpawn and persists for the entity's lifetime.

		// Drive sprite animation + facing
		if (Entry->SpriteActor->IsBodyReady())
		{
			auto State = Entry->SpriteActor->GetAnimState();
			// Don't override one-shot animations (Hit, Attack, Death) — let them finish
			if (State != ESpriteAnimState::Death && State != ESpriteAnimState::Hit
				&& State != ESpriteAnimState::Attack)
			{
				// CRITICAL: only call SetAnimState when the state ACTUALLY changes.
				// SetAnimState resets CurrentFrame=0 which would prevent the Walk cycle
				// from ever completing if we called it every move tick (~30Hz). That
				// would also break OnAnimCycleComplete — the hook that drives monster
				// move SFX (Poring "boing" cadence). Mirror the sprite's own
				// velocity-driven pattern at SpriteCharacterActor.cpp:2326.
				const ESpriteAnimState Desired = bIsMoving
					? ESpriteAnimState::Walk
					: ESpriteAnimState::Idle;
				if (State != Desired)
				{
					Entry->SpriteActor->SetAnimState(Desired);
				}

				if (bIsMoving)
				{
					FVector Dir = NewPos - Enemy->GetActorLocation();
					Dir.Z = 0.f;
					if (Dir.SizeSquared() > 1.f)
						Entry->SpriteActor->SetFacingDirection(Dir);
				}
			}
		}
	}
	else
	{
		// Non-sprite enemies: use BP Tick interpolation (existing path)
		SetBPVector(Enemy, TEXT("TargetPosition"), NewPos);
		SetBPBool(Enemy, TEXT("bIsMoving"), bIsMoving);
	}
}

// ============================================================
// HandleEnemyDeath — keep actor in map for respawn
// ============================================================

void UEnemySubsystem::HandleEnemyDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
	int32 EnemyId = (int32)EnemyIdD;

	// Check the MVP flag from the death payload (server emits isMvp:true for boss kills)
	bool bIsMvp = false;
	Obj->TryGetBoolField(TEXT("isMvp"), bIsMvp);

	FEnemyEntry* Entry = Enemies.Find(EnemyId);
	if (!Entry || !Entry->Actor.IsValid()) return;
	AActor* Enemy = Entry->Actor.Get();

	Entry->bIsDead = true;

	// Sprite enemy: sprite IS the primary actor
	if (Entry->SpriteActor.IsValid() && Entry->SpriteActor->IsBodyReady())
	{
		Entry->SpriteActor->SetAnimState(ESpriteAnimState::Death);
		Entry->SpriteActor->DisableClickCollision();

		// Play monster death SFX (RO Classic-style: <monster>_die.wav, the iconic poring "pop")
		if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
		{
			Audio->PlayMonsterSound(Entry->SpriteClass, EMonsterSoundType::Die, Enemy->GetActorLocation());

			// MVP fanfare — iconic RO Classic "you killed an MVP" stinger (2D non-spatial)
			if (bIsMvp)
			{
				Audio->PlayMvpFanfareSound();
			}
		}

		// Hide after corpse linger (4s)
		TWeakObjectPtr<AActor> WeakActor = Enemy;
		TWeakObjectPtr<UEnemySubsystem> WeakThis(this);
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis, WeakActor, EnemyId]()
		{
			if (!WeakThis.IsValid()) return;

			FEnemyEntry* E = WeakThis->Enemies.Find(EnemyId);
			if (E && !E->bIsDead) return;  // Already respawned

			if (WeakActor.IsValid())
				WeakActor->SetActorHiddenInGame(true);

			if (UNameTagSubsystem* NTS = WeakThis->GetWorld()->GetSubsystem<UNameTagSubsystem>())
			{
				if (WeakActor.IsValid()) NTS->SetVisible(WeakActor.Get(), false);
			}
		}, 4.0f, false);
	}
	else
	{
		// Non-sprite BP enemy: hide immediately, call BP death function
		SetBPBool(Enemy, TEXT("bIsDead"), true);
		Enemy->SetActorHiddenInGame(true);
		Enemy->SetActorEnableCollision(false);
		if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
			NTS->SetVisible(Enemy, false);

		CallBPFunction(Enemy, TEXT("OnEnemyDeath"),
			[Enemy](UFunction* Func, uint8* Params)
			{
				if (auto* P = CastField<FObjectProperty>(Func->FindPropertyByName(TEXT("InDeadEnemy"))))
					*P->ContainerPtrToValuePtr<UObject*>(Params) = Enemy;
			});
	}

	UE_LOG(LogEnemySubsystem, Verbose, TEXT("Enemy %d died."), EnemyId);
}

// ============================================================
// HandleEnemyHealthUpdate — call UpdateEnemyHealth on BP actor
// ============================================================

void UEnemySubsystem::HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
	int32 EnemyId = (int32)EnemyIdD;

	FEnemyEntry* Entry = Enemies.Find(EnemyId);
	if (!Entry || !Entry->Actor.IsValid()) return;
	AActor* Enemy = Entry->Actor.Get();

	double Health = 0, MaxHealth = 0;
	Obj->TryGetNumberField(TEXT("health"), Health);
	Obj->TryGetNumberField(TEXT("maxHealth"), MaxHealth);
	bool bInCombat = false;
	Obj->TryGetBoolField(TEXT("inCombat"), bInCombat);

	// Hit animation when health decreases
	if (Health < Entry->Health && Entry->SpriteActor.IsValid() && Entry->SpriteActor->IsBodyReady())
	{
		auto State = Entry->SpriteActor->GetAnimState();
		if (State != ESpriteAnimState::Death && State != ESpriteAnimState::Attack)
			Entry->SpriteActor->SetAnimState(ESpriteAnimState::Hit);

		// Play monster damage SFX (RO Classic-style: <monster>_damage.wav)
		if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
			Audio->PlayMonsterSound(Entry->SpriteClass, EMonsterSoundType::Damage, Enemy->GetActorLocation());
	}

	// Update struct
	Entry->Health = Health;
	Entry->MaxHealth = MaxHealth;

	CallBPFunction(Enemy, TEXT("UpdateEnemyHealth"),
		[&](UFunction* Func, uint8* Params)
		{
			if (auto* P = CastField<FDoubleProperty>(Func->FindPropertyByName(TEXT("NewHealth"))))
				*P->ContainerPtrToValuePtr<double>(Params) = Health;
			if (auto* P = CastField<FDoubleProperty>(Func->FindPropertyByName(TEXT("NewMaxHealth"))))
				*P->ContainerPtrToValuePtr<double>(Params) = MaxHealth;
			if (auto* P = CastField<FBoolProperty>(Func->FindPropertyByName(TEXT("InCombat"))))
				P->SetPropertyValue_InContainer(Params, bInCombat);
		});
}

// ============================================================
// HandleEnemyAttack — play attack animation (migrated from MultiplayerEventSubsystem C5)
// ============================================================

void UEnemySubsystem::HandleEnemyAttack(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD)) return;
	int32 EnemyId = (int32)EnemyIdD;

	FEnemyEntry* Entry = Enemies.Find(EnemyId);
	if (!Entry || !Entry->Actor.IsValid()) return;
	AActor* Enemy = Entry->Actor.Get();

	// Sprite attack animation
	if (Entry->SpriteActor.IsValid() && Entry->SpriteActor->IsBodyReady())
	{
		// Check if this is a skill cast (monster skill events include skillId)
		double SkillIdD = 0;
		Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
		int32 SkillId = (int32)SkillIdD;

		if (SkillId > 0)
		{
			// Monster skill cast — pick cast state based on targetType if available
			FString TargetType;
			Obj->TryGetStringField(TEXT("targetType"), TargetType);
			ESpriteAnimState CastState = ESpriteAnimState::CastSingle;
			if (TargetType == TEXT("self"))        CastState = ESpriteAnimState::CastSelf;
			else if (TargetType == TEXT("ground")) CastState = ESpriteAnimState::CastGround;
			else if (TargetType == TEXT("aoe"))    CastState = ESpriteAnimState::CastAoe;
			Entry->SpriteActor->SetAnimState(CastState);

			if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
				Audio->PlayMonsterSound(Entry->SpriteClass, EMonsterSoundType::Attack, Enemy->GetActorLocation());
		}
		else
		{
			Entry->SpriteActor->SetAnimState(ESpriteAnimState::Attack);

			if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
				Audio->PlayMonsterSound(Entry->SpriteClass, EMonsterSoundType::Attack, Enemy->GetActorLocation());

			// ---- Attack lunge: jolt sprite toward target ----
			// Find the target position (local player or another enemy)
			FVector TargetPos = FVector::ZeroVector;
			bool bHasTarget = false;

			double TargetIdD = 0;
			Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);

			// Try local player first
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					TargetPos = Pawn->GetActorLocation();
					bHasTarget = true;
				}
			}

			if (bHasTarget)
			{
				FVector EnemyPos = Enemy->GetActorLocation();
				float DistToTarget = FVector::Dist2D(EnemyPos, TargetPos);
				static constexpr float LungeDistance = 60.f;
				static constexpr float MinClearance = 40.f;
				static constexpr float WindUpDistance = 20.f;
				static constexpr float MeleeThreshold = 250.f; // server MELEE_RANGE(150)+TOLERANCE(50)+buffer
				static constexpr float WindUpDelay = 0.08f;    // wind-up duration
				static constexpr float LungeHold = 0.10f;     // hold at peak
				static constexpr float ReturnDelay = 0.15f;    // snap back

				float MaxLunge = FMath::Max(0.f, DistToTarget - MinClearance);
				float ActualLunge = FMath::Min(LungeDistance, MaxLunge);

				if (ActualLunge >= 5.f && DistToTarget <= MeleeThreshold)
				{
					// ---- Melee: wind-up + lunge toward target ----
					FVector Dir = (TargetPos - EnemyPos).GetSafeNormal2D();
					FVector OriginalPos = Entry->SpriteActor->ServerTargetPos;
					FVector WindUpPos = EnemyPos - Dir * WindUpDistance;
					FVector LungePos = EnemyPos + Dir * ActualLunge;

					TWeakObjectPtr<UEnemySubsystem> WeakThis(this);
					int32 LungeEnemyId = EnemyId;

					Entry->SpriteActor->ServerTargetPos = WindUpPos;

					FTimerHandle LungeTimer;
					GetWorld()->GetTimerManager().SetTimer(LungeTimer,
						[WeakThis, LungeEnemyId, LungePos]()
						{
							if (!WeakThis.IsValid()) return;
							FEnemyEntry* E = WeakThis->Enemies.Find(LungeEnemyId);
							if (!E || E->bIsDead || !E->SpriteActor.IsValid()) return;
							E->SpriteActor->ServerTargetPos = LungePos;
						},
						WindUpDelay, false);

					FTimerHandle ReturnTimer;
					GetWorld()->GetTimerManager().SetTimer(ReturnTimer,
						[WeakThis, LungeEnemyId, OriginalPos]()
						{
							if (!WeakThis.IsValid()) return;
							FEnemyEntry* E = WeakThis->Enemies.Find(LungeEnemyId);
							if (!E || E->bIsDead || !E->SpriteActor.IsValid()) return;
							E->SpriteActor->ServerTargetPos = OriginalPos;
						},
						WindUpDelay + LungeHold + ReturnDelay, false);
				}
				else if (DistToTarget > MeleeThreshold)
				{
					// ---- Ranged: ground vine decal at target's feet ----
					// Spawns a temporary RootTendril/CrackedEarth decal that fades after 0.8s
					UMaterialInterface* VineDecalMat = Cast<UMaterialInterface>(
						StaticLoadObject(UMaterialInterface::StaticClass(), nullptr,
							TEXT("/Game/SabriMMO/Materials/Environment/Decals/Instances/MI_T_Decal_RootTendril.MI_T_Decal_RootTendril")));
					if (!VineDecalMat)
						VineDecalMat = Cast<UMaterialInterface>(StaticLoadObject(
							UMaterialInterface::StaticClass(), nullptr,
							TEXT("/Game/SabriMMO/Materials/Environment/Decals/Instances/MI_T_Decal_CrackedEarth.MI_T_Decal_CrackedEarth")));

					if (VineDecalMat)
					{
						FVector DecalSize(80.f, 80.f, 80.f);
						FRotator DecalRot(-90.f, FMath::FRandRange(0.f, 360.f), 0.f);
						UGameplayStatics::SpawnDecalAtLocation(
							GetWorld(), VineDecalMat, DecalSize,
							TargetPos, DecalRot, 0.8f);
					}
				}
			}
		}
	}

	CallBPFunction(Enemy, TEXT("PlayAttackAnimation"));
}

// ============================================================
// HandleCombatKnockback — move enemy to new position after knockback
// ============================================================

void UEnemySubsystem::HandleCombatKnockback(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Only handle enemy knockbacks
	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	if (!bIsEnemy) return;

	double TargetIdD = 0;
	if (!Obj->TryGetNumberField(TEXT("targetId"), TargetIdD)) return;
	int32 EnemyId = (int32)TargetIdD;

	AActor* Enemy = GetEnemy(EnemyId);
	if (!Enemy) return;

	double NewX = 0, NewY = 0, NewZ = 0;
	Obj->TryGetNumberField(TEXT("newX"), NewX);
	Obj->TryGetNumberField(TEXT("newY"), NewY);
	Obj->TryGetNumberField(TEXT("newZ"), NewZ);

	// Set TargetPosition for BP_EnemyCharacter Tick interpolation
	SetBPVector(Enemy, TEXT("TargetPosition"), FVector((float)NewX, (float)NewY, (float)NewZ));
	SetBPBool(Enemy, TEXT("bIsMoving"), true);

	UE_LOG(LogEnemySubsystem, Verbose, TEXT("Knockback enemy %d to (%.0f, %.0f, %.0f)"),
		EnemyId, NewX, NewY, NewZ);
}

// ============================================================
// HandleSenseResult — show monster info popup (Wizard Sense / Sage Sense)
// ============================================================

void UEnemySubsystem::HandleSenseResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	ShowSensePopup(Data);
}

void UEnemySubsystem::ShowSensePopup(const TSharedPtr<FJsonValue>& Data)
{
	HideSensePopup();

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FSenseResultData SenseData;
	double D = 0;
	if (Obj->TryGetNumberField(TEXT("targetId"), D)) SenseData.TargetId = (int32)D;
	Obj->TryGetStringField(TEXT("targetName"), SenseData.TargetName);
	if (Obj->TryGetNumberField(TEXT("level"), D)) SenseData.Level = (int32)D;
	Obj->TryGetNumberField(TEXT("health"), SenseData.Health);
	Obj->TryGetNumberField(TEXT("maxHealth"), SenseData.MaxHealth);
	Obj->TryGetStringField(TEXT("element"), SenseData.Element);
	if (Obj->TryGetNumberField(TEXT("elementLevel"), D)) SenseData.ElementLevel = (int32)D;
	Obj->TryGetStringField(TEXT("race"), SenseData.Race);
	Obj->TryGetStringField(TEXT("size"), SenseData.Size);
	if (Obj->TryGetNumberField(TEXT("hardDef"), D)) SenseData.HardDef = (int32)D;
	if (Obj->TryGetNumberField(TEXT("hardMdef"), D)) SenseData.HardMdef = (int32)D;
	if (Obj->TryGetNumberField(TEXT("str"), D)) SenseData.STR = (int32)D;
	if (Obj->TryGetNumberField(TEXT("agi"), D)) SenseData.AGI = (int32)D;
	if (Obj->TryGetNumberField(TEXT("vit"), D)) SenseData.VIT = (int32)D;
	if (Obj->TryGetNumberField(TEXT("int"), D)) SenseData.INT = (int32)D;
	if (Obj->TryGetNumberField(TEXT("dex"), D)) SenseData.DEX = (int32)D;
	if (Obj->TryGetNumberField(TEXT("luk"), D)) SenseData.LUK = (int32)D;
	if (Obj->TryGetNumberField(TEXT("baseExp"), D)) SenseData.BaseExp = (int32)D;
	if (Obj->TryGetNumberField(TEXT("jobExp"), D)) SenseData.JobExp = (int32)D;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	SensePopup = SNew(SSenseResultPopup)
		.Subsystem(this)
		.SenseData(SenseData);

	// AlignmentWrapper with SelfHitTestInvisible — clicks pass through empty area to the game
	SenseAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			SensePopup.ToSharedRef()
		];

	SenseOverlay = SNew(SWeakWidget).PossiblyNullContent(SenseAlignWrapper);
	VC->AddViewportWidgetContent(SenseOverlay.ToSharedRef(), 24);
	bSensePopupVisible = true;

	UE_LOG(LogEnemySubsystem, Log, TEXT("Sense popup shown for %s (ID %d)"),
		*SenseData.TargetName, SenseData.TargetId);
}

void UEnemySubsystem::HideSensePopup()
{
	if (!bSensePopupVisible) return;

	if (SenseOverlay.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* VC = World->GetGameViewport())
			{
				VC->RemoveViewportWidgetContent(SenseOverlay.ToSharedRef());
			}
		}
	}
	SensePopup.Reset();
	SenseAlignWrapper.Reset();
	SenseOverlay.Reset();
	bSensePopupVisible = false;

	UE_LOG(LogEnemySubsystem, Log, TEXT("Sense popup hidden."));
}
