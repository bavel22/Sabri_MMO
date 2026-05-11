#!/usr/bin/env node
/**
 * extract_canonical_item_db.js
 *
 * Parse rAthena pre-re item_db_*.yml into a flat aegisName → displayName lookup.
 * Used by audit_monster_drops.js to resolve canonical drop item names to display names.
 *
 * Usage:
 *   node _audits/extract_canonical_item_db.js
 */

'use strict';

const fs = require('fs');
const path = require('path');

const SOURCES = [
    'C:/Users/pladr/AppData/Local/Temp/rathena/db/pre-re/item_db_usable.yml',
    'C:/Users/pladr/AppData/Local/Temp/rathena/db/pre-re/item_db_equip.yml',
    'C:/Users/pladr/AppData/Local/Temp/rathena/db/pre-re/item_db_etc.yml',
];

const OUT = path.resolve(__dirname, 'canonical/item_db_pre_re_full.json');

const items = {};            // id → { aegisName, name }
const aegisToId = {};        // aegisName → id

for (const src of SOURCES) {
    if (!fs.existsSync(src)) {
        console.warn('Skipping (not found):', src);
        continue;
    }
    const lines = fs.readFileSync(src, 'utf8').split(/\r?\n/);
    let current = null;
    for (const line of lines) {
        const stripped = line.replace(/\s*#.*$/, '');
        const idMatch = stripped.match(/^\s*-\s*Id:\s*(\d+)/);
        if (idMatch) {
            if (current && current.id) {
                items[current.id] = { aegisName: current.AegisName, name: current.Name };
                if (current.AegisName) aegisToId[current.AegisName] = current.id;
            }
            current = { id: parseInt(idMatch[1]) };
            continue;
        }
        if (!current) continue;
        // Only top-level fields at 4-space indent
        const m = stripped.match(/^\s{4}(AegisName|Name):\s*(.+?)\s*$/);
        if (m) {
            let v = m[2];
            if ((v.startsWith('"') && v.endsWith('"')) || (v.startsWith("'") && v.endsWith("'"))) v = v.slice(1, -1);
            current[m[1]] = v;
        }
    }
    if (current && current.id) {
        items[current.id] = { aegisName: current.AegisName, name: current.Name };
        if (current.AegisName) aegisToId[current.AegisName] = current.id;
    }
}

const outDir = path.dirname(OUT);
if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

fs.writeFileSync(OUT, JSON.stringify({
    _comment: `Extracted from rAthena pre-re item_db_*.yml on ${new Date().toISOString()}`,
    _count: Object.keys(items).length,
    items: items,
    aegisToId: aegisToId
}, null, 2));

console.log(`Extracted ${Object.keys(items).length} items → ${OUT}`);
console.log('Sample lookups:');
for (const aegis of ['Jellopy', 'Sticky_Mucus', 'Knife_', 'Knife', 'Poring_Card', 'Apple', 'Bee_Sting']) {
    const id = aegisToId[aegis];
    const item = items[id];
    console.log(`  ${aegis.padEnd(16)} → id ${id || 'N/A'} display "${item ? item.name : 'N/A'}"`);
}
