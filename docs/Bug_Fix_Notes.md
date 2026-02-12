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

## Known Issues & Workarounds

### Issue: None currently
All reported bugs have been resolved. Check server console for `[COMBAT]` and `[ENEMY AI]` prefixed messages for debugging.

---

**Last Updated**: 2026-02-11  
**Version**: 1.0.6
