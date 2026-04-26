# Node.js Server Documentation

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [System_Architecture](../01_Architecture/System_Architecture.md) | [Combat_System](Combat_System.md) | [Skill_System](Skill_System.md) | [Enemy_System](Enemy_System.md) | [API_Documentation](API_Documentation.md)

## Overview

The server is a single monolithic file: `server/src/index.js` (35,281 lines as of 2026-04-15) plus 21 `ro_*.js` data modules (~7,000 lines total — combat formulas, skills, monsters, items, cards, buffs, statuses, AI codes, navmesh, world map, ensembles, etc.). It handles REST API (11 endpoints), Socket.io real-time events (106 `socket.on` handlers), combat tick loop, enemy AI state machine, inventory management, skill system, zone transitions, NPC shops, party system, buff/debuff tracking, audio SFX event broadcasting, ground item drops, and database operations.

**Runtime**: Node.js 18+ LTS
**Port**: 3001 (configurable via `PORT` env var)
**Entry**: `node src/index.js` or `npm start`

## File Structure

```
server/
├── src/
│   ├── index.js                  # Main server (35,281 lines as of 2026-04-15)
│   ├── ro_monster_templates.js   # 509 RO monster definitions
│   ├── ro_item_mapping.js        # RO item name ↔ ID mapping
│   ├── ro_exp_tables.js          # EXP tables, class config, level caps
│   ├── ro_skill_data.js          # 69 first-class skill definitions
│   ├── ro_skill_data_2nd.js      # 224 second-class skill definitions
│   ├── ro_monster_ai_codes.js    # 1,004 monster AI code mappings
│   ├── ro_zone_data.js           # Zone registry, warps, spawns, Kafra NPCs
│   ├── ro_damage_formulas.js     # Physical/magical damage, elements, size (1,079 lines)
│   ├── ro_buff_system.js         # 95 buff types, apply/expire/modifiers (1,179 lines)
│   ├── ro_status_effects.js      # 10 status effects, resist/apply/tick (711 lines)
│   ├── ro_ground_effects.js      # Ground AoE zones: traps, songs, dances (622 lines)
│   ├── ro_monster_skills.js      # 40+ NPC_ monster skills (548 lines)
│   ├── ro_item_effects.js        # Consumable use effects, item scripts (520 lines)
│   ├── ro_card_prefix_suffix.js  # Card naming system (541 lines)
│   ├── ro_homunculus_data.js     # 4 homunculus types, growth tables (243 lines)
│   ├── ro_arrow_crafting.js      # 45 arrow crafting recipes (77 lines)
│   └── test_mode.js              # Test mode routes
├── .env                          # Environment variables
├── package.json                  # Dependencies
├── logs/
│   └── server.log                # Runtime log output
├── check_database.js             # DB diagnostic script
├── check_table.js                # Table diagnostic script
├── test_auth.js                  # Auth test script
└── test_login.js                 # Login test script
```

## Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| `express` | ^4.18.2 | REST API framework |
| `socket.io` | ^4.8.3 | Real-time WebSocket server |
| `pg` | ^8.11.3 | PostgreSQL client (Pool) |
| `redis` | ^5.10.0 | Redis client (v4+ async API) |
| `bcrypt` | ^5.1.1 | Password hashing (10 salt rounds) |
| `jsonwebtoken` | ^9.0.3 | JWT creation/verification (24h expiry) |
| `cors` | ^2.8.5 | Cross-origin request handling (all origins) |
| `express-rate-limit` | ^8.2.1 | 100 requests / 15 min on `/api/*` |
| `dotenv` | ^16.3.1 | `.env` file loading |
| `nodemon` | ^3.0.2 | Dev auto-restart (devDependency) |

## Environment Variables (.env)

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT` | 3001 | Server port |
| `DB_HOST` | localhost | PostgreSQL host |
| `DB_PORT` | 5432 | PostgreSQL port |
| `DB_NAME` | sabri_mmo | Database name |
| `DB_USER` | postgres | Database user |
| `DB_PASSWORD` | _(required)_ | Database password |
| `JWT_SECRET` | _(required)_ | JWT signing secret |
| `LOG_LEVEL` | INFO | Logging level (DEBUG/INFO/WARN/ERROR) |

## Data Module Imports (8 modules)

| Module | File | Key Exports |
|--------|------|-------------|
| `ro_monster_templates` | `ro_monster_templates.js` | `RO_MONSTER_TEMPLATES` (509 monsters) |
| `ro_item_mapping` | `ro_item_mapping.js` | `RO_ITEM_NAME_TO_ID`, `EXISTING_ITEM_EXTRA_DROPS` |
| `ro_exp_tables` | `ro_exp_tables.js` | `BASE_EXP_TABLE`, `JOB_CLASS_CONFIG`, `MAX_BASE_LEVEL`, `FIRST_CLASSES`, `SECOND_CLASS_UPGRADES`, stat/skill point helpers |
| `ro_skill_data` | `ro_skill_data.js` | `ALL_SKILLS`, `SKILL_MAP`, `CLASS_SKILLS`, `CLASS_PROGRESSION`, `getAvailableSkills`, `canLearnSkill` |
| `ro_monster_ai_codes` | `ro_monster_ai_codes.js` | `MONSTER_AI_CODES` (1,004 monster ID → AI type code mappings) |
| `ro_zone_data` | `ro_zone_data.js` | `ZONE_REGISTRY`, `getZone`, `getAllEnemySpawns`, `getZoneNames` |
| `ro_damage_formulas` | `ro_damage_formulas.js` | `ELEMENT_TABLE` (10×10×4, rAthena pre-renewal canonical), `SIZE_PENALTY` (18 weapon types × 3 sizes), `calculateDerivedStats`, `calculatePhysicalDamage` (per-category multiplicative card stacking), `calculateMagicalDamage`, hit/crit rate, `getElementModifier`, `getSizePenalty` |
| `test_mode` | `test_mode.js` | Express Router for `/test/*` routes |

## Server Startup Sequence

```
1. Start HTTP server on PORT
2. Test Redis connection (ping)
3. Auto-add missing stat columns to characters table
4. Ensure items table exists (CREATE TABLE IF NOT EXISTS)
5. Ensure weapon_type columns exist on items table
6. Ensure character_inventory table + indexes exist
7. Ensure skills, skill_prerequisites, skill_levels, character_skills tables exist
8. Ensure character_hotbar table exists with row_index support
9. Seed base items if items table is empty (22 items)
10. Upsert skill definitions from ro_skill_data into skills/skill_levels/skill_prerequisites tables
11. Load item definitions into memory cache
12. Enemy spawning is LAZY — enemies spawn when first player enters a zone
```

## In-Memory Data Structures

### connectedPlayers: Map<charId, PlayerData>
```javascript
{
    socketId, characterName, userId, class, level,
    health, maxHealth, mana, maxMana, isDead,
    stats: { str, agi, vit, int, dex, luk },
    derivedStats, baseExp, jobExp, jobLevel, jobClass,
    skillPoints, statPoints, zuzucoin, learnedSkills,
    buffs, zone, lastX, lastY, lastZ,
    weaponATK, weaponType, weaponLevel, weaponElement,
    hardDef, equipATK, equipDEF, equipMDEF, equipBonuses,
    equipHIT, equipFLEE, equipCrit, equipASPDMod,
    maxHPBonus, maxSPBonus, hairStyle, hairColor, gender,
    saveMap, saveX, saveY, saveZ, passiveATK
}
```

### enemies: Map<enemyId, EnemyData>
```javascript
{
    enemyId, templateId, name, level, health, maxHealth,
    damage, defense, mdef, stats, element, race, size, monsterClass,
    attackRange, attackDelay, attackMotion, damageMotion, moveSpeed,
    aggroRange, chaseRange, walkSpeed, baseExp, jobExp,
    modeFlags, drops, mvpDrops,
    x, y, z, spawnX, spawnY, spawnZ, zone,
    isDead, aiState, targetPlayerId, inCombatWith (Set),
    isWandering, wanderTargetX, wanderTargetY, nextWanderTime,
    aggroOriginX, aggroOriginY, lastAttackTime, lastMoveBroadcast,
    lastDamageTime, lastAggroScan, pendingTargetSwitch, buffs
}
```

### Other Structures

| Name | Type | Purpose |
|------|------|---------|
| `autoAttackState` | Map<charId, { targetCharId, isEnemy, startTime, lastAttackTime }> | Active auto-attack states |
| `activeCasts` | Map<charId, { skillId, targetId, isEnemy, learnedLevel, levelData, skill, castStartTime, castEndTime, actualCastTime, socketId, casterName, groundX, groundY, groundZ }> | In-progress skill casts |
| `afterCastDelayEnd` | Map<charId, timestamp> | After-Cast Delay expiry times |
| `activeGroundEffects` | Map<effectId, { id, type, casterId, x, y, z, duration, createdAt, expiresAt, damage, hitCount, maxHits, radius, zone, element, hitTargets }> | Fire Wall / Safety Wall effects |
| `itemDefinitions` | Map<itemId, ItemDef> | Cached item rows from DB |
| `spawnedZones` | Set | Zones that have had enemies spawned |
| `zoneTransitioning` | Set | Character IDs mid-zone-transition |
| `ENEMY_TEMPLATES` | Object | 509 RO monsters adapted from `RO_MONSTER_TEMPLATES` |

## Constants

### COMBAT
```javascript
{
    BASE_DAMAGE: 1, DAMAGE_VARIANCE: 0,
    MELEE_RANGE: 150, RANGED_RANGE: 800,
    DEFAULT_ASPD: 175, ASPD_CAP: 195,
    RANGE_TOLERANCE: 50, COMBAT_TICK_MS: 50,
    RESPAWN_DELAY_MS: 5000,
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }
}
```

### ENEMY_AI
```javascript
{
    TICK_MS: 200,
    AGGRO_SCAN_MS: 500, MOVE_BROADCAST_MS: 200,
    CHASE_GIVE_UP_EXTRA: 200, IDLE_AFTER_CHASE_MS: 3000,
    WANDER_PAUSE_MIN: 3000, WANDER_PAUSE_MAX: 8000
}
```

### Other Constants

| Name | Purpose |
|------|---------|
| `MD` (Mode Flags) | 16 bitmask flags for monster behavior (aggressive, assist, detector, knockbackImmune, etc.) |
| `AI_TYPE_MODES` | 27 AI type codes (1-27) → hex mode bitmask values |
| `AI_STATE` | IDLE=0, CHASE=1, ATTACK=2, DEAD=3 |
| `SPEED_TONICS` | 4 consumable tonic definitions (ASPD bonuses) |
| `INVENTORY` | MAX_SLOTS: 100, MAX_WEIGHT: 2000 |
| `PVP_ENABLED` | `false` (all PvP damage disabled) |

## REST API Endpoints (12)

| Method | Path | Auth | Description |
|--------|------|------|-------------|
| `GET` | `/health` | No | Health check (DB ping, timestamp) |
| `POST` | `/api/auth/register` | No | Register user (bcrypt hash, JWT 24h, validation: name 3-50 chars, email regex, password 8+ alphanumeric) |
| `POST` | `/api/auth/login` | No | Login (password verify, JWT 24h, updates last_login) |
| `GET` | `/api/auth/verify` | Yes | Verify JWT, return user info |
| `GET` | `/api/servers` | No | Server list (single "Sabri" server, population = connectedPlayers.size) |
| `GET` | `/api/characters` | Yes | List user's characters (max 9, soft-delete filter, enriched with level_name from zone registry) |
| `POST` | `/api/characters` | Yes | Create character (name 2-24 alphanum, 9 per account, global unique, novice class, 48 stat points) |
| `GET` | `/api/characters/:id` | Yes | Get single character by ID |
| `DELETE` | `/api/characters/:id` | Yes | Soft-delete character (requires password via bcrypt, checks if online) |
| `PUT` | `/api/characters/:id/position` | Yes | Save character position (x, y, z) |
| `GET` | `/api/test` | No | Simple test ("Hello from MMO Server!") |
| `*` | `/test/*` | No | Test mode routes (delegated to test_mode router) |

## Socket.io Events

### Client → Server (34 events)

| Event | Description |
|-------|-------------|
| `player:join` | Join game (JWT auth + character ownership verify, DB load, stats calc, zone room join, lazy spawn enemies) |
| `player:position` | Position update (broadcast to zone, Redis cache, cast interrupt on movement > 5 units) |
| `player:request_stats` | Request full stat recalculation (sends player:stats) |
| `player:allocate_stat` | Allocate stat point (validates cost formula, increments stat, saves to DB) |
| `zone:warp` | Warp portal zone transition (validates warp ID from zone registry, saves state, sends zone:change) |
| `zone:ready` | Post-zone-load sync (broadcasts player:joined, sends existing enemies/players) |
| `kafra:open` | Open Kafra NPC dialog (validates kafra ID from zone registry) |
| `kafra:save` | Save respawn point (save_map, save_x/y/z to DB) |
| `kafra:teleport` | Kafra teleport to save point (zone transition if different zone) |
| `combat:attack` | Start auto-attack on target (validates range, starts attack state) |
| `combat:stop_attack` | Stop auto-attacking |
| `combat:respawn` | Respawn after death (restore HP/SP, teleport to save point, cross-zone respawn support) |
| `job:change` | Change job class (Novice→1st at JLv10, 1st→2nd at JLv40+, resets job level/exp) |
| `skill:data` | Request skill tree data (available + learned skills for current class) |
| `skill:learn` | Learn/upgrade skill (validates prerequisites, SP cost, saves to DB) |
| `skill:reset` | Reset all skills (refund skill points, clear character_skills) |
| `skill:use` | Use active skill (17+ skill handlers, cast time, cooldowns, ACD, ground targeting) |
| `chat:message` | Send chat message (GLOBAL channel, broadcast to zone) |
| `inventory:load` | Request full inventory data |
| `inventory:use` | Use consumable (potions, Fly Wing 1029, Butterfly Wing 1028) |
| `inventory:equip` | Equip/unequip item (validates level req, dual-accessory support) |
| `inventory:drop` | Drop item (partial/full quantity, DB delete) |
| `inventory:move` | Move item to different inventory slot |
| `hotbar:save` | Save item to hotbar slot (DB upsert) |
| `hotbar:request` | Request all hotbar data (4 rows × 9 slots) |
| `hotbar:save_skill` | Save skill to hotbar slot (supports zeroBased flag) |
| `hotbar:clear` | Clear a hotbar slot |
| `shop:open` | Open NPC shop (validates shopId, sends item list) |
| `shop:buy` | Buy single item (deprecated, kept for legacy) |
| `shop:sell` | Sell single item (deprecated, kept for legacy) |
| `shop:buy_batch` | Buy multiple items (Discount skill applied, weight check) |
| `shop:sell_batch` | Sell multiple items (Overcharge skill applied) |
| `disconnect` | Client disconnect (save all state to DB, cleanup combat/cast/autoattack/rooms) |

### Server → Client (43+ events)

| Event | Description |
|-------|-------------|
| `player:joined` | New player joined zone (full character data) |
| `player:moved` | Player position updated |
| `player:left` | Player left zone / disconnected |
| `player:stats` | Full stat sheet (base + derived + equipment + buffs) |
| `player:teleport` | Teleport player within zone |
| `player:join_error` | Join failed (auth error, character not found) |
| `combat:damage` | Damage dealt (auto-attack AND enemy attacks; attacker, target, damage, isCritical, isMiss, hitType, element, coordinates) |
| `combat:health_update` | HP/MP updated on target |
| `combat:death` | Entity killed (player or enemy) |
| `combat:respawn` | Player respawned |
| `combat:error` | Combat error |
| `combat:auto_attack_started` | Auto-attack loop started |
| `combat:auto_attack_stopped` | Auto-attack loop stopped |
| `combat:target_lost` | Target no longer valid |
| `combat:out_of_range` | Target out of range |
| `enemy:spawn` | Enemy spawned in zone (full template data) |
| `enemy:death` | Enemy killed (with loot drops) |
| `enemy:health_update` | Enemy health changed |
| `enemy:move` | Enemy AI movement (wander/chase, includes target coords) |
| `enemy:attack` | Enemy attacked player (enemyId, targetId, attackMotion) |
| `exp:gain` | EXP gained (base + job, totals, progress %) |
| `exp:level_up` | Level up (base or job level) |
| `skill:data` | Skill tree data (available + learned) |
| `skill:learned` | Skill learned/upgraded |
| `skill:refresh` | Skill data refreshed after reset |
| `skill:error` | Skill action error |
| `skill:used` | Skill used (instant cast, no cast time) |
| `skill:cast_start` | Cast bar started (skillId, castTime, casterName) |
| `skill:cast_complete` | Cast completed |
| `skill:cast_interrupted` | Cast interrupted (damage or movement) |
| `skill:effect_damage` | Skill damage per-hit (hit index, total hits, coordinates) |
| `skill:buff_applied` | Buff/debuff applied (type, duration, target) |
| `skill:buff_removed` | Buff/debuff removed |
| `skill:status_applied` | Status effect applied (frozen, stoned) |
| `skill:cooldown_started` | Skill cooldown started |
| `skill:acd_started` | After-Cast Delay started (global skill lockout) |
| `skill:ground_effect_created` | Ground effect placed (Fire Wall, Safety Wall) |
| `skill:ground_effect_removed` | Ground effect expired/destroyed |
| `skill:ground_effect_blocked` | Safety Wall blocked damage |
| `skill:reset_complete` | Skills reset, points refunded |
| `zone:change` | Zone transition instruction (newZone, levelName, spawnX/Y/Z) |
| `zone:data` | Zone data on entry |
| `zone:error` | Zone transition error |
| `kafra:data` | Kafra NPC data (services available) |
| `kafra:saved` | Save point confirmed |
| `kafra:teleported` | Kafra teleport confirmed |
| `kafra:error` | Kafra error |
| `inventory:data` | Full inventory list |
| `inventory:equipped` | Item equipped/unequipped |
| `inventory:dropped` | Item dropped |
| `inventory:used` | Consumable used |
| `inventory:error` | Inventory error |
| `hotbar:alldata` | All hotbar slots (4 rows × 9 slots) |
| `shop:data` | NPC shop item list |
| `shop:bought` | Purchase confirmed |
| `shop:sold` | Sale confirmed |
| `shop:error` | Shop error |
| `loot:drop` | Item dropped by enemy (for future ground loot) |
| `chat:receive` | Chat message (sender, message, channel, timestamp) |
| `job:changed` | Job class changed |
| `job:error` | Job change error |

## Periodic Tick Loops (8)

| Loop | Interval | Purpose |
|------|----------|---------|
| Combat Tick | 50ms | Process all auto-attacks (player→enemy, enemy→player, range/ASPD checks, damage, death/loot) |
| Enemy AI Tick | 200ms | Full state machine (IDLE→CHASE→ATTACK→DEAD), wander, aggro scan, chase, attack timing |
| HP Natural Regen | 6,000ms | `max(1, floor(MaxHP/200)) + floor(VIT/5)` |
| SP Natural Regen | 8,000ms | `1 + floor(MaxSP/100) + floor(INT/6)`, bonus if INT ≥ 120 |
| Skill-Based Regen | 10,000ms | HP Recovery (102): `Lv*5 + Lv*MaxHP/500`; SP Recovery (204): `Lv*3 + Lv*MaxSP/500` |
| Buff Expiry Tick | 1,000ms | Check all buffs, remove expired, Stone Curse HP drain (1% MaxHP/sec) |
| Ground Effects Tick | 500ms | Fire Wall damage ticks, Safety Wall block tracking, expiry cleanup |
| Periodic DB Save | 60,000ms | Save HP/MP, EXP, zone + position for all connected players |

## Stat Formulas (RO Pre-Renewal Style)

### Derived Stats (`calculateDerivedStats` from `ro_damage_formulas.js`)
```
statusATK  = STR + floor(STR/10)² + floor(DEX/5) + floor(LUK/3)
matkMin    = INT + floor(INT/7)² + floor(weaponMATK * 0.7)
matkMax    = INT + floor(INT/5)² + weaponMATK
HIT        = 175 + Level + DEX + bonusHit
FLEE       = 100 + Level + AGI + bonusFlee
softDEF    = floor(VIT/2) + floor(AGI/5) + floor(Level/2)
softMDEF   = INT + floor(VIT/5) + floor(DEX/5) + floor(Level/4)
Critical   = 1 + floor(LUK*0.3) + bonusCrit
PerfDodge  = 1 + floor(LUK/10) + bonusPD
ASPD       = 200 - (WD - floor((WD*AGI/25 + WD*DEX/100) / 10)) * (1 - SpeedMod)
MaxHP      = floor(BaseHP * (1 + VIT*0.01) * TransMod) + bonusMaxHp  [class-aware iterative]
MaxSP      = floor((10 + Level * SP_JOB) * (1 + INT*0.01) * TransMod) + bonusMaxSp
```
Data tables: `HP_SP_COEFFICIENTS`, `ASPD_BASE_DELAYS`, `TRANS_TO_BASE_CLASS`, `TRANSCENDENT_CLASSES` in `ro_exp_tables.js`.

### Attack Interval
```
getAttackIntervalMs(aspd) = (200 - min(aspd, 195)) * 50
  ASPD 175 → 1250ms, ASPD 185 → 750ms, ASPD 195 → 250ms
  Above 195: exponential diminishing returns, floor ~217ms
```

### Cast Time
```
actualCastTime = BaseCastTime * (1 - DEX/150)
  DEX ≥ 150 = instant cast
```

### Stat Point Cost
```
cost = floor((currentStat - 1) / 10) + 2
```

## Skill System

### Active Skills (17 with execution handlers)

| ID | Name | Type | Element | Pattern |
|----|------|------|---------|---------|
| 2 | First Aid | Self Heal | — | Instant HP restore, VFX-only buff |
| 103 | Bash | Physical Single | — | ATK% modifier, Stun proc at Lv6+ |
| 104 | Provoke | Debuff | — | DEF reduction + ATK increase, 30s duration |
| 105 | Magnum Break | Physical AoE | Fire | 500 range, all enemies in radius |
| 106 | Endure | Self Buff | — | MDEF bonus, duration per level |
| 200 | Cold Bolt | Magic Multi-Hit | Water | N=skillLevel bolts, 150ms stagger |
| 201 | Fire Bolt | Magic Multi-Hit | Fire | N=skillLevel bolts, 150ms stagger |
| 202 | Lightning Bolt | Magic Multi-Hit | Wind | N=skillLevel bolts, 150ms stagger |
| 203 | Napalm Beat | Magic AoE | Ghost | Damage split across targets in range |
| 205 | Sight | Self Buff | — | Reveal hidden, 10s duration |
| 206 | Stone Curse | CC Debuff | Earth | Petrification, element override, 1% HP drain/sec |
| 207 | Fire Ball | Magic AoE Projectile | Fire | Single projectile, AoE on impact, primaryTargetId |
| 208 | Frost Diver | Magic CC | Water | Damage + freeze, element override, duration=level*3s |
| 209 | Fire Wall | Ground Persistent | Fire | Ground effect, fire damage ticks, max 3 per caster, 5 hits |
| 210 | Soul Strike | Magic Multi-Hit | Ghost | N=floor((level+1)/2) hits, 200ms stagger |
| 211 | Safety Wall | Ground Protection | — | Blocks N hits (level-based), max 3 per caster |
| 212 | Thunderstorm | Magic Ground AoE | Wind | N=skillLevel strikes, 300ms stagger, random in AoE |

### Passive Skills (7, stat bonuses via `getPassiveSkillBonuses()`)

| ID | Name | Effect |
|----|------|--------|
| 100 | Sword Mastery | +4 ATK/level with daggers & 1H swords |
| 101 | 2H Sword Mastery | +4 ATK/level with 2H swords |
| 102 | HP Recovery | Extra HP regen per 10s tick |
| 204 | SP Recovery | Extra SP regen per 10s tick |
| 600 | Enlarge Weight Limit | +200 max weight per level |
| 601 | Discount | Reduces NPC buy prices (7 + (Lv-1)*2 %) |
| 602 | Overcharge | Increases NPC sell prices (7 + (Lv-1)*2 %) |

## Buff / Debuff System

Buffs stored in `player.buffs[type]` with `expiresAt` timestamp. 1s polling interval checks expiry.

| Type | Effect | Duration | Applied By |
|------|--------|----------|------------|
| `provoke` | DEF reduction + ATK increase | 30,000ms | Provoke (104) |
| `endure` | MDEF bonus | Per skill level | Endure (106) |
| `frozen` | Frozen status, element → Water Lv1 | level * 3,000ms | Frost Diver (208) |
| `stone_curse` | Petrified, element → Earth Lv1, 1% HP drain/sec | Per skill level | Stone Curse (206) |
| `sight` | Reveal hidden | 10,000ms | Sight (205) |
| `first_aid` | VFX-only (no stat effect) | 0ms (immediate) | First Aid (2) |

## NPC Shops (4)

| Shop ID | Name | Items |
|---------|------|-------|
| 1 | Tool Dealer | Consumables (1001-1009), Fly Wing (1029), Butterfly Wing (1028) |
| 2 | Weapon Dealer | Weapons 3001-3018 (18 items) |
| 3 | Armor Dealer | Armors 4001-4014 (14 items) |
| 4 | General Store | Consumables (1001-1005) + basic armors (4001-4003) |

**Pricing**: Buy = `item.price * 2` (with Discount reduction). Sell = `item.price` (with Overcharge increase). Unlimited stock.

## Enemy AI State Machine

| State | Behavior |
|-------|----------|
| `IDLE` | Wander randomly (60% move speed, 3-8s pauses). Aggressive mobs scan for players in aggro range every 500ms. |
| `CHASE` | Move toward target at 100% speed. Give up if target exceeds chase range. Switch target based on AI mode flags. |
| `ATTACK` | Attack target at attackDelay interval. Broadcast `enemy:attack` event. Target switching on melee/chase mode flags. |
| `DEAD` | Removed from combat. Respawn timer per template (10-30s). Drops loot based on template drop tables. |

**Aggro**: `setEnemyAggro()` is central — called from ALL damage paths (auto-attack + all skill types). `triggerAssist()` pulls same-type mobs within 550 UE units.

**AI Codes**: `ro_monster_ai_codes.js` maps 1,004 monster IDs to rAthena AI type codes (1-27). Each code maps to a hex mode bitmask parsed into 18 boolean flags (aggressive, assist, changeTargetMelee, detector, etc.).

## Zone System

- **Zone-scoped broadcasting**: `broadcastToZone()` / `broadcastToZoneExcept()` via Socket.io rooms
- **Lazy enemy spawning**: `spawnedZones` Set tracks spawned zones; enemies spawn on first player entry
- **Zone transitions**: `zone:warp` validates warp ID from `ZONE_REGISTRY`, saves state, sends `zone:change`
- **Cross-zone respawn**: `combat:respawn` detects different-zone save points, triggers `zone:change`
- **Zone registry**: `ro_zone_data.js` defines `ZONE_REGISTRY` with warps, spawns, Kafra NPC positions per zone
- **DB persistence**: `zone_name`, `save_map`, `save_x/y/z` columns on characters table
- **Position caching**: Redis with 5min expiry for player positions

## Database Tables (9)

| Table | Purpose | Auto-Created |
|-------|---------|-------------|
| `users` | User accounts (username, email, bcrypt password, last_login) | No (init.sql) |
| `characters` | Character data (30+ columns, soft-delete, stats, zone, save point) | No (init.sql); stat columns auto-added |
| `items` | Static item definitions (22 seeded, 148+ total with RO items) | Yes |
| `character_inventory` | Per-character inventory (equipped_position, slot_index) | Yes |
| `character_hotbar` | Hotbar slots (4 rows × 9 slots, items + skills) | Migration; columns auto-added |
| `skills` | Skill definitions (upserted from ro_skill_data on startup) | Yes |
| `skill_prerequisites` | Skill prerequisite chains | Yes |
| `skill_levels` | Per-level skill data (SP cost, damage, cast time) | Yes |
| `character_skills` | Learned skills per character | Yes |

## Logging

```javascript
const logger = { debug, info, warn, error };
```
- **Format**: `[ISO_TIMESTAMP] [LEVEL] message`
- **File**: `server/logs/server.log` (append mode)
- **Conventions**: `[RECV]` incoming, `[SEND]` outgoing, `[BROADCAST]` all clients, `[COMBAT]` combat, `[ENEMY]` AI, `[ITEMS]` inventory, `[DB]` database, `[LOOT]` drops

---

**Last Updated**: 2026-03-09
