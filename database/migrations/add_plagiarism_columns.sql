-- Plagiarism (Rogue skill 1714): persist copied skill across sessions
ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_id INTEGER DEFAULT NULL;
ALTER TABLE characters ADD COLUMN IF NOT EXISTS plagiarized_skill_level INTEGER DEFAULT 0;
