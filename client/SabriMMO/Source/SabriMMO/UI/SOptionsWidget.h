#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

class UOptionsSubsystem;

/**
 * SFPSCounterWidget — smoothed FPS counter at top-center of screen.
 */
class SFPSCounterWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFPSCounterWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	int32 CachedFPS = 0;
	float SmoothedFPS = 60.f;
	EActiveTimerReturnType UpdateFPS(double InCurrentTime, float InDeltaTime);
};

/**
 * SOptionsWidget — Tabbed options panel: Game tab + Video tab.
 */
class SOptionsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOptionsWidget) : _Subsystem(nullptr) {}
		SLATE_ARGUMENT(UOptionsSubsystem*, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TWeakObjectPtr<UOptionsSubsystem> OwningSubsystem;

	// ---- Tab state ----
	int32 ActiveTabIndex = 0;

	// ---- Shared builders ----
	TSharedRef<SWidget> BuildTitleBar();
	TSharedRef<SWidget> BuildTabBar();
	TSharedRef<SWidget> BuildButton(const FText& Label, FOnClicked OnClicked);
	TSharedRef<SWidget> BuildSectionHeader(const FText& Title);
	TSharedRef<SWidget> BuildToggleRow(const FText& Label,
		TSharedRef<TFunction<bool()>> IsOnGetter, FOnClicked OnToggle);
	TSharedRef<SWidget> BuildSliderRow(const FText& Label,
		float MinVal, float MaxVal, int32 Decimals,
		TSharedRef<TFunction<float()>> Getter,
		TSharedRef<TFunction<void(float)>> Setter);
	TSharedRef<SWidget> BuildDropdownRow(const FText& Label,
		TArray<TSharedPtr<FString>>* OptionsSource,
		TSharedPtr<SComboBox<TSharedPtr<FString>>>* OutComboPtr,
		TSharedRef<TFunction<int32()>> CurrentIndexGetter,
		TSharedRef<TFunction<void(int32)>> OnSelected);

	// ---- Tab content builders ----
	TSharedRef<SWidget> BuildGameContent();
	TSharedRef<SWidget> BuildVideoContent();
	TSharedRef<SWidget> BuildResolutionConfirmOverlay();

	// ---- Video settings helpers ----
	void PopulateVideoOptions();
	void RefreshVideoDropdowns();
	FReply OnApplyVideoSettings();
	FReply OnAutoDetect();
	void ShowResolutionConfirmation();
	FReply OnConfirmResolution();
	FReply OnRevertResolution();
	EActiveTimerReturnType TickConfirmCountdown(double InCurrentTime, float InDeltaTime);
	int32 FindOptionIndex(const TArray<TSharedPtr<FString>>& Options, const FString& Value) const;

	// ---- Dropdown option arrays (must outlive SComboBox) ----
	TArray<TSharedPtr<FString>> WindowModeOptions;
	TArray<TSharedPtr<FString>> ResolutionOptions;
	TArray<TSharedPtr<FString>> FrameRateLimitOptions;
	TArray<TSharedPtr<FString>> QualityLevelOptions;
	TArray<TSharedPtr<FString>> OverallQualityOptions;
	TArray<FIntPoint> ResolutionValues;
	TArray<float> FrameRateLimitValues;

	// ---- Combo box refs (for refresh after auto-detect) ----
	TSharedPtr<SComboBox<TSharedPtr<FString>>> WindowModeCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ResolutionCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FrameRateCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> OverallQualityCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ShadowCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> TextureCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> AACombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> EffectsCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ViewDistCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PostProcCombo;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FoliageCombo;

	// ---- Resolution confirmation ----
	TSharedPtr<SWidget> ConfirmOverlay;
	TSharedPtr<STextBlock> ConfirmCountdownText;
	FIntPoint PrevResolution;
	int32 PrevWindowMode = 0;
	float ConfirmTimeRemaining = 15.f;
	bool bWaitingForConfirm = false;
};
