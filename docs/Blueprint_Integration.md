# Blueprint Integration

## Overview

Blueprint integration documentation for the MMO's event-driven architecture. Covers GameFlow orchestration, event bindings, and UI coordination.

## BP_GameFlow

### Purpose
Central orchestrator Blueprint that manages game flow, UI transitions, and event handling. Exists in game level or as persistent actor.

### Event Bindings Setup

**Event BeginPlay:**
```
Sequence
    ├─ Then 1: Bind OnLoginSuccess
    ├─ Then 2: Bind OnLoginFailed  
    ├─ Then 3: Bind OnCharacterListReceived
    └─ Then 4: Bind OnCharacterCreated

[After all bindings]
    ↓
Create WBP_LoginScreen → Add to Viewport → Store: LoginWidget
```

### Event Handlers

**HandleLoginSuccess:**
```
Custom Event: HandleLoginSuccess
    ↓
Get LoginWidget → Is Valid? → Branch
    ├─ TRUE:
    │   Remove from Parent
    │   Set Input Mode Game and UI
    │   Set Show Mouse Cursor: false
    │   Clear LoginWidget variable
    └─ FALSE: [Continue]
    ↓
HttpManager::GetCharacters()
```

**HandleLoginFailed:**
```
Custom Event: HandleLoginFailed
    ↓
Get LoginWidget → Cast to WBP_LoginScreen
    ↓
Get Txt_Error → Set Text: "Invalid username or password"
Get Txt_Error → Set Visibility: Visible
```

**ShowCharacterSelect:**
```
Custom Event: ShowCharacterSelect (bound to OnCharacterListReceived)
    ↓
Create WBP_CharacterSelect
Add to Viewport
Set Input Mode UI Only
Set Show Mouse Cursor: true
```

**RefreshCharacterList:**
```
Custom Event: RefreshCharacterList (bound to OnCharacterCreated)
    ↓
HttpManager::GetCharacters()
```

### GameInstance Access Pattern

```
Get Game Instance → Cast to MMOGameInstance
    ↓
[Access properties/events]
    ├─ Get AuthToken
    ├─ Get CharacterList
    ├─ Set SelectedCharacter
    ├─ Bind Event to OnLoginSuccess
    └─ [etc]
```

## Event Binding Pattern

### Standard Binding Flow

```
1. Get Game Instance
2. Cast to MMOGameInstance
3. Drag off Cast Result
4. Select "Bind Event to [EventName]"
5. Drag off Event pin (red)
6. Select/Create Custom Event
```

### Available Events

| Event | Fired When | Typical Handler |
|-------|-----------|-----------------|
| OnLoginSuccess | JWT token received | Close login, get characters |
| OnLoginFailed | 401 or connection fail | Show error message |
| OnCharacterListReceived | Character list loaded | Show character select |
| OnCharacterCreated | New character created | Refresh list |

### Event Timing

**Important:** Bind events BEFORE triggering actions

```
✓ CORRECT:
Bind Event to OnLoginSuccess
    ↓
Login User
    ↓
[Event fires → handler called]

✗ WRONG:
Login User
    ↓
Bind Event to OnLoginSuccess
    ↓
[Event already fired → handler never called]
```

## UI State Management

### Input Mode Transitions

| State | Input Mode | Mouse Cursor |
|-------|-----------|--------------|
| Login Screen | UI Only | true |
| Character Select | UI Only | true |
| Character Creation | UI Only | true |
| In-Game (Top-down) | Game and UI | true |
| In-Game (FPS) | Game Only | false |

### Widget Lifecycle

**Creating Widget:**
```
Create Widget (Class)
    ↓
Add to Viewport
    ├─ ZOrder: 100 (top layer)
    └─ Player Controller
    ↓
Store reference in variable (for later removal)
```

**Removing Widget:**
```
Get Widget Reference
    ↓
Remove from Parent
    ↓
Clear variable (set to None/Empty)
```

### Z-Order Layering

| Z-Order | Widget Type | Example |
|---------|-------------|---------|
| 0 | Base UI | HUD elements |
| 100 | Screens | WBP_LoginScreen |
| 200 | Modals | WBP_CreateCharacter |
| 300 | Popups | Error dialogs |

## Common Blueprint Patterns

### Validation Pattern

```
Get Input Value
    ↓
Branch (Input != "")
    ├─ FALSE:
    │   Show Error Text
    │   Set Error Visibility: Visible
    └─ TRUE:
        Hide Error
        Proceed with action
```

### Loading Pattern

```
Show Loading Indicator
    ↓
Call HTTP Function
    ↓
[Wait for response]
    ↓
Hide Loading Indicator
Process Result
```

### Cast Safety Pattern

```
Get Object → Cast to ExpectedType
    ↓
Is Valid? → Branch
    ├─ TRUE: Proceed with cast result
    └─ FALSE: Log error / Show fallback
```

## Widget Communication

### GameFlow → Widget

**Direct Reference (stored variable):**
```
Get LoginWidget → Cast to WBP_LoginScreen
    ↓
Call Function / Set Property
```

**Event Broadcast (decoupled):**
```
GameFlow fires event
    ↓
All bound handlers execute
    ↓
Widgets respond appropriately
```

### Widget → GameFlow

**Through GameInstance events:**
```
[WBP_CharacterEntry]
Select Button Clicked
    ↓
Get Game Instance → SelectCharacter(ID)
    ↓
[Other systems can listen to selection changes]
```

## Debugging Blueprints

### Useful Nodes

**Print String:**
```
"Event fired!"
"Character ID: " + [ID variable]
```

**Is Valid Checks:**
```
Get Widget → Is Valid? → Branch
Get Controller → Is Valid? → Branch
```

**Breakpoints:**
- Right-click node → "Add Breakpoint"
- Execution pauses when node is hit

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Event not firing | Binding after action | Bind in BeginPlay before any calls |
| Widget not showing | Not added to viewport | Check Add to Viewport node |
| Cast failing | Wrong object type | Check object source |
| Mouse not working | Input mode wrong | Set Input Mode UI Only |
| UI frozen | Input mode set to Game Only | Set to UI Only or Game and UI |

## Best Practices

1. **Always bind events in BeginPlay** before any network calls
2. **Store widget references** when creating (for later cleanup)
3. **Clear widget references** after Remove from Parent
4. **Use Is Valid checks** before casting or calling functions
5. **Set input mode explicitly** when showing/hiding UI
6. **Use Z-order** to layer UI elements properly
7. **Handle failure cases** - network errors, invalid inputs

## Files

- `client/SabriMMO/Content/Blueprints/BP_GameFlow.uasset`
- `client/SabriMMO/Source/SabriMMO/MMOGameInstance.h` (event definitions)
