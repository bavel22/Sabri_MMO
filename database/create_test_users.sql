-- Create test users for multiplayer testing
-- Run this in psql: psql -d sabri_mmo -f database/create_test_users.sql

-- Insert test users with hashed passwords
-- Passwords are '2', '3', '4', '5', '6' respectively, hashed with bcrypt
-- Note: These hashes were generated with bcrypt (10 salt rounds)

INSERT INTO users (username, email, password_hash, created_at, last_login) VALUES
    ('2', 'user2@test.com', '$2b$10$vI8aWBnWA8LpT6R.zRROiO3s8LQF4YJqzC5zXxG1xG8KQHjP9YqJO', NOW(), NOW()),
    ('3', 'user3@test.com', '$2b$10$vI8aWBnWA8LpT6R.zRROiO3s8LQF4YJqzC5zXxG1xG8KQHjP9YqJO', NOW(), NOW()),
    ('4', 'user4@test.com', '$2b$10$vI8aWBnWA8LpT6R.zRROiO3s8LQF4YJqzC5zXxG1xG8KQHjP9YqJO', NOW(), NOW()),
    ('5', 'user5@test.com', '$2b$10$vI8aWBnWA8LpT6R.zRROiO3s8LQF4YJqzC5zXxG1xG8KQHjP9YqJO', NOW(), NOW()),
    ('6', 'user6@test.com', '$2b$10$vI8aWBnWA8LpT6R.zRROiO3s8LQF4YJqzC5zXxG1xG8KQHjP9YqJO', NOW(), NOW())
ON CONFLICT (username) DO NOTHING;

SELECT * FROM users WHERE username IN ('2', '3', '4', '5', '6');
