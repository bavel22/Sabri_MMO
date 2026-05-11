#!/usr/bin/env node
/**
 * add_missing_cards.js
 *
 * Adds the 9 missing variant card drops identified by audit_monster_drops.js HIGH findings.
 * Verifies each card exists in our items DB before adding the drop.
 *
 * Usage:
 *   node _audits/add_missing_cards.js [--dry-run]
 */

'use strict';

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const args = process.argv.slice(2);
const dryRun = args.includes('--dry-run');

const TEMPLATES_PATH = path.join(ROOT, 'server/src/ro_monster_templates.js');
const ITEM_DB = JSON.parse(fs.readFileSync(path.join(ROOT, '_audits/canonical/item_db_pre_re_full.json'), 'utf8'));

// (mob_id, card_item_id, card_display_name, rate_pct)
// rate_pct = 0.01 means 1 in 10000 (canonical card drop rate)
const MISSING_CARDS = [
    { mobId: 1050, cardId: 4011, name: 'Picky Egg Card',     rate: 0.01 },
    { mobId: 1062, cardId: 4005, name: 'Santa Poring Card',  rate: 0.01 },
    { mobId: 1101, cardId: 4129, name: 'Bapho Jr. Card',     rate: 0.01 },
    { mobId: 1156, cardId: 4120, name: 'Sky Petite Card',    rate: 0.01 },
    { mobId: 1241, cardId: 4011, name: 'Picky Egg Card',     rate: 0.01 },
    { mobId: 1716, cardId: 4379, name: 'Blue Acidus Card',   rate: 0.01 },
    { mobId: 1717, cardId: 4381, name: 'Green Ferus Card',   rate: 0.01 },
    { mobId: 1718, cardId: 4382, name: 'Yellow Novus Card',  rate: 0.01 },
    // These are NOT cards (id < 4001 or > 4419) but canonical marks them StealProtected.
    // Add the drops at canonical rates with stealProtected: true to honor the flag.
    { mobId: 1789, cardId: 7066, name: 'Ice Cubic',          rate: 5 },     // canon 500/10000 = 5%
    { mobId: 1840, cardId: 12239, name: 'New Year Rice Cake', rate: 30 },   // canon 3000/10000 = 30%
];

let templatesText = fs.readFileSync(TEMPLATES_PATH, 'utf8');

function findDropsBlock(id) {
    const re = new RegExp(`\\bid:\\s*${id}\\s*,`);
    const lines = templatesText.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (re.test(lines[i])) {
            for (let j = i; j < Math.min(lines.length, i + 100); j++) {
                if (lines[j].trim().startsWith('drops:')) {
                    // Find the closing ]
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

let added = 0;
const log = [];

for (const card of MISSING_CARDS) {
    // Verify card exists in canonical
    if (!ITEM_DB.items[card.cardId]) {
        log.push(`  SKIP id ${card.cardId}: not in canonical item_db`);
        continue;
    }

    const block = findDropsBlock(card.mobId);
    if (!block) {
        log.push(`  SKIP mob ${card.mobId}: drops block not found`);
        continue;
    }

    const lines = templatesText.split('\n');
    const dropsContent = lines.slice(block.dropsStart, block.dropsEnd + 1).join('\n');

    // Check if this card is already in drops
    const escapedName = card.name.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/'/g, "\\'");
    if (new RegExp(`itemName:\\s*'${escapedName}'`).test(dropsContent)) {
        log.push(`  SKIP mob ${card.mobId} card "${card.name}": already in drops`);
        continue;
    }

    // Get the indentation pattern from an existing drop line
    let indent = '        '; // default 8 spaces
    for (let i = block.dropsStart + 1; i < block.dropsEnd; i++) {
        const m = lines[i].match(/^(\s+)\{/);
        if (m) { indent = m[1]; break; }
    }

    // Build the new drop line — escape apostrophe for JS string literal
    const escapedJs = card.name.replace(/'/g, "\\'");
    const newLine = `${indent}{ itemName: '${escapedJs}', rate: ${card.rate}, stealProtected: true },`;

    // Insert before the closing `]`
    const before = lines.slice(0, block.dropsEnd).join('\n');
    const after = lines.slice(block.dropsEnd).join('\n');
    templatesText = before + '\n' + newLine + '\n' + after;

    log.push(`  ✓ mob ${card.mobId}: added "${card.name}" drop @ ${card.rate}%`);
    added++;
}

console.log(`Added ${added} card drops:`);
for (const l of log) console.log(l);

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
