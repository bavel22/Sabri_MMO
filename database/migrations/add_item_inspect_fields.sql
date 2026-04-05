-- Migration: Add refine_level and compounded_cards to character_inventory
-- Required for: Item Tooltip & Inspect System
-- Date: 2026-03-12

-- refine_level: current refine level (+0 to +10) per equipment instance
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS refine_level INTEGER DEFAULT 0;

-- compounded_cards: JSONB array of card item_ids per slot
-- Format: [null, 4036, null, null] — index = slot position, value = card item_id or null
-- Empty slots are null, filled slots are the card's item_id
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS compounded_cards JSONB DEFAULT '[]'::jsonb;
