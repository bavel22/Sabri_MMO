#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CharacterData.generated.h"

USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 CharacterId;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString Name;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString CharacterClass;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Level;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float X;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Y;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Z;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Health;

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Mana;

    FCharacterData()
    {
        CharacterId = 0;
        Name = TEXT("");
        CharacterClass = TEXT("warrior");
        Level = 1;
        X = 0.0f;
        Y = 0.0f;
        Z = 0.0f;
        Health = 100;
        Mana = 100;
    }
};
