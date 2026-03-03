// SServerSelectWidget.h -- Server selection screen (RO Classic style)
// Shows a list of available servers with status, population, and connect/cancel buttons.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class ULoginFlowSubsystem;
class SVerticalBox;

class SServerSelectWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SServerSelectWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(ULoginFlowSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuild the server list from data */
	void PopulateServerList(const TArray<FServerInfo>& Servers);

private:
	TWeakObjectPtr<ULoginFlowSubsystem> OwningSubsystem;
	TSharedPtr<SVerticalBox> ServerListBox;
	int32 SelectedIndex = -1;
	TArray<FServerInfo> CachedServers;

	TSharedRef<SWidget> BuildServerRow(const FServerInfo& Server, int32 Index);
	void SelectServer(int32 Index);

	FReply OnConnectClicked();
	FReply OnCancelClicked();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
};
