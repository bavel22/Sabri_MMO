#!/usr/bin/env node
/**
 * audit_monster_drops.js — V2 (item-id matching, not name)
 *
 * Compare our monster drop tables against canonical pre-renewal drops.
 *
 * V1 matched by display name and produced 151 missing + 151 extra findings caused by
 * subtle display-name divergences (e.g. "Animal's Skin" with apostrophe in our data
 * vs "Animal Skin" without in canonical item_db). V2 resolves both sides to canonical
 * item IDs and compares IDs — which is what the runtime drop resolver actually does.
 *
 * Each drop entry from our side is resolved to canonical id by:
 *   1. Direct match: canonical.items.find(i => i.name == ourItemName)
 *   2. Apostrophe-strip: try stripping apostrophes from ourItemName and re-match
 *   3. Aegis match: try our name as if it were an AegisName (for raw drops)
 * If none match → flag as UNRESOLVABLE_OUR_NAME (real bug — drop won't fire).
 *
 * Each canonical drop entry resolves to id via aegisToId.
 *
 * Usage:
 *   node _audits/audit_monster_drops.js [--severity=ALL|HIGH|MEDIUM|LOW] [--summary] [--id=N]
 */

'use strict';

const path = require('path');
const fs = require('fs');

const ROOT = path.resolve(__dirname, '..');
const TEMPLATES = require(path.join(ROOT, 'server/src/ro_monster_templates'));
const CANONICAL = JSON.parse(fs.readFileSync(
    path.join(ROOT, '_audits/canonical/mob_db_pre_re_full.json'), 'utf8'
)).monsters;
const ITEM_DB = JSON.parse(fs.readFileSync(
    path.join(ROOT, '_audits/canonical/item_db_pre_re_full.json'), 'utf8'
));

const args = process.argv.slice(2);
const severityFilter = (args.find(a => a.startsWith('--severity='))?.split('=')[1] || 'ALL').toUpperCase();
const focusId = args.find(a => a.startsWith('--id='))?.split('=')[1];
const summaryOnly = args.includes('--summary');

// Build display-name → id map.
// Match server runtime behavior: itemNameToId.set(name, id) is called for each row in
// SELECT * FROM items ORDER BY item_id. Map.set OVERWRITES, so for duplicate display names
// (e.g., Knife 1201 vs Knife_ 1202 both display "Knife"), the HIGHEST id wins.
// This mirrors server/src/index.js line 5257 (itemNameToId.set(row.name, row.item_id)).
const nameToId = new Map();
const sortedIds = Object.keys(ITEM_DB.items).map(Number).sort((a, b) => a - b);
for (const id of sortedIds) {
    const item = ITEM_DB.items[id];
    if (item.name) nameToId.set(item.name.toLowerCase(), id);  // last write wins (highest id)
}

// Resolve our drop name → canonical item ID (or null + reason)
function resolveOurDropToId(name) {
    if (!name) return { id: null, reason: 'empty_name' };
    const lower = name.toLowerCase();
    if (nameToId.has(lower)) return { id: nameToId.get(lower), reason: 'exact' };
    // Try apostrophe-strip on our name (our "Animal's Skin" → "Animal Skin")
    const stripped = name.replace(/'/g, '').toLowerCase();
    if (nameToId.has(stripped)) return { id: nameToId.get(stripped), reason: 'apostrophe_stripped', canonName: ITEM_DB.items[nameToId.get(stripped)].name };
    // Try as aegis name (with underscore for spaces)
    const asAegis = name.replace(/ /g, '_');
    if (ITEM_DB.aegisToId[asAegis]) {
        const id = ITEM_DB.aegisToId[asAegis];
        return { id, reason: 'matched_as_aegis', canonName: ITEM_DB.items[id].name };
    }
    return { id: null, reason: 'unresolvable' };
}

function findOurTemplate(canonId) {
    const all = TEMPLATES.RO_MONSTER_TEMPLATES || TEMPLATES;
    for (const m of Object.values(all)) {
        if (m.id === canonId) return m;
    }
    return null;
}

function rateSeverity(ourRate, canonRate) {
    const canonPct = canonRate / 100;
    if (ourRate === canonPct) return null;
    const diff = Math.abs(ourRate - canonPct);
    const pctOff = canonPct === 0 ? 100 : (diff / canonPct) * 100;
    if (pctOff > 50) return 'HIGH';
    if (pctOff > 20) return 'MEDIUM';
    return 'LOW';
}

const findings = [];
function report(severity, monsterId, monsterName, kind, detail) {
    findings.push({ severity, monsterId, monsterName, kind, detail });
}

const SEVERITY_ORDER = ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'];
let auditedCount = 0;
let perfectMatches = 0;

for (const [idStr, canon] of Object.entries(CANONICAL)) {
    const id = parseInt(idStr);
    if (focusId && id !== parseInt(focusId)) continue;
    const ours = findOurTemplate(id);
    if (!ours) continue;
    auditedCount++;

    const canonDrops = canon.drops || [];
    const ourDrops = ours.drops || [];
    if (canonDrops.length === 0 && ourDrops.length === 0) { perfectMatches++; continue; }

    // Resolve our drops to canonical item IDs (preserving duplicates as a list).
    // Server runtime: resolveDropItemIds() prefers d.itemId over d.itemName lookup.
    // Mirror that: if itemId is present in our template, use it directly.
    const oursById = new Map();
    for (const od of ourDrops) {
        let resolvedId;
        if (od.itemId != null) {
            resolvedId = od.itemId;
        } else {
            const res = resolveOurDropToId(od.itemName);
            if (res.id == null) {
                report('HIGH', id, canon.name, 'unresolvable_our_name',
                    `"${od.itemName}" doesn't resolve to any canonical item — runtime drop will silently fail`);
                continue;
            }
            if (res.reason === 'apostrophe_stripped') {
                report('LOW', id, canon.name, 'wrong_display_name',
                    `"${od.itemName}" should be "${res.canonName}" (canonical display has no apostrophe)`);
            }
            resolvedId = res.id;
        }
        if (!oursById.has(resolvedId)) oursById.set(resolvedId, []);
        oursById.get(resolvedId).push(od);
    }

    // Group canonical drops by id too (handles canonical duplicates like Poring's 2 Apples)
    const canonById = new Map();
    for (const cd of canonDrops) {
        const canonId = ITEM_DB.aegisToId[cd.item];
        if (!canonId) {
            report('LOW', id, canon.name, 'unresolvable_canon_aegis',
                `canonical aegis "${cd.item}" not in item_db — likely Renewal-only item`);
            continue;
        }
        if (!canonById.has(canonId)) canonById.set(canonId, []);
        canonById.get(canonId).push(cd);
    }

    const canonIdsSeen = new Set();
    for (const [canonId, canonGroup] of canonById) {
        canonIdsSeen.add(canonId);
        const ourGroup = oursById.get(canonId) || [];

        // Sort both by rate descending so we match high-rate to high-rate
        const sortedCanon = canonGroup.slice().sort((a, b) => b.rate - a.rate);
        const sortedOurs = ourGroup.slice().sort((a, b) => b.rate - a.rate);

        // Pair off
        const maxLen = Math.max(sortedCanon.length, sortedOurs.length);
        for (let i = 0; i < maxLen; i++) {
            const cd = sortedCanon[i];
            const od = sortedOurs[i];

            if (cd && !od) {
                const sev = cd.stealProtected ? 'HIGH' : 'MEDIUM';
                report(sev, id, canon.name, 'missing_drop',
                    `${ITEM_DB.items[canonId].name} (id ${canonId}, canon ${cd.item} rate ${cd.rate / 100}%${cd.stealProtected ? ' — card' : ''})`);
                continue;
            }
            if (od && !cd) {
                report('LOW', id, canon.name, 'extra_drop_dupe',
                    `${od.itemName} (id ${canonId}, rate ${od.rate}%) — extra duplicate; canonical only has ${sortedCanon.length} entries for this item`);
                continue;
            }

            // Both exist — check rate / stealProtected
            const sev = rateSeverity(od.rate, cd.rate);
            if (sev) {
                report(sev, id, canon.name, 'rate_mismatch',
                    `${ITEM_DB.items[canonId].name} ours=${od.rate}% canon=${(cd.rate / 100)}%`);
            }
            const ourSP = !!od.stealProtected;
            const canonSP = !!cd.stealProtected;
            if (ourSP !== canonSP) {
                report('MEDIUM', id, canon.name, 'stealProtected_mismatch',
                    `${ITEM_DB.items[canonId].name} ours=${ourSP} canon=${canonSP}`);
            }
            if (canonId >= 4001 && canonId <= 4419 && !ourSP) {
                report('HIGH', id, canon.name, 'card_not_steal_protected',
                    `${ITEM_DB.items[canonId].name} is a card but stealProtected is false`);
            }
        }
    }

    // Items we have that canonical doesn't have at all
    for (const [resolvedId, oursList] of oursById) {
        if (!canonIdsSeen.has(resolvedId)) {
            for (const od of oursList) {
                report('LOW', id, canon.name, 'extra_drop',
                    `${od.itemName} (resolves to id ${resolvedId}, rate ${od.rate}%) — not in canonical`);
            }
        }
    }
}

function shouldShow(s) {
    if (severityFilter === 'ALL') return true;
    return SEVERITY_ORDER.indexOf(s) <= SEVERITY_ORDER.indexOf(severityFilter);
}

const filtered = findings.filter(f => shouldShow(f.severity));
const bySeverity = { CRITICAL: [], HIGH: [], MEDIUM: [], LOW: [] };
for (const f of filtered) bySeverity[f.severity].push(f);

console.log('═══════════════════════════════════════════════════════════════════');
console.log('  MONSTER DROP TABLE AUDIT V2 (item-id matching)');
console.log('═══════════════════════════════════════════════════════════════════');
console.log(`Audited monsters:        ${auditedCount}`);
console.log(`Findings (≤ ${severityFilter}):       ${filtered.length}`);
for (const sev of SEVERITY_ORDER) console.log(`  ${sev}:`.padEnd(10) + ` ${bySeverity[sev].length}`);
console.log('');

if (summaryOnly) {
    const byKind = {};
    for (const f of filtered) byKind[f.kind] = (byKind[f.kind] || 0) + 1;
    console.log('Findings by kind:');
    for (const [k, n] of Object.entries(byKind).sort((a, b) => b[1] - a[1])) {
        console.log(`  ${n.toString().padStart(5)}  ${k}`);
    }
    process.exit(bySeverity.CRITICAL.length + bySeverity.HIGH.length > 0 ? 1 : 0);
}

for (const sev of SEVERITY_ORDER) {
    if (!bySeverity[sev].length) continue;
    console.log(`─── ${sev} (${bySeverity[sev].length}) ───`);
    for (const f of bySeverity[sev].slice(0, 60)) {
        console.log(`  [${f.monsterId}] ${f.monsterName.padEnd(20)} ${f.kind.padEnd(28)} ${f.detail}`);
    }
    if (bySeverity[sev].length > 60) console.log(`  ... +${bySeverity[sev].length - 60} more`);
    console.log('');
}

process.exit(bySeverity.CRITICAL.length + bySeverity.HIGH.length > 0 ? 1 : 0);
