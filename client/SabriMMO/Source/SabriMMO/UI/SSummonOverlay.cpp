// SSummonOverlay.cpp — Renders summoned Flora plants and Marine Spheres as
// colored ground markers with name tags, HP bars, and duration timers.
// Pure OnPaint rendering — no meshes or Blueprint actors.

#include "SSummonOverlay.h"
#include "SummonSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"
#include "Widgets/SNullWidget.h"

void SSummonOverlay::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	SetVisibility(EVisibility::HitTestInvisible);
	ChildSlot[ SNullWidget::NullWidget ];

	RegisterActiveTimer(0.0f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SSummonOverlay::OnRepaintTick));
}

EActiveTimerReturnType SSummonOverlay::OnRepaintTick(double, float)
{
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

int32 SSummonOverlay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 OutLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	USummonSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return OutLayerId;

	const float GeometryScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const float InvScale = (GeometryScale > 0.f) ? (1.f / GeometryScale) : 1.f;
	const FVector2D ViewportSize = AllottedGeometry.GetLocalSize();
	const int32 DrawLayer = OutLayerId + 1;
	const double Now = FPlatformTime::Seconds();

	// Draw plants
	for (const auto& Pair : Sub->Plants)
	{
		const FSummonedPlant& Plant = Pair.Value;
		FVector2D ScreenPos;
		if (!Sub->ProjectWorldToScreen(Plant.Position, ScreenPos)) continue;

		// Scale screen pos to local space
		ScreenPos *= InvScale;

		// Cull off-screen
		if (ScreenPos.X < -50 || ScreenPos.X > ViewportSize.X + 50 ||
			ScreenPos.Y < -50 || ScreenPos.Y > ViewportSize.Y + 50) continue;

		float HPPct = (Plant.MaxHP > 0) ? FMath::Clamp((float)Plant.HP / (float)Plant.MaxHP, 0.f, 1.f) : 1.f;
		float TimeRemaining = FMath::Max(0.f, Plant.Duration - (float)(Now - Plant.SpawnTime));
		bool bIsOwn = (Plant.OwnerId == Sub->LocalCharacterId);

		DrawPlantIndicator(OutDrawElements, DrawLayer, AllottedGeometry, InvScale,
			ScreenPos, Plant.Type, HPPct, TimeRemaining, bIsOwn);
	}

	// Draw marine spheres
	for (const auto& Pair : Sub->Spheres)
	{
		const FSummonedSphere& Sphere = Pair.Value;
		FVector2D ScreenPos;
		if (!Sub->ProjectWorldToScreen(Sphere.Position, ScreenPos)) continue;

		ScreenPos *= InvScale;

		if (ScreenPos.X < -50 || ScreenPos.X > ViewportSize.X + 50 ||
			ScreenPos.Y < -50 || ScreenPos.Y > ViewportSize.Y + 50) continue;

		float HPPct = (Sphere.MaxHP > 0) ? FMath::Clamp((float)Sphere.HP / (float)Sphere.MaxHP, 0.f, 1.f) : 1.f;
		float TimeRemaining = FMath::Max(0.f, 30.f - (float)(Now - Sphere.SpawnTime));
		bool bIsOwn = (Sphere.OwnerId == Sub->LocalCharacterId);

		DrawSphereIndicator(OutDrawElements, DrawLayer, AllottedGeometry, InvScale,
			ScreenPos, HPPct, TimeRemaining, bIsOwn);
	}

	return DrawLayer;
}

// ── Plant Indicator ──────────────────────────────────────────────

void SSummonOverlay::DrawPlantIndicator(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geo, float InvScale,
	const FVector2D& ScreenPos, const FString& Name, float HPPct, float TimeRemaining, bool bIsOwn) const
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Ground marker (colored square at feet position)
	const FLinearColor MarkerColor = bIsOwn ? SummonColors::PlantMarkerOwn : SummonColors::PlantMarker;
	const float HalfSize = MARKER_SIZE * 0.5f;
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId, Geo.ToPaintGeometry(
			FVector2D(MARKER_SIZE, MARKER_SIZE),
			FSlateLayoutTransform(FVector2f(ScreenPos.X - HalfSize, ScreenPos.Y - HalfSize))),
		WhiteBrush, ESlateDrawEffect::None, MarkerColor);

	// Name text above marker
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 8);
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D NameSize = FontMeasure->Measure(Name, Font);
	const float NameX = ScreenPos.X - NameSize.X * 0.5f;
	const float NameY = ScreenPos.Y + NAME_OFFSET_Y - NameSize.Y;

	// Shadow
	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(NameSize, FSlateLayoutTransform(FVector2f(NameX + 1.f, NameY + 1.f))),
		Name, Font, ESlateDrawEffect::None, SummonColors::NameShadow);
	// Text
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		Geo.ToPaintGeometry(NameSize, FSlateLayoutTransform(FVector2f(NameX, NameY))),
		Name, Font, ESlateDrawEffect::None, SummonColors::NameText);

	// HP bar below marker
	const float BarX = ScreenPos.X - HP_BAR_WIDTH * 0.5f;
	const float BarY = ScreenPos.Y + HP_OFFSET_Y;

	// Border
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH + 2, HP_BAR_HEIGHT + 2),
			FSlateLayoutTransform(FVector2f(BarX - 1, BarY - 1))),
		WhiteBrush, ESlateDrawEffect::None, SummonColors::HPBorder);
	// Background
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH, HP_BAR_HEIGHT),
			FSlateLayoutTransform(FVector2f(BarX, BarY))),
		WhiteBrush, ESlateDrawEffect::None, SummonColors::HPBg);
	// Fill
	if (HPPct > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH * HPPct, HP_BAR_HEIGHT),
				FSlateLayoutTransform(FVector2f(BarX, BarY))),
			WhiteBrush, ESlateDrawEffect::None, SummonColors::HPGreen);
	}

	// Duration timer text
	if (TimeRemaining > 0.f)
	{
		int32 Secs = FMath::CeilToInt(TimeRemaining);
		FString TimerStr = FString::Printf(TEXT("%ds"), Secs);
		const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 7);
		const FVector2D TimerSize = FontMeasure->Measure(TimerStr, SmallFont);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			Geo.ToPaintGeometry(TimerSize, FSlateLayoutTransform(
				FVector2f(ScreenPos.X - TimerSize.X * 0.5f, BarY + HP_BAR_HEIGHT + 2.f))),
			TimerStr, SmallFont, ESlateDrawEffect::None, SummonColors::TimerText);
	}
}

// ── Sphere Indicator ─────────────────────────────────────────────

void SSummonOverlay::DrawSphereIndicator(
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FGeometry& Geo, float InvScale,
	const FVector2D& ScreenPos, float HPPct, float TimeRemaining, bool bIsOwn) const
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Ground marker (orange/red square)
	const FLinearColor MarkerColor = bIsOwn ? SummonColors::SphereMarkerOwn : SummonColors::SphereMarker;
	const float HalfSize = MARKER_SIZE * 0.5f;
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId, Geo.ToPaintGeometry(
			FVector2D(MARKER_SIZE, MARKER_SIZE),
			FSlateLayoutTransform(FVector2f(ScreenPos.X - HalfSize, ScreenPos.Y - HalfSize))),
		WhiteBrush, ESlateDrawEffect::None, MarkerColor);

	// Name text
	const FString SphereLabel = TEXT("Marine Sphere");
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 8);
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D NameSize = FontMeasure->Measure(SphereLabel, Font);
	const float NameX = ScreenPos.X - NameSize.X * 0.5f;
	const float NameY = ScreenPos.Y + NAME_OFFSET_Y - NameSize.Y;

	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(NameSize, FSlateLayoutTransform(FVector2f(NameX + 1.f, NameY + 1.f))),
		SphereLabel, Font, ESlateDrawEffect::None, SummonColors::NameShadow);
	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
		Geo.ToPaintGeometry(NameSize, FSlateLayoutTransform(FVector2f(NameX, NameY))),
		SphereLabel, Font, ESlateDrawEffect::None, SummonColors::NameText);

	// HP bar
	const float BarX = ScreenPos.X - HP_BAR_WIDTH * 0.5f;
	const float BarY = ScreenPos.Y + HP_OFFSET_Y;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH + 2, HP_BAR_HEIGHT + 2),
			FSlateLayoutTransform(FVector2f(BarX - 1, BarY - 1))),
		WhiteBrush, ESlateDrawEffect::None, SummonColors::HPBorder);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH, HP_BAR_HEIGHT),
			FSlateLayoutTransform(FVector2f(BarX, BarY))),
		WhiteBrush, ESlateDrawEffect::None, SummonColors::HPBg);
	if (HPPct > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			Geo.ToPaintGeometry(FVector2D(HP_BAR_WIDTH * HPPct, HP_BAR_HEIGHT),
				FSlateLayoutTransform(FVector2f(BarX, BarY))),
			WhiteBrush, ESlateDrawEffect::None, SummonColors::HPGreen);
	}

	// Countdown timer
	if (TimeRemaining > 0.f)
	{
		int32 Secs = FMath::CeilToInt(TimeRemaining);
		FString TimerStr = FString::Printf(TEXT("%ds"), Secs);
		const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 7);
		const FVector2D TimerSize = FontMeasure->Measure(TimerStr, SmallFont);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			Geo.ToPaintGeometry(TimerSize, FSlateLayoutTransform(
				FVector2f(ScreenPos.X - TimerSize.X * 0.5f, BarY + HP_BAR_HEIGHT + 2.f))),
			TimerStr, SmallFont, ESlateDrawEffect::None, SummonColors::TimerText);
	}
}
