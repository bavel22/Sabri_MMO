# C++ Module Overview

## Module: SabriMMO

**Build File**: `Source/SabriMMO/SabriMMO.Build.cs`  
**PCH Mode**: `UseExplicitOrSharedPCHs`  
**Engine**: Unreal Engine 5.7  
**Build Settings**: `BuildSettingsVersion.V6`, `EngineIncludeOrderVersion.Unreal5_7`

## Public Dependencies

| Module | Purpose |
|--------|---------|
| `Core` | Core engine types, containers, delegates |
| `CoreUObject` | UObject system, reflection, GC |
| `Engine` | Engine subsystems, GameInstance, GameMode |
| `InputCore` | Input key/axis types |
| `EnhancedInput` | Enhanced Input System (UE5) |
| `AIModule` | AI controllers, navigation |
| `StateTreeModule` | StateTree behavior system |
| `GameplayStateTreeModule` | Gameplay-specific StateTree tasks |
| `UMG` | Unreal Motion Graphics (UI) |
| `Slate` | Low-level UI framework |
| `HTTP` | HTTP request module |
| `Json` | JSON parsing/serialization |
| `JsonUtilities` | JSON utility functions |

## Include Paths

```
SabriMMO/
SabriMMO/Variant_Platforming/
SabriMMO/Variant_Platforming/Animation/
SabriMMO/Variant_Combat/
SabriMMO/Variant_Combat/AI/
SabriMMO/Variant_Combat/Animation/
SabriMMO/Variant_Combat/Gameplay/
SabriMMO/Variant_Combat/Interfaces/
SabriMMO/Variant_Combat/UI/
SabriMMO/Variant_SideScrolling/
SabriMMO/Variant_SideScrolling/AI/
SabriMMO/Variant_SideScrolling/Gameplay/
SabriMMO/Variant_SideScrolling/Interfaces/
SabriMMO/Variant_SideScrolling/UI/
```

## File Inventory

### Core Module (19 files)

| File | Lines | Purpose |
|------|-------|---------|
| `SabriMMO.h` | 8 | Module header, declares `LogSabriMMO` log category |
| `SabriMMO.cpp` | 8 | Module implementation, defines `LogSabriMMO` |
| `SabriMMO.Build.cs` | 56 | Build configuration, dependencies |
| `SabriMMO.Target.cs` | 16 | Game build target |
| `SabriMMOEditor.Target.cs` | 16 | Editor build target |
| `CharacterData.h` | 52 | `FCharacterData` USTRUCT |
| `MMOGameInstance.h` | 79 | `UMMOGameInstance` header |
| `MMOGameInstance.cpp` | 73 | `UMMOGameInstance` implementation |
| `MMOHttpManager.h` | 53 | `UHttpManager` header |
| `MMOHttpManager.cpp` | 537 | `UHttpManager` REST API client |
| `SabriMMOCharacter.h` | 97 | `ASabriMMOCharacter` base character |
| `SabriMMOCharacter.cpp` | 134 | Base character implementation |
| `SabriMMOPlayerController.h` | 53 | `ASabriMMOPlayerController` header |
| `SabriMMOPlayerController.cpp` | 68 | Player controller implementation |
| `SabriMMOGameMode.h` | 25 | `ASabriMMOGameMode` header |
| `SabriMMOGameMode.cpp` | 9 | Game mode stub |
| `OtherCharacterMovementComponent.h` | 18 | Empty CharacterMovementComponent subclass |
| `OtherCharacterMovementComponent.cpp` | 6 | Empty implementation |

### Variant_Combat (42 files)

| Subdirectory | Files | Key Classes |
|-------------|-------|-------------|
| Root | 6 | `ACombatCharacter`, `ACombatPlayerController`, `ACombatGameMode` |
| AI/ | 8 | `ACombatEnemy`, `ACombatAIController`, `ACombatEnemySpawner`, `CombatStateTreeUtility`, `EnvQueryContext_Danger/Player` |
| Animation/ | 6 | `AnimNotify_CheckChargedAttack`, `AnimNotify_CheckCombo`, `AnimNotify_DoAttackTrace` |
| Gameplay/ | 10 | `CombatActivationVolume`, `CombatCheckpointVolume`, `CombatDamageableBox`, `CombatDummy`, `CombatLavaFloor` |
| Interfaces/ | 6 | `ICombatAttacker`, `ICombatDamageable`, `ICombatActivatable` |
| UI/ | 2 | `UCombatLifeBar` |

### Variant_Platforming (8 files)

| Subdirectory | Files | Key Classes |
|-------------|-------|-------------|
| Root | 6 | `APlatformingCharacter`, `APlatformingPlayerController`, `APlatformingGameMode` |
| Animation/ | 2 | `AnimNotify_EndDash` |

### Variant_SideScrolling (26 files)

| Subdirectory | Files | Key Classes |
|-------------|-------|-------------|
| Root | 8 | `ASideScrollingCharacter`, `ASideScrollingPlayerController`, `ASideScrollingGameMode`, `ASideScrollingCameraManager` |
| AI/ | 6 | `ASideScrollingAIController`, `ASideScrollingNPC`, `SideScrollingStateTreeUtility` |
| Gameplay/ | 8 | `SideScrollingJumpPad`, `SideScrollingMovingPlatform`, `SideScrollingPickup`, `SideScrollingSoftPlatform` |
| Interfaces/ | 2 | `ISideScrollingInteractable` |
| UI/ | 2 | `SideScrollingUI` |

## Class Hierarchy

```
UObject
├── UGameInstance
│   └── UMMOGameInstance              # Auth state, character data, delegates
├── UBlueprintFunctionLibrary
│   └── UHttpManager                  # Static REST API functions
├── UUserWidget
│   └── UCombatLifeBar                # Life bar widget (abstract)
├── UCharacterMovementComponent
│   └── UOtherCharacterMovementComponent  # Remote player movement
└── UInterface
    ├── UCombatAttacker → ICombatAttacker
    ├── UCombatDamageable → ICombatDamageable
    └── UCombatActivatable → ICombatActivatable

AActor
├── ACharacter
│   ├── ASabriMMOCharacter            # Base player (abstract)
│   ├── ACombatCharacter              # Combat player (abstract)
│   │   implements ICombatAttacker, ICombatDamageable
│   ├── ACombatEnemy                  # AI enemy (abstract)
│   │   implements ICombatAttacker, ICombatDamageable
│   ├── APlatformingCharacter         # Platforming variant
│   └── ASideScrollingCharacter       # Side-scrolling variant
├── AGameModeBase
│   ├── ASabriMMOGameMode             # Base game mode (abstract)
│   ├── ACombatGameMode               # Combat game mode (abstract)
│   ├── APlatformingGameMode
│   └── ASideScrollingGameMode
└── APlayerController
    ├── ASabriMMOPlayerController      # Base controller (abstract)
    ├── ACombatPlayerController        # Combat controller (abstract)
    ├── APlatformingPlayerController
    └── ASideScrollingPlayerController

AAIController
└── ACombatAIController               # StateTree AI (abstract)
```

---

**Last Updated**: 2026-02-17
