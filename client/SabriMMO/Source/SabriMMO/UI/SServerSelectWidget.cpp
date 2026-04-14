// SServerSelectWidget.cpp -- Server selection screen (RO Classic brown/gold theme)

#include "SServerSelectWidget.h"
#include "LoginFlowSubsystem.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette -- Server Select
// NOTE: Use FLinearColor() directly -- FColor() applies sRGB->linear
//       conversion which makes UI colors far too dark.
// ============================================================
namespace ServerColors
{
	static const FLinearColor PanelBrown       (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark        (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim         (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight    (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider      (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary      (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright       (1.0f, 1.0f, 1.0f, 1.f);
	static const FLinearColor TextShadow       (0.0f, 0.0f, 0.0f, 0.85f);
	static const FLinearColor SelectedRow      (0.50f, 0.38f, 0.15f, 0.6f);
	static const FLinearColor HoverRow         (0.40f, 0.30f, 0.12f, 0.3f);
	static const FLinearColor OnlineGreen      (0.20f, 0.75f, 0.20f, 1.f);
	static const FLinearColor OfflineRed       (0.75f, 0.20f, 0.20f, 1.f);
	static const FLinearColor MaintenanceYellow(0.90f, 0.75f, 0.10f, 1.f);
	static const FLinearColor ButtonGold       (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor ButtonDark       (0.30f, 0.20f, 0.10f, 1.f);
	static const FLinearColor Transparent      (0.0f, 0.0f, 0.0f, 0.0f);
}

// ============================================================
// Construction
// ============================================================
void SServerSelectWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		// Full-screen overlay: center the panel
		SNew(SOverlay)

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(400.f)
			[
				// Layer 1: Gold trim border (outermost, 2px)
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(ServerColors::GoldTrim)
				.Padding(FMargin(2.f))
				[
					// Layer 2: Dark inset (1px)
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(ServerColors::PanelDark)
					.Padding(FMargin(1.f))
					[
						// Layer 3: Brown content panel
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(ServerColors::PanelBrown)
						.Padding(FMargin(10.f, 8.f))
						[
							SNew(SVerticalBox)

							// --- Title: "Select Server" ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.HAlign(HAlign_Center)
							.Padding(0, 0, 0, 4)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Select Server")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
								.ColorAndOpacity(FSlateColor(ServerColors::GoldHighlight))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(ServerColors::TextShadow)
							]

							// --- Gold divider ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(1.f)
								.Padding(FMargin(2.f, 2.f, 2.f, 4.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(ServerColors::GoldDivider)
								]
							]

							// --- Server list (scrollable, fills remaining height) ---
							+ SVerticalBox::Slot()
							.FillHeight(1.f)
							.Padding(0, 0, 0, 4)
							[
								SNew(SBox)
								.MinDesiredHeight(120.f)
								.MaxDesiredHeight(300.f)
								[
									SNew(SScrollBox)

									+ SScrollBox::Slot()
									[
										SAssignNew(ServerListBox, SVerticalBox)
									]
								]
							]

							// --- Gold divider ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.HeightOverride(1.f)
								.Padding(FMargin(2.f, 2.f, 2.f, 2.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(ServerColors::GoldDivider)
								]
							]

							// --- Button row ---
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(8, 6)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)

								// Connect button (gold, fills available width)
								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.ButtonColorAndOpacity(ServerColors::ButtonGold)
									.OnClicked(FOnClicked::CreateSP(this, &SServerSelectWidget::OnConnectClicked))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Connect")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										.ColorAndOpacity(FSlateColor(ServerColors::TextBright))
										.ShadowOffset(FVector2D(1, 1))
										.ShadowColorAndOpacity(ServerColors::TextShadow)
									]
								]

								// 8px spacer
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SBox)
									.WidthOverride(8.f)
								]

								// Cancel button (dark, auto width)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SButton)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.ButtonColorAndOpacity(ServerColors::ButtonDark)
									.OnClicked(FOnClicked::CreateSP(this, &SServerSelectWidget::OnCancelClicked))
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("Cancel")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
										.ColorAndOpacity(FSlateColor(ServerColors::TextPrimary))
										.ShadowOffset(FVector2D(1, 1))
										.ShadowColorAndOpacity(ServerColors::TextShadow)
									]
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
// Server List Population
// ============================================================
void SServerSelectWidget::PopulateServerList(const TArray<FServerInfo>& Servers)
{
	CachedServers = Servers;

	if (!ServerListBox.IsValid())
	{
		return;
	}

	ServerListBox->ClearChildren();

	for (int32 Index = 0; Index < CachedServers.Num(); ++Index)
	{
		ServerListBox->AddSlot()
		.AutoHeight()
		[
			BuildServerRow(CachedServers[Index], Index)
		];
	}

	// Auto-select the first server if available
	if (CachedServers.Num() > 0)
	{
		SelectServer(0);
	}
	else
	{
		SelectedIndex = -1;
	}
}

// ============================================================
// Build a single server row
// ============================================================
TSharedRef<SWidget> SServerSelectWidget::BuildServerRow(const FServerInfo& Server, int32 Index)
{
	// Determine row background based on selection state
	const FLinearColor RowBackground = (Index == SelectedIndex)
		? ServerColors::SelectedRow
		: ServerColors::Transparent;

	// Determine status color and text
	FLinearColor StatusColor = ServerColors::OnlineGreen;
	FString StatusText = TEXT("Online");

	if (Server.Status == TEXT("offline"))
	{
		StatusColor = ServerColors::OfflineRed;
		StatusText = TEXT("Offline");
	}
	else if (Server.Status == TEXT("maintenance"))
	{
		StatusColor = ServerColors::MaintenanceYellow;
		StatusText = TEXT("Maintenance");
	}

	// Population string
	const FString PopulationText = FString::Printf(TEXT("%d/%d"), Server.Population, Server.MaxPopulation);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(RowBackground)
		.Padding(FMargin(6.f, 4.f))
		.OnMouseButtonDown_Lambda([this, Index](const FGeometry&, const FPointerEvent& MouseEvent) -> FReply
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				SelectServer(Index);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
			SNew(SHorizontalBox)

			// Server name (auto width)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Server.Name))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				.ColorAndOpacity(FSlateColor(ServerColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ServerColors::TextShadow)
			]

			// Fill spacer
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(SBox)
			]

			// Population (auto width)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(PopulationText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(ServerColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ServerColors::TextShadow)
			]

			// 8px spacer
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(8.f)
			]

			// Status (auto width, colored)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(StatusText))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(StatusColor))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(ServerColors::TextShadow)
			]
		];
}

// ============================================================
// Row Selection
// ============================================================
void SServerSelectWidget::SelectServer(int32 Index)
{
	if (Index < 0 || Index >= CachedServers.Num())
	{
		return;
	}

	SelectedIndex = Index;

	// Rebuild all rows to reflect the new selection highlight
	if (!ServerListBox.IsValid())
	{
		return;
	}

	ServerListBox->ClearChildren();

	for (int32 RowIndex = 0; RowIndex < CachedServers.Num(); ++RowIndex)
	{
		ServerListBox->AddSlot()
		.AutoHeight()
		[
			BuildServerRow(CachedServers[RowIndex], RowIndex)
		];
	}
}

// ============================================================
// Button Handlers
// ============================================================
FReply SServerSelectWidget::OnConnectClicked()
{
	UAudioSubsystem::PlayUIClickStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (SelectedIndex >= 0 && SelectedIndex < CachedServers.Num())
	{
		if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
		{
			Subsystem->OnServerSelected(CachedServers[SelectedIndex]);
		}
	}
	return FReply::Handled();
}

FReply SServerSelectWidget::OnCancelClicked()
{
	UAudioSubsystem::PlayUICancelStatic(OwningSubsystem.IsValid() ? OwningSubsystem->GetWorld() : nullptr);
	if (ULoginFlowSubsystem* Subsystem = OwningSubsystem.Get())
	{
		Subsystem->OnBackToLogin();
	}
	return FReply::Handled();
}

// ============================================================
// Keyboard Input
// ============================================================
FReply SServerSelectWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept)
	{
		return OnConnectClicked();
	}

	if (Key == EKeys::Escape)
	{
		return OnCancelClicked();
	}

	return FReply::Unhandled();
}
