# Socket Event Migration Plan

**Status:** NOT STARTED
**Date:** 2026-03-13
**Scope:** Migrate socket event handlers from BP_SocketManager to C++ subsystems
**Depends on:** Master Migration Plan (00_Master_Migration_Plan.md), Phases 3-4

---

## Current Event Routing Architecture

### How Events Flow Today

The persistent socket lives on `UMMOGameInstance` (established in Phase 4). All inbound Socket.io events are dispatched through `USocketEventRouter`, which supports multiple handlers per event name.

Two categories of consumers exist:

1. **C++ subsystems** -- Register directly with `USocketEventRouter` in `OnWorldBeginPlay`. Each subsystem handles its own event parsing and logic. Examples: `DamageNumberSubsystem`, `BasicInfoSubsystem`, `BuffBarSubsystem`, `CastBarSubsystem`, `InventorySubsystem`, `EquipmentSubsystem`, `ShopSubsystem`, `CombatStatsSubsystem`, `HotbarSubsystem`, `KafraSubsystem`, `ZoneTransitionSubsystem`, `SkillTreeSubsystem`, `WorldHealthBarSubsystem`, `SkillVFXSubsystem`.

2. **MultiplayerEventSubsystem (BP bridge)** -- Registers handlers for 31 events. Each handler serializes the `FJsonValue` to a JSON string and calls the corresponding Blueprint function on `BP_SocketManager` via `ProcessEvent`. BP_SocketManager's handler functions then parse the JSON string back into Blueprint variables and execute Blueprint graph logic.

### The Result: Dual Handling

Many events are handled by BOTH a C++ subsystem AND the BP bridge simultaneously. `USocketEventRouter` dispatches to all registered handlers for an event name, so both paths execute.

Example: `combat:damage` currently triggers:
- `DamageNumberSubsystem::HandleCombatDamage` (floating damage numbers)
- `BasicInfoSubsystem::HandleCombatDamage` (local HP bar update)
- `WorldHealthBarSubsystem::HandleCombatDamage` (enemy/player HP bars)
- `MultiplayerEventSubsystem` -> `BP_SocketManager::OnCombatDamage` (215 nodes: attack animation, facing, targeting state, hit reaction, remote player animations)

The BP handler does work that no C++ subsystem currently covers -- primarily attack animation triggering, target facing rotation, and combat state management. This is the logic that `UCombatActionSubsystem` will absorb.

### BP_SocketManager's Current Role

BP_SocketManager is placed in every game level as an actor. Its `SocketIOClient` component is already disconnected (the persistent socket on GameInstance replaced it in Phase 4). It serves purely as a handler shell -- its Blueprint functions are called by `MultiplayerEventSubsystem` via `ProcessEvent`.

It has approximately 34 handler functions receiving events, plus emit functions (`EmitCombatAttack`, `EmitStopAttack`, `EmitShop`, `EmitPositionUpdate`). Several emit functions have already been superseded:
- `EmitCombatAttack` / `EmitStopAttack` -- now on `MultiplayerEventSubsystem` (`EmitCombatAttack`, `EmitStopAttack`)
- `EmitPositionUpdate` -- now on `PositionBroadcastSubsystem` (Phase 4)

---

## Events to Migrate (Remove from BP Bridge)

### Phase 3: Combat Events (to UCombatActionSubsystem)

These events control the auto-attack state machine, attack animation triggering, death handling, and respawn flow. BP_SocketManager's handler functions for these events contain the most complex Blueprint graphs in the project.

| Event | BP Function | Node Count | C++ Handler | What It Does |
|-------|------------|------------|-------------|-------------|
| `combat:damage` | OnCombatDamage | ~215 | `UCombatActionSubsystem::HandleCombatDamage` | Parse damage payload, check if local player is attacker/target, face target, play attack animation, trigger hit reaction, handle dual wield second hit (damage2), update combat state |
| `combat:auto_attack_started` | OnAutoAttackStarted | ~32 | `UCombatActionSubsystem::HandleAutoAttackStarted` | Set attacking state flag, store target reference, begin attack animation loop timing |
| `combat:auto_attack_stopped` | OnAutoAttackStopped | ~27 | `UCombatActionSubsystem::HandleAutoAttackStopped` | Clear attacking state, stop attack animation, reset targeting |
| `combat:target_lost` | OnTargetLost | ~27 | `UCombatActionSubsystem::HandleTargetLost` | Target died or despawned, clear target reference, return to idle state |
| `combat:out_of_range` | OnCombatOutOfRange | ~30 | `UPlayerInputSubsystem::HandleOutOfRange` | Target moved out of attack range, begin chase (walk to target) |
| `combat:death` | OnCombatDeath | ~19 | `UCombatActionSubsystem::HandleCombatDeath` | Handle local player death (show death overlay, disable input) or remote entity death |
| `combat:respawn` | OnCombatRespawn | ~72 | `UCombatActionSubsystem::HandleCombatRespawn` | Restore local player: set position, reset HP, hide death overlay, re-enable input, reset camera |
| `combat:error` | OnCombatError | ~3 | `UCombatActionSubsystem::HandleCombatError` | Display error message (e.g., "Target too far", "Cannot attack") |

**Note:** `combat:out_of_range` routes to `UPlayerInputSubsystem` (not `UCombatActionSubsystem`) because it triggers movement behavior (walk-to-target), which is the input subsystem's domain. `SkillTreeSubsystem` also listens to `combat:out_of_range` to dismiss ground targeting circles.

### Phase 4: Entity Events (to UOtherPlayerSubsystem + UEnemySubsystem)

These events manage the lifecycle and position updates of remote players and enemies. BP_SocketManager currently forwards them to `BP_OtherPlayerManager` and `BP_EnemyManager` via stored actor references.

| Event | BP Function | Node Count | C++ Handler | What It Does |
|-------|------------|------------|-------------|-------------|
| `player:moved` | OnPlayerMoved | ~38 | `UOtherPlayerSubsystem::HandlePlayerMoved` | Spawn or update remote player: create actor if new ID, set interpolation target position, update displayed info (name, class, level) |
| `player:left` | OnPlayerLeft | ~10 | `UOtherPlayerSubsystem::HandlePlayerLeft` | Remove remote player actor from world and registry |
| `enemy:spawn` | OnEnemySpawn | ~41 | `UEnemySubsystem::HandleEnemySpawn` | Spawn enemy actor at position with template data (name, level, HP, mesh), add to registry |
| `enemy:move` | OnEnemyMove | ~28 | `UEnemySubsystem::HandleEnemyMove` | Update enemy interpolation target position |
| `enemy:death` | OnEnemyDeath | ~45 | `UEnemySubsystem::HandleEnemyDeath` | Trigger death animation, disable collision, start corpse timer, remove from registry after delay |
| `enemy:health_update` | OnEnemyHealthUpdate | ~42 | `UEnemySubsystem::HandleEnemyHealthUpdate` | Update enemy HP/MaxHP for WorldHealthBarSubsystem display |

### Already in C++ (No Migration Needed)

These events are already handled by C++ subsystems that register directly with `USocketEventRouter`. The BP bridge may also forward some of these to BP_SocketManager, but the C++ handler is the primary/authoritative consumer. When the BP bridge entry is removed, these events continue working with zero changes.

| Event | C++ Subsystem(s) | What It Does |
|-------|-------------------|-------------|
| `enemy:attack` | `MultiplayerEventSubsystem::HandleEnemyAttack` (direct C++ -- no BP function) | Find enemy actor via BP_EnemyManager, call `PlayAttackAnimation()` |
| `combat:damage` | `DamageNumberSubsystem`, `BasicInfoSubsystem`, `WorldHealthBarSubsystem` | Floating damage numbers, local HP update, enemy HP bars |
| `combat:health_update` | `BasicInfoSubsystem`, `WorldHealthBarSubsystem`, `SkillVFXSubsystem` | HP/SP bar updates |
| `combat:death` | `BasicInfoSubsystem`, `WorldHealthBarSubsystem` | Death state tracking, HP bar hide |
| `combat:respawn` | `BasicInfoSubsystem`, `WorldHealthBarSubsystem` | Reset HP bars |
| `skill:effect_damage` | `DamageNumberSubsystem`, `BasicInfoSubsystem`, `WorldHealthBarSubsystem`, `SkillVFXSubsystem` | Skill damage numbers, HP updates, VFX triggers |
| `skill:cast_start` | `CastBarSubsystem`, `SkillVFXSubsystem` | Cast bar display, casting circle VFX |
| `skill:cast_complete` | `CastBarSubsystem`, `SkillVFXSubsystem` | Cast bar complete, VFX trigger |
| `skill:cast_interrupted` | `CastBarSubsystem`, `SkillTreeSubsystem`, `SkillVFXSubsystem` | Cast bar cancel, dismiss targeting circle |
| `skill:cast_interrupted_broadcast` | `CastBarSubsystem`, `SkillVFXSubsystem` | Remote player cast interrupted |
| `skill:cast_failed` | `CastBarSubsystem`, `SkillTreeSubsystem` | Cast bar error, dismiss targeting circle |
| `skill:buff_applied` | `BuffBarSubsystem`, `SkillTreeSubsystem`, `SkillVFXSubsystem` | Buff icon display, skill tree cooldown, buff VFX |
| `skill:buff_removed` | `BuffBarSubsystem`, `SkillTreeSubsystem`, `SkillVFXSubsystem` | Buff icon removal, VFX cleanup |
| `status:applied` | `BuffBarSubsystem` | Debuff icon display with countdown timer |
| `status:removed` | `BuffBarSubsystem` | Debuff icon removal |
| `buff:list` | `BuffBarSubsystem` | Full buff list sync (zone transition) |
| `skill:data` | `SkillTreeSubsystem` | Skill tree data sync |
| `skill:learned` | `SkillTreeSubsystem` | Skill point allocation result |
| `skill:refresh` | `SkillTreeSubsystem` | Skill data refresh |
| `skill:reset_complete` | `SkillTreeSubsystem` | Skill reset result |
| `skill:error` | `SkillTreeSubsystem` | Skill use error message |
| `skill:used` | `SkillTreeSubsystem` | Skill use confirmation, cooldown start |
| `skill:cooldown_started` | `SkillTreeSubsystem` | Cooldown timer start |
| `skill:ground_effect_created` | `SkillVFXSubsystem` | Ground AoE VFX spawn |
| `skill:ground_effect_removed` | `SkillVFXSubsystem` | Ground AoE VFX cleanup |
| `inventory:data` | `InventorySubsystem`, `EquipmentSubsystem`, `BasicInfoSubsystem` | Full inventory sync |
| `inventory:equipped` | `InventorySubsystem` | Equipment change confirmation |
| `inventory:dropped` | `InventorySubsystem` | Item drop confirmation |
| `inventory:error` | `InventorySubsystem` | Inventory operation error |
| `card:result` | `InventorySubsystem` | Card compound result |
| `shop:data` | `ShopSubsystem` | NPC shop item list |
| `shop:bought` | `ShopSubsystem`, `BasicInfoSubsystem` | Purchase confirmation, zeny update |
| `shop:sold` | `ShopSubsystem`, `BasicInfoSubsystem` | Sell confirmation, zeny update |
| `shop:error` | `ShopSubsystem` | Shop transaction error |
| `kafra:data` | `KafraSubsystem` | Kafra service data |
| `kafra:saved` | `KafraSubsystem` | Save point confirmation |
| `kafra:teleported` | `KafraSubsystem` | Teleport confirmation |
| `kafra:error` | `KafraSubsystem` | Kafra service error |
| `hotbar:alldata` | `HotbarSubsystem`, `SkillTreeSubsystem` | Full hotbar data sync |
| `player:stats` | `CombatStatsSubsystem`, `BasicInfoSubsystem`, `WorldHealthBarSubsystem` | Stat update (derived stats, HP, SP, etc.) |
| `exp:gain` | `BasicInfoSubsystem` | EXP bar update |
| `exp:level_up` | `BasicInfoSubsystem` | Level up notification |
| `player:joined` | `BasicInfoSubsystem` | Initial player data on zone entry |
| `zone:change` | `ZoneTransitionSubsystem` | Zone transition trigger |
| `zone:error` | `ZoneTransitionSubsystem` | Zone transition error |
| `player:teleport` | `ZoneTransitionSubsystem` | Kafra teleport / warp |

### Events That Stay in BP Bridge (Not Migrated)

| Event | BP Function | Reason |
|-------|------------|--------|
| `chat:receive` | OnChatReceived | Chat widget is still UMG (`WBP_ChatWidget`). Migrating chat to Slate is a separate task. |
| `loot:drop` | OnLootDrop | Loot popup is still UMG (`WBP_LootPopup`). Migrating loot UI to Slate is a separate task. |

### Events with Redundant BP Bridge Forwarding

These events are currently forwarded by the BP bridge to BP_SocketManager AND handled by C++ subsystems. The BP_SocketManager handler for these is either dead code or performs only minor secondary work (e.g., updating a legacy BP stat window). These bridge entries can be removed when the corresponding migration phase removes the primary BP handler.

| Event | BP Function | C++ Already Handles | BP Handler Does |
|-------|------------|--------------------|----|
| `player:stats` | OnPlayerStats (UpdateStats) | `CombatStatsSubsystem`, `BasicInfoSubsystem`, `WorldHealthBarSubsystem` | Legacy stat window update (can be removed) |
| `inventory:data` | OnInventoryData | `InventorySubsystem`, `EquipmentSubsystem`, `BasicInfoSubsystem` | BP equip visual update (already superseded by C++ EquipmentSubsystem) |
| `inventory:used` | OnItemUsed | `InventorySubsystem` (via `inventory:data` refresh) | Item use visual feedback (minimal, can be in C++) |
| `inventory:equipped` | OnItemEquipped | `InventorySubsystem` | Equip visual on character mesh (needs migration to C++ actor or stays until mesh management migrates) |
| `inventory:dropped` | OnItemDropped | `InventorySubsystem` | Drop visual feedback (minimal) |
| `inventory:error` | OnInventoryError | `InventorySubsystem` | Error message display (minimal) |
| `hotbar:data` | OnHotbarData | `HotbarSubsystem` | Legacy single-row hotbar update (superseded by `hotbar:alldata`) |
| `shop:data` | OnShopData | `ShopSubsystem` | Shop open in BP (already handled by ShopSubsystem Slate widget) |
| `shop:bought` | OnShopBought | `ShopSubsystem`, `BasicInfoSubsystem` | Purchase confirmation (already handled) |
| `shop:sold` | OnShopSold | `ShopSubsystem`, `BasicInfoSubsystem` | Sell confirmation (already handled) |
| `shop:error` | OnShopError | `ShopSubsystem` | Error display (already handled) |

---

## Migration Process for MultiplayerEventSubsystem

### Step 1: New C++ Subsystem Registers Handler via EventRouter

The new subsystem registers for the event in its `OnWorldBeginPlay`, following the exact same pattern as all 19 existing C++ subsystems.

```cpp
// Example: UEnemySubsystem::OnWorldBeginPlay
void UEnemySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UMMOGameInstance* GI = Cast<UMMOGameInstance>(InWorld.GetGameInstance());
    if (!GI) return;

    USocketEventRouter* Router = GI->GetEventRouter();
    if (!Router) return;

    Router->RegisterHandler(TEXT("enemy:spawn"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemySpawn(D); });
    Router->RegisterHandler(TEXT("enemy:move"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyMove(D); });
    Router->RegisterHandler(TEXT("enemy:death"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyDeath(D); });
    Router->RegisterHandler(TEXT("enemy:health_update"), this,
        [this](const TSharedPtr<FJsonValue>& D) { HandleEnemyHealthUpdate(D); });
}
```

Unregistration in `Deinitialize`:

```cpp
void UEnemySubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UMMOGameInstance* GI = Cast<UMMOGameInstance>(World->GetGameInstance()))
        {
            if (USocketEventRouter* Router = GI->GetEventRouter())
            {
                Router->UnregisterAllForOwner(this);
            }
        }
    }
    Super::Deinitialize();
}
```

### Step 2: Remove from MultiplayerEventSubsystem Bridge List

Once the C++ subsystem is verified to handle the event correctly, remove the bridge registration from `MultiplayerEventSubsystem::OnWorldBeginPlay`.

```cpp
// In MultiplayerEventSubsystem::OnWorldBeginPlay
// BEFORE (bridge active):
Router->RegisterHandler(TEXT("enemy:spawn"), this,
    [this](const TSharedPtr<FJsonValue>& D) { ForwardToBPHandler(TEXT("OnEnemySpawn"), D); });

// AFTER (bridge removed):
// (line deleted — enemy:spawn is now handled by UEnemySubsystem)
```

Update the event count log:
```cpp
UE_LOG(LogMultiplayerBridge, Log, TEXT("MultiplayerEventSubsystem — %d events registered (bridge to %s)."),
    EventCount, *SocketManagerActor->GetName());
```

### Step 3: BP_SocketManager Function Becomes Dead Code

The BP_SocketManager function (e.g., `OnEnemySpawn`) is never called because no handler forwards to it. The Blueprint function still exists in the `.uasset` but does nothing.

**Do not delete the Blueprint function during migration.** It stays as dormant code until Phase 6 cleanup, when BP_SocketManager's migrated handler functions are bulk-deleted.

### Step 4: Verify No Double-Handling

After removing the bridge entry, verify that the event is not double-handled:

1. Check that `USocketEventRouter` has exactly the expected number of handlers for the event
2. Test in-game that the behavior is identical to pre-migration
3. Check UE5 log output for any `LogMultiplayerBridge` warnings about missing BP functions

---

## Event Count Summary

| Category | Count | Details |
|----------|-------|---------|
| Total events bridged by MultiplayerEventSubsystem | 31 | All events currently forwarded to BP_SocketManager |
| Events migrating to C++ (Phase 3, combat) | 8 | `combat:damage`, `auto_attack_started/stopped`, `target_lost`, `out_of_range`, `death`, `respawn`, `error` |
| Events migrating to C++ (Phase 4, entity) | 6 | `player:moved/left`, `enemy:spawn/move/death/health_update` |
| Events already fully handled by C++ subsystems | ~20 | Inventory, shop, skills, buffs, cast bar, hotbar, kafra, zone, stats, exp, VFX |
| Events staying in BP bridge | 2 | `chat:receive`, `loot:drop` |
| Redundant bridge entries (removable alongside migration) | ~11 | inventory:*, shop:*, hotbar:data, player:stats — BP handler is dead/superseded |
| **After full migration: bridge handles only** | **2** | **Chat and loot** |

### MultiplayerEventSubsystem Progression

| Phase | Bridge Event Count | Events Removed |
|-------|-------------------|----------------|
| Current (pre-migration) | 31 | -- |
| After Phase 3 (combat) | 23 | 8 combat events |
| After Phase 4 (entity) | 17 | 6 entity events |
| After Phase 6 (cleanup) | 2 | 15 redundant entries (inventory, shop, hotbar, stats) |
| Future (chat + loot Slate) | 0 | BP_SocketManager fully retired |

---

## Emit Migration

### Current State

`MultiplayerEventSubsystem` exposes 4 `BlueprintCallable` emit functions (called by BP_MMOCharacter and other Blueprint actors):

| Function | Event Emitted | Current Caller |
|----------|--------------|----------------|
| `EmitCombatAttack(TargetId, bIsEnemy)` | `combat:attack` | BP_MMOCharacter (click-to-attack) |
| `EmitStopAttack()` | `combat:stop_attack` | BP_MMOCharacter (movement cancels attack) |
| `RequestRespawn()` | `combat:respawn` | Death overlay respawn button |
| `EmitChatMessage(Message, Channel)` | `chat:message` | WBP_ChatWidget |

### Migration Targets

| Function | Migration Target | Reason |
|----------|-----------------|--------|
| `EmitCombatAttack` | `UCombatActionSubsystem::StartAutoAttack(TargetId, bIsEnemy)` | Combat initiation is the combat subsystem's responsibility. Called by `UPlayerInputSubsystem` when player clicks an enemy. |
| `EmitStopAttack` | `UCombatActionSubsystem::StopAutoAttack()` | Combat termination belongs in the combat subsystem. Called when player moves, clicks elsewhere, or target dies. |
| `RequestRespawn` | `UCombatActionSubsystem::RequestRespawn()` | Respawn is part of the combat death/respawn flow. Called from the death overlay widget (which will be Slate). |
| `EmitChatMessage` | Stays on `MultiplayerEventSubsystem` | Chat is still UMG-based. The emit function stays until chat migrates to Slate. |

### enemy:attack Special Case

`enemy:attack` is already handled directly in C++ by `MultiplayerEventSubsystem::HandleEnemyAttack`. This function bypasses the BP bridge entirely -- it calls `BP_EnemyManager::GetEnemyActor()` and `PlayAttackAnimation()` via `ProcessEvent`.

After Phase 4, when `UEnemySubsystem` owns all enemy actors, `HandleEnemyAttack` migrates into `UEnemySubsystem`:
- `UEnemySubsystem` has direct access to its `TMap<int32, AMMOEnemyActor*>` registry
- No `ProcessEvent` call needed -- direct C++ method call: `EnemyActor->PlayAttackAnimation()`
- `MultiplayerEventSubsystem::HandleEnemyAttack` and `FindEnemyManagerActor()` become dead code

---

## Inventory Event Bridge -- Special Considerations

The BP bridge currently forwards 5 inventory events to BP_SocketManager:
- `inventory:data` -> `OnInventoryData`
- `inventory:used` -> `OnItemUsed`
- `inventory:equipped` -> `OnItemEquipped`
- `inventory:dropped` -> `OnItemDropped`
- `inventory:error` -> `OnInventoryError`

All 5 are already handled by `InventorySubsystem` and/or `EquipmentSubsystem` in C++. The BP handlers are redundant EXCEPT for one thing: `OnItemEquipped` in BP_SocketManager currently updates the character mesh's visual equipment (attaching weapon meshes, armor visibility, etc.).

This visual equipment update is NOT yet handled by any C++ subsystem. Options:

1. **Keep the bridge entry for `inventory:equipped` until visual equipment is migrated** -- safest, but delays full bridge cleanup
2. **Add visual equipment handling to `EquipmentSubsystem`** -- clean, but requires mesh attachment logic in C++
3. **Move visual equipment to `UCombatActionSubsystem` or `AMMORemotePlayer`** -- if equipment visuals are part of actor setup

Recommended: Option 2. `EquipmentSubsystem` already parses the equipped event and knows which items are equipped. Adding mesh attachment calls is a natural extension.

---

## Testing Checklist

### Phase 3 Migration Verification

- [ ] Click enemy -> auto-attack starts -> `combat:auto_attack_started` handled by C++
- [ ] Damage numbers appear (DamageNumberSubsystem -- already C++, no change)
- [ ] Attack animation plays on local player (UCombatActionSubsystem -- new)
- [ ] Local player faces target when attacking (UCombatActionSubsystem -- new)
- [ ] Hit reaction plays on enemy (UCombatActionSubsystem -- new)
- [ ] Dual wield: both damage numbers appear with Z offset
- [ ] Player moves: auto-attack stops -> `combat:auto_attack_stopped` handled by C++
- [ ] Target dies: `combat:target_lost` handled -> return to idle
- [ ] Target moves out of range: `combat:out_of_range` handled -> chase
- [ ] Local player dies: death overlay shown, input disabled
- [ ] Respawn button: `combat:respawn` handled -> restore position, HP, input
- [ ] Error messages display correctly
- [ ] Remove all 8 combat bridge entries from MultiplayerEventSubsystem
- [ ] Verify no `LogMultiplayerBridge` warnings about missing BP functions

### Phase 4 Migration Verification

- [ ] Enter zone: remote players spawn at correct positions
- [ ] Remote players move smoothly (interpolated)
- [ ] Remote player leaves: actor destroyed
- [ ] Enter zone: enemies spawn at correct positions with correct names/levels
- [ ] Enemies move smoothly (interpolated)
- [ ] Enemy dies: death animation, corpse timer, cleanup
- [ ] Enemy HP updates display correctly on WorldHealthBarSubsystem
- [ ] `enemy:attack` handled by UEnemySubsystem (not ProcessEvent to BP_EnemyManager)
- [ ] Zone transition: all entities cleaned up, new zone entities spawn
- [ ] Remove all 6 entity bridge entries from MultiplayerEventSubsystem
- [ ] Remove `HandleEnemyAttack` and `FindEnemyManagerActor` from MultiplayerEventSubsystem

### Phase 6 Cleanup Verification

- [ ] Remove all redundant bridge entries (inventory, shop, hotbar, stats)
- [ ] MultiplayerEventSubsystem bridge count drops to 2 (chat, loot)
- [ ] BP_SocketManager has only 2 active handler functions
- [ ] All 19 existing C++ subsystems still work (no regressions)
- [ ] Full gameplay regression test: login, zone transition, combat, inventory, skills, shops, buffs, equipment

---

## File Changes Summary

### Files Modified

| File | Changes |
|------|---------|
| `UI/MultiplayerEventSubsystem.cpp` | Remove bridge registrations for migrated events (Phase 3: 8 lines, Phase 4: 6 lines, Phase 6: ~15 lines). Remove `HandleEnemyAttack`, `FindEnemyManagerActor`. Update event count log. |
| `UI/MultiplayerEventSubsystem.h` | Remove `HandleEnemyAttack`, `FindEnemyManagerActor`, `EnemyManagerActor` member. Remove emit functions migrated to `UCombatActionSubsystem`. |

### Files Created (by other migration phases)

| File | Phase | Registers for Events |
|------|-------|---------------------|
| `UI/CombatActionSubsystem.h/.cpp` | Phase 3 | `combat:damage`, `combat:auto_attack_started/stopped`, `combat:target_lost`, `combat:death`, `combat:respawn`, `combat:error` |
| `UI/PlayerInputSubsystem.h/.cpp` | Phase 1 | `combat:out_of_range` (chase trigger) |
| `UI/OtherPlayerSubsystem.h/.cpp` | Phase 4 | `player:moved`, `player:left` |
| `UI/EnemySubsystem.h/.cpp` | Phase 4 | `enemy:spawn`, `enemy:move`, `enemy:death`, `enemy:health_update`, `enemy:attack` |

### Files Unchanged

BP_SocketManager Blueprint asset (`.uasset`) is not modified during migration. Its handler functions become dead code as bridge entries are removed, then are deleted in Phase 6 cleanup.
