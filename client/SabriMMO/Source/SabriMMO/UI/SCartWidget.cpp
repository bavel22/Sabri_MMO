// SCartWidget.cpp — RO Classic cart inventory window: scrollable grid, drag-and-drop,
// double-click to move items to inventory, weight bar, item tooltips.

#include "SCartWidget.h"
#include "CartSubsystem.h"
#include "InventorySubsystem.h"
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
#include "Widgets/SToolTip.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Colors (same brown/gold palette as other widgets)
// ============================================================

namespace CartColors
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
	// Item type colors for placeholder icons
	static const FLinearColor ConsumableRed (0.70f, 0.20f, 0.15f, 1.f);
	static const FLinearColor WeaponOrange  (0.75f, 0.50f, 0.15f, 1.f);
	static const FLinearColor ArmorBlue     (0.20f, 0.35f, 0.65f, 1.f);
	static const FLinearColor EtcGreen      (0.25f, 0.55f, 0.25f, 1.f);
	static const FLinearColor CardPurple    (0.55f, 0.25f, 0.60f, 1.f);
	static const FLinearColor WeightBarBg   (0.12f, 0.08f, 0.04f, 1.f);
	static const FLinearColor WeightBarFill (0.72f, 0.58f, 0.28f, 1.f);
}

static constexpr float CellSize = 34.f;   // 32 + 2 border
static constexpr float IconSize = 28.f;

// ============================================================
// Construction
// ============================================================

void SCartWidget::Construct(const FArguments& InArgs)
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
			.BorderBackgroundColor(CartColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CartColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CartColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(CartColors::GoldDivider) ]
						]
						// Weight bar
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildWeightBar() ]
						// Gold divider
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 1.f))
							[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(CartColors::GoldDivider) ]
						]
						// Grid area
						+ SVerticalBox::Slot().FillHeight(1.f)
						[ BuildGridArea() ]
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

void SCartWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UCartSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Rebuild grid when data version changes
	uint32 CurrentVersion = Sub->DataVersion;
	if (CurrentVersion != LastDataVersion)
	{
		LastDataVersion = CurrentVersion;
		RebuildGrid();
	}

	// Update drag cursor position to follow mouse (uses InventorySubsystem's drag cursor)
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

TSharedRef<SWidget> SCartWidget::BuildTitleBar()
{
	UCartSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Cart (F10)")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(CartColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CartColors::TextShadow)
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
						Sub->HideWidget();
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(CartColors::TextDim))
				]
			]
		];
}

// ============================================================
// Weight bar — "Weight: 1234 / 8000" with progress bar fill
// ============================================================

TSharedRef<SWidget> SCartWidget::BuildWeightBar()
{
	UCartSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(18.f)
		.Padding(FMargin(4.f, 2.f))
		[
			SNew(SOverlay)
			// Background bar
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CartColors::WeightBarBg)
			]
			// Gold fill bar (width based on weight ratio)
			+ SOverlay::Slot().HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride_Lambda([Sub]() -> FOptionalSize {
					if (!Sub || Sub->CartMaxWeight <= 0) return 0.f;
					float Ratio = FMath::Clamp((float)Sub->CartWeight / (float)Sub->CartMaxWeight, 0.f, 1.f);
					// Approximate fill width within the padding area
					return FOptionalSize(Ratio * 370.f);
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CartColors::WeightBarFill)
				]
			]
			// Weight text overlay
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::GetEmpty();
					return FText::FromString(FString::Printf(TEXT("Weight: %d / %d"), Sub->CartWeight, Sub->CartMaxWeight));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CartColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CartColors::TextShadow)
			]
		];
}

// ============================================================
// Grid area (scrollable item grid)
// ============================================================

TSharedRef<SWidget> SCartWidget::BuildGridArea()
{
	return SAssignNew(GridScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible)
		+ SScrollBox::Slot()
		[
			SAssignNew(GridContainer, SVerticalBox)
		];
}

void SCartWidget::RebuildGrid()
{
	if (!GridContainer.IsValid()) return;
	GridContainer->ClearChildren();

	UCartSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Build rows of GridColumns cells each (minimum 3 visible rows)
	int32 TotalSlots = FMath::Max(Sub->CartItems.Num(), GridColumns * 3);
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
			bool bHasItem = ItemIndex < Sub->CartItems.Num();

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
						.BorderBackgroundColor(CartColors::SlotBg)
					]
				];
			}
		}
	}
}

// ============================================================
// Individual item slot
// ============================================================

TSharedRef<SWidget> SCartWidget::BuildItemSlot(int32 SlotIndex)
{
	FInventoryItem Item = GetItemAtSlot(SlotIndex);

	// Pre-build icon widget: texture icon if available, colored placeholder otherwise
	TSharedRef<SWidget> IconWidget = [&]() -> TSharedRef<SWidget>
	{
		// Use InventorySubsystem's shared icon brush cache
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
		// Fallback: colored square + 2-letter text
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(Item.IsValid() ? GetItemTypeColor(Item.ItemType) : FLinearColor::Transparent)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Item.IsValid() && Item.Name.Len() > 0 ? FText::FromString(Item.Name.Left(2)) : FText::GetEmpty())
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(CartColors::TextBright))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CartColors::TextShadow)
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
				// Dim the border when this slot is being dragged
				if (DragSourceSlotIndex == SlotIndex && bDragInitiated)
					return FSlateColor(FLinearColor(0.25f, 0.18f, 0.10f, 0.5f));
				return FSlateColor(CartColors::SlotBorder);
			})
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CartColors::SlotBg)
				[
					SNew(SOverlay)
					// Item icon (texture or colored placeholder)
					+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(IconSize)
						.HeightOverride(IconSize)
						[
							IconWidget
						]
					]
					// Quantity badge (bottom-right)
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
						.ColorAndOpacity(FSlateColor(CartColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(CartColors::TextShadow)
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

	// Hover tooltip for filled slots
	if (Item.IsValid())
	{
		SlotBox->SetToolTip(SNew(SToolTip)[ ItemTooltipBuilder::Build(Item) ]);
	}

	return SlotBox;
}

// ============================================================
// Helpers
// ============================================================

FInventoryItem SCartWidget::GetItemAtSlot(int32 SlotIndex) const
{
	UCartSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FInventoryItem();

	if (SlotIndex >= 0 && SlotIndex < Sub->CartItems.Num())
		return Sub->CartItems[SlotIndex];
	return FInventoryItem();
}

FLinearColor SCartWidget::GetItemTypeColor(const FString& ItemType) const
{
	if (ItemType == TEXT("consumable")) return CartColors::ConsumableRed;
	if (ItemType == TEXT("weapon"))     return CartColors::WeaponOrange;
	if (ItemType == TEXT("armor"))      return CartColors::ArmorBlue;
	if (ItemType == TEXT("card"))       return CartColors::CardPurple;
	return CartColors::EtcGreen;
}

int32 SCartWidget::GetSlotIndexFromPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	// Calculate which grid slot the mouse is over
	// Grid starts after: 3px borders + 20px title + 1px divider + 18px weight + 1px divider = ~43px top,
	//                    3px borders = 3px left
	FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	float GridLeft = 5.f;
	float GridTop = 45.f;

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

FVector2D SCartWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return CurrentSize;
}

void SCartWidget::ApplyLayout()
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

FReply SCartWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

		// Check if clicking on a grid item — start potential drag
		int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
		FInventoryItem Item = GetItemAtSlot(SlotIdx);
		if (Item.IsValid())
		{
			bDragInitiated = true;
			DragStartPos = ScreenPos;
			DragSourceSlotIndex = SlotIdx;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}

	// Right-click: open item inspect
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
		FInventoryItem Item = GetItemAtSlot(SlotIdx);
		if (Item.IsValid())
		{
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

FReply SCartWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}

		// If InventorySubsystem is dragging (actual item drag), handle the drop
		UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
		UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
		if (InvSub && InvSub->bIsDragging)
		{
			if (InvSub->DragState.Source == EItemDragSource::Inventory)
			{
				// Drop from inventory to cart
				InvSub->CompleteDrop(EItemDropTarget::CartSlot);
			}
			else if (InvSub->DragState.Source == EItemDragSource::Cart)
			{
				// Rearranging within cart — just cancel
				InvSub->CancelDrag();
			}
			else
			{
				// Other source (equipment, etc.) — cancel
				InvSub->CancelDrag();
			}
			DragSourceSlotIndex = -1;
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Handled();
}

FReply SCartWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
	if (bDragInitiated && DragSourceSlotIndex >= 0)
	{
		float Distance = FVector2D::Distance(ScreenPos, DragStartPos);
		if (Distance > DragThreshold)
		{
			// Start actual drag via InventorySubsystem (shared drag cursor)
			UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
			UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
			if (InvSub)
			{
				FInventoryItem Item = GetItemAtSlot(DragSourceSlotIndex);
				if (Item.IsValid())
				{
					InvSub->StartDrag(Item, EItemDragSource::Cart);
				}
			}
			bDragInitiated = false;
			DragSourceSlotIndex = -1;
			// Release mouse capture so other widgets (inventory, equipment) can
			// receive the mouse-up event and handle the drop directly.
			return FReply::Handled().ReleaseMouseCapture();
		}
	}

	return FReply::Unhandled();
}

FReply SCartWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	int32 SlotIdx = GetSlotIndexFromPosition(MyGeometry, ScreenPos);
	FInventoryItem Item = GetItemAtSlot(SlotIdx);

	if (!Item.IsValid()) return FReply::Handled();

	UCartSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return FReply::Handled();

	// Double-click: move item to inventory
	Sub->MoveToInventory(Item.InventoryId, Item.Quantity);

	return FReply::Handled();
}

FReply SCartWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (GridScrollBox.IsValid())
	{
		float ScrollDelta = MouseEvent.GetWheelDelta() * -CellSize;
		GridScrollBox->SetScrollOffset(GridScrollBox->GetScrollOffset() + ScrollDelta);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FCursorReply SCartWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Hide hardware cursor during item drag — the drag icon replaces it
	UWorld* World = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
	UInventorySubsystem* InvSub = World ? World->GetSubsystem<UInventorySubsystem>() : nullptr;
	if (InvSub && InvSub->bIsDragging)
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
