#include "MMOGameInstance.h"
#include "Misc/ConfigCacheIni.h"

void UMMOGameInstance::Init()
{
    Super::Init();
    LoadRememberedUsername();
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
    // Save remembered username before clearing
    SaveRememberedUsername();

    ClearAuthData();
    SelectedServer = FServerInfo();
    ServerList.Empty();

    UE_LOG(LogTemp, Log, TEXT("Logged out. Returning to login screen."));
}
