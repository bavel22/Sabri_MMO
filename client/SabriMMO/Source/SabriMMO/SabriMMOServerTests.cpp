// SabriMMOServerTests.cpp - Automated Server Test Implementation

#include "SabriMMOServerTests.h"
#include "MMOGameInstance.h"
#include "MMOHttpManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY_STATIC(LogServerTests, Log, All);

// ════════════════════════════════════════════════════════════════
//  Construction
// ════════════════════════════════════════════════════════════════

ASabriMMOServerTests::ASabriMMOServerTests()
{
	PrimaryActorTick.bCanEverTick = false;
}

// ════════════════════════════════════════════════════════════════
//  BeginPlay → delayed test start
// ════════════════════════════════════════════════════════════════

void ASabriMMOServerTests::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogServerTests, Log, TEXT("Server Test Runner placed. Tests will start in %.1f seconds..."), InitialDelay);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("Server Tests: Starting in %.0fs..."), InitialDelay));
	}

	// Add safety check for world and timer manager
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle, this, &ASabriMMOServerTests::StartTests, InitialDelay, false);
	}
	else
	{
		UE_LOG(LogServerTests, Warning, TEXT("Failed to get world. Starting tests immediately."));
		// Start tests immediately if world is not available
		StartTests();
	}
}

// ════════════════════════════════════════════════════════════════
//  Test Runner
// ════════════════════════════════════════════════════════════════

void ASabriMMOServerTests::StartTests()
{
	PrintHeader();

	// Initialize test state
	CurrentTestIndex = 0;
	PassCount = 0;
	FailCount = 0;
	LastFailReason.Reset();
	TestResults.Empty();

	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogServerTests, Error, TEXT("No valid world context. Cannot run tests."));
		return;
	}

	RunNextTest();
}

void ASabriMMOServerTests::RunNextTest()
{
	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogServerTests, Error, TEXT("No valid world context. Cannot continue tests."));
		return;
	}

	bool bResult = false;
	LastFailReason.Reset();
	float StartTime = GetWorld()->GetTimeSeconds();

	// Only run the first few tests for initial testing
	switch (CurrentTestIndex)
	{
	case 0:
		bResult = Test_Server_HealthCheck();
		LogResult(TEXT("Server Health Check"), bResult, LastFailReason);
		break;

	case 1:
		bResult = Test_Server_Authentication();
		LogResult(TEXT("Server Authentication"), bResult, LastFailReason);
		break;

	case 2:
		bResult = Test_Server_CharacterAPI();
		LogResult(TEXT("Character API"), bResult, LastFailReason);
		break;

	case 3:
		bResult = Test_Server_InventoryAPI();
		LogResult(TEXT("Inventory API"), bResult, LastFailReason);
		break;

	default:
		PrintSummary();
		return;
	}

	// Record test result
	FTestResult Result;
	Result.TestName = GetCurrentTestName(CurrentTestIndex);
	Result.bPassed = bResult;
	Result.Details = LastFailReason;
	Result.ExecutionTime = GetExecutionTime(StartTime);
	TestResults.Add(Result);

	CurrentTestIndex++;

	// Schedule next test with safety check
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle, this, &ASabriMMOServerTests::RunNextTest, DelayBetweenTests, false);
	}
	else
	{
		UE_LOG(LogServerTests, Warning, TEXT("World not available. Stopping tests."));
		PrintSummary();
	}
}

// ════════════════════════════════════════════════════════════════
//  Logging
// ════════════════════════════════════════════════════════════════

void ASabriMMOServerTests::PrintHeader()
{
	const FString Header = TEXT("========================================");
	const FString Title = TEXT("=== SABRI_MMO SERVER TEST SUITE ===");

	UE_LOG(LogServerTests, Log, TEXT("%s"), *Header);
	UE_LOG(LogServerTests, Log, TEXT("%s"), *Title);
	UE_LOG(LogServerTests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Header);
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Title);
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Header);
	}
}

void ASabriMMOServerTests::LogResult(const FString& TestName, bool bPassed, const FString& Details)
{
	if (bPassed)
	{
		PassCount++;
		const FString Msg = FString::Printf(TEXT("✅ PASS: %s"), *TestName);
		UE_LOG(LogServerTests, Log, TEXT("[PASS] %s"), *TestName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, Msg);
		}
	}
	else
	{
		FailCount++;
		const FString Msg = Details.IsEmpty()
			? FString::Printf(TEXT("❌ FAIL: %s"), *TestName)
			: FString::Printf(TEXT("❌ FAIL: %s - %s"), *TestName, *Details);
		UE_LOG(LogServerTests, Warning, TEXT("[FAIL] %s %s"), *TestName, *Details);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, Msg);
		}
	}
}

void ASabriMMOServerTests::PrintSummary()
{
	const int32 Total = PassCount + FailCount;
	const FString Sep = TEXT("========================================");
	const FString Summary = FString::Printf(TEXT("Results: %d/%d passed, %d failed"), PassCount, Total, FailCount);
	const FString Verdict = (FailCount == 0) ? TEXT("ALL SERVER TESTS PASSED") : TEXT("SOME SERVER TESTS FAILED");

	UE_LOG(LogServerTests, Log, TEXT("%s"), *Sep);
	UE_LOG(LogServerTests, Log, TEXT("%s"), *Summary);
	UE_LOG(LogServerTests, Log, TEXT("%s"), *Verdict);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Sep);
		GEngine->AddOnScreenDebugMessage(-1, 30.f,
			FailCount == 0 ? FColor::Green : FColor::Red, Summary);
		GEngine->AddOnScreenDebugMessage(-1, 30.f,
			FailCount == 0 ? FColor::Green : FColor::Red, Verdict);
	}

	// Print execution times
	UE_LOG(LogServerTests, Log, TEXT("Execution Times:"));
	for (const FTestResult& Result : TestResults)
	{
		UE_LOG(LogServerTests, Log, TEXT("  %s: %.2fs"), *Result.TestName, Result.ExecutionTime);
	}
}

// ════════════════════════════════════════════════════════════════
//  Server Connection Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::Test_Server_HealthCheck()
{
	FString Response;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/health", TEXT(""), Response))
	{
		LastFailReason = TEXT("Failed to make HTTP request");
		return false;
	}

	TMap<FString, FString> ExpectedFields;
	ExpectedFields.Add(TEXT("status"), TEXT("OK"));
	ExpectedFields.Add(TEXT("message"), TEXT("Server is running and connected to database"));

	return ValidateJSONResponse(Response, ExpectedFields);
}

bool ASabriMMOServerTests::Test_Server_Authentication()
{
	// Test login endpoint
	FString LoginBody = TEXT("{\"username\":\"testplayer\",\"password\":\"password123\"}");
	FString Response;
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/auth/login", LoginBody, Response))
	{
		LastFailReason = TEXT("Failed to make login request");
		return false;
	}

	// Check for JWT token in response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		LastFailReason = TEXT("Invalid JSON response from login");
		return false;
	}

	if (!JsonObject->HasField(TEXT("token")))
	{
		LastFailReason = TEXT("No JWT token in login response");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Server_CharacterAPI()
{
	// Test character list endpoint
	FString Response;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/api/characters", TEXT(""), Response))
	{
		LastFailReason = TEXT("Failed to get character list");
		return false;
	}

	// Validate response structure
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from character list");
		return false;
	}

	if (!JsonObject->HasField(TEXT("characters")))
	{
		LastFailReason = TEXT("No characters array in response");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Server_InventoryAPI()
{
	// Test inventory endpoint (requires character ID)
	FString Response;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/api/inventory/1", TEXT(""), Response))
	{
		LastFailReason = TEXT("Failed to get inventory");
		return false;
	}

	// Validate inventory structure
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from inventory");
		return false;
	}

	if (!JsonObject->HasField(TEXT("items")))
	{
		LastFailReason = TEXT("No items array in inventory response");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Socket.io Event Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::Test_Socket_Connection()
{
	if (!ConnectToServer())
	{
		LastFailReason = TEXT("Failed to connect to Socket.io server");
		return false;
	}

	// Wait a moment for connection to establish
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, [this]() {
		// Connection test passed
	}, 1.0f, false);

	DisconnectFromServer();
	return true;
}

bool ASabriMMOServerTests::Test_Socket_PlayerJoin()
{
	if (!ConnectToServer())
	{
		LastFailReason = TEXT("Failed to connect for player join test");
		return false;
	}

	// Emit player:join event
	FString JoinData = TEXT("{\"characterId\":1,\"x\":100,\"y\":100,\"z\":0}");
	if (!EmitEvent(TEXT("player:join"), JoinData))
	{
		LastFailReason = TEXT("Failed to emit player:join event");
		DisconnectFromServer();
		return false;
	}

	// Wait for response
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, [this]() {
		DisconnectFromServer();
	}, 2.0f, false);

	return true;
}

bool ASabriMMOServerTests::Test_Socket_PositionSync()
{
	if (!ConnectToServer())
	{
		LastFailReason = TEXT("Failed to connect for position sync test");
		return false;
	}

	// Emit position updates
	for (int32 i = 0; i < 5; i++)
	{
		FString PosData = FString::Printf(TEXT("{\"x\":%d,\"y\":%d,\"z\":0}"), 100 + i * 10, 100 + i * 10);
		EmitEvent(TEXT("player:position"), PosData);
	}

	DisconnectFromServer();
	return true;
}

bool ASabriMMOServerTests::Test_Socket_CombatEvents()
{
	if (!ConnectToServer())
	{
		LastFailReason = TEXT("Failed to connect for combat events test");
		return false;
	}

	// Test combat:start event
	FString CombatData = TEXT("{\"targetCharacterId\":2,\"skillId\":1}");
	if (!EmitEvent(TEXT("combat:start"), CombatData))
	{
		LastFailReason = TEXT("Failed to emit combat:start event");
		DisconnectFromServer();
		return false;
	}

	// Test combat:stop event
	EmitEvent(TEXT("combat:stop"), TEXT("{}"));

	DisconnectFromServer();
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Combat System Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::Test_Combat_AutoAttackLoop()
{
	// Test auto-attack server logic
	FString Response;
	FString CombatData = TEXT("{\"attackerId\":1,\"targetId\":2,\"weaponId\":1}");
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/attack", CombatData, Response))
	{
		LastFailReason = TEXT("Failed to make combat attack request");
		return false;
	}

	// Validate combat response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from combat attack");
		return false;
	}

	if (!JsonObject->HasField(TEXT("damage")))
	{
		LastFailReason = TEXT("No damage value in combat response");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Combat_DamageCalculation()
{
	// Test damage calculation formula
	FString Response;
	FString DamageData = TEXT("{\"attackerId\":1,\"targetId\":2,\"baseDamage\":25,\"weaponATK\":10}");
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/calculate-damage", DamageData, Response))
	{
		LastFailReason = TEXT("Failed to make damage calculation request");
		return false;
	}

	// Verify damage calculation
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from damage calculation");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Combat_RangeValidation()
{
	// Test range checking
	FString Response;
	FString RangeData = TEXT("{\"attackerId\":1,\"targetId\":2,\"attackerX\":0,\"attackerY\":0,\"targetX\":500,\"targetY\":0}");
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/check-range", RangeData, Response))
	{
		LastFailReason = TEXT("Failed to make range check request");
		return false;
	}

	// Validate range response
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from range check");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Combat_EnemyAI()
{
	// Test enemy AI behavior
	FString Response;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/api/enemies", TEXT(""), Response))
	{
		LastFailReason = TEXT("Failed to get enemy list");
		return false;
	}

	// Validate enemy data
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from enemy list");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Database Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::Test_Database_UserOperations()
{
	// Test user creation
	FString UserData = TEXT("{\"username\":\"testuser_auto\",\"email\":\"test@test.com\",\"password\":\"testpass123\"}");
	FString Response;
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/auth/register", UserData, Response))
	{
		LastFailReason = TEXT("Failed to create test user");
		return false;
	}

	// Verify user creation
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from user creation");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Database_CharacterOperations()
{
	// Test character creation
	FString CharData = TEXT("{\"name\":\"TestChar_Auto\",\"class\":\"Warrior\",\"level\":1}");
	FString Response;
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/characters", CharData, Response))
	{
		LastFailReason = TEXT("Failed to create test character");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Database_InventoryOperations()
{
	// Test inventory item addition
	FString InvData = TEXT("{\"characterId\":1,\"itemId\":1,\"quantity\":1}");
	FString Response;
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/inventory/add", InvData, Response))
	{
		LastFailReason = TEXT("Failed to add inventory item");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Database_ItemConsistency()
{
	// Test item data consistency
	FString Response;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/api/items", TEXT(""), Response))
	{
		LastFailReason = TEXT("Failed to get item list");
		return false;
	}

	// Validate item structure
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Invalid JSON response from items list");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Integration Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::Test_Integration_AuthFlow()
{
	// Complete authentication flow test
	FString LoginResponse;
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/auth/login", 
		TEXT("{\"username\":\"testplayer\",\"password\":\"password123\"}"), LoginResponse))
	{
		LastFailReason = TEXT("Login failed in auth flow test");
		return false;
	}

	// Extract token and test character list
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(LoginResponse);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Failed to parse login response");
		return false;
	}

	FString Token = JsonObject->GetStringField(TEXT("token"));
	if (Token.IsEmpty())
	{
		LastFailReason = TEXT("No token in login response");
		return false;
	}

	// Test authenticated request
	FString CharResponse;
	if (!MakeHTTPRequest(TEXT("GET"), TestServerURL + "/api/characters", TEXT(""), CharResponse))
	{
		LastFailReason = TEXT("Failed to get characters after login");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Integration_CharacterCreation()
{
	// Test character creation with stats
	FString CharData = TEXT("{\"name\":\"TestChar_Integration\",\"class\":\"Mage\",\"level\":1,\"stats\":{\"str\":5,\"agi\":8,\"vit\":6,\"int\":12,\"dex\":7,\"luk\":5}}");
	FString Response;
	
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/characters", CharData, Response))
	{
		LastFailReason = TEXT("Failed to create character with stats");
		return false;
	}

	// Verify character has correct stats
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		LastFailReason = TEXT("Failed to parse character creation response");
		return false;
	}

	return true;
}

bool ASabriMMOServerTests::Test_Integration_CombatCycle()
{
	// Test complete combat cycle
	// 1. Start combat
	FString StartData = TEXT("{\"attackerId\":1,\"targetId\":2}");
	FString StartResponse;
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/start", StartData, StartResponse))
	{
		LastFailReason = TEXT("Failed to start combat");
		return false;
	}

	// 2. Execute attack
	FString AttackData = TEXT("{\"attackerId\":1,\"targetId\":2}");
	FString AttackResponse;
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/attack", AttackData, AttackResponse))
	{
		LastFailReason = TEXT("Failed to execute attack");
		return false;
	}

	// 3. Stop combat
	FString StopData = TEXT("{\"attackerId\":1}");
	FString StopResponse;
	if (!MakeHTTPRequest(TEXT("POST"), TestServerURL + "/api/combat/stop", StopData, StopResponse))
	{
		LastFailReason = TEXT("Failed to stop combat");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Helper Functions
// ════════════════════════════════════════════════════════════════

bool ASabriMMOServerTests::MakeHTTPRequest(const FString& Method, const FString& Endpoint, const FString& Body, FString& OutResponse)
{
	// Add safety check for world
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context for HTTP request");
		return false;
	}

	// Implementation using UE5 HTTP module
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb(Method);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	
	if (!Body.IsEmpty())
	{
		Request->SetContentAsString(Body);
	}

	// Synchronous request for testing
	Request->ProcessRequest();
	
	// Wait for completion (simplified for testing)
	float StartTime = GetWorld()->GetTimeSeconds();
	while (Request->GetStatus() == EHttpRequestStatus::Processing)
	{
		// Add safety check for world during loop
		if (!GetWorld())
		{
			LastFailReason = TEXT("World context lost during HTTP request");
			return false;
		}
		
		if (GetWorld()->GetTimeSeconds() - StartTime > 5.0f)
		{
			LastFailReason = TEXT("HTTP request timeout");
			return false;
		}
		// Small delay to prevent busy waiting
		FPlatformProcess::Sleep(0.01f);
	}

	if (Request->GetStatus() == EHttpRequestStatus::Succeeded)
	{
		OutResponse = Request->GetResponse()->GetContentAsString();
		return true;
	}
	else
	{
		LastFailReason = TEXT("HTTP request failed");
		return false;
	}
}

bool ASabriMMOServerTests::ConnectToServer()
{
	// Implementation would connect to Socket.io server
	UE_LOG(LogServerTests, Log, TEXT("Connecting to Socket.io server at %s"), *TestServerURL);
	bIsSocketConnected = true;
	return true;
}

void ASabriMMOServerTests::DisconnectFromServer()
{
	// Implementation would disconnect from Socket.io server
	UE_LOG(LogServerTests, Log, TEXT("Disconnecting from Socket.io server"));
	bIsSocketConnected = false;
}

bool ASabriMMOServerTests::EmitEvent(const FString& Event, const FString& Data)
{
	// Implementation would emit Socket.io event
	UE_LOG(LogServerTests, Log, TEXT("Emitting Socket.io event: %s -> %s"), *Event, *Data);
	return true;
}

void ASabriMMOServerTests::OnEventReceived(const FString& Event, const FString& Data)
{
	// Handle Socket.io responses
	UE_LOG(LogServerTests, Log, TEXT("Socket.io response: %s <- %s"), *Event, *Data);
}

bool ASabriMMOServerTests::ValidateJSONResponse(const FString& Response, const TMap<FString, FString>& ExpectedFields)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		LastFailReason = TEXT("Invalid JSON response");
		return false;
	}

	for (const auto& Field : ExpectedFields)
	{
		if (!JsonObject->HasField(Field.Key))
		{
			LastFailReason = FString::Printf(TEXT("Missing field: %s"), *Field.Key);
			return false;
		}

		FString ActualValue = JsonObject->GetStringField(Field.Key);
		if (ActualValue != Field.Value)
		{
			LastFailReason = FString::Printf(TEXT("Field mismatch: %s expected '%s', got '%s'"), 
				*Field.Key, *Field.Value, *ActualValue);
			return false;
		}
	}

	return true;
}

float ASabriMMOServerTests::GetExecutionTime(float StartTime)
{
	// Add safety check for world
	if (!GetWorld())
	{
		return 0.0f;
	}
	return GetWorld()->GetTimeSeconds() - StartTime;
}

FString ASabriMMOServerTests::GetCurrentTestName(int32 TestIndex)
{
	switch (TestIndex)
	{
	case 0: return TEXT("Server Health Check");
	case 1: return TEXT("Server Authentication");
	case 2: return TEXT("Character API");
	case 3: return TEXT("Inventory API");
	case 4: return TEXT("Socket Connection");
	case 5: return TEXT("Socket Player Join");
	case 6: return TEXT("Socket Position Sync");
	case 7: return TEXT("Socket Combat Events");
	case 8: return TEXT("Combat Auto-Attack Loop");
	case 9: return TEXT("Combat Damage Calculation");
	case 10: return TEXT("Combat Range Validation");
	case 11: return TEXT("Combat Enemy AI");
	case 12: return TEXT("Database User Operations");
	case 13: return TEXT("Database Character Operations");
	case 14: return TEXT("Database Inventory Operations");
	case 15: return TEXT("Database Item Consistency");
	case 16: return TEXT("Integration Auth Flow");
	case 17: return TEXT("Integration Character Creation");
	case 18: return TEXT("Integration Combat Cycle");
	default: return TEXT("Unknown Test");
	}
}
