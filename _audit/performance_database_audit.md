# Database Query Performance Audit

**Date**: 2026-03-23
**Server file**: `server/src/index.js` (~32,567 lines)
**Database**: PostgreSQL + Redis (position cache)
**Total queries cataloged**: ~370 `pool.query` + ~30 `client.query` (transaction) calls

---

## Table of Contents

1. [Database Schema Summary](#1-database-schema-summary)
2. [Index Coverage Analysis](#2-index-coverage-analysis)
3. [Query Catalog by Frequency](#3-query-catalog-by-frequency)
4. [Hot-Path Queries (Tick Loops)](#4-hot-path-queries-tick-loops)
5. [N+1 Query Patterns](#5-n1-query-patterns)
6. [Full Table Scans & SELECT *](#6-full-table-scans--select-)
7. [Missing WHERE Clauses & Over-fetching](#7-missing-where-clauses--over-fetching)
8. [Caching Opportunities (Redis)](#8-caching-opportunities-redis)
9. [Optimization Recommendations](#9-optimization-recommendations)

---

## 1. Database Schema Summary

### Tables

| Table | Primary Key | Foreign Keys | Estimated Row Count (active game) |
|-------|------------|-------------|----------------------------------|
| `users` | `user_id` (SERIAL) | - | Low (10s-100s) |
| `characters` | `character_id` (SERIAL) | `user_id -> users` | Low-Med (100s) |
| `items` | `item_id` (SERIAL) | - | **6,169** (static, canonical rAthena) |
| `character_inventory` | `inventory_id` (SERIAL) | `character_id -> characters`, `item_id -> items` | Med-High (1000s+) |
| `character_hotbar` | `(character_id, row_index, slot_index)` | `character_id -> characters`, `inventory_id -> character_inventory` | Low (36 per char max) |
| `character_skills` | `(character_id, skill_id)` | `character_id -> characters` | Med (10-30 per char) |
| `character_cart` | `cart_id` (SERIAL) | `character_id -> characters`, `item_id -> items` | Low |
| `character_homunculus` | `homunculus_id` (SERIAL) | `character_id -> characters` | Low (1 per Alchemist) |
| `character_pets` | `id` (SERIAL) | `character_id -> characters` | Low |
| `character_memo` | `(character_id, slot_index)` | `character_id -> characters` | Low (3 per char max) |
| `parties` | `party_id` (SERIAL) | `leader_id -> characters` | Low |
| `party_members` | `(party_id, character_id)` | FK to both | Low |
| `vending_shops` | `shop_id` (SERIAL) | `character_id -> characters` | Low (transient) |
| `vending_items` | `vend_item_id` (SERIAL) | `shop_id -> vending_shops` | Low (transient) |
| `skills` | `skill_id` (SERIAL) | - | Med (unused at runtime; skills defined in JS) |
| `skill_prerequisites` | - | `skill_id -> skills` | Med (unused at runtime) |
| `skill_levels` | - | `skill_id -> skills` | Med (unused at runtime) |

### Key Observations
- `items` table (6,169 rows) is loaded into memory at startup via `loadItemDefinitions()` -- good.
- `skills`, `skill_prerequisites`, `skill_levels` tables exist but are NOT queried at runtime. Skill data lives in `ro_skill_data.js`.
- `character_inventory` is the most heavily queried table (inventory, equipment, items, weight).

---

## 2. Index Coverage Analysis

### Existing Indexes

| Index | Table | Column(s) | Type |
|-------|-------|-----------|------|
| `idx_users_username` | users | username | B-tree |
| `idx_users_email` | users | email | B-tree |
| `idx_characters_user_id` | characters | user_id | B-tree |
| `idx_characters_deleted` | characters | deleted | Partial (WHERE deleted = FALSE) |
| `idx_characters_job_class` | characters | job_class | B-tree |
| `idx_characters_level` | characters | level | B-tree |
| `idx_characters_zone` | characters | zone_name | B-tree |
| `idx_characters_delete_date` | characters | delete_date | B-tree (partial: WHERE delete_date IS NOT NULL) |
| `idx_inventory_character` | character_inventory | character_id | B-tree |
| `idx_inventory_item` | character_inventory | item_id | B-tree |
| `idx_inventory_broken` | character_inventory | (character_id, is_broken) | Partial (WHERE is_broken = TRUE) |
| `idx_ci_equipped_cards` | character_inventory | character_id | Partial (WHERE is_equipped AND compounded_cards != '[]') |
| `idx_hotbar_character` | character_hotbar | character_id | B-tree |
| `idx_hotbar_character_row` | character_hotbar | (character_id, row_index) | B-tree |
| `idx_skills_class` | skills | class_id | B-tree |
| `idx_char_skills_character` | character_skills | character_id | B-tree |
| `idx_char_skills_skill` | character_skills | skill_id | B-tree |
| `idx_cart_character` | character_cart | character_id | B-tree |
| `idx_vending_zone` | vending_shops | zone | B-tree |
| `idx_homunculus_character` | character_homunculus | character_id | UNIQUE |
| `idx_homunculus_summoned` | character_homunculus | (character_id, is_summoned) | B-tree |
| `idx_party_members_char` | party_members | character_id | B-tree |
| `idx_parties_leader` | parties | leader_id | B-tree |
| `idx_character_pets_char` | character_pets | character_id | B-tree |

### MISSING Indexes (Critical)

| Table | Missing Index | Queried Pattern | Impact |
|-------|--------------|----------------|--------|
| **character_inventory** | `(character_id, is_equipped)` | `WHERE character_id = $1 AND is_equipped = true` | **HIGH** -- queried 6+ times during player:join, every equip/unequip |
| **character_inventory** | `(character_id, item_id, is_equipped)` | `WHERE character_id = $1 AND item_id = $2 AND is_equipped = false` | **HIGH** -- catalyst consumption, item stacking checks |
| **character_inventory** | `(character_id, is_equipped, equipped_position)` | `WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.equipped_position IN (...)` | **MEDIUM** -- weapon/armor/ammo queries during player:join |
| **vending_items** | `(shop_id)` | `WHERE shop_id = $1` | **LOW** -- vending is transient, low volume |
| **character_memo** | `(character_id)` | `WHERE character_id = $1` | **LOW** -- max 3 rows per character |
| **parties** | `(name)` | `WHERE name = $1` (uniqueness check) | **LOW** -- UNIQUE constraint on name already creates an implicit index |

---

## 3. Query Catalog by Frequency

### Startup (once)

| Line | Query | Notes |
|------|-------|-------|
| 31984-32504 | ~40 `ALTER TABLE ADD COLUMN IF NOT EXISTS` | Schema migration on boot |
| 32251-32504 | `CREATE TABLE IF NOT EXISTS`, `CREATE INDEX IF NOT EXISTS` | Table/index creation |
| 4840 | `SELECT * FROM items` | Loads all 6,169 items into memory cache |
| 32392-32396 | `SELECT column_name FROM information_schema.columns` | Schema introspection |
| 32465-32475 | Slot re-index query | Fixes character inventory slots once |

### Per Player Join (once per session, ~12 sequential queries)

| Line | Query | Notes |
|------|-------|-------|
| 5311 | `SELECT 1 FROM characters WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE` | JWT ownership check |
| 5335 | `SELECT x, y, z, health, max_health, ... FROM characters WHERE character_id = $1` | Load character data |
| 5384 | `SELECT str, agi, vit, int_stat, ... FROM characters WHERE character_id = $1` | Load stats |
| 5417 | `SELECT i.atk, i.weapon_type, ... FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.equipped_position IN ('weapon', 'weapon_left')` | Weapon load |
| 5470 | `SELECT ... FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.equipped_position = 'ammo'` | Ammo load |
| 5497 | `SELECT i.element FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'armor'` | Armor element |
| 5516 | `SELECT i.weight, ci.refine_level FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.equipped_position = 'shield'` | Shield load |
| 5535 | `SELECT skill_id, level FROM character_skills WHERE character_id = $1` | Skills load |
| 5563 | `SELECT i.def, i.mdef, ... FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true` | All equipment bonuses |
| 5766 | `SELECT ... FROM character_cart cc JOIN items i ... WHERE cc.character_id = $1` | Cart items load |
| 5786 | `SELECT * FROM parties p JOIN party_members pm ... WHERE pm.character_id = $1` | Party load |
| 5798 | `SELECT pm.character_id, c.name ... FROM party_members pm JOIN characters c ... WHERE pm.party_id = $1` | Party members |
| 2725 | `SELECT ci.compounded_cards, ... FROM character_inventory ci JOIN items i ... WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.compounded_cards IS NOT NULL` | Card bonuses rebuild |

**Issue**: These 12+ sequential queries could be consolidated. See [Recommendation R1](#r1-consolidate-player-join-queries).

### Per Request (user-triggered, variable frequency)

| Category | Lines | Query Count per Action | Notes |
|----------|-------|----------------------|-------|
| Inventory load | 4981 | 1 (large JOIN) | `getPlayerInventory()` -- ~50 columns, returns ALL items |
| Hotbar load | 5041 | 1 (LEFT JOIN) | `getPlayerHotbar()` |
| Weight calculation | 4787 | 1 (SUM + JOIN) | `calculatePlayerCurrentWeight()` -- called after every inventory change |
| Item use | 22664-22716 | 3-4 | Unidentified check + delete/update + inventory reload + weight recalc |
| Equip/unequip | 22736+ | 3-10 | Item fetch + old equip check + update + potentially multiple accessory swaps |
| Buy from NPC | 23768+ | 3-5 per transaction | Slot lookup + insert/update + inventory reload |
| Sell to NPC | 23877+ | 3-4 per transaction | Item lookup + delete/update + zeny update |
| Refine | 24237-24321 | 4-6 | Item lookup + ore check + ore consume + zeny update + refine update/destroy |
| Forge | 24398-24553 | 8-15 (transaction) | Material checks + material consumption + forged item insert + zeny |
| Vending buy | 7120-7185 | 3-6 per item purchased | See [N+1 pattern](#n1-3) |
| Card compound | 23338-23429 | 4-5 | Card check + equip check + update cards + consume card + update equip |
| Skill learn | 8297-8327 | 2-3 | Skill check + upsert + skill points update |
| Chat /memo | 8844 | 1 | UPSERT memo location |
| Party create | 8381-8395 | 3 | Name check + insert party + insert member |

### Periodic Timer Queries

| Interval | Line | Query | Frequency |
|----------|------|-------|-----------|
| **60s** | 32550-32565 | `savePlayerHealthToDB()` + `saveExpDataToDB()` + zone/position UPDATE | **3 queries per connected player every 60s** |
| On EXP gain | 2105 | `UPDATE characters SET base_exp = $1, job_exp = $2 WHERE character_id = $3` | Per enemy kill |
| On pet hunger tick | 2132 | `UPDATE character_pets SET intimacy = $1 WHERE id = $2` | Every 60s per active pet |
| On homunculus death | 27561 | `UPDATE character_homunculus SET is_alive = false ...` | Rare |
| On pet starvation | 27644 | `DELETE FROM character_pets WHERE id = $1` | Rare |

---

## 4. Hot-Path Queries (Tick Loops)

### Combat Tick (50ms interval, line 24778)

**No direct DB queries in the combat tick loop.** This is correct. The combat tick at line 24778 only uses in-memory data (`connectedPlayers`, `enemies`, `autoAttackState` Maps). Position lookups use `getPlayerPosSync()` (in-memory) for enemy AI and `getPlayerPosition()` (Redis) for player skill targeting.

**However**, `processEnemyDeathFromSkill()` is called from within the combat tick when an enemy dies from auto-attack, and this function triggers:
- `pool.query('UPDATE characters SET base_exp = ...')` (line 2105) -- EXP save
- `addItemToInventory()` for drops (1-5 queries per drop)
- `updatePlayerWeightCache()` (1 query for weight recalc)
- `getPlayerInventory()` (1 large query for inventory refresh)
- `saveExpDataToDB()` on level-up (1 query)

**Total per enemy kill: 4-10 DB queries** triggered from within the 50ms tick. These are awaited, meaning the tick stalls until DB I/O completes. With multiple players fighting, this can cascade.

### Enemy AI Tick (200ms interval, line 30107)

**No direct DB queries.** Uses `getPlayerPosSync()` (in-memory) for all position checks. Correctly avoids Redis and DB in the hot loop.

### Position Update Handler (30Hz per player, line 5974)

**Redis only.** Calls `setPlayerPosition()` (Redis `setEx`) on every position update. No PostgreSQL queries on normal movement. Zone transitions trigger DB writes (line 6221) but those are rare events.

### Regen Ticks (1-10s intervals, lines 26577-26808)

**No DB queries.** All regen calculations use in-memory player state. Correct.

### Status/Buff Expiry Tick (1s interval, line 26814)

**No DB queries.** Status effects and buffs are tracked in-memory. Correct.

---

## 5. N+1 Query Patterns

### N+1 #1: Periodic Save (CRITICAL -- line 32550)

```javascript
setInterval(async () => {
  for (const [charId, player] of connectedPlayers.entries()) {
    if (!player.isDead) {
      await savePlayerHealthToDB(charId, player.health, player.mana);    // Query 1
      await saveExpDataToDB(charId, player);                              // Query 2
      await pool.query('UPDATE characters SET zone_name = ...');          // Query 3
    }
  }
}, 60000);
```

**Impact**: **3 sequential UPDATE queries per connected player, every 60 seconds.** With 50 players online, that is 150 sequential queries every minute. With 200 players, 600 queries -- all serialized because they await each player before moving to the next.

**Worse**: `savePlayerHealthToDB` and `saveExpDataToDB` both UPDATE the `characters` table for the same row. These should be a single UPDATE statement, and the entire loop should batch all updates into one query.

### N+1 #2: Player Join Equipment Queries (MEDIUM -- lines 5311-5798)

During `player:join`, the server runs 12+ sequential queries against the same character's data:
1. Ownership check (characters)
2. Position/health (characters)
3. Stats (characters)
4. Weapon (character_inventory JOIN items, filtered by equipped+position)
5. Ammo (character_inventory JOIN items, filtered by equipped+position)
6. Armor element (character_inventory JOIN items, filtered by equipped+slot)
7. Shield (character_inventory JOIN items, filtered by equipped+position)
8. Learned skills (character_skills)
9. Equipment bonuses (character_inventory JOIN items, filtered by equipped)
10. Card bonuses (character_inventory JOIN items, filtered by equipped+cards)
11. Cart items (character_cart JOIN items)
12. Party membership (parties JOIN party_members)

Queries 1-3 all hit the `characters` table for the same `character_id`. Queries 4-7, 9-10 all hit `character_inventory` with `is_equipped = true` for the same `character_id`. These could be consolidated into 3-4 queries total.

### N+1 #3: Vending Buy Loop (MEDIUM -- lines 7157-7187)

```javascript
for (const vp of validPurchases) {
    const cartItemAttrs = await pool.query('SELECT ... FROM character_cart WHERE cart_id = $1', [vp.cartId]);
    await addFullItemToInventory(characterId, vp.itemId, vp.qty, attrs);  // 2-3 queries
    await pool.query('DELETE/UPDATE vending_items ...');                     // 1 query
    const cartItem = await pool.query('SELECT quantity FROM character_cart WHERE cart_id = $1', [vp.cartId]);
    await pool.query('DELETE/UPDATE character_cart ...');                    // 1 query
}
```

**Impact**: 5-7 queries per purchased item, serialized in a loop. A player buying 5 items = 25-35 sequential queries. Not in a transaction, so partially-failed purchases can leave inconsistent state.

### N+1 #4: Cart-to-Inventory Transfer (MEDIUM -- lines 6792-6848)

The `cart:to_inventory` handler runs multiple queries per item transferred:
1. Inventory lookup for source item
2. Delete/update source
3. Check existing stack in destination
4. Insert/update destination
5. Cart reload query

### N+1 #5: Catalyst Consumption (LOW -- lines 1918-1926)

```javascript
for (const cat of catalysts) {
    await pool.query('UPDATE character_inventory SET quantity = quantity - $3 WHERE ctid = ...');
}
```

Most skills consume 1 catalyst (1 query). `land_protector` consumes 2 (2 queries). Low impact but pattern exists.

### N+1 #6: Disconnect Save (LOW -- lines 7287-7425)

On player disconnect, the server runs 4-8 sequential UPDATE queries:
1. Vending cleanup (2 DELETEs if vending)
2. Health save
3. Stats + EXP save
4. Zone + position save
5. Homunculus persist
6. Pet persist
7. Plagiarism persist

These all target different tables so cannot easily batch, but they are sequential awaits on a single disconnect event.

---

## 6. Full Table Scans & SELECT *

### SELECT * FROM items (line 4840)

```javascript
const result = await pool.query('SELECT * FROM items');
```

Fetches all 6,169 rows with all ~60 columns at startup. This is intentional (populates the in-memory `itemDefinitions` cache) and runs once. **Acceptable** but could be optimized to only fetch columns actually used at runtime.

### getPlayerInventory() Over-fetching (line 4981)

```javascript
SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
       ci.equipped_position, ci.refine_level, ci.compounded_cards,
       ci.forged_by, ci.forged_element, ci.forged_star_crumbs, ci.identified,
       i.name, i.description, i.full_description, i.item_type, i.equip_slot,
       i.weight, i.price, i.buy_price, i.sell_price,
       i.atk, i.def, i.matk, i.mdef,
       i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
       i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus, i.perfect_dodge_bonus,
       i.max_hp_rate, i.max_sp_rate, i.aspd_rate,
       i.hp_regen_rate, i.sp_regen_rate, i.crit_atk_rate, i.cast_rate, i.use_sp_rate, i.heal_power,
       i.required_level, i.stackable, i.icon,
       i.weapon_type, i.aspd_modifier, i.weapon_range,
       i.slots, i.weapon_level, i.refineable, i.jobs_allowed,
       i.card_type, i.card_prefix, i.card_suffix,
       i.two_handed, i.element
FROM character_inventory ci
JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1
```

**~50 columns** fetched via JOIN for every item in inventory. This query is called:
- On `inventory:load`
- After every item use (consumable)
- After every catalyst consumption (`consumeSkillCatalysts`)
- After every NPC buy/sell
- After every vending purchase
- After loot drops

Since `items` data is already cached in `itemDefinitions` Map, the JOIN is redundant. The query only needs `character_inventory` columns, then card details can be resolved from the in-memory cache.

### Party Member Query on Join (line 5798)

```javascript
SELECT pm.character_id, c.name, c.job_class, c.level, c.health, c.max_health, c.mana, c.max_mana, c.zone_name
FROM party_members pm JOIN characters c ON pm.character_id = c.character_id
WHERE pm.party_id = $1
```

This is fine -- small result set, only on join. But the data is already available in `activeParties` in-memory Map for online members.

---

## 7. Missing WHERE Clauses & Over-fetching

### Weight Calculation (line 4787)

```javascript
SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as total_weight
FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1
```

This scans ALL inventory rows for the character (equipped + unequipped). Could use `itemDefinitions` cache to avoid the JOIN entirely: just `SELECT item_id, quantity FROM character_inventory WHERE character_id = $1` and sum weights in JS using the cache.

### Equipped Item Queries -- Repeated Scans

During `player:join`, the following pattern repeats 6 times, each scanning `character_inventory` with `is_equipped = true`:
- Weapon query (line 5417): filtered by `equipped_position IN ('weapon', 'weapon_left')`
- Ammo query (line 5470): filtered by `equipped_position = 'ammo'`
- Armor query (line 5497): filtered by `equip_slot = 'armor'`
- Shield query (line 5516): filtered by `equipped_position = 'shield'`
- All equipment bonuses (line 5563): no position filter (gets ALL equipped)
- Card bonuses (line 2725): filtered by `compounded_cards IS NOT NULL`

Queries 4-7 could be done with a single query: `WHERE ci.character_id = $1 AND ci.is_equipped = true`. Parse results in JS.

### Unidentified Item Check During Item Use (line 22664)

```javascript
SELECT ... FROM character_inventory ci JOIN items i ... WHERE ci.inventory_id = $1 AND ci.character_id = $2
```

Fetches from DB even though the full inventory was just sent to the client. Could validate against in-memory state.

---

## 8. Caching Opportunities (Redis)

### Currently Using Redis For
- **Player positions**: `player:{id}:position` with 5-minute TTL (good)

### NOT Using Redis For (Opportunities)

| Data | Current Approach | Suggested Cache | Benefit |
|------|-----------------|----------------|---------|
| **Item definitions** | Loaded into JS Map at startup | Already cached in-memory (good) | N/A |
| **Player inventory** | Full DB query on every change | Cache inventory in Redis, invalidate on write | Eliminates ~50-column JOIN queries |
| **Player equipment bonuses** | Computed from 6 DB queries on join | Cache equipment summary in Redis, rebuild on equip/unequip | Eliminates 6 queries on rejoin |
| **Character stats** | 3 separate DB reads on join | Single Redis hash `char:{id}:stats` | Eliminates 2 of 3 character reads |
| **Hotbar data** | DB query on every load/save | Cache in Redis, invalidate on save | Minor savings |
| **Learned skills** | DB query on join | Cache in Redis `char:{id}:skills` | Eliminates 1 query on rejoin |

### Redis Anti-Pattern: `KEYS` Command (line 31952)

```javascript
async function getPlayersInZone(zone = 'default') {
    const keys = await redisClient.keys('player:*:position');
    ...
}
```

`KEYS` is O(N) and blocks Redis during execution. With many cached keys, this degrades performance. **However**, this function does not appear to be called at runtime (no callers found in the codebase) -- it is likely dead code from an earlier design. Safe to remove.

---

## 9. Optimization Recommendations

Ranked by impact (estimated performance gain + effort):

### R1: Batch Periodic Save (CRITICAL)

**Current**: 3 sequential queries per player every 60s (N+1 pattern).
**Fix**: Combine into a single batched query using PostgreSQL `unnest` or a CTE:

```javascript
setInterval(async () => {
    const updates = [];
    for (const [charId, player] of connectedPlayers.entries()) {
        if (!player.isDead) {
            updates.push([charId, player.health, player.mana,
                player.stats?.level || 1, player.jobLevel || 1,
                player.baseExp || 0, player.jobExp || 0,
                player.jobClass || 'novice', player.stats?.statPoints || 0,
                player.skillPoints || 0, player.maxHealth, player.maxMana,
                player.zone || 'prontera_south',
                player.lastX || 0, player.lastY || 0, player.lastZ || 0]);
        }
    }
    if (updates.length === 0) return;
    // Single query updates all players at once
    await pool.query(`
        UPDATE characters SET
            health = v.health, mana = v.mana,
            level = v.level, job_level = v.job_level,
            base_exp = v.base_exp, job_exp = v.job_exp,
            job_class = v.job_class, stat_points = v.stat_points,
            skill_points = v.skill_points, max_health = v.max_health,
            max_mana = v.max_mana, zone_name = v.zone_name,
            x = v.x, y = v.y, z = v.z
        FROM (VALUES ${updates.map((_, i) => {
            const base = i * 16;
            return `($${base+1}::int, $${base+2}::int, ...)`;
        }).join(',')}) AS v(...)
        WHERE characters.character_id = v.char_id
    `, updates.flat());
}, 60000);
```

**Impact**: Reduces 3N queries to 1 query. With 50 players: 150 queries -> 1 query. **~99% reduction**.
**Effort**: Medium. Needs careful parameter mapping.

### R2: Add Composite Index on character_inventory (HIGH)

```sql
CREATE INDEX IF NOT EXISTS idx_ci_char_equipped
ON character_inventory (character_id, is_equipped)
WHERE is_equipped = true;

CREATE INDEX IF NOT EXISTS idx_ci_char_item_unequipped
ON character_inventory (character_id, item_id)
WHERE is_equipped = false;
```

**Impact**: Speeds up the 6+ equipped-item queries during `player:join`, every equip/unequip, and catalyst consumption. Currently these scan all inventory rows for the character then filter in PostgreSQL.
**Effort**: Low. Just add the indexes.

### R3: Consolidate Player Join Queries (HIGH)

**Current**: 12+ sequential queries loading character data on `player:join`.
**Fix**: Merge the 3 `characters` table reads into 1 query. Merge the 6 `character_inventory` equipped-item reads into 1 query. Parse results in JavaScript.

```javascript
// Single character query (combines lines 5311, 5335, 5384)
const charResult = await pool.query(
    `SELECT c.*, 1 as owner_check
     FROM characters c
     WHERE c.character_id = $1 AND c.user_id = $2 AND c.deleted = FALSE`,
    [characterId, decoded.user_id]
);

// Single equipped items query (combines lines 5417, 5470, 5497, 5516, 5563, 2725)
const equipResult = await pool.query(
    `SELECT ci.*, i.*
     FROM character_inventory ci
     JOIN items i ON ci.item_id = i.item_id
     WHERE ci.character_id = $1 AND ci.is_equipped = true`,
    [characterId]
);
// Parse weapon, ammo, armor, shield, bonuses, and cards from single result set in JS
```

**Impact**: Reduces 12+ queries to ~4 queries (character, equipped items, skills, party). Saves ~100ms of sequential DB latency per join.
**Effort**: Medium. Requires restructuring the join handler.

### R4: Eliminate JOIN in getPlayerInventory (MEDIUM)

**Current**: 50-column JOIN with `items` table every time inventory is refreshed.
**Fix**: Since `itemDefinitions` Map is already populated at startup, query only `character_inventory` columns and enrich from the cache:

```javascript
async function getPlayerInventory(characterId) {
    const result = await pool.query(
        `SELECT inventory_id, item_id, quantity, is_equipped, slot_index,
                equipped_position, refine_level, compounded_cards,
                forged_by, forged_element, forged_star_crumbs, identified
         FROM character_inventory WHERE character_id = $1
         ORDER BY slot_index ASC, created_at ASC`,
        [characterId]
    );
    return result.rows.map(ci => {
        const itemDef = itemDefinitions.get(ci.item_id);
        return { ...ci, ...itemDef }; // Merge static item data from cache
    });
}
```

**Impact**: Eliminates the JOIN, reduces data transferred from PostgreSQL by ~80%. Query becomes a simple index scan on `character_inventory(character_id)`.
**Effort**: Low. The `itemDefinitions` cache is already there and used elsewhere.

### R5: Eliminate JOIN in Weight Calculation (MEDIUM)

**Current**: `calculatePlayerCurrentWeight()` does a SUM with JOIN against `items` table.
**Fix**: Query only `item_id` and `quantity` from `character_inventory`, sum weights using `itemDefinitions` cache:

```javascript
async function calculatePlayerCurrentWeight(characterId) {
    const result = await pool.query(
        'SELECT item_id, quantity FROM character_inventory WHERE character_id = $1',
        [characterId]
    );
    let total = 0;
    for (const row of result.rows) {
        const def = itemDefinitions.get(row.item_id);
        total += (def?.weight || 0) * row.quantity;
    }
    return total;
}
```

**Impact**: Eliminates JOIN. Weight query runs after every inventory change -- this optimization compounds.
**Effort**: Low.

### R6: Wrap Vending Purchase in Transaction (MEDIUM)

**Current**: Vending buy loop (lines 7148-7195) runs multiple queries without a transaction. If the server crashes mid-loop, zeny is deducted but items may not be delivered.
**Fix**: Wrap the entire purchase in a PostgreSQL transaction (`BEGIN`/`COMMIT`/`ROLLBACK`).

**Impact**: Data integrity. Also allows batching the UPDATE/DELETE/INSERT queries within the transaction.
**Effort**: Medium.

### R7: Deduplicate EXP Save (LOW)

**Current**: `processEnemyDeathFromSkill` calls `pool.query('UPDATE characters SET base_exp = $1, job_exp = $2 ...')` on every enemy kill (line 2105). The periodic 60s save also writes EXP.
**Fix**: Remove the per-kill EXP save. Rely on:
1. The 60-second periodic save
2. The disconnect save
3. Level-up events (which already call `saveExpDataToDB`)

**Risk**: If the server crashes, up to 60 seconds of EXP could be lost. Acceptable for a game server.
**Impact**: Eliminates 1 UPDATE per enemy kill. In active combat, this is frequent.
**Effort**: Low. Just remove the line.

### R8: Remove Dead Code - getPlayersInZone (LOW)

**Current**: `getPlayersInZone()` (line 31951) uses Redis `KEYS` command (O(N) blocking scan). Function appears to have no callers.
**Fix**: Remove the function. If zone-player queries are needed in the future, use Redis `SCAN` or better yet, use the in-memory `connectedPlayers` Map filtered by `player.zone`.

**Impact**: Prevents accidental future use of a performance-hostile pattern.
**Effort**: Trivial.

### R9: Cache Equipped Item Summary in Redis (LOW)

**Current**: 6 queries for equipped items on every `player:join`.
**Fix**: After computing equipment bonuses, cache them in Redis as `char:{id}:equip_summary`. Invalidate on equip/unequip. On rejoin, load from Redis first; if miss, fall back to DB.

**Impact**: Reduces rejoin (zone transition reconnect) latency by eliminating 6 DB queries.
**Effort**: Medium. Needs cache invalidation on every equip/unequip event.

### R10: Rate-Limit getPlayerInventory Calls (LOW)

**Current**: `getPlayerInventory()` is called after every single inventory change (use item, buy, sell, loot, etc.). Multiple rapid actions (e.g., using 5 potions) trigger 5 full inventory reloads.
**Fix**: Debounce inventory refresh -- coalesce multiple changes within a 100ms window into a single reload.

**Impact**: Reduces redundant large queries during rapid inventory changes.
**Effort**: Low-Medium.

---

## Summary

### Top 3 Quick Wins (Low Effort, High Impact)

1. **R2**: Add composite index on `character_inventory(character_id, is_equipped) WHERE is_equipped = true`
2. **R4**: Eliminate JOIN in `getPlayerInventory()` using in-memory item cache
3. **R5**: Eliminate JOIN in `calculatePlayerCurrentWeight()` using in-memory item cache

### Top 3 Structural Improvements (Medium Effort, High Impact)

1. **R1**: Batch the 60-second periodic save into a single query (eliminates 3N queries)
2. **R3**: Consolidate `player:join` from 12+ queries to 4 queries
3. **R6**: Wrap vending purchases in a transaction (integrity + batching opportunity)

### Hot-Path Assessment

The server correctly avoids DB queries in:
- Combat tick (50ms) -- all in-memory
- Enemy AI tick (200ms) -- all in-memory
- Regen ticks (1-10s) -- all in-memory
- Status/buff expiry (1s) -- all in-memory
- Position handler (30Hz) -- Redis only

The main performance risks are:
- **Enemy death processing** (triggered from combat tick): 4-10 DB queries per kill
- **Periodic save** (60s): 3N sequential queries
- **Player join** (per session): 12+ sequential queries
- **Inventory refresh** (per action): large JOIN query called too frequently
