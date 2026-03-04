// SShopWidget.cpp — RO Classic NPC shop UI with buy/sell modes,
// shopping cart, quantity popup, item tooltips, and drag support.

#include "SShopWidget.h"
#include "ShopSubsystem.h"
#include "InventorySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

// ============================================================
// RO Classic Color Palette (same values as other widgets)
// ============================================================

namespace ShopColors
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
	static const FLinearColor TextDim       (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor ZuzucoinGold  (0.95f, 0.82f, 0.48f, 1.f);
	static const FLinearColor ErrorRed      (0.95f, 0.25f, 0.20f, 1.f);
	static const FLinearColor ButtonBg      (0.35f, 0.24f, 0.14f, 1.f);
	static const FLinearColor ButtonHover   (0.45f, 0.34f, 0.20f, 1.f);
	static const FLinearColor RowBg         (0.28f, 0.19f, 0.11f, 1.f);
	static const FLinearColor RowBgAlt      (0.32f, 0.22f, 0.13f, 1.f);
	static const FLinearColor PopupOverlay  (0.00f, 0.00f, 0.00f, 0.60f);
	static const FLinearColor CartBg        (0.18f, 0.12f, 0.07f, 1.f);
}

// ============================================================
// Helpers
// ============================================================

FString SShopWidget::FormatPrice(int32 Price)
{
	if (Price < 1000) return FString::Printf(TEXT("%dz"), Price);
	if (Price < 1000000)
		return FString::Printf(TEXT("%d,%03dz"), Price / 1000, Price % 1000);
	return FString::Printf(TEXT("%d,%03d,%03dz"), Price / 1000000, (Price / 1000) % 1000, Price % 1000);
}

static TSharedRef<SWidget> BuildGoldDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(2.f, 1.f, 2.f, 1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(ShopColors::GoldDivider)
		];
}

static TSharedRef<SWidget> MakeGoldButton(const FText& Label, FOnClicked OnClick)
{
	return SNew(SButton)
		.ButtonColorAndOpacity(ShopColors::ButtonBg)
		.OnClicked(OnClick)
		.ContentPadding(FMargin(12.f, 4.f))
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(ShopColors::TextShadow)
			.Justification(ETextJustify::Center)
		];
}

// ============================================================
// Construct
// ============================================================

void SShopWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ModeSelectPanel = BuildModeSelectPanel();
	BuyModePanel = BuildBuyModePanel();
	SellModePanel = BuildSellModePanel();
	QuantityPopupPanel = BuildQuantityPopupOverlay();

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(CurrentSize.X)
		.HeightOverride(CurrentSize.Y)
		[
			// 3-layer frame: Gold → Dark → Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(ShopColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ShopColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildGoldDivider() ]
						+ SVerticalBox::Slot().FillHeight(1.f)
						[
							SAssignNew(ContentOverlay, SOverlay)
							+ SOverlay::Slot()
							[ ModeSelectPanel.ToSharedRef() ]
							+ SOverlay::Slot()
							[ BuyModePanel.ToSharedRef() ]
							+ SOverlay::Slot()
							[ SellModePanel.ToSharedRef() ]
							+ SOverlay::Slot()
							[ QuantityPopupPanel.ToSharedRef() ]
						]
					]
				]
			]
		]
	];

	// Start with mode select visible, others hidden
	SwitchToMode(EShopMode::ModeSelect);
	ApplyLayout();
}

// ============================================================
// Title Bar
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildTitleBar()
{
	UShopSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f, 4.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([Sub]() -> FText {
					if (!Sub) return FText::FromString(TEXT("Shop"));
					return FText::FromString(Sub->ShopName);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(ShopColors::PanelDark)
				.ContentPadding(FMargin(4.f, 0.f))
				.OnClicked_Lambda([Sub]() -> FReply {
					if (Sub) Sub->CloseShop();
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
				]
			]
		];
}

// ============================================================
// Mode Select Panel (Buy / Sell / Cancel)
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildModeSelectPanel()
{
	UShopSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("What would you like to do?")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("Buy")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub) Sub->SetMode(EShopMode::BuyMode);
							return FReply::Handled();
						}))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("Sell")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub) Sub->SetMode(EShopMode::SellMode);
							return FReply::Handled();
						}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			.HAlign(HAlign_Center)
			[
				MakeGoldButton(FText::FromString(TEXT("Cancel")),
					FOnClicked::CreateLambda([Sub]() -> FReply {
						if (Sub) Sub->CloseShop();
						return FReply::Handled();
					}))
			]
		];
}

// ============================================================
// Buy Mode Panel
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildBuyModePanel()
{
	UShopSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SVerticalBox)
		// Split: shop items (left) + cart (right)
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SHorizontalBox)
			// Left: Shop Items
			+ SHorizontalBox::Slot().FillWidth(0.55f).Padding(0, 0, 2, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Shop Items")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(ShopItemContainer, SVerticalBox)
						]
					]
				]
			]
			// Right: Cart
			+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2, 0, 0, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Cart")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::CartBg)
					.Padding(FMargin(1.f))
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(BuyCartContainer, SVerticalBox)
						]
					]
				]
			]
		]
		// Bottom bar: total, weight, zeny, buttons
		+ SVerticalBox::Slot().AutoHeight()
		[ BuildGoldDivider() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(4, 2, 4, 4)
		[
			SNew(SVerticalBox)
			// Info row
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("Total: %s"), *FormatPrice(Sub->GetBuyCartTotalCost())));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::TextBright))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						int32 CartW = Sub->GetBuyCartTotalWeight();
						return FText::FromString(FString::Printf(TEXT("Wt: +%d (%d/%d)"),
							CartW, Sub->CurrentWeight + CartW, Sub->MaxWeight));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(ShopColors::TextDim))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("Zeny: %s"), *FormatPrice(Sub->PlayerZuzucoin)));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
			]
			// Error message
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText {
					UShopSubsystem* S = OwningSubsystem.Get();
					if (!S || S->LastErrorMessage.IsEmpty()) return FText::GetEmpty();
					if (FPlatformTime::Seconds() > S->ErrorExpireTime) return FText::GetEmpty();
					return FText::FromString(S->LastErrorMessage);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(ShopColors::ErrorRed))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
			]
			// Buttons
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("OK")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub && Sub->BuyCart.Num() > 0) Sub->SubmitBuyCart();
							return FReply::Handled();
						}))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("Cancel")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub) Sub->CloseShop();
							return FReply::Handled();
						}))
				]
			]
		];
}

// ============================================================
// Sell Mode Panel
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildSellModePanel()
{
	UShopSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4.f)
		[
			SNew(SHorizontalBox)
			// Left: Sellable items
			+ SHorizontalBox::Slot().FillWidth(0.55f).Padding(0, 0, 2, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Your Items")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(SellItemContainer, SVerticalBox)
						]
					]
				]
			]
			// Right: Sell cart
			+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2, 0, 0, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2, 2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Items to Sell")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::GoldHighlight))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::CartBg)
					.Padding(FMargin(1.f))
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(SellCartContainer, SVerticalBox)
						]
					]
				]
			]
		]
		// Bottom bar
		+ SVerticalBox::Slot().AutoHeight()
		[ BuildGoldDivider() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(4, 2, 4, 4)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
				[
					SNew(STextBlock)
					.Text_Lambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("Revenue: %s"), *FormatPrice(Sub->GetSellCartTotalRevenue())));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::TextBright))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([Sub]() -> FText {
						if (!Sub) return FText::GetEmpty();
						return FText::FromString(FString::Printf(TEXT("Zeny: %s"), *FormatPrice(Sub->PlayerZuzucoin)));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(ShopColors::TextShadow)
				]
			]
			// Error message
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText {
					UShopSubsystem* S = OwningSubsystem.Get();
					if (!S || S->LastErrorMessage.IsEmpty()) return FText::GetEmpty();
					if (FPlatformTime::Seconds() > S->ErrorExpireTime) return FText::GetEmpty();
					return FText::FromString(S->LastErrorMessage);
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(ShopColors::ErrorRed))
			]
			// Buttons
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("OK")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub && Sub->SellCart.Num() > 0) Sub->SubmitSellCart();
							return FReply::Handled();
						}))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					MakeGoldButton(FText::FromString(TEXT("Cancel")),
						FOnClicked::CreateLambda([Sub]() -> FReply {
							if (Sub) Sub->CloseShop();
							return FReply::Handled();
						}))
				]
			]
		];
}

// ============================================================
// Quantity Popup Overlay
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildQuantityPopupOverlay()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ShopColors::PopupOverlay)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(220.f)
			[
				// Gold → Dark → Brown frame
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ShopColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ShopColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(ShopColors::PanelBrown)
						.Padding(FMargin(8.f))
						[
							SNew(SVerticalBox)
							// Item name
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									return FText::FromString(QuantityItemName);
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FSlateColor(ShopColors::TextBright))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(ShopColors::TextShadow)
							]
							// [-] [qty] [+]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(ShopColors::ButtonBg)
									.ContentPadding(FMargin(8, 2))
									.OnClicked_Lambda([this]() -> FReply {
										AdjustQuantity(-1);
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("-")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FSlateColor(ShopColors::TextBright))
									]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SBox).WidthOverride(60.f)
									[
										SAssignNew(QuantityTextBox, SEditableTextBox)
										.Text_Lambda([this]() -> FText {
											return FText::AsNumber(QuantityValue);
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.Justification(ETextJustify::Center)
										.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) {
											int32 NewVal = FCString::Atoi(*NewText.ToString());
											QuantityValue = FMath::Clamp(NewVal, 1, QuantityMax);
										})
									]
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SButton)
									.ButtonColorAndOpacity(ShopColors::ButtonBg)
									.ContentPadding(FMargin(8, 2))
									.OnClicked_Lambda([this]() -> FReply {
										AdjustQuantity(1);
										return FReply::Handled();
									})
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("+")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
										.ColorAndOpacity(FSlateColor(ShopColors::TextBright))
									]
								]
							]
							// Total price
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 2)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() -> FText {
									return FText::FromString(FString::Printf(TEXT("Total: %s"),
										*FormatPrice(QuantityValue * QuantityUnitPrice)));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(ShopColors::TextShadow)
							]
							// OK / Cancel
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									MakeGoldButton(FText::FromString(TEXT("OK")),
										FOnClicked::CreateLambda([this]() -> FReply {
											ConfirmQuantity();
											return FReply::Handled();
										}))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									MakeGoldButton(FText::FromString(TEXT("Cancel")),
										FOnClicked::CreateLambda([this]() -> FReply {
											HideQuantityPopup();
											return FReply::Handled();
										}))
								]
							]
						]
					]
				]
			]
		];
}

// ============================================================
// Item Row Builders
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildShopItemRow(int32 ItemIndex)
{
	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->ShopItems.IsValidIndex(ItemIndex))
		return SNullWidget::NullWidget;

	const FShopItem& Item = Sub->ShopItems[ItemIndex];
	const FLinearColor RowColor = (ItemIndex % 2 == 0) ? ShopColors::RowBg : ShopColors::RowBgAlt;

	// Try to get icon from InventorySubsystem
	FSlateBrush* IconBrush = nullptr;
	if (UWorld* World = Sub->GetWorld())
	{
		if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
		{
			IconBrush = InvSub->GetOrCreateItemIconBrush(Item.Icon);
		}
	}

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	// Icon
	if (IconBrush)
	{
		Row->AddSlot()
			.AutoWidth().VAlign(VAlign_Center).Padding(2, 1)
			[
				SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
				[
					SNew(SImage).Image(IconBrush)
				]
			];
	}

	// Name
	Row->AddSlot()
		.FillWidth(1.f).VAlign(VAlign_Center).Padding(4, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Item.Name))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(ShopColors::TextShadow)
			.ToolTipText(FText::FromString(Item.Description))
		];

	// Price
	Row->AddSlot()
		.AutoWidth().VAlign(VAlign_Center).Padding(4, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FormatPrice(Item.BuyPrice)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(ShopColors::TextShadow)
		];

	// Weight
	if (Item.Weight > 0)
	{
		Row->AddSlot()
			.AutoWidth().VAlign(VAlign_Center).Padding(2, 1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("w%d"), Item.Weight)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::TextDim))
			];
	}

	// Capture values for lambda
	int32 CapturedIndex = ItemIndex;
	FString CapturedName = Item.Name;
	int32 CapturedPrice = Item.BuyPrice;
	int32 CapturedWeight = Item.Weight;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(RowColor)
		.Padding(FMargin(2.f, 1.f))
		.Cursor(EMouseCursor::Hand)
		.OnMouseButtonDown_Lambda([this, CapturedIndex, CapturedName, CapturedPrice, CapturedWeight](
			const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				ShowQuantityPopup(CapturedName, 99, CapturedPrice, CapturedWeight, true, CapturedIndex);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[ Row ];
}

TSharedRef<SWidget> SShopWidget::BuildBuyCartRow(int32 CartIndex)
{
	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->BuyCart.IsValidIndex(CartIndex))
		return SNullWidget::NullWidget;

	const FCartItem& CI = Sub->BuyCart[CartIndex];
	int32 CapturedIdx = CartIndex;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ShopColors::RowBg)
		.Padding(FMargin(2.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(2, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s x%d"), *CI.Name, CI.Quantity)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FormatPrice(CI.GetTotalPrice())))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(ShopColors::PanelDark)
				.ContentPadding(FMargin(3, 0))
				.OnClicked_Lambda([Sub, CapturedIdx]() -> FReply {
					if (Sub) Sub->RemoveFromBuyCart(CapturedIdx);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(ShopColors::ErrorRed))
				]
			]
		];
}

TSharedRef<SWidget> SShopWidget::BuildSellItemRow(const FInventoryItem& Item, int32 SellPrice)
{
	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return SNullWidget::NullWidget;

	// Get icon
	FSlateBrush* IconBrush = nullptr;
	if (UWorld* World = Sub->GetWorld())
	{
		if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
		{
			IconBrush = InvSub->GetOrCreateItemIconBrush(Item.Icon);
		}
	}

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	if (IconBrush)
	{
		Row->AddSlot()
			.AutoWidth().VAlign(VAlign_Center).Padding(2, 1)
			[
				SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
				[
					SNew(SImage).Image(IconBrush)
				]
			];
	}

	Row->AddSlot()
		.FillWidth(1.f).VAlign(VAlign_Center).Padding(4, 1)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s x%d"), *Item.Name, Item.Quantity)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
				.ToolTipText(FText::FromString(Item.Description))
			]
		];

	Row->AddSlot()
		.AutoWidth().VAlign(VAlign_Center).Padding(4, 1)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FormatPrice(SellPrice)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
			.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(ShopColors::TextShadow)
		];

	// Capture for lambda
	FString CapturedName = Item.Name;
	int32 CapturedInvId = Item.InventoryId;
	int32 CapturedMaxQty = Item.Quantity;
	int32 CapturedSellPrice = SellPrice;

	// Check remaining quantity after cart
	int32 InCartQty = 0;
	for (const FCartItem& CI : Sub->SellCart)
	{
		if (CI.InventoryId == Item.InventoryId) { InCartQty = CI.Quantity; break; }
	}
	int32 RemainingQty = CapturedMaxQty - InCartQty;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ShopColors::RowBg)
		.Padding(FMargin(2.f, 1.f))
		.Cursor(EMouseCursor::Hand)
		.OnMouseButtonDown_Lambda([this, CapturedName, CapturedSellPrice, RemainingQty, CapturedInvId](
			const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton && RemainingQty > 0)
			{
				if (RemainingQty == 1)
				{
					// Non-stackable or single item — add directly
					UShopSubsystem* S = OwningSubsystem.Get();
					if (S)
					{
						FInventoryItem TempItem;
						TempItem.InventoryId = CapturedInvId;
						TempItem.Quantity = 1;
						TempItem.Name = CapturedName;
						S->AddToSellCart(TempItem, 1, CapturedSellPrice);
					}
				}
				else
				{
					ShowQuantityPopup(CapturedName, RemainingQty, CapturedSellPrice, 0, false, 0, CapturedInvId);
				}
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[ Row ];
}

TSharedRef<SWidget> SShopWidget::BuildSellCartRow(int32 CartIndex)
{
	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->SellCart.IsValidIndex(CartIndex))
		return SNullWidget::NullWidget;

	const FCartItem& CI = Sub->SellCart[CartIndex];
	int32 CapturedIdx = CartIndex;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(ShopColors::RowBg)
		.Padding(FMargin(2.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(2, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s x%d"), *CI.Name, CI.Quantity)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ShopColors::TextShadow)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FormatPrice(CI.GetTotalPrice())))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::ZuzucoinGold))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(ShopColors::PanelDark)
				.ContentPadding(FMargin(3, 0))
				.OnClicked_Lambda([Sub, CapturedIdx]() -> FReply {
					if (Sub) Sub->RemoveFromSellCart(CapturedIdx);
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(ShopColors::ErrorRed))
				]
			]
		];
}

// ============================================================
// Tooltip Builder
// ============================================================

TSharedRef<SWidget> SShopWidget::BuildItemTooltip(const FString& Desc, int32 ATK, int32 DEF,
	int32 MATK, int32 MDEF, int32 ReqLvl, int32 Weight, const FString& ItemType)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	if (!Desc.IsEmpty())
	{
		Box->AddSlot().AutoHeight().Padding(0, 0, 0, 2)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Desc))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(ShopColors::TextPrimary))
			.WrapTextAt(200.f)
		];
	}

	auto AddStatLine = [&Box](const FString& Label, int32 Value)
	{
		if (Value > 0)
		{
			Box->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s: %d"), *Label, Value)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(ShopColors::TextDim))
			];
		}
	};

	AddStatLine(TEXT("ATK"), ATK);
	AddStatLine(TEXT("DEF"), DEF);
	AddStatLine(TEXT("MATK"), MATK);
	AddStatLine(TEXT("MDEF"), MDEF);
	AddStatLine(TEXT("Weight"), Weight);

	if (ReqLvl > 1)
	{
		Box->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Req. Lv: %d"), ReqLvl)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(ShopColors::TextDim))
		];
	}

	return Box;
}

// ============================================================
// Rebuild Helpers
// ============================================================

void SShopWidget::RebuildShopItemList()
{
	if (!ShopItemContainer.IsValid()) return;
	ShopItemContainer->ClearChildren();

	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	for (int32 i = 0; i < Sub->ShopItems.Num(); ++i)
	{
		ShopItemContainer->AddSlot().AutoHeight()
		[
			BuildShopItemRow(i)
		];
	}
}

void SShopWidget::RebuildBuyCart()
{
	if (!BuyCartContainer.IsValid()) return;
	BuyCartContainer->ClearChildren();

	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	for (int32 i = 0; i < Sub->BuyCart.Num(); ++i)
	{
		BuyCartContainer->AddSlot().AutoHeight()
		[
			BuildBuyCartRow(i)
		];
	}
}

void SShopWidget::RebuildSellItemList()
{
	if (!SellItemContainer.IsValid()) return;
	SellItemContainer->ClearChildren();

	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	CachedSellableItems = Sub->GetSellableItems();

	for (const FInventoryItem& Item : CachedSellableItems)
	{
		int32 SellPrice = Sub->GetSellPrice(Item);
		SellItemContainer->AddSlot().AutoHeight()
		[
			BuildSellItemRow(Item, SellPrice)
		];
	}
}

void SShopWidget::RebuildSellCart()
{
	if (!SellCartContainer.IsValid()) return;
	SellCartContainer->ClearChildren();

	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	for (int32 i = 0; i < Sub->SellCart.Num(); ++i)
	{
		SellCartContainer->AddSlot().AutoHeight()
		[
			BuildSellCartRow(i)
		];
	}
}

// ============================================================
// Mode Switch
// ============================================================

void SShopWidget::SwitchToMode(EShopMode NewMode)
{
	if (ModeSelectPanel.IsValid())
		ModeSelectPanel->SetVisibility(NewMode == EShopMode::ModeSelect ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	if (BuyModePanel.IsValid())
		BuyModePanel->SetVisibility(NewMode == EShopMode::BuyMode ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	if (SellModePanel.IsValid())
		SellModePanel->SetVisibility(NewMode == EShopMode::SellMode ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
	if (QuantityPopupPanel.IsValid())
		QuantityPopupPanel->SetVisibility(bShowQuantityPopup ? EVisibility::Visible : EVisibility::Collapsed);

	// Rebuild lists on mode entry
	if (NewMode == EShopMode::BuyMode)
	{
		RebuildShopItemList();
		RebuildBuyCart();
	}
	else if (NewMode == EShopMode::SellMode)
	{
		RebuildSellItemList();
		RebuildSellCart();
	}
}

// ============================================================
// Quantity Popup
// ============================================================

void SShopWidget::ShowQuantityPopup(const FString& ItemName, int32 MaxQty, int32 UnitPrice,
	int32 UnitWeight, bool bIsBuy, int32 ItemIndex, int32 InvId)
{
	QuantityItemName = ItemName;
	QuantityMax = FMath::Max(1, MaxQty);
	QuantityUnitPrice = UnitPrice;
	QuantityWeight = UnitWeight;
	bQuantityIsBuy = bIsBuy;
	QuantityItemIndex = ItemIndex;
	QuantityInventoryId = InvId;
	QuantityValue = 1;
	bShowQuantityPopup = true;

	if (QuantityPopupPanel.IsValid())
		QuantityPopupPanel->SetVisibility(EVisibility::Visible);
}

void SShopWidget::HideQuantityPopup()
{
	bShowQuantityPopup = false;
	if (QuantityPopupPanel.IsValid())
		QuantityPopupPanel->SetVisibility(EVisibility::Collapsed);
}

void SShopWidget::ConfirmQuantity()
{
	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || QuantityValue <= 0) { HideQuantityPopup(); return; }

	if (bQuantityIsBuy)
	{
		// Add to buy cart
		if (Sub->ShopItems.IsValidIndex(QuantityItemIndex))
		{
			Sub->AddToBuyCart(Sub->ShopItems[QuantityItemIndex], QuantityValue);
		}
	}
	else
	{
		// Add to sell cart — find the inventory item
		for (const FInventoryItem& Item : CachedSellableItems)
		{
			if (Item.InventoryId == QuantityInventoryId)
			{
				int32 SellPrice = Sub->GetSellPrice(Item);
				Sub->AddToSellCart(Item, QuantityValue, SellPrice);
				break;
			}
		}
	}

	HideQuantityPopup();
}

void SShopWidget::AdjustQuantity(int32 Delta)
{
	QuantityValue = FMath::Clamp(QuantityValue + Delta, 1, QuantityMax);
}

// ============================================================
// Tick — detect mode/data changes
// ============================================================

void SShopWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UShopSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Mode change detection
	if (Sub->CurrentMode != LastMode)
	{
		LastMode = Sub->CurrentMode;
		HideQuantityPopup();
		SwitchToMode(LastMode);
	}

	// Data version change detection — rebuild active lists
	if (Sub->DataVersion != LastDataVersion)
	{
		LastDataVersion = Sub->DataVersion;

		if (LastMode == EShopMode::BuyMode)
		{
			RebuildBuyCart();
		}
		else if (LastMode == EShopMode::SellMode)
		{
			RebuildSellItemList();
			RebuildSellCart();
		}
	}

	// Also detect inventory changes for sell mode
	if (LastMode == EShopMode::SellMode)
	{
		if (UWorld* World = Sub->GetWorld())
		{
			if (UInventorySubsystem* InvSub = World->GetSubsystem<UInventorySubsystem>())
			{
				if (InvSub->DataVersion != LastInventoryVersion)
				{
					LastInventoryVersion = InvSub->DataVersion;
					RebuildSellItemList();
					RebuildSellCart();
				}
			}
		}
	}
}

// ============================================================
// Layout + Drag
// ============================================================

void SShopWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
	if (RootSizeBox.IsValid())
	{
		RootSizeBox->SetWidthOverride(CurrentSize.X);
		RootSizeBox->SetHeightOverride(CurrentSize.Y);
	}
}

FVector2D SShopWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return CurrentSize;
}

FReply SShopWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D ContentBounds = GetContentSize();

	// Bounds check — clicks outside pass through
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
		return FReply::Unhandled();

	// Title bar drag (top TitleBarHeight pixels)
	if (LocalPos.Y < TitleBarHeight)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Handled(); // Consume click inside widget
}

FReply SShopWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SShopWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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
