-- Migration: Add equipped_position to character_inventory
-- Tracks which equipment slot an item occupies when equipped.
-- Needed for dual-accessory support (accessory_1 vs accessory_2).
-- Values: weapon, armor, shield, head_top, head_mid, head_low,
--         footgear, garment, accessory_1, accessory_2

ALTER TABLE character_inventory
ADD COLUMN IF NOT EXISTS equipped_position VARCHAR(20) DEFAULT NULL;

-- Backfill: for currently equipped items, set equipped_position from items.equip_slot
-- Accessories get '_1' suffix; all others use equip_slot directly
UPDATE character_inventory ci
SET equipped_position = CASE
    WHEN i.equip_slot = 'accessory' THEN 'accessory_1'
    ELSE i.equip_slot
END
FROM items i
WHERE ci.item_id = i.item_id
  AND ci.is_equipped = true
  AND ci.equipped_position IS NULL;
