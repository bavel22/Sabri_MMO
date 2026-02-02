// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterData.h"
#include "MMOGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginFailed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterCreated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterListReceived);

UCLASS()
class SABRIMMO_API UMMOGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // Stored authentication data
    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    FString AuthToken;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    FString Username;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    int32 UserId;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    bool bIsLoggedIn;

    UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
    TArray<FCharacterData> CharacterList;

    // Event Dispatchers
    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginSuccess OnLoginSuccess;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnLoginFailed OnLoginFailed;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterCreated OnCharacterCreated;

    UPROPERTY(BlueprintAssignable, Category = "MMO Events")
    FOnCharacterListReceived OnCharacterListReceived;

    // Functions
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

private:
    UPROPERTY()
    int32 SelectedCharacterId;

    UPROPERTY()
    FCharacterData SelectedCharacter;
};
