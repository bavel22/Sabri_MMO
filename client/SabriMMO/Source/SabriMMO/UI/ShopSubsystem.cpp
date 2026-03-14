// ShopSubsystem.cpp — Manages NPC shop state, shopping cart, batch buy/sell,
// and SShopWidget overlay lifecycle.

#include "ShopSubsystem.h"
#include "SShopWidget.h"
#include "InventorySubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogShop, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UShopSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UShopSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("shop:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopData(D); });
		Router->RegisterHandler(TEXT("shop:bought"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopBought(D); });
		Router->RegisterHandler(TEXT("shop:sold"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopSold(D); });
		Router->RegisterHandler(TEXT("shop:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleShopError(D); });
	}

	UE_LOG(LogShop, Log, TEXT("ShopSubsystem started — events registered via EventRouter."));
}

void UShopSubsystem::Deinitialize()
{
	HideWidget();

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

FShopItem UShopSubsystem::ParseShopItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FShopItem Item;
	if (!Obj.IsValid()) return Item;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("itemId"), Val)) Item.ItemId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("buyPrice"), Val)) Item.BuyPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("sellPrice"), Val)) Item.SellPrice = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weight"), Val)) Item.Weight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("atk"), Val)) Item.ATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("def"), Val)) Item.DEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("matk"), Val)) Item.MATK = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("mdef"), Val)) Item.MDEF = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weaponRange"), Val)) Item.WeaponRange = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("aspdModifier"), Val)) Item.ASPDModifier = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("requiredLevel"), Val)) Item.RequiredLevel = (int32)Val;
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
	if (Obj->TryGetNumberField(TEXT("slots"), Val)) Item.Slots = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("weaponLevel"), Val)) Item.WeaponLevel = (int32)Val;

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

	bool bBool = false;
	if (Obj->TryGetBoolField(TEXT("stackable"), bBool)) Item.bStackable = bBool;
	if (Obj->TryGetBoolField(TEXT("refineable"), bBool)) Item.bRefineable = bBool;
	if (Obj->TryGetBoolField(TEXT("twoHanded"), bBool)) Item.bTwoHanded = bBool;

	return Item;
}

void UShopSubsystem::HandleShopData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("shopId"), Val)) CurrentShopId = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("playerZuzucoin"), Val)) PlayerZuzucoin = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("discountPercent"), Val)) DiscountPercent = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("overchargePercent"), Val)) OverchargePercent = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("currentWeight"), Val)) CurrentWeight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxWeight"), Val)) MaxWeight = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("usedSlots"), Val)) UsedSlots = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("maxSlots"), Val)) MaxSlots = (int32)Val;

	FString Str;
	if (Obj->TryGetStringField(TEXT("shopName"), Str)) ShopName = Str;

	// Parse shop items
	ShopItems.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArr) && ItemsArr)
	{
		for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArr)
		{
			const TSharedPtr<FJsonObject>* ItemObjPtr = nullptr;
			if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObjPtr) && ItemObjPtr)
			{
				FShopItem SI = ParseShopItemFromJson(*ItemObjPtr);
				if (SI.IsValid())
				{
					ShopItems.Add(SI);
				}
			}
		}
	}

	// Clear carts on new shop open
	BuyCart.Empty();
	SellCart.Empty();
	LastErrorMessage.Empty();

	CurrentMode = EShopMode::ModeSelect;
	++DataVersion;

	ShowWidget();

	UE_LOG(LogShop, Log, TEXT("Shop opened: %s (%d items) Zeny=%d Disc=%d%% OC=%d%%"),
		*ShopName, ShopItems.Num(), PlayerZuzucoin, DiscountPercent, OverchargePercent);
}

void UShopSubsystem::HandleShopBought(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("newZuzucoin"), Val)) PlayerZuzucoin = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("totalCost"), Val))
	{
		UE_LOG(LogShop, Log, TEXT("Purchase successful. Cost=%d, Remaining=%d"), (int32)Val, PlayerZuzucoin);
	}

	BuyCart.Empty();
	++DataVersion;
}

void UShopSubsystem::HandleShopSold(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("newZuzucoin"), Val)) PlayerZuzucoin = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("totalRevenue"), Val))
	{
		UE_LOG(LogShop, Log, TEXT("Sale successful. Revenue=%d, Total=%d"), (int32)Val, PlayerZuzucoin);
	}

	SellCart.Empty();
	++DataVersion;
}

void UShopSubsystem::HandleShopError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString Msg;
	if (Obj->TryGetStringField(TEXT("message"), Msg))
	{
		LastErrorMessage = Msg;
		ErrorExpireTime = FPlatformTime::Seconds() + 4.0;
		++DataVersion;
		UE_LOG(LogShop, Warning, TEXT("Shop error: %s"), *Msg);
	}
}

// ============================================================
// Shop operations
// ============================================================

void UShopSubsystem::RequestOpenShop(int32 ShopId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("shopId"), ShopId);

	GI->EmitSocketEvent(TEXT("shop:open"), Payload);
	UE_LOG(LogShop, Log, TEXT("Requesting shop %d"), ShopId);
}

void UShopSubsystem::SubmitBuyCart()
{
	if (BuyCart.Num() == 0) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("shopId"), CurrentShopId);

	TArray<TSharedPtr<FJsonValue>> CartArr;
	for (const FCartItem& CI : BuyCart)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetNumberField(TEXT("itemId"), CI.ItemId);
		Entry->SetNumberField(TEXT("quantity"), CI.Quantity);
		CartArr.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Payload->SetArrayField(TEXT("cart"), CartArr);

	GI->EmitSocketEvent(TEXT("shop:buy_batch"), Payload);
	UE_LOG(LogShop, Log, TEXT("Submitted buy cart: %d items, total %d"), BuyCart.Num(), GetBuyCartTotalCost());
}

void UShopSubsystem::SubmitSellCart()
{
	if (SellCart.Num() == 0) return;
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();

	TArray<TSharedPtr<FJsonValue>> CartArr;
	for (const FCartItem& CI : SellCart)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetNumberField(TEXT("inventoryId"), CI.InventoryId);
		Entry->SetNumberField(TEXT("quantity"), CI.Quantity);
		CartArr.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Payload->SetArrayField(TEXT("cart"), CartArr);

	GI->EmitSocketEvent(TEXT("shop:sell_batch"), Payload);
	UE_LOG(LogShop, Log, TEXT("Submitted sell cart: %d items, total %d"), SellCart.Num(), GetSellCartTotalRevenue());
}

void UShopSubsystem::CloseShop()
{
	CurrentMode = EShopMode::Closed;
	BuyCart.Empty();
	SellCart.Empty();
	ShopItems.Empty();
	LastErrorMessage.Empty();
	++DataVersion;
	HideWidget();
}

void UShopSubsystem::SetMode(EShopMode NewMode)
{
	if (NewMode == EShopMode::Closed)
	{
		CloseShop();
		return;
	}
	CurrentMode = NewMode;
	if (NewMode == EShopMode::SellMode)
	{
		SellCart.Empty();
	}
	else if (NewMode == EShopMode::BuyMode)
	{
		BuyCart.Empty();
	}
	++DataVersion;
}

// ============================================================
// Buy cart management
// ============================================================

int32 UShopSubsystem::GetBuyCartTotalCost() const
{
	int32 Total = 0;
	for (const FCartItem& CI : BuyCart)
	{
		Total += CI.GetTotalPrice();
	}
	return Total;
}

int32 UShopSubsystem::GetBuyCartTotalWeight() const
{
	int32 Total = 0;
	for (const FCartItem& CI : BuyCart)
	{
		Total += CI.GetTotalWeight();
	}
	return Total;
}

void UShopSubsystem::AddToBuyCart(const FShopItem& Item, int32 Quantity)
{
	if (Quantity <= 0 || !Item.IsValid()) return;

	// Check if already in cart — merge
	for (FCartItem& CI : BuyCart)
	{
		if (CI.ItemId == Item.ItemId)
		{
			CI.Quantity += Quantity;
			++DataVersion;
			return;
		}
	}

	// New cart entry
	FCartItem CI;
	CI.ItemId = Item.ItemId;
	CI.Name = Item.Name;
	CI.Icon = Item.Icon;
	CI.Quantity = Quantity;
	CI.UnitPrice = Item.BuyPrice;
	CI.Weight = Item.Weight;
	BuyCart.Add(CI);
	++DataVersion;
}

void UShopSubsystem::RemoveFromBuyCart(int32 CartIndex)
{
	if (BuyCart.IsValidIndex(CartIndex))
	{
		BuyCart.RemoveAt(CartIndex);
		++DataVersion;
	}
}

void UShopSubsystem::ClearBuyCart()
{
	BuyCart.Empty();
	++DataVersion;
}

// ============================================================
// Sell cart management
// ============================================================

int32 UShopSubsystem::GetSellCartTotalRevenue() const
{
	int32 Total = 0;
	for (const FCartItem& CI : SellCart)
	{
		Total += CI.GetTotalPrice();
	}
	return Total;
}

void UShopSubsystem::AddToSellCart(const FInventoryItem& Item, int32 Quantity, int32 SellPrice)
{
	if (Quantity <= 0 || !Item.IsValid()) return;

	// Check if already in cart — merge (capped at owned quantity)
	for (FCartItem& CI : SellCart)
	{
		if (CI.InventoryId == Item.InventoryId)
		{
			CI.Quantity = FMath::Min(CI.Quantity + Quantity, Item.Quantity);
			++DataVersion;
			return;
		}
	}

	FCartItem CI;
	CI.InventoryId = Item.InventoryId;
	CI.ItemId = Item.ItemId;
	CI.Name = Item.GetDisplayName();
	CI.Icon = Item.Icon;
	CI.Quantity = FMath::Min(Quantity, Item.Quantity);
	CI.UnitPrice = SellPrice;
	CI.Weight = Item.Weight;
	SellCart.Add(CI);
	++DataVersion;
}

void UShopSubsystem::RemoveFromSellCart(int32 CartIndex)
{
	if (SellCart.IsValidIndex(CartIndex))
	{
		SellCart.RemoveAt(CartIndex);
		++DataVersion;
	}
}

void UShopSubsystem::ClearSellCart()
{
	SellCart.Empty();
	++DataVersion;
}

// ============================================================
// Sell list (filtered inventory)
// ============================================================

TArray<FInventoryItem> UShopSubsystem::GetSellableItems() const
{
	TArray<FInventoryItem> Result;
	UWorld* World = GetWorld();
	if (!World) return Result;

	UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
	if (!InvSub) return Result;

	for (const FInventoryItem& Item : InvSub->Items)
	{
		// Exclude: equipped, price 0, already fully in sell cart
		if (Item.bIsEquipped) continue;
		if (Item.Price <= 0) continue;

		// Check remaining sellable quantity (subtract what's already in sell cart)
		int32 InCartQty = 0;
		for (const FCartItem& CI : SellCart)
		{
			if (CI.InventoryId == Item.InventoryId)
			{
				InCartQty = CI.Quantity;
				break;
			}
		}
		if (InCartQty >= Item.Quantity) continue; // Fully in cart already

		Result.Add(Item);
	}

	return Result;
}

int32 UShopSubsystem::GetSellPrice(const FInventoryItem& Item) const
{
	int32 BasePrice = Item.Price; // item.price from DB = base sell price
	if (OverchargePercent > 0)
	{
		return FMath::FloorToInt32(BasePrice * (100.0 + OverchargePercent) / 100.0);
	}
	return BasePrice;
}

// ============================================================
// Widget lifecycle
// ============================================================

void UShopSubsystem::ShowWidget()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	ShopWidget = SNew(SShopWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			ShopWidget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 18);
	bWidgetAdded = true;

	// RO Classic: Lock player movement while shop is open
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (ACharacter* Char = PC->GetCharacter())
		{
			if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
			{
				MovComp->DisableMovement();
			}
		}
	}

	UE_LOG(LogShop, Log, TEXT("Shop widget shown (Z=18)."));
}

void UShopSubsystem::HideWidget()
{
	if (!bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* ViewportClient = World->GetGameViewport();
		if (ViewportClient && ViewportOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}

	ShopWidget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;

	// RO Classic: Unlock player movement when shop closes
	if (World)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (ACharacter* Char = PC->GetCharacter())
			{
				if (UCharacterMovementComponent* MovComp = Char->GetCharacterMovement())
				{
					MovComp->SetMovementMode(MOVE_Walking);
				}
			}
		}
	}

	UE_LOG(LogShop, Log, TEXT("Shop widget hidden."));
}

bool UShopSubsystem::IsWidgetVisible() const
{
	return bWidgetAdded && CurrentMode != EShopMode::Closed;
}

