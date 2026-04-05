// SSkillTreeWidget.cpp — Draggable Slate Skill Tree HUD (RO Classic brown/gold theme)
// REWRITE v3: Grid layout with prerequisite connecting lines, compact cells, hover tooltips.
// Inner SSkillGridPanel handles OnPaint for line drawing relative to its own geometry.

#include "SSkillTreeWidget.h"
#include "SSkillTooltipWidget.h"
#include "SkillTreeSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
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
// SSkillGridPanel — simple container for the skill grid
// ============================================================
class SSkillGridPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSkillGridPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SAssignNew(ContentBox, SVerticalBox)
		];
	}

	TSharedPtr<SVerticalBox> ContentBox;
};

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

								// Skill points display
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

						// --- Scrollable skill grid ---
						+ SVerticalBox::Slot()
						.FillHeight(1.f)
						[
							SAssignNew(SkillScrollBox, SScrollBox)
							.Orientation(Orient_Vertical)
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

	// Schedule initial build for next Tick
	bPendingFullRebuild = true;
}

// ============================================================
// Public API — set flags, actual work deferred to Tick()
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
// DoRebuildSkillContent — full rebuild (tabs + grid), called from Tick only
// ============================================================
void SSkillTreeWidget::DoRebuildSkillContent()
{
	if (!ClassTabsContainer.IsValid()) return;

	USkillTreeSubsystem* Sub = GetSub();

	ClassTabsContainer->ClearChildren();

	if (!Sub || Sub->SkillGroups.Num() == 0)
	{
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
		TSharedRef<SHorizontalBox> TabRow = SNew(SHorizontalBox);
		TWeakPtr<SSkillTreeWidget> WeakSelf = SharedThis(this);

		for (int32 i = 0; i < Sub->SkillGroups.Num(); ++i)
		{
			const FString ClassName = PrettifyClassName(Sub->SkillGroups[i].ClassId);
			const int32 TabIndex = i;

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
// DoRebuildSkillGrid — grid layout with compact cells + prerequisite lines
// ============================================================
void SSkillTreeWidget::DoRebuildSkillGrid()
{
	if (!SkillScrollBox.IsValid()) return;
	SkillScrollBox->ClearChildren();

	USkillTreeSubsystem* Sub = GetSub();
	if (!Sub || Sub->SkillGroups.Num() == 0)
	{
		SkillScrollBox->AddSlot()
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

	// Build 2D lookup: SkillGrid[row][col] -> skill pointer
	TMap<int32, TMap<int32, const FSkillEntry*>> SkillGrid;
	int32 MaxRow = 0, MaxCol = 0;
	for (const FSkillEntry& Skill : Group.Skills)
	{
		SkillGrid.FindOrAdd(Skill.TreeRow).Add(Skill.TreeCol, &Skill);
		MaxRow = FMath::Max(MaxRow, Skill.TreeRow);
		MaxCol = FMath::Max(MaxCol, Skill.TreeCol);
	}

	// Create the grid panel
	GridPanel = SNew(SSkillGridPanel);

	TWeakObjectPtr<USkillTreeSubsystem> WeakSub = OwningSubsystem;
	TWeakPtr<SSkillTreeWidget> WeakSelf = SharedThis(this);

	// Build grid rows
	TSharedRef<SVerticalBox> GridVBox = SNew(SVerticalBox);

	for (int32 Row = 0; Row <= MaxRow; ++Row)
	{
		TSharedRef<SHorizontalBox> RowHBox = SNew(SHorizontalBox);

		for (int32 Col = 0; Col <= MaxCol; ++Col)
		{
			const FSkillEntry* const* FoundPtr = SkillGrid.Contains(Row) ? SkillGrid[Row].Find(Col) : nullptr;
			const FSkillEntry* Skill = FoundPtr ? *FoundPtr : nullptr;

			if (!Skill)
			{
				// Empty cell placeholder
				RowHBox->AddSlot()
				.AutoWidth()
				.Padding(CELL_HGAP * 0.5f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(CELL_WIDTH)
					.HeightOverride(CELL_HEIGHT)
				];
				continue;
			}

			const int32 SkillId = Skill->SkillId;
			const bool bLearned = Skill->CurrentLevel > 0;
			const bool bMaxed = Skill->CurrentLevel >= Skill->MaxLevel;
			const bool bCanLearn = Skill->bCanLearn;

			// Slot background color
			FLinearColor SlotBg;
			if (bMaxed)
				SlotBg = SKColors::SkillMaxed;
			else if (bLearned)
				SlotBg = SKColors::SkillLearned;
			else if (bCanLearn)
				SlotBg = (Skill->Type == TEXT("passive")) ? SKColors::SkillPassive : SKColors::SkillActive;
			else
				SlotBg = SKColors::SkillLocked;

			// Icon brush
			FSlateBrush* IconBrush = Sub->GetOrCreateIconBrush(Skill->IconPath);

			// Draggable skill (learned active/toggle only)
			const bool bDraggableSkill = bLearned && Skill->Type != TEXT("passive") && IconBrush != nullptr;
			const FString CapIconPath = Skill->IconPath;
			const FString CapDisplayName = Skill->DisplayName;

			// Build compact cell content
			TSharedRef<SVerticalBox> CellContent = SNew(SVerticalBox);

			// Row 1: Icon (centered, 32x32)
			if (IconBrush)
			{
				TSharedRef<SWidget> IconWidget = bDraggableSkill
					? TSharedRef<SWidget>(
						SNew(SBox)
						.WidthOverride(32.f)
						.HeightOverride(32.f)
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
						.WidthOverride(32.f)
						.HeightOverride(32.f)
						[
							SNew(SImage).Image(IconBrush)
						]
					);

				CellContent->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 1.f, 0.f, 1.f)
				[
					IconWidget
				];
			}

			// Row 2: Skill name (truncated)
			CellContent->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Skill->DisplayName))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 6))
				.ColorAndOpacity(FSlateColor(bLearned ? SKColors::TextBright : (bCanLearn ? SKColors::TextPrimary : SKColors::TextDim)))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
				.WrapTextAt(CELL_WIDTH - 8.f)
			];

			// Row 3: Level text + use-level selector arrows (RO Classic)
			CellContent->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 1.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				// Down arrow (decrease use level) — only for active/toggle skills, not passives
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0.f))
					.Visibility(bLearned && Skill->CurrentLevel > 1 && Skill->Type != TEXT("passive") ? EVisibility::Visible : EVisibility::Hidden)
					.OnClicked_Lambda([WeakSub, SkillId]() -> FReply {
						if (USkillTreeSubsystem* S = WeakSub.Get()) {
							int32 Cur = S->GetSelectedLevel(SkillId);
							if (Cur > 1) S->SetSelectedLevel(SkillId, Cur - 1);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("<")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(SKColors::TextGreen))
					]
				]
				// Level display: "useLevel / maxLearned"
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([WeakSub, SkillId, Skill]() -> FText {
						if (USkillTreeSubsystem* S = WeakSub.Get()) {
							if (Skill->CurrentLevel > 0) {
								int32 UseLv = S->GetSelectedLevel(SkillId);
								return FText::FromString(FString::Printf(TEXT("%d/%d"), UseLv, Skill->CurrentLevel));
							}
						}
						return FText::FromString(FString::Printf(TEXT("%d/%d"), Skill->CurrentLevel, Skill->MaxLevel));
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 6))
					.ColorAndOpacity(FSlateColor(bMaxed ? SKColors::GoldHighlight : (bLearned ? SKColors::TextGreen : SKColors::TextDim)))
				]
				// Up arrow (increase use level) — only for active/toggle skills, not passives
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0.f))
					.Visibility(bLearned && Skill->CurrentLevel > 1 && Skill->Type != TEXT("passive") ? EVisibility::Visible : EVisibility::Hidden)
					.OnClicked_Lambda([WeakSub, SkillId, Skill]() -> FReply {
						if (USkillTreeSubsystem* S = WeakSub.Get()) {
							int32 Cur = S->GetSelectedLevel(SkillId);
							if (Cur < Skill->CurrentLevel) S->SetSelectedLevel(SkillId, Cur + 1);
						}
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT(">")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
						.ColorAndOpacity(FSlateColor(SKColors::TextGreen))
					]
				]
			];

			// Row 4: Learn button [+] (only if can learn and not maxed)
			if (bCanLearn && !bMaxed)
			{
				CellContent->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(0.f))
					.OnClicked_Lambda([WeakSub, SkillId]() -> FReply {
						if (USkillTreeSubsystem* S = WeakSub.Get()) S->LearnSkill(SkillId);
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(SKColors::ButtonLearn)
						.Padding(FMargin(6.f, 1.f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("+")))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.ColorAndOpacity(FSlateColor(SKColors::TextBright))
						]
					]
				];
			}

			// Build the cell widget with tooltip
			TSharedRef<SWidget> CellWidget =
				SNew(SBox)
				.WidthOverride(CELL_WIDTH)
				.HeightOverride(CELL_HEIGHT)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(SKColors::GoldDark)
					.Padding(FMargin(1.f))
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
						.BorderBackgroundColor(SlotBg)
						.Padding(FMargin(2.f, 1.f))
						[
							CellContent
						]
					]
				];

			// Attach tooltip
			CellWidget->SetToolTip(
				SNew(SToolTip)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBrush"))
				[
					SNew(SSkillTooltipWidget)
					.SkillData(Skill)
					.Subsystem(Sub)
				]
			);

			RowHBox->AddSlot()
			.AutoWidth()
			.Padding(CELL_HGAP * 0.5f, 0.f)
			[
				CellWidget
			];
		}

		GridVBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, CELL_VGAP * 0.5f)
		[
			RowHBox
		];
	}

	// Set the grid content into the panel
	if (GridPanel->ContentBox.IsValid())
	{
		GridPanel->ContentBox->ClearChildren();
		GridPanel->ContentBox->AddSlot()
		.AutoHeight()
		.Padding(GRID_PADDING)
		[
			GridVBox
		];
	}

	// Add grid panel to scroll box
	SkillScrollBox->AddSlot()
	[
		GridPanel.ToSharedRef()
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
