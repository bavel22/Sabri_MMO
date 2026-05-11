#!/usr/bin/env node
/**
 * audit_monster_stats.js — V2
 *
 * Diff our ro_monster_templates.js values against AUTHORITATIVE canonical data
 * (mob_db_pre_re_canonical.json — sourced from Hercules pre-re mob_db.conf master).
 *
 * Severity:
 *   CRITICAL : wrong race / wrong size / wrong element type / HP off >25%
 *   HIGH     : ATK off >20%, wrong element level, EXP off >50%, DEF mismatch >5
 *   MEDIUM   : HP/ATK off 10-25%, MDEF mismatch, walkSpeed/attackDelay >20%
 *   LOW      : Stats (str/agi/vit/int/dex/luk) off by 5+
 *
 * Usage:
 *   node _audits/audit_monster_stats.js [--severity=ALL|CRITICAL|HIGH|MEDIUM|LOW] [--fix-suggestions]
 */

'use strict';

const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const TEMPLATES = require(path.join(ROOT, 'server/src/ro_monster_templates'));
// Prefer the FULL canonical reference (auto-extracted from rAthena master);
// fall back to the curated subset if the full file isn't present.
const FULL_PATH = path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json');
const CURATED_PATH = path.join(ROOT, '_audits/canonical/mob_db_pre_re_canonical.json');
const CANONICAL_PATH = fs.existsSync(FULL_PATH) ? FULL_PATH : CURATED_PATH;
console.log(`[audit] Using canonical reference: ${path.basename(CANONICAL_PATH)}`);
const CANONICAL = JSON.parse(fs.readFileSync(CANONICAL_PATH, 'utf8'));

const args = process.argv.slice(2);
const severityFilter = (args.find(a => a.startsWith('--severity='))?.split('=')[1] || 'ALL').toUpperCase();
const showFixes = args.includes('--fix-suggestions');
// By default, skip "MISSING_TEMPLATE" — these are canonical mobs we just haven't added (e.g. Guardians, Treasure Chests).
// Pass --include-missing to see them.
const includeMissing = args.includes('--include-missing');

const SEVERITY_ORDER = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'];
const findings = [];

function report(severity, monsterId, monsterName, field, ours, theirs, note) {
    findings.push({ severity, monsterId, monsterName, field, ours, theirs, note });
}

function pctDiff(a, b) {
    if (a == null || b == null) return null;
    if (b === 0) return a === 0 ? 0 : Infinity;
    return Math.abs((a - b) / b) * 100;
}

function findOurTemplate(canonId) {
    const all = TEMPLATES.RO_MONSTER_TEMPLATES || TEMPLATES;
    for (const m of Object.values(all)) {
        if (m.id === canonId) return m;
    }
    return null;
}

const monsters = CANONICAL.monsters;
let auditedCount = 0;
let missingCount = 0;
const fixSuggestions = [];

for (const [key, canon] of Object.entries(monsters)) {
    if (key.startsWith('_')) continue;
    const id = parseInt(key);
    const ours = findOurTemplate(id);
    if (!ours) {
        if (includeMissing) report('CRITICAL', id, canon.name, 'MISSING_TEMPLATE', '(not in templates)', `id ${id}`, 'Monster missing entirely');
        missingCount++;
        continue;
    }
    auditedCount++;

    // HP
    if (canon.hp != null) {
        const diff = pctDiff(ours.maxHealth, canon.hp);
        if (diff > 25) report('CRITICAL', id, canon.name, 'maxHealth', ours.maxHealth, canon.hp, `${diff.toFixed(0)}% off`);
        else if (diff > 10) report('MEDIUM', id, canon.name, 'maxHealth', ours.maxHealth, canon.hp, `${diff.toFixed(0)}% off`);
        else if (diff > 5) report('LOW', id, canon.name, 'maxHealth', ours.maxHealth, canon.hp, `${diff.toFixed(0)}% off`);
        if (diff > 5 && showFixes) {
            fixSuggestions.push(`templates['${ours.aegisName ? ours.aegisName.toLowerCase() : id}'].maxHealth = ${canon.hp}; // was ${ours.maxHealth}`);
        }
    }

    // ATK
    if (canon.atk1 != null && ours.attack !== canon.atk1) {
        const diff = pctDiff(ours.attack, canon.atk1);
        const sev = diff > 20 ? 'HIGH' : diff > 10 ? 'MEDIUM' : 'LOW';
        report(sev, id, canon.name, 'attack', ours.attack, canon.atk1, `${diff.toFixed(0)}% off`);
    }
    if (canon.atk2 != null && ours.attack2 !== canon.atk2) {
        const diff = pctDiff(ours.attack2, canon.atk2);
        const sev = diff > 20 ? 'HIGH' : diff > 10 ? 'MEDIUM' : 'LOW';
        report(sev, id, canon.name, 'attack2', ours.attack2, canon.atk2, `${diff.toFixed(0)}% off`);
    }

    // DEF / MDEF
    if (canon.def != null && (ours.defense || 0) !== canon.def) {
        const diff = Math.abs((ours.defense || 0) - canon.def);
        const sev = diff > 10 ? 'HIGH' : 'MEDIUM';
        report(sev, id, canon.name, 'defense', ours.defense || 0, canon.def, `off by ${diff}`);
    }
    if (canon.mdef != null && (ours.magicDefense || 0) !== canon.mdef) {
        const diff = Math.abs((ours.magicDefense || 0) - canon.mdef);
        const sev = diff > 15 ? 'HIGH' : 'MEDIUM';
        report(sev, id, canon.name, 'magicDefense', ours.magicDefense || 0, canon.mdef, `off by ${diff}`);
    }

    // Element + level
    if (canon.element != null && ours.element) {
        if (ours.element.type !== canon.element) {
            report('CRITICAL', id, canon.name, 'element.type', ours.element.type, canon.element, 'WRONG element');
        }
        if (canon.elementLevel != null && ours.element.level !== canon.elementLevel) {
            report('HIGH', id, canon.name, 'element.level', ours.element.level, canon.elementLevel, 'wrong level');
        }
    }

    // Race
    if (canon.race != null && ours.race !== canon.race) {
        report('CRITICAL', id, canon.name, 'race', ours.race, canon.race, 'WRONG race');
    }

    // Size
    if (canon.size != null && ours.size !== canon.size) {
        report('CRITICAL', id, canon.name, 'size', ours.size, canon.size, 'WRONG size');
    }

    // EXP
    if (canon.baseExp != null && ours.baseExp !== canon.baseExp) {
        const diff = pctDiff(ours.baseExp, canon.baseExp);
        if (diff > 50) report('HIGH', id, canon.name, 'baseExp', ours.baseExp, canon.baseExp, `${diff?.toFixed(0)}% off`);
        else if (diff > 20) report('MEDIUM', id, canon.name, 'baseExp', ours.baseExp, canon.baseExp, `${diff?.toFixed(0)}% off`);
    }
    if (canon.jobExp != null && ours.jobExp !== canon.jobExp) {
        const diff = pctDiff(ours.jobExp, canon.jobExp);
        if (diff > 50) report('HIGH', id, canon.name, 'jobExp', ours.jobExp, canon.jobExp, `${diff?.toFixed(0)}% off`);
        else if (diff > 20) report('MEDIUM', id, canon.name, 'jobExp', ours.jobExp, canon.jobExp, `${diff?.toFixed(0)}% off`);
    }

    // Stats — LOW severity (mob status doesn't matter for damage formula since atk1/atk2 is final)
    const statKeys = ['str', 'agi', 'vit', 'int', 'dex', 'luk'];
    for (const stat of statKeys) {
        if (canon[stat] != null && ours[stat] !== canon[stat]) {
            const diff = Math.abs(ours[stat] - canon[stat]);
            if (diff >= 10) report('LOW', id, canon.name, `stats.${stat}`, ours[stat], canon[stat], `off by ${diff}`);
        }
    }
}

function shouldShow(severity) {
    if (severityFilter === 'ALL') return true;
    return SEVERITY_ORDER.indexOf(severity) <= SEVERITY_ORDER.indexOf(severityFilter);
}

const filtered = findings.filter(f => shouldShow(f.severity));

console.log('═══════════════════════════════════════════════════════════════════');
console.log('  MONSTER STAT AUDIT V2 vs Hercules pre-renewal mob_db.conf');
console.log('═══════════════════════════════════════════════════════════════════');
console.log(`Audited: ${auditedCount} monsters from canonical reference`);
console.log(`Missing from our templates: ${missingCount}`);
console.log(`Findings (severity ≤ ${severityFilter}): ${filtered.length}`);
console.log('');

const bySeverity = { CRITICAL: [], HIGH: [], MEDIUM: [], LOW: [] };
for (const f of filtered) bySeverity[f.severity].push(f);

for (const sev of SEVERITY_ORDER) {
    if (!bySeverity[sev].length) continue;
    console.log(`─── ${sev} (${bySeverity[sev].length}) ───`);
    for (const f of bySeverity[sev]) {
        const ours = JSON.stringify(f.ours);
        const theirs = JSON.stringify(f.theirs);
        console.log(`  [${f.monsterId}] ${f.monsterName.padEnd(20)} ${f.field.padEnd(20)} ours=${ours.padEnd(10)} canon=${theirs.padEnd(10)} ${f.note || ''}`);
    }
    console.log('');
}

const criticalCount = bySeverity.CRITICAL.length;
console.log(`Total CRITICAL findings: ${criticalCount}`);
console.log(`Total HIGH findings: ${bySeverity.HIGH.length}`);
console.log(`Total MEDIUM findings: ${bySeverity.MEDIUM.length}`);
console.log(`Total LOW findings: ${bySeverity.LOW.length}`);

process.exit(criticalCount > 0 ? 1 : 0);
