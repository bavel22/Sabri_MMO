-- ============================================================
-- Migration: Class & Skill System (Ragnarok Online Classic)
-- Adds skill definitions, prerequisites, and character skills
-- ============================================================

-- Skill definitions table (static data, populated by server on startup)
CREATE TABLE IF NOT EXISTS skills (
    skill_id INTEGER PRIMARY KEY,
    internal_name VARCHAR(100) NOT NULL UNIQUE,
    display_name VARCHAR(100) NOT NULL,
    class_id VARCHAR(30) NOT NULL,           -- 'novice', 'swordsman', 'mage', etc.
    max_level INTEGER NOT NULL DEFAULT 1,
    skill_type VARCHAR(20) NOT NULL DEFAULT 'active',  -- active, passive, toggle
    target_type VARCHAR(20) NOT NULL DEFAULT 'none',    -- none, self, single, ground, aoe
    element VARCHAR(20) NOT NULL DEFAULT 'neutral',     -- neutral, fire, water, wind, earth, holy, dark, ghost, undead, poison
    skill_range INTEGER NOT NULL DEFAULT 0,             -- range in game units (0 = self/melee)
    description TEXT DEFAULT '',
    icon VARCHAR(100) DEFAULT 'default_skill',
    tree_row INTEGER DEFAULT 0,                         -- row position in skill tree UI (0-indexed)
    tree_col INTEGER DEFAULT 0                          -- column position in skill tree UI (0-indexed)
);

-- Skill prerequisites (many-to-many: a skill can have multiple prerequisites)
CREATE TABLE IF NOT EXISTS skill_prerequisites (
    skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
    required_skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
    required_level INTEGER NOT NULL DEFAULT 1,
    PRIMARY KEY (skill_id, required_skill_id)
);

-- Skill level data (SP cost, cast time, cooldown per level)
CREATE TABLE IF NOT EXISTS skill_levels (
    skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
    level INTEGER NOT NULL,
    sp_cost INTEGER NOT NULL DEFAULT 0,
    cast_time_ms INTEGER NOT NULL DEFAULT 0,
    cooldown_ms INTEGER NOT NULL DEFAULT 0,
    effect_value INTEGER DEFAULT 0,            -- generic effect value (damage %, heal amount, buff amount, etc.)
    duration_ms INTEGER DEFAULT 0,             -- buff/debuff duration
    description TEXT DEFAULT '',
    PRIMARY KEY (skill_id, level)
);

-- Character learned skills
CREATE TABLE IF NOT EXISTS character_skills (
    character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
    skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
    level INTEGER NOT NULL DEFAULT 1,
    PRIMARY KEY (character_id, skill_id)
);

-- Indexes for performance
CREATE INDEX IF NOT EXISTS idx_skills_class ON skills(class_id);
CREATE INDEX IF NOT EXISTS idx_skill_prereqs_skill ON skill_prerequisites(skill_id);
CREATE INDEX IF NOT EXISTS idx_skill_prereqs_required ON skill_prerequisites(required_skill_id);
CREATE INDEX IF NOT EXISTS idx_skill_levels_skill ON skill_levels(skill_id);
CREATE INDEX IF NOT EXISTS idx_char_skills_character ON character_skills(character_id);
CREATE INDEX IF NOT EXISTS idx_char_skills_skill ON character_skills(skill_id);
