#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OptionsSubsystem.generated.h"

class SOptionsWidget;

/**
 * UOptionsSubsystem
 *
 * Manages the Options panel (opened from ESC menu) and the FPS counter overlay.
 * All settings are persisted on GameInstance (survives zone transitions)
 * and to SabriMMO.ini (survives game restarts).
 */
UCLASS()
class SABRIMMO_API UOptionsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowOptionsPanel();
	void HideOptionsPanel();
	bool IsOptionsPanelVisible() const { return bOptionsPanelVisible; }

	// ---- Display ----
	bool IsShowFPS() const { return bShowFPS; }
	void SetShowFPS(bool bEnabled);

	bool IsSkillEffects() const { return bSkillEffects; }
	void SetSkillEffects(bool bEnabled);

	bool IsShowMissText() const { return bShowMissText; }
	void SetShowMissText(bool bEnabled);

	float GetBrightness() const { return fBrightness; }
	void SetBrightness(float Value);

	// ---- Interface ----
	bool IsShowDamageNumbers() const { return bShowDamageNumbers; }
	void SetShowDamageNumbers(bool bEnabled);

	bool IsShowEnemyHPBars() const { return bShowEnemyHPBars; }
	void SetShowEnemyHPBars(bool bEnabled);

	bool IsShowPlayerNames() const { return bShowPlayerNames; }
	void SetShowPlayerNames(bool bEnabled);

	bool IsShowEnemyNames() const { return bShowEnemyNames; }
	void SetShowEnemyNames(bool bEnabled);

	// ---- Camera ----
	float GetCameraSensitivity() const { return fCameraSensitivity; }
	void SetCameraSensitivity(float Value);

	float GetCameraZoomSpeed() const { return fCameraZoomSpeed; }
	void SetCameraZoomSpeed(float Value);

	// ---- Interface (extended) ----
	bool IsShowCastBars() const { return bShowCastBars; }
	void SetShowCastBars(bool bEnabled);

	bool IsShowNPCNames() const { return bShowNPCNames; }
	void SetShowNPCNames(bool bEnabled);

	bool IsShowChatTimestamps() const { return bShowChatTimestamps; }
	void SetShowChatTimestamps(bool bEnabled);

	float GetChatOpacity() const { return fChatOpacity; }
	void SetChatOpacity(float Value);

	float GetDamageNumberScale() const { return fDamageNumberScale; }
	void SetDamageNumberScale(float Value);

	// ---- Drop Sounds (per tier toggle) ----
	bool IsDropSoundMvp() const    { return bDropSoundMvp; }
	bool IsDropSoundCard() const   { return bDropSoundCard; }
	bool IsDropSoundEquip() const  { return bDropSoundEquip; }
	bool IsDropSoundHeal() const   { return bDropSoundHeal; }
	bool IsDropSoundUsable() const { return bDropSoundUsable; }
	bool IsDropSoundMisc() const   { return bDropSoundMisc; }
	void SetDropSoundMvp(bool b);
	void SetDropSoundCard(bool b);
	void SetDropSoundEquip(bool b);
	void SetDropSoundHeal(bool b);
	void SetDropSoundUsable(bool b);
	void SetDropSoundMisc(bool b);

	/** Check if a given tier color should play a drop sound. */
	bool ShouldPlayDropSound(const FString& TierColor) const;

	// ---- Audio ----
	bool IsMuteWhenMinimized() const { return bMuteWhenMinimized; }
	void SetMuteWhenMinimized(bool bEnabled);

	float GetMasterVolume() const  { return fMasterVolume;  }
	void  SetMasterVolume(float V);

	float GetBgmVolume() const     { return fBgmVolume;     }
	void  SetBgmVolume(float V);

	float GetSfxVolume() const     { return fSfxVolume;     }
	void  SetSfxVolume(float V);

	float GetAmbientVolume() const { return fAmbientVolume; }
	void  SetAmbientVolume(float V);

	// ---- Gameplay ----
	bool IsNoCtrl() const { return bNoCtrl; }
	void SetNoCtrl(bool bEnabled);

	bool IsNoShift() const { return bNoShift; }
	void SetNoShift(bool bEnabled);

	bool IsAutoDeclineTrades() const { return bAutoDeclineTrades; }
	void SetAutoDeclineTrades(bool bEnabled);

	bool IsAutoDeclineParty() const { return bAutoDeclineParty; }
	void SetAutoDeclineParty(bool bEnabled);

	// ---- Video / Sprite Quality ----
	// 0 = Ultra (full res), 1 = High, 2 = Medium, 3 = Low — maps to LODBias on every sprite atlas.
	int32 GetSpriteQuality() const { return iSpriteQuality; }
	void SetSpriteQuality(int32 NewValue);

private:
	/** Iterate already-loaded sprite atlas textures and apply current LODBias.
	 *  Called when SetSpriteQuality changes value. New atlases loaded after this
	 *  pick up the bias automatically via FSingleAnimAtlasInfo::GlobalLODBias. */
	void ApplySpriteQualityToLoadedTextures();

	bool bOptionsPanelVisible = false;
	bool bOptionsWidgetAdded = false;
	bool bFPSOverlayAdded = false;

	// Local mirror of all settings (restored from GameInstance on BeginPlay)
	bool bShowFPS = false;
	bool bSkillEffects = true;
	bool bShowMissText = true;
	float fBrightness = 1.0f;
	bool bShowDamageNumbers = true;
	bool bShowEnemyHPBars = true;
	bool bShowPlayerNames = true;
	bool bShowEnemyNames = true;
	float fCameraSensitivity = 0.6f;
	float fCameraZoomSpeed = 80.f;
	bool bShowCastBars = true;
	bool bShowNPCNames = true;
	bool bShowChatTimestamps = false;
	float fChatOpacity = 0.90f;
	float fDamageNumberScale = 1.0f;
	bool bDropSoundMvp    = true;
	bool bDropSoundCard   = true;
	bool bDropSoundEquip  = false;
	bool bDropSoundHeal   = false;
	bool bDropSoundUsable = false;
	bool bDropSoundMisc   = false;
	bool bMuteWhenMinimized = true;
	float fMasterVolume  = 1.0f;
	float fBgmVolume     = 0.7f;
	float fSfxVolume     = 1.0f;
	float fAmbientVolume = 0.5f;
	bool bNoCtrl = true;
	bool bNoShift = false;
	bool bAutoDeclineTrades = false;
	bool bAutoDeclineParty = false;
	int32 iSpriteQuality = 1;  // High by default (LODBias 1 → effective half-res)

	TSharedPtr<SOptionsWidget> OptionsWidget;
	TSharedPtr<SWidget> OptionsAlignmentWrapper;
	TSharedPtr<SWidget> OptionsViewportOverlay;

	TSharedPtr<SWidget> FPSWidget;
	TSharedPtr<SWidget> FPSAlignmentWrapper;
	TSharedPtr<SWidget> FPSViewportOverlay;

	void AddOptionsWidgetToViewport();
	void RemoveOptionsWidgetFromViewport();
	void AddFPSOverlay();
	void RemoveFPSOverlay();

	void PushSettingsToSubsystems();
	void SaveToGameInstance();

	// Z-order 210 puts the Options panel above ESC menu (40) AND above the login
	// background (200) + login widgets (201), so it can be opened from both contexts.
	static constexpr int32 OptionsZOrder = 210;
	static constexpr int32 FPSZOrder = 50;
};
