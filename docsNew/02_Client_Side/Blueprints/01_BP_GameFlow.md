# BP_GameFlow

**Path**: `/Game/SabriMMO/Blueprints/BP_GameFlow`  
**Parent Class**: `Actor`  
**Placed In Level**: Yes — `BP_GameFlow_C_1` at (760, -70, -80)  
**Purpose**: Orchestrates the entire login → character select → enter world flow. The first Blueprint to execute when the game starts.

## Components

| Component | Type | Notes |
|-----------|------|-------|
| `DefaultSceneRoot` | SceneComponent | Root (no visual) |

## Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `bIsLoggedIn` | bool | Tracks login state |
| `Target` | object | General-purpose reference |
| `LoginWidgetRef` | object (WBP_LoginScreen) | Reference to the spawned login widget |
| `GameInstanceRef` | object (UMMOGameInstance) | Cached reference to Game Instance |

## Event Graph (74 nodes)

### Event BeginPlay — Main Flow

```
Event BeginPlay
    ↓
[Sequence]
    ├─ Pin 0: Get Game Instance → Cast To MMOGameInstance → Set GameInstanceRef
    │   ↓
    │   [Bind Event to On Login Success] → OnLoginSuccessEvent (Custom Event)
    │   [Bind Event to On Login Failed] → HandleLoginFailed (Custom Event)
    │   [Bind Event to On Character Created] → OnCharacterCreatedEvent (Custom Event)
    │   [Bind Event to On Character List Received] → OnCharacterListReceivedEvent (Custom Event)
    │
    ├─ Pin 1: Check Auth Status
    │   ↓
    │   Is Authenticated (GameInstanceRef)
    │   ↓ Branch
    │   ├─ TRUE: "already authenticated" → [skip login]
    │   └─ FALSE: Display Login Widget
    │       ↓
    │       Get Player Controller → Set bShowMouseCursor = true
    │       ↓ Delay
    │       Create WBP_LoginScreen Widget → Add to Viewport
    │       Set LoginWidgetRef
    │       Set Input Mode UI Only
    │
    └─ Pin 2: (reserved)
```

### OnLoginSuccessEvent (Custom Event)

```
OnLoginSuccessEvent
    ↓
[Sequence]
    ├─ Pin 0: Remove Login Widget
    │   ↓
    │   Get LoginWidgetRef → Is Valid → Branch
    │   ├─ TRUE: Get Player Controller → Set Input Mode Game And UI
    │   │   → Remove from Parent (LoginWidgetRef)
    │   │   → Set LoginWidgetRef = null
    │   └─ FALSE: (skip)
    │
    └─ Pin 1: Fetch Characters
        ↓
        Delay → Get Characters (UHttpManager static call)
```

### OnCharacterCreatedEvent (Custom Event)

```
OnCharacterCreatedEvent
    ↓
Delay → Get Characters
```

### OnCharacterListReceivedEvent (Custom Event)

```
OnCharacterListReceivedEvent
    ↓
Get Selected Character (GameInstanceRef) → Break Character Data
    ↓
Branch: CharacterId > 0 ?
    ├─ TRUE: "Character already selected, do nothing"
    │   (Print String — debug only)
    └─ FALSE: Go to Character Select
        ↓
        Create WBP_CharacterSelect Widget → Add to Viewport
        Get Player Controller → Set bShowMouseCursor = true
        Set Input Mode UI Only
```

### HandleLoginFailed (Custom Event)

```
HandleLoginFailed
    ↓
Get LoginWidgetRef → Is Valid → Branch
    ├─ TRUE: Get Txt_Error (widget child)
    │   → SetText ("Login failed. Check credentials.")
    │   → Set Visibility (Visible)
    └─ FALSE: (skip)
```

## Design Patterns

| Pattern | Application |
|---------|-------------|
| **Event-Driven** | Binds to GameInstance delegates (OnLoginSuccess, OnLoginFailed, OnCharacterCreated, OnCharacterListReceived) |
| **Game Instance** | Caches GameInstanceRef for repeated access |
| **Widget Lifecycle** | Creates → stores ref → removes → nulls ref |

## Connections to Other Blueprints

| Blueprint | How Connected |
|-----------|---------------|
| `UMMOGameInstance` (C++) | Cast To, bind delegates, call IsAuthenticated/GetSelectedCharacter |
| `UHttpManager` (C++) | Static calls: GetCharacters |
| `WBP_LoginScreen` | Created and managed by this Blueprint |
| `WBP_CharacterSelect` | Created when character list received |

---

**Last Updated**: 2026-02-17  
**Source**: Read via unrealMCP `read_blueprint_content`
