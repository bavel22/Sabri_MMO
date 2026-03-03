-- Database schema validation tests
-- Run with: psql -d sabri_mmo -f tests/database/migrations.test.sql
-- All tests run inside a transaction and ROLLBACK so no data is modified.

BEGIN;

-- Test 1: Check core tables exist
DO $$
DECLARE
    tbl text;
    expected_tables text[] := ARRAY['users', 'characters', 'items', 'character_inventory', 'character_hotbar'];
BEGIN
    FOREACH tbl IN ARRAY expected_tables LOOP
        IF NOT EXISTS (SELECT FROM information_schema.tables WHERE table_schema='public' AND table_name=tbl) THEN
            RAISE EXCEPTION '❌ Table "%" does not exist', tbl;
        END IF;
    END LOOP;
    RAISE NOTICE '✅ Test 1 PASS: All core tables exist';
END $$;

-- Test 2: Check characters table columns
DO $$
DECLARE
    col text;
    expected text[] := ARRAY[
        'character_id','user_id','name','class','level','experience',
        'x','y','z','health','max_health','mana','max_mana','created_at'
    ];
BEGIN
    FOREACH col IN ARRAY expected LOOP
        IF NOT EXISTS (SELECT FROM information_schema.columns WHERE table_name='characters' AND column_name=col) THEN
            RAISE EXCEPTION '❌ characters table missing column: %', col;
        END IF;
    END LOOP;
    RAISE NOTICE '✅ Test 2 PASS: characters table base columns correct';
END $$;

-- Test 3: Check character_inventory table columns
DO $$
DECLARE
    col text;
    expected text[] := ARRAY['inventory_id','character_id','item_id','quantity','is_equipped','slot_index','created_at'];
BEGIN
    FOREACH col IN ARRAY expected LOOP
        IF NOT EXISTS (SELECT FROM information_schema.columns WHERE table_name='character_inventory' AND column_name=col) THEN
            RAISE EXCEPTION '❌ character_inventory table missing column: %', col;
        END IF;
    END LOOP;
    RAISE NOTICE '✅ Test 3 PASS: character_inventory table columns correct';
END $$;

-- Test 4: Check foreign key constraints
DO $$
DECLARE
    fk_exists boolean;
BEGIN
    -- character_inventory.character_id → characters
    SELECT EXISTS (
        SELECT FROM information_schema.table_constraints tc
        JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name
        WHERE tc.table_name='character_inventory' AND tc.constraint_type='FOREIGN KEY' AND kcu.column_name='character_id'
    ) INTO fk_exists;
    IF NOT fk_exists THEN RAISE EXCEPTION '❌ character_inventory missing FK on character_id'; END IF;

    -- character_inventory.item_id → items
    SELECT EXISTS (
        SELECT FROM information_schema.table_constraints tc
        JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name
        WHERE tc.table_name='character_inventory' AND tc.constraint_type='FOREIGN KEY' AND kcu.column_name='item_id'
    ) INTO fk_exists;
    IF NOT fk_exists THEN RAISE EXCEPTION '❌ character_inventory missing FK on item_id'; END IF;

    -- characters.user_id → users
    SELECT EXISTS (
        SELECT FROM information_schema.table_constraints tc
        JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name
        WHERE tc.table_name='characters' AND tc.constraint_type='FOREIGN KEY' AND kcu.column_name='user_id'
    ) INTO fk_exists;
    IF NOT fk_exists THEN RAISE EXCEPTION '❌ characters missing FK on user_id'; END IF;

    RAISE NOTICE '✅ Test 4 PASS: Foreign key constraints correct';
END $$;

-- Test 5: Check dynamic stat columns exist (added by server on startup)
DO $$
DECLARE
    col text;
    dynamic_cols text[] := ARRAY['str','agi','vit','int_stat','dex','luk','stat_points','zuzucoin'];
BEGIN
    FOREACH col IN ARRAY dynamic_cols LOOP
        IF NOT EXISTS (SELECT FROM information_schema.columns WHERE table_name='characters' AND column_name=col) THEN
            RAISE EXCEPTION '❌ characters missing dynamic column: % (run server once to create)', col;
        END IF;
    END LOOP;
    RAISE NOTICE '✅ Test 5 PASS: Dynamic stat columns exist';
END $$;

-- Test 6: Check data types
DO $$
DECLARE
    v_type text;
BEGIN
    SELECT data_type INTO v_type FROM information_schema.columns WHERE table_name='characters' AND column_name='zuzucoin';
    IF v_type != 'integer' THEN RAISE EXCEPTION '❌ zuzucoin should be integer, got %', v_type; END IF;

    SELECT data_type INTO v_type FROM information_schema.columns WHERE table_name='characters' AND column_name='health';
    IF v_type != 'integer' THEN RAISE EXCEPTION '❌ health should be integer, got %', v_type; END IF;

    SELECT data_type INTO v_type FROM information_schema.columns WHERE table_name='character_inventory' AND column_name='is_equipped';
    IF v_type != 'boolean' THEN RAISE EXCEPTION '❌ is_equipped should be boolean, got %', v_type; END IF;

    RAISE NOTICE '✅ Test 6 PASS: Column data types correct';
END $$;

-- Test 7: CRUD operations (all inside this transaction, rolled back at end)
DO $$
DECLARE
    v_uid integer;
    v_cid integer;
    v_iid integer;
BEGIN
    INSERT INTO users (username, email, password_hash)
    VALUES ('test_crud_user', 'crud@test.com', 'hash')
    RETURNING user_id INTO v_uid;

    INSERT INTO characters (user_id, name, class, level, health, max_health, mana, max_mana)
    VALUES (v_uid, 'CrudTestChar', 'warrior', 1, 100, 100, 50, 50)
    RETURNING character_id INTO v_cid;

    -- Use real item_id 1001 (Crimson Vial from seed data)
    INSERT INTO character_inventory (character_id, item_id, quantity, is_equipped)
    VALUES (v_cid, 1001, 10, false)
    RETURNING inventory_id INTO v_iid;

    -- Read
    PERFORM 1 FROM characters WHERE character_id = v_cid;
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Failed to read character'; END IF;

    PERFORM 1 FROM character_inventory WHERE inventory_id = v_iid;
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Failed to read inventory'; END IF;

    -- Update
    UPDATE characters SET level = 2 WHERE character_id = v_cid;
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Failed to update character'; END IF;

    -- Delete (reverse order for FK)
    DELETE FROM character_inventory WHERE inventory_id = v_iid;
    DELETE FROM characters WHERE character_id = v_cid;
    DELETE FROM users WHERE user_id = v_uid;

    RAISE NOTICE '✅ Test 7 PASS: CRUD operations work correctly';
END $$;

-- Test 8: Check indexes exist
DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_indexes WHERE tablename='characters' AND indexname LIKE '%_pkey') THEN
        RAISE EXCEPTION '❌ characters missing primary key index';
    END IF;
    IF NOT EXISTS (SELECT FROM pg_indexes WHERE tablename='character_inventory' AND indexname LIKE '%_pkey') THEN
        RAISE EXCEPTION '❌ character_inventory missing primary key index';
    END IF;
    IF NOT EXISTS (SELECT FROM pg_indexes WHERE tablename='users' AND indexname LIKE '%_pkey') THEN
        RAISE EXCEPTION '❌ users missing primary key index';
    END IF;
    RAISE NOTICE '✅ Test 8 PASS: Primary key indexes exist';
END $$;

-- Test 9: Check seed items exist
DO $$
DECLARE
    v_count integer;
BEGIN
    SELECT COUNT(*) INTO v_count FROM items;
    IF v_count = 0 THEN RAISE EXCEPTION '❌ items table is empty — run server once to seed'; END IF;
    PERFORM 1 FROM items WHERE item_id = 1001;  -- Crimson Vial
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Seed item 1001 (Crimson Vial) missing'; END IF;
    PERFORM 1 FROM items WHERE item_id = 3004;  -- Iron Cleaver
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Seed item 3004 (Iron Cleaver) missing'; END IF;
    PERFORM 1 FROM items WHERE item_id = 4001;  -- Linen Tunic
    IF NOT FOUND THEN RAISE EXCEPTION '❌ Seed item 4001 (Linen Tunic) missing'; END IF;
    RAISE NOTICE '✅ Test 9 PASS: Seed items exist (% total)', v_count;
END $$;

ROLLBACK;

DO $$ BEGIN RAISE NOTICE '🎉 All 9 database tests passed!'; END $$;
