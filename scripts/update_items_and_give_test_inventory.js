// Migration script: Update item names to RO-inspired names, add bow, standardize weapon ranges/ASPD modifiers
// Also gives test inventory items to ALL characters in the database
// Run from scripts directory: node update_items_and_give_test_inventory.js

const path = require('path');
const { Pool } = require(path.join(__dirname, '..', 'server', 'node_modules', 'pg'));
require(path.join(__dirname, '..', 'server', 'node_modules', 'dotenv')).config({ path: path.join(__dirname, '..', 'server', '.env') });

const pool = new Pool({
    host: process.env.DB_HOST || 'localhost',
    port: process.env.DB_PORT || 5432,
    database: process.env.DB_NAME || 'sabri_mmo',
    user: process.env.DB_USER || 'postgres',
    password: process.env.DB_PASSWORD || 'postgres'
});

async function run() {
    console.log('=== Item Migration & Test Inventory Script ===\n');

    // ============================================================
    // STEP 1: Update existing item names + add weapon columns
    // ============================================================
    console.log('--- Step 1: Updating item names & weapon stats ---');

    // Ensure weapon columns exist
    try {
        await pool.query(`
            ALTER TABLE items
            ADD COLUMN IF NOT EXISTS weapon_type VARCHAR(20) DEFAULT NULL,
            ADD COLUMN IF NOT EXISTS aspd_modifier INTEGER DEFAULT 0,
            ADD COLUMN IF NOT EXISTS weapon_range INTEGER DEFAULT 0
        `);
        console.log('  Weapon columns verified.');
    } catch (err) {
        console.log('  Weapon columns already exist or error:', err.message);
    }

    // Update consumable names
    const consumableUpdates = [
        [1001, 'Crimson Vial', 'A small red tonic. Restores 50 HP.'],
        [1002, 'Amber Elixir', 'A warm orange draught. Restores 150 HP.'],
        [1003, 'Golden Salve', 'A potent yellow remedy. Restores 350 HP.'],
        [1004, 'Azure Philter', 'A calming blue brew. Restores 60 SP.'],
        [1005, 'Roasted Haunch', 'Tender roasted meat. Restores 70 HP.'],
    ];
    for (const [id, name, desc] of consumableUpdates) {
        await pool.query('UPDATE items SET name = $1, description = $2 WHERE item_id = $3', [name, desc, id]);
        console.log(`  Updated consumable ${id}: ${name}`);
    }

    // Update etc/loot names
    const etcUpdates = [
        [2001, 'Gloopy Residue', 'A small, squishy blob of unknown origin.'],
        [2002, 'Viscous Slime', 'Thick, gooey substance secreted by monsters.'],
        [2003, 'Chitin Shard', 'A hard, protective outer shell fragment.'],
        [2004, 'Downy Plume', 'A light, downy feather.'],
        [2005, 'Spore Cluster', 'Tiny spores from a forest mushroom.'],
        [2006, 'Barbed Limb', 'A chitinous leg from a large insect.'],
        [2007, 'Verdant Leaf', 'A common medicinal herb with healing properties.'],
        [2008, 'Silken Tuft', 'Soft, fluffy material from a plant creature.'],
    ];
    for (const [id, name, desc] of etcUpdates) {
        await pool.query('UPDATE items SET name = $1, description = $2 WHERE item_id = $3', [name, desc, id]);
        console.log(`  Updated etc ${id}: ${name}`);
    }

    // Update weapon names + weapon_type, aspd_modifier, weapon_range
    const weaponUpdates = [
        [3001, 'Rustic Shiv', 'A crude but swift dagger. Better than bare fists.', 'dagger', 5, 150],
        [3002, 'Keen Edge', 'A finely honed short blade.', 'dagger', 5, 150],
        [3003, 'Stiletto Fang', 'An elegant parrying dagger with a needle-thin tip.', 'dagger', 5, 150],
        [3004, 'Iron Cleaver', 'A standard one-handed sword with a broad blade.', 'one_hand_sword', 0, 150],
        [3005, 'Crescent Saber', 'A curved, heavy cutting sword.', 'one_hand_sword', 0, 150],
    ];
    for (const [id, name, desc, weaponType, aspdMod, range] of weaponUpdates) {
        await pool.query(
            'UPDATE items SET name = $1, description = $2, weapon_type = $3, aspd_modifier = $4, weapon_range = $5 WHERE item_id = $6',
            [name, desc, weaponType, aspdMod, range, id]
        );
        console.log(`  Updated weapon ${id}: ${name} (type=${weaponType}, aspd=${aspdMod >= 0 ? '+' : ''}${aspdMod}, range=${range})`);
    }

    // Insert bow (3006) if not exists
    try {
        await pool.query(`
            INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, atk, required_level, icon, weapon_type, aspd_modifier, weapon_range)
            VALUES (3006, 'Hunting Longbow', 'A sturdy ranged bow with impressive reach.', 'weapon', 'weapon', 50, 400, 35, 4, 'bow', 'bow', -3, 800)
            ON CONFLICT (item_id) DO UPDATE SET
                name = EXCLUDED.name, description = EXCLUDED.description,
                weapon_type = EXCLUDED.weapon_type, aspd_modifier = EXCLUDED.aspd_modifier, weapon_range = EXCLUDED.weapon_range
        `);
        console.log('  Added/updated weapon 3006: Hunting Longbow (type=bow, aspd=-3, range=800)');
    } catch (err) {
        console.log('  Bow insert error:', err.message);
    }

    // Update armor names
    const armorUpdates = [
        [4001, 'Linen Tunic', 'A simple linen shirt offering minimal protection.'],
        [4002, 'Quilted Vest', 'Light quilted padding for basic protection.'],
        [4003, 'Ringweave Hauberk', 'Interlocking metal rings forged into sturdy armor.'],
    ];
    for (const [id, name, desc] of armorUpdates) {
        await pool.query('UPDATE items SET name = $1, description = $2 WHERE item_id = $3', [name, desc, id]);
        console.log(`  Updated armor ${id}: ${name}`);
    }

    console.log('\n--- Step 1 Complete ---\n');

    // ============================================================
    // STEP 2: Give test inventory to ALL characters
    // ============================================================
    console.log('--- Step 2: Giving test inventory to all characters ---');

    const characters = await pool.query('SELECT character_id, name FROM characters');
    if (characters.rows.length === 0) {
        console.log('  No characters found in database!');
    }

    for (const char of characters.rows) {
        const charId = char.character_id;
        console.log(`\n  Character: ${char.name} (ID: ${charId})`);

        // Clear existing inventory for clean test (optional — comment out to keep existing items)
        await pool.query('DELETE FROM character_inventory WHERE character_id = $1', [charId]);
        console.log('    Cleared existing inventory.');

        // 3 Weapons: Rustic Shiv (3001), Iron Cleaver (3004), Hunting Longbow (3006)
        const weapons = [3001, 3004, 3006];
        for (const itemId of weapons) {
            await pool.query(
                'INSERT INTO character_inventory (character_id, item_id, quantity, is_equipped) VALUES ($1, $2, 1, false)',
                [charId, itemId]
            );
        }
        console.log('    Added 3 weapons: Rustic Shiv, Iron Cleaver, Hunting Longbow');

        // 3 Armors: Linen Tunic (4001), Quilted Vest (4002), Ringweave Hauberk (4003)
        const armors = [4001, 4002, 4003];
        for (const itemId of armors) {
            await pool.query(
                'INSERT INTO character_inventory (character_id, item_id, quantity, is_equipped) VALUES ($1, $2, 1, false)',
                [charId, itemId]
            );
        }
        console.log('    Added 3 armors: Linen Tunic, Quilted Vest, Ringweave Hauberk');

        // 10 each of 3 consumables: Crimson Vial (1001), Amber Elixir (1002), Roasted Haunch (1005)
        const consumables = [
            [1001, 10],  // 10x Crimson Vial
            [1002, 10],  // 10x Amber Elixir
            [1005, 10],  // 10x Roasted Haunch
        ];
        for (const [itemId, qty] of consumables) {
            await pool.query(
                'INSERT INTO character_inventory (character_id, item_id, quantity, is_equipped) VALUES ($1, $2, $3, false)',
                [charId, itemId, qty]
            );
        }
        console.log('    Added consumables: 10x Crimson Vial, 10x Amber Elixir, 10x Roasted Haunch');
    }

    console.log('\n--- Step 2 Complete ---\n');

    // ============================================================
    // STEP 3: Verify
    // ============================================================
    console.log('--- Step 3: Verification ---');
    const items = await pool.query('SELECT item_id, name, item_type, weapon_type, aspd_modifier, weapon_range FROM items ORDER BY item_id');
    console.log('\nAll Items in DB:');
    console.log('  ID   | Name                   | Type       | WeaponType     | ASPD Mod | Range');
    console.log('  -----|------------------------|------------|----------------|----------|------');
    for (const row of items.rows) {
        const id = String(row.item_id).padEnd(4);
        const name = (row.name || '').padEnd(22);
        const type = (row.item_type || '').padEnd(10);
        const wtype = (row.weapon_type || '-').padEnd(14);
        const aspd = String(row.aspd_modifier || 0).padStart(4);
        const range = String(row.weapon_range || 0).padStart(4);
        console.log(`  ${id} | ${name} | ${type} | ${wtype} | ${aspd}     | ${range}`);
    }

    for (const char of characters.rows) {
        const inv = await pool.query(
            `SELECT ci.inventory_id, i.name, ci.quantity, ci.is_equipped, i.item_type
             FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1 ORDER BY i.item_type, i.item_id`,
            [char.character_id]
        );
        console.log(`\n  Inventory for ${char.name} (${inv.rows.length} entries):`);
        for (const row of inv.rows) {
            console.log(`    [${row.item_type}] ${row.name} x${row.quantity} ${row.is_equipped ? '(EQUIPPED)' : ''}`);
        }
    }

    console.log('\n=== Done! ===');
    await pool.end();
}

run().catch(err => {
    console.error('FATAL:', err);
    pool.end();
    process.exit(1);
});
