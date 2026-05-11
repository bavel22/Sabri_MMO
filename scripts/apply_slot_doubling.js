// Slot Doubling Pass
//
// For every (name, item_type) group of equipment, finds the highest-priced item at the
// MIN slot count and treats it as the "base". Then for each higher-slot variant, ensures
// price >= base_price * 2^slot_delta. Updates only when the current price is below that.
//
// Run AFTER apply_shop_pricing.js so the base prices already reflect the formula floor.
//
// Usage:  cd server && NODE_PATH=node_modules node ../scripts/apply_slot_doubling.js

const path = require('path');
const { Pool } = require('pg');
require('dotenv').config({ path: path.join(__dirname, '..', 'server', '.env') });

(async () => {
    const pool = new Pool({
        host: process.env.DB_HOST, port: process.env.DB_PORT,
        user: process.env.DB_USER, password: process.env.DB_PASSWORD,
        database: process.env.DB_NAME,
    });

    // Pull every priced equipment row with the columns we need.
    const r = await pool.query(`
        SELECT item_id, name, item_type, slots, buy_price
          FROM items
         WHERE item_type IN ('weapon','armor') AND buy_price > 0
         ORDER BY name, item_type, slots ASC
    `);

    // Group by (name, item_type). Two items sharing both are treated as slot siblings
    // even if their atk/def/stats differ slightly (rAthena occasionally has variants
    // with the same display name).
    const groups = new Map();
    for (const row of r.rows) {
        const key = `${row.item_type}|${row.name}`;
        if (!groups.has(key)) groups.set(key, []);
        groups.get(key).push(row);
    }

    const updates = [];
    let groupsConsidered = 0;

    for (const [key, variants] of groups) {
        // Need at least two distinct slot counts to enforce doubling.
        const slotSet = new Set(variants.map(v => v.slots));
        if (slotSet.size < 2) continue;
        groupsConsidered++;

        // Sort by slots ASC, then by buy_price DESC within each slot count.
        // The MIN slot count is our reference; we use the MAX price among items
        // at that slot count as the base (most conservative — ensures the slotted
        // version costs at least 2x of the most-expensive sibling at the lower slot).
        variants.sort((a, b) => (a.slots - b.slots) || (b.buy_price - a.buy_price));
        const minSlots = variants[0].slots;
        const base = variants.find(v => v.slots === minSlots); // first one = highest price at min slot
        const basePrice = base.buy_price;

        for (const v of variants) {
            if (v.slots <= minSlots) continue;
            const slotDelta = v.slots - minSlots;
            const expected = Math.min(2_000_000_000, basePrice * Math.pow(2, slotDelta));
            if (v.buy_price < expected) {
                const newBuy = Math.floor(expected);
                const newSell = Math.floor(newBuy / 2);
                updates.push({
                    item_id: v.item_id, name: v.name, slots: v.slots,
                    base_id: base.item_id, base_slots: base.slots, base_price: basePrice,
                    oldBuy: v.buy_price, newBuy, newSell, slotDelta
                });
            }
        }
    }

    console.log(`Groups with multi-slot variants: ${groupsConsidered}`);
    console.log(`Items to bump (slot-doubling violation): ${updates.length}`);
    console.log('\nSample changes (first 20):');
    for (const u of updates.slice(0, 20)) {
        const oldStr = u.oldBuy.toString().padStart(10);
        const newStr = u.newBuy.toString().padStart(12);
        console.log(`  ${u.name.padEnd(32)} [${u.base_slots}->${u.slots}] base=${u.base_price.toString().padStart(10)} cur=${oldStr} -> ${newStr}`);
    }

    // Apply in a transaction.
    const client = await pool.connect();
    try {
        await client.query('BEGIN');
        for (const u of updates) {
            await client.query(
                'UPDATE items SET buy_price = $1, sell_price = $2 WHERE item_id = $3',
                [u.newBuy, u.newSell, u.item_id]
            );
        }
        await client.query('COMMIT');
        console.log(`\nOK — committed ${updates.length} slot-doubling updates.`);
    } catch (err) {
        await client.query('ROLLBACK');
        console.error('ROLLBACK due to error:', err.message);
        process.exit(1);
    } finally {
        client.release();
    }

    await pool.end();
})().catch(e => { console.error(e); process.exit(1); });
