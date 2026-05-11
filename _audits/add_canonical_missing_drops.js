#!/usr/bin/env node
/**
 * add_canonical_missing_drops.js
 *
 * Adds every canonical drop entry we're missing per the drop audit.
 * Uses explicit `itemId:` field to avoid display-name ambiguity (server
 * resolveDropItemIds() at index.js:4655 prefers itemId over name).
 *
 * Conservative: only adds drops where:
 *   1. The canonical item ID exists in our items table (via canonical_items.sql cross-check)
 *   2. We don't already have a drop with that itemId on this monster
 *
 * Usage:
 *   node _audits/add_canonical_missing_drops.js [--dry-run]
 */

'use strict';

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const args = process.argv.slice(2);
const dryRun = args.includes('--dry-run');

const TEMPLATES_PATH = path.join(ROOT, 'server/src/ro_monster_templates.js');
const ITEM_DB = JSON.parse(fs.readFileSync(path.join(ROOT, '_audits/canonical/item_db_pre_re_full.json'), 'utf8'));
const CANONICAL = JSON.parse(fs.readFileSync(path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json'), 'utf8')).monsters;
// Load templates as actual JS objects (parsed correctly with all apostrophes etc.)
const TEMPLATES = require(path.join(ROOT, 'server/src/ro_monster_templates'));
const TEMPLATE_OBJECTS = TEMPLATES.RO_MONSTER_TEMPLATES || TEMPLATES;
function findTemplateById(id) {
    for (const m of Object.values(TEMPLATE_OBJECTS)) if (m.id === id) return m;
    return null;
}

// Verify our items table contains the IDs we're about to drop. Since canonical_items.sql
// is the source for our items table, parse it once to build a present-id set.
const ITEMS_SQL = fs.readFileSync(path.join(ROOT, 'scripts/output/canonical_items.sql'), 'utf8');
const presentIds = new Set();
const idLineRe = /^\((\d+),/gm;
let match;
while ((match = idLineRe.exec(ITEMS_SQL)) !== null) presentIds.add(parseInt(match[1]));

// Build runtime-matched name lookup (for mapping our existing drops to ids)
const nameToId = new Map();
const sortedIds = Object.keys(ITEM_DB.items).map(Number).sort((a, b) => a - b);
for (const id of sortedIds) {
    const item = ITEM_DB.items[id];
    if (item.name) nameToId.set(item.name.toLowerCase(), id);
}

let templatesText = fs.readFileSync(TEMPLATES_PATH, 'utf8');

function findDropsBlock(id) {
    const re = new RegExp(`\\bid:\\s*${id}\\s*,`);
    const lines = templatesText.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (re.test(lines[i])) {
            for (let j = i; j < Math.min(lines.length, i + 100); j++) {
                if (lines[j].trim().startsWith('drops:')) {
                    for (let k = j; k < Math.min(lines.length, j + 50); k++) {
                        if (lines[k].trim().match(/^\],?$/)) {
                            return { dropsStart: j, dropsEnd: k };
                        }
                    }
                }
            }
        }
    }
    return null;
}

function findOurTemplate(canonId) {
    // Find by parsing the templates text — use the same approach as findDropsBlock
    const re = new RegExp(`\\bid:\\s*${canonId}\\s*,`);
    return re.test(templatesText);
}

let added = 0;
let skipped_no_item = 0;
let skipped_no_template = 0;
let skipped_already_have = 0;
const log = [];

for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const mobId = parseInt(idStr);
    const block = findDropsBlock(mobId);
    if (!block) continue;

    // Use parsed JS templates (handles apostrophes correctly via real require())
    const tmpl = findTemplateById(mobId);
    if (!tmpl) continue;
    const oursById = new Map();
    for (const od of (tmpl.drops || [])) {
        let rid = od.itemId;
        if (!rid) {
            const lower = (od.itemName || '').toLowerCase();
            rid = nameToId.get(lower);
            if (!rid) {
                const stripped = (od.itemName || '').replace(/'/g, '').toLowerCase();
                rid = nameToId.get(stripped);
            }
            if (!rid) {
                const aegis = (od.itemName || '').replace(/ /g, '_');
                rid = ITEM_DB.aegisToId[aegis];
            }
        }
        if (rid) {
            if (!oursById.has(rid)) oursById.set(rid, []);
            oursById.get(rid).push(od);
        }
    }
    const lines = templatesText.split('\n');

    // Group canonical by id
    const canonById = new Map();
    for (const cd of (canon.drops || [])) {
        const cid = ITEM_DB.aegisToId[cd.item];
        if (!cid) continue;
        if (!canonById.has(cid)) canonById.set(cid, []);
        canonById.get(cid).push(cd);
    }

    // Determine missing drops to add
    const toAdd = [];
    for (const [cid, cdGroup] of canonById) {
        const ourCount = (oursById.get(cid) || []).length;
        const missingCount = cdGroup.length - ourCount;
        if (missingCount <= 0) continue;
        if (!presentIds.has(cid)) {
            skipped_no_item++;
            log.push(`  SKIP mob ${mobId} item id ${cid}: not in our items table`);
            continue;
        }
        // Take the highest-rate missing entries first
        const sorted = cdGroup.slice().sort((a, b) => b.rate - a.rate);
        for (let i = ourCount; i < cdGroup.length; i++) {
            const cd = sorted[i];
            const dispName = ITEM_DB.items[cid]?.name || `Item_${cid}`;
            toAdd.push({ itemId: cid, itemName: dispName, rate: cd.rate / 100, stealProtected: !!cd.stealProtected });
        }
    }

    if (toAdd.length === 0) continue;

    // Insert each drop before the closing ]
    let indent = '        ';
    for (let i = block.dropsStart + 1; i < block.dropsEnd; i++) {
        const m = lines[i].match(/^(\s+)\{/);
        if (m) { indent = m[1]; break; }
    }

    const newLines = [];
    for (const a of toAdd) {
        const escapedName = a.itemName.replace(/'/g, "\\'");
        const sp = a.stealProtected ? ', stealProtected: true' : '';
        newLines.push(`${indent}{ itemName: '${escapedName}', itemId: ${a.itemId}, rate: ${a.rate}${sp} },`);
        log.push(`  ✓ mob ${mobId} ${canon.name}: added "${a.itemName}" id=${a.itemId} @ ${a.rate}%${a.stealProtected ? ' SP' : ''}`);
        added++;
    }

    const before = lines.slice(0, block.dropsEnd).join('\n');
    const after = lines.slice(block.dropsEnd).join('\n');
    templatesText = before + '\n' + newLines.join('\n') + '\n' + after;
}

console.log(`Added ${added} canonical drops`);
console.log(`Skipped (item not in our DB): ${skipped_no_item}`);
console.log(`(showing first 40 log entries):`);
for (const l of log.slice(0, 40)) console.log(l);
if (log.length > 40) console.log(`  ... +${log.length - 40} more`);

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
