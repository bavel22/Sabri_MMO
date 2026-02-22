# BP_SocketManager

**Path**: `/Game/SabriMMO/Blueprints/BP_SocketManager`  
**Parent Class**: `Actor`  
**Spawned By**: Runtime (GetActorOfClass from other BPs)  
**Purpose**: Central Socket.io hub. Manages the WebSocket connection, binds ALL server events to handler functions, and routes data to managers/widgets. The single most critical Blueprint in the project — every real-time feature flows through here.

**Status**: Phase 1 stabilization complete. Added 10 IsValid guards for HUDManager calls. See refactoring plan: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`

## Components

| Component | Type | Purpose |
|-----------|------|---------|
| `DefaultSceneRoot` | SceneComponent | Root |
| `SocketIO` | SocketIOClientComponent | Socket.io connection (from SocketIOClient plugin) |

## Variables

| Variable | Type | Purpose |
|----------|------|---------|
| `bIsConnected` | bool | Socket connection state |
| `AuthToken` | string | JWT token from GameInstance |
| `JoinJsonString` | string | JSON payload for player:join |
| `ServerURL` | string | Server URL (http://localhost:3001) |
| `CharacterId` | int | Local player's character ID |
| `CharacterName` | string | Local player's character name |
| `StatAllocationWidgetRef` | object | Reference to stat allocation widget |
| `InventoryWindowRef` | object | Reference to inventory window widget |
| `MMOCharaterRef` | object (BP_MMOCharacter) | Reference to local player character |
| `OtherPlayerManagerRef` | object (BP_OtherPlayerManager) | Reference to player manager |
| `EnemyManagerRef` | object (BP_EnemyManager) | Reference to enemy manager |
| `GameHUDWidgetRef` | object (WBP_GameHUD) | Reference to game HUD |
| `bIsTargetingEnemy` | bool | Whether current target is enemy |
| `bIsAutoAttacking` | bool | Whether auto-attack is active |
| `TargetEnemyId` | int | Current targeted enemy ID |
| `PositionTimerHandle` | TimerHandle | Timer for position broadcast |
| `MMOCharacterHudManager` | object (AC_HUDManager) | Cached HUD manager reference |
| `bHUDManagerReady` | bool | Whether HUD manager is initialized |
| `BufferedHealthData` | string (Array) | Buffered health updates until HUD ready |

## Event Graph (182 nodes)

### Event BeginPlay — Connection & Binding Setup

```
Event BeginPlay
    ↓
[Sequence] — All bindings BEFORE connect (critical ordering)
    │
    ├─ Pin 0: Socket Event Bindings (via multiple Sequences)
    │   │
    │   ├─ [Sequence — Misc Player Bindings]
    │   │   ├─ Bind "player:moved" → OnPlayerMoved
    │   │   └─ Bind "player:left" → OnPlayerLeft
    │   │
    │   ├─ [Sequence — Combat Bindings]
    │   │   ├─ Bind "combat:health_update" → OnHealthUpdate
    │   │   ├─ Bind "combat:damage" → OnCombatDamage
    │   │   ├─ Bind "combat:death" → OnCombatDeath
    │   │   ├─ Bind "combat:auto_attack_started" → OnAutoAttackStarted
    │   │   ├─ Bind "combat:auto_attack_stopped" → OnAutoAttackStopped
    │   │   ├─ Bind "combat:target_lost" → OnTargetLost
    │   │   ├─ Bind "combat:out_of_range" → OnCombatOutOfRange
    │   │   ├─ Bind "combat:respawn" → OnCombatRespawn
    │   │   └─ Bind "combat:error" → OnCombatError
    │   │
    │   ├─ [Sequence — Stat Bindings]
    │   │   └─ Bind "player:stats" → OnPlayerStats
    │   │
    │   ├─ [Sequence — Enemy Bindings]
    │   │   ├─ Bind "enemy:spawn" → OnEnemySpawn
    │   │   ├─ Bind "enemy:move" → OnEnemyMove
    │   │   ├─ Bind "enemy:death" → OnEnemyDeath
    │   │   └─ Bind "enemy:health_update" → OnEnemyHealthUpdate
    │   │
    │   ├─ [Sequence — Inventory Bindings]
    │   │   ├─ Bind "inventory:data" → OnInventoryData
    │   │   ├─ Bind "inventory:used" → OnItemUsed
    │   │   ├─ Bind "inventory:equipped" → OnItemEquipped
    │   │   ├─ Bind "inventory:dropped" → OnItemDropped
    │   │   └─ Bind "inventory:error" → OnInventoryError
    │   │
    │   ├─ [Sequence — Loot Bindings]
    │   │   └─ Bind "loot:drop" → OnLootDrop
    │   │
    │   └─ [Sequence — Chat Bindings]
    │       └─ Bind "chat:receive" → OnChatReceived
    │
    ├─ Pin 1: Bind OnConnected → OnSocketConnectedEvent
    │
    └─ Pin 2: Connect to ServerURL via SocketIO component
```

### OnSocketConnectedEvent (Custom Event)

```
OnSocketConnectedEvent
    ↓
Set bIsConnected = true
    ↓
[Sequence]
    ├─ Pin 0: Get GameInstance → Cast To MMOGameInstance
    │   ↓ Branch: IsAuthenticated?
    │   ├─ TRUE:
    │   │   Set AuthToken = GetAuthHeader()
    │   │   Set CharacterId = GetSelectedCharacter().CharacterId
    │   │   Set CharacterName = GetSelectedCharacter().Name
    │   │   ↓
    │   │   Construct JSON: {characterId, token (AuthToken), characterName}
    │   │   ↓
    │   │   SocketIO → Emit("player:join", jsonValue)
    │   └─ FALSE: Print "Not authenticated"
    │
    ├─ Pin 1: Set Manager Refs (with Delay for level load)
    │   ↓ Delay
    │   Get Actor Of Class(BP_OtherPlayerManager) → Set OtherPlayerManagerRef
    │   Get Actor Of Class(BP_EnemyManager) → Set EnemyManagerRef
    │   Get Player Character → Cast To BP_MMOCharacter → Set MMOCharaterRef
    │   Get HUDManager (from MMOCharacter) → Set MMOCharacterHudManager
    │   ↓
    │   Initialize HUD (AC_HUDManager)
    │   Set bHUDManagerReady = true
    │   ↓
    │   Branch: BufferedHealthData.Length > 0?
    │   ├─ TRUE: For Each Loop → OnHealthUpdate(buffered data) → Clear buffer
    │   └─ FALSE: (skip)
    │
    └─ Pin 2: Set Timer "EmitPositionUpdate" (looping, 0.033s = ~30Hz)
```

### Event End Play — Cleanup

```
Event End Play
    ↓
Clear Timer "EmitPositionUpdate"
Clear and Invalidate PositionTimerHandle
SocketIO → Disconnect
```

## Functions (34 total)

### Emit Functions (Client → Server)

| Function | Params | Emits | Description |
|----------|--------|-------|-------------|
| `EmitPositionUpdate` | _(none, reads player pos)_ | `player:position` | Broadcasts local player position at ~30Hz via timer |
| `EmitAttack` | `TargetCharacterId (int), AttackerRef (object)` | `combat:attack` | Determines enemy vs player target (ID > 2M = enemy), emits with `targetEnemyId` or `targetCharacterId` |
| `EmitStartAttack` | — | `combat:attack` | Wrapper for starting attack |
| `EmitStopAttack` | — | `combat:stop_attack` | Emits stop attack event |
| `EmitShop` | `ShopId (int)` | `shop:open` | Formats JSON `{shopId}`, emits `shop:open`. Called from `BP_MMOCharacter` on NPC click. **Guarded**: IsValid(SocketIO) check. ✅ Follows correct emit abstraction pattern |

### Player Event Handlers (Server → Client)

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnPlayerMoved` | 38 | `Data (string)` | Parses characterId, name, x, y, z, health, maxHealth → calls `OtherPlayerManagerRef.SpawnOrUpdatePlayer()` |
| `OnPlayerLeft` | 10 | `Data (string)` | Parses characterId → calls `OtherPlayerManagerRef.RemovePlayer()` |
| `OnPlayerStats` | 39 | `Data (string)` | Parses stats object (str, agi, vit, int, dex, luk, level, statPoints) + derived object → calls `HUDManager.UpdateStats()` |
| `OnPlayerJoined` | ~15 | `Data (string)` | Handles `player:joined` confirmation. Parses `zuzucoin` field → calls `MMOCharacterHudManager → Set PlayerZuzucoin`. **Guarded**: IsValid(MMOCharacterHudManager) |
| `FindRemotePlayer` | 11 | `CharId (int)` → returns Actor | Looks up remote player via `OtherPlayerManagerRef.GetOtherPlayerActor()` |

### Combat Event Handlers

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnAutoAttackStarted` | 32 | `Data` | Parses targetId, targetName, isEnemy, attackRange, aspd, attackIntervalMs → sets `bIsAutoAttacking=true`, `bIsTargetingEnemy`, shows target frame via HUDManager. **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnAutoAttackStopped` | 27 | `Data` | Parses reason → sets `bIsAutoAttacking=false`, hides target frame. **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnCombatDamage` | **215** | `Data` | **Most complex function**. Parses all damage fields. Determines: who is attacker? who is target? (local/remote/enemy). Plays attack animations, rotates actors to face each other, updates health bars via BPI_Damageable, shows damage numbers, updates target frame in HUD. **Guarded**: Multiple IsValid(MMOCharacterHudManager) checks added |
| `OnHealthUpdate` | 50 | `Data` | Parses charId, health, maxHealth, mana, maxMana. If local player → updates HUD. If remote → updates overhead health bar. Buffers data if HUDManager not ready yet |
| `OnCombatOutOfRange` | 30 | `Data` | Parses targetId, isEnemy, targetX/Y/Z, distance, requiredRange → moves local player toward target via Simple Move To Location |
| `OnTargetLost` | 27 | `Data` | Parses reason, isEnemy → stops auto-attack, hides target frame, clears target refs. **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnCombatDeath` | 19 | `Data` | Parses killedId, killerId → if local player died: shows death overlay. If remote player: plays death effects. **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnCombatRespawn` | 72 | `Data` | Parses charId, x, y, z, health, maxHealth, mana, maxMana. If local: teleport to spawn, hide death overlay, restore HUD bars. If remote: teleport actor, update overhead health bar. **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnCombatError` | 3 | `Data` | Prints error message |

### Enemy Event Handlers

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnEnemySpawn` | 41 | `Data` | Parses enemyId, templateId, name, level, health, maxHealth, x, y, z → calls `EnemyManagerRef.SpawnOrUpdateEnemy()` |
| `OnEnemyMove` | 28 | `Data` | Parses enemyId, x, y, z, isMoving → calls `EnemyManagerRef.SpawnOrUpdateEnemy()` to update position |
| `OnEnemyDeath` | 45 | `Data` | Parses enemyId, killerId → calls `EnemyManagerRef` to handle death (disable collision, hide mesh), stops auto-attack if targeting this enemy |
| `OnEnemyHealthUpdate` | 42 | `Data` | Parses enemyId, health, maxHealth, inCombat → updates enemy health bar and combat indicator |

### Inventory Event Handlers

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnInventoryData` | 3 | `Data` | Passes raw JSON to HUDManager.PopulateInventory(). **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnItemUsed` | 17 | `Data` | Parses healed, spRestored, health, mana → updates HUD, shows usage notification |
| `OnItemEquipped` | **72** | `Data` | Parses: `itemName`, `equipped`, `slot`, `attackRange`, `aspd`, `maxHealth`, `maxMana`. Sets `AttackRange` on MMOCharacterRef + TargetingSystem (if equipping). Calls `UpdateLocalPlayerHealth` / `UpdateLocalPlayerMana` with new max values (preserving current). **Guarded**: MMOCharacterRef, MMOCharacterHudManager, GameHudRef null guards. **✅ Working**: Server sends `player:stats` event immediately after `inventory:equipped` (line 1372), so stat window refreshes automatically via existing `OnPlayerStats` handler.
| `OnInventoryError` | 6 | `Data` | Shows error message in inventory UI |
| `OnLootDrop` | 3 | `Data` | Passes loot data to HUDManager.ShowLootPopup(). **Guarded**: IsValid(MMOCharacterHudManager) check added |
| `OnItemDropped` | 4 | `Data` | Refreshes inventory after drop |

### Chat Handler

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnChatReceived` | 3 | `Data` | Passes data to HUDManager.ChatReceived(). **Guarded**: IsValid(MMOCharacterHudManager) check added |

### Shop Event Handlers

| Function | Nodes | Input | Description |
|----------|-------|-------|-------------|
| `OnShopData` | ~20 | `Data (string)` | Handles `shop:data`. Passes raw Data to `MMOCharacterHudManager → OpenShop(Data)`. **Guarded**: IsValid(MMOCharacterHudManager) |
| `OnShopBought` | 20 | `Data (string)` | Handles `shop:bought`. Parses `newZuzucoin` (int) → calls `MMOCharacterHudManager → UpdateZuzucoinEverywhere(newZuzucoin)`. Updates both shop + inventory displays. **Guarded**: IsValid(MMOCharacterHudManager) |
| `OnShopSold` | 23 | `Data (string)` | Handles `shop:sold`. Parses `newZuzucoin` (int) → calls `MMOCharacterHudManager → UpdateZuzucoinEverywhere(newZuzucoin)`. Updates both shop + inventory displays. **Guarded**: IsValid(MMOCharacterHudManager) |
| `OnShopError` | ~8 | `Data (string)` | Handles `shop:error`. Parses `message` → calls `MMOCharacterHudManager → ShowShopError(message)`. **Guarded**: IsValid(MMOCharacterHudManager) |

## OnCombatDamage — Detailed Flow (215 nodes)

This is the most complex function in the entire project. Here's the complete logic:

```
OnCombatDamage(Data: String)
    ↓
Parse JSON → Extract: attackerId, attackerName, targetId, targetName,
    isEnemy, damage, targetHealth, targetMaxHealth,
    attackerX, attackerY, attackerZ, targetX, targetY, targetZ
    ↓
[Step 1: Determine target type]
Branch: TargetId == CharacterId (local player)?
    ├─ YES → Target is Local Player
    │   ↓
    │   Get Player Character location → Set TargetWorldPos
    │   Update Local Player Health (HUDManager)
    │   Show Damage Numbers at player position
    │   ↓
    │   BPI_Damageable: UpdateHealthDisplay on local player
    │   BPI_Damageable: ReceiveDamageVisual (rotate toward attacker)
    │
    └─ NO → Branch: isEnemy == "true"?
        ├─ YES → Target is Enemy
        │   ↓
        │   Get Enemy via EnemyManagerRef → Set TargetEnemyRef
        │   Get enemy actor location → Set TargetWorldPos
        │   ↓
        │   Branch: TargetEnemyId == TargetId? (is this OUR target?)
        │   ├─ YES → Update Target Health in HUD (ShowTargetFrame + UpdateTargetHealth)
        │   └─ NO → (skip target frame update)
        │   ↓
        │   BPI_Damageable: UpdateHealthDisplay on enemy
        │   BPI_Damageable: ReceiveDamageVisual (rotate toward attacker)
        │   Show Damage Numbers at enemy position
        │
        └─ NO → Target is Remote Player
            ↓
            FindRemotePlayer(TargetId) → Set OtherPlayerTargetRef
            Get actor location → Set TargetWorldPos
            ↓
            BPI_Damageable: UpdateHealthDisplay on remote player
            BPI_Damageable: ReceiveDamageVisual (rotate toward attacker)
            Update Target Health in HUD
            Show Damage Numbers

[Step 2: Attacker animation & rotation]
Branch: AttackerId == CharacterId (I am the attacker)?
    ├─ YES → Play Attack Animation on local player (BP_MMOCharacter.PlayAttackAnimation)
    └─ NO → Find remote attacker → rotate toward target using FindLookAtRotation
            → Play Attack Animation on remote player (BP_OtherPlayerCharacter.PlayAttackAnimation)
            → Rotate remote attacker to face target using MakeVector(AttackerX/Y) → FindLookAtRotation → SetActorRotation
```

## EmitAttack — Detailed Flow (25 nodes)

```
EmitAttack(TargetCharacterId: int, AttackerRef: object)
    ↓
Set TargetCharacterIdLocal = TargetCharacterId
    ↓
Branch: TargetCharacterIdLocal > 2000000 AND TargetCharacterIdLocal < 3000000?
    ├─ YES → Enemy target
    │   Construct JSON: {targetEnemyId: TargetCharacterIdLocal}
    │   SocketIO → Emit("combat:attack", json)
    │
    └─ NO → Player target
        Construct JSON: {targetCharacterId: TargetCharacterIdLocal (as string)}
        SocketIO → Emit("combat:attack", json)
```

## EmitPositionUpdate — Detailed Flow (21 nodes)

```
EmitPositionUpdate() — called every 0.033s by timer
    ↓
Branch: bIsConnected AND CharacterId > 0?
    ├─ TRUE:
    │   Get Player Character → Get Actor Location → Break Vector
    │   Construct JSON: {characterId (string), x, y, z (floats)}
    │   SocketIO → Emit("player:position", json)
    └─ FALSE: (skip)
```

## Socket Event Binding Summary

| Server Event | → Handler Function | Category |
|-------------|-------------------|----------|
| `player:moved` | `OnPlayerMoved` | Player |
| `player:left` | `OnPlayerLeft` | Player |
| `player:stats` | `OnPlayerStats` | Stats |
| `combat:health_update` | `OnHealthUpdate` | Combat |
| `combat:damage` | `OnCombatDamage` | Combat |
| `combat:death` | `OnCombatDeath` | Combat |
| `combat:auto_attack_started` | `OnAutoAttackStarted` | Combat |
| `combat:auto_attack_stopped` | `OnAutoAttackStopped` | Combat |
| `combat:target_lost` | `OnTargetLost` | Combat |
| `combat:out_of_range` | `OnCombatOutOfRange` | Combat |
| `combat:respawn` | `OnCombatRespawn` | Combat |
| `combat:error` | `OnCombatError` | Combat |
| `enemy:spawn` | `OnEnemySpawn` | Enemy |
| `enemy:move` | `OnEnemyMove` | Enemy |
| `enemy:death` | `OnEnemyDeath` | Enemy |
| `enemy:health_update` | `OnEnemyHealthUpdate` | Enemy |
| `inventory:data` | `OnInventoryData` | Inventory |
| `inventory:used` | `OnItemUsed` | Inventory |
| `inventory:equipped` | `OnItemEquipped` | Inventory |
| `inventory:dropped` | `OnItemDropped` | Inventory |
| `inventory:error` | `OnInventoryError` | Inventory |
| `loot:drop` | `OnLootDrop` | Loot |
| `chat:receive` | `OnChatReceived` | Chat |
| `player:joined` | `OnPlayerJoined` | Player |
| `shop:data` | `OnShopData` | Shop |
| `shop:bought` | `OnShopBought` | Shop |
| `shop:sold` | `OnShopSold` | Shop |
| `shop:error` | `OnShopError` | Shop |

## Connections to Other Blueprints

| Blueprint | How Connected | Functions Called |
|-----------|--------------|-----------------|
| `BP_MMOCharacter` | MMOCharaterRef (via GetPlayerCharacter + Cast) | PlayAttackAnimation |
| `BP_OtherPlayerManager` | OtherPlayerManagerRef (via GetActorOfClass) | SpawnOrUpdatePlayer, RemovePlayer, GetOtherPlayerActor |
| `BP_OtherPlayerCharacter` | Via FindRemotePlayer | PlayAttackAnimation, UpdateHealthBar, Set TargetPosition |
| `BP_EnemyManager` | EnemyManagerRef (via GetActorOfClass) | SpawnOrUpdateEnemy, RemoveEnemy, GetEnemyActor |
| `BP_EnemyCharacter` | Via EnemyManager | UpdateEnemyHealth, OnEnemyDeath, InitializeEnemy |
| `AC_HUDManager` | MMOCharacterHudManager (via MMOCharacter.HUDManager) | InitializeHUD, UpdateLocalPlayerHealth/Mana, UpdateTargetHealth, ShowTargetFrame, HideTargetFrame, ShowDeathOverlay, HideDeathOverlay, UpdateStats, PopulateInventory, ShowLootPopup, ShowDamageNumbers, ChatReceived, Set PlayerZuzucoin, OpenShop, UpdateZuzucoinEverywhere, ShowShopError |
| `UMMOGameInstance` (C++) | Via GetGameInstance + Cast | IsAuthenticated, GetAuthHeader, GetSelectedCharacter |
| `BPI_Damageable` | Interface messages on targets | UpdateHealthDisplay, ReceiveDamageVisual |

## Design Patterns

| Pattern | Application |
|---------|-------------|
| **Event-Driven** | All Socket.io events bound to handler functions |
| **Manager** | Routes events to OtherPlayerManager and EnemyManager |
| **Buffering** | BufferedHealthData array stores health updates until HUDManager is ready |
| **Hub/Mediator** | Central coordinator between server and all client Blueprints |

## Critical Implementation Notes

1. **Binding order**: ALL `Bind Event to Function` calls happen BEFORE `Connect` to prevent missed events
2. **Delay for refs**: Manager references are set after a Delay to ensure level actors are spawned
3. **HUD buffering**: Health updates received before HUDManager initialization are buffered and replayed
4. **Enemy ID detection**: IDs > 2,000,000 are treated as enemy IDs in `EmitAttack`
5. **Position timer**: Broadcasts at ~30Hz (0.033s interval) via `Set Timer by Function Name`
6. **Emit abstraction**: All socket emissions use named functions (e.g., `EmitShop`, `EmitAttack`). `BP_MMOCharacter` and other Blueprints NEVER call SocketIO directly — they call `BP_SocketManager` functions.

---

## Safety Nets Added (Phase 1)

### HUDManager Null Guards Implemented

| Function | Guard Type | Reason |
|----------|------------|--------|
| OnCombatDeath | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnAutoAttackStarted | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnAutoAttackStopped | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnTargetLost | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnInventoryData | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnLootDrop | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnChatReceived | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnPlayerStats | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnCombatRespawn | IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized |
| OnCombatDamage | Multiple IsValid(MMOCharacterHudManager) | Prevents crash if HUDManager not initialized (5-6 locations) |

### Existing Protection (No Changes Needed)
- **OnHealthUpdate**: Already has `bHUDManagerReady` buffering system
- **All other handlers**: Do not call HUDManager directly

### Implementation Pattern
```
[Parse JSON] → [Business Logic] → Get MMOCharacterHudManager
    ↓
IsValid(MMOCharacterHudManager)
    ├─ TRUE → Call HUDManager function
    └─ FALSE → Skip (or Print debug message)
```

### Performance Impact
- **Neutral**: IsValid checks add minimal overhead (<0.01ms per call)
- **Positive**: Prevents crashes during level load timing issues
- **Positive**: Graceful degradation if HUDManager fails to initialize

### Future Phases
See complete refactoring plan: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`

---

**Last Updated**: 2026-02-22 (Feature #20 NPC Shop: added EmitShop function; added OnPlayerJoined handler (parses zuzucoin); added shop event handlers: OnShopData, OnShopBought, OnShopSold, OnShopError; shop bindings added to BeginPlay; function count 29→34. ⚠️ unrealMCP scan unavailable — node counts approximate)

**Previous**: 2026-02-18 (Phase 1 safety nets implemented. ✅ Confirmed: OnItemEquipped stat window refresh working via server player:stats event)  
**Source**: Read via unrealMCP `read_blueprint_content` + `get_blueprint_function_details`  
**Related**: `@C:/Sabri_MMO/docsNew/05_Development/AC_HUDManager_Refactoring_Plan.md:1`
