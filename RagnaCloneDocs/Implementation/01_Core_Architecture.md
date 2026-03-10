# 01 - Core Architecture: UE5 C++ Implementation Guide

> **Scope**: Complete C++ implementation guide for Sabri_MMO, a Ragnarok Online Classic 3D MMO replica.
> **Stack**: UE5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis
> **Audience**: AI coding assistant (Claude Code) -- every pattern shown here must be achievable without manual Blueprint input from the user.

---

## Table of Contents

1. [Project Structure](#1-project-structure)
2. [GameInstance (UMMOGameInstance)](#2-gameinstance-ummogameinstance)
3. [Networking Layer](#3-networking-layer)
4. [UWorldSubsystem Pattern](#4-uworldsubsystem-pattern)
5. [Data Structures](#5-data-structures)
6. [GameMode and PlayerController](#6-gamemode-and-playercontroller)
7. [Character Actor (ASabriMMOCharacter)](#7-character-actor-asabrimmocharacter)
8. [Server Communication Protocol](#8-server-communication-protocol)
9. [Configuration System](#9-configuration-system)

---

## 1. Project Structure

### 1.1 Module Organization

The project uses a single C++ module named `SabriMMO`. All game code lives inside this module. There is no multi-module split -- this keeps compilation fast and avoids circular dependency headaches for a solo-developer project.

**Module root**: `client/SabriMMO/Source/SabriMMO/`

### 1.2 Folder Structure

```
client/SabriMMO/Source/SabriMMO/
|-- SabriMMO.h                      # Module header (DECLARE_LOG_CATEGORY)
|-- SabriMMO.cpp                    # Module implementation (empty)
|-- SabriMMO.Build.cs               # Module build rules
|-- CharacterData.h                 # All shared data structs (FCharacterData, FInventoryItem, etc.)
|-- MMOGameInstance.h/.cpp          # Cross-level persistent state
|-- MMOHttpManager.h/.cpp           # REST API (BlueprintFunctionLibrary)
|-- SabriMMOCharacter.h/.cpp        # Base player pawn class
|-- SabriMMOGameMode.h/.cpp         # Base GameMode (DefaultPawnClass = nullptr)
|-- SabriMMOPlayerController.h/.cpp # C++ PlayerController (not currently used by game)
|-- OtherCharacterMovementComponent.h/.cpp  # Remote player interpolation
|-- WarpPortal.h/.cpp               # Zone transition trigger actor
|-- KafraNPC.h/.cpp                 # Clickable Kafra NPC actor
|-- ShopNPC.h/.cpp                  # Clickable Shop NPC actor
|
|-- UI/                             # All UI subsystems and Slate widgets
|   |-- LoginFlowSubsystem.h/.cpp           # Login -> ServerSelect -> CharSelect state machine
|   |-- BasicInfoSubsystem.h/.cpp           # HP/SP/EXP HUD panel
|   |-- CombatStatsSubsystem.h/.cpp         # Detailed stat window (F8)
|   |-- DamageNumberSubsystem.h/.cpp        # Floating damage numbers
|   |-- CastBarSubsystem.h/.cpp             # Skill cast bars
|   |-- InventorySubsystem.h/.cpp           # Inventory window (F6)
|   |-- EquipmentSubsystem.h/.cpp           # Equipment window (F7)
|   |-- HotbarSubsystem.h/.cpp              # Hotbar rows (F5 cycle)
|   |-- SkillTreeSubsystem.h/.cpp           # Skill tree (K key, Blueprint-based)
|   |-- WorldHealthBarSubsystem.h/.cpp      # Floating HP/SP bars over characters
|   |-- ZoneTransitionSubsystem.h/.cpp      # Zone change + loading overlay
|   |-- KafraSubsystem.h/.cpp               # Kafra NPC service dialog
|   |-- ShopSubsystem.h/.cpp                # NPC shop buy/sell
|   |-- SLoginWidget.h/.cpp                 # Slate: login screen
|   |-- SServerSelectWidget.h/.cpp          # Slate: server list
|   |-- SCharacterSelectWidget.h/.cpp       # Slate: character grid
|   |-- SCharacterCreateWidget.h/.cpp       # Slate: character creation
|   |-- SLoadingOverlayWidget.h/.cpp        # Slate: loading overlay
|   |-- SBasicInfoWidget.h/.cpp             # Slate: HP/SP/EXP bars
|   |-- SCombatStatsWidget.h/.cpp           # Slate: stat detail window
|   |-- SDamageNumberOverlay.h/.cpp         # Slate: floating damage text
|   |-- SCastBarOverlay.h/.cpp              # Slate: cast bar overlay
|   |-- SInventoryWidget.h/.cpp             # Slate: inventory grid
|   |-- SEquipmentWidget.h/.cpp             # Slate: equipment panel
|   |-- SHotbarRowWidget.h/.cpp             # Slate: single hotbar row
|   |-- SHotbarKeybindWidget.h/.cpp         # Slate: hotbar keybind config
|   |-- SSkillTreeWidget.h/.cpp             # Slate: skill tree panel
|   |-- SWorldHealthBarOverlay.h/.cpp       # Slate: floating world HP bars
|   |-- SKafraWidget.h/.cpp                 # Slate: Kafra service dialog
|   |-- SShopWidget.h/.cpp                  # Slate: NPC shop buy/sell
|   |-- SSkillTargetingOverlay.h/.cpp       # Slate: ground targeting reticle
|   |-- SkillDragDropOperation.h            # Drag-drop data for skills
|
|-- VFX/                            # Visual effects subsystem
|   |-- SkillVFXSubsystem.h/.cpp    # Niagara/Cascade VFX spawning
|   |-- SkillVFXData.h/.cpp         # Per-skill VFX config structs
|   |-- CastingCircleActor.h/.cpp   # Decal/Niagara casting circle
```

### 1.3 Header/Source File Conventions

Every C++ class follows this pattern:

```
ClassName.h    -- Full class declaration with UCLASS(), UPROPERTY(), UFUNCTION()
ClassName.cpp  -- Implementation, includes, log category definition
```

Rules:
- One UCLASS per `.h/.cpp` pair (small helper structs can share the same header)
- Data-only structs (USTRUCT) go in `CharacterData.h` when shared across multiple systems
- System-specific structs go in that system's header (e.g., `FSkillVFXConfig` in `SkillVFXData.h`)
- Use `#pragma once` (not include guards)
- Define log categories per subsystem: `DEFINE_LOG_CATEGORY_STATIC(LogBasicInfo, Log, All);`

### 1.4 Build.cs Module Dependencies

**File**: `client/SabriMMO/Source/SabriMMO/SabriMMO.Build.cs`

```csharp
using UnrealBuildTool;

public class SabriMMO : ModuleRules
{
    public SabriMMO(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            // Core engine
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",

            // Enhanced Input system
            "EnhancedInput",

            // AI (for Variant_Combat enemies, StateTree)
            "AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",

            // UI (Slate + UMG)
            "UMG",
            "Slate",
            "SlateCore",

            // Networking (Socket.io plugin + HTTP)
            "SocketIOClient",   // Third-party Socket.io UE5 plugin
            "SIOJson",          // JSON helpers from SocketIOClient plugin
            "HTTP",             // UE5 built-in HTTP module
            "Json",             // FJsonObject / FJsonValue
            "JsonUtilities",    // FJsonObjectConverter

            // VFX
            "Niagara",          // UNiagaraSystem, UNiagaraComponent
            "NiagaraCore"       // Core Niagara types
        });

        // NetworkPrediction excluded due to UE 5.7 compiler bug
        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicIncludePaths.AddRange(new string[] {
            "SabriMMO",
            "SabriMMO/UI",
            "SabriMMO/VFX"
            // ... additional variant paths as needed
        });
    }
}
```

**Key dependency notes**:

| Dependency | Why |
|-----------|-----|
| `SocketIOClient` + `SIOJson` | Socket.io client for real-time server communication. Provides `USocketIOClientComponent`, `FSocketIONative`, JSON event handling. |
| `EnhancedInput` | UE5's input system. Used for WASD, hotbar slots, UI toggle keys (F5-F9). All input mapped programmatically in C++ -- no editor assets needed. |
| `Slate` + `SlateCore` | All UI is pure Slate (SNew/SCompoundWidget), not UMG. UMG is included for legacy variant code. |
| `Niagara` + `NiagaraCore` | Skill VFX system uses Niagara particle systems. Casting circles, bolt effects, AoE impacts all spawn via `UNiagaraFunctionLibrary::SpawnSystemAtLocation`. |
| `HTTP` + `Json` | REST API calls (login, register, character CRUD, position saves) use `FHttpModule` and `FJsonObject`. |

---

## 2. GameInstance (UMMOGameInstance)

### 2.1 Why GameInstance

`UGameInstance` is the **only** UObject that persists across level transitions in UE5. When the player warps from `L_PrtSouth` to `L_Prontera`, the World, GameMode, PlayerController, all Actors, and all Subsystems are destroyed and recreated. Only the GameInstance survives.

This makes it the correct place for:
- Auth state (JWT token, user ID)
- Character data (selected character)
- Server connection info (URL, selected server)
- Zone transition state (pending zone, spawn location, transitioning flag)

### 2.2 Complete Header

**File**: `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterData.h"
#include "MMOGameInstance.generated.h"

// -- Delegate declarations --
// Legacy (no params, kept for Blueprint compatibility)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterListReceived);

// New (with error info for Slate widgets)
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

    // ---- Saved Preferences ----
    UPROPERTY(BlueprintReadWrite, Category = "MMO Settings")
    bool bRememberUsername = false;

    UPROPERTY(BlueprintReadWrite, Category = "MMO Settings")
    FString RememberedUsername;

    // ---- Configurable Server URL ----
    UPROPERTY(BlueprintReadWrite, Category = "MMO Server")
    FString ServerBaseUrl = TEXT("http://localhost:3001");

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

    // ---- Event Dispatchers ----
    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginSuccess OnLoginSuccess;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginFailed OnLoginFailed;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterCreated OnCharacterCreated;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterListReceived OnCharacterListReceived;

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

    UFUNCTION(BlueprintCallable, Category = "MMO Server")
    void SetServerList(const TArray<FServerInfo>& Servers);

    UFUNCTION(BlueprintCallable, Category = "MMO Server")
    void SelectServer(const FServerInfo& Server);

    UFUNCTION(BlueprintPure, Category = "MMO Server")
    FString GetServerSocketUrl() const;

    UFUNCTION(BlueprintCallable, Category = "MMO Settings")
    void SaveRememberedUsername();

    UFUNCTION(BlueprintCallable, Category = "MMO Settings")
    void LoadRememberedUsername();

    UFUNCTION(BlueprintCallable, Category = "MMO Auth")
    void Logout();

    virtual void Init() override;

private:
    UPROPERTY()
    int32 SelectedCharacterId;

    UPROPERTY()
    FCharacterData SelectedCharacter;
};
```

### 2.3 Complete Implementation

**File**: `client/SabriMMO/Source/SabriMMO/MMOGameInstance.cpp`

```cpp
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
    OnLoginSuccess.Broadcast();
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
}

bool UMMOGameInstance::IsAuthenticated() const
{
    return bIsLoggedIn && !AuthToken.IsEmpty();
}

FString UMMOGameInstance::GetAuthHeader() const
{
    if (AuthToken.IsEmpty()) return TEXT("");
    return TEXT("Bearer ") + AuthToken;
}

void UMMOGameInstance::SetCharacterList(const TArray<FCharacterData>& Characters)
{
    CharacterList = Characters;
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
            return;
        }
    }
}

FCharacterData UMMOGameInstance::GetSelectedCharacter() const
{
    return SelectedCharacter;
}

void UMMOGameInstance::SetServerList(const TArray<FServerInfo>& Servers)
{
    ServerList = Servers;
    OnServerListReceived.Broadcast(Servers);
}

void UMMOGameInstance::SelectServer(const FServerInfo& Server)
{
    SelectedServer = Server;
    ServerBaseUrl = FString::Printf(TEXT("http://%s:%d"), *Server.Host, Server.Port);
}

FString UMMOGameInstance::GetServerSocketUrl() const
{
    if (!SelectedServer.Host.IsEmpty())
    {
        return FString::Printf(TEXT("http://%s:%d"), *SelectedServer.Host, SelectedServer.Port);
    }
    return ServerBaseUrl;
}

// ---- Persisted username (saved via GConfig to SabriMMO.ini) ----

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

void UMMOGameInstance::Logout()
{
    SaveRememberedUsername();
    ClearAuthData();
    SelectedServer = FServerInfo();
    ServerList.Empty();
}
```

### 2.4 How to Extend for New Systems

When adding guild data, party data, or any new cross-level state, add it directly to `UMMOGameInstance`:

```cpp
// In MMOGameInstance.h, add:
// ---- Party System (survives level transitions) ----
UPROPERTY(BlueprintReadWrite, Category = "MMO Party")
int32 CurrentPartyId = 0;

UPROPERTY(BlueprintReadWrite, Category = "MMO Party")
TArray<FPartyMember> PartyMembers;

// ---- Guild System ----
UPROPERTY(BlueprintReadWrite, Category = "MMO Guild")
int32 CurrentGuildId = 0;

UPROPERTY(BlueprintReadWrite, Category = "MMO Guild")
FString GuildName;
```

**Do NOT store cross-level state in**:
- PlayerController (destroyed on level change)
- GameMode (destroyed on level change)
- Subsystems (destroyed on level change)
- Any Actor (destroyed on level change)

---

## 3. Networking Layer

### 3.1 Socket.io Client Integration

The project uses the [SocketIOClient-Unreal](https://github.com/nicholasc/SocketIOClient-Unreal) plugin, which provides:
- `USocketIOClientComponent` -- an ActorComponent that manages a single Socket.io connection
- `FSocketIONative` -- the native C++ Socket.io client (accessed via `GetNativeClient()`)
- Automatic JSON serialization/deserialization via `SIOJson`
- Game-thread callback marshalling via `ESIOThreadOverrideOption::USE_GAME_THREAD`

**Setup**: A Blueprint actor called `BP_SocketManager` exists in each game level. It has a `USocketIOClientComponent` attached and connects to the server on BeginPlay. C++ subsystems find this component at runtime.

### 3.2 Finding the Socket.io Component

Every subsystem that needs Socket.io events uses the same discovery pattern:

```cpp
USocketIOClientComponent* UMySubsystem::FindSocketIOComponent() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    // Iterate all actors looking for one with a USocketIOClientComponent
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (USocketIOClientComponent* Comp = It->FindComponentByClass<USocketIOClientComponent>())
        {
            return Comp;
        }
    }
    return nullptr;
}
```

This works because there is exactly one `BP_SocketManager` actor per level. The search is done once, and the result is cached in a `TWeakObjectPtr<USocketIOClientComponent>`.

### 3.3 Event Registration Pattern (Per-Subsystem Wrapping)

The key challenge: `BP_SocketManager` registers Blueprint callbacks for socket events during its own BeginPlay. C++ subsystems start at `OnWorldBeginPlay`, which may happen before or after the Blueprint actor initializes. The solution is a **polling timer** that waits for the Socket.io component to be connected and have its Blueprint events bound, then wraps those events with additional C++ handlers.

#### Step 1: Poll for readiness (OnWorldBeginPlay)

```cpp
void UMySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Start a repeating timer to poll for the SocketIO component
    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UMySubsystem::TryWrapSocketEvents),
        0.5f,   // Check every 500ms
        true    // Repeat until we succeed
    );
}
```

#### Step 2: Check readiness and wrap

```cpp
void UMySubsystem::TryWrapSocketEvents()
{
    if (bEventsWrapped) return;

    USocketIOClientComponent* SIOComp = FindSocketIOComponent();
    if (!SIOComp) return;

    TSharedPtr<FSocketIONative> NativeClient = SIOComp->GetNativeClient();
    if (!NativeClient.IsValid() || !NativeClient->bIsConnected) return;

    // Wait for Blueprint to have bound the key events
    if (!NativeClient->EventFunctionMap.Contains(TEXT("combat:health_update")))
        return;

    CachedSIOComponent = SIOComp;

    // Populate local character ID from GameInstance
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetWorld()->GetGameInstance()))
    {
        LocalCharacterId = GI->GetSelectedCharacter().CharacterId;
    }

    // --- Wrap each event ---
    WrapSingleEvent(TEXT("combat:health_update"),
        [this](const TSharedPtr<FJsonValue>& D) { HandleHealthUpdate(D); });

    WrapSingleEvent(TEXT("player:stats"),
        [this](const TSharedPtr<FJsonValue>& D) { HandlePlayerStats(D); });

    // ... more events ...

    bEventsWrapped = true;

    // Stop the polling timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }

    // Request fresh data (initial events may have fired before wrapping)
    SIOComp->EmitNative(TEXT("player:request_stats"), TEXT("{}"));
}
```

#### Step 3: WrapSingleEvent -- non-destructive event interception

```cpp
void UMySubsystem::WrapSingleEvent(
    const FString& EventName,
    TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler)
{
    if (!CachedSIOComponent.IsValid()) return;

    TSharedPtr<FSocketIONative> NativeClient = CachedSIOComponent->GetNativeClient();
    if (!NativeClient.IsValid()) return;

    // Save the original Blueprint callback
    TFunction<void(const FString&, const TSharedPtr<FJsonValue>&)> OriginalCallback;
    FSIOBoundEvent* Existing = NativeClient->EventFunctionMap.Find(EventName);
    if (Existing)
    {
        OriginalCallback = Existing->Function;
    }

    // Replace with a combined callback that runs both original + ours
    NativeClient->OnEvent(EventName,
        [OriginalCallback, OurHandler](const FString& Event, const TSharedPtr<FJsonValue>& Message)
        {
            // Call original Blueprint handler first (preserves existing behavior)
            if (OriginalCallback)
            {
                OriginalCallback(Event, Message);
            }
            // Then call our C++ handler
            if (OurHandler)
            {
                OurHandler(Message);
            }
        },
        TEXT("/"),
        ESIOThreadOverrideOption::USE_GAME_THREAD  // CRITICAL: run on game thread
    );
}
```

**Why wrapping instead of separate registration**: The SocketIOClient plugin's `OnEvent` replaces any previous callback for that event name. If we registered our callback separately, it would overwrite the Blueprint's callback. Wrapping preserves the chain.

### 3.4 JSON Deserialization Helpers

All socket event data arrives as `TSharedPtr<FJsonValue>`. The standard parsing pattern:

```cpp
void UMySubsystem::HandleHealthUpdate(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;

    // Step 1: Extract the JSON object
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    // Step 2: Read fields with safe accessors
    double CharIdD = 0;
    Obj->TryGetNumberField(TEXT("characterId"), CharIdD);
    int32 CharId = (int32)CharIdD;

    // Step 3: Filter by local character ID
    if (LocalCharacterId > 0 && CharId != LocalCharacterId) return;

    // Step 4: Extract values
    double H = 0, MH = 0, M = 0, MM = 0;
    Obj->TryGetNumberField(TEXT("health"),    H);
    Obj->TryGetNumberField(TEXT("maxHealth"), MH);
    Obj->TryGetNumberField(TEXT("mana"),      M);
    Obj->TryGetNumberField(TEXT("maxMana"),   MM);

    // Step 5: Update subsystem state
    CurrentHP = (int32)H;
    MaxHP     = FMath::Max((int32)MH, 1);
    CurrentSP = (int32)M;
    MaxSP     = FMath::Max((int32)MM, 1);
}
```

**JSON field access patterns**:

| JSON type | UE5 accessor | C++ type |
|-----------|-------------|----------|
| Number | `TryGetNumberField()` | `double` (cast to `int32`/`float`) |
| String | `TryGetStringField()` | `FString` |
| Boolean | `TryGetBoolField()` | `bool` |
| Nested object | `TryGetObjectField()` | `TSharedPtr<FJsonObject>` |
| Array | `TryGetArrayField()` | `TArray<TSharedPtr<FJsonValue>>` |

**Nested object access** (e.g., `player:stats` has `stats.str`, `exp.baseLevel`):

```cpp
void UMySubsystem::HandlePlayerStats(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;
    const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

    // Nested "stats" object
    const TSharedPtr<FJsonObject>* StatsPtr = nullptr;
    if (Obj->TryGetObjectField(TEXT("stats"), StatsPtr) && StatsPtr)
    {
        const TSharedPtr<FJsonObject>& Stats = *StatsPtr;
        double StrD = 0;
        Stats->TryGetNumberField(TEXT("str"), StrD);
        STR = (int32)StrD;
        // ... more stats ...
    }

    // Nested "exp" object
    const TSharedPtr<FJsonObject>* ExpPtr = nullptr;
    if (Obj->TryGetObjectField(TEXT("exp"), ExpPtr) && ExpPtr)
    {
        const TSharedPtr<FJsonObject>& Exp = *ExpPtr;
        double BL = 0, JL = 0;
        Exp->TryGetNumberField(TEXT("baseLevel"), BL);
        Exp->TryGetNumberField(TEXT("jobLevel"), JL);
        BaseLevel = (int32)BL;
        JobLevel  = (int32)JL;
    }
}
```

### 3.5 HTTP Manager (REST API Calls)

**File**: `client/SabriMMO/Source/SabriMMO/MMOHttpManager.h`

The HTTP manager is a `UBlueprintFunctionLibrary` with static functions. It uses UE5's `FHttpModule` for async HTTP requests.

```cpp
UCLASS()
class SABRIMMO_API UHttpManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

private:
    static UMMOGameInstance* GetGameInstance(UObject* WorldContextObject);
    static FString GetServerBaseUrl(UObject* WorldContextObject);
    static FString ExtractErrorMessage(const FString& ResponseContent, const FString& DefaultMessage);

public:
    // Auth
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password);

    // Characters
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void GetCharacters(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName,
        const FString& CharacterClass, int32 HairStyle = 1, int32 HairColor = 0,
        const FString& Gender = TEXT("male"));

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void DeleteCharacter(UObject* WorldContextObject, int32 CharacterId, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);

    // Server list
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void GetServerList(UObject* WorldContextObject);
};
```

**Implementation pattern** (LoginUser as example):

```cpp
void UHttpManager::LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password)
{
    FString BaseUrl = GetServerBaseUrl(WorldContextObject);
    FString Url = BaseUrl + TEXT("/api/auth/login");

    // Build JSON body
    TSharedPtr<FJsonObject> JsonBody = MakeShareable(new FJsonObject());
    JsonBody->SetStringField(TEXT("username"), Username);
    JsonBody->SetStringField(TEXT("password"), Password);

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);

    // Create HTTP request
    TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(RequestBody);

    // Capture WorldContextObject weakly to avoid dangling pointers
    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);

    Request->OnProcessRequestComplete().BindLambda(
        [WeakContext](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bConnectedSuccessfully)
    {
        if (!WeakContext.IsValid()) return;
        UMMOGameInstance* GI = GetGameInstance(WeakContext.Get());
        if (!GI) return;

        if (!bConnectedSuccessfully || !Resp.IsValid())
        {
            GI->OnLoginFailedWithReason.Broadcast(TEXT("Could not connect to server"));
            return;
        }

        int32 Code = Resp->GetResponseCode();
        FString Body = Resp->GetContentAsString();

        if (Code == 200)
        {
            // Parse: { "token": "...", "user": { "id": 1, "username": "..." } }
            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
            if (FJsonSerializer::Deserialize(Reader, Json) && Json.IsValid())
            {
                FString Token = Json->GetStringField(TEXT("token"));
                const TSharedPtr<FJsonObject>* UserObj;
                if (Json->TryGetObjectField(TEXT("user"), UserObj))
                {
                    int32 Id = (int32)(*UserObj)->GetNumberField(TEXT("id"));
                    FString Name = (*UserObj)->GetStringField(TEXT("username"));
                    GI->SetAuthData(Token, Name, Id);
                }
            }
        }
        else
        {
            FString Msg = ExtractErrorMessage(Body, TEXT("Login failed"));
            GI->OnLoginFailedWithReason.Broadcast(Msg);
        }
    });

    Request->ProcessRequest();
}
```

### 3.6 Connection Lifecycle Management

```
1. Client launches → login level loads
2. LoginFlowSubsystem creates login widgets
3. User enters credentials → LoginUser() HTTP call
4. On success → server list HTTP call → user selects server
5. User selects character → OpenLevel(game level)
6. Game level loads → Level Blueprint spawns BP_SocketManager
7. BP_SocketManager's USocketIOClientComponent connects to server
8. BP_SocketManager emits player:join { characterId, token, characterName }
9. Server validates JWT, returns player:joined
10. All C++ subsystems discover SocketIO component via polling timer
11. Each subsystem wraps its events and requests initial data
```

### 3.7 Reconnection Handling

The SocketIOClient plugin handles automatic reconnection. On disconnect:
1. `USocketIOClientComponent::OnDisconnected` delegate fires
2. The plugin attempts reconnection on a backoff schedule
3. On reconnect, `OnConnected` delegate fires
4. `BP_SocketManager` Blueprint re-emits `player:join`
5. C++ subsystems do NOT need to re-wrap -- the wrapped callbacks persist as long as the component exists

If the level changes (zone transition), the old `BP_SocketManager` and its `USocketIOClientComponent` are destroyed. The new level's `BP_SocketManager` creates a fresh connection.

---

## 4. UWorldSubsystem Pattern

### 4.1 Why UWorldSubsystem

`UWorldSubsystem` is the correct base for all game subsystems in this project because:

1. **Per-world lifecycle**: Created when the world initializes, destroyed when the world is torn down. No dangling references across level transitions.
2. **Multiplayer-safe**: In PIE with multiple clients, each PIE instance has its own World and its own set of subsystems. No shared global state that bleeds between clients.
3. **No manual creation**: UE5 creates subsystems automatically. No `Spawn`, `CreateInstance`, or `RegisterSubsystem` calls needed.
4. **Access from anywhere**: `GetWorld()->GetSubsystem<UMySubsystem>()` works from any Actor, Component, or other Subsystem.

**Do NOT use**:
- `UGameInstanceSubsystem` -- shared across levels, does not respect world boundaries. NOT safe for multiplayer PIE testing.
- `ULocalPlayerSubsystem` -- per-local-player, but awkward for systems that need World access (timers, actor iteration).
- Singleton patterns / global pointers -- violates UE5's world isolation. `GEngine->GameViewport` is a global singleton and MUST NOT be used (see multiplayer rules).

### 4.2 Base Subsystem Template

Every game subsystem in this project follows the same skeleton. Here is the canonical pattern:

```cpp
// MySubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dom/JsonValue.h"
#include "Dom/JsonObject.h"
#include "MySubsystem.generated.h"

class USocketIOClientComponent;

UCLASS()
class SABRIMMO_API UMySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // ---- Public data fields (read by Slate widgets) ----
    int32 SomeValue = 0;

    // ---- Lifecycle ----
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // ---- Widget visibility ----
    void ShowWidget();
    void HideWidget();
    bool IsWidgetVisible() const;

private:
    // ---- Socket event wrapping ----
    void TryWrapSocketEvents();
    void WrapSingleEvent(const FString& EventName,
        TFunction<void(const TSharedPtr<FJsonValue>&)> OurHandler);
    USocketIOClientComponent* FindSocketIOComponent() const;

    // ---- Event handlers ----
    void HandleSomeEvent(const TSharedPtr<FJsonValue>& Data);

    // ---- State ----
    bool bEventsWrapped = false;
    bool bWidgetAdded = false;
    int32 LocalCharacterId = 0;

    FTimerHandle BindCheckTimer;
    TSharedPtr<SWidget> MySlateWidget;
    TSharedPtr<SWidget> ViewportOverlay;
    TWeakObjectPtr<USocketIOClientComponent> CachedSIOComponent;
};
```

```cpp
// MySubsystem.cpp
#include "MySubsystem.h"
#include "MMOGameInstance.h"
#include "SocketIOClientComponent.h"
#include "SocketIONative.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Widgets/SWeakWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogMySubsystem, Log, All);

bool UMySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}

void UMySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    InWorld.GetTimerManager().SetTimer(
        BindCheckTimer,
        FTimerDelegate::CreateUObject(this, &UMySubsystem::TryWrapSocketEvents),
        0.5f, true
    );
}

void UMySubsystem::Deinitialize()
{
    HideWidget();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(BindCheckTimer);
    }
    bEventsWrapped = false;
    CachedSIOComponent = nullptr;
    Super::Deinitialize();
}
```

### 4.3 ShouldCreateSubsystem -- Controlling Where Subsystems Exist

```cpp
bool UMySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
    if (!World) return false;
    return World->IsGameWorld();
}
```

This ensures the subsystem only exists in gameplay worlds (not editor, not preview, not dedicated server). Some subsystems refine this further:

- **LoginFlowSubsystem** -- only creates in the login level (checks for absence of BP_SocketManager)
- **Game subsystems** (BasicInfo, Combat, etc.) -- create in all game worlds (the timer-based polling handles gracefully if no SocketIO component exists)

### 4.4 Widget Management Pattern

All Slate widgets are added to the viewport via `UGameViewportClient` (the world-scoped viewport client, NOT the global `GEngine->GameViewport`):

```cpp
void UMySubsystem::ShowWidget()
{
    if (bWidgetAdded) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // CRITICAL: Use World->GetGameViewport(), NEVER GEngine->GameViewport
    UGameViewportClient* ViewportClient = World->GetGameViewport();
    if (!ViewportClient) return;

    // Create the Slate widget
    MySlateWidget = SNew(SMyWidget).Subsystem(this);

    // Wrap it for viewport management (prevents GC of the shared pointer)
    ViewportOverlay = SNew(SWeakWidget).PossiblyNullContent(MySlateWidget);

    // Add at a specific Z-order
    // Z-order convention: BasicInfo=10, CombatStats=12, Inventory=14,
    // Equipment=15, Hotbar=16, Kafra=19, SkillTree=20, DamageNumbers=20,
    // CastBar=25, HotbarKeybind=30, LoadingOverlay=50
    ViewportClient->AddViewportWidgetContent(ViewportOverlay.ToSharedRef(), 10);
    bWidgetAdded = true;
}

void UMySubsystem::HideWidget()
{
    if (!bWidgetAdded) return;

    if (UWorld* World = GetWorld())
    {
        if (UGameViewportClient* VC = World->GetGameViewport())
        {
            if (ViewportOverlay.IsValid())
            {
                VC->RemoveViewportWidgetContent(ViewportOverlay.ToSharedRef());
            }
        }
    }

    MySlateWidget.Reset();
    ViewportOverlay.Reset();
    bWidgetAdded = false;
}
```

**MANDATORY RULE**: Never use `GEngine->GameViewport`. It is a global singleton pointing to PIE-0's viewport. In multiplayer testing (2+ PIE instances), it causes widgets from one client to appear in the wrong viewport. Always use `World->GetGameViewport()`.

### 4.5 Subsystem Initialization Order

UE5 creates and initializes subsystems in registration order, which is not guaranteed. The project handles this with:

1. **No cross-subsystem calls in OnWorldBeginPlay** -- each subsystem only sets up its own timer
2. **Timer-based discovery** -- all subsystems poll independently at 500ms intervals
3. **Data requests after wrapping** -- each subsystem emits its own data request (e.g., `player:request_stats`, `inventory:load`) after wrapping completes

The effective initialization order is:
```
0.0s  OnWorldBeginPlay fires for all subsystems
0.0s  Each subsystem starts its 500ms polling timer
0.5s  First poll -- BP_SocketManager may not exist yet (skip)
1.0s  Second poll -- SocketIO component found, but not connected yet (skip)
1.5s  Third poll -- Connected but Blueprint events not bound yet (skip)
2.0s  Fourth poll -- Blueprint events bound, wrapping succeeds
2.0s  Each subsystem requests its initial data from server
2.1s  Server responds with player:stats, inventory:data, etc.
2.1s  Widgets appear with correct data
```

### 4.6 Cross-Subsystem Communication Patterns

Subsystems communicate through:

1. **Shared GameInstance data** -- any subsystem can read `UMMOGameInstance` for auth state, character data, zone info
2. **Direct subsystem access** -- `GetWorld()->GetSubsystem<UOtherSubsystem>()` is safe from any subsystem
3. **Indirect via socket events** -- most communication goes through the server. Subsystem A emits a socket event, server processes it, server broadcasts a result event, Subsystem B receives it.

Example: EquipmentSubsystem reads from InventorySubsystem:

```cpp
// In EquipmentSubsystem, to get the full inventory data:
if (UInventorySubsystem* InvSub = GetWorld()->GetSubsystem<UInventorySubsystem>())
{
    const TArray<FInventoryItem>& AllItems = InvSub->GetItems();
    // Filter for equipped items...
}
```

**Do NOT use delegates for subsystem-to-subsystem communication** unless there is a clear one-to-many relationship. The socket event wrapping pattern already provides the event-driven architecture.

---

## 5. Data Structures

### 5.1 FCharacterData (Complete Struct)

Shared across the entire client. Defined in `CharacterData.h`. Mirrors the server's character data.

```cpp
// File: client/SabriMMO/Source/SabriMMO/CharacterData.h

USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()

    // Identity
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString CharacterClass;   // "novice", "swordman", "mage", etc.

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Level;

    // Position
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float X;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Y;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Z;

    // Vitals
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Health;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 MaxHealth;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Mana;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 MaxMana;

    // EXP and Leveling (RO-style dual progression)
    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 JobLevel;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString JobClass;    // "novice", "knight", "wizard", "lord_knight", etc.

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int64 BaseExp;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int64 JobExp;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 SkillPoints;

    // Base Stats
    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Str;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Agi;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Vit;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 IntStat;    // "Int" is a reserved keyword in many contexts

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Dex;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 Luk;

    UPROPERTY(BlueprintReadWrite, Category = "Stats")
    int32 StatPoints;

    // Economy
    UPROPERTY(BlueprintReadWrite, Category = "Economy")
    int32 Zuzucoin;

    // Appearance
    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    int32 HairStyle;    // 1-19

    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    int32 HairColor;    // 0-8

    UPROPERTY(BlueprintReadWrite, Category = "Appearance")
    FString Gender;     // "male" or "female"

    // Zone
    UPROPERTY(BlueprintReadWrite, Category = "Zone")
    FString ZoneName;   // "prontera", "prontera_south", etc.

    UPROPERTY(BlueprintReadWrite, Category = "Zone")
    FString LevelName;  // "L_Prontera", "L_PrtSouth", etc.

    // Meta
    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString DeleteDate;

    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString CreatedAt;

    UPROPERTY(BlueprintReadWrite, Category = "Meta")
    FString LastPlayed;

    FCharacterData()
    {
        CharacterId = 0;
        Name = TEXT("");
        CharacterClass = TEXT("novice");
        Level = 1;
        X = 0.0f; Y = 0.0f; Z = 0.0f;
        Health = 100; MaxHealth = 100;
        Mana = 100; MaxMana = 100;
        JobLevel = 1;
        JobClass = TEXT("novice");
        BaseExp = 0; JobExp = 0;
        SkillPoints = 0;
        Str = 1; Agi = 1; Vit = 1; IntStat = 1; Dex = 1; Luk = 1;
        StatPoints = 48;
        Zuzucoin = 0;
        HairStyle = 1; HairColor = 0;
        Gender = TEXT("male");
    }
};
```

### 5.2 FInventoryItem

```cpp
USTRUCT(BlueprintType)
struct FInventoryItem
{
    GENERATED_BODY()

    int32 InventoryId = 0;
    int32 ItemId = 0;
    FString Name;
    FString Description;
    FString ItemType;           // weapon, armor, consumable, card
    FString EquipSlot;          // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory
    int32 Quantity = 1;
    bool bIsEquipped = false;
    FString EquippedPosition;   // weapon, armor, shield, head_top, head_mid, head_low, footgear, garment, accessory_1, accessory_2
    int32 SlotIndex = -1;       // Position in inventory grid (-1 = auto)
    int32 Weight = 0;
    int32 Price = 0;
    int32 ATK = 0;
    int32 DEF = 0;
    int32 MATK = 0;
    int32 MDEF = 0;
    int32 StrBonus = 0;
    int32 AgiBonus = 0;
    int32 VitBonus = 0;
    int32 IntBonus = 0;
    int32 DexBonus = 0;
    int32 LukBonus = 0;
    int32 MaxHPBonus = 0;
    int32 MaxSPBonus = 0;
    int32 RequiredLevel = 1;
    bool bStackable = false;
    int32 MaxStack = 1;
    FString Icon;
    FString WeaponType;         // dagger, one_hand_sword, bow, mace, staff, spear, axe, whip, instrument
    int32 ASPDModifier = 0;
    int32 WeaponRange = 150;

    bool IsValid() const { return InventoryId > 0; }
    bool IsEquippable() const { return !EquipSlot.IsEmpty(); }
    bool IsConsumable() const { return ItemType == TEXT("consumable"); }
};
```

### 5.3 FServerInfo

```cpp
USTRUCT(BlueprintType)
struct FServerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    int32 ServerId = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    FString Host;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    int32 Port = 3001;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    FString Status;  // "online", "offline", "maintenance"

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    int32 Population = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    int32 MaxPopulation = 1000;

    UPROPERTY(BlueprintReadWrite, Category = "Server")
    FString Region;
};
```

### 5.4 Drag-and-Drop Enums

```cpp
UENUM()
enum class EItemDragSource : uint8
{
    None,
    Inventory,
    Equipment
};

UENUM()
enum class EItemDropTarget : uint8
{
    Cancelled,
    InventorySlot,
    EquipmentSlot,
    EquipmentPortrait,
    GameWorld,
    HotbarSlot
};
```

### 5.5 FDraggedItem

```cpp
USTRUCT()
struct FDraggedItem
{
    GENERATED_BODY()

    int32 InventoryId = 0;
    int32 ItemId = 0;
    FString Name;
    FString ItemType;
    FString EquipSlot;
    FString EquippedPosition;
    FString Icon;
    int32 Quantity = 1;
    bool bIsEquipped = false;
    EItemDragSource Source = EItemDragSource::None;
    int32 SourceSlotIndex = -1;

    bool IsValid() const { return InventoryId > 0; }

    static FDraggedItem FromItem(const FInventoryItem& Item, EItemDragSource InSource)
    {
        FDraggedItem D;
        D.InventoryId = Item.InventoryId;
        D.ItemId = Item.ItemId;
        D.Name = Item.Name;
        D.ItemType = Item.ItemType;
        D.EquipSlot = Item.EquipSlot;
        D.EquippedPosition = Item.EquippedPosition;
        D.Icon = Item.Icon;
        D.Quantity = Item.Quantity;
        D.bIsEquipped = Item.bIsEquipped;
        D.Source = InSource;
        D.SourceSlotIndex = Item.SlotIndex;
        return D;
    }
};
```

### 5.6 FShopItem and FCartItem

```cpp
USTRUCT()
struct FShopItem
{
    GENERATED_BODY()

    int32 ItemId = 0;
    FString Name;
    FString Description;
    FString ItemType;
    FString Icon;
    int32 BuyPrice = 0;
    int32 SellPrice = 0;
    int32 Weight = 0;
    int32 ATK = 0;
    int32 DEF = 0;
    int32 MATK = 0;
    int32 MDEF = 0;
    FString EquipSlot;
    FString WeaponType;
    int32 WeaponRange = 0;
    int32 ASPDModifier = 0;
    int32 RequiredLevel = 1;
    bool bStackable = false;
    int32 StrBonus = 0;
    int32 AgiBonus = 0;
    int32 VitBonus = 0;
    int32 IntBonus = 0;
    int32 DexBonus = 0;
    int32 LukBonus = 0;
    int32 MaxHPBonus = 0;
    int32 MaxSPBonus = 0;

    bool IsValid() const { return ItemId > 0; }
};

USTRUCT()
struct FCartItem
{
    GENERATED_BODY()

    int32 ItemId = 0;
    int32 InventoryId = 0;
    FString Name;
    FString Icon;
    int32 Quantity = 1;
    int32 UnitPrice = 0;
    int32 Weight = 0;

    int32 GetTotalPrice() const { return UnitPrice * Quantity; }
    int32 GetTotalWeight() const { return Weight * Quantity; }
    bool IsValid() const { return ItemId > 0 || InventoryId > 0; }
};
```

### 5.7 Planned Data Structures (Not Yet Implemented)

These structs will be needed for future systems:

```cpp
// ---- Party System ----
USTRUCT(BlueprintType)
struct FPartyMember
{
    GENERATED_BODY()

    int32 CharacterId = 0;
    FString Name;
    FString JobClass;
    int32 BaseLevel = 1;
    int32 CurrentHP = 0;
    int32 MaxHP = 0;
    int32 CurrentSP = 0;
    int32 MaxSP = 0;
    FString ZoneName;
    bool bIsOnline = false;
    bool bIsLeader = false;
};

// ---- Guild System ----
USTRUCT(BlueprintType)
struct FGuildMember
{
    GENERATED_BODY()

    int32 CharacterId = 0;
    FString Name;
    FString JobClass;
    int32 BaseLevel = 1;
    FString Rank;          // "Guild Master", "Officer", "Member"
    int32 RankId = 0;
    bool bIsOnline = false;
    FString LastOnline;    // ISO timestamp
};

// ---- Buff Data ----
USTRUCT()
struct FBuffData
{
    GENERATED_BODY()

    int32 SkillId = 0;
    FString BuffName;
    float RemainingDuration = 0.0f;   // seconds
    float TotalDuration = 0.0f;       // seconds
    FString IconPath;
    bool bIsDebuff = false;
};

// ---- Skill VFX Config (defined in SkillVFXData.h) ----
// See VFX/SkillVFXData.h for FSkillVFXConfig
```

### 5.8 Enums (Current and Planned)

```cpp
// ---- Login Flow States (in LoginFlowSubsystem.h) ----
UENUM()
enum class ELoginFlowState : uint8
{
    Login,
    ServerSelect,
    CharacterSelect,
    CharacterCreate,
    EnteringWorld
};

// ---- Planned Enums (for future systems) ----

UENUM()
enum class ECharacterClass : uint8
{
    Novice,
    Swordman, Mage, Archer, Merchant, Thief, Acolyte,
    Knight, Wizard, Hunter, Blacksmith, Assassin, Priest,
    Crusader, Sage, BardDancer, Alchemist, Rogue, Monk,
    LordKnight, HighWizard, Sniper, Whitesmith, AssassinCross, HighPriest,
    Paladin, Scholar, Minstrel, Biochemist, Stalker, Champion,
    HighNovice, SuperNovice
};

UENUM()
enum class EElement : uint8
{
    Neutral, Water, Earth, Fire, Wind,
    Poison, Holy, Shadow, Ghost, Undead
};

UENUM()
enum class ERace : uint8
{
    Formless, Undead, Brute, Plant, Insect,
    Fish, Demon, DemiHuman, Angel, Dragon
};

UENUM()
enum class ESize : uint8
{
    Small, Medium, Large
};

UENUM()
enum class EStatusEffect : uint8
{
    None,
    Poison, Stun, Freeze, Stone, Sleep, Blind, Silence, Curse, Confusion,
    Bleeding, Fear
};

UENUM()
enum class EWeaponType : uint8
{
    BareHand, Dagger, OneHandSword, TwoHandSword,
    OneHandSpear, TwoHandSpear, OneHandAxe, TwoHandAxe,
    Mace, Staff, Bow, Knuckle, Instrument, Whip, Book, Katar
};
```

---

## 6. GameMode and PlayerController

### 6.1 Why DefaultPawnClass = nullptr

In this project, the **Level Blueprint** handles pawn spawning, not the GameMode. This is because:

1. The spawn location comes from the server (saved position in database)
2. The spawn is conditional (wait for GameInstance data, check zone transition state)
3. Different levels may need different spawn logic (login level has no pawn, game levels do)

If `DefaultPawnClass` were set, UE5 would auto-spawn a pawn at `PlayerStart` on `BeginPlay`, creating a duplicate pawn before the Level Blueprint can spawn the real one.

### 6.2 GM_MMOGameMode Setup

**File**: `client/SabriMMO/Source/SabriMMO/SabriMMOGameMode.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SabriMMOGameMode.generated.h"

UCLASS(abstract)
class ASabriMMOGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASabriMMOGameMode();
};
```

```cpp
// SabriMMOGameMode.cpp
#include "SabriMMOGameMode.h"

ASabriMMOGameMode::ASabriMMOGameMode()
{
    DefaultPawnClass = nullptr;  // Level Blueprint handles spawning
}
```

The Blueprint `GM_MMOGameMode` inherits from `ASabriMMOGameMode`. It does NOT override DefaultPawnClass.

### 6.3 Level Blueprint Pawn Spawning

The Level Blueprint (not C++ code, but documented here for completeness) does:

```
BeginPlay
  -> Delay 0.2s (wait for subsystems to initialize)
  -> Cast To MMOGameInstance
  -> Get Selected Character
  -> Branch: has saved location (X != 0 || Y != 0)?
     Yes -> SpawnActor BP_MMOCharacter at saved (X, Y, Z)
     No  -> SpawnActor BP_MMOCharacter at default spawn
  -> Possess the spawned actor
  -> Set timer: SaveCharacterPosition every 5 seconds
  -> EndPlay: cleanup (destroy pawn reference)
```

### 6.4 PlayerController

The game uses `PC_MMOPlayerController` (a **Blueprint** that inherits directly from `APlayerController`, NOT from `ASabriMMOPlayerController`). The C++ `ASabriMMOPlayerController` class exists but is currently dead code.

Input handling is done in `ASabriMMOCharacter::SetupPlayerInputComponent` using Enhanced Input. The PlayerController does not handle any input directly -- it is used only for pawn possession and input mode configuration.

### 6.5 Enhanced Input System Integration

All input is set up programmatically in C++ -- no Blueprint Input Action or Input Mapping Context assets are needed for the UI toggle keys.

```cpp
// In ASabriMMOCharacter::CreateUIToggleActions()

void ASabriMMOCharacter::CreateUIToggleActions()
{
    // Create a programmatic Input Mapping Context
    UIToggleIMC = NewObject<UInputMappingContext>(this, TEXT("IMC_UIToggles"));

    // F8: Toggle Combat Stats
    ToggleCombatStatsAction = NewObject<UInputAction>(this, TEXT("IA_ToggleCombatStats"));
    ToggleCombatStatsAction->ValueType = EInputActionValueType::Boolean;
    UIToggleIMC->MapKey(ToggleCombatStatsAction, EKeys::F8);

    // F6: Toggle Inventory
    ToggleInventoryAction = NewObject<UInputAction>(this, TEXT("IA_ToggleInventory"));
    ToggleInventoryAction->ValueType = EInputActionValueType::Boolean;
    UIToggleIMC->MapKey(ToggleInventoryAction, EKeys::F6);

    // F7: Toggle Equipment
    ToggleEquipmentAction = NewObject<UInputAction>(this, TEXT("IA_ToggleEquipment"));
    ToggleEquipmentAction->ValueType = EInputActionValueType::Boolean;
    UIToggleIMC->MapKey(ToggleEquipmentAction, EKeys::F7);

    // F9: Toggle Shop
    ToggleShopAction = NewObject<UInputAction>(this, TEXT("IA_ToggleShop"));
    ToggleShopAction->ValueType = EInputActionValueType::Boolean;
    UIToggleIMC->MapKey(ToggleShopAction, EKeys::F9);

    // H: Cycle Hotbar visibility
    CycleHotbarAction = NewObject<UInputAction>(this, TEXT("IA_CycleHotbar"));
    CycleHotbarAction->ValueType = EInputActionValueType::Boolean;
    UIToggleIMC->MapKey(CycleHotbarAction, EKeys::H);

    // 1-9: Hotbar slot keys
    static const EKeys::Type NumKeys[] = {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
        EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine
    };
    for (int32 i = 0; i < 9; ++i)
    {
        FString ActionName = FString::Printf(TEXT("IA_HotbarSlot%d"), i + 1);
        HotbarSlotActions[i] = NewObject<UInputAction>(this, *ActionName);
        HotbarSlotActions[i]->ValueType = EInputActionValueType::Boolean;
        UIToggleIMC->MapKey(HotbarSlotActions[i], NumKeys[i]);
    }
}
```

The key bindings are registered in `SetupPlayerInputComponent`:

```cpp
void ASabriMMOCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC) return;

    // Movement and camera (from Blueprint IMC_MMOCharacter)
    EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
    EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Move);
    EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);
    EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASabriMMOCharacter::Look);

    // Create and register UI toggle actions
    CreateUIToggleActions();
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (UIToggleIMC)
                Sub->AddMappingContext(UIToggleIMC, 1); // Priority 1 (below movement)
        }
    }

    // Bind the actions
    EIC->BindAction(ToggleCombatStatsAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleCombatStats);
    EIC->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleInventory);
    EIC->BindAction(ToggleEquipmentAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleEquipment);
    EIC->BindAction(ToggleShopAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleToggleShop);
    EIC->BindAction(CycleHotbarAction, ETriggerEvent::Started, this, &ASabriMMOCharacter::HandleCycleHotbar);

    // Hotbar slots 1-9
    typedef void (ASabriMMOCharacter::*SlotHandler)();
    static const SlotHandler SlotHandlers[] = {
        &ASabriMMOCharacter::HandleHotbarSlot1, &ASabriMMOCharacter::HandleHotbarSlot2,
        &ASabriMMOCharacter::HandleHotbarSlot3, &ASabriMMOCharacter::HandleHotbarSlot4,
        &ASabriMMOCharacter::HandleHotbarSlot5, &ASabriMMOCharacter::HandleHotbarSlot6,
        &ASabriMMOCharacter::HandleHotbarSlot7, &ASabriMMOCharacter::HandleHotbarSlot8,
        &ASabriMMOCharacter::HandleHotbarSlot9
    };
    for (int32 i = 0; i < 9; ++i)
    {
        if (HotbarSlotActions[i])
            EIC->BindAction(HotbarSlotActions[i], ETriggerEvent::Started, this, SlotHandlers[i]);
    }
}
```

The handler functions route to the appropriate subsystem:

```cpp
void ASabriMMOCharacter::HandleToggleCombatStats()
{
    if (UWorld* W = GetWorld())
    {
        if (UCombatStatsSubsystem* Sub = W->GetSubsystem<UCombatStatsSubsystem>())
        {
            Sub->ToggleWidget();
        }
    }
}

void ASabriMMOCharacter::HandleHotbarSlotInternal(int32 KeyNumber)
{
    if (UWorld* W = GetWorld())
    {
        if (UHotbarSubsystem* HB = W->GetSubsystem<UHotbarSubsystem>())
        {
            HB->ActivateSlotByKeybind(KeyNumber);
        }
    }
}
```

---

## 7. Character Actor (ASabriMMOCharacter)

### 7.1 Base Pawn Class Design

`ASabriMMOCharacter` inherits from `ACharacter` (which includes `UCharacterMovementComponent`, `UCapsuleComponent`, and `USkeletalMeshComponent`). It is declared `abstract` because the game uses `BP_MMOCharacter` (a Blueprint child) as the actual pawn.

### 7.2 Complete Header

```cpp
// SabriMMOCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SabriMMOCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UHotbarSubsystem;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class ASabriMMOCharacter : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

protected:
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

public:
    ASabriMMOCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

private:
    // ---- Programmatic UI toggle input ----
    UPROPERTY() UInputMappingContext* UIToggleIMC = nullptr;
    UPROPERTY() UInputAction* ToggleCombatStatsAction = nullptr;
    UPROPERTY() UInputAction* ToggleInventoryAction = nullptr;
    UPROPERTY() UInputAction* ToggleEquipmentAction = nullptr;
    UPROPERTY() UInputAction* ToggleShopAction = nullptr;
    UPROPERTY() UInputAction* CycleHotbarAction = nullptr;
    UPROPERTY() UInputAction* HotbarSlotActions[9];

    void CreateUIToggleActions();
    void HandleToggleCombatStats();
    void HandleToggleInventory();
    void HandleToggleEquipment();
    void HandleToggleShop();
    void HandleCycleHotbar();
    void HandleHotbarSlot1();
    // ... through HandleHotbarSlot9()
    void HandleHotbarSlotInternal(int32 KeyNumber);

public:
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

    UFUNCTION(BlueprintCallable, Category="NPC Interaction")
    bool TryInteractWithNPC(AActor* HitActor);

    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
```

### 7.3 Constructor Defaults

```cpp
ASabriMMOCharacter::ASabriMMOCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Controller rotation does NOT rotate the character
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Character rotates to face movement direction
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // Follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}
```

### 7.4 BeginPlay -- Zone Transition Teleport

```cpp
void ASabriMMOCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Set Game+UI input mode so Slate widgets receive mouse events
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);
        PC->SetShowMouseCursor(true);
    }

    // Teleport to DB-loaded position during zone transitions / initial login
    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(GetGameInstance()))
    {
        if (GI->bIsZoneTransitioning && !GI->PendingSpawnLocation.IsNearlyZero())
        {
            SetActorLocation(GI->PendingSpawnLocation);
        }
    }
}
```

### 7.5 NPC Interaction

```cpp
bool ASabriMMOCharacter::TryInteractWithNPC(AActor* HitActor)
{
    if (!HitActor) return false;

    // Shop NPC
    if (AShopNPC* Shop = Cast<AShopNPC>(HitActor))
    {
        if (UShopSubsystem* ShopSub = GetWorld()->GetSubsystem<UShopSubsystem>())
        {
            ShopSub->RequestOpenShop(Shop->ShopId);
            return true;
        }
    }

    // Kafra NPC
    if (AKafraNPC* Kafra = Cast<AKafraNPC>(HitActor))
    {
        if (UKafraSubsystem* KafraSub = GetWorld()->GetSubsystem<UKafraSubsystem>())
        {
            KafraSub->RequestOpenKafra(Kafra->KafraId);
            return true;
        }
    }

    return false;
}
```

### 7.6 Socket.io Event Binding

The character actor itself does NOT bind socket events. All socket event handling is done by subsystems. The character is purely presentation + input. This follows the server-authoritative model: the character pawn is a visual representation, not the source of truth for any game state.

### 7.7 Equipment Visual Attachment Points (Future)

Equipment visuals will be handled by a dedicated `UEquipmentVisualComponent` (not yet implemented). Design:

```cpp
// Future: EquipmentVisualComponent.h
UCLASS()
class UEquipmentVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    void SetWeaponMesh(UStaticMesh* Mesh);
    void SetShieldMesh(UStaticMesh* Mesh);
    void SetHeadgearMesh(int32 HeadSlot, UStaticMesh* Mesh); // 0=top, 1=mid, 2=low

private:
    UPROPERTY()
    UStaticMeshComponent* WeaponMeshComp;

    UPROPERTY()
    UStaticMeshComponent* ShieldMeshComp;

    UPROPERTY()
    UStaticMeshComponent* HeadTopMeshComp;

    UPROPERTY()
    UStaticMeshComponent* HeadMidMeshComp;

    UPROPERTY()
    UStaticMeshComponent* HeadLowMeshComp;
};
```

---

## 8. Server Communication Protocol

### 8.1 Event Naming Conventions

All socket events follow the pattern `domain:action`:

| Domain | Events |
|--------|--------|
| `player` | `join`, `joined`, `position`, `moved`, `left`, `stats`, `request_stats`, `allocate_stat`, `teleport` |
| `combat` | `attack`, `stop_attack`, `damage`, `health_update`, `death`, `out_of_range`, `target_lost`, `error` |
| `skill` | `use`, `used`, `cast_start`, `cast_complete`, `cast_interrupted`, `effect_damage`, `buff_applied`, `buff_removed`, `ground_effect_placed`, `ground_effect_removed`, `data`, `learn`, `learned`, `error` |
| `inventory` | `load`, `data`, `use`, `used`, `equip`, `equipped`, `drop`, `dropped`, `move`, `error` |
| `enemy` | `spawn`, `move`, `attack`, `health_update`, `death` |
| `zone` | `warp`, `change`, `ready`, `data`, `error` |
| `kafra` | `open`, `data`, `save`, `saved`, `teleport`, `teleported`, `error` |
| `shop` | `open`, `data`, `buy`, `buy_batch`, `bought`, `sell`, `sell_batch`, `sold`, `error` |
| `chat` | `message`, `receive` |
| `exp` | `gain`, `level_up` |
| `job` | `change`, `changed`, `error` |
| `hotbar` | `request`, `alldata`, `save`, `save_skill`, `clear` |
| `loot` | `drop` |

### 8.2 Payload Formats

**Important distinction**: `combat:damage` is emitted ONLY for auto-attacks. `skill:effect_damage` is emitted for ALL skills. Client subsystems that need to show damage from any source must listen to BOTH events.

**player:stats payload** (the most complex payload):

```json
{
  "characterId": 123,
  "stats": {
    "str": 50, "agi": 30, "vit": 40, "int": 10, "dex": 60, "luk": 20,
    "level": 75, "statPoints": 42,
    "strCost": 7, "agiCost": 5, "vitCost": 6, "intCost": 3, "dexCost": 8, "lukCost": 4,
    "statusATK": 85, "equipATK": 120, "weaponATK": 95,
    "statusMATK": 15, "minMATK": 12, "maxMATK": 18,
    "hit": 135, "flee": 95, "criticalRate": 8, "perfectDodge": 2,
    "softDEF": 45, "hardDEF": 30, "softMDEF": 20,
    "aspd": 175, "attackInterval": 1250,
    "maxHP": 5500, "maxSP": 200,
    "equipBonuses": {
      "str": 5, "agi": 0, "vit": 3, "int": 0, "dex": 2, "luk": 0,
      "maxHp": 100, "maxSp": 50, "hit": 10, "flee": 5, "critical": 3
    }
  },
  "exp": {
    "baseLevel": 75, "jobLevel": 42, "jobClass": "knight",
    "baseExp": 1234567, "baseExpNext": 2230113, "baseExpPercent": 55.4,
    "jobExp": 98765, "jobExpNext": 204351, "jobExpPercent": 48.3,
    "skillPoints": 5
  },
  "health": 4200, "maxHealth": 5500,
  "mana": 180, "maxMana": 200,
  "attackRange": 150
}
```

### 8.3 Request/Response Patterns

**Client-initiated action** (e.g., use a skill):

```
Client                          Server
  |                               |
  |--- skill:use { skillId: 210,  |
  |    targetId: 500,             |
  |    isEnemy: true }            |
  |------------------------------>|
  |                               | Validate: skill learned? SP sufficient?
  |                               | Cooldown? Target exists? In range?
  |                               |
  |<--- skill:cast_start ---------|  (broadcast to zone)
  |     { casterId, skillName,    |
  |       castTime, ... }         |
  |                               |  ... cast time elapses ...
  |                               |
  |<--- skill:cast_complete ------|  (broadcast to zone)
  |<--- skill:used ---------------|  (to caster only: SP cost, remaining SP)
  |<--- skill:effect_damage ------|  (broadcast to zone, per hit)
  |<--- combat:health_update -----|  (broadcast to zone, updated HP)
```

**Server-pushed update** (e.g., enemy AI attacks player):

```
Server (Enemy AI tick)          Client
  |                               |
  | Enemy attacks player          |
  |--- enemy:attack ------------->|  (broadcast to zone)
  |--- combat:health_update ----->|  (broadcast to zone)
  |                               |
```

### 8.4 Error Handling

Every domain has an `error` event. The client should display these as temporary notifications:

```cpp
void UMySubsystem::HandleSkillError(const TSharedPtr<FJsonValue>& Data)
{
    if (!Data.IsValid()) return;
    const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
    if (!Data->TryGetObject(ObjPtr) || !ObjPtr) return;

    FString Message;
    (*ObjPtr)->TryGetStringField(TEXT("message"), Message);

    UE_LOG(LogMySubsystem, Warning, TEXT("Skill error: %s"), *Message);
    // TODO: show in-game notification
}
```

### 8.5 Emitting Events from C++

```cpp
// Emit a simple event with JSON payload
if (CachedSIOComponent.IsValid())
{
    // Method 1: String payload (simple)
    CachedSIOComponent->EmitNative(TEXT("player:request_stats"), TEXT("{}"));

    // Method 2: FJsonObject payload (complex)
    TSharedPtr<FJsonObject> Payload = MakeShareable(new FJsonObject());
    Payload->SetNumberField(TEXT("skillId"), 210);
    Payload->SetNumberField(TEXT("targetId"), 500);
    Payload->SetBoolField(TEXT("isEnemy"), true);

    FString PayloadStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadStr);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    CachedSIOComponent->EmitNative(TEXT("skill:use"), PayloadStr);

    // Method 3: Using SIOJson helpers (if available)
    auto SIOJsonObj = USIOJsonObject::ConstructJsonObject(GetWorld());
    SIOJsonObj->SetNumberField(TEXT("warpId"), WarpId);
    CachedSIOComponent->Emit(TEXT("zone:warp"), SIOJsonObj);
}
```

---

## 9. Configuration System

### 9.1 Game Settings Locations

| Setting | Storage | File | Read by |
|---------|---------|------|---------|
| Server URL | `UMMOGameInstance::ServerBaseUrl` | Runtime (default: `http://localhost:3001`) | HTTP Manager, SocketIO |
| Remember Username | `GConfig` | `{GeneratedConfigDir}/SabriMMO.ini` | LoginFlowSubsystem |
| Remembered Username | `GConfig` | `{GeneratedConfigDir}/SabriMMO.ini` | LoginFlowSubsystem |
| Hotbar Keybinds | `GConfig` | `{GeneratedConfigDir}/GameUserSettings.ini` | HotbarSubsystem |
| Window size, fullscreen | Engine config | `GameUserSettings.ini` | Engine default |
| Audio volumes | Engine config | `GameUserSettings.ini` | Engine default |

### 9.2 Config File Access Pattern

```cpp
// Writing
const FString IniPath = FPaths::GeneratedConfigDir() + TEXT("SabriMMO.ini");
GConfig->SetBool(TEXT("SectionName"), TEXT("KeyName"), bValue, IniPath);
GConfig->SetString(TEXT("SectionName"), TEXT("KeyName"), *StringValue, IniPath);
GConfig->SetInt(TEXT("SectionName"), TEXT("KeyName"), IntValue, IniPath);
GConfig->Flush(false, IniPath);  // Write to disk

// Reading
bool bValue = false;
GConfig->GetBool(TEXT("SectionName"), TEXT("KeyName"), bValue, IniPath);

FString StringValue;
GConfig->GetString(TEXT("SectionName"), TEXT("KeyName"), StringValue, IniPath);

int32 IntValue = 0;
GConfig->GetInt(TEXT("SectionName"), TEXT("KeyName"), IntValue, IniPath);
```

### 9.3 Hotbar Keybind Storage

Hotbar keybinds are stored in `GameUserSettings.ini` under the `[HotbarKeybinds]` section:

```ini
[HotbarKeybinds]
Row0_Slot0=One
Row0_Slot1=Two
Row0_Slot2=Three
; ...
Row1_Slot0=F1
Row1_Slot1=F2
; ...
```

Default keybinds (applied if no config exists):
- Row 0: Keys 1-9
- Row 1: Alt+1 through Alt+9
- Rows 2-3: Unbound

### 9.4 Server URL Configuration

The server URL can be configured in several ways:

1. **Default**: `http://localhost:3001` (hardcoded in `UMMOGameInstance`)
2. **Server selection**: When the user picks a server from the server list, `SelectServer()` constructs the URL from the server's `Host` and `Port` fields
3. **Manual override**: Set `UMMOGameInstance::ServerBaseUrl` directly (e.g., from a settings screen or command-line argument)

```cpp
// In SelectServer:
void UMMOGameInstance::SelectServer(const FServerInfo& Server)
{
    SelectedServer = Server;
    ServerBaseUrl = FString::Printf(TEXT("http://%s:%d"), *Server.Host, Server.Port);
}

// HTTP Manager reads the URL:
FString UHttpManager::GetServerBaseUrl(UObject* WorldContextObject)
{
    UMMOGameInstance* GI = GetGameInstance(WorldContextObject);
    return GI ? GI->ServerBaseUrl : TEXT("http://localhost:3001");
}
```

---

## Appendix A: Complete File Inventory

| File Path | Lines | Purpose |
|-----------|-------|---------|
| `SabriMMO/CharacterData.h` | ~360 | All shared data structs |
| `SabriMMO/MMOGameInstance.h` | ~160 | Cross-level state |
| `SabriMMO/MMOGameInstance.cpp` | ~150 | GameInstance implementation |
| `SabriMMO/MMOHttpManager.h` | ~65 | REST API declarations |
| `SabriMMO/MMOHttpManager.cpp` | ~400 | HTTP request implementations |
| `SabriMMO/SabriMMOCharacter.h` | ~150 | Base pawn header |
| `SabriMMO/SabriMMOCharacter.cpp` | ~300 | Pawn implementation + input |
| `SabriMMO/SabriMMOGameMode.h` | ~25 | GameMode header |
| `SabriMMO/SabriMMOGameMode.cpp` | ~15 | DefaultPawnClass = nullptr |
| `SabriMMO/UI/BasicInfoSubsystem.h` | ~90 | HUD subsystem header |
| `SabriMMO/UI/BasicInfoSubsystem.cpp` | ~500 | HUD implementation |
| `SabriMMO/UI/LoginFlowSubsystem.h` | ~120 | Login flow header |
| `SabriMMO/UI/ZoneTransitionSubsystem.h` | ~75 | Zone transition header |
| `SabriMMO/VFX/SkillVFXSubsystem.h` | ~175 | VFX subsystem header |

## Appendix B: Widget Z-Order Convention

| Z-Order | Widget | Subsystem |
|---------|--------|-----------|
| 5 | Login flow widgets | LoginFlowSubsystem |
| 8 | World health bars | WorldHealthBarSubsystem |
| 10 | Basic info (HP/SP/EXP) | BasicInfoSubsystem |
| 12 | Combat stats | CombatStatsSubsystem |
| 14 | Inventory | InventorySubsystem |
| 15 | Equipment | EquipmentSubsystem |
| 16 | Hotbar rows | HotbarSubsystem |
| 19 | Kafra dialog | KafraSubsystem |
| 20 | Skill tree | SkillTreeSubsystem |
| 20 | Damage numbers | DamageNumberSubsystem |
| 25 | Cast bar | CastBarSubsystem |
| 30 | Hotbar keybind config | HotbarSubsystem |
| 50 | Loading overlay | ZoneTransitionSubsystem / LoginFlowSubsystem |

## Appendix C: Multiplayer Safety Checklist

Before committing any new subsystem or widget code, verify:

- [ ] **No `GEngine->GameViewport`** -- use `World->GetGameViewport()` instead
- [ ] **No `GEngine->AddOnScreenDebugMessage`** for per-player feedback -- use `UE_LOG` or per-world widgets
- [ ] **All world access through `GetWorld()`** -- never through globals or singletons
- [ ] **Subsystem inherits from `UWorldSubsystem`** -- not `UGameInstanceSubsystem`
- [ ] **ShouldCreateSubsystem checks `IsGameWorld()`** -- prevents creation in editor or preview
- [ ] **Widget added via `World->GetGameViewport()->AddViewportWidgetContent()`** -- not via global viewport
- [ ] **Tested with 2+ PIE instances** -- verify no cross-client state bleed
