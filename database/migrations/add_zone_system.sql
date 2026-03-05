-- Zone System Migration
-- Adds zone tracking and save point columns to characters table

-- Zone tracking: which map the character is currently in
ALTER TABLE characters
ADD COLUMN IF NOT EXISTS zone_name VARCHAR(50) DEFAULT 'prontera_south';

-- Save point: where the character respawns on death / Butterfly Wing
ALTER TABLE characters
ADD COLUMN IF NOT EXISTS save_map VARCHAR(50) DEFAULT 'prontera',
ADD COLUMN IF NOT EXISTS save_x FLOAT DEFAULT 0,
ADD COLUMN IF NOT EXISTS save_y FLOAT DEFAULT 0,
ADD COLUMN IF NOT EXISTS save_z FLOAT DEFAULT 580;

-- Index for efficient zone-based queries
CREATE INDEX IF NOT EXISTS idx_characters_zone ON characters(zone_name);
