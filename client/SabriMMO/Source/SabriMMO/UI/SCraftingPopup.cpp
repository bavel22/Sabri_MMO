// SCraftingPopup.cpp — Shared crafting popup for Arrow Crafting + Pharmacy.
// Adapted from SCardCompoundPopup pattern: fullscreen backdrop, centered popup,
// scrollable recipe list, click-to-craft, status message area.

#include "SCraftingPopup.h"
#include "CraftingSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

// RO Classic brown/gold theme colors (same as SCardCompoundPopup)
namespace CraftColors
{
	static const FLinearColor Backdrop     (0.f, 0.f, 0.f, 0.4f);
	static const FLinearColor GoldTrim     (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor PanelDark    (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor PanelBrown   (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor GoldHighlight(0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider  (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary  (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextDim      (0.70f, 0.62f, 0.48f, 1.f);
	static const FLinearColor TextShadow   (0.f, 0.f, 0.f, 0.85f);
	static const FLinearColor RowBg        (0.18f, 0.12f, 0.07f, 1.f);
	static const FLinearColor RowHover     (0.28f, 0.20f, 0.12f, 1.f);
	static const FLinearColor SuccessGreen (0.30f, 0.85f, 0.30f, 1.f);
	static const FLinearColor ErrorRed     (0.95f, 0.25f, 0.20f, 1.f);
	static const FLinearColor RateYellow   (0.90f, 0.75f, 0.10f, 1.f);
	static const FLinearColor QuantityBlue (0.55f, 0.75f, 0.95f, 1.f);
}

static constexpr float PopupWidth = 320.f;
static constexpr float MaxListHeight = 300.f;

void SCraftingPopup::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;
	PopupTitle = InArgs._Title;
	RecipeList = InArgs._Recipes;
	bShowRate = InArgs._ShowSuccessRate;

	ChildSlot
	[
		// Fullscreen backdrop — click outside dismisses
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(CraftColors::Backdrop)
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
			.WidthOverride(PopupWidth)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(CraftColors::Backdrop)
				.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) -> FReply
				{
					return FReply::Handled();
				})
				[
					// 3-layer RO frame: Gold -> Dark -> Brown
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(CraftColors::GoldTrim)
					.Padding(FMargin(2.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CraftColors::PanelDark)
						.Padding(FMargin(1.f))
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(CraftColors::PanelBrown)
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
										.BorderBackgroundColor(CraftColors::GoldDivider)
									]
								]

								// Instruction text
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Select an item to craft:")))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(CraftColors::TextDim))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CraftColors::TextShadow)
								]

								// Scrollable recipe list
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBox)
									.MaxDesiredHeight(MaxListHeight)
									[ BuildRecipeList() ]
								]

								// Status message (success/error)
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SAssignNew(StatusTextBlock, STextBlock)
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(CraftColors::SuccessGreen))
									.ShadowOffset(FVector2D(1, 1))
									.ShadowColorAndOpacity(CraftColors::TextShadow)
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

// ── Title Bar ────────────────────────────────────────────────────

TSharedRef<SWidget> SCraftingPopup::BuildTitleBar()
{
	return SNew(SHorizontalBox)
		// Title text
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(PopupTitle))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(CraftColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(CraftColors::TextShadow)
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
				.ColorAndOpacity(FSlateColor(CraftColors::TextDim))
			]
		];
}

// ── Recipe List ──────────────────────────────────────────────────

TSharedRef<SWidget> SCraftingPopup::BuildRecipeList()
{
	TSharedRef<SScrollBox> ScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Visible);

	for (const FCraftingRecipe& Recipe : RecipeList)
	{
		ScrollBox->AddSlot().Padding(0, 1)
		[
			BuildRecipeRow(Recipe)
		];
	}

	return ScrollBox;
}

TSharedRef<SWidget> SCraftingPopup::BuildRecipeRow(const FCraftingRecipe& Recipe)
{
	// Build subtitle: "from [SourceName]" for Arrow Crafting, or ingredient list for Pharmacy
	FString Subtitle;
	if (!Recipe.SourceItemName.IsEmpty())
	{
		if (Recipe.SourceQuantity > 1)
			Subtitle = FString::Printf(TEXT("from %s [%d]"), *Recipe.SourceItemName, Recipe.SourceQuantity);
		else
			Subtitle = FString::Printf(TEXT("from %s"), *Recipe.SourceItemName);
	}

	// Build quantity text
	FString QtyText = Recipe.OutputQuantity > 1
		? FString::Printf(TEXT(" x%d"), Recipe.OutputQuantity)
		: TEXT("");

	// Build rate text (Pharmacy only)
	FString RateText = bShowRate
		? FString::Printf(TEXT("Rate: %d%%"), Recipe.SuccessRate)
		: TEXT("");

	// Capture recipe by value for lambda
	FCraftingRecipe CapturedRecipe = Recipe;

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(CraftColors::RowBg)
		.Padding(FMargin(6.f, 4.f))
		.Cursor(EMouseCursor::Hand)
		.OnMouseButtonDown_Lambda([this, CapturedRecipe](const FGeometry&, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				OnRecipeClicked(CapturedRecipe);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
			SNew(SVerticalBox)

			// Row 1: Output name + quantity
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Recipe.OutputName + QtyText))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(CraftColors::TextPrimary))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(CraftColors::TextShadow)
				]
				// Success rate (Pharmacy only)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(RateText))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(CraftColors::RateYellow))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(CraftColors::TextShadow)
					.Visibility(bShowRate ? EVisibility::Visible : EVisibility::Collapsed)
				]
			]

			// Row 2: Source/ingredients subtitle
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Subtitle))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
				.ColorAndOpacity(FSlateColor(CraftColors::TextDim))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(CraftColors::TextShadow)
				.Visibility(Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			]
		];
}

// ── Click Handler ────────────────────────────────────────────────

void SCraftingPopup::OnRecipeClicked(const FCraftingRecipe& Recipe)
{
	UCraftingSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	if (Sub->IsCraftingPopupVisible())
	{
		switch (Sub->GetCurrentCraftType())
		{
		case UCraftingSubsystem::ECraftType::ArrowCrafting:
			Sub->EmitArrowCraft(Recipe.SourceInventoryId);
			break;
		case UCraftingSubsystem::ECraftType::Converter:
			Sub->EmitConverterCraft(Recipe.OutputItemId);
			break;
		default:
			Sub->EmitPharmacyCraft(Recipe.OutputItemId);
			break;
		}
	}
}

// ── Status Message ───────────────────────────────────────────────

void SCraftingPopup::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (!StatusTextBlock.IsValid()) return;

	StatusTextBlock->SetText(FText::FromString(Message));
	StatusTextBlock->SetColorAndOpacity(FSlateColor(bIsError ? CraftColors::ErrorRed : CraftColors::SuccessGreen));
	StatusTextBlock->SetVisibility(EVisibility::Visible);
}

// ── Dismiss ──────────────────────────────────────────────────────

void SCraftingPopup::DismissPopup()
{
	UCraftingSubsystem* Sub = OwningSubsystem.Get();
	if (Sub)
	{
		Sub->HideCraftingPopup();
	}
}

FReply SCraftingPopup::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		DismissPopup();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
