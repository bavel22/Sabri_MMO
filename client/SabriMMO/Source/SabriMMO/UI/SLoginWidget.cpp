// SLoginWidget.cpp — Full-screen centered login/register form (RO Classic brown/gold theme)

#include "SLoginWidget.h"
#include "LoginFlowSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformMisc.h"

// ============================================================
// RO Classic Color Palette — Login Screen
// NOTE: Use FLinearColor() directly — FColor() applies sRGB->linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace LoginColors
{
	// Panel backgrounds
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium  (0.33f, 0.22f, 0.13f, 1.f);
	// Gold trim + highlights
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark     (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider  (0.60f, 0.48f, 0.22f, 1.f);
	// Text
	static const FLinearColor TextPrimary  (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright   (1.0f, 1.0f, 1.0f, 1.f);
	static const FLinearColor TextShadow   (0.0f, 0.0f, 0.0f, 0.85f);
	// Error
	static const FLinearColor ErrorRed     (0.90f, 0.20f, 0.20f, 1.f);
	// Buttons
	static const FLinearColor ButtonGreen  (0.20f, 0.55f, 0.20f, 1.f);
	static const FLinearColor ButtonRed    (0.55f, 0.15f, 0.15f, 1.f);
	static const FLinearColor ButtonBlue   (0.20f, 0.35f, 0.60f, 1.f);
	// Input fields
	static const FLinearColor FieldBg      (0.15f, 0.10f, 0.06f, 1.f);
	// Link text
	static const FLinearColor LinkGold     (0.85f, 0.72f, 0.35f, 1.f);
}

// ============================================================
// Construction
// ============================================================
void SLoginWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		// Full viewport overlay — visible but passes hits through to centered panel
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// Fixed-width login panel
			SNew(SBox)
			.WidthOverride(320.f)
			[
				// Outer gold trim border (2px)
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(LoginColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Inner dark inset (1px)
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(LoginColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Main brown panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(LoginColors::PanelBrown)
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)

							// --- Title Bar ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(LoginColors::PanelDark)
								.Padding(FMargin(0.f, 6.f))
								.HAlign(HAlign_Center)
								[
									SAssignNew(TitleTextBlock, STextBlock)
									.Text(FText::FromString(TEXT("SABRI MMO")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									.ColorAndOpacity(FSlateColor(LoginColors::GoldHighlight))
								]
							]

							// --- Gold divider after title ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(LoginColors::GoldDivider)
								.Padding(FMargin(0.f, 0.5f))
							]

							// --- Form fields area ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.f, 6.f))
							[
								SNew(SVerticalBox)

								// Username label
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Username")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FSlateColor(LoginColors::TextPrimary))
								]

								// Username input field
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(UsernameField, SEditableTextBox)
									.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
									.BackgroundColor(LoginColors::FieldBg)
									.ForegroundColor(FSlateColor(LoginColors::TextBright))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
								]

								// --- Email row (register only) ---
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
								[
									SAssignNew(EmailRow, SVerticalBox)
									.Visibility(EVisibility::Collapsed)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Email")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(LoginColors::TextPrimary))
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SAssignNew(EmailField, SEditableTextBox)
										.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
										.BackgroundColor(LoginColors::FieldBg)
										.ForegroundColor(FSlateColor(LoginColors::TextBright))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									]
								]

								// 4px spacing + Password label
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Password")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FSlateColor(LoginColors::TextPrimary))
								]

								// Password input field
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(PasswordField, SEditableTextBox)
									.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
									.BackgroundColor(LoginColors::FieldBg)
									.ForegroundColor(FSlateColor(LoginColors::TextBright))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									.IsPassword(true)
								]

								// --- Confirm Password row (register only) ---
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 4.f, 0.f, 0.f))
								[
									SAssignNew(ConfirmPasswordRow, SVerticalBox)
									.Visibility(EVisibility::Collapsed)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Confirm Password")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
										.ColorAndOpacity(FSlateColor(LoginColors::TextPrimary))
									]

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SAssignNew(ConfirmPasswordField, SEditableTextBox)
										.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
										.BackgroundColor(LoginColors::FieldBg)
										.ForegroundColor(FSlateColor(LoginColors::TextBright))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
										.IsPassword(true)
									]
								]

								// 6px spacing + Remember Username checkbox (login only)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
								[
									SAssignNew(RememberRow, SHorizontalBox)

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SAssignNew(RememberCheckbox, SCheckBox)
									]

									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									.Padding(FMargin(4.f, 0.f, 0.f, 0.f))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Remember Username")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.ColorAndOpacity(FSlateColor(LoginColors::TextPrimary))
									]
								]

								// 6px spacing + Error text (hidden by default)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
								[
									SAssignNew(ErrorTextBlock, STextBlock)
									.Text(FText::GetEmpty())
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FSlateColor(LoginColors::ErrorRed))
									.Visibility(EVisibility::Collapsed)
									.AutoWrapText(true)
								]

								// 6px bottom spacing
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(FMargin(0.f, 6.f, 0.f, 0.f))
								[
									SNullWidget::NullWidget
								]
							]

							// --- Gold divider before buttons ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(LoginColors::GoldDivider)
								.Padding(FMargin(0.f, 0.5f))
							]

							// --- Button row ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.f, 6.f))
							[
								SNew(SHorizontalBox)

								// Login/Register button (fills available space)
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(LoginColors::ButtonGreen)
									.OnClicked(this, &SLoginWidget::OnSubmitClicked)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(16.f, 4.f))
										[
											SAssignNew(SubmitButtonText, STextBlock)
											.Text(FText::FromString(TEXT("Login")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
											.ColorAndOpacity(FSlateColor(LoginColors::TextBright))
										]
									]
								]

								// 8px spacer
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(FMargin(8.f, 0.f, 0.f, 0.f))
								[
									// Exit button (auto width)
									SNew(SButton)
									.ButtonColorAndOpacity(LoginColors::ButtonRed)
									.OnClicked(this, &SLoginWidget::OnExitClicked)
									[
										SNew(SBox)
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										.Padding(FMargin(16.f, 4.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Exit")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
											.ColorAndOpacity(FSlateColor(LoginColors::TextBright))
										]
									]
								]
							]

							// --- Toggle mode link ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(FMargin(8.f, 0.f, 8.f, 6.f))
							.HAlign(HAlign_Center)
							[
								SNew(SButton)
								.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
								.OnClicked(this, &SLoginWidget::OnToggleModeClicked)
								.Cursor(EMouseCursor::Hand)
								[
									SAssignNew(ToggleModeText, STextBlock)
									.Text(FText::FromString(TEXT("Create Account")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FSlateColor(LoginColors::LinkGold))
								]
							]
						]
					]
				]
			]
		]
	];
}

// ============================================================
// Mode Toggle
// ============================================================
void SLoginWidget::UpdateModeVisuals()
{
	if (bIsRegisterMode)
	{
		TitleTextBlock->SetText(FText::FromString(TEXT("CREATE ACCOUNT")));
		SubmitButtonText->SetText(FText::FromString(TEXT("Register")));
		ToggleModeText->SetText(FText::FromString(TEXT("Back to Login")));
		EmailRow->SetVisibility(EVisibility::Visible);
		ConfirmPasswordRow->SetVisibility(EVisibility::Visible);
		RememberRow->SetVisibility(EVisibility::Collapsed);
	}
	else
	{
		TitleTextBlock->SetText(FText::FromString(TEXT("SABRI MMO")));
		SubmitButtonText->SetText(FText::FromString(TEXT("Login")));
		ToggleModeText->SetText(FText::FromString(TEXT("Create Account")));
		EmailRow->SetVisibility(EVisibility::Collapsed);
		ConfirmPasswordRow->SetVisibility(EVisibility::Collapsed);
		RememberRow->SetVisibility(EVisibility::Visible);
	}
	ClearError();
}

FReply SLoginWidget::OnToggleModeClicked()
{
	bIsRegisterMode = !bIsRegisterMode;
	UpdateModeVisuals();
	FocusAppropriateField();
	return FReply::Handled();
}

// ============================================================
// Error Display
// ============================================================
void SLoginWidget::ShowError(const FString& Message)
{
	if (ErrorTextBlock.IsValid())
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
		ErrorTextBlock->SetVisibility(EVisibility::Visible);
	}
}

void SLoginWidget::ClearError()
{
	if (ErrorTextBlock.IsValid())
	{
		ErrorTextBlock->SetText(FText::GetEmpty());
		ErrorTextBlock->SetVisibility(EVisibility::Collapsed);
	}
}

// ============================================================
// Pre-fill Helpers
// ============================================================
void SLoginWidget::SetUsername(const FString& Username)
{
	if (UsernameField.IsValid())
	{
		UsernameField->SetText(FText::FromString(Username));
	}
}

void SLoginWidget::SetRememberUsername(bool bRemember)
{
	if (RememberCheckbox.IsValid())
	{
		RememberCheckbox->SetIsChecked(bRemember ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	}
}

void SLoginWidget::FocusAppropriateField()
{
	if (UsernameField.IsValid() && !UsernameField->GetText().IsEmpty())
	{
		// Username already filled in, jump to password
		FSlateApplication::Get().SetKeyboardFocus(PasswordField, EFocusCause::SetDirectly);
	}
	else if (UsernameField.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(UsernameField, EFocusCause::SetDirectly);
	}
}

// ============================================================
// Button Handlers
// ============================================================
FReply SLoginWidget::OnSubmitClicked()
{
	if (bIsRegisterMode)
		AttemptRegister();
	else
		AttemptLogin();
	return FReply::Handled();
}

FReply SLoginWidget::OnExitClicked()
{
	if (OwningSubsystem.IsValid())
	{
		OwningSubsystem->OnExitRequested();
	}
	else
	{
		FPlatformMisc::RequestExit(false);
	}
	return FReply::Handled();
}

void SLoginWidget::AttemptLogin()
{
	if (!UsernameField.IsValid() || !PasswordField.IsValid())
	{
		return;
	}

	const FString Username = UsernameField->GetText().ToString();
	const FString Password = PasswordField->GetText().ToString();

	// Validate non-empty
	if (Username.IsEmpty())
	{
		ShowError(TEXT("Please enter your username."));
		FSlateApplication::Get().SetKeyboardFocus(UsernameField, EFocusCause::SetDirectly);
		return;
	}

	if (Password.IsEmpty())
	{
		ShowError(TEXT("Please enter your password."));
		FSlateApplication::Get().SetKeyboardFocus(PasswordField, EFocusCause::SetDirectly);
		return;
	}

	ClearError();

	const bool bRememberChecked = RememberCheckbox.IsValid()
		&& RememberCheckbox->IsChecked();

	if (OwningSubsystem.IsValid())
	{
		OwningSubsystem->OnLoginSubmitted(Username, Password, bRememberChecked);
	}
}

void SLoginWidget::AttemptRegister()
{
	if (!UsernameField.IsValid() || !EmailField.IsValid() || !PasswordField.IsValid() || !ConfirmPasswordField.IsValid())
		return;

	const FString Username = UsernameField->GetText().ToString();
	const FString Email = EmailField->GetText().ToString();
	const FString Password = PasswordField->GetText().ToString();
	const FString ConfirmPassword = ConfirmPasswordField->GetText().ToString();

	if (Username.IsEmpty() || Username.Len() < 3)
	{
		ShowError(TEXT("Username must be at least 3 characters."));
		FSlateApplication::Get().SetKeyboardFocus(UsernameField, EFocusCause::SetDirectly);
		return;
	}

	if (Email.IsEmpty() || !Email.Contains(TEXT("@")) || !Email.Contains(TEXT(".")))
	{
		ShowError(TEXT("Please enter a valid email address."));
		FSlateApplication::Get().SetKeyboardFocus(EmailField, EFocusCause::SetDirectly);
		return;
	}

	if (Password.Len() < 8)
	{
		ShowError(TEXT("Password must be at least 8 characters."));
		FSlateApplication::Get().SetKeyboardFocus(PasswordField, EFocusCause::SetDirectly);
		return;
	}

	if (Password != ConfirmPassword)
	{
		ShowError(TEXT("Passwords do not match."));
		FSlateApplication::Get().SetKeyboardFocus(ConfirmPasswordField, EFocusCause::SetDirectly);
		return;
	}

	ClearError();

	if (OwningSubsystem.IsValid())
	{
		OwningSubsystem->OnRegisterSubmitted(Username, Email, Password);
	}
}

// ============================================================
// Keyboard Input
// ============================================================
FReply SLoginWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter)
	{
		if (bIsRegisterMode)
			AttemptRegister();
		else
			AttemptLogin();
		return FReply::Handled();
	}

	if (Key == EKeys::Tab)
	{
		// Cycle focus through visible fields
		TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetKeyboardFocusedWidget();

		if (bIsRegisterMode)
		{
			if (Focused == UsernameField)
				FSlateApplication::Get().SetKeyboardFocus(EmailField, EFocusCause::Navigation);
			else if (Focused == EmailField)
				FSlateApplication::Get().SetKeyboardFocus(PasswordField, EFocusCause::Navigation);
			else if (Focused == PasswordField)
				FSlateApplication::Get().SetKeyboardFocus(ConfirmPasswordField, EFocusCause::Navigation);
			else
				FSlateApplication::Get().SetKeyboardFocus(UsernameField, EFocusCause::Navigation);
		}
		else
		{
			if (Focused == UsernameField)
				FSlateApplication::Get().SetKeyboardFocus(PasswordField, EFocusCause::Navigation);
			else
				FSlateApplication::Get().SetKeyboardFocus(UsernameField, EFocusCause::Navigation);
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Escape && bIsRegisterMode)
	{
		bIsRegisterMode = false;
		UpdateModeVisuals();
		FocusAppropriateField();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}
