// CombatActionSubsystem.cpp — All server combat response handlers.
// Phase 2 of Blueprint-to-C++ migration. Replaces 432 BP nodes.
// C3 audit fix: BP bridges removed simultaneously in MultiplayerEventSubsystem.

#include "CombatActionSubsystem.h"
#include "EnemySubsystem.h"
#include "Sprite/SpriteCharacterActor.h"
#include "Sprite/SpriteAtlasData.h"
#include "VFX/SkillVFXSubsystem.h"
#include "Audio/AudioSubsystem.h"
#include "OtherPlayerSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "UI/PlayerInputSubsystem.h"
#include "UI/MultiplayerEventSubsystem.h"
#include "UI/ZoneTransitionSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Components/CapsuleComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatAction, Log, All);

namespace
{
	// Resolve a player character ID to its job class string ("swordman", "knight", etc.).
	// Returns empty string for enemies (id >= 2000000) or unknown players.
	// Used by HandleCombatDamage to drive per-class player swing/hit/body sounds.
	FString LookupPlayerJobClass(UWorld* World, int32 CharacterId, int32 LocalCharacterId)
	{
		if (!World || CharacterId <= 0) return FString();
		if (CharacterId >= 2000000) return FString(); // enemy, not a player

		if (CharacterId == LocalCharacterId)
		{
			if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
				return GI->GetSelectedCharacter().JobClass;
			return FString();
		}

		if (UOtherPlayerSubsystem* OPS = World->GetSubsystem<UOtherPlayerSubsystem>())
		{
			if (const FPlayerEntry* E = OPS->GetPlayerData(CharacterId))
				return E->JobClass;
		}
		return FString();
	}

	// Returns true if the player's gender is female. Defaults to male when unknown.
	bool LookupPlayerIsFemale(UWorld* World, int32 CharacterId, int32 LocalCharacterId)
	{
		if (!World || CharacterId <= 0) return false;
		if (CharacterId >= 2000000) return false;

		if (CharacterId == LocalCharacterId)
		{
			if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
			{
				const FString& G = GI->GetSelectedCharacter().Gender;
				return G.Equals(TEXT("F"), ESearchCase::IgnoreCase) || G.Equals(TEXT("female"), ESearchCase::IgnoreCase);
			}
			return false;
		}

		if (UOtherPlayerSubsystem* OPS = World->GetSubsystem<UOtherPlayerSubsystem>())
		{
			if (const FPlayerEntry* E = OPS->GetPlayerData(CharacterId))
			{
				return E->Gender.Equals(TEXT("F"), ESearchCase::IgnoreCase) || E->Gender.Equals(TEXT("female"), ESearchCase::IgnoreCase);
			}
		}
		return false;
	}
}

// ============================================================
// RO Classic colors (matches existing widgets)
// ============================================================

namespace CombatColors
{
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor HPRed        (0.85f, 0.15f, 0.15f, 1.f);
	static const FLinearColor BarBg        (0.10f, 0.07f, 0.04f, 1.f);
	static const FLinearColor TextBright   (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow   (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor DeathBg      (0.00f, 0.00f, 0.00f, 0.70f);
	static const FLinearColor RespawnBtn   (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor RespawnHover (0.65f, 0.50f, 0.22f, 1.f);
}

// ============================================================
// STargetFrameWidget — compact target info panel (Z=9)
// ============================================================

class STargetFrameWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STargetFrameWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UCombatActionSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Sub = InArgs._Subsystem;

		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(200.f)
			[
				// 3-layer frame: Gold -> Dark -> Brown
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CombatColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CombatColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CombatColors::PanelBrown)
						.Padding(FMargin(4.f, 3.f))
						[
							SNew(SVerticalBox)
							// Target name
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									return Sub ? FText::FromString(Sub->TargetFrameName) : FText::GetEmpty();
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FSlateColor(CombatColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(CombatColors::TextShadow)
							]
							// HP bar
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(14.f)
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(CombatColors::PanelDark)
									.Padding(FMargin(1.f))
									[
										SNew(SOverlay)
										+ SOverlay::Slot()
										[
											SNew(SBorder)
											.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
											.BorderBackgroundColor(CombatColors::BarBg)
										]
										+ SOverlay::Slot()
										[
											SNew(SProgressBar)
											.Percent_Lambda([this]() -> TOptional<float> {
												if (!Sub || Sub->TargetFrameMaxHP <= 0.f) return 0.f;
												return FMath::Clamp(Sub->TargetFrameHP / Sub->TargetFrameMaxHP, 0.f, 1.f);
											})
											.FillColorAndOpacity(CombatColors::HPRed)
										]
										+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
										[
											SNew(STextBlock)
											.Text_Lambda([this]() -> FText {
												if (!Sub) return FText::GetEmpty();
												return FText::FromString(FString::Printf(TEXT("%d / %d"),
													(int32)Sub->TargetFrameHP, (int32)Sub->TargetFrameMaxHP));
											})
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
											.ColorAndOpacity(FSlateColor(CombatColors::TextBright))
											.ShadowOffset(FVector2D(1, 1))
											.ShadowColorAndOpacity(CombatColors::TextShadow)
										]
									]
								]
							]
						]
					]
				]
			]
		];
	}

private:
	UCombatActionSubsystem* Sub = nullptr;
};

// ============================================================
// SDeathOverlayWidget — fullscreen death screen (Z=40)
// ============================================================

class SDeathOverlayWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDeathOverlayWidget) {}
		SLATE_EVENT(FSimpleDelegate, OnRespawnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		RespawnDelegate = InArgs._OnRespawnClicked;

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CombatColors::DeathBg)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 20)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("You have been defeated")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
					.ColorAndOpacity(FSlateColor(CombatColors::HPRed))
					.ShadowOffset(FVector2D(2, 2))
					.ShadowColorAndOpacity(CombatColors::TextShadow)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CombatColors::RespawnBtn)
					.Padding(FMargin(20.f, 8.f))
					.Cursor(EMouseCursor::Hand)
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply {
						if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
						{
							if (RespawnDelegate.IsBound())
								RespawnDelegate.Execute();
							return FReply::Handled();
						}
						return FReply::Unhandled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Respawn")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						.ColorAndOpacity(FSlateColor(CombatColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(CombatColors::TextShadow)
					]
				]
			]
		];
	}

private:
	FSimpleDelegate RespawnDelegate;
};

// ============================================================
// Lifecycle
// ============================================================

bool UCombatActionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCombatActionSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	// Resolve local character ID
	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register ALL combat event handlers (C1: this subsystem is the sole owner)
	Router->RegisterHandler(TEXT("combat:damage"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDamage(D); });
	Router->RegisterHandler(TEXT("combat:auto_attack_started"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleAutoAttackStarted(D); });
	Router->RegisterHandler(TEXT("combat:auto_attack_stopped"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleAutoAttackStopped(D); });
	Router->RegisterHandler(TEXT("combat:target_lost"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleTargetLost(D); });
	Router->RegisterHandler(TEXT("combat:out_of_range"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleOutOfRange(D); });
	Router->RegisterHandler(TEXT("combat:death"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatDeath(D); });
	Router->RegisterHandler(TEXT("combat:respawn"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatRespawn(D); });
	Router->RegisterHandler(TEXT("combat:error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleCombatError(D); });
	Router->RegisterHandler(TEXT("combat:health_update"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleHealthUpdate(D); });
	Router->RegisterHandler(TEXT("enemy:health_update"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleEnemyHealthUpdate(D); });

	// Load hit sound assets
	auto TryLoadSound = [](const TCHAR* Path) -> USoundBase*
	{
		return LoadObject<USoundBase>(nullptr, Path);
	};
	if (USoundBase* S1 = TryLoadSound(TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_01.Hit_Normal_01")))
		NormalHitSounds.Add(S1);
	if (USoundBase* S2 = TryLoadSound(TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_02.Hit_Normal_02")))
		NormalHitSounds.Add(S2);
	if (USoundBase* S3 = TryLoadSound(TEXT("/Game/SabriMMO/Audio/HitSounds/Normal/Hit_Normal_03.Hit_Normal_03")))
		NormalHitSounds.Add(S3);
	CritHitSound = TryLoadSound(TEXT("/Game/SabriMMO/Audio/HitSounds/Critical/Hit_Crit_01.Hit_Crit_01"));

	// Defer readiness by one frame (prevents ProcessEvent during PostLoad)
	bReadyToProcess = false;
	InWorld.GetTimerManager().SetTimerForNextTick([this]()
	{
		bReadyToProcess = true;
	});

	// Gate behind socket connection (don't show widgets on login screen)
	if (!GI->IsSocketConnected()) return;

	UE_LOG(LogCombatAction, Log, TEXT("CombatActionSubsystem — 9 combat events registered (charId=%d)"),
		LocalCharacterId);
}

void UCombatActionSubsystem::Deinitialize()
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

	HideTargetFrame();
	HideDeathOverlay();
	bReadyToProcess = false;
	bIsAutoAttacking = false;

	Super::Deinitialize();
}

// ============================================================
// HandleCombatDamage — rotation + attack animation (replaces 215-node BP)
// ============================================================

void UCombatActionSubsystem::HandleCombatDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!bReadyToProcess || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse fields
	double AttackerIdD = 0, TargetIdD = 0;
	Obj->TryGetNumberField(TEXT("attackerId"), AttackerIdD);
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	int32 AttackerId = (int32)AttackerIdD;
	int32 TargetId = (int32)TargetIdD;

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	FString HitType;
	Obj->TryGetStringField(TEXT("hitType"), HitType);

	// Server-supplied attacker weapon type (for player attackers — drives weapon-type
	// swing/hit SFX path; falls back to per-class sound if missing or unmapped).
	FString AttackerWeaponType;
	Obj->TryGetStringField(TEXT("attackerWeaponType"), AttackerWeaponType);

	FString DamageElement;
	Obj->TryGetStringField(TEXT("element"), DamageElement);

	// Update target frame HP from damage events
	if (bTargetFrameVisible)
	{
		double THP = 0, TMH = 0;
		if (Obj->TryGetNumberField(TEXT("targetHealth"), THP))
		{
			Obj->TryGetNumberField(TEXT("targetMaxHealth"), TMH);
			if (TargetId == LockedTargetId)
			{
				TargetFrameHP = (float)THP;
				TargetFrameMaxHP = (float)TMH;
			}
		}
	}

	// Skip animation for misses
	if (HitType == TEXT("miss") || HitType == TEXT("dodge") || HitType == TEXT("perfectDodge"))
		return;

	// Heal events flow through the same handler — track separately so we play the heal
	// sound at the target instead of the hit/body reaction sounds.
	const bool bIsHeal = (HitType == TEXT("heal"));

	// Resolve attacker actor
	AActor* Attacker = nullptr;
	if (AttackerId == LocalCharacterId)
	{
		APlayerController* PC = GetLocalPC();
		Attacker = PC ? PC->GetPawn() : nullptr;
	}
	else if (AttackerId >= 2000000)
	{
		Attacker = FindEnemyActor(AttackerId);
	}
	else
	{
		Attacker = FindPlayerActor(AttackerId);
	}

	// Resolve target actor
	AActor* Target = nullptr;
	if (TargetId == LocalCharacterId)
	{
		APlayerController* PC = GetLocalPC();
		Target = PC ? PC->GetPawn() : nullptr;
	}
	else if (bIsEnemy || TargetId >= 2000000)
	{
		Target = FindEnemyActor(TargetId);
	}
	else
	{
		Target = FindPlayerActor(TargetId);
	}

	// If local player is the attacker: disable orient FIRST, then stop movement
	// Order matters: orient must be off before StopMovement to prevent rotation snap
	if (AttackerId == LocalCharacterId)
	{
		SetOrientToMovement(false);
		StopPawnMovement();
	}

	// Rotate attacker to face target (yaw only)
	if (Attacker && Target)
	{
		FVector Dir = Target->GetActorLocation() - Attacker->GetActorLocation();
		if (!Dir.IsNearlyZero())
		{
			FRotator LookRot = Dir.Rotation();
			Attacker->SetActorRotation(FRotator(0.f, LookRot.Yaw, 0.f));
		}
	}

	// Play attack animation (players only — enemy animations handled by enemy:attack).
	// BP_MMOCharacter's PlayAttackAnimation takes InTargetPosition (FVector) for facing.
	if (Attacker && AttackerId < 2000000)
	{
		FVector TargetPos = Target ? Target->GetActorLocation() : Attacker->GetActorLocation();
		PlayAttackAnimationOnActor(Attacker, TargetPos);
	}

	// ---- Player attacker swing sound ----
	// RO Classic two-tier resolution:
	//   1. Per-weapon-type sound (sword/dagger/axe/bow/etc.) — primary path
	//   2. Per-class unarmed sound (swordman/archer/etc.) — fallback when weapon
	//      type is missing, bare_hand, or has no wav mapping (knuckle/whip/instrument).
	// Skipped for heal events and enemy attackers.
	UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>();
	if (!bIsHeal && Audio && Attacker && AttackerId < 2000000)
	{
		const FVector AtkLoc = Attacker->GetActorLocation();
		const bool bWeaponPlayed = Audio->PlayWeaponAttackSound(AttackerWeaponType, AtkLoc);
		if (!bWeaponPlayed)
		{
			const FString AtkClass = LookupPlayerJobClass(GetWorld(), AttackerId, LocalCharacterId);
			if (!AtkClass.IsEmpty())
				Audio->PlayPlayerAttackSound(AtkClass, AtkLoc);
		}
	}

	// Parse crit flag (used by particles + sound)
	bool bIsCritical = false;
	Obj->TryGetBoolField(TEXT("isCritical"), bIsCritical);

	// ---- Hit / heal / body reaction sounds at target ----
	if (Target && Audio)
	{
		const FVector TargetLoc = Target->GetActorLocation();
		const bool bTargetIsPlayer = !bIsEnemy && TargetId < 2000000 && TargetId > 0;

		if (bIsHeal)
		{
			// Heal: single heal_effect.wav at target location, no hit/body reaction.
			Audio->PlayHealSound(TargetLoc);
		}
		else
		{
			// Hit sound — RO Classic two-tier:
			//   1. Per-weapon-type hit (hit_sword/hit_arrow/hit_axe/etc.) — primary
			//   2. Per-class hit reaction — fallback when weapon type is missing/unmapped
			if (AttackerId > 0 && AttackerId < 2000000)
			{
				const bool bWeaponHitPlayed = Audio->PlayWeaponHitSound(AttackerWeaponType, TargetLoc);
				if (!bWeaponHitPlayed)
				{
					const FString AtkClass = LookupPlayerJobClass(GetWorld(), AttackerId, LocalCharacterId);
					if (!AtkClass.IsEmpty())
						Audio->PlayPlayerHitSound(AtkClass, TargetLoc);
				}
			}

			// Element-typed hit overlay — adds fire/wind/etc. flavor on elemental
			// attacks. Layered ON TOP of the weapon hit, not a replacement. Skipped
			// for plain neutral hits to avoid noise.
			if (!DamageElement.IsEmpty() && DamageElement != TEXT("neutral") && DamageElement != TEXT("normal"))
			{
				Audio->PlayElementHitSound(DamageElement, TargetLoc);
			}

			// Body reaction + voice grunt: target is a player -> play class body
			// material sound (cloth/wood/metal) AND a voice grunt for the third audio
			// layer (per RO Classic, players have 3 layers of damage feedback: weapon
			// hit + body material + voice grunt).
			if (bTargetIsPlayer)
			{
				const FString TgtClass = LookupPlayerJobClass(GetWorld(), TargetId, LocalCharacterId);
				if (!TgtClass.IsEmpty())
				{
					Audio->PlayPlayerBodyReaction(TgtClass, TargetLoc);
					const bool bIsFemale = LookupPlayerIsFemale(GetWorld(), TargetId, LocalCharacterId);
					Audio->PlayPlayerDamageVoice(TgtClass, bIsFemale, TargetLoc);
				}
			}

			// Legacy fallback hit sound for enemy targets (since the monster's own
			// damage sound covers most of it but the original NormalHitSounds add weight).
			// Routed through the SFX volume bus so the Options slider controls it.
			if (bIsEnemy || TargetId >= 2000000)
			{
				USoundBase* HitSound = nullptr;
				if (bIsCritical && CritHitSound)
					HitSound = CritHitSound;
				else if (NormalHitSounds.Num() > 0)
					HitSound = NormalHitSounds[FMath::RandRange(0, NormalHitSounds.Num() - 1)];
				if (HitSound)
				{
					const float SfxVol = Audio ? Audio->GetEffectiveSfxVolume() : 1.0f;
					UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound,
						TargetLoc, SfxVol, FMath::FRandRange(0.95f, 1.05f));
				}
			}
		}
	}

	// Hit flash + flinch on target sprite
	// For enemies, use SpriteActor from EnemyEntry (GetEnemy returns generic AActor)
	ASpriteCharacterActor* TargetSprite = nullptr;
	if (bIsEnemy || TargetId >= 2000000)
	{
		if (UEnemySubsystem* ES = GetWorld()->GetSubsystem<UEnemySubsystem>())
		{
			const FEnemyEntry* Entry = ES->GetEnemyData(TargetId);
			if (Entry && Entry->SpriteActor.IsValid())
				TargetSprite = Entry->SpriteActor.Get();
		}
	}
	else if (Target)
	{
		TargetSprite = Cast<ASpriteCharacterActor>(Target);
		if (!TargetSprite)
		{
			TArray<AActor*> Attached;
			Target->GetAttachedActors(Attached);
			for (AActor* Child : Attached)
			{
				if (ASpriteCharacterActor* S = Cast<ASpriteCharacterActor>(Child))
				{
					TargetSprite = S;
					break;
				}
			}
		}
	}
	if (TargetSprite)
	{
		TargetSprite->PlayHitFlash();
		TargetSprite->SetAnimState(ESpriteAnimState::Hit);

		// Hit impact particles — spawn at sprite visual center (not actor origin,
		// which is underground due to billboard depth offset compensation)
		if (USkillVFXSubsystem* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
		{
			FVector VFXPos = TargetSprite->GetActorLocation();
			VFXPos.Z += TargetSprite->SpriteSize.Y * 0.5f;  // Halfway up the sprite
			VFX->SpawnAutoAttackHitEffect(VFXPos, bIsCritical);
		}
	}
	else if (Target)
	{
		// Non-sprite target (BP actor) — spawn particles at actor location
		if (USkillVFXSubsystem* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
		{
			VFX->SpawnAutoAttackHitEffect(Target->GetActorLocation(), bIsCritical);
		}
	}
}

// ============================================================
// HandleAutoAttackStarted
// ============================================================

void UCombatActionSubsystem::HandleAutoAttackStarted(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetIdD = 0, RangeD = 0, AspdD = 0;
	Obj->TryGetNumberField(TEXT("targetId"), TargetIdD);
	Obj->TryGetNumberField(TEXT("attackRange"), RangeD);
	Obj->TryGetNumberField(TEXT("aspd"), AspdD);

	FString TargetName;
	Obj->TryGetStringField(TEXT("targetName"), TargetName);

	bool bIsEnemy = false;
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);

	LockedTargetId = (int32)TargetIdD;
	AttackRange = (float)RangeD;
	bIsAutoAttacking = true;

	// Player is in range — disable orient FIRST, then stop and rotate
	SetOrientToMovement(false);
	StopPawnMovement();

	AActor* TargetActor = bIsEnemy ? FindEnemyActor(LockedTargetId) : FindPlayerActor(LockedTargetId);
	RotatePawnToward(TargetActor);

	// Show target frame
	TargetFrameName = TargetName;
	bTargetFrameIsEnemy = bIsEnemy;
	ShowTargetFrame();

	UE_LOG(LogCombatAction, Log, TEXT("Auto-attack started on %s (id=%d, range=%.0f, aspd=%.0f)"),
		*TargetName, LockedTargetId, AttackRange, AspdD);
}

// ============================================================
// HandleAutoAttackStopped
// ============================================================

void UCombatActionSubsystem::HandleAutoAttackStopped(const TSharedPtr<FJsonValue>& Data)
{
	FString Reason;
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
			(*ObjPtr)->TryGetStringField(TEXT("reason"), Reason);
	}

	ClearAutoAttackState();
	UE_LOG(LogCombatAction, Log, TEXT("Auto-attack stopped: %s"), *Reason);
}

// ============================================================
// HandleTargetLost
// ============================================================

void UCombatActionSubsystem::HandleTargetLost(const TSharedPtr<FJsonValue>& Data)
{
	FString Reason;
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
			(*ObjPtr)->TryGetStringField(TEXT("reason"), Reason);
	}

	ClearAutoAttackState();
	UE_LOG(LogCombatAction, Log, TEXT("Target lost: %s"), *Reason);
}

// ============================================================
// HandleOutOfRange — move player toward target
// ============================================================

void UCombatActionSubsystem::HandleOutOfRange(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TX = 0, TY = 0, TZ = 0;
	bool bHasPos = Obj->TryGetNumberField(TEXT("targetX"), TX);
	Obj->TryGetNumberField(TEXT("targetY"), TY);
	Obj->TryGetNumberField(TEXT("targetZ"), TZ);

	if (!bHasPos) return;

	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	// Re-enable orient so player faces movement direction while chasing
	SetOrientToMovement(true);

	// Project onto NavMesh then move
	FVector Dest((float)TX, (float)TY, (float)TZ);
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Dest, NavLoc, FVector(500.f, 500.f, 500.f)))
		{
			Dest = NavLoc.Location;
		}
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Dest);
}

// ============================================================
// HandleCombatDeath — show death overlay for local player
// ============================================================

void UCombatActionSubsystem::HandleCombatDeath(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double KilledIdD = 0;
	Obj->TryGetNumberField(TEXT("killedId"), KilledIdD);
	int32 KilledId = (int32)KilledIdD;

	// Play death voice for ANY player death (local + remote), so killing/being killed
	// has the same audible feedback the RO Classic client used. The voice is positional
	// at the player's last known position.
	if (KilledId > 0 && KilledId < 2000000)
	{
		if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
		{
			const FString TgtClass = LookupPlayerJobClass(GetWorld(), KilledId, LocalCharacterId);
			if (!TgtClass.IsEmpty())
			{
				const bool bIsFemale = LookupPlayerIsFemale(GetWorld(), KilledId, LocalCharacterId);
				FVector Loc = FVector::ZeroVector;
				if (KilledId == LocalCharacterId)
				{
					if (APlayerController* PC = GetLocalPC())
						if (APawn* P = PC->GetPawn()) Loc = P->GetActorLocation();
				}
				else if (UOtherPlayerSubsystem* OPS = GetWorld()->GetSubsystem<UOtherPlayerSubsystem>())
				{
					if (AActor* PA = OPS->GetPlayer(KilledId))
						Loc = PA->GetActorLocation();
				}
				Audio->PlayPlayerDeathVoice(TgtClass, bIsFemale, Loc);
			}
		}
	}

	if (KilledId == LocalCharacterId)
	{
		StopPawnMovement();
		ClearAutoAttackState();
		ShowDeathOverlay();

		FString KillerName;
		Obj->TryGetStringField(TEXT("killerName"), KillerName);
		UE_LOG(LogCombatAction, Log, TEXT("Local player killed by %s"), *KillerName);
	}
}

// ============================================================
// HandleCombatRespawn — teleport + ground snap + hide overlay
// ============================================================

void UCombatActionSubsystem::HandleCombatRespawn(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	if (CharId != LocalCharacterId) return;

	HideDeathOverlay();

	double X = 0, Y = 0, Z = 0;
	Obj->TryGetNumberField(TEXT("x"), X);
	Obj->TryGetNumberField(TEXT("y"), Y);
	Obj->TryGetNumberField(TEXT("z"), Z);

	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	// Teleport to respawn position
	FVector SpawnLoc((float)X, (float)Y, (float)Z);

	// Ground snap (same pattern as ZoneTransitionSubsystem)
	float HalfHeight = 96.f;
	if (ACharacter* Char = Cast<ACharacter>(Pawn))
	{
		if (UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	}
	FVector Snapped = UZoneTransitionSubsystem::SnapLocationToGround(
		GetWorld(), SpawnLoc, HalfHeight);
	Pawn->SetActorLocation(Snapped);

	UE_LOG(LogCombatAction, Log, TEXT("Respawned at (%.0f, %.0f, %.0f)"), Snapped.X, Snapped.Y, Snapped.Z);
}

// ============================================================
// HandleCombatError
// ============================================================

void UCombatActionSubsystem::HandleCombatError(const TSharedPtr<FJsonValue>& Data)
{
	FString Message;
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
			(*ObjPtr)->TryGetStringField(TEXT("message"), Message);
	}
	UE_LOG(LogCombatAction, Warning, TEXT("Combat error: %s"), *Message);
}

// ============================================================
// HandleHealthUpdate — update target frame HP if matches locked target
// ============================================================

void UCombatActionSubsystem::HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!bTargetFrameVisible || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double CharIdD = 0;
	Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
	int32 CharId = (int32)CharIdD;

	if (CharId != LockedTargetId) return;

	double HP = 0, MaxHP = 0;
	Obj->TryGetNumberField(TEXT("health"), HP);
	Obj->TryGetNumberField(TEXT("maxHealth"), MaxHP);

	TargetFrameHP = (float)HP;
	TargetFrameMaxHP = (float)MaxHP;
	// Widget auto-updates via TAttribute lambdas
}

// ============================================================
// HandleEnemyHealthUpdate — update target frame for enemy HP changes
// ============================================================

void UCombatActionSubsystem::HandleEnemyHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
	if (!bTargetFrameVisible || !Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double EnemyIdD = 0;
	Obj->TryGetNumberField(TEXT("enemyId"), EnemyIdD);
	int32 EnemyId = (int32)EnemyIdD;

	if (EnemyId != LockedTargetId) return;

	double HP = 0, MaxHP = 0;
	Obj->TryGetNumberField(TEXT("health"), HP);
	Obj->TryGetNumberField(TEXT("maxHealth"), MaxHP);

	TargetFrameHP = (float)HP;
	TargetFrameMaxHP = (float)MaxHP;
}

// ============================================================
// Actor Resolution (Phase 3: direct subsystem lookup)
// ============================================================

AActor* UCombatActionSubsystem::FindEnemyActor(int32 EnemyId)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	if (UEnemySubsystem* Sub = World->GetSubsystem<UEnemySubsystem>())
		return Sub->GetEnemy(EnemyId);
	return nullptr;
}

AActor* UCombatActionSubsystem::FindPlayerActor(int32 CharacterId)
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	if (UOtherPlayerSubsystem* Sub = World->GetSubsystem<UOtherPlayerSubsystem>())
		return Sub->GetPlayer(CharacterId);
	return nullptr;
}

// ============================================================
// Attack Animation
// ============================================================

void UCombatActionSubsystem::PlayAttackAnimationOnActor(AActor* Actor, const FVector& TargetPosition)
{
	if (!Actor) return;

	UFunction* Func = Actor->FindFunction(TEXT("PlayAttackAnimation"));
	if (!Func) return;

	if (Func->ParmsSize > 0)
	{
		// BP_MMOCharacter's PlayAttackAnimation takes InTargetPosition (FVector).
		// It calls FindLookAtRotation + SetActorRotation + Montage Play.
		uint8* Params = (uint8*)FMemory_Alloca(Func->ParmsSize);
		FMemory::Memzero(Params, Func->ParmsSize);

		// Set the InTargetPosition parameter via reflection
		FStructProperty* VecProp = CastField<FStructProperty>(
			Func->FindPropertyByName(TEXT("InTargetPosition")));
		if (VecProp && VecProp->Struct == TBaseStructure<FVector>::Get())
		{
			*VecProp->ContainerPtrToValuePtr<FVector>(Params) = TargetPosition;
		}

		Actor->ProcessEvent(Func, Params);
	}
	else
	{
		// Parameterless function (e.g. BP_EnemyCharacter) — nullptr is safe
		Actor->ProcessEvent(Func, nullptr);
	}
}

// ============================================================
// Target Frame Widget (Z=9)
// ============================================================

void UCombatActionSubsystem::ShowTargetFrame()
{
	if (bTargetFrameVisible) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* Viewport = World->GetGameViewport();
	if (!Viewport) return;

	TSharedRef<STargetFrameWidget> Frame = SNew(STargetFrameWidget).Subsystem(this);
	TargetFrameWidget = Frame;

	TargetFrameWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.Padding(FMargin(0.f, 40.f, 0.f, 0.f))
		[
			Frame
		];

	TargetFrameViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(TargetFrameWrapper);
	Viewport->AddViewportWidgetContent(TargetFrameViewportOverlay.ToSharedRef(), 9);
	bTargetFrameVisible = true;
}

void UCombatActionSubsystem::HideTargetFrame()
{
	if (!bTargetFrameVisible) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (Viewport && TargetFrameViewportOverlay.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(TargetFrameViewportOverlay.ToSharedRef());
		}
	}

	TargetFrameWidget.Reset();
	TargetFrameWrapper.Reset();
	TargetFrameViewportOverlay.Reset();
	bTargetFrameVisible = false;
	TargetFrameName.Empty();
	TargetFrameHP = 0.f;
	TargetFrameMaxHP = 0.f;
}

// ============================================================
// Death Overlay Widget (Z=40)
// ============================================================

void UCombatActionSubsystem::ShowDeathOverlay()
{
	if (bDeathOverlayVisible) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* Viewport = World->GetGameViewport();
	if (!Viewport) return;

	FSimpleDelegate RespawnDel;
	RespawnDel.BindLambda([this]()
	{
		if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
		{
			MES->RequestRespawn();
		}
	});

	TSharedRef<SDeathOverlayWidget> Overlay = SNew(SDeathOverlayWidget).OnRespawnClicked(RespawnDel);
	DeathOverlayWidget = Overlay;

	DeathOverlayWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			Overlay
		];

	DeathOverlayViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(DeathOverlayWrapper);
	Viewport->AddViewportWidgetContent(DeathOverlayViewportOverlay.ToSharedRef(), 40);
	bDeathOverlayVisible = true;
}

void UCombatActionSubsystem::HideDeathOverlay()
{
	if (!bDeathOverlayVisible) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (Viewport && DeathOverlayViewportOverlay.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(DeathOverlayViewportOverlay.ToSharedRef());
		}
	}

	DeathOverlayWidget.Reset();
	DeathOverlayWrapper.Reset();
	DeathOverlayViewportOverlay.Reset();
	bDeathOverlayVisible = false;
}

// ============================================================
// State Helpers
// ============================================================

void UCombatActionSubsystem::ClearAutoAttackState()
{
	bIsAutoAttacking = false;
	LockedTargetId = 0;
	HideTargetFrame();

	// Restore normal movement rotation (disabled during auto-attack)
	SetOrientToMovement(true);

	// Clear PlayerInputSubsystem's walk-to-attack state without re-emitting stop_attack
	if (UWorld* World = GetWorld())
	{
		if (UPlayerInputSubsystem* InputSub = World->GetSubsystem<UPlayerInputSubsystem>())
		{
			InputSub->ClearAttackStateNoEmit();
		}
	}
}

void UCombatActionSubsystem::RestoreOrientToMovement()
{
	SetOrientToMovement(true);
}

void UCombatActionSubsystem::SetOrientToMovement(bool bEnable)
{
	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	ACharacter* Char = Cast<ACharacter>(PC->GetPawn());
	if (!Char) return;

	if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = bEnable;
	}
}

void UCombatActionSubsystem::StopPawnMovement()
{
	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	// Stop pathfinding + velocity. Do NOT call SimpleMoveToLocation(self) here —
	// that generates a zero-direction path which, combined with bOrientRotationToMovement,
	// snaps the character to face the world origin before we can disable orient.
	PC->StopMovement();
	if (UCharacterMovementComponent* CMC = Pawn->FindComponentByClass<UCharacterMovementComponent>())
	{
		CMC->StopMovementImmediately();
	}
}

void UCombatActionSubsystem::RotatePawnToward(AActor* Target)
{
	if (!Target) return;

	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	FVector Dir = Target->GetActorLocation() - Pawn->GetActorLocation();
	if (!Dir.IsNearlyZero())
	{
		FRotator LookRot = Dir.Rotation();
		Pawn->SetActorRotation(FRotator(0.f, LookRot.Yaw, 0.f));
	}
}

APlayerController* UCombatActionSubsystem::GetLocalPC() const
{
	UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}
