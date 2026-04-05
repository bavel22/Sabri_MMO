// EquipmentSubsystem.cpp — Tracks equipped items by slot position and manages
// the SEquipmentWidget overlay. Reads from InventorySubsystem for item data.

#include "EquipmentSubsystem.h"
#include "SEquipmentWidget.h"
#include "InventorySubsystem.h"
#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogEquipment, Log, All);

// ============================================================
// Lifecycle
// ============================================================

bool UEquipmentSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return World->IsGameWorld();
}

void UEquipmentSubsystem::OnWorldBeginPlay(UWorld& InWorld)
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
	}

	UE_LOG(LogEquipment, Log, TEXT("EquipmentSubsystem started — events registered via EventRouter. LocalCharId=%d"), LocalCharacterId);
}

void UEquipmentSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	if (bWidgetVisible)
	{
		ToggleWidget();
	}

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

	UE_LOG(LogEquipment, Log, TEXT("EquipmentSubsystem deinitialized."));
	Super::Deinitialize();
}

// ============================================================
// Event handler — rebuild equipped slots on inventory update
// ============================================================

void UEquipmentSubsystem::HandleInventoryData(const TSharedPtr<FJsonValue>& Data)
{
	// InventorySubsystem also handles inventory:data and parses the items array.
	// Defer our refresh by one tick so InventorySubsystem has already updated its Items.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
		TWeakObjectPtr<UEquipmentSubsystem> WeakThis(this);
		RefreshTimerHandle = World->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (UEquipmentSubsystem* Self = WeakThis.Get())
			{
				Self->RefreshEquippedSlots();
			}
		});
	}
}

// ============================================================
// Equipped slots management
// ============================================================

void UEquipmentSubsystem::RefreshEquippedSlots()
{
	EquippedSlots.Empty();

	UWorld* World = GetWorld();
	if (!World) return;

	UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
	if (!InvSub) return;

	for (const FInventoryItem& Item : InvSub->Items)
	{
		if (!Item.bIsEquipped) continue;

		FString Position = Item.EquippedPosition;
		if (Position.IsEmpty())
		{
			// Fallback: use equip_slot directly (for items without equipped_position set)
			Position = Item.EquipSlot;
			if (Position == TEXT("accessory"))
			{
				Position = EquipSlots::Accessory1;
			}
		}

		if (!Position.IsEmpty())
		{
			EquippedSlots.Add(Position, Item);
		}
	}

	UE_LOG(LogEquipment, Log, TEXT("Equipment refreshed: %d slots occupied"), EquippedSlots.Num());

	OnEquipmentChanged.Broadcast();
}

FInventoryItem UEquipmentSubsystem::GetEquippedItem(const FString& SlotPosition) const
{
	const FInventoryItem* Found = EquippedSlots.Find(SlotPosition);
	return Found ? *Found : FInventoryItem();
}

bool UEquipmentSubsystem::IsSlotOccupied(const FString& SlotPosition) const
{
	return EquippedSlots.Contains(SlotPosition);
}

void UEquipmentSubsystem::UnequipSlot(const FString& SlotPosition)
{
	const FInventoryItem* Found = EquippedSlots.Find(SlotPosition);
	if (!Found) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
	if (!InvSub) return;

	InvSub->UnequipItem(Found->InventoryId);
	UE_LOG(LogEquipment, Log, TEXT("Unequip slot %s (inv_id=%d)"), *SlotPosition, Found->InventoryId);
}

bool UEquipmentSubsystem::CanEquipToSlot(const FString& ItemEquipSlot, const FString& SlotPosition) const
{
	if (ItemEquipSlot == TEXT("accessory"))
	{
		return SlotPosition == EquipSlots::Accessory1 || SlotPosition == EquipSlots::Accessory2;
	}
	// Assassin: weapons can go to weapon_left slot (displayed as Shield slot)
	if (ItemEquipSlot == TEXT("weapon") && SlotPosition == EquipSlots::WeaponLeft)
	{
		FString JobClass = GetLocalJobClass();
		if (EquipSlots::CanDualWield(JobClass)) return true;
	}
	// Assassin: weapons can also go to the shield slot position (which becomes weapon_left)
	if (ItemEquipSlot == TEXT("weapon") && SlotPosition == EquipSlots::Shield)
	{
		FString JobClass = GetLocalJobClass();
		if (EquipSlots::CanDualWield(JobClass)) return true;
	}
	// Ammo items (equip_slot "Ammo" or "ammo") → ammo slot
	if ((ItemEquipSlot == TEXT("Ammo") || ItemEquipSlot == TEXT("ammo")) && SlotPosition == EquipSlots::Ammo)
		return true;
	return ItemEquipSlot == SlotPosition;
}

bool UEquipmentSubsystem::CanEquipToSlot(const FString& ItemEquipSlot, const FString& SlotPosition, const FString& WeaponType) const
{
	if (ItemEquipSlot == TEXT("weapon") && (SlotPosition == EquipSlots::WeaponLeft || SlotPosition == EquipSlots::Shield))
	{
		FString JobClass = GetLocalJobClass();
		return EquipSlots::CanDualWield(JobClass) && EquipSlots::IsValidLeftHandWeapon(WeaponType);
	}
	return CanEquipToSlot(ItemEquipSlot, SlotPosition);
}

FString UEquipmentSubsystem::GetLocalJobClass() const
{
	UWorld* World = GetWorld();
	if (!World) return TEXT("");

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI) return TEXT("");

	return GI->GetSelectedCharacter().JobClass;
}

// ============================================================
// Widget toggle
// ============================================================

void UEquipmentSubsystem::ToggleWidget()
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
		EquipWidget.Reset();
		AlignmentWrapper.Reset();
		ViewportOverlay.Reset();
		bWidgetVisible = false;
		UE_LOG(LogEquipment, Log, TEXT("Equipment widget hidden."));
	}
	else
	{
		// Refresh equipped slots before showing
		RefreshEquippedSlots();

		EquipWidget = SNew(SEquipmentWidget).Subsystem(this);

		AlignmentWrapper =
			SNew(SBox)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				EquipWidget.ToSharedRef()
			];

		ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
		ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 15);
		bWidgetVisible = true;

		UE_LOG(LogEquipment, Log, TEXT("Equipment widget shown (Z=15)."));
	}
}

bool UEquipmentSubsystem::IsWidgetVisible() const
{
	return bWidgetVisible;
}
