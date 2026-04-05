// PlayerInputSubsystem.cpp — Click routing: move, attack, interact.
// Input routed from ASabriMMOCharacter. Uses timer for walk-to polling.

#include "PlayerInputSubsystem.h"
#include "CameraSubsystem.h"
#include "MMOGameInstance.h"
#include "SabriMMOCharacter.h"
#include "ShopNPC.h"
#include "KafraNPC.h"
#include "UI/MultiplayerEventSubsystem.h"
#include "UI/CombatActionSubsystem.h"
#include "UI/SkillTreeSubsystem.h"
#include "UI/EnemySubsystem.h"
#include "UI/OtherPlayerSubsystem.h"
#include "UI/VendingSubsystem.h"
#include "UI/TradeSubsystem.h"
#include "UI/PartySubsystem.h"
#include "UI/ChatSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMOInput, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UPlayerInputSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UPlayerInputSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	// Start a 50ms polling timer for walk-to-attack and walk-to-interact
	InWorld.GetTimerManager().SetTimer(WalkPollTimer, this,
		&UPlayerInputSubsystem::OnWalkPollTick, 0.05f, true);

	UE_LOG(LogMMOInput, Log, TEXT("PlayerInputSubsystem started — walk poll timer active"));
}

void UPlayerInputSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WalkPollTimer);
	}
	Super::Deinitialize();
}

// ============================================================
// Walk poll timer (replaces FTickableGameObject::Tick)
// ============================================================

void UPlayerInputSubsystem::OnWalkPollTick()
{
	// Block walk-to processing while dead
	if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
	{
		if (CAS->IsDead()) return;
	}

	// Block walk-to processing while Play Dead or Hiding (movement locked)
	// Also cancels any in-progress navmesh pathfollowing from click-to-move
	if (UBuffBarSubsystem* BBS = GetWorld()->GetSubsystem<UBuffBarSubsystem>())
	{
		if (BBS->HasBuff(TEXT("play_dead")) || BBS->HasBuff(TEXT("hiding")) || BBS->HasBuff(TEXT("sitting")))
		{
			CancelMovement();
			bWalkingToAttack = false;
			bIsClickMoving = false;
			PendingInteractNPC.Reset();
			return;
		}
	}

	// --- Walk-to-attack ---
	if (bWalkingToAttack)
	{
		// Stop walking if server has confirmed auto-attack is active
		// (CombatActionSubsystem::HandleAutoAttackStarted already stopped movement)
		if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
		{
			if (CAS->IsAutoAttacking())
			{
				bWalkingToAttack = false;
			}
		}

		if (bWalkingToAttack)
		{
			APawn* Pawn = GetLocalPawn();
			AActor* Target = AttackTargetActor.Get();

			if (!Pawn || !Target)
			{
				StopAutoAttack();
			}
			else
			{
				// Use server-known attack range
				float EffectiveRange = 150.f;
				if (UCombatActionSubsystem* CAS2 = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
					EffectiveRange = CAS2->GetAttackRange();

				float Dist = FVector::Dist2D(Pawn->GetActorLocation(), Target->GetActorLocation());
				if (Dist <= EffectiveRange + 30.f)
				{
					// In range — stop walking. Attack was already emitted on click.
					CancelMovement();
					bWalkingToAttack = false;
					UE_LOG(LogMMOInput, Log, TEXT("Walk-to-attack: in range (dist=%.0f, range=%.0f)"), Dist, EffectiveRange);
				}
				else
				{
					// Re-issue move toward target (target may have moved)
					MoveToLocation(Target->GetActorLocation());
				}
			}
		}
	}

	// --- Walk-to-interact ---
	if (AActor* NPC = PendingInteractNPC.Get())
	{
		APawn* Pawn = GetLocalPawn();
		if (!Pawn)
		{
			PendingInteractNPC.Reset();
		}
		else
		{
			float Dist = FVector::Dist2D(Pawn->GetActorLocation(), NPC->GetActorLocation());
			if (Dist <= InteractRange + 30.f)
			{
				CancelMovement();
				LastInteractTime = FPlatformTime::Seconds();

				if (AShopNPC* Shop = Cast<AShopNPC>(NPC))
					Shop->Interact();
				else if (AKafraNPC* Kafra = Cast<AKafraNPC>(NPC))
					Kafra->Interact();

				PendingInteractNPC.Reset();
				UE_LOG(LogMMOInput, Log, TEXT("Walk-to-interact: arrived — interacted with NPC"));
			}
		}
	}
}

// ============================================================
// Public API — called by ASabriMMOCharacter::HandleLeftClick
// ============================================================

void UPlayerInputSubsystem::OnLeftClickFromCharacter(ASabriMMOCharacter* Character)
{
	if (!Character) return;

	// Guard: dead — block all input until respawn
	if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
	{
		if (CAS->IsDead()) return;
	}

	// Guard: Play Dead / Hiding — block all movement, attack, and interact input
	// RO Classic: both states lock the player in place; server also rejects, but
	// blocking here prevents visual desync and unnecessary network traffic
	if (UBuffBarSubsystem* BBS = GetWorld()->GetSubsystem<UBuffBarSubsystem>())
	{
		if (BBS->HasBuff(TEXT("play_dead")) || BBS->HasBuff(TEXT("hiding")) || BBS->HasBuff(TEXT("sitting"))) return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	// Guard: skill targeting owns the click
	if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
	{
		if (SkillSub->IsInTargetingMode())
			return;
	}

	// Guard: camera rotation — ignore left clicks
	if (UCameraSubsystem* CamSub = GetWorld()->GetSubsystem<UCameraSubsystem>())
	{
		if (CamSub->IsRotatingCamera())
			return;
	}

	// Spam guard: 150ms between click processing
	static double LastClickTime = 0.0;
	double Now = FPlatformTime::Seconds();
	if (Now - LastClickTime < 0.15) return;
	LastClickTime = Now;

	// Cursor trace
	FHitResult Hit;
	if (!PC->GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
	{
		return;
	}

	if (!Hit.bBlockingHit)
	{
		UE_LOG(LogMMOInput, Verbose, TEXT("Left click: no blocking hit"));
		return;
	}

	AActor* HitActor = Hit.GetActor();

	// Priority 1: NPCs
	if (HitActor)
	{
		if (Cast<AShopNPC>(HitActor) || Cast<AKafraNPC>(HitActor))
		{
			ProcessClickOnNPC(HitActor);
			return;
		}
	}

	// Priority 2: Vending player shops (click to browse)
	if (HitActor)
	{
		if (UOtherPlayerSubsystem* OPS = GetWorld()->GetSubsystem<UOtherPlayerSubsystem>())
		{
			int32 PlayerId = OPS->GetPlayerIdFromActor(HitActor);
			if (PlayerId > 0)
			{
				const FPlayerEntry* PE = OPS->GetPlayerData(PlayerId);
				if (PE && PE->bIsVending)
				{
					if (UVendingSubsystem* VendSub = GetWorld()->GetSubsystem<UVendingSubsystem>())
					{
						VendSub->BrowseShop(PlayerId);
					}
					return;
				}
			}
		}
	}

	// Priority 3: Enemies
	if (HitActor)
	{
		int32 EnemyId = GetEnemyIdFromActor(HitActor);
		if (EnemyId > 0)
		{
			ProcessClickOnEnemy(HitActor);
			return;
		}
	}

	// Priority 3: Ground
	if (bIsAutoAttacking)
	{
		StopAutoAttack();
	}
	PendingInteractNPC.Reset();
	ProcessClickOnGround(Hit.ImpactPoint);
}

// ============================================================
// Right-click — player context menu (RO Classic)
// ============================================================

void UPlayerInputSubsystem::OnRightClickFromCharacter(ASabriMMOCharacter* Character)
{
	if (!Character) return;

	// Guard: dead
	if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
	{
		if (CAS->IsDead()) return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	// Cursor trace
	FHitResult Hit;
	if (!PC->GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
	{
		return;
	}

	if (!Hit.bBlockingHit) return;

	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	// Only show context menu for other players
	UOtherPlayerSubsystem* OPS = GetWorld()->GetSubsystem<UOtherPlayerSubsystem>();
	if (!OPS) return;

	int32 PlayerId = OPS->GetPlayerIdFromActor(HitActor);
	if (PlayerId <= 0) return;

	// Skip hidden players (Hiding/Cloaking)
	if (OPS->IsPlayerHidden(PlayerId)) return;

	const FPlayerEntry* PE = OPS->GetPlayerData(PlayerId);
	if (!PE) return;

	ShowPlayerContextMenu(PlayerId, PE->PlayerName);
}

void UPlayerInputSubsystem::ShowPlayerContextMenu(int32 CharacterId, const FString& PlayerName)
{
	if (!FSlateApplication::IsInitialized()) return;

	FMenuBuilder MenuBuilder(true, nullptr);

	// Header: player name
	MenuBuilder.BeginSection(NAME_None, FText::FromString(PlayerName));
	MenuBuilder.EndSection();

	// Trade
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Trade")),
		FText::FromString(TEXT("Request a trade")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, CharacterId]()
		{
			if (UTradeSubsystem* TradeSub = GetWorld()->GetSubsystem<UTradeSubsystem>())
				TradeSub->RequestTrade(CharacterId);
		}))
	);

	// Party Invite
	FString CapturedName = PlayerName;
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Party Invite")),
		FText::FromString(TEXT("Invite to party")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, CapturedName]()
		{
			if (UPartySubsystem* PartySub = GetWorld()->GetSubsystem<UPartySubsystem>())
				PartySub->InvitePlayer(CapturedName);
		}))
	);

	// Whisper
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Whisper")),
		FText::FromString(TEXT("Send a private message")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, CapturedName]()
		{
			if (UChatSubsystem* ChatSub = GetWorld()->GetSubsystem<UChatSubsystem>())
				ChatSub->StartWhisperTo(CapturedName);
		}))
	);

	TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelRegularWindow();
	if (!ParentWindow.IsValid()) return;

	FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
	FSlateApplication::Get().PushMenu(
		ParentWindow.ToSharedRef(),
		FWidgetPath(),
		MenuWidget,
		CursorPos,
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);
}

// ============================================================
// Click routing
// ============================================================

void UPlayerInputSubsystem::ProcessClickOnEnemy(AActor* EnemyActor)
{
	int32 EnemyId = GetEnemyIdFromActor(EnemyActor);
	if (EnemyId <= 0) return;

	if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
		SkillSub->CancelWalkToCast();

	PendingInteractNPC.Reset();

	AttackTargetId = EnemyId;
	bAttackTargetIsEnemy = true;
	AttackTargetActor = EnemyActor;
	bIsAutoAttacking = true;

	// Always emit attack immediately — server handles range validation.
	// Server responds with auto_attack_started (sets correct attackRange) or
	// out_of_range (CombatActionSubsystem walks player toward target).
	if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
		MES->EmitCombatAttack(EnemyId, true);

	APawn* Pawn = GetLocalPawn();
	if (!Pawn) return;

	// Use server-known attack range (updated by auto_attack_started events)
	float EffectiveRange = 150.f;
	if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
		EffectiveRange = CAS->GetAttackRange();

	float Dist = FVector::Dist2D(Pawn->GetActorLocation(), EnemyActor->GetActorLocation());
	UE_LOG(LogMMOInput, Log, TEXT("Clicked enemy %d — dist=%.0f range=%.0f"), EnemyId, Dist, EffectiveRange);

	if (Dist <= EffectiveRange + 30.f)
	{
		CancelMovement();
		bWalkingToAttack = false;
		UE_LOG(LogMMOInput, Log, TEXT("In range — emitted combat:attack"));
	}
	else
	{
		bWalkingToAttack = true;
		MoveToLocation(EnemyActor->GetActorLocation());
		UE_LOG(LogMMOInput, Log, TEXT("Out of range — walking to enemy"));
	}
}

void UPlayerInputSubsystem::ProcessClickOnNPC(AActor* NPCActor)
{
	if (bIsAutoAttacking)
		StopAutoAttack();
	bWalkingToAttack = false;

	APawn* Pawn = GetLocalPawn();
	if (!Pawn) return;

	float Dist = FVector::Dist2D(Pawn->GetActorLocation(), NPCActor->GetActorLocation());
	UE_LOG(LogMMOInput, Log, TEXT("Clicked NPC — dist=%.0f range=%.0f"), Dist, InteractRange);

	if (Dist <= InteractRange + 30.f)
	{
		double Now = FPlatformTime::Seconds();
		if (Now - LastInteractTime < 1.0) return;
		LastInteractTime = Now;

		if (AShopNPC* Shop = Cast<AShopNPC>(NPCActor))
			Shop->Interact();
		else if (AKafraNPC* Kafra = Cast<AKafraNPC>(NPCActor))
			Kafra->Interact();
	}
	else
	{
		PendingInteractNPC = NPCActor;
		MoveToLocation(NPCActor->GetActorLocation());
		UE_LOG(LogMMOInput, Log, TEXT("Out of range — walking to NPC"));
	}
}

void UPlayerInputSubsystem::ProcessClickOnGround(const FVector& Location)
{
	bIsClickMoving = true;
	UE_LOG(LogMMOInput, Log, TEXT("Click-to-move: (%.0f, %.0f, %.0f)"), Location.X, Location.Y, Location.Z);
	MoveToLocation(Location); // MoveToLocation projects onto NavMesh internally
}

// ============================================================
// Public API
// ============================================================

void UPlayerInputSubsystem::StopAutoAttack()
{
	if (!bIsAutoAttacking && !bWalkingToAttack) return;

	bIsAutoAttacking = false;
	bWalkingToAttack = false;
	AttackTargetId = 0;
	AttackTargetActor.Reset();

	// Re-enable orient-to-movement immediately so click-to-move faces the walk direction.
	// CombatActionSubsystem disables it during auto-attack; we must restore it here
	// because the server's auto_attack_stopped response arrives after a network round-trip.
	if (UCombatActionSubsystem* CAS = GetWorld()->GetSubsystem<UCombatActionSubsystem>())
	{
		CAS->RestoreOrientToMovement();
	}

	if (UMultiplayerEventSubsystem* MES = GetWorld()->GetSubsystem<UMultiplayerEventSubsystem>())
		MES->EmitStopAttack();

	UE_LOG(LogMMOInput, Log, TEXT("Auto-attack stopped"));
}

void UPlayerInputSubsystem::ClearAttackStateNoEmit()
{
	bIsAutoAttacking = false;
	bWalkingToAttack = false;
	AttackTargetId = 0;
	AttackTargetActor.Reset();
}

// ============================================================
// Movement helpers
// ============================================================

void UPlayerInputSubsystem::MoveToLocation(const FVector& Destination)
{
	APlayerController* PC = GetLocalPC();
	if (!PC)
	{
		UE_LOG(LogMMOInput, Warning, TEXT("MoveToLocation: no PlayerController!"));
		return;
	}

	// Project destination onto NavMesh to ensure it's a valid walkable point.
	// Raw positions from cursor traces or actor locations may be off-NavMesh.
	FVector FinalDest = Destination;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Destination, NavLoc, FVector(500.f, 500.f, 500.f)))
		{
			FinalDest = NavLoc.Location;
		}
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, FinalDest);
}

void UPlayerInputSubsystem::CancelMovement()
{
	APawn* Pawn = GetLocalPawn();
	if (!Pawn) return;

	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Pawn->GetActorLocation());
	PC->StopMovement();
	if (UCharacterMovementComponent* CMC = Pawn->FindComponentByClass<UCharacterMovementComponent>())
		CMC->StopMovementImmediately();
}

void UPlayerInputSubsystem::ForceStopAllMovement()
{
	CancelMovement();
	bWalkingToAttack = false;
	bIsClickMoving = false;
	PendingInteractNPC.Reset();
}

// ============================================================
// Enemy ID extraction
// ============================================================

int32 UPlayerInputSubsystem::GetEnemyIdFromActor(AActor* Actor) const
{
	if (!Actor) return 0;
	UWorld* World = GetWorld();
	if (!World) return 0;
	if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
		return ES->GetEnemyIdFromActor(Actor);
	return 0;
}

// ============================================================
// Helpers
// ============================================================

APawn* UPlayerInputSubsystem::GetLocalPawn() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	APlayerController* PC = World->GetFirstPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

APlayerController* UPlayerInputSubsystem::GetLocalPC() const
{
	UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}
