#include "ItemTooltipBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

namespace TooltipColors
{
	static const FLinearColor TooltipBg      (0.15f, 0.10f, 0.06f, 0.95f);
	static const FLinearColor GoldDark       (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight  (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider    (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary    (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextDim        (0.65f, 0.55f, 0.40f, 1.f);
}

FString ItemTooltipBuilder::FormatItemType(const FInventoryItem& Item)
{
	// Map weapon subtypes to display names
	if (Item.ItemType == TEXT("weapon"))
	{
		if (Item.WeaponType == TEXT("dagger")) return TEXT("Dagger");
		if (Item.WeaponType == TEXT("one_hand_sword")) return Item.bTwoHanded ? TEXT("Two-Handed Sword") : TEXT("One-Handed Sword");
		if (Item.WeaponType == TEXT("two_hand_sword")) return TEXT("Two-Handed Sword");
		if (Item.WeaponType == TEXT("spear")) return Item.bTwoHanded ? TEXT("Two-Handed Spear") : TEXT("One-Handed Spear");
		if (Item.WeaponType == TEXT("axe")) return Item.bTwoHanded ? TEXT("Two-Handed Axe") : TEXT("One-Handed Axe");
		if (Item.WeaponType == TEXT("mace")) return TEXT("Mace");
		if (Item.WeaponType == TEXT("staff")) return Item.bTwoHanded ? TEXT("Two-Handed Staff") : TEXT("Rod");
		if (Item.WeaponType == TEXT("bow")) return TEXT("Bow");
		if (Item.WeaponType == TEXT("knuckle")) return TEXT("Knuckle");
		if (Item.WeaponType == TEXT("instrument")) return TEXT("Instrument");
		if (Item.WeaponType == TEXT("whip")) return TEXT("Whip");
		if (Item.WeaponType == TEXT("book")) return TEXT("Book");
		if (Item.WeaponType == TEXT("katar")) return TEXT("Katar");
		if (Item.WeaponType == TEXT("gun")) return TEXT("Gun");
		return TEXT("Weapon");
	}

	// Map equip slots to display names for armor types
	if (Item.ItemType == TEXT("armor"))
	{
		if (Item.EquipSlot == TEXT("armor")) return TEXT("Body Armor");
		if (Item.EquipSlot == TEXT("shield")) return TEXT("Shield");
		if (Item.EquipSlot == TEXT("head_top")) return TEXT("Headgear (Upper)");
		if (Item.EquipSlot == TEXT("head_mid")) return TEXT("Headgear (Mid)");
		if (Item.EquipSlot == TEXT("head_low")) return TEXT("Headgear (Lower)");
		if (Item.EquipSlot == TEXT("garment")) return TEXT("Garment");
		if (Item.EquipSlot == TEXT("footgear")) return TEXT("Footgear");
		if (Item.EquipSlot == TEXT("accessory")) return TEXT("Accessory");
		return TEXT("Armor");
	}

	if (Item.ItemType == TEXT("card")) return TEXT("Card");
	if (Item.ItemType == TEXT("consumable")) return TEXT("Consumable");
	if (Item.ItemType == TEXT("ammo")) return TEXT("Ammunition");
	if (Item.ItemType == TEXT("etc")) return TEXT("Misc");

	return Item.ItemType;
}

TSharedRef<SWidget> ItemTooltipBuilder::Build(const FInventoryItem& Item)
{
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	// Unidentified items: show generic type name + "Unidentified" (RO Classic hides real name)
	if (!Item.bIdentified)
	{
		// Map equip slot / weapon type to RO Classic generic unidentified display name
		FString GenericName;
		if (Item.ItemType == TEXT("weapon"))
		{
			if (Item.WeaponType == TEXT("dagger")) GenericName = TEXT("Dagger");
			else if (Item.WeaponType == TEXT("sword") || Item.WeaponType == TEXT("1hsword")) GenericName = TEXT("Sword");
			else if (Item.WeaponType == TEXT("2hsword")) GenericName = TEXT("Two-Handed Sword");
			else if (Item.WeaponType == TEXT("spear") || Item.WeaponType == TEXT("1hspear")) GenericName = TEXT("Spear");
			else if (Item.WeaponType == TEXT("2hspear")) GenericName = TEXT("Two-Handed Spear");
			else if (Item.WeaponType == TEXT("axe") || Item.WeaponType == TEXT("1haxe")) GenericName = TEXT("Axe");
			else if (Item.WeaponType == TEXT("2haxe")) GenericName = TEXT("Two-Handed Axe");
			else if (Item.WeaponType == TEXT("mace")) GenericName = TEXT("Mace");
			else if (Item.WeaponType == TEXT("rod") || Item.WeaponType == TEXT("staff")) GenericName = TEXT("Rod");
			else if (Item.WeaponType == TEXT("bow")) GenericName = TEXT("Bow");
			else if (Item.WeaponType == TEXT("katar")) GenericName = TEXT("Katar");
			else if (Item.WeaponType == TEXT("knuckle") || Item.WeaponType == TEXT("fist")) GenericName = TEXT("Knuckle");
			else if (Item.WeaponType == TEXT("instrument") || Item.WeaponType == TEXT("musical")) GenericName = TEXT("Instrument");
			else if (Item.WeaponType == TEXT("whip")) GenericName = TEXT("Whip");
			else if (Item.WeaponType == TEXT("book")) GenericName = TEXT("Book");
			else GenericName = TEXT("Weapon");
		}
		else
		{
			if (Item.EquipSlot == TEXT("shield")) GenericName = TEXT("Shield");
			else if (Item.EquipSlot == TEXT("head_top")) GenericName = TEXT("Headgear");
			else if (Item.EquipSlot == TEXT("head_mid")) GenericName = TEXT("Headgear");
			else if (Item.EquipSlot == TEXT("head_low")) GenericName = TEXT("Headgear");
			else if (Item.EquipSlot == TEXT("garment")) GenericName = TEXT("Garment");
			else if (Item.EquipSlot == TEXT("footgear")) GenericName = TEXT("Shoes");
			else if (Item.EquipSlot == TEXT("accessory")) GenericName = TEXT("Accessory");
			else if (Item.EquipSlot == TEXT("armor")) GenericName = TEXT("Armor");
			else GenericName = TEXT("Equipment");
		}

		Content->AddSlot().AutoHeight().Padding(4, 4, 4, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(GenericName))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.55f, 0.40f, 1.f)))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
		];
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Unidentified")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f, 1.f)))
		];
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.22f, 0.14f, 0.08f, 0.95f))
			.Padding(FMargin(2.f))
			[
				Content
			];
	}

	// Name (formatted with refine, prefix, suffix, slots)
	Content->AddSlot().AutoHeight().Padding(4, 4, 4, 2)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item.GetDisplayName()))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(TooltipColors::GoldHighlight))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.85f))
	];

	// Type
	Content->AddSlot().AutoHeight().Padding(4, 0, 4, 2)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FormatItemType(Item)))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		.ColorAndOpacity(FSlateColor(TooltipColors::TextDim))
	];

	// Description (short)
	if (!Item.Description.IsEmpty())
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 2)
		[
			SNew(SBox).WidthOverride(200.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Description))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(TooltipColors::TextPrimary))
				.AutoWrapText(true)
			]
		];
	}

	// Divider
	Content->AddSlot().AutoHeight().Padding(4, 2)
	[
		SNew(SBox).HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(TooltipColors::GoldDivider)
		]
	];

	// Key stats (only show non-zero)
	auto AddStatLine = [&Content](const FString& Label, int32 Value)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s: %d"), *Label, Value)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TooltipColors::TextPrimary))
		];
	};

	auto AddBonusLine = [&Content](const FString& Label, int32 Value)
	{
		FString Sign = Value >= 0 ? TEXT("+ ") : TEXT("- ");
		int32 AbsVal = FMath::Abs(Value);
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s %s%d"), *Label, *Sign, AbsVal)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(Value >= 0 ? TooltipColors::TextPrimary : FLinearColor(0.9f, 0.3f, 0.3f, 1.f)))
		];
	};

	if (Item.ATK > 0) AddStatLine(TEXT("ATK"), Item.ATK);
	if (Item.MATK > 0) AddStatLine(TEXT("MATK"), Item.MATK);
	if (Item.DEF > 0) AddStatLine(TEXT("DEF"), Item.DEF);
	if (Item.MDEF > 0) AddStatLine(TEXT("MDEF"), Item.MDEF);
	if (Item.StrBonus != 0) AddBonusLine(TEXT("STR"), Item.StrBonus);
	if (Item.AgiBonus != 0) AddBonusLine(TEXT("AGI"), Item.AgiBonus);
	if (Item.VitBonus != 0) AddBonusLine(TEXT("VIT"), Item.VitBonus);
	if (Item.IntBonus != 0) AddBonusLine(TEXT("INT"), Item.IntBonus);
	if (Item.DexBonus != 0) AddBonusLine(TEXT("DEX"), Item.DexBonus);
	if (Item.LukBonus != 0) AddBonusLine(TEXT("LUK"), Item.LukBonus);
	if (Item.MaxHPBonus != 0) AddBonusLine(TEXT("Max HP"), Item.MaxHPBonus);
	if (Item.MaxSPBonus != 0) AddBonusLine(TEXT("Max SP"), Item.MaxSPBonus);
	if (Item.HitBonus != 0) AddBonusLine(TEXT("HIT"), Item.HitBonus);
	if (Item.FleeBonus != 0) AddBonusLine(TEXT("FLEE"), Item.FleeBonus);
	if (Item.CriticalBonus != 0) AddBonusLine(TEXT("Critical"), Item.CriticalBonus);
	if (Item.PerfectDodgeBonus != 0) AddBonusLine(TEXT("P.Dodge"), Item.PerfectDodgeBonus);
	if (Item.Weight > 0) AddStatLine(TEXT("Weight"), Item.Weight);
	if (Item.RequiredLevel > 1)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Required Lv: %d"), Item.RequiredLevel)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TooltipColors::TextDim))
		];
	}

	// Wrapped in gold border frame
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(TooltipColors::GoldDark)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(TooltipColors::TooltipBg)
			[
				Content
			]
		];
}
