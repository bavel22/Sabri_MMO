/**
 * Ragnarok Online EXP Tables — Pre-Renewal Classic
 * 
 * Complete Base EXP (levels 1-99) and Job EXP tables for:
 * - Novice (Job 1-10)
 * - First Class (Job 1-50)
 * - Second Class (Job 1-50)
 * 
 * Sources: Verified from official RO data, rAthena pre-renewal database,
 *          WarpPortal Forum testing, Gaming Fanon Wiki
 * 
 * Usage:
 *   BASE_EXP_TABLE[level] = EXP needed to reach (level+1)
 *   NOVICE_JOB_EXP_TABLE[jobLevel] = EXP needed to reach (jobLevel+1)
 *   FIRST_CLASS_JOB_EXP_TABLE[jobLevel] = EXP needed to reach (jobLevel+1)
 *   SECOND_CLASS_JOB_EXP_TABLE[jobLevel] = EXP needed to reach (jobLevel+1)
 */

'use strict';

// ============================================================
// Base EXP Table (Levels 1-99)
// Index = current level, Value = EXP needed to reach next level
// Level 99 is max — no entry needed for 99→100
// ============================================================
const BASE_EXP_TABLE = {
    1: 9,
    2: 16,
    3: 25,
    4: 36,
    5: 77,
    6: 112,
    7: 153,
    8: 200,
    9: 253,
    10: 320,
    11: 385,
    12: 490,
    13: 585,
    14: 700,
    15: 830,
    16: 970,
    17: 1120,
    18: 1260,
    19: 1420,
    20: 1620,
    21: 1860,
    22: 1990,
    23: 2240,
    24: 2504,
    25: 2950,
    26: 3426,
    27: 3934,
    28: 4474,
    29: 6889,
    30: 7995,
    31: 9174,
    32: 10425,
    33: 11748,
    34: 13967,
    35: 15775,
    36: 17678,
    37: 19677,
    38: 21773,
    39: 30543,
    40: 34212,
    41: 38065,
    42: 42102,
    43: 46323,
    44: 53026,
    45: 58419,
    46: 64041,
    47: 69892,
    48: 75973,
    49: 102468,
    50: 115254,
    51: 128692,
    52: 142784,
    53: 157528,
    54: 178184,
    55: 196300,
    56: 215198,
    57: 234879,
    58: 255341,
    59: 330188,
    60: 365914,
    61: 403224,
    62: 442116,
    63: 482590,
    64: 536948,
    65: 585191,
    66: 635278,
    67: 687211,
    68: 740988,
    69: 925400,
    70: 1473746,
    71: 1594058,
    72: 1718928,
    73: 1848355,
    74: 1982340,
    75: 2230113,
    76: 2386162,
    77: 2547417,
    78: 2713878,
    79: 3206160,
    80: 3681024,
    81: 4022472,
    82: 4377024,
    83: 4744680,
    84: 5125440,
    85: 5767272,
    86: 6204000,
    87: 6655464,
    88: 7121664,
    89: 7602600,
    90: 9738720,
    91: 11649960,
    92: 13643520,
    93: 18339300,
    94: 23836800,
    95: 35658000,
    96: 48687000,
    97: 58135000,
    98: 99999998
};

// ============================================================
// Novice Job EXP Table (Job Levels 1-10)
// Index = current job level, Value = EXP needed to reach next job level
// Job level 10 is max for Novice
// ============================================================
const NOVICE_JOB_EXP_TABLE = {
    1: 10,
    2: 18,
    3: 28,
    4: 40,
    5: 91,
    6: 151,
    7: 205,
    8: 268,
    9: 340
};

// ============================================================
// First Class Job EXP Table (Job Levels 1-50)
// Swordsman, Mage, Archer, Acolyte, Thief, Merchant
// Index = current job level, Value = EXP needed to reach next job level
// Job level 50 is max for First Class
// ============================================================
const FIRST_CLASS_JOB_EXP_TABLE = {
    1: 30,
    2: 43,
    3: 58,
    4: 76,
    5: 116,
    6: 180,
    7: 220,
    8: 272,
    9: 336,
    10: 520,
    11: 604,
    12: 699,
    13: 802,
    14: 948,
    15: 1125,
    16: 1668,
    17: 1937,
    18: 2226,
    19: 3040,
    20: 3988,
    21: 5564,
    22: 6272,
    23: 7021,
    24: 9114,
    25: 11473,
    26: 15290,
    27: 16891,
    28: 18570,
    29: 23229,
    30: 28359,
    31: 36478,
    32: 39716,
    33: 43088,
    34: 52417,
    35: 62495,
    36: 78160,
    37: 84175,
    38: 90404,
    39: 107611,
    40: 125915,
    41: 153941,
    42: 191781,
    43: 204351,
    44: 248352,
    45: 286212,
    46: 386371,
    47: 409795,
    48: 482092,
    49: 509596
};

// ============================================================
// Second Class Job EXP Table (Job Levels 1-50)
// Knight, Wizard, Hunter, Priest, Assassin, Blacksmith, etc.
// Index = current job level, Value = EXP needed to reach next job level
// Job level 50 is max for Second Class
// ============================================================
const SECOND_CLASS_JOB_EXP_TABLE = {
    1: 144,
    2: 184,
    3: 284,
    4: 348,
    5: 603,
    6: 887,
    7: 1096,
    8: 1598,
    9: 2540,
    10: 3676,
    11: 4290,
    12: 4946,
    13: 6679,
    14: 9492,
    15: 12770,
    16: 14344,
    17: 16005,
    18: 20642,
    19: 27434,
    20: 35108,
    21: 38577,
    22: 42206,
    23: 52708,
    24: 66971,
    25: 82688,
    26: 89544,
    27: 96669,
    28: 117821,
    29: 144921,
    30: 174201,
    31: 186677,
    32: 199584,
    33: 238617,
    34: 286366,
    35: 337147,
    36: 358435,
    37: 380376,
    38: 447685,
    39: 526989,
    40: 610246,
    41: 644736,
    42: 793535,
    43: 921810,
    44: 1106758,
    45: 1260955,
    46: 1487304,
    47: 1557657,
    48: 1990632,
    49: 2083386
};

// ============================================================
// Job Class Configuration
// ============================================================
const JOB_CLASS_CONFIG = {
    // Novice: Starting class
    'novice': {
        maxJobLevel: 10,
        jobExpTable: NOVICE_JOB_EXP_TABLE,
        tier: 0,
        displayName: 'Novice'
    },
    // First Classes (all share same job EXP table in classic RO)
    'swordsman': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Swordsman'
    },
    'mage': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Mage'
    },
    'archer': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Archer'
    },
    'acolyte': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Acolyte'
    },
    'thief': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Thief'
    },
    'merchant': {
        maxJobLevel: 50,
        jobExpTable: FIRST_CLASS_JOB_EXP_TABLE,
        tier: 1,
        displayName: 'Merchant'
    },
    // Second Classes (all share same job EXP table in classic RO)
    'knight': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Knight'
    },
    'wizard': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Wizard'
    },
    'hunter': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Hunter'
    },
    'priest': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Priest'
    },
    'assassin': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Assassin'
    },
    'blacksmith': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Blacksmith'
    },
    'crusader': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Crusader'
    },
    'sage': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Sage'
    },
    'bard': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Bard'
    },
    'dancer': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Dancer'
    },
    'monk': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Monk'
    },
    'rogue': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Rogue'
    },
    'alchemist': {
        maxJobLevel: 50,
        jobExpTable: SECOND_CLASS_JOB_EXP_TABLE,
        tier: 2,
        displayName: 'Alchemist'
    }
};

// ============================================================
// Constants
// ============================================================
const MAX_BASE_LEVEL = 99;

// Stat points gained per base level up (RO classic formula)
// Formula: floor(newLevel / 5) + 3
function getStatPointsForLevelUp(newLevel) {
    return Math.floor(newLevel / 5) + 3;
}

// Skill points gained per job level up (always 1 in classic RO)
function getSkillPointsForJobLevelUp() {
    return 1;
}

// Get EXP needed for next base level (returns 0 if at max)
function getBaseExpForNextLevel(currentLevel) {
    if (currentLevel >= MAX_BASE_LEVEL) return 0;
    return BASE_EXP_TABLE[currentLevel] || 0;
}

// Get EXP needed for next job level (returns 0 if at max)
function getJobExpForNextLevel(jobClass, currentJobLevel) {
    const config = JOB_CLASS_CONFIG[jobClass];
    if (!config) return 0;
    if (currentJobLevel >= config.maxJobLevel) return 0;
    return config.jobExpTable[currentJobLevel] || 0;
}

// Get max job level for a class
function getMaxJobLevel(jobClass) {
    const config = JOB_CLASS_CONFIG[jobClass];
    return config ? config.maxJobLevel : 10;
}

// Get class tier (0=novice, 1=first, 2=second)
function getClassTier(jobClass) {
    const config = JOB_CLASS_CONFIG[jobClass];
    return config ? config.tier : 0;
}

// Valid first classes for job change from novice
const FIRST_CLASSES = ['swordsman', 'mage', 'archer', 'acolyte', 'thief', 'merchant'];

// Valid second class upgrades from first classes
const SECOND_CLASS_UPGRADES = {
    'swordsman': ['knight', 'crusader'],
    'mage': ['wizard', 'sage'],
    'archer': ['hunter', 'bard', 'dancer'],
    'acolyte': ['priest', 'monk'],
    'thief': ['assassin', 'rogue'],
    'merchant': ['blacksmith', 'alchemist']
};

module.exports = {
    BASE_EXP_TABLE,
    NOVICE_JOB_EXP_TABLE,
    FIRST_CLASS_JOB_EXP_TABLE,
    SECOND_CLASS_JOB_EXP_TABLE,
    JOB_CLASS_CONFIG,
    MAX_BASE_LEVEL,
    FIRST_CLASSES,
    SECOND_CLASS_UPGRADES,
    getStatPointsForLevelUp,
    getSkillPointsForJobLevelUp,
    getBaseExpForNextLevel,
    getJobExpForNextLevel,
    getMaxJobLevel,
    getClassTier
};
