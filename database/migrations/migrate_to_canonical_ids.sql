-- ============================================================
-- Migration: Custom Item IDs → rAthena Canonical IDs
-- ============================================================
-- This migration:
-- 1. Adds new columns to the items table for rAthena data
-- 2. Drops FK constraints temporarily
-- 3. Remaps existing custom item IDs to rAthena canonical IDs
-- 4. Inserts all 6,169 rAthena canonical items
-- 5. Restores FK constraints
-- 6. Updates sequences
--
-- ALWAYS BACKUP YOUR DATABASE BEFORE RUNNING THIS MIGRATION!
-- pg_dump -U postgres sabri_mmo > sabri_mmo_backup.sql
-- ============================================================

BEGIN;

-- ============================================================
-- Step 1a: Add missing columns that server code already queries
-- ============================================================
ALTER TABLE items ADD COLUMN IF NOT EXISTS hit_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS flee_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS critical_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS perfect_dodge_bonus INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS two_handed BOOLEAN DEFAULT false;
ALTER TABLE items ADD COLUMN IF NOT EXISTS element VARCHAR(10) DEFAULT 'neutral';
ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_type VARCHAR(30) DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS aspd_modifier FLOAT DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_range INTEGER DEFAULT 150;

-- ============================================================
-- Step 1b: Add new rAthena columns
-- ============================================================
ALTER TABLE items ADD COLUMN IF NOT EXISTS aegis_name VARCHAR(100);
ALTER TABLE items ADD COLUMN IF NOT EXISTS full_description TEXT DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS buy_price INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS sell_price INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS weapon_level INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS armor_level INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS slots INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS equip_level_min INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS equip_level_max INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS refineable BOOLEAN DEFAULT false;
ALTER TABLE items ADD COLUMN IF NOT EXISTS jobs_allowed TEXT DEFAULT 'All';
ALTER TABLE items ADD COLUMN IF NOT EXISTS classes_allowed TEXT DEFAULT 'All';
ALTER TABLE items ADD COLUMN IF NOT EXISTS gender_allowed VARCHAR(10) DEFAULT 'Both';
ALTER TABLE items ADD COLUMN IF NOT EXISTS equip_locations TEXT DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS script TEXT DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS equip_script TEXT DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS unequip_script TEXT DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS sub_type VARCHAR(30) DEFAULT NULL;
ALTER TABLE items ADD COLUMN IF NOT EXISTS view_sprite INTEGER DEFAULT 0;

-- Populate buy_price and sell_price from existing price column
UPDATE items SET sell_price = price, buy_price = price * 2 WHERE sell_price = 0 AND price > 0;

-- ============================================================
-- Step 2: Drop FK constraints temporarily
-- ============================================================
ALTER TABLE character_inventory DROP CONSTRAINT IF EXISTS character_inventory_item_id_fkey;
ALTER TABLE character_hotbar DROP CONSTRAINT IF EXISTS character_hotbar_item_id_fkey;

-- ============================================================
-- Step 3: Remap existing custom item IDs to canonical IDs
-- ============================================================
-- Process in reverse order of target IDs to avoid conflicts
-- When multiple old IDs map to the same new ID, keep inventory from
-- the first one and delete the duplicate item definition

-- Helper: for items where multiple old IDs map to same new ID,
-- we remap inventory/hotbar rows from ALL old IDs to the new ID,
-- then delete the duplicate item definitions.

-- === Consumables ===
UPDATE character_inventory SET item_id = 501 WHERE item_id = 1001;
UPDATE character_hotbar SET item_id = 501 WHERE item_id = 1001;
UPDATE character_inventory SET item_id = 502 WHERE item_id = 1002;
UPDATE character_hotbar SET item_id = 502 WHERE item_id = 1002;
UPDATE character_inventory SET item_id = 503 WHERE item_id = 1003;
UPDATE character_hotbar SET item_id = 503 WHERE item_id = 1003;
UPDATE character_inventory SET item_id = 505 WHERE item_id = 1004;
UPDATE character_hotbar SET item_id = 505 WHERE item_id = 1004;
UPDATE character_inventory SET item_id = 517 WHERE item_id = 1005;
UPDATE character_hotbar SET item_id = 517 WHERE item_id = 1005;
UPDATE character_inventory SET item_id = 507 WHERE item_id = 1006;
UPDATE character_hotbar SET item_id = 507 WHERE item_id = 1006;
UPDATE character_inventory SET item_id = 508 WHERE item_id = 1007;
UPDATE character_hotbar SET item_id = 508 WHERE item_id = 1007;
UPDATE character_inventory SET item_id = 511 WHERE item_id = 1008;
UPDATE character_hotbar SET item_id = 511 WHERE item_id = 1008;
UPDATE character_inventory SET item_id = 510 WHERE item_id = 1009;
UPDATE character_hotbar SET item_id = 510 WHERE item_id = 1009;
UPDATE character_inventory SET item_id = 520 WHERE item_id = 1010;
UPDATE character_hotbar SET item_id = 520 WHERE item_id = 1010;
UPDATE character_inventory SET item_id = 512 WHERE item_id = 1011;
UPDATE character_hotbar SET item_id = 512 WHERE item_id = 1011;
UPDATE character_inventory SET item_id = 513 WHERE item_id = 1012;
UPDATE character_hotbar SET item_id = 513 WHERE item_id = 1012;
UPDATE character_inventory SET item_id = 515 WHERE item_id = 1013;
UPDATE character_hotbar SET item_id = 515 WHERE item_id = 1013;
UPDATE character_inventory SET item_id = 514 WHERE item_id = 1014;
UPDATE character_hotbar SET item_id = 514 WHERE item_id = 1014;
UPDATE character_inventory SET item_id = 517 WHERE item_id = 1015;
UPDATE character_hotbar SET item_id = 517 WHERE item_id = 1015;
UPDATE character_inventory SET item_id = 582 WHERE item_id = 1016;
UPDATE character_hotbar SET item_id = 582 WHERE item_id = 1016;
UPDATE character_inventory SET item_id = 578 WHERE item_id = 1017;
UPDATE character_hotbar SET item_id = 578 WHERE item_id = 1017;
UPDATE character_inventory SET item_id = 633 WHERE item_id = 1018;
UPDATE character_hotbar SET item_id = 633 WHERE item_id = 1018;
UPDATE character_inventory SET item_id = 518 WHERE item_id = 1019;
UPDATE character_hotbar SET item_id = 518 WHERE item_id = 1019;
UPDATE character_inventory SET item_id = 620 WHERE item_id = 1020;
UPDATE character_hotbar SET item_id = 620 WHERE item_id = 1020;
UPDATE character_inventory SET item_id = 627 WHERE item_id = 1021;
UPDATE character_hotbar SET item_id = 627 WHERE item_id = 1021;
UPDATE character_inventory SET item_id = 622 WHERE item_id = 1022;
UPDATE character_hotbar SET item_id = 622 WHERE item_id = 1022;
UPDATE character_inventory SET item_id = 7182 WHERE item_id = 1023;
UPDATE character_hotbar SET item_id = 7182 WHERE item_id = 1023;
UPDATE character_inventory SET item_id = 619 WHERE item_id = 1024;
UPDATE character_hotbar SET item_id = 619 WHERE item_id = 1024;
UPDATE character_inventory SET item_id = 545 WHERE item_id = 1025;
UPDATE character_hotbar SET item_id = 545 WHERE item_id = 1025;
UPDATE character_inventory SET item_id = 972 WHERE item_id = 1026;
UPDATE character_hotbar SET item_id = 972 WHERE item_id = 1026;
UPDATE character_inventory SET item_id = 520 WHERE item_id = 1027;
UPDATE character_hotbar SET item_id = 520 WHERE item_id = 1027;
UPDATE character_inventory SET item_id = 602 WHERE item_id = 1028;
UPDATE character_hotbar SET item_id = 602 WHERE item_id = 1028;
UPDATE character_inventory SET item_id = 601 WHERE item_id = 1029;
UPDATE character_hotbar SET item_id = 601 WHERE item_id = 1029;

-- === Etc / Loot — Custom "fancy name" items ===
UPDATE character_inventory SET item_id = 909 WHERE item_id = 2001;
UPDATE character_hotbar SET item_id = 909 WHERE item_id = 2001;
UPDATE character_inventory SET item_id = 938 WHERE item_id = 2002;
UPDATE character_hotbar SET item_id = 938 WHERE item_id = 2002;
UPDATE character_inventory SET item_id = 935 WHERE item_id = 2003;
UPDATE character_hotbar SET item_id = 935 WHERE item_id = 2003;
UPDATE character_inventory SET item_id = 949 WHERE item_id = 2004;
UPDATE character_hotbar SET item_id = 949 WHERE item_id = 2004;
UPDATE character_inventory SET item_id = 921 WHERE item_id = 2005;
UPDATE character_hotbar SET item_id = 921 WHERE item_id = 2005;
UPDATE character_inventory SET item_id = 928 WHERE item_id = 2006;
UPDATE character_hotbar SET item_id = 928 WHERE item_id = 2006;
UPDATE character_inventory SET item_id = 511 WHERE item_id = 2007;
UPDATE character_hotbar SET item_id = 511 WHERE item_id = 2007;
UPDATE character_inventory SET item_id = 914 WHERE item_id = 2008;
UPDATE character_hotbar SET item_id = 914 WHERE item_id = 2008;

-- === Etc / Loot — RO-named items ===
UPDATE character_inventory SET item_id = 909 WHERE item_id = 2009;
UPDATE character_hotbar SET item_id = 909 WHERE item_id = 2009;
UPDATE character_inventory SET item_id = 914 WHERE item_id = 2010;
UPDATE character_hotbar SET item_id = 914 WHERE item_id = 2010;
UPDATE character_inventory SET item_id = 935 WHERE item_id = 2011;
UPDATE character_hotbar SET item_id = 935 WHERE item_id = 2011;
UPDATE character_inventory SET item_id = 949 WHERE item_id = 2012;
UPDATE character_hotbar SET item_id = 949 WHERE item_id = 2012;
UPDATE character_inventory SET item_id = 921 WHERE item_id = 2013;
UPDATE character_hotbar SET item_id = 921 WHERE item_id = 2013;
UPDATE character_inventory SET item_id = 917 WHERE item_id = 2014;
UPDATE character_hotbar SET item_id = 917 WHERE item_id = 2014;
UPDATE character_inventory SET item_id = 902 WHERE item_id = 2015;
UPDATE character_hotbar SET item_id = 902 WHERE item_id = 2015;
UPDATE character_inventory SET item_id = 913 WHERE item_id = 2016;
UPDATE character_hotbar SET item_id = 913 WHERE item_id = 2016;
UPDATE character_inventory SET item_id = 939 WHERE item_id = 2017;
UPDATE character_hotbar SET item_id = 939 WHERE item_id = 2017;
UPDATE character_inventory SET item_id = 928 WHERE item_id = 2018;
UPDATE character_hotbar SET item_id = 928 WHERE item_id = 2018;
UPDATE character_inventory SET item_id = 932 WHERE item_id = 2019;
UPDATE character_hotbar SET item_id = 932 WHERE item_id = 2019;
UPDATE character_inventory SET item_id = 930 WHERE item_id = 2020;
UPDATE character_hotbar SET item_id = 930 WHERE item_id = 2020;
UPDATE character_inventory SET item_id = 918 WHERE item_id = 2021;
UPDATE character_hotbar SET item_id = 918 WHERE item_id = 2021;
UPDATE character_inventory SET item_id = 908 WHERE item_id = 2022;
UPDATE character_hotbar SET item_id = 908 WHERE item_id = 2022;
UPDATE character_inventory SET item_id = 942 WHERE item_id = 2023;
UPDATE character_hotbar SET item_id = 942 WHERE item_id = 2023;
UPDATE character_inventory SET item_id = 945 WHERE item_id = 2024;
UPDATE character_hotbar SET item_id = 945 WHERE item_id = 2024;
UPDATE character_inventory SET item_id = 905 WHERE item_id = 2025;
UPDATE character_hotbar SET item_id = 905 WHERE item_id = 2025;
UPDATE character_inventory SET item_id = 924 WHERE item_id = 2026;
UPDATE character_hotbar SET item_id = 924 WHERE item_id = 2026;
UPDATE character_inventory SET item_id = 7033 WHERE item_id = 2027;
UPDATE character_hotbar SET item_id = 7033 WHERE item_id = 2027;
UPDATE character_inventory SET item_id = 911 WHERE item_id = 2028;
UPDATE character_hotbar SET item_id = 911 WHERE item_id = 2028;
UPDATE character_inventory SET item_id = 907 WHERE item_id = 2029;
UPDATE character_hotbar SET item_id = 907 WHERE item_id = 2029;
UPDATE character_inventory SET item_id = 916 WHERE item_id = 2030;
UPDATE character_hotbar SET item_id = 916 WHERE item_id = 2030;
UPDATE character_inventory SET item_id = 925 WHERE item_id = 2031;
UPDATE character_hotbar SET item_id = 925 WHERE item_id = 2031;
UPDATE character_inventory SET item_id = 919 WHERE item_id = 2032;
UPDATE character_hotbar SET item_id = 919 WHERE item_id = 2032;
UPDATE character_inventory SET item_id = 712 WHERE item_id = 2033;
UPDATE character_hotbar SET item_id = 712 WHERE item_id = 2033;
UPDATE character_inventory SET item_id = 922 WHERE item_id = 2034;
UPDATE character_hotbar SET item_id = 922 WHERE item_id = 2034;
UPDATE character_inventory SET item_id = 706 WHERE item_id = 2035;
UPDATE character_hotbar SET item_id = 706 WHERE item_id = 2035;
UPDATE character_inventory SET item_id = 915 WHERE item_id = 2036;
UPDATE character_hotbar SET item_id = 915 WHERE item_id = 2036;
UPDATE character_inventory SET item_id = 938 WHERE item_id = 2037;
UPDATE character_hotbar SET item_id = 938 WHERE item_id = 2037;
UPDATE character_inventory SET item_id = 910 WHERE item_id = 2038;
UPDATE character_hotbar SET item_id = 910 WHERE item_id = 2038;
UPDATE character_inventory SET item_id = 713 WHERE item_id = 2039;
UPDATE character_hotbar SET item_id = 713 WHERE item_id = 2039;
UPDATE character_inventory SET item_id = 1002 WHERE item_id = 2040;
UPDATE character_hotbar SET item_id = 1002 WHERE item_id = 2040;
UPDATE character_inventory SET item_id = 998 WHERE item_id = 2041;
UPDATE character_hotbar SET item_id = 998 WHERE item_id = 2041;
UPDATE character_inventory SET item_id = 1010 WHERE item_id = 2042;
UPDATE character_hotbar SET item_id = 1010 WHERE item_id = 2042;
UPDATE character_inventory SET item_id = 756 WHERE item_id = 2043;
UPDATE character_hotbar SET item_id = 756 WHERE item_id = 2043;
UPDATE character_inventory SET item_id = 992 WHERE item_id = 2044;
UPDATE character_hotbar SET item_id = 992 WHERE item_id = 2044;
UPDATE character_inventory SET item_id = 715 WHERE item_id = 2045;
UPDATE character_hotbar SET item_id = 715 WHERE item_id = 2045;
UPDATE character_inventory SET item_id = 726 WHERE item_id = 2046;
UPDATE character_hotbar SET item_id = 726 WHERE item_id = 2046;
UPDATE character_inventory SET item_id = 727 WHERE item_id = 2047;
UPDATE character_hotbar SET item_id = 727 WHERE item_id = 2047;
UPDATE character_inventory SET item_id = 728 WHERE item_id = 2048;
UPDATE character_hotbar SET item_id = 728 WHERE item_id = 2048;
UPDATE character_inventory SET item_id = 729 WHERE item_id = 2049;
UPDATE character_hotbar SET item_id = 729 WHERE item_id = 2049;
UPDATE character_inventory SET item_id = 912 WHERE item_id = 2050;
UPDATE character_hotbar SET item_id = 912 WHERE item_id = 2050;
UPDATE character_inventory SET item_id = 993 WHERE item_id = 2051;
UPDATE character_hotbar SET item_id = 993 WHERE item_id = 2051;
UPDATE character_inventory SET item_id = 7850 WHERE item_id = 2052;
UPDATE character_hotbar SET item_id = 7850 WHERE item_id = 2052;
UPDATE character_inventory SET item_id = 1750 WHERE item_id = 2053;
UPDATE character_hotbar SET item_id = 1750 WHERE item_id = 2053;
UPDATE character_inventory SET item_id = 742 WHERE item_id = 2054;
UPDATE character_hotbar SET item_id = 742 WHERE item_id = 2054;
UPDATE character_inventory SET item_id = 752 WHERE item_id = 2055;
UPDATE character_hotbar SET item_id = 752 WHERE item_id = 2055;
UPDATE character_inventory SET item_id = 753 WHERE item_id = 2056;
UPDATE character_hotbar SET item_id = 753 WHERE item_id = 2056;
UPDATE character_inventory SET item_id = 754 WHERE item_id = 2057;
UPDATE character_hotbar SET item_id = 754 WHERE item_id = 2057;
UPDATE character_inventory SET item_id = 743 WHERE item_id = 2058;
UPDATE character_hotbar SET item_id = 743 WHERE item_id = 2058;

-- === Weapons — Custom "fancy name" items ===
UPDATE character_inventory SET item_id = 1201 WHERE item_id = 3001;
UPDATE character_hotbar SET item_id = 1201 WHERE item_id = 3001;
UPDATE character_inventory SET item_id = 1202 WHERE item_id = 3002;
UPDATE character_hotbar SET item_id = 1202 WHERE item_id = 3002;
UPDATE character_inventory SET item_id = 1204 WHERE item_id = 3003;
UPDATE character_hotbar SET item_id = 1204 WHERE item_id = 3003;
UPDATE character_inventory SET item_id = 1101 WHERE item_id = 3004;
UPDATE character_hotbar SET item_id = 1101 WHERE item_id = 3004;
UPDATE character_inventory SET item_id = 1109 WHERE item_id = 3005;
UPDATE character_hotbar SET item_id = 1109 WHERE item_id = 3005;
UPDATE character_inventory SET item_id = 1701 WHERE item_id = 3006;
UPDATE character_hotbar SET item_id = 1701 WHERE item_id = 3006;

-- === Weapons — RO-named items ===
UPDATE character_inventory SET item_id = 1201 WHERE item_id = 3007;
UPDATE character_hotbar SET item_id = 1201 WHERE item_id = 3007;
UPDATE character_inventory SET item_id = 1202 WHERE item_id = 3008;
UPDATE character_hotbar SET item_id = 1202 WHERE item_id = 3008;
UPDATE character_inventory SET item_id = 1204 WHERE item_id = 3009;
UPDATE character_hotbar SET item_id = 1204 WHERE item_id = 3009;
UPDATE character_inventory SET item_id = 1109 WHERE item_id = 3010;
UPDATE character_hotbar SET item_id = 1109 WHERE item_id = 3010;
UPDATE character_inventory SET item_id = 1504 WHERE item_id = 3011;
UPDATE character_hotbar SET item_id = 1504 WHERE item_id = 3011;
UPDATE character_inventory SET item_id = 1601 WHERE item_id = 3012;
UPDATE character_hotbar SET item_id = 1601 WHERE item_id = 3012;
UPDATE character_inventory SET item_id = 1701 WHERE item_id = 3013;
UPDATE character_hotbar SET item_id = 1701 WHERE item_id = 3013;
UPDATE character_inventory SET item_id = 1401 WHERE item_id = 3014;
UPDATE character_hotbar SET item_id = 1401 WHERE item_id = 3014;
UPDATE character_inventory SET item_id = 1404 WHERE item_id = 3015;
UPDATE character_hotbar SET item_id = 1404 WHERE item_id = 3015;
UPDATE character_inventory SET item_id = 1301 WHERE item_id = 3016;
UPDATE character_hotbar SET item_id = 1301 WHERE item_id = 3016;
UPDATE character_inventory SET item_id = 1501 WHERE item_id = 3017;
UPDATE character_hotbar SET item_id = 1501 WHERE item_id = 3017;
UPDATE character_inventory SET item_id = 1604 WHERE item_id = 3018;
UPDATE character_hotbar SET item_id = 1604 WHERE item_id = 3018;
UPDATE character_inventory SET item_id = 1907 WHERE item_id = 3019;
UPDATE character_hotbar SET item_id = 1907 WHERE item_id = 3019;
UPDATE character_inventory SET item_id = 1950 WHERE item_id = 3020;
UPDATE character_hotbar SET item_id = 1950 WHERE item_id = 3020;

-- === Armor ===
UPDATE character_inventory SET item_id = 2301 WHERE item_id = 4001;
UPDATE character_hotbar SET item_id = 2301 WHERE item_id = 4001;
UPDATE character_inventory SET item_id = 2312 WHERE item_id = 4002;
UPDATE character_hotbar SET item_id = 2312 WHERE item_id = 4002;
UPDATE character_inventory SET item_id = 2314 WHERE item_id = 4003;
UPDATE character_hotbar SET item_id = 2314 WHERE item_id = 4003;
UPDATE character_inventory SET item_id = 2101 WHERE item_id = 4004;
UPDATE character_hotbar SET item_id = 2101 WHERE item_id = 4004;
UPDATE character_inventory SET item_id = 2220 WHERE item_id = 4005;
UPDATE character_hotbar SET item_id = 2220 WHERE item_id = 4005;
UPDATE character_inventory SET item_id = 2401 WHERE item_id = 4006;
UPDATE character_hotbar SET item_id = 2401 WHERE item_id = 4006;
UPDATE character_inventory SET item_id = 2321 WHERE item_id = 4007;
UPDATE character_hotbar SET item_id = 2321 WHERE item_id = 4007;
UPDATE character_inventory SET item_id = 2208 WHERE item_id = 4008;
UPDATE character_hotbar SET item_id = 2208 WHERE item_id = 4008;
UPDATE character_inventory SET item_id = 2213 WHERE item_id = 4009;
UPDATE character_hotbar SET item_id = 2213 WHERE item_id = 4009;
UPDATE character_inventory SET item_id = 2609 WHERE item_id = 4010;
UPDATE character_hotbar SET item_id = 2609 WHERE item_id = 4010;
UPDATE character_inventory SET item_id = 2298 WHERE item_id = 4011;
UPDATE character_hotbar SET item_id = 2298 WHERE item_id = 4011;
UPDATE character_inventory SET item_id = 2262 WHERE item_id = 4012;
UPDATE character_hotbar SET item_id = 2262 WHERE item_id = 4012;
UPDATE character_inventory SET item_id = 5113 WHERE item_id = 4013;
UPDATE character_hotbar SET item_id = 5113 WHERE item_id = 4013;
UPDATE character_inventory SET item_id = 2207 WHERE item_id = 4014;
UPDATE character_hotbar SET item_id = 2207 WHERE item_id = 4014;

-- === Cards ===
UPDATE character_inventory SET item_id = 4001 WHERE item_id = 5001;
UPDATE character_hotbar SET item_id = 4001 WHERE item_id = 5001;
UPDATE character_inventory SET item_id = 4006 WHERE item_id = 5002;
UPDATE character_hotbar SET item_id = 4006 WHERE item_id = 5002;
UPDATE character_inventory SET item_id = 4003 WHERE item_id = 5003;
UPDATE character_hotbar SET item_id = 4003 WHERE item_id = 5003;
UPDATE character_inventory SET item_id = 4005 WHERE item_id = 5004;
UPDATE character_hotbar SET item_id = 4005 WHERE item_id = 5004;
UPDATE character_inventory SET item_id = 4004 WHERE item_id = 5005;
UPDATE character_hotbar SET item_id = 4004 WHERE item_id = 5005;
UPDATE character_inventory SET item_id = 4009 WHERE item_id = 5006;
UPDATE character_hotbar SET item_id = 4009 WHERE item_id = 5006;
UPDATE character_inventory SET item_id = 4015 WHERE item_id = 5007;
UPDATE character_hotbar SET item_id = 4015 WHERE item_id = 5007;
UPDATE character_inventory SET item_id = 4010 WHERE item_id = 5008;
UPDATE character_hotbar SET item_id = 4010 WHERE item_id = 5008;
UPDATE character_inventory SET item_id = 4011 WHERE item_id = 5009;
UPDATE character_hotbar SET item_id = 4011 WHERE item_id = 5009;
UPDATE character_inventory SET item_id = 4016 WHERE item_id = 5010;
UPDATE character_hotbar SET item_id = 4016 WHERE item_id = 5010;
UPDATE character_inventory SET item_id = 4021 WHERE item_id = 5011;
UPDATE character_hotbar SET item_id = 4021 WHERE item_id = 5011;
UPDATE character_inventory SET item_id = 4020 WHERE item_id = 5012;
UPDATE character_hotbar SET item_id = 4020 WHERE item_id = 5012;
UPDATE character_inventory SET item_id = 4045 WHERE item_id = 5013;
UPDATE character_hotbar SET item_id = 4045 WHERE item_id = 5013;
UPDATE character_inventory SET item_id = 4022 WHERE item_id = 5014;
UPDATE character_hotbar SET item_id = 4022 WHERE item_id = 5014;
UPDATE character_inventory SET item_id = 4038 WHERE item_id = 5015;
UPDATE character_hotbar SET item_id = 4038 WHERE item_id = 5015;
UPDATE character_inventory SET item_id = 4037 WHERE item_id = 5016;
UPDATE character_hotbar SET item_id = 4037 WHERE item_id = 5016;
UPDATE character_inventory SET item_id = 4042 WHERE item_id = 5017;
UPDATE character_hotbar SET item_id = 4042 WHERE item_id = 5017;
UPDATE character_inventory SET item_id = 4033 WHERE item_id = 5018;
UPDATE character_hotbar SET item_id = 4033 WHERE item_id = 5018;
UPDATE character_inventory SET item_id = 4031 WHERE item_id = 5019;
UPDATE character_hotbar SET item_id = 4031 WHERE item_id = 5019;
UPDATE character_inventory SET item_id = 4029 WHERE item_id = 5020;
UPDATE character_hotbar SET item_id = 4029 WHERE item_id = 5020;
UPDATE character_inventory SET item_id = 4048 WHERE item_id = 5021;
UPDATE character_hotbar SET item_id = 4048 WHERE item_id = 5021;
UPDATE character_inventory SET item_id = 4044 WHERE item_id = 5022;
UPDATE character_hotbar SET item_id = 4044 WHERE item_id = 5022;
UPDATE character_inventory SET item_id = 4051 WHERE item_id = 5023;
UPDATE character_hotbar SET item_id = 4051 WHERE item_id = 5023;

-- ============================================================
-- Step 4: Delete old custom item definitions
-- ============================================================
-- These old custom IDs now have no inventory/hotbar references pointing to them
-- (all rows were remapped to canonical IDs above).
-- Delete them so the canonical INSERT can use these IDs.
DELETE FROM items WHERE item_id IN (
    -- Custom consumables
    1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010,
    1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020,
    1021, 1022, 1023, 1024, 1025, 1026, 1027, 1028, 1029,
    -- Items 1030-1033 conflict with rAthena (will be overwritten by canonical INSERT)
    1030, 1031, 1032, 1033,
    -- Custom etc items
    2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
    2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018,
    2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026, 2027, 2028,
    2029, 2030, 2031, 2032, 2033, 2034, 2035, 2036, 2037, 2038,
    2039, 2040, 2041, 2042, 2043, 2044, 2045, 2046, 2047, 2048,
    2049, 2050, 2051, 2052, 2053, 2054, 2055, 2056, 2057, 2058,
    -- Custom weapons
    3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009, 3010,
    3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018, 3019, 3020,
    -- Custom armor
    4001, 4002, 4003, 4004, 4005, 4006, 4007, 4008, 4009, 4010,
    4011, 4012, 4013, 4014,
    -- Custom cards
    5001, 5002, 5003, 5004, 5005, 5006, 5007, 5008, 5009, 5010,
    5011, 5012, 5013, 5014, 5015, 5016, 5017, 5018, 5019, 5020,
    5021, 5022, 5023
);

COMMIT;

-- ============================================================
-- Step 5: Insert all 6,169 rAthena canonical items
-- ============================================================
-- NOW run canonical_items.sql (AFTER this migration commits):
--   \i scripts/output/canonical_items.sql
--
-- That file ends with FK restore + sequence update.
-- Do NOT skip it — FKs are currently dropped!
