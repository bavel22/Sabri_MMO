// HotbarSubsystem.cpp — Manages hotbar slot data, Socket.io events, keybinds, and widget lifecycle.

#include "HotbarSubsystem.h"
#include "SHotbarRowWidget.h"
#include "SHotbarKeybindWidget.h"
#include "InventorySubsystem.h"
#include "SkillTreeSubsystem.h"
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
#include "Misc/ConfigCacheIni.h"

DEFINE_LOG_CATEGORY_STATIC(LogHotbar, Log, All);

const FHotbarSlot UHotbarSubsystem::EmptySlot = FHotbarSlot();

// ============================================================
// FHotbarKeybind helpers
// ============================================================

FString FHotbarKeybind::GetDisplayString() const
{
	if (!IsValid()) return TEXT("");

	FString Result;
	if (bRequiresCtrl) Result += TEXT("C+");
	if (bRequiresAlt) Result += TEXT("A+");
	if (bRequiresShift) Result += TEXT("S+");

	// Shorten common key names
	FString KeyStr = PrimaryKey.GetDisplayName().ToString();
	if (KeyStr.Len() == 1 || KeyStr.StartsWith(TEXT("F")))
	{
		Result += KeyStr;
	}
	else
	{
		// Truncate long names
		Result += KeyStr.Left(3);
	}
	return Result;
}

// ============================================================
// Lifecycle
// ============================================================

bool UHotbarSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UHotbarSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
	}

	InitializeDefaultKeybinds();
	LoadKeybinds();

	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &UHotbarSubsystem::TryWrapSocketEvents),
		0.5f, true
	);

	UE_LOG(LogHotbar, Log, TEXT("HotbarSubsystem started — waiting for SocketIO bindings..."));
}

void UHotbarSubsystem::Deinitialize()
{
	UE_LOG(LogHotbar, Log, TEXT("Deinitialize: World=%p CharId=%d bEventsWrapped=%s RowsAdded=[%d,%d,%d,%d]"),
		GetWorld(), LocalCharacterId, bEventsWrapped ? TEXT("true") : TEXT("false"),
		RowWidgets[0].bIsAdded ? 1 : 0, RowWidgets[1].bIsAdded ? 1 : 0,
		RowWidgets[2].bIsAdded ? 1 : 0, RowWidgets[3].bIsAdded ? 1 : 0);

	HideAllRows();
	HideKeybindWidget();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	bEventsWrapped = false;
	bInitialRowsShown = false;
	CachedSIOComponent = nullptr;
	CachedViewportClient = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Find SocketIO component
// ============================================================

USocketIOClientComponent* UHotbarSubsystem::FindSocketIOComponent() const
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
// Timer callback — wrap events when ready
// ============================================================

void UHotbarSubsystem::TryWrapSocketEvents()
{
	// Phase 1: Wrap socket events (runs once)
	if (!bEventsWrapped)
	{
		USocketIOClientComponent* SIOComp = FindSocketIOComponent();
		if (!SIOComp) return;

		TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
		if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

		// Wait for BP to bind events first
		if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update"))) return;

		CachedSIOComponent = SIOComp;

		if (LocalCharacterId == 0)
		{
			if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
			{
				FCharacterData SelChar = GI->GetSelectedCharacter();
				LocalCharacterId = SelChar.CharacterId;
			}
		}

		// Wrap hotbar:alldata — chains after SkillTreeSubsystem's handler (both parse independently)
		WrapExistingEvent(TEXT("hotbar:alldata"),
			[this](const TSharedPtr<FJsonValue>& D) { HandleHotbarAllData(D); });

		bEventsWrapped = true;

		// Request fresh hotbar data (initial events may have fired before wrapping)
		// Use a delay to allow SkillTreeSubsystem to also wrap first
		if (UWorld* World = GetWorld())
		{
			FTimerHandle RequestTimer;
			World->GetTimerManager().SetTimer(RequestTimer, [this]()
			{
				if (CachedSIOComponent.IsValid())
				{
					CachedSIOComponent->EmitNative(TEXT("hotbar:request"), TEXT("{}"));
					UE_LOG(LogHotbar, Log, TEXT("Emitted hotbar:request (delayed after wrap)"));
				}
			}, 1.0f, false);
		}

		UE_LOG(LogHotbar, Log, TEXT("HotbarSubsystem — events wrapped. LocalCharId=%d World=%p VC=%p"),
			LocalCharacterId, GetWorld(), GetWorld() ? GetWorld()->GetGameViewport() : nullptr);
	}

	// Phase 2: Show initial rows (retries until viewport is available)
	if (bEventsWrapped && !bInitialRowsShown)
	{
		UWorld* World = GetWorld();
		UGameViewportClient* VC = World ? World->GetGameViewport() : nullptr;
		if (VC)
		{
			// Cache this VC for the lifetime of this subsystem.
			// World->GetGameViewport() returns GEngine->GameViewport which is a global
			// that changes when other PIE instances transition levels. We must always
			// use the VC we first added widgets to for all subsequent add/remove calls.
			CachedViewportClient = VC;
			ShowRows();
			bInitialRowsShown = true;
			UE_LOG(LogHotbar, Log, TEXT("Initial rows shown. CachedVC=%p World=%p"), VC, World);
		}
		else
		{
			UE_LOG(LogHotbar, Log, TEXT("Viewport not ready yet — will retry (World=%p)"), World);
		}
	}

	// Only stop timer when BOTH phases are complete
	if (bEventsWrapped && bInitialRowsShown)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BindCheckTimer);
		}
	}
}

// ============================================================
// Event wrapping (same pattern as other subsystems)
// ============================================================

void UHotbarSubsystem::WrapExistingEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
	FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
	if (Existing)
	{
		OriginalCallback = Existing->Function;
	}

	NativeClient->OnEvent(EventName,
		[OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (OriginalCallback) OriginalCallback(Event, Message);
			if (OurHandler) OurHandler(Message);
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);
}

void UHotbarSubsystem::BindNewEvent(
	const FString& EventName,
	TFunction<void(const TSharedPtr<FJsonValue>&)> Handler)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
	if (!NativeClient.IsValid()) return;

	NativeClient->OnEvent(EventName,
		[Handler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
		{
			if (Handler) Handler(Message);
		},
		TEXT("/"),
		ESIOThreadOverrideOption::USE_GAME_THREAD
	);
}

// ============================================================
// HandleHotbarAllData — parse all slots from server
// ============================================================

void UHotbarSubsystem::HandleHotbarAllData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
	if (!Obj->TryGetArrayField(TEXT("slots"), SlotsArray) || !SlotsArray) return;

	// Clear all slots first
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			Slots[r][s].Clear();
			Slots[r][s].RowIndex = r;
			Slots[r][s].SlotIndex = s;
		}
	}

	int32 ItemCount = 0, SkillCount = 0;

	for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
	{
		const TSharedPtr<FJsonObject>* SlotObj = nullptr;
		if (!SlotVal->TryGetObject(SlotObj) || !SlotObj) continue;
		const TSharedPtr<FJsonObject>& Slot = *SlotObj;

		double RowD = 0, SlotD = 0;
		Slot->TryGetNumberField(TEXT("row_index"), RowD);
		Slot->TryGetNumberField(TEXT("slot_index"), SlotD);

		int32 Row = (int32)RowD;
		int32 Col = (int32)SlotD;
		if (Row < 0 || Row >= NUM_ROWS || Col < 0 || Col >= SLOTS_PER_ROW) continue;

		FHotbarSlot& S = Slots[Row][Col];
		S.RowIndex = Row;
		S.SlotIndex = Col;

		FString SlotType;
		Slot->TryGetStringField(TEXT("slot_type"), SlotType);
		S.SlotType = SlotType;

		if (SlotType == TEXT("skill"))
		{
			double SkillId = 0;
			Slot->TryGetNumberField(TEXT("skill_id"), SkillId);
			S.SkillId = (int32)SkillId;
			Slot->TryGetStringField(TEXT("skill_name"), S.SkillName);

			// Look up icon from SkillTreeSubsystem
			if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
			{
				const FSkillEntry* Entry = SkillSub->FindSkillEntry(S.SkillId);
				if (Entry)
				{
					S.SkillIcon = Entry->Icon;
					if (S.SkillName.IsEmpty()) S.SkillName = Entry->DisplayName;
				}
			}

			SkillCount++;
			UE_LOG(LogHotbar, Verbose, TEXT("  Row %d Slot %d: skill %s (id=%d)"), Row, Col, *S.SkillName, S.SkillId);
		}
		else if (SlotType == TEXT("item"))
		{
			double InvId = 0, ItemIdD = 0, Qty = 0;
			Slot->TryGetNumberField(TEXT("inventory_id"), InvId);
			Slot->TryGetNumberField(TEXT("item_id"), ItemIdD);
			Slot->TryGetNumberField(TEXT("quantity"), Qty);
			S.InventoryId = (int32)InvId;
			S.ItemId = (int32)ItemIdD;
			S.Quantity = (int32)Qty;
			Slot->TryGetStringField(TEXT("item_name"), S.ItemName);

			// Look up icon from InventorySubsystem
			if (UInventorySubsystem* InvSub = GetWorld()->GetSubsystem<UInventorySubsystem>())
			{
				FInventoryItem* FoundItem = InvSub->FindItemByInventoryId(S.InventoryId);
				if (FoundItem)
				{
					S.ItemIcon = FoundItem->Icon;
					S.Quantity = FoundItem->Quantity; // Use live quantity
				}
			}

			ItemCount++;
			UE_LOG(LogHotbar, Verbose, TEXT("  Row %d Slot %d: item %s (qty=%d)"), Row, Col, *S.ItemName, S.Quantity);
		}
	}

	DataVersion++;
	OnHotbarDataUpdated.Broadcast();

	UE_LOG(LogHotbar, Log, TEXT("HandleHotbarAllData: %d items, %d skills across %d raw slots"),
		ItemCount, SkillCount, SlotsArray->Num());
}

// ============================================================
// Slot operations
// ============================================================

const FHotbarSlot& UHotbarSubsystem::GetSlot(int32 RowIndex, int32 SlotIndex) const
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW)
		return EmptySlot;
	return Slots[RowIndex][SlotIndex];
}

void UHotbarSubsystem::AssignItem(int32 RowIndex, int32 SlotIndex, const FInventoryItem& Item)
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

	FHotbarSlot& S = Slots[RowIndex][SlotIndex];
	S.Clear();
	S.RowIndex = RowIndex;
	S.SlotIndex = SlotIndex;
	S.SlotType = TEXT("item");
	S.InventoryId = Item.InventoryId;
	S.ItemId = Item.ItemId;
	S.ItemName = Item.Name;
	S.ItemIcon = Item.Icon;
	S.Quantity = Item.Quantity;

	EmitSaveItem(RowIndex, SlotIndex, Item.InventoryId, Item.ItemId, Item.Name);
	DataVersion++;
	OnHotbarDataUpdated.Broadcast();

	UE_LOG(LogHotbar, Log, TEXT("AssignItem: row %d slot %d = %s (qty=%d)"), RowIndex, SlotIndex, *Item.Name, Item.Quantity);
}

void UHotbarSubsystem::AssignSkill(int32 RowIndex, int32 SlotIndex, int32 SkillId, const FString& SkillName, const FString& SkillIcon)
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

	FHotbarSlot& S = Slots[RowIndex][SlotIndex];
	S.Clear();
	S.RowIndex = RowIndex;
	S.SlotIndex = SlotIndex;
	S.SlotType = TEXT("skill");
	S.SkillId = SkillId;
	S.SkillName = SkillName;
	S.SkillIcon = SkillIcon;

	// Look up icon if not provided
	if (S.SkillIcon.IsEmpty())
	{
		if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
		{
			const FSkillEntry* Entry = SkillSub->FindSkillEntry(SkillId);
			if (Entry)
			{
				S.SkillIcon = Entry->Icon;
			}
		}
	}

	EmitSaveSkill(RowIndex, SlotIndex, SkillId, SkillName);
	DataVersion++;
	OnHotbarDataUpdated.Broadcast();

	UE_LOG(LogHotbar, Log, TEXT("AssignSkill: row %d slot %d = %s (id=%d)"), RowIndex, SlotIndex, *SkillName, SkillId);
}

void UHotbarSubsystem::ClearSlot(int32 RowIndex, int32 SlotIndex)
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

	Slots[RowIndex][SlotIndex].Clear();
	Slots[RowIndex][SlotIndex].RowIndex = RowIndex;
	Slots[RowIndex][SlotIndex].SlotIndex = SlotIndex;

	EmitClearSlot(RowIndex, SlotIndex);
	DataVersion++;
	OnHotbarDataUpdated.Broadcast();

	UE_LOG(LogHotbar, Log, TEXT("ClearSlot: row %d slot %d"), RowIndex, SlotIndex);
}

void UHotbarSubsystem::ActivateSlot(int32 RowIndex, int32 SlotIndex)
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;

	const FHotbarSlot& S = Slots[RowIndex][SlotIndex];
	if (S.IsEmpty()) return;

	if (S.IsSkill())
	{
		if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
		{
			UE_LOG(LogHotbar, Log, TEXT("ActivateSlot: row %d slot %d → UseSkill(%d) %s"), RowIndex, SlotIndex, S.SkillId, *S.SkillName);
			SkillSub->UseSkill(S.SkillId);
		}
	}
	else if (S.IsItem())
	{
		if (UInventorySubsystem* InvSub = GetWorld()->GetSubsystem<UInventorySubsystem>())
		{
			UE_LOG(LogHotbar, Log, TEXT("ActivateSlot: row %d slot %d → UseItem(%d) %s"), RowIndex, SlotIndex, S.InventoryId, *S.ItemName);
			InvSub->UseItem(S.InventoryId);
		}
	}
}

// ============================================================
// Server emit helpers
// ============================================================

void UHotbarSubsystem::EmitSaveItem(int32 RowIndex, int32 SlotIndex, int32 InventoryId, int32 ItemId, const FString& ItemName)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("rowIndex"), RowIndex);
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("itemId"), ItemId);
	Payload->SetStringField(TEXT("itemName"), ItemName);

	CachedSIOComponent->EmitNative(TEXT("hotbar:save"), Payload);
}

void UHotbarSubsystem::EmitSaveSkill(int32 RowIndex, int32 SlotIndex, int32 SkillId, const FString& SkillName)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("rowIndex"), RowIndex);
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);
	Payload->SetNumberField(TEXT("skillId"), SkillId);
	Payload->SetStringField(TEXT("skillName"), SkillName);
	Payload->SetBoolField(TEXT("zeroBased"), true); // Tell server slotIndex is 0-based

	CachedSIOComponent->EmitNative(TEXT("hotbar:save_skill"), Payload);
}

void UHotbarSubsystem::EmitClearSlot(int32 RowIndex, int32 SlotIndex)
{
	if (!CachedSIOComponent.IsValid()) return;

	TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
	Payload->SetNumberField(TEXT("rowIndex"), RowIndex);
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);

	CachedSIOComponent->EmitNative(TEXT("hotbar:clear"), Payload);
}

// ============================================================
// Visibility cycling
// ============================================================

void UHotbarSubsystem::CycleVisibility()
{
	switch (Visibility)
	{
	case EHotbarVisibility::OneRow:    Visibility = EHotbarVisibility::TwoRows; break;
	case EHotbarVisibility::TwoRows:   Visibility = EHotbarVisibility::ThreeRows; break;
	case EHotbarVisibility::ThreeRows: Visibility = EHotbarVisibility::FourRows; break;
	case EHotbarVisibility::FourRows:  Visibility = EHotbarVisibility::Hidden; break;
	case EHotbarVisibility::Hidden:    Visibility = EHotbarVisibility::OneRow; break;
	}

	ShowRows();

	int32 RowCount = GetVisibleRowCount();
	UE_LOG(LogHotbar, Log, TEXT("CycleVisibility → %d rows visible"), RowCount);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow,
			FString::Printf(TEXT("[Hotbar] H → %d rows visible"), RowCount));
	}
}

int32 UHotbarSubsystem::GetVisibleRowCount() const
{
	switch (Visibility)
	{
	case EHotbarVisibility::OneRow:    return 1;
	case EHotbarVisibility::TwoRows:   return 2;
	case EHotbarVisibility::ThreeRows: return 3;
	case EHotbarVisibility::FourRows:  return 4;
	case EHotbarVisibility::Hidden:    return 0;
	}
	return 0;
}

bool UHotbarSubsystem::IsVisible() const
{
	return Visibility != EHotbarVisibility::Hidden;
}

// ============================================================
// Row widget lifecycle
// ============================================================

void UHotbarSubsystem::ShowRows()
{
	// Use cached viewport client (PIE-safe). World->GetGameViewport() returns
	// GEngine->GameViewport which is a GLOBAL that changes when other PIE instances
	// transition levels. Using the cached VC ensures we always add/remove from the
	// correct viewport for THIS subsystem's PIE instance.
	UGameViewportClient* VC = CachedViewportClient.Get();
	if (!VC)
	{
		UE_LOG(LogHotbar, Warning, TEXT("ShowRows: CachedViewportClient is null!"));
		return;
	}

	int32 VisibleCount = GetVisibleRowCount();

	for (int32 i = 0; i < NUM_ROWS; ++i)
	{
		if (i < VisibleCount && !RowWidgets[i].bIsAdded)
		{
			// Create row widget
			RowWidgets[i].Widget = SNew(SHotbarRowWidget)
				.Subsystem(this)
				.RowIndex(i);

			RowWidgets[i].AlignmentWrapper =
				SNew(SBox)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Visibility(EVisibility::SelfHitTestInvisible)
				[
					RowWidgets[i].Widget.ToSharedRef()
				];

			RowWidgets[i].ViewportOverlay =
				SNew(SWeakWidget).PossiblyNullContent(RowWidgets[i].AlignmentWrapper);

			VC->AddViewportWidgetContent(RowWidgets[i].ViewportOverlay.ToSharedRef(), 16);
			RowWidgets[i].bIsAdded = true;

			UE_LOG(LogHotbar, Log, TEXT("ShowRows: added row %d widget"), i);
		}
		else if (i >= VisibleCount && RowWidgets[i].bIsAdded)
		{
			// Remove row widget
			VC->RemoveViewportWidgetContent(RowWidgets[i].ViewportOverlay.ToSharedRef());
			RowWidgets[i].Widget.Reset();
			RowWidgets[i].AlignmentWrapper.Reset();
			RowWidgets[i].ViewportOverlay.Reset();
			RowWidgets[i].bIsAdded = false;

			UE_LOG(LogHotbar, Log, TEXT("ShowRows: removed row %d widget"), i);
		}
	}
}

void UHotbarSubsystem::HideAllRows()
{
	UGameViewportClient* VC = CachedViewportClient.Get();

	for (int32 i = 0; i < NUM_ROWS; ++i)
	{
		if (RowWidgets[i].bIsAdded && VC && RowWidgets[i].ViewportOverlay.IsValid())
		{
			VC->RemoveViewportWidgetContent(RowWidgets[i].ViewportOverlay.ToSharedRef());
		}
		RowWidgets[i].Widget.Reset();
		RowWidgets[i].AlignmentWrapper.Reset();
		RowWidgets[i].ViewportOverlay.Reset();
		RowWidgets[i].bIsAdded = false;
	}
}

// ============================================================
// Keybind config widget lifecycle
// ============================================================

void UHotbarSubsystem::ToggleKeybindWidget()
{
	if (bKeybindWidgetVisible)
	{
		HideKeybindWidget();
	}
	else
	{
		UGameViewportClient* VC = CachedViewportClient.Get();
		if (!VC) return;

		KeybindWidget = SNew(SHotbarKeybindWidget).Subsystem(this);

		KeybindAlignmentWrapper =
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				KeybindWidget.ToSharedRef()
			];

		KeybindViewportOverlay =
			SNew(SWeakWidget).PossiblyNullContent(KeybindAlignmentWrapper);

		VC->AddViewportWidgetContent(KeybindViewportOverlay.ToSharedRef(), 30);
		bKeybindWidgetVisible = true;
	}
}

void UHotbarSubsystem::HideKeybindWidget()
{
	if (!bKeybindWidgetVisible) return;

	UGameViewportClient* VC = CachedViewportClient.Get();
	if (VC && KeybindViewportOverlay.IsValid())
	{
		VC->RemoveViewportWidgetContent(KeybindViewportOverlay.ToSharedRef());
	}

	KeybindWidget.Reset();
	KeybindAlignmentWrapper.Reset();
	KeybindViewportOverlay.Reset();
	bKeybindWidgetVisible = false;
}

bool UHotbarSubsystem::IsKeybindWidgetVisible() const
{
	return bKeybindWidgetVisible;
}

// ============================================================
// Keybind management
// ============================================================

void UHotbarSubsystem::InitializeDefaultKeybinds()
{
	// Clear all
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			Keybinds[r][s] = FHotbarKeybind();
		}
	}

	// Row 0: Keys 1-9 (no modifiers)
	static const FKey NumberKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
	};
	for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
	{
		Keybinds[0][s].PrimaryKey = NumberKeys[s];
		Keybinds[0][s].bRequiresAlt = false;
	}

	// Row 1: Alt + 1-9
	for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
	{
		Keybinds[1][s].PrimaryKey = NumberKeys[s];
		Keybinds[1][s].bRequiresAlt = true;
	}

	// Rows 2-3: unbound by default (already Invalid)
}

void UHotbarSubsystem::ResetKeybindsToDefaults()
{
	InitializeDefaultKeybinds();
	SaveKeybinds();
	UE_LOG(LogHotbar, Log, TEXT("Keybinds reset to defaults"));
}

FHotbarKeybind UHotbarSubsystem::GetKeybind(int32 RowIndex, int32 SlotIndex) const
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW)
		return FHotbarKeybind();
	return Keybinds[RowIndex][SlotIndex];
}

void UHotbarSubsystem::SetKeybind(int32 RowIndex, int32 SlotIndex, const FHotbarKeybind& Keybind)
{
	if (RowIndex < 0 || RowIndex >= NUM_ROWS || SlotIndex < 0 || SlotIndex >= SLOTS_PER_ROW) return;
	Keybinds[RowIndex][SlotIndex] = Keybind;
}

FString UHotbarSubsystem::GetKeybindDisplayString(int32 RowIndex, int32 SlotIndex) const
{
	return GetKeybind(RowIndex, SlotIndex).GetDisplayString();
}

void UHotbarSubsystem::SaveKeybinds()
{
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			FString Key = FString::Printf(TEXT("Row%d_Slot%d"), r, s);
			FString Value = FString::Printf(TEXT("%s,%d,%d,%d"),
				*Keybinds[r][s].PrimaryKey.ToString(),
				Keybinds[r][s].bRequiresAlt ? 1 : 0,
				Keybinds[r][s].bRequiresCtrl ? 1 : 0,
				Keybinds[r][s].bRequiresShift ? 1 : 0);
			GConfig->SetString(TEXT("SabriMMO.HotbarKeybinds"), *Key, *Value, GGameUserSettingsIni);
		}
	}
	GConfig->Flush(false, GGameUserSettingsIni);
	UE_LOG(LogHotbar, Log, TEXT("Keybinds saved to config"));
}

void UHotbarSubsystem::LoadKeybinds()
{
	bool bFoundAny = false;
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			FString Key = FString::Printf(TEXT("Row%d_Slot%d"), r, s);
			FString Value;
			if (GConfig->GetString(TEXT("SabriMMO.HotbarKeybinds"), *Key, Value, GGameUserSettingsIni))
			{
				TArray<FString> Parts;
				Value.ParseIntoArray(Parts, TEXT(","));
				if (Parts.Num() >= 4)
				{
					FKey ParsedKey(*Parts[0]);
					if (ParsedKey.IsValid())
					{
						Keybinds[r][s].PrimaryKey = ParsedKey;
						Keybinds[r][s].bRequiresAlt = FCString::Atoi(*Parts[1]) != 0;
						Keybinds[r][s].bRequiresCtrl = FCString::Atoi(*Parts[2]) != 0;
						Keybinds[r][s].bRequiresShift = FCString::Atoi(*Parts[3]) != 0;
						bFoundAny = true;
					}
				}
			}
		}
	}

	if (bFoundAny)
	{
		UE_LOG(LogHotbar, Log, TEXT("Keybinds loaded from config"));
	}
}

// ============================================================
// Handle number key input from SabriMMOCharacter
// ============================================================

void UHotbarSubsystem::HandleNumberKey(int32 KeyNumber, bool bAlt, bool bCtrl, bool bShift)
{
	if (KeyNumber < 1 || KeyNumber > 9) return;

	// Convert key number to FKey for comparison
	static const FKey NumberKeys[] = {
		EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
		EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
	};
	FKey PressedKey = NumberKeys[KeyNumber - 1];

	// Scan ALL keybinds across all rows and slots to find a match
	// This correctly handles rebinding (e.g., key 5 bound to Row 2 Slot 0)
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			const FHotbarKeybind& KB = Keybinds[r][s];
			if (!KB.IsValid()) continue;

			if (KB.PrimaryKey == PressedKey &&
				KB.bRequiresAlt == bAlt &&
				KB.bRequiresCtrl == bCtrl &&
				KB.bRequiresShift == bShift)
			{
				ActivateSlot(r, s);
				return;
			}
		}
	}
}

// ============================================================
// Refresh item quantities from inventory
// ============================================================

void UHotbarSubsystem::RefreshItemQuantities()
{
	UInventorySubsystem* InvSub = GetWorld() ? GetWorld()->GetSubsystem<UInventorySubsystem>() : nullptr;
	if (!InvSub) return;

	bool bChanged = false;
	for (int32 r = 0; r < NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < SLOTS_PER_ROW; ++s)
		{
			FHotbarSlot& Slot = Slots[r][s];
			if (!Slot.IsItem()) continue;

			FInventoryItem* Item = InvSub->FindItemByInventoryId(Slot.InventoryId);
			if (Item)
			{
				if (Slot.Quantity != Item->Quantity)
				{
					Slot.Quantity = Item->Quantity;
					bChanged = true;
				}
			}
			else
			{
				// Item no longer in inventory — auto-clear the slot
				Slot.Clear();
				Slot.RowIndex = r;
				Slot.SlotIndex = s;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		DataVersion++;
		OnHotbarDataUpdated.Broadcast();
	}
}

// ============================================================
// Icon utilities (delegate to other subsystems)
// ============================================================

FSlateBrush* UHotbarSubsystem::GetItemIconBrush(const FString& IconName)
{
	if (UInventorySubsystem* InvSub = GetWorld()->GetSubsystem<UInventorySubsystem>())
	{
		return InvSub->GetOrCreateItemIconBrush(IconName);
	}
	return nullptr;
}

FSlateBrush* UHotbarSubsystem::GetSkillIconBrush(const FString& IconName)
{
	if (USkillTreeSubsystem* SkillSub = GetWorld()->GetSubsystem<USkillTreeSubsystem>())
	{
		FString ContentPath = SkillSub->ResolveIconContentPath(IconName);
		return SkillSub->GetOrCreateIconBrush(ContentPath);
	}
	return nullptr;
}
