# Character Select UI Implementation

## Overview
Complete character selection system with dynamic list population, character creation dialog, and game entry flow.

## C++ Backend Components

### FCharacterData Struct (`CharacterData.h`)
```cpp
USTRUCT(BlueprintType)
struct FCharacterData {
    UPROPERTY(BlueprintReadWrite) int32 CharacterId;
    UPROPERTY(BlueprintReadWrite) FString Name;
    UPROPERTY(BlueprintReadWrite) FString CharacterClass;
    UPROPERTY(BlueprintReadWrite) int32 Level;
};
```

### MMOGameInstance Extensions
- `CharacterList` (TArray<FCharacterData>) - stores user's characters
- `SelectedCharacter` (FCharacterData) - currently selected character
- `OnCharacterListReceived` event dispatcher
- `OnCharacterCreated` event dispatcher
- `SetCharacterList()`, `SelectCharacter()`, `GetSelectedCharacter()` functions

### HttpManager API Methods
- `GetCharacters()` - fetches character list from server
- `CreateCharacter(name, class)` - creates new character
- Parses JSON responses and stores data in GameInstance

## Widget Blueprints

### WBP_CharacterSelect (Main Selection Screen)
**Hierarchy:**
```
Canvas Panel
└── Scale Box
    └── Size Box (400x500)
        └── Canvas Panel
            ├── Text: "Select Character"
            ├── Scroll Box: CharacterListScrollBox
            └── Horizontal Box
                ├── Button: CreateCharacterButton
                └── Button: EnterWorldButton
```

**Event Construct:**
1. Get Game Instance → Cast to MMOGameInstance
2. Get Character List
3. For Each Loop over CharacterList
4. Create Widget (WBP_CharacterEntry)
5. Call SetupCharacterData (pass Array Element)
6. Add Child to Scroll Box

### WBP_CharacterEntry (Individual Character Row)
**Hierarchy:**
```
Canvas Panel
└── Horizontal Box
    ├── Text: NameText
    ├── Text: ClassText
    ├── Text: LevelText
    └── Button: SelectButton
```

**Variables:**
- `CharacterId` (int32, Expose on Spawn)

**Functions:**
- `SetupCharacterData(CharacterData)` - sets text fields and CharacterId

**Select Button Event:**
1. Get Game Instance → Cast to MMOGameInstance
2. Select Character (CharacterId)

### WBP_CreateCharacter (Character Creation Dialog)
**Hierarchy:**
```
Canvas Panel
└── Scale Box
    └── Size Box (400x350)
        └── Canvas Panel
            ├── Text: "Create Character"
            ├── Editable Text Box: NameInput
            ├── Combo Box String: ClassDropdown
            └── Horizontal Box
                ├── Button: CancelButton
                └── Button: CreateButton
```

**Event Construct:**
- ClassDropdown → Add Options: warrior, mage, rogue, priest
- Set Selected Option: warrior

**Create Button:**
1. Get Game Instance → Cast to MMOGameInstance
2. Create Character (Name, Class)
3. Remove from Parent

**Cancel Button:**
1. Remove from Parent
2. Create Widget (WBP_CharacterSelect)
3. Add to Viewport

## BP_GameFlow Integration

### Event Bindings (BeginPlay)
```
Sequence
    ↓ Then 0: Bind OnLoginSuccess
    ↓ Then 1: Bind OnCharacterCreated
    ↓ Then 2: Bind OnCharacterListReceived
    ↓
Delay 0.5s
    ↓
Login User
```

### OnCharacterListReceived Handler
```
Create Widget (WBP_CharacterSelect)
    ↓
Add to Viewport
    ↓
Get Player Controller → Set Show Mouse Cursor (true)
    ↓
Set Input Mode UI Only
```

## Flow Summary
1. Login → OnLoginSuccess fires
2. Get Characters → Server returns character list
3. OnCharacterListReceived → Show WBP_CharacterSelect
4. UI populates Scroll Box with WBP_CharacterEntry widgets
5. User clicks character → SelectCharacter stores selection
6. User clicks "Create Character" → Show WBP_CreateCharacter
7. User clicks "Enter World" → Open Level (game map)

## Database Integration
- Server stores characters in PostgreSQL `characters` table
- JWT authentication required for all character endpoints
- Character data includes: character_id, user_id, name, class, level
