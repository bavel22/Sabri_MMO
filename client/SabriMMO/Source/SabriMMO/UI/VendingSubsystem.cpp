// VendingSubsystem.cpp — Manages vending shop setup (merchant) and browse (buyer).
// Listens for vending:* socket events. Uses CartSubsystem for setup item data.

#include "VendingSubsystem.h"
#include "SVendingSetupPopup.h"
#include "SVendingBrowsePopup.h"
#include "CartSubsystem.h"
#include "NameTagSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY_STATIC(LogVending, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UVendingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UVendingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	Router->RegisterHandler(TEXT("vending:started"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingStarted(D); });
	Router->RegisterHandler(TEXT("vending:item_list"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingItemList(D); });
	Router->RegisterHandler(TEXT("vending:buy_result"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingBuyResult(D); });
	Router->RegisterHandler(TEXT("vending:sold"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingSold(D); });
	Router->RegisterHandler(TEXT("vending:shop_closed"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingShopClosed(D); });
	Router->RegisterHandler(TEXT("vending:error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingError(D); });
	Router->RegisterHandler(TEXT("vending:setup"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleVendingSetup(D); });

	UE_LOG(LogVending, Log, TEXT("VendingSubsystem initialized — 7 event handlers registered"));
}

void UVendingSubsystem::Deinitialize()
{
	HideSetupPopup();
	HideBrowsePopup();

	UWorld* World = GetWorld();
	if (World)
	{
		UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
		if (GI)
		{
			USocketEventRouter* Router = GI->GetEventRouter();
			if (Router) Router->UnregisterAllForOwner(this);
		}
	}

	Super::Deinitialize();
}

// ============================================================
// Socket Handlers
// ============================================================

void UVendingSubsystem::HandleVendingStarted(const TSharedPtr<FJsonValue>& Data)
{
	bIsVending = true;
	HideSetupPopup();

	// Parse shop title from server response if available
	if (Data.IsValid())
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (Data->TryGetObject(ObjPtr) && ObjPtr)
		{
			FString Title;
			if ((*ObjPtr)->TryGetStringField(TEXT("title"), Title)) OwnShopTitle = Title;
		}
	}

	UE_LOG(LogVending, Log, TEXT("Vending shop opened successfully"));

	// Show shop sign above local player's head + lock movement
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
			{
				// Show shop sign
				if (UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>())
				{
					NTS->SetVendingTitle(Char, OwnShopTitle);
				}
				// Lock movement (RO Classic: vendor cannot move while shop is open)
				if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
				{
					CMC->DisableMovement();
				}
			}
		}
	}

	// Show vendor's own shop view (read-only browse popup with sale log)
	// Populate OwnShopItems from the setup items that were sent to the server
	BrowseVendorName = TEXT("Your Shop");
	BrowseShopTitle = OwnShopTitle;
	bBrowseIsOwnShop = true;
	BrowseItems = OwnShopItems;  // Copy vendor's items to browse view
	ShowBrowsePopup();
	if (BrowsePopup.IsValid())
	{
		BrowsePopup->bIsVendorView = true;
		BrowsePopup->RebuildItemList();
	}
}

void UVendingSubsystem::HandleVendingItemList(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse vendor info
	double VId = 0;
	Obj->TryGetNumberField(TEXT("vendorId"), VId);
	BrowseVendorId = (int32)VId;

	Obj->TryGetStringField(TEXT("vendorName"), BrowseVendorName);
	Obj->TryGetStringField(TEXT("shopTitle"), BrowseShopTitle);

	double Zeny = 0;
	Obj->TryGetNumberField(TEXT("playerZeny"), Zeny);
	PlayerZeny = (int32)Zeny;

	// Parse items array
	BrowseItems.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
	{
		for (const auto& Val : *ItemsArr)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (!Val->TryGetObject(ItemObj) || !ItemObj) continue;
			const TSharedPtr<FJsonObject>& It = *ItemObj;

			FVendBrowseItem Item;
			double d;
			FString s;
			if (It->TryGetNumberField(TEXT("vendItemId"), d)) Item.VendItemId = (int32)d;
			if (It->TryGetNumberField(TEXT("itemId"), d)) Item.ItemId = (int32)d;
			if (It->TryGetStringField(TEXT("name"), s)) Item.Name = s;
			if (It->TryGetStringField(TEXT("icon"), s)) Item.Icon = s;
			if (It->TryGetNumberField(TEXT("amount"), d)) Item.Amount = (int32)d;
			if (It->TryGetNumberField(TEXT("price"), d)) Item.Price = (int32)d;
			if (It->TryGetNumberField(TEXT("weight"), d)) Item.Weight = (int32)d;
			if (It->TryGetNumberField(TEXT("refineLevel"), d)) Item.RefineLevel = (int32)d;
			if (It->TryGetNumberField(TEXT("slots"), d)) Item.Slots = (int32)d;
			if (It->TryGetStringField(TEXT("itemType"), s)) Item.ItemType = s;
			BrowseItems.Add(Item);
		}
	}

	UE_LOG(LogVending, Log, TEXT("Received vending item list: %d items from %s (%s)"),
		BrowseItems.Num(), *BrowseVendorName, *BrowseShopTitle);

	ShowBrowsePopup();
}

void UVendingSubsystem::HandleVendingBuyResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bSuccess = false;
	Obj->TryGetBoolField(TEXT("success"), bSuccess);
	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	if (bSuccess)
	{
		UE_LOG(LogVending, Log, TEXT("Vending purchase success: %s"), *Message);
		HideBrowsePopup();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
				FString::Printf(TEXT("Purchase: %s"), *Message));
		}
	}
	else
	{
		UE_LOG(LogVending, Warning, TEXT("Vending purchase failed: %s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
				FString::Printf(TEXT("Purchase failed: %s"), *Message));
		}
	}
}

void UVendingSubsystem::HandleVendingSold(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString ItemName;
	Obj->TryGetStringField(TEXT("itemName"), ItemName);
	double Amount = 0, Price = 0;
	Obj->TryGetNumberField(TEXT("amount"), Amount);
	Obj->TryGetNumberField(TEXT("price"), Price);

	FString BuyerName;
	Obj->TryGetStringField(TEXT("buyerName"), BuyerName);
	double VendItemIdD = 0, RemainingD = 0;
	Obj->TryGetNumberField(TEXT("vendItemId"), VendItemIdD);
	Obj->TryGetNumberField(TEXT("remainingAmount"), RemainingD);
	int32 VendItemId = (int32)VendItemIdD;
	int32 Remaining = (int32)RemainingD;

	UE_LOG(LogVending, Log, TEXT("Vending sale: %s bought %s x%d for %dz (remaining=%d)"),
		*BuyerName, *ItemName, (int32)Amount, (int32)Price, Remaining);

	// Update own shop items
	// Try VendItemId first (matches if server sent real DB ID), fall back to name match
	bool bFound = false;
	for (FVendBrowseItem& OwnItem : OwnShopItems)
	{
		if ((VendItemId > 0 && OwnItem.VendItemId == VendItemId) || (!bFound && OwnItem.Name == ItemName && OwnItem.Amount > 0))
		{
			OwnItem.Amount = Remaining;
			bFound = true;
			break;
		}
	}

	// Update vendor's browse popup live (sale log + rebuild item list from updated OwnShopItems)
	if (BrowsePopup.IsValid() && bBrowseVisible && bBrowseIsOwnShop)
	{
		BrowsePopup->RebuildItemList();
		BrowsePopup->AddSaleMessage(BuyerName, ItemName, (int32)Amount, (int32)Price);
	}
}

void UVendingSubsystem::HandleVendingShopClosed(const TSharedPtr<FJsonValue>& Data)
{
	bIsVending = false;
	bBrowseIsOwnShop = false;
	OwnShopItems.Empty();
	OwnShopTitle.Empty();
	HideSetupPopup();
	HideBrowsePopup();

	// Remove shop sign from local player + unlock movement
	if (UWorld* World = GetWorld())
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			if (ACharacter* Char = Cast<ACharacter>(PC->GetPawn()))
			{
				if (UNameTagSubsystem* NTS = World->GetSubsystem<UNameTagSubsystem>())
				{
					NTS->SetVendingTitle(Char, FString());
				}
				if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
				{
					CMC->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}

	UE_LOG(LogVending, Log, TEXT("Vending shop closed"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, TEXT("Vending shop closed."));
	}
}

void UVendingSubsystem::HandleVendingError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);

	UE_LOG(LogVending, Warning, TEXT("Vending error: %s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red,
			FString::Printf(TEXT("Vending: %s"), *Message));
	}
}

void UVendingSubsystem::HandleVendingSetup(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Slots = 3;
	Obj->TryGetNumberField(TEXT("maxSlots"), Slots);

	UE_LOG(LogVending, Log, TEXT("Server sent vending:setup with maxSlots=%d"), (int32)Slots);
	OpenVendingSetup((int32)Slots);
}

// ============================================================
// Public API
// ============================================================

void UVendingSubsystem::OpenVendingSetup(int32 MaxSlots)
{
	VendingMaxSlots = FMath::Clamp(MaxSlots, 1, 12);

	UE_LOG(LogVending, Log, TEXT("OpenVendingSetup: maxSlots=%d"), VendingMaxSlots);
	ShowSetupPopup();
}

void UVendingSubsystem::CloseVendingSetup()
{
	HideSetupPopup();
}

void UVendingSubsystem::StartVending(const FString& Title, const TArray<FVendSetupItem>& Items)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	// Build JSON payload: { title, items: [ { cartId, amount, price } ] }
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("title"), Title);

	TArray<TSharedPtr<FJsonValue>> ItemsArray;
	for (const FVendSetupItem& Item : Items)
	{
		TSharedPtr<FJsonObject> ItemObj = MakeShared<FJsonObject>();
		ItemObj->SetNumberField(TEXT("cartId"), Item.CartId);
		ItemObj->SetNumberField(TEXT("amount"), Item.Quantity);
		ItemObj->SetNumberField(TEXT("price"), Item.Price);
		ItemsArray.Add(MakeShared<FJsonValueObject>(ItemObj));
	}
	Payload->SetArrayField(TEXT("items"), ItemsArray);

	// Cache vendor's own items for the self-view shop window
	OwnShopTitle = Title;
	OwnShopItems.Empty();
	for (const FVendSetupItem& Item : Items)
	{
		FVendBrowseItem BI;
		BI.VendItemId = Item.CartId;  // Will be updated by server, but use CartId as temporary key
		BI.ItemId = Item.ItemId;
		BI.Name = Item.Name;
		BI.Icon = Item.Icon;
		BI.Amount = Item.Quantity;
		BI.Price = Item.Price;
		BI.Weight = Item.Weight;
		OwnShopItems.Add(BI);
	}

	GI->EmitSocketEvent(TEXT("vending:start"), Payload);

	UE_LOG(LogVending, Log, TEXT("Emitted vending:start — title='%s', %d items"), *Title, Items.Num());
}

void UVendingSubsystem::StopVending()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	GI->EmitSocketEvent(TEXT("vending:close"), Payload);

	UE_LOG(LogVending, Log, TEXT("Emitted vending:close"));
}

void UVendingSubsystem::BrowseShop(int32 VendorId)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("vendorId"), VendorId);
	GI->EmitSocketEvent(TEXT("vending:browse"), Payload);

	UE_LOG(LogVending, Log, TEXT("Emitted vending:browse vendorId=%d"), VendorId);
}

void UVendingSubsystem::CloseBrowse()
{
	HideBrowsePopup();
}

void UVendingSubsystem::BuyItem(int32 VendorId, int32 VendItemId, int32 Amount)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return;

	// Build payload: { vendorId, items: [ { vendItemId, amount } ] }
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("vendorId"), VendorId);

	TArray<TSharedPtr<FJsonValue>> ItemsArray;
	TSharedPtr<FJsonObject> ItemObj = MakeShared<FJsonObject>();
	ItemObj->SetNumberField(TEXT("vendItemId"), VendItemId);
	ItemObj->SetNumberField(TEXT("amount"), Amount);
	ItemsArray.Add(MakeShared<FJsonValueObject>(ItemObj));
	Payload->SetArrayField(TEXT("items"), ItemsArray);

	GI->EmitSocketEvent(TEXT("vending:buy"), Payload);

	UE_LOG(LogVending, Log, TEXT("Emitted vending:buy vendorId=%d vendItemId=%d amount=%d"),
		VendorId, VendItemId, Amount);
}

// ============================================================
// Popup Lifecycle — Setup
// ============================================================

void UVendingSubsystem::ShowSetupPopup()
{
	HideSetupPopup();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	// Build setup items from cart
	TArray<FVendSetupItem> CartSetupItems;
	UCartSubsystem* CartSub = World->GetSubsystem<UCartSubsystem>();
	if (CartSub && CartSub->bHasCart)
	{
		for (const FInventoryItem& CI : CartSub->CartItems)
		{
			FVendSetupItem SI;
			SI.CartId = CI.InventoryId; // cart_id maps to InventoryId
			SI.ItemId = CI.ItemId;
			SI.Name = CI.Name;
			SI.Icon = CI.Icon;
			SI.Quantity = CI.Quantity;
			SI.MaxQuantity = CI.Quantity;
			SI.Price = 0;
			SI.Weight = CI.Weight;
			CartSetupItems.Add(SI);
		}
	}

	SetupPopup = SNew(SVendingSetupPopup)
		.Subsystem(this)
		.CartItems(CartSetupItems)
		.MaxSlots(VendingMaxSlots);

	SetupAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			SetupPopup.ToSharedRef()
		];

	SetupOverlay = SNew(SWeakWidget).PossiblyNullContent(SetupAlignWrapper);
	VC->AddViewportWidgetContent(SetupOverlay.ToSharedRef(), 24);
	bSetupVisible = true;

	FSlateApplication::Get().SetKeyboardFocus(SetupPopup);

	UE_LOG(LogVending, Log, TEXT("Vending setup popup shown — %d cart items, maxSlots=%d (Z=24)"),
		CartSetupItems.Num(), VendingMaxSlots);
}

void UVendingSubsystem::HideSetupPopup()
{
	if (!bSetupVisible) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* VC = World->GetGameViewport();
		if (VC && SetupOverlay.IsValid())
		{
			VC->RemoveViewportWidgetContent(SetupOverlay.ToSharedRef());
		}
	}

	SetupPopup.Reset();
	SetupAlignWrapper.Reset();
	SetupOverlay.Reset();
	bSetupVisible = false;

	UE_LOG(LogVending, Log, TEXT("Vending setup popup hidden"));
}

// ============================================================
// Popup Lifecycle — Browse
// ============================================================

void UVendingSubsystem::ShowBrowsePopup()
{
	HideBrowsePopup();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	BrowsePopup = SNew(SVendingBrowsePopup)
		.Subsystem(this);

	BrowseAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			BrowsePopup.ToSharedRef()
		];

	BrowseOverlay = SNew(SWeakWidget).PossiblyNullContent(BrowseAlignWrapper);
	VC->AddViewportWidgetContent(BrowseOverlay.ToSharedRef(), 24);
	bBrowseVisible = true;

	FSlateApplication::Get().SetKeyboardFocus(BrowsePopup);

	UE_LOG(LogVending, Log, TEXT("Vending browse popup shown — %d items from %s (Z=24)"),
		BrowseItems.Num(), *BrowseVendorName);
}

void UVendingSubsystem::HideBrowsePopup()
{
	if (!bBrowseVisible) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* VC = World->GetGameViewport();
		if (VC && BrowseOverlay.IsValid())
		{
			VC->RemoveViewportWidgetContent(BrowseOverlay.ToSharedRef());
		}
	}

	BrowsePopup.Reset();
	BrowseAlignWrapper.Reset();
	BrowseOverlay.Reset();
	bBrowseVisible = false;

	UE_LOG(LogVending, Log, TEXT("Vending browse popup hidden"));
}
