// SWorldHealthBarOverlay.h — Fullscreen transparent Slate overlay that renders
// RO-style floating HP/SP bars below the local player and enemy characters.
// All bar rendering is done in OnPaint via FSlateDrawElement::MakeBox.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class UWorldHealthBarSubsystem;

// ============================================================
// RO Classic Floating Bar Colors
// CRITICAL: All FLinearColor — never FColor (sRGB conversion = dark/black)
// ============================================================
namespace WorldBarColors
{
	// Border — dark navy blue (#10189C)
	static const FLinearColor NavyBorder    (0.063f, 0.094f, 0.612f, 1.f);

	// Background — dark gray (#424242)
	static const FLinearColor DarkGrayBg    (0.259f, 0.259f, 0.259f, 1.f);

	// Player HP — bright green (#10EF21), turns red (#FF0000) at <=25%
	static const FLinearColor PlayerHPGreen (0.063f, 0.937f, 0.129f, 1.f);
	static const FLinearColor PlayerHPCritRed(1.0f,  0.0f,   0.0f,   1.f);

	// Player SP — royal blue (#1863DE)
	static const FLinearColor PlayerSPBlue  (0.094f, 0.388f, 0.871f, 1.f);

	// Enemy HP — hot pink/magenta (#FF00E7), turns yellow (#FFFF00) at <=25%
	static const FLinearColor EnemyHPPink   (1.0f,   0.0f,   0.906f, 1.f);
	static const FLinearColor EnemyHPCritYellow(1.0f, 1.0f,  0.0f,   1.f);

	// NPC name text — white with dark shadow
	static const FLinearColor NPCNameWhite  (1.0f,   1.0f,   1.0f,   1.f);
	static const FLinearColor NPCNameShadow (0.0f,   0.0f,   0.0f,   0.7f);
}

class SWorldHealthBarOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldHealthBarOverlay) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UWorldHealthBarSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	// Active timer for continuous repaint
	EActiveTimerReturnType OnRepaintTick(double InCurrentTime, float InDeltaTime);

	TWeakObjectPtr<UWorldHealthBarSubsystem> OwningSubsystem;

	// ---- Bar dimensions (in Slate local pixels) ----
	static constexpr float BAR_WIDTH      = 80.f;   // Total width including border
	static constexpr float BAR_FILL_H     = 5.f;    // Height of HP or SP fill area
	static constexpr float BAR_BORDER     = 1.f;    // Border thickness
	static constexpr float BAR_DIVIDER    = 1.f;    // Divider between HP and SP (same as border)
	static constexpr float VERTICAL_OFFSET = -8.f;  // Negative = above feet, positive = below. Adjusted for sprite.
	static constexpr float CRITICAL_THRESHOLD = 0.25f; // 25% HP triggers color change

	// Player bar total height: border + fill + divider + fill + border = 1+5+1+5+1 = 13
	static constexpr float PLAYER_BAR_H  = BAR_BORDER + BAR_FILL_H + BAR_DIVIDER + BAR_FILL_H + BAR_BORDER;

	// Enemy bar total height: border + fill + border = 1+5+1 = 7
	static constexpr float ENEMY_BAR_H   = BAR_BORDER + BAR_FILL_H + BAR_BORDER;

	// ---- Drawing helpers ----
	void DrawPlayerBar(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& AllottedGeometry, float InvScale,
		const FVector2D& ScreenPos, float HPPercent, float SPPercent, bool bCritical) const;

	void DrawEnemyBar(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& AllottedGeometry, float InvScale,
		const FVector2D& ScreenPos, float HPPercent, bool bCritical) const;

	void DrawFilledBar(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& AllottedGeometry,
		float X, float Y, float Width, float Height,
		float FillPercent, const FLinearColor& FillColor) const;

	void DrawNPCName(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& AllottedGeometry, float InvScale,
		const FVector2D& ScreenPos, const FString& Name) const;

	// NPC name text offset above the actor's head
	static constexpr float NPC_NAME_OFFSET_Y = -30.f;  // Above the projected position
	static constexpr float NPC_NAME_FONT_SIZE = 12.f;
};
