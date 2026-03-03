-- Migration: Add character customization fields for login redesign
-- Required for: Hair style, hair color, gender at character creation

-- Hair style (integer ID, 1-19 in RO classic)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS hair_style INTEGER DEFAULT 1;

-- Hair color (integer ID, palette index 0-8)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS hair_color INTEGER DEFAULT 0;

-- Gender ('male' or 'female')
ALTER TABLE characters ADD COLUMN IF NOT EXISTS gender VARCHAR(10) DEFAULT 'male';

-- Soft-delete timestamp for pending deletion (NULL = not marked)
ALTER TABLE characters ADD COLUMN IF NOT EXISTS delete_date TIMESTAMP DEFAULT NULL;

-- Index for pending deletions
CREATE INDEX IF NOT EXISTS idx_characters_delete_date ON characters (delete_date)
WHERE delete_date IS NOT NULL;
