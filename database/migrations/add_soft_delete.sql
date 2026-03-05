-- Migration: Add soft delete for characters
-- Characters are never fully deleted; a 'deleted' flag is set instead.

ALTER TABLE characters ADD COLUMN IF NOT EXISTS deleted BOOLEAN NOT NULL DEFAULT FALSE;

-- Index for efficient filtering of non-deleted characters
CREATE INDEX IF NOT EXISTS idx_characters_deleted ON characters(deleted) WHERE deleted = FALSE;
