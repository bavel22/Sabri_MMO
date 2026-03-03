// SHotbarKeybindWidget.cpp — Keybind configuration panel for hotbar.

#include "SHotbarKeybindWidget.h"
#include "HotbarSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SNullWidget.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"

namespace KBColors
{
	static const FLinearColor PanelBrown    (0.43f, 0.29f, 0.17f, 1.f);
	static const FLinearColor PanelDark     (0.22f, 0.14f, 0.08f, 1.f);
	static const FLinearColor GoldTrim      (0.72f, 0.58f, 0.28f, 1.f);
	static const FLinearColor GoldHighlight (0.92f, 0.80f, 0.45f, 1.f);
	static const FLinearColor GoldDivider   (0.60f, 0.48f, 0.22f, 1.f);
	static const FLinearColor TextPrimary   (0.96f, 0.90f, 0.78f, 1.f);
	static const FLinearColor TextBright    (1.00f, 1.00f, 1.00f, 1.f);
	static const FLinearColor TextShadow    (0.00f, 0.00f, 0.00f, 0.85f);
	static const FLinearColor CellBg        (0.15f, 0.10f, 0.06f, 1.f);
	static const FLinearColor CellBorder    (0.40f, 0.30f, 0.15f, 1.f);
	static const FLinearColor CellListening (0.90f, 0.80f, 0.20f, 0.4f);
	static const FLinearColor ButtonBg      (0.35f, 0.25f, 0.12f, 1.f);
	static const FLinearColor ConflictRed   (0.90f, 0.20f, 0.20f, 1.f);
}

// ============================================================
// Construct
// ============================================================

void SHotbarKeybindWidget::Construct(const FArguments& InArgs)
{
	OwningSubsystem = InArgs._Subsystem;

	ChildSlot
	[
		SAssignNew(RootSizeBox, SBox)
		.WidthOverride(PanelWidth)
		.HeightOverride(PanelHeight)
		[
			// 3-layer frame
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(KBColors::GoldTrim)
			.Padding(FMargin(2.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(KBColors::PanelDark)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(KBColors::PanelBrown)
					.Padding(FMargin(4.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[ BuildTitleBar() ]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
						[
							// Gold divider
							SNew(SBox).HeightOverride(1.f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
								.BorderBackgroundColor(KBColors::GoldDivider)
							]
						]

						+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 4.f)
						[ BuildKeybindGrid() ]

						// Conflict message
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() -> FText
							{
								return FText::FromString(ConflictMessage);
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FSlateColor(KBColors::ConflictRed))
						]

						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
						[ BuildButtonRow() ]
					]
				]
			]
		]
	];

	// Focus this widget so it receives key events
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

// ============================================================
// Title bar
// ============================================================

TSharedRef<SWidget> SHotbarKeybindWidget::BuildTitleBar()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Hotbar Keybinds")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FSlateColor(KBColors::GoldHighlight))
			.ShadowOffset(FVector2D(1, 1))
			.ShadowColorAndOpacity(KBColors::TextShadow)
		]

		// Close button (X)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(KBColors::ButtonBg)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Cursor(EMouseCursor::Hand)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("X")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(KBColors::TextPrimary))
				]
			]
		];
}

// ============================================================
// Keybind grid (4 rows x 9 columns)
// ============================================================

TSharedRef<SWidget> SHotbarKeybindWidget::BuildKeybindGrid()
{
	TSharedRef<SVerticalBox> Grid = SNew(SVerticalBox);

	for (int32 Row = 0; Row < UHotbarSubsystem::NUM_ROWS; ++Row)
	{
		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

		// Row label
		RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBox).WidthOverride(40.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Row %d"), Row + 1)))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity(FSlateColor(KBColors::TextPrimary))
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(KBColors::TextShadow)
				]
			];

		// 9 cells
		for (int32 Slot = 0; Slot < UHotbarSubsystem::SLOTS_PER_ROW; ++Slot)
		{
			RowBox->AddSlot()
				.AutoWidth()
				.Padding(1.f)
				[
					BuildKeybindCell(Row, Slot)
				];
		}

		Grid->AddSlot()
			.AutoHeight()
			.Padding(0.f, 1.f)
			[
				RowBox
			];
	}

	return Grid;
}

TSharedRef<SWidget> SHotbarKeybindWidget::BuildKeybindCell(int32 RowIndex, int32 SlotIndex)
{
	UHotbarSubsystem* Sub = OwningSubsystem.Get();

	return SNew(SBox)
		.WidthOverride(CellWidth)
		.HeightOverride(CellHeight)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(KBColors::CellBorder)
			.Padding(FMargin(1.f))
			[
				SNew(SOverlay)

				// Background
				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor_Lambda([this, RowIndex, SlotIndex]() -> FSlateColor
					{
						if (bIsListening && ListeningRow == RowIndex && ListeningSlot == SlotIndex)
							return FSlateColor(KBColors::CellListening);
						return FSlateColor(KBColors::CellBg);
					})
				]

				// Keybind text or "..." if listening
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, RowIndex, SlotIndex, Sub]() -> FText
					{
						if (bIsListening && ListeningRow == RowIndex && ListeningSlot == SlotIndex)
							return FText::FromString(TEXT("..."));
						if (!Sub) return FText::GetEmpty();
						FString Display = Sub->GetKeybindDisplayString(RowIndex, SlotIndex);
						if (Display.IsEmpty()) return FText::FromString(TEXT("-"));
						return FText::FromString(Display);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.ColorAndOpacity_Lambda([this, RowIndex, SlotIndex]() -> FSlateColor
					{
						if (bIsListening && ListeningRow == RowIndex && ListeningSlot == SlotIndex)
							return FSlateColor(KBColors::GoldHighlight);
						return FSlateColor(KBColors::TextBright);
					})
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(KBColors::TextShadow)
				]
			]
		];
}

// ============================================================
// Button row (Reset Defaults, Close)
// ============================================================

TSharedRef<SWidget> SHotbarKeybindWidget::BuildButtonRow()
{
	auto MakeButton = [this](const FString& Label) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(24.f)
			.Padding(FMargin(8.f, 0.f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
				.BorderBackgroundColor(KBColors::GoldTrim)
				.Padding(FMargin(1.f))
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
					.BorderBackgroundColor(KBColors::ButtonBg)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Padding(FMargin(8.f, 2.f))
					.Cursor(EMouseCursor::Hand)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Label))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
						.ColorAndOpacity(FSlateColor(KBColors::TextPrimary))
						.ShadowOffset(FVector2D(1, 1))
						.ShadowColorAndOpacity(KBColors::TextShadow)
					]
				]
			];
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f) // spacer
		[
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			MakeButton(TEXT("Reset Defaults"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8.f, 0.f, 0.f, 0.f)
		[
			MakeButton(TEXT("Close"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f) // spacer
		[
			SNullWidget::NullWidget
		];
}

// ============================================================
// Listening mode (click-to-bind)
// ============================================================

void SHotbarKeybindWidget::StartListening(int32 Row, int32 Slot)
{
	bIsListening = true;
	ListeningRow = Row;
	ListeningSlot = Slot;
	ConflictMessage.Empty();
}

void SHotbarKeybindWidget::StopListening()
{
	bIsListening = false;
	ListeningRow = -1;
	ListeningSlot = -1;
}

FString SHotbarKeybindWidget::CheckConflict(int32 Row, int32 Slot, FKey Key, bool bAlt, bool bCtrl, bool bShift) const
{
	UHotbarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub) return TEXT("");

	for (int32 r = 0; r < UHotbarSubsystem::NUM_ROWS; ++r)
	{
		for (int32 s = 0; s < UHotbarSubsystem::SLOTS_PER_ROW; ++s)
		{
			if (r == Row && s == Slot) continue;
			FHotbarKeybind Existing = Sub->GetKeybind(r, s);
			if (Existing.PrimaryKey == Key &&
				Existing.bRequiresAlt == bAlt &&
				Existing.bRequiresCtrl == bCtrl &&
				Existing.bRequiresShift == bShift)
			{
				return FString::Printf(TEXT("Conflict: Row %d Slot %d already uses this key"), r + 1, s + 1);
			}
		}
	}
	return TEXT("");
}

void SHotbarKeybindWidget::ApplyKeybind(FKey NewKey, bool bAlt, bool bCtrl, bool bShift)
{
	UHotbarSubsystem* Sub = OwningSubsystem.Get();
	if (!Sub || !bIsListening) return;

	// Check for conflicts
	FString Conflict = CheckConflict(ListeningRow, ListeningSlot, NewKey, bAlt, bCtrl, bShift);
	if (!Conflict.IsEmpty())
	{
		// Clear the conflicting keybind first
		for (int32 r = 0; r < UHotbarSubsystem::NUM_ROWS; ++r)
		{
			for (int32 s = 0; s < UHotbarSubsystem::SLOTS_PER_ROW; ++s)
			{
				if (r == ListeningRow && s == ListeningSlot) continue;
				FHotbarKeybind Existing = Sub->GetKeybind(r, s);
				if (Existing.PrimaryKey == NewKey &&
					Existing.bRequiresAlt == bAlt &&
					Existing.bRequiresCtrl == bCtrl &&
					Existing.bRequiresShift == bShift)
				{
					FHotbarKeybind Empty;
					Sub->SetKeybind(r, s, Empty);
				}
			}
		}
		ConflictMessage = Conflict + TEXT(" (cleared)");
	}

	FHotbarKeybind KB;
	KB.PrimaryKey = NewKey;
	KB.bRequiresAlt = bAlt;
	KB.bRequiresCtrl = bCtrl;
	KB.bRequiresShift = bShift;

	Sub->SetKeybind(ListeningRow, ListeningSlot, KB);
	Sub->SaveKeybinds();
	StopListening();
}

// ============================================================
// Key input (for listening mode)
// ============================================================

FReply SHotbarKeybindWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (bIsListening)
	{
		FKey Key = InKeyEvent.GetKey();

		// ESC cancels listening
		if (Key == EKeys::Escape)
		{
			StopListening();
			return FReply::Handled();
		}

		// Delete/Backspace clears the keybind
		if (Key == EKeys::Delete || Key == EKeys::BackSpace)
		{
			UHotbarSubsystem* Sub = OwningSubsystem.Get();
			if (Sub)
			{
				FHotbarKeybind Empty;
				Sub->SetKeybind(ListeningRow, ListeningSlot, Empty);
				Sub->SaveKeybinds();
			}
			StopListening();
			return FReply::Handled();
		}

		// Skip modifier-only keys
		if (Key == EKeys::LeftShift || Key == EKeys::RightShift ||
			Key == EKeys::LeftAlt || Key == EKeys::RightAlt ||
			Key == EKeys::LeftControl || Key == EKeys::RightControl)
		{
			return FReply::Handled();
		}

		// Apply the keybind
		ApplyKeybind(Key,
			InKeyEvent.IsAltDown(),
			InKeyEvent.IsControlDown(),
			InKeyEvent.IsShiftDown());
		return FReply::Handled();
	}

	// ESC closes the panel
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		UHotbarSubsystem* Sub = OwningSubsystem.Get();
		if (Sub) Sub->HideKeybindWidget();
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

// ============================================================
// Mouse handlers
// ============================================================

FReply SHotbarKeybindWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D ContentBounds = GetContentSize();

	// Outside content bounds — pass through
	if (LocalPos.X < 0 || LocalPos.X > ContentBounds.X ||
		LocalPos.Y < 0 || LocalPos.Y > ContentBounds.Y)
	{
		return FReply::Unhandled();
	}

	UHotbarSubsystem* Sub = OwningSubsystem.Get();

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Check if clicking a keybind cell
		// Grid starts after title bar (~24px) + divider (~5px) + padding
		float GridStartY = 24.f + 5.f + 4.f + 2.f + 1.f + 2.f + 4.f;
		float GridStartX = 4.f + 2.f + 1.f + 2.f + 4.f + 40.f + 4.f; // padding + borders + row label

		for (int32 Row = 0; Row < UHotbarSubsystem::NUM_ROWS; ++Row)
		{
			for (int32 Slot = 0; Slot < UHotbarSubsystem::SLOTS_PER_ROW; ++Slot)
			{
				float CellX = GridStartX + (Slot * (CellWidth + 2.f));
				float CellY = GridStartY + (Row * (CellHeight + 4.f));

				if (LocalPos.X >= CellX && LocalPos.X < CellX + CellWidth &&
					LocalPos.Y >= CellY && LocalPos.Y < CellY + CellHeight)
				{
					StartListening(Row, Slot);
					return FReply::Handled();
				}
			}
		}

		// Check Close button (X) — top-right area
		if (LocalPos.X > ContentBounds.X - 30.f && LocalPos.Y < 24.f && Sub)
		{
			Sub->HideKeybindWidget();
			return FReply::Handled();
		}

		// Check button area (bottom)
		float ButtonY = ContentBounds.Y - 30.f;
		if (LocalPos.Y >= ButtonY)
		{
			float CenterX = ContentBounds.X * 0.5f;

			// "Reset Defaults" button — left of center
			if (LocalPos.X < CenterX && Sub)
			{
				Sub->ResetKeybindsToDefaults();
				ConflictMessage = TEXT("Keybinds reset to defaults");
				return FReply::Handled();
			}
			// "Close" button — right of center
			else if (Sub)
			{
				Sub->SaveKeybinds();
				Sub->HideKeybindWidget();
				return FReply::Handled();
			}
		}

		// Title bar drag
		if (LocalPos.Y < 24.f)
		{
			bIsDragging = true;
			DragOffset = MouseEvent.GetScreenSpacePosition();
			DragStartWidgetPos = WidgetPosition;
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SHotbarKeybindWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SHotbarKeybindWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		const FVector2D AbsDelta = MouseEvent.GetScreenSpacePosition() - DragOffset;
		const float DPIScale = (MyGeometry.GetLocalSize().X > 0.f) ? (MyGeometry.GetAbsoluteSize().X / MyGeometry.GetLocalSize().X) : 1.f;
		WidgetPosition = DragStartWidgetPos + AbsDelta / DPIScale;
		ApplyLayout();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ============================================================
// Layout helpers
// ============================================================

FVector2D SHotbarKeybindWidget::GetContentSize() const
{
	if (RootSizeBox.IsValid())
		return RootSizeBox->GetDesiredSize();
	return FVector2D(PanelWidth, PanelHeight);
}

void SHotbarKeybindWidget::ApplyLayout()
{
	const FVector2f Pos((float)WidgetPosition.X, (float)WidgetPosition.Y);
	SetRenderTransform(FSlateRenderTransform(Pos));
}
