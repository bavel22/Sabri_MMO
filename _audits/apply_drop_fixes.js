#!/usr/bin/env node
/**
 * apply_drop_fixes.js
 *
 * Auto-applies HIGH-severity drop rate corrections from the canonical reference.
 * Conservative: only patches drops where the rate diff is > 50% (HIGH severity)
 * AND the item is uniquely resolvable. Preserves all other drop fields.
 *
 * Usage:
 *   node _audits/apply_drop_fixes.js [--dry-run] [--threshold-pct=50]
 */

'use strict';

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const args = process.argv.slice(2);
const dryRun = args.includes('--dry-run');
const thresholdArg = args.find(a => a.startsWith('--threshold-pct='));
const threshold = thresholdArg ? parseFloat(thresholdArg.split('=')[1]) : 50;

const TEMPLATES_PATH = path.join(ROOT, 'server/src/ro_monster_templates.js');
const ITEM_DB = JSON.parse(fs.readFileSync(path.join(ROOT, '_audits/canonical/item_db_pre_re_full.json'), 'utf8'));
const CANONICAL = JSON.parse(fs.readFileSync(path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json'), 'utf8')).monsters;

// Build display name → id lookup
const nameToId = new Map();
for (const [idStr, item] of Object.entries(ITEM_DB.items)) {
    if (item.name && !nameToId.has(item.name.toLowerCase())) {
        nameToId.set(item.name.toLowerCase(), parseInt(idStr));
    }
}

let templatesText = fs.readFileSync(TEMPLATES_PATH, 'utf8');

function findTemplateBlock(id) {
    const re = new RegExp(`\\bid:\\s*${id}\\s*,`);
    const lines = templatesText.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (re.test(lines[i])) {
            // Find drops: start
            for (let j = i; j < Math.min(lines.length, i + 100); j++) {
                if (lines[j].includes('drops:')) {
                    // Find end of drops array
                    for (let k = j; k < Math.min(lines.length, j + 30); k++) {
                        if (lines[k].match(/^\s+\],?$/)) {
                            return { idLine: i, dropsStart: j, dropsEnd: k };
                        }
                    }
                }
            }
        }
    }
    return null;
}

function pctDiff(a, b) {
    if (b === 0) return a === 0 ? 0 : Infinity;
    return Math.abs((a - b) / b) * 100;
}

let updates = 0;
const fixLog = [];

for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const id = parseInt(idStr);
    const block = findTemplateBlock(id);
    if (!block) continue;
    const lines = templatesText.split('\n');
    let dropsBlockText = lines.slice(block.dropsStart, block.dropsEnd + 1).join('\n');
    let modified = dropsBlockText;
    let anyChanged = false;

    for (const cd of canon.drops || []) {
        const canonItemId = ITEM_DB.aegisToId[cd.item];
        if (!canonItemId) continue;
        const canonName = ITEM_DB.items[canonItemId]?.name;
        if (!canonName) continue;
        const canonRatePct = cd.rate / 100;

        // Find the drop line in our drops block — match by exact display name
        // Pattern: { itemName: 'Name', rate: 1.5, ...}
        const escapedName = canonName.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/'/g, "\\\\'");
        // Match: 'Name' OR 'Name with apostrophe' (escaped)
        const regex = new RegExp(`(\\{\\s*itemName:\\s*'${escapedName.replace(/\\\\'/g, "\\\\'")}'\\s*,\\s*rate:\\s*)([\\d.]+)`, 'g');

        modified = modified.replace(regex, (match, prefix, rate) => {
            const ourRate = parseFloat(rate);
            const diff = pctDiff(ourRate, canonRatePct);
            if (diff > threshold) {
                fixLog.push(`[${id}] ${canon.name}: "${canonName}" ${ourRate}% → ${canonRatePct}% (${diff.toFixed(0)}% off)`);
                anyChanged = true;
                updates++;
                return prefix + canonRatePct;
            }
            return match;
        });
    }

    if (anyChanged) {
        const before = lines.slice(0, block.dropsStart).join('\n');
        const after = lines.slice(block.dropsEnd + 1).join('\n');
        templatesText = before + '\n' + modified + '\n' + after;
    }
}

console.log(`Drop rate fixes (>${threshold}% off): ${updates}`);
for (const l of fixLog) console.log('  ' + l);

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
