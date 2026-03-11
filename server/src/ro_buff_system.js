/**
 * ro_buff_system.js — Generic Buff System for Sabri_MMO
 *
 * Handles positive buffs (Blessing, Increase AGI, Endure, Sight, etc.)
 * and debuffs (Provoke) that modify stats via the activeBuffs array.
 *
 * Distinction from ro_status_effects.js:
 *   - Status effects: 10 core CC/DoT conditions (stun, freeze, poison, etc.)
 *     stored in target.activeStatusEffects Map
 *   - Buffs: Stat-modifying skills with duration stored in target.activeBuffs array
 *
 * Usage in index.js:
 *   const { applyBuff, expireBuffs, getBuffModifiers, ... } = require('./ro_buff_system');
 */

// ============================================================================
// BUFF TYPE DEFINITIONS
// ============================================================================

const BUFF_TYPES = {
    // -- Existing buffs (already in game) --
    provoke: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Provoke',
        abbrev: 'PRV'
    },
    endure: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Endure',
        abbrev: 'END'
    },
    sight: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Sight',
        abbrev: 'SGT'
    },

    // -- Future buffs (Phase 3: Acolyte/support skills) --
    blessing: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Blessing',
        abbrev: 'BLS'
    },
    increase_agi: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Increase AGI',
        abbrev: 'AGI'
    },
    decrease_agi: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Decrease AGI',
        abbrev: 'DAG'
    },
    angelus: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Angelus',
        abbrev: 'ANG'
    },
    pneuma: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Pneuma',
        abbrev: 'PNE'
    },
    signum_crucis: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Signum Crucis',
        abbrev: 'SCR'
    },

    // -- Future buffs (Phase 3: other classes) --
    auto_berserk: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Auto Berserk',
        abbrev: 'BRK'
    },
    hiding: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Hiding',
        abbrev: 'HID'
    },
    improve_concentration: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Concentration',
        abbrev: 'CON'
    },

    // -- Future buffs (Phase 6: second classes) --
    two_hand_quicken: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Two-Hand Quicken',
        abbrev: 'THQ'
    },
    kyrie_eleison: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Kyrie Eleison',
        abbrev: 'KYR'
    },
    magnificat: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Magnificat',
        abbrev: 'MAG'
    },
    gloria: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Gloria',
        abbrev: 'GLO'
    },
    lex_aeterna: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Lex Aeterna',
        abbrev: 'LEX'
    },
    aspersio: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Aspersio',
        abbrev: 'ASP'
    },
    energy_coat: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Energy Coat',
        abbrev: 'ENC'
    },
    enchant_poison: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Enchant Poison',
        abbrev: 'EPO'
    },
    cloaking: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Cloaking',
        abbrev: 'CLK'
    },
    quagmire: {
        stackRule: 'refresh',
        category: 'debuff',
        displayName: 'Quagmire',
        abbrev: 'QUA'
    },
    loud_exclamation: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Loud Exclamation',
        abbrev: 'LXC'
    },
    ruwach: {
        stackRule: 'refresh',
        category: 'buff',
        displayName: 'Ruwach',
        abbrev: 'RUW'
    }
};

// stackRule options:
//   'refresh' — replace existing buff of same name (most common)
//   'stack'   — allow multiple instances (rare)
//   'reject'  — do not overwrite if already active

// ============================================================================
// APPLY / REMOVE / EXPIRE
// ============================================================================

/**
 * Apply a buff to a target.
 *
 * @param {Object} target - Player or enemy (needs .activeBuffs array)
 * @param {Object} buffDef - Buff definition with at minimum:
 *   { name, skillId, casterId, casterName, duration }
 *   Plus optional stat fields: defReduction, atkIncrease, mdefBonus, etc.
 * @returns {boolean} true if applied, false if rejected
 */
function applyBuff(target, buffDef) {
    if (!target.activeBuffs) target.activeBuffs = [];

    const config = BUFF_TYPES[buffDef.name];
    const stackRule = (config && config.stackRule) || 'refresh';

    if (stackRule === 'refresh') {
        // Remove existing buff of same name
        target.activeBuffs = target.activeBuffs.filter(b => b.name !== buffDef.name);
    } else if (stackRule === 'reject') {
        if (target.activeBuffs.some(b => b.name === buffDef.name)) {
            return false;
        }
    }
    // 'stack' allows duplicates — no filtering

    const now = Date.now();
    target.activeBuffs.push({
        ...buffDef,
        appliedAt: now,
        expiresAt: now + buffDef.duration
    });

    return true;
}

/**
 * Remove a specific buff by name.
 * @returns {Object|null} The removed buff object, or null if not found
 */
function removeBuff(target, buffName) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return null;

    const idx = target.activeBuffs.findIndex(b => b.name === buffName);
    if (idx === -1) return null;

    const removed = target.activeBuffs[idx];
    target.activeBuffs.splice(idx, 1);
    return removed;
}

/**
 * Remove expired buffs and return them.
 * @returns {Object[]} Array of expired buff objects
 */
function expireBuffs(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return [];

    const now = Date.now();
    const expired = target.activeBuffs.filter(b => now >= b.expiresAt);
    target.activeBuffs = target.activeBuffs.filter(b => now < b.expiresAt);
    return expired;
}

/**
 * Check if a target has a specific buff active.
 */
function hasBuff(target, buffName) {
    if (!target.activeBuffs) return false;
    const now = Date.now();
    return target.activeBuffs.some(b => b.name === buffName && now < b.expiresAt);
}

// ============================================================================
// BUFF MODIFIER ACCESSOR
// ============================================================================

/**
 * Get aggregate stat modifiers from ALL active buffs on a target.
 *
 * NOTE: This does NOT include status effects (freeze/stone/stun/etc.)
 *       Those are handled by getStatusModifiers() in ro_status_effects.js.
 *       Use getCombinedModifiers() in index.js for the merged result.
 *
 * @param {Object} target
 * @returns {Object} Modifier object with multipliers and flat bonuses
 */
function getBuffModifiers(target) {
    const mods = {
        // Multiplicative modifiers
        defMultiplier: 1.0,
        atkMultiplier: 1.0,
        aspdMultiplier: 1.0,

        // Flat additive bonuses
        bonusMDEF: 0,
        strBonus: 0,
        agiBonus: 0,
        vitBonus: 0,
        intBonus: 0,
        dexBonus: 0,
        lukBonus: 0,
        defPercent: 0,       // Angelus: +VIT% defense
        moveSpeedBonus: 0,   // Increase AGI, etc.
        bonusHit: 0,
        bonusFlee: 0,
        bonusCritical: 0,
        bonusMaxHp: 0,
        bonusMaxSp: 0,

        // Special flags
        weaponElement: null,  // Aspersio, Enchant Poison, etc.
        isHidden: false,      // Hiding, Cloaking
        doubleNextDamage: false, // Lex Aeterna
        blockRanged: false,   // Pneuma
    };

    if (!target.activeBuffs || target.activeBuffs.length === 0) {
        return mods;
    }

    const now = Date.now();
    for (const buff of target.activeBuffs) {
        if (now >= buff.expiresAt) continue;

        switch (buff.name) {
            case 'provoke':
                mods.defMultiplier *= (1 - (buff.defReduction || 0) / 100);
                mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
                break;

            case 'endure':
                mods.bonusMDEF += (buff.mdefBonus || 0);
                break;

            case 'blessing':
                mods.strBonus += (buff.strBonus || 0);
                mods.dexBonus += (buff.dexBonus || 0);
                mods.intBonus += (buff.intBonus || 0);
                break;

            case 'increase_agi':
                mods.agiBonus += (buff.agiBonus || 0);
                mods.moveSpeedBonus += (buff.moveSpeedBonus || 0);
                break;

            case 'decrease_agi':
                mods.agiBonus -= (buff.agiReduction || 0);
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 0);
                break;

            case 'angelus':
                mods.defPercent += (buff.defPercent || 0);
                break;

            case 'gloria':
                mods.lukBonus += (buff.lukBonus || 0);
                break;

            case 'improve_concentration':
                mods.agiBonus += (buff.agiBonus || 0);
                mods.dexBonus += (buff.dexBonus || 0);
                break;

            case 'two_hand_quicken':
                mods.aspdMultiplier *= (1 + (buff.aspdIncrease || 0) / 100);
                break;

            case 'lex_aeterna':
                mods.doubleNextDamage = true;
                break;

            case 'aspersio':
                mods.weaponElement = 'holy';
                break;

            case 'enchant_poison':
                mods.weaponElement = 'poison';
                break;

            case 'pneuma':
                mods.blockRanged = true;
                break;

            case 'hiding':
            case 'cloaking':
                mods.isHidden = true;
                break;

            case 'quagmire':
                mods.agiBonus -= (buff.agiReduction || 0);
                mods.dexBonus -= (buff.dexReduction || 0);
                mods.moveSpeedBonus -= (buff.moveSpeedReduction || 0);
                break;

            case 'auto_berserk':
                mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
                if ((buff.atkIncrease || 0) > 0) mods.defMultiplier *= 0.75;
                break;

            case 'energy_coat':
                mods.defPercent += (buff.defPercent || 0);
                break;

            case 'loud_exclamation':
                mods.strBonus += (buff.strBonus || 0);
                break;

            case 'signum_crucis':
                mods.defMultiplier *= (1 - (buff.defReduction || 0) / 100);
                break;

            case 'ruwach':
                // Presence only — reveal check done separately
                break;

            // Generic fallback: check for common stat fields
            default:
                if (buff.defReduction) mods.defMultiplier *= (1 - buff.defReduction / 100);
                if (buff.atkIncrease) mods.atkMultiplier *= (1 + buff.atkIncrease / 100);
                if (buff.mdefBonus) mods.bonusMDEF += buff.mdefBonus;
                break;
        }
    }

    return mods;
}

/**
 * Get all active buffs as a serializable array (for buff:list event).
 */
function getActiveBuffList(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) {
        return [];
    }
    const now = Date.now();
    return target.activeBuffs
        .filter(b => now < b.expiresAt)
        .map(b => {
            const config = BUFF_TYPES[b.name];
            return {
                name: b.name,
                skillId: b.skillId,
                duration: b.duration,
                remainingMs: b.expiresAt - now,
                category: (config && config.category) || 'buff',
                displayName: (config && config.displayName) || b.name,
                abbrev: (config && config.abbrev) || b.name.substring(0, 3).toUpperCase()
            };
        });
}

// ============================================================================
// MODULE EXPORTS
// ============================================================================

module.exports = {
    BUFF_TYPES,
    applyBuff,
    removeBuff,
    expireBuffs,
    hasBuff,
    getBuffModifiers,
    getActiveBuffList
};
