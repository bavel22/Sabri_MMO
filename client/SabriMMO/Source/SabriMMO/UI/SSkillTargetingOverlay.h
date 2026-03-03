// SSkillTargetingOverlay.h — Full-screen transparent Slate overlay for skill targeting mode.
// Captures mouse/keyboard input. Renders a banner with skill name and targeting instructions.
// Cursor changes to crosshairs while active.  Right-click or ESC cancels.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE(FOnTargetingLeftClick);
DECLARE_DELEGATE(FOnTargetingCancelled);

class SSkillTargetingOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSkillTargetingOverlay)
		: _SkillName(TEXT("Skill"))
		, _TargetingHint(TEXT("Left-click target — Right-click / ESC to cancel"))
	{}
		SLATE_ARGUMENT(FString, SkillName)
		SLATE_ARGUMENT(FString, TargetingHint)
		SLATE_EVENT(FOnTargetingLeftClick, OnLeftClick)
		SLATE_EVENT(FOnTargetingCancelled, OnCancelled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// ---- input overrides ----
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

private:
	FOnTargetingLeftClick OnLeftClick;
	FOnTargetingCancelled OnCancelled;
};
