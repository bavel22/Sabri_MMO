// SDamageNumberOverlay.h — Fullscreen transparent Slate overlay that renders
// RO Classic damage numbers via OnPaint with parabolic sine arc, scale shrink,
// diagonal drift, and per-type animation curves. Faithful to roBrowser Damage.js.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Fonts/SlateFontInfo.h"
#include "Engine/Texture2D.h"

// Damage pop-up type — determines color and scale
enum class EDamagePopType : uint8
{
	NormalDamage,    // White — auto-attack damage dealt to enemies (RO Classic)
	CriticalDamage,  // Yellow — critical hit on enemy (RO Classic)
	PlayerHit,       // Red — local player receiving damage
	PlayerCritHit,   // Bright red, larger — local player receiving a critical hit
	SkillDamage,     // Orange — skill damage dealt to enemies
	Miss,            // Light blue — "Miss" text
	Heal,            // Green — healing received
	Dodge,           // Green — FLEE dodge (target dodged via AGI)
	PerfectDodge,    // Bright green — Lucky Dodge (target dodged via LUK)
	Block,           // Silver/white — Auto Guard shield block
	ComboTotal       // Yellow — multi-hit skill total (3s duration, rapid pop-in)
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
	FString TextLabel;         // If non-empty, renders this text instead of Value digits
	FLinearColor CustomColor = FLinearColor::White;  // Used when TextLabel is set
	bool bHasCustomColor = false;
	float Lifetime = 1.5f;        // Per-entry duration (type-dependent)
	float DriftDirection = 1.0f;  // Horizontal drift sign (+1 right, -1 left, 0 none)
};

class SDamageNumberOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDamageNumberOverlay)
		: _CritStarburstTexture(nullptr)
	{}
		SLATE_ARGUMENT(UTexture2D*, CritStarburstTexture)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Add a new damage pop-up at the given screen position. */
	void AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition, const FString& Element = TEXT(""), const FLinearColor* CustomColor = nullptr);

	/** Add a floating text label (e.g. "Poisoned!", "Stunned!") at the given screen position. */
	void AddTextPop(const FString& Text, const FLinearColor& Color, FVector2D ScreenPosition);

	/** Options: font scale multiplier (set by OptionsSubsystem). */
	static float FontScaleMultiplier;

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

	// ---- Animation Constants (RO Classic faithful) ----
	// Per-type durations
	static constexpr float LIFETIME_DAMAGE = 1.5f;        // Normal/crit/skill damage
	static constexpr float LIFETIME_MISS = 0.8f;          // Miss/dodge/block text
	static constexpr float LIFETIME_HEAL = 1.5f;          // Heal numbers
	static constexpr float LIFETIME_COMBO = 3.0f;         // Combo total (RO: 3s)

	// Damage arc parameters (screen pixels)
	static constexpr float ARC_AMPLITUDE = 120.0f;        // Height of sine arc peak
	static constexpr float ARC_BASE_OFFSET = 30.0f;       // Starting offset above spawn
	static constexpr float DRIFT_DISTANCE = 60.0f;        // Horizontal drift over lifetime
	static constexpr float SCALE_START = 2.5f;            // Initial scale multiplier (shrinks to 0)

	// Miss/dodge/block parameters
	static constexpr float MISS_SCALE = 0.7f;             // Fixed scale for miss text
	static constexpr float MISS_BASE_OFFSET = 20.0f;      // Starting offset above spawn
	static constexpr float MISS_RISE_DISTANCE = 100.0f;   // Vertical rise distance

	// Heal parameters
	static constexpr float HEAL_STATIONARY_PCT = 0.4f;    // Hold still for first 40%
	static constexpr float HEAL_SCALE_START = 2.5f;       // Initial heal scale
	static constexpr float HEAL_SCALE_FLOOR = 0.8f;       // Minimum heal scale
	static constexpr float HEAL_RISE_SPEED = 80.0f;       // Rise speed (pixels/sec) after hold

	// Shared constants
	static constexpr float DIGIT_BASE_GAP = 1.0f;         // Gap between digits (pixels)
	static constexpr float RANDOM_X_RANGE = 12.0f;        // Random horizontal bias range (+/-)
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
	static constexpr int32 STATUS_TEXT_FONT_SIZE = 17;
	static constexpr float OUTLINE_SIZE_NORMAL = 2.0f;
	static constexpr float OUTLINE_SIZE_CRIT = 3.0f;

	// ---- Helpers ----
	static FLinearColor GetFillColor(EDamagePopType Type);
	static FLinearColor GetOutlineColor(EDamagePopType Type);
	static int32 GetFontSize(EDamagePopType Type);
	static float GetOutlineSize(EDamagePopType Type);

	/** Get tint color based on attack element (for elemental damage coloring). */
	static FLinearColor GetElementTint(const FString& Element);

	// Critical starburst brush (rendered behind crit numbers)
	FSlateBrush CritStarburstBrush;

	// Cached font measure service (avoid repeated lookups)
	mutable bool bFontCacheValid = false;
	mutable float CachedDigitWidth = 0.0f;
	mutable float CachedDigitHeight = 0.0f;
	void EnsureFontMeasured() const;
};
