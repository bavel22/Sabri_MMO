// SSenseResultPopup.h — Draggable panel displaying monster information from Wizard Sense skill.
// Non-blocking: player can move, attack, cast while this is open. Closes only via X button.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UEnemySubsystem;
class SBox;

/**
 * Data passed from the sense_result socket event to the popup.
 */
struct FSenseResultData
{
	int32 TargetId = 0;
	FString TargetName;
	int32 Level = 0;
	double Health = 0;
	double MaxHealth = 0;
	FString Element;
	int32 ElementLevel = 1;
	FString Race;
	FString Size;
	int32 HardDef = 0;
	int32 HardMdef = 0;
	int32 STR = 0;
	int32 AGI = 0;
	int32 VIT = 0;
	int32 INT = 0;
	int32 DEX = 0;
	int32 LUK = 0;
	int32 BaseExp = 0;
	int32 JobExp = 0;
};

class SSenseResultPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSenseResultPopup)
		: _Subsystem(nullptr)
	{}
		SLATE_ARGUMENT(UEnemySubsystem*, Subsystem)
		SLATE_ARGUMENT(FSenseResultData, SenseData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TWeakObjectPtr<UEnemySubsystem> OwningSubsystem;
	FSenseResultData Data;
	TSharedPtr<SBox> RootSizeBox;

	// Build helpers
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildStatRow(const FString& Label, const FString& Value);
	TSharedRef<SWidget> BuildStatRow(const FString& Label, int32 Value);
	TSharedRef<SWidget> BuildGoldDivider();
	TSharedRef<SWidget> BuildHPBar();

	void DismissPopup();

	// Drag state
	bool bIsDragging = false;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FVector2D DragStartWidgetPos = FVector2D::ZeroVector;
	FVector2D WidgetPosition = FVector2D(400.0, 200.0);
	static constexpr float TitleBarHeight = 20.f;

	FVector2D GetContentSize() const;
	void ApplyLayout();

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
};
