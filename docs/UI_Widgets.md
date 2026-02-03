# UI Widgets Documentation

## Overview

Complete UI widget system for the MMO including authentication, character management, and game flow. All widgets use UMG (Unreal Motion Graphics) with Blueprint scripting.

## Widget List

| Widget | Purpose | Parent Widget |
|--------|---------|---------------|
| WBP_LoginScreen | User authentication | Standalone (GameFlow) |
| WBP_CharacterSelect | Character selection | Standalone (GameFlow) |
| WBP_CharacterEntry | Single character display | WBP_CharacterSelect |
| WBP_CreateCharacter | Create new character | Standalone (GameFlow) |

---

## WBP_LoginScreen

### Purpose
Primary login interface for user authentication. Displays on game start, collects credentials, handles errors.

### Hierarchy
```
Canvas Panel
└── Vertical Box (centered)
    ├── Text Block: "Sabri MMO" (Title)
    ├── Spacer (20px)
    ├── Text Block: "Username"
    ├── Editable Text Box: TB_Username
    ├── Spacer (10px)
    ├── Text Block: "Password"
    ├── Editable Text Box: TB_Password (Is Password: true)
    ├── Spacer (20px)
    ├── Button: Btn_Login
    │   └── Text Block: "Login"
    ├── Spacer (10px)
    └── Text Block: Txt_Error (red, initially Hidden)
```

### Components

| Name | Type | Properties |
|------|------|------------|
| TB_Username | Editable Text Box | Hint Text: "Enter username..." |
| TB_Password | Editable Text Box | Hint Text: "Enter password...", Is Password: true |
| Btn_Login | Button | Size Y: 40 |
| Txt_Error | Text Block | Color: Red, Visibility: Hidden |

### Event Graph

**Event Construct:**
```
Set Input Mode UI Only
    └─ Player Controller: Get Player Controller (Index 0)

Set Show Mouse Cursor
    └─ Target: Get Player Controller
    └─ Show Mouse Cursor: true

Set User Focus
    ├─ Widget: TB_Username
    └─ Player Controller: Get Player Controller
```

**Btn_Login OnClicked:**
```
TB_Username → Get Text → To String → Variable: Username
TB_Password → Get Text → To String → Variable: Password

Branch (Username != "" AND Password != "")
    ├─ FALSE:
    │   Txt_Error → Set Text: "Please enter username and password"
    │   Txt_Error → Set Visibility: Visible
    │
    └─ TRUE:
        Txt_Error → Set Visibility: Hidden
        HttpManager::LoginUser(World, Username, Password)
```

### Input Mode Handling

When shown:
- Input Mode: UI Only (mouse interacts with UI only)
- Show Mouse Cursor: true

When login succeeds (handled by GameFlow):
- Input Mode: Game and UI (or Game Only)
- Show Mouse Cursor: false (or true for top-down)

---

## WBP_CharacterSelect

### Purpose
Main character selection interface. Displays scrollable list of user's characters with options to create new or enter world.

### Hierarchy
```
Canvas Panel
└── Scale Box
    └── Size Box (400x500)
        └── Canvas Panel
            ├── Text Block: "Select Character"
            ├── Scroll Box: CharacterListScrollBox
            └── Horizontal Box
                ├── Button: CreateCharacterButton
                │   └── Text Block: "Create Character"
                └── Button: EnterWorldButton
                    └── Text Block: "Enter World"
```

### Components

| Name | Type | Purpose |
|------|------|---------|
| CharacterListScrollBox | Scroll Box | Container for character entries |
| CreateCharacterButton | Button | Opens creation dialog |
| EnterWorldButton | Button | Loads game level |

### Event Graph

**Event Construct:**
```
Get Game Instance → Cast to MMOGameInstance
    ↓
Bind Event to OnCharacterListReceived → PopulateCharacterList
    ↓
HttpManager::GetCharacters()
```

**PopulateCharacterList (Custom Event):**
```
Get Game Instance → Get CharacterList
    ↓
Clear Children (CharacterListScrollBox)
    ↓
For Each Loop (CharacterList)
    ├─ Create Widget: WBP_CharacterEntry
    ├─ SetupCharacterData (Array Element)
    ├─ Add Child to CharacterListScrollBox
    └─ Loop Complete
```

**CreateCharacterButton OnClicked:**
```
Remove from Parent
Create Widget: WBP_CreateCharacter
Add to Viewport
```

**EnterWorldButton OnClicked:**
```
Get Game Instance → Get SelectedCharacter
    ↓
Is Valid? → Branch
    ├─ FALSE: Show "Please select a character" message
    └─ TRUE: Open Level ("ThirdPersonMap")
```

---

## WBP_CharacterEntry

### Purpose
Individual character row displayed in WBP_CharacterSelect scroll box. Shows character info and handles selection.

### Hierarchy
```
Canvas Panel
└── Horizontal Box
    ├── Text Block: NameText
    ├── Text Block: ClassText
    ├── Text Block: LevelText
    └── Button: SelectButton
        └── Text Block: "Select"
```

### Variables

| Name | Type | Properties |
|------|------|------------|
| CharacterId | int32 | Expose on Spawn: true, Instance Editable: false |
| CharacterData | FCharacterData | Used internally |

### Functions

**SetupCharacterData:**
```
Input: InCharacterData (FCharacterData)
    ↓
Set NameText: InCharacterData.Name
Set ClassText: InCharacterData.CharacterClass
Set LevelText: "Level " + InCharacterData.Level
Set CharacterId: InCharacterData.CharacterId
```

**SelectButton OnClicked:**
```
Get Game Instance → Cast to MMOGameInstance
    ↓
Select Character (CharacterId)
    ↓
[Visual feedback: highlight selected entry]
```

### Spawn Parameters

When created from WBP_CharacterSelect:
```
Create Widget (WBP_CharacterEntry)
    └─ CharacterId: [from CharacterData.CharacterId]
```

---

## WBP_CreateCharacter

### Purpose
Modal dialog for creating new characters. Collects name and class selection.

### Hierarchy
```
Canvas Panel
└── Scale Box
    └── Size Box (400x350)
        └── Canvas Panel
            ├── Text Block: "Create Character"
            ├── Text Block: "Name"
            ├── Editable Text Box: NameInput
            ├── Text Block: "Class"
            ├── Combo Box String: ClassDropdown
            └── Horizontal Box
                ├── Button: CancelButton
                │   └── Text Block: "Cancel"
                └── Button: CreateButton
                    └── Text Block: "Create"
```

### Components

| Name | Type | Properties |
|------|------|------------|
| NameInput | Editable Text Box | Hint Text: "Character name..." |
| ClassDropdown | Combo Box String | Options: warrior, mage, archer, healer |

### Event Graph

**Event Construct:**
```
ClassDropdown → Add Option: "warrior"
ClassDropdown → Add Option: "mage"
ClassDropdown → Add Option: "archer"
ClassDropdown → Add Option: "healer"
ClassDropdown → Set Selected Option: "warrior"
```

**CreateButton OnClicked:**
```
NameInput → Get Text → To String
ClassDropdown → Get Selected Option
    ↓
Branch (Name != "")
    ├─ FALSE: [Show error - name required]
    └─ TRUE:
        Get Game Instance → Cast to MMOGameInstance
        HttpManager::CreateCharacter(Name, SelectedClass)
        Remove from Parent
```

**CancelButton OnClicked:**
```
Remove from Parent
Create Widget: WBP_CharacterSelect
Add to Viewport
```

---

## Input Mode Reference

### Set Input Mode UI Only
Used when showing any menu/overlay that requires mouse interaction.
```
Set Input Mode UI Only
Set Show Mouse Cursor: true
```

### Set Input Mode Game Only
Used when player should control character without UI interference.
```
Set Input Mode Game Only
Set Show Mouse Cursor: false
```

### Set Input Mode Game and UI
Used for top-down games where player moves with mouse but UI is also interactive.
```
Set Input Mode Game and UI
Set Show Mouse Cursor: true
```

### Widget Usage

| Widget | Input Mode | Mouse Cursor |
|--------|-----------|--------------|
| WBP_LoginScreen | UI Only | true |
| WBP_CharacterSelect | UI Only | true |
| WBP_CreateCharacter | UI Only | true |
| Game World (top-down) | Game and UI | true |
| Game World (FPS) | Game Only | false |

---

## Event Dispatcher Binding

All widgets communicate with GameFlow through GameInstance events:

### GameFlow Setup
```
Get Game Instance → Cast to MMOGameInstance
    ↓
Bind Event to OnLoginSuccess → HandleLoginSuccess
Bind Event to OnLoginFailed → HandleLoginFailed
Bind Event to OnCharacterListReceived → ShowCharacterSelect
Bind Event to OnCharacterCreated → RefreshCharacterList
```

### Widget Response

| GameInstance Event | Widget Action |
|-------------------|---------------|
| OnLoginSuccess | WBP_LoginScreen closes |
| OnLoginFailed | WBP_LoginScreen shows error |
| OnCharacterListReceived | WBP_CharacterSelect populates list |
| OnCharacterCreated | WBP_CharacterSelect refreshes |

---

## Styling Guidelines

### Color Scheme
- Primary Background: Dark (near black)
- Text: White or light gray
- Error Text: Red (#FF0000)
- Buttons: Medium gray with white text
- Selected Item: Highlight color (blue/orange)

### Typography
- Title: 36pt bold
- Labels: 18pt regular
- Button Text: 16pt semi-bold
- Body Text: 14pt regular

### Layout
- Centered on screen
- Fixed sizes with Scale Box for responsiveness
- Consistent padding (10-20px between elements)

---

## Files

### Blueprint Files
- `client/SabriMMO/Content/UI/WBP_LoginScreen.uasset`
- `client/SabriMMO/Content/UI/WBP_CharacterSelect.uasset`
- `client/SabriMMO/Content/UI/WBP_CharacterEntry.uasset`
- `client/SabriMMO/Content/UI/WBP_CreateCharacter.uasset`

### Supporting Files
- `client/SabriMMO/Source/SabriMMO/CharacterData.h` - Data structure
- `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h` - Event dispatchers
- `client/SabriMMO/Content/Blueprints/BP_GameFlow.uasset` - Orchestration
