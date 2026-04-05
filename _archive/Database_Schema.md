# Database Schema

## Overview

PostgreSQL database schema for the MMO backend. Supports user authentication, character storage, and position persistence.

## Tables

### users

Stores user account information with bcrypt password hashing.

```sql
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);
```

**Fields:**
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| user_id | SERIAL | PRIMARY KEY | Auto-incrementing ID |
| username | VARCHAR(50) | UNIQUE, NOT NULL | User login name (3-50 chars) |
| email | VARCHAR(100) | UNIQUE, NOT NULL | User email address |
| password_hash | VARCHAR(255) | NOT NULL | bcrypt hash (60 chars) |
| created_at | TIMESTAMP | DEFAULT NOW() | Account creation time |
| last_login | TIMESTAMP | NULLABLE | Last successful login |

**Indexes:**
- PRIMARY KEY on user_id
- UNIQUE on username
- UNIQUE on email

---

### characters

Stores character data including position for world persistence.

```sql
CREATE TABLE characters (
    character_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id),
    name VARCHAR(50) NOT NULL,
    class VARCHAR(20) DEFAULT 'warrior',
    level INTEGER DEFAULT 1,
    x FLOAT DEFAULT 0,
    y FLOAT DEFAULT 0,
    z FLOAT DEFAULT 0,
    health INTEGER DEFAULT 100,
    mana INTEGER DEFAULT 100,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Fields:**
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| character_id | SERIAL | PRIMARY KEY | Auto-incrementing ID |
| user_id | INTEGER | FOREIGN KEY → users | Owner of character |
| name | VARCHAR(50) | NOT NULL | Character name (2-50 chars) |
| class | VARCHAR(20) | DEFAULT 'warrior' | Character class |
| level | INTEGER | DEFAULT 1 | Character level |
| x | FLOAT | DEFAULT 0 | X coordinate in world |
| y | FLOAT | DEFAULT 0 | Y coordinate in world |
| z | FLOAT | DEFAULT 0 | Z coordinate in world |
| health | INTEGER | DEFAULT 100 | Current/max health |
| mana | INTEGER | DEFAULT 100 | Current/max mana |
| created_at | TIMESTAMP | DEFAULT NOW() | Character creation time |

**Valid Classes:**
- `warrior` - High health, melee combat
- `mage` - High mana, magical abilities
- `archer` - Ranged combat
- `healer` - Support abilities

**Indexes:**
- PRIMARY KEY on character_id
- FOREIGN KEY on user_id (cascading delete)

---

### items

Static item definitions — weapons, armor, consumables, loot materials.

```sql
CREATE TABLE items (
    item_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT DEFAULT '',
    item_type VARCHAR(20) NOT NULL DEFAULT 'etc',
    equip_slot VARCHAR(20) DEFAULT NULL,
    weight INTEGER DEFAULT 0,
    price INTEGER DEFAULT 0,
    atk INTEGER DEFAULT 0,
    def INTEGER DEFAULT 0,
    matk INTEGER DEFAULT 0,
    mdef INTEGER DEFAULT 0,
    str_bonus INTEGER DEFAULT 0,
    agi_bonus INTEGER DEFAULT 0,
    vit_bonus INTEGER DEFAULT 0,
    int_bonus INTEGER DEFAULT 0,
    dex_bonus INTEGER DEFAULT 0,
    luk_bonus INTEGER DEFAULT 0,
    max_hp_bonus INTEGER DEFAULT 0,
    max_sp_bonus INTEGER DEFAULT 0,
    required_level INTEGER DEFAULT 1,
    stackable BOOLEAN DEFAULT false,
    max_stack INTEGER DEFAULT 1,
    icon VARCHAR(100) DEFAULT 'default_item',
    weapon_type VARCHAR(20) DEFAULT NULL,
    aspd_modifier INTEGER DEFAULT 0,
    weapon_range INTEGER DEFAULT 0
);
```

**item_type values:** weapon, armor, garment, footgear, accessory, consumable, etc
**equip_slot values:** weapon, shield, armor, garment, footgear, accessory, head_top, head_mid, head_low
**weapon_type values:** dagger (+5 ASPD, 150 range), one_hand_sword (+0 ASPD, 150 range), bow (-3 ASPD, 800 range)

---

### character_inventory

Per-character item ownership, quantities, and equipped state.

```sql
CREATE TABLE character_inventory (
    inventory_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    is_equipped BOOLEAN DEFAULT false,
    slot_index INTEGER DEFAULT -1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**Indexes:**
- `idx_inventory_character` on character_id
- `idx_inventory_item` on item_id

---

## Entity Relationships

```
┌───────────────┐         ┌──────────────────┐         ┌──────────────────────┐
│     users     │         │    characters    │         │ character_inventory  │
├───────────────┤         ├──────────────────┤         ├──────────────────────┤
│ PK user_id    │◄───────┼ FK user_id       │◄───────┼ FK character_id      │
│    username   │    1:M │ PK character_id  │    1:M │ PK inventory_id      │
│    email      │         │    name          │         │ FK item_id ──────────┼──┐
│    password   │         │    class         │         │    quantity          │  │
│    created_at │         │    level         │         │    is_equipped       │  │
│    last_login │         │    x, y, z       │         │    slot_index        │  │
└───────────────┘         │    stats...      │         └──────────────────────┘  │
                          └──────────────────┘                                   │
                                                        ┌───────────────┐        │
                                                        │     items     │◄───────┘
                                                        ├───────────────┤
                                                        │ PK item_id    │
                                                        │    name       │
                                                        │    item_type  │
                                                        │    equip_slot │
                                                        │    atk/def    │
                                                        │    weapon_type│
                                                        │    weapon_range│
                                                        │    aspd_mod   │
                                                        └───────────────┘
```

- **One-to-Many**: One user can have multiple characters
- **One-to-Many**: One character can have multiple inventory entries
- **Many-to-One**: Many inventory entries reference one item definition
- **Cascade Delete**: Deleting user deletes all their characters and inventory

## Common Queries

### User Registration

```sql
INSERT INTO users (username, email, password_hash) 
VALUES ($1, $2, $3) 
RETURNING user_id, username, email, created_at;
```

### User Login

```sql
-- Get user by username
SELECT user_id, username, email, password_hash 
FROM users 
WHERE username = $1;

-- Update last_login on success
UPDATE users 
SET last_login = CURRENT_TIMESTAMP 
WHERE user_id = $1;
```

### Get User's Characters

```sql
SELECT character_id, name, class, level, x, y, z, health, mana, created_at 
FROM characters 
WHERE user_id = $1 
ORDER BY created_at DESC;
```

### Create Character

```sql
INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana) 
VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
RETURNING character_id, name, class, level, x, y, z, health, mana, created_at;
```

**Default Values:**
- level: 1
- x, y, z: 0
- health: 100
- mana: 100

### Update Character Position

```sql
UPDATE characters 
SET x = $1, y = $2, z = $3 
WHERE character_id = $4 AND user_id = $5;
```

### Check Character Ownership

```sql
SELECT character_id 
FROM characters 
WHERE character_id = $1 AND user_id = $2;
```

### Check Duplicate Character Name

```sql
SELECT character_id 
FROM characters 
WHERE user_id = $1 AND name = $2;
```

## Sample Data

### Users

| user_id | username | email | password_hash | created_at |
|---------|----------|-------|---------------|------------|
| 1 | admin | admin@sabrimmo.com | $2b$10$... | 2026-02-01 |
| 2 | testplayer | test@example.com | $2b$10$... | 2026-02-01 |

### Characters

| character_id | user_id | name | class | level | x | y | z |
|--------------|---------|------|-------|-------|---|---|---|
| 10 | 2 | MyHero | warrior | 1 | 123.45 | 67.89 | 0.0 |
| 11 | 2 | MagicMan | mage | 1 | 0.0 | 0.0 | 0.0 |

## Migration Strategy

### Adding New Tables

```sql
-- Create new table
CREATE TABLE new_table (
    id SERIAL PRIMARY KEY,
    -- fields
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Add foreign key if needed
ALTER TABLE new_table 
ADD COLUMN character_id INTEGER REFERENCES characters(character_id);
```

### Adding New Columns

```sql
-- Add column with default
ALTER TABLE characters 
ADD COLUMN experience INTEGER DEFAULT 0;

-- Add nullable column
ALTER TABLE users 
ADD COLUMN display_name VARCHAR(50);
```

### Creating Indexes

```sql
-- Index for frequent lookups
CREATE INDEX idx_characters_user_id ON characters(user_id);

-- Index for name searches
CREATE INDEX idx_characters_name ON characters(name);
```

## Backup Strategy

### Full Backup

```bash
pg_dump -U postgres -d sabri_mmo > backup.sql
```

### Restore

```bash
psql -U postgres -d sabri_mmo < backup.sql
```

## Performance Considerations

### Current Optimizations
- Primary keys on all tables
- Unique indexes on username/email
- Foreign key indexes (auto-created)

### Future Optimizations
- Connection pooling (PgBouncer)
- Read replicas for heavy queries
- Partitioning for large tables

## Files

- `database/init.sql` - Initial schema creation
