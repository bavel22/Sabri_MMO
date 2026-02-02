const { Pool } = require('pg');
require('dotenv').config();

const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: process.env.DB_PORT || 5432,
    database: process.env.DB_NAME || 'sabri_mmo',
    user: process.env.DB_USER || 'postgres',
    password: process.env.DB_PASSWORD || 'goku22',
});

async function checkDatabase() {
    try {
        console.log('=== DATABASE VERIFICATION ===\n');
        
        // Check users table
        console.log('1. USERS TABLE:');
        const usersResult = await pool.query(
            'SELECT user_id, username, email, password_hash, created_at, last_login FROM users ORDER BY user_id'
        );
        
        if (usersResult.rows.length === 0) {
            console.log('   No users found in database.\n');
        } else {
            console.log(`   Total users: ${usersResult.rows.length}\n`);
            usersResult.rows.forEach((user, index) => {
                console.log(`   User ${index + 1}:`);
                console.log(`     - ID: ${user.user_id}`);
                console.log(`     - Username: ${user.username}`);
                console.log(`     - Email: ${user.email}`);
                console.log(`     - Password Hash: ${user.password_hash.substring(0, 60)}...`);
                console.log(`     - Created: ${user.created_at}`);
                console.log(`     - Last Login: ${user.last_login || 'Never logged in'}`);
                console.log('');
            });
        }
        
        // Check characters table
        console.log('2. CHARACTERS TABLE:');
        const charResult = await pool.query(
            'SELECT character_id, user_id, name, class, level, x, y, z, created_at FROM characters ORDER BY character_id'
        );
        
        if (charResult.rows.length === 0) {
            console.log('   No characters found in database.\n');
        } else {
            console.log(`   Total characters: ${charResult.rows.length}\n`);
            charResult.rows.forEach((char, index) => {
                console.log(`   Character ${index + 1}:`);
                console.log(`     - ID: ${char.character_id}`);
                console.log(`     - User ID: ${char.user_id}`);
                console.log(`     - Name: ${char.name}`);
                console.log(`     - Class: ${char.class}`);
                console.log(`     - Level: ${char.level}`);
                console.log(`     - Position: (${char.x}, ${char.y}, ${char.z})`);
                console.log('');
            });
        }
        
        // Verify password hashing is working
        console.log('3. PASSWORD SECURITY CHECK:');
        if (usersResult.rows.length > 0) {
            const testUser = usersResult.rows[0];
            const isHashed = testUser.password_hash.startsWith('$2');
            console.log(`   - Password hashed with bcrypt: ${isHashed ? '✓ YES' : '✗ NO'}`);
            console.log(`   - Hash length: ${testUser.password_hash.length} characters`);
            console.log(`   - Hash starts with: ${testUser.password_hash.substring(0, 30)}...`);
        }
        
        console.log('\n=== VERIFICATION COMPLETE ===');
        
    } catch (err) {
        console.error('Database error:', err.message);
    } finally {
        await pool.end();
    }
}

checkDatabase();
