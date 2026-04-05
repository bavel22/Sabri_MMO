-- Homunculus System — Phase 6B
-- One homunculus per character (Alchemist class only)

CREATE TABLE IF NOT EXISTS character_homunculus (
    homunculus_id   SERIAL PRIMARY KEY,
    character_id    INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    type            VARCHAR(20) NOT NULL CHECK (type IN ('lif','amistr','filir','vanilmirth')),
    sprite_variant  INTEGER NOT NULL DEFAULT 1,
    name            VARCHAR(24) NOT NULL DEFAULT 'Homunculus',
    level           INTEGER NOT NULL DEFAULT 1,
    experience      BIGINT NOT NULL DEFAULT 0,
    intimacy        INTEGER NOT NULL DEFAULT 250 CHECK (intimacy >= 0 AND intimacy <= 1000),
    hunger          INTEGER NOT NULL DEFAULT 50 CHECK (hunger >= 0 AND hunger <= 100),
    hp_current      INTEGER NOT NULL DEFAULT 150,
    hp_max          INTEGER NOT NULL DEFAULT 150,
    sp_current      INTEGER NOT NULL DEFAULT 40,
    sp_max          INTEGER NOT NULL DEFAULT 40,
    str             INTEGER NOT NULL DEFAULT 10,
    agi             INTEGER NOT NULL DEFAULT 10,
    vit             INTEGER NOT NULL DEFAULT 10,
    int_stat        INTEGER NOT NULL DEFAULT 10,
    dex             INTEGER NOT NULL DEFAULT 10,
    luk             INTEGER NOT NULL DEFAULT 10,
    skill_1_level   INTEGER NOT NULL DEFAULT 1,
    skill_2_level   INTEGER NOT NULL DEFAULT 0,
    skill_3_level   INTEGER NOT NULL DEFAULT 0,
    skill_points    INTEGER NOT NULL DEFAULT 0,
    is_evolved      BOOLEAN NOT NULL DEFAULT FALSE,
    is_alive        BOOLEAN NOT NULL DEFAULT TRUE,
    is_summoned     BOOLEAN NOT NULL DEFAULT FALSE,
    created_at      TIMESTAMP DEFAULT NOW(),
    updated_at      TIMESTAMP DEFAULT NOW()
);

-- One homunculus per character
CREATE UNIQUE INDEX IF NOT EXISTS idx_homunculus_character ON character_homunculus(character_id);
-- Quick lookup for summoned state
CREATE INDEX IF NOT EXISTS idx_homunculus_summoned ON character_homunculus(character_id, is_summoned);
