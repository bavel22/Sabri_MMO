# MMO Development Progress — 2026-02-09

## Summary
Combat system refinement session focusing on client-side integration, range tolerance, remote player rotation data, and comprehensive Blueprint instructions for all pending UI/combat features.

---

## Server-Side Changes (`server/src/index.js`)

### 1. Range Tolerance System
- Added `RANGE_TOLERANCE: 50` to COMBAT constants
- `combat:out_of_range` event now sends `requiredRange: attackRange - RANGE_TOLERANCE` (instead of raw attackRange)
- **Purpose**: When client moves toward target and stops at `requiredRange`, they're actually 50 units INSIDE the real attack range — prevents position-sync boundary issues where player stops at exact range but server sees them slightly out of range
- Applies to both enemy and player target range checks

### 2. Remote Player Rotation Data
- `combat:damage` payload now includes attacker and target positions:
  - `attackerX`, `attackerY`, `attackerZ` — attacker's Redis-cached position
  - `targetX`, `targetY`, `targetZ` — target's position (Redis for players, in-memory for enemies)
- **Purpose**: Remote clients can calculate the facing rotation for the attacker toward the target, enabling proper rotation animation on BP_OtherPlayerCharacter actors

### 3. Health in Position Broadcasts (User Change)
- Initial join broadcast (`player:moved` on join, lines 273-282) now includes `health` and `maxHealth` from database
- Regular 30Hz position updates (`player:moved`, lines 391-398) now include `health` and `maxHealth` from connectedPlayers map
- Fallback values of 100 used if player data isn't available
- **Purpose**: BP_OtherPlayerManager can pass health to SpawnOrUpdatePlayer so remote players show correct health immediately

### 4. parseInt Fix for Stat Allocation (User Change)
- Line 655: Changed `const pts = amount || 1` to `const pts = parseInt(amount) || 1`
- **Root Cause**: Client sends `amount` as string `"1"`, JavaScript did string concatenation (`"1" + 1 = "11"`) instead of numeric addition
- Now properly converts to integer before arithmetic

---

## Blueprint Instructions Provided

### 1. Target Frame Visibility Fix
- Client must bind BOTH `combat:auto_attack_stopped` AND `combat:target_lost` events
- Both handlers must: Set `IsAutoAttacking = false` + Call `HideTargetFrame` on WBP_GameHud
- `combat:auto_attack_stopped` fires on manual stop; `combat:target_lost` fires on target death/disconnect/respawn

### 2. Health Bar Initial Load
- `OnCombatHealthUpdate` handler should also find BP_OtherPlayerCharacter via BP_OtherPlayerManager and call `UpdateHealth` on its WBP_TargetHealthBar
- With health now included in `player:moved`, SpawnOrUpdatePlayer can initialize health immediately

### 3. Respawn Teleport for Remote Players
- `OnCombatRespawn` handler for remote players must use `SetActorLocation` with Teleport=true
- MUST also set `TargetPosition` variable to the new location (prevents walking back on next tick)

### 4. Enemy Spawn Positioning
- `enemy:spawn` handler must pass x/y/z as **Spawn Transform** in `Spawn Actor from Class`
- BP_EnemyCharacter's `Event BeginPlay` should init `TargetPosition = GetActorLocation()`

### 5. Remote Player Rotation
- Parse `attackerX/Y/Z` and `targetX/Y/Z` from `combat:damage`
- Use `Find Look at Rotation` (Start=attacker pos, Target=target pos)
- Apply via `Set Actor Rotation` on BP_OtherPlayerCharacter with Teleport=true

### 6. Enemy Target ID Fix
- When emitting `combat:attack` for enemies, client MUST use `targetEnemyId` field (not `targetCharacterId`)
- Cast clicked actor to determine type: BP_EnemyCharacter → use `targetEnemyId`, BP_OtherPlayerCharacter → use `targetCharacterId`

### 7. Stat Window Setup
- Full step-by-step: IA_OpenStats input action, toggle WBP_StatAllocation widget
- Bind `player:stats` event in BP_SocketManager, create `OnPlayerStats` callback
- `UpdateStats` function parses JSON and updates text blocks + button enabled states
- Each +button emits `player:allocate_stat` with correct `statName` string

---

## .gitignore Update
- Added paths for `client/SabriMMO/` nested UE5 project structure (Binaries, DerivedDataCache, Intermediate, Saved, .vs, .diversion, Config, Content, Plugins)

---

## New Socket Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| (none new) | — | All changes were to existing event payloads |

### Updated Event Payloads

| Event | Change |
|-------|--------|
| `combat:out_of_range` | `requiredRange` now = `attackRange - RANGE_TOLERANCE` (50 units padding) |
| `combat:damage` | Added `attackerX/Y/Z` and `targetX/Y/Z` position fields |
| `player:moved` | Added `health` and `maxHealth` fields |

---

## Next Steps
- Fix UE5 crash on player kill (event ordering issue)
- Fix wrong remote player receiving animations/health updates (characterId lookup)
- Load stats from DB into stat window on open
- Fix remote player rotation direction
- Update all documentation
