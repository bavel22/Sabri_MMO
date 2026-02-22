# UHttpManager (MMOHttpManager)

**Files**: `Source/SabriMMO/MMOHttpManager.h` (53 lines), `MMOHttpManager.cpp` (537 lines)  
**Parent**: `UBlueprintFunctionLibrary`  
**Purpose**: Static HTTP API client for REST communication with the Node.js server. All functions are `BlueprintCallable` with `WorldContext` meta for Blueprint usage.

**Note**: The file is named `MMOHttpManager` to avoid collision with UE5's built-in `HttpManager.h` in unity builds. The class name `UHttpManager` is unchanged.

## Server URL

All requests target `http://localhost:3001` (hardcoded in implementation).

## Functions

### TestServerConnection
```cpp
static void TestServerConnection(UObject* WorldContextObject);
```
Calls `HealthCheck()` internally. Convenience wrapper.

### HealthCheck
```cpp
static void HealthCheck(UObject* WorldContextObject);
```
- **Endpoint**: `GET /health`
- **Response Handler**: `OnHealthCheckResponse`
- **Success (200)**: Logs "Server is ONLINE and connected to database!"
- **Failure**: Logs connection error

### RegisterUser
```cpp
static void RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password);
```
- **Endpoint**: `POST /api/auth/register`
- **Payload**: `{"username":"...","email":"...","password":"..."}`
- **Response Handler**: `OnRegisterResponse`
- **Success (201)**: Logs success
- **Error (409)**: Username/email already exists

### LoginUser
```cpp
static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);
```
- **Endpoint**: `POST /api/auth/login`
- **Payload**: `{"username":"...","password":"..."}`
- **Response Handler**: `OnLoginResponse` (uses lambda to capture WorldContextObject)
- **Success (200)**: Parses token, username, user_id from JSON → calls `GameInstance->SetAuthData()` → triggers `OnLoginSuccess`
- **Error (401)**: Broadcasts `OnLoginFailed`

### GetCharacters
```cpp
static void GetCharacters(UObject* WorldContextObject);
```
- **Endpoint**: `GET /api/characters`
- **Auth**: `Authorization: Bearer <token>` header
- **Response Handler**: `OnGetCharactersResponse`
- **Success (200)**: Parses character array from JSON → populates `TArray<FCharacterData>` → calls `GameInstance->SetCharacterList()`
- **Error (401)**: Not authenticated

### CreateCharacter
```cpp
static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass);
```
- **Endpoint**: `POST /api/characters`
- **Auth**: Bearer token
- **Payload**: `{"name":"...","characterClass":"..."}`
- **Response Handler**: `OnCreateCharacterResponse`
- **Success (201)**: Broadcasts `GameInstance->OnCharacterCreated`
- **Error (409)**: Name already exists

### SaveCharacterPosition
```cpp
static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);
```
- **Endpoint**: `PUT /api/characters/{id}/position`
- **Auth**: Bearer token
- **Payload**: `{"x":...,"y":...,"z":...}`
- **Response Handler**: `OnSavePositionResponse`

## JSON Parsing

The implementation uses manual string parsing (not UE5 `FJsonSerializer`) for simplicity:

```cpp
// Pattern: Find prefix, extract between prefix end and next quote/comma
FString TokenPrefix = TEXT("\"token\":\"");
int32 TokenStart = ResponseContent.Find(TokenPrefix);
TokenStart += TokenPrefix.Len();
int32 TokenEnd = ResponseContent.Find(TEXT("\""), ..., TokenStart);
FString Token = ResponseContent.Mid(TokenStart, TokenEnd - TokenStart);
```

Character list parsing splits on `},{` and extracts fields individually.

## Helper Function

```cpp
static UMMOGameInstance* GetGameInstance(UObject* WorldContextObject);
```
Private. Returns `Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject))`.

## Blueprint Usage

```
WBP_LoginScreen — On Login Button Clicked:
    → Login User (WorldContextObject=Self, Username, Password)

BP_GameFlow — Event BeginPlay:
    → Get Game Instance → Cast To MMOGameInstance → Bind Events
    → OnLoginSuccess → Get Characters(WorldContextObject=Self)
    → OnCharacterListReceived → Show Character Select UI
```

---

**Last Updated**: 2026-02-17
