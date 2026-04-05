-- Player-to-Player Trading System
-- Creates trade_logs table for completed trade audit trail

CREATE TABLE IF NOT EXISTS trade_logs (
    trade_id        SERIAL PRIMARY KEY,
    player_a_id     INTEGER NOT NULL,
    player_b_id     INTEGER NOT NULL,
    items_a_gave    JSONB NOT NULL DEFAULT '[]',
    items_b_gave    JSONB NOT NULL DEFAULT '[]',
    zeny_a_gave     BIGINT NOT NULL DEFAULT 0,
    zeny_b_gave     BIGINT NOT NULL DEFAULT 0,
    zone_name       VARCHAR(50),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_trade_log_a ON trade_logs(player_a_id);
CREATE INDEX IF NOT EXISTS idx_trade_log_b ON trade_logs(player_b_id);
CREATE INDEX IF NOT EXISTS idx_trade_log_time ON trade_logs(created_at);
