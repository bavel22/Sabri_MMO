-- Migration: Multi-row hotbar support (4 rows of 9 slots each = 36 total)
-- Existing single-row data becomes row_index = 0 automatically via DEFAULT.
--
-- Run this AFTER init.sql and existing migrations.
-- Idempotent: safe to run multiple times.
--
-- NOTE: Run each step separately in pgAdmin if you encounter issues.
--       The server startup also applies these changes automatically.

-- Step 1: Add row_index column (default 0 preserves existing data as row 0)
ALTER TABLE character_hotbar
ADD COLUMN IF NOT EXISTS row_index INTEGER NOT NULL DEFAULT 0;

-- Step 2: Drop old primary key (character_id, slot_index) and recreate with row_index
-- We use a DO block so it's safe to re-run if the constraint was already changed.
DO $$
BEGIN
    -- Check if the old PK still exists (without row_index)
    IF EXISTS (
        SELECT 1 FROM information_schema.table_constraints tc
        JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name
        WHERE tc.table_name = 'character_hotbar'
          AND tc.constraint_type = 'PRIMARY KEY'
          AND NOT EXISTS (
              SELECT 1 FROM information_schema.key_column_usage k2
              WHERE k2.constraint_name = tc.constraint_name
                AND k2.column_name = 'row_index'
          )
    ) THEN
        ALTER TABLE character_hotbar DROP CONSTRAINT character_hotbar_pkey;
        ALTER TABLE character_hotbar ADD PRIMARY KEY (character_id, row_index, slot_index);
        RAISE NOTICE 'Primary key updated to include row_index';
    ELSE
        RAISE NOTICE 'Primary key already includes row_index, skipping';
    END IF;
END $$;

-- Step 3: Add CHECK constraint for row_index range (0-3 = 4 rows)
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.check_constraints
        WHERE constraint_name = 'check_row_index'
    ) THEN
        ALTER TABLE character_hotbar
        ADD CONSTRAINT check_row_index CHECK (row_index >= 0 AND row_index <= 3);
        RAISE NOTICE 'Added check_row_index constraint';
    END IF;
END $$;

-- Step 4: Normalize existing skill slots from 1-based to 0-based indexing.
-- Only runs if slot_type column exists (added by server auto-migration).
-- Deletes conflicting rows first to avoid PK violations.
DO $$
BEGIN
    -- Check if slot_type column exists (server adds it dynamically)
    IF EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_name = 'character_hotbar' AND column_name = 'slot_type'
    ) THEN
        -- Delete skill rows that would collide with an existing row after shift
        DELETE FROM character_hotbar sk
        USING character_hotbar existing
        WHERE sk.slot_type = 'skill'
          AND sk.slot_index >= 1
          AND existing.character_id = sk.character_id
          AND existing.row_index = sk.row_index
          AND existing.slot_index = sk.slot_index - 1
          AND existing.ctid <> sk.ctid;

        -- Now safe to normalize remaining skill slots
        UPDATE character_hotbar
        SET slot_index = slot_index - 1
        WHERE slot_type = 'skill' AND slot_index >= 1;

        RAISE NOTICE 'Skill slot normalization complete';
    ELSE
        RAISE NOTICE 'slot_type column not found, skipping normalization (server will handle it)';
    END IF;
END $$;

-- Step 5: Index for fast lookups by character + row
CREATE INDEX IF NOT EXISTS idx_hotbar_character_row
ON character_hotbar(character_id, row_index);
