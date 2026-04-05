// SStorageWidget.cpp — RO Classic Kafra Storage window: scrollable grid, tab filtering,
// drag-and-drop, double-click to withdraw, item tooltips, slot count display.

#include "SStorageWidget.h"
#include "StorageSubsystem.h"
#include "InventorySubsystem.h"
#include "CartSubsystem.h"
#include "ItemInspectSubsystem.h"
#include "ItemTooltipBuilder.h"
#include "Engine/Engine.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Colors (same brown/gold palette as other widgets)
// ============================================================

namespace StorageColors
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
	static const FLinearColor TabActive     (0.55f, 0.40f, 0.20f, 1.f);
	static const FLinearColor TabInactive   (0.30f, 0.20f, 0.12f, 1.f);
	static const FLinearColor ConsumableRed (0.70f, 0.20f, 0.15f, 1.f);
	static const FLinearColor WeaponOrange  (0.75f, 0.50f, 0.15f, 1.f);
	static const FLinearColor ArmorBlue     (0.20f, 0.35f, 0.65f, 1.f);
	static const FLinearColor EtcGreen      (0.25f, 0.55f, 0.25f, 1.f);
	static const FLinearColor CardPurple    (0.55f, 0.25f, 0.60f, 1.f);
	static const FLinearColor SlotBarBg     (0.12f, 0.08f, 0.04f, 1.f);
	static const FLinearColor SlotBarFill   (0.72f, 0.58f, 0.28f, 1.f);
}

static constexpr float CellSize = 34.f;
static constexpr float IconSize = 28.f;

// ============================================================
// Construction
// ============================================================

void SStorageWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(CurrentSize.X)
		.HeightOverride(CurrentSize.Y)
		[
			// 3-layer frame: Gold -> Dark -> Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(StorageColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(StorageColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(StorageColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(StorageColors::GoldDivider) ]
						]
						// Tab bar
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTabBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(StorageColors::GoldDivider) ]
						]
						// Toolbar (Search + Sort + AutoStack)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildToolbar() ]
						// Grid area
						+ SVerticalBox::Slot().FillHeight(1.f)
						[ BuildGridArea() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(StorageColors::GoldDivider) ]
						]
						// Slot count bar
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildSlotCountBar() ]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ============================================================
// Tick — rebuild grid when data changes or tab changes
// ============================================================

void SStorageWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UStorageSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	uint32 CurrentVersion = Sub->DataVersion;
	int32 CurrentTabVal = Sub->CurrentTab;
	if (CurrentVersion != LastDataVersion || CurrentTabVal != LastTab)
	{
		LastDataVersion = CurrentVersion;
		LastTab = CurrentTabVal;
		FilteredItems = Sub->GetFilteredItems();
		RebuildGrid();
	}

	// Update drag cursor position (uses InventorySubsystem's shared drag cursor)
	if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
	{
		if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
		{
			if (InvSub->bIsDragging)
			{
				InvSub->UpdateDragCursorPosition();
			}
		}
	}
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildTitleBar()
{
	UStorageSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::FromString(TEXT("Kafra Storage"));
					return FText::FromString(FString::Printf(TEXT("Kafra Storage (%d/%d)"), Sub->UsedSlots, Sub->MaxSlots));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(StorageColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(StorageColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(FLinearColor::Transparent)
				.Padding(FMargin(2.f, 0.f))
				.OnMouseButtonDown_Lambda([Sub](const FGeometry&, const FPointerEvent& Event) -> FReply {
					if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Sub)
					{
						Sub->RequestClose();
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(StorageColors::TextDim))
				]
			]
		];
}

// ============================================================
// Tab bar — All / Item / Equip / Etc
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildTabBar()
{
	UStorageSubsystem* Sub = OwningSubsystem.Get();

	auto MakeTab = [Sub](const FString& Label, int32 TabIndex) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([Sub, TabIndex]() -> FSlateColor {
				return (Sub && Sub->CurrentTab == TabIndex) ? StorageColors::TabActive : StorageColors::TabInactive;
			})
			.Padding(FMargin(6.f, 2.f))
			.OnMouseButtonDown_Lambda([Sub, TabIndex](const FGeometry&, const FPointerEvent& Event) -> FReply {
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton && Sub)
				{
					Sub->CurrentTab = TabIndex;
					++Sub->DataVersion; // Force rebuild
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(StorageColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(StorageColors::TextShadow)
			];
	};

	return SNew(SBox)
		.HeightOverride(TabBarHeight)
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f) [ MakeTab(TEXT("All"), 0) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f) [ MakeTab(TEXT("Item"), 1) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f) [ MakeTab(TEXT("Equip"), 2) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f) [ MakeTab(TEXT("Etc"), 3) ]
		];
}

// ============================================================
// Toolbar — Search box + Sort buttons + Auto-stack (not visible as a text input)
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildToolbar()
{
	UStorageSubsystem* Sub = OwningSubsystem.Get();

	auto MakeToolBtn = [Sub](const FString& Label, TFunction<void()> OnClick) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(StorageColors::PanelMedium)
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
				.ColorAndOpacity(FSlateColor(StorageColors::TextPrimary))
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
			[ MakeToolBtn(TEXT("Type"), [Sub]() { if (Sub) Sub->SortStorage(TEXT("type")); }) ]
			// Sort: Name
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Name"), [Sub]() { if (Sub) Sub->SortStorage(TEXT("name")); }) ]
			// Sort: Weight
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(1, 0)
			[ MakeToolBtn(TEXT("Wt"), [Sub]() { if (Sub) Sub->SortStorage(TEXT("weight")); }) ]
		];
}

// ============================================================
// Slot count bar — "127/300 slots"
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildSlotCountBar()
{
	UStorageSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(18.f)
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(StorageColors::SlotBarBg)
			]
			+ SOverlay::Slot().HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride_Lambda([Sub]() -> FOptionalSize {
					if (!Sub || Sub->MaxSlots <= 0) return 0.f;
					float Ratio = FMath::Clamp((float)Sub->UsedSlots / (float)Sub->MaxSlots, 0.f, 1.f);
					return FOptionalSize(Ratio * 370.f);
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(StorageColors::SlotBarFill)
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("%d / %d slots"), Sub->UsedSlots, Sub->MaxSlots));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(StorageColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(StorageColors::TextShadow)
			]
		];
}

// ============================================================
// Grid area (scrollable item grid)
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildGridArea()
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
		];
}

void SStorageWidget::RebuildGrid()
{
	if (!GridContainer.IsValid()) return;
	GridContainer->ClearChildren();

	int32 TotalSlots = FMath::Max(FilteredItems.Num(), GridColumns * 3);
	int32 NumRows = FMath::CeilToInt32((float)TotalSlots / (float)GridColumns);

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
				RowBox->AddSlot().AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(CellSize)
					.HeightOverride(CellSize)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(StorageColors::SlotBg)
					]
				];
			}
		}
	}
}

// ============================================================
// Individual item slot
// ============================================================

TSharedRef<SWidget> SStorageWidget::BuildItemSlot(int32 SlotIndex)
{
	FInventoryItem Item = GetItemAtSlot(SlotIndex);

	TSharedRef<SWidget> IconWidget = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = nullptr;
		if (UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr)
		{
			if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
			{
				Brush = InvSub->GetOrCreateItemIconBrush(Item.Icon);
			}
		}
		if (Brush)
		{
			return SNew(SImage).Image(Brush);
		}
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Item.IsValid() ? GetItemTypeColor(Item.ItemType) : FLinearColor::Transparent)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Item.IsValid() && Item.Name.Len() > 0 ? FText::FromString(Item.Name.Left(2)) : FText::GetEmpty())
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(StorageColors::TextBright))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(StorageColors::TextShadow)
			];
	}();

	TSharedRef<SBox> SlotBox = SNew(SBox)
		.WidthOverride(CellSize)
		.HeightOverride(CellSize)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor_Lambda([this, SlotIndex]() -> FSlateColor {
				if (DragSourceSlotIndex == SlotIndex && bDragInitiated)
					return FSlateColor(FLinearColor(0.25f, 0.18f, 0.10f, 0.5f));
				return FSlateColor(StorageColors::SlotBorder);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(StorageColors::SlotBg)
				[
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(IconSize)
						.HeightOverride(IconSize)
						[ IconWidget ]
					]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
					[
						SNew(STextBlock)
						.Text_Lambda([this, SlotIndex]() -> FText {
							FInventoryItem It = GetItemAtSlot(SlotIndex);
							if (It.IsValid() && It.bStackable && It.Quantity > 1)
								return FText::AsNumber(It.Quantity);
							return FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(StorageColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(StorageColors::TextShadow)
					]
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)
					[
						SNew(STextBlock)
						.Text_Lambda([this, SlotIndex]() -> FText {
							FInventoryItem It = GetItemAtSlot(SlotIndex);
							return (It.IsValid() && !It.bIdentified) ? FText::FromString(TEXT("?")) : FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.2f, 1.f)))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f))
					]
				]
			]
		];

	if (Item.IsValid())
	{
		SlotBox->SetToolTip(SNew(SToolTip)[ ItemTooltipBuilder::Build(Item) ]);
	}

	return SlotBox;
}

// ============================================================
// Helpers
// ============================================================

FInventoryItem SStorageWidget::GetItemAtSlot(int32 SlotIndex) const
{
	if (SlotIndex >= 0 && SlotIndex < FilteredItems.Num())
		return FilteredItems[SlotIndex];
	return FInventoryItem();
}

FLinearColor SStorageWidget::GetItemTypeColor(const FString& ItemType) const
{
	if (ItemType == TEXT("consumable")) return StorageColors::ConsumableRed;
	if (ItemType == TEXT("weapon"))     return StorageColors::WeaponOrange;
	if (ItemType == TEXT("armor"))      return StorageColors::ArmorBlue;
	if (ItemType == TEXT("card"))       return StorageColors::CardPurple;
	return StorageColors::EtcGreen;
}

int32 SStorageWidget::GetSlotIndexFromPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	// Grid starts after: 3px borders + 20px title + 3px divider + 22px tabs + 3px divider + 20px toolbar = 71px top
	float GridLeft = 5.f;
	float GridTop = 71.f;

	float RelX = LocalPos.X - GridLeft;
	float RelY = LocalPos.Y - GridTop;

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

FVector2D SStorageWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return CurrentSize;
}

void SStorageWidget::ApplyLayout()
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
// Split/withdraw quantity popup (floating at icon position)
// ============================================================

void SStorageWidget::RebuildSplitPopup(const FVector2D& LocalPos)
{
	// Called by ShowSplitPopup — not used separately anymore
}

void SStorageWidget::ShowSplitPopup(int32 StorageId, int32 MaxQty, const FVector2D& LocalPos)
{
	SplitSourceStorageId = StorageId;
	SplitMaxQuantity = MaxQty;
	bSplitPopupActive = true;
	if (!SplitPopupBox.IsValid()) return;

	SplitPopupBox->SetVisibility(EVisibility::Visible);
	SplitPopupBox->SetContent(
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(StorageColors::GoldTrim)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(StorageColors::PanelDark)
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
					.BorderBackgroundColor(StorageColors::GoldDark)
					.Padding(FMargin(3.f, 1.f))
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
						if (E.GetEffectingButton() == EKeys::LeftMouseButton) { ConfirmSplit(); return FReply::Handled(); }
						return FReply::Unhandled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("OK")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(StorageColors::TextBright))
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

void SStorageWidget::HideSplitPopup()
{
	bSplitPopupActive = false;
	SplitSourceStorageId = 0;
	SplitMaxQuantity = 0;
	SplitInputBox.Reset();
	if (SplitPopupBox.IsValid())
	{
		SplitPopupBox->SetVisibility(EVisibility::Collapsed);
		SplitPopupBox->SetContent(SNullWidget::NullWidget);
	}
}

void SStorageWidget::ConfirmSplit()
{
	if (!SplitInputBox.IsValid() || SplitSourceStorageId == 0) { HideSplitPopup(); return; }
	int32 Qty = FCString::Atoi(*SplitInputBox->GetText().ToString().TrimStartAndEnd());
	if (Qty < 1 || Qty > SplitMaxQuantity) { HideSplitPopup(); return; }
	UStorageSubsystem* Sub = OwningSubsystem.Get();
	if (Sub) Sub->WithdrawItem(SplitSourceStorageId, Qty);
	HideSplitPopup();
}

// ============================================================
// Keyboard input
// ============================================================

FReply SStorageWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (bSplitPopupActive)
		{
			HideSplitPopup();
			return FReply::Handled();
		}
		UStorageSubsystem* Sub = OwningSubsystem.Get();
		if (Sub) Sub->RequestClose();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Mouse input
// ============================================================

FReply SStorageWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

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

		// Check if clicking on a grid item
		int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
		FInventoryItem Item = GetItemAtSlot(SlotIdx);
		if (Item.IsValid())
		{
			// Shift+Click: show withdraw quantity dialog
			if (MouseEvent.IsShiftDown() && Item.bStackable && Item.Quantity > 1)
			{
				ShowSplitPopup(Item.InventoryId, Item.Quantity, LocalPos);
				return FReply::Handled();
			}

			// Normal click: start potential drag
			bDragInitiated = true;
			DragStartPos = ScreenPos;
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
			// Alt+RightClick: quick withdraw to inventory
			if (MouseEvent.IsAltDown())
			{
				UStorageSubsystem* Sub = OwningSubsystem.Get();
				if (Sub)
				{
					Sub->WithdrawItem(Item.InventoryId, Item.Quantity);
					return FReply::Handled();
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

FReply SStorageWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}

		// Handle drops from InventorySubsystem drag system
		UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
		UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
		if (InvSub && InvSub->bIsDragging)
		{
			if (InvSub->DragState.Source == EItemDragSource::Inventory)
			{
				// Drop from inventory to storage -> deposit
				InvSub->CompleteDrop(EItemDropTarget::StorageSlot);
			}
			else if (InvSub->DragState.Source == EItemDragSource::Cart)
			{
				// Drop from cart to storage -> cart deposit
				UStorageSubsystem* Sub = OwningSubsystem.Get();
				if (Sub)
				{
					Sub->DepositFromCart(InvSub->DragState.InventoryId, InvSub->DragState.Quantity);
				}
				InvSub->CancelDrag();
			}
			else if (InvSub->DragState.Source == EItemDragSource::Storage)
			{
				// Rearranging within storage — just cancel
				InvSub->CancelDrag();
			}
			else
			{
				InvSub->CancelDrag();
			}
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Handled();
}

FReply SStorageWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

	// Item drag detection
	if (bDragInitiated && DragSourceSlotIndex >= 0)
	{
		float Distance = FVector2D::Distance(ScreenPos, DragStartPos);
		if (Distance > DragThreshold)
		{
			UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
			UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
			if (InvSub)
			{
				FInventoryItem Item = GetItemAtSlot(DragSourceSlotIndex);
				if (Item.IsValid())
				{
					InvSub->StartDrag(Item, EItemDragSource::Storage);
				}
			}
			bDragInitiated = false;
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

FReply SStorageWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
	FInventoryItem Item = GetItemAtSlot(SlotIdx);

	if (!Item.IsValid()) return FReply::Handled();

	UStorageSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FReply::Handled();

	// Double-click: withdraw item to inventory
	Sub->WithdrawItem(Item.InventoryId, Item.Quantity);

	return FReply::Handled();
}

FReply SStorageWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GridScrollBox.IsValid())
	{
		float ScrollDelta = MouseEvent.GetWheelDelta() * -CellSize;
		GridScrollBox->SetScrollOffset(GridScrollBox->GetScrollOffset() + ScrollDelta);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FCursorReply SStorageWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
	UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
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

	int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, CursorEvent.GetScreenSpacePosition());
	FInventoryItem Item = GetItemAtSlot(SlotIdx);
	if (Item.IsValid())
	{
		return FCursorReply::Cursor(EMouseCursor::GrabHand);
	}

	return FCursorReply::Unhandled();
}
