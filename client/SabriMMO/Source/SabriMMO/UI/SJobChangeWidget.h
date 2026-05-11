// SJobChangeWidget.h — Slate widget for the Job Master class change dialog.
// Three pages: Greeting -> Selection -> Congrats. Centered + draggable from title bar.
// Reads state from UJobChangeSubsystem via TAttribute lambdas (no manual invalidation).

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UJobChangeSubsystem;
class STextBlock;
class SBox;

class SJobChangeWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJobChangeWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UJobChangeSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UJobChangeSubsystem> OwningSubsystem;
	TSharedPtr<SBox> RootSizeBox;

	// Build methods — one per visual page
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildPageContent();
	TSharedRef<SWidget> BuildGreetingPage();
	TSharedRef<SWidget> BuildSelectionPage();
	TSharedRef<SWidget> BuildCongratsPage();
	TSharedRef<SWidget> BuildClassButton(const FString& ClassId, const FString& DisplayName);

	// Drag state — DPI-correct pattern from sabrimmo-ui Phase 7.
	// WidgetPosition is an offset from the alignment-wrapper-applied center.
	// (Alignment wrapper is HAlign_Center+VAlign_Center, so (0,0) means dead-center.)
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;          // start mouse pos (absolute screen)
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;  // start widget offset (local)
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	static constexpr float TitleBarHeight = 28.f;
	static constexpr float WidgetWidth = 360.f;
	static constexpr float WidgetHeight = 320.f;

	void ApplyLayout();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
};
