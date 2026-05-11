// Shop Pricing Proposal Generator (one-time analysis tool)
//
// Reads the items table, applies the proposed pricing floor formula, and writes
// docsNew/05_Development/Shop_Pricing_Proposal.md with full per-item before/after
// listings for review.
//
// Run:  cd server && node ../scripts/generate_shop_pricing_proposal.js

const fs = require('fs');
const path = require('path');
const { Pool } = require('pg');
require('dotenv').config({ path: path.join(__dirname, '..', 'server', '.env') });
const { ITEM_USE_EFFECTS } = require('../server/src/ro_item_effects.js');

// ============================================================
// PRICING FORMULAS (proposed)
// ============================================================

// User-specified stat tier table (1..4 stats explicit, 5+ = 1.5× exponential)
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

// 5× more aggressive ATK pricing (was {1:5, 2:30, 3:200, 4:800})
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

// Per-item-name overrides — items the user has called out as worth more than the
// formula suggests. Floor enforced AFTER the standard formula.
const SPECIFIC_ITEM_FLOOR = {
    13029: 300_000,  // Prinsense Knife (dagger, ATK 120)
    1177:  300_000,  // Muramash (2H sword, ATK 120)
    13406: 300_000,  // Edger (1H sword, ATK 120)
    1423:  300_000,  // Pole XO (spear, ATK 120)
    1536:  300_000,  // Good Morning Star (mace, ATK 120)
    1627:  300_000,  // Staffy (staff, ATK 40)
    1735:  300_000,  // Kkakkung (bow, ATK 120)
    1801:  300_000,  // Waghnak[3] (knuckle)
    1802:  300_000,  // Waghnak[4] (knuckle)
    1272:  300_000,  // Scratcher (katar, ATK 120)
    1921:  300_000,  // Gun Moon Gom (instrument, ATK 120)
    1975:  300_000,  // Queen Is Whip (whip, ATK 120)
    1531:  500_000,  // Wrench (mace, ATK 115)
    1534:  500_000,  // Wrench (mace, ATK 150)
};

// Any equipment with a non-neutral element must clear this floor
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
            value += computeConsumableFloor(item, sub) - 100; // strip the 100z min from each sub
        }
    } else if (t === 'sc_start') {
        // Buff potions and stat foods — fixed premium based on existing canonical
        value = Math.max(item.buy_price || 0, 500);
    } else if (t === 'itemskill') {
        value = Math.max(item.buy_price || 0, 200);
    } else if (t === 'getitem') {
        value = item.buy_price || 100;
    }
    return Math.max(value, 100);
}

// ============================================================
// FORMATTING HELPERS
// ============================================================

function fmtZ(n) {
    if (n >= 1_000_000_000) return (n / 1_000_000_000).toFixed(2) + 'B';
    if (n >= 1_000_000) return (n / 1_000_000).toFixed(2) + 'M';
    if (n >= 1_000) return (n / 1_000).toFixed(1) + 'k';
    return String(n);
}

function fmtMult(oldP, newP) {
    if (oldP <= 0) return '—';
    const m = newP / oldP;
    if (m < 1.05) return '—';
    if (m < 100) return m.toFixed(1) + '×';
    if (m < 1000) return Math.round(m) + '×';
    if (m < 1_000_000) return (m / 1000).toFixed(1) + 'k×';
    return (m / 1_000_000).toFixed(1) + 'M×';
}

// View_sprite IDs used by Weapon Dealer query (must match index.js)
const SHOP_WEAPON_VIEW_SPRITES = [1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 23];

// Tool Dealer fixed item list (mirror of index.js)
const TOOL_ITEMS = [
    501, 502, 503, 505, 506, 507, 508, 510, 511, 520,
    601, 602,
    611,
    1010, 1011, 984, 985,
    994, 995, 996, 997, 1000, 1003, 7521, 7522, 7523, 7524
];

function getToolCategory(item) {
    const id = item.item_id;
    if (id === 984 || id === 985 || id === 1010 || id === 1011) return 'Refine';
    if ((id >= 994 && id <= 997) || id === 1000 || id === 1003 || (id >= 7521 && id <= 7524)) return 'Forge';
    if (id === 601 || id === 602) return 'Travel';
    if (id === 611) return 'Utility';
    return 'Healing';
}

const WEAPON_CATEGORY_LABELS = {
    'dagger': 'Dagger', 'one_hand_sword': '1H Sword', 'two_hand_sword': '2H Sword',
    'spear': 'Spear', 'axe': 'Axe', 'mace': 'Mace', 'staff': 'Staff', 'bow': 'Bow',
    'knuckle': 'Knuckle', 'katar': 'Katar', 'instrument': 'Instrument',
    'whip': 'Whip', 'book': 'Book',
};
const ARMOR_CATEGORY_LABELS = {
    'head_top': 'Headgear', 'head_mid': 'Glasses', 'head_low': 'Mask',
    'armor': 'Armor', 'shield': 'Shield', 'garment': 'Garment',
    'footgear': 'Footgear', 'accessory': 'Accessory',
};

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
    function isFunctional(id) { return ITEM_USE_EFFECTS[id] !== undefined || SPECIAL_USABLE.has(id); }

    // Fetch items
    const weaponRes = await pool.query(
        `SELECT * FROM items WHERE item_type='weapon' AND view_sprite = ANY($1::int[]) AND buy_price > 0
         ORDER BY equip_level_min ASC, buy_price ASC, item_id ASC`,
        [SHOP_WEAPON_VIEW_SPRITES]
    );
    const armorRes = await pool.query(
        `SELECT * FROM items WHERE item_type='armor' AND buy_price > 0
         ORDER BY equip_slot ASC, equip_level_min ASC, buy_price ASC, item_id ASC`
    );
    const consumableRes = await pool.query(
        `SELECT * FROM items WHERE item_type='consumable' AND buy_price > 0 AND name NOT ILIKE '%quiver%'
         ORDER BY buy_price ASC, item_id ASC`
    );
    const arrowRes = await pool.query(
        `SELECT * FROM items WHERE (item_type='ammo' AND sub_type='Arrow' AND buy_price > 0)
              OR (item_type='consumable' AND name ILIKE '%quiver%' AND buy_price > 0)
         ORDER BY CASE WHEN item_type='ammo' THEN 0 ELSE 1 END, buy_price ASC, item_id ASC`
    );
    const cardRes = await pool.query(
        `SELECT * FROM items WHERE item_type='card' AND buy_price > 0
         ORDER BY card_type ASC NULLS LAST, name ASC, item_id ASC`
    );
    const toolRes = await pool.query(
        `SELECT * FROM items WHERE item_id = ANY($1::int[]) ORDER BY item_id`,
        [TOOL_ITEMS]
    );

    // Filter consumables/arrows by isFunctional (matches runtime filter)
    const consumables = consumableRes.rows.filter(r => isFunctional(r.item_id));
    const arrows = arrowRes.rows.filter(r => r.item_type === 'ammo' || isFunctional(r.item_id));

    // ─────────────── Build Markdown ───────────────
    let md = '';

    md += '# Shop Pricing Proposal\n\n';
    md += `_Generated: ${new Date().toISOString()}_\n\n`;
    md += '> **Read this first** — these are PROPOSED prices, not yet applied.\n';
    md += '> Tell me to ship it and I\'ll write the SQL migration. Tweak any formula\n';
    md += '> coefficient and I\'ll regenerate.\n\n';

    md += '## Pricing Formula (proposed)\n\n';
    md += '### Stat tier table (per item, total of STR+AGI+VIT+INT+DEX+LUK)\n\n';
    md += '| Total stats | Minimum floor |\n|---|---|\n';
    md += '| 0 | 0 (no bonus) |\n';
    md += '| 1 | 200,000z |\n';
    md += '| 2 | 600,000z |\n';
    md += '| 3 | 1,500,000z |\n';
    md += '| 4 | 4,000,000z |\n';
    md += '| 5+ | 4M × 1.5^(n-4), capped at 2B |\n\n';

    md += '### Weapon formula\n\n';
    md += '```\n';
    md += 'wLvFactor = {1:30, 2:150, 3:1000, 4:4000}    // 5x more aggressive than v1\n';
    md += 'atkBase   = ATK * wLvFactor[weapon_level]\n';
    md += 'stat      = statTierTable(total_stats)\n';
    md += 'hpsp      = floor(maxHP/100)*50k + floor(maxSP/10)*50k\n';
    md += 'element   = (holy|dark|ghost|undead): 75k\n';
    md += '            (fire|water|wind|earth|poison): 25k\n';
    md += 'final     = max(existing_buy_price, (atkBase + stat + hpsp + element) * 2^slots)\n';
    md += '```\n\n';

    md += '### Armor formula\n\n';
    md += '```\n';
    md += 'defBase   = DEF * 1000\n';
    md += 'stat      = statTierTable(total_stats)\n';
    md += 'hpsp      = same as weapon\n';
    md += 'element   = same as weapon\n';
    md += 'levelMult = max(1, equip_level_min / 20)\n';
    md += 'final     = max(existing_buy_price, (defBase + stat + hpsp + element) * levelMult * 2^slots)\n';
    md += '```\n\n';

    md += '### Consumable formula (HP/SP heal items only)\n\n';
    md += '```\n';
    md += 'hp_value = HP * (1 + HP/100)\n';
    md += 'sp_value = SP * 100\n';
    md += 'cure     = num_cured_statuses * 100\n';
    md += 'final    = max(existing_buy_price, hp_value + sp_value + cure, 100)\n';
    md += '```\n\n';

    md += '### Floor approach\n\n';
    md += 'Every item gets `final = max(existing_buy_price, computed_floor)`.\n';
    md += '**Nothing ever gets cheaper.** Items currently priced at canonical rAthena values\n';
    md += 'higher than the floor stay at their canonical price.\n\n';
    md += '---\n\n';

    // ─────────────── Aggregate summary ───────────────
    function summarize(items, computeFn, label) {
        let raised = 0, sameOrLower = 0, totalOld = 0, totalNew = 0;
        for (const item of items) {
            const oldP = item.buy_price || 0;
            const newP = Math.max(oldP, computeFn(item));
            totalOld += oldP;
            totalNew += newP;
            if (newP > oldP * 1.05) raised++;
            else sameOrLower++;
        }
        return { count: items.length, raised, sameOrLower, totalOld, totalNew, label };
    }
    const wSum = summarize(weaponRes.rows, computeWeaponFloor, 'Weapon Dealer');
    const aSum = summarize(armorRes.rows, computeArmorFloor, 'Armor Dealer');
    const cSum = summarize(consumables, item => computeConsumableFloor(item, ITEM_USE_EFFECTS[item.item_id]), 'General Store');

    md += '## Aggregate Impact\n\n';
    md += '| Shop | Items | Raised | Unchanged | Total Zeny (old → new) |\n|---|---|---|---|---|\n';
    for (const s of [wSum, aSum, cSum]) {
        md += `| ${s.label} | ${s.count} | ${s.raised} | ${s.sameOrLower} | ${fmtZ(s.totalOld)} → ${fmtZ(s.totalNew)} |\n`;
    }
    md += `| Tool Dealer | ${toolRes.rows.length} | (no formula change) | — | — |\n`;
    md += `| Arrow Dealer | ${arrows.length} | (no formula change) | — | — |\n`;
    md += `| Card Shop | ${cardRes.rows.length} | (no formula change) | — | — |\n`;
    md += '\n---\n\n';

    // ─────────────── Per-item tables ───────────────

    function makeWeaponTable(items, header) {
        let s = `### ${header}\n\n`;
        s += '| ID | Name | wLv | ATK | Stats | Slots | Elem | Old | **New** | × |\n';
        s += '|---|---|---|---|---|---|---|---|---|---|\n';
        const sorted = [...items].sort((a,b) => (a.equip_level_min - b.equip_level_min) || (a.atk - b.atk));
        for (const it of sorted) {
            const oldP = it.buy_price || 0;
            const newP = Math.max(oldP, computeWeaponFloor(it));
            const ts = totalStatsOf(it);
            const elem = (it.element && it.element !== 'neutral') ? it.element : '';
            s += `| ${it.item_id} | ${it.name} | ${it.weapon_level||'?'} | ${it.atk||0} | ${ts || ''} | ${it.slots||0} | ${elem} | ${fmtZ(oldP)} | **${fmtZ(newP)}** | ${fmtMult(oldP, newP)} |\n`;
        }
        return s;
    }

    function makeArmorTable(items, header) {
        let s = `### ${header}\n\n`;
        s += '| ID | Name | DEF | Stats | Slots | Lvl | Elem | Old | **New** | × |\n';
        s += '|---|---|---|---|---|---|---|---|---|---|\n';
        const sorted = [...items].sort((a,b) => (a.equip_level_min - b.equip_level_min) || (a.def - b.def));
        for (const it of sorted) {
            const oldP = it.buy_price || 0;
            const newP = Math.max(oldP, computeArmorFloor(it));
            const ts = totalStatsOf(it);
            const elem = (it.element && it.element !== 'neutral') ? it.element : '';
            s += `| ${it.item_id} | ${it.name} | ${it.def||0} | ${ts || ''} | ${it.slots||0} | ${it.equip_level_min||0} | ${elem} | ${fmtZ(oldP)} | **${fmtZ(newP)}** | ${fmtMult(oldP, newP)} |\n`;
        }
        return s;
    }

    function makeConsumableTable(items, header) {
        let s = `### ${header}\n\n`;
        s += '| ID | Name | Effect | Old | **New** | × |\n|---|---|---|---|---|---|\n';
        const sorted = [...items].sort((a,b) => a.buy_price - b.buy_price);
        for (const it of sorted) {
            const oldP = it.buy_price || 0;
            const eff = ITEM_USE_EFFECTS[it.item_id];
            const newP = Math.max(oldP, computeConsumableFloor(it, eff));
            const effDesc = eff ? eff.type : (it.item_id === 601 || it.item_id === 602 ? 'wing' : 'special');
            s += `| ${it.item_id} | ${it.name} | ${effDesc} | ${fmtZ(oldP)} | **${fmtZ(newP)}** | ${fmtMult(oldP, newP)} |\n`;
        }
        return s;
    }

    // Group weapons by category
    md += '## Weapon Dealer (623 items)\n\n';
    const weaponByCat = {};
    for (const it of weaponRes.rows) {
        const cat = WEAPON_CATEGORY_LABELS[it.weapon_type] || 'Other';
        (weaponByCat[cat] ||= []).push(it);
    }
    for (const cat of ['Dagger','1H Sword','2H Sword','Spear','Axe','Mace','Staff','Bow','Knuckle','Katar','Instrument','Whip','Book','Other']) {
        if (weaponByCat[cat]) md += makeWeaponTable(weaponByCat[cat], `${cat} (${weaponByCat[cat].length})`) + '\n';
    }

    md += '\n---\n\n## Armor Dealer (1055 items)\n\n';
    const armorByCat = {};
    for (const it of armorRes.rows) {
        const cat = ARMOR_CATEGORY_LABELS[it.equip_slot] || 'Other';
        (armorByCat[cat] ||= []).push(it);
    }
    for (const cat of ['Headgear','Glasses','Mask','Armor','Shield','Garment','Footgear','Accessory','Other']) {
        if (armorByCat[cat]) md += makeArmorTable(armorByCat[cat], `${cat} (${armorByCat[cat].length})`) + '\n';
    }

    md += '\n---\n\n## General Store (382 functional items)\n\n';
    md += makeConsumableTable(consumables, 'All consumables') + '\n';

    // Tool Dealer (no formula change but list for reference)
    md += '\n---\n\n## Tool Dealer (current prices, no change)\n\n';
    md += '| ID | Name | Tab | Price |\n|---|---|---|---|\n';
    for (const it of toolRes.rows) {
        md += `| ${it.item_id} | ${it.name} | ${getToolCategory(it)} | ${fmtZ(it.buy_price)} |\n`;
    }

    // Arrow Dealer
    md += '\n---\n\n## Arrow Dealer (current prices, no change)\n\n';
    md += '| ID | Name | Type | Price |\n|---|---|---|---|\n';
    for (const it of arrows) {
        md += `| ${it.item_id} | ${it.name} | ${it.item_type} | ${fmtZ(it.buy_price)} |\n`;
    }

    // Card Shop (full listing, no pricing change yet)
    md += '\n---\n\n## Card Shop (current prices, no change — all 20z placeholder)\n\n';
    const cardByType = {};
    for (const it of cardRes.rows) {
        const t = (it.card_type || 'other');
        (cardByType[t] ||= []).push(it);
    }
    for (const slot of ['weapon','armor','shield','garment','footgear','headgear','accessory','other']) {
        if (cardByType[slot]) {
            md += `### ${slot} cards (${cardByType[slot].length})\n\n`;
            md += '| ID | Name | Price |\n|---|---|---|\n';
            for (const it of cardByType[slot]) md += `| ${it.item_id} | ${it.name} | ${fmtZ(it.buy_price)} |\n`;
            md += '\n';
        }
    }

    // Write file
    const outPath = path.join(__dirname, '..', 'docsNew', '05_Development', 'Shop_Pricing_Proposal.md');
    fs.mkdirSync(path.dirname(outPath), { recursive: true });
    fs.writeFileSync(outPath, md);
    console.log(`Wrote ${outPath}`);
    console.log(`File size: ${(fs.statSync(outPath).size / 1024).toFixed(1)} KB`);
    console.log(`Aggregate: weapons ${wSum.raised}/${wSum.count} raised, armor ${aSum.raised}/${aSum.count} raised, consumables ${cSum.raised}/${cSum.count} raised`);

    await pool.end();
})().catch(e => { console.error(e); process.exit(1); });
