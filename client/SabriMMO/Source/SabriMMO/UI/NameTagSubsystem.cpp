// NameTagSubsystem.cpp — Single Slate OnPaint overlay renders ALL entity name tags.
// Phase 5 of Blueprint-to-C++ migration. Replaces N per-actor WBP_PlayerNameTag WidgetComponents.
// RO Classic (pre-renewal): players=always visible, monsters/NPCs=hover-only,
// 4-pass black outline, level-based monster colors, party members blue.

#include "NameTagSubsystem.h"
#include "TargetingSubsystem.h"
#include "MMOGameInstance.h"
#include "Sprite/SpriteAtlasData.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWeakWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Fonts/FontMeasure.h"

DEFINE_LOG_CATEGORY_STATIC(LogNameTag, Log, All);

// ============================================================
// RO Classic Name Tag Colors (pre-renewal, from roBrowser source)
// ============================================================

namespace NameTagColors
{
	// Player names
	static const FLinearColor PlayerWhite     (1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF
	static const FLinearColor PartyBlue       (0.33f, 0.60f, 1.00f, 1.00f); // #5599FF
	static const FLinearColor GMYellow        (1.00f, 1.00f, 0.00f, 1.00f); // #FFFF00

	// Monster names (level-based)
	static const FLinearColor MonsterGrey     (0.75f, 0.75f, 0.75f, 1.00f); // #C0C0C0 — much lower level
	static const FLinearColor MonsterWhite    (1.00f, 1.00f, 1.00f, 1.00f); // #FFFFFF — similar level
	static const FLinearColor MonsterRed      (1.00f, 0.00f, 0.00f, 1.00f); // #FF0000 — much higher level

	// NPC names
	static const FLinearColor NPCLightBlue   (0.58f, 0.74f, 0.97f, 1.00f); // #94BDF7

	// Outline
	static const FLinearColor OutlineBlack    (0.00f, 0.00f, 0.00f, 0.85f);
}

// ============================================================
// Level-based monster name color (RO Classic)
// ============================================================

static FLinearColor GetMonsterNameColor(int32 PlayerLevel, int32 MonsterLevel)
{
	int32 Diff = MonsterLevel - PlayerLevel;
	if (Diff <= -10) return NameTagColors::MonsterGrey;
	if (Diff >= 10)  return NameTagColors::MonsterRed;
	return NameTagColors::MonsterWhite;
}

// ============================================================
// SNameTagOverlay — OnPaint widget rendering all name tags
// ============================================================

class SNameTagOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNameTagOverlay) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UNameTagSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Sub = InArgs._Subsystem;
		SetVisibility(EVisibility::SelfHitTestInvisible);
		ChildSlot[SNullWidget::NullWidget];
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (!Sub) return LayerId;

		UWorld* World = Sub->GetWorld();
		if (!World) return LayerId;

		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) return LayerId;

		// Get hovered actor from TargetingSubsystem (for monster/NPC hover-only display)
		UTargetingSubsystem* TargetSub = World->GetSubsystem<UTargetingSubsystem>();
		AActor* HoveredActor = TargetSub ? TargetSub->GetHoveredActor() : nullptr;

		const int32 LocalPlayerLevel = Sub->GetLocalPlayerLevel();

		// Font setup
		const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
		const FSlateFontInfo SubFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);
		TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		// DPI scale: screen pixels → Slate units
		const float DPIScale = (AllottedGeometry.GetLocalSize().X > 0.f)
			? (AllottedGeometry.GetAbsoluteSize().X / AllottedGeometry.GetLocalSize().X) : 1.f;

		const TArray<FNameTagEntry>& Entries = Sub->GetEntries();

		for (const FNameTagEntry& Entry : Entries)
		{
			if (!Entry.Actor.IsValid() || !Entry.bVisible) continue;

			// RO Classic: monsters and NPCs are HOVER-ONLY
			if (Entry.Type == ENameTagEntityType::Monster || Entry.Type == ENameTagEntityType::NPC)
			{
				if (Entry.Actor.Get() != HoveredActor) continue;
			}

			// Project to screen — use sprite height if available for zoom-proportional positioning
			FVector ActorPos = Entry.Actor->GetActorLocation();
			FVector2D ScreenPos;

			if (Entry.SpriteHeight > 0.f)
			{
				// Sprite mode: use billboard up vector for accurate screen bounds
				FVector2D TopScreen, BottomScreen;
				if (!GetSpriteScreenBounds(PC, ActorPos, Entry.SpriteHeight,
				                           TopScreen, BottomScreen))
					continue;

				float SpriteScreenH = BottomScreen.Y - TopScreen.Y;
				// Name position: proportional margin above sprite top (5% of sprite height)
				float Margin = FMath::Max(SpriteScreenH * 0.05f, 4.f);
				ScreenPos.X = TopScreen.X;
				ScreenPos.Y = TopScreen.Y - Margin;
			}
			else
			{
				// Classic mode: world Z offset
				FVector WorldPos = ActorPos;
				WorldPos.Z += Entry.VerticalOffset;
				if (!PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, true))
					continue;
			}

			// Convert screen pixels to Slate units
			FVector2D SlatePos = ScreenPos / DPIScale;

			// Determine name color
			FLinearColor NameColor;
			switch (Entry.Type)
			{
			case ENameTagEntityType::LocalPlayer:
			case ENameTagEntityType::Player:
				NameColor = NameTagColors::PlayerWhite;
				break;
			case ENameTagEntityType::Monster:
				NameColor = GetMonsterNameColor(LocalPlayerLevel, Entry.Level);
				break;
			case ENameTagEntityType::NPC:
				NameColor = NameTagColors::NPCLightBlue;
				break;
			}

			// Build display text
			FString DisplayText = Entry.DisplayName;
			if (Entry.Type == ENameTagEntityType::Monster && Entry.Level > 0)
			{
				DisplayText = FString::Printf(TEXT("%s Lv.%d"), *Entry.DisplayName, Entry.Level);
			}

			// Measure text for centering
			FVector2D TextSize = FontMeasure->Measure(DisplayText, NameFont);
			FVector2D DrawPos(SlatePos.X - TextSize.X * 0.5f, SlatePos.Y);

			// RO Classic vending sign: chat-bubble style board above the name tag
			// White text on dark brown/maroon background with gold border, like RO's vendor sign
			if (!Entry.VendingTitle.IsEmpty())
			{
				const FSlateFontInfo ShopFont = FCoreStyle::GetDefaultFontStyle("Bold", 9);
				FVector2D ShopSize = FontMeasure->Measure(Entry.VendingTitle, ShopFont);
				const float PadX = 8.f, PadY = 4.f;
				float SignW = ShopSize.X + PadX * 2.f;
				float SignH = ShopSize.Y + PadY * 2.f;
				float SignX = SlatePos.X - SignW * 0.5f;
				float SignY = DrawPos.Y - SignH - 4.f; // 4px gap above name

				// Gold border (1px)
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(
					FVector2f((float)(SignX - 1.f), (float)(SignY - 1.f)),
					FVector2f((float)(SignW + 2.f), (float)(SignH + 2.f))),
					FCoreStyle::Get().GetBrush("GenericWhiteBox"),
					ESlateDrawEffect::None,
					FLinearColor(0.72f, 0.58f, 0.28f, 1.f)); // Gold trim

				// Dark background
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(
					FVector2f((float)SignX, (float)SignY),
					FVector2f((float)SignW, (float)SignH)),
					FCoreStyle::Get().GetBrush("GenericWhiteBox"),
					ESlateDrawEffect::None,
					FLinearColor(0.15f, 0.08f, 0.04f, 0.92f)); // Dark brown

				// Shop title text (white/cream, centered)
				FVector2D TitlePos(SlatePos.X - ShopSize.X * 0.5f, SignY + PadY);
				DrawTextAt(OutDrawElements, LayerId, AllottedGeometry, TitlePos,
					Entry.VendingTitle, ShopFont,
					FLinearColor(1.f, 0.96f, 0.85f, 1.f)); // Cream white
			}

			// Draw 4-pass black outline (RO Classic style: ±1px cardinal offsets)
			DrawOutlinedText(OutDrawElements, LayerId, AllottedGeometry,
				DrawPos, DisplayText, NameFont, NameColor);

			// Draw sub-text below name (guild, party, etc.)
			if (!Entry.SubText.IsEmpty())
			{
				FVector2D SubSize = FontMeasure->Measure(Entry.SubText, SubFont);
				FVector2D SubPos(SlatePos.X - SubSize.X * 0.5f, DrawPos.Y + TextSize.Y + 1.f);
				DrawOutlinedText(OutDrawElements, LayerId, AllottedGeometry,
					SubPos, Entry.SubText, SubFont,
					FLinearColor(0.7f, 0.7f, 0.7f, 1.f));
			}
		}

		return LayerId;
	}

private:
	UNameTagSubsystem* Sub = nullptr;

	// Draw text with 4-pass black outline + colored center (RO Classic rendering)
	void DrawOutlinedText(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geo, const FVector2D& Pos, const FString& Text,
		const FSlateFontInfo& Font, const FLinearColor& Color) const
	{
		const float Offset = 1.f;
		const FLinearColor& Shadow = NameTagColors::OutlineBlack;

		// 4 cardinal shadow passes
		DrawTextAt(OutDrawElements, LayerId, Geo, FVector2D(Pos.X - Offset, Pos.Y), Text, Font, Shadow);
		DrawTextAt(OutDrawElements, LayerId, Geo, FVector2D(Pos.X + Offset, Pos.Y), Text, Font, Shadow);
		DrawTextAt(OutDrawElements, LayerId, Geo, FVector2D(Pos.X, Pos.Y - Offset), Text, Font, Shadow);
		DrawTextAt(OutDrawElements, LayerId, Geo, FVector2D(Pos.X, Pos.Y + Offset), Text, Font, Shadow);

		// Main colored text on top
		DrawTextAt(OutDrawElements, LayerId + 1, Geo, Pos, Text, Font, Color);
	}

	void DrawTextAt(FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FGeometry& Geo, const FVector2D& Pos, const FString& Text,
		const FSlateFontInfo& Font, const FLinearColor& Color) const
	{
		const FVector2f LocalSize(1000.f, 30.f); // Large enough to not clip
		const FSlateLayoutTransform LayoutTransform(FVector2f((float)Pos.X, (float)Pos.Y));
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			Geo.ToPaintGeometry(LocalSize, LayoutTransform),
			Text,
			Font,
			ESlateDrawEffect::None,
			Color
		);
	}
};

// ============================================================
// Lifecycle
// ============================================================

bool UNameTagSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UNameTagSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
	if (!GI || !GI->IsSocketConnected()) return;

	// Get local player level for monster color calculation
	FCharacterData SelChar = GI->GetSelectedCharacter();
	LocalPlayerLevel = FMath::Max(SelChar.Level, 1);

	ShowOverlay();

	UE_LOG(LogNameTag, Log, TEXT("NameTagSubsystem started — overlay at Z=7, localLevel=%d"), LocalPlayerLevel);
}

void UNameTagSubsystem::Deinitialize()
{
	HideOverlay();
	Entries.Empty();
	Super::Deinitialize();
}

// ============================================================
// Registration API
// ============================================================

void UNameTagSubsystem::RegisterEntity(AActor* Actor, const FString& Name,
	ENameTagEntityType Type, int32 Level, float VertOffset, float InSpriteHeight)
{
	if (!Actor) return;

	// Don't double-register
	if (FindEntry(Actor)) return;

	FNameTagEntry Entry;
	Entry.Actor = Actor;
	Entry.DisplayName = Name;
	Entry.Type = Type;
	Entry.Level = Level;
	Entry.VerticalOffset = VertOffset;
	Entry.SpriteHeight = InSpriteHeight;
	Entry.bVisible = true;
	Entries.Add(Entry);
}

void UNameTagSubsystem::UnregisterEntity(AActor* Actor)
{
	if (!Actor) return;
	Entries.RemoveAll([Actor](const FNameTagEntry& E) { return E.Actor.Get() == Actor; });
}

void UNameTagSubsystem::SetVisible(AActor* Actor, bool bVisible)
{
	if (FNameTagEntry* Entry = FindEntry(Actor))
	{
		Entry->bVisible = bVisible;
	}
}

void UNameTagSubsystem::UpdateName(AActor* Actor, const FString& NewName)
{
	if (FNameTagEntry* Entry = FindEntry(Actor))
	{
		Entry->DisplayName = NewName;
	}
}

void UNameTagSubsystem::SetVendingTitle(AActor* Actor, const FString& Title)
{
	if (FNameTagEntry* Entry = FindEntry(Actor))
	{
		Entry->VendingTitle = Title;
	}
}

FNameTagEntry* UNameTagSubsystem::FindEntry(AActor* Actor)
{
	for (FNameTagEntry& E : Entries)
	{
		if (E.Actor.Get() == Actor) return &E;
	}
	return nullptr;
}

// ============================================================
// Widget Lifecycle
// ============================================================

void UNameTagSubsystem::ShowOverlay()
{
	if (bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameViewportClient* Viewport = World->GetGameViewport();
	if (!Viewport) return;

	TSharedRef<SNameTagOverlay> Overlay = SNew(SNameTagOverlay).Subsystem(this);
	OverlayWidget = Overlay;

	ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(OverlayWidget);
	Viewport->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 7); // Z=7, below health bars (Z=8)
	bWidgetAdded = true;
}

void UNameTagSubsystem::HideOverlay()
{
	if (!bWidgetAdded) return;

	UWorld* World = GetWorld();
	if (World)
	{
		UGameViewportClient* Viewport = World->GetGameViewport();
		if (Viewport && ViewportOverlay.IsValid())
		{
			Viewport->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
		}
	}

	OverlayWidget.Reset();
	ViewportOverlay.Reset();
	bWidgetAdded = false;
}
