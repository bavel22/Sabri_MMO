# 11 - Multiplayer Networking Architecture

## Ragnarok Online Classic 3D Replica
**Stack**: Unreal Engine 5.7 (C++ + Blueprints) | Node.js + Express + Socket.io | PostgreSQL + Redis

---

## Table of Contents

1. [Server Architecture](#1-server-architecture)
2. [Socket.io Event Architecture](#2-socketio-event-architecture)
3. [Position Synchronization](#3-position-synchronization)
4. [Combat Tick Loop](#4-combat-tick-loop)
5. [Redis Usage](#5-redis-usage)
6. [Database Interaction Patterns](#6-database-interaction-patterns)
7. [Scalability Considerations](#7-scalability-considerations)
8. [Anti-Cheat Measures](#8-anti-cheat-measures)
9. [UE5 Client Networking](#9-ue5-client-networking)
10. [Error Handling and Recovery](#10-error-handling-and-recovery)

---

## 1. Server Architecture

### 1.1 Single Monolithic Node.js Server (Current Approach)

The server is a single `index.js` file (~8,400 lines) that handles all game logic. This is a deliberate architectural decision for a solo-developer project and follows the Ragnarok Online server tradition (eAthena/rAthena are single-process C servers). The monolith avoids the operational complexity of microservices while the player count remains under ~500 concurrent.

```
server/src/index.js    (~8,400 lines)
  |-- Express REST API   (auth, characters, server list, positions)
  |-- Socket.io events   (real-time game: position, combat, chat, inventory, skills, zones)
  |-- Combat tick loop    (50ms interval, ASPD-based auto-attack processing)
  |-- Enemy AI loop       (200ms interval, state machine per monster)
  |-- Regen tick loops    (HP: 6s, SP: 8s, Skill-based: 10s)
  |-- Buff expiry tick    (1s interval, checks all player and enemy buffs)
  |-- Ground effects tick (500ms interval, Fire Wall damage, Safety Wall charges)
  |-- Cast completion     (checked every combat tick, re-validates then executes)
```

**Why not microservices?**

| Factor | Monolith | Microservices |
|--------|----------|---------------|
| Deployment | `npm run dev` | Docker compose, service mesh, API gateways |
| Latency | Zero inter-service latency | Network hops between services add 1-5ms each |
| State sharing | In-process Maps/Sets (ns access) | Redis/message bus (ms access) |
| Debugging | Single log file, single stack trace | Distributed tracing needed |
| Solo developer | Manageable | Operational overhead not justified |
| Player count <500 | Comfortable on one core | Over-engineered |

**When to split**: If the game scales beyond ~500 concurrent players, the first split should be zone-based sharding (each zone on its own process), coordinated through Redis Pub/Sub. The monolith would become a gateway/login server that routes connections to the correct zone process.

### 1.2 Server-Authoritative Model

**Every authoritative game decision happens on the server.** The client is presentation and input only.

| Domain | Server Responsibility | Client Role |
|--------|----------------------|-------------|
| **Combat** | Damage calculation, HIT/FLEE rolls, critical chance, element modifiers, range validation | Sends `combat:attack` target ID; plays animation on `combat:damage` event |
| **Stats** | Derived stat formulas, stat point cost validation, equipment bonuses | Displays values from `player:stats`; sends `player:allocate_stat` requests |
| **Movement** | Position caching, cast interruption on move, zone boundary enforcement | Sends position at ~15 Hz; interpolates other players |
| **Inventory** | Item creation, stacking, equip validation, weight limits | Renders UI from `inventory:data`; sends equip/use/drop requests |
| **Skills** | SP cost validation, cooldown enforcement, cast time management, AoE hit detection | Sends `skill:use` with target; plays VFX on skill events |
| **Economy** | Zeny (Zuzucoin) tracking, NPC buy/sell price calculation, trade validation | Displays balances; sends buy/sell requests |
| **Zone transitions** | Warp validation, zone registry lookup, spawn position assignment | Triggers `OpenLevel()` on `zone:change`; sends `zone:ready` when loaded |

**Trust boundary**: The server never trusts any value sent by the client for authoritative data. Client sends intentions (who to attack, what to use); server validates and executes.

### 1.3 Connection Flow

```
Client                          Server                          Database
  |                               |                               |
  |--- POST /api/auth/login ----->|                               |
  |                               |--- SELECT password_hash ----->|
  |                               |<-- bcrypt compare ------------|
  |<-- { token, user } ----------|                               |
  |                               |                               |
  |--- GET /api/characters ------>|  (Authorization: Bearer token)|
  |                               |--- SELECT characters -------->|
  |<-- { characters[] } ---------|<-- rows ----------------------|
  |                               |                               |
  |--- GET /api/servers --------->|                               |
  |<-- { servers[] } ------------|  (includes live population)   |
  |                               |                               |
  |=== Socket.io connect ========>|  (same host:port as HTTP)    |
  |                               |                               |
  |--- player:join -------------->|  (characterId, token, name)  |
  |                               |--- jwt.verify(token) ------->|
  |                               |--- SELECT char ownership --->|
  |                               |--- SELECT stats, weapon ---->|
  |                               |--- SELECT equipment -------->|
  |                               |--- SELECT learned skills --->|
  |                               |--- socket.join('zone:X') --->|
  |<-- player:joined ------------|  (success, zone, position)   |
  |<-- combat:health_update -----|  (HP/SP for self and others) |
  |<-- player:stats --------------|  (all stats + derived)       |
  |<-- hotbar:alldata ------------|  (delayed 600ms)             |
  |<-- enemy:spawn (x N) --------|  (all alive enemies in zone) |
  |<-- zone:data -----------------|  (warps, Kafra, flags)       |
  |                               |                               |
  |--- player:position ---------->|  (x, y, z every ~66ms)      |
  |                               |--- Redis SET position ------>|
  |                               |--- broadcast player:moved -->| (to zone room)
```

### 1.4 Express REST API Endpoints

| Method | Path | Auth | Purpose |
|--------|------|------|---------|
| `GET` | `/health` | No | Server health check + DB connectivity |
| `POST` | `/api/auth/register` | No | Create account (username, email, password) |
| `POST` | `/api/auth/login` | No | Authenticate, returns JWT (24h expiry) |
| `GET` | `/api/auth/verify` | JWT | Validate token, return user info |
| `GET` | `/api/servers` | No | Server list with live population count |
| `GET` | `/api/characters` | JWT | List characters (max 9, excludes soft-deleted) |
| `POST` | `/api/characters` | JWT | Create character (name, class, hair, gender) |
| `GET` | `/api/characters/:id` | JWT | Get single character details |
| `DELETE` | `/api/characters/:id` | JWT | Soft-delete character (requires password) |
| `PUT` | `/api/characters/:id/position` | JWT | Save position (used by Level Blueprint timer) |
| `GET` | `/api/test` | No | Health check (legacy) |

**Rate limiting**: `express-rate-limit` is configured globally. Registration and login have stricter limits than general API calls.

**JWT tokens**: Signed with `JWT_SECRET` from `.env`, 24-hour expiry. Payload contains `user_id` and `username`. The `authenticateToken` middleware validates and attaches `req.user` for all protected routes.

### 1.5 Data Module Architecture

The server imports pre-built data modules at startup rather than querying a database for static game data:

| Module | File | Records | Purpose |
|--------|------|---------|---------|
| `ro_monster_templates` | `server/src/ro_monster_templates.js` | 509 monsters | Full RO pre-renewal monster database |
| `ro_item_mapping` | `server/src/ro_item_mapping.js` | 126 RO items | Item name-to-ID mapping + extra drops config |
| `ro_exp_tables` | `server/src/ro_exp_tables.js` | 99 levels | Base/job EXP tables, class tier config |
| `ro_skill_data` | `server/src/ro_skill_data.js` | All skills | Skill definitions, prerequisites, SP costs |
| `ro_monster_ai_codes` | `server/src/ro_monster_ai_codes.js` | 1,004 mappings | Monster ID to rAthena AI type code |
| `ro_zone_data` | `server/src/ro_zone_data.js` | 4+ zones | Zone registry, warps, spawns, Kafra NPCs |
| `ro_damage_formulas` | `server/src/ro_damage_formulas.js` | - | Full RO pre-renewal damage calculation |

---

## 2. Socket.io Event Architecture

### 2.1 Event Naming Convention

All events follow the pattern `domain:action`. The colon is the namespace separator. Events flow in two directions:

- **C->S** (Client to Server): Client requests an action
- **S->C** (Server to Client): Server pushes state updates
- **S->Zone** (Server to Zone Room): Server broadcasts to all clients in a zone

### 2.2 Complete Event Reference

#### Player Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `player:join` | C->S | `{ characterId, token, characterName }` | Authenticate and enter world |
| `player:joined` | S->C | `{ success, zuzucoin, zone, levelName, displayName, x, y, z }` | Join confirmation with spawn data |
| `player:join_error` | S->C | `{ error }` | JWT validation or ownership check failed |
| `player:position` | C->S | `{ characterId, x, y, z }` | Client position update (~15 Hz) |
| `player:moved` | S->Zone | `{ characterId, characterName, x, y, z, health, maxHealth, timestamp }` | Broadcast position to zone |
| `player:left` | S->Zone | `{ characterId, characterName }` | Player disconnected or changed zone |
| `player:request_stats` | C->S | (none) | Request full stat refresh |
| `player:stats` | S->C | Full stats payload (see section 2.3) | All base, derived, and combat stats |
| `player:allocate_stat` | C->S | `{ stat }` | Spend stat points (str/agi/vit/int/dex/luk) |
| `player:teleport` | S->C | `{ x, y, z }` | Server-initiated position change |

#### Combat Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `combat:attack` | C->S | `{ targetCharacterId? \| targetEnemyId? }` | Start auto-attack on target |
| `combat:stop_attack` | C->S | (none) | Stop auto-attacking |
| `combat:auto_attack_started` | S->C | `{ targetId, isEnemy, targetName }` | Confirms auto-attack loop started |
| `combat:auto_attack_stopped` | S->C | `{ reason }` | Auto-attack ended (target dead, etc.) |
| `combat:damage` | S->Zone | `{ attackerId, attackerName, targetId, targetName, isEnemy, damage, isCritical, isMiss, hitType, element, targetHealth, targetMaxHealth, attackerX/Y/Z, targetX/Y/Z, timestamp }` | Auto-attack damage result (miss/hit/crit) |
| `combat:health_update` | S->C/Zone | `{ characterId, health, maxHealth, mana, maxMana, healAmount? }` | HP/SP synchronization |
| `combat:out_of_range` | S->C | `{ targetId, isEnemy, targetX/Y/Z, distance, requiredRange }` | Target too far, client should pathfind closer |
| `combat:target_lost` | S->C | `{ reason, isEnemy }` | Target invalid (died, disconnected, PvP disabled) |
| `combat:death` | S->Zone | `{ killedId, killedName, killerId, killerName, isEnemy, targetHealth, targetMaxHealth, timestamp }` | Player death broadcast |
| `combat:error` | S->C | `{ message }` | Combat validation failure |
| `combat:respawn` | C->S | `{ characterId }` | Request respawn after death |

**Important distinction**: `combat:damage` is emitted ONLY for auto-attacks. Skill damage uses `skill:effect_damage`. C++ subsystems (DamageNumberSubsystem, BasicInfoSubsystem, WorldHealthBarSubsystem) listen to BOTH events.

#### Skill Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `skill:data` | C->S / S->C | Request: (none). Response: `{ skills, learnedSkills, skillPoints, jobClass }` | Load available/learned skills |
| `skill:use` | C->S | `{ skillId, targetId?, isEnemy?, groundX/Y/Z? }` | Request skill execution |
| `skill:used` | S->C | `{ skillId, skillName, level, spCost, remainingMana, maxMana, hits?, totalDamage?, enemiesHit? }` | Skill successfully executed |
| `skill:cast_start` | S->Zone | `{ casterId, casterName, skillId, skillName, castTime, targetId?, isEnemy? }` | Cast bar begins (broadcast to zone for visual) |
| `skill:cast_complete` | S->Zone | `{ casterId, skillId }` | Cast finished, skill executing |
| `skill:cast_interrupted` | S->C | `{ skillId, reason }` | Cast cancelled (moved, took damage) |
| `skill:cast_interrupted_broadcast` | S->Zone | `{ casterId, skillId }` | Cast interruption broadcast |
| `skill:cast_failed` | S->C | `{ skillId, reason }` | Post-cast validation failed |
| `skill:effect_damage` | S->Zone | `{ attackerId, attackerName, targetId, targetName, isEnemy, skillId, skillName, element, damage, isCritical, isMiss, hitType, targetX/Y/Z, targetHealth, targetMaxHealth, hitIndex?, totalHits?, timestamp }` | Skill damage per hit (multi-hit sends multiple) |
| `skill:buff_applied` | S->C/Zone | `{ targetId, isEnemy, buffName, skillId, duration, ... }` | Buff/debuff applied (Provoke, Frost Diver, etc.) |
| `skill:buff_removed` | S->C/Zone | `{ targetId, isEnemy?, buffName, skillId, reason }` | Buff expired or removed |
| `skill:ground_effect_placed` | S->Zone | `{ effectId, type, casterId, x, y, z, radius, duration, ... }` | Ground AoE created (Fire Wall, Safety Wall) |
| `skill:ground_effect_blocked` | S->Zone | `{ effectId, type, hitsRemaining, targetId, targetName, isEnemy }` | Safety Wall blocked an attack |
| `skill:ground_effect_removed` | S->Zone | `{ effectId, type, reason }` | Ground effect expired or charges depleted |
| `skill:aoe_damage` | S->Zone | `{ skillId, skillName, casterId, casterName, groundX/Y/Z, stormCenter?, ... }` | AoE effect broadcast (Thunderstorm) |
| `skill:cooldown_started` | S->C | `{ skillId, cooldownMs }` | Per-skill cooldown started |
| `skill:acd_started` | S->C | `{ afterCastDelay }` | Global skill lockout (After-Cast Delay) |
| `skill:learn` | C->S | `{ skillId }` | Learn or level up a skill |
| `skill:learned` | S->C | `{ skillId, skillName, level, skillPoints }` | Skill learned successfully |
| `skill:refresh` | S->C | `{ skillPoints }` | Skill points updated |
| `skill:reset` | C->S | (none) | Reset all skill points |
| `skill:reset_complete` | S->C | `{ skillPoints, learnedSkills }` | Skills reset successfully |
| `skill:error` | S->C | `{ message }` | Skill validation failure |

**Skill targeting types** (determined by `skill.targetType`):

| Target Type | Client Sends | Server Behavior |
|-------------|-------------|-----------------|
| `self` | `{ skillId }` | No target needed, applies to caster |
| `single` | `{ skillId, targetId, isEnemy }` | Validates target exists, range check, line-of-sight |
| `aoe` / `ground` | `{ skillId, groundX, groundY, groundZ }` | Ground-targeted, hits all enemies in radius |

**Cast time system** (RO pre-renewal formula):

```
ActualCastTime = BaseCastTime * (1 - DEX / 150)
DEX >= 150 = instant cast
```

Cast state stored in `activeCasts` Map: `characterId -> { skillId, targetId, isEnemy, learnedLevel, levelData, skill, castStartTime, castEndTime, actualCastTime, socketId, spCost, groundX/Y/Z, hasGroundPos, casterName }`.

#### Inventory Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `inventory:load` | C->S | (none) | Request full inventory sync |
| `inventory:data` | S->C | `{ items[], zuzucoin }` | Full inventory state (sent after any change) |
| `inventory:use` | C->S | `{ inventoryId }` | Use consumable item |
| `inventory:used` | S->C | `{ itemName, healAmount?, healType? }` | Item consumed successfully |
| `inventory:equip` | C->S | `{ inventoryId }` | Equip item |
| `inventory:equipped` | S->C | `{ weaponAtk?, weaponRange?, weaponType?, equipSlot }` | Equipment state changed |
| `inventory:drop` | C->S | `{ inventoryId, quantity? }` | Drop/discard item |
| `inventory:dropped` | S->C | `{ itemName, quantity }` | Item dropped successfully |
| `inventory:move` | C->S | `{ inventoryId, targetSlot }` | Reorder inventory slot |
| `inventory:error` | S->C | `{ message }` | Inventory validation failure |

#### Enemy Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `enemy:spawn` | S->Zone | `{ enemyId, templateId, name, level, health, maxHealth, x, y, z, monsterClass?, size?, race?, element? }` | New enemy appears (spawn or respawn) |
| `enemy:move` | S->Zone | `{ enemyId, x, y, z, isMoving, knockback? }` | Enemy position update (wander, chase, knockback) |
| `enemy:attack` | S->Zone | `{ enemyId, targetId, attackMotion, damage?, targetHealth?, targetMaxHealth? }` | Enemy attacks player |
| `enemy:health_update` | S->Zone | `{ enemyId, health, maxHealth, inCombat }` | Enemy HP sync (for health bars) |
| `enemy:death` | S->Zone | `{ enemyId, enemyName, killerId, killerName, isEnemy, isDead, baseExp, jobExp, timestamp }` | Enemy killed |

#### Chat Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `chat:message` | C->S | `{ channel, message }` | Send chat message |
| `chat:receive` | S->C/All | `{ type, channel, senderId, senderName, message, timestamp }` | Receive chat message |

**Channels**: `GLOBAL` (all players, via `io.emit`), `ZONE` (same zone, via `broadcastToZone`), `COMBAT` (kill notifications), `SYSTEM` (level ups, announcements).

#### EXP and Leveling Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `exp:gain` | S->C | `{ characterId, baseExpGained, jobExpGained, enemyName, enemyLevel, exp, baseLevelUps[], jobLevelUps[] }` | EXP awarded from kill |
| `exp:level_up` | S->C/Zone | `{ characterId, characterName, baseLevelUps[], jobLevelUps[], totalStatPoints, totalSkillPoints, exp }` | Level up notification |

#### Job Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `job:change` | C->S | `{ targetClass }` | Request job change |
| `job:changed` | S->C | `{ oldClass, newClass, jobLevel, skillPoints }` | Job change successful |
| `job:error` | S->C | `{ message }` | Job change validation failed |

#### Zone Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `zone:warp` | C->S | `{ warpId }` | Request zone transition via warp portal |
| `zone:change` | S->C | `{ zone, levelName, displayName, spawnX/Y/Z }` | Server instructs client to load new level |
| `zone:ready` | C->S | `{ characterId }` | Client finished loading new zone |
| `zone:data` | S->C | `{ zone, displayName, levelName, flags, warps, kafraNpcs }` | Zone metadata (warps, NPCs, flags) |
| `zone:error` | S->C | `{ message }` | Zone transition failed |

**Zone transition flow**:

```
1. Client overlaps WarpPortal actor
2. ZoneTransitionSubsystem calls RequestWarp(WarpId)
3. Client emits zone:warp { warpId }
4. Server validates: warp exists, player near warp, destination zone exists
5. Server: socket.leave('zone:oldZone'), socket.join('zone:newZone')
6. Server emits zone:change { zone, levelName, spawnX/Y/Z }
7. Client calls UGameplayStatics::OpenLevel(levelName)
8. Level Blueprint spawns pawn at PendingSpawnLocation
9. Client emits zone:ready { characterId }
10. Server sends: enemies, other players, zone:data for new zone
```

#### Kafra NPC Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `kafra:open` | C->S | `{ kafraId }` | Request Kafra NPC dialog |
| `kafra:data` | S->C | `{ kafraId, services, destinations[] }` | Kafra services available |
| `kafra:save` | C->S | (none) | Save current location as respawn point |
| `kafra:saved` | S->C | `{ saveMap, saveX, saveY, saveZ }` | Save point confirmed |
| `kafra:teleport` | C->S | `{ destinationIndex }` | Teleport to destination |
| `kafra:teleported` | S->C | `{ destination, cost, newZuzucoin }` | Teleport successful |
| `kafra:error` | S->C | `{ message }` | Kafra service failed |

#### Shop Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `shop:open` | C->S | `{ shopId }` | Request NPC shop catalog |
| `shop:data` | S->C | `{ shopId, shopName, items[], playerZuzucoin }` | Shop inventory |
| `shop:buy` | C->S | `{ itemId, quantity }` | Buy single item |
| `shop:buy_batch` | C->S | `{ shopId, cart[] }` | Buy multiple items |
| `shop:bought` | S->C | `{ itemId?, items[]?, totalCost, newZuzucoin }` | Purchase successful |
| `shop:sell` | C->S | `{ inventoryId, quantity }` | Sell single item |
| `shop:sell_batch` | C->S | `{ cart[] }` | Sell multiple items |
| `shop:sold` | S->C | `{ inventoryId?, items[]?, totalRevenue, newZuzucoin }` | Sale successful |
| `shop:error` | S->C | `{ code?, message }` | Shop operation failed |

#### Hotbar Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `hotbar:request` | C->S | (none) | Request hotbar data |
| `hotbar:alldata` | S->C | `{ slots[] }` | Full hotbar state (4 rows x 9 slots) |
| `hotbar:save` | C->S | `{ slotIndex, rowIndex, inventoryId, itemId, itemName }` | Save item to hotbar slot |
| `hotbar:save_skill` | C->S | `{ slotIndex, rowIndex, skillId, skillName, zeroBased? }` | Save skill to hotbar slot |
| `hotbar:clear` | C->S | `{ slotIndex, rowIndex }` | Clear hotbar slot |

#### Loot Events

| Event | Direction | Payload | Purpose |
|-------|-----------|---------|---------|
| `loot:drop` | S->C | `{ enemyId, enemyName, items[] }` | Items dropped by killed enemy |

### 2.3 Player Stats Payload (Full)

The `player:stats` event sends a comprehensive payload built by `buildFullStatsPayload()`:

```javascript
{
  characterId,
  stats: {
    // Base stats
    str, agi, vit, int, dex, luk, level, statPoints,
    // Stat allocation costs (for UI)
    strCost, agiCost, vitCost, intCost, dexCost, lukCost,
    // Derived combat stats
    statusATK, equipATK, weaponATK,
    statusMATK, minMATK, maxMATK,
    hit, flee, criticalRate, perfectDodge,
    softDEF, hardDEF, softMDEF,
    aspd, attackInterval,
    maxHP, maxSP,
    // Equipment bonuses
    equipBonuses: { str, agi, vit, int, dex, luk, maxHp, maxSp, hit, flee, critical }
  },
  // EXP and leveling
  exp: {
    baseLevel, jobLevel, jobClass,
    baseExp, baseExpNext, baseExpPercent,
    jobExp, jobExpNext, jobExpPercent,
    skillPoints
  },
  // Vitals
  health, maxHealth, mana, maxMana,
  attackRange
}
```

### 2.4 Future Events (Not Yet Implemented)

These events are planned for future development phases:

#### Party Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `party:create` | C->S | Create party (name, settings) |
| `party:invite` | C->S | Invite player to party |
| `party:join` | C->S | Accept party invitation |
| `party:leave` | C->S | Leave current party |
| `party:kick` | C->S | Kick member (leader only) |
| `party:update` | S->C | Member list or settings changed |
| `party:leader_change` | S->C | Party leader transferred |
| `party:exp_share` | S->C | EXP distribution from party kill |

#### Guild Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `guild:create` | C->S | Create guild (name, emblem) |
| `guild:invite` | C->S | Invite player to guild |
| `guild:join` | C->S | Accept guild invitation |
| `guild:leave` | C->S | Leave guild |
| `guild:kick` | C->S | Remove member (officer+) |
| `guild:update` | S->C | Member list or settings changed |
| `guild:notice_update` | S->C | Guild notice/announcement changed |
| `guild:storage` | C->S | Access guild storage |
| `guild:skill_use` | C->S | Use guild skill |
| `guild:alliance` | C->S | Alliance request/accept |
| `guild:enemy` | C->S | Declare guild war |

#### Trade Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `trade:request` | C->S | Initiate trade with player |
| `trade:accept` | C->S | Accept trade request |
| `trade:cancel` | C->S | Cancel trade |
| `trade:add_item` | C->S | Add item to trade window |
| `trade:remove_item` | C->S | Remove item from trade window |
| `trade:set_zeny` | C->S | Set zeny amount in trade |
| `trade:confirm` | C->S | Lock in trade offer |
| `trade:complete` | S->C | Both confirmed, items exchanged |

#### NPC Dialog Events

| Event | Direction | Purpose |
|-------|-----------|---------|
| `npc:interact` | C->S | Click NPC to start dialogue |
| `npc:dialogue` | S->C | Dialogue text and options |
| `npc:choice` | C->S | Player selected dialogue option |
| `npc:close` | C->S | Close NPC dialogue |

---

## 3. Position Synchronization

### 3.1 Client-Side Position Sending

The client sends `player:position` events at approximately 15 Hz (every ~66ms). This is controlled by the `BP_SocketManager` Blueprint's position broadcast timer.

**Payload sent by client**:
```json
{
  "characterId": 123,
  "x": 1500.5,
  "y": -2340.2,
  "z": 580.0
}
```

The position includes only XYZ coordinates. Rotation and movement state are derived client-side by other players through interpolation.

### 3.2 Server-Side Position Processing

On receiving `player:position`, the server:

1. **Checks for cast interruption**: If the player has an active cast in `activeCasts`, calculates movement distance from `player.lastX/lastY/lastZ`. If the movement exceeds 5 Unreal units (`MOVE_THRESHOLD`), calls `interruptCast(characterId, 'moved')`. Sub-5-unit jitter is ignored to prevent false interruptions from idle position updates.

2. **Updates last known position**: Sets `player.lastX`, `player.lastY`, `player.lastZ` on the `connectedPlayers` Map entry. These values are used by the enemy AI system for aggro range calculations and chase targeting.

3. **Caches in Redis**: Calls `setPlayerPosition(characterId, x, y, z)` which stores `{ x, y, z, zone, timestamp }` as a JSON string in Redis with a 5-minute TTL. Key format: `player:{characterId}:position`.

4. **Broadcasts to zone**: Calls `broadcastToZoneExcept(socket, zone, 'player:moved', payload)` which emits to all sockets in the `zone:{zoneName}` Socket.io room, excluding the sender.

**Broadcast payload**:
```json
{
  "characterId": 123,
  "characterName": "PlayerOne",
  "x": 1500.5,
  "y": -2340.2,
  "z": 580.0,
  "health": 450,
  "maxHealth": 500,
  "timestamp": 1709823456789
}
```

### 3.3 Client-Side Interpolation

Remote players are rendered using `UOtherCharacterMovementComponent` (C++ class extending `UCharacterMovementComponent`) and Blueprint logic in `BP_OtherPlayerCharacter`.

**Interpolation strategy**: The Blueprint receives `player:moved` events and smoothly interpolates the remote character's position toward the target. This avoids teleporting characters to new positions every 66ms, which would look jerky.

The interpolation system:
- Stores the most recent target position received from the server
- On each Tick, moves the character toward the target at the character's movement speed
- If the distance to target exceeds a threshold (e.g., 500 UE units), teleports directly (handles lag spikes)
- Calculates facing direction from movement vector for rotation

### 3.4 Zone-Scoped Broadcasting

Position updates are only broadcast to players in the same zone, implemented through Socket.io rooms:

```javascript
function broadcastToZone(zone, event, data) {
    io.to('zone:' + zone).emit(event, data);
}

function broadcastToZoneExcept(socket, zone, event, data) {
    socket.to('zone:' + zone).emit(event, data);
}
```

When a player changes zones:
1. `socket.leave('zone:' + oldZone)` -- stop receiving events from old zone
2. `socket.join('zone:' + newZone)` -- start receiving events from new zone
3. `player:left` broadcast to old zone
4. `player:moved` broadcast to new zone (initial position)

### 3.5 Position Persistence

Player positions are saved to PostgreSQL through multiple mechanisms:

| Trigger | Frequency | Method |
|---------|-----------|--------|
| Level Blueprint timer | Every 5 seconds | `PUT /api/characters/:id/position` (REST API) |
| Server periodic save | Every 60 seconds | `UPDATE characters SET x, y, z` (server-side timer) |
| Player disconnect | Once | `UPDATE characters SET zone_name, x, y, z` (in `disconnect` handler) |
| Zone transition | Once | Position saved before leaving old zone |

Redis position cache is the authoritative real-time position (for combat range checks), while PostgreSQL is the durable store (survives server restarts).

---

## 4. Combat Tick Loop

### 4.1 Auto-Attack Processing (50ms Tick)

The combat tick loop runs every 50ms (`COMBAT.COMBAT_TICK_MS = 50`), processing all active auto-attack states. It is implemented as a `setInterval` at the bottom of `index.js`.

**Auto-attack state tracking**: `autoAttackState` is a `Map<attackerCharId, { targetCharId, isEnemy, startTime }>`.

**Per-tick processing order**:

```
1. Cast Completion Check
   - Iterate activeCasts Map
   - If now >= cast.castEndTime, delete from activeCasts and call executeCastComplete()

2. Auto-Attack Processing (for each entry in autoAttackState)
   a. Validate attacker exists and is alive
   b. Check CC status (frozen/stoned skip tick)
   c. Branch: Enemy target or Player target

3. Enemy Target Path:
   a. Validate enemy exists and is alive
   b. ASPD timing check: if (now - lastAttackTime < attackInterval) skip
   c. Range check: get attacker position from Redis, calculate distance
   d. If out of range: emit combat:out_of_range (client pathfinds closer)
   e. Safety Wall check: if target in Safety Wall, consume charge, skip damage
   f. Calculate damage using full RO formula (roPhysicalDamage)
   g. Apply damage to enemy.health
   h. Set enemy aggro (triggers AI chase/assist)
   i. Broadcast combat:damage to zone
   j. Fire element breaks Frozen status
   k. If enemy.health <= 0: process death, award EXP, roll loot, schedule respawn

4. Player Target Path (PvP):
   a. If PVP_ENABLED is false: clear auto-attack, emit target_lost
   b. Same flow as enemy but with player stats
   c. Damage interrupts active casts on the target
   d. On death: broadcast combat:death, save HP to DB
```

### 4.2 ASPD and Attack Interval

ASPD (Attack Speed) ranges from 0 to 199, derived from AGI, DEX, and weapon type. The conversion to milliseconds between attacks:

```javascript
function getAttackIntervalMs(aspd) {
    if (aspd <= 195) {
        // Linear formula up to hard cap
        return (200 - aspd) * 50;
        // ASPD 175 = 1250ms, ASPD 185 = 750ms, ASPD 195 = 250ms
    } else {
        // Diminishing returns above 195 (exponential decay)
        // Floor ~217ms (~4.6 attacks/sec absolute max at ASPD 199)
        const excessAspd = Math.min(aspd - 195, 9);
        const decayFactor = Math.exp(-excessAspd * 0.35);
        const maxBonus = 130;
        const actualBonus = Math.floor(maxBonus * (1 - decayFactor));
        return Math.max(217, 250 - actualBonus);
    }
}
```

### 4.3 RO Damage Formula

Physical damage uses the full RO pre-renewal formula from `ro_damage_formulas.js`:

```
1. HIT/FLEE check: hitRate = 80 + attacker.hit - target.flee (capped 5-95%)
2. Critical check: critRate = attacker.luk * 0.3 + bonusCrit (ignores FLEE)
3. Perfect Dodge: target.luk * 0.1 (avoids even crits)
4. Base damage: statusATK + weaponATK +/- variance
5. Size penalty: applied based on weapon type vs target size (S/M/L)
6. Element modifier: 10x10 element table (attacker element vs target element)
7. DEF reduction: soft DEF (percentage) then hard DEF (flat subtraction)
8. Skill multiplier: for skills, damage * skillMultiplier%
9. Buff modifiers: Provoke ATK increase, DEF reduction
```

Magic damage uses a separate formula (`roMagicalDamage`) with MATK, MDEF, and element interactions.

### 4.4 Enemy AI Tick (200ms)

Enemy AI runs on a separate `setInterval` at 200ms (5 ticks/second), which is the standard rAthena AI tick rate. It processes only enemies in zones that have at least one player (`getActiveZones()`).

**State machine**: `IDLE -> CHASE -> ATTACK -> DEAD`

```
IDLE state:
  - If modeFlags.aggressive: scan for players within aggroRange every 500ms
  - If modeFlags.assist: join combat when same-type mob is attacked within 550 UU
  - If !modeFlags.noRandomWalk: wander randomly (100-300 UU offset, clamped to wanderRadius)
  - Move at 60% of walk speed while wandering

CHASE state:
  - Move toward targetPlayerId at 100% walk speed
  - If target within attackRange: transition to ATTACK
  - If target beyond chaseRange + 200: give up, transition to IDLE
  - If target disconnects: pick next target from inCombatWith, or IDLE

ATTACK state:
  - Check attackMotion cooldown (per-monster attack animation time)
  - Calculate damage using RO formula (enemy stats vs player stats)
  - Emit enemy:attack + combat:health_update to zone
  - If target dies: pick next target or IDLE
  - If target moves out of range: transition to CHASE

DEAD state:
  - No AI processing
  - setTimeout schedules respawn (enemy.respawnMs from template)
  - On respawn: reset HP, position, AI state, broadcast enemy:spawn
```

**Mode flags** (parsed from rAthena AI codes): Each monster has 18 boolean flags derived from a hex bitmask that control its behavior (aggressive, assist, changeTargetMelee, changeTargetChase, randomTarget, targetWeak, castSensorIdle, knockbackImmune, statusImmune, etc.).

### 4.5 Regeneration Tick Loops

Three separate `setInterval` loops handle RO Classic natural regeneration:

| Loop | Interval | Formula | Condition |
|------|----------|---------|-----------|
| HP Regen | 6,000ms | `max(1, floor(MaxHP/200)) + floor(VIT/5)` | Not dead, HP < MaxHP |
| SP Regen | 8,000ms | `1 + floor(MaxSP/100) + floor(INT/6)` + bonus if INT >= 120 | Not dead, SP < MaxSP |
| Skill Regen | 10,000ms | HP Recovery: `Lv*5 + Lv*MaxHP/500`; SP Recovery: `Lv*3 + Lv*MaxSP/500` | Has passive skills learned |

### 4.6 Buff Expiry Tick (1s)

Every 1 second, the server iterates all connected players and all alive enemies, calling `expireBuffs()` to remove buffs whose `expiresAt` timestamp has passed. Expired buffs trigger `skill:buff_removed` events.

Special handling:
- **Stone Curse**: Drains 1% MaxHP every 5 seconds while petrified (after a 3-second half-stone phase). Does not kill (minimum 1 HP).
- **Frozen**: Target becomes Water Lv1 element (fire attacks deal bonus damage and break the effect).

### 4.7 Ground Effects Tick (500ms)

Every 500ms, the server processes active ground effects:

- **Fire Wall**: Enemies inside the wall radius take 50% MATK fire damage per tick. Non-boss enemies are knocked back outside the wall radius. Boss-type enemies are knockback-immune and consume charges rapidly. Per-target 300ms cooldown prevents double-hits.
- **Safety Wall**: Charges are consumed when melee attacks hit a target inside the wall area (processed in the combat tick loop, not here).
- **Expired effects**: Removed and `skill:ground_effect_removed` broadcast to zone.

---

## 5. Redis Usage

### 5.1 Current Implementation

Redis serves as a fast in-memory cache layer between the game server and PostgreSQL. The current implementation uses a single Redis client:

```javascript
const redisClient = redis.createClient({
    host: process.env.REDIS_HOST || 'localhost',
    port: process.env.REDIS_PORT || 6379
});
```

### 5.2 Player Position Cache

**Primary use case**: Player positions are cached in Redis to avoid hitting PostgreSQL on every position update (15 Hz per player = up to 15 writes/sec/player).

```javascript
async function setPlayerPosition(characterId, x, y, z, zone = 'default') {
    const key = `player:${characterId}:position`;
    const position = JSON.stringify({ x, y, z, zone, timestamp: Date.now() });
    await redisClient.setEx(key, 300, position); // 5-minute TTL
}

async function getPlayerPosition(characterId) {
    const key = `player:${characterId}:position`;
    const data = await redisClient.get(key);
    return data ? JSON.parse(data) : null;
}
```

**Key pattern**: `player:{characterId}:position`
**TTL**: 300 seconds (5 minutes) -- auto-expires if player disconnects without cleanup
**Access pattern**: Written every ~66ms per player, read by combat tick for range checks

### 5.3 In-Memory State (Not Yet in Redis)

Several state structures currently live in-process JavaScript Maps/Sets that would benefit from Redis migration for horizontal scaling:

| State | Current Storage | Redis Migration Path |
|-------|----------------|---------------------|
| `connectedPlayers` | `Map<charId, PlayerObj>` | Redis Hash per player + Redis Set for zone membership |
| `autoAttackState` | `Map<attackerId, TargetState>` | Redis Hash `combat:autoattack:{charId}` |
| `activeCasts` | `Map<charId, CastState>` | Redis Hash `cast:active:{charId}` with TTL |
| `enemies` | `Map<enemyId, EnemyObj>` | Redis Hash per enemy (zone-scoped key) |
| `activeGroundEffects` | `Map<effectId, EffectObj>` | Redis Hash `zone:{zone}:ground_effects` |
| `afterCastDelayEnd` | `Map<charId, timestamp>` | Redis key with TTL matching ACD duration |

### 5.4 Planned Redis Expansion

For future scaling, Redis would handle:

| Purpose | Key Pattern | Type | TTL |
|---------|-------------|------|-----|
| Session management | `session:{socketId}` | Hash | 24h (matches JWT) |
| Rate limiting | `ratelimit:{socketId}:{event}` | Counter | 1-60s |
| Zone player lists | `zone:{zoneName}:players` | Set | -- |
| Party online members | `party:{partyId}:online` | Set | -- |
| Guild online members | `guild:{guildId}:online` | Set | -- |
| Buff timers | `buff:{targetType}:{targetId}` | List | Max buff duration |
| Enemy HP cache | `enemy:{zone}:{enemyId}:hp` | String | Until respawn |
| Cooldown tracking | `cooldown:{charId}:{skillId}` | String | Cooldown duration |
| Socket.io adapter | (built-in) | Pub/Sub | -- |

### 5.5 Redis as Socket.io Adapter

For horizontal scaling (multiple Node.js processes), the `@socket.io/redis-adapter` package enables Socket.io to broadcast events across process boundaries:

```javascript
// Future: Multi-process setup
const { createAdapter } = require('@socket.io/redis-adapter');
const pubClient = redis.createClient({ url: REDIS_URL });
const subClient = pubClient.duplicate();
io.adapter(createAdapter(pubClient, subClient));
```

This allows `broadcastToZone()` to work transparently across multiple server instances without code changes.

---

## 6. Database Interaction Patterns

### 6.1 Connection Pooling

PostgreSQL connections use `pg.Pool` for connection pooling:

```javascript
const pool = new Pool({
    host: process.env.DB_HOST,
    port: process.env.DB_PORT,
    database: process.env.DB_NAME,
    user: process.env.DB_USER,
    password: process.env.DB_PASSWORD,
});
```

Default pool settings: 10 connections max, 30-second idle timeout. All queries use `pool.query()` which automatically acquires and releases connections.

### 6.2 Write Frequency Strategy

**Not every state change triggers a database write.** The server batches and throttles writes to avoid overwhelming PostgreSQL:

| Data | Write Trigger | Frequency | Method |
|------|---------------|-----------|--------|
| Position (x, y, z) | Level Blueprint REST timer | Every 5 seconds | `PUT /api/characters/:id/position` |
| Position (x, y, z) | Server periodic save | Every 60 seconds | Direct `pool.query()` |
| Position + zone | Disconnect handler | Once | Direct `pool.query()` |
| HP / Mana | Player death, disconnect | On event | `savePlayerHealthToDB()` |
| Stats | Stat allocation, level up | On change | Direct `pool.query()` |
| EXP data | Enemy kill (async) | On kill | `saveExpDataToDB()` |
| Stats + EXP | Disconnect | Once | Direct `pool.query()` |
| Inventory | Item pickup, use, equip, drop | On change | Direct `pool.query()` |
| Hotbar | Slot save/clear | On change | `UPSERT` into `character_hotbar` |
| Learned skills | Skill learn/reset | On change | `INSERT/DELETE` on `character_skills` |

### 6.3 Immediate vs Deferred Saves

**Immediate saves** (must not lose data):
- Item acquisition (monster drops going to inventory)
- Item consumption (HP potion used, quantity decremented)
- Equipment changes (weapon equipped, stats recalculated)
- Stat point allocation (irreversible in RO Classic)
- Skill learning (skill point spent)
- Level up (stat/skill points awarded)
- Zeny changes (NPC buy/sell)
- Character creation/deletion

**Deferred saves** (tolerable data loss on crash):
- Position updates (cached in Redis, persisted every 5-60s)
- HP/Mana regeneration ticks (saved on disconnect)
- Combat health changes during auto-attack (saved on death or disconnect)

### 6.4 Transaction Safety

Currently, multi-table operations use sequential queries rather than explicit transactions. For future trade and party systems, proper PostgreSQL transactions will be required:

```javascript
// Future: Trade system with transaction
const client = await pool.connect();
try {
    await client.query('BEGIN');
    await client.query('UPDATE character_inventory SET character_id = $1 WHERE inventory_id = $2', [buyer, itemId]);
    await client.query('UPDATE characters SET zuzucoin = zuzucoin - $1 WHERE character_id = $2', [price, buyer]);
    await client.query('UPDATE characters SET zuzucoin = zuzucoin + $1 WHERE character_id = $2', [price, seller]);
    await client.query('COMMIT');
} catch (e) {
    await client.query('ROLLBACK');
    throw e;
} finally {
    client.release();
}
```

### 6.5 Schema Auto-Migration

The server auto-creates missing stat columns on startup to avoid manual migration for simple schema additions. Complex schema changes use migration files in `database/migrations/`.

### 6.6 Database Tables

| Table | Purpose | Records |
|-------|---------|---------|
| `users` | Account credentials | Grows with registrations |
| `characters` | Character data, stats, position, zone | 1-9 per user |
| `items` | Static item definitions | 148 items |
| `character_inventory` | Per-character item ownership | Variable |
| `character_hotbar` | Hotbar slot assignments | Up to 36 per character |
| `character_skills` | Learned skill levels | Variable |

---

## 7. Scalability Considerations

### 7.1 Zone-Based Player Rooms

Socket.io rooms partition players by zone. Every broadcast operation targets only the relevant zone room:

```javascript
// All zone-scoped broadcasts go through these helpers
function broadcastToZone(zone, event, data) {
    io.to('zone:' + zone).emit(event, data);
}
function broadcastToZoneExcept(socket, zone, event, data) {
    socket.to('zone:' + zone).emit(event, data);
}
```

**Current zones**: `prontera` (town), `prontera_south` (starter field), `prontera_north` (field), `prt_dungeon_01` (dungeon). Zones 4-9 are defined but disabled.

**Broadcast count**: If 100 players are online but only 20 are in `prontera_south`, a position update in that zone only goes to 19 other players (not 99).

### 7.2 Lazy Enemy Spawning

Enemies are only spawned when the first player enters a zone. The `spawnedZones` Set tracks which zones have been initialized:

```javascript
if (!spawnedZones.has(playerZone)) {
    const zoneData = getZone(playerZone);
    if (zoneData && zoneData.enemySpawns.length > 0) {
        for (const spawn of zoneData.enemySpawns) {
            spawnEnemy({ ...spawn, zone: playerZone });
        }
    }
    spawnedZones.add(playerZone);
}
```

### 7.3 Active Zone Optimization

The enemy AI tick only processes zones with at least one connected player:

```javascript
function getActiveZones() {
    const zones = new Set();
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone) zones.add(player.zone);
    }
    return zones;
}
```

Zones with zero players have their AI processing completely skipped, saving significant CPU.

### 7.4 Entity Interest Management

Currently, all entities in a zone are sent to all players in that zone. For very large zones or high player counts, future optimization would implement area-of-interest filtering:

```
// Future: Only send entities within render distance
const INTEREST_RADIUS = 5000; // Unreal units
for (const [eid, enemy] of enemies.entries()) {
    if (enemy.zone !== playerZone) continue;
    const dx = enemy.x - player.lastX;
    const dy = enemy.y - player.lastY;
    if (Math.sqrt(dx*dx + dy*dy) < INTEREST_RADIUS) {
        socket.emit('enemy:spawn', buildEnemyPayload(enemy));
    }
}
```

### 7.5 Connection Limits

Current soft limits:
- Socket.io max connections: Unbounded (limited by OS file descriptors)
- PostgreSQL pool: 10 connections (default `pg.Pool`)
- Redis: Single client connection (handles all operations via pipelining)

For production:
- Set `maxHttpBufferSize` on Socket.io to prevent large payload attacks
- Configure `pingTimeout` and `pingInterval` for connection health monitoring
- Set OS `ulimit -n` to at least 10,000 for file descriptor headroom

### 7.6 Horizontal Scaling Strategy

**Phase 1 (Current)**: Single process, single Redis, single PostgreSQL.

**Phase 2 (500-2000 players)**: Multiple Node.js processes behind a load balancer, connected via Redis adapter for Socket.io. Sticky sessions (by character ID or socket ID) ensure consistent routing.

```
                    ┌─────────────────────┐
                    │   Load Balancer      │
                    │   (sticky sessions)  │
                    └─────┬───────┬────────┘
                          |       |
              ┌───────────┴──┐ ┌──┴───────────┐
              │ Node.js #1   │ │ Node.js #2   │
              │ (zone A, B)  │ │ (zone C, D)  │
              └──────┬───────┘ └──────┬───────┘
                     |                |
              ┌──────┴────────────────┴──────┐
              │         Redis Cluster         │
              │  (Socket.io adapter + cache)  │
              └──────────────┬───────────────┘
                             |
              ┌──────────────┴───────────────┐
              │     PostgreSQL Primary        │
              │     (with read replicas)      │
              └──────────────────────────────┘
```

**Phase 3 (2000+ players)**: Zone-based process sharding where each zone runs in its own Node.js process. A gateway server handles authentication and routes connections to the correct zone process. Inter-zone communication (whispers, guild chat, party sync) goes through Redis Pub/Sub.

---

## 8. Anti-Cheat Measures

### 8.1 Server-Authoritative Validation

The fundamental anti-cheat principle: **the server never trusts client-sent values for authoritative game state.** The client sends intentions; the server validates and executes.

| Client Sends | Server Validates |
|-------------|-----------------|
| `combat:attack { targetId }` | Target exists, is alive, attacker is alive, not CC'd |
| `skill:use { skillId }` | Skill learned, SP sufficient, not on cooldown, no ACD, target valid, in range |
| `inventory:equip { inventoryId }` | Item exists, belongs to player, meets level requirement |
| `player:position { x, y, z }` | (Currently: accepted. Future: speed validation) |
| `stat:allocate { stat }` | Stat < 99, sufficient stat points, valid stat name |
| `zone:warp { warpId }` | Warp exists in current zone, player within proximity |

### 8.2 JWT Token Validation

Every `player:join` event verifies the JWT token and confirms character ownership:

```javascript
socket.on('player:join', async (data) => {
    const rawToken = token.startsWith('Bearer ') ? token.slice(7) : token;
    try {
        const decoded = jwt.verify(rawToken, process.env.JWT_SECRET);
        const ownerCheck = await pool.query(
            'SELECT 1 FROM characters WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE',
            [characterId, decoded.user_id]
        );
        if (ownerCheck.rows.length === 0) {
            socket.emit('player:join_error', { error: 'Character does not belong to this account' });
            return;
        }
    } catch (err) {
        socket.emit('player:join_error', { error: 'Invalid or expired token' });
        return;
    }
});
```

### 8.3 Movement Speed Validation (Planned)

Currently, the server accepts all position updates from the client. Future implementation will validate movement speed:

```javascript
// Future: Movement speed validation
const MAX_SPEED = 600; // UE units per second (walking)
const timeDelta = (now - lastPositionTimestamp) / 1000; // seconds
const distance = Math.sqrt(dx*dx + dy*dy);
const speed = distance / timeDelta;

if (speed > MAX_SPEED * 1.5) { // 50% tolerance for network jitter
    logger.warn(`[ANTICHEAT] ${player.characterName} speed hack: ${speed} UU/s (max ${MAX_SPEED})`);
    // Snap player back to last valid position
    socket.emit('player:teleport', { x: player.lastX, y: player.lastY, z: player.lastZ });
    return;
}
```

### 8.4 Attack Range Validation

The server validates attack range on every auto-attack tick and every skill use:

```javascript
const distance = Math.sqrt(dx * dx + dy * dy);
if (distance > attacker.attackRange) {
    attackerSocket.emit('combat:out_of_range', {
        targetId, isEnemy, targetX, targetY, targetZ,
        distance, requiredRange: Math.max(0, attacker.attackRange - COMBAT.RANGE_TOLERANCE)
    });
    continue; // Skip this attack tick
}
```

`COMBAT.RANGE_TOLERANCE` (50 UE units) provides a small buffer to account for network latency and position update frequency.

### 8.5 Cooldown Enforcement

All cooldowns are tracked server-side. The client cannot bypass them:

```javascript
// Per-skill cooldown
function isSkillOnCooldown(player, skillId) {
    const cd = (player.skillCooldowns || {})[skillId];
    if (!cd) return false;
    return Date.now() < cd;
}

// After-Cast Delay (global skill lockout)
const acdEnd = afterCastDelayEnd.get(characterId);
if (acdEnd && Date.now() < acdEnd) {
    socket.emit('skill:error', { message: `Please wait (${remaining}s)` });
    return;
}
```

### 8.6 Inventory Manipulation Prevention

- Items are created/destroyed only by server-side functions (`addItemToInventory`, `removeItemFromInventory`)
- Client cannot directly set item quantities, IDs, or stats
- Equipment changes recalculate all derived stats server-side
- Equipped items are validated against level requirements
- Duplicate equip prevention (can't equip same item twice)
- Equipped items cannot be sold without unequipping first

### 8.7 Rate Limiting

**REST API**: `express-rate-limit` limits request frequency per IP.

**Socket.io events**: Currently handled by the natural throttling of game logic (ASPD limits attacks, cooldowns limit skills). Future: explicit rate limiting per event type using Redis counters:

```javascript
// Future: Per-event rate limiting
const RATE_LIMITS = {
    'player:position': { max: 20, window: 1000 },  // 20/sec
    'chat:message':    { max: 5,  window: 5000 },   // 5/5sec
    'skill:use':       { max: 10, window: 1000 },   // 10/sec
    'combat:attack':   { max: 5,  window: 1000 },   // 5/sec
};
```

### 8.8 Input Validation

All client inputs are validated:
- Character names: 2-24 characters, alphanumeric + spaces only
- Stats: Range [1, 99], valid stat names, sufficient points
- Skill IDs: Must exist in `SKILL_MAP`
- Item IDs: Must exist in item definitions
- Zone warp IDs: Must exist in current zone's warp registry
- Numeric fields: `parseInt()` with NaN checks

---

## 9. UE5 Client Networking

### 9.1 HTTP Manager (REST API Client)

`UMMOHttpManager` is a `BlueprintFunctionLibrary` that wraps Unreal's `FHttpModule` for REST API calls:

```cpp
// MMOHttpManager.h (Blueprint-callable static functions)
UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void Login(const FString& Username, const FString& Password);

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void Register(const FString& Username, const FString& Email, const FString& Password);

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void GetServers();

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void GetCharacters();

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void CreateCharacter(const FString& Name, const FString& Gender, int32 HairStyle, int32 HairColor);

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void DeleteCharacter(int32 CharacterId, const FString& Password);

UFUNCTION(BlueprintCallable, Category = "MMO HTTP")
static void SavePosition(int32 CharacterId, float X, float Y, float Z);
```

All HTTP calls use the `ServerBaseUrl` from `UMMOGameInstance` (default `http://localhost:3001`). Responses are deserialized from JSON and broadcast through `UMMOGameInstance` delegates (`OnLoginSuccess`, `OnLoginFailedWithReason`, `OnCharacterListReceived`, etc.).

### 9.2 Socket.io Connection (Blueprint)

Socket.io communication is handled by the SocketIOClient plugin for UE5, managed through a `BP_SocketManager` Blueprint actor:

- **Connection**: Created in the Level Blueprint after the game level loads
- **Authentication**: Sends `player:join` with the JWT token from `UMMOGameInstance::AuthToken`
- **Event binding**: Each subsystem binds to its relevant events through the Socket Manager

### 9.3 Subsystem Architecture

The client uses `UWorldSubsystem` subclasses to organize networking by domain. Each subsystem registers its Socket.io event handlers and manages its own Slate widget:

| Subsystem | Socket Events Handled | Widget | Z-Order |
|-----------|----------------------|--------|---------|
| `BasicInfoSubsystem` | `combat:health_update`, `combat:damage`, `skill:effect_damage` | `SBasicInfoWidget` | 10 |
| `CombatStatsSubsystem` | `player:stats` | `SCombatStatsWidget` | 12 |
| `InventorySubsystem` | `inventory:data`, `inventory:equipped`, `inventory:dropped`, `inventory:error` | `SInventoryWidget` | 14 |
| `EquipmentSubsystem` | `inventory:data` | `SEquipmentWidget` | 15 |
| `HotbarSubsystem` | `hotbar:alldata` | `SHotbarRowWidget` x4 | 16 |
| `SkillTreeSubsystem` | `skill:data`, `skill:learned`, `skill:refresh` | `SSkillTreeWidget` | 20 |
| `DamageNumberSubsystem` | `combat:damage`, `skill:effect_damage` | `SDamageNumberOverlay` | 20 |
| `CastBarSubsystem` | `skill:cast_start`, `skill:cast_complete`, `skill:cast_interrupted` | `SCastBarOverlay` | 25 |
| `WorldHealthBarSubsystem` | `combat:health_update`, `combat:damage`, `skill:effect_damage` | `SWorldHealthBarOverlay` | 8 |
| `ZoneTransitionSubsystem` | `zone:change`, `zone:error`, `player:teleport` | `SLoadingOverlayWidget` | 50 |
| `KafraSubsystem` | `kafra:data`, `kafra:saved`, `kafra:teleported`, `kafra:error` | `SKafraWidget` | 19 |
| `SkillVFXSubsystem` | `skill:cast_start`, `skill:effect_damage`, `skill:buff_applied`, `skill:buff_removed`, `skill:aoe_damage` | (VFX components) | N/A |
| `LoginFlowSubsystem` | (REST only, no socket events) | Login flow widgets | 5 |

### 9.4 Game Instance State Persistence

`UMMOGameInstance` survives level transitions (unlike PlayerController or GameMode). It stores:

```cpp
// Authentication
FString AuthToken;
FString Username;
int32 UserId;
bool bIsLoggedIn;

// Character selection
TArray<FCharacterData> CharacterList;
FCharacterData SelectedCharacter;

// Server selection
FServerInfo SelectedServer;
FString ServerBaseUrl;

// Zone transition state
FString CurrentZoneName;
FString PendingZoneName;
FString PendingLevelName;
FVector PendingSpawnLocation;
bool bIsZoneTransitioning;
```

### 9.5 JSON Serialization

Socket.io payloads are received as JSON objects. The UE5 SocketIOClient plugin provides `USIOJsonValue` and `USIOJsonObject` wrappers. C++ subsystems parse these manually:

```cpp
// Example: Parsing combat:damage in a subsystem
void UCombatSubsystem::OnCombatDamage(USIOJsonValue* Data)
{
    USIOJsonObject* Obj = Data->AsObject();
    int32 Damage = Obj->GetIntegerField("damage");
    bool bIsCritical = Obj->GetBoolField("isCritical");
    FString TargetName = Obj->GetStringField("targetName");
    float TargetX = Obj->GetNumberField("targetX");
    // ... process damage display
}
```

### 9.6 Multiplayer-Safe Coding Rules

Critical rules for C++ networking code (from project MEMORY.md):

1. **NEVER use `GEngine->GameViewport`** -- it is a global singleton pointing to PIE-0's viewport. Always use `World->GetGameViewport()` via the owning subsystem's world.
2. **NEVER use `GEngine->AddOnScreenDebugMessage`** for per-player feedback -- it is global. Use `UE_LOG` or per-world widget text.
3. **Always access world-scoped objects through `GetWorld()`** -- never through global pointers or singletons.
4. Every new Slate widget or subsystem feature MUST be tested with 2+ PIE instances before considering it done.

### 9.7 Reconnection Logic

Currently, if the socket connection drops, the client must return to the login screen and re-authenticate. The flow is:

```
1. Socket disconnects (network error, server restart)
2. BP_SocketManager detects disconnect
3. Client displays error message
4. Player returns to login screen
5. Re-login -> re-select character -> player:join

Future: Automatic reconnection with session restoration
1. Socket disconnects
2. Client attempts reconnect with exponential backoff (1s, 2s, 4s, 8s, max 30s)
3. On reconnect, send player:rejoin { token, characterId, lastKnownPosition }
4. Server restores session from Redis (player still in connectedPlayers for grace period)
5. Client receives state sync: health, buffs, position, inventory
```

---

## 10. Error Handling and Recovery

### 10.1 Client Disconnect Handling

When a socket disconnects, the `disconnect` event handler performs comprehensive cleanup:

```
1. Save health/mana to PostgreSQL
2. Save stats + EXP data to PostgreSQL
3. Save zone + position to PostgreSQL
4. Clear auto-attack state (Map entry)
5. Clear active cast (Map entry)
6. Clear after-cast delay (Map entry)
7. Stop anyone auto-attacking this player (iterate autoAttackState)
8. Remove from enemy combat sets (iterate all enemies)
9. If an enemy was targeting this player, pick next target or go IDLE
10. Remove from connectedPlayers Map
11. Broadcast player:left to zone room
```

**Data durability**: All persistent data (stats, EXP, position, zone, items) is saved to PostgreSQL on disconnect. The server does not rely on in-memory state surviving a process restart.

### 10.2 Server Crash Recovery

If the Node.js process crashes:

**Data preserved** (in PostgreSQL):
- Player stats, EXP, level, class (saved on last disconnect or stat change)
- Inventory state (saved on every item change)
- Character position (saved every 5-60 seconds + on disconnect)
- Zone assignment (saved on disconnect or zone change)

**Data lost** (in-memory only):
- Active combat state (auto-attack targets, ongoing fights)
- Active casts (interrupted by crash)
- Buff durations (active buffs expire when player reconnects without them)
- Enemy HP and AI state (enemies respawn at full HP)
- Ground effects (Fire Wall, Safety Wall disappear)
- Position updates since last Redis/DB write (5-60 seconds of movement)

**Recovery sequence on restart**:
1. Express/Socket.io server starts and begins listening
2. Redis reconnects (position cache regenerates as players rejoin)
3. PostgreSQL pool initializes
4. Players reconnect and send `player:join`
5. `player:join` handler loads full state from PostgreSQL (stats, items, skills, equipment)
6. Enemies lazily respawn when first player enters each zone

### 10.3 Network Latency Handling

**Position lag**: The 66ms position update rate combined with client-side interpolation smooths over up to ~200ms of latency. Beyond that, other players appear to teleport slightly. The server does not perform server-side position prediction; it uses the latest received position for all range checks.

**Combat lag**: Auto-attack damage is processed on the server tick (50ms). The client sees damage results with a round-trip delay of ~100-200ms (client -> server processing -> broadcast back). The `timestamp` field in damage events allows the client to order events correctly.

**Skill lag**: Cast times are tracked server-side using `Date.now()` timestamps. The cast bar on the client starts when `skill:cast_start` arrives and runs for the specified `castTime` duration. Minor desync is tolerable since the server is authoritative on when the cast completes.

### 10.4 Packet Loss Mitigation

Socket.io (built on Engine.io) uses WebSocket with HTTP long-polling fallback. WebSocket is TCP-based, so packet loss is handled by TCP retransmission. There is no UDP channel, so:

- **No dropped packets**: All events are guaranteed to arrive (TCP)
- **Potential ordering issues**: Socket.io guarantees per-socket ordering, but events from different sockets may interleave
- **Head-of-line blocking**: A lost TCP segment blocks all subsequent segments until retransmitted
- **Latency spikes**: TCP retransmission adds 100-500ms+ when packets are lost

For a low-to-medium scale MMO, TCP/WebSocket is acceptable. High-scale MMOs use custom UDP protocols with their own reliability layer, but the development complexity is not justified at this stage.

### 10.5 Graceful Degradation

The server handles subsystem failures without crashing:

- **Redis unavailable**: Position cache operations (`setPlayerPosition`, `getPlayerPosition`) throw errors that are caught. The combat tick falls back to `player.lastX/lastY/lastZ` when Redis lookup returns null.
- **PostgreSQL slow**: Async DB writes (EXP save, position save) do not block the game loop. Failed writes are logged but do not crash the server.
- **Socket.io event errors**: Each event handler wraps its logic in try/catch to prevent one player's error from affecting others.

### 10.6 Logging and Monitoring

**Structured logging** with levels:
- `ERROR`: Broken operations (DB write failures, uncaught exceptions)
- `WARN`: Unexpected but handled (invalid JWT, character ownership mismatch)
- `INFO`: Important state changes (player join/leave, level ups, kills)
- `DEBUG`: Per-frame data (position updates, AI ticks) -- disabled in production

All logs are written to both console and `server/logs/server.log` with ISO timestamps.

**Key events logged**:
- `[RECV]` prefix: Client event received
- `[SEND]` prefix: Event emitted to specific client
- `[BROADCAST]` prefix: Event broadcast to zone/all
- `[COMBAT]` prefix: Damage dealt, kills, range checks
- `[SKILL-COMBAT]` prefix: Skill damage results
- `[CAST]` prefix: Cast start, complete, interrupted
- `[BUFF]` prefix: Buff applied, removed, expired
- `[EXP]` prefix: Experience gained, level ups
- `[LOOT]` prefix: Item drops from monsters
- `[DB]` prefix: Database operations
- `[SECURITY]` prefix: JWT failures, ownership mismatches
- `[ZONE]` prefix: Zone transitions, enemy spawning
- `[ITEMS]` prefix: Equipment loading, stat bonuses

---

## Appendix A: Key Constants

```javascript
const COMBAT = {
    BASE_DAMAGE: 1,
    DAMAGE_VARIANCE: 0,
    MELEE_RANGE: 150,           // Default melee range (UE units)
    RANGED_RANGE: 800,          // Default ranged range
    DEFAULT_ASPD: 175,          // Default attack speed (0-195 scale)
    ASPD_CAP: 195,              // Hard cap before diminishing returns
    RANGE_TOLERANCE: 50,        // Extra units for range checks
    COMBAT_TICK_MS: 50,         // 20 ticks/second
    RESPAWN_DELAY_MS: 5000,
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }
};

const ENEMY_AI = {
    TICK_MS: 200,               // 5 ticks/second
    WANDER_PAUSE_MIN: 3000,     // ms before next wander
    WANDER_PAUSE_MAX: 8000,
    WANDER_DIST_MIN: 100,       // UE units per wander step
    WANDER_DIST_MAX: 300,
    MOVE_BROADCAST_MS: 200,     // Position broadcast throttle
    AGGRO_SCAN_MS: 500,         // Aggro detection frequency
    ASSIST_RANGE: 550,          // 11 RO cells * 50 UE units
    CHASE_GIVE_UP_EXTRA: 200,   // Extra range before dropping chase
    IDLE_AFTER_CHASE_MS: 2000,  // Delay before returning to wander
};

// Regen intervals
HP_REGEN_INTERVAL = 6000;       // 6 seconds
SP_REGEN_INTERVAL = 8000;       // 8 seconds
SKILL_REGEN_INTERVAL = 10000;   // 10 seconds
BUFF_EXPIRY_INTERVAL = 1000;    // 1 second
GROUND_EFFECT_INTERVAL = 500;   // 0.5 seconds
```

## Appendix B: In-Memory Data Structures

```javascript
// Core state stores (all in-process JavaScript Maps/Sets)
connectedPlayers   = Map<charId, PlayerObject>     // All online players
autoAttackState    = Map<attackerId, AttackTarget>  // Active auto-attack loops
activeCasts        = Map<charId, CastState>         // Skills being cast
afterCastDelayEnd  = Map<charId, timestamp>         // Global skill lockout timers
enemies            = Map<enemyId, EnemyObject>      // All spawned enemies (alive + dead)
activeGroundEffects = Map<effectId, EffectObject>   // Fire Wall, Safety Wall
spawnedZones       = Set<zoneName>                  // Zones that have had enemies spawned
zoneTransitioning  = Set<charId>                    // Players mid-zone-change
itemDefinitions    = Map<itemId, ItemDef>            // Cached item DB rows

// PlayerObject fields:
{
    socketId, characterId, characterName, zone,
    health, maxHealth, mana, maxMana, isDead,
    lastAttackTime, aspd, attackRange, weaponAspdMod,
    equipmentBonuses, hardDef, stats (base 6 + level + statPoints),
    zuzucoin, jobLevel, baseExp, jobExp, jobClass, skillPoints,
    learnedSkills, weaponType, activeBuffs, skillCooldowns,
    weaponElement, weaponLevel, armorElement, cardMods,
    lastX, lastY, lastZ  // For AI aggro + cast interruption
}

// EnemyObject fields:
{
    enemyId, templateId, name, level, zone,
    health, maxHealth, isDead,
    x, y, z, spawnX, spawnY, spawnZ,
    stats, hardDef, element, size, race, monsterClass,
    attackRange, attackMotion, damageMotion,
    aggroRange, chaseRange, wanderRadius, walkSpeed,
    baseExp, jobExp, drops[],
    aiState, modeFlags, targetPlayerId, inCombatWith (Set),
    lastAttackTime, lastDamageTime, lastAggroScan,
    wanderTargetX, wanderTargetY, isWandering, nextWanderTime,
    activeBuffs, pendingTargetSwitch, respawnMs
}
```

## Appendix C: Event Flow Diagrams

### Auto-Attack Flow (Player vs Enemy)

```
Client                     Server                          Other Clients
  |                          |                                   |
  |-- combat:attack -------->|                                   |
  |  { targetEnemyId }       |-- validate target alive           |
  |                          |-- check PvP/dead/CC               |
  |<- combat:auto_attack_----|                                   |
  |   started                |                                   |
  |                          |                                   |
  |                          |=== Combat Tick (50ms) ===         |
  |                          |-- ASPD timing check               |
  |                          |-- get pos from Redis              |
  |                          |-- range check                     |
  |                          |-- if out of range:                |
  |<- combat:out_of_range ---|                                   |
  |  (client pathfinds)      |                                   |
  |                          |                                   |
  |                          |-- if in range:                    |
  |                          |-- HIT/FLEE roll                   |
  |                          |-- damage calculation              |
  |                          |-- apply damage to enemy           |
  |                          |-- set enemy aggro                 |
  |<-------------- combat:damage (broadcast to zone) ---------->|
  |<-------------- enemy:health_update (zone) ----------------->|
  |                          |                                   |
  |                          |-- if enemy.health <= 0:           |
  |                          |   award EXP                       |
  |<- exp:gain --------------|                                   |
  |<- exp:level_up ----------|-------- (broadcast to zone) ---->|
  |                          |   roll loot                       |
  |<- loot:drop -------------|                                   |
  |<- inventory:data --------|                                   |
  |                          |   schedule respawn                |
  |<-------------- enemy:death (broadcast to zone) ------------>|
```

### Skill Cast Flow

```
Client                     Server                          Other Clients
  |                          |                                   |
  |-- skill:use ------------>|                                   |
  |  { skillId, targetId }   |-- validate: skill learned?        |
  |                          |-- validate: SP enough?            |
  |                          |-- validate: cooldown clear?       |
  |                          |-- validate: ACD clear?            |
  |                          |-- validate: target in range?      |
  |                          |-- validate: not CC'd?             |
  |                          |                                   |
  |                          |-- calculate cast time             |
  |                          |   (BaseCast * (1 - DEX/150))      |
  |                          |                                   |
  |                          |-- if castTime > 0:                |
  |                          |   store in activeCasts Map        |
  |<-------------- skill:cast_start (broadcast to zone) ------->|
  |                          |                                   |
  |                          |=== Combat Tick (waits) ===        |
  |                          |-- if now >= castEndTime:          |
  |                          |   re-validate all conditions      |
  |<-------------- skill:cast_complete (broadcast) ------------>|
  |                          |   execute skill effect            |
  |                          |                                   |
  |                          |-- if castTime == 0:               |
  |                          |   execute immediately             |
  |                          |                                   |
  |<- skill:used ------------|                                   |
  |<- combat:health_update --|                                   |
  |<-------------- skill:effect_damage (broadcast) ------------>|
  |<- skill:cooldown_started-|                                   |
  |<- skill:acd_started -----|                                   |
```

### Zone Transition Flow

```
Client                     Server
  |                          |
  |-- (overlap WarpPortal)   |
  |-- zone:warp ------------>|
  |  { warpId }              |-- validate warp exists
  |                          |-- validate proximity
  |                          |-- lookup destination zone
  |                          |-- socket.leave old zone room
  |                          |-- save position to DB
  |                          |-- broadcast player:left
  |                          |-- update player.zone
  |                          |-- socket.join new zone room
  |<- zone:change -----------|
  |  { zone, levelName,      |
  |    spawnX/Y/Z }          |
  |                          |
  |-- OpenLevel(levelName)   |
  |-- (level loads)          |
  |-- (Level BP spawns pawn) |
  |                          |
  |-- zone:ready ----------->|
  |  { characterId }         |-- send existing enemies
  |<- enemy:spawn (x N) ----|-- send other players
  |<- player:moved (x N) ---|-- send zone metadata
  |<- zone:data -------------|
```

---

*Document generated from analysis of the Sabri_MMO codebase at commit 1258848. Server source: `server/src/index.js` (~8,400 lines). Client source: `client/SabriMMO/Source/SabriMMO/` (23 core C++ files + subsystems).*
