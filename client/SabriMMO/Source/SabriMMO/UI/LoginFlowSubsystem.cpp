#include "LoginFlowSubsystem.h"
#include "SLoginWidget.h"
#include "SServerSelectWidget.h"
#include "SCharacterSelectWidget.h"
#include "SCharacterCreateWidget.h"
#include "SLoadingOverlayWidget.h"
#include "MMOGameInstance.h"
#include "MMOHttpManager.h"
#include "Audio/AudioSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Images/SImage.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogLoginFlow, Log, All);

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

	UMMOGameInstance* DetectGI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());

	// If socket is connected AND we're NOT returning to char select, this is a game level — skip.
	if (DetectGI && DetectGI->IsSocketConnected() && !DetectGI->bReturningToCharSelect)
	{
		UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Game level detected (socket already connected). Skipping login UI."));
		return;
	}

	// Detect return-from-game path (ESC → Character Select)
	const bool bReturnToCharSelect = DetectGI && DetectGI->bReturningToCharSelect;
	if (bReturnToCharSelect)
	{
		UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Returning to character select from game."));
		DetectGI->bReturningToCharSelect = false;  // Consume the flag
	}
	else
	{
		UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Login level detected. Initializing login flow..."));
	}

	UMMOGameInstance* GI = GetGI();
	if (!GI) return;

	// Create all widgets and add to viewport
	UGameViewportClient* ViewportClient = InWorld.GetGameViewport();
	if (!ViewportClient) return;

	// --- Fullscreen Background (below all login widgets) ---
	// Deferred — viewport and textures are not ready during OnWorldBeginPlay in standalone.
	TryLoadBackgroundTexture(ViewportClient, 15);

	// --- Login screen BGM (RO Classic Title track 01) ---
	if (UAudioSubsystem* Audio = InWorld.GetSubsystem<UAudioSubsystem>())
	{
		Audio->PlayBgm(TEXT("/Game/SabriMMO/Audio/BGM/bgm_01"));
	}

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

	// Set input mode to UI only for the login level
	if (APlayerController* PC = InWorld.GetFirstPlayerController())
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}

	if (bReturnToCharSelect)
	{
		// Skip login + server select — go straight to character select.
		// Re-fetch character list (stats may have changed in-game).
		ShowLoadingOverlay(TEXT("Loading characters..."));
		UHttpManager::GetCharacters(GetWorld());
		// HandleCharacterListReceived will TransitionTo(CharacterSelect) when data arrives.
	}
	else
	{
		// Normal fresh login flow
		if (GI->bRememberUsername && !GI->RememberedUsername.IsEmpty())
		{
			LoginWidget->SetUsername(GI->RememberedUsername);
			LoginWidget->SetRememberUsername(true);
		}
		TransitionTo(ELoginFlowState::Login);
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
		World->GetTimerManager().ClearTimer(BackgroundRetryTimer);
	}

	// Remove widgets from viewport
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VC = World->GetGameViewport())
		{
			if (BackgroundViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(BackgroundViewportWidget.ToSharedRef());
			if (LoginViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(LoginViewportWidget.ToSharedRef());
			if (ServerSelectViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(ServerSelectViewportWidget.ToSharedRef());
			if (CharacterSelectViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(CharacterSelectViewportWidget.ToSharedRef());
			if (CharacterCreateViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(CharacterCreateViewportWidget.ToSharedRef());
			if (LoadingOverlayViewportWidget.IsValid()) VC->RemoveViewportWidgetContent(LoadingOverlayViewportWidget.ToSharedRef());
		}
	}

	BackgroundWidget.Reset();
	BackgroundBrush = FSlateBrush();
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

	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Transitioning to state: %d"), (int32)NewState);

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

void ULoginFlowSubsystem::OnRegisterSubmitted(const FString& Username, const FString& Email, const FString& Password)
{
	ShowLoadingOverlay(TEXT("Creating account..."));
	UHttpManager::RegisterUser(GetWorld(), Username, Email, Password);
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

	if (GI->IsSocketConnected())
	{
		// Socket already connected (returning from game via ESC menu).
		// Just re-emit player:join with the new character data.
		GI->EmitPlayerJoin();
	}
	else
	{
		// Fresh login — connect socket (OnConnected callback will emit player:join).
		GI->ConnectSocket();
	}

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
	// If socket is connected (returned from game via ESC), disconnect it
	UMMOGameInstance* GI = GetGI();
	if (GI && GI->IsSocketConnected())
	{
		GI->DisconnectSocket();
	}

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
	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Login successful. Fetching server list..."));
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
	UE_LOG(LogLoginFlow, Warning, TEXT("[LoginFlow] Login failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CurrentState == ELoginFlowState::Login && LoginWidget.IsValid())
	{
		LoginWidget->ShowError(ErrorMessage);
	}
}

void ULoginFlowSubsystem::HandleServerListReceived(const TArray<FServerInfo>& Servers)
{
	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Server list received: %d servers"), Servers.Num());
	HideLoadingOverlay();

	if (ServerSelectWidget.IsValid())
	{
		ServerSelectWidget->PopulateServerList(Servers);
	}

	TransitionTo(ELoginFlowState::ServerSelect);
}

void ULoginFlowSubsystem::HandleCharacterListReceived()
{
	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Character list received"));
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
	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Character created. Refreshing list..."));
	// Re-fetch characters to get the new one
	ShowLoadingOverlay(TEXT("Loading characters..."));
	UHttpManager::GetCharacters(GetWorld());
}

void ULoginFlowSubsystem::HandleCharacterCreateFailed(const FString& ErrorMessage)
{
	UE_LOG(LogLoginFlow, Warning, TEXT("[LoginFlow] Character creation failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CurrentState == ELoginFlowState::CharacterCreate && CharacterCreateWidget.IsValid())
	{
		CharacterCreateWidget->ShowError(ErrorMessage);
	}
}

void ULoginFlowSubsystem::HandleCharacterDeleteSuccess(const FString& CharacterName)
{
	UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Character deleted: %s. Refreshing list..."), *CharacterName);

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
	UE_LOG(LogLoginFlow, Warning, TEXT("[LoginFlow] Character deletion failed: %s"), *ErrorMessage);
	HideLoadingOverlay();

	if (CharacterSelectWidget.IsValid())
	{
		CharacterSelectWidget->ShowStatusMessage(ErrorMessage);
	}
}

// ============================================================
// Background
// ============================================================

void ULoginFlowSubsystem::TryLoadBackgroundTexture(UGameViewportClient* VC, int32 RetriesLeft)
{
	if (!VC) return;

	static const TCHAR* BackgroundPaths[] = {
		TEXT("/Game/SabriMMO/Textures/UI/T_LoginBackground.T_LoginBackground"),
		TEXT("/Game/SabriMMO/Textures/LoadingScreens/T_Loading_00.T_Loading_00"),
		TEXT("/Game/SabriMMO/Textures/LoadingScreens/T_Loading_01.T_Loading_01"),
	};

	for (const TCHAR* Path : BackgroundPaths)
	{
		UTexture2D* Tex = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, Path));
		if (Tex && Tex->GetSizeX() > 64)
		{
			BackgroundTexture = Tex;
			break;
		}
	}

	if (BackgroundTexture)
	{
		UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Background texture loaded %dx%d"),
			BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());
		CreateBackgroundWidget(VC);
	}
	else if (RetriesLeft > 0)
	{
		// Texture not ready yet — retry in 0.2s
		UE_LOG(LogLoginFlow, Log, TEXT("[LoginFlow] Background texture not ready, retrying (%d attempts left)"), RetriesLeft);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(BackgroundRetryTimer, [this, VC, RetriesLeft]()
			{
				TryLoadBackgroundTexture(VC, RetriesLeft - 1);
			}, 0.2f, false);
		}
	}
	else
	{
		// All retries exhausted — show fallback dark background
		UE_LOG(LogLoginFlow, Warning, TEXT("[LoginFlow] Background texture never loaded at full resolution, using dark fallback"));
		CreateBackgroundWidget(VC);
	}
}

void ULoginFlowSubsystem::CreateBackgroundWidget(UGameViewportClient* VC)
{
	if (!VC) return;

	if (BackgroundTexture)
	{
		BackgroundBrush.SetResourceObject(BackgroundTexture);
		BackgroundBrush.ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
		BackgroundBrush.Tiling = ESlateBrushTileType::NoTile;

		BackgroundWidget =
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFill)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(&BackgroundBrush)
			];
	}
	else
	{
		static const FLinearColor DarkBg(0.05f, 0.03f, 0.02f, 1.f);
		BackgroundWidget =
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
			.BorderBackgroundColor(DarkBg);
	}

	BackgroundViewportWidget = SNew(SWeakWidget).PossiblyNullContent(BackgroundWidget);
	VC->AddViewportWidgetContent(BackgroundViewportWidget.ToSharedRef(), BackgroundZOrder);
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
