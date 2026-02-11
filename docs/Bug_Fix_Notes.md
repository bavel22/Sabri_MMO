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

## Known Issues & Workarounds

### Issue: combat:target_lost / combat:auto_attack_stopped may not trigger in logs
Server logging has been enhanced (2026-02-11) to show detailed state when these events fire or don't fire. Check server console for `[COMBAT]` prefixed messages.

---

**Last Updated**: 2026-02-11  
**Version**: 1.0.5
