-- Migration: Cart Inventory, Vending, Item Identification
-- Date: 2026-03-16
-- Systems: Pushcart (604), Vending (605), Item Appraisal (606), Change Cart (607)

-- 1. Cart state columns on characters
ALTER TABLE characters ADD COLUMN IF NOT EXISTS has_cart BOOLEAN DEFAULT false;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS cart_type INTEGER DEFAULT 0;

-- 2. Identified column on character_inventory
ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS identified BOOLEAN DEFAULT true;

-- 3. Cart inventory table (separate from character_inventory)
CREATE TABLE IF NOT EXISTS character_cart (
    cart_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    item_id INTEGER REFERENCES items(item_id),
    quantity INTEGER DEFAULT 1,
    slot_index INTEGER DEFAULT -1,
    identified BOOLEAN DEFAULT true,
    refine_level INTEGER DEFAULT 0,
    compounded_cards JSONB DEFAULT '[]',
    forged_by VARCHAR(50) DEFAULT NULL,
    forged_element VARCHAR(10) DEFAULT NULL,
    forged_star_crumbs INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_cart_character ON character_cart(character_id);

-- 4. Vending shops table
CREATE TABLE IF NOT EXISTS vending_shops (
    shop_id SERIAL PRIMARY KEY,
    character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
    title VARCHAR(80) NOT NULL DEFAULT 'Shop',
    zone VARCHAR(50) NOT NULL,
    x REAL NOT NULL,
    y REAL NOT NULL,
    z REAL DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_vending_zone ON vending_shops(zone);

-- 5. Vending items table
CREATE TABLE IF NOT EXISTS vending_items (
    vend_item_id SERIAL PRIMARY KEY,
    shop_id INTEGER REFERENCES vending_shops(shop_id) ON DELETE CASCADE,
    cart_id INTEGER NOT NULL,
    item_id INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    price INTEGER NOT NULL CHECK (price > 0 AND price <= 1000000000)
);

-- 6. Ensure Magnifier item exists in items table (Tool Dealer consumable for identification)
INSERT INTO items (item_id, name, description, item_type, weight, price, buy_price, sell_price, stackable, icon)
VALUES (611, 'Magnifier', 'A magnifying glass used to identify unknown items.', 'usable', 5, 40, 40, 20, true, 'magnifier')
ON CONFLICT (item_id) DO NOTHING;
