// JobChangeSubsystem.h — UWorldSubsystem managing the Job Master class change dialog.
// Registers Socket.io event handlers via the persistent EventRouter.
// On job:changed: respawns local player sprite, refreshes skill tree, shows congrats page.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "JobChangeSubsystem.generated.h"

class SJobChangeWidget;

/** Eligibility result for a class change attempt. */
UENUM()
enum class EJobEligibility : uint8
{
	Ready,                  // Player meets all requirements — show class buttons
	NoviceJobLevelTooLow,   // Novice but jobLevel < 10
	NoviceBasicSkillTooLow, // Novice job 10 but Basic Skill (id 1) < 9
	FirstClassJobLevelTooLow, // First class but jobLevel < 40
	AlreadySecondClass,     // Tier 2 — nothing more to learn
	Transcendent,           // Trans tier — speak with Valkyrie (placeholder)
};

/** A class option offered in the Selection page. */
USTRUCT()
struct FJobChangeTarget
{
	GENERATED_BODY()

	FString ClassId;     // canonical lowercase, e.g. "knight"
	FString DisplayName; // user-facing, e.g. "Knight"
};

/** Which page of the dialog is currently shown. */
UENUM()
enum class EJobChangePage : uint8
{
	Greeting,
	Selection,
	Congrats,
};

UCLASS()
class SABRIMMO_API UJobChangeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ---- dialog state (read by widget via lambdas) ----
	EJobChangePage CurrentPage = EJobChangePage::Greeting;
	FString CurrentClassDisplayName;
	int32 CurrentJobLevel = 1;
	int32 BasicSkillLevel = 0;
	EJobEligibility Eligibility = EJobEligibility::Ready;
	TArray<FJobChangeTarget> EligibleTargets;
	FString RequirementMessage;     // Shown on Selection page when Eligibility != Ready
	FString ServerErrorMessage;     // Last job:error message (red, on Selection)
	FString NewClassDisplayName;    // Set from job:changed payload, shown on Congrats page
	bool bRequestInflight = false;  // Disables target buttons while waiting for server reply

	// ---- public API ----
	void OpenDialog();
	void CloseDialog();
	void RequestChangeJob(const FString& TargetClass);
	void GoToGreetingPage();
	void GoToSelectionPage();

	// ---- widget lifecycle ----
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ShowWidget();
	void HideWidget();
	bool IsWidgetVisible() const;

private:
	// ---- event handlers ----
	void HandleJobChanged(const TSharedPtr<FJsonValue>& Data);
	void HandleJobError(const TSharedPtr<FJsonValue>& Data);

	// ---- helpers ----
	void RecomputeEligibility();
	void RespawnLocalPlayerSprite(const FString& NewJobClass);
	static FString GetClassDisplayName(const FString& ClassId);

	// ---- state ----
	bool bWidgetAdded = false;

	TSharedPtr<SJobChangeWidget> Widget;
	TSharedPtr<SWidget> AlignmentWrapper;
	TSharedPtr<SWidget> ViewportOverlay;
};
