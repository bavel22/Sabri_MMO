// SkillTreeSubsystem.cpp — Implementation of the Skill Tree HUD subsystem

#include "SkillTreeSubsystem.h"
#include "CombatStatsSubsystem.h"
#include "EnemySubsystem.h"
#include "OtherPlayerSubsystem.h"
#include "VendingSubsystem.h"
#include "SSkillTreeWidget.h"
#include "SSkillTargetingOverlay.h"
#include "DrawDebugHelpers.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Texture2D.h"
#include "UObject/UnrealType.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogSkillTree, Log, All);

// ============================================================
// Ground AoE indicator — cpp-local state (no header changes needed)
// Keyed by UWorld* for multiplayer (PIE) safety.
// ============================================================
namespace GroundAoE
{
	// Simple POD state — no destructors, no atexit, fully Live Coding safe
	struct FState
	{
		float AoERadius = 0.f;
		FColor CircleColor = FColor::Green;
		FTimerHandle DrawTimer;
		FTimerHandle DismissTimer;
		FVector LockedPosition = FVector::ZeroVector;
		bool bActive = false;
		bool bLocked = false;  // true = circle locked at cast position, no longer following cursor
	};

	// Heap-allocated map — never freed, avoids atexit destructor crash on Live Coding DLL unload
	static TMap<UWorld*, FState>& GetStateMap()
	{
		static auto* Map = new TMap<UWorld*, FState>();
		return *Map;
	}

	static FState& Get(UWorld* World)
	{
		return GetStateMap().FindOrAdd(World);
	}

	static void Cleanup(UWorld* World)
	{
		GetStateMap().Remove(World);
	}

	// Called every frame — draws circle at cursor (targeting) or locked position (casting)
	static void DrawCircleAtCursor(UWorld* World)
	{
		if (!World) return;
		FState& S = Get(World);
		if (!S.bActive || S.AoERadius <= 0.f) return;

		FVector Center;
		if (S.bLocked)
		{
			// Circle locked at cast position — stays put until spell ends
			Center = S.LockedPosition;
		}
		else
		{
			// Following cursor during targeting
			APlayerController* PC = World->GetFirstPlayerController();
			if (!PC) return;

			FHitResult HitResult;
			if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult) || !HitResult.bBlockingHit)
				return;

			Center = HitResult.ImpactPoint + FVector(0.f, 0.f, 5.f);
		}

		// Draw filled-looking circle: thick outer ring + inner ring
		DrawDebugCircle(World, Center, S.AoERadius, 64, S.CircleColor,
			false, 0.02f, SDPG_World, 4.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

		// Inner ring at 85% radius for a "band" effect
		DrawDebugCircle(World, Center, S.AoERadius * 0.85f, 48, FColor(S.CircleColor.R, S.CircleColor.G, S.CircleColor.B, 120),
			false, 0.02f, SDPG_World, 2.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);

		// Center crosshair
		const float Cross = FMath::Min(S.AoERadius * 0.1f, 30.f);
		DrawDebugLine(World, Center - FVector(Cross, 0, 0), Center + FVector(Cross, 0, 0),
			S.CircleColor, false, 0.02f, SDPG_World, 2.f);
		DrawDebugLine(World, Center - FVector(0, Cross, 0), Center + FVector(0, Cross, 0),
			S.CircleColor, false, 0.02f, SDPG_World, 2.f);
	}

	static void StartDrawing(UWorld* World)
	{
		if (!World) return;
		FState& S = Get(World);
		S.bActive = true;

		World->GetTimerManager().SetTimer(
			S.DrawTimer,
			FTimerDelegate::CreateLambda([World]() { DrawCircleAtCursor(World); }),
			0.016f, true);

		UE_LOG(LogSkillTree, Log, TEXT("Ground AoE circle started: radius=%.0f"), S.AoERadius);
	}

	static void StopDrawing(UWorld* World)
	{
		if (!World) return;
		FState& S = Get(World);
		S.bActive = false;
		S.bLocked = false;
		World->GetTimerManager().ClearTimer(S.DrawTimer);
		World->GetTimerManager().ClearTimer(S.DismissTimer);
	}

	// Lock circle at a position and auto-dismiss after duration (cast time + effect)
	static void LockAndDismissAfter(UWorld* World, FVector Position, float DurationSec)
	{
		if (!World) return;
		FState& S = Get(World);
		if (!S.bActive) return;

		S.bLocked = true;
		S.LockedPosition = Position + FVector(0.f, 0.f, 5.f);

		// Auto-dismiss after the spell finishes
		World->GetTimerManager().SetTimer(
			S.DismissTimer,
			FTimerDelegate::CreateLambda([World]() { StopDrawing(World); }),
			DurationSec, false);

		UE_LOG(LogSkillTree, Log, TEXT("Ground AoE circle locked — dismiss in %.1fs"), DurationSec);
	}

	// Local AoE data — avoids cross-module calls to SkillVFXData (Live Coding safe)
	// EffectDurationSec = how long the spell effect lasts AFTER cast completes
	struct FAoEInfo { float Radius; FColor Color; float EffectDurationSec; };
	static FAoEInfo GetAoEInfo(int32 SkillId, int32 SkillLevel = 1)
	{
		switch (SkillId)
		{
		case 105: return { 300.f, FColor(255, 120,  30), 0.5f };  // Magnum Break — 300 UE units (matches server AOE_RADIUS)
		case 203: return { 300.f, FColor(180, 120, 255), 1.0f };  // Napalm Beat — ghost AoE
		case 207: return { 500.f, FColor(255,  80,  30), 1.0f };  // Fire Ball — fire AoE explosion
		case 209: return {   0.f, FColor(255,  50,  20), 0.f  };  // Fire Wall — line effect, no circle
		case 211: return { 100.f, FColor(255, 255, 255), 10.f };  // Safety Wall — persists 10s
		case 212: {
			// Thunderstorm: N=SkillLevel strikes staggered 300ms client-side + ~0.5s last strike animation
			float EffectSec = FMath::Max(1, SkillLevel - 1) * 0.3f + 0.5f;
			return { 500.f, FColor(180, 220,  80), EffectSec };
		}
		case 304: return { 125.f, FColor(180, 140,  60), 0.f  };  // Arrow Shower — 5x5 cells = 125 UE radius
		case 407: return { 500.f, FColor(255, 255, 200), 0.f  };  // Signum Crucis — holy AoE
		case 608: return { 300.f, FColor(160, 110,  60), 0.f  };  // Cart Revolution — neutral AoE
		case 800: return {   0.f, FColor(100, 180, 255), 0.f  };  // Jupitel Thunder — single target, no circle
		case 801: return { 450.f, FColor(150, 150, 255), 4.0f };  // Lord of Vermilion — 9x9 wind AoE
		case 802: return { 350.f, FColor(255, 100,  30), 3.0f };  // Meteor Storm — 7x7 fire rain
		case 803: return { 350.f, FColor(100, 150, 255), 5.0f };  // Storm Gust — 7x7 water blizzard
		case 805: return { 250.f, FColor(150, 110,  50), 1.0f };  // Heaven's Drive — 5x5 earth AoE
		case 1418: return { 250.f, FColor(150, 110, 50), 1.0f };  // Heaven's Drive Sage
		case 806: return { 250.f, FColor( 80,  70,  30), 5.0f };  // Quagmire — 5x5 debuff zone
		case 808: return { 125.f, FColor(130, 200, 255), 10.f };  // Ice Wall — ice barrier
		case 810: return {  50.f, FColor(255,  80,  20), 30.f };  // Fire Pillar — ground trap
		case 1412: return { 350.f, FColor(255, 100,  30), 60.f };  // Volcano — fire buff zone
		case 1413: return { 350.f, FColor( 80, 130, 230), 60.f };  // Deluge — water buff zone
		case 1414: return { 350.f, FColor(130, 200, 100), 60.f };  // Violent Gale — wind buff zone
		case 1415: return { 350.f, FColor(200, 200, 150), 120.f }; // Land Protector — nullification zone
		case 1303: return { 150.f, FColor(255, 230, 100), 1.0f }; // Grand Cross — cross-shaped holy AoE
		default:  return { 200.f, FColor(100, 200, 255), 2.0f };  // Default
		}
	}
}

// ============================================================
// Walk-to-Cast — cpp-local state (no header changes needed)
// When a targeted skill is clicked out of range, the player
// walks toward the target and auto-casts when close enough.
// Supports both ground-target and single-target (enemy) skills.
// ============================================================
namespace WalkToCast
{
	struct FPending
	{
		int32 SkillId = 0;
		FVector GroundTarget = FVector::ZeroVector;
		float RequiredRange = 0.f;
		FTimerHandle CheckTimer;
		bool bActive = false;
		bool bWaitingForStop = false;  // true = in range, waiting for velocity to reach zero

		// Single-target fields (walk toward enemy, then UseSkillOnTarget)
		bool bIsSingleTarget = false;
		int32 TargetId = 0;
		bool bTargetIsEnemy = false;
		int32 SkillLevel = 0;
		TWeakObjectPtr<AActor> TargetActor;  // track the enemy actor for live position
	};

	static TMap<UWorld*, FPending>& GetMap()
	{
		static auto* Map = new TMap<UWorld*, FPending>();
		return *Map;
	}

	static FPending& Get(UWorld* World) { return GetMap().FindOrAdd(World); }

	static void StopMovement(UWorld* World)
	{
		if (!World) return;
		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) return;
		APawn* Pawn = PC->GetPawn();
		if (!Pawn) return;

		// Stop AI pathfinding
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, Pawn->GetActorLocation());
		// Stop controller-level movement
		PC->StopMovement();
		// Directly zero out velocity on the CharacterMovementComponent
		if (ACharacter* Char = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
			{
				CMC->StopMovementImmediately();
			}
		}
	}

	static void Cancel(UWorld* World)
	{
		if (!World) return;
		FPending& P = Get(World);
		if (!P.bActive) return;
		P.bActive = false;
		P.bWaitingForStop = false;
		P.bIsSingleTarget = false;
		P.SkillId = 0;
		P.TargetId = 0;
		P.TargetActor.Reset();
		World->GetTimerManager().ClearTimer(P.CheckTimer);
		StopMovement(World);
		UE_LOG(LogSkillTree, Log, TEXT("WalkToCast cancelled"));
	}

	static void Cleanup(UWorld* World)
	{
		if (!World) return;
		Cancel(World);
		GetMap().Remove(World);
	}
}

// ============================================================
// Lifecycle
// ============================================================

bool USkillTreeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void USkillTreeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Populate character ID from GameInstance
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
	}

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		// --- Skill events ---
		Router->RegisterHandler(TEXT("skill:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillData(D); });
		Router->RegisterHandler(TEXT("skill:learned"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillLearned(D); });
		Router->RegisterHandler(TEXT("skill:refresh"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillRefresh(D); });
		Router->RegisterHandler(TEXT("skill:reset_complete"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillResetComplete(D); });
		Router->RegisterHandler(TEXT("skill:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillError(D); });
		Router->RegisterHandler(TEXT("skill:used"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillUsed(D); });
		Router->RegisterHandler(TEXT("skill:effect_damage"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillEffectDamage(D); });
		Router->RegisterHandler(TEXT("skill:buff_applied"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillBuffApplied(D); });
		Router->RegisterHandler(TEXT("skill:buff_removed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillBuffRemoved(D); });
		Router->RegisterHandler(TEXT("skill:cooldown_started"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleSkillCooldownStarted(D); });
		Router->RegisterHandler(TEXT("warp_portal:select"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleWarpPortalSelect(D); });

		// --- Events that dismiss the ground AoE circle (cast rejected/failed/interrupted) ---
		auto DismissCircle = [this](const TSharedPtr<FJsonValue>&)
		{
			GroundAoE::StopDrawing(GetWorld());
			WalkToCast::Cancel(GetWorld());
		};
		Router->RegisterHandler(TEXT("combat:out_of_range"), this, DismissCircle);
		Router->RegisterHandler(TEXT("skill:cast_failed"), this, DismissCircle);
		Router->RegisterHandler(TEXT("skill:cast_interrupted"), this, DismissCircle);

		// --- Hotbar data ---
		Router->RegisterHandler(TEXT("hotbar:alldata"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleHotbarAllData(D); });
	}

	UE_LOG(LogSkillTree, Log, TEXT("SkillTreeSubsystem started — all skill socket events registered via EventRouter."));

	// Only emit data requests when socket is connected (game level)
	if (GI->IsSocketConnected())
	{
		// Immediately request skill data so it's cached before the user ever opens the window.
		RequestSkillData();
		UE_LOG(LogSkillTree, Log, TEXT("Eagerly requesting skill:data now that socket is ready."));

		// Request hotbar data after a delay to ensure BP's HUDManager is fully initialized
		InWorld.GetTimerManager().SetTimer(HotbarRequestTimer, FTimerDelegate::CreateLambda([this]()
		{
			if (UWorld* World = GetWorld())
			{
				if (UMMOGameInstance* HGI = Cast<UMMOGameInstance>(World->GetGameInstance()))
				{
					HGI->EmitSocketEvent(TEXT("hotbar:request"), TEXT("{}"));
					UE_LOG(LogSkillTree, Log, TEXT("Emitted hotbar:request (delayed — HUD should be ready)"));
				}
			}
		}), 3.0f, false);
	}
}

void USkillTreeSubsystem::Deinitialize()
{
	HideWidget();

	// Clean up targeting overlay, ground indicator, and walk-to-cast if active
	GroundAoE::StopDrawing(GetWorld());
	GroundAoE::Cleanup(GetWorld());
	WalkToCast::Cleanup(GetWorld());
	bIsInTargetingMode = false;
	HideTargetingOverlay();

	// Clean up skill drag cursor if active
	HideSkillDragCursor();
	bSkillDragging = false;

	// Clear icon caches — release GC-rooted textures and brush memory
	IconBrushCache.Empty();
	IconTextureCache.Empty();
	DynamicIconPaths.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HotbarRequestTimer);

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
// Widget visibility
// ============================================================

void USkillTreeSubsystem::ToggleWidget()
{
	if (bWidgetAdded)
	{
		HideWidget();
	}
	else
	{
		ShowWidget();
	}
}

void USkillTreeSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;

	// Always create the widget immediately — it will show a "Loading..."
	// state if skill data hasn't arrived yet, then rebuild when data comes in.
	ShowWidgetInternal();

	// If skill data is not yet cached, request it now.
	// When HandleSkillData fires, it will call RebuildSkillContent on the existing widget.
	if (SkillGroups.Num() == 0)
	{
		RequestSkillData();
		UE_LOG(LogSkillTree, Log, TEXT("Skill data not cached — widget shown with loading state, requesting data..."));
	}
}

void USkillTreeSubsystem::ShowWidgetInternal()
{
	if (bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	SkillTreeWidget = SNew(SSkillTreeWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			SkillTreeWidget.ToSharedRef()
		];

	ViewportOverlay =
		SNew(SWeakWidget)
		.PossiblyNullContent(AlignmentWrapper);

	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 20);
	bWidgetAdded = true;

	UE_LOG(LogSkillTree, Log, TEXT("SkillTree widget added to viewport (SkillGroups=%d)."), SkillGroups.Num());
}

void USkillTreeSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

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

	SkillTreeWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
}

bool USkillTreeSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded;
}

// ============================================================
// Actions (called from the widget or Blueprint)
// ============================================================

void USkillTreeSubsystem::RequestSkillData()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("RequestSkillData — socket not ready yet, will retry when connected."));
		return;
	}

	GI->EmitSocketEvent(TEXT("skill:data"), TEXT("{}"));
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:data request"));
}

void USkillTreeSubsystem::LearnSkill(int32 SkillId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected()) return;

	// Validate skillId before sending to server
	if (SkillId <= 0)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("LearnSkill called with invalid skillId=%d, aborting"), SkillId);
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillId"), SkillId);

	GI->EmitSocketEvent(TEXT("skill:learn"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:learn for skillId=%d"), SkillId);
}

void USkillTreeSubsystem::ResetAllSkills()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected()) return;

	GI->EmitSocketEvent(TEXT("skill:reset"), TEXT("{}"));
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:reset"));
}

void USkillTreeSubsystem::AssignSkillToHotbar(int32 SkillId, const FString& SkillDisplayName, int32 SlotIndex)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("AssignSkillToHotbar: SocketIO not connected"));
		return;
	}

	if (SlotIndex < 1 || SlotIndex > 9)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("AssignSkillToHotbar: Invalid slot index %d"), SlotIndex);
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	Payload->SetStringField(TEXT("skillName"), SkillDisplayName);

	GI->EmitSocketEvent(TEXT("hotbar:save_skill"), Payload);

	// Update local skill map (SlotIndex is 1-based from UI, convert to 0-based for storage)
	HotbarSkillMap.Add(SlotIndex - 1, SkillId);

	UE_LOG(LogSkillTree, Log, TEXT("AssignSkillToHotbar: skill %s (id=%d) -> slot %d"),
		*SkillDisplayName, SkillId, SlotIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("%s assigned to hotbar slot %d"), *SkillDisplayName, SlotIndex));
	}
}

// ============================================================
// TryUseHotbarSkill — Called by Blueprint UseHotbarSlot.
// Returns true if the slot had a skill and was handled (so BP should skip item logic).
// SlotIndex is 0-based (0 = key 1, 8 = key 9).
// ============================================================

bool USkillTreeSubsystem::TryUseHotbarSkill(int32 SlotIndex)
{
	const int32* FoundSkillId = HotbarSkillMap.Find(SlotIndex);
	if (!FoundSkillId || *FoundSkillId <= 0)
	{
		return false; // Not a skill slot — let Blueprint handle it (item or empty)
	}

	UE_LOG(LogSkillTree, Log, TEXT("TryUseHotbarSkill: slot %d → skill %d"), SlotIndex, *FoundSkillId);
	UseSkill(*FoundSkillId);
	return true;
}

// ============================================================
// HandleHotbarAllData — Parse hotbar:alldata to populate HotbarSkillMap.
// Receives hotbar data from server via EventRouter.
// ============================================================

void USkillTreeSubsystem::HandleHotbarAllData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
	if (!Obj->TryGetArrayField(TEXT("slots"), SlotsArray) || !SlotsArray) return;

	// Clear and rebuild the skill map from server data
	HotbarSkillMap.Empty();

	for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
	{
		const TSharedPtr<FJsonObject>* SlotObj = nullptr;
		if (!SlotVal->TryGetObject(SlotObj) || !SlotObj) continue;
		const TSharedPtr<FJsonObject>& Slot = *SlotObj;

		FString SlotType;
		Slot->TryGetStringField(TEXT("slot_type"), SlotType);

		if (SlotType == TEXT("skill"))
		{
			double SlotIdx = 0, SkillId = 0;
			Slot->TryGetNumberField(TEXT("slot_index"), SlotIdx);
			Slot->TryGetNumberField(TEXT("skill_id"), SkillId);

			int32 Index = (int32)SlotIdx; // Server now stores 0-based slot indices
			if (Index >= 0 && Index < 9 && (int32)SkillId > 0)
			{
				HotbarSkillMap.Add(Index, (int32)SkillId);

				FString SkillName;
				Slot->TryGetStringField(TEXT("skill_name"), SkillName);
				UE_LOG(LogSkillTree, Log, TEXT("HotbarSkillMap: slot %d → skill %s (id=%d)"),
					Index, *SkillName, (int32)SkillId);
			}
		}
	}

	UE_LOG(LogSkillTree, Log, TEXT("HandleHotbarAllData: %d skill slots mapped"), HotbarSkillMap.Num());
}

int32 USkillTreeSubsystem::GetSelectedLevel(int32 SkillId) const
{
	const int32* Level = SelectedUseLevels.Find(SkillId);
	if (Level && *Level > 0) return *Level;
	// Default to max learned level
	const FSkillEntry* Entry = FindSkillEntry(SkillId);
	return Entry ? Entry->CurrentLevel : 1;
}

void USkillTreeSubsystem::SetSelectedLevel(int32 SkillId, int32 Level)
{
	const FSkillEntry* Entry = FindSkillEntry(SkillId);
	if (!Entry || Level < 1) return;
	SelectedUseLevels.Add(SkillId, FMath::Clamp(Level, 1, Entry->CurrentLevel));
}

void USkillTreeSubsystem::UseSkill(int32 SkillId, int32 OverrideLevel)
{
	// Store the level to use for targeting (from hotbar slot or selected level)
	PendingSkillLevel = (OverrideLevel > 0) ? OverrideLevel : GetSelectedLevel(SkillId);

	// Check if this skill requires targeting (RO-style click-to-cast)
	const FSkillEntry* Entry = FindSkillEntry(SkillId);
	if (Entry && Entry->CurrentLevel > 0)
	{
		if (Entry->TargetType == TEXT("single"))
		{
			BeginTargeting(SkillId);
			return;
		}
		if (Entry->TargetType == TEXT("aoe"))
		{
			// Self-centered AoE (range=0): fire immediately, no targeting needed
			// e.g. Frost Nova, Sight Rasher, Magnum Break — centered on caster
			if (Entry->Range <= 0)
			{
				UseSkillOnTarget(SkillId, 0, false, PendingSkillLevel);
				return;
			}
			BeginTargeting(SkillId);
			return;
		}
		if (Entry->TargetType == TEXT("ground"))
		{
			BeginTargeting(SkillId);
			return;
		}
	}

	// Self/none/passive skills execute immediately (e.g. First Aid)
	UseSkillOnTarget(SkillId, 0, false, PendingSkillLevel);
}


void USkillTreeSubsystem::UseSkillOnTarget(int32 SkillId, int32 TargetId, bool bIsEnemy, int32 SkillLevel)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("UseSkill: SocketIO not connected"));
		return;
	}

	if (SkillId <= 0)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("UseSkill: Invalid skillId=%d"), SkillId);
		return;
	}

	// Client-side cooldown check for immediate feedback
	if (IsSkillOnCooldown(SkillId))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("Skill on cooldown"));
		}
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	if (SkillLevel > 0)
		Payload->SetNumberField(TEXT("skillLevel"), SkillLevel);
	if (TargetId > 0)
	{
		Payload->SetNumberField(TEXT("targetId"), TargetId);
		Payload->SetBoolField(TEXT("isEnemy"), bIsEnemy);
	}

	GI->EmitSocketEvent(TEXT("skill:use"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:use for skillId=%d target=%d isEnemy=%d"),
		SkillId, TargetId, bIsEnemy);
}

void USkillTreeSubsystem::UseSkillOnGround(int32 SkillId, FVector GroundPosition, int32 SkillLevel)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	if (!GI || !GI->IsSocketConnected())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("UseSkillOnGround: SocketIO not connected"));
		return;
	}

	if (SkillId <= 0)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("UseSkillOnGround: Invalid skillId=%d"), SkillId);
		return;
	}

	if (IsSkillOnCooldown(SkillId))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("Skill on cooldown"));
		}
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	if (SkillLevel > 0)
		Payload->SetNumberField(TEXT("skillLevel"), SkillLevel);
	Payload->SetNumberField(TEXT("groundX"), GroundPosition.X);
	Payload->SetNumberField(TEXT("groundY"), GroundPosition.Y);
	Payload->SetNumberField(TEXT("groundZ"), GroundPosition.Z);

	GI->EmitSocketEvent(TEXT("skill:use"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:use for skillId=%d Lv%d at ground (%.0f, %.0f, %.0f)"),
		SkillId, SkillLevel, GroundPosition.X, GroundPosition.Y, GroundPosition.Z);
}

bool USkillTreeSubsystem::IsSkillOnCooldown(int32 SkillId) const
{
	const double* ExpiryPtr = SkillCooldownExpiry.Find(SkillId);
	if (!ExpiryPtr) return false;
	return FPlatformTime::Seconds() < *ExpiryPtr;
}

float USkillTreeSubsystem::GetSkillCooldownRemaining(int32 SkillId) const
{
	const double* ExpiryPtr = SkillCooldownExpiry.Find(SkillId);
	if (!ExpiryPtr) return 0.f;
	return FMath::Max(0.f, (float)(*ExpiryPtr - FPlatformTime::Seconds()));
}

// ============================================================
// Icon Utilities
// ============================================================

FString USkillTreeSubsystem::ResolveIconContentPath(const FString& IconName) const
{
	if (IconName.IsEmpty()) return FString();

	// Already a full content path (e.g. from drag-to-hotbar which passes IconPath, not icon name)
	if (IconName.StartsWith(TEXT("/Game/")))
	{
		return IconName;
	}

	// Explicit overrides for server icon names that don't match PNG filenames
	static TMap<FString, FString> NameOverrides;
	if (NameOverrides.Num() == 0)
	{
		NameOverrides.Add(TEXT("backslide"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Thief/back_slide"));
		NameOverrides.Add(TEXT("two_handed_sword_mastery"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/two_hand_sword_mastery"));
		NameOverrides.Add(TEXT("auto_guard"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Crusader/guard"));
		NameOverrides.Add(TEXT("dodge"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Monk/dodge_monk"));
		NameOverrides.Add(TEXT("flasher"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Hunter/flasher_trap"));
		NameOverrides.Add(TEXT("sandman"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Hunter/sandman_trap"));
		NameOverrides.Add(TEXT("siegfried"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Bard/invulnerable_siegfried"));
		NameOverrides.Add(TEXT("nibelungen"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Bard/ring_of_nibelungen"));
		NameOverrides.Add(TEXT("a_poem_of_bragi"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Bard/poem_of_bragi"));
		NameOverrides.Add(TEXT("drum_battlefield"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Bard/drum_on_battlefield"));
		NameOverrides.Add(TEXT("smith_two_handed_sword"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Blacksmith/smith_ths"));
		NameOverrides.Add(TEXT("rest"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Alchemist/rest_homunculus"));
		NameOverrides.Add(TEXT("investigate"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Monk/investigate"));
	}

	if (const FString* Override = NameOverrides.Find(IconName))
	{
		return *Override;
	}

	// Check dynamic map (populated from server skill data in HandleSkillData)
	if (const FString* Found = DynamicIconPaths.Find(IconName))
	{
		return *Found;
	}

	// Fallback: try Novice folder (for skills resolved before skill data arrives)
	return FString::Printf(TEXT("/Game/SabriMMO/Assets/Skill_Icons/Novice/%s"), *IconName);
}

FSlateBrush* USkillTreeSubsystem::GetOrCreateIconBrush(const FString& ContentPath)
{
	if (ContentPath.IsEmpty()) return nullptr;

	if (TSharedPtr<FSlateBrush>* Found = IconBrushCache.Find(ContentPath))
	{
		return Found->Get();
	}

	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *ContentPath);
	if (!Tex)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("Failed to load skill icon: %s"), *ContentPath);
		// Cache a null entry so we don't re-attempt LoadObject every rebuild
		IconBrushCache.Add(ContentPath, nullptr);
		return nullptr;
	}

	// Force UI-quality texture settings so icons stay crisp at small display sizes.
	// Without this, UE5 defaults (DXT compression + mipmap chain + streaming) make
	// 1024px icons look blurry/blocky when rendered at 24-32px in Slate.
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->Filter = TF_Bilinear;          // No trilinear (no mipmaps to blend)
	Tex->NeverStream = true;
	Tex->UpdateResource();

	// CRITICAL: Store the texture in a UPROPERTY TMap so the GC sees it as referenced.
	// Without this, the UTexture2D is only reachable via FSlateBrush (inside a non-UPROPERTY
	// TSharedPtr chain) — invisible to GC — and gets garbage collected → crash during Paint.
	IconTextureCache.Add(ContentPath, Tex);

	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Tex);
	Brush->ImageSize = FVector2D(24.f, 24.f);
	Brush->DrawAs = ESlateBrushDrawType::Image;

	FSlateBrush* RawPtr = Brush.Get();
	IconBrushCache.Add(ContentPath, Brush);

	UE_LOG(LogSkillTree, Log, TEXT("Loaded skill icon: %s"), *ContentPath);
	return RawPtr;
}

// ============================================================
// Event Handlers
// ============================================================

void USkillTreeSubsystem::HandleSkillData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse top-level fields
	Obj->TryGetStringField(TEXT("jobClass"), JobClass);
	double SP = 0;
	Obj->TryGetNumberField(TEXT("skillPoints"), SP);
	SkillPoints = (int32)SP;

	// Parse learnedSkills map
	LearnedSkills.Empty();
	const TSharedPtr<FJsonObject>* LearnedObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("learnedSkills"), LearnedObj) && LearnedObj)
	{
		for (const auto& Pair : (*LearnedObj)->Values)
		{
			int32 SId = FCString::Atoi(*Pair.Key);
			double Lvl = 0;
			Pair.Value->TryGetNumber(Lvl);
			LearnedSkills.Add(SId, (int32)Lvl);
		}
	}

	// Parse skillTree (grouped by classId)
	SkillGroups.Empty();
	const TSharedPtr<FJsonObject>* TreeObj = nullptr;
	if (Obj->TryGetObjectField(TEXT("skillTree"), TreeObj) && TreeObj)
	{
		for (const auto& ClassPair : (*TreeObj)->Values)
		{
			FSkillClassGroup Group;
			Group.ClassId = ClassPair.Key;
			Group.ClassDisplayName = ClassPair.Key; // Will be prettified by widget

			const TArray<TSharedPtr<FJsonValue>>* SkillArray = nullptr;
			if (ClassPair.Value->TryGetArray(SkillArray))
			{
				for (const TSharedPtr<FJsonValue>& SkillVal : *SkillArray)
				{
					const TSharedPtr<FJsonObject>* SkillObj = nullptr;
					if (!SkillVal->TryGetObject(SkillObj) || !SkillObj) continue;
					const TSharedPtr<FJsonObject>& S = *SkillObj;

					FSkillEntry Entry;
					double Tmp = 0;

					S->TryGetNumberField(TEXT("skillId"), Tmp); Entry.SkillId = (int32)Tmp;

					// Validate skillId was parsed correctly
					if (Entry.SkillId <= 0)
					{
						UE_LOG(LogSkillTree, Warning, TEXT("Invalid skillId (%d) in skill data, skipping entry"), Entry.SkillId);
						continue;
					}
					S->TryGetStringField(TEXT("name"), Entry.Name);
					S->TryGetStringField(TEXT("displayName"), Entry.DisplayName);
					Tmp = 0; S->TryGetNumberField(TEXT("maxLevel"), Tmp); Entry.MaxLevel = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("currentLevel"), Tmp); Entry.CurrentLevel = (int32)Tmp;
					S->TryGetStringField(TEXT("type"), Entry.Type);
					S->TryGetStringField(TEXT("targetType"), Entry.TargetType);
					S->TryGetStringField(TEXT("element"), Entry.Element);
					Tmp = 0; S->TryGetNumberField(TEXT("range"), Tmp); Entry.Range = (int32)Tmp;
					S->TryGetStringField(TEXT("description"), Entry.Description);
					S->TryGetStringField(TEXT("icon"), Entry.Icon);
					// Build icon path: /Game/SabriMMO/Assets/Skill_Icons/<ClassFolder>/<icon>
					// Use iconClassId (original class) for the folder, not the tab's class
					{
						FString ClassFolder;
						S->TryGetStringField(TEXT("iconClassId"), ClassFolder);
						if (ClassFolder.IsEmpty()) ClassFolder = Group.ClassId;
						if (ClassFolder.Len() > 0) ClassFolder[0] = FChar::ToUpper(ClassFolder[0]);
						Entry.IconPath = FString::Printf(TEXT("/Game/SabriMMO/Assets/Skill_Icons/%s/%s"), *ClassFolder, *Entry.Icon);
						if (!DynamicIconPaths.Contains(Entry.Icon))
						{
							DynamicIconPaths.Add(Entry.Icon, Entry.IconPath);
						}
					}
					Tmp = 0; S->TryGetNumberField(TEXT("treeRow"), Tmp); Entry.TreeRow = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("treeCol"), Tmp); Entry.TreeCol = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("spCost"), Tmp); Entry.SpCost = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("nextSpCost"), Tmp); Entry.NextSpCost = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("castTime"), Tmp); Entry.CastTime = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("cooldown"), Tmp); Entry.Cooldown = (int32)Tmp;
					Tmp = 0; S->TryGetNumberField(TEXT("effectValue"), Tmp); Entry.EffectValue = (int32)Tmp;
					S->TryGetBoolField(TEXT("canLearn"), Entry.bCanLearn);

					// Parse prerequisites array
					const TArray<TSharedPtr<FJsonValue>>* PrereqArray = nullptr;
					if (S->TryGetArrayField(TEXT("prerequisites"), PrereqArray))
					{
						for (const TSharedPtr<FJsonValue>& PrereqVal : *PrereqArray)
						{
							const TSharedPtr<FJsonObject>* PrereqObj = nullptr;
							if (PrereqVal->TryGetObject(PrereqObj) && PrereqObj)
							{
								double PSkillId = 0, PLevel = 0;
								(*PrereqObj)->TryGetNumberField(TEXT("skillId"), PSkillId);
								(*PrereqObj)->TryGetNumberField(TEXT("level"), PLevel);
								FSkillPrerequisite Prereq;
								Prereq.RequiredSkillId = (int32)PSkillId;
								Prereq.RequiredLevel = (int32)PLevel;
								Entry.Prerequisites.Add(Prereq);
							}
						}
					}

					// Parse per-level data for tooltip
					const TArray<TSharedPtr<FJsonValue>>* LevelsArray = nullptr;
					if (S->TryGetArrayField(TEXT("allLevels"), LevelsArray))
					{
						for (const TSharedPtr<FJsonValue>& LevelVal : *LevelsArray)
						{
							const TSharedPtr<FJsonObject>* LevelObjPtr = nullptr;
							if (!LevelVal->TryGetObject(LevelObjPtr) || !LevelObjPtr) continue;
							const TSharedPtr<FJsonObject>& L = *LevelObjPtr;
							FSkillLevelInfo Info;
							double T = 0;
							T = 0; L->TryGetNumberField(TEXT("level"), T); Info.Level = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("spCost"), T); Info.SpCost = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("castTime"), T); Info.CastTime = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("cooldown"), T); Info.Cooldown = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("effectValue"), T); Info.EffectValue = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("duration"), T); Info.Duration = (int32)T;
							T = 0; L->TryGetNumberField(TEXT("afterCastDelay"), T); Info.AfterCastDelay = (int32)T;
							Entry.AllLevels.Add(Info);
						}
					}

					Group.Skills.Add(Entry);
				}
			}

			SkillGroups.Add(Group);
		}
	}

	UE_LOG(LogSkillTree, Log, TEXT("HandleSkillData — %d class groups, %d skill points, class=%s"),
		SkillGroups.Num(), SkillPoints, *JobClass);

	// Rebuild the widget content if it exists — this is the key path that populates
	// tabs/skills when data arrives after the widget was already shown in "Loading..." state
	if (SkillTreeWidget.IsValid())
	{
		SkillTreeWidget->RebuildSkillContent();
	}
}

void USkillTreeSubsystem::HandleSkillLearned(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double SId = 0, NewLvl = 0, SP = 0;
	Obj->TryGetNumberField(TEXT("skillId"), SId);
	Obj->TryGetNumberField(TEXT("newLevel"), NewLvl);
	Obj->TryGetNumberField(TEXT("skillPoints"), SP);

	FString SkillName;
	Obj->TryGetStringField(TEXT("skillName"), SkillName);

	SkillPoints = (int32)SP;
	LearnedSkills.Add((int32)SId, (int32)NewLvl);

	UE_LOG(LogSkillTree, Log, TEXT("Learned %s Lv%d (%d points remaining)"),
		*SkillName, (int32)NewLvl, SkillPoints);

	// Request full refresh to update canLearn states
	RequestSkillData();
}

void USkillTreeSubsystem::HandleSkillRefresh(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double SP = 0;
	Obj->TryGetNumberField(TEXT("skillPoints"), SP);
	SkillPoints = (int32)SP;

	// Request full skill data to refresh the tree
	RequestSkillData();
}

void USkillTreeSubsystem::HandleSkillResetComplete(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double SP = 0, Refunded = 0;
	Obj->TryGetNumberField(TEXT("skillPoints"), SP);
	Obj->TryGetNumberField(TEXT("refundedPoints"), Refunded);

	SkillPoints = (int32)SP;
	LearnedSkills.Empty();

	UE_LOG(LogSkillTree, Log, TEXT("Skills reset — refunded %d points (total: %d)"),
		(int32)Refunded, SkillPoints);

	// Refresh the full tree
	RequestSkillData();
}

void USkillTreeSubsystem::HandleSkillError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Dismiss ground AoE circle on any skill error
	GroundAoE::StopDrawing(GetWorld());

	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	UE_LOG(LogSkillTree, Warning, TEXT("Skill error: %s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			FString::Printf(TEXT("Skill Error: %s"), *Message));
	}
}

void USkillTreeSubsystem::HandleSkillUsed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString SkillName;
	Obj->TryGetStringField(TEXT("skillName"), SkillName);
	double SkillIdD = 0, Level = 0, SpCost = 0;
	Obj->TryGetNumberField(TEXT("skillId"), SkillIdD);
	Obj->TryGetNumberField(TEXT("level"), Level);
	Obj->TryGetNumberField(TEXT("spCost"), SpCost);
	int32 SkillId = (int32)SkillIdD;
	int32 SkillLevel = (int32)Level;

	UE_LOG(LogSkillTree, Log, TEXT("Skill used: %s (id=%d) Lv%d (SP cost: %d)"),
		*SkillName, SkillId, SkillLevel, (int32)SpCost);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("%s Lv%d used! (-%d SP)"), *SkillName, SkillLevel, (int32)SpCost));
	}

	// Vending skill (605): server sends vending:setup event which triggers the UI.
	// Do NOT open the popup here — it causes a dual-trigger (setup shown twice).
}

void USkillTreeSubsystem::HandleHotbarData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse slots array
	const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
	if (!Obj->TryGetArrayField(TEXT("slots"), SlotsArray) || !SlotsArray)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("HandleHotbarData: No slots array found"));
		return;
	}

	UE_LOG(LogSkillTree, Log, TEXT("HandleHotbarData: Received %d hotbar slots"), SlotsArray->Num());

	// TODO: Store hotbar data in subsystem or pass to HUD manager
	// For now, just log the contents for debugging
	for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
	{
		const TSharedPtr<FJsonObject>* SlotObj = nullptr;
		if (!SlotVal->TryGetObject(SlotObj) || !SlotObj) continue;
		const TSharedPtr<FJsonObject>& Slot = *SlotObj;

		double SlotIndex = 0;
		Slot->TryGetNumberField(TEXT("slot_index"), SlotIndex);

		FString ItemName, SkillName, SlotType;
		Slot->TryGetStringField(TEXT("item_name"), ItemName);
		Slot->TryGetStringField(TEXT("skill_name"), SkillName);
		Slot->TryGetStringField(TEXT("slot_type"), SlotType);

		double SkillId = 0, ItemId = 0, Quantity = 0;
		Slot->TryGetNumberField(TEXT("skill_id"), SkillId);
		Slot->TryGetNumberField(TEXT("item_id"), ItemId);
		Slot->TryGetNumberField(TEXT("quantity"), Quantity);

		UE_LOG(LogSkillTree, Verbose, TEXT("Slot %d: type=%s, item=%s (id=%d, qty=%d), skill=%s (id=%d)"),
			(int32)SlotIndex, *SlotType, *ItemName, (int32)ItemId, (int32)Quantity, *SkillName, (int32)SkillId);
	}
}

// ============================================================
// HandleSkillEffectDamage — skill:effect_damage from server
// Payload: attackerId, attackerName, targetId, targetName, isEnemy,
//          skillId, skillName, skillLevel, element, damage, isCritical,
//          targetHealth, targetMaxHealth, attackerX/Y/Z, targetX/Y/Z, timestamp
// ============================================================
void USkillTreeSubsystem::HandleSkillEffectDamage(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double AttackerId = 0, TargetId = 0, SkillId = 0, Damage = 0, SkillLevel = 0;
	double TargetHealth = 0, TargetMaxHealth = 0;
	bool bIsEnemy = false, bIsCritical = false;
	FString AttackerName, TargetName, SkillName, Element;

	Obj->TryGetNumberField(TEXT("attackerId"), AttackerId);
	Obj->TryGetNumberField(TEXT("targetId"), TargetId);
	Obj->TryGetNumberField(TEXT("skillId"), SkillId);
	Obj->TryGetNumberField(TEXT("damage"), Damage);
	Obj->TryGetNumberField(TEXT("skillLevel"), SkillLevel);
	Obj->TryGetNumberField(TEXT("targetHealth"), TargetHealth);
	Obj->TryGetNumberField(TEXT("targetMaxHealth"), TargetMaxHealth);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetBoolField(TEXT("isCritical"), bIsCritical);
	Obj->TryGetStringField(TEXT("attackerName"), AttackerName);
	Obj->TryGetStringField(TEXT("targetName"), TargetName);
	Obj->TryGetStringField(TEXT("skillName"), SkillName);
	Obj->TryGetStringField(TEXT("element"), Element);

	// Determine if the local player was the attacker or the target
	const bool bIsLocalAttacker = (static_cast<int32>(AttackerId) == LocalCharacterId);
	const bool bIsLocalTarget   = (!bIsEnemy && static_cast<int32>(TargetId) == LocalCharacterId);

	if (bIsLocalAttacker)
	{
		UE_LOG(LogSkillTree, Log,
			TEXT("Skill hit! %s Lv%d dealt %d%s %s damage to %s (%d/%d HP)"),
			*SkillName, (int32)SkillLevel, (int32)Damage,
			bIsCritical ? TEXT(" CRIT") : TEXT(""),
			*Element, *TargetName,
			(int32)TargetHealth, (int32)TargetMaxHealth);
	}
	else if (bIsLocalTarget)
	{
		UE_LOG(LogSkillTree, Log,
			TEXT("Hit by %s's %s Lv%d for %d%s %s damage! HP: %d/%d"),
			*AttackerName, *SkillName, (int32)SkillLevel, (int32)Damage,
			bIsCritical ? TEXT(" CRIT") : TEXT(""),
			*Element,
			(int32)TargetHealth, (int32)TargetMaxHealth);
	}

	// NOTE: combat:damage is also emitted by the server for each skill hit,
	// so the existing Blueprint damage number system will display floating numbers.
	// This handler is for C++-side tracking and future skill-specific VFX.
}

// ============================================================
// HandleSkillBuffApplied — skill:buff_applied from server
// Payload: targetId, targetName, isEnemy, casterId, casterName,
//          skillId, buffName, duration, effects: { ... }
// ============================================================
void USkillTreeSubsystem::HandleSkillBuffApplied(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetId = 0, CasterId = 0, SkillId = 0, Duration = 0;
	bool bIsEnemy = false;
	FString TargetName, CasterName, BuffName;

	Obj->TryGetNumberField(TEXT("targetId"), TargetId);
	Obj->TryGetNumberField(TEXT("casterId"), CasterId);
	Obj->TryGetNumberField(TEXT("skillId"), SkillId);
	Obj->TryGetNumberField(TEXT("duration"), Duration);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetStringField(TEXT("targetName"), TargetName);
	Obj->TryGetStringField(TEXT("casterName"), CasterName);
	Obj->TryGetStringField(TEXT("buffName"), BuffName);

	const int32 DurationMs = static_cast<int32>(Duration);
	const double ExpiresAt = FPlatformTime::Seconds() + (DurationMs / 1000.0);

	// Check if this buff is on the local player (either as target or caster for self-buffs)
	const bool bIsLocalTarget = (!bIsEnemy && static_cast<int32>(TargetId) == LocalCharacterId);

	if (bIsLocalTarget)
	{
		// Remove any existing buff with the same skillId (refresh)
		ActiveBuffs.RemoveAll([SkillIdInt = static_cast<int32>(SkillId)](const FActiveBuff& B)
		{
			return B.SkillId == SkillIdInt;
		});

		FActiveBuff NewBuff;
		NewBuff.SkillId   = static_cast<int32>(SkillId);
		NewBuff.BuffName  = BuffName;
		NewBuff.ExpiresAt = ExpiresAt;
		NewBuff.Duration  = DurationMs;
		ActiveBuffs.Add(NewBuff);

		UE_LOG(LogSkillTree, Log, TEXT("Buff applied: %s (skill %d) for %dms — expires in %.1fs"),
			*BuffName, (int32)SkillId, DurationMs, DurationMs / 1000.0);
	}
	else
	{
		UE_LOG(LogSkillTree, Log, TEXT("Buff applied to %s%s: %s by %s (skill %d, %dms)"),
			bIsEnemy ? TEXT("enemy ") : TEXT(""),
			*TargetName, *BuffName, *CasterName,
			(int32)SkillId, DurationMs);
	}

	// Broadcast update so any listening widgets can refresh
	OnSkillDataUpdated.Broadcast();
}

// ============================================================
// HandleSkillBuffRemoved — skill:buff_removed from server
// Payload: targetId, [isEnemy], buffName, skillId, reason
// ============================================================
void USkillTreeSubsystem::HandleSkillBuffRemoved(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double TargetId = 0, SkillId = 0;
	bool bIsEnemy = false;
	FString BuffName, Reason;

	Obj->TryGetNumberField(TEXT("targetId"), TargetId);
	Obj->TryGetNumberField(TEXT("skillId"), SkillId);
	Obj->TryGetBoolField(TEXT("isEnemy"), bIsEnemy);
	Obj->TryGetStringField(TEXT("buffName"), BuffName);
	Obj->TryGetStringField(TEXT("reason"), Reason);

	const bool bIsLocalTarget = (!bIsEnemy && static_cast<int32>(TargetId) == LocalCharacterId);

	if (bIsLocalTarget)
	{
		const int32 SkillIdInt = static_cast<int32>(SkillId);
		const int32 Removed = ActiveBuffs.RemoveAll([SkillIdInt](const FActiveBuff& B)
		{
			return B.SkillId == SkillIdInt;
		});

		UE_LOG(LogSkillTree, Log, TEXT("Buff removed: %s (skill %d) — reason: %s (removed %d entries)"),
			*BuffName, SkillIdInt, *Reason, Removed);
	}
	else
	{
		UE_LOG(LogSkillTree, Verbose, TEXT("Buff removed from %s%s: %s (skill %d) — %s"),
			bIsEnemy ? TEXT("enemy ") : TEXT(""),
			*FString::FromInt(static_cast<int32>(TargetId)),
			*BuffName, (int32)SkillId, *Reason);
	}

	OnSkillDataUpdated.Broadcast();
}

// ============================================================
// HandleSkillCooldownStarted — skill:cooldown_started from server
// Payload: { skillId, cooldownMs }
// ============================================================
void USkillTreeSubsystem::HandleSkillCooldownStarted(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double SkillId = 0, CooldownMs = 0;
	Obj->TryGetNumberField(TEXT("skillId"), SkillId);
	Obj->TryGetNumberField(TEXT("cooldownMs"), CooldownMs);

	const int32 SkillIdInt = static_cast<int32>(SkillId);
	const double ExpiresAt = FPlatformTime::Seconds() + (CooldownMs / 1000.0);

	SkillCooldownExpiry.Add(SkillIdInt, ExpiresAt);

	UE_LOG(LogSkillTree, Log, TEXT("Cooldown started: skill %d for %.1fs"),
		SkillIdInt, CooldownMs / 1000.0);

	// Broadcast so the skill tree widget can grey out the skill button
	OnSkillDataUpdated.Broadcast();
}

// ============================================================
// HandleWarpPortalSelect — server sends destination choices
// ============================================================

void USkillTreeSubsystem::HandleWarpPortalSelect(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* DestsArray = nullptr;
	if (!Obj->TryGetArrayField(TEXT("destinations"), DestsArray) || !DestsArray) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	// Build destination buttons as a simple Slate popup
	TSharedPtr<SVerticalBox> ButtonList = SNew(SVerticalBox);

	TWeakObjectPtr<USkillTreeSubsystem> WeakThis(this);
	for (const TSharedPtr<FJsonValue>& DestVal : *DestsArray)
	{
		const TSharedPtr<FJsonObject>* DestObj = nullptr;
		if (!DestVal->TryGetObject(DestObj) || !DestObj) continue;

		double IdxD = 0;
		FString Name;
		(*DestObj)->TryGetNumberField(TEXT("index"), IdxD);
		(*DestObj)->TryGetStringField(TEXT("name"), Name);
		const int32 DestIdx = (int32)IdxD;

		ButtonList->AddSlot().AutoHeight().Padding(2.f)
		[
			SNew(SButton)
			.OnClicked_Lambda([WeakThis, DestIdx, VC]() -> FReply {
				if (USkillTreeSubsystem* S = WeakThis.Get())
				{
					// Send confirmation to server
					UMMOGameInstance* GI = Cast<UMMOGameInstance>(S->GetWorld()->GetGameInstance());
					if (GI)
					{
						TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
						Payload->SetNumberField(TEXT("destIndex"), DestIdx);
						GI->EmitSocketEvent(TEXT("warp_portal:confirm"), Payload);
					}
					// Remove popup
					if (S->WarpPortalPopupWrapper.IsValid() && VC)
						VC->RemoveViewportWidgetContent(S->WarpPortalPopupWrapper.ToSharedRef());
					S->WarpPortalPopup.Reset();
					S->WarpPortalPopupWrapper.Reset();
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Name))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
			]
		];
	}

	WarpPortalPopup = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.12f, 0.08f, 0.04f, 0.95f))
		.Padding(FMargin(10.f))
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Select Warp Destination")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.8f, 0.3f)))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				ButtonList.ToSharedRef()
			]
		];

	WarpPortalPopupWrapper = SNew(SWeakWidget).PossiblyNullContent(WarpPortalPopup);
	VC->AddViewportWidgetContent(WarpPortalPopupWrapper.ToSharedRef(), 150);

	UE_LOG(LogSkillTree, Log, TEXT("Warp Portal: showing %d destination choices"), DestsArray->Num());
}

// ============================================================
// FindSkillEntry — look up a skill by ID from cached groups
// ============================================================

const FSkillEntry* USkillTreeSubsystem::FindSkillEntry(int32 SkillId) const
{
	for (const FSkillClassGroup& Group : SkillGroups)
	{
		for (const FSkillEntry& Skill : Group.Skills)
		{
			if (Skill.SkillId == SkillId)
			{
				return &Skill;
			}
		}
	}
	return nullptr;
}

// ============================================================
// Targeting Mode — RO-style click-to-cast for Bash, Magnum Break, etc.
// ============================================================

void USkillTreeSubsystem::BeginTargeting(int32 SkillId)
{
	// Cancel any active walk-to-cast from a previous skill
	WalkToCast::Cancel(GetWorld());

	// If already targeting a different skill, cancel the old one first
	if (bIsInTargetingMode)
	{
		GroundAoE::StopDrawing(GetWorld());
		HideTargetingOverlay();
	}

	// Client-side cooldown check
	if (IsSkillOnCooldown(SkillId))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("Skill on cooldown"));
		}
		return;
	}

	const FSkillEntry* Entry = FindSkillEntry(SkillId);
	if (!Entry)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("BeginTargeting: skill %d not found in cached data"), SkillId);
		return;
	}

	if (Entry->CurrentLevel <= 0)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
				FString::Printf(TEXT("%s not learned"), *Entry->DisplayName));
		}
		return;
	}

	PendingSkillId   = SkillId;
	PendingSkillName = (PendingSkillLevel > 0)
		? FString::Printf(TEXT("%s Lv %d"), *Entry->DisplayName, PendingSkillLevel)
		: Entry->DisplayName;

	if (Entry->TargetType == TEXT("single"))
	{
		PendingTargetingMode = ESkillTargetingMode::SingleTarget;
	}
	else
	{
		PendingTargetingMode = ESkillTargetingMode::GroundTarget;

		// Set AoE radius and color for ground indicator (local lookup, no cross-module calls)
		UWorld* World = GetWorld();
		if (World)
		{
			GroundAoE::FAoEInfo Info = GroundAoE::GetAoEInfo(SkillId);
			GroundAoE::FState& S = GroundAoE::Get(World);
			S.AoERadius = Info.Radius;
			S.CircleColor = Info.Color;
		}
	}

	bIsInTargetingMode = true;
	ShowTargetingOverlay();

	// Start drawing ground AoE circle for ground-targeted skills
	if (PendingTargetingMode == ESkillTargetingMode::GroundTarget)
	{
		GroundAoE::StartDrawing(GetWorld());
	}

	UE_LOG(LogSkillTree, Log, TEXT("Targeting started: %s (mode=%s)"),
		*PendingSkillName,
		PendingTargetingMode == ESkillTargetingMode::SingleTarget ? TEXT("SingleTarget") : TEXT("GroundTarget"));
}

void USkillTreeSubsystem::CancelTargeting()
{
	if (!bIsInTargetingMode) return;

	GroundAoE::StopDrawing(GetWorld());
	WalkToCast::Cancel(GetWorld());

	bIsInTargetingMode   = false;
	PendingSkillId       = 0;
	PendingSkillName.Empty();
	PendingTargetingMode = ESkillTargetingMode::None;

	HideTargetingOverlay();

	UE_LOG(LogSkillTree, Log, TEXT("Targeting cancelled"));
}

// ============================================================
// Targeting Overlay — add/remove the Slate widget from viewport
// ============================================================

void USkillTreeSubsystem::ShowTargetingOverlay()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	FString Hint;
	if (PendingTargetingMode == ESkillTargetingMode::SingleTarget)
	{
		Hint = TEXT("Left-click a target to use — Right-click / ESC to cancel");
	}
	else
	{
		Hint = TEXT("Left-click ground to cast — Right-click / ESC to cancel");
	}

	TargetingOverlay = SNew(SSkillTargetingOverlay)
		.SkillName(PendingSkillName)
		.TargetingHint(Hint)
		.OnLeftClick(FOnTargetingLeftClick::CreateUObject(this, &USkillTreeSubsystem::HandleTargetingClick))
		.OnCancelled(FOnTargetingCancelled::CreateUObject(this, &USkillTreeSubsystem::HandleTargetingCancel));

	TargetingOverlayWrapper =
		SNew(SWeakWidget)
		.PossiblyNullContent(TargetingOverlay);

	// Z-order 100 puts it above all other HUD widgets
	ViewportClient->AddViewportWidgetContent(TargetingOverlayWrapper.ToSharedRef(), 100);

	// Set keyboard focus so ESC key events reach the overlay
	FSlateApplication::Get().SetKeyboardFocus(TargetingOverlay);

	UE_LOG(LogSkillTree, Log, TEXT("Targeting overlay shown"));
}

void USkillTreeSubsystem::HideTargetingOverlay()
{
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (TargetingOverlayWrapper.IsValid())
			{
				VC->RemoveViewportWidgetContent(TargetingOverlayWrapper.ToSharedRef());
			}
		}
	}

	TargetingOverlay.Reset();
	TargetingOverlayWrapper.Reset();
}

// ============================================================
// HandleTargetingClick — left-click during targeting mode
// Line-traces under cursor to find the target, then executes.
// ============================================================

void USkillTreeSubsystem::HandleTargetingClick()
{
	if (!bIsInTargetingMode) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	if (PendingTargetingMode == ESkillTargetingMode::SingleTarget)
	{
		// Line trace under cursor against Visibility channel (sprite enemies use Visibility, not Pawn)
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult) && HitResult.bBlockingHit)
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor)
			{
				const int32 SkillToUse = PendingSkillId;
				const FSkillEntry* Entry = FindSkillEntry(SkillToUse);
				const float SkillRange = Entry ? static_cast<float>(Entry->Range) : 0.f;
				APawn* Pawn = PC->GetPawn();
				const float RangeTolerance = 30.f;

				// Check if clicked an enemy
				int32 EnemyId = GetEnemyIdFromActor(HitActor);
				// Check if clicked another player
				int32 PlayerId = 0;
				if (EnemyId <= 0)
				{
					if (UOtherPlayerSubsystem* OPS = World->GetSubsystem<UOtherPlayerSubsystem>())
						PlayerId = OPS->GetPlayerIdFromActor(HitActor);
				}
				// Check if clicked self (own pawn)
				bool bClickedSelf = (EnemyId <= 0 && PlayerId <= 0 && HitActor == Pawn);

				if (EnemyId > 0 || PlayerId > 0)
				{
					const int32 TargetId = (EnemyId > 0) ? EnemyId : PlayerId;
					const bool bIsTargetEnemy = (EnemyId > 0);
					const float Dist2D = Pawn ? FVector::Dist2D(Pawn->GetActorLocation(), HitActor->GetActorLocation()) : 0.f;

					if (SkillRange > 0.f && Pawn && Dist2D > SkillRange + RangeTolerance)
					{
						// Out of range — walk toward target and auto-cast when close enough
						WalkToCast::FPending& P = WalkToCast::Get(World);
						P.SkillId = SkillToUse;
						P.GroundTarget = HitActor->GetActorLocation();
						P.RequiredRange = SkillRange;
						P.bActive = true;
						P.bWaitingForStop = false;
						P.bIsSingleTarget = true;
						P.TargetId = TargetId;
						P.SkillLevel = PendingSkillLevel;
						P.bTargetIsEnemy = bIsTargetEnemy;
						P.TargetActor = HitActor;

						bIsInTargetingMode = false;
						PendingSkillId = 0;
						PendingSkillName.Empty();
						PendingTargetingMode = ESkillTargetingMode::None;
						HideTargetingOverlay();

						UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, HitActor->GetActorLocation());

						TWeakObjectPtr<USkillTreeSubsystem> WeakThis(this);
						World->GetTimerManager().SetTimer(
							P.CheckTimer,
							FTimerDelegate::CreateLambda([WeakThis, World]()
							{
								if (!WeakThis.IsValid() || !World) return;
								WalkToCast::FPending& WP = WalkToCast::Get(World);
								if (!WP.bActive || !WP.bIsSingleTarget) return;

								APlayerController* WPC = World->GetFirstPlayerController();
								APawn* WPawn = WPC ? WPC->GetPawn() : nullptr;
								if (!WPawn)
								{
									WalkToCast::Cancel(World);
									return;
								}

								FVector TargetPos = WP.GroundTarget;
								if (WP.TargetActor.IsValid())
								{
									TargetPos = WP.TargetActor->GetActorLocation();
									WP.GroundTarget = TargetPos;
									if (!WP.bWaitingForStop)
										UAIBlueprintHelperLibrary::SimpleMoveToLocation(WPC, TargetPos);
								}

								const float WDist = FVector::Dist2D(WPawn->GetActorLocation(), TargetPos);

								if (!WP.bWaitingForStop)
								{
									if (WDist <= WP.RequiredRange + 30.f)
									{
										WP.bWaitingForStop = true;
										WalkToCast::StopMovement(World);
										UE_LOG(LogSkillTree, Log, TEXT("WalkToCast(single): reached range (%.0f), stopping"), WDist);
									}
								}
								else
								{
									const float Speed = WPawn->GetVelocity().Size();
									if (Speed < 5.f)
									{
										const int32 CapturedSkillId = WP.SkillId;
										const int32 CapturedTargetId = WP.TargetId;
										const bool CapturedIsEnemy = WP.bTargetIsEnemy;
										const int32 CapturedLevel = WP.SkillLevel;
										WP.bActive = false;
										WP.bWaitingForStop = false;
										WP.bIsSingleTarget = false;
										WP.TargetActor.Reset();
										World->GetTimerManager().ClearTimer(WP.CheckTimer);

										WeakThis->UseSkillOnTarget(CapturedSkillId, CapturedTargetId, CapturedIsEnemy, CapturedLevel);
										UE_LOG(LogSkillTree, Log, TEXT("WalkToCast(single): stopped, casting skill %d on target %d"),
											CapturedSkillId, CapturedTargetId);
									}
								}
							}),
							0.05f, true);

						UE_LOG(LogSkillTree, Log, TEXT("WalkToCast(single): skill %d on %s %d out of range (%.0f > %.0f), walking"),
							SkillToUse, bIsTargetEnemy ? TEXT("enemy") : TEXT("player"), TargetId, Dist2D, SkillRange);
					}
					else
					{
						// In range — fire immediately
						CancelTargeting();
						UseSkillOnTarget(SkillToUse, TargetId, bIsTargetEnemy, PendingSkillLevel);
						UE_LOG(LogSkillTree, Log, TEXT("Targeting executed: skill %d Lv%d on %s %d"), SkillToUse, PendingSkillLevel, bIsTargetEnemy ? TEXT("enemy") : TEXT("player"), TargetId);
					}
					return;
				}

				// Clicked own pawn → self-cast
				if (bClickedSelf)
				{
					CancelTargeting();
					UseSkillOnTarget(SkillToUse, 0, false, PendingSkillLevel);
					UE_LOG(LogSkillTree, Log, TEXT("Targeting executed: skill %d Lv%d on self"), SkillToUse, PendingSkillLevel);
					return;
				}
			}
		}

		// No valid target under cursor — self-cast (supportive skills like Heal default to self)
		{
			const int32 SkillToUse = PendingSkillId;
			const int32 LevelToUse = PendingSkillLevel;
			CancelTargeting();
			UseSkillOnTarget(SkillToUse, 0, false, LevelToUse);
			UE_LOG(LogSkillTree, Log, TEXT("Targeting: no target under cursor, self-casting skill %d Lv%d"), SkillToUse, LevelToUse);
		}
	}
	else if (PendingTargetingMode == ESkillTargetingMode::GroundTarget)
	{
		// Ground targeting: line-trace under cursor to get world position
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult) && HitResult.bBlockingHit)
		{
			const FVector GroundPos = HitResult.ImpactPoint;
			const int32 SkillToUse = PendingSkillId;
			const FSkillEntry* Entry = FindSkillEntry(SkillToUse);
			const float SkillRange = Entry ? static_cast<float>(Entry->Range) : 0.f;

			// Check if player is in range (0 range = unlimited)
			APawn* Pawn = PC->GetPawn();
			const float Dist2D = Pawn ? FVector::Dist2D(Pawn->GetActorLocation(), GroundPos) : 0.f;
			const float RangeTolerance = 30.f;

			if (SkillRange > 0.f && Pawn && Dist2D > SkillRange + RangeTolerance)
			{
				// Out of range — walk toward target and auto-cast when close enough
				WalkToCast::FPending& P = WalkToCast::Get(World);
				P.SkillId = SkillToUse;
				P.GroundTarget = GroundPos;
				P.RequiredRange = SkillRange;
				P.SkillLevel = PendingSkillLevel;
				P.bActive = true;

				// Clear targeting UI but keep ground circle following the locked position
				bIsInTargetingMode = false;
				PendingSkillId = 0;
				PendingSkillName.Empty();
				PendingTargetingMode = ESkillTargetingMode::None;
				HideTargetingOverlay();

				// Lock the AoE circle at the target position while walking
				GroundAoE::FState& GS = GroundAoE::Get(World);
				GS.bLocked = true;
				GS.LockedPosition = GroundPos + FVector(0.f, 0.f, 5.f);

				// Start walking toward target
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(PC, GroundPos);

				// Poll distance at 50ms intervals — two-phase: walk to range, then wait for stop
				TWeakObjectPtr<USkillTreeSubsystem> WeakThis(this);
				World->GetTimerManager().SetTimer(
					P.CheckTimer,
					FTimerDelegate::CreateLambda([WeakThis, World]()
					{
						if (!WeakThis.IsValid() || !World) return;
						WalkToCast::FPending& WP = WalkToCast::Get(World);
						if (!WP.bActive) return;

						APlayerController* WPC = World->GetFirstPlayerController();
						APawn* WPawn = WPC ? WPC->GetPawn() : nullptr;
						if (!WPawn)
						{
							WalkToCast::Cancel(World);
							GroundAoE::StopDrawing(World);
							return;
						}

						const float WDist = FVector::Dist2D(WPawn->GetActorLocation(), WP.GroundTarget);

						if (!WP.bWaitingForStop)
						{
							// Phase 1: walking toward target — check if we've reached range
							if (WDist <= WP.RequiredRange + 30.f)
							{
								// In range — stop movement and wait for pawn to fully stop
								WP.bWaitingForStop = true;
								WalkToCast::StopMovement(World);
								UE_LOG(LogSkillTree, Log, TEXT("WalkToCast: reached range (%.0f), stopping movement"), WDist);
							}
						}
						else
						{
							// Phase 2: waiting for pawn velocity to reach zero
							const float Speed = WPawn->GetVelocity().Size();
							if (Speed < 5.f)
							{
								// Fully stopped — cast at max range toward the original target
								const int32 CapturedSkillId = WP.SkillId;
								const FVector OriginalTarget = WP.GroundTarget;
								const float MaxRange = WP.RequiredRange;
								WP.bActive = false;
								WP.bWaitingForStop = false;
								World->GetTimerManager().ClearTimer(WP.CheckTimer);

								// Compute cast position: from player toward target, clamped to max range
								const FVector PlayerPos = WPawn->GetActorLocation();
								const FVector ToTarget = OriginalTarget - PlayerPos;
								const float DistToTarget = ToTarget.Size2D();
								FVector CastPos;
								if (DistToTarget <= MaxRange)
								{
									// Already within max range — cast at original target
									CastPos = OriginalTarget;
								}
								else
								{
									// Cast at max range along the direction toward target
									const FVector Dir2D = FVector(ToTarget.X, ToTarget.Y, 0.f).GetSafeNormal();
									CastPos = FVector(PlayerPos.X + Dir2D.X * MaxRange,
										PlayerPos.Y + Dir2D.Y * MaxRange,
										OriginalTarget.Z);
								}

								// Lock circle + dismiss after cast+effect duration
								const FSkillEntry* WEntry = WeakThis->FindSkillEntry(CapturedSkillId);
								int32 WBaseCastMs = WEntry ? WEntry->CastTime : 0;
								int32 WSkillLv = WEntry ? FMath::Max(1, WEntry->CurrentLevel) : 1;
								int32 WDex = 1;
								if (UCombatStatsSubsystem* WCSS = World->GetSubsystem<UCombatStatsSubsystem>())
									WDex = FMath::Max(1, WCSS->DEX);
								float WCastSec = FMath::Max(0.f, (WBaseCastMs / 1000.f) * (1.f - WDex / 150.f));
								GroundAoE::FAoEInfo WAoE = GroundAoE::GetAoEInfo(CapturedSkillId, WSkillLv);
								GroundAoE::LockAndDismissAfter(World, CastPos, WCastSec + WAoE.EffectDurationSec);

								WeakThis->UseSkillOnGround(CapturedSkillId, CastPos, WP.SkillLevel);
								UE_LOG(LogSkillTree, Log, TEXT("WalkToCast: stopped, casting skill %d Lv%d at max range"),
									CapturedSkillId, WP.SkillLevel);
							}
						}
					}),
					0.05f, true);

				UE_LOG(LogSkillTree, Log, TEXT("WalkToCast: skill %d out of range (%.0f > %.0f), walking to target"),
					SkillToUse, Dist2D, SkillRange);
			}
			else
			{
				// In range (or unlimited range) — fire immediately
				int32 BaseCastTimeMs = Entry ? Entry->CastTime : 0;
				int32 SkillLevel = Entry ? FMath::Max(1, Entry->CurrentLevel) : 1;

				int32 PlayerDex = 1;
				if (UCombatStatsSubsystem* CSS = World->GetSubsystem<UCombatStatsSubsystem>())
				{
					PlayerDex = FMath::Max(1, CSS->DEX);
				}
				float ActualCastTimeSec = FMath::Max(0.f, (BaseCastTimeMs / 1000.f) * (1.f - PlayerDex / 150.f));

				GroundAoE::FAoEInfo AoEInfo = GroundAoE::GetAoEInfo(SkillToUse, SkillLevel);
				float TotalDuration = ActualCastTimeSec + AoEInfo.EffectDurationSec;

				GroundAoE::LockAndDismissAfter(World, GroundPos, TotalDuration);

				bIsInTargetingMode = false;
				PendingSkillId = 0;
				PendingSkillName.Empty();
				PendingTargetingMode = ESkillTargetingMode::None;
				HideTargetingOverlay();

				UseSkillOnGround(SkillToUse, GroundPos, PendingSkillLevel);

				UE_LOG(LogSkillTree, Log, TEXT("Targeting executed: skill %d Lv%d at ground (%.0f, %.0f, %.0f)"),
					SkillToUse, PendingSkillLevel, GroundPos.X, GroundPos.Y, GroundPos.Z);
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("Invalid ground position"));
			}
			UE_LOG(LogSkillTree, Log, TEXT("Ground targeting: no valid ground under cursor"));
		}
	}
}

void USkillTreeSubsystem::HandleTargetingCancel()
{
	CancelTargeting();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, TEXT("Targeting cancelled"));
	}
}

// ============================================================
// Skill drag state (for drag-to-hotbar)
// ============================================================

void USkillTreeSubsystem::StartSkillDrag(int32 SkillId, const FString& Name, const FString& Icon)
{
	bSkillDragging = true;
	DraggedSkillId = SkillId;
	DraggedSkillLevel = GetSelectedLevel(SkillId);
	DraggedSkillName = Name;
	DraggedSkillIcon = Icon;
	ShowSkillDragCursor(Icon);
}

void USkillTreeSubsystem::CancelSkillDrag()
{
	bSkillDragging = false;
	DraggedSkillId = 0;
	DraggedSkillName.Empty();
	DraggedSkillIcon.Empty();
	HideSkillDragCursor();
}

void USkillTreeSubsystem::CancelWalkToCast()
{
	WalkToCast::Cancel(GetWorld());
}

void USkillTreeSubsystem::ShowSkillDragCursor(const FString& IconPath)
{
	HideSkillDragCursor();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	static constexpr float DragIconSize = 28.f;

	TSharedRef<SWidget> IconContent = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = GetOrCreateIconBrush(IconPath);
		if (Brush)
		{
			return SNew(SImage).Image(Brush);
		}
		// Fallback: gold square
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.72f, 0.58f, 0.28f, 1.f));
	}();

	SAssignNew(SkillDragCursorBox, SBox)
		.WidthOverride(DragIconSize)
		.HeightOverride(DragIconSize)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::HitTestInvisible)
		[
			IconContent
		];

	SkillDragCursorAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::HitTestInvisible)
		[
			SkillDragCursorBox.ToSharedRef()
		];

	UpdateSkillDragCursorPosition();

	SkillDragCursorOverlay = SNew(SWeakWidget).PossiblyNullContent(SkillDragCursorAlignWrapper);
	VC->AddViewportWidgetContent(SkillDragCursorOverlay.ToSharedRef(), 50);
}

void USkillTreeSubsystem::HideSkillDragCursor()
{
	if (SkillDragCursorOverlay.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameViewportClient* VC = World->GetGameViewport();
			if (VC)
			{
				VC->RemoveViewportWidgetContent(SkillDragCursorOverlay.ToSharedRef());
			}
		}
	}
	SkillDragCursorBox.Reset();
	SkillDragCursorAlignWrapper.Reset();
	SkillDragCursorOverlay.Reset();
}

void USkillTreeSubsystem::UpdateSkillDragCursorPosition()
{
	if (!bSkillDragging || !SkillDragCursorBox.IsValid() || !SkillDragCursorAlignWrapper.IsValid()) return;

	FVector2D CursorAbsPos = FSlateApplication::Get().GetCursorPos();
	FGeometry WrapperGeo = SkillDragCursorAlignWrapper->GetCachedGeometry();
	FVector2D LocalPos = WrapperGeo.AbsoluteToLocal(CursorAbsPos);

	static constexpr float HalfIcon = 14.f;  // 28 / 2
	SkillDragCursorBox->SetRenderTransform(FSlateRenderTransform(
		FVector2f((float)LocalPos.X - HalfIcon, (float)LocalPos.Y - HalfIcon)));
}

// ============================================================
// GetEnemyIdFromActor — uses EnemySubsystem C++ struct lookup
// (replaces property reflection — struct refactor)
// ============================================================

int32 USkillTreeSubsystem::GetEnemyIdFromActor(AActor* Actor) const
{
	if (!Actor) return 0;
	UWorld* World = GetWorld();
	if (!World) return 0;
	if (UEnemySubsystem* ES = World->GetSubsystem<UEnemySubsystem>())
		return ES->GetEnemyIdFromActor(Actor);
	return 0;
}
