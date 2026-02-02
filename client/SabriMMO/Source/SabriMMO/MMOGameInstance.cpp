// Fill out your copyright notice in the Description page of Project Settings.

#include "MMOGameInstance.h"

void UMMOGameInstance::SetAuthData(const FString& InToken, const FString& InUsername, int32 InUserId)
{
    AuthToken = InToken;
    Username = InUsername;
    UserId = InUserId;
    bIsLoggedIn = true;
    
    UE_LOG(LogTemp, Log, TEXT("Auth data set for user: %s (ID: %d)"), *Username, UserId);
    UE_LOG(LogTemp, Log, TEXT("Broadcasting OnLoginSuccess event..."));
    OnLoginSuccess.Broadcast();
    UE_LOG(LogTemp, Log, TEXT("OnLoginSuccess event broadcast complete"));
}

void UMMOGameInstance::SetCharacterList(const TArray<FCharacterData>& Characters)
{
    CharacterList = Characters;
    UE_LOG(LogTemp, Log, TEXT("Character list updated with %d characters"), Characters.Num());
    OnCharacterListReceived.Broadcast();
}

void UMMOGameInstance::SelectCharacter(int32 CharacterId)
{
    SelectedCharacterId = CharacterId;
    
    for (const FCharacterData& Character : CharacterList)
    {
        if (Character.CharacterId == CharacterId)
        {
            SelectedCharacter = Character;
            UE_LOG(LogTemp, Log, TEXT("Selected character: %s (ID: %d)"), *Character.Name, CharacterId);
            return;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Character ID %d not found in character list"), CharacterId);
}

FCharacterData UMMOGameInstance::GetSelectedCharacter() const
{
    return SelectedCharacter;
}

bool UMMOGameInstance::IsAuthenticated() const
{
    return bIsLoggedIn && !AuthToken.IsEmpty();
}

FString UMMOGameInstance::GetAuthHeader() const
{
    if (AuthToken.IsEmpty())
    {
        return TEXT("");
    }
    return TEXT("Bearer ") + AuthToken;
}

void UMMOGameInstance::ClearAuthData()
{
    AuthToken.Empty();
    Username.Empty();
    UserId = 0;
    bIsLoggedIn = false;
    CharacterList.Empty();
    SelectedCharacterId = 0;
    SelectedCharacter = FCharacterData();
    
    UE_LOG(LogTemp, Log, TEXT("Auth data cleared"));
}
