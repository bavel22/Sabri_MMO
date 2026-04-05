-- Migration: Add forged weapon metadata to character_inventory
-- Required for: Blacksmith Forging System (Phase 5C)
-- Tracks forger name, elemental stone, and star crumb bonus on forged weapons

-- forged_by: character name of the Blacksmith who forged this weapon
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS forged_by VARCHAR(100) DEFAULT NULL;

-- forged_element: element applied via elemental stone during forging (fire/water/wind/earth or null)
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS forged_element VARCHAR(10) DEFAULT NULL;

-- forged_star_crumbs: number of star crumbs used (0-3), determines mastery ATK bonus (+5/+10/+40)
ALTER TABLE character_inventory
    ADD COLUMN IF NOT EXISTS forged_star_crumbs INTEGER DEFAULT 0;
