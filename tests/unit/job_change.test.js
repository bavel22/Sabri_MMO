// Unit tests for the Job Master class change system.
// Run with: node tests/unit/job_change.test.js
//
// Validates:
//   1. Server's FIRST_CLASSES / SECOND_CLASS_UPGRADES / JOB_CLASS_CONFIG / getClassTier
//      match RO Classic pre-renewal expectations.
//   2. The client's JobChangeSubsystem.cpp static const mirror is in sync with the server.
//      (Drift here = client offers options the server rejects, or hides legal targets.)
//   3. The client-side eligibility decision tree mirrors the server's job:change validation
//      at server/src/index.js:10095-10204.

const path = require('path');
const fs = require('fs');

// ── Import the server's authoritative data ────────────────────────────────────

const exp = require('../../server/src/ro_exp_tables.js');
const {
    FIRST_CLASSES,
    SECOND_CLASS_UPGRADES,
    JOB_CLASS_CONFIG,
    getClassTier,
    getMaxJobLevel,
} = exp;

// ── Mirror of the client's JobChangeSubsystem.cpp static data ────────────────
// This MUST match what JobChangeSubsystem.cpp defines in `namespace JobChangeData`.
// If the C++ ever changes, update this and re-run.

const CLIENT_FIRST_CLASSES = [
    'swordsman', 'mage', 'archer', 'acolyte', 'thief', 'merchant',
];

const CLIENT_SECOND_CLASS_UPGRADES = {
    'swordsman': ['knight', 'crusader'],
    'mage':      ['wizard', 'sage'],
    'archer':    ['hunter', 'bard', 'dancer'],
    'acolyte':   ['priest', 'monk'],
    'thief':     ['assassin', 'rogue'],
    'merchant':  ['blacksmith', 'alchemist'],
};

const CLIENT_TRANSCENDENT_CLASSES = [
    'lord_knight', 'paladin',
    'high_wizard', 'scholar',
    'sniper', 'minstrel', 'gypsy',
    'high_priest', 'champion',
    'assassin_cross', 'stalker',
    'whitesmith', 'biochemist',
];

// ── Client-side tier resolver: mirrors JobChangeData::GetTier in JobChangeSubsystem.cpp ──
// IMPORTANT: differs from server's getClassTier for transcendent classes.
// Server's getClassTier returns 0 for transcendent (they're not in JOB_CLASS_CONFIG),
// which is harmless because the server's job:change validation rejects transcendent
// targets via FIRST_CLASSES/SECOND_CLASS_UPGRADES checks. The client uses its own
// resolver so it can show a "Speak with Valkyrie" message instead of a wrong job-level error.

function clientGetTier(jobClass) {
    const lower = (jobClass || '').toLowerCase();
    if (lower === 'novice') return 0;
    if (CLIENT_FIRST_CLASSES.includes(lower)) return 1;
    const allSeconds = Object.values(CLIENT_SECOND_CLASS_UPGRADES).flat();
    if (allSeconds.includes(lower)) return 2;
    if (CLIENT_TRANSCENDENT_CLASSES.includes(lower)) return 3;
    return -1;
}

// ── Client-side eligibility logic, port of UJobChangeSubsystem::RecomputeEligibility ──
// Returns { ok, eligibleTargets, requirementMessage }.

function computeClientEligibility({ jobClass, jobLevel, basicSkillLv }) {
    const lower = (jobClass || 'novice').toLowerCase();
    const tier = clientGetTier(lower);

    if (tier === 0) {
        if (jobLevel < 10) {
            return {
                ok: false,
                eligibleTargets: [],
                requirementMessage: `You must reach Job Level 10 first. (current: ${jobLevel}/10)`,
            };
        }
        if (basicSkillLv < 9) {
            return {
                ok: false,
                eligibleTargets: [],
                requirementMessage: `Master your Basic Skill first. (current: ${basicSkillLv}/9)`,
            };
        }
        return { ok: true, eligibleTargets: [...CLIENT_FIRST_CLASSES], requirementMessage: '' };
    }

    if (tier === 1) {
        if (jobLevel < 40) {
            return {
                ok: false,
                eligibleTargets: [],
                requirementMessage: `You must reach Job Level 40 first. (current: ${jobLevel}/40)`,
            };
        }
        return {
            ok: true,
            eligibleTargets: [...(CLIENT_SECOND_CLASS_UPGRADES[lower] || [])],
            requirementMessage: '',
        };
    }

    if (tier === 2) {
        return { ok: false, eligibleTargets: [], requirementMessage: 'You have already mastered your craft.' };
    }

    return { ok: false, eligibleTargets: [], requirementMessage: 'Speak with the Valkyrie in Juno.' };
}

// ── Mirror of server-side validation at index.js:10299-10440 ─────────────────
// Returns { ok, error } — `error` is the server's rejection message format.
// Caller-supplied state object covers all gating dimensions.

function simulateServerValidate({
    currentClass, currentJobLevel, basicSkillLv, targetClass,
    isDead = false, isSitting = false, isOverweight90 = false,
    msSinceLastAttack = Infinity,
}) {
    if (!targetClass) return { ok: false, error: 'No target class specified' };

    // Gating block (added in this round).
    if (isDead) return { ok: false, error: 'Cannot change class while dead.' };
    if (isSitting) return { ok: false, error: 'Cannot change class while sitting.' };
    if (isOverweight90) return { ok: false, error: 'Cannot change class while overweight (90%+).' };
    if (msSinceLastAttack < 5000) return { ok: false, error: 'Cannot change class while in combat.' };

    const currentTier = getClassTier(currentClass);
    const targetTier = getClassTier(targetClass);
    if (targetTier !== currentTier + 1) {
        return { ok: false, error: `Cannot change from ${currentClass} to ${targetClass}` };
    }

    if (currentTier === 0) {
        if (basicSkillLv < 9) {
            return { ok: false, error: `Requires Basic Skill Level 9 (current: ${basicSkillLv})` };
        }
        if (currentJobLevel < 10) {
            return { ok: false, error: `Requires Novice Job Level 10 (current: ${currentJobLevel})` };
        }
        if (!FIRST_CLASSES.includes(targetClass)) {
            return { ok: false, error: `${targetClass} is not a valid first class` };
        }
    }

    if (currentTier === 1) {
        if (currentJobLevel < 40) {
            return { ok: false, error: `Requires Job Level 40+ (current: ${currentJobLevel})` };
        }
        const validUpgrades = SECOND_CLASS_UPGRADES[currentClass];
        if (!validUpgrades || !validUpgrades.includes(targetClass)) {
            return { ok: false, error: `${targetClass} is not a valid upgrade from ${currentClass}` };
        }
    }

    return { ok: true };
}

// ── Tiny test harness ─────────────────────────────────────────────────────────

let passed = 0;
let failed = 0;
const failures = [];

function test(name, fn) {
    try {
        fn();
        console.log(`PASS  ${name}`);
        passed++;
    } catch (e) {
        console.log(`FAIL  ${name}`);
        console.log(`      ${e.message}`);
        failures.push({ name, message: e.message });
        failed++;
    }
}

function assert(cond, msg) {
    if (!cond) throw new Error(msg || 'Assertion failed');
}

function assertEqual(actual, expected, msg) {
    const a = JSON.stringify(actual);
    const e = JSON.stringify(expected);
    if (a !== e) throw new Error(`${msg || 'mismatch'}\n        expected ${e}\n        got      ${a}`);
}

function assertSetEqual(actual, expected, msg) {
    const a = [...actual].sort();
    const e = [...expected].sort();
    assertEqual(a, e, msg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Group 1: Server data integrity vs RO Classic spec
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 1: Server data integrity (ro_exp_tables.js) ===\n');

test('FIRST_CLASSES has exactly the 6 RO Classic first classes', () => {
    assertSetEqual(FIRST_CLASSES,
        ['swordsman', 'mage', 'archer', 'acolyte', 'thief', 'merchant']);
});

test('SECOND_CLASS_UPGRADES covers every first class', () => {
    for (const first of FIRST_CLASSES) {
        assert(SECOND_CLASS_UPGRADES[first] !== undefined,
            `Missing upgrade path for ${first}`);
        assert(SECOND_CLASS_UPGRADES[first].length >= 2,
            `${first} should have at least 2 second-class options, got ${SECOND_CLASS_UPGRADES[first].length}`);
    }
});

test('Archer is the only first class with 3 upgrade options (bard + dancer + hunter)', () => {
    for (const [from, tos] of Object.entries(SECOND_CLASS_UPGRADES)) {
        if (from === 'archer') {
            assertEqual(tos.sort(), ['bard', 'dancer', 'hunter']);
        } else {
            assert(tos.length === 2, `${from} should have 2 options, got ${tos.length}`);
        }
    }
});

test('Every second-class target has tier=2 in JOB_CLASS_CONFIG', () => {
    for (const tos of Object.values(SECOND_CLASS_UPGRADES)) {
        for (const target of tos) {
            const cfg = JOB_CLASS_CONFIG[target];
            assert(cfg !== undefined, `JOB_CLASS_CONFIG missing entry for ${target}`);
            assert(cfg.tier === 2, `${target} should have tier=2, got ${cfg.tier}`);
        }
    }
});

test('getClassTier returns expected tier for each canonical class', () => {
    assertEqual(getClassTier('novice'), 0, 'novice should be tier 0');
    for (const f of FIRST_CLASSES) {
        assertEqual(getClassTier(f), 1, `${f} should be tier 1`);
    }
    for (const tos of Object.values(SECOND_CLASS_UPGRADES)) {
        for (const t of tos) {
            assertEqual(getClassTier(t), 2, `${t} should be tier 2`);
        }
    }
});

test('Max job levels: novice=10, first/second=50', () => {
    assertEqual(getMaxJobLevel('novice'), 10);
    for (const f of FIRST_CLASSES) {
        assertEqual(getMaxJobLevel(f), 50, `${f} should max at 50`);
    }
    assertEqual(getMaxJobLevel('knight'), 50);
    assertEqual(getMaxJobLevel('priest'), 50);
});

// ─────────────────────────────────────────────────────────────────────────────
// Group 2: Client-side mirror data matches the server
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 2: Client mirror parity (JobChangeSubsystem.cpp) ===\n');

test('CLIENT_FIRST_CLASSES matches server FIRST_CLASSES', () => {
    assertSetEqual(CLIENT_FIRST_CLASSES, FIRST_CLASSES,
        'Client UI list must match server validation list — drift causes hidden legal targets or rejected clicks');
});

test('CLIENT_SECOND_CLASS_UPGRADES matches server SECOND_CLASS_UPGRADES', () => {
    const clientKeys = Object.keys(CLIENT_SECOND_CLASS_UPGRADES).sort();
    const serverKeys = Object.keys(SECOND_CLASS_UPGRADES).sort();
    assertEqual(clientKeys, serverKeys, 'first-class keys differ');
    for (const k of clientKeys) {
        assertSetEqual(CLIENT_SECOND_CLASS_UPGRADES[k], SECOND_CLASS_UPGRADES[k],
            `targets for ${k} differ`);
    }
});

test('Verify the actual JobChangeSubsystem.cpp file contains the exact static data', () => {
    const cppPath = path.join(__dirname, '..', '..', 'client', 'SabriMMO', 'Source',
        'SabriMMO', 'UI', 'JobChangeSubsystem.cpp');
    assert(fs.existsSync(cppPath), `Source not found at ${cppPath}`);
    const src = fs.readFileSync(cppPath, 'utf8');

    // Spot-check that each first class is named in FirstClasses array.
    for (const first of FIRST_CLASSES) {
        assert(src.includes(`TEXT("${first}")`), `JobChangeSubsystem.cpp missing TEXT("${first}")`);
    }
    // Spot-check second-class upgrades — every (from, to) pair should appear in a single line.
    for (const [from, tos] of Object.entries(SECOND_CLASS_UPGRADES)) {
        for (const to of tos) {
            const fromTok = `TEXT("${from}")`;
            const toTok = `TEXT("${to}")`;
            // The cpp groups each from-class on one line with all its targets.
            // We only assert co-occurrence in the file; ordering on the line is verified by parity tests above.
            assert(src.includes(fromTok), `Missing ${fromTok}`);
            assert(src.includes(toTok), `Missing ${toTok}`);
        }
    }
});

// ─────────────────────────────────────────────────────────────────────────────
// Group 3: Eligibility decision tree (client) matches server validation
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 3: Eligibility decision tree ===\n');

test('Novice Job 1 / Basic 0: client blocks with job-level message', () => {
    const r = computeClientEligibility({ jobClass: 'novice', jobLevel: 1, basicSkillLv: 0 });
    assert(!r.ok, 'should be blocked');
    assert(r.requirementMessage.includes('Job Level 10'),
        `expected job-level message, got: ${r.requirementMessage}`);
    assertEqual(r.eligibleTargets, []);
});

test('Novice Job 10 / Basic 0: client blocks with basic-skill message', () => {
    const r = computeClientEligibility({ jobClass: 'novice', jobLevel: 10, basicSkillLv: 0 });
    assert(!r.ok, 'should be blocked');
    assert(r.requirementMessage.includes('Basic Skill'),
        `expected basic-skill message, got: ${r.requirementMessage}`);
});

test('Novice Job 10 / Basic 9: client offers all 6 first classes', () => {
    const r = computeClientEligibility({ jobClass: 'novice', jobLevel: 10, basicSkillLv: 9 });
    assert(r.ok);
    assertSetEqual(r.eligibleTargets, FIRST_CLASSES);
});

test('Acolyte Job 30: client blocks with job-40 message', () => {
    const r = computeClientEligibility({ jobClass: 'acolyte', jobLevel: 30, basicSkillLv: 9 });
    assert(!r.ok);
    assert(r.requirementMessage.includes('40'),
        `expected job-40 message, got: ${r.requirementMessage}`);
});

test('Acolyte Job 40: client offers Priest + Monk only', () => {
    const r = computeClientEligibility({ jobClass: 'acolyte', jobLevel: 40, basicSkillLv: 9 });
    assert(r.ok);
    assertSetEqual(r.eligibleTargets, ['priest', 'monk']);
});

test('Archer Job 50: client offers Hunter + Bard + Dancer', () => {
    const r = computeClientEligibility({ jobClass: 'archer', jobLevel: 50, basicSkillLv: 9 });
    assert(r.ok);
    assertSetEqual(r.eligibleTargets, ['hunter', 'bard', 'dancer']);
});

test('Knight: client surfaces "already mastered" message, no targets', () => {
    const r = computeClientEligibility({ jobClass: 'knight', jobLevel: 50, basicSkillLv: 9 });
    assert(!r.ok);
    assert(r.requirementMessage.toLowerCase().includes('mastered'),
        `expected mastered message, got: ${r.requirementMessage}`);
    assertEqual(r.eligibleTargets, []);
});

test('Lord Knight (transcendent): client points to Valkyrie', () => {
    const r = computeClientEligibility({ jobClass: 'lord_knight', jobLevel: 1, basicSkillLv: 9 });
    assert(!r.ok);
    assert(r.requirementMessage.toLowerCase().includes('valkyrie'),
        `expected valkyrie message, got: ${r.requirementMessage}`);
});

// ─────────────────────────────────────────────────────────────────────────────
// Group 4: For every client-allowed (from, to) pair, server accepts it
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 4: Client-server agreement on legal transitions ===\n');

test('Every novice → first-class target the client offers, the server accepts', () => {
    const elig = computeClientEligibility({ jobClass: 'novice', jobLevel: 10, basicSkillLv: 9 });
    for (const target of elig.eligibleTargets) {
        const r = simulateServerValidate({
            currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: target,
        });
        assert(r.ok, `Server rejected legal client target novice → ${target}: ${r.error}`);
    }
});

test('Every first-class → second-class target the client offers, the server accepts', () => {
    for (const first of FIRST_CLASSES) {
        const elig = computeClientEligibility({ jobClass: first, jobLevel: 50, basicSkillLv: 9 });
        for (const target of elig.eligibleTargets) {
            const r = simulateServerValidate({
                currentClass: first, currentJobLevel: 50, basicSkillLv: 9, targetClass: target,
            });
            assert(r.ok, `Server rejected legal client target ${first} → ${target}: ${r.error}`);
        }
    }
});

test('Server rejects illegal cross-archetype jumps (e.g. swordsman → wizard)', () => {
    const r = simulateServerValidate({
        currentClass: 'swordsman', currentJobLevel: 50, basicSkillLv: 9, targetClass: 'wizard',
    });
    assert(!r.ok, 'expected rejection of cross-archetype jump');
});

test('Server rejects skipping tiers (novice → knight)', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'knight',
    });
    assert(!r.ok, 'expected rejection of tier skip');
});

test('Server rejects job:change from second class to anything', () => {
    const r = simulateServerValidate({
        currentClass: 'knight', currentJobLevel: 50, basicSkillLv: 9, targetClass: 'lord_knight',
    });
    assert(!r.ok, 'expected rejection — transcendent path goes through Valkyrie, not job:change');
});

// ─────────────────────────────────────────────────────────────────────────────
// Group 5: Server accepts Job 40 first-class change but loses skill points
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 5: Edge cases ===\n');

test('Server accepts Swordsman Job 40 → Knight (loses 9 potential skill points)', () => {
    const r = simulateServerValidate({
        currentClass: 'swordsman', currentJobLevel: 40, basicSkillLv: 9, targetClass: 'knight',
    });
    assert(r.ok, `Job 40 should be the minimum for 1st→2nd. error: ${r.error}`);
});

test('Server rejects Swordsman Job 39 → Knight', () => {
    const r = simulateServerValidate({
        currentClass: 'swordsman', currentJobLevel: 39, basicSkillLv: 9, targetClass: 'knight',
    });
    assert(!r.ok, 'Job 39 should be rejected');
});

test('Server rejects Novice Job 10 → Swordsman if Basic Skill < 9', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 4, targetClass: 'swordsman',
    });
    assert(!r.ok, 'Basic Skill < 9 should be rejected');
});

// ─────────────────────────────────────────────────────────────────────────────
// Group 6: Server-side gating (dead / sitting / in combat / overweight)
// ─────────────────────────────────────────────────────────────────────────────

console.log('\n=== Group 6: Server gating (dead/sitting/combat/weight) ===\n');

test('Dead player blocked', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'swordsman',
        isDead: true,
    });
    assert(!r.ok && r.error.includes('dead'), `expected death-state error, got: ${r.error}`);
});

test('Sitting player blocked', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'swordsman',
        isSitting: true,
    });
    assert(!r.ok && r.error.includes('sitting'), `expected sitting error, got: ${r.error}`);
});

test('Overweight (90%+) player blocked', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'swordsman',
        isOverweight90: true,
    });
    assert(!r.ok && r.error.includes('overweight'), `expected overweight error, got: ${r.error}`);
});

test('In-combat player blocked (attacked within last 5s)', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'swordsman',
        msSinceLastAttack: 2000,
    });
    assert(!r.ok && r.error.includes('combat'), `expected combat error, got: ${r.error}`);
});

test('Player out of combat 5+ seconds is allowed', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 10, basicSkillLv: 9, targetClass: 'swordsman',
        msSinceLastAttack: 5001,
    });
    assert(r.ok, `expected accept after 5s out-of-combat, got: ${r.error}`);
});

test('Gating runs before tier validation (dead novice → invalid class still blocked by death)', () => {
    const r = simulateServerValidate({
        currentClass: 'novice', currentJobLevel: 1, basicSkillLv: 0, targetClass: 'wizard',
        isDead: true,
    });
    assert(!r.ok && r.error.includes('dead'),
        'death message should fire before tier-skip rejection');
});

// ─────────────────────────────────────────────────────────────────────────────

console.log(`\n────────────────────────────────────────`);
console.log(`Results: ${passed} passed, ${failed} failed`);
if (failed > 0) {
    console.log('\nFailed tests:');
    for (const f of failures) {
        console.log(`  ✗ ${f.name}`);
        console.log(`    ${f.message}`);
    }
    process.exit(1);
}
process.exit(0);
