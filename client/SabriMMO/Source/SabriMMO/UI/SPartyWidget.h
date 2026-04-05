// SPartyWidget.h — RO Classic party window Slate widget.
// Shows party members with HP bars, leader indicator, online status,
// and map names. Supports drag-to-move, invite popup, and right-click
// context menu (kick/delegate leader).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"

class UPartySubsystem;
struct FPartyMember;

class SPartyWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPartyWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UPartySubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// Data source
	TWeakObjectPtr<UPartySubsystem> OwningSubsystem;
	uint32 LastRenderedVersion = 0;
	bool bLastHadInvite = false;

	// Drag support
	bool bIsDragging = false;
	FVector2D DragOffset;
	FVector2D DragStartWidgetPos;
	FVector2D WidgetPosition = FVector2D(800.f, 50.f);

	// Sizes
	float WidgetWidth = 260.f;
	float TitleBarHeight = 22.f;
	float NavBarHeight = 24.f;
	float MemberRowHeight = 38.f;
	float FooterHeight = 20.f;

	// Content
	TSharedPtr<SVerticalBox> MemberListBox;
	TSharedPtr<SVerticalBox> InvitePopupBox;
	TSharedPtr<SBox> RootSizeBox;

	// Text input fields
	TSharedPtr<SEditableTextBox> CreateNameInput;
	TSharedPtr<SEditableTextBox> InviteNameInput;

	// Settings popup
	TSharedPtr<SVerticalBox> SettingsPopupBox;
	bool bSettingsOpen = false;
	void RebuildSettingsPopup();

	// Build methods
	void RebuildMemberList();
	void RebuildInvitePopup();
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildNavBar();
	TSharedRef<SWidget> BuildFooter();
	TSharedRef<SWidget> BuildMemberRow(const FPartyMember& Member);
	TSharedRef<SWidget> BuildStatBar(float Percent, const FLinearColor& FillColor);

	// Interactions
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void ApplyLayout();

	// Colors (RO Classic Party Window)
	static const FLinearColor PanelBrown;
	static const FLinearColor PanelDark;
	static const FLinearColor GoldTrim;
	static const FLinearColor GoldHighlight;
	static const FLinearColor ContentWhite;
	static const FLinearColor MemberNameTeal;
	static const FLinearColor LeaderNameBlue;
	static const FLinearColor OfflineGray;
	static const FLinearColor HPBarGreen;
	static const FLinearColor HPBarRed;
	static const FLinearColor HPBarBorder;
	static const FLinearColor HPBarBackground;
};
