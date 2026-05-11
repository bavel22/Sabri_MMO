#!/usr/bin/env node
/**
 * audit_monster_modes.js
 *
 * Compare our monster AI mode flags against canonical rAthena pre-re.
 *
 * Canonical: Ai code (1-27) maps to a hex bitmask via mob_db_mode_list.txt;
 *            explicit Modes: block adds/overrides individual flags.
 * Ours:      MONSTER_AI_CODES[id] || getDefaultAiCode(template.aiType) → AI_TYPE_MODES → hex
 *            template.modes overlays additional flags (rare)
 *            monsterClass='boss'/'mvp' adds knockbackImmune+statusImmune+detector(+mvp)
 *
 * Usage:
 *   node _audits/audit_monster_modes.js [--severity=ALL|HIGH|MEDIUM|LOW] [--summary] [--id=N]
 */

'use strict';

const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const TEMPLATES = require(path.join(ROOT, 'server/src/ro_monster_templates'));
const { MONSTER_AI_CODES } = require(path.join(ROOT, 'server/src/ro_monster_ai_codes'));
const CANONICAL = JSON.parse(fs.readFileSync(
    path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json'), 'utf8'
)).monsters;

const args = process.argv.slice(2);
const severityFilter = (args.find(a => a.startsWith('--severity='))?.split('=')[1] || 'ALL').toUpperCase();
const focusId = args.find(a => a.startsWith('--id='))?.split('=')[1];
const summaryOnly = args.includes('--summary');

// rAthena MD bitmask (mirror of server/src/index.js MD constants)
const MD = {
    canMove: 0x0000001, looter: 0x0000002, aggressive: 0x0000004, assist: 0x0000008,
    castSensorIdle: 0x0000010, noRandomWalk: 0x0000020, canAttack: 0x0000080,
    castSensorChase: 0x0000200, changeChase: 0x0000400, angry: 0x0000800,
    changeTargetMelee: 0x0001000, changeTargetChase: 0x0002000, targetWeak: 0x0004000,
    randomTarget: 0x0008000, mvp: 0x0080000, knockbackImmune: 0x0200000,
    detector: 0x2000000, statusImmune: 0x4000000
};

// AI code → hex (mirror of server/src/index.js AI_TYPE_MODES)
const AI_TYPE_MODES = {
    1: 0x0081, 2: 0x0083, 3: 0x1089, 4: 0x3885, 5: 0x2085, 6: 0x0000, 7: 0x108B,
    8: 0x7085, 9: 0x3095, 10: 0x0084, 11: 0x0084, 12: 0x2085, 13: 0x308D,
    17: 0x0091, 19: 0x3095, 20: 0x3295, 21: 0x3695, 24: 0x00A1, 25: 0x0001,
    26: 0xB695, 27: 0x8084
};

function getDefaultAiCode(aiType) {
    switch (aiType) {
        case 'passive': return 1;
        case 'aggressive': return 5;
        case 'reactive': return 3;
        default: return 1;
    }
}

function parseModeFlags(hexMode) {
    const flags = {};
    for (const [name, bit] of Object.entries(MD)) flags[name] = !!(hexMode & bit);
    return flags;
}

// Compute canonical mode flags from { ai, class, modes }
// rAthena interpretation:
//   - Absent `Ai:` field = no AI specified = hex 0x0000 (no flags).
//   - `Class: Boss` adds 0x6200000 (knockbackImmune + statusImmune + detector) per
//     mob_db_mode_list.txt "Aegis Class Types" table.
//   - `Class: Guardian` adds 0x4000000 (statusImmune).
//   - `Class: Battlefield` adds 0xC000000 (statusImmune + skillImmune).
//   - Explicit `Modes:` block overlays additional flags.
function canonicalFlags(canon) {
    let hex;
    if (canon.ai != null) {
        const aiCode = parseInt(canon.ai);
        hex = AI_TYPE_MODES[aiCode] ?? 0;
    } else {
        hex = 0;
    }
    // Class type → additional flags
    const cls = canon.class || 'Normal';
    if (cls === 'Boss')        hex |= 0x6200000;  // KB-immune + status-immune + detector
    else if (cls === 'Guardian')   hex |= 0x4000000;
    else if (cls === 'Battlefield') hex |= 0xC000000;
    // Event class adds 0x1000000 (no-drop-rate-modifier) — we don't track that flag

    const flags = parseModeFlags(hex);
    for (const [k, v] of Object.entries(canon.modes || {})) {
        if (v === true && k in flags) flags[k] = true;
    }
    return flags;
}

// Compute our mode flags from template
function ourFlags(template) {
    const aiCode = (MONSTER_AI_CODES && MONSTER_AI_CODES[template.id]) || getDefaultAiCode(template.aiType);
    let hex = AI_TYPE_MODES[aiCode] ?? 0x0081;
    const flags = parseModeFlags(hex);
    // template.modes overlay
    for (const [k, v] of Object.entries(template.modes || {})) {
        if (v === true && k in flags) flags[k] = true;
    }
    // boss/mvp class adds flags (mirrors spawnEnemy logic)
    if (template.monsterClass === 'boss' || template.monsterClass === 'mvp') {
        flags.knockbackImmune = true;
        flags.statusImmune = true;
        flags.detector = true;
        if (template.monsterClass === 'mvp') flags.mvp = true;
    }
    return flags;
}

function findOurTemplate(canonId) {
    const all = TEMPLATES.RO_MONSTER_TEMPLATES || TEMPLATES;
    for (const m of Object.values(all)) {
        if (m.id === canonId) return m;
    }
    return null;
}

const findings = [];
function report(severity, monsterId, monsterName, kind, detail) {
    findings.push({ severity, monsterId, monsterName, kind, detail });
}

const SEVERITY_ORDER = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'];
let auditedCount = 0;
let perfectMatches = 0;
let totalMismatchedFlags = 0;

// Severity per flag — gameplay-impact weighted
const FLAG_SEVERITY = {
    aggressive: 'HIGH',          // changes target acquisition completely
    canMove: 'HIGH',             // immobile vs mobile is huge
    canAttack: 'HIGH',           // can/can't attack
    looter: 'MEDIUM',            // picks up drops
    detector: 'MEDIUM',          // can see hidden players
    statusImmune: 'MEDIUM',      // immune to status effects
    knockbackImmune: 'MEDIUM',   // immune to knockback
    mvp: 'MEDIUM',               // MVP indicator
    assist: 'LOW',               // helps allies
    castSensorIdle: 'LOW', castSensorChase: 'LOW', noRandomWalk: 'LOW',
    angry: 'LOW', changeChase: 'LOW', changeTargetMelee: 'LOW',
    changeTargetChase: 'LOW', targetWeak: 'LOW', randomTarget: 'LOW'
};

for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const id = parseInt(idStr);
    if (focusId && id !== parseInt(focusId)) continue;
    const ours = findOurTemplate(id);
    if (!ours) continue;
    auditedCount++;

    const canonFlags = canonicalFlags(canon);
    const myFlags = ourFlags(ours);

    let monsterIssues = 0;
    for (const flag of Object.keys(MD)) {
        if (canonFlags[flag] !== myFlags[flag]) {
            const sev = FLAG_SEVERITY[flag] || 'LOW';
            report(sev, id, canon.name, `flag.${flag}`,
                `ours=${myFlags[flag]} canon=${canonFlags[flag]} (canonAi=${canon.ai}, ourAi=${MONSTER_AI_CODES[id] || `default(${ours.aiType})`})`);
            monsterIssues++;
            totalMismatchedFlags++;
        }
    }
    if (monsterIssues === 0) perfectMatches++;
}

function shouldShow(s) {
    if (severityFilter === 'ALL') return true;
    return SEVERITY_ORDER.indexOf(s) <= SEVERITY_ORDER.indexOf(severityFilter);
}

const filtered = findings.filter(f => shouldShow(f.severity));
const bySeverity = { CRITICAL: [], HIGH: [], MEDIUM: [], LOW: [] };
for (const f of filtered) bySeverity[f.severity].push(f);

console.log('═══════════════════════════════════════════════════════════════════');
console.log('  MONSTER AI MODE BITMASK AUDIT vs rAthena pre-re mob_db.yml');
console.log('═══════════════════════════════════════════════════════════════════');
console.log(`Audited monsters:    ${auditedCount}`);
console.log(`Perfect flag match:  ${perfectMatches}`);
console.log(`Total flag diffs:    ${totalMismatchedFlags}`);
console.log(`Findings (≤ ${severityFilter}):    ${filtered.length}`);
for (const sev of SEVERITY_ORDER) console.log(`  ${sev}:`.padEnd(10) + ` ${bySeverity[sev].length}`);
console.log('');

if (summaryOnly) {
    const byKind = {};
    for (const f of filtered) byKind[f.kind] = (byKind[f.kind] || 0) + 1;
    console.log('Findings by flag (top):');
    for (const [k, n] of Object.entries(byKind).sort((a, b) => b[1] - a[1]).slice(0, 30)) {
        console.log(`  ${n.toString().padStart(5)}  ${k}`);
    }
    process.exit(bySeverity.CRITICAL.length + bySeverity.HIGH.length > 0 ? 1 : 0);
}

for (const sev of SEVERITY_ORDER) {
    if (!bySeverity[sev].length) continue;
    console.log(`─── ${sev} (${bySeverity[sev].length}) ───`);
    for (const f of bySeverity[sev].slice(0, 80)) {
        console.log(`  [${f.monsterId}] ${f.monsterName.padEnd(20)} ${f.kind.padEnd(28)} ${f.detail}`);
    }
    if (bySeverity[sev].length > 80) console.log(`  ... +${bySeverity[sev].length - 80} more`);
    console.log('');
}

process.exit(bySeverity.CRITICAL.length + bySeverity.HIGH.length > 0 ? 1 : 0);
