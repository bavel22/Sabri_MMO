-- Card Compound System Support
-- Ensures compounded_cards JSONB column exists (may already exist from add_item_inspect_fields.sql)
-- Populates card_type from equip_locations for all card items

-- Ensure compounded_cards column exists
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS compounded_cards JSONB DEFAULT '[]'::jsonb;

-- Populate card_type for card items based on their equip_locations column
-- This maps rAthena equip_locations to our card_type categories
UPDATE items SET card_type = 'weapon'    WHERE item_type = 'card' AND equip_locations = 'Right_Hand' AND card_type IS NULL;
UPDATE items SET card_type = 'shield'    WHERE item_type = 'card' AND equip_locations = 'Left_Hand' AND card_type IS NULL;
UPDATE items SET card_type = 'armor'     WHERE item_type = 'card' AND equip_locations = 'Armor' AND card_type IS NULL;
UPDATE items SET card_type = 'garment'   WHERE item_type = 'card' AND equip_locations = 'Garment' AND card_type IS NULL;
UPDATE items SET card_type = 'footgear'  WHERE item_type = 'card' AND equip_locations = 'Shoes' AND card_type IS NULL;
UPDATE items SET card_type = 'headgear'  WHERE item_type = 'card' AND (equip_locations LIKE '%Head%') AND card_type IS NULL;
UPDATE items SET card_type = 'accessory' WHERE item_type = 'card' AND equip_locations = 'Both_Accessory' AND card_type IS NULL;

-- Index for faster card bonus lookups on equipped items with cards
CREATE INDEX IF NOT EXISTS idx_ci_equipped_cards ON character_inventory (character_id)
    WHERE is_equipped = true AND compounded_cards IS NOT NULL AND compounded_cards != '[]'::jsonb;
