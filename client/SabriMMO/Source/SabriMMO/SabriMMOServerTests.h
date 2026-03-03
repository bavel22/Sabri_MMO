// SabriMMOServerTests.h - Automated Server Test Framework
// Comprehensive testing for server-side logic, API endpoints, and Socket.io events

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SabriMMOServerTests.generated.h"

UCLASS(Blueprintable, meta=(DisplayName="Server Test Runner"))
class SABRIMMO_API ASabriMMOServerTests : public AActor
{
	GENERATED_BODY()

public:
	ASabriMMOServerTests();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tests")
	float InitialDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tests")
	float DelayBetweenTests = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tests")
	FString TestServerURL = "http://localhost:3001";

protected:
	virtual void BeginPlay() override;

private:
	// ── Test State ──
	int32 CurrentTestIndex = 0;
	int32 PassCount = 0;
	int32 FailCount = 0;
	FString LastFailReason;
	FTimerHandle TestTimerHandle;

	// ── Test Results ──
	struct FTestResult
	{
		FString TestName;
		bool bPassed;
		FString Details;
		float ExecutionTime;
	};

	TArray<FTestResult> TestResults;

	// ── Test Runner ──
	void StartTests();
	void RunNextTest();
	void LogResult(const FString& TestName, bool bPassed, const FString& Details = TEXT(""));
	void PrintHeader();
	void PrintSummary();

	// ── Server Connection Tests ──
	bool Test_Server_HealthCheck();
	bool Test_Server_Authentication();
	bool Test_Server_CharacterAPI();
	bool Test_Server_InventoryAPI();

	// ── Socket.io Event Tests ──
	bool Test_Socket_Connection();
	bool Test_Socket_PlayerJoin();
	bool Test_Socket_PositionSync();
	bool Test_Socket_CombatEvents();

	// ── Combat System Tests ──
	bool Test_Combat_AutoAttackLoop();
	bool Test_Combat_DamageCalculation();
	bool Test_Combat_RangeValidation();
	bool Test_Combat_EnemyAI();

	// ── Database Tests ──
	bool Test_Database_UserOperations();
	bool Test_Database_CharacterOperations();
	bool Test_Database_InventoryOperations();
	bool Test_Database_ItemConsistency();

	// ── Integration Tests ──
	bool Test_Integration_AuthFlow();
	bool Test_Integration_CharacterCreation();
	bool Test_Integration_CombatCycle();

	// ── Network State ──
	bool bIsSocketConnected;
	TArray<FString> ConnectedClients;
	TMap<FString, FString> LastReceivedEvents;

	// ── Helper Functions ──
	bool MakeHTTPRequest(const FString& Method, const FString& Endpoint, const FString& Body, FString& OutResponse);
	bool ConnectToServer();
	void DisconnectFromServer();
	bool EmitEvent(const FString& Event, const FString& Data);
	void OnEventReceived(const FString& Event, const FString& Data);
	bool ValidateJSONResponse(const FString& Response, const TMap<FString, FString>& ExpectedFields);
	float GetExecutionTime(float StartTime);
	FString GetCurrentTestName(int32 TestIndex);
};
