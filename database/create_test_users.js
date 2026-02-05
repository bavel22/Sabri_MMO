// Create test users with properly hashed passwords
// Run from database directory: node create_test_users.js

const bcrypt = require('../server/node_modules/bcrypt');
const { Pool } = require('../server/node_modules/pg');
require('../server/node_modules/dotenv').config({ path: '../server/.env' });

const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: process.env.DB_PORT || 5432,
    database: process.env.DB_NAME || 'sabri_mmo',
    user: process.env.DB_USER || 'postgres',
    password: process.env.DB_PASSWORD || 'postgres'
});

const users = [
    { username: '2', email: 'user2@test.com', password: '2' },
    { username: '3', email: 'user3@test.com', password: '3' },
    { username: '4', email: 'user4@test.com', password: '4' },
    { username: '5', email: 'user5@test.com', password: '5' },
    { username: '6', email: 'user6@test.com', password: '6' }
];

async function createUsers() {
    console.log('Creating test users...\n');
    
    for (const user of users) {
        try {
            // Hash password
            const hashedPassword = await bcrypt.hash(user.password, 10);
            
            // Insert user
            const result = await pool.query(
                `INSERT INTO users (username, email, password_hash, created_at) 
                 VALUES ($1, $2, $3, NOW()) 
                 ON CONFLICT (username) DO UPDATE 
                 SET password_hash = $3, last_login = NOW()
                 RETURNING user_id, username`,
                [user.username, user.email, hashedPassword]
            );
            
            console.log(`✓ User ${user.username} created (ID: ${result.rows[0].user_id})`);
            console.log(`  Password: ${user.password}`);
            console.log(`  Email: ${user.email}\n`);
        } catch (err) {
            console.error(`✗ Failed to create user ${user.username}:`, err.message);
        }
    }
    
    console.log('\nTest users created successfully!');
    console.log('You can now login with:');
    users.forEach(u => console.log(`  Username: ${u.username}, Password: ${u.password}`));
    
    await pool.end();
}

createUsers();
