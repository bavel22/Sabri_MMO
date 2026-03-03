-- ============================================================
-- Migration: Add Ragnarok Online-style EXP & Leveling System
-- 
-- Adds dual progression (Base Level + Job Level) columns to characters table.
-- Base Level = stat progression (stat points on level up)
-- Job Level = class mastery (skill points on level up)
-- Job Class = novice → first class → second class progression
-- ============================================================

-- Add job leveling columns to characters table
ALTER TABLE characters
    ADD COLUMN IF NOT EXISTS job_level INTEGER DEFAULT 1,
    ADD COLUMN IF NOT EXISTS base_exp BIGINT DEFAULT 0,
    ADD COLUMN IF NOT EXISTS job_exp BIGINT DEFAULT 0,
    ADD COLUMN IF NOT EXISTS job_class VARCHAR(30) DEFAULT 'novice',
    ADD COLUMN IF NOT EXISTS skill_points INTEGER DEFAULT 0;

-- Rename 'experience' to track base_exp if it exists (legacy column)
-- Note: We use the new 'base_exp' column instead of the old 'experience' column.
-- The old 'experience' column is left untouched for backwards compatibility.

-- Update existing characters: set base_exp from experience if they have any
UPDATE characters SET base_exp = experience WHERE experience > 0 AND base_exp = 0;

-- Index for efficient lookups by job class (useful for leaderboards, class-based queries)
CREATE INDEX IF NOT EXISTS idx_characters_job_class ON characters(job_class);
CREATE INDEX IF NOT EXISTS idx_characters_level ON characters(level);
