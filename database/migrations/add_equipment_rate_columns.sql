-- Add equipment rate bonus columns for bMaxHPrate, bMaxSPrate, bAspdRate
-- These were identified in the Item_Stat_Script_Audit (2026-03-13) as missing.
-- The generator now parses these from rAthena scripts and includes them in canonical_items.sql.

ALTER TABLE items ADD COLUMN IF NOT EXISTS max_hp_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS max_sp_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS aspd_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS hp_regen_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS sp_regen_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS crit_atk_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS cast_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS use_sp_rate INTEGER DEFAULT 0;
ALTER TABLE items ADD COLUMN IF NOT EXISTS heal_power INTEGER DEFAULT 0;
