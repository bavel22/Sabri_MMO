-- System I: Equipment Breaking — adds is_broken state to inventory items
-- Broken equipment has zeroed stats and must be repaired by NPC or Blacksmith skill

ALTER TABLE character_inventory ADD COLUMN IF NOT EXISTS is_broken BOOLEAN DEFAULT FALSE;

-- Index for finding broken items (repair NPC queries)
CREATE INDEX IF NOT EXISTS idx_inventory_broken ON character_inventory (character_id, is_broken) WHERE is_broken = TRUE;
