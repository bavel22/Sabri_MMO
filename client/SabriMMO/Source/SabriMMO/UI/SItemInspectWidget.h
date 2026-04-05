#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CharacterData.h"

class UItemInspectSubsystem;
class SBox;
class SVerticalBox;
class SScrollBox;

/**
 * Item inspect popup window — RO Classic style.
 * Shows: title bar (icon + name + [X]), large icon left, formatted description right, card slot footer.
 * Draggable by title bar. One instance, updated via UpdateItem().
 */
class SItemInspectWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SItemInspectWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UItemInspectSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Update displayed item without rebuilding the widget shell */
	void UpdateItem(const FInventoryItem& Item);

private:
	TWeakObjectPtr<UItemInspectSubsystem> OwningSubsystem;

	// Stored item data
	FInventoryItem CurrentItem;

	// Dynamic content slots (rebuilt on UpdateItem)
	TSharedPtr<SVerticalBox> ContentArea;
	TSharedPtr<SVerticalBox> CardSlotArea;
	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<SBox> IconBox;

	// Build methods
	void RebuildContent();
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> FormatFullDescription(const FString& FullDesc);
	TSharedRef<SWidget> BuildLabelValueRow(const FString& Label, const FString& Value);
	TSharedRef<SWidget> BuildPlainTextLine(const FString& Text);
	TSharedRef<SWidget> BuildGoldDivider();
	TSharedRef<SWidget> BuildCardSlotFooter();
	TSharedRef<SWidget> BuildEmptySlot();
	TSharedRef<SWidget> BuildFilledSlot(const FCompoundedCardInfo& Card);
	TSharedRef<SWidget> BuildIconArea();

	// Drag state
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(0.0, 0.0);

	void ApplyLayout();

	static constexpr float TitleBarHeight = 24.f;
	static constexpr float WidgetWidth = 380.f;
	static constexpr float IconSize = 128.f;

	// Mouse interaction
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
};
