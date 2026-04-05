// SSummonOverlay.h — Full-viewport OnPaint overlay that renders summon indicators
// at world-projected screen positions. Draws colored ground markers, name tags,
// HP bars, and duration timers for Flora plants and Marine Spheres.
// Pure C++ — no Blueprint actors or mesh assets required.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class USummonSubsystem;

namespace SummonColors
{
	// Plant ground marker — green circle
	static const FLinearColor PlantMarker     (0.15f, 0.70f, 0.20f, 0.60f);
	static const FLinearColor PlantMarkerOwn  (0.20f, 0.85f, 0.30f, 0.70f);

	// Sphere ground marker — orange/red pulsing
	static const FLinearColor SphereMarker    (0.90f, 0.40f, 0.10f, 0.65f);
	static const FLinearColor SphereMarkerOwn (1.00f, 0.50f, 0.15f, 0.75f);

	// Name text
	static const FLinearColor NameText        (0.96f, 0.90f, 0.78f, 1.0f);
	static const FLinearColor NameShadow      (0.0f,  0.0f,  0.0f,  0.85f);

	// HP bar
	static const FLinearColor HPGreen         (0.10f, 0.85f, 0.15f, 1.0f);
	static const FLinearColor HPBg            (0.15f, 0.15f, 0.15f, 0.8f);
	static const FLinearColor HPBorder        (0.06f, 0.09f, 0.61f, 1.0f);

	// Duration timer text
	static const FLinearColor TimerText       (0.80f, 0.80f, 0.60f, 1.0f);
}

class SSummonOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSummonOverlay) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(USummonSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType OnRepaintTick(double InCurrentTime, float InDeltaTime);
	TWeakObjectPtr<USummonSubsystem> OwningSubsystem;

	// Drawing constants
	static constexpr float MARKER_SIZE   = 24.f;
	static constexpr float HP_BAR_WIDTH  = 60.f;
	static constexpr float HP_BAR_HEIGHT = 4.f;
	static constexpr float NAME_OFFSET_Y = -8.f;  // Above marker
	static constexpr float HP_OFFSET_Y   = 16.f;  // Below marker

	void DrawPlantIndicator(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geo, float InvScale,
		const FVector2D& ScreenPos, const FString& Name, float HPPct, float TimeRemaining, bool bIsOwn) const;

	void DrawSphereIndicator(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geo, float InvScale,
		const FVector2D& ScreenPos, float HPPct, float TimeRemaining, bool bIsOwn) const;
};
