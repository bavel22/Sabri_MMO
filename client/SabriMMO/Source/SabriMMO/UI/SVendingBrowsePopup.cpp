// SVendingBrowsePopup.cpp — Modal popup for browsing another player's vending shop.
// Each row: icon + name (with refine/slots) + amount + price + Buy button.

#include "SVendingBrowsePopup.h"
#include "VendingSubsystem.h"
#include "InventorySubsystem.h"
#include "BasicInfoSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SNullWidget.h"
#include "Styling/CoreStyle.h"

// RO Classic brown/gold theme colors
namespace VendBrowseColors
{
	static const FLinearColor Backdrop      (0.f, 0.f, 0.f, 0.4f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextDim       (0.70f, 0.62f, 0.48f, 1.f);
	static const FLinearColor TextShadow    (0.f, 0.f, 0.f, 0.85f);
	static const FLinearColor RowBg         (0.18f, 0.12f, 0.07f, 1.f);
	static const FLinearColor SuccessGreen  (0.30f, 0.85f, 0.30f, 1.f);
	static const FLinearColor ErrorRed      (0.95f, 0.25f, 0.20f, 1.f);
	static const FLinearColor ButtonBrown   (0.35f, 0.24f, 0.14f, 1.f);
	static const FLinearColor GoldPrice     (0.90f, 0.75f, 0.10f, 1.f);
	static const FLinearColor AmountBlue    (0.55f, 0.75f, 0.95f, 1.f);
}

static constexpr float BrowsePopupWidth = 350.f;
static constexpr float BrowseMaxListHeight = 300.f;
static constexpr float BrowseIconSize = 24.f;

void SVendingBrowsePopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		// Fullscreen backdrop — click outside dismisses
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(VendBrowseColors::Backdrop)
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
			// Popup box — stops click propagation
			SNew(SBox)
			.WidthOverride(BrowsePopupWidth)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(VendBrowseColors::Backdrop)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) -> FReply
				{
					return FReply::Handled();
				})
				[
					// 3-layer RO frame: Gold -> Dark -> Brown
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(VendBrowseColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(VendBrowseColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(VendBrowseColors::PanelBrown)
							.Padding(FMargin(6.f))
							[
								SNew(SVerticalBox)

								// Title bar
								+ SVerticalBox::Slot().AutoHeight()
								[ BuildTitleBar() ]

								// Gold divider
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
								[
									SNew(SBox).HeightOverride(1.f)
									[
										SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
										.BorderBackgroundColor(VendBrowseColors::GoldDivider)
									]
								]

								// Shop title subtitle
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SNew(STextBlock)
									.Text_Lambda([this]() -> FText
									{
										UVendingSubsystem* Sub = OwningSubsystem.Get();
										return FText::FromString(Sub ? Sub->BrowseShopTitle : TEXT(""));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(VendBrowseColors::TextDim))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
									.AutoWrapText(true)
								]

								// Scrollable item list
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.MaxDesiredHeight(BrowseMaxListHeight)
									[
										SNew(SScrollBox)
										+ SScrollBox::Slot()
										[
											SAssignNew(ItemListContainer, SVerticalBox)
										]
									]
								]

								// Sale log (vendor mode only — shows buyer notifications)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SNew(SBox)
									.MaxDesiredHeight(80.f)
									.Visibility_Lambda([this]() { return bIsVendorView ? EVisibility::Visible : EVisibility::Collapsed; })
									[
										SNew(SScrollBox)
										+ SScrollBox::Slot()
										[
											SAssignNew(SaleLogContainer, SVerticalBox)
										]
									]
								]

								// Bottom bar (zeny + status)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[ BuildBottomBar() ]
							]
						]
					]
				]
			]
		]
	];

	// Populate initial item list
	RebuildItemList();
}

// ── Title Bar ────────────────────────────────────────────────────

TSharedRef<SWidget> SVendingBrowsePopup::BuildTitleBar()
{
	return SNew(SHorizontalBox)
		// Vendor name title
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText
			{
				UVendingSubsystem* Sub = OwningSubsystem.Get();
				FString Name = Sub ? Sub->BrowseVendorName : TEXT("Unknown");
				return FText::FromString(FString::Printf(TEXT("%s's Shop"), *Name));
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(VendBrowseColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
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
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(VendBrowseColors::TextDim))
			]
		];
}

// ── Item List ────────────────────────────────────────────────────

TSharedRef<SWidget> SVendingBrowsePopup::BuildItemList()
{
	// ItemListContainer is already SAssigned in Construct — just populate it
	RebuildItemList();
	return SNullWidget::NullWidget;  // Container already in the hierarchy
}

void SVendingBrowsePopup::RebuildItemList()
{
	if (!ItemListContainer.IsValid()) return;
	ItemListContainer->ClearChildren();

	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	const TArray<FVendBrowseItem>& Items = bIsVendorView ? Sub->OwnShopItems : Sub->BrowseItems;
	for (const FVendBrowseItem& Item : Items)
	{
		if (Item.Amount <= 0) continue;  // Skip sold-out items
		ItemListContainer->AddSlot().AutoHeight().Padding(0, 1)
		[
			BuildItemRow(Item)
		];
	}
}

void SVendingBrowsePopup::UpdateItemAmount(int32 VendItemId, int32 NewAmount)
{
	// Update the data and rebuild
	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	TArray<FVendBrowseItem>& Items = bIsVendorView ? Sub->OwnShopItems : Sub->BrowseItems;
	for (FVendBrowseItem& Item : Items)
	{
		if (Item.VendItemId == VendItemId)
		{
			Item.Amount = NewAmount;
			break;
		}
	}
	RebuildItemList();
}

void SVendingBrowsePopup::AddSaleMessage(const FString& BuyerName, const FString& ItemName, int32 Amount, int32 Price)
{
	if (!SaleLogContainer.IsValid()) return;

	FString Msg = FString::Printf(TEXT("%s bought %s x%d (%sz)"), *BuyerName, *ItemName, Amount, *FormatPrice(Price));

	SaleLogContainer->AddSlot().AutoHeight().Padding(2, 1)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Msg))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
		.ColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.9f, 0.4f, 1.f))) // Green for sale notifications
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
		.AutoWrapText(true)
	];
}

TSharedRef<SWidget> SVendingBrowsePopup::BuildItemRow(const FVendBrowseItem& Item)
{
	// Build icon widget
	TSharedRef<SWidget> IconWidget = [&]() -> TSharedRef<SWidget>
	{
		UVendingSubsystem* Sub = OwningSubsystem.Get();
		if (Sub)
		{
			UWorld* World = Sub->GetWorld();
			if (World)
			{
				UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>();
				if (InvSub)
				{
					FSlateBrush* Brush = InvSub->GetOrCreateItemIconBrush(Item.Icon);
					if (Brush)
					{
						return SNew(SBox).WidthOverride(BrowseIconSize).HeightOverride(BrowseIconSize)
						[
							SNew(SImage).Image(Brush)
						];
					}
				}
			}
		}
		// Fallback: colored box
		return SNew(SBox).WidthOverride(BrowseIconSize).HeightOverride(BrowseIconSize)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(VendBrowseColors::GoldDivider)
		];
	}();

	FString DisplayName = FormatItemName(Item);
	FString AmountText = FString::Printf(TEXT("x%d"), Item.Amount);
	FString PriceText = FormatPrice(Item.Price);
	int32 CapturedVendItemId = Item.VendItemId;
	int32 CapturedMaxAmount = Item.Amount;

	// Shared pointer for quantity value (captured by buy lambda)
	TSharedPtr<int32> BuyQuantity = MakeShared<int32>(1);

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)

		// Icon
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[ IconWidget ]

		// Name
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(DisplayName))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(VendBrowseColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
			.AutoWrapText(false)
		]

		// Amount in stock
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(AmountText))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(VendBrowseColors::AmountBlue))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
		]

		// Price per unit
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(PriceText))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
			.ColorAndOpacity(FSlateColor(VendBrowseColors::GoldPrice))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
		];

	// Buyer mode: quantity input + Buy button
	if (!bIsVendorView)
	{
		// Quantity input (only if stackable: amount > 1)
		if (CapturedMaxAmount > 1)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SBox).WidthOverride(30.f)
				[
					SNew(SEditableTextBox)
					.Text(FText::AsNumber(1))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.Justification(ETextJustify::Center)
					.OnTextCommitted_Lambda([BuyQuantity, CapturedMaxAmount](const FText& NewText, ETextCommit::Type)
					{
						int32 Val = FCString::Atoi(*NewText.ToString());
						*BuyQuantity = FMath::Clamp(Val, 1, CapturedMaxAmount);
					})
				]
			];
		}

		// Buy button
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ContentPadding(FMargin(6, 2))
			.ButtonColorAndOpacity(VendBrowseColors::ButtonBrown)
			.OnClicked_Lambda([this, CapturedVendItemId, BuyQuantity]() -> FReply
			{
				OnBuyClicked(CapturedVendItemId, BuyQuantity.IsValid() ? *BuyQuantity : 1);
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Buy")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
				.ColorAndOpacity(FSlateColor(VendBrowseColors::TextPrimary))
			]
		];
	}

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(VendBrowseColors::RowBg)
		.Padding(FMargin(4.f, 3.f))
		[ Row ];
}

// ── Bottom Bar ───────────────────────────────────────────────────

TSharedRef<SWidget> SVendingBrowsePopup::BuildBottomBar()
{
	return SNew(SVerticalBox)

		// Gold divider
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
		[
			SNew(SBox).HeightOverride(1.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(VendBrowseColors::GoldDivider)
			]
		]

		// Player zeny display
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText
				{
					UVendingSubsystem* Sub = OwningSubsystem.Get();
					if (!Sub) return FText::GetEmpty();
					if (bIsVendorView)
					{
						// Vendor mode: read live zeny from BasicInfoSubsystem (updated by inventory:zeny_update)
						if (UWorld* World = Sub->GetWorld())
						{
							if (UBasicInfoSubsystem* BIS = World->GetSubsystem<UBasicInfoSubsystem>())
							{
								return FText::FromString(FString::Printf(TEXT("Your Zeny: %s"), *FormatPrice(BIS->Zuzucoin)));
							}
						}
					}
					return FText::FromString(FString::Printf(TEXT("Your Zeny: %s"), *FormatPrice(Sub->PlayerZeny)));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(VendBrowseColors::GoldPrice))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
			]
		]

		// Status message
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[
			SAssignNew(StatusTextBlock, STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(VendBrowseColors::SuccessGreen))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendBrowseColors::TextShadow)
			.AutoWrapText(true)
			.Visibility(EVisibility::Collapsed)
		]

		// Close Shop button (vendor mode only)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return bIsVendorView ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SButton)
				.ContentPadding(FMargin(8, 4))
				.ButtonColorAndOpacity(FLinearColor(0.6f, 0.15f, 0.15f, 1.f))  // Red tint
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() -> FReply
				{
					UVendingSubsystem* Sub = OwningSubsystem.Get();
					if (Sub)
					{
						Sub->StopVending();
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Close Shop")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 1.f)))
					.Justification(ETextJustify::Center)
				]
			]
		];
}

// ── Actions ──────────────────────────────────────────────────────

void SVendingBrowsePopup::OnBuyClicked(int32 VendItemId, int32 Quantity)
{
	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	Sub->BuyItem(Sub->BrowseVendorId, VendItemId, FMath::Max(1, Quantity));
}

// ── Status Message ───────────────────────────────────────────────

void SVendingBrowsePopup::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (!StatusTextBlock.IsValid()) return;

	StatusTextBlock->SetText(FText::FromString(Message));
	StatusTextBlock->SetColorAndOpacity(FSlateColor(
		bIsError ? VendBrowseColors::ErrorRed : VendBrowseColors::SuccessGreen));
	StatusTextBlock->SetVisibility(
		Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
}

// ── Helpers ──────────────────────────────────────────────────────

FString SVendingBrowsePopup::FormatItemName(const FVendBrowseItem& Item)
{
	FString Result;

	// Prefix with +refine if applicable
	if (Item.RefineLevel > 0)
	{
		Result = FString::Printf(TEXT("+%d "), Item.RefineLevel);
	}

	Result += Item.Name;

	// Suffix with [slots] if applicable
	if (Item.Slots > 0)
	{
		Result += FString::Printf(TEXT(" [%d]"), Item.Slots);
	}

	return Result;
}

FString SVendingBrowsePopup::FormatPrice(int32 Price)
{
	if (Price >= 1000000)
	{
		return FString::Printf(TEXT("%d,%03d,%03dz"), Price / 1000000, (Price / 1000) % 1000, Price % 1000);
	}
	if (Price >= 1000)
	{
		return FString::Printf(TEXT("%d,%03dz"), Price / 1000, Price % 1000);
	}
	return FString::Printf(TEXT("%dz"), Price);
}

// ── Dismiss ──────────────────────────────────────────────────────

void SVendingBrowsePopup::DismissPopup()
{
	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		if (bIsVendorView)
		{
			// Vendor closing their own shop view — close the vending shop entirely
			Sub->StopVending();
		}
		else
		{
			// Buyer closing browse window
			Sub->CloseBrowse();
		}
	}
}

FReply SVendingBrowsePopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		DismissPopup();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
