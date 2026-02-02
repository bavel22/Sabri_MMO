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

void UMMOGameInstance::ClearAuthData()
{
    AuthToken.Empty();
    Username.Empty();
    UserId = 0;
    bIsLoggedIn = false;
    
    UE_LOG(LogTemp, Log, TEXT("Auth data cleared"));
}

bool UMMOGameInstance::IsAuthenticated() const
{
    return bIsLoggedIn && !AuthToken.IsEmpty();
}

FString UMMOGameInstance::GetAuthHeader() const
{
    if (AuthToken.IsEmpty())
    {
        return FString();
    }
    return FString::Printf(TEXT("Bearer %s"), *AuthToken);
}
