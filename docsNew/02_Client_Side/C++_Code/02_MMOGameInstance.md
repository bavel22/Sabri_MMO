# UMMOGameInstance

**Files**: `Source/SabriMMO/MMOGameInstance.h` (79 lines), `MMOGameInstance.cpp` (73 lines)  
**Parent**: `UGameInstance`  
**Purpose**: Persists authentication state, character data, and event dispatchers across level loads. Central data hub for the MMO client.

## Properties

### Authentication (Category: "MMO Auth")

| Property | Type | Access | Description |
|----------|------|--------|-------------|
| `AuthToken` | FString | BlueprintReadOnly | JWT token from server |
| `Username` | FString | BlueprintReadOnly | Logged-in username |
| `UserId` | int32 | BlueprintReadOnly | Server user ID |
| `bIsLoggedIn` | bool | BlueprintReadOnly | Whether user is authenticated |
| `CharacterList` | TArray\<FCharacterData\> | BlueprintReadOnly | All characters for this user |

### Private

| Property | Type | Description |
|----------|------|-------------|
| `SelectedCharacterId` | int32 | Currently selected character ID |
| `SelectedCharacter` | FCharacterData | Full data of selected character |

## Event Dispatchers (Category: "MMO Events")

| Delegate | Type | Fired When |
|----------|------|------------|
| `OnLoginSuccess` | FOnLoginSuccess | `SetAuthData()` called (login response received) |
| `OnLoginFailed` | FOnLoginFailed | Login response returns 401 or connection fails |
| `OnCharacterCreated` | FOnCharacterCreated | Character creation response returns 201 |
| `OnCharacterListReceived` | FOnCharacterListReceived | `SetCharacterList()` called |

All delegates are `DECLARE_DYNAMIC_MULTICAST_DELEGATE` (no parameters).

## Functions

### SetAuthData (BlueprintCallable)
```cpp
void SetAuthData(const FString& InToken, const FString& InUsername, int32 InUserId);
```
Stores auth credentials, sets `bIsLoggedIn = true`, broadcasts `OnLoginSuccess`.

### ClearAuthData (BlueprintCallable)
```cpp
void ClearAuthData();
```
Resets all auth fields to defaults, clears character list and selection.

### IsAuthenticated (BlueprintPure)
```cpp
bool IsAuthenticated() const;
```
Returns `bIsLoggedIn && !AuthToken.IsEmpty()`.

### GetAuthHeader (BlueprintPure)
```cpp
FString GetAuthHeader() const;
```
Returns `"Bearer " + AuthToken` or empty string if no token.

### SetCharacterList (BlueprintCallable)
```cpp
void SetCharacterList(const TArray<FCharacterData>& Characters);
```
Stores character array, broadcasts `OnCharacterListReceived`.

### SelectCharacter (BlueprintCallable)
```cpp
void SelectCharacter(int32 CharacterId);
```
Searches `CharacterList` for matching ID, stores in `SelectedCharacter`.

### GetSelectedCharacter (BlueprintPure)
```cpp
FCharacterData GetSelectedCharacter() const;
```
Returns the currently selected character data.

## Blueprint Usage

```
Event BeginPlay
    → Get Game Instance → Cast to MMOGameInstance
    → Store reference
    → Bind OnLoginSuccess → [Navigate to character select]
    → Bind OnCharacterListReceived → [Populate UI]
```

## Design Patterns

- **Game Instance Pattern**: Survives level transitions
- **Event-Driven**: Delegates notify UI of state changes
- **Dependency Injection**: Blueprints get reference via `GetGameInstance()`

---

**Last Updated**: 2026-02-17
