/**
 * Ragnarok Online Homunculus Data — Pre-Renewal Classic
 * 4 types: Lif, Amistr, Filir, Vanilmirth
 * Sources: rAthena pre-re DB, iRO Wiki Classic, Alchemist_Class_Research.md
 */
'use strict';

// Base stats at Level 1
const HOMUNCULUS_TYPES = {
    lif: {
        name: 'Lif', race: 'demihuman', element: 'neutral',
        foodItemId: 537, // Pet Food
        baseHP: 150, baseSP: 40,
        baseSTR: 17, baseAGI: 20, baseVIT: 15, baseINT: 35, baseDEX: 24, baseLUK: 12,
        // Growth probability tables: [chance_of_+0, chance_of_+1, chance_of_+2]
        growthHP: { min: 60, max: 100 },  // avg ~80
        growthSP: { min: 4, max: 9 },     // avg ~6.5
        growthSTR: [0.3333, 0.6000, 0.0667],
        growthAGI: [0.3333, 0.6000, 0.0667],
        growthVIT: [0.3333, 0.6000, 0.0667],
        growthINT: [0.3529, 0.5882, 0.0588],
        growthDEX: [0.2667, 0.6667, 0.0667],
        growthLUK: [0.2667, 0.6667, 0.0667],
        skills: [
            { name: 'Healing Hands', maxLevel: 5, type: 'active', spCost: [13, 16, 19, 22, 25], cooldown: 2000, description: 'Heals owner HP' },
            { name: 'Urgent Escape', maxLevel: 5, type: 'active', spCost: [20, 25, 30, 35, 40], cooldown: [60000, 70000, 80000, 90000, 120000], description: '+10-50% move speed' },
            { name: 'Brain Surgery', maxLevel: 5, type: 'passive', description: '+1-5% MaxSP, +2-10% heal, +3-15% SP regen' }
        ]
    },
    amistr: {
        name: 'Amistr', race: 'brute', element: 'neutral',
        foodItemId: 912, // Zargon
        baseHP: 320, baseSP: 10,
        baseSTR: 20, baseAGI: 17, baseVIT: 35, baseINT: 11, baseDEX: 24, baseLUK: 12,
        growthHP: { min: 80, max: 130 },  // avg ~105
        growthSP: { min: 1, max: 4 },     // avg ~2.5
        growthSTR: [0.1538, 0.7692, 0.0769],
        growthAGI: [0.3529, 0.5882, 0.0588],
        growthVIT: [0.3529, 0.5882, 0.0588],
        growthINT: [0.9000, 0.1000, 0.0000],
        growthDEX: [0.4117, 0.5882, 0.0000],
        growthLUK: [0.4117, 0.5882, 0.0000],
        skills: [
            { name: 'Castling', maxLevel: 5, type: 'active', spCost: [10, 10, 10, 10, 10], cooldown: 5000, description: 'Swap position with owner' },
            { name: 'Amistr Bulwark', maxLevel: 5, type: 'active', spCost: [20, 20, 20, 20, 20], cooldown: [60000, 70000, 80000, 90000, 120000], description: '+2 to +10 VIT/DEF per level (pre-renewal)' },
            { name: 'Adamantium Skin', maxLevel: 5, type: 'passive', description: '+2-10% MaxHP, +5-25% HP regen, +4-20 DEF' }
        ]
    },
    filir: {
        name: 'Filir', race: 'brute', element: 'neutral',
        foodItemId: 910, // Garlet
        baseHP: 90, baseSP: 25,
        baseSTR: 29, baseAGI: 35, baseVIT: 9, baseINT: 8, baseDEX: 30, baseLUK: 9,
        growthHP: { min: 45, max: 75 },   // avg ~60
        growthSP: { min: 3, max: 6 },     // avg ~4.5
        growthSTR: [0.3529, 0.5882, 0.0588],
        growthAGI: [0.1538, 0.7692, 0.0769],
        growthVIT: [0.9000, 0.1000, 0.0000],
        growthINT: [0.4117, 0.5882, 0.0000],
        growthDEX: [0.3529, 0.5882, 0.0588],
        growthLUK: [0.4117, 0.5882, 0.0000],
        skills: [
            { name: 'Moonlight', maxLevel: 5, type: 'offensive', hits: [1, 2, 2, 2, 3], atkPct: [220, 330, 440, 550, 660], spCost: [4, 8, 12, 16, 20], cooldown: 3000, description: 'Multi-hit attack' },
            { name: 'Flitting', maxLevel: 5, type: 'active', spCost: [20, 20, 20, 20, 20], cooldown: [60000, 70000, 80000, 90000, 120000], description: '+3-15% ASPD, +10-30% ATK' },
            { name: 'Accelerated Flight', maxLevel: 5, type: 'passive', description: '+20-60 FLEE' }
        ]
    },
    vanilmirth: {
        name: 'Vanilmirth', race: 'formless', element: 'neutral',
        foodItemId: 911, // Scell
        baseHP: 80, baseSP: 11,
        baseSTR: 11, baseAGI: 11, baseVIT: 11, baseINT: 11, baseDEX: 11, baseLUK: 11,
        growthHP: { min: 60, max: 120 },   // avg ~90
        growthSP: { min: 1, max: 6 },      // avg ~3.5
        // Vanilmirth has uniform growth with +3 possibility
        growthSTR: [0.3000, 0.3333, 0.3333, 0.0333],
        growthAGI: [0.3000, 0.3333, 0.3333, 0.0333],
        growthVIT: [0.3000, 0.3333, 0.3333, 0.0333],
        growthINT: [0.3000, 0.3333, 0.3333, 0.0333],
        growthDEX: [0.3000, 0.3333, 0.3333, 0.0333],
        growthLUK: [0.3000, 0.3333, 0.3333, 0.0333],
        skills: [
            { name: 'Caprice', maxLevel: 5, type: 'offensive', hits: [1, 2, 3, 4, 5], spCost: [22, 24, 26, 28, 30], cooldown: 2000, description: 'Random element bolt' },
            { name: 'Chaotic Blessings', maxLevel: 5, type: 'supportive', spCost: [40, 40, 40, 40, 40], cooldown: 5000, description: 'Random heal' },
            { name: 'Instruction Change', maxLevel: 5, type: 'passive', description: '+1-5 INT, +1-4 STR' }
        ]
    }
};

// Roll stat growth for a single stat on level-up
function rollStatGrowth(probArray) {
    const roll = Math.random();
    let cumulative = 0;
    for (let i = 0; i < probArray.length; i++) {
        cumulative += probArray[i];
        if (roll < cumulative) return i; // +0, +1, +2, or +3
    }
    return 0;
}

// Roll HP/SP growth (uniform between min and max)
function rollHPSPGrowth(range) {
    return Math.floor(Math.random() * (range.max - range.min + 1)) + range.min;
}

// Apply level-up stat growth
function applyLevelUpGrowth(hState) {
    const typeDef = HOMUNCULUS_TYPES[hState.type];
    if (!typeDef) return {};

    const gains = {
        hp: rollHPSPGrowth(typeDef.growthHP),
        sp: rollHPSPGrowth(typeDef.growthSP),
        str: rollStatGrowth(typeDef.growthSTR),
        agi: rollStatGrowth(typeDef.growthAGI),
        vit: rollStatGrowth(typeDef.growthVIT),
        int: rollStatGrowth(typeDef.growthINT),
        dex: rollStatGrowth(typeDef.growthDEX),
        luk: rollStatGrowth(typeDef.growthLUK)
    };

    hState.hpMax += gains.hp;
    hState.spMax += gains.sp;
    hState.str += gains.str;
    hState.agi += gains.agi;
    hState.vit += gains.vit;
    hState.int_stat += gains.int;
    hState.dex += gains.dex;
    hState.luk += gains.luk;

    // Heal to full on level-up
    hState.hpCurrent = hState.hpMax;
    hState.spCurrent = hState.spMax;

    return gains;
}

// Calculate derived combat stats from base stats
function calculateHomunculusStats(hState) {
    const lv = hState.level;
    const str = hState.str, agi = hState.agi, vit = hState.vit;
    const int = hState.int_stat, dex = hState.dex, luk = hState.luk;

    return {
        atk: 2 * lv + str,
        atkMin: Math.floor((str + dex) / 5),
        atkMax: Math.floor((luk + str + dex) / 3),
        matk: int + lv + Math.floor((luk + int + dex) / 3),
        matkMin: int + lv + Math.floor((int + dex) / 5),
        matkMax: int + lv + Math.floor((luk + int + dex) / 3),
        hit: lv + dex + 150,
        crit: Math.floor(luk / 3) + 1,
        def: vit + Math.floor(lv / 2),
        softDef: vit + Math.floor(agi / 2),
        mdef: Math.floor((vit + lv) / 4) + Math.floor(int / 2),
        softMdef: Math.floor((vit + int) / 2),
        flee: lv + agi,
        aspd: Math.max(100, 200 - Math.floor((1000 - 4 * agi - dex) * 700 / 1000 / 10)),
        int: int
    };
}

// EXP needed for next level (approximate rAthena pre-renewal curve, ~203M total to 99)
function getExpToNextLevel(level) {
    if (level >= 99) return 0;
    return Math.floor(21 * Math.pow(level, 2.8) + 100);
}

// Intimacy brackets
function getIntimacyBracket(intimacy) {
    if (intimacy >= 910) return 'loyal';
    if (intimacy >= 750) return 'cordial';
    if (intimacy >= 250) return 'neutral';
    if (intimacy >= 100) return 'shy';
    return 'awkward';
}

// Hunger feeding effects on intimacy
function getFeedIntimacyChange(hunger) {
    if (hunger <= 10) return 5;    // Very Hungry: small gain
    if (hunger <= 25) return 10;   // Hungry: best gain (optimal timing)
    if (hunger <= 75) return 3;    // Neutral: moderate gain
    if (hunger <= 90) return 0;    // Satisfied: no gain
    return -10;                     // Stuffed: intimacy LOSS
}

// Create initial homunculus state from type
function createHomunculus(type, characterId, name) {
    const typeDef = HOMUNCULUS_TYPES[type];
    if (!typeDef) return null;

    const spriteVariant = Math.random() < 0.5 ? 1 : 2;

    return {
        characterId,
        type,
        spriteVariant,
        name: name || typeDef.name,
        level: 1,
        experience: 0,
        intimacy: 250,  // Start at Neutral
        hunger: 50,     // Start half-fed
        hpCurrent: typeDef.baseHP,
        hpMax: typeDef.baseHP,
        spCurrent: typeDef.baseSP,
        spMax: typeDef.baseSP,
        str: typeDef.baseSTR,
        agi: typeDef.baseAGI,
        vit: typeDef.baseVIT,
        int_stat: typeDef.baseINT,
        dex: typeDef.baseDEX,
        luk: typeDef.baseLUK,
        skill1Level: 1,  // Start with 1 point in first skill
        skill2Level: 0,
        skill3Level: 0,
        skillPoints: 0,
        isEvolved: false,
        isAlive: true,
        isSummoned: false,
        // Runtime state (not persisted)
        targetId: null,
        command: 'follow',
        lastAttackTime: 0,
        lastHungerTick: Date.now(),
        x: 0, y: 0, z: 0
    };
}

// Allowed Alchemist-tree classes
const HOMUNCULUS_CLASSES = new Set(['alchemist', 'biochemist', 'creator', 'geneticist']);

module.exports = {
    HOMUNCULUS_TYPES,
    HOMUNCULUS_CLASSES,
    rollStatGrowth,
    rollHPSPGrowth,
    applyLevelUpGrowth,
    calculateHomunculusStats,
    getExpToNextLevel,
    getIntimacyBracket,
    getFeedIntimacyChange,
    createHomunculus
};
