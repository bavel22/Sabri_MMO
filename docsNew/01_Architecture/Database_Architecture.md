# Database Architecture

## Overview

Sabri_MMO uses **PostgreSQL 15.4** as the primary relational database and **Redis 7.2+** as an in-memory cache. PostgreSQL stores all persistent game data (users, characters, items, inventory). Redis caches real-time player positions for fast combat range checks.

## Database Connection

```javascript
// PostgreSQL (pg module v8.11.3)
const pool = new Pool({
  host: process.env.DB_HOST,      // localhost
  port: process.env.DB_PORT,      // 5432
  database: process.env.DB_NAME,  // sabri_mmo
  user: process.env.DB_USER,      // postgres
  password: process.env.DB_PASSWORD // goku22
});

// Redis (redis module v5.10.0)
const redisClient = redis.createClient({
  host: process.env.REDIS_HOST || 'localhost',
  port: process.env.REDIS_PORT || 6379
});
await redisClient.connect(); // Required for redis v4+
```

## Entity-Relationship Diagram

```
┌─────────────┐       ┌──────────────────┐       ┌─────────────┐
│    users     │       │   characters     │       │    items     │
├─────────────┤       ├──────────────────┤       ├─────────────┤
│ user_id (PK)│──1:N─►│ character_id (PK)│       │ item_id (PK)│
│ username    │       │ user_id (FK)     │       │ name        │
│ email       │       │ name             │       │ description │
│ password_hash│      │ class            │       │ item_type   │
│ created_at  │       │ level            │       │ equip_slot  │
│ last_login  │       │ experience       │       │ weight      │
└─────────────┘       │ x, y, z          │       │ price       │
                      │ health/max_health│       │ atk/def/matk│
                      │ mana/max_mana    │       │ stat bonuses│
                      │ str/agi/vit/...  │       │ stackable   │
                      │ stat_points      │       │ weapon_type │
                      │ created_at       │       │ aspd_mod    │
                      │ last_played      │       │ weapon_range│
                      └────────┬─────────┘       └──────┬──────┘
                               │                        │
                               │    ┌───────────────────┘
                               │    │
                      ┌────────┴────┴────────┐
                      │ character_inventory   │
                      ├──────────────────────┤
                      │ inventory_id (PK)    │◄──────────────────────┐
                      │ character_id (FK)    │                       │
                      │ item_id (FK)         │       ┌───────────────┴──────┐
                      │ quantity             │       │ character_hotbar      │
                      │ is_equipped          │       ├──────────────────────┤
                      │ slot_index           │       │ character_id (PK,FK) │
                      │ created_at           │       │ slot_index (PK)      │
                      └──────────────────────┘       │ inventory_id (FK)    │
                                                     │ item_id              │
                                                     │ item_name            │
                                                     └──────────────────────┘
```

## Table Definitions

### `users`

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| `user_id` | SERIAL | PRIMARY KEY | Auto-incrementing user ID |
| `username` | VARCHAR(50) | UNIQUE NOT NULL | Login username |
| `email` | VARCHAR(100) | UNIQUE NOT NULL | User email |
| `password_hash` | VARCHAR(255) | NOT NULL | bcrypt hashed password |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Account creation time |
| `last_login` | TIMESTAMP | nullable | Last login timestamp |

### `characters`

| Column | Type | Constraints | Default | Description |
|--------|------|-------------|---------|-------------|
| `character_id` | SERIAL | PRIMARY KEY | auto | Character ID |
| `user_id` | INTEGER | FK → users(user_id) ON DELETE CASCADE | — | Owner |
| `name` | VARCHAR(50) | NOT NULL | — | Character name |
| `class` | VARCHAR(20) | — | 'warrior' | Class: warrior/mage/archer/healer/priest |
| `level` | INTEGER | — | 1 | Character level |
| `experience` | INTEGER | — | 0 | Experience points |
| `x` | FLOAT | — | 0 | World X position |
| `y` | FLOAT | — | 0 | World Y position |
| `z` | FLOAT | — | 0 | World Z position |
| `health` | INTEGER | — | 100 | Current health |
| `max_health` | INTEGER | — | 100 | Maximum health |
| `mana` | INTEGER | — | 100 | Current mana |
| `max_mana` | INTEGER | — | 100 | Maximum mana |
| `str` | INTEGER | — | 1 | Strength stat |
| `agi` | INTEGER | — | 1 | Agility stat |
| `vit` | INTEGER | — | 1 | Vitality stat |
| `int_stat` | INTEGER | — | 1 | Intelligence stat (int_stat to avoid SQL keyword) |
| `dex` | INTEGER | — | 1 | Dexterity stat |
| `luk` | INTEGER | — | 1 | Luck stat |
| `stat_points` | INTEGER | — | 48 | Available stat points |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | — | Creation time |
| `last_played` | TIMESTAMP | nullable | — | Last play session |
| `zuzucoin` | INTEGER | — | 0 | Character currency (Zuzucoin) |

**Note**: Stat columns and `zuzucoin` (`str` through `stat_points`) and `max_health`/`max_mana` are added via `ALTER TABLE ... ADD COLUMN IF NOT EXISTS` on server startup if missing.

### `items`

| Column | Type | Constraints | Default | Description |
|--------|------|-------------|---------|-------------|
| `item_id` | SERIAL | PRIMARY KEY | auto | Item definition ID |
| `name` | VARCHAR(100) | NOT NULL | — | Item display name |
| `description` | TEXT | — | '' | Item description |
| `item_type` | VARCHAR(20) | NOT NULL | 'etc' | weapon/armor/garment/footgear/accessory/consumable/etc |
| `equip_slot` | VARCHAR(20) | nullable | NULL | head_top/head_mid/head_low/weapon/shield/armor/garment/footgear/accessory |
| `weight` | INTEGER | — | 0 | Item weight |
| `price` | INTEGER | — | 0 | NPC sell price (buy = price × 2) |
| `atk` | INTEGER | — | 0 | Weapon ATK bonus |
| `def` | INTEGER | — | 0 | Armor DEF bonus |
| `matk` | INTEGER | — | 0 | Magic ATK bonus |
| `mdef` | INTEGER | — | 0 | Magic DEF bonus |
| `str_bonus` | INTEGER | — | 0 | STR bonus when equipped |
| `agi_bonus` | INTEGER | — | 0 | AGI bonus when equipped |
| `vit_bonus` | INTEGER | — | 0 | VIT bonus when equipped |
| `int_bonus` | INTEGER | — | 0 | INT bonus when equipped |
| `dex_bonus` | INTEGER | — | 0 | DEX bonus when equipped |
| `luk_bonus` | INTEGER | — | 0 | LUK bonus when equipped |
| `max_hp_bonus` | INTEGER | — | 0 | Max HP bonus |
| `max_sp_bonus` | INTEGER | — | 0 | Max SP bonus |
| `required_level` | INTEGER | — | 1 | Required level to equip |
| `stackable` | BOOLEAN | — | false | Can stack in inventory |
| `max_stack` | INTEGER | — | 1 | Max stack size |
| `icon` | VARCHAR(100) | — | 'default_item' | Icon asset name |
| `weapon_type` | VARCHAR(20) | nullable | NULL | dagger/one_hand_sword/bow |
| `aspd_modifier` | INTEGER | — | 0 | ASPD modifier when equipped |
| `weapon_range` | INTEGER | — | 0 | Attack range in UE units |
| `hit_bonus` | INTEGER | — | 0 | Direct HIT bonus (equipment flat bonus, stacks with DEX-based HIT) |
| `flee_bonus` | INTEGER | — | 0 | Direct FLEE bonus (equipment flat bonus, stacks with AGI-based FLEE) |
| `critical_bonus` | INTEGER | — | 0 | Direct CRIT bonus (equipment flat bonus, stacks with LUK-based Crit) |

### `character_inventory`

| Column | Type | Constraints | Default | Description |
|--------|------|-------------|---------|-------------|
| `inventory_id` | SERIAL | PRIMARY KEY | auto | Inventory entry ID |
| `character_id` | INTEGER | FK → characters ON DELETE CASCADE | — | Owner character |
| `item_id` | INTEGER | FK → items(item_id) | — | Item reference |
| `quantity` | INTEGER | — | 1 | Stack count |
| `is_equipped` | BOOLEAN | — | false | Currently equipped |
| `slot_index` | INTEGER | — | -1 | Grid position (-1 = auto) |
| `created_at` | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | — | When acquired |

### `character_hotbar`

Persists hotbar slot assignments per character. The FK to `character_inventory` uses **ON DELETE CASCADE** — when an inventory item is deleted (fully consumed, dropped), the hotbar row auto-clears. No server-side cleanup code needed.

| Column | Type | Constraints | Default | Description |
|--------|------|-------------|---------|-------------|
| `character_id` | INTEGER | PK, FK → characters ON DELETE CASCADE | — | Owner character |
| `slot_index` | INTEGER | PK, CHECK (0–8) | — | Hotbar slot position (0-indexed) |
| `inventory_id` | INTEGER | NOT NULL, FK → character_inventory ON DELETE CASCADE | — | Inventory entry in this slot |
| `item_id` | INTEGER | NOT NULL | — | Item ID (denormalized for display) |
| `item_name` | VARCHAR(100) | NOT NULL | '' | Item name (denormalized for display) |

**Migration:** `database/migrations/add_character_hotbar.sql`

## Indexes

```sql
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_characters_user_id ON characters(user_id);
CREATE INDEX IF NOT EXISTS idx_inventory_character ON character_inventory(character_id);
CREATE INDEX IF NOT EXISTS idx_inventory_item ON character_inventory(item_id);
CREATE INDEX IF NOT EXISTS idx_hotbar_character ON character_hotbar(character_id);
```

## Seed Data

### Consumables (item_id 1001–1005)
| ID | Name | HP Restore | SP Restore | Price |
|----|------|-----------|-----------|-------|
| 1001 | Crimson Vial | 50 HP | — | 25 |
| 1002 | Amber Elixir | 150 HP | — | 100 |
| 1003 | Golden Salve | 350 HP | — | 275 |
| 1004 | Azure Philter | — | 60 SP | 500 |
| 1005 | Roasted Haunch | 70 HP | — | 25 |

### Loot/Etc Items (item_id 2001–2008)
| ID | Name | Weight | Price | Max Stack |
|----|------|--------|-------|-----------|
| 2001 | Gloopy Residue | 1 | 3 | 999 |
| 2002 | Viscous Slime | 1 | 7 | 999 |
| 2003 | Chitin Shard | 2 | 14 | 999 |
| 2004 | Downy Plume | 1 | 5 | 999 |
| 2005 | Spore Cluster | 1 | 10 | 999 |
| 2006 | Barbed Limb | 1 | 12 | 999 |
| 2007 | Verdant Leaf | 3 | 8 | 99 |
| 2008 | Silken Tuft | 1 | 4 | 999 |

### Weapons (item_id 3001–3006)
| ID | Name | Type | ATK | Range | ASPD Mod | Req Lvl |
|----|------|------|-----|-------|----------|---------|
| 3001 | Rustic Shiv | dagger | 17 | 150 | +5 | 1 |
| 3002 | Keen Edge | dagger | 30 | 150 | +5 | 1 |
| 3003 | Stiletto Fang | dagger | 43 | 150 | +5 | 12 |
| 3004 | Iron Cleaver | one_hand_sword | 25 | 150 | 0 | 2 |
| 3005 | Crescent Saber | one_hand_sword | 49 | 150 | 0 | 18 |
| 3006 | Hunting Longbow | bow | 35 | 800 | -3 | 4 |

### Armor (item_id 4001–4003)
| ID | Name | DEF | Weight | Req Lvl |
|----|------|-----|--------|---------|
| 4001 | Linen Tunic | 1 | 10 | 1 |
| 4002 | Quilted Vest | 4 | 80 | 1 |
| 4003 | Ringweave Hauberk | 8 | 150 | 20 |

## Redis Cache Schema

| Key Pattern | Value | TTL | Purpose |
|------------|-------|-----|---------|
| `player:{charId}:position` | `{"x","y","z","zone","timestamp"}` | 300s | Position cache for range checks |

## Auto-Migration on Startup

The server automatically ensures schema completeness on startup:

1. **Stat columns**: `ALTER TABLE characters ADD COLUMN IF NOT EXISTS str/agi/vit/int_stat/dex/luk/stat_points/max_health/max_mana`
2. **Items table**: `CREATE TABLE IF NOT EXISTS items (...)`
3. **Weapon columns**: `ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_type/aspd_modifier/weapon_range`
4a. **Derived bonus columns**: `ALTER TABLE items ADD COLUMN IF NOT EXISTS hit_bonus/flee_bonus/critical_bonus`
4. **Inventory table**: `CREATE TABLE IF NOT EXISTS character_inventory (...)`
5. **Indexes**: `CREATE INDEX IF NOT EXISTS ...`
6. **Seed data**: If `items` table is empty, insert all 16 base items

## Key Queries Used by Server

### Authentication
```sql
-- Register
INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING user_id, username, email, created_at;

-- Login
SELECT user_id, username, email, password_hash FROM users WHERE username = $1;

-- Update last login
UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE user_id = $1;
```

### Characters
```sql
-- List characters
SELECT character_id, name, class, level, x, y, z, health, mana, created_at FROM characters WHERE user_id = $1 ORDER BY created_at DESC;

-- Create character
INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana) VALUES ($1, $2, $3, 1, 0, 0, 0, 100, 100) RETURNING ...;

-- Load on join
SELECT x, y, z, health, max_health, mana, max_mana FROM characters WHERE character_id = $1;
SELECT str, agi, vit, int_stat, dex, luk, level, stat_points FROM characters WHERE character_id = $1;

-- Save position
UPDATE characters SET x = $1, y = $2, z = $3 WHERE character_id = $4;

-- Save health/mana
UPDATE characters SET health = $1, mana = $2 WHERE character_id = $3;

-- Save stats
UPDATE characters SET str=$1, agi=$2, vit=$3, int_stat=$4, dex=$5, luk=$6, stat_points=$7 WHERE character_id=$8;
```

### Inventory
```sql
-- Load inventory (with item details)
SELECT ci.*, i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
       i.atk, i.def, i.matk, i.mdef, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus,
       i.dex_bonus, i.luk_bonus, i.max_hp_bonus, i.max_sp_bonus, i.required_level,
       i.stackable, i.icon, i.weapon_type, i.aspd_modifier, i.weapon_range
FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id = $1 ORDER BY ci.slot_index ASC, ci.created_at ASC;

-- Add item (stackable check)
SELECT inventory_id, quantity FROM character_inventory WHERE character_id=$1 AND item_id=$2 AND is_equipped=false;
UPDATE character_inventory SET quantity=$1 WHERE inventory_id=$2;  -- Stack
INSERT INTO character_inventory (character_id, item_id, quantity) VALUES ($1,$2,$3) RETURNING inventory_id;  -- New entry

-- Equip weapon (load for join)
SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range FROM character_inventory ci
JOIN items i ON ci.item_id = i.item_id
WHERE ci.character_id=$1 AND ci.is_equipped=true AND i.equip_slot='weapon' LIMIT 1;

-- Equip/unequip
UPDATE character_inventory SET is_equipped=false WHERE character_id=$1 AND is_equipped=true AND item_id IN (SELECT item_id FROM items WHERE equip_slot=$2);
UPDATE character_inventory SET is_equipped=true WHERE inventory_id=$1;
```

---

**Last Updated**: 2026-02-21 — Fully renamed database column from zeny to zuzucoin
