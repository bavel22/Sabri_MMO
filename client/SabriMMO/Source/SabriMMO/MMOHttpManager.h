#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CharacterData.h"
#include "MMOHttpManager.generated.h"

class IHttpRequest;
class IHttpResponse;
class UMMOGameInstance;

UCLASS()
class SABRIMMO_API UHttpManager : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

private:
    static UMMOGameInstance* GetGameInstance(UObject* WorldContextObject);
    static FString GetServerBaseUrl(UObject* WorldContextObject);
    static FString ExtractErrorMessage(const FString& ResponseContent, const FString& DefaultMessage);

public:
    // ---- Connection Test ----
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void TestServerConnection(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void HealthCheck(UObject* WorldContextObject);

    // ---- Auth ----
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);

    // ---- Server List ----
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void GetServerList(UObject* WorldContextObject);

    // ---- Characters ----
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void GetCharacters(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass,
        int32 HairStyle = 1, int32 HairColor = 0, const FString& Gender = TEXT("male"));

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void DeleteCharacter(UObject* WorldContextObject, int32 CharacterId, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);

    // ---- Skill/UI Bridges ----
    UFUNCTION(BlueprintCallable, Category = "Skills", meta = (WorldContext = "WorldContextObject"))
    static void UseSkillWithTargeting(UObject* WorldContextObject, int32 SkillId);

    UFUNCTION(BlueprintCallable, Category = "UI", meta = (WorldContext = "WorldContextObject"))
    static void ToggleCombatStatsWidget(UObject* WorldContextObject);
};
