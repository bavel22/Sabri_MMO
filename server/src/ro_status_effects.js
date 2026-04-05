/**
 * ro_status_effects.js — Generic Status Effect Engine for Sabri_MMO
 *
 * Handles all 10 RO Classic status effects: Stun, Freeze, Stone, Sleep,
 * Poison, Blind, Silence, Confusion, Bleeding, Curse.
 *
 * Provides: config definitions, resistance formulas, apply/remove/cleanse,
 * tick processing (periodic drains, expiry), and stat modifier aggregation.
 *
 * Usage in index.js:
 *   const { applyStatusEffect, removeStatusEffect, ... } = require('./ro_status_effects');
 *   const result = applyStatusEffect(source, target, 'stun', 50);
 *   if (result.applied) broadcastToZone(zone, 'status:applied', { ... });
 */

// ============================================================================
// STATUS EFFECT DEFINITIONS
// ============================================================================

const STATUS_EFFECTS = {
    stun: {
        resistStat: 'vit',
        resistCap: 97,
        baseDuration: 5000,
        canKill: false,
        breakOnDamage: false,
        preventsMovement: true,
        preventsCasting: true,
        preventsAttack: true,
        preventsItems: true,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: { fleeMultiplier: 0 }
    },
    freeze: {
        resistStat: 'mdef',
        resistCap: null,  // MDEF-based, no cap
        baseDuration: 12000,
        canKill: false,
        breakOnDamage: true,
        preventsMovement: true,
        preventsCasting: true,
        preventsAttack: true,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: { defMultiplier: 0.5, mdefMultiplier: 1.25 },
        elementOverride: { type: 'water', level: 1 }
    },
    stone: {
        resistStat: 'mdef',
        resistCap: null,
        baseDuration: 20000,
        canKill: false,
        breakOnDamage: true,
        preventsMovement: true,
        preventsCasting: true,
        preventsAttack: true,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: { defMultiplier: 0.5, mdefMultiplier: 1.25 },
        elementOverride: { type: 'earth', level: 1 },
        periodicDrain: {
            startDelay: 3000,   // 3s half-stone phase before drain starts
            interval: 5000,     // drain every 5s
            hpPercent: 0.01,    // 1% of max HP
            flatDrain: 0,
            minHpPercent: 0.25   // RO Classic: stone HP drain stops at 25% MaxHP
        }
    },
    sleep: {
        resistStat: 'int',
        resistCap: 97,
        baseDuration: 30000,
        canKill: false,
        breakOnDamage: true,
        preventsMovement: true,
        preventsCasting: true,
        preventsAttack: true,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {}
        // Special: attacks against sleeping targets always hit, 2x crit rate
    },
    ankle_snare: {
        resistStat: null,           // No resistance — always applies (rAthena: SCSTART_NORATEDEF)
        resistCap: null,
        baseDuration: 20000,
        canKill: false,
        breakOnDamage: false,       // NOT broken by damage (rAthena: no RemoveOnDamaged flag)
        preventsMovement: true,     // ONLY prevents movement
        preventsCasting: false,     // CAN cast while snared (rAthena status.yml: only NoMove)
        preventsAttack: false,      // CAN attack while snared
        preventsItems: false,       // CAN use items while snared
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {}
    },
    poison: {
        resistStat: 'vit',
        resistCap: 97,
        baseDuration: 60000,
        canKill: false,
        breakOnDamage: false,
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: true,
        statMods: { defMultiplier: 0.75 },
        periodicDrain: {
            startDelay: 0,
            interval: 1000,     // every 1s
            hpPercent: 0.015,   // 1.5% max HP
            flatDrain: 2        // +2 flat
            // canKill: false → floor is 1 HP (RO Classic: poison can't kill)
        }
    },
    blind: {
        resistStat: 'avg_int_vit',
        resistCap: 193,
        baseDuration: 30000,
        canKill: false,
        breakOnDamage: false,
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: { hitMultiplier: 0.75, fleeMultiplier: 0.75 }
    },
    silence: {
        resistStat: 'vit',
        resistCap: 97,
        baseDuration: 30000,
        canKill: false,
        breakOnDamage: false,
        preventsMovement: false,
        preventsCasting: true,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {}
    },
    confusion: {
        resistStat: 'avg_str_int',
        resistCap: 193,
        baseDuration: 30000,
        canKill: false,
        breakOnDamage: true,
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {}
        // Special: movement inputs are randomized on client
    },
    bleeding: {
        resistStat: 'vit',
        resistCap: 97,
        baseDuration: 120000,
        canKill: true,
        breakOnDamage: false,
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: true,
        blocksSPRegen: true,
        statMods: {},
        periodicDrain: {
            startDelay: 0,
            interval: 4000,     // every 4s
            hpPercent: 0.02,    // 2% max HP
            flatDrain: 0,
            minHpPercent: null   // canKill=true → can reach 0
        }
    },
    curse: {
        resistStat: 'luk',
        resistCap: 97,
        baseDuration: 30000,
        canKill: false,
        breakOnDamage: false,
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {
            atkMultiplier: 0.75,
            lukOverride: 0,
            moveSpeedMultiplier: 0.1
        }
    },
    petrifying: {
        resistStat: 'mdef',
        resistCap: null,
        baseDuration: 5000,      // Phase 1: 5 seconds
        canKill: false,
        breakOnDamage: false,    // NOT broken by damage (unlike most CC)
        preventsMovement: false, // Can still move during Phase 1
        preventsCasting: true,   // Cannot cast
        preventsAttack: true,    // Cannot attack
        preventsItems: false,    // Can use items
        blocksHPRegen: false,
        blocksSPRegen: false,
        statMods: {},
        transitionsTo: 'stone'   // Auto-transitions to full stone when Phase 1 expires
    }
};

// Damage-breakable status types
const BREAKABLE_STATUSES = ['freeze', 'stone', 'sleep', 'confusion'];

// ============================================================================
// HELPER: Get resistance stat value from target
// ============================================================================

function getResistStatValue(target, resistStat) {
    const stats = target.stats || target;

    switch (resistStat) {
        case 'vit':
            return stats.vit || 0;
        case 'int':
            return stats.int_stat || stats.int || 0;
        case 'luk':
            return stats.luk || 0;
        case 'mdef': {
            // Hard MDEF from equipment
            const hardMdef = target.hardMdef || target.equipmentMdef || 0;
            return hardMdef;
        }
        case 'avg_int_vit': {
            const intVal = stats.int_stat || stats.int || 0;
            const vitVal = stats.vit || 0;
            return Math.floor((intVal + vitVal) / 2);
        }
        case 'avg_str_int': {
            const strVal = stats.str || 0;
            const intVal = stats.int_stat || stats.int || 0;
            return Math.floor((strVal + intVal) / 2);
        }
        default:
            return 0;
    }
}

// ============================================================================
// RESISTANCE FORMULA
// ============================================================================

/**
 * Calculate whether a status effect lands and its duration.
 *
 * Formula:
 *   FinalChance = BaseChance - (BaseChance * ResistStat / 100) + srcLevel - tarLevel - tarLUK
 *   Duration = BaseDuration - (BaseDuration * ResistStat / 200) - 10 * tarLUK
 *
 * @param {Object} source - Caster (needs .level or .stats.level, .characterId or .id)
 * @param {Object} target - Target (needs .stats, .modeFlags, .activeStatusEffects)
 * @param {string} statusType - One of the STATUS_EFFECTS keys
 * @param {number} baseChance - Base % chance (0-100) before resistance
 * @returns {{ applied: boolean, duration?: number, chance?: number, reason?: string }}
 */
function calculateResistance(source, target, statusType, baseChance) {
    const config = STATUS_EFFECTS[statusType];
    if (!config) {
        return { applied: false, reason: 'unknown_status' };
    }

    // Boss/MVP immunity
    if (target.modeFlags && target.modeFlags.statusImmune) {
        return { applied: false, reason: 'boss_immune' };
    }

    // Already has this status — no stacking
    if (target.activeStatusEffects && target.activeStatusEffects.has(statusType)) {
        return { applied: false, reason: 'already_active' };
    }

    // Get resistance stat value
    const resistStatValue = getResistStatValue(target, config.resistStat);

    // LUK immunity: 300+ LUK = immune
    const targetLuk = (target.stats && (target.stats.luk || 0)) || 0;
    if (targetLuk >= 300) {
        return { applied: false, reason: 'luk_immune' };
    }

    // Source level
    const srcLevel = source.level || (source.stats && source.stats.level) || 1;
    const tarLevel = (target.stats && target.stats.level) || target.level || 1;

    // Phase 5: Card resistance (bResEff) — reduces chance before roll
    // cardResEff values are in rAthena 1/10000 scale (10000 = 100%)
    const cardResEff = (target.cardResEff && target.cardResEff[statusType]) || 0;
    let cardResistReduction = 0;
    if (cardResEff > 0) {
        cardResistReduction = Math.floor(baseChance * cardResEff / 10000);
    }

    // Buff-based status resist (Invulnerable Siegfried ensemble — reduces chance by statusResist%)
    let buffStatusResist = 0;
    if (target.activeBuffs) {
        const now = Date.now();
        for (const buff of target.activeBuffs) {
            if (now >= buff.expiresAt) continue;
            if (buff.statusResist) buffStatusResist += buff.statusResist;
        }
    }
    const buffResistReduction = buffStatusResist > 0 ? Math.floor(baseChance * buffStatusResist / 100) : 0;

    // Final chance calculation
    let finalChance = baseChance - (baseChance * resistStatValue / 100) + srcLevel - tarLevel - targetLuk - cardResistReduction - buffResistReduction;
    finalChance = Math.max(5, Math.min(95, finalChance));

    // Roll
    if (Math.random() * 100 >= finalChance) {
        return { applied: false, reason: 'resisted', chance: finalChance };
    }

    // Calculate duration with resistance reduction
    let duration = config.baseDuration - (config.baseDuration * resistStatValue / 200) - 10 * targetLuk;
    duration = Math.max(1000, duration); // Minimum 1 second

    return { applied: true, duration, chance: finalChance };
}

// ============================================================================
// APPLY / REMOVE / CLEANSE
// ============================================================================

/**
 * Apply a status effect to a target with resistance checks.
 *
 * @param {Object} source - The caster
 * @param {Object} target - The target
 * @param {string} statusType - Status effect key
 * @param {number} baseChance - Base chance (0-100)
 * @param {number} [overrideDuration] - Override calculated duration (ms)
 * @returns {{ applied: boolean, duration?: number, reason?: string }}
 */
function applyStatusEffect(source, target, statusType, baseChance, overrideDuration) {
    const result = calculateResistance(source, target, statusType, baseChance);
    if (!result.applied) return result;

    const duration = overrideDuration || result.duration;

    if (!target.activeStatusEffects) {
        target.activeStatusEffects = new Map();
    }

    const now = Date.now();
    target.activeStatusEffects.set(statusType, {
        type: statusType,
        appliedAt: now,
        duration: duration,
        expiresAt: now + duration,
        sourceId: source.characterId || source.id || 0,
        sourceLevel: source.level || (source.stats && source.stats.level) || 1,
        lastDrainTime: null
    });

    // Devotion break on CC: if target (Crusader) has devotionLinks and gets CC'd, break all links
    // rAthena: Devotion breaks when Crusader is stunned/frozen/petrified/put to sleep
    const CC_TYPES = ['stun', 'freeze', 'stone', 'sleep'];
    if (CC_TYPES.includes(statusType) && target.devotionLinks && target.devotionLinks.length > 0) {
        const brokenLinks = [...target.devotionLinks];
        target.devotionLinks = [];
        // Clear devotedTo on each linked target (objects are passed by reference)
        for (const link of brokenLinks) {
            // link.targetCharId references the protected player — we need to find them
            // The caller's connectedPlayers map isn't available here, so we flag for caller
        }
        return { applied: true, duration, devotionBroken: true, brokenLinks };
    }

    return { applied: true, duration };
}

/**
 * Force-apply a status effect bypassing resistance (for debug/special cases).
 */
function forceApplyStatusEffect(target, statusType, duration) {
    const config = STATUS_EFFECTS[statusType];
    if (!config) return { applied: false, reason: 'unknown_status' };

    if (!target.activeStatusEffects) {
        target.activeStatusEffects = new Map();
    }

    const effectDuration = duration || config.baseDuration;
    const now = Date.now();
    target.activeStatusEffects.set(statusType, {
        type: statusType,
        appliedAt: now,
        duration: effectDuration,
        expiresAt: now + effectDuration,
        sourceId: 0,
        sourceLevel: 1,
        lastDrainTime: null
    });

    return { applied: true, duration: effectDuration };
}

/**
 * Remove a specific status effect from a target.
 * @returns {boolean} Whether the effect was present and removed
 */
function removeStatusEffect(target, statusType) {
    if (!target.activeStatusEffects) return false;
    return target.activeStatusEffects.delete(statusType);
}

/**
 * Cleanse multiple status effects at once (for Cure, Recovery, etc.)
 * @param {Object} target
 * @param {string[]} statusTypes - Array of status type keys to remove
 * @returns {string[]} Array of types that were actually removed
 */
function cleanse(target, statusTypes) {
    const removed = [];
    for (const type of statusTypes) {
        if (removeStatusEffect(target, type)) {
            removed.push(type);
        }
    }
    return removed;
}

/**
 * Check and break damage-breakable statuses after dealing damage.
 * Call this after ANY damage is dealt to a target.
 *
 * Breakable: freeze, stone, sleep, confusion
 *
 * @param {Object} target
 * @returns {string[]} Array of broken status types
 */
function checkDamageBreakStatuses(target) {
    if (!target.activeStatusEffects || target.activeStatusEffects.size === 0) {
        return [];
    }

    const broken = [];
    for (const type of BREAKABLE_STATUSES) {
        if (target.activeStatusEffects.has(type)) {
            target.activeStatusEffects.delete(type);
            broken.push(type);
        }
    }
    return broken;
}

// ============================================================================
// TICK PROCESSING (called from 1s server tick)
// ============================================================================

/**
 * Process status effect ticks for a target.
 * Handles: expiry, periodic HP drains (poison, bleeding, stone).
 *
 * @param {Object} target - Player or enemy object (needs .health, .maxHealth, .activeStatusEffects)
 * @param {number} now - Current timestamp (Date.now())
 * @returns {{ expired: string[], drains: Array<{ type: string, drain: number, newHealth: number }> }}
 */
function tickStatusEffects(target, now) {
    if (!target.activeStatusEffects || target.activeStatusEffects.size === 0) {
        return { expired: [], drains: [] };
    }

    const expired = [];
    const drains = [];
    const transitions = []; // { from: 'petrifying', to: 'stone', duration: 20000 }

    for (const [type, effect] of target.activeStatusEffects.entries()) {
        // Expiry check
        if (now >= effect.expiresAt) {
            target.activeStatusEffects.delete(type);
            expired.push(type);

            // Auto-transition: petrifying → stone
            const config = STATUS_EFFECTS[type];
            if (config && config.transitionsTo) {
                const toType = config.transitionsTo;
                const toConfig = STATUS_EFFECTS[toType];
                if (toConfig && !target.activeStatusEffects.has(toType)) {
                    const toDuration = toConfig.baseDuration;
                    target.activeStatusEffects.set(toType, {
                        type: toType,
                        appliedAt: now,
                        duration: toDuration,
                        expiresAt: now + toDuration,
                        sourceId: effect.sourceId,
                        sourceLevel: effect.sourceLevel,
                        lastDrainTime: null
                    });
                    transitions.push({ from: type, to: toType, duration: toDuration });
                }
            }
            continue;
        }

        // Periodic drain
        const config = STATUS_EFFECTS[type];
        if (!config || !config.periodicDrain) continue;

        const pd = config.periodicDrain;
        const elapsed = now - effect.appliedAt;

        // Wait for start delay
        if (elapsed < pd.startDelay) continue;

        // Initialize lastDrainTime if not set
        if (!effect.lastDrainTime) {
            effect.lastDrainTime = effect.appliedAt + pd.startDelay;
        }

        // Check if interval has passed
        if (now - effect.lastDrainTime < pd.interval) continue;

        // Apply drain
        effect.lastDrainTime = now;
        let drain = Math.max(1, Math.floor((target.maxHealth || target.health || 100) * pd.hpPercent));
        if (pd.flatDrain) {
            drain += pd.flatDrain;
        }

        // Calculate minimum HP
        let minHp;
        if (config.canKill) {
            minHp = 0;
        } else if (pd.minHpPercent) {
            minHp = Math.floor((target.maxHealth || 100) * pd.minHpPercent);
        } else {
            minHp = 1;
        }

        const oldHealth = target.health;
        target.health = Math.max(minHp, target.health - drain);
        const actualDrain = oldHealth - target.health;

        if (actualDrain > 0) {
            drains.push({
                type,
                drain: actualDrain,
                newHealth: target.health
            });
        }
    }

    return { expired, drains, transitions };
}

// ============================================================================
// STATUS MODIFIER ACCESSOR
// ============================================================================

/**
 * Get aggregate stat modifiers from ALL active status effects on a target.
 * Used by getCombinedModifiers() in index.js to affect damage/combat calculations.
 *
 * @param {Object} target
 * @returns {Object} Modifier object with flags, multipliers, and overrides
 */
function getStatusModifiers(target) {
    const mods = {
        // Prevention flags
        preventsMovement: false,
        preventsCasting: false,
        preventsAttack: false,
        preventsItems: false,
        blocksHPRegen: false,
        blocksSPRegen: false,

        // Stat multipliers (multiplicative)
        defMultiplier: 1.0,
        mdefMultiplier: 1.0,
        atkMultiplier: 1.0,
        hitMultiplier: 1.0,
        fleeMultiplier: 1.0,
        moveSpeedMultiplier: 1.0,

        // Overrides
        lukOverride: null,
        overrideElement: null,

        // Individual status flags (for specific checks)
        isFrozen: false,
        isStoned: false,
        isPetrifying: false,
        isStunned: false,
        isSleeping: false,
        isPoisoned: false,
        isBleeding: false,
        isCursed: false,
        isBlind: false,
        isSilenced: false,
        isConfused: false
    };

    if (!target.activeStatusEffects || target.activeStatusEffects.size === 0) {
        return mods;
    }

    for (const [type, effect] of target.activeStatusEffects.entries()) {
        const config = STATUS_EFFECTS[type];
        if (!config) continue;

        // Skip expired (shouldn't happen if tick runs, but safety)
        if (Date.now() >= effect.expiresAt) continue;

        // Aggregate prevention flags
        if (config.preventsMovement) mods.preventsMovement = true;
        if (config.preventsCasting) mods.preventsCasting = true;
        if (config.preventsAttack) mods.preventsAttack = true;
        if (config.preventsItems) mods.preventsItems = true;
        if (config.blocksHPRegen) mods.blocksHPRegen = true;
        if (config.blocksSPRegen) mods.blocksSPRegen = true;

        // Aggregate stat multipliers
        const sm = config.statMods;
        if (sm.defMultiplier !== undefined) mods.defMultiplier *= sm.defMultiplier;
        if (sm.mdefMultiplier !== undefined) mods.mdefMultiplier *= sm.mdefMultiplier;
        if (sm.atkMultiplier !== undefined) mods.atkMultiplier *= sm.atkMultiplier;
        if (sm.hitMultiplier !== undefined) mods.hitMultiplier *= sm.hitMultiplier;
        if (sm.fleeMultiplier !== undefined) mods.fleeMultiplier *= sm.fleeMultiplier;
        if (sm.moveSpeedMultiplier !== undefined) mods.moveSpeedMultiplier *= sm.moveSpeedMultiplier;

        // Overrides
        if (sm.lukOverride !== undefined) mods.lukOverride = sm.lukOverride;
        if (config.elementOverride) mods.overrideElement = config.elementOverride;

        // Individual flags
        switch (type) {
            case 'freeze':      mods.isFrozen = true; break;
            case 'stone':       mods.isStoned = true; break;
            case 'petrifying':  mods.isPetrifying = true; break;
            case 'stun':        mods.isStunned = true; break;
            case 'sleep':     mods.isSleeping = true; break;
            case 'poison':    mods.isPoisoned = true; break;
            case 'bleeding':  mods.isBleeding = true; break;
            case 'curse':     mods.isCursed = true; break;
            case 'blind':     mods.isBlind = true; break;
            case 'silence':   mods.isSilenced = true; break;
            case 'confusion': mods.isConfused = true; break;
        }
    }

    return mods;
}

/**
 * Check if a target has a specific status effect active.
 */
function hasStatusEffect(target, statusType) {
    if (!target.activeStatusEffects) return false;
    const effect = target.activeStatusEffects.get(statusType);
    if (!effect) return false;
    return Date.now() < effect.expiresAt;
}

/**
 * Get all active status effects as a serializable array (for buff:list event).
 */
function getActiveStatusList(target) {
    if (!target.activeStatusEffects || target.activeStatusEffects.size === 0) {
        return [];
    }
    const now = Date.now();
    const list = [];
    for (const [type, effect] of target.activeStatusEffects.entries()) {
        if (now >= effect.expiresAt) continue;
        list.push({
            type,
            duration: effect.duration,
            remainingMs: effect.expiresAt - now
        });
    }
    return list;
}

// ============================================================================
// MODULE EXPORTS
// ============================================================================

module.exports = {
    STATUS_EFFECTS,
    BREAKABLE_STATUSES,
    calculateResistance,
    applyStatusEffect,
    forceApplyStatusEffect,
    removeStatusEffect,
    cleanse,
    checkDamageBreakStatuses,
    tickStatusEffects,
    getStatusModifiers,
    hasStatusEffect,
    getActiveStatusList
};
