# Character Management System

## Overview

Complete character CRUD (Create, Read, Update, Delete) system allowing players to create, view, and select characters. Characters are persisted in PostgreSQL with position data for world spawning.

## Architecture

```
UE5 Client (WBP_CharacterSelect)
    ↓
HttpManager::GetCharacters()
    ↓
GET /api/characters (JWT required)
    ↓
Node.js Server
    ↓
PostgreSQL Database
    ↓
Returns character list with position data
```

## Data Structure

### FCharacterData Struct (C++)

```cpp
USTRUCT(BlueprintType)
struct FCharacterData
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    int32 CharacterId;
    
    UPROPERTY(BlueprintReadWrite)
    FString Name;
    
    UPROPERTY(BlueprintReadWrite)
    FString CharacterClass;
    
    UPROPERTY(BlueprintReadWrite)
    int32 Level;
    
    UPROPERTY(BlueprintReadWrite)
    float X;
    
    UPROPERTY(BlueprintReadWrite)
    float Y;
    
    UPROPERTY(BlueprintReadWrite)
    float Z;
    
    UPROPERTY(BlueprintReadWrite)
    int32 Health;
    
    UPROPERTY(BlueprintReadWrite)
    int32 Mana;
};
```

## Client Implementation

### MMOGameInstance Character Storage

```cpp
UPROPERTY(BlueprintReadOnly, Category = "MMO Auth")
TArray<FCharacterData> CharacterList;

UPROPERTY(BlueprintAssignable, Category = "MMO Events")
FOnCharacterListReceived OnCharacterListReceived;

UPROPERTY(BlueprintAssignable, Category = "MMO Events")
FOnCharacterCreated OnCharacterCreated;

UFUNCTION(BlueprintCallable, Category = "MMO Auth")
void SetCharacterList(const TArray<FCharacterData>& Characters);

UFUNCTION(BlueprintCallable, Category = "MMO Auth")
void SelectCharacter(int32 CharacterId);

UFUNCTION(BlueprintPure, Category = "MMO Auth")
FCharacterData GetSelectedCharacter() const;
```

### HttpManager Functions

**Get Characters:**
```cpp
static void GetCharacters(UObject* WorldContextObject);
```

**Create Character:**
```cpp
static void CreateCharacter(
    UObject* WorldContextObject, 
    const FString& CharacterName, 
    const FString& CharacterClass
);
```

**Implementation Flow:**
1. Get JWT token from GameInstance
2. Set Authorization header
3. Send HTTP request
4. Parse JSON response
5. Store data in GameInstance
6. Broadcast appropriate event

## Server Implementation

### Database Queries

**Get All Characters for User:**
```sql
SELECT character_id, name, class, level, x, y, z, health, mana, created_at 
FROM characters 
WHERE user_id = $1 
ORDER BY created_at DESC
```

**Create Character:**
```sql
INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana) 
VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
RETURNING character_id, name, class, level, x, y, z, health, mana, created_at
```

### Character Classes

Valid classes for new characters:
- `warrior` - Default, high health
- `mage` - High mana, magical abilities
- `archer` - Ranged combat
- `healer` - Support abilities

## Blueprint Flow

### Character Select Screen (WBP_CharacterSelect)

**Event Construct:**
```
Get Game Instance → Cast to MMOGameInstance
    ↓
Get Characters (HttpManager)
    ↓
[OnCharacterListReceived fires]
```

**OnCharacterListReceived Handler:**
```
Get Game Instance → Get CharacterList
    ↓
For Each Loop (CharacterList)
    ↓
Create Widget (WBP_CharacterEntry)
    ↓
Call SetupCharacterData (Array Element)
    ↓
Add Child to Scroll Box
```

### Character Entry (WBP_CharacterEntry)

**Variables:**
- `CharacterId` (int32, Expose on Spawn)

**SetupCharacterData Function:**
```
Input: CharacterData (FCharacterData structure)
    ↓
Set NameText: CharacterData.Name
Set ClassText: CharacterData.CharacterClass
Set LevelText: "Level " + CharacterData.Level
Store CharacterId
```

**Select Button OnClicked:**
```
Get Game Instance → Cast to MMOGameInstance
    ↓
Select Character (CharacterId)
```

### Character Creation (WBP_CreateCharacter)

**Event Construct:**
```
ClassDropdown → Add Option: "warrior"
ClassDropdown → Add Option: "mage"
ClassDropdown → Add Option: "archer"
ClassDropdown → Add Option: "healer"
ClassDropdown → Set Selected Option: "warrior"
```

**Create Button OnClicked:**
```
Get NameInput → Get Text → To String
Get ClassDropdown → Get Selected Option
    ↓
Get Game Instance → Cast to MMOGameInstance
    ↓
Create Character (Name, Class)
    ↓
Remove from Parent
```

**Cancel Button OnClicked:**
```
Remove from Parent
Create Widget (WBP_CharacterSelect)
Add to Viewport
```

### Enter World Flow

**Enter World Button (WBP_CharacterSelect):**
```
Get Game Instance → Get SelectedCharacter
    ↓
Is Valid? → Branch
    ├─ FALSE → Show "Please select a character" error
    └─ TRUE → Open Level ("ThirdPersonMap")
```

## API Endpoints

### GET /api/characters

**Headers:**
```
Authorization: Bearer <jwt_token>
```

**Success Response (200):**
```json
{
    "message": "Characters retrieved successfully",
    "characters": [
        {
            "character_id": 10,
            "name": "MyHero",
            "class": "warrior",
            "level": 1,
            "x": 123.45,
            "y": 67.89,
            "z": 0.00,
            "health": 100,
            "mana": 100
        }
    ]
}
```

### POST /api/characters

**Headers:**
```
Authorization: Bearer <jwt_token>
Content-Type: application/json
```

**Request:**
```json
{
    "name": "NewHero",
    "characterClass": "mage"
}
```

**Success Response (201):**
```json
{
    "message": "Character created successfully",
    "character": {
        "character_id": 11,
        "name": "NewHero",
        "class": "mage",
        "level": 1,
        "x": 0,
        "y": 0,
        "z": 0,
        "health": 100,
        "mana": 100
    }
}
```

**Error Responses:**
- `400` - Invalid character name (2-50 chars required)
- `401` - Not authenticated
- `409` - Character name already exists for this user

### GET /api/characters/:id

**Headers:**
```
Authorization: Bearer <jwt_token>
```

**Success Response (200):**
```json
{
    "message": "Character retrieved successfully",
    "character": { ... }
}
```

## Character Spawning

### Level Blueprint (ThirdPersonMap)

**Event BeginPlay:**
```
Get Game Instance → Get SelectedCharacter
    ↓
Branch (X==0 AND Y==0 AND Z==0?)
    ├─ TRUE → Get Player Start → Get Transform
    │         Spawn Character at Player Start
    └─ FALSE → Spawn Character at Saved Position
                  (Make Vector: X, Y, Z)
```

**Auto-Save Position (Timer):**
```
Event BeginPlay → Set Timer by Function Name
    ↓
Function: SaveCharacterPosition
Time: 5.0 (seconds)
Looping: true
```

**SaveCharacterPosition Function:**
```
Get Player Character → Get Actor Location
    ↓
Break Vector (X, Y, Z)
    ↓
Get Game Instance → Get SelectedCharacter → CharacterId
    ↓
HttpManager::SaveCharacterPosition(CharacterId, X, Y, Z)
```

## Files

### Server
- `server/src/index.js` - Character endpoints

### Client C++
- `client/SabriMMO/Source/SabriMMO/CharacterData.h`
- `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h/.cpp`
- `client/SabriMMO/Source/SabriMMO/HttpManager.h/.cpp`

### Client Blueprints
- `client/SabriMMO/Content/UI/WBP_CharacterSelect.uasset`
- `client/SabriMMO/Content/UI/WBP_CharacterEntry.uasset`
- `client/SabriMMO/Content/UI/WBP_CreateCharacter.uasset`
- `client/SabriMMO/Content/Maps/ThirdPersonMap_BuiltData.uasset` (Level Blueprint)

## Integration Flow

1. **User logs in** → OnLoginSuccess fires
2. **Get Characters** called → Character list retrieved
3. **OnCharacterListReceived** → WBP_CharacterSelect shown
4. **User selects character** → Selection stored in GameInstance
5. **User clicks Enter World** → Level loads
6. **Level BeginPlay** → Character spawned at saved position
7. **Timer starts** → Position saved every 5 seconds
