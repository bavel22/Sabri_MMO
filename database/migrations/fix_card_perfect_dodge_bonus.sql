-- Fix: Card items missing perfect_dodge_bonus values
-- Root cause: canonical_items.sql INSERT did not include perfect_dodge_bonus column,
-- so all items defaulted to 0. The fix_item_script_column_audit.sql migration only
-- fixed equipment items with bFlee2, not card items.
-- Generator (generate_canonical_migration.js) has also been fixed to include
-- perfect_dodge_bonus in future regenerations.

-- Cards with unconditional bFlee2 in their rAthena scripts:
UPDATE items SET perfect_dodge_bonus = 1  WHERE item_id = 4001;  -- Poring Card (Armor)
UPDATE items SET perfect_dodge_bonus = 1  WHERE item_id = 4006;  -- Lunatic Card (Weapon)
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 4051;  -- Yoyo Card (Accessory)
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 4285;  -- Choco Card (Garment)
UPDATE items SET perfect_dodge_bonus = 1  WHERE item_id = 4306;  -- Toad Card (Garment)
UPDATE items SET perfect_dodge_bonus = 3  WHERE item_id = 4422;  -- Roween Card (Garment)

-- NOTE: Cards with CONDITIONAL bFlee2 (inside if/autobonus blocks) are NOT set here.
-- These require the future script engine to evaluate at runtime:
--   4331 Heater Card: if (BaseClass == Job_Swordman) bonus bFlee2,3;
--   4351 Kavac Icarus Card: if (getrefine()<=4) bonus bFlee2,1;
