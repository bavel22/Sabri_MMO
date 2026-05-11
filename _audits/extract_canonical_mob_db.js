#!/usr/bin/env node
/**
 * extract_canonical_mob_db.js
 *
 * Parse rAthena pre-renewal mob_db.yml directly into normalized JSON.
 * No AI in the loop — deterministic regex/state-machine parser.
 *
 * Usage:
 *   node _audits/extract_canonical_mob_db.js \
 *     --src /tmp/rathena/db/pre-re/mob_db.yml \
 *     --out _audits/canonical/mob_db_pre_re_full.json
 */

'use strict';

const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);
function getArg(name) {
    const i = args.findIndex(a => a === '--' + name);
    return i >= 0 ? args[i + 1] : null;
}

const SRC = getArg('src') || '/tmp/rathena/db/pre-re/mob_db.yml';
const OUT = getArg('out') || path.join(__dirname, 'canonical/mob_db_pre_re_full.json');

if (!fs.existsSync(SRC)) {
    console.error('Source file not found:', SRC);
    process.exit(1);
}

const lines = fs.readFileSync(SRC, 'utf8').split(/\r?\n/);

const RACE_MAP = {
    'Formless': 'formless', 'Undead': 'undead', 'Brute': 'brute',
    'Plant': 'plant', 'Insect': 'insect', 'Fish': 'fish',
    'Demon': 'demon', 'Demihuman': 'demihuman', 'Angel': 'angel',
    'Dragon': 'dragon'
};
const SIZE_MAP = { 'Small': 'small', 'Medium': 'medium', 'Large': 'large' };
// rAthena uses "Dark" for shadow element
const ELEMENT_MAP = {
    'Neutral': 'neutral', 'Water': 'water', 'Earth': 'earth', 'Fire': 'fire',
    'Wind': 'wind', 'Poison': 'poison', 'Holy': 'holy',
    'Dark': 'shadow', 'Shadow': 'shadow',
    'Ghost': 'ghost', 'Undead': 'undead'
};

const monsters = {};
let current = null;

function flush() {
    if (current && current.id) {
        const id = current.id;
        // Normalize fields
        const obj = {
            name: current.Name || current.AegisName || `Mob_${id}`,
            aegisName: current.AegisName || null,
            level: current.Level ?? null,
            hp: current.Hp ?? null,
            sp: current.Sp ?? null,
            baseExp: current.BaseExp ?? 0,
            jobExp: current.JobExp ?? 0,
            mvpExp: current.MvpExp ?? 0,
            atk1: current.Attack ?? 0,
            atk2: current.Attack2 ?? current.Attack ?? 0,
            def: current.Defense ?? 0,
            mdef: current.MagicDefense ?? 0,
            str: current.Str ?? 1,
            agi: current.Agi ?? 1,
            vit: current.Vit ?? 1,
            int: current.Int ?? 1,
            dex: current.Dex ?? 1,
            luk: current.Luk ?? 1,
            attackRange: current.AttackRange ?? 1,
            skillRange: current.SkillRange ?? 10,
            chaseRange: current.ChaseRange ?? 12,
            size: SIZE_MAP[current.Size] || (current.Size || 'medium').toLowerCase(),
            race: RACE_MAP[current.Race] || (current.Race || 'formless').toLowerCase(),
            element: ELEMENT_MAP[current.Element] || (current.Element || 'neutral').toLowerCase(),
            elementLevel: current.ElementLevel ?? 1,
            walkSpeed: current.WalkSpeed ?? 200,
            attackDelay: current.AttackDelay ?? 1500,
            attackMotion: current.AttackMotion ?? 500,
            damageMotion: current.DamageMotion ?? 300,
            ai: current.Ai ?? null,
            class: current.Class ?? null,  // Normal | Boss | Guardian | Battlefield | Event
            modes: current._modes || {},
            drops: current._drops || [],
            mvpDrops: current._mvpDrops || []
        };
        monsters[id] = obj;
    }
    current = null;
}

let inModes = false;
let inDrops = false;
let inMvpDrops = false;
// State for drop entries (each is a sub-object: - Item / Rate / StealProtected)
let pendingDrop = null;

for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    // Strip comments
    const stripped = line.replace(/\s*#.*$/, '');

    // Top-level monster entry: "  - Id: 1234"
    const idMatch = stripped.match(/^\s*-\s*Id:\s*(\d+)/);
    if (idMatch) {
        // Flush any pending drop before flushing the monster
        if (pendingDrop && current) {
            const list = inMvpDrops ? '_mvpDrops' : '_drops';
            (current[list] = current[list] || []).push(pendingDrop);
            pendingDrop = null;
        }
        flush();
        current = { id: parseInt(idMatch[1]), _modes: {}, _drops: [], _mvpDrops: [] };
        inModes = false; inDrops = false; inMvpDrops = false;
        continue;
    }
    if (!current) continue;

    // Sub-blocks (entering)
    if (/^\s{4}Modes:/.test(stripped)) {
        if (pendingDrop) { (current[inMvpDrops ? '_mvpDrops' : '_drops']).push(pendingDrop); pendingDrop = null; }
        inModes = true; inDrops = false; inMvpDrops = false; continue;
    }
    if (/^\s{4}Drops:/.test(stripped))  {
        inDrops = true; inModes = false; inMvpDrops = false; pendingDrop = null; continue;
    }
    if (/^\s{4}MvpDrops:/.test(stripped)) {
        if (pendingDrop) { current._drops.push(pendingDrop); pendingDrop = null; }
        inMvpDrops = true; inDrops = false; inModes = false; continue;
    }

    // Modes sub-fields
    if (inModes) {
        const m = stripped.match(/^\s{6}(\w+):\s*(true|false)/);
        if (m) {
            current._modes[m[1].charAt(0).toLowerCase() + m[1].slice(1)] = (m[2] === 'true');
            continue;
        }
        if (/^\s{4}\w/.test(stripped)) inModes = false; // exit modes block
    }

    // Drops / MvpDrops parsing
    if (inDrops || inMvpDrops) {
        // New drop entry: "      - Item: Item_Name"
        const itemStart = stripped.match(/^\s{6}-\s*Item:\s*(\S+)/);
        if (itemStart) {
            // Flush previous pendingDrop
            if (pendingDrop) (current[inMvpDrops ? '_mvpDrops' : '_drops']).push(pendingDrop);
            pendingDrop = { item: itemStart[1], rate: 0 };
            continue;
        }
        // Sub-fields of the current drop entry: "        Rate: 5000"
        const rateMatch = stripped.match(/^\s{8}Rate:\s*(\d+)/);
        if (rateMatch && pendingDrop) {
            pendingDrop.rate = parseInt(rateMatch[1]);
            continue;
        }
        const stealMatch = stripped.match(/^\s{8}StealProtected:\s*(true|false)/);
        if (stealMatch && pendingDrop) {
            pendingDrop.stealProtected = (stealMatch[1] === 'true');
            continue;
        }
        const cardSlotMatch = stripped.match(/^\s{8}RandomOptionGroup:\s*(\S+)/);
        if (cardSlotMatch && pendingDrop) {
            pendingDrop.optionGroup = cardSlotMatch[1];
            continue;
        }
        // If we hit a new top-level field (4-space indent), exit the drops block
        if (/^\s{4}\w/.test(stripped)) {
            if (pendingDrop) {
                (current[inMvpDrops ? '_mvpDrops' : '_drops']).push(pendingDrop);
                pendingDrop = null;
            }
            inDrops = false; inMvpDrops = false;
            // fall through to parse as top-level field
        } else {
            continue;
        }
    }

    // Top-level field: "    Field: value"
    const fieldMatch = stripped.match(/^\s{4}(\w+):\s*(.+?)\s*$/);
    if (fieldMatch) {
        const key = fieldMatch[1];
        let value = fieldMatch[2];
        // Strip quotes
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith("'") && value.endsWith("'"))) {
            value = value.slice(1, -1);
        }
        // Convert numbers
        if (/^-?\d+$/.test(value)) value = parseInt(value);
        else if (/^-?\d+\.\d+$/.test(value)) value = parseFloat(value);
        // Convert booleans
        else if (value === 'true') value = true;
        else if (value === 'false') value = false;

        current[key] = value;
    }
}

// Final flush — any pending drop on the last monster
if (pendingDrop && current) {
    (current[inMvpDrops ? '_mvpDrops' : '_drops']).push(pendingDrop);
    pendingDrop = null;
}
flush();

// Ensure output directory exists
const outDir = path.dirname(OUT);
if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });

const result = {
    _comment: `Auto-extracted from ${SRC} on ${new Date().toISOString()}. Parser: extract_canonical_mob_db.js`,
    _source: SRC,
    _count: Object.keys(monsters).length,
    monsters: monsters
};

fs.writeFileSync(OUT, JSON.stringify(result, null, 2));
console.log(`Extracted ${Object.keys(monsters).length} monsters → ${OUT}`);

// Show a sample
const sampleIds = [1002, 1058, 1088, 1141, 1145];
console.log('\nSample entries:');
for (const id of sampleIds) {
    if (monsters[id]) {
        console.log(`  ${id} ${monsters[id].name.padEnd(16)} HP=${String(monsters[id].hp).padStart(6)} atk=${monsters[id].atk1}-${monsters[id].atk2} ${monsters[id].element}/${monsters[id].elementLevel} ${monsters[id].race}/${monsters[id].size}`);
    } else {
        console.log(`  ${id}: (not found)`);
    }
}
