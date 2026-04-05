-- Account-shared Kafra storage (300 slots, shared across all characters on same user_id)
-- Access fee: 40 Zeny per open, or Free Ticket for Kafra Storage (item 7059)
-- Prerequisite: Basic Skill Level 6

CREATE TABLE IF NOT EXISTS account_storage (
    storage_id      SERIAL PRIMARY KEY,
    user_id         INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    item_id         INTEGER NOT NULL REFERENCES items(item_id),
    quantity        INTEGER NOT NULL DEFAULT 1,
    slot_index      INTEGER NOT NULL DEFAULT -1,
    identified      BOOLEAN NOT NULL DEFAULT true,
    refine_level    INTEGER NOT NULL DEFAULT 0,
    compounded_cards JSONB DEFAULT '[]',
    forged_by       VARCHAR(50) DEFAULT NULL,
    forged_element  VARCHAR(10) DEFAULT NULL,
    forged_star_crumbs INTEGER DEFAULT 0,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (user_id, slot_index)
);

CREATE INDEX IF NOT EXISTS idx_storage_user ON account_storage(user_id);
