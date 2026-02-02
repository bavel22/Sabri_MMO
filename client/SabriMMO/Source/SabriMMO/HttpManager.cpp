// Fill out your copyright notice in the Description page of Project Settings.

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"
#include "Kismet/GameplayStatics.h"
#include "MMOGameInstance.h"

UMMOGameInstance* UHttpManager::GetGameInstance(UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }
    return Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

void UHttpManager::TestServerConnection(UObject* WorldContextObject)
{
    UE_LOG(LogTemp, Log, TEXT("Testing server connection..."));
    HealthCheck(WorldContextObject);
}

void UHttpManager::HealthCheck(UObject* WorldContextObject)
{
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindStatic(&UHttpManager::OnHealthCheckResponse);
    Request->SetURL(TEXT("http://localhost:3001/health"));
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->ProcessRequest();

    UE_LOG(LogTemp, Log, TEXT("Sending health check request to: http://localhost:3001"));
}

void UHttpManager::OnHealthCheckResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Health Check Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Health Check Response: %s"), *ResponseContent);

        if (ResponseCode == 200)
        {
            UE_LOG(LogTemp, Display, TEXT("✓ Server is ONLINE and connected to database!"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Server returned error code: %d"), ResponseCode);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to server. Is it running on port 3001?"));
    }
}

void UHttpManager::RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password)
{
    UE_LOG(LogTemp, Log, TEXT("Registering user: %s"), *Username);

    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindStatic(&UHttpManager::OnRegisterResponse);
    Request->SetURL(TEXT("http://localhost:3001/api/auth/register"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Create JSON payload
    FString JsonPayload = FString::Printf(TEXT("{\"username\":\"%s\",\"email\":\"%s\",\"password\":\"%s\"}"), 
        *Username, *Email, *Password);
    Request->SetContentAsString(JsonPayload);

    Request->ProcessRequest();
}

void UHttpManager::OnRegisterResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Register Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Register Response: %s"), *ResponseContent);

        if (ResponseCode == 201)
        {
            UE_LOG(LogTemp, Display, TEXT("✓ User registered successfully! Token stored for authenticated requests."));
        }
        else if (ResponseCode == 409)
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Username or email already exists"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Registration failed: %s"), *ResponseContent);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to registration server"));
    }
}

void UHttpManager::LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Log, TEXT("Logging in user: %s"), *Username);

    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WorldContextObject](TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        UHttpManager::OnLoginResponse(WorldContextObject, Request, Response, bWasSuccessful);
    });
    Request->SetURL(TEXT("http://localhost:3001/api/auth/login"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    // Create JSON payload
    FString JsonPayload = FString::Printf(TEXT("{\"username\":\"%s\",\"password\":\"%s\"}"), 
        *Username, *Password);
    Request->SetContentAsString(JsonPayload);

    Request->ProcessRequest();
}

void UHttpManager::OnLoginResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Login Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Login Response: %s"), *ResponseContent);

        if (ResponseCode == 200)
        {
            // Extract token and user data from JSON response
            FString TokenPrefix = TEXT("\"token\":\"");
            int32 TokenStart = ResponseContent.Find(TokenPrefix);
            if (TokenStart != INDEX_NONE)
            {
                TokenStart += TokenPrefix.Len();
                int32 TokenEnd = ResponseContent.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, TokenStart);
                if (TokenEnd != INDEX_NONE)
                {
                    FString Token = ResponseContent.Mid(TokenStart, TokenEnd - TokenStart);
                    
                    // Extract username and user_id
                    FString UsernamePrefix = TEXT("\"username\":\"");
                    int32 UsernameStart = ResponseContent.Find(UsernamePrefix);
                    FString Username = TEXT("Unknown");
                    int32 UserId = 0;
                    
                    if (UsernameStart != INDEX_NONE)
                    {
                        UsernameStart += UsernamePrefix.Len();
                        int32 UsernameEnd = ResponseContent.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, UsernameStart);
                        if (UsernameEnd != INDEX_NONE)
                        {
                            Username = ResponseContent.Mid(UsernameStart, UsernameEnd - UsernameStart);
                        }
                    }
                    
                    // Extract user_id
                    FString UserIdPrefix = TEXT("\"user_id\":");
                    int32 UserIdStart = ResponseContent.Find(UserIdPrefix);
                    if (UserIdStart != INDEX_NONE)
                    {
                        UserIdStart += UserIdPrefix.Len();
                        int32 UserIdEnd = ResponseContent.Find(TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, UserIdStart);
                        if (UserIdEnd != INDEX_NONE)
                        {
                            FString UserIdStr = ResponseContent.Mid(UserIdStart, UserIdEnd - UserIdStart);
                            UserId = FCString::Atoi(*UserIdStr);
                        }
                    }
                    
                    // Set auth data in GameInstance and broadcast event
                    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
                    {
                        GI->SetAuthData(Token, Username, UserId);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to get GameInstance for login response"));
                    }
                }
            }
        }
        else if (ResponseCode == 401)
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Invalid credentials"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Login failed: %s"), *ResponseContent);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to login server"));
    }
}

void UHttpManager::GetCharacters(UObject* WorldContextObject)
{
    UE_LOG(LogTemp, Log, TEXT("Fetching characters for user..."));

    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WorldContextObject](TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        UHttpManager::OnGetCharactersResponse(WorldContextObject, Request, Response, bWasSuccessful);
    });
    Request->SetURL(TEXT("http://localhost:3001/api/characters"));
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
    {
        if (GI->IsAuthenticated())
        {
            Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot get characters: Not authenticated"));
            return;
        }
    }

    Request->ProcessRequest();
}

void UHttpManager::OnGetCharactersResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Get Characters Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Get Characters Response: %s"), *ResponseContent);

        if (ResponseCode == 200)
        {
            // Parse character list from JSON
            TArray<FCharacterData> CharacterList;
            
            // Simple JSON parsing (basic implementation)
            FString CharactersPrefix = TEXT("\"characters\":[");
            int32 CharactersStart = ResponseContent.Find(CharactersPrefix);
            if (CharactersStart != INDEX_NONE)
            {
                CharactersStart += CharactersPrefix.Len();
                int32 CharactersEnd = ResponseContent.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CharactersStart);
                if (CharactersEnd != INDEX_NONE)
                {
                    FString CharactersArray = ResponseContent.Mid(CharactersStart, CharactersEnd - CharactersStart);
                    
                    // Parse each character object
                    TArray<FString> CharacterObjects;
                    CharactersArray.ParseIntoArray(CharacterObjects, TEXT("},{"));
                    
                    for (FString& CharObj : CharacterObjects)
                    {
                        // Clean up the string
                        CharObj = CharObj.Replace(TEXT("{"), TEXT(""));
                        CharObj = CharObj.Replace(TEXT("}"), TEXT(""));
                        CharObj = CharObj.Replace(TEXT("["), TEXT(""));
                        CharObj = CharObj.Replace(TEXT("]"), TEXT(""));
                        
                        FCharacterData CharData;
                        
                        // Parse character_id
                        FString IdPrefix = TEXT("\"character_id\":");
                        int32 IdStart = CharObj.Find(IdPrefix);
                        if (IdStart != INDEX_NONE)
                        {
                            IdStart += IdPrefix.Len();
                            FString IdStr = CharObj.Mid(IdStart);
                            int32 CommaPos = IdStr.Find(TEXT(","));
                            if (CommaPos != INDEX_NONE)
                            {
                                IdStr = IdStr.Left(CommaPos);
                            }
                            CharData.CharacterId = FCString::Atoi(*IdStr);
                        }
                        
                        // Parse name
                        FString NamePrefix = TEXT("\"name\":\"");
                        int32 NameStart = CharObj.Find(NamePrefix);
                        if (NameStart != INDEX_NONE)
                        {
                            NameStart += NamePrefix.Len();
                            int32 NameEnd = CharObj.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
                            if (NameEnd != INDEX_NONE)
                            {
                                CharData.Name = CharObj.Mid(NameStart, NameEnd - NameStart);
                            }
                        }
                        
                        // Parse class
                        FString ClassPrefix = TEXT("\"class\":\"");
                        int32 ClassStart = CharObj.Find(ClassPrefix);
                        if (ClassStart != INDEX_NONE)
                        {
                            ClassStart += ClassPrefix.Len();
                            int32 ClassEnd = CharObj.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, ClassStart);
                            if (ClassEnd != INDEX_NONE)
                            {
                                CharData.CharacterClass = CharObj.Mid(ClassStart, ClassEnd - ClassStart);
                            }
                        }
                        
                        // Parse level
                        FString LevelPrefix = TEXT("\"level\":");
                        int32 LevelStart = CharObj.Find(LevelPrefix);
                        if (LevelStart != INDEX_NONE)
                        {
                            LevelStart += LevelPrefix.Len();
                            FString LevelStr = CharObj.Mid(LevelStart);
                            int32 CommaPos = LevelStr.Find(TEXT(","));
                            if (CommaPos != INDEX_NONE)
                            {
                                LevelStr = LevelStr.Left(CommaPos);
                            }
                            CharData.Level = FCString::Atoi(*LevelStr);
                        }
                        
                        CharacterList.Add(CharData);
                    }
                }
            }
            
            // Store character list in GameInstance
            if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
            {
                GI->SetCharacterList(CharacterList);
                UE_LOG(LogTemp, Display, TEXT("✓ Characters retrieved successfully! Found %d characters"), CharacterList.Num());
            }
        }
        else if (ResponseCode == 401)
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Not authenticated. Please login first."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Failed to get characters: %s"), *ResponseContent);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to character server"));
    }
}

void UHttpManager::CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass)
{
    UE_LOG(LogTemp, Log, TEXT("Creating character: %s (Class: %s)"), *CharacterName, *CharacterClass);

    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WorldContextObject](TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        UHttpManager::OnCreateCharacterResponse(WorldContextObject, Request, Response, bWasSuccessful);
    });
    Request->SetURL(TEXT("http://localhost:3001/api/characters"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    
    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
    {
        if (GI->IsAuthenticated())
        {
            Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
        }
    }

    // Create JSON payload
    FString JsonPayload = FString::Printf(TEXT("{\"name\":\"%s\",\"characterClass\":\"%s\"}"), 
        *CharacterName, *CharacterClass);
    Request->SetContentAsString(JsonPayload);

    Request->ProcessRequest();
}

void UHttpManager::OnCreateCharacterResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid())
    {
        int32 ResponseCode = Response->GetResponseCode();
        FString ResponseContent = Response->GetContentAsString();

        UE_LOG(LogTemp, Log, TEXT("Create Character Response Code: %d"), ResponseCode);
        UE_LOG(LogTemp, Log, TEXT("Create Character Response: %s"), *ResponseContent);

        if (ResponseCode == 201)
        {
            UE_LOG(LogTemp, Display, TEXT("✓ Character created successfully!"));
            
            // Broadcast OnCharacterCreated event
            if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
            {
                UE_LOG(LogTemp, Log, TEXT("Broadcasting OnCharacterCreated event..."));
                GI->OnCharacterCreated.Broadcast();
                UE_LOG(LogTemp, Log, TEXT("OnCharacterCreated event broadcast complete"));
            }
        }
        else if (ResponseCode == 401)
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Not authenticated. Please login first."));
        }
        else if (ResponseCode == 409)
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Character with this name already exists"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("✗ Failed to create character: %s"), *ResponseContent);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("✗ Failed to connect to character creation server"));
    }
}
