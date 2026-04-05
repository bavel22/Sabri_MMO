// InventorySubsystem.cpp — Manages inventory data, drag-and-drop state,
// and the SInventoryWidget overlay lifecycle.

#include "InventorySubsystem.h"
#include "SInventoryWidget.h"
#include "SCardCompoundPopup.h"
#include "SIdentifyPopup.h"
#include "HotbarSubsystem.h"
#include "CartSubsystem.h"
#include "StorageSubsystem.h"
#include "TradeSubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SNullWidget.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogInventory, Log, All);

// ============================================================
// SLootNotificationOverlay — OnPaint overlay for loot popups
// ============================================================

class SLootNotificationOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLootNotificationOverlay) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UInventorySubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OwningSubsystem = InArgs._Subsystem;
		SetVisibility(EVisibility::HitTestInvisible);
		ChildSlot[ SNullWidget::NullWidget ];

		RegisterActiveTimer(0.0f,
			FWidgetActiveTimerDelegate::CreateSP(this, &SLootNotificationOverlay::OnRepaintTick));
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		int32 OutLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
			OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		UInventorySubsystem* Sub = OwningSubsystem.Get();
		if (!Sub || Sub->LootNotifications.Num() == 0) return OutLayerId;

		const double Now = FPlatformTime::Seconds();
		const FSlateFontInfo EnemyFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		const FSlateFontInfo ItemFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		// Full-viewport geometry — use DPI-corrected local size
		const FVector2D AreaSize = AllottedGeometry.GetLocalSize();
		if (AreaSize.X < 100.f || AreaSize.Y < 100.f) return OutLayerId;

		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

		static const FLinearColor BgColor(0.15f, 0.10f, 0.05f, 0.85f);
		static const FLinearColor BorderColor(0.60f, 0.48f, 0.22f, 1.0f);
		static const FLinearColor EnemyNameColor(0.95f, 0.82f, 0.48f, 1.0f);
		static const FLinearColor ItemTextColor(0.96f, 0.90f, 0.78f, 1.0f);
		static const FLinearColor ShadowColor(0.0f, 0.0f, 0.0f, 0.9f);

		constexpr float PadX = 8.f;
		constexpr float PadY = 3.f;
		constexpr float EntryGap = 3.f;
		constexpr float BottomMargin = 80.f;   // above hotbar area
		constexpr float RightMargin = 15.f;

		const int32 BoxLayer = OutLayerId + 1;
		const int32 TextLayer = OutLayerId + 2;

		// Start from bottom-right of the actual viewport
		float YBottom = AreaSize.Y - BottomMargin;

		// Draw newest at bottom, oldest at top
		for (int32 i = Sub->LootNotifications.Num() - 1; i >= 0; --i)
		{
			const FLootNotifyEntry& Entry = Sub->LootNotifications[i];
			const float Age = (float)(Now - Entry.SpawnTime);
			if (Age > UInventorySubsystem::LOOT_NOTIFY_DURATION) continue;

			// Fade during last FADE_TIME seconds
			float Alpha = 1.0f;
			const float FadeStart = UInventorySubsystem::LOOT_NOTIFY_DURATION
				- UInventorySubsystem::LOOT_NOTIFY_FADE_TIME;
			if (Age > FadeStart)
			{
				Alpha = 1.0f - (Age - FadeStart) / UInventorySubsystem::LOOT_NOTIFY_FADE_TIME;
			}
			Alpha = FMath::Clamp(Alpha, 0.f, 1.f);
			if (Alpha <= 0.01f) continue;

			// Build item text (always show quantity: "Red Potion x1, Jellopy x3")
			FString ItemsText;
			for (int32 j = 0; j < Entry.Items.Num(); ++j)
			{
				if (j > 0) ItemsText += TEXT(", ");
				ItemsText += FString::Printf(TEXT("%s x%d"), *Entry.Items[j].Name, Entry.Items[j].Quantity);
			}

			const FString EnemyText = Entry.EnemyName + TEXT(": ");
			const FVector2D EnemySize = FontMeasure->Measure(EnemyText, EnemyFont);
			const FVector2D ItemsSize = FontMeasure->Measure(ItemsText, ItemFont);
			const float TotalWidth = EnemySize.X + ItemsSize.X + PadX * 2;
			const float BoxHeight = FMath::Max(EnemySize.Y, ItemsSize.Y) + PadY * 2;

			// Right-aligned within the actual viewport, stacking upward
			YBottom -= BoxHeight;
			const float XOffset = AreaSize.X - TotalWidth - RightMargin;

			// Border box (1px larger)
			FLinearColor BorderAlpha = BorderColor;
			BorderAlpha.A *= Alpha;
			FSlateDrawElement::MakeBox(OutDrawElements, BoxLayer,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TotalWidth + 2, BoxHeight + 2),
					FSlateLayoutTransform(FVector2f(XOffset - 1, YBottom - 1))),
				WhiteBrush, ESlateDrawEffect::None, BorderAlpha);

			// Background
			FLinearColor BgAlpha = BgColor;
			BgAlpha.A *= Alpha;
			FSlateDrawElement::MakeBox(OutDrawElements, BoxLayer + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TotalWidth, BoxHeight),
					FSlateLayoutTransform(FVector2f(XOffset, YBottom))),
				WhiteBrush, ESlateDrawEffect::None, BgAlpha);

			// Text Y centering
			const float TextBaseY = YBottom + PadY;

			// Shadow + enemy name
			FLinearColor ShadAlpha = ShadowColor;
			ShadAlpha.A *= Alpha;
			FSlateDrawElement::MakeText(OutDrawElements, TextLayer,
				AllottedGeometry.ToPaintGeometry(
					EnemySize, FSlateLayoutTransform(FVector2f(XOffset + PadX + 1, TextBaseY + 1))),
				EnemyText, EnemyFont, ESlateDrawEffect::None, ShadAlpha);
			FLinearColor EnemyAlpha = EnemyNameColor;
			EnemyAlpha.A *= Alpha;
			FSlateDrawElement::MakeText(OutDrawElements, TextLayer + 1,
				AllottedGeometry.ToPaintGeometry(
					EnemySize, FSlateLayoutTransform(FVector2f(XOffset + PadX, TextBaseY))),
				EnemyText, EnemyFont, ESlateDrawEffect::None, EnemyAlpha);

			// Shadow + items text
			const float ItemTextX = XOffset + PadX + EnemySize.X;
			FSlateDrawElement::MakeText(OutDrawElements, TextLayer,
				AllottedGeometry.ToPaintGeometry(
					ItemsSize, FSlateLayoutTransform(FVector2f(ItemTextX + 1, TextBaseY + 1))),
				ItemsText, ItemFont, ESlateDrawEffect::None, ShadAlpha);
			FLinearColor ItemAlpha = ItemTextColor;
			ItemAlpha.A *= Alpha;
			FSlateDrawElement::MakeText(OutDrawElements, TextLayer + 1,
				AllottedGeometry.ToPaintGeometry(
					ItemsSize, FSlateLayoutTransform(FVector2f(ItemTextX, TextBaseY))),
				ItemsText, ItemFont, ESlateDrawEffect::None, ItemAlpha);

			YBottom -= EntryGap;
		}

		return TextLayer + 2;
	}

private:
	EActiveTimerReturnType OnRepaintTick(double, float)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
		return EActiveTimerReturnType::Continue;
	}

	TWeakObjectPtr<UInventorySubsystem> OwningSubsystem;
};

// ============================================================
// Lifecycle
// ============================================================

bool UInventorySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UInventorySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalCharacterId = SelChar.CharacterId;

	// Register socket event handlers via persistent EventRouter
	USocketEventRouter* Router = GI->GetEventRouter();
	if (Router)
	{
		Router->RegisterHandler(TEXT("inventory:data"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryData(D); });
		Router->RegisterHandler(TEXT("inventory:equipped"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryEquipped(D); });
		Router->RegisterHandler(TEXT("inventory:dropped"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryDropped(D); });
		Router->RegisterHandler(TEXT("inventory:error"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryError(D); });
		Router->RegisterHandler(TEXT("card:result"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleCardResult(D); });
		Router->RegisterHandler(TEXT("inventory:used"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleItemUsed(D); });
		Router->RegisterHandler(TEXT("loot:drop"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleLootDrop(D); });
		Router->RegisterHandler(TEXT("identify:item_list"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleIdentifyItemList(D); });
		Router->RegisterHandler(TEXT("identify:result"), this,
			[this](const TSharedPtr<FJsonValue>& D) { HandleIdentifyResult(D); });
	}

	// Request fresh inventory data
	GI->EmitSocketEvent(TEXT("inventory:load"), TEXT("{}"));

	// Show loot notification overlay (always visible when in game world)
	if (GI->IsSocketConnected())
	{
		ShowLootOverlay();

		// Periodic cleanup of expired notifications
		InWorld.GetTimerManager().SetTimer(LootCleanupTimer,
			FTimerDelegate::CreateLambda([this]()
			{
				const double Now = FPlatformTime::Seconds();
				LootNotifications.RemoveAll([Now](const FLootNotifyEntry& E)
				{
					return (float)(Now - E.SpawnTime) > LOOT_NOTIFY_DURATION + 0.5f;
				});
			}),
			1.0f, true);
	}

	UE_LOG(LogInventory, Log, TEXT("InventorySubsystem started — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);
}

void UInventorySubsystem::Deinitialize()
{
	HideLootOverlay();
	LootNotifications.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LootCleanupTimer);
	}

	HideCardCompoundPopup();
	HideIdentifyPopup();

	if (bWidgetVisible)
	{
		ToggleWidget();
	}

	HideDragCursor();

	// Clear icon caches — release GC-rooted textures and brush memory
	ItemIconBrushCache.Empty();
	ItemIconTextureCache.Empty();

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

FInventoryItem UInventorySubsystem::ParseItemFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FInventoryItem Item;
	if (!Obj.IsValid()) return Item;

	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("inventory_id"), Val)) Item.InventoryId = (int32)Val;
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
	if (Obj->TryGetBoolField(TEXT("is_equipped"), bBool)) Item.bIsEquipped = bBool;
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
	if (Obj->TryGetStringField(TEXT("equipped_position"), Str)) Item.EquippedPosition = Str;
	if (Obj->TryGetStringField(TEXT("icon"), Str)) Item.Icon = Str;
	if (Obj->TryGetNumberField(TEXT("view_sprite"), Val)) Item.ViewSprite = (int32)Val;
	if (Obj->TryGetStringField(TEXT("weapon_type"), Str)) Item.WeaponType = Str;
	if (Obj->TryGetStringField(TEXT("jobs_allowed"), Str)) Item.JobsAllowed = Str;
	if (Obj->TryGetStringField(TEXT("card_type"), Str)) Item.CardType = Str;
	if (Obj->TryGetStringField(TEXT("card_prefix"), Str)) Item.CardPrefix = Str;
	if (Obj->TryGetStringField(TEXT("card_suffix"), Str)) Item.CardSuffix = Str;
	if (Obj->TryGetStringField(TEXT("element"), Str)) Item.Element = Str;

	// Parse compounded cards array: [null, 4036, null, null]
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

void UInventorySubsystem::HandleInventoryData(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;

	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse zuzucoin
	double Val = 0;
	if (Obj->TryGetNumberField(TEXT("zuzucoin"), Val)) Zuzucoin = (int32)Val;

	// Parse items array
	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		Items.Empty();
		for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (ItemVal->TryGetObject(ItemObj) && ItemObj)
			{
				Items.Add(ParseItemFromJson(*ItemObj));
			}
		}
	}

	RecalculateWeight();
	++DataVersion;

	UE_LOG(LogInventory, Log, TEXT("Inventory updated: %d items, %d zuzucoin, weight=%d/%d (v%u)"),
		Items.Num(), Zuzucoin, CurrentWeight, MaxWeight, DataVersion);
}

void UInventorySubsystem::HandleInventoryEquipped(const TSharedPtr<FJsonValue>& Data)
{
	// Full inventory refresh is also sent via inventory:data, so this is supplementary
	UE_LOG(LogInventory, Log, TEXT("inventory:equipped received"));
}

void UInventorySubsystem::HandleInventoryDropped(const TSharedPtr<FJsonValue>& Data)
{
	UE_LOG(LogInventory, Log, TEXT("inventory:dropped received"));
}

void UInventorySubsystem::HandleInventoryError(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

	FString Msg;
	(*ObjPtr)->TryGetStringField(TEXT("message"), Msg);
	UE_LOG(LogInventory, Warning, TEXT("Inventory error: %s"), *Msg);
}

void UInventorySubsystem::HandleItemUsed(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString ItemName;
	double Healed = 0, SPRestored = 0;
	Obj->TryGetStringField(TEXT("itemName"), ItemName);
	Obj->TryGetNumberField(TEXT("healed"), Healed);
	Obj->TryGetNumberField(TEXT("spRestored"), SPRestored);

	UE_LOG(LogInventory, Log, TEXT("Item used: %s — healed %d, SP restored %d"),
		*ItemName, (int32)Healed, (int32)SPRestored);

	// Request full inventory refresh to update quantities
	// (server also sends inventory:data, but requesting ensures consistency)
	RequestInventoryRefresh();
}

void UInventorySubsystem::HandleLootDrop(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	FString EnemyName;
	Obj->TryGetStringField(TEXT("enemyName"), EnemyName);

	FLootNotifyEntry Notification;
	Notification.EnemyName = EnemyName;
	Notification.SpawnTime = FPlatformTime::Seconds();

	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Obj->TryGetArrayField(TEXT("items"), ItemsArray) && ItemsArray)
	{
		for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
		{
			const TSharedPtr<FJsonObject>* ItemObj = nullptr;
			if (!ItemVal->TryGetObject(ItemObj) || !ItemObj) continue;

			FLootNotifyItem LootItem;
			double Qty = 0;
			(*ItemObj)->TryGetStringField(TEXT("itemName"), LootItem.Name);
			(*ItemObj)->TryGetNumberField(TEXT("quantity"), Qty);
			LootItem.Quantity = (int32)Qty;

			Notification.Items.Add(LootItem);

			UE_LOG(LogInventory, Log, TEXT("Looted %s x%d from %s"),
				*LootItem.Name, LootItem.Quantity, *EnemyName);
		}
	}

	// Add notification (cap to MAX)
	if (LootNotifications.Num() >= MAX_LOOT_NOTIFICATIONS)
	{
		LootNotifications.RemoveAt(0);
	}
	LootNotifications.Add(MoveTemp(Notification));

	// No need to refresh inventory — server already sends inventory:data after loot:drop
}

// ============================================================
// Loot notification overlay lifecycle
// ============================================================

void UInventorySubsystem::ShowLootOverlay()
{
	if (bLootOverlayAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	// Full-viewport overlay (same pattern as WorldHealthBarSubsystem / DamageNumberSubsystem)
	// No alignment wrapper — the widget fills the entire viewport and draws at bottom-right in OnPaint
	LootOverlayWidget = SNew(SLootNotificationOverlay).Subsystem(this);
	LootOverlayViewport = SNew(SWeakWidget).PossiblyNullContent(LootOverlayWidget);
	VC->AddViewportWidgetContent(LootOverlayViewport.ToSharedRef(), 17);
	bLootOverlayAdded = true;
}

void UInventorySubsystem::HideLootOverlay()
{
	if (!bLootOverlayAdded) return;

	UWorld* World = GetWorld();
	UGameViewportClient* VC = World ? World->GetGameViewport() : nullptr;
	if (VC && LootOverlayViewport.IsValid())
	{
		VC->RemoveViewportWidgetContent(LootOverlayViewport.ToSharedRef());
	}

	LootOverlayWidget.Reset();
	LootOverlayViewport.Reset();
	bLootOverlayAdded = false;
}

// ============================================================
// Helpers
// ============================================================

void UInventorySubsystem::RecalculateWeight()
{
	CurrentWeight = 0;
	for (const FInventoryItem& Item : Items)
	{
		CurrentWeight += Item.Weight * Item.Quantity;
	}
	// MaxWeight is based on STR — use a reasonable default, updated from player:stats
	if (MaxWeight <= 0) MaxWeight = 2000;
}

FInventoryItem* UInventorySubsystem::FindItemByInventoryId(int32 InventoryId)
{
	for (FInventoryItem& Item : Items)
	{
		if (Item.InventoryId == InventoryId) return &Item;
	}
	return nullptr;
}

TArray<FInventoryItem> UInventorySubsystem::GetEquippedItems() const
{
	TArray<FInventoryItem> Equipped;
	for (const FInventoryItem& Item : Items)
	{
		if (Item.bIsEquipped) Equipped.Add(Item);
	}
	return Equipped;
}

// ============================================================
// Item icon loading (same pattern as SkillTreeSubsystem)
// ============================================================

FSlateBrush* UInventorySubsystem::GetOrCreateItemIconBrush(const FString& IconName)
{
	if (IconName.IsEmpty()) return nullptr;

	// Check cache first
	if (TSharedPtr<FSlateBrush>* Found = ItemIconBrushCache.Find(IconName))
	{
		return Found->Get();
	}

	// Map server icon field → actual UE5 asset name (filesystem rename breaks .uasset internals)
	static const TMap<FString, FString> IconAssetMap = {
		// Consumables
		{TEXT("red_potion"),     TEXT("CrimsonVial")},
		{TEXT("orange_potion"),  TEXT("AmberElixir")},
		{TEXT("yellow_potion"), TEXT("GoldenSalve")},
		{TEXT("blue_potion"),    TEXT("AzurePhilter")},
		{TEXT("meat"),           TEXT("RoastedHaunch")},
		{TEXT("strawberry"),     TEXT("Strawberry")},
		{TEXT("green_herb"),     TEXT("VerdantLeaf")},
		// Loot / Etc
		{TEXT("jellopy"),        TEXT("GloopyResidue")},
		{TEXT("sticky_mucus"),   TEXT("ViscousSlime")},
		{TEXT("shell"),          TEXT("ChitinShard")},
		{TEXT("feather"),        TEXT("DownyPlume")},
		{TEXT("mushroom_spore"), TEXT("SporeCluster")},
		{TEXT("insect_leg"),     TEXT("BarbedLimb")},
		{TEXT("fluff"),          TEXT("SilkenTuft")},
		// Weapons
		{TEXT("knife"),          TEXT("RusticShiv")},
		{TEXT("cutter"),         TEXT("KeenEdge")},
		{TEXT("main_gauche"),    TEXT("StilettoFang")},
		{TEXT("sword"),          TEXT("IronCleaver")},
		{TEXT("falchion"),       TEXT("CrescentSaber")},
		{TEXT("bow"),            TEXT("HuntingLongbow")},
		// Armor
		{TEXT("cotton_shirt"),   TEXT("LinenTunic")},
		{TEXT("padded_armor"),   TEXT("QuiltedVest")},
		{TEXT("chain_mail"),     TEXT("RingweaveHauberk")},
	};

	// All card items share a single generic card icon (icon names end with "_card")
	if (IconName.EndsWith(TEXT("_card")))
	{
		FString ContentPath = TEXT("/Game/SabriMMO/Assets/Item_Icons/cards/Icon_Card");
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *ContentPath);
		if (Tex)
		{
			Tex->LODGroup = TEXTUREGROUP_UI;
			Tex->Filter = TF_Bilinear;
			Tex->NeverStream = true;
			Tex->UpdateResource();
			ItemIconTextureCache.Add(IconName, Tex);
			TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
			Brush->SetResourceObject(Tex);
			Brush->ImageSize = FVector2D(28.f, 28.f);
			Brush->DrawAs = ESlateBrushDrawType::Image;
			FSlateBrush* RawPtr = Brush.Get();
			ItemIconBrushCache.Add(IconName, Brush);
			UE_LOG(LogInventory, Log, TEXT("Loaded card icon: %s → %s"), *IconName, *ContentPath);
			return RawPtr;
		}
		UE_LOG(LogInventory, Warning, TEXT("Failed to load card icon: %s"), *ContentPath);
		ItemIconBrushCache.Add(IconName, nullptr);
		return nullptr;
	}

	const FString* AssetName = IconAssetMap.Find(IconName);
	FString ContentPath = FString::Printf(TEXT("/Game/SabriMMO/Assets/Item_Icons/Icon_%s"),
		AssetName ? **AssetName : *IconName);

	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *ContentPath);
	if (!Tex)
	{
		UE_LOG(LogInventory, Warning, TEXT("Failed to load item icon: %s"), *ContentPath);
		// Cache null so we don't retry LoadObject every grid rebuild
		ItemIconBrushCache.Add(IconName, nullptr);
		return nullptr;
	}

	// Force UI-quality texture settings so icons stay crisp at small display sizes.
	// Without this, UE5 defaults (DXT compression + mipmap chain + streaming) make
	// 1024px icons look blurry/blocky when rendered at 28px in Slate.
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->Filter = TF_Bilinear;
	Tex->NeverStream = true;
	Tex->UpdateResource();

	// CRITICAL: Store texture in UPROPERTY TMap so GC sees it as referenced.
	// Without this, UTexture2D is only reachable via FSlateBrush (inside a non-UPROPERTY
	// TSharedPtr chain) — invisible to GC — gets garbage collected → crash during Paint.
	ItemIconTextureCache.Add(IconName, Tex);

	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Tex);
	Brush->ImageSize = FVector2D(28.f, 28.f);
	Brush->DrawAs = ESlateBrushDrawType::Image;

	FSlateBrush* RawPtr = Brush.Get();
	ItemIconBrushCache.Add(IconName, Brush);

	UE_LOG(LogInventory, Log, TEXT("Loaded item icon: %s"), *ContentPath);
	return RawPtr;
}

// ============================================================
// Tab filtering
// ============================================================

void UInventorySubsystem::SetTab(int32 Tab)
{
	CurrentTab = FMath::Clamp(Tab, 0, 2);
}

TArray<FInventoryItem> UInventorySubsystem::GetFilteredItems() const
{
	TArray<FInventoryItem> Filtered;
	for (const FInventoryItem& Item : Items)
	{
		if (Item.bIsEquipped) continue; // Equipped items don't show in inventory grid

		// Tab filter
		bool bPassTab = false;
		switch (CurrentTab)
		{
		case 0: bPassTab = (Item.ItemType == TEXT("consumable") || Item.ItemType == TEXT("usable")); break;
		case 1: bPassTab = (!Item.EquipSlot.IsEmpty() || Item.ItemType == TEXT("ammo")); break;
		case 2: bPassTab = (Item.ItemType == TEXT("etc") || Item.ItemType == TEXT("card")); break;
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

void UInventorySubsystem::UseItem(int32 InventoryId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	GI->EmitSocketEvent(TEXT("inventory:use"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:use for inv_id=%d"), InventoryId);
}

void UInventorySubsystem::EquipItem(int32 InventoryId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetBoolField(TEXT("equip"), true);
	GI->EmitSocketEvent(TEXT("inventory:equip"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:equip (equip=true) for inv_id=%d"), InventoryId);
}

void UInventorySubsystem::UnequipItem(int32 InventoryId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetBoolField(TEXT("equip"), false);
	GI->EmitSocketEvent(TEXT("inventory:equip"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:equip (equip=false) for inv_id=%d"), InventoryId);
}

void UInventorySubsystem::DropItem(int32 InventoryId, int32 Quantity)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	if (Quantity > 0) Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("inventory:drop"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:drop for inv_id=%d qty=%d"), InventoryId, Quantity);
}

void UInventorySubsystem::MoveItem(int32 InventoryId, int32 NewSlotIndex)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("newSlotIndex"), NewSlotIndex);
	GI->EmitSocketEvent(TEXT("inventory:move"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:move inv_id=%d → slot=%d"), InventoryId, NewSlotIndex);
}

void UInventorySubsystem::MergeItems(int32 SourceInventoryId, int32 TargetInventoryId)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("sourceInventoryId"), SourceInventoryId);
	Payload->SetNumberField(TEXT("targetInventoryId"), TargetInventoryId);
	GI->EmitSocketEvent(TEXT("inventory:merge"), Payload);
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:merge source=%d → target=%d"), SourceInventoryId, TargetInventoryId);
}

void UInventorySubsystem::RequestInventoryRefresh()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	GI->EmitSocketEvent(TEXT("inventory:load"), TEXT("{}"));
	UE_LOG(LogInventory, Log, TEXT("Sent inventory:load request"));
}

void UInventorySubsystem::SplitStack(int32 InventoryId, int32 Quantity)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	Payload->SetNumberField(TEXT("quantity"), Quantity);
	GI->EmitSocketEvent(TEXT("inventory:split"), Payload);
}

void UInventorySubsystem::SortInventory(const FString& SortBy)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;
	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("sortBy"), SortBy);
	GI->EmitSocketEvent(TEXT("inventory:sort"), Payload);
}

void UInventorySubsystem::AutoStack()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;
	GI->EmitSocketEvent(TEXT("inventory:auto_stack"), TEXT("{}"));
}

// ============================================================
// Drag-and-drop
// ============================================================

void UInventorySubsystem::StartDrag(const FInventoryItem& Item, EItemDragSource Source)
{
	DragState = FDraggedItem::FromItem(Item, Source);
	bIsDragging = true;
	ShowDragCursor(Item);
	UE_LOG(LogInventory, Log, TEXT("Drag started: %s from %s"), *Item.Name, Source == EItemDragSource::Inventory ? TEXT("Inventory") : TEXT("Equipment"));
}

void UInventorySubsystem::CompleteDrop(EItemDropTarget Target, const FString& SlotPosition, int32 TargetSlotIndex)
{
	if (!bIsDragging || !DragState.IsValid())
	{
		CancelDrag();
		return;
	}

	switch (Target)
	{
	case EItemDropTarget::EquipmentSlot:
	case EItemDropTarget::EquipmentPortrait:
		if (!DragState.bIsEquipped && !DragState.EquipSlot.IsEmpty())
		{
			EquipItem(DragState.InventoryId);
		}
		break;

	case EItemDropTarget::InventorySlot:
		// Cart → Inventory: move item from cart to inventory
		if (DragState.Source == EItemDragSource::Cart)
		{
			if (UWorld* World = GetWorld())
			{
				if (UCartSubsystem* CartSub = World->GetSubsystem<UCartSubsystem>())
				{
					CartSub->MoveToInventory(DragState.InventoryId, DragState.Quantity);
				}
			}
			break;
		}
		// Storage → Inventory: withdraw item from storage
		if (DragState.Source == EItemDragSource::Storage)
		{
			if (UWorld* World = GetWorld())
			{
				if (UStorageSubsystem* StorageSub = World->GetSubsystem<UStorageSubsystem>())
				{
					StorageSub->WithdrawItem(DragState.InventoryId, DragState.Quantity);
				}
			}
			break;
		}
		if (DragState.bIsEquipped)
		{
			UnequipItem(DragState.InventoryId);
		}
		else if (TargetSlotIndex >= 0)
		{
			// Check if target slot has same stackable item → merge instead of swap
			FInventoryItem* TargetItem = nullptr;
			for (FInventoryItem& It : Items)
			{
				if (It.SlotIndex == TargetSlotIndex && It.InventoryId != DragState.InventoryId)
				{
					TargetItem = &It;
					break;
				}
			}
			if (TargetItem && TargetItem->ItemId == DragState.ItemId && DragState.bStackable)
			{
				MergeItems(DragState.InventoryId, TargetItem->InventoryId);
			}
			else
			{
				MoveItem(DragState.InventoryId, TargetSlotIndex);
			}
		}
		break;

	case EItemDropTarget::GameWorld:
		DropItem(DragState.InventoryId, 0);
		break;

	case EItemDropTarget::HotbarSlot:
		// TargetSlotIndex encodes row*100 + slot (e.g., 102 = row 1, slot 2)
		if (DragState.ItemType == TEXT("consumable"))
		{
			if (UHotbarSubsystem* HotbarSub = GetWorld()->GetSubsystem<UHotbarSubsystem>())
			{
				int32 Row = TargetSlotIndex / 100;
				int32 Slot = TargetSlotIndex % 100;
				FInventoryItem* FullItem = FindItemByInventoryId(DragState.InventoryId);
				if (FullItem)
				{
					HotbarSub->AssignItem(Row, Slot, *FullItem);
				}
			}
		}
		break;

	case EItemDropTarget::CartSlot:
		if (DragState.Source == EItemDragSource::Inventory)
		{
			if (UWorld* World = GetWorld())
			{
				if (UCartSubsystem* CartSub = World->GetSubsystem<UCartSubsystem>())
				{
					CartSub->MoveToCart(DragState.InventoryId, DragState.Quantity);
				}
			}
		}
		else if (DragState.Source == EItemDragSource::Storage)
		{
			if (UWorld* World = GetWorld())
			{
				if (UStorageSubsystem* StorageSub = World->GetSubsystem<UStorageSubsystem>())
				{
					StorageSub->WithdrawToCart(DragState.InventoryId, DragState.Quantity);
				}
			}
		}
		break;

	case EItemDropTarget::StorageSlot:
		if (DragState.Source == EItemDragSource::Inventory)
		{
			if (UWorld* World = GetWorld())
			{
				if (UStorageSubsystem* StorageSub = World->GetSubsystem<UStorageSubsystem>())
				{
					StorageSub->DepositItem(DragState.InventoryId, DragState.Quantity);
				}
			}
		}
		break;

	case EItemDropTarget::TradeSlot:
		if (DragState.Source == EItemDragSource::Inventory)
		{
			if (UWorld* World = GetWorld())
			{
				if (UTradeSubsystem* TradeSub = World->GetSubsystem<UTradeSubsystem>())
				{
					TradeSub->AddItem(DragState.InventoryId, DragState.Quantity);
				}
			}
		}
		break;

	default:
		break;
	}

	CancelDrag();
}

void UInventorySubsystem::CancelDrag()
{
	HideDragCursor();
	bIsDragging = false;
	DragState = FDraggedItem();
}

// ============================================================
// Drag cursor overlay (icon follows mouse during drag)
// ============================================================

void UInventorySubsystem::ShowDragCursor(const FInventoryItem& Item)
{
	HideDragCursor();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	// Build the icon content: texture if available, colored square fallback
	// Size matches inventory grid icon (28x28)
	static constexpr float DragIconSize = 28.f;

	TSharedRef<SWidget> IconContent = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = GetOrCreateItemIconBrush(Item.Icon);
		if (Brush)
		{
			return SNew(SImage).Image(Brush);
		}
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.5f, 0.4f, 0.2f, 1.f));
	}();

	SAssignNew(DragCursorBox, SBox)
		.WidthOverride(DragIconSize)
		.HeightOverride(DragIconSize)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::HitTestInvisible)
		[
			IconContent
		];

	// Alignment wrapper prevents SBox from stretching to fill viewport
	// (AddViewportWidgetContent allocates full viewport size — without this,
	// the SImage fills 1920x1080 instead of 28x28)
	DragCursorAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::HitTestInvisible)
		[
			DragCursorBox.ToSharedRef()
		];

	// Set initial position at cursor
	UpdateDragCursorPosition();

	DragCursorOverlay = SNew(SWeakWidget).PossiblyNullContent(DragCursorAlignWrapper);
	VC->AddViewportWidgetContent(DragCursorOverlay.ToSharedRef(), 50);
}

void UInventorySubsystem::HideDragCursor()
{
	if (DragCursorOverlay.IsValid())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			UGameViewportClient* VC = World->GetGameViewport();
			if (VC)
			{
				VC->RemoveViewportWidgetContent(DragCursorOverlay.ToSharedRef());
			}
		}
	}
	DragCursorBox.Reset();
	DragCursorAlignWrapper.Reset();
	DragCursorOverlay.Reset();
}

void UInventorySubsystem::UpdateDragCursorPosition()
{
	if (!bIsDragging || !DragCursorBox.IsValid() || !DragCursorAlignWrapper.IsValid()) return;

	// GetCursorPos() returns absolute desktop coordinates.
	// RenderTransform is relative to the viewport origin.
	// Convert via the alignment wrapper's geometry (it sits at viewport origin).
	FVector2D CursorAbsPos = FSlateApplication::Get().GetCursorPos();
	FGeometry WrapperGeo = DragCursorAlignWrapper->GetCachedGeometry();
	FVector2D LocalPos = WrapperGeo.AbsoluteToLocal(CursorAbsPos);

	// Center the icon on the cursor
	static constexpr float HalfIcon = 14.f;  // 28 / 2
	DragCursorBox->SetRenderTransform(FSlateRenderTransform(
		FVector2f((float)LocalPos.X - HalfIcon, (float)LocalPos.Y - HalfIcon)));
}

// ============================================================
// Widget toggle
// ============================================================

void UInventorySubsystem::ToggleWidget()
{
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	if (bWidgetVisible)
	{
		if (ViewportOverlay.IsValid())
		{
			ViewportClient->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
		InventoryWidget.Reset();
		AlignmentWrapper.Reset();
		ViewportOverlay.Reset();
		bWidgetVisible = false;
		UE_LOG(LogInventory, Log, TEXT("Inventory widget hidden."));
	}
	else
	{
		InventoryWidget = SNew(SInventoryWidget).Subsystem(this);

		AlignmentWrapper =
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				InventoryWidget.ToSharedRef()
			];

		ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
		ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 14);
		bWidgetVisible = true;

		// Request fresh data when showing
		RequestInventoryRefresh();

		UE_LOG(LogInventory, Log, TEXT("Inventory widget shown (Z=14)."));
	}
}

bool UInventorySubsystem::IsWidgetVisible() const
{
	return bWidgetVisible;
}

// ============================================================
// Card compound
// ============================================================

TArray<FInventoryItem> UInventorySubsystem::FindEligibleEquipment(const FInventoryItem& Card) const
{
	TArray<FInventoryItem> Result;
	if (Card.CardType.IsEmpty()) return Result;

	for (const FInventoryItem& Item : Items)
	{
		// Must have an equip slot (i.e. is equipment)
		if (Item.EquipSlot.IsEmpty()) continue;
		// Must have card slots
		if (Item.Slots <= 0) continue;
		// RO Classic: equipment must be unequipped
		if (Item.bIsEquipped) continue;

		// Match card type to equipment slot
		bool bMatches = false;
		if (Card.CardType == TEXT("weapon") && Item.EquipSlot == TEXT("weapon"))
			bMatches = true;
		else if (Card.CardType == TEXT("shield") && Item.EquipSlot == TEXT("shield"))
			bMatches = true;
		else if (Card.CardType == TEXT("armor") && Item.EquipSlot == TEXT("armor"))
			bMatches = true;
		else if (Card.CardType == TEXT("garment") && Item.EquipSlot == TEXT("garment"))
			bMatches = true;
		else if (Card.CardType == TEXT("footgear") && Item.EquipSlot == TEXT("footgear"))
			bMatches = true;
		else if (Card.CardType == TEXT("headgear") &&
				(Item.EquipSlot == TEXT("head_top") || Item.EquipSlot == TEXT("head_mid") || Item.EquipSlot == TEXT("head_low")))
			bMatches = true;
		else if (Card.CardType == TEXT("accessory") &&
				(Item.EquipSlot == TEXT("accessory") || Item.EquipSlot.StartsWith(TEXT("accessory"))))
			bMatches = true;

		if (!bMatches) continue;

		// Check has at least one empty card slot
		int32 FilledSlots = 0;
		for (int32 CardId : Item.CompoundedCards)
		{
			if (CardId > 0) FilledSlots++;
		}
		if (FilledSlots >= Item.Slots) continue;

		Result.Add(Item);
	}
	return Result;
}

void UInventorySubsystem::BeginCardCompound(const FInventoryItem& Card)
{
	HideCardCompoundPopup();

	TArray<FInventoryItem> Eligible = FindEligibleEquipment(Card);
	if (Eligible.Num() == 0)
	{
		// RO Classic: silently does nothing when no valid equipment exists
		UE_LOG(LogInventory, Log, TEXT("No eligible equipment for card %s (CardType=%s)"), *Card.Name, *Card.CardType);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	CardCompoundPopup = SNew(SCardCompoundPopup)
		.Subsystem(this)
		.Card(Card)
		.EligibleEquipment(Eligible);

	CardCompoundAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			CardCompoundPopup.ToSharedRef()
		];

	CardCompoundOverlay = SNew(SWeakWidget).PossiblyNullContent(CardCompoundAlignWrapper);
	VC->AddViewportWidgetContent(CardCompoundOverlay.ToSharedRef(), 23);
	bCardCompoundVisible = true;

	// Focus the popup so Escape key works
	FSlateApplication::Get().SetKeyboardFocus(CardCompoundPopup);

	UE_LOG(LogInventory, Log, TEXT("Card compound popup shown for %s — %d eligible items (Z=23)"), *Card.Name, Eligible.Num());
}

void UInventorySubsystem::HideCardCompoundPopup()
{
	if (!bCardCompoundVisible) return;

	if (CardCompoundOverlay.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* VC = World->GetGameViewport())
			{
				VC->RemoveViewportWidgetContent(CardCompoundOverlay.ToSharedRef());
			}
		}
	}
	CardCompoundPopup.Reset();
	CardCompoundAlignWrapper.Reset();
	CardCompoundOverlay.Reset();
	bCardCompoundVisible = false;

	UE_LOG(LogInventory, Log, TEXT("Card compound popup hidden."));
}

bool UInventorySubsystem::IsCardCompoundVisible() const
{
	return bCardCompoundVisible;
}

void UInventorySubsystem::EmitCardCompound(int32 CardInventoryId, int32 EquipInventoryId, int32 SlotIndex)
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("cardInventoryId"), CardInventoryId);
	Payload->SetNumberField(TEXT("equipInventoryId"), EquipInventoryId);
	Payload->SetNumberField(TEXT("slotIndex"), SlotIndex);
	GI->EmitSocketEvent(TEXT("card:compound"), Payload);

	UE_LOG(LogInventory, Log, TEXT("Sent card:compound cardInvId=%d equipInvId=%d slot=%d"),
		CardInventoryId, EquipInventoryId, SlotIndex);
}

void UInventorySubsystem::HandleCardResult(const TSharedPtr<FJsonValue>& Data)
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
		UE_LOG(LogInventory, Log, TEXT("Card compound success: %s"), *Message);
		HideCardCompoundPopup();
		// inventory:data refresh arrives separately from server and triggers grid rebuild via DataVersion++
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("Card compound failed: %s"), *Message);
		// Show error in popup if still visible
		if (CardCompoundPopup.IsValid() && bCardCompoundVisible)
		{
			CardCompoundPopup->SetStatusMessage(Message, true);
		}
	}
}

// ============================================================
// Identify popup — Item Appraisal
// ============================================================

void UInventorySubsystem::HandleIdentifyItemList(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	// Parse source field to determine if magnifier or skill
	FString Source;
	Obj->TryGetStringField(TEXT("source"), Source);
	bool bMagnifier = (Source == TEXT("magnifier"));

	// Parse items array
	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (!Obj->TryGetArrayField(TEXT("items"), ItemsArray) || !ItemsArray)
	{
		UE_LOG(LogInventory, Warning, TEXT("identify:item_list — no items array"));
		return;
	}

	TArray<FInventoryItem> UnidentifiedItems;
	for (const TSharedPtr<FJsonValue>& ItemVal : *ItemsArray)
	{
		const TSharedPtr<FJsonObject>* ItemObjPtr = nullptr;
		if (!ItemVal.IsValid() || !ItemVal->TryGetObject(ItemObjPtr) || !ItemObjPtr) continue;
		const TSharedPtr<FJsonObject>& ItemObj = *ItemObjPtr;

		FInventoryItem Item;
		double Val = 0;
		if (ItemObj->TryGetNumberField(TEXT("inventory_id"), Val)) Item.InventoryId = (int32)Val;
		if (ItemObj->TryGetNumberField(TEXT("item_id"), Val)) Item.ItemId = (int32)Val;

		FString Str;
		if (ItemObj->TryGetStringField(TEXT("name"), Str)) Item.Name = Str;
		if (ItemObj->TryGetStringField(TEXT("item_type"), Str)) Item.ItemType = Str;
		if (ItemObj->TryGetStringField(TEXT("icon"), Str)) Item.Icon = Str;
		if (ItemObj->TryGetStringField(TEXT("equip_slot"), Str)) Item.EquipSlot = Str;
		if (ItemObj->TryGetStringField(TEXT("weapon_type"), Str)) Item.WeaponType = Str;
		Item.bIdentified = false;

		if (Item.InventoryId > 0)
		{
			UnidentifiedItems.Add(Item);
		}
	}

	if (UnidentifiedItems.Num() == 0)
	{
		UE_LOG(LogInventory, Log, TEXT("identify:item_list — no unidentified items to show"));
		return;
	}

	UE_LOG(LogInventory, Log, TEXT("identify:item_list — %d items, source=%s"), UnidentifiedItems.Num(), *Source);
	ShowIdentifyPopup(UnidentifiedItems, bMagnifier);
}

void UInventorySubsystem::HandleIdentifyResult(const TSharedPtr<FJsonValue>& Data)
{
	if (!Data.IsValid()) return;
	const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
	if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
	const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

	bool bSuccess = false;
	Obj->TryGetBoolField(TEXT("success"), bSuccess);
	FString Message;
	Obj->TryGetStringField(TEXT("message"), Message);
	FString ItemName;
	Obj->TryGetStringField(TEXT("itemName"), ItemName);

	double InvIdVal = 0;
	int32 InventoryId = 0;
	if (Obj->TryGetNumberField(TEXT("inventoryId"), InvIdVal))
	{
		InventoryId = (int32)InvIdVal;
	}

	if (bSuccess)
	{
		UE_LOG(LogInventory, Log, TEXT("Identify success: %s (invId=%d)"), *ItemName, InventoryId);

		// RO Classic: both Magnifier and Item Appraisal identify 1 item per use, then close
		HideIdentifyPopup();
		// inventory:data refresh arrives separately from server and triggers grid rebuild via DataVersion++
	}
	else
	{
		UE_LOG(LogInventory, Warning, TEXT("Identify failed: %s"), *Message);
		if (IdentifyPopup.IsValid() && bIdentifyPopupVisible)
		{
			IdentifyPopup->SetStatusMessage(Message, true);
		}
	}
}

void UInventorySubsystem::ShowIdentifyPopup(const TArray<FInventoryItem>& UnidentifiedItems, bool bIsMagnifier)
{
	HideIdentifyPopup();

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	bIdentifyIsMagnifier = bIsMagnifier;

	IdentifyPopup = SNew(SIdentifyPopup)
		.Subsystem(this)
		.UnidentifiedItems(UnidentifiedItems)
		.IsMagnifier(bIsMagnifier);

	IdentifyAlignWrapper =
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::Visible)
		[
			IdentifyPopup.ToSharedRef()
		];

	IdentifyOverlay = SNew(SWeakWidget).PossiblyNullContent(IdentifyAlignWrapper);
	VC->AddViewportWidgetContent(IdentifyOverlay.ToSharedRef(), 24);
	bIdentifyPopupVisible = true;

	// Focus the popup so Escape key works
	FSlateApplication::Get().SetKeyboardFocus(IdentifyPopup);

	UE_LOG(LogInventory, Log, TEXT("Identify popup shown — %d items, magnifier=%d (Z=24)"), UnidentifiedItems.Num(), bIsMagnifier ? 1 : 0);
}

void UInventorySubsystem::HideIdentifyPopup()
{
	if (!bIdentifyPopupVisible) return;

	if (IdentifyOverlay.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameViewportClient* VC = World->GetGameViewport())
			{
				VC->RemoveViewportWidgetContent(IdentifyOverlay.ToSharedRef());
			}
		}
	}
	IdentifyPopup.Reset();
	IdentifyAlignWrapper.Reset();
	IdentifyOverlay.Reset();
	bIdentifyPopupVisible = false;
	bIdentifyIsMagnifier = false;

	UE_LOG(LogInventory, Log, TEXT("Identify popup hidden."));
}

bool UInventorySubsystem::IsIdentifyPopupVisible() const
{
	return bIdentifyPopupVisible;
}
