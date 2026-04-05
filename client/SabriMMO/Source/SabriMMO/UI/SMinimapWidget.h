// SMinimapWidget.h — RO Classic-style minimap overlay.
// Live overhead SceneCapture, entity dots, draggable, zoom +/-.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"

class UMinimapSubsystem;

class SMinimapWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMinimapWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UMinimapSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	// Mouse input for dragging
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TWeakObjectPtr<UMinimapSubsystem> OwningSubsystem;

	// Render target brush (displays the overhead capture)
	mutable FSlateBrush CaptureBrush;
	mutable bool bCaptureBrushReady = false;
	void EnsureCaptureBrush() const;

	// Paint helpers
	void DrawMinimapBackground(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawPlayerArrow(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawWarpDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawEnemyDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawOtherPlayerDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawNPCDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;
	void DrawMinimapMarks(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const;

	// Convert a world position to minimap pixel coords (relative to player-centered camera)
	FVector2D WorldToMinimapPixel(const FVector& WorldPos) const;

	// Drag state
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(0.f, 0.f);  // render transform offset
	void ApplyPosition();

	// Timing for mark blinking
	mutable float AccumulatedTime = 0.f;
	mutable double LastTickTime = 0.0;

	// Constants — compact layout: frame + map + title overlay inside
	static constexpr float MapSize = 128.f;
	static constexpr float FramePad = 3.f;   // gold border thickness
	static constexpr float TotalSize = MapSize + FramePad * 2.f;  // 134x134
};
