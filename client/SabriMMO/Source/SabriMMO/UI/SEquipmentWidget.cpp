// SEquipmentWidget.cpp — RO Classic equipment window with 10 equipment slots
// arranged around a character portrait, with drag-and-drop support.

#include "SEquipmentWidget.h"
#include "EquipmentSubsystem.h"
#include "InventorySubsystem.h"
#include "ItemInspectSubsystem.h"
#include "ItemTooltipBuilder.h"
#include "Engine/Engine.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Engine/World.h"

// ============================================================
// Colors
// ============================================================

namespace EqColors
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
	static const FLinearColor TextDim       (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor SlotBg        (0.15f, 0.10f, 0.06f, 1.f);
	static const FLinearColor SlotBorder    (0.40f, 0.30f, 0.15f, 1.f);
	static const FLinearColor SlotHighlight (0.60f, 0.50f, 0.25f, 1.f);
	static const FLinearColor PortraitBg    (0.18f, 0.12f, 0.07f, 1.f);
	static const FLinearColor PortraitBorder(0.45f, 0.35f, 0.18f, 1.f);
	static const FLinearColor TooltipBg     (0.12f, 0.08f, 0.05f, 0.95f);
	// Item type colors
	static const FLinearColor WeaponOrange  (0.75f, 0.50f, 0.15f, 1.f);
	static const FLinearColor ArmorBlue     (0.20f, 0.35f, 0.65f, 1.f);
	static const FLinearColor EtcGreen      (0.25f, 0.55f, 0.25f, 1.f);
}

// ============================================================
// Construction
// ============================================================

void SEquipmentWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(CurrentSize.X)
		.HeightOverride(CurrentSize.Y)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(EqColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(EqColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(EqColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(EqColors::GoldDivider) ]
						]
						+ SVerticalBox::Slot().FillHeight(1.f).Padding(FMargin(4.f))
						[
							SAssignNew(EquipmentContainer, SBox)
							[ BuildEquipmentLayout() ]
						]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Tick — rebuild equipment slots when inventory data changes
// ============================================================

void SEquipmentWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UInventorySubsystem* InvSub = GetInventorySubsystem();
	if (!InvSub) return;

	uint32 CurrentVersion = InvSub->DataVersion;
	if (CurrentVersion != LastDataVersion)
	{
		LastDataVersion = CurrentVersion;
		RebuildEquipmentSlots();
	}

	// Update drag cursor position to follow mouse
	if (InvSub->bIsDragging)
	{
		InvSub->UpdateDragCursorPosition();
	}
}

void SEquipmentWidget::RebuildEquipmentSlots()
{
	if (!EquipmentContainer.IsValid()) return;
	EquipmentContainer->SetContent(BuildEquipmentLayout());
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SEquipmentWidget::BuildTitleBar()
{
	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Equipment")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(EqColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(EqColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("X")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(EqColors::TextDim))
			]
		];
}

// ============================================================
// Equipment slot builder
// ============================================================

TSharedRef<SWidget> SEquipmentWidget::BuildEquipSlot(const FString& SlotPosition)
{
	UEquipmentSubsystem* Sub = OwningSubsystem.Get();
	// Capture by value for lambda safety
	FString SlotPos = SlotPosition;

	// Pre-build icon widget: texture icon if equipped item has one, colored placeholder otherwise
	TSharedRef<SWidget> EquipIconWidget = [&]() -> TSharedRef<SWidget>
	{
		if (Sub)
		{
			FInventoryItem Item = Sub->GetEquippedItem(SlotPos);
			if (Item.IsValid())
			{
				UInventorySubsystem* InvSub = GetInventorySubsystem();
				FSlateBrush* Brush = InvSub ? InvSub->GetOrCreateItemIconBrush(Item.Icon) : nullptr;
				if (Brush)
				{
					return SNew(SImage).Image(Brush);
				}
			}
		}
		// Fallback: colored square + 2-letter text (lambda-driven for dynamic updates)
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, Sub, SlotPos]() -> FSlateColor {
				if (!Sub) return FSlateColor(FLinearColor(0.2f, 0.15f, 0.08f, 0.5f));
				FInventoryItem It = Sub->GetEquippedItem(SlotPos);
				if (It.IsValid())
					return FSlateColor(GetItemTypeColor(It.ItemType));
				return FSlateColor(FLinearColor(0.2f, 0.15f, 0.08f, 0.5f));
			})
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub, SlotPos]() -> FText {
					if (!Sub) return FText::GetEmpty();
					FInventoryItem It = Sub->GetEquippedItem(SlotPos);
					if (It.IsValid() && It.Name.Len() > 0)
						return FText::FromString(It.Name.Left(2));
					return FText::GetEmpty();
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(EqColors::TextBright))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(EqColors::TextShadow)
			];
	}();

	TSharedRef<SBox> SlotBox = SNew(SBox)
		.WidthOverride(90.f)
		.HeightOverride(42.f)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, Sub, SlotPos]() -> FSlateColor {
				// Highlight slot when a compatible item is being dragged
				UInventorySubsystem* InvSub = GetInventorySubsystem();
				if (InvSub && InvSub->bIsDragging && Sub)
				{
					if (Sub->CanEquipToSlot(InvSub->DragState.EquipSlot, SlotPos))
						return FSlateColor(EqColors::SlotHighlight);
				}
				return FSlateColor(EqColors::SlotBorder);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(EqColors::SlotBg)
				.Padding(FMargin(2.f))
				[
					SNew(SHorizontalBox)
					// Item icon (texture or colored placeholder)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(28.f)
						.HeightOverride(28.f)
						[
							EquipIconWidget
						]
					]
					// Item name or slot label (ammo slot shows quantity)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(4, 0, 0, 0)
					[
						SNew(STextBlock)
						.Text_Lambda([Sub, SlotPos]() -> FText {
							if (!Sub) return FText::FromString(EquipSlots::GetDisplayName(SlotPos));
							FInventoryItem Item = Sub->GetEquippedItem(SlotPos);
							if (Item.IsValid())
							{
								if (SlotPos == EquipSlots::Ammo && Item.Quantity > 0)
									return FText::FromString(FString::Printf(TEXT("%s x%d"), *Item.GetDisplayName(), Item.Quantity));
								return FText::FromString(Item.GetDisplayName());
							}
							return FText::FromString(EquipSlots::GetDisplayName(SlotPos, Sub->GetLocalJobClass()));
						})
						.Font_Lambda([Sub, SlotPos]() -> FSlateFontInfo {
							if (Sub)
							{
								FInventoryItem Item = Sub->GetEquippedItem(SlotPos);
								if (Item.IsValid())
									return FCoreStyle::GetDefaultFontStyle("Bold", 7);
							}
							return FCoreStyle::GetDefaultFontStyle("Italic", 7);
						})
						.ColorAndOpacity_Lambda([Sub, SlotPos]() -> FSlateColor {
							if (Sub)
							{
								FInventoryItem Item = Sub->GetEquippedItem(SlotPos);
								if (Item.IsValid())
									return FSlateColor(EqColors::TextPrimary);
							}
							return FSlateColor(EqColors::TextDim);
						})
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(EqColors::TextShadow)
					]
				]
			]
		];

	// Hover tooltip for occupied slots
	if (Sub)
	{
		FInventoryItem Item = Sub->GetEquippedItem(SlotPos);
		if (Item.IsValid())
		{
			SlotBox->SetToolTip(SNew(SToolTip)[ ItemTooltipBuilder::Build(Item) ]);
		}
	}

	return SlotBox;
}

// ============================================================
// Character portrait
// ============================================================

TSharedRef<SWidget> SEquipmentWidget::BuildPortrait()
{
	return SNew(SBox)
		.WidthOverride(70.f)
		.HeightOverride(130.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(EqColors::PortraitBorder)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(EqColors::PortraitBg)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						// Head placeholder
						SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(FLinearColor(0.55f, 0.45f, 0.35f, 0.6f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 2, 0, 0)
					[
						// Body placeholder
						SNew(SBox).WidthOverride(30.f).HeightOverride(40.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(FLinearColor(0.55f, 0.45f, 0.35f, 0.5f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 2, 0, 0)
					[
						// Legs placeholder
						SNew(SBox).WidthOverride(28.f).HeightOverride(30.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(FLinearColor(0.55f, 0.45f, 0.35f, 0.4f))
						]
					]
				]
			]
		];
}

// ============================================================
// Main equipment layout (3-column: left slots | portrait | right slots)
// ============================================================

TSharedRef<SWidget> SEquipmentWidget::BuildEquipmentLayout()
{
	// Determine if this character is an Assassin (dual wield capable)
	// If so, the Shield slot becomes a "Left Hand" weapon slot
	FString JobClass;
	if (UEquipmentSubsystem* Sub = OwningSubsystem.Get())
	{
		JobClass = Sub->GetLocalJobClass();
	}
	const bool bCanDualWield = EquipSlots::CanDualWield(JobClass);

	// For Assassin: show weapon_left slot instead of shield
	// If weapon_left has an item, show that; otherwise show the shield slot (Assassin can use shields too)
	FString OffHandSlot = bCanDualWield ? EquipSlots::WeaponLeft : EquipSlots::Shield;
	// If Assassin has no left-hand weapon but has a shield, fall back to showing shield
	if (bCanDualWield)
	{
		UEquipmentSubsystem* Sub = OwningSubsystem.Get();
		if (Sub && !Sub->IsSlotOccupied(EquipSlots::WeaponLeft) && Sub->IsSlotOccupied(EquipSlots::Shield))
		{
			OffHandSlot = EquipSlots::Shield;
		}
	}

	return SNew(SHorizontalBox)
		// LEFT COLUMN: Head Top, Head Low, Weapon, Garment, Accessory 1
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::HeadTop) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::HeadLow) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Weapon) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Garment) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Accessory1) ]
		]
		// CENTER: Character portrait
		+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			BuildPortrait()
		]
		// RIGHT COLUMN: Head Mid, Armor, Shield/Left Hand, Shoes, Accessory 2
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::HeadMid) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Armor) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(OffHandSlot) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Footgear) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Accessory2) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[ BuildEquipSlot(EquipSlots::Ammo) ]
		];
}

// ============================================================
// Tooltip
// ============================================================

TSharedRef<SWidget> SEquipmentWidget::BuildTooltip(const FInventoryItem& Item)
{
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	Content->AddSlot().AutoHeight().Padding(4, 4, 4, 2)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item.GetDisplayName()))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(EqColors::GoldHighlight))
	];

	if (!Item.Description.IsEmpty())
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 2)
		[
			SNew(SBox).WidthOverride(180.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Description))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(EqColors::TextPrimary))
				.AutoWrapText(true)
			]
		];
	}

	// Divider
	Content->AddSlot().AutoHeight().Padding(4, 2)
	[
		SNew(SBox).HeightOverride(1.f)
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(EqColors::GoldDivider) ]
	];

	if (Item.ATK > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("ATK: %d"), Item.ATK)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(EqColors::TextPrimary))
		];
	}
	if (Item.DEF > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("DEF: %d"), Item.DEF)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(EqColors::TextPrimary))
		];
	}
	if (Item.Weight > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Weight: %d"), Item.Weight)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(EqColors::TextDim))
		];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(EqColors::GoldDark)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(EqColors::TooltipBg)
			[ Content ]
		];
}

// ============================================================
// Helpers
// ============================================================

FLinearColor SEquipmentWidget::GetItemTypeColor(const FString& ItemType) const
{
	if (ItemType == TEXT("weapon")) return EqColors::WeaponOrange;
	if (ItemType == TEXT("armor"))  return EqColors::ArmorBlue;
	return EqColors::EtcGreen;
}

UInventorySubsystem* SEquipmentWidget::GetInventorySubsystem() const
{
	UEquipmentSubsystem* EqSub = OwningSubsystem.Get();
	if (!EqSub) return nullptr;
	UWorld* World = EqSub->GetWorld();
	if (!World) return nullptr;
	return World->GetSubsystem<UInventorySubsystem>();
}

FString SEquipmentWidget::GetSlotAtPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	// Simplified hit testing — check local position against known slot areas
	// Layout: title(20) + divider(3) + padding(4) = 27px top offset
	// Left column: x=[4, 94], each slot is 42px tall + 4px padding
	// Right column: x=[width-94, width-4]
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const float TopOffset = 27.f;
	const float SlotHeight = 46.f;  // 42 + 4 padding

	float RelY = LocalPos.Y - TopOffset;
	if (RelY < 0) return TEXT("");

	int32 RowIndex = (int32)(RelY / SlotHeight);
	if (RowIndex < 0 || RowIndex > 5) return TEXT("");  // 6 rows (0-5) — added Ammo slot

	// Left column check (x < 100)
	if (LocalPos.X >= 4.f && LocalPos.X <= 100.f)
	{
		static const FString LeftSlots[] = {
			EquipSlots::HeadTop, EquipSlots::HeadLow, EquipSlots::Weapon,
			EquipSlots::Garment, EquipSlots::Accessory1
		};
		return LeftSlots[RowIndex];
	}

	// Right column check (x > width - 100)
	const FVector2D ContentBounds = GetContentSize();
	if (LocalPos.X >= ContentBounds.X - 100.f && LocalPos.X <= ContentBounds.X - 4.f)
	{
		static const FString RightSlots[] = {
			EquipSlots::HeadMid, EquipSlots::Armor, EquipSlots::Shield,
			EquipSlots::Footgear, EquipSlots::Accessory2, EquipSlots::Ammo
		};
		return RightSlots[RowIndex];
	}

	// Center (portrait area) — for auto-equip drop
	return TEXT("portrait");
}

FVector2D SEquipmentWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return CurrentSize;
}

void SEquipmentWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
	if (RootSizeBox.IsValid())
	{
		RootSizeBox->SetWidthOverride(CurrentSize.X);
		RootSizeBox->SetHeightOverride(CurrentSize.Y);
	}
}

// ============================================================
// Mouse input
// ============================================================

FReply SEquipmentWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Bounds check
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

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

		// Bottom-right resize
		if (LocalPos.X > ContentBounds.X - ResizeGrabZone && LocalPos.Y > ContentBounds.Y - ResizeGrabZone)
		{
			bIsResizing = true;
			ResizeStartMouse = ScreenPos;
			ResizeStartSize = CurrentSize;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		// Check if clicking on an equipped slot — start potential drag
		FString SlotPos = GetSlotAtPosition(MyGeometry, ScreenPos);
		UEquipmentSubsystem* EqSub = OwningSubsystem.Get();
		if (EqSub && !SlotPos.IsEmpty() && SlotPos != TEXT("portrait") && EqSub->IsSlotOccupied(SlotPos))
		{
			bDragInitiated = true;
			DragStartPos = ScreenPos;
			DragSourceSlot = SlotPos;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}

	// Right-click: open item inspect
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		FString SlotPos = GetSlotAtPosition(MyGeometry, ScreenPos);
		UEquipmentSubsystem* EqSub = OwningSubsystem.Get();
		if (EqSub && !SlotPos.IsEmpty() && SlotPos != TEXT("portrait"))
		{
			FInventoryItem Item = EqSub->GetEquippedItem(SlotPos);
			if (Item.IsValid())
			{
				if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
				{
					if (UItemInspectSubsystem* InspectSub = World->GetSubsystem<UItemInspectSubsystem>())
					{
						InspectSub->ShowInspect(Item);
					}
				}
			}
		}
		return FReply::Handled();
	}

	return FReply::Handled();
}

FReply SEquipmentWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsDragging)
		{
			bIsDragging = false;
			return FReply::Handled().ReleaseMouseCapture();
		}

		if (bIsResizing)
		{
			bIsResizing = false;
			return FReply::Handled().ReleaseMouseCapture();
		}

		if (bDragInitiated)
		{
			bDragInitiated = false;
			DragSourceSlot.Empty();
			return FReply::Handled().ReleaseMouseCapture();
		}

		// Handle drop FROM inventory TO equipment
		UInventorySubsystem* InvSub = GetInventorySubsystem();
		if (InvSub && InvSub->bIsDragging)
		{
			const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
			const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
			const FVector2D ContentBounds = GetContentSize();

			if (LocalPos.X >= 0 && LocalPos.X <= ContentBounds.X &&
				LocalPos.Y >= 0 && LocalPos.Y <= ContentBounds.Y)
			{
				FString SlotPos = GetSlotAtPosition(MyGeometry, ScreenPos);
				UEquipmentSubsystem* EqSub = OwningSubsystem.Get();

				if (SlotPos == TEXT("portrait"))
				{
					// Auto-equip
					InvSub->CompleteDrop(EItemDropTarget::EquipmentPortrait);
				}
				else if (EqSub && EqSub->CanEquipToSlot(InvSub->DragState.EquipSlot, SlotPos))
				{
					InvSub->CompleteDrop(EItemDropTarget::EquipmentSlot, SlotPos);
				}
				else
				{
					InvSub->CancelDrag();
				}
			}
			else
			{
				// Dropped outside equipment window
				if (InvSub->DragState.Source == EItemDragSource::Equipment)
				{
					// Dragged from equipment to outside = unequip
					InvSub->CompleteDrop(EItemDropTarget::InventorySlot);
				}
				else
				{
					InvSub->CancelDrag();
				}
			}
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Handled();
}

FReply SEquipmentWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();

	if (bIsDragging)
	{
		const FVector2D AbsDelta = ScreenPos - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}

	if (bIsResizing)
	{
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		FVector2D Delta = (ScreenPos - ResizeStartMouse) / DPIScale;
		CurrentSize.X = FMath::Max(MinWidth, ResizeStartSize.X + Delta.X);
		CurrentSize.Y = FMath::Max(MinHeight, ResizeStartSize.Y + Delta.Y);
		ApplyLayout();
		return FReply::Handled();
	}

	// Item drag from equipment
	if (bDragInitiated && !DragSourceSlot.IsEmpty())
	{
		float Distance = FVector2D::Distance(ScreenPos, DragStartPos);
		if (Distance > DragThreshold)
		{
			UEquipmentSubsystem* EqSub = OwningSubsystem.Get();
			UInventorySubsystem* InvSub = GetInventorySubsystem();
			if (EqSub && InvSub)
			{
				FInventoryItem Item = EqSub->GetEquippedItem(DragSourceSlot);
				if (Item.IsValid())
				{
					InvSub->StartDrag(Item, EItemDragSource::Equipment);
				}
			}
			bDragInitiated = false;
			DragSourceSlot.Empty();
			// Release mouse capture so other widgets (hotbar, inventory) can
			// receive the mouse-up event and handle the drop directly.
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

FReply SEquipmentWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	FString SlotPos = GetSlotAtPosition(MyGeometry, ScreenPos);

	UEquipmentSubsystem* EqSub = OwningSubsystem.Get();
	if (EqSub && !SlotPos.IsEmpty() && SlotPos != TEXT("portrait") && EqSub->IsSlotOccupied(SlotPos))
	{
		EqSub->UnequipSlot(SlotPos);
		return FReply::Handled();
	}

	return FReply::Handled();
}

FCursorReply SEquipmentWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Hide hardware cursor during item drag — the drag icon replaces it
	UInventorySubsystem* InvSub = const_cast<SEquipmentWidget*>(this)->GetInventorySubsystem();
	if (InvSub && InvSub->bIsDragging)
	{
		return FCursorReply::Cursor(EMouseCursor::None);
	}

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	const FVector2D ContentBounds = GetContentSize();

	if (LocalPos.X > ContentBounds.X - ResizeGrabZone && LocalPos.Y > ContentBounds.Y - ResizeGrabZone &&
		LocalPos.X >= 0 && LocalPos.Y >= 0)
	{
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);
	}

	return FCursorReply::Unhandled();
}
