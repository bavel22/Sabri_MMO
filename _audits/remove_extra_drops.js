#!/usr/bin/env node
/**
 * remove_extra_drops.js
 *
 * Removes monster drop entries that aren't in canonical pre-renewal mob_db
 * for the given monster. Uses mob ID + resolved item ID + rate to identify
 * the exact line to remove (handles duplicate drops correctly).
 *
 * Conservative approach:
 *   - Loads templates as parsed JS (handles apostrophes correctly)
 *   - For each "extra" drop, locates the exact line in the templates source
 *   - Removes one line per extra (matches first unconsumed line per signature)
 *
 * Usage:
 *   node _audits/remove_extra_drops.js [--dry-run]
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

// Re-load templates fresh (don't use cached require since we'll be rewriting)
delete require.cache[require.resolve(path.join(ROOT, 'server/src/ro_monster_templates'))];
const TEMPLATES = require(path.join(ROOT, 'server/src/ro_monster_templates'));
const TEMPLATE_OBJECTS = TEMPLATES.RO_MONSTER_TEMPLATES || TEMPLATES;
function findTemplateById(id) {
    for (const m of Object.values(TEMPLATE_OBJECTS)) if (m.id === id) return m;
    return null;
}

// Runtime-matched name lookup (last-write-wins)
const nameToId = new Map();
const sortedIds = Object.keys(ITEM_DB.items).map(Number).sort((a, b) => a - b);
for (const id of sortedIds) {
    const it = ITEM_DB.items[id];
    if (it.name) nameToId.set(it.name.toLowerCase(), id);
}

function resolveDropToId(od) {
    if (od.itemId != null) return od.itemId;
    const lower = (od.itemName || '').toLowerCase();
    let id = nameToId.get(lower);
    if (id) return id;
    const stripped = (od.itemName || '').replace(/'/g, '').toLowerCase();
    id = nameToId.get(stripped);
    if (id) return id;
    const aegis = (od.itemName || '').replace(/ /g, '_');
    return ITEM_DB.aegisToId[aegis] || null;
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

// Identify extras per mob (deterministic)
const extrasPerMob = new Map();   // mobId → array of { itemName, rate, hasItemId, itemId? }
for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const mobId = parseInt(idStr);
    const tmpl = findTemplateById(mobId);
    if (!tmpl) continue;

    const canonIds = new Set();
    for (const cd of (canon.drops || [])) {
        const cid = ITEM_DB.aegisToId[cd.item];
        if (cid) canonIds.add(cid);
    }

    for (const od of (tmpl.drops || [])) {
        const rid = resolveDropToId(od);
        if (!rid) continue;
        if (!canonIds.has(rid)) {
            if (!extrasPerMob.has(mobId)) extrasPerMob.set(mobId, []);
            extrasPerMob.get(mobId).push({
                itemName: od.itemName,
                itemId: od.itemId || null,
                rate: od.rate,
                stealProtected: !!od.stealProtected,
                resolvedId: rid
            });
        }
    }
}

let removed = 0;
let mobsModified = 0;
const log = [];

for (const [mobId, extras] of extrasPerMob) {
    const block = findDropsBlock(mobId);
    if (!block) continue;
    const lines = templatesText.split('\n');
    const blockLines = lines.slice(block.dropsStart, block.dropsEnd + 1);

    // For each extra, find the matching line and remove it
    const linesToRemove = new Set();
    for (const extra of extras) {
        // Match line by: itemName + (itemId if present) + rate
        for (let i = 0; i < blockLines.length; i++) {
            if (linesToRemove.has(i)) continue;
            const line = blockLines[i];
            // Quick check: must contain the itemName (escaped)
            const escapedName = extra.itemName.replace(/'/g, "\\'");
            if (!line.includes(`itemName: '${escapedName}'`)) continue;
            // Must contain the rate
            const ratePattern = `rate: ${extra.rate}`;
            if (!line.includes(ratePattern)) continue;
            // If extra had explicit itemId, that must match
            if (extra.itemId != null) {
                const idPattern = `itemId: ${extra.itemId}`;
                if (!line.includes(idPattern)) continue;
            } else {
                // Extra had NO itemId — line must also have NO itemId field
                if (/\bitemId:\s*\d+/.test(line)) continue;
            }
            // Match: stealProtected (or absence) must align
            const lineHasSP = /stealProtected:\s*true/.test(line);
            if (lineHasSP !== extra.stealProtected) continue;

            linesToRemove.add(i);
            log.push(`  [${mobId}] remove "${extra.itemName}" id=${extra.resolvedId} rate=${extra.rate}${extra.stealProtected ? ' SP' : ''}`);
            removed++;
            break;
        }
    }

    if (linesToRemove.size > 0) {
        mobsModified++;
        const newBlockLines = blockLines.filter((_, i) => !linesToRemove.has(i));
        const before = lines.slice(0, block.dropsStart).join('\n');
        const after = lines.slice(block.dropsEnd + 1).join('\n');
        templatesText = before + '\n' + newBlockLines.join('\n') + '\n' + after;
    }
}

console.log(`Removed ${removed} extra drops across ${mobsModified} monsters`);
console.log('(showing first 40):');
for (const l of log.slice(0, 40)) console.log(l);
if (log.length > 40) console.log(`  ... +${log.length - 40} more`);

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
