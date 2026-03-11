-- Create database (run this manually in pgAdmin first)
-- CREATE DATABASE sabri_mmo;

-- Connect to sabri_mmo database, then run:

-- Users table
CREATE TABLE IF NOT EXISTS users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP
);

-- Characters table
CREATE TABLE IF NOT EXISTS characters (
    character_id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(user_id) ON DELETE CASCADE,
    name VARCHAR(50) NOT NULL UNIQUE,
    class VARCHAR(20) DEFAULT 'warrior',
    level INTEGER DEFAULT 1,
    experience INTEGER DEFAULT 0,
    x FLOAT DEFAULT 0,
    y FLOAT DEFAULT 0,
    z FLOAT DEFAULT 0,
    health INTEGER DEFAULT 100,
    max_health INTEGER DEFAULT 100,
    mana INTEGER DEFAULT 100,
    max_mana INTEGER DEFAULT 100,
    hair_style INTEGER DEFAULT 1,
    hair_color INTEGER DEFAULT 0,
    gender VARCHAR(10) DEFAULT 'male',
    delete_date TIMESTAMP DEFAULT NULL,
    deleted BOOLEAN NOT NULL DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_played TIMESTAMP
);

-- Stat columns (added by server on startup if missing)
-- str, agi, vit, int_stat, dex, luk, stat_points, max_health, max_mana

-- Items definition table (static item data — rAthena canonical IDs)
CREATE TABLE IF NOT EXISTS items (
    item_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    aegis_name VARCHAR(100) DEFAULT NULL,           -- rAthena internal name
    description TEXT DEFAULT '',
    full_description TEXT DEFAULT NULL,              -- Full tooltip text from rAthena
    item_type VARCHAR(20) NOT NULL DEFAULT 'etc',   -- weapon, armor, consumable, card, etc, ammo
    equip_slot VARCHAR(20) DEFAULT NULL,             -- head_top, head_mid, head_low, weapon, shield, armor, garment, footgear, accessory
    weight INTEGER DEFAULT 0,
    price INTEGER DEFAULT 0,                         -- Legacy sell price (use sell_price instead)
    buy_price INTEGER DEFAULT 0,                     -- NPC buy price
    sell_price INTEGER DEFAULT 0,                    -- NPC sell price
    atk INTEGER DEFAULT 0,                           -- Weapon ATK bonus
    def INTEGER DEFAULT 0,                           -- Armor DEF bonus
    matk INTEGER DEFAULT 0,                          -- Magic ATK bonus
    mdef INTEGER DEFAULT 0,                          -- Magic DEF bonus
    str_bonus INTEGER DEFAULT 0,
    agi_bonus INTEGER DEFAULT 0,
    vit_bonus INTEGER DEFAULT 0,
    int_bonus INTEGER DEFAULT 0,
    dex_bonus INTEGER DEFAULT 0,
    luk_bonus INTEGER DEFAULT 0,
    max_hp_bonus INTEGER DEFAULT 0,
    max_sp_bonus INTEGER DEFAULT 0,
    hit_bonus INTEGER DEFAULT 0,
    flee_bonus INTEGER DEFAULT 0,
    critical_bonus INTEGER DEFAULT 0,
    required_level INTEGER DEFAULT 1,
    stackable BOOLEAN DEFAULT false,
    max_stack INTEGER DEFAULT 1,
    icon VARCHAR(100) DEFAULT 'default_item',
    weapon_type VARCHAR(30) DEFAULT NULL,            -- dagger, one_hand_sword, bow, staff, etc.
    aspd_modifier FLOAT DEFAULT 0,
    weapon_range INTEGER DEFAULT 150,
    weapon_level INTEGER DEFAULT 0,                  -- 1-4 for weapons
    armor_level INTEGER DEFAULT 0,
    slots INTEGER DEFAULT 0,                         -- 0-4 card slots
    equip_level_min INTEGER DEFAULT 0,
    equip_level_max INTEGER DEFAULT 0,
    refineable BOOLEAN DEFAULT false,
    jobs_allowed TEXT DEFAULT 'All',
    classes_allowed TEXT DEFAULT 'All',
    gender_allowed VARCHAR(10) DEFAULT 'Both',
    equip_locations TEXT DEFAULT NULL,
    script TEXT DEFAULT NULL,                        -- rAthena raw script (stat bonuses, etc.)
    equip_script TEXT DEFAULT NULL,                  -- rAthena equip script
    unequip_script TEXT DEFAULT NULL,                -- rAthena unequip script
    sub_type VARCHAR(30) DEFAULT NULL,
    view_sprite INTEGER DEFAULT 0,
    two_handed BOOLEAN DEFAULT false,
    element VARCHAR(10) DEFAULT 'neutral',
    card_type VARCHAR(20) DEFAULT NULL,
    card_prefix VARCHAR(50) DEFAULT NULL,
    card_suffix VARCHAR(50) DEFAULT NULL,
    class_restrictions TEXT DEFAULT NULL,
    perfect_dodge_bonus INTEGER DEFAULT 0,
    ammo_type VARCHAR(20) DEFAULT NULL
);

-- Character inventory table
CREATE TABLE IF NOT EXISTS character_inventory (
    inventory_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    is_equipped BOOLEAN DEFAULT false,
    slot_index INTEGER DEFAULT -1,  -- Position in inventory grid (-1 = auto)
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Hotbar slot assignments (persisted per character)
-- ON DELETE CASCADE: when inventory item is deleted (consumed/dropped), hotbar slot auto-clears
CREATE TABLE IF NOT EXISTS character_hotbar (
    character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    slot_index INTEGER NOT NULL CHECK (slot_index >= 0 AND slot_index <= 8),
    inventory_id INTEGER NOT NULL REFERENCES character_inventory(inventory_id) ON DELETE CASCADE,
    item_id INTEGER NOT NULL,
    item_name VARCHAR(100) NOT NULL DEFAULT '',
    PRIMARY KEY (character_id, slot_index)
);

-- Simple indexes
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_characters_user_id ON characters(user_id);
CREATE INDEX IF NOT EXISTS idx_characters_deleted ON characters(deleted) WHERE deleted = FALSE;
CREATE INDEX IF NOT EXISTS idx_inventory_character ON character_inventory(character_id);
CREATE INDEX IF NOT EXISTS idx_inventory_item ON character_inventory(item_id);

-- ============================================================
-- Seed Data: Canonical rAthena Items (6,169 items)
-- ============================================================
-- For fresh installs, run the canonical items SQL after this file:
--   \i scripts/output/canonical_items.sql
--
-- The canonical_items.sql contains all 6,169 rAthena pre-renewal items
-- with full data: stats, prices, scripts, descriptions, weapon types, etc.
-- It uses ON CONFLICT (item_id) DO UPDATE to safely merge with any existing data.
--
-- For existing databases migrating from custom IDs, run instead:
--   \i database/migrations/migrate_to_canonical_ids.sql
--   \i scripts/output/canonical_items.sql
