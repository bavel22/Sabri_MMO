'use strict';

// ============================================================
// RO Item Groups — Used by card effects (bAddItemGroupHealRate, bAddMonsterDropItemGroup)
// Based on rAthena item_group_db.txt pre-renewal definitions
// ============================================================

const ITEM_GROUPS = {
    // Potions (Red/Orange/Yellow/White)
    'IG_Potion': [501, 502, 503, 504],

    // Juices
    'IG_Juice': [531, 532, 533],

    // Candy
    'IG_Candy': [529, 530],

    // Food items (broad category)
    'IG_Food': [
        512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, // Fruits & meats
        523, 525, 526, 528, 529, 530, 531, 532, 533, 534, 535, 538 // Royal Jelly, Rice Cake, etc.
    ],

    // Recovery items (herbs, potions, special)
    'IG_Recovery': [
        501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, // Potions & herbs
        512, 526, 528 // Apple, Royal Jelly, Rice Cake
    ],
};

/**
 * Check if an item belongs to a specific group
 * @param {number} itemId
 * @param {string} groupName - e.g., 'IG_Potion'
 * @returns {boolean}
 */
function isItemInGroup(itemId, groupName) {
    const group = ITEM_GROUPS[groupName];
    return group ? group.includes(itemId) : false;
}

/**
 * Get all groups an item belongs to
 * @param {number} itemId
 * @returns {string[]} - array of group names
 */
function getItemGroups(itemId) {
    const groups = [];
    for (const [name, items] of Object.entries(ITEM_GROUPS)) {
        if (items.includes(itemId)) groups.push(name);
    }
    return groups;
}

/**
 * Pick a random item from a group
 * @param {string} groupName
 * @returns {number|null} - item ID or null if group not found
 */
function pickRandomFromGroup(groupName) {
    const group = ITEM_GROUPS[groupName];
    if (!group || group.length === 0) return null;
    return group[Math.floor(Math.random() * group.length)];
}

module.exports = {
    ITEM_GROUPS,
    isItemInGroup,
    getItemGroups,
    pickRandomFromGroup,
};
