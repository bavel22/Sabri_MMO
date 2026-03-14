// SkillVFXSubsystem.cpp — Spawns Niagara VFX for skills, casting circles, buffs.

#include "SkillVFXSubsystem.h"
#include "CastingCircleActor.h"
#include "MMOGameInstance.h"
#include "CharacterData.h"
#include "SocketEventRouter.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkillVFX, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool USkillVFXSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void USkillVFXSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Try to load Niagara systems from expected asset paths.
	// If the assets don't exist yet, these will be null — that's fine,
	// we just won't spawn VFX for those template types until they're created.
	auto TryLoad = [](const TCHAR* Path) -> UNiagaraSystem*
	{
		return LoadObject<UNiagaraSystem>(nullptr, Path);
	};

	// --- Load from migrated packs ---
	// Bolt/Lightning: Mixed_Magic has NS_Lightning_Strike (best for bolt-from-sky effects)
	NS_BoltFromSky     = TryLoad(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike.NS_Lightning_Strike"));
	// Projectile: Free_Magic has NS_Free_Magic_Projectile1 (travel + impact)
	NS_Projectile      = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1.NS_Free_Magic_Projectile1"));
	// AoE Impact: Free_Magic has NS_Free_Magic_Hit1 (burst on impact)
	NS_AoEImpact       = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1.NS_Free_Magic_Hit1"));
	// Ground Persistent: Free_Magic has NS_Free_Magic_Area1 (looping ground effect)
	NS_GroundPersistent = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Area1.NS_Free_Magic_Area1"));
	// Ground AoE Rain: Reuse lightning strike (spawned at random positions in area)
	NS_GroundAoERain   = TryLoad(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike.NS_Lightning_Strike"));
	// Self Buff: Free_Magic has NS_Free_Magic_Buff (aura around character)
	NS_SelfBuff        = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Buff.NS_Free_Magic_Buff"));
	// Target Debuff: NiagaraExamples has NS_Player_DeBuff_Looping
	NS_TargetDebuff    = TryLoad(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_DeBuff_Looping.NS_Player_DeBuff_Looping"));
	// Heal Flash: Mixed_Magic has NS_Potion (healing potion effect)
	NS_HealFlash       = TryLoad(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion"));

	// --- Fallbacks: try alternate paths if primary didn't load ---
	if (!NS_BoltFromSky)  NS_BoltFromSky  = TryLoad(TEXT("/Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_03_Yellow.NS_Zap_03_Yellow"));
	if (!NS_Projectile)   NS_Projectile   = TryLoad(TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Projectile.NS_Magma_Shot_Projectile"));
	if (!NS_AoEImpact)    NS_AoEImpact    = TryLoad(TEXT("/Game/NiagaraExamples/FX_Explosions/NS_Explosion_Small.NS_Explosion_Small"));
	if (!NS_SelfBuff)     NS_SelfBuff     = TryLoad(TEXT("/Game/NiagaraExamples/FX_Player/NS_Player_Buff_Looping.NS_Player_Buff_Looping"));
	if (!NS_HealFlash)    NS_HealFlash    = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2.NS_Free_Magic_Hit2"));

	MI_CastingCircle = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/SabriMMO/VFX/Materials/M_CastingCircle.M_CastingCircle"));

	// Also load the Free_Magic circle as a Niagara-based casting circle alternative
	// This can be used instead of or alongside the decal-based circle
	if (!NS_CastingCircle)
	{
		NS_CastingCircle = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle1.NS_Free_Magic_Circle1"));
		if (!NS_CastingCircle)
			NS_CastingCircle = TryLoad(TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle2.NS_Free_Magic_Circle2"));
	}

	int32 LoadedCount = 0;
	if (NS_BoltFromSky)     ++LoadedCount;
	if (NS_Projectile)      ++LoadedCount;
	if (NS_AoEImpact)       ++LoadedCount;
	if (NS_GroundPersistent) ++LoadedCount;
	if (NS_GroundAoERain)   ++LoadedCount;
	if (NS_SelfBuff)        ++LoadedCount;
	if (NS_TargetDebuff)    ++LoadedCount;
	if (NS_HealFlash)       ++LoadedCount;

	UE_LOG(LogSkillVFX, Log, TEXT("SkillVFXSubsystem started — loaded %d/8 Niagara templates, CastCircleMat=%s"),
		LoadedCount, MI_CastingCircle ? TEXT("YES") : TEXT("NO"));

	// Register socket event handlers via persistent EventRouter
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (GI)
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;

		USocketEventRouter* Router = GI->GetEventRouter();
		if (Router)
		{
			Router->RegisterHandler(TEXT("skill:cast_start"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleCastStart(D); });
			Router->RegisterHandler(TEXT("skill:cast_complete"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleCastComplete(D); });
			Router->RegisterHandler(TEXT("skill:cast_interrupted_broadcast"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterrupted(D); });
			Router->RegisterHandler(TEXT("skill:cast_interrupted"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleCastInterrupted(D); });
			Router->RegisterHandler(TEXT("skill:effect_damage"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleSkillEffectDamage(D); });
			Router->RegisterHandler(TEXT("skill:buff_applied"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleBuffApplied(D); });
			Router->RegisterHandler(TEXT("skill:buff_removed"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleBuffRemoved(D); });
			Router->RegisterHandler(TEXT("skill:ground_effect_created"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleGroundEffectCreated(D); });
			Router->RegisterHandler(TEXT("skill:ground_effect_removed"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleGroundEffectRemoved(D); });
			Router->RegisterHandler(TEXT("combat:health_update"), this,
				[this](const TSharedPtr<FJsonValue>& D) { HandleCombatHealthUpdate(D); });
		}
	}

	UE_LOG(LogSkillVFX, Log, TEXT("SkillVFXSubsystem — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);

	// Defer VFX processing by one frame — prevents actor access / component spawning during PostLoad.
	// (UE5 assertion: "Cannot call UnrealScript while PostLoading objects")
	// VFX events during PostLoad are safely dropped — purely cosmetic, no gameplay impact.
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	// Re-activate non-looping Cascade buff particles every 2s so they persist until buff removal.
	// Cascade PSCs can report IsActive()==true even after all particles have faded,
	// so we unconditionally re-activate to ensure continuous visual emission.
	InWorld.GetTimerManager().SetTimer(
		CascadeLoopTimer,
		FTimerDelegate::CreateLambda([this]()
		{
			for (auto It = ActiveCascadeBuffs.CreateIterator(); It; ++It)
			{
				if (It.Value().IsValid())
				{
					UParticleSystemComponent* PSC = It.Value().Get();
					PSC->ActivateSystem(true); // reset and replay unconditionally
				}
				else
				{
					It.RemoveCurrent(); // clean up stale entries
				}
			}
		}),
		2.0f,
		true
	);
}

void USkillVFXSubsystem::Deinitialize()
{
	bReadyToProcess = false;

	// Destroy all active casting circles
	for (auto& Pair : ActiveCastingCircles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Destroy();
		}
	}
	ActiveCastingCircles.Empty();

	// Deactivate persistent effects
	for (auto& Pair : ActivePersistentEffects)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->DeactivateImmediate();
		}
	}
	ActivePersistentEffects.Empty();

	// Deactivate buff auras
	for (auto& Pair : ActiveBuffAuras)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->DeactivateImmediate();
		}
	}
	ActiveBuffAuras.Empty();

	// Deactivate Cascade buff auras
	for (auto& Pair : ActiveCascadeBuffs)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->DeactivateImmediate();
			Pair.Value->DestroyComponent();
		}
	}
	ActiveCascadeBuffs.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CascadeLoopTimer);

		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}
	}

	UE_LOG(LogSkillVFX, Log, TEXT("SkillVFXSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Event handlers
// ============================================================

void USkillVFXSubsystem::HandleCastStart(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!bVFXEnabled || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CasterIdD = 0, SkillIdD = 0, CastTimeD = 0;
	double CasterXD = 0, CasterYD = 0, CasterZD = 0;
	Obj->TryGetNumberField(TEXT("casterId"), CasterIdD);
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	Obj->TryGetNumberField(TEXT("actualCastTime"), CastTimeD);
	Obj->TryGetNumberField(TEXT("casterX"), CasterXD);
	Obj->TryGetNumberField(TEXT("casterY"), CasterYD);
	Obj->TryGetNumberField(TEXT("casterZ"), CasterZD);

	const int32 CasterId = static_cast<int32>(CasterIdD);
	const int32 SkillId = static_cast<int32>(SkillIdD);
	const float CastTimeSec = static_cast<float>(CastTimeD) / 1000.f;

	// Show casting circle for ANY skill with a cast time (skip instant skills)
	if (CastTimeSec <= 0.f) return;

	const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(SkillId);

	// RO style: casting circle always appears at the CASTER's feet.
	// Try actor location first (accurate real-time position), fall back to
	// event coordinates (casterX/Y/Z) for remote players whose actors
	// may not have a PlayerId tag.
	FVector CircleLocation = GetActorLocationById(CasterId, false);
	if (CircleLocation.IsZero())
	{
		CircleLocation = FVector(CasterXD, CasterYD, CasterZD);
	}

	if (!CircleLocation.IsZero())
	{
		SpawnCastingCircle(CasterId, CircleLocation, Config, CastTimeSec);
	}
	else
	{
		UE_LOG(LogSkillVFX, Warning, TEXT("Could not find caster %d for casting circle (SkillId=%d)"), CasterId, SkillId);
	}
}

void USkillVFXSubsystem::HandleCastComplete(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	double CasterIdD = 0;
	(*ObjPtr)->TryGetNumberField(TEXT("casterId"), CasterIdD);
	RemoveCastingCircle(static_cast<int32>(CasterIdD));
}

void USkillVFXSubsystem::HandleCastInterrupted(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	double CasterIdD = 0;
	(*ObjPtr)->TryGetNumberField(TEXT("casterId"), CasterIdD);
	RemoveCastingCircle(static_cast<int32>(CasterIdD));
}

void USkillVFXSubsystem::HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!bVFXEnabled || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double AttackerIdD = 0, TargetIdD = 0, SkillIdD = 0, DamageD = 0;
	double TargetXD = 0, TargetYD = 0, TargetZD = 0;
	double AttackerXD = 0, AttackerYD = 0, AttackerZD = 0;
	double HitNumberD = 0, TotalHitsD = 0;
	bool bIsEnemy = false;
	FString Element;

	Obj->TryGetNumberField(TEXT("attackerId"), AttackerIdD);
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	Obj->TryGetNumberField(TEXT("damage"), DamageD);
	Obj->TryGetNumberField(TEXT("targetX"), TargetXD);
	Obj->TryGetNumberField(TEXT("targetY"), TargetYD);
	Obj->TryGetNumberField(TEXT("targetZ"), TargetZD);
	Obj->TryGetNumberField(TEXT("attackerX"), AttackerXD);
	Obj->TryGetNumberField(TEXT("attackerY"), AttackerYD);
	Obj->TryGetNumberField(TEXT("attackerZ"), AttackerZD);
	Obj->TryGetNumberField(TEXT("hitNumber"), HitNumberD);
	Obj->TryGetNumberField(TEXT("hits"), TotalHitsD);       // bolt skills send "hits" = numHits
	Obj->TryGetNumberField(TEXT("totalHits"), TotalHitsD);   // combat:damage sends "totalHits"
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetStringField(TEXT("element"), Element);

	const int32 SkillId = static_cast<int32>(SkillIdD);
	const int32 HitNumber = static_cast<int32>(HitNumberD);
	const int32 TotalHits = FMath::Max(1, static_cast<int32>(TotalHitsD));
	FVector TargetLoc(TargetXD, TargetYD, TargetZD);
	FVector AttackerLoc(AttackerXD, AttackerYD, AttackerZD);

	// If target position is zero, try to find actor
	if (TargetLoc.IsNearlyZero())
	{
		TargetLoc = GetActorLocationById(static_cast<int32>(TargetIdD), bIsEnemy);
	}
	if (AttackerLoc.IsNearlyZero())
	{
		AttackerLoc = GetActorLocationById(static_cast<int32>(AttackerIdD), false);
	}

	// Remove casting circle for this caster (cast is complete, damage is arriving)
	RemoveCastingCircle(static_cast<int32>(AttackerIdD));

	const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(SkillId);
	if (Config.Template == ESkillVFXTemplate::None) return;

	switch (Config.Template)
	{
	case ESkillVFXTemplate::BoltFromSky:
		SpawnBoltFromSky(TargetLoc, Config, TotalHits);
		break;

	case ESkillVFXTemplate::Projectile:
	{
		if (Config.bSingleProjectile)
		{
			// AoE projectile (Fire Ball): one projectile toward primary target
			const int64 Key = static_cast<int64>(AttackerIdD) * 10000 + SkillId;
			const double Now = FPlatformTime::Seconds();
			bool bSpawn = true;
			if (const double* Last = SingleProjectileLastSpawnTime.Find(Key))
			{
				if (Now - *Last < 1.0) bSpawn = false;
			}
			if (bSpawn)
			{
				SingleProjectileLastSpawnTime.Add(Key, Now);
				FVector ProjectileDestination = TargetLoc;
				double PrimaryTargetIdD = 0;
				bool bPrimaryIsEnemy = false;
				Obj->TryGetNumberField(TEXT("primaryTargetId"), PrimaryTargetIdD);
				Obj->TryGetBoolField(TEXT("primaryTargetIsEnemy"), bPrimaryIsEnemy);
				if (PrimaryTargetIdD > 0)
				{
					FVector PrimaryLoc = GetActorLocationById(static_cast<int32>(PrimaryTargetIdD), bPrimaryIsEnemy);
					if (!PrimaryLoc.IsNearlyZero()) ProjectileDestination = PrimaryLoc;
				}
				SpawnProjectileEffect(AttackerLoc, ProjectileDestination, Config);
			}
		}
		else if (TotalHits > 1)
		{
			// Multi-hit projectile (Soul Strike): N projectiles staggered by 200ms
			SpawnMultiHitProjectile(AttackerLoc, TargetLoc, Config, TotalHits);
		}
		else
		{
			// Single-target single-hit projectile
			SpawnProjectileEffect(AttackerLoc, TargetLoc, Config);
		}
		break;
	}

	case ESkillVFXTemplate::AoEImpact:
		// Only spawn once (on first hit for multi-hit AoEs)
		if (HitNumber <= 1)
		{
			// Self-centered AoE (Magnum Break) spawns at caster, not target
			FVector AoELocation = Config.bSelfCentered ? AttackerLoc : TargetLoc;
			SpawnAoEImpact(AoELocation, Config);
		}
		break;

	case ESkillVFXTemplate::GroundPersistent:
		// Ground persistent VFX (Fire Wall, Safety Wall) are spawned by HandleGroundEffectCreated,
		// NOT from damage events. The server sends skill:ground_effect_created with the full
		// lifecycle (effectId, position, duration). Spawning here would cause double-VFX.
		break;

	case ESkillVFXTemplate::GroundAoERain:
		// Spawn per-hit (each hit = one lightning strike in the area)
		SpawnGroundAoERain(TargetLoc, Config, HitNumber);
		break;

	case ESkillVFXTemplate::HealFlash:
		SpawnHealFlash(TargetLoc, Config);
		break;

	case ESkillVFXTemplate::SelfBuff:
	{
		// Find target actor and spawn attached (same as HandleBuffApplied SelfBuff path)
		AActor* SelfBuffActor = FindPlayerActorById(static_cast<int32>(TargetIdD));
		if (SelfBuffActor)
		{
			SpawnSelfBuff(SelfBuffActor, Config, SkillId, static_cast<int32>(TargetIdD));
		}
		else
		{
			SpawnVFXAtLocation(Config, TargetLoc);
		}
		break;
	}

	case ESkillVFXTemplate::TargetDebuff:
	{
		// Try to attach to the target actor so the VFX follows them
		AActor* TargetActor = bIsEnemy
			? FindEnemyActorById(static_cast<int32>(TargetIdD))
			: FindPlayerActorById(static_cast<int32>(TargetIdD));

		UE_LOG(LogSkillVFX, Warning, TEXT("TargetDebuff path: SkillId=%d TargetId=%d IsEnemy=%d Actor=%s Scale=%.3f Override=%s"),
			SkillId, static_cast<int32>(TargetIdD), bIsEnemy,
			TargetActor ? *TargetActor->GetName() : TEXT("NULL"),
			Config.Scale, *Config.VFXOverridePath);

		if (TargetActor && TargetActor->GetRootComponent())
		{
			SpawnVFXAttached(Config, TargetActor->GetRootComponent());
		}
		else if (!TargetLoc.IsNearlyZero())
		{
			UE_LOG(LogSkillVFX, Warning, TEXT("TargetDebuff FALLBACK to SpawnVFXAtLocation (no actor found)"));
			SpawnVFXAtLocation(Config, TargetLoc + FVector(0, 0, 50.f));
		}
		break;
	}

	default:
		break;
	}
}

void USkillVFXSubsystem::HandleBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	UE_LOG(LogSkillVFX, Log, TEXT("HandleBuffApplied ENTERED — bVFXEnabled=%d DataValid=%d"), bVFXEnabled, Data.IsValid());
	if (!bVFXEnabled || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0, SkillIdD = 0, DurationD = 0;
	bool bIsEnemy = false;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	Obj->TryGetNumberField(TEXT("duration"), DurationD);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	const int32 TargetId = static_cast<int32>(TargetIdD);
	const int32 SkillId = static_cast<int32>(SkillIdD);
	UE_LOG(LogSkillVFX, Log, TEXT("HandleBuffApplied — SkillId=%d TargetId=%d IsEnemy=%d"), SkillId, TargetId, bIsEnemy);

	const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(SkillId);

	// Handle SelfBuff template (Endure, Sight, etc.) — spawn attached to target
	if (Config.Template == ESkillVFXTemplate::SelfBuff)
	{
		AActor* TargetActor = bIsEnemy
			? FindEnemyActorById(TargetId)
			: FindPlayerActorById(TargetId);

		if (TargetActor)
		{
			SpawnSelfBuff(TargetActor, Config, SkillId, TargetId);
		}
		else
		{
			FVector BuffLoc = GetActorLocationById(TargetId, bIsEnemy);
			if (!BuffLoc.IsNearlyZero())
			{
				SpawnVFXAtLocation(Config, BuffLoc);
			}
		}
	}
	// Handle TargetDebuff template (Provoke, Frost Diver, etc.) — attach to target actor and track for removal
	else if (Config.Template == ESkillVFXTemplate::TargetDebuff)
	{
		AActor* TargetActor = bIsEnemy
			? FindEnemyActorById(TargetId)
			: FindPlayerActorById(TargetId);

		UE_LOG(LogSkillVFX, Log, TEXT("HandleBuffApplied TargetDebuff — SkillId=%d TargetId=%d IsEnemy=%d Actor=%s Override=%s bCascade=%d Scale=%.1f"),
			SkillId, TargetId, bIsEnemy, TargetActor ? *TargetActor->GetName() : TEXT("NULL"), *Config.VFXOverridePath, Config.bIsCascade, Config.Scale);

		const int64 Key = static_cast<int64>(TargetId) * 10000 + SkillId;

		// Remove any existing effect for this target+skill combo first
		if (TWeakObjectPtr<UParticleSystemComponent>* OldCascade = ActiveCascadeBuffs.Find(Key))
		{
			if (OldCascade->IsValid()) { OldCascade->Get()->DeactivateImmediate(); OldCascade->Get()->DestroyComponent(); }
			ActiveCascadeBuffs.Remove(Key);
		}
		if (TWeakObjectPtr<UNiagaraComponent>* OldNiagara = ActiveBuffAuras.Find(Key))
		{
			if (OldNiagara->IsValid()) { OldNiagara->Get()->DeactivateImmediate(); OldNiagara->Get()->DestroyComponent(); }
			ActiveBuffAuras.Remove(Key);
		}

		if (TargetActor && TargetActor->GetRootComponent())
		{
			USceneComponent* Root = TargetActor->GetRootComponent();
			FVector FinalScale = FVector(Config.Scale);

			if (Config.bIsCascade && !Config.VFXOverridePath.IsEmpty())
			{
				UParticleSystem* CascadeSystem = GetOrLoadCascadeOverride(Config.VFXOverridePath);
				if (CascadeSystem)
				{
					UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(
						CascadeSystem, Root, NAME_None,
						FVector::ZeroVector, FRotator::ZeroRotator, FinalScale,
						EAttachLocation::KeepRelativeOffset, false); // bAutoDestroy=false
					if (PSC)
					{
						ActiveCascadeBuffs.Add(Key, PSC);
					}
				}
			}
			else if (!Config.VFXOverridePath.IsEmpty())
			{
				UNiagaraSystem* NiagaraSystem = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
				if (NiagaraSystem)
				{
					UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(
						NiagaraSystem, Root, NAME_None,
						FVector::ZeroVector, FRotator::ZeroRotator, FinalScale,
						EAttachLocation::KeepRelativeOffset, false, // bAutoDestroy=false so it persists
						ENCPoolMethod::None);
					if (Comp)
					{
						SetNiagaraColor(Comp, Config.PrimaryColor);
						SetNiagaraScale(Comp, Config.Scale);
						ActiveBuffAuras.Add(Key, Comp);
					}
				}
			}
		}
		else
		{
			// Fallback to location-based spawn (no tracking — fire-and-forget)
			FVector TargetLoc = GetActorLocationById(TargetId, bIsEnemy);
			if (!TargetLoc.IsNearlyZero())
			{
				SpawnVFXAtLocation(Config, TargetLoc + FVector(0, 0, 150.f));
			}
		}
	}
	// Handle HealFlash or any other template triggered by buff events
	else if (Config.Template != ESkillVFXTemplate::None)
	{
		FVector TargetLoc = GetActorLocationById(TargetId, bIsEnemy);
		if (!TargetLoc.IsNearlyZero())
		{
			SpawnVFXAtLocation(Config, TargetLoc);
		}
	}
}

void USkillVFXSubsystem::HandleBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0, SkillIdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);

	const int64 Key = static_cast<int64>(TargetIdD) * 10000 + static_cast<int64>(SkillIdD);

	// Clean up Niagara buff
	if (TWeakObjectPtr<UNiagaraComponent>* Found = ActiveBuffAuras.Find(Key))
	{
		if (Found->IsValid())
		{
			Found->Get()->DeactivateImmediate();
			Found->Get()->DestroyComponent();
		}
		ActiveBuffAuras.Remove(Key);
	}

	// Clean up Cascade buff (Frost Diver, etc.)
	if (TWeakObjectPtr<UParticleSystemComponent>* CascadeFound = ActiveCascadeBuffs.Find(Key))
	{
		if (CascadeFound->IsValid())
		{
			CascadeFound->Get()->DeactivateImmediate();
			CascadeFound->Get()->DestroyComponent();
		}
		ActiveCascadeBuffs.Remove(Key);
	}
}

// ============================================================
// Ground effect lifecycle (Fire Wall, Safety Wall)
// ============================================================

// File-local Cascade ground effect tracking (avoids header change for TMap<int32, PSC*>)
static TMap<int32, TWeakObjectPtr<UParticleSystemComponent>>& GetCascadeGroundEffects()
{
	static auto* Map = new TMap<int32, TWeakObjectPtr<UParticleSystemComponent>>();
	return *Map;
}

void USkillVFXSubsystem::HandleGroundEffectCreated(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!bVFXEnabled || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EffectIdD = 0, XD = 0, YD = 0, ZD = 0, RadiusD = 0;
	FString Type, Element;
	Obj->TryGetNumberField(TEXT("effectId"), EffectIdD);
	Obj->TryGetNumberField(TEXT("x"), XD);
	Obj->TryGetNumberField(TEXT("y"), YD);
	Obj->TryGetNumberField(TEXT("z"), ZD);
	Obj->TryGetNumberField(TEXT("radius"), RadiusD);
	Obj->TryGetStringField(TEXT("type"), Type);
	Obj->TryGetStringField(TEXT("element"), Element);

	const int32 EffectId = static_cast<int32>(EffectIdD);
	FVector Location(XD, YD, ZD);

	// Map server type string to skill ID for config lookup
	int32 SkillId = 0;
	if (Type == TEXT("fire_wall")) SkillId = 209;
	else if (Type == TEXT("safety_wall")) SkillId = 211;

	const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(SkillId);

	UWorld* World = GetWorld();
	if (!World) return;

	// Use per-skill VFX override if available
	if (!Config.VFXOverridePath.IsEmpty())
	{
		FVector FinalScale = FVector(Config.Scale);

		if (Config.bIsCascade)
		{
			// Cascade ground effect (Fire Wall uses P_Env_Fire_Grate_01)
			UParticleSystem* CascadeSystem = GetOrLoadCascadeOverride(Config.VFXOverridePath);
			if (CascadeSystem)
			{
				UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
					World, CascadeSystem, Location, FRotator::ZeroRotator, FinalScale, false);
				if (PSC)
				{
					GetCascadeGroundEffects().Add(EffectId, PSC);

					UE_LOG(LogSkillVFX, Log, TEXT("Ground effect created: %s (id=%d) using Cascade override at (%.0f, %.0f, %.0f)"),
						*Type, EffectId, XD, YD, ZD);
				}
			}
		}
		else
		{
			// Niagara ground effect
			UNiagaraSystem* NiagaraSystem = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
			if (NiagaraSystem)
			{
				UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NiagaraSystem, Location, FRotator::ZeroRotator, FinalScale);
				if (Comp)
				{
					SetNiagaraColor(Comp, Config.PrimaryColor);
					ActivePersistentEffects.Add(EffectId, Comp);

					UE_LOG(LogSkillVFX, Log, TEXT("Ground effect created: %s (id=%d) using Niagara override at (%.0f, %.0f, %.0f)"),
						*Type, EffectId, XD, YD, ZD);
				}
			}
		}
		return;
	}

	// Fallback to default NS_GroundPersistent
	if (!NS_GroundPersistent) return;

	FLinearColor Color = SkillVFXDataHelper::GetElementColor(Element);
	UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_GroundPersistent, Location);
	if (Comp)
	{
		SetNiagaraColor(Comp, Color);
		ActivePersistentEffects.Add(EffectId, Comp);

		UE_LOG(LogSkillVFX, Log, TEXT("Ground effect created: %s (id=%d) at (%.0f, %.0f, %.0f)"),
			*Type, EffectId, XD, YD, ZD);
	}
}

void USkillVFXSubsystem::HandleGroundEffectRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EffectIdD = 0;
	FString Type, Reason;
	Obj->TryGetNumberField(TEXT("effectId"), EffectIdD);
	Obj->TryGetStringField(TEXT("type"), Type);
	Obj->TryGetStringField(TEXT("reason"), Reason);

	const int32 EffectId = static_cast<int32>(EffectIdD);

	// Check Niagara tracking
	if (TWeakObjectPtr<UNiagaraComponent>* Found = ActivePersistentEffects.Find(EffectId))
	{
		if (Found->IsValid())
		{
			Found->Get()->DeactivateImmediate();
		}
		ActivePersistentEffects.Remove(EffectId);

		UE_LOG(LogSkillVFX, Log, TEXT("Ground effect removed: %s (id=%d) reason=%s"),
			*Type, EffectId, *Reason);
		return;
	}

	// Check Cascade tracking (Fire Wall)
	auto& CascadeMap = GetCascadeGroundEffects();
	if (TWeakObjectPtr<UParticleSystemComponent>* Found = CascadeMap.Find(EffectId))
	{
		if (Found->IsValid())
		{
			Found->Get()->DeactivateImmediate();
			Found->Get()->DestroyComponent();
		}
		CascadeMap.Remove(EffectId);

		UE_LOG(LogSkillVFX, Log, TEXT("Ground effect removed (Cascade): %s (id=%d) reason=%s"),
			*Type, EffectId, *Reason);
	}
}

// ============================================================
// Combat health update — for First Aid VFX
// ============================================================

void USkillVFXSubsystem::HandleCombatHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess) return;
	if (!bVFXEnabled || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double HealAmountD = 0, CharIdD = 0;
	Obj->TryGetNumberField(TEXT("healAmount"), HealAmountD);
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);

	// Only trigger VFX for actual heals (healAmount > 0)
	if (HealAmountD <= 0) return;

	// Server may omit characterId for self-heals — fall back to local character
	int32 CharId = static_cast<int32>(CharIdD);
	if (CharId == 0) CharId = LocalCharacterId;

	FVector Location = GetActorLocationById(CharId, false);
	if (Location.IsNearlyZero()) return;

	// Use First Aid VFX config (skill 2)
	const FSkillVFXConfig& Config = SkillVFXDataHelper::GetSkillVFXConfig(2);
	UE_LOG(LogSkillVFX, Log, TEXT("HandleCombatHealthUpdate — CharId=%d HealAmt=%.0f Loc=(%.0f,%.0f,%.0f) Template=%d Override=%s Scale=%.1f"),
		CharId, HealAmountD, Location.X, Location.Y, Location.Z, (int32)Config.Template, *Config.VFXOverridePath, Config.Scale);
	if (Config.Template != ESkillVFXTemplate::None)
	{
		SpawnHealFlash(Location, Config);
	}
}

// ============================================================
// VFX spawning — per template type
// ============================================================

void USkillVFXSubsystem::SpawnBoltFromSky(FVector TargetLocation, const FSkillVFXConfig& Config, int32 TotalHits)
{
	UWorld* World = GetWorld();
	if (!World) return;

	TotalHits = FMath::Clamp(TotalHits, 1, 10);

	// Pre-resolve assets once (not per bolt)
	UParticleSystem* CascadeSys = nullptr;
	UNiagaraSystem* NiagaraSys = nullptr;
	const bool bUseCascade = !Config.VFXOverridePath.IsEmpty() && Config.bIsCascade;

	if (bUseCascade)
	{
		CascadeSys = GetOrLoadCascadeOverride(Config.VFXOverridePath);
		if (!CascadeSys) return;
	}
	else
	{
		if (!Config.VFXOverridePath.IsEmpty())
			NiagaraSys = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
		if (!NiagaraSys) NiagaraSys = NS_BoltFromSky;
		if (!NiagaraSys) return;
	}

	// Spawn one bolt per hit, staggered by BoltInterval
	for (int32 i = 0; i < TotalHits; ++i)
	{
		const float SpawnDelay = i * Config.BoltInterval;

		// Capture everything needed by the timer lambda
		TWeakObjectPtr<UWorld> WeakWorld = World;
		FVector CapturedTarget = TargetLocation;
		FSkillVFXConfig CapturedConfig = Config;

		FTimerHandle SpawnTimer;
		World->GetTimerManager().SetTimer(SpawnTimer,
			[this, WeakWorld, CapturedTarget, CapturedConfig, bUseCascade, CascadeSys, NiagaraSys]()
			{
				UWorld* W = WeakWorld.Get();
				if (!W) return;

				// Random XY offset so each bolt is visually distinct
				const float SpawnHeight = FMath::Max(CapturedConfig.BoltSpawnHeight, 100.f);
				const float RandAngle = FMath::FRandRange(0.f, 2.f * PI);
				const float RandDist = FMath::FRandRange(30.f, 80.f);
				const FVector RandomOffset(FMath::Cos(RandAngle) * RandDist, FMath::Sin(RandAngle) * RandDist, 0.f);

				FVector SpawnLoc = CapturedTarget + FVector(0, 0, SpawnHeight) + RandomOffset;
				FRotator SpawnRot = FRotator(-90.f, 0.f, 0.f);
				FVector BoltScale = FVector(CapturedConfig.Scale);

				USceneComponent* MovingComp = nullptr;

				if (bUseCascade)
				{
					UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
						W, CascadeSys, SpawnLoc, SpawnRot, BoltScale, false);
					MovingComp = PSC;
				}
				else
				{
					UNiagaraComponent* NComp = SpawnNiagaraAtLocation(NiagaraSys, SpawnLoc, SpawnRot, BoltScale);
					if (NComp) SetNiagaraColor(NComp, CapturedConfig.PrimaryColor);
					MovingComp = NComp;
				}

				if (!MovingComp) return;

				// Animate bolt moving downward — 0.3s travel time
				const float TravelTime = 0.3f;
				const float TickInterval = 0.016f;
				const int32 TotalSteps = FMath::Max(1, FMath::FloorToInt(TravelTime / TickInterval));

				struct FBoltMoveData
				{
					TWeakObjectPtr<USceneComponent> Comp;
					FVector Start;
					FVector End;
					int32 CurrentStep;
					int32 MaxSteps;
				};

				TSharedPtr<FBoltMoveData> MoveData = MakeShared<FBoltMoveData>();
				MoveData->Comp = MovingComp;
				MoveData->Start = SpawnLoc;
				MoveData->End = CapturedTarget + FVector(0, 0, 50.f);
				MoveData->CurrentStep = 0;
				MoveData->MaxSteps = TotalSteps;

				FTimerHandle MoveTimer;
				W->GetTimerManager().SetTimer(MoveTimer,
					[MoveData]()
					{
						if (!MoveData->Comp.IsValid()) return;

						MoveData->CurrentStep++;
						float Alpha = FMath::Clamp(
							static_cast<float>(MoveData->CurrentStep) / static_cast<float>(MoveData->MaxSteps),
							0.f, 1.f);
						Alpha = Alpha * Alpha; // Ease-in acceleration

						FVector NewPos = FMath::Lerp(MoveData->Start, MoveData->End, Alpha);
						MoveData->Comp->SetWorldLocation(NewPos);
					},
					TickInterval, true, 0.f);

				// After travel ends: stop movement, then destroy after linger time
				// CascadeLifetime doubles as linger time for all bolt types
				const float LingerTime = FMath::Max(CapturedConfig.CascadeLifetime, 0.f);
				TWeakObjectPtr<UWorld> WeakW = W;
				TWeakObjectPtr<USceneComponent> WeakComp = MovingComp;
				FTimerHandle CleanupTimer;
				W->GetTimerManager().SetTimer(CleanupTimer,
					[WeakW, MoveTimer, WeakComp, LingerTime]() mutable
					{
						// Stop movement
						if (WeakW.IsValid())
						{
							WeakW->GetTimerManager().ClearTimer(MoveTimer);
						}
						// If no linger, destroy now
						if (LingerTime <= 0.f)
						{
							if (WeakComp.IsValid())
							{
								if (UNiagaraComponent* NC = Cast<UNiagaraComponent>(WeakComp.Get()))
									NC->DeactivateImmediate();
								else if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(WeakComp.Get()))
								{
									PSC->DeactivateImmediate();
									PSC->DestroyComponent();
								}
							}
						}
						else if (WeakW.IsValid())
						{
							// Destroy after linger
							FTimerHandle LingerTimer;
							WeakW->GetTimerManager().SetTimer(LingerTimer,
								[WeakComp]()
								{
									if (WeakComp.IsValid())
									{
										if (UNiagaraComponent* NC = Cast<UNiagaraComponent>(WeakComp.Get()))
											NC->DeactivateImmediate();
										else if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(WeakComp.Get()))
										{
											PSC->DeactivateImmediate();
											PSC->DestroyComponent();
										}
									}
								},
								LingerTime, false);
						}
					},
					TravelTime, false);
			},
			SpawnDelay, false); // Stagger each bolt spawn
	}
}

void USkillVFXSubsystem::SpawnProjectileEffect(FVector AttackerLocation, FVector TargetLocation, const FSkillVFXConfig& Config)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector SpawnLoc = AttackerLocation + FVector(0, 0, 80.f);
	FVector EndLoc = TargetLocation + FVector(0, 0, 50.f);

	// Calculate rotation to face from attacker toward target
	FVector Direction = (EndLoc - SpawnLoc).GetSafeNormal();
	FRotator SpawnRot = Direction.Rotation();
	FVector ProjScale = FVector(Config.Scale);

	USceneComponent* MovingComp = nullptr;

	if (!Config.VFXOverridePath.IsEmpty())
	{
		if (Config.bIsCascade)
		{
			UParticleSystem* CascadeSys = GetOrLoadCascadeOverride(Config.VFXOverridePath);
			if (!CascadeSys) return;
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
				World, CascadeSys, SpawnLoc, SpawnRot, ProjScale, false);
			MovingComp = PSC;
		}
		else
		{
			UNiagaraSystem* NiagaraSys = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
			if (!NiagaraSys) return;
			UNiagaraComponent* NComp = SpawnNiagaraAtLocation(NiagaraSys, SpawnLoc, SpawnRot, ProjScale);
			if (NComp) SetNiagaraColor(NComp, Config.PrimaryColor);
			MovingComp = NComp;
		}
	}
	else
	{
		if (!NS_Projectile) return;
		UNiagaraComponent* NComp = SpawnNiagaraAtLocation(NS_Projectile, SpawnLoc, SpawnRot, ProjScale);
		if (NComp) SetNiagaraColor(NComp, Config.PrimaryColor);
		MovingComp = NComp;
	}

	if (!MovingComp) return;

	// Animate projectile from caster to target over travel time
	float Distance = FVector::Dist(SpawnLoc, EndLoc);
	float TravelTime = FMath::Clamp(Distance / FMath::Max(Config.ProjectileSpeed, 500.f), 0.1f, 2.0f);
	const float TickInterval = 0.016f;
	const int32 TotalSteps = FMath::Max(1, FMath::FloorToInt(TravelTime / TickInterval));

	struct FProjMoveData
	{
		TWeakObjectPtr<USceneComponent> Comp;
		FVector Start;
		FVector End;
		int32 CurrentStep;
		int32 MaxSteps;
	};

	TSharedPtr<FProjMoveData> MoveData = MakeShared<FProjMoveData>();
	MoveData->Comp = MovingComp;
	MoveData->Start = SpawnLoc;
	MoveData->End = EndLoc;
	MoveData->CurrentStep = 0;
	MoveData->MaxSteps = TotalSteps;

	FTimerHandle MoveTimer;
	World->GetTimerManager().SetTimer(MoveTimer,
		[MoveData]()
		{
			if (!MoveData->Comp.IsValid()) return;
			MoveData->CurrentStep++;
			float Alpha = FMath::Clamp(
				static_cast<float>(MoveData->CurrentStep) / static_cast<float>(MoveData->MaxSteps), 0.f, 1.f);
			FVector NewPos = FMath::Lerp(MoveData->Start, MoveData->End, Alpha);
			MoveData->Comp->SetWorldLocation(NewPos);
		},
		TickInterval, true, 0.f);

	// Cleanup: deactivate projectile when it arrives + spawn impact explosion + clear timer
	TWeakObjectPtr<UWorld> WeakWorld = World;
	TWeakObjectPtr<USceneComponent> WeakMoving = MovingComp;
	FVector CapturedEndLoc = EndLoc;
	FLinearColor CapturedColor = Config.PrimaryColor;
	float CapturedScale = Config.Scale;
	FString CapturedImpactPath = Config.ImpactOverridePath;
	TWeakObjectPtr<USkillVFXSubsystem> WeakThis = this;
	FTimerHandle CleanupTimer;
	World->GetTimerManager().SetTimer(CleanupTimer,
		[WeakWorld, MoveTimer, WeakMoving, CapturedEndLoc, CapturedColor, CapturedScale, CapturedImpactPath, WeakThis]() mutable
		{
			if (WeakWorld.IsValid())
				WeakWorld->GetTimerManager().ClearTimer(MoveTimer);
			// Deactivate the projectile VFX on arrival
			if (WeakMoving.IsValid())
			{
				if (UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(WeakMoving.Get()))
				{
					PSC->DeactivateImmediate();
					PSC->DestroyComponent();
				}
				// Niagara auto-destroys via bAutoDestroy=true
			}
			// Spawn impact explosion at target location
			if (WeakThis.IsValid() && !CapturedImpactPath.IsEmpty())
			{
				UNiagaraSystem* ImpactSys = WeakThis->GetOrLoadNiagaraOverride(CapturedImpactPath);
				if (ImpactSys)
				{
					UNiagaraComponent* ImpactComp = WeakThis->SpawnNiagaraAtLocation(
						ImpactSys, CapturedEndLoc, FRotator::ZeroRotator, FVector(CapturedScale));
					if (ImpactComp)
					{
						WeakThis->SetNiagaraColor(ImpactComp, CapturedColor);
					}
				}
			}
		},
		TravelTime + 0.05f, false);
}

void USkillVFXSubsystem::SpawnMultiHitProjectile(FVector AttackerLocation, FVector TargetLocation, const FSkillVFXConfig& Config, int32 TotalHits)
{
	UWorld* World = GetWorld();
	if (!World) return;

	TotalHits = FMath::Clamp(TotalHits, 1, 10);

	// Pre-resolve asset once
	UNiagaraSystem* NiagaraSys = nullptr;
	if (!Config.VFXOverridePath.IsEmpty() && !Config.bIsCascade)
	{
		NiagaraSys = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
	}
	if (!NiagaraSys) NiagaraSys = NS_Projectile;
	if (!NiagaraSys) return;

	// Spawn one projectile per hit, staggered by 200ms (matching server SS_HIT_DELAY)
	const float HitInterval = 0.2f;

	for (int32 i = 0; i < TotalHits; ++i)
	{
		const float SpawnDelay = i * HitInterval;

		TWeakObjectPtr<UWorld> WeakWorld = World;
		TWeakObjectPtr<USkillVFXSubsystem> WeakThis = this;
		FVector CapturedStart = AttackerLocation;
		FVector CapturedEnd = TargetLocation;
		FSkillVFXConfig CapturedConfig = Config;

		FTimerHandle SpawnTimer;
		World->GetTimerManager().SetTimer(SpawnTimer,
			[WeakWorld, WeakThis, NiagaraSys, CapturedStart, CapturedEnd, CapturedConfig]()
			{
				UWorld* W = WeakWorld.Get();
				if (!W || !WeakThis.IsValid()) return;

				FVector SpawnLoc = CapturedStart + FVector(0, 0, 80.f);
				FVector EndLoc = CapturedEnd + FVector(0, 0, 50.f);

				// Random offset so each projectile is visually distinct
				const float RandAngle = FMath::FRandRange(0.f, 2.f * PI);
				const float RandDist = FMath::FRandRange(20.f, 60.f);
				SpawnLoc += FVector(FMath::Cos(RandAngle) * RandDist, FMath::Sin(RandAngle) * RandDist, 0.f);

				FVector Direction = (EndLoc - SpawnLoc).GetSafeNormal();
				FRotator SpawnRot = Direction.Rotation();
				FVector ProjScale = FVector(CapturedConfig.Scale);

				UNiagaraComponent* NComp = WeakThis->SpawnNiagaraAtLocation(NiagaraSys, SpawnLoc, SpawnRot, ProjScale);
				if (!NComp) return;
				WeakThis->SetNiagaraColor(NComp, CapturedConfig.PrimaryColor);

				// Animate projectile from caster to target
				float Distance = FVector::Dist(SpawnLoc, EndLoc);
				float TravelTime = FMath::Clamp(Distance / FMath::Max(CapturedConfig.ProjectileSpeed, 500.f), 0.1f, 2.0f);
				const float TickInterval = 0.016f;
				const int32 TotalSteps = FMath::Max(1, FMath::FloorToInt(TravelTime / TickInterval));

				struct FProjMoveData
				{
					TWeakObjectPtr<USceneComponent> Comp;
					FVector Start;
					FVector End;
					int32 CurrentStep;
					int32 MaxSteps;
				};

				TSharedPtr<FProjMoveData> MoveData = MakeShared<FProjMoveData>();
				MoveData->Comp = NComp;
				MoveData->Start = SpawnLoc;
				MoveData->End = EndLoc;
				MoveData->CurrentStep = 0;
				MoveData->MaxSteps = TotalSteps;

				FTimerHandle MoveTimer;
				W->GetTimerManager().SetTimer(MoveTimer,
					[MoveData]()
					{
						if (!MoveData->Comp.IsValid()) return;
						MoveData->CurrentStep++;
						float Alpha = FMath::Clamp(
							static_cast<float>(MoveData->CurrentStep) / static_cast<float>(MoveData->MaxSteps), 0.f, 1.f);
						FVector NewPos = FMath::Lerp(MoveData->Start, MoveData->End, Alpha);
						MoveData->Comp->SetWorldLocation(NewPos);
					},
					TickInterval, true, 0.f);

				// Cleanup on arrival
				TWeakObjectPtr<UWorld> WeakW = W;
				TWeakObjectPtr<USceneComponent> WeakComp = NComp;
				FTimerHandle CleanupTimer;
				W->GetTimerManager().SetTimer(CleanupTimer,
					[WeakW, MoveTimer, WeakComp]() mutable
					{
						if (WeakW.IsValid())
							WeakW->GetTimerManager().ClearTimer(MoveTimer);
						// Niagara auto-destroys via bAutoDestroy=true
					},
					TravelTime + 0.05f, false);
			},
			SpawnDelay, false);
	}
}

void USkillVFXSubsystem::SpawnAoEImpact(FVector Location, const FSkillVFXConfig& Config)
{
	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAtLocation(Config, Location);
		return;
	}

	if (!NS_AoEImpact) return;
	UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_AoEImpact, Location);
	if (Comp) SetNiagaraColor(Comp, Config.PrimaryColor);
}

void USkillVFXSubsystem::SpawnGroundPersistent(FVector Location, const FSkillVFXConfig& Config, int32 SkillId)
{
	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAtLocation(Config, Location);
		return;
	}

	if (!NS_GroundPersistent) return;
	UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_GroundPersistent, Location);
	if (Comp) SetNiagaraColor(Comp, Config.PrimaryColor);
}

void USkillVFXSubsystem::SpawnGroundAoERain(FVector Location, const FSkillVFXConfig& Config, int32 HitNumber)
{
	float RandAngle = FMath::FRandRange(0.f, 2.f * PI);
	float RandDist = FMath::FRandRange(0.f, Config.AoERadius);
	FVector Offset(FMath::Cos(RandAngle) * RandDist, FMath::Sin(RandAngle) * RandDist, 0.f);
	FVector StrikeLoc = Location + Offset;

	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAtLocation(Config, StrikeLoc);
		return;
	}

	if (!NS_GroundAoERain) return;
	UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_GroundAoERain, StrikeLoc);
	if (Comp) SetNiagaraColor(Comp, Config.PrimaryColor);
}

void USkillVFXSubsystem::SpawnSelfBuff(AActor* TargetActor, const FSkillVFXConfig& Config, int32 SkillId, int32 TargetId)
{
	if (!TargetActor) return;
	USceneComponent* Root = TargetActor->GetRootComponent();
	if (!Root) return;

	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAttached(Config, Root);
		return;
	}

	if (!NS_SelfBuff) return;
	UNiagaraComponent* Comp = SpawnNiagaraAttached(NS_SelfBuff, Root);
	if (Comp)
	{
		SetNiagaraColor(Comp, Config.PrimaryColor);
		int64 Key = static_cast<int64>(TargetId) * 10000 + SkillId;
		ActiveBuffAuras.Add(Key, Comp);
	}
}

void USkillVFXSubsystem::SpawnTargetDebuff(AActor* TargetActor, const FSkillVFXConfig& Config, int32 SkillId)
{
	if (!TargetActor) return;
	USceneComponent* Root = TargetActor->GetRootComponent();
	if (!Root) return;

	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAttached(Config, Root);
		return;
	}

	if (!NS_TargetDebuff) return;
	UNiagaraComponent* Comp = SpawnNiagaraAttached(NS_TargetDebuff, Root);
	if (Comp) SetNiagaraColor(Comp, Config.PrimaryColor);
}

void USkillVFXSubsystem::SpawnHealFlash(FVector Location, const FSkillVFXConfig& Config)
{
	if (!Config.VFXOverridePath.IsEmpty())
	{
		SpawnVFXAtLocation(Config, Location);
		return;
	}

	if (!NS_HealFlash) return;
	UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NS_HealFlash, Location);
	if (Comp) SetNiagaraColor(Comp, Config.PrimaryColor);
}

// ============================================================
// Casting circle management
// ============================================================

void USkillVFXSubsystem::SpawnCastingCircle(int32 CasterId, FVector Location, const FSkillVFXConfig& Config, float CastDuration)
{
	// Remove existing circle for this caster (if re-casting)
	RemoveCastingCircle(CasterId);

	UWorld* World = GetWorld();
	if (!World) return;

	// Prefer Niagara-based casting circle from Free_Magic pack (looks better, already animated)
	if (NS_CastingCircle)
	{
		UNiagaraComponent* CircleComp = SpawnNiagaraAtLocation(
			NS_CastingCircle, Location, FRotator::ZeroRotator,
			FVector(Config.CastingCircleRadius / 400.f)); // Scale relative to 400 (50% smaller than before)

		if (CircleComp)
		{
			SetNiagaraColor(CircleComp, Config.CastingCircleColor);

			// Auto-deactivate after cast duration (weak pointer prevents dangling reference)
			if (CastDuration > 0.f)
			{
				TWeakObjectPtr<UNiagaraComponent> WeakCircleComp = CircleComp;
				FTimerHandle TempHandle;
				World->GetTimerManager().SetTimer(TempHandle,
					[WeakCircleComp]()
					{
						if (WeakCircleComp.IsValid())
						{
							WeakCircleComp->DeactivateImmediate();
						}
					},
					CastDuration + 0.1f, false);
			}

			// Store a dummy entry so RemoveCastingCircle can clean up
			// (We use the Niagara component map for this)
			int64 NiagaraKey = static_cast<int64>(CasterId) * 10000 + 9999; // Special key for casting circles
			ActiveBuffAuras.Add(NiagaraKey, CircleComp);
		}

		UE_LOG(LogSkillVFX, Verbose, TEXT("Niagara casting circle spawned for caster %d at (%.0f, %.0f, %.0f) — duration %.1fs"),
			CasterId, Location.X, Location.Y, Location.Z, CastDuration);
		return;
	}

	// Fallback: Decal-based casting circle
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACastingCircleActor* Circle = World->SpawnActor<ACastingCircleActor>(
		ACastingCircleActor::StaticClass(), Location, FRotator::ZeroRotator, Params);

	if (Circle)
	{
		if (MI_CastingCircle)
		{
			Circle->BaseMaterial = MI_CastingCircle;
		}

		Circle->Initialize(Config.CastingCircleColor, Config.CastingCircleRadius, CastDuration);
		ActiveCastingCircles.Add(CasterId, Circle);

		UE_LOG(LogSkillVFX, Verbose, TEXT("Decal casting circle spawned for caster %d at (%.0f, %.0f, %.0f) — duration %.1fs"),
			CasterId, Location.X, Location.Y, Location.Z, CastDuration);
	}
}

void USkillVFXSubsystem::RemoveCastingCircle(int32 CasterId)
{
	// Remove Niagara-based circle (if used)
	int64 NiagaraKey = static_cast<int64>(CasterId) * 10000 + 9999;
	if (TWeakObjectPtr<UNiagaraComponent>* NiagaraFound = ActiveBuffAuras.Find(NiagaraKey))
	{
		if (NiagaraFound->IsValid())
		{
			NiagaraFound->Get()->DeactivateImmediate();
		}
		ActiveBuffAuras.Remove(NiagaraKey);
	}

	// Remove decal-based circle (if used)
	if (TWeakObjectPtr<ACastingCircleActor>* Found = ActiveCastingCircles.Find(CasterId))
	{
		if (Found->IsValid())
		{
			Found->Get()->FadeOut(0.3f);
		}
		ActiveCastingCircles.Remove(CasterId);
	}
}

// ============================================================
// Generic Niagara helpers
// ============================================================

UNiagaraComponent* USkillVFXSubsystem::SpawnNiagaraAtLocation(
	UNiagaraSystem* System, FVector Location, FRotator Rotation, FVector Scale)
{
	UWorld* World = GetWorld();
	if (!World || !System) return nullptr;

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, Location, Rotation, Scale,
		true,  // bAutoDestroy
		true,  // bAutoActivate
		ENCPoolMethod::AutoRelease,
		true   // bPreCullCheck
	);
}

UNiagaraComponent* USkillVFXSubsystem::SpawnNiagaraAttached(
	UNiagaraSystem* System, USceneComponent* AttachTo)
{
	if (!System || !AttachTo) return nullptr;

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		System,
		AttachTo,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		FVector::OneVector,
		EAttachLocation::KeepRelativeOffset,
		true,  // bAutoActivate
		ENCPoolMethod::AutoRelease
	);
}

void USkillVFXSubsystem::SetNiagaraColor(UNiagaraComponent* Comp, FLinearColor Color)
{
	if (!Comp) return;
	// Try multiple common parameter names used by third-party Niagara systems
	Comp->SetVariableLinearColor(FName("SpellColor"), Color);
	Comp->SetVariableLinearColor(FName("Color"), Color);
	Comp->SetVariableLinearColor(FName("BaseColor"), Color);
	Comp->SetVariableLinearColor(FName("ParticleColor"), Color);
	Comp->SetVariableLinearColor(FName("User.Color"), Color);
	Comp->SetVariableLinearColor(FName("User.BaseColor"), Color);
	Comp->SetVariableLinearColor(FName("Tint"), Color);
	Comp->SetVariableLinearColor(FName("ColorMultiplier"), Color);
	Comp->SetColorParameter(FName("Color"), Color);
}

void USkillVFXSubsystem::SetNiagaraScale(UNiagaraComponent* Comp, float Scale)
{
	if (!Comp) return;
	// Set common size/radius/scale parameters used by third-party Niagara systems
	// This is needed because world-space Niagara systems ignore component transform scale
	Comp->SetVariableFloat(FName("Scale"), Scale);
	Comp->SetVariableFloat(FName("Size"), Scale);
	Comp->SetVariableFloat(FName("Radius"), Scale * 100.f);  // Some use Radius in UE units
	Comp->SetVariableFloat(FName("SpawnRadius"), Scale * 100.f);
	Comp->SetVariableFloat(FName("SphereRadius"), Scale * 100.f);
	Comp->SetVariableVec3(FName("Scale"), FVector(Scale));
	Comp->SetVariableVec3(FName("User.Scale"), FVector(Scale));
	Comp->SetVariableFloat(FName("User.Scale"), Scale);
	Comp->SetVariableFloat(FName("User.Size"), Scale);
	Comp->SetVariableFloat(FName("ParticleSize"), Scale);
	Comp->SetVariableFloat(FName("SpriteSize"), Scale * 50.f);
}

UNiagaraSystem* USkillVFXSubsystem::GetOrLoadNiagaraOverride(const FString& Path)
{
	if (Path.IsEmpty()) return nullptr;
	if (TObjectPtr<UNiagaraSystem>* Found = NiagaraOverrideCache.Find(Path))
		return *Found;
	UNiagaraSystem* Loaded = LoadObject<UNiagaraSystem>(nullptr, *Path);
	if (!Loaded)
	{
		UE_LOG(LogSkillVFX, Warning, TEXT("FAILED to load Niagara asset: %s — skill will have NO VFX!"), *Path);
	}
	NiagaraOverrideCache.Add(Path, Loaded);
	return Loaded;
}

UParticleSystem* USkillVFXSubsystem::GetOrLoadCascadeOverride(const FString& Path)
{
	if (Path.IsEmpty()) return nullptr;
	if (TObjectPtr<UParticleSystem>* Found = CascadeOverrideCache.Find(Path))
		return *Found;
	UParticleSystem* Loaded = LoadObject<UParticleSystem>(nullptr, *Path);
	if (!Loaded)
	{
		UE_LOG(LogSkillVFX, Warning, TEXT("FAILED to load Cascade asset: %s — skill will have NO VFX!"), *Path);
	}
	CascadeOverrideCache.Add(Path, Loaded);
	return Loaded;
}

void USkillVFXSubsystem::SpawnVFXAtLocation(const FSkillVFXConfig& Config, FVector Location,
	FRotator Rotation, FVector Scale)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Apply per-skill scale from config
	FVector FinalScale = Scale * Config.Scale;

	if (Config.bIsCascade && !Config.VFXOverridePath.IsEmpty())
	{
		UParticleSystem* CascadeSystem = GetOrLoadCascadeOverride(Config.VFXOverridePath);
		if (CascadeSystem)
		{
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
				World, CascadeSystem, Location, Rotation, FinalScale, false); // bAutoDestroy=false — we manage lifetime

			// Cascade effects with looping emitters never auto-destroy.
			// Force cleanup after CascadeLifetime (or default 0.5s for one-shot, skip for persistent/looping).
			if (PSC && !Config.bLooping)
			{
				float Lifetime = (Config.CascadeLifetime > 0.f) ? Config.CascadeLifetime : 0.5f;
				TWeakObjectPtr<UParticleSystemComponent> WeakPSC = PSC;
				FTimerHandle CascadeCleanup;
				World->GetTimerManager().SetTimer(CascadeCleanup,
					[WeakPSC]()
					{
						if (WeakPSC.IsValid())
						{
							WeakPSC->DeactivateImmediate();
							WeakPSC->DestroyComponent();
						}
					},
					Lifetime, false);
			}
		}
	}
	else if (!Config.VFXOverridePath.IsEmpty())
	{
		UNiagaraSystem* NiagaraSystem = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
		if (NiagaraSystem)
		{
			UNiagaraComponent* Comp = SpawnNiagaraAtLocation(NiagaraSystem, Location, Rotation, FinalScale);
			if (Comp)
			{
				SetNiagaraColor(Comp, Config.PrimaryColor);
				SetNiagaraScale(Comp, Config.Scale);
			}
		}
	}
}

void USkillVFXSubsystem::SpawnVFXAttached(const FSkillVFXConfig& Config, USceneComponent* AttachTo)
{
	if (!AttachTo) return;

	FVector FinalScale = FVector(Config.Scale);

	if (Config.bIsCascade && !Config.VFXOverridePath.IsEmpty())
	{
		UParticleSystem* CascadeSystem = GetOrLoadCascadeOverride(Config.VFXOverridePath);
		if (CascadeSystem)
		{
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAttached(
				CascadeSystem, AttachTo, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, FinalScale,
				EAttachLocation::KeepRelativeOffset, false); // bAutoDestroy=false

			if (PSC && !Config.bLooping)
			{
				float Lifetime = (Config.CascadeLifetime > 0.f) ? Config.CascadeLifetime : 0.5f;
				TWeakObjectPtr<UParticleSystemComponent> WeakPSC = PSC;
				UWorld* World = GetWorld();
				if (World)
				{
					FTimerHandle CascadeCleanup;
					World->GetTimerManager().SetTimer(CascadeCleanup,
						[WeakPSC]()
						{
							if (WeakPSC.IsValid())
							{
								WeakPSC->DeactivateImmediate();
								WeakPSC->DestroyComponent();
							}
						},
						Lifetime, false);
				}
			}
		}
	}
	else if (!Config.VFXOverridePath.IsEmpty())
	{
		UNiagaraSystem* NiagaraSystem = GetOrLoadNiagaraOverride(Config.VFXOverridePath);
		if (NiagaraSystem)
		{
			UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				NiagaraSystem, AttachTo, NAME_None,
				FVector::ZeroVector, FRotator::ZeroRotator, FinalScale,
				EAttachLocation::KeepRelativeOffset, true,
				ENCPoolMethod::AutoRelease);
			if (Comp)
			{
				// Apply scale via transform AND Niagara parameters (covers both local/world-space systems)
				Comp->SetRelativeScale3D(FinalScale);
				Comp->SetWorldScale3D(FinalScale);
				SetNiagaraColor(Comp, Config.PrimaryColor);
				SetNiagaraScale(Comp, Config.Scale);
				UE_LOG(LogSkillVFX, Warning, TEXT("SpawnVFXAttached Niagara: Override=%s Scale=%.3f FinalScale=(%.3f,%.3f,%.3f)"),
					*Config.VFXOverridePath, Config.Scale, FinalScale.X, FinalScale.Y, FinalScale.Z);
			}
		}
	}
}

// ============================================================
// Actor lookup helpers
// ============================================================

AActor* USkillVFXSubsystem::FindEnemyActorById(int32 EnemyId) const
{
	// BP_EnemyCharacter stores server ID in a Blueprint variable "EnemyId".
	// Use property reflection to read it (same approach as SkillTreeSubsystem::GetEnemyIdFromActor).
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		// Try reading the "EnemyId" Blueprint variable via reflection
		static const FName PropNames[] = { FName("EnemyId"), FName("EnemyID"), FName("enemyId") };
		for (const FName& PropName : PropNames)
		{
			FProperty* Prop = Actor->GetClass()->FindPropertyByName(PropName);
			if (Prop)
			{
				FIntProperty* IntProp = CastField<FIntProperty>(Prop);
				if (IntProp)
				{
					int32 Value = IntProp->GetPropertyValue_InContainer(Actor);
					if (Value == EnemyId)
					{
						return Actor;
					}
				}
				break; // Found the property name but wrong type or wrong value
			}
		}
	}
	return nullptr;
}

AActor* USkillVFXSubsystem::FindPlayerActorById(int32 PlayerId) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// Local player
	if (PlayerId == LocalCharacterId)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		return PC ? PC->GetPawn() : nullptr;
	}

	// Remote players — search by tag "PlayerId_<N>"
	FString Tag = FString::Printf(TEXT("PlayerId_%d"), PlayerId);
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(FName(*Tag)))
		{
			return *It;
		}
	}
	return nullptr;
}

FVector USkillVFXSubsystem::GetActorLocationById(int32 Id, bool bIsEnemy) const
{
	AActor* Actor = bIsEnemy ? FindEnemyActorById(Id) : FindPlayerActorById(Id);
	if (Actor)
	{
		return Actor->GetActorLocation();
	}
	return FVector::ZeroVector;
}

// ============================================================
// Test spawning
// ============================================================

void USkillVFXSubsystem::SpawnTestEffect(ESkillVFXTemplate Template, FVector Location, FLinearColor Color)
{
	UNiagaraSystem* System = nullptr;
	switch (Template)
	{
	case ESkillVFXTemplate::BoltFromSky:      System = NS_BoltFromSky; break;
	case ESkillVFXTemplate::Projectile:       System = NS_Projectile; break;
	case ESkillVFXTemplate::AoEImpact:        System = NS_AoEImpact; break;
	case ESkillVFXTemplate::GroundPersistent: System = NS_GroundPersistent; break;
	case ESkillVFXTemplate::GroundAoERain:    System = NS_GroundAoERain; break;
	case ESkillVFXTemplate::SelfBuff:         System = NS_SelfBuff; break;
	case ESkillVFXTemplate::TargetDebuff:     System = NS_TargetDebuff; break;
	case ESkillVFXTemplate::HealFlash:        System = NS_HealFlash; break;
	default: break;
	}

	if (System)
	{
		UNiagaraComponent* Comp = SpawnNiagaraAtLocation(System, Location);
		if (Comp)
		{
			SetNiagaraColor(Comp, Color);
		}
		UE_LOG(LogSkillVFX, Log, TEXT("Test effect spawned: Template=%d at (%.0f, %.0f, %.0f)"),
			(int32)Template, Location.X, Location.Y, Location.Z);
	}
	else
	{
		UE_LOG(LogSkillVFX, Warning, TEXT("Test effect FAILED — Niagara system not loaded for template %d"), (int32)Template);
	}
}

UNiagaraComponent* USkillVFXSubsystem::SpawnLoopingPortalEffect(FVector Location)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	// Primary: Use Cascade P_AuraCircle_Ice_Base_01 from Infinity Blade
	UParticleSystem* CascadePortal = GetOrLoadCascadeOverride(
		TEXT("/Game/InfinityBladeEffects/Effects/FX_Skill_Aura/P_AuraCircle_Ice_Base_01.P_AuraCircle_Ice_Base_01"));
	if (CascadePortal)
	{
		FVector PortalScale = FVector(0.5f); // Scale down to match ~200 unit warp portal trigger
		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
			World, CascadePortal, Location, FRotator::ZeroRotator, PortalScale, false);
		UE_LOG(LogSkillVFX, Log, TEXT("Warp portal VFX (Cascade) spawned at (%.0f, %.0f, %.0f)"),
			Location.X, Location.Y, Location.Z);
		// Return nullptr for Niagara — the WarpPortal tracks via SpawnedPortalVFX but
		// we need to handle Cascade cleanup differently. For now the PSC auto-manages.
		return nullptr;
	}

	// Fallback: Niagara casting circle
	UNiagaraSystem* PortalSystem = NS_CastingCircle;
	if (!PortalSystem) PortalSystem = NS_SelfBuff;
	if (!PortalSystem)
	{
		UE_LOG(LogSkillVFX, Warning, TEXT("No VFX system available for warp portal"));
		return nullptr;
	}

	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, PortalSystem, Location, FRotator::ZeroRotator, FVector::OneVector,
		false, true, ENCPoolMethod::None, true);
	if (Comp)
	{
		SetNiagaraColor(Comp, FLinearColor(0.3f, 0.7f, 1.0f, 1.0f)); // Ice-blue tint
		UE_LOG(LogSkillVFX, Log, TEXT("Warp portal VFX (Niagara fallback) spawned at (%.0f, %.0f, %.0f)"),
			Location.X, Location.Y, Location.Z);
	}
	return Comp;
}

