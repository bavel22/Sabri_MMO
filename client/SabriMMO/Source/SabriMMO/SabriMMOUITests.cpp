// SabriMMOUITests.cpp - Automated UI Test Runner Implementation

#include "SabriMMOUITests.h"
#include "MMOGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogUITests, Log, All);

// ════════════════════════════════════════════════════════════════
//  Construction
// ════════════════════════════════════════════════════════════════

ASabriMMOUITests::ASabriMMOUITests()
{
	PrimaryActorTick.bCanEverTick = false;
}

// ════════════════════════════════════════════════════════════════
//  BeginPlay → delayed test start
// ════════════════════════════════════════════════════════════════

void ASabriMMOUITests::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogUITests, Log, TEXT("UI Test Runner placed. Tests will start in %.1f seconds..."), InitialDelay);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan,
			FString::Printf(TEXT("UI Test Runner: Starting in %.0fs..."), InitialDelay));
	}

	GetWorldTimerManager().SetTimer(
		TestTimerHandle, this, &ASabriMMOUITests::StartTests, InitialDelay, false);
}

// ════════════════════════════════════════════════════════════════
//  Test Runner
// ════════════════════════════════════════════════════════════════

void ASabriMMOUITests::StartTests()
{
	PrintHeader();

	// Wait for player to spawn (retry up to 10 times)
	int32 RetryCount = 0;
	while (RetryCount < 10)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC && PC->GetPawn())
		{
			PlayerPawn = PC->GetPawn();
			UE_LOG(LogUITests, Log, TEXT("Player pawn found: %s"), *PlayerPawn->GetClass()->GetName());
			break;
		}
		
		UE_LOG(LogUITests, Warning, TEXT("Waiting for player to spawn... retry %d/10"), RetryCount + 1);
		RetryCount++;
		
		// Wait a frame and try again using a simple delay
		if (RetryCount < 10)
		{
			FTimerHandle TempHandle;
			GetWorldTimerManager().SetTimer(TempHandle, [this]() {}, 0.5f, false);
			// Wait for the timer to complete
			float WaitTime = 0.0f;
			while (WaitTime < 0.5f)
			{
				WaitTime += GetWorld()->GetDeltaSeconds();
			}
		}
	}

	// If still no player, try to spawn one manually
	if (!PlayerPawn)
	{
		UE_LOG(LogUITests, Warning, TEXT("Player not found, attempting manual spawn..."));
		
		// Try to get the Game Mode and spawn player
		AGameModeBase* GameMode = GetWorld()->GetAuthGameMode();
		if (GameMode)
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
			if (PC)
			{
				// Try to spawn the player using the Game Mode
				APawn* SpawnedPawn = GameMode->SpawnDefaultPawnFor(PC, nullptr);
				if (SpawnedPawn)
				{
					PC->Possess(SpawnedPawn);
					PlayerPawn = SpawnedPawn;
					UE_LOG(LogUITests, Log, TEXT("Manually spawned player: %s"), *PlayerPawn->GetClass()->GetName());
				}
			}
		}
	}

	if (!PlayerPawn)
	{
		UE_LOG(LogUITests, Error, TEXT("FAILED: Player pawn never spawned!"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, TEXT("UI TESTS FAILED: No player character spawned!"));
		}
		return;
	}

	// Cache HUD Manager if player pawn is valid
	if (PlayerPawn)
	{
		HUDManagerComp = FindComponentByClassName(PlayerPawn, TEXT("AC_HUDManager"));
	}

	CurrentTestIndex = 0;
	PassCount = 0;
	FailCount = 0;
	RunNextTest();
}

void ASabriMMOUITests::RunNextTest()
{
	bool bResult = false;
	LastFailReason.Reset();

	switch (CurrentTestIndex)
	{
	case 0:
		bResult = Test_GameInstanceValid();
		LogResult(TEXT("GameInstance Valid"), bResult, LastFailReason);
		break;

	case 1:
		bResult = Test_PlayerCharacterValid();
		LogResult(TEXT("PlayerCharacter Valid"), bResult, LastFailReason);
		break;

	case 2:
		bResult = Test_HUDManagerFound();
		LogResult(TEXT("HUDManager Found"), bResult, LastFailReason);
		break;

	case 3:
		bResult = Test_InventoryToggle();
		LogResult(TEXT("Inventory Toggle"), bResult, LastFailReason);
		break;

	case 4:
		bResult = Test_ZuzucoinUpdate();
		LogResult(TEXT("Zuzucoin Update"), bResult, LastFailReason);
		break;

	default:
		PrintSummary();
		return;
	}

	CurrentTestIndex++;

	// Schedule next test with delay (allows UI to update between tests)
	GetWorldTimerManager().SetTimer(
		TestTimerHandle, this, &ASabriMMOUITests::RunNextTest, DelayBetweenTests, false);
}

// ════════════════════════════════════════════════════════════════
//  Logging
// ════════════════════════════════════════════════════════════════

void ASabriMMOUITests::PrintHeader()
{
	const FString Header = TEXT("========================================");
	const FString Title  = TEXT("=== SABRI_MMO UI TEST SUITE ===");

	UE_LOG(LogUITests, Log, TEXT("%s"), *Header);
	UE_LOG(LogUITests, Log, TEXT("%s"), *Title);
	UE_LOG(LogUITests, Log, TEXT("%s"), *Header);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Header);
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Title);
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Header);
	}
}

void ASabriMMOUITests::LogResult(const FString& TestName, bool bPassed, const FString& Details)
{
	if (bPassed)
	{
		PassCount++;
		const FString Msg = FString::Printf(TEXT("PASS: %s"), *TestName);
		UE_LOG(LogUITests, Log, TEXT("[PASS] %s"), *TestName);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Green, Msg);
		}
	}
	else
	{
		FailCount++;
		const FString Msg = Details.IsEmpty()
			? FString::Printf(TEXT("FAIL: %s"), *TestName)
			: FString::Printf(TEXT("FAIL: %s - %s"), *TestName, *Details);
		UE_LOG(LogUITests, Warning, TEXT("[FAIL] %s %s"), *TestName, *Details);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, Msg);
		}
	}
}

void ASabriMMOUITests::PrintSummary()
{
	const int32 Total = PassCount + FailCount;
	const FString Sep = TEXT("========================================");
	const FString Summary = FString::Printf(TEXT("Results: %d/%d passed, %d failed"), PassCount, Total, FailCount);
	const FString Verdict = (FailCount == 0) ? TEXT("ALL TESTS PASSED") : TEXT("SOME TESTS FAILED");

	UE_LOG(LogUITests, Log, TEXT("%s"), *Sep);
	UE_LOG(LogUITests, Log, TEXT("%s"), *Summary);
	UE_LOG(LogUITests, Log, TEXT("%s"), *Verdict);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, Sep);
		GEngine->AddOnScreenDebugMessage(-1, 30.f,
			FailCount == 0 ? FColor::Green : FColor::Red, Summary);
		GEngine->AddOnScreenDebugMessage(-1, 30.f,
			FailCount == 0 ? FColor::Green : FColor::Red, Verdict);
	}
}

// ════════════════════════════════════════════════════════════════
//  Test 1: GameInstance Valid
// ════════════════════════════════════════════════════════════════

bool ASabriMMOUITests::Test_GameInstanceValid()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI)
	{
		LastFailReason = TEXT("GameInstance is null");
		return false;
	}

	UMMOGameInstance* MMOGI = Cast<UMMOGameInstance>(GI);
	if (!MMOGI)
	{
		LastFailReason = TEXT("GameInstance is not UMMOGameInstance");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Test 2: Player Character Valid
// ════════════════════════════════════════════════════════════════

bool ASabriMMOUITests::Test_PlayerCharacterValid()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		LastFailReason = TEXT("No PlayerController");
		return false;
	}

	if (!PlayerPawn)
	{
		LastFailReason = TEXT("PlayerController has no Pawn");
		return false;
	}

	const FString ClassName = PlayerPawn->GetClass()->GetName();
	if (!ClassName.Contains(TEXT("MMOCharacter")))
	{
		LastFailReason = FString::Printf(TEXT("Pawn class is '%s', expected BP_MMOCharacter"), *ClassName);
		return false;
	}

	UE_LOG(LogUITests, Log, TEXT("  Player class: %s"), *ClassName);
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Test 3: HUD Manager Found
// ════════════════════════════════════════════════════════════════

bool ASabriMMOUITests::Test_HUDManagerFound()
{
	if (!PlayerPawn)
	{
		LastFailReason = TEXT("No PlayerPawn cached");
		return false;
	}

	if (!HUDManagerComp)
	{
		FString CompList;
		for (UActorComponent* Comp : PlayerPawn->GetComponents())
		{
			CompList += Comp->GetClass()->GetName() + TEXT(", ");
		}
		LastFailReason = FString::Printf(TEXT("AC_HUDManager not found. Components: [%s]"), *CompList);
		return false;
	}

	UFunction* ToggleFunc = HUDManagerComp->FindFunction(FName("ToggleInventory"));
	UFunction* UpdateFunc = HUDManagerComp->FindFunction(FName("UpdateZuzucoinEverywhere"));

	if (!ToggleFunc)
	{
		LastFailReason = TEXT("ToggleInventory function not found on HUDManager");
		return false;
	}

	if (!UpdateFunc)
	{
		LastFailReason = TEXT("UpdateZuzucoinEverywhere function not found on HUDManager");
		return false;
	}

	UE_LOG(LogUITests, Log, TEXT("  HUDManager class: %s"), *HUDManagerComp->GetClass()->GetName());
	return true;
}

// ════════════════════════════════════════════════════════════════
//  Test 4: Inventory Toggle
// ════════════════════════════════════════════════════════════════

bool ASabriMMOUITests::Test_InventoryToggle()
{
	if (!HUDManagerComp)
	{
		LastFailReason = TEXT("HUDManager not available");
		return false;
	}

	// Read initial state of bIsInventoryOpen
	const bool bInitialState = GetBoolProperty(HUDManagerComp, TEXT("bIsInventoryOpen"));
	UE_LOG(LogUITests, Log, TEXT("  Initial bIsInventoryOpen: %s"), bInitialState ? TEXT("true") : TEXT("false"));

	// Call ToggleInventory to OPEN
	if (!CallBlueprintFunction(HUDManagerComp, TEXT("ToggleInventory")))
	{
		LastFailReason = TEXT("ToggleInventory function call failed");
		return false;
	}

	// Read state after first toggle
	const bool bAfterOpen = GetBoolProperty(HUDManagerComp, TEXT("bIsInventoryOpen"));
	UE_LOG(LogUITests, Log, TEXT("  After open toggle bIsInventoryOpen: %s"), bAfterOpen ? TEXT("true") : TEXT("false"));

	if (bAfterOpen == bInitialState)
	{
		LastFailReason = TEXT("bIsInventoryOpen did not change after ToggleInventory");
		return false;
	}

	// Check InventoryWindowRef is valid after opening
	UObject* InvWindowRef = GetObjectProperty(HUDManagerComp, TEXT("InventoryWindowRef"));
	if (!InvWindowRef)
	{
		UE_LOG(LogUITests, Warning, TEXT("  InventoryWindowRef is null after toggle (may need server connection)"));
	}
	else
	{
		UE_LOG(LogUITests, Log, TEXT("  InventoryWindowRef valid: %s"), *InvWindowRef->GetClass()->GetName());
	}

	// Call ToggleInventory to CLOSE
	CallBlueprintFunction(HUDManagerComp, TEXT("ToggleInventory"));
	const bool bAfterClose = GetBoolProperty(HUDManagerComp, TEXT("bIsInventoryOpen"));

	if (bAfterClose != bInitialState)
	{
		LastFailReason = TEXT("bIsInventoryOpen did not revert after second toggle");
		return false;
	}

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Test 5: Zuzucoin Update
// ════════════════════════════════════════════════════════════════

bool ASabriMMOUITests::Test_ZuzucoinUpdate()
{
	if (!HUDManagerComp)
	{
		LastFailReason = TEXT("HUDManager not available");
		return false;
	}

	// Read initial PlayerZuzucoin
	const int32 InitialZuzucoin = GetIntProperty(HUDManagerComp, TEXT("PlayerZuzucoin"));
	UE_LOG(LogUITests, Log, TEXT("  Initial PlayerZuzucoin: %d"), InitialZuzucoin);

	// Call UpdateZuzucoinEverywhere with test value 4000
	UFunction* UpdateFunc = HUDManagerComp->FindFunction(FName("UpdateZuzucoinEverywhere"));
	if (!UpdateFunc)
	{
		LastFailReason = TEXT("UpdateZuzucoinEverywhere not found");
		return false;
	}

	// Prepare parameter struct matching the function signature: void UpdateZuzucoinEverywhere(int32 NewAmount)
	struct FZuzucoinParams
	{
		int32 NewAmount;
	};
	FZuzucoinParams Params;
	Params.NewAmount = 4000;

	HUDManagerComp->ProcessEvent(UpdateFunc, &Params);

	// Read updated PlayerZuzucoin
	const int32 UpdatedZuzucoin = GetIntProperty(HUDManagerComp, TEXT("PlayerZuzucoin"));
	UE_LOG(LogUITests, Log, TEXT("  After update PlayerZuzucoin: %d (expected 4000)"), UpdatedZuzucoin);

	if (UpdatedZuzucoin != 4000)
	{
		LastFailReason = FString::Printf(TEXT("Expected 4000, got %d"), UpdatedZuzucoin);
		return false;
	}

	// Restore original value
	Params.NewAmount = InitialZuzucoin;
	HUDManagerComp->ProcessEvent(UpdateFunc, &Params);

	return true;
}

// ════════════════════════════════════════════════════════════════
//  Reflection Helpers
// ════════════════════════════════════════════════════════════════

UActorComponent* ASabriMMOUITests::FindComponentByClassName(AActor* Actor, const FString& ClassName) const
{
	if (!Actor) return nullptr;

	for (UActorComponent* Comp : Actor->GetComponents())
	{
		if (Comp && Comp->GetClass()->GetName().Contains(ClassName))
		{
			return Comp;
		}
	}
	return nullptr;
}

bool ASabriMMOUITests::CallBlueprintFunction(UObject* Object, const FString& FunctionName, void* Params) const
{
	if (!Object) return false;

	UFunction* Func = Object->FindFunction(FName(*FunctionName));
	if (!Func)
	{
		UE_LOG(LogUITests, Warning, TEXT("  Function '%s' not found on %s"), *FunctionName, *Object->GetClass()->GetName());
		return false;
	}

	Object->ProcessEvent(Func, Params);
	return true;
}

int32 ASabriMMOUITests::GetIntProperty(UObject* Object, const FString& PropertyName) const
{
	if (!Object) return 0;

	FProperty* Prop = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (!Prop) return 0;

	const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Object);
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
	{
		return IntProp->GetPropertyValue(ValuePtr);
	}
	return 0;
}

UObject* ASabriMMOUITests::GetObjectProperty(UObject* Object, const FString& PropertyName) const
{
	if (!Object) return nullptr;

	FProperty* Prop = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (!Prop) return nullptr;

	if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
	{
		return ObjProp->GetObjectPropertyValue(Prop->ContainerPtrToValuePtr<void>(Object));
	}
	return nullptr;
}

bool ASabriMMOUITests::GetBoolProperty(UObject* Object, const FString& PropertyName) const
{
	if (!Object) return false;

	FProperty* Prop = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (!Prop) return false;

	const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Object);
	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
	{
		return BoolProp->GetPropertyValue(ValuePtr);
	}
	return false;
}
