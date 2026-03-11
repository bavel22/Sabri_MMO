// SSkillTooltipWidget.cpp — RO Classic skill tooltip with header, info, prereqs, description, level table

#include "SSkillTooltipWidget.h"
#include "SkillTreeSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

// ============================================================
// Tooltip Color Palette
// ============================================================
namespace TTColors
{
	static const FLinearColor PanelBg       (0.12f, 0.08f, 0.05f, 0.95f);
	static const FLinearColor GoldBorder    (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 0.6f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextDim       (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor TextGreen     (0.30f, 0.85f, 0.30f, 1.f);
	static const FLinearColor TextRed       (0.85f, 0.25f, 0.25f, 1.f);
	static const FLinearColor RowCurrentLv  (0.50f, 0.38f, 0.15f, 0.25f);  // gold tint
	static const FLinearColor RowNextLv     (0.22f, 0.50f, 0.22f, 0.20f);  // green tint
	static const FLinearColor RowNormal     (0.00f, 0.00f, 0.00f, 0.00f);  // transparent
	static const FLinearColor HeaderBg      (0.18f, 0.12f, 0.07f, 1.f);
}

// ============================================================
// Helpers
// ============================================================
static TSharedRef<SWidget> MakeTooltipDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(TTColors::GoldDivider)
		];
}

FString SSkillTooltipWidget::FormatTime(int32 TimeMs)
{
	if (TimeMs <= 0) return TEXT("0.0s");
	float Seconds = TimeMs / 1000.f;
	return FString::Printf(TEXT("%.1fs"), Seconds);
}

FString SSkillTooltipWidget::GetSkillTypeLabel(const FString& Type)
{
	if (Type == TEXT("active")) return TEXT("Active");
	if (Type == TEXT("passive")) return TEXT("Passive");
	if (Type == TEXT("toggle")) return TEXT("Toggle");
	return Type;
}

FString SSkillTooltipWidget::GetTargetLabel(const FString& TargetType)
{
	if (TargetType == TEXT("single")) return TEXT("Single Target");
	if (TargetType == TEXT("self")) return TEXT("Self");
	if (TargetType == TEXT("ground")) return TEXT("Ground Area");
	if (TargetType == TEXT("aoe")) return TEXT("Area of Effect");
	if (TargetType == TEXT("none")) return TEXT("None");
	return TargetType;
}

FLinearColor SSkillTooltipWidget::GetElementColor(const FString& Element)
{
	if (Element == TEXT("fire"))    return FLinearColor(1.0f, 0.4f, 0.2f, 1.f);
	if (Element == TEXT("water"))   return FLinearColor(0.3f, 0.6f, 1.0f, 1.f);
	if (Element == TEXT("wind"))    return FLinearColor(0.6f, 0.9f, 0.3f, 1.f);
	if (Element == TEXT("earth"))   return FLinearColor(0.7f, 0.5f, 0.3f, 1.f);
	if (Element == TEXT("holy"))    return FLinearColor(1.0f, 1.0f, 0.7f, 1.f);
	if (Element == TEXT("ghost"))   return FLinearColor(0.7f, 0.5f, 0.9f, 1.f);
	if (Element == TEXT("poison"))  return FLinearColor(0.6f, 0.3f, 0.7f, 1.f);
	if (Element == TEXT("undead"))  return FLinearColor(0.5f, 0.4f, 0.5f, 1.f);
	return TTColors::TextDim; // neutral
}

// ============================================================
// Construct
// ============================================================
void SSkillTooltipWidget::Construct(const FArguments& InArgs)
{
	const FSkillEntry* SkillData = InArgs._SkillData;
	USkillTreeSubsystem* Sub = InArgs._Subsystem;

	if (!SkillData)
	{
		ChildSlot [ SNullWidget::NullWidget ];
		return;
	}

	const FSkillEntry& Skill = *SkillData;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(320.f)
		[
			// Gold border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(TTColors::GoldBorder)
			.Padding(FMargin(1.f))
			[
				// Dark panel
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(TTColors::PanelBg)
				.Padding(FMargin(6.f, 4.f))
				[
					SNew(SVerticalBox)

					// Header (icon + name + max level)
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildHeader(Skill, Sub) ]

					+ SVerticalBox::Slot().AutoHeight()
					[ MakeTooltipDivider() ]

					// Skill info (type, element, target, range)
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildSkillInfo(Skill) ]

					+ SVerticalBox::Slot().AutoHeight()
					[ MakeTooltipDivider() ]

					// Prerequisites
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildPrerequisites(Skill, Sub) ]

					+ SVerticalBox::Slot().AutoHeight()
					[ MakeTooltipDivider() ]

					// Description
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildDescription(Skill) ]

					+ SVerticalBox::Slot().AutoHeight()
					[ MakeTooltipDivider() ]

					// Level table
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildLevelTable(Skill) ]
				]
			]
		]
	];
}

// ============================================================
// BuildHeader
// ============================================================
TSharedRef<SWidget> SSkillTooltipWidget::BuildHeader(const FSkillEntry& Skill, USkillTreeSubsystem* Sub)
{
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox);

	// Icon
	FSlateBrush* IconBrush = Sub ? Sub->GetOrCreateIconBrush(Skill.IconPath) : nullptr;
	if (IconBrush)
	{
		Header->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(28.f)
			.HeightOverride(28.f)
			[
				SNew(SImage).Image(IconBrush)
			]
		];
	}

	// Name + level
	Header->AddSlot()
	.FillWidth(1.f)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(Skill.DisplayName))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.8f))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Lv %d / %d"), Skill.CurrentLevel, Skill.MaxLevel)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(
				Skill.CurrentLevel >= Skill.MaxLevel ? TTColors::GoldHighlight :
				Skill.CurrentLevel > 0 ? TTColors::TextGreen : TTColors::TextDim))
		]
	];

	return Header;
}

// ============================================================
// BuildSkillInfo
// ============================================================
TSharedRef<SWidget> SSkillTooltipWidget::BuildSkillInfo(const FSkillEntry& Skill)
{
	TSharedRef<SVerticalBox> Info = SNew(SVerticalBox);

	// Type
	Info->AddSlot().AutoHeight().Padding(0, 1)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Type: ")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextDim))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(GetSkillTypeLabel(Skill.Type)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
		]
	];

	// Element (only if not neutral)
	if (!Skill.Element.IsEmpty() && Skill.Element != TEXT("neutral"))
	{
		FString ElemDisplay = Skill.Element;
		if (ElemDisplay.Len() > 0) ElemDisplay[0] = FChar::ToUpper(ElemDisplay[0]);

		Info->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Element: ")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(TTColors::TextDim))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(ElemDisplay))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
				.ColorAndOpacity(FSlateColor(GetElementColor(Skill.Element)))
			]
		];
	}

	// Target
	Info->AddSlot().AutoHeight().Padding(0, 1)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Target: ")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextDim))
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(STextBlock)
			.Text(FText::FromString(GetTargetLabel(Skill.TargetType)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
		]
	];

	// Range (only if > 0)
	if (Skill.Range > 0)
	{
		Info->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Range: ")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(TTColors::TextDim))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), Skill.Range)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
				.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
			]
		];
	}

	return Info;
}

// ============================================================
// BuildPrerequisites
// ============================================================
TSharedRef<SWidget> SSkillTooltipWidget::BuildPrerequisites(const FSkillEntry& Skill, USkillTreeSubsystem* Sub)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	Box->AddSlot().AutoHeight().Padding(0, 1)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Prerequisites")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
		.ColorAndOpacity(FSlateColor(TTColors::TextDim))
	];

	if (Skill.Prerequisites.Num() == 0)
	{
		Box->AddSlot().AutoHeight().Padding(8, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("(none)")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextDim))
		];
	}
	else
	{
		for (const FSkillPrerequisite& Prereq : Skill.Prerequisites)
		{
			// Lookup prerequisite skill name and current level
			FString PrereqName = FString::Printf(TEXT("Skill #%d"), Prereq.RequiredSkillId);
			int32 CurrentPrereqLevel = 0;

			if (Sub)
			{
				const FSkillEntry* PrereqSkill = Sub->FindSkillEntry(Prereq.RequiredSkillId);
				if (PrereqSkill)
				{
					PrereqName = PrereqSkill->DisplayName;
				}
				const int32* Lvl = Sub->LearnedSkills.Find(Prereq.RequiredSkillId);
				if (Lvl) CurrentPrereqLevel = *Lvl;
			}

			const bool bMet = CurrentPrereqLevel >= Prereq.RequiredLevel;
			FString PrereqText = FString::Printf(TEXT("%s Lv %d (%d/%d)"),
				*PrereqName, Prereq.RequiredLevel, CurrentPrereqLevel, Prereq.RequiredLevel);

			Box->AddSlot().AutoHeight().Padding(8, 1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(PrereqText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(bMet ? TTColors::TextGreen : TTColors::TextRed))
			];
		}
	}

	return Box;
}

// ============================================================
// BuildDescription
// ============================================================
TSharedRef<SWidget> SSkillTooltipWidget::BuildDescription(const FSkillEntry& Skill)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Skill.Description))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
		.AutoWrapText(true)
		.WrapTextAt(300.f);
}

// ============================================================
// BuildLevelTable
// ============================================================
TSharedRef<SWidget> SSkillTooltipWidget::BuildLevelTable(const FSkillEntry& Skill)
{
	if (Skill.AllLevels.Num() == 0)
	{
		return SNew(STextBlock)
			.Text(FText::FromString(TEXT("No detailed level data available.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
			.ColorAndOpacity(FSlateColor(TTColors::TextDim));
	}

	// Determine which columns to show
	bool bShowCast = false, bShowCooldown = false, bShowDuration = false;
	for (const FSkillLevelInfo& L : Skill.AllLevels)
	{
		if (L.CastTime > 0) bShowCast = true;
		if (L.Cooldown > 0) bShowCooldown = true;
		if (L.Duration > 0) bShowDuration = true;
	}

	TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);

	// Header label
	Table->AddSlot().AutoHeight().Padding(0, 1)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Level Details")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
		.ColorAndOpacity(FSlateColor(TTColors::TextDim))
	];

	// Column header row
	{
		TSharedRef<SHorizontalBox> HeaderRow = SNew(SHorizontalBox);

		HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(22.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Lv")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
				.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
			]
		];

		HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(30.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("SP")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
				.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
			]
		];

		if (bShowCast)
		{
			HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Cast")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
					.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
				]
			];
		}

		if (bShowCooldown)
		{
			HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CD")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
					.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
				]
			];
		}

		HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(40.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Effect")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
				.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
			]
		];

		if (bShowDuration)
		{
			HeaderRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Dur")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
					.ColorAndOpacity(FSlateColor(TTColors::GoldHighlight))
				]
			];
		}

		Table->AddSlot().AutoHeight().Padding(0, 2, 0, 1)
		[ HeaderRow ];
	}

	// Data rows
	for (const FSkillLevelInfo& L : Skill.AllLevels)
	{
		// Row highlight: gold for current, green for next, transparent otherwise
		FLinearColor RowBg = TTColors::RowNormal;
		if (L.Level == Skill.CurrentLevel && Skill.CurrentLevel > 0)
			RowBg = TTColors::RowCurrentLv;
		else if (L.Level == Skill.CurrentLevel + 1 && Skill.CurrentLevel < Skill.MaxLevel)
			RowBg = TTColors::RowNextLv;

		TSharedRef<SHorizontalBox> DataRow = SNew(SHorizontalBox);

		// Level number
		DataRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(22.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), L.Level)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
				.ColorAndOpacity(FSlateColor(TTColors::TextBright))
			]
		];

		// SP
		DataRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(30.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), L.SpCost)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
				.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
			]
		];

		// Cast Time
		if (bShowCast)
		{
			DataRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FormatTime(L.CastTime)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
					.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
				]
			];
		}

		// Cooldown
		if (bShowCooldown)
		{
			DataRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FormatTime(L.Cooldown)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
					.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
				]
			];
		}

		// Effect Value
		DataRow->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(SBox).WidthOverride(40.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), L.EffectValue)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
				.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
			]
		];

		// Duration
		if (bShowDuration)
		{
			DataRow->AddSlot().AutoWidth().Padding(1, 0)
			[
				SNew(SBox).WidthOverride(38.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(L.Duration > 0 ? FormatTime(L.Duration) : TEXT("-")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
					.ColorAndOpacity(FSlateColor(TTColors::TextPrimary))
				]
			];
		}

		Table->AddSlot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(RowBg)
			.Padding(FMargin(2.f, 1.f))
			[
				DataRow
			]
		];
	}

	return Table;
}
