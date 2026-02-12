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
    name VARCHAR(50) NOT NULL,
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
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_played TIMESTAMP
);

-- Stat columns (added by server on startup if missing)
-- str, agi, vit, int_stat, dex, luk, stat_points, max_health, max_mana

-- Items definition table (static item data)
CREATE TABLE IF NOT EXISTS items (
    item_id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description TEXT DEFAULT '',
    item_type VARCHAR(20) NOT NULL DEFAULT 'etc',  -- weapon, armor, garment, footgear, accessory, consumable, etc
    equip_slot VARCHAR(20) DEFAULT NULL,            -- head_top, head_mid, head_low, weapon, shield, armor, garment, footgear, accessory
    weight INTEGER DEFAULT 0,
    price INTEGER DEFAULT 0,                        -- NPC sell price (buy = price * 2)
    atk INTEGER DEFAULT 0,                          -- Weapon ATK bonus
    def INTEGER DEFAULT 0,                          -- Armor DEF bonus
    matk INTEGER DEFAULT 0,                         -- Magic ATK bonus
    mdef INTEGER DEFAULT 0,                         -- Magic DEF bonus
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
    icon VARCHAR(100) DEFAULT 'default_item'
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

-- Simple indexes
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_characters_user_id ON characters(user_id);
CREATE INDEX IF NOT EXISTS idx_inventory_character ON character_inventory(character_id);
CREATE INDEX IF NOT EXISTS idx_inventory_item ON character_inventory(item_id);

-- ============================================================
-- Seed Data: Base Items
-- ============================================================

-- Consumables
INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, max_stack, icon) VALUES
(1001, 'Crimson Vial', 'A small red tonic. Restores 50 HP.', 'consumable', 7, 25, true, 99, 'red_potion'),
(1002, 'Amber Elixir', 'A warm orange draught. Restores 150 HP.', 'consumable', 10, 100, true, 99, 'orange_potion'),
(1003, 'Golden Salve', 'A potent yellow remedy. Restores 350 HP.', 'consumable', 13, 275, true, 99, 'yellow_potion'),
(1004, 'Azure Philter', 'A calming blue brew. Restores 60 SP.', 'consumable', 15, 500, true, 99, 'blue_potion'),
(1005, 'Roasted Haunch', 'Tender roasted meat. Restores 70 HP.', 'consumable', 15, 25, true, 99, 'meat')
ON CONFLICT (item_id) DO NOTHING;

-- Etc / Loot items (dropped by monsters, sold to NPCs)
INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, max_stack, icon) VALUES
(2001, 'Gloopy Residue', 'A small, squishy blob of unknown origin.', 'etc', 1, 3, true, 999, 'jellopy'),
(2002, 'Viscous Slime', 'Thick, gooey substance secreted by monsters.', 'etc', 1, 7, true, 999, 'sticky_mucus'),
(2003, 'Chitin Shard', 'A hard, protective outer shell fragment.', 'etc', 2, 14, true, 999, 'shell'),
(2004, 'Downy Plume', 'A light, downy feather.', 'etc', 1, 5, true, 999, 'feather'),
(2005, 'Spore Cluster', 'Tiny spores from a forest mushroom.', 'etc', 1, 10, true, 999, 'mushroom_spore'),
(2006, 'Barbed Limb', 'A chitinous leg from a large insect.', 'etc', 1, 12, true, 999, 'insect_leg'),
(2007, 'Verdant Leaf', 'A common medicinal herb with healing properties.', 'etc', 3, 8, true, 99, 'green_herb'),
(2008, 'Silken Tuft', 'Soft, fluffy material from a plant creature.', 'etc', 1, 4, true, 999, 'fluff')
ON CONFLICT (item_id) DO NOTHING;

-- Weapons (weapon_type, aspd_modifier, weapon_range included)
-- Melee range: 150 UU | Bow range: 800 UU
-- ASPD modifiers: dagger +5, 1h_sword +0, bow -3
INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, atk, required_level, icon, weapon_type, aspd_modifier, weapon_range) VALUES
(3001, 'Rustic Shiv', 'A crude but swift dagger. Better than bare fists.', 'weapon', 'weapon', 40, 50, 17, 1, 'knife', 'dagger', 5, 150),
(3002, 'Keen Edge', 'A finely honed short blade.', 'weapon', 'weapon', 40, 150, 30, 1, 'cutter', 'dagger', 5, 150),
(3003, 'Stiletto Fang', 'An elegant parrying dagger with a needle-thin tip.', 'weapon', 'weapon', 60, 500, 43, 12, 'main_gauche', 'dagger', 5, 150),
(3004, 'Iron Cleaver', 'A standard one-handed sword with a broad blade.', 'weapon', 'weapon', 80, 100, 25, 2, 'sword', 'one_hand_sword', 0, 150),
(3005, 'Crescent Saber', 'A curved, heavy cutting sword.', 'weapon', 'weapon', 60, 600, 49, 18, 'falchion', 'one_hand_sword', 0, 150),
(3006, 'Hunting Longbow', 'A sturdy ranged bow with impressive reach.', 'weapon', 'weapon', 50, 400, 35, 4, 'bow', 'bow', -3, 800)
ON CONFLICT (item_id) DO NOTHING;

-- Armor
INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, def, required_level, icon) VALUES
(4001, 'Linen Tunic', 'A simple linen shirt offering minimal protection.', 'armor', 'armor', 10, 20, 1, 1, 'cotton_shirt'),
(4002, 'Quilted Vest', 'Light quilted padding for basic protection.', 'armor', 'armor', 80, 200, 4, 1, 'padded_armor'),
(4003, 'Ringweave Hauberk', 'Interlocking metal rings forged into sturdy armor.', 'armor', 'armor', 150, 800, 8, 20, 'chain_mail')
ON CONFLICT (item_id) DO NOTHING;
