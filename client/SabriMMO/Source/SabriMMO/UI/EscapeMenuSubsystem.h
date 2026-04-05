#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EscapeMenuSubsystem.generated.h"

class SEscapeMenuWidget;
class UMMOGameInstance;

/**
 * UEscapeMenuSubsystem
 *
 * Manages the RO Classic ESC menu (Select Option popup).
 * Toggle with ESC key. Shows Character Select / Hotkey / Exit / Cancel.
 * When dead: shows Respawn (Save Point) instead of settings buttons.
 */
UCLASS()
class SABRIMMO_API UEscapeMenuSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Toggle the ESC menu open/closed. Called by SabriMMOCharacter ESC key binding. */
	void ToggleMenu();

	/** Show/hide explicitly. */
	void ShowMenu();
	void HideMenu();

	bool IsMenuVisible() const { return bMenuVisible; }

	/** Check if the local player is dead (for showing respawn buttons). */
	bool IsPlayerDead() const;

	/** Button actions — called by SEscapeMenuWidget. */
	void OnCharacterSelectPressed();
	void OnRespawnPressed();
	void OnHotkeyPressed();
	void OnExitGamePressed();

private:
	bool bMenuVisible = false;
	bool bWidgetAdded = false;

	TSharedPtr<SEscapeMenuWidget> Widget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;

	void AddWidgetToViewport();
	void RemoveWidgetFromViewport();

	UMMOGameInstance* GetGI() const;

	static constexpr int32 EscMenuZOrder = 40;
};
