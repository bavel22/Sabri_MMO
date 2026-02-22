# FCharacterData

**File**: `Source/SabriMMO/CharacterData.h`  
**Type**: USTRUCT(BlueprintType)  
**Purpose**: Data transfer object for character information between C++ and Blueprints. Used by `UMMOGameInstance` to store character list and selected character.

## Definition

```cpp
USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 CharacterId;          // Server-assigned character ID

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString Name;               // Character display name

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    FString CharacterClass;     // Class: warrior, mage, archer, healer, priest

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Level;                // Character level

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float X;                    // World X position

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Y;                    // World Y position

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    float Z;                    // World Z position

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Health;               // Current health

    UPROPERTY(BlueprintReadWrite, Category = "Character Data")
    int32 Mana;                 // Current mana
};
```

## Default Values

| Field | Default |
|-------|---------|
| CharacterId | 0 |
| Name | "" |
| CharacterClass | "warrior" |
| Level | 1 |
| X, Y, Z | 0.0f |
| Health | 100 |
| Mana | 100 |

## Usage

- Stored in `UMMOGameInstance::CharacterList` (TArray<FCharacterData>)
- Populated by `UHttpManager::OnGetCharactersResponse` from JSON
- Selected character stored in `UMMOGameInstance::SelectedCharacter`
- Accessed in Blueprints via "Break FCharacterData" node

## JSON Mapping (Server → C++)

| JSON Field | C++ Field |
|-----------|-----------|
| `character_id` | `CharacterId` |
| `name` | `Name` |
| `class` | `CharacterClass` |
| `level` | `Level` |
| `x` | `X` |
| `y` | `Y` |
| `z` | `Z` |
| `health` | `Health` |
| `mana` | `Mana` |

---

**Last Updated**: 2026-02-17
