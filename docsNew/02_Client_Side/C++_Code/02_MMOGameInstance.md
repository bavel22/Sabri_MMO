# UMMOGameInstance

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Module_Overview](00_Module_Overview.md) | [CharacterData](01_CharacterData.md) | [Authentication_Flow](../../04_Integration/Authentication_Flow.md)

**Files**: `Source/SabriMMO/MMOGameInstance.h` (157 lines), `MMOGameInstance.cpp` (149 lines)
**Parent**: `UGameInstance`
**Purpose**: Persists authentication state, character data, server selection, zone transition state, and event dispatchers across level loads. Central data hub for the MMO client.

## Delegate Declarations (9)

| Delegate | Parameters | Used By |
|----------|------------|---------|
| `FOnLoginSuccess` | none | `OnLoginSuccess` — broadcast from `SetAuthData()` |
| `FOnLoginFailed` | none | `OnLoginFailed` — legacy, broadcast by `MMOHttpManager` |
| `FOnCharacterCreated` | none | `OnCharacterCreated` — broadcast by `MMOHttpManager` |
| `FOnCharacterListReceived` | none | `OnCharacterListReceived` — broadcast from `SetCharacterList()` |
| `FOnLoginFailedWithReason` | `const FString& ErrorMessage` | `OnLoginFailedWithReason` — broadcast by `MMOHttpManager` |
| `FOnServerListReceived` | `const TArray<FServerInfo>& Servers` | `OnServerListReceived` — broadcast from `SetServerList()` |
| `FOnCharacterCreateFailed` | `const FString& ErrorMessage` | `OnCharacterCreateFailed` — broadcast by `MMOHttpManager` |
| `FOnCharacterDeleteSuccess` | `const FString& CharacterName` | `OnCharacterDeleteSuccess` — broadcast by `MMOHttpManager` |
| `FOnCharacterDeleteFailed` | `const FString& ErrorMessage` | `OnCharacterDeleteFailed` — broadcast by `MMOHttpManager` |

**Note**: Only `OnLoginSuccess`, `OnCharacterListReceived`, and `OnServerListReceived` are broadcast from within MMOGameInstance. The rest are broadcast by external systems (`MMOHttpManager`).

## Properties

### Authentication (Category: "MMO Auth")

| Property | Type | Access | Description |
|----------|------|--------|-------------|
| `AuthToken` | FString | BlueprintReadOnly | JWT token from server |
| `Username` | FString | BlueprintReadOnly | Logged-in username |
| `UserId` | int32 | BlueprintReadOnly | Server user ID |
| `bIsLoggedIn` | bool | BlueprintReadOnly | Whether user is authenticated |
| `CharacterList` | TArray\<FCharacterData\> | BlueprintReadOnly | All characters for this user (max 9) |

### Server Selection (Category: "MMO Server")

| Property | Type | Access | Default | Description |
|----------|------|--------|---------|-------------|
| `SelectedServer` | FServerInfo | BlueprintReadOnly | — | Currently selected server |
| `ServerList` | TArray\<FServerInfo\> | BlueprintReadOnly | — | Available servers from `/api/servers` |
| `ServerBaseUrl` | FString | BlueprintReadWrite | `"http://localhost:3001"` | Configurable server URL, used by all HTTP calls |

### Settings (Category: "MMO Settings")

| Property | Type | Access | Default | Description |
|----------|------|--------|---------|-------------|
| `bRememberUsername` | bool | BlueprintReadWrite | false | Whether to persist username across sessions |
| `RememberedUsername` | FString | BlueprintReadWrite | "" | Persisted username |

### Zone System (Category: "MMO Zone")

| Property | Type | Access | Default | Description |
|----------|------|--------|---------|-------------|
| `CurrentZoneName` | FString | BlueprintReadWrite | `"prontera_south"` | Zone the player is currently in |
| `PendingZoneName` | FString | BlueprintReadWrite | "" | Destination zone during transition |
| `PendingLevelName` | FString | BlueprintReadWrite | "" | UE level name to load (e.g. "L_PrtSouth") |
| `PendingSpawnLocation` | FVector | BlueprintReadWrite | ZeroVector | Spawn coordinates after level load |
| `bIsZoneTransitioning` | bool | BlueprintReadWrite | false | True during zone transition (checked by `ASabriMMOCharacter::BeginPlay`) |

### Private

| Property | Type | Description |
|----------|------|-------------|
| `SelectedCharacterId` | int32 | Currently selected character ID |
| `SelectedCharacter` | FCharacterData | Full data of selected character |

## Event Dispatchers (Category: "MMO Events")

| Dispatcher | Type | Fired When |
|------------|------|------------|
| `OnLoginSuccess` | FOnLoginSuccess | `SetAuthData()` called |
| `OnLoginFailed` | FOnLoginFailed | Login failed (legacy, no error message) |
| `OnCharacterCreated` | FOnCharacterCreated | Character creation succeeded |
| `OnCharacterListReceived` | FOnCharacterListReceived | `SetCharacterList()` called |
| `OnLoginFailedWithReason` | FOnLoginFailedWithReason | Login failed (with error message) |
| `OnServerListReceived` | FOnServerListReceived | `SetServerList()` called (passes server array) |
| `OnCharacterCreateFailed` | FOnCharacterCreateFailed | Character creation failed (with error) |
| `OnCharacterDeleteSuccess` | FOnCharacterDeleteSuccess | Character deleted (with name) |
| `OnCharacterDeleteFailed` | FOnCharacterDeleteFailed | Character deletion failed (with error) |

## Functions (14)

### Auth Management

| Function | Specifier | Description |
|----------|-----------|-------------|
| `SetAuthData(Token, Username, UserId)` | BlueprintCallable | Stores credentials, sets `bIsLoggedIn = true`, broadcasts `OnLoginSuccess` |
| `ClearAuthData()` | BlueprintCallable | Resets all auth fields, clears character list and selection |
| `IsAuthenticated()` | BlueprintPure | Returns `bIsLoggedIn && !AuthToken.IsEmpty()` |
| `GetAuthHeader()` | BlueprintPure | Returns `"Bearer " + AuthToken` or empty string |
| `Logout()` | BlueprintCallable | Saves remembered username, clears auth data, clears server selection |

### Character Management

| Function | Specifier | Description |
|----------|-----------|-------------|
| `SetCharacterList(Characters)` | BlueprintCallable | Stores array, broadcasts `OnCharacterListReceived` |
| `SelectCharacter(CharacterId)` | BlueprintCallable | Finds character by ID in list, copies to `SelectedCharacter` |
| `GetSelectedCharacter()` | BlueprintPure | Returns copy of selected character data |

### Server Management

| Function | Specifier | Description |
|----------|-----------|-------------|
| `SetServerList(Servers)` | BlueprintCallable | Stores array, broadcasts `OnServerListReceived(Servers)` |
| `SelectServer(Server)` | BlueprintCallable | Stores selection, rebuilds `ServerBaseUrl` as `http://{host}:{port}` |
| `GetServerSocketUrl()` | BlueprintPure | Returns selected server URL or falls back to `ServerBaseUrl` |

### Settings

| Function | Specifier | Description |
|----------|-----------|-------------|
| `SaveRememberedUsername()` | BlueprintCallable | Persists to `{ConfigDir}/SabriMMO.ini` under `[LoginSettings]` |
| `LoadRememberedUsername()` | BlueprintCallable | Reads from ini file |

### Lifecycle

| Function | Description |
|----------|-------------|
| `Init()` (override) | Calls `Super::Init()`, then `LoadRememberedUsername()` |

## Design Patterns

- **Game Instance Pattern**: Survives level transitions — essential for zone changes
- **Event-Driven**: 9 delegates notify UI subsystems of state changes
- **Configurable URL**: `ServerBaseUrl` updated by `SelectServer()`, used by all `MMOHttpManager` calls
- **Remember Username**: Persisted via `GConfig` to `SabriMMO.ini`, loaded automatically on `Init()`
- **Zone State**: 5 properties track zone transitions, consumed by `ZoneTransitionSubsystem` and `LoginFlowSubsystem`

---

**Last Updated**: 2026-03-09
