#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "SocketIONative.h"
#include "SocketIOClient.h"
#include "SIOJsonObject.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMOSocket, Log, All);

void UMMOGameInstance::Init()
{
    Super::Init();
    LoadRememberedUsername();
    LoadGameOptions();

    // Create the event router (persists for the lifetime of the game instance)
    EventRouter = NewObject<USocketEventRouter>(this);
}

void UMMOGameInstance::SetAuthData(const FString& InToken, const FString& InUsername, int32 InUserId)
{
    AuthToken = InToken;
    Username = InUsername;
    UserId = InUserId;
    bIsLoggedIn = true;

    UE_LOG(LogTemp, Log, TEXT("Auth data set for user: %s (ID: %d)"), *Username, UserId);
    OnLoginSuccess.Broadcast();
}

void UMMOGameInstance::SetCharacterList(const TArray<FCharacterData>& Characters)
{
    CharacterList = Characters;
    UE_LOG(LogTemp, Log, TEXT("Character list updated with %d characters"), Characters.Num());
    OnCharacterListReceived.Broadcast();
}

void UMMOGameInstance::SelectCharacter(int32 CharacterId)
{
    SelectedCharacterId = CharacterId;

    for (const FCharacterData& Character : CharacterList)
    {
        if (Character.CharacterId == CharacterId)
        {
            SelectedCharacter = Character;
            UE_LOG(LogTemp, Log, TEXT("Selected character: %s (ID: %d)"), *Character.Name, CharacterId);
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Character ID %d not found in character list"), CharacterId);
}

FCharacterData UMMOGameInstance::GetSelectedCharacter() const
{
    return SelectedCharacter;
}

bool UMMOGameInstance::IsAuthenticated() const
{
    return bIsLoggedIn && !AuthToken.IsEmpty();
}

FString UMMOGameInstance::GetAuthHeader() const
{
    if (AuthToken.IsEmpty())
    {
        return TEXT("");
    }
    return TEXT("Bearer ") + AuthToken;
}

void UMMOGameInstance::ClearAuthData()
{
    AuthToken.Empty();
    Username.Empty();
    UserId = 0;
    bIsLoggedIn = false;
    CharacterList.Empty();
    SelectedCharacterId = 0;
    SelectedCharacter = FCharacterData();

    UE_LOG(LogTemp, Log, TEXT("Auth data cleared"));
}

// ---- Server Selection ----

void UMMOGameInstance::SetServerList(const TArray<FServerInfo>& Servers)
{
    ServerList = Servers;
    UE_LOG(LogTemp, Log, TEXT("Server list updated with %d servers"), Servers.Num());
    OnServerListReceived.Broadcast(Servers);
}

void UMMOGameInstance::SelectServer(const FServerInfo& Server)
{
    SelectedServer = Server;
    ServerBaseUrl = FString::Printf(TEXT("http://%s:%d"), *Server.Host, Server.Port);
    UE_LOG(LogTemp, Log, TEXT("Selected server: %s (%s)"), *Server.Name, *ServerBaseUrl);
}

FString UMMOGameInstance::GetServerSocketUrl() const
{
    if (!SelectedServer.Host.IsEmpty())
    {
        return FString::Printf(TEXT("http://%s:%d"), *SelectedServer.Host, SelectedServer.Port);
    }
    return ServerBaseUrl;
}

// ---- Remember Username ----

// ============================================================
// Persistence via UE5 SaveGame system (reliable in all modes)
// Saves to: {Saved}/SaveGames/SabriMMO_Options.sav
// ============================================================

static const FString OptionsSaveSlot = TEXT("SabriMMO_Options");

void UMMOGameInstance::SaveRememberedUsername()
{
    SaveGameOptions(); // All settings saved together
}

void UMMOGameInstance::LoadRememberedUsername()
{
    // No-op — LoadGameOptions() in Init() loads everything including username
}

// ---- Game Options ----

void UMMOGameInstance::SaveGameOptions()
{
    UOptionsSaveGame* Save = NewObject<UOptionsSaveGame>();
    // Display
    Save->bShowFPS = bOptionShowFPS;
    Save->bSkillEffects = bOptionSkillEffects;
    Save->bShowMissText = bOptionShowMissText;
    Save->fBrightness = fOptionBrightness;
    // Interface
    Save->bShowDamageNumbers = bOptionShowDamageNumbers;
    Save->bShowEnemyHPBars = bOptionShowEnemyHPBars;
    Save->bShowPlayerNames = bOptionShowPlayerNames;
    Save->bShowEnemyNames = bOptionShowEnemyNames;
    // Camera
    Save->fCameraSensitivity = fOptionCameraSensitivity;
    Save->fCameraZoomSpeed = fOptionCameraZoomSpeed;
    // Gameplay
    Save->bNoCtrl = bOptionNoCtrl;
    Save->bNoShift = bOptionNoShift;
    Save->bAutoDeclineTrades = bOptionAutoDeclineTrades;
    Save->bAutoDeclineParty = bOptionAutoDeclineParty;
    // Interface (extended)
    Save->bShowCastBars = bOptionShowCastBars;
    Save->bShowNPCNames = bOptionShowNPCNames;
    Save->bShowChatTimestamps = bOptionShowChatTimestamps;
    Save->fChatOpacity = fOptionChatOpacity;
    Save->fDamageNumberScale = fOptionDamageNumberScale;
    // Drop Sounds
    Save->bDropSoundMvp    = bOptionDropSoundMvp;
    Save->bDropSoundCard   = bOptionDropSoundCard;
    Save->bDropSoundEquip  = bOptionDropSoundEquip;
    Save->bDropSoundHeal   = bOptionDropSoundHeal;
    Save->bDropSoundUsable = bOptionDropSoundUsable;
    Save->bDropSoundMisc   = bOptionDropSoundMisc;
    // Audio
    Save->bMuteWhenMinimized = bOptionMuteWhenMinimized;
    Save->fMasterVolume  = fOptionMasterVolume;
    Save->fBgmVolume     = fOptionBgmVolume;
    Save->fSfxVolume     = fOptionSfxVolume;
    Save->fAmbientVolume = fOptionAmbientVolume;
    // Video
    Save->iSpriteQuality = iOptionSpriteQuality;
    // Login
    Save->bRememberUsername = bRememberUsername;
    Save->RememberedUsername = (bRememberUsername && !Username.IsEmpty()) ? Username : FString();

    UGameplayStatics::SaveGameToSlot(Save, OptionsSaveSlot, 0);
}

void UMMOGameInstance::LoadGameOptions()
{
    if (!UGameplayStatics::DoesSaveGameExist(OptionsSaveSlot, 0))
        return;

    UOptionsSaveGame* Save = Cast<UOptionsSaveGame>(
        UGameplayStatics::LoadGameFromSlot(OptionsSaveSlot, 0));
    if (!Save) return;

    // Display
    bOptionShowFPS = Save->bShowFPS;
    bOptionSkillEffects = Save->bSkillEffects;
    bOptionShowMissText = Save->bShowMissText;
    fOptionBrightness = Save->fBrightness;
    // Interface
    bOptionShowDamageNumbers = Save->bShowDamageNumbers;
    bOptionShowEnemyHPBars = Save->bShowEnemyHPBars;
    bOptionShowPlayerNames = Save->bShowPlayerNames;
    bOptionShowEnemyNames = Save->bShowEnemyNames;
    // Camera
    fOptionCameraSensitivity = Save->fCameraSensitivity;
    fOptionCameraZoomSpeed = Save->fCameraZoomSpeed;
    // Gameplay
    bOptionNoCtrl = Save->bNoCtrl;
    bOptionNoShift = Save->bNoShift;
    bOptionAutoDeclineTrades = Save->bAutoDeclineTrades;
    bOptionAutoDeclineParty = Save->bAutoDeclineParty;
    // Interface (extended)
    bOptionShowCastBars = Save->bShowCastBars;
    bOptionShowNPCNames = Save->bShowNPCNames;
    bOptionShowChatTimestamps = Save->bShowChatTimestamps;
    fOptionChatOpacity = Save->fChatOpacity;
    fOptionDamageNumberScale = Save->fDamageNumberScale;
    // Drop Sounds
    bOptionDropSoundMvp    = Save->bDropSoundMvp;
    bOptionDropSoundCard   = Save->bDropSoundCard;
    bOptionDropSoundEquip  = Save->bDropSoundEquip;
    bOptionDropSoundHeal   = Save->bDropSoundHeal;
    bOptionDropSoundUsable = Save->bDropSoundUsable;
    bOptionDropSoundMisc   = Save->bDropSoundMisc;
    // Audio
    bOptionMuteWhenMinimized = Save->bMuteWhenMinimized;
    fOptionMasterVolume  = Save->fMasterVolume;
    fOptionBgmVolume     = Save->fBgmVolume;
    fOptionSfxVolume     = Save->fSfxVolume;
    fOptionAmbientVolume = Save->fAmbientVolume;
    // Video
    iOptionSpriteQuality = FMath::Clamp(Save->iSpriteQuality, 0, 3);
    // Login
    bRememberUsername = Save->bRememberUsername;
    if (bRememberUsername)
        RememberedUsername = Save->RememberedUsername;
    else
        RememberedUsername.Empty();
}

// ---- Logout ----

void UMMOGameInstance::Logout()
{
    // Disconnect persistent socket first
    DisconnectSocket();

    // Save remembered username before clearing
    SaveRememberedUsername();

    ClearAuthData();
    SelectedServer = FServerInfo();
    ServerList.Empty();

    UE_LOG(LogTemp, Log, TEXT("Logged out. Returning to login screen."));
}

void UMMOGameInstance::ReturnToCharacterSelect()
{
    // Emit player:leave so the server does full cleanup but keeps the socket alive
    EmitSocketEvent(TEXT("player:leave"), TEXT("{}"));

    // Clear character selection (but keep auth + server selection)
    SelectedCharacterId = 0;
    SelectedCharacter = FCharacterData();
    PendingLevelName.Empty();
    PendingZoneName.Empty();
    PendingSpawnLocation = FVector::ZeroVector;
    bIsZoneTransitioning = false;
    CurrentZoneName = TEXT("prontera_south");

    // Flag that we're returning to char select (socket stays connected)
    bReturningToCharSelect = true;

    UE_LOG(LogTemp, Log, TEXT("[ReturnToCharSelect] Emitted player:leave. Opening login level."));

    // Open the login level. LoginFlowSubsystem's background widget uses Z=200 which
    // fully covers all game UI (Z 5-50). Game widgets get cleaned up when subsystem
    // Deinitialize() runs during world teardown.
    UWorld* World = GetWorld();
    if (World)
    {
        UGameplayStatics::OpenLevel(World, *LoginLevelName);
    }
}

void UMMOGameInstance::Shutdown()
{
    DisconnectSocket();
    Super::Shutdown();
}

// ============================================================
// Persistent Socket Connection (Phase 4)
// ============================================================

void UMMOGameInstance::ConnectSocket()
{
    if (NativeSocket.IsValid() && NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("ConnectSocket called but socket is already connected."));
        return;
    }

    // Create a fresh native socket via the plugin module
    NativeSocket = ISocketIOClientModule::Get().NewValidNativePointer();
    if (!NativeSocket.IsValid())
    {
        UE_LOG(LogMMOSocket, Error, TEXT("Failed to create FSocketIONative via plugin module!"));
        return;
    }

    // Configure socket behavior
    NativeSocket->bCallbackOnGameThread = true;
    NativeSocket->bUnbindEventsOnDisconnect = false;  // Keep event bindings across reconnects
    NativeSocket->MaxReconnectionAttempts = 0;         // Infinite reconnection attempts
    NativeSocket->ReconnectionDelay = 3000;            // 3s between reconnect attempts
    NativeSocket->VerboseLog = false;

    // Set up connection callbacks
    NativeSocket->OnConnectedCallback = [this](const FString& SocketId, const FString& SessionId)
    {
        OnSocketConnected(SocketId, SessionId);
    };

    NativeSocket->OnDisconnectedCallback = [this](const ESIOConnectionCloseReason Reason)
    {
        OnSocketDisconnected((int32)Reason);
    };

    NativeSocket->OnReconnectionCallback = [this](const uint32 AttemptCount, const uint32 DelayInMs)
    {
        OnSocketReconnecting(AttemptCount, DelayInMs);
    };

    // Bind the event router to this native client
    if (EventRouter)
    {
        EventRouter->BindToNativeClient(NativeSocket);
    }

    // Connect to the server
    FString SocketUrl = GetServerSocketUrl();
    UE_LOG(LogMMOSocket, Log, TEXT("Connecting persistent socket to: %s"), *SocketUrl);
    NativeSocket->Connect(SocketUrl);
}

void UMMOGameInstance::DisconnectSocket()
{
    if (!NativeSocket.IsValid()) return;

    UE_LOG(LogMMOSocket, Log, TEXT("Disconnecting persistent socket..."));

    // Unbind router from native client
    if (EventRouter)
    {
        EventRouter->UnbindFromNativeClient();
    }

    NativeSocket->SyncDisconnect();
    ISocketIOClientModule::Get().ReleaseNativePointer(NativeSocket);
    NativeSocket.Reset();
}

bool UMMOGameInstance::IsSocketConnected() const
{
    return NativeSocket.IsValid() && NativeSocket->bIsConnected;
}

void UMMOGameInstance::EmitSocketEvent(const FString& EventName, const TSharedPtr<FJsonObject>& Payload)
{
    if (!NativeSocket.IsValid() || !NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("EmitSocketEvent(%s) — socket not connected, dropping event."), *EventName);
        return;
    }

    NativeSocket->Emit(EventName, Payload);
}

void UMMOGameInstance::EmitSocketEvent(const FString& EventName, const FString& StringPayload)
{
    if (!NativeSocket.IsValid() || !NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("EmitSocketEvent(%s) — socket not connected, dropping event."), *EventName);
        return;
    }

    NativeSocket->Emit(EventName, StringPayload);
}

void UMMOGameInstance::K2_EmitSocketEvent(const FString& EventName, USIOJsonObject* Payload)
{
    if (!NativeSocket.IsValid() || !NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("K2_EmitSocketEvent(%s) — socket not connected, dropping event."), *EventName);
        return;
    }

    if (Payload)
    {
        NativeSocket->Emit(EventName, Payload->GetRootObject());
    }
    else
    {
        NativeSocket->Emit(EventName, FString(TEXT("{}")));
    }
}

void UMMOGameInstance::EmitPlayerJoin()
{
    if (!NativeSocket.IsValid() || !NativeSocket->bIsConnected)
    {
        UE_LOG(LogMMOSocket, Warning, TEXT("EmitPlayerJoin — socket not connected!"));
        return;
    }

    auto Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("characterId"), FString::FromInt(SelectedCharacter.CharacterId));
    Payload->SetStringField(TEXT("token"), GetAuthHeader());
    Payload->SetStringField(TEXT("characterName"), SelectedCharacter.Name);

    NativeSocket->Emit(TEXT("player:join"), Payload);

    UE_LOG(LogMMOSocket, Log, TEXT("EmitPlayerJoin — characterId=%d, name=%s"),
        SelectedCharacter.CharacterId, *SelectedCharacter.Name);
}

void UMMOGameInstance::OnSocketConnected(const FString& SocketId, const FString& SessionId)
{
    UE_LOG(LogMMOSocket, Log, TEXT("Persistent socket connected! SocketId=%s, SessionId=%s"),
        *SocketId, *SessionId);

    // Re-bind event router after reconnect (events may have been cleared)
    if (EventRouter)
    {
        EventRouter->RebindAllEvents();
    }

    // Automatically re-join the game world
    EmitPlayerJoin();
}

void UMMOGameInstance::OnSocketDisconnected(int32 Reason)
{
    UE_LOG(LogMMOSocket, Warning, TEXT("Persistent socket disconnected (reason=%d). Auto-reconnect will attempt."),
        Reason);
}

void UMMOGameInstance::OnSocketReconnecting(const uint32 AttemptCount, const uint32 DelayInMs)
{
    UE_LOG(LogMMOSocket, Log, TEXT("Socket reconnecting... attempt #%u (delay=%ums)"), AttemptCount, DelayInMs);
}
