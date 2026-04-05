#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UEscapeMenuSubsystem;

/**
 * SEscapeMenuWidget
 *
 * RO Classic "Select Option" popup. Centered on screen.
 * Buttons change based on alive/dead state.
 * Alive: Character Select, Hotkey, Exit, Cancel
 * Dead:  Save Point (respawn), Character Select, Exit, Cancel
 */
class SEscapeMenuWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEscapeMenuWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UEscapeMenuSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UEscapeMenuSubsystem> OwningSubsystem;

	/** Build a styled menu button. */
	TSharedRef<SWidget> BuildButton(const FText& Label, FOnClicked OnClicked);

	/** Build the title bar. */
	TSharedRef<SWidget> BuildTitleBar();
};
