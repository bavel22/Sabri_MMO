// SPartyWidget.cpp — RO Classic party window with member HP bars,
// leader indicator, invite popup, context menu, and drag-to-move.

#include "SPartyWidget.h"
#include "PartySubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"

// ============================================================
// Colors — RO Classic Party Window
// ============================================================

const FLinearColor SPartyWidget::PanelBrown     (0.43f, 0.29f, 0.17f, 0.92f);
const FLinearColor SPartyWidget::PanelDark      (0.22f, 0.14f, 0.08f, 1.0f);
const FLinearColor SPartyWidget::GoldTrim       (0.72f, 0.58f, 0.28f, 1.0f);
const FLinearColor SPartyWidget::GoldHighlight  (0.92f, 0.80f, 0.45f, 1.0f);
const FLinearColor SPartyWidget::ContentWhite   (1.0f, 1.0f, 1.0f, 0.95f);
const FLinearColor SPartyWidget::MemberNameTeal (0.0f, 0.48f, 0.48f, 1.0f);
const FLinearColor SPartyWidget::LeaderNameBlue (0.03f, 0.19f, 0.48f, 1.0f);
const FLinearColor SPartyWidget::OfflineGray    (0.5f, 0.5f, 0.5f, 1.0f);
const FLinearColor SPartyWidget::HPBarGreen     (0.063f, 0.937f, 0.129f, 1.0f);
const FLinearColor SPartyWidget::HPBarRed       (1.0f, 0.0f, 0.0f, 1.0f);
const FLinearColor SPartyWidget::HPBarBorder    (0.063f, 0.094f, 0.612f, 1.0f);
const FLinearColor SPartyWidget::HPBarBackground(0.259f, 0.259f, 0.259f, 1.0f);

namespace PartyColors
{
	static const FLinearColor GoldDivider  (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextShadow   (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor TextDim      (0.65f, 0.55f, 0.40f, 1.f);
	static const FLinearColor TextPrimary  (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor ButtonBg     (0.28f, 0.18f, 0.10f, 1.f);
	static const FLinearColor ButtonHover  (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor InviteBg     (0.15f, 0.10f, 0.06f, 0.95f);
	static const FLinearColor AcceptGreen  (0.2f, 0.55f, 0.2f, 1.f);
	static const FLinearColor DeclineRed   (0.55f, 0.15f, 0.15f, 1.f);
	static const FLinearColor HPTextBlack  (0.0f, 0.0f, 0.0f, 1.0f);
	static const FLinearColor MapTextGray  (0.45f, 0.45f, 0.45f, 1.0f);
	static const FLinearColor SPBarBlue    (0.12f, 0.35f, 0.85f, 1.0f);
}

// ============================================================
// Construction
// ============================================================

void SPartyWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SNew(SOverlay)

		// Layer 0: Main party window
		+ SOverlay::Slot()
		[
			SAssignNew(RootSizeBox, SBox)
			.WidthOverride(WidgetWidth)
			[
				// 3-layer RO frame: Gold > Dark > Brown
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(GoldTrim)
				.Padding(FMargin(2.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(PanelDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(PanelBrown)
						.Padding(FMargin(0.f))
						[
							SNew(SVerticalBox)

							// ---- Title bar ----
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildTitleBar() ]

							// ---- Gold divider ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 0.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(PartyColors::GoldDivider)
								]
							]

							// ---- Nav bar ----
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildNavBar() ]

							// ---- Gold divider ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 0.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(PartyColors::GoldDivider)
								]
							]

							// ---- Member list (scrollable) ----
							+ SVerticalBox::Slot().FillHeight(1.f).Padding(FMargin(4.f, 2.f))
							[
								SNew(SScrollBox)
								.ScrollBarAlwaysVisible(false)
								+ SScrollBox::Slot()
								[
									SAssignNew(MemberListBox, SVerticalBox)
								]
							]

							// ---- Gold divider ----
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SBox).HeightOverride(1.f).Padding(FMargin(2.f, 0.f))
								[
									SNew(SBorder)
									.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.BorderBackgroundColor(PartyColors::GoldDivider)
								]
							]

							// ---- Footer ----
							+ SVerticalBox::Slot().AutoHeight()
							[ BuildFooter() ]
						]
					]
				]
			]
		]

		// Layer 1: Invite popup (overlaid on top)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(InvitePopupBox, SVerticalBox)
			.Visibility(EVisibility::Collapsed)
		]

		// Layer 2: Settings popup (overlaid on top)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SAssignNew(SettingsPopupBox, SVerticalBox)
			.Visibility(EVisibility::Collapsed)
		]
	];

	ApplyLayout();
	RebuildMemberList();
}

// ============================================================
// Title bar — "Party - PartyName" with close button
// ============================================================

TSharedRef<SWidget> SPartyWidget::BuildTitleBar()
{
	return SNew(SBox)
		.HeightOverride(TitleBarHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText
				{
					UPartySubsystem* Sub = OwningSubsystem.Get();
					if (Sub && Sub->bInParty && !Sub->PartyName.IsEmpty())
					{
						return FText::FromString(FString::Printf(TEXT("Party - %s"), *Sub->PartyName));
					}
					return FText::FromString(TEXT("Party"));
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				.ColorAndOpacity(FSlateColor(GoldHighlight))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(PartyColors::TextShadow)
			]
			// Close button
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(PartyColors::ButtonBg)
				.Padding(FMargin(4.f, 1.f))
				.Cursor(EMouseCursor::Hand)
				.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
				{
					if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
					{
						UPartySubsystem* Sub = OwningSubsystem.Get();
						if (Sub) Sub->HideWidget();
						return FReply::Handled();
					}
					return FReply::Unhandled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(PartyColors::TextDim))
				]
			]
		];
}

// ============================================================
// Nav bar — context-sensitive buttons
// ============================================================

TSharedRef<SWidget> SPartyWidget::BuildNavBar()
{
	auto MakeNavButton = [this](const FString& Label, TFunction<void()> OnClick, TFunction<bool()> IsVisible) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(PartyColors::ButtonBg)
			.Padding(FMargin(6.f, 2.f))
			.Cursor(EMouseCursor::Hand)
			.Visibility_Lambda([IsVisible]() -> EVisibility
			{
				return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.OnMouseButtonDown_Lambda([OnClick](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
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
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
				.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
				.ShadowOffset(FVector2D(1, 1))
				.ShadowColorAndOpacity(PartyColors::TextShadow)
			];
	};

	TWeakObjectPtr<UPartySubsystem> WeakSub = OwningSubsystem;

	return SNew(SVerticalBox)

		// Row 1: Create party — text input + button (only when NOT in party)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 2.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([WeakSub]() -> EVisibility
			{
				UPartySubsystem* Sub = WeakSub.Get();
				return (Sub && !Sub->bInParty) ? EVisibility::Visible : EVisibility::Collapsed;
			})

			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)
			[
				SAssignNew(CreateNameInput, SEditableTextBox)
				.HintText(FText::FromString(TEXT("Party name...")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ForegroundColor(FSlateColor(PartyColors::TextPrimary))
				.BackgroundColor(FLinearColor(0.15f, 0.10f, 0.06f, 1.f))
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				MakeNavButton(TEXT("Create"),
					[this, WeakSub]()
					{
						if (UPartySubsystem* Sub = WeakSub.Get())
						{
							FString Name = CreateNameInput.IsValid()
								? CreateNameInput->GetText().ToString().TrimStartAndEnd()
								: FString();
							if (Name.IsEmpty()) Name = TEXT("Party");
							Sub->CreateParty(Name);
							if (CreateNameInput.IsValid()) CreateNameInput->SetText(FText::GetEmpty());
						}
					},
					[]() -> bool { return true; })
			]
		]

		// Row 2: Invite player — text input + button (only when leader)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 2.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([WeakSub]() -> EVisibility
			{
				UPartySubsystem* Sub = WeakSub.Get();
				return (Sub && Sub->bInParty && Sub->IsLeader()) ? EVisibility::Visible : EVisibility::Collapsed;
			})

			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)
			[
				SAssignNew(InviteNameInput, SEditableTextBox)
				.HintText(FText::FromString(TEXT("Player name...")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ForegroundColor(FSlateColor(PartyColors::TextPrimary))
				.BackgroundColor(FLinearColor(0.15f, 0.10f, 0.06f, 1.f))
			]

			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				MakeNavButton(TEXT("Invite"),
					[this, WeakSub]()
					{
						if (UPartySubsystem* Sub = WeakSub.Get())
						{
							FString Target = InviteNameInput.IsValid()
								? InviteNameInput->GetText().ToString().TrimStartAndEnd()
								: FString();
							if (!Target.IsEmpty())
							{
								Sub->InvitePlayer(Target);
								if (InviteNameInput.IsValid()) InviteNameInput->SetText(FText::GetEmpty());
							}
						}
					},
					[]() -> bool { return true; })
			]
		]

		// Row 3: Leave + Settings buttons (when in party)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 0.f, 4.f, 2.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([WeakSub]() -> EVisibility
			{
				UPartySubsystem* Sub = WeakSub.Get();
				return (Sub && Sub->bInParty) ? EVisibility::Visible : EVisibility::Collapsed;
			})

			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f)
			[
				MakeNavButton(TEXT("Leave"),
					[WeakSub]() {
						if (UPartySubsystem* Sub = WeakSub.Get())
							Sub->LeaveParty();
					},
					[]() -> bool { return true; })
			]

			+ SHorizontalBox::Slot().AutoWidth().Padding(1.f, 0.f)
			[
				MakeNavButton(TEXT("Settings"),
					[this]() {
						bSettingsOpen = !bSettingsOpen;
						RebuildSettingsPopup();
					},
					[WeakSub]() -> bool {
						UPartySubsystem* Sub = WeakSub.Get();
						return Sub && Sub->IsLeader();
					})
			]
		];
}

// ============================================================
// Footer — "Party" label
// ============================================================

TSharedRef<SWidget> SPartyWidget::BuildFooter()
{
	return SNew(SBox)
		.HeightOverride(FooterHeight)
		.Padding(FMargin(6.f, 2.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Party")))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(PartyColors::TextDim))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(PartyColors::TextShadow)
		];
}

// ============================================================
// Stat bar — reusable for HP and SP
// ============================================================

TSharedRef<SWidget> SPartyWidget::BuildStatBar(float Percent, const FLinearColor& FillColor)
{
	const float ClampedPercent = FMath::Clamp(Percent, 0.f, 1.f);
	const float EmptyFraction = FMath::Max(1.f - ClampedPercent, 0.001f);

	return SNew(SBox)
		.HeightOverride(7.f)
		[
			// Border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(HPBarBorder)
			.Padding(FMargin(1.f))
			[
				// Proportional fill using FillWidth slots
				SNew(SHorizontalBox)

				// Filled portion
				+ SHorizontalBox::Slot().FillWidth(ClampedPercent)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(FillColor)
				]

				// Empty portion (gray background)
				+ SHorizontalBox::Slot().FillWidth(EmptyFraction)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(HPBarBackground)
				]
			]
		];
}

// ============================================================
// Member row — name, HP numbers, HP bar, map name
// ============================================================

TSharedRef<SWidget> SPartyWidget::BuildMemberRow(const FPartyMember& Member)
{
	// Offline members: just name in gray, no HP/map
	if (!Member.bIsOnline)
	{
		return SNew(SBox)
			.HeightOverride(MemberRowHeight)
			.Padding(FMargin(6.f, 2.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Member.CharacterName))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FSlateColor(OfflineGray))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(PartyColors::TextShadow)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("(offline)")))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
					.ColorAndOpacity(FSlateColor(OfflineGray))
				]
			];
	}

	// Online member
	const FLinearColor NameColor = Member.bIsLeader ? LeaderNameBlue : MemberNameTeal;
	const FString NamePrefix = Member.bIsLeader ? FString(TEXT("\u2605 ")) : FString();
	const FString StyleName = Member.bIsLeader ? FString(TEXT("Bold")) : FString(TEXT("Regular"));

	const float HPPercent = (Member.MaxHP > 0)
		? (float)Member.HP / (float)Member.MaxHP
		: 0.f;
	const float SPPercent = (Member.MaxSP > 0)
		? (float)Member.SP / (float)Member.MaxSP
		: 0.f;

	const FString HPText = FString::Printf(TEXT("%d/%d"), Member.HP, Member.MaxHP);
	const FString SPText = FString::Printf(TEXT("%d/%d"), Member.SP, Member.MaxSP);
	const FString MapText = Member.MapName.IsEmpty()
		? FString()
		: FString::Printf(TEXT("(%s)"), *Member.MapName);

	// Capture character ID for right-click context menu
	const int32 MemberCharId = Member.CharacterId;
	const bool bMemberIsLeader = Member.bIsLeader;
	TWeakObjectPtr<UPartySubsystem> WeakSub = OwningSubsystem;

	TSharedRef<SWidget> Row = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.f)) // transparent; enables OnMouseButtonDown
		.Padding(FMargin(6.f, 2.f))
		.OnMouseButtonDown_Lambda([this, MemberCharId, bMemberIsLeader, WeakSub](const FGeometry& Geo, const FPointerEvent& Event) -> FReply
		{
			if (Event.GetEffectingButton() != EKeys::RightMouseButton)
				return FReply::Unhandled();

			UPartySubsystem* Sub = WeakSub.Get();
			if (!Sub || !Sub->bInParty)
				return FReply::Unhandled();

			// Build context menu
			FMenuBuilder MenuBuilder(true, nullptr);

			const bool bIsSelf = (MemberCharId == Sub->LeaderId && Sub->IsLeader())
				? false  // self check below
				: false;
			// Proper self check: compare against local character
			bool bClickedSelf = false;
			for (const FPartyMember& M : Sub->Members)
			{
				if (M.CharacterId == MemberCharId && M.bIsLeader && Sub->IsLeader())
				{
					bClickedSelf = true;
					break;
				}
			}
			// Actually just compare with subsystem's local char ID
			// The subsystem knows its own local character id via LeaderId matching
			// We need a simpler approach — check all members for the right-clicked one

			if (Sub->IsLeader() && !bMemberIsLeader)
			{
				// Leader right-clicked another member
				MenuBuilder.AddMenuEntry(
					FText::FromString(TEXT("Kick")),
					FText::FromString(TEXT("Remove from party")),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([WeakSub, MemberCharId]()
					{
						if (UPartySubsystem* S = WeakSub.Get())
							S->KickMember(MemberCharId);
					}))
				);

				MenuBuilder.AddMenuEntry(
					FText::FromString(TEXT("Delegate Leader")),
					FText::FromString(TEXT("Transfer party leadership")),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([WeakSub, MemberCharId]()
					{
						if (UPartySubsystem* S = WeakSub.Get())
							S->DelegateLeader(MemberCharId);
					}))
				);
			}

			// Everyone can leave
			MenuBuilder.AddMenuEntry(
				FText::FromString(TEXT("Leave Party")),
				FText::FromString(TEXT("Leave the current party")),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([WeakSub]()
				{
					if (UPartySubsystem* S = WeakSub.Get())
						S->LeaveParty();
				}))
			);

			if (FSlateApplication::IsInitialized())
			{
				TSharedRef<SWidget> MenuWidget = MenuBuilder.MakeWidget();
				FSlateApplication::Get().PushMenu(
					SharedThis(this),
					FWidgetPath(),
					MenuWidget,
					Event.GetScreenSpacePosition(),
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
				);
			}

			return FReply::Handled();
		})
		[
			SNew(SVerticalBox)

			// Row 1: Name + HP numbers
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				// Name with optional leader star
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(NamePrefix + Member.CharacterName))
					.Font(FCoreStyle::GetDefaultFontStyle(*StyleName, 9))
					.ColorAndOpacity(FSlateColor(NameColor))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(PartyColors::TextShadow)
				]

				// HP/SP numbers right-aligned
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("HP %s  SP %s"), *HPText, *SPText)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(PartyColors::HPTextBlack))
				]
			]

			// Row 2: HP bar
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f, 0.f, 0.f)
			[
				BuildStatBar(HPPercent, (HPPercent > 0.25f) ? HPBarGreen : HPBarRed)
			]

			// Row 3: SP bar
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f, 0.f, 0.f)
			[
				BuildStatBar(SPPercent, PartyColors::SPBarBlue)
			]

			// Row 4: Map name
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(MapText))
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
				.ColorAndOpacity(FSlateColor(PartyColors::MapTextGray))
				.Visibility(MapText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
			]
		];

	return Row;
}

// ============================================================
// Rebuild member list from subsystem data
// ============================================================

void SPartyWidget::RebuildMemberList()
{
	if (!MemberListBox.IsValid()) return;

	MemberListBox->ClearChildren();

	UPartySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->bInParty || Sub->Members.Num() == 0)
	{
		// Show "Not in a party" placeholder
		MemberListBox->AddSlot()
		.AutoHeight()
		.Padding(6.f, 8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Not in a party.")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
			.ColorAndOpacity(FSlateColor(PartyColors::TextDim))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(PartyColors::TextShadow)
		];
		return;
	}

	for (const FPartyMember& Member : Sub->Members)
	{
		MemberListBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 1.f)
		[
			BuildMemberRow(Member)
		];

		// Subtle divider between members
		MemberListBox->AddSlot()
		.AutoHeight()
		[
			SNew(SBox).HeightOverride(1.f).Padding(FMargin(8.f, 0.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(FLinearColor(0.35f, 0.24f, 0.14f, 0.5f))
			]
		];
	}
}

// ============================================================
// Rebuild invite popup overlay
// ============================================================

void SPartyWidget::RebuildInvitePopup()
{
	if (!InvitePopupBox.IsValid()) return;

	InvitePopupBox->ClearChildren();

	UPartySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !Sub->bHasPendingInvite)
	{
		InvitePopupBox->SetVisibility(EVisibility::Collapsed);
		return;
	}

	InvitePopupBox->SetVisibility(EVisibility::Visible);

	TWeakObjectPtr<UPartySubsystem> WeakSub = OwningSubsystem;

	const FString InviteText = FString::Printf(
		TEXT("%s invites you to %s"),
		*Sub->PendingInviterName,
		*Sub->PendingInvitePartyName
	);

	InvitePopupBox->AddSlot()
	.AutoHeight()
	[
		// 3-layer popup frame
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(GoldTrim)
		.Padding(FMargin(2.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(PanelDark)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(PartyColors::InviteBg)
				.Padding(FMargin(10.f, 8.f))
				[
					SNew(SVerticalBox)

					// Invite text
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(InviteText))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor(GoldHighlight))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(PartyColors::TextShadow)
						.AutoWrapText(true)
						.WrapTextAt(200.f)
					]

					// Accept / Decline buttons
					+ SVerticalBox::Slot().AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(PartyColors::AcceptGreen)
							.Padding(FMargin(12.f, 4.f))
							.Cursor(EMouseCursor::Hand)
							.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
							{
								if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
								{
									if (UPartySubsystem* S = WeakSub.Get())
										S->RespondToInvite(true);
									return FReply::Handled();
								}
								return FReply::Unhandled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Accept")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FSlateColor(FLinearColor::White))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(PartyColors::TextShadow)
							]
						]

						+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(PartyColors::DeclineRed)
							.Padding(FMargin(12.f, 4.f))
							.Cursor(EMouseCursor::Hand)
							.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
							{
								if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
								{
									if (UPartySubsystem* S = WeakSub.Get())
										S->RespondToInvite(false);
									return FReply::Handled();
								}
								return FReply::Unhandled();
							})
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("Decline")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity(FSlateColor(FLinearColor::White))
								.ShadowOffset(FVector2D(1, 1))
								.ShadowColorAndOpacity(PartyColors::TextShadow)
							]
						]
					]
				]
			]
		]
	];
}

// ============================================================
// Settings popup — EXP share mode toggle
// ============================================================

void SPartyWidget::RebuildSettingsPopup()
{
	if (!SettingsPopupBox.IsValid()) return;

	SettingsPopupBox->ClearChildren();

	if (!bSettingsOpen)
	{
		SettingsPopupBox->SetVisibility(EVisibility::Collapsed);
		return;
	}

	SettingsPopupBox->SetVisibility(EVisibility::Visible);

	TWeakObjectPtr<UPartySubsystem> WeakSub = OwningSubsystem;

	// Determine current mode
	FString CurrentMode;
	FString CurrentItemShare;
	FString CurrentItemDistribute;
	if (UPartySubsystem* Sub = WeakSub.Get())
	{
		CurrentMode = Sub->ExpShare;
		CurrentItemShare = Sub->ItemShare;
		CurrentItemDistribute = Sub->ItemDistribute;
	}

	auto MakeOptionButton = [this, WeakSub](const FString& Label, const FString& Description, const FString& Mode, bool bIsActive) -> TSharedRef<SWidget>
	{
		const FLinearColor BgColor = bIsActive
			? FLinearColor(0.20f, 0.45f, 0.20f, 1.f)
			: PartyColors::ButtonBg;

		return SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(BgColor)
			.Padding(FMargin(8.f, 4.f))
			.Cursor(EMouseCursor::Hand)
			.OnMouseButtonDown_Lambda([this, WeakSub, Mode](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					if (UPartySubsystem* Sub = WeakSub.Get())
						Sub->ChangeExpShare(Mode);
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(bIsActive ? TEXT("[x]") : TEXT("[ ]")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
						.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor(GoldHighlight))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Description))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(PartyColors::TextDim))
					.AutoWrapText(true)
					.WrapTextAt(180.f)
				]
			];
	};

	SettingsPopupBox->AddSlot()
	.AutoHeight()
	[
		// 3-layer popup frame
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.BorderBackgroundColor(GoldTrim)
		.Padding(FMargin(2.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(PanelDark)
			.Padding(FMargin(1.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(PartyColors::InviteBg)
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SVerticalBox)

					// Title
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Party Settings")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FSlateColor(GoldHighlight))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(PartyColors::TextShadow)
					]

					// Divider
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
					[
						SNew(SBox).HeightOverride(1.f)
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(PartyColors::GoldDivider)
						]
					]

					// EXP Share label
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("EXP Distribution:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
					]

					// Each Take option
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						MakeOptionButton(
							TEXT("Each Take"),
							TEXT("Only the killer receives EXP."),
							TEXT("each_take"),
							CurrentMode != TEXT("even_share"))
					]

					// Even Share option
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						MakeOptionButton(
							TEXT("Even Share"),
							TEXT("EXP split evenly with +20% bonus per member. Requires <15 level gap."),
							TEXT("even_share"),
							CurrentMode == TEXT("even_share"))
					]

					// ── Item Pickup label ──
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Item Pickup:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
					]

					// Each Take (items)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CurrentItemShare != TEXT("party_share") ? FLinearColor(0.20f, 0.45f, 0.20f, 1.f) : PartyColors::ButtonBg)
						.Padding(FMargin(8.f, 4.f))
						.Cursor(EMouseCursor::Hand)
						.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								if (UPartySubsystem* Sub = WeakSub.Get()) Sub->ChangeItemShare(TEXT("each_take"));
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
								[ SNew(STextBlock).Text(FText::FromString(CurrentItemShare != TEXT("party_share") ? TEXT("[x]") : TEXT("[ ]"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).ColorAndOpacity(FSlateColor(PartyColors::TextPrimary)) ]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[ SNew(STextBlock).Text(FText::FromString(TEXT("Each Take"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(GoldHighlight)) ]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 0.f, 0.f, 0.f)
							[ SNew(STextBlock).Text(FText::FromString(TEXT("Only damage dealers can loot during priority window."))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 7)).ColorAndOpacity(FSlateColor(PartyColors::TextDim)).AutoWrapText(true).WrapTextAt(180.f) ]
						]
					]

					// Party Share (items)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CurrentItemShare == TEXT("party_share") ? FLinearColor(0.20f, 0.45f, 0.20f, 1.f) : PartyColors::ButtonBg)
						.Padding(FMargin(8.f, 4.f))
						.Cursor(EMouseCursor::Hand)
						.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								if (UPartySubsystem* Sub = WeakSub.Get()) Sub->ChangeItemShare(TEXT("party_share"));
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
								[ SNew(STextBlock).Text(FText::FromString(CurrentItemShare == TEXT("party_share") ? TEXT("[x]") : TEXT("[ ]"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).ColorAndOpacity(FSlateColor(PartyColors::TextPrimary)) ]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[ SNew(STextBlock).Text(FText::FromString(TEXT("Party Share"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(GoldHighlight)) ]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 0.f, 0.f, 0.f)
							[ SNew(STextBlock).Text(FText::FromString(TEXT("Any party member can pick up dropped items immediately."))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 7)).ColorAndOpacity(FSlateColor(PartyColors::TextDim)).AutoWrapText(true).WrapTextAt(180.f) ]
						]
					]

					// ── Item Distribution label ──
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Item Distribution:")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
					]

					// Individual
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CurrentItemDistribute != TEXT("shared") ? FLinearColor(0.20f, 0.45f, 0.20f, 1.f) : PartyColors::ButtonBg)
						.Padding(FMargin(8.f, 4.f))
						.Cursor(EMouseCursor::Hand)
						.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								if (UPartySubsystem* Sub = WeakSub.Get()) Sub->ChangeItemDistribute(TEXT("individual"));
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
								[ SNew(STextBlock).Text(FText::FromString(CurrentItemDistribute != TEXT("shared") ? TEXT("[x]") : TEXT("[ ]"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).ColorAndOpacity(FSlateColor(PartyColors::TextPrimary)) ]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[ SNew(STextBlock).Text(FText::FromString(TEXT("Individual"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(GoldHighlight)) ]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 0.f, 0.f, 0.f)
							[ SNew(STextBlock).Text(FText::FromString(TEXT("You keep whatever you pick up."))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 7)).ColorAndOpacity(FSlateColor(PartyColors::TextDim)).AutoWrapText(true).WrapTextAt(180.f) ]
						]
					]

					// Shared
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 1.f)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(CurrentItemDistribute == TEXT("shared") ? FLinearColor(0.20f, 0.45f, 0.20f, 1.f) : PartyColors::ButtonBg)
						.Padding(FMargin(8.f, 4.f))
						.Cursor(EMouseCursor::Hand)
						.OnMouseButtonDown_Lambda([WeakSub](const FGeometry&, const FPointerEvent& Event) -> FReply
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								if (UPartySubsystem* Sub = WeakSub.Get()) Sub->ChangeItemDistribute(TEXT("shared"));
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
								[ SNew(STextBlock).Text(FText::FromString(CurrentItemDistribute == TEXT("shared") ? TEXT("[x]") : TEXT("[ ]"))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 8)).ColorAndOpacity(FSlateColor(PartyColors::TextPrimary)) ]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
								[ SNew(STextBlock).Text(FText::FromString(TEXT("Shared"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 9)).ColorAndOpacity(FSlateColor(GoldHighlight)) ]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(18.f, 0.f, 0.f, 0.f)
							[ SNew(STextBlock).Text(FText::FromString(TEXT("Picked up items randomly go to a party member in range."))).Font(FCoreStyle::GetDefaultFontStyle("Regular", 7)).ColorAndOpacity(FSlateColor(PartyColors::TextDim)).AutoWrapText(true).WrapTextAt(180.f) ]
						]
					]

					// Close button
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
					.HAlign(HAlign_Center)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(PartyColors::ButtonBg)
						.Padding(FMargin(16.f, 3.f))
						.Cursor(EMouseCursor::Hand)
						.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event) -> FReply
						{
							if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								bSettingsOpen = false;
								RebuildSettingsPopup();
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Close")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FSlateColor(PartyColors::TextPrimary))
						]
					]
				]
			]
		]
	];
}

// ============================================================
// Tick — version-driven rebuild
// ============================================================

void SPartyWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UPartySubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return;

	// Rebuild member list and settings popup when data version changes
	if (Sub->DataVersion != LastRenderedVersion)
	{
		LastRenderedVersion = Sub->DataVersion;
		RebuildMemberList();
		if (bSettingsOpen) RebuildSettingsPopup();
	}

	// Show/hide invite popup when invite state changes
	if (Sub->bHasPendingInvite != bLastHadInvite)
	{
		bLastHadInvite = Sub->bHasPendingInvite;
		RebuildInvitePopup();
	}
}

// ============================================================
// Drag — title bar only
// ============================================================

FReply SPartyWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		return FReply::Unhandled();

	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	// Title bar drag zone
	if (LocalPos.Y < TitleBarHeight)
	{
		bIsDragging = true;
		DragOffset = MouseEvent.GetScreenSpacePosition();
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SPartyWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SPartyWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f)
			? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X)
			: 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Layout
// ============================================================

void SPartyWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}
