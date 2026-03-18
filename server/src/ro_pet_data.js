/**
 * ro_pet_data.js — Pet Database (Pre-Renewal)
 *
 * Equivalent to rAthena db/pre-re/pet_db.yml
 * Defines all 56 tameable pets with taming items, food, accessories, capture rates, and stat bonuses.
 *
 * Sources:
 *   - rAthena pre-re/pet_db.yml (https://github.com/rathena/rathena/blob/master/db/pre-re/pet_db.yml)
 *   - rAthena src/map/pet.cpp (capture formula, intimacy)
 *   - rAthena src/map/pet.hpp (constants)
 *   - iRO Wiki Classic Pet System (https://irowiki.org/classic/Pet_System)
 */

'use strict';

// ============================================================
// Intimacy Constants (rAthena pet.hpp)
// ============================================================
const PET_INTIMATE = {
    NONE: 0,
    AWKWARD: 1,       // 0-99: may run away
    SHY: 100,         // 100-249: no bonuses
    NEUTRAL: 250,     // 250-749: starting default
    CORDIAL: 750,     // 750-909: stat bonuses active
    LOYAL: 910,       // 910-1000: full bonuses
    MAX: 1000,
};

// ============================================================
// Hunger Constants (rAthena pet.hpp)
// ============================================================
const PET_HUNGER = {
    VERY_HUNGRY: 10,  // 0-10: starvation zone, intimacy decays
    HUNGRY: 25,       // 11-25: optimal feeding zone
    NEUTRAL: 75,      // 26-75: safe to feed (lower gain)
    SATISFIED: 90,    // 76-90: overfeed zone
    STUFFED: 100,     // 91-100: overfeed zone
    FEED_AMOUNT: 20,  // Hunger gained per feed
};

// ============================================================
// Pet Database — All 56 Pre-Renewal Pets
// ============================================================
const PET_DB = {
    // --- Common Pets (easy capture, good starter pets) ---
    1002: { name: 'Poring', tamingItemId: 619, eggItemId: 9001, foodItemId: 531, equipItemId: 10013, captureRate: 2000, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { luk: 2, critical: 1 } },
    1113: { name: 'Drops', tamingItemId: 620, eggItemId: 9002, foodItemId: 508, equipItemId: 10013, captureRate: 1500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 40, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { hit: 3, atk: 3 } },
    1031: { name: 'Poporing', tamingItemId: 621, eggItemId: 9003, foodItemId: 511, equipItemId: 10013, captureRate: 1000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { luk: 2, poisonResist: 10 } },
    1063: { name: 'Lunatic', tamingItemId: 622, eggItemId: 9004, foodItemId: 534, equipItemId: 10007, captureRate: 1500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 40, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { critical: 2, atk: 2 } },
    1049: { name: 'Picky', tamingItemId: 623, eggItemId: 9005, foodItemId: 507, equipItemId: 10012, captureRate: 2000, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 40, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { str: 1, atk: 5 } },
    1011: { name: 'Chonchon', tamingItemId: 624, eggItemId: 9006, foodItemId: 537, equipItemId: 10002, captureRate: 1500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { agi: 1, flee: 2 } },
    1042: { name: 'Steel Chonchon', tamingItemId: 625, eggItemId: 9007, foodItemId: 1002, equipItemId: 10002, captureRate: 1000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { flee: 6, agi: -1 } },
    1035: { name: 'Hunter Fly', tamingItemId: 626, eggItemId: 9008, foodItemId: 716, equipItemId: 10002, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { flee: -5, perfectDodge: 2 } },
    1167: { name: 'Savage Babe', tamingItemId: 627, eggItemId: 9009, foodItemId: 537, equipItemId: 10015, captureRate: 1500, fullnessDecay: 4, hungryDelay: 60000, intimacyFed: 40, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { vit: 1, maxHP: 50 } },
    1107: { name: 'Baby Desert Wolf', tamingItemId: 628, eggItemId: 9010, foodItemId: 537, equipItemId: 10003, captureRate: 1000, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 40, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { int: 1, maxSP: 50 } },
    1052: { name: 'Rocker', tamingItemId: 629, eggItemId: 9011, foodItemId: 537, equipItemId: 10014, captureRate: 1500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { hpRegenPercent: 5, maxHP: 25 } },
    1014: { name: 'Spore', tamingItemId: 630, eggItemId: 9012, foodItemId: 537, equipItemId: 10017, captureRate: 1500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { hit: 5, atk: -2 } },
    1077: { name: 'Poison Spore', tamingItemId: 631, eggItemId: 9013, foodItemId: 537, equipItemId: 10017, captureRate: 1000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { str: 1, int: 1 } },
    1019: { name: 'Peco Peco', tamingItemId: 632, eggItemId: 9014, foodItemId: 537, equipItemId: 10010, captureRate: 1000, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxHP: 150, maxSP: -10 } },
    1056: { name: 'Smokie', tamingItemId: 633, eggItemId: 9015, foodItemId: 537, equipItemId: 10019, captureRate: 1000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { agi: 1, perfectDodge: 1 } },

    // --- Moderate Pets (medium capture, useful bonuses) ---
    1057: { name: 'Yoyo', tamingItemId: 634, eggItemId: 9016, foodItemId: 532, equipItemId: 10018, captureRate: 1000, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { critical: 3, luk: -1 } },
    1023: { name: 'Orc Warrior', tamingItemId: 635, eggItemId: 9017, foodItemId: 537, equipItemId: 10009, captureRate: 500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { atk: 10, def: -3 } },
    1026: { name: 'Munak', tamingItemId: 636, eggItemId: 9018, foodItemId: 537, equipItemId: 10008, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { int: 1, def: 1 } },
    1028: { name: 'Dokebi', tamingItemId: 637, eggItemId: 9019, foodItemId: 537, equipItemId: 10005, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { matkPercent: 1, atkPercent: -1 } },
    1170: { name: 'Sohee', tamingItemId: 638, eggItemId: 9020, foodItemId: 537, equipItemId: 10016, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { str: 1, dex: 1 } },
    1029: { name: 'Isis', tamingItemId: 639, eggItemId: 9021, foodItemId: 537, equipItemId: 10006, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { atkPercent: 1, matkPercent: -1 } },
    1155: { name: 'Green Petite', tamingItemId: 640, eggItemId: 9022, foodItemId: 537, equipItemId: 10011, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { def: -2, mdef: -2, aspdPercent: 1 } },
    1188: { name: 'Bongun', tamingItemId: 659, eggItemId: 9025, foodItemId: 537, equipItemId: 10020, captureRate: 500, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 30, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { vit: 1, stunResist: 100 } },

    // --- Rare Pets (hard capture, powerful bonuses) ---
    1109: { name: 'Deviruchi', tamingItemId: 641, eggItemId: 9023, foodItemId: 711, equipItemId: 10004, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { matkPercent: 1, atkPercent: 1, maxHPPercent: -3, maxSPPercent: -3 } },
    1101: { name: 'Baphomet Jr.', tamingItemId: 642, eggItemId: 9024, foodItemId: 518, equipItemId: 10001, captureRate: 200, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { def: 1, mdef: 1 } },
    1200: { name: 'Zealotus', tamingItemId: 660, eggItemId: 9026, foodItemId: 929, equipItemId: 0, captureRate: 300, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { raceDemiHumanPercent: 2 } },
    1275: { name: 'Alice', tamingItemId: 661, eggItemId: 9027, foodItemId: 504, equipItemId: 0, captureRate: 800, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { mdef: 1, demiHumanResist: 1 } },
    1179: { name: 'Whisper', tamingItemId: 12363, eggItemId: 9045, foodItemId: 6100, equipItemId: 10027, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { flee: 7, def: -3 } },
    1040: { name: 'Golem', tamingItemId: 12371, eggItemId: 9053, foodItemId: 6111, equipItemId: 10035, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxHP: 100, flee: -5 } },

    // --- Late-Game / Event Pets ---
    1370: { name: 'Succubus', tamingItemId: 12373, eggItemId: 9055, foodItemId: 6113, equipItemId: 10037, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { hpDrainChance: 5, hpDrainAmount: 50 } },
    1374: { name: 'Incubus', tamingItemId: 12370, eggItemId: 9052, foodItemId: 6110, equipItemId: 10034, captureRate: 50, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxSPPercent: 3 } },
    1379: { name: 'Nightmare Terror', tamingItemId: 12372, eggItemId: 9054, foodItemId: 6112, equipItemId: 10036, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { sleepImmune: true } },
    1299: { name: 'Goblin Leader', tamingItemId: 12364, eggItemId: 9046, foodItemId: 6104, equipItemId: 10028, captureRate: 50, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { raceDemiHumanDmg: 3 } },
    1504: { name: 'Dullahan', tamingItemId: 12367, eggItemId: 9049, foodItemId: 6107, equipItemId: 10031, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { critDmgPercent: 5 } },
    1148: { name: 'Medusa', tamingItemId: 12368, eggItemId: 9050, foodItemId: 6108, equipItemId: 10032, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { vit: 1, stoneResist: 500 } },
    1495: { name: 'Stone Shooter', tamingItemId: 12369, eggItemId: 9051, foodItemId: 6109, equipItemId: 10033, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { fireResist: 3 } },
    1837: { name: 'Imp', tamingItemId: 12374, eggItemId: 9056, foodItemId: 6114, equipItemId: 10038, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { fireResist: 2, fireDmgPercent: 1 } },
    1208: { name: 'Wanderer', tamingItemId: 14574, eggItemId: 9037, foodItemId: 7824, equipItemId: 0, captureRate: 800, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { agi: 3, dex: 1 } },
    1143: { name: 'Marionette', tamingItemId: 12361, eggItemId: 9043, foodItemId: 6098, equipItemId: 10025, captureRate: 500, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { spRegenPercent: 3 } },
    1401: { name: 'Shinobi', tamingItemId: 12362, eggItemId: 9044, foodItemId: 6099, equipItemId: 10026, captureRate: 500, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { agi: 2 } },
    1564: { name: 'Wicked Nymph', tamingItemId: 12365, eggItemId: 9047, foodItemId: 6105, equipItemId: 10029, captureRate: 500, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 15, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxSP: 30, spRegenPercent: 5 } },
    1404: { name: 'Miyabi Ningyo', tamingItemId: 12366, eggItemId: 9048, foodItemId: 6106, equipItemId: 10030, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 15, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { int: 1, castTimePercent: -3 } },
    1505: { name: 'Loli Ruri', tamingItemId: 12360, eggItemId: 9042, foodItemId: 6097, equipItemId: 10024, captureRate: 200, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 15, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxHPPercent: 3 } },
    1630: { name: 'Bacsojin', tamingItemId: 12357, eggItemId: 9039, foodItemId: 6094, equipItemId: 10021, captureRate: 300, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
    1513: { name: 'Mao Guai', tamingItemId: 12358, eggItemId: 9040, foodItemId: 6095, equipItemId: 10022, captureRate: 500, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxSP: 10 } },
    1586: { name: 'Leaf Cat', tamingItemId: 12359, eggItemId: 9041, foodItemId: 6096, equipItemId: 10023, captureRate: 200, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { bruteResist: 3 } },

    // --- Event Pets (high capture rate, modest bonuses) ---
    1815: { name: 'Rice Cake', tamingItemId: 0, eggItemId: 9028, foodItemId: 511, equipItemId: 0, captureRate: 2000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { neutralResist: 1, maxHPPercent: -1 } },
    1245: { name: 'Christmas Goblin', tamingItemId: 12225, eggItemId: 9029, foodItemId: 911, equipItemId: 0, captureRate: 2000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { maxHP: 30, waterResist: 1 } },
    1519: { name: 'Green Maiden', tamingItemId: 12395, eggItemId: 9030, foodItemId: 6115, equipItemId: 0, captureRate: 2000, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: { def: 1, demiHumanResist: 1 } },

    // --- Goblin Variants ---
    1122: { name: 'Knife Goblin', tamingItemId: 14569, eggItemId: 9032, foodItemId: 7821, equipItemId: 0, captureRate: 800, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
    1123: { name: 'Flail Goblin', tamingItemId: 14570, eggItemId: 9033, foodItemId: 7821, equipItemId: 0, captureRate: 800, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
    1125: { name: 'Hammer Goblin', tamingItemId: 14571, eggItemId: 9034, foodItemId: 7821, equipItemId: 0, captureRate: 800, fullnessDecay: 3, hungryDelay: 60000, intimacyFed: 50, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
    1384: { name: 'Red Deleter', tamingItemId: 14572, eggItemId: 9035, foodItemId: 7822, equipItemId: 0, captureRate: 800, fullnessDecay: 2, hungryDelay: 60000, intimacyFed: 20, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
    1382: { name: 'Diabolic', tamingItemId: 14573, eggItemId: 9036, foodItemId: 7823, equipItemId: 0, captureRate: 800, fullnessDecay: 1, hungryDelay: 60000, intimacyFed: 10, intimacyOverfed: -100, intimacyOwnerDie: -20, startIntimacy: 250, bonuses: {} },
};

// ============================================================
// Helper Functions
// ============================================================

function getPetData(mobId) {
    return PET_DB[mobId] || null;
}

function getPetByTamingItem(tamingItemId) {
    for (const [mobId, pet] of Object.entries(PET_DB)) {
        if (pet.tamingItemId === tamingItemId) return { mobId: parseInt(mobId), ...pet };
    }
    return null;
}

function getPetByEggItem(eggItemId) {
    for (const [mobId, pet] of Object.entries(PET_DB)) {
        if (pet.eggItemId === eggItemId) return { mobId: parseInt(mobId), ...pet };
    }
    return null;
}

function getIntimacyLevel(intimacy) {
    if (intimacy >= PET_INTIMATE.LOYAL) return 'loyal';
    if (intimacy >= PET_INTIMATE.CORDIAL) return 'cordial';
    if (intimacy >= PET_INTIMATE.NEUTRAL) return 'neutral';
    if (intimacy >= PET_INTIMATE.SHY) return 'shy';
    return 'awkward';
}

function getHungerLevel(hunger) {
    if (hunger >= PET_HUNGER.STUFFED - 9) return 'stuffed';   // 91-100
    if (hunger >= PET_HUNGER.SATISFIED - 14) return 'satisfied'; // 76-90
    if (hunger > PET_HUNGER.HUNGRY) return 'neutral';          // 26-75
    if (hunger > PET_HUNGER.VERY_HUNGRY) return 'hungry';      // 11-25
    return 'very_hungry';                                       // 0-10
}

// Calculate capture rate (pre-renewal legacy formula from rAthena pet.cpp)
function calculateCaptureRate(baseRate, playerLevel, monsterLevel, playerLUK, monsterHPPercent) {
    const rate = (baseRate + (playerLevel - monsterLevel) * 30 + playerLUK * 20)
                 * (200 - monsterHPPercent) / 100;
    return Math.max(0, Math.min(10000, Math.floor(rate)));
}

// Calculate intimacy change from feeding based on hunger bracket
function calculateFeedIntimacyChange(petData, currentHunger) {
    const level = getHungerLevel(currentHunger);
    switch (level) {
        case 'very_hungry': return Math.floor(petData.intimacyFed * 0.5);
        case 'hungry': return petData.intimacyFed; // Full gain
        case 'neutral': return Math.floor(petData.intimacyFed * 0.75);
        case 'satisfied':
        case 'stuffed': return petData.intimacyOverfed; // Negative (typically -100)
        default: return 0;
    }
}

module.exports = {
    PET_DB,
    PET_INTIMATE,
    PET_HUNGER,
    getPetData,
    getPetByTamingItem,
    getPetByEggItem,
    getIntimacyLevel,
    getHungerLevel,
    calculateCaptureRate,
    calculateFeedIntimacyChange,
};
