// SWorldMapWidget.h — Full-screen RO Classic world map overlay.
// Shows continent illustration with hoverable zone rectangles,
// party member dots, current location arrow, zone names, monster info.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class UMinimapSubsystem;

class SWorldMapWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldMapWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UMinimapSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UMinimapSubsystem> OwningSubsystem;

	// World map texture brush
	FSlateBrush WorldMapBrush;
	bool bHasTexture = false;

	// Hovered zone
	mutable FString HoveredZoneName;
	mutable FVector2D LastMousePos = FVector2D::ZeroVector;

	// Paint helpers
	void DrawBackground(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawZoneRectangles(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawCurrentLocation(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawPartyMembers(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawZoneTooltip(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawInstructions(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;

	// Convert normalized bounds (0-1) to pixel coordinates on the map area
	FVector2D NormToPixel(const FGeometry& Geo, float NormX, float NormY) const;
	FBox2D GetZonePixelBounds(const FGeometry& Geo, float X1, float Y1, float X2, float Y2) const;
};
