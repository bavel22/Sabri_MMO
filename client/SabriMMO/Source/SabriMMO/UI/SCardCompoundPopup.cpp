// SCardCompoundPopup.cpp — Card compound popup: shows eligible equipment when
// double-clicking a card. Matches RO Classic behavior: select equipment → instant compound.

#include "SCardCompoundPopup.h"
#include "InventorySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

// ============================================================
// Colors (reuse RO palette)
// ============================================================

namespace CompoundColors
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
	static const FLinearColor SlotEmpty     (0.45f, 0.38f, 0.25f, 1.f);
	static const FLinearColor SlotFilled    (0.75f, 0.60f, 0.25f, 1.f);
	static const FLinearColor ErrorRed      (0.90f, 0.25f, 0.20f, 1.f);
	static const FLinearColor CardPurple    (0.55f, 0.25f, 0.60f, 1.f);
	static const FLinearColor Backdrop      (0.00f, 0.00f, 0.00f, 0.40f);
}

static constexpr float PopupWidth = 280.f;
static constexpr float RowIconSize = 24.f;
static constexpr float MaxListHeight = 240.f;

// ============================================================
// Construction
// ============================================================

void SCardCompoundPopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	CardItem = InArgs._Card;
	EligibleItems = InArgs._EligibleEquipment;

	ChildSlot
	[
		// Fullscreen transparent backdrop — click anywhere outside to dismiss
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(CompoundColors::Backdrop)
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
			.WidthOverride(PopupWidth)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CompoundColors::Backdrop)  // prevent backdrop click-through
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) -> FReply
				{
					return FReply::Handled();  // Block propagation to backdrop
				})
				[
					// 3-layer frame: Gold → Dark → Brown
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CompoundColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CompoundColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(CompoundColors::PanelBrown)
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
									[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(CompoundColors::GoldDivider) ]
								]

								// Subtitle
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Select equipment to compound into:")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(CompoundColors::TextDim))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CompoundColors::TextShadow)
								]

								// Equipment list (scrollable)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.MaxDesiredHeight(MaxListHeight)
									[
										BuildEquipmentList()
									]
								]

								// Status message (hidden by default)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SAssignNew(StatusTextBlock, STextBlock)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(CompoundColors::ErrorRed))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CompoundColors::TextShadow)
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
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SCardCompoundPopup::BuildTitleBar()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();

	TSharedRef<SWidget> CardIcon = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = Sub ? Sub->GetOrCreateItemIconBrush(CardItem.Icon) : nullptr;
		if (Brush)
		{
			return SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
			[
				SNew(SImage).Image(Brush)
			];
		}
		return SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CompoundColors::CardPurple)
		];
	}();

	return SNew(SHorizontalBox)
		// Card icon
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[ CardIcon ]
		// Card name
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("Compound: %s"), *CardItem.Name)))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			.ColorAndOpacity(FSlateColor(CompoundColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(CompoundColors::TextShadow)
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
				.ColorAndOpacity(FSlateColor(CompoundColors::TextDim))
			]
		];
}

// ============================================================
// Equipment list
// ============================================================

TSharedRef<SWidget> SCardCompoundPopup::BuildEquipmentList()
{
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible);

	for (const FInventoryItem& Equipment : EligibleItems)
	{
		ScrollBox->AddSlot().Padding(0, 1)
		[
			BuildEquipmentRow(Equipment)
		];
	}

	return ScrollBox;
}

TSharedRef<SWidget> SCardCompoundPopup::BuildEquipmentRow(const FInventoryItem& Equipment)
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	int32 EquipInvId = Equipment.InventoryId;

	// Build icon
	TSharedRef<SWidget> EquipIcon = [&]() -> TSharedRef<SWidget>
	{
		FSlateBrush* Brush = Sub ? Sub->GetOrCreateItemIconBrush(Equipment.Icon) : nullptr;
		if (Brush)
		{
			return SNew(SBox).WidthOverride(RowIconSize).HeightOverride(RowIconSize)
			[
				SNew(SImage).Image(Brush)
			];
		}
		return SNew(SBox).WidthOverride(RowIconSize).HeightOverride(RowIconSize)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(CompoundColors::GoldDark)
		];
	}();

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(CompoundColors::RowBg)
		.Padding(FMargin(4.f, 3.f))
		.Cursor(EMouseCursor::Hand)
		.OnMouseButtonDown_Lambda([this, EquipInvId](const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				OnEquipmentClicked(EquipInvId);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
			SNew(SHorizontalBox)
			// Icon
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
			[ EquipIcon ]
			// Name
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Equipment.GetDisplayName()))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(CompoundColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CompoundColors::TextShadow)
			]
			// Slot diamonds
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
			[ BuildSlotDiamonds(Equipment) ]
		];
}

// ============================================================
// Slot diamond indicators
// ============================================================

TSharedRef<SWidget> SCardCompoundPopup::BuildSlotDiamonds(const FInventoryItem& Equipment)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);

	for (int32 i = 0; i < Equipment.Slots; ++i)
	{
		bool bFilled = (i < Equipment.CompoundedCards.Num() && Equipment.CompoundedCards[i] > 0);

		// Unicode: filled diamond U+25C6, empty diamond U+25C7
		FString DiamondChar = bFilled ? FString(TEXT("\u25C6")) : FString(TEXT("\u25C7"));
		FLinearColor DiamondColor = bFilled ? CompoundColors::SlotFilled : CompoundColors::SlotEmpty;

		Row->AddSlot().AutoWidth().Padding(1, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(DiamondChar))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity(FSlateColor(DiamondColor))
		];
	}

	return Row;
}

// ============================================================
// Actions
// ============================================================

void SCardCompoundPopup::OnEquipmentClicked(int32 EquipmentInventoryId)
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Find the equipment in our eligible list
	const FInventoryItem* Equipment = nullptr;
	for (const FInventoryItem& Item : EligibleItems)
	{
		if (Item.InventoryId == EquipmentInventoryId)
		{
			Equipment = &Item;
			break;
		}
	}
	if (!Equipment) return;

	int32 SlotIndex = FindFirstEmptySlot(*Equipment);
	if (SlotIndex < 0) return;

	// Emit compound event — no confirmation dialog (matches RO Classic)
	Sub->EmitCardCompound(CardItem.InventoryId, EquipmentInventoryId, SlotIndex);
}

int32 SCardCompoundPopup::FindFirstEmptySlot(const FInventoryItem& Equipment) const
{
	for (int32 i = 0; i < Equipment.Slots; ++i)
	{
		if (i >= Equipment.CompoundedCards.Num() || Equipment.CompoundedCards[i] <= 0)
			return i;
	}
	return -1;
}

void SCardCompoundPopup::DismissPopup()
{
	UInventorySubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		Sub->HideCardCompoundPopup();
	}
}

// ============================================================
// Status message
// ============================================================

void SCardCompoundPopup::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(FText::FromString(Message));
		StatusTextBlock->SetColorAndOpacity(FSlateColor(
			bIsError ? CompoundColors::ErrorRed : CompoundColors::TextPrimary));
		StatusTextBlock->SetVisibility(
			Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible);
	}
}

// ============================================================
// Input
// ============================================================

FReply SCardCompoundPopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		DismissPopup();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
