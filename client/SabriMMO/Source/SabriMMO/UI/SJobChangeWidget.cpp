// SJobChangeWidget.cpp — Three-page modal for class change.

#include "SJobChangeWidget.h"
#include "JobChangeSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

namespace JobChangeColors
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
	static const FLinearColor ButtonBrown   (0.35f, 0.24f, 0.14f, 1.f);
	static const FLinearColor ButtonHover   (0.50f, 0.35f, 0.18f, 1.f);
	static const FLinearColor ErrorRed      (0.95f, 0.25f, 0.20f, 1.f);
	static const FLinearColor SuccessGreen  (0.30f, 0.85f, 0.30f, 1.f);
}

static constexpr float JobChangeMaxListHeight = 200.f;

void SJobChangeWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		// Fixed-size popup; centered by the subsystem's AlignmentWrapper (HAlign_Center/VAlign_Center).
		// Drag offset is applied via SetRenderTransform in ApplyLayout().
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(WidgetWidth)
		.HeightOverride(WidgetHeight)
		[
			// 3-layer RO frame: Gold -> Dark -> Brown
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(JobChangeColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(JobChangeColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(JobChangeColors::PanelBrown)
					.Padding(FMargin(8.f))
					[
						SNew(SVerticalBox)

						// Title bar (drag handle — top TitleBarHeight px)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						// Gold divider
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
						[
							SNew(SBox).HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(JobChangeColors::GoldDivider)
							]
						]

						// Page content (fills remaining height — keeps dialog the same size across pages)
						+ SVerticalBox::Slot().FillHeight(1.f)
						[ BuildPageContent() ]
					]
				]
			]
		]
	];

	ApplyLayout();
}

// ── Title Bar ────────────────────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildTitleBar()
{
	return SNew(STextBlock)
		.Text(FText::FromString(TEXT("Job Master")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		.ColorAndOpacity(FSlateColor(JobChangeColors::GoldHighlight))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(JobChangeColors::TextShadow)
		.Justification(ETextJustify::Center);
}

// ── Page dispatcher ──────────────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildPageContent()
{
	// SWidgetSwitcher with a dynamic active index — page swaps when CurrentPage changes.
	// All three pages are constructed once; the switcher shows exactly one at a time.
	return SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]() -> int32
		{
			UJobChangeSubsystem* S = OwningSubsystem.Get();
			if (!S) return 0;
			switch (S->CurrentPage)
			{
				case EJobChangePage::Greeting:  return 0;
				case EJobChangePage::Selection: return 1;
				case EJobChangePage::Congrats:  return 2;
				default:                        return 0;
			}
		})

		+ SWidgetSwitcher::Slot() [ BuildGreetingPage() ]
		+ SWidgetSwitcher::Slot() [ BuildSelectionPage() ]
		+ SWidgetSwitcher::Slot() [ BuildCongratsPage() ];
}

// ── Greeting page ────────────────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildGreetingPage()
{
	UJobChangeSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SVerticalBox)

		// Greeting text
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 8)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText
			{
				UJobChangeSubsystem* S = OwningSubsystem.Get();
				if (!S) return FText::GetEmpty();
				const FString CurrentClass = S->CurrentClassDisplayName.IsEmpty()
					? TEXT("adventurer") : S->CurrentClassDisplayName;
				return FText::FromString(FString::Printf(
					TEXT("Greetings, %s.\nI am the Job Master.\nWhat would you like to do?"),
					*CurrentClass));
			})
			.AutoWrapText(true)
			.Justification(ETextJustify::Center)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
			.ColorAndOpacity(FSlateColor(JobChangeColors::TextPrimary))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(JobChangeColors::TextShadow)
		]

		// Buttons row
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 2, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Change Job")))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() -> FReply
				{
					if (UJobChangeSubsystem* S = OwningSubsystem.Get())
					{
						S->GoToSelectionPage();
					}
					return FReply::Handled();
				})
			]

			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 2, 0)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Cancel")))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([this]() -> FReply
				{
					if (UJobChangeSubsystem* S = OwningSubsystem.Get())
					{
						S->CloseDialog();
					}
					return FReply::Handled();
				})
			]
		];
}

// ── Selection page ───────────────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildSelectionPage()
{
	UJobChangeSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return SNew(SBox);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	// Header text — varies by eligibility
	Box->AddSlot().AutoHeight().Padding(0, 4, 0, 8)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() -> FText
		{
			UJobChangeSubsystem* S = OwningSubsystem.Get();
			if (!S) return FText::GetEmpty();
			if (S->Eligibility != EJobEligibility::Ready)
			{
				return FText::FromString(S->RequirementMessage);
			}
			return FText::FromString(TEXT("Select your new path:"));
		})
		.AutoWrapText(true)
		.Justification(ETextJustify::Center)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.ColorAndOpacity(FSlateColor(JobChangeColors::TextPrimary))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(JobChangeColors::TextShadow)
	];

	// Class buttons (only when Ready)
	if (Sub->Eligibility == EJobEligibility::Ready)
	{
		TSharedRef<SVerticalBox> ClassList = SNew(SVerticalBox);
		for (const FJobChangeTarget& Target : Sub->EligibleTargets)
		{
			ClassList->AddSlot().AutoHeight().Padding(0, 2, 0, 2)
			[
				BuildClassButton(Target.ClassId, Target.DisplayName)
			];
		}

		Box->AddSlot().AutoHeight()
		[
			SNew(SBox)
			.MaxDesiredHeight(JobChangeMaxListHeight)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[ ClassList ]
			]
		];
	}

	// Server error text (red, only visible when set)
	Box->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() -> FText
		{
			UJobChangeSubsystem* S = OwningSubsystem.Get();
			if (!S) return FText::GetEmpty();
			return FText::FromString(S->ServerErrorMessage);
		})
		.Visibility_Lambda([this]() -> EVisibility
		{
			UJobChangeSubsystem* S = OwningSubsystem.Get();
			return (S && !S->ServerErrorMessage.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.AutoWrapText(true)
		.Justification(ETextJustify::Center)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		.ColorAndOpacity(FSlateColor(JobChangeColors::ErrorRed))
		.ShadowOffset(FVector2D(1, 1))
		.ShadowColorAndOpacity(JobChangeColors::TextShadow)
	];

	// Back / Close row
	Box->AddSlot().AutoHeight().Padding(0, 10, 0, 0)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 2, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Back")))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([this]() -> FReply
			{
				if (UJobChangeSubsystem* S = OwningSubsystem.Get())
				{
					S->GoToGreetingPage();
				}
				return FReply::Handled();
			})
		]

		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2, 0, 2, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Close")))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([this]() -> FReply
			{
				if (UJobChangeSubsystem* S = OwningSubsystem.Get())
				{
					S->CloseDialog();
				}
				return FReply::Handled();
			})
		]
	];

	return Box;
}

// ── One class-target button ──────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildClassButton(const FString& ClassId, const FString& DisplayName)
{
	return SNew(SButton)
		.Text(FText::FromString(DisplayName))
		.HAlign(HAlign_Center)
		.IsEnabled_Lambda([this]() -> bool
		{
			UJobChangeSubsystem* S = OwningSubsystem.Get();
			return S ? !S->bRequestInflight : false;
		})
		.OnClicked_Lambda([this, ClassId]() -> FReply
		{
			if (UJobChangeSubsystem* S = OwningSubsystem.Get())
			{
				S->RequestChangeJob(ClassId);
			}
			return FReply::Handled();
		});
}

// ── Congrats page ────────────────────────────────────────────────

TSharedRef<SWidget> SJobChangeWidget::BuildCongratsPage()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 12)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText
			{
				UJobChangeSubsystem* S = OwningSubsystem.Get();
				if (!S) return FText::GetEmpty();
				return FText::FromString(FString::Printf(
					TEXT("Congratulations on becoming a %s!"),
					*S->NewClassDisplayName));
			})
			.AutoWrapText(true)
			.Justification(ETextJustify::Center)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(FSlateColor(JobChangeColors::SuccessGreen))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(JobChangeColors::TextShadow)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Close")))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([this]() -> FReply
			{
				if (UJobChangeSubsystem* S = OwningSubsystem.Get())
				{
					S->CloseDialog();
				}
				return FReply::Handled();
			})
		];
}

// ── Keyboard ─────────────────────────────────────────────────────

FReply SJobChangeWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		if (UJobChangeSubsystem* S = OwningSubsystem.Get())
		{
			S->CloseDialog();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ── Drag (title bar only, DPI-correct per sabrimmo-ui Phase 7) ───

void SJobChangeWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}

FReply SJobChangeWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	const FVector2D ScreenPos = MouseEvent.GetScreenSpacePosition();
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenPos);

	// Title bar = drag handle. Outer border padding (2+1+8 = ~11) + ~17px text height = ~28px.
	if (LocalPos.Y < TitleBarHeight)
	{
		bIsDragging = true;
		DragOffset = ScreenPos;
		DragStartWidgetPos = WidgetPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SJobChangeWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SJobChangeWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
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

FCursorReply SJobChangeWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());
	if (LocalPos.Y >= 0.f && LocalPos.Y < TitleBarHeight &&
		LocalPos.X >= 0.f && LocalPos.X < WidgetWidth)
	{
		return FCursorReply::Cursor(bIsDragging ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
	}
	return FCursorReply::Unhandled();
}
