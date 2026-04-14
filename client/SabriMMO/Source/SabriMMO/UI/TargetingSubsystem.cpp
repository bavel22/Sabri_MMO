// TargetingSubsystem.cpp — 30Hz hover trace, cursor switching, hover indicator.
// Phase 2 of Blueprint-to-C++ migration. Replaces AC_TargetingSystem (59 nodes).

#include "TargetingSubsystem.h"
#include "EnemySubsystem.h"
#include "OtherPlayerSubsystem.h"
#include "GroundItemSubsystem.h"
#include "GroundItemActor.h"
#include "MMOGameInstance.h"
#include "ShopNPC.h"
#include "KafraNPC.h"
#include "UI/SkillTreeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/WidgetComponent.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTargeting, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UTargetingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UTargetingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	// 30Hz hover trace (0.033s interval)
	InWorld.GetTimerManager().SetTimer(HoverTraceTimer, this,
		&UTargetingSubsystem::PerformHoverTrace, 0.033f, true);

	UE_LOG(LogTargeting, Log, TEXT("TargetingSubsystem started — 30Hz hover trace active"));
}

void UTargetingSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoverTraceTimer);

		// Reset cursor
		if (APlayerController* PC = GetLocalPC())
		{
			PC->CurrentMouseCursor = EMouseCursor::Default;
		}

		// Hide any active indicator
		if (HoveredActor.IsValid())
		{
			HideHoverIndicator(HoveredActor.Get());
		}
	}

	HoveredActor.Reset();
	HoveredTargetType = ETargetType::None;
	HoveredTargetId = 0;

	Super::Deinitialize();
}

// ============================================================
// 30Hz Hover Trace
// ============================================================

void UTargetingSubsystem::PerformHoverTrace()
{
	APlayerController* PC = GetLocalPC();
	if (!PC || !PC->GetPawn()) return;

	// H1 audit fix: pause hover detection when skill targeting is active
	if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
	{
		if (SkillSub->IsInTargetingMode())
			return;
	}

	FHitResult Hit;
	if (!PC->GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit))
	{
		// No trace result — clear hover
		if (HoveredActor.IsValid())
		{
			HideHoverIndicator(HoveredActor.Get());
			if (AGroundItemActor* OldGI = Cast<AGroundItemActor>(HoveredActor.Get()))
			{
				OldGI->SetNameVisible(false);
			}
			HoveredActor.Reset();
			HoveredTargetType = ETargetType::None;
			HoveredTargetId = 0;
			UpdateCursor(ETargetType::None);
		}
		return;
	}

	AActor* HitActor = Hit.GetActor();

	// Classify the hit actor
	int32 NewId = 0;
	ETargetType NewType = ClassifyActor(HitActor, NewId);

	// Check if hover target changed
	if (HitActor != HoveredActor.Get())
	{
		// Hide old indicator
		if (HoveredActor.IsValid())
		{
			HideHoverIndicator(HoveredActor.Get());

			// Hide ground item name label on old target
			if (AGroundItemActor* OldGI = Cast<AGroundItemActor>(HoveredActor.Get()))
			{
				OldGI->SetNameVisible(false);
			}
		}

		// Show new indicator
		if (HitActor && NewType != ETargetType::None)
		{
			ShowHoverIndicator(HitActor);
		}

		// Show ground item name label on new target
		if (AGroundItemActor* NewGI = Cast<AGroundItemActor>(HitActor))
		{
			NewGI->SetNameVisible(true);
		}

		HoveredActor = HitActor;
		HoveredTargetType = NewType;
		HoveredTargetId = NewId;
	}

	UpdateCursor(HoveredTargetType);
}

// ============================================================
// Actor Classification (M2 audit fix: property reflection, not name-contains)
// ============================================================

ETargetType UTargetingSubsystem::ClassifyActor(AActor* Actor, int32& OutId) const
{
	OutId = 0;
	if (!Actor) return ETargetType::None;

	// Priority 1: NPCs (C++ classes — direct cast)
	if (Cast<AShopNPC>(Actor) || Cast<AKafraNPC>(Actor))
	{
		return ETargetType::NPC;
	}

	UWorld* World = GetWorld();
	if (!World) return ETargetType::None;

	// Priority 2: Enemies (C++ struct lookup — no property reflection)
	if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
	{
		int32 EnemyId = ES->GetEnemyIdFromActor(Actor);
		if (EnemyId > 0)
		{
			OutId = EnemyId;
			return ETargetType::Enemy;
		}
	}

	// Priority 3: Ground items (C++ struct lookup)
	if (UGroundItemSubsystem* GIS = World->GetSubsystem<UGroundItemSubsystem>())
	{
		int32 GItemId = GIS->GetGroundItemIdFromActor(Actor);
		if (GItemId > 0)
		{
			OutId = GItemId;
			return ETargetType::GroundItem;
		}
	}

	// Priority 4: Other players (C++ struct lookup — fixes targeting bug)
	if (UOtherPlayerSubsystem* PS = World->GetSubsystem<UOtherPlayerSubsystem>())
	{
		int32 PlayerId = PS->GetPlayerIdFromActor(Actor);
		if (PlayerId > 0)
		{
			OutId = PlayerId;
			return ETargetType::Player;
		}
	}

	return ETargetType::None;
}

int32 UTargetingSubsystem::GetEnemyIdFromActor(AActor* Actor)
{
	// Static helper — still available for external callers, delegates to subsystem
	if (!Actor) return 0;
	UWorld* World = Actor->GetWorld();
	if (!World) return 0;
	if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
		return ES->GetEnemyIdFromActor(Actor);
	return 0;
}

// ============================================================
// Cursor Management
// ============================================================

void UTargetingSubsystem::UpdateCursor(ETargetType Type)
{
	APlayerController* PC = GetLocalPC();
	if (!PC) return;

	switch (Type)
	{
	case ETargetType::Enemy:
		PC->CurrentMouseCursor = EMouseCursor::Crosshairs;
		break;
	case ETargetType::NPC:
		PC->CurrentMouseCursor = EMouseCursor::TextEditBeam;
		break;
	case ETargetType::GroundItem:
		PC->CurrentMouseCursor = EMouseCursor::GrabHand;
		break;
	default:
		PC->CurrentMouseCursor = EMouseCursor::Default;
		break;
	}
}

// ============================================================
// Hover Indicator (Phase 2: uses existing WidgetComponent on BP actors)
// ============================================================

void UTargetingSubsystem::ShowHoverIndicator(AActor* Actor)
{
	if (!Actor) return;

	TArray<UWidgetComponent*> Widgets;
	Actor->GetComponents<UWidgetComponent>(Widgets);
	for (UWidgetComponent* W : Widgets)
	{
		if (W->GetName().Contains(TEXT("HoverOver")))
		{
			W->SetVisibility(true);
			return;
		}
	}
}

void UTargetingSubsystem::HideHoverIndicator(AActor* Actor)
{
	if (!Actor) return;

	TArray<UWidgetComponent*> Widgets;
	Actor->GetComponents<UWidgetComponent>(Widgets);
	for (UWidgetComponent* W : Widgets)
	{
		if (W->GetName().Contains(TEXT("HoverOver")))
		{
			W->SetVisibility(false);
			return;
		}
	}
}

// ============================================================
// Helpers
// ============================================================

APlayerController* UTargetingSubsystem::GetLocalPC() const
{
	UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}
