// SAdvancedStatsWidget.h — RO-style advanced stats panel showing elemental
// resistances. Draggable Slate widget, reads data from CombatStatsSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SBasicInfoWidget.h" // EResizeEdge

class UCombatStatsSubsystem;
class SBox;

class SAdvancedStatsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAdvancedStatsWidget) {}
		SLATE_ARGUMENT(UCombatStatsSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	UCombatStatsSubsystem* Subsystem = nullptr;

	// --- drag state ---
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(240.0, 200.0);

	TSharedPtr<SBox> RootSizeBox;

	void ApplyLayout();
	FVector2D GetContentSize() const;

	// --- helper builders ---
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildSectionHeader(const FString& Title);
	TSharedRef<SWidget> BuildResistRow(const FString& ElementName, const FLinearColor& ElementColor, TAttribute<FText> Value);
	TSharedRef<SWidget> BuildDualRow(const FString& Label, const FLinearColor& LabelColor, TAttribute<FText> AtkValue, TAttribute<FText> DefValue);
	TSharedRef<SWidget> BuildDivider();

	// --- close button ---
	FReply OnCloseClicked();
};
