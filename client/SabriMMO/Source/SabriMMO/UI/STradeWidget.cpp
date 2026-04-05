// STradeWidget.cpp — Dual-panel trade UI.
// Items displayed via TAttribute lambdas (auto-update, no rebuild needed).
// Drag-drop from inventory: checks InventorySubsystem::bIsDragging on click.
// Right-click own items to remove from trade.

#include "STradeWidget.h"
#include "TradeSubsystem.h"
#include "InventorySubsystem.h"
#include "ItemInspectSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace TWColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelMedium   (0.33f, 0.22f, 0.13f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor LockedGreen   (0.20f, 0.80f, 0.20f, 1.f);
	static const FLinearColor SlotBg        (0.18f, 0.12f, 0.07f, 1.f);
	static const FLinearColor SlotBorder    (0.35f, 0.25f, 0.12f, 1.f);
	static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
}

// ============================================================
// Helpers
// ============================================================

static FString FormatTradeItemName(const FTradeItem& Item)
{
	FString Result;
	if (Item.RefineLevel > 0)
		Result = FString::Printf(TEXT("+%d "), Item.RefineLevel);
	Result += Item.Name;
	if (Item.Quantity > 1)
		Result += FString::Printf(TEXT(" x%d"), Item.Quantity);
	return Result;
}

// ============================================================
// Construct
// ============================================================

void STradeWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(520.f)
		[
			// 3-layer RO frame: Gold -> Dark -> Brown
			SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(TWColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(TWColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(TWColors::PanelBrown)
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildTitleBar() ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildContent() ]
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildButtonRow() ]
						]
					]
				]
			]
	];
}

// ============================================================
// Title Bar
// ============================================================

TSharedRef<SWidget> STradeWidget::BuildTitleBar()
{
	TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(TWColors::PanelDark)
		.Padding(FMargin(6.f, 4.f))
		[
			SNew(STextBlock)
			.Text_Lambda([WeakSub]() -> FText {
				UTradeSubsystem* Sub = WeakSub.Get();
				if (!Sub) return FText::FromString(TEXT("Trade"));
				return FText::FromString(FString::Printf(TEXT("Trade with %s"), *Sub->PartnerName));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(TWColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(TWColors::TextShadow)
		];
}

// ============================================================
// Content: Two panels side by side
// ============================================================

TSharedRef<SWidget> STradeWidget::BuildContent()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4)
		[ BuildItemPanel(true) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4)
		[ BuildItemPanel(false) ];
}

TSharedRef<SWidget> STradeWidget::BuildItemPanel(bool bIsMyPanel)
{
	TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox)

		// Header row with lock indicator
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(bIsMyPanel ? TEXT("Your Offer") : TEXT("Partner's Offer")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(TWColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(TWColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[ BuildLockIndicator(bIsMyPanel) ]
		]

		// Gold divider
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(1.f).Padding(FMargin(0, 1, 0, 1))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(TWColors::GoldDivider)
			]
		];

	// 10 item slots (5 rows x 2 columns)
	for (int32 Row = 0; Row < 5; ++Row)
	{
		Panel->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[ BuildItemSlot(Row * 2, bIsMyPanel) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[ BuildItemSlot(Row * 2 + 1, bIsMyPanel) ]
		];
	}

	// Zeny row
	Panel->AddSlot().AutoHeight().Padding(0, 4, 0, 2)
	[
		BuildZenySection(bIsMyPanel)
	];

	return Panel;
}

TSharedRef<SWidget> STradeWidget::BuildLockIndicator(bool bIsMyLock)
{
	TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

	return SNew(STextBlock)
		.Text_Lambda([WeakSub, bIsMyLock]() -> FText {
			UTradeSubsystem* Sub = WeakSub.Get();
			if (!Sub) return FText::GetEmpty();
			bool bLocked = bIsMyLock ? Sub->bMyLocked : Sub->bPartnerLocked;
			return bLocked ? FText::FromString(TEXT(" LOCKED")) : FText::GetEmpty();
		})
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		.ColorAndOpacity(FSlateColor(TWColors::LockedGreen))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(TWColors::TextShadow);
}

// ============================================================
// FTradeItem -> FInventoryItem (for ItemInspect popup)
// ============================================================

static FInventoryItem TradeItemToInventoryItem(const FTradeItem& T)
{
	FInventoryItem I;
	I.InventoryId = T.InventoryId;
	I.ItemId = T.ItemId;
	I.Name = T.Name;
	I.Icon = T.Icon;
	I.Quantity = T.Quantity;
	I.RefineLevel = T.RefineLevel;
	I.Slots = T.Slots;
	I.ItemType = T.ItemType;
	I.EquipSlot = T.EquipSlot;
	I.Weight = T.Weight;
	I.bIdentified = T.bIdentified;
	I.CardPrefix = T.CardPrefix;
	I.CardSuffix = T.CardSuffix;
	I.WeaponLevel = T.WeaponLevel;
	return I;
}

// ============================================================
// Item Slot (icon + name, right-click inspect/remove)
// ============================================================

TSharedRef<SWidget> STradeWidget::BuildItemSlot(int32 SlotIndex, bool bIsMyItem)
{
	TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

	return SNew(SBox)
		.HeightOverride(28.f)
		.Padding(FMargin(1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(TWColors::SlotBorder)
			.Padding(FMargin(1.f))
			.OnMouseButtonDown_Lambda([WeakSub, SlotIndex, bIsMyItem](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() != EKeys::RightMouseButton)
					return FReply::Unhandled();

				UTradeSubsystem* Sub = WeakSub.Get();
				if (!Sub) return FReply::Unhandled();

				const TArray<FTradeItem>& Items = bIsMyItem ? Sub->MyItems : Sub->PartnerItems;
				if (SlotIndex >= Items.Num()) return FReply::Unhandled();

				if (bIsMyItem)
				{
					// Right-click own item: remove from trade
					if (!Sub->bMyLocked)
						Sub->RemoveItem(Items[SlotIndex].TradeSlot);
				}
				else
				{
					// Right-click partner item: open inspect popup
					if (UWorld* World = GEngine->GetCurrentPlayWorld())
					{
						if (UItemInspectSubsystem* InspectSub = World->GetSubsystem<UItemInspectSubsystem>())
						{
							FInventoryItem InvItem = TradeItemToInventoryItem(Items[SlotIndex]);
							InspectSub->ShowInspect(InvItem);
						}
					}
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(TWColors::SlotBg)
				.Padding(FMargin(2.f, 1.f))
				[
					SNew(SHorizontalBox)

					// Icon (20x20)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 3, 0)
					[
						SNew(SBox)
						.WidthOverride(20.f)
						.HeightOverride(20.f)
						[
							SNew(SImage)
							.Image(TAttribute<const FSlateBrush*>::CreateLambda(
								[WeakSub, SlotIndex, bIsMyItem]() -> const FSlateBrush*
								{
									UTradeSubsystem* Sub = WeakSub.Get();
									if (!Sub) return nullptr;
									const TArray<FTradeItem>& Items = bIsMyItem ? Sub->MyItems : Sub->PartnerItems;
									if (SlotIndex >= Items.Num()) return nullptr;
									UWorld* World = GEngine->GetCurrentPlayWorld();
									if (!World) return nullptr;
									UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
									if (!InvSub) return nullptr;
									return InvSub->GetOrCreateItemIconBrush(Items[SlotIndex].Icon);
								}))
						]
					]

					// Item name
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakSub, SlotIndex, bIsMyItem]() -> FText {
							UTradeSubsystem* Sub = WeakSub.Get();
							if (!Sub) return FText::GetEmpty();
							const TArray<FTradeItem>& Items = bIsMyItem ? Sub->MyItems : Sub->PartnerItems;
							if (SlotIndex >= Items.Num()) return FText::GetEmpty();
							return FText::FromString(FormatTradeItemName(Items[SlotIndex]));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FSlateColor(TWColors::TextPrimary))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(TWColors::TextShadow)
					]
				]
			]
		];
}

// ============================================================
// Zeny Section
// ============================================================

TSharedRef<SWidget> STradeWidget::BuildZenySection(bool bIsMine)
{
	TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

	// Build the value widget: editable input for our side, read-only text for partner
	TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
	if (bIsMine)
	{
		ValueWidget = SAssignNew(ZenyInputBox, SEditableTextBox)
			.Text(FText::FromString(TEXT("0")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.OnTextCommitted_Lambda([WeakSub](const FText& Text, ETextCommit::Type)
			{
				UTradeSubsystem* Sub = WeakSub.Get();
				if (!Sub) return;
				int64 Amount = FCString::Atoi64(*Text.ToString());
				Amount = FMath::Clamp(Amount, (int64)0, (int64)999999999);
				Sub->SetZeny(Amount);
			})
			.IsReadOnly_Lambda([WeakSub]() -> bool {
				UTradeSubsystem* Sub = WeakSub.Get();
				return !Sub || Sub->bMyLocked;
			});
	}
	else
	{
		ValueWidget = SNew(STextBlock)
			.Text_Lambda([WeakSub]() -> FText {
				UTradeSubsystem* Sub = WeakSub.Get();
				if (!Sub) return FText::FromString(TEXT("0"));
				return FText::FromString(FString::Printf(TEXT("%lld"), Sub->PartnerZeny));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(TWColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(TWColors::TextShadow);
	}

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Zeny:")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(TWColors::ZuzucoinGold))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(TWColors::TextShadow)
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[ ValueWidget ];
}

// ============================================================
// Button Row
// ============================================================

TSharedRef<SWidget> STradeWidget::BuildButtonRow()
{
	TWeakObjectPtr<UTradeSubsystem> WeakSub = OwningSubsystem;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(TWColors::PanelDark)
		.Padding(FMargin(6.f, 4.f))
		[
			SNew(SHorizontalBox)

			// OK button (lock offer)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(TEXT("OK")))
				.IsEnabled_Lambda([WeakSub]() -> bool {
					UTradeSubsystem* Sub = WeakSub.Get();
					return Sub && Sub->State == ETradeState::Open && !Sub->bMyLocked;
				})
				.OnClicked_Lambda([WeakSub]() -> FReply {
					if (UTradeSubsystem* Sub = WeakSub.Get())
						Sub->Lock();
					return FReply::Handled();
				})
			]

			// Trade button (finalize)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(TEXT("Trade")))
				.IsEnabled_Lambda([WeakSub]() -> bool {
					UTradeSubsystem* Sub = WeakSub.Get();
					return Sub && Sub->State == ETradeState::BothLocked;
				})
				.OnClicked_Lambda([WeakSub]() -> FReply {
					if (UTradeSubsystem* Sub = WeakSub.Get())
						Sub->Confirm();
					return FReply::Handled();
				})
			]

			// Cancel button (always available)
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(FText::FromString(TEXT("Cancel")))
				.OnClicked_Lambda([WeakSub]() -> FReply {
					if (UTradeSubsystem* Sub = WeakSub.Get())
						Sub->Cancel();
					return FReply::Handled();
				})
			]
		];
}

// ============================================================
// Mouse handlers (inventory drag-drop + escape)
// ============================================================

FReply STradeWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Accept item drag from inventory
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
			{
				if (InvSub->bIsDragging && InvSub->DragState.IsValid())
				{
					UTradeSubsystem* Sub = OwningSubsystem.Get();
					if (Sub && !Sub->bMyLocked && Sub->State == ETradeState::Open)
					{
						Sub->AddItem(InvSub->DragState.InventoryId, InvSub->DragState.Quantity);
					}
					InvSub->CancelDrag();
					return FReply::Handled();
				}
			}
		}
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply STradeWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Check for drag-drop on mouse up (release after drag)
	if (UWorld* World = GEngine->GetCurrentPlayWorld())
	{
		if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
		{
			if (InvSub->bIsDragging && InvSub->DragState.IsValid())
			{
				UTradeSubsystem* Sub = OwningSubsystem.Get();
				if (Sub && !Sub->bMyLocked && Sub->State == ETradeState::Open)
				{
					Sub->AddItem(InvSub->DragState.InventoryId, InvSub->DragState.Quantity);
				}
				InvSub->CancelDrag();
				return FReply::Handled();
			}
		}
	}

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply STradeWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		if (UTradeSubsystem* Sub = OwningSubsystem.Get())
			Sub->Cancel();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
