const bcrypt = require('bcrypt');
const { Pool } = require('pg');

const pool = new Pool({
    user: 'postgres',
    password: 'goku22',
    host: 'localhost',
    port: 5432,
    database: 'sabri_mmo'
});

async function createTestUser() {
    try {
        const username = '1';
        const password = '1';
        const email = '1@test.com';
        
        // Hash password
        const passwordHash = await bcrypt.hash(password, 10);
        
        // Insert user
        const result = await pool.query(
            `INSERT INTO users (username, email, password_hash) 
             VALUES ($1, $2, $3) 
             RETURNING user_id, username, email, created_at`,
            [username, email, passwordHash]
        );
        
        console.log('✓ User created successfully:');
        console.log(result.rows[0]);
        
    } catch (err) {
        if (err.code === '23505') {
            console.log('✗ User already exists');
        } else {
            console.error('Error:', err.message);
        }
    } finally {
        pool.end();
    }
}

createTestUser();
