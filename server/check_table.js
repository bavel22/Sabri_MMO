const { Pool } = require('pg');
require('dotenv').config();

const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: process.env.DB_PORT || 5432,
    database: process.env.DB_NAME || 'sabri_mmo',
    user: process.env.DB_USER || 'postgres',
    password: process.env.DB_PASSWORD || 'goku22',
});

async function checkTableStructure() {
    try {
        console.log('=== USERS TABLE STRUCTURE ===\n');
        
        const result = await pool.query(`
            SELECT column_name, data_type, is_nullable, column_default
            FROM information_schema.columns
            WHERE table_name = 'users'
            ORDER BY ordinal_position
        `);
        
        console.log('Columns in users table:\n');
        result.rows.forEach(col => {
            console.log(`  - ${col.column_name}: ${col.data_type}`);
            if (col.column_default) {
                console.log(`    Default: ${col.column_default}`);
            }
        });
        
        // Check if last_login exists
        const hasLastLogin = result.rows.some(col => col.column_name === 'last_login');
        
        console.log('\n');
        if (!hasLastLogin) {
            console.log('❌ last_login column does NOT exist in users table');
            console.log('   We never added it to the schema.');
        } else {
            console.log('✓ last_login column exists');
        }
        
    } catch (err) {
        console.error('Error:', err.message);
    } finally {
        await pool.end();
    }
}

checkTableStructure();
