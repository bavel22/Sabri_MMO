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

---

## In-Game Widgets

---

## WBP_GameHud

### Purpose
Main in-game HUD overlay. Shows player health/mana bars, player name, and target frame. Created by BP_SocketManager after `player:join` emit succeeds.

### Hierarchy
```
Canvas Panel
├── Vertical Box (top-left)
│   ├── Text Block: PlayerNameText
│   ├── Size Box (overridden width: 200)
│   │   └── Progress Bar: HealthBar (green fill)
│   ├── Size Box (overridden width: 200)
│   │   └── Text Block: HealthText (e.g. "100/100")
│   ├── Size Box (overridden width: 200)
│   │   └── Progress Bar: ManaBar (blue fill)
│   └── Size Box (overridden width: 200)
│       └── Text Block: ManaText (e.g. "100/100")
├── Vertical Box (top-right, target frame)
│   ├── Text Block: TargetNameText
│   ├── Size Box (overridden width: 200)
│   │   └── Progress Bar: TargetHealthBar
│   ├── Size Box (overridden width: 200)
│   │   └── Text Block: TargetHealthText
│   ├── Size Box (overridden width: 200)
│   │   └── Progress Bar: TargetManaBar
│   └── Size Box (overridden width: 200)
│       └── Text Block: TargetManaText
└── (optional) WBP_ChatBox (anchored bottom-left)
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| HealthBar | Progress Bar | Player HP visual fill |
| ManaBar | Progress Bar | Player MP visual fill |
| HealthText | Text Block | Player HP numeric "current/max" |
| ManaText | Text Block | Player MP numeric "current/max" |
| PlayerNameText | Text Block | Player character name |
| TargetNameText | Text Block | Target name (hidden when no target) |
| TargetHealthBar | Progress Bar | Target HP fill |
| TargetHealthText | Text Block | Target HP numeric |
| TargetManaBar | Progress Bar | Target MP fill |
| TargetManaText | Text Block | Target MP numeric |

### Functions

**UpdateHealth(CurrentHealth, MaxHealth):**
- Sets HealthBar Percent = CurrentHealth / MaxHealth
- Sets HealthText = CurrentHealth + "/" + MaxHealth

**UpdateMana(CurrentMana, MaxMana):**
- Sets ManaBar Percent = CurrentMana / MaxMana
- Sets ManaText = CurrentMana + "/" + MaxMana

**UpdateTargetHealth(TargetName, CurrentHP, MaxHP, CurrentMP, MaxMP):**
- Sets TargetNameText = TargetName
- Sets TargetHealthBar Percent = CurrentHP / MaxHP
- Sets TargetHealthText = CurrentHP + "/" + MaxHP
- Sets TargetManaBar Percent = CurrentMP / MaxMP
- Sets TargetManaText = CurrentMP + "/" + MaxMP
- Sets target frame Visibility = Visible

**HideTargetFrame():**
- Sets target frame Visibility = Hidden

### Size Box Rationale
Size Boxes with overridden widths (200px) prevent the progress bars and text from resizing when values change (e.g., "100/100" → "9/100"). This ensures a consistent HUD layout.

### Creation
Created in **BP_SocketManager** after `player:joined` is received, not in BP_MMOCharacter. This ensures the HUD only appears once the socket connection is established and the player has successfully joined the server.

---

## HoverOverIndicator (In-World)

### Purpose
A **Widget Component** attached to `BP_OtherPlayerCharacter` (and `BP_EnemyCharacter`) that displays a selection ring/highlight and the character's name when the player hovers the mouse cursor over the actor. This is the RO-style "hover to see name" mechanic.

### Component Setup (on BP_OtherPlayerCharacter / BP_EnemyCharacter)
- **Type**: Widget Component
- **Name**: HoverOverIndicator
- **Space**: Screen (or World if preferred)
- **Widget Class**: WBP_PlayerNameTag (or a dedicated WBP_HoverIndicator widget)
- **Draw at Desired Size**: Checked
- **Visibility**: Hidden by default

### Visibility Logic
The HoverOverIndicator should be **visible** when:
1. The mouse cursor is hovering over the actor (detected via `OnBeginCursorOver`)
2. The actor is the current target (selected for auto-attack)

The HoverOverIndicator should be **hidden** when:
1. The mouse cursor leaves the actor (detected via `OnEndCursorOver`)
2. AND the actor is NOT the current active target

### Blueprint Logic (BP_OtherPlayerCharacter / BP_EnemyCharacter)

**OnBeginCursorOver:**
```
Event OnBeginCursorOver
    ↓
HoverOverIndicator → Set Visibility: Visible
```

**OnEndCursorOver:**
```
Event OnEndCursorOver
    ↓
Branch: IsActiveTarget?
    ├─ TRUE: (do nothing — keep visible because it's the active target)
    └─ FALSE: HoverOverIndicator → Set Visibility: Hidden
```

**When Targeted (via combat:auto_attack_started):**
```
Set IsActiveTarget = true
HoverOverIndicator → Set Visibility: Visible
```

**When Target Lost (via combat:target_lost or combat:stop_attack):**
```
Set IsActiveTarget = false
HoverOverIndicator → Set Visibility: Hidden
```

### Required Settings
- The actor must have **"Generate Overlap Events"** or cursor events enabled
- In the Player Controller or BP_MMOCharacter, ensure **"Show Mouse Cursor"** is true
- Ensure the actor has a collision profile that responds to cursor traces

---

## WBP_DeathOverlay

### Purpose
Full-screen semi-transparent overlay shown when the player dies. Contains a "Return to Save Point" button that triggers respawn.

### Hierarchy
```
Canvas Panel (full screen)
└── Border (semi-transparent black overlay, anchored to fill)
    └── Vertical Box (centered)
        ├── Text Block: "You Died" (large red text)
        ├── Spacer (40px)
        └── Button: ReturnToSavePointButton
            └── Text Block: "Return to Save Point"
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| ReturnToSavePointButton | Button | Triggers combat:respawn emit |

### Event Graph

**ReturnToSavePointButton OnClicked:**
```
Get BP_SocketManager reference (e.g. from GameInstance)
    ↓
BP_SocketManager → Emit("combat:respawn", {})
    ↓
Remove from Parent (removes the overlay)
```

### Creation
Added to viewport by the **OnCombatDeath** function in BP_MMOCharacter or BP_SocketManager when `combat:death` is received for the local player's characterId.

### Removal
Removed from viewport when the respawn button is clicked (Remove from Parent). The `combat:respawn` response will handle resetting position and health.

---

## WBP_DamageNumber

### Purpose
Floating damage text shown above targets when damage is dealt. Spawned as a world-space widget or 3D text actor at the target's location.

### Hierarchy
```
Canvas Panel
└── Text Block: DamageText (bold, outlined)
```

### Variables

| Name | Type | Default | Purpose |
|------|------|---------|---------|
| DamageText | Text Block | — | Displays damage amount |
| DamageAmount | Integer | 0 | Set before adding to viewport |
| DamageColor | Linear Color | White | Color of the text |
| FloatSpeed | Float | 50.0 | Upward movement speed |
| FadeTime | Float | 1.5 | Seconds before fade-out |

### Color Rules
- **White**: Player is dealing damage (attacker is local player)
- **Red**: Player is taking damage (target is local player)
- **Yellow** (optional): Critical hit

### Design Notes (TODO)
- **Float Upward**: Damage text should float upward from the target's head position. Currently static — implement world-space widget with timeline animation or use a 3D text actor with upward velocity.
- **Implementation approach**: Spawn a `WBP_DamageNumber` widget component at the target location, play a timeline that moves it upward by ~100 units over 1.5 seconds while fading opacity to 0, then destroy.

### Creation
Spawned by the `OnCombatDamage` handler in BP_SocketManager or BP_MMOCharacter when `combat:damage` is received. The handler checks:
- If `attackerId` == local player's characterId → spawn **white** text at target
- If `targetId` == local player's characterId → spawn **red** text at self

---

## WBP_TargetHealthBar

### Purpose
Small health bar widget displayed **above a target's head** (attached to their actor via a Widget Component). Shows when the target is in combat or selected.

### Hierarchy
```
Canvas Panel
└── Vertical Box
    ├── Progress Bar: HealthBar (small, red/green fill)
    └── (optional) Text Block: HealthText
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| HealthBar | Progress Bar | HP visual fill |
| HealthText | Text Block | Optional HP numeric display |

### Functions

**UpdateHealth(NewHealth, NewMaxHealth):**
```
Set HealthBar Percent = NewHealth / NewMaxHealth
(Optional) Set HealthText = NewHealth + "/" + NewMaxHealth
```

### Visibility Rules for Enemies
Enemy health bars (on `BP_EnemyCharacter`) should **only be visible** when the enemy is in combat with the local player or another player. The server broadcasts `enemy:health_update` with an `inCombat` boolean field.

- **When `enemy:health_update` received with `inCombat: true`**: Set WBP_TargetHealthBar visibility to Visible
- **When `enemy:health_update` received with `inCombat: false`**: Set WBP_TargetHealthBar visibility to Hidden
- **When `enemy:death` received**: Set visibility to Hidden

### Attachment
Attached as a **Widget Component** on `BP_OtherPlayerCharacter` or `BP_EnemyCharacter`, positioned above the capsule/mesh.

---

## WBP_PlayerNameTag

### Purpose
World-space widget displaying a player or enemy name above their head. Attached via Widget Component.

### Hierarchy
```
Canvas Panel
└── Text Block: NameText (white, outlined, centered)
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| NameText | Text Block | Character/enemy name display |

### Usage
- Set `NameText` to the character name when spawning `BP_OtherPlayerCharacter` or `BP_EnemyCharacter`
- For enemies: format as "EnemyName Lv.X" (e.g., "Blobby Lv.1")

---

## WBP_ChatBox

### Purpose
In-game chat panel anchored to bottom-left of HUD. Contains a scrollable message list and text input.

### Hierarchy
```
Canvas Panel
├── Scroll Box: MessageScrollBox
│   └── (children: WBP_ChatMessageLine instances)
└── Horizontal Box
    ├── Editable Text Box: ChatInput
    └── Button: SendButton
        └── Text Block: "Send"
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| MessageScrollBox | Scroll Box | Container for chat messages |
| ChatInput | Editable Text Box | Text input field |
| SendButton | Button | Sends chat message |

### Functions

**AddChatMessage(FormattedMessage: String):**
```
Create Widget: WBP_ChatMessageLine
    ↓
Set LineMessageText = FormattedMessage
    ↓
Add Child to MessageScrollBox
    ↓
Scroll to End of MessageScrollBox
```

**SendButton OnClicked / ChatInput OnTextCommitted (Enter key):**
```
ChatInput → Get Text → To String → Variable: MessageText
    ↓
Branch (MessageText != "")
    ├─ TRUE:
    │   BP_SocketManager → Emit("chat:message", { channel: "GLOBAL", message: MessageText })
    │   ChatInput → Set Text: ""
    └─ FALSE: (do nothing)
```

---

## WBP_ChatMessageLine

### Purpose
Single chat message line displayed in WBP_ChatBox scroll box.

### Hierarchy
```
Canvas Panel
└── Text Block: MessageTextBlock
```

### Variables

| Name | Type | Properties |
|------|------|------------|
| LineMessageText | String | Instance Editable: true, Blueprint Read Write: true |

### Binding
- Select `MessageTextBlock` in Designer → Details → Text → Bind → Create Binding
- In binding function: drag `LineMessageText` variable → connect to Return Value

---

## WBP_StatAllocation

### Purpose
Stat allocation panel (RO-style) for distributing stat points into STR, AGI, VIT, INT, DEX, LUK. Opened via a UI button or hotkey.

### Hierarchy
```
Canvas Panel
└── Border (dark background)
    └── Vertical Box
        ├── Text Block: "Status Window" (title)
        ├── Text Block: StatPointsText ("Remaining: 48")
        ├── [For each stat: STR, AGI, VIT, INT, DEX, LUK]
        │   Horizontal Box
        │   ├── Text Block: StatLabel (e.g. "STR")
        │   ├── Text Block: StatValueText (e.g. "1")
        │   └── Button: IncreaseButton ("+")
        └── Button: CloseButton ("Close")
```

### Variables

| Name | Type | Purpose |
|------|------|---------|
| StatPointsText | Text Block | Remaining stat points |
| STR_Value / AGI_Value / etc. | Text Block | Current stat values |
| STR_Button / AGI_Button / etc. | Button | "+" buttons for each stat |

### Functions

**UpdateStats(Stats, Derived):**
- Called when `player:stats` event received
- Sets each stat text block to the corresponding value
- Sets StatPointsText to "Remaining: " + stats.statPoints
- Disables "+" buttons if statPoints == 0

**OnIncrease_STR / OnIncrease_AGI / etc. (Button OnClicked):**
```
BP_SocketManager → Emit("player:allocate_stat", { statName: "str", amount: 1 })
```
(Replace "str" with the appropriate stat for each button)

### Detailed Blueprint Instructions
See the **Blueprint Instructions** section below for full step-by-step creation guide.

---

## Widget Relationship Diagram

```
BP_SocketManager
    ├── Creates → WBP_GameHud (after player:joined)
    ├── Updates → WBP_GameHud.UpdateHealth (on combat:health_update)
    ├── Updates → WBP_GameHud.UpdateMana (on combat:health_update)
    ├── Updates → WBP_GameHud.UpdateTargetHealth (on combat:damage for target)
    ├── Creates → WBP_DeathOverlay (on combat:death for local player)
    ├── Spawns → WBP_DamageNumber (on combat:damage)
    └── Updates → WBP_ChatBox.AddChatMessage (on chat:receive)

BP_OtherPlayerCharacter / BP_EnemyCharacter
    ├── Has → WBP_PlayerNameTag (Widget Component, above head)
    ├── Has → WBP_TargetHealthBar (Widget Component, above head)
    └── Has → HoverOverIndicator (Widget Component, selection ring/highlight)

BP_MMOCharacter
    ├── References → WBP_GameHud (for local player HUD)
    └── OnCombatDeath → Creates WBP_DeathOverlay
```
