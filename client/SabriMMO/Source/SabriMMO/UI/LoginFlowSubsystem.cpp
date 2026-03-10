#include "LoginFlowSubsystem.h"
#include "SLoginWidget.h"
#include "SServerSelectWidget.h"
#include "SCharacterSelectWidget.h"
#include "SCharacterCreateWidget.h"
#include "SLoadingOverlayWidget.h"
#include "MMOGameInstance.h"
#include "MMOHttpManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformMisc.h"

// ============================================================
// Lifecycle
// ============================================================

bool ULoginFlowSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	if (!World) return false;

	// Only create in game worlds (PIE or standalone)
	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
		return false;

	// The subsystem creates in all game worlds, but OnWorldBeginPlay will skip
	// initialization if the persistent socket is already connected (game level).
	return true;
}

void ULoginFlowSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// If the persistent socket is already connected, this is a game level — skip login UI.
	// (Login level is loaded before ConnectSocket() is called from OnPlayCharacter.)
	if (UMMOGameInstance* DetectGI = Cast<UMMOGameInstance>(InWorld.GetGameInstance()))
	{
		if (DetectGI->IsSocketConnected())
		{
			UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Game level detected (socket already connected). Skipping login UI."));
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Login level detected. Initializing login flow..."));

	UMMOGameInstance* GI = GetGI();
	if (!GI) return;

	// Create all widgets and add to viewport
	UGameViewportClient* ViewportClient = InWorld.GetGameViewport();
	if (!ViewportClient) return;

	// --- Login Widget ---
	LoginWidget = SNew(SLoginWidget).Subsystem(this);
	LoginViewportWidget = SNew(SWeakWidget).PossiblyNullContent(LoginWidget);
	ViewportClient->AddViewportWidgetContent(LoginViewportWidget.ToSharedRef(), LoginWidgetZOrder);

	// --- Server Select Widget ---
	ServerSelectWidget = SNew(SServerSelectWidget).Subsystem(this);
	ServerSelectViewportWidget = SNew(SWeakWidget).PossiblyNullContent(ServerSelectWidget);
	ViewportClient->AddViewportWidgetContent(ServerSelectViewportWidget.ToSharedRef(), LoginWidgetZOrder);

	// --- Character Select Widget ---
	CharacterSelectWidget = SNew(SCharacterSelectWidget).Subsystem(this);
	CharacterSelectViewportWidget = SNew(SWeakWidget).PossiblyNullContent(CharacterSelectWidget);
	ViewportClient->AddViewportWidgetContent(CharacterSelectViewportWidget.ToSharedRef(), LoginWidgetZOrder);

	// --- Character Create Widget ---
	CharacterCreateWidget = SNew(SCharacterCreateWidget).Subsystem(this);
	CharacterCreateViewportWidget = SNew(SWeakWidget).PossiblyNullContent(CharacterCreateWidget);
	ViewportClient->AddViewportWidgetContent(CharacterCreateViewportWidget.ToSharedRef(), LoginWidgetZOrder);

	// --- Loading Overlay (highest Z-order) ---
	LoadingOverlayWidget = SNew(SLoadingOverlayWidget);
	LoadingOverlayViewportWidget = SNew(SWeakWidget).PossiblyNullContent(LoadingOverlayWidget);
	ViewportClient->AddViewportWidgetContent(LoadingOverlayViewportWidget.ToSharedRef(), OverlayWidgetZOrder);

	bWidgetsCreated = true;

	// Bind GameInstance delegates
	GI->OnLoginSuccess.AddDynamic(this, &ULoginFlowSubsystem::HandleLoginSuccess);
	GI->OnLoginFailedWithReason.AddDynamic(this, &ULoginFlowSubsystem::HandleLoginFailedWithReason);
	GI->OnServerListReceived.AddDynamic(this, &ULoginFlowSubsystem::HandleServerListReceived);
	GI->OnCharacterListReceived.AddDynamic(this, &ULoginFlowSubsystem::HandleCharacterListReceived);
	GI->OnCharacterCreated.AddDynamic(this, &ULoginFlowSubsystem::HandleCharacterCreated);
	GI->OnCharacterCreateFailed.AddDynamic(this, &ULoginFlowSubsystem::HandleCharacterCreateFailed);
	GI->OnCharacterDeleteSuccess.AddDynamic(this, &ULoginFlowSubsystem::HandleCharacterDeleteSuccess);
	GI->OnCharacterDeleteFailed.AddDynamic(this, &ULoginFlowSubsystem::HandleCharacterDeleteFailed);

	// Pre-fill remembered username
	if (GI->bRememberUsername && !GI->RememberedUsername.IsEmpty())
	{
		LoginWidget->SetUsername(GI->RememberedUsername);
		LoginWidget->SetRememberUsername(true);
	}

	// Start at login screen
	TransitionTo(ELoginFlowState::Login);

	// Set input mode to UI only for the login level
	if (APlayerController* PC = InWorld.GetFirstPlayerController())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
}

void ULoginFlowSubsystem::Deinitialize()
{
	if (UMMOGameInstance* GI = GetGI())
	{
		GI->OnLoginSuccess.RemoveDynamic(this, &ULoginFlowSubsystem::HandleLoginSuccess);
		GI->OnLoginFailedWithReason.RemoveDynamic(this, &ULoginFlowSubsystem::HandleLoginFailedWithReason);
		GI->OnServerListReceived.RemoveDynamic(this, &ULoginFlowSubsystem::HandleServerListReceived);
		GI->OnCharacterListReceived.RemoveDynamic(this, &ULoginFlowSubsystem::HandleCharacterListReceived);
		GI->OnCharacterCreated.RemoveDynamic(this, &ULoginFlowSubsystem::HandleCharacterCreated);
		GI->OnCharacterCreateFailed.RemoveDynamic(this, &ULoginFlowSubsystem::HandleCharacterCreateFailed);
		GI->OnCharacterDeleteSuccess.RemoveDynamic(this, &ULoginFlowSubsystem::HandleCharacterDeleteSuccess);
		GI->OnCharacterDeleteFailed.RemoveDynamic(this, &ULoginFlowSubsystem::HandleCharacterDeleteFailed);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EnterWorldTimer);
	}

	// Remove widgets from viewport
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (LoginViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(LoginViewportWidget.ToSharedRef());
			if (ServerSelectViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(ServerSelectViewportWidget.ToSharedRef());
			if (CharacterSelectViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(CharacterSelectViewportWidget.ToSharedRef());
			if (CharacterCreateViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(CharacterCreateViewportWidget.ToSharedRef());
			if (LoadingOverlayViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(LoadingOverlayViewportWidget.ToSharedRef());
		}
	}

	LoginWidget.Reset();
	ServerSelectWidget.Reset();
	CharacterSelectWidget.Reset();
	CharacterCreateWidget.Reset();
	LoadingOverlayWidget.Reset();

	Super::Deinitialize();
}

// ============================================================
// State Machine
// ============================================================

void ULoginFlowSubsystem::TransitionTo(ELoginFlowState NewState)
{
	if (!bWidgetsCreated) return;

	HideAllWidgets();
	CurrentState = NewState;

	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Transitioning to state: %d"), (int32)NewState);

	switch (NewState)
	{
	case ELoginFlowState::Login:
		LoginWidget->SetVisibility(EVisibility::Visible);
		LoginWidget->ClearError();
		LoginWidget->FocusAppropriateField();
		break;

	case ELoginFlowState::ServerSelect:
		ServerSelectWidget->SetVisibility(EVisibility::Visible);
		// Server list data will be populated by HandleServerListReceived callback
		break;

	case ELoginFlowState::CharacterSelect:
		CharacterSelectWidget->SetVisibility(EVisibility::Visible);
		// Character list data will be populated by HandleCharacterListReceived callback
		break;

	case ELoginFlowState::CharacterCreate:
		CharacterCreateWidget->SetVisibility(EVisibility::Visible);
		CharacterCreateWidget->ClearError();
		break;

	case ELoginFlowState::EnteringWorld:
		ShowLoadingOverlay(TEXT("Entering world..."));
		// Brief delay then open level
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(EnterWorldTimer, [this]()
			{
				HideLoadingOverlay();
				// Restore input mode for gameplay before transitioning
				if (UWorld* W = GetWorld())
				{
					if (APlayerController* PC = W->GetFirstPlayerController())
					{
						FInputModeGameAndUI InputMode;
						InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
						InputMode.SetHideCursorDuringCapture(false);
						PC->SetInputMode(InputMode);
					}
				}
				{
					UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance());
					FString LevelName = TEXT("L_PrtSouth");
					if (GI && !GI->PendingLevelName.IsEmpty())
					{
						LevelName = GI->PendingLevelName;
					}
					UGameplayStatics::OpenLevel(GetWorld(), *LevelName);
				}
			}, 0.5f, false);
		}
		break;
	}
}

void ULoginFlowSubsystem::HideAllWidgets()
{
	if (LoginWidget.IsValid()) LoginWidget->SetVisibility(EVisibility::Collapsed);
	if (ServerSelectWidget.IsValid()) ServerSelectWidget->SetVisibility(EVisibility::Collapsed);
	if (CharacterSelectWidget.IsValid()) CharacterSelectWidget->SetVisibility(EVisibility::Collapsed);
	if (CharacterCreateWidget.IsValid()) CharacterCreateWidget->SetVisibility(EVisibility::Collapsed);
	HideLoadingOverlay();
}

void ULoginFlowSubsystem::ShowLoadingOverlay(const FString& StatusText)
{
	if (LoadingOverlayWidget.IsValid())
	{
		LoadingOverlayWidget->SetStatusText(StatusText);
		LoadingOverlayWidget->Show();
	}
}

void ULoginFlowSubsystem::HideLoadingOverlay()
{
	if (LoadingOverlayWidget.IsValid())
	{
		LoadingOverlayWidget->Hide();
	}
}

// ============================================================
// Widget Callbacks
// ============================================================

void ULoginFlowSubsystem::OnLoginSubmitted(const FString& Username, const FString& Password, bool bRememberUsername)
{
	UMMOGameInstance* GI = GetGI();
	if (!GI) return;

	// Save remember-username preference
	GI->bRememberUsername = bRememberUsername;
	if (bRememberUsername)
	{
		GI->RememberedUsername = Username;
	}

	ShowLoadingOverlay(TEXT("Authenticating..."));
	UHttpManager::LoginUser(GetWorld(), Username, Password);
}

void ULoginFlowSubsystem::OnExitRequested()
{
	FPlatformMisc::RequestExit(false);
}

void ULoginFlowSubsystem::OnServerSelected(const FServerInfo& Server)
{
	UMMOGameInstance* GI = GetGI();
	if (!GI) return;

	GI->SelectServer(Server);

	ShowLoadingOverlay(TEXT("Loading characters..."));
	UHttpManager::GetCharacters(GetWorld());
}

void ULoginFlowSubsystem::OnBackToLogin()
{
	UMMOGameInstance* GI = GetGI();
	if (GI) GI->Logout();
	TransitionTo(ELoginFlowState::Login);
}

void ULoginFlowSubsystem::OnPlayCharacter(const FCharacterData& Character)
{
	UMMOGameInstance* GI = GetGI();
	if (!GI) return;

	GI->SelectCharacter(Character.CharacterId);
	GI->SaveRememberedUsername();

	// Set the correct level/zone from the character's saved data
	// so we load directly into the right map instead of defaulting to L_PrtSouth
	GI->PendingLevelName = Character.LevelName;
	GI->PendingZoneName = Character.ZoneName;
	GI->CurrentZoneName = Character.ZoneName;
	GI->PendingSpawnLocation = FVector(Character.X, Character.Y, Character.Z);
	GI->bIsZoneTransitioning = true;

	// Connect persistent socket before level transition — survives OpenLevel
	GI->ConnectSocket();

	TransitionTo(ELoginFlowState::EnteringWorld);
}

void ULoginFlowSubsystem::OnDeleteCharacterConfirmed(int32 CharacterId, const FString& Password)
{
	ShowLoadingOverlay(TEXT("Deleting character..."));
	UHttpManager::DeleteCharacter(GetWorld(), CharacterId, Password);
}

void ULoginFlowSubsystem::OnCreateCharacterRequested()
{
	TransitionTo(ELoginFlowState::CharacterCreate);
}

void ULoginFlowSubsystem::OnBackToServerSelect()
{
	// Re-fetch servers for fresh population data
	ShowLoadingOverlay(TEXT("Fetching server list..."));
	UHttpManager::GetServerList(GetWorld());
}

void ULoginFlowSubsystem::OnCreateCharacterSubmitted(const FString& Name, const FString& Class,
	int32 HairStyle, int32 HairColor, const FString& Gender)
{
	ShowLoadingOverlay(TEXT("Creating character..."));
	UHttpManager::CreateCharacter(GetWorld(), Name, Class, HairStyle, HairColor, Gender);
}

void ULoginFlowSubsystem::OnCreateCharacterCancelled()
{
	TransitionTo(ELoginFlowState::CharacterSelect);
	// Repopulate with existing data
	UMMOGameInstance* GI = GetGI();
	if (GI && CharacterSelectWidget.IsValid())
	{
		CharacterSelectWidget->PopulateCharacters(GI->CharacterList);
	}
}

// ============================================================
// GameInstance Delegate Handlers
// ============================================================

void ULoginFlowSubsystem::HandleLoginSuccess()
{
	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Login successful. Fetching server list..."));
	HideLoadingOverlay();

	// Save remembered username after successful login
	if (UMMOGameInstance* GI = GetGI())
	{
		GI->SaveRememberedUsername();
	}

	// Fetch server list
	ShowLoadingOverlay(TEXT("Fetching server list..."));
	UHttpManager::GetServerList(GetWorld());
}

void ULoginFlowSubsystem::HandleLoginFailedWithReason(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("[LoginFlow] Login failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CurrentState == ELoginFlowState::Login && LoginWidget.IsValid())
	{
		LoginWidget->ShowError(ErrorMessage);
	}
}

void ULoginFlowSubsystem::HandleServerListReceived(const TArray<FServerInfo>& Servers)
{
	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Server list received: %d servers"), Servers.Num());
	HideLoadingOverlay();

	if (ServerSelectWidget.IsValid())
	{
		ServerSelectWidget->PopulateServerList(Servers);
	}

	TransitionTo(ELoginFlowState::ServerSelect);
}

void ULoginFlowSubsystem::HandleCharacterListReceived()
{
	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Character list received"));
	HideLoadingOverlay();

	UMMOGameInstance* GI = GetGI();
	if (GI && CharacterSelectWidget.IsValid())
	{
		CharacterSelectWidget->PopulateCharacters(GI->CharacterList);
	}

	TransitionTo(ELoginFlowState::CharacterSelect);
}

void ULoginFlowSubsystem::HandleCharacterCreated()
{
	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Character created. Refreshing list..."));
	// Re-fetch characters to get the new one
	ShowLoadingOverlay(TEXT("Loading characters..."));
	UHttpManager::GetCharacters(GetWorld());
}

void ULoginFlowSubsystem::HandleCharacterCreateFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("[LoginFlow] Character creation failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CurrentState == ELoginFlowState::CharacterCreate && CharacterCreateWidget.IsValid())
	{
		CharacterCreateWidget->ShowError(ErrorMessage);
	}
}

void ULoginFlowSubsystem::HandleCharacterDeleteSuccess(const FString& CharacterName)
{
	UE_LOG(LogTemp, Log, TEXT("[LoginFlow] Character deleted: %s. Refreshing list..."), *CharacterName);

	// Close the delete confirmation overlay
	if (CharacterSelectWidget.IsValid())
	{
		CharacterSelectWidget->HideDeleteConfirmation();
	}

	// Re-fetch characters
	ShowLoadingOverlay(TEXT("Loading characters..."));
	UHttpManager::GetCharacters(GetWorld());
}

void ULoginFlowSubsystem::HandleCharacterDeleteFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("[LoginFlow] Character deletion failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CharacterSelectWidget.IsValid())
	{
		CharacterSelectWidget->ShowStatusMessage(ErrorMessage);
	}
}

// ============================================================
// Helper
// ============================================================

UMMOGameInstance* ULoginFlowSubsystem::GetGI() const
{
	if (UWorld* World = GetWorld())
	{
		return Cast<UMMOGameInstance>(World->GetGameInstance());
	}
	return nullptr;
}
