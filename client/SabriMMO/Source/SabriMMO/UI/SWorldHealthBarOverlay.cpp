// SWorldHealthBarOverlay.cpp — RO-style floating HP/SP bar rendering overlay.
// Uses OnPaint to draw bars as colored rectangles at world-projected screen positions.

#include "SWorldHealthBarOverlay.h"
#include "WorldHealthBarSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SNullWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogWorldHealthBarOverlay, Log, All);

// ============================================================
// Construction
// ============================================================

void SWorldHealthBarOverlay::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	// Full viewport, all clicks pass through
	SetVisibility(EVisibility::HitTestInvisible);

	ChildSlot
	[
		SNullWidget::NullWidget
	];

	// Register active timer for continuous repaint (bars must track moving entities)
	RegisterActiveTimer(0.0f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SWorldHealthBarOverlay::OnRepaintTick));
}

// ============================================================
// Active timer — invalidate for repaint each frame
// ============================================================

EActiveTimerReturnType SWorldHealthBarOverlay::OnRepaintTick(double InCurrentTime, float InDeltaTime)
{
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

// ============================================================
// OnPaint — core rendering loop
// Draws player HP/SP bar and all visible enemy HP bars.
// ============================================================

int32 SWorldHealthBarOverlay::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Paint children first (null widget — no-op)
	int32 OutLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	UWorldHealthBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return OutLayerId;

	// DPI scale factor: convert screen-pixel coordinates to Slate local coordinates
	const float GeometryScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const float InvScale = (GeometryScale > 0.0f) ? (1.0f / GeometryScale) : 1.0f;

	// Use a dedicated layer for bar rendering
	const int32 BarLayerId = OutLayerId + 1;

	// ---- Draw local player bar (HP + SP) ----
	if (!Sub->bPlayerDead && Sub->PlayerMaxHP > 0)
	{
		FVector PlayerFeetPos;
		if (Sub->GetPlayerFeetPosition(PlayerFeetPos))
		{
			FVector2D ScreenPos;
			if (Sub->ProjectWorldToScreen(PlayerFeetPos, ScreenPos))
			{
				const float HPPercent = FMath::Clamp(
					(float)Sub->PlayerCurrentHP / (float)Sub->PlayerMaxHP, 0.f, 1.f);
				const float SPPercent = FMath::Clamp(
					(float)Sub->PlayerCurrentSP / (float)FMath::Max(Sub->PlayerMaxSP, 1), 0.f, 1.f);
				const bool bCritical = HPPercent <= CRITICAL_THRESHOLD;

				DrawPlayerBar(OutDrawElements, BarLayerId, AllottedGeometry, InvScale,
					ScreenPos, HPPercent, SPPercent, bCritical);
			}
		}
	}

	// ---- Draw enemy bars ----
	for (const auto& Pair : Sub->EnemyHealthMap)
	{
		const FEnemyBarData& Enemy = Pair.Value;

		// Skip: not visible or dead
		if (!Enemy.bBarVisible || Enemy.bIsDead) continue;

		// Get enemy feet position (cached actor for smooth tracking, fallback to socket position)
		FVector EnemyFeetPos;
		if (!Sub->GetEnemyFeetPosition(Enemy, EnemyFeetPos)) continue;

		FVector2D ScreenPos;
		if (!Sub->ProjectWorldToScreen(EnemyFeetPos, ScreenPos)) continue;

		const float HPPercent = FMath::Clamp(
			(float)Enemy.CurrentHP / (float)FMath::Max(Enemy.MaxHP, 1), 0.f, 1.f);
		const bool bCritical = HPPercent <= CRITICAL_THRESHOLD;

		DrawEnemyBar(OutDrawElements, BarLayerId, AllottedGeometry, InvScale,
			ScreenPos, HPPercent, bCritical);
	}

	// ---- Draw NPC name labels (screen-space text above NPC actors) ----
	const int32 TextLayerId = BarLayerId + 2;
	for (const FNPCNameData& NPC : Sub->NPCNames)
	{
		if (!NPC.Actor.IsValid()) continue;

		// Project NPC head position to screen (offset above capsule)
		FVector NPCPos = NPC.Actor->GetActorLocation();
		NPCPos.Z += 120.f; // Above the NPC's head

		FVector2D ScreenPos;
		if (!Sub->ProjectWorldToScreen(NPCPos, ScreenPos)) continue;

		DrawNPCName(OutDrawElements, TextLayerId, AllottedGeometry, InvScale,
			ScreenPos, NPC.DisplayName);
	}

	return TextLayerId;
}

// ============================================================
// Draw player bar: HP (green) + SP (blue) stacked vertically
// ============================================================

void SWorldHealthBarOverlay::DrawPlayerBar(
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FGeometry& AllottedGeometry,
	float InvScale,
	const FVector2D& ScreenPos,
	float HPPercent,
	float SPPercent,
	bool bCritical) const
{
	// Convert screen-pixel position to Slate local coordinates
	const float CenterX = (float)ScreenPos.X * InvScale;
	const float TopY = (float)ScreenPos.Y * InvScale + VERTICAL_OFFSET;

	// Bar origin: centered horizontally on the character
	const float BarX = CenterX - BAR_WIDTH * 0.5f;
	const float BarY = TopY;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Layer 0: Navy border (outer rectangle)
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(BAR_WIDTH, PLAYER_BAR_H),
			FSlateLayoutTransform(FVector2f(BarX, BarY))
		),
		WhiteBrush, ESlateDrawEffect::None,
		WorldBarColors::NavyBorder
	);

	// Layer 1: HP bar (top section)
	{
		const float FillX = BarX + BAR_BORDER;
		const float FillY = BarY + BAR_BORDER;
		const float FillW = BAR_WIDTH - 2.f * BAR_BORDER;
		const float FillH = BAR_FILL_H;

		const FLinearColor HPColor = bCritical
			? WorldBarColors::PlayerHPCritRed
			: WorldBarColors::PlayerHPGreen;

		DrawFilledBar(OutDrawElements, LayerId + 1, AllottedGeometry,
			FillX, FillY, FillW, FillH, HPPercent, HPColor);
	}

	// Layer 2: SP bar (bottom section, below the navy divider)
	{
		const float FillX = BarX + BAR_BORDER;
		const float FillY = BarY + BAR_BORDER + BAR_FILL_H + BAR_DIVIDER;
		const float FillW = BAR_WIDTH - 2.f * BAR_BORDER;
		const float FillH = BAR_FILL_H;

		DrawFilledBar(OutDrawElements, LayerId + 1, AllottedGeometry,
			FillX, FillY, FillW, FillH, SPPercent, WorldBarColors::PlayerSPBlue);
	}
}

// ============================================================
// Draw enemy bar: HP only (pink/magenta)
// ============================================================

void SWorldHealthBarOverlay::DrawEnemyBar(
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FGeometry& AllottedGeometry,
	float InvScale,
	const FVector2D& ScreenPos,
	float HPPercent,
	bool bCritical) const
{
	const float CenterX = (float)ScreenPos.X * InvScale;
	const float TopY = (float)ScreenPos.Y * InvScale + VERTICAL_OFFSET;

	const float BarX = CenterX - BAR_WIDTH * 0.5f;
	const float BarY = TopY;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Layer 0: Navy border (outer rectangle)
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(BAR_WIDTH, ENEMY_BAR_H),
			FSlateLayoutTransform(FVector2f(BarX, BarY))
		),
		WhiteBrush, ESlateDrawEffect::None,
		WorldBarColors::NavyBorder
	);

	// Layer 1: HP fill
	{
		const float FillX = BarX + BAR_BORDER;
		const float FillY = BarY + BAR_BORDER;
		const float FillW = BAR_WIDTH - 2.f * BAR_BORDER;
		const float FillH = BAR_FILL_H;

		const FLinearColor HPColor = bCritical
			? WorldBarColors::EnemyHPCritYellow
			: WorldBarColors::EnemyHPPink;

		DrawFilledBar(OutDrawElements, LayerId + 1, AllottedGeometry,
			FillX, FillY, FillW, FillH, HPPercent, HPColor);
	}
}

// ============================================================
// Draw a single filled bar: dark background + colored fill
// ============================================================

void SWorldHealthBarOverlay::DrawFilledBar(
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FGeometry& AllottedGeometry,
	float X, float Y, float Width, float Height,
	float FillPercent,
	const FLinearColor& FillColor) const
{
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Dark gray background (full width)
	FSlateDrawElement::MakeBox(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(Width, Height),
			FSlateLayoutTransform(FVector2f(X, Y))
		),
		WhiteBrush, ESlateDrawEffect::None,
		WorldBarColors::DarkGrayBg
	);

	// Colored fill (percentage width)
	if (FillPercent > 0.f)
	{
		const float FillWidth = FMath::Max(Width * FillPercent, 1.f); // At least 1px when > 0%

		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(FillWidth, Height),
				FSlateLayoutTransform(FVector2f(X, Y))
			),
			WhiteBrush, ESlateDrawEffect::None,
			FillColor
		);
	}
}

// ============================================================
// Draw NPC name — centered white text with shadow above NPC
// ============================================================

void SWorldHealthBarOverlay::DrawNPCName(
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FGeometry& AllottedGeometry,
	float InvScale,
	const FVector2D& ScreenPos,
	const FString& Name) const
{
	FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", NPC_NAME_FONT_SIZE);

	TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D TextSize = FontMeasure->Measure(Name, NameFont);

	const float CenterX = (float)ScreenPos.X * InvScale;
	const float TopY = (float)ScreenPos.Y * InvScale + NPC_NAME_OFFSET_Y;

	const FVector2f TextPos(
		CenterX - (float)TextSize.X * 0.5f,
		TopY - (float)TextSize.Y * 0.5f);

	const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

	// Shadow
	const FVector2f ShadowPos(TextPos.X + 1.0f, TextPos.Y + 1.0f);
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			TextSize,
			FSlateLayoutTransform(ShadowPos)),
		Name,
		NameFont,
		DrawEffects,
		WorldBarColors::NPCNameShadow);

	// Main text
	FSlateDrawElement::MakeText(
		OutDrawElements, LayerId + 1,
		AllottedGeometry.ToPaintGeometry(
			TextSize,
			FSlateLayoutTransform(TextPos)),
		Name,
		NameFont,
		DrawEffects,
		WorldBarColors::NPCNameWhite);
}
