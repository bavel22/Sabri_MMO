// EnemySubsystem.cpp — Enemy entity management: spawn, move, death, health, attack.
// Phase 3 of Blueprint-to-C++ migration. Replaces BP_EnemyManager.

#include "EnemySubsystem.h"
#include "NameTagSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

	// Defer readiness by one frame (prevents ProcessEvent during PostLoad)
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	UE_LOG(LogEnemySubsystem, Log, TEXT("EnemySubsystem — 5 enemy events registered."));
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

	// Destroy all spawned enemy actors
	for (auto& Pair : Enemies)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Destroy();
		}
	}
	Enemies.Empty();

	bReadyToProcess = false;
	EnemyBPClass = nullptr;

	Super::Deinitialize();
}

// ============================================================
// Public API
// ============================================================

AActor* UEnemySubsystem::GetEnemy(int32 EnemyId) const
{
	const TWeakObjectPtr<AActor>* Found = Enemies.Find(EnemyId);
	if (Found && Found->IsValid())
		return Found->Get();
	return nullptr;
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
	TWeakObjectPtr<AActor>* Existing = Enemies.Find(EnemyId);
	if (Existing && Existing->IsValid())
	{
		AActor* Enemy = Existing->Get();

		if (GetBPBool(Enemy, TEXT("bIsDead")))
		{
			// Dead respawn: re-show, re-enable collision, re-initialize
			Enemy->SetActorHiddenInGame(false);
			Enemy->SetActorEnableCollision(true);
			SetBPBool(Enemy, TEXT("bIsDead"), false);
			Enemy->SetActorLocation(Pos);
			InitEnemy(Enemy);
			SetBPVector(Enemy, TEXT("TargetPosition"), Pos);

			// Re-show name tag on respawn
			if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
				NTS->SetVisible(Enemy, true);

			UE_LOG(LogEnemySubsystem, Verbose, TEXT("Respawned enemy %d (%s)"), EnemyId, *Name);
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
	if (!World || !EnemyBPClass) return;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Pos);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewEnemy = World->SpawnActor(EnemyBPClass, &SpawnTransform, SpawnParams);
	if (!NewEnemy)
	{
		UE_LOG(LogEnemySubsystem, Warning, TEXT("Failed to spawn enemy %d (%s)"), EnemyId, *Name);
		return;
	}

	InitEnemy(NewEnemy);
	SetBPVector(NewEnemy, TEXT("TargetPosition"), Pos);
	Enemies.Add(EnemyId, NewEnemy);

	// Register name tag (RO Classic: hover-only for monsters)
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->RegisterEntity(NewEnemy, Name, ENameTagEntityType::Monster, Level);

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

	AActor* Enemy = GetEnemy(EnemyId);
	if (!Enemy) return;

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	SetBPVector(Enemy, TEXT("TargetPosition"), FVector((float)X, (float)Y, (float)Z));

	bool bIsMoving = false;
	Obj->TryGetBoolField(TEXT("isMoving"), bIsMoving);
	SetBPBool(Enemy, TEXT("bIsMoving"), bIsMoving);
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

	AActor* Enemy = GetEnemy(EnemyId);
	if (!Enemy) return;

	SetBPBool(Enemy, TEXT("bIsDead"), true);

	// Hide name tag on death (RO Classic: monster names disappear with sprite)
	if (UNameTagSubsystem* NTS = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NTS->SetVisible(Enemy, false);

	// Call OnEnemyDeath(InDeadEnemy) — BP plays death animation, hides mesh, etc.
	CallBPFunction(Enemy, TEXT("OnEnemyDeath"),
		[Enemy](UFunction* Func, uint8* Params)
		{
			if (auto* P = CastField<FObjectProperty>(Func->FindPropertyByName(TEXT("InDeadEnemy"))))
				*P->ContainerPtrToValuePtr<UObject*>(Params) = Enemy;
		});

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

	AActor* Enemy = GetEnemy(EnemyId);
	if (!Enemy) return;

	double Health = 0, MaxHealth = 0;
	Obj->TryGetNumberField(TEXT("health"), Health);
	Obj->TryGetNumberField(TEXT("maxHealth"), MaxHealth);
	bool bInCombat = false;
	Obj->TryGetBoolField(TEXT("inCombat"), bInCombat);

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

	AActor* Enemy = GetEnemy(EnemyId);
	if (!Enemy) return;

	CallBPFunction(Enemy, TEXT("PlayAttackAnimation"));
}
