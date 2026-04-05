// CartSubsystem.cpp — Manages cart inventory data and the SCartWidget overlay lifecycle.
// Cart is the Merchant-class portable storage (8000 weight capacity).
// Registers socket event handlers via persistent EventRouter.

#include "CartSubsystem.h"
#include "SCartWidget.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogCart, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UCartSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UCartSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("cart:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartData(D); });
		Router->RegisterHandler(TEXT("cart:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartError(D); });
		Router->RegisterHandler(TEXT("cart:equipped"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCartEquipped(D); });
	}

	// Request cart data now that handlers are registered.
	// cart:data sent during player:join arrives before this subsystem exists (level not loaded yet).
	if (GI->IsSocketConnected())
	{
		GI->EmitSocketEvent(TEXT("cart:load"), TEXT("{}"));
	}

	UE_LOG(LogCart, Log, TEXT("[Cart] Events registered via EventRouter."));
}

void UCartSubsystem::Deinitialize()
{
	if (bWidgetVisible)
	{
		HideWidget();
	}

	CartItems.Empty();

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

void UCartSubsystem::HandleCartData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse weight fields
	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("cartWeight"), Val)) CartWeight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("cartMaxWeight"), Val)) CartMaxWeight = (int32)Val;

	// Parse items array
	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		CartItems.Empty();
		for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (ItemVal->TryGetObject(ItemObj) && ItemObj)
			{
				CartItems.Add(ParseCartItemFromJson(*ItemObj));
			}
		}
	}

	bHasCart = true;
	++DataVersion;

	UE_LOG(LogCart, Log, TEXT("Cart updated: %d items, weight=%d/%d (v%u)"),
		CartItems.Num(), CartWeight, CartMaxWeight, DataVersion);
}

void UCartSubsystem::HandleCartEquipped(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bNewHasCart = false;
	Obj->TryGetBoolField(TEXT("hasCart"), bNewHasCart);
	bHasCart = bNewHasCart;

	if (!bHasCart)
	{
		CartItems.Empty();
		CartWeight = 0;
		++DataVersion;
		UE_LOG(LogCart, Log, TEXT("Cart unequipped — items cleared."));
	}
	else
	{
		UE_LOG(LogCart, Log, TEXT("Cart equipped."));
	}
}

void UCartSubsystem::HandleCartError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	(*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
	UE_LOG(LogCart, Warning, TEXT("[Cart] Error: %s"), *Msg);
}

// ============================================================
// JSON parsing (mirrors InventorySubsystem::ParseItemFromJson
// but maps cart_id -> InventoryId)
// ============================================================

FInventoryItem UCartSubsystem::ParseCartItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FInventoryItem Item;
	if (!Obj.IsValid()) return Item;

	double Val = 0;

	// cart_id maps to InventoryId so existing tooltip/inspect systems work
	if (Obj->TryGetNumberField(TEXT("cart_id"), Val)) Item.InventoryId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("item_id"), Val)) Item.ItemId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("quantity"), Val)) Item.Quantity = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("slot_index"), Val)) Item.SlotIndex = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weight"), Val)) Item.Weight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("price"), Val)) Item.Price = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("atk"), Val)) Item.ATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("def"), Val)) Item.DEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("matk"), Val)) Item.MATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("mdef"), Val)) Item.MDEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("str_bonus"), Val)) Item.StrBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("agi_bonus"), Val)) Item.AgiBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("vit_bonus"), Val)) Item.VitBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("int_bonus"), Val)) Item.IntBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("dex_bonus"), Val)) Item.DexBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("luk_bonus"), Val)) Item.LukBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("max_hp_bonus"), Val)) Item.MaxHPBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("max_sp_bonus"), Val)) Item.MaxSPBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("hit_bonus"), Val)) Item.HitBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("flee_bonus"), Val)) Item.FleeBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("critical_bonus"), Val)) Item.CriticalBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("perfect_dodge_bonus"), Val)) Item.PerfectDodgeBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("required_level"), Val)) Item.RequiredLevel = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("aspd_modifier"), Val)) Item.ASPDModifier = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weapon_range"), Val)) Item.WeaponRange = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("max_stack"), Val)) Item.MaxStack = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("buy_price"), Val)) Item.BuyPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("sell_price"), Val)) Item.SellPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("slots"), Val)) Item.Slots = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weapon_level"), Val)) Item.WeaponLevel = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("refine_level"), Val)) Item.RefineLevel = (int32)Val;

	bool bBool = false;
	if (Obj->TryGetBoolField(TEXT("stackable"), bBool)) Item.bStackable = bBool;
	if (Obj->TryGetBoolField(TEXT("refineable"), bBool)) Item.bRefineable = bBool;
	if (Obj->TryGetBoolField(TEXT("two_handed"), bBool)) Item.bTwoHanded = bBool;
	if (Obj->TryGetBoolField(TEXT("identified"), bBool)) Item.bIdentified = bBool;

	FString Str;
	if (Obj->TryGetStringField(TEXT("name"), Str)) Item.Name = Str;
	if (Obj->TryGetStringField(TEXT("description"), Str)) Item.Description = Str;
	if (Obj->TryGetStringField(TEXT("full_description"), Str)) Item.FullDescription = Str;
	if (Obj->TryGetStringField(TEXT("item_type"), Str)) Item.ItemType = Str;
	if (Obj->TryGetStringField(TEXT("equip_slot"), Str)) Item.EquipSlot = Str;
	if (Obj->TryGetStringField(TEXT("icon"), Str)) Item.Icon = Str;
	if (Obj->TryGetStringField(TEXT("weapon_type"), Str)) Item.WeaponType = Str;
	if (Obj->TryGetStringField(TEXT("jobs_allowed"), Str)) Item.JobsAllowed = Str;
	if (Obj->TryGetStringField(TEXT("card_type"), Str)) Item.CardType = Str;
	if (Obj->TryGetStringField(TEXT("card_prefix"), Str)) Item.CardPrefix = Str;
	if (Obj->TryGetStringField(TEXT("card_suffix"), Str)) Item.CardSuffix = Str;
	if (Obj->TryGetStringField(TEXT("element"), Str)) Item.Element = Str;
	if (Obj->TryGetStringField(TEXT("sub_type"), Str)) Item.WeaponType = Str; // sub_type maps to WeaponType for cart

	// Parse compounded cards array
	const TArray<TSharedPtr<FJsonValue>>* CardsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("compounded_cards"), CardsArray) && CardsArray)
	{
		for (const TSharedPtr<FJsonValue>& CardVal : *CardsArray)
		{
			if (!CardVal.IsValid() || CardVal->IsNull())
				Item.CompoundedCards.Add(-1);
			else
				Item.CompoundedCards.Add((int32)CardVal->AsNumber());
		}
	}

	// Parse compounded card details array
	const TArray<TSharedPtr<FJsonValue>>* CardDetailsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("compounded_card_details"), CardDetailsArray) && CardDetailsArray)
	{
		for (const TSharedPtr<FJsonValue>& DetailVal : *CardDetailsArray)
		{
			FCompoundedCardInfo CardInfo;
			if (!DetailVal.IsValid() || DetailVal->IsNull())
			{
				Item.CompoundedCardDetails.Add(CardInfo);
				continue;
			}
			const TSharedPtr<FJsonObject>* DetailObj = nullptr;
			if (DetailVal->TryGetObject(DetailObj) && DetailObj)
			{
				double DVal = 0;
				FString SVal;
				if ((*DetailObj)->TryGetNumberField(TEXT("item_id"), DVal)) CardInfo.ItemId = (int32)DVal;
				if ((*DetailObj)->TryGetStringField(TEXT("name"), SVal)) CardInfo.Name = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("description"), SVal)) CardInfo.Description = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("full_description"), SVal)) CardInfo.FullDescription = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("icon"), SVal)) CardInfo.Icon = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("card_type"), SVal)) CardInfo.CardType = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("card_prefix"), SVal)) CardInfo.CardPrefix = SVal;
				if ((*DetailObj)->TryGetStringField(TEXT("card_suffix"), SVal)) CardInfo.CardSuffix = SVal;
				if ((*DetailObj)->TryGetNumberField(TEXT("weight"), DVal)) CardInfo.Weight = (int32)DVal;
			}
			Item.CompoundedCardDetails.Add(CardInfo);
		}
	}

	return Item;
}

// ============================================================
// Item operations (emit to server)
// ============================================================

void UCartSubsystem::MoveToCart(int32 InventoryId, int32 Amount)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("amount"), Amount);
	GI->EmitSocketEvent(TEXT("cart:move_to_cart"), Payload);
	UE_LOG(LogCart, Log, TEXT("Sent cart:move_to_cart inv_id=%d amount=%d"), InventoryId, Amount);
}

void UCartSubsystem::MoveToInventory(int32 CartId, int32 Amount)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("cartId"), CartId);
	Payload->SetNumberField(TEXT("amount"), Amount);
	GI->EmitSocketEvent(TEXT("cart:move_to_inventory"), Payload);
	UE_LOG(LogCart, Log, TEXT("Sent cart:move_to_inventory cart_id=%d amount=%d"), CartId, Amount);
}

// ============================================================
// Item lookup
// ============================================================

FInventoryItem* UCartSubsystem::FindItemByInventoryId(int32 InvId)
{
	for (FInventoryItem& Item : CartItems)
	{
		if (Item.InventoryId == InvId)
			return &Item;
	}
	return nullptr;
}

// ============================================================
// Widget lifecycle (viewport overlay at Z=14)
// ============================================================

void UCartSubsystem::ShowWidget()
{
	if (bWidgetVisible) return;
	if (!bHasCart) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	CartWidget = SNew(SCartWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			CartWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 14);
	bWidgetVisible = true;

	UE_LOG(LogCart, Log, TEXT("Cart widget shown (Z=14)."));
}

void UCartSubsystem::HideWidget()
{
	if (!bWidgetVisible) return;

	UWorld* World = GetWorld();
	UGameViewportClient* ViewportClient = World ? World->GetGameViewport() : nullptr;
	if (ViewportClient && ViewportOverlay.IsValid())
	{
		ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
	}
	CartWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetVisible = false;

	UE_LOG(LogCart, Log, TEXT("Cart widget hidden."));
}

void UCartSubsystem::ToggleWidget()
{
	if (bWidgetVisible)
	{
		HideWidget();
	}
	else
	{
		ShowWidget();
	}
}

bool UCartSubsystem::IsWidgetVisible() const
{
	return bWidgetVisible;
}
