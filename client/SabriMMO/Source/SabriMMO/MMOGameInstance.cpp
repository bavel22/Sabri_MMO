#include "MMOGameInstance.h"
#include "SocketEventRouter.h"
#include "SocketIONative.h"
#include "SocketIOClient.h"
#include "SIOJsonObject.h"
#include "Misc/ConfigCacheIni.h"

DEFINE_LOG_CATEGORY_STATIC(LogMMOSocket, Log, All);

void UMMOGameInstance::Init()
{
    Super::Init();
    LoadRememberedUsername();

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

void UMMOGameInstance::SaveRememberedUsername()
{
    const FString IniPath = FPaths::GeneratedConfigDir() + TEXT("SabriMMO.ini");

    if (bRememberUsername && !Username.IsEmpty())
    {
        GConfig->SetBool(TEXT("LoginSettings"), TEXT("bRememberUsername"), true, IniPath);
        GConfig->SetString(TEXT("LoginSettings"), TEXT("RememberedUsername"), *Username, IniPath);
    }
    else
    {
        GConfig->SetBool(TEXT("LoginSettings"), TEXT("bRememberUsername"), false, IniPath);
        GConfig->SetString(TEXT("LoginSettings"), TEXT("RememberedUsername"), TEXT(""), IniPath);
    }
    GConfig->Flush(false, IniPath);
}

void UMMOGameInstance::LoadRememberedUsername()
{
    const FString IniPath = FPaths::GeneratedConfigDir() + TEXT("SabriMMO.ini");

    GConfig->GetBool(TEXT("LoginSettings"), TEXT("bRememberUsername"), bRememberUsername, IniPath);
    if (bRememberUsername)
    {
        GConfig->GetString(TEXT("LoginSettings"), TEXT("RememberedUsername"), RememberedUsername, IniPath);
    }
    else
    {
        RememberedUsername.Empty();
    }
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
