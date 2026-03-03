// Fill out your copyright notice in the Description page of Project Settings.

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

public:
    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void TestServerConnection(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void HealthCheck(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void RegisterUser(UObject* WorldContextObject, const FString& Username, const FString& Email, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void LoginUser(UObject* WorldContextObject, const FString& Username, const FString& Password);

    static void OnRegisterResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);
    static void OnLoginResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void GetCharacters(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void CreateCharacter(UObject* WorldContextObject, const FString& CharacterName, const FString& CharacterClass);

    UFUNCTION(BlueprintCallable, Category = "Network", meta = (WorldContext = "WorldContextObject"))
    static void SaveCharacterPosition(UObject* WorldContextObject, int32 CharacterId, float X, float Y, float Z);

    static void OnGetCharactersResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);
    static void OnCreateCharacterResponse(UObject* WorldContextObject, TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);
    static void OnSavePositionResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);

    static void OnHealthCheckResponse(TSharedPtr<IHttpRequest> Request, TSharedPtr<IHttpResponse> Response, bool bWasSuccessful);

    // ---- Skill targeting bridge (called by Blueprint hotbar to route through C++ targeting system) ----

    /** Use a skill through the SkillTreeSubsystem targeting system.
     *  For Bash/Magnum Break etc., this enters targeting mode (cursor changes, click to cast).
     *  For self/passive skills, executes immediately. */
    UFUNCTION(BlueprintCallable, Category = "Skills", meta = (WorldContext = "WorldContextObject"))
    static void UseSkillWithTargeting(UObject* WorldContextObject, int32 SkillId);

    /** Toggle the Combat Stats window (shows ATK, MATK, HIT, FLEE, CRI, DEF, MDEF, ASPD, base stats).
     *  Also bound to F8 key in C++. */
    UFUNCTION(BlueprintCallable, Category = "UI", meta = (WorldContext = "WorldContextObject"))
    static void ToggleCombatStatsWidget(UObject* WorldContextObject);
};
