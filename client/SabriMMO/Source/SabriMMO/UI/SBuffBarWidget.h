// SBuffBarWidget.h — Slate widget showing active buffs and status effects.
// Displays a horizontal row of small colored boxes with 3-letter abbreviations
// and countdown timers. RO Classic style.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"

class UBuffBarSubsystem;

class SBuffBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBuffBarWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UBuffBarSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	TWeakObjectPtr<UBuffBarSubsystem> OwningSubsystem;

	// Cached icon box for rebuilds
	TSharedPtr<SHorizontalBox> IconRow;
	int32 LastIconCount = -1; // Force rebuild on first tick

	void RebuildIcons();
	TSharedRef<SWidget> BuildSingleIcon(bool bIsStatus, int32 Index);

	// Color lookup
	static FLinearColor GetStatusColor(const FString& Type);
	static FLinearColor GetBuffCategoryColor(const FString& Category);
	static FString GetStatusAbbrev(const FString& Type);

	// Position offset (below BasicInfoWidget)
	FVector2D WidgetPosition = FVector2D(10.0, 175.0);
};
