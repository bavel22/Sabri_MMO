/**
 * Ragnarok Online Monster Templates — Pre-Renewal Database
 * 
 * Auto-generated from rAthena pre-renewal mob_db.yml
 * Source: https://github.com/rathena/rathena/blob/master/db/pre-re/mob_db.yml
 * 
 * Total monsters: 509
 * Level range: 1 - 99
 * 
 * Generated: 2026-02-24T06:47:41.909Z
 * 
 * Structure per template:
 *   id, aegisName, name, level, maxHealth, baseExp, jobExp, mvpExp,
 *   attack, attack2, defense, magicDefense,
 *   str, agi, vit, int, dex, luk,
 *   attackRange, aggroRange, chaseRange,
 *   aspd, walkSpeed, attackDelay, attackMotion, damageMotion,
 *   size, race, element {type, level}, monsterClass, aiType,
 *   respawnMs, raceGroups, stats {}, modes {}, drops [], mvpDrops []
 */

'use strict';

const RO_MONSTER_TEMPLATES = {};

// ──── Poring (ID: 1002) ──── Level 1 | HP 50 | NORMAL | plant/water1 | passive
RO_MONSTER_TEMPLATES['poring'] = {
    id: 1002, name: 'Poring', aegisName: 'PORING',
    level: 1, maxHealth: 50, baseExp: 2, jobExp: 1, mvpExp: 0,
    attack: 7, attack2: 10, defense: 0, magicDefense: 5,
    str: 0, agi: 0, vit: 0, int: 0, dex: 6, luk: 30,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 400, attackDelay: 1872, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 6, luk: 30, level: 1, weaponATK: 7 },
    modes: {},
    spriteClass: 'poring', weaponMode: 0,
    drops: [
        { itemName: 'Jellopy', rate: 70 },
        { itemName: 'Knife', rate: 1 },
        { itemName: 'Sticky Mucus', rate: 4 },
        { itemName: 'Apple', rate: 10 },
        { itemName: 'Empty Bottle', rate: 15 },
        { itemName: 'Apple', rate: 1.5 },
        { itemName: 'Unripe Apple', rate: 0.2 },
        { itemName: 'Poring Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Red Plant (ID: 1078) ──── Level 1 | HP 10 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['red_plant'] = {
    id: 1078, name: 'Red Plant', aegisName: 'RED_PLANT',
    level: 1, maxHealth: 10, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Red Herb', rate: 55 },
        { itemName: 'Flower', rate: 10 },
        { itemName: 'Shoot', rate: 10 },
        { itemName: 'Stem', rate: 5 },
        { itemName: 'Pointed Scale', rate: 3 },
        { itemName: 'Fluff', rate: 5 },
        { itemName: 'Ment', rate: 0.5 },
        { itemName: 'Romantic Flower', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Blue Plant (ID: 1079) ──── Level 1 | HP 10 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['blue_plant'] = {
    id: 1079, name: 'Blue Plant', aegisName: 'BLUE_PLANT',
    level: 1, maxHealth: 10, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Blue Herb', rate: 55 },
        { itemName: 'Flower', rate: 10 },
        { itemName: 'Shoot', rate: 10 },
        { itemName: 'Stem', rate: 5 },
        { itemName: 'Pointed Scale', rate: 3 },
        { itemName: 'Mastela Fruit', rate: 0.5 },
        { itemName: 'Grape', rate: 10 },
        { itemName: 'Romantic Leaf', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Green Plant (ID: 1080) ──── Level 1 | HP 10 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['green_plant'] = {
    id: 1080, name: 'Green Plant', aegisName: 'GREEN_PLANT',
    level: 1, maxHealth: 10, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Green Herb', rate: 70 },
        { itemName: 'Flower', rate: 10 },
        { itemName: 'Bitter Herb', rate: 0.2 },
        { itemName: 'Stem', rate: 30 },
        { itemName: 'Pointed Scale', rate: 15 },
        { itemName: 'Aloe', rate: 0.5 },
        { itemName: 'Aloe Leaflet', rate: 0.5 },
        { itemName: 'Romantic Leaf', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Yellow Plant (ID: 1081) ──── Level 1 | HP 10 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['yellow_plant'] = {
    id: 1081, name: 'Yellow Plant', aegisName: 'YELLOW_PLANT',
    level: 1, maxHealth: 10, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Yellow Herb', rate: 55 },
        { itemName: 'Flower', rate: 10 },
        { itemName: 'Shoot', rate: 10 },
        { itemName: 'Stem', rate: 5 },
        { itemName: 'Pointed Scale', rate: 3 },
        { itemName: 'Singing Plant', rate: 0.05 },
        { itemName: 'Fluff', rate: 5 },
        { itemName: 'Romantic Flower', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── White Plant (ID: 1082) ──── Level 1 | HP 10 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['white_plant'] = {
    id: 1082, name: 'White Plant', aegisName: 'WHITE_PLANT',
    level: 1, maxHealth: 10, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'White Herb', rate: 55 },
        { itemName: 'Flower', rate: 10 },
        { itemName: 'Deadly Noxious Herb', rate: 0.2 },
        { itemName: 'Stem', rate: 30 },
        { itemName: 'Pointed Scale', rate: 15 },
        { itemName: 'Aloe Leaflet', rate: 0.5 },
        { itemName: 'Hinalle', rate: 0.5 },
        { itemName: 'Romantic Flower', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Shining Plant (ID: 1083) ──── Level 1 | HP 20 | NORMAL | plant/holy1 | passive
RO_MONSTER_TEMPLATES['shining_plant'] = {
    id: 1083, name: 'Shining Plant', aegisName: 'SHINING_PLANT',
    level: 1, maxHealth: 20, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 90,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'holy', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 90, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Blue Herb', rate: 55 },
        { itemName: 'Yellow Herb', rate: 10 },
        { itemName: 'White Herb', rate: 10 },
        { itemName: 'Illusion Flower', rate: 0.05 },
        { itemName: 'Yggdrasil Seed', rate: 0.2 },
        { itemName: 'Honey', rate: 5 },
        { itemName: 'Yggdrasil Berry', rate: 0.5 },
        { itemName: 'Emperium', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Black Mushroom (ID: 1084) ──── Level 1 | HP 15 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['black_mushroom'] = {
    id: 1084, name: 'Black Mushroom', aegisName: 'BLACK_MUSHROOM',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Alcohol', rate: 0.5 },
        { itemName: 'Detrimindexta', rate: 0.5 },
        { itemName: 'Dew Laden Moss', rate: 0.2 },
        { itemName: 'Feather', rate: 20 },
        { itemName: 'Crystal Blue', rate: 8 },
        { itemName: 'Mushroom Spore', rate: 55 },
        { itemName: 'Mushroom Spore', rate: 55 },
        { itemName: 'Poison Spore', rate: 55, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Red Mushroom (ID: 1085) ──── Level 1 | HP 15 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['red_mushroom'] = {
    id: 1085, name: 'Red Mushroom', aegisName: 'RED_MUSHROOM',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Alcohol', rate: 0.5 },
        { itemName: 'Karvodailnirol', rate: 0.5 },
        { itemName: 'Dew Laden Moss', rate: 0.2 },
        { itemName: 'Feather', rate: 20 },
        { itemName: 'Red Blood', rate: 10 },
        { itemName: 'Mushroom Spore', rate: 55 },
        { itemName: 'Mushroom Spore', rate: 55 },
        { itemName: 'Poison Spore', rate: 55, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thief Mushroom (ID: 1182) ──── Level 1 | HP 15 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['thief_mushroom'] = {
    id: 1182, name: 'Thief Mushroom', aegisName: 'THIEF_MUSHROOM',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 1, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 1, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Orange Net Mushroom', rate: 15 },
        { itemName: 'Orange Gooey Mushroom', rate: 30 },
    ],
    mvpDrops: [],
};

// ──── Fabre (ID: 1184) ──── Level 1 | HP 30 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['fabre_'] = {
    id: 1184, name: 'Fabre', aegisName: 'FABRE_',
    level: 1, maxHealth: 30, baseExp: 1, jobExp: 0, mvpExp: 0,
    attack: 4, attack2: 7, defense: 0, magicDefense: 0,
    str: 0, agi: 2, vit: 0, int: 0, dex: 4, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 400, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 5500,
    raceGroups: {},
    stats: { str: 0, agi: 2, vit: 0, int: 0, dex: 4, luk: 5, level: 1, weaponATK: 4 },
    modes: { detector: true },
    drops: [
        { itemName: 'Fluff', rate: 20 },
        { itemName: 'Feather', rate: 2.5 },
        { itemName: 'Club', rate: 0.8 },
        { itemName: 'Emerald', rate: 0.02 },
        { itemName: 'Green Herb', rate: 3.5 },
        { itemName: 'Clover', rate: 5 },
        { itemName: 'Club', rate: 2 },
    ],
    mvpDrops: [],
};

// ──── Wind Crystal (ID: 1395) ──── Level 1 | HP 15 | BOSS | formless/neutral1 | aggressive
RO_MONSTER_TEMPLATES['crystal_1'] = {
    id: 1395, name: 'Wind Crystal', aegisName: 'CRYSTAL_1',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 1, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 190, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0, level: 1, weaponATK: 0 },
    modes: {},
    drops: [
        { itemName: 'Piece of Cake', rate: 38 },
        { itemName: 'Candy Cane', rate: 45 },
        { itemName: 'White Chocolate', rate: 50 },
        { itemName: 'Gift Box', rate: 49 },
        { itemName: 'Holiday Hat', rate: 70 },
        { itemName: 'Banana Juice', rate: 65 },
        { itemName: 'Chocolate', rate: 50 },
        { itemName: 'Yggdrasil Berry', rate: 2, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Earth Crystal (ID: 1396) ──── Level 1 | HP 15 | BOSS | formless/neutral1 | aggressive
RO_MONSTER_TEMPLATES['crystal_2'] = {
    id: 1396, name: 'Earth Crystal', aegisName: 'CRYSTAL_2',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 1, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 190, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0, level: 1, weaponATK: 0 },
    modes: {},
    drops: [
        { itemName: 'Piece of Cake', rate: 38 },
        { itemName: 'Candy Cane', rate: 45 },
        { itemName: 'White Chocolate', rate: 50 },
        { itemName: 'Gift Box', rate: 49 },
        { itemName: 'Holiday Hat', rate: 70 },
        { itemName: 'Apple Juice', rate: 65 },
        { itemName: 'Chocolate', rate: 50 },
        { itemName: 'Yggdrasil Seed', rate: 2.5, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Fire Crystal (ID: 1397) ──── Level 1 | HP 15 | BOSS | formless/neutral1 | aggressive
RO_MONSTER_TEMPLATES['crystal_3'] = {
    id: 1397, name: 'Fire Crystal', aegisName: 'CRYSTAL_3',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 1, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 190, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0, level: 1, weaponATK: 0 },
    modes: {},
    drops: [
        { itemName: 'Piece of Cake', rate: 38 },
        { itemName: 'Candy Cane', rate: 45 },
        { itemName: 'White Chocolate', rate: 50 },
        { itemName: 'Gift Box', rate: 49 },
        { itemName: 'Holiday Hat', rate: 70 },
        { itemName: 'Carrot Juice', rate: 65 },
        { itemName: 'Chocolate', rate: 50 },
        { itemName: 'Dead Branch', rate: 3, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Water Crystal (ID: 1398) ──── Level 1 | HP 15 | BOSS | formless/neutral1 | aggressive
RO_MONSTER_TEMPLATES['crystal_4'] = {
    id: 1398, name: 'Water Crystal', aegisName: 'CRYSTAL_4',
    level: 1, maxHealth: 15, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 1, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 190, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 999, luk: 0, level: 1, weaponATK: 0 },
    modes: {},
    drops: [
        { itemName: 'Piece of Cake', rate: 38 },
        { itemName: 'Candy Cane', rate: 45 },
        { itemName: 'White Chocolate', rate: 50 },
        { itemName: 'Gift Box', rate: 49 },
        { itemName: 'Holiday Hat', rate: 70 },
        { itemName: 'Grape Juice', rate: 65 },
        { itemName: 'Chocolate', rate: 50 },
        { itemName: 'Old Blue Box', rate: 1, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Fabre (ID: 1007) ──── Level 2 | HP 63 | NORMAL | insect/earth1 | passive
RO_MONSTER_TEMPLATES['fabre'] = {
    id: 1007, name: 'Fabre', aegisName: 'FABRE',
    level: 2, maxHealth: 63, baseExp: 3, jobExp: 2, mvpExp: 0,
    attack: 8, attack2: 11, defense: 0, magicDefense: 0,
    str: 0, agi: 2, vit: 4, int: 0, dex: 7, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 400, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6000,
    raceGroups: {},
    stats: { str: 0, agi: 2, vit: 4, int: 0, dex: 7, luk: 5, level: 2, weaponATK: 8 },
    modes: { detector: true },
    drops: [
        { itemName: 'Fluff', rate: 65 },
        { itemName: 'Feather', rate: 5 },
        { itemName: 'Club', rate: 0.8 },
        { itemName: 'Emerald', rate: 0.05 },
        { itemName: 'Green Herb', rate: 7 },
        { itemName: 'Clover', rate: 10 },
        { itemName: 'Club', rate: 2 },
        { itemName: 'Fabre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pupa (ID: 1008) ──── Level 2 | HP 427 | NORMAL | insect/earth1 | passive
RO_MONSTER_TEMPLATES['pupa'] = {
    id: 1008, name: 'Pupa', aegisName: 'PUPA',
    level: 2, maxHealth: 427, baseExp: 2, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 0, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 2, weaponATK: 1 },
    modes: { detector: true },
    drops: [
        { itemName: 'Phracon', rate: 0.8 },
        { itemName: 'Chrysalis', rate: 55 },
        { itemName: 'Sticky Mucus', rate: 6 },
        { itemName: 'Guard', rate: 0.02 },
        { itemName: 'Shell', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 6 },
        { itemName: 'Iron Ore', rate: 2 },
        { itemName: 'Pupa Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mastering (ID: 1090) ──── Level 2 | HP 2,415 | BOSS | plant/water1 | aggressive
RO_MONSTER_TEMPLATES['mastering'] = {
    id: 1090, name: 'Mastering', aegisName: 'MASTERING',
    level: 2, maxHealth: 2415, baseExp: 30, jobExp: 10, mvpExp: 0,
    attack: 18, attack2: 24, defense: 0, magicDefense: 10,
    str: 0, agi: 2, vit: 2, int: 0, dex: 17, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 300, attackDelay: 1072, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'water', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 2, vit: 2, int: 0, dex: 17, luk: 60, level: 2, weaponATK: 18 },
    modes: {},
    drops: [
        { itemName: 'Unicorn Horn', rate: 2 },
        { itemName: 'Unripe Apple', rate: 0.5 },
        { itemName: 'Pearl', rate: 10 },
        { itemName: 'Angel\'s Safeguard', rate: 10 },
        { itemName: 'Apple', rate: 80 },
        { itemName: 'Apple', rate: 80 },
        { itemName: 'Apple Juice', rate: 40 },
        { itemName: 'Mastering Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Fabre (ID: 1229) ──── Level 2 | HP 63 | NORMAL | insect/earth1 | passive
RO_MONSTER_TEMPLATES['meta_fabre'] = {
    id: 1229, name: 'Fabre', aegisName: 'META_FABRE',
    level: 2, maxHealth: 63, baseExp: 3, jobExp: 2, mvpExp: 0,
    attack: 8, attack2: 11, defense: 0, magicDefense: 0,
    str: 0, agi: 2, vit: 4, int: 0, dex: 7, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 400, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6000,
    raceGroups: {},
    stats: { str: 0, agi: 2, vit: 4, int: 0, dex: 7, luk: 5, level: 2, weaponATK: 8 },
    modes: { detector: true },
    drops: [
        { itemName: 'Fluff', rate: 65 },
        { itemName: 'Feather', rate: 6 },
        { itemName: 'Club', rate: 0.8 },
        { itemName: 'Emerald', rate: 0.08 },
        { itemName: 'Green Herb', rate: 7.5 },
        { itemName: 'Clover', rate: 15 },
        { itemName: 'Club', rate: 2 },
        { itemName: 'Fabre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pupa (ID: 1230) ──── Level 2 | HP 427 | NORMAL | insect/earth1 | passive
RO_MONSTER_TEMPLATES['meta_pupa'] = {
    id: 1230, name: 'Pupa', aegisName: 'META_PUPA',
    level: 2, maxHealth: 427, baseExp: 2, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 20, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 2, weaponATK: 1 },
    modes: { detector: true },
    drops: [
        { itemName: 'Phracon', rate: 3 },
        { itemName: 'Chrysalis', rate: 60 },
        { itemName: 'Sticky Mucus', rate: 7 },
        { itemName: 'Guard', rate: 0.02 },
        { itemName: 'Shell', rate: 13 },
        { itemName: 'Sticky Mucus', rate: 7 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Pupa Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Peco Peco Egg (ID: 1047) ──── Level 3 | HP 420 | NORMAL | formless/neutral3 | passive
RO_MONSTER_TEMPLATES['pecopeco_egg'] = {
    id: 1047, name: 'Peco Peco Egg', aegisName: 'PECOPECO_EGG',
    level: 3, maxHealth: 420, baseExp: 4, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 20, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 3, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 2.5 },
        { itemName: 'Shell', rate: 15 },
        { itemName: 'Guard', rate: 0.02 },
        { itemName: 'Red Herb', rate: 4 },
        { itemName: 'Red Herb', rate: 4 },
        { itemName: 'Empty Bottle', rate: 18 },
        { itemName: 'China', rate: 0.1 },
        { itemName: 'Peco Peco Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Picky (ID: 1049) ──── Level 3 | HP 80 | NORMAL | brute/fire1 | passive
RO_MONSTER_TEMPLATES['picky'] = {
    id: 1049, name: 'Picky', aegisName: 'PICKY',
    level: 3, maxHealth: 80, baseExp: 4, jobExp: 3, mvpExp: 0,
    attack: 9, attack2: 12, defense: 0, magicDefense: 0,
    str: 0, agi: 3, vit: 3, int: 5, dex: 10, luk: 30,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 988, attackMotion: 288, damageMotion: 168,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 5, dex: 10, luk: 30, level: 3, weaponATK: 9 },
    modes: {},
    drops: [
        { itemName: 'Feather of Birds', rate: 90 },
        { itemName: 'Feather', rate: 7 },
        { itemName: 'Cotton Shirt', rate: 1.5 },
        { itemName: 'Red Herb', rate: 5.5 },
        { itemName: 'Milk', rate: 3 },
        { itemName: 'Yellow Gemstone', rate: 0.5 },
        { itemName: 'Picky Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Santa Poring (ID: 1062) ──── Level 3 | HP 69 | NORMAL | plant/holy1 | passive
RO_MONSTER_TEMPLATES['poring_'] = {
    id: 1062, name: 'Santa Poring', aegisName: 'PORING_',
    level: 3, maxHealth: 69, baseExp: 4, jobExp: 5, mvpExp: 0,
    attack: 12, attack2: 16, defense: 0, magicDefense: 0,
    str: 0, agi: 14, vit: 3, int: 10, dex: 12, luk: 90,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 400, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'holy', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 14, vit: 3, int: 10, dex: 12, luk: 90, level: 3, weaponATK: 12 },
    modes: {},
    drops: [
        { itemName: 'Candy', rate: 20 },
        { itemName: 'Candy Cane', rate: 10 },
        { itemName: 'Red Herb', rate: 10 },
        { itemName: 'Apple', rate: 10 },
        { itemName: 'Santa\'s Hat', rate: 1 },
        { itemName: 'Apple', rate: 0.07 },
        { itemName: 'Poring Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Lunatic (ID: 1063) ──── Level 3 | HP 60 | NORMAL | brute/neutral3 | passive
RO_MONSTER_TEMPLATES['lunatic'] = {
    id: 1063, name: 'Lunatic', aegisName: 'LUNATIC',
    level: 3, maxHealth: 60, baseExp: 6, jobExp: 2, mvpExp: 0,
    attack: 9, attack2: 12, defense: 0, magicDefense: 20,
    str: 0, agi: 3, vit: 3, int: 10, dex: 8, luk: 60,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 171, walkSpeed: 200, attackDelay: 1456, attackMotion: 456, damageMotion: 336,
    size: 'small', race: 'brute', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 10, dex: 8, luk: 60, level: 3, weaponATK: 9 },
    modes: {},
    drops: [
        { itemName: 'Clover', rate: 65 },
        { itemName: 'Feather', rate: 10 },
        { itemName: 'Clown Nose', rate: 0.04 },
        { itemName: 'Apple', rate: 20 },
        { itemName: 'Red Herb', rate: 6 },
        { itemName: 'Carrot', rate: 11 },
        { itemName: 'Rainbow Carrot', rate: 0.2 },
        { itemName: 'Lunatic Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Drops (ID: 1113) ──── Level 3 | HP 55 | NORMAL | plant/fire1 | passive
RO_MONSTER_TEMPLATES['drops'] = {
    id: 1113, name: 'Drops', aegisName: 'DROPS',
    level: 3, maxHealth: 55, baseExp: 4, jobExp: 3, mvpExp: 0,
    attack: 10, attack2: 13, defense: 0, magicDefense: 0,
    str: 0, agi: 3, vit: 3, int: 0, dex: 12, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 173, walkSpeed: 400, attackDelay: 1372, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 0, dex: 12, luk: 15, level: 3, weaponATK: 10 },
    modes: {},
    drops: [
        { itemName: 'Jellopy', rate: 75 },
        { itemName: 'Rod', rate: 0.8 },
        { itemName: 'Sticky Mucus', rate: 5 },
        { itemName: 'Apple', rate: 11 },
        { itemName: 'Empty Bottle', rate: 17 },
        { itemName: 'Apple', rate: 8 },
        { itemName: 'Orange Juice', rate: 0.2 },
        { itemName: 'Drops Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Peco Peco Egg (ID: 1232) ──── Level 3 | HP 420 | NORMAL | formless/neutral3 | passive
RO_MONSTER_TEMPLATES['meta_pecopeco_egg'] = {
    id: 1232, name: 'Peco Peco Egg', aegisName: 'META_PECOPECO_EGG',
    level: 3, maxHealth: 420, baseExp: 4, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 20, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 3, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 1.2 },
        { itemName: 'Shell', rate: 15 },
        { itemName: 'Guard', rate: 0.02 },
        { itemName: 'Red Herb', rate: 4.5 },
        { itemName: 'Red Herb', rate: 4.5 },
        { itemName: 'Empty Bottle', rate: 20 },
        { itemName: 'China', rate: 0.15 },
        { itemName: 'Peco Peco Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Picky (ID: 1240) ──── Level 3 | HP 80 | NORMAL | brute/fire1 | passive
RO_MONSTER_TEMPLATES['meta_picky'] = {
    id: 1240, name: 'Picky', aegisName: 'META_PICKY',
    level: 3, maxHealth: 80, baseExp: 4, jobExp: 3, mvpExp: 0,
    attack: 9, attack2: 12, defense: 0, magicDefense: 0,
    str: 0, agi: 3, vit: 3, int: 0, dex: 10, luk: 30,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 988, attackMotion: 288, damageMotion: 168,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 6500,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 0, dex: 10, luk: 30, level: 3, weaponATK: 9 },
    modes: {},
    drops: [
        { itemName: 'Feather of Birds', rate: 65 },
        { itemName: 'Feather', rate: 8.5 },
        { itemName: 'Cotton Shirt', rate: 1.5 },
        { itemName: 'Red Herb', rate: 6.5 },
        { itemName: 'Milk', rate: 3.5 },
        { itemName: 'Yellow Gemstone', rate: 0.6 },
        { itemName: 'Picky Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Willow (ID: 1010) ──── Level 4 | HP 95 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['wilow'] = {
    id: 1010, name: 'Willow', aegisName: 'WILOW',
    level: 4, maxHealth: 95, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 9, attack2: 12, defense: 5, magicDefense: 15,
    str: 0, agi: 4, vit: 8, int: 30, dex: 9, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 200, attackDelay: 1672, attackMotion: 672, damageMotion: 432,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 4, vit: 8, int: 30, dex: 9, luk: 10, level: 4, weaponATK: 9 },
    modes: {},
    drops: [
        { itemName: 'Tree Root', rate: 90 },
        { itemName: 'Wooden Block', rate: 1 },
        { itemName: 'Resin', rate: 15 },
        { itemName: 'Sweet Potato', rate: 7 },
        { itemName: 'Barren Trunk', rate: 35 },
        { itemName: 'Solid Trunk', rate: 20 },
        { itemName: 'Fine-grained Trunk', rate: 10 },
        { itemName: 'Willow Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Chonchon (ID: 1011) ──── Level 4 | HP 67 | NORMAL | insect/wind1 | passive
RO_MONSTER_TEMPLATES['chonchon'] = {
    id: 1011, name: 'Chonchon', aegisName: 'CHONCHON',
    level: 4, maxHealth: 67, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 10, attack2: 13, defense: 10, magicDefense: 0,
    str: 0, agi: 10, vit: 4, int: 5, dex: 12, luk: 2,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1076, attackMotion: 576, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 10, vit: 4, int: 5, dex: 12, luk: 2, level: 4, weaponATK: 10 },
    modes: { detector: true },
    drops: [
        { itemName: 'Iron', rate: 0.5 },
        { itemName: 'Shell', rate: 65 },
        { itemName: 'Jellopy', rate: 15 },
        { itemName: 'Cutter', rate: 0.55 },
        { itemName: 'Fly Wing', rate: 1 },
        { itemName: 'Chonchon Doll', rate: 0.05 },
        { itemName: 'Iron Ore', rate: 1.5 },
        { itemName: 'Chonchon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thief Bug Egg (ID: 1048) ──── Level 4 | HP 48 | NORMAL | insect/shadow1 | passive
RO_MONSTER_TEMPLATES['thief_bug_egg'] = {
    id: 1048, name: 'Thief Bug Egg', aegisName: 'THIEF_BUG_EGG',
    level: 4, maxHealth: 48, baseExp: 8, jobExp: 4, mvpExp: 0,
    attack: 13, attack2: 17, defense: 20, magicDefense: 0,
    str: 0, agi: 6, vit: 4, int: 0, dex: 14, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 186, walkSpeed: 1000, attackDelay: 701, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'insect', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 6, vit: 4, int: 0, dex: 14, luk: 20, level: 4, weaponATK: 13 },
    modes: { detector: true },
    drops: [
        { itemName: 'Phracon', rate: 3 },
        { itemName: 'Chrysalis', rate: 50 },
        { itemName: 'Guard', rate: 0.02 },
        { itemName: 'Sticky Mucus', rate: 6 },
        { itemName: 'Red Gemstone', rate: 1 },
        { itemName: 'Black Ladle', rate: 0.1 },
        { itemName: 'Iron Ore', rate: 2.5 },
        { itemName: 'Thief Bug Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Picky (ID: 1050) ──── Level 4 | HP 83 | NORMAL | brute/fire1 | passive
RO_MONSTER_TEMPLATES['picky_'] = {
    id: 1050, name: 'Picky', aegisName: 'PICKY_',
    level: 4, maxHealth: 83, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 8, attack2: 11, defense: 20, magicDefense: 0,
    str: 0, agi: 3, vit: 3, int: 10, dex: 11, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 988, attackMotion: 288, damageMotion: 168,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 10, dex: 11, luk: 20, level: 4, weaponATK: 8 },
    modes: {},
    drops: [
        { itemName: 'Feather of Birds', rate: 90 },
        { itemName: 'Feather', rate: 7 },
        { itemName: 'Egg Shell', rate: 0.1 },
        { itemName: 'Red Herb', rate: 6 },
        { itemName: 'Milk', rate: 3 },
        { itemName: 'Yellow Gemstone', rate: 0.5 },
        { itemName: 'Tiny Egg Shell', rate: 0.1 },
        { itemName: 'Picky Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ant Egg (ID: 1097) ──── Level 4 | HP 420 | NORMAL | formless/neutral3 | passive
RO_MONSTER_TEMPLATES['ant_egg'] = {
    id: 1097, name: 'Ant Egg', aegisName: 'ANT_EGG',
    level: 4, maxHealth: 420, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 20, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 4, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 3.2 },
        { itemName: 'Shell', rate: 20 },
        { itemName: 'Jellopy', rate: 20 },
        { itemName: 'Sticky Mucus', rate: 6.5 },
        { itemName: 'Empty Bottle', rate: 20 },
        { itemName: 'Iron Ore', rate: 2 },
        { itemName: 'Andre Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Chonchon (ID: 1183) ──── Level 4 | HP 67 | NORMAL | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['chonchon_'] = {
    id: 1183, name: 'Chonchon', aegisName: 'CHONCHON_',
    level: 4, maxHealth: 67, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 10, attack2: 13, defense: 10, magicDefense: 0,
    str: 0, agi: 10, vit: 4, int: 5, dex: 12, luk: 2,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1076, attackMotion: 576, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 10, vit: 4, int: 5, dex: 12, luk: 2, level: 4, weaponATK: 10 },
    modes: { detector: true },
    drops: [
        { itemName: 'Iron', rate: 0.5 },
        { itemName: 'Shell', rate: 55 },
        { itemName: 'Jellopy', rate: 15 },
        { itemName: 'Cutter', rate: 0.55 },
        { itemName: 'Fly Wing', rate: 1 },
        { itemName: 'Chonchon Doll', rate: 0.05 },
        { itemName: 'Chonchon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ant Egg (ID: 1236) ──── Level 4 | HP 420 | NORMAL | formless/neutral3 | passive
RO_MONSTER_TEMPLATES['meta_ant_egg'] = {
    id: 1236, name: 'Ant Egg', aegisName: 'META_ANT_EGG',
    level: 4, maxHealth: 420, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 1, attack2: 2, defense: 20, magicDefense: 20,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 1000, attackDelay: 1001, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 20, level: 4, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 1.35 },
        { itemName: 'Shell', rate: 27.4 },
        { itemName: 'Jellopy', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 7.5 },
        { itemName: 'Empty Bottle', rate: 20 },
        { itemName: 'Iron Ore', rate: 2.2 },
        { itemName: 'Andre Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Picky (ID: 1241) ──── Level 4 | HP 83 | NORMAL | brute/fire1 | passive
RO_MONSTER_TEMPLATES['meta_picky_'] = {
    id: 1241, name: 'Picky', aegisName: 'META_PICKY_',
    level: 4, maxHealth: 83, baseExp: 5, jobExp: 4, mvpExp: 0,
    attack: 8, attack2: 11, defense: 20, magicDefense: 0,
    str: 0, agi: 3, vit: 3, int: 0, dex: 11, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 988, attackMotion: 288, damageMotion: 168,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7000,
    raceGroups: {},
    stats: { str: 0, agi: 3, vit: 3, int: 0, dex: 11, luk: 20, level: 4, weaponATK: 8 },
    modes: {},
    drops: [
        { itemName: 'Feather of Birds', rate: 65 },
        { itemName: 'Feather', rate: 8.5 },
        { itemName: 'Egg Shell', rate: 0.07 },
        { itemName: 'Red Herb', rate: 7.5 },
        { itemName: 'Milk', rate: 3.5 },
        { itemName: 'Yellow Gemstone', rate: 0.6 },
        { itemName: 'Picky Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Condor (ID: 1009) ──── Level 5 | HP 92 | NORMAL | brute/wind1 | reactive
RO_MONSTER_TEMPLATES['condor'] = {
    id: 1009, name: 'Condor', aegisName: 'CONDOR',
    level: 5, maxHealth: 92, baseExp: 6, jobExp: 5, mvpExp: 0,
    attack: 11, attack2: 14, defense: 0, magicDefense: 0,
    str: 0, agi: 13, vit: 5, int: 0, dex: 13, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 150, attackDelay: 1148, attackMotion: 648, damageMotion: 480,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 7500,
    raceGroups: {},
    stats: { str: 0, agi: 13, vit: 5, int: 0, dex: 13, luk: 10, level: 5, weaponATK: 11 },
    modes: {},
    drops: [
        { itemName: 'Talon', rate: 90 },
        { itemName: 'Bow', rate: 1.5 },
        { itemName: 'Yellow Gemstone', rate: 0.8 },
        { itemName: 'Arrow', rate: 55 },
        { itemName: 'Meat', rate: 4 },
        { itemName: 'Feather of Birds', rate: 20 },
        { itemName: 'Orange', rate: 6 },
        { itemName: 'Condor Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Roda Frog (ID: 1012) ──── Level 5 | HP 133 | NORMAL | fish/water1 | passive
RO_MONSTER_TEMPLATES['roda_frog'] = {
    id: 1012, name: 'Roda Frog', aegisName: 'RODA_FROG',
    level: 5, maxHealth: 133, baseExp: 6, jobExp: 5, mvpExp: 0,
    attack: 11, attack2: 14, defense: 0, magicDefense: 5,
    str: 0, agi: 5, vit: 5, int: 5, dex: 10, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 160, walkSpeed: 200, attackDelay: 2016, attackMotion: 816, damageMotion: 288,
    size: 'medium', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 7500,
    raceGroups: {},
    stats: { str: 0, agi: 5, vit: 5, int: 5, dex: 10, luk: 5, level: 5, weaponATK: 11 },
    modes: {},
    drops: [
        { itemName: 'Sticky Webfoot', rate: 90 },
        { itemName: 'Spawn', rate: 5 },
        { itemName: 'Green Herb', rate: 3 },
        { itemName: 'Emerald', rate: 0.07 },
        { itemName: 'Empty Bottle', rate: 20 },
        { itemName: 'Roda Frog Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thief Bug (ID: 1051) ──── Level 6 | HP 126 | NORMAL | insect/neutral3 | aggressive
RO_MONSTER_TEMPLATES['thief_bug'] = {
    id: 1051, name: 'Thief Bug', aegisName: 'THIEF_BUG',
    level: 6, maxHealth: 126, baseExp: 17, jobExp: 5, mvpExp: 0,
    attack: 18, attack2: 24, defense: 5, magicDefense: 0,
    str: 0, agi: 6, vit: 6, int: 0, dex: 11, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1288, attackMotion: 288, damageMotion: 768,
    size: 'small', race: 'insect', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 8000,
    raceGroups: {},
    stats: { str: 0, agi: 6, vit: 6, int: 0, dex: 11, luk: 0, level: 6, weaponATK: 18 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 25 },
        { itemName: 'Jacket', rate: 0.8 },
        { itemName: 'Red Herb', rate: 3.5 },
        { itemName: 'Jellopy', rate: 20 },
        { itemName: 'Jacket', rate: 1.2 },
        { itemName: 'Iron Ore', rate: 2.5 },
        { itemName: 'Thief Bug Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Eclipse (ID: 1093) ──── Level 6 | HP 1,800 | BOSS | brute/neutral3 | aggressive
RO_MONSTER_TEMPLATES['eclipse'] = {
    id: 1093, name: 'Eclipse', aegisName: 'ECLIPSE',
    level: 6, maxHealth: 1800, baseExp: 60, jobExp: 55, mvpExp: 0,
    attack: 20, attack2: 26, defense: 0, magicDefense: 40,
    str: 0, agi: 36, vit: 6, int: 0, dex: 11, luk: 80,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 200, attackDelay: 1456, attackMotion: 456, damageMotion: 336,
    size: 'medium', race: 'brute', element: { type: 'neutral', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 36, vit: 6, int: 0, dex: 11, luk: 80, level: 6, weaponATK: 20 },
    modes: {},
    drops: [
        { itemName: 'Cute Ribbon', rate: 2 },
        { itemName: 'Red Herb', rate: 80 },
        { itemName: 'Opal', rate: 12 },
        { itemName: 'Glass Bead', rate: 15 },
        { itemName: 'Four Leaf Clover', rate: 0.3 },
        { itemName: 'Rainbow Carrot', rate: 0.5 },
        { itemName: 'Angel\'s Protection', rate: 10 },
        { itemName: 'Eclipse Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Savage Babe (ID: 1167) ──── Level 7 | HP 182 | NORMAL | brute/earth1 | passive
RO_MONSTER_TEMPLATES['savage_babe'] = {
    id: 1167, name: 'Savage Babe', aegisName: 'SAVAGE_BABE',
    level: 7, maxHealth: 182, baseExp: 14, jobExp: 12, mvpExp: 0,
    attack: 20, attack2: 25, defense: 0, magicDefense: 0,
    str: 0, agi: 7, vit: 14, int: 5, dex: 12, luk: 35,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 168, walkSpeed: 400, attackDelay: 1624, attackMotion: 624, damageMotion: 576,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 8500,
    raceGroups: {},
    stats: { str: 0, agi: 7, vit: 14, int: 5, dex: 12, luk: 35, level: 7, weaponATK: 20 },
    modes: {},
    drops: [
        { itemName: 'Animal\'s Skin', rate: 90 },
        { itemName: 'Axe', rate: 1 },
        { itemName: 'Meat', rate: 5 },
        { itemName: 'Arrow', rate: 10 },
        { itemName: 'Feather', rate: 8.5 },
        { itemName: 'Phracon', rate: 0.8 },
        { itemName: 'Sweet Milk', rate: 0.4 },
        { itemName: 'Savage Babe Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hornet (ID: 1004) ──── Level 8 | HP 169 | NORMAL | insect/wind1 | reactive
RO_MONSTER_TEMPLATES['hornet'] = {
    id: 1004, name: 'Hornet', aegisName: 'HORNET',
    level: 8, maxHealth: 169, baseExp: 19, jobExp: 15, mvpExp: 0,
    attack: 22, attack2: 27, defense: 5, magicDefense: 5,
    str: 6, agi: 20, vit: 8, int: 10, dex: 17, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1292, attackMotion: 792, damageMotion: 216,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 9000,
    raceGroups: {},
    stats: { str: 6, agi: 20, vit: 8, int: 10, dex: 17, luk: 5, level: 8, weaponATK: 22 },
    modes: { detector: true },
    drops: [
        { itemName: 'Wind of Verdure', rate: 0.8 },
        { itemName: 'Bee Sting', rate: 90 },
        { itemName: 'Jellopy', rate: 35 },
        { itemName: 'Main Gauche', rate: 0.15 },
        { itemName: 'Green Herb', rate: 3.5 },
        { itemName: 'Honey', rate: 1.5 },
        { itemName: 'Hornet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Familiar (ID: 1005) ──── Level 8 | HP 155 | NORMAL | brute/shadow1 | aggressive
RO_MONSTER_TEMPLATES['farmiliar'] = {
    id: 1005, name: 'Familiar', aegisName: 'FARMILIAR',
    level: 8, maxHealth: 155, baseExp: 28, jobExp: 15, mvpExp: 0,
    attack: 20, attack2: 28, defense: 0, magicDefense: 0,
    str: 0, agi: 12, vit: 8, int: 5, dex: 28, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1276, attackMotion: 576, damageMotion: 384,
    size: 'small', race: 'brute', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 9000,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 8, int: 5, dex: 28, luk: 0, level: 8, weaponATK: 20 },
    modes: {},
    drops: [
        { itemName: 'Tooth of Bat', rate: 55 },
        { itemName: 'Falchion', rate: 0.2 },
        { itemName: 'Ribbon', rate: 0.15 },
        { itemName: 'Fly Wing', rate: 0.5 },
        { itemName: 'Grape', rate: 1 },
        { itemName: 'Red Herb', rate: 7 },
        { itemName: 'Concentration Potion', rate: 0.5 },
        { itemName: 'Familiar Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dragon Fly (ID: 1091) ──── Level 8 | HP 2,400 | BOSS | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['dragon_fly'] = {
    id: 1091, name: 'Dragon Fly', aegisName: 'DRAGON_FLY',
    level: 8, maxHealth: 2400, baseExp: 88, jobExp: 44, mvpExp: 0,
    attack: 22, attack2: 27, defense: 40, magicDefense: 0,
    str: 0, agi: 20, vit: 8, int: 15, dex: 17, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 100, attackDelay: 1076, attackMotion: 576, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 8, int: 15, dex: 17, luk: 5, level: 8, weaponATK: 22 },
    modes: {},
    drops: [
        { itemName: 'Sweet Gent', rate: 2 },
        { itemName: 'Red Herb', rate: 80 },
        { itemName: 'Amethyst', rate: 15 },
        { itemName: 'Chonchon Doll', rate: 20 },
        { itemName: 'Clip', rate: 30 },
        { itemName: 'Rusty Iron', rate: 0.5 },
        { itemName: 'Grape Juice', rate: 30 },
        { itemName: 'Dragon Fly Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Rocker (ID: 1052) ──── Level 9 | HP 198 | NORMAL | insect/earth1 | passive
RO_MONSTER_TEMPLATES['rocker'] = {
    id: 1052, name: 'Rocker', aegisName: 'ROCKER',
    level: 9, maxHealth: 198, baseExp: 20, jobExp: 16, mvpExp: 0,
    attack: 24, attack2: 29, defense: 5, magicDefense: 10,
    str: 0, agi: 9, vit: 18, int: 10, dex: 14, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1864, attackMotion: 864, damageMotion: 540,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 9500,
    raceGroups: {},
    stats: { str: 0, agi: 9, vit: 18, int: 10, dex: 14, luk: 15, level: 9, weaponATK: 24 },
    modes: { detector: true },
    drops: [
        { itemName: 'Grasshopper\'s Leg', rate: 90 },
        { itemName: 'Green Acre Guitar', rate: 0.1 },
        { itemName: 'Green Feeler', rate: 0.04 },
        { itemName: 'Javelin', rate: 0.8 },
        { itemName: 'Hinalle Leaflet', rate: 0.1 },
        { itemName: 'Rocker Doll', rate: 0.1 },
        { itemName: 'Hinalle', rate: 0.1 },
        { itemName: 'Rocker Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baby Desert Wolf (ID: 1107) ──── Level 9 | HP 164 | NORMAL | brute/fire1 | reactive
RO_MONSTER_TEMPLATES['desert_wolf_b'] = {
    id: 1107, name: 'Baby Desert Wolf', aegisName: 'DESERT_WOLF_B',
    level: 9, maxHealth: 164, baseExp: 20, jobExp: 16, mvpExp: 0,
    attack: 30, attack2: 36, defense: 0, magicDefense: 0,
    str: 0, agi: 9, vit: 9, int: 5, dex: 21, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 300, attackDelay: 1600, attackMotion: 900, damageMotion: 240,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 9500,
    raceGroups: {},
    stats: { str: 0, agi: 9, vit: 9, int: 5, dex: 21, luk: 40, level: 9, weaponATK: 30 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 0.85 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Adventurere\'s Suit', rate: 0.8 },
        { itemName: 'Meat', rate: 6 },
        { itemName: 'Cotton Shirt', rate: 2 },
        { itemName: 'Asura', rate: 0.05 },
        { itemName: 'Orange', rate: 10 },
        { itemName: 'Baby Desert Wolf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thief Bug Female (ID: 1053) ──── Level 10 | HP 170 | NORMAL | insect/shadow1 | aggressive
RO_MONSTER_TEMPLATES['thief_bug_'] = {
    id: 1053, name: 'Thief Bug Female', aegisName: 'THIEF_BUG_',
    level: 10, maxHealth: 170, baseExp: 35, jobExp: 18, mvpExp: 0,
    attack: 33, attack2: 40, defense: 5, magicDefense: 5,
    str: 0, agi: 15, vit: 10, int: 5, dex: 23, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 988, attackMotion: 288, damageMotion: 768,
    size: 'medium', race: 'insect', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 10000,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 10, int: 5, dex: 23, luk: 5, level: 10, weaponATK: 33 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 35 },
        { itemName: 'Garlet', rate: 2.5 },
        { itemName: 'Blade', rate: 0.15 },
        { itemName: 'Insect Feeler', rate: 2 },
        { itemName: 'Red Herb', rate: 4 },
        { itemName: 'Red Gemstone', rate: 0.5 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Female Thief Bug Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skeleton (ID: 1076) ──── Level 10 | HP 234 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['skeleton'] = {
    id: 1076, name: 'Skeleton', aegisName: 'SKELETON',
    level: 10, maxHealth: 234, baseExp: 18, jobExp: 14, mvpExp: 0,
    attack: 39, attack2: 47, defense: 10, magicDefense: 10,
    str: 0, agi: 5, vit: 10, int: 0, dex: 12, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 155, walkSpeed: 200, attackDelay: 2228, attackMotion: 528, damageMotion: 576,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 10000,
    spriteClass: 'skeleton', weaponMode: 0,
    raceGroups: {},
    stats: { str: 0, agi: 5, vit: 10, int: 0, dex: 12, luk: 0, level: 10, weaponATK: 39 },
    modes: {},
    drops: [
        { itemName: 'Phracon', rate: 0.9 },
        { itemName: 'Skel-Bone', rate: 8 },
        { itemName: 'Mace', rate: 0.8 },
        { itemName: 'Jellopy', rate: 30 },
        { itemName: 'Red Herb', rate: 8.5 },
        { itemName: 'Skull Ring', rate: 0.3 },
        { itemName: 'Skeleton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Toad (ID: 1089) ──── Level 10 | HP 5,065 | BOSS | fish/water1 | aggressive
RO_MONSTER_TEMPLATES['toad'] = {
    id: 1089, name: 'Toad', aegisName: 'TOAD',
    level: 10, maxHealth: 5065, baseExp: 100, jobExp: 50, mvpExp: 0,
    attack: 26, attack2: 32, defense: 0, magicDefense: 0,
    str: 0, agi: 5, vit: 10, int: 10, dex: 10, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 200, attackDelay: 1236, attackMotion: 336, damageMotion: 432,
    size: 'medium', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 5, vit: 10, int: 10, dex: 10, luk: 25, level: 10, weaponATK: 26 },
    modes: {},
    drops: [
        { itemName: 'Big Sis\' Ribbon', rate: 0.5 },
        { itemName: 'Honey', rate: 20 },
        { itemName: 'Zircon', rate: 10 },
        { itemName: 'Glass Bead', rate: 15 },
        { itemName: 'Alcohol', rate: 1 },
        { itemName: 'Detrimindexta', rate: 1 },
        { itemName: 'Angel\'s Kiss', rate: 10 },
        { itemName: 'Toad Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plankton (ID: 1161) ──── Level 10 | HP 354 | NORMAL | plant/water3 | passive
RO_MONSTER_TEMPLATES['plankton'] = {
    id: 1161, name: 'Plankton', aegisName: 'PLANKTON',
    level: 10, maxHealth: 354, baseExp: 23, jobExp: 18, mvpExp: 0,
    attack: 26, attack2: 31, defense: 0, magicDefense: 5,
    str: 0, agi: 10, vit: 10, int: 0, dex: 15, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 156, walkSpeed: 400, attackDelay: 2208, attackMotion: 1008, damageMotion: 324,
    size: 'small', race: 'plant', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 10000,
    raceGroups: {},
    stats: { str: 0, agi: 10, vit: 10, int: 0, dex: 15, luk: 0, level: 10, weaponATK: 26 },
    modes: {},
    drops: [
        { itemName: 'Single Cell', rate: 90 },
        { itemName: 'Garlet', rate: 3 },
        { itemName: 'Sticky Mucus', rate: 7 },
        { itemName: 'Alcohol', rate: 0.04 },
        { itemName: 'Empty Bottle', rate: 10 },
        { itemName: 'Dew Laden Moss', rate: 0.2 },
        { itemName: 'Concentration Potion', rate: 0.5 },
        { itemName: 'Plankton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Antonio (ID: 1247) ──── Level 10 | HP 10 | NORMAL | plant/holy3 | passive
RO_MONSTER_TEMPLATES['antonio'] = {
    id: 1247, name: 'Antonio', aegisName: 'ANTONIO',
    level: 10, maxHealth: 10, baseExp: 3, jobExp: 2, mvpExp: 0,
    attack: 13, attack2: 20, defense: 100, magicDefense: 0,
    str: 0, agi: 0, vit: 0, int: 50, dex: 100, luk: 100,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 186, walkSpeed: 100, attackDelay: 720, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'plant', element: { type: 'holy', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 10000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 50, dex: 100, luk: 100, level: 10, weaponATK: 13 },
    modes: {},
    drops: [
        { itemName: 'Red Stocking', rate: 100 },
        { itemName: 'Gift Box', rate: 2 },
        { itemName: 'Well-baked Cookie', rate: 15 },
        { itemName: 'Piece of Cake', rate: 10 },
        { itemName: 'Candy', rate: 55 },
        { itemName: 'Candy Cane', rate: 55 },
        { itemName: 'Santa\'s Hat', rate: 2.5 },
        { itemName: 'Antonio Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kukre (ID: 1070) ──── Level 11 | HP 507 | NORMAL | fish/water1 | passive
RO_MONSTER_TEMPLATES['kukre'] = {
    id: 1070, name: 'Kukre', aegisName: 'KUKRE',
    level: 11, maxHealth: 507, baseExp: 38, jobExp: 28, mvpExp: 0,
    attack: 28, attack2: 37, defense: 15, magicDefense: 0,
    str: 0, agi: 11, vit: 11, int: 5, dex: 16, luk: 2,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 164, walkSpeed: 150, attackDelay: 1776, attackMotion: 576, damageMotion: 288,
    size: 'small', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 11300,
    raceGroups: {},
    stats: { str: 0, agi: 11, vit: 11, int: 5, dex: 16, luk: 2, level: 11, weaponATK: 28 },
    modes: {},
    drops: [
        { itemName: 'Crystal Blue', rate: 0.3 },
        { itemName: 'Worm Peeling', rate: 55 },
        { itemName: 'Garlet', rate: 4 },
        { itemName: 'Monster\'s Feed', rate: 5 },
        { itemName: 'Red Herb', rate: 6.5 },
        { itemName: 'Insect Feeler', rate: 4.5 },
        { itemName: 'Earthworm the Dude', rate: 0.2 },
        { itemName: 'Kukre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Tarou (ID: 1175) ──── Level 11 | HP 284 | NORMAL | brute/shadow1 | aggressive
RO_MONSTER_TEMPLATES['tarou'] = {
    id: 1175, name: 'Tarou', aegisName: 'TAROU',
    level: 11, maxHealth: 284, baseExp: 57, jobExp: 28, mvpExp: 0,
    attack: 34, attack2: 45, defense: 0, magicDefense: 0,
    str: 0, agi: 20, vit: 11, int: 10, dex: 24, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 150, attackDelay: 1744, attackMotion: 1044, damageMotion: 684,
    size: 'small', race: 'brute', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 11300,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 11, int: 10, dex: 24, luk: 5, level: 11, weaponATK: 34 },
    modes: {},
    drops: [
        { itemName: 'Rat Tail', rate: 90 },
        { itemName: 'Animal\'s Skin', rate: 30 },
        { itemName: 'Feather', rate: 8 },
        { itemName: 'Monster\'s Feed', rate: 10 },
        { itemName: 'Ora Ora', rate: 0.02 },
        { itemName: 'Tarou Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mandragora (ID: 1020) ──── Level 12 | HP 405 | NORMAL | plant/earth3 | passive
RO_MONSTER_TEMPLATES['mandragora'] = {
    id: 1020, name: 'Mandragora', aegisName: 'MANDRAGORA',
    level: 12, maxHealth: 405, baseExp: 45, jobExp: 32, mvpExp: 0,
    attack: 26, attack2: 35, defense: 0, magicDefense: 25,
    str: 0, agi: 12, vit: 24, int: 0, dex: 36, luk: 15,
    attackRange: 200, aggroRange: 0, chaseRange: 600,
    aspd: 165, walkSpeed: 1000, attackDelay: 1768, attackMotion: 768, damageMotion: 576,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 11600,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 24, int: 0, dex: 36, luk: 15, level: 12, weaponATK: 26 },
    modes: {},
    drops: [
        { itemName: 'Green Live', rate: 0.5 },
        { itemName: 'Stem', rate: 90 },
        { itemName: 'Spear', rate: 0.3 },
        { itemName: 'Green Herb', rate: 3.5 },
        { itemName: 'Shoot', rate: 3 },
        { itemName: 'Four Leaf Clover', rate: 0.03 },
        { itemName: 'Gaia Whip', rate: 0.1 },
        { itemName: 'Mandragora Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ambernite (ID: 1094) ──── Level 13 | HP 495 | NORMAL | insect/water1 | aggressive
RO_MONSTER_TEMPLATES['ambernite'] = {
    id: 1094, name: 'Ambernite', aegisName: 'AMBERNITE',
    level: 13, maxHealth: 495, baseExp: 57, jobExp: 38, mvpExp: 0,
    attack: 39, attack2: 46, defense: 30, magicDefense: 0,
    str: 0, agi: 13, vit: 13, int: 5, dex: 18, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 159, walkSpeed: 400, attackDelay: 2048, attackMotion: 648, damageMotion: 648,
    size: 'large', race: 'insect', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 11900,
    raceGroups: {},
    stats: { str: 0, agi: 13, vit: 13, int: 5, dex: 18, luk: 5, level: 13, weaponATK: 39 },
    modes: { detector: true },
    drops: [
        { itemName: 'Crystal Blue', rate: 0.5 },
        { itemName: 'Snail\'s Shell', rate: 90 },
        { itemName: 'Garlet', rate: 12 },
        { itemName: 'Shell', rate: 30 },
        { itemName: 'Solid Shell', rate: 0.02 },
        { itemName: 'Rough Elunium', rate: 0.14 },
        { itemName: 'Iron Ore', rate: 1.5 },
        { itemName: 'Ambernite Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wormtail (ID: 1024) ──── Level 14 | HP 426 | NORMAL | plant/earth1 | aggressive
RO_MONSTER_TEMPLATES['worm_tail'] = {
    id: 1024, name: 'Wormtail', aegisName: 'WORM_TAIL',
    level: 14, maxHealth: 426, baseExp: 59, jobExp: 40, mvpExp: 0,
    attack: 42, attack2: 51, defense: 5, magicDefense: 0,
    str: 0, agi: 14, vit: 28, int: 5, dex: 46, luk: 5,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1048, attackMotion: 48, damageMotion: 192,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 12200,
    raceGroups: {},
    stats: { str: 0, agi: 14, vit: 28, int: 5, dex: 46, luk: 5, level: 14, weaponATK: 42 },
    modes: {},
    drops: [
        { itemName: 'Green Live', rate: 0.6 },
        { itemName: 'Emveretarcon', rate: 0.25 },
        { itemName: 'Pointed Scale', rate: 55 },
        { itemName: 'Pike', rate: 0.3 },
        { itemName: 'Yellow Herb', rate: 0.7 },
        { itemName: 'Emerald', rate: 0.05 },
        { itemName: 'Green Lace', rate: 1 },
        { itemName: 'Wormtail Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Poporing (ID: 1031) ──── Level 14 | HP 344 | NORMAL | plant/poison1 | passive
RO_MONSTER_TEMPLATES['poporing'] = {
    id: 1031, name: 'Poporing', aegisName: 'POPORING',
    level: 14, maxHealth: 344, baseExp: 81, jobExp: 44, mvpExp: 0,
    attack: 59, attack2: 72, defense: 0, magicDefense: 10,
    str: 0, agi: 14, vit: 14, int: 0, dex: 19, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 300, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12200,
    raceGroups: {},
    stats: { str: 0, agi: 14, vit: 14, int: 0, dex: 19, luk: 15, level: 14, weaponATK: 59 },
    modes: {},
    drops: [
        { itemName: 'Sticky Mucus', rate: 55 },
        { itemName: 'Garlet', rate: 15 },
        { itemName: 'Green Herb', rate: 5 },
        { itemName: 'Grape', rate: 2 },
        { itemName: 'Apple', rate: 0.05 },
        { itemName: 'Main Gauche', rate: 0.05 },
        { itemName: 'Apple', rate: 2.5 },
        { itemName: 'Poporing Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hydra (ID: 1068) ──── Level 14 | HP 660 | NORMAL | plant/water2 | passive
RO_MONSTER_TEMPLATES['hydra'] = {
    id: 1068, name: 'Hydra', aegisName: 'HYDRA',
    level: 14, maxHealth: 660, baseExp: 59, jobExp: 40, mvpExp: 0,
    attack: 22, attack2: 28, defense: 0, magicDefense: 40,
    str: 0, agi: 14, vit: 14, int: 0, dex: 40, luk: 2,
    attackRange: 350, aggroRange: 0, chaseRange: 600,
    aspd: 184, walkSpeed: 1000, attackDelay: 800, attackMotion: 432, damageMotion: 600,
    size: 'small', race: 'plant', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12200,
    raceGroups: {},
    stats: { str: 0, agi: 14, vit: 14, int: 0, dex: 40, luk: 2, level: 14, weaponATK: 22 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.25 },
        { itemName: 'Tentacle', rate: 55 },
        { itemName: 'Sticky Mucus', rate: 15 },
        { itemName: 'Detrimindexta', rate: 0.2 },
        { itemName: 'Panacea', rate: 0.05 },
        { itemName: 'Meat', rate: 7 },
        { itemName: 'Hydra Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zombie (ID: 1015) ──── Level 15 | HP 534 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['zombie'] = {
    id: 1015, name: 'Zombie', aegisName: 'ZOMBIE',
    level: 15, maxHealth: 534, baseExp: 50, jobExp: 33, mvpExp: 0,
    attack: 67, attack2: 79, defense: 0, magicDefense: 10,
    str: 0, agi: 8, vit: 7, int: 0, dex: 15, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 148, walkSpeed: 400, attackDelay: 2612, attackMotion: 912, damageMotion: 288,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 12500,
    raceGroups: {},
    stats: { str: 0, agi: 8, vit: 7, int: 0, dex: 15, luk: 0, level: 15, weaponATK: 67 },
    modes: {},
    drops: [
        { itemName: 'Decayed Nail', rate: 90 },
        { itemName: 'Ruby', rate: 0.05 },
        { itemName: 'Sticky Mucus', rate: 10 },
        { itemName: 'Horrendous Mouth', rate: 0.5 },
        { itemName: 'Opal', rate: 0.7 },
        { itemName: 'Zombie Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Boa (ID: 1025) ──── Level 15 | HP 471 | NORMAL | brute/earth1 | passive
RO_MONSTER_TEMPLATES['snake'] = {
    id: 1025, name: 'Boa', aegisName: 'SNAKE',
    level: 15, maxHealth: 471, baseExp: 72, jobExp: 48, mvpExp: 0,
    attack: 46, attack2: 55, defense: 0, magicDefense: 0,
    str: 0, agi: 15, vit: 15, int: 10, dex: 35, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1576, attackMotion: 576, damageMotion: 576,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12500,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 15, int: 10, dex: 35, luk: 5, level: 15, weaponATK: 46 },
    modes: {},
    drops: [
        { itemName: 'Snake Scale', rate: 90 },
        { itemName: 'Katana', rate: 0.15 },
        { itemName: 'Red Herb', rate: 9 },
        { itemName: 'Emveretarcon', rate: 0.35 },
        { itemName: 'Venom Canine', rate: 8 },
        { itemName: 'Shining Scale', rate: 0.01 },
        { itemName: 'Strawberry', rate: 6 },
        { itemName: 'Snake Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Shellfish (ID: 1074) ──── Level 15 | HP 920 | NORMAL | fish/water1 | aggressive
RO_MONSTER_TEMPLATES['shellfish'] = {
    id: 1074, name: 'Shellfish', aegisName: 'SHELLFISH',
    level: 15, maxHealth: 920, baseExp: 66, jobExp: 44, mvpExp: 0,
    attack: 35, attack2: 42, defense: 35, magicDefense: 0,
    str: 0, agi: 12, vit: 8, int: 0, dex: 32, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 200, attackDelay: 864, attackMotion: 864, damageMotion: 384,
    size: 'small', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 12500,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 8, int: 0, dex: 32, luk: 5, level: 15, weaponATK: 35 },
    modes: {},
    drops: [
        { itemName: 'Clam Shell', rate: 55 },
        { itemName: 'Clam Flesh', rate: 10 },
        { itemName: 'Stone', rate: 5 },
        { itemName: 'Grit', rate: 10 },
        { itemName: 'Star Dust', rate: 0.1 },
        { itemName: 'Rough Elunium', rate: 0.18 },
        { itemName: 'Shell Fish Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marin (ID: 1242) ──── Level 15 | HP 742 | NORMAL | plant/water2 | passive
RO_MONSTER_TEMPLATES['marin'] = {
    id: 1242, name: 'Marin', aegisName: 'MARIN',
    level: 15, maxHealth: 742, baseExp: 66, jobExp: 44, mvpExp: 0,
    attack: 39, attack2: 43, defense: 0, magicDefense: 10,
    str: 0, agi: 10, vit: 10, int: 5, dex: 35, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 400, attackDelay: 1872, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12500,
    raceGroups: {},
    stats: { str: 0, agi: 10, vit: 10, int: 5, dex: 35, luk: 15, level: 15, weaponATK: 39 },
    modes: {},
    drops: [
        { itemName: 'Garlet', rate: 32 },
        { itemName: 'Sticky Mucus', rate: 15 },
        { itemName: 'Level 1 Frost Diver', rate: 1 },
        { itemName: 'Aquamarine', rate: 0.4 },
        { itemName: 'Blue Herb', rate: 0.75 },
        { itemName: 'Candy', rate: 3.5 },
        { itemName: 'Poring Hat', rate: 0.01 },
        { itemName: 'Marin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Boiled Rice (ID: 1520) ──── Level 15 | HP 400 | NORMAL | plant/water1 | passive
RO_MONSTER_TEMPLATES['boiled_rice'] = {
    id: 1520, name: 'Boiled Rice', aegisName: 'BOILED_RICE',
    level: 15, maxHealth: 400, baseExp: 84, jobExp: 45, mvpExp: 0,
    attack: 49, attack2: 82, defense: 0, magicDefense: 10,
    str: 0, agi: 14, vit: 14, int: 0, dex: 19, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 177, walkSpeed: 170, attackDelay: 1152, attackMotion: 672, damageMotion: 672,
    size: 'medium', race: 'plant', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12500,
    raceGroups: {},
    stats: { str: 0, agi: 14, vit: 14, int: 0, dex: 19, luk: 15, level: 15, weaponATK: 49 },
    modes: {},
    drops: [
        { itemName: 'Rice Ball', rate: 55 },
        { itemName: 'Rice Ball Doll', rate: 30 },
        { itemName: 'Soft Blade of Grass', rate: 10 },
        { itemName: 'Huge Leaf', rate: 10 },
    ],
    mvpDrops: [],
};

// ──── Spore (ID: 1014) ──── Level 16 | HP 510 | NORMAL | plant/water1 | passive
RO_MONSTER_TEMPLATES['spore'] = {
    id: 1014, name: 'Spore', aegisName: 'SPORE',
    level: 16, maxHealth: 510, baseExp: 66, jobExp: 108, mvpExp: 0,
    attack: 24, attack2: 48, defense: 0, magicDefense: 5,
    str: 0, agi: 12, vit: 12, int: 5, dex: 19, luk: 8,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1872, attackMotion: 672, damageMotion: 288,
    size: 'medium', race: 'plant', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12800,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 12, int: 5, dex: 19, luk: 8, level: 16, weaponATK: 24 },
    modes: {},
    drops: [
        { itemName: 'Mushroom Spore', rate: 90 },
        { itemName: 'Red Herb', rate: 8 },
        { itemName: 'Blue Herb', rate: 0.5 },
        { itemName: 'Spore Doll', rate: 0.1 },
        { itemName: 'Hat', rate: 0.4 },
        { itemName: 'Poison Spore', rate: 0.05 },
        { itemName: 'Strawberry', rate: 6 },
        { itemName: 'Spore Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Creamy (ID: 1018) ──── Level 16 | HP 595 | NORMAL | insect/wind1 | passive
RO_MONSTER_TEMPLATES['creamy'] = {
    id: 1018, name: 'Creamy', aegisName: 'CREAMY',
    level: 16, maxHealth: 595, baseExp: 105, jobExp: 70, mvpExp: 0,
    attack: 53, attack2: 64, defense: 0, magicDefense: 30,
    str: 0, agi: 40, vit: 16, int: 15, dex: 16, luk: 55,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 177, walkSpeed: 150, attackDelay: 1136, attackMotion: 720, damageMotion: 840,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12800,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 16, int: 15, dex: 16, luk: 55, level: 16, weaponATK: 53 },
    modes: { detector: true },
    drops: [
        { itemName: 'Powder of Butterfly', rate: 90 },
        { itemName: 'Silk Robe', rate: 0.1 },
        { itemName: 'Honey', rate: 1.5 },
        { itemName: 'Butterfly Wing', rate: 1 },
        { itemName: 'Fancy Flower', rate: 0.02 },
        { itemName: 'Flower', rate: 5 },
        { itemName: 'Level 3 Lightening Bolt', rate: 1 },
        { itemName: 'Creamy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Stainer (ID: 1174) ──── Level 16 | HP 538 | NORMAL | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['stainer'] = {
    id: 1174, name: 'Stainer', aegisName: 'STAINER',
    level: 16, maxHealth: 538, baseExp: 105, jobExp: 70, mvpExp: 0,
    attack: 53, attack2: 64, defense: 10, magicDefense: 0,
    str: 0, agi: 40, vit: 16, int: 5, dex: 30, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 200, attackDelay: 1688, attackMotion: 1188, damageMotion: 612,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 12800,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 16, int: 5, dex: 30, luk: 5, level: 16, weaponATK: 53 },
    modes: { detector: true },
    drops: [
        { itemName: 'Wind of Verdure', rate: 0.7 },
        { itemName: 'Emveretarcon', rate: 0.3 },
        { itemName: 'Rainbow Shell', rate: 90 },
        { itemName: 'Garlet', rate: 21 },
        { itemName: 'Rough Elunium', rate: 0.25 },
        { itemName: 'Solid Shell', rate: 0.1 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Stainer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Creamy (ID: 1231) ──── Level 16 | HP 595 | NORMAL | insect/wind1 | passive
RO_MONSTER_TEMPLATES['meta_creamy'] = {
    id: 1231, name: 'Creamy', aegisName: 'META_CREAMY',
    level: 16, maxHealth: 595, baseExp: 96, jobExp: 64, mvpExp: 0,
    attack: 53, attack2: 64, defense: 0, magicDefense: 30,
    str: 0, agi: 40, vit: 16, int: 15, dex: 16, luk: 55,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 176, walkSpeed: 200, attackDelay: 1220, attackMotion: 720, damageMotion: 288,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 12800,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 16, int: 15, dex: 16, luk: 55, level: 16, weaponATK: 53 },
    modes: { detector: true },
    drops: [
        { itemName: 'Powder of Butterfly', rate: 60 },
        { itemName: 'Silk Robe', rate: 0.1 },
        { itemName: 'Honey', rate: 1.8 },
        { itemName: 'Butterfly Wing', rate: 2 },
        { itemName: 'Fancy Flower', rate: 0.04 },
        { itemName: 'Flower', rate: 8 },
        { itemName: 'Creamy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Steel Chonchon (ID: 1042) ──── Level 17 | HP 530 | NORMAL | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['steel_chonchon'] = {
    id: 1042, name: 'Steel Chonchon', aegisName: 'STEEL_CHONCHON',
    level: 17, maxHealth: 530, baseExp: 109, jobExp: 71, mvpExp: 0,
    attack: 54, attack2: 65, defense: 15, magicDefense: 0,
    str: 0, agi: 43, vit: 17, int: 5, dex: 33, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 150, attackDelay: 1076, attackMotion: 576, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 0, agi: 43, vit: 17, int: 5, dex: 33, luk: 10, level: 17, weaponATK: 54 },
    modes: { detector: true },
    drops: [
        { itemName: 'Wind of Verdure', rate: 0.9 },
        { itemName: 'Steel', rate: 0.3 },
        { itemName: 'Garlet', rate: 24 },
        { itemName: 'Shell', rate: 90 },
        { itemName: 'Solid Shell', rate: 0.3 },
        { itemName: 'Iron', rate: 2 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Steel Chonchon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Muka (ID: 1055) ──── Level 17 | HP 610 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['muka'] = {
    id: 1055, name: 'Muka', aegisName: 'MUKA',
    level: 17, maxHealth: 610, baseExp: 273, jobExp: 120, mvpExp: 0,
    attack: 40, attack2: 49, defense: 5, magicDefense: 5,
    str: 15, agi: 15, vit: 30, int: 5, dex: 20, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 161, walkSpeed: 300, attackDelay: 1960, attackMotion: 960, damageMotion: 384,
    size: 'large', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 15, agi: 15, vit: 30, int: 5, dex: 20, luk: 10, level: 17, weaponATK: 40 },
    modes: {},
    drops: [
        { itemName: 'Green Live', rate: 0.7 },
        { itemName: 'Cactus Needle', rate: 90 },
        { itemName: 'Empty Bottle', rate: 20 },
        { itemName: 'Green Herb', rate: 4 },
        { itemName: 'Red Herb', rate: 10 },
        { itemName: 'Guisarme', rate: 0.5 },
        { itemName: 'Iron Ore', rate: 2.5 },
        { itemName: 'Muka Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Andre (ID: 1095) ──── Level 17 | HP 688 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['andre'] = {
    id: 1095, name: 'Andre', aegisName: 'ANDRE',
    level: 17, maxHealth: 688, baseExp: 109, jobExp: 71, mvpExp: 0,
    attack: 60, attack2: 71, defense: 10, magicDefense: 0,
    str: 0, agi: 17, vit: 24, int: 20, dex: 26, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 300, attackDelay: 1288, attackMotion: 288, damageMotion: 384,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 0, agi: 17, vit: 24, int: 20, dex: 26, luk: 20, level: 17, weaponATK: 60 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 90 },
        { itemName: 'Garlet', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 5 },
        { itemName: 'Green Live', rate: 0.5 },
        { itemName: 'Star Dust', rate: 0.04 },
        { itemName: 'Iron Ore', rate: 3.5 },
        { itemName: 'Rough Elunium', rate: 0.28 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Coco (ID: 1104) ──── Level 17 | HP 817 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['coco'] = {
    id: 1104, name: 'Coco', aegisName: 'COCO',
    level: 17, maxHealth: 817, baseExp: 120, jobExp: 78, mvpExp: 0,
    attack: 56, attack2: 67, defense: 0, magicDefense: 0,
    str: 24, agi: 17, vit: 34, int: 20, dex: 24, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 150, attackDelay: 1864, attackMotion: 864, damageMotion: 1008,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 24, agi: 17, vit: 34, int: 20, dex: 24, luk: 10, level: 17, weaponATK: 56 },
    modes: {},
    drops: [
        { itemName: 'Acorn', rate: 90 },
        { itemName: 'Hood', rate: 0.2 },
        { itemName: 'Fluff', rate: 30 },
        { itemName: 'Animal\'s Skin', rate: 25 },
        { itemName: 'Sweet Potato', rate: 5 },
        { itemName: 'Sandals', rate: 0.25 },
        { itemName: 'Strawberry', rate: 6 },
        { itemName: 'Coco Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Rafflesia (ID: 1162) ──── Level 17 | HP 1,333 | NORMAL | plant/earth1 | aggressive
RO_MONSTER_TEMPLATES['rafflesia'] = {
    id: 1162, name: 'Rafflesia', aegisName: 'RAFFLESIA',
    level: 17, maxHealth: 1333, baseExp: 333, jobExp: 333, mvpExp: 0,
    attack: 105, attack2: 127, defense: 0, magicDefense: 2,
    str: 0, agi: 18, vit: 24, int: 11, dex: 37, luk: 10,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 512, attackMotion: 528, damageMotion: 240,
    size: 'small', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 0, agi: 18, vit: 24, int: 11, dex: 37, luk: 10, level: 17, weaponATK: 105 },
    modes: {},
    drops: [
        { itemName: 'Maneater Root', rate: 55 },
        { itemName: 'Scell', rate: 16 },
        { itemName: 'Four Leaf Clover', rate: 0.02 },
        { itemName: 'Ment', rate: 0.1 },
        { itemName: 'Hinalle', rate: 0.1 },
        { itemName: 'Shoot', rate: 5.5 },
        { itemName: 'White Herb', rate: 0.3 },
        { itemName: 'Rafflesia Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Andre (ID: 1237) ──── Level 17 | HP 688 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['meta_andre'] = {
    id: 1237, name: 'Andre', aegisName: 'META_ANDRE',
    level: 17, maxHealth: 688, baseExp: 109, jobExp: 71, mvpExp: 0,
    attack: 60, attack2: 71, defense: 10, magicDefense: 0,
    str: 0, agi: 17, vit: 24, int: 20, dex: 26, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 300, attackDelay: 1288, attackMotion: 288, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13100,
    raceGroups: {},
    stats: { str: 0, agi: 17, vit: 24, int: 20, dex: 26, luk: 20, level: 17, weaponATK: 60 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 60 },
        { itemName: 'Garlet', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 10 },
        { itemName: 'Shell', rate: 30 },
        { itemName: 'Star Dust', rate: 0.06 },
        { itemName: 'Iron Ore', rate: 3.5 },
        { itemName: 'Rough Elunium', rate: 0.28 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Smokie (ID: 1056) ──── Level 18 | HP 641 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['smokie'] = {
    id: 1056, name: 'Smokie', aegisName: 'SMOKIE',
    level: 18, maxHealth: 641, baseExp: 134, jobExp: 86, mvpExp: 0,
    attack: 61, attack2: 72, defense: 0, magicDefense: 10,
    str: 0, agi: 18, vit: 36, int: 25, dex: 26, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1576, attackMotion: 576, damageMotion: 420,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 0, agi: 18, vit: 36, int: 25, dex: 26, luk: 35, level: 18, weaponATK: 61 },
    modes: {},
    drops: [
        { itemName: 'Raccoon Leaf', rate: 55 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Sweet Potato', rate: 8 },
        { itemName: 'Kitty Band', rate: 0.01 },
        { itemName: 'Raccoon Doll', rate: 0.02 },
        { itemName: 'Zargon', rate: 0.05 },
        { itemName: 'Zircon', rate: 0.02 },
        { itemName: 'Smokie Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Vocal (ID: 1088) ──── Level 18 | HP 3,016 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['vocal'] = {
    id: 1088, name: 'Vocal', aegisName: 'VOCAL',
    level: 18, maxHealth: 3016, baseExp: 110, jobExp: 88, mvpExp: 0,
    attack: 71, attack2: 82, defense: 10, magicDefense: 30,
    str: 77, agi: 28, vit: 26, int: 30, dex: 53, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1080, attackMotion: 648, damageMotion: 480,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 77, agi: 28, vit: 26, int: 30, dex: 53, luk: 40, level: 18, weaponATK: 71 },
    modes: { detector: true },
    drops: [
        { itemName: 'Oldman\'s Romance', rate: 0.5 },
        { itemName: 'Grasshopper\'s Leg', rate: 80 },
        { itemName: 'Emerald', rate: 10 },
        { itemName: 'Rocker Doll', rate: 15 },
        { itemName: 'Angel\'s Arrival', rate: 10 },
        { itemName: 'Concentration Potion', rate: 7 },
        { itemName: 'Gentle Breeze Guitar', rate: 0.1 },
        { itemName: 'Vocal Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ghostring (ID: 1120) ──── Level 18 | HP 73,300 | BOSS | demon/ghost4 | aggressive
RO_MONSTER_TEMPLATES['ghostring'] = {
    id: 1120, name: 'Ghostring', aegisName: 'GHOSTRING',
    level: 18, maxHealth: 73300, baseExp: 101, jobExp: 108, mvpExp: 0,
    attack: 82, attack2: 122, defense: 0, magicDefense: 60,
    str: 40, agi: 27, vit: 18, int: 45, dex: 72, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 176, walkSpeed: 300, attackDelay: 1220, attackMotion: 1080, damageMotion: 648,
    size: 'medium', race: 'demon', element: { type: 'ghost', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 40, agi: 27, vit: 18, int: 45, dex: 72, luk: 30, level: 18, weaponATK: 82 },
    modes: {},
    drops: [
        { itemName: 'Fabric', rate: 53.35 },
        { itemName: 'Ghost Bandana', rate: 1 },
        { itemName: 'Thief Clothes', rate: 0.5 },
        { itemName: 'Dead Branch', rate: 5 },
        { itemName: 'Old Blue Box', rate: 0.1 },
        { itemName: 'Emperium', rate: 0.3 },
        { itemName: 'Level 5 Soul Strike', rate: 1 },
        { itemName: 'Ghostring Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Horn (ID: 1128) ──── Level 18 | HP 659 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['horn'] = {
    id: 1128, name: 'Horn', aegisName: 'HORN',
    level: 18, maxHealth: 659, baseExp: 134, jobExp: 86, mvpExp: 0,
    attack: 58, attack2: 69, defense: 10, magicDefense: 0,
    str: 22, agi: 18, vit: 28, int: 10, dex: 47, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 200, attackDelay: 1528, attackMotion: 528, damageMotion: 288,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 22, agi: 18, vit: 28, int: 10, dex: 47, luk: 15, level: 18, weaponATK: 58 },
    modes: { detector: true },
    drops: [
        { itemName: 'Green Live', rate: 0.8 },
        { itemName: 'Emveretarcon', rate: 0.35 },
        { itemName: 'Horn', rate: 55 },
        { itemName: 'Guisarme', rate: 0.15 },
        { itemName: 'Shell', rate: 55 },
        { itemName: 'Solid Shell', rate: 0.7 },
        { itemName: 'Horn Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Martin (ID: 1145) ──── Level 18 | HP 1,109 | NORMAL | brute/earth2 | passive
RO_MONSTER_TEMPLATES['martin'] = {
    id: 1145, name: 'Martin', aegisName: 'MARTIN',
    level: 18, maxHealth: 1109, baseExp: 134, jobExp: 86, mvpExp: 0,
    attack: 52, attack2: 63, defense: 0, magicDefense: 5,
    str: 12, agi: 18, vit: 30, int: 15, dex: 15, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 300, attackDelay: 1480, attackMotion: 480, damageMotion: 480,
    size: 'small', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 12, agi: 18, vit: 30, int: 15, dex: 15, luk: 5, level: 18, weaponATK: 52 },
    modes: {},
    drops: [
        { itemName: 'Mole Whiskers', rate: 90 },
        { itemName: 'Mole Claw', rate: 5 },
        { itemName: 'Jur', rate: 0.1 },
        { itemName: 'Goggles', rate: 0.05 },
        { itemName: 'Safety Helmet', rate: 0.01 },
        { itemName: 'Battered Pot', rate: 0.1 },
        { itemName: 'Goggles', rate: 0.15 },
        { itemName: 'Martin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Piere (ID: 1160) ──── Level 18 | HP 733 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['piere'] = {
    id: 1160, name: 'Piere', aegisName: 'PIERE',
    level: 18, maxHealth: 733, baseExp: 122, jobExp: 78, mvpExp: 0,
    attack: 64, attack2: 75, defense: 15, magicDefense: 0,
    str: 0, agi: 18, vit: 26, int: 20, dex: 27, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 200, attackDelay: 1288, attackMotion: 288, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 0, agi: 18, vit: 26, int: 20, dex: 27, luk: 15, level: 18, weaponATK: 64 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 90 },
        { itemName: 'Garlet', rate: 11 },
        { itemName: 'Sticky Mucus', rate: 6 },
        { itemName: 'Wind of Verdure', rate: 0.3 },
        { itemName: 'Star Dust', rate: 0.05 },
        { itemName: 'Iron Ore', rate: 4 },
        { itemName: 'Rough Elunium', rate: 0.31 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Piere (ID: 1238) ──── Level 18 | HP 733 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['meta_piere'] = {
    id: 1238, name: 'Piere', aegisName: 'META_PIERE',
    level: 18, maxHealth: 733, baseExp: 122, jobExp: 78, mvpExp: 0,
    attack: 64, attack2: 75, defense: 15, magicDefense: 0,
    str: 0, agi: 18, vit: 26, int: 20, dex: 27, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 200, attackDelay: 1288, attackMotion: 288, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 0, agi: 18, vit: 26, int: 20, dex: 27, luk: 15, level: 18, weaponATK: 64 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 57 },
        { itemName: 'Garlet', rate: 11 },
        { itemName: 'Sticky Mucus', rate: 6 },
        { itemName: 'Wind of Verdure', rate: 0.15 },
        { itemName: 'Star Dust', rate: 0.05 },
        { itemName: 'Iron Ore', rate: 4 },
        { itemName: 'Rough Elunium', rate: 0.31 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Aster (ID: 1266) ──── Level 18 | HP 1,372 | NORMAL | fish/earth1 | aggressive
RO_MONSTER_TEMPLATES['aster'] = {
    id: 1266, name: 'Aster', aegisName: 'ASTER',
    level: 18, maxHealth: 1372, baseExp: 122, jobExp: 78, mvpExp: 0,
    attack: 56, attack2: 64, defense: 0, magicDefense: 10,
    str: 0, agi: 19, vit: 15, int: 0, dex: 34, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 400, attackDelay: 1264, attackMotion: 864, damageMotion: 216,
    size: 'small', race: 'fish', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13400,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 15, int: 0, dex: 34, luk: 5, level: 18, weaponATK: 56 },
    modes: {},
    drops: [
        { itemName: 'Sticky Mucus', rate: 5 },
        { itemName: 'Coral Reef', rate: 0.4 },
        { itemName: 'Single Cell', rate: 12 },
        { itemName: 'Yellow Herb', rate: 2 },
        { itemName: 'Zargon', rate: 0.6 },
        { itemName: 'Apple', rate: 1 },
        { itemName: 'Aster Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Peco Peco (ID: 1019) ──── Level 19 | HP 531 | NORMAL | brute/fire1 | reactive
RO_MONSTER_TEMPLATES['pecopeco'] = {
    id: 1019, name: 'Peco Peco', aegisName: 'PECOPECO',
    level: 19, maxHealth: 531, baseExp: 159, jobExp: 72, mvpExp: 0,
    attack: 50, attack2: 64, defense: 0, magicDefense: 0,
    str: 0, agi: 13, vit: 13, int: 25, dex: 27, luk: 9,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 200, attackDelay: 1564, attackMotion: 864, damageMotion: 576,
    size: 'large', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 13, vit: 13, int: 25, dex: 27, luk: 9, level: 19, weaponATK: 50 },
    modes: {},
    drops: [
        { itemName: 'Bill of Birds', rate: 90 },
        { itemName: 'Sandals', rate: 0.2 },
        { itemName: 'Yellow Herb', rate: 2 },
        { itemName: 'Red Herb', rate: 9 },
        { itemName: 'Wand', rate: 1 },
        { itemName: 'Orange', rate: 10 },
        { itemName: 'Peco Peco Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thief Bug Male (ID: 1054) ──── Level 19 | HP 583 | NORMAL | insect/shadow1 | aggressive
RO_MONSTER_TEMPLATES['thief_bug__'] = {
    id: 1054, name: 'Thief Bug Male', aegisName: 'THIEF_BUG__',
    level: 19, maxHealth: 583, baseExp: 223, jobExp: 93, mvpExp: 0,
    attack: 76, attack2: 88, defense: 15, magicDefense: 5,
    str: 0, agi: 29, vit: 16, int: 5, dex: 36, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 300, attackDelay: 988, attackMotion: 288, damageMotion: 768,
    size: 'medium', race: 'insect', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 29, vit: 16, int: 5, dex: 36, luk: 0, level: 19, weaponATK: 76 },
    modes: { detector: true },
    drops: [
        { itemName: 'Emveretarcon', rate: 0.4 },
        { itemName: 'Insect Feeler', rate: 55 },
        { itemName: 'Worm Peeling', rate: 15 },
        { itemName: 'Slayer', rate: 0.1 },
        { itemName: 'Yellow Herb', rate: 0.9 },
        { itemName: 'Zircon', rate: 0.05 },
        { itemName: 'Katana', rate: 0.5 },
        { itemName: 'Male Thief Bug Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Vadon (ID: 1066) ──── Level 19 | HP 1,017 | NORMAL | fish/water1 | aggressive
RO_MONSTER_TEMPLATES['vadon'] = {
    id: 1066, name: 'Vadon', aegisName: 'VADON',
    level: 19, maxHealth: 1017, baseExp: 135, jobExp: 85, mvpExp: 0,
    attack: 74, attack2: 85, defense: 20, magicDefense: 0,
    str: 0, agi: 19, vit: 16, int: 10, dex: 36, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 300, attackDelay: 1632, attackMotion: 432, damageMotion: 540,
    size: 'small', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 16, int: 10, dex: 36, luk: 15, level: 19, weaponATK: 74 },
    modes: {},
    drops: [
        { itemName: 'Crystal Blue', rate: 0.4 },
        { itemName: 'Nipper', rate: 90 },
        { itemName: 'Garlet', rate: 30 },
        { itemName: 'Padded Armor', rate: 0.05 },
        { itemName: 'Solid Shell', rate: 1 },
        { itemName: 'Rough Elunium', rate: 0.4 },
        { itemName: 'Blue Gemstone', rate: 0.5 },
        { itemName: 'Vadon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Poison Spore (ID: 1077) ──── Level 19 | HP 665 | NORMAL | plant/poison1 | aggressive
RO_MONSTER_TEMPLATES['poison_spore'] = {
    id: 1077, name: 'Poison Spore', aegisName: 'POISON_SPORE',
    level: 19, maxHealth: 665, baseExp: 186, jobExp: 93, mvpExp: 0,
    attack: 89, attack2: 101, defense: 0, magicDefense: 0,
    str: 0, agi: 19, vit: 25, int: 0, dex: 24, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 200, attackDelay: 1672, attackMotion: 672, damageMotion: 288,
    size: 'medium', race: 'plant', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 25, int: 0, dex: 24, luk: 0, level: 19, weaponATK: 89 },
    modes: {},
    drops: [
        { itemName: 'Poison Spore', rate: 90 },
        { itemName: 'Hat', rate: 0.2 },
        { itemName: 'Green Herb', rate: 5.5 },
        { itemName: 'Blue Herb', rate: 0.6 },
        { itemName: 'Karvodailnirol', rate: 0.5 },
        { itemName: 'Mushroom Spore', rate: 12 },
        { itemName: 'Zargon', rate: 0.05 },
        { itemName: 'Poison Spore Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deniro (ID: 1105) ──── Level 19 | HP 760 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['deniro'] = {
    id: 1105, name: 'Deniro', aegisName: 'DENIRO',
    level: 19, maxHealth: 760, baseExp: 135, jobExp: 85, mvpExp: 0,
    attack: 68, attack2: 79, defense: 15, magicDefense: 0,
    str: 0, agi: 19, vit: 30, int: 20, dex: 43, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1288, attackMotion: 288, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 30, int: 20, dex: 43, luk: 10, level: 19, weaponATK: 68 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 90 },
        { itemName: 'Garlet', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 12 },
        { itemName: 'Red Blood', rate: 0.5 },
        { itemName: 'Star Dust', rate: 0.08 },
        { itemName: 'Iron Ore', rate: 4.5 },
        { itemName: 'Rough Elunium', rate: 0.34 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Yoyo (ID: 1234) ──── Level 19 | HP 879 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['provoke_yoyo'] = {
    id: 1234, name: 'Yoyo', aegisName: 'PROVOKE_YOYO',
    level: 19, maxHealth: 879, baseExp: 135, jobExp: 85, mvpExp: 0,
    attack: 71, attack2: 82, defense: 0, magicDefense: 0,
    str: 0, agi: 24, vit: 30, int: 35, dex: 32, luk: 55,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1054, attackMotion: 54, damageMotion: 384,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 30, int: 35, dex: 32, luk: 55, level: 19, weaponATK: 71 },
    modes: {},
    drops: [
        { itemName: 'Yoyo Tail', rate: 60 },
        { itemName: 'Cacao', rate: 5 },
        { itemName: 'Yellow Herb', rate: 1.3 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Yoyo Doll', rate: 0.07 },
        { itemName: 'Strawberry', rate: 5 },
        { itemName: 'Orange', rate: 10 },
        { itemName: 'Yoyo Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deniro (ID: 1239) ──── Level 19 | HP 760 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['meta_deniro'] = {
    id: 1239, name: 'Deniro', aegisName: 'META_DENIRO',
    level: 19, maxHealth: 760, baseExp: 135, jobExp: 85, mvpExp: 0,
    attack: 68, attack2: 79, defense: 15, magicDefense: 0,
    str: 0, agi: 19, vit: 30, int: 20, dex: 43, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1288, attackMotion: 288, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 13700,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 30, int: 20, dex: 43, luk: 10, level: 19, weaponATK: 68 },
    modes: { detector: true },
    drops: [
        { itemName: 'Worm Peeling', rate: 60 },
        { itemName: 'Garlet', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 12 },
        { itemName: 'Red Blood', rate: 0.45 },
        { itemName: 'Star Dust', rate: 0.08 },
        { itemName: 'Iron Ore', rate: 4.5 },
        { itemName: 'Rough Elunium', rate: 0.34 },
        { itemName: 'Andre Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Elder Willow (ID: 1033) ──── Level 20 | HP 693 | NORMAL | plant/fire2 | aggressive
RO_MONSTER_TEMPLATES['elder_wilow'] = {
    id: 1033, name: 'Elder Willow', aegisName: 'ELDER_WILOW',
    level: 20, maxHealth: 693, baseExp: 163, jobExp: 101, mvpExp: 0,
    attack: 58, attack2: 70, defense: 10, magicDefense: 30,
    str: 0, agi: 20, vit: 25, int: 35, dex: 38, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 200, attackDelay: 1372, attackMotion: 672, damageMotion: 432,
    size: 'medium', race: 'plant', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 25, int: 35, dex: 38, luk: 30, level: 20, weaponATK: 58 },
    modes: {},
    drops: [
        { itemName: 'Red Blood', rate: 0.5 },
        { itemName: 'Resin', rate: 90 },
        { itemName: 'Wooden Block', rate: 3.5 },
        { itemName: 'Rough Elunium', rate: 0.4 },
        { itemName: 'Wooden Mail', rate: 0.3 },
        { itemName: 'Level 3 Fire Bolt', rate: 1 },
        { itemName: 'Dead Branch', rate: 1 },
        { itemName: 'Elder Willow Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Crab (ID: 1073) ──── Level 20 | HP 2,451 | NORMAL | fish/water1 | passive
RO_MONSTER_TEMPLATES['crab'] = {
    id: 1073, name: 'Crab', aegisName: 'CRAB',
    level: 20, maxHealth: 2451, baseExp: 163, jobExp: 101, mvpExp: 0,
    attack: 71, attack2: 81, defense: 35, magicDefense: 0,
    str: 18, agi: 20, vit: 15, int: 0, dex: 36, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 992, attackMotion: 792, damageMotion: 360,
    size: 'small', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14000,
    raceGroups: {},
    stats: { str: 18, agi: 20, vit: 15, int: 0, dex: 36, luk: 15, level: 20, weaponATK: 71 },
    modes: {},
    drops: [
        { itemName: 'Crab Shell', rate: 55 },
        { itemName: 'Nipper', rate: 15 },
        { itemName: 'Stone', rate: 7 },
        { itemName: 'Star Dust', rate: 0.13 },
        { itemName: 'Rough Elunium', rate: 0.37 },
        { itemName: 'Crab Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Angeling (ID: 1096) ──── Level 20 | HP 55,000 | BOSS | angel/holy4 | aggressive
RO_MONSTER_TEMPLATES['angeling'] = {
    id: 1096, name: 'Angeling', aegisName: 'ANGELING',
    level: 20, maxHealth: 55000, baseExp: 163, jobExp: 144, mvpExp: 0,
    attack: 120, attack2: 195, defense: 0, magicDefense: 70,
    str: 0, agi: 50, vit: 20, int: 75, dex: 68, luk: 200,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1072, attackMotion: 672, damageMotion: 672,
    size: 'medium', race: 'angel', element: { type: 'holy', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 50, vit: 20, int: 75, dex: 68, luk: 200, level: 20, weaponATK: 120 },
    modes: {},
    drops: [
        { itemName: 'Angel Wing', rate: 1 },
        { itemName: 'Scapulare', rate: 0.6 },
        { itemName: 'Yggdrasil Leaf', rate: 5 },
        { itemName: 'Halo', rate: 0.01 },
        { itemName: 'White Herb', rate: 20 },
        { itemName: 'Apple', rate: 0.28 },
        { itemName: 'Emperium', rate: 0.4 },
        { itemName: 'Angeling Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Vitata (ID: 1176) ──── Level 20 | HP 894 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['vitata'] = {
    id: 1176, name: 'Vitata', aegisName: 'VITATA',
    level: 20, maxHealth: 894, baseExp: 163, jobExp: 101, mvpExp: 0,
    attack: 69, attack2: 80, defense: 15, magicDefense: 20,
    str: 0, agi: 20, vit: 25, int: 65, dex: 40, luk: 70,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 300, attackDelay: 1768, attackMotion: 768, damageMotion: 384,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 25, int: 65, dex: 40, luk: 70, level: 20, weaponATK: 69 },
    modes: { detector: true },
    drops: [
        { itemName: 'Green Live', rate: 0.9 },
        { itemName: 'Worm Peeling', rate: 50 },
        { itemName: 'Scell', rate: 2 },
        { itemName: 'Honey', rate: 3.5 },
        { itemName: 'Honey', rate: 3.5 },
        { itemName: 'Royal Jelly', rate: 2 },
        { itemName: 'Rough Oridecon', rate: 0.26 },
        { itemName: 'Vitata Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Yoyo (ID: 1057) ──── Level 21 | HP 879 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['yoyo'] = {
    id: 1057, name: 'Yoyo', aegisName: 'YOYO',
    level: 21, maxHealth: 879, baseExp: 280, jobExp: 111, mvpExp: 0,
    attack: 71, attack2: 82, defense: 0, magicDefense: 0,
    str: 0, agi: 24, vit: 30, int: 35, dex: 32, luk: 55,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1054, attackMotion: 54, damageMotion: 384,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14300,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 30, int: 35, dex: 32, luk: 55, level: 21, weaponATK: 71 },
    modes: {},
    drops: [
        { itemName: 'Yoyo Tail', rate: 90 },
        { itemName: 'Banana', rate: 15 },
        { itemName: 'Yellow Herb', rate: 2 },
        { itemName: 'Cacao', rate: 9 },
        { itemName: 'Yoyo Doll', rate: 0.1 },
        { itemName: 'Rough Oridecon', rate: 0.24 },
        { itemName: 'Strawberry', rate: 10 },
        { itemName: 'Yoyo Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dustiness (ID: 1114) ──── Level 21 | HP 1,044 | NORMAL | insect/wind2 | aggressive
RO_MONSTER_TEMPLATES['dustiness'] = {
    id: 1114, name: 'Dustiness', aegisName: 'DUSTINESS',
    level: 21, maxHealth: 1044, baseExp: 218, jobExp: 140, mvpExp: 0,
    attack: 80, attack2: 102, defense: 0, magicDefense: 10,
    str: 0, agi: 53, vit: 17, int: 0, dex: 38, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 150, attackDelay: 1004, attackMotion: 504, damageMotion: 384,
    size: 'small', race: 'insect', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14300,
    raceGroups: {},
    stats: { str: 0, agi: 53, vit: 17, int: 0, dex: 38, luk: 5, level: 21, weaponATK: 80 },
    modes: { detector: true },
    drops: [
        { itemName: 'Moth Dust', rate: 90 },
        { itemName: 'Moth Wings', rate: 5 },
        { itemName: 'Masquerade', rate: 0.04 },
        { itemName: 'Insect Feeler', rate: 20 },
        { itemName: 'Star Dust', rate: 0.1 },
        { itemName: 'Red Herb', rate: 12 },
        { itemName: 'Dustiness Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marina (ID: 1141) ──── Level 21 | HP 2,087 | NORMAL | plant/water2 | passive
RO_MONSTER_TEMPLATES['marina'] = {
    id: 1141, name: 'Marina', aegisName: 'MARINA',
    level: 21, maxHealth: 2087, baseExp: 218, jobExp: 140, mvpExp: 0,
    attack: 84, attack2: 106, defense: 0, magicDefense: 5,
    str: 0, agi: 21, vit: 21, int: 0, dex: 36, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 154, walkSpeed: 400, attackDelay: 2280, attackMotion: 1080, damageMotion: 864,
    size: 'small', race: 'plant', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14300,
    raceGroups: {},
    stats: { str: 0, agi: 21, vit: 21, int: 0, dex: 36, luk: 10, level: 21, weaponATK: 84 },
    modes: {},
    drops: [
        { itemName: 'Single Cell', rate: 50 },
        { itemName: 'Sticky Mucus', rate: 15 },
        { itemName: 'Crystal Blue', rate: 0.45 },
        { itemName: 'Mystic Frozen', rate: 0.02 },
        { itemName: 'Blue Gemstone', rate: 2 },
        { itemName: 'Deadly Noxious Herb', rate: 0.2 },
        { itemName: 'Marina Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Raggler (ID: 1254) ──── Level 21 | HP 1,020 | NORMAL | brute/wind1 | aggressive
RO_MONSTER_TEMPLATES['raggler'] = {
    id: 1254, name: 'Raggler', aegisName: 'RAGGLER',
    level: 21, maxHealth: 1020, baseExp: 218, jobExp: 140, mvpExp: 0,
    attack: 102, attack2: 113, defense: 0, magicDefense: 5,
    str: 18, agi: 10, vit: 32, int: 20, dex: 39, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 1000, attackMotion: 900, damageMotion: 384,
    size: 'small', race: 'brute', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14300,
    raceGroups: {},
    stats: { str: 18, agi: 10, vit: 32, int: 20, dex: 39, luk: 35, level: 21, weaponATK: 102 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 30 },
        { itemName: 'Feather of Birds', rate: 50 },
        { itemName: 'Concentration Potion', rate: 2 },
        { itemName: 'Grape', rate: 2 },
        { itemName: 'Wind of Verdure', rate: 0.9 },
        { itemName: 'Goggles', rate: 0.07 },
        { itemName: 'Rough Oridecon', rate: 0.32 },
        { itemName: 'Raggler Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Baby (ID: 1686) ──── Level 21 | HP 912 | NORMAL | demihuman/earth1 | aggressive
RO_MONSTER_TEMPLATES['orc_baby'] = {
    id: 1686, name: 'Orc Baby', aegisName: 'ORC_BABY',
    level: 21, maxHealth: 912, baseExp: 220, jobExp: 220, mvpExp: 0,
    attack: 135, attack2: 270, defense: 10, magicDefense: 10,
    str: 30, agi: 15, vit: 10, int: 18, dex: 35, luk: 2,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 200, attackDelay: 672, attackMotion: 864, damageMotion: 288,
    size: 'small', race: 'demihuman', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14300,
    raceGroups: {},
    stats: { str: 30, agi: 15, vit: 10, int: 18, dex: 35, luk: 2, level: 21, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 10 },
        { itemName: 'Pacifier', rate: 1 },
        { itemName: 'Orc Helm', rate: 0.01 },
        { itemName: 'Milk', rate: 50 },
        { itemName: 'Nursing Bottle', rate: 2 },
        { itemName: 'Pinafore', rate: 1 },
        { itemName: 'Orc Baby Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Thara Frog (ID: 1034) ──── Level 22 | HP 2,152 | NORMAL | fish/water2 | passive
RO_MONSTER_TEMPLATES['thara_frog'] = {
    id: 1034, name: 'Thara Frog', aegisName: 'THARA_FROG',
    level: 22, maxHealth: 2152, baseExp: 219, jobExp: 138, mvpExp: 0,
    attack: 105, attack2: 127, defense: 0, magicDefense: 10,
    str: 0, agi: 22, vit: 22, int: 5, dex: 34, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 160, walkSpeed: 200, attackDelay: 2016, attackMotion: 816, damageMotion: 288,
    size: 'medium', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14600,
    raceGroups: {},
    stats: { str: 0, agi: 22, vit: 22, int: 5, dex: 34, luk: 10, level: 22, weaponATK: 105 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.45 },
        { itemName: 'Spawn', rate: 55 },
        { itemName: 'Scell', rate: 6 },
        { itemName: 'White Herb', rate: 0.3 },
        { itemName: 'Red Jewel', rate: 0.05 },
        { itemName: 'Sticky Webfoot', rate: 20 },
        { itemName: 'Thara Frog Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Metaller (ID: 1058) ──── Level 22 | HP 926 | NORMAL | insect/fire1 | aggressive
RO_MONSTER_TEMPLATES['metaller'] = {
    id: 1058, name: 'Metaller', aegisName: 'METALLER',
    level: 22, maxHealth: 926, baseExp: 241, jobExp: 152, mvpExp: 0,
    attack: 131, attack2: 159, defense: 15, magicDefense: 30,
    str: 0, agi: 22, vit: 22, int: 20, dex: 49, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 200, attackDelay: 1708, attackMotion: 1008, damageMotion: 540,
    size: 'medium', race: 'insect', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14600,
    raceGroups: {},
    stats: { str: 0, agi: 22, vit: 22, int: 20, dex: 49, luk: 50, level: 22, weaponATK: 131 },
    modes: { detector: true },
    drops: [
        { itemName: 'Red Blood', rate: 0.6 },
        { itemName: 'Grasshopper\'s Leg', rate: 65 },
        { itemName: 'Scell', rate: 4 },
        { itemName: 'Rough Elunium', rate: 0.49 },
        { itemName: 'Singing Plant', rate: 0.2 },
        { itemName: 'Shell', rate: 30 },
        { itemName: 'Burning Passion Guitar', rate: 0.1 },
        { itemName: 'Metaller Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin (ID: 1126) ──── Level 22 | HP 1,952 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_5'] = {
    id: 1126, name: 'Goblin', aegisName: 'GOBLIN_5',
    level: 22, maxHealth: 1952, baseExp: 241, jobExp: 152, mvpExp: 0,
    attack: 105, attack2: 127, defense: 10, magicDefense: 5,
    str: 0, agi: 22, vit: 22, int: 15, dex: 32, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14600,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 22, vit: 22, int: 15, dex: 32, luk: 10, level: 22, weaponATK: 105 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 1.5 },
        { itemName: 'Scell', rate: 90 },
        { itemName: 'Wand', rate: 0.15 },
        { itemName: 'Buckler', rate: 0.01 },
        { itemName: 'Annoyed Mask', rate: 0.15 },
        { itemName: 'Red Herb', rate: 15 },
        { itemName: 'Yellow Herb', rate: 2.2 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Anacondaq (ID: 1030) ──── Level 23 | HP 1,109 | NORMAL | brute/poison1 | aggressive
RO_MONSTER_TEMPLATES['anacondaq'] = {
    id: 1030, name: 'Anacondaq', aegisName: 'ANACONDAQ',
    level: 23, maxHealth: 1109, baseExp: 300, jobExp: 149, mvpExp: 0,
    attack: 124, attack2: 157, defense: 0, magicDefense: 0,
    str: 0, agi: 23, vit: 28, int: 10, dex: 36, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1576, attackMotion: 576, damageMotion: 576,
    size: 'medium', race: 'brute', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 0, agi: 23, vit: 28, int: 10, dex: 36, luk: 5, level: 23, weaponATK: 124 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.5 },
        { itemName: 'Venom Canine', rate: 90 },
        { itemName: 'Glaive', rate: 0.1 },
        { itemName: 'Snake Scale', rate: 15 },
        { itemName: 'Scale Shell', rate: 2 },
        { itemName: 'Yellow Herb', rate: 1.5 },
        { itemName: 'Rough Oridecon', rate: 0.5 },
        { itemName: 'Anacondaq Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cornutus (ID: 1067) ──── Level 23 | HP 1,620 | NORMAL | fish/water1 | aggressive
RO_MONSTER_TEMPLATES['cornutus'] = {
    id: 1067, name: 'Cornutus', aegisName: 'CORNUTUS',
    level: 23, maxHealth: 1620, baseExp: 240, jobExp: 149, mvpExp: 0,
    attack: 109, attack2: 131, defense: 30, magicDefense: 0,
    str: 0, agi: 23, vit: 23, int: 5, dex: 36, luk: 12,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 200, attackDelay: 1248, attackMotion: 48, damageMotion: 480,
    size: 'small', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 0, agi: 23, vit: 23, int: 5, dex: 36, luk: 12, level: 23, weaponATK: 109 },
    modes: {},
    drops: [
        { itemName: 'Crystal Blue', rate: 0.45 },
        { itemName: 'Conch', rate: 55 },
        { itemName: 'Scell', rate: 8 },
        { itemName: 'Rough Elunium', rate: 0.53 },
        { itemName: 'Shield', rate: 0.05 },
        { itemName: 'Solid Shell', rate: 10 },
        { itemName: 'Blue Gemstone', rate: 1 },
        { itemName: 'Cornutus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Caramel (ID: 1103) ──── Level 23 | HP 1,424 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['caramel'] = {
    id: 1103, name: 'Caramel', aegisName: 'CARAMEL',
    level: 23, maxHealth: 1424, baseExp: 264, jobExp: 162, mvpExp: 0,
    attack: 90, attack2: 112, defense: 5, magicDefense: 5,
    str: 35, agi: 23, vit: 46, int: 5, dex: 38, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1604, attackMotion: 840, damageMotion: 756,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 35, agi: 23, vit: 46, int: 5, dex: 38, luk: 10, level: 23, weaponATK: 90 },
    modes: {},
    drops: [
        { itemName: 'Porcupine Quill', rate: 90 },
        { itemName: 'Coat', rate: 0.05 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Glaive', rate: 0.1 },
        { itemName: 'Spear', rate: 0.15 },
        { itemName: 'Pike', rate: 0.2 },
        { itemName: 'Caramel Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin (ID: 1125) ──── Level 23 | HP 1,359 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_4'] = {
    id: 1125, name: 'Goblin', aegisName: 'GOBLIN_4',
    level: 23, maxHealth: 1359, baseExp: 264, jobExp: 164, mvpExp: 0,
    attack: 109, attack2: 131, defense: 10, magicDefense: 5,
    str: 0, agi: 23, vit: 46, int: 15, dex: 36, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14900,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 23, vit: 46, int: 15, dex: 36, luk: 10, level: 23, weaponATK: 109 },
    modes: {},
    drops: [
        { itemName: 'Green Live', rate: 1 },
        { itemName: 'Iron', rate: 1.7 },
        { itemName: 'Poker Face', rate: 0.15 },
        { itemName: 'Zorro Masque', rate: 0.03 },
        { itemName: 'Smasher', rate: 0.1 },
        { itemName: 'Buckler', rate: 0.01 },
        { itemName: 'Red Herb', rate: 15 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zerom (ID: 1178) ──── Level 23 | HP 1,109 | NORMAL | demihuman/fire1 | aggressive
RO_MONSTER_TEMPLATES['zerom'] = {
    id: 1178, name: 'Zerom', aegisName: 'ZEROM',
    level: 23, maxHealth: 1109, baseExp: 240, jobExp: 149, mvpExp: 0,
    attack: 127, attack2: 155, defense: 0, magicDefense: 10,
    str: 0, agi: 23, vit: 23, int: 5, dex: 42, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 200, attackDelay: 1780, attackMotion: 1080, damageMotion: 432,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 0, agi: 23, vit: 23, int: 5, dex: 42, luk: 0, level: 23, weaponATK: 127 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.55 },
        { itemName: 'Iron', rate: 1.9 },
        { itemName: 'Pantie', rate: 2 },
        { itemName: 'Gangster Mask', rate: 0.03 },
        { itemName: 'Shackles', rate: 0.1 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Iron Ore', rate: 3 },
        { itemName: 'Zerom Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Anopheles (ID: 1627) ──── Level 23 | HP 100 | NORMAL | insect/wind3 | aggressive
RO_MONSTER_TEMPLATES['anopheles'] = {
    id: 1627, name: 'Anopheles', aegisName: 'ANOPHELES',
    level: 23, maxHealth: 100, baseExp: 99, jobExp: 55, mvpExp: 0,
    attack: 48, attack2: 63, defense: 0, magicDefense: 90,
    str: 0, agi: 200, vit: 4, int: 5, dex: 120, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 140, attackMotion: 864, damageMotion: 430,
    size: 'small', race: 'insect', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 0, agi: 200, vit: 4, int: 5, dex: 120, luk: 5, level: 23, weaponATK: 48 },
    modes: { detector: true },
    drops: [
        { itemName: 'Fly Wing', rate: 10 },
        { itemName: 'Bacillus', rate: 5 },
        { itemName: 'Anopheles Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Stapo (ID: 1784) ──── Level 23 | HP 666 | NORMAL | formless/earth2 | passive
RO_MONSTER_TEMPLATES['stapo'] = {
    id: 1784, name: 'Stapo', aegisName: 'STAPO',
    level: 23, maxHealth: 666, baseExp: 332, jobExp: 221, mvpExp: 0,
    attack: 135, attack2: 370, defense: 90, magicDefense: 5,
    str: 12, agi: 11, vit: 15, int: 12, dex: 23, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 181, walkSpeed: 300, attackDelay: 936, attackMotion: 792, damageMotion: 432,
    size: 'small', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 14900,
    raceGroups: {},
    stats: { str: 12, agi: 11, vit: 15, int: 12, dex: 23, luk: 0, level: 23, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Jellopy', rate: 10 },
        { itemName: 'Jubilee', rate: 10 },
        { itemName: 'Apple', rate: 10 },
        { itemName: 'Large Jellopy', rate: 1 },
        { itemName: 'Green Live', rate: 0.1 },
        { itemName: 'Seismic Fist', rate: 0.03 },
        { itemName: 'Stapo Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Scorpion (ID: 1001) ──── Level 24 | HP 1,109 | NORMAL | insect/fire1 | aggressive
RO_MONSTER_TEMPLATES['scorpion'] = {
    id: 1001, name: 'Scorpion', aegisName: 'SCORPION',
    level: 24, maxHealth: 1109, baseExp: 287, jobExp: 176, mvpExp: 0,
    attack: 80, attack2: 135, defense: 30, magicDefense: 0,
    str: 0, agi: 24, vit: 24, int: 5, dex: 52, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 200, attackDelay: 1564, attackMotion: 864, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 24, int: 5, dex: 52, luk: 5, level: 24, weaponATK: 80 },
    modes: { detector: true },
    drops: [
        { itemName: 'Red Blood', rate: 0.7 },
        { itemName: 'Scorpion\'s Tail', rate: 55 },
        { itemName: 'Rough Elunium', rate: 0.57 },
        { itemName: 'Solid Shell', rate: 2.1 },
        { itemName: 'Fine Grit', rate: 1 },
        { itemName: 'Yellow Herb', rate: 2 },
        { itemName: 'Rusty Iron', rate: 0.2 },
        { itemName: 'Scorpion Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Warrior (ID: 1023) ──── Level 24 | HP 1,400 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['ork_warrior'] = {
    id: 1023, name: 'Orc Warrior', aegisName: 'ORK_WARRIOR',
    level: 24, maxHealth: 1400, baseExp: 408, jobExp: 160, mvpExp: 0,
    attack: 104, attack2: 126, defense: 10, magicDefense: 5,
    str: 0, agi: 24, vit: 48, int: 25, dex: 34, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 24, vit: 48, int: 25, dex: 34, luk: 10, level: 24, weaponATK: 104 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.1 },
        { itemName: 'Orcish Voucher', rate: 90 },
        { itemName: 'Rough Oridecon', rate: 0.4 },
        { itemName: 'Cigarette', rate: 0.03 },
        { itemName: 'Battle Axe', rate: 0.1 },
        { itemName: 'Orcish Axe', rate: 0.05 },
        { itemName: 'Axe', rate: 1 },
        { itemName: 'Orc Warrior Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Megalodon (ID: 1064) ──── Level 24 | HP 1,648 | NORMAL | undead/undead1 | passive
RO_MONSTER_TEMPLATES['megalodon'] = {
    id: 1064, name: 'Megalodon', aegisName: 'MEGALODON',
    level: 24, maxHealth: 1648, baseExp: 215, jobExp: 132, mvpExp: 0,
    attack: 155, attack2: 188, defense: 0, magicDefense: 15,
    str: 0, agi: 12, vit: 24, int: 0, dex: 26, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 150, walkSpeed: 200, attackDelay: 2492, attackMotion: 792, damageMotion: 432,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 24, int: 0, dex: 26, luk: 5, level: 24, weaponATK: 155 },
    modes: {},
    drops: [
        { itemName: 'Stinky Scale', rate: 55 },
        { itemName: 'Skel-Bone', rate: 15 },
        { itemName: 'Blue Herb', rate: 0.8 },
        { itemName: 'Blue Gemstone', rate: 1.2 },
        { itemName: 'Amethyst', rate: 0.1 },
        { itemName: 'Old Blue Box', rate: 0.02 },
        { itemName: 'Rotten Fish', rate: 0.2 },
        { itemName: 'Megalodon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Vagabond Wolf (ID: 1092) ──── Level 24 | HP 12,240 | BOSS | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['vagabond_wolf'] = {
    id: 1092, name: 'Vagabond Wolf', aegisName: 'VAGABOND_WOLF',
    level: 24, maxHealth: 12240, baseExp: 247, jobExp: 176, mvpExp: 0,
    attack: 135, attack2: 159, defense: 10, magicDefense: 0,
    str: 57, agi: 45, vit: 48, int: 20, dex: 50, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 150, attackDelay: 1048, attackMotion: 648, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 57, agi: 45, vit: 48, int: 20, dex: 50, luk: 65, level: 24, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Western Grace', rate: 2 },
        { itemName: 'Wolf Claw', rate: 80 },
        { itemName: 'Topaz', rate: 15 },
        { itemName: 'Star Dust Blade', rate: 1 },
        { itemName: 'Angel\'s Warmth', rate: 10 },
        { itemName: 'Red Jewel', rate: 0.1 },
        { itemName: 'Monster Juice', rate: 0.5 },
        { itemName: 'Vagabond Wolf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Drainliar (ID: 1111) ──── Level 24 | HP 1,162 | NORMAL | brute/shadow2 | aggressive
RO_MONSTER_TEMPLATES['drainliar'] = {
    id: 1111, name: 'Drainliar', aegisName: 'DRAINLIAR',
    level: 24, maxHealth: 1162, baseExp: 431, jobExp: 176, mvpExp: 0,
    attack: 74, attack2: 84, defense: 0, magicDefense: 0,
    str: 0, agi: 36, vit: 24, int: 0, dex: 78, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 250, attackDelay: 1276, attackMotion: 576, damageMotion: 384,
    size: 'small', race: 'brute', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 36, vit: 24, int: 0, dex: 78, luk: 0, level: 24, weaponATK: 74 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.6 },
        { itemName: 'Tooth of Bat', rate: 30 },
        { itemName: 'Red Jewel', rate: 0.2 },
        { itemName: 'Red Herb', rate: 10 },
        { itemName: 'Wing of Red Bat', rate: 55 },
        { itemName: 'Wing of Red Bat', rate: 15 },
        { itemName: 'Rough Oridecon', rate: 0.4 },
        { itemName: 'Drainliar Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Eggyra (ID: 1116) ──── Level 24 | HP 633 | NORMAL | formless/ghost2 | aggressive
RO_MONSTER_TEMPLATES['eggyra'] = {
    id: 1116, name: 'Eggyra', aegisName: 'EGGYRA',
    level: 24, maxHealth: 633, baseExp: 215, jobExp: 220, mvpExp: 0,
    attack: 85, attack2: 107, defense: 20, magicDefense: 25,
    str: 0, agi: 36, vit: 24, int: 0, dex: 32, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 200, attackDelay: 1816, attackMotion: 816, damageMotion: 288,
    size: 'medium', race: 'formless', element: { type: 'ghost', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 36, vit: 24, int: 0, dex: 32, luk: 0, level: 24, weaponATK: 85 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 10 },
        { itemName: 'Egg Shell', rate: 0.2 },
        { itemName: 'Piece of Egg Shell', rate: 5.5 },
        { itemName: 'Red Herb', rate: 10 },
        { itemName: 'Pet Incubator', rate: 3 },
        { itemName: 'Concentration Potion', rate: 2.5 },
        { itemName: 'Rough Elunium', rate: 0.57 },
        { itemName: 'Eggyra Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin (ID: 1123) ──── Level 24 | HP 1,034 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_2'] = {
    id: 1123, name: 'Goblin', aegisName: 'GOBLIN_2',
    level: 24, maxHealth: 1034, baseExp: 287, jobExp: 176, mvpExp: 0,
    attack: 88, attack2: 100, defense: 10, magicDefense: 5,
    str: 0, agi: 24, vit: 24, int: 15, dex: 66, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 24, vit: 24, int: 15, dex: 66, luk: 10, level: 24, weaponATK: 88 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.5 },
        { itemName: 'Scell', rate: 90 },
        { itemName: 'Indian Fillet', rate: 0.03 },
        { itemName: 'Flail', rate: 0.1 },
        { itemName: 'Buckler', rate: 0.01 },
        { itemName: 'Red Herb', rate: 15.5 },
        { itemName: 'Goblin Mask', rate: 0.03 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin (ID: 1124) ──── Level 24 | HP 1,034 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_3'] = {
    id: 1124, name: 'Goblin', aegisName: 'GOBLIN_3',
    level: 24, maxHealth: 1034, baseExp: 357, jobExp: 176, mvpExp: 0,
    attack: 132, attack2: 165, defense: 10, magicDefense: 5,
    str: 0, agi: 24, vit: 24, int: 15, dex: 24, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 24, vit: 24, int: 15, dex: 24, luk: 10, level: 24, weaponATK: 132 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.3 },
        { itemName: 'Scell', rate: 90 },
        { itemName: 'Red Bandana', rate: 0.03 },
        { itemName: 'Surprised Mask', rate: 0.15 },
        { itemName: 'Buckler', rate: 0.01 },
        { itemName: 'Red Herb', rate: 15.5 },
        { itemName: 'Yellow Herb', rate: 2.2 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Zombie (ID: 1153) ──── Level 24 | HP 1,568 | NORMAL | undead/neutral1 | passive
RO_MONSTER_TEMPLATES['orc_zombie'] = {
    id: 1153, name: 'Orc Zombie', aegisName: 'ORC_ZOMBIE',
    level: 24, maxHealth: 1568, baseExp: 196, jobExp: 120, mvpExp: 0,
    attack: 151, attack2: 184, defense: 5, magicDefense: 10,
    str: 0, agi: 12, vit: 24, int: 0, dex: 24, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'undead', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 12, vit: 24, int: 0, dex: 24, luk: 5, level: 24, weaponATK: 151 },
    modes: {},
    drops: [
        { itemName: 'Orc Claw', rate: 55 },
        { itemName: 'Sticky Mucus', rate: 30 },
        { itemName: 'Emperium', rate: 0.01 },
        { itemName: 'Orc Zombie Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Smoking Orc (ID: 1235) ──── Level 24 | HP 1,400 | NORMAL | demihuman/earth1 | aggressive
RO_MONSTER_TEMPLATES['smoking_orc'] = {
    id: 1235, name: 'Smoking Orc', aegisName: 'SMOKING_ORC',
    level: 24, maxHealth: 1400, baseExp: 261, jobExp: 160, mvpExp: 0,
    attack: 114, attack2: 136, defense: 10, magicDefense: 20,
    str: 0, agi: 24, vit: 48, int: 20, dex: 34, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1864, attackMotion: 864, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 48, int: 20, dex: 34, luk: 0, level: 24, weaponATK: 114 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.1 },
        { itemName: 'Orcish Voucher', rate: 55 },
        { itemName: 'Rough Oridecon', rate: 0.4 },
        { itemName: 'Cigarette', rate: 0.03 },
        { itemName: 'Battle Axe', rate: 0.1 },
        { itemName: 'Orcish Axe', rate: 0.05 },
        { itemName: 'Axe', rate: 1 },
        { itemName: 'Orc Warrior Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Christmas Orc (ID: 1588) ──── Level 24 | HP 1,400 | NORMAL | demihuman/earth1 | passive
RO_MONSTER_TEMPLATES['orc_xmas'] = {
    id: 1588, name: 'Christmas Orc', aegisName: 'ORC_XMAS',
    level: 24, maxHealth: 1400, baseExp: 261, jobExp: 160, mvpExp: 0,
    attack: 104, attack2: 126, defense: 10, magicDefense: 5,
    str: 0, agi: 24, vit: 48, int: 25, dex: 34, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1864, attackMotion: 864, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15200,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 48, int: 25, dex: 34, luk: 10, level: 24, weaponATK: 104 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.1 },
        { itemName: 'Orcish Voucher', rate: 55 },
        { itemName: 'Rough Oridecon', rate: 0.4 },
        { itemName: 'Wrapping Paper', rate: 16 },
        { itemName: 'Battle Axe', rate: 0.1 },
        { itemName: 'Gift Box', rate: 0.15 },
        { itemName: 'Wrapping Lace', rate: 16 },
        { itemName: 'Orc Warrior Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wolf (ID: 1013) ──── Level 25 | HP 919 | NORMAL | brute/earth1 | reactive
RO_MONSTER_TEMPLATES['wolf'] = {
    id: 1013, name: 'Wolf', aegisName: 'WOLF',
    level: 25, maxHealth: 919, baseExp: 329, jobExp: 199, mvpExp: 0,
    attack: 37, attack2: 46, defense: 0, magicDefense: 0,
    str: 0, agi: 20, vit: 28, int: 15, dex: 32, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1054, attackMotion: 504, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 28, int: 15, dex: 32, luk: 20, level: 25, weaponATK: 37 },
    modes: {},
    drops: [
        { itemName: 'Emveretarcon', rate: 0.2 },
        { itemName: 'Wolf Claw', rate: 90 },
        { itemName: 'Mantle', rate: 0.1 },
        { itemName: 'Meat', rate: 6.5 },
        { itemName: 'Monster\'s Feed', rate: 10.5 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Strawberry', rate: 6 },
        { itemName: 'Wolf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Golem (ID: 1040) ──── Level 25 | HP 3,900 | NORMAL | formless/neutral1 | passive
RO_MONSTER_TEMPLATES['golem'] = {
    id: 1040, name: 'Golem', aegisName: 'GOLEM',
    level: 25, maxHealth: 3900, baseExp: 465, jobExp: 94, mvpExp: 0,
    attack: 175, attack2: 187, defense: 40, magicDefense: 0,
    str: 0, agi: 15, vit: 25, int: 0, dex: 15, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15500,
    raceGroups: { Golem: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 15, vit: 25, int: 0, dex: 15, luk: 0, level: 25, weaponATK: 175 },
    modes: {},
    drops: [
        { itemName: 'Steel', rate: 1.5 },
        { itemName: 'Stone Heart', rate: 90 },
        { itemName: 'Zargon', rate: 2.2 },
        { itemName: 'Rough Elunium', rate: 0.7 },
        { itemName: 'Coal', rate: 2.1 },
        { itemName: 'Yellow Gemstone', rate: 2 },
        { itemName: 'Iron', rate: 3.5 },
        { itemName: 'Golem Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bigfoot (ID: 1060) ──── Level 25 | HP 1,619 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['bigfoot'] = {
    id: 1060, name: 'Bigfoot', aegisName: 'BIGFOOT',
    level: 25, maxHealth: 1619, baseExp: 310, jobExp: 188, mvpExp: 0,
    attack: 198, attack2: 220, defense: 10, magicDefense: 0,
    str: 0, agi: 25, vit: 55, int: 15, dex: 20, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 300, attackDelay: 1260, attackMotion: 192, damageMotion: 192,
    size: 'large', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 25, vit: 55, int: 15, dex: 20, luk: 25, level: 25, weaponATK: 198 },
    modes: {},
    drops: [
        { itemName: 'Bear\'s Foot', rate: 90 },
        { itemName: 'Poo Poo Hat', rate: 0.05 },
        { itemName: 'Animal\'s Skin', rate: 50 },
        { itemName: 'Puppet', rate: 0.8 },
        { itemName: 'Sweet Potato', rate: 15 },
        { itemName: 'Honey', rate: 4.5 },
        { itemName: 'Rough Oridecon', rate: 0.43 },
        { itemName: 'Bigfoot Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pirate Skeleton (ID: 1071) ──── Level 25 | HP 1,676 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['pirate_skel'] = {
    id: 1071, name: 'Pirate Skeleton', aegisName: 'PIRATE_SKEL',
    level: 25, maxHealth: 1676, baseExp: 233, jobExp: 142, mvpExp: 0,
    attack: 145, attack2: 178, defense: 10, magicDefense: 15,
    str: 25, agi: 13, vit: 25, int: 5, dex: 25, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 200, attackDelay: 1754, attackMotion: 554, damageMotion: 288,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 25, agi: 13, vit: 25, int: 5, dex: 25, luk: 10, level: 25, weaponATK: 145 },
    modes: {},
    drops: [
        { itemName: 'Skel-Bone', rate: 30 },
        { itemName: 'Pirate Bandana', rate: 0.15 },
        { itemName: 'Level 6 Cookbook', rate: 0.05 },
        { itemName: 'Bandana', rate: 2.5 },
        { itemName: 'Falchion', rate: 2.5 },
        { itemName: 'Rough Oridecon', rate: 0.43 },
        { itemName: 'Well-Dried Bone', rate: 0.2 },
        { itemName: 'Pirate Skeleton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Argos (ID: 1100) ──── Level 25 | HP 1,117 | NORMAL | insect/poison1 | aggressive
RO_MONSTER_TEMPLATES['argos'] = {
    id: 1100, name: 'Argos', aegisName: 'ARGOS',
    level: 25, maxHealth: 1117, baseExp: 388, jobExp: 188, mvpExp: 0,
    attack: 158, attack2: 191, defense: 15, magicDefense: 0,
    str: 0, agi: 25, vit: 25, int: 5, dex: 32, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 300, attackDelay: 1468, attackMotion: 468, damageMotion: 768,
    size: 'large', race: 'insect', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 25, vit: 25, int: 5, dex: 32, luk: 15, level: 25, weaponATK: 158 },
    modes: { detector: true },
    drops: [
        { itemName: 'Cobweb', rate: 90 },
        { itemName: 'Scell', rate: 12 },
        { itemName: 'Bug Leg', rate: 5 },
        { itemName: 'Rough Elunium', rate: 0.61 },
        { itemName: 'Green Herb', rate: 6.7 },
        { itemName: 'Yellow Herb', rate: 2.5 },
        { itemName: 'Bark Shorts', rate: 0.15 },
        { itemName: 'Argos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin (ID: 1122) ──── Level 25 | HP 1,176 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_1'] = {
    id: 1122, name: 'Goblin', aegisName: 'GOBLIN_1',
    level: 25, maxHealth: 1176, baseExp: 310, jobExp: 188, mvpExp: 0,
    attack: 118, attack2: 140, defense: 10, magicDefense: 5,
    str: 0, agi: 53, vit: 25, int: 20, dex: 38, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15500,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 53, vit: 25, int: 20, dex: 38, luk: 10, level: 25, weaponATK: 118 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 2.7 },
        { itemName: 'Scell', rate: 90 },
        { itemName: 'Rough Oridecon', rate: 0.43 },
        { itemName: 'Goblin Mask', rate: 0.03 },
        { itemName: 'Dirk', rate: 0.1 },
        { itemName: 'Buckler', rate: 0.05 },
        { itemName: 'Red Herb', rate: 18 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Christmas Goblin (ID: 1245) ──── Level 25 | HP 1,176 | NORMAL | demihuman/wind1 | passive
RO_MONSTER_TEMPLATES['gobline_xmas'] = {
    id: 1245, name: 'Christmas Goblin', aegisName: 'GOBLINE_XMAS',
    level: 25, maxHealth: 1176, baseExp: 282, jobExp: 171, mvpExp: 0,
    attack: 118, attack2: 140, defense: 10, magicDefense: 5,
    str: 0, agi: 53, vit: 25, int: 20, dex: 38, luk: 45,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 178, walkSpeed: 100, attackDelay: 1120, attackMotion: 620, damageMotion: 240,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 53, vit: 25, int: 20, dex: 38, luk: 45, level: 25, weaponATK: 118 },
    modes: {},
    drops: [
        { itemName: 'Wrapping Lace', rate: 5.5 },
        { itemName: 'Wrapping Paper', rate: 5.5 },
        { itemName: 'Rough Oridecon', rate: 0.43 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: 'Dirk', rate: 0.1 },
        { itemName: 'Buckler', rate: 0.05 },
        { itemName: 'Santa\'s Hat', rate: 0.1 },
        { itemName: 'Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cookie (ID: 1265) ──── Level 25 | HP 950 | NORMAL | demihuman/neutral3 | reactive
RO_MONSTER_TEMPLATES['cookie'] = {
    id: 1265, name: 'Cookie', aegisName: 'COOKIE',
    level: 25, maxHealth: 950, baseExp: 310, jobExp: 188, mvpExp: 0,
    attack: 130, attack2: 145, defense: 0, magicDefense: 25,
    str: 0, agi: 35, vit: 20, int: 53, dex: 37, luk: 90,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1036, attackMotion: 936, damageMotion: 240,
    size: 'small', race: 'demihuman', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 35, vit: 20, int: 53, dex: 37, luk: 90, level: 25, weaponATK: 130 },
    modes: {},
    drops: [
        { itemName: 'Well-baked Cookie', rate: 10 },
        { itemName: 'Candy Cane', rate: 1.5 },
        { itemName: 'Darkgreen Dyestuffs', rate: 0.01 },
        { itemName: 'Chef King Orleans Vol.1', rate: 0.5 },
        { itemName: 'Sandals', rate: 0.3 },
        { itemName: 'Level 3 Heal', rate: 1 },
        { itemName: 'Candy', rate: 3.2 },
        { itemName: 'Cookie Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Rotar Zairo (ID: 1392) ──── Level 25 | HP 1,209 | NORMAL | formless/wind2 | aggressive
RO_MONSTER_TEMPLATES['rotar_zairo'] = {
    id: 1392, name: 'Rotar Zairo', aegisName: 'ROTAR_ZAIRO',
    level: 25, maxHealth: 1209, baseExp: 351, jobExp: 215, mvpExp: 0,
    attack: 109, attack2: 137, defense: 4, magicDefense: 34,
    str: 0, agi: 62, vit: 45, int: 26, dex: 55, luk: 5,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 152, walkSpeed: 155, attackDelay: 2416, attackMotion: 2016, damageMotion: 432,
    size: 'large', race: 'formless', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15500,
    raceGroups: {},
    stats: { str: 0, agi: 62, vit: 45, int: 26, dex: 55, luk: 5, level: 25, weaponATK: 109 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 5 },
        { itemName: 'Padded Armor', rate: 0.01 },
        { itemName: 'Cyfar', rate: 10 },
        { itemName: 'Steel', rate: 4.5 },
        { itemName: 'Oridecon', rate: 0.01 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Garlet', rate: 55 },
        { itemName: 'Rotar Zairo Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Flora (ID: 1118) ──── Level 26 | HP 2,092 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['flora'] = {
    id: 1118, name: 'Flora', aegisName: 'FLORA',
    level: 26, maxHealth: 2092, baseExp: 357, jobExp: 226, mvpExp: 0,
    attack: 242, attack2: 273, defense: 10, magicDefense: 35,
    str: 0, agi: 26, vit: 35, int: 5, dex: 43, luk: 80,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 171, walkSpeed: 1000, attackDelay: 1432, attackMotion: 432, damageMotion: 576,
    size: 'large', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 35, int: 5, dex: 43, luk: 80, level: 26, weaponATK: 242 },
    modes: {},
    drops: [
        { itemName: 'Maneater Blossom', rate: 90 },
        { itemName: 'Sunflower', rate: 0.03 },
        { itemName: 'Aloe', rate: 0.1 },
        { itemName: 'Aloe Leaflet', rate: 0.5 },
        { itemName: 'Singing Flower', rate: 0.2 },
        { itemName: 'Stem', rate: 20 },
        { itemName: 'Witherless Rose', rate: 0.01 },
        { itemName: 'Flora Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hode (ID: 1127) ──── Level 26 | HP 2,282 | NORMAL | brute/earth2 | passive
RO_MONSTER_TEMPLATES['hode'] = {
    id: 1127, name: 'Hode', aegisName: 'HODE',
    level: 26, maxHealth: 2282, baseExp: 550, jobExp: 300, mvpExp: 0,
    attack: 146, attack2: 177, defense: 0, magicDefense: 30,
    str: 0, agi: 26, vit: 42, int: 5, dex: 49, luk: 40,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1480, attackMotion: 480, damageMotion: 720,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 42, int: 5, dex: 49, luk: 40, level: 26, weaponATK: 146 },
    modes: {},
    drops: [
        { itemName: 'Green Live', rate: 1.2 },
        { itemName: 'Earthworm Peeling', rate: 90 },
        { itemName: 'Rough Elunium', rate: 0.8 },
        { itemName: 'Sticky Mucus', rate: 30 },
        { itemName: 'Town Sword', rate: 0.1 },
        { itemName: 'Foolishness of the Blind', rate: 0.01 },
        { itemName: 'Fatty Chubby Earthworm', rate: 0.2 },
        { itemName: 'Hode Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Magnolia (ID: 1138) ──── Level 26 | HP 3,195 | NORMAL | demon/water1 | passive
RO_MONSTER_TEMPLATES['magnolia'] = {
    id: 1138, name: 'Magnolia', aegisName: 'MAGNOLIA',
    level: 26, maxHealth: 3195, baseExp: 393, jobExp: 248, mvpExp: 0,
    attack: 120, attack2: 151, defense: 5, magicDefense: 30,
    str: 0, agi: 26, vit: 26, int: 0, dex: 39, luk: 5,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 169, walkSpeed: 250, attackDelay: 1560, attackMotion: 360, damageMotion: 360,
    size: 'small', race: 'demon', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 26, int: 0, dex: 39, luk: 5, level: 26, weaponATK: 120 },
    modes: { detector: true },
    drops: [
        { itemName: 'Old Frying Pan', rate: 90 },
        { itemName: 'Garlet', rate: 8 },
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Zargon', rate: 0.1 },
        { itemName: 'Black Ladle', rate: 0.4 },
        { itemName: 'Yellow Herb', rate: 4 },
        { itemName: 'Professional Cooking Kit', rate: 0.05 },
        { itemName: 'Magnolia Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mantis (ID: 1139) ──── Level 26 | HP 2,472 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['mantis'] = {
    id: 1139, name: 'Mantis', aegisName: 'MANTIS',
    level: 26, maxHealth: 2472, baseExp: 393, jobExp: 248, mvpExp: 0,
    attack: 118, attack2: 149, defense: 10, magicDefense: 0,
    str: 0, agi: 26, vit: 24, int: 5, dex: 45, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 200, attackDelay: 1528, attackMotion: 660, damageMotion: 432,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 24, int: 5, dex: 45, luk: 15, level: 26, weaponATK: 118 },
    modes: { detector: true },
    drops: [
        { itemName: 'Green Live', rate: 1.1 },
        { itemName: 'Mantis Scythe', rate: 90 },
        { itemName: 'Scell', rate: 14 },
        { itemName: 'Rough Elunium', rate: 0.7 },
        { itemName: 'Solid Shell', rate: 2.5 },
        { itemName: 'Emerald', rate: 0.1 },
        { itemName: 'Red Herb', rate: 6.5 },
        { itemName: 'Mantis Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Phen (ID: 1158) ──── Level 26 | HP 3,347 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['phen'] = {
    id: 1158, name: 'Phen', aegisName: 'PHEN',
    level: 26, maxHealth: 3347, baseExp: 357, jobExp: 226, mvpExp: 0,
    attack: 138, attack2: 150, defense: 0, magicDefense: 15,
    str: 0, agi: 26, vit: 26, int: 0, dex: 88, luk: 75,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 149, walkSpeed: 150, attackDelay: 2544, attackMotion: 1344, damageMotion: 1152,
    size: 'medium', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 26, int: 0, dex: 88, luk: 75, level: 26, weaponATK: 138 },
    modes: {},
    drops: [
        { itemName: 'Fish Tail', rate: 55 },
        { itemName: 'Sharp Scale', rate: 20 },
        { itemName: 'Aquamarine', rate: 0.05 },
        { itemName: 'Meat', rate: 10 },
        { itemName: 'Fin', rate: 5 },
        { itemName: 'Rough Oridecon', rate: 0.25 },
        { itemName: 'Phen Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Savage (ID: 1166) ──── Level 26 | HP 2,092 | NORMAL | brute/earth2 | aggressive
RO_MONSTER_TEMPLATES['savage'] = {
    id: 1166, name: 'Savage', aegisName: 'SAVAGE',
    level: 26, maxHealth: 2092, baseExp: 521, jobExp: 248, mvpExp: 0,
    attack: 120, attack2: 150, defense: 10, magicDefense: 5,
    str: 0, agi: 26, vit: 54, int: 10, dex: 37, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 960, damageMotion: 384,
    size: 'large', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 54, int: 10, dex: 37, luk: 15, level: 26, weaponATK: 120 },
    modes: {},
    drops: [
        { itemName: 'Wild Boar\'s Mane', rate: 90 },
        { itemName: 'Grape', rate: 3 },
        { itemName: 'Animal Gore', rate: 0.02 },
        { itemName: 'Angled Glasses', rate: 0.01 },
        { itemName: 'Anodyne', rate: 0.1 },
        { itemName: 'Rough Elunium', rate: 0.7 },
        { itemName: 'Royal Jelly', rate: 0.02 },
        { itemName: 'Savage Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Savage (ID: 1221) ──── Level 26 | HP 2,092 | NORMAL | brute/earth2 | aggressive
RO_MONSTER_TEMPLATES['m_savage'] = {
    id: 1221, name: 'Savage', aegisName: 'M_SAVAGE',
    level: 26, maxHealth: 2092, baseExp: 357, jobExp: 226, mvpExp: 0,
    attack: 146, attack2: 177, defense: 10, magicDefense: 5,
    str: 0, agi: 26, vit: 54, int: 10, dex: 37, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 960, damageMotion: 384,
    size: 'large', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 0, agi: 26, vit: 54, int: 10, dex: 37, luk: 10, level: 26, weaponATK: 146 },
    modes: {},
    drops: [
        { itemName: 'Wild Boar\'s Mane', rate: 60 },
        { itemName: 'Grape', rate: 1.5 },
        { itemName: 'Animal Gore', rate: 0.03 },
        { itemName: 'Angled Glasses', rate: 0.02 },
        { itemName: 'Anodyne', rate: 0.15 },
        { itemName: 'Rough Elunium', rate: 0.7 },
        { itemName: 'Savage Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Metaling (ID: 1613) ──── Level 26 | HP 889 | NORMAL | formless/neutral1 | passive
RO_MONSTER_TEMPLATES['metaling'] = {
    id: 1613, name: 'Metaling', aegisName: 'METALING',
    level: 26, maxHealth: 889, baseExp: 492, jobExp: 249, mvpExp: 0,
    attack: 135, attack2: 270, defense: 5, magicDefense: 3,
    str: 30, agi: 15, vit: 10, int: 18, dex: 35, luk: 2,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 300, attackDelay: 384, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 15800,
    raceGroups: {},
    stats: { str: 30, agi: 15, vit: 10, int: 18, dex: 35, luk: 2, level: 26, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Flexible Tube', rate: 40 },
        { itemName: 'Iron Ore', rate: 10 },
        { itemName: 'Iron', rate: 5 },
        { itemName: 'Large Jellopy', rate: 10 },
        { itemName: 'Rusty Screw', rate: 2 },
        { itemName: 'Crimson Bolt', rate: 0.05 },
        { itemName: 'Jubilee', rate: 50 },
        { itemName: 'Metaling Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Desert Wolf (ID: 1106) ──── Level 27 | HP 1,716 | NORMAL | brute/fire1 | aggressive
RO_MONSTER_TEMPLATES['desert_wolf'] = {
    id: 1106, name: 'Desert Wolf', aegisName: 'DESERT_WOLF',
    level: 27, maxHealth: 1716, baseExp: 427, jobExp: 266, mvpExp: 0,
    attack: 169, attack2: 208, defense: 0, magicDefense: 10,
    str: 56, agi: 27, vit: 45, int: 15, dex: 56, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1120, attackMotion: 420, damageMotion: 288,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16100,
    raceGroups: {},
    stats: { str: 56, agi: 27, vit: 45, int: 15, dex: 56, luk: 10, level: 27, weaponATK: 169 },
    modes: {},
    drops: [
        { itemName: 'Katar', rate: 0.05 },
        { itemName: 'Claw of Desert Wolf', rate: 55 },
        { itemName: 'Mink Coat', rate: 0.01 },
        { itemName: 'Meat', rate: 12 },
        { itemName: 'Wolf Claw', rate: 20 },
        { itemName: 'Rough Oridecon', rate: 0.53 },
        { itemName: 'Stiletto', rate: 1.4 },
        { itemName: 'Desert Wolf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Desert Wolf (ID: 1220) ──── Level 27 | HP 1,716 | NORMAL | brute/fire1 | aggressive
RO_MONSTER_TEMPLATES['m_desert_wolf'] = {
    id: 1220, name: 'Desert Wolf', aegisName: 'M_DESERT_WOLF',
    level: 27, maxHealth: 1716, baseExp: 388, jobExp: 242, mvpExp: 0,
    attack: 169, attack2: 208, defense: 0, magicDefense: 10,
    str: 0, agi: 27, vit: 45, int: 15, dex: 56, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1120, attackMotion: 420, damageMotion: 288,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16100,
    raceGroups: {},
    stats: { str: 0, agi: 27, vit: 45, int: 15, dex: 56, luk: 10, level: 27, weaponATK: 169 },
    modes: {},
    drops: [
        { itemName: 'Katar', rate: 0.05 },
        { itemName: 'Claw of Desert Wolf', rate: 55 },
        { itemName: 'Mink Coat', rate: 0.01 },
        { itemName: 'Meat', rate: 12 },
        { itemName: 'Wolf Claw', rate: 20 },
        { itemName: 'Rough Oridecon', rate: 0.53 },
        { itemName: 'Desert Wolf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dumpling Child (ID: 1409) ──── Level 27 | HP 2,098 | NORMAL | demihuman/neutral1 | aggressive
RO_MONSTER_TEMPLATES['rice_cake_boy'] = {
    id: 1409, name: 'Dumpling Child', aegisName: 'RICE_CAKE_BOY',
    level: 27, maxHealth: 2098, baseExp: 231, jobExp: 149, mvpExp: 0,
    attack: 112, attack2: 134, defense: 5, magicDefense: 12,
    str: 0, agi: 22, vit: 29, int: 5, dex: 41, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 160, attackDelay: 647, attackMotion: 768, damageMotion: 420,
    size: 'small', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16100,
    raceGroups: {},
    stats: { str: 0, agi: 22, vit: 29, int: 5, dex: 41, luk: 10, level: 27, weaponATK: 112 },
    modes: {},
    drops: [
        { itemName: 'Piece of Bamboo', rate: 32 },
        { itemName: 'Oil Paper', rate: 25 },
        { itemName: 'Clown Nose', rate: 0.01 },
        { itemName: 'Vane', rate: 50 },
        { itemName: 'Bun', rate: 10 },
        { itemName: 'Festival Mask', rate: 30 },
        { itemName: 'Dumpling Child Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marine Sphere (ID: 1142) ──── Level 28 | HP 3,518 | NORMAL | plant/water2 | passive
RO_MONSTER_TEMPLATES['marine_sphere'] = {
    id: 1142, name: 'Marine Sphere', aegisName: 'MARINE_SPHERE',
    level: 28, maxHealth: 3518, baseExp: 461, jobExp: 284, mvpExp: 0,
    attack: 120, attack2: 320, defense: 0, magicDefense: 40,
    str: 0, agi: 28, vit: 28, int: 0, dex: 33, luk: 50,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 176, walkSpeed: 800, attackDelay: 1201, attackMotion: 1, damageMotion: 1,
    size: 'small', race: 'plant', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 16400,
    raceGroups: {},
    stats: { str: 0, agi: 28, vit: 28, int: 0, dex: 33, luk: 50, level: 28, weaponATK: 120 },
    modes: {},
    drops: [
        { itemName: 'Tendon', rate: 50 },
        { itemName: 'Detonator', rate: 25 },
        { itemName: 'Chain', rate: 0.1 },
        { itemName: 'Aquamarine', rate: 0.1 },
        { itemName: 'Blue Gemstone', rate: 1.5 },
        { itemName: 'Transparent Head Protector', rate: 0.1 },
        { itemName: 'Marine Sphere Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Skeleton (ID: 1152) ──── Level 28 | HP 2,278 | NORMAL | undead/neutral1 | passive
RO_MONSTER_TEMPLATES['orc_skeleton'] = {
    id: 1152, name: 'Orc Skeleton', aegisName: 'ORC_SKELETON',
    level: 28, maxHealth: 2278, baseExp: 315, jobExp: 194, mvpExp: 0,
    attack: 190, attack2: 236, defense: 10, magicDefense: 10,
    str: 0, agi: 14, vit: 18, int: 0, dex: 30, luk: 15,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'undead', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 16400,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 14, vit: 18, int: 0, dex: 30, luk: 15, level: 28, weaponATK: 190 },
    modes: {},
    drops: [
        { itemName: 'Orc\'s Fang', rate: 55 },
        { itemName: 'Skel-Bone', rate: 35 },
        { itemName: 'Rough Elunium', rate: 0.8 },
        { itemName: 'Orc Helm', rate: 0.02 },
        { itemName: 'Buster', rate: 0.1 },
        { itemName: 'Green Herb', rate: 0.5 },
        { itemName: 'Orc Skeleton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Christmas Cookie (ID: 1246) ──── Level 28 | HP 2,090 | NORMAL | demihuman/holy2 | aggressive
RO_MONSTER_TEMPLATES['cookie_xmas'] = {
    id: 1246, name: 'Christmas Cookie', aegisName: 'COOKIE_XMAS',
    level: 28, maxHealth: 2090, baseExp: 461, jobExp: 284, mvpExp: 0,
    attack: 140, attack2: 170, defense: 0, magicDefense: 50,
    str: 0, agi: 24, vit: 30, int: 53, dex: 45, luk: 100,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 400, attackDelay: 1248, attackMotion: 1248, damageMotion: 240,
    size: 'small', race: 'demihuman', element: { type: 'holy', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16400,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 30, int: 53, dex: 45, luk: 100, level: 28, weaponATK: 140 },
    modes: {},
    drops: [
        { itemName: 'Well-baked Cookie', rate: 15 },
        { itemName: 'Pearl', rate: 0.45 },
        { itemName: 'Zargon', rate: 2 },
        { itemName: 'Hood', rate: 0.25 },
        { itemName: 'Gift Box', rate: 0.05 },
        { itemName: 'Level 3 Cold Bolt', rate: 1 },
        { itemName: 'Red Herb', rate: 17 },
        { itemName: 'Christmas Cookie Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin Archer (ID: 1258) ──── Level 28 | HP 1,750 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_archer'] = {
    id: 1258, name: 'Goblin Archer', aegisName: 'GOBLIN_ARCHER',
    level: 28, maxHealth: 1750, baseExp: 461, jobExp: 284, mvpExp: 0,
    attack: 89, attack2: 113, defense: 0, magicDefense: 0,
    str: 10, agi: 15, vit: 20, int: 15, dex: 72, luk: 20,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 16400,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 10, agi: 15, vit: 20, int: 15, dex: 72, luk: 20, level: 28, weaponATK: 89 },
    modes: {},
    drops: [
        { itemName: 'Goblin Mask', rate: 0.03 },
        { itemName: 'Iron', rate: 2.5 },
        { itemName: 'Scell', rate: 10 },
        { itemName: 'Oridecon Arrow', rate: 30 },
        { itemName: 'Red Herb', rate: 6 },
        { itemName: 'Composite Bow', rate: 0.25 },
        { itemName: 'Grape', rate: 3 },
        { itemName: 'Goblin Archer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Porcellio (ID: 1619) ──── Level 28 | HP 1,654 | NORMAL | insect/earth3 | passive
RO_MONSTER_TEMPLATES['porcellio'] = {
    id: 1619, name: 'Porcellio', aegisName: 'PORCELLIO',
    level: 28, maxHealth: 1654, baseExp: 512, jobExp: 346, mvpExp: 0,
    attack: 82, attack2: 247, defense: 0, magicDefense: 8,
    str: 0, agi: 31, vit: 21, int: 50, dex: 54, luk: 85,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 186, walkSpeed: 150, attackDelay: 720, attackMotion: 360, damageMotion: 360,
    size: 'small', race: 'insect', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 16400,
    raceGroups: {},
    stats: { str: 0, agi: 31, vit: 21, int: 50, dex: 54, luk: 85, level: 28, weaponATK: 82 },
    modes: { detector: true },
    drops: [
        { itemName: 'Jubilee', rate: 50 },
        { itemName: 'Main Gauche', rate: 0.25 },
        { itemName: 'Insect Feeler', rate: 10 },
        { itemName: 'Single Cell', rate: 30 },
        { itemName: 'Dew Laden Moss', rate: 0.02 },
        { itemName: 'Fluorescent Liquid', rate: 0.3 },
        { itemName: 'Porcellio Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Soldier Skeleton (ID: 1028) ──── Level 29 | HP 2,334 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['soldier_skeleton'] = {
    id: 1028, name: 'Soldier Skeleton', aegisName: 'SOLDIER_SKELETON',
    level: 29, maxHealth: 2334, baseExp: 372, jobExp: 226, mvpExp: 0,
    attack: 221, attack2: 245, defense: 10, magicDefense: 15,
    str: 0, agi: 15, vit: 22, int: 5, dex: 40, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 154, walkSpeed: 200, attackDelay: 2276, attackMotion: 576, damageMotion: 432,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16700,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 22, int: 5, dex: 40, luk: 15, level: 29, weaponATK: 221 },
    modes: {},
    drops: [
        { itemName: 'Skel-Bone', rate: 55 },
        { itemName: 'Rough Oridecon', rate: 0.6 },
        { itemName: 'Dagger', rate: 0.12 },
        { itemName: 'Red Herb', rate: 7 },
        { itemName: 'Memento', rate: 0.1 },
        { itemName: 'Knife', rate: 1.5 },
        { itemName: 'Stiletto', rate: 0.5 },
        { itemName: 'Soldier Skeleton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Giearth (ID: 1121) ──── Level 29 | HP 2,252 | NORMAL | demon/earth1 | aggressive
RO_MONSTER_TEMPLATES['giearth'] = {
    id: 1121, name: 'Giearth', aegisName: 'GIEARTH',
    level: 29, maxHealth: 2252, baseExp: 495, jobExp: 301, mvpExp: 0,
    attack: 154, attack2: 185, defense: 10, magicDefense: 50,
    str: 25, agi: 29, vit: 46, int: 60, dex: 64, luk: 105,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1848, attackMotion: 1296, damageMotion: 432,
    size: 'small', race: 'demon', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 16700,
    raceGroups: {},
    stats: { str: 25, agi: 29, vit: 46, int: 60, dex: 64, luk: 105, level: 29, weaponATK: 154 },
    modes: { detector: true },
    drops: [
        { itemName: 'Great Nature', rate: 0.3 },
        { itemName: 'Coal', rate: 1.5 },
        { itemName: 'Elder Pixie\'s Beard', rate: 55 },
        { itemName: 'Elven Ears', rate: 0.01 },
        { itemName: 'Cap', rate: 0.1 },
        { itemName: 'Star Dust', rate: 1 },
        { itemName: 'Giearth Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Munak (ID: 1026) ──── Level 30 | HP 2,872 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['munak'] = {
    id: 1026, name: 'Munak', aegisName: 'MUNAK',
    level: 30, maxHealth: 2872, baseExp: 601, jobExp: 318, mvpExp: 0,
    attack: 150, attack2: 230, defense: 0, magicDefense: 0,
    str: 0, agi: 15, vit: 20, int: 5, dex: 46, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 151, walkSpeed: 200, attackDelay: 2468, attackMotion: 768, damageMotion: 288,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 20, int: 5, dex: 46, luk: 15, level: 30, weaponATK: 150 },
    modes: {},
    drops: [
        { itemName: 'Daenggie', rate: 90 },
        { itemName: 'Munak Hat', rate: 0.02 },
        { itemName: 'Shoes', rate: 0.15 },
        { itemName: 'Amulet', rate: 0.2 },
        { itemName: 'Ninja Suit', rate: 0.01 },
        { itemName: 'Adventurer\'s Suit', rate: 1 },
        { itemName: 'Girl\'s Diary', rate: 0.05 },
        { itemName: 'Munak Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Swordfish (ID: 1069) ──── Level 30 | HP 4,299 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['sword_fish'] = {
    id: 1069, name: 'Swordfish', aegisName: 'SWORD_FISH',
    level: 30, maxHealth: 4299, baseExp: 1251, jobExp: 638, mvpExp: 0,
    attack: 168, attack2: 199, defense: 5, magicDefense: 20,
    str: 0, agi: 30, vit: 30, int: 41, dex: 62, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 200, attackDelay: 1968, attackMotion: 768, damageMotion: 384,
    size: 'large', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 30, int: 41, dex: 62, luk: 30, level: 30, weaponATK: 168 },
    modes: {},
    drops: [
        { itemName: 'Mystic Frozen', rate: 0.1 },
        { itemName: 'Sharp Scale', rate: 90 },
        { itemName: 'Rough Oridecon', rate: 0.33 },
        { itemName: 'Unicorn Horn', rate: 0.02 },
        { itemName: 'Rough Elunium', rate: 0.5 },
        { itemName: 'Katana', rate: 0.25 },
        { itemName: 'Gill', rate: 6 },
        { itemName: 'Swordfish Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Frilldora (ID: 1119) ──── Level 30 | HP 2,023 | NORMAL | brute/fire1 | aggressive
RO_MONSTER_TEMPLATES['frilldora'] = {
    id: 1119, name: 'Frilldora', aegisName: 'FRILLDORA',
    level: 30, maxHealth: 2023, baseExp: 529, jobExp: 319, mvpExp: 0,
    attack: 200, attack2: 239, defense: 0, magicDefense: 10,
    str: 35, agi: 30, vit: 38, int: 15, dex: 53, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1540, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 35, agi: 30, vit: 38, int: 15, dex: 53, luk: 30, level: 30, weaponATK: 200 },
    modes: {},
    drops: [
        { itemName: 'Frill', rate: 55 },
        { itemName: 'Rough Elunium', rate: 0.9 },
        { itemName: 'Reptile Tongue', rate: 15 },
        { itemName: 'Emerald', rate: 0.15 },
        { itemName: 'Yellow Gemstone', rate: 2 },
        { itemName: 'Red Herb', rate: 8 },
        { itemName: 'Zargon', rate: 1.2 },
        { itemName: 'Frilldora Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skeleton Worker (ID: 1169) ──── Level 30 | HP 2,872 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['skel_worker'] = {
    id: 1169, name: 'Skeleton Worker', aegisName: 'SKEL_WORKER',
    level: 30, maxHealth: 2872, baseExp: 397, jobExp: 240, mvpExp: 0,
    attack: 242, attack2: 288, defense: 0, magicDefense: 15,
    str: 0, agi: 15, vit: 30, int: 5, dex: 42, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 152, walkSpeed: 400, attackDelay: 2420, attackMotion: 720, damageMotion: 384,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 30, int: 5, dex: 42, luk: 10, level: 30, weaponATK: 242 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 4 },
        { itemName: 'Lantern', rate: 55 },
        { itemName: 'Rough Elunium', rate: 0.9 },
        { itemName: 'Safety Helmet', rate: 0.02 },
        { itemName: 'Steel', rate: 1 },
        { itemName: 'Coal', rate: 2 },
        { itemName: 'Iron Ore', rate: 8 },
        { itemName: 'Skeleton Worker Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sasquatch (ID: 1243) ──── Level 30 | HP 3,163 | NORMAL | brute/neutral3 | aggressive
RO_MONSTER_TEMPLATES['sasquatch'] = {
    id: 1243, name: 'Sasquatch', aegisName: 'SASQUATCH',
    level: 30, maxHealth: 3163, baseExp: 529, jobExp: 319, mvpExp: 0,
    attack: 250, attack2: 280, defense: 5, magicDefense: 0,
    str: 75, agi: 25, vit: 60, int: 10, dex: 34, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 300, attackDelay: 1260, attackMotion: 192, damageMotion: 192,
    size: 'large', race: 'brute', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 75, agi: 25, vit: 60, int: 10, dex: 34, luk: 20, level: 30, weaponATK: 250 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 7.5 },
        { itemName: 'White Herb', rate: 8 },
        { itemName: 'Feather', rate: 10 },
        { itemName: 'Panda Hat', rate: 0.01 },
        { itemName: 'Bear\'s Foot', rate: 50 },
        { itemName: 'Opal', rate: 0.3 },
        { itemName: 'Rough Elunium', rate: 0.9 },
        { itemName: 'Sasquatch Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Karakasa (ID: 1400) ──── Level 30 | HP 3,092 | NORMAL | formless/neutral3 | passive
RO_MONSTER_TEMPLATES['karakasa'] = {
    id: 1400, name: 'Karakasa', aegisName: 'KARAKASA',
    level: 30, maxHealth: 3092, baseExp: 489, jobExp: 322, mvpExp: 0,
    attack: 141, attack2: 183, defense: 1, magicDefense: 5,
    str: 0, agi: 45, vit: 12, int: 20, dex: 49, luk: 60,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 167, walkSpeed: 155, attackDelay: 1638, attackMotion: 2016, damageMotion: 576,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 17000,
    raceGroups: {},
    stats: { str: 0, agi: 45, vit: 12, int: 20, dex: 49, luk: 60, level: 30, weaponATK: 141 },
    modes: {},
    drops: [
        { itemName: 'Oil Paper', rate: 50 },
        { itemName: 'Piece of Bamboo', rate: 42.68 },
        { itemName: 'Wooden Block', rate: 32 },
        { itemName: 'Slick Paper', rate: 22 },
        { itemName: 'Zargon', rate: 40.74 },
        { itemName: 'Glass Bead', rate: 0.3 },
        { itemName: 'Murasame', rate: 0.05 },
        { itemName: 'Karakasa Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Archer Skeleton (ID: 1016) ──── Level 31 | HP 3,040 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['archer_skeleton'] = {
    id: 1016, name: 'Archer Skeleton', aegisName: 'ARCHER_SKELETON',
    level: 31, maxHealth: 3040, baseExp: 483, jobExp: 283, mvpExp: 0,
    attack: 128, attack2: 153, defense: 0, magicDefense: 0,
    str: 0, agi: 8, vit: 14, int: 5, dex: 90, luk: 5,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 143, walkSpeed: 300, attackDelay: 2864, attackMotion: 864, damageMotion: 576,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 0, agi: 8, vit: 14, int: 5, dex: 90, luk: 5, level: 31, weaponATK: 128 },
    modes: {},
    drops: [
        { itemName: 'Skel-Bone', rate: 45 },
        { itemName: 'Rough Oridecon', rate: 0.7 },
        { itemName: 'Apple of Archer', rate: 0.03 },
        { itemName: 'Great Bow', rate: 0.35 },
        { itemName: 'Fire Arrow', rate: 10 },
        { itemName: 'Red Herb', rate: 18 },
        { itemName: 'Bow', rate: 1.5 },
        { itemName: 'Archer Skeleton Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Obeaune (ID: 1044) ──── Level 31 | HP 3,952 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['obeaune'] = {
    id: 1044, name: 'Obeaune', aegisName: 'OBEAUNE',
    level: 31, maxHealth: 3952, baseExp: 644, jobExp: 407, mvpExp: 0,
    attack: 141, attack2: 165, defense: 0, magicDefense: 40,
    str: 0, agi: 31, vit: 31, int: 55, dex: 74, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1872, attackMotion: 672, damageMotion: 288,
    size: 'medium', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 0, agi: 31, vit: 31, int: 55, dex: 74, luk: 85, level: 31, weaponATK: 141 },
    modes: {},
    drops: [
        { itemName: 'Mystic Frozen', rate: 0.13 },
        { itemName: 'Heart of Mermaid', rate: 90 },
        { itemName: 'Fin Helm', rate: 0.01 },
        { itemName: 'Saint\'s Robe', rate: 0.1 },
        { itemName: 'Aquamarine', rate: 0.1 },
        { itemName: 'Fin', rate: 5 },
        { itemName: 'Witherless Rose', rate: 0.3 },
        { itemName: 'Obeaune Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kobold (ID: 1134) ──── Level 31 | HP 2,179 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['kobold_2'] = {
    id: 1134, name: 'Kobold', aegisName: 'KOBOLD_2',
    level: 31, maxHealth: 2179, baseExp: 806, jobExp: 407, mvpExp: 0,
    attack: 262, attack2: 324, defense: 15, magicDefense: 10,
    str: 0, agi: 31, vit: 31, int: 20, dex: 46, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18200,
    raceGroups: { Kobold: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 31, vit: 31, int: 20, dex: 46, luk: 20, level: 31, weaponATK: 262 },
    modes: {},
    drops: [
        { itemName: 'Steel', rate: 1 },
        { itemName: 'Blue Hair', rate: 53.35 },
        { itemName: 'Zargon', rate: 2 },
        { itemName: 'Buckler', rate: 0.03 },
        { itemName: 'Yellow Herb', rate: 1 },
        { itemName: 'Guard', rate: 1 },
        { itemName: 'Kobold Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kobold (ID: 1135) ──── Level 31 | HP 2,179 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['kobold_3'] = {
    id: 1135, name: 'Kobold', aegisName: 'KOBOLD_3',
    level: 31, maxHealth: 2179, baseExp: 644, jobExp: 407, mvpExp: 0,
    attack: 186, attack2: 216, defense: 15, magicDefense: 10,
    str: 0, agi: 31, vit: 31, int: 20, dex: 88, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18200,
    raceGroups: { Kobold: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 31, vit: 31, int: 20, dex: 88, luk: 20, level: 31, weaponATK: 186 },
    modes: {},
    drops: [
        { itemName: 'Red Blood', rate: 0.35 },
        { itemName: 'Steel', rate: 1 },
        { itemName: 'Blue Hair', rate: 53.35 },
        { itemName: 'Zargon', rate: 2 },
        { itemName: 'Hammer', rate: 0.05 },
        { itemName: 'Buckler', rate: 0.03 },
        { itemName: 'Yellow Herb', rate: 1 },
        { itemName: 'Kobold Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marse (ID: 1144) ──── Level 31 | HP 5,034 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['marse'] = {
    id: 1144, name: 'Marse', aegisName: 'MARSE',
    level: 31, maxHealth: 5034, baseExp: 586, jobExp: 370, mvpExp: 0,
    attack: 211, attack2: 252, defense: 0, magicDefense: 5,
    str: 0, agi: 31, vit: 25, int: 5, dex: 52, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 300, attackDelay: 1956, attackMotion: 756, damageMotion: 528,
    size: 'small', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 0, agi: 31, vit: 25, int: 5, dex: 52, luk: 30, level: 31, weaponATK: 211 },
    modes: {},
    drops: [
        { itemName: 'Squid Ink', rate: 90 },
        { itemName: 'Tentacle', rate: 30 },
        { itemName: 'Blue Gemstone', rate: 2 },
        { itemName: 'Aquamarine', rate: 0.1 },
        { itemName: 'Mystic Frozen', rate: 0.12 },
        { itemName: 'Necklace of Wisdom', rate: 0.05 },
        { itemName: 'Grape', rate: 3 },
        { itemName: 'Marse Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Matyr (ID: 1146) ──── Level 31 | HP 2,585 | NORMAL | brute/shadow1 | aggressive
RO_MONSTER_TEMPLATES['matyr'] = {
    id: 1146, name: 'Matyr', aegisName: 'MATYR',
    level: 31, maxHealth: 2585, baseExp: 967, jobExp: 407, mvpExp: 0,
    attack: 134, attack2: 160, defense: 0, magicDefense: 0,
    str: 0, agi: 47, vit: 38, int: 5, dex: 64, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 432, attackMotion: 432, damageMotion: 360,
    size: 'medium', race: 'brute', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 0, agi: 47, vit: 38, int: 5, dex: 64, luk: 5, level: 31, weaponATK: 134 },
    modes: {},
    drops: [
        { itemName: 'Matyr\'s Flea Guard', rate: 0.1 },
        { itemName: 'Monster\'s Feed', rate: 50 },
        { itemName: 'Animal\'s Skin', rate: 55 },
        { itemName: 'Pet Food', rate: 4 },
        { itemName: 'Rough Elunium', rate: 1 },
        { itemName: 'Grape', rate: 2 },
        { itemName: 'Matyr Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zenorc (ID: 1177) ──── Level 31 | HP 2,585 | NORMAL | demihuman/shadow1 | passive
RO_MONSTER_TEMPLATES['zenorc'] = {
    id: 1177, name: 'Zenorc', aegisName: 'ZENORC',
    level: 31, maxHealth: 2585, baseExp: 967, jobExp: 407, mvpExp: 0,
    attack: 188, attack2: 223, defense: 0, magicDefense: 15,
    str: 0, agi: 77, vit: 15, int: 0, dex: 76, luk: 10,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 176, walkSpeed: 150, attackDelay: 1180, attackMotion: 480, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 0, agi: 77, vit: 15, int: 0, dex: 76, luk: 10, level: 31, weaponATK: 188 },
    modes: {},
    drops: [
        { itemName: 'Zenorc\'s Fang', rate: 55 },
        { itemName: 'Rough Oridecon', rate: 0.7 },
        { itemName: 'Sticky Mucus', rate: 25 },
        { itemName: 'Old Magicbook', rate: 0.05 },
        { itemName: 'Yellow Herb', rate: 1 },
        { itemName: 'Shining Stone', rate: 0.2 },
        { itemName: 'Zenorc Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Lady (ID: 1273) ──── Level 31 | HP 2,000 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['orc_lady'] = {
    id: 1273, name: 'Orc Lady', aegisName: 'ORC_LADY',
    level: 31, maxHealth: 2000, baseExp: 644, jobExp: 407, mvpExp: 0,
    attack: 135, attack2: 170, defense: 10, magicDefense: 10,
    str: 35, agi: 42, vit: 25, int: 15, dex: 69, luk: 55,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18200,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 35, agi: 42, vit: 25, int: 15, dex: 69, luk: 55, level: 31, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 46.56 },
        { itemName: 'Iron', rate: 3 },
        { itemName: 'Earring', rate: 0.01 },
        { itemName: 'Wedding Veil', rate: 0.01 },
        { itemName: 'Professional Cooking Kit', rate: 0.1 },
        { itemName: 'Level 6 Cookbook', rate: 0.03 },
        { itemName: 'Wedding Dress', rate: 0.01 },
        { itemName: 'Orc Lady Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deviling (ID: 1582) ──── Level 31 | HP 64,500 | BOSS | demon/shadow4 | aggressive
RO_MONSTER_TEMPLATES['deviling'] = {
    id: 1582, name: 'Deviling', aegisName: 'DEVILING',
    level: 31, maxHealth: 64500, baseExp: 211, jobExp: 412, mvpExp: 0,
    attack: 135, attack2: 270, defense: 5, magicDefense: 70,
    str: 30, agi: 50, vit: 20, int: 75, dex: 77, luk: 200,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1072, attackMotion: 1056, damageMotion: 384,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 30, agi: 50, vit: 20, int: 75, dex: 77, luk: 200, level: 31, weaponATK: 135 },
    modes: {},
    drops: [
        { itemName: 'Little Evil Wing', rate: 30 },
        { itemName: 'Zargon', rate: 48.5 },
        { itemName: 'Evil Wing', rate: 1 },
        { itemName: 'Apple', rate: 50 },
        { itemName: 'Blade Lost in Darkness', rate: 0.01 },
        { itemName: 'Black Dyestuffs', rate: 1 },
        { itemName: 'Level 3 Soul Strike', rate: 1 },
        { itemName: 'Deviling Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Roween (ID: 1782) ──── Level 31 | HP 5,716 | NORMAL | brute/wind1 | aggressive
RO_MONSTER_TEMPLATES['roween'] = {
    id: 1782, name: 'Roween', aegisName: 'ROWEEN',
    level: 31, maxHealth: 5716, baseExp: 1669, jobExp: 1266, mvpExp: 0,
    attack: 298, attack2: 377, defense: 0, magicDefense: 7,
    str: 51, agi: 39, vit: 48, int: 18, dex: 67, luk: 19,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 412, attackMotion: 840, damageMotion: 300,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18200,
    raceGroups: {},
    stats: { str: 51, agi: 39, vit: 48, int: 18, dex: 67, luk: 19, level: 31, weaponATK: 298 },
    modes: {},
    drops: [
        { itemName: 'Rotten Meat', rate: 30 },
        { itemName: 'Animal\'s Skin', rate: 30 },
        { itemName: 'Wind of Verdure', rate: 0.5 },
        { itemName: 'Combo Battle Glove', rate: 0.02 },
        { itemName: 'Roween Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bongun (ID: 1188) ──── Level 32 | HP 3,520 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['bon_gun'] = {
    id: 1188, name: 'Bongun', aegisName: 'BON_GUN',
    level: 32, maxHealth: 3520, baseExp: 424, jobExp: 242, mvpExp: 0,
    attack: 220, attack2: 260, defense: 0, magicDefense: 0,
    str: 45, agi: 15, vit: 36, int: 10, dex: 48, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 200, attackDelay: 1720, attackMotion: 500, damageMotion: 420,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18400,
    raceGroups: {},
    stats: { str: 45, agi: 15, vit: 36, int: 10, dex: 48, luk: 15, level: 32, weaponATK: 220 },
    modes: {},
    drops: [
        { itemName: 'Short Daenggie', rate: 55 },
        { itemName: 'Old Portrait', rate: 0.4 },
        { itemName: 'Worn Out Scroll', rate: 0.6 },
        { itemName: 'Bongun Hat', rate: 0.01 },
        { itemName: 'Amulet', rate: 0.15 },
        { itemName: 'Yellow Herb', rate: 10 },
        { itemName: 'Yellow Herb', rate: 12.5 },
        { itemName: 'Bongun Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Tri Joint (ID: 1279) ──── Level 32 | HP 2,300 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['tri_joint'] = {
    id: 1279, name: 'Tri Joint', aegisName: 'TRI_JOINT',
    level: 32, maxHealth: 2300, baseExp: 386, jobExp: 220, mvpExp: 0,
    attack: 178, attack2: 206, defense: 20, magicDefense: 5,
    str: 0, agi: 48, vit: 24, int: 10, dex: 67, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 200, attackDelay: 860, attackMotion: 660, damageMotion: 624,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18400,
    raceGroups: {},
    stats: { str: 0, agi: 48, vit: 24, int: 10, dex: 67, luk: 20, level: 32, weaponATK: 178 },
    modes: { detector: true },
    drops: [
        { itemName: 'Cyfar', rate: 1 },
        { itemName: 'Solid Shell', rate: 3.8 },
        { itemName: 'Aloevera', rate: 2 },
        { itemName: 'Green Live', rate: 1.6 },
        { itemName: 'Star Dust', rate: 1.4 },
        { itemName: 'Rough Elunium', rate: 1.06 },
        { itemName: 'Tri Joint Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baby Leopard (ID: 1415) ──── Level 32 | HP 2,590 | NORMAL | brute/ghost1 | aggressive
RO_MONSTER_TEMPLATES['baby_leopard'] = {
    id: 1415, name: 'Baby Leopard', aegisName: 'BABY_LEOPARD',
    level: 32, maxHealth: 2590, baseExp: 352, jobExp: 201, mvpExp: 0,
    attack: 155, attack2: 207, defense: 0, magicDefense: 5,
    str: 20, agi: 44, vit: 20, int: 4, dex: 49, luk: 10,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 318, attackMotion: 528, damageMotion: 420,
    size: 'small', race: 'brute', element: { type: 'ghost', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18400,
    raceGroups: {},
    stats: { str: 20, agi: 44, vit: 20, int: 4, dex: 49, luk: 10, level: 32, weaponATK: 155 },
    modes: {},
    drops: [
        { itemName: 'Leopard Skin', rate: 52 },
        { itemName: 'Leopard Claw', rate: 32 },
        { itemName: 'Rough Oridecon', rate: 1.5 },
        { itemName: 'Meat', rate: 20 },
        { itemName: 'Dagger', rate: 1 },
        { itemName: 'Pet Food', rate: 5 },
        { itemName: 'Baby Leopard Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dokebi (ID: 1110) ──── Level 33 | HP 2,697 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['dokebi'] = {
    id: 1110, name: 'Dokebi', aegisName: 'DOKEBI',
    level: 33, maxHealth: 2697, baseExp: 889, jobExp: 455, mvpExp: 0,
    attack: 197, attack2: 249, defense: 0, magicDefense: 10,
    str: 50, agi: 50, vit: 40, int: 35, dex: 69, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 250, attackDelay: 1156, attackMotion: 456, damageMotion: 384,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18600,
    raceGroups: {},
    stats: { str: 50, agi: 50, vit: 40, int: 35, dex: 69, luk: 40, level: 33, weaponATK: 197 },
    modes: { detector: true },
    drops: [
        { itemName: 'Dokebi Horn', rate: 90 },
        { itemName: 'Rough Elunium', rate: 1.5 },
        { itemName: 'Sword Mace', rate: 0.02 },
        { itemName: 'Mighty Staff', rate: 0.01 },
        { itemName: 'Gold', rate: 0.01 },
        { itemName: 'Club', rate: 3 },
        { itemName: 'Hammer of Blacksmith', rate: 0.05 },
        { itemName: 'Dokebi Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sohee (ID: 1170) ──── Level 33 | HP 5,628 | NORMAL | demon/water1 | aggressive
RO_MONSTER_TEMPLATES['sohee'] = {
    id: 1170, name: 'Sohee', aegisName: 'SOHEE',
    level: 33, maxHealth: 5628, baseExp: 739, jobExp: 455, mvpExp: 0,
    attack: 210, attack2: 251, defense: 0, magicDefense: 10,
    str: 0, agi: 33, vit: 33, int: 10, dex: 58, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 158, walkSpeed: 250, attackDelay: 2112, attackMotion: 912, damageMotion: 576,
    size: 'medium', race: 'demon', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18600,
    raceGroups: {},
    stats: { str: 0, agi: 33, vit: 33, int: 10, dex: 58, luk: 15, level: 33, weaponATK: 210 },
    modes: { detector: true },
    drops: [
        { itemName: 'Black Hair', rate: 90 },
        { itemName: 'Skirt of Virgin', rate: 0.5 },
        { itemName: 'Nurse Cap', rate: 0.01 },
        { itemName: 'Muffler', rate: 0.05 },
        { itemName: 'Stiletto', rate: 0.05 },
        { itemName: 'Red Herb', rate: 10 },
        { itemName: 'Authoritative Badge', rate: 3.5 },
        { itemName: 'Sohee Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kobold Archer (ID: 1282) ──── Level 33 | HP 2,560 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['kobold_archer'] = {
    id: 1282, name: 'Kobold Archer', aegisName: 'KOBOLD_ARCHER',
    level: 33, maxHealth: 2560, baseExp: 739, jobExp: 455, mvpExp: 0,
    attack: 155, attack2: 185, defense: 10, magicDefense: 5,
    str: 10, agi: 20, vit: 15, int: 30, dex: 100, luk: 25,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18600,
    raceGroups: { Kobold: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 10, agi: 20, vit: 15, int: 30, dex: 100, luk: 25, level: 33, weaponATK: 155 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 2.5 },
        { itemName: 'Steel', rate: 0.6 },
        { itemName: 'Blue Hair', rate: 48.5 },
        { itemName: 'Puppy Headband', rate: 0.5 },
        { itemName: 'Poison Arrow', rate: 20 },
        { itemName: 'Crossbow', rate: 0.05 },
        { itemName: 'Rough Oridecon', rate: 0.79 },
        { itemName: 'Kobold Archer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Miyabi Doll (ID: 1404) ──── Level 33 | HP 6,300 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['miyabi_ningyo'] = {
    id: 1404, name: 'Miyabi Doll', aegisName: 'MIYABI_NINGYO',
    level: 33, maxHealth: 6300, baseExp: 795, jobExp: 453, mvpExp: 0,
    attack: 250, attack2: 305, defense: 1, magicDefense: 20,
    str: 0, agi: 52, vit: 15, int: 10, dex: 62, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 250, attackDelay: 1938, attackMotion: 2112, damageMotion: 768,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18600,
    raceGroups: {},
    stats: { str: 0, agi: 52, vit: 15, int: 10, dex: 62, luk: 15, level: 33, weaponATK: 250 },
    modes: { detector: true },
    drops: [
        { itemName: 'Glossy Hair', rate: 53.35 },
        { itemName: 'Worn-out Kimono', rate: 25 },
        { itemName: 'White Herb', rate: 15.5 },
        { itemName: 'Star Crumb', rate: 12.5 },
        { itemName: 'Professional Cooking Kit', rate: 0.1 },
        { itemName: 'Hakujin', rate: 0.05 },
        { itemName: 'Mandolin', rate: 0.02 },
        { itemName: 'Miyabi Doll Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Horong (ID: 1129) ──── Level 34 | HP 1,939 | NORMAL | formless/fire4 | aggressive
RO_MONSTER_TEMPLATES['horong'] = {
    id: 1129, name: 'Horong', aegisName: 'HORONG',
    level: 34, maxHealth: 1939, baseExp: 786, jobExp: 479, mvpExp: 0,
    attack: 275, attack2: 327, defense: 99, magicDefense: 50,
    str: 0, agi: 34, vit: 10, int: 0, dex: 50, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 162, walkSpeed: 400, attackDelay: 1888, attackMotion: 1152, damageMotion: 828,
    size: 'small', race: 'formless', element: { type: 'fire', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 0, agi: 34, vit: 10, int: 0, dex: 50, luk: 0, level: 34, weaponATK: 275 },
    modes: {},
    drops: [
        { itemName: 'Stone Heart', rate: 65 },
        { itemName: 'Zargon', rate: 5 },
        { itemName: 'Bomb Wick', rate: 0.05 },
        { itemName: 'Fire Arrow', rate: 100 },
        { itemName: 'Rough Elunium', rate: 1.18 },
        { itemName: 'Sweet Potato', rate: 0.2 },
        { itemName: 'Alcohol', rate: 0.5 },
        { itemName: 'Horong Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sandman (ID: 1165) ──── Level 34 | HP 3,413 | NORMAL | formless/earth3 | aggressive
RO_MONSTER_TEMPLATES['sand_man'] = {
    id: 1165, name: 'Sandman', aegisName: 'SAND_MAN',
    level: 34, maxHealth: 3413, baseExp: 810, jobExp: 492, mvpExp: 0,
    attack: 180, attack2: 205, defense: 10, magicDefense: 25,
    str: 24, agi: 34, vit: 58, int: 38, dex: 60, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 250, attackDelay: 1672, attackMotion: 720, damageMotion: 288,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 24, agi: 34, vit: 58, int: 38, dex: 60, luk: 5, level: 34, weaponATK: 180 },
    modes: {},
    drops: [
        { itemName: 'Great Nature', rate: 0.35 },
        { itemName: 'Grit', rate: 53.35 },
        { itemName: 'Rough Elunium', rate: 1.18 },
        { itemName: 'Fine Sand', rate: 3.5 },
        { itemName: 'Star Dust', rate: 2 },
        { itemName: 'Katar of Quaking', rate: 0.01 },
        { itemName: 'Hypnotist\'s Staff', rate: 0.05 },
        { itemName: 'Sandman Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Whisper (ID: 1179) ──── Level 34 | HP 1,796 | NORMAL | demon/ghost3 | aggressive
RO_MONSTER_TEMPLATES['whisper'] = {
    id: 1179, name: 'Whisper', aegisName: 'WHISPER',
    level: 34, maxHealth: 1796, baseExp: 591, jobExp: 599, mvpExp: 0,
    attack: 180, attack2: 221, defense: 0, magicDefense: 45,
    str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 960, damageMotion: 504,
    size: 'small', race: 'demon', element: { type: 'ghost', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0, level: 34, weaponATK: 180 },
    modes: { detector: true },
    drops: [
        { itemName: 'Star Dust', rate: 1.5 },
        { itemName: 'Fabric', rate: 53.35 },
        { itemName: 'Halo', rate: 0.01 },
        { itemName: 'Silver Robe', rate: 0.1 },
        { itemName: 'Whisper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Whisper (ID: 1185) ──── Level 34 | HP 1,796 | NORMAL | undead/ghost1 | passive
RO_MONSTER_TEMPLATES['whisper_'] = {
    id: 1185, name: 'Whisper', aegisName: 'WHISPER_',
    level: 34, maxHealth: 1796, baseExp: 537, jobExp: 545, mvpExp: 0,
    attack: 198, attack2: 239, defense: 0, magicDefense: 45,
    str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 960, damageMotion: 504,
    size: 'small', race: 'undead', element: { type: 'ghost', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0, level: 34, weaponATK: 198 },
    modes: {},
    drops: [
        { itemName: 'Star Dust', rate: 0.1 },
        { itemName: 'Fabric', rate: 1 },
        { itemName: 'Silver Robe', rate: 0.01 },
    ],
    mvpDrops: [],
};

// ──── Giant Whisper (ID: 1186) ──── Level 34 | HP 5,040 | NORMAL | demon/ghost2 | aggressive
RO_MONSTER_TEMPLATES['whisper_boss'] = {
    id: 1186, name: 'Giant Whisper', aegisName: 'WHISPER_BOSS',
    level: 34, maxHealth: 5040, baseExp: 537, jobExp: 545, mvpExp: 0,
    attack: 198, attack2: 239, defense: 0, magicDefense: 45,
    str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 149, walkSpeed: 250, attackDelay: 2536, attackMotion: 1536, damageMotion: 672,
    size: 'small', race: 'demon', element: { type: 'ghost', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 0, agi: 51, vit: 14, int: 0, dex: 60, luk: 0, level: 34, weaponATK: 198 },
    modes: { detector: true },
    drops: [
        { itemName: 'Star Dust', rate: 1.5 },
        { itemName: 'Fabric', rate: 53.35 },
        { itemName: 'Halo', rate: 0.01 },
        { itemName: 'Silver Robe', rate: 0.1 },
        { itemName: 'Giant Whisper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Beetle King (ID: 1494) ──── Level 34 | HP 1,874 | NORMAL | insect/earth1 | reactive
RO_MONSTER_TEMPLATES['kind_of_beetle'] = {
    id: 1494, name: 'Beetle King', aegisName: 'KIND_OF_BEETLE',
    level: 34, maxHealth: 1874, baseExp: 679, jobExp: 442, mvpExp: 0,
    attack: 191, attack2: 243, defense: 45, magicDefense: 12,
    str: 0, agi: 34, vit: 10, int: 0, dex: 40, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 165, attackDelay: 1247, attackMotion: 768, damageMotion: 576,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 18800,
    raceGroups: {},
    stats: { str: 0, agi: 34, vit: 10, int: 0, dex: 40, luk: 0, level: 34, weaponATK: 191 },
    modes: { detector: true },
    drops: [
        { itemName: 'Solid Husk', rate: 65 },
        { itemName: 'Pincher of Beetle', rate: 45 },
        { itemName: 'Insect Feeler', rate: 10 },
        { itemName: 'Worm Peeling', rate: 5 },
        { itemName: 'Guard', rate: 0.01 },
        { itemName: 'Beetle King Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Requiem (ID: 1164) ──── Level 35 | HP 3,089 | NORMAL | demihuman/shadow1 | aggressive
RO_MONSTER_TEMPLATES['requiem'] = {
    id: 1164, name: 'Requiem', aegisName: 'REQUIEM',
    level: 35, maxHealth: 3089, baseExp: 800, jobExp: 458, mvpExp: 0,
    attack: 220, attack2: 272, defense: 0, magicDefense: 15,
    str: 0, agi: 53, vit: 35, int: 5, dex: 57, luk: 2,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 400, attackDelay: 1516, attackMotion: 816, damageMotion: 432,
    size: 'medium', race: 'demihuman', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19000,
    raceGroups: {},
    stats: { str: 0, agi: 53, vit: 35, int: 5, dex: 57, luk: 2, level: 35, weaponATK: 220 },
    modes: {},
    drops: [
        { itemName: 'Old Blue Box', rate: 0.35 },
        { itemName: 'Emperium', rate: 0.01 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Horrendous Mouth', rate: 35 },
        { itemName: 'Memento', rate: 15 },
        { itemName: 'Mantle', rate: 0.1 },
        { itemName: 'Level 6 Cookbook', rate: 0.01 },
        { itemName: 'Requiem Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cruiser (ID: 1248) ──── Level 35 | HP 2,820 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['cruiser'] = {
    id: 1248, name: 'Cruiser', aegisName: 'CRUISER',
    level: 35, maxHealth: 2820, baseExp: 1100, jobExp: 450, mvpExp: 0,
    attack: 175, attack2: 215, defense: 5, magicDefense: 5,
    str: 0, agi: 40, vit: 10, int: 10, dex: 90, luk: 25,
    attackRange: 350, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 400, attackDelay: 1296, attackMotion: 1296, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19000,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 10, int: 10, dex: 90, luk: 25, level: 35, weaponATK: 175 },
    modes: {},
    drops: [
        { itemName: 'Manacles', rate: 9 },
        { itemName: 'Monk Hat', rate: 0.02 },
        { itemName: 'Iron', rate: 3.2 },
        { itemName: 'Rough Wind', rate: 0.05 },
        { itemName: 'Scell', rate: 35 },
        { itemName: 'Branch', rate: 0.05 },
        { itemName: 'Rough Oridecon', rate: 0.87 },
        { itemName: 'Cruiser Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin Steamrider (ID: 1280) ──── Level 35 | HP 2,490 | NORMAL | demihuman/wind2 | aggressive
RO_MONSTER_TEMPLATES['steam_goblin'] = {
    id: 1280, name: 'Goblin Steamrider', aegisName: 'STEAM_GOBLIN',
    level: 35, maxHealth: 2490, baseExp: 864, jobExp: 495, mvpExp: 0,
    attack: 234, attack2: 269, defense: 20, magicDefense: 5,
    str: 58, agi: 59, vit: 32, int: 15, dex: 75, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 1008, attackMotion: 1008, damageMotion: 528,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19000,
    raceGroups: {},
    stats: { str: 58, agi: 59, vit: 32, int: 15, dex: 75, luk: 25, level: 35, weaponATK: 234 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 25 },
        { itemName: 'Cyfar', rate: 38.8 },
        { itemName: 'Iron', rate: 3 },
        { itemName: 'Steel', rate: 0.55 },
        { itemName: 'Coal', rate: 3.2 },
        { itemName: 'Garrison', rate: 0.05 },
        { itemName: 'Rough Elunium', rate: 1.24 },
        { itemName: 'Goblin Steamrider Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zipper Bear (ID: 1417) ──── Level 35 | HP 2,901 | NORMAL | brute/shadow1 | aggressive
RO_MONSTER_TEMPLATES['zipper_bear'] = {
    id: 1417, name: 'Zipper Bear', aegisName: 'ZIPPER_BEAR',
    level: 35, maxHealth: 2901, baseExp: 370, jobExp: 225, mvpExp: 0,
    attack: 248, attack2: 289, defense: 10, magicDefense: 5,
    str: 30, agi: 25, vit: 55, int: 15, dex: 28, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 155, attackDelay: 780, attackMotion: 1008, damageMotion: 420,
    size: 'medium', race: 'brute', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19000,
    raceGroups: {},
    stats: { str: 30, agi: 25, vit: 55, int: 15, dex: 28, luk: 25, level: 35, weaponATK: 248 },
    modes: {},
    drops: [
        { itemName: 'Black Bear\'s Skin', rate: 44.62 },
        { itemName: 'Strange Steel Piece', rate: 35 },
        { itemName: 'Royal Jelly', rate: 4 },
        { itemName: 'Honey', rate: 9 },
        { itemName: 'Apple', rate: 0.9 },
        { itemName: 'Zipper Bear Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Noxious (ID: 1620) ──── Level 35 | HP 2,038 | NORMAL | formless/ghost3 | aggressive
RO_MONSTER_TEMPLATES['noxious'] = {
    id: 1620, name: 'Noxious', aegisName: 'NOXIOUS',
    level: 35, maxHealth: 2038, baseExp: 698, jobExp: 698, mvpExp: 0,
    attack: 299, attack2: 400, defense: 0, magicDefense: 60,
    str: 12, agi: 41, vit: 10, int: 30, dex: 44, luk: 2,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 350, attackDelay: 768, attackMotion: 1440, damageMotion: 672,
    size: 'medium', race: 'formless', element: { type: 'ghost', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19000,
    raceGroups: {},
    stats: { str: 12, agi: 41, vit: 10, int: 30, dex: 44, luk: 2, level: 35, weaponATK: 299 },
    modes: {},
    drops: [
        { itemName: 'Toxic Gas', rate: 10 },
        { itemName: 'Mould Powder', rate: 30 },
        { itemName: 'Anodyne', rate: 0.5 },
        { itemName: 'Dust Pollutant', rate: 30 },
        { itemName: 'Old Blue Box', rate: 0.01 },
        { itemName: 'Noxious Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marc (ID: 1045) ──── Level 36 | HP 6,900 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['marc'] = {
    id: 1045, name: 'Marc', aegisName: 'MARC',
    level: 36, maxHealth: 6900, baseExp: 988, jobExp: 625, mvpExp: 0,
    attack: 220, attack2: 280, defense: 5, magicDefense: 10,
    str: 0, agi: 36, vit: 36, int: 20, dex: 56, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 150, attackDelay: 1272, attackMotion: 72, damageMotion: 480,
    size: 'medium', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19200,
    raceGroups: {},
    stats: { str: 0, agi: 36, vit: 36, int: 20, dex: 56, luk: 30, level: 36, weaponATK: 220 },
    modes: {},
    drops: [
        { itemName: 'Mystic Frozen', rate: 0.18 },
        { itemName: 'Gill', rate: 90 },
        { itemName: 'Rough Oridecon', rate: 0.95 },
        { itemName: 'Fin', rate: 10 },
        { itemName: 'Aquamarine', rate: 0.1 },
        { itemName: 'Blue Gemstone', rate: 2 },
        { itemName: 'White Herb', rate: 7 },
        { itemName: 'Marc Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kobold (ID: 1133) ──── Level 36 | HP 3,893 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['kobold_1'] = {
    id: 1133, name: 'Kobold', aegisName: 'KOBOLD_1',
    level: 36, maxHealth: 3893, baseExp: 988, jobExp: 625, mvpExp: 0,
    attack: 265, attack2: 318, defense: 15, magicDefense: 10,
    str: 0, agi: 90, vit: 36, int: 30, dex: 52, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19200,
    raceGroups: { Kobold: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 90, vit: 36, int: 30, dex: 52, luk: 20, level: 36, weaponATK: 265 },
    modes: {},
    drops: [
        { itemName: 'Steel', rate: 1 },
        { itemName: 'Blue Hair', rate: 53.35 },
        { itemName: 'Zargon', rate: 7 },
        { itemName: 'Rough Elunium', rate: 0.25 },
        { itemName: 'Gladius', rate: 0.02 },
        { itemName: 'Buckler', rate: 0.05 },
        { itemName: 'Kobold Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Lude (ID: 1509) ──── Level 36 | HP 3,214 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['lude'] = {
    id: 1509, name: 'Lude', aegisName: 'LUDE',
    level: 36, maxHealth: 3214, baseExp: 392, jobExp: 247, mvpExp: 0,
    attack: 287, attack2: 451, defense: 14, magicDefense: 10,
    str: 0, agi: 59, vit: 21, int: 18, dex: 36, luk: 21,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 150, attackDelay: 890, attackMotion: 960, damageMotion: 480,
    size: 'small', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19200,
    raceGroups: {},
    stats: { str: 0, agi: 59, vit: 21, int: 18, dex: 36, luk: 21, level: 36, weaponATK: 287 },
    modes: {},
    drops: [
        { itemName: 'Pumpkin Lantern', rate: 32 },
        { itemName: 'Ectoplasm', rate: 57.23 },
        { itemName: 'Fabric', rate: 10 },
        { itemName: 'Halo', rate: 0.1 },
        { itemName: 'Rough Elunium', rate: 0.1 },
        { itemName: 'Level 3 Heal', rate: 1 },
        { itemName: 'Lude Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Holden (ID: 1628) ──── Level 36 | HP 2,209 | NORMAL | brute/earth2 | reactive
RO_MONSTER_TEMPLATES['mole'] = {
    id: 1628, name: 'Holden', aegisName: 'MOLE',
    level: 36, maxHealth: 2209, baseExp: 268, jobExp: 172, mvpExp: 0,
    attack: 52, attack2: 63, defense: 0, magicDefense: 5,
    str: 24, agi: 18, vit: 23, int: 30, dex: 45, luk: 5,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 300, attackDelay: 140, attackMotion: 960, damageMotion: 504,
    size: 'small', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 19200,
    raceGroups: {},
    stats: { str: 24, agi: 18, vit: 23, int: 30, dex: 45, luk: 5, level: 36, weaponATK: 52 },
    modes: {},
    drops: [
        { itemName: 'Mole Whiskers', rate: 50 },
        { itemName: 'Mole Claw', rate: 50 },
        { itemName: 'Super Novice Hat', rate: 0.5 },
        { itemName: 'Six Shooter', rate: 0.05 },
        { itemName: 'Holden Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mummy (ID: 1041) ──── Level 37 | HP 5,176 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['mummy'] = {
    id: 1041, name: 'Mummy', aegisName: 'MUMMY',
    level: 37, maxHealth: 5176, baseExp: 800, jobExp: 602, mvpExp: 0,
    attack: 305, attack2: 360, defense: 0, magicDefense: 10,
    str: 28, agi: 19, vit: 32, int: 0, dex: 63, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 300, attackDelay: 1772, attackMotion: 72, damageMotion: 384,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19400,
    raceGroups: {},
    stats: { str: 28, agi: 19, vit: 32, int: 0, dex: 63, luk: 20, level: 37, weaponATK: 305 },
    modes: {},
    drops: [
        { itemName: 'Rotten Bandage', rate: 90 },
        { itemName: 'Rough Oridecon', rate: 1 },
        { itemName: 'Memento', rate: 5.5 },
        { itemName: 'Glove', rate: 0.01 },
        { itemName: 'Silver Ring', rate: 0.1 },
        { itemName: 'Panacea', rate: 2.5 },
        { itemName: 'Yellow Herb', rate: 8.5 },
        { itemName: 'Mummy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Verit (ID: 1032) ──── Level 38 | HP 5,272 | NORMAL | undead/undead1 | passive
RO_MONSTER_TEMPLATES['verit'] = {
    id: 1032, name: 'Verit', aegisName: 'VERIT',
    level: 38, maxHealth: 5272, baseExp: 835, jobExp: 517, mvpExp: 0,
    attack: 389, attack2: 469, defense: 0, magicDefense: 5,
    str: 0, agi: 19, vit: 38, int: 0, dex: 38, luk: 20,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 151, walkSpeed: 250, attackDelay: 2468, attackMotion: 768, damageMotion: 480,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 0, agi: 19, vit: 38, int: 0, dex: 38, luk: 20, level: 38, weaponATK: 389 },
    modes: {},
    drops: [
        { itemName: 'Immortal Heart', rate: 90 },
        { itemName: 'Zargon', rate: 7 },
        { itemName: 'Rotten Bandage', rate: 11 },
        { itemName: 'White Herb', rate: 6 },
        { itemName: 'Skull Ring', rate: 0.01 },
        { itemName: 'Flower Ring', rate: 2 },
        { itemName: 'Armlet of Obedience', rate: 0.2 },
        { itemName: 'Verit Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Jakk (ID: 1130) ──── Level 38 | HP 3,581 | NORMAL | formless/fire2 | aggressive
RO_MONSTER_TEMPLATES['jakk'] = {
    id: 1130, name: 'Jakk', aegisName: 'JAKK',
    level: 38, maxHealth: 3581, baseExp: 1408, jobExp: 880, mvpExp: 0,
    attack: 315, attack2: 382, defense: 5, magicDefense: 30,
    str: 0, agi: 38, vit: 38, int: 43, dex: 75, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 176, walkSpeed: 200, attackDelay: 1180, attackMotion: 480, damageMotion: 648,
    size: 'medium', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 0, agi: 38, vit: 38, int: 43, dex: 75, luk: 45, level: 38, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Jack o\' Pumpkin', rate: 90 },
        { itemName: 'Zargon', rate: 9 },
        { itemName: 'Elunium', rate: 0.31 },
        { itemName: 'Tights', rate: 0.05 },
        { itemName: 'Necklace of Oblivion', rate: 0.05 },
        { itemName: 'Pumpkin', rate: 10 },
        { itemName: 'Jakk Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Myst (ID: 1151) ──── Level 38 | HP 3,745 | NORMAL | formless/poison1 | aggressive
RO_MONSTER_TEMPLATES['myst'] = {
    id: 1151, name: 'Myst', aegisName: 'MYST',
    level: 38, maxHealth: 3745, baseExp: 1391, jobExp: 688, mvpExp: 0,
    attack: 365, attack2: 445, defense: 0, magicDefense: 40,
    str: 0, agi: 38, vit: 18, int: 0, dex: 53, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1576, attackMotion: 576, damageMotion: 384,
    size: 'large', race: 'formless', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 0, agi: 38, vit: 18, int: 0, dex: 53, luk: 10, level: 38, weaponATK: 365 },
    modes: {},
    drops: [
        { itemName: 'Gas Mask', rate: 0.02 },
        { itemName: 'Wooden Block', rate: 8 },
        { itemName: 'Wig', rate: 0.1 },
        { itemName: 'Rough Oridecon', rate: 0.65 },
        { itemName: 'Rough Elunium', rate: 0.97 },
        { itemName: 'Anodyne', rate: 0.2 },
        { itemName: 'Grape', rate: 0.35 },
        { itemName: 'Myst Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Christmas Jakk (ID: 1244) ──── Level 38 | HP 3,581 | NORMAL | formless/fire2 | passive
RO_MONSTER_TEMPLATES['jakk_xmas'] = {
    id: 1244, name: 'Christmas Jakk', aegisName: 'JAKK_XMAS',
    level: 38, maxHealth: 3581, baseExp: 1113, jobExp: 688, mvpExp: 0,
    attack: 315, attack2: 382, defense: 5, magicDefense: 30,
    str: 0, agi: 38, vit: 38, int: 43, dex: 75, luk: 45,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 176, walkSpeed: 200, attackDelay: 1180, attackMotion: 480, damageMotion: 648,
    size: 'medium', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 0, agi: 38, vit: 38, int: 43, dex: 75, luk: 45, level: 38, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Jack o\' Pumpkin', rate: 53.35 },
        { itemName: 'Zargon', rate: 9 },
        { itemName: 'Elunium', rate: 0.31 },
        { itemName: 'Tights', rate: 0.05 },
        { itemName: 'Gift Box', rate: 0.2 },
        { itemName: 'Wrapping Paper', rate: 12 },
        { itemName: 'Wrapping Lace', rate: 12 },
        { itemName: 'Jakk Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Myst Case (ID: 1249) ──── Level 38 | HP 3,450 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['mystcase'] = {
    id: 1249, name: 'Myst Case', aegisName: 'MYSTCASE',
    level: 38, maxHealth: 3450, baseExp: 1113, jobExp: 688, mvpExp: 0,
    attack: 160, attack2: 360, defense: 5, magicDefense: 10,
    str: 65, agi: 50, vit: 25, int: 5, dex: 48, luk: 75,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 400, attackDelay: 1248, attackMotion: 1248, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 65, agi: 50, vit: 25, int: 5, dex: 48, luk: 75, level: 38, weaponATK: 160 },
    modes: {},
    drops: [
        { itemName: 'Candy Cane', rate: 0.9 },
        { itemName: 'Zargon', rate: 15 },
        { itemName: 'Old Blue Box', rate: 0.2 },
        { itemName: 'Piece of Cake', rate: 8 },
        { itemName: 'Pearl', rate: 1.5 },
        { itemName: '1carat Diamond', rate: 0.05 },
        { itemName: 'Candy', rate: 3.4 },
        { itemName: 'Myst Case Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wild Rose (ID: 1261) ──── Level 38 | HP 2,980 | NORMAL | brute/wind1 | passive
RO_MONSTER_TEMPLATES['wild_rose'] = {
    id: 1261, name: 'Wild Rose', aegisName: 'WILD_ROSE',
    level: 38, maxHealth: 2980, baseExp: 1113, jobExp: 688, mvpExp: 0,
    attack: 315, attack2: 360, defense: 0, magicDefense: 15,
    str: 65, agi: 85, vit: 15, int: 35, dex: 65, luk: 80,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 181, walkSpeed: 100, attackDelay: 964, attackMotion: 864, damageMotion: 288,
    size: 'small', race: 'brute', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 65, agi: 85, vit: 15, int: 35, dex: 65, luk: 80, level: 38, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 53.35 },
        { itemName: 'Witherless Rose', rate: 0.5 },
        { itemName: 'Nut Shell', rate: 1.2 },
        { itemName: 'Arrow of Shadow', rate: 30 },
        { itemName: 'Rotten Fish', rate: 0.35 },
        { itemName: 'Monster\'s Feed', rate: 6 },
        { itemName: 'Big Sis\' Ribbon', rate: 0.02 },
        { itemName: 'Wild Rose Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Leaf Cat (ID: 1586) ──── Level 38 | HP 2,396 | NORMAL | brute/earth1 | passive
RO_MONSTER_TEMPLATES['leaf_cat'] = {
    id: 1586, name: 'Leaf Cat', aegisName: 'LEAF_CAT',
    level: 38, maxHealth: 2396, baseExp: 165, jobExp: 1212, mvpExp: 0,
    attack: 266, attack2: 307, defense: 5, magicDefense: 19,
    str: 25, agi: 67, vit: 12, int: 45, dex: 60, luk: 29,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 181, walkSpeed: 150, attackDelay: 960, attackMotion: 864, damageMotion: 720,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 25, agi: 67, vit: 12, int: 45, dex: 60, luk: 29, level: 38, weaponATK: 266 },
    modes: {},
    drops: [
        { itemName: 'Huge Leaf', rate: 43.65 },
        { itemName: 'Hinalle Leaflet', rate: 3 },
        { itemName: 'Yggdrasil Seed', rate: 0.05 },
        { itemName: 'Fish Tail', rate: 11 },
        { itemName: 'Lemon', rate: 2.5 },
        { itemName: 'Shrimp', rate: 5 },
        { itemName: 'Fig Leaf', rate: 53.35 },
        { itemName: 'Leaf Cat Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Iceicle (ID: 1789) ──── Level 38 | HP 10 | NORMAL | formless/water2 | passive
RO_MONSTER_TEMPLATES['iceicle'] = {
    id: 1789, name: 'Iceicle', aegisName: 'ICEICLE',
    level: 38, maxHealth: 10, baseExp: 5, jobExp: 5, mvpExp: 0,
    attack: 241, attack2: 1082, defense: 0, magicDefense: 10,
    str: 0, agi: 10, vit: 10, int: 10, dex: 172, luk: 5,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 173, walkSpeed: 1000, attackDelay: 1344, attackMotion: 500, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 19600,
    raceGroups: {},
    stats: { str: 0, agi: 10, vit: 10, int: 10, dex: 172, luk: 5, level: 38, weaponATK: 241 },
    modes: {},
    drops: [
        { itemName: 'Ice Piece', rate: 10 },
        { itemName: 'Ice Piece', rate: 10 },
        { itemName: 'Ice Piece', rate: 10 },
        { itemName: 'Ice Piece', rate: 5 },
        { itemName: 'Ice Piece', rate: 5 },
        { itemName: 'Ice Piece', rate: 5 },
        { itemName: 'Ice Piece', rate: 5 },
        { itemName: 'Ice Piece', rate: 5, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wootan Shooter (ID: 1498) ──── Level 39 | HP 3,977 | NORMAL | demihuman/earth2 | aggressive
RO_MONSTER_TEMPLATES['wootan_shooter'] = {
    id: 1498, name: 'Wootan Shooter', aegisName: 'WOOTAN_SHOOTER',
    level: 39, maxHealth: 3977, baseExp: 886, jobExp: 453, mvpExp: 0,
    attack: 84, attack2: 105, defense: 10, magicDefense: 28,
    str: 15, agi: 35, vit: 29, int: 15, dex: 100, luk: 42,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 200, attackDelay: 857, attackMotion: 1056, damageMotion: 576,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 19800,
    raceGroups: {},
    stats: { str: 15, agi: 35, vit: 29, int: 15, dex: 100, luk: 42, level: 39, weaponATK: 84 },
    modes: {},
    drops: [
        { itemName: 'Slingshot', rate: 45 },
        { itemName: 'Elastic Band', rate: 35 },
        { itemName: 'Banana', rate: 10 },
        { itemName: 'Stone', rate: 10 },
        { itemName: 'Apple', rate: 1 },
        { itemName: 'Cacao', rate: 1 },
        { itemName: 'Banana Hat', rate: 0.1 },
        { itemName: 'Wootan Shooter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ghoul (ID: 1036) ──── Level 40 | HP 5,418 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['ghoul'] = {
    id: 1036, name: 'Ghoul', aegisName: 'GHOUL',
    level: 40, maxHealth: 5418, baseExp: 1088, jobExp: 622, mvpExp: 0,
    attack: 420, attack2: 500, defense: 5, magicDefense: 20,
    str: 0, agi: 20, vit: 29, int: 0, dex: 45, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 151, walkSpeed: 250, attackDelay: 2456, attackMotion: 912, damageMotion: 504,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 29, int: 0, dex: 45, luk: 20, level: 40, weaponATK: 420 },
    modes: {},
    drops: [
        { itemName: 'Horrendous Mouth', rate: 60 },
        { itemName: 'Rough Oridecon', rate: 1.1 },
        { itemName: 'White Herb', rate: 7 },
        { itemName: 'Green Herb', rate: 8 },
        { itemName: 'Skull Ring', rate: 0.6 },
        { itemName: 'Memento', rate: 1.5 },
        { itemName: 'Sharpened Legbone of Ghoul', rate: 0.01 },
        { itemName: 'Ghoul Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marduk (ID: 1140) ──── Level 40 | HP 4,214 | NORMAL | demihuman/fire1 | aggressive
RO_MONSTER_TEMPLATES['marduk'] = {
    id: 1140, name: 'Marduk', aegisName: 'MARDUK',
    level: 40, maxHealth: 4214, baseExp: 1238, jobExp: 752, mvpExp: 0,
    attack: 315, attack2: 382, defense: 0, magicDefense: 60,
    str: 0, agi: 40, vit: 20, int: 79, dex: 78, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1540, attackMotion: 840, damageMotion: 504,
    size: 'large', race: 'demihuman', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 20, int: 79, dex: 78, luk: 20, level: 40, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Flame Heart', rate: 0.35 },
        { itemName: 'Cultish Masque', rate: 43.65 },
        { itemName: 'Staff', rate: 0.1 },
        { itemName: 'Celebrant\'s Mitten', rate: 0.01 },
        { itemName: 'Wand of Occult', rate: 0.03 },
        { itemName: 'Level 5 Fire Bolt', rate: 1 },
        { itemName: 'Book of the Devil', rate: 0.2 },
        { itemName: 'Marduk Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Stem Worm (ID: 1215) ──── Level 40 | HP 6,136 | NORMAL | plant/wind1 | aggressive
RO_MONSTER_TEMPLATES['stem_worm'] = {
    id: 1215, name: 'Stem Worm', aegisName: 'STEM_WORM',
    level: 40, maxHealth: 6136, baseExp: 1452, jobExp: 939, mvpExp: 0,
    attack: 290, attack2: 375, defense: 5, magicDefense: 10,
    str: 0, agi: 30, vit: 26, int: 15, dex: 79, luk: 35,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'medium', race: 'plant', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 26, int: 15, dex: 79, luk: 35, level: 40, weaponATK: 290 },
    modes: {},
    drops: [
        { itemName: 'Tough Scalelike Stem', rate: 53.35 },
        { itemName: 'White Herb', rate: 18 },
        { itemName: 'Skipping Rope', rate: 0.1 },
        { itemName: 'Rough Oridecon', rate: 1.15 },
        { itemName: 'Great Nature', rate: 0.05 },
        { itemName: 'Glaive', rate: 0.2 },
        { itemName: 'Yggdrasil Seed', rate: 0.45 },
        { itemName: 'Stem Worm Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Nereid (ID: 1255) ──── Level 40 | HP 4,120 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['neraid'] = {
    id: 1255, name: 'Nereid', aegisName: 'NERAID',
    level: 40, maxHealth: 4120, baseExp: 1126, jobExp: 684, mvpExp: 0,
    attack: 325, attack2: 360, defense: 0, magicDefense: 10,
    str: 0, agi: 45, vit: 50, int: 5, dex: 64, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 200, attackDelay: 776, attackMotion: 576, damageMotion: 288,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 45, vit: 50, int: 5, dex: 64, luk: 5, level: 40, weaponATK: 325 },
    modes: {},
    drops: [
        { itemName: 'Earthworm Peeling', rate: 51 },
        { itemName: 'Cyfar', rate: 10 },
        { itemName: 'Blue Herb', rate: 2.3 },
        { itemName: 'Icicle Whip', rate: 0.1 },
        { itemName: 'Grape', rate: 2.5 },
        { itemName: 'Rough Elunium', rate: 1.8 },
        { itemName: 'Elunium', rate: 0.37 },
        { itemName: 'Nereid Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pest (ID: 1256) ──── Level 40 | HP 3,240 | NORMAL | brute/shadow2 | aggressive
RO_MONSTER_TEMPLATES['pest'] = {
    id: 1256, name: 'Pest', aegisName: 'PEST',
    level: 40, maxHealth: 3240, baseExp: 1238, jobExp: 752, mvpExp: 0,
    attack: 375, attack2: 450, defense: 0, magicDefense: 5,
    str: 0, agi: 60, vit: 22, int: 5, dex: 80, luk: 5,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 165, attackDelay: 700, attackMotion: 648, damageMotion: 480,
    size: 'small', race: 'brute', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 60, vit: 22, int: 5, dex: 80, luk: 5, level: 40, weaponATK: 375 },
    modes: {},
    drops: [
        { itemName: 'Earthworm Peeling', rate: 55 },
        { itemName: 'Brigan', rate: 2 },
        { itemName: 'Animal Gore', rate: 0.1 },
        { itemName: 'Anodyne', rate: 1 },
        { itemName: 'Red Gemstone', rate: 2.5 },
        { itemName: 'Rough Oridecon', rate: 1.15 },
        { itemName: 'Pest Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Greatest General (ID: 1277) ──── Level 40 | HP 3,632 | NORMAL | formless/fire2 | passive
RO_MONSTER_TEMPLATES['greatest_general'] = {
    id: 1277, name: 'Greatest General', aegisName: 'GREATEST_GENERAL',
    level: 40, maxHealth: 3632, baseExp: 1238, jobExp: 752, mvpExp: 0,
    attack: 350, attack2: 400, defense: 15, magicDefense: 15,
    str: 0, agi: 20, vit: 60, int: 55, dex: 82, luk: 140,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 177, walkSpeed: 200, attackDelay: 1152, attackMotion: 1152, damageMotion: 384,
    size: 'medium', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 60, int: 55, dex: 82, luk: 140, level: 40, weaponATK: 350 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 20 },
        { itemName: 'Wooden Block', rate: 20 },
        { itemName: 'Club', rate: 1 },
        { itemName: 'Authoritative Badge', rate: 3 },
        { itemName: 'Stop Post', rate: 0.01 },
        { itemName: 'Yellow Herb', rate: 2.5 },
        { itemName: 'Level 3 Earth Spike', rate: 1 },
        { itemName: 'Greatest General Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Quve (ID: 1508) ──── Level 40 | HP 4,559 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['quve'] = {
    id: 1508, name: 'Quve', aegisName: 'QUVE',
    level: 40, maxHealth: 4559, baseExp: 414, jobExp: 306, mvpExp: 0,
    attack: 299, attack2: 469, defense: 12, magicDefense: 12,
    str: 0, agi: 61, vit: 24, int: 19, dex: 37, luk: 24,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 150, attackDelay: 912, attackMotion: 1248, damageMotion: 576,
    size: 'small', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 61, vit: 24, int: 19, dex: 37, luk: 24, level: 40, weaponATK: 299 },
    modes: {},
    drops: [
        { itemName: 'Piece of Black Cloth', rate: 32 },
        { itemName: 'Ectoplasm', rate: 57.23 },
        { itemName: 'Fly Wing', rate: 10 },
        { itemName: 'Poisonous Powder', rate: 1 },
        { itemName: 'Rough Oridecon', rate: 0.1 },
        { itemName: 'Quve Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mime Monkey (ID: 1585) ──── Level 40 | HP 6,000 | NORMAL | plant/water1 | passive
RO_MONSTER_TEMPLATES['mime_monkey'] = {
    id: 1585, name: 'Mime Monkey', aegisName: 'MIME_MONKEY',
    level: 40, maxHealth: 6000, baseExp: 200, jobExp: 22, mvpExp: 0,
    attack: 300, attack2: 350, defense: 40, magicDefense: 40,
    str: 0, agi: 40, vit: 40, int: 40, dex: 40, luk: 30,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 163, walkSpeed: 400, attackDelay: 1872, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 40, int: 40, dex: 40, luk: 30, level: 40, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Jellopy', rate: 70 },
        { itemName: 'Knife', rate: 1 },
        { itemName: 'Sticky Mucus', rate: 4 },
        { itemName: 'Apple', rate: 10 },
        { itemName: 'Empty Bottle', rate: 15 },
        { itemName: 'Poring Doll', rate: 0.05 },
        { itemName: 'Unripe Apple', rate: 0.2 },
    ],
    mvpDrops: [],
};

// ──── Magmaring (ID: 1836) ──── Level 40 | HP 5,300 | NORMAL | formless/fire2 | passive
RO_MONSTER_TEMPLATES['magmaring'] = {
    id: 1836, name: 'Magmaring', aegisName: 'MAGMARING',
    level: 40, maxHealth: 5300, baseExp: 2110, jobExp: 1910, mvpExp: 0,
    attack: 550, attack2: 700, defense: 25, magicDefense: 24,
    str: 40, agi: 60, vit: 30, int: 10, dex: 60, luk: 17,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 171, walkSpeed: 300, attackDelay: 1472, attackMotion: 384, damageMotion: 288,
    size: 'small', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 20000,
    raceGroups: {},
    stats: { str: 40, agi: 60, vit: 30, int: 10, dex: 60, luk: 17, level: 40, weaponATK: 550 },
    modes: {},
    drops: [
        { itemName: 'Burning Heart', rate: 30 },
        { itemName: 'Rough Elunium', rate: 0.34 },
        { itemName: 'Magmaring Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Argiope (ID: 1099) ──── Level 41 | HP 4,382 | NORMAL | insect/poison1 | aggressive
RO_MONSTER_TEMPLATES['argiope'] = {
    id: 1099, name: 'Argiope', aegisName: 'ARGIOPE',
    level: 41, maxHealth: 4382, baseExp: 1797, jobExp: 849, mvpExp: 0,
    attack: 395, attack2: 480, defense: 30, magicDefense: 0,
    str: 0, agi: 41, vit: 31, int: 10, dex: 56, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 300, attackDelay: 1792, attackMotion: 792, damageMotion: 336,
    size: 'large', race: 'insect', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20200,
    raceGroups: {},
    stats: { str: 0, agi: 41, vit: 31, int: 10, dex: 56, luk: 30, level: 41, weaponATK: 395 },
    modes: { detector: true },
    drops: [
        { itemName: 'Bug Leg', rate: 53.35 },
        { itemName: 'Zargon', rate: 12 },
        { itemName: 'Rough Elunium', rate: 1.75 },
        { itemName: 'Boots', rate: 0.05 },
        { itemName: 'Green Herb', rate: 15 },
        { itemName: 'Amethyst', rate: 0.1 },
        { itemName: 'Argiope Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Marionette (ID: 1143) ──── Level 41 | HP 3,222 | NORMAL | demon/ghost3 | aggressive
RO_MONSTER_TEMPLATES['marionette'] = {
    id: 1143, name: 'Marionette', aegisName: 'MARIONETTE',
    level: 41, maxHealth: 3222, baseExp: 1078, jobExp: 1276, mvpExp: 0,
    attack: 355, attack2: 422, defense: 0, magicDefense: 25,
    str: 0, agi: 62, vit: 36, int: 44, dex: 69, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 300, attackDelay: 1480, attackMotion: 480, damageMotion: 1056,
    size: 'small', race: 'demon', element: { type: 'ghost', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20200,
    raceGroups: {},
    stats: { str: 0, agi: 62, vit: 36, int: 44, dex: 69, luk: 45, level: 41, weaponATK: 355 },
    modes: { detector: true },
    drops: [
        { itemName: 'Golden Hair', rate: 90 },
        { itemName: 'Star Dust', rate: 0.05 },
        { itemName: 'Brooch', rate: 0.01 },
        { itemName: 'Level 5 Fire Wall', rate: 1 },
        { itemName: 'Chain', rate: 0.15 },
        { itemName: 'Crystal Pumps', rate: 0.01 },
        { itemName: 'Marionette Doll', rate: 0.03 },
        { itemName: 'Marionette Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kapha (ID: 1406) ──── Level 41 | HP 7,892 | NORMAL | fish/water1 | aggressive
RO_MONSTER_TEMPLATES['kapha'] = {
    id: 1406, name: 'Kapha', aegisName: 'KAPHA',
    level: 41, maxHealth: 7892, baseExp: 2278, jobExp: 1552, mvpExp: 0,
    attack: 399, attack2: 719, defense: 20, magicDefense: 38,
    str: 0, agi: 51, vit: 49, int: 22, dex: 73, luk: 45,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 160, walkSpeed: 165, attackDelay: 2012, attackMotion: 1728, damageMotion: 672,
    size: 'medium', race: 'fish', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20200,
    raceGroups: {},
    stats: { str: 0, agi: 51, vit: 49, int: 22, dex: 73, luk: 45, level: 41, weaponATK: 399 },
    modes: {},
    drops: [
        { itemName: 'Yellow Plate', rate: 65 },
        { itemName: 'Cyfar', rate: 35 },
        { itemName: 'Huuma Calm Mind', rate: 0.2 },
        { itemName: 'Aloe Leaflet', rate: 23 },
        { itemName: 'Ment', rate: 0.02 },
        { itemName: 'Loner\'s Guitar', rate: 0.1 },
        { itemName: 'Jitte', rate: 0.05 },
        { itemName: 'Kapha Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wootan Fighter (ID: 1499) ──── Level 41 | HP 4,457 | NORMAL | demihuman/fire2 | aggressive
RO_MONSTER_TEMPLATES['wootan_fighter'] = {
    id: 1499, name: 'Wootan Fighter', aegisName: 'WOOTAN_FIGHTER',
    level: 41, maxHealth: 4457, baseExp: 1790, jobExp: 833, mvpExp: 0,
    attack: 395, attack2: 480, defense: 30, magicDefense: 19,
    str: 0, agi: 41, vit: 31, int: 10, dex: 45, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 200, attackDelay: 912, attackMotion: 1344, damageMotion: 480,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20200,
    raceGroups: {},
    stats: { str: 0, agi: 41, vit: 31, int: 10, dex: 45, luk: 30, level: 41, weaponATK: 395 },
    modes: {},
    drops: [
        { itemName: 'Meat', rate: 45 },
        { itemName: 'Shoulder Protector', rate: 40 },
        { itemName: 'Waghnak', rate: 0.03 },
        { itemName: 'Finger', rate: 0.01 },
        { itemName: 'Banana', rate: 10 },
        { itemName: 'Huge Leaf', rate: 10 },
        { itemName: 'Banana Hat', rate: 0.05 },
        { itemName: 'Wootan Fighter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hunter Fly (ID: 1035) ──── Level 42 | HP 5,242 | NORMAL | insect/wind2 | aggressive
RO_MONSTER_TEMPLATES['hunter_fly'] = {
    id: 1035, name: 'Hunter Fly', aegisName: 'HUNTER_FLY',
    level: 42, maxHealth: 5242, baseExp: 1517, jobExp: 952, mvpExp: 0,
    attack: 246, attack2: 333, defense: 25, magicDefense: 15,
    str: 33, agi: 105, vit: 32, int: 15, dex: 72, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 150, attackDelay: 676, attackMotion: 576, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 33, agi: 105, vit: 32, int: 15, dex: 72, luk: 30, level: 42, weaponATK: 246 },
    modes: { detector: true },
    drops: [
        { itemName: 'Rough Wind', rate: 0.3 },
        { itemName: 'Steel', rate: 1 },
        { itemName: 'Solid Shell', rate: 53.35 },
        { itemName: 'Zargon', rate: 13 },
        { itemName: 'Rough Oridecon', rate: 1.29 },
        { itemName: 'Mini Propeller', rate: 0.01 },
        { itemName: 'Damascus', rate: 0.02 },
        { itemName: 'Hunter Fly Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Chepet (ID: 1250) ──── Level 42 | HP 4,950 | NORMAL | demihuman/fire1 | aggressive
RO_MONSTER_TEMPLATES['chepet'] = {
    id: 1250, name: 'Chepet', aegisName: 'CHEPET',
    level: 42, maxHealth: 4950, baseExp: 1518, jobExp: 946, mvpExp: 0,
    attack: 380, attack2: 440, defense: 0, magicDefense: 25,
    str: 0, agi: 72, vit: 35, int: 71, dex: 65, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 400, attackDelay: 672, attackMotion: 672, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 0, agi: 72, vit: 35, int: 71, dex: 65, luk: 85, level: 42, weaponATK: 380 },
    modes: {},
    drops: [
        { itemName: 'Matchstick', rate: 25 },
        { itemName: 'Zargon', rate: 7.5 },
        { itemName: 'Apple', rate: 55 },
        { itemName: 'Unripe Apple', rate: 0.4 },
        { itemName: 'Red Muffler', rate: 0.05 },
        { itemName: 'Yellow Herb', rate: 13 },
        { itemName: 'Ragamuffin Manteau', rate: 0.05 },
        { itemName: 'Chepet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Alligator (ID: 1271) ──── Level 42 | HP 6,962 | NORMAL | brute/water1 | aggressive
RO_MONSTER_TEMPLATES['alligator'] = {
    id: 1271, name: 'Alligator', aegisName: 'ALLIGATOR',
    level: 42, maxHealth: 6962, baseExp: 1379, jobExp: 866, mvpExp: 0,
    attack: 315, attack2: 360, defense: 2, magicDefense: 5,
    str: 0, agi: 45, vit: 50, int: 10, dex: 69, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1100, attackMotion: 900, damageMotion: 480,
    size: 'medium', race: 'brute', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 0, agi: 45, vit: 50, int: 10, dex: 69, luk: 65, level: 42, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 10 },
        { itemName: 'Worn-out Prison Uniform', rate: 6 },
        { itemName: 'Anolian Skin', rate: 20 },
        { itemName: 'Yggdrasil Seed', rate: 0.5 },
        { itemName: 'Rough Oridecon', rate: 1.29 },
        { itemName: 'Alligator Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Stone Shooter (ID: 1495) ──── Level 42 | HP 4,104 | NORMAL | plant/fire3 | aggressive
RO_MONSTER_TEMPLATES['stone_shooter'] = {
    id: 1495, name: 'Stone Shooter', aegisName: 'STONE_SHOOTER',
    level: 42, maxHealth: 4104, baseExp: 1238, jobExp: 752, mvpExp: 0,
    attack: 309, attack2: 350, defense: 12, magicDefense: 45,
    str: 0, agi: 40, vit: 20, int: 79, dex: 92, luk: 20,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 152, walkSpeed: 175, attackDelay: 2413, attackMotion: 1248, damageMotion: 768,
    size: 'medium', race: 'plant', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 20, int: 79, dex: 92, luk: 20, level: 42, weaponATK: 309 },
    modes: {},
    drops: [
        { itemName: 'Strong Branch', rate: 50 },
        { itemName: 'Log', rate: 50 },
        { itemName: 'Brown Root', rate: 10 },
        { itemName: 'Wooden Block', rate: 20 },
        { itemName: 'Rough Oridecon', rate: 1 },
        { itemName: 'Stone', rate: 10 },
        { itemName: 'Stone Shooter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venomous (ID: 1621) ──── Level 42 | HP 4,653 | NORMAL | formless/poison1 | aggressive
RO_MONSTER_TEMPLATES['venomous'] = {
    id: 1621, name: 'Venomous', aegisName: 'VENOMOUS',
    level: 42, maxHealth: 4653, baseExp: 1780, jobExp: 1280, mvpExp: 0,
    attack: 422, attack2: 844, defense: 0, magicDefense: 49,
    str: 12, agi: 60, vit: 17, int: 19, dex: 60, luk: 0,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 350, attackDelay: 768, attackMotion: 1440, damageMotion: 672,
    size: 'medium', race: 'formless', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 12, agi: 60, vit: 17, int: 19, dex: 60, luk: 0, level: 42, weaponATK: 422 },
    modes: {},
    drops: [
        { itemName: 'Dust Pollutant', rate: 50 },
        { itemName: 'Bacillus', rate: 30 },
        { itemName: 'Poisonous Powder', rate: 10 },
        { itemName: 'Toxic Gas', rate: 20 },
        { itemName: 'Old Blue Box', rate: 0.01 },
        { itemName: 'Venomous Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Novus (ID: 1715) ──── Level 42 | HP 5,430 | NORMAL | dragon/neutral1 | aggressive
RO_MONSTER_TEMPLATES['novus'] = {
    id: 1715, name: 'Novus', aegisName: 'NOVUS',
    level: 42, maxHealth: 5430, baseExp: 1320, jobExp: 1002, mvpExp: 0,
    attack: 284, attack2: 384, defense: 20, magicDefense: 28,
    str: 0, agi: 56, vit: 43, int: 45, dex: 124, luk: 43,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 110, attackDelay: 151, attackMotion: 288, damageMotion: 360,
    size: 'small', race: 'dragon', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 0, agi: 56, vit: 43, int: 45, dex: 124, luk: 43, level: 42, weaponATK: 284 },
    modes: {},
    drops: [
        { itemName: 'Green Herb', rate: 30 },
        { itemName: 'Cyfar', rate: 10.35 },
        { itemName: 'Dragon Scale', rate: 5.89 },
        { itemName: 'Red Novus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Siroma (ID: 1776) ──── Level 42 | HP 6,800 | NORMAL | formless/water3 | passive
RO_MONSTER_TEMPLATES['siroma'] = {
    id: 1776, name: 'Siroma', aegisName: 'SIROMA',
    level: 42, maxHealth: 6800, baseExp: 2230, jobExp: 1005, mvpExp: 0,
    attack: 220, attack2: 440, defense: 12, magicDefense: 8,
    str: 33, agi: 23, vit: 52, int: 11, dex: 40, luk: 19,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 432, attackMotion: 648, damageMotion: 240,
    size: 'small', race: 'formless', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 20400,
    raceGroups: {},
    stats: { str: 33, agi: 23, vit: 52, int: 11, dex: 40, luk: 19, level: 42, weaponATK: 220 },
    modes: {},
    drops: [
        { itemName: 'Glacial Heart', rate: 10 },
        { itemName: 'Ice Piece', rate: 5 },
        { itemName: 'Blue Herb', rate: 0.1 },
        { itemName: 'Crystal Blue', rate: 0.2 },
        { itemName: 'Siroma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Side Winder (ID: 1037) ──── Level 43 | HP 4,929 | NORMAL | brute/poison1 | aggressive
RO_MONSTER_TEMPLATES['side_winder'] = {
    id: 1037, name: 'Side Winder', aegisName: 'SIDE_WINDER',
    level: 43, maxHealth: 4929, baseExp: 1996, jobExp: 993, mvpExp: 0,
    attack: 240, attack2: 320, defense: 5, magicDefense: 10,
    str: 38, agi: 43, vit: 40, int: 15, dex: 115, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1576, attackMotion: 576, damageMotion: 576,
    size: 'medium', race: 'brute', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 38, agi: 43, vit: 40, int: 15, dex: 115, luk: 20, level: 43, weaponATK: 240 },
    modes: {},
    drops: [
        { itemName: 'Shining Scale', rate: 53.35 },
        { itemName: 'Zargon', rate: 14 },
        { itemName: 'Rough Oridecon', rate: 1.34 },
        { itemName: 'Tsurugi', rate: 0.02 },
        { itemName: 'Venom Canine', rate: 25 },
        { itemName: 'Snake Scale', rate: 50 },
        { itemName: 'White Herb', rate: 10 },
        { itemName: 'Sidewinder Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Punk (ID: 1199) ──── Level 43 | HP 3,620 | NORMAL | plant/wind1 | aggressive
RO_MONSTER_TEMPLATES['punk'] = {
    id: 1199, name: 'Punk', aegisName: 'PUNK',
    level: 43, maxHealth: 3620, baseExp: 1699, jobExp: 1033, mvpExp: 0,
    attack: 292, attack2: 365, defense: 0, magicDefense: 45,
    str: 0, agi: 105, vit: 5, int: 45, dex: 65, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 300, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'small', race: 'plant', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 105, vit: 5, int: 45, dex: 65, luk: 20, level: 43, weaponATK: 292 },
    modes: {},
    drops: [
        { itemName: 'Mould Powder', rate: 53.35 },
        { itemName: 'Yellow Gemstone', rate: 8 },
        { itemName: 'Pacifier', rate: 1 },
        { itemName: 'Witched Starsand', rate: 10 },
        { itemName: 'Moth Dust', rate: 30 },
        { itemName: 'Fly Wing', rate: 11 },
        { itemName: 'Hood', rate: 0.15 },
        { itemName: 'Punk Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Choco (ID: 1214) ──── Level 43 | HP 4,278 | NORMAL | brute/fire1 | aggressive
RO_MONSTER_TEMPLATES['choco'] = {
    id: 1214, name: 'Choco', aegisName: 'CHOCO',
    level: 43, maxHealth: 4278, baseExp: 1265, jobExp: 1265, mvpExp: 0,
    attack: 315, attack2: 402, defense: 5, magicDefense: 5,
    str: 65, agi: 68, vit: 55, int: 45, dex: 65, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'small', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 65, agi: 68, vit: 55, int: 45, dex: 65, luk: 25, level: 43, weaponATK: 315 },
    modes: {},
    drops: [
        { itemName: 'Claw of Monkey', rate: 53.35 },
        { itemName: 'Yoyo Tail', rate: 70 },
        { itemName: 'Elunium', rate: 0.53 },
        { itemName: 'Banana', rate: 50 },
        { itemName: 'Tropical Banana', rate: 0.2 },
        { itemName: 'Banana Juice', rate: 10 },
        { itemName: 'Yggdrasil Berry', rate: 0.25 },
        { itemName: 'Choco Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sage Worm (ID: 1281) ──── Level 43 | HP 3,850 | NORMAL | brute/neutral3 | aggressive
RO_MONSTER_TEMPLATES['sageworm'] = {
    id: 1281, name: 'Sage Worm', aegisName: 'SAGEWORM',
    level: 43, maxHealth: 3850, baseExp: 1155, jobExp: 1320, mvpExp: 0,
    attack: 120, attack2: 280, defense: 0, magicDefense: 50,
    str: 0, agi: 52, vit: 24, int: 88, dex: 79, luk: 55,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 200, attackDelay: 936, attackMotion: 936, damageMotion: 288,
    size: 'small', race: 'brute', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 52, vit: 24, int: 88, dex: 79, luk: 55, level: 43, weaponATK: 120 },
    modes: {},
    drops: [
        { itemName: 'Librarian Glove', rate: 0.05 },
        { itemName: 'Worn Out Page', rate: 10 },
        { itemName: 'Earthworm Peeling', rate: 30 },
        { itemName: 'Level 5 Fire Bolt', rate: 1 },
        { itemName: 'Blue Potion', rate: 0.4 },
        { itemName: 'Level 5 Cold Bolt', rate: 1 },
        { itemName: 'Ph.D Hat', rate: 0.01 },
        { itemName: 'Sage Worm Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Blazer (ID: 1367) ──── Level 43 | HP 8,252 | NORMAL | demon/fire2 | aggressive
RO_MONSTER_TEMPLATES['blazzer'] = {
    id: 1367, name: 'Blazer', aegisName: 'BLAZZER',
    level: 43, maxHealth: 8252, baseExp: 3173, jobExp: 1871, mvpExp: 0,
    attack: 533, attack2: 709, defense: 50, magicDefense: 40,
    str: 0, agi: 52, vit: 50, int: 39, dex: 69, luk: 40,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 180, attackDelay: 1732, attackMotion: 1332, damageMotion: 540,
    size: 'medium', race: 'demon', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 52, vit: 50, int: 39, dex: 69, luk: 40, level: 43, weaponATK: 533 },
    modes: { detector: true },
    drops: [
        { itemName: 'Burning Heart', rate: 48.5 },
        { itemName: 'Live Coal', rate: 34 },
        { itemName: 'White Herb', rate: 30 },
        { itemName: 'Blazer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pitman (ID: 1616) ──── Level 43 | HP 5,015 | NORMAL | undead/earth2 | aggressive
RO_MONSTER_TEMPLATES['pitman'] = {
    id: 1616, name: 'Pitman', aegisName: 'PITMAN',
    level: 43, maxHealth: 5015, baseExp: 1799, jobExp: 1083, mvpExp: 0,
    attack: 290, attack2: 486, defense: 22, magicDefense: 26,
    str: 0, agi: 15, vit: 5, int: 5, dex: 52, luk: 36,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 180, attackDelay: 960, attackMotion: 336, damageMotion: 300,
    size: 'large', race: 'undead', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 15, vit: 5, int: 5, dex: 52, luk: 36, level: 43, weaponATK: 290 },
    modes: {},
    drops: [
        { itemName: 'Old Pick', rate: 30 },
        { itemName: 'Used Iron Plate', rate: 5 },
        { itemName: 'Iron', rate: 8 },
        { itemName: 'Steel', rate: 5 },
        { itemName: 'Coal', rate: 1 },
        { itemName: 'Lantern', rate: 10 },
        { itemName: 'Flashlight', rate: 0.8 },
        { itemName: 'Pitman Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hill Wind (ID: 1629) ──── Level 43 | HP 3,189 | NORMAL | brute/wind3 | aggressive
RO_MONSTER_TEMPLATES['hill_wind'] = {
    id: 1629, name: 'Hill Wind', aegisName: 'HILL_WIND',
    level: 43, maxHealth: 3189, baseExp: 1800, jobExp: 1100, mvpExp: 0,
    attack: 290, attack2: 480, defense: 10, magicDefense: 15,
    str: 21, agi: 42, vit: 31, int: 50, dex: 41, luk: 23,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 336, attackMotion: 540, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 21, agi: 42, vit: 31, int: 50, dex: 41, luk: 23, level: 43, weaponATK: 290 },
    modes: {},
    drops: [
        { itemName: 'Meat', rate: 10 },
        { itemName: 'Monster\'s Feed', rate: 10 },
        { itemName: 'Hill Wind Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plasma (ID: 1694) ──── Level 43 | HP 5,700 | NORMAL | formless/fire4 | aggressive
RO_MONSTER_TEMPLATES['plasma_r'] = {
    id: 1694, name: 'Plasma', aegisName: 'PLASMA_R',
    level: 43, maxHealth: 5700, baseExp: 2000, jobExp: 1000, mvpExp: 0,
    attack: 300, attack2: 700, defense: 0, magicDefense: 30,
    str: 0, agi: 30, vit: 5, int: 56, dex: 90, luk: 30,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 608, attackMotion: 1440, damageMotion: 576,
    size: 'small', race: 'formless', element: { type: 'fire', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 5, int: 56, dex: 90, luk: 30, level: 43, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.02 },
        { itemName: 'Red Gemstone', rate: 1 },
        { itemName: 'Red Blood', rate: 0.45 },
        { itemName: 'Plasma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Novus (ID: 1718) ──── Level 43 | HP 5,830 | NORMAL | dragon/neutral1 | aggressive
RO_MONSTER_TEMPLATES['novus_'] = {
    id: 1718, name: 'Novus', aegisName: 'NOVUS_',
    level: 43, maxHealth: 5830, baseExp: 1411, jobExp: 1100, mvpExp: 0,
    attack: 314, attack2: 414, defense: 24, magicDefense: 28,
    str: 0, agi: 60, vit: 43, int: 39, dex: 119, luk: 43,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 252, attackMotion: 816, damageMotion: 480,
    size: 'small', race: 'dragon', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 60, vit: 43, int: 39, dex: 119, luk: 43, level: 43, weaponATK: 314 },
    modes: {},
    drops: [
        { itemName: 'Yellow Herb', rate: 20 },
        { itemName: 'Cyfar', rate: 10.35 },
        { itemName: 'Dragon Scale', rate: 5.89 },
        { itemName: 'Red Novus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dragon Egg (ID: 1721) ──── Level 43 | HP 18,322 | NORMAL | dragon/neutral2 | passive
RO_MONSTER_TEMPLATES['dragon_egg'] = {
    id: 1721, name: 'Dragon Egg', aegisName: 'DRAGON_EGG',
    level: 43, maxHealth: 18322, baseExp: 6740, jobExp: 0, mvpExp: 0,
    attack: 1, attack2: 2, defense: 78, magicDefense: 60,
    str: 0, agi: 0, vit: 56, int: 67, dex: 0, luk: 63,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 190, walkSpeed: 1000, attackDelay: 24, attackMotion: 1, damageMotion: 1,
    size: 'medium', race: 'dragon', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 20600,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 56, int: 67, dex: 0, luk: 63, level: 43, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Elunium', rate: 0.05 },
        { itemName: 'Piece of Egg Shell', rate: 1 },
        { itemName: '1carat Diamond', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.05 },
        { itemName: 'Garnet', rate: 0.1 },
        { itemName: 'Aquamarine', rate: 0.1 },
        { itemName: 'Topaz', rate: 0.1 },
        { itemName: 'Dragon Egg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bathory (ID: 1102) ──── Level 44 | HP 5,415 | NORMAL | demihuman/shadow1 | aggressive
RO_MONSTER_TEMPLATES['bathory'] = {
    id: 1102, name: 'Bathory', aegisName: 'BATHORY',
    level: 44, maxHealth: 5415, baseExp: 2503, jobExp: 1034, mvpExp: 0,
    attack: 198, attack2: 398, defense: 0, magicDefense: 60,
    str: 0, agi: 76, vit: 24, int: 85, dex: 65, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 100, attackDelay: 1504, attackMotion: 840, damageMotion: 900,
    size: 'medium', race: 'demihuman', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20800,
    raceGroups: {},
    stats: { str: 0, agi: 76, vit: 24, int: 85, dex: 65, luk: 15, level: 44, weaponATK: 198 },
    modes: {},
    drops: [
        { itemName: 'Star Dust', rate: 2 },
        { itemName: 'Witched Starsand', rate: 48.5 },
        { itemName: 'Wizard Hat', rate: 0.03 },
        { itemName: 'Arc Wand', rate: 0.05 },
        { itemName: 'Star Crumb', rate: 0.3 },
        { itemName: 'Old Magicbook', rate: 0.15 },
        { itemName: 'Old Broom', rate: 0.2 },
        { itemName: 'Bathory Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Petite (ID: 1155) ──── Level 44 | HP 6,881 | NORMAL | dragon/earth1 | aggressive
RO_MONSTER_TEMPLATES['petit'] = {
    id: 1155, name: 'Petite', aegisName: 'PETIT',
    level: 44, maxHealth: 6881, baseExp: 1677, jobExp: 1034, mvpExp: 0,
    attack: 360, attack2: 427, defense: 30, magicDefense: 30,
    str: 0, agi: 44, vit: 62, int: 55, dex: 79, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1624, attackMotion: 620, damageMotion: 384,
    size: 'medium', race: 'dragon', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20800,
    raceGroups: {},
    stats: { str: 0, agi: 44, vit: 62, int: 55, dex: 79, luk: 60, level: 44, weaponATK: 360 },
    modes: {},
    drops: [
        { itemName: 'Dragon Canine', rate: 53.35 },
        { itemName: 'Dragon Tail', rate: 3 },
        { itemName: 'Rough Oridecon', rate: 1.4 },
        { itemName: 'White Herb', rate: 10 },
        { itemName: 'Flail', rate: 1.5 },
        { itemName: 'Zargon', rate: 15 },
        { itemName: 'Aloevera', rate: 0.15 },
        { itemName: 'Earth Petite Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plasma (ID: 1697) ──── Level 44 | HP 8,200 | NORMAL | formless/water4 | aggressive
RO_MONSTER_TEMPLATES['plasma_b'] = {
    id: 1697, name: 'Plasma', aegisName: 'PLASMA_B',
    level: 44, maxHealth: 8200, baseExp: 2000, jobExp: 1000, mvpExp: 0,
    attack: 300, attack2: 700, defense: 0, magicDefense: 30,
    str: 0, agi: 30, vit: 5, int: 73, dex: 90, luk: 30,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 608, attackMotion: 1440, damageMotion: 576,
    size: 'small', race: 'formless', element: { type: 'water', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 20800,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 5, int: 73, dex: 90, luk: 30, level: 44, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.02 },
        { itemName: 'Blue Gemstone', rate: 1 },
        { itemName: 'Crystal Blue', rate: 0.35 },
        { itemName: 'Plasma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Galion (ID: 1783) ──── Level 44 | HP 32,240 | BOSS | brute/wind2 | aggressive
RO_MONSTER_TEMPLATES['galion'] = {
    id: 1783, name: 'Galion', aegisName: 'GALION',
    level: 44, maxHealth: 32240, baseExp: 10020, jobExp: 3368, mvpExp: 0,
    attack: 336, attack2: 441, defense: 11, magicDefense: 12,
    str: 51, agi: 52, vit: 59, int: 25, dex: 72, luk: 32,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 150, attackDelay: 864, attackMotion: 624, damageMotion: 360,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 51, agi: 52, vit: 59, int: 25, dex: 72, luk: 32, level: 44, weaponATK: 336 },
    modes: {},
    drops: [
        { itemName: 'Rotten Meat', rate: 30 },
        { itemName: 'Animal\'s Skin', rate: 30 },
        { itemName: 'Rough Wind', rate: 0.1 },
        { itemName: 'Ulfhedinn', rate: 0.05 },
        { itemName: 'Galion Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Petite (ID: 1156) ──── Level 45 | HP 5,747 | NORMAL | dragon/wind1 | aggressive
RO_MONSTER_TEMPLATES['petit_'] = {
    id: 1156, name: 'Petite', aegisName: 'PETIT_',
    level: 45, maxHealth: 5747, baseExp: 1758, jobExp: 1075, mvpExp: 0,
    attack: 300, attack2: 355, defense: 20, magicDefense: 45,
    str: 0, agi: 113, vit: 45, int: 55, dex: 73, luk: 80,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 172, walkSpeed: 150, attackDelay: 1420, attackMotion: 1080, damageMotion: 528,
    size: 'medium', race: 'dragon', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21000,
    raceGroups: {},
    stats: { str: 0, agi: 113, vit: 45, int: 55, dex: 73, luk: 80, level: 45, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Dragon Scale', rate: 53.35 },
        { itemName: 'Dragon Tail', rate: 3 },
        { itemName: 'Elunium', rate: 0.61 },
        { itemName: 'White Herb', rate: 10 },
        { itemName: 'Khukri', rate: 0.05 },
        { itemName: 'Zargon', rate: 15 },
        { itemName: 'Aloevera', rate: 0.15 },
        { itemName: 'Earth Petite Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Megalith (ID: 1274) ──── Level 45 | HP 5,300 | NORMAL | formless/neutral4 | passive
RO_MONSTER_TEMPLATES['megalith'] = {
    id: 1274, name: 'Megalith', aegisName: 'MEGALITH',
    level: 45, maxHealth: 5300, baseExp: 1758, jobExp: 1075, mvpExp: 0,
    attack: 264, attack2: 314, defense: 50, magicDefense: 25,
    str: 0, agi: 45, vit: 60, int: 5, dex: 95, luk: 5,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 173, walkSpeed: 200, attackDelay: 1332, attackMotion: 1332, damageMotion: 672,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 21000,
    raceGroups: {},
    stats: { str: 0, agi: 45, vit: 60, int: 5, dex: 95, luk: 5, level: 45, weaponATK: 264 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 1 },
        { itemName: 'Stone', rate: 10 },
        { itemName: 'Old Purple Box', rate: 0.01 },
        { itemName: 'Elunium', rate: 0.61 },
        { itemName: 'Rough Elunium', rate: 2.07 },
        { itemName: 'Megalith Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hill Wind (ID: 1680) ──── Level 45 | HP 4,233 | NORMAL | brute/wind3 | aggressive
RO_MONSTER_TEMPLATES['hill_wind_1'] = {
    id: 1680, name: 'Hill Wind', aegisName: 'HILL_WIND_1',
    level: 45, maxHealth: 4233, baseExp: 2132, jobExp: 1722, mvpExp: 0,
    attack: 320, attack2: 510, defense: 10, magicDefense: 15,
    str: 21, agi: 42, vit: 31, int: 50, dex: 67, luk: 23,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 170, attackDelay: 504, attackMotion: 480, damageMotion: 360,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21000,
    raceGroups: {},
    stats: { str: 21, agi: 42, vit: 31, int: 50, dex: 67, luk: 23, level: 45, weaponATK: 320 },
    modes: {},
    drops: [
        { itemName: 'Harpy\'s Feather', rate: 40 },
        { itemName: 'Harpy\'s Claw', rate: 30 },
        { itemName: 'Monster\'s Feed', rate: 10 },
        { itemName: 'Blue Herb', rate: 0.1 },
        { itemName: 'Hill Wind Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deviruchi (ID: 1109) ──── Level 46 | HP 6,666 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['deviruchi'] = {
    id: 1109, name: 'Deviruchi', aegisName: 'DEVIRUCHI',
    level: 46, maxHealth: 6666, baseExp: 2662, jobExp: 1278, mvpExp: 0,
    attack: 475, attack2: 560, defense: 10, magicDefense: 25,
    str: 0, agi: 69, vit: 40, int: 55, dex: 70, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 150, attackDelay: 980, attackMotion: 600, damageMotion: 384,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 0, agi: 69, vit: 40, int: 55, dex: 70, luk: 30, level: 46, weaponATK: 475 },
    modes: { detector: true },
    drops: [
        { itemName: 'Little Evil Horn', rate: 53.35 },
        { itemName: 'Little Evil Wing', rate: 4 },
        { itemName: 'Oridecon', rate: 0.02 },
        { itemName: 'Partizan', rate: 0.02 },
        { itemName: 'Hand of God', rate: 0.05 },
        { itemName: 'Zargon', rate: 15 },
        { itemName: 'Rough Oridecon', rate: 1.54 },
        { itemName: 'Deviruchi Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Brilight (ID: 1211) ──── Level 46 | HP 5,562 | NORMAL | insect/fire1 | aggressive
RO_MONSTER_TEMPLATES['brilight'] = {
    id: 1211, name: 'Brilight', aegisName: 'BRILIGHT',
    level: 46, maxHealth: 5562, baseExp: 1826, jobExp: 1331, mvpExp: 0,
    attack: 298, attack2: 383, defense: 30, magicDefense: 5,
    str: 0, agi: 90, vit: 15, int: 10, dex: 50, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'small', race: 'insect', element: { type: 'fire', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 0, agi: 90, vit: 15, int: 10, dex: 50, luk: 35, level: 46, weaponATK: 298 },
    modes: { detector: true },
    drops: [
        { itemName: 'Glitter Shell', rate: 53.35 },
        { itemName: 'Wind of Verdure', rate: 2 },
        { itemName: 'Zargon', rate: 12 },
        { itemName: 'Butterfly Wing', rate: 10 },
        { itemName: 'Rough Elunium', rate: 2.2 },
        { itemName: 'Yggdrasil Leaf', rate: 2.5 },
        { itemName: 'White Herb', rate: 26 },
        { itemName: 'Brilight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Explosion (ID: 1383) ──── Level 46 | HP 8,054 | NORMAL | brute/fire3 | aggressive
RO_MONSTER_TEMPLATES['explosion'] = {
    id: 1383, name: 'Explosion', aegisName: 'EXPLOSION',
    level: 46, maxHealth: 8054, baseExp: 2404, jobExp: 1642, mvpExp: 0,
    attack: 336, attack2: 447, defense: 35, magicDefense: 27,
    str: 0, agi: 61, vit: 56, int: 50, dex: 66, luk: 38,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 165, attackDelay: 1260, attackMotion: 960, damageMotion: 336,
    size: 'small', race: 'brute', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 0, agi: 61, vit: 56, int: 50, dex: 66, luk: 38, level: 46, weaponATK: 336 },
    modes: {},
    drops: [
        { itemName: 'Wing of Red Bat', rate: 55 },
        { itemName: 'Burning Heart', rate: 22 },
        { itemName: 'Burning Hair', rate: 32 },
        { itemName: 'Rough Oridecon', rate: 8 },
        { itemName: 'Mastela Fruit', rate: 4 },
        { itemName: 'Explosion Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Poison Toad (ID: 1402) ──── Level 46 | HP 6,629 | NORMAL | brute/poison2 | passive
RO_MONSTER_TEMPLATES['poison_toad'] = {
    id: 1402, name: 'Poison Toad', aegisName: 'POISON_TOAD',
    level: 46, maxHealth: 6629, baseExp: 1929, jobExp: 1457, mvpExp: 0,
    attack: 288, attack2: 408, defense: 5, magicDefense: 10,
    str: 20, agi: 34, vit: 19, int: 14, dex: 66, luk: 55,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 177, walkSpeed: 160, attackDelay: 1148, attackMotion: 1728, damageMotion: 864,
    size: 'medium', race: 'brute', element: { type: 'poison', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 20, agi: 34, vit: 19, int: 14, dex: 66, luk: 55, level: 46, weaponATK: 288 },
    modes: {},
    drops: [
        { itemName: 'Poison Toad\'s Skin', rate: 55 },
        { itemName: 'Poisonous Powder', rate: 24 },
        { itemName: 'Gold Ring', rate: 0.04 },
        { itemName: 'Green Herb', rate: 5.4 },
        { itemName: 'Ruby', rate: 0.02 },
        { itemName: 'Royal Jelly', rate: 0.02 },
        { itemName: 'Cinquedea', rate: 0.1 },
        { itemName: 'Poisonous Toad Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hermit Plant (ID: 1413) ──── Level 46 | HP 6,900 | NORMAL | plant/fire2 | aggressive
RO_MONSTER_TEMPLATES['wild_ginseng'] = {
    id: 1413, name: 'Hermit Plant', aegisName: 'WILD_GINSENG',
    level: 46, maxHealth: 6900, baseExp: 1038, jobExp: 692, mvpExp: 0,
    attack: 220, attack2: 280, defense: 10, magicDefense: 20,
    str: 13, agi: 42, vit: 36, int: 55, dex: 66, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 140, attackDelay: 512, attackMotion: 756, damageMotion: 360,
    size: 'small', race: 'plant', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 13, agi: 42, vit: 36, int: 55, dex: 66, luk: 30, level: 46, weaponATK: 220 },
    modes: {},
    drops: [
        { itemName: 'Hinalle Leaflet', rate: 35 },
        { itemName: 'Aloe Leaflet', rate: 35 },
        { itemName: 'Maneater Root', rate: 38 },
        { itemName: 'Maneater Blossom', rate: 48 },
        { itemName: 'Sweet Potato', rate: 48 },
        { itemName: 'Rope', rate: 0.01 },
        { itemName: 'Strawberry', rate: 10 },
        { itemName: 'Hermit Plant Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Drosera (ID: 1781) ──── Level 46 | HP 7,221 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['drosera'] = {
    id: 1781, name: 'Drosera', aegisName: 'DROSERA',
    level: 46, maxHealth: 7221, baseExp: 2612, jobExp: 1022, mvpExp: 0,
    attack: 389, attack2: 589, defense: 10, magicDefense: 13,
    str: 0, agi: 30, vit: 27, int: 17, dex: 76, luk: 41,
    attackRange: 350, aggroRange: 0, chaseRange: 600,
    aspd: 183, walkSpeed: 1000, attackDelay: 864, attackMotion: 576, damageMotion: 336,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 21200,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 27, int: 17, dex: 76, luk: 41, level: 46, weaponATK: 389 },
    modes: {},
    drops: [
        { itemName: 'Sticky Poison', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 30 },
        { itemName: 'Maneater Blossom', rate: 20 },
        { itemName: 'Maneater Root', rate: 20 },
        { itemName: 'Bitter Herb', rate: 0.03 },
        { itemName: 'Stem', rate: 10 },
        { itemName: 'Drosera Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Isis (ID: 1029) ──── Level 47 | HP 7,003 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['isis'] = {
    id: 1029, name: 'Isis', aegisName: 'ISIS',
    level: 47, maxHealth: 7003, baseExp: 3709, jobExp: 1550, mvpExp: 0,
    attack: 423, attack2: 507, defense: 10, magicDefense: 35,
    str: 38, agi: 65, vit: 43, int: 50, dex: 66, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 172, walkSpeed: 200, attackDelay: 1384, attackMotion: 768, damageMotion: 336,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21400,
    raceGroups: {},
    stats: { str: 38, agi: 65, vit: 43, int: 50, dex: 66, luk: 15, level: 47, weaponATK: 423 },
    modes: { detector: true },
    drops: [
        { itemName: 'Scale Shell', rate: 53.35 },
        { itemName: 'Circlet', rate: 0.05 },
        { itemName: 'Necklace', rate: 0.01 },
        { itemName: '1carat Diamond', rate: 1.5 },
        { itemName: '1carat Diamond', rate: 0.2 },
        { itemName: 'Shining Scale', rate: 10 },
        { itemName: '1carat Diamond', rate: 0.05 },
        { itemName: 'Isis Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deviace (ID: 1108) ──── Level 47 | HP 20,090 | NORMAL | fish/water4 | aggressive
RO_MONSTER_TEMPLATES['deviace'] = {
    id: 1108, name: 'Deviace', aegisName: 'DEVIACE',
    level: 47, maxHealth: 20090, baseExp: 9988, jobExp: 7207, mvpExp: 0,
    attack: 514, attack2: 1024, defense: 10, magicDefense: 20,
    str: 0, agi: 47, vit: 62, int: 48, dex: 62, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 400, attackDelay: 1680, attackMotion: 480, damageMotion: 384,
    size: 'medium', race: 'fish', element: { type: 'water', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21400,
    raceGroups: {},
    stats: { str: 0, agi: 47, vit: 62, int: 48, dex: 62, luk: 25, level: 47, weaponATK: 514 },
    modes: {},
    drops: [
        { itemName: 'Mystic Frozen', rate: 0.25 },
        { itemName: 'Ancient Tooth', rate: 90 },
        { itemName: 'Ancient Lips', rate: 10 },
        { itemName: 'Aerial', rate: 0.02 },
        { itemName: 'Detrimindexta', rate: 2 },
        { itemName: 'Katar of Frozen Icicle', rate: 0.03 },
        { itemName: 'Rough Oridecon', rate: 1.61 },
        { itemName: 'Deviace Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Iron Fist (ID: 1212) ──── Level 47 | HP 4,221 | NORMAL | insect/neutral3 | aggressive
RO_MONSTER_TEMPLATES['iron_fist'] = {
    id: 1212, name: 'Iron Fist', aegisName: 'IRON_FIST',
    level: 47, maxHealth: 4221, baseExp: 1435, jobExp: 1520, mvpExp: 0,
    attack: 430, attack2: 590, defense: 40, magicDefense: 5,
    str: 0, agi: 25, vit: 15, int: 10, dex: 81, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'medium', race: 'insect', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21400,
    raceGroups: {},
    stats: { str: 0, agi: 25, vit: 15, int: 10, dex: 81, luk: 20, level: 47, weaponATK: 430 },
    modes: { detector: true },
    drops: [
        { itemName: 'Tail of Steel Scorpion', rate: 53.35 },
        { itemName: 'Rough Elunium', rate: 2.29 },
        { itemName: 'Rough Elunium', rate: 0.22 },
        { itemName: 'Iron Ore', rate: 7.5 },
        { itemName: 'Steel', rate: 1.8 },
        { itemName: 'Iron', rate: 3 },
        { itemName: 'Iron Fist Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Firelock Soldier (ID: 1403) ──── Level 47 | HP 3,852 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['antique_firelock'] = {
    id: 1403, name: 'Firelock Soldier', aegisName: 'ANTIQUE_FIRELOCK',
    level: 47, maxHealth: 3852, baseExp: 1293, jobExp: 1003, mvpExp: 0,
    attack: 289, attack2: 336, defense: 10, magicDefense: 10,
    str: 15, agi: 35, vit: 29, int: 15, dex: 120, luk: 42,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 170, attackDelay: 1084, attackMotion: 2304, damageMotion: 576,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21400,
    raceGroups: {},
    stats: { str: 15, agi: 35, vit: 29, int: 15, dex: 120, luk: 42, level: 47, weaponATK: 289 },
    modes: {},
    drops: [
        { itemName: 'Iron', rate: 55 },
        { itemName: 'Apple of Archer', rate: 0.01 },
        { itemName: 'Large Jellopy', rate: 14 },
        { itemName: 'Yellow Herb', rate: 0.4 },
        { itemName: 'Yam', rate: 3.5 },
        { itemName: 'Panacea', rate: 2.5 },
        { itemName: 'Cyclone', rate: 0.05 },
        { itemName: 'Firelock Soldier Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plasma (ID: 1695) ──── Level 47 | HP 7,600 | NORMAL | formless/earth4 | aggressive
RO_MONSTER_TEMPLATES['plasma_g'] = {
    id: 1695, name: 'Plasma', aegisName: 'PLASMA_G',
    level: 47, maxHealth: 7600, baseExp: 2000, jobExp: 1000, mvpExp: 0,
    attack: 300, attack2: 700, defense: 0, magicDefense: 30,
    str: 0, agi: 30, vit: 5, int: 61, dex: 90, luk: 30,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 608, attackMotion: 1440, damageMotion: 576,
    size: 'small', race: 'formless', element: { type: 'earth', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21400,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 5, int: 61, dex: 90, luk: 30, level: 47, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.02 },
        { itemName: 'Blue Gemstone', rate: 1 },
        { itemName: 'Green Live', rate: 0.4 },
        { itemName: 'Plasma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Strouf (ID: 1065) ──── Level 48 | HP 11,990 | NORMAL | fish/water3 | aggressive
RO_MONSTER_TEMPLATES['strouf'] = {
    id: 1065, name: 'Strouf', aegisName: 'STROUF',
    level: 48, maxHealth: 11990, baseExp: 3080, jobExp: 2098, mvpExp: 0,
    attack: 200, attack2: 1250, defense: 5, magicDefense: 50,
    str: 0, agi: 40, vit: 45, int: 92, dex: 43, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 150, attackDelay: 1872, attackMotion: 672, damageMotion: 384,
    size: 'large', race: 'fish', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21600,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 45, int: 92, dex: 43, luk: 65, level: 48, weaponATK: 200 },
    modes: {},
    drops: [
        { itemName: 'Fin', rate: 53.35 },
        { itemName: 'Rough Oridecon', rate: 1.15 },
        { itemName: 'Grampa Beard', rate: 0.02 },
        { itemName: 'Trident', rate: 0.02 },
        { itemName: 'Feather', rate: 30 },
        { itemName: 'Aquamarine', rate: 0.2 },
        { itemName: 'Gill', rate: 15 },
        { itemName: 'Strouf Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gargoyle (ID: 1253) ──── Level 48 | HP 3,950 | NORMAL | demon/wind3 | aggressive
RO_MONSTER_TEMPLATES['gargoyle'] = {
    id: 1253, name: 'Gargoyle', aegisName: 'GARGOYLE',
    level: 48, maxHealth: 3950, baseExp: 1650, jobExp: 1650, mvpExp: 0,
    attack: 290, attack2: 360, defense: 10, magicDefense: 10,
    str: 15, agi: 61, vit: 20, int: 20, dex: 126, luk: 40,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 1020, attackMotion: 720, damageMotion: 384,
    size: 'medium', race: 'demon', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21600,
    raceGroups: {},
    stats: { str: 15, agi: 61, vit: 20, int: 20, dex: 126, luk: 40, level: 48, weaponATK: 290 },
    modes: { detector: true },
    drops: [
        { itemName: 'Zargon', rate: 38.8 },
        { itemName: 'Little Evil Wing', rate: 5 },
        { itemName: 'Bow Thimble', rate: 0.01 },
        { itemName: 'Mute Arrow', rate: 20 },
        { itemName: 'Rough Elunium', rate: 2.38 },
        { itemName: 'Gargoyle Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Jing Guai (ID: 1517) ──── Level 48 | HP 5,920 | NORMAL | demon/earth3 | aggressive
RO_MONSTER_TEMPLATES['li_me_mang_ryang'] = {
    id: 1517, name: 'Jing Guai', aegisName: 'LI_ME_MANG_RYANG',
    level: 48, maxHealth: 5920, baseExp: 1643, jobExp: 1643, mvpExp: 0,
    attack: 434, attack2: 633, defense: 23, magicDefense: 16,
    str: 46, agi: 51, vit: 19, int: 8, dex: 57, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 165, attackDelay: 1120, attackMotion: 576, damageMotion: 420,
    size: 'medium', race: 'demon', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21600,
    raceGroups: {},
    stats: { str: 46, agi: 51, vit: 19, int: 8, dex: 57, luk: 30, level: 48, weaponATK: 434 },
    modes: { detector: true },
    drops: [
        { itemName: 'Tiger Panty', rate: 45 },
        { itemName: 'Little Ghost Doll', rate: 4 },
        { itemName: 'Club', rate: 0.1 },
        { itemName: 'Spike', rate: 0.01 },
        { itemName: 'Jing Guai Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Nightmare (ID: 1061) ──── Level 49 | HP 4,437 | NORMAL | demon/ghost3 | aggressive
RO_MONSTER_TEMPLATES['nightmare'] = {
    id: 1061, name: 'Nightmare', aegisName: 'NIGHTMARE',
    level: 49, maxHealth: 4437, baseExp: 1912, jobExp: 1912, mvpExp: 0,
    attack: 447, attack2: 529, defense: 0, magicDefense: 40,
    str: 0, agi: 74, vit: 25, int: 15, dex: 64, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 150, attackDelay: 1816, attackMotion: 816, damageMotion: 432,
    size: 'large', race: 'demon', element: { type: 'ghost', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21800,
    raceGroups: {},
    stats: { str: 0, agi: 74, vit: 25, int: 15, dex: 64, luk: 10, level: 49, weaponATK: 447 },
    modes: { detector: true },
    drops: [
        { itemName: 'Horseshoe', rate: 60 },
        { itemName: 'Blue Herb', rate: 5 },
        { itemName: 'Rosary', rate: 0.02 },
        { itemName: 'Old Blue Box', rate: 0.3 },
        { itemName: 'Blue Potion', rate: 1 },
        { itemName: 'Infiltrator', rate: 0.01 },
        { itemName: 'Oridecon', rate: 0.6 },
        { itemName: 'Nightmare Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Archer (ID: 1189) ──── Level 49 | HP 7,440 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['orc_archer'] = {
    id: 1189, name: 'Orc Archer', aegisName: 'ORC_ARCHER',
    level: 49, maxHealth: 7440, baseExp: 1729, jobExp: 1787, mvpExp: 0,
    attack: 310, attack2: 390, defense: 10, magicDefense: 5,
    str: 0, agi: 44, vit: 25, int: 20, dex: 125, luk: 20,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 21800,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 44, vit: 25, int: 20, dex: 125, luk: 20, level: 49, weaponATK: 310 },
    modes: {},
    drops: [
        { itemName: 'Fang', rate: 46.56 },
        { itemName: 'Steel Arrow', rate: 10 },
        { itemName: 'Stone Arrow', rate: 25 },
        { itemName: 'Arrow of Wind', rate: 25 },
        { itemName: 'Orc Archer Bow', rate: 0.02 },
        { itemName: 'Red Herb', rate: 14 },
        { itemName: 'White Herb', rate: 9 },
        { itemName: 'Orc Archer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Parasite (ID: 1500) ──── Level 49 | HP 5,188 | NORMAL | plant/wind2 | passive
RO_MONSTER_TEMPLATES['parasite'] = {
    id: 1500, name: 'Parasite', aegisName: 'PARASITE',
    level: 49, maxHealth: 5188, baseExp: 1098, jobExp: 1453, mvpExp: 0,
    attack: 215, attack2: 430, defense: 10, magicDefense: 19,
    str: 0, agi: 40, vit: 30, int: 30, dex: 90, luk: 50,
    attackRange: 400, aggroRange: 0, chaseRange: 600,
    aspd: 183, walkSpeed: 400, attackDelay: 864, attackMotion: 864, damageMotion: 672,
    size: 'medium', race: 'plant', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 21800,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 30, int: 30, dex: 90, luk: 50, level: 49, weaponATK: 215 },
    modes: {},
    drops: [
        { itemName: 'Sprout', rate: 55 },
        { itemName: 'Soft Blade of Grass', rate: 20 },
        { itemName: 'Thin Trunk', rate: 38.8 },
        { itemName: 'Huge Leaf', rate: 5 },
        { itemName: 'Rante Whip', rate: 0.01 },
        { itemName: 'Blade Whip', rate: 0.01 },
        { itemName: 'Shoot', rate: 5 },
        { itemName: 'Parasite Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Green Maiden (ID: 1519) ──── Level 49 | HP 23,900 | NORMAL | demihuman/neutral2 | aggressive
RO_MONSTER_TEMPLATES['chung_e'] = {
    id: 1519, name: 'Green Maiden', aegisName: 'CHUNG_E',
    level: 49, maxHealth: 23900, baseExp: 2396, jobExp: 993, mvpExp: 0,
    attack: 460, attack2: 1050, defense: 8, magicDefense: 15,
    str: 38, agi: 65, vit: 43, int: 30, dex: 90, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 170, attackDelay: 1728, attackMotion: 816, damageMotion: 1188,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21800,
    raceGroups: {},
    stats: { str: 38, agi: 65, vit: 43, int: 30, dex: 90, luk: 15, level: 49, weaponATK: 460 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 48.5 },
        { itemName: 'Puppet', rate: 1 },
        { itemName: 'Studded Knuckles', rate: 0.1 },
        { itemName: 'Honey', rate: 5 },
        { itemName: 'Tantan Noodle', rate: 0.2 },
        { itemName: 'Cake Hat', rate: 0.5 },
        { itemName: 'Bao Bao', rate: 0.02, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plasma (ID: 1696) ──── Level 49 | HP 5,900 | NORMAL | formless/shadow4 | aggressive
RO_MONSTER_TEMPLATES['plasma_p'] = {
    id: 1696, name: 'Plasma', aegisName: 'PLASMA_P',
    level: 49, maxHealth: 5900, baseExp: 2000, jobExp: 1000, mvpExp: 0,
    attack: 300, attack2: 700, defense: 0, magicDefense: 30,
    str: 0, agi: 30, vit: 5, int: 54, dex: 90, luk: 30,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 608, attackMotion: 1440, damageMotion: 576,
    size: 'small', race: 'formless', element: { type: 'shadow', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 21800,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 5, int: 54, dex: 90, luk: 30, level: 49, weaponATK: 300 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.02 },
        { itemName: 'Red Gemstone', rate: 1 },
        { itemName: 'Ruby', rate: 1 },
        { itemName: 'Plasma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baphomet Jr. (ID: 1101) ──── Level 50 | HP 8,578 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['baphomet_'] = {
    id: 1101, name: 'Baphomet Jr.', aegisName: 'BAPHOMET_',
    level: 50, maxHealth: 8578, baseExp: 2706, jobExp: 1480, mvpExp: 0,
    attack: 487, attack2: 590, defense: 15, magicDefense: 25,
    str: 0, agi: 75, vit: 55, int: 0, dex: 93, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 100, attackDelay: 868, attackMotion: 480, damageMotion: 120,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22000,
    raceGroups: {},
    stats: { str: 0, agi: 75, vit: 55, int: 0, dex: 93, luk: 45, level: 50, weaponATK: 487 },
    modes: { detector: true },
    drops: [
        { itemName: 'Evil Horn', rate: 5 },
        { itemName: 'Oridecon', rate: 0.63 },
        { itemName: 'Halberd', rate: 0.02 },
        { itemName: 'Yggdrasil Berry', rate: 0.5 },
        { itemName: 'Yggdrasil Leaf', rate: 1 },
        { itemName: 'Yellow Herb', rate: 13 },
        { itemName: 'Boots', rate: 0.5 },
        { itemName: 'Baphomet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dryad (ID: 1493) ──── Level 50 | HP 8,791 | NORMAL | plant/earth4 | aggressive
RO_MONSTER_TEMPLATES['dryad'] = {
    id: 1493, name: 'Dryad', aegisName: 'DRYAD',
    level: 50, maxHealth: 8791, baseExp: 2763, jobExp: 1493, mvpExp: 0,
    attack: 499, attack2: 589, defense: 15, magicDefense: 33,
    str: 0, agi: 75, vit: 55, int: 0, dex: 78, luk: 45,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 170, attackDelay: 950, attackMotion: 2520, damageMotion: 576,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22000,
    raceGroups: {},
    stats: { str: 0, agi: 75, vit: 55, int: 0, dex: 78, luk: 45, level: 50, weaponATK: 499 },
    modes: {},
    drops: [
        { itemName: 'Tough Vines', rate: 53.35 },
        { itemName: 'Huge Leaf', rate: 10 },
        { itemName: 'Brown Root', rate: 30 },
        { itemName: 'Rope', rate: 0.8 },
        { itemName: 'Chemeti Whip', rate: 0.01 },
        { itemName: 'Romantic Leaf', rate: 0.1 },
        { itemName: 'Sharp Leaf', rate: 30 },
        { itemName: 'Dryad Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kraben (ID: 1587) ──── Level 50 | HP 5,880 | NORMAL | formless/ghost2 | aggressive
RO_MONSTER_TEMPLATES['kraben'] = {
    id: 1587, name: 'Kraben', aegisName: 'KRABEN',
    level: 50, maxHealth: 5880, baseExp: 206, jobExp: 1322, mvpExp: 0,
    attack: 125, attack2: 765, defense: 5, magicDefense: 42,
    str: 50, agi: 125, vit: 0, int: 66, dex: 75, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 100, attackDelay: 1152, attackMotion: 1536, damageMotion: 576,
    size: 'medium', race: 'formless', element: { type: 'ghost', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22000,
    raceGroups: {},
    stats: { str: 50, agi: 125, vit: 0, int: 66, dex: 75, luk: 50, level: 50, weaponATK: 125 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 35 },
        { itemName: 'Milk', rate: 30 },
        { itemName: 'Aloe Leaflet', rate: 10 },
        { itemName: 'Guard', rate: 0.01 },
        { itemName: 'Straw Basket', rate: 48.5 },
        { itemName: 'Red Chile', rate: 10 },
        { itemName: 'Old Blue Box', rate: 0.1 },
        { itemName: 'Kraben Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Obsidian (ID: 1615) ──── Level 50 | HP 8,812 | NORMAL | formless/earth2 | aggressive
RO_MONSTER_TEMPLATES['obsidian'] = {
    id: 1615, name: 'Obsidian', aegisName: 'OBSIDIAN',
    level: 50, maxHealth: 8812, baseExp: 2799, jobExp: 1802, mvpExp: 0,
    attack: 841, attack2: 980, defense: 35, magicDefense: 5,
    str: 62, agi: 32, vit: 42, int: 24, dex: 61, luk: 55,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 350, attackDelay: 720, attackMotion: 864, damageMotion: 504,
    size: 'small', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22000,
    raceGroups: {},
    stats: { str: 62, agi: 32, vit: 42, int: 24, dex: 61, luk: 55, level: 50, weaponATK: 841 },
    modes: {},
    drops: [
        { itemName: 'Dark Crystal Fragment', rate: 30 },
        { itemName: '1carat Diamond', rate: 5 },
        { itemName: 'Coal', rate: 5 },
        { itemName: 'Elunium', rate: 0.5 },
        { itemName: 'Steel', rate: 5 },
        { itemName: 'Unholy Touch', rate: 0.1 },
        { itemName: 'Obsidian Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Knocker (ID: 1838) ──── Level 50 | HP 7,755 | NORMAL | demon/earth1 | aggressive
RO_MONSTER_TEMPLATES['knocker'] = {
    id: 1838, name: 'Knocker', aegisName: 'KNOCKER',
    level: 50, maxHealth: 7755, baseExp: 2202, jobExp: 4023, mvpExp: 0,
    attack: 889, attack2: 990, defense: 28, magicDefense: 50,
    str: 25, agi: 44, vit: 50, int: 62, dex: 65, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 200, attackDelay: 1548, attackMotion: 384, damageMotion: 288,
    size: 'small', race: 'demon', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22000,
    raceGroups: {},
    stats: { str: 25, agi: 44, vit: 50, int: 62, dex: 65, luk: 60, level: 50, weaponATK: 889 },
    modes: { detector: true },
    drops: [
        { itemName: 'Great Nature', rate: 0.3 },
        { itemName: 'Coal', rate: 1.5 },
        { itemName: 'Elder Pixie\'s Beard', rate: 55 },
        { itemName: 'Elven Ears', rate: 0.01 },
        { itemName: 'Ribbon', rate: 0.1 },
        { itemName: 'Thorny Buckler', rate: 0.03 },
        { itemName: 'Earth Bow', rate: 0.05 },
        { itemName: 'Knocker Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Nine Tail (ID: 1180) ──── Level 51 | HP 7,766 | NORMAL | brute/fire3 | aggressive
RO_MONSTER_TEMPLATES['nine_tail'] = {
    id: 1180, name: 'Nine Tail', aegisName: 'NINE_TAIL',
    level: 51, maxHealth: 7766, baseExp: 2812, jobExp: 825, mvpExp: 0,
    attack: 610, attack2: 734, defense: 10, magicDefense: 25,
    str: 0, agi: 80, vit: 46, int: 0, dex: 74, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 150, attackDelay: 840, attackMotion: 540, damageMotion: 480,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 0, agi: 80, vit: 46, int: 0, dex: 74, luk: 85, level: 51, weaponATK: 610 },
    modes: {},
    drops: [
        { itemName: 'Nine Tails', rate: 46.56 },
        { itemName: 'Glass Bead', rate: 2 },
        { itemName: 'Old Blue Box', rate: 1 },
        { itemName: 'Dead Branch', rate: 1 },
        { itemName: 'Royal Jelly', rate: 2.5 },
        { itemName: 'Panacea', rate: 3.5 },
        { itemName: 'Rough Oridecon', rate: 1 },
        { itemName: 'Nine Tail Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mimic (ID: 1191) ──── Level 51 | HP 6,120 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['mimic'] = {
    id: 1191, name: 'Mimic', aegisName: 'MIMIC',
    level: 51, maxHealth: 6120, baseExp: 165, jobExp: 165, mvpExp: 0,
    attack: 150, attack2: 900, defense: 10, magicDefense: 40,
    str: 44, agi: 121, vit: 0, int: 60, dex: 75, luk: 110,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 100, attackDelay: 972, attackMotion: 500, damageMotion: 288,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 44, agi: 121, vit: 0, int: 60, dex: 75, luk: 110, level: 51, weaponATK: 150 },
    modes: {},
    drops: [
        { itemName: 'Old Purple Box', rate: 0.05 },
        { itemName: 'Old Blue Box', rate: 0.45 },
        { itemName: 'Trap', rate: 12 },
        { itemName: 'Magnifier', rate: 30 },
        { itemName: 'Emperium', rate: 0.03 },
        { itemName: 'Rosary', rate: 0.01 },
        { itemName: 'Rough Elunium', rate: 2.7 },
        { itemName: 'Mimic Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Injustice (ID: 1257) ──── Level 51 | HP 7,600 | NORMAL | undead/shadow2 | aggressive
RO_MONSTER_TEMPLATES['injustice'] = {
    id: 1257, name: 'Injustice', aegisName: 'INJUSTICE',
    level: 51, maxHealth: 7600, baseExp: 2118, jobExp: 1488, mvpExp: 0,
    attack: 480, attack2: 600, defense: 0, magicDefense: 0,
    str: 84, agi: 42, vit: 39, int: 0, dex: 71, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 400, attackDelay: 770, attackMotion: 720, damageMotion: 336,
    size: 'medium', race: 'undead', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 84, agi: 42, vit: 39, int: 0, dex: 71, luk: 35, level: 51, weaponATK: 480 },
    modes: {},
    drops: [
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Cyfar', rate: 35 },
        { itemName: 'Padded Armor', rate: 0.05 },
        { itemName: 'Full Plate', rate: 0.02 },
        { itemName: 'Forbidden Red Candle', rate: 0.02 },
        { itemName: 'Jamadhar', rate: 0.02 },
        { itemName: 'Injustice Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wind Ghost (ID: 1263) ──── Level 51 | HP 4,820 | NORMAL | demon/wind3 | aggressive
RO_MONSTER_TEMPLATES['wind_ghost'] = {
    id: 1263, name: 'Wind Ghost', aegisName: 'WIND_GHOST',
    level: 51, maxHealth: 4820, baseExp: 2424, jobExp: 1488, mvpExp: 0,
    attack: 489, attack2: 639, defense: 0, magicDefense: 45,
    str: 0, agi: 89, vit: 15, int: 90, dex: 85, luk: 25,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 150, attackDelay: 1056, attackMotion: 1056, damageMotion: 336,
    size: 'medium', race: 'demon', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 0, agi: 89, vit: 15, int: 90, dex: 85, luk: 25, level: 51, weaponATK: 489 },
    modes: { detector: true },
    drops: [
        { itemName: 'Zargon', rate: 45.59 },
        { itemName: 'Skel-Bone', rate: 60 },
        { itemName: 'Skull', rate: 5 },
        { itemName: 'Level 5 Lightening Bolt', rate: 1 },
        { itemName: 'Arc Wand', rate: 0.08 },
        { itemName: 'Rough Wind', rate: 1 },
        { itemName: 'Evil Bone Wand', rate: 0.01 },
        { itemName: 'Wind Ghost Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Carat (ID: 1267) ──── Level 51 | HP 5,200 | NORMAL | demon/wind2 | aggressive
RO_MONSTER_TEMPLATES['carat'] = {
    id: 1267, name: 'Carat', aegisName: 'CARAT',
    level: 51, maxHealth: 5200, baseExp: 1926, jobExp: 1353, mvpExp: 0,
    attack: 330, attack2: 417, defense: 0, magicDefense: 25,
    str: 0, agi: 41, vit: 45, int: 5, dex: 85, luk: 155,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1078, attackMotion: 768, damageMotion: 384,
    size: 'medium', race: 'demon', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 0, agi: 41, vit: 45, int: 5, dex: 85, luk: 155, level: 51, weaponATK: 330 },
    modes: { detector: true },
    drops: [
        { itemName: 'Brigan', rate: 32 },
        { itemName: 'Ice Cream', rate: 10 },
        { itemName: 'High Heels', rate: 0.05 },
        { itemName: 'Joker Jester', rate: 0.01 },
        { itemName: 'White Herb', rate: 14.5 },
        { itemName: 'Carat Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wooden Golem (ID: 1497) ──── Level 51 | HP 9,200 | NORMAL | plant/neutral1 | passive
RO_MONSTER_TEMPLATES['wooden_golem'] = {
    id: 1497, name: 'Wooden Golem', aegisName: 'WOODEN_GOLEM',
    level: 51, maxHealth: 9200, baseExp: 1926, jobExp: 1353, mvpExp: 0,
    attack: 570, attack2: 657, defense: 32, magicDefense: 36,
    str: 0, agi: 41, vit: 69, int: 5, dex: 85, luk: 155,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'plant', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 22200,
    raceGroups: { Golem: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 41, vit: 69, int: 5, dex: 85, luk: 155, level: 51, weaponATK: 570 },
    modes: {},
    drops: [
        { itemName: 'Wooden Heart', rate: 40 },
        { itemName: 'Brown Root', rate: 40 },
        { itemName: 'Rough Elunium', rate: 1.1 },
        { itemName: 'Romantic Leaf', rate: 0.1 },
        { itemName: 'Dead Branch', rate: 1 },
        { itemName: 'Log', rate: 50 },
        { itemName: 'Mushroom Spore', rate: 10 },
        { itemName: 'Wooden Golem Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Heirozoist (ID: 1510) ──── Level 51 | HP 7,186 | NORMAL | demon/shadow2 | aggressive
RO_MONSTER_TEMPLATES['hylozoist'] = {
    id: 1510, name: 'Heirozoist', aegisName: 'HYLOZOIST',
    level: 51, maxHealth: 7186, baseExp: 2314, jobExp: 1297, mvpExp: 0,
    attack: 317, attack2: 498, defense: 16, magicDefense: 51,
    str: 0, agi: 28, vit: 26, int: 47, dex: 66, luk: 14,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 155, attackDelay: 741, attackMotion: 1536, damageMotion: 480,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 0, agi: 28, vit: 26, int: 47, dex: 66, luk: 14, level: 51, weaponATK: 317 },
    modes: { detector: true },
    drops: [
        { itemName: 'Broken Needle', rate: 43.65 },
        { itemName: 'Spool', rate: 53.35 },
        { itemName: 'Needle Packet', rate: 20 },
        { itemName: 'Puppet', rate: 0.8 },
        { itemName: 'Ectoplasm', rate: 3 },
        { itemName: 'Rough Elunium', rate: 0.1 },
        { itemName: 'Angry Mouth', rate: 0.01 },
        { itemName: 'Hylozoist Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mi Gao (ID: 1516) ──── Level 51 | HP 8,230 | NORMAL | formless/earth3 | aggressive
RO_MONSTER_TEMPLATES['increase_soil'] = {
    id: 1516, name: 'Mi Gao', aegisName: 'INCREASE_SOIL',
    level: 51, maxHealth: 8230, baseExp: 2760, jobExp: 2110, mvpExp: 0,
    attack: 560, attack2: 700, defense: 30, magicDefense: 12,
    str: 40, agi: 45, vit: 23, int: 12, dex: 69, luk: 12,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 445, attackDelay: 106, attackMotion: 1056, damageMotion: 576,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22200,
    raceGroups: {},
    stats: { str: 40, agi: 45, vit: 23, int: 12, dex: 69, luk: 12, level: 51, weaponATK: 560 },
    modes: {},
    drops: [
        { itemName: 'Dry Sand', rate: 43.65 },
        { itemName: 'Mud Lump', rate: 23 },
        { itemName: 'Great Nature', rate: 0.1 },
        { itemName: 'Gold', rate: 0.02 },
        { itemName: 'Mi Gao Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Minorous (ID: 1149) ──── Level 52 | HP 7,431 | NORMAL | brute/fire2 | aggressive
RO_MONSTER_TEMPLATES['minorous'] = {
    id: 1149, name: 'Minorous', aegisName: 'MINOROUS',
    level: 52, maxHealth: 7431, baseExp: 2750, jobExp: 1379, mvpExp: 0,
    attack: 590, attack2: 770, defense: 15, magicDefense: 5,
    str: 65, agi: 42, vit: 61, int: 66, dex: 52, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 200, attackDelay: 1360, attackMotion: 960, damageMotion: 432,
    size: 'large', race: 'brute', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 65, agi: 42, vit: 61, int: 66, dex: 52, luk: 25, level: 52, weaponATK: 590 },
    modes: {},
    drops: [
        { itemName: 'Nose Ring', rate: 53.35 },
        { itemName: 'Rough Oridecon', rate: 1.96 },
        { itemName: 'Two-Handed Axe', rate: 0.02 },
        { itemName: 'Hammer of Blacksmith', rate: 0.1 },
        { itemName: 'Sweet Potato', rate: 15 },
        { itemName: 'Axe', rate: 2 },
        { itemName: 'Lemon', rate: 3 },
        { itemName: 'Minorous Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Raydric (ID: 1163) ──── Level 52 | HP 8,613 | NORMAL | demihuman/shadow2 | aggressive
RO_MONSTER_TEMPLATES['raydric'] = {
    id: 1163, name: 'Raydric', aegisName: 'RAYDRIC',
    level: 52, maxHealth: 8613, baseExp: 3410, jobExp: 1795, mvpExp: 0,
    attack: 830, attack2: 930, defense: 40, magicDefense: 15,
    str: 58, agi: 47, vit: 42, int: 5, dex: 69, luk: 26,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 150, attackDelay: 824, attackMotion: 780, damageMotion: 420,
    size: 'large', race: 'demihuman', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 58, agi: 47, vit: 42, int: 5, dex: 69, luk: 26, level: 52, weaponATK: 830 },
    modes: {},
    drops: [
        { itemName: 'Elunium', rate: 1.06 },
        { itemName: 'Iron Cain', rate: 0.01 },
        { itemName: 'Chain Mail', rate: 0.02 },
        { itemName: 'Two-Handed Sword', rate: 0.02 },
        { itemName: 'Katana', rate: 1 },
        { itemName: 'Chivalry Emblem', rate: 0.1 },
        { itemName: 'Brigan', rate: 48.5 },
        { itemName: 'Raydric Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skeleton Prisoner (ID: 1196) ──── Level 52 | HP 8,691 | NORMAL | undead/undead3 | aggressive
RO_MONSTER_TEMPLATES['skel_prisoner'] = {
    id: 1196, name: 'Skeleton Prisoner', aegisName: 'SKEL_PRISONER',
    level: 52, maxHealth: 8691, baseExp: 2466, jobExp: 1562, mvpExp: 0,
    attack: 660, attack2: 890, defense: 10, magicDefense: 20,
    str: 55, agi: 20, vit: 36, int: 0, dex: 76, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 350, attackDelay: 1848, attackMotion: 500, damageMotion: 576,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 55, agi: 20, vit: 36, int: 0, dex: 76, luk: 25, level: 52, weaponATK: 660 },
    modes: {},
    drops: [
        { itemName: 'Manacles', rate: 35 },
        { itemName: 'Spoon Stub', rate: 1 },
        { itemName: 'Formal Suit', rate: 0.01 },
        { itemName: 'Red Gemstone', rate: 6 },
        { itemName: 'Rotten Bandage', rate: 35 },
        { itemName: 'Shackles', rate: 0.35 },
        { itemName: 'Memento', rate: 15 },
        { itemName: 'Skeleton Prisoner Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── High Orc (ID: 1213) ──── Level 52 | HP 6,890 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['high_orc'] = {
    id: 1213, name: 'High Orc', aegisName: 'HIGH_ORC',
    level: 52, maxHealth: 6890, baseExp: 3618, jobExp: 1639, mvpExp: 0,
    attack: 428, attack2: 533, defense: 15, magicDefense: 5,
    str: 55, agi: 46, vit: 55, int: 35, dex: 82, luk: 40,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 22400,
    raceGroups: { Orc: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 55, agi: 46, vit: 55, int: 35, dex: 82, luk: 40, level: 52, weaponATK: 428 },
    modes: {},
    drops: [
        { itemName: 'Ogre Tooth', rate: 25 },
        { itemName: 'Orcish Axe', rate: 0.1 },
        { itemName: 'Steel', rate: 0.9 },
        { itemName: 'Orcish Voucher', rate: 75 },
        { itemName: 'Zargon', rate: 13 },
        { itemName: 'Rough Oridecon', rate: 1.96 },
        { itemName: 'Yellow Herb', rate: 9 },
        { itemName: 'High Orc Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Raydric Archer (ID: 1276) ──── Level 52 | HP 5,250 | NORMAL | demon/shadow2 | aggressive
RO_MONSTER_TEMPLATES['raydric_archer'] = {
    id: 1276, name: 'Raydric Archer', aegisName: 'RAYDRIC_ARCHER',
    level: 52, maxHealth: 5250, baseExp: 3025, jobExp: 2125, mvpExp: 0,
    attack: 415, attack2: 500, defense: 35, magicDefense: 5,
    str: 15, agi: 25, vit: 22, int: 5, dex: 145, luk: 35,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 200, attackDelay: 1152, attackMotion: 1152, damageMotion: 480,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 15, agi: 25, vit: 22, int: 5, dex: 145, luk: 35, level: 52, weaponATK: 415 },
    modes: { detector: true },
    drops: [
        { itemName: 'Brigan', rate: 46.56 },
        { itemName: 'Chain Mail', rate: 0.02 },
        { itemName: 'Bow', rate: 1.5 },
        { itemName: 'Sharp Arrow', rate: 20 },
        { itemName: 'Arbalest', rate: 0.03 },
        { itemName: 'Elunium', rate: 1.06 },
        { itemName: 'Raydric Archer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Driller (ID: 1380) ──── Level 52 | HP 7,452 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['driller'] = {
    id: 1380, name: 'Driller', aegisName: 'DRILLER',
    level: 52, maxHealth: 7452, baseExp: 3215, jobExp: 1860, mvpExp: 0,
    attack: 666, attack2: 886, defense: 48, magicDefense: 31,
    str: 0, agi: 66, vit: 58, int: 50, dex: 60, luk: 47,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 165, attackDelay: 1300, attackMotion: 900, damageMotion: 336,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 58, int: 50, dex: 60, luk: 47, level: 52, weaponATK: 666 },
    modes: {},
    drops: [
        { itemName: 'Frill', rate: 75 },
        { itemName: 'Yellow Gemstone', rate: 38.8 },
        { itemName: 'Red Gemstone', rate: 35 },
        { itemName: 'Driller Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Tamruan (ID: 1584) ──── Level 52 | HP 10,234 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['tamruan'] = {
    id: 1584, name: 'Tamruan', aegisName: 'TAMRUAN',
    level: 52, maxHealth: 10234, baseExp: 3812, jobExp: 55, mvpExp: 0,
    attack: 489, attack2: 534, defense: 15, magicDefense: 35,
    str: 80, agi: 62, vit: 38, int: 75, dex: 72, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 140, attackDelay: 512, attackMotion: 1152, damageMotion: 672,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22400,
    raceGroups: {},
    stats: { str: 80, agi: 62, vit: 38, int: 75, dex: 72, luk: 15, level: 52, weaponATK: 489 },
    modes: { detector: true },
    drops: [
        { itemName: 'Tassel', rate: 48.5 },
        { itemName: 'Destroyed Armor', rate: 30 },
        { itemName: 'Katana', rate: 0.4 },
        { itemName: 'Bastard Sword', rate: 0.08 },
        { itemName: 'Chain Mail', rate: 0.03 },
        { itemName: 'Tamruan Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wraith (ID: 1192) ──── Level 53 | HP 10,999 | NORMAL | undead/undead4 | aggressive
RO_MONSTER_TEMPLATES['wraith'] = {
    id: 1192, name: 'Wraith', aegisName: 'WRAITH',
    level: 53, maxHealth: 10999, baseExp: 2199, jobExp: 1099, mvpExp: 0,
    attack: 580, attack2: 760, defense: 5, magicDefense: 30,
    str: 0, agi: 95, vit: 30, int: 65, dex: 95, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 300, attackDelay: 1816, attackMotion: 576, damageMotion: 240,
    size: 'large', race: 'undead', element: { type: 'undead', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22600,
    raceGroups: {},
    stats: { str: 0, agi: 95, vit: 30, int: 65, dex: 95, luk: 35, level: 53, weaponATK: 580 },
    modes: {},
    drops: [
        { itemName: 'Fabric', rate: 58.2 },
        { itemName: 'Wedding Veil', rate: 0.1 },
        { itemName: 'Manteau', rate: 0.02 },
        { itemName: 'Red Gemstone', rate: 6.5 },
        { itemName: 'Butterfly Wing', rate: 13 },
        { itemName: 'Manteau', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.05 },
        { itemName: 'Wraith Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zombie Prisoner (ID: 1197) ──── Level 53 | HP 11,280 | NORMAL | undead/undead3 | aggressive
RO_MONSTER_TEMPLATES['zombie_prisoner'] = {
    id: 1197, name: 'Zombie Prisoner', aegisName: 'ZOMBIE_PRISONER',
    level: 53, maxHealth: 11280, baseExp: 2635, jobExp: 1724, mvpExp: 0,
    attack: 780, attack2: 930, defense: 10, magicDefense: 20,
    str: 0, agi: 24, vit: 39, int: 0, dex: 72, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 350, attackDelay: 1768, attackMotion: 500, damageMotion: 192,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22600,
    raceGroups: {},
    stats: { str: 0, agi: 24, vit: 39, int: 0, dex: 72, luk: 25, level: 53, weaponATK: 780 },
    modes: {},
    drops: [
        { itemName: 'Worn-out Prison Uniform', rate: 35 },
        { itemName: 'Spoon Stub', rate: 1.05 },
        { itemName: 'Iron Cain', rate: 0.01 },
        { itemName: 'Red Gemstone', rate: 6 },
        { itemName: 'Rotten Bandage', rate: 35 },
        { itemName: 'Shackles', rate: 0.39 },
        { itemName: 'Elunium', rate: 1.12 },
        { itemName: 'Zombie Prisoner Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Merman (ID: 1264) ──── Level 53 | HP 14,690 | NORMAL | demihuman/water3 | aggressive
RO_MONSTER_TEMPLATES['merman'] = {
    id: 1264, name: 'Merman', aegisName: 'MERMAN',
    level: 53, maxHealth: 14690, baseExp: 4500, jobExp: 3000, mvpExp: 0,
    attack: 482, attack2: 964, defense: 10, magicDefense: 35,
    str: 72, agi: 45, vit: 46, int: 35, dex: 60, luk: 55,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 220, attackDelay: 916, attackMotion: 816, damageMotion: 336,
    size: 'medium', race: 'demihuman', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22600,
    raceGroups: {},
    stats: { str: 72, agi: 45, vit: 46, int: 35, dex: 60, luk: 55, level: 53, weaponATK: 482 },
    modes: {},
    drops: [
        { itemName: 'Ancient Lips', rate: 13 },
        { itemName: 'Holy Water', rate: 3 },
        { itemName: 'Lemon', rate: 4 },
        { itemName: 'Aquamarine', rate: 0.4 },
        { itemName: 'Mystic Frozen', rate: 0.35 },
        { itemName: 'Trident', rate: 0.03 },
        { itemName: 'Rough Oridecon', rate: 2.03 },
        { itemName: 'Merman Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Enchanted Peach Tree (ID: 1410) ──── Level 53 | HP 8,905 | NORMAL | plant/earth2 | aggressive
RO_MONSTER_TEMPLATES['live_peach_tree'] = {
    id: 1410, name: 'Enchanted Peach Tree', aegisName: 'LIVE_PEACH_TREE',
    level: 53, maxHealth: 8905, baseExp: 2591, jobExp: 1799, mvpExp: 0,
    attack: 301, attack2: 351, defense: 10, magicDefense: 38,
    str: 72, agi: 45, vit: 35, int: 39, dex: 80, luk: 5,
    attackRange: 350, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 410, attackDelay: 400, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22600,
    raceGroups: {},
    stats: { str: 72, agi: 45, vit: 35, int: 39, dex: 80, luk: 5, level: 53, weaponATK: 301 },
    modes: {},
    drops: [
        { itemName: 'Solid Peach', rate: 43.65 },
        { itemName: 'Royal Jelly', rate: 10 },
        { itemName: 'Dead Branch', rate: 4 },
        { itemName: 'Banana Juice', rate: 1 },
        { itemName: 'Old Blue Box', rate: 0.05 },
        { itemName: 'Enchanted Peach Tree Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gremlin (ID: 1632) ──── Level 53 | HP 9,280 | NORMAL | demon/shadow2 | aggressive
RO_MONSTER_TEMPLATES['gremlin'] = {
    id: 1632, name: 'Gremlin', aegisName: 'GREMLIN',
    level: 53, maxHealth: 9280, baseExp: 4355, jobExp: 1768, mvpExp: 0,
    attack: 329, attack2: 762, defense: 29, magicDefense: 25,
    str: 80, agi: 41, vit: 59, int: 75, dex: 62, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 140, attackDelay: 432, attackMotion: 540, damageMotion: 432,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 22600,
    raceGroups: {},
    stats: { str: 80, agi: 41, vit: 59, int: 75, dex: 62, luk: 15, level: 53, weaponATK: 329 },
    modes: { detector: true },
    drops: [
        { itemName: 'Will of the Darkness', rate: 30 },
        { itemName: 'Sticky Mucus', rate: 30 },
        { itemName: 'Amethyst', rate: 1 },
        { itemName: 'Boots', rate: 0.01 },
        { itemName: 'Bloody Roar', rate: 0.01 },
        { itemName: 'Old Blue Box', rate: 0.02 },
        { itemName: 'Gremlin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zhu Po Long (ID: 1514) ──── Level 54 | HP 9,136 | NORMAL | dragon/wind2 | passive
RO_MONSTER_TEMPLATES['dancing_dragon'] = {
    id: 1514, name: 'Zhu Po Long', aegisName: 'DANCING_DRAGON',
    level: 54, maxHealth: 9136, baseExp: 3030, jobExp: 769, mvpExp: 0,
    attack: 550, attack2: 789, defense: 39, magicDefense: 10,
    str: 55, agi: 62, vit: 55, int: 25, dex: 72, luk: 22,
    attackRange: 100, aggroRange: 0, chaseRange: 600,
    aspd: 188, walkSpeed: 160, attackDelay: 600, attackMotion: 840, damageMotion: 504,
    size: 'medium', race: 'dragon', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 22800,
    raceGroups: {},
    stats: { str: 55, agi: 62, vit: 55, int: 25, dex: 72, luk: 22, level: 54, weaponATK: 550 },
    modes: {},
    drops: [
        { itemName: 'Denture from Dragon Mask', rate: 43.65 },
        { itemName: 'Dragon Horn', rate: 30 },
        { itemName: 'Little Ghost Doll', rate: 8 },
        { itemName: 'Dragon Scale', rate: 10 },
        { itemName: 'Yarn', rate: 30 },
        { itemName: 'Zhu Po Long Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Grove (ID: 1687) ──── Level 54 | HP 6,444 | NORMAL | brute/earth2 | passive
RO_MONSTER_TEMPLATES['green_iguana'] = {
    id: 1687, name: 'Grove', aegisName: 'GREEN_IGUANA',
    level: 54, maxHealth: 6444, baseExp: 2400, jobExp: 2050, mvpExp: 0,
    attack: 550, attack2: 650, defense: 0, magicDefense: 10,
    str: 0, agi: 52, vit: 64, int: 5, dex: 98, luk: 14,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 186, walkSpeed: 200, attackDelay: 720, attackMotion: 528, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 22800,
    raceGroups: {},
    stats: { str: 0, agi: 52, vit: 64, int: 5, dex: 98, luk: 14, level: 54, weaponATK: 550 },
    modes: {},
    drops: [
        { itemName: 'Aloe Leaflet', rate: 15 },
        { itemName: 'Reptile Tongue', rate: 10 },
        { itemName: 'Hinalle Leaflet', rate: 10 },
        { itemName: 'Green Herb', rate: 10 },
        { itemName: 'Monster\'s Feed', rate: 20 },
        { itemName: 'Aloevera', rate: 0.1 },
        { itemName: 'Green Herb', rate: 0.01 },
        { itemName: 'Grove Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Giant Spider (ID: 1304) ──── Level 55 | HP 11,874 | NORMAL | insect/poison1 | aggressive
RO_MONSTER_TEMPLATES['giant_spider'] = {
    id: 1304, name: 'Giant Spider', aegisName: 'GIANT_SPIDER',
    level: 55, maxHealth: 11874, baseExp: 6211, jobExp: 2146, mvpExp: 0,
    attack: 624, attack2: 801, defense: 41, magicDefense: 28,
    str: 5, agi: 36, vit: 43, int: 5, dex: 73, luk: 69,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 165, attackDelay: 1468, attackMotion: 468, damageMotion: 768,
    size: 'large', race: 'insect', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 5, agi: 36, vit: 43, int: 5, dex: 73, luk: 69, level: 55, weaponATK: 624 },
    modes: { detector: true },
    drops: [
        { itemName: 'Cobweb', rate: 45.5 },
        { itemName: 'Bug Leg', rate: 12 },
        { itemName: 'Rough Elunium', rate: 1.4 },
        { itemName: 'Panacea', rate: 4.5 },
        { itemName: 'Solid Shell', rate: 12 },
        { itemName: 'Round Shell', rate: 6.8 },
        { itemName: 'Cyfar', rate: 8 },
        { itemName: 'Giant Spider Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bloody Butterfly (ID: 1408) ──── Level 55 | HP 8,082 | NORMAL | insect/wind2 | aggressive
RO_MONSTER_TEMPLATES['blood_butterfly'] = {
    id: 1408, name: 'Bloody Butterfly', aegisName: 'BLOOD_BUTTERFLY',
    level: 55, maxHealth: 8082, baseExp: 2119, jobExp: 1562, mvpExp: 0,
    attack: 121, attack2: 342, defense: 5, magicDefense: 23,
    str: 0, agi: 59, vit: 14, int: 55, dex: 68, luk: 15,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 145, attackDelay: 472, attackMotion: 576, damageMotion: 288,
    size: 'medium', race: 'insect', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 0, agi: 59, vit: 14, int: 55, dex: 68, luk: 15, level: 55, weaponATK: 121 },
    modes: { detector: true },
    drops: [
        { itemName: 'Hard Feeler', rate: 46.08 },
        { itemName: 'Giant Butterfly Wing', rate: 25 },
        { itemName: 'Butterfly Wing', rate: 12 },
        { itemName: 'Powder of Butterfly', rate: 55 },
        { itemName: 'Waghnak', rate: 0.03 },
        { itemName: 'Lariat Whip', rate: 0.01 },
        { itemName: 'Bloody Butterfly Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Disguise (ID: 1506) ──── Level 55 | HP 7,543 | NORMAL | demon/earth4 | aggressive
RO_MONSTER_TEMPLATES['disguise'] = {
    id: 1506, name: 'Disguise', aegisName: 'DISGUISE',
    level: 55, maxHealth: 7543, baseExp: 2815, jobExp: 1919, mvpExp: 0,
    attack: 279, attack2: 546, defense: 18, magicDefense: 29,
    str: 0, agi: 72, vit: 45, int: 35, dex: 48, luk: 65,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 147, attackDelay: 516, attackMotion: 768, damageMotion: 384,
    size: 'medium', race: 'demon', element: { type: 'earth', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 0, agi: 72, vit: 45, int: 35, dex: 48, luk: 65, level: 55, weaponATK: 279 },
    modes: { detector: true },
    drops: [
        { itemName: 'Red Scarf', rate: 48.5 },
        { itemName: 'Tangled Chains', rate: 36.86 },
        { itemName: 'Hood', rate: 0.5 },
        { itemName: 'Honey', rate: 1 },
        { itemName: 'Ragamuffin Manteau', rate: 0.5 },
        { itemName: 'Muffler', rate: 0.02 },
        { itemName: 'Rider Insignia', rate: 0.05 },
        { itemName: 'Disguise Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Remover (ID: 1682) ──── Level 55 | HP 10,289 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['removal'] = {
    id: 1682, name: 'Remover', aegisName: 'REMOVAL',
    level: 55, maxHealth: 10289, baseExp: 3831, jobExp: 1278, mvpExp: 0,
    attack: 558, attack2: 797, defense: 5, magicDefense: 20,
    str: 0, agi: 20, vit: 56, int: 35, dex: 57, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 250, attackDelay: 1536, attackMotion: 1056, damageMotion: 1152,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 56, int: 35, dex: 57, luk: 20, level: 55, weaponATK: 558 },
    modes: {},
    drops: [
        { itemName: 'Empty Bottle', rate: 50 },
        { itemName: 'Used Iron Plate', rate: 50 },
        { itemName: 'Gas Mask', rate: 0.1 },
        { itemName: 'Yam', rate: 5 },
        { itemName: 'Detrimindexta', rate: 0.5 },
        { itemName: 'Karvodailnirol', rate: 1 },
        { itemName: 'Bucket Hat', rate: 0.06 },
        { itemName: 'Remover Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Constant (ID: 1738) ──── Level 55 | HP 10,000 | NORMAL | formless/shadow3 | aggressive
RO_MONSTER_TEMPLATES['constant'] = {
    id: 1738, name: 'Constant', aegisName: 'CONSTANT',
    level: 55, maxHealth: 10000, baseExp: 3230, jobExp: 116, mvpExp: 0,
    attack: 460, attack2: 580, defense: 12, magicDefense: 12,
    str: 50, agi: 28, vit: 26, int: 47, dex: 66, luk: 14,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 150, attackDelay: 720, attackMotion: 360, damageMotion: 360,
    size: 'small', race: 'formless', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 50, agi: 28, vit: 26, int: 47, dex: 66, luk: 14, level: 55, weaponATK: 460 },
    modes: {},
    drops: [
        { itemName: 'Burnt Part', rate: 1 },
        { itemName: 'Solid Iron Piece', rate: 15 },
        { itemName: 'Flexible Tube', rate: 0.1 },
        { itemName: 'Steel', rate: 0.1 },
        { itemName: 'Rough Elunium', rate: 0.1 },
    ],
    mvpDrops: [],
};

// ──── Gazeti (ID: 1778) ──── Level 55 | HP 12,300 | NORMAL | demon/water1 | aggressive
RO_MONSTER_TEMPLATES['gazeti'] = {
    id: 1778, name: 'Gazeti', aegisName: 'GAZETI',
    level: 55, maxHealth: 12300, baseExp: 5758, jobExp: 2075, mvpExp: 0,
    attack: 512, attack2: 612, defense: 65, magicDefense: 25,
    str: 0, agi: 12, vit: 20, int: 60, dex: 101, luk: 5,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 190, attackDelay: 576, attackMotion: 370, damageMotion: 270,
    size: 'medium', race: 'demon', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23000,
    raceGroups: {},
    stats: { str: 0, agi: 12, vit: 20, int: 60, dex: 101, luk: 5, level: 55, weaponATK: 512 },
    modes: { detector: true },
    drops: [
        { itemName: 'Glacial Heart', rate: 30 },
        { itemName: 'Ice Piece', rate: 30 },
        { itemName: 'Elunium', rate: 0.2 },
        { itemName: 'Frozen Bow', rate: 0.01 },
        { itemName: 'Gazeti Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cramp (ID: 1209) ──── Level 56 | HP 4,720 | NORMAL | brute/poison2 | aggressive
RO_MONSTER_TEMPLATES['cramp'] = {
    id: 1209, name: 'Cramp', aegisName: 'CRAMP',
    level: 56, maxHealth: 4720, baseExp: 2300, jobExp: 1513, mvpExp: 0,
    attack: 395, attack2: 465, defense: 0, magicDefense: 5,
    str: 0, agi: 85, vit: 35, int: 5, dex: 65, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 100, attackDelay: 1000, attackMotion: 500, damageMotion: 1000,
    size: 'small', race: 'brute', element: { type: 'poison', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 85, vit: 35, int: 5, dex: 65, luk: 60, level: 56, weaponATK: 395 },
    modes: {},
    drops: [
        { itemName: 'Claw of Rat', rate: 46.56 },
        { itemName: 'Monster\'s Feed', rate: 10 },
        { itemName: 'Blue Jewel', rate: 0.8 },
        { itemName: 'Glass Bead', rate: 1.1 },
        { itemName: 'Lemon', rate: 2.5 },
        { itemName: 'Blue Herb', rate: 0.7 },
        { itemName: 'Oridecon', rate: 0.95 },
        { itemName: 'Cramp Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Killer Mantis (ID: 1294) ──── Level 56 | HP 13,183 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['killer_mantis'] = {
    id: 1294, name: 'Killer Mantis', aegisName: 'KILLER_MANTIS',
    level: 56, maxHealth: 13183, baseExp: 6509, jobExp: 2366, mvpExp: 0,
    attack: 764, attack2: 927, defense: 35, magicDefense: 20,
    str: 5, agi: 26, vit: 24, int: 5, dex: 75, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 175, attackDelay: 1528, attackMotion: 660, damageMotion: 432,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 5, agi: 26, vit: 24, int: 5, dex: 75, luk: 40, level: 56, weaponATK: 764 },
    modes: { detector: true },
    drops: [
        { itemName: 'Mantis Scythe', rate: 45.5 },
        { itemName: 'Solid Shell', rate: 25 },
        { itemName: 'Emerald', rate: 0.1 },
        { itemName: 'White Herb', rate: 0.15 },
        { itemName: 'Grape', rate: 0.25 },
        { itemName: 'Loki\'s Nail', rate: 0.01 },
        { itemName: 'Mirror Shield', rate: 0.01 },
        { itemName: 'Killer Mantis Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Giant Hornet (ID: 1303) ──── Level 56 | HP 13,105 | NORMAL | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['giant_honet'] = {
    id: 1303, name: 'Giant Hornet', aegisName: 'GIANT_HONET',
    level: 56, maxHealth: 13105, baseExp: 5785, jobExp: 2006, mvpExp: 0,
    attack: 650, attack2: 852, defense: 38, magicDefense: 43,
    str: 35, agi: 38, vit: 32, int: 10, dex: 71, luk: 64,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 155, attackDelay: 1292, attackMotion: 792, damageMotion: 340,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 35, agi: 38, vit: 32, int: 10, dex: 71, luk: 64, level: 56, weaponATK: 650 },
    modes: { detector: true },
    drops: [
        { itemName: 'Royal Jelly', rate: 5.5 },
        { itemName: 'Honey', rate: 12 },
        { itemName: 'Mastela Fruit', rate: 0.12 },
        { itemName: 'Yggdrasil Leaf', rate: 0.15 },
        { itemName: 'Staff', rate: 0.03 },
        { itemName: 'Pearl', rate: 0.2 },
        { itemName: 'Double Bound', rate: 0.15 },
        { itemName: 'Giant Hornet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Geographer (ID: 1368) ──── Level 56 | HP 8,071 | NORMAL | plant/earth3 | passive
RO_MONSTER_TEMPLATES['geographer'] = {
    id: 1368, name: 'Geographer', aegisName: 'GEOGRAPHER',
    level: 56, maxHealth: 8071, baseExp: 2715, jobExp: 2000, mvpExp: 0,
    attack: 467, attack2: 621, defense: 28, magicDefense: 26,
    str: 0, agi: 66, vit: 47, int: 60, dex: 68, luk: 44,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 174, walkSpeed: 1000, attackDelay: 1308, attackMotion: 1008, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 47, int: 60, dex: 68, luk: 44, level: 56, weaponATK: 467 },
    modes: {},
    drops: [
        { itemName: 'Maneater Blossom', rate: 62 },
        { itemName: 'Maneater Root', rate: 55 },
        { itemName: 'Sunflower', rate: 0.3 },
        { itemName: 'Fancy Flower', rate: 0.5 },
        { itemName: 'Level 5 Heal', rate: 1 },
        { itemName: 'Geographer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── The Paper (ID: 1375) ──── Level 56 | HP 18,557 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['the_paper'] = {
    id: 1375, name: 'The Paper', aegisName: 'THE_PAPER',
    level: 56, maxHealth: 18557, baseExp: 2849, jobExp: 1998, mvpExp: 0,
    attack: 845, attack2: 1124, defense: 25, magicDefense: 24,
    str: 0, agi: 66, vit: 52, int: 76, dex: 71, luk: 79,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 170, attackDelay: 1160, attackMotion: 960, damageMotion: 336,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 52, int: 76, dex: 71, luk: 79, level: 56, weaponATK: 845 },
    modes: {},
    drops: [
        { itemName: 'Slick Paper', rate: 49.47 },
        { itemName: 'Sharp Paper', rate: 32 },
        { itemName: 'Yellow Herb', rate: 18 },
        { itemName: 'Green Herb', rate: 20 },
        { itemName: 'Kamaitachi', rate: 0.05 },
        { itemName: 'The Paper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Demon Pungus (ID: 1378) ──── Level 56 | HP 7,259 | NORMAL | demon/poison3 | aggressive
RO_MONSTER_TEMPLATES['demon_pungus'] = {
    id: 1378, name: 'Demon Pungus', aegisName: 'DEMON_PUNGUS',
    level: 56, maxHealth: 7259, baseExp: 3148, jobExp: 1817, mvpExp: 0,
    attack: 360, attack2: 479, defense: 48, magicDefense: 31,
    str: 0, agi: 83, vit: 55, int: 59, dex: 63, luk: 34,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 170, attackDelay: 1260, attackMotion: 960, damageMotion: 672,
    size: 'small', race: 'demon', element: { type: 'poison', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 83, vit: 55, int: 59, dex: 63, luk: 34, level: 56, weaponATK: 360 },
    modes: { detector: true },
    drops: [
        { itemName: 'Bacillus', rate: 40.74 },
        { itemName: 'Mould Powder', rate: 45.59 },
        { itemName: 'Yellow Gemstone', rate: 38.8 },
        { itemName: 'Witched Starsand', rate: 50 },
        { itemName: 'Demon Pungus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Taoist Hermit (ID: 1412) ──── Level 56 | HP 10,392 | NORMAL | formless/neutral2 | aggressive
RO_MONSTER_TEMPLATES['evil_cloud_hermit'] = {
    id: 1412, name: 'Taoist Hermit', aegisName: 'EVIL_CLOUD_HERMIT',
    level: 56, maxHealth: 10392, baseExp: 3304, jobExp: 2198, mvpExp: 0,
    attack: 311, attack2: 333, defense: 25, magicDefense: 59,
    str: 0, agi: 20, vit: 18, int: 50, dex: 136, luk: 11,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 190, attackDelay: 480, attackMotion: 840, damageMotion: 432,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 20, vit: 18, int: 50, dex: 136, luk: 11, level: 56, weaponATK: 311 },
    modes: {},
    drops: [
        { itemName: 'Cloud Crumb', rate: 46.56 },
        { itemName: 'Cheese', rate: 56 },
        { itemName: 'Rice Cake', rate: 45 },
        { itemName: 'Bun', rate: 68 },
        { itemName: 'Guitar', rate: 0.02 },
        { itemName: 'Rough Elunium', rate: 1.5 },
        { itemName: 'Level 5 Lightening Bolt', rate: 1 },
        { itemName: 'Cloud Hermit Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Yao Jun (ID: 1512) ──── Level 56 | HP 9,981 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['hyegun'] = {
    id: 1512, name: 'Yao Jun', aegisName: 'HYEGUN',
    level: 56, maxHealth: 9981, baseExp: 2199, jobExp: 1022, mvpExp: 0,
    attack: 710, attack2: 1128, defense: 12, magicDefense: 10,
    str: 60, agi: 40, vit: 36, int: 10, dex: 73, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 180, attackDelay: 890, attackMotion: 1320, damageMotion: 720,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 60, agi: 40, vit: 36, int: 10, dex: 73, luk: 15, level: 56, weaponATK: 710 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 38.8 },
        { itemName: 'Amulet', rate: 1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Boots', rate: 0.01 },
        { itemName: 'Munak Doll', rate: 3 },
        { itemName: 'Yao Jun Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mineral (ID: 1614) ──── Level 56 | HP 7,950 | NORMAL | formless/neutral2 | aggressive
RO_MONSTER_TEMPLATES['mineral'] = {
    id: 1614, name: 'Mineral', aegisName: 'MINERAL',
    level: 56, maxHealth: 7950, baseExp: 3563, jobExp: 1768, mvpExp: 0,
    attack: 723, attack2: 812, defense: 29, magicDefense: 35,
    str: 60, agi: 52, vit: 35, int: 21, dex: 67, luk: 32,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 250, attackDelay: 648, attackMotion: 480, damageMotion: 360,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 60, agi: 52, vit: 35, int: 21, dex: 67, luk: 32, level: 56, weaponATK: 723 },
    modes: {},
    drops: [
        { itemName: 'Crystal Fragment', rate: 30 },
        { itemName: 'Topaz', rate: 5 },
        { itemName: 'Emperium', rate: 0.02 },
        { itemName: 'Oridecon', rate: 0.8 },
        { itemName: 'Emveretarcon', rate: 8 },
        { itemName: 'Yellow Gemstone', rate: 1 },
        { itemName: 'Gold', rate: 0.02 },
        { itemName: 'Mineral Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Beholder (ID: 1633) ──── Level 56 | HP 7,950 | NORMAL | formless/wind2 | aggressive
RO_MONSTER_TEMPLATES['beholder'] = {
    id: 1633, name: 'Beholder', aegisName: 'BEHOLDER',
    level: 56, maxHealth: 7950, baseExp: 4821, jobExp: 3822, mvpExp: 0,
    attack: 723, attack2: 812, defense: 17, magicDefense: 30,
    str: 60, agi: 62, vit: 25, int: 59, dex: 85, luk: 32,
    attackRange: 300, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 190, attackDelay: 336, attackMotion: 840, damageMotion: 360,
    size: 'small', race: 'formless', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 60, agi: 62, vit: 25, int: 59, dex: 85, luk: 32, level: 56, weaponATK: 723 },
    modes: {},
    drops: [
        { itemName: 'Prickly Fruit', rate: 30 },
        { itemName: 'Anodyne', rate: 1 },
        { itemName: 'Rough Wind', rate: 1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Old Blue Box', rate: 0.02 },
        { itemName: 'Beholder Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Breeze (ID: 1692) ──── Level 56 | HP 5,099 | NORMAL | formless/wind3 | aggressive
RO_MONSTER_TEMPLATES['breeze'] = {
    id: 1692, name: 'Breeze', aegisName: 'BREEZE',
    level: 56, maxHealth: 5099, baseExp: 2390, jobExp: 1340, mvpExp: 0,
    attack: 94, attack2: 215, defense: 7, magicDefense: 32,
    str: 0, agi: 96, vit: 6, int: 38, dex: 91, luk: 45,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 140, attackMotion: 384, damageMotion: 504,
    size: 'medium', race: 'formless', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 96, vit: 6, int: 38, dex: 91, luk: 45, level: 56, weaponATK: 94 },
    modes: {},
    drops: [
        { itemName: 'Raccoon Leaf', rate: 5 },
        { itemName: 'Four Leaf Clover', rate: 0.1 },
        { itemName: 'Romantic Leaf', rate: 0.1 },
        { itemName: 'Gust Bow', rate: 0.1 },
        { itemName: 'Dead Branch', rate: 0.1 },
        { itemName: 'Romantic Flower', rate: 0.1 },
        { itemName: 'Rough Wind', rate: 0.1 },
        { itemName: 'Breeze Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Plasma (ID: 1693) ──── Level 56 | HP 8,400 | NORMAL | formless/ghost4 | aggressive
RO_MONSTER_TEMPLATES['plasma_y'] = {
    id: 1693, name: 'Plasma', aegisName: 'PLASMA_Y',
    level: 56, maxHealth: 8400, baseExp: 2200, jobExp: 2100, mvpExp: 0,
    attack: 400, attack2: 900, defense: 0, magicDefense: 40,
    str: 0, agi: 30, vit: 10, int: 83, dex: 105, luk: 45,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 100, attackDelay: 608, attackMotion: 1440, damageMotion: 576,
    size: 'small', race: 'formless', element: { type: 'ghost', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23200,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 10, int: 83, dex: 105, luk: 45, level: 56, weaponATK: 400 },
    modes: {},
    drops: [
        { itemName: 'Scell', rate: 1 },
        { itemName: 'Gift Box', rate: 0.1 },
        { itemName: '1carat Diamond', rate: 0.02 },
        { itemName: 'Yellow Gemstone', rate: 1 },
        { itemName: 'Gold', rate: 0.01 },
        { itemName: 'Plasma Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Joker (ID: 1131) ──── Level 57 | HP 12,450 | NORMAL | demihuman/wind4 | aggressive
RO_MONSTER_TEMPLATES['joker'] = {
    id: 1131, name: 'Joker', aegisName: 'JOKER',
    level: 57, maxHealth: 12450, baseExp: 3706, jobExp: 2362, mvpExp: 0,
    attack: 621, attack2: 738, defense: 10, magicDefense: 35,
    str: 0, agi: 143, vit: 47, int: 75, dex: 98, luk: 175,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 100, attackDelay: 1364, attackMotion: 864, damageMotion: 432,
    size: 'large', race: 'demihuman', element: { type: 'wind', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23400,
    raceGroups: {},
    stats: { str: 0, agi: 143, vit: 47, int: 75, dex: 98, luk: 175, level: 57, weaponATK: 621 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 20 },
        { itemName: 'Old Card Album', rate: 0.02 },
        { itemName: 'Contract in Shadow', rate: 0.2 },
        { itemName: 'Yellow Herb', rate: 10 },
        { itemName: 'Katar of Piercing Wind', rate: 0.01 },
        { itemName: 'Oridecon', rate: 1 },
        { itemName: 'Level 5 Soul Strike', rate: 1 },
        { itemName: 'Joker Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Penomena (ID: 1216) ──── Level 57 | HP 7,256 | NORMAL | fish/poison1 | aggressive
RO_MONSTER_TEMPLATES['penomena'] = {
    id: 1216, name: 'Penomena', aegisName: 'PENOMENA',
    level: 57, maxHealth: 7256, baseExp: 2870, jobExp: 2200, mvpExp: 0,
    attack: 415, attack2: 565, defense: 5, magicDefense: 50,
    str: 0, agi: 5, vit: 35, int: 15, dex: 136, luk: 30,
    attackRange: 350, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 400, attackDelay: 832, attackMotion: 500, damageMotion: 600,
    size: 'medium', race: 'fish', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23400,
    raceGroups: {},
    stats: { str: 0, agi: 5, vit: 35, int: 15, dex: 136, luk: 30, level: 57, weaponATK: 415 },
    modes: {},
    drops: [
        { itemName: 'Coral Reef', rate: 48.5 },
        { itemName: 'Tentacle', rate: 80 },
        { itemName: 'Sticky Mucus', rate: 70 },
        { itemName: 'Panacea', rate: 2 },
        { itemName: 'Amethyst', rate: 0.15 },
        { itemName: 'Katar of Raging Blaze', rate: 0.01 },
        { itemName: 'Red Gemstone', rate: 5.5 },
        { itemName: 'Penomena Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Muscipular (ID: 1780) ──── Level 57 | HP 4,332 | NORMAL | plant/earth1 | passive
RO_MONSTER_TEMPLATES['muscipular'] = {
    id: 1780, name: 'Muscipular', aegisName: 'MUSCIPULAR',
    level: 57, maxHealth: 4332, baseExp: 1706, jobExp: 1706, mvpExp: 0,
    attack: 521, attack2: 726, defense: 12, magicDefense: 12,
    str: 0, agi: 53, vit: 39, int: 25, dex: 92, luk: 51,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 187, walkSpeed: 1000, attackDelay: 672, attackMotion: 648, damageMotion: 360,
    size: 'medium', race: 'plant', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 23400,
    raceGroups: {},
    stats: { str: 0, agi: 53, vit: 39, int: 25, dex: 92, luk: 51, level: 57, weaponATK: 521 },
    modes: {},
    drops: [
        { itemName: 'Sticky Poison', rate: 30 },
        { itemName: 'Maneater Blossom', rate: 30 },
        { itemName: 'Singing Flower', rate: 0.02 },
        { itemName: 'Maneater Root', rate: 20 },
        { itemName: 'Stem', rate: 10 },
        { itemName: 'Deadly Noxious Herb', rate: 0.03 },
        { itemName: 'Muscipular Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Evil Druid (ID: 1117) ──── Level 58 | HP 16,506 | NORMAL | undead/undead4 | aggressive
RO_MONSTER_TEMPLATES['evil_druid'] = {
    id: 1117, name: 'Evil Druid', aegisName: 'EVIL_DRUID',
    level: 58, maxHealth: 16506, baseExp: 2890, jobExp: 1827, mvpExp: 0,
    attack: 420, attack2: 670, defense: 5, magicDefense: 60,
    str: 0, agi: 29, vit: 58, int: 80, dex: 68, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 154, walkSpeed: 300, attackDelay: 2276, attackMotion: 576, damageMotion: 336,
    size: 'large', race: 'undead', element: { type: 'undead', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 0, agi: 29, vit: 58, int: 80, dex: 68, luk: 30, level: 58, weaponATK: 420 },
    modes: {},
    drops: [
        { itemName: 'Biretta', rate: 0.1 },
        { itemName: 'Evil Bone Wand', rate: 0.01 },
        { itemName: 'Ragamuffin Manteau', rate: 0.02 },
        { itemName: 'Bible', rate: 0.1 },
        { itemName: 'Yggdrasil Leaf', rate: 2 },
        { itemName: 'Level 7 Cookbook', rate: 0.04 },
        { itemName: 'White Herb', rate: 20 },
        { itemName: 'Evil Druid Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Alarm (ID: 1193) ──── Level 58 | HP 10,647 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['alarm'] = {
    id: 1193, name: 'Alarm', aegisName: 'ALARM',
    level: 58, maxHealth: 10647, baseExp: 3987, jobExp: 2300, mvpExp: 0,
    attack: 480, attack2: 600, defense: 15, magicDefense: 15,
    str: 0, agi: 62, vit: 72, int: 10, dex: 85, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 300, attackDelay: 1020, attackMotion: 500, damageMotion: 768,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 0, agi: 62, vit: 72, int: 10, dex: 85, luk: 45, level: 58, weaponATK: 480 },
    modes: {},
    drops: [
        { itemName: 'Needle of Alarm', rate: 53.35 },
        { itemName: 'Clip', rate: 0.01 },
        { itemName: 'Skull', rate: 15 },
        { itemName: 'Magnifier', rate: 13 },
        { itemName: 'Oridecon', rate: 1.05 },
        { itemName: 'Key of Clock Tower', rate: 0.2 },
        { itemName: 'Zargon', rate: 15 },
        { itemName: 'Alarm Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Leib Olmai (ID: 1306) ──── Level 58 | HP 24,233 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['leib_olmai'] = {
    id: 1306, name: 'Leib Olmai', aegisName: 'LEIB_OLMAI',
    level: 58, maxHealth: 24233, baseExp: 6011, jobExp: 2171, mvpExp: 0,
    attack: 740, attack2: 1390, defense: 27, magicDefense: 31,
    str: 5, agi: 35, vit: 95, int: 5, dex: 64, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 175, attackDelay: 1260, attackMotion: 230, damageMotion: 192,
    size: 'large', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 5, agi: 35, vit: 95, int: 5, dex: 64, luk: 85, level: 58, weaponATK: 740 },
    modes: {},
    drops: [
        { itemName: 'Bear\'s Foot', rate: 45.5 },
        { itemName: 'Poo Poo Hat', rate: 0.08 },
        { itemName: 'Puppet', rate: 1.2 },
        { itemName: 'Honey', rate: 5 },
        { itemName: 'Pocket Watch', rate: 0.05 },
        { itemName: 'Gold', rate: 0.05 },
        { itemName: 'Cyfar', rate: 8 },
        { itemName: 'Leib Olmai Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Spring Rabbit (ID: 1322) ──── Level 58 | HP 9,045 | NORMAL | brute/earth2 | passive
RO_MONSTER_TEMPLATES['spring_rabbit'] = {
    id: 1322, name: 'Spring Rabbit', aegisName: 'SPRING_RABBIT',
    level: 58, maxHealth: 9045, baseExp: 3982, jobExp: 1766, mvpExp: 0,
    attack: 585, attack2: 813, defense: 29, magicDefense: 21,
    str: 45, agi: 61, vit: 5, int: 15, dex: 77, luk: 90,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 178, walkSpeed: 160, attackDelay: 1120, attackMotion: 552, damageMotion: 511,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 45, agi: 61, vit: 5, int: 15, dex: 77, luk: 90, level: 58, weaponATK: 585 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 35 },
        { itemName: 'Cyfar', rate: 25 },
        { itemName: 'Feather', rate: 25 },
        { itemName: 'Green Herb', rate: 45 },
        { itemName: 'Yellow Herb', rate: 8 },
        { itemName: 'Blue Herb', rate: 2 },
        { itemName: 'White Herb', rate: 8 },
        { itemName: 'Spring Rabbit Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Grand Peco (ID: 1369) ──── Level 58 | HP 8,054 | NORMAL | brute/fire2 | reactive
RO_MONSTER_TEMPLATES['grand_peco'] = {
    id: 1369, name: 'Grand Peco', aegisName: 'GRAND_PECO',
    level: 58, maxHealth: 8054, baseExp: 2387, jobExp: 1361, mvpExp: 0,
    attack: 444, attack2: 565, defense: 37, magicDefense: 30,
    str: 0, agi: 66, vit: 66, int: 50, dex: 71, luk: 51,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 165, attackDelay: 1460, attackMotion: 960, damageMotion: 432,
    size: 'large', race: 'brute', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 66, int: 50, dex: 71, luk: 51, level: 58, weaponATK: 444 },
    modes: {},
    drops: [
        { itemName: 'Peco Peco Feather', rate: 48.5 },
        { itemName: 'Mastela Fruit', rate: 3 },
        { itemName: 'Wind of Verdure', rate: 10 },
        { itemName: 'Gold', rate: 0.01 },
        { itemName: 'Orange', rate: 5 },
        { itemName: 'Grand Peco Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gibbet (ID: 1503) ──── Level 58 | HP 6,841 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['gibbet'] = {
    id: 1503, name: 'Gibbet', aegisName: 'GIBBET',
    level: 58, maxHealth: 6841, baseExp: 4011, jobExp: 1824, mvpExp: 0,
    attack: 418, attack2: 656, defense: 28, magicDefense: 31,
    str: 0, agi: 42, vit: 42, int: 27, dex: 46, luk: 28,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 180, attackDelay: 917, attackMotion: 1584, damageMotion: 576,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 0, agi: 42, vit: 42, int: 27, dex: 46, luk: 28, level: 58, weaponATK: 418 },
    modes: { detector: true },
    drops: [
        { itemName: 'Hung Doll', rate: 18 },
        { itemName: 'Decomposed Rope', rate: 53.35 },
        { itemName: 'Wooden Gnarl', rate: 40.74 },
        { itemName: 'Ruby', rate: 3 },
        { itemName: 'Red Gemstone', rate: 1 },
        { itemName: 'Dead Branch', rate: 0.1 },
        { itemName: 'Gibbet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Egnigem Cenia (ID: 1652) ──── Level 58 | HP 11,200 | NORMAL | demihuman/fire2 | aggressive
RO_MONSTER_TEMPLATES['ygnizem'] = {
    id: 1652, name: 'Egnigem Cenia', aegisName: 'YGNIZEM',
    level: 58, maxHealth: 11200, baseExp: 4870, jobExp: 98, mvpExp: 0,
    attack: 823, attack2: 1212, defense: 35, magicDefense: 8,
    str: 60, agi: 35, vit: 52, int: 18, dex: 79, luk: 20,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 145, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23600,
    raceGroups: {},
    stats: { str: 60, agi: 35, vit: 52, int: 18, dex: 79, luk: 20, level: 58, weaponATK: 823 },
    modes: {},
    drops: [
        { itemName: 'Research Chart', rate: 10 },
        { itemName: 'Katzbalger', rate: 0.01 },
        { itemName: 'Two-Handed Sword', rate: 0.2 },
        { itemName: 'Saber', rate: 0.2 },
        { itemName: 'Padded Armor', rate: 0.1 },
        { itemName: 'Slayer', rate: 0.8 },
        { itemName: 'Full Plate', rate: 0.01 },
        { itemName: 'Egnigem Cenia Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Arclouze (ID: 1194) ──── Level 59 | HP 6,075 | NORMAL | insect/earth2 | aggressive
RO_MONSTER_TEMPLATES['arclouse'] = {
    id: 1194, name: 'Arclouze', aegisName: 'ARCLOUSE',
    level: 59, maxHealth: 6075, baseExp: 860, jobExp: 1000, mvpExp: 0,
    attack: 570, attack2: 640, defense: 10, magicDefense: 15,
    str: 0, agi: 75, vit: 5, int: 5, dex: 75, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 100, attackDelay: 960, attackMotion: 500, damageMotion: 480,
    size: 'medium', race: 'insect', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 0, agi: 75, vit: 5, int: 5, dex: 75, luk: 50, level: 59, weaponATK: 570 },
    modes: { detector: true },
    drops: [
        { itemName: 'Round Shell', rate: 35 },
        { itemName: 'Sticky Mucus', rate: 30 },
        { itemName: 'Solid Shell', rate: 8 },
        { itemName: 'Zargon', rate: 4.5 },
        { itemName: 'Red Gemstone', rate: 3 },
        { itemName: 'Great Nature', rate: 0.2 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Arclouze Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Rideword (ID: 1195) ──── Level 59 | HP 11,638 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['rideword'] = {
    id: 1195, name: 'Rideword', aegisName: 'RIDEWORD',
    level: 59, maxHealth: 11638, baseExp: 2007, jobExp: 3106, mvpExp: 0,
    attack: 584, attack2: 804, defense: 5, magicDefense: 35,
    str: 48, agi: 75, vit: 10, int: 20, dex: 120, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 150, attackDelay: 864, attackMotion: 500, damageMotion: 192,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 48, agi: 75, vit: 10, int: 20, dex: 120, luk: 45, level: 59, weaponATK: 584 },
    modes: {},
    drops: [
        { itemName: 'Worn Out Page', rate: 48.5 },
        { itemName: 'Book of Billows', rate: 0.04 },
        { itemName: 'Book of Mother Earth', rate: 0.04 },
        { itemName: 'Book of Blazing Sun', rate: 0.02 },
        { itemName: 'Book of Gust of Wind', rate: 0.02 },
        { itemName: 'Bookclip in Memory', rate: 3 },
        { itemName: 'Old Magicbook', rate: 0.2 },
        { itemName: 'Rideword Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dark Frame (ID: 1260) ──── Level 59 | HP 7,500 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['dark_frame'] = {
    id: 1260, name: 'Dark Frame', aegisName: 'DARK_FRAME',
    level: 59, maxHealth: 7500, baseExp: 3652, jobExp: 3271, mvpExp: 0,
    attack: 960, attack2: 1210, defense: 10, magicDefense: 45,
    str: 0, agi: 72, vit: 42, int: 45, dex: 85, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 200, attackDelay: 920, attackMotion: 720, damageMotion: 200,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 0, agi: 72, vit: 42, int: 45, dex: 85, luk: 25, level: 59, weaponATK: 960 },
    modes: { detector: true },
    drops: [
        { itemName: 'Brigan', rate: 46.56 },
        { itemName: 'Red Frame', rate: 10 },
        { itemName: 'Manteau', rate: 0.3 },
        { itemName: 'Star Crumb', rate: 0.8 },
        { itemName: 'Crystal Mirror', rate: 0.03 },
        { itemName: 'Dark Frame Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Panzer Goblin (ID: 1308) ──── Level 59 | HP 14,130 | NORMAL | demihuman/wind2 | aggressive
RO_MONSTER_TEMPLATES['panzer_goblin'] = {
    id: 1308, name: 'Panzer Goblin', aegisName: 'PANZER_GOBLIN',
    level: 59, maxHealth: 14130, baseExp: 7212, jobExp: 2697, mvpExp: 0,
    attack: 683, attack2: 878, defense: 41, magicDefense: 28,
    str: 60, agi: 60, vit: 40, int: 20, dex: 81, luk: 160,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 200, attackDelay: 960, attackMotion: 1008, damageMotion: 840,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 60, agi: 60, vit: 40, int: 20, dex: 81, luk: 160, level: 59, weaponATK: 683 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 44.13 },
        { itemName: 'Brigan', rate: 35 },
        { itemName: 'Steel', rate: 1.8 },
        { itemName: 'Iron', rate: 3.6 },
        { itemName: 'Coal', rate: 5.8 },
        { itemName: 'Butcher', rate: 0.05 },
        { itemName: 'Flame Heart', rate: 1.6 },
        { itemName: 'Panzer Goblin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sea Otter (ID: 1323) ──── Level 59 | HP 9,999 | NORMAL | brute/water3 | aggressive
RO_MONSTER_TEMPLATES['see_otter'] = {
    id: 1323, name: 'Sea Otter', aegisName: 'SEE_OTTER',
    level: 59, maxHealth: 9999, baseExp: 3048, jobExp: 1642, mvpExp: 0,
    attack: 650, attack2: 813, defense: 33, magicDefense: 35,
    str: 5, agi: 36, vit: 40, int: 25, dex: 82, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 190, attackDelay: 1132, attackMotion: 583, damageMotion: 532,
    size: 'medium', race: 'brute', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 5, agi: 36, vit: 40, int: 25, dex: 82, luk: 65, level: 59, weaponATK: 650 },
    modes: {},
    drops: [
        { itemName: 'Pearl', rate: 1.5 },
        { itemName: 'Clam Shell', rate: 55 },
        { itemName: 'Sea-Otter Fur', rate: 43.65 },
        { itemName: 'Red Jewel', rate: 0.5 },
        { itemName: 'Blue Jewel', rate: 0.5 },
        { itemName: 'Glass Bead', rate: 6.5 },
        { itemName: 'Cyfar', rate: 12 },
        { itemName: 'Sea-Otter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Green Maiden (ID: 1631) ──── Level 59 | HP 23,900 | NORMAL | demihuman/wind2 | aggressive
RO_MONSTER_TEMPLATES['chung_e_'] = {
    id: 1631, name: 'Green Maiden', aegisName: 'CHUNG_E_',
    level: 59, maxHealth: 23900, baseExp: 4256, jobExp: 920, mvpExp: 0,
    attack: 460, attack2: 1050, defense: 8, magicDefense: 15,
    str: 38, agi: 65, vit: 43, int: 30, dex: 90, luk: 15,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 150, attackDelay: 1728, attackMotion: 816, damageMotion: 1188,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 38, agi: 65, vit: 43, int: 30, dex: 90, luk: 15, level: 59, weaponATK: 460 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 42 },
        { itemName: 'Puppet', rate: 1 },
        { itemName: 'Studded Knuckles', rate: 0.1 },
        { itemName: 'Honey', rate: 5 },
        { itemName: 'Tantan Noodle', rate: 0.1 },
        { itemName: 'Bao Bao', rate: 0.5 },
        { itemName: 'Green Maiden Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Errende Ebecee (ID: 1655) ──── Level 59 | HP 6,980 | NORMAL | demihuman/holy2 | aggressive
RO_MONSTER_TEMPLATES['erend'] = {
    id: 1655, name: 'Errende Ebecee', aegisName: 'EREND',
    level: 59, maxHealth: 6980, baseExp: 4501, jobExp: 67, mvpExp: 0,
    attack: 896, attack2: 1159, defense: 14, magicDefense: 30,
    str: 0, agi: 31, vit: 41, int: 93, dex: 67, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 130, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'holy', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 0, agi: 31, vit: 41, int: 93, dex: 67, luk: 30, level: 59, weaponATK: 896 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 5 },
        { itemName: 'Biretta', rate: 0.05 },
        { itemName: 'Morning Star', rate: 0.5 },
        { itemName: 'Sword Mace', rate: 0.2 },
        { itemName: 'Saint\'s Robe', rate: 0.05 },
        { itemName: 'Scapulare', rate: 0.1 },
        { itemName: 'Spike', rate: 0.01 },
        { itemName: 'Errende Ebecee Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Vanberk (ID: 1771) ──── Level 59 | HP 9,988 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['vanberk'] = {
    id: 1771, name: 'Vanberk', aegisName: 'VANBERK',
    level: 59, maxHealth: 9988, baseExp: 4203, jobExp: 901, mvpExp: 0,
    attack: 230, attack2: 660, defense: 24, magicDefense: 6,
    str: 69, agi: 66, vit: 39, int: 29, dex: 51, luk: 41,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 250, attackDelay: 768, attackMotion: 360, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 23800,
    raceGroups: {},
    stats: { str: 69, agi: 66, vit: 39, int: 29, dex: 51, luk: 41, level: 59, weaponATK: 230 },
    modes: {},
    drops: [
        { itemName: 'White Mask', rate: 25 },
        { itemName: 'Royal Jelly', rate: 0.1 },
        { itemName: 'Bloody Rune', rate: 10 },
        { itemName: 'Beret', rate: 0.1 },
        { itemName: 'Scalpel', rate: 0.05 },
        { itemName: 'Bloody Rune', rate: 1 },
        { itemName: 'Vanberk Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kaho (ID: 1072) ──── Level 60 | HP 8,409 | NORMAL | demon/fire4 | aggressive
RO_MONSTER_TEMPLATES['kaho'] = {
    id: 1072, name: 'Kaho', aegisName: 'KAHO',
    level: 60, maxHealth: 8409, baseExp: 3990, jobExp: 450, mvpExp: 0,
    attack: 110, attack2: 760, defense: 5, magicDefense: 50,
    str: 0, agi: 55, vit: 43, int: 88, dex: 80, luk: 46,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 150, attackDelay: 1700, attackMotion: 1000, damageMotion: 500,
    size: 'medium', race: 'demon', element: { type: 'fire', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 0, agi: 55, vit: 43, int: 88, dex: 80, luk: 46, level: 60, weaponATK: 110 },
    modes: { detector: true },
    drops: [
        { itemName: 'Flame Heart', rate: 0.3 },
        { itemName: 'Coal', rate: 1.5 },
        { itemName: 'Burning Heart', rate: 30 },
        { itemName: 'Level 3 Fire Bolt', rate: 1 },
        { itemName: 'Rough Elunium', rate: 10 },
        { itemName: 'Red Gemstone', rate: 3 },
        { itemName: 'Alcohol', rate: 0.05 },
        { itemName: 'Kaho Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Clock (ID: 1269) ──── Level 60 | HP 11,050 | NORMAL | formless/earth2 | aggressive
RO_MONSTER_TEMPLATES['clock'] = {
    id: 1269, name: 'Clock', aegisName: 'CLOCK',
    level: 60, maxHealth: 11050, baseExp: 3410, jobExp: 2904, mvpExp: 0,
    attack: 720, attack2: 909, defense: 15, magicDefense: 10,
    str: 0, agi: 70, vit: 50, int: 25, dex: 90, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 200, attackDelay: 1092, attackMotion: 792, damageMotion: 480,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 0, agi: 70, vit: 50, int: 25, dex: 90, luk: 50, level: 60, weaponATK: 720 },
    modes: {},
    drops: [
        { itemName: 'Needle of Alarm', rate: 53.35 },
        { itemName: 'Wooden Block', rate: 8 },
        { itemName: 'White Herb', rate: 19 },
        { itemName: 'Lemon', rate: 3.2 },
        { itemName: 'Key of Clock Tower', rate: 0.3 },
        { itemName: 'Key of Underground', rate: 0.3 },
        { itemName: 'Elunium', rate: 1.63 },
        { itemName: 'Clock Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Stalactic Golem (ID: 1278) ──── Level 60 | HP 18,700 | NORMAL | formless/neutral1 | passive
RO_MONSTER_TEMPLATES['stalactic_golem'] = {
    id: 1278, name: 'Stalactic Golem', aegisName: 'STALACTIC_GOLEM',
    level: 60, maxHealth: 18700, baseExp: 5808, jobExp: 2695, mvpExp: 0,
    attack: 950, attack2: 1260, defense: 50, magicDefense: 5,
    str: 73, agi: 45, vit: 85, int: 5, dex: 90, luk: 25,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 24000,
    raceGroups: { Golem: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 73, agi: 45, vit: 85, int: 5, dex: 90, luk: 25, level: 60, weaponATK: 950 },
    modes: {},
    drops: [
        { itemName: 'Mud Lump', rate: 20 },
        { itemName: 'Brigan', rate: 48.5 },
        { itemName: 'Star Crumb', rate: 2.5 },
        { itemName: 'Great Nature', rate: 0.3 },
        { itemName: 'Rough Elunium', rate: 2.5 },
        { itemName: 'Elunium', rate: 1.63 },
        { itemName: 'Stalactic Golem Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gig (ID: 1387) ──── Level 60 | HP 8,409 | NORMAL | brute/fire2 | aggressive
RO_MONSTER_TEMPLATES['gig'] = {
    id: 1387, name: 'Gig', aegisName: 'GIG',
    level: 60, maxHealth: 8409, baseExp: 3934, jobExp: 2039, mvpExp: 0,
    attack: 360, attack2: 479, defense: 60, magicDefense: 28,
    str: 0, agi: 61, vit: 80, int: 53, dex: 59, luk: 46,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 170, attackDelay: 1264, attackMotion: 864, damageMotion: 576,
    size: 'small', race: 'brute', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 0, agi: 61, vit: 80, int: 53, dex: 59, luk: 46, level: 60, weaponATK: 360 },
    modes: {},
    drops: [
        { itemName: 'Scropion\'s Nipper', rate: 43.65 },
        { itemName: 'Scorpion\'s Tail', rate: 55 },
        { itemName: 'Red Gemstone', rate: 1.5 },
        { itemName: 'Panacea', rate: 25 },
        { itemName: 'Flame Heart', rate: 8.5 },
        { itemName: 'Gig Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Arc Angeling (ID: 1388) ──── Level 60 | HP 79,523 | BOSS | angel/holy3 | aggressive
RO_MONSTER_TEMPLATES['archangeling'] = {
    id: 1388, name: 'Arc Angeling', aegisName: 'ARCHANGELING',
    level: 60, maxHealth: 79523, baseExp: 4152, jobExp: 2173, mvpExp: 0,
    attack: 669, attack2: 890, defense: 54, magicDefense: 58,
    str: 0, agi: 65, vit: 80, int: 74, dex: 65, luk: 105,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 180, attackDelay: 1072, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'angel', element: { type: 'holy', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 65, vit: 80, int: 74, dex: 65, luk: 105, level: 60, weaponATK: 669 },
    modes: {},
    drops: [
        { itemName: 'Evil Wing', rate: 0.05 },
        { itemName: 'Yggdrasil Leaf', rate: 18 },
        { itemName: 'Yggdrasil Seed', rate: 1.5 },
        { itemName: 'Agate', rate: 15 },
        { itemName: 'Angel Wing', rate: 0.05 },
        { itemName: 'Full Plate', rate: 0.03 },
        { itemName: 'Turquoise', rate: 15 },
        { itemName: 'Arc Angeling Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kavach Icarus (ID: 1656) ──── Level 60 | HP 7,899 | NORMAL | demihuman/wind2 | aggressive
RO_MONSTER_TEMPLATES['kavac'] = {
    id: 1656, name: 'Kavach Icarus', aegisName: 'KAVAC',
    level: 60, maxHealth: 7899, baseExp: 4090, jobExp: 86, mvpExp: 0,
    attack: 684, attack2: 904, defense: 12, magicDefense: 5,
    str: 48, agi: 100, vit: 10, int: 15, dex: 118, luk: 40,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 48, agi: 100, vit: 10, int: 15, dex: 118, luk: 40, level: 60, weaponATK: 684 },
    modes: {},
    drops: [
        { itemName: 'Research Chart', rate: 20 },
        { itemName: 'Gakkung Bow', rate: 0.01 },
        { itemName: 'Steel Arrow Quiver', rate: 1 },
        { itemName: 'Great Bow', rate: 0.1 },
        { itemName: 'Mantle', rate: 0.05 },
        { itemName: 'Sandals', rate: 0.3 },
        { itemName: 'Shoes', rate: 0.02 },
        { itemName: 'Kavach Icarus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ancient Mimic (ID: 1699) ──── Level 60 | HP 8,080 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['ancient_mimic'] = {
    id: 1699, name: 'Ancient Mimic', aegisName: 'ANCIENT_MIMIC',
    level: 60, maxHealth: 8080, baseExp: 2950, jobExp: 2650, mvpExp: 0,
    attack: 530, attack2: 1697, defense: 20, magicDefense: 40,
    str: 50, agi: 100, vit: 30, int: 40, dex: 150, luk: 110,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 168, attackMotion: 480, damageMotion: 360,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 50, agi: 100, vit: 30, int: 40, dex: 150, luk: 110, level: 60, weaponATK: 530 },
    modes: {},
    drops: [
        { itemName: 'Old Blue Box', rate: 0.3 },
        { itemName: 'Old Purple Box', rate: 0.01 },
        { itemName: 'Gift Box', rate: 0.5 },
        { itemName: 'Shoes', rate: 0.05 },
        { itemName: 'Manteau', rate: 0.01 },
        { itemName: 'Fricco\'s Shoes', rate: 0.1 },
        { itemName: 'Gold Ring', rate: 1 },
        { itemName: 'Ancient Mimic Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Snowier (ID: 1775) ──── Level 60 | HP 19,230 | NORMAL | formless/water2 | aggressive
RO_MONSTER_TEMPLATES['snowier'] = {
    id: 1775, name: 'Snowier', aegisName: 'SNOWIER',
    level: 60, maxHealth: 19230, baseExp: 5882, jobExp: 2699, mvpExp: 0,
    attack: 770, attack2: 1347, defense: 22, magicDefense: 12,
    str: 73, agi: 46, vit: 72, int: 15, dex: 52, luk: 25,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 220, attackDelay: 936, attackMotion: 1020, damageMotion: 420,
    size: 'large', race: 'formless', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 73, agi: 46, vit: 72, int: 15, dex: 52, luk: 25, level: 60, weaponATK: 770 },
    modes: {},
    drops: [
        { itemName: 'Glacial Heart', rate: 30 },
        { itemName: 'Ice Piece', rate: 10 },
        { itemName: 'Rough Elunium', rate: 1 },
        { itemName: 'Blue Herb', rate: 0.5 },
        { itemName: 'White Herb', rate: 5 },
        { itemName: 'Icicle Fist', rate: 0.03 },
        { itemName: 'Crystal Blue', rate: 1 },
        { itemName: 'Snowier Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ice Titan (ID: 1777) ──── Level 60 | HP 38,200 | NORMAL | formless/water3 | aggressive
RO_MONSTER_TEMPLATES['ice_titan'] = {
    id: 1777, name: 'Ice Titan', aegisName: 'ICE_TITAN',
    level: 60, maxHealth: 38200, baseExp: 13872, jobExp: 7928, mvpExp: 0,
    attack: 1090, attack2: 1570, defense: 71, magicDefense: 15,
    str: 99, agi: 34, vit: 88, int: 10, dex: 79, luk: 29,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 250, attackDelay: 861, attackMotion: 660, damageMotion: 144,
    size: 'large', race: 'formless', element: { type: 'water', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 24000,
    raceGroups: {},
    stats: { str: 99, agi: 34, vit: 88, int: 10, dex: 79, luk: 29, level: 60, weaponATK: 1090 },
    modes: {},
    drops: [
        { itemName: 'Glacial Heart', rate: 50 },
        { itemName: 'Ice Piece', rate: 30 },
        { itemName: 'Frozen Rose', rate: 1 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Elunium', rate: 0.3 },
        { itemName: 'Mystic Frozen', rate: 1 },
        { itemName: 'Ice Titan Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pasana (ID: 1154) ──── Level 61 | HP 8,289 | NORMAL | demihuman/fire2 | aggressive
RO_MONSTER_TEMPLATES['pasana'] = {
    id: 1154, name: 'Pasana', aegisName: 'PASANA',
    level: 61, maxHealth: 8289, baseExp: 4087, jobExp: 2135, mvpExp: 0,
    attack: 513, attack2: 682, defense: 29, magicDefense: 35,
    str: 0, agi: 73, vit: 50, int: 61, dex: 59, luk: 43,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 165, attackDelay: 976, attackMotion: 576, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 0, agi: 73, vit: 50, int: 61, dex: 59, luk: 43, level: 61, weaponATK: 513 },
    modes: {},
    drops: [
        { itemName: 'Broken Sword', rate: 43.65 },
        { itemName: 'Honey Pot', rate: 25 },
        { itemName: 'Rough Elunium', rate: 0.2 },
        { itemName: 'Falchion', rate: 5 },
        { itemName: 'Stiletto', rate: 1.5 },
        { itemName: 'Undershirt', rate: 1 },
        { itemName: 'Pasana Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Anolian (ID: 1206) ──── Level 61 | HP 18,960 | NORMAL | fish/water2 | aggressive
RO_MONSTER_TEMPLATES['anolian'] = {
    id: 1206, name: 'Anolian', aegisName: 'ANOLIAN',
    level: 61, maxHealth: 18960, baseExp: 5900, jobExp: 3700, mvpExp: 0,
    attack: 640, attack2: 980, defense: 15, magicDefense: 15,
    str: 0, agi: 43, vit: 58, int: 25, dex: 80, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 190, attackDelay: 900, attackMotion: 500, damageMotion: 864,
    size: 'medium', race: 'fish', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 0, agi: 43, vit: 58, int: 25, dex: 80, luk: 65, level: 61, weaponATK: 640 },
    modes: {},
    drops: [
        { itemName: 'Anolian Skin', rate: 48.5 },
        { itemName: 'Crystal Arrow', rate: 20 },
        { itemName: 'Royal Jelly', rate: 0.05 },
        { itemName: 'Red Muffler', rate: 0.1 },
        { itemName: 'Solid Shell', rate: 53.35 },
        { itemName: 'Brooch', rate: 0.01 },
        { itemName: 'Oridecon', rate: 1.34 },
        { itemName: 'Anolian Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sting (ID: 1207) ──── Level 61 | HP 9,500 | NORMAL | formless/earth3 | aggressive
RO_MONSTER_TEMPLATES['sting'] = {
    id: 1207, name: 'Sting', aegisName: 'STING',
    level: 61, maxHealth: 9500, baseExp: 4081, jobExp: 2970, mvpExp: 0,
    attack: 850, attack2: 1032, defense: 5, magicDefense: 30,
    str: 57, agi: 45, vit: 55, int: 5, dex: 120, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 189, walkSpeed: 300, attackDelay: 528, attackMotion: 500, damageMotion: 240,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 57, agi: 45, vit: 55, int: 5, dex: 120, luk: 85, level: 61, weaponATK: 850 },
    modes: {},
    drops: [
        { itemName: 'Mud Lump', rate: 48.5 },
        { itemName: 'Stone Arrow', rate: 15 },
        { itemName: 'Glove', rate: 0.01 },
        { itemName: 'Coal', rate: 1.3 },
        { itemName: 'Great Nature', rate: 0.25 },
        { itemName: 'Silk Ribbon', rate: 0.1 },
        { itemName: 'Amethyst', rate: 0.03 },
        { itemName: 'Sting Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Am Mut (ID: 1301) ──── Level 61 | HP 12,099 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['am_mut'] = {
    id: 1301, name: 'Am Mut', aegisName: 'AM_MUT',
    level: 61, maxHealth: 12099, baseExp: 7709, jobExp: 2690, mvpExp: 0,
    attack: 1040, attack2: 1121, defense: 50, magicDefense: 10,
    str: 50, agi: 65, vit: 40, int: 35, dex: 83, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 200, attackDelay: 1156, attackMotion: 456, damageMotion: 384,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 50, agi: 65, vit: 40, int: 35, dex: 83, luk: 45, level: 61, weaponATK: 1040 },
    modes: { detector: true },
    drops: [
        { itemName: 'Dokebi Horn', rate: 45.5 },
        { itemName: 'Rough Elunium', rate: 2.5 },
        { itemName: 'Sword Mace', rate: 0.03 },
        { itemName: 'Gold', rate: 0.05 },
        { itemName: 'Halo', rate: 0.01 },
        { itemName: 'Old Card Album', rate: 0.01 },
        { itemName: 'Glass Bead', rate: 2.5 },
        { itemName: 'Am Mut Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mobster (ID: 1313) ──── Level 61 | HP 7,991 | NORMAL | demihuman/neutral1 | aggressive
RO_MONSTER_TEMPLATES['mobster'] = {
    id: 1313, name: 'Mobster', aegisName: 'MOBSTER',
    level: 61, maxHealth: 7991, baseExp: 4424, jobExp: 1688, mvpExp: 0,
    attack: 910, attack2: 1128, defense: 41, magicDefense: 37,
    str: 76, agi: 46, vit: 20, int: 35, dex: 76, luk: 55,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 250, attackDelay: 1100, attackMotion: 560, damageMotion: 580,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 76, agi: 46, vit: 20, int: 35, dex: 76, luk: 55, level: 61, weaponATK: 910 },
    modes: {},
    drops: [
        { itemName: 'Poison Knife', rate: 0.03 },
        { itemName: 'Blue Jewel', rate: 45.59 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Red Gemstone', rate: 6 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Panacea', rate: 4.5 },
        { itemName: 'Blue Potion', rate: 0.6 },
        { itemName: 'Mobster Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dragon Tail (ID: 1321) ──── Level 61 | HP 8,368 | NORMAL | insect/wind2 | aggressive
RO_MONSTER_TEMPLATES['dragon_tail'] = {
    id: 1321, name: 'Dragon Tail', aegisName: 'DRAGON_TAIL',
    level: 61, maxHealth: 8368, baseExp: 3587, jobExp: 1453, mvpExp: 0,
    attack: 520, attack2: 715, defense: 25, magicDefense: 19,
    str: 10, agi: 68, vit: 15, int: 5, dex: 67, luk: 67,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 175, attackDelay: 862, attackMotion: 534, damageMotion: 312,
    size: 'medium', race: 'insect', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 10, agi: 68, vit: 15, int: 5, dex: 67, luk: 67, level: 61, weaponATK: 520 },
    modes: { detector: true },
    drops: [
        { itemName: 'Wing of Dragonfly', rate: 44.13 },
        { itemName: 'Round Shell', rate: 4 },
        { itemName: 'Solid Shell', rate: 8 },
        { itemName: 'Fancy Flower', rate: 0.08 },
        { itemName: 'Cap', rate: 0.02 },
        { itemName: 'Fly Wing', rate: 3 },
        { itemName: 'Butterfly Wing', rate: 1.5 },
        { itemName: 'Dragon Tail Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Galapago (ID: 1391) ──── Level 61 | HP 9,145 | NORMAL | brute/earth1 | aggressive
RO_MONSTER_TEMPLATES['galapago'] = {
    id: 1391, name: 'Galapago', aegisName: 'GALAPAGO',
    level: 61, maxHealth: 9145, baseExp: 3204, jobExp: 1966, mvpExp: 0,
    attack: 457, attack2: 608, defense: 33, magicDefense: 33,
    str: 0, agi: 56, vit: 56, int: 45, dex: 66, luk: 57,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 165, attackDelay: 1430, attackMotion: 1080, damageMotion: 1080,
    size: 'small', race: 'brute', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 0, agi: 56, vit: 56, int: 45, dex: 66, luk: 57, level: 61, weaponATK: 457 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 53.35 },
        { itemName: 'Yggdrasil Leaf', rate: 1 },
        { itemName: 'Yellow Herb', rate: 35 },
        { itemName: 'Aloevera', rate: 1 },
        { itemName: 'Anodyne', rate: 1 },
        { itemName: 'Galapago Cap', rate: 0.01 },
        { itemName: 'Orange', rate: 10 },
        { itemName: 'Galapago Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baby Hatii (ID: 1515) ──── Level 61 | HP 20,199 | NORMAL | brute/water2 | aggressive
RO_MONSTER_TEMPLATES['garm_baby'] = {
    id: 1515, name: 'Baby Hatii', aegisName: 'GARM_BABY',
    level: 61, maxHealth: 20199, baseExp: 1022, jobExp: 2980, mvpExp: 0,
    attack: 680, attack2: 1179, defense: 34, magicDefense: 13,
    str: 45, agi: 30, vit: 56, int: 55, dex: 85, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 450, attackDelay: 879, attackMotion: 672, damageMotion: 576,
    size: 'medium', race: 'brute', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 45, agi: 30, vit: 56, int: 55, dex: 85, luk: 30, level: 61, weaponATK: 680 },
    modes: {},
    drops: [
        { itemName: 'Nursing Bottle', rate: 15 },
        { itemName: 'Pinafore', rate: 25 },
        { itemName: 'Ice Piece', rate: 43.65 },
        { itemName: 'Frozen Rose', rate: 1 },
        { itemName: 'Level 5 Frost Diver', rate: 1 },
        { itemName: 'Hatii Babe Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Laurell Weinder (ID: 1657) ──── Level 61 | HP 6,168 | NORMAL | demihuman/ghost2 | aggressive
RO_MONSTER_TEMPLATES['rawrel'] = {
    id: 1657, name: 'Laurell Weinder', aegisName: 'RAWREL',
    level: 61, maxHealth: 6168, baseExp: 4620, jobExp: 30, mvpExp: 0,
    attack: 430, attack2: 517, defense: 8, magicDefense: 48,
    str: 0, agi: 41, vit: 5, int: 120, dex: 45, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'ghost', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 0, agi: 41, vit: 5, int: 120, dex: 45, luk: 10, level: 61, weaponATK: 430 },
    modes: {},
    drops: [
        { itemName: 'Memento', rate: 10 },
        { itemName: 'Wing Staff', rate: 0.01 },
        { itemName: 'Guard', rate: 0.05 },
        { itemName: 'Staff', rate: 0.5 },
        { itemName: 'Silk Robe', rate: 0.1 },
        { itemName: 'Silver Robe', rate: 0.3 },
        { itemName: 'Clip', rate: 0.01 },
        { itemName: 'Laurell Weinder Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hodremlin (ID: 1773) ──── Level 61 | HP 12,180 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['hodremlin'] = {
    id: 1773, name: 'Hodremlin', aegisName: 'HODREMLIN',
    level: 61, maxHealth: 12180, baseExp: 6782, jobExp: 2022, mvpExp: 0,
    attack: 845, attack2: 1678, defense: 29, magicDefense: 25,
    str: 80, agi: 41, vit: 81, int: 56, dex: 62, luk: 11,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 140, attackDelay: 960, attackMotion: 528, damageMotion: 432,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27200,
    raceGroups: {},
    stats: { str: 80, agi: 41, vit: 81, int: 56, dex: 62, luk: 11, level: 61, weaponATK: 845 },
    modes: { detector: true },
    drops: [
        { itemName: 'Prickly Fruit', rate: 10 },
        { itemName: 'Will of the Darkness', rate: 10 },
        { itemName: 'Boots', rate: 0.02 },
        { itemName: 'Sticky Mucus', rate: 10 },
        { itemName: 'Bloody Rune', rate: 10 },
        { itemName: 'Witched Starsand', rate: 20 },
        { itemName: 'Shadow Walk', rate: 0.1 },
        { itemName: 'Hodremlin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Alice (ID: 1275) ──── Level 62 | HP 10,000 | NORMAL | demihuman/neutral3 | aggressive
RO_MONSTER_TEMPLATES['alice'] = {
    id: 1275, name: 'Alice', aegisName: 'ALICE',
    level: 62, maxHealth: 10000, baseExp: 3583, jobExp: 2400, mvpExp: 0,
    attack: 550, attack2: 700, defense: 5, magicDefense: 5,
    str: 64, agi: 64, vit: 42, int: 85, dex: 100, luk: 130,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 502, attackMotion: 2304, damageMotion: 480,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 64, agi: 64, vit: 42, int: 85, dex: 100, luk: 130, level: 62, weaponATK: 550 },
    modes: {},
    drops: [
        { itemName: 'Alice\'s Apron', rate: 25 },
        { itemName: 'Old Broom', rate: 0.4 },
        { itemName: 'Crystal Pumps', rate: 0.03 },
        { itemName: 'Rouge', rate: 0.3 },
        { itemName: 'Small Ribbons', rate: 0.01 },
        { itemName: 'Royal Cooking Kit', rate: 0.1 },
        { itemName: 'Level 5 Heal', rate: 1 },
        { itemName: 'Alice Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Creamy Fear (ID: 1293) ──── Level 62 | HP 13,387 | NORMAL | insect/wind1 | aggressive
RO_MONSTER_TEMPLATES['cremy_fear'] = {
    id: 1293, name: 'Creamy Fear', aegisName: 'CREMY_FEAR',
    level: 62, maxHealth: 13387, baseExp: 7365, jobExp: 2691, mvpExp: 0,
    attack: 666, attack2: 829, defense: 45, magicDefense: 30,
    str: 5, agi: 40, vit: 16, int: 15, dex: 68, luk: 55,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 155, attackDelay: 1136, attackMotion: 720, damageMotion: 840,
    size: 'small', race: 'insect', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 5, agi: 40, vit: 16, int: 15, dex: 68, luk: 55, level: 62, weaponATK: 666 },
    modes: { detector: true },
    drops: [
        { itemName: 'Powder of Butterfly', rate: 45.5 },
        { itemName: 'Silver Robe', rate: 0.1 },
        { itemName: 'Honey', rate: 5.5 },
        { itemName: 'Butterfly Wing', rate: 2 },
        { itemName: 'Book', rate: 0.08 },
        { itemName: 'Icarus Wings', rate: 0.05 },
        { itemName: 'Mastela Fruit', rate: 0.5 },
        { itemName: 'Creamy Fear Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zombie Master (ID: 1298) ──── Level 62 | HP 14,211 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['zombie_master'] = {
    id: 1298, name: 'Zombie Master', aegisName: 'ZOMBIE_MASTER',
    level: 62, maxHealth: 14211, baseExp: 7610, jobExp: 2826, mvpExp: 0,
    attack: 824, attack2: 1084, defense: 37, magicDefense: 26,
    str: 25, agi: 20, vit: 30, int: 5, dex: 77, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 148, walkSpeed: 175, attackDelay: 2612, attackMotion: 912, damageMotion: 288,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 25, agi: 20, vit: 30, int: 5, dex: 77, luk: 35, level: 62, weaponATK: 824 },
    modes: {},
    drops: [
        { itemName: 'Tattered Clothes', rate: 44.13 },
        { itemName: 'Sticky Mucus', rate: 15 },
        { itemName: 'Horrendous Mouth', rate: 15 },
        { itemName: 'Ruby', rate: 2 },
        { itemName: 'Opal', rate: 1 },
        { itemName: 'Sharpened Legbone of Ghoul', rate: 0.01 },
        { itemName: 'Scapulare', rate: 0.02 },
        { itemName: 'Zombie Master Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gullinbursti (ID: 1311) ──── Level 62 | HP 21,331 | NORMAL | brute/earth2 | aggressive
RO_MONSTER_TEMPLATES['gullinbursti'] = {
    id: 1311, name: 'Gullinbursti', aegisName: 'GULLINBURSTI',
    level: 62, maxHealth: 21331, baseExp: 5814, jobExp: 2376, mvpExp: 0,
    attack: 699, attack2: 1431, defense: 10, magicDefense: 15,
    str: 55, agi: 25, vit: 60, int: 5, dex: 70, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 960, damageMotion: 384,
    size: 'large', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 55, agi: 25, vit: 60, int: 5, dex: 70, luk: 45, level: 62, weaponATK: 699 },
    modes: {},
    drops: [
        { itemName: 'Wild Boar\'s Mane', rate: 35 },
        { itemName: 'Grape', rate: 2.9 },
        { itemName: 'Animal Gore', rate: 0.06 },
        { itemName: 'Angled Glasses', rate: 0.01 },
        { itemName: 'Anodyne', rate: 0.15 },
        { itemName: 'Belt', rate: 0.01 },
        { itemName: 'Zargon', rate: 1.6 },
        { itemName: 'Gullinbursti Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dullahan (ID: 1504) ──── Level 62 | HP 12,437 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['dullahan'] = {
    id: 1504, name: 'Dullahan', aegisName: 'DULLAHAN',
    level: 62, maxHealth: 12437, baseExp: 4517, jobExp: 2963, mvpExp: 0,
    attack: 647, attack2: 1065, defense: 47, magicDefense: 38,
    str: 0, agi: 30, vit: 5, int: 45, dex: 62, luk: 22,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 155, attackDelay: 847, attackMotion: 1152, damageMotion: 480,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 5, int: 45, dex: 62, luk: 22, level: 62, weaponATK: 647 },
    modes: {},
    drops: [
        { itemName: 'Dullahan\'s Helm', rate: 32 },
        { itemName: 'Armor Piece of Dullahan', rate: 48.5 },
        { itemName: 'Eye of Dullahan', rate: 0.01 },
        { itemName: 'Manteau', rate: 0.13 },
        { itemName: 'Manteau', rate: 0.01 },
        { itemName: 'Dullahan Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mao Guai (ID: 1513) ──── Level 62 | HP 14,390 | NORMAL | brute/wind2 | aggressive
RO_MONSTER_TEMPLATES['civil_servant'] = {
    id: 1513, name: 'Mao Guai', aegisName: 'CIVIL_SERVANT',
    level: 62, maxHealth: 14390, baseExp: 4023, jobExp: 2750, mvpExp: 0,
    attack: 650, attack2: 1010, defense: 42, magicDefense: 5,
    str: 58, agi: 15, vit: 20, int: 60, dex: 80, luk: 50,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 200, attackDelay: 1257, attackMotion: 528, damageMotion: 432,
    size: 'medium', race: 'brute', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 58, agi: 15, vit: 20, int: 60, dex: 80, luk: 50, level: 62, weaponATK: 650 },
    modes: {},
    drops: [
        { itemName: 'Folding Fan of Cat Ghost', rate: 41.71 },
        { itemName: 'Cat\'s Eye', rate: 20 },
        { itemName: 'Aloevera', rate: 0.1 },
        { itemName: 'Fish Tail', rate: 1 },
        { itemName: 'Level 5 Lightening Bolt', rate: 1 },
        { itemName: 'Mao Guai Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wickebine Tres (ID: 1653) ──── Level 62 | HP 7,320 | NORMAL | demihuman/poison3 | aggressive
RO_MONSTER_TEMPLATES['whikebain'] = {
    id: 1653, name: 'Wickebine Tres', aegisName: 'WHIKEBAIN',
    level: 62, maxHealth: 7320, baseExp: 4204, jobExp: 21, mvpExp: 0,
    attack: 693, attack2: 889, defense: 9, magicDefense: 8,
    str: 0, agi: 102, vit: 34, int: 20, dex: 83, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 120, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'poison', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 0, agi: 102, vit: 34, int: 20, dex: 83, luk: 30, level: 62, weaponATK: 693 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 20 },
        { itemName: 'Fortune Sword', rate: 0.01 },
        { itemName: 'Adventurere\'s Suit', rate: 0.4 },
        { itemName: 'Gladius', rate: 0.1 },
        { itemName: 'Chain Mail', rate: 0.02 },
        { itemName: 'Rogue\'s Treasure', rate: 0.02 },
        { itemName: 'Cowardice Blade', rate: 0.1 },
        { itemName: 'Wickebine Tres Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Isilla (ID: 1772) ──── Level 62 | HP 8,297 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['isilla'] = {
    id: 1772, name: 'Isilla', aegisName: 'ISILLA',
    level: 62, maxHealth: 8297, baseExp: 3001, jobExp: 3001, mvpExp: 0,
    attack: 89, attack2: 733, defense: 11, magicDefense: 19,
    str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 300, attackDelay: 768, attackMotion: 360, damageMotion: 432,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12, level: 62, weaponATK: 89 },
    modes: {},
    drops: [
        { itemName: 'White Mask', rate: 25 },
        { itemName: 'High Fashion Sandals', rate: 0.01 },
        { itemName: 'Bloody Rune', rate: 10 },
        { itemName: 'Gold Ring', rate: 0.1 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Bloody Rune', rate: 1 },
        { itemName: 'Isilla Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Aunoe (ID: 1796) ──── Level 62 | HP 21,297 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['aunoe'] = {
    id: 1796, name: 'Aunoe', aegisName: 'AUNOE',
    level: 62, maxHealth: 21297, baseExp: 4102, jobExp: 4102, mvpExp: 0,
    attack: 89, attack2: 733, defense: 11, magicDefense: 19,
    str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 250, attackDelay: 768, attackMotion: 432, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12, level: 62, weaponATK: 89 },
    modes: {},
    drops: [
        { itemName: 'White Mask', rate: 25 },
        { itemName: 'High Fashion Sandals', rate: 0.02 },
        { itemName: 'Bloody Rune', rate: 40 },
        { itemName: 'Memory Book', rate: 0.01 },
        { itemName: 'Holy Arrow Quiver', rate: 0.5 },
        { itemName: 'Bloody Rune', rate: 1 },
        { itemName: 'Musika', rate: 0.05 },
    ],
    mvpDrops: [],
};

// ──── Fanat (ID: 1797) ──── Level 62 | HP 21,297 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['fanat'] = {
    id: 1797, name: 'Fanat', aegisName: 'FANAT',
    level: 62, maxHealth: 21297, baseExp: 4102, jobExp: 4102, mvpExp: 0,
    attack: 89, attack2: 733, defense: 11, magicDefense: 19,
    str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 250, attackDelay: 768, attackMotion: 432, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27400,
    raceGroups: {},
    stats: { str: 0, agi: 28, vit: 12, int: 97, dex: 57, luk: 12, level: 62, weaponATK: 89 },
    modes: {},
    drops: [
        { itemName: 'Kandura', rate: 0.1 },
        { itemName: 'High Fashion Sandals', rate: 0.02 },
        { itemName: 'Bloody Rune', rate: 40 },
        { itemName: 'Memory Book', rate: 0.01 },
        { itemName: 'Holy Arrow Quiver', rate: 0.5 },
        { itemName: 'White Mask', rate: 25 },
    ],
    mvpDrops: [],
};

// ──── Khalitzburg (ID: 1132) ──── Level 63 | HP 19,276 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['khalitzburg'] = {
    id: 1132, name: 'Khalitzburg', aegisName: 'KHALITZBURG',
    level: 63, maxHealth: 19276, baseExp: 4378, jobExp: 2750, mvpExp: 0,
    attack: 875, attack2: 1025, defense: 45, magicDefense: 10,
    str: 58, agi: 65, vit: 48, int: 5, dex: 73, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 189, walkSpeed: 350, attackDelay: 528, attackMotion: 1000, damageMotion: 396,
    size: 'large', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 58, agi: 65, vit: 48, int: 5, dex: 73, luk: 40, level: 63, weaponATK: 875 },
    modes: {},
    drops: [
        { itemName: 'Skel-Bone', rate: 80 },
        { itemName: 'Elunium', rate: 1.91 },
        { itemName: 'Bone Helm', rate: 0.01 },
        { itemName: 'Mirror Shield', rate: 0.02 },
        { itemName: 'Chivalry Emblem', rate: 0.1 },
        { itemName: 'White Herb', rate: 20 },
        { itemName: 'Saber', rate: 0.02 },
        { itemName: 'Khalitzburg Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Zealotus (ID: 1200) ──── Level 63 | HP 18,300 | NORMAL | demihuman/neutral3 | aggressive
RO_MONSTER_TEMPLATES['zherlthsh'] = {
    id: 1200, name: 'Zealotus', aegisName: 'ZHERLTHSH',
    level: 63, maxHealth: 18300, baseExp: 3608, jobExp: 2304, mvpExp: 0,
    attack: 700, attack2: 850, defense: 10, magicDefense: 15,
    str: 70, agi: 85, vit: 40, int: 30, dex: 125, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 200, attackDelay: 800, attackMotion: 2112, damageMotion: 768,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 70, agi: 85, vit: 40, int: 30, dex: 125, luk: 60, level: 63, weaponATK: 700 },
    modes: {},
    drops: [
        { itemName: 'Executioner\'s Mitten', rate: 0.05 },
        { itemName: 'White Herb', rate: 18 },
        { itemName: 'Rose Quartz', rate: 15 },
        { itemName: 'Tights', rate: 0.08 },
        { itemName: 'Earring', rate: 0.01 },
        { itemName: 'Queen\'s Whip', rate: 1 },
        { itemName: 'Masquerade', rate: 0.03 },
        { itemName: 'Zealotus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Clock Tower Manager (ID: 1270) ──── Level 63 | HP 18,600 | NORMAL | formless/neutral4 | aggressive
RO_MONSTER_TEMPLATES['c_tower_manager'] = {
    id: 1270, name: 'Clock Tower Manager', aegisName: 'C_TOWER_MANAGER',
    level: 63, maxHealth: 18600, baseExp: 4378, jobExp: 2850, mvpExp: 0,
    attack: 880, attack2: 1180, defense: 35, magicDefense: 30,
    str: 0, agi: 75, vit: 20, int: 64, dex: 75, luk: 60,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 200, attackDelay: 1072, attackMotion: 672, damageMotion: 384,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 0, agi: 75, vit: 20, int: 64, dex: 75, luk: 60, level: 63, weaponATK: 880 },
    modes: {},
    drops: [
        { itemName: 'Needle of Alarm', rate: 53.35 },
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Steel', rate: 5 },
        { itemName: 'Hinalle Leaflet', rate: 8.5 },
        { itemName: 'Memory Book', rate: 0.01 },
        { itemName: 'Key of Clock Tower', rate: 20 },
        { itemName: 'Key of Underground', rate: 20 },
        { itemName: 'Tower Keeper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gajomart (ID: 1309) ──── Level 63 | HP 13,669 | NORMAL | formless/fire4 | aggressive
RO_MONSTER_TEMPLATES['gajomart'] = {
    id: 1309, name: 'Gajomart', aegisName: 'GAJOMART',
    level: 63, maxHealth: 13669, baseExp: 6625, jobExp: 2900, mvpExp: 0,
    attack: 917, attack2: 950, defense: 85, magicDefense: 50,
    str: 5, agi: 34, vit: 10, int: 5, dex: 75, luk: 140,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 300, attackDelay: 1000, attackMotion: 1152, damageMotion: 828,
    size: 'small', race: 'formless', element: { type: 'fire', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 5, agi: 34, vit: 10, int: 5, dex: 75, luk: 140, level: 63, weaponATK: 917 },
    modes: {},
    drops: [
        { itemName: 'Stone Heart', rate: 65 },
        { itemName: 'Zargon', rate: 23 },
        { itemName: 'Yellow Herb', rate: 8.7 },
        { itemName: 'Bomb Wick', rate: 0.08 },
        { itemName: 'Fire Arrow', rate: 100 },
        { itemName: 'Magic Bible Vol1', rate: 0.2 },
        { itemName: 'Flame Heart', rate: 1.8 },
        { itemName: 'Gajomart Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Permeter (ID: 1314) ──── Level 63 | HP 8,228 | NORMAL | brute/neutral2 | aggressive
RO_MONSTER_TEMPLATES['permeter'] = {
    id: 1314, name: 'Permeter', aegisName: 'PERMETER',
    level: 63, maxHealth: 8228, baseExp: 3756, jobExp: 1955, mvpExp: 0,
    attack: 943, attack2: 1211, defense: 46, magicDefense: 45,
    str: 69, agi: 59, vit: 60, int: 5, dex: 69, luk: 100,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 250, attackDelay: 1100, attackMotion: 483, damageMotion: 528,
    size: 'medium', race: 'brute', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 69, agi: 59, vit: 60, int: 5, dex: 69, luk: 100, level: 63, weaponATK: 943 },
    modes: {},
    drops: [
        { itemName: 'Turtle Shell', rate: 44.13 },
        { itemName: 'Broken Shell', rate: 0.45 },
        { itemName: 'Wooden Block', rate: 12.4 },
        { itemName: 'Red Herb', rate: 24.5 },
        { itemName: 'Zargon', rate: 12.4 },
        { itemName: 'Mastela Fruit', rate: 0.25 },
        { itemName: 'Anodyne', rate: 0.01 },
        { itemName: 'Permeter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Seal (ID: 1317) ──── Level 63 | HP 9,114 | NORMAL | brute/water1 | aggressive
RO_MONSTER_TEMPLATES['fur_seal'] = {
    id: 1317, name: 'Seal', aegisName: 'FUR_SEAL',
    level: 63, maxHealth: 9114, baseExp: 3765, jobExp: 1824, mvpExp: 0,
    attack: 845, attack2: 1203, defense: 25, magicDefense: 33,
    str: 5, agi: 28, vit: 22, int: 15, dex: 69, luk: 84,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 168, walkSpeed: 200, attackDelay: 1612, attackMotion: 622, damageMotion: 583,
    size: 'medium', race: 'brute', element: { type: 'water', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 5, agi: 28, vit: 22, int: 15, dex: 69, luk: 84, level: 63, weaponATK: 845 },
    modes: {},
    drops: [
        { itemName: 'Zargon', rate: 43.65 },
        { itemName: 'Blue Herb', rate: 2.5 },
        { itemName: 'Coat', rate: 0.05 },
        { itemName: 'Cyfar', rate: 12 },
        { itemName: 'Guisarme', rate: 0.01 },
        { itemName: 'Panacea', rate: 2 },
        { itemName: 'Glass Bead', rate: 1.2 },
        { itemName: 'Seal Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Evil Nymph (ID: 1416) ──── Level 63 | HP 16,029 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['wicked_nymph'] = {
    id: 1416, name: 'Evil Nymph', aegisName: 'WICKED_NYMPH',
    level: 63, maxHealth: 16029, baseExp: 3945, jobExp: 2599, mvpExp: 0,
    attack: 399, attack2: 1090, defense: 12, magicDefense: 75,
    str: 0, agi: 64, vit: 12, int: 69, dex: 100, luk: 80,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 200, attackDelay: 637, attackMotion: 1008, damageMotion: 360,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27600,
    raceGroups: {},
    stats: { str: 0, agi: 64, vit: 12, int: 69, dex: 100, luk: 80, level: 63, weaponATK: 399 },
    modes: { detector: true },
    drops: [
        { itemName: 'Transparent Celestial Robe', rate: 39.77 },
        { itemName: 'Soft Silk', rate: 13.8 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Mandolin', rate: 0.04 },
        { itemName: 'Lute', rate: 0.01 },
        { itemName: 'Level 5 Heal', rate: 1 },
        { itemName: 'Oriental Lute', rate: 0.1 },
        { itemName: 'Evil Nymph Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Golden Thief Bug (ID: 1086) ──── Level 64 | HP 126,000 | MVP | insect/fire2 | aggressive
RO_MONSTER_TEMPLATES['golden_bug'] = {
    id: 1086, name: 'Golden Thief Bug', aegisName: 'GOLDEN_BUG',
    level: 64, maxHealth: 126000, baseExp: 14300, jobExp: 7150, mvpExp: 7150,
    attack: 870, attack2: 1145, defense: 60, magicDefense: 45,
    str: 65, agi: 75, vit: 35, int: 45, dex: 85, luk: 150,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 100, attackDelay: 768, attackMotion: 768, damageMotion: 480,
    size: 'large', race: 'insect', element: { type: 'fire', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 65, agi: 75, vit: 35, int: 45, dex: 85, luk: 150, level: 64, weaponATK: 870 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Gold', rate: 10 },
        { itemName: 'Golden Mace', rate: 1.5 },
        { itemName: 'Golden Gear', rate: 2.5 },
        { itemName: 'Golden Bell', rate: 5 },
        { itemName: 'Emperium', rate: 3 },
        { itemName: 'Elunium', rate: 20 },
        { itemName: 'Oridecon', rate: 15 },
        { itemName: 'Golden Thief Bug Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Gold Ring', rate: 20 },
    ],
};

// ──── Ancient Mummy (ID: 1297) ──── Level 64 | HP 40,599 | NORMAL | undead/undead2 | aggressive
RO_MONSTER_TEMPLATES['ancient_mummy'] = {
    id: 1297, name: 'Ancient Mummy', aegisName: 'ANCIENT_MUMMY',
    level: 64, maxHealth: 40599, baseExp: 8040, jobExp: 3499, mvpExp: 0,
    attack: 836, attack2: 1129, defense: 27, magicDefense: 27,
    str: 28, agi: 19, vit: 32, int: 5, dex: 83, luk: 35,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 175, attackDelay: 1772, attackMotion: 120, damageMotion: 384,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27800,
    raceGroups: {},
    stats: { str: 28, agi: 19, vit: 32, int: 5, dex: 83, luk: 35, level: 64, weaponATK: 836 },
    modes: {},
    drops: [
        { itemName: 'Rotten Bandage', rate: 44.13 },
        { itemName: 'Memento', rate: 18 },
        { itemName: 'Glove', rate: 0.01 },
        { itemName: 'Silver Ring', rate: 1.5 },
        { itemName: 'Yellow Herb', rate: 6.5 },
        { itemName: 'Rough Oridecon', rate: 1.5 },
        { itemName: 'Rough Elunium', rate: 1 },
        { itemName: 'Ancient Mummy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Goblin Leader (ID: 1299) ──── Level 64 | HP 20,152 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['goblin_leader'] = {
    id: 1299, name: 'Goblin Leader', aegisName: 'GOBLIN_LEADER',
    level: 64, maxHealth: 20152, baseExp: 6036, jobExp: 2184, mvpExp: 0,
    attack: 663, attack2: 752, defense: 48, magicDefense: 16,
    str: 5, agi: 55, vit: 37, int: 30, dex: 69, luk: 58,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 27800,
    raceGroups: { Goblin: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 5, agi: 55, vit: 37, int: 30, dex: 69, luk: 58, level: 64, weaponATK: 663 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 15 },
        { itemName: 'Steel', rate: 8 },
        { itemName: 'Rough Oridecon', rate: 1.2 },
        { itemName: 'Goblin Leader Mask', rate: 0.5 },
        { itemName: 'Shield', rate: 0.02 },
        { itemName: 'Yellow Herb', rate: 6.5 },
        { itemName: 'Angry Mouth', rate: 0.1 },
        { itemName: 'Goblin Leader Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Caterpillar (ID: 1300) ──── Level 64 | HP 14,439 | NORMAL | insect/earth1 | aggressive
RO_MONSTER_TEMPLATES['caterpillar'] = {
    id: 1300, name: 'Caterpillar', aegisName: 'CATERPILLAR',
    level: 64, maxHealth: 14439, baseExp: 6272, jobExp: 3107, mvpExp: 0,
    attack: 894, attack2: 1447, defense: 47, magicDefense: 29,
    str: 35, agi: 25, vit: 85, int: 15, dex: 69, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 300, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'insect', element: { type: 'earth', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27800,
    raceGroups: {},
    stats: { str: 35, agi: 25, vit: 85, int: 15, dex: 69, luk: 45, level: 64, weaponATK: 894 },
    modes: { detector: true },
    drops: [
        { itemName: 'Feather', rate: 30 },
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Desert Twilight', rate: 0.2 },
        { itemName: 'Star Crumb', rate: 1 },
        { itemName: 'Great Nature', rate: 0.5 },
        { itemName: 'Blue Potion', rate: 0.12 },
        { itemName: 'Yellow Herb', rate: 5 },
        { itemName: 'Caterpillar Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Elder (ID: 1377) ──── Level 64 | HP 21,592 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['elder'] = {
    id: 1377, name: 'Elder', aegisName: 'ELDER',
    level: 64, maxHealth: 21592, baseExp: 5650, jobExp: 3408, mvpExp: 0,
    attack: 421, attack2: 560, defense: 45, magicDefense: 68,
    str: 0, agi: 76, vit: 68, int: 108, dex: 72, luk: 86,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 169, walkSpeed: 165, attackDelay: 1552, attackMotion: 1152, damageMotion: 336,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 27800,
    raceGroups: {},
    stats: { str: 0, agi: 76, vit: 68, int: 108, dex: 72, luk: 86, level: 64, weaponATK: 421 },
    modes: {},
    drops: [
        { itemName: 'Worn-out Magic Scroll', rate: 40 },
        { itemName: 'Torn Magic Book', rate: 15 },
        { itemName: 'Torn Scroll', rate: 15 },
        { itemName: 'Encyclopedia', rate: 0.1 },
        { itemName: 'Wizardry Staff', rate: 0.01 },
        { itemName: 'Old Card Album', rate: 0.01 },
        { itemName: 'Key of Underground', rate: 30 },
        { itemName: 'Elder Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Photon Cannon (ID: 1666) ──── Level 64 | HP 7,100 | NORMAL | formless/neutral2 | passive
RO_MONSTER_TEMPLATES['poton_canon_2'] = {
    id: 1666, name: 'Photon Cannon', aegisName: 'POTON_CANON_2',
    level: 64, maxHealth: 7100, baseExp: 3100, jobExp: 2700, mvpExp: 0,
    attack: 800, attack2: 900, defense: 8, magicDefense: 30,
    str: 0, agi: 40, vit: 21, int: 29, dex: 80, luk: 91,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1536, attackMotion: 960, damageMotion: 480,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 27800,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 21, int: 29, dex: 80, luk: 91, level: 64, weaponATK: 800 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 50 },
        { itemName: 'Emerald', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 10 },
    ],
    mvpDrops: [],
};

// ──── Eddga (ID: 1115) ──── Level 65 | HP 152,000 | MVP | brute/fire1 | aggressive
RO_MONSTER_TEMPLATES['eddga'] = {
    id: 1115, name: 'Eddga', aegisName: 'EDDGA',
    level: 65, maxHealth: 152000, baseExp: 25025, jobExp: 12870, mvpExp: 12512,
    attack: 1215, attack2: 1565, defense: 15, magicDefense: 15,
    str: 78, agi: 70, vit: 85, int: 66, dex: 90, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 300, attackDelay: 872, attackMotion: 1344, damageMotion: 432,
    size: 'large', race: 'brute', element: { type: 'fire', level: 1 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 78, agi: 70, vit: 85, int: 66, dex: 90, luk: 85, level: 65, weaponATK: 1215 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Fireblend', rate: 1.5 },
        { itemName: 'Pipe', rate: 2.5 },
        { itemName: 'Honey', rate: 100 },
        { itemName: 'Katar of Raging Blaze', rate: 5 },
        { itemName: 'Tiger\'s Footskin', rate: 2.5 },
        { itemName: 'Elunium', rate: 23 },
        { itemName: 'Krierg', rate: 1 },
        { itemName: 'Eddga Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Tiger\'s Skin', rate: 50 },
        { itemName: 'Tiger\'s Footskin', rate: 10 },
    ],
};

// ──── Executioner (ID: 1205) ──── Level 65 | HP 28,980 | BOSS | formless/shadow2 | aggressive
RO_MONSTER_TEMPLATES['executioner'] = {
    id: 1205, name: 'Executioner', aegisName: 'EXECUTIONER',
    level: 65, maxHealth: 28980, baseExp: 4730, jobExp: 3536, mvpExp: 0,
    attack: 570, attack2: 950, defense: 35, magicDefense: 35,
    str: 64, agi: 85, vit: 40, int: 25, dex: 88, luk: 60,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 200, attackDelay: 768, attackMotion: 500, damageMotion: 384,
    size: 'large', race: 'formless', element: { type: 'shadow', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 64, agi: 85, vit: 40, int: 25, dex: 88, luk: 60, level: 65, weaponATK: 570 },
    modes: {},
    drops: [
        { itemName: 'Bloody Edge', rate: 0.05 },
        { itemName: 'Phlogopite', rate: 15 },
        { itemName: 'Rapier', rate: 0.8 },
        { itemName: 'Scimitar', rate: 0.6 },
        { itemName: 'Ring Pommel Saber', rate: 0.4 },
        { itemName: 'Steel', rate: 1.2 },
        { itemName: 'Oridecon', rate: 1.45 },
        { itemName: 'Executioner Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mutant Dragonoid (ID: 1262) ──── Level 65 | HP 62,600 | BOSS | dragon/fire2 | aggressive
RO_MONSTER_TEMPLATES['mutant_dragon'] = {
    id: 1262, name: 'Mutant Dragonoid', aegisName: 'MUTANT_DRAGON',
    level: 65, maxHealth: 62600, baseExp: 4730, jobExp: 3536, mvpExp: 0,
    attack: 2400, attack2: 3400, defense: 15, magicDefense: 20,
    str: 75, agi: 47, vit: 30, int: 68, dex: 45, luk: 35,
    attackRange: 200, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 250, attackDelay: 1280, attackMotion: 1080, damageMotion: 240,
    size: 'large', race: 'dragon', element: { type: 'fire', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 75, agi: 47, vit: 30, int: 68, dex: 45, luk: 35, level: 65, weaponATK: 2400 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 48.5 },
        { itemName: 'Dragon Canine', rate: 5 },
        { itemName: 'Dragon Scale', rate: 5 },
        { itemName: 'Rotten Bandage', rate: 5 },
        { itemName: 'Legacy of Dragon', rate: 1 },
        { itemName: 'Pyroxene', rate: 15 },
        { itemName: 'Dragon Breath', rate: 0.5 },
        { itemName: 'Mutant Dragonoid Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kobold Leader (ID: 1296) ──── Level 65 | HP 18,313 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['kobold_leader'] = {
    id: 1296, name: 'Kobold Leader', aegisName: 'KOBOLD_LEADER',
    level: 65, maxHealth: 18313, baseExp: 7432, jobExp: 2713, mvpExp: 0,
    attack: 649, attack2: 958, defense: 37, magicDefense: 37,
    str: 5, agi: 90, vit: 36, int: 30, dex: 77, luk: 59,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 28000,
    raceGroups: { Kobold: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 5, agi: 90, vit: 36, int: 30, dex: 77, luk: 59, level: 65, weaponATK: 649 },
    modes: {},
    drops: [
        { itemName: 'Steel', rate: 4.5 },
        { itemName: 'Blue Hair', rate: 63.05 },
        { itemName: 'Zargon', rate: 12 },
        { itemName: 'Flail', rate: 0.06 },
        { itemName: 'Mighty Staff', rate: 0.02 },
        { itemName: 'Panacea', rate: 1.5 },
        { itemName: 'Royal Jelly', rate: 1 },
        { itemName: 'Kobold Leader Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── False Angel (ID: 1371) ──── Level 65 | HP 16,845 | NORMAL | angel/holy3 | aggressive
RO_MONSTER_TEMPLATES['fake_angel'] = {
    id: 1371, name: 'False Angel', aegisName: 'FAKE_ANGEL',
    level: 65, maxHealth: 16845, baseExp: 3371, jobExp: 1949, mvpExp: 0,
    attack: 513, attack2: 682, defense: 50, magicDefense: 35,
    str: 0, agi: 64, vit: 57, int: 70, dex: 61, luk: 88,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 160, attackDelay: 920, attackMotion: 720, damageMotion: 336,
    size: 'small', race: 'angel', element: { type: 'holy', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 0, agi: 64, vit: 57, int: 70, dex: 61, luk: 88, level: 65, weaponATK: 513 },
    modes: {},
    drops: [
        { itemName: 'Blue Gemstone', rate: 10 },
        { itemName: 'Yellow Gemstone', rate: 10 },
        { itemName: 'Red Gemstone', rate: 10 },
        { itemName: 'Cursed Water', rate: 10 },
        { itemName: 'Carrot Whip', rate: 0.2 },
        { itemName: 'False Angel Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deleter (ID: 1385) ──── Level 65 | HP 15,168 | NORMAL | dragon/fire2 | aggressive
RO_MONSTER_TEMPLATES['deleter_'] = {
    id: 1385, name: 'Deleter', aegisName: 'DELETER_',
    level: 65, maxHealth: 15168, baseExp: 3403, jobExp: 2066, mvpExp: 0,
    attack: 446, attack2: 593, defense: 52, magicDefense: 53,
    str: 0, agi: 66, vit: 40, int: 65, dex: 72, luk: 68,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 175, attackDelay: 1024, attackMotion: 624, damageMotion: 336,
    size: 'medium', race: 'dragon', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 40, int: 65, dex: 72, luk: 68, level: 65, weaponATK: 446 },
    modes: {},
    drops: [
        { itemName: 'Dragon\'s Skin', rate: 40.74 },
        { itemName: 'Dragon Canine', rate: 53.35 },
        { itemName: 'Dragon Tail', rate: 38.8 },
        { itemName: 'Dragon Scale', rate: 35.89 },
        { itemName: 'Earth Deleter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Tengu (ID: 1405) ──── Level 65 | HP 16,940 | NORMAL | demon/earth2 | aggressive
RO_MONSTER_TEMPLATES['tengu'] = {
    id: 1405, name: 'Tengu', aegisName: 'TENGU',
    level: 65, maxHealth: 16940, baseExp: 4207, jobExp: 2843, mvpExp: 0,
    attack: 660, attack2: 980, defense: 12, magicDefense: 82,
    str: 90, agi: 42, vit: 69, int: 45, dex: 78, luk: 80,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 200, attackDelay: 1439, attackMotion: 1920, damageMotion: 672,
    size: 'large', race: 'demon', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 90, agi: 42, vit: 69, int: 45, dex: 78, luk: 80, level: 65, weaponATK: 660 },
    modes: { detector: true },
    drops: [
        { itemName: 'Tengu\'s Nose', rate: 35 },
        { itemName: 'Broken Liquor Jar', rate: 55 },
        { itemName: 'Huuma Giant Wheel Shuriken', rate: 0.05 },
        { itemName: 'Mastela Fruit', rate: 1.5 },
        { itemName: 'Huuma Giant Wheel Shuriken', rate: 0.05 },
        { itemName: 'Royal Cooking Kit', rate: 0.2 },
        { itemName: 'Level 5 Earth Spike', rate: 1 },
        { itemName: 'Tengu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Photon Cannon (ID: 1667) ──── Level 65 | HP 7,800 | NORMAL | formless/neutral2 | passive
RO_MONSTER_TEMPLATES['poton_canon_3'] = {
    id: 1667, name: 'Photon Cannon', aegisName: 'POTON_CANON_3',
    level: 65, maxHealth: 7800, baseExp: 3800, jobExp: 2300, mvpExp: 0,
    attack: 700, attack2: 800, defense: 15, magicDefense: 30,
    str: 0, agi: 40, vit: 23, int: 30, dex: 90, luk: 99,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1536, attackMotion: 960, damageMotion: 480,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 23, int: 30, dex: 90, luk: 99, level: 65, weaponATK: 700 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 50 },
        { itemName: 'Topaz', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 10 },
    ],
    mvpDrops: [],
};

// ──── Death Word (ID: 1698) ──── Level 65 | HP 18,990 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['deathword'] = {
    id: 1698, name: 'Death Word', aegisName: 'DEATHWORD',
    level: 65, maxHealth: 18990, baseExp: 2986, jobExp: 4912, mvpExp: 0,
    attack: 622, attack2: 1102, defense: 10, magicDefense: 40,
    str: 50, agi: 75, vit: 10, int: 20, dex: 140, luk: 45,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 176, attackMotion: 912, damageMotion: 300,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 50, agi: 75, vit: 10, int: 20, dex: 140, luk: 45, level: 65, weaponATK: 622 },
    modes: {},
    drops: [
        { itemName: 'Worn Out Page', rate: 40 },
        { itemName: 'Bookclip in Memory', rate: 3 },
        { itemName: 'Kafra Legend Vol.1', rate: 0.5 },
        { itemName: 'Bloody Page', rate: 5 },
        { itemName: 'Vidar\'s Boots', rate: 0.1 },
        { itemName: 'Level 8 Cookbook', rate: 0.02 },
        { itemName: 'Level 9 Cookbook', rate: 0.01 },
        { itemName: 'Death Word Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Seeker (ID: 1774) ──── Level 65 | HP 10,090 | NORMAL | formless/wind3 | aggressive
RO_MONSTER_TEMPLATES['seeker'] = {
    id: 1774, name: 'Seeker', aegisName: 'SEEKER',
    level: 65, maxHealth: 10090, baseExp: 5671, jobExp: 4278, mvpExp: 0,
    attack: 723, attack2: 852, defense: 17, magicDefense: 30,
    str: 60, agi: 52, vit: 34, int: 143, dex: 107, luk: 27,
    attackRange: 300, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 190, attackDelay: 576, attackMotion: 432, damageMotion: 300,
    size: 'small', race: 'formless', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28000,
    raceGroups: {},
    stats: { str: 60, agi: 52, vit: 34, int: 143, dex: 107, luk: 27, level: 65, weaponATK: 723 },
    modes: {},
    drops: [
        { itemName: 'Prickly Fruit', rate: 10 },
        { itemName: 'Will of the Darkness', rate: 10 },
        { itemName: 'Elunium', rate: 0.2 },
        { itemName: 'Witched Starsand', rate: 40 },
        { itemName: 'Bloody Rune', rate: 10 },
        { itemName: 'Berdysz', rate: 0.2 },
        { itemName: 'Seeker Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Majoruros (ID: 1310) ──── Level 66 | HP 57,991 | NORMAL | brute/fire2 | aggressive
RO_MONSTER_TEMPLATES['majoruros'] = {
    id: 1310, name: 'Majoruros', aegisName: 'MAJORUROS',
    level: 66, maxHealth: 57991, baseExp: 8525, jobExp: 3799, mvpExp: 0,
    attack: 780, attack2: 1300, defense: 10, magicDefense: 25,
    str: 65, agi: 50, vit: 75, int: 50, dex: 85, luk: 48,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 250, attackDelay: 1100, attackMotion: 960, damageMotion: 780,
    size: 'large', race: 'brute', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28200,
    raceGroups: {},
    stats: { str: 65, agi: 50, vit: 75, int: 50, dex: 85, luk: 48, level: 66, weaponATK: 780 },
    modes: {},
    drops: [
        { itemName: 'Nose Ring', rate: 44.13 },
        { itemName: 'Two-Handed Axe', rate: 0.04 },
        { itemName: 'Lemon', rate: 3 },
        { itemName: 'Oridecon', rate: 0.16 },
        { itemName: 'White Herb', rate: 18.5 },
        { itemName: 'Silver Ring', rate: 1.6 },
        { itemName: 'Star Crumb', rate: 2.5 },
        { itemName: 'Majoruros Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Apocalypse (ID: 1365) ──── Level 66 | HP 22,880 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['apocalips'] = {
    id: 1365, name: 'Apocalypse', aegisName: 'APOCALIPS',
    level: 66, maxHealth: 22880, baseExp: 6540, jobExp: 4935, mvpExp: 0,
    attack: 1030, attack2: 1370, defense: 62, magicDefense: 49,
    str: 0, agi: 48, vit: 120, int: 48, dex: 66, luk: 85,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 400, attackDelay: 1840, attackMotion: 1440, damageMotion: 384,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28200,
    raceGroups: {},
    stats: { str: 0, agi: 48, vit: 120, int: 48, dex: 66, luk: 85, level: 66, weaponATK: 1030 },
    modes: {},
    drops: [
        { itemName: 'Metal Fragment', rate: 53.35 },
        { itemName: 'Fragment', rate: 24 },
        { itemName: 'Cogwheel', rate: 22 },
        { itemName: 'Elunium', rate: 0.05 },
        { itemName: 'Destroyer', rate: 0.01 },
        { itemName: 'Manteau', rate: 0.2 },
        { itemName: 'Steel', rate: 25 },
        { itemName: 'Apocalipse Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Deleter (ID: 1384) ──── Level 66 | HP 17,292 | NORMAL | dragon/fire2 | aggressive
RO_MONSTER_TEMPLATES['deleter'] = {
    id: 1384, name: 'Deleter', aegisName: 'DELETER',
    level: 66, maxHealth: 17292, baseExp: 3403, jobExp: 2066, mvpExp: 0,
    attack: 446, attack2: 593, defense: 45, magicDefense: 53,
    str: 0, agi: 104, vit: 40, int: 65, dex: 72, luk: 54,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 175, attackDelay: 1020, attackMotion: 720, damageMotion: 384,
    size: 'medium', race: 'dragon', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28200,
    raceGroups: {},
    stats: { str: 0, agi: 104, vit: 40, int: 65, dex: 72, luk: 54, level: 66, weaponATK: 446 },
    modes: {},
    drops: [
        { itemName: 'Dragon\'s Skin', rate: 40.74 },
        { itemName: 'Dragon Canine', rate: 53.35 },
        { itemName: 'Dragon Tail', rate: 38.8 },
        { itemName: 'Dragon Scale', rate: 35.89 },
        { itemName: 'Sky Deleter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Armeyer Dinze (ID: 1654) ──── Level 66 | HP 7,110 | NORMAL | demihuman/earth3 | aggressive
RO_MONSTER_TEMPLATES['armaia'] = {
    id: 1654, name: 'Armeyer Dinze', aegisName: 'ARMAIA',
    level: 66, maxHealth: 7110, baseExp: 4008, jobExp: 35, mvpExp: 0,
    attack: 750, attack2: 913, defense: 42, magicDefense: 6,
    str: 5, agi: 36, vit: 50, int: 15, dex: 89, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 120, attackDelay: 576, attackMotion: 432, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28200,
    raceGroups: {},
    stats: { str: 5, agi: 36, vit: 50, int: 15, dex: 89, luk: 60, level: 66, weaponATK: 750 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 10 },
        { itemName: 'Muffler', rate: 0.01 },
        { itemName: 'Buster', rate: 0.5 },
        { itemName: 'Battle Axe', rate: 0.4 },
        { itemName: 'Mink Coat', rate: 0.1 },
        { itemName: 'Axe', rate: 0.8 },
        { itemName: 'Windhawk', rate: 0.1 },
        { itemName: 'Armeyer Dinze Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Photon Cannon (ID: 1664) ──── Level 66 | HP 8,000 | NORMAL | formless/neutral2 | passive
RO_MONSTER_TEMPLATES['poton_canon'] = {
    id: 1664, name: 'Photon Cannon', aegisName: 'POTON_CANON',
    level: 66, maxHealth: 8000, baseExp: 3900, jobExp: 1800, mvpExp: 0,
    attack: 800, attack2: 900, defense: 10, magicDefense: 30,
    str: 0, agi: 40, vit: 25, int: 20, dex: 80, luk: 80,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1536, attackMotion: 960, damageMotion: 480,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 28200,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 25, int: 20, dex: 80, luk: 80, level: 66, weaponATK: 800 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 50 },
        { itemName: 'Garnet', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 10 },
    ],
    mvpDrops: [],
};

// ──── Moonlight Flower (ID: 1150) ──── Level 67 | HP 120,000 | MVP | demon/fire3 | aggressive
RO_MONSTER_TEMPLATES['moonlight'] = {
    id: 1150, name: 'Moonlight Flower', aegisName: 'MOONLIGHT',
    level: 67, maxHealth: 120000, baseExp: 27500, jobExp: 14300, mvpExp: 13750,
    attack: 1200, attack2: 1700, defense: 10, magicDefense: 55,
    str: 55, agi: 99, vit: 55, int: 82, dex: 95, luk: 120,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 150, attackDelay: 1276, attackMotion: 576, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'fire', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 55, agi: 99, vit: 55, int: 82, dex: 95, luk: 120, level: 67, weaponATK: 1200 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Spectral Spear', rate: 5 },
        { itemName: 'Moonlight Dagger', rate: 1 },
        { itemName: 'Long Mace', rate: 1.5 },
        { itemName: 'Punisher', rate: 5 },
        { itemName: 'Silver Knife of Chastity', rate: 6.5 },
        { itemName: 'Elunium', rate: 26 },
        { itemName: 'Staff Of Bordeaux', rate: 1 },
        { itemName: 'Moonlight Flower Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Nine Tails', rate: 50 },
        { itemName: 'White Potion', rate: 15 },
    ],
};

// ──── Ancient Worm (ID: 1305) ──── Level 67 | HP 22,598 | NORMAL | insect/poison1 | aggressive
RO_MONSTER_TEMPLATES['ancient_worm'] = {
    id: 1305, name: 'Ancient Worm', aegisName: 'ANCIENT_WORM',
    level: 67, maxHealth: 22598, baseExp: 8174, jobExp: 3782, mvpExp: 0,
    attack: 948, attack2: 1115, defense: 35, magicDefense: 30,
    str: 5, agi: 35, vit: 56, int: 55, dex: 81, luk: 72,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 165, attackDelay: 1792, attackMotion: 792, damageMotion: 336,
    size: 'large', race: 'insect', element: { type: 'poison', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28400,
    raceGroups: {},
    stats: { str: 5, agi: 35, vit: 56, int: 55, dex: 81, luk: 72, level: 67, weaponATK: 948 },
    modes: { detector: true },
    drops: [
        { itemName: 'Bug Leg', rate: 44.13 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Boots', rate: 0.09 },
        { itemName: 'Bowman Scarf', rate: 0.05 },
        { itemName: 'Round Shell', rate: 6.8 },
        { itemName: 'Sticky Mucus', rate: 35 },
        { itemName: 'Brigan', rate: 25 },
        { itemName: 'Ancient Worm Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Diabolic (ID: 1382) ──── Level 67 | HP 9,642 | NORMAL | demon/shadow2 | aggressive
RO_MONSTER_TEMPLATES['diabolic'] = {
    id: 1382, name: 'Diabolic', aegisName: 'DIABOLIC',
    level: 67, maxHealth: 9642, baseExp: 3662, jobExp: 2223, mvpExp: 0,
    attack: 796, attack2: 1059, defense: 64, magicDefense: 36,
    str: 0, agi: 84, vit: 53, int: 67, dex: 71, luk: 69,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 150, attackDelay: 1080, attackMotion: 780, damageMotion: 180,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28400,
    raceGroups: {},
    stats: { str: 0, agi: 84, vit: 53, int: 67, dex: 71, luk: 69, level: 67, weaponATK: 796 },
    modes: { detector: true },
    drops: [
        { itemName: 'Little Evil Horn', rate: 58.2 },
        { itemName: 'Little Evil Wing', rate: 48.5 },
        { itemName: 'Brooch', rate: 0.03 },
        { itemName: 'Oridecon', rate: 0.2 },
        { itemName: 'Unholy Touch', rate: 0.1 },
        { itemName: 'Diabolic Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Sleeper (ID: 1386) ──── Level 67 | HP 8,237 | NORMAL | formless/earth2 | aggressive
RO_MONSTER_TEMPLATES['sleeper'] = {
    id: 1386, name: 'Sleeper', aegisName: 'SLEEPER',
    level: 67, maxHealth: 8237, baseExp: 3603, jobExp: 2144, mvpExp: 0,
    attack: 593, attack2: 789, defense: 49, magicDefense: 35,
    str: 0, agi: 48, vit: 100, int: 57, dex: 75, luk: 28,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 195, attackDelay: 1350, attackMotion: 1200, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28400,
    raceGroups: {},
    stats: { str: 0, agi: 48, vit: 100, int: 57, dex: 75, luk: 28, level: 67, weaponATK: 593 },
    modes: {},
    drops: [
        { itemName: 'Sand Clump', rate: 49.47 },
        { itemName: 'Grit', rate: 53.35 },
        { itemName: 'Great Nature', rate: 25 },
        { itemName: 'Rough Oridecon', rate: 3 },
        { itemName: 'Damascus', rate: 0.05 },
        { itemName: 'Hypnotist\'s Staff', rate: 0.05 },
        { itemName: 'Fine Sand', rate: 12 },
        { itemName: 'Sleeper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Photon Cannon (ID: 1665) ──── Level 67 | HP 7,500 | NORMAL | formless/neutral2 | passive
RO_MONSTER_TEMPLATES['poton_canon_1'] = {
    id: 1665, name: 'Photon Cannon', aegisName: 'POTON_CANON_1',
    level: 67, maxHealth: 7500, baseExp: 4300, jobExp: 2000, mvpExp: 0,
    attack: 700, attack2: 800, defense: 15, magicDefense: 30,
    str: 0, agi: 40, vit: 30, int: 40, dex: 86, luk: 80,
    attackRange: 450, aggroRange: 0, chaseRange: 600,
    aspd: 169, walkSpeed: 300, attackDelay: 1536, attackMotion: 960, damageMotion: 480,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 28400,
    raceGroups: {},
    stats: { str: 0, agi: 40, vit: 30, int: 40, dex: 86, luk: 80, level: 67, weaponATK: 700 },
    modes: {},
    drops: [
        { itemName: 'Large Jellopy', rate: 50 },
        { itemName: 'Blue Jewel', rate: 10 },
        { itemName: 'Sticky Mucus', rate: 10 },
        { itemName: 'Destroyer', rate: 0.05 },
    ],
    mvpDrops: [],
};

// ──── Mini Demon (ID: 1292) ──── Level 68 | HP 32,538 | NORMAL | demon/shadow1 | aggressive
RO_MONSTER_TEMPLATES['mini_demon'] = {
    id: 1292, name: 'Mini Demon', aegisName: 'MINI_DEMON',
    level: 68, maxHealth: 32538, baseExp: 8396, jobExp: 3722, mvpExp: 0,
    attack: 1073, attack2: 1414, defense: 30, magicDefense: 25,
    str: 5, agi: 75, vit: 40, int: 55, dex: 89, luk: 42,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 150, attackDelay: 1000, attackMotion: 600, damageMotion: 384,
    size: 'small', race: 'demon', element: { type: 'shadow', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28600,
    raceGroups: {},
    stats: { str: 5, agi: 75, vit: 40, int: 55, dex: 89, luk: 42, level: 68, weaponATK: 1073 },
    modes: { detector: true },
    drops: [
        { itemName: 'Little Evil Horn', rate: 44.13 },
        { itemName: 'Little Evil Wing', rate: 4.5 },
        { itemName: 'Evil Wing', rate: 0.03 },
        { itemName: 'Rough Elunium', rate: 1.6 },
        { itemName: 'Zargon', rate: 25 },
        { itemName: 'Hand of God', rate: 0.1 },
        { itemName: 'Ahlspiess', rate: 0.05 },
        { itemName: 'Mini Demon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Heater (ID: 1318) ──── Level 68 | HP 11,020 | NORMAL | brute/fire2 | aggressive
RO_MONSTER_TEMPLATES['heater'] = {
    id: 1318, name: 'Heater', aegisName: 'HEATER',
    level: 68, maxHealth: 11020, baseExp: 3766, jobExp: 2359, mvpExp: 0,
    attack: 683, attack2: 1008, defense: 40, magicDefense: 42,
    str: 69, agi: 47, vit: 25, int: 5, dex: 71, luk: 100,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 250, attackDelay: 1452, attackMotion: 483, damageMotion: 528,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28600,
    raceGroups: {},
    stats: { str: 69, agi: 47, vit: 25, int: 5, dex: 71, luk: 100, level: 68, weaponATK: 683 },
    modes: {},
    drops: [
        { itemName: 'Turtle Shell', rate: 44.13 },
        { itemName: 'Broken Shell', rate: 7.5 },
        { itemName: 'Level 5 Fire Ball', rate: 1 },
        { itemName: 'Zargon', rate: 16.4 },
        { itemName: 'Royal Jelly', rate: 1.4 },
        { itemName: 'Brigan', rate: 6 },
        { itemName: 'Burnt Tree', rate: 12.5 },
        { itemName: 'Heater Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Grizzly (ID: 1381) ──── Level 68 | HP 11,733 | NORMAL | brute/fire3 | aggressive
RO_MONSTER_TEMPLATES['grizzly'] = {
    id: 1381, name: 'Grizzly', aegisName: 'GRIZZLY',
    level: 68, maxHealth: 11733, baseExp: 3341, jobExp: 2012, mvpExp: 0,
    attack: 809, attack2: 1076, defense: 44, magicDefense: 32,
    str: 0, agi: 55, vit: 68, int: 58, dex: 70, luk: 61,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 165, attackDelay: 1492, attackMotion: 1092, damageMotion: 192,
    size: 'large', race: 'brute', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28600,
    raceGroups: {},
    stats: { str: 0, agi: 55, vit: 68, int: 58, dex: 70, luk: 61, level: 68, weaponATK: 809 },
    modes: {},
    drops: [
        { itemName: 'Bear\'s Foot', rate: 50 },
        { itemName: 'Animal\'s Skin', rate: 50 },
        { itemName: 'Yam', rate: 25 },
        { itemName: 'Grizzly Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Old Stove (ID: 1617) ──── Level 68 | HP 15,895 | NORMAL | formless/neutral1 | aggressive
RO_MONSTER_TEMPLATES['waste_stove'] = {
    id: 1617, name: 'Old Stove', aegisName: 'WASTE_STOVE',
    level: 68, maxHealth: 15895, baseExp: 4412, jobExp: 1135, mvpExp: 0,
    attack: 692, attack2: 1081, defense: 23, magicDefense: 10,
    str: 20, agi: 69, vit: 55, int: 5, dex: 59, luk: 77,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 300, attackDelay: 1152, attackMotion: 528, damageMotion: 360,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28600,
    raceGroups: {},
    stats: { str: 20, agi: 69, vit: 55, int: 5, dex: 59, luk: 77, level: 68, weaponATK: 692 },
    modes: {},
    drops: [
        { itemName: 'Battered Kettle', rate: 10 },
        { itemName: 'Burnt Tree', rate: 10 },
        { itemName: 'Iron', rate: 5 },
        { itemName: 'Rusty Iron', rate: 0.5 },
        { itemName: 'Iron Ore', rate: 10 },
        { itemName: 'Dead Branch', rate: 0.5 },
        { itemName: 'Used Iron Plate', rate: 38 },
        { itemName: 'Waste Stove Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Phreeoni (ID: 1159) ──── Level 69 | HP 188,000 | MVP | brute/neutral3 | aggressive
RO_MONSTER_TEMPLATES['phreeoni'] = {
    id: 1159, name: 'Phreeoni', aegisName: 'PHREEONI',
    level: 69, maxHealth: 188000, baseExp: 32175, jobExp: 16445, mvpExp: 16087,
    attack: 880, attack2: 1530, defense: 10, magicDefense: 20,
    str: 0, agi: 85, vit: 78, int: 35, dex: 130, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 200, attackDelay: 1020, attackMotion: 1020, damageMotion: 288,
    size: 'large', race: 'brute', element: { type: 'neutral', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 85, vit: 78, int: 35, dex: 130, luk: 60, level: 69, weaponATK: 880 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Thin N\' Long Tongue', rate: 97 },
        { itemName: 'Fortune Sword', rate: 5 },
        { itemName: 'Sucsamad', rate: 1.5 },
        { itemName: 'Ant Jaw', rate: 50 },
        { itemName: 'Mr. Scream', rate: 3 },
        { itemName: 'Elunium', rate: 29 },
        { itemName: 'Weihna', rate: 1 },
        { itemName: 'Phreeoni Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Necklace of Oblivion', rate: 5 },
        { itemName: '1carat Diamond', rate: 10 },
    ],
};

// ──── Goat (ID: 1372) ──── Level 69 | HP 11,077 | NORMAL | brute/fire3 | reactive
RO_MONSTER_TEMPLATES['goat'] = {
    id: 1372, name: 'Goat', aegisName: 'GOAT',
    level: 69, maxHealth: 11077, baseExp: 3357, jobExp: 2015, mvpExp: 0,
    attack: 457, attack2: 608, defense: 44, magicDefense: 25,
    str: 0, agi: 58, vit: 66, int: 62, dex: 67, luk: 43,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 172, walkSpeed: 165, attackDelay: 1380, attackMotion: 1080, damageMotion: 336,
    size: 'medium', race: 'brute', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'reactive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 0, agi: 58, vit: 66, int: 62, dex: 67, luk: 43, level: 69, weaponATK: 457 },
    modes: {},
    drops: [
        { itemName: 'Goat\'s Horn', rate: 45.59 },
        { itemName: 'Gaoat\'s Skin', rate: 25 },
        { itemName: 'Empty Bottle', rate: 50 },
        { itemName: 'Red Herb', rate: 5 },
        { itemName: 'Blue Herb', rate: 10 },
        { itemName: 'Yellow Herb', rate: 25 },
        { itemName: 'Green Herb', rate: 55 },
        { itemName: 'Goat Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Shinobi (ID: 1401) ──── Level 69 | HP 12,700 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['shinobi'] = {
    id: 1401, name: 'Shinobi', aegisName: 'SHINOBI',
    level: 69, maxHealth: 12700, baseExp: 4970, jobExp: 3010, mvpExp: 0,
    attack: 460, attack2: 1410, defense: 34, magicDefense: 21,
    str: 85, agi: 85, vit: 25, int: 25, dex: 100, luk: 100,
    attackRange: 100, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 28800,
    raceGroups: { Ninja: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 85, agi: 85, vit: 25, int: 25, dex: 100, luk: 100, level: 69, weaponATK: 460 },
    modes: {},
    drops: [
        { itemName: 'Broken Shuriken', rate: 53.35 },
        { itemName: 'Ninja Suit', rate: 0.02 },
        { itemName: 'Cyfar', rate: 22 },
        { itemName: 'Shinobi\'s Sash', rate: 1 },
        { itemName: 'Thief Clothes', rate: 0.01 },
        { itemName: 'Dark Mask', rate: 20 },
        { itemName: 'Murasame', rate: 0.05 },
        { itemName: 'Shinobi Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ungoliant (ID: 1618) ──── Level 69 | HP 29,140 | NORMAL | insect/poison2 | aggressive
RO_MONSTER_TEMPLATES['ungoliant'] = {
    id: 1618, name: 'Ungoliant', aegisName: 'UNGOLIANT',
    level: 69, maxHealth: 29140, baseExp: 8211, jobExp: 142, mvpExp: 0,
    attack: 1290, attack2: 2280, defense: 25, magicDefense: 25,
    str: 33, agi: 52, vit: 57, int: 25, dex: 119, luk: 43,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 350, attackDelay: 420, attackMotion: 576, damageMotion: 420,
    size: 'large', race: 'insect', element: { type: 'poison', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 33, agi: 52, vit: 57, int: 25, dex: 119, luk: 43, level: 69, weaponATK: 1290 },
    modes: { detector: true },
    drops: [
        { itemName: 'Insect Leg', rate: 45 },
        { itemName: 'Ant Jaw', rate: 35 },
        { itemName: 'Rainbow Shell', rate: 10 },
        { itemName: 'Peridot', rate: 15 },
        { itemName: 'Fluorescent Liquid', rate: 25 },
        { itemName: 'Garnet', rate: 15 },
        { itemName: 'Boots', rate: 5 },
        { itemName: 'Ungoliant Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ferus (ID: 1717) ──── Level 69 | HP 21,182 | NORMAL | dragon/earth2 | aggressive
RO_MONSTER_TEMPLATES['ferus_'] = {
    id: 1717, name: 'Ferus', aegisName: 'FERUS_',
    level: 69, maxHealth: 21182, baseExp: 6750, jobExp: 2230, mvpExp: 0,
    attack: 930, attack2: 1170, defense: 14, magicDefense: 38,
    str: 0, agi: 66, vit: 77, int: 60, dex: 79, luk: 35,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 120, attackDelay: 108, attackMotion: 576, damageMotion: 432,
    size: 'large', race: 'dragon', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 77, int: 60, dex: 79, luk: 35, level: 69, weaponATK: 930 },
    modes: {},
    drops: [
        { itemName: 'Fresh Fish', rate: 51 },
        { itemName: 'Dragon Canine', rate: 10 },
        { itemName: 'Dragon Scale', rate: 35.89 },
        { itemName: 'Green Bijou', rate: 8 },
        { itemName: 'Great Nature', rate: 0.2 },
        { itemName: 'Green Bijou', rate: 1 },
        { itemName: 'Red Ferus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Aliza (ID: 1737) ──── Level 69 | HP 19,000 | NORMAL | demihuman/neutral3 | aggressive
RO_MONSTER_TEMPLATES['aliza'] = {
    id: 1737, name: 'Aliza', aegisName: 'ALIZA',
    level: 69, maxHealth: 19000, baseExp: 6583, jobExp: 3400, mvpExp: 0,
    attack: 750, attack2: 1100, defense: 8, magicDefense: 5,
    str: 74, agi: 74, vit: 52, int: 35, dex: 110, luk: 140,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 220, attackDelay: 1440, attackMotion: 576, damageMotion: 600,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 74, agi: 74, vit: 52, int: 35, dex: 110, luk: 140, level: 69, weaponATK: 750 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 40 },
        { itemName: 'Morpheus\'s Shawl', rate: 0.1 },
        { itemName: 'Rosary', rate: 0.1 },
        { itemName: 'Alice\'s Apron', rate: 0.05 },
        { itemName: 'Royal Cooking Kit', rate: 0.5 },
        { itemName: 'Soft Apron', rate: 0.01 },
        { itemName: 'Orleans\'s Server', rate: 0.05 },
        { itemName: 'Aliza Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Frus (ID: 1753) ──── Level 69 | HP 83,422 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['frus'] = {
    id: 1753, name: 'Frus', aegisName: 'FRUS',
    level: 69, maxHealth: 83422, baseExp: 20620, jobExp: 10, mvpExp: 0,
    attack: 1110, attack2: 1780, defense: 20, magicDefense: 15,
    str: 0, agi: 69, vit: 60, int: 50, dex: 76, luk: 52,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 480, attackMotion: 576, damageMotion: 432,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 0, agi: 69, vit: 60, int: 50, dex: 76, luk: 52, level: 69, weaponATK: 1110 },
    modes: { detector: true },
    drops: [
        { itemName: 'Rune of Darkness', rate: 35 },
        { itemName: 'Brigan', rate: 10 },
        { itemName: 'Red Gemstone', rate: 10 },
        { itemName: 'Earring', rate: 0.03 },
        { itemName: 'Mantle', rate: 0.1 },
        { itemName: 'Rough Elunium', rate: 5 },
        { itemName: 'Frus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Echio (ID: 1770) ──── Level 69 | HP 34,900 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['echio'] = {
    id: 1770, name: 'Echio', aegisName: 'ECHIO',
    level: 69, maxHealth: 34900, baseExp: 13560, jobExp: 4300, mvpExp: 0,
    attack: 750, attack2: 1800, defense: 33, magicDefense: 11,
    str: 74, agi: 74, vit: 52, int: 35, dex: 59, luk: 56,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 250, attackDelay: 768, attackMotion: 360, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 28800,
    raceGroups: {},
    stats: { str: 74, agi: 74, vit: 52, int: 35, dex: 59, luk: 56, level: 69, weaponATK: 750 },
    modes: {},
    drops: [
        { itemName: 'Suspicious Hat', rate: 25 },
        { itemName: 'Yggdrasil Seed', rate: 0.1 },
        { itemName: 'Bloody Rune', rate: 40 },
        { itemName: 'Beret', rate: 0.2 },
        { itemName: 'Holy Arrow Quiver', rate: 0.2 },
        { itemName: 'Bloody Rune', rate: 1 },
        { itemName: 'Divine Cloth', rate: 0.2 },
        { itemName: 'Echio Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Drake (ID: 1112) ──── Level 70 | HP 326,666 | MVP | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['drake'] = {
    id: 1112, name: 'Drake', aegisName: 'DRAKE',
    level: 70, maxHealth: 326666, baseExp: 28600, jobExp: 22880, mvpExp: 14300,
    attack: 1800, attack2: 2100, defense: 20, magicDefense: 35,
    str: 85, agi: 80, vit: 49, int: 75, dex: 79, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 400, attackDelay: 620, attackMotion: 420, damageMotion: 360,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 85, agi: 80, vit: 49, int: 75, dex: 79, luk: 50, level: 70, weaponATK: 1800 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Saber', rate: 6 },
        { itemName: 'Ring Pommel Saber', rate: 9.5 },
        { itemName: 'Cutlus', rate: 1.5 },
        { itemName: 'Haedonggum', rate: 4 },
        { itemName: 'Corsair', rate: 3.5 },
        { itemName: 'Elunium', rate: 32 },
        { itemName: 'Krasnaya', rate: 1 },
        { itemName: 'Drake Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'White Potion', rate: 50 },
    ],
};

// ──── Chimera (ID: 1283) ──── Level 70 | HP 32,600 | BOSS | brute/fire3 | aggressive
RO_MONSTER_TEMPLATES['chimera'] = {
    id: 1283, name: 'Chimera', aegisName: 'CHIMERA',
    level: 70, maxHealth: 32600, baseExp: 4950, jobExp: 3000, mvpExp: 0,
    attack: 1200, attack2: 1320, defense: 30, magicDefense: 10,
    str: 0, agi: 72, vit: 110, int: 88, dex: 75, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 200, attackDelay: 772, attackMotion: 672, damageMotion: 360,
    size: 'large', race: 'brute', element: { type: 'fire', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 72, vit: 110, int: 88, dex: 75, luk: 85, level: 70, weaponATK: 1200 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Horrendous Hair', rate: 25 },
        { itemName: 'Lemon', rate: 10 },
        { itemName: 'War Axe', rate: 0.01 },
        { itemName: 'Citrin', rate: 15 },
        { itemName: 'Great Axe', rate: 0.01 },
        { itemName: 'Oridecon', rate: 1.6 },
        { itemName: 'Chimera Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Solider (ID: 1316) ──── Level 70 | HP 12,099 | NORMAL | brute/earth2 | aggressive
RO_MONSTER_TEMPLATES['solider'] = {
    id: 1316, name: 'Solider', aegisName: 'SOLIDER',
    level: 70, maxHealth: 12099, baseExp: 4458, jobExp: 1951, mvpExp: 0,
    attack: 797, attack2: 979, defense: 57, magicDefense: 43,
    str: 69, agi: 35, vit: 85, int: 5, dex: 74, luk: 100,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 250, attackDelay: 1452, attackMotion: 483, damageMotion: 528,
    size: 'medium', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29000,
    raceGroups: {},
    stats: { str: 69, agi: 35, vit: 85, int: 5, dex: 74, luk: 100, level: 70, weaponATK: 797 },
    modes: {},
    drops: [
        { itemName: 'Turtle Shell', rate: 44.13 },
        { itemName: 'Broken Shell', rate: 0.64 },
        { itemName: 'Stone Fragment', rate: 8.5 },
        { itemName: 'Yellow Herb', rate: 21 },
        { itemName: 'Zargon', rate: 12.4 },
        { itemName: 'Honey', rate: 8.5 },
        { itemName: 'Chain', rate: 0.01 },
        { itemName: 'Solider Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Harpy (ID: 1376) ──── Level 70 | HP 16,599 | NORMAL | demon/wind3 | aggressive
RO_MONSTER_TEMPLATES['harpy'] = {
    id: 1376, name: 'Harpy', aegisName: 'HARPY',
    level: 70, maxHealth: 16599, baseExp: 3562, jobExp: 2133, mvpExp: 0,
    attack: 926, attack2: 1231, defense: 42, magicDefense: 44,
    str: 0, agi: 112, vit: 72, int: 67, dex: 74, luk: 76,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 181, walkSpeed: 155, attackDelay: 972, attackMotion: 672, damageMotion: 470,
    size: 'medium', race: 'demon', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29000,
    raceGroups: {},
    stats: { str: 0, agi: 112, vit: 72, int: 67, dex: 74, luk: 76, level: 70, weaponATK: 926 },
    modes: { detector: true },
    drops: [
        { itemName: 'Harpy\'s Feather', rate: 48.5 },
        { itemName: 'Harpy\'s Claw', rate: 25 },
        { itemName: 'Yellow Herb', rate: 15 },
        { itemName: 'Yellow Herb', rate: 8 },
        { itemName: 'Izidor', rate: 0.2 },
        { itemName: 'Electric Fist', rate: 0.2 },
        { itemName: 'Harpy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Tao Gunka (ID: 1583) ──── Level 70 | HP 193,000 | MVP | demon/neutral3 | aggressive
RO_MONSTER_TEMPLATES['tao_gunka'] = {
    id: 1583, name: 'Tao Gunka', aegisName: 'TAO_GUNKA',
    level: 70, maxHealth: 193000, baseExp: 59175, jobExp: 10445, mvpExp: 29587,
    attack: 1450, attack2: 1770, defense: 20, magicDefense: 20,
    str: 0, agi: 85, vit: 78, int: 35, dex: 140, luk: 60,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 150, attackDelay: 1020, attackMotion: 288, damageMotion: 144,
    size: 'large', race: 'demon', element: { type: 'neutral', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 85, vit: 78, int: 35, dex: 140, luk: 60, level: 70, weaponATK: 1450 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Gemstone', rate: 48.5 },
        { itemName: 'Stone Fragment', rate: 48.5 },
        { itemName: 'Topaz', rate: 10 },
        { itemName: 'Binoculars', rate: 4 },
        { itemName: 'White Potion', rate: 30 },
        { itemName: 'Iron Ore', rate: 10 },
        { itemName: 'Gemmed Sallet', rate: 0.05 },
        { itemName: 'Tao Gunka Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Oridecon', rate: 60 },
        { itemName: 'Old Purple Box', rate: 30 },
    ],
};

// ──── Ferus (ID: 1714) ──── Level 70 | HP 29,218 | NORMAL | dragon/fire2 | aggressive
RO_MONSTER_TEMPLATES['ferus'] = {
    id: 1714, name: 'Ferus', aegisName: 'FERUS',
    level: 70, maxHealth: 29218, baseExp: 8093, jobExp: 3952, mvpExp: 0,
    attack: 1056, attack2: 1496, defense: 14, magicDefense: 45,
    str: 0, agi: 78, vit: 45, int: 72, dex: 81, luk: 73,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 108, attackMotion: 576, damageMotion: 432,
    size: 'large', race: 'dragon', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29000,
    raceGroups: {},
    stats: { str: 0, agi: 78, vit: 45, int: 72, dex: 81, luk: 73, level: 70, weaponATK: 1056 },
    modes: {},
    drops: [
        { itemName: 'Strawberry', rate: 22 },
        { itemName: 'Dragon Canine', rate: 10 },
        { itemName: 'Dragon\'s Skin', rate: 10 },
        { itemName: 'Dragon Scale', rate: 20 },
        { itemName: 'Red Bijou', rate: 8 },
        { itemName: 'Flame Heart', rate: 0.2 },
        { itemName: 'Magni\'s Cap', rate: 0.5 },
        { itemName: 'Red Ferus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skogul (ID: 1752) ──── Level 70 | HP 87,544 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['skogul'] = {
    id: 1752, name: 'Skogul', aegisName: 'SKOGUL',
    level: 70, maxHealth: 87544, baseExp: 27620, jobExp: 10, mvpExp: 0,
    attack: 1110, attack2: 1930, defense: 20, magicDefense: 15,
    str: 0, agi: 69, vit: 70, int: 50, dex: 67, luk: 52,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 190, attackDelay: 720, attackMotion: 384, damageMotion: 480,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29000,
    raceGroups: {},
    stats: { str: 0, agi: 69, vit: 70, int: 50, dex: 67, luk: 52, level: 70, weaponATK: 1110 },
    modes: { detector: true },
    drops: [
        { itemName: 'Rune of Darkness', rate: 35 },
        { itemName: 'Brigan', rate: 10 },
        { itemName: 'Red Gemstone', rate: 10 },
        { itemName: 'Rouge', rate: 5 },
        { itemName: 'Skull Ring', rate: 1 },
        { itemName: 'Rough Elunium', rate: 5 },
        { itemName: 'Blood Tears', rate: 0.05 },
        { itemName: 'Skogul Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Rybio (ID: 1201) ──── Level 71 | HP 9,572 | NORMAL | demon/neutral2 | aggressive
RO_MONSTER_TEMPLATES['rybio'] = {
    id: 1201, name: 'Rybio', aegisName: 'RYBIO',
    level: 71, maxHealth: 9572, baseExp: 6317, jobExp: 3520, mvpExp: 0,
    attack: 686, attack2: 912, defense: 45, magicDefense: 37,
    str: 0, agi: 97, vit: 75, int: 74, dex: 77, luk: 90,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 200, attackDelay: 1790, attackMotion: 1440, damageMotion: 540,
    size: 'large', race: 'demon', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29200,
    raceGroups: {},
    stats: { str: 0, agi: 97, vit: 75, int: 74, dex: 77, luk: 90, level: 71, weaponATK: 686 },
    modes: { detector: true },
    drops: [
        { itemName: 'Thin N\' Long Tongue', rate: 38.8 },
        { itemName: 'Executioner\'s Mitten', rate: 0.03 },
        { itemName: 'White Herb', rate: 18 },
        { itemName: '1carat Diamond', rate: 0.3 },
        { itemName: 'Necklace of Oblivion', rate: 0.1 },
        { itemName: 'Oridecon', rate: 1 },
        { itemName: 'Izidor', rate: 0.3 },
        { itemName: 'Rybio Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Ogretooth (ID: 1204) ──── Level 71 | HP 29,900 | BOSS | formless/shadow3 | aggressive
RO_MONSTER_TEMPLATES['tirfing'] = {
    id: 1204, name: 'Ogretooth', aegisName: 'TIRFING',
    level: 71, maxHealth: 29900, baseExp: 5412, jobExp: 4235, mvpExp: 0,
    attack: 950, attack2: 1146, defense: 30, magicDefense: 35,
    str: 58, agi: 87, vit: 55, int: 35, dex: 132, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 100, attackDelay: 816, attackMotion: 500, damageMotion: 240,
    size: 'medium', race: 'formless', element: { type: 'shadow', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 58, agi: 87, vit: 55, int: 35, dex: 132, luk: 65, level: 71, weaponATK: 950 },
    modes: {},
    drops: [
        { itemName: 'Old Hilt', rate: 0.01 },
        { itemName: 'Silver Knife of Chastity', rate: 0.5 },
        { itemName: 'Muscovite', rate: 15 },
        { itemName: 'Dagger', rate: 0.7 },
        { itemName: 'Stiletto', rate: 0.4 },
        { itemName: 'Steel', rate: 1.2 },
        { itemName: 'Oridecon', rate: 1.89 },
        { itemName: 'Ogretooth Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Assaulter (ID: 1315) ──── Level 71 | HP 11,170 | NORMAL | demihuman/neutral1 | passive
RO_MONSTER_TEMPLATES['assulter'] = {
    id: 1315, name: 'Assaulter', aegisName: 'ASSULTER',
    level: 71, maxHealth: 11170, baseExp: 4854, jobExp: 2654, mvpExp: 0,
    attack: 764, attack2: 1499, defense: 35, magicDefense: 28,
    str: 85, agi: 74, vit: 10, int: 35, dex: 100, luk: 100,
    attackRange: 100, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 29200,
    raceGroups: { Ninja: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 85, agi: 74, vit: 10, int: 35, dex: 100, luk: 100, level: 71, weaponATK: 764 },
    modes: {},
    drops: [
        { itemName: 'Turtle Shell', rate: 44.13 },
        { itemName: 'Destroyed Armor', rate: 12 },
        { itemName: 'Old Shuriken', rate: 8.4 },
        { itemName: 'Yellow Herb', rate: 12.8 },
        { itemName: 'Zargon', rate: 12.4 },
        { itemName: 'Huuma Wing Shuriken', rate: 0.05 },
        { itemName: 'Old Blue Box', rate: 0.01 },
        { itemName: 'Assaulter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Samurai Specter (ID: 1492) ──── Level 71 | HP 218,652 | MVP | demihuman/shadow3 | aggressive
RO_MONSTER_TEMPLATES['incantation_samurai'] = {
    id: 1492, name: 'Samurai Specter', aegisName: 'INCANTATION_SAMURAI',
    level: 71, maxHealth: 218652, baseExp: 33095, jobExp: 18214, mvpExp: 16547,
    attack: 2219, attack2: 3169, defense: 10, magicDefense: 51,
    str: 91, agi: 85, vit: 30, int: 85, dex: 150, luk: 60,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 135, attackDelay: 874, attackMotion: 1344, damageMotion: 576,
    size: 'large', race: 'demihuman', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 91, agi: 85, vit: 30, int: 85, dex: 150, luk: 60, level: 71, weaponATK: 2219 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Masamune', rate: 0.02 },
        { itemName: 'Elunium', rate: 35 },
        { itemName: 'Assassin Mask', rate: 5 },
        { itemName: 'Yggdrasil Berry', rate: 45 },
        { itemName: 'Steel', rate: 63.05 },
        { itemName: 'Huuma Blaze Shuriken', rate: 75 },
        { itemName: 'Azoth', rate: 0.8 },
        { itemName: 'Samurai Spector Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Yggdrasil Seed', rate: 35 },
    ],
};

// ──── Loli Ruri (ID: 1505) ──── Level 71 | HP 23,470 | NORMAL | demon/shadow4 | aggressive
RO_MONSTER_TEMPLATES['loli_ruri'] = {
    id: 1505, name: 'Loli Ruri', aegisName: 'LOLI_RURI',
    level: 71, maxHealth: 23470, baseExp: 6641, jobExp: 4314, mvpExp: 0,
    attack: 1476, attack2: 2317, defense: 39, magicDefense: 44,
    str: 0, agi: 66, vit: 54, int: 74, dex: 81, luk: 43,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 125, attackDelay: 747, attackMotion: 1632, damageMotion: 576,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29200,
    raceGroups: {},
    stats: { str: 0, agi: 66, vit: 54, int: 74, dex: 81, luk: 43, level: 71, weaponATK: 1476 },
    modes: { detector: true },
    drops: [
        { itemName: 'Black Cat Doll', rate: 8 },
        { itemName: 'Striped Sock', rate: 30 },
        { itemName: 'Bat Cage', rate: 50.44 },
        { itemName: 'Elunium', rate: 1 },
        { itemName: 'Loki\'s Whispers', rate: 0.01 },
        { itemName: 'Lunatic Brooch', rate: 0.05 },
        { itemName: 'Loli Ruri Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Teddy Bear (ID: 1622) ──── Level 71 | HP 8,109 | NORMAL | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['teddy_bear'] = {
    id: 1622, name: 'Teddy Bear', aegisName: 'TEDDY_BEAR',
    level: 71, maxHealth: 8109, baseExp: 5891, jobExp: 3455, mvpExp: 0,
    attack: 621, attack2: 1432, defense: 19, magicDefense: 32,
    str: 5, agi: 155, vit: 32, int: 41, dex: 121, luk: 26,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 512, attackMotion: 780, damageMotion: 504,
    size: 'small', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29200,
    raceGroups: {},
    stats: { str: 5, agi: 155, vit: 32, int: 41, dex: 121, luk: 26, level: 71, weaponATK: 621 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 38 },
        { itemName: 'Honey', rate: 10 },
        { itemName: 'Oridecon Hammer', rate: 3 },
        { itemName: 'Gold Lux', rate: 0.05 },
        { itemName: 'Angry Mouth', rate: 0.5 },
        { itemName: 'Goddess of Fortune\'s Cursed Brooch', rate: 0.1 },
        { itemName: 'Elunium', rate: 1 },
        { itemName: 'Teddy Bear Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Doppelganger (ID: 1046) ──── Level 72 | HP 249,000 | MVP | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['doppelganger'] = {
    id: 1046, name: 'Doppelganger', aegisName: 'DOPPELGANGER',
    level: 72, maxHealth: 249000, baseExp: 51480, jobExp: 10725, mvpExp: 25740,
    attack: 1340, attack2: 1590, defense: 60, magicDefense: 35,
    str: 88, agi: 90, vit: 30, int: 35, dex: 125, luk: 65,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 480, attackMotion: 480, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 88, agi: 90, vit: 30, int: 35, dex: 125, luk: 65, level: 72, weaponATK: 1340 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Full Plate', rate: 2.5 },
        { itemName: 'Broad Sword', rate: 2.2 },
        { itemName: 'Zweihander', rate: 1.5 },
        { itemName: 'Spiky Band', rate: 3.5 },
        { itemName: 'Lance', rate: 5.5 },
        { itemName: 'Elunium', rate: 36.86 },
        { itemName: 'Oridecon', rate: 27 },
        { itemName: 'Doppelganger Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Ruby', rate: 15 },
    ],
};

// ──── Gryphon (ID: 1259) ──── Level 72 | HP 27,800 | BOSS | brute/wind4 | aggressive
RO_MONSTER_TEMPLATES['gryphon'] = {
    id: 1259, name: 'Gryphon', aegisName: 'GRYPHON',
    level: 72, maxHealth: 27800, baseExp: 5896, jobExp: 4400, mvpExp: 0,
    attack: 880, attack2: 1260, defense: 35, magicDefense: 35,
    str: 68, agi: 95, vit: 78, int: 65, dex: 115, luk: 75,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 186, walkSpeed: 100, attackDelay: 704, attackMotion: 504, damageMotion: 432,
    size: 'large', race: 'brute', element: { type: 'wind', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 68, agi: 95, vit: 78, int: 65, dex: 115, luk: 75, level: 72, weaponATK: 880 },
    modes: {},
    drops: [
        { itemName: 'Talon of Griffon', rate: 25 },
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Soft Feather', rate: 1.2 },
        { itemName: 'Guisarme', rate: 15 },
        { itemName: 'Pole Axe', rate: 0.01 },
        { itemName: 'Oridecon', rate: 1.85 },
        { itemName: 'Rough Wind', rate: 1.5 },
        { itemName: 'Gryphon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Freezer (ID: 1319) ──── Level 72 | HP 8,636 | NORMAL | brute/water2 | aggressive
RO_MONSTER_TEMPLATES['freezer'] = {
    id: 1319, name: 'Freezer', aegisName: 'FREEZER',
    level: 72, maxHealth: 8636, baseExp: 3665, jobExp: 2197, mvpExp: 0,
    attack: 671, attack2: 983, defense: 55, magicDefense: 43,
    str: 69, agi: 41, vit: 59, int: 5, dex: 67, luk: 100,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 250, attackDelay: 1452, attackMotion: 483, damageMotion: 528,
    size: 'medium', race: 'brute', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29400,
    raceGroups: {},
    stats: { str: 69, agi: 41, vit: 59, int: 5, dex: 67, luk: 100, level: 72, weaponATK: 671 },
    modes: {},
    drops: [
        { itemName: 'Turtle Shell', rate: 44.13 },
        { itemName: 'Broken Shell', rate: 8.5 },
        { itemName: 'Ice Piece', rate: 12.5 },
        { itemName: 'Zargon', rate: 18 },
        { itemName: 'Royal Jelly', rate: 1.6 },
        { itemName: 'Cyfar', rate: 6 },
        { itemName: 'Level 5 Cold Bolt', rate: 1 },
        { itemName: 'Freezer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bloody Murderer (ID: 1507) ──── Level 72 | HP 27,521 | NORMAL | demihuman/shadow3 | aggressive
RO_MONSTER_TEMPLATES['bloody_murderer'] = {
    id: 1507, name: 'Bloody Murderer', aegisName: 'BLOODY_MURDERER',
    level: 72, maxHealth: 27521, baseExp: 9742, jobExp: 3559, mvpExp: 0,
    attack: 864, attack2: 1081, defense: 37, magicDefense: 41,
    str: 0, agi: 30, vit: 90, int: 15, dex: 52, luk: 12,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 175, attackDelay: 914, attackMotion: 1344, damageMotion: 384,
    size: 'large', race: 'demihuman', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29400,
    raceGroups: {},
    stats: { str: 0, agi: 30, vit: 90, int: 15, dex: 52, luk: 12, level: 72, weaponATK: 864 },
    modes: {},
    drops: [
        { itemName: 'Old Manteau', rate: 41.71 },
        { itemName: 'Contorted Self-Portrait', rate: 10 },
        { itemName: 'Rusty Kitchen Knife', rate: 20 },
        { itemName: 'Mr. Scream', rate: 0.5 },
        { itemName: 'Oridecon', rate: 1 },
        { itemName: 'Mama\'s Knife', rate: 0.03 },
        { itemName: 'Ginnungagap', rate: 0.01 },
        { itemName: 'Bloody Murderer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── White Lady (ID: 1518) ──── Level 72 | HP 56,380 | NORMAL | demihuman/water2 | aggressive
RO_MONSTER_TEMPLATES['bacsojin'] = {
    id: 1518, name: 'White Lady', aegisName: 'BACSOJIN',
    level: 72, maxHealth: 56380, baseExp: 5590, jobExp: 1659, mvpExp: 0,
    attack: 560, attack2: 1446, defense: 10, magicDefense: 15,
    str: 38, agi: 65, vit: 34, int: 80, dex: 102, luk: 35,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 160, attackDelay: 576, attackMotion: 960, damageMotion: 480,
    size: 'large', race: 'demihuman', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29400,
    raceGroups: {},
    stats: { str: 38, agi: 65, vit: 34, int: 80, dex: 102, luk: 35, level: 72, weaponATK: 560 },
    modes: {},
    drops: [
        { itemName: 'Black Hair', rate: 55 },
        { itemName: 'Old Blue Box', rate: 0.02 },
        { itemName: 'Old Purple Box', rate: 0.02 },
        { itemName: 'Transparent Celestial Robe', rate: 30 },
        { itemName: 'Soft Silk', rate: 10 },
        { itemName: 'Crystal Mirror', rate: 5 },
        { itemName: 'Tiara', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venatu (ID: 1676) ──── Level 72 | HP 8,900 | NORMAL | formless/neutral2 | aggressive
RO_MONSTER_TEMPLATES['venatu_1'] = {
    id: 1676, name: 'Venatu', aegisName: 'VENATU_1',
    level: 72, maxHealth: 8900, baseExp: 4000, jobExp: 2000, mvpExp: 0,
    attack: 800, attack2: 1400, defense: 30, magicDefense: 20,
    str: 5, agi: 26, vit: 24, int: 5, dex: 82, luk: 30,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 504, attackMotion: 1020, damageMotion: 360,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29400,
    raceGroups: {},
    stats: { str: 5, agi: 26, vit: 24, int: 5, dex: 82, luk: 30, level: 72, weaponATK: 800 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 20 },
        { itemName: 'Crest Piece', rate: 3.5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Drifter', rate: 0.05 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Professional Cooking Kit', rate: 1 },
        { itemName: 'Venatu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gemini-S58 (ID: 1681) ──── Level 72 | HP 57,870 | BOSS | formless/water1 | aggressive
RO_MONSTER_TEMPLATES['gemini'] = {
    id: 1681, name: 'Gemini-S58', aegisName: 'GEMINI',
    level: 72, maxHealth: 57870, baseExp: 22024, jobExp: 9442, mvpExp: 0,
    attack: 2150, attack2: 3030, defense: 60, magicDefense: 45,
    str: 88, agi: 75, vit: 70, int: 77, dex: 105, luk: 55,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 163, walkSpeed: 200, attackDelay: 1872, attackMotion: 360, damageMotion: 864,
    size: 'medium', race: 'formless', element: { type: 'water', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 88, agi: 75, vit: 70, int: 77, dex: 105, luk: 55, level: 72, weaponATK: 2150 },
    modes: {},
    drops: [
        { itemName: 'Skull', rate: 30 },
        { itemName: 'Old Blue Box', rate: 10 },
        { itemName: 'Butcher', rate: 0.05 },
        { itemName: 'Condensed Yellow Potion', rate: 5 },
        { itemName: 'Condensed White Potion', rate: 4 },
        { itemName: 'Level 8 Cookbook', rate: 0.06 },
        { itemName: 'Stone of Sage', rate: 3 },
        { itemName: 'Gemini-S58 Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Phendark (ID: 1202) ──── Level 73 | HP 22,729 | NORMAL | demihuman/neutral2 | aggressive
RO_MONSTER_TEMPLATES['phendark'] = {
    id: 1202, name: 'Phendark', aegisName: 'PHENDARK',
    level: 73, maxHealth: 22729, baseExp: 6826, jobExp: 3443, mvpExp: 0,
    attack: 794, attack2: 1056, defense: 52, magicDefense: 36,
    str: 0, agi: 62, vit: 120, int: 65, dex: 76, luk: 66,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 165, walkSpeed: 175, attackDelay: 1744, attackMotion: 1344, damageMotion: 600,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29600,
    raceGroups: {},
    stats: { str: 0, agi: 62, vit: 120, int: 65, dex: 76, luk: 66, level: 73, weaponATK: 794 },
    modes: {},
    drops: [
        { itemName: 'Thin N\' Long Tongue', rate: 38.8 },
        { itemName: 'Executioner\'s Mitten', rate: 0.04 },
        { itemName: 'White Herb', rate: 18 },
        { itemName: 'Oridecon', rate: 1.5 },
        { itemName: 'Electric Wire', rate: 1 },
        { itemName: 'Phendark Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Hatii (ID: 1252) ──── Level 73 | HP 197,000 | MVP | brute/water4 | aggressive
RO_MONSTER_TEMPLATES['garm'] = {
    id: 1252, name: 'Hatii', aegisName: 'GARM',
    level: 73, maxHealth: 197000, baseExp: 50050, jobExp: 20020, mvpExp: 25025,
    attack: 1700, attack2: 1900, defense: 40, magicDefense: 45,
    str: 85, agi: 126, vit: 82, int: 65, dex: 95, luk: 60,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 400, attackDelay: 608, attackMotion: 408, damageMotion: 336,
    size: 'large', race: 'brute', element: { type: 'water', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 85, agi: 126, vit: 82, int: 65, dex: 95, luk: 60, level: 73, weaponATK: 1700 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Fang of Hatii', rate: 55 },
        { itemName: 'Ice Falchion', rate: 1.5 },
        { itemName: 'Katar of Frozen Icicle', rate: 5 },
        { itemName: 'Hatii Claw', rate: 5 },
        { itemName: 'Elunium', rate: 39.77 },
        { itemName: 'Oridecon', rate: 29 },
        { itemName: 'Hatii Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Fang of Hatii', rate: 10 },
        { itemName: 'Old Blue Box', rate: 30 },
    ],
};

// ──── Skeleton General (ID: 1290) ──── Level 73 | HP 17,402 | NORMAL | undead/undead1 | aggressive
RO_MONSTER_TEMPLATES['skeleton_general'] = {
    id: 1290, name: 'Skeleton General', aegisName: 'SKELETON_GENERAL',
    level: 73, maxHealth: 17402, baseExp: 8170, jobExp: 3370, mvpExp: 0,
    attack: 910, attack2: 1089, defense: 25, magicDefense: 25,
    str: 90, agi: 25, vit: 40, int: 20, dex: 77, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 154, walkSpeed: 150, attackDelay: 2276, attackMotion: 576, damageMotion: 432,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29600,
    raceGroups: {},
    stats: { str: 90, agi: 25, vit: 40, int: 20, dex: 77, luk: 25, level: 73, weaponATK: 910 },
    modes: {},
    drops: [
        { itemName: 'Burnt Tree', rate: 25.5 },
        { itemName: 'Rough Oridecon', rate: 1.6 },
        { itemName: 'Yellow Herb', rate: 8 },
        { itemName: 'Gladius', rate: 0.35 },
        { itemName: 'Gladius', rate: 0.8 },
        { itemName: 'Sandstorm', rate: 0.15 },
        { itemName: 'Ghost Bandana', rate: 0.01 },
        { itemName: 'Skeleton General Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Evil Snake Lord (ID: 1418) ──── Level 73 | HP 254,993 | MVP | brute/ghost3 | aggressive
RO_MONSTER_TEMPLATES['dark_snake_lord'] = {
    id: 1418, name: 'Evil Snake Lord', aegisName: 'DARK_SNAKE_LORD',
    level: 73, maxHealth: 254993, baseExp: 34288, jobExp: 17950, mvpExp: 17144,
    attack: 2433, attack2: 4210, defense: 25, magicDefense: 55,
    str: 70, agi: 83, vit: 30, int: 80, dex: 164, luk: 88,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 200, attackDelay: 588, attackMotion: 816, damageMotion: 420,
    size: 'large', race: 'brute', element: { type: 'ghost', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 70, agi: 83, vit: 30, int: 80, dex: 164, luk: 88, level: 73, weaponATK: 2433 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Ba Gua', rate: 58.2 },
        { itemName: 'Grave Keeper\'s Sword', rate: 51 },
        { itemName: 'Hellfire', rate: 0.8 },
        { itemName: 'Ph.D Hat', rate: 0.8 },
        { itemName: 'Gae Bolg', rate: 5 },
        { itemName: 'Pill', rate: 9 },
        { itemName: 'Soft Apron', rate: 20 },
        { itemName: 'Evil Snake Lord Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Old Purple Box', rate: 50 },
    ],
};

// ──── Agav (ID: 1769) ──── Level 73 | HP 29,620 | NORMAL | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['agav'] = {
    id: 1769, name: 'Agav', aegisName: 'AGAV',
    level: 73, maxHealth: 29620, baseExp: 9780, jobExp: 6622, mvpExp: 0,
    attack: 103, attack2: 1109, defense: 15, magicDefense: 35,
    str: 0, agi: 32, vit: 27, int: 132, dex: 69, luk: 15,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 300, attackDelay: 768, attackMotion: 360, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29600,
    raceGroups: {},
    stats: { str: 0, agi: 32, vit: 27, int: 132, dex: 69, luk: 15, level: 73, weaponATK: 103 },
    modes: {},
    drops: [
        { itemName: 'Suspicious Hat', rate: 25 },
        { itemName: 'High Fashion Sandals', rate: 0.02 },
        { itemName: 'Bloody Rune', rate: 40 },
        { itemName: 'Memory Book', rate: 0.01 },
        { itemName: 'Holy Arrow Quiver', rate: 0.5 },
        { itemName: 'Bloody Rune', rate: 1 },
        { itemName: 'Agav Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mistress (ID: 1059) ──── Level 74 | HP 212,000 | MVP | insect/wind4 | aggressive
RO_MONSTER_TEMPLATES['mistress'] = {
    id: 1059, name: 'Mistress', aegisName: 'MISTRESS',
    level: 74, maxHealth: 212000, baseExp: 39325, jobExp: 27170, mvpExp: 19662,
    attack: 880, attack2: 1110, defense: 40, magicDefense: 60,
    str: 50, agi: 165, vit: 60, int: 95, dex: 70, luk: 130,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 100, attackDelay: 1148, attackMotion: 648, damageMotion: 300,
    size: 'small', race: 'insect', element: { type: 'wind', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 50, agi: 165, vit: 60, int: 95, dex: 70, luk: 130, level: 74, weaponATK: 880 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Gungnir', rate: 1.5 },
        { itemName: 'Honey', rate: 100 },
        { itemName: 'Coronet', rate: 2.5 },
        { itemName: 'Old Card Album', rate: 10 },
        { itemName: 'Young Twig', rate: 0.1 },
        { itemName: 'Elunium', rate: 42.68 },
        { itemName: 'Red Square Bag', rate: 1 },
        { itemName: 'Mistress Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Rough Wind', rate: 15 },
        { itemName: 'Royal Jelly', rate: 40 },
    ],
};

// ──── Orc Lord (ID: 1190) ──── Level 74 | HP 783,000 | MVP | demihuman/earth4 | aggressive
RO_MONSTER_TEMPLATES['orc_lord'] = {
    id: 1190, name: 'Orc Lord', aegisName: 'ORC_LORD',
    level: 74, maxHealth: 783000, baseExp: 62205, jobExp: 8580, mvpExp: 31102,
    attack: 3700, attack2: 4150, defense: 40, magicDefense: 5,
    str: 85, agi: 82, vit: 30, int: 70, dex: 110, luk: 85,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 100, attackDelay: 1248, attackMotion: 500, damageMotion: 360,
    size: 'large', race: 'demihuman', element: { type: 'earth', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 85, agi: 82, vit: 30, int: 70, dex: 110, luk: 85, level: 74, weaponATK: 3700 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Bloody Axe', rate: 4 },
        { itemName: 'Ring', rate: 4 },
        { itemName: 'Grand Circlet', rate: 4 },
        { itemName: 'Doom Slayer', rate: 4 },
        { itemName: 'Old Purple Box', rate: 10 },
        { itemName: 'Elunium', rate: 42.68 },
        { itemName: 'Erde', rate: 31 },
        { itemName: 'Orc Lord Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Heroic Emblem', rate: 55 },
    ],
};

// ──── Wanderer (ID: 1208) ──── Level 74 | HP 8,170 | NORMAL | demon/wind1 | aggressive
RO_MONSTER_TEMPLATES['wander_man'] = {
    id: 1208, name: 'Wanderer', aegisName: 'WANDER_MAN',
    level: 74, maxHealth: 8170, baseExp: 5786, jobExp: 4730, mvpExp: 0,
    attack: 450, attack2: 1170, defense: 5, magicDefense: 5,
    str: 0, agi: 192, vit: 38, int: 45, dex: 127, luk: 85,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 100, attackDelay: 672, attackMotion: 500, damageMotion: 192,
    size: 'medium', race: 'demon', element: { type: 'wind', level: 1 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29800,
    raceGroups: {},
    stats: { str: 0, agi: 192, vit: 38, int: 45, dex: 127, luk: 85, level: 74, weaponATK: 450 },
    modes: { detector: true },
    drops: [
        { itemName: 'Skull', rate: 48.5 },
        { itemName: 'Old Card Album', rate: 0.01 },
        { itemName: 'Hakujin', rate: 0.05 },
        { itemName: 'Romantic Leaf', rate: 0.05 },
        { itemName: 'Yggdrasil Leaf', rate: 6.5 },
        { itemName: 'Oridecon', rate: 2.17 },
        { itemName: 'Muramasa', rate: 0.01 },
        { itemName: 'Wanderer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Wraith Dead (ID: 1291) ──── Level 74 | HP 43,021 | NORMAL | undead/undead4 | aggressive
RO_MONSTER_TEMPLATES['wraith_dead'] = {
    id: 1291, name: 'Wraith Dead', aegisName: 'WRAITH_DEAD',
    level: 74, maxHealth: 43021, baseExp: 10341, jobExp: 3618, mvpExp: 0,
    attack: 1366, attack2: 1626, defense: 25, magicDefense: 30,
    str: 5, agi: 99, vit: 55, int: 75, dex: 115, luk: 45,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 164, walkSpeed: 175, attackDelay: 1816, attackMotion: 576, damageMotion: 240,
    size: 'large', race: 'undead', element: { type: 'undead', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 29800,
    raceGroups: {},
    stats: { str: 5, agi: 99, vit: 55, int: 75, dex: 115, luk: 45, level: 74, weaponATK: 1366 },
    modes: {},
    drops: [
        { itemName: 'Fabric', rate: 44.13 },
        { itemName: 'Wedding Veil', rate: 0.1 },
        { itemName: 'Manteau', rate: 0.08 },
        { itemName: 'Red Gemstone', rate: 7 },
        { itemName: '1carat Diamond', rate: 0.05 },
        { itemName: 'Old Blue Box', rate: 1 },
        { itemName: 'Lemon', rate: 3 },
        { itemName: 'Wraith Dead Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Anubis (ID: 1098) ──── Level 75 | HP 38,000 | NORMAL | demihuman/undead2 | aggressive
RO_MONSTER_TEMPLATES['anubis'] = {
    id: 1098, name: 'Anubis', aegisName: 'ANUBIS',
    level: 75, maxHealth: 38000, baseExp: 28000, jobExp: 22000, mvpExp: 0,
    attack: 530, attack2: 1697, defense: 25, magicDefense: 31,
    str: 5, agi: 65, vit: 10, int: 82, dex: 77, luk: 33,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 175, walkSpeed: 150, attackDelay: 1250, attackMotion: 768, damageMotion: 360,
    size: 'large', race: 'demihuman', element: { type: 'undead', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 5, agi: 65, vit: 10, int: 82, dex: 77, luk: 33, level: 75, weaponATK: 530 },
    modes: {},
    drops: [
        { itemName: 'Rotten Bandage', rate: 30 },
        { itemName: 'Healing Staff', rate: 0.1 },
        { itemName: 'Memento', rate: 5.5 },
        { itemName: 'Oridecon', rate: 1.05 },
        { itemName: 'Cultish Masque', rate: 43.65 },
        { itemName: 'Celebrant\'s Mitten', rate: 0.01 },
        { itemName: 'Wand of Occult', rate: 0.03 },
        { itemName: 'Anubis Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Owl Baron (ID: 1295) ──── Level 75 | HP 60,746 | BOSS | demon/neutral3 | aggressive
RO_MONSTER_TEMPLATES['owl_baron'] = {
    id: 1295, name: 'Owl Baron', aegisName: 'OWL_BARON',
    level: 75, maxHealth: 60746, baseExp: 10967, jobExp: 4811, mvpExp: 0,
    attack: 1252, attack2: 1610, defense: 65, magicDefense: 25,
    str: 25, agi: 25, vit: 80, int: 95, dex: 95, luk: 55,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 175, attackDelay: 1345, attackMotion: 824, damageMotion: 440,
    size: 'large', race: 'demon', element: { type: 'neutral', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 25, agi: 25, vit: 80, int: 95, dex: 95, luk: 55, level: 75, weaponATK: 1252 },
    modes: {},
    drops: [
        { itemName: 'Tattered Clothes', rate: 35 },
        { itemName: 'Soft Feather', rate: 25 },
        { itemName: 'Gakkung Bow', rate: 0.02 },
        { itemName: 'Soul Staff', rate: 0.01 },
        { itemName: 'Gentleman\'s Staff', rate: 0.02 },
        { itemName: 'Level 5 Lightening Bolt', rate: 1 },
        { itemName: 'Magician Hat', rate: 0.05 },
        { itemName: 'Owl Baron Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Owl Duke (ID: 1320) ──── Level 75 | HP 26,623 | BOSS | demon/neutral3 | aggressive
RO_MONSTER_TEMPLATES['owl_duke'] = {
    id: 1320, name: 'Owl Duke', aegisName: 'OWL_DUKE',
    level: 75, maxHealth: 26623, baseExp: 7217, jobExp: 3474, mvpExp: 0,
    attack: 715, attack2: 910, defense: 27, magicDefense: 49,
    str: 15, agi: 45, vit: 40, int: 75, dex: 79, luk: 88,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 195, attackDelay: 1345, attackMotion: 824, damageMotion: 440,
    size: 'large', race: 'demon', element: { type: 'neutral', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 15, agi: 45, vit: 40, int: 75, dex: 79, luk: 88, level: 75, weaponATK: 715 },
    modes: {},
    drops: [
        { itemName: 'Tattered Clothes', rate: 44.13 },
        { itemName: 'Soft Feather', rate: 15 },
        { itemName: 'Level 5 Lightening Bolt', rate: 1 },
        { itemName: 'Crystal Mirror', rate: 0.01 },
        { itemName: 'Guisarme', rate: 0.03 },
        { itemName: 'Morning Star', rate: 0.02 },
        { itemName: 'Magician Hat', rate: 0.01 },
        { itemName: 'Owl Duke Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Incubus (ID: 1374) ──── Level 75 | HP 17,281 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['incubus'] = {
    id: 1374, name: 'Incubus', aegisName: 'INCUBUS',
    level: 75, maxHealth: 17281, baseExp: 5254, jobExp: 4212, mvpExp: 0,
    attack: 1408, attack2: 1873, defense: 58, magicDefense: 46,
    str: 0, agi: 97, vit: 95, int: 103, dex: 89, luk: 87,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 165, attackDelay: 850, attackMotion: 600, damageMotion: 336,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 0, agi: 97, vit: 95, int: 103, dex: 89, luk: 87, level: 75, weaponATK: 1408 },
    modes: { detector: true },
    drops: [
        { itemName: 'Mastela Fruit', rate: 15 },
        { itemName: 'White Herb', rate: 55 },
        { itemName: 'Incubus Horn', rate: 0.01 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Gold Ring', rate: 5 },
        { itemName: 'Diamond Ring', rate: 1.5 },
        { itemName: 'White Herb', rate: 22 },
        { itemName: 'Incubus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Violy (ID: 1390) ──── Level 75 | HP 18,257 | NORMAL | demihuman/neutral2 | aggressive
RO_MONSTER_TEMPLATES['violy'] = {
    id: 1390, name: 'Violy', aegisName: 'VIOLY',
    level: 75, maxHealth: 18257, baseExp: 6353, jobExp: 3529, mvpExp: 0,
    attack: 738, attack2: 982, defense: 37, magicDefense: 36,
    str: 0, agi: 93, vit: 54, int: 58, dex: 101, luk: 83,
    attackRange: 500, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 170, attackDelay: 1356, attackMotion: 1056, damageMotion: 540,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 0, agi: 93, vit: 54, int: 58, dex: 101, luk: 83, level: 75, weaponATK: 738 },
    modes: {},
    drops: [
        { itemName: 'Golden Hair', rate: 63.05 },
        { itemName: 'Professional Cooking Kit', rate: 0.5 },
        { itemName: 'Puppet', rate: 12 },
        { itemName: 'Bass Guitar', rate: 0.5 },
        { itemName: 'Royal Jelly', rate: 14 },
        { itemName: 'Cursed Water', rate: 10 },
        { itemName: 'Violin', rate: 5 },
        { itemName: 'Violy Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venatu (ID: 1679) ──── Level 75 | HP 12,300 | NORMAL | formless/water2 | aggressive
RO_MONSTER_TEMPLATES['venatu_4'] = {
    id: 1679, name: 'Venatu', aegisName: 'VENATU_4',
    level: 75, maxHealth: 12300, baseExp: 4000, jobExp: 2000, mvpExp: 0,
    attack: 800, attack2: 1400, defense: 30, magicDefense: 20,
    str: 5, agi: 26, vit: 24, int: 5, dex: 100, luk: 30,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 504, attackMotion: 1020, damageMotion: 360,
    size: 'medium', race: 'formless', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 5, agi: 26, vit: 24, int: 5, dex: 100, luk: 30, level: 75, weaponATK: 800 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 20 },
        { itemName: 'Crest Piece', rate: 3 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Armor Charm', rate: 0.1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Professional Cooking Kit', rate: 1 },
        { itemName: 'Venatu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Alicel (ID: 1735) ──── Level 75 | HP 37,520 | NORMAL | demon/neutral3 | aggressive
RO_MONSTER_TEMPLATES['alicel'] = {
    id: 1735, name: 'Alicel', aegisName: 'ALICEL',
    level: 75, maxHealth: 37520, baseExp: 8890, jobExp: 5420, mvpExp: 0,
    attack: 1800, attack2: 2770, defense: 30, magicDefense: 30,
    str: 50, agi: 58, vit: 50, int: 51, dex: 92, luk: 40,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 178, walkSpeed: 250, attackDelay: 1080, attackMotion: 480, damageMotion: 504,
    size: 'medium', race: 'demon', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 50, agi: 58, vit: 50, int: 51, dex: 92, luk: 40, level: 75, weaponATK: 1800 },
    modes: { detector: true },
    drops: [
        { itemName: 'Burnt Part', rate: 20 },
        { itemName: 'Solid Iron Piece', rate: 30 },
        { itemName: 'Steel', rate: 2 },
        { itemName: 'Rusty Screw', rate: 5 },
        { itemName: 'Drill Katar', rate: 0.05 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Vali\'s Manteau', rate: 0.2 },
        { itemName: 'Alicel Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Aliot (ID: 1736) ──── Level 75 | HP 48,290 | NORMAL | demon/neutral3 | aggressive
RO_MONSTER_TEMPLATES['aliot'] = {
    id: 1736, name: 'Aliot', aegisName: 'ALIOT',
    level: 75, maxHealth: 48290, baseExp: 13020, jobExp: 4006, mvpExp: 0,
    attack: 950, attack2: 2470, defense: 35, magicDefense: 15,
    str: 50, agi: 32, vit: 87, int: 12, dex: 68, luk: 19,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 200, attackDelay: 1296, attackMotion: 432, damageMotion: 360,
    size: 'medium', race: 'demon', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30000,
    raceGroups: {},
    stats: { str: 50, agi: 32, vit: 87, int: 12, dex: 68, luk: 19, level: 75, weaponATK: 950 },
    modes: { detector: true },
    drops: [
        { itemName: 'Burnt Part', rate: 20 },
        { itemName: 'Solid Iron Piece', rate: 30 },
        { itemName: 'Falcon Muffler', rate: 0.1 },
        { itemName: 'Rusty Screw', rate: 5 },
        { itemName: 'Claw', rate: 0.1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Curved Sword', rate: 0.15 },
        { itemName: 'Aliot Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mysteltainn (ID: 1203) ──── Level 76 | HP 33,350 | BOSS | formless/shadow4 | aggressive
RO_MONSTER_TEMPLATES['mysteltainn'] = {
    id: 1203, name: 'Mysteltainn', aegisName: 'MYSTELTAINN',
    level: 76, maxHealth: 33350, baseExp: 6457, jobExp: 5159, mvpExp: 0,
    attack: 1160, attack2: 1440, defense: 30, magicDefense: 30,
    str: 77, agi: 139, vit: 80, int: 35, dex: 159, luk: 65,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 250, attackDelay: 1152, attackMotion: 500, damageMotion: 240,
    size: 'large', race: 'formless', element: { type: 'shadow', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 77, agi: 139, vit: 80, int: 35, dex: 159, luk: 65, level: 76, weaponATK: 1160 },
    modes: {},
    drops: [
        { itemName: 'Loki\'s Whispers', rate: 0.01 },
        { itemName: 'Biotite', rate: 15 },
        { itemName: 'Slayer', rate: 0.7 },
        { itemName: 'Bastard Sword', rate: 0.4 },
        { itemName: 'Claymore', rate: 0.02 },
        { itemName: 'Steel', rate: 1.2 },
        { itemName: 'Oridecon', rate: 2.43 },
        { itemName: 'Mysteltainn Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cat o' Nine Tails (ID: 1307) ──── Level 76 | HP 64,512 | BOSS | demon/fire3 | aggressive
RO_MONSTER_TEMPLATES['cat_o_nine_tail'] = {
    id: 1307, name: 'Cat o\' Nine Tails', aegisName: 'CAT_O_NINE_TAIL',
    level: 76, maxHealth: 64512, baseExp: 10869, jobExp: 4283, mvpExp: 0,
    attack: 1112, attack2: 1275, defense: 61, magicDefense: 55,
    str: 55, agi: 75, vit: 55, int: 82, dex: 86, luk: 120,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 155, attackDelay: 1276, attackMotion: 576, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'fire', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 55, agi: 75, vit: 55, int: 82, dex: 86, luk: 120, level: 76, weaponATK: 1112 },
    modes: {},
    drops: [
        { itemName: 'Puppy Love', rate: 0.01 },
        { itemName: 'Silver Knife of Chastity', rate: 1.5 },
        { itemName: 'Punisher', rate: 0.05 },
        { itemName: 'Elunium', rate: 6 },
        { itemName: 'Oridecon', rate: 8 },
        { itemName: 'Gold', rate: 0.06 },
        { itemName: 'Old Purple Box', rate: 0.01 },
        { itemName: 'Cat O\' Nine Tails Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Acidus (ID: 1716) ──── Level 76 | HP 39,111 | NORMAL | dragon/wind2 | aggressive
RO_MONSTER_TEMPLATES['acidus_'] = {
    id: 1716, name: 'Acidus', aegisName: 'ACIDUS_',
    level: 76, maxHealth: 39111, baseExp: 14392, jobExp: 4203, mvpExp: 0,
    attack: 1180, attack2: 2000, defense: 21, magicDefense: 47,
    str: 0, agi: 78, vit: 31, int: 93, dex: 88, luk: 52,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 168, attackMotion: 768, damageMotion: 360,
    size: 'large', race: 'dragon', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30200,
    raceGroups: {},
    stats: { str: 0, agi: 78, vit: 31, int: 93, dex: 88, luk: 52, level: 76, weaponATK: 1180 },
    modes: {},
    drops: [
        { itemName: 'Blue Potion', rate: 1.5 },
        { itemName: 'Dragon Canine', rate: 40 },
        { itemName: 'Blue Herb', rate: 1.5 },
        { itemName: 'Dragon Scale', rate: 35.89 },
        { itemName: 'Blue Bijou', rate: 8 },
        { itemName: 'Rough Wind', rate: 0.2 },
        { itemName: 'Blue Bijou', rate: 1 },
        { itemName: 'Gold Acidus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Fire Imp (ID: 1837) ──── Level 76 | HP 46,430 | NORMAL | demon/fire3 | aggressive
RO_MONSTER_TEMPLATES['imp'] = {
    id: 1837, name: 'Fire Imp', aegisName: 'IMP',
    level: 76, maxHealth: 46430, baseExp: 25200, jobExp: 11077, mvpExp: 0,
    attack: 1059, attack2: 1509, defense: 27, magicDefense: 50,
    str: 37, agi: 76, vit: 30, int: 150, dex: 99, luk: 10,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 150, attackDelay: 824, attackMotion: 432, damageMotion: 360,
    size: 'small', race: 'demon', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30200,
    raceGroups: {},
    stats: { str: 37, agi: 76, vit: 30, int: 150, dex: 99, luk: 10, level: 76, weaponATK: 1059 },
    modes: { detector: true },
    drops: [
        { itemName: 'Burning Hair', rate: 30 },
        { itemName: 'Huuma Blaze Shuriken', rate: 0.03 },
        { itemName: 'Live Coal', rate: 25 },
        { itemName: 'Heart Breaker', rate: 0.1 },
        { itemName: 'Electric Eel', rate: 0.25 },
        { itemName: 'Ice Fireworks', rate: 0.2 },
        { itemName: 'Imp Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Orc Hero (ID: 1087) ──── Level 77 | HP 585,700 | MVP | demihuman/earth2 | aggressive
RO_MONSTER_TEMPLATES['ork_hero'] = {
    id: 1087, name: 'Orc Hero', aegisName: 'ORK_HERO',
    level: 77, maxHealth: 585700, baseExp: 58630, jobExp: 32890, mvpExp: 29315,
    attack: 2257, attack2: 2542, defense: 40, magicDefense: 45,
    str: 0, agi: 91, vit: 30, int: 70, dex: 105, luk: 90,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 150, attackDelay: 1678, attackMotion: 780, damageMotion: 648,
    size: 'large', race: 'demihuman', element: { type: 'earth', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 91, vit: 30, int: 70, dex: 105, luk: 90, level: 77, weaponATK: 2257 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Heroic Emblem', rate: 97 },
        { itemName: 'Monkey Circlet', rate: 5 },
        { itemName: 'Light Epsilon', rate: 1.5 },
        { itemName: 'Shield', rate: 2.5 },
        { itemName: 'Orcish Sword', rate: 10 },
        { itemName: 'Elunium', rate: 45.59 },
        { itemName: 'Giant Axe', rate: 1 },
        { itemName: 'Orc Hero Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Red Jewel', rate: 20 },
        { itemName: 'Yggdrasil Berry', rate: 15 },
    ],
};

// ──── Stormy Knight (ID: 1251) ──── Level 77 | HP 240,000 | MVP | formless/wind4 | aggressive
RO_MONSTER_TEMPLATES['knight_of_windstorm'] = {
    id: 1251, name: 'Stormy Knight', aegisName: 'KNIGHT_OF_WINDSTORM',
    level: 77, maxHealth: 240000, baseExp: 64350, jobExp: 21450, mvpExp: 32175,
    attack: 1425, attack2: 1585, defense: 35, magicDefense: 60,
    str: 75, agi: 185, vit: 83, int: 55, dex: 130, luk: 79,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 200, attackDelay: 468, attackMotion: 468, damageMotion: 288,
    size: 'large', race: 'formless', element: { type: 'wind', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 75, agi: 185, vit: 83, int: 55, dex: 130, luk: 79, level: 77, weaponATK: 1425 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Zephyrus', rate: 1.5 },
        { itemName: 'Old Blue Box', rate: 30 },
        { itemName: 'Old Purple Box', rate: 40 },
        { itemName: 'Ring', rate: 2 },
        { itemName: 'Manteau', rate: 5 },
        { itemName: 'Elunium', rate: 45.59 },
        { itemName: 'Grand Circlet', rate: 0.01 },
        { itemName: 'Stormy Knight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Aquamarine', rate: 45 },
        { itemName: 'Boots', rate: 5 },
    ],
};

// ──── Dark Illusion (ID: 1302) ──── Level 77 | HP 103,631 | BOSS | demon/undead4 | aggressive
RO_MONSTER_TEMPLATES['dark_illusion'] = {
    id: 1302, name: 'Dark Illusion', aegisName: 'DARK_ILLUSION',
    level: 77, maxHealth: 103631, baseExp: 11163, jobExp: 4181, mvpExp: 0,
    attack: 1300, attack2: 1983, defense: 64, magicDefense: 70,
    str: 5, agi: 100, vit: 40, int: 100, dex: 97, luk: 40,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 145, attackDelay: 1024, attackMotion: 768, damageMotion: 480,
    size: 'large', race: 'demon', element: { type: 'undead', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 5, agi: 100, vit: 40, int: 100, dex: 97, luk: 40, level: 77, weaponATK: 1300 },
    modes: {},
    drops: [
        { itemName: 'Evil Bone Wand', rate: 0.03 },
        { itemName: 'Bone Helm', rate: 0.02 },
        { itemName: 'Ragamuffin Manteau', rate: 0.03 },
        { itemName: 'Brigan', rate: 53.35 },
        { itemName: 'Mastela Fruit', rate: 1.2 },
        { itemName: 'White Herb', rate: 15.5 },
        { itemName: 'Broad Sword', rate: 0.02 },
        { itemName: 'Dark Illusion Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Lava Golem (ID: 1366) ──── Level 77 | HP 24,324 | NORMAL | formless/neutral1 | passive
RO_MONSTER_TEMPLATES['lava_golem'] = {
    id: 1366, name: 'Lava Golem', aegisName: 'LAVA_GOLEM',
    level: 77, maxHealth: 24324, baseExp: 6470, jobExp: 3879, mvpExp: 0,
    attack: 1541, attack2: 2049, defense: 65, magicDefense: 50,
    str: 0, agi: 57, vit: 115, int: 70, dex: 76, luk: 68,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 170, walkSpeed: 200, attackDelay: 1500, attackMotion: 500, damageMotion: 300,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 1 },
    monsterClass: 'normal', aiType: 'passive', respawnMs: 30400,
    raceGroups: { Golem: true, Element: true, ElementLevel: true, WalkSpeed: true, AttackDelay: true, AttackMotion: true, ClientAttackMotion: true, DamageMotion: true, Ai: true },
    stats: { str: 0, agi: 57, vit: 115, int: 70, dex: 76, luk: 68, level: 77, weaponATK: 1541 },
    modes: {},
    drops: [
        { itemName: 'Lava', rate: 45.59 },
        { itemName: 'Burning Heart', rate: 36.86 },
        { itemName: 'Full Plate', rate: 0.01 },
        { itemName: 'Full Plate', rate: 0.02 },
        { itemName: 'White Herb', rate: 25 },
        { itemName: 'Magma Fist', rate: 0.2 },
        { itemName: 'Lava Golem Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dimik (ID: 1669) ──── Level 77 | HP 10,000 | NORMAL | formless/neutral2 | aggressive
RO_MONSTER_TEMPLATES['dimik'] = {
    id: 1669, name: 'Dimik', aegisName: 'DIMIK',
    level: 77, maxHealth: 10000, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1040, attack2: 1880, defense: 45, magicDefense: 28,
    str: 15, agi: 35, vit: 40, int: 15, dex: 120, luk: 42,
    attackRange: 250, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 200, attackDelay: 576, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30400,
    raceGroups: {},
    stats: { str: 15, agi: 35, vit: 40, int: 15, dex: 120, luk: 42, level: 77, weaponATK: 1040 },
    modes: {},
    drops: [
        { itemName: 'Dimik Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venatu (ID: 1675) ──── Level 77 | HP 8,000 | NORMAL | formless/fire2 | aggressive
RO_MONSTER_TEMPLATES['venatu'] = {
    id: 1675, name: 'Venatu', aegisName: 'VENATU',
    level: 77, maxHealth: 8000, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 1200, attack2: 1800, defense: 35, magicDefense: 20,
    str: 5, agi: 26, vit: 24, int: 5, dex: 75, luk: 40,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 504, attackMotion: 1020, damageMotion: 360,
    size: 'medium', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30400,
    raceGroups: {},
    stats: { str: 5, agi: 26, vit: 24, int: 5, dex: 75, luk: 40, level: 77, weaponATK: 1200 },
    modes: {},
    drops: [
        { itemName: 'Venatu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Lady Solace (ID: 1703) ──── Level 77 | HP 25,252 | BOSS | angel/holy3 | aggressive
RO_MONSTER_TEMPLATES['solace'] = {
    id: 1703, name: 'Lady Solace', aegisName: 'SOLACE',
    level: 77, maxHealth: 25252, baseExp: 21000, jobExp: 25110, mvpExp: 0,
    attack: 1392, attack2: 1462, defense: 21, magicDefense: 67,
    str: 12, agi: 76, vit: 29, int: 145, dex: 99, luk: 100,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 180, attackDelay: 576, attackMotion: 420, damageMotion: 360,
    size: 'medium', race: 'angel', element: { type: 'holy', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 12, agi: 76, vit: 29, int: 145, dex: 99, luk: 100, level: 77, weaponATK: 1392 },
    modes: {},
    drops: [
        { itemName: 'Blue Feather', rate: 2 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Stone of Sage', rate: 0.5 },
        { itemName: 'Garnet', rate: 10 },
        { itemName: 'Harp', rate: 0.5 },
        { itemName: 'Harp', rate: 1 },
        { itemName: 'Cursed Seal', rate: 0.5 },
        { itemName: 'Lady Solace Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Osiris (ID: 1038) ──── Level 78 | HP 415,400 | MVP | undead/undead4 | aggressive
RO_MONSTER_TEMPLATES['osiris'] = {
    id: 1038, name: 'Osiris', aegisName: 'OSIRIS',
    level: 78, maxHealth: 415400, baseExp: 71500, jobExp: 28600, mvpExp: 35750,
    attack: 780, attack2: 2880, defense: 10, magicDefense: 25,
    str: 0, agi: 75, vit: 30, int: 37, dex: 86, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 179, walkSpeed: 100, attackDelay: 1072, attackMotion: 672, damageMotion: 384,
    size: 'medium', race: 'undead', element: { type: 'undead', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 75, vit: 30, int: 37, dex: 86, luk: 40, level: 78, weaponATK: 780 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Old Purple Box', rate: 20 },
        { itemName: 'Assassin Dagger', rate: 1.5 },
        { itemName: 'Crown', rate: 2 },
        { itemName: 'Jamadhar', rate: 6 },
        { itemName: 'Hand of God', rate: 10 },
        { itemName: 'Sphinx Hat', rate: 1.5 },
        { itemName: 'Chakram', rate: 1 },
        { itemName: 'Osiris Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Blue Box', rate: 40 },
        { itemName: 'Yggdrasil Seed', rate: 30 },
    ],
};

// ──── Nightmare Terror (ID: 1379) ──── Level 78 | HP 22,605 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['nightmare_terror'] = {
    id: 1379, name: 'Nightmare Terror', aegisName: 'NIGHTMARE_TERROR',
    level: 78, maxHealth: 22605, baseExp: 6683, jobExp: 4359, mvpExp: 0,
    attack: 757, attack2: 1007, defense: 37, magicDefense: 37,
    str: 0, agi: 76, vit: 55, int: 60, dex: 76, luk: 54,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 176, walkSpeed: 165, attackDelay: 1216, attackMotion: 816, damageMotion: 432,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30600,
    raceGroups: {},
    stats: { str: 0, agi: 76, vit: 55, int: 60, dex: 76, luk: 54, level: 78, weaponATK: 757 },
    modes: { detector: true },
    drops: [
        { itemName: 'Burning Horseshoe', rate: 49.47 },
        { itemName: 'Rosary', rate: 0.01 },
        { itemName: 'Rosary', rate: 0.3 },
        { itemName: 'Blue Potion', rate: 0.5 },
        { itemName: 'Blue Herb', rate: 1.5 },
        { itemName: 'Level 5 Soul Strike', rate: 1 },
        { itemName: 'Infiltrator', rate: 0.01 },
        { itemName: 'Nightmare Terror Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venatu (ID: 1678) ──── Level 78 | HP 9,500 | NORMAL | formless/earth2 | aggressive
RO_MONSTER_TEMPLATES['venatu_3'] = {
    id: 1678, name: 'Venatu', aegisName: 'VENATU_3',
    level: 78, maxHealth: 9500, baseExp: 4500, jobExp: 2000, mvpExp: 0,
    attack: 800, attack2: 1400, defense: 30, magicDefense: 20,
    str: 5, agi: 26, vit: 68, int: 5, dex: 95, luk: 30,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 504, attackMotion: 1020, damageMotion: 360,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30600,
    raceGroups: {},
    stats: { str: 5, agi: 26, vit: 68, int: 5, dex: 95, luk: 30, level: 78, weaponATK: 800 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 20 },
        { itemName: 'Crest Piece', rate: 4 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Armor Charm', rate: 0.1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Professional Cooking Kit', rate: 1 },
        { itemName: 'Venatu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Medusa (ID: 1148) ──── Level 79 | HP 16,408 | NORMAL | demon/neutral2 | aggressive
RO_MONSTER_TEMPLATES['medusa'] = {
    id: 1148, name: 'Medusa', aegisName: 'MEDUSA',
    level: 79, maxHealth: 16408, baseExp: 6876, jobExp: 4697, mvpExp: 0,
    attack: 827, attack2: 1100, defense: 28, magicDefense: 18,
    str: 0, agi: 74, vit: 50, int: 57, dex: 77, luk: 69,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 166, walkSpeed: 180, attackDelay: 1720, attackMotion: 1320, damageMotion: 360,
    size: 'medium', race: 'demon', element: { type: 'neutral', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30800,
    raceGroups: {},
    stats: { str: 0, agi: 74, vit: 50, int: 57, dex: 77, luk: 69, level: 79, weaponATK: 827 },
    modes: { detector: true },
    drops: [
        { itemName: 'Horrendous Hair', rate: 53.35 },
        { itemName: 'Red Flame Whip', rate: 2.5 },
        { itemName: 'Animal Gore', rate: 2 },
        { itemName: 'Sea Witch\'s Foot', rate: 0.2 },
        { itemName: 'Pearl', rate: 2.5 },
        { itemName: 'Turtle Shell', rate: 35 },
        { itemName: 'Necklace of Wisdom', rate: 0.03 },
        { itemName: 'Medusa Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dark Priest (ID: 1198) ──── Level 79 | HP 101,992 | BOSS | demon/undead4 | aggressive
RO_MONSTER_TEMPLATES['dark_priest'] = {
    id: 1198, name: 'Dark Priest', aegisName: 'DARK_PRIEST',
    level: 79, maxHealth: 101992, baseExp: 12192, jobExp: 5152, mvpExp: 0,
    attack: 1238, attack2: 2037, defense: 56, magicDefense: 70,
    str: 5, agi: 91, vit: 41, int: 101, dex: 103, luk: 42,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 200, attackDelay: 864, attackMotion: 1252, damageMotion: 476,
    size: 'medium', race: 'demon', element: { type: 'undead', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 5, agi: 91, vit: 41, int: 101, dex: 103, luk: 42, level: 79, weaponATK: 1238 },
    modes: {},
    drops: [
        { itemName: 'Book of the Apocalypse', rate: 0.05 },
        { itemName: 'Rosary', rate: 0.3 },
        { itemName: 'Blue Potion', rate: 1 },
        { itemName: 'Red Gemstone', rate: 4.5 },
        { itemName: 'Hand of God', rate: 0.5 },
        { itemName: 'Glittering Jacket', rate: 0.05 },
        { itemName: 'Cursed Dagger', rate: 0.01 },
        { itemName: 'Dark Priest Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Abysmal Knight (ID: 1219) ──── Level 79 | HP 36,140 | NORMAL | demihuman/shadow4 | aggressive
RO_MONSTER_TEMPLATES['knight_of_abyss'] = {
    id: 1219, name: 'Abysmal Knight', aegisName: 'KNIGHT_OF_ABYSS',
    level: 79, maxHealth: 36140, baseExp: 8469, jobExp: 6268, mvpExp: 0,
    attack: 1600, attack2: 2150, defense: 55, magicDefense: 50,
    str: 66, agi: 68, vit: 64, int: 25, dex: 135, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 170, walkSpeed: 300, attackDelay: 1500, attackMotion: 500, damageMotion: 1000,
    size: 'large', race: 'demihuman', element: { type: 'shadow', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30800,
    raceGroups: {},
    stats: { str: 66, agi: 68, vit: 64, int: 25, dex: 135, luk: 50, level: 79, weaponATK: 1600 },
    modes: {},
    drops: [
        { itemName: 'Reins', rate: 53.35 },
        { itemName: 'Blade Lost in Darkness', rate: 0.05 },
        { itemName: 'Lord\'s Clothes', rate: 0.01 },
        { itemName: 'Battle Hook', rate: 0.25 },
        { itemName: 'Broad Sword', rate: 0.01 },
        { itemName: 'Elunium', rate: 3.69 },
        { itemName: 'Oridecon', rate: 2.59 },
        { itemName: 'Abysmal Knight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Egnigem Cenia (ID: 1658) ──── Level 79 | HP 214,200 | MVP | demihuman/fire2 | aggressive
RO_MONSTER_TEMPLATES['b_ygnizem'] = {
    id: 1658, name: 'Egnigem Cenia', aegisName: 'B_YGNIZEM',
    level: 79, maxHealth: 214200, baseExp: 258760, jobExp: 86000, mvpExp: 129380,
    attack: 3890, attack2: 5690, defense: 48, magicDefense: 25,
    str: 82, agi: 60, vit: 45, int: 31, dex: 110, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 100, attackDelay: 1008, attackMotion: 864, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 82, agi: 60, vit: 45, int: 31, dex: 110, luk: 40, level: 79, weaponATK: 3890 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Broad Sword', rate: 10 },
        { itemName: 'Gift Box', rate: 50 },
        { itemName: 'Old Blue Box', rate: 50 },
        { itemName: 'Schweizersabel', rate: 10 },
        { itemName: 'Formal Suit', rate: 10 },
        { itemName: 'Boots', rate: 10 },
        { itemName: 'Nagan', rate: 10 },
        { itemName: 'General Egnigem Cenia Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Archdam (ID: 1668) ──── Level 79 | HP 25,000 | NORMAL | demihuman/neutral3 | aggressive
RO_MONSTER_TEMPLATES['archdam'] = {
    id: 1668, name: 'Archdam', aegisName: 'ARCHDAM',
    level: 79, maxHealth: 25000, baseExp: 8000, jobExp: 5000, mvpExp: 0,
    attack: 1000, attack2: 2000, defense: 15, magicDefense: 15,
    str: 65, agi: 65, vit: 35, int: 75, dex: 75, luk: 15,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 180, attackDelay: 580, attackMotion: 288, damageMotion: 360,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30800,
    raceGroups: {},
    stats: { str: 65, agi: 65, vit: 35, int: 75, dex: 75, luk: 15, level: 79, weaponATK: 1000 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 50 },
        { itemName: 'Steel', rate: 5 },
        { itemName: 'Oridecon', rate: 2 },
        { itemName: 'Elunium', rate: 2 },
        { itemName: 'Gate Keeper', rate: 0.05 },
        { itemName: 'Gate Keeper-DD', rate: 0.05 },
        { itemName: 'Archdam Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dimik (ID: 1670) ──── Level 79 | HP 16,000 | NORMAL | formless/wind2 | aggressive
RO_MONSTER_TEMPLATES['dimik_1'] = {
    id: 1670, name: 'Dimik', aegisName: 'DIMIK_1',
    level: 79, maxHealth: 16000, baseExp: 6400, jobExp: 3500, mvpExp: 0,
    attack: 1140, attack2: 1980, defense: 45, magicDefense: 28,
    str: 15, agi: 88, vit: 20, int: 20, dex: 120, luk: 40,
    attackRange: 350, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 576, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 30800,
    raceGroups: {},
    stats: { str: 15, agi: 88, vit: 20, int: 20, dex: 120, luk: 40, level: 79, weaponATK: 1140 },
    modes: {},
    drops: [
        { itemName: 'Used Iron Plate', rate: 20 },
        { itemName: 'Transparent Plate', rate: 0.5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Dusk', rate: 0.05 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Royal Cooking Kit', rate: 0.5 },
        { itemName: 'Dimik Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baroness of Retribution (ID: 1702) ──── Level 79 | HP 46,666 | BOSS | angel/shadow3 | aggressive
RO_MONSTER_TEMPLATES['retribution'] = {
    id: 1702, name: 'Baroness of Retribution', aegisName: 'RETRIBUTION',
    level: 79, maxHealth: 46666, baseExp: 28332, jobExp: 33120, mvpExp: 0,
    attack: 2022, attack2: 2288, defense: 35, magicDefense: 35,
    str: 30, agi: 142, vit: 66, int: 72, dex: 133, luk: 39,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 120, attackDelay: 360, attackMotion: 480, damageMotion: 360,
    size: 'medium', race: 'angel', element: { type: 'shadow', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 30, agi: 142, vit: 66, int: 72, dex: 133, luk: 39, level: 79, weaponATK: 2022 },
    modes: {},
    drops: [
        { itemName: 'Red Feather', rate: 4 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Stone of Sage', rate: 0.5 },
        { itemName: 'Ruby', rate: 10 },
        { itemName: 'Manteau', rate: 0.05 },
        { itemName: 'Two-Handed Sword', rate: 0.1 },
        { itemName: 'Cursed Seal', rate: 0.5 },
        { itemName: 'Baroness of Retribution Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dark Lord (ID: 1272) ──── Level 80 | HP 720,000 | MVP | demon/undead4 | aggressive
RO_MONSTER_TEMPLATES['dark_lord'] = {
    id: 1272, name: 'Dark Lord', aegisName: 'DARK_LORD',
    level: 80, maxHealth: 720000, baseExp: 65780, jobExp: 45045, mvpExp: 32890,
    attack: 2800, attack2: 3320, defense: 30, magicDefense: 70,
    str: 0, agi: 120, vit: 30, int: 118, dex: 99, luk: 60,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 100, attackDelay: 868, attackMotion: 768, damageMotion: 480,
    size: 'large', race: 'demon', element: { type: 'undead', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 120, vit: 30, int: 118, dex: 99, luk: 60, level: 80, weaponATK: 2800 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Evil Bone Wand', rate: 8 },
        { itemName: 'Kronos', rate: 1 },
        { itemName: 'Grimtooth', rate: 3 },
        { itemName: 'Mage Coat', rate: 3 },
        { itemName: 'Ancient Cape', rate: 1 },
        { itemName: 'Elunium', rate: 51.41 },
        { itemName: 'Bone Helm', rate: 0.1 },
        { itemName: 'Dark Lord Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Skull', rate: 60 },
        { itemName: 'Coif', rate: 5 },
    ],
};

// ──── Dimik (ID: 1672) ──── Level 80 | HP 19,000 | NORMAL | formless/earth2 | aggressive
RO_MONSTER_TEMPLATES['dimik_3'] = {
    id: 1672, name: 'Dimik', aegisName: 'DIMIK_3',
    level: 80, maxHealth: 19000, baseExp: 5900, jobExp: 2800, mvpExp: 0,
    attack: 1240, attack2: 2080, defense: 68, magicDefense: 28,
    str: 15, agi: 30, vit: 78, int: 20, dex: 120, luk: 30,
    attackRange: 250, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 200, attackDelay: 576, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'earth', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31000,
    raceGroups: {},
    stats: { str: 15, agi: 30, vit: 78, int: 20, dex: 120, luk: 30, level: 80, weaponATK: 1240 },
    modes: {},
    drops: [
        { itemName: 'Used Iron Plate', rate: 20 },
        { itemName: 'Transparent Plate', rate: 0.5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Armor Charm', rate: 0.1 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Royal Cooking Kit', rate: 0.5 },
        { itemName: 'Dimik Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Venatu (ID: 1677) ──── Level 80 | HP 9,000 | NORMAL | formless/wind2 | aggressive
RO_MONSTER_TEMPLATES['venatu_2'] = {
    id: 1677, name: 'Venatu', aegisName: 'VENATU_2',
    level: 80, maxHealth: 9000, baseExp: 4000, jobExp: 2000, mvpExp: 0,
    attack: 900, attack2: 1500, defense: 30, magicDefense: 20,
    str: 5, agi: 82, vit: 32, int: 5, dex: 105, luk: 30,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 504, attackMotion: 1020, damageMotion: 360,
    size: 'medium', race: 'formless', element: { type: 'wind', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31000,
    raceGroups: {},
    stats: { str: 5, agi: 82, vit: 32, int: 5, dex: 105, luk: 30, level: 80, weaponATK: 900 },
    modes: {},
    drops: [
        { itemName: 'Rusty Screw', rate: 20 },
        { itemName: 'Crest Piece', rate: 5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Long Barrel', rate: 0.1 },
        { itemName: 'Elunium', rate: 0.1 },
        { itemName: 'Professional Cooking Kit', rate: 1 },
        { itemName: 'Venatu Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Mistress of Shelter (ID: 1701) ──── Level 80 | HP 38,000 | BOSS | angel/holy3 | aggressive
RO_MONSTER_TEMPLATES['shelter'] = {
    id: 1701, name: 'Mistress of Shelter', aegisName: 'SHELTER',
    level: 80, maxHealth: 38000, baseExp: 29010, jobExp: 25110, mvpExp: 0,
    attack: 1871, attack2: 1971, defense: 22, magicDefense: 63,
    str: 12, agi: 67, vit: 34, int: 167, dex: 157, luk: 120,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 160, attackDelay: 432, attackMotion: 420, damageMotion: 360,
    size: 'medium', race: 'angel', element: { type: 'holy', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 12, agi: 67, vit: 34, int: 167, dex: 157, luk: 120, level: 80, weaponATK: 1871 },
    modes: {},
    drops: [
        { itemName: 'Red Feather', rate: 2 },
        { itemName: 'Cursed Seal', rate: 0.01 },
        { itemName: 'Stone of Sage', rate: 0.5 },
        { itemName: 'Pearl', rate: 10 },
        { itemName: 'Skull', rate: 10 },
        { itemName: 'Cursed Seal', rate: 0.5 },
        { itemName: 'Mistress of Shelter Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Acidus (ID: 1713) ──── Level 80 | HP 51,112 | NORMAL | dragon/holy2 | aggressive
RO_MONSTER_TEMPLATES['acidus'] = {
    id: 1713, name: 'Acidus', aegisName: 'ACIDUS',
    level: 80, maxHealth: 51112, baseExp: 28043, jobExp: 8023, mvpExp: 0,
    attack: 1289, attack2: 2109, defense: 29, magicDefense: 69,
    str: 0, agi: 71, vit: 55, int: 135, dex: 103, luk: 69,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 170, attackDelay: 168, attackMotion: 1008, damageMotion: 300,
    size: 'large', race: 'dragon', element: { type: 'holy', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31000,
    raceGroups: {},
    stats: { str: 0, agi: 71, vit: 55, int: 135, dex: 103, luk: 69, level: 80, weaponATK: 1289 },
    modes: {},
    drops: [
        { itemName: 'Orange', rate: 51 },
        { itemName: 'Dragon Canine', rate: 40 },
        { itemName: 'Treasure Box', rate: 0.05 },
        { itemName: 'Dragon Scale', rate: 35.89 },
        { itemName: 'Dragonball Yellow', rate: 8 },
        { itemName: 'Inverse Scale', rate: 0.1 },
        { itemName: 'Stone Buckler', rate: 0.5 },
        { itemName: 'Gold Acidus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bow Master (ID: 1830) ──── Level 80 | HP 80,404 | BOSS | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['bow_guardian'] = {
    id: 1830, name: 'Bow Master', aegisName: 'BOW_GUARDIAN',
    level: 80, maxHealth: 80404, baseExp: 50149, jobExp: 23006, mvpExp: 0,
    attack: 1840, attack2: 2520, defense: 40, magicDefense: 62,
    str: 95, agi: 80, vit: 33, int: 90, dex: 165, luk: 55,
    attackRange: 600, aggroRange: 700, chaseRange: 800,
    aspd: 190, walkSpeed: 170, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 95, agi: 80, vit: 33, int: 90, dex: 165, luk: 55, level: 80, weaponATK: 1840 },
    modes: {},
    drops: [
        { itemName: 'Destroyed Armor', rate: 30 },
        { itemName: 'Luna Bow', rate: 0.3 },
        { itemName: 'Bow', rate: 0.5 },
        { itemName: 'Sniping Suit', rate: 0.2 },
        { itemName: 'Orleans\'s Glove', rate: 0.04 },
        { itemName: 'Bow Guardian Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Baphomet (ID: 1039) ──── Level 81 | HP 668,000 | MVP | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['baphomet'] = {
    id: 1039, name: 'Baphomet', aegisName: 'BAPHOMET',
    level: 81, maxHealth: 668000, baseExp: 107250, jobExp: 37895, mvpExp: 53625,
    attack: 3220, attack2: 4040, defense: 35, magicDefense: 45,
    str: 0, agi: 152, vit: 30, int: 85, dex: 120, luk: 95,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 185, walkSpeed: 100, attackDelay: 768, attackMotion: 768, damageMotion: 576,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 152, vit: 30, int: 85, dex: 120, luk: 95, level: 81, weaponATK: 3220 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Crescent Scythe', rate: 4 },
        { itemName: 'Majestic Goat', rate: 3 },
        { itemName: 'Crescent Scythe', rate: 0.5 },
        { itemName: 'Emperium', rate: 5 },
        { itemName: 'Majestic Goat', rate: 0.1 },
        { itemName: 'Elunium', rate: 54.32 },
        { itemName: 'Oridecon', rate: 41.71 },
        { itemName: 'Baphomet Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 20 },
        { itemName: 'Baphomet Doll', rate: 5 },
    ],
};

// ──── Maya (ID: 1147) ──── Level 81 | HP 169,000 | MVP | insect/earth4 | aggressive
RO_MONSTER_TEMPLATES['maya'] = {
    id: 1147, name: 'Maya', aegisName: 'MAYA',
    level: 81, maxHealth: 169000, baseExp: 42900, jobExp: 17875, mvpExp: 21450,
    attack: 1800, attack2: 2070, defense: 60, magicDefense: 25,
    str: 95, agi: 97, vit: 76, int: 95, dex: 82, luk: 105,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 100, attackDelay: 864, attackMotion: 1000, damageMotion: 480,
    size: 'large', race: 'insect', element: { type: 'earth', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 95, agi: 97, vit: 76, int: 95, dex: 82, luk: 105, level: 81, weaponATK: 1800 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Queen\'s Hair Ornament', rate: 5 },
        { itemName: 'Safety Ring', rate: 2 },
        { itemName: 'Tiara', rate: 2 },
        { itemName: 'Armlet of Obedience', rate: 5 },
        { itemName: 'Mother\'s Nightmare', rate: 0.1 },
        { itemName: 'Elunium', rate: 35 },
        { itemName: 'Dea Staff', rate: 1 },
        { itemName: 'Maya Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: '1carat Diamond', rate: 20 },
        { itemName: 'Old Blue Box', rate: 30 },
    ],
};

// ──── Maya Purple (ID: 1289) ──── Level 81 | HP 55,479 | BOSS | insect/earth4 | aggressive
RO_MONSTER_TEMPLATES['maya_puple'] = {
    id: 1289, name: 'Maya Purple', aegisName: 'MAYA_PUPLE',
    level: 81, maxHealth: 55479, baseExp: 10496, jobExp: 3893, mvpExp: 0,
    attack: 1447, attack2: 2000, defense: 68, magicDefense: 48,
    str: 95, agi: 90, vit: 80, int: 95, dex: 90, luk: 119,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 180, walkSpeed: 100, attackDelay: 1024, attackMotion: 1000, damageMotion: 480,
    size: 'large', race: 'insect', element: { type: 'earth', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 95, agi: 90, vit: 80, int: 95, dex: 90, luk: 119, level: 81, weaponATK: 1447 },
    modes: {},
    drops: [
        { itemName: 'Cyfar', rate: 44.13 },
        { itemName: 'Rough Elunium', rate: 2.5 },
        { itemName: 'Rough Oridecon', rate: 3 },
        { itemName: 'Gold', rate: 1 },
        { itemName: 'Oridecon', rate: 1.5 },
        { itemName: 'Queen\'s Hair Ornament', rate: 0.01 },
        { itemName: 'Level 10 Cookbook', rate: 0.02 },
        { itemName: 'Maya Purple Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dame of Sentinel (ID: 1700) ──── Level 81 | HP 65,111 | BOSS | angel/neutral4 | aggressive
RO_MONSTER_TEMPLATES['observation'] = {
    id: 1700, name: 'Dame of Sentinel', aegisName: 'OBSERVATION',
    level: 81, maxHealth: 65111, baseExp: 39872, jobExp: 33120, mvpExp: 0,
    attack: 1666, attack2: 2609, defense: 55, magicDefense: 55,
    str: 30, agi: 74, vit: 56, int: 126, dex: 145, luk: 114,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 432, attackMotion: 480, damageMotion: 360,
    size: 'medium', race: 'angel', element: { type: 'neutral', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 30, agi: 74, vit: 56, int: 126, dex: 145, luk: 114, level: 81, weaponATK: 1666 },
    modes: {},
    drops: [
        { itemName: 'Blue Feather', rate: 5 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Cursed Seal', rate: 1 },
        { itemName: 'Topaz', rate: 10 },
        { itemName: 'Stone of Sage', rate: 1 },
        { itemName: 'Hair Band', rate: 0.1 },
        { itemName: 'Golden Ornament', rate: 1 },
        { itemName: 'Dame of Sentinel Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skeggiold (ID: 1754) ──── Level 81 | HP 295,200 | BOSS | angel/holy2 | aggressive
RO_MONSTER_TEMPLATES['skeggiold'] = {
    id: 1754, name: 'Skeggiold', aegisName: 'SKEGGIOLD',
    level: 81, maxHealth: 295200, baseExp: 91100, jobExp: 10, mvpExp: 0,
    attack: 1400, attack2: 2020, defense: 12, magicDefense: 24,
    str: 80, agi: 100, vit: 50, int: 72, dex: 90, luk: 50,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 250, attackDelay: 672, attackMotion: 780, damageMotion: 480,
    size: 'small', race: 'angel', element: { type: 'holy', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 80, agi: 100, vit: 50, int: 72, dex: 90, luk: 50, level: 81, weaponATK: 1400 },
    modes: {},
    drops: [
        { itemName: 'Rune of Darkness', rate: 60 },
        { itemName: 'Angel Wing', rate: 0.01 },
        { itemName: 'Soft Feather', rate: 10 },
        { itemName: 'Divine Cross', rate: 0.25 },
        { itemName: 'Rune of Darkness', rate: 10 },
        { itemName: 'Silk Robe', rate: 1 },
        { itemName: 'Odin\'s Blessing', rate: 1 },
        { itemName: 'Skeggiold Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Bloody Knight (ID: 1268) ──── Level 82 | HP 57,870 | NORMAL | formless/shadow4 | aggressive
RO_MONSTER_TEMPLATES['bloody_knight'] = {
    id: 1268, name: 'Bloody Knight', aegisName: 'BLOODY_KNIGHT',
    level: 82, maxHealth: 57870, baseExp: 10120, jobExp: 6820, mvpExp: 0,
    attack: 2150, attack2: 3030, defense: 60, magicDefense: 50,
    str: 88, agi: 75, vit: 70, int: 77, dex: 125, luk: 55,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 250, attackDelay: 828, attackMotion: 528, damageMotion: 192,
    size: 'large', race: 'formless', element: { type: 'shadow', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31400,
    raceGroups: {},
    stats: { str: 88, agi: 75, vit: 70, int: 77, dex: 125, luk: 55, level: 82, weaponATK: 2150 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 48.5 },
        { itemName: 'Helm', rate: 0.45 },
        { itemName: 'Full Plate', rate: 0.05 },
        { itemName: 'Strong Shield', rate: 0.62 },
        { itemName: 'Katzbalger', rate: 0.01 },
        { itemName: 'Pole Axe', rate: 0.02 },
        { itemName: 'Elunium', rate: 4.33 },
        { itemName: 'Bloody Knight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Cecil Damon (ID: 1638) ──── Level 82 | HP 58,900 | NORMAL | demihuman/wind3 | aggressive
RO_MONSTER_TEMPLATES['shecil'] = {
    id: 1638, name: 'Cecil Damon', aegisName: 'SHECIL',
    level: 82, maxHealth: 58900, baseExp: 100000, jobExp: 118260, mvpExp: 0,
    attack: 1226, attack2: 1854, defense: 25, magicDefense: 15,
    str: 0, agi: 145, vit: 27, int: 32, dex: 134, luk: 80,
    attackRange: 700, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31400,
    raceGroups: {},
    stats: { str: 0, agi: 145, vit: 27, int: 32, dex: 134, luk: 80, level: 82, weaponATK: 1226 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 30 },
        { itemName: 'Immaterial Arrow Quiver', rate: 1.1 },
        { itemName: 'Tights', rate: 0.1 },
        { itemName: 'Crossbow', rate: 1 },
        { itemName: 'Oridecon Arrow Quiver', rate: 1.5 },
        { itemName: 'Old Blue Box', rate: 0.5 },
        { itemName: 'Falken Blitz', rate: 0.01 },
        { itemName: 'Cecil Damon Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dimik (ID: 1673) ──── Level 82 | HP 13,900 | NORMAL | formless/fire2 | aggressive
RO_MONSTER_TEMPLATES['dimik_4'] = {
    id: 1673, name: 'Dimik', aegisName: 'DIMIK_4',
    level: 82, maxHealth: 13900, baseExp: 5800, jobExp: 4500, mvpExp: 0,
    attack: 1840, attack2: 2840, defense: 45, magicDefense: 28,
    str: 15, agi: 20, vit: 20, int: 10, dex: 120, luk: 30,
    attackRange: 250, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 200, attackDelay: 576, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'fire', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31400,
    raceGroups: {},
    stats: { str: 15, agi: 20, vit: 20, int: 10, dex: 120, luk: 30, level: 82, weaponATK: 1840 },
    modes: {},
    drops: [
        { itemName: 'Used Iron Plate', rate: 20 },
        { itemName: 'Transparent Plate', rate: 0.5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Armor Charm', rate: 0.1 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Royal Cooking Kit', rate: 0.5 },
        { itemName: 'Dimik Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Atroce (ID: 1785) ──── Level 82 | HP 1,008,420 | MVP | brute/shadow3 | aggressive
RO_MONSTER_TEMPLATES['atroce'] = {
    id: 1785, name: 'Atroce', aegisName: 'ATROCE',
    level: 82, maxHealth: 1008420, baseExp: 295550, jobExp: 118895, mvpExp: 147775,
    attack: 2526, attack2: 3646, defense: 25, magicDefense: 25,
    str: 100, agi: 87, vit: 30, int: 49, dex: 89, luk: 72,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 150, attackDelay: 576, attackMotion: 600, damageMotion: 240,
    size: 'large', race: 'brute', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 87, vit: 30, int: 49, dex: 89, luk: 72, level: 82, weaponATK: 2526 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Bloody Rune', rate: 70 },
        { itemName: 'Yggdrasil Seed', rate: 10 },
        { itemName: 'Ring', rate: 10 },
        { itemName: 'Old Purple Box', rate: 50 },
        { itemName: 'Yggdrasil Berry', rate: 50 },
        { itemName: 'Ulle\'s Cap', rate: 1 },
        { itemName: 'Atlas Weapon', rate: 1 },
        { itemName: 'Atroce Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Old Purple Box', rate: 50 },
    ],
};

// ──── Bloody Knight (ID: 1795) ──── Level 82 | HP 800,000 | BOSS | angel/ghost1 | aggressive
RO_MONSTER_TEMPLATES['bloody_knight_'] = {
    id: 1795, name: 'Bloody Knight', aegisName: 'BLOODY_KNIGHT_',
    level: 82, maxHealth: 800000, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 10000, attack2: 30000, defense: 60, magicDefense: 60,
    str: 88, agi: 121, vit: 100, int: 100, dex: 125, luk: 55,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 250, attackDelay: 828, attackMotion: 528, damageMotion: 192,
    size: 'large', race: 'angel', element: { type: 'ghost', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 88, agi: 121, vit: 100, int: 100, dex: 125, luk: 55, level: 82, weaponATK: 10000 },
    modes: {},
    drops: [
        { itemName: 'Pole Axe', rate: 1 },
        { itemName: 'Greaves', rate: 1 },
        { itemName: 'Pauldron', rate: 2 },
        { itemName: 'Legion Plate Armor', rate: 3 },
        { itemName: 'Heavenly Maiden Robe', rate: 2 },
        { itemName: 'Survivor\'s Rod', rate: 2 },
        { itemName: 'Old Purple Box', rate: 70 },
        { itemName: 'Countermagic Crystal', rate: 100, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Howard Alt-Eisen (ID: 1636) ──── Level 83 | HP 78,690 | NORMAL | demihuman/water4 | aggressive
RO_MONSTER_TEMPLATES['harword'] = {
    id: 1636, name: 'Howard Alt-Eisen', aegisName: 'HARWORD',
    level: 83, maxHealth: 78690, baseExp: 100000, jobExp: 112540, mvpExp: 0,
    attack: 1890, attack2: 2390, defense: 59, magicDefense: 10,
    str: 90, agi: 62, vit: 99, int: 35, dex: 98, luk: 66,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'water', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 31600,
    raceGroups: {},
    stats: { str: 90, agi: 62, vit: 99, int: 35, dex: 98, luk: 66, level: 83, weaponATK: 1890 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 30 },
        { itemName: 'Pauldron', rate: 0.01 },
        { itemName: 'Vecer Axe', rate: 0.01 },
        { itemName: 'Two-Handed Axe', rate: 1.1 },
        { itemName: 'Buckler', rate: 0.1 },
        { itemName: 'Lord\'s Clothes', rate: 0.01 },
        { itemName: 'Old Blue Box', rate: 0.5 },
        { itemName: 'Howard Alt-Eisen Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Maero of Thanatos (ID: 1706) ──── Level 83 | HP 62,000 | BOSS | undead/ghost4 | aggressive
RO_MONSTER_TEMPLATES['tha_maero'] = {
    id: 1706, name: 'Maero of Thanatos', aegisName: 'THA_MAERO',
    level: 83, maxHealth: 62000, baseExp: 56699, jobExp: 63880, mvpExp: 0,
    attack: 2022, attack2: 2288, defense: 29, magicDefense: 72,
    str: 100, agi: 176, vit: 30, int: 200, dex: 122, luk: 29,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 160, attackMotion: 480, damageMotion: 360,
    size: 'medium', race: 'undead', element: { type: 'ghost', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 176, vit: 30, int: 200, dex: 122, luk: 29, level: 83, weaponATK: 2022 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 10 },
        { itemName: '1carat Diamond', rate: 5 },
        { itemName: '1carat Diamond', rate: 1 },
        { itemName: 'Fragment of Misery', rate: 100 },
        { itemName: 'Old Card Album', rate: 0.1 },
        { itemName: 'Goibne\'s Armor', rate: 10 },
        { itemName: 'Maero of Thanatos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dolor of Thanatos (ID: 1707) ──── Level 83 | HP 59,922 | BOSS | undead/ghost4 | aggressive
RO_MONSTER_TEMPLATES['tha_dolor'] = {
    id: 1707, name: 'Dolor of Thanatos', aegisName: 'THA_DOLOR',
    level: 83, maxHealth: 59922, baseExp: 43200, jobExp: 51220, mvpExp: 0,
    attack: 1392, attack2: 2092, defense: 21, magicDefense: 80,
    str: 100, agi: 76, vit: 29, int: 206, dex: 139, luk: 44,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 160, attackMotion: 672, damageMotion: 480,
    size: 'small', race: 'undead', element: { type: 'ghost', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 76, vit: 29, int: 206, dex: 139, luk: 44, level: 83, weaponATK: 1392 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 10 },
        { itemName: '1carat Diamond', rate: 5 },
        { itemName: '1carat Diamond', rate: 1 },
        { itemName: 'Fragment of Agony', rate: 100 },
        { itemName: 'Old Card Album', rate: 0.1 },
        { itemName: 'Goibne\'s Helmet', rate: 10 },
        { itemName: 'Dolor of Thanatos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Skeggiold (ID: 1755) ──── Level 83 | HP 315,200 | BOSS | angel/holy2 | aggressive
RO_MONSTER_TEMPLATES['skeggiold_'] = {
    id: 1755, name: 'Skeggiold', aegisName: 'SKEGGIOLD_',
    level: 83, maxHealth: 315200, baseExp: 99200, jobExp: 10, mvpExp: 0,
    attack: 1600, attack2: 2050, defense: 15, magicDefense: 24,
    str: 80, agi: 120, vit: 60, int: 85, dex: 98, luk: 80,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 187, walkSpeed: 250, attackDelay: 672, attackMotion: 780, damageMotion: 480,
    size: 'small', race: 'angel', element: { type: 'holy', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 80, agi: 120, vit: 60, int: 85, dex: 98, luk: 80, level: 83, weaponATK: 1600 },
    modes: {},
    drops: [
        { itemName: 'Rune of Darkness', rate: 60 },
        { itemName: 'Angel Wing', rate: 0.01 },
        { itemName: 'Soft Feather', rate: 10 },
        { itemName: 'Divine Cross', rate: 0.25 },
        { itemName: 'Rune of Darkness', rate: 10 },
        { itemName: 'Silk Robe', rate: 1 },
        { itemName: 'Odin\'s Blessing', rate: 1 },
        { itemName: 'Skeggiold Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Succubus (ID: 1370) ──── Level 85 | HP 16,955 | NORMAL | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['succubus'] = {
    id: 1370, name: 'Succubus', aegisName: 'SUCCUBUS',
    level: 85, maxHealth: 16955, baseExp: 5357, jobExp: 4322, mvpExp: 0,
    attack: 1268, attack2: 1686, defense: 54, magicDefense: 48,
    str: 0, agi: 97, vit: 95, int: 103, dex: 89, luk: 87,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 155, attackDelay: 1306, attackMotion: 1056, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 32000,
    raceGroups: {},
    stats: { str: 0, agi: 97, vit: 95, int: 103, dex: 89, luk: 87, level: 85, weaponATK: 1268 },
    modes: { detector: true },
    drops: [
        { itemName: 'Mastela Fruit', rate: 15 },
        { itemName: 'Crystal Pumps', rate: 0.03 },
        { itemName: 'Boy\'s Naivety', rate: 0.01 },
        { itemName: 'Diamond Ring', rate: 2.5 },
        { itemName: 'Succubus Horn', rate: 0.01 },
        { itemName: 'Soul Staff', rate: 0.01 },
        { itemName: 'Blue Potion', rate: 10 },
        { itemName: 'Succubus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dracula (ID: 1389) ──── Level 85 | HP 320,096 | MVP | demon/shadow4 | aggressive
RO_MONSTER_TEMPLATES['dracula'] = {
    id: 1389, name: 'Dracula', aegisName: 'DRACULA',
    level: 85, maxHealth: 320096, baseExp: 120157, jobExp: 38870, mvpExp: 60078,
    attack: 1625, attack2: 1890, defense: 45, magicDefense: 76,
    str: 0, agi: 95, vit: 90, int: 87, dex: 85, luk: 100,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 174, walkSpeed: 145, attackDelay: 1290, attackMotion: 1140, damageMotion: 576,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 95, vit: 90, int: 87, dex: 85, luk: 100, level: 85, weaponATK: 1625 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Yggdrasil Berry', rate: 47 },
        { itemName: 'Wizardry Staff', rate: 0.05 },
        { itemName: 'Ballista', rate: 0.05 },
        { itemName: 'Ancient Cape', rate: 0.15 },
        { itemName: 'Ring', rate: 0.04 },
        { itemName: 'Book of the Apocalypse', rate: 0.04 },
        { itemName: 'Dracula Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: '1carat Diamond', rate: 50 },
    ],
};

// ──── White Lady (ID: 1630) ──── Level 85 | HP 253,221 | MVP | demihuman/wind3 | aggressive
RO_MONSTER_TEMPLATES['bacsojin_'] = {
    id: 1630, name: 'White Lady', aegisName: 'BACSOJIN_',
    level: 85, maxHealth: 253221, baseExp: 45250, jobExp: 16445, mvpExp: 22625,
    attack: 1868, attack2: 6124, defense: 20, magicDefense: 55,
    str: 52, agi: 65, vit: 44, int: 112, dex: 152, luk: 35,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 130, attackDelay: 576, attackMotion: 960, damageMotion: 480,
    size: 'large', race: 'demihuman', element: { type: 'wind', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 52, agi: 65, vit: 44, int: 112, dex: 152, luk: 35, level: 85, weaponATK: 1868 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Black Hair', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
        { itemName: 'Tantan Noodle', rate: 0.5 },
        { itemName: 'Transparent Celestial Robe', rate: 30 },
        { itemName: 'Soft Silk', rate: 10 },
        { itemName: 'Red Silk Seal', rate: 1 },
        { itemName: 'Tiara', rate: 0.1 },
        { itemName: 'White Lady Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Heavenly Maiden Robe', rate: 20 },
    ],
};

// ──── Kasa (ID: 1833) ──── Level 85 | HP 80,375 | BOSS | formless/fire3 | aggressive
RO_MONSTER_TEMPLATES['kasa'] = {
    id: 1833, name: 'Kasa', aegisName: 'KASA',
    level: 85, maxHealth: 80375, baseExp: 49000, jobExp: 38000, mvpExp: 0,
    attack: 3030, attack2: 3500, defense: 23, magicDefense: 70,
    str: 45, agi: 110, vit: 31, int: 200, dex: 140, luk: 30,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 184, walkSpeed: 150, attackDelay: 800, attackMotion: 600, damageMotion: 288,
    size: 'large', race: 'formless', element: { type: 'fire', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 45, agi: 110, vit: 31, int: 200, dex: 140, luk: 30, level: 85, weaponATK: 3030 },
    modes: {},
    drops: [
        { itemName: 'Burning Heart', rate: 30 },
        { itemName: 'Burning Hair', rate: 25 },
        { itemName: 'Flame Heart', rate: 0.3 },
        { itemName: 'Lesser Elemental Ring', rate: 0.01 },
        { itemName: 'Lucius\'s Fierce Armor of Volcano', rate: 0.1 },
        { itemName: 'Burning Bow', rate: 0.1 },
        { itemName: 'Piercing Staff', rate: 0.1 },
        { itemName: 'Kasa Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── RSX-0806 (ID: 1623) ──── Level 86 | HP 560,733 | MVP | formless/neutral3 | aggressive
RO_MONSTER_TEMPLATES['rsx_0806'] = {
    id: 1623, name: 'RSX-0806', aegisName: 'RSX_0806',
    level: 86, maxHealth: 560733, baseExp: 31010, jobExp: 32011, mvpExp: 15505,
    attack: 2740, attack2: 5620, defense: 39, magicDefense: 41,
    str: 85, agi: 51, vit: 30, int: 25, dex: 93, luk: 84,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 220, attackDelay: 128, attackMotion: 1104, damageMotion: 240,
    size: 'large', race: 'formless', element: { type: 'neutral', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 85, agi: 51, vit: 30, int: 25, dex: 93, luk: 84, level: 86, weaponATK: 2740 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Cogwheel', rate: 60 },
        { itemName: 'Ice Pick', rate: 0.1 },
        { itemName: 'Ice Pick', rate: 0.01 },
        { itemName: 'Old Purple Box', rate: 10 },
        { itemName: 'Flashlight', rate: 50 },
        { itemName: 'Dagger of Counter', rate: 0.5 },
        { itemName: 'Wrench', rate: 0.2 },
        { itemName: 'RSX-0806 Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Dark Blinder', rate: 35 },
    ],
};

// ──── Sword Master (ID: 1829) ──── Level 86 | HP 152,533 | BOSS | demihuman/neutral4 | aggressive
RO_MONSTER_TEMPLATES['sword_guardian'] = {
    id: 1829, name: 'Sword Master', aegisName: 'SWORD_GUARDIAN',
    level: 86, maxHealth: 152533, baseExp: 155013, jobExp: 122604, mvpExp: 0,
    attack: 7590, attack2: 9140, defense: 60, magicDefense: 33,
    str: 110, agi: 40, vit: 54, int: 65, dex: 125, luk: 65,
    attackRange: 100, aggroRange: 700, chaseRange: 800,
    aspd: 190, walkSpeed: 170, attackDelay: 140, attackMotion: 384, damageMotion: 288,
    size: 'large', race: 'demihuman', element: { type: 'neutral', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 110, agi: 40, vit: 54, int: 65, dex: 125, luk: 65, level: 86, weaponATK: 7590 },
    modes: {},
    drops: [
        { itemName: 'Destroyed Armor', rate: 30 },
        { itemName: 'Doom Slayer', rate: 0.3 },
        { itemName: 'Claymore', rate: 0.5 },
        { itemName: 'Zweihander', rate: 0.01 },
        { itemName: 'Platinum Shield', rate: 0.1 },
        { itemName: 'Muscle Cutter', rate: 0.5 },
        { itemName: 'Sword Guardian Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Byorgue (ID: 1839) ──── Level 86 | HP 38,133 | BOSS | demihuman/neutral1 | aggressive
RO_MONSTER_TEMPLATES['byorgue'] = {
    id: 1839, name: 'Byorgue', aegisName: 'BYORGUE',
    level: 86, maxHealth: 38133, baseExp: 19000, jobExp: 9500, mvpExp: 0,
    attack: 1340, attack2: 2590, defense: 20, magicDefense: 13,
    str: 25, agi: 80, vit: 12, int: 30, dex: 70, luk: 10,
    attackRange: 100, aggroRange: 700, chaseRange: 800,
    aspd: 184, walkSpeed: 170, attackDelay: 800, attackMotion: 600, damageMotion: 360,
    size: 'medium', race: 'demihuman', element: { type: 'neutral', level: 1 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 25, agi: 80, vit: 12, int: 30, dex: 70, luk: 10, level: 86, weaponATK: 1340 },
    modes: {},
    drops: [
        { itemName: 'Drill Katar', rate: 0.5 },
        { itemName: 'Assassin Mask', rate: 0.03 },
        { itemName: 'Scalpel', rate: 1.5 },
        { itemName: 'Steamed Alligator with Vegetable', rate: 5 },
        { itemName: 'Old Blue Box', rate: 0.4 },
        { itemName: 'Rider Insignia', rate: 0.01 },
        { itemName: 'Broken Sword', rate: 43.65 },
        { itemName: 'Byorgue Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Eremes Guile (ID: 1635) ──── Level 87 | HP 60,199 | NORMAL | demon/poison4 | aggressive
RO_MONSTER_TEMPLATES['eremes'] = {
    id: 1635, name: 'Eremes Guile', aegisName: 'EREMES',
    level: 87, maxHealth: 60199, baseExp: 100000, jobExp: 99800, mvpExp: 0,
    attack: 2020, attack2: 2320, defense: 23, magicDefense: 12,
    str: 45, agi: 138, vit: 31, int: 19, dex: 99, luk: 30,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'poison', level: 4 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 32400,
    raceGroups: {},
    stats: { str: 45, agi: 138, vit: 31, int: 19, dex: 99, luk: 30, level: 87, weaponATK: 2020 },
    modes: { detector: true },
    drops: [
        { itemName: 'Research Chart', rate: 20 },
        { itemName: 'Krishna', rate: 0.01 },
        { itemName: 'Pauldron', rate: 0.01 },
        { itemName: 'Loki\'s Nail', rate: 0.03 },
        { itemName: 'Specialty Jur', rate: 0.3 },
        { itemName: 'Poison Bottle', rate: 1.1 },
        { itemName: 'Thief Clothes', rate: 0.02 },
        { itemName: 'Eremes Guile Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Amon Ra (ID: 1511) ──── Level 88 | HP 1,214,138 | MVP | demihuman/earth3 | passive
RO_MONSTER_TEMPLATES['amon_ra'] = {
    id: 1511, name: 'Amon Ra', aegisName: 'AMON_RA',
    level: 88, maxHealth: 1214138, baseExp: 87264, jobExp: 35891, mvpExp: 43632,
    attack: 1647, attack2: 2576, defense: 26, magicDefense: 52,
    str: 0, agi: 0, vit: 90, int: 124, dex: 74, luk: 45,
    attackRange: 150, aggroRange: 0, chaseRange: 600,
    aspd: 183, walkSpeed: 170, attackDelay: 854, attackMotion: 2016, damageMotion: 480,
    size: 'large', race: 'demihuman', element: { type: 'earth', level: 3 },
    monsterClass: 'mvp', aiType: 'passive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 90, int: 124, dex: 74, luk: 45, level: 88, weaponATK: 1647 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Sphinx Hat', rate: 1.5 },
        { itemName: 'Safety Ring', rate: 0.5 },
        { itemName: 'Fragment of Rossata Stone', rate: 77.6 },
        { itemName: 'Elunium', rate: 38.8 },
        { itemName: 'Old Card Album', rate: 4 },
        { itemName: 'Tablet', rate: 0.1 },
        { itemName: 'Yggdrasil Berry', rate: 30 },
        { itemName: 'Amon Ra Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Yggdrasil Seed', rate: 35 },
    ],
};

// ──── Monemus (ID: 1674) ──── Level 88 | HP 80,000 | BOSS | formless/fire3 | passive
RO_MONSTER_TEMPLATES['monemus'] = {
    id: 1674, name: 'Monemus', aegisName: 'MONEMUS',
    level: 88, maxHealth: 80000, baseExp: 0, jobExp: 0, mvpExp: 0,
    attack: 2000, attack2: 3000, defense: 54, magicDefense: 25,
    str: 0, agi: 0, vit: 90, int: 24, dex: 144, luk: 45,
    attackRange: 250, aggroRange: 0, chaseRange: 600,
    aspd: 173, walkSpeed: 400, attackDelay: 1368, attackMotion: 1344, damageMotion: 432,
    size: 'large', race: 'formless', element: { type: 'fire', level: 3 },
    monsterClass: 'boss', aiType: 'passive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 90, int: 24, dex: 144, luk: 45, level: 88, weaponATK: 2000 },
    modes: {},
    drops: [
        { itemName: 'Stone', rate: 20 },
        { itemName: 'Stone Heart', rate: 10 },
    ],
    mvpDrops: [],
};

// ──── Despero of Thanatos (ID: 1705) ──── Level 88 | HP 86,666 | BOSS | undead/ghost4 | aggressive
RO_MONSTER_TEMPLATES['tha_despero'] = {
    id: 1705, name: 'Despero of Thanatos', aegisName: 'THA_DESPERO',
    level: 88, maxHealth: 86666, baseExp: 62001, jobExp: 51220, mvpExp: 0,
    attack: 2182, attack2: 3082, defense: 38, magicDefense: 39,
    str: 100, agi: 167, vit: 79, int: 92, dex: 151, luk: 120,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 150, attackDelay: 160, attackMotion: 528, damageMotion: 360,
    size: 'large', race: 'undead', element: { type: 'ghost', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 167, vit: 79, int: 92, dex: 151, luk: 120, level: 88, weaponATK: 2182 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 10 },
        { itemName: '1carat Diamond', rate: 5 },
        { itemName: '1carat Diamond', rate: 1 },
        { itemName: 'Fragment of Despair', rate: 100 },
        { itemName: 'Old Card Album', rate: 0.1 },
        { itemName: 'Goibne\'s Combat Boots', rate: 10 },
        { itemName: 'Despero of Thanatos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Dimik (ID: 1671) ──── Level 89 | HP 29,000 | NORMAL | formless/water2 | aggressive
RO_MONSTER_TEMPLATES['dimik_2'] = {
    id: 1671, name: 'Dimik', aegisName: 'DIMIK_2',
    level: 89, maxHealth: 29000, baseExp: 8000, jobExp: 5000, mvpExp: 0,
    attack: 1440, attack2: 2280, defense: 45, magicDefense: 28,
    str: 15, agi: 40, vit: 30, int: 30, dex: 150, luk: 70,
    attackRange: 250, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 200, attackDelay: 576, attackMotion: 720, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'water', level: 2 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 32800,
    raceGroups: {},
    stats: { str: 15, agi: 40, vit: 30, int: 30, dex: 150, luk: 70, level: 89, weaponATK: 1440 },
    modes: {},
    drops: [
        { itemName: 'Used Iron Plate', rate: 20 },
        { itemName: 'Transparent Plate', rate: 0.5 },
        { itemName: 'Steel', rate: 3 },
        { itemName: 'Fragment', rate: 3 },
        { itemName: 'Thunder P', rate: 0.1 },
        { itemName: 'Oridecon', rate: 0.1 },
        { itemName: 'Royal Cooking Kit', rate: 0.5 },
        { itemName: 'Dimik Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Lady Tanee (ID: 1688) ──── Level 89 | HP 493,000 | MVP | plant/wind3 | passive
RO_MONSTER_TEMPLATES['lady_tanee'] = {
    id: 1688, name: 'Lady Tanee', aegisName: 'LADY_TANEE',
    level: 89, maxHealth: 493000, baseExp: 64995, jobExp: 43222, mvpExp: 32497,
    attack: 450, attack2: 2170, defense: 20, magicDefense: 44,
    str: 0, agi: 125, vit: 48, int: 78, dex: 210, luk: 38,
    attackRange: 700, aggroRange: 0, chaseRange: 600,
    aspd: 188, walkSpeed: 100, attackDelay: 576, attackMotion: 432, damageMotion: 360,
    size: 'large', race: 'plant', element: { type: 'wind', level: 3 },
    monsterClass: 'mvp', aiType: 'passive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 125, vit: 48, int: 78, dex: 210, luk: 38, level: 89, weaponATK: 450 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Steamed Desert Scorpions', rate: 50 },
        { itemName: 'Tropical Banana', rate: 40 },
        { itemName: 'Fantastic Cooking Kit', rate: 10 },
        { itemName: 'Banana Hat', rate: 10 },
        { itemName: 'Elunium', rate: 50 },
        { itemName: 'Old Purple Box', rate: 20 },
        { itemName: 'Gakkung Bow', rate: 60 },
        { itemName: 'Lady Tanee Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Hwergelmir\'s Tonic', rate: 50 },
    ],
};

// ──── Hydrolancer (ID: 1720) ──── Level 89 | HP 308,230 | BOSS | dragon/shadow2 | aggressive
RO_MONSTER_TEMPLATES['hydro'] = {
    id: 1720, name: 'Hydrolancer', aegisName: 'HYDRO',
    level: 89, maxHealth: 308230, baseExp: 83450, jobExp: 2480, mvpExp: 0,
    attack: 2554, attack2: 3910, defense: 52, magicDefense: 62,
    str: 0, agi: 96, vit: 110, int: 86, dex: 94, luk: 32,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 160, attackDelay: 140, attackMotion: 672, damageMotion: 432,
    size: 'large', race: 'dragon', element: { type: 'shadow', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 96, vit: 110, int: 86, dex: 94, luk: 32, level: 89, weaponATK: 2554 },
    modes: {},
    drops: [
        { itemName: 'Dragon\'s Skin', rate: 40 },
        { itemName: 'Dragon Canine', rate: 40 },
        { itemName: 'Three-Headed Dragon\'s Head', rate: 38.8 },
        { itemName: 'Morpheus\'s Hood', rate: 5 },
        { itemName: 'Morrigane\'s Helm', rate: 5 },
        { itemName: 'Immortal Stew', rate: 3 },
        { itemName: 'Fricca\'s Circlet', rate: 5 },
        { itemName: 'Hydrolancer Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Gloom Under Night (ID: 1768) ──── Level 89 | HP 2,298,000 | MVP | formless/ghost3 | aggressive
RO_MONSTER_TEMPLATES['gloomundernight'] = {
    id: 1768, name: 'Gloom Under Night', aegisName: 'GLOOMUNDERNIGHT',
    level: 89, maxHealth: 2298000, baseExp: 962175, jobExp: 276445, mvpExp: 481087,
    attack: 5880, attack2: 9516, defense: 10, magicDefense: 20,
    str: 100, agi: 115, vit: 98, int: 78, dex: 111, luk: 50,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 173, walkSpeed: 200, attackDelay: 1344, attackMotion: 2880, damageMotion: 576,
    size: 'large', race: 'formless', element: { type: 'ghost', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 115, vit: 98, int: 78, dex: 111, luk: 50, level: 89, weaponATK: 5880 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Will of the Darkness', rate: 70 },
        { itemName: 'Blade Lost in Darkness', rate: 40 },
        { itemName: 'Old Hilt', rate: 20 },
        { itemName: 'Old Card Album', rate: 50 },
        { itemName: 'Heavenly Maiden Robe', rate: 10 },
        { itemName: 'Hurricane\'s Fury', rate: 1 },
        { itemName: 'Gloom Under Night Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Old Purple Box', rate: 50 },
    ],
};

// ──── Margaretha Sorin (ID: 1637) ──── Level 90 | HP 61,282 | NORMAL | demihuman/holy3 | aggressive
RO_MONSTER_TEMPLATES['magaleta'] = {
    id: 1637, name: 'Margaretha Sorin', aegisName: 'MAGALETA',
    level: 90, maxHealth: 61282, baseExp: 100000, jobExp: 117800, mvpExp: 0,
    attack: 1300, attack2: 2053, defense: 35, magicDefense: 60,
    str: 0, agi: 9, vit: 97, int: 145, dex: 88, luk: 40,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 180, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'holy', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 33000,
    raceGroups: {},
    stats: { str: 0, agi: 9, vit: 97, int: 145, dex: 88, luk: 40, level: 90, weaponATK: 1300 },
    modes: {},
    drops: [
        { itemName: 'Research Chart', rate: 20 },
        { itemName: 'Croce Staff', rate: 0.02 },
        { itemName: 'Rod', rate: 2 },
        { itemName: 'Hardcover Book', rate: 0.1 },
        { itemName: 'Holy Robe', rate: 0.01 },
        { itemName: 'Old Blue Box', rate: 0.5 },
        { itemName: 'Muffler', rate: 0.1 },
        { itemName: 'Margaretha Sorin Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Detardeurus (ID: 1719) ──── Level 90 | HP 960,000 | MVP | dragon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['detale'] = {
    id: 1719, name: 'Detardeurus', aegisName: 'DETALE',
    level: 90, maxHealth: 960000, baseExp: 291850, jobExp: 123304, mvpExp: 145925,
    attack: 4560, attack2: 5548, defense: 66, magicDefense: 59,
    str: 100, agi: 90, vit: 30, int: 136, dex: 140, luk: 56,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 250, attackDelay: 432, attackMotion: 936, damageMotion: 360,
    size: 'large', race: 'dragon', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 90, vit: 30, int: 136, dex: 140, luk: 56, level: 90, weaponATK: 4560 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Morpheus\'s Armlet', rate: 10 },
        { itemName: 'Morpheus\'s Ring', rate: 10 },
        { itemName: 'Treasure Box', rate: 50 },
        { itemName: 'Fire Dragon Scale', rate: 35.89 },
        { itemName: 'Dragon Breath Cocktail', rate: 10 },
        { itemName: 'Pole Axe', rate: 1 },
        { itemName: 'Jewel Crown', rate: 5 },
        { itemName: 'Detardeurus Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Kiehl (ID: 1733) ──── Level 90 | HP 523,000 | BOSS | formless/shadow2 | aggressive
RO_MONSTER_TEMPLATES['kiel'] = {
    id: 1733, name: 'Kiehl', aegisName: 'KIEL',
    level: 90, maxHealth: 523000, baseExp: 36500, jobExp: 23405, mvpExp: 0,
    attack: 1682, attack2: 3311, defense: 28, magicDefense: 32,
    str: 100, agi: 112, vit: 76, int: 89, dex: 156, luk: 102,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 140, attackDelay: 1152, attackMotion: 576, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'shadow', level: 2 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 112, vit: 76, int: 89, dex: 156, luk: 102, level: 90, weaponATK: 1682 },
    modes: {},
    drops: [],
    mvpDrops: [],
};

// ──── Kiel D-01 (ID: 1734) ──── Level 90 | HP 1,523,000 | MVP | formless/shadow2 | aggressive
RO_MONSTER_TEMPLATES['kiel_'] = {
    id: 1734, name: 'Kiel D-01', aegisName: 'KIEL_',
    level: 90, maxHealth: 1523000, baseExp: 2356200, jobExp: 512602, mvpExp: 1178100,
    attack: 3280, attack2: 6560, defense: 28, magicDefense: 32,
    str: 100, agi: 130, vit: 30, int: 160, dex: 199, luk: 180,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 130, attackDelay: 1152, attackMotion: 576, damageMotion: 432,
    size: 'medium', race: 'formless', element: { type: 'shadow', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 130, vit: 30, int: 160, dex: 199, luk: 180, level: 90, weaponATK: 3280 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Pocket Watch', rate: 30 },
        { itemName: 'Old Purple Box', rate: 30 },
        { itemName: 'Morrigane\'s Pendant', rate: 10 },
        { itemName: 'Glittering Jacket', rate: 10 },
        { itemName: 'Survivor\'s Rod', rate: 5 },
        { itemName: 'Dagger of Counter', rate: 5 },
        { itemName: 'Morrigane\'s Belt', rate: 10 },
        { itemName: 'Kiel-D-01 Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Dream Metal (ID: 1846) ──── Level 90 | HP 999 | BOSS | formless/holy1 | passive
RO_MONSTER_TEMPLATES['dreammetal'] = {
    id: 1846, name: 'Dream Metal', aegisName: 'DREAMMETAL',
    level: 90, maxHealth: 999, baseExp: 1, jobExp: 1, mvpExp: 0,
    attack: 1, attack2: 2, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 174, walkSpeed: 300, attackDelay: 1288, attackMotion: 288, damageMotion: 384,
    size: 'small', race: 'formless', element: { type: 'holy', level: 1 },
    monsterClass: 'boss', aiType: 'passive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, level: 90, weaponATK: 1 },
    modes: {},
    drops: [
        { itemName: 'Dragonball Yellow', rate: 20 },
    ],
    mvpDrops: [],
};

// ──── Seyren Windsor (ID: 1634) ──── Level 91 | HP 88,402 | NORMAL | demon/fire3 | aggressive
RO_MONSTER_TEMPLATES['seyren'] = {
    id: 1634, name: 'Seyren Windsor', aegisName: 'SEYREN',
    level: 91, maxHealth: 88402, baseExp: 100000, jobExp: 116460, mvpExp: 0,
    attack: 2100, attack2: 2530, defense: 63, magicDefense: 12,
    str: 90, agi: 89, vit: 72, int: 20, dex: 99, luk: 25,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 170, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demon', element: { type: 'fire', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 33200,
    raceGroups: {},
    stats: { str: 90, agi: 89, vit: 72, int: 20, dex: 99, luk: 25, level: 91, weaponATK: 2100 },
    modes: { detector: true },
    drops: [
        { itemName: 'Handcuffs', rate: 30 },
        { itemName: 'Dragon Killer', rate: 0.02 },
        { itemName: 'Claymore', rate: 2 },
        { itemName: 'Old Blue Box', rate: 0.3 },
        { itemName: 'Helm', rate: 0.12 },
        { itemName: 'Full Plate', rate: 0.01 },
        { itemName: 'Ruber', rate: 0.01 },
        { itemName: 'Seyren Windsor Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Salamander (ID: 1831) ──── Level 91 | HP 97,934 | BOSS | formless/fire3 | aggressive
RO_MONSTER_TEMPLATES['salamander'] = {
    id: 1831, name: 'Salamander', aegisName: 'SALAMANDER',
    level: 91, maxHealth: 97934, baseExp: 72000, jobExp: 55000, mvpExp: 0,
    attack: 7590, attack2: 10860, defense: 65, magicDefense: 50,
    str: 90, agi: 55, vit: 44, int: 45, dex: 180, luk: 25,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 160, attackDelay: 140, attackMotion: 384, damageMotion: 288,
    size: 'large', race: 'formless', element: { type: 'fire', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 90, agi: 55, vit: 44, int: 45, dex: 180, luk: 25, level: 91, weaponATK: 7590 },
    modes: {},
    drops: [
        { itemName: 'Burning Heart', rate: 30 },
        { itemName: 'Flame Heart', rate: 0.3 },
        { itemName: 'Red Gemstone', rate: 1 },
        { itemName: 'Lesser Elemental Ring', rate: 0.01 },
        { itemName: 'Berserk Guitar', rate: 0.5 },
        { itemName: 'Ring', rate: 0.01 },
        { itemName: 'Meteo Plate Armor', rate: 0.2 },
        { itemName: 'Salamander Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Kathryne Keyron (ID: 1639) ──── Level 92 | HP 47,280 | NORMAL | demihuman/ghost3 | aggressive
RO_MONSTER_TEMPLATES['katrinn'] = {
    id: 1639, name: 'Kathryne Keyron', aegisName: 'KATRINN',
    level: 92, maxHealth: 47280, baseExp: 100000, jobExp: 116470, mvpExp: 0,
    attack: 497, attack2: 1697, defense: 10, magicDefense: 74,
    str: 0, agi: 5, vit: 77, int: 180, dex: 110, luk: 39,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 150, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'ghost', level: 3 },
    monsterClass: 'normal', aiType: 'aggressive', respawnMs: 33400,
    raceGroups: {},
    stats: { str: 0, agi: 5, vit: 77, int: 180, dex: 110, luk: 39, level: 92, weaponATK: 497 },
    modes: {},
    drops: [
        { itemName: 'Handcuffs', rate: 30 },
        { itemName: 'Old Blue Box', rate: 0.5 },
        { itemName: 'La\'cryma Stick', rate: 0.01 },
        { itemName: 'Survivor\'s Rod', rate: 0.05 },
        { itemName: 'Guard', rate: 0.3 },
        { itemName: 'Small Ribbons', rate: 0.01 },
        { itemName: 'Shoes', rate: 0.2 },
        { itemName: 'Kathryne Keyron Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Odium of Thanatos (ID: 1704) ──── Level 92 | HP 72,389 | BOSS | undead/ghost4 | aggressive
RO_MONSTER_TEMPLATES['tha_odium'] = {
    id: 1704, name: 'Odium of Thanatos', aegisName: 'THA_ODIUM',
    level: 92, maxHealth: 72389, baseExp: 88420, jobExp: 63880, mvpExp: 0,
    attack: 2100, attack2: 2800, defense: 68, magicDefense: 30,
    str: 100, agi: 52, vit: 165, int: 62, dex: 185, luk: 90,
    attackRange: 450, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 432, attackMotion: 288, damageMotion: 420,
    size: 'large', race: 'undead', element: { type: 'ghost', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 52, vit: 165, int: 62, dex: 185, luk: 90, level: 92, weaponATK: 2100 },
    modes: {},
    drops: [
        { itemName: 'Brigan', rate: 10 },
        { itemName: '1carat Diamond', rate: 5 },
        { itemName: '1carat Diamond', rate: 1 },
        { itemName: 'Fragment of Hatred', rate: 100 },
        { itemName: 'Old Card Album', rate: 0.1 },
        { itemName: 'Goibne\'s Shoulder Arms', rate: 10 },
        { itemName: 'Odium of Thanatos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [],
};

// ──── Pharaoh (ID: 1157) ──── Level 93 | HP 445,997 | MVP | demihuman/shadow3 | aggressive
RO_MONSTER_TEMPLATES['pharaoh'] = {
    id: 1157, name: 'Pharaoh', aegisName: 'PHARAOH',
    level: 93, maxHealth: 445997, baseExp: 114990, jobExp: 41899, mvpExp: 57495,
    attack: 2267, attack2: 3015, defense: 67, magicDefense: 70,
    str: 0, agi: 93, vit: 100, int: 104, dex: 89, luk: 112,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 183, walkSpeed: 125, attackDelay: 868, attackMotion: 768, damageMotion: 288,
    size: 'large', race: 'demihuman', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 93, vit: 100, int: 104, dex: 89, luk: 112, level: 93, weaponATK: 2267 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Broken Pharaoh Emblem', rate: 58.2 },
        { itemName: 'Tutankhamen\'s Mask', rate: 25 },
        { itemName: 'Solar Sword', rate: 1 },
        { itemName: 'Holy Robe', rate: 1.5 },
        { itemName: 'Jewel Crown', rate: 5 },
        { itemName: 'Tablet', rate: 3 },
        { itemName: 'Bazerald', rate: 0.8 },
        { itemName: 'Pharaoh Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Royal Jelly', rate: 50 },
    ],
};

// ──── Lord of the Dead (ID: 1373) ──── Level 94 | HP 603,383 | MVP | demon/shadow3 | aggressive
RO_MONSTER_TEMPLATES['lord_of_death'] = {
    id: 1373, name: 'Lord of the Dead', aegisName: 'LORD_OF_DEATH',
    level: 94, maxHealth: 603383, baseExp: 131343, jobExp: 43345, mvpExp: 65671,
    attack: 3430, attack2: 4232, defense: 77, magicDefense: 73,
    str: 0, agi: 99, vit: 30, int: 109, dex: 100, luk: 106,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 171, walkSpeed: 180, attackDelay: 1446, attackMotion: 1296, damageMotion: 360,
    size: 'large', race: 'demon', element: { type: 'shadow', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 99, vit: 30, int: 109, dex: 100, luk: 106, level: 94, weaponATK: 3430 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Piece of Shield', rate: 53.35 },
        { itemName: 'Pole Axe', rate: 0.05 },
        { itemName: 'Ice Pick', rate: 0.1 },
        { itemName: 'Ring', rate: 0.02 },
        { itemName: 'Shining Spear Blade', rate: 0.1 },
        { itemName: 'War Axe', rate: 0.01 },
        { itemName: 'Iron Driver', rate: 0.02 },
        { itemName: 'Lord of The Dead Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: '1carat Diamond', rate: 50 },
    ],
};

// ──── Turtle General (ID: 1312) ──── Level 97 | HP 320,700 | MVP | brute/earth2 | aggressive
RO_MONSTER_TEMPLATES['turtle_general'] = {
    id: 1312, name: 'Turtle General', aegisName: 'TURTLE_GENERAL',
    level: 97, maxHealth: 320700, baseExp: 18202, jobExp: 9800, mvpExp: 9101,
    attack: 2438, attack2: 3478, defense: 50, magicDefense: 54,
    str: 100, agi: 45, vit: 55, int: 65, dex: 105, luk: 164,
    attackRange: 100, aggroRange: 500, chaseRange: 600,
    aspd: 182, walkSpeed: 200, attackDelay: 900, attackMotion: 1000, damageMotion: 500,
    size: 'large', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 45, vit: 55, int: 65, dex: 105, luk: 164, level: 97, weaponATK: 2438 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Iron Driver', rate: 0.08 },
        { itemName: 'War Axe', rate: 0.05 },
        { itemName: 'Level 9 Cookbook', rate: 2 },
        { itemName: 'Pole Axe', rate: 0.09 },
        { itemName: 'Broken Shell', rate: 53.35 },
        { itemName: 'Immaterial Sword', rate: 0.8 },
        { itemName: 'Union of Tribe', rate: 0.01 },
        { itemName: 'Turtle General Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Turtle Shell', rate: 55 },
        { itemName: 'Yggdrasil Berry', rate: 15 },
    ],
};

// ──── Vesper (ID: 1685) ──── Level 97 | HP 640,700 | MVP | brute/holy2 | aggressive
RO_MONSTER_TEMPLATES['apocalips_h'] = {
    id: 1685, name: 'Vesper', aegisName: 'APOCALIPS_H',
    level: 97, maxHealth: 640700, baseExp: 200000, jobExp: 100000, mvpExp: 100000,
    attack: 4000, attack2: 10000, defense: 50, magicDefense: 54,
    str: 100, agi: 50, vit: 30, int: 70, dex: 160, luk: 150,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 180, attackDelay: 504, attackMotion: 912, damageMotion: 432,
    size: 'large', race: 'brute', element: { type: 'holy', level: 2 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 50, vit: 30, int: 70, dex: 160, luk: 150, level: 97, weaponATK: 4000 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Metal Fragment', rate: 50 },
        { itemName: 'Fragment', rate: 30 },
        { itemName: 'Old Purple Box', rate: 10 },
        { itemName: 'Vesper Core 01', rate: 1 },
        { itemName: 'Vesper Core 02', rate: 1 },
        { itemName: 'Vesper Core 03', rate: 1 },
        { itemName: 'Vesper Core 04', rate: 1 },
        { itemName: 'Vesper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Ktullanux (ID: 1779) ──── Level 98 | HP 4,417,000 | MVP | brute/water4 | aggressive
RO_MONSTER_TEMPLATES['ktullanux'] = {
    id: 1779, name: 'Ktullanux', aegisName: 'KTULLANUX',
    level: 98, maxHealth: 4417000, baseExp: 2720050, jobExp: 1120020, mvpExp: 1360025,
    attack: 1680, attack2: 10360, defense: 40, magicDefense: 42,
    str: 85, agi: 126, vit: 30, int: 125, dex: 177, luk: 112,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 400, attackDelay: 432, attackMotion: 840, damageMotion: 216,
    size: 'large', race: 'brute', element: { type: 'water', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 85, agi: 126, vit: 30, int: 125, dex: 177, luk: 112, level: 98, weaponATK: 1680 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Ice Scale', rate: 90 },
        { itemName: 'Old Card Album', rate: 30 },
        { itemName: 'Survivor\'s Manteau', rate: 30 },
        { itemName: 'Sacred Mission', rate: 50 },
        { itemName: 'Old Purple Box', rate: 50 },
        { itemName: 'Yggdrasil Berry', rate: 50 },
        { itemName: 'Ktullanux Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Yggdrasil Berry', rate: 55 },
        { itemName: 'Old Purple Box', rate: 50 },
    ],
};

// ──── Bring it on! (ID: 1502) ──── Level 99 | HP 95,000,000 | MVP | plant/poison1 | aggressive
RO_MONSTER_TEMPLATES['poring_v'] = {
    id: 1502, name: 'Bring it on!', aegisName: 'PORING_V',
    level: 99, maxHealth: 95000000, baseExp: 87250, jobExp: 27895, mvpExp: 43625,
    attack: 10000, attack2: 30000, defense: 0, magicDefense: 10,
    str: 100, agi: 100, vit: 65, int: 100, dex: 255, luk: 255,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 167, walkSpeed: 160, attackDelay: 1672, attackMotion: 672, damageMotion: 480,
    size: 'medium', race: 'plant', element: { type: 'poison', level: 1 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 100, vit: 65, int: 100, dex: 255, luk: 255, level: 99, weaponATK: 10000 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Poring Hat', rate: 100 },
        { itemName: 'Lucius\'s Fierce Armor of Volcano', rate: 25 },
        { itemName: 'Saphien\'s Armor of Ocean', rate: 25 },
        { itemName: 'Claytos Cracking Earth Armor', rate: 25 },
        { itemName: 'Aebecee\'s Raging Typhoon Armor', rate: 25 },
        { itemName: 'Bloody Iron Ball', rate: 40 },
        { itemName: 'Large Jellopy', rate: 100 },
        { itemName: 'Holy Guard', rate: 45, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Grave Keeper\'s Sword', rate: 10 },
        { itemName: 'Poring Card', rate: 1 },
    ],
};

// ──── Lord Knight Seyren (ID: 1646) ──── Level 99 | HP 1,647,590 | MVP | demihuman/fire4 | aggressive
RO_MONSTER_TEMPLATES['b_seyren'] = {
    id: 1646, name: 'Lord Knight Seyren', aegisName: 'B_SEYREN',
    level: 99, maxHealth: 1647590, baseExp: 4835600, jobExp: 1569970, mvpExp: 2417800,
    attack: 7238, attack2: 11040, defense: 72, magicDefense: 37,
    str: 120, agi: 110, vit: 81, int: 65, dex: 130, luk: 52,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 120, agi: 110, vit: 81, int: 65, dex: 130, luk: 52, level: 99, weaponATK: 7238 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Edge', rate: 25 },
        { itemName: 'Legion Plate Armor', rate: 35 },
        { itemName: 'Greaves', rate: 90 },
        { itemName: 'Brionac', rate: 35 },
        { itemName: 'Longinus\'s Spear', rate: 30 },
        { itemName: 'Dragon Slayer', rate: 25 },
        { itemName: 'Brocca', rate: 15 },
        { itemName: 'Lord Knight Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Assassin Cross Eremes (ID: 1647) ──── Level 99 | HP 1,411,230 | MVP | demihuman/poison4 | aggressive
RO_MONSTER_TEMPLATES['b_eremes'] = {
    id: 1647, name: 'Assassin Cross Eremes', aegisName: 'B_EREMES',
    level: 99, maxHealth: 1411230, baseExp: 4083400, jobExp: 1592380, mvpExp: 2041700,
    attack: 4189, attack2: 8289, defense: 37, magicDefense: 39,
    str: 90, agi: 181, vit: 62, int: 37, dex: 122, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'poison', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 90, agi: 181, vit: 62, int: 37, dex: 122, luk: 60, level: 99, weaponATK: 4189 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Moonlight Dagger', rate: 15 },
        { itemName: 'Ice Pick', rate: 15 },
        { itemName: 'Glittering Jacket', rate: 90 },
        { itemName: 'Exorciser', rate: 35 },
        { itemName: 'Assassin Dagger', rate: 35 },
        { itemName: 'Bloody Roar', rate: 35 },
        { itemName: 'Ginnungagap', rate: 35 },
        { itemName: 'Assassin Cross Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Whitesmith Howard (ID: 1648) ──── Level 99 | HP 1,460,000 | MVP | demihuman/earth4 | aggressive
RO_MONSTER_TEMPLATES['b_harword'] = {
    id: 1648, name: 'Whitesmith Howard', aegisName: 'B_HARWORD',
    level: 99, maxHealth: 1460000, baseExp: 4002340, jobExp: 1421000, mvpExp: 2001170,
    attack: 7822, attack2: 8251, defense: 66, magicDefense: 36,
    str: 100, agi: 73, vit: 112, int: 35, dex: 136, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 73, vit: 112, int: 35, dex: 136, luk: 60, level: 99, weaponATK: 7822 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Mysteltainn', rate: 35 },
        { itemName: 'Byeollungum', rate: 25 },
        { itemName: 'Lord\'s Clothes', rate: 90 },
        { itemName: 'Sabbath', rate: 35 },
        { itemName: 'Great Axe', rate: 35 },
        { itemName: 'Guillotine', rate: 25 },
        { itemName: 'Tomahawk', rate: 35 },
        { itemName: 'MasterSmith Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── High Priest Margaretha (ID: 1649) ──── Level 99 | HP 1,092,910 | MVP | demihuman/holy4 | aggressive
RO_MONSTER_TEMPLATES['b_magaleta'] = {
    id: 1649, name: 'High Priest Margaretha', aegisName: 'B_MAGALETA',
    level: 99, maxHealth: 1092910, baseExp: 4257000, jobExp: 1318800, mvpExp: 2128500,
    attack: 4688, attack2: 5580, defense: 35, magicDefense: 78,
    str: 0, agi: 84, vit: 64, int: 182, dex: 92, luk: 100,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 125, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'holy', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 84, vit: 64, int: 182, dex: 92, luk: 100, level: 99, weaponATK: 4688 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Berserk', rate: 35 },
        { itemName: 'Safety Ring', rate: 25 },
        { itemName: 'Heavenly Maiden Robe', rate: 90 },
        { itemName: 'Book of the Apocalypse', rate: 35 },
        { itemName: 'Quadrille', rate: 35 },
        { itemName: 'Grand Cross', rate: 25 },
        { itemName: 'Diary Of Great Sage', rate: 35 },
        { itemName: 'High Priest Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Sniper Cecil (ID: 1650) ──── Level 99 | HP 1,349,000 | MVP | demihuman/wind4 | aggressive
RO_MONSTER_TEMPLATES['b_shecil'] = {
    id: 1650, name: 'Sniper Cecil', aegisName: 'B_SHECIL',
    level: 99, maxHealth: 1349000, baseExp: 4093000, jobExp: 1526000, mvpExp: 2046500,
    attack: 4892, attack2: 9113, defense: 22, magicDefense: 35,
    str: 0, agi: 180, vit: 39, int: 67, dex: 193, luk: 130,
    attackRange: 700, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 180, vit: 39, int: 67, dex: 193, luk: 130, level: 99, weaponATK: 4892 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Combat Knife', rate: 35 },
        { itemName: 'Sucsamad', rate: 35 },
        { itemName: 'Old Purple Box', rate: 90 },
        { itemName: 'Moonlight Dagger', rate: 15 },
        { itemName: 'Grimtooth', rate: 35 },
        { itemName: 'Rudra Bow', rate: 15 },
        { itemName: 'Dragon Wing', rate: 25 },
        { itemName: 'Sniper Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── High Wizard Kathryne (ID: 1651) ──── Level 99 | HP 1,069,920 | MVP | demihuman/ghost3 | aggressive
RO_MONSTER_TEMPLATES['b_katrinn'] = {
    id: 1651, name: 'High Wizard Kathryne', aegisName: 'B_KATRINN',
    level: 99, maxHealth: 1069920, baseExp: 4008200, jobExp: 1636700, mvpExp: 2004100,
    attack: 1197, attack2: 4394, defense: 10, magicDefense: 88,
    str: 0, agi: 89, vit: 42, int: 223, dex: 128, luk: 93,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 150, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'ghost', level: 3 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 0, agi: 89, vit: 42, int: 223, dex: 128, luk: 93, level: 99, weaponATK: 1197 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Cursed Dagger', rate: 35 },
        { itemName: 'Dagger of Counter', rate: 35 },
        { itemName: 'Critical Ring', rate: 90 },
        { itemName: 'Robe of Cast', rate: 25 },
        { itemName: 'Heavenly Maiden Robe', rate: 25 },
        { itemName: 'Survivor\'s Rod', rate: 30 },
        { itemName: 'Glittering Jacket', rate: 35 },
        { itemName: 'High Wizard Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Memory of Thanatos (ID: 1708) ──── Level 99 | HP 445,660 | MVP | demon/ghost4 | aggressive
RO_MONSTER_TEMPLATES['thanatos'] = {
    id: 1708, name: 'Memory of Thanatos', aegisName: 'THANATOS',
    level: 99, maxHealth: 445660, baseExp: 3666000, jobExp: 2145060, mvpExp: 1833000,
    attack: 3812, attack2: 7483, defense: 35, magicDefense: 35,
    str: 100, agi: 108, vit: 30, int: 86, dex: 147, luk: 32,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 120, attackDelay: 115, attackMotion: 816, damageMotion: 504,
    size: 'large', race: 'demon', element: { type: 'ghost', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 108, vit: 30, int: 86, dex: 147, luk: 32, level: 99, weaponATK: 3812 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Treasure Box', rate: 10 },
        { itemName: 'Morrigane\'s Manteau', rate: 10 },
        { itemName: 'Skeletal Armor Piece', rate: 50 },
        { itemName: 'Legion Plate Armor', rate: 50 },
        { itemName: 'Greaves', rate: 50 },
        { itemName: 'Eagle Wing', rate: 10 },
        { itemName: 'Bloody Iron Ball', rate: 5 },
        { itemName: 'Memory of Thanatos Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Valkyrie Randgris (ID: 1751) ──── Level 99 | HP 3,567,200 | MVP | angel/holy4 | aggressive
RO_MONSTER_TEMPLATES['randgris'] = {
    id: 1751, name: 'Valkyrie Randgris', aegisName: 'RANDGRIS',
    level: 99, maxHealth: 3567200, baseExp: 2854900, jobExp: 3114520, mvpExp: 1427450,
    attack: 5560, attack2: 9980, defense: 25, magicDefense: 42,
    str: 100, agi: 120, vit: 30, int: 120, dex: 220, luk: 210,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 188, walkSpeed: 100, attackDelay: 576, attackMotion: 576, damageMotion: 480,
    size: 'large', race: 'angel', element: { type: 'holy', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 100, agi: 120, vit: 30, int: 120, dex: 220, luk: 210, level: 99, weaponATK: 5560 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Valhalla\'s Flower', rate: 50 },
        { itemName: 'Valkyrian Armor', rate: 16 },
        { itemName: 'Valkyrian Manteau', rate: 30 },
        { itemName: 'Valkyrian Shoes', rate: 30 },
        { itemName: 'Helm', rate: 50 },
        { itemName: 'Bloody Edge', rate: 25 },
        { itemName: 'Randgris Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Lord Knight Seyren (ID: 1805) ──── Level 99 | HP 1,647,590 | BOSS | demihuman/fire4 | aggressive
RO_MONSTER_TEMPLATES['b_seyren_'] = {
    id: 1805, name: 'Lord Knight Seyren', aegisName: 'B_SEYREN_',
    level: 99, maxHealth: 1647590, baseExp: 4835600, jobExp: 1569970, mvpExp: 0,
    attack: 7238, attack2: 11040, defense: 72, magicDefense: 37,
    str: 120, agi: 110, vit: 81, int: 65, dex: 130, luk: 52,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'fire', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 120, agi: 110, vit: 81, int: 65, dex: 130, luk: 52, level: 99, weaponATK: 7238 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── Assassin Cross Eremes (ID: 1806) ──── Level 99 | HP 1,411,230 | BOSS | demihuman/poison4 | aggressive
RO_MONSTER_TEMPLATES['b_eremes_'] = {
    id: 1806, name: 'Assassin Cross Eremes', aegisName: 'B_EREMES_',
    level: 99, maxHealth: 1411230, baseExp: 4083400, jobExp: 1592380, mvpExp: 0,
    attack: 4189, attack2: 8289, defense: 37, magicDefense: 39,
    str: 90, agi: 181, vit: 62, int: 37, dex: 122, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'poison', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 90, agi: 181, vit: 62, int: 37, dex: 122, luk: 60, level: 99, weaponATK: 4189 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── Mastersmith Howard (ID: 1807) ──── Level 99 | HP 1,460,000 | BOSS | demihuman/earth4 | aggressive
RO_MONSTER_TEMPLATES['b_harword_'] = {
    id: 1807, name: 'Mastersmith Howard', aegisName: 'B_HARWORD_',
    level: 99, maxHealth: 1460000, baseExp: 4002340, jobExp: 1421000, mvpExp: 0,
    attack: 7822, attack2: 8251, defense: 66, magicDefense: 36,
    str: 100, agi: 73, vit: 112, int: 35, dex: 136, luk: 60,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'earth', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 100, agi: 73, vit: 112, int: 35, dex: 136, luk: 60, level: 99, weaponATK: 7822 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── High Priest Margaretha (ID: 1808) ──── Level 99 | HP 1,092,910 | BOSS | demihuman/holy4 | aggressive
RO_MONSTER_TEMPLATES['b_magaleta_'] = {
    id: 1808, name: 'High Priest Margaretha', aegisName: 'B_MAGALETA_',
    level: 99, maxHealth: 1092910, baseExp: 4257000, jobExp: 1318800, mvpExp: 0,
    attack: 4688, attack2: 5580, defense: 35, magicDefense: 78,
    str: 0, agi: 84, vit: 64, int: 182, dex: 92, luk: 100,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 125, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'holy', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 84, vit: 64, int: 182, dex: 92, luk: 100, level: 99, weaponATK: 4688 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── Sniper Cecil (ID: 1809) ──── Level 99 | HP 1,349,000 | BOSS | demihuman/wind4 | aggressive
RO_MONSTER_TEMPLATES['b_shecil_'] = {
    id: 1809, name: 'Sniper Cecil', aegisName: 'B_SHECIL_',
    level: 99, maxHealth: 1349000, baseExp: 4093000, jobExp: 1526000, mvpExp: 0,
    attack: 4892, attack2: 9113, defense: 22, magicDefense: 35,
    str: 0, agi: 180, vit: 39, int: 67, dex: 193, luk: 130,
    attackRange: 700, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 100, attackDelay: 76, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'wind', level: 4 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 180, vit: 39, int: 67, dex: 193, luk: 130, level: 99, weaponATK: 4892 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── High Wizard Kathryne (ID: 1810) ──── Level 99 | HP 1,069,920 | BOSS | demihuman/ghost3 | aggressive
RO_MONSTER_TEMPLATES['b_katrinn_'] = {
    id: 1810, name: 'High Wizard Kathryne', aegisName: 'B_KATRINN_',
    level: 99, maxHealth: 1069920, baseExp: 4008200, jobExp: 1636700, mvpExp: 0,
    attack: 1197, attack2: 4394, defense: 10, magicDefense: 88,
    str: 0, agi: 89, vit: 42, int: 223, dex: 128, luk: 93,
    attackRange: 50, aggroRange: 500, chaseRange: 600,
    aspd: 177, walkSpeed: 150, attackDelay: 1152, attackMotion: 384, damageMotion: 288,
    size: 'medium', race: 'demihuman', element: { type: 'ghost', level: 3 },
    monsterClass: 'boss', aiType: 'aggressive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 89, vit: 42, int: 223, dex: 128, luk: 93, level: 99, weaponATK: 1197 },
    modes: {},
    drops: [
        { itemName: 'Evil Mind', rate: 100 },
    ],
    mvpDrops: [],
};

// ──── Ifrit (ID: 1832) ──── Level 99 | HP 7,700,000 | MVP | formless/fire4 | aggressive
RO_MONSTER_TEMPLATES['ifrit'] = {
    id: 1832, name: 'Ifrit', aegisName: 'IFRIT',
    level: 99, maxHealth: 7700000, baseExp: 3154321, jobExp: 3114520, mvpExp: 1577160,
    attack: 13530, attack2: 17000, defense: 40, magicDefense: 50,
    str: 120, agi: 180, vit: 25, int: 190, dex: 199, luk: 50,
    attackRange: 150, aggroRange: 500, chaseRange: 600,
    aspd: 190, walkSpeed: 130, attackDelay: 212, attackMotion: 384, damageMotion: 360,
    size: 'large', race: 'formless', element: { type: 'fire', level: 4 },
    monsterClass: 'mvp', aiType: 'aggressive', respawnMs: 3600000,
    raceGroups: {},
    stats: { str: 120, agi: 180, vit: 25, int: 190, dex: 199, luk: 50, level: 99, weaponATK: 13530 },
    modes: { mvp: true },
    drops: [
        { itemName: 'Flame Heart', rate: 100 },
        { itemName: 'Spiritual Ring', rate: 30 },
        { itemName: 'Ring Of Flame Lord', rate: 2 },
        { itemName: 'Ring Of Resonance', rate: 2 },
        { itemName: 'Hellfire', rate: 20 },
        { itemName: 'Fireblend', rate: 20 },
        { itemName: 'Lucius\'s Fierce Armor of Volcano', rate: 1 },
        { itemName: 'Ifrit Card', rate: 0.01, stealProtected: true },
    ],
    mvpDrops: [
        { itemName: 'Old Purple Box', rate: 55 },
        { itemName: 'Old Blue Box', rate: 50 },
    ],
};

// ──── Golden Savage (ID: 1840) ──── Level 99 | HP 500 | BOSS | brute/earth2 | passive
RO_MONSTER_TEMPLATES['golden_savage'] = {
    id: 1840, name: 'Golden Savage', aegisName: 'GOLDEN_SAVAGE',
    level: 99, maxHealth: 500, baseExp: 1, jobExp: 1, mvpExp: 0,
    attack: 500, attack2: 700, defense: 100, magicDefense: 99,
    str: 0, agi: 0, vit: 0, int: 50, dex: 120, luk: 0,
    attackRange: 50, aggroRange: 0, chaseRange: 600,
    aspd: 161, walkSpeed: 150, attackDelay: 1960, attackMotion: 480, damageMotion: 384,
    size: 'large', race: 'brute', element: { type: 'earth', level: 2 },
    monsterClass: 'boss', aiType: 'passive', respawnMs: 1800000,
    raceGroups: {},
    stats: { str: 0, agi: 0, vit: 0, int: 50, dex: 120, luk: 0, level: 99, weaponATK: 500 },
    modes: {},
    drops: [
        { itemName: 'Yggdrasil Leaf', rate: 30 },
        { itemName: 'Treasure Box', rate: 1 },
        { itemName: 'Old Card Album', rate: 0.05 },
        { itemName: 'Gold', rate: 5 },
        { itemName: 'Emperium', rate: 1 },
        { itemName: 'Golden Gear', rate: 0.01 },
        { itemName: 'New Year Rice Cake', rate: 30 },
        { itemName: 'New Year Rice Cake', rate: 30, stealProtected: true },
    ],
    mvpDrops: [],
};

// ════════════════════════════════════════════════════════════
// SUMMARY
// ════════════════════════════════════════════════════════════
// Total monsters: 509
// Level 01-10: 50
// Level 11-20: 41
// Level 21-30: 59
// Level 31-40: 51
// Level 41-50: 52
// Level 51-60: 67
// Level 61-70: 72
// Level 71-85: 75
// Level 86+: 42
//
// normal: 412
// boss: 54
// mvp: 43
//
// plant: 44
// insect: 55
// formless: 89
// brute: 83
// fish: 19
// undead: 38
// demon: 64
// angel: 11
// demihuman: 91
// dragon: 15
//
// Monsters with drops: 508
// Monsters without drops: 1

module.exports = { RO_MONSTER_TEMPLATES };
