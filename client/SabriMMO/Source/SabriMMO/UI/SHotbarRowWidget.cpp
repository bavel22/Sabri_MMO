// SHotbarRowWidget.cpp — Single hotbar row (9 slots) with RO Classic theme.

#include "SHotbarRowWidget.h"
#include "HotbarSubsystem.h"
#include "InventorySubsystem.h"
#include "SkillTreeSubsystem.h"
#include "ItemTooltipBuilder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameViewportClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogHotbarWidget, Log, All);

// ============================================================
// RO Classic colors (same namespace as other widgets)
// ============================================================

namespace HotbarColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium   (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldDark      (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor SlotBg        (0.15f, 0.10f, 0.06f, 1.f);
	static const FLinearColor SlotBorder    (0.40f, 0.30f, 0.15f, 1.f);
	static const FLinearColor SlotHover     (0.71f, 1.0f, 0.71f, 0.25f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor CooldownOverlay(0.00f, 0.00f, 0.00f, 0.6f);
	static const FLinearColor HandleBg      (0.30f, 0.20f, 0.10f, 1.f);
	static const FLinearColor GearColor     (0.60f, 0.50f, 0.30f, 1.f);
}

// ============================================================
// Construct
// ============================================================

void SHotbarRowWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	RowIndex = InArgs._RowIndex;

	// Default position: bottom-center, stacked upward per row index
	// Will be refined in first Tick when we know viewport size
	WidgetPosition = FVector2D(400.0, 600.0 - (RowIndex * (RowHeight + 4.0)));

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(TotalWidth)
		.HeightOverride(RowHeight)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(HotbarColors::GoldTrim)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(HotbarColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(HotbarColors::PanelBrown)
					.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
					[
						SNew(SHorizontalBox)

						// Handle (drag area + row number)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(HandleWidth)
							.HeightOverride(SlotSize)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(HotbarColors::HandleBg)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::AsNumber(RowIndex + 1))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(FSlateColor(HotbarColors::GoldHighlight))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(HotbarColors::TextShadow)
								]
							]
						]

						// Slot container
						+ SHorizontalBox::Slot().AutoWidth().Padding(2.f, 0.f, 0.f, 0.f)
						[
							SAssignNew(SlotContainer, SHorizontalBox)
						]

						// Gear icon (open keybind config)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f, 0.f, 0.f)
						[
							SNew(SBox)
							.WidthOverride(GearWidth)
							.HeightOverride(GearWidth)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(HotbarColors::GearColor)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.Cursor(EMouseCursor::Hand)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("\u2699")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
									.ColorAndOpacity(FSlateColor(HotbarColors::PanelDark))
								]
							]
						]
					]
				]
			]
		]
	];

	// Build initial slots
	RebuildSlots();
	ApplyLayout();
}

// ============================================================
// Build individual slot
// ============================================================

TSharedRef<SWidget> SHotbarRowWidget::BuildSlot(int32 SlotIndex)
{
	UHotbarSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.WidthOverride(SlotSize)
		.HeightOverride(SlotSize)
		.Padding(FMargin(1.f))
		.ToolTipText_Lambda([this, SlotIndex, Sub]() -> FText
		{
			if (!Sub) return FText::GetEmpty();
			const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);
			if (Slot.IsItem() && !Slot.ItemName.IsEmpty())
				return FText::FromString(Slot.ItemName);
			if (Slot.IsSkill() && !Slot.SkillName.IsEmpty())
				return FText::FromString(Slot.SkillName);
			return FText::GetEmpty();
		})
		[
			// Slot border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(HotbarColors::SlotBorder)
			.Padding(FMargin(1.f))
			[
				// Dark slot background with overlay layers
				SNew(SOverlay)

				// Layer 0: Dark background
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(HotbarColors::SlotBg)
				]

				// Layer 1: Hover highlight
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor_Lambda([this, SlotIndex]() -> FSlateColor
					{
						return (HoveredSlotIndex == SlotIndex)
							? FSlateColor(HotbarColors::SlotHover)
							: FSlateColor(FLinearColor::Transparent);
					})
				]

				// Layer 2: Icon (item or skill)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(IconSize)
					.HeightOverride(IconSize)
					[
						SNew(SImage)
						.Image_Lambda([this, SlotIndex, Sub]() -> const FSlateBrush*
						{
							if (!Sub) return nullptr;
							const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);

							if (Slot.IsSkill() && !Slot.SkillIcon.IsEmpty())
							{
								return Sub->GetSkillIconBrush(Slot.SkillIcon);
							}
							else if (Slot.IsItem() && !Slot.ItemIcon.IsEmpty())
							{
								return Sub->GetItemIconBrush(Slot.ItemIcon);
							}
							return nullptr;
						})
					]
				]

				// Layer 3: Keybind label (top-left)
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding(FMargin(1.f, 0.f, 0.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this, SlotIndex, Sub]() -> FText
					{
						if (!Sub) return FText::GetEmpty();
						FString Display = Sub->GetKeybindDisplayString(RowIndex, SlotIndex);
						return FText::FromString(Display);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(HotbarColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(HotbarColors::TextShadow)
				]

				// Layer 4: Quantity badge (bottom-right, items only)
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(FMargin(0.f, 0.f, 2.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this, SlotIndex, Sub]() -> FText
					{
						if (!Sub) return FText::GetEmpty();
						const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);
						if (Slot.IsItem() && Slot.Quantity > 1)
						{
							return FText::AsNumber(Slot.Quantity);
						}
						return FText::GetEmpty();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(HotbarColors::TextBright))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(HotbarColors::TextShadow)
				]

				// Layer 4b: Skill level badge (top-right, skills only)
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				.Padding(FMargin(0.f, 1.f, 2.f, 0.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this, SlotIndex, Sub]() -> FText
					{
						if (!Sub) return FText::GetEmpty();
						const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);
						if (Slot.IsSkill() && Slot.SkillLevel > 0)
							return FText::FromString(FString::Printf(TEXT("Lv%d"), Slot.SkillLevel));
						return FText::GetEmpty();
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.8f, 0.3f, 1.0f)))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(HotbarColors::TextShadow)
				]

				// Layer 5: Cooldown overlay (skills only)
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor_Lambda([this, SlotIndex, Sub]() -> FSlateColor
					{
						if (!Sub) return FSlateColor(FLinearColor::Transparent);
						const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);
						if (Slot.IsSkill() && Slot.SkillId > 0)
						{
							UWorld* World = Sub->GetWorld();
							if (World)
							{
								if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
								{
									if (SkillSub->IsSkillOnCooldown(Slot.SkillId))
									{
										return FSlateColor(HotbarColors::CooldownOverlay);
									}
								}
							}
						}
						return FSlateColor(FLinearColor::Transparent);
					})
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						// Cooldown remaining text
						SNew(STextBlock)
						.Text_Lambda([this, SlotIndex, Sub]() -> FText
						{
							if (!Sub) return FText::GetEmpty();
							const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIndex);
							if (Slot.IsSkill() && Slot.SkillId > 0)
							{
								UWorld* World = Sub->GetWorld();
								if (World)
								{
									if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
									{
										float Remaining = SkillSub->GetSkillCooldownRemaining(Slot.SkillId);
										if (Remaining > 0.f)
										{
											return FText::FromString(FString::Printf(TEXT("%.1f"), Remaining));
										}
									}
								}
							}
							return FText::GetEmpty();
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(HotbarColors::TextBright))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(HotbarColors::TextShadow)
					]
				]
			]
		];
}

// ============================================================
// Rebuild all slots
// ============================================================

void SHotbarRowWidget::RebuildSlots()
{
	if (!SlotContainer.IsValid()) return;
	SlotContainer->ClearChildren();

	for (int32 i = 0; i < UHotbarSubsystem::SLOTS_PER_ROW; ++i)
	{
		SlotContainer->AddSlot()
			.AutoWidth()
			[
				BuildSlot(i)
			];
	}
}

// ============================================================
// Tick — refresh items, detect hover, handle drops
// ============================================================

void SHotbarRowWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UHotbarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Detect data version changes and update item quantities
	if (Sub->DataVersion != LastDataVersion)
	{
		LastDataVersion = Sub->DataVersion;
		// Slots auto-refresh via lambdas, but we can trigger a full rebuild if needed
	}

	// Refresh item quantities periodically (inventory changes don't trigger hotbar events)
	QuantityRefreshTimer += InDeltaTime;
	if (QuantityRefreshTimer > 1.0f)
	{
		QuantityRefreshTimer = 0.f;
		Sub->RefreshItemQuantities();
	}

	// Update skill drag cursor to follow mouse (same pattern as inventory drag cursor)
	{
		UWorld* World = Sub->GetWorld();
		if (World)
		{
			if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
			{
				if (SkillSub->bSkillDragging)
				{
					SkillSub->UpdateSkillDragCursorPosition();
				}
			}
		}
	}

	// Safety cleanup: cancel orphaned drags when mouse button is no longer held.
	// This handles the case where the user releases the mouse over empty viewport space
	// and no widget receives the mouse-up event.
	if (FSlateApplication::IsInitialized())
	{
		const bool bMouseDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);
		if (!bMouseDown)
		{
			UWorld* World = Sub->GetWorld();
			if (World)
			{
				if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
				{
					if (InvSub->bIsDragging)
					{
						InvSub->CancelDrag();
					}
				}
				if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
				{
					if (SkillSub->bSkillDragging)
					{
						SkillSub->CancelSkillDrag();
					}
				}
			}
		}
	}

	// Update default position on first tick (when we know viewport size)
	if (!bPositionInitialized)
	{
		FVector2D ViewportSize = FVector2D(1920, 1080);
		// Use per-world viewport (not GEngine->GameViewport which is global/wrong in multi-PIE)
		UHotbarSubsystem* SubPtr = OwningSubsystem.Get();
		if (SubPtr)
		{
			if (UWorld* World = SubPtr->GetWorld())
			{
				if (UGameViewportClient* VC = World->GetGameViewport())
				{
					FVector2D ViewSz;
					VC->GetViewportSize(ViewSz);
					if (ViewSz.X > 0) ViewportSize = ViewSz;
				}
			}
		}
		// Top-left quadrant, stacked down per row
		const double RowSpacing = RowHeight + 4.0;
		WidgetPosition.X = ViewportSize.X * 0.05;
		WidgetPosition.Y = ViewportSize.Y * 0.15 + ((3 - RowIndex) * RowSpacing);
		ApplyLayout();
		bPositionInitialized = true;

		UE_LOG(LogHotbarWidget, Log, TEXT("Row%d positioned: (%.0f, %.0f) viewport=(%.0f, %.0f) World=%p"),
			RowIndex, WidgetPosition.X, WidgetPosition.Y, ViewportSize.X, ViewportSize.Y,
			SubPtr ? SubPtr->GetWorld() : nullptr);
	}
}

// ============================================================
// Geometry helpers
// ============================================================

FVector2D SHotbarRowWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return FVector2D(TotalWidth, RowHeight);
}

int32 SHotbarRowWidget::GetSlotIndexAtPosition(const FGeometry& MyGeometry, const FVector2D& ScreenPos) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

	// Slot area starts after handle
	float SlotStartX = HandleWidth + 2.f + 1.f + 1.f + 2.f; // handle + padding + borders
	float SlotY = 2.f + 1.f + 1.f + 2.f; // top borders/padding

	for (int32 i = 0; i < UHotbarSubsystem::SLOTS_PER_ROW; ++i)
	{
		float SlotX = SlotStartX + (i * SlotSize);
		if (LocalPos.X >= SlotX && LocalPos.X < SlotX + SlotSize &&
			LocalPos.Y >= SlotY && LocalPos.Y < SlotY + SlotSize)
		{
			return i;
		}
	}
	return -1;
}

// ============================================================
// Mouse handlers
// ============================================================

FReply SHotbarRowWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D ContentBounds = GetContentSize();

	// Outside content bounds — pass through
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
	{
		return FReply::Unhandled();
	}

	UHotbarSubsystem* Sub = OwningSubsystem.Get();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Check if clicking the gear icon (rightmost area)
		float GearStartX = ContentBounds.X - GearWidth - 4.f;
		if (LocalPos.X >= GearStartX && Sub)
		{
			Sub->ToggleKeybindWidget();
			return FReply::Handled();
		}

		// Check if clicking a slot
		int32 SlotIdx = GetSlotIndexAtPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());

		// Check if a drag is active (item or skill) — this click completes the drop
		if (SlotIdx >= 0 && Sub)
		{
			UWorld* World = Sub->GetWorld();
			if (World)
			{
				// Item drag from inventory
				if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
				{
					if (InvSub->bIsDragging && InvSub->DragState.IsValid())
					{
						if (InvSub->DragState.ItemType == TEXT("consumable"))
						{
							FInventoryItem* FullItem = InvSub->FindItemByInventoryId(InvSub->DragState.InventoryId);
							if (FullItem)
							{
								Sub->AssignItem(RowIndex, SlotIdx, *FullItem);
							}
						}
						InvSub->CancelDrag();
						return FReply::Handled();
					}
				}

				// Skill drag from skill tree
				if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
				{
					if (SkillSub->bSkillDragging && SkillSub->DraggedSkillId > 0)
					{
						Sub->AssignSkill(RowIndex, SlotIdx,
							SkillSub->DraggedSkillId,
							SkillSub->DraggedSkillName,
							SkillSub->DraggedSkillIcon,
							SkillSub->DraggedSkillLevel);
						SkillSub->CancelSkillDrag();
						return FReply::Handled();
					}
				}
			}
		}

		// Click on a filled slot — activate it
		if (SlotIdx >= 0 && Sub)
		{
			const FHotbarSlot& Slot = Sub->GetSlot(RowIndex, SlotIdx);
			if (!Slot.IsEmpty())
			{
				Sub->ActivateSlot(RowIndex, SlotIdx);
				return FReply::Handled();
			}
		}

		// Click on handle area (left side) — start drag
		if (LocalPos.X < HandleWidth + 4.f)
		{
			bIsDragging = true;
			DragOffset = MouseEvent.GetScreenSpacePosition();
			DragStartWidgetPos = WidgetPosition;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// Right-click on a slot — clear it
		int32 SlotIdx = GetSlotIndexAtPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (SlotIdx >= 0 && Sub)
		{
			Sub->ClearSlot(RowIndex, SlotIdx);
			return FReply::Handled();
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SHotbarRowWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	// Handle drops on mouse up (items from inventory or skills from skill tree)
	UHotbarSubsystem* Sub = OwningSubsystem.Get();
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Sub)
	{
		int32 SlotIdx = GetSlotIndexAtPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());
		if (SlotIdx >= 0)
		{
			UWorld* World = Sub->GetWorld();
			if (World)
			{
				// Check for inventory item drag
				if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
				{
					if (InvSub->bIsDragging && InvSub->DragState.IsValid())
					{
						if (InvSub->DragState.ItemType == TEXT("consumable"))
						{
							FInventoryItem* FullItem = InvSub->FindItemByInventoryId(InvSub->DragState.InventoryId);
							if (FullItem)
							{
								Sub->AssignItem(RowIndex, SlotIdx, *FullItem);
							}
						}
						// Only cancel drag for items we accept (consumable).
						// Non-consumable items: cancel too (item stays in inventory).
						InvSub->CancelDrag();
						return FReply::Handled();
					}
				}

				// Check for skill drag from skill tree
				if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
				{
					if (SkillSub->bSkillDragging && SkillSub->DraggedSkillId > 0)
					{
						Sub->AssignSkill(RowIndex, SlotIdx,
							SkillSub->DraggedSkillId,
							SkillSub->DraggedSkillName,
							SkillSub->DraggedSkillIcon,
							SkillSub->DraggedSkillLevel);
						SkillSub->CancelSkillDrag();
						return FReply::Handled();
					}
				}
			}
		}
	}

	return FReply::Unhandled();
}

FReply SHotbarRowWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Update hover tracking
	int32 NewHover = GetSlotIndexAtPosition(MyGeometry, MouseEvent.GetScreenSpacePosition());
	if (NewHover != HoveredSlotIndex)
	{
		HoveredSlotIndex = NewHover;
	}

	// Handle window drag
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FCursorReply SHotbarRowWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Hide hardware cursor during item or skill drag — the drag icon replaces it
	UHotbarSubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		UWorld* World = Sub->GetWorld();
		if (World)
		{
			if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
			{
				if (InvSub->bIsDragging)
					return FCursorReply::Cursor(EMouseCursor::None);
			}
			if (USkillTreeSubsystem* SkillSub = World->GetSubsystem<USkillTreeSubsystem>())
			{
				if (SkillSub->bSkillDragging)
					return FCursorReply::Cursor(EMouseCursor::None);
			}
		}
	}
	return FCursorReply::Unhandled();
}

// ============================================================
// Layout
// ============================================================

void SHotbarRowWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}
