# Blueprint Cross-Reference Map

## Overview

This document maps every connection between every Blueprint in the Sabri_MMO project. Use this as the definitive reference for understanding data flow, call chains, and dependencies. When modifying any Blueprint, consult this map to identify all downstream effects.

## Complete Blueprint Inventory

### Game Actors (8)
| Blueprint | Path | Parent | Nodes | Functions | Variables |
|-----------|------|--------|-------|-----------|-----------|
| `BP_GameFlow` | `/Game/SabriMMO/Blueprints/` | Actor | 74 | 0 | 4 |
| `BP_MMOCharacter` | `/Game/SabriMMO/Blueprints/` | BP_ThirdPersonCharacter | 203 | 1 | 21 |
| `BP_ThirdPersonCharacter` | `/Game/SabriMMO/Blueprints/` | ASabriMMOCharacter (C++) | 10 | 0 | 0 |
| `BP_SocketManager` | `/Game/SabriMMO/Blueprints/` | Actor | 182 | 29 | 19 |
| `BP_OtherPlayerManager` | `/Game/SabriMMO/Blueprints/` | Actor | 3 | 3 | 2 |
| `BP_OtherPlayerCharacter` | `/Game/SabriMMO/Blueprints/` | Character | 42 | 2 | 6 |
| `BP_EnemyManager` | `/Game/SabriMMO/Blueprints/` | Actor | 3 | 3 | 3 |
| `BP_EnemyCharacter` | `/Game/SabriMMO/Blueprints/` | Character | 77 | 5 | 11 |

### Actor Components (3)
| Component | Path | Parent | Functions | Variables |
|-----------|------|--------|-----------|-----------|
| `AC_HUDManager` | `/Game/SabriMMO/Components/` | ActorComponent | **25** | 10 |
| `AC_CameraController` | `/Game/SabriMMO/Components/` | ActorComponent | 4 | 9 |
| `AC_TargetingSystem` | `/Game/SabriMMO/Components/` | ActorComponent | 3 | 13 |

### Widget Blueprints (15)
| Widget | Path | Parent | Nodes | Functions | Variables |
|--------|------|--------|-------|-----------|-----------|
| `WBP_LoginScreen` | `/Game/SabriMMO/Widgets/` | UserWidget | 39 | 0 | 2 |
| `WBP_CharacterSelect` | `/Game/SabriMMO/Widgets/` | UserWidget | 31 | 0 | 1 |
| `WBP_CharacterEntry` | `/Game/SabriMMO/Widgets/` | UserWidget | 10 | 1 | 1 |
| `WBP_CreateCharacter` | `/Game/SabriMMO/Widgets/` | UserWidget | 30 | 0 | 0 |
| `WBP_GameHUD` | `/Game/SabriMMO/Widgets/` | UserWidget | 3 | 0 | 8 |
| `WBP_PlayerNameTag` | `/Game/SabriMMO/Widgets/` | UserWidget | 3 | 1 | 1 |
| `WBP_ChatWidget` | `/Game/SabriMMO/Widgets/` | UserWidget | 17 | 0 | 3 |
| `WBP_ChatMessageLine` | `/Game/SabriMMO/Widgets/` | UserWidget | 3 | 1 | 1 |
| `WBP_StatAllocation` | `/Game/SabriMMO/Widgets/` | UserWidget | 36 | 0 | 3 |
| `WBP_TargetHealthBar` | `/Game/SabriMMO/Widgets/` | UserWidget | 3 | 1 | 0 |
| `WBP_InventoryWindow` | `/Game/SabriMMO/Widgets/` | UserWidget | 10 | 0 | 3 |
| `WBP_InventorySlot` | `/Game/SabriMMO/Widgets/` | UserWidget | 23 | 0 | 15 |
| `WBP_DeathOverlay` | `/Game/SabriMMO/Widgets/` | UserWidget | 16 | 0 | 1 |
| `WBP_LootPopup` | `/Game/SabriMMO/Widgets/` | UserWidget | 22 | 0 | 3 |
| `WBP_DamageNumber` | `/Game/SabriMMO/Widgets/` | UserWidget | 17 | 0 | 4 |

### Interfaces (1)
| Interface | Path | Functions |
|-----------|------|-----------|
| `BPI_Damageable` | `/Game/SabriMMO/Interfaces/` | ReceiveDamageVisual, UpdateHealthDisplay, GetHealthInfo |

### C++ Classes Used by Blueprints
| Class | Used By |
|-------|---------|
| `UMMOGameInstance` | BP_GameFlow, BP_SocketManager, WBP_CharacterSelect, WBP_CharacterEntry, WBP_CreateCharacter, BP_MMOCharacter |
| `UHttpManager` | WBP_LoginScreen (LoginUser), WBP_CreateCharacter (CreateCharacter), BP_GameFlow (GetCharacters) |
| `ASabriMMOCharacter` | BP_ThirdPersonCharacter (inherits) |
| `FCharacterData` | BP_GameFlow (Break struct), WBP_CharacterEntry (SetupCharacterData), BP_MMOCharacter (name tag) |

## Connection Dependency Graph

```
                          ┌─────────────────────┐
                          │    UE5 Engine        │
                          │  (Level Load)        │
                          └──────────┬───────────┘
                                     │ Spawns actors in level
                                     ▼
                          ┌─────────────────────┐
                          │   BP_GameFlow        │◄── Only BP placed in level
                          │   (Login Flow)       │
                          └──────────┬───────────┘
                                     │ Creates widgets
                          ┌──────────┼──────────────────────────┐
                          ▼          ▼                          ▼
                 WBP_LoginScreen  WBP_CharacterSelect   (binds GameInstance delegates)
                          │          │
                          │          ├── WBP_CharacterEntry (per character)
                          │          └── WBP_CreateCharacter
                          │
                          │ On login success → level transition
                          ▼
              ┌──────────────────────────────────────────────┐
              │              GAME LEVEL                       │
              │                                              │
              │  ┌────────────────────────────────────────┐  │
              │  │          BP_SocketManager               │  │
              │  │  (Central Hub - 29 functions)           │  │
              │  │                                        │  │
              │  │  Connects to server via SocketIO       │  │
              │  │  Binds 23 socket events                │  │
              │  │  Routes data to all other BPs          │  │
              │  └────────┬──────────┬──────────┬─────────┘  │
              │           │          │          │             │
              │     ┌─────┘    ┌─────┘    ┌─────┘             │
              │     ▼          ▼          ▼                   │
              │  ┌────────┐ ┌────────┐ ┌────────────────┐    │
              │  │BP_Other│ │BP_Enemy│ │BP_MMOCharacter  │    │
              │  │Player  │ │Manager │ │(Local Player)   │    │
              │  │Manager │ │        │ │                 │    │
              │  └───┬────┘ └───┬────┘ │ Components:     │    │
              │      │          │      │ ├─AC_HUDManager │    │
              │      ▼          ▼      │ ├─AC_Camera     │    │
              │  BP_Other   BP_Enemy   │ └─AC_Targeting  │    │
              │  Player     Character  └────────┬────────┘    │
              │  Character                      │             │
              │                                 │             │
              │              ┌──────────────────┘             │
              │              ▼                                │
              │  ┌────────────────────────────────────────┐   │
              │  │          AC_HUDManager                  │   │
              │  │  (25 functions - Widget Controller)     │   │
              │  │                                        │   │
              │  │  Creates/manages ALL widgets:          │   │
              │  │  ├─ WBP_GameHUD (HP/MP bars)          │   │
              │  │  ├─ WBP_ChatWidget + ChatMessageLine  │   │
              │  │  ├─ WBP_InventoryWindow + Slots       │   │
              │  │  ├─ WBP_StatAllocation                │   │
              │  │  ├─ WBP_DeathOverlay                  │   │
              │  │  ├─ WBP_LootPopup                     │   │
              │  │  └─ WBP_DamageNumber                  │   │
              │  └────────────────────────────────────────┘   │
              └──────────────────────────────────────────────┘
```

## Function Call Map

### BP_SocketManager → Other Blueprints

| Socket Event | Handler | Calls On | Function Called |
|-------------|---------|----------|-----------------|
| `player:moved` | OnPlayerMoved | BP_OtherPlayerManager | SpawnOrUpdatePlayer |
| `player:left` | OnPlayerLeft | BP_OtherPlayerManager | RemovePlayer |
| `player:stats` | OnPlayerStats | AC_HUDManager | UpdateStats |
| `combat:health_update` | OnHealthUpdate | AC_HUDManager | UpdateLocalPlayerHealth, UpdateLocalPlayerMana |
| `combat:health_update` | OnHealthUpdate | BP_OtherPlayerCharacter | UpdateHealthBar (via GetOtherPlayerActor) |
| `combat:damage` | OnCombatDamage | BP_MMOCharacter | PlayAttackAnimation |
| `combat:damage` | OnCombatDamage | BP_OtherPlayerCharacter | PlayAttackAnimation, SetActorRotation |
| `combat:damage` | OnCombatDamage | BP_EnemyCharacter | BPI_Damageable messages |
| `combat:damage` | OnCombatDamage | AC_HUDManager | UpdateTargetHealth, ShowTargetFrame, UpdateLocalPlayerHealth, ShowDamageNumbers |
| `combat:death` | OnCombatDeath | AC_HUDManager | ShowDeathOverlay |
| `combat:auto_attack_started` | OnAutoAttackStarted | AC_HUDManager | ShowTargetFrame, UpdateTargetHealth |
| `combat:auto_attack_stopped` | OnAutoAttackStopped | AC_HUDManager | HideTargetFrame |
| `combat:target_lost` | OnTargetLost | AC_HUDManager | HideTargetFrame |
| `combat:out_of_range` | OnCombatOutOfRange | BP_MMOCharacter | Simple Move To Location (via controller) |
| `combat:respawn` | OnCombatRespawn | AC_HUDManager | HideDeathOverlay, UpdateLocalPlayerHealth/Mana |
| `combat:respawn` | OnCombatRespawn | BP_OtherPlayerCharacter | Set TargetPosition, UpdateHealthBar |
| `enemy:spawn` | OnEnemySpawn | BP_EnemyManager | SpawnOrUpdateEnemy |
| `enemy:move` | OnEnemyMove | BP_EnemyManager | SpawnOrUpdateEnemy (position update) |
| `enemy:death` | OnEnemyDeath | BP_EnemyCharacter | OnEnemyDeath |
| `enemy:health_update` | OnEnemyHealthUpdate | BP_EnemyCharacter | UpdateEnemyHealth |
| `inventory:data` | OnInventoryData | AC_HUDManager | PopulateInventory |
| `inventory:used` | OnItemUsed | AC_HUDManager | UpdateLocalPlayerHealth/Mana |
| `inventory:equipped` | OnItemEquipped | AC_HUDManager | PopulateInventory (refresh) |
| `loot:drop` | OnLootDrop | AC_HUDManager | ShowLootPopup |
| `chat:receive` | OnChatReceived | AC_HUDManager | ChatReceived |

### BP_MMOCharacter → Other Blueprints

| Action | Target | Function/Property |
|--------|--------|-------------------|
| IA_Attack (hit player) | BP_SocketManager | EmitAttack(CharacterId) |
| IA_Attack (hit enemy) | BP_SocketManager | EmitAttack(EnemyId) |
| IA_Attack (hit nothing) | BP_SocketManager | EmitStopAttack |
| IA_ToggleInventory | AC_HUDManager | ToggleInventory |
| IA_CameraRotate | AC_CameraController | SetRotating(true/false) |
| IA_Look | AC_CameraController | SetLookInput(Vector2D) |
| IA_Zoom | AC_CameraController | HandleZoom(AxisValue) |
| Event Tick | AC_CameraController | HandleCameraRotation(DeltaTime) |
| Event Tick | AC_TargetingSystem | UpdateHoverDetection |
| Event Tick | AC_TargetingSystem | CheckIfInAttackRangeOfTargetPlayer/Enemy |

### Widget → AC_HUDManager Calls

| Widget | Action | AC_HUDManager Function |
|--------|--------|------------------------|
| WBP_StatAllocation | STR/AGI/VIT/INT/DEX/LUK buttons | SendStatIncreaseRequest("stat") |
| WBP_StatAllocation | Close button | ToggleStats |
| WBP_ChatWidget | Send button / Enter key | SendChatMessageRequest |
| WBP_InventoryWindow | Close button | ToggleInventory |
| WBP_InventorySlot | Slot click | OnSlotAction delegate → HandleSlotAction |
| WBP_DeathOverlay | Respawn button | SendRespawnRequest |

## GetActorOfClass References

These lookups find singleton actors at runtime:

| Caller | Finds | Variable Stored In |
|--------|-------|--------------------|
| BP_SocketManager (BeginPlay) | BP_OtherPlayerManager | OtherPlayerManagerRef |
| BP_SocketManager (BeginPlay) | BP_EnemyManager | EnemyManagerRef |
| BP_MMOCharacter (BeginPlay) | BP_SocketManager | SocketManagerRef |
| BP_MMOCharacter (BeginPlay) | BP_EnemyManager | EnemyManagerRef |
| BP_MMOCharacter (BeginPlay) | BP_OtherPlayerManager | OtherPlayerManagerRef |
| AC_HUDManager (BeginPlay) | BP_SocketManager | SocketManagerRef |
| WBP_ChatWidget (Construct) | BP_MMOCharacter | → gets HUDManager component |
| WBP_StatAllocation (Construct) | BP_MMOCharacter | → gets HUDManager component |
| WBP_InventoryWindow (Construct) | BP_MMOCharacter | → gets HUDManager component |
| WBP_DeathOverlay (Construct) | BP_MMOCharacter | → gets HUDManager component |

## Data Flow: Server Event → UI Display

### Example: Player Takes Damage

```
1. Server combat tick calculates damage
2. Server broadcasts combat:damage to all clients
3. BP_SocketManager.OnCombatDamage(jsonString) fires
4. Parses JSON → extracts attackerId, targetId, damage, health, positions
5. Branch: targetId == my CharacterId → I got hit!
    a. AC_HUDManager.UpdateLocalPlayerHealth(health, maxHealth) → updates WBP_GameHUD HP bar
    b. AC_HUDManager.ShowDamageNumbers(worldPos, damage) → creates WBP_DamageNumber at screen position
    c. BPI_Damageable.ReceiveDamageVisual(attackerPos) → BP_MMOCharacter rotates toward attacker
    d. BPI_Damageable.UpdateHealthDisplay(health, maxHealth) → updates overhead health bar
6. Branch: attackerId == my CharacterId → I dealt the hit!
    a. BP_MMOCharacter.PlayAttackAnimation() → plays attack montage
7. Branch: attacker is remote player?
    a. FindRemotePlayer(attackerId) → BP_OtherPlayerCharacter
    b. SetActorRotation(FindLookAtRotation(attackerPos, targetPos)) → face target
    c. PlayAttackAnimation() on remote player
```

### Example: Enemy Spawns

```
1. Server startup / respawn timer fires
2. Server broadcasts enemy:spawn
3. BP_SocketManager.OnEnemySpawn(jsonString) fires
4. Parses JSON → extracts enemyId, name, level, health, x, y, z
5. BP_EnemyManager.SpawnOrUpdateEnemy(enemyId, ..., x, y, z)
6. EnemyManager checks EnemyMap:
    a. New enemy → SpawnActor(BP_EnemyCharacter) at (x, y, z)
       → BP_EnemyCharacter.InitializeEnemy(id, name, level, health, maxHealth)
       → Sets name tag "Name Lv.X", initializes health bar
       → Add to EnemyMap[enemyId]
    b. Existing → Update TargetPosition (for respawned enemies)
```

### Example: Opening Inventory

```
1. Player presses IA_ToggleInventory key
2. BP_MMOCharacter → AC_HUDManager.ToggleInventory()
3. AC_HUDManager:
    a. If not open: Create WBP_InventoryWindow → Add to Viewport
       → SocketManagerRef.SocketIO.Emit("inventory:load")
       → Set bIsInventoryOpen = true
    b. If open: Remove WBP_InventoryWindow → Set bIsInventoryOpen = false
4. Server receives inventory:load → queries DB → emits inventory:data
5. BP_SocketManager.OnInventoryData(jsonString) fires
6. AC_HUDManager.PopulateInventory(jsonString)
    → Parses items array → For each item: Create WBP_InventorySlot
    → Set all item properties → Bind OnSlotAction delegate → Add to grid
7. Player clicks item slot → WBP_InventorySlot fires OnSlotAction delegate
8. AC_HUDManager.HandleSlotAction(inventoryId, itemType, bEquip)
    → Routes to SendEquipRequest / SendUseItemRequest / SendDropRequest
9. Server processes → emits updated inventory:data → cycle repeats
```

## Impact Analysis: What Breaks If You Change...

| If You Change... | These Blueprints Are Affected |
|-----------------|-------------------------------|
| **Socket.io event name** on server | BP_SocketManager (binding string), all handlers |
| **JSON field name** in server event | BP_SocketManager handler (Try Get String Field calls) |
| **BP_MMOCharacter component name** | Widgets using GetActorOfClass → Get Component |
| **AC_HUDManager function signature** | BP_SocketManager, all widgets calling HUDManager |
| **BPI_Damageable function signature** | BP_MMOCharacter, BP_OtherPlayerCharacter, BP_EnemyCharacter, BP_SocketManager.OnCombatDamage |
| **BP_OtherPlayerManager.SpawnOrUpdatePlayer params** | BP_SocketManager.OnPlayerMoved |
| **BP_EnemyManager.SpawnOrUpdateEnemy params** | BP_SocketManager.OnEnemySpawn, OnEnemyMove |
| **WBP_InventorySlot variable names** | AC_HUDManager.PopulateInventory |
| **CharacterId field on any actor** | BP_SocketManager (multiple handlers), BP_MMOCharacter |
| **UMMOGameInstance delegates** | BP_GameFlow (all 4 bindings) |
| **UHttpManager static functions** | WBP_LoginScreen, WBP_CreateCharacter, BP_GameFlow |

## Refactoring Impact: AC_HUDManager Decomposition

**Current State**: AC_HUDManager is a God Component (25 functions) managing all UI widgets

**Phase 1 Complete** (2026-02-17): Added 17 null guards + removed Event Tick
**Phase 2 Planned**: Widget pooling for damage numbers, chat, inventory
**Phase 3 Planned**: Split into 5 focused components:
- AC_HealthDisplayManager (6 functions)
- AC_CombatUIManager (4 functions) 
- AC_InventoryUIManager (5 functions)
- AC_StatsUIManager (3 functions)
- AC_ChatUIManager (3 functions)

**Impact of Phase 3 Split**:
| Blueprint | Current Calls | Future Calls | Changes Needed |
|-----------|---------------|-------------|----------------|
| BP_SocketManager | MMOCharacterHudManager → UpdateLocalPlayerHealth | MMOCharacter → Get HealthDisplayManager → UpdateLocalPlayerHealth | 10 function calls updated |
| BP_SocketManager | MMOCharacterHudManager → ShowDeathOverlay | MMOCharacter → Get CombatUIManager → ShowDeathOverlay | 3 function calls updated |
| BP_SocketManager | MMOCharacterHudManager → PopulateInventory | MMOCharacter → Get InventoryUIManager → PopulateInventory | 2 function calls updated |
| BP_SocketManager | MMOCharacterHudManager → UpdateStats | MMOCharacter → Get StatsUIManager → UpdateStats | 1 function call updated |
| BP_SocketManager | MMOCharacterHudManager → ChatReceived | MMOCharacter → Get ChatUIManager → ChatReceived | 1 function call updated |
| WBP_StatAllocation | HUDManager → SendStatIncreaseRequest | MMOCharacter → Get StatsUIManager → SendStatIncreaseRequest | 1 function call updated |
| WBP_ChatWidget | HUDManager → SendChatMessageRequest | MMOCharacter → Get ChatUIManager → SendChatMessageRequest | 1 function call updated |
| WBP_InventoryWindow | HUDManager → ToggleInventory | MMOCharacter → Get InventoryUIManager → ToggleInventory | 1 function call updated |
| WBP_DeathOverlay | HUDManager → SendRespawnRequest | MMOCharacter → Get CombatUIManager → SendRespawnRequest | 1 function call updated |

**Alternative**: Keep AC_HUDManager as facade (minimal BP_SocketManager changes) |

## Total Blueprint Statistics

| Metric | Count |
|--------|-------|
| **Total Blueprints** | 27 (8 actors + 3 components + 15 widgets + 1 interface) |
| **Total Event Graph Nodes** | ~660 |
| **Total Function Nodes** | ~1,400+ |
| **Total Variables** | ~160 |
| **Total Functions** | ~70 |
| **Socket.io Bindings** | 23 events |
| **BPI_Damageable Implementors** | 3 |
| **Widgets managed by AC_HUDManager** | 9 |

---

**Last Updated**: 2026-02-17 (Added AC_HUDManager refactoring impact analysis)  
**Source**: All data read via unrealMCP `read_blueprint_content` + `get_blueprint_function_details`  
**Related**: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`
