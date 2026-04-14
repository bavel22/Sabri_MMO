// SInventoryWidget.cpp — RO Classic inventory window: tabs, scrollable grid, drag-and-drop,
// double-click equip/use, right-click tooltips, weight/zuzucoin display.

#include "SInventoryWidget.h"
#include "InventorySubsystem.h"
#include "StorageSubsystem.h"
#include "ItemInspectSubsystem.h"
#include "ItemTooltipBuilder.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Colors (same namespace as other widgets)
// ============================================================

namespace InvColors
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
	static const FLinearColor SlotHover     (0.55f, 0.45f, 0.22f, 1.f);
	static const FLinearColor TabActive     (0.50f, 0.38f, 0.20f, 1.f);
	static const FLinearColor TabInactive   (0.30f, 0.20f, 0.10f, 1.f);
	static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
	// Item type colors for placeholder icons
	static const FLinearColor ConsumableRed (0.70f, 0.20f, 0.15f, 1.f);
	static const FLinearColor WeaponOrange  (0.75f, 0.50f, 0.15f, 1.f);
	static const FLinearColor ArmorBlue     (0.20f, 0.35f, 0.65f, 1.f);
	static const FLinearColor EtcGreen      (0.25f, 0.55f, 0.25f, 1.f);
	static const FLinearColor CardPurple    (0.55f, 0.25f, 0.60f, 1.f);
	static const FLinearColor TooltipBg     (0.12f, 0.08f, 0.05f, 0.95f);
}

static constexpr float CellSize = 34.f;   // 32 + 2 border
static constexpr float IconSize = 28.f;

// ============================================================
// Construction
// ============================================================

void SInventoryWidget::Construct(const FArguments& InArgs)
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
			.BorderBackgroundColor(InvColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(InvColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(InvColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(InvColors::GoldDivider) ]
						]
						// Toolbar (Search + Sort + AutoStack)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildToolbar() ]
						// Tab bar + Grid area
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[ BuildTabBar() ]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[ BuildGridArea() ]
						]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(InvColors::GoldDivider) ]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildBottomBar() ]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Tick — rebuild grid when data changes
// ============================================================

void SInventoryWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Detect drag release outside any widget — mouse capture was released when drag started,
	// so OnMouseButtonUp won't fire on this widget. Poll the mouse button state instead.
	if (Sub->bIsDragging && Sub->DragState.Source == EItemDragSource::Inventory)
	{
		if (!FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton))
		{
			// Left mouse released while dragging — show drop confirmation popup
			FInventoryItem* Item = Sub->FindItemByInventoryId(Sub->DragState.InventoryId);
			if (Item)
			{
				ShowDropPopup(Item->InventoryId, Item->Name, Item->bStackable, Item->Quantity);
			}
			Sub->CancelDrag(); // Cancel the visual drag, popup handles the actual drop
		}
	}

	// Check if inventory data or tab changed
	uint32 CurrentVersion = Sub->DataVersion;
	int32 CurrentTabId = Sub->CurrentTab;
	if (CurrentVersion != LastDataVersion || CurrentTabId != LastTabId)
	{
		const bool bTabChanged = (CurrentTabId != LastTabId);
		LastDataVersion = CurrentVersion;
		LastTabId = CurrentTabId;

		// Compare the filtered item set — if only quantities changed, skip the
		// full rebuild.  The per-frame lambdas (quantity badge, "?" indicator)
		// already read live data from the cached filtered array, so they
		// update automatically without recreating widgets.
		const TArray<FInventoryItem>& Filtered = Sub->GetFilteredItems();
		bool bNeedsRebuild = bTabChanged || (Filtered.Num() != LastFilteredInventoryIds.Num());
		if (!bNeedsRebuild)
		{
			for (int32 i = 0; i < Filtered.Num(); ++i)
			{
				if (Filtered[i].InventoryId != LastFilteredInventoryIds[i])
				{
					bNeedsRebuild = true;
					break;
				}
			}
		}

		if (bNeedsRebuild)
		{
			RebuildGrid();
			// Snapshot current filtered IDs for next comparison
			LastFilteredInventoryIds.Empty(Filtered.Num());
			for (const FInventoryItem& Item : Filtered)
			{
				LastFilteredInventoryIds.Add(Item.InventoryId);
			}
		}
	}

	// Update drag cursor position to follow mouse
	if (Sub->bIsDragging)
	{
		Sub->UpdateDragCursorPosition();
	}
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildTitleBar()
{
	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Inventory")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(InvColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(InvColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("X")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(InvColors::TextDim))
			]
		];
}

// ============================================================
// Tab bar (vertical tabs on left: Item | Equip | Etc)
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildTabBar()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();

	auto MakeTab = [this, Sub](int32 TabId, const FString& Label) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(22.f)
			.HeightOverride(60.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor_Lambda([Sub, TabId]() -> FSlateColor {
					if (Sub && Sub->CurrentTab == TabId)
						return FSlateColor(InvColors::TabActive);
					return FSlateColor(InvColors::TabInactive);
				})
				.Padding(FMargin(2.f))
				.OnMouseButtonDown_Lambda([this, Sub, TabId](const FGeometry&, const FPointerEvent& Event) -> FReply {
					if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Sub)
					{
						Sub->SetTab(TabId);
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(InvColors::TextShadow)
					.Justification(ETextJustify::Center)
				]
			];
	};

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight() [ MakeTab(0, TEXT("Item")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 0) [ MakeTab(1, TEXT("Equip")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 0) [ MakeTab(2, TEXT("Etc")) ];
}

// ============================================================
// Toolbar — Search + Sort + AutoStack
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildToolbar()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();

	auto MakeToolBtn = [](const FString& Label, TFunction<void()> OnClick) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InvColors::PanelMedium)
			.Padding(FMargin(4.f, 1.f))
			.OnMouseButtonDown_Lambda([OnClick](const FGeometry&, const FPointerEvent& Event) -> FReply {
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					OnClick();
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
			];
	};

	return SNew(SBox)
		.HeightOverride(20.f)
		.Padding(FMargin(4.f, 1.f))
		[
			SNew(SHorizontalBox)
			// Search input
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SEditableTextBox)
				.HintText(FText::FromString(TEXT("Search...")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.OnTextChanged_Lambda([Sub](const FText& NewText) {
					if (Sub)
					{
						Sub->SearchFilter = NewText.ToString();
						++Sub->DataVersion;
					}
				})
			]
			// Sort: Type
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Type"), [Sub]() { if (Sub) Sub->SortInventory(TEXT("type")); }) ]
			// Sort: Name
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Name"), [Sub]() { if (Sub) Sub->SortInventory(TEXT("name")); }) ]
			// Sort: Weight
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Wt"), [Sub]() { if (Sub) Sub->SortInventory(TEXT("weight")); }) ]
			// Auto-Stack
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Stack"), [Sub]() { if (Sub) Sub->AutoStack(); }) ]
		];
}

// ============================================================
// Grid area (scrollable item grid)
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildGridArea()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(GridScrollBox, SScrollBox)
			.Orientation(Orient_Vertical)
			.ScrollBarVisibility(EVisibility::Visible)
			+ SScrollBox::Slot()
			[
				SAssignNew(GridContainer, SVerticalBox)
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SAssignNew(SplitPopupBox, SBox)
			.Visibility(EVisibility::Collapsed)
		]
		;
}

void SInventoryWidget::RebuildGrid()
{
	if (!GridContainer.IsValid()) return;
	GridContainer->ClearChildren();

	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	const TArray<FInventoryItem>& FilteredItems = Sub->GetFilteredItems();

	// Build rows of GridColumns cells each
	int32 NumRows = FMath::CeilToInt32((float)FMath::Max(FilteredItems.Num(), GridColumns * 4) / (float)GridColumns);

	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		TSharedPtr<SHorizontalBox> RowBox;
		GridContainer->AddSlot().AutoHeight()
		[
			SAssignNew(RowBox, SHorizontalBox)
		];

		for (int32 Col = 0; Col < GridColumns; ++Col)
		{
			int32 ItemIndex = Row * GridColumns + Col;
			bool bHasItem = ItemIndex < FilteredItems.Num();

			if (bHasItem)
			{
				RowBox->AddSlot().AutoWidth()
				[ BuildItemSlot(ItemIndex) ];
			}
			else
			{
				// Empty slot
				RowBox->AddSlot().AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(CellSize)
					.HeightOverride(CellSize)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(InvColors::SlotBg)
					]
				];
			}
		}
	}

	// Snapshot filtered IDs so Tick can detect quantity-only changes
	LastFilteredInventoryIds.Empty(FilteredItems.Num());
	for (const FInventoryItem& Item : FilteredItems)
	{
		LastFilteredInventoryIds.Add(Item.InventoryId);
	}
}

// ============================================================
// Individual item slot
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildItemSlot(int32 SlotIndex)
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	FInventoryItem Item = GetItemAtSlot(SlotIndex);

	// Pre-build icon widget: texture icon if available, colored placeholder otherwise
	TSharedRef<SWidget> IconWidget = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = Sub ? Sub->GetOrCreateItemIconBrush(Item.Icon) : nullptr;
		if (Brush)
		{
			return SNew(SImage).Image(Brush);
		}
		// Fallback: colored square + 2-letter text
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Item.IsValid() ? GetItemTypeColor(Item.ItemType) : FLinearColor::Transparent)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Item.IsValid() && Item.Name.Len() > 0 ? FText::FromString(Item.Name.Left(2)) : FText::GetEmpty())
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(InvColors::TextBright))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(InvColors::TextShadow)
			];
	}();

	TSharedRef<SBox> SlotBox = SNew(SBox)
		.WidthOverride(CellSize)
		.HeightOverride(CellSize)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, Sub, SlotIndex]() -> FSlateColor {
				// Dim the border when this slot is being dragged
				if (Sub && Sub->bIsDragging && DragSourceSlotIndex == SlotIndex)
					return FSlateColor(FLinearColor(0.25f, 0.18f, 0.10f, 0.5f));
				return FSlateColor(InvColors::SlotBorder);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(InvColors::SlotBg)
				[
					SNew(SOverlay)
					// Item icon (texture or colored placeholder)
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(IconSize)
						.HeightOverride(IconSize)
						.Visibility_Lambda([this, Sub, SlotIndex]() -> EVisibility {
							if (Sub && Sub->bIsDragging && DragSourceSlotIndex == SlotIndex)
								return EVisibility::Hidden;
							return EVisibility::SelfHitTestInvisible;
						})
						[
							IconWidget
						]
					]
					// Quantity badge (bottom-right)
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Sub, SlotIndex]() -> FText {
							if (Sub && Sub->bIsDragging && DragSourceSlotIndex == SlotIndex)
								return FText::GetEmpty();
							FInventoryItem It = GetItemAtSlot(SlotIndex);
							if (It.IsValid() && It.bStackable && It.Quantity > 1)
								return FText::AsNumber(It.Quantity);
							return FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(InvColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(InvColors::TextShadow)
					]
					// Unidentified "?" indicator (top-left)
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text_Lambda([this, SlotIndex]() -> FText {
							FInventoryItem It = GetItemAtSlot(SlotIndex);
							return (It.IsValid() && !It.bIdentified)
								? FText::FromString(TEXT("?"))
								: FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f, 1.f)))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
					]
				]
			]
		];

	// Hover tooltip for filled slots (rebuilt each time grid rebuilds)
	if (Item.IsValid())
	{
		SlotBox->SetToolTip(SNew(SToolTip)[ ItemTooltipBuilder::Build(Item) ]);
	}

	return SlotBox;
}

TSharedRef<SWidget> SInventoryWidget::BuildItemIcon(const FInventoryItem& Item)
{
	return SNullWidget::NullWidget;
}

// ============================================================
// Tooltip
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildTooltip(const FInventoryItem& Item)
{
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	// Name
	Content->AddSlot().AutoHeight().Padding(4, 4, 4, 2)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Item.GetDisplayName()))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(InvColors::GoldHighlight))
	];

	// Type
	Content->AddSlot().AutoHeight().Padding(4, 0, 4, 2)
	[
		SNew(STextBlock)
		.Text(FText::FromString(FString::Printf(TEXT("Type: %s"), *Item.ItemType)))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		.ColorAndOpacity(FSlateColor(InvColors::TextDim))
	];

	// Description
	if (!Item.Description.IsEmpty())
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 2)
		[
			SNew(SBox).WidthOverride(180.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Description))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
				.AutoWrapText(true)
			]
		];
	}

	// Divider
	Content->AddSlot().AutoHeight().Padding(4, 2)
	[
		SNew(SBox).HeightOverride(1.f)
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(InvColors::GoldDivider) ]
	];

	// Stats
	if (Item.ATK > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("ATK: %d"), Item.ATK)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
		];
	}
	if (Item.DEF > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("DEF: %d"), Item.DEF)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
		];
	}
	if (Item.Weight > 0)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Weight: %d"), Item.Weight)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(InvColors::TextPrimary))
		];
	}
	if (Item.RequiredLevel > 1)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0, 4, 4)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Required Lv: %d"), Item.RequiredLevel)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(InvColors::TextDim))
		];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InvColors::GoldDark)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InvColors::TooltipBg)
			[
				Content
			]
		];
}

void SInventoryWidget::ShowTooltip(const FInventoryItem& Item, const FGeometry& Geometry, const FVector2D& ScreenPos)
{
	// Tooltips are handled via the tooltip system — we use Slate's native tooltip
	// For now, the tooltip is built inline. A future improvement could use STooltipPresenter.
	TooltipItemId = Item.InventoryId;
}

void SInventoryWidget::HideTooltip()
{
	TooltipItemId = -1;
}

// ============================================================
// Bottom bar (weight + zuzucoin)
// ============================================================

TSharedRef<SWidget> SInventoryWidget::BuildBottomBar()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(20.f)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			// Weight
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("Wt: %d/%d"), Sub->CurrentWeight, Sub->MaxWeight));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(InvColors::TextDim))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(InvColors::TextShadow)
			]
			// Zuzucoin
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("%d z"), Sub->Zuzucoin));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(InvColors::ZuzucoinGold))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(InvColors::TextShadow)
			]
		];
}

// ============================================================
// Helpers
// ============================================================

FInventoryItem SInventoryWidget::GetItemAtSlot(int32 SlotIndex) const
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FInventoryItem();

	const TArray<FInventoryItem>& Filtered = Sub->GetFilteredItems();
	if (SlotIndex >= 0 && SlotIndex < Filtered.Num())
		return Filtered[SlotIndex];
	return FInventoryItem();
}

FLinearColor SInventoryWidget::GetItemTypeColor(const FString& ItemType) const
{
	if (ItemType == TEXT("consumable")) return InvColors::ConsumableRed;
	if (ItemType == TEXT("weapon"))     return InvColors::WeaponOrange;
	if (ItemType == TEXT("armor"))      return InvColors::ArmorBlue;
	if (ItemType == TEXT("card"))       return InvColors::CardPurple;
	return InvColors::EtcGreen;
}

int32 SInventoryWidget::GetSlotIndexFromPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	// Calculate which grid slot the mouse is over
	// Grid starts after: 3px borders + 20px title + 3px divider + 20px toolbar = 46px top,
	//                    3px borders + 22px tab bar = 25px left
	FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	float GridLeft = 25.f;
	float GridTop = 46.f;

	float RelX = LocalPos.X - GridLeft;
	float RelY = LocalPos.Y - GridTop;

	// Account for scroll offset
	if (GridScrollBox.IsValid())
	{
		RelY += GridScrollBox->GetScrollOffset();
	}

	if (RelX < 0 || RelY < 0) return -1;

	int32 Col = (int32)(RelX / CellSize);
	int32 Row = (int32)(RelY / CellSize);

	if (Col >= GridColumns || Col < 0) return -1;

	return Row * GridColumns + Col;
}

FVector2D SInventoryWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return CurrentSize;
}

void SInventoryWidget::ApplyLayout()
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
// Keyboard input
// ============================================================

FReply SInventoryWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (bSplitPopupActive)
		{
			HideSplitPopup();
			return FReply::Handled();
		}
		UInventorySubsystem* Sub = OwningSubsystem.Get();
		if (Sub) Sub->ToggleWidget();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Split quantity popup (floating at icon position)
// ============================================================

void SInventoryWidget::ShowSplitPopup(int32 InventoryId, int32 MaxQty, const FVector2D& LocalPos)
{
	SplitSourceInventoryId = InventoryId;
	SplitMaxQuantity = MaxQty;
	bSplitPopupActive = true;
	if (!SplitPopupBox.IsValid()) return;

	SplitPopupBox->SetVisibility(EVisibility::Visible);
	SplitPopupBox->SetContent(
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InvColors::GoldTrim)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InvColors::PanelDark)
			.Padding(FMargin(2.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(36.f)
					[
						SAssignNew(SplitInputBox, SEditableTextBox)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.Text(FText::AsNumber(FMath::Max(1, MaxQty / 2)))
						.SelectAllTextWhenFocused(true)
						.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType) {
							if (CommitType == ETextCommit::OnEnter) ConfirmSplit();
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0, 0, 0).VAlign(VAlign_Center)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(InvColors::GoldDark)
					.Padding(FMargin(3.f, 1.f))
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
						if (E.GetEffectingButton() == EKeys::LeftMouseButton) { ConfirmSplit(); return FReply::Handled(); }
						return FReply::Unhandled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("OK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(InvColors::TextBright))
					]
				]
			]
		]
	);

	float PopupX = FMath::Clamp(LocalPos.X - 30.f, 0.f, CurrentSize.X - 70.f);
	float PopupY = FMath::Clamp(LocalPos.Y - 22.f, 0.f, CurrentSize.Y - 22.f);
	SplitPopupBox->SetRenderTransform(FSlateRenderTransform(FVector2f((float)PopupX, (float)PopupY)));

	if (SplitInputBox.IsValid()) FSlateApplication::Get().SetKeyboardFocus(SplitInputBox);
}

void SInventoryWidget::HideSplitPopup()
{
	bSplitPopupActive = false;
	SplitSourceInventoryId = 0;
	SplitMaxQuantity = 0;
	SplitInputBox.Reset();
	if (SplitPopupBox.IsValid())
	{
		SplitPopupBox->SetVisibility(EVisibility::Collapsed);
		SplitPopupBox->SetContent(SNullWidget::NullWidget);
	}
}

void SInventoryWidget::ConfirmSplit()
{
	if (!SplitInputBox.IsValid() || SplitSourceInventoryId == 0) { HideSplitPopup(); return; }
	int32 Qty = FCString::Atoi(*SplitInputBox->GetText().ToString().TrimStartAndEnd());
	if (Qty < 1 || Qty > SplitMaxQuantity) { HideSplitPopup(); return; }
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (Sub) Sub->SplitStack(SplitSourceInventoryId, Qty);
	HideSplitPopup();
}

// ============================================================
// Drop confirmation popup (RO Classic)
// Non-stackable: "Drop [Item Name]?" with OK/Cancel
// Stackable: quantity input + OK/Cancel
// ============================================================

void SInventoryWidget::ShowDropPopup(int32 InventoryId, const FString& ItemName, bool bStackable, int32 MaxQty)
{
	HideDropPopup(); // Clean up any previous popup

	DropSourceInventoryId = InventoryId;
	DropMaxQuantity = MaxQty;
	bDropIsStackable = bStackable;
	DropItemName = ItemName;
	bDropPopupActive = true;

	// Build popup content
	TSharedRef<SVerticalBox> Content = SNew(SVerticalBox);

	Content->AddSlot().AutoHeight().Padding(4.f, 2.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(bStackable
			? FString::Printf(TEXT("Drop how many %s?"), *ItemName)
			: FString::Printf(TEXT("Drop %s?"), *ItemName)))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(InvColors::TextBright))
		.Justification(ETextJustify::Center)
	];

	if (bStackable)
	{
		Content->AddSlot().AutoHeight().Padding(4.f, 2.f).HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(50.f)
			[
				SAssignNew(DropQuantityInput, SEditableTextBox)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Text(FText::AsNumber(MaxQty))
				.SelectAllTextWhenFocused(true)
				.Justification(ETextJustify::Center)
				.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType) {
					if (CommitType == ETextCommit::OnEnter) ConfirmDrop();
				})
			]
		];
	}

	Content->AddSlot().AutoHeight().Padding(4.f, 4.f).HAlign(HAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.20f, 0.45f, 0.20f, 1.f))
			.Padding(FMargin(10.f, 3.f))
			.Cursor(EMouseCursor::Hand)
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
				if (E.GetEffectingButton() == EKeys::LeftMouseButton) { ConfirmDrop(); return FReply::Handled(); }
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("OK")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(InvColors::TextBright))
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(FLinearColor(0.45f, 0.20f, 0.20f, 1.f))
			.Padding(FMargin(10.f, 3.f))
			.Cursor(EMouseCursor::Hand)
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
				if (E.GetEffectingButton() == EKeys::LeftMouseButton) { HideDropPopup(); return FReply::Handled(); }
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Cancel")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(InvColors::TextBright))
			]
		]
	];

	// Position popup at cursor
	FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();

	DropPopupBox = SNew(SBox)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(InvColors::GoldTrim)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(InvColors::PanelDark)
			.Padding(FMargin(4.f))
			[
				Content
			]
		]
	];

	// Use FSlateApplication::PushMenu — positions at absolute cursor coords natively
	FVector2D AbsCursor = FSlateApplication::Get().GetCursorPos();

	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelRegularWindow();
	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().PushMenu(
			ParentWindow.ToSharedRef(),
			FWidgetPath(),
			DropPopupBox.ToSharedRef(),
			AbsCursor,
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);
	}
}

void SInventoryWidget::HideDropPopup()
{
	bDropPopupActive = false;
	bPendingDropFromDrag = false;
	DropSourceInventoryId = 0;
	DropMaxQuantity = 0;
	DropQuantityInput.Reset();

	if (DropPopupBox.IsValid())
	{
		FSlateApplication::Get().DismissAllMenus();
	}
	DropPopupBox.Reset();
	DropPopupAlignWrapper.Reset();
	DropPopupViewportOverlay.Reset();
}

void SInventoryWidget::ConfirmDrop()
{
	if (DropSourceInventoryId == 0) { HideDropPopup(); return; }

	int32 DropQty = DropMaxQuantity;
	if (bDropIsStackable && DropQuantityInput.IsValid())
	{
		DropQty = FCString::Atoi(*DropQuantityInput->GetText().ToString().TrimStartAndEnd());
		if (DropQty < 1 || DropQty > DropMaxQuantity) { HideDropPopup(); return; }
	}

	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (Sub) Sub->DropItem(DropSourceInventoryId, DropQty);
	HideDropPopup();
}

// ============================================================
// Mouse input
// ============================================================

FReply SInventoryWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Bounds check — clicks outside pass through
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Title bar drag (window movement)
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

		// Check if clicking on a grid item
		int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
		FInventoryItem Item = GetItemAtSlot(SlotIdx);
		if (Item.IsValid())
		{
			// Shift+Click: show split quantity dialog
			if (MouseEvent.IsShiftDown() && Item.bStackable && Item.Quantity > 1)
			{
				ShowSplitPopup(Item.InventoryId, Item.Quantity - 1, LocalPos);
				return FReply::Handled();
			}

			// Normal click: start potential drag
			bDragInitiated = true;
			DragStartPos = ScreenPos;
			DragSourceInventoryId = Item.InventoryId;
			DragSourceSlotIndex = SlotIdx;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}

	// Right-click handling
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
		FInventoryItem Item = GetItemAtSlot(SlotIdx);
		if (Item.IsValid())
		{
			// Alt+RightClick: quick deposit to storage (if open)
			if (MouseEvent.IsAltDown())
			{
				if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
				{
					if (UStorageSubsystem* StorageSub = World->GetSubsystem<UStorageSubsystem>())
					{
						if (StorageSub->bIsOpen)
						{
							StorageSub->DepositItem(Item.InventoryId, Item.Quantity);
							return FReply::Handled();
						}
					}
				}
			}

			// Normal right-click: open item inspect
			if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
			{
				if (UItemInspectSubsystem* InspectSub = World->GetSubsystem<UItemInspectSubsystem>())
				{
					InspectSub->ShowInspect(Item);
				}
			}
			return FReply::Handled();
		}
	}

	return FReply::Handled();
}

FReply SInventoryWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

		// If we were in item drag mode but it was actually a click (not a drag)
		if (bDragInitiated)
		{
			bDragInitiated = false;
			DragSourceInventoryId = 0;
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}

		// If subsystem is dragging (actual item drag), complete the drop
		UInventorySubsystem* Sub = OwningSubsystem.Get();
		if (Sub && Sub->bIsDragging)
		{
			const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
			const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
			const FVector2D ContentBounds = GetContentSize();

			if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
				LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
			{
				// Dropped outside inventory window
				if (Sub->DragState.Source == EItemDragSource::Equipment)
				{
					// From equipment → drop outside inventory = unequip to inventory
					Sub->CompleteDrop(EItemDropTarget::InventorySlot);
				}
				else if (Sub->DragState.Source == EItemDragSource::Cart)
				{
					// From cart → drop outside inventory = move to inventory
					Sub->CompleteDrop(EItemDropTarget::InventorySlot);
				}
				else if (!Sub->DragState.EquipSlot.IsEmpty())
				{
					// Equippable item dragged outside — show drop confirmation
					FInventoryItem* DItem = Sub->FindItemByInventoryId(Sub->DragState.InventoryId);
					if (DItem) ShowDropPopup(DItem->InventoryId, DItem->Name, DItem->bStackable, DItem->Quantity);
					Sub->CancelDrag();
				}
				else
				{
					// Non-equippable item dragged outside — show drop confirmation
					FInventoryItem* DItem = Sub->FindItemByInventoryId(Sub->DragState.InventoryId);
					if (DItem) ShowDropPopup(DItem->InventoryId, DItem->Name, DItem->bStackable, DItem->Quantity);
					Sub->CancelDrag();
				}
			}
			else
			{
				// Dropped inside inventory
				int32 TargetVisualSlot = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
				if (Sub->DragState.Source == EItemDragSource::Equipment)
				{
					Sub->CompleteDrop(EItemDropTarget::InventorySlot);
				}
				else if (Sub->DragState.Source == EItemDragSource::Cart)
				{
					// From cart → dropped on inventory = move to inventory
					Sub->CompleteDrop(EItemDropTarget::InventorySlot);
				}
				else if (TargetVisualSlot >= 0)
				{
					// Use the target item's DB slot_index for proper swap
					FInventoryItem TargetItem = GetItemAtSlot(TargetVisualSlot);
					int32 TargetDbSlotIndex = TargetItem.IsValid() ? TargetItem.SlotIndex : TargetVisualSlot;
					Sub->CompleteDrop(EItemDropTarget::InventorySlot, TEXT(""), TargetDbSlotIndex);
				}
				else
				{
					// Invalid target slot — cancel
					Sub->CancelDrag();
				}
			}
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Handled();
}

FReply SInventoryWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();

	// Window dragging
	if (bIsDragging)
	{
		const FVector2D AbsDelta = ScreenPos - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}

	// Window resizing
	if (bIsResizing)
	{
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		FVector2D Delta = (ScreenPos - ResizeStartMouse) / DPIScale;
		CurrentSize.X = FMath::Max(MinWidth, ResizeStartSize.X + Delta.X);
		CurrentSize.Y = FMath::Max(MinHeight, ResizeStartSize.Y + Delta.Y);
		ApplyLayout();
		return FReply::Handled();
	}

	// Item drag detection
	if (bDragInitiated && DragSourceInventoryId > 0)
	{
		float Distance = FVector2D::Distance(ScreenPos, DragStartPos);
		if (Distance > DragThreshold)
		{
			// Start actual drag
			UInventorySubsystem* Sub = OwningSubsystem.Get();
			if (Sub)
			{
				FInventoryItem* Item = Sub->FindItemByInventoryId(DragSourceInventoryId);
				if (Item)
				{
					Sub->StartDrag(*Item, EItemDragSource::Inventory);
				}
			}
			bDragInitiated = false;
			DragSourceInventoryId = 0;
			DragSourceSlotIndex = -1;
			// Release mouse capture so other widgets (hotbar, equipment) can
			// receive the mouse-up event and handle the drop directly.
			// Drag cursor icon update continues via Tick (not OnMouseMove).
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

FReply SInventoryWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
	FInventoryItem Item = GetItemAtSlot(SlotIdx);

	if (!Item.IsValid()) return FReply::Handled();

	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FReply::Handled();

	if (Item.IsConsumable())
	{
		Sub->UseItem(Item.InventoryId);
	}
	else if (Item.IsCard())
	{
		Sub->BeginCardCompound(Item);
	}
	else if (Item.IsEquippable())
	{
		Sub->EquipItem(Item.InventoryId);
	}

	return FReply::Handled();
}

FReply SInventoryWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GridScrollBox.IsValid())
	{
		float ScrollDelta = MouseEvent.GetWheelDelta() * -CellSize;
		GridScrollBox->SetScrollOffset(GridScrollBox->GetScrollOffset() + ScrollDelta);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FCursorReply SInventoryWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Hide hardware cursor during item drag — the drag icon replaces it
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (Sub && Sub->bIsDragging)
	{
		return FCursorReply::Cursor(EMouseCursor::None);
	}

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	const FVector2D ContentBounds = GetContentSize();

	// Resize cursor at bottom-right corner
	if (LocalPos.X > ContentBounds.X - ResizeGrabZone && LocalPos.Y > ContentBounds.Y - ResizeGrabZone &&
		LocalPos.X >= 0 && LocalPos.Y >= 0)
	{
		return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);
	}

	// Show grab hand when hovering over items (hint that they're draggable)
	int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, CursorEvent.GetScreenSpacePosition());
	FInventoryItem Item = GetItemAtSlot(SlotIdx);
	if (Item.IsValid())
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHand);
	}

	return FCursorReply::Unhandled();
}
