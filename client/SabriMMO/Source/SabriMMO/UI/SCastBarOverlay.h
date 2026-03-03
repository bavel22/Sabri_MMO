// SCastBarOverlay.h — Fullscreen transparent Slate overlay that renders
// RO-style cast bars above character heads via OnPaint with world-to-screen
// projection. Shows skill name + green progress bar during casting.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UCastBarSubsystem;

class SCastBarOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCastBarOverlay) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UCastBarSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType OnAnimationTick(double InCurrentTime, float InDeltaTime);

	// Find the world position of a caster by their characterId
	FVector FindCasterWorldPosition(int32 CasterId) const;

	// Project world position to screen coordinates
	bool ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const;

	TWeakObjectPtr<UCastBarSubsystem> OwningSubsystem;

	// ---- Visual Constants (matching RO reference screenshot) ----
	static constexpr float BAR_WIDTH = 140.0f;
	static constexpr float BAR_HEIGHT = 12.0f;
	static constexpr float BAR_BORDER = 2.0f;
	static constexpr float LABEL_PADDING_H = 6.0f;       // Horizontal padding inside label bg
	static constexpr float LABEL_PADDING_V = 2.0f;       // Vertical padding inside label bg
	static constexpr float LABEL_BAR_GAP = 2.0f;          // Gap between label and bar
	static constexpr float HEAD_OFFSET_Z = 200.0f;        // World units above actor origin
	static constexpr int32 SKILL_NAME_FONT_SIZE = 9;
};
