// SWorldMapWidget.cpp — Full-screen RO Classic world map with grid overlay.
// Each grid cell maps to a zone. Hovering highlights and shows tooltip.

#include "SWorldMapWidget.h"
#include "MinimapSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"

namespace WorldMapColors
{
	static const FLinearColor Background(0.08f, 0.06f, 0.04f, 0.96f);
	static const FLinearColor GridLine(0.60f, 0.48f, 0.22f, 0.25f);
	static const FLinearColor GridLineHover(0.90f, 0.72f, 0.30f, 0.5f);

	static const FLinearColor CellField(0.30f, 0.50f, 0.25f, 0.18f);
	static const FLinearColor CellTown(0.65f, 0.50f, 0.20f, 0.25f);
	static const FLinearColor CellDungeon(0.55f, 0.18f, 0.15f, 0.22f);
	static const FLinearColor CellOcean(0.f, 0.f, 0.f, 0.f);  // transparent

	static const FLinearColor CellHover(1.f, 1.f, 1.f, 0.15f);
	static const FLinearColor CellHoverBorder(0.95f, 0.80f, 0.35f, 0.8f);
	static const FLinearColor CellCurrentZone(0.95f, 0.85f, 0.40f, 0.30f);

	static const FLinearColor ZoneNameText(1.f, 1.f, 1.f, 0.92f);
	static const FLinearColor LevelText(0.85f, 0.80f, 0.65f, 0.85f);
	static const FLinearColor MonsterText(0.50f, 1.f, 0.50f, 0.90f);
	static const FLinearColor TitleText(0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor InstructionText(0.80f, 0.75f, 0.65f, 0.6f);

	static const FLinearColor TooltipBg(0.12f, 0.08f, 0.04f, 0.94f);
	static const FLinearColor TooltipBorder(0.72f, 0.58f, 0.28f, 1.f);

	static const FLinearColor CurrentLocDot(1.f, 1.f, 1.f, 1.f);
	static const FLinearColor PartyDot(0.90f, 0.40f, 0.60f, 1.f);
}

// ============================================================

void SWorldMapWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (Sub && Sub->WorldMapTexture)
	{
		WorldMapBrush.SetResourceObject(Sub->WorldMapTexture);
		WorldMapBrush.ImageSize = FVector2D(Sub->WorldMapTexture->GetSizeX(), Sub->WorldMapTexture->GetSizeY());
		WorldMapBrush.DrawAs = ESlateBrushDrawType::Image;
		WorldMapBrush.Tiling = ESlateBrushTileType::NoTile;
		bHasTexture = true;
	}

	SetVisibility(EVisibility::Visible);

	RegisterActiveTimer(0.033f, FWidgetActiveTimerDelegate::CreateLambda(
		[](double, float) -> EActiveTimerReturnType { return EActiveTimerReturnType::Continue; }));

	// Full-viewport interactive area — transparent but hittable so mouse/key events arrive.
	// OnPaint draws everything; this SBorder is purely a hit-test target.
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.f))
		.Visibility(EVisibility::Visible)
	];

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
	}
}

// ============================================================
// Geometry helpers — compute the map draw area (aspect-fitted)
// ============================================================

struct FMapDrawArea
{
	float X, Y, W, H;  // pixel position and size of the map image
	float CellW, CellH; // size of one grid cell
	int32 Cols, Rows;
};

static FMapDrawArea ComputeMapArea(const FGeometry& Geo, const FSlateBrush& Brush, bool bHasTex, int32 Cols, int32 Rows)
{
	const FVector2D Size = Geo.GetLocalSize();
	const float MarginX = Size.X * 0.03f;
	const float MarginTop = 36.f;  // title
	const float MarginBot = 28.f;  // instructions
	const float AreaW = Size.X - MarginX * 2.f;
	const float AreaH = Size.Y - MarginTop - MarginBot;

	FMapDrawArea A;
	A.Cols = FMath::Max(Cols, 1);
	A.Rows = FMath::Max(Rows, 1);
	A.X = MarginX;
	A.Y = MarginTop;
	A.W = AreaW;
	A.H = AreaH;

	if (bHasTex && Brush.ImageSize.Y > 0.f)
	{
		const float TexAspect = Brush.ImageSize.X / Brush.ImageSize.Y;
		const float AreaAspect = AreaW / FMath::Max(AreaH, 1.f);
		if (TexAspect > AreaAspect)
		{
			A.H = AreaW / TexAspect;
			A.Y = MarginTop + (AreaH - A.H) * 0.5f;
		}
		else
		{
			A.W = AreaH * TexAspect;
			A.X = MarginX + (AreaW - A.W) * 0.5f;
		}
	}

	A.CellW = A.W / (float)A.Cols;
	A.CellH = A.H / (float)A.Rows;
	return A;
}

FVector2D SWorldMapWidget::NormToPixel(const FGeometry& Geo, float NormX, float NormY) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	int32 Cols = Sub ? Sub->GridCols : 12;
	int32 Rows = Sub ? Sub->GridRows : 8;
	FMapDrawArea A = ComputeMapArea(Geo, WorldMapBrush, bHasTexture, Cols, Rows);
	return FVector2D(A.X + NormX * A.W, A.Y + NormY * A.H);
}

FBox2D SWorldMapWidget::GetZonePixelBounds(const FGeometry& Geo, float X1, float Y1, float X2, float Y2) const
{
	FVector2D Min = NormToPixel(Geo, X1, Y1);
	FVector2D Max = NormToPixel(Geo, X2, Y2);
	return FBox2D(Min, Max);
}

// ============================================================
// OnPaint — main render
// ============================================================

int32 SWorldMapWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return LayerId;

	DrawBackground(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;
	DrawZoneRectangles(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;
	DrawCurrentLocation(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;
	DrawPartyMembers(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;
	DrawZoneTooltip(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;
	DrawInstructions(AllottedGeometry, OutDrawElements, LayerId);
	LayerId++;

	return LayerId;
}

// ============================================================
// Draw methods
// ============================================================

void SWorldMapWidget::DrawBackground(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	const FVector2D Size = Geo.GetLocalSize();

	// Dark fullscreen background
	FSlateDrawElement::MakeBox(OutElements, LayerId, Geo.ToPaintGeometry(),
		FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, WorldMapColors::Background);

	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	int32 Cols = Sub ? Sub->GridCols : 12;
	int32 Rows = Sub ? Sub->GridRows : 8;
	FMapDrawArea A = ComputeMapArea(Geo, WorldMapBrush, bHasTexture, Cols, Rows);

	// Draw world map texture
	if (bHasTexture)
	{
		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(FVector2D(A.W, A.H), FSlateLayoutTransform(FVector2f(A.X, A.Y))),
			&WorldMapBrush, ESlateDrawEffect::None, FLinearColor::White);
	}

	// Title
	FSlateDrawElement::MakeText(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(Size.X, 28.f), FSlateLayoutTransform(FVector2f(A.X, 6.f))),
		TEXT("World Map"),
		FCoreStyle::GetDefaultFontStyle("Bold", 14),
		ESlateDrawEffect::None, WorldMapColors::TitleText);
}

void SWorldMapWidget::DrawZoneRectangles(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	FMapDrawArea A = ComputeMapArea(Geo, WorldMapBrush, bHasTexture, Sub->GridCols, Sub->GridRows);

	HoveredZoneName = TEXT("");

	// Determine which grid cell the mouse is in
	int32 HoverCol = -1, HoverRow = -1;
	if (LastMousePos.X >= A.X && LastMousePos.X < A.X + A.W &&
		LastMousePos.Y >= A.Y && LastMousePos.Y < A.Y + A.H)
	{
		HoverCol = FMath::Clamp((int32)((LastMousePos.X - A.X) / A.CellW), 0, A.Cols - 1);
		HoverRow = FMath::Clamp((int32)((LastMousePos.Y - A.Y) / A.CellH), 0, A.Rows - 1);
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	for (int32 Row = 0; Row < A.Rows; ++Row)
	{
		for (int32 Col = 0; Col < A.Cols; ++Col)
		{
			FString ZoneName = Sub->GetGridZone(Col, Row);
			bool bIsOcean = ZoneName.IsEmpty();
			bool bIsHovered = (Col == HoverCol && Row == HoverRow && !bIsOcean);
			bool bIsCurrentZone = (!bIsOcean && ZoneName == Sub->CurrentZoneName);

			float CellX = A.X + Col * A.CellW;
			float CellY = A.Y + Row * A.CellH;

			// Cell tint (semi-transparent overlay on the illustration)
			if (!bIsOcean)
			{
				const FZoneMapInfo* Info = Sub->GetZoneInfo(ZoneName);
				FLinearColor FillColor = WorldMapColors::CellField;
				if (Info)
				{
					if (Info->bIsTown) FillColor = WorldMapColors::CellTown;
					else if (Info->bIsDungeon) FillColor = WorldMapColors::CellDungeon;
				}

				if (bIsCurrentZone)
				{
					FillColor = WorldMapColors::CellCurrentZone;
				}

				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(A.CellW, A.CellH),
						FSlateLayoutTransform(FVector2f(CellX, CellY))),
					WhiteBrush, ESlateDrawEffect::None, FillColor);
			}

			// Hover highlight
			if (bIsHovered)
			{
				HoveredZoneName = ZoneName;

				// Bright hover fill
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(A.CellW, A.CellH),
						FSlateLayoutTransform(FVector2f(CellX, CellY))),
					WhiteBrush, ESlateDrawEffect::None, WorldMapColors::CellHover);

				// Gold hover border (4 edges)
				FLinearColor BC = WorldMapColors::CellHoverBorder;
				float T = 2.f; // border thickness
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(A.CellW, T), FSlateLayoutTransform(FVector2f(CellX, CellY))),
					WhiteBrush, ESlateDrawEffect::None, BC);
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(A.CellW, T), FSlateLayoutTransform(FVector2f(CellX, CellY + A.CellH - T))),
					WhiteBrush, ESlateDrawEffect::None, BC);
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(T, A.CellH), FSlateLayoutTransform(FVector2f(CellX, CellY))),
					WhiteBrush, ESlateDrawEffect::None, BC);
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(T, A.CellH), FSlateLayoutTransform(FVector2f(CellX + A.CellW - T, CellY))),
					WhiteBrush, ESlateDrawEffect::None, BC);
			}

			// Zone name label (shown when names toggled or cell is hovered)
			if (!bIsOcean && (Sub->bShowZoneNames || bIsHovered))
			{
				const FZoneMapInfo* Info = Sub->GetZoneInfo(ZoneName);
				FString Label = Info ? Info->DisplayName : ZoneName;

				// Dark backdrop behind text for readability
				float TextY = CellY + A.CellH * 0.5f - 8.f;
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(A.CellW, 16.f),
						FSlateLayoutTransform(FVector2f(CellX, TextY))),
					WhiteBrush, ESlateDrawEffect::None,
					FLinearColor(0.f, 0.f, 0.f, 0.65f));

				FSlateDrawElement::MakeText(OutElements, LayerId + 1,
					Geo.ToPaintGeometry(FVector2D(A.CellW - 4.f, 14.f),
						FSlateLayoutTransform(FVector2f(CellX + 3.f, TextY + 1.f))),
					Label,
					FCoreStyle::GetDefaultFontStyle("Bold", 8),
					ESlateDrawEffect::None, WorldMapColors::ZoneNameText);

				// Level range below name
				FString LvlRange = Info ? Info->LevelRange : TEXT("");
				if (!LvlRange.IsEmpty())
				{
					float LvlY = TextY + 16.f;
					FSlateDrawElement::MakeBox(OutElements, LayerId,
						Geo.ToPaintGeometry(FVector2D(A.CellW, 12.f),
							FSlateLayoutTransform(FVector2f(CellX, LvlY))),
						WhiteBrush, ESlateDrawEffect::None,
						FLinearColor(0.f, 0.f, 0.f, 0.5f));

					FSlateDrawElement::MakeText(OutElements, LayerId + 1,
						Geo.ToPaintGeometry(FVector2D(A.CellW - 4.f, 12.f),
							FSlateLayoutTransform(FVector2f(CellX + 3.f, LvlY))),
						FString::Printf(TEXT("Lv %s"), *LvlRange),
						FCoreStyle::GetDefaultFontStyle("Regular", 7),
						ESlateDrawEffect::None, WorldMapColors::LevelText);
				}
			}

			// Monster info text (when toggled)
			if (!bIsOcean && Sub->bShowMonsterInfo)
			{
				const FZoneMapInfo* Info = Sub->GetZoneInfo(ZoneName);
				if (Info && Info->Monsters.Num() > 0)
				{
					FString MonText = Info->Monsters[0].Name;
					if (Info->Monsters.Num() > 1)
					{
						MonText += FString::Printf(TEXT(" +%d"), Info->Monsters.Num() - 1);
					}

					// Dark backdrop
					float MonY = CellY + A.CellH * 0.5f + 10.f;
					FSlateDrawElement::MakeBox(OutElements, LayerId,
						Geo.ToPaintGeometry(FVector2D(A.CellW, 12.f),
							FSlateLayoutTransform(FVector2f(CellX, MonY))),
						WhiteBrush, ESlateDrawEffect::None,
						FLinearColor(0.f, 0.15f, 0.f, 0.6f));

					FSlateDrawElement::MakeText(OutElements, LayerId + 1,
						Geo.ToPaintGeometry(FVector2D(A.CellW - 4.f, 12.f),
							FSlateLayoutTransform(FVector2f(CellX + 3.f, MonY))),
						MonText,
						FCoreStyle::GetDefaultFontStyle("Regular", 7),
						ESlateDrawEffect::None, WorldMapColors::MonsterText);
				}
			}
		}
	}

	// Draw subtle grid lines
	FLinearColor LineColor = WorldMapColors::GridLine;
	for (int32 Col = 0; Col <= A.Cols; ++Col)
	{
		float LX = A.X + Col * A.CellW;
		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(FVector2D(1.f, A.H), FSlateLayoutTransform(FVector2f(LX, A.Y))),
			WhiteBrush, ESlateDrawEffect::None, LineColor);
	}
	for (int32 Row = 0; Row <= A.Rows; ++Row)
	{
		float LY = A.Y + Row * A.CellH;
		FSlateDrawElement::MakeBox(OutElements, LayerId,
			Geo.ToPaintGeometry(FVector2D(A.W, 1.f), FSlateLayoutTransform(FVector2f(A.X, LY))),
			WhiteBrush, ESlateDrawEffect::None, LineColor);
	}
}

void SWorldMapWidget::DrawCurrentLocation(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || Sub->CurrentZoneName.IsEmpty()) return;

	FMapDrawArea A = ComputeMapArea(Geo, WorldMapBrush, bHasTexture, Sub->GridCols, Sub->GridRows);

	// Find which grid cell(s) contain the current zone — use the first match
	for (int32 Row = 0; Row < A.Rows; ++Row)
	{
		for (int32 Col = 0; Col < A.Cols; ++Col)
		{
			if (Sub->GetGridZone(Col, Row) == Sub->CurrentZoneName)
			{
				float CX = A.X + (Col + 0.5f) * A.CellW;
				float CY = A.Y + (Row + 0.5f) * A.CellH;

				// White pulsing dot
				const float DotSize = 8.f;
				FSlateDrawElement::MakeBox(OutElements, LayerId,
					Geo.ToPaintGeometry(FVector2D(DotSize, DotSize),
						FSlateLayoutTransform(FVector2f(CX - DotSize * 0.5f, CY - DotSize * 0.5f))),
					FCoreStyle::Get().GetBrush("GenericWhiteBox"),
					ESlateDrawEffect::None, WorldMapColors::CurrentLocDot);
				return;
			}
		}
	}
}

void SWorldMapWidget::DrawPartyMembers(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	FMapDrawArea A = ComputeMapArea(Geo, WorldMapBrush, bHasTexture, Sub->GridCols, Sub->GridRows);

	for (const auto& Member : Sub->PartyMemberZones)
	{
		// Skip the local player — they already have the white "You are here" dot
		if (Member.CharacterId == Sub->LocalCharacterId) continue;

		// Find grid cell for this member's zone
		for (int32 Row = 0; Row < A.Rows; ++Row)
		{
			for (int32 Col = 0; Col < A.Cols; ++Col)
			{
				if (Sub->GetGridZone(Col, Row) == Member.ZoneName)
				{
					float CX = A.X + (Col + 0.5f) * A.CellW + 6.f;
					float CY = A.Y + (Row + 0.5f) * A.CellH + 4.f;

					const float DotSize = 6.f;
					FSlateDrawElement::MakeBox(OutElements, LayerId,
						Geo.ToPaintGeometry(FVector2D(DotSize, DotSize),
							FSlateLayoutTransform(FVector2f(CX - DotSize * 0.5f, CY - DotSize * 0.5f))),
						FCoreStyle::Get().GetBrush("GenericWhiteBox"),
						ESlateDrawEffect::None, WorldMapColors::PartyDot);
					goto next_member;  // found first cell, move to next member
				}
			}
		}
		next_member:;
	}
}

void SWorldMapWidget::DrawZoneTooltip(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || HoveredZoneName.IsEmpty()) return;

	const FZoneMapInfo* Info = Sub->GetZoneInfo(HoveredZoneName);

	// Tooltip content
	FString ZoneName = Info ? Info->DisplayName : HoveredZoneName;
	FString TypeStr = Info ? Info->Type : TEXT("Unknown");
	if (TypeStr.Len() > 0) TypeStr[0] = FChar::ToUpper(TypeStr[0]);
	FString LevelStr = Info ? Info->LevelRange : TEXT("");

	int32 MonsterCount = Info ? Info->Monsters.Num() : 0;
	float TipW = 200.f;
	float TipH = 44.f + FMath::Min(MonsterCount, 4) * 14.f;

	float TipX = LastMousePos.X + 18.f;
	float TipY = LastMousePos.Y + 18.f;

	FVector2D ViewSize = Geo.GetLocalSize();
	if (TipX + TipW > ViewSize.X - 10.f) TipX = LastMousePos.X - TipW - 10.f;
	if (TipY + TipH > ViewSize.Y - 10.f) TipY = LastMousePos.Y - TipH - 10.f;

	const FSlateBrush* WB = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Background + border
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(TipW, TipH), FSlateLayoutTransform(FVector2f(TipX, TipY))),
		WB, ESlateDrawEffect::None, WorldMapColors::TooltipBg);
	// Border edges
	FLinearColor BC = WorldMapColors::TooltipBorder;
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(TipW, 2.f), FSlateLayoutTransform(FVector2f(TipX, TipY))),
		WB, ESlateDrawEffect::None, BC);
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(TipW, 1.f), FSlateLayoutTransform(FVector2f(TipX, TipY + TipH - 1.f))),
		WB, ESlateDrawEffect::None, BC);
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(1.f, TipH), FSlateLayoutTransform(FVector2f(TipX, TipY))),
		WB, ESlateDrawEffect::None, BC);
	FSlateDrawElement::MakeBox(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(1.f, TipH), FSlateLayoutTransform(FVector2f(TipX + TipW - 1.f, TipY))),
		WB, ESlateDrawEffect::None, BC);

	// Zone name (bold)
	FSlateDrawElement::MakeText(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(TipW - 12.f, 16.f), FSlateLayoutTransform(FVector2f(TipX + 6.f, TipY + 6.f))),
		ZoneName, FCoreStyle::GetDefaultFontStyle("Bold", 10),
		ESlateDrawEffect::None, WorldMapColors::TitleText);

	// Type + level
	FString SubLine = TypeStr;
	if (!LevelStr.IsEmpty()) SubLine += TEXT("  |  Lv ") + LevelStr;
	FSlateDrawElement::MakeText(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(TipW - 12.f, 12.f), FSlateLayoutTransform(FVector2f(TipX + 6.f, TipY + 24.f))),
		SubLine, FCoreStyle::GetDefaultFontStyle("Regular", 8),
		ESlateDrawEffect::None, WorldMapColors::LevelText);

	// Monster list
	if (Info)
	{
		float MY = TipY + 42.f;
		int32 Cnt = 0;
		for (const auto& Mon : Info->Monsters)
		{
			if (Cnt >= 4) break;
			FString MonStr = FString::Printf(TEXT("%s (Lv %d)"), *Mon.Name, Mon.Level);
			FSlateDrawElement::MakeText(OutElements, LayerId,
				Geo.ToPaintGeometry(FVector2D(TipW - 16.f, 12.f), FSlateLayoutTransform(FVector2f(TipX + 10.f, MY))),
				MonStr, FCoreStyle::GetDefaultFontStyle("Regular", 7),
				ESlateDrawEffect::None, WorldMapColors::MonsterText);
			MY += 14.f;
			Cnt++;
		}
	}
}

void SWorldMapWidget::DrawInstructions(const FGeometry& Geo, FSlateWindowElementList& OutElements, int32& LayerId) const
{
	const FVector2D Size = Geo.GetLocalSize();
	FSlateDrawElement::MakeText(OutElements, LayerId,
		Geo.ToPaintGeometry(FVector2D(400.f, 14.f),
			FSlateLayoutTransform(FVector2f(Size.X * 0.5f - 200.f, Size.Y - 22.f))),
		TEXT("M: Close  |  N: Zone Names  |  Tab: Monster Info"),
		FCoreStyle::GetDefaultFontStyle("Regular", 8),
		ESlateDrawEffect::None, WorldMapColors::InstructionText);
}

// ============================================================
// Input handling
// ============================================================

FReply SWorldMapWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SWorldMapWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	LastMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

FReply SWorldMapWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	UMinimapSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FReply::Unhandled();

	FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Escape || Key == EKeys::M)
	{
		Sub->CloseWorldMap();
		return FReply::Handled();
	}

	if (Key == EKeys::Tab)
	{
		Sub->ToggleMonsterInfo();
		return FReply::Handled();
	}

	if (Key == EKeys::N)
	{
		Sub->ToggleZoneNames();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
