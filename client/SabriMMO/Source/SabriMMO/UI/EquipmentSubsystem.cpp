// EquipmentSubsystem.cpp — Tracks equipped items by slot position and manages
// the SEquipmentWidget overlay. Reads from InventorySubsystem for item data.

#include "EquipmentSubsystem.h"
#include "SEquipmentWidget.h"
#include "InventorySubsystem.h"
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

	if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		FCharacterData SelChar = GI->GetSelectedCharacter();
		LocalCharacterId = SelChar.CharacterId;
	}

	InWorld.GetTimerManager().SetTimer(
		BindCheckTimer,
		FTimerDelegate::CreateUObject(this, &UEquipmentSubsystem::TryWrapSocketEvents),
		0.5f, true
	);

	UE_LOG(LogEquipment, Log, TEXT("EquipmentSubsystem started — waiting for SocketIO bindings..."));
}

void UEquipmentSubsystem::Deinitialize()
{
	if (bWidgetVisible)
	{
		ToggleWidget();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	bEventsWrapped = false;
	CachedSIOComponent = nullptr;
	Super::Deinitialize();
}

// ============================================================
// Find SocketIO component
// ============================================================

USocketIOClientComponent* UEquipmentSubsystem::FindSocketIOComponent() const
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
// Timer callback — wrap events
// ============================================================

void UEquipmentSubsystem::TryWrapSocketEvents()
{
	if (bEventsWrapped) return;

	USocketIOClientComponent* SIOComp = FindSocketIOComponent();
	if (!SIOComp) return;

	TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
	if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

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

	// Wrap inventory:data to update equipped slots when inventory changes
	WrapSingleEvent(TEXT("inventory:data"),
		[this](const TSharedPtr<FJsonValue>& D) { HandleInventoryData(D); });

	bEventsWrapped = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BindCheckTimer);
	}

	UE_LOG(LogEquipment, Log, TEXT("EquipmentSubsystem — inventory:data wrapped. LocalCharId=%d"), LocalCharacterId);
}

void UEquipmentSubsystem::WrapSingleEvent(
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

// ============================================================
// Event handler — rebuild equipped slots on inventory update
// ============================================================

void UEquipmentSubsystem::HandleInventoryData(const TSharedPtr<FJsonValue>& Data)
{
	// InventorySubsystem parses the full items array.
	// We just need to refresh our equipped slots from its data.
	RefreshEquippedSlots();
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
	return ItemEquipSlot == SlotPosition;
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
