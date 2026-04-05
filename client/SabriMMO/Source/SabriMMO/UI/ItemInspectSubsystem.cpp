#include "ItemInspectSubsystem.h"
#include "SItemInspectWidget.h"
#include "MMOGameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogItemInspect, Log, All);

bool UItemInspectSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	UGameInstance* GI = World->GetGameInstance();
	if (UMMOGameInstance* MMO = Cast<UMMOGameInstance>(GI))
	{
		return MMO->IsSocketConnected();
	}
	return false;
}

void UItemInspectSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UE_LOG(LogItemInspect, Log, TEXT("ItemInspectSubsystem started"));
}

void UItemInspectSubsystem::Deinitialize()
{
	HideInspect();
	Widget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	Super::Deinitialize();
}

void UItemInspectSubsystem::EnsureWidgetCreated()
{
	if (bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* ViewportClient = World->GetGameViewport();
	if (!ViewportClient) return;

	Widget = SNew(SItemInspectWidget).Subsystem(this);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			Widget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 22);
	bWidgetAdded = true;

	UE_LOG(LogItemInspect, Log, TEXT("ItemInspect widget added to viewport at Z=22"));
}

void UItemInspectSubsystem::ShowInspect(const FInventoryItem& Item)
{
	InspectedItem = Item;
	EnsureWidgetCreated();

	if (Widget.IsValid())
	{
		Widget->UpdateItem(Item);
		Widget->SetVisibility(EVisibility::Visible);
	}
	bWidgetVisible = true;

	UE_LOG(LogItemInspect, Log, TEXT("ShowInspect: %s (ID=%d)"), *Item.GetDisplayName(), Item.ItemId);
}

void UItemInspectSubsystem::HideInspect()
{
	if (Widget.IsValid())
	{
		Widget->SetVisibility(EVisibility::Collapsed);
	}
	bWidgetVisible = false;
}
