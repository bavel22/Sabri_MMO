// SSkillTreeWidget.cpp — Draggable Slate Skill Tree HUD (RO Classic brown/gold theme)
// REWRITE v2: Deferred rebuilds — tab clicks and data updates set flags, Tick() performs
// the actual widget-tree mutation safely outside Slate event processing.
// All lambda captures use TWeakObjectPtr<USkillTreeSubsystem> to prevent
// dangling pointer crashes during Slate paint/layout when subsystem is GC'd.

#include "SSkillTreeWidget.h"
#include "SkillTreeSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"

// ============================================================
// RO Classic Color Palette
// ============================================================
namespace SKColors
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
	static const FLinearColor TextGreen     (0.30f, 0.85f, 0.30f, 1.f);
	static const FLinearColor TextRed       (0.85f, 0.25f, 0.25f, 1.f);
	static const FLinearColor SkillActive   (0.28f, 0.42f, 0.60f, 1.f);
	static const FLinearColor SkillPassive  (0.35f, 0.50f, 0.30f, 1.f);
	static const FLinearColor SkillLocked   (0.25f, 0.20f, 0.15f, 0.8f);
	static const FLinearColor SkillLearned  (0.18f, 0.35f, 0.18f, 1.f);
	static const FLinearColor SkillMaxed    (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor TabActive     (0.50f, 0.38f, 0.15f, 1.f);
	static const FLinearColor TabInactive   (0.28f, 0.19f, 0.10f, 1.f);
	static const FLinearColor ButtonLearn   (0.22f, 0.50f, 0.22f, 1.f);
	static const FLinearColor ButtonReset   (0.60f, 0.20f, 0.15f, 1.f);
}

// ============================================================
// Helpers
// ============================================================
static TSharedRef<SWidget> MakeGoldDivider()
{
	return SNew(SBox)
		.HeightOverride(1.f)
		.Padding(FMargin(2.f, 1.f))
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SKColors::GoldDivider)
		];
}

static FString PrettifyClassName(const FString& ClassId)
{
	if (ClassId == TEXT("novice")) return TEXT("Novice");
	if (ClassId == TEXT("swordsman")) return TEXT("Swordsman");
	if (ClassId == TEXT("mage")) return TEXT("Mage");
	if (ClassId == TEXT("archer")) return TEXT("Archer");
	if (ClassId == TEXT("acolyte")) return TEXT("Acolyte");
	if (ClassId == TEXT("thief")) return TEXT("Thief");
	if (ClassId == TEXT("merchant")) return TEXT("Merchant");
	if (ClassId == TEXT("knight")) return TEXT("Knight");
	if (ClassId == TEXT("crusader")) return TEXT("Crusader");
	if (ClassId == TEXT("wizard")) return TEXT("Wizard");
	if (ClassId == TEXT("sage")) return TEXT("Sage");
	if (ClassId == TEXT("hunter")) return TEXT("Hunter");
	if (ClassId == TEXT("bard")) return TEXT("Bard");
	if (ClassId == TEXT("dancer")) return TEXT("Dancer");
	if (ClassId == TEXT("priest")) return TEXT("Priest");
	if (ClassId == TEXT("monk")) return TEXT("Monk");
	if (ClassId == TEXT("assassin")) return TEXT("Assassin");
	if (ClassId == TEXT("rogue")) return TEXT("Rogue");
	if (ClassId == TEXT("blacksmith")) return TEXT("Blacksmith");
	if (ClassId == TEXT("alchemist")) return TEXT("Alchemist");
	FString Result = ClassId;
	if (Result.Len() > 0) Result[0] = FChar::ToUpper(Result[0]);
	return Result;
}

// ============================================================
// Safe subsystem access
// ============================================================
USkillTreeSubsystem* SSkillTreeWidget::GetSub() const
{
	return OwningSubsystem.IsValid() ? OwningSubsystem.Get() : nullptr;
}

// ============================================================
// Construction
// ============================================================
void SSkillTreeWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	// Weak pointer captured by ALL lambdas in this widget — never a raw pointer
	TWeakObjectPtr<USkillTreeSubsystem> WeakSub = OwningSubsystem;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(CurrentSize.X)
		.HeightOverride(CurrentSize.Y)
		[
			// Outer gold trim border
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SKColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				// Inner dark inset
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(SKColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					// Main brown panel
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(SKColors::PanelBrown)
					.Padding(FMargin(0.f))
					[
						SNew(SVerticalBox)

						// --- Title Bar ---
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(SKColors::PanelDark)
							.Padding(FMargin(6.f, 3.f))
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("Skill Tree")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
									.ColorAndOpacity(FSlateColor(SKColors::GoldHighlight))
								]

								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNullWidget::NullWidget
								]

								// Skill points display (lambda uses weak ptr)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text_Lambda([WeakSub]() -> FText {
										USkillTreeSubsystem* S = WeakSub.Get();
										if (!S) return FText::GetEmpty();
										return FText::FromString(FString::Printf(TEXT("Points: %d"), S->SkillPoints));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
									.ColorAndOpacity(FSlateColor(SKColors::TextGreen))
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(6.f, 0.f, 0.f, 0.f)
								.VAlign(VAlign_Center)
								[
									SNew(SButton)
									.ButtonStyle(FCoreStyle::Get(), "NoBorder")
									.ContentPadding(FMargin(2.f))
									.OnClicked_Lambda([WeakSub]() -> FReply {
										if (USkillTreeSubsystem* S = WeakSub.Get()) S->HideWidget();
										return FReply::Handled();
									})
									[
										SNew(STextBlock)
										.Text(FText::FromString(TEXT("X")))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
										.ColorAndOpacity(FSlateColor(SKColors::TextRed))
									]
								]
							]
						]

						// --- Class Tabs (dynamically rebuilt) ---
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SAssignNew(ClassTabsContainer, SVerticalBox)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeGoldDivider()
						]

						// --- Scrollable skill content ---
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SNew(SScrollBox)
							.Orientation(Orient_Vertical)
							+ SScrollBox::Slot()
							[
								SAssignNew(SkillContentBox, SVerticalBox)
							]
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeGoldDivider()
						]

						// --- Bottom bar: skill points + reset ---
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.BorderBackgroundColor(SKColors::PanelDark)
							.Padding(FMargin(6.f, 3.f))
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text_Lambda([WeakSub]() -> FText {
										USkillTreeSubsystem* S = WeakSub.Get();
										if (!S) return FText::GetEmpty();
										return FText::FromString(FString::Printf(TEXT("Remaining Skill Points: %d"), S->SkillPoints));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
									.ColorAndOpacity(FSlateColor(SKColors::TextPrimary))
								]

								+ SHorizontalBox::Slot()
								.FillWidth(1.f)
								[
									SNullWidget::NullWidget
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(SButton)
									.ButtonStyle(FCoreStyle::Get(), "NoBorder")
									.ContentPadding(FMargin(2.f))
									.OnClicked_Lambda([WeakSub]() -> FReply {
										if (USkillTreeSubsystem* S = WeakSub.Get()) S->ResetAllSkills();
										return FReply::Handled();
									})
									[
										SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
										.BorderBackgroundColor(SKColors::ButtonReset)
										.Padding(FMargin(8.f, 2.f))
										[
											SNew(STextBlock)
											.Text(FText::FromString(TEXT("Reset All")))
											.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
											.ColorAndOpacity(FSlateColor(SKColors::TextBright))
										]
									]
								]
							]
						]
					]
				]
			]
		]
	];

	ApplyLayout();

	// Schedule initial build for next Tick (not during Construct)
	bPendingFullRebuild = true;
}

// ============================================================
// Public API — just set flags, actual work is deferred to Tick()
// ============================================================
void SSkillTreeWidget::RebuildSkillContent()
{
	bPendingFullRebuild = true;
}

void SSkillTreeWidget::RebuildSkillGrid()
{
	bPendingGridRebuild = true;
}

// ============================================================
// Tick — performs deferred rebuilds safely outside Slate event/paint processing
// ============================================================
void SSkillTreeWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Full rebuild supersedes grid-only rebuild
	if (bPendingFullRebuild)
	{
		bPendingFullRebuild = false;
		bPendingGridRebuild = false;
		DoRebuildSkillContent();
	}
	else if (bPendingGridRebuild)
	{
		bPendingGridRebuild = false;
		DoRebuildSkillGrid();
	}
}

// ============================================================
// DoRebuildSkillContent — full (tabs + grid), called from Tick only
// ============================================================
void SSkillTreeWidget::DoRebuildSkillContent()
{
	if (!ClassTabsContainer.IsValid()) return;

	USkillTreeSubsystem* Sub = GetSub();

	ClassTabsContainer->ClearChildren();

	if (!Sub || Sub->SkillGroups.Num() == 0)
	{
		// Show loading state
		ClassTabsContainer->AddSlot()
		.AutoHeight()
		.Padding(4.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Loading skills...")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
			.ColorAndOpacity(FSlateColor(SKColors::GoldHighlight))
		];
	}
	else
	{
		// Build class tabs using a TSharedRef to this widget for safe lambda capture
		TSharedRef<SHorizontalBox> TabRow = SNew(SHorizontalBox);
		TWeakPtr<SSkillTreeWidget> WeakSelf = SharedThis(this);

		for (int32 i = 0; i < Sub->SkillGroups.Num(); ++i)
		{
			const FString ClassName = PrettifyClassName(Sub->SkillGroups[i].ClassId);
			const int32 TabIndex = i;

			// Tab click: set flag + index, Tick() will call DoRebuildSkillGrid() next frame
			// CRITICAL: Do NOT call DoRebuildSkillGrid() here — we are inside Slate event processing
			TabRow->AddSlot()
			.AutoWidth()
			.Padding(1.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(FCoreStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(6.f, 3.f))
				.OnClicked_Lambda([WeakSelf, TabIndex]() -> FReply {
					TSharedPtr<SSkillTreeWidget> Pin = WeakSelf.Pin();
					if (Pin.IsValid())
					{
						Pin->ActiveClassTab = TabIndex;
						Pin->bPendingGridRebuild = true;
					}
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor_Lambda([WeakSelf, TabIndex]() -> FSlateColor {
						TSharedPtr<SSkillTreeWidget> Pin = WeakSelf.Pin();
						if (Pin.IsValid() && Pin->ActiveClassTab == TabIndex)
							return FSlateColor(SKColors::TabActive);
						return FSlateColor(SKColors::TabInactive);
					})
					.Padding(FMargin(6.f, 2.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(ClassName))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity_Lambda([WeakSelf, TabIndex]() -> FSlateColor {
							TSharedPtr<SSkillTreeWidget> Pin = WeakSelf.Pin();
							if (Pin.IsValid() && Pin->ActiveClassTab == TabIndex)
								return FSlateColor(SKColors::TextBright);
							return FSlateColor(SKColors::TextDim);
						})
					]
				]
			];
		}

		ClassTabsContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(SKColors::PanelMedium)
			.Padding(FMargin(2.f, 1.f))
			[
				TabRow
			]
		];
	}

	DoRebuildSkillGrid();
}

// ============================================================
// DoRebuildSkillGrid — grid only, called from Tick or DoRebuildSkillContent
// ============================================================
void SSkillTreeWidget::DoRebuildSkillGrid()
{
	if (!SkillContentBox.IsValid()) return;
	SkillContentBox->ClearChildren();

	USkillTreeSubsystem* Sub = GetSub();
	if (!Sub || Sub->SkillGroups.Num() == 0)
	{
		SkillContentBox->AddSlot()
		.AutoHeight()
		.Padding(8.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Loading skill tree...")))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
			.ColorAndOpacity(FSlateColor(SKColors::GoldHighlight))
		];
		return;
	}

	ActiveClassTab = FMath::Clamp(ActiveClassTab, 0, Sub->SkillGroups.Num() - 1);
	const FSkillClassGroup& Group = Sub->SkillGroups[ActiveClassTab];

	// Sort skills by treeRow, then treeCol
	TArray<FSkillEntry> SortedSkills = Group.Skills;
	SortedSkills.Sort([](const FSkillEntry& A, const FSkillEntry& B) {
		if (A.TreeRow != B.TreeRow) return A.TreeRow < B.TreeRow;
		return A.TreeCol < B.TreeCol;
	});

	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox).UseAllottedSize(true);
	TWeakObjectPtr<USkillTreeSubsystem> WeakSub = OwningSubsystem;
	TWeakPtr<SSkillTreeWidget> WeakSelf = SharedThis(this);

	for (const FSkillEntry& Skill : SortedSkills)
	{
		const int32 SkillId = Skill.SkillId;
		const bool bLearned = Skill.CurrentLevel > 0;
		const bool bMaxed = Skill.CurrentLevel >= Skill.MaxLevel;
		const bool bCanLearn = Skill.bCanLearn;

		// Slot background color
		FLinearColor SlotBg;
		if (bMaxed)
			SlotBg = SKColors::SkillMaxed;
		else if (bLearned)
			SlotBg = SKColors::SkillLearned;
		else if (bCanLearn)
			SlotBg = (Skill.Type == TEXT("passive")) ? SKColors::SkillPassive : SKColors::SkillActive;
		else
			SlotBg = SKColors::SkillLocked;

		// Type label
		FString TypeLabel;
		if (Skill.Type == TEXT("passive")) TypeLabel = TEXT("[P]");
		else if (Skill.Type == TEXT("toggle")) TypeLabel = TEXT("[T]");
		else TypeLabel = TEXT("[A]");

		FString LevelText = FString::Printf(TEXT("Lv %d/%d"), Skill.CurrentLevel, Skill.MaxLevel);
		FString SpText;
		if (Skill.Type != TEXT("passive") && Skill.SpCost > 0)
		{
			SpText = FString::Printf(TEXT("SP: %d"), bLearned ? Skill.SpCost : Skill.NextSpCost);
		}

		// Try to load icon brush (safe — returns nullptr if subsystem gone)
		FSlateBrush* IconBrush = Sub->GetOrCreateIconBrush(Skill.IconPath);

		// Draggable icon (learned active/toggle skills only)
		const bool bDraggableSkill = bLearned && Skill.Type != TEXT("passive") && IconBrush != nullptr;
		const FString CapIconPath = Skill.IconPath;
		const FString CapDisplayName = Skill.DisplayName;

		// --- Build skill slot ---
		TSharedRef<SVerticalBox> SlotContent = SNew(SVerticalBox)

			// Row 1: Icon + Name
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 3.f, 0.f)
				[
					IconBrush
					? (bDraggableSkill
						? TSharedRef<SWidget>(
							SNew(SBox)
							.WidthOverride(24.f)
							.HeightOverride(24.f)
							.Cursor(EMouseCursor::GrabHand)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
								.Padding(0)
								.OnMouseButtonDown_Lambda([WeakSelf, SkillId, CapDisplayName, CapIconPath](const FGeometry&, const FPointerEvent& Event) -> FReply {
									if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
									TSharedPtr<SSkillTreeWidget> Pin = WeakSelf.Pin();
									if (Pin.IsValid())
									{
										Pin->bSkillDragInitiated = true;
										Pin->DragSourceSkillId = SkillId;
										Pin->DragSourceSkillName = CapDisplayName;
										Pin->DragSourceSkillIcon = CapIconPath;
										Pin->SkillDragStartPos = Event.GetScreenSpacePosition();
										return FReply::Handled().CaptureMouse(Pin.ToSharedRef());
									}
									return FReply::Unhandled();
								})
								[
									SNew(SImage).Image(IconBrush)
								]
							]
						)
						: TSharedRef<SWidget>(
							SNew(SBox)
							.WidthOverride(24.f)
							.HeightOverride(24.f)
							[
								SNew(SImage).Image(IconBrush)
							]
						)
					)
					: TSharedRef<SWidget>(
						SNew(STextBlock)
						.Text(FText::FromString(TypeLabel))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
						.ColorAndOpacity(FSlateColor(SKColors::TextDim))
					)
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Skill.DisplayName))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
					.ColorAndOpacity(FSlateColor(SKColors::TextBright))
					.AutoWrapText(true)
				]
			]

			// Row 2: Level + SP
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 1.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(LevelText))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
					.ColorAndOpacity(FSlateColor(bMaxed ? SKColors::GoldHighlight : SKColors::TextPrimary))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[ SNullWidget::NullWidget ]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(SpText))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
					.ColorAndOpacity(FSlateColor(SKColors::TextDim))
				]
			]

			// Row 3: Description
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Skill.Description))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
				.ColorAndOpacity(FSlateColor(SKColors::TextDim))
				.AutoWrapText(true)
				.LineHeightPercentage(1.1f)
			];

		// Row 4: Learn button (only if can learn and not maxed)
		if (bCanLearn && !bMaxed)
		{
			SlotContent->AddSlot()
			.AutoHeight()
			.Padding(0.f, 3.f, 0.f, 0.f)
			[
				SNew(SButton)
				.ButtonStyle(FCoreStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(4.f, 1.f))
				.OnClicked_Lambda([WeakSub, SkillId]() -> FReply {
					if (USkillTreeSubsystem* S = WeakSub.Get()) S->LearnSkill(SkillId);
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(SKColors::ButtonLearn)
					.Padding(FMargin(6.f, 1.f))
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(bLearned ? TEXT("Level Up") : TEXT("Learn")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(SKColors::TextBright))
					]
				]
			];
		}

		// Row 5: Hotbar quick-assign [1]-[9] (only for learned active/toggle skills)
		if (bLearned && Skill.Type != TEXT("passive"))
		{
			TSharedRef<SHorizontalBox> HotbarRow = SNew(SHorizontalBox);

			HotbarRow->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Bar:")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 5))
				.ColorAndOpacity(FSlateColor(SKColors::TextDim))
			];

			for (int32 Slot = 0; Slot < 9; ++Slot)
			{
				const int32 SlotIndex = Slot + 1;
				HotbarRow->AddSlot()
				.AutoWidth()
				.Padding(1.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0.f))
					.OnClicked_Lambda([WeakSub, SkillId, SlotIndex, CapDisplayName]() -> FReply {
						if (USkillTreeSubsystem* S = WeakSub.Get())
							S->AssignSkillToHotbar(SkillId, CapDisplayName, SlotIndex);
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(SKColors::PanelMedium)
						.Padding(FMargin(3.f, 1.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%d"), SlotIndex)))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
							.ColorAndOpacity(FSlateColor(SKColors::GoldHighlight))
						]
					]
				];
			}

			SlotContent->AddSlot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				HotbarRow
			];
		}

		// Wrap slot in borders
		WrapBox->AddSlot()
		.Padding(3.f)
		[
			SNew(SBox)
			.WidthOverride(140.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(SKColors::GoldDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(SlotBg)
					.Padding(FMargin(4.f, 3.f))
					[
						SlotContent
					]
				]
			]
		];
	}

	SkillContentBox->AddSlot()
	.AutoHeight()
	.Padding(4.f)
	[
		WrapBox
	];

	Invalidate(EInvalidateWidgetReason::Layout);
}

// ============================================================
// Drag handling
// ============================================================
void SSkillTreeWidget::ApplyLayout()
{
	SetRenderTransform(FSlateRenderTransform(WidgetPosition));
}

FReply SSkillTreeWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		if (LocalPos.Y < 28.f)
		{
			bIsDragging = true;
			DragOffset = MouseEvent.GetScreenSpacePosition();
			DragStartWidgetPos = WidgetPosition;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
	}
	return FReply::Handled();
}

FReply SSkillTreeWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsDragging)
		{
			bIsDragging = false;
			return FReply::Handled().ReleaseMouseCapture();
		}

		// Cancel skill drag initiation (mouse released before threshold)
		if (bSkillDragInitiated)
		{
			bSkillDragInitiated = false;
			DragSourceSkillId = 0;
			DragSourceSkillName.Empty();
			DragSourceSkillIcon.Empty();
			return FReply::Handled().ReleaseMouseCapture();
		}
	}
	return FReply::Handled();
}

FReply SSkillTreeWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}

	// Skill drag threshold detection
	if (bSkillDragInitiated && DragSourceSkillId > 0)
	{
		float Distance = FVector2D::Distance(MouseEvent.GetScreenSpacePosition(), SkillDragStartPos);
		if (Distance > SkillDragThreshold)
		{
			USkillTreeSubsystem* Sub = GetSub();
			if (Sub)
			{
				Sub->StartSkillDrag(DragSourceSkillId, DragSourceSkillName, DragSourceSkillIcon);
			}
			bSkillDragInitiated = false;
			DragSourceSkillId = 0;
			DragSourceSkillName.Empty();
			DragSourceSkillIcon.Empty();
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
