// SVendingSetupPopup.cpp — Modal popup for configuring a vending shop.
// Cart items with checkbox selection, quantity/price inputs, shop name field.

#include "SVendingSetupPopup.h"
#include "VendingSubsystem.h"
#include "InventorySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

// RO Classic brown/gold theme colors
namespace VendSetupColors
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
}

static constexpr float SetupPopupWidth = 380.f;
static constexpr float SetupMaxListHeight = 280.f;
static constexpr float SetupIconSize = 24.f;

void SVendingSetupPopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	AllCartItems = InArgs._CartItems;
	MaxVendSlots = InArgs._MaxSlots;

	// Initialize per-row state
	RowStates.SetNum(AllCartItems.Num());
	for (int32 i = 0; i < AllCartItems.Num(); ++i)
	{
		RowStates[i].bChecked = false;
		RowStates[i].Quantity = AllCartItems[i].Quantity;
		RowStates[i].MaxQuantity = AllCartItems[i].MaxQuantity;
		RowStates[i].Price = 0;
	}

	ChildSlot
	[
		// Fullscreen backdrop — click outside dismisses
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(VendSetupColors::Backdrop)
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
			.WidthOverride(SetupPopupWidth)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(VendSetupColors::Backdrop)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) -> FReply
				{
					return FReply::Handled();
				})
				[
					// 3-layer RO frame: Gold -> Dark -> Brown
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(VendSetupColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(VendSetupColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(VendSetupColors::PanelBrown)
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
										.BorderBackgroundColor(VendSetupColors::GoldDivider)
									]
								]

								// Shop name input
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Shop Name:")))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
										.ColorAndOpacity(FSlateColor(VendSetupColors::TextDim))
										.ShadowOffset(FVector2D(1, 1))
										.ShadowColorAndOpacity(VendSetupColors::TextShadow)
									]
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
									[
										SAssignNew(ShopNameTextBox, SEditableTextBox)
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
										.HintText(FText::FromString(TEXT("My Shop")))
									]
								]

								// Max items label
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SAssignNew(SelectedCountText, STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("Selected: 0 / %d slots"), MaxVendSlots)))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(VendSetupColors::TextDim))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(VendSetupColors::TextShadow)
								]

								// Scrollable cart item list
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.MaxDesiredHeight(SetupMaxListHeight)
									[ BuildItemList() ]
								]

								// Bottom bar (Open Shop button + status)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[ BuildBottomBar() ]
							]
						]
					]
				]
			]
		]
	];
}

// ── Title Bar ────────────────────────────────────────────────────

TSharedRef<SWidget> SVendingSetupPopup::BuildTitleBar()
{
	return SNew(SHorizontalBox)
		// Title text
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Open Vending Shop")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(VendSetupColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendSetupColors::TextShadow)
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
				.ColorAndOpacity(FSlateColor(VendSetupColors::TextDim))
			]
		];
}

// ── Item List ────────────────────────────────────────────────────

TSharedRef<SWidget> SVendingSetupPopup::BuildItemList()
{
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible);

	for (int32 i = 0; i < AllCartItems.Num(); ++i)
	{
		ScrollBox->AddSlot().Padding(0, 1)
		[
			BuildItemRow(i)
		];
	}

	return ScrollBox;
}

TSharedRef<SWidget> SVendingSetupPopup::BuildItemRow(int32 Index)
{
	const FVendSetupItem& Item = AllCartItems[Index];

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
						return SNew(SBox).WidthOverride(SetupIconSize).HeightOverride(SetupIconSize)
						[
							SNew(SImage).Image(Brush)
						];
					}
				}
			}
		}
		// Fallback: colored box
		return SNew(SBox).WidthOverride(SetupIconSize).HeightOverride(SetupIconSize)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(VendSetupColors::GoldDivider)
		];
	}();

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(VendSetupColors::RowBg)
		.Padding(FMargin(4.f, 3.f))
		[
			SNew(SHorizontalBox)

			// Checkbox
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this, Index]() -> ECheckBoxState
				{
					return (Index < RowStates.Num() && RowStates[Index].bChecked)
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState NewState)
				{
					if (Index < RowStates.Num())
					{
						RowStates[Index].bChecked = (NewState == ECheckBoxState::Checked);
						UpdateSelectedCount();
					}
				})
			]

			// Icon
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[ IconWidget ]

			// Name
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item.Name))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(VendSetupColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(VendSetupColors::TextShadow)
				.AutoWrapText(false)
			]

			// Quantity input
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
			[
				SNew(SBox).WidthOverride(40.f)
				[
					SNew(SEditableTextBox)
					.Text(FText::AsNumber(Index < RowStates.Num() ? RowStates[Index].Quantity : 1))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.Justification(ETextJustify::Center)
					.OnTextCommitted_Lambda([this, Index](const FText& NewText, ETextCommit::Type)
					{
						if (Index < RowStates.Num())
						{
							int32 NewVal = FCString::Atoi(*NewText.ToString());
							RowStates[Index].Quantity = FMath::Clamp(NewVal, 1, RowStates[Index].MaxQuantity);
						}
					})
				]
			]

			// Price input
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 2, 0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("z")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(VendSetupColors::GoldPrice))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(60.f)
					[
						SNew(SEditableTextBox)
						.Text(FText::GetEmpty())
						.HintText(FText::FromString(TEXT("Price")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.Justification(ETextJustify::Right)
						.OnTextCommitted_Lambda([this, Index](const FText& NewText, ETextCommit::Type)
						{
							if (Index < RowStates.Num())
							{
								int32 NewVal = FCString::Atoi(*NewText.ToString());
								RowStates[Index].Price = FMath::Max(NewVal, 0);
							}
						})
					]
				]
			]
		];
}

// ── Bottom Bar ───────────────────────────────────────────────────

TSharedRef<SWidget> SVendingSetupPopup::BuildBottomBar()
{
	return SNew(SVerticalBox)

		// Open Shop button
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SButton)
			.ContentPadding(FMargin(6, 3))
			.HAlign(HAlign_Center)
			.ButtonColorAndOpacity(VendSetupColors::ButtonBrown)
			.OnClicked_Lambda([this]() -> FReply
			{
				OnOpenShopClicked();
				return FReply::Handled();
			})
			.IsEnabled_Lambda([this]() -> bool
			{
				int32 Count = CountCheckedItems();
				return Count > 0 && Count <= MaxVendSlots;
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Open Shop")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(VendSetupColors::TextPrimary))
			]
		]

		// Status message
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[
			SAssignNew(StatusTextBlock, STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(VendSetupColors::SuccessGreen))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(VendSetupColors::TextShadow)
			.AutoWrapText(true)
			.Visibility(EVisibility::Collapsed)
		];
}

// ── Actions ──────────────────────────────────────────────────────

void SVendingSetupPopup::OnOpenShopClicked()
{
	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Validate: at least 1 checked, all checked must have price > 0
	TArray<FVendSetupItem> SelectedItems;
	for (int32 i = 0; i < AllCartItems.Num() && i < RowStates.Num(); ++i)
	{
		if (!RowStates[i].bChecked) continue;

		if (RowStates[i].Price <= 0)
		{
			SetStatusMessage(FString::Printf(TEXT("Set a price for: %s"), *AllCartItems[i].Name), true);
			return;
		}

		FVendSetupItem Item = AllCartItems[i];
		Item.Quantity = RowStates[i].Quantity;
		Item.Price = RowStates[i].Price;
		SelectedItems.Add(Item);
	}

	if (SelectedItems.Num() == 0)
	{
		SetStatusMessage(TEXT("Select at least one item to sell."), true);
		return;
	}

	if (SelectedItems.Num() > MaxVendSlots)
	{
		SetStatusMessage(FString::Printf(TEXT("Too many items selected (max %d)."), MaxVendSlots), true);
		return;
	}

	// Get shop name
	FString ShopName = ShopNameTextBox.IsValid()
		? ShopNameTextBox->GetText().ToString()
		: TEXT("");
	if (ShopName.IsEmpty())
	{
		ShopName = TEXT("My Shop");
	}
	// Clamp to 80 chars
	if (ShopName.Len() > 80)
	{
		ShopName = ShopName.Left(80);
	}

	Sub->StartVending(ShopName, SelectedItems);
}

int32 SVendingSetupPopup::CountCheckedItems() const
{
	int32 Count = 0;
	for (const FRowState& RS : RowStates)
	{
		if (RS.bChecked) ++Count;
	}
	return Count;
}

void SVendingSetupPopup::UpdateSelectedCount()
{
	if (SelectedCountText.IsValid())
	{
		int32 Count = CountCheckedItems();
		SelectedCountText->SetText(FText::FromString(
			FString::Printf(TEXT("Selected: %d / %d slots"), Count, MaxVendSlots)));

		FLinearColor Color = (Count > MaxVendSlots)
			? VendSetupColors::ErrorRed
			: VendSetupColors::TextDim;
		SelectedCountText->SetColorAndOpacity(FSlateColor(Color));
	}
}

// ── Status Message ───────────────────────────────────────────────

void SVendingSetupPopup::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (!StatusTextBlock.IsValid()) return;

	StatusTextBlock->SetText(FText::FromString(Message));
	StatusTextBlock->SetColorAndOpacity(FSlateColor(
		bIsError ? VendSetupColors::ErrorRed : VendSetupColors::SuccessGreen));
	StatusTextBlock->SetVisibility(
		Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible);
}

// ── Dismiss ──────────────────────────────────────────────────────

void SVendingSetupPopup::DismissPopup()
{
	UVendingSubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		Sub->CloseVendingSetup();
	}
}

FReply SVendingSetupPopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		DismissPopup();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
