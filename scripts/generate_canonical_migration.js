#!/usr/bin/env node
/**
 * generate_canonical_migration.js
 *
 * Parses rAthena pre-renewal YAML item databases and generates:
 * 1. scripts/output/id_mapping.json        — old custom ID → new canonical ID
 * 2. scripts/output/canonical_items.sql     — full INSERT for 6,169 items
 * 3. scripts/output/name_fixes.json        — monster template name fixes
 * 4. server/src/ro_item_effects.js          — consumable use-effect data
 *
 * Usage: node scripts/generate_canonical_migration.js
 */

const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

// ============================================================
// Configuration
// ============================================================

const ITEMS_DIR = path.join(__dirname, '..', 'docsNew', 'items');
const OUTPUT_DIR = path.join(__dirname, 'output');
const SERVER_DIR = path.join(__dirname, '..', 'server', 'src');

const YAML_FILES = [
    path.join(ITEMS_DIR, 'item_db_usable.yml'),
    path.join(ITEMS_DIR, 'item_db_equip.yml'),
    path.join(ITEMS_DIR, 'item_db_etc.yml'),
];

const DESCRIPTIONS_FILE = path.join(ITEMS_DIR, 'item_descriptions.json');

// ============================================================
// Custom → Canonical ID Mapping
// ============================================================

// Maps our current custom item IDs → rAthena canonical IDs
// Verified against rAthena pre-renewal YAML database
//
// IMPORTANT: Custom "fancy name" items (1001-1005, 2001-2008, 3001-3006, 4001-4003)
// map to the SAME canonical IDs as their RO-named counterparts (1006+, 2009+, 3007+, 4004+).
// The migration SQL handles this by remapping the first occurrence and deleting duplicates.
const CUSTOM_TO_CANONICAL = {
    // === Consumables ===
    1001: 501,   // Crimson Vial → Red Potion
    1002: 502,   // Amber Elixir → Orange Potion
    1003: 503,   // Golden Salve → Yellow Potion
    1004: 505,   // Azure Philter → Blue Potion
    1005: 517,   // Roasted Haunch → Meat
    1006: 507,   // Red Herb
    1007: 508,   // Yellow Herb
    1008: 511,   // Green Herb
    1009: 510,   // Blue Herb
    1010: 520,   // Hinalle → Hinalle Leaflet (rAthena AegisName: Leaflet_Of_Hinal)
    1011: 512,   // Apple
    1012: 513,   // Banana
    1013: 515,   // Carrot
    1014: 514,   // Grape
    1015: 517,   // Meat (duplicate of 1005 → 517, handled in migration)
    1016: 582,   // Orange (fruit, rAthena id=582)
    1017: 578,   // Strawberry (rAthena id=578)
    1018: 633,   // Sweet Potato (rAthena AegisName: Baked_Yam)
    1019: 518,   // Honey
    1020: 620,   // Orange Juice (rAthena id=620)
    1021: 627,   // Sweet Milk (rAthena id=627)
    1022: 622,   // Rainbow Carrot (rAthena id=622)
    1023: 7182,  // Cacao (rAthena id=7182, type=Etc)
    1024: 619,   // Unripe Apple (rAthena id=619, type=Usable)
    1025: 545,   // Center Potion → Condensed Red Potion (rAthena id=545)
    1026: 972,   // Karvodailnirol (rAthena id=972, type=Etc — it's a loot item, not usable)
    1027: 520,   // Leaflet Of Hinal → same as Hinalle (rAthena id=520)
    1028: 602,   // Wing Of Butterfly → Butterfly Wing
    1029: 601,   // Wing Of Fly → Fly Wing

    // === Etc / Loot — Custom "fancy name" items ===
    // These were our original custom items that duplicate the RO items below
    2001: 909,   // Gloopy Residue → Jellopy
    2002: 938,   // Viscous Slime → Sticky Mucus
    2003: 935,   // Chitin Shard → Shell
    2004: 949,   // Downy Plume → Feather (rAthena: Feather id=949, type=Etc)
    2005: 921,   // Spore Cluster → Mushroom Spore
    2006: 928,   // Barbed Limb → Grasshopper's Leg
    2007: 511,   // Verdant Leaf → Green Herb (id=511, type=Healing in rAthena)
    2008: 914,   // Silken Tuft → Fluff

    // === Etc / Loot — RO-named items ===
    2009: 909,   // Jellopy
    2010: 914,   // Fluff
    2011: 935,   // Shell
    2012: 949,   // Feather (rAthena: Feather id=949, type=Etc)
    2013: 921,   // Mushroom Spore
    2014: 917,   // Talon
    2015: 902,   // Tree Root
    2016: 913,   // Tooth Of Bat
    2017: 939,   // Bee Sting
    2018: 928,   // Grasshopper's Leg
    2019: 932,   // Skel-Bone
    2020: 930,   // Decayed Nail
    2021: 918,   // Sticky Webfoot
    2022: 908,   // Spawn
    2023: 942,   // Yoyo Tail
    2024: 945,   // Raccoon Leaf
    2025: 905,   // Stem
    2026: 924,   // Powder Of Butterfly
    2027: 7033,  // Poison Spore (etc item)
    2028: 911,   // Shoot
    2029: 907,   // Resin
    2030: 916,   // Feather Of Birds (rAthena: Feather_Of_Birds id=916)
    2031: 925,   // Bill Of Birds (rAthena: Bill_Of_Birds id=925)
    2032: 919,   // Animal Skin (rAthena: Animal's_Skin id=919)
    2033: 712,   // Flower (rAthena: Flower id=712, type=Etc)
    2034: 922,   // Clover
    2035: 706,   // Four Leaf Clover
    2036: 915,   // Chrysalis
    2037: 938,   // Sticky Mucus (duplicate of 2002)
    2038: 910,   // Garlet
    2039: 713,   // Empty Bottle
    2040: 1002,  // Iron Ore (rAthena id=1002, type=Etc)
    2041: 998,   // Iron
    2042: 1010,  // Phracon (rAthena id=1010)
    2043: 756,   // Oridecon Stone → Rough Oridecon (rAthena: Oridecon_Stone id=756)
    2044: 992,   // Wind Of Verdure (rAthena id=992)
    2045: 715,   // Yellow Gemstone
    2046: 726,   // Azure Jewel → Sapphire (rAthena: Blue_Jewel id=726)
    2047: 727,   // Bluish Green Jewel → Opal (rAthena id=727)
    2048: 728,   // Cardinal Jewel → Topaz (rAthena id=728)
    2049: 729,   // White Jewel → Zircon (rAthena id=729)
    2050: 912,   // Zargon
    2051: 993,   // Yellow Live → Green Live (rAthena AegisName=Yellow_Live, id=993)
    2052: 7850,  // Wooden Block (rAthena: Wooden_Block_ id=7850)
    2053: 1750,  // Arrow (rAthena: Arrow id=1750, type=Ammo)
    2054: 742,   // Chonchon Doll
    2055: 752,   // Grasshopper Doll → Rocker Doll (rAthena AegisName: Grasshopper_Doll, id=752)
    2056: 753,   // Monkey Doll → Yoyo Doll (rAthena AegisName: Monkey_Doll, id=753)
    2057: 754,   // Raccoondog Doll → Raccoon Doll (rAthena AegisName: Raccoondog_Doll, id=754)
    2058: 743,   // Spore Doll

    // === Weapons — Custom "fancy name" items ===
    3001: 1201,  // Rustic Shiv → Knife (3-slot)
    3002: 1202,  // Keen Edge → Cutter (3-slot)
    3003: 1204,  // Stiletto Fang → Main Gauche (3-slot)
    3004: 1101,  // Iron Cleaver → Sword (3-slot)
    3005: 1109,  // Crescent Saber → Falchion (2-slot)
    3006: 1701,  // Hunting Longbow → Bow (3-slot)

    // === Weapons — RO-named items ===
    3007: 1201,  // Knife (duplicate of 3001)
    3008: 1202,  // Cutter (duplicate of 3002)
    3009: 1204,  // Main Gauche (duplicate of 3003)
    3010: 1109,  // Falchion (duplicate of 3005)
    3011: 1504,  // Mace (rAthena: Mace id=1504, 3-slot)
    3012: 1601,  // Rod (rAthena: Rod id=1601, 3-slot)
    3013: 1701,  // Bow (duplicate of 3006)
    3014: 1401,  // Javelin (rAthena: Javelin id=1401, 3-slot)
    3015: 1404,  // Spear (rAthena: Spear id=1404, 3-slot)
    3016: 1301,  // Axe (rAthena: Axe id=1301, 3-slot)
    3017: 1501,  // Club (rAthena: Club id=1501, 3-slot)
    3018: 1604,  // Wand (rAthena: Wand id=1604, 2-slot)
    3019: 1907,  // Guitar Of Vast Land → Guitar (rAthena: Guitar id=1907)
    3020: 1950,  // Whip Of Earth → Rope (rAthena: Rope id=1950)

    // === Armor ===
    4001: 2301,  // Linen Tunic → Cotton Shirt (0-slot)
    4002: 2312,  // Quilted Vest → Padded Armor (0-slot)
    4003: 2314,  // Ringweave Hauberk → Chain Mail (0-slot)
    4004: 2101,  // Guard (0-slot)
    4005: 2220,  // Hat (rAthena: Hat id=2220, 0-slot)
    4006: 2401,  // Sandals (0-slot)
    4007: 2321,  // Silk Robe (0-slot)
    4008: 2208,  // Ribbon (rAthena: Ribbon id=2208, 0-slot)
    4009: 2213,  // Cat Hairband → Kitty Band (rAthena: Cat_Hairband id=2213)
    4010: 2609,  // Skul Ring → Skull Ring (rAthena: Skul_Ring id=2609)
    4011: 2298,  // Green Feeler (rAthena id=2298)
    4012: 2262,  // Pierrot Nose → Clown Nose (rAthena: Pierrot_Nose id=2262)
    4013: 5113,  // Horrendous Mouth → Angry Snarl (rAthena head_low equip id=5113)
    4014: 2207,  // Fancy Flower (rAthena id=2207)

    // === Cards ===
    5001: 4001,  // Poring Card
    5002: 4006,  // Lunatic Card
    5003: 4003,  // Fabre Card
    5004: 4005,  // Pupa Card
    5005: 4004,  // Drops Card
    5006: 4009,  // Chonchon Card
    5007: 4015,  // Condor Card
    5008: 4010,  // Willow Card (our DB has typo "Wilow")
    5009: 4011,  // Roda Frog Card
    5010: 4016,  // Hornet Card
    5011: 4021,  // Rocker Card
    5012: 4020,  // Familiar Card (our DB has typo "Farmiliar")
    5013: 4045,  // Savage Babe Card
    5014: 4022,  // Spore Card
    5015: 4038,  // Zombie Card
    5016: 4037,  // Skeleton Card
    5017: 4042,  // Creamy Card
    5018: 4033,  // Poporing Card
    5019: 4031,  // Pecopeco Card
    5020: 4029,  // Mandragora Card
    5021: 4048,  // Poison Spore Card
    5022: 4044,  // Smokie Card
    5023: 4051,  // Yoyo Card
};

// ============================================================
// Helpers
// ============================================================

/**
 * Normalize rAthena item type to our item_type
 */
function normalizeItemType(rathenaType) {
    switch (rathenaType) {
        case 'Healing':
        case 'Usable':
        case 'Delayconsume':
        case 'Cash':
            return 'consumable';
        case 'Weapon':
            return 'weapon';
        case 'Armor':
            return 'armor';
        case 'Card':
            return 'card';
        case 'Ammo':
            return 'ammo';
        case 'Etc':
        case 'Petegg':
        case 'Petarmor':
        default:
            return 'etc';
    }
}

/**
 * Normalize rAthena Locations to our equip_slot
 */
function normalizeEquipSlot(locations) {
    if (!locations) return null;
    if (locations.Right_Hand) return 'weapon';
    if (locations.Both_Hand) return 'weapon';
    if (locations.Left_Hand) return 'shield';
    if (locations.Armor) {
        // For cards, Armor location means "compound on armor"
        return 'armor';
    }
    if (locations.Garment) return 'garment';
    if (locations.Shoes) return 'footgear';
    if (locations.Head_Top && locations.Head_Mid) return 'head_top'; // combo
    if (locations.Head_Top) return 'head_top';
    if (locations.Head_Mid) return 'head_mid';
    if (locations.Head_Low) return 'head_low';
    if (locations.Both_Accessory) return 'accessory';
    if (locations.Right_Accessory) return 'accessory';
    if (locations.Left_Accessory) return 'accessory';
    return null;
}

/**
 * Normalize rAthena weapon SubType to our weapon_type
 */
function normalizeWeaponType(subType) {
    const map = {
        'Dagger': 'dagger',
        '1hSword': 'one_hand_sword',
        '2hSword': 'two_hand_sword',
        '1hSpear': 'spear',
        '2hSpear': 'spear',
        '1hAxe': 'axe',
        '2hAxe': 'axe',
        'Mace': 'mace',
        '2hMace': 'mace',
        'Staff': 'staff',
        '2hStaff': 'staff',
        'Bow': 'bow',
        'Knuckle': 'knuckle',
        'Musical': 'instrument',
        'Whip': 'whip',
        'Book': 'book',
        'Katar': 'katar',
        'Revolver': 'gun',
        'Rifle': 'gun',
        'Shotgun': 'gun',
        'Gatling': 'gun',
        'Grenade': 'gun',
        'Huuma': 'huuma',
    };
    return map[subType] || null;
}

/**
 * Check if a weapon subtype is two-handed
 */
function isTwoHanded(subType, locations) {
    if (locations && locations.Both_Hand) return true;
    const twoHanded = ['2hSword', '2hSpear', '2hAxe', '2hMace', '2hStaff',
                        'Bow', 'Katar', 'Musical', 'Whip',
                        'Huuma', 'Rifle', 'Shotgun', 'Gatling', 'Grenade'];
    return twoHanded.includes(subType);
}

/**
 * Parse simple bonus bXxx,N; scripts into stat columns
 */
function parseBonusScript(script) {
    if (!script) return {};
    const bonuses = {};

    // Match bonus bXxx,N; patterns
    const bonusRegex = /bonus\s+b(\w+)\s*,\s*(-?\d+)\s*;/g;
    let match;
    while ((match = bonusRegex.exec(script)) !== null) {
        const [, bonusType, value] = match;
        const n = parseInt(value);

        switch (bonusType) {
            case 'Str': bonuses.str_bonus = (bonuses.str_bonus || 0) + n; break;
            case 'Agi': bonuses.agi_bonus = (bonuses.agi_bonus || 0) + n; break;
            case 'Vit': bonuses.vit_bonus = (bonuses.vit_bonus || 0) + n; break;
            case 'Int': bonuses.int_bonus = (bonuses.int_bonus || 0) + n; break;
            case 'Dex': bonuses.dex_bonus = (bonuses.dex_bonus || 0) + n; break;
            case 'Luk': bonuses.luk_bonus = (bonuses.luk_bonus || 0) + n; break;
            case 'Def': bonuses.def_bonus = n; break;
            case 'Def2': bonuses.def_bonus = (bonuses.def_bonus || 0) + n; break;
            case 'Mdef': bonuses.mdef = (bonuses.mdef || 0) + n; break;
            case 'Mdef2': bonuses.mdef = (bonuses.mdef || 0) + n; break;
            case 'Matk':
            case 'MatkRate': /* Pre-renewal: skip storing % as flat MATK */ break;
            case 'MaxHP': bonuses.max_hp_bonus = (bonuses.max_hp_bonus || 0) + n; break;
            case 'MaxSP': bonuses.max_sp_bonus = (bonuses.max_sp_bonus || 0) + n; break;
            case 'MaxHPrate':
            case 'MaxHPRate': bonuses.max_hp_rate = (bonuses.max_hp_rate || 0) + n; break;
            case 'MaxSPrate':
            case 'MaxSPRate': bonuses.max_sp_rate = (bonuses.max_sp_rate || 0) + n; break;
            case 'Hit': bonuses.hit_bonus = (bonuses.hit_bonus || 0) + n; break;
            case 'Flee': bonuses.flee_bonus = (bonuses.flee_bonus || 0) + n; break;
            case 'Flee2': bonuses.perfect_dodge_bonus = (bonuses.perfect_dodge_bonus || 0) + n; break;
            case 'Critical': bonuses.critical_bonus = (bonuses.critical_bonus || 0) + n; break;
            case 'Atk':
            case 'Atk2':
            case 'BaseAtk': bonuses.atk_bonus = (bonuses.atk_bonus || 0) + n; break;
            case 'AspdRate': bonuses.aspd_rate = (bonuses.aspd_rate || 0) + n; break;
            case 'Aspd': bonuses.aspd_modifier = (bonuses.aspd_modifier || 0) + n; break;
            case 'HPrecovRate': bonuses.hp_regen_rate = (bonuses.hp_regen_rate || 0) + n; break;
            case 'SPrecovRate': bonuses.sp_regen_rate = (bonuses.sp_regen_rate || 0) + n; break;
            case 'CritAtkRate': bonuses.crit_atk_rate = (bonuses.crit_atk_rate || 0) + n; break;
            case 'Castrate':
            case 'CastRate': bonuses.cast_rate = (bonuses.cast_rate || 0) + n; break;
            case 'UseSPrate':
            case 'UseSPRate': bonuses.use_sp_rate = (bonuses.use_sp_rate || 0) + n; break;
            case 'HealPower': bonuses.heal_power = (bonuses.heal_power || 0) + n; break;
        }
    }

    // Parse bonus bAllStats,N;
    const allStatsMatch = script.match(/bonus\s+bAllStats\s*,\s*(-?\d+)\s*;/);
    if (allStatsMatch) {
        const n = parseInt(allStatsMatch[1]);
        bonuses.str_bonus = (bonuses.str_bonus || 0) + n;
        bonuses.agi_bonus = (bonuses.agi_bonus || 0) + n;
        bonuses.vit_bonus = (bonuses.vit_bonus || 0) + n;
        bonuses.int_bonus = (bonuses.int_bonus || 0) + n;
        bonuses.dex_bonus = (bonuses.dex_bonus || 0) + n;
        bonuses.luk_bonus = (bonuses.luk_bonus || 0) + n;
    }

    // Parse element from bAtkEle (weapon attack element) or bDefEle (armor defense element)
    const ELE_MAP = {
        Ele_Neutral: 'neutral', Ele_Water: 'water', Ele_Earth: 'earth',
        Ele_Fire: 'fire', Ele_Wind: 'wind', Ele_Poison: 'poison',
        Ele_Holy: 'holy', Ele_Dark: 'dark', Ele_Ghost: 'ghost', Ele_Undead: 'undead'
    };
    const eleMatch = script.match(/bonus\s+b(?:AtkEle|DefEle)\s*,\s*(Ele_\w+)\s*;/);
    if (eleMatch && ELE_MAP[eleMatch[1]]) {
        bonuses.element = ELE_MAP[eleMatch[1]];
    }

    return bonuses;
}

/**
 * Parse consumable script into use-effect data for ro_item_effects.js
 */
function parseConsumableScript(script) {
    if (!script) return null;
    const effects = [];

    // itemheal rand(X,Y),rand(A,B); or itemheal rand(X,Y),0; or itemheal X,Y;
    const itemhealRegex = /itemheal\s+(rand\((\d+),(\d+)\)|(\d+))\s*,\s*(rand\((\d+),(\d+)\)|(\d+))\s*;/g;
    let match;
    while ((match = itemhealRegex.exec(script)) !== null) {
        const hpMin = match[2] ? parseInt(match[2]) : parseInt(match[4]);
        const hpMax = match[3] ? parseInt(match[3]) : parseInt(match[4]);
        const spMin = match[6] ? parseInt(match[6]) : parseInt(match[8]);
        const spMax = match[7] ? parseInt(match[7]) : parseInt(match[8]);

        const effect = { type: 'heal' };
        if (hpMin > 0 || hpMax > 0) {
            effect.hpMin = hpMin;
            effect.hpMax = hpMax;
        }
        if (spMin > 0 || spMax > 0) {
            effect.spMin = spMin;
            effect.spMax = spMax;
        }
        if (effect.hpMin || effect.spMin) effects.push(effect);
    }

    // percentheal X,Y;
    const percenthealRegex = /percentheal\s+(-?\d+)\s*,\s*(-?\d+)\s*;/g;
    while ((match = percenthealRegex.exec(script)) !== null) {
        const hp = parseInt(match[1]);
        const sp = parseInt(match[2]);
        if (hp !== 0 || sp !== 0) {
            effects.push({ type: 'percentheal', hp, sp });
        }
    }

    // sc_end SC_X;
    const scEndRegex = /sc_end\s+SC_(\w+)\s*;/g;
    const cures = [];
    while ((match = scEndRegex.exec(script)) !== null) {
        cures.push('SC_' + match[1]);
    }
    if (cures.length > 0) {
        effects.push({ type: 'cure', cures });
    }

    // itemskill "SKILL_NAME",LEVEL; or itemskill SKILL_NAME,LEVEL;
    const itemskillRegex = /itemskill\s+"?([A-Z_]+)"?\s*,\s*(\d+)\s*;/g;
    while ((match = itemskillRegex.exec(script)) !== null) {
        effects.push({ type: 'itemskill', skill: match[1], level: parseInt(match[2]) });
    }

    // sc_start SC_X,duration,value;
    const scStartRegex = /sc_start\s+SC_(\w+)\s*,\s*(\d+)\s*,\s*(\d+)\s*;/g;
    while ((match = scStartRegex.exec(script)) !== null) {
        effects.push({
            type: 'sc_start',
            status: 'SC_' + match[1],
            duration: parseInt(match[2]),
            value: parseInt(match[3])
        });
    }

    if (effects.length === 0) return null;
    // Flatten single heal effects
    if (effects.length === 1) return effects[0];
    // Multiple effects = combine
    return { type: 'multi', effects };
}

/**
 * Escape SQL string values
 */
function sqlEscape(str) {
    if (str === null || str === undefined) return 'NULL';
    return "'" + String(str).replace(/'/g, "''") + "'";
}

/**
 * Get stackable + max_stack based on rAthena type and Stack info
 */
function getStackInfo(item) {
    const stackableTypes = ['Healing', 'Usable', 'Delayconsume', 'Etc', 'Ammo', 'Cash'];
    const isStackable = stackableTypes.includes(item.Type || 'Etc');

    let maxStack = isStackable ? 999 : 1;
    if (item.Stack && item.Stack.Amount) {
        maxStack = item.Stack.Amount;
    }

    return { stackable: isStackable, maxStack };
}

/**
 * Serialize equip locations to text
 */
function serializeLocations(locations) {
    if (!locations) return null;
    return Object.keys(locations).filter(k => locations[k]).join(',');
}

/**
 * Serialize jobs to text
 */
function serializeJobs(jobs) {
    if (!jobs) return 'All';
    const jobNames = Object.keys(jobs).filter(k => jobs[k]);
    if (jobNames.length === 0) return 'All';
    if (jobNames.includes('All')) return 'All';
    return jobNames.join(',');
}

// ============================================================
// Main Generator
// ============================================================

async function main() {
    console.log('=== Canonical Item Migration Generator ===\n');

    // 1. Parse all rAthena YAML files
    console.log('Parsing rAthena YAML files...');
    const allItems = [];

    for (const yamlFile of YAML_FILES) {
        const content = fs.readFileSync(yamlFile, 'utf8');
        const doc = yaml.load(content);
        if (doc.Body) {
            console.log(`  ${path.basename(yamlFile)}: ${doc.Body.length} items`);
            allItems.push(...doc.Body);
        }
    }
    console.log(`  Total: ${allItems.length} items\n`);

    // Build canonical ID → item map
    const canonicalItems = new Map();
    for (const item of allItems) {
        canonicalItems.set(item.Id, item);
    }

    // 2. Load item descriptions
    console.log('Loading item descriptions...');
    let descriptions = {};
    try {
        const descData = JSON.parse(fs.readFileSync(DESCRIPTIONS_FILE, 'utf8'));
        descriptions = descData.items || {};
        console.log(`  Loaded ${Object.keys(descriptions).length} descriptions\n`);
    } catch (e) {
        console.log(`  Warning: Could not load descriptions: ${e.message}\n`);
    }

    // 3. Build ID mapping (validate against canonical items)
    console.log('Building custom → canonical ID mapping...');
    const idMapping = {};
    const conflicts = [];
    const unmapped = [];

    for (const [oldId, newId] of Object.entries(CUSTOM_TO_CANONICAL)) {
        if (canonicalItems.has(newId)) {
            idMapping[oldId] = newId;
        } else {
            // Check if this ID might be in a different range
            unmapped.push({ oldId, attemptedNewId: newId });
        }
    }

    console.log(`  Mapped: ${Object.keys(idMapping).length} items`);
    if (unmapped.length > 0) {
        console.log(`  Unmapped (canonical ID not found): ${unmapped.length}`);
        for (const u of unmapped) {
            console.log(`    ${u.oldId} → ${u.attemptedNewId} (NOT FOUND in rAthena)`);
        }
    }

    // Handle duplicate mappings (multiple old IDs → same new ID)
    const newIdCounts = {};
    for (const [oldId, newId] of Object.entries(idMapping)) {
        if (!newIdCounts[newId]) newIdCounts[newId] = [];
        newIdCounts[newId].push(parseInt(oldId));
    }
    const duplicates = Object.entries(newIdCounts).filter(([, ids]) => ids.length > 1);
    if (duplicates.length > 0) {
        console.log(`  Warning: ${duplicates.length} canonical IDs mapped from multiple old IDs:`);
        for (const [newId, oldIds] of duplicates) {
            console.log(`    ${newId} ← [${oldIds.join(', ')}]`);
        }
    }
    console.log('');

    // 4. Write id_mapping.json
    const mappingPath = path.join(OUTPUT_DIR, 'id_mapping.json');
    fs.writeFileSync(mappingPath, JSON.stringify(idMapping, null, 2));
    console.log(`Written: ${mappingPath}`);

    // 5. Generate canonical_items.sql
    console.log('\nGenerating canonical_items.sql...');
    generateCanonicalSQL(allItems, canonicalItems, descriptions);

    // 6. Generate ro_item_effects.js
    console.log('\nGenerating ro_item_effects.js...');
    generateItemEffects(allItems);

    // 7. Generate name resolution data
    console.log('\nGenerating name resolution data...');
    generateNameResolution(canonicalItems);

    console.log('\n=== Generation Complete ===');
}

// ============================================================
// Description Stat Correction (Renewal → Pre-Renewal)
// ============================================================

/**
 * Extract all bonuses from rAthena pre-renewal Script field.
 * Returns a map of bonus name → value for description rewriting.
 */
function extractScriptBonuses(script) {
    if (!script) return {};
    const b = {};

    // Simple bonus bXxx,N;
    const re = /bonus\s+b(\w+)\s*,\s*(-?\d+)\s*;/g;
    let m;
    while ((m = re.exec(script)) !== null) {
        const [, type, val] = m;
        const n = parseInt(val);
        switch (type) {
            case 'Str': b.str = (b.str || 0) + n; break;
            case 'Agi': b.agi = (b.agi || 0) + n; break;
            case 'Vit': b.vit = (b.vit || 0) + n; break;
            case 'Int': b.int = (b.int || 0) + n; break;
            case 'Dex': b.dex = (b.dex || 0) + n; break;
            case 'Luk': b.luk = (b.luk || 0) + n; break;
            case 'Def': case 'Def2': b.def = (b.def || 0) + n; break;
            case 'Mdef': case 'Mdef2': b.mdef = (b.mdef || 0) + n; break;
            case 'MaxHP': b.maxhp = (b.maxhp || 0) + n; break;
            case 'MaxSP': b.maxsp = (b.maxsp || 0) + n; break;
            case 'MaxHPrate': case 'MaxHPRate': b.maxhp_rate = (b.maxhp_rate || 0) + n; break;
            case 'MaxSPrate': case 'MaxSPRate': b.maxsp_rate = (b.maxsp_rate || 0) + n; break;
            case 'Hit': b.hit = (b.hit || 0) + n; break;
            case 'Flee': b.flee = (b.flee || 0) + n; break;
            case 'Flee2': b.perfect_dodge = (b.perfect_dodge || 0) + n; break;
            case 'Critical': b.crit = (b.crit || 0) + n; break;
            case 'Atk': case 'Atk2': case 'BaseAtk': b.atk = (b.atk || 0) + n; break;
            case 'Matk': b.matk = (b.matk || 0) + n; break;
            case 'MatkRate': b.matk_rate = (b.matk_rate || 0) + n; break;
            case 'AspdRate': b.aspd_rate = (b.aspd_rate || 0) + n; break;
            case 'Aspd': b.aspd = (b.aspd || 0) + n; break;
            case 'HPrecovRate': b.hp_regen_rate = (b.hp_regen_rate || 0) + n; break;
            case 'SPrecovRate': b.sp_regen_rate = (b.sp_regen_rate || 0) + n; break;
            case 'CritAtkRate': b.crit_atk_rate = (b.crit_atk_rate || 0) + n; break;
            case 'Castrate': case 'CastRate': b.cast_rate = (b.cast_rate || 0) + n; break;
            case 'UseSPrate': case 'UseSPRate': b.use_sp_rate = (b.use_sp_rate || 0) + n; break;
            case 'HealPower': b.heal_power = (b.heal_power || 0) + n; break;
            case 'LongAtkRate': b.long_atk_rate = (b.long_atk_rate || 0) + n; break;
            case 'SplashRange': b.splash = (b.splash || 0) + n; break;
            case 'SpeedRate': b.speed_rate = (b.speed_rate || 0) + n; break;
            case 'SpeedAddRate': b.speed_rate = (b.speed_rate || 0) + n; break;
            case 'Unbreakable': b.unbreakable = true; break;
            case 'NoKnockback': b.no_knockback = true; break;
        }
    }

    // bonus bAllStats,N;
    const allM = script.match(/bonus\s+bAllStats\s*,\s*(-?\d+)\s*;/);
    if (allM) {
        const n = parseInt(allM[1]);
        b.all_stats = n;
        ['str','agi','vit','int','dex','luk'].forEach(s => b[s] = (b[s] || 0) + n);
    }

    // Element: bAtkEle or bDefEle
    const eleM = script.match(/bonus\s+b(?:AtkEle|DefEle)\s*,\s*Ele_(\w+)\s*;/);
    if (eleM) b.element = eleM[1].toLowerCase();

    return b;
}

/**
 * Build bonus text lines from pre-renewal script bonuses.
 * Returns an array of human-readable bonus lines.
 */
function buildBonusLines(b) {
    const lines = [];

    // Stat bonuses — group if all_stats
    if (b.all_stats && b.all_stats !== 0) {
        lines.push(`All Stats ${b.all_stats > 0 ? '+' : ''}${b.all_stats}`);
    } else {
        if (b.str) lines.push(`STR ${b.str > 0 ? '+' : ''}${b.str}`);
        if (b.agi) lines.push(`AGI ${b.agi > 0 ? '+' : ''}${b.agi}`);
        if (b.vit) lines.push(`VIT ${b.vit > 0 ? '+' : ''}${b.vit}`);
        if (b.int) lines.push(`INT ${b.int > 0 ? '+' : ''}${b.int}`);
        if (b.dex) lines.push(`DEX ${b.dex > 0 ? '+' : ''}${b.dex}`);
        if (b.luk) lines.push(`LUK ${b.luk > 0 ? '+' : ''}${b.luk}`);
    }

    // Combat stats
    if (b.atk) lines.push(`ATK ${b.atk > 0 ? '+' : ''}${b.atk}`);
    if (b.matk) lines.push(`MATK ${b.matk > 0 ? '+' : ''}${b.matk}`);
    if (b.matk_rate) lines.push(`MATK ${b.matk_rate > 0 ? '+' : ''}${b.matk_rate}%`);
    if (b.def) lines.push(`DEF ${b.def > 0 ? '+' : ''}${b.def}`);
    if (b.mdef) lines.push(`MDEF ${b.mdef > 0 ? '+' : ''}${b.mdef}`);
    if (b.hit) lines.push(`HIT ${b.hit > 0 ? '+' : ''}${b.hit}`);
    if (b.flee) lines.push(`Flee ${b.flee > 0 ? '+' : ''}${b.flee}`);
    if (b.perfect_dodge) lines.push(`Perfect Dodge ${b.perfect_dodge > 0 ? '+' : ''}${b.perfect_dodge}`);
    if (b.crit) lines.push(`Critical ${b.crit > 0 ? '+' : ''}${b.crit}`);

    // ASPD
    if (b.aspd) lines.push(`ASPD ${b.aspd > 0 ? '+' : ''}${b.aspd}`);
    if (b.aspd_rate) lines.push(`Reduces after attack delay by ${b.aspd_rate}%.`);

    // HP/SP
    if (b.maxhp) lines.push(`MaxHP ${b.maxhp > 0 ? '+' : ''}${b.maxhp}`);
    if (b.maxsp) lines.push(`MaxSP ${b.maxsp > 0 ? '+' : ''}${b.maxsp}`);
    if (b.maxhp_rate) lines.push(`MaxHP ${b.maxhp_rate > 0 ? '+' : ''}${b.maxhp_rate}%`);
    if (b.maxsp_rate) lines.push(`MaxSP ${b.maxsp_rate > 0 ? '+' : ''}${b.maxsp_rate}%`);

    // Regen
    if (b.hp_regen_rate) lines.push(`HP recovery ${b.hp_regen_rate > 0 ? '+' : ''}${b.hp_regen_rate}%`);
    if (b.sp_regen_rate) lines.push(`SP recovery ${b.sp_regen_rate > 0 ? '+' : ''}${b.sp_regen_rate}%`);

    // Cast / SP cost
    if (b.cast_rate) lines.push(`Reduces cast time by ${Math.abs(b.cast_rate)}%.`);
    if (b.use_sp_rate) lines.push(`SP consumption ${b.use_sp_rate > 0 ? '+' : ''}${b.use_sp_rate}%`);

    // Misc
    if (b.heal_power) lines.push(`Increases healing by ${b.heal_power}%.`);
    if (b.crit_atk_rate) lines.push(`Critical damage ${b.crit_atk_rate > 0 ? '+' : ''}${b.crit_atk_rate}%`);
    if (b.long_atk_rate) lines.push(`Ranged attack ${b.long_atk_rate > 0 ? '+' : ''}${b.long_atk_rate}%`);
    if (b.splash) lines.push(`Splash attack range +${b.splash}`);
    if (b.speed_rate) lines.push(`Movement speed ${b.speed_rate > 0 ? '+' : ''}${b.speed_rate}%`);
    if (b.unbreakable) lines.push('Indestructible in battle.');
    if (b.no_knockback) lines.push('Prevents knockback.');

    return lines;
}

/**
 * Insert a line into text after the first line matching a pattern.
 */
function insertAfterLine(text, afterPattern, newLine) {
    const lines = text.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (afterPattern.test(lines[i])) {
            lines.splice(i + 1, 0, newLine);
            return lines.join('\n');
        }
    }
    // Pattern not found — prepend
    return newLine + '\n' + text;
}

/**
 * Get human-readable item type name from rAthena item data.
 */
function getItemTypeName(item) {
    if (item.Type === 'Weapon') {
        const map = {
            'Dagger': 'Dagger', '1hSword': 'One-Handed Sword', '2hSword': 'Two-Handed Sword',
            '1hSpear': 'One-Handed Spear', '2hSpear': 'Two-Handed Spear',
            '1hAxe': 'One-Handed Axe', '2hAxe': 'Two-Handed Axe',
            'Mace': 'Mace', '2hMace': 'Two-Handed Mace', 'Staff': 'One-Handed Staff',
            '2hStaff': 'Two-Handed Staff', 'Bow': 'Bow', 'Knuckle': 'Knuckle',
            'Musical': 'Musical Instrument', 'Whip': 'Whip', 'Book': 'Book',
            'Katar': 'Katar', 'Revolver': 'Revolver', 'Rifle': 'Rifle',
            'Shotgun': 'Shotgun', 'Gatling': 'Gatling Gun', 'Grenade': 'Grenade Launcher',
            'Huuma': 'Huuma Shuriken',
        };
        return map[item.SubType] || 'Weapon';
    }
    if (item.Type === 'Armor') {
        const loc = item.Locations || {};
        if (loc.Left_Hand) return 'Shield';
        if (loc.Armor) return 'Armor';
        if (loc.Garment) return 'Garment';
        if (loc.Shoes) return 'Footgear';
        if (loc.Head_Top || loc.Head_Mid || loc.Head_Low) return 'Headgear';
        if (loc.Both_Accessory || loc.Right_Accessory || loc.Left_Accessory) return 'Accessory';
        return 'Armor';
    }
    return null;
}

/**
 * Comprehensive description fixer: replaces Renewal-era stats and bonus text
 * with correct pre-renewal values from the rAthena YAML source.
 *
 * Description structure varies:
 *   CASE A (4+ sections): flavor | bonus(es) | stats | requirement
 *   CASE B (3 sections):  flavor | stats | requirement  (no bonus area)
 *   CASE C (multi-bonus): flavor | bonus1 | bonus2 | ... | stats | requirement
 *
 * The stats section is identified by containing "Type:" or "Defense:" or "Attack:".
 * The requirement section is identified by containing "Requirement:".
 */
function fixDescriptionStats(text, item) {
    const SEP = '________________________';
    const sections = text.split(SEP);

    if (sections.length < 3) {
        // Too few sections — rebuild description with proper structure
        let fixed = text;
        fixed = fixed.replace(/^(Defense\s*:\s*)\d+/mi, `$1${item.Defense || 0}`);
        if (item.Attack > 0) fixed = fixed.replace(/^(Attack\s*:\s*)\d+/mi, `$1${item.Attack}`);

        // Check if description is missing a stats section (no "Type:" or "Attack:/Defense:")
        const hasStatsSection = /Type\s*:/i.test(fixed) || (/(?:Defense|Attack)\s*:/i.test(fixed) && /Weight\s*:/i.test(fixed));
        const scriptBonuses = extractScriptBonuses(item.Script);
        const bonusLines = buildBonusLines(scriptBonuses);
        const needsBonusSection = bonusLines.length > 0 && !bonusLines.some(bl => fixed.includes(bl));

        if (needsBonusSection || !hasStatsSection) {
            // Rebuild with proper structure: flavor | bonus | stats
            const flavor = sections[0] ? sections[0].trim() : item.Name;
            const parts = [flavor];
            // Bonus section (if any)
            if (bonusLines.length > 0) {
                parts.push('\n' + bonusLines.join('\n') + '\n');
            }
            // Stats section
            const statsLines = [];
            const typeName = getItemTypeName(item);
            if (typeName) statsLines.push('Type: ' + typeName);
            if (item.Attack > 0) statsLines.push('Attack: ' + item.Attack);
            if (item.Defense > 0) statsLines.push('Defense: ' + (item.Defense || 0));
            if (item.Weight > 0) statsLines.push('Weight: ' + Math.floor(item.Weight / 10));
            if (item.WeaponLevel > 0) statsLines.push('Weapon Level: ' + item.WeaponLevel);
            if (item.Refineable) statsLines.push('Refinable: Yes');
            if (item.Slots > 0) statsLines.push('Slots: ' + item.Slots);
            if (statsLines.length > 0) parts.push('\n' + statsLines.join('\n') + '\n');
            return parts.join(SEP);
        }
        return fixed;
    }

    // --- Identify section roles ---
    // Find the stats section (contains "Type:" or base "Defense:" or "Attack:" + "Weight:")
    let statsIdx = -1;
    for (let i = 1; i < sections.length; i++) {
        const s = sections[i].trim();
        if (/^Type\s*:/mi.test(s) || (/(?:Defense|Attack)\s*:/i.test(s) && /Weight\s*:/i.test(s))) {
            statsIdx = i;
            break;
        }
    }

    // Find the requirement section
    let reqIdx = -1;
    for (let i = sections.length - 1; i >= 1; i--) {
        if (/Requirement/i.test(sections[i])) {
            reqIdx = i;
            break;
        }
    }

    // If no stats section found, fall back to inline fixes only
    if (statsIdx === -1) {
        let fixed = text;
        fixed = fixed.replace(/^(Defense\s*:\s*)\d+/mi, `$1${item.Defense || 0}`);
        if (item.Attack > 0) fixed = fixed.replace(/^(Attack\s*:\s*)\d+/mi, `$1${item.Attack}`);
        return fixed;
    }

    // --- Fix ALL properties in the stats section from YAML values ---
    let st = sections[statsIdx];

    // Fix Type line to match YAML
    const correctType = getItemTypeName(item);
    if (correctType) {
        st = st.replace(/^(Type\s*:\s*).+$/mi, `$1${correctType}`);
    }

    // Fix/add Defense
    const yamlDef = item.Defense || 0;
    if (/Defense\s*:/i.test(st)) {
        st = st.replace(/^(Defense\s*:\s*)\d+/mi, `$1${yamlDef}`);
    } else if (yamlDef > 0) {
        st = insertAfterLine(st, /^Type\s*:/mi, `Defense: ${yamlDef}`);
    }

    // Fix/add Attack
    const yamlAtk = item.Attack || 0;
    if (/Attack\s*:/i.test(st)) {
        if (yamlAtk > 0) st = st.replace(/^(Attack\s*:\s*)\d+/mi, `$1${yamlAtk}`);
    } else if (yamlAtk > 0) {
        st = insertAfterLine(st, /^Type\s*:/mi, `Attack: ${yamlAtk}`);
    }

    // Fix Weight from YAML (rAthena stores weight×10)
    const yamlWeight = Math.floor((item.Weight || 0) / 10);
    if (/Weight\s*:/i.test(st)) {
        st = st.replace(/^(Weight\s*:\s*)[\d.]+/mi, `$1${yamlWeight}`);
    } else {
        st = insertAfterLine(st, /^(?:Attack|Defense)\s*:/mi, `Weight: ${yamlWeight}`);
    }

    // Fix Element from YAML script
    const scriptBonusesForEle = extractScriptBonuses(item.Script);
    const yamlElement = scriptBonusesForEle.element || null;
    if (yamlElement && yamlElement !== 'neutral') {
        const eleCap = yamlElement.charAt(0).toUpperCase() + yamlElement.slice(1);
        if (/Element\s*:/i.test(st)) {
            st = st.replace(/^(Element\s*:\s*).+$/mi, `$1${eleCap}`);
        } else {
            st = insertAfterLine(st, /^(?:Attack|Defense|Weight)\s*:/mi, `Element: ${eleCap}`);
        }
    } else if (/Element\s*:/i.test(st) && (!yamlElement || yamlElement === 'neutral')) {
        // Remove element line if item is neutral
        st = st.replace(/^Element\s*:.*\n?/mi, '');
    }

    // Fix Weapon Level from YAML
    const yamlWL = item.WeaponLevel || 0;
    if (yamlWL > 0 && /Weapon Level\s*:/i.test(st)) {
        st = st.replace(/^(Weapon Level\s*:\s*)\d+/mi, `$1${yamlWL}`);
    } else if (yamlWL > 0 && !/Weapon Level\s*:/i.test(st)) {
        st = insertAfterLine(st, /^(?:Weight|Element)\s*:/mi, `Weapon Level: ${yamlWL}`);
    }

    // Fix Refinable from YAML
    const yamlRef = item.Refineable || false;
    if (/Refin(?:able|able)\s*:/i.test(st)) {
        st = st.replace(/^(Refin(?:able|able)\s*:\s*)\w+/mi, `$1${yamlRef ? 'Yes' : 'No'}`);
    }

    sections[statsIdx] = st;

    // --- Fix Requirements section ---
    if (reqIdx >= 0) {
        let req = sections[reqIdx];
        const yamlReqLevel = item.EquipLevelMin || 0;
        if (yamlReqLevel > 0) {
            if (/Base\s+level\s+\d+/i.test(req)) {
                req = req.replace(/^(Base\s+level\s+)\d+/mi, `$1${yamlReqLevel}`);
            } else {
                req = req.replace(/(Requirement\s*:\s*\n?)/i, `$1Base level ${yamlReqLevel}\n`);
            }
        } else if (/Base\s+level\s+\d+/i.test(req)) {
            // YAML has no level requirement — remove renewal-era "Base level N" from desc
            req = req.replace(/^Base\s+level\s+\d+\s*\n?/mi, '');
        }
        sections[reqIdx] = req;
    }

    // --- Rebuild the bonus area (sections between flavor and stats) ---
    // sections[0] = flavor text
    // sections[1..statsIdx-1] = bonus sections (one or more)
    // sections[statsIdx] = stats
    // sections[statsIdx+1..] = requirement etc.
    const bonusSectionIndices = [];
    for (let i = 1; i < statsIdx; i++) {
        bonusSectionIndices.push(i);
    }

    if (bonusSectionIndices.length === 0) {
        // No existing bonus area — check if we need to INSERT one
        const scriptBonuses = extractScriptBonuses(item.Script);
        const newBonusLines = buildBonusLines(scriptBonuses);
        if (newBonusLines.length > 0) {
            // Insert a bonus section before the stats section
            const newSections = [sections[0]]; // flavor
            newSections.push('\n' + newBonusLines.join('\n') + '\n'); // new bonus section
            for (let i = statsIdx; i < sections.length; i++) {
                newSections.push(sections[i]);
            }
            return newSections.join(SEP);
        }
        return sections.join(SEP);
    }

    // Collect all original bonus lines across all bonus sections
    const origBonusText = bonusSectionIndices.map(i => sections[i]).join('\n');
    const origLines = origBonusText.split('\n').map(l => l.trim()).filter(l => l.length > 0);

    // Separate lines into "kept" (special effects) vs "dropped" (stat bonuses we'll rebuild)
    const keptLines = [];
    for (const line of origLines) {
        // Keep special effect descriptions (complex effects not in parseBonusScript)
        if (/^(Random chance|Adds? (a )?chance|Chance to|Has a|Enables?|Drains?|Increases? damage of|Autocast|Auto.?cast|Reflects?|Damage to|Damage from|Absorb|Each|Every|Ignores?|Bypass|Gain |Restores?|Recovers? |Increases? (the )?resist|Indestructible|Inflicts?|Makes? |Impervious|Immune|Cannot|Prevents?|Receives?|Protection|Skills? .* (is|are)|healing effectiveness)/i.test(line)) {
            keptLines.push(line);
            continue;
        }
        // Keep "Rental Item"
        if (/^Rental Item$/i.test(line)) { keptLines.push(line); continue; }
        // Keep element property descriptions
        if (/^(Holy|Fire|Water|Wind|Earth|Shadow|Ghost|Poison|Undead|Neutral)\s+(property|element)/i.test(line)) {
            keptLines.push(line); continue;
        }
        // Keep "Can be enchanted" and "Weapon element" lines
        if (/^(Can be enchanted|Weapon element|Element:)/i.test(line)) { keptLines.push(line); continue; }
        // Keep "When VIP" blocks warning — mark as removed (renewal-only VIP bonuses)
        if (/^When VIP/i.test(line)) continue;
        // Keep conditional lines about job/level requirements in bonus area
        if (/^(Usable by|Only for|Exclusive|When equipped|While|During|Worn by|Job Level)/i.test(line)) {
            keptLines.push(line); continue;
        }
        // Drop stat bonus lines (STR +N, ATK +N, DEF +N, MATK +N, MDEF +N, etc.)
        // These will be rebuilt from the pre-renewal script
    }

    // Build pre-renewal bonus lines from YAML Script
    const scriptBonuses = extractScriptBonuses(item.Script);
    const newBonusLines = buildBonusLines(scriptBonuses);

    // Combine: rebuilt pre-renewal bonuses + kept special effect lines
    const allLines = [...newBonusLines, ...keptLines];

    // Replace all bonus sections with a single rebuilt one
    // Remove old bonus sections and insert the new one
    const newSections = [sections[0]]; // flavor
    newSections.push(allLines.length > 0 ? '\n' + allLines.join('\n') + '\n' : '\n'); // single bonus section
    for (let i = statsIdx; i < sections.length; i++) {
        newSections.push(sections[i]); // stats + requirement
    }

    return newSections.join(SEP);
}

// ============================================================
// SQL Generation
// ============================================================

function generateCanonicalSQL(allItems, canonicalItems, descriptions) {
    const lines = [];

    lines.push('-- ============================================================');
    lines.push('-- Canonical rAthena Items (auto-generated)');
    lines.push('-- Generated by: node scripts/generate_canonical_migration.js');
    lines.push(`-- Date: ${new Date().toISOString()}`);
    lines.push(`-- Total items: ${allItems.length}`);
    lines.push('-- ============================================================');
    lines.push('');

    // Process items in batches of 100 for manageable SQL
    const BATCH_SIZE = 100;
    const columns = [
        'item_id', 'name', 'aegis_name', 'description', 'full_description',
        'item_type', 'equip_slot', 'weight', 'price', 'buy_price', 'sell_price',
        'atk', 'def', 'matk', 'mdef',
        'str_bonus', 'agi_bonus', 'vit_bonus', 'int_bonus', 'dex_bonus', 'luk_bonus',
        'max_hp_bonus', 'max_sp_bonus', 'hit_bonus', 'flee_bonus', 'critical_bonus', 'perfect_dodge_bonus',
        'max_hp_rate', 'max_sp_rate', 'aspd_rate',
        'hp_regen_rate', 'sp_regen_rate', 'crit_atk_rate', 'cast_rate', 'use_sp_rate', 'heal_power',
        'required_level', 'stackable', 'max_stack', 'icon',
        'weapon_type', 'weapon_level', 'armor_level', 'weapon_range', 'slots',
        'refineable', 'sub_type', 'view_sprite',
        'equip_level_min', 'equip_level_max',
        'jobs_allowed', 'classes_allowed', 'gender_allowed', 'equip_locations',
        'element', 'two_handed',
        'script', 'equip_script', 'unequip_script'
    ];

    let itemCount = 0;

    for (let batchStart = 0; batchStart < allItems.length; batchStart += BATCH_SIZE) {
        const batch = allItems.slice(batchStart, batchStart + BATCH_SIZE);

        lines.push(`INSERT INTO items (${columns.join(', ')}) VALUES`);

        const valueLines = [];
        for (const item of batch) {
            const itemType = normalizeItemType(item.Type);
            const equipSlot = (itemType === 'card')
                ? null  // Cards don't equip, they compound
                : normalizeEquipSlot(item.Locations);

            const bonuses = parseBonusScript(item.Script);
            const { stackable, maxStack } = getStackInfo(item);

            // Price handling: rAthena stores Buy and/or Sell
            let buyPrice = item.Buy || 0;
            let sellPrice = item.Sell || 0;
            if (buyPrice > 0 && sellPrice === 0) sellPrice = Math.floor(buyPrice / 2);
            if (sellPrice > 0 && buyPrice === 0) buyPrice = sellPrice * 2;

            // Weight: rAthena stores weight×10
            const weight = Math.floor((item.Weight || 0) / 10);

            // Description from item_descriptions.json
            // NOTE: item_descriptions.json contains Renewal-era client text.
            // We post-process to replace Renewal DEF/ATK values with pre-renewal YAML values.
            const desc = descriptions[String(item.Id)];
            const description = desc ? desc.description : item.Name;
            const rawFullDescription = desc && desc.fullDescription
                ? desc.fullDescription.join('\n')
                : null;
            let fullDescription = rawFullDescription
                ? fixDescriptionStats(rawFullDescription, item)
                : null;

            // For items with no description but has script bonuses, generate one
            if (!fullDescription && item.Script && (item.Type === 'Weapon' || item.Type === 'Armor')) {
                const sb = extractScriptBonuses(item.Script);
                const bl = buildBonusLines(sb);
                if (bl.length > 0) {
                    const SEP = '________________________';
                    const parts = [item.Name, '\n' + bl.join('\n') + '\n'];
                    const statsLines = [];
                    const tn = getItemTypeName(item);
                    if (tn) statsLines.push('Type: ' + tn);
                    if (item.Attack > 0) statsLines.push('Attack: ' + item.Attack);
                    if (item.Defense > 0) statsLines.push('Defense: ' + (item.Defense || 0));
                    if (item.Weight > 0) statsLines.push('Weight: ' + Math.floor(item.Weight / 10));
                    if (item.WeaponLevel > 0) statsLines.push('Weapon Level: ' + item.WeaponLevel);
                    if (statsLines.length > 0) parts.push('\n' + statsLines.join('\n') + '\n');
                    fullDescription = parts.join(SEP);
                }
            }

            // Build icon from AegisName (lowercase, for asset references)
            const icon = (item.AegisName || item.Name || 'default_item').toLowerCase();

            // Weapon-specific fields
            const weaponType = itemType === 'weapon' ? normalizeWeaponType(item.SubType) : null;
            const weaponLevel = item.WeaponLevel || 0;
            const armorLevel = item.ArmorLevel || 0;
            const weaponRange = item.Range || 0;
            const slots = item.Slots || 0;
            const refineable = item.Refineable || false;
            const subType = item.SubType || null;
            const viewSprite = item.View || 0;
            const equipLevelMin = item.EquipLevelMin || 0;
            const equipLevelMax = item.EquipLevelMax || 0;

            // ATK/DEF from rAthena
            const atk = (item.Attack || 0) + (bonuses.atk_bonus || 0);
            const def = (item.Defense || 0) + (bonuses.def_bonus || 0);
            const matk = (item.MagicAttack || 0) + (bonuses.matk || 0);
            const mdef = bonuses.mdef || 0;

            const values = [
                item.Id,                                         // item_id
                sqlEscape(item.Name),                            // name
                sqlEscape(item.AegisName),                       // aegis_name
                sqlEscape(description),                          // description
                sqlEscape(fullDescription),                      // full_description
                sqlEscape(itemType),                             // item_type
                sqlEscape(equipSlot),                            // equip_slot
                weight,                                          // weight
                sellPrice,                                       // price (legacy, = sell_price)
                buyPrice,                                        // buy_price
                sellPrice,                                       // sell_price
                atk,                                             // atk
                def,                                             // def
                matk,                                            // matk
                mdef,                                            // mdef
                bonuses.str_bonus || 0,                          // str_bonus
                bonuses.agi_bonus || 0,                          // agi_bonus
                bonuses.vit_bonus || 0,                          // vit_bonus
                bonuses.int_bonus || 0,                          // int_bonus
                bonuses.dex_bonus || 0,                          // dex_bonus
                bonuses.luk_bonus || 0,                          // luk_bonus
                bonuses.max_hp_bonus || 0,                       // max_hp_bonus
                bonuses.max_sp_bonus || 0,                       // max_sp_bonus
                bonuses.hit_bonus || 0,                          // hit_bonus
                bonuses.flee_bonus || 0,                         // flee_bonus
                bonuses.critical_bonus || 0,                     // critical_bonus
                bonuses.perfect_dodge_bonus || 0,                // perfect_dodge_bonus
                bonuses.max_hp_rate || 0,                        // max_hp_rate
                bonuses.max_sp_rate || 0,                        // max_sp_rate
                bonuses.aspd_rate || 0,                          // aspd_rate
                bonuses.hp_regen_rate || 0,                      // hp_regen_rate
                bonuses.sp_regen_rate || 0,                      // sp_regen_rate
                bonuses.crit_atk_rate || 0,                      // crit_atk_rate
                bonuses.cast_rate || 0,                          // cast_rate
                bonuses.use_sp_rate || 0,                        // use_sp_rate
                bonuses.heal_power || 0,                         // heal_power
                Math.max(1, equipLevelMin || 1),                 // required_level
                stackable,                                       // stackable
                maxStack,                                        // max_stack
                sqlEscape(icon),                                 // icon
                sqlEscape(weaponType),                           // weapon_type
                weaponLevel,                                     // weapon_level
                armorLevel,                                      // armor_level
                weaponRange,                                     // weapon_range
                slots,                                           // slots
                refineable,                                      // refineable
                sqlEscape(subType),                              // sub_type
                viewSprite,                                      // view_sprite
                equipLevelMin,                                   // equip_level_min
                equipLevelMax,                                   // equip_level_max
                sqlEscape(serializeJobs(item.Jobs)),             // jobs_allowed
                sqlEscape(item.Classes ? Object.keys(item.Classes).filter(k => item.Classes[k]).join(',') : 'All'), // classes_allowed
                sqlEscape(item.Gender || 'Both'),                // gender_allowed
                sqlEscape(serializeLocations(item.Locations)),   // equip_locations
                sqlEscape(bonuses.element || 'neutral'),         // element
                isTwoHanded(item.SubType, item.Locations),       // two_handed
                sqlEscape(item.Script || null),                  // script
                sqlEscape(item.EquipScript || null),             // equip_script
                sqlEscape(item.UnEquipScript || null)             // unequip_script
            ];

            valueLines.push(`(${values.join(', ')})`);
            itemCount++;
        }

        lines.push(valueLines.join(',\n'));
        lines.push('ON CONFLICT (item_id) DO UPDATE SET');
        lines.push('  name = EXCLUDED.name,');
        lines.push('  aegis_name = EXCLUDED.aegis_name,');
        lines.push('  description = EXCLUDED.description,');
        lines.push('  full_description = EXCLUDED.full_description,');
        lines.push('  item_type = EXCLUDED.item_type,');
        lines.push('  equip_slot = EXCLUDED.equip_slot,');
        lines.push('  weight = EXCLUDED.weight,');
        lines.push('  price = EXCLUDED.price,');
        lines.push('  buy_price = EXCLUDED.buy_price,');
        lines.push('  sell_price = EXCLUDED.sell_price,');
        lines.push('  atk = EXCLUDED.atk,');
        lines.push('  def = EXCLUDED.def,');
        lines.push('  matk = EXCLUDED.matk,');
        lines.push('  mdef = EXCLUDED.mdef,');
        lines.push('  str_bonus = EXCLUDED.str_bonus,');
        lines.push('  agi_bonus = EXCLUDED.agi_bonus,');
        lines.push('  vit_bonus = EXCLUDED.vit_bonus,');
        lines.push('  int_bonus = EXCLUDED.int_bonus,');
        lines.push('  dex_bonus = EXCLUDED.dex_bonus,');
        lines.push('  luk_bonus = EXCLUDED.luk_bonus,');
        lines.push('  max_hp_bonus = EXCLUDED.max_hp_bonus,');
        lines.push('  max_sp_bonus = EXCLUDED.max_sp_bonus,');
        lines.push('  hit_bonus = EXCLUDED.hit_bonus,');
        lines.push('  flee_bonus = EXCLUDED.flee_bonus,');
        lines.push('  critical_bonus = EXCLUDED.critical_bonus,');
        lines.push('  perfect_dodge_bonus = EXCLUDED.perfect_dodge_bonus,');
        lines.push('  max_hp_rate = EXCLUDED.max_hp_rate,');
        lines.push('  max_sp_rate = EXCLUDED.max_sp_rate,');
        lines.push('  aspd_rate = EXCLUDED.aspd_rate,');
        lines.push('  hp_regen_rate = EXCLUDED.hp_regen_rate,');
        lines.push('  sp_regen_rate = EXCLUDED.sp_regen_rate,');
        lines.push('  crit_atk_rate = EXCLUDED.crit_atk_rate,');
        lines.push('  cast_rate = EXCLUDED.cast_rate,');
        lines.push('  use_sp_rate = EXCLUDED.use_sp_rate,');
        lines.push('  heal_power = EXCLUDED.heal_power,');
        lines.push('  required_level = EXCLUDED.required_level,');
        lines.push('  stackable = EXCLUDED.stackable,');
        lines.push('  max_stack = EXCLUDED.max_stack,');
        lines.push('  icon = EXCLUDED.icon,');
        lines.push('  weapon_type = EXCLUDED.weapon_type,');
        lines.push('  weapon_level = EXCLUDED.weapon_level,');
        lines.push('  armor_level = EXCLUDED.armor_level,');
        lines.push('  weapon_range = EXCLUDED.weapon_range,');
        lines.push('  slots = EXCLUDED.slots,');
        lines.push('  refineable = EXCLUDED.refineable,');
        lines.push('  sub_type = EXCLUDED.sub_type,');
        lines.push('  view_sprite = EXCLUDED.view_sprite,');
        lines.push('  equip_level_min = EXCLUDED.equip_level_min,');
        lines.push('  equip_level_max = EXCLUDED.equip_level_max,');
        lines.push('  jobs_allowed = EXCLUDED.jobs_allowed,');
        lines.push('  classes_allowed = EXCLUDED.classes_allowed,');
        lines.push('  gender_allowed = EXCLUDED.gender_allowed,');
        lines.push('  equip_locations = EXCLUDED.equip_locations,');
        lines.push('  element = EXCLUDED.element,');
        lines.push('  two_handed = EXCLUDED.two_handed,');
        lines.push('  script = EXCLUDED.script,');
        lines.push('  equip_script = EXCLUDED.equip_script,');
        lines.push('  unequip_script = EXCLUDED.unequip_script;');
        lines.push('');
    }

    const sqlPath = path.join(OUTPUT_DIR, 'canonical_items.sql');
    fs.writeFileSync(sqlPath, lines.join('\n'));
    console.log(`  Written: ${sqlPath} (${itemCount} items)`);
}

// ============================================================
// Item Effects Generation
// ============================================================

function generateItemEffects(allItems) {
    const effects = {};
    let parsedCount = 0;

    for (const item of allItems) {
        const itemType = normalizeItemType(item.Type);
        if (itemType !== 'consumable') continue;
        if (!item.Script) continue;

        const effect = parseConsumableScript(item.Script);
        if (effect) {
            effects[item.Id] = effect;
            parsedCount++;
        }
    }

    // Generate the JS file
    const lines = [];
    lines.push('// Auto-generated by scripts/generate_canonical_migration.js');
    lines.push('// Consumable item use-effect data parsed from rAthena scripts');
    lines.push(`// Generated: ${new Date().toISOString()}`);
    lines.push(`// Total effects: ${parsedCount}`);
    lines.push('//');
    lines.push('// Effect types:');
    lines.push('//   heal        — { hpMin, hpMax, spMin, spMax } fixed heal');
    lines.push('//   percentheal — { hp, sp } percentage heal');
    lines.push('//   cure        — { cures: [SC_...] } remove status effects');
    lines.push('//   itemskill   — { skill, level } trigger skill on use');
    lines.push('//   sc_start    — { status, duration, value } apply status');
    lines.push('//   multi       — { effects: [...] } multiple effects');
    lines.push('');
    lines.push('const ITEM_USE_EFFECTS = {');

    // Sort by item ID for readability
    const sortedIds = Object.keys(effects).map(Number).sort((a, b) => a - b);

    for (let i = 0; i < sortedIds.length; i++) {
        const id = sortedIds[i];
        const effect = effects[id];
        // Find item name for comment
        const item = allItems.find(it => it.Id === id);
        const name = item ? item.Name : 'Unknown';

        const comma = i < sortedIds.length - 1 ? ',' : '';
        lines.push(`    ${id}: ${JSON.stringify(effect)}${comma} // ${name}`);
    }

    lines.push('};');
    lines.push('');
    lines.push('module.exports = { ITEM_USE_EFFECTS };');

    const effectsPath = path.join(SERVER_DIR, 'ro_item_effects.js');
    fs.writeFileSync(effectsPath, lines.join('\n'));
    console.log(`  Written: ${effectsPath} (${parsedCount} effects)`);
}

// ============================================================
// Name Resolution (for monster template fixes)
// ============================================================

function generateNameResolution(canonicalItems) {
    // Build a name→id map from canonical items
    const canonicalNameMap = new Map();
    for (const [id, item] of canonicalItems) {
        canonicalNameMap.set(item.Name, id);
    }

    // Build normalized name → canonical name map for fuzzy matching
    const canonicalNormMap = new Map();
    for (const [id, item] of canonicalItems) {
        const norm = normalizeName(item.Name);
        // Keep first occurrence (lower IDs tend to be more common items)
        if (!canonicalNormMap.has(norm)) {
            canonicalNormMap.set(norm, item.Name);
        }
    }

    // Also build AegisName lookups (underscore-separated names)
    const aegisNameMap = new Map();
    for (const [id, item] of canonicalItems) {
        if (item.AegisName) {
            // Convert AegisName to display format: "Red_Potion" → "red potion"
            const displayName = item.AegisName.replace(/_+$/g, '').replace(/_/g, ' ').toLowerCase();
            if (!aegisNameMap.has(displayName)) {
                aegisNameMap.set(displayName, item.Name);
            }
        }
    }

    // Load monster templates to find all referenced item names
    const templatesPath = path.join(__dirname, '..', 'server', 'src', 'ro_monster_templates.js');
    const templatesContent = fs.readFileSync(templatesPath, 'utf8');

    // Extract all itemName references
    const itemNameRegex = /itemName:\s*['"]([^'"]+)['"]/g;
    const referencedNames = new Set();
    let match;
    while ((match = itemNameRegex.exec(templatesContent)) !== null) {
        referencedNames.add(match[1]);
    }

    console.log(`  Found ${referencedNames.size} unique item names in monster templates`);

    // Check which resolve and which don't
    let resolved = 0;
    let unresolved = 0;
    const nameFixMap = {};
    const stillUnresolved = [];

    for (const name of referencedNames) {
        if (canonicalNameMap.has(name)) {
            resolved++;
        } else {
            // Try to find closest match
            let closestMatch = findClosestName(name, canonicalNameMap, canonicalNormMap);

            // Also try AegisName-based matching
            if (!closestMatch) {
                const normName = name.replace(/\\/g, "'s").replace(/[''`]/g, "'").toLowerCase();
                closestMatch = aegisNameMap.get(normName);
            }

            if (closestMatch) {
                nameFixMap[name] = closestMatch;
                resolved++;
            } else {
                unresolved++;
                stillUnresolved.push(name);
            }
        }
    }

    console.log(`  Directly resolved: ${resolved - Object.keys(nameFixMap).length}`);
    console.log(`  Auto-matched: ${Object.keys(nameFixMap).length}`);
    console.log(`  Total resolved: ${resolved} / ${referencedNames.size} (${(resolved/referencedNames.size*100).toFixed(1)}%)`);
    console.log(`  Still unresolved: ${unresolved}`);

    const nameFixPath = path.join(OUTPUT_DIR, 'name_fixes.json');
    fs.writeFileSync(nameFixPath, JSON.stringify(nameFixMap, null, 2));
    console.log(`  Written: ${nameFixPath}`);

    // Also save unresolved list for manual review
    const unresolvedPath = path.join(OUTPUT_DIR, 'unresolved_names.txt');
    fs.writeFileSync(unresolvedPath, stillUnresolved.sort().join('\n'));
    console.log(`  Written: ${unresolvedPath} (${stillUnresolved.length} items need manual review)`);
}

/**
 * Normalize a name for fuzzy matching
 */
function normalizeName(name) {
    return name
        .replace(/\\/g, "'s")  // backslash = truncated apostrophe: "Alice\" → "Alice's"
        .replace(/[''`]/g, "'")  // normalize apostrophe types
        .replace(/\s+/g, ' ')   // collapse whitespace
        .trim()
        .toLowerCase();
}

/**
 * Find closest matching canonical name for an unresolved name
 */
function findClosestName(name, canonicalNameMap, canonicalNormMap) {
    // 1. Backslash = truncated apostrophe
    if (name.endsWith('\\')) {
        const withApostrophe = name.slice(0, -1) + "'s";
        for (const [canonName] of canonicalNameMap) {
            if (canonName === withApostrophe) return canonName;
        }
    }

    // 2. Normalized matching (case, apostrophe, whitespace)
    const norm = normalizeName(name);
    const match = canonicalNormMap.get(norm);
    if (match) return match;

    // 3. Try without apostrophes entirely
    const noApostrophe = norm.replace(/'/g, '').replace(/\s+/g, ' ');
    for (const [normCanon, canonName] of canonicalNormMap) {
        if (normCanon.replace(/'/g, '').replace(/\s+/g, ' ') === noApostrophe) return canonName;
    }

    // 4. Try "Of" vs "of" normalization
    const withLowerOf = name.replace(/\bOf\b/g, 'of');
    if (canonicalNameMap.has(withLowerOf)) return withLowerOf;

    // 5. Common typo corrections
    const typoMap = {
        'alchol': 'alcohol',
        'assasin': 'assassin',
        'balistar': 'ballista',
        'aloebera': 'aloe vera',
        'apocalips': 'apocalypse',
        'arclouse': 'arclouse',
        'adventurere': 'adventurer',
    };
    let fixed = norm;
    for (const [typo, correction] of Object.entries(typoMap)) {
        fixed = fixed.replace(typo, correction);
    }
    if (fixed !== norm) {
        const fixMatch = canonicalNormMap.get(fixed);
        if (fixMatch) return fixMatch;
    }

    return null;
}

// ============================================================
// Run
// ============================================================

main().catch(err => {
    console.error('Fatal error:', err);
    process.exit(1);
});
