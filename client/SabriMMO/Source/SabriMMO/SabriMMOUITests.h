// SabriMMOUITests.h - Automated UI Test Runner
// Place this actor in any map. Tests auto-run on BeginPlay after a short delay.
// Results print to screen and Output Log.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SabriMMOUITests.generated.h"

UCLASS(Blueprintable, meta=(DisplayName="UI Test Runner"))
class SABRIMMO_API ASabriMMOUITests : public AActor
{
	GENERATED_BODY()

public:
	ASabriMMOUITests();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tests")
	float InitialDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tests")
	float DelayBetweenTests = 0.5f;

protected:
	virtual void BeginPlay() override;

private:
	// ── Test State ──
	int32 CurrentTestIndex = 0;
	int32 PassCount = 0;
	int32 FailCount = 0;
	FString LastFailReason;
	FTimerHandle TestTimerHandle;

	// ── Cached References ──
	UPROPERTY()
	TObjectPtr<APawn> PlayerPawn;

	UPROPERTY()
	TObjectPtr<UActorComponent> HUDManagerComp;

	// ── Test Runner ──
	void StartTests();
	void RunNextTest();
	void LogResult(const FString& TestName, bool bPassed, const FString& Details = TEXT(""));
	void PrintHeader();
	void PrintSummary();

	// ── Individual Tests ──
	bool Test_GameInstanceValid();
	bool Test_PlayerCharacterValid();
	bool Test_HUDManagerFound();
	bool Test_InventoryToggle();
	bool Test_ZuzucoinUpdate();

	// ── Reflection Helpers ──
	UActorComponent* FindComponentByClassName(AActor* Actor, const FString& ClassName) const;
	bool CallBlueprintFunction(UObject* Object, const FString& FunctionName, void* Params = nullptr) const;
	int32 GetIntProperty(UObject* Object, const FString& PropertyName) const;
	UObject* GetObjectProperty(UObject* Object, const FString& PropertyName) const;
	bool GetBoolProperty(UObject* Object, const FString& PropertyName) const;
};
