-- Insert test user (username: 1, password: 1)
-- Password hash generated with bcrypt (10 rounds)

INSERT INTO users (username, email, password_hash) 
VALUES (
    '1', 
    '1@test.com', 
    '$2b$10$h0lj7BLKcriJgjxS.oABC.hhHS2wxlXv/.S2YP7fcpEdrJmnxRYs6'
)
ON CONFLICT (username) DO NOTHING;
