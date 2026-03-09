// SDamageNumberOverlay.cpp — RO-style damage number rendering overlay
// Renders individual digits with fan-out spread, rise animation, fade, and outline.

#include "SDamageNumberOverlay.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SNullWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamageOverlay, Log, All);

// ============================================================
// RO-style Damage Number Color Palette
// ============================================================
namespace RODamageColors
{
	// Fill colors — bright, saturated, RO-faithful
	static const FLinearColor NormalYellow  (1.00f, 0.92f, 0.23f, 1.0f);  // Yellow/gold auto-attack
	static const FLinearColor CritWhite    (1.00f, 1.00f, 0.95f, 1.0f);  // Warm white for crits
	static const FLinearColor PlayerRed    (1.00f, 0.20f, 0.20f, 1.0f);  // Red for player-received
	static const FLinearColor PlayerCritRed(1.00f, 0.35f, 0.55f, 1.0f);  // Magenta-red for player crit
	static const FLinearColor SkillOrange  (1.00f, 0.65f, 0.15f, 1.0f);  // Orange for skill damage
	static const FLinearColor MissBlue     (0.50f, 0.70f, 1.00f, 1.0f);  // Light blue for miss
	static const FLinearColor HealGreen    (0.30f, 1.00f, 0.40f, 1.0f);  // Bright green for heals
	static const FLinearColor DodgeGreen   (0.40f, 0.90f, 0.50f, 1.0f);  // Green for FLEE dodge
	static const FLinearColor PerfDodgeGold(0.95f, 0.85f, 0.20f, 1.0f);  // Gold for Lucky Dodge

	// Element tint colors — used when attack has non-neutral element
	static const FLinearColor EleWater     (0.40f, 0.70f, 1.00f, 1.0f);  // Blue
	static const FLinearColor EleEarth     (0.70f, 0.55f, 0.30f, 1.0f);  // Brown
	static const FLinearColor EleFire      (1.00f, 0.40f, 0.15f, 1.0f);  // Orange-red
	static const FLinearColor EleWind      (0.60f, 1.00f, 0.60f, 1.0f);  // Light green
	static const FLinearColor ElePoison    (0.70f, 0.30f, 0.80f, 1.0f);  // Purple
	static const FLinearColor EleHoly      (1.00f, 1.00f, 0.80f, 1.0f);  // Bright white-gold
	static const FLinearColor EleShadow    (0.50f, 0.30f, 0.60f, 1.0f);  // Dark purple
	static const FLinearColor EleGhost     (0.80f, 0.80f, 0.95f, 1.0f);  // Pale lavender
	static const FLinearColor EleUndead    (0.50f, 0.50f, 0.50f, 1.0f);  // Grey

	// Outline color — near-black for contrast
	static const FLinearColor OutlineDark  (0.02f, 0.02f, 0.02f, 1.0f);
}

// ============================================================
// Construction
// ============================================================

void SDamageNumberOverlay::Construct(const FArguments& InArgs)
{
	SetVisibility(EVisibility::SelfHitTestInvisible);

	// SCompoundWidget requires a ChildSlot — use an invisible null widget
	ChildSlot
	[
		SNullWidget::NullWidget
	];

	// Register an active timer that ticks every frame for animation
	RegisterActiveTimer(0.0f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SDamageNumberOverlay::OnAnimationTick));

	UE_LOG(LogDamageOverlay, Log, TEXT("SDamageNumberOverlay constructed — active timer registered."));
}

// ============================================================
// Add a new damage pop-up
// ============================================================

void SDamageNumberOverlay::AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition, const FString& Element)
{
	const double Now = FPlatformTime::Seconds();

	// Stacking: count recent entries near this position and offset upward
	int32 StackCount = 0;
	for (int32 i = 0; i < MAX_ENTRIES; ++i)
	{
		const FDamagePopEntry& E = Entries[i];
		if (!E.bActive) continue;
		if ((Now - E.SpawnTime) > STACK_CHECK_TIME) continue;
		if (FVector2D::Distance(E.ScreenAnchor, ScreenPosition) < STACK_CHECK_RADIUS)
		{
			++StackCount;
		}
	}

	// Apply stack offset (move upward) and random horizontal bias
	FVector2D AdjustedPos = ScreenPosition;
	AdjustedPos.Y += StackCount * STACK_OFFSET_Y;  // Negative constant = upward in screen space

	FDamagePopEntry& Entry = Entries[NextEntryIndex];
	Entry.bActive = true;
	Entry.Value = Value;
	Entry.Type = Type;
	Entry.ScreenAnchor = AdjustedPos;
	Entry.SpawnTime = Now;
	Entry.RandomXBias = FMath::FRandRange(-RANDOM_X_RANGE, RANDOM_X_RANGE);
	Entry.Element = Element;

	NextEntryIndex = (NextEntryIndex + 1) % MAX_ENTRIES;
	++ActiveCount;

	UE_LOG(LogDamageOverlay, Log, TEXT("AddDamagePop: %d dmg, type=%d, ele=%s, screen=(%.0f, %.0f), stack=%d, active=%d"),
		Value, (int32)Type, *Element, AdjustedPos.X, AdjustedPos.Y, StackCount, ActiveCount);
}

// ============================================================
// Active timer — invalidate for repaint each frame
// ============================================================

EActiveTimerReturnType SDamageNumberOverlay::OnAnimationTick(double InCurrentTime, float InDeltaTime)
{
	const double Now = FPlatformTime::Seconds();

	// Clean up expired entries
	int32 Count = 0;
	for (int32 i = 0; i < MAX_ENTRIES; ++i)
	{
		FDamagePopEntry& E = Entries[i];
		if (!E.bActive) continue;
		if ((Now - E.SpawnTime) >= LIFETIME)
		{
			E.bActive = false;
		}
		else
		{
			++Count;
		}
	}
	ActiveCount = Count;

	// Always invalidate to ensure paint runs (even idle, this is cheap)
	Invalidate(EInvalidateWidgetReason::Paint);

	return EActiveTimerReturnType::Continue;
}

// ============================================================
// Font measurement cache
// ============================================================

void SDamageNumberOverlay::EnsureFontMeasured() const
{
	if (bFontCacheValid) return;

	if (!FSlateApplication::IsInitialized()) return;
	TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	FSlateFontInfo MeasureFont = FCoreStyle::GetDefaultFontStyle("Bold", NORMAL_FONT_SIZE);
	FVector2D Size = FontMeasure->Measure(TEXT("0"), MeasureFont);
	CachedDigitWidth = (float)Size.X;
	CachedDigitHeight = (float)Size.Y;
	bFontCacheValid = true;
}

// ============================================================
// Color/size helpers
// ============================================================

FLinearColor SDamageNumberOverlay::GetFillColor(EDamagePopType Type)
{
	switch (Type)
	{
	case EDamagePopType::NormalDamage:   return RODamageColors::NormalYellow;
	case EDamagePopType::CriticalDamage: return RODamageColors::CritWhite;
	case EDamagePopType::PlayerHit:      return RODamageColors::PlayerRed;
	case EDamagePopType::PlayerCritHit:  return RODamageColors::PlayerCritRed;
	case EDamagePopType::SkillDamage:    return RODamageColors::SkillOrange;
	case EDamagePopType::Miss:           return RODamageColors::MissBlue;
	case EDamagePopType::Heal:           return RODamageColors::HealGreen;
	case EDamagePopType::Dodge:          return RODamageColors::DodgeGreen;
	case EDamagePopType::PerfectDodge:   return RODamageColors::PerfDodgeGold;
	default:                             return RODamageColors::NormalYellow;
	}
}

FLinearColor SDamageNumberOverlay::GetOutlineColor(EDamagePopType Type)
{
	return RODamageColors::OutlineDark;
}

int32 SDamageNumberOverlay::GetFontSize(EDamagePopType Type)
{
	switch (Type)
	{
	case EDamagePopType::CriticalDamage: return CRIT_FONT_SIZE;
	case EDamagePopType::PlayerCritHit:  return CRIT_FONT_SIZE;
	case EDamagePopType::SkillDamage:    return SKILL_FONT_SIZE;
	case EDamagePopType::Miss:           return MISS_FONT_SIZE;
	case EDamagePopType::Heal:           return HEAL_FONT_SIZE;
	case EDamagePopType::Dodge:          return DODGE_FONT_SIZE;
	case EDamagePopType::PerfectDodge:   return DODGE_FONT_SIZE;
	default:                             return NORMAL_FONT_SIZE;
	}
}

float SDamageNumberOverlay::GetOutlineSize(EDamagePopType Type)
{
	switch (Type)
	{
	case EDamagePopType::CriticalDamage: return OUTLINE_SIZE_CRIT;
	case EDamagePopType::PlayerCritHit:  return OUTLINE_SIZE_CRIT;
	default:                             return OUTLINE_SIZE_NORMAL;
	}
}

FLinearColor SDamageNumberOverlay::GetElementTint(const FString& Element)
{
	if (Element == TEXT("water"))  return RODamageColors::EleWater;
	if (Element == TEXT("earth"))  return RODamageColors::EleEarth;
	if (Element == TEXT("fire"))   return RODamageColors::EleFire;
	if (Element == TEXT("wind"))   return RODamageColors::EleWind;
	if (Element == TEXT("poison")) return RODamageColors::ElePoison;
	if (Element == TEXT("holy"))   return RODamageColors::EleHoly;
	if (Element == TEXT("shadow")) return RODamageColors::EleShadow;
	if (Element == TEXT("ghost"))  return RODamageColors::EleGhost;
	if (Element == TEXT("undead")) return RODamageColors::EleUndead;
	return FLinearColor::White; // Neutral — no tint
}

// ============================================================
// OnPaint — the core rendering loop
// Draws all active damage numbers with RO-style per-digit
// fan-out, rise animation, alpha fade, and outlined text.
// ============================================================

int32 SDamageNumberOverlay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Paint children first (the null widget — does nothing, but required for SCompoundWidget)
	int32 OutLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (ActiveCount <= 0) return OutLayerId;

	EnsureFontMeasured();

	const double Now = FPlatformTime::Seconds();
	const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

	// Get the DPI scale to convert screen-pixel positions to Slate local coordinates
	const float GeometryScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const float InvScale = (GeometryScale > 0.0f) ? (1.0f / GeometryScale) : 1.0f;

	// Font measure service for per-size digit width
	TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// Use a higher layer for damage text so it renders on top
	const int32 TextLayerId = OutLayerId + 1;

	for (int32 i = 0; i < MAX_ENTRIES; ++i)
	{
		const FDamagePopEntry& Entry = Entries[i];
		if (!Entry.bActive) continue;

		const float Elapsed = (float)(Now - Entry.SpawnTime);
		const float LifeAlpha = FMath::Clamp(Elapsed / LIFETIME, 0.0f, 1.0f);
		if (LifeAlpha >= 1.0f) continue;

		// ---- Animation curves ----

		// Rise: ease-out deceleration (fast at start, slows down)
		const float RiseProgress = FMath::InterpEaseOut(0.0f, 1.0f, LifeAlpha, 3.0f);
		const float RiseY = -RISE_DISTANCE * RiseProgress;

		// Digit spread: ease-out, reaches max at ~50% lifetime
		const float SpreadT = FMath::Clamp(LifeAlpha * 2.0f, 0.0f, 1.0f);
		const float SpreadFactor = FMath::InterpEaseOut(0.0f, 1.0f, SpreadT, 2.0f);

		// Alpha fade: full opacity until FADE_START, then linear fade to 0
		float Alpha = 1.0f;
		if (LifeAlpha > FADE_START)
		{
			Alpha = 1.0f - (LifeAlpha - FADE_START) / (1.0f - FADE_START);
		}
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		// ---- Font setup ----
		const int32 FontSize = GetFontSize(Entry.Type);
		const float OutlineSz = GetOutlineSize(Entry.Type);

		FSlateFontInfo DigitFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
		DigitFont.OutlineSettings.OutlineSize = (int32)OutlineSz;
		DigitFont.OutlineSettings.OutlineColor = GetOutlineColor(Entry.Type);

		// Measure a single digit in this font size
		const FVector2D DigitMeasure = FontMeasure->Measure(TEXT("0"), DigitFont);
		const float DigitW = (float)DigitMeasure.X;
		const float DigitH = (float)DigitMeasure.Y;

		// ---- Build display text ----
		FString DisplayText;
		bool bIsTextLabel = false; // True for "Miss", "Dodge", etc. — render as single block
		if (Entry.Type == EDamagePopType::Miss)
		{
			DisplayText = TEXT("Miss");
			bIsTextLabel = true;
		}
		else if (Entry.Type == EDamagePopType::Dodge)
		{
			DisplayText = TEXT("Dodge");
			bIsTextLabel = true;
		}
		else if (Entry.Type == EDamagePopType::PerfectDodge)
		{
			DisplayText = TEXT("Lucky Dodge");
			bIsTextLabel = true;
		}
		else
		{
			DisplayText = FString::FromInt(FMath::Abs(Entry.Value));
		}
		const int32 NumChars = DisplayText.Len();

		// ---- Tint color with alpha ----
		FLinearColor TintColor = GetFillColor(Entry.Type);

		// Apply element tinting for non-neutral elemental attacks (blend toward element color)
		if (!Entry.Element.IsEmpty() && Entry.Element != TEXT("neutral") && !bIsTextLabel && Entry.Type != EDamagePopType::Heal)
		{
			const FLinearColor EleTint = GetElementTint(Entry.Element);
			if (EleTint != FLinearColor::White)
			{
				// Blend: 60% base type color + 40% element color for subtle but visible tint
				TintColor = FLinearColor::LerpUsingHSV(TintColor, EleTint, 0.4f);
			}
		}
		TintColor.A = Alpha;

		// ---- Base screen position (with rise and random bias) ----
		// Convert from screen-pixel coordinates to Slate local coordinates (DPI-adjusted)
		const FVector2D BasePos(
			(Entry.ScreenAnchor.X + Entry.RandomXBias) * InvScale,
			(Entry.ScreenAnchor.Y + RiseY) * InvScale
		);

		// ---- Render each character individually with fan-out spread ----
		if (bIsTextLabel)
		{
			// "Miss"/"Dodge"/"Lucky Dodge" rendered as a single text block (no digit spread)
			const FVector2D TextMeasure = FontMeasure->Measure(DisplayText, DigitFont);
			const float TextW = (float)TextMeasure.X;

			const FVector2f DrawPos(
				(float)(BasePos.X - TextW * 0.5f),
				(float)(BasePos.Y - DigitH * 0.5f)
			);

			FSlateDrawElement::MakeText(
				OutDrawElements,
				TextLayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TextW, DigitH),
					FSlateLayoutTransform(DrawPos)
				),
				DisplayText,
				DigitFont,
				DrawEffects,
				TintColor
			);
		}
		else
		{
			// Per-digit rendering with horizontal fan-out
			const float SpreadExtra = SpreadFactor * SPREAD_PER_DIGIT;
			const float EffectiveDigitSpacing = DigitW + DIGIT_BASE_GAP + SpreadExtra;

			for (int32 c = 0; c < NumChars; ++c)
			{
				// Center the digit group around BasePos.X
				const float DigitCenterOffset = (c - (NumChars - 1) * 0.5f) * EffectiveDigitSpacing;

				const FVector2f DrawPos(
					(float)(BasePos.X + DigitCenterOffset - DigitW * 0.5f),
					(float)(BasePos.Y - DigitH * 0.5f)
				);

				const FString SingleChar = DisplayText.Mid(c, 1);

				FSlateDrawElement::MakeText(
					OutDrawElements,
					TextLayerId,
					AllottedGeometry.ToPaintGeometry(
						FVector2D(DigitW, DigitH),
						FSlateLayoutTransform(DrawPos)
					),
					SingleChar,
					DigitFont,
					DrawEffects,
					TintColor
				);
			}
		}
	}

	return TextLayerId;
}
