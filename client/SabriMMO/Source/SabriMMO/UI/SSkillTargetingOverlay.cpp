// SSkillTargetingOverlay.cpp — Skill targeting overlay implementation.
// Full-screen transparent widget that intercepts clicks for RO-style skill targeting.

#include "SSkillTargetingOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

void SSkillTargetingOverlay::Construct(const FArguments& InArgs)
{
	OnLeftClick = InArgs._OnLeftClick;
	OnCancelled = InArgs._OnCancelled;

	ChildSlot
	[
		SNew(SVerticalBox)

		// ── Banner at top of screen ──
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.f, 30.f, 0.f, 0.f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.12f, 0.08f, 0.05f, 0.9f))
			.Padding(FMargin(28.f, 10.f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(InArgs._SkillName))
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 18))
					.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.3f, 1.0f))  // Gold
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(InArgs._TargetingHint))
					.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12))
					.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
				]
			]
		]

		// ── Transparent full-screen area (hit-testable so clicks bubble to this widget) ──
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.f))
			.Visibility(EVisibility::Visible)
		]
	];
}

// ────────────────────────────────────────────────────────
// Mouse: Left-click → execute targeting, Right-click → cancel
// ────────────────────────────────────────────────────────

FReply SSkillTargetingOverlay::OnMouseButtonDown(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnLeftClick.ExecuteIfBound();
		return FReply::Handled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnCancelled.ExecuteIfBound();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ────────────────────────────────────────────────────────
// Keyboard: ESC → cancel targeting
// ────────────────────────────────────────────────────────

FReply SSkillTargetingOverlay::OnKeyDown(
	const FGeometry& MyGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCancelled.ExecuteIfBound();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ────────────────────────────────────────────────────────
// Cursor: show crosshairs while targeting
// ────────────────────────────────────────────────────────

FCursorReply SSkillTargetingOverlay::OnCursorQuery(
	const FGeometry& MyGeometry,
	const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Crosshairs);
}
