// SCastBarOverlay.cpp — RO-style cast bar rendering overlay.
// Renders a skill name label + green progress bar above each casting
// character's head using OnPaint with world-to-screen projection.

#include "SCastBarOverlay.h"
#include "CastBarSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SNullWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogCastBarOverlay, Log, All);

// ============================================================
// RO-style Cast Bar Color Palette
// ============================================================
namespace CastBarColors
{
	// Label background (dark translucent, matching RO reference)
	static const FLinearColor LabelBg       (0.10f, 0.12f, 0.10f, 0.88f);
	static const FLinearColor LabelBorder   (0.35f, 0.40f, 0.35f, 0.90f);

	// Bar colors
	static const FLinearColor BarBackground (0.08f, 0.06f, 0.04f, 0.85f);
	static const FLinearColor BarBorder     (0.35f, 0.40f, 0.35f, 0.90f);
	static const FLinearColor BarFill       (0.15f, 0.75f, 0.20f, 1.0f);  // Green

	// Text
	static const FLinearColor TextWhite     (1.00f, 1.00f, 1.00f, 1.0f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.90f);
}

// ============================================================
// Construction
// ============================================================

void SCastBarOverlay::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	SetVisibility(EVisibility::SelfHitTestInvisible);

	ChildSlot
	[
		SNullWidget::NullWidget
	];

	RegisterActiveTimer(0.0f,
		FWidgetActiveTimerDelegate::CreateSP(this, &SCastBarOverlay::OnAnimationTick));
}

// ============================================================
// Active timer — invalidate for repaint each frame
// ============================================================

EActiveTimerReturnType SCastBarOverlay::OnAnimationTick(double InCurrentTime, float InDeltaTime)
{
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

// ============================================================
// Find world position of a caster by their characterId
// ============================================================

FVector SCastBarOverlay::FindCasterWorldPosition(int32 CasterId) const
{
	UCastBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FVector::ZeroVector;

	UWorld* World = Sub->GetWorld();
	if (!World) return FVector::ZeroVector;

	// Local player — use the controlled pawn directly
	if (CasterId == Sub->LocalCharacterId)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC && PC->GetPawn())
		{
			return PC->GetPawn()->GetActorLocation();
		}
		return FVector::ZeroVector;
	}

	// Other players — find BP actor with matching CharacterId property
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		FProperty* Prop = Actor->GetClass()->FindPropertyByName(FName("CharacterId"));
		if (!Prop) continue;

		FIntProperty* IntProp = CastField<FIntProperty>(Prop);
		if (!IntProp) continue;

		int32 ActorCharId = IntProp->GetPropertyValue_InContainer(Actor);
		if (ActorCharId == CasterId)
		{
			return Actor->GetActorLocation();
		}
	}

	return FVector::ZeroVector;
}

// ============================================================
// World-to-screen projection
// ============================================================

bool SCastBarOverlay::ProjectWorldToScreen(const FVector& WorldPos, FVector2D& OutScreenPos) const
{
	UCastBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return false;

	UWorld* World = Sub->GetWorld();
	if (!World) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) return false;

	return PC->ProjectWorldLocationToScreen(WorldPos, OutScreenPos, false);
}

// ============================================================
// OnPaint — render cast bars above each caster's head
// ============================================================

int32 SCastBarOverlay::OnPaint(
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

	UCastBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->bCastBarsEnabled || Sub->ActiveCasts.Num() == 0) return OutLayerId;

	const double Now = FPlatformTime::Seconds();
	const ESlateDrawEffect DrawEffects = ESlateDrawEffect::None;

	// DPI scale conversion (screen pixels → Slate local coordinates)
	const float GeometryScale = AllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
	const float InvScale = (GeometryScale > 0.0f) ? (1.0f / GeometryScale) : 1.0f;

	const FSlateBrush* WhiteBox = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Font for skill name
	FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", SKILL_NAME_FONT_SIZE);

	TSharedRef<FSlateFontMeasure> FontMeasure =
		FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	// Layer ordering
	const int32 BorderLayer = OutLayerId + 1;
	const int32 BgLayer = OutLayerId + 2;
	const int32 FillLayer = OutLayerId + 3;
	const int32 TextLayer = OutLayerId + 4;

	for (const auto& Pair : Sub->ActiveCasts)
	{
		const FCastBarEntry& Cast = Pair.Value;

		// Find caster world position
		FVector WorldPos = FindCasterWorldPosition(Cast.CasterId);
		if (WorldPos.IsZero()) continue;

		// Offset above head
		WorldPos.Z += HEAD_OFFSET_Z;

		// Project to screen
		FVector2D ScreenPos;
		if (!ProjectWorldToScreen(WorldPos, ScreenPos)) continue;

		// Calculate progress (0 → 1 as cast progresses)
		float Progress = 0.0f;
		if (Cast.CastDuration > 0.0f)
		{
			float Elapsed = (float)(Now - Cast.CastStartTime);
			Progress = FMath::Clamp(Elapsed / Cast.CastDuration, 0.0f, 1.0f);
		}

		// Convert to Slate local coordinates (DPI-adjusted)
		const float CenterX = (float)ScreenPos.X * InvScale;
		const float CenterY = (float)ScreenPos.Y * InvScale;

		// ---- Measure skill name text ----
		const FVector2D TextMeasure = FontMeasure->Measure(Cast.SkillName, NameFont);
		const float TextW = (float)TextMeasure.X;
		const float TextH = (float)TextMeasure.Y;

		// Label background dimensions (text + padding)
		const float LabelW = FMath::Max(TextW + LABEL_PADDING_H * 2.0f, BAR_WIDTH);
		const float LabelH = TextH + LABEL_PADDING_V * 2.0f;

		// Total height: label + gap + bar
		const float TotalH = LabelH + LABEL_BAR_GAP + BAR_BORDER * 2.0f + BAR_HEIGHT;

		// Position everything centered on CenterX, above the projected point
		const float BlockTop = CenterY - TotalH;

		// =============== 1. LABEL (skill name with background) ===============

		// Label border
		{
			const FVector2f BorderPos(
				CenterX - LabelW * 0.5f - 1.0f,
				BlockTop - 1.0f);
			const FVector2D BorderSize(LabelW + 2.0f, LabelH + 2.0f);

			FSlateDrawElement::MakeBox(
				OutDrawElements, BorderLayer,
				AllottedGeometry.ToPaintGeometry(BorderSize, FSlateLayoutTransform(BorderPos)),
				WhiteBox, DrawEffects,
				CastBarColors::LabelBorder);
		}

		// Label background
		{
			const FVector2f BgPos(
				CenterX - LabelW * 0.5f,
				BlockTop);
			const FVector2D BgSize(LabelW, LabelH);

			FSlateDrawElement::MakeBox(
				OutDrawElements, BgLayer,
				AllottedGeometry.ToPaintGeometry(BgSize, FSlateLayoutTransform(BgPos)),
				WhiteBox, DrawEffects,
				CastBarColors::LabelBg);
		}

		// Skill name text (centered in label)
		{
			const FVector2f TextPos(
				CenterX - TextW * 0.5f,
				BlockTop + LABEL_PADDING_V);

			// Shadow
			const FVector2f ShadowPos(TextPos.X + 1.0f, TextPos.Y + 1.0f);
			FSlateDrawElement::MakeText(
				OutDrawElements, TextLayer,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TextW, TextH),
					FSlateLayoutTransform(ShadowPos)),
				Cast.SkillName,
				NameFont,
				DrawEffects,
				CastBarColors::TextShadow);

			// Main text
			FSlateDrawElement::MakeText(
				OutDrawElements, TextLayer,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(TextW, TextH),
					FSlateLayoutTransform(TextPos)),
				Cast.SkillName,
				NameFont,
				DrawEffects,
				CastBarColors::TextWhite);
		}

		// =============== 2. PROGRESS BAR ===============

		const float BarTop = BlockTop + LabelH + LABEL_BAR_GAP;

		// Bar border
		{
			const FVector2f BorderPos(
				CenterX - BAR_WIDTH * 0.5f - BAR_BORDER,
				BarTop);
			const FVector2D BorderSize(
				BAR_WIDTH + BAR_BORDER * 2.0f,
				BAR_HEIGHT + BAR_BORDER * 2.0f);

			FSlateDrawElement::MakeBox(
				OutDrawElements, BorderLayer,
				AllottedGeometry.ToPaintGeometry(BorderSize, FSlateLayoutTransform(BorderPos)),
				WhiteBox, DrawEffects,
				CastBarColors::BarBorder);
		}

		// Bar background (dark)
		{
			const FVector2f BgPos(
				CenterX - BAR_WIDTH * 0.5f,
				BarTop + BAR_BORDER);
			const FVector2D BgSize(BAR_WIDTH, BAR_HEIGHT);

			FSlateDrawElement::MakeBox(
				OutDrawElements, BgLayer,
				AllottedGeometry.ToPaintGeometry(BgSize, FSlateLayoutTransform(BgPos)),
				WhiteBox, DrawEffects,
				CastBarColors::BarBackground);
		}

		// Bar fill (green, proportional to cast progress)
		{
			const float FillWidth = BAR_WIDTH * Progress;
			if (FillWidth > 0.5f)
			{
				const FVector2f FillPos(
					CenterX - BAR_WIDTH * 0.5f,
					BarTop + BAR_BORDER);
				const FVector2D FillSize(FillWidth, BAR_HEIGHT);

				FSlateDrawElement::MakeBox(
					OutDrawElements, FillLayer,
					AllottedGeometry.ToPaintGeometry(FillSize, FSlateLayoutTransform(FillPos)),
					WhiteBox, DrawEffects,
					CastBarColors::BarFill);
			}
		}
	}

	return TextLayer;
}
