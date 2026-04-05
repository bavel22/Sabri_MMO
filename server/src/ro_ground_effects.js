/**
 * Ragnarok Online Ground Effect System — Foundation Infrastructure
 * Reusable by: Wizard (Storm Gust, Meteor Storm, LoV, Ice Wall, Fire Pillar, Quagmire),
 *   Sage (Volcano, Deluge, Violent Gale, Land Protector), Priest (Sanctuary, Magnus Exorcismus),
 *   Hunter (all traps), Bard/Dancer (all songs/dances), Alchemist (Demonstration, Bio Cannibalize)
 *
 * Phase 0B — Foundation system. Individual skill handlers come later.
 */
'use strict';

// ============================================================
// Constants
// ============================================================
const CELL_SIZE = 50; // 1 RO cell = 50 UE units

const AOE_RADIUS = {
    '1x1': 25,
    '3x3': 75,
    '5x5': 125,
    '7x7': 175,
    '9x9': 225,
    '11x11': 275,
};

const GROUND_EFFECT_TICK_MS = 500;
const TRAP_CHECK_MS = 200;

// Ground effect type categories
const EFFECT_CATEGORY = {
    DAMAGE_ZONE:  'damage_zone',   // Storm Gust, Meteor Storm, LoV, Fire Pillar, Demonstration
    HEAL_ZONE:    'heal_zone',     // Sanctuary
    BUFF_ZONE:    'buff_zone',     // Volcano, Deluge, Violent Gale, songs/dances
    DEBUFF_ZONE:  'debuff_zone',   // Quagmire
    TRAP:         'trap',          // All Hunter traps
    OBSTACLE:     'obstacle',      // Ice Wall, Pneuma, Safety Wall
    PROTECTOR:    'protector',     // Land Protector (special — blocks other ground effects)
    CONTACT:      'contact',       // Fire Wall (hit-on-contact), Warp Portal
};

// Skills blocked by Land Protector
const LAND_PROTECTOR_BLOCKED = new Set([
    'safety_wall', 'pneuma', 'warp_portal', 'sanctuary', 'magnus_exorcismus',
    'volcano', 'deluge', 'violent_gale', 'fire_wall', 'fire_pillar',
    'thunderstorm', 'storm_gust', 'lord_of_vermilion', 'meteor_storm',
    'quagmire', 'ice_wall', 'frost_nova',
    // Priest duplicates
    'safety_wall_priest',
]);

// Per-caster limits: { skillName: maxCount }
const CASTER_LIMITS = {
    fire_wall: 3,
    safety_wall: 1,
    safety_wall_priest: 1,
    pneuma: 1,
    sanctuary: 1,
    ice_wall: 5,
    // Sage elemental zones: only 1 of each type, and only 1 across all three
    volcano: 1,
    deluge: 1,
    violent_gale: 1,
    land_protector: 1,
    // Traps: individual limits aren't enforced (limited by inventory + SP)
    // Songs: 1 active performance per performer
};

// Sage elemental zone mutual exclusion group
const SAGE_ZONE_GROUP = new Set(['volcano', 'deluge', 'violent_gale']);


// ============================================================
// Ground Effect Registry
// ============================================================
const activeGroundEffects = new Map(); // effectId → GroundEffect
let effectIdCounter = 0;

/**
 * @typedef {Object} GroundEffect
 * @property {number} id - Unique effect ID
 * @property {string} type - Internal skill name (e.g., 'sanctuary', 'volcano', 'land_mine')
 * @property {string} category - EFFECT_CATEGORY value
 * @property {number} skillId - Skill ID from ro_skill_data
 * @property {number} skillLevel - Cast level
 * @property {number} casterId - Character ID of caster
 * @property {string} casterName
 * @property {string} zone - Map zone ID
 * @property {number} centerX - Position (UE units)
 * @property {number} centerY
 * @property {number} centerZ
 * @property {number} radius - Effect radius (UE units)
 * @property {string} element - 'neutral'|'fire'|'water'|'wind'|'earth'|'holy'|'poison'|'ghost'|'undead'
 * @property {number} createdAt - Date.now() when created
 * @property {number} expiresAt - Date.now() + duration
 * @property {number} duration - Total duration (ms)
 * @property {number} tickInterval - ms between effect ticks (0 = no ticking, trigger only)
 * @property {number} lastTickTime - Last tick timestamp
 * @property {number} wavesTotal - Total waves (Storm Gust=10, LoV=4, etc.), 0=unlimited
 * @property {number} wavesSent - Waves emitted so far
 * @property {number} hitsRemaining - For Safety Wall / Fire Wall (charge-based), -1=unlimited
 * @property {number} maxTargetsPerTick - Max targets per tick (Sanctuary cap)
 * @property {Object} lastHitTargets - { targetKey: lastHitTimestamp } for immunity windows
 * @property {number} immunityWindowMs - Per-target immunity between hits (Magnus=3000ms)
 * @property {Object} data - Skill-specific extra data (damagePercent, statBonus, statusEffect, etc.)
 * @property {boolean} blockedByLP - Whether Land Protector nullifies this effect
 * @property {Function|null} onTick - Custom per-tick handler (called by tick loop)
 * @property {Function|null} onEnter - Custom handler when entity enters radius
 * @property {Function|null} onExpire - Custom handler when effect expires
 */

/**
 * Create a new ground effect and register it.
 * Returns the effect ID, or null if placement was blocked (e.g., by Land Protector).
 *
 * @param {Object} params - See GroundEffect typedef
 * @returns {number|null} effectId or null if blocked
 */
function createGroundEffect(params) {
    const {
        type, category, skillId, skillLevel, casterId, casterName, zone,
        centerX, centerY, centerZ, radius, element,
        duration, tickInterval, wavesTotal, hitsRemaining, maxTargetsPerTick,
        immunityWindowMs, data, onTick, onEnter, onExpire,
        blockedByLP,
    } = params;

    const now = Date.now();

    // --- Placement validation ---

    // 1. Land Protector blocks most ground effects
    if (blockedByLP !== false && LAND_PROTECTOR_BLOCKED.has(type)) {
        const blocking = getGroundEffectsAt(centerX, centerY, zone, 0)
            .find(e => e.category === EFFECT_CATEGORY.PROTECTOR && now < e.expiresAt);
        if (blocking) {
            return null; // Blocked by Land Protector
        }
    }

    // 2. Per-caster limits
    const limit = CASTER_LIMITS[type];
    if (limit !== undefined) {
        const existing = countEffects(casterId, type);
        if (existing >= limit) {
            // Remove oldest to make room
            removeOldestEffect(casterId, type);
        }
    }

    // 3. Sage elemental zone mutual exclusion (only 1 of volcano/deluge/violent_gale)
    if (SAGE_ZONE_GROUP.has(type)) {
        for (const sageType of SAGE_ZONE_GROUP) {
            if (sageType !== type) {
                removeAllEffects(casterId, sageType);
            }
        }
    }

    // 4. Land Protector creation: destroy all blockable effects in its area
    if (category === EFFECT_CATEGORY.PROTECTOR) {
        const overlapping = getGroundEffectsAt(centerX, centerY, zone, radius);
        for (const e of overlapping) {
            if (LAND_PROTECTOR_BLOCKED.has(e.type) && e.id) {
                removeGroundEffect(e.id);
            }
        }
    }

    // --- Create the effect ---
    const id = ++effectIdCounter;
    const effect = {
        id,
        type: type || 'unknown',
        category: category || EFFECT_CATEGORY.DAMAGE_ZONE,
        skillId: skillId || 0,
        skillLevel: skillLevel || 1,
        casterId: casterId || 0,
        casterName: casterName || '',
        zone: zone || 'prontera_south',
        centerX: centerX || 0,
        centerY: centerY || 0,
        centerZ: centerZ || 0,
        radius: radius || 125,
        element: element || 'neutral',
        createdAt: now,
        expiresAt: now + (duration || 30000),
        duration: duration || 30000,
        tickInterval: tickInterval || 0,
        lastTickTime: now,
        wavesTotal: wavesTotal || 0,
        wavesSent: 0,
        hitsRemaining: hitsRemaining !== undefined ? hitsRemaining : -1,
        maxTargetsPerTick: maxTargetsPerTick || 0, // 0 = unlimited
        lastHitTargets: {},
        immunityWindowMs: immunityWindowMs || 0,
        data: data || {},
        blockedByLP: blockedByLP !== false,
        onTick: onTick || null,
        onEnter: onEnter || null,
        onExpire: onExpire || null,
    };

    activeGroundEffects.set(id, effect);
    return id;
}

/**
 * Remove a ground effect by ID.
 * @param {number} effectId
 * @returns {Object|null} The removed effect, or null
 */
function removeGroundEffect(effectId) {
    const effect = activeGroundEffects.get(effectId);
    if (effect) {
        if (effect.onExpire) {
            try { effect.onExpire(effect); } catch (e) { /* ignore */ }
        }
        activeGroundEffects.delete(effectId);
    }
    return effect || null;
}

/**
 * Get all active ground effects at a position within a search radius.
 * @param {number} x
 * @param {number} y
 * @param {string} zone
 * @param {number} searchRadius - Extra search radius (0 = point query, checks effect.radius)
 * @returns {Array<Object>}
 */
function getGroundEffectsAt(x, y, zone, searchRadius) {
    const now = Date.now();
    const results = [];
    for (const [, effect] of activeGroundEffects.entries()) {
        if (effect.zone !== zone) continue;
        if (now >= effect.expiresAt) continue;
        const dx = x - effect.centerX;
        const dy = y - effect.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        // Entity at (x,y) is inside this effect if dist <= effect.radius
        // OR if we're doing a broad search (searchRadius > 0) for overlapping effects
        if (dist <= effect.radius + searchRadius) {
            results.push(effect);
        }
    }
    return results;
}

/**
 * Get all ground effects in a zone.
 * @param {string} zone
 * @returns {Array<Object>}
 */
function getGroundEffectsInZone(zone) {
    const results = [];
    const now = Date.now();
    for (const [, effect] of activeGroundEffects.entries()) {
        if (effect.zone === zone && now < effect.expiresAt) {
            results.push(effect);
        }
    }
    return results;
}

/**
 * Count active effects by caster and type.
 */
function countEffects(casterId, type) {
    let count = 0;
    const now = Date.now();
    for (const [, effect] of activeGroundEffects.entries()) {
        if (effect.casterId === casterId && effect.type === type && now < effect.expiresAt) {
            count++;
        }
    }
    return count;
}

/**
 * Remove oldest effect of a given type by a caster.
 * @returns {number|null} Removed effect ID, or null
 */
function removeOldestEffect(casterId, type) {
    let oldest = null;
    let oldestId = null;
    for (const [id, effect] of activeGroundEffects.entries()) {
        if (effect.casterId === casterId && effect.type === type) {
            if (!oldest || effect.createdAt < oldest.createdAt) {
                oldest = effect;
                oldestId = id;
            }
        }
    }
    if (oldestId) {
        return removeGroundEffect(oldestId);
    }
    return null;
}

/**
 * Remove all effects of a given type by a caster.
 * @returns {number} Count removed
 */
function removeAllEffects(casterId, type) {
    const toRemove = [];
    for (const [id, effect] of activeGroundEffects.entries()) {
        if (effect.casterId === casterId && effect.type === type) {
            toRemove.push(id);
        }
    }
    for (const id of toRemove) {
        removeGroundEffect(id);
    }
    return toRemove.length;
}

/**
 * Remove all ground effects in a zone (for zone cleanup/reset).
 * @param {string} zone
 * @returns {number} Count removed
 */
function cleanupZone(zone) {
    const toRemove = [];
    for (const [id, effect] of activeGroundEffects.entries()) {
        if (effect.zone === zone) {
            toRemove.push(id);
        }
    }
    for (const id of toRemove) {
        activeGroundEffects.delete(id);
    }
    return toRemove.length;
}

/**
 * Check if a position is blocked by Land Protector for a given skill type.
 * @param {number} x
 * @param {number} y
 * @param {string} zone
 * @param {string} skillType - Internal skill name
 * @returns {boolean}
 */
function isBlockedByLandProtector(x, y, zone, skillType) {
    if (!LAND_PROTECTOR_BLOCKED.has(skillType)) return false;
    const now = Date.now();
    for (const [, effect] of activeGroundEffects.entries()) {
        if (effect.zone !== zone) continue;
        if (effect.category !== EFFECT_CATEGORY.PROTECTOR) continue;
        if (now >= effect.expiresAt) continue;
        const dx = x - effect.centerX;
        const dy = y - effect.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist <= effect.radius) return true;
    }
    return false;
}

/**
 * Check if a target has immunity to a ground effect (per-target cooldown).
 * @param {Object} effect - The ground effect
 * @param {string} targetKey - e.g. 'enemy_123' or 'player_456'
 * @returns {boolean} true if immune (should skip)
 */
function hasImmunity(effect, targetKey) {
    if (effect.immunityWindowMs <= 0) return false;
    const lastHit = effect.lastHitTargets[targetKey];
    if (!lastHit) return false;
    return (Date.now() - lastHit) < effect.immunityWindowMs;
}

/**
 * Record a hit on a target for immunity tracking.
 * @param {Object} effect
 * @param {string} targetKey
 */
function recordHit(effect, targetKey) {
    effect.lastHitTargets[targetKey] = Date.now();
}

/**
 * Find all entities (enemies + players) inside a ground effect's radius.
 * Returns { enemies: [{id, enemy}], players: [{id, player}] }
 *
 * @param {Object} effect - Ground effect object
 * @param {Map} enemies - Server enemies Map
 * @param {Map} connectedPlayers - Server connectedPlayers Map
 * @param {Function} getPlayerPosition - Async function to get player position
 * @returns {{ enemies: Array<{id: number, entity: Object}>, players: Array<{id: number, entity: Object}> }}
 */
function findEntitiesInEffect(effect, enemies, connectedPlayers, getPlayerPosition) {
    const results = { enemies: [], players: [] };

    // Check enemies (synchronous — enemy positions are in-memory)
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead) continue;
        if (enemy.zone !== effect.zone) continue;
        const dx = enemy.x - effect.centerX;
        const dy = enemy.y - effect.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist <= effect.radius) {
            results.enemies.push({ id: eid, entity: enemy });
        }
    }

    // Check players (synchronous using cached positions)
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if ((player.zone || 'prontera_south') !== effect.zone) continue;
        const px = player.lastX || 0;
        const py = player.lastY || 0;
        const dx = px - effect.centerX;
        const dy = py - effect.centerY;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist <= effect.radius) {
            results.players.push({ id: charId, entity: player });
        }
    }

    return results;
}

/**
 * Process the ground effect tick loop. Called from index.js setInterval.
 * Handles: expiration, wave tracking, custom onTick callbacks.
 *
 * @param {Object} context - { enemies, connectedPlayers, broadcastToZone, io, logger, ... }
 * @returns {{ expired: number[], ticked: number[] }}
 */
function processGroundEffectTick(context) {
    const now = Date.now();
    const expired = [];
    const ticked = [];

    for (const [effectId, effect] of activeGroundEffects.entries()) {
        // 1. Check expiration
        if (now >= effect.expiresAt) {
            expired.push(effectId);
            continue;
        }

        // 2. Check charges exhausted
        if (effect.hitsRemaining === 0) {
            expired.push(effectId);
            continue;
        }

        // 3. Check wave limit
        if (effect.wavesTotal > 0 && effect.wavesSent >= effect.wavesTotal) {
            expired.push(effectId);
            continue;
        }

        // 4. Tick-based processing
        if (effect.tickInterval > 0 && (now - effect.lastTickTime) >= effect.tickInterval) {
            effect.lastTickTime = now;
            effect.wavesSent++;

            // Custom tick handler (skill-specific logic lives here)
            if (effect.onTick) {
                try {
                    effect.onTick(effect, context);
                } catch (e) {
                    if (context.logger) context.logger.error(`[GROUND-EFFECT] onTick error for ${effect.type}#${effectId}: ${e.message}`);
                }
            }

            ticked.push(effectId);
        }
    }

    // Remove expired effects and broadcast
    for (const id of expired) {
        const effect = activeGroundEffects.get(id);
        if (effect) {
            const reason = effect.hitsRemaining === 0 ? 'hits_exhausted'
                : (effect.wavesTotal > 0 && effect.wavesSent >= effect.wavesTotal) ? 'waves_complete'
                : 'expired';

            if (effect.onExpire) {
                try { effect.onExpire(effect, context); } catch (e) { /* ignore */ }
            }

            activeGroundEffects.delete(id);

            if (context.broadcastToZone) {
                context.broadcastToZone(effect.zone, 'ground:effect_expired', {
                    effectId: id,
                    type: effect.type,
                    skillId: effect.skillId,
                    reason,
                });
            }
        }
    }

    return { expired, ticked };
}

/**
 * Check if a trap at a given position should be triggered by an entity.
 * Traps trigger when an entity enters within triggerRadius (default 1 cell = 50 UE).
 *
 * @param {Object} effect - Must have category === TRAP
 * @param {number} entityX
 * @param {number} entityY
 * @returns {boolean}
 */
function shouldTrapTrigger(effect, entityX, entityY) {
    if (effect.category !== EFFECT_CATEGORY.TRAP) return false;
    if (effect.data.isTriggered) return false;
    const triggerRadius = effect.data.triggerRadius || CELL_SIZE;
    const dx = entityX - effect.centerX;
    const dy = entityY - effect.centerY;
    return Math.sqrt(dx * dx + dy * dy) <= triggerRadius;
}

/**
 * Mark a trap as triggered.
 */
function triggerTrap(effect) {
    if (effect.category === EFFECT_CATEGORY.TRAP) {
        effect.data.isTriggered = true;
    }
}

/**
 * Get the active ground effects Map (for direct access from index.js).
 */
function getActiveEffects() {
    return activeGroundEffects;
}

/**
 * Get an effect by ID.
 */
function getEffect(effectId) {
    return activeGroundEffects.get(effectId) || null;
}


// ============================================================
// Trap damage formula (MISC type — ignores DEF/MDEF/FLEE)
// ============================================================

/**
 * Calculate MISC-type trap damage (Hunter traps).
 * Ignores DEF, MDEF, FLEE, Cards, Size Penalty. Cannot miss, cannot crit.
 * Subject to element modifier table only.
 *
 * @param {string} trapType - 'land_mine', 'blast_mine', 'claymore_trap', 'freezing_trap'
 * @param {number} skillLevel
 * @param {number} dex - Caster DEX
 * @param {number} int - Caster INT
 * @param {number} elementModifier - Element table modifier (percentage, e.g. 100 = neutral, 200 = 2x)
 * @returns {number} Final damage
 */
function calculateTrapDamage(trapType, skillLevel, dex, int, elementModifier) {
    let baseDamage = 0;
    switch (trapType) {
        case 'land_mine':
            baseDamage = skillLevel * (dex + 75) * (1 + int / 100);
            break;
        case 'blast_mine':
            baseDamage = skillLevel * (50 + Math.floor(dex / 2)) * (1 + int / 100);
            break;
        case 'claymore_trap':
            baseDamage = skillLevel * (75 + Math.floor(dex / 2)) * (1 + int / 100);
            break;
        // NOTE: freezing_trap removed — uses ATK-based Weapon pipeline (rAthena Type:Weapon), not MISC
        default:
            baseDamage = skillLevel * dex;
    }

    // Apply element modifier and ±10% variance
    const variance = 0.9 + Math.random() * 0.2; // 0.9 to 1.1
    return Math.max(1, Math.floor(baseDamage * (elementModifier / 100) * variance));
}


// ============================================================
// Exports
// ============================================================
module.exports = {
    // Constants
    CELL_SIZE,
    AOE_RADIUS,
    GROUND_EFFECT_TICK_MS,
    TRAP_CHECK_MS,
    EFFECT_CATEGORY,
    LAND_PROTECTOR_BLOCKED,
    CASTER_LIMITS,
    SAGE_ZONE_GROUP,

    // Registry CRUD
    createGroundEffect,
    removeGroundEffect,
    getEffect,
    getActiveEffects,

    // Queries
    getGroundEffectsAt,
    getGroundEffectsInZone,
    countEffects,
    isBlockedByLandProtector,
    findEntitiesInEffect,

    // Cleanup
    removeOldestEffect,
    removeAllEffects,
    cleanupZone,

    // Tick processing
    processGroundEffectTick,

    // Immunity
    hasImmunity,
    recordHit,

    // Traps
    shouldTrapTrigger,
    triggerTrap,
    calculateTrapDamage,
};
