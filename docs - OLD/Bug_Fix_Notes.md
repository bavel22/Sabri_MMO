# Bug Fix Notes

## Bug: Camera Uses Wrong Pawn on First Character Creation

**Date Fixed**: 2026-02-03  
**Severity**: Medium  
**Status**: ✅ Fixed

### Problem
When creating a new character and entering the world for the first time, the game used the incorrect camera (default ThirdPersonCharacter camera instead of the custom SpringArm camera). However, after closing and relaunching the game, logging back in used the correct camera.

### Root Cause
The Level Blueprint had **two different Spawn Actor nodes** for different scenarios:
1. **First spawn** (new character creation) → Spawned `BP_ThirdPersonCharacter`
2. **Subsequent spawns** (existing character login) → Spawned `BP_MMOCharacter`

The first spawn node was never updated when the custom character blueprint was created.

### Solution
Changed both Spawn Actor nodes in the Level Blueprint to use `BP_MMOCharacter`:

1. Open **lvl_ThirdPerson** → **Level Blueprint**
2. Find all **Spawn Actor** nodes
3. Change **Class** pin on each node to: `BP_MMOCharacter`
4. **Compile** and **Save**

### Prevention
When creating a custom character blueprint:
1. Search for ALL Spawn Actor nodes in Level Blueprint
2. Update the Class on every spawn node
3. Test both paths: new character creation AND existing character login

### Files Modified
- `lvl_ThirdPerson` (Level Blueprint)

### Related Documentation
- [Camera System](Camera_System.md)
- [Top-Down Movement System](Top_Down_Movement_System.md)

---

---

## Bug: UE5 Crash When Single Player Kills Enemy

**Date Fixed**: 2026-02-10  
**Severity**: Critical  
**Status**: Fixed

### Problem
UE5 crashed with `EXCEPTION_ACCESS_VIOLATION` in `SocketIOClientComponent::CallBPFunctionWithResponse` when a single player killed an enemy.

### Root Cause
When an enemy died, the server sent `combat:target_lost` to the killer AND `enemy:death` nearly simultaneously. Blueprint processed `target_lost` first (cleaned up enemy reference), then `enemy:death` tried to access the destroyed reference.

### Solution
Excluded the killer from receiving `combat:target_lost` during enemy death. Killer only receives `enemy:death`.

### Files Modified
- `server/src/index.js` (enemy death handler in combat tick loop)

---

## Bug: UE5 Crash When 2 Players Kill Same Enemy

**Date Fixed**: 2026-02-11  
**Severity**: Critical  
**Status**: Fixed

### Problem
Game didn't crash with a single player attacking an enemy, but when 2 different players attacked the same enemy and it died, UE5 crashed with `EXCEPTION_ACCESS_VIOLATION` in `SocketIOClientComponent::CallBPFunctionWithResponse`.

### Root Cause
Same pattern as single-player crash but for the second attacker. On enemy death:
1. Killer's autoAttackState was deleted (no `target_lost` sent to killer - previous fix)
2. Other attackers received BOTH `combat:target_lost` AND `enemy:death` (via `io.emit`)
3. Blueprint processed `target_lost` first (nullified enemy reference), then `enemy:death` tried to access destroyed reference

### Solution
Removed ALL `combat:target_lost` emissions during enemy death. All attackers now only receive `enemy:death` broadcast, which is the single authoritative "enemy is dead" signal. AutoAttackState entries are silently deleted.

### Prevention
Pattern: When an entity dies, NEVER send `target_lost` + `death` to the same client. Only send the death event.

### Files Modified
- `server/src/index.js` (enemy death handler — removed `combat:target_lost` loop, kept only `enemy:death` broadcast)

---

## Bug: "Target not found" When Attacking Enemies/Players

**Date Fixed**: 2026-02-10  
**Severity**: High  
**Status**: Fixed

### Problem
Server logged "Target not found" when attacking enemies, even though enemies existed in the `enemies` Map.

### Root Cause
Type mismatch: `enemies` Map uses integer keys (from `nextEnemyId++`), but UE5 SocketIOClient sends `targetEnemyId` as a string. `enemies.get("2000001")` returns `undefined` because `"2000001" !== 2000001`. Same issue with `player:join` storing `characterId` as string.

### Solution
Added `parseInt()` to all client-supplied IDs at entry points:
- `player:join`: `parseInt(data.characterId)`
- `player:position`: `parseInt(data.characterId)`
- `combat:attack`: `parseInt(data.targetCharacterId)` and `parseInt(data.targetEnemyId)`

### Files Modified
- `server/src/index.js` (player:join, player:position, combat:attack handlers)

---

## Bug: Player Kill Crash (target_lost + death race condition)

**Date Fixed**: 2026-02-10  
**Severity**: Critical  
**Status**: Fixed

### Problem
UE5 crashed when killing another player. Same pattern as enemy kill crash.

### Root Cause
Killer received `combat:target_lost` AND `combat:death` nearly simultaneously. Blueprint processed `target_lost` first (cleaned up player reference), then `combat:death` tried to access it.

### Solution
Excluded the killer from receiving `combat:target_lost` during player death sequence. Killer only receives `combat:death`.

### Files Modified
- `server/src/index.js` (player death handler in combat tick loop)

---

## Bug: Hover Indicator Not Hiding When Switching Targets

**Date Fixed**: 2026-02-11  
**Severity**: Medium  
**Status**: Fixed

### Problem
When a player switched from attacking one enemy to attacking a different enemy (or clicked the ground), the hover indicator on the old target would not hide. The `combat:auto_attack_stopped` event was never sent for the old target.

### Root Cause
The `combat:attack` handler overwrote the old `autoAttackState` entry when switching targets without cleaning up:
1. Old enemy still had the player in its `inCombatWith` set (preventing wandering)
2. No `combat:auto_attack_stopped` was emitted for the old target
3. Client Blueprint never received the signal to hide the old hover indicator

### Solution
Added target-switch cleanup in `combat:attack` handler (before setting new auto-attack state):
- Checks if player already has an active attack on a different target
- Removes player from old enemy's `inCombatWith` set
- Emits `combat:auto_attack_stopped` with `reason: 'Switched target'`, `oldTargetId`, and `oldIsEnemy`
- Blueprint `OnAutoAttackStopped` handler receives this and hides the old hover indicator

### Files Modified
- `server/src/index.js` (combat:attack handler — added target-switch cleanup block)

---

## Bug: Enemy Wandering Not Visible / No enemy:move Logs

**Date Fixed**: 2026-02-11  
**Severity**: Medium  
**Status**: Fixed

### Problem
Enemies appeared stationary. No `enemy:move` events visible in server logs. User could not confirm wandering was working.

### Root Cause
Two issues:
1. Wandering debug messages used `logger.debug()` — invisible at default INFO log level
2. `enemy:move` broadcast emissions had no logging at all
3. `pickRandomWanderPoint()` used polar coordinates from spawn point — could generate very small movements

### Solution
1. Changed all wandering log calls from `logger.debug()` to `logger.info()`
2. Added `logger.info()` after every `io.emit('enemy:move', ...)` call
3. Rewrote `pickRandomWanderPoint()` to use current position + random 100-300 offset per axis (randomly +/-)
4. Added `WANDER_DIST_MIN: 100` and `WANDER_DIST_MAX: 300` to `ENEMY_AI` constants
5. Clamped wander destination to `wanderRadius` from spawn to prevent infinite drift

### Files Modified
- `server/src/index.js` (ENEMY_AI constants, pickRandomWanderPoint, AI tick loop logging)

---

## Bug: Duplicate Socket Event Binding for Inventory Events

**Date Fixed**: 2026-02-13  
**Severity**: High  
**Status**: ✅ Fixed (Phase 1.1)

### Problem
In BP_SocketManager → BeginPlay, three different handler functions were all bound to the same socket event name `inventory:used`:
- `OnItemUsed` → bound to `inventory:used` ✅ (correct)
- `OnItemEquipped` → bound to `inventory:used` ❌ (should be `inventory:equipped`)
- `OnInventoryError` → bound to `inventory:used` ❌ (should be `inventory:error`)

This caused only the last binding to fire, or all three handlers to execute on item use events.

### Root Cause
Copy-paste error during inventory system Blueprint setup. The event name strings were not updated when creating new bindings.

### Solution
Corrected the `Bind Event to Function` event name strings:
- `OnItemEquipped` → now bound to `inventory:equipped`
- `OnInventoryError` → now bound to `inventory:error`

Verified server emits these as distinct events:
- `server/src/index.js` line 1073: `socket.emit('inventory:used', ...)`
- `server/src/index.js` line 1180: `socket.emit('inventory:equipped', ...)`
- `server/src/index.js` line 1020/1035/1042/1087: `socket.emit('inventory:error', ...)`

### Files Modified
- `Content/Blueprints/BP_SocketManager.uasset` (BeginPlay event bindings)

### Related Documentation
- [SocketIO_RealTime_Multiplayer.md](SocketIO_RealTime_Multiplayer.md) — Event binding section updated

---

## Bug: UE5 Crash When 2 Players Attack Same Player and Target Dies

**Date Fixed**: 2026-02-13  
**Severity**: Critical  
**Status**: ✅ Fixed

### Problem
UE5 crashed with `EXCEPTION_ACCESS_VIOLATION reading address 0x0000000000000010` when two players were auto-attacking the same player target and that target's health reached 0. Stack trace went through `UnrealEditor_SocketIOClient` → `TGraphTask::ExecuteTask`.

### Root Cause
Identical pattern to the enemy death crashes (fixed 2026-02-10 and 2026-02-11). When a player target died:
1. Server sent `combat:target_lost` to the **other attackers** (not the killer)
2. Server broadcast `combat:death` to **all clients** via `io.emit`
3. Other attackers' Blueprints processed `target_lost` first → nulled BP_OtherPlayerCharacter reference
4. Then `combat:death` arrived and tried to access the destroyed/null reference → crash at offset 0x10

The enemy death handler already had this fix (with an explanatory comment), but the player death handler still sent both events.

### Solution
Removed `combat:target_lost` emission from the player death cleanup loop. All attackers now only receive the `combat:death` broadcast, which is the single authoritative "player died" signal. `autoAttackState` entries are silently deleted server-side.

### Prevention
**Pattern (now applied to ALL death handlers):** When any entity dies, NEVER send `target_lost` + `death` to the same client. Only send the death event. The Blueprint `combat:death` / `enemy:death` handler must handle all cleanup (stop auto-attack, hide target frame, null references).

### Files Modified
- `server/src/index.js` (player death handler in combat tick loop — removed `combat:target_lost` loop, kept only `combat:death` broadcast)

### Related Documentation
- [SocketIO_RealTime_Multiplayer.md](SocketIO_RealTime_Multiplayer.md)
- [Multiplayer_Architecture.md](Multiplayer_Architecture.md)

---

## Known Issues & Workarounds

### Issue: None currently
All reported bugs have been resolved. Check server console for `[COMBAT]` and `[ENEMY AI]` prefixed messages for debugging.

---

**Last Updated**: 2026-02-13  
**Version**: 1.0.8
