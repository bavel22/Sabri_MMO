// SkillTreeSubsystem.cpp — Implementation of the Skill Tree HUD subsystem

#include "SkillTreeSubsystem.h"
#include "SSkillTreeWidget.h"
#include "SSkillTargetingOverlay.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogSkillTree, Log, All);

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

	// Start a repeating timer to poll for the SocketIO component
	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &USkillTreeSubsystem::TryWrapSocketEvents),
		0.5f,
		true
	);

	UE_LOG(LogSkillTree, Log, TEXT("SkillTreeSubsystem started — waiting for SocketIO bindings..."));
}

void USkillTreeSubsystem::Deinitialize()
{
	HideWidget();

	// Clean up targeting overlay if active
	bIsInTargetingMode = false;
	HideTargetingOverlay();

	// Clean up skill drag cursor if active
	HideSkillDragCursor();
	bSkillDragging = false;

	// Clear icon caches — release GC-rooted textures and brush memory
	IconBrushCache.Empty();
	IconTextureCache.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
		World->GetTimerManager().ClearTimer(HotbarRequestTimer);
	}

	bEventsWrapped = false;
	CachedSIOComponent = nullptr;

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
// Find the SocketIO component on BP_SocketManager
// ============================================================

USocketIOClientComponent* USkillTreeSubsystem::FindSocketIOComponent() const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
		{
			return Comp;
		}
	}
	return nullptr;
}

// ============================================================
// Timer callback — wait until BP events are bound, then wrap
// ============================================================

void USkillTreeSubsystem::TryWrapSocketEvents()
{
	if (bEventsWrapped) return;

	USocketIOClientComponent* SIOComp = FindSocketIOComponent();
	if (!SIOComp) return;

	TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
	if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

	// Wait for BP to have bound at least one event (meaning connection is fully established)
	if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

	CachedSIOComponent = SIOComp;

	// Populate character ID from GameInstance
	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
	}

	// --- Bind skill events (these are new events, not wrapping existing BP ones) ---
	BindNewEvent(TEXT("skill:data"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillData(D); });

	BindNewEvent(TEXT("skill:learned"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillLearned(D); });

	BindNewEvent(TEXT("skill:refresh"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillRefresh(D); });

	BindNewEvent(TEXT("skill:reset_complete"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillResetComplete(D); });

	BindNewEvent(TEXT("skill:error"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillError(D); });

	BindNewEvent(TEXT("skill:used"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillUsed(D); });

	// --- Bind NEW skill combat events (C++ only, not in BP) ---
	BindNewEvent(TEXT("skill:effect_damage"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillEffectDamage(D); });

	BindNewEvent(TEXT("skill:buff_applied"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillBuffApplied(D); });

	BindNewEvent(TEXT("skill:buff_removed"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillBuffRemoved(D); });

	BindNewEvent(TEXT("skill:cooldown_started"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleSkillCooldownStarted(D); });

	// Wrap hotbar:alldata — BP_SocketManager already handles it (PopulateHotbarFromServer),
	// so we use WrapExistingEvent to chain our handler AFTER the BP handler runs.
	// This lets us parse skill slots into HotbarSkillMap without breaking item slot display.
	WrapExistingEvent(TEXT("hotbar:alldata"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleHotbarAllData(D); });

	bEventsWrapped = true;

	// Stop the polling timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	UE_LOG(LogSkillTree, Log, TEXT("SkillTreeSubsystem — all skill socket events bound successfully."));

	// Immediately request skill data so it's cached before the user ever opens the window.
	// If the widget is already visible (user opened it before socket was ready),
	// HandleSkillData will rebuild its content when the response arrives.
	RequestSkillData();
	UE_LOG(LogSkillTree, Log, TEXT("Eagerly requesting skill:data now that socket is ready."));

	// Request hotbar data after a delay to ensure BP's HUDManager is fully initialized
	// (BP_SocketManager has a 0.2s Delay before setting MMOCharacterHudManager + InitializeHUD)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HotbarRequestTimer, FTimerDelegate::CreateLambda([this]()
		{
			if (CachedSIOComponent.IsValid())
			{
				CachedSIOComponent->EmitNative(TEXT("hotbar:request"), TEXT("{}"));
				UE_LOG(LogSkillTree, Log, TEXT("Emitted hotbar:request (delayed — HUD should be ready)"));
			}
		}), 3.0f, false);
	}
}

// ============================================================
// Bind a new event (skill events are not wrapped from BP, they are C++ only)
// ============================================================

void USkillTreeSubsystem::BindNewEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	NativeClient->OnEvent(EventName,
		[Handler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (Handler)
			{
				Handler(Message);
			}
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);

	UE_LOG(LogSkillTree, Verbose, TEXT("Bound skill event: %s"), *EventName);
}

// ============================================================
// Wrap an existing BP-bound event: save original, call both
// ============================================================

void USkillTreeSubsystem::WrapExistingEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	// Save the original callback (Blueprint handler)
	TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
	FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
	if (Existing)
	{
		OriginalCallback = Existing->Function;
	}

	// Replace with a combined callback that calls both
	NativeClient->OnEvent(EventName,
		[OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			// Call original Blueprint handler first
			if (OriginalCallback)
			{
				OriginalCallback(Event, Message);
			}
			// Then call our C++ handler
			if (OurHandler)
			{
				OurHandler(Message);
			}
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);

	UE_LOG(LogSkillTree, Log, TEXT("Wrapped event: %s (had original: %s)"),
		*EventName, OriginalCallback ? TEXT("yes") : TEXT("no"));
}

// ============================================================
// Actions (called from the widget or Blueprint)
// ============================================================

void USkillTreeSubsystem::RequestSkillData()
{
	if (!CachedSIOComponent.IsValid())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("RequestSkillData — socket not ready yet, will retry when connected."));
		return;
	}

	CachedSIOComponent->EmitNative(TEXT("skill:data"), TEXT("{}"));
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:data request"));
}

void USkillTreeSubsystem::LearnSkill(int32 SkillId)
{
	if (!CachedSIOComponent.IsValid()) return;

	// Validate skillId before sending to server
	if (SkillId <= 0)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("LearnSkill called with invalid skillId=%d, aborting"), SkillId);
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("skillId"), SkillId);

	CachedSIOComponent->EmitNative(TEXT("skill:learn"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:learn for skillId=%d"), SkillId);
}

void USkillTreeSubsystem::ResetAllSkills()
{
	if (!CachedSIOComponent.IsValid()) return;

	CachedSIOComponent->EmitNative(TEXT("skill:reset"), TEXT("{}"));
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:reset"));
}

void USkillTreeSubsystem::AssignSkillToHotbar(int32 SkillId, const FString& SkillDisplayName, int32 SlotIndex)
{
	if (!CachedSIOComponent.IsValid())
	{
		UE_LOG(LogSkillTree, Warning, TEXT("AssignSkillToHotbar: SocketIO not connected"));
		return;
	}

	if (SlotIndex < 1 || SlotIndex > 9)
	{
		UE_LOG(LogSkillTree, Warning, TEXT("AssignSkillToHotbar: Invalid slot index %d"), SlotIndex);
		return;
	}

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	Payload->SetStringField(TEXT("skillName"), SkillDisplayName);

	CachedSIOComponent->EmitNative(TEXT("hotbar:save_skill"), Payload);

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
// Chained after BP PopulateHotbarFromServer via WrapExistingEvent.
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

void USkillTreeSubsystem::UseSkill(int32 SkillId)
{
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
	UseSkillOnTarget(SkillId, 0, false);
}


void USkillTreeSubsystem::UseSkillOnTarget(int32 SkillId, int32 TargetId, bool bIsEnemy)
{
	if (!CachedSIOComponent.IsValid())
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

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	if (TargetId > 0)
	{
		Payload->SetNumberField(TEXT("targetId"), TargetId);
		Payload->SetBoolField(TEXT("isEnemy"), bIsEnemy);
	}

	CachedSIOComponent->EmitNative(TEXT("skill:use"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:use for skillId=%d target=%d isEnemy=%d"),
		SkillId, TargetId, bIsEnemy);
}

void USkillTreeSubsystem::UseSkillOnGround(int32 SkillId, FVector GroundPosition)
{
	if (!CachedSIOComponent.IsValid())
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

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	Payload->SetNumberField(TEXT("groundX"), GroundPosition.X);
	Payload->SetNumberField(TEXT("groundY"), GroundPosition.Y);
	Payload->SetNumberField(TEXT("groundZ"), GroundPosition.Z);

	CachedSIOComponent->EmitNative(TEXT("skill:use"), Payload);
	UE_LOG(LogSkillTree, Log, TEXT("Emitted skill:use for skillId=%d at ground (%.0f, %.0f, %.0f)"),
		SkillId, GroundPosition.X, GroundPosition.Y, GroundPosition.Z);
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

	// Explicit mapping for icons whose asset names don't match the server icon name
	static TMap<FString, FString> ExplicitMap;
	if (ExplicitMap.Num() == 0)
	{
		ExplicitMap.Add(TEXT("first_aid"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/RO_Skill_Icon_Heal"));
		ExplicitMap.Add(TEXT("heal"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/RO_Skill_Icon_Heal"));
		// Swordsman skill icons
		ExplicitMap.Add(TEXT("sword_mastery"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/sword_mastery_icon"));
		ExplicitMap.Add(TEXT("two_handed_sword_mastery"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/two_handed_sword_mastery_icon"));
		ExplicitMap.Add(TEXT("increase_hp_recovery"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/increase_hp_recovery_icon"));
		ExplicitMap.Add(TEXT("bash"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/Bash_Icon"));
		ExplicitMap.Add(TEXT("provoke"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/Provoke_Icon"));
		ExplicitMap.Add(TEXT("magnum_break"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/Magnum_Break_Icon"));
		ExplicitMap.Add(TEXT("endure"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Swordsman/endure_icon"));
		// Mage skill icons
		ExplicitMap.Add(TEXT("cold_bolt"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/cold_bolt_icon"));
		ExplicitMap.Add(TEXT("fire_bolt"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/fire_bolt_icon"));
		ExplicitMap.Add(TEXT("lightning_bolt"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/lightning_bolt_icon"));
		ExplicitMap.Add(TEXT("napalm_beat"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/napalm_beat_icon"));
		ExplicitMap.Add(TEXT("increase_sp_recovery"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/increase_SP_recovery_icon"));
		ExplicitMap.Add(TEXT("sight"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/sight_icon"));
		ExplicitMap.Add(TEXT("stone_curse"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/stone_curse_icon"));
		ExplicitMap.Add(TEXT("fire_ball"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/fire_ball_icon"));
		ExplicitMap.Add(TEXT("frost_diver"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/frost_diver_icon"));
		ExplicitMap.Add(TEXT("fire_wall"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/firewall_icon"));
		ExplicitMap.Add(TEXT("safety_wall"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/safety_wall_icon"));
		ExplicitMap.Add(TEXT("soul_strike"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/soul_strike_icon"));
		ExplicitMap.Add(TEXT("thunderstorm"), TEXT("/Game/SabriMMO/Assets/Skill_Icons/Mage/thunderstorm_icon"));
	}

	if (const FString* Found = ExplicitMap.Find(IconName))
	{
		return *Found;
	}

	// Convention fallback: /Game/SabriMMO/Assets/Skill_Icons/RO_Skill_Icon_<PascalName>
	FString PascalName = IconName;
	PascalName = PascalName.Replace(TEXT("_"), TEXT(" "));
	// Capitalize each word
	bool bCapNext = true;
	for (int32 i = 0; i < PascalName.Len(); ++i)
	{
		if (PascalName[i] == ' ') { bCapNext = true; continue; }
		if (bCapNext) { PascalName[i] = FChar::ToUpper(PascalName[i]); bCapNext = false; }
	}
	PascalName = PascalName.Replace(TEXT(" "), TEXT("_"));
	return FString::Printf(TEXT("/Game/SabriMMO/Assets/Skill_Icons/RO_Skill_Icon_%s"), *PascalName);
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
					Entry.IconPath = ResolveIconContentPath(Entry.Icon);
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
	double Level = 0, SpCost = 0;
	Obj->TryGetNumberField(TEXT("level"), Level);
	Obj->TryGetNumberField(TEXT("spCost"), SpCost);

	UE_LOG(LogSkillTree, Log, TEXT("Skill used: %s Lv%d (SP cost: %d)"),
		*SkillName, (int32)Level, (int32)SpCost);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
			FString::Printf(TEXT("%s Lv%d used! (-%d SP)"), *SkillName, (int32)Level, (int32)SpCost));
	}
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
	// If already targeting a different skill, cancel the old one first
	if (bIsInTargetingMode)
	{
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
	PendingSkillName = Entry->DisplayName;

	if (Entry->TargetType == TEXT("single"))
	{
		PendingTargetingMode = ESkillTargetingMode::SingleTarget;
	}
	else
	{
		PendingTargetingMode = ESkillTargetingMode::GroundTarget;
	}

	bIsInTargetingMode = true;
	ShowTargetingOverlay();

	UE_LOG(LogSkillTree, Log, TEXT("Targeting started: %s (mode=%s)"),
		*PendingSkillName,
		PendingTargetingMode == ESkillTargetingMode::SingleTarget ? TEXT("SingleTarget") : TEXT("GroundTarget"));
}

void USkillTreeSubsystem::CancelTargeting()
{
	if (!bIsInTargetingMode) return;

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
		Hint = TEXT("Left-click an enemy to use — Right-click / ESC to cancel");
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
		// Line trace under cursor against Pawn channel (Characters have capsule collision)
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Pawn, false, HitResult) && HitResult.bBlockingHit)
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor)
			{
				int32 EnemyId = GetEnemyIdFromActor(HitActor);
				if (EnemyId > 0)
				{
					const int32 SkillToUse = PendingSkillId;
					CancelTargeting();
					UseSkillOnTarget(SkillToUse, EnemyId, /*bIsEnemy=*/ true);

					UE_LOG(LogSkillTree, Log, TEXT("Targeting executed: skill %d on enemy %d"), SkillToUse, EnemyId);
					return;
				}
			}
		}

		// No valid enemy under cursor
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("Invalid target — click an enemy"));
		}
		UE_LOG(LogSkillTree, Log, TEXT("Targeting click: no valid enemy under cursor"));
	}
	else if (PendingTargetingMode == ESkillTargetingMode::GroundTarget)
	{
		// Ground targeting: line-trace under cursor to get world position
		FHitResult HitResult;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, HitResult) && HitResult.bBlockingHit)
		{
			const FVector GroundPos = HitResult.ImpactPoint;
			const int32 SkillToUse = PendingSkillId;
			CancelTargeting();
			UseSkillOnGround(SkillToUse, GroundPos);

			UE_LOG(LogSkillTree, Log, TEXT("Targeting executed: skill %d at ground (%.0f, %.0f, %.0f)"),
				SkillToUse, GroundPos.X, GroundPos.Y, GroundPos.Z);
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
// GetEnemyIdFromActor — extract the EnemyId Blueprint variable
// via UE5 property reflection.  BP_EnemyCharacter stores
// "EnemyId" as an int variable (server-assigned, 2000001+).
// ============================================================

int32 USkillTreeSubsystem::GetEnemyIdFromActor(AActor* Actor) const
{
	if (!Actor) return 0;

	// Try the known Blueprint variable name first (from BP_EnemyCharacter docs)
	static const FName PrimaryName(TEXT("EnemyId"));
	if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(PrimaryName))
	{
		if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			int32 Value = IntProp->GetPropertyValue_InContainer(Actor);
			if (Value > 0)
			{
				return Value;
			}
		}
	}

	// Fallback: try alternate capitalisations
	static const FName Alternates[] = {
		FName(TEXT("EnemyID")),
		FName(TEXT("enemyId")),
		FName(TEXT("Enemy Id"))
	};

	for (const FName& AltName : Alternates)
	{
		if (FProperty* Prop = Actor->GetClass()->FindPropertyByName(AltName))
		{
			if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				int32 Value = IntProp->GetPropertyValue_InContainer(Actor);
				if (Value > 0)
				{
					return Value;
				}
			}
		}
	}

	return 0;
}
