/**
 * test_element_table.js — Comprehensive element table verification
 *
 * Verifies ELEMENT_TABLE against rAthena pre-renewal attr_fix.yml canonical values.
 * Also tests SIZE_PENALTY, card modifier stacking, and element modifier edge cases.
 *
 * Run: node server/tests/test_element_table.js
 */

'use strict';

const {
    ELEMENT_TABLE, SIZE_PENALTY,
    getElementModifier, getSizePenalty,
    calculatePhysicalDamage, calculateMagicalDamage,
    calculateDerivedStats
} = require('../src/ro_damage_formulas.js');

let passed = 0;
let failed = 0;
const failures = [];

function assert(condition, message) {
    if (condition) {
        passed++;
    } else {
        failed++;
        failures.push(message);
        console.log(`  FAIL: ${message}`);
    }
}

function assertEqual(actual, expected, message) {
    if (actual === expected) {
        passed++;
    } else {
        failed++;
        const msg = `${message} — expected ${expected}, got ${actual}`;
        failures.push(msg);
        console.log(`  FAIL: ${msg}`);
    }
}

// ============================================================
// Test 1: Element Table Structure
// ============================================================
console.log('\n=== Test 1: Element Table Structure ===');

const ELEMENTS = ['neutral', 'water', 'earth', 'fire', 'wind', 'poison', 'holy', 'shadow', 'ghost', 'undead'];

for (const atkEle of ELEMENTS) {
    assert(ELEMENT_TABLE[atkEle] !== undefined, `Attack element "${atkEle}" exists`);
    for (const defEle of ELEMENTS) {
        const row = ELEMENT_TABLE[atkEle][defEle];
        assert(row !== undefined, `${atkEle} vs ${defEle} entry exists`);
        assert(Array.isArray(row) && row.length === 4, `${atkEle} vs ${defEle} has 4 levels`);
    }
}

// ============================================================
// Test 2: Key Element Interactions (Level 1)
// Verified against rAthena db/pre-re/attr_fix.yml
// ============================================================
console.log('\n=== Test 2: Key Element Interactions (Lv1) ===');

// Fire/Water/Earth/Wind rock-paper-scissors
assertEqual(getElementModifier('water', 'fire', 1), 150, 'Water vs Fire Lv1 = 150%');
assertEqual(getElementModifier('fire', 'earth', 1), 150, 'Fire vs Earth Lv1 = 150% (WRONG before: was missing)');
assertEqual(getElementModifier('earth', 'wind', 1), 150, 'Earth vs Wind Lv1 = 150%');
assertEqual(getElementModifier('wind', 'water', 1), 175, 'Wind vs Water Lv1 = 175%');

// Reverse: weak against what beats you
assertEqual(getElementModifier('fire', 'water', 1), 50, 'Fire vs Water Lv1 = 50% (was 90!)');
assertEqual(getElementModifier('water', 'wind', 1), 50, 'Water vs Wind Lv1 = 50% (was 90!)');
assertEqual(getElementModifier('wind', 'earth', 1), 50, 'Wind vs Earth Lv1 = 50% (was 90!)');
assertEqual(getElementModifier('earth', 'fire', 1), 50, 'Earth vs Fire Lv1 = 50% (was 90!)');

// Same element
assertEqual(getElementModifier('fire', 'fire', 1), 25, 'Fire vs Fire Lv1 = 25%');
assertEqual(getElementModifier('water', 'water', 1), 25, 'Water vs Water Lv1 = 25%');
assertEqual(getElementModifier('earth', 'earth', 1), 100, 'Earth vs Earth Lv1 = 100%');

// Ghost special rules
assertEqual(getElementModifier('neutral', 'ghost', 1), 25, 'Neutral vs Ghost Lv1 = 25%');
assertEqual(getElementModifier('neutral', 'ghost', 2), 25, 'Neutral vs Ghost Lv2 = 25% (was 0!)');
assertEqual(getElementModifier('neutral', 'ghost', 3), 0, 'Neutral vs Ghost Lv3 = 0%');
assertEqual(getElementModifier('ghost', 'neutral', 1), 25, 'Ghost vs Neutral Lv1 = 25% (was 100!)');
assertEqual(getElementModifier('ghost', 'neutral', 2), 0, 'Ghost vs Neutral Lv2 = 0%');
assertEqual(getElementModifier('ghost', 'ghost', 1), 125, 'Ghost vs Ghost Lv1 = 125%');

// Holy/Shadow opposition
assertEqual(getElementModifier('holy', 'shadow', 1), 125, 'Holy vs Shadow Lv1 = 125%');
assertEqual(getElementModifier('shadow', 'holy', 1), 125, 'Shadow vs Holy Lv1 = 125%');
assertEqual(getElementModifier('holy', 'holy', 1), 0, 'Holy vs Holy Lv1 = 0% (immune)');
assertEqual(getElementModifier('shadow', 'shadow', 1), 0, 'Shadow vs Shadow Lv1 = 0% (immune)');

// Undead interactions — MAJOR FIXES
assertEqual(getElementModifier('undead', 'shadow', 1), 0, 'Undead vs Shadow Lv1 = 0% (was 100!)');
assertEqual(getElementModifier('undead', 'poison', 1), 50, 'Undead vs Poison Lv1 = 50% (was -50!)');
assertEqual(getElementModifier('undead', 'ghost', 1), 100, 'Undead vs Ghost Lv1 = 100% (was 50!)');
assertEqual(getElementModifier('undead', 'holy', 1), 100, 'Undead vs Holy Lv1 = 100% (was 125!)');
assertEqual(getElementModifier('undead', 'undead', 1), 0, 'Undead vs Undead Lv1 = 0% (immune)');
assertEqual(getElementModifier('holy', 'undead', 1), 150, 'Holy vs Undead Lv1 = 150%');

// Poison interactions — FIXES
assertEqual(getElementModifier('poison', 'earth', 1), 125, 'Poison vs Earth Lv1 = 125% (was 100!)');
assertEqual(getElementModifier('poison', 'fire', 1), 125, 'Poison vs Fire Lv1 = 125% (was 100!)');
assertEqual(getElementModifier('poison', 'wind', 1), 125, 'Poison vs Wind Lv1 = 125% (was 100!)');
assertEqual(getElementModifier('poison', 'holy', 1), 75, 'Poison vs Holy Lv1 = 75% (was 125!)');
assertEqual(getElementModifier('poison', 'undead', 1), -25, 'Poison vs Undead Lv1 = -25% (heals)');
assertEqual(getElementModifier('poison', 'poison', 1), 0, 'Poison vs Poison Lv1 = 0% (immune)');

// Ghost vs Undead — FIX
assertEqual(getElementModifier('ghost', 'undead', 1), 100, 'Ghost vs Undead Lv1 = 100% (was 50!)');

// ============================================================
// Test 3: Higher Level Elements (Lv2-4)
// ============================================================
console.log('\n=== Test 3: Higher Level Elements (Lv2-4) ===');

// Same element at higher levels → negative (heals)
assertEqual(getElementModifier('water', 'water', 2), 0, 'Water vs Water Lv2 = 0%');
assertEqual(getElementModifier('water', 'water', 3), -25, 'Water vs Water Lv3 = -25% (heals)');
assertEqual(getElementModifier('water', 'water', 4), -50, 'Water vs Water Lv4 = -50% (heals)');
assertEqual(getElementModifier('fire', 'fire', 3), -25, 'Fire vs Fire Lv3 = -25% (heals)');
assertEqual(getElementModifier('fire', 'fire', 4), -50, 'Fire vs Fire Lv4 = -50% (heals)');

// Earth vs Earth at higher levels
assertEqual(getElementModifier('earth', 'earth', 2), 50, 'Earth vs Earth Lv2 = 50%');
assertEqual(getElementModifier('earth', 'earth', 3), 0, 'Earth vs Earth Lv3 = 0%');
assertEqual(getElementModifier('earth', 'earth', 4), -25, 'Earth vs Earth Lv4 = -25%');

// Holy vs Holy at higher levels → strongly negative
assertEqual(getElementModifier('holy', 'holy', 2), -25, 'Holy vs Holy Lv2 = -25% (heals)');
assertEqual(getElementModifier('holy', 'holy', 3), -50, 'Holy vs Holy Lv3 = -50% (heals)');
assertEqual(getElementModifier('holy', 'holy', 4), -100, 'Holy vs Holy Lv4 = -100% (full heal)');

// Shadow vs Shadow
assertEqual(getElementModifier('shadow', 'shadow', 2), -25, 'Shadow vs Shadow Lv2 = -25%');
assertEqual(getElementModifier('shadow', 'shadow', 4), -100, 'Shadow vs Shadow Lv4 = -100%');

// Cross-element scaling
assertEqual(getElementModifier('water', 'fire', 2), 175, 'Water vs Fire Lv2 = 175%');
assertEqual(getElementModifier('water', 'fire', 3), 200, 'Water vs Fire Lv3 = 200%');
assertEqual(getElementModifier('water', 'fire', 4), 200, 'Water vs Fire Lv4 = 200%');

assertEqual(getElementModifier('fire', 'water', 2), 25, 'Fire vs Water Lv2 = 25%');
assertEqual(getElementModifier('fire', 'water', 3), 0, 'Fire vs Water Lv3 = 0%');
assertEqual(getElementModifier('fire', 'water', 4), 0, 'Fire vs Water Lv4 = 0%');

// Holy vs Undead at higher levels
assertEqual(getElementModifier('holy', 'undead', 2), 175, 'Holy vs Undead Lv2 = 175%');
assertEqual(getElementModifier('holy', 'undead', 3), 200, 'Holy vs Undead Lv3 = 200%');
assertEqual(getElementModifier('holy', 'undead', 4), 200, 'Holy vs Undead Lv4 = 200%');

// Fire vs Undead at higher levels
assertEqual(getElementModifier('fire', 'undead', 2), 150, 'Fire vs Undead Lv2 = 150%');
assertEqual(getElementModifier('fire', 'undead', 3), 175, 'Fire vs Undead Lv3 = 175%');
assertEqual(getElementModifier('fire', 'undead', 4), 200, 'Fire vs Undead Lv4 = 200%');

// Ghost vs Ghost scaling
assertEqual(getElementModifier('ghost', 'ghost', 2), 150, 'Ghost vs Ghost Lv2 = 150%');
assertEqual(getElementModifier('ghost', 'ghost', 3), 175, 'Ghost vs Ghost Lv3 = 175%');
assertEqual(getElementModifier('ghost', 'ghost', 4), 200, 'Ghost vs Ghost Lv4 = 200%');

// Ghost vs Undead scaling — was completely wrong
assertEqual(getElementModifier('ghost', 'undead', 2), 125, 'Ghost vs Undead Lv2 = 125% (was 25!)');
assertEqual(getElementModifier('ghost', 'undead', 3), 150, 'Ghost vs Undead Lv3 = 150% (was 0!)');
assertEqual(getElementModifier('ghost', 'undead', 4), 175, 'Ghost vs Undead Lv4 = 175% (was 0!)');

// Undead vs Shadow — all levels immune
assertEqual(getElementModifier('undead', 'shadow', 2), 0, 'Undead vs Shadow Lv2 = 0%');
assertEqual(getElementModifier('undead', 'shadow', 3), 0, 'Undead vs Shadow Lv3 = 0%');
assertEqual(getElementModifier('undead', 'shadow', 4), 0, 'Undead vs Shadow Lv4 = 0%');

// Holy interactions at Lv4 — all main elements at 75% (not declining to 25)
assertEqual(getElementModifier('holy', 'water', 4), 75, 'Holy vs Water Lv4 = 75% (was 25!)');
assertEqual(getElementModifier('holy', 'earth', 4), 75, 'Holy vs Earth Lv4 = 75% (was 25!)');
assertEqual(getElementModifier('holy', 'fire', 4), 75, 'Holy vs Fire Lv4 = 75% (was 25!)');
assertEqual(getElementModifier('holy', 'wind', 4), 75, 'Holy vs Wind Lv4 = 75% (was 25!)');

// Shadow stays constant against basic elements until Lv4
assertEqual(getElementModifier('shadow', 'water', 2), 100, 'Shadow vs Water Lv2 = 100% (was 75!)');
assertEqual(getElementModifier('shadow', 'water', 3), 100, 'Shadow vs Water Lv3 = 100% (was 50!)');
assertEqual(getElementModifier('shadow', 'water', 4), 75, 'Shadow vs Water Lv4 = 75%');

// Poison vs Shadow at higher levels
assertEqual(getElementModifier('poison', 'shadow', 2), 25, 'Poison vs Shadow Lv2 = 25%');
assertEqual(getElementModifier('poison', 'shadow', 3), 0, 'Poison vs Shadow Lv3 = 0%');
assertEqual(getElementModifier('poison', 'shadow', 4), -25, 'Poison vs Shadow Lv4 = -25% (heals)');

// ============================================================
// Test 4: Complete Level 1 Row Verification
// Verify EVERY cell at Level 1 against rAthena canonical
// ============================================================
console.log('\n=== Test 4: Complete Level 1 Verification ===');

// rAthena canonical Level 1 table (rows=attack, cols=defend)
// Order: neutral, water, earth, fire, wind, poison, holy, shadow, ghost, undead
const CANONICAL_LV1 = {
    neutral: [100, 100, 100, 100, 100, 100, 100, 100,  25, 100],
    water:   [100,  25, 100, 150,  50, 100,  75, 100, 100, 100],
    earth:   [100, 100, 100,  50, 150, 100,  75, 100, 100, 100],
    fire:    [100,  50, 150,  25, 100, 100,  75, 100, 100, 125],
    wind:    [100, 175,  50, 100,  25, 100,  75, 100, 100, 100],
    poison:  [100, 100, 125, 125, 125,   0,  75,  50, 100, -25],
    holy:    [100, 100, 100, 100, 100, 100,   0, 125, 100, 150],
    shadow:  [100, 100, 100, 100, 100,  50, 125,   0, 100, -25],
    ghost:   [ 25, 100, 100, 100, 100, 100,  75,  75, 125, 100],
    undead:  [100, 100, 100, 100, 100,  50, 100,   0, 100,   0]
};

for (const atkEle of ELEMENTS) {
    const row = CANONICAL_LV1[atkEle];
    for (let i = 0; i < ELEMENTS.length; i++) {
        const defEle = ELEMENTS[i];
        const expected = row[i];
        const actual = getElementModifier(atkEle, defEle, 1);
        assertEqual(actual, expected, `Lv1: ${atkEle} → ${defEle}`);
    }
}

// ============================================================
// Test 5: Complete Level 4 Row Verification
// ============================================================
console.log('\n=== Test 5: Complete Level 4 Verification ===');

const CANONICAL_LV4 = {
    neutral: [100, 100, 100, 100, 100, 100, 100, 100,   0, 100],
    water:   [100, -50, 100, 200,   0,  75,   0,  25, 100, 150],
    earth:   [100, 100, -25,   0, 200,  75,   0,  25, 100,  50],
    fire:    [100,   0, 200, -50, 100,  75,   0,  25, 100, 200],
    wind:    [100, 200,   0, 100, -50,  75,   0,  25, 100, 100],
    poison:  [100,  25,  75,  75,  75,   0,   0, -25,  25,-100],
    holy:    [100,  75,  75,  75,  75, 125,-100, 200, 100, 200],
    shadow:  [100,  75,  75,  75,  75, -25, 200,-100, 100,-100],
    ghost:   [  0,  25,  25,  25,  25,  25,   0,   0, 200, 175],
    undead:  [100,  25,  25,  25,  25, -25, 175,   0, 100,   0]
};

for (const atkEle of ELEMENTS) {
    const row = CANONICAL_LV4[atkEle];
    for (let i = 0; i < ELEMENTS.length; i++) {
        const defEle = ELEMENTS[i];
        const expected = row[i];
        const actual = getElementModifier(atkEle, defEle, 4);
        assertEqual(actual, expected, `Lv4: ${atkEle} → ${defEle}`);
    }
}

// ============================================================
// Test 6: Size Penalty Verification
// ============================================================
console.log('\n=== Test 6: Size Penalty Table ===');

assertEqual(getSizePenalty('bare_hand', 'small'), 100, 'Bare hand vs Small = 100%');
assertEqual(getSizePenalty('bare_hand', 'large'), 100, 'Bare hand vs Large = 100%');
assertEqual(getSizePenalty('dagger', 'small'), 100, 'Dagger vs Small = 100%');
assertEqual(getSizePenalty('dagger', 'medium'), 75, 'Dagger vs Medium = 75%');
assertEqual(getSizePenalty('dagger', 'large'), 50, 'Dagger vs Large = 50%');
assertEqual(getSizePenalty('one_hand_sword', 'small'), 75, '1H Sword vs Small = 75%');
assertEqual(getSizePenalty('one_hand_sword', 'medium'), 100, '1H Sword vs Medium = 100%');
assertEqual(getSizePenalty('one_hand_sword', 'large'), 75, '1H Sword vs Large = 75%');
assertEqual(getSizePenalty('two_hand_sword', 'large'), 100, '2H Sword vs Large = 100%');
assertEqual(getSizePenalty('one_hand_axe', 'small'), 50, '1H Axe vs Small = 50%');
assertEqual(getSizePenalty('one_hand_axe', 'large'), 100, '1H Axe vs Large = 100%');
assertEqual(getSizePenalty('mace', 'small'), 75, 'Mace vs Small = 75%');
assertEqual(getSizePenalty('mace', 'medium'), 100, 'Mace vs Medium = 100%');
assertEqual(getSizePenalty('mace', 'large'), 100, 'Mace vs Large = 100%');
assertEqual(getSizePenalty('bow', 'small'), 100, 'Bow vs Small = 100%');
assertEqual(getSizePenalty('bow', 'large'), 75, 'Bow vs Large = 75%');
assertEqual(getSizePenalty('katar', 'small'), 75, 'Katar vs Small = 75%');
assertEqual(getSizePenalty('katar', 'medium'), 100, 'Katar vs Medium = 100%');
assertEqual(getSizePenalty('katar', 'large'), 75, 'Katar vs Large = 75%');
assertEqual(getSizePenalty('book', 'large'), 50, 'Book vs Large = 50%');
assertEqual(getSizePenalty('knuckle', 'medium'), 75, 'Knuckle vs Medium = 75%');
assertEqual(getSizePenalty('knuckle', 'large'), 50, 'Knuckle vs Large = 50%');
assertEqual(getSizePenalty('whip', 'large'), 50, 'Whip vs Large = 50%');
assertEqual(getSizePenalty('instrument', 'small'), 75, 'Instrument vs Small = 75%');
assertEqual(getSizePenalty('rod', 'large'), 100, 'Rod vs Large = 100%');
assertEqual(getSizePenalty('staff', 'small'), 100, 'Staff vs Small = 100%');

// Fallback
assertEqual(getSizePenalty('unknown_weapon', 'small'), 100, 'Unknown weapon defaults to 100%');

// ============================================================
// Test 7: Card Modifier Stacking (Per-Category Multiplicative)
// ============================================================
console.log('\n=== Test 7: Card Modifier Stacking ===');

// With per-category multiplicative: race 20% and size 20% should give
// damage * 1.2 * 1.2 = damage * 1.44 (not 1.40)
const atkStats = { str: 50, agi: 30, vit: 20, int: 10, dex: 30, luk: 10, level: 50 };
const defStats = { str: 10, agi: 10, vit: 10, int: 10, dex: 10, luk: 10, level: 50 };

// Test with high ATK to make card bonus visible
const resultNoCards = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'neutral' },
    { stats: defStats, hardDef: 0, element: { type: 'neutral', level: 1 }, size: 'medium', race: 'demi_human' },
    { forceHit: true, forceCrit: true }
);

const resultWithCards = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'neutral',
      cardMods: { race_demi_human: 20, size_medium: 20 } },
    { stats: defStats, hardDef: 0, element: { type: 'neutral', level: 1 }, size: 'medium', race: 'demi_human' },
    { forceHit: true, forceCrit: true }
);

// Per-category multiplicative: 1.2 * 1.2 = 1.44, floor rounding shifts slightly
// With floor() on each step, ratio can be 1.43-1.47 depending on base damage
const ratio = resultWithCards.damage / resultNoCards.damage;
assert(ratio >= 1.40 && ratio <= 1.50, `Card stacking is multiplicative (ratio=${ratio.toFixed(4)}, expected ~1.44)`);
// Verify it's NOT purely additive (would give exactly 1.40)
assert(ratio > 1.40, `Card stacking is NOT purely additive (ratio=${ratio.toFixed(4)} > 1.40)`);

// ============================================================
// Test 8: Element Modifier Integration with Damage
// ============================================================
console.log('\n=== Test 8: Element Modifier in Damage Calc ===');

// Fire attack vs Water target (Lv1) = 50% damage
const fireVsWater = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'fire' },
    { stats: defStats, hardDef: 0, element: { type: 'water', level: 1 }, size: 'medium', race: 'formless' },
    { forceHit: true, forceCrit: true }
);
assertEqual(fireVsWater.elementModifier, 50, 'Fire vs Water Lv1 elementModifier = 50');
assertEqual(fireVsWater.element, 'fire', 'Attack element reported as fire');

// Water attack vs Fire target (Lv1) = 150% damage
const waterVsFire = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'water' },
    { stats: defStats, hardDef: 0, element: { type: 'fire', level: 1 }, size: 'medium', race: 'formless' },
    { forceHit: true, forceCrit: true }
);
assertEqual(waterVsFire.elementModifier, 150, 'Water vs Fire Lv1 elementModifier = 150');

// Neutral vs Ghost Lv3 = immune (0%)
const neutralVsGhost3 = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'neutral' },
    { stats: defStats, hardDef: 0, element: { type: 'ghost', level: 3 }, size: 'medium', race: 'formless' },
    { forceHit: true, forceCrit: true }
);
assertEqual(neutralVsGhost3.damage, 0, 'Neutral vs Ghost Lv3 = 0 damage (immune)');
assertEqual(neutralVsGhost3.isMiss, true, 'Neutral vs Ghost Lv3 is a miss');
assertEqual(neutralVsGhost3.hitType, 'elementImmune', 'Neutral vs Ghost Lv3 hitType = elementImmune');

// Poison vs Undead Lv1 = -25% (heals = treated as no damage)
const poisonVsUndead = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'poison' },
    { stats: defStats, hardDef: 0, element: { type: 'undead', level: 1 }, size: 'medium', race: 'undead' },
    { forceHit: true, forceCrit: true }
);
assertEqual(poisonVsUndead.damage, 0, 'Poison vs Undead Lv1 = 0 damage (would heal)');
assertEqual(poisonVsUndead.hitType, 'elementHeal', 'Poison vs Undead Lv1 hitType = elementHeal');

// ============================================================
// Test 9: Skill Element Override
// ============================================================
console.log('\n=== Test 9: Skill Element Override ===');

// Weapon is neutral but skill forces fire element
const fireBoltVsEarth = calculatePhysicalDamage(
    { stats: atkStats, weaponATK: 100, weaponType: 'bare_hand', weaponElement: 'neutral' },
    { stats: defStats, hardDef: 0, element: { type: 'earth', level: 1 }, size: 'medium', race: 'formless' },
    { forceHit: true, forceCrit: true, skillElement: 'fire', isSkill: true, skillMultiplier: 100 }
);
assertEqual(fireBoltVsEarth.element, 'fire', 'Skill element overrides weapon element');
assertEqual(fireBoltVsEarth.elementModifier, 150, 'Fire skill vs Earth Lv1 = 150%');

// ============================================================
// Test 10: Magic Damage Element Modifier
// ============================================================
console.log('\n=== Test 10: Magic Damage Element Modifier ===');

const magicAtkStats = { str: 10, agi: 10, vit: 10, int: 80, dex: 30, luk: 10, level: 50 };

// Cold Bolt (water) vs Fire Lv2 = 175%
const coldBoltVsFire = calculateMagicalDamage(
    { stats: magicAtkStats, weaponMATK: 0 },
    { stats: defStats, hardMdef: 0, element: { type: 'fire', level: 2 } },
    { skillMultiplier: 100, skillElement: 'water' }
);
assertEqual(coldBoltVsFire.elementModifier, 175, 'Cold Bolt (water) vs Fire Lv2 = 175%');

// Soul Strike (ghost) vs Ghost Lv1 = 125%
const soulStrikeVsGhost = calculateMagicalDamage(
    { stats: magicAtkStats, weaponMATK: 0 },
    { stats: defStats, hardMdef: 0, element: { type: 'ghost', level: 1 } },
    { skillMultiplier: 100, skillElement: 'ghost' }
);
assertEqual(soulStrikeVsGhost.elementModifier, 125, 'Soul Strike (ghost) vs Ghost Lv1 = 125%');

// Fire Bolt (fire) vs Water Lv4 = immune (0%)
const fireBoltVsWater4 = calculateMagicalDamage(
    { stats: magicAtkStats, weaponMATK: 0 },
    { stats: defStats, hardMdef: 0, element: { type: 'water', level: 4 } },
    { skillMultiplier: 100, skillElement: 'fire' }
);
assertEqual(fireBoltVsWater4.damage, 0, 'Fire Bolt vs Water Lv4 = immune');
assertEqual(fireBoltVsWater4.hitType, 'elementImmune', 'Fire Bolt vs Water Lv4 hitType = elementImmune');

// ============================================================
// Test 11: Edge Cases
// ============================================================
console.log('\n=== Test 11: Edge Cases ===');

// Unknown element defaults to neutral
assertEqual(getElementModifier('nonexistent', 'fire', 1), 100, 'Unknown attack element defaults to neutral row');
assertEqual(getElementModifier('fire', 'nonexistent', 1), 100, 'Unknown defend element defaults to neutral column');

// Level out of range clamped
assertEqual(getElementModifier('fire', 'water', 0), 50, 'Level 0 clamped to Level 1 (50%)');
assertEqual(getElementModifier('fire', 'water', 5), 0, 'Level 5 clamped to Level 4 (0%)');
assertEqual(getElementModifier('fire', 'water', -1), 50, 'Level -1 clamped to Level 1 (50%)');

// Unknown weapon type defaults to 100%
assertEqual(getSizePenalty('laser_cannon', 'small'), 100, 'Unknown weapon type defaults to 100%');

// ============================================================
// Results Summary
// ============================================================
console.log('\n========================================');
console.log(`RESULTS: ${passed} passed, ${failed} failed`);
console.log('========================================');

if (failures.length > 0) {
    console.log('\nFailed tests:');
    failures.forEach((f, i) => console.log(`  ${i + 1}. ${f}`));
}

process.exit(failed > 0 ? 1 : 0);
