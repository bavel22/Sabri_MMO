-- Migration: Add character_hotbar table for hotbar persistence
-- Run this on existing databases that already have the base schema
-- ON DELETE CASCADE ensures hotbar slot auto-clears when inventory item is consumed/dropped

CREATE TABLE IF NOT EXISTS character_hotbar (
    character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    slot_index INTEGER NOT NULL CHECK (slot_index >= 0 AND slot_index <= 8),
    inventory_id INTEGER NOT NULL REFERENCES character_inventory(inventory_id) ON DELETE CASCADE,
    item_id INTEGER NOT NULL,
    item_name VARCHAR(100) NOT NULL DEFAULT '',
    PRIMARY KEY (character_id, slot_index)
);

CREATE INDEX IF NOT EXISTS idx_hotbar_character ON character_hotbar(character_id);
