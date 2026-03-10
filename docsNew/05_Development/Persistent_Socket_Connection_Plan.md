# Persistent Socket Connection Across Zone Changes

**Status**: MERGED into `Strategic_Implementation_Plan_v3.md` as Phase 4. This file is kept for reference only.
**Created**: 2026-03-10

---

## Problem

Currently, every zone change causes a full **socket disconnect → reconnect → player:join** cycle:

1. Player enters warp portal → server sends `zone:change`
2. Client calls `UGameplayStatics::OpenLevel()` → destroys current world
3. `BP_SocketManager` (a per-level actor) is destroyed → socket disconnects
4. Server disconnect handler runs: saves to DB, removes from `connectedPlayers`, broadcasts `player:left`
5. New level loads → new `BP_SocketManager` creates a fresh socket connection
6. Client sends `player:join` → server creates brand new player object from DB
7. All in-memory state is lost: buffs, status effects, auto-attack state, active casts, enemy aggro

### Consequences

| Issue | Impact |
|-------|--------|
| **Buffs/statuses lost** | Endure, Provoke, Freeze — all wiped (mitigated by reconnect cache, but it's a band-aid) |
| **`player:left` + `player:joined` spam** | Other players in the zone see the player "leave and rejoin" on every zone change |
| **Auto-attack state lost** | Player was fighting → zone change → combat state gone |
| **Active casts interrupted** | Mid-cast when warping → cast data lost (not just interrupted, deleted) |
| **Enemy aggro disrupted** | Enemies targeting this player lose their target on disconnect |
| **DB writes on every zone change** | Disconnect handler saves stats/position to DB — unnecessary I/O |
| **Re-authentication overhead** | `player:join` re-validates JWT, re-loads equipment, skills, inventory from DB |
| **Race conditions** | Brief window where player exists in neither zone (between disconnect and reconnect) |

---

## Industry Standard: Persistent Connection

MMOs like WoW, FFXIV, Guild Wars 2, and Ragnarok Online servers (rAthena/Hercules) all use persistent connections:

- The **network connection lives at the session/account level**, not the map level
- Zone changes are a **server-side operation**: move player from zone A's bucket to zone B's bucket
- The client loads the new map assets while the connection stays alive
- No disconnect, no re-authentication, no DB reload
- Buffs, party state, guild state, chat history, combat state all persist naturally

---

## Current Architecture

```
UE5 Level (L_Prontera)
  └── BP_SocketManager (Actor)           ← DESTROYED on OpenLevel
       └── USocketIOClientComponent      ← Socket connection dies here
            └── Connected to server

UMMOGameInstance                          ← SURVIVES OpenLevel
  └── Auth token, character data, zone state
```

All 15 C++ subsystems find the socket via `FindSocketIOComponent()` which iterates world actors looking for a `USocketIOClientComponent`. When the level changes, the actor is gone.

## Target Architecture

```
UMMOGameInstance                          ← SURVIVES OpenLevel
  └── USocketIOClientComponent (UPROPERTY) ← Connection persists here
  └── Auth token, character data, zone state

UE5 Level (any)
  └── Subsystems find socket via GameInstance, not actor search
```

---

## Implementation Plan

### Phase A: Move SocketIO to GameInstance

1. **Add `USocketIOClientComponent` to `UMMOGameInstance`**
   - Create as a `UPROPERTY()` default subobject in constructor
   - Connect on login success (after JWT obtained)
   - Disconnect on logout / return to login screen
   - Never disconnect on zone change

2. **Update `FindSocketIOComponent()` in all 15 subsystems**
   - Change from actor iteration to: `Cast<UMMOGameInstance>(GetGameInstance())->GetSocketIOComponent()`
   - Single function in a shared header or on GameInstance itself

3. **Remove `BP_SocketManager` actor from all levels**
   - Currently in Level Blueprints — remove the actor spawn
   - Move any Blueprint event bindings to GameInstance or a new C++ subsystem

### Phase B: Refactor Zone Change Flow

**Current flow:**
```
Client: warp portal → emit zone:warp
Server: validate → emit zone:change { zone, level, x, y, z }
Client: OpenLevel → socket dies → reconnect → player:join
Server: full DB reload, new player object
```

**Target flow:**
```
Client: warp portal → emit zone:warp
Server: validate → move player to new zone bucket → emit zone:change { zone, level, x, y, z }
Client: OpenLevel → socket stays alive
Client: new level loads → emit zone:ready
Server: send zone enemies, other players, zone metadata (same as now, but no player:join needed)
```

4. **Server: Add `zone:transition` event** (replaces disconnect/reconnect)
   - Server moves player between zone rooms/buckets without disconnect
   - `broadcastToZone(oldZone, 'player:left', ...)` — notify old zone
   - Update `player.zone` in memory
   - `broadcastToZone(newZone, 'player:moved', ...)` — notify new zone
   - No `connectedPlayers.delete()`, no DB save, no re-auth

5. **Server: Remove re-init from zone change path**
   - No re-loading equipment/skills/inventory from DB (already in memory)
   - No re-sending `player:stats` (already sent)
   - Only send: zone enemies, other players, zone metadata, `buff:list`

6. **Client: `ZoneTransitionSubsystem` update**
   - On `zone:change`, call `OpenLevel` but do NOT expect socket to die
   - After new level loads, subsystems re-wrap events on the same (still-alive) component
   - Emit `zone:ready` as before

### Phase C: Blueprint Event Migration

7. **Move BP_SocketManager event bindings to C++**
   - The Blueprint SocketManager currently binds ~20 socket events (enemy:spawn, player:moved, etc.)
   - These need to move to C++ subsystems or a new `USocketEventRouter` subsystem
   - This is the largest single task — requires auditing all Blueprint event bindings

### Phase D: Cleanup

8. **Remove `reconnectBuffCache`** — no longer needed (buffs persist naturally)
9. **Remove `player:join` from zone change path** — only used on initial login
10. **Remove per-level `BP_SocketManager`** actor from all Level Blueprints
11. **Update all documentation** — CLAUDE.md, docsNew, memory files

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Blueprint event bindings break | HIGH | Audit all BP_SocketManager bindings before removing. Migrate one at a time. |
| Subsystems can't find socket during level transition | MEDIUM | GameInstance component is always available. Add null checks during transition. |
| Socket events arrive during level load (no world yet) | MEDIUM | Queue events in GameInstance, replay when new world's subsystems are ready |
| Multiple `OpenLevel` calls while socket is alive | LOW | Already handled by `bIsZoneTransitioning` flag |

---

## Dependency Graph

```
Phase A (Move SocketIO to GameInstance)
  │
  ├── Phase B (Refactor zone change flow)
  │     └── Phase D (Cleanup)
  │
  └── Phase C (Blueprint event migration)
        └── Phase D (Cleanup)
```

Phase A must come first. Phases B and C can run in parallel. Phase D comes last.

**Estimated effort**: 3-5 days for Phases A+B, 2-3 days for Phase C, 1 day for Phase D. Total: ~1-1.5 weeks.

---

## Files Affected

### Must Change
| File | Change |
|------|--------|
| `MMOGameInstance.h/.cpp` | Add `USocketIOClientComponent*` UPROPERTY, connect/disconnect methods |
| All 15 UI subsystems (`*Subsystem.cpp`) | Change `FindSocketIOComponent()` to use GameInstance |
| `SkillVFXSubsystem.cpp` | Same — uses FindSocketIOComponent |
| `SabriMMOCharacter.cpp` | Same — finds socket for position/combat events |
| `ZoneTransitionSubsystem.cpp` | Remove disconnect expectation from zone change flow |
| `server/src/index.js` | Add `zone:transition` event, remove reconnect from zone path |

### Must Remove
| File | Change |
|------|--------|
| Level Blueprints (all 4 levels) | Remove BP_SocketManager actor spawn |
| `BP_SocketManager` Blueprint | Delete or repurpose (event bindings move to C++) |

### Must Audit
| File | Reason |
|------|--------|
| `BP_SocketManager` Blueprint | All event bindings must be cataloged and migrated |
| `AC_HUDManager` Blueprint | May reference BP_SocketManager |
| All Level Blueprints | May reference BP_SocketManager for initial setup |
