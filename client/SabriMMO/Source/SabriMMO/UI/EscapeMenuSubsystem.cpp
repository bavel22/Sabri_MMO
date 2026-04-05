#include "EscapeMenuSubsystem.h"
#include "SEscapeMenuWidget.h"
#include "MMOGameInstance.h"
#include "CombatActionSubsystem.h"
#include "MultiplayerEventSubsystem.h"
#include "HotbarSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "GenericPlatform/GenericPlatformMisc.h"

// ============================================================
// Lifecycle
// ============================================================

bool UEscapeMenuSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UEscapeMenuSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = GetGI();
	if (!GI || !GI->IsSocketConnected()) return;

	AddWidgetToViewport();
}

void UEscapeMenuSubsystem::Deinitialize()
{
	RemoveWidgetFromViewport();
	Super::Deinitialize();
}

// ============================================================
// Toggle / Show / Hide
// ============================================================

void UEscapeMenuSubsystem::ToggleMenu()
{
	if (bMenuVisible)
		HideMenu();
	else
		ShowMenu();
}

void UEscapeMenuSubsystem::ShowMenu()
{
	if (!bWidgetAdded) return;
	bMenuVisible = true;
	if (Widget.IsValid())
	{
		Widget->SetVisibility(EVisibility::Visible);
	}
}

void UEscapeMenuSubsystem::HideMenu()
{
	bMenuVisible = false;
	if (Widget.IsValid())
	{
		Widget->SetVisibility(EVisibility::Collapsed);
	}
}

// ============================================================
// Dead state query
// ============================================================

bool UEscapeMenuSubsystem::IsPlayerDead() const
{
	UWorld* World = GetWorld();
	if (!World) return false;
	if (UCombatActionSubsystem* CAS = World->GetSubsystem<UCombatActionSubsystem>())
	{
		return CAS->IsDead();
	}
	return false;
}

// ============================================================
// Button Actions
// ============================================================

void UEscapeMenuSubsystem::OnCharacterSelectPressed()
{
	HideMenu();
	UMMOGameInstance* GI = GetGI();
	if (GI)
	{
		GI->ReturnToCharacterSelect();
	}
}

void UEscapeMenuSubsystem::OnRespawnPressed()
{
	HideMenu();
	UWorld* World = GetWorld();
	if (!World) return;
	if (UMultiplayerEventSubsystem* MES = World->GetSubsystem<UMultiplayerEventSubsystem>())
	{
		MES->RequestRespawn();
	}
}

void UEscapeMenuSubsystem::OnHotkeyPressed()
{
	HideMenu();
	UWorld* World = GetWorld();
	if (!World) return;
	if (UHotbarSubsystem* HS = World->GetSubsystem<UHotbarSubsystem>())
	{
		HS->ToggleKeybindWidget();
	}
}

void UEscapeMenuSubsystem::OnExitGamePressed()
{
	HideMenu();
	// Disconnect socket cleanly then exit
	UMMOGameInstance* GI = GetGI();
	if (GI)
	{
		GI->DisconnectSocket();
	}
	FPlatformMisc::RequestExit(false);
}

// ============================================================
// Viewport Integration
// ============================================================

void UEscapeMenuSubsystem::AddWidgetToViewport()
{
	if (bWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	Widget = SNew(SEscapeMenuWidget).Subsystem(this);
	Widget->SetVisibility(EVisibility::Collapsed);

	AlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			Widget.ToSharedRef()
		];

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(AlignmentWrapper);
	VC->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), EscMenuZOrder);
	bWidgetAdded = true;
}

void UEscapeMenuSubsystem::RemoveWidgetFromViewport()
{
	if (!bWidgetAdded) return;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (ViewportOverlay.IsValid())
			{
				VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
			}
		}
	}
	Widget.Reset();
	AlignmentWrapper.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
}

// ============================================================
// Helper
// ============================================================

UMMOGameInstance* UEscapeMenuSubsystem::GetGI() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<UMMOGameInstance>(World->GetGameInstance());
	}
	return nullptr;
}
