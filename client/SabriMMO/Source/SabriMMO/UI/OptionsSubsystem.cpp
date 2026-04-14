#include "OptionsSubsystem.h"
#include "SOptionsWidget.h"
#include "MMOGameInstance.h"
#include "CameraSubsystem.h"
#include "DamageNumberSubsystem.h"
#include "WorldHealthBarSubsystem.h"
#include "NameTagSubsystem.h"
#include "PostProcessSubsystem.h"
#include "SkillVFXSubsystem.h"
#include "CastBarSubsystem.h"
#include "ChatSubsystem.h"
#include "SDamageNumberOverlay.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "TimerManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOptions, Log, All);
DEFINE_LOG_CATEGORY(LogOptions);

// ============================================================
// Lifecycle
// ============================================================

bool UOptionsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;
	return (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

void UOptionsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI) return;

	// Login screen path: socket not connected yet. We still want the Options panel
	// available so the player can adjust BGM/SFX volume while listening to the title
	// music. Game-only side effects (FPS overlay, push to non-existent gameplay
	// subsystems) are skipped via the bIsGameWorld check below.
	const bool bIsGameWorld = GI->IsSocketConnected();

	// Restore all settings from GameInstance
	bShowFPS = GI->bOptionShowFPS;
	bSkillEffects = GI->bOptionSkillEffects;
	bShowMissText = GI->bOptionShowMissText;
	fBrightness = GI->fOptionBrightness;
	bShowDamageNumbers = GI->bOptionShowDamageNumbers;
	bShowEnemyHPBars = GI->bOptionShowEnemyHPBars;
	bShowPlayerNames = GI->bOptionShowPlayerNames;
	bShowEnemyNames = GI->bOptionShowEnemyNames;
	fCameraSensitivity = GI->fOptionCameraSensitivity;
	fCameraZoomSpeed = GI->fOptionCameraZoomSpeed;
	bShowCastBars = GI->bOptionShowCastBars;
	bShowNPCNames = GI->bOptionShowNPCNames;
	bShowChatTimestamps = GI->bOptionShowChatTimestamps;
	fChatOpacity = GI->fOptionChatOpacity;
	fDamageNumberScale = GI->fOptionDamageNumberScale;
	bDropSoundMvp    = GI->bOptionDropSoundMvp;
	bDropSoundCard   = GI->bOptionDropSoundCard;
	bDropSoundEquip  = GI->bOptionDropSoundEquip;
	bDropSoundHeal   = GI->bOptionDropSoundHeal;
	bDropSoundUsable = GI->bOptionDropSoundUsable;
	bDropSoundMisc   = GI->bOptionDropSoundMisc;
	bMuteWhenMinimized = GI->bOptionMuteWhenMinimized;
	fMasterVolume  = GI->fOptionMasterVolume;
	fBgmVolume     = GI->fOptionBgmVolume;
	fSfxVolume     = GI->fOptionSfxVolume;
	fAmbientVolume = GI->fOptionAmbientVolume;
	bNoCtrl = GI->bOptionNoCtrl;
	bNoShift = GI->bOptionNoShift;
	bAutoDeclineTrades = GI->bOptionAutoDeclineTrades;
	bAutoDeclineParty = GI->bOptionAutoDeclineParty;

	AddOptionsWidgetToViewport();

	// FPS overlay is game-only — skip on login screen
	if (bShowFPS && bIsGameWorld)
		AddFPSOverlay();

	// Push settings to target subsystems on next tick (they may not be ready yet).
	// On login screen this only pushes audio volumes (other gameplay subsystems
	// don't exist yet — PushSettingsToSubsystems gracefully no-ops missing ones).
	TWeakObjectPtr<UOptionsSubsystem> WeakThis(this);
	InWorld.GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (UOptionsSubsystem* Self = WeakThis.Get())
			Self->PushSettingsToSubsystems();
	});
}

void UOptionsSubsystem::Deinitialize()
{
	RemoveFPSOverlay();
	RemoveOptionsWidgetFromViewport();
	Super::Deinitialize();
}

// ============================================================
// Options Panel Show / Hide
// ============================================================

void UOptionsSubsystem::ShowOptionsPanel()
{
	if (!bOptionsWidgetAdded) return;
	bOptionsPanelVisible = true;
	if (OptionsWidget.IsValid())
		OptionsWidget->SetVisibility(EVisibility::Visible);
}

void UOptionsSubsystem::HideOptionsPanel()
{
	UAudioSubsystem::PlayUICancelStatic(GetWorld());
	bOptionsPanelVisible = false;
	if (OptionsWidget.IsValid())
		OptionsWidget->SetVisibility(EVisibility::Collapsed);
}

// ============================================================
// Push all settings to target subsystems
// ============================================================

void UOptionsSubsystem::PushSettingsToSubsystems()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (auto* VFX = World->GetSubsystem<USkillVFXSubsystem>())
		VFX->SetEffectsEnabled(bSkillEffects);

	if (auto* DMG = World->GetSubsystem<UDamageNumberSubsystem>())
	{
		DMG->bDamageNumbersEnabled = bShowDamageNumbers;
		DMG->bShowMissText = bShowMissText;
	}

	if (auto* WHB = World->GetSubsystem<UWorldHealthBarSubsystem>())
		WHB->bShowEnemyBars = bShowEnemyHPBars;

	if (auto* NT = World->GetSubsystem<UNameTagSubsystem>())
	{
		NT->bShowPlayerNames = bShowPlayerNames;
		NT->bShowEnemyNames = bShowEnemyNames;
	}

	if (auto* Cam = World->GetSubsystem<UCameraSubsystem>())
	{
		Cam->RotationSensitivity = fCameraSensitivity;
		Cam->ZoomSpeed = fCameraZoomSpeed;
	}

	if (auto* PP = World->GetSubsystem<UPostProcessSubsystem>())
	{
		if (fBrightness != 1.0f)
			PP->SetBrightness(fBrightness);
	}

	if (auto* CB = World->GetSubsystem<UCastBarSubsystem>())
		CB->bCastBarsEnabled = bShowCastBars;

	if (auto* Chat = World->GetSubsystem<UChatSubsystem>())
	{
		Chat->bShowTimestamps = bShowChatTimestamps;
		Chat->ChatPanelOpacity = fChatOpacity;
	}

	SDamageNumberOverlay::FontScaleMultiplier = fDamageNumberScale;

	// Push initial volume settings to the audio bus system
	if (UAudioSubsystem* Audio = World->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetMasterVolume(fMasterVolume);
		Audio->SetBgmVolume(fBgmVolume);
		Audio->SetSfxVolume(fSfxVolume);
		Audio->SetAmbientVolume(fAmbientVolume);
		Audio->SetMuteWhenMinimizedEnabled(bMuteWhenMinimized);
	}
}

// ============================================================
// Save current settings to GameInstance + disk
// ============================================================

void UOptionsSubsystem::SaveToGameInstance()
{
	UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	GI->bOptionShowFPS = bShowFPS;
	GI->bOptionSkillEffects = bSkillEffects;
	GI->bOptionShowMissText = bShowMissText;
	GI->fOptionBrightness = fBrightness;
	GI->bOptionShowDamageNumbers = bShowDamageNumbers;
	GI->bOptionShowEnemyHPBars = bShowEnemyHPBars;
	GI->bOptionShowPlayerNames = bShowPlayerNames;
	GI->bOptionShowEnemyNames = bShowEnemyNames;
	GI->fOptionCameraSensitivity = fCameraSensitivity;
	GI->fOptionCameraZoomSpeed = fCameraZoomSpeed;
	GI->bOptionShowCastBars = bShowCastBars;
	GI->bOptionShowNPCNames = bShowNPCNames;
	GI->bOptionShowChatTimestamps = bShowChatTimestamps;
	GI->fOptionChatOpacity = fChatOpacity;
	GI->fOptionDamageNumberScale = fDamageNumberScale;
	GI->bOptionDropSoundMvp    = bDropSoundMvp;
	GI->bOptionDropSoundCard   = bDropSoundCard;
	GI->bOptionDropSoundEquip  = bDropSoundEquip;
	GI->bOptionDropSoundHeal   = bDropSoundHeal;
	GI->bOptionDropSoundUsable = bDropSoundUsable;
	GI->bOptionDropSoundMisc   = bDropSoundMisc;
	GI->bOptionMuteWhenMinimized = bMuteWhenMinimized;
	GI->fOptionMasterVolume  = fMasterVolume;
	GI->fOptionBgmVolume     = fBgmVolume;
	GI->fOptionSfxVolume     = fSfxVolume;
	GI->fOptionAmbientVolume = fAmbientVolume;
	GI->bOptionNoCtrl = bNoCtrl;
	GI->bOptionNoShift = bNoShift;
	GI->bOptionAutoDeclineTrades = bAutoDeclineTrades;
	GI->bOptionAutoDeclineParty = bAutoDeclineParty;
	GI->SaveGameOptions();
}

// ============================================================
// Display Setters
// ============================================================

void UOptionsSubsystem::SetShowFPS(bool bEnabled)
{
	bShowFPS = bEnabled;
	if (bShowFPS) AddFPSOverlay(); else RemoveFPSOverlay();
	SaveToGameInstance();
}

void UOptionsSubsystem::SetSkillEffects(bool bEnabled)
{
	bSkillEffects = bEnabled;
	if (auto* VFX = GetWorld()->GetSubsystem<USkillVFXSubsystem>())
		VFX->SetEffectsEnabled(bSkillEffects);
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowMissText(bool bEnabled)
{
	bShowMissText = bEnabled;
	if (auto* DMG = GetWorld()->GetSubsystem<UDamageNumberSubsystem>())
		DMG->bShowMissText = bShowMissText;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetBrightness(float Value)
{
	fBrightness = FMath::Clamp(Value, 0.5f, 2.0f);
	if (auto* PP = GetWorld()->GetSubsystem<UPostProcessSubsystem>())
		PP->SetBrightness(fBrightness);
	SaveToGameInstance();
}

// ============================================================
// Interface Setters
// ============================================================

void UOptionsSubsystem::SetShowDamageNumbers(bool bEnabled)
{
	bShowDamageNumbers = bEnabled;
	if (auto* DMG = GetWorld()->GetSubsystem<UDamageNumberSubsystem>())
		DMG->bDamageNumbersEnabled = bShowDamageNumbers;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowEnemyHPBars(bool bEnabled)
{
	bShowEnemyHPBars = bEnabled;
	if (auto* WHB = GetWorld()->GetSubsystem<UWorldHealthBarSubsystem>())
		WHB->bShowEnemyBars = bShowEnemyHPBars;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowPlayerNames(bool bEnabled)
{
	bShowPlayerNames = bEnabled;
	if (auto* NT = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NT->bShowPlayerNames = bShowPlayerNames;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowEnemyNames(bool bEnabled)
{
	bShowEnemyNames = bEnabled;
	if (auto* NT = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NT->bShowEnemyNames = bShowEnemyNames;
	SaveToGameInstance();
}

// ============================================================
// Camera Setters
// ============================================================

void UOptionsSubsystem::SetCameraSensitivity(float Value)
{
	fCameraSensitivity = FMath::Clamp(Value, 0.1f, 2.0f);
	if (auto* Cam = GetWorld()->GetSubsystem<UCameraSubsystem>())
		Cam->RotationSensitivity = fCameraSensitivity;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetCameraZoomSpeed(float Value)
{
	fCameraZoomSpeed = FMath::Clamp(Value, 20.f, 200.f);
	if (auto* Cam = GetWorld()->GetSubsystem<UCameraSubsystem>())
		Cam->ZoomSpeed = fCameraZoomSpeed;
	SaveToGameInstance();
}

// ============================================================
// Interface (extended) Setters
// ============================================================

void UOptionsSubsystem::SetShowCastBars(bool bEnabled)
{
	bShowCastBars = bEnabled;
	if (auto* CB = GetWorld()->GetSubsystem<UCastBarSubsystem>())
		CB->bCastBarsEnabled = bShowCastBars;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowNPCNames(bool bEnabled)
{
	bShowNPCNames = bEnabled;
	if (auto* NT = GetWorld()->GetSubsystem<UNameTagSubsystem>())
		NT->bShowNPCNames = bShowNPCNames;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetShowChatTimestamps(bool bEnabled)
{
	bShowChatTimestamps = bEnabled;
	if (auto* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->bShowTimestamps = bShowChatTimestamps;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetChatOpacity(float Value)
{
	fChatOpacity = FMath::Clamp(Value, 0.0f, 1.0f);
	if (auto* Chat = GetWorld()->GetSubsystem<UChatSubsystem>())
		Chat->ChatPanelOpacity = fChatOpacity;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetDamageNumberScale(float Value)
{
	fDamageNumberScale = FMath::Clamp(Value, 0.5f, 2.0f);
	SDamageNumberOverlay::FontScaleMultiplier = fDamageNumberScale;
	SaveToGameInstance();
}

// ============================================================
// Audio Setters
// ============================================================

void UOptionsSubsystem::SetMuteWhenMinimized(bool bEnabled)
{
	bMuteWhenMinimized = bEnabled;
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetMuteWhenMinimizedEnabled(bEnabled);
	}
	SaveToGameInstance();
}

// ---- Drop Sound Tier Setters ----
void UOptionsSubsystem::SetDropSoundMvp(bool b)    { bDropSoundMvp = b;    SaveToGameInstance(); }
void UOptionsSubsystem::SetDropSoundCard(bool b)   { bDropSoundCard = b;   SaveToGameInstance(); }
void UOptionsSubsystem::SetDropSoundEquip(bool b)  { bDropSoundEquip = b;  SaveToGameInstance(); }
void UOptionsSubsystem::SetDropSoundHeal(bool b)   { bDropSoundHeal = b;   SaveToGameInstance(); }
void UOptionsSubsystem::SetDropSoundUsable(bool b) { bDropSoundUsable = b; SaveToGameInstance(); }
void UOptionsSubsystem::SetDropSoundMisc(bool b)   { bDropSoundMisc = b;   SaveToGameInstance(); }

bool UOptionsSubsystem::ShouldPlayDropSound(const FString& TierColor) const
{
	if (TierColor == TEXT("red"))    return bDropSoundMvp;
	if (TierColor == TEXT("purple")) return bDropSoundCard;
	if (TierColor == TEXT("blue"))   return bDropSoundEquip;
	if (TierColor == TEXT("green"))  return bDropSoundHeal;
	if (TierColor == TEXT("yellow")) return bDropSoundUsable;
	if (TierColor == TEXT("pink"))   return bDropSoundMisc;
	return false;
}

void UOptionsSubsystem::SetMasterVolume(float V)
{
	fMasterVolume = FMath::Clamp(V, 0.0f, 1.0f);
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetMasterVolume(fMasterVolume);
	}
	SaveToGameInstance();
}

void UOptionsSubsystem::SetBgmVolume(float V)
{
	fBgmVolume = FMath::Clamp(V, 0.0f, 1.0f);
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetBgmVolume(fBgmVolume);
	}
	SaveToGameInstance();
}

void UOptionsSubsystem::SetSfxVolume(float V)
{
	fSfxVolume = FMath::Clamp(V, 0.0f, 1.0f);
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetSfxVolume(fSfxVolume);
	}
	SaveToGameInstance();
}

void UOptionsSubsystem::SetAmbientVolume(float V)
{
	fAmbientVolume = FMath::Clamp(V, 0.0f, 1.0f);
	if (UAudioSubsystem* Audio = GetWorld()->GetSubsystem<UAudioSubsystem>())
	{
		Audio->SetAmbientVolume(fAmbientVolume);
	}
	SaveToGameInstance();
}

// ============================================================
// Gameplay Setters (flags read by other subsystems via GameInstance)
// ============================================================

void UOptionsSubsystem::SetNoCtrl(bool bEnabled)
{
	bNoCtrl = bEnabled;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetNoShift(bool bEnabled)
{
	bNoShift = bEnabled;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetAutoDeclineTrades(bool bEnabled)
{
	bAutoDeclineTrades = bEnabled;
	SaveToGameInstance();
}

void UOptionsSubsystem::SetAutoDeclineParty(bool bEnabled)
{
	bAutoDeclineParty = bEnabled;
	SaveToGameInstance();
}

// ============================================================
// Viewport Integration — Options Panel
// ============================================================

void UOptionsSubsystem::AddOptionsWidgetToViewport()
{
	if (bOptionsWidgetAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	OptionsWidget = SNew(SOptionsWidget).Subsystem(this);
	OptionsWidget->SetVisibility(EVisibility::Collapsed);

	OptionsAlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Visibility(EVisibility::SelfHitTestInvisible)
		[
			OptionsWidget.ToSharedRef()
		];

	OptionsViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(OptionsAlignmentWrapper);
	VC->AddViewportWidgetContent(OptionsViewportOverlay.ToSharedRef(), OptionsZOrder);
	bOptionsWidgetAdded = true;
}

void UOptionsSubsystem::RemoveOptionsWidgetFromViewport()
{
	if (!bOptionsWidgetAdded) return;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (OptionsViewportOverlay.IsValid())
				VC->RemoveViewportWidgetContent(OptionsViewportOverlay.ToSharedRef());
		}
	}
	OptionsWidget.Reset();
	OptionsAlignmentWrapper.Reset();
	OptionsViewportOverlay.Reset();
	bOptionsWidgetAdded = false;
}

// ============================================================
// Viewport Integration — FPS Overlay
// ============================================================

void UOptionsSubsystem::AddFPSOverlay()
{
	if (bFPSOverlayAdded) return;
	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* VC = World->GetGameViewport();
	if (!VC) return;

	FPSWidget = SNew(SFPSCounterWidget);

	FPSAlignmentWrapper =
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Visibility(EVisibility::HitTestInvisible)
		[
			FPSWidget.ToSharedRef()
		];

	FPSViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(FPSAlignmentWrapper);
	VC->AddViewportWidgetContent(FPSViewportOverlay.ToSharedRef(), FPSZOrder);
	bFPSOverlayAdded = true;
}

void UOptionsSubsystem::RemoveFPSOverlay()
{
	if (!bFPSOverlayAdded) return;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (FPSViewportOverlay.IsValid())
				VC->RemoveViewportWidgetContent(FPSViewportOverlay.ToSharedRef());
		}
	}
	FPSWidget.Reset();
	FPSAlignmentWrapper.Reset();
	FPSViewportOverlay.Reset();
	bFPSOverlayAdded = false;
}
