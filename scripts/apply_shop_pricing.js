// Apply Shop Pricing Migration
//
// Reads the items table, computes the new buy_price floor (and sell_price = floor/2),
// and runs UPDATE statements in a single transaction. Safe to re-run — uses GREATEST
// so an item never gets cheaper.
//
// Run:  cd server && NODE_PATH=node_modules node ../scripts/apply_shop_pricing.js

const path = require('path');
const { Pool } = require('pg');
require('dotenv').config({ path: path.join(__dirname, '..', 'server', '.env') });
const { ITEM_USE_EFFECTS } = require('../server/src/ro_item_effects.js');

// ============================================================
// PRICING FORMULAS — must mirror generate_shop_pricing_proposal.js
// ============================================================

const STAT_TIER = [0, 200_000, 600_000, 1_500_000, 4_000_000];
function getStatBonusFloor(totalStats) {
    if (totalStats <= 0) return 0;
    if (totalStats <= 4) return STAT_TIER[totalStats];
    let value = 4_000_000;
    for (let i = 5; i <= totalStats; i++) {
        value = Math.floor(value * 1.5);
        if (value > 2_000_000_000) return 2_000_000_000;
    }
    return value;
}

const WEAPON_LEVEL_FACTOR = { 1: 30, 2: 150, 3: 1000, 4: 4000 };

const ELEMENT_HIGH = new Set(['holy', 'dark', 'ghost', 'undead']);
const ELEMENT_LOW = new Set(['fire', 'water', 'wind', 'earth', 'poison']);
function getElementBonus(elem) {
    elem = (elem || '').toLowerCase();
    if (ELEMENT_HIGH.has(elem)) return 75_000;
    if (ELEMENT_LOW.has(elem)) return 25_000;
    return 0;
}
function isElementalItem(item) {
    const e = (item.element || '').toLowerCase();
    return e && e !== 'neutral';
}

const SPECIFIC_ITEM_FLOOR = {
    13029: 300_000, 1177: 300_000, 13406: 300_000, 1423: 300_000, 1536: 300_000,
    1627: 300_000, 1735: 300_000, 1801: 300_000, 1802: 300_000, 1272: 300_000,
    1921: 300_000, 1975: 300_000,
    1531: 500_000, 1534: 500_000,
};

const ELEMENT_MINIMUM_FLOOR = 200_000;

function totalStatsOf(item) {
    return (item.str_bonus|0) + (item.agi_bonus|0) + (item.vit_bonus|0) +
           (item.int_bonus|0) + (item.dex_bonus|0) + (item.luk_bonus|0);
}

function getHpSpBonus(item) {
    return Math.floor((item.max_hp_bonus||0) / 100) * 50_000 +
           Math.floor((item.max_sp_bonus||0) / 10) * 50_000;
}

function computeWeaponFloor(item) {
    const wLvFactor = WEAPON_LEVEL_FACTOR[item.weapon_level] || 30;
    const atkBase = (item.atk || 0) * wLvFactor;
    const stat = getStatBonusFloor(totalStatsOf(item));
    const hpsp = getHpSpBonus(item);
    const elem = getElementBonus(item.element);
    const slotMult = Math.pow(2, item.slots || 0);
    let floor = (atkBase + stat + hpsp + elem) * slotMult;
    if (isElementalItem(item)) floor = Math.max(floor, ELEMENT_MINIMUM_FLOOR);
    if (SPECIFIC_ITEM_FLOOR[item.item_id]) floor = Math.max(floor, SPECIFIC_ITEM_FLOOR[item.item_id]);
    return Math.min(2_000_000_000, Math.floor(floor));
}

function computeArmorFloor(item) {
    const defBase = (item.def || 0) * 1000;
    const stat = getStatBonusFloor(totalStatsOf(item));
    const hpsp = getHpSpBonus(item);
    const elem = getElementBonus(item.element);
    const lvlMult = Math.max(1, (item.equip_level_min || 0) / 20);
    const slotMult = Math.pow(2, item.slots || 0);
    let floor = (defBase + stat + hpsp + elem) * lvlMult * slotMult;
    if (isElementalItem(item)) floor = Math.max(floor, ELEMENT_MINIMUM_FLOOR);
    if (SPECIFIC_ITEM_FLOOR[item.item_id]) floor = Math.max(floor, SPECIFIC_ITEM_FLOOR[item.item_id]);
    return Math.min(2_000_000_000, Math.floor(floor));
}

function computeConsumableFloor(item, effect) {
    if (!effect) return Math.max(item.buy_price || 0, 100);
    let value = 0;
    const t = effect.type;
    if (t === 'heal') {
        const hpAvg = ((effect.hpMin || 0) + (effect.hpMax || 0)) / 2;
        const spAvg = ((effect.spMin || 0) + (effect.spMax || 0)) / 2;
        value = Math.floor(hpAvg * (1 + hpAvg / 100)) + Math.floor(spAvg * 100);
    } else if (t === 'percentheal') {
        value = item.buy_price || 100;
    } else if (t === 'cure') {
        value = ((effect.cures || []).length) * 100;
    } else if (t === 'multi') {
        for (const sub of (effect.effects || [])) {
            value += computeConsumableFloor(item, sub) - 100;
        }
    } else if (t === 'sc_start') {
        value = Math.max(item.buy_price || 0, 500);
    } else if (t === 'itemskill') {
        value = Math.max(item.buy_price || 0, 200);
    } else if (t === 'getitem') {
        value = item.buy_price || 100;
    }
    return Math.max(value, 100);
}

const SHOP_WEAPON_VIEW_SPRITES = [1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 23];

// ============================================================
// MAIN
// ============================================================

(async () => {
    const pool = new Pool({
        host: process.env.DB_HOST, port: process.env.DB_PORT,
        user: process.env.DB_USER, password: process.env.DB_PASSWORD,
        database: process.env.DB_NAME,
    });
    const SPECIAL_USABLE = new Set([601, 602]);

    // Fetch shop catalogs (same filters as runtime populateShopsFromDB)
    const [weaponRes, armorRes, consumableRes] = await Promise.all([
        pool.query(
            `SELECT * FROM items WHERE item_type='weapon' AND view_sprite = ANY($1::int[]) AND buy_price > 0`,
            [SHOP_WEAPON_VIEW_SPRITES]
        ),
        pool.query(`SELECT * FROM items WHERE item_type='armor' AND buy_price > 0`),
        pool.query(`SELECT * FROM items WHERE item_type='consumable' AND buy_price > 0 AND name NOT ILIKE '%quiver%'`),
    ]);
    const consumables = consumableRes.rows.filter(r => ITEM_USE_EFFECTS[r.item_id] !== undefined || SPECIAL_USABLE.has(r.item_id));

    function planUpdates(items, computeFn) {
        const updates = [];
        for (const item of items) {
            const oldP = item.buy_price || 0;
            const computed = computeFn(item);
            const newBuy = Math.max(oldP, computed);
            if (newBuy !== oldP) {
                const newSell = Math.floor(newBuy / 2);
                updates.push({ item_id: item.item_id, name: item.name, oldBuy: oldP, newBuy, newSell });
            }
        }
        return updates;
    }

    const weaponUpdates = planUpdates(weaponRes.rows, computeWeaponFloor);
    const armorUpdates = planUpdates(armorRes.rows, computeArmorFloor);
    const consumableUpdates = planUpdates(consumables, item => computeConsumableFloor(item, ITEM_USE_EFFECTS[item.item_id]));

    const allUpdates = [...weaponUpdates, ...armorUpdates, ...consumableUpdates];

    console.log(`Planned: ${weaponUpdates.length} weapons, ${armorUpdates.length} armor, ${consumableUpdates.length} consumables = ${allUpdates.length} total updates`);

    // Sample of changes
    const samples = [13029, 1531, 1534, 1177, 13406, 1423, 1536, 1627, 1735, 1801, 1272, 1921, 1975, 1145, 1201];
    console.log('\nSample changes (selected items):');
    for (const sid of samples) {
        const u = allUpdates.find(x => x.item_id === sid);
        if (u) console.log(`  ${u.item_id} ${u.name.padEnd(28)} ${u.oldBuy.toString().padStart(10)} -> ${u.newBuy.toString().padStart(12)}`);
    }

    // Apply in a transaction
    console.log('\nApplying updates in a transaction...');
    const client = await pool.connect();
    try {
        await client.query('BEGIN');
        for (const u of allUpdates) {
            await client.query(
                'UPDATE items SET buy_price = $1, sell_price = $2 WHERE item_id = $3',
                [u.newBuy, u.newSell, u.item_id]
            );
        }
        await client.query('COMMIT');
        console.log(`OK — committed ${allUpdates.length} item price updates.`);
    } catch (err) {
        await client.query('ROLLBACK');
        console.error('ROLLBACK due to error:', err.message);
        process.exit(1);
    } finally {
        client.release();
    }

    await pool.end();
})().catch(e => { console.error(e); process.exit(1); });
