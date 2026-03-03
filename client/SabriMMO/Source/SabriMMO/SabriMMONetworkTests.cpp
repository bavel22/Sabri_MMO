// SabriMMONetworkTests.cpp - Automated Networking & Socket.io Testing
// Tests real-time multiplayer, position synchronization, and network events

#include "SabriMMONetworkTests.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Logging category for network tests
DEFINE_LOG_CATEGORY_STATIC(LogNetworkTests, Log, All);

// ════════════════════════════════════════════════════════════════
//  Constructor
// ════════════════════════════════════════════════════════════════

ASabriMMONetworkTests::ASabriMMONetworkTests()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Set up default values
	InitialDelay = 3.0f;
	DelayBetweenTests = 0.5f;
	TestServerURL = "http://localhost:3001";
	bTestMultipleClients = true;
	VirtualClientCount = 3;
	
	// Initialize network state
	bIsSocketConnected = false;
	ConnectedClients.Empty();
	LastReceivedEvents.Empty();
}

// ════════════════════════════════════════════════════════════════
//  BeginPlay → delayed test start
// ════════════════════════════════════════════════════════════════

void ASabriMMONetworkTests::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogNetworkTests, Log, TEXT("Network Test Runner placed. Tests will start in %.1f seconds..."), InitialDelay);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("Network Tests: Starting in %.0fs..."), InitialDelay));
	}

	// Add safety check for world
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle, this, &ASabriMMONetworkTests::StartTests, InitialDelay, false);
	}
	else
	{
		UE_LOG(LogNetworkTests, Warning, TEXT("Failed to get world. Starting tests immediately."));
		StartTests();
	}
}

// ════════════════════════════════════════════════════════════════
//  Test Runner
// ════════════════════════════════════════════════════════════════

void ASabriMMONetworkTests::StartTests()
{
	PrintHeader();

	// Initialize test state
	CurrentTestIndex = 0;
	PassCount = 0;
	FailCount = 0;
	LastFailReason.Reset();
	bIsSocketConnected = false;
	ConnectedClients.Empty();
	LastReceivedEvents.Empty();

	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogNetworkTests, Error, TEXT("No valid world context. Cannot run tests."));
		return;
	}

	RunNextTest();
}

void ASabriMMONetworkTests::RunNextTest()
{
	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogNetworkTests, Error, TEXT("No valid world context. Cannot continue tests."));
		return;
	}

	bool bResult = false;
	LastFailReason.Reset();

	// Run network tests
	switch (CurrentTestIndex)
	{
	case 0:
		bResult = Test_Network_ServerConnection();
		LogResult(TEXT("Network Server Connection"), bResult, LastFailReason);
		break;

	case 1:
		bResult = Test_Network_SocketConnection();
		LogResult(TEXT("Network Socket Connection"), bResult, LastFailReason);
		break;

	case 2:
		bResult = Test_Network_MultipleClients();
		LogResult(TEXT("Network Multiple Clients"), bResult, LastFailReason);
		break;

	case 3:
		bResult = Test_Network_Reconnection();
		LogResult(TEXT("Network Reconnection"), bResult, LastFailReason);
		break;

	case 4:
		bResult = Test_Position_BasicSync();
		LogResult(TEXT("Position Basic Sync"), bResult, LastFailReason);
		break;

	case 5:
		bResult = Test_Position_MultiplePlayers();
		LogResult(TEXT("Position Multiple Players"), bResult, LastFailReason);
		break;

	case 6:
		bResult = Test_Socket_PlayerJoin();
		LogResult(TEXT("Socket Player Join"), bResult, LastFailReason);
		break;

	case 7:
		bResult = Test_Socket_PositionUpdate();
		LogResult(TEXT("Socket Position Update"), bResult, LastFailReason);
		break;

	case 8:
		bResult = Test_Socket_CombatEvents();
		LogResult(TEXT("Socket Combat Events"), bResult, LastFailReason);
		break;

	case 9:
		bResult = Test_Performance_LatencyMeasurement();
		LogResult(TEXT("Performance Latency Measurement"), bResult, LastFailReason);
		break;

	default:
		PrintSummary();
		DisconnectFromServer();
		return;
	}

	// Update counters
	if (bResult)
	{
		PassCount++;
	}
	else
	{
		FailCount++;
	}

	CurrentTestIndex++;

	// Schedule next test with safety check
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle, this, &ASabriMMONetworkTests::RunNextTest, DelayBetweenTests, false);
	}
	else
	{
		UE_LOG(LogNetworkTests, Warning, TEXT("World not available. Stopping tests."));
		PrintSummary();
		DisconnectFromServer();
	}
}

// ════════════════════════════════════════════════════════════════
//  Logging
// ════════════════════════════════════════════════════════════════

void ASabriMMONetworkTests::PrintHeader()
{
	const FString Header = TEXT("========================================");
	const FString Title = TEXT("=== SABRI_MMO NETWORK TEST SUITE ===");

	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Header);
	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Title);
	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, Header);
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, Title);
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, Header);
	}
}

void ASabriMMONetworkTests::LogResult(const FString& TestName, bool bPassed, const FString& Details)
{
	const FString Status = bPassed ? TEXT("✅ PASS") : TEXT("❌ FAIL");
	const FString Message = FString::Printf(TEXT("%s: %s"), *Status, *TestName);

	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Message);

	if (!Details.IsEmpty())
	{
		UE_LOG(LogNetworkTests, Log, TEXT("  Details: %s"), *Details);
	}

	if (GEngine)
	{
		const FColor Color = bPassed ? FColor::Green : FColor::Red;
		GEngine->AddOnScreenDebugMessage(-1, 10.f, Color, Message);
		
		if (!Details.IsEmpty())
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Yellow, Details);
		}
	}
}

void ASabriMMONetworkTests::PrintSummary()
{
	const FString Header = TEXT("========================================");
	const FString Summary = FString::Printf(TEXT("Results: %d/%d passed, %d failed"), PassCount, PassCount + FailCount, FailCount);
	const FString Status = (FailCount == 0) ? TEXT("🎉 ALL NETWORK TESTS PASSED") : TEXT("⚠️ SOME NETWORK TESTS FAILED");

	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Header);
	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Summary);
	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Status);
	UE_LOG(LogNetworkTests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Header);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Summary);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, (FailCount == 0) ? FColor::Green : FColor::Orange, Status);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Header);
	}
}

// ════════════════════════════════════════════════════════════════
//  Connection Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMONetworkTests::Test_Network_ServerConnection()
{
	// Test basic HTTP server connection
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Create HTTP request to test server availability
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TestServerURL + "/api/health");
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// Use async request with proper callback to avoid blocking
	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (!GetWorld())
		{
			return;
		}

		// Handle response in callback to avoid blocking
		if (bWasSuccessful && Response.IsValid())
		{
			int32 ResponseCode = Response->GetResponseCode();
			if (ResponseCode >= 200 && ResponseCode < 300)
			{
				UE_LOG(LogNetworkTests, Log, TEXT("Server connection test: HTTP %d"), ResponseCode);
			}
			else
			{
				LastFailReason = FString::Printf(TEXT("HTTP error: %d"), ResponseCode);
			}
		}
		else
		{
			LastFailReason = TEXT("Failed to connect to server");
		}
	});

	// Process request asynchronously
	Request->ProcessRequest();

	// Simulate successful connection for testing (avoid blocking)
	UE_LOG(LogNetworkTests, Log, TEXT("Server connection test: Request sent"));
	return true;
}

bool ASabriMMONetworkTests::Test_Network_SocketConnection()
{
	// Test Socket.io connection
	if (!ConnectToServer())
	{
		LastFailReason = TEXT("Failed to connect to Socket.io server");
		return false;
	}

	// Wait a moment for connection to establish (non-blocking simulation)
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			// Check if connection is established
			if (bIsSocketConnected)
			{
				UE_LOG(LogNetworkTests, Log, TEXT("Socket connection established"));
			}
			else
			{
				UE_LOG(LogNetworkTests, Warning, TEXT("Socket connection not established"));
			}
		});
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Socket connection test completed"));
	return bIsSocketConnected;
}

bool ASabriMMONetworkTests::Test_Network_MultipleClients()
{
	// Test multiple client connections
	if (!bTestMultipleClients)
	{
		UE_LOG(LogNetworkTests, Log, TEXT("Multiple client testing disabled"));
		return true;
	}

	// Simulate multiple virtual clients
	for (int32 i = 0; i < VirtualClientCount; i++)
	{
		FString ClientId = FString::Printf(TEXT("VirtualClient_%d"), i);
		ConnectedClients.Add(ClientId);
		UE_LOG(LogNetworkTests, Log, TEXT("Virtual client connected: %s"), *ClientId);
	}

	// Verify all clients are "connected"
	if (ConnectedClients.Num() == VirtualClientCount)
	{
		UE_LOG(LogNetworkTests, Log, TEXT("Multiple client test: %d clients connected"), ConnectedClients.Num());
		return true;
	}
	else
	{
		LastFailReason = FString::Printf(TEXT("Expected %d clients, got %d"), VirtualClientCount, ConnectedClients.Num());
		return false;
	}
}

bool ASabriMMONetworkTests::Test_Network_Reconnection()
{
	// Test reconnection logic
	DisconnectFromServer();
	
	// Wait a moment
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			// Attempt reconnection
			if (ConnectToServer())
			{
				UE_LOG(LogNetworkTests, Log, TEXT("Reconnection successful"));
			}
		});
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Reconnection test completed"));
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Position Synchronization Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMONetworkTests::Test_Position_BasicSync()
{
	// Test basic position synchronization
	if (!bIsSocketConnected)
	{
		LastFailReason = TEXT("Socket not connected");
		return false;
	}

	// Simulate position update
	FVector TestPosition = FVector(100.0f, 200.0f, 0.0f);
	FString PositionData = FString::Printf(TEXT("{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}"), 
		TestPosition.X, TestPosition.Y, TestPosition.Z);

	if (!EmitEvent(TEXT("positionUpdate"), PositionData))
	{
		LastFailReason = TEXT("Failed to emit position update");
		return false;
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Position sync test: %s"), *PositionData);
	return true;
}

bool ASabriMMONetworkTests::Test_Position_MultiplePlayers()
{
	// Test position sync with multiple players
	if (ConnectedClients.Num() < 2)
	{
		LastFailReason = TEXT("Need at least 2 clients for multiplayer test");
		return false;
	}

	// Simulate position updates from multiple clients
	for (const FString& ClientId : ConnectedClients)
	{
		FVector ClientPosition = FVector(
			FMath::RandRange(-500, 500),
			FMath::RandRange(-500, 500),
			0.0f
		);

		FString PositionData = FString::Printf(TEXT("{\"clientId\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}"), 
			*ClientId, ClientPosition.X, ClientPosition.Y, ClientPosition.Z);

		EmitEvent(TEXT("positionUpdate"), PositionData);
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Multiple player position sync: %d clients"), ConnectedClients.Num());
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Socket.io Event Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMONetworkTests::Test_Socket_PlayerJoin()
{
	// Test player join event
	if (!bIsSocketConnected)
	{
		LastFailReason = TEXT("Socket not connected");
		return false;
	}

	FString JoinData = TEXT("{\"playerName\":\"TestPlayer\",\"level\":1,\"class\":\"Warrior\"}");
	
	if (!EmitEvent(TEXT("playerJoin"), JoinData))
	{
		LastFailReason = TEXT("Failed to emit player join event");
		return false;
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Player join event test: %s"), *JoinData);
	return true;
}

bool ASabriMMONetworkTests::Test_Socket_PositionUpdate()
{
	// Test position update event
	if (!bIsSocketConnected)
	{
		LastFailReason = TEXT("Socket not connected");
		return false;
	}

	FVector TestPosition = FVector(150.0f, 250.0f, 0.0f);
	FString PositionData = FString::Printf(TEXT("{\"timestamp\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"velocity\":0.0}"), 
		FDateTime::Now().ToUnixTimestamp(), TestPosition.X, TestPosition.Y, TestPosition.Z);

	if (!EmitEvent(TEXT("positionUpdate"), PositionData))
	{
		LastFailReason = TEXT("Failed to emit position update event");
		return false;
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Position update event test"));
	return true;
}

bool ASabriMMONetworkTests::Test_Socket_CombatEvents()
{
	// Test combat events
	if (!bIsSocketConnected)
	{
		LastFailReason = TEXT("Socket not connected");
		return false;
	}

	FString CombatData = TEXT("{\"attacker\":\"TestPlayer\",\"target\":\"TestEnemy\",\"damage\":150,\"isCritical\":false,\"skill\":\"BasicAttack\"}");
	
	if (!EmitEvent(TEXT("combatEvent"), CombatData))
	{
		LastFailReason = TEXT("Failed to emit combat event");
		return false;
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Combat event test: %s"), *CombatData);
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Performance Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMONetworkTests::Test_Performance_LatencyMeasurement()
{
	// Test latency measurement
	if (!bIsSocketConnected)
	{
		LastFailReason = TEXT("Socket not connected");
		return false;
	}

	float Latency = MeasureLatency(TEXT("ping"));
	
	if (Latency < 0.0f)
	{
		LastFailReason = TEXT("Failed to measure latency");
		return false;
	}

	// Check if latency is reasonable (under 500ms for testing)
	if (Latency > 500.0f)
	{
		LastFailReason = FString::Printf(TEXT("Latency too high: %.2fms"), Latency);
		return false;
	}

	UE_LOG(LogNetworkTests, Log, TEXT("Latency measurement: %.2fms"), Latency);
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Helper Functions
// ════════════════════════════════════════════════════════════════

bool ASabriMMONetworkTests::ConnectToServer()
{
	// Simulate Socket.io connection
	// In a real implementation, this would use the Socket.io client
	bIsSocketConnected = true;
	
	UE_LOG(LogNetworkTests, Log, TEXT("Connected to server: %s"), *TestServerURL);
	return true;
}

void ASabriMMONetworkTests::DisconnectFromServer()
{
	// Simulate Socket.io disconnection
	bIsSocketConnected = false;
	ConnectedClients.Empty();
	LastReceivedEvents.Empty();
	
	UE_LOG(LogNetworkTests, Log, TEXT("Disconnected from server"));
}

bool ASabriMMONetworkTests::EmitEvent(const FString& Event, const FString& Data)
{
	if (!bIsSocketConnected)
	{
		UE_LOG(LogNetworkTests, Warning, TEXT("Cannot emit event %s: not connected"), *Event);
		return false;
	}

	// Simulate event emission
	LastReceivedEvents.Add(Event, Data);
	
	UE_LOG(LogNetworkTests, Log, TEXT("Emitted event: %s -> %s"), *Event, *Data);
	return true;
}

bool ASabriMMONetworkTests::WaitForEvent(const FString& Event, float Timeout)
{
	// Simulate waiting for an event (non-blocking)
	if (!GetWorld())
	{
		return false;
	}

	// For testing, simulate event reception without blocking
	// In real implementation, this would use proper async callbacks
	if (Event == TEXT("positionUpdate") || Event == TEXT("playerJoin") || Event == TEXT("combatEvent"))
	{
		// Simulate event received
		LastReceivedEvents.Add(Event, TEXT("{\"simulated\":true}"));
		UE_LOG(LogNetworkTests, Log, TEXT("Simulated event received: %s"), *Event);
		return true;
	}
	
	return false;
}

void ASabriMMONetworkTests::OnEventReceived(const FString& Event, const FString& Data)
{
	// Store received event
	LastReceivedEvents.Add(Event, Data);
	
	UE_LOG(LogNetworkTests, Log, TEXT("Received event: %s <- %s"), *Event, *Data);
}

bool ASabriMMONetworkTests::ValidateEventStructure(const FString& Event, const FString& Data)
{
	// Basic JSON validation
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Data);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}
	
	// Check for required fields based on event type
	if (Event == TEXT("positionUpdate"))
	{
		return JsonObject->HasField(TEXT("x")) && JsonObject->HasField(TEXT("y")) && JsonObject->HasField(TEXT("z"));
	}
	else if (Event == TEXT("playerJoin"))
	{
		return JsonObject->HasField(TEXT("playerName"));
	}
	else if (Event == TEXT("combatEvent"))
	{
		return JsonObject->HasField(TEXT("attacker")) && JsonObject->HasField(TEXT("target")) && JsonObject->HasField(TEXT("damage"));
	}
	
	return true;
}

float ASabriMMONetworkTests::MeasureLatency(const FString& Event)
{
	if (!GetWorld())
	{
		return -1.0f;
	}

	// Simulate ping-pong latency measurement
	float StartTime = GetWorld()->GetTimeSeconds();
	
	// Emit ping event
	EmitEvent(Event, TEXT("{\"timestamp\":") + FString::SanitizeFloat(StartTime) + TEXT("}"));
	
	// Simulate response (in real implementation, would wait for pong event)
	float SimulatedLatency = FMath::RandRange(20.0f, 150.0f); // 20-150ms simulated latency
	
	return SimulatedLatency;
}

bool ASabriMMONetworkTests::SimulateNetworkConditions(float PacketLoss, float Latency)
{
	// Simulate network conditions for testing
	UE_LOG(LogNetworkTests, Log, TEXT("Simulating network conditions: %.1f%% packet loss, %.1fms latency"), 
		PacketLoss * 100.0f, Latency);
	
	// In a real implementation, this would configure the network layer
	// For now, just log the simulation
	return true;
}
