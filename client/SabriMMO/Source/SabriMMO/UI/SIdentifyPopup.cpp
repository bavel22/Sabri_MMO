// SIdentifyPopup.cpp — Item Appraisal popup: shows unidentified items when
// using Magnifying Glass or Identify skill. Click an item to identify it.

#include "SIdentifyPopup.h"
#include "InventorySubsystem.h"
#include "MMOGameInstance.h"
#include "Dom/JsonObject.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

// ============================================================
// Colors (reuse RO palette)
// ============================================================

namespace IdentifyColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextDim       (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor RowBg         (0.18f, 0.12f, 0.07f, 1.f);
	static const FLinearColor RowHover      (0.30f, 0.22f, 0.12f, 1.f);
	static const FLinearColor ErrorRed      (0.90f, 0.25f, 0.20f, 1.f);
	static const FLinearColor Backdrop      (0.00f, 0.00f, 0.00f, 0.40f);
}

static constexpr float IdentifyPopupWidth = 280.f;
static constexpr float IdentifyRowIconSize = 24.f;
static constexpr float IdentifyMaxListHeight = 300.f;

// ============================================================
// Construction
// ============================================================

void SIdentifyPopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	PendingItems = InArgs._UnidentifiedItems;
	bIsMagnifier = InArgs._IsMagnifier;

	ChildSlot
	[
		// Fullscreen transparent backdrop — click anywhere outside to dismiss
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(IdentifyColors::Backdrop)
		.Visibility(EVisibility::Visible)
		.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				DismissPopup();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			// The popup itself — stops click propagation to backdrop
			SNew(SBox)
			.WidthOverride(IdentifyPopupWidth)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(IdentifyColors::Backdrop)  // prevent backdrop click-through
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) -> FReply
				{
					return FReply::Handled();  // Block propagation to backdrop
				})
				[
					// 3-layer frame: Gold -> Dark -> Brown
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(IdentifyColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(IdentifyColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(IdentifyColors::PanelBrown)
							.Padding(FMargin(4.f))
							[
								SNew(SVerticalBox)

								// Title bar
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildTitleBar() ]

								// Gold divider
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 3)
								[
									SNew(SBox).HeightOverride(1.f)
									[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(IdentifyColors::GoldDivider) ]
								]

								// Subtitle
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SNew(STextBlock)
									.Text(FText::FromString(bIsMagnifier
										? TEXT("Select item to identify: (1 item)")
										: TEXT("Select item to identify:")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(IdentifyColors::TextDim))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(IdentifyColors::TextShadow)
								]

								// Item list (scrollable)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.MaxDesiredHeight(IdentifyMaxListHeight)
									[
										SAssignNew(ItemListContainer, SVerticalBox)
									]
								]

								// Status message (hidden by default)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SAssignNew(StatusTextBlock, STextBlock)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(IdentifyColors::ErrorRed))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(IdentifyColors::TextShadow)
									.AutoWrapText(true)
									.Visibility(EVisibility::Collapsed)
								]
							]
						]
					]
				]
			]
		]
	];

	// Build the initial item list inside the container
	RebuildItemList();
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SIdentifyPopup::BuildTitleBar()
{
	FString TitleText = bIsMagnifier ? TEXT("Magnifier") : TEXT("Item Appraisal");

	return SNew(SHorizontalBox)
		// Title text
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TitleText))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(IdentifyColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(IdentifyColors::TextShadow)
		]
		// Close button
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
			.Cursor(EMouseCursor::Hand)
			.Padding(FMargin(4.f, 0.f))
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					DismissPopup();
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("X")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(IdentifyColors::TextDim))
			]
		];
}

// ============================================================
// Item list
// ============================================================

void SIdentifyPopup::RebuildItemList()
{
	if (!ItemListContainer.IsValid()) return;

	ItemListContainer->ClearChildren();

	if (PendingItems.Num() == 0)
	{
		ItemListContainer->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("No unidentified items.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(IdentifyColors::TextDim))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(IdentifyColors::TextShadow)
		];
		return;
	}

	// Wrap list in a scroll box
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible);

	for (const FInventoryItem& Item : PendingItems)
	{
		ScrollBox->AddSlot().Padding(0, 1)
		[
			BuildItemRow(Item)
		];
	}

	ItemListContainer->AddSlot().AutoHeight()
	[
		ScrollBox
	];
}

TSharedRef<SWidget> SIdentifyPopup::BuildItemRow(const FInventoryItem& Item)
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	int32 ItemInvId = Item.InventoryId;

	// Build icon
	TSharedRef<SWidget> ItemIcon = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = Sub ? Sub->GetOrCreateItemIconBrush(Item.Icon) : nullptr;
		if (Brush)
		{
			return SNew(SBox).WidthOverride(IdentifyRowIconSize).HeightOverride(IdentifyRowIconSize)
			[
				SNew(SImage).Image(Brush)
			];
		}
		return SNew(SBox).WidthOverride(IdentifyRowIconSize).HeightOverride(IdentifyRowIconSize)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(IdentifyColors::GoldDark)
		];
	}();

	// Generic type name for unidentified display (RO Classic hides real name)
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

	// Dimmed icon tint for unidentified appearance
	FLinearColor IconTint(0.5f, 0.5f, 0.5f, 1.f);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(IdentifyColors::RowBg)
		.Padding(FMargin(4.f, 3.f))
		.Cursor(EMouseCursor::Hand)
		.OnMouseButtonDown_Lambda([this, ItemInvId](const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				OnItemClicked(ItemInvId);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
			SNew(SHorizontalBox)
			// Icon (dimmed)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
			[ ItemIcon ]
			// Generic type name
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(GenericName))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.55f, 0.40f, 1.f)))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(IdentifyColors::TextShadow)
			]
			// "?" badge
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("?")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f, 1.f)))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(IdentifyColors::TextShadow)
			]
		];
}

// ============================================================
// Actions
// ============================================================

void SIdentifyPopup::OnItemClicked(int32 InventoryId)
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	UMMOGameInstance* GI = Cast<UMMOGameInstance>(Sub->GetWorld()->GetGameInstance());
	if (!GI) return;

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("inventoryId"), InventoryId);
	GI->EmitSocketEvent(TEXT("identify:select"), Payload);
}

void SIdentifyPopup::RemoveItem(int32 InventoryId)
{
	PendingItems.RemoveAll([InventoryId](const FInventoryItem& Item)
	{
		return Item.InventoryId == InventoryId;
	});
	RebuildItemList();
}

void SIdentifyPopup::DismissPopup()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		Sub->HideIdentifyPopup();
	}
}

// ============================================================
// Status message
// ============================================================

void SIdentifyPopup::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(Message));
		StatusTextBlock->SetColorAndOpacity(FSlateColor(
			bIsError ? IdentifyColors::ErrorRed : IdentifyColors::TextPrimary));
		StatusTextBlock->SetVisibility(
			Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible);
	}
}

// ============================================================
// Input
// ============================================================

FReply SIdentifyPopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		DismissPopup();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
