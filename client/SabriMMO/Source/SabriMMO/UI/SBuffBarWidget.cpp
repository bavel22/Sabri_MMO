// SBuffBarWidget.cpp — Renders buff/status icons as a horizontal row.

#include "SBuffBarWidget.h"
#include "BuffBarSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

// ============================================================================
// RO Classic Color Palette (matching sabrimmo-ui skill)
// ============================================================================

namespace BuffBarColors
{
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 0.85f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 0.90f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark     (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor TextBright   (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow   (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor TimerText    (0.90f, 0.85f, 0.70f, 1.f);

	// Status effect colors
	static const FLinearColor StunYellow   (0.90f, 0.85f, 0.20f, 1.f);
	static const FLinearColor FreezeCyan   (0.30f, 0.85f, 0.95f, 1.f);
	static const FLinearColor StoneGray    (0.60f, 0.60f, 0.60f, 1.f);
	static const FLinearColor SleepPurple  (0.65f, 0.40f, 0.85f, 1.f);
	static const FLinearColor PoisonGreen  (0.30f, 0.80f, 0.30f, 1.f);
	static const FLinearColor BlindDkGray  (0.40f, 0.40f, 0.40f, 1.f);
	static const FLinearColor SilenceBlue  (0.35f, 0.55f, 0.90f, 1.f);
	static const FLinearColor ConfusePink  (0.90f, 0.50f, 0.70f, 1.f);
	static const FLinearColor BleedDkRed   (0.70f, 0.15f, 0.15f, 1.f);
	static const FLinearColor CurseDkPurp  (0.50f, 0.20f, 0.60f, 1.f);

	// Buff/debuff category colors
	static const FLinearColor BuffGreen    (0.25f, 0.75f, 0.30f, 1.f);
	static const FLinearColor DebuffRed    (0.85f, 0.25f, 0.20f, 1.f);
	static const FLinearColor DebuffOrange (0.90f, 0.55f, 0.15f, 1.f);
}

// ============================================================================
// Construction
// ============================================================================

void SBuffBarWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SNew(SBox)
		.Padding(FMargin(WidgetPosition.X, WidgetPosition.Y, 0, 0))
		[
			SAssignNew(IconRow, SHorizontalBox)
		]
	];
}

// ============================================================================
// Tick — rebuild icons when count changes, update timers
// ============================================================================

void SBuffBarWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UBuffBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	const int32 CurrentCount = Sub->ActiveStatuses.Num() + Sub->ActiveBuffs.Num();

	// Remove expired entries (client-side cleanup based on timer)
	for (int32 i = Sub->ActiveStatuses.Num() - 1; i >= 0; --i)
	{
		float Remaining = UBuffBarSubsystem::GetRemainingSeconds(
			Sub->ActiveStatuses[i].RemainingMs, Sub->ActiveStatuses[i].ReceivedAt);
		if (Remaining <= 0.0f)
		{
			Sub->ActiveStatuses.RemoveAt(i);
		}
	}
	for (int32 i = Sub->ActiveBuffs.Num() - 1; i >= 0; --i)
	{
		float Remaining = UBuffBarSubsystem::GetRemainingSeconds(
			Sub->ActiveBuffs[i].RemainingMs, Sub->ActiveBuffs[i].ReceivedAt);
		if (Remaining <= 0.0f)
		{
			Sub->ActiveBuffs.RemoveAt(i);
		}
	}

	const int32 CleanedCount = Sub->ActiveStatuses.Num() + Sub->ActiveBuffs.Num();

	// Rebuild if count changed
	if (CleanedCount != LastIconCount)
	{
		RebuildIcons();
		LastIconCount = CleanedCount;
	}
}

// ============================================================================
// Rebuild Icons
// ============================================================================

void SBuffBarWidget::RebuildIcons()
{
	if (!IconRow.IsValid()) return;
	IconRow->ClearChildren();

	UBuffBarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Status effects first (more urgent)
	for (int32 i = 0; i < Sub->ActiveStatuses.Num() && i < 20; ++i)
	{
		IconRow->AddSlot()
			.AutoWidth()
			.Padding(1.f, 0.f)
			[
				BuildSingleIcon(true, i)
			];
	}

	// Then buffs
	for (int32 i = 0; i < Sub->ActiveBuffs.Num() && (Sub->ActiveStatuses.Num() + i) < 20; ++i)
	{
		IconRow->AddSlot()
			.AutoWidth()
			.Padding(1.f, 0.f)
			[
				BuildSingleIcon(false, i)
			];
	}
}

// ============================================================================
// Build Single Icon
// ============================================================================

TSharedRef<SWidget> SBuffBarWidget::BuildSingleIcon(bool bIsStatus, int32 Index)
{
	UBuffBarSubsystem* Sub = OwningSubsystem.Get();

	// Determine border color, abbreviation text
	FLinearColor BorderColor;
	FString Abbrev;

	if (bIsStatus)
	{
		if (Sub && Index < Sub->ActiveStatuses.Num())
		{
			const FString& Type = Sub->ActiveStatuses[Index].Type;
			BorderColor = GetStatusColor(Type);
			Abbrev = GetStatusAbbrev(Type);
		}
	}
	else
	{
		if (Sub && Index < Sub->ActiveBuffs.Num())
		{
			const FString& Cat = Sub->ActiveBuffs[Index].Category;
			BorderColor = GetBuffCategoryColor(Cat);
			Abbrev = Sub->ActiveBuffs[Index].Abbrev;
			if (Abbrev.IsEmpty())
			{
				Abbrev = Sub->ActiveBuffs[Index].Name.ToUpper().Left(3);
			}
		}
	}

	// Capture for lambdas
	const bool IsStatus = bIsStatus;
	const int32 Idx = Index;
	TWeakObjectPtr<UBuffBarSubsystem> WeakSub = OwningSubsystem;

	return SNew(SBox)
		.WidthOverride(28.f)
		.HeightOverride(36.f)
		[
			// Gold border → dark background
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(BuffBarColors::GoldDark)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(BuffBarColors::PanelDark)
				.Padding(FMargin(0.f))
				[
					SNew(SVerticalBox)
					// Colored indicator bar at top
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).HeightOverride(3.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(FSlateColor(BorderColor))
						]
					]
					// Abbreviation text
					+ SVerticalBox::Slot().FillHeight(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Abbrev))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(BuffBarColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(BuffBarColors::TextShadow)
					]
					// Timer text
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 1)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakSub, IsStatus, Idx]() -> FText
						{
							UBuffBarSubsystem* S = WeakSub.Get();
							if (!S) return FText::GetEmpty();

							float RemSec = 0;
							if (IsStatus && Idx < S->ActiveStatuses.Num())
							{
								RemSec = UBuffBarSubsystem::GetRemainingSeconds(
									S->ActiveStatuses[Idx].RemainingMs,
									S->ActiveStatuses[Idx].ReceivedAt);
							}
							else if (!IsStatus && Idx < S->ActiveBuffs.Num())
							{
								RemSec = UBuffBarSubsystem::GetRemainingSeconds(
									S->ActiveBuffs[Idx].RemainingMs,
									S->ActiveBuffs[Idx].ReceivedAt);
							}

							if (RemSec <= 0) return FText::GetEmpty();
							if (RemSec >= 60)
								return FText::FromString(FString::Printf(TEXT("%dm"), FMath::FloorToInt32(RemSec / 60)));
							return FText::FromString(FString::Printf(TEXT("%ds"), FMath::CeilToInt32(RemSec)));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
						.ColorAndOpacity(FSlateColor(BuffBarColors::TimerText))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(BuffBarColors::TextShadow)
					]
				]
			]
		];
}

// ============================================================================
// Color/Abbreviation Lookups
// ============================================================================

FLinearColor SBuffBarWidget::GetStatusColor(const FString& Type)
{
	if (Type == TEXT("stun"))       return BuffBarColors::StunYellow;
	if (Type == TEXT("freeze"))     return BuffBarColors::FreezeCyan;
	if (Type == TEXT("stone"))      return BuffBarColors::StoneGray;
	if (Type == TEXT("sleep"))      return BuffBarColors::SleepPurple;
	if (Type == TEXT("poison"))     return BuffBarColors::PoisonGreen;
	if (Type == TEXT("blind"))      return BuffBarColors::BlindDkGray;
	if (Type == TEXT("silence"))    return BuffBarColors::SilenceBlue;
	if (Type == TEXT("confusion"))  return BuffBarColors::ConfusePink;
	if (Type == TEXT("bleeding"))   return BuffBarColors::BleedDkRed;
	if (Type == TEXT("curse"))      return BuffBarColors::CurseDkPurp;
	return BuffBarColors::DebuffRed;
}

FLinearColor SBuffBarWidget::GetBuffCategoryColor(const FString& Category)
{
	if (Category == TEXT("debuff")) return BuffBarColors::DebuffOrange;
	return BuffBarColors::BuffGreen;
}

FString SBuffBarWidget::GetStatusAbbrev(const FString& Type)
{
	if (Type == TEXT("stun"))       return TEXT("STN");
	if (Type == TEXT("freeze"))     return TEXT("FRZ");
	if (Type == TEXT("stone"))      return TEXT("PTR");
	if (Type == TEXT("sleep"))      return TEXT("SLP");
	if (Type == TEXT("poison"))     return TEXT("PSN");
	if (Type == TEXT("blind"))      return TEXT("BLD");
	if (Type == TEXT("silence"))    return TEXT("SIL");
	if (Type == TEXT("confusion"))  return TEXT("CNF");
	if (Type == TEXT("bleeding"))   return TEXT("BLE");
	if (Type == TEXT("curse"))      return TEXT("CRS");
	return Type.ToUpper().Left(3);
}
