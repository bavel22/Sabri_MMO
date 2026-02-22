-- Migration: Add zuzucoin (currency) column to characters table
-- Feature: NPC Shop system (Feature #20)
-- Date: 2026-02-20
-- Note: This migration is deprecated - use rename_zeny_to_zuzucoin.sql instead

ALTER TABLE characters ADD COLUMN IF NOT EXISTS zuzucoin INTEGER DEFAULT 0;

-- Verify
SELECT column_name, data_type, column_default
FROM information_schema.columns
WHERE table_name = 'characters' AND column_name = 'zuzucoin';
