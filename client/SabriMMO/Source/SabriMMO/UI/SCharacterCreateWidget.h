// SCharacterCreateWidget.h -- Character creation screen (RO Classic style)
// Shows: Name field, class (locked Novice), gender toggle, hair style/color pickers, preview

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ULoginFlowSubsystem;
class SEditableTextBox;
class STextBlock;

class SCharacterCreateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterCreateWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(ULoginFlowSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show an error message */
	void ShowError(const FString& Message);
	void ClearError();

private:
	TWeakObjectPtr<ULoginFlowSubsystem> OwningSubsystem;

	TSharedPtr<SEditableTextBox> NameField;
	TSharedPtr<STextBlock> ErrorTextBlock;
	TSharedPtr<STextBlock> PreviewText;
	TSharedPtr<STextBlock> HairStyleText;
	TSharedPtr<STextBlock> HairColorText;

	// Creation state
	bool bIsMale = true;
	int32 CurrentHairStyle = 1;
	int32 CurrentHairColor = 0;

	static constexpr int32 MinHairStyle = 1;
	static constexpr int32 MaxHairStyle = 19;
	static constexpr int32 MinHairColor = 0;
	static constexpr int32 MaxHairColor = 8;

	// Helpers
	void UpdatePreview();
	static FString GetHairColorName(int32 ColorIndex);

	// Actions
	FReply OnCreateClicked();
	FReply OnCancelClicked();
	FReply OnMaleClicked();
	FReply OnFemaleClicked();
	FReply OnHairStyleLeft();
	FReply OnHairStyleRight();
	FReply OnHairColorLeft();
	FReply OnHairColorRight();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
};
