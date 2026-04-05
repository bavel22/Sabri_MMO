# MMO Development Progress — 2026-02-10

## Summary
Critical bug fixes for player kill crash, wrong remote player targeting, stats loading, and remote player rotation. Documentation updates for 02-09 and 02-10 sessions.

---

## Server-Side Changes (`server/src/index.js`)

### 1. Player Kill Crash Fix (MAJOR)
- **Problem**: UE5 crashed when killing another player. The killer received `combat:target_lost` AND `combat:death` nearly simultaneously, causing double-processing and null reference crashes on the client.
- **Fix**: The killer no longer receives `combat:target_lost` during the death sequence — only `combat:death`. Other attackers still receive `combat:target_lost` as before.
- **Additional**: `combat:death` payload now includes `targetHealth: 0` and `targetMaxHealth` so clients can safely update health without accessing stale references.
- **Event order for killer**: `combat:damage` (health=0) → `combat:death` (2 events, not 3)
- **Event order for other attackers**: `combat:target_lost` → `combat:death` (unchanged)

### 2. Stats Request Event (MAJOR)
- **Problem**: Stat allocation window only showed correct values after pressing +/- buttons (which triggers `player:allocate_stat` → `player:stats` response). On initial open, stats were empty/default.
- **Fix**: Added `player:request_stats` event handler (Client → Server). When the client opens the stat window, it emits `player:request_stats` (no data needed). Server responds with `player:stats` containing current base stats + derived stats from in-memory player data.
- **Client flow**: Open stat window → Emit `player:request_stats` → Server responds with `player:stats` → `OnPlayerStats` handler calls `UpdateStats` on WBP_StatAllocation

### 3. combat:death Payload Enhancement
- Added `targetHealth: 0` and `targetMaxHealth` to `combat:death` payload
- Clients can now update health bars directly from death event without needing to reference the actor's current state

---

## Blueprint Instructions Provided

### 1. Wrong Remote Player Fix (MAJOR — characterId-based lookup)
- **Root Cause**: Client uses `Get Actor of Class` (returns FIRST found instance) instead of looking up by characterId. With 3+ players, all events target the same (first) remote actor.
- **Fix**: Use a `PlayerMap` (Map<Integer, BP_OtherPlayerCharacter>) in BP_OtherPlayerManager. Look up actors by characterId from event payloads.
- **Affected handlers**: `OnCombatDamage`, `OnCombatHealthUpdate`, `OnCombatDeath`, `OnCombatRespawn`, hover indicator visibility
- Detailed node-by-node instructions provided (see Blueprint Instructions section below)

### 2. Stats Loading on Window Open
- When opening WBP_StatAllocation, emit `player:request_stats` immediately after creating the widget
- Server responds with `player:stats` which triggers `OnPlayerStats` → `UpdateStats`

### 3. Remote Player Rotation Direction Fix
- **Root Cause**: UE5 and the server may use different coordinate conventions. The `attackerX/Y/Z` from Redis are in UE5 coordinates, but `Find Look at Rotation` needs proper axis mapping.
- **Fix**: Verify that X/Y mapping is consistent between server Redis cache and UE5 world coordinates. The server stores whatever the client sends via `player:position`.

---

## Updated Event Reference

### New Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `player:request_stats` | Client → Server | (none) | Request current stats for stat window |

### Updated Events

| Event | Change |
|-------|--------|
| `combat:death` (player) | Added `targetHealth: 0`, `targetMaxHealth` fields |
| `combat:target_lost` | No longer sent to the KILLER on player death (only to other attackers) |

---

## Git Commits
- `549d7c1` — Combat system: range tolerance, damage positions, health in broadcasts, parseInt fix, gitignore update
- (pending) — Player kill crash fix, request_stats event, documentation updates

---

## Known Issues Being Addressed

| Issue | Status | Fix Type |
|-------|--------|----------|
| UE5 crash on player kill | Fixed server-side (event ordering) | Server |
| Wrong remote player animations/health | Blueprint instructions provided | Client |
| Stats not loading on window open | Fixed server-side (request_stats event) | Server + Client |
| Remote player wrong rotation direction | Blueprint instructions provided | Client |

---

## Next Steps
- Implement characterId-based PlayerMap lookup in BP_OtherPlayerManager (client)
- Emit `player:request_stats` when opening stat window (client)
- Test player kill with new event ordering
- Test 3-player scenario for correct remote targeting
- Implement proper coordinate mapping for remote rotation
