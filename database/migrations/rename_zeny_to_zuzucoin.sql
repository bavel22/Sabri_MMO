-- Migration: Rename zeny column to zuzucoin
-- Feature: NPC Shop system - currency rename (Feature #20)
-- Date: 2026-02-21
-- Reason: Obscure currency name to avoid direct Ragnarok Online parallels

-- Rename the column
ALTER TABLE characters RENAME COLUMN zeny TO zuzucoin;

-- Verify the change
SELECT column_name, data_type, column_default
FROM information_schema.columns
WHERE table_name = 'characters' AND column_name = 'zuzucoin';
