#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "CharacterData.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "MMOGameInstance.generated.h"

class FSocketIONative;
class USocketEventRouter;
class USIOJsonObject;

/**
 * Persists game options across sessions via UE5 SaveGame system.
 * Saved to {Saved}/SaveGames/SabriMMO_Options.sav
 */
UCLASS()
class SABRIMMO_API UOptionsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Display
	UPROPERTY() bool bShowFPS = false;
	UPROPERTY() bool bSkillEffects = true;
	UPROPERTY() bool bShowMissText = true;
	UPROPERTY() float fBrightness = 1.0f;
	// Interface
	UPROPERTY() bool bShowDamageNumbers = true;
	UPROPERTY() bool bShowEnemyHPBars = true;
	UPROPERTY() bool bShowPlayerNames = true;
	UPROPERTY() bool bShowEnemyNames = true;
	// Camera
	UPROPERTY() float fCameraSensitivity = 0.6f;
	UPROPERTY() float fCameraZoomSpeed = 80.f;
	// Gameplay
	UPROPERTY() bool bNoCtrl = true;
	UPROPERTY() bool bNoShift = false;
	UPROPERTY() bool bAutoDeclineTrades = false;
	UPROPERTY() bool bAutoDeclineParty = false;
	// Interface (extended)
	UPROPERTY() bool bShowCastBars = true;
	UPROPERTY() bool bShowNPCNames = true;
	UPROPERTY() bool bShowChatTimestamps = false;
	UPROPERTY() float fChatOpacity = 0.90f;
	UPROPERTY() float fDamageNumberScale = 1.0f;
	// Drop Sounds
	UPROPERTY() bool bDropSoundMvp    = true;
	UPROPERTY() bool bDropSoundCard   = true;
	UPROPERTY() bool bDropSoundEquip  = false;
	UPROPERTY() bool bDropSoundHeal   = false;
	UPROPERTY() bool bDropSoundUsable = false;
	UPROPERTY() bool bDropSoundMisc   = false;
	// Audio
	UPROPERTY() bool bMuteWhenMinimized = true;
	UPROPERTY() float fMasterVolume  = 1.0f;
	UPROPERTY() float fBgmVolume     = 0.7f;
	UPROPERTY() float fSfxVolume     = 1.0f;
	UPROPERTY() float fAmbientVolume = 0.5f;
	// Login
	UPROPERTY() bool bRememberUsername = false;
	UPROPERTY() FString RememberedUsername;
};

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

    // Return to character select screen (ESC menu). Emits player:leave, keeps socket alive.
    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void ReturnToCharacterSelect();

    // Login level name (loaded from config or default)
    FString LoginLevelName = TEXT("L_Startup");

    virtual void Init() override;
    virtual void Shutdown() override;

    // ---- Persistent Socket Connection (Phase 4) ----

    // Connect the persistent socket to the server. Call after character selection.
    UFUNCTION(BlueprintCallable, Category = "MMO Socket")
    void ConnectSocket();

    // Disconnect the persistent socket. Called on logout.
    UFUNCTION(BlueprintCallable, Category = "MMO Socket")
    void DisconnectSocket();

    // Check if the persistent socket is currently connected.
    UFUNCTION(BlueprintPure, Category = "MMO Socket")
    bool IsSocketConnected() const;

    // Emit a socket event with a JSON object payload (C++ only).
    void EmitSocketEvent(const FString& EventName, const TSharedPtr<FJsonObject>& Payload);

    // Emit a socket event with a string payload (C++ only).
    void EmitSocketEvent(const FString& EventName, const FString& StringPayload);

    // Emit a socket event from Blueprint using SIOJsonObject (same type BP_SocketManager uses).
    UFUNCTION(BlueprintCallable, Category = "MMO Socket", meta = (DisplayName = "Emit Socket Event"))
    void K2_EmitSocketEvent(const FString& EventName, USIOJsonObject* Payload);

    // Emit player:join with current character data and auth token.
    void EmitPlayerJoin();

    // Get the event router for registering socket event handlers.
    USocketEventRouter* GetEventRouter() const { return EventRouter; }

    // Get the raw native socket (rarely needed — prefer EmitSocketEvent).
    TSharedPtr<FSocketIONative> GetNativeSocket() const { return NativeSocket; }

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

    // ---- Return to Character Select (ESC menu) ----
    // Set by ReturnToCharacterSelect(), consumed by LoginFlowSubsystem
    bool bReturningToCharSelect = false;

    // ---- Minimap Preferences (persist across zone transitions) ----
    int32 MinimapOpacityState = 2;  // 0=hidden, 1=transparent, 2=opaque
    int32 MinimapZoomLevel = 0;     // 0-4

    // ---- Game Options (persist across zones + sessions via SabriMMO.ini) ----
    // Display
    bool bOptionShowFPS = false;
    bool bOptionSkillEffects = true;
    bool bOptionShowMissText = true;
    float fOptionBrightness = 1.0f;
    // Interface
    bool bOptionShowDamageNumbers = true;
    bool bOptionShowEnemyHPBars = true;
    bool bOptionShowPlayerNames = true;
    bool bOptionShowEnemyNames = true;
    // Camera
    float fOptionCameraSensitivity = 0.6f;
    float fOptionCameraZoomSpeed = 80.f;
    // Gameplay
    bool bOptionNoCtrl = true;              // RO /noctrl: attack on click without Ctrl
    bool bOptionNoShift = false;            // RO /noshift: support skills without Shift
    bool bOptionAutoDeclineTrades = false;
    bool bOptionAutoDeclineParty = false;
    // Interface (extended)
    bool bOptionShowCastBars = true;
    bool bOptionShowNPCNames = true;
    bool bOptionShowChatTimestamps = false;
    float fOptionChatOpacity = 0.90f;
    float fOptionDamageNumberScale = 1.0f;
    // Drop Sounds (per tier — which item tiers play a sound on drop)
    bool bOptionDropSoundMvp    = true;   // Red — MVP exclusive
    bool bOptionDropSoundCard   = true;   // Purple — cards
    bool bOptionDropSoundEquip  = false;  // Blue — weapons/armor
    bool bOptionDropSoundHeal   = false;  // Green — potions
    bool bOptionDropSoundUsable = false;  // Yellow — usable/ammo
    bool bOptionDropSoundMisc   = false;  // Pink — etc/materials
    // Audio
    bool bOptionMuteWhenMinimized = true;
    float fOptionMasterVolume  = 1.0f;
    float fOptionBgmVolume     = 0.7f;
    float fOptionSfxVolume     = 1.0f;
    float fOptionAmbientVolume = 0.5f;

    void SaveGameOptions();
    void LoadGameOptions();

private:
    UPROPERTY()
    int32 SelectedCharacterId;

    UPROPERTY()
    FCharacterData SelectedCharacter;

    // ---- Persistent Socket (Phase 4) ----
    TSharedPtr<FSocketIONative> NativeSocket;

    UPROPERTY()
    TObjectPtr<USocketEventRouter> EventRouter;

    void OnSocketConnected(const FString& SocketId, const FString& SessionId);
    void OnSocketDisconnected(int32 Reason);
    void OnSocketReconnecting(const uint32 AttemptCount, const uint32 DelayInMs);
};
