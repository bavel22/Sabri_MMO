// StorageSubsystem.cpp — Manages Kafra Storage data and the SStorageWidget overlay lifecycle.
// Storage is account-shared (300 slots, 40z access fee).
// Registers socket event handlers via persistent EventRouter.

#include "StorageSubsystem.h"
#include "SStorageWidget.h"
#include "ChatSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogStorage, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UStorageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UStorageSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("storage:opened"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStorageOpened(D); });
		Router->RegisterHandler(TEXT("storage:closed"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStorageClosed(D); });
		Router->RegisterHandler(TEXT("storage:updated"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStorageUpdated(D); });
		Router->RegisterHandler(TEXT("storage:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleStorageError(D); });
	}

	UE_LOG(LogStorage, Log, TEXT("[Storage] Events registered via EventRouter."));
}

void UStorageSubsystem::Deinitialize()
{
	if (bWidgetVisible)
	{
		HideWidget();
	}

	StorageItems.Empty();
	bIsOpen = false;

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

	Super::Deinitialize();
}

// ============================================================
// Event handlers
// ============================================================

void UStorageSubsystem::HandleStorageOpened(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("usedSlots"), Val)) UsedSlots = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxSlots"), Val)) MaxSlots = (int32)Val;

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		ParseStorageItemsFromArray(ItemsArray);
	}

	bIsOpen = true;
	++DataVersion;
	ShowWidget();

	UE_LOG(LogStorage, Log, TEXT("Storage opened: %d/%d slots (v%u)"), UsedSlots, MaxSlots, DataVersion);
}

void UStorageSubsystem::HandleStorageClosed(const TSharedPtr<FJsonValue>& Data)
{
	bIsOpen = false;
	StorageItems.Empty();
	++DataVersion;
	HideWidget();
	UE_LOG(LogStorage, Log, TEXT("Storage closed."));
}

void UStorageSubsystem::HandleStorageUpdated(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("usedSlots"), Val)) UsedSlots = (int32)Val;

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		ParseStorageItemsFromArray(ItemsArray);
	}

	++DataVersion;
	UE_LOG(LogStorage, Log, TEXT("Storage updated: %d/%d slots (v%u)"), UsedSlots, MaxSlots, DataVersion);
}

void UStorageSubsystem::HandleStorageError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	(*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
	UE_LOG(LogStorage, Warning, TEXT("[Storage] Error: %s"), *Msg);

	// Show error in chat so the user sees it
	if (UWorld* World = GetWorld())
	{
		if (UChatSubsystem* ChatSub = World->GetSubsystem<UChatSubsystem>())
		{
			ChatSub->AddCombatLogMessage(FString::Printf(TEXT("[Storage] %s"), *Msg));
		}
	}
}

// ============================================================
// JSON parsing (mirrors CartSubsystem::ParseCartItemFromJson
// but maps storage_id -> InventoryId)
// ============================================================

void UStorageSubsystem::ParseStorageItemsFromArray(const TArray<TSharedPtr<FJsonValue>>* ItemsArray)
{
	StorageItems.Empty();
	if (!ItemsArray) return;

	for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (ItemVal->TryGetObject(ItemObj) && ItemObj)
		{
			StorageItems.Add(ParseStorageItemFromJson(*ItemObj));
		}
	}
}

FInventoryItem UStorageSubsystem::ParseStorageItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FInventoryItem Item;
	if (!Obj.IsValid()) return Item;

	double Val = 0;

	// storageId maps to InventoryId so existing tooltip/inspect systems work
	if (Obj->TryGetNumberField(TEXT("storageId"), Val)) Item.InventoryId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("itemId"), Val)) Item.ItemId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("quantity"), Val)) Item.Quantity = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("slotIndex"), Val)) Item.SlotIndex = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weight"), Val)) Item.Weight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("price"), Val)) Item.Price = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("buyPrice"), Val)) Item.BuyPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("sellPrice"), Val)) Item.SellPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("atk"), Val)) Item.ATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("def"), Val)) Item.DEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("matk"), Val)) Item.MATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("mdef"), Val)) Item.MDEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("strBonus"), Val)) Item.StrBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("agiBonus"), Val)) Item.AgiBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("vitBonus"), Val)) Item.VitBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("intBonus"), Val)) Item.IntBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("dexBonus"), Val)) Item.DexBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("lukBonus"), Val)) Item.LukBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxHpBonus"), Val)) Item.MaxHPBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxSpBonus"), Val)) Item.MaxSPBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("hitBonus"), Val)) Item.HitBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("fleeBonus"), Val)) Item.FleeBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("criticalBonus"), Val)) Item.CriticalBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("perfectDodgeBonus"), Val)) Item.PerfectDodgeBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("requiredLevel"), Val)) Item.RequiredLevel = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxStack"), Val)) Item.MaxStack = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("slots"), Val)) Item.Slots = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weaponLevel"), Val)) Item.WeaponLevel = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("refineLevel"), Val)) Item.RefineLevel = (int32)Val;

	bool bBool = false;
	if (Obj->TryGetBoolField(TEXT("stackable"), bBool)) Item.bStackable = bBool;
	if (Obj->TryGetBoolField(TEXT("refineable"), bBool)) Item.bRefineable = bBool;
	if (Obj->TryGetBoolField(TEXT("twoHanded"), bBool)) Item.bTwoHanded = bBool;
	if (Obj->TryGetBoolField(TEXT("identified"), bBool)) Item.bIdentified = bBool;

	FString Str;
	if (Obj->TryGetStringField(TEXT("name"), Str)) Item.Name = Str;
	if (Obj->TryGetStringField(TEXT("description"), Str)) Item.Description = Str;
	if (Obj->TryGetStringField(TEXT("fullDescription"), Str)) Item.FullDescription = Str;
	if (Obj->TryGetStringField(TEXT("itemType"), Str)) Item.ItemType = Str;
	if (Obj->TryGetStringField(TEXT("equipSlot"), Str)) Item.EquipSlot = Str;
	if (Obj->TryGetStringField(TEXT("icon"), Str)) Item.Icon = Str;
	if (Obj->TryGetStringField(TEXT("weaponType"), Str)) Item.WeaponType = Str;
	if (Obj->TryGetStringField(TEXT("jobsAllowed"), Str)) Item.JobsAllowed = Str;
	if (Obj->TryGetStringField(TEXT("cardType"), Str)) Item.CardType = Str;
	if (Obj->TryGetStringField(TEXT("cardPrefix"), Str)) Item.CardPrefix = Str;
	if (Obj->TryGetStringField(TEXT("cardSuffix"), Str)) Item.CardSuffix = Str;
	if (Obj->TryGetStringField(TEXT("element"), Str)) Item.Element = Str;
	if (Obj->TryGetStringField(TEXT("subType"), Str)) Item.WeaponType = Str;

	// Parse compounded cards array
	const TArray<TSharedPtr<FJsonValue>>* CardsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("compoundedCards"), CardsArray) && CardsArray)
	{
		for (const TSharedPtr<FJsonValue>& CardVal : *CardsArray)
		{
			if (!CardVal.IsValid() || CardVal->IsNull())
				Item.CompoundedCards.Add(-1);
			else
				Item.CompoundedCards.Add((int32)CardVal->AsNumber());
		}
	}

	return Item;
}

// ============================================================
// Tab filtering
// ============================================================

TArray<FInventoryItem> UStorageSubsystem::GetFilteredItems() const
{
	TArray<FInventoryItem> Filtered;
	for (const FInventoryItem& Item : StorageItems)
	{
		// Tab filter
		bool bPassTab = (CurrentTab == 0); // All tab
		if (!bPassTab)
		{
			switch (CurrentTab)
			{
			case 1: bPassTab = (Item.ItemType == TEXT("consumable")); break;
			case 2: bPassTab = (Item.ItemType == TEXT("weapon") || Item.ItemType == TEXT("armor")); break;
			case 3: bPassTab = (Item.ItemType == TEXT("etc") || Item.ItemType == TEXT("ammo") || Item.ItemType == TEXT("card")); break;
			}
		}
		if (!bPassTab) continue;

		// Search filter
		if (!SearchFilter.IsEmpty())
		{
			if (!Item.Name.Contains(SearchFilter, ESearchCase::IgnoreCase))
				continue;
		}

		Filtered.Add(Item);
	}
	return Filtered;
}

// ============================================================
// Item operations (emit to server)
// ============================================================

void UStorageSubsystem::RequestOpen()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogStorage, Warning, TEXT("RequestOpen: GameInstance is null!"));
		return;
	}
	if (!GI->IsSocketConnected())
	{
		UE_LOG(LogStorage, Warning, TEXT("RequestOpen: Socket not connected!"));
		return;
	}
	UE_LOG(LogStorage, Log, TEXT("RequestOpen: Emitting storage:open"));
	GI->EmitSocketEvent(TEXT("storage:open"), TEXT("{}"));
}

void UStorageSubsystem::RequestClose()
{
	UAudioSubsystem::PlayUICancelStatic(GetWorld());
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;
	bIsOpen = false;
	GI->EmitSocketEvent(TEXT("storage:close"), TEXT("{}"));
	HideWidget();
}

void UStorageSubsystem::DepositItem(int32 InventoryId, int32 Quantity)
{
	// Storage deposit reuses the equip kachunk sound (item slides into a slot)
	UAudioSubsystem::PlayEquipStatic(GetWorld());
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("storage:deposit"), Payload);
}

void UStorageSubsystem::WithdrawItem(int32 StorageId, int32 Quantity)
{
	UAudioSubsystem::PlayEquipStatic(GetWorld());
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("storageId"), StorageId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("storage:withdraw"), Payload);
}

void UStorageSubsystem::DepositFromCart(int32 CartId, int32 Quantity)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("cartId"), CartId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("storage:cart_deposit"), Payload);
}

void UStorageSubsystem::WithdrawToCart(int32 StorageId, int32 Quantity)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("storageId"), StorageId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("storage:cart_withdraw"), Payload);
}

void UStorageSubsystem::SortStorage(const FString& SortBy)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("sortBy"), SortBy);
	GI->EmitSocketEvent(TEXT("storage:sort"), Payload);
}

// ============================================================
// Item lookup
// ============================================================

FInventoryItem* UStorageSubsystem::FindItemByStorageId(int32 StorageId)
{
	for (FInventoryItem& Item : StorageItems)
	{
		if (Item.InventoryId == StorageId)
			return &Item;
	}
	return nullptr;
}

// ============================================================
// Widget lifecycle (viewport overlay at Z=21)
// ============================================================

void UStorageSubsystem::ShowWidget()
{
	if (bWidgetVisible) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	StorageWidget = SNew(SStorageWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			StorageWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 21);
	bWidgetVisible = true;

	UE_LOG(LogStorage, Log, TEXT("Storage widget shown (Z=21)."));
}

void UStorageSubsystem::HideWidget()
{
	if (!bWidgetVisible) return;

	UWorld* World = GetWorld();
	UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
	if (ViewportClient && ViewportOverlay.IsValid())
	{
		ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}
	StorageWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetVisible = false;

	UE_LOG(LogStorage, Log, TEXT("Storage widget hidden."));
}

bool UStorageSubsystem::IsWidgetVisible() const
{
	return bWidgetVisible;
}
