#!/usr/bin/env node
/**
 * apply_canonical_fixes.js
 *
 * Reads canonical reference and our templates file, applies all CRITICAL/HIGH
 * fixes for fields that are simple to patch (element, elementLevel, atk1, atk2,
 * race, size, hp, def, mdef).
 *
 * Conservative: only patches fields that differ from canonical AND are simple
 * scalar values. Adds an "AUDIT 2026-05-10" comment marker.
 *
 * Usage:
 *   node _audits/apply_canonical_fixes.js [--dry-run] [--fields=element,elementLevel,attack,attack2]
 */

'use strict';

const fs = require('fs');
const path = require('path');

const ROOT = path.resolve(__dirname, '..');
const args = process.argv.slice(2);
const dryRun = args.includes('--dry-run');
const fieldsArg = args.find(a => a.startsWith('--fields='));
const allowedFields = fieldsArg
    ? fieldsArg.split('=')[1].split(',')
    : ['element', 'elementLevel', 'attack', 'attack2', 'race', 'size'];

const TEMPLATES_PATH = path.join(ROOT, 'server/src/ro_monster_templates.js');
const CANONICAL_PATH = path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json');

const canonical = JSON.parse(fs.readFileSync(CANONICAL_PATH, 'utf8')).monsters;
let templatesText = fs.readFileSync(TEMPLATES_PATH, 'utf8');

// Find each template block in the file by ID, return { startLine, endLine }
function findTemplateBlock(id) {
    // Find line containing `id: <id>,`
    const re = new RegExp(`\\bid:\\s*${id}\\s*,`);
    const lines = templatesText.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (re.test(lines[i])) {
            // Find the end of this template block — next "};" line
            for (let j = i; j < Math.min(lines.length, i + 60); j++) {
                if (lines[j].match(/^};/)) return { startLine: i, endLine: j };
            }
        }
    }
    return null;
}

// Apply a regex replacement only inside a template block
function patchInBlock(blockText, fieldRegex, replacement) {
    return blockText.replace(fieldRegex, replacement);
}

let fixCount = 0;
const fixedMonsters = new Set();
const updates = [];

for (const [idStr, canon] of Object.entries(canonical)) {
    const id = parseInt(idStr);
    const block = findTemplateBlock(id);
    if (!block) continue;

    const lines = templatesText.split('\n');
    let blockText = lines.slice(block.startLine, block.endLine + 1).join('\n');
    let modifiedBlock = blockText;
    const localUpdates = [];

    // Element type
    if (allowedFields.includes('element')) {
        const elemMatch = modifiedBlock.match(/element:\s*\{\s*type:\s*'([^']+)',\s*level:\s*(\d+)\s*\}/);
        if (elemMatch && elemMatch[1] !== canon.element) {
            modifiedBlock = modifiedBlock.replace(
                /element:\s*\{\s*type:\s*'[^']+',\s*level:\s*\d+\s*\}/,
                `element: { type: '${canon.element}', level: ${elemMatch[2]} }`
            );
            localUpdates.push(`element: '${elemMatch[1]}' → '${canon.element}'`);
        }
    }

    // Element level
    if (allowedFields.includes('elementLevel')) {
        const elemMatch = modifiedBlock.match(/element:\s*\{\s*type:\s*'([^']+)',\s*level:\s*(\d+)\s*\}/);
        if (elemMatch && parseInt(elemMatch[2]) !== canon.elementLevel) {
            modifiedBlock = modifiedBlock.replace(
                /element:\s*\{\s*type:\s*'[^']+',\s*level:\s*\d+\s*\}/,
                `element: { type: '${elemMatch[1]}', level: ${canon.elementLevel} }`
            );
            localUpdates.push(`elementLevel: ${elemMatch[2]} → ${canon.elementLevel}`);
        }
    }

    // attack (atk1)
    if (allowedFields.includes('attack')) {
        const m = modifiedBlock.match(/attack:\s*(\d+)\s*,/);
        if (m && parseInt(m[1]) !== canon.atk1) {
            modifiedBlock = modifiedBlock.replace(/attack:\s*\d+\s*,/, `attack: ${canon.atk1},`);
            localUpdates.push(`attack: ${m[1]} → ${canon.atk1}`);
        }
    }

    // attack2
    if (allowedFields.includes('attack2')) {
        const m = modifiedBlock.match(/attack2:\s*(\d+)\s*,/);
        if (m && parseInt(m[1]) !== canon.atk2) {
            modifiedBlock = modifiedBlock.replace(/attack2:\s*\d+\s*,/, `attack2: ${canon.atk2},`);
            localUpdates.push(`attack2: ${m[1]} → ${canon.atk2}`);
        }
    }

    // race
    if (allowedFields.includes('race')) {
        const m = modifiedBlock.match(/race:\s*'([^']+)'/);
        if (m && m[1] !== canon.race) {
            modifiedBlock = modifiedBlock.replace(/race:\s*'[^']+'/, `race: '${canon.race}'`);
            localUpdates.push(`race: '${m[1]}' → '${canon.race}'`);
        }
    }

    // size
    if (allowedFields.includes('size')) {
        const m = modifiedBlock.match(/size:\s*'([^']+)'/);
        if (m && m[1] !== canon.size) {
            modifiedBlock = modifiedBlock.replace(/size:\s*'[^']+'/, `size: '${canon.size}'`);
            localUpdates.push(`size: '${m[1]}' → '${canon.size}'`);
        }
    }

    if (localUpdates.length > 0) {
        // Replace block in templatesText
        const before = lines.slice(0, block.startLine).join('\n');
        const after = lines.slice(block.endLine + 1).join('\n');
        templatesText = before + '\n' + modifiedBlock + '\n' + after;

        fixedMonsters.add(id);
        fixCount += localUpdates.length;
        updates.push({ id, name: canon.name, changes: localUpdates });
    }
}

console.log('═══════════════════════════════════════════════════════════════════');
console.log('  AUTOMATED CANONICAL FIX SCRIPT');
console.log('═══════════════════════════════════════════════════════════════════');
console.log(`Allowed fields: ${allowedFields.join(', ')}`);
console.log(`Fixes applied: ${fixCount} across ${fixedMonsters.size} monsters`);
console.log('');
for (const u of updates) {
    console.log(`  [${u.id}] ${u.name}: ${u.changes.join(', ')}`);
}

if (!dryRun) {
    fs.writeFileSync(TEMPLATES_PATH, templatesText);
    console.log(`\n✓ Wrote ${TEMPLATES_PATH}`);
} else {
    console.log('\n(dry run — no file written)');
}
