#include "GroundItemSubsystem.h"
#include "GroundItemActor.h"
#include "SabriMMO/MMOGameInstance.h"
#include "SabriMMO/SocketEventRouter.h"
#include "Audio/AudioSubsystem.h"
#include "UI/ChatSubsystem.h"
#include "UI/OptionsSubsystem.h"
#include "SabriMMO/SabriMMOCharacter.h"
#include "SabriMMO/Sprite/SpriteCharacterActor.h"
#include "SabriMMO/Sprite/SpriteAtlasData.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogGroundItemSub, Log, All);

void UGroundItemSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	USocketEventRouter* Router = GI->GetEventRouter();
	if (!Router) return;

	// Register socket event handlers
	Router->RegisterHandler(TEXT("item:spawned_batch"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemSpawnedBatch(D); });
	Router->RegisterHandler(TEXT("item:picked_up"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemPickedUp(D); });
	Router->RegisterHandler(TEXT("item:despawned_batch"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemDespawnedBatch(D); });
	Router->RegisterHandler(TEXT("item:ground_list"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandleItemGroundList(D); });
	Router->RegisterHandler(TEXT("item:pickup_success"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePickupSuccess(D); });
	Router->RegisterHandler(TEXT("item:pickup_error"), this,
		[this](const TSharedPtr<FJsonValue>& D) { HandlePickupError(D); });

	if (!GI->IsSocketConnected()) return;

	UE_LOG(LogGroundItemSub, Log, TEXT("GroundItemSubsystem initialized"));
}

void UGroundItemSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
		{
			if (USocketEventRouter* Router = GI->GetEventRouter())
			{
				Router->UnregisterAllForOwner(this);
			}
		}

		// Destroy all ground item actors
		for (auto& Pair : GroundItemMap)
		{
			if (Pair.Value.Actor.IsValid())
			{
				Pair.Value.Actor->Destroy();
			}
		}
	}

	GroundItemMap.Empty();
	ActorToGroundItemId.Empty();

	Super::Deinitialize();
}

AGroundItemActor* UGroundItemSubsystem::GetGroundItemActor(int32 GroundItemId) const
{
	const FGroundItemEntry* Entry = GroundItemMap.Find(GroundItemId);
	if (Entry && Entry->Actor.IsValid())
	{
		return Entry->Actor.Get();
	}
	return nullptr;
}

int32 UGroundItemSubsystem::GetGroundItemIdFromActor(AActor* Actor) const
{
	const int32* Found = ActorToGroundItemId.Find(Actor);
	return Found ? *Found : -1;
}

void UGroundItemSubsystem::RequestPickup(int32 GroundItemId)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("groundItemId"), GroundItemId);

	// Send current position so server uses fresh data (avoids 30Hz broadcast lag)
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			FVector Loc = Pawn->GetActorLocation();
			Payload->SetNumberField(TEXT("px"), Loc.X);
			Payload->SetNumberField(TEXT("py"), Loc.Y);
		}
	}

	GI->EmitSocketEvent(TEXT("item:pickup"), Payload);
}

// ── Socket Event Handlers ──

void UGroundItemSubsystem::HandleItemSpawnedBatch(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
	if (!(*Obj)->TryGetArrayField(TEXT("items"), ItemsArr)) return;

	for (const auto& ItemVal : *ItemsArr)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObj) && ItemObj)
		{
			SpawnGroundItemFromJson(*ItemObj);
		}
	}
}

void UGroundItemSubsystem::HandleItemPickedUp(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	int32 GId = static_cast<int32>((*Obj)->GetNumberField(TEXT("groundItemId")));
	RemoveGroundItem(GId);
}

void UGroundItemSubsystem::HandleItemDespawnedBatch(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	const TArray<TSharedPtr<FJsonValue>>* IdsArr = nullptr;
	if (!(*Obj)->TryGetArrayField(TEXT("groundItemIds"), IdsArr)) return;

	for (const auto& IdVal : *IdsArr)
	{
		int32 GId = static_cast<int32>(IdVal->AsNumber());
		RemoveGroundItem(GId, true); // Fade out on despawn
	}
}

void UGroundItemSubsystem::HandleItemGroundList(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	const TArray<TSharedPtr<FJsonValue>>* ItemsArr = nullptr;
	if (!(*Obj)->TryGetArrayField(TEXT("items"), ItemsArr)) return;

	// Clear existing ground items (zone transition — fresh state)
	for (auto& Pair : GroundItemMap)
	{
		if (Pair.Value.Actor.IsValid())
		{
			Pair.Value.Actor->Destroy();
		}
	}
	GroundItemMap.Empty();
	ActorToGroundItemId.Empty();

	for (const auto& ItemVal : *ItemsArr)
	{
		const TSharedPtr<FJsonObject>* ItemObj = nullptr;
		if (ItemVal.IsValid() && ItemVal->TryGetObject(ItemObj) && ItemObj)
		{
			SpawnGroundItemFromJson(*ItemObj, false); // No sound for existing items
		}
	}

	UE_LOG(LogGroundItemSub, Log, TEXT("Loaded %d ground items from zone:ready"), GroundItemMap.Num());
}

void UGroundItemSubsystem::HandlePickupSuccess(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	FString TierColor = (*Obj)->GetStringField(TEXT("tierColor"));
	FString ItemName = (*Obj)->GetStringField(TEXT("itemName"));
	int32 Qty = static_cast<int32>((*Obj)->GetNumberField(TEXT("quantity")));

	UWorld* World = GetWorld();
	if (!World) return;

	APawn* Pawn = World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr;

	// Play pickup sound (gated by per-tier option)
	{
		bool bShouldPlay = true;
		if (UOptionsSubsystem* Opts = World->GetSubsystem<UOptionsSubsystem>())
			bShouldPlay = Opts->ShouldPlayDropSound(TierColor);
		if (bShouldPlay)
		{
			if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
			{
				FVector Loc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
				Audio->PlayItemDropSound(TierColor, Loc);
			}
		}
	}

	// Play Pickup animation on the local player's sprite
	if (ASabriMMOCharacter* Char = Cast<ASabriMMOCharacter>(Pawn))
	{
		if (Char->LocalSprite.IsValid())
		{
			Char->LocalSprite->SetAnimState(ESpriteAnimState::Pickup);
		}
	}

	UE_LOG(LogGroundItemSub, Log, TEXT("Picked up %dx %s"), Qty, *ItemName);
}

void UGroundItemSubsystem::HandlePickupError(const TSharedPtr<FJsonValue>& Data)
{
	const TSharedPtr<FJsonObject>* Obj = nullptr;
	if (!Data.IsValid() || !Data->TryGetObject(Obj) || !Obj) return;

	FString Message = (*Obj)->GetStringField(TEXT("message"));
	UE_LOG(LogGroundItemSub, Warning, TEXT("Pickup error: %s"), *Message);

	// Route to chat as combat log message
	UWorld* World = GetWorld();
	if (World)
	{
		if (UChatSubsystem* Chat = World->GetSubsystem<UChatSubsystem>())
		{
			Chat->AddCombatLogMessage(Message);
		}
	}
}

// ── Helpers ──

void UGroundItemSubsystem::SpawnGroundItemFromJson(const TSharedPtr<FJsonObject>& ItemObj, bool bPlaySound)
{
	if (!ItemObj.IsValid()) return;

	int32 GId = static_cast<int32>(ItemObj->GetNumberField(TEXT("groundItemId")));
	if (GroundItemMap.Contains(GId)) return; // Already exists

	FGroundItemEntry Entry;
	Entry.GroundItemId = GId;
	Entry.ItemId = static_cast<int32>(ItemObj->GetNumberField(TEXT("itemId")));
	Entry.ItemName = ItemObj->GetStringField(TEXT("itemName"));
	Entry.Quantity = FMath::Max(1, static_cast<int32>(ItemObj->GetNumberField(TEXT("quantity"))));
	Entry.Icon = ItemObj->GetStringField(TEXT("icon"));
	Entry.TierColor = ItemObj->GetStringField(TEXT("tierColor"));
	Entry.ItemType = ItemObj->GetStringField(TEXT("itemType"));
	Entry.Position = FVector(
		ItemObj->GetNumberField(TEXT("x")),
		ItemObj->GetNumberField(TEXT("y")),
		ItemObj->GetNumberField(TEXT("z"))
	);
	Entry.SourcePosition = FVector(
		ItemObj->GetNumberField(TEXT("sourceX")),
		ItemObj->GetNumberField(TEXT("sourceY")),
		ItemObj->GetNumberField(TEXT("sourceZ"))
	);
	Entry.SourceType = ItemObj->GetStringField(TEXT("sourceType"));
	Entry.OwnerCharId = static_cast<int32>(ItemObj->GetNumberField(TEXT("ownerCharId")));
	Entry.bIsMvpDrop = ItemObj->GetBoolField(TEXT("isMvpDrop"));

	// Spawn actor
	UWorld* World = GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn at server position — actor's BeginPlay handles ground-snap via line trace
	FVector SpawnLoc = Entry.Position;

	AGroundItemActor* Actor = World->SpawnActor<AGroundItemActor>(
		AGroundItemActor::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);

	if (Actor)
	{
		Actor->InitGroundItem(GId, Entry.ItemName, Entry.Icon, Entry.TierColor, Entry.Quantity);
		Entry.Actor = Actor;
		ActorToGroundItemId.Add(Actor, GId);

		if (bPlaySound)
		{
			if (Entry.SourceType == TEXT("monster"))
			{
				// Monster drops: arc from death position to scatter position
				Actor->PlayDropArc(Entry.SourcePosition, SpawnLoc, 0.4f);
			}
			else
			{
				// Player drops: short drop arc (from slightly above to same position)
				FVector AbovePos = SpawnLoc;
				AbovePos.Z += 40.f;
				Actor->PlayDropArc(AbovePos, SpawnLoc, 0.25f);
			}
		}
		else
		{
			// zone:ready bulk load — no animation, just snap to ground
			Actor->SnapToGround();
		}

		// Play drop sound for newly spawned items (gated by per-tier option)
		if (bPlaySound)
		{
			bool bShouldPlay = true;
			if (UOptionsSubsystem* Opts = World->GetSubsystem<UOptionsSubsystem>())
			{
				bShouldPlay = Opts->ShouldPlayDropSound(Entry.TierColor);
			}
			if (bShouldPlay)
			{
				if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
				{
					Audio->PlayItemDropSound(Entry.TierColor, SpawnLoc);
				}
			}
		}
	}

	GroundItemMap.Add(GId, Entry);
}

void UGroundItemSubsystem::RemoveGroundItem(int32 GroundItemId, bool bFade)
{
	FGroundItemEntry* Entry = GroundItemMap.Find(GroundItemId);
	if (!Entry) return;

	if (Entry->Actor.IsValid())
	{
		ActorToGroundItemId.Remove(Entry->Actor.Get());
		if (bFade)
		{
			Entry->Actor->FadeOutAndDestroy(0.5f);
		}
		else
		{
			Entry->Actor->Destroy();
		}
	}

	GroundItemMap.Remove(GroundItemId);
}
