-- Party System Tables
-- Pre-renewal RO Classic: max 12 members, Even Share/Each Take EXP, Party Share items

CREATE TABLE IF NOT EXISTS parties (
    party_id SERIAL PRIMARY KEY,
    name VARCHAR(24) UNIQUE NOT NULL,
    leader_id INTEGER NOT NULL REFERENCES characters(id),
    exp_share SMALLINT DEFAULT 0,        -- 0=each_take, 1=even_share
    item_share SMALLINT DEFAULT 0,       -- 0=each_take, 1=party_share
    item_distribute SMALLINT DEFAULT 0,  -- 0=individual, 1=shared
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS party_members (
    party_id INTEGER NOT NULL REFERENCES parties(party_id) ON DELETE CASCADE,
    character_id INTEGER NOT NULL REFERENCES characters(id),
    joined_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (party_id, character_id)
);

CREATE INDEX IF NOT EXISTS idx_party_members_char ON party_members(character_id);
CREATE INDEX IF NOT EXISTS idx_parties_leader ON parties(leader_id);
