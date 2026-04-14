// SDamageNumberOverlay.cpp — RO Classic damage number rendering overlay
// Renders per-digit numbers with parabolic sine arc, scale shrink, diagonal drift,
// and immediate linear alpha fade — faithful to roBrowser's Damage.js implementation.

#include "SDamageNumberOverlay.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

float SDamageNumberOverlay::FontScaleMultiplier = 1.0f;
#include "Widgets/SNullWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamageOverlay, Log, All);

// ============================================================
// RO-style Damage Number Color Palette
// ============================================================
namespace RODamageColors
{
	// Fill colors — bright, saturated, RO-faithful
	static const FLinearColor NormalWhite   (1.00f, 1.00f, 1.00f, 1.0f);  // White — normal damage (RO Classic)
	static const FLinearColor CritYellow    (0.90f, 0.90f, 0.15f, 1.0f);  // Yellow — critical hits (RO Classic)
	static const FLinearColor PlayerRed    (1.00f, 0.00f, 0.00f, 1.0f);  // Pure red — player-received
	static const FLinearColor PlayerCritRed(1.00f, 0.35f, 0.55f, 1.0f);  // Magenta-red for player crit
	static const FLinearColor SkillOrange  (1.00f, 0.65f, 0.15f, 1.0f);  // Orange for skill damage
	static const FLinearColor MissBlue     (0.50f, 0.70f, 1.00f, 1.0f);  // Light blue for miss
	static const FLinearColor HealGreen    (0.00f, 1.00f, 0.00f, 1.0f);  // Pure green — heals (RO Classic)
	static const FLinearColor DodgeGreen   (0.40f, 0.90f, 0.50f, 1.0f);  // Green for FLEE dodge
	static const FLinearColor PerfDodgeGold(0.95f, 0.85f, 0.20f, 1.0f);  // Gold for Lucky Dodge
	static const FLinearColor BlockSilver  (0.85f, 0.90f, 1.00f, 1.0f);  // Silver/white for shield block

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

	// Init critical starburst brush
	if (InArgs._CritStarburstTexture)
	{
		CritStarburstBrush.SetResourceObject(InArgs._CritStarburstTexture);
		CritStarburstBrush.ImageSize = FVector2D(128, 128);
		CritStarburstBrush.DrawAs = ESlateBrushDrawType::Image;
	}

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

void SDamageNumberOverlay::AddDamagePop(int32 Value, EDamagePopType Type, FVector2D ScreenPosition, const FString& Element, const FLinearColor* CustomColor)
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
	Entry.TextLabel.Empty();
	if (CustomColor)
	{
		Entry.CustomColor = *CustomColor;
		Entry.bHasCustomColor = true;
	}
	else
	{
		Entry.bHasCustomColor = false;
	}

	// RO Classic: per-type lifetime and drift direction
	if (Type == EDamagePopType::Miss || Type == EDamagePopType::Dodge ||
		Type == EDamagePopType::PerfectDodge || Type == EDamagePopType::Block)
	{
		Entry.Lifetime = LIFETIME_MISS;
		Entry.DriftDirection = 0.0f;
	}
	else if (Type == EDamagePopType::Heal)
	{
		Entry.Lifetime = LIFETIME_HEAL;
		Entry.DriftDirection = 0.0f;
	}
	else if (Type == EDamagePopType::ComboTotal)
	{
		Entry.Lifetime = LIFETIME_COMBO;
		Entry.DriftDirection = 0.0f;
	}
	else
	{
		Entry.Lifetime = LIFETIME_DAMAGE;
		// Alternate left/right drift with slight random magnitude variation
		Entry.DriftDirection = ((NextEntryIndex % 2 == 0) ? 1.0f : -1.0f) * FMath::FRandRange(0.7f, 1.0f);
	}

	NextEntryIndex = (NextEntryIndex + 1) % MAX_ENTRIES;
	++ActiveCount;

	UE_LOG(LogDamageOverlay, Log, TEXT("AddDamagePop: %d dmg, type=%d, ele=%s, screen=(%.0f, %.0f), stack=%d, active=%d"),
		Value, (int32)Type, *Element, AdjustedPos.X, AdjustedPos.Y, StackCount, ActiveCount);
}

void SDamageNumberOverlay::AddTextPop(const FString& Text, const FLinearColor& Color, FVector2D ScreenPosition)
{
	const double Now = FPlatformTime::Seconds();

	// Stacking (same logic as AddDamagePop)
	int32 StackCount = 0;
	for (int32 i = 0; i < MAX_ENTRIES; ++i)
	{
		const FDamagePopEntry& E = Entries[i];
		if (!E.bActive) continue;
		if ((Now - E.SpawnTime) > STACK_CHECK_TIME) continue;
		if (FVector2D::Distance(E.ScreenAnchor, ScreenPosition) < STACK_CHECK_RADIUS)
			++StackCount;
	}

	FVector2D AdjustedPos = ScreenPosition;
	AdjustedPos.Y += StackCount * STACK_OFFSET_Y;

	FDamagePopEntry& Entry = Entries[NextEntryIndex];
	Entry.bActive = true;
	Entry.Value = 0;
	Entry.Type = EDamagePopType::Miss; // Reuse Miss type for font sizing fallback
	Entry.ScreenAnchor = AdjustedPos;
	Entry.SpawnTime = Now;
	Entry.RandomXBias = FMath::FRandRange(-RANDOM_X_RANGE, RANDOM_X_RANGE);
	Entry.Element = TEXT("");
	Entry.TextLabel = Text;
	Entry.CustomColor = Color;
	Entry.bHasCustomColor = true;
	Entry.Lifetime = LIFETIME_MISS;
	Entry.DriftDirection = 0.0f;

	NextEntryIndex = (NextEntryIndex + 1) % MAX_ENTRIES;
	++ActiveCount;
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
		if ((Now - E.SpawnTime) >= E.Lifetime)
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
	case EDamagePopType::NormalDamage:   return RODamageColors::NormalWhite;
	case EDamagePopType::CriticalDamage: return RODamageColors::CritYellow;
	case EDamagePopType::PlayerHit:      return RODamageColors::PlayerRed;
	case EDamagePopType::PlayerCritHit:  return RODamageColors::PlayerCritRed;
	case EDamagePopType::SkillDamage:    return RODamageColors::SkillOrange;
	case EDamagePopType::Miss:           return RODamageColors::MissBlue;
	case EDamagePopType::Heal:           return RODamageColors::HealGreen;
	case EDamagePopType::Dodge:          return RODamageColors::DodgeGreen;
	case EDamagePopType::PerfectDodge:   return RODamageColors::PerfDodgeGold;
	case EDamagePopType::Block:          return RODamageColors::BlockSilver;
	case EDamagePopType::ComboTotal:     return RODamageColors::CritYellow;
	default:                             return RODamageColors::NormalWhite;
	}
}

FLinearColor SDamageNumberOverlay::GetOutlineColor(EDamagePopType Type)
{
	return RODamageColors::OutlineDark;
}

int32 SDamageNumberOverlay::GetFontSize(EDamagePopType Type)
{
	int32 Base;
	switch (Type)
	{
	case EDamagePopType::CriticalDamage: Base = CRIT_FONT_SIZE; break;
	case EDamagePopType::PlayerCritHit:  Base = CRIT_FONT_SIZE; break;
	case EDamagePopType::SkillDamage:    Base = SKILL_FONT_SIZE; break;
	case EDamagePopType::Miss:           Base = MISS_FONT_SIZE; break;
	case EDamagePopType::Heal:           Base = HEAL_FONT_SIZE; break;
	case EDamagePopType::Dodge:          Base = DODGE_FONT_SIZE; break;
	case EDamagePopType::PerfectDodge:   Base = DODGE_FONT_SIZE; break;
	case EDamagePopType::Block:          Base = DODGE_FONT_SIZE; break;
	case EDamagePopType::ComboTotal:     Base = CRIT_FONT_SIZE; break;
	default:                             Base = NORMAL_FONT_SIZE; break;
	}
	return FMath::RoundToInt((float)Base * FontScaleMultiplier);
}

float SDamageNumberOverlay::GetOutlineSize(EDamagePopType Type)
{
	switch (Type)
	{
	case EDamagePopType::CriticalDamage: return OUTLINE_SIZE_CRIT;
	case EDamagePopType::PlayerCritHit:  return OUTLINE_SIZE_CRIT;
	case EDamagePopType::ComboTotal:     return OUTLINE_SIZE_CRIT;
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
// Draws all active damage numbers with RO Classic parabolic sine arc,
// scale shrink, diagonal drift, and per-type animation curves.
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
		const float t = FMath::Clamp(Elapsed / Entry.Lifetime, 0.0f, 1.0f);
		if (t >= 1.0f) continue;

		// ---- Determine animation category ----
		const bool bIsHeal = (Entry.Type == EDamagePopType::Heal);
		const bool bIsMissType = (Entry.Type == EDamagePopType::Miss ||
		                          Entry.Type == EDamagePopType::Dodge ||
		                          Entry.Type == EDamagePopType::PerfectDodge ||
		                          Entry.Type == EDamagePopType::Block ||
		                          !Entry.TextLabel.IsEmpty());

		// ---- Per-type animation curves (RO Classic) ----
		float OffsetX = 0.0f;
		float OffsetY = 0.0f;
		float Scale = 1.0f;
		float Alpha = FMath::Max(0.0f, 1.0f - t);  // Immediate linear fade

		if (Entry.Type == EDamagePopType::ComboTotal)
		{
			// Combo total: rapid pop-in (0→3.5x in ~50ms), then hold. High position, slow rise.
			const float PopT = FMath::Clamp(t / 0.017f, 0.0f, 1.0f);
			Scale = PopT * 3.5f;
			OffsetY = -(50.0f + 20.0f * t);
			OffsetX = 0.0f;
			Alpha = FMath::Max(0.0f, 1.0f - t * 0.8f);
		}
		else if (bIsHeal)
		{
			// Heal: stationary for 40%, then rise. Quick shrink to floor then hold.
			if (t < HEAL_STATIONARY_PCT)
			{
				OffsetY = -ARC_BASE_OFFSET;
			}
			else
			{
				const float RiseT = (t - HEAL_STATIONARY_PCT) / (1.0f - HEAL_STATIONARY_PCT);
				OffsetY = -(ARC_BASE_OFFSET + HEAL_RISE_SPEED * Entry.Lifetime * RiseT);
			}
			Scale = FMath::Max(HEAL_SCALE_FLOOR, (1.0f - t * 2.0f) * HEAL_SCALE_START);
		}
		else if (bIsMissType)
		{
			// Miss/Dodge/Block/Status text: straight rise, fixed scale, no drift
			OffsetY = -(MISS_BASE_OFFSET + MISS_RISE_DISTANCE * t);
			Scale = MISS_SCALE;
		}
		else
		{
			// Damage: parabolic sine arc + horizontal drift + linear shrink (RO Classic)
			OffsetX = Entry.DriftDirection * DRIFT_DISTANCE * t;
			const float SineInput = -PI / 2.0f + PI * (0.5f + t * 1.5f);
			OffsetY = -(ARC_BASE_OFFSET + FMath::Sin(SineInput) * ARC_AMPLITUDE);
			Scale = FMath::Max(0.01f, (1.0f - t) * SCALE_START);
		}

		// ---- Font setup with dynamic scale ----
		const int32 BaseFontSize = Entry.bHasCustomColor
			? FMath::RoundToInt((float)STATUS_TEXT_FONT_SIZE * FontScaleMultiplier)
			: GetFontSize(Entry.Type);
		const int32 ScaledFontSz = FMath::Max(6, FMath::RoundToInt((float)BaseFontSize * Scale));
		const float OutlineSz = GetOutlineSize(Entry.Type);

		FSlateFontInfo DigitFont = FCoreStyle::GetDefaultFontStyle("Bold", ScaledFontSz);
		DigitFont.OutlineSettings.OutlineSize = FMath::Max(1, (int32)OutlineSz);
		DigitFont.OutlineSettings.OutlineColor = GetOutlineColor(Entry.Type);

		const FVector2D DigitMeasure = FontMeasure->Measure(TEXT("0"), DigitFont);
		const float DigitW = (float)DigitMeasure.X;
		const float DigitH = (float)DigitMeasure.Y;

		// ---- Build display text ----
		FString DisplayText;
		bool bIsTextLabel = false;
		if (!Entry.TextLabel.IsEmpty())
		{
			DisplayText = Entry.TextLabel;
			bIsTextLabel = true;
		}
		else if (Entry.Type == EDamagePopType::Miss)
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
		else if (Entry.Type == EDamagePopType::Block)
		{
			DisplayText = TEXT("Block");
			bIsTextLabel = true;
		}
		else
		{
			DisplayText = FString::FromInt(FMath::Abs(Entry.Value));
		}
		const int32 NumChars = DisplayText.Len();

		// ---- Tint color with alpha ----
		FLinearColor TintColor = Entry.bHasCustomColor ? Entry.CustomColor : GetFillColor(Entry.Type);

		if (!Entry.Element.IsEmpty() && Entry.Element != TEXT("neutral") && !bIsTextLabel && Entry.Type != EDamagePopType::Heal)
		{
			const FLinearColor EleTint = GetElementTint(Entry.Element);
			if (EleTint != FLinearColor::White)
			{
				TintColor = FLinearColor::LerpUsingHSV(TintColor, EleTint, 0.4f);
			}
		}
		TintColor.A = Alpha;

		// ---- Screen position with animation offsets ----
		const FVector2D BasePos(
			(Entry.ScreenAnchor.X + Entry.RandomXBias + OffsetX) * InvScale,
			(Entry.ScreenAnchor.Y + OffsetY) * InvScale
		);

		// ---- Critical starburst background (render BEFORE digits, centered on number) ----
		const bool bIsCrit = (Entry.Type == EDamagePopType::CriticalDamage ||
		                      Entry.Type == EDamagePopType::PlayerCritHit);
		if (bIsCrit && CritStarburstBrush.GetResourceObject())
		{
			// Size proportional to digit group — large enough to frame the number
			const float BurstSize = DigitH * 4.0f;
			// Center exactly on BasePos (which is the center of the digit group)
			const FVector2f BurstPos(
				(float)(BasePos.X - BurstSize * 0.5f),
				(float)(BasePos.Y - BurstSize * 0.5f));
			// Full color — texture is already the desired red/orange
			const FLinearColor BurstColor(1.0f, 1.0f, 1.0f, Alpha);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				OutLayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(BurstSize, BurstSize),
					FSlateLayoutTransform(BurstPos)),
				&CritStarburstBrush,
				DrawEffects,
				BurstColor);
		}

		// ---- Render ----
		if (bIsTextLabel)
		{
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
			// Per-digit rendering (scaling handles visual interest, no time-based spread)
			const float EffectiveDigitSpacing = DigitW + DIGIT_BASE_GAP;

			for (int32 c = 0; c < NumChars; ++c)
			{
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
