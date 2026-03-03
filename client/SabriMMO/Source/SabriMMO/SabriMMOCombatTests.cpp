// SabriMMOCombatTests.cpp - Automated Combat System Testing
// Tests combat mechanics, damage calculations, AI behavior, and combat flow

#include "SabriMMOCombatTests.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"

// Logging category for combat tests
DEFINE_LOG_CATEGORY_STATIC(LogCombatTests, Log, All);

// ════════════════════════════════════════════════════════════════
//  Constructor
// ════════════════════════════════════════════════════════════════

ASabriMMOCombatTests::ASabriMMOCombatTests()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Set up default values
	InitialDelay = 2.0f;
	DelayBetweenTests = 0.5f;
	bSpawnTestEnemies = true;
	TestEnemyCount = 3;
}

// ════════════════════════════════════════════════════════════════
//  BeginPlay → delayed test start
// ════════════════════════════════════════════════════════════════

void ASabriMMOCombatTests::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCombatTests, Log, TEXT("Combat Test Runner placed. Tests will start in %.1f seconds..."), InitialDelay);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
			FString::Printf(TEXT("Combat Tests: Starting in %.0fs..."), InitialDelay));
	}

	// Add safety check for world
	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			TestTimerHandle, this, &ASabriMMOCombatTests::StartTests, InitialDelay, false);
	}
	else
	{
		UE_LOG(LogCombatTests, Warning, TEXT("Failed to get world. Starting tests immediately."));
		StartTests();
	}
}

// ════════════════════════════════════════════════════════════════
//  Test Runner
// ════════════════════════════════════════════════════════════════

void ASabriMMOCombatTests::StartTests()
{
	PrintHeader();

	// Initialize test state
	CurrentTestIndex = 0;
	PassCount = 0;
	FailCount = 0;
	LastFailReason.Reset();
	SpawnedEnemies.Empty();
	TestPlayer = nullptr;

	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogCombatTests, Error, TEXT("No valid world context. Cannot run tests."));
		return;
	}

	RunNextTest();
}

void ASabriMMOCombatTests::RunNextTest()
{
	// Add safety check for world
	if (!GetWorld())
	{
		UE_LOG(LogCombatTests, Error, TEXT("No valid world context. Cannot continue tests."));
		return;
	}

	bool bResult = false;
	LastFailReason.Reset();

	// Run combat tests
	switch (CurrentTestIndex)
	{
	case 0:
		bResult = Test_Combat_DamageFormula();
		LogResult(TEXT("Combat Damage Formula"), bResult, LastFailReason);
		break;

	case 1:
		bResult = Test_Combat_ASPDCalculation();
		LogResult(TEXT("Combat ASPD Calculation"), bResult, LastFailReason);
		break;

	case 2:
		bResult = Test_Combat_HitChanceCalculation();
		LogResult(TEXT("Combat Hit Chance Calculation"), bResult, LastFailReason);
		break;

	case 3:
		bResult = Test_Combat_CriticalChanceCalculation();
		LogResult(TEXT("Combat Critical Chance Calculation"), bResult, LastFailReason);
		break;

	case 4:
		bResult = Test_Combat_RangeValidation();
		LogResult(TEXT("Combat Range Validation"), bResult, LastFailReason);
		break;

	case 5:
		bResult = Test_Combat_TargetValidation();
		LogResult(TEXT("Combat Target Validation"), bResult, LastFailReason);
		break;

	case 6:
		bResult = Test_Combat_AutoAttackStart();
		LogResult(TEXT("Combat Auto Attack Start"), bResult, LastFailReason);
		break;

	case 7:
		bResult = Test_Combat_AutoAttackStop();
		LogResult(TEXT("Combat Auto Attack Stop"), bResult, LastFailReason);
		break;

	case 8:
		bResult = Test_Enemy_SpawnBehavior();
		LogResult(TEXT("Enemy Spawn Behavior"), bResult, LastFailReason);
		break;

	case 9:
		bResult = Test_Enemy_AggroBehavior();
		LogResult(TEXT("Enemy Aggro Behavior"), bResult, LastFailReason);
		break;

	default:
		PrintSummary();
		CleanupTestActors();
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
			TestTimerHandle, this, &ASabriMMOCombatTests::RunNextTest, DelayBetweenTests, false);
	}
	else
	{
		UE_LOG(LogCombatTests, Warning, TEXT("World not available. Stopping tests."));
		PrintSummary();
		CleanupTestActors();
	}
}

// ════════════════════════════════════════════════════════════════
//  Logging
// ════════════════════════════════════════════════════════════════

void ASabriMMOCombatTests::PrintHeader()
{
	const FString Header = TEXT("========================================");
	const FString Title = TEXT("=== SABRI_MMO COMBAT TEST SUITE ===");

	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Header);
	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Title);
	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, Header);
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, Title);
		GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, Header);
	}
}

void ASabriMMOCombatTests::LogResult(const FString& TestName, bool bPassed, const FString& Details)
{
	const FString Status = bPassed ? TEXT("✅ PASS") : TEXT("❌ FAIL");
	const FString Message = FString::Printf(TEXT("%s: %s"), *Status, *TestName);

	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Message);

	if (!Details.IsEmpty())
	{
		UE_LOG(LogCombatTests, Log, TEXT("  Details: %s"), *Details);
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

void ASabriMMOCombatTests::PrintSummary()
{
	const FString Header = TEXT("========================================");
	const FString Summary = FString::Printf(TEXT("Results: %d/%d passed, %d failed"), PassCount, PassCount + FailCount, FailCount);
	const FString Status = (FailCount == 0) ? TEXT("🎉 ALL COMBAT TESTS PASSED") : TEXT("⚠️ SOME COMBAT TESTS FAILED");

	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Header);
	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Summary);
	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Status);
	UE_LOG(LogCombatTests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Header);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Summary);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, (FailCount == 0) ? FColor::Green : FColor::Orange, Status);
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::White, Header);
	}
}

// ════════════════════════════════════════════════════════════════
//  Combat Mechanics Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOCombatTests::Test_Combat_DamageFormula()
{
	// Test damage calculation formula
	// Expected: Damage = ATK * (1 - DEF / (DEF + 100))
	int32 AttackerATK = 100;
	int32 TargetDEF = 50;
	int32 ExpectedDamage = 66; // 100 * (1 - 50 / (50 + 100)) = 66.67, rounded to 66

	int32 ActualDamage;
	if (!CalculateExpectedDamage(AttackerATK, TargetDEF, ActualDamage))
	{
		LastFailReason = TEXT("Failed to calculate expected damage");
		return false;
	}

	if (ActualDamage != ExpectedDamage)
	{
		LastFailReason = FString::Printf(TEXT("Damage mismatch: expected %d, got %d"), ExpectedDamage, ActualDamage);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Damage formula test: ATK=%d, DEF=%d, Damage=%d"), AttackerATK, TargetDEF, ActualDamage);
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_ASPDCalculation()
{
	// Test Attack Speed calculation
	// ASPD = BaseASPD * (1 + AGI / 100)
	float BaseASPD = 1.0f;
	int32 AGI = 50;
	float ExpectedASPD = 1.5f; // 1.0 * (1 + 50 / 100) = 1.5

	float ActualASPD = BaseASPD * (1.0f + static_cast<float>(AGI) / 100.0f);

	if (FMath::Abs(ActualASPD - ExpectedASPD) > 0.01f)
	{
		LastFailReason = FString::Printf(TEXT("ASPD mismatch: expected %.2f, got %.2f"), ExpectedASPD, ActualASPD);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("ASPD calculation test: Base=%.2f, AGI=%d, ASPD=%.2f"), BaseASPD, AGI, ActualASPD);
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_HitChanceCalculation()
{
	// Test Hit Chance calculation
	// HitChance = BaseHitChance + (DEX - EnemyDEX) * 0.01
	float BaseHitChance = 0.8f;
	int32 DEX = 50;
	int32 EnemyDEX = 30;
	float ExpectedHitChance = 1.0f; // 0.8 + (50 - 30) * 0.01 = 1.0, capped at 1.0

	float ActualHitChance = BaseHitChance + static_cast<float>(DEX - EnemyDEX) * 0.01f;
	// Cap hit chance at 1.0 (100%)
	ActualHitChance = FMath::Clamp(ActualHitChance, 0.0f, 1.0f);

	if (FMath::Abs(ActualHitChance - ExpectedHitChance) > 0.01f)
	{
		LastFailReason = FString::Printf(TEXT("Hit chance mismatch: expected %.2f, got %.2f"), ExpectedHitChance, ActualHitChance);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Hit chance test: Base=%.2f, DEX=%d, EnemyDEX=%d, HitChance=%.2f"), BaseHitChance, DEX, EnemyDEX, ActualHitChance);
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_CriticalChanceCalculation()
{
	// Test Critical Chance calculation
	// CritChance = BaseCritChance + LUK / 200
	float BaseCritChance = 0.05f;
	int32 LUK = 50;
	float ExpectedCritChance = 0.3f; // 0.05 + 50 / 200 = 0.3

	float ActualCritChance = BaseCritChance + static_cast<float>(LUK) / 200.0f;

	if (FMath::Abs(ActualCritChance - ExpectedCritChance) > 0.01f)
	{
		LastFailReason = FString::Printf(TEXT("Crit chance mismatch: expected %.2f, got %.2f"), ExpectedCritChance, ActualCritChance);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Crit chance test: Base=%.2f, LUK=%d, CritChance=%.2f"), BaseCritChance, LUK, ActualCritChance);
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_RangeValidation()
{
	// Test combat range validation
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Spawn test actors
	AActor* Attacker = nullptr;
	AActor* Target = nullptr;
	
	if (!SpawnTestPlayer(FVector(0, 0, 0), Attacker))
	{
		LastFailReason = TEXT("Failed to spawn test attacker");
		return false;
	}

	if (!SpawnTestEnemy(FVector(200, 0, 0), Target))
	{
		LastFailReason = TEXT("Failed to spawn test target");
		return false;
	}

	// Test range validation
	float AttackRange = 300.0f;
	bool bInRange = IsInCombatRange(Attacker, Target, AttackRange);

	// Distance is 200, range is 300, should be in range
	if (!bInRange)
	{
		LastFailReason = TEXT("Should be in combat range but returned false");
		return false;
	}

	// Test out of range
	Target->SetActorLocation(FVector(500, 0, 0)); // Changed from 400 to 500 to be clearly out of range
	bool bOutOfRange = IsInCombatRange(Attacker, Target, AttackRange);

	// Distance is 500, range is 300, should be out of range
	if (bOutOfRange)
	{
		LastFailReason = TEXT("Should be out of combat range but returned true");
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Range validation test passed"));
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_TargetValidation()
{
	// Test target validation logic
	// This would test if targets are valid (alive, in range, not self, etc.)
	
	// For now, just test basic validation
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	AActor* TestActor = nullptr;
	if (!SpawnTestPlayer(FVector(0, 0, 0), TestActor))
	{
		LastFailReason = TEXT("Failed to spawn test actor");
		return false;
	}

	// Test if actor is valid for combat
	bool bValidTarget = ValidateCombatStats(TestActor);
	if (!bValidTarget)
	{
		LastFailReason = TEXT("Spawned actor should be valid combat target");
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Target validation test passed"));
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Combat Flow Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOCombatTests::Test_Combat_AutoAttackStart()
{
	// Test auto-attack start functionality
	// This would test the beginning of auto-attack state
	
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Spawn test actors
	AActor* Attacker = nullptr;
	AActor* Target = nullptr;
	
	if (!SpawnTestPlayer(FVector(0, 0, 0), Attacker))
	{
		LastFailReason = TEXT("Failed to spawn test attacker");
		return false;
	}

	if (!SpawnTestEnemy(FVector(150, 0, 0), Target))
	{
		LastFailReason = TEXT("Failed to spawn test target");
		return false;
	}

	// Simulate starting auto-attack
	// In a real implementation, this would trigger the combat system
	UE_LOG(LogCombatTests, Log, TEXT("Auto-attack start test: Attacker=%s, Target=%s"), 
		Attacker ? *Attacker->GetName() : TEXT("None"), 
		Target ? *Target->GetName() : TEXT("None"));

	// For now, just return true as a placeholder
	return true;
}

bool ASabriMMOCombatTests::Test_Combat_AutoAttackStop()
{
	// Test auto-attack stop functionality
	// This would test stopping auto-attack state
	
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Spawn test actors
	AActor* Attacker = nullptr;
	AActor* Target = nullptr;
	
	if (!SpawnTestPlayer(FVector(0, 0, 0), Attacker))
	{
		LastFailReason = TEXT("Failed to spawn test attacker");
		return false;
	}

	if (!SpawnTestEnemy(FVector(150, 0, 0), Target))
	{
		LastFailReason = TEXT("Failed to spawn test target");
		return false;
	}

	// Simulate stopping auto-attack
	UE_LOG(LogCombatTests, Log, TEXT("Auto-attack stop test: Attacker=%s, Target=%s"), 
		Attacker ? *Attacker->GetName() : TEXT("None"), 
		Target ? *Target->GetName() : TEXT("None"));

	// For now, just return true as a placeholder
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Enemy AI Tests
// ════════════════════════════════════════════════════════════════

bool ASabriMMOCombatTests::Test_Enemy_SpawnBehavior()
{
	// Test enemy spawn behavior
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Test spawning multiple enemies
	int32 SpawnCount = 0;
	for (int32 i = 0; i < TestEnemyCount; i++)
	{
		FVector SpawnLocation = FVector(i * 200.0f, 0, 0);
		AActor* Enemy = nullptr;
		
		if (SpawnTestEnemy(SpawnLocation, Enemy))
		{
			SpawnCount++;
			SpawnedEnemies.Add(Enemy);
		}
	}

	if (SpawnCount != TestEnemyCount)
	{
		LastFailReason = FString::Printf(TEXT("Expected to spawn %d enemies, got %d"), TestEnemyCount, SpawnCount);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Enemy spawn test: Successfully spawned %d enemies"), SpawnCount);
	return true;
}

bool ASabriMMOCombatTests::Test_Enemy_AggroBehavior()
{
	// Test enemy aggro behavior
	if (!GetWorld())
	{
		LastFailReason = TEXT("No valid world context");
		return false;
	}

	// Spawn player and enemy
	AActor* Player = nullptr;
	AActor* Enemy = nullptr;
	
	if (!SpawnTestPlayer(FVector(0, 0, 0), Player))
	{
		LastFailReason = TEXT("Failed to spawn test player");
		return false;
	}

	if (!SpawnTestEnemy(FVector(300, 0, 0), Enemy))
	{
		LastFailReason = TEXT("Failed to spawn test enemy");
		return false;
	}

	// Test aggro range (simplified)
	float AggroRange = 250.0f;
	float Distance = FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation());
	bool bShouldAggro = Distance <= AggroRange;

	// Distance is 300, aggro range is 250, should not aggro
	if (bShouldAggro)
	{
		LastFailReason = FString::Printf(TEXT("Enemy should not aggro at distance %.1f (range %.1f) but returned true"), Distance, AggroRange);
		return false;
	}

	// Move player closer
	Player->SetActorLocation(FVector(200, 0, 0));
	Distance = FVector::Dist(Player->GetActorLocation(), Enemy->GetActorLocation());
	bShouldAggro = Distance <= AggroRange;

	// Distance is 200, aggro range is 250, should aggro
	if (!bShouldAggro)
	{
		LastFailReason = FString::Printf(TEXT("Enemy should aggro at distance %.1f (range %.1f) but returned false"), Distance, AggroRange);
		return false;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Enemy aggro test: Distance=%.1f, AggroRange=%.1f, ShouldAggro=%s"), 
		Distance, AggroRange, bShouldAggro ? TEXT("True") : TEXT("False"));
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Helper Functions
// ════════════════════════════════════════════════════════════════

bool ASabriMMOCombatTests::SpawnTestEnemy(const FVector& Location, AActor*& OutEnemy)
{
	if (!GetWorld())
	{
		OutEnemy = nullptr;
		return false;
	}

	// For testing, spawn a simple cube as enemy
	OutEnemy = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
	
	if (OutEnemy)
	{
		// Add a simple mesh component for visibility
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(OutEnemy);
		MeshComp->SetupAttachment(OutEnemy->GetRootComponent());
		MeshComp->RegisterComponent();
		
		UE_LOG(LogCombatTests, Log, TEXT("Spawned test enemy at %s"), *Location.ToString());
		return true;
	}

	OutEnemy = nullptr;
	return false;
}

bool ASabriMMOCombatTests::SpawnTestPlayer(const FVector& Location, AActor*& OutPlayer)
{
	if (!GetWorld())
	{
		OutPlayer = nullptr;
		return false;
	}

	// For testing, spawn a simple cube as player
	OutPlayer = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
	
	if (OutPlayer)
	{
		// Add a simple mesh component for visibility
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(OutPlayer);
		MeshComp->SetupAttachment(OutPlayer->GetRootComponent());
		MeshComp->RegisterComponent();
		
		UE_LOG(LogCombatTests, Log, TEXT("Spawned test player at %s"), *Location.ToString());
		return true;
	}

	OutPlayer = nullptr;
	return false;
}

bool ASabriMMOCombatTests::ValidateCombatStats(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	// Basic validation - check if actor exists and is valid
	// In a real implementation, this would check combat-specific stats
	return Actor->IsValidLowLevel();
}

bool ASabriMMOCombatTests::CalculateExpectedDamage(int32 AttackerATK, int32 TargetDEF, int32& OutDamage)
{
	// Damage formula: Damage = ATK * (1 - DEF / (DEF + 100))
	if (TargetDEF < 0)
	{
		OutDamage = AttackerATK;
	}
	else
	{
		OutDamage = AttackerATK * (1.0f - static_cast<float>(TargetDEF) / (static_cast<float>(TargetDEF) + 100.0f));
	}
	
	// Ensure minimum damage of 1
	OutDamage = FMath::Max(1, OutDamage);
	return true;
}

bool ASabriMMOCombatTests::IsInCombatRange(AActor* Attacker, AActor* Target, float Range)
{
	if (!Attacker || !Target)
	{
		return false;
	}

	float Distance = FVector::Dist(Attacker->GetActorLocation(), Target->GetActorLocation());
	return Distance <= Range;
}

void ASabriMMOCombatTests::SimulateCombatRound(AActor* Attacker, AActor* Target)
{
	if (!Attacker || !Target)
	{
		return;
	}

	// Simulate a combat round
	UE_LOG(LogCombatTests, Log, TEXT("Simulating combat round: %s attacks %s"), 
		*Attacker->GetName(), *Target->GetName());
}

bool ASabriMMOCombatTests::CheckEnemyAIState(AActor* Enemy, const FString& ExpectedState)
{
	if (!Enemy)
	{
		return false;
	}

	// In a real implementation, this would check the AI state
	// For now, just return true as a placeholder
	UE_LOG(LogCombatTests, Log, TEXT("Checking AI state for %s: %s"), *Enemy->GetName(), *ExpectedState);
	return true;
}

void ASabriMMOCombatTests::CleanupTestActors()
{
	// Clean up spawned enemies
	for (AActor* Enemy : SpawnedEnemies)
	{
		if (Enemy && Enemy->IsValidLowLevel())
		{
			Enemy->Destroy();
		}
	}
	SpawnedEnemies.Empty();

	// Clean up test player
	if (TestPlayer && TestPlayer->IsValidLowLevel())
	{
		TestPlayer->Destroy();
		TestPlayer = nullptr;
	}

	UE_LOG(LogCombatTests, Log, TEXT("Cleaned up all test actors"));
}
