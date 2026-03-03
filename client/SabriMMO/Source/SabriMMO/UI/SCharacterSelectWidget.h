// SCharacterSelectWidget.h -- Character selection screen (RO Classic style)
// Shows a 3x3 card grid of character slots on the left, detail panel on the right.
// Supports character selection, play, deletion with password confirmation, and create new.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class ULoginFlowSubsystem;
class SVerticalBox;
class STextBlock;
class SEditableTextBox;
class SBox;

class SCharacterSelectWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterSelectWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(ULoginFlowSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuild the card grid and detail panel from character data */
	void PopulateCharacters(const TArray<FCharacterData>& Characters);

	/** Show deletion confirmation dialog */
	void ShowDeleteConfirmation();
	void HideDeleteConfirmation();

	/** Show a brief status message */
	void ShowStatusMessage(const FString& Message);

private:
	TWeakObjectPtr<ULoginFlowSubsystem> OwningSubsystem;

	// Grid
	TSharedPtr<SVerticalBox> CardGridBox;
	TArray<FCharacterData> CachedCharacters;
	int32 SelectedIndex = -1;
	static constexpr int32 MaxSlots = 9;
	static constexpr int32 GridColumns = 3;

	// Detail panel
	TSharedPtr<SVerticalBox> DetailPanelBox;

	// Delete confirmation overlay
	TSharedPtr<SWidget> DeleteOverlay;
	TSharedPtr<SEditableTextBox> DeletePasswordField;
	TSharedPtr<STextBlock> DeleteErrorText;

	// Status message
	TSharedPtr<STextBlock> StatusText;

	// Build methods
	TSharedRef<SWidget> BuildCardGrid();
	TSharedRef<SWidget> BuildDetailPanel();
	TSharedRef<SWidget> BuildCharacterCard(int32 SlotIndex);
	TSharedRef<SWidget> BuildDeleteConfirmationOverlay();
	void RebuildCardGrid();
	void RebuildDetailPanel();
	void SelectSlot(int32 SlotIndex);

	// Actions
	FReply OnPlayClicked();
	FReply OnDeleteClicked();
	FReply OnBackClicked();
	FReply OnConfirmDeleteClicked();
	FReply OnCancelDeleteClicked();

	// Helpers
	TSharedRef<SWidget> BuildStatRow(const FString& Label, const FString& Value);
	static FString FormatJobClass(const FString& JobClass);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
};
