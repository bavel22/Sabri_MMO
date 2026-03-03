#include "MMOHttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"
#include "Kismet/GameplayStatics.h"
#include "MMOGameInstance.h"
#include "UI/SkillTreeSubsystem.h"
#include "UI/CombatStatsSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// ============================================================
// Helpers
// ============================================================

UMMOGameInstance* UHttpManager::GetGameInstance(UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;
    return Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject));
}

FString UHttpManager::GetServerBaseUrl(UObject* WorldContextObject)
{
    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
    {
        return GI->ServerBaseUrl;
    }
    return TEXT("http://localhost:3001");
}

FString UHttpManager::ExtractErrorMessage(const FString& ResponseContent, const FString& DefaultMessage)
{
    TSharedPtr<FJsonObject> JsonObj;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);
    if (FJsonSerializer::Deserialize(Reader, JsonObj) && JsonObj.IsValid())
    {
        FString Error;
        if (JsonObj->TryGetStringField(TEXT("error"), Error))
        {
            return Error;
        }
    }
    return DefaultMessage;
}

// ============================================================
// Connection Test
// ============================================================

void UHttpManager::TestServerConnection(UObject* WorldContextObject)
{
    HealthCheck(WorldContextObject);
}

void UHttpManager::HealthCheck(UObject* WorldContextObject)
{
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
        {
            UE_LOG(LogTemp, Display, TEXT("[HTTP] Server is ONLINE"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] Server health check failed"));
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/health"));
    Request->SetVerb(TEXT("GET"));
    Request->ProcessRequest();
}

// ============================================================
// Auth — Register
// ============================================================

void UHttpManager::RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Registering user: %s"), *Username);

    TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetStringField(TEXT("username"), Username);
    JsonObj->SetStringField(TEXT("email"), Email);
    JsonObj->SetStringField(TEXT("password"), Password);

    FString Payload;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(JsonObj, Writer);

    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] Register: connection failed"));
            return;
        }

        int32 Code = Response->GetResponseCode();
        FString Content = Response->GetContentAsString();

        if (Code == 201)
        {
            UE_LOG(LogTemp, Display, TEXT("[HTTP] User registered successfully"));
            // Auto-login after registration: parse the token
            if (WeakContext.IsValid())
            {
                TSharedPtr<FJsonObject> JsonResponse;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
                if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
                {
                    FString Token;
                    if (JsonResponse->TryGetStringField(TEXT("token"), Token))
                    {
                        FString RespUsername;
                        int32 RespUserId = 0;
                        const TSharedPtr<FJsonObject>* UserObj;
                        if (JsonResponse->TryGetObjectField(TEXT("user"), UserObj))
                        {
                            (*UserObj)->TryGetStringField(TEXT("username"), RespUsername);
                            (*UserObj)->TryGetNumberField(TEXT("user_id"), RespUserId);
                        }
                        if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                        {
                            GI->SetAuthData(Token, RespUsername, RespUserId);
                        }
                    }
                }
            }
        }
        else
        {
            FString Err = ExtractErrorMessage(Content, TEXT("Registration failed"));
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] Register failed (%d): %s"), Code, *Err);
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnLoginFailedWithReason.Broadcast(Err);
                }
            }
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/api/auth/register"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Payload);
    Request->ProcessRequest();
}

// ============================================================
// Auth — Login
// ============================================================

void UHttpManager::LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Logging in user: %s"), *Username);

    TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetStringField(TEXT("username"), Username);
    JsonObj->SetStringField(TEXT("password"), Password);

    FString Payload;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(JsonObj, Writer);

    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] Login: connection failed"));
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnLoginFailed.Broadcast();
                    GI->OnLoginFailedWithReason.Broadcast(TEXT("Cannot connect to server"));
                }
            }
            return;
        }

        int32 Code = Response->GetResponseCode();
        FString Content = Response->GetContentAsString();

        if (Code == 200)
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
            {
                FString Token;
                JsonResponse->TryGetStringField(TEXT("token"), Token);

                FString RespUsername;
                int32 RespUserId = 0;

                const TSharedPtr<FJsonObject>* UserObj;
                if (JsonResponse->TryGetObjectField(TEXT("user"), UserObj))
                {
                    (*UserObj)->TryGetStringField(TEXT("username"), RespUsername);
                    double IdD = 0;
                    (*UserObj)->TryGetNumberField(TEXT("user_id"), IdD);
                    RespUserId = (int32)IdD;
                }
                else
                {
                    // Fallback: fields at root level
                    JsonResponse->TryGetStringField(TEXT("username"), RespUsername);
                    double IdD = 0;
                    JsonResponse->TryGetNumberField(TEXT("user_id"), IdD);
                    RespUserId = (int32)IdD;
                }

                if (WeakContext.IsValid())
                {
                    if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                    {
                        GI->SetAuthData(Token, RespUsername, RespUserId);
                    }
                }
            }
        }
        else
        {
            FString Err = ExtractErrorMessage(Content, TEXT("Invalid credentials"));
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] Login failed (%d): %s"), Code, *Err);
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnLoginFailed.Broadcast();
                    GI->OnLoginFailedWithReason.Broadcast(Err);
                }
            }
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/api/auth/login"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(Payload);
    Request->ProcessRequest();
}

// ============================================================
// Server List
// ============================================================

void UHttpManager::GetServerList(UObject* WorldContextObject)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Fetching server list..."));

    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] GetServerList failed"));
            return;
        }

        FString Content = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonResponse;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
        if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid()) return;

        const TArray<TSharedPtr<FJsonValue>>* ServersArray;
        if (!JsonResponse->TryGetArrayField(TEXT("servers"), ServersArray)) return;

        TArray<FServerInfo> Servers;
        for (const TSharedPtr<FJsonValue>& Val : *ServersArray)
        {
            const TSharedPtr<FJsonObject>* Obj;
            if (!Val->TryGetObject(Obj)) continue;

            FServerInfo Info;
            double IdD = 0;
            (*Obj)->TryGetNumberField(TEXT("id"), IdD);
            Info.ServerId = (int32)IdD;
            (*Obj)->TryGetStringField(TEXT("name"), Info.Name);
            (*Obj)->TryGetStringField(TEXT("host"), Info.Host);
            double PortD = 0;
            (*Obj)->TryGetNumberField(TEXT("port"), PortD);
            Info.Port = (int32)PortD;
            (*Obj)->TryGetStringField(TEXT("status"), Info.Status);
            double PopD = 0;
            (*Obj)->TryGetNumberField(TEXT("population"), PopD);
            Info.Population = (int32)PopD;
            double MaxD = 0;
            (*Obj)->TryGetNumberField(TEXT("maxPopulation"), MaxD);
            Info.MaxPopulation = (int32)MaxD;
            (*Obj)->TryGetStringField(TEXT("region"), Info.Region);

            Servers.Add(Info);
        }

        UE_LOG(LogTemp, Log, TEXT("[HTTP] Retrieved %d servers"), Servers.Num());

        if (WeakContext.IsValid())
        {
            if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
            {
                GI->SetServerList(Servers);
            }
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/api/servers"));
    Request->SetVerb(TEXT("GET"));
    Request->ProcessRequest();
}

// ============================================================
// Characters — Get List
// ============================================================

static FCharacterData ParseCharacterFromJson(const TSharedPtr<FJsonObject>& Obj)
{
    FCharacterData CD;
    double D = 0;

    Obj->TryGetNumberField(TEXT("character_id"), D); CD.CharacterId = (int32)D;
    Obj->TryGetStringField(TEXT("name"), CD.Name);
    Obj->TryGetStringField(TEXT("class"), CD.CharacterClass);
    D = 0; Obj->TryGetNumberField(TEXT("level"), D); CD.Level = (int32)D;
    D = 0; Obj->TryGetNumberField(TEXT("x"), D); CD.X = (float)D;
    D = 0; Obj->TryGetNumberField(TEXT("y"), D); CD.Y = (float)D;
    D = 0; Obj->TryGetNumberField(TEXT("z"), D); CD.Z = (float)D;
    D = 0; Obj->TryGetNumberField(TEXT("health"), D); CD.Health = (int32)D;
    D = 0; Obj->TryGetNumberField(TEXT("max_health"), D); CD.MaxHealth = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("mana"), D); CD.Mana = (int32)D;
    D = 0; Obj->TryGetNumberField(TEXT("max_mana"), D); CD.MaxMana = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("job_level"), D); CD.JobLevel = FMath::Max(1, (int32)D);
    Obj->TryGetStringField(TEXT("job_class"), CD.JobClass);
    D = 0; Obj->TryGetNumberField(TEXT("base_exp"), D); CD.BaseExp = (int64)D;
    D = 0; Obj->TryGetNumberField(TEXT("job_exp"), D); CD.JobExp = (int64)D;
    D = 0; Obj->TryGetNumberField(TEXT("skill_points"), D); CD.SkillPoints = (int32)D;

    // Stats
    D = 0; Obj->TryGetNumberField(TEXT("str"), D); CD.Str = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("agi"), D); CD.Agi = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("vit"), D); CD.Vit = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("int_stat"), D); CD.IntStat = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("dex"), D); CD.Dex = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("luk"), D); CD.Luk = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("stat_points"), D); CD.StatPoints = (int32)D;
    D = 0; Obj->TryGetNumberField(TEXT("zuzucoin"), D); CD.Zuzucoin = (int32)D;

    // Appearance
    D = 0; Obj->TryGetNumberField(TEXT("hair_style"), D); CD.HairStyle = FMath::Max(1, (int32)D);
    D = 0; Obj->TryGetNumberField(TEXT("hair_color"), D); CD.HairColor = (int32)D;
    Obj->TryGetStringField(TEXT("gender"), CD.Gender);

    // Meta
    Obj->TryGetStringField(TEXT("delete_date"), CD.DeleteDate);
    Obj->TryGetStringField(TEXT("created_at"), CD.CreatedAt);
    Obj->TryGetStringField(TEXT("last_played"), CD.LastPlayed);

    // Defaults for missing values
    if (CD.MaxHealth <= 0) CD.MaxHealth = FMath::Max(CD.Health, 100);
    if (CD.MaxMana <= 0) CD.MaxMana = FMath::Max(CD.Mana, 100);
    if (CD.JobClass.IsEmpty()) CD.JobClass = TEXT("novice");
    if (CD.Gender.IsEmpty()) CD.Gender = TEXT("male");

    return CD;
}

void UHttpManager::GetCharacters(UObject* WorldContextObject)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Fetching characters..."));

    UMMOGameInstance* GI = GetGameInstance(WorldContextObject);
    if (!GI || !GI->IsAuthenticated())
    {
        UE_LOG(LogTemp, Error, TEXT("[HTTP] Cannot get characters: not authenticated"));
        return;
    }

    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] GetCharacters: connection failed"));
            return;
        }

        int32 Code = Response->GetResponseCode();
        FString Content = Response->GetContentAsString();

        if (Code == 200)
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse.IsValid()) return;

            const TArray<TSharedPtr<FJsonValue>>* CharsArray;
            if (!JsonResponse->TryGetArrayField(TEXT("characters"), CharsArray)) return;

            TArray<FCharacterData> CharacterList;
            for (const TSharedPtr<FJsonValue>& Val : *CharsArray)
            {
                const TSharedPtr<FJsonObject>* Obj;
                if (!Val->TryGetObject(Obj)) continue;
                CharacterList.Add(ParseCharacterFromJson(*Obj));
            }

            UE_LOG(LogTemp, Display, TEXT("[HTTP] Retrieved %d characters"), CharacterList.Num());

            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->SetCharacterList(CharacterList);
                }
            }
        }
        else if (Code == 401)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] GetCharacters: not authenticated"));
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/api/characters"));
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
    Request->ProcessRequest();
}

// ============================================================
// Characters — Create
// ============================================================

void UHttpManager::CreateCharacter(UObject* WorldContextObject, const FString& CharacterName,
    const FString& CharacterClass, int32 HairStyle, int32 HairColor, const FString& Gender)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Creating character: %s"), *CharacterName);

    UMMOGameInstance* GI = GetGameInstance(WorldContextObject);
    if (!GI || !GI->IsAuthenticated())
    {
        UE_LOG(LogTemp, Error, TEXT("[HTTP] Cannot create character: not authenticated"));
        return;
    }

    TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetStringField(TEXT("name"), CharacterName);
    JsonObj->SetStringField(TEXT("characterClass"), CharacterClass);
    JsonObj->SetNumberField(TEXT("hairStyle"), HairStyle);
    JsonObj->SetNumberField(TEXT("hairColor"), HairColor);
    JsonObj->SetStringField(TEXT("gender"), Gender);

    FString Payload;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(JsonObj, Writer);

    FString AuthHeader = GI->GetAuthHeader();
    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] CreateCharacter: connection failed"));
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterCreateFailed.Broadcast(TEXT("Cannot connect to server"));
                }
            }
            return;
        }

        int32 Code = Response->GetResponseCode();
        FString Content = Response->GetContentAsString();

        if (Code == 201)
        {
            UE_LOG(LogTemp, Display, TEXT("[HTTP] Character created successfully"));
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterCreated.Broadcast();
                }
            }
        }
        else
        {
            FString Err = ExtractErrorMessage(Content, TEXT("Failed to create character"));
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] CreateCharacter failed (%d): %s"), Code, *Err);
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterCreateFailed.Broadcast(Err);
                }
            }
        }
    });
    Request->SetURL(GetServerBaseUrl(WorldContextObject) + TEXT("/api/characters"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), AuthHeader);
    Request->SetContentAsString(Payload);
    Request->ProcessRequest();
}

// ============================================================
// Characters — Delete
// ============================================================

void UHttpManager::DeleteCharacter(UObject* WorldContextObject, int32 CharacterId, const FString& Password)
{
    UE_LOG(LogTemp, Log, TEXT("[HTTP] Deleting character ID: %d"), CharacterId);

    UMMOGameInstance* GI = GetGameInstance(WorldContextObject);
    if (!GI || !GI->IsAuthenticated())
    {
        UE_LOG(LogTemp, Error, TEXT("[HTTP] Cannot delete character: not authenticated"));
        return;
    }

    TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetStringField(TEXT("password"), Password);

    FString Payload;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(JsonObj, Writer);

    FString AuthHeader = GI->GetAuthHeader();
    TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([WeakContext](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[HTTP] DeleteCharacter: connection failed"));
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterDeleteFailed.Broadcast(TEXT("Cannot connect to server"));
                }
            }
            return;
        }

        int32 Code = Response->GetResponseCode();
        FString Content = Response->GetContentAsString();

        if (Code == 200)
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Content);
            FString CharName = TEXT("Character");
            if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
            {
                JsonResponse->TryGetStringField(TEXT("characterName"), CharName);
            }
            UE_LOG(LogTemp, Display, TEXT("[HTTP] Character deleted: %s"), *CharName);

            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterDeleteSuccess.Broadcast(CharName);
                }
            }
        }
        else
        {
            FString Err = ExtractErrorMessage(Content, TEXT("Failed to delete character"));
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] DeleteCharacter failed (%d): %s"), Code, *Err);
            if (WeakContext.IsValid())
            {
                if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(UGameplayStatics::GetGameInstance(WeakContext.Get())))
                {
                    GI->OnCharacterDeleteFailed.Broadcast(Err);
                }
            }
        }
    });
    FString Url = FString::Printf(TEXT("%s/api/characters/%d"), *GetServerBaseUrl(WorldContextObject), CharacterId);
    Request->SetURL(Url);
    Request->SetVerb(TEXT("DELETE"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), AuthHeader);
    Request->SetContentAsString(Payload);
    Request->ProcessRequest();
}

// ============================================================
// Characters — Save Position
// ============================================================

void UHttpManager::SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z)
{
    TSharedRef<FJsonObject> JsonObj = MakeShared<FJsonObject>();
    JsonObj->SetNumberField(TEXT("x"), X);
    JsonObj->SetNumberField(TEXT("y"), Y);
    JsonObj->SetNumberField(TEXT("z"), Z);

    FString Payload;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    FJsonSerializer::Serialize(JsonObj, Writer);

    TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([](TSharedPtr<IHttpRequest>, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful) {
        if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
        {
            UE_LOG(LogTemp, Warning, TEXT("[HTTP] SavePosition failed"));
        }
    });

    FString Url = FString::Printf(TEXT("%s/api/characters/%d/position"), *GetServerBaseUrl(WorldContextObject), CharacterId);
    Request->SetURL(Url);
    Request->SetVerb(TEXT("PUT"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

    if (UMMOGameInstance* GI = GetGameInstance(WorldContextObject))
    {
        if (GI->IsAuthenticated())
        {
            Request->SetHeader(TEXT("Authorization"), GI->GetAuthHeader());
        }
    }

    Request->SetContentAsString(Payload);
    Request->ProcessRequest();
}

// ============================================================
// Skill/UI Bridges (unchanged)
// ============================================================

void UHttpManager::UseSkillWithTargeting(UObject* WorldContextObject, int32 SkillId)
{
    if (!WorldContextObject || SkillId <= 0) return;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return;

    USkillTreeSubsystem* Sub = World->GetSubsystem<USkillTreeSubsystem>();
    if (Sub)
    {
        Sub->UseSkill(SkillId);
    }
}

void UHttpManager::ToggleCombatStatsWidget(UObject* WorldContextObject)
{
    if (!WorldContextObject) return;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) return;

    UCombatStatsSubsystem* Sub = World->GetSubsystem<UCombatStatsSubsystem>();
    if (Sub)
    {
        Sub->ToggleWidget();
    }
}
