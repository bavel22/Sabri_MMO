// SCraftingPopup.h — Shared crafting popup for Arrow Crafting + Pharmacy.
// Fullscreen backdrop + centered scrollable recipe list with RO Classic theme.
// Pattern adapted from SCardCompoundPopup.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "CraftingSubsystem.h"

class STextBlock;

class SCraftingPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCraftingPopup)
		: _Subsystem(nullptr)
		, _ShowSuccessRate(false)
	{}
		SLATE_ARGUMENT(UCraftingSubsystem*, Subsystem)
		SLATE_ARGUMENT(FString, Title)
		SLATE_ARGUMENT(TArray<FCraftingRecipe>, Recipes)
		SLATE_ARGUMENT(bool, ShowSuccessRate)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStatusMessage(const FString& Message, bool bIsError);

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UCraftingSubsystem> OwningSubsystem;
	FString PopupTitle;
	TArray<FCraftingRecipe> RecipeList;
	bool bShowRate = false;

	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildRecipeList();
	TSharedRef<SWidget> BuildRecipeRow(const FCraftingRecipe& Recipe);

	TSharedPtr<STextBlock> StatusTextBlock;

	void OnRecipeClicked(const FCraftingRecipe& Recipe);
	void DismissPopup();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
};
