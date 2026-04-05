#include "SItemInspectWidget.h"
#include "ItemInspectSubsystem.h"
#include "ItemTooltipBuilder.h"
#include "InventorySubsystem.h"
#include "Engine/Engine.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

namespace InspectColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium   (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor LabelText     (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor ValueText     (0.85f, 0.80f, 0.70f, 1.f);
	static const FLinearColor SlotEmpty     (0.50f, 0.38f, 0.15f, 0.50f);
	static const FLinearColor SlotFilled    (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor CardNameText  (0.80f, 0.70f, 0.50f, 1.f);
	static const FLinearColor CloseBtn      (0.85f, 0.25f, 0.20f, 1.f);
	static const FLinearColor IconBg        (0.18f, 0.12f, 0.07f, 1.f);
}

void SItemInspectWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	[
		SNew(SBox)
		.WidthOverride(WidgetWidth)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InspectColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(InspectColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(InspectColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)

						// Title bar
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// Content area (icon left + description right)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
						[
							SAssignNew(ContentArea, SVerticalBox)
						]

						// Card slot footer (only populated when item has slots)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SAssignNew(CardSlotArea, SVerticalBox)
						]
					]
				]
			]
		]
	];

	SetVisibility(EVisibility::Collapsed);
}

TSharedRef<SWidget> SItemInspectWidget::BuildTitleBar()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InspectColors::PanelDark)
		.Padding(FMargin(6.f, 3.f))
		[
			SNew(SHorizontalBox)

			// Title text
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SAssignNew(TitleText, STextBlock)
				.Text(FText::GetEmpty())
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(InspectColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(InspectColors::TextShadow)
			]

			// Close [X] button
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[
				SNew(SBox).WidthOverride(18.f).HeightOverride(18.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(InspectColors::CloseBtn)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Cursor(EMouseCursor::Hand)
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
					{
						if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
						{
							if (UItemInspectSubsystem* Sub = OwningSubsystem.Get())
								Sub->HideInspect();
							return FReply::Handled();
						}
						return FReply::Unhandled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("X")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(InspectColors::TextBright))
					]
				]
			]
		];
}

void SItemInspectWidget::UpdateItem(const FInventoryItem& Item)
{
	CurrentItem = Item;

	// Update title (hide real name for unidentified items — RO Classic shows generic type)
	if (TitleText.IsValid())
	{
		if (!Item.bIdentified)
		{
			// Generic type name matching RO Classic
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
				else if (Item.EquipSlot == TEXT("head_top") || Item.EquipSlot == TEXT("head_mid") || Item.EquipSlot == TEXT("head_low")) GenericName = TEXT("Headgear");
				else if (Item.EquipSlot == TEXT("garment")) GenericName = TEXT("Garment");
				else if (Item.EquipSlot == TEXT("footgear")) GenericName = TEXT("Shoes");
				else if (Item.EquipSlot == TEXT("accessory")) GenericName = TEXT("Accessory");
				else if (Item.EquipSlot == TEXT("armor")) GenericName = TEXT("Armor");
				else GenericName = TEXT("Equipment");
			}
			TitleText->SetText(FText::FromString(GenericName));
		}
		else
		{
			TitleText->SetText(FText::FromString(Item.GetDisplayName()));
		}
	}

	RebuildContent();
}

void SItemInspectWidget::RebuildContent()
{
	if (!ContentArea.IsValid()) return;
	ContentArea->ClearChildren();
	CardSlotArea->ClearChildren();

	// Unidentified items: show only icon + "not identified" message, no stats/description/cards
	if (!CurrentItem.bIdentified)
	{
		ContentArea->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)

			// Icon area (left)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Top)
			[ BuildIconArea() ]

			// Unidentified message (right)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("This item has not been identified.")))
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f, 1.f)))
				.AutoWrapText(true)
			]
		];
		return;
	}

	// Main content: icon left + description right
	ContentArea->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)

		// Icon area (left)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0).VAlign(VAlign_Top)
		[ BuildIconArea() ]

		// Description area (right)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
		[
			SNew(SScrollBox)
			.ScrollBarVisibility(EVisibility::Collapsed)
			+ SScrollBox::Slot()
			[
				FormatFullDescription(
					CurrentItem.FullDescription.IsEmpty() ? CurrentItem.Description : CurrentItem.FullDescription
				)
			]
		]
	];

	// Card slot footer (only for equipment with slots > 0)
	if (CurrentItem.HasSlots())
	{
		CardSlotArea->AddSlot().AutoHeight()
		[ BuildCardSlotFooter() ];
	}
}

TSharedRef<SWidget> SItemInspectWidget::BuildIconArea()
{
	// Try to get the actual icon texture from InventorySubsystem's cache
	FSlateBrush* IconBrush = nullptr;
	if (!CurrentItem.Icon.IsEmpty())
	{
		if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
		{
			if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
			{
				IconBrush = InvSub->GetOrCreateItemIconBrush(CurrentItem.Icon);
			}
		}
	}

	// Icon fills the entire 128x128 area edge-to-edge at full quality
	TSharedRef<SWidget> IconContent = (IconBrush != nullptr)
		? StaticCastSharedRef<SWidget>(
			SNew(SBox)
			.WidthOverride(IconSize)
			.HeightOverride(IconSize)
			[
				SNew(SImage)
				.Image(IconBrush)
			]
		)
		: StaticCastSharedRef<SWidget>(
			// Fallback: show item type text if no icon texture
			SNew(SBox)
			.WidthOverride(IconSize)
			.HeightOverride(IconSize)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(InspectColors::IconBg)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(ItemTooltipBuilder::FormatItemType(CurrentItem)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(InspectColors::GoldDark))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]
			]
		);

	// Gold border around the icon
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InspectColors::GoldDark)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InspectColors::IconBg)
			.Padding(FMargin(0.f))
			[
				IconContent
			]
		];
}

TSharedRef<SWidget> SItemInspectWidget::FormatFullDescription(const FString& FullDesc)
{
	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);

	if (FullDesc.IsEmpty())
	{
		return VBox;
	}

	// Split by section divider (24+ underscores)
	TArray<FString> Sections;
	FString Divider = TEXT("________________________");

	// Manual split since ParseIntoArray doesn't handle multi-char delimiters cleanly
	FString Remaining = FullDesc;
	while (true)
	{
		int32 Idx = Remaining.Find(Divider);
		if (Idx == INDEX_NONE)
		{
			Sections.Add(Remaining);
			break;
		}
		Sections.Add(Remaining.Left(Idx));
		Remaining = Remaining.Mid(Idx + Divider.Len());
	}

	for (int32 i = 0; i < Sections.Num(); i++)
	{
		FString Section = Sections[i].TrimStartAndEnd();
		if (Section.IsEmpty()) continue;

		// Add gold divider between sections (not before first)
		if (i > 0)
		{
			VBox->AddSlot().AutoHeight().Padding(0, 3)
			[ BuildGoldDivider() ];
		}

		// Split section into lines
		TArray<FString> Lines;
		Section.ParseIntoArrayLines(Lines);

		for (const FString& Line : Lines)
		{
			FString Trimmed = Line.TrimStartAndEnd();
			if (Trimmed.IsEmpty()) continue;

			// Check for "Label : Value" or "Label: Value" pattern
			int32 ColonIdx = INDEX_NONE;
			Trimmed.FindChar(TEXT(':'), ColonIdx);

			if (ColonIdx > 0 && ColonIdx < Trimmed.Len() - 1)
			{
				FString Label = Trimmed.Left(ColonIdx).TrimEnd();
				FString Value = Trimmed.Mid(ColonIdx + 1).TrimStart();

				// Only treat as label:value if label is short (likely a field name)
				if (Label.Len() <= 30 && !Value.IsEmpty())
				{
					VBox->AddSlot().AutoHeight()
					[ BuildLabelValueRow(Label, Value) ];
					continue;
				}
			}

			// Plain text line
			VBox->AddSlot().AutoHeight()
			[ BuildPlainTextLine(Trimmed) ];
		}
	}

	return VBox;
}

TSharedRef<SWidget> SItemInspectWidget::BuildLabelValueRow(const FString& Label, const FString& Value)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label + TEXT(" : ")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(InspectColors::LabelText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(InspectColors::TextShadow)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(InspectColors::ValueText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(InspectColors::TextShadow)
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SItemInspectWidget::BuildPlainTextLine(const FString& Text)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Text))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		.ColorAndOpacity(FSlateColor(InspectColors::TextPrimary))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(InspectColors::TextShadow)
		.AutoWrapText(true);
}

TSharedRef<SWidget> SItemInspectWidget::BuildGoldDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InspectColors::GoldDivider)
		];
}

// ============================================================
// Card Slot Footer
// ============================================================

TSharedRef<SWidget> SItemInspectWidget::BuildCardSlotFooter()
{
	TSharedRef<SHorizontalBox> SlotRow = SNew(SHorizontalBox);

	for (int32 i = 0; i < CurrentItem.Slots; i++)
	{
		bool bFilled = (i < CurrentItem.CompoundedCardDetails.Num() && CurrentItem.CompoundedCardDetails[i].IsValid());

		SlotRow->AddSlot().AutoWidth().Padding(2, 0)
		[
			bFilled ? BuildFilledSlot(CurrentItem.CompoundedCardDetails[i]) : BuildEmptySlot()
		];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InspectColors::PanelMedium)
		.Padding(FMargin(6.f, 4.f))
		[
			SlotRow
		];
}

TSharedRef<SWidget> SItemInspectWidget::BuildEmptySlot()
{
	return SNew(SBox)
		.WidthOverride(24.f)
		.HeightOverride(24.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Chr(0x25C7)))  // ◇ diamond
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
			.ColorAndOpacity(FSlateColor(InspectColors::SlotEmpty))
		];
}

TSharedRef<SWidget> SItemInspectWidget::BuildFilledSlot(const FCompoundedCardInfo& Card)
{
	// Build a clickable card slot with tooltip and right-click inspect
	FCompoundedCardInfo CardCopy = Card;

	TSharedRef<SWidget> SlotWidget =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InspectColors::SlotFilled)
		.Padding(FMargin(3.f, 2.f))
		.Cursor(EMouseCursor::Hand)
		.ToolTip(
			SNew(SToolTip)
			[
				ItemTooltipBuilder::Build(FInventoryItem::FromCardInfo(Card))
			]
		)
		.OnMouseButtonDown_Lambda([this, CardCopy](const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::RightMouseButton)
			{
				// Open card inspect
				if (UItemInspectSubsystem* Sub = OwningSubsystem.Get())
				{
					Sub->ShowInspect(FInventoryItem::FromCardInfo(CardCopy));
				}
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
			SNew(STextBlock)
			.Text(FText::FromString(Card.Name))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(InspectColors::CardNameText))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(InspectColors::TextShadow)
		];

	return SlotWidget;
}

// ============================================================
// Drag + Mouse Interaction
// ============================================================

void SItemInspectWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}

FReply SItemInspectWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Title bar drag
		if (LocalPos.Y < TitleBarHeight)
		{
			bIsDragging = true;
			DragOffset = ScreenPos;
			DragStartWidgetPos = WidgetPosition;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}

	return FReply::Handled();
}

FReply SItemInspectWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Handled();
}

FReply SItemInspectWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
			? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
