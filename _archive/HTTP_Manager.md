# HTTP Manager (C++)

## Overview

Static Blueprint function library providing HTTP communication between UE5 client and Node.js server. Handles authentication, character management, and position saving.

## Class Definition

```cpp
UCLASS()
class SABRIMMO_API UHttpManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Authentication
    UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
    static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);

    // Character Management
    UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
    static void GetCharacters(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
    static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass);

    // Position Persistence
    UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
    static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);
};
```

## Key Concepts

### World Context Object

All functions require `UObject* WorldContextObject` parameter:
- Enables access to GameInstance
- Required for UBlueprintFunctionLibrary static functions
- Pass `Get World` from Blueprint

### Lambda Callbacks

```cpp
Request->OnProcessRequestComplete().BindLambda([WorldContextObject](
    TSharedPtr<IHttpRequest> Request, 
    TSharedPtr<IHttpResponse> Response, 
    bool bWasSuccessful
) {
    UHttpManager::OnLoginResponse(WorldContextObject, Request, Response, bWasSuccessful);
});
```

### JWT Header Attachment

```cpp
if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
{
    if (GI->IsAuthenticated())
    {
        Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
    }
}
```

## Function Details

### LoginUser

**Blueprint Usage:**
```
HttpManager::LoginUser(World, Username, Password)
```

**Implementation:**
```cpp
void UHttpManager::LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password)
{
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    
    // Bind callback with WorldContext capture
    Request->OnProcessRequestComplete().BindLambda([WorldContextObject](
        TSharedPtr<IHttpRequest> Request, 
        TSharedPtr<IHttpResponse> Response, 
        bool bWasSuccessful
    ) {
        UHttpManager::OnLoginResponse(WorldContextObject, Request, Response, bWasSuccessful);
    });
    
    Request->SetURL(TEXT("http://localhost:3001/api/auth/login"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    // JSON payload
    FString JsonPayload = FString::Printf(
        TEXT("{\"username\":\"%s\",\"password\":\"%s\"}"), 
        *Username, *Password
    );
    Request->SetContentAsString(JsonPayload);
    
    Request->ProcessRequest();
}
```

**Response Handler:**
```cpp
void UHttpManager::OnLoginResponse(UObject* WorldContextObject, ...)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();
        
        if (ResponseCode == 200)
        {
            // Parse token from JSON
            FString TokenPrefix = TEXT("\"token\":\"");
            int32 TokenStart = ResponseContent.Find(TokenPrefix);
            // ... extraction logic ...
            
            // Extract user data
            FString Username = TEXT("Unknown");
            int32 UserId = 0;
            // ... extraction logic ...
            
            // Store in GameInstance and broadcast success
            if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
            {
                GI->SetAuthData(Token, Username, UserId);
            }
        }
        else if (ResponseCode == 401)
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid credentials"));
            if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
            {
                GI->OnLoginFailed.Broadcast();
            }
        }
        else
        {
            // Other errors
            if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
            {
                GI->OnLoginFailed.Broadcast();
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to connect to login server"));
        if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
        {
            GI->OnLoginFailed.Broadcast();
        }
    }
}
```

### GetCharacters

**Blueprint Usage:**
```
HttpManager::GetCharacters(World)
```

**Key Features:**
- Requires authentication (adds JWT header)
- Parses JSON array of characters
- Stores result in GameInstance
- Broadcasts OnCharacterListReceived

### CreateCharacter

**Blueprint Usage:**
```
HttpManager::CreateCharacter(World, "HeroName", "warrior")
```

**Valid Classes:**
- warrior (default)
- mage
- archer
- healer

### SaveCharacterPosition

**Blueprint Usage:**
```
HttpManager::SaveCharacterPosition(World, CharacterId, X, Y, Z)
```

**Notes:**
- Called automatically by Level Blueprint timer (5s interval)
- Silent operation (no UI feedback needed)
- Requires valid JWT token

## JSON Parsing

### Manual String Parsing

Used for simple JSON responses:
```cpp
FString TokenPrefix = TEXT("\"token\":\"");
int32 TokenStart = ResponseContent.Find(TokenPrefix);
if (TokenStart != INDEX_NONE)
{
    TokenStart += TokenPrefix.Len();
    int32 TokenEnd = ResponseContent.Find(
        TEXT("\""), 
        ESearchCase::CaseSensitive, 
        ESearchDir::FromStart, 
        TokenStart
    );
    if (TokenEnd != INDEX_NONE)
    {
        FString Token = ResponseContent.Mid(TokenStart, TokenEnd - TokenStart);
    }
}
```

### Array Parsing

For character list:
```cpp
FString CharactersPrefix = TEXT("\"characters\":[");
int32 CharactersStart = ResponseContent.Find(CharactersPrefix);
if (CharactersStart != INDEX_NONE)
{
    CharactersStart += CharactersPrefix.Len();
    int32 CharactersEnd = ResponseContent.Find(
        TEXT("]"), 
        ESearchCase::CaseSensitive, 
        ESearchDir::FromStart, 
        CharactersStart
    );
    // Parse individual objects
}
```

## Error Handling

### Network Errors

```cpp
if (!bWasSuccessful)
{
    UE_LOG(LogTemp, Error, TEXT("Failed to connect to server"));
    // Optionally broadcast failure event
}
```

### HTTP Error Codes

| Code | Meaning | Action |
|------|---------|--------|
| 200 | Success | Process response |
| 201 | Created | Character created successfully |
| 400 | Bad Request | Invalid input data |
| 401 | Unauthorized | Invalid/missing JWT |
| 403 | Forbidden | Invalid token |
| 409 | Conflict | Duplicate resource |
| 500 | Server Error | Server issue |

## UE5 HTTP Module

### Required Module

In `SabriMMO.Build.cs`:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
    "Core", 
    "CoreUObject", 
    "Engine", 
    "InputCore",
    "HTTP",      // <-- Required
    "Json"       // <-- For JSON handling
});
```

### Key Classes

| Class | Purpose |
|-------|---------|
| FHttpModule | HTTP module singleton |
| IHttpRequest | HTTP request interface |
| IHttpResponse | HTTP response interface |
| TSharedPtr | Smart pointer for request lifecycle |

### Request Flow

```cpp
// 1. Create request
TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

// 2. Bind callback
Request->OnProcessRequestComplete().BindLambda([](...) { ... });

// 3. Configure
Request->SetURL(TEXT("http://localhost:3001/api/endpoint"));
Request->SetVerb(TEXT("POST"));
Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

// 4. Set body (for POST/PUT)
Request->SetContentAsString(JsonPayload);

// 5. Send
Request->ProcessRequest();
```

## Files

- `client/SabriMMO/Source/SabriMMO/HttpManager.h`
- `client/SabriMMO/Source/SabriMMO/HttpManager.cpp`
