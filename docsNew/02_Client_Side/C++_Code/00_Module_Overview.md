# C++ Module Overview

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Project Overview](../../00_Project_Overview.md) | [System_Architecture](../../01_Architecture/System_Architecture.md)
> **Source**: `client/SabriMMO/Source/SabriMMO/` — 19 core files, 66 UI files, 6 VFX files, 76 variant files

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
| `SlateCore` | Core Slate widgets and rendering |
| `SocketIOClient` | Socket.io client plugin |
| `SIOJson` | JSON utilities for Socket.io |
| `HTTP` | HTTP request module |
| `Json` | JSON parsing/serialization |
| `JsonUtilities` | JSON utility functions |
| `Niagara` | Niagara VFX system |
| `NiagaraCore` | Core Niagara types |
| `NavigationSystem` | NavMesh queries for ground-snap (`ProjectPointToNavigation`) |

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
SabriMMO/UI/
```

## File Inventory

### Core Module

| File | Purpose |
|------|---------|
| `SabriMMO.h/.cpp` | Module header, declares `LogSabriMMO` log category |
| `SabriMMO.Build.cs` | Build configuration, 16 module dependencies (incl. NavigationSystem) |
| `CharacterData.h` | 6 USTRUCTs (FCharacterData 32 fields, FInventoryItem 31 fields, FDraggedItem, FShopItem, FCartItem, FServerInfo), 2 UENUMs |
| `MMOGameInstance.h/.cpp` | Auth state, server selection, zone state, 9 delegates, 13 UFUNCTIONs |
| `MMOHttpManager.h/.cpp` | Static REST API client (9 endpoints + 2 bridge functions), configurable URL |
| `SabriMMOCharacter.h/.cpp` | Base player pawn — movement, socket events, stats, input bindings, BeginPlay ground-snap |
| `SabriMMOPlayerController.h/.cpp` | **Dead code** — not used at runtime (Blueprint PC inherits APlayerController) |
| `SabriMMOGameMode.h/.cpp` | Base GameMode — sets DefaultPawnClass=nullptr |
| `OtherCharacterMovementComponent.h/.cpp` | Remote player movement — per-tick floor-snap via line trace (Z correction >50 units) |
| `WarpPortal.h/.cpp` | Overlap trigger actor for zone warp portals |
| `KafraNPC.h/.cpp` | Clickable Kafra NPC actor |
| `ShopNPC.h/.cpp` | Clickable shop NPC actor |
| `CastingCircleActor.h/.cpp` | Ground casting circle VFX actor |

### UI Subsystems (34 subsystems + 30+ Slate widgets)

| Subsystem | Widget | Z | Toggle | Socket Events |
|-----------|--------|---|--------|---------------|
| `LoginFlowSubsystem` | SLoginWidget, SServerSelectWidget, SCharacterSelectWidget, SCharacterCreateWidget, SLoadingOverlayWidget, Background | 200-250 | — | GameInstance delegates. Return-from-game path skips to CharacterSelect. |
| `EscapeMenuSubsystem` | SEscapeMenuWidget | 40 | ESC | — (reads CombatActionSubsystem::IsDead() for dead state) |
| `BasicInfoSubsystem` | SBasicInfoWidget | 10 | — | combat:damage, skill:effect_damage, exp:gain, combat:health_update |
| `CombatStatsSubsystem` | SCombatStatsWidget | 12 | F8 | player:stats |
| `InventorySubsystem` | SInventoryWidget | 14 | F6 | inventory:data/equipped/dropped/used/error |
| `EquipmentSubsystem` | SEquipmentWidget | 15 | F7 | inventory:data |
| `HotbarSubsystem` | SHotbarRowWidget ×4, SHotbarKeybindWidget | 16, 30 | F5 | hotbar:alldata |
| `ShopSubsystem` | SShopWidget | 18 | — | shop:data/bought/sold/error |
| `KafraSubsystem` | SKafraWidget | 19 | — | kafra:data/saved/teleported/error |
| `SkillTreeSubsystem` | SSkillTreeWidget, SSkillTargetingOverlay | 20 | K | skill:data/learned/refresh/error |
| `DamageNumberSubsystem` | SDamageNumberOverlay | 20 | — | combat:damage, skill:effect_damage |
| `CastBarSubsystem` | SCastBarOverlay | 25 | — | skill:cast_start/complete/interrupted |
| `WorldHealthBarSubsystem` | SWorldHealthBarOverlay | 8 | — | combat:health_update, enemy:health_update |
| `ZoneTransitionSubsystem` | SLoadingOverlayWidget | 50 | — | zone:change/error, player:teleport. Static `SnapLocationToGround()` for NavMesh+trace ground correction |
| `MinimapSubsystem` | SMinimapWidget, SWorldMapWidget | 6, 45 | Tab/M/N | map:world_data, map:party_positions, map:mark, zone:change |
| `SkillVFXSubsystem` | — (Niagara/Cascade) | — | — | skill:effect_damage, skill:buff_applied/removed, skill:ground_effect_* |

### Map System Files

| File | Purpose |
|------|---------|
| `UI/MinimapSubsystem.h/.cpp` | Combined minimap + world map subsystem: SceneCapture2D setup, zoom/opacity, EventRouter handlers (map:world_data, map:party_positions, map:mark), entity dot projection, world map grid data cache |
| `UI/SMinimapWidget.h/.cpp` | 128x128 minimap widget (top-right): renders SceneCapture RT + entity dots (enemies, players, party, warps) + zone name label, zoom +/- buttons |
| `UI/SWorldMapWidget.h/.cpp` | Fullscreen world map: 12x8 grid overlay on continent illustration, zone category tinting, hover tooltips, Tab=monsters, N=zone names, M/Esc=close, party member cross-zone dots |

### Additional UI Files

| File | Purpose |
|------|---------|
| `UI/SkillDragDropOperation.h/.cpp` | Skill drag-drop operation for hotbar |
| `VFX/SkillVFXData.h/.cpp` | Skill VFX configuration data (BuildSkillVFXConfigs) |
| `VFX/SkillVFXSubsystem.h/.cpp` | VFX subsystem (patterns A-E: Bolt, AoE Projectile, Multi-Hit, Persistent Buff, Ground AoE Rain) |

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
├── UWorldSubsystem (33 subsystems)
│   ├── ULoginFlowSubsystem          # Login flow state machine + 5 widgets
│   ├── UBasicInfoSubsystem          # HP/SP/EXP bars
│   ├── UCombatStatsSubsystem        # Detailed stats panel (F8) + Advanced Stats
│   ├── UInventorySubsystem          # Inventory grid (F6)
│   ├── UEquipmentSubsystem          # Equipment slots (F7)
│   ├── UHotbarSubsystem             # 4×9 hotbar (F5)
│   ├── UShopSubsystem               # NPC shop buy/sell
│   ├── UKafraSubsystem              # Kafra NPC services
│   ├── USkillTreeSubsystem          # Skill tree + targeting (K)
│   ├── UDamageNumberSubsystem       # Floating damage numbers
│   ├── UCastBarSubsystem            # Cast bars for all players
│   ├── UWorldHealthBarSubsystem     # Floating HP/SP bars
│   ├── UZoneTransitionSubsystem     # Zone warp + loading overlay
│   ├── UBuffBarSubsystem            # Buff icons + timers
│   ├── UChatSubsystem               # Chat window + combat log
│   ├── UEnemySubsystem              # Enemy actor registry + 5 events
│   ├── UOtherPlayerSubsystem        # Other player registry + 2 events
│   ├── UNameTagSubsystem            # Entity name tags overlay
│   ├── UCombatActionSubsystem       # 10 combat events + target frame + death overlay
│   ├── UMultiplayerEventSubsystem   # Outbound emit helpers
│   ├── UCameraSubsystem             # Right-click yaw + scroll zoom
│   ├── UPlayerInputSubsystem        # Click-to-move/attack/interact
│   ├── UTargetingSubsystem          # 30Hz hover trace + cursor
│   ├── UPositionBroadcastSubsystem  # 30Hz position updates
│   ├── UCartSubsystem               # Cart inventory (F10)
│   ├── UVendingSubsystem            # Vending shop system
│   ├── UItemInspectSubsystem        # Item inspection panel
│   ├── UPartySubsystem              # Party management + HP bars
│   ├── UCraftingSubsystem           # Pharmacy/crafting UI
│   ├── USummonSubsystem             # Summon Flora/Marine Sphere
│   ├── UPetSubsystem                # Pet taming/feeding/commands
│   ├── UHomunculusSubsystem         # Homunculus management
│   ├── UCompanionVisualSubsystem    # Pet/homunculus/mount visuals
│   ├── UMinimapSubsystem            # Minimap + world map (SceneCapture2D, 12x8 grid)
│   └── USkillVFXSubsystem           # Niagara VFX (97+ configs)
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

**Last Updated**: 2026-03-09

### Sprite System

| File | Purpose |
|------|---------|
| `Sprite/SpriteAtlasData.h` | `FSingleAnimAtlasInfo`, `FSpriteAtlasKey`, `GetSpriteScreenBounds()` — atlas data structs |
| `Sprite/SpriteCharacterActor.h/.cpp` | Billboarded sprite actor — v2 atlas loading, weapon modes, animation state machine, equipment layers |

`SpriteCharacterActor` is used for both player characters and enemies. Player characters have their sprite managed by `SabriMMOCharacter` (local) or `OtherPlayerSubsystem` (remote). Enemy sprites are managed by `EnemySubsystem`.

**Enemy sprite support in EnemySubsystem**: When the server sends `spriteClass` in an `enemy:spawn` event, `EnemySubsystem` spawns a `SpriteCharacterActor` attached to the enemy, hides the BP actor's mesh components, and drives sprite animations from all 5 enemy event handlers (spawn, move, attack, health_update, death). `FEnemyEntry` stores `SpriteActor`, `SpriteClass`, and `WeaponMode` fields. Name tags are registered on the sprite actor for billboard tracking. The `SetBodyClass(const FString&)` overload searches `Body/{name}/`, `Body/enemies/{name}/`, and `Body/` for atlas manifests.
