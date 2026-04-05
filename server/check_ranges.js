require('dotenv').config();
const { Pool } = require('pg');
const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: parseInt(process.env.DB_PORT || '5432'),
    database: process.env.DB_NAME || 'sabri_mmo',
    user: process.env.DB_USER || 'postgres',
    password: process.env.DB_PASSWORD || ''
});

(async () => {
    // weapon_range distribution by weapon_type
    const dist = await pool.query(
        "SELECT weapon_type, weapon_range, COUNT(*) as cnt FROM items WHERE equip_slot = 'weapon' GROUP BY weapon_type, weapon_range ORDER BY weapon_type, weapon_range"
    );
    console.log('=== weapon_range by weapon_type ===');
    dist.rows.forEach(r => console.log(`  ${r.weapon_type || 'null'} range=${r.weapon_range}: ${r.cnt} weapons`));

    // Currently equipped weapons
    const equipped = await pool.query(
        "SELECT ci.character_id, i.name, i.weapon_type, i.weapon_range, i.atk FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id WHERE ci.is_equipped = true AND i.equip_slot = 'weapon'"
    );
    console.log('\n=== Currently equipped weapons ===');
    equipped.rows.forEach(r => console.log(`  char:${r.character_id} ${r.name} type:${r.weapon_type} range:${r.weapon_range} atk:${r.atk}`));

    // Sample bows
    const bows = await pool.query(
        "SELECT item_id, name, weapon_type, weapon_range, atk FROM items WHERE weapon_type = 'bow' ORDER BY weapon_range DESC LIMIT 10"
    );
    console.log('\n=== Sample bows ===');
    bows.rows.forEach(r => console.log(`  ${r.name} (id:${r.item_id}) range:${r.weapon_range} atk:${r.atk}`));

    // Sample swords
    const swords = await pool.query(
        "SELECT item_id, name, weapon_type, weapon_range, atk FROM items WHERE weapon_type IN ('sword', '1h_sword', '2h_sword', 'one_handed_sword', 'two_handed_sword') ORDER BY weapon_range DESC LIMIT 10"
    );
    console.log('\n=== Sample swords ===');
    swords.rows.forEach(r => console.log(`  ${r.name} (id:${r.item_id}) type:${r.weapon_type} range:${r.weapon_range} atk:${r.atk}`));

    // Check monster template ranges for comparison
    console.log('\n=== Reference: MELEE_RANGE=150, RANGED_RANGE=800 ===');

    await pool.end();
})();
