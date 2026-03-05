// SKafraWidget.h — Slate widget for Kafra NPC service dialog.
// RO Classic themed: save point, teleport service with zeny costs.
// Two views: main menu and teleport destination list.
// Draggable via title bar.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UKafraSubsystem;
class SVerticalBox;

class SKafraWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SKafraWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UKafraSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Called by subsystem when kafra:data arrives to rebuild destination list. */
	void RebuildDestinations();

private:
	TWeakObjectPtr<UKafraSubsystem> OwningSubsystem;

	// View state
	enum class EKafraView : uint8 { MainMenu, TeleportList };
	EKafraView CurrentView = EKafraView::MainMenu;

	// Widget references for view switching
	TSharedPtr<SWidget> MainMenuPanel;
	TSharedPtr<SWidget> TeleportPanel;
	TSharedPtr<SVerticalBox> DestContainer;

	// Build methods
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildMainMenuContent();
	TSharedRef<SWidget> BuildTeleportContent();
	TSharedRef<SWidget> BuildBottomRow();
	TSharedRef<SWidget> BuildKafraButton(const FText& Label, FOnClicked OnClicked);

	// Drag state
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(300.0, 200.0);

	void ApplyLayout();

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
};
