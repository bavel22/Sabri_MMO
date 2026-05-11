#!/usr/bin/env node
/**
 * apply_slot_aware_drops.js
 *
 * Adds explicit `itemId:` to drop entries where canonical specifies a different
 * variant than our display-name lookup resolves to (mainly slotted vs unslotted
 * equipment that share the same display name like Knife 1201/1202/1203).
 *
 * The server's resolveDropItemIds() at index.js:4655 prefers `d.itemId` over name
 * lookup, so adding itemId is fully backwards compatible.
 *
 * Strategy:
 *   For each our-template drop:
 *     - Resolve our `itemName` via runtime-matching nameToId (last-write-wins).
 *     - Find canonical drop on same monster matching by display name.
 *     - If canonical id != our resolved id, add `itemId: <canonical>` to lock it.
 *
 * Usage:
 *   node _audits/apply_slot_aware_drops.js [--dry-run]
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

// nameToId — runtime-matching (last-write-wins → highest id with that display name)
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

let updates = 0;
let monstersChanged = 0;
const log = [];

for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const id = parseInt(idStr);
    const block = findDropsBlock(id);
    if (!block) continue;

    const lines = templatesText.split('\n');
    let blockText = lines.slice(block.dropsStart, block.dropsEnd + 1).join('\n');
    let modified = blockText;

    // Build canonical drop list with resolved IDs and display names, ordered by appearance
    const canonResolved = [];
    for (const cd of (canon.drops || [])) {
        const cid = ITEM_DB.aegisToId[cd.item];
        if (!cid) continue;
        const dispName = ITEM_DB.items[cid]?.name;
        if (!dispName) continue;
        canonResolved.push({ id: cid, name: dispName, rate: cd.rate, stealProtected: !!cd.stealProtected });
    }

    // Track which canonical drops we've already mapped (for duplicate handling)
    const canonByName = new Map();
    for (const cr of canonResolved) {
        const key = cr.name.toLowerCase();
        if (!canonByName.has(key)) canonByName.set(key, []);
        canonByName.get(key).push(cr);
    }

    let changed = false;
    let consumedByName = new Map();  // track how many we've matched per name

    // Process each drop entry line by line
    const blockLines = blockText.split('\n');
    for (let li = 0; li < blockLines.length; li++) {
        const line = blockLines[li];
        // Match drops without an itemId field already set
        const m = line.match(/^(\s+)\{\s*itemName:\s*'([^']+)'(.*)/);
        if (!m) continue;
        const [, indent, itemName, rest] = m;
        if (line.includes('itemId:')) continue;  // already disambiguated

        const lower = itemName.toLowerCase();
        const ourResolvedId = nameToId.get(lower);
        const canonGroup = canonByName.get(lower);
        if (!canonGroup || canonGroup.length === 0) continue;

        // How many of this name have we already matched?
        const consumed = consumedByName.get(lower) || 0;
        if (consumed >= canonGroup.length) continue;  // no more canonical matches

        const targetCanonId = canonGroup[consumed].id;
        consumedByName.set(lower, consumed + 1);

        if (targetCanonId === ourResolvedId) continue;  // already resolves correctly

        // Inject itemId field after itemName
        const newLine = line.replace(
            /(\{\s*itemName:\s*'[^']+',)/,
            `$1 itemId: ${targetCanonId},`
        );
        if (newLine !== line) {
            blockLines[li] = newLine;
            changed = true;
            updates++;
            log.push(`  [${id}] ${canon.name}: "${itemName}" → itemId: ${targetCanonId} (was resolving to ${ourResolvedId})`);
        }
    }

    if (changed) {
        monstersChanged++;
        const newBlock = blockLines.join('\n');
        const before = lines.slice(0, block.dropsStart).join('\n');
        const after = lines.slice(block.dropsEnd + 1).join('\n');
        templatesText = before + '\n' + newBlock + '\n' + after;
    }
}

console.log(`Slot-aware itemId additions: ${updates} drops across ${monstersChanged} monsters`);
console.log(`(showing first 30):`);
for (const l of log.slice(0, 30)) console.log(l);
if (log.length > 30) console.log(`  ... +${log.length - 30} more`);

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
