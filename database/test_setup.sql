-- Database setup for UI testing
-- Run with: psql -d sabri_mmo -f database/test_setup.sql
-- Safe to re-run: deletes and recreates test data each time

-- Ensure stat columns exist (server creates these dynamically on startup)
DO $$
BEGIN
    ALTER TABLE characters 
    ADD COLUMN IF NOT EXISTS str INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS agi INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS vit INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS int_stat INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS dex INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS luk INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS stat_points INTEGER DEFAULT 48,
    ADD COLUMN IF NOT EXISTS zuzucoin INTEGER DEFAULT 0;
EXCEPTION
    WHEN OTHERS THEN
        RAISE NOTICE 'Stat columns may already exist';
END $$;

-- Clean up any previous test data (safe re-run)
DO $$
DECLARE
    old_char_id integer;
BEGIN
    SELECT character_id INTO old_char_id FROM characters WHERE name = 'TestPlayer_UI';
    IF old_char_id IS NOT NULL THEN
        DELETE FROM character_hotbar WHERE character_id = old_char_id;
        DELETE FROM character_inventory WHERE character_id = old_char_id;
        DELETE FROM characters WHERE character_id = old_char_id;
    END IF;
    DELETE FROM users WHERE username = 'test_ui_user';
END $$;

-- Create test user
INSERT INTO users (username, email, password_hash, created_at)
VALUES ('test_ui_user', 'test_ui@example.com', '$2b$10$mock.hash.for.testing.only', NOW());

-- Create test character with stats
DO $$
DECLARE
    v_user_id integer;
    v_char_id integer;
BEGIN
    SELECT user_id INTO v_user_id FROM users WHERE username = 'test_ui_user';

    INSERT INTO characters (
        user_id, name, class, level,
        x, y, z, health, max_health, mana, max_mana,
        str, agi, vit, int_stat, dex, luk,
        stat_points, zuzucoin, created_at
    ) VALUES (
        v_user_id, 'TestPlayer_UI', 'warrior', 10,
        0, 0, 300, 500, 500, 200, 200,
        20, 15, 18, 10, 12, 8,
        20, 5000, NOW()
    )
    RETURNING character_id INTO v_char_id;

    -- Create test inventory using REAL item IDs from init.sql seed data
    --   3004 = Iron Cleaver (weapon, atk 25)
    --   1001 = Crimson Vial  (consumable, stackable)
    --   4001 = Linen Tunic   (armor, def 1)
    INSERT INTO character_inventory (character_id, item_id, quantity, is_equipped, slot_index)
    VALUES
        (v_char_id, 3004, 1, false, -1),  -- Iron Cleaver (weapon, unequipped)
        (v_char_id, 1001, 5, false, -1),  -- Crimson Vial x5 (consumable)
        (v_char_id, 4001, 1, true,  -1);  -- Linen Tunic (armor, equipped)

    RAISE NOTICE '✅ Test character created: TestPlayer_UI (character_id=%)', v_char_id;
    RAISE NOTICE '✅ Test inventory: 3 items (Iron Cleaver, 5x Crimson Vial, Linen Tunic equipped)';
    RAISE NOTICE '✅ Test user: test_ui_user (user_id=%)', v_user_id;
END $$;

-- Verify
DO $$
DECLARE
    v_char_id integer;
    v_inv_count integer;
    v_zuzucoin integer;
BEGIN
    SELECT character_id, zuzucoin INTO v_char_id, v_zuzucoin
    FROM characters WHERE name = 'TestPlayer_UI';

    SELECT COUNT(*) INTO v_inv_count
    FROM character_inventory WHERE character_id = v_char_id;

    RAISE NOTICE '--- Verification ---';
    RAISE NOTICE '  character_id : %', v_char_id;
    RAISE NOTICE '  zuzucoin     : %', v_zuzucoin;
    RAISE NOTICE '  inventory    : % items', v_inv_count;
END $$;

/*
Shops are defined in-memory in server/src/index.js (NPC_SHOPS constant):
  Shop 1 = General Store  (items: 1001-1005, 4001-4003)
  Shop 2 = Weapon Shop     (items: 3001-3006)
No shops table exists in the database.

To verify manually:
  SELECT c.character_id, c.name, c.zuzucoin FROM characters c WHERE c.name = 'TestPlayer_UI';
  SELECT ci.inventory_id, i.name, ci.quantity, ci.is_equipped
    FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
    WHERE ci.character_id = (SELECT character_id FROM characters WHERE name = 'TestPlayer_UI');
*/
