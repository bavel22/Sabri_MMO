# Troubleshooting Guide

## Server Issues

### Server won't start — Port in use
```bash
# Check what's using port 3001
netstat -ano | findstr :3001

# Kill all Node processes
taskkill /F /IM node.exe
```

### Server won't start — Database connection failed
1. Verify PostgreSQL service is running: `net start postgresql-x64-15`
2. Check `server/.env` credentials match your PostgreSQL setup
3. Test connection: `psql -U postgres -d sabri_mmo -c "SELECT 1;"`
4. Ensure `sabri_mmo` database exists: `psql -U postgres -c "\l"`

### Server won't start — Redis connection failed
1. Start Redis: `redis-server`
2. Verify: `redis-cli ping` (should return `PONG`)
3. Check error in server log: `server/logs/server.log`

### Server starts but UE5 can't connect
1. Verify server health: `curl http://localhost:3001/health`
2. Check Windows Firewall allows port 3001
3. Ensure SocketIOClient plugin is installed in UE5 project
4. Check UE5 Output Log for HTTP/Socket errors
5. Verify server URL in Blueprint is `http://localhost:3001`

---

## UE5 Client Issues

### C++ compilation error — FormatStringSan.h
**Symptom**: C2971 errors with MSVC 14.38  
**Fix**: Patch `C:\UE_5.7\Engine\Source\Runtime\Core\Public\String\FormatStringSan.h`:
```cpp
// Change UE_CHECK_FORMAT_STRING macro to:
#define UE_CHECK_FORMAT_STRING(...) do {} while(false);
```
**Note**: Revert when updating to MSVC 14.44.35214+

### C++ compilation error — HttpManager.h shadowing
**Symptom**: Ambiguous include, engine's `HttpManager.h` shadows project's  
**Fix**: Project file already renamed to `MMOHttpManager.h` / `MMOHttpManager.cpp`. Class name `UHttpManager` unchanged.

### Blueprint errors after C++ changes
1. Close UE5 Editor
2. Delete `Intermediate/` and `Binaries/` folders in `client/SabriMMO/`
3. Rebuild from Visual Studio or re-open UE5 (triggers rebuild)

### Socket.io events not firing
1. Verify `GI->IsSocketConnected()` returns true (socket lives on GameInstance, not in level actors)
2. Check `USocketEventRouter` handler registration in the relevant subsystem's `OnWorldBeginPlay`
3. Check server logs for `[RECV]` / `[SEND]` messages
4. Add `UE_LOG` in the subsystem's event handler to confirm it fires
5. Verify the subsystem's `ShouldCreateSubsystem()` returns true for the current world

### Duplicate character spawning (one at origin, one controlled)
**Symptom**: Two BP_MMOCharacter instances - one stuck at (0,0,0) and one you can control  
**Root Cause**: Game Mode auto-spawns a Default Pawn at origin AND custom spawn logic creates another character  
**Fix**: Set `Default Pawn Class` to `None` in your Game Mode settings. This prevents the automatic spawn at origin, allowing only your custom character spawning logic to create the playable character.

### Remote players not appearing
1. Verify `UOtherPlayerSubsystem` is active (check Output Log for registration messages)
2. Verify `player:moved` event is being received by the EventRouter
3. Check that `BP_OtherPlayerCharacter` spawn class exists in Content Browser
4. Verify the player is in the same zone on the server

### Click-to-move not working
1. Check NavMesh is built (press P in editor to visualize)
2. Verify `CharacterMovement` > `Use Acceleration for Paths` is checked
3. Check Input Mode is set to "Game and UI" (not "UI Only")
4. Verify `BP_MMOCharacter` has NavMesh-compatible collision

---

## Database Issues

### Tables missing
The server auto-creates tables on startup. If tables are still missing:
```bash
psql -U postgres -d sabri_mmo -f database/init.sql
```

### Items table empty
The server auto-seeds items if the table is empty. Restart the server, or manually:
```bash
psql -U postgres -d sabri_mmo -f database/init.sql
```

### Stats not saving
1. Stats are saved on disconnect (`socket.on('disconnect')`)
2. Check server logs for `[DB] Saved stats for character X on disconnect`
3. Verify stat columns exist: `\d characters` in psql
4. If columns missing, restart server (auto-adds via `ALTER TABLE IF NOT EXISTS`)

### Character position resets to (0,0,0)
1. Position is saved via `PUT /api/characters/:id/position` (auto-save every 5s in Blueprint)
2. Also saved via Redis cache on `player:position` events
3. Check server logs for position save confirmations
4. On join, position is loaded from DB: `SELECT x, y, z FROM characters WHERE character_id = $1`

---

## Combat Issues

### Attacks not registering
1. Check auto-attack state in server logs: `[COMBAT] X started auto-attacking Y`
2. Verify range: `[COMBAT] combat:out_of_range` in logs means too far
3. Check ASPD timing: attack interval = `(200 - ASPD) * 50` ms
4. Ensure target exists in `connectedPlayers` (for players) or `enemies` (for NPCs)
5. `RANGE_TOLERANCE: 50` was added to prevent attacks failing at exact range boundary

### Target frame not hiding after kill
Client needs to bind BOTH `combat:auto_attack_stopped` AND `combat:target_lost` events. Both must set `IsAutoAttacking = false` and call `HideTargetFrame`.

### Enemy walking to world origin on spawn
`enemy:spawn` handler must use `x/y/z` from the event payload as the Spawn Transform location. Do NOT spawn at (0,0,0) then move.

### Crash on Butterfly Wing / zone transition (EXCEPTION_ACCESS_VIOLATION in TSparseArray::DestroyElements)
**Symptom**: `EXCEPTION_ACCESS_VIOLATION reading address 0x0000001900000018` in `TSparseArray::DestroyElements()` called from a subsystem's `TMap::Empty()` during zone transition (Butterfly Wing, warp portal, etc.)
**Root Cause**: `SetTimerForNextTick([this]() { ... })` captures `this` as a raw pointer. During zone transition, the subsystem is destroyed (`Deinitialize` + destructor free TMap memory), but the next-tick timer still fires on the dangling pointer.
**Fix**: Use `TWeakObjectPtr<UMySubsystem>` in the lambda capture, store the `FTimerHandle`, and clear it in `Deinitialize()`:
```cpp
// In event handler:
TWeakObjectPtr<UMySubsystem> WeakThis(this);
RefreshTimerHandle = World->GetTimerManager().SetTimerForNextTick([WeakThis]()
{
    if (UMySubsystem* Self = WeakThis.Get())
        Self->DoWork();
});

// In Deinitialize():
World->GetTimerManager().ClearTimer(RefreshTimerHandle);
```
**Fixed in**: EquipmentSubsystem (2026-03-15). All subsystems using `SetTimerForNextTick` should follow this pattern.

### Player crash on enemy kill (EXCEPTION_ACCESS_VIOLATION)
Server sends `enemy:death` broadcast only — does NOT also send `combat:target_lost`. Sending both events causes Blueprint to null the enemy reference (via target_lost) before enemy:death tries to access it.

### Remote player rotation during combat
Use `attackerX/Y/Z` and `targetX/Y/Z` from `combat:damage` payload. Call `FindLookAtRotation` then `SetActorRotation` on both `BP_OtherPlayerCharacter` and `BP_EnemyCharacter`.

---

## Performance Issues

### Server tick loop high CPU
- Combat tick runs every 50ms — OK for small player counts
- Enemy AI tick runs every 500ms — minimal CPU impact
- If CPU is high, check `connectedPlayers` size and `autoAttackState` size
- Reduce logging by setting `LOG_LEVEL=WARN` in `.env`

### Redis memory growing
- Position keys have 300s TTL — should auto-expire
- Check: `redis-cli KEYS 'player:*'` to see active keys
- If stale keys accumulate: `redis-cli FLUSHALL` (development only)

### Client FPS drops
- Check UE5 profiling tools (Stat FPS, Stat Unit)
- Remote players: ensure interpolation is smooth (not snapping)
- Widget overhead: hide name tags for distant players
- NavMesh: ensure it's properly built for the level

---

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| Socket.io function param not named `Data` | Rename to exactly `Data` (String type) |
| Using `targetCharacterId` for enemies | Use `targetEnemyId` field for enemy targets |
| Spawning Blueprint at (0,0,0) then teleporting | Set spawn transform from event data directly |
| Reading `int` stat from JSON as `int` | Server uses `int_stat` column (SQL keyword avoidance) |
| Not parsing IDs with `parseInt()` | Always parse client-supplied IDs |
| Hardcoded server URL | Use `http://localhost:3001` consistently |
| Forgetting to Compile & Save Blueprint | Always Compile + Save after changes |
| Game Mode Default Pawn Class causing duplicate spawns | Set `Default Pawn Class` to `None` in Game Mode |

---

## Recent Bug Fixes

### Back Slide doesn't move character
**Symptom**: Casting Back Slide does nothing — character stays in place.
**Root Cause**: Two bugs:
1. **Server**: `getPlayerPosition()` read from Redis which could return null, silently skipping the teleport.
2. **Client**: `HandlePlayerTeleport` in `ZoneTransitionSubsystem.cpp` didn't cancel active pathfinding before `SetActorLocation`, so `SimpleMoveToLocation` pathfollowing dragged the pawn back to its previous destination.

**Fix**:
1. Server: Use in-memory `player.lastX/lastY/lastZ` instead of Redis lookup.
2. Client: Added `ForceStopAllMovement()` to `PlayerInputSubsystem`, called before teleport.

### Server crash on login (charRow not defined)
**Symptom**: Server crashes with `ReferenceError: charRow is not defined` during `player:join`.
**Root Cause**: Cart state initialization referenced `charRow.has_cart` but the variable was `row`, scoped inside an if block and not visible at the cart init code.
**Fix**: Added `has_cart`/`cart_type` to the SELECT query and extracted values to outer-scope variables `dbHasCart`/`dbCartType`.

### Server crash on death (calculateASPD not defined)
**Symptom**: Server crashes with `ReferenceError: calculateASPD is not defined` when a player dies.
**Root Cause**: `applyDeathPenalty` called `calculateASPD(player)` and `calculateDerivedStats()` which don't exist as standalone functions.
**Fix**: Use `roDerivedStats(effStats)` + `Math.min(COMBAT.ASPD_CAP, derived.aspd + weaponAspdMod)`.

### Death penalty EXP not persisting
**Symptom**: Player dies and loses EXP, but after relog the EXP is restored to the pre-death value.
**Root Cause**: DB query used `WHERE id = $3` but the column is `character_id`.
**Fix**: Changed to `WHERE character_id = $3`.

### Poison DoT stops at 25% HP
**Symptom**: Poison damage-over-time stops draining HP once the player reaches 25% HP.
**Root Cause**: `ro_status_effects.js` poison config had `minHpPercent: 0.25`. In RO Classic, poison drains HP all the way down to 1 HP.
**Fix**: Removed `minHpPercent` from poison config. The DoT now falls through to the default `minHp = 1` floor via `canKill: false`.

### Warp Portal Lv1 shows all memo destinations
**Symptom**: Casting Warp Portal at Lv1 (should only offer "Random" destination) shows all memorized destinations as if cast at max level.
**Root Cause**: Cast completion callback didn't pass `skillLevel`, so after cast time the skill handler used the max learned level instead of the selected level.
**Fix**: Cast completion now includes `skillLevel: cast.learnedLevel` in callback data.

### Teleport Lv2 doesn't teleport to save point
**Symptom**: Teleport Lv2 (should warp to save point) does nothing or behaves like Lv1.
**Root Cause**: Two bugs:
1. Column name `save_zone` doesn't exist — correct column is `save_map`.
2. Cross-zone teleport only emitted `zone:change` without updating server state (zone rooms, player.zone, enemy aggro).

**Fix**: Full zone transition matching Kafra/Warp Portal pattern (leave old zone room, join new, update `player.zone`, deaggro enemies, emit `zone:change`).

### UseSkillOnGround doesn't send skill level
**Symptom**: Ground-targeted skills (Warp Portal, Pneuma) always cast at max learned level regardless of which hotbar level was selected.
**Root Cause**: `UseSkillOnGround` didn't accept or send `skillLevel` — the parameter was missing from the function signature and all call sites.
**Fix**: Added `skillLevel` parameter to `UseSkillOnGround` and all call sites (hotbar, PlayerInputSubsystem).

### Enemy debuffs not shown in combat log
**Symptom**: Casting a debuff on an enemy (e.g., Quagmire, Decrease AGI) produces no combat log message.
**Root Cause**: `HandleBuffApplied` in `ChatSubsystem` returned early for all enemy targets (`if (bIsEnemy) return`), suppressing all enemy debuff messages.
**Fix**: Now shows "X is affected by Y (Zs)" when the local player casts a debuff on an enemy.

### Increase AGI doesn't increase move speed
**Symptom**: Casting Increase AGI on a player doesn't visibly increase their movement speed, even though AGI goes up.
**Root Cause**: `moveSpeed` calculation in `buildFullStatsPayload` used `effectiveStats.moveSpeedBonus` which was always 0 (not populated by `getEffectiveStats`).
**Fix**: Reads `moveSpeedBonus` from `getCombinedModifiers(player)` directly.

### Cannot cast buffs on other players (PvP disabled)
**Symptom**: Casting Heal, Blessing, Increase AGI, or other supportive skills on another player does nothing when `PVP_ENABLED=false`.
**Root Cause**: The `skill:use` handler blocked ALL skills targeting other players when PvP was disabled, with no exception for supportive skills.
**Fix**: Added a `SUPPORTIVE_SKILLS` whitelist set containing heal, blessing, increase_agi, cure, resurrection, and 14 other supportive skills that bypass the PvP check. Offensive skills remain blocked when PvP is disabled.

### Angelus DEF not reflected in stat window
**Symptom**: Casting Angelus shows the buff icon but the stat window DEF value doesn't change.
**Root Cause**: `softDEF` in `buildFullStatsPayload` only applied `buffDefMultiplier`, not the Angelus `defPercent` modifier.
**Fix**: Now multiplies by `(1 + defPercent/100)`.

---

**Last Updated**: 2026-03-19
