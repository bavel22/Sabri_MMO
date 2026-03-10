# UHttpManager (MMOHttpManager)

**Files**: `Source/SabriMMO/MMOHttpManager.h` (61 lines), `MMOHttpManager.cpp` (672 lines)
**Parent**: `UBlueprintFunctionLibrary`
**Purpose**: Static HTTP API client for REST communication with the Node.js server, plus bridge functions for Blueprint→Subsystem calls. All functions are `BlueprintCallable` with `WorldContext` meta.

**Note**: The file is named `MMOHttpManager` to avoid collision with UE5's built-in `HttpManager.h` in unity builds. The class name `UHttpManager` is unchanged.

## Server URL

The URL is **configurable**, resolved per-call via `GetServerBaseUrl()`:

```cpp
FString UHttpManager::GetServerBaseUrl(UObject* WorldContextObject)
{
    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
        return GI->ServerBaseUrl;   // Reads from UMMOGameInstance property
    return TEXT("http://localhost:3001");  // Null-safety fallback only
}
```

`UMMOGameInstance::ServerBaseUrl` defaults to `"http://localhost:3001"` and is updated by `SelectServer()` when a server is chosen from the server list.

## Helper Functions (4, all private static)

| Function | Purpose |
|----------|---------|
| `GetGameInstance(WorldContextObject)` | Resolves `UMMOGameInstance` via `UGameplayStatics::GetGameInstance()` |
| `GetServerBaseUrl(WorldContextObject)` | Returns `GI->ServerBaseUrl` or fallback URL |
| `ExtractErrorMessage(ResponseContent, DefaultMessage)` | Parses `"error"` field from JSON response body |
| `ParseCharacterFromJson(JsonObject)` | File-scope free function — maps JSON → `FCharacterData` (32 fields) |

## HTTP Functions (9)

### TestServerConnection
```cpp
static void TestServerConnection(UObject* WorldContextObject);
```
Alias — calls `HealthCheck()` internally.

### HealthCheck
- **Endpoint**: `GET {ServerBaseUrl}/health`
- **Auth**: No
- **Success (200)**: Logs "Server is ONLINE"
- **Delegates**: None

### RegisterUser
```cpp
static void RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password);
```
- **Endpoint**: `POST {ServerBaseUrl}/api/auth/register`
- **Auth**: No
- **Payload**: `{ "username", "email", "password" }`
- **Success (201)**: Parses token + user data → `GI->SetAuthData()` (auto-login after registration)
- **Error**: `GI->OnLoginFailedWithReason(error)`

### LoginUser
```cpp
static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);
```
- **Endpoint**: `POST {ServerBaseUrl}/api/auth/login`
- **Auth**: No
- **Payload**: `{ "username", "password" }`
- **Success (200)**: Parses token, username, user_id → `GI->SetAuthData()`
- **Connection fail**: `GI->OnLoginFailed` + `GI->OnLoginFailedWithReason("Cannot connect to server")`
- **Error**: `GI->OnLoginFailed` + `GI->OnLoginFailedWithReason(error)`

### GetServerList
```cpp
static void GetServerList(UObject* WorldContextObject);
```
- **Endpoint**: `GET {ServerBaseUrl}/api/servers`
- **Auth**: No
- **Success (200)**: Parses `servers` array → `TArray<FServerInfo>` → `GI->SetServerList()`
- **Error**: Logs error, no delegate

### GetCharacters
```cpp
static void GetCharacters(UObject* WorldContextObject);
```
- **Endpoint**: `GET {ServerBaseUrl}/api/characters`
- **Auth**: Required (`Authorization: {GI->GetAuthHeader()}`), pre-checks `IsAuthenticated()`
- **Success (200)**: Parses `characters` array via `ParseCharacterFromJson()` → `GI->SetCharacterList()`

### CreateCharacter
```cpp
static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass, int32 HairStyle = 1, int32 HairColor = 0, const FString& Gender = TEXT("male"));
```
- **Endpoint**: `POST {ServerBaseUrl}/api/characters`
- **Auth**: Required
- **Payload**: `{ "name", "characterClass", "hairStyle", "hairColor", "gender" }`
- **Success (201)**: `GI->OnCharacterCreated`
- **Error**: `GI->OnCharacterCreateFailed(error)`

### DeleteCharacter
```cpp
static void DeleteCharacter(UObject* WorldContextObject, int32 CharacterId, const FString& Password);
```
- **Endpoint**: `DELETE {ServerBaseUrl}/api/characters/{CharacterId}`
- **Auth**: Required
- **Payload**: `{ "password" }` (server-side bcrypt verification)
- **Success (200)**: `GI->OnCharacterDeleteSuccess(CharacterName)`
- **Error**: `GI->OnCharacterDeleteFailed(error)`

### SaveCharacterPosition
```cpp
static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);
```
- **Endpoint**: `PUT {ServerBaseUrl}/api/characters/{CharacterId}/position`
- **Auth**: Conditional (sets header if authenticated, but no pre-check)
- **Payload**: `{ "x", "y", "z" }`
- **Delegates**: None (silent)

## Bridge Functions (2, not HTTP calls)

These exist so Blueprints can call subsystem methods without C++ casts.

### UseSkillWithTargeting
```cpp
static void UseSkillWithTargeting(UObject* WorldContextObject, int32 SkillId);
```
- **Category**: "Skills"
- Gets `USkillTreeSubsystem` from world, calls `Sub->UseSkill(SkillId)`

### ToggleCombatStatsWidget
```cpp
static void ToggleCombatStatsWidget(UObject* WorldContextObject);
```
- **Category**: "UI"
- Gets `UCombatStatsSubsystem` from world, calls `Sub->ToggleWidget()`

## Summary Table

| # | Function | Method | Endpoint | Auth | Delegates |
|---|----------|--------|----------|------|-----------|
| 1 | TestServerConnection | — | (alias) | No | — |
| 2 | HealthCheck | GET | `/health` | No | None |
| 3 | RegisterUser | POST | `/api/auth/register` | No | SetAuthData / OnLoginFailedWithReason |
| 4 | LoginUser | POST | `/api/auth/login` | No | SetAuthData / OnLoginFailed + OnLoginFailedWithReason |
| 5 | GetServerList | GET | `/api/servers` | No | SetServerList |
| 6 | GetCharacters | GET | `/api/characters` | Yes | SetCharacterList |
| 7 | CreateCharacter | POST | `/api/characters` | Yes | OnCharacterCreated / OnCharacterCreateFailed |
| 8 | DeleteCharacter | DELETE | `/api/characters/{id}` | Yes | OnCharacterDeleteSuccess / OnCharacterDeleteFailed |
| 9 | SaveCharacterPosition | PUT | `/api/characters/{id}/position` | Conditional | None |

## JSON Parsing

Uses UE5 `FJsonObject` / `FJsonSerializer` throughout. The `ParseCharacterFromJson()` free function is the single source of truth for mapping server character JSON to `FCharacterData`. Error extraction uses `ExtractErrorMessage()` to pull the `"error"` field from server JSON responses.

---

**Last Updated**: 2026-03-09
