// SDamageNumberOverlay.h — Fullscreen transparent Slate overlay that renders
// RO-style damage numbers via OnPaint with per-digit fan-out animation.
// All rendering is done in OnPaint for maximum performance and control.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Fonts/SlateFontInfo.h"

// Damage pop-up type — determines color and scale
enum class EDamagePopType : uint8
{
	NormalDamage,    // Yellow/gold — auto-attack damage dealt to enemies
	CriticalDamage,  // White, larger — critical hit on enemy
	PlayerHit,       // Red — local player receiving damage
	PlayerCritHit,   // Bright red, larger — local player receiving a critical hit
	SkillDamage,     // Orange — skill damage dealt to enemies
	Miss,            // Light blue — "Miss" text
	Heal,            // Green — healing received
	Dodge,           // Green — FLEE dodge (target dodged via AGI)
	PerfectDodge     // Bright green — Lucky Dodge (target dodged via LUK)
};

// Single damage pop-up entry in the pool
struct FDamagePopEntry
{
	bool bActive = false;
	int32 Value = 0;
	EDamagePopType Type = EDamagePopType::NormalDamage;
	FVector2D ScreenAnchor = FVector2D::ZeroVector;  // Screen-space anchor at spawn
	double SpawnTime = 0.0;
	float RandomXBias = 0.0f;  // Slight random horizontal offset for organic feel
	FString Element;           // Attack element for coloring (e.g., "fire", "water")
};

class SDamageNumberOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDamageNumberOverlay) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Add a new damage pop-up at the given screen position. */
	void AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition, const FString& Element = TEXT(""));

	// SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	// Active timer — drives continuous repainting while entries are alive
	EActiveTimerReturnType OnAnimationTick(double InCurrentTime, float InDeltaTime);

	// Entry pool (circular buffer)
	static constexpr int32 MAX_ENTRIES = 64;
	FDamagePopEntry Entries[MAX_ENTRIES];
	int32 NextEntryIndex = 0;
	int32 ActiveCount = 0;  // Rough count of active entries (for timer optimization)

	// ---- Animation Constants (RO-faithful) ----
	static constexpr float LIFETIME = 1.3f;              // Total pop-up duration (seconds)
	static constexpr float RISE_DISTANCE = 85.0f;        // Pixels to rise over lifetime
	static constexpr float SPREAD_PER_DIGIT = 4.5f;      // Max extra pixels of spread per digit
	static constexpr float FADE_START = 0.65f;            // Start fading at 65% of lifetime
	static constexpr float DIGIT_BASE_GAP = 1.0f;        // Base gap between digits (pixels)
	static constexpr float RANDOM_X_RANGE = 12.0f;       // Random horizontal bias range (+/-)
	static constexpr float STACK_CHECK_RADIUS = 60.0f;    // Pixel radius for stacking detection
	static constexpr float STACK_CHECK_TIME = 0.8f;       // Time window for stacking (seconds)
	static constexpr float STACK_OFFSET_Y = -26.0f;       // Vertical offset per stacked number

	// ---- Font sizes ----
	static constexpr int32 NORMAL_FONT_SIZE = 20;
	static constexpr int32 CRIT_FONT_SIZE = 28;
	static constexpr int32 SKILL_FONT_SIZE = 22;
	static constexpr int32 MISS_FONT_SIZE = 18;
	static constexpr int32 HEAL_FONT_SIZE = 19;
	static constexpr int32 DODGE_FONT_SIZE = 18;
	static constexpr float OUTLINE_SIZE_NORMAL = 2.0f;
	static constexpr float OUTLINE_SIZE_CRIT = 3.0f;

	// ---- Helpers ----
	static FLinearColor GetFillColor(EDamagePopType Type);
	static FLinearColor GetOutlineColor(EDamagePopType Type);
	static int32 GetFontSize(EDamagePopType Type);
	static float GetOutlineSize(EDamagePopType Type);

	/** Get tint color based on attack element (for elemental damage coloring). */
	static FLinearColor GetElementTint(const FString& Element);

	// Cached font measure service (avoid repeated lookups)
	mutable bool bFontCacheValid = false;
	mutable float CachedDigitWidth = 0.0f;
	mutable float CachedDigitHeight = 0.0f;
	void EnsureFontMeasured() const;
};
