# MMO Development Progress — 2026-02-08

## Summary
Major server-side overhaul: stat system, enemy/NPC system, combat improvements, health persistence, respawn fixes, and comprehensive documentation updates.

---

## Server-Side Changes (`server/src/index.js`)

### 1. RO-Style Stat System
- Added `calculateDerivedStats(stats)` — computes statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, aspd, maxHP, maxSP from base stats (STR/AGI/VIT/INT/DEX/LUK)
- Added `calculatePhysicalDamage(attackerStats, targetStats)` — RO-style damage formula with variance, soft DEF, and critical hits
- Player stats loaded from DB on `player:join`, sent via `player:stats` event
- New event: `player:allocate_stat` — server-side stat point allocation with DB persistence
- DB migration: auto-adds `str`, `agi`, `vit`, `int_stat`, `dex`, `luk`, `stat_points`, `max_health`, `max_mana` columns to `characters` table on startup

### 2. Enemy/NPC System
- Enemy IDs start at **2000001** (not 1000)
- 6 enemy templates with RO-inspired but obscured names:
  - **Blobby** (Lv.1) — passive, 50 HP
  - **Hoplet** (Lv.3) — passive, 100 HP
  - **Crawlid** (Lv.2) — passive, 75 HP
  - **Shroomkin** (Lv.4) — passive, 120 HP
  - **Buzzer** (Lv.5) — aggressive, 150 HP
  - **Mosswort** (Lv.3) — passive, 90 HP
- 12 spawn points across the map
- Each enemy has: stats, ASPD, attack range, aggro range, exp, respawn timer, AI type
- Enemies spawn at server startup, respawn after death (configurable timer per template)
- New events: `enemy:spawn`, `enemy:death`, `enemy:health_update`

### 3. Combat Tick Loop Overhaul
- Now handles **player-vs-player** AND **player-vs-enemy** combat
- New `isEnemy` flag on auto-attack state to differentiate target types
- `combat:attack` now accepts both `targetCharacterId` (player) and `targetEnemyId` (enemy)
- `combat:out_of_range` event — server tells client when attacker is out of range (with target position), so client can pathfind toward target and stop at melee range
- `combat:target_lost` now includes `isEnemy` field
- `combat:damage` now includes `isEnemy` field
- Enemy death stops all attackers, clears combat sets, broadcasts `enemy:death` with exp reward
- Enemy respawn via `setTimeout` after configurable `respawnMs`

### 4. Respawn Fixes
- `combat:respawn` now accepts data parameter (can be empty `{}`)
- Respawn stops all players auto-attacking the dead player (sends `combat:target_lost` with reason "Target respawned")
- Respawn payload now includes `teleport: true` flag — clients should use this to teleport the respawned character instead of pathfinding
- Health saved to DB via `savePlayerHealthToDB()` helper

### 5. Health Persistence
- **On disconnect**: Health, mana, and stats saved to DB
- **Periodic save**: Every 60 seconds, all connected (alive) players' health/mana saved to DB
- **On death**: Target health saved as 0
- **On respawn**: Full health/mana saved to DB
- New helper: `savePlayerHealthToDB(characterId, health, mana)`

### 6. Disconnect Improvements
- Saves health/mana to DB before cleanup
- Saves stats to DB before cleanup
- Removes disconnected player from all enemy `inCombatWith` sets
- Checks `isEnemy` flag when clearing auto-attack states

### 7. `combat:stop_attack` Improvements
- Now removes the attacker from the enemy's `inCombatWith` set when stopping attack on an enemy

---

## Documentation Updates

### `docs/UI_Widgets.md`
- Added comprehensive documentation for all in-game widgets:
  - **WBP_GameHud** — Main HUD with player/target health, mana, name
  - **HoverOverIndicator** — Widget Component for hover/selection on actors
  - **WBP_DeathOverlay** — Death screen with "Return to Save Point" button
  - **WBP_DamageNumber** — Floating damage text (color rules: white for dealing, red for taking)
  - **WBP_TargetHealthBar** — Health bar above target's head (visibility rules for enemies)
  - **WBP_PlayerNameTag** — Name tag above characters/enemies
  - **WBP_ChatBox** — Chat panel with message list and input
  - **WBP_ChatMessageLine** — Individual chat message line
  - **WBP_StatAllocation** — Stat point distribution panel
- Added Widget Relationship Diagram showing which Blueprints create/update which widgets

---

## New Socket Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `player:stats` | Server → Client | Send base stats + derived stats |
| `player:allocate_stat` | Client → Server | Allocate stat points (statName, amount) |
| `enemy:spawn` | Server → Client | Enemy spawned/respawned |
| `enemy:death` | Server → Client | Enemy killed (includes exp) |
| `enemy:health_update` | Server → Client | Enemy health changed (includes inCombat flag) |
| `combat:out_of_range` | Server → Client | Attacker out of melee range (includes target position) |

### Updated Events
| Event | Change |
|-------|--------|
| `combat:attack` | Now accepts `targetEnemyId` in addition to `targetCharacterId` |
| `combat:damage` | Now includes `isEnemy` field |
| `combat:death` | Now includes `isEnemy` field |
| `combat:target_lost` | Now includes `isEnemy` field |
| `combat:auto_attack_started` | Now includes `isEnemy` field |
| `combat:respawn` | Now includes `teleport: true` flag, accepts data param |

---

## Blueprint Instructions Provided

1. **WBP_StatAllocation** — Full step-by-step creation guide
2. **BP_EnemyCharacter** — UpdateHealth function clarification (uses WBP_TargetHealthBar.UpdateHealth)
3. **BP_EnemyManager.GetEnemy** — Detailed node-by-node instructions for returning a BP_EnemyCharacter reference
4. **Rotate to Target** — Instructions for BP_MMOCharacter and BP_OtherPlayerCharacter to face target before/during attack animation

---

## Issues Addressed

1. **combat:respawn JSON field**: Event accepts optional data `{}` — no specific field required
2. **OnCombatDeath**: Confirmed it already exists — user appended death overlay to it
3. **Respawn teleport**: Added `teleport: true` flag to respawn payload; client should check this flag and use `SetActorLocation` instead of pathfinding
4. **Health DB persistence**: Now saved on disconnect, on death, on respawn, and every 60 seconds
5. **combat:target_lost**: Verified it is broadcast in all scenarios (target death, target disconnect, target respawn, enemy death)
6. **Enemy health bar visibility**: Server sends `inCombat` flag with `enemy:health_update`; client shows/hides health bar based on this
7. **Stay at melee range**: Added `combat:out_of_range` event so client knows to move toward target then stop at range

---

## Database Changes

New columns added to `characters` table (auto-migrated on server startup):
```sql
ALTER TABLE characters 
ADD COLUMN IF NOT EXISTS str INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS agi INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS vit INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS int_stat INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS dex INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS luk INTEGER DEFAULT 1,
ADD COLUMN IF NOT EXISTS stat_points INTEGER DEFAULT 48,
ADD COLUMN IF NOT EXISTS max_health INTEGER DEFAULT 100,
ADD COLUMN IF NOT EXISTS max_mana INTEGER DEFAULT 100;
```

---

## Next Steps
- Implement client-side handling for all new events in BP_SocketManager
- Build BP_EnemyCharacter and BP_EnemyManager Blueprints
- Build WBP_StatAllocation widget
- Implement damage number float-upward animation
- Add attack animation montage system
- Implement character rotation toward target during attacks
