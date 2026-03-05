#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterData.h"
#include "MMOGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterListReceived);

// New delegates with error info
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginFailedWithReason, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerListReceived, const TArray<FServerInfo>&, Servers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterCreateFailed, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeleteSuccess, const FString&, CharacterName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeleteFailed, const FString&, ErrorMessage);

UCLASS()
class SABRIMMO_API UMMOGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // ---- Authentication ----
    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    FString AuthToken;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    FString Username;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    int32 UserId;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    bool bIsLoggedIn;

    // ---- Character Data ----
    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    TArray<FCharacterData> CharacterList;

    // ---- Server Selection ----
    UPROPERTY(BlueprintReadOnly, Category = "MMO Server")
    FServerInfo SelectedServer;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Server")
    TArray<FServerInfo> ServerList;

    // ---- Remember Username ----
    UPROPERTY(BlueprintReadWrite, Category = "MMO Settings")
    bool bRememberUsername = false;

    UPROPERTY(BlueprintReadWrite, Category = "MMO Settings")
    FString RememberedUsername;

    // ---- Configurable Server URL ----
    UPROPERTY(BlueprintReadWrite, Category = "MMO Server")
    FString ServerBaseUrl = TEXT("http://localhost:3001");

    // ---- Event Dispatchers (legacy — kept for Blueprint compatibility) ----
    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginSuccess OnLoginSuccess;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginFailed OnLoginFailed;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterCreated OnCharacterCreated;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterListReceived OnCharacterListReceived;

    // ---- Event Dispatchers (new — with error info) ----
    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginFailedWithReason OnLoginFailedWithReason;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnServerListReceived OnServerListReceived;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterCreateFailed OnCharacterCreateFailed;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterDeleteSuccess OnCharacterDeleteSuccess;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterDeleteFailed OnCharacterDeleteFailed;

    // ---- Functions ----
    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void SetAuthData(const FString& InToken, const FString& InUsername, int32 InUserId);

    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void ClearAuthData();

    UFUNCTION(BlueprintPure, Category = "MMO Auth")
    bool IsAuthenticated() const;

    UFUNCTION(BlueprintPure, Category = "MMO Auth")
    FString GetAuthHeader() const;

    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void SetCharacterList(const TArray<FCharacterData>& Characters);

    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void SelectCharacter(int32 CharacterId);

    UFUNCTION(BlueprintPure, Category = "MMO Auth")
    FCharacterData GetSelectedCharacter() const;

    // Server selection
    UFUNCTION(BlueprintCallable, Category = "MMO Server")
    void SetServerList(const TArray<FServerInfo>& Servers);

    UFUNCTION(BlueprintCallable, Category = "MMO Server")
    void SelectServer(const FServerInfo& Server);

    UFUNCTION(BlueprintPure, Category = "MMO Server")
    FString GetServerSocketUrl() const;

    // Remember username (persisted via GameUserSettings.ini section)
    UFUNCTION(BlueprintCallable, Category = "MMO Settings")
    void SaveRememberedUsername();

    UFUNCTION(BlueprintCallable, Category = "MMO Settings")
    void LoadRememberedUsername();

    // Logout
    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void Logout();

    virtual void Init() override;

    // ---- Zone System (survives level transitions) ----
    UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
    FString CurrentZoneName = TEXT("prontera_south");

    UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
    FString PendingZoneName;

    UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
    FString PendingLevelName;

    UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
    FVector PendingSpawnLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "MMO Zone")
    bool bIsZoneTransitioning = false;

private:
    UPROPERTY()
    int32 SelectedCharacterId;

    UPROPERTY()
    FCharacterData SelectedCharacter;
};
