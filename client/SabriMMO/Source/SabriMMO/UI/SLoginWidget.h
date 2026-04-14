// SLoginWidget.h — Full-screen centered login form (RO Classic style)
// Shows: Username/Password fields, Remember checkbox, Login/Exit buttons

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ULoginFlowSubsystem;
class SEditableTextBox;
class STextBlock;
class SCheckBox;

class SLoginWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoginWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(ULoginFlowSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Show an error message below the form */
	void ShowError(const FString& Message);
	void ClearError();

	/** Pre-fill username from remembered settings */
	void SetUsername(const FString& Username);
	void SetRememberUsername(bool bRemember);

	/** Focus the appropriate field */
	void FocusAppropriateField();

private:
	TWeakObjectPtr<ULoginFlowSubsystem> OwningSubsystem;

	// Shared fields
	TSharedPtr<SEditableTextBox> UsernameField;
	TSharedPtr<SEditableTextBox> PasswordField;
	TSharedPtr<SCheckBox> RememberCheckbox;
	TSharedPtr<STextBlock> ErrorTextBlock;

	// Register-only fields
	TSharedPtr<SEditableTextBox> EmailField;
	TSharedPtr<SEditableTextBox> ConfirmPasswordField;

	// Containers for toggling login/register visibility
	TSharedPtr<SWidget> EmailRow;
	TSharedPtr<SWidget> ConfirmPasswordRow;
	TSharedPtr<SWidget> RememberRow;
	TSharedPtr<STextBlock> TitleTextBlock;
	TSharedPtr<STextBlock> SubmitButtonText;
	TSharedPtr<STextBlock> ToggleModeText;

	bool bIsRegisterMode = false;

	// Button handlers
	FReply OnSubmitClicked();
	FReply OnExitClicked();
	FReply OnOptionsClicked();
	FReply OnToggleModeClicked();
	void AttemptLogin();
	void AttemptRegister();
	void UpdateModeVisuals();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
};
