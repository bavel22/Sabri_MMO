// SMinimapWidget.cpp — RO Classic minimap: live overhead camera + entity dots.

#include "SMinimapWidget.h"
#include "MinimapSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EnemySubsystem.h"
#include "OtherPlayerSubsystem.h"
#include "PartySubsystem.h"
#include "EngineUtils.h"

// RO Classic minimap colors
namespace MinimapColors
{
	static const FLinearColor Background(0.08f, 0.06f, 0.04f, 0.90f);
	static const FLinearColor BorderOuter(0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor BorderInner(0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor TitleBg(0.0f, 0.0f, 0.0f, 0.55f);

	static const FLinearColor PlayerArrow(1.f, 1.f, 1.f, 1.f);
	static const FLinearColor WarpDot(0.9f, 0.15f, 0.15f, 1.f);
	static const FLinearColor EnemyDot(0.9f, 0.35f, 0.10f, 1.f);
	static const FLinearColor OtherPlayerDot(0.85f, 0.85f, 0.85f, 1.f);
	static const FLinearColor PartyDot(0.90f, 0.40f, 0.65f, 1.f);
	static const FLinearColor NPCDot(0.40f, 0.75f, 0.95f, 1.f);

	static const FLinearColor TitleText(0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor ZoomBtnText(0.92f, 0.80f, 0.45f, 1.f);
}

// ============================================================
// Construct
// ============================================================

void SMinimapWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	LastTickTime = FPlatformTime::Seconds();

	RegisterActiveTimer(0.03f, FWidgetActiveTimerDelegate::CreateLambda(
		[](double, float) -> EActiveTimerReturnType { return EActiveTimerReturnType::Continue; }));

	UMinimapSubsystem* Sub = OwningSubsystem.Get();

	// Compact layout: just a fixed-size box. All visuals drawn in OnPaint.
	// Only Slate children = zone name text + zoom buttons (overlaid).
	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(TotalSize)
		.HeightOverride(TotalSize)
		[
			SNew(SOverlay)

			// Zone name (overlaid inside top of map)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			.Padding(FMargin(FramePad, FramePad + 1.f, FramePad, 0.f))
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(Sub->CurrentDisplayName);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(MinimapColors::TitleText))
				.Justification(ETextJustify::Center)
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f))
			]

			// Zoom buttons (bottom-right, inside frame)
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(0.f, 0.f, FramePad + 2.f, FramePad + 2.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 3, 0)
				[
					SNew(SButton)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.OnClicked_Lambda([Sub]() -> FReply {
						if (Sub) Sub->ZoomIn();
						return FReply::Handled();
					})
					.ContentPadding(FMargin(2.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("+")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FSlateColor(MinimapColors::ZoomBtnText))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
					.OnClicked_Lambda([Sub]() -> FReply {
						if (Sub) Sub->ZoomOut();
						return FReply::Handled();
					})
					.ContentPadding(FMargin(2.f, 0.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("-")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FSlateColor(MinimapColors::ZoomBtnText))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f))
					]
				]
			]
		]
	];
}

FVector2D SMinimapWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(TotalSize, TotalSize);
}

// ============================================================
// Drag support
// ============================================================

void SMinimapWidget::ApplyPosition()
{
	SetRenderTransform(FSlateRenderTransform(FVector2f((float)WidgetPosition.X, (float)WidgetPosition.Y)));
}

FReply SMinimapWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		DragOffset = MouseEvent.GetScreenSpacePosition();
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	return FReply::Unhandled();
}

FReply SMinimapWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SMinimapWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
			? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyPosition();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// OnPaint
// ============================================================

int32 SMinimapWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (Sub->OpacityState == 0)
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	// Update blink timer
	double CurrentTime = FPlatformTime::Seconds();
	float DeltaTime = (float)(CurrentTime - LastTickTime);
	LastTickTime = CurrentTime;
	AccumulatedTime += DeltaTime;

	// Map geometry: inside the frame border
	const FGeometry MapGeo = AllottedGeometry.MakeChild(
		FVector2D(MapSize, MapSize),
		FSlateLayoutTransform(FVector2f(FramePad, FramePad)));

	DrawMinimapBackground(MapGeo, OutDrawElements, LayerId);
	LayerId++;

	DrawEnemyDots(MapGeo, OutDrawElements, LayerId);
	DrawOtherPlayerDots(MapGeo, OutDrawElements, LayerId);
	DrawNPCDots(MapGeo, OutDrawElements, LayerId);
	DrawWarpDots(MapGeo, OutDrawElements, LayerId);
	LayerId++;

	DrawMinimapMarks(MapGeo, OutDrawElements, LayerId);
	LayerId++;

	DrawPlayerArrow(MapGeo, OutDrawElements, LayerId);
	LayerId++;

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

// ============================================================
// Capture brush
// ============================================================

void SMinimapWidget::EnsureCaptureBrush() const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->MinimapRenderTarget) return;

	if (!bCaptureBrushReady)
	{
		CaptureBrush.SetResourceObject(Sub->MinimapRenderTarget);
		CaptureBrush.ImageSize = FVector2D(256.f, 256.f);
		CaptureBrush.DrawAs = ESlateBrushDrawType::Image;
		CaptureBrush.Tiling = ESlateBrushTileType::NoTile;
		bCaptureBrushReady = true;
	}
}

// ============================================================
// Draw methods
// ============================================================

void SMinimapWidget::DrawMinimapBackground(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;
	const FSlateBrush* WB = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Gold outer frame
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(
			FVector2D(MapSize + 6.f, MapSize + 6.f),
			FSlateLayoutTransform(FVector2f(-3.f, -3.f))),
		WB, ESlateDrawEffect::None,
		MinimapColors::BorderOuter * FLinearColor(1, 1, 1, Alpha));

	// Dark inner frame
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(
			FVector2D(MapSize + 2.f, MapSize + 2.f),
			FSlateLayoutTransform(FVector2f(-1.f, -1.f))),
		WB, ESlateDrawEffect::None,
		MinimapColors::BorderInner * FLinearColor(1, 1, 1, Alpha));

	// Dark background
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(), WB, ESlateDrawEffect::None,
		MinimapColors::Background * FLinearColor(1, 1, 1, Alpha));

	// Overhead camera capture
	EnsureCaptureBrush();
	if (bCaptureBrushReady)
	{
		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(), &CaptureBrush, ESlateDrawEffect::None,
			FLinearColor(1, 1, 1, Alpha));
	}

	// Semi-transparent strip behind zone name (inside map, top)
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(
			FVector2D(MapSize, 16.f),
			FSlateLayoutTransform(FVector2f(0.f, 0.f))),
		WB, ESlateDrawEffect::None,
		MinimapColors::TitleBg * FLinearColor(1, 1, 1, Alpha));
}

void SMinimapWidget::DrawPlayerArrow(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->GetWorld()) return;

	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(Sub->GetWorld(), 0);
	if (!PlayerChar) return;

	float CenterX = MapSize * 0.5f;
	float CenterY = MapSize * 0.5f;

	const float DotSize = 6.f;
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(
			FVector2D(DotSize, DotSize),
			FSlateLayoutTransform(FVector2f(CenterX - DotSize * 0.5f, CenterY - DotSize * 0.5f))),
		FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None,
		MinimapColors::PlayerArrow * FLinearColor(1, 1, 1, Alpha));

	FRotator Rot = PlayerChar->GetActorRotation();
	float AngleRad = FMath::DegreesToRadians(Rot.Yaw);
	float ArrowLen = 8.f;
	float EndX = CenterX + FMath::Sin(AngleRad) * ArrowLen;
	float EndY = CenterY - FMath::Cos(AngleRad) * ArrowLen;

	TArray<FVector2D> LinePoints;
	LinePoints.Add(FVector2D(CenterX, CenterY));
	LinePoints.Add(FVector2D(EndX, EndY));
	FSlateDrawElement::MakeLines(OutElements, LayerId,
		Geo.ToPaintGeometry(), LinePoints, ESlateDrawEffect::None,
		MinimapColors::PlayerArrow * FLinearColor(1, 1, 1, Alpha), true, 2.f);
}

// ============================================================
// World-to-minimap projection (player-centered overhead camera)
// ============================================================

FVector2D SMinimapWidget::WorldToMinimapPixel(const FVector& WorldPos) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->GetWorld()) return FVector2D(MapSize * 0.5f, MapSize * 0.5f);

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(Sub->GetWorld(), 0);
	if (!PlayerChar) return FVector2D(MapSize * 0.5f, MapSize * 0.5f);

	FVector PlayerPos = PlayerChar->GetActorLocation();
	FVector Delta = WorldPos - PlayerPos;

	int32 ClampedZoom = FMath::Clamp(Sub->ZoomLevel, 0, 4);
	float OrthoWidth = 4000.f / UMinimapSubsystem::ZoomFactors[ClampedZoom];
	float HalfOrtho = OrthoWidth * 0.5f;

	// Camera FRotator(-90, 0, 0): right = +Y, up = +X
	float PixelX = MapSize * 0.5f + (Delta.Y / HalfOrtho) * (MapSize * 0.5f);
	float PixelY = MapSize * 0.5f - (Delta.X / HalfOrtho) * (MapSize * 0.5f);

	return FVector2D(PixelX, PixelY);
}

// ============================================================
// Entity dots
// ============================================================

static void DrawDot(FSlateWindowElementList& OutElements, int32 LayerId, const FGeometry& Geo,
	float PixelX, float PixelY, float DotSize, const FLinearColor& Color, float MapSz)
{
	if (PixelX < -DotSize || PixelX > MapSz + DotSize ||
		PixelY < -DotSize || PixelY > MapSz + DotSize)
		return;

	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(
			FVector2D(DotSize, DotSize),
			FSlateLayoutTransform(FVector2f(PixelX - DotSize * 0.5f, PixelY - DotSize * 0.5f))),
		FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, Color);
}

void SMinimapWidget::DrawEnemyDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->GetWorld()) return;
	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	UEnemySubsystem* EnemySub = Sub->GetWorld()->GetSubsystem<UEnemySubsystem>();
	if (!EnemySub) return;

	for (const auto& Pair : EnemySub->GetAllEnemies())
	{
		const FEnemyEntry& Entry = Pair.Value;
		if (Entry.bIsDead || !Entry.Actor.IsValid()) continue;
		FVector2D Pixel = WorldToMinimapPixel(Entry.Actor->GetActorLocation());
		DrawDot(OutElements, LayerId, Geo, Pixel.X, Pixel.Y, 3.f,
			MinimapColors::EnemyDot * FLinearColor(1, 1, 1, Alpha), MapSize);
	}
}

void SMinimapWidget::DrawOtherPlayerDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->GetWorld()) return;
	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	UOtherPlayerSubsystem* PlayerSub = Sub->GetWorld()->GetSubsystem<UOtherPlayerSubsystem>();
	if (!PlayerSub) return;

	TSet<int32> PartyIds;
	UPartySubsystem* PartySub = Sub->GetWorld()->GetSubsystem<UPartySubsystem>();
	if (PartySub && PartySub->bInParty)
	{
		for (const auto& Member : PartySub->Members)
			PartyIds.Add(Member.CharacterId);
	}

	for (const auto& Pair : PlayerSub->GetAllPlayers())
	{
		const FPlayerEntry& Entry = Pair.Value;
		if (!Entry.Actor.IsValid()) continue;
		if (PlayerSub->IsPlayerHidden(Entry.CharacterId)) continue;

		FVector2D Pixel = WorldToMinimapPixel(Entry.Actor->GetActorLocation());
		FLinearColor DotColor = MinimapColors::OtherPlayerDot;
		float DotSz = 3.f;
		if (PartyIds.Contains(Entry.CharacterId))
		{
			DotColor = MinimapColors::PartyDot;
			DotSz = 5.f;
		}
		DrawDot(OutElements, LayerId, Geo, Pixel.X, Pixel.Y, DotSz,
			DotColor * FLinearColor(1, 1, 1, Alpha), MapSize);
	}
}

void SMinimapWidget::DrawNPCDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->GetWorld()) return;
	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	for (TActorIterator<AActor> It(Sub->GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;
		bool bIsNPC = Actor->ActorHasTag(TEXT("NPC")) ||
			Actor->GetClass()->GetName().Contains(TEXT("KafraNPC")) ||
			Actor->GetClass()->GetName().Contains(TEXT("ShopNPC"));
		if (!bIsNPC) continue;

		FVector2D Pixel = WorldToMinimapPixel(Actor->GetActorLocation());
		DrawDot(OutElements, LayerId, Geo, Pixel.X, Pixel.Y, 4.f,
			MinimapColors::NPCDot * FLinearColor(1, 1, 1, Alpha), MapSize);
	}
}

void SMinimapWidget::DrawWarpDots(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;
	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	const auto& Warps = Sub->GetCurrentZoneWarps();
	for (const auto& Warp : Warps)
	{
		FVector WarpPos(Warp.X, Warp.Y, Warp.Z);
		FVector2D Pixel = WorldToMinimapPixel(WarpPos);
		DrawDot(OutElements, LayerId, Geo, Pixel.X, Pixel.Y, 5.f,
			MinimapColors::WarpDot * FLinearColor(1, 1, 1, Alpha), MapSize);
	}
}

void SMinimapWidget::DrawMinimapMarks(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;
	float Alpha = (Sub->OpacityState == 1) ? 0.5f : 1.f;

	bool bVisible = (FMath::Fmod(AccumulatedTime, 1.f) < 0.5f);
	if (!bVisible) return;

	for (const auto& Mark : Sub->MinimapMarks)
	{
		FVector MarkPos(Mark.X, Mark.Y, 0.f);
		FVector2D Pixel = WorldToMinimapPixel(MarkPos);
		FLinearColor MarkColor = Mark.Color * FLinearColor(1, 1, 1, Alpha);

		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(FVector2D(8.f, 2.f),
				FSlateLayoutTransform(FVector2f(Pixel.X - 4.f, Pixel.Y - 1.f))),
			FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, MarkColor);
		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(FVector2D(2.f, 8.f),
				FSlateLayoutTransform(FVector2f(Pixel.X - 1.f, Pixel.Y - 4.f))),
			FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, MarkColor);
	}
}
