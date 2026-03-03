// SLoadingOverlayWidget.h — Full-screen "please wait" overlay for async operations (RO Classic style)

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;

class SLoadingOverlayWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadingOverlayWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Update the status text shown in the overlay */
	void SetStatusText(const FString& Text);

	/** Show/hide the overlay */
	void Show();
	void Hide();
	bool IsShowing() const;

private:
	TSharedPtr<STextBlock> StatusTextBlock;
	bool bIsVisible = false;
};
