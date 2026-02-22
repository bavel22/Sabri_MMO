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
1. Check `BP_SocketManager` is spawned in the level
2. Verify event binding uses `Bind Event to Function` node
3. Verify function parameter is named exactly `Data` (String type)
4. Check server logs for `[RECV]` / `[SEND]` messages
5. Add Print String nodes before and after each binding for debugging

### Duplicate character spawning (one at origin, one controlled)
**Symptom**: Two BP_MMOCharacter instances - one stuck at (0,0,0) and one you can control  
**Root Cause**: Game Mode auto-spawns a Default Pawn at origin AND custom spawn logic creates another character  
**Fix**: Set `Default Pawn Class` to `None` in your Game Mode settings. This prevents the automatic spawn at origin, allowing only your custom character spawning logic to create the playable character.

### Remote players not appearing
1. Check `BP_OtherPlayerManager` is spawned in the level
2. Verify `player:moved` event is being received (print in handler)
3. Check that `SpawnOrUpdatePlayer` function is being called
4. Verify spawn class reference is set in BP_OtherPlayerManager Details panel

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

**Last Updated**: 2026-02-17
