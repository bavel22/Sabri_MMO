const express = require('express');
const cors = require('cors');
const { Pool } = require('pg');
const bcrypt = require('bcrypt');
const jwt = require('jsonwebtoken');
const rateLimit = require('express-rate-limit');
const fs = require('fs');
const path = require('path');
const redis = require('redis');
const { Server } = require('socket.io');
require('dotenv').config();

// Import test mode for UI testing
const testModeRouter = require('./test_mode');

// Import RO monster templates (509 monsters from rAthena pre-renewal database)
const { RO_MONSTER_TEMPLATES } = require('./ro_monster_templates');
// Import RO item name → ID mapping + existing item extra drops config
const { RO_ITEM_NAME_TO_ID, EXISTING_ITEM_EXTRA_DROPS } = require('./ro_item_mapping');
// Import RO EXP tables (base levels 1-99, job levels for novice/1st/2nd class)
const {
    BASE_EXP_TABLE, JOB_CLASS_CONFIG, MAX_BASE_LEVEL,
    FIRST_CLASSES, SECOND_CLASS_UPGRADES,
    getStatPointsForLevelUp, getSkillPointsForJobLevelUp,
    getBaseExpForNextLevel, getJobExpForNextLevel,
    getMaxJobLevel, getClassTier
} = require('./ro_exp_tables');
// Import RO skill definitions (all classes, prerequisites, SP costs)
const {
    ALL_SKILLS, SKILL_MAP, CLASS_SKILLS, CLASS_PROGRESSION,
    getAvailableSkills, canLearnSkill
} = require('./ro_skill_data');
// Import RO pre-renewal damage formulas (element table, size penalty, HIT/FLEE, critical, DEF)
const {
    ELEMENT_TABLE, SIZE_PENALTY,
    calculateDerivedStats: roDerivedStats,
    getElementModifier, getSizePenalty,
    calculateHitRate, calculateCritRate,
    calculatePhysicalDamage: roPhysicalDamage,
    calculateMagicalDamage: roMagicalDamage
} = require('./ro_damage_formulas');

// Setup file logging
const logsDir = path.join(__dirname, '..', 'logs');
if (!fs.existsSync(logsDir)) {
    fs.mkdirSync(logsDir, { recursive: true });
}
const logFile = fs.createWriteStream(path.join(logsDir, 'server.log'), { flags: 'a' });

// Log levels: DEBUG, INFO, WARN, ERROR
const LOG_LEVEL = process.env.LOG_LEVEL || 'INFO';
const LOG_LEVELS = { DEBUG: 0, INFO: 1, WARN: 2, ERROR: 3 };

function shouldLog(level) {
    return LOG_LEVELS[level] >= LOG_LEVELS[LOG_LEVEL];
}

function formatLog(level, message) {
    return `[${new Date().toISOString()}] [${level}] ${message}`;
}

const logger = {
    debug: (...args) => {
        if (shouldLog('DEBUG')) {
            const msg = formatLog('DEBUG', args.join(' '));
            console.log(msg);
            logFile.write(msg + '\n');
        }
    },
    info: (...args) => {
        if (shouldLog('INFO')) {
            const msg = formatLog('INFO', args.join(' '));
            console.log(msg);
            logFile.write(msg + '\n');
        }
    },
    warn: (...args) => {
        if (shouldLog('WARN')) {
            const msg = formatLog('WARN', args.join(' '));
            console.warn(msg);
            logFile.write(msg + '\n');
        }
    },
    error: (...args) => {
        if (shouldLog('ERROR')) {
            const msg = formatLog('ERROR', args.join(' '));
            console.error(msg);
            logFile.write(msg + '\n');
        }
    }
};

const app = express();
const PORT = process.env.PORT || 3000;

// Create HTTP server for Socket.io
const http = require('http');
const server = http.createServer(app);

// Setup Socket.io
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

// Store connected players
const connectedPlayers = new Map();

// PvP toggle — set to false to disable all player-vs-player damage
const PVP_ENABLED = false;

// Combat constants (Ragnarok Online-style)
const COMBAT = {
    BASE_DAMAGE: 1,             // Default damage per hit (before stat formulas)
    DAMAGE_VARIANCE: 0,         // Random variance added to base damage
    MELEE_RANGE: 150,           // Default melee attack range (Unreal units)
    RANGED_RANGE: 800,          // Default ranged attack range
    DEFAULT_ASPD: 175,          // Default attack speed (0-195 scale, higher = faster)
    ASPD_CAP: 195,              // Hard cap — above this, diminishing returns apply up to 199
    RANGE_TOLERANCE: 50,         // Extra units added to out_of_range check so client moves closer than max range
    COMBAT_TICK_MS: 50,         // Server combat tick interval (ms)
    RESPAWN_DELAY_MS: 5000,
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }
};

// ASPD potion/consumable definitions (temporary ASPD boosts)
// Duration in ms, aspdBonus added directly to final ASPD before interval calc
const SPEED_TONICS = {
    'veil_draught':   { duration: 60000,  aspdBonus: 5 },  // Short burst, big bonus
    'dusk_tincture':  { duration: 180000, aspdBonus: 3 },  // Sustained, smaller bonus
    'ember_salve':    { duration: 30000,  aspdBonus: 8 },  // Very short, large bonus
    'grey_catalyst':  { duration: 300000, aspdBonus: 2 },  // Long duration, minor bonus
};

// Calculate attack interval from ASPD (ms between attacks)
// ASPD 175 = 1250ms, ASPD 185 = 750ms, ASPD 195 = 250ms
// Above 195: exponential diminishing returns — max theoretical ~217ms at ASPD 199
function getAttackIntervalMs(aspd) {
    if (aspd <= COMBAT.ASPD_CAP) {
        // Linear formula up to hard cap (195)
        return (200 - aspd) * 50; // e.g. ASPD 195 → 5 * 50 = 250ms
    } else {
        // Diminishing returns above 195: exponential decay, bottoms out ~217ms at 199
        const excessAspd = Math.min(aspd - COMBAT.ASPD_CAP, 9); // max 9 excess points (196-199 useful range)
        const decayFactor = Math.exp(-excessAspd * 0.35);
        const maxBonus = 130; // Maximum ms reduction above 195 (250ms → ~120ms floor)
        const actualBonus = Math.floor(maxBonus * (1 - decayFactor));
        return Math.max(217, 250 - actualBonus); // Floor ~217ms (~4.6 attacks/sec absolute max)
    }
}

// Auto-attack state tracking: Map<attackerCharId, { targetCharId, isEnemy, startTime }>
const autoAttackState = new Map();

// Cast time system (RO pre-renewal)
// activeCasts: characterId → { skillId, targetId, isEnemy, learnedLevel, levelData, skill, castStartTime, castEndTime, actualCastTime, socketId }
const activeCasts = new Map();
// afterCastDelayEnd: characterId → timestamp when ACD expires (global skill lockout)
const afterCastDelayEnd = new Map();

// Calculate actual cast time after DEX reduction (RO pre-renewal formula)
// ActualCastTime = BaseCastTime × (1 - DEX/150)
// DEX >= 150 → instant cast
function calculateActualCastTime(baseCastTime, casterDex) {
    if (baseCastTime <= 0) return 0;
    if (casterDex >= 150) return 0;
    return Math.max(0, Math.floor(baseCastTime * (1 - casterDex / 150)));
}

// Interrupt a player's active cast (called when they take damage or move)
function interruptCast(characterId, reason) {
    const cast = activeCasts.get(characterId);
    if (!cast) return;
    activeCasts.delete(characterId);
    const sock = io.sockets.sockets.get(cast.socketId);
    if (sock) sock.emit('skill:cast_interrupted', { skillId: cast.skillId, reason });
    io.emit('skill:cast_interrupted_broadcast', { casterId: characterId, skillId: cast.skillId });
    logger.info(`[CAST] ${cast.casterName}'s ${cast.skill.displayName} interrupted (${reason})`);
}

// ============================================================
// ============================================================
// Ground Effects System (Fire Wall, Safety Wall)
// ============================================================
const activeGroundEffects = new Map(); // effectId → { type, casterId, x, y, z, ... }
let groundEffectIdCounter = 0;

function createGroundEffect(def) {
    const id = ++groundEffectIdCounter;
    activeGroundEffects.set(id, { ...def, id, createdAt: Date.now(), expiresAt: Date.now() + def.duration });
    return id;
}

function removeGroundEffect(id) {
    activeGroundEffects.delete(id);
}

function getGroundEffectsAtPosition(x, y, z, radius) {
    const effects = [];
    for (const [id, effect] of activeGroundEffects.entries()) {
        if (Date.now() >= effect.expiresAt) continue;
        const dx = x - effect.x;
        const dy = y - effect.y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist <= radius) effects.push(effect);
    }
    return effects;
}

// Count active ground effects by type and caster
function countGroundEffects(casterId, type) {
    let count = 0;
    for (const [, effect] of activeGroundEffects.entries()) {
        if (effect.casterId === casterId && effect.type === type && Date.now() < effect.expiresAt) count++;
    }
    return count;
}

// Remove oldest ground effect of a type by caster
function removeOldestGroundEffect(casterId, type) {
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
    if (oldestId) activeGroundEffects.delete(oldestId);
    return oldestId;
}

// ============================================================
// Skill Combat System — Passives, Buffs, Cooldowns, HP Regen
// ============================================================

// Get passive skill bonuses for a player (Sword Mastery, 2H Sword Mastery, HP Recovery)
function getPassiveSkillBonuses(player) {
    const bonuses = { bonusATK: 0, hpRegenBonus: 0, spRegenBonus: 0, bonusMDEF: 0 };
    const learned = player.learnedSkills || {};
    const wType = player.weaponType || null;

    // Sword Mastery (100): +4 ATK/level with daggers & 1H swords
    const smLv = learned[100] || 0;
    if (smLv > 0 && (wType === 'dagger' || wType === 'one_hand_sword')) {
        bonuses.bonusATK += smLv * 4;
    }
    // Two-Handed Sword Mastery (101): +4 ATK/level with 2H swords
    const tsmLv = learned[101] || 0;
    if (tsmLv > 0 && wType === 'two_hand_sword') {
        bonuses.bonusATK += tsmLv * 4;
    }
    // Increase HP Recovery (102): +5 HP per regen tick per level
    const hprLv = learned[102] || 0;
    if (hprLv > 0) {
        bonuses.hpRegenBonus = hprLv * 5;
    }
    // Increase SP Recovery (204): +3 SP per regen tick per level
    const sprLv = learned[204] || 0;
    if (sprLv > 0) {
        bonuses.spRegenBonus = sprLv * 3;
    }
    return bonuses;
}

// Check skill cooldown
function isSkillOnCooldown(player, skillId) {
    const cd = (player.skillCooldowns || {})[skillId];
    if (!cd) return false;
    return Date.now() < cd;
}

// Get remaining cooldown ms
function getSkillCooldownRemaining(player, skillId) {
    const cd = (player.skillCooldowns || {})[skillId];
    if (!cd) return 0;
    return Math.max(0, cd - Date.now());
}

// Set a skill cooldown
function setSkillCooldown(player, skillId, cooldownMs) {
    if (!player.skillCooldowns) player.skillCooldowns = {};
    player.skillCooldowns[skillId] = Date.now() + cooldownMs;
}

// Set After-Cast Delay (global skill lockout) + per-skill cooldown + emit events
function applySkillDelays(characterId, player, skillId, levelData, socket) {
    const cooldownMs = levelData.cooldown || 0;
    const acdMs = levelData.afterCastDelay || 0;

    // Per-skill cooldown (only THIS skill is locked)
    if (cooldownMs > 0) {
        setSkillCooldown(player, skillId, cooldownMs);
        if (socket) socket.emit('skill:cooldown_started', { skillId, cooldownMs });
    }

    // After-Cast Delay (ALL skills locked globally)
    if (acdMs > 0) {
        afterCastDelayEnd.set(characterId, Date.now() + acdMs);
        if (socket) socket.emit('skill:acd_started', { afterCastDelay: acdMs });
    }
}

// Apply a buff/debuff to a target (player or enemy)
function applyBuff(target, buffDef) {
    if (!target.activeBuffs) target.activeBuffs = [];
    // Remove existing buff of same skillId (refresh)
    target.activeBuffs = target.activeBuffs.filter(b => b.skillId !== buffDef.skillId);
    target.activeBuffs.push({
        ...buffDef,
        appliedAt: Date.now(),
        expiresAt: Date.now() + buffDef.duration
    });
}

// Remove expired buffs, returns array of expired buff objects
function expireBuffs(target) {
    if (!target.activeBuffs || target.activeBuffs.length === 0) return [];
    const now = Date.now();
    const expired = target.activeBuffs.filter(b => now >= b.expiresAt);
    target.activeBuffs = target.activeBuffs.filter(b => now < b.expiresAt);
    return expired;
}

// Get stat modifiers from all active buffs/debuffs
function getBuffStatModifiers(target) {
    const mods = { defMultiplier: 1.0, atkMultiplier: 1.0, bonusMDEF: 0, isFrozen: false, isStoned: false, overrideElement: null };
    if (!target.activeBuffs) return mods;
    const now = Date.now();
    for (const buff of target.activeBuffs) {
        if (now >= buff.expiresAt) continue; // skip expired
        if (buff.name === 'provoke') {
            mods.defMultiplier *= (1 - (buff.defReduction || 0) / 100);
            mods.atkMultiplier *= (1 + (buff.atkIncrease || 0) / 100);
        }
        if (buff.name === 'endure') {
            mods.bonusMDEF += buff.mdefBonus || 0;
        }
        if (buff.name === 'frozen') {
            mods.isFrozen = true;
            mods.overrideElement = { type: 'water', level: 1 }; // Frozen targets become Water Lv1
        }
        if (buff.name === 'stone_curse') {
            mods.isStoned = true;
            mods.overrideElement = { type: 'earth', level: 1 }; // Stoned targets become Earth Lv1
        }
    }
    return mods;
}

// Calculate skill damage using full RO damage system
function calculateSkillDamage(attackerStats, targetStats, targetHardDef, skillMultiplier, attackerBuffMods, targetBuffMods, targetInfo = {}, attackerInfo = {}, skillOptions = {}) {
    return calculatePhysicalDamage(
        attackerStats, targetStats, targetHardDef,
        { ...targetInfo, buffMods: targetBuffMods || { defMultiplier: 1.0 } },
        { ...attackerInfo, buffMods: attackerBuffMods || { atkMultiplier: 1.0 } },
        { isSkill: true, skillMultiplier, ...skillOptions }
    );
}

// Calculate magic skill damage using the RO pre-renewal MATK system
function calculateMagicSkillDamage(attackerStats, targetStats, targetHardMdef, skillMultiplier, skillElement, targetInfo = {}) {
    return roMagicalDamage(
        { stats: attackerStats, weaponMATK: 0, buffMods: { atkMultiplier: 1.0 } },
        { stats: targetStats, hardMdef: targetHardMdef,
          element: targetInfo.element || { type: 'neutral', level: 1 },
          buffMods: targetInfo.buffMods || { defMultiplier: 1.0, bonusMDEF: 0 } },
        { skillMultiplier, skillElement }
    );
}

// Helper: process enemy death from skill kill (shares logic with auto-attack kill)
async function processEnemyDeathFromSkill(enemy, attacker, attackerId, io) {
    enemy.isDead = true;
    logger.info(`[SKILL-COMBAT] Enemy ${enemy.name}(${enemy.enemyId}) killed by ${attacker.characterName} (skill)`);

    // Stop all auto-attackers targeting this enemy
    for (const [otherId, otherAtk] of autoAttackState.entries()) {
        if (otherAtk.isEnemy && otherAtk.targetCharId === enemy.enemyId) {
            autoAttackState.delete(otherId);
        }
    }
    enemy.inCombatWith.clear();

    // Award EXP
    const baseExpReward = enemy.baseExp || 0;
    const jobExpReward = enemy.jobExp || 0;
    const expResult = processExpGain(attacker, baseExpReward, jobExpReward);

    const killerSocket = io.sockets.sockets.get(attacker.socketId);
    if (killerSocket && (expResult.baseExpGained > 0 || expResult.jobExpGained > 0)) {
        killerSocket.emit('exp:gain', {
            characterId: attackerId,
            baseExpGained: expResult.baseExpGained,
            jobExpGained: expResult.jobExpGained,
            enemyName: enemy.name,
            enemyLevel: enemy.level,
            exp: buildExpPayload(attacker),
            baseLevelUps: expResult.baseLevelUps,
            jobLevelUps: expResult.jobLevelUps
        });

        if (expResult.baseLevelUps.length > 0 || expResult.jobLevelUps.length > 0) {
            const levelUpPayload = {
                characterId: attackerId, characterName: attacker.characterName,
                baseLevelUps: expResult.baseLevelUps, jobLevelUps: expResult.jobLevelUps,
                totalStatPoints: expResult.totalStatPoints, totalSkillPoints: expResult.totalSkillPoints,
                exp: buildExpPayload(attacker)
            };
            killerSocket.emit('exp:level_up', levelUpPayload);
            killerSocket.broadcast.emit('exp:level_up', levelUpPayload);

            const effectiveStats = getEffectiveStats(attacker);
            const newDerived = calculateDerivedStats(effectiveStats);
            const newFinalAspd = Math.min(COMBAT.ASPD_CAP, newDerived.aspd + (attacker.weaponAspdMod || 0));
            killerSocket.emit('player:stats', buildFullStatsPayload(attackerId, attacker, effectiveStats, newDerived, newFinalAspd));
            io.emit('combat:health_update', {
                characterId: attackerId, health: attacker.health,
                maxHealth: attacker.maxHealth, mana: attacker.mana, maxMana: attacker.maxMana
            });
            for (const lu of expResult.baseLevelUps) {
                io.emit('chat:receive', { type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
                    message: `${attacker.characterName} has reached Base Level ${lu.newLevel}!`, timestamp: Date.now() });
            }
            for (const lu of expResult.jobLevelUps) {
                io.emit('chat:receive', { type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
                    message: `${attacker.characterName} has reached Job Level ${lu.newJobLevel}!`, timestamp: Date.now() });
            }
        }
        saveExpDataToDB(attackerId, attacker);
    }

    // Broadcast enemy death
    io.emit('enemy:death', {
        enemyId: enemy.enemyId, enemyName: enemy.name,
        killerId: attackerId, killerName: attacker.characterName,
        isEnemy: true, isDead: true,
        baseExp: baseExpReward, jobExp: jobExpReward, exp: enemy.exp,
        timestamp: Date.now()
    });

    // Roll and award loot
    const droppedItems = rollEnemyDrops(enemy);
    if (droppedItems.length > 0) {
        const lootItems = [];
        let addedToDb = false;
        for (const drop of droppedItems) {
            if (drop.itemId) {
                const added = await addItemToInventory(attackerId, drop.itemId, drop.quantity);
                if (added) {
                    const itemDef = itemDefinitions.get(drop.itemId);
                    lootItems.push({ itemId: drop.itemId, itemName: drop.itemName, quantity: drop.quantity,
                        icon: itemDef ? itemDef.icon : 'default_item', itemType: itemDef ? itemDef.item_type : 'etc' });
                    addedToDb = true;
                }
            } else {
                lootItems.push({ itemId: null, itemName: drop.itemName, quantity: drop.quantity,
                    icon: 'default_item', itemType: 'etc' });
            }
        }
        if (lootItems.length > 0 && killerSocket) {
            killerSocket.emit('loot:drop', { enemyId: enemy.enemyId, enemyName: enemy.name, items: lootItems });
            if (addedToDb) {
                const killerInventory = await getPlayerInventory(attackerId);
                killerSocket.emit('inventory:data', { items: killerInventory, zuzucoin: attacker.zuzucoin });
            }
        }
    }

    // Schedule respawn
    setTimeout(() => {
        enemy.health = enemy.maxHealth;
        enemy.isDead = false;
        enemy.x = enemy.spawnX; enemy.y = enemy.spawnY; enemy.z = enemy.spawnZ;
        enemy.targetPlayerId = null;
        enemy.inCombatWith = new Set();
        if (typeof initEnemyWanderState === 'function') initEnemyWanderState(enemy);
        io.emit('enemy:spawn', {
            enemyId: enemy.enemyId, templateId: enemy.templateId, name: enemy.name,
            level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
            x: enemy.x, y: enemy.y, z: enemy.z
        });
    }, enemy.respawnMs);
}

// ============================================================
// RO-Style Derived Stat Calculations (delegates to ro_damage_formulas.js)
// ============================================================
function calculateDerivedStats(stats) {
    return roDerivedStats(stats);
}

/**
 * Full RO pre-renewal physical damage calculation.
 * Handles: HIT/FLEE, Critical, Perfect Dodge, Size Penalty, Element, DEF.
 *
 * @param {Object} attackerStats — effective stats from getEffectiveStats()
 * @param {Object} targetStats — effective stats or enemy stats
 * @param {number} targetHardDef — target's equipment DEF
 * @param {Object} targetInfo — { element, size, race, numAttackers, buffMods }
 * @param {Object} attackerInfo — { weaponType, weaponElement, weaponLevel, buffMods, cardMods }
 * @param {Object} options — { isSkill, skillMultiplier, skillHitBonus, forceHit, skillElement }
 * @returns {Object} { damage, isCritical, isMiss, hitType, element, sizePenalty, elementModifier }
 */
function calculatePhysicalDamage(attackerStats, targetStats, targetHardDef = 0, targetInfo = {}, attackerInfo = {}, options = {}) {
    const attacker = {
        stats: attackerStats,
        weaponATK: attackerStats.weaponATK || 0,
        passiveATK: attackerStats.passiveATK || 0,
        weaponType: attackerInfo.weaponType || 'bare_hand',
        weaponElement: attackerInfo.weaponElement || 'neutral',
        weaponLevel: attackerInfo.weaponLevel || 1,
        buffMods: attackerInfo.buffMods || { atkMultiplier: attackerStats.buffAtkMultiplier || 1.0 },
        cardMods: attackerInfo.cardMods || null
    };

    const target = {
        stats: targetStats,
        hardDef: targetHardDef,
        element: targetInfo.element || { type: 'neutral', level: 1 },
        size: targetInfo.size || 'medium',
        race: targetInfo.race || 'formless',
        numAttackers: targetInfo.numAttackers || 1,
        buffMods: targetInfo.buffMods || { defMultiplier: targetStats.buffDefMultiplier || 1.0 }
    };

    return roPhysicalDamage(attacker, target, options);
}

// Merge base stats with equipment bonuses + passive skill bonuses + buff modifiers
function getEffectiveStats(player) {
    const bonuses = player.equipmentBonuses || {};
    const passive = getPassiveSkillBonuses(player);
    const buffMods = getBuffStatModifiers(player);
    return {
        str: (player.stats.str || 1) + (bonuses.str || 0),
        agi: (player.stats.agi || 1) + (bonuses.agi || 0),
        vit: (player.stats.vit || 1) + (bonuses.vit || 0),
        int: (player.stats.int || 1) + (bonuses.int || 0),
        dex: (player.stats.dex || 1) + (bonuses.dex || 0),
        luk: (player.stats.luk || 1) + (bonuses.luk || 0),
        level: player.stats.level || 1,
        weaponATK: player.stats.weaponATK || 0,
        passiveATK: passive.bonusATK,  // Sword Mastery / 2H Sword Mastery bonus
        statPoints: player.stats.statPoints || 0,
        bonusHit: bonuses.hit || 0,
        bonusFlee: bonuses.flee || 0,
        bonusCritical: bonuses.critical || 0,
        bonusMaxHp: bonuses.maxHp || 0,
        bonusMaxSp: bonuses.maxSp || 0,
        buffAtkMultiplier: buffMods.atkMultiplier,
        buffDefMultiplier: buffMods.defMultiplier,
        buffBonusMDEF: buffMods.bonusMDEF
    };
}

// Build attacker info object for the RO damage system
function getAttackerInfo(player) {
    return {
        weaponType: player.weaponType || 'bare_hand',
        weaponElement: player.weaponElement || 'neutral',
        weaponLevel: player.weaponLevel || 1,
        buffMods: getBuffStatModifiers(player),
        cardMods: player.cardMods || null
    };
}

// Build target info object for enemies
function getEnemyTargetInfo(enemy) {
    // Count how many players are currently auto-attacking this enemy
    let numAttackers = 0;
    for (const [, atkState] of autoAttackState.entries()) {
        if (atkState.isEnemy && atkState.targetCharId === enemy.enemyId) {
            numAttackers++;
        }
    }
    return {
        element: enemy.element || { type: 'neutral', level: 1 },
        size: enemy.size || 'medium',
        race: enemy.race || 'formless',
        numAttackers: Math.max(1, numAttackers),
        buffMods: getBuffStatModifiers(enemy)
    };
}

// Build target info object for players
function getPlayerTargetInfo(player, targetCharId) {
    // Count how many entities are auto-attacking this player
    let numAttackers = 0;
    for (const [, atkState] of autoAttackState.entries()) {
        if (!atkState.isEnemy && atkState.targetCharId === targetCharId) {
            numAttackers++;
        }
    }
    return {
        element: player.armorElement || { type: 'neutral', level: 1 },
        size: 'medium', // Players are always medium
        race: 'demihuman',
        numAttackers: Math.max(1, numAttackers),
        buffMods: getBuffStatModifiers(player)
    };
}

// ============================================================
// EXP & Leveling System — Ragnarok Online Classic
// Dual progression: Base Level (stats) + Job Level (skills)
// ============================================================

// Process EXP gain for a player after killing a monster
// Returns { baseExpGained, jobExpGained, baseLevelUps[], jobLevelUps[], totalStatPoints, totalSkillPoints }
function processExpGain(player, baseExpReward, jobExpReward) {
    const result = {
        baseExpGained: 0,
        jobExpGained: 0,
        baseLevelUps: [],    // Array of { newLevel, statPointsGained }
        jobLevelUps: [],     // Array of { newJobLevel, skillPointsGained }
        totalStatPoints: 0,
        totalSkillPoints: 0
    };
    
    const baseLevel = player.stats.level || 1;
    const jobLevel = player.jobLevel || 1;
    const jobClass = player.jobClass || 'novice';
    
    // ── Base EXP Processing ──
    if (baseLevel < MAX_BASE_LEVEL) {
        player.baseExp = (player.baseExp || 0) + baseExpReward;
        result.baseExpGained = baseExpReward;
        
        // Check for multiple level ups (in case of large EXP gains)
        let currentLevel = baseLevel;
        let expNeeded = getBaseExpForNextLevel(currentLevel);
        
        while (expNeeded > 0 && player.baseExp >= expNeeded && currentLevel < MAX_BASE_LEVEL) {
            // Level up!
            player.baseExp -= expNeeded;
            currentLevel++;
            player.stats.level = currentLevel;
            
            // Grant stat points (RO formula: floor(newLevel / 5) + 3)
            const statPtsGained = getStatPointsForLevelUp(currentLevel);
            player.stats.statPoints = (player.stats.statPoints || 0) + statPtsGained;
            result.totalStatPoints += statPtsGained;
            
            result.baseLevelUps.push({
                newLevel: currentLevel,
                statPointsGained: statPtsGained
            });
            
            logger.info(`[EXP] ${player.characterName} BASE LEVEL UP! ${currentLevel - 1} → ${currentLevel} (+${statPtsGained} stat points)`);
            
            // Recalculate derived stats (maxHP/maxSP scale with level)
            const effectiveStats = getEffectiveStats(player);
            const derived = calculateDerivedStats(effectiveStats);
            player.maxHealth = derived.maxHP;
            player.maxMana = derived.maxSP;
            // Fully heal on level up (classic RO behavior)
            player.health = player.maxHealth;
            player.mana = player.maxMana;
            
            expNeeded = getBaseExpForNextLevel(currentLevel);
        }
        
        // Cap base_exp at max needed if at max level
        if (currentLevel >= MAX_BASE_LEVEL) {
            player.baseExp = 0;
        }
    }
    
    // ── Job EXP Processing ──
    const maxJobLevel = getMaxJobLevel(jobClass);
    if (jobLevel < maxJobLevel) {
        player.jobExp = (player.jobExp || 0) + jobExpReward;
        result.jobExpGained = jobExpReward;
        
        let currentJobLevel = jobLevel;
        let jobExpNeeded = getJobExpForNextLevel(jobClass, currentJobLevel);
        
        while (jobExpNeeded > 0 && player.jobExp >= jobExpNeeded && currentJobLevel < maxJobLevel) {
            // Job level up!
            player.jobExp -= jobExpNeeded;
            currentJobLevel++;
            player.jobLevel = currentJobLevel;
            
            // Grant skill points (always 1 per job level in classic RO)
            const skillPtsGained = getSkillPointsForJobLevelUp();
            player.skillPoints = (player.skillPoints || 0) + skillPtsGained;
            result.totalSkillPoints += skillPtsGained;
            
            result.jobLevelUps.push({
                newJobLevel: currentJobLevel,
                skillPointsGained: skillPtsGained
            });
            
            logger.info(`[EXP] ${player.characterName} JOB LEVEL UP! ${currentJobLevel - 1} → ${currentJobLevel} [${jobClass}] (+${skillPtsGained} skill points)`);
            
            jobExpNeeded = getJobExpForNextLevel(jobClass, currentJobLevel);
        }
        
        // Cap job_exp at 0 if at max job level
        if (currentJobLevel >= maxJobLevel) {
            player.jobExp = 0;
        }
    }
    
    return result;
}

// Save EXP and leveling data to database
async function saveExpDataToDB(characterId, player) {
    try {
        await pool.query(
            `UPDATE characters SET 
                level = $1, job_level = $2, base_exp = $3, job_exp = $4, 
                job_class = $5, stat_points = $6, skill_points = $7,
                max_health = $8, max_mana = $9, health = $10, mana = $11
             WHERE character_id = $12`,
            [
                player.stats.level || 1,
                player.jobLevel || 1,
                player.baseExp || 0,
                player.jobExp || 0,
                player.jobClass || 'novice',
                player.stats.statPoints || 0,
                player.skillPoints || 0,
                player.maxHealth,
                player.maxMana,
                player.health,
                player.mana,
                characterId
            ]
        );
        logger.debug(`[DB] Saved EXP data for character ${characterId}: BLv${player.stats.level} JLv${player.jobLevel} [${player.jobClass}]`);
    } catch (err) {
        logger.error(`[DB] Failed to save EXP data for character ${characterId}: ${err.message}`);
    }
}

// Build EXP payload for client (sent with player:stats and exp:gain events)
function buildExpPayload(player) {
    const jobClass = player.jobClass || 'novice';
    const baseLevel = player.stats.level || 1;
    const jobLevel = player.jobLevel || 1;
    return {
        baseLevel,
        jobLevel,
        baseExp: player.baseExp || 0,
        jobExp: player.jobExp || 0,
        baseExpNext: getBaseExpForNextLevel(baseLevel),
        jobExpNext: getJobExpForNextLevel(jobClass, jobLevel),
        jobClass,
        jobClassDisplayName: (JOB_CLASS_CONFIG[jobClass] || {}).displayName || jobClass,
        maxBaseLevel: MAX_BASE_LEVEL,
        maxJobLevel: getMaxJobLevel(jobClass),
        statPoints: player.stats.statPoints || 0,
        skillPoints: player.skillPoints || 0
    };
}

// RO Classic stat point cost formula: floor((currentStat - 1) / 10) + 2
function getStatPointCost(currentStatValue) {
    return Math.floor((currentStatValue - 1) / 10) + 2;
}

// Build full player:stats payload including stat allocation info
function buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd) {
    // Calculate per-stat costs for the UI (based on BASE stats, not effective)
    const statCosts = {
        str: getStatPointCost(player.stats.str || 1),
        agi: getStatPointCost(player.stats.agi || 1),
        vit: getStatPointCost(player.stats.vit || 1),
        int: getStatPointCost(player.stats.int || 1),
        dex: getStatPointCost(player.stats.dex || 1),
        luk: getStatPointCost(player.stats.luk || 1)
    };
    return {
        characterId,
        stats: {
            ...player.stats,
            str: effectiveStats.str, agi: effectiveStats.agi, vit: effectiveStats.vit,
            int: effectiveStats.int, dex: effectiveStats.dex, luk: effectiveStats.luk,
            hardDef: player.hardDef || 0
        },
        baseStats: {
            str: player.stats.str || 1, agi: player.stats.agi || 1, vit: player.stats.vit || 1,
            int: player.stats.int || 1, dex: player.stats.dex || 1, luk: player.stats.luk || 1
        },
        statCosts,
        derived: {
            ...derived,
            aspd: finalAspd
        },
        exp: buildExpPayload(player)
    };
}

// ============================================================
// Enemy/NPC System — Ragnarok Online Pre-Renewal Monsters
// 509 monsters from rAthena database + compatibility adapter
// ============================================================
const enemies = new Map();
let nextEnemyId = 2000001;

// Build ENEMY_TEMPLATES from RO_MONSTER_TEMPLATES
// This adapter converts RO template format → game-compatible format
const ENEMY_TEMPLATES = {};
for (const [key, ro] of Object.entries(RO_MONSTER_TEMPLATES)) {
    ENEMY_TEMPLATES[key] = {
        name: ro.name,
        level: ro.level,
        maxHealth: ro.maxHealth,
        damage: ro.attack,                        // RO Attack → game damage
        attackRange: ro.attackRange,               // Already converted (cells * 50)
        aggroRange: ro.aggroRange,                 // 0 for passive mobs
        chaseRange: ro.chaseRange,
        aspd: ro.aspd,
        exp: ro.baseExp + ro.jobExp,               // Total EXP = Base + Job
        baseExp: ro.baseExp,
        jobExp: ro.jobExp,
        mvpExp: ro.mvpExp,
        respawnMs: ro.respawnMs,
        aiType: ro.aiType,
        monsterClass: ro.monsterClass,             // normal/boss/mvp
        size: ro.size,                             // small/medium/large
        race: ro.race,                             // plant/insect/brute/etc
        element: ro.element,                       // { type, level }
        walkSpeed: ro.walkSpeed,
        attackDelay: ro.attackDelay,
        attackMotion: ro.attackMotion,
        damageMotion: ro.damageMotion,
        attack2: ro.attack2,                       // Max attack (pre-renewal)
        defense: ro.defense,                       // Hard DEF
        magicDefense: ro.magicDefense,             // Hard MDEF
        raceGroups: ro.raceGroups || {},
        stats: { ...ro.stats },
        modes: { ...ro.modes },
        // Drop format: { itemName, itemId, chance (decimal), rate (%) }
        // itemId resolved from RO_ITEM_NAME_TO_ID mapping (null if not in DB)
        drops: (ro.drops || []).map(d => ({
            itemName: d.itemName,
            itemId: RO_ITEM_NAME_TO_ID[d.itemName] || null,
            chance: d.rate / 100,                  // Convert % to decimal for Math.random()
            rate: d.rate,                          // Keep original % for display
            stealProtected: d.stealProtected || false
        })),
        mvpDrops: (ro.mvpDrops || []).map(d => ({
            itemName: d.itemName,
            itemId: RO_ITEM_NAME_TO_ID[d.itemName] || null,
            chance: d.rate / 100,
            rate: d.rate
        }))
    };
}

// Inject existing game items as extra drops on appropriate RO monsters
for (const [itemIdStr, monsterDrops] of Object.entries(EXISTING_ITEM_EXTRA_DROPS)) {
    const itemId = parseInt(itemIdStr);
    for (const { monster, chance } of monsterDrops) {
        if (ENEMY_TEMPLATES[monster]) {
            ENEMY_TEMPLATES[monster].drops.push({
                itemName: null,  // Will be resolved from itemDefinitions at drop time
                itemId: itemId,
                chance: chance,
                rate: chance * 100,
                stealProtected: false,
                isExistingItem: true
            });
        }
    }
}

const RO_TEMPLATE_COUNT = Object.keys(ENEMY_TEMPLATES).length;
logger.info(`[ENEMY] Loaded ${RO_TEMPLATE_COUNT} RO monster templates`);

// Count how many drops have resolved itemIds vs unresolved
let resolvedDrops = 0, unresolvedDrops = 0;
for (const t of Object.values(ENEMY_TEMPLATES)) {
    for (const d of t.drops) {
        if (d.itemId) resolvedDrops++; else unresolvedDrops++;
    }
}
logger.info(`[ENEMY] Drop itemId resolution: ${resolvedDrops} resolved, ${unresolvedDrops} unresolved`);

// ============================================================
// Spawn Configuration — Zone-based RO monster spawns
// Organized by level zones radiating outward from origin
// ============================================================
const ENEMY_SPAWNS = [
    // ── Zone 1: Starter Area (Level 1-10) — Near origin ────────
    // Porings, Lunatics, Fabres — the classic beginner mobs
    { template: 'poring',       x: 300,   y: 300,   z: 300, wanderRadius: 400 },
    { template: 'poring',       x: -200,  y: 400,   z: 300, wanderRadius: 400 },
    { template: 'poring',       x: 100,   y: -300,  z: 300, wanderRadius: 400 },
    { template: 'poring',       x: -400,  y: -200,  z: 300, wanderRadius: 400 },
    { template: 'poring',       x: 500,   y: -100,  z: 300, wanderRadius: 400 },
    { template: 'lunatic',      x: 600,   y: 500,   z: 300, wanderRadius: 350 },
    { template: 'lunatic',      x: -500,  y: 600,   z: 300, wanderRadius: 350 },
    { template: 'lunatic',      x: 200,   y: 700,   z: 300, wanderRadius: 350 },
    { template: 'fabre',        x: -600,  y: -400,  z: 300, wanderRadius: 300 },
    { template: 'fabre',        x: 700,   y: -300,  z: 300, wanderRadius: 300 },
    { template: 'fabre',        x: -300,  y: 800,   z: 300, wanderRadius: 300 },
    { template: 'pupa',         x: 400,   y: 600,   z: 300, wanderRadius: 200 },
    { template: 'pupa',         x: -100,  y: -600,  z: 300, wanderRadius: 200 },
    { template: 'drops',        x: 800,   y: 200,   z: 300, wanderRadius: 350 },
    { template: 'drops',        x: -700,  y: 300,   z: 300, wanderRadius: 350 },

    // ── Zone 2: Prontera Fields (Level 5-15) ───────────────────
    { template: 'chonchon',     x: 1200,  y: 400,   z: 300, wanderRadius: 400 },
    { template: 'chonchon',     x: 1000,  y: -500,  z: 300, wanderRadius: 400 },
    { template: 'chonchon',     x: -1100, y: 200,   z: 300, wanderRadius: 400 },
    { template: 'condor',       x: 1300,  y: 700,   z: 300, wanderRadius: 500 },
    { template: 'condor',       x: -1200, y: -600,  z: 300, wanderRadius: 500 },
    { template: 'wilow',        x: 900,   y: 900,   z: 300, wanderRadius: 350 },
    { template: 'wilow',        x: -900,  y: 800,   z: 300, wanderRadius: 350 },
    { template: 'roda_frog',    x: 1100,  y: -800,  z: 300, wanderRadius: 400 },
    { template: 'roda_frog',    x: -1000, y: -900,  z: 300, wanderRadius: 400 },
    { template: 'hornet',       x: 1400,  y: 100,   z: 300, wanderRadius: 450 },
    { template: 'hornet',       x: -1300, y: -300,  z: 300, wanderRadius: 450 },
    { template: 'rocker',       x: 1500,  y: -200,  z: 300, wanderRadius: 400 },
    { template: 'rocker',       x: -1400, y: 500,   z: 300, wanderRadius: 400 },
    { template: 'farmiliar',    x: 1200,  y: -1000, z: 300, wanderRadius: 400 },
    { template: 'savage_babe',  x: -1100, y: -1100, z: 300, wanderRadius: 350 },

    // ── Zone 3: Payon/Morroc Fields (Level 10-20) ──────────────
    { template: 'spore',        x: 1800,  y: 600,   z: 300, wanderRadius: 500 },
    { template: 'spore',        x: -1700, y: 700,   z: 300, wanderRadius: 500 },
    { template: 'zombie',       x: 2000,  y: -300,  z: 300, wanderRadius: 400 },
    { template: 'zombie',       x: -1900, y: -400,  z: 300, wanderRadius: 400 },
    { template: 'zombie',       x: 1700,  y: -800,  z: 300, wanderRadius: 400 },
    { template: 'skeleton',     x: 2100,  y: 200,   z: 300, wanderRadius: 450 },
    { template: 'skeleton',     x: -2000, y: 300,   z: 300, wanderRadius: 450 },
    { template: 'creamy',       x: 1600,  y: 1100,  z: 300, wanderRadius: 400 },
    { template: 'poporing',     x: -1800, y: 1000,  z: 300, wanderRadius: 400 },
    { template: 'poporing',     x: 1900,  y: 900,   z: 300, wanderRadius: 400 },
    { template: 'pecopeco',     x: 2200,  y: -600,  z: 300, wanderRadius: 500 },
    { template: 'pecopeco',     x: -2100, y: -700,  z: 300, wanderRadius: 500 },
    { template: 'mandragora',   x: 2300,  y: 800,   z: 300, wanderRadius: 300 },
    { template: 'poison_spore', x: 1800,  y: -1200, z: 300, wanderRadius: 400 },
    { template: 'smokie',       x: -1600, y: -1300, z: 300, wanderRadius: 400 },
    { template: 'yoyo',         x: 2000,  y: 1200,  z: 300, wanderRadius: 450 },

    // ── Zones 4-9 and Boss/MVP spawns DISABLED ──────────────────
    // These zones will be enabled when higher-level content is ready
    // See docsNew/03_Server_Side/Enemy_System.md for full zone config
];

function spawnEnemy(spawnConfig) {
    const template = ENEMY_TEMPLATES[spawnConfig.template];
    if (!template) {
        logger.warn(`[ENEMY] Unknown template '${spawnConfig.template}' — skipping spawn`);
        return null;
    }
    const enemyId = nextEnemyId++;
    const enemy = {
        enemyId, templateId: spawnConfig.template,
        name: template.name, level: template.level,
        health: template.maxHealth, maxHealth: template.maxHealth,
        damage: template.damage, attackRange: template.attackRange,
        aggroRange: template.aggroRange, aspd: template.aspd,
        exp: template.exp, baseExp: template.baseExp || 0, jobExp: template.jobExp || 0,
        mvpExp: template.mvpExp || 0,
        aiType: template.aiType,
        monsterClass: template.monsterClass || 'normal',
        size: template.size || 'medium',
        race: template.race || 'formless',
        element: template.element || { type: 'neutral', level: 1 },
        attack2: template.attack2 || template.damage,
        hardDef: template.defense || 0,
        magicDefense: template.magicDefense || 0,
        walkSpeed: template.walkSpeed || 200,
        attackDelay: template.attackDelay || 1500,
        attackMotion: template.attackMotion || 500,
        damageMotion: template.damageMotion || 300,
        raceGroups: template.raceGroups || {},
        modes: template.modes || {},
        stats: { ...template.stats }, isDead: false,
        x: spawnConfig.x, y: spawnConfig.y, z: spawnConfig.z,
        spawnX: spawnConfig.x, spawnY: spawnConfig.y, spawnZ: spawnConfig.z,
        wanderRadius: spawnConfig.wanderRadius, respawnMs: template.respawnMs,
        targetPlayerId: null, lastAttackTime: 0,
        inCombatWith: new Set()
    };
    enemies.set(enemyId, enemy);
    logger.info(`[ENEMY] Spawned ${enemy.name} (ID: ${enemyId}) Lv${enemy.level} [${enemy.monsterClass}] at (${enemy.x}, ${enemy.y}, ${enemy.z})`);
    io.emit('enemy:spawn', {
        enemyId, templateId: spawnConfig.template, name: enemy.name,
        level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
        monsterClass: enemy.monsterClass, size: enemy.size, race: enemy.race,
        element: enemy.element,
        x: enemy.x, y: enemy.y, z: enemy.z
    });
    return enemy;
}

// Helper: Save player health/mana to database
async function savePlayerHealthToDB(characterId, health, mana) {
    try {
        await pool.query(
            'UPDATE characters SET health = $1, mana = $2 WHERE character_id = $3',
            [health, mana, characterId]
        );
        logger.debug(`[DB] Saved health/mana for character ${characterId}: HP=${health}, MP=${mana}`);
    } catch (err) {
        logger.error(`[DB] Failed to save health for character ${characterId}:`, err.message);
    }
}

// ============================================================
// Item & Inventory System
// ============================================================
const INVENTORY = {
    MAX_SLOTS: 100,           // Max inventory slots per character
    MAX_WEIGHT: 2000          // Max carry weight (future use)
};

// Item definitions cache (loaded from DB on startup)
const itemDefinitions = new Map();

// NPC Shop definitions (server-authoritative, shopId → shop config)
// Buy price = item.price * 2  |  Sell price = item.price
const NPC_SHOPS = {
    1: {
        name: 'General Store',
        itemIds: [1001, 1002, 1003, 1004, 1005, 4001, 4002, 4003]
    },
    2: {
        name: 'Weapon Shop',
        itemIds: [3001, 3002, 3003, 3004, 3005, 3006]
    }
};

async function loadItemDefinitions() {
    try {
        const result = await pool.query('SELECT * FROM items');
        for (const row of result.rows) {
            itemDefinitions.set(row.item_id, row);
        }
        logger.info(`[ITEMS] Loaded ${itemDefinitions.size} item definitions from database`);
    } catch (err) {
        logger.error(`[ITEMS] Failed to load item definitions: ${err.message}`);
    }
}

// Roll enemy drops using pre-resolved itemIds from RO_ITEM_NAME_TO_ID mapping
function rollEnemyDrops(enemy) {
    const template = ENEMY_TEMPLATES[enemy.templateId];
    if (!template || !template.drops) return [];
    
    const droppedItems = [];
    for (const drop of template.drops) {
        if (Math.random() < drop.chance) {
            const qty = drop.minQty && drop.maxQty
                ? drop.minQty + Math.floor(Math.random() * (drop.maxQty - drop.minQty + 1))
                : 1;
            
            // itemId is pre-resolved in the adapter (from RO_ITEM_NAME_TO_ID or existing items)
            const itemId = drop.itemId;
            const itemDef = itemId ? itemDefinitions.get(itemId) : null;
            const itemName = drop.itemName || (itemDef ? itemDef.name : null);
            
            if (itemId) {
                droppedItems.push({
                    itemId: itemId,
                    quantity: qty,
                    itemName: itemName || `Item#${itemId}`
                });
            } else if (itemName) {
                // Unresolved RO item (not in DB) — still notify client
                droppedItems.push({
                    itemId: null,
                    quantity: qty,
                    itemName: itemName
                });
            }
        }
    }
    
    // Also roll MVP drops if this is an MVP monster
    if (template.mvpDrops && template.mvpDrops.length > 0 && enemy.monsterClass === 'mvp') {
        for (const drop of template.mvpDrops) {
            if (Math.random() < drop.chance) {
                const itemId = drop.itemId;
                const itemDef = itemId ? itemDefinitions.get(itemId) : null;
                droppedItems.push({
                    itemId: itemId,
                    quantity: 1,
                    itemName: drop.itemName || (itemDef ? itemDef.name : `Item#${itemId}`),
                    isMvpDrop: true
                });
            }
        }
    }
    
    return droppedItems;
}

// Add item to character inventory (DB + return updated entry)
async function addItemToInventory(characterId, itemId, quantity = 1) {
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) {
        logger.error(`[ITEMS] Cannot add unknown item ${itemId} to inventory`);
        return null;
    }
    
    try {
        if (itemDef.stackable) {
            // Check if player already has this item stacked
            const existing = await pool.query(
                'SELECT inventory_id, quantity FROM character_inventory WHERE character_id = $1 AND item_id = $2 AND is_equipped = false',
                [characterId, itemId]
            );
            
            if (existing.rows.length > 0) {
                const newQty = Math.min(existing.rows[0].quantity + quantity, itemDef.max_stack);
                await pool.query(
                    'UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2',
                    [newQty, existing.rows[0].inventory_id]
                );
                logger.info(`[ITEMS] Stacked ${quantity}x ${itemDef.name} for char ${characterId} (now ${newQty})`);
                return { inventoryId: existing.rows[0].inventory_id, itemId, quantity: newQty, isEquipped: false };
            }
        }
        
        // Find next available slot_index for this character
        const maxSlotResult = await pool.query(
            'SELECT COALESCE(MAX(slot_index), -1) as max_slot FROM character_inventory WHERE character_id = $1',
            [characterId]
        );
        const nextSlot = maxSlotResult.rows[0].max_slot + 1;

        // Insert new inventory entry with auto-assigned slot
        const result = await pool.query(
            'INSERT INTO character_inventory (character_id, item_id, quantity, slot_index) VALUES ($1, $2, $3, $4) RETURNING inventory_id',
            [characterId, itemId, quantity, nextSlot]
        );
        logger.info(`[ITEMS] Added ${quantity}x ${itemDef.name} to char ${characterId} inventory`);
        return { inventoryId: result.rows[0].inventory_id, itemId, quantity, isEquipped: false };
    } catch (err) {
        logger.error(`[ITEMS] Failed to add item ${itemId} to char ${characterId}: ${err.message}`);
        return null;
    }
}

// Get full inventory for a character
async function getPlayerInventory(characterId) {
    try {
        const result = await pool.query(
            `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped, ci.slot_index,
                    ci.equipped_position,
                    i.name, i.description, i.item_type, i.equip_slot, i.weight, i.price,
                    i.atk, i.def, i.matk, i.mdef,
                    i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                    i.max_hp_bonus, i.max_sp_bonus, i.required_level, i.stackable, i.icon,
                    i.weapon_type, i.aspd_modifier, i.weapon_range
             FROM character_inventory ci
             JOIN items i ON ci.item_id = i.item_id
             WHERE ci.character_id = $1
             ORDER BY ci.slot_index ASC, ci.created_at ASC`,
            [characterId]
        );
        return result.rows;
    } catch (err) {
        logger.error(`[ITEMS] Failed to load inventory for char ${characterId}: ${err.message}`);
        return [];
    }
}

// Get hotbar slot assignments for a character (joined with current inventory quantities)
// LEFT JOIN allows skill-type slots (no inventory reference) to be returned too
// Supports multi-row hotbar (4 rows of 9 slots each)
async function getPlayerHotbar(characterId) {
    try {
        const result = await pool.query(
            `SELECT COALESCE(ch.row_index, 0) as row_index,
                    ch.slot_index, ch.inventory_id, ch.item_id, ch.item_name,
                    COALESCE(ci.quantity, 0) as quantity,
                    COALESCE(ch.slot_type, 'item') as slot_type,
                    COALESCE(ch.skill_id, 0) as skill_id,
                    COALESCE(ch.skill_name, '') as skill_name
             FROM character_hotbar ch
             LEFT JOIN character_inventory ci ON ch.inventory_id = ci.inventory_id
             WHERE ch.character_id = $1
             ORDER BY ch.row_index ASC, ch.slot_index ASC`,
            [characterId]
        );
        return result.rows;
    } catch (err) {
        logger.error(`[HOTBAR] Failed to load hotbar for char ${characterId}: ${err.message}`);
        return [];
    }
}

// Remove item from inventory (by inventory_id, optionally partial quantity)
async function removeItemFromInventory(inventoryId, quantity = null) {
    try {
        if (quantity !== null) {
            // Partial removal for stackable items
            const existing = await pool.query('SELECT quantity FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
            if (existing.rows.length === 0) return false;
            const newQty = existing.rows[0].quantity - quantity;
            if (newQty <= 0) {
                await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
            } else {
                await pool.query('UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2', [newQty, inventoryId]);
            }
        } else {
            await pool.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
        }
        return true;
    } catch (err) {
        logger.error(`[ITEMS] Failed to remove inventory entry ${inventoryId}: ${err.message}`);
        return false;
    }
}

// Helper: Find player entry by socket ID
function findPlayerBySocketId(socketId) {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.socketId === socketId) {
            return { characterId: charId, player };
        }
    }
    return null;
}

// Socket.io connection handler
io.on('connection', (socket) => {
    logger.info(`Socket connected: ${socket.id}`);
    
    // Player authentication and join
    socket.on('player:join', async (data) => {
        logger.info(`[RECV] player:join from ${socket.id}: ${JSON.stringify(data)}`);
        const characterId = parseInt(data.characterId);
        const { token, characterName } = data;
        
        // Fetch character data from database (position + health/mana + zuzucoin)
        let health = 100, maxHealth = 100, mana = 100, maxMana = 100, zuzucoin = 0;
        try {
            const charResult = await pool.query(
                'SELECT x, y, z, health, max_health, mana, max_mana, zuzucoin FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (charResult.rows.length > 0) {
                const row = charResult.rows[0];
                health = row.health;
                maxHealth = row.max_health;
                mana = row.mana;
                maxMana = row.max_mana;
                zuzucoin = row.zuzucoin || 0;
                
                // Cache correct position in Redis
                await setPlayerPosition(characterId, row.x, row.y, row.z);
                // Broadcast correct position to other players immediately
                socket.broadcast.emit('player:moved', {
                    characterId,
                    characterName,
                    x: row.x,
                    y: row.y,
                    z: row.z,
                    health,
                    maxHealth,
                    timestamp: Date.now()
                });
                logger.info(`Broadcasted initial position for ${characterName} (Character ${characterId}) at (${row.x}, ${row.y}, ${row.z})`);
            }
        } catch (err) {
            logger.error(`Failed to fetch initial data for character ${characterId}:`, err.message);
        }
        
        // Default base stats (RO-style)
        const baseStats = { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1, weaponATK: 0, statPoints: 48 };
        // Default EXP/leveling data
        let jobLevel = 1, baseExp = 0, jobExp = 0, jobClass = 'novice', skillPoints = 0;
        
        // Load stats + EXP data from DB
        try {
            const statsResult = await pool.query(
                'SELECT str, agi, vit, int_stat, dex, luk, level, stat_points, job_level, base_exp, job_exp, job_class, skill_points FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (statsResult.rows.length > 0) {
                const s = statsResult.rows[0];
                if (s.str != null) baseStats.str = s.str;
                if (s.agi != null) baseStats.agi = s.agi;
                if (s.vit != null) baseStats.vit = s.vit;
                if (s.int_stat != null) baseStats.int = s.int_stat;
                if (s.dex != null) baseStats.dex = s.dex;
                if (s.luk != null) baseStats.luk = s.luk;
                if (s.level != null) baseStats.level = s.level;
                if (s.stat_points != null) baseStats.statPoints = s.stat_points;
                // EXP/leveling fields
                if (s.job_level != null) jobLevel = s.job_level;
                if (s.base_exp != null) baseExp = parseInt(s.base_exp) || 0;
                if (s.job_exp != null) jobExp = parseInt(s.job_exp) || 0;
                if (s.job_class != null) jobClass = s.job_class;
                if (s.skill_points != null) skillPoints = s.skill_points;
            }
        } catch (err) {
            logger.warn(`Could not load stats for character ${characterId}: ${err.message}`);
        }
        
        // Load equipped weapon ATK, range, and ASPD modifier
        let weaponRange = COMBAT.MELEE_RANGE;
        let weaponAspdMod = 0;
        try {
            const weaponResult = await pool.query(
                `SELECT i.atk, i.weapon_type, i.aspd_modifier, i.weapon_range FROM character_inventory ci
                 JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon'
                 LIMIT 1`,
                [characterId]
            );
            if (weaponResult.rows.length > 0) {
                const w = weaponResult.rows[0];
                baseStats.weaponATK = w.atk || 0;
                weaponRange = w.weapon_range || COMBAT.MELEE_RANGE;
                weaponAspdMod = w.aspd_modifier || 0;
                logger.info(`[ITEMS] Loaded equipped weapon: ATK=${baseStats.weaponATK}, range=${weaponRange}, aspdMod=${weaponAspdMod}, type=${w.weapon_type} for char ${characterId}`);
            }
        } catch (err) {
            logger.debug(`[ITEMS] No equipped weapon found for char ${characterId}`);
        }

        // Track weapon type for passive mastery skills
        let weaponType = null;
        try {
            const wtResult = await pool.query(
                `SELECT i.weapon_type FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = 'weapon' LIMIT 1`,
                [characterId]
            );
            if (wtResult.rows.length > 0) weaponType = wtResult.rows[0].weapon_type;
        } catch (err) { /* ignore */ }

        // Load learned skills for passive bonuses and skill usage
        const learnedSkills = {};
        try {
            const skillsResult = await pool.query(
                'SELECT skill_id, level FROM character_skills WHERE character_id = $1',
                [characterId]
            );
            for (const row of skillsResult.rows) {
                learnedSkills[row.skill_id] = row.level;
            }
            if (Object.keys(learnedSkills).length > 0) {
                logger.info(`[SKILLS] Loaded ${Object.keys(learnedSkills).length} learned skills for char ${characterId}`);
            }
        } catch (err) {
            logger.debug(`[SKILLS] Could not load learned skills for char ${characterId}: ${err.message}`);
        }

        // Load stat bonuses and hardDEF from ALL equipped items (armor, accessories, etc.)
        const equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0 };
        let hardDef = 0;
        try {
            const equipResult = await pool.query(
                `SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1 AND ci.is_equipped = true`,
                [characterId]
            );
            for (const row of equipResult.rows) {
                equipmentBonuses.str += row.str_bonus || 0;
                equipmentBonuses.agi += row.agi_bonus || 0;
                equipmentBonuses.vit += row.vit_bonus || 0;
                equipmentBonuses.int += row.int_bonus || 0;
                equipmentBonuses.dex += row.dex_bonus || 0;
                equipmentBonuses.luk += row.luk_bonus || 0;
                equipmentBonuses.maxHp += row.max_hp_bonus || 0;
                equipmentBonuses.maxSp += row.max_sp_bonus || 0;
                equipmentBonuses.hit += row.hit_bonus || 0;
                equipmentBonuses.flee += row.flee_bonus || 0;
                equipmentBonuses.critical += row.critical_bonus || 0;
                hardDef += row.def || 0;
            }
            if (hardDef > 0 || Object.values(equipmentBonuses).some(v => v > 0)) {
                logger.info(`[ITEMS] Loaded equipment bonuses for char ${characterId}: bonuses=${JSON.stringify(equipmentBonuses)} hardDef=${hardDef}`);
            }
        } catch (err) {
            logger.debug(`[ITEMS] Could not load equipment bonuses for char ${characterId}: ${err.message}`);
        }
        
        // Build a temporary player object to use getEffectiveStats for derived calculation
        const tempPlayer = { stats: baseStats, equipmentBonuses, learnedSkills, weaponType, activeBuffs: [] };
        const effectiveStats = getEffectiveStats(tempPlayer);
        const derived = calculateDerivedStats(effectiveStats);
        // Use derived maxHP/maxSP as the authoritative max values
        maxHealth = derived.maxHP;
        maxMana = derived.maxSP;
        
        // Store player connection with combat data + EXP/leveling data
        connectedPlayers.set(characterId, {
            socketId: socket.id,
            characterId: characterId,
            characterName: characterName || 'Unknown',
            health: Math.min(health, maxHealth),
            maxHealth,
            mana: Math.min(mana, maxMana),
            maxMana,
            isDead: false,
            lastAttackTime: 0,
            aspd: Math.min(COMBAT.ASPD_CAP, (derived.aspd || COMBAT.DEFAULT_ASPD) + weaponAspdMod),
            attackRange: weaponRange,
            weaponAspdMod,
            equipmentBonuses,
            hardDef,
            stats: baseStats,
            zuzucoin,
            // EXP & Leveling (RO-style dual progression)
            jobLevel,
            baseExp,
            jobExp,
            jobClass,
            skillPoints,
            // Skill combat system
            learnedSkills,
            weaponType,
            activeBuffs: [],
            skillCooldowns: {},
            // RO damage system: weapon/armor properties
            weaponElement: 'neutral',  // Default; updated when weapon has element
            weaponLevel: 1,            // Default; updated from item data
            armorElement: { type: 'neutral', level: 1 }, // Default; updated from armor
            cardMods: null             // Card % bonuses (race/element/size)
        });
        
        logger.info(`Player joined: ${characterName || 'Unknown'} (Character ${characterId}) HP: ${health}/${maxHealth} MP: ${mana}/${maxMana}`);
        const joinedPayload = { success: true, zuzucoin };
        socket.emit('player:joined', joinedPayload);
        logger.info(`[SEND] player:joined to ${socket.id}: ${JSON.stringify(joinedPayload)}`);
        
        // Send initial health state to the joining player
        const selfHealthPayload = { characterId, health, maxHealth, mana, maxMana };
        socket.emit('combat:health_update', selfHealthPayload);
        logger.info(`[SEND] combat:health_update to ${socket.id}: ${JSON.stringify(selfHealthPayload)}`);
        
        // Broadcast this player's health to others so they can show HP bars
        const broadcastHealthPayload = { characterId, health, maxHealth, mana, maxMana };
        socket.broadcast.emit('combat:health_update', broadcastHealthPayload);
        logger.info(`[BROADCAST] combat:health_update (excl ${socket.id}): ${JSON.stringify(broadcastHealthPayload)}`);
        
        // Send existing players' health to the joining player
        for (const [existingCharId, existingPlayer] of connectedPlayers.entries()) {
            if (existingCharId !== characterId) {
                socket.emit('combat:health_update', {
                    characterId: existingCharId,
                    health: existingPlayer.health,
                    maxHealth: existingPlayer.maxHealth,
                    mana: existingPlayer.mana,
                    maxMana: existingPlayer.maxMana
                });
            }
        }
        
        // Send player stats + EXP data (including stat costs for UI)
        const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + weaponAspdMod);
        const playerObj = connectedPlayers.get(characterId);
        const effectiveStatsJoin = getEffectiveStats(playerObj);
        const statsPayload = buildFullStatsPayload(characterId, playerObj, effectiveStatsJoin, derived, finalAspd);
        socket.emit('player:stats', statsPayload);
        logger.info(`[SEND] player:stats to ${socket.id} on join`);
        
        // Send hotbar data after a short delay to ensure HUD is ready
        setTimeout(async () => {
            const hotbar = await getPlayerHotbar(characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
            logger.info(`[SEND] hotbar:alldata to ${socket.id}: ${hotbar.length} slots (on join, delayed)`);
        }, 600); // 0.6 second delay
        
        // Send existing enemies to the joining player
        for (const [eid, enemy] of enemies.entries()) {
            if (!enemy.isDead) {
                socket.emit('enemy:spawn', {
                    enemyId: eid, templateId: enemy.templateId, name: enemy.name,
                    level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
                    x: enemy.x, y: enemy.y, z: enemy.z
                });
            }
        }
    });
    
    // Position update from client
    socket.on('player:position', async (data) => {
        logger.debug(`[RECV] player:position from ${socket.id}: ${JSON.stringify(data)}`);
        const characterId = parseInt(data.characterId);
        const { x, y, z } = data;
        const player = connectedPlayers.get(characterId);
        const characterName = player ? player.characterName : 'Unknown';

        // Movement cancels casting (RO: cannot move while casting)
        // Only interrupt if the player actually moved (client sends constant position updates even when idle)
        if (activeCasts.has(characterId)) {
            const lastPos = player ? { x: player.lastX, y: player.lastY, z: player.lastZ } : null;
            const MOVE_THRESHOLD = 5; // UE units — ignore sub-5-unit jitter
            if (lastPos && lastPos.x !== undefined) {
                const moveDist = Math.sqrt((x - lastPos.x) ** 2 + (y - lastPos.y) ** 2);
                if (moveDist > MOVE_THRESHOLD) {
                    interruptCast(characterId, 'moved');
                }
            }
            // If no lastPos yet, don't interrupt on the first position update
        }

        // Track last position for movement detection
        if (player) {
            player.lastX = x;
            player.lastY = y;
            player.lastZ = z;
        }

        // Cache in Redis
        await setPlayerPosition(characterId, x, y, z);
        
        // Broadcast to other players with name
        socket.broadcast.emit('player:moved', {
            characterId,
            characterName,
            x, y, z,
            health: player ? player.health : 100,
            maxHealth: player ? player.maxHealth : 100,
            timestamp: Date.now()
        });
        logger.debug(`[BROADCAST] player:moved for ${characterName} (Character ${characterId})`);
    });
    
    // Disconnect
    socket.on('disconnect', async () => {
        logger.info(`[RECV] disconnect from ${socket.id}`);
        logger.info(`Connected players count: ${connectedPlayers.size}`);
        
        // Remove from connected players
        for (const [charId, player] of connectedPlayers.entries()) {
            logger.info(`Checking player ${charId} with socket ${player.socketId} against ${socket.id}`);
            if (player.socketId === socket.id) {
                // Clean up active cast on disconnect
                activeCasts.delete(charId);
                afterCastDelayEnd.delete(charId);

                // Save health/mana to DB on disconnect
                await savePlayerHealthToDB(charId, player.health, player.mana);
                
                // Save stats + EXP data to DB on disconnect
                if (player.stats) {
                    try {
                        await pool.query(
                            `UPDATE characters SET str = $1, agi = $2, vit = $3, int_stat = $4, dex = $5, luk = $6, stat_points = $7,
                             level = $9, job_level = $10, base_exp = $11, job_exp = $12, job_class = $13, skill_points = $14
                             WHERE character_id = $8`,
                            [player.stats.str, player.stats.agi, player.stats.vit, player.stats.int, player.stats.dex, player.stats.luk, player.stats.statPoints, charId,
                             player.stats.level || 1, player.jobLevel || 1, player.baseExp || 0, player.jobExp || 0, player.jobClass || 'novice', player.skillPoints || 0]
                        );
                        logger.info(`[DB] Saved stats + EXP for character ${charId} on disconnect (BLv${player.stats.level} JLv${player.jobLevel} [${player.jobClass}])`);
                    } catch (err) {
                        logger.error(`[DB] Failed to save stats for character ${charId}:`, err.message);
                    }
                }
                
                // Clear this player's auto-attack
                autoAttackState.delete(charId);
                
                // Clear anyone auto-attacking this player
                for (const [attackerId, atkState] of autoAttackState.entries()) {
                    if (atkState.targetCharId === charId && !atkState.isEnemy) {
                        autoAttackState.delete(attackerId);
                        const attackerPlayer = connectedPlayers.get(attackerId);
                        if (attackerPlayer) {
                            const attackerSocket = io.sockets.sockets.get(attackerPlayer.socketId);
                            if (attackerSocket) {
                                const lostPayload = { reason: 'Target disconnected', isEnemy: false };
                                attackerSocket.emit('combat:target_lost', lostPayload);
                                logger.info(`[SEND] combat:target_lost to ${attackerPlayer.socketId}: ${JSON.stringify(lostPayload)}`);
                            }
                        }
                    }
                }
                
                // Remove this player from enemy combat sets
                for (const [, enemy] of enemies.entries()) {
                    enemy.inCombatWith.delete(charId);
                }
                
                connectedPlayers.delete(charId);
                logger.info(`Player left: Character ${charId}`);
                
                // Broadcast to other players using io (socket is already disconnected)
                const leftPayload = { characterId: charId, characterName: player.characterName || 'Unknown' };
                io.emit('player:left', leftPayload);
                logger.info(`[BROADCAST] player:left: ${JSON.stringify(leftPayload)}`);
                break;
            }
        }
    });
    
    // Combat: Start auto-attack (RO-style: click once → path → auto-attack loop)
    // Supports both player targets (targetCharacterId) and enemy targets (targetEnemyId)
    socket.on('combat:attack', (data) => {
        logger.info(`[RECV] combat:attack from ${socket.id}: ${JSON.stringify(data)}`);
        const targetCharacterId = data.targetCharacterId != null ? parseInt(data.targetCharacterId) : undefined;
        const targetEnemyId = data.targetEnemyId != null ? parseInt(data.targetEnemyId) : undefined;
        
        const attackerInfo = findPlayerBySocketId(socket.id);
        if (!attackerInfo) {
            logger.warn(`Attack from unknown socket: ${socket.id}`);
            return;
        }
        
        const { characterId: attackerId, player: attacker } = attackerInfo;
        
        if (attacker.isDead) {
            socket.emit('combat:error', { message: 'You are dead' });
            return;
        }
        
        // --- Clean up previous auto-attack if switching targets ---
        if (autoAttackState.has(attackerId)) {
            const oldAtk = autoAttackState.get(attackerId);
            const newTargetId = targetEnemyId != null ? targetEnemyId : targetCharacterId;
            const newIsEnemy = targetEnemyId != null;
            
            // Only clean up if actually switching to a different target
            if (oldAtk.targetCharId !== newTargetId || oldAtk.isEnemy !== newIsEnemy) {
                // Remove from old enemy's combat set
                if (oldAtk.isEnemy) {
                    const oldEnemy = enemies.get(oldAtk.targetCharId);
                    if (oldEnemy) {
                        oldEnemy.inCombatWith.delete(attackerId);
                        logger.info(`[COMBAT] ${attacker.characterName} switched targets — removed from enemy ${oldEnemy.name}(${oldEnemy.enemyId}) combat set`);
                    }
                }
                autoAttackState.delete(attackerId);
                
                // Notify client to clean up old target (hide hover indicator, etc.)
                const stopPayload = { reason: 'Switched target', oldTargetId: oldAtk.targetCharId, oldIsEnemy: oldAtk.isEnemy };
                socket.emit('combat:auto_attack_stopped', stopPayload);
                logger.info(`[SEND] combat:auto_attack_stopped to ${socket.id} (target switch): ${JSON.stringify(stopPayload)}`);
            }
        }
        
        // --- Attacking an enemy (NPC/AI) ---
        if (targetEnemyId != null) {
            const enemy = enemies.get(targetEnemyId);
            if (!enemy) {
                socket.emit('combat:error', { message: 'Enemy not found' });
                return;
            }
            if (enemy.isDead) {
                socket.emit('combat:error', { message: 'Enemy is already dead' });
                return;
            }
            
            autoAttackState.set(attackerId, {
                targetCharId: targetEnemyId,
                isEnemy: true,
                startTime: Date.now()
            });
            
            // Mark enemy in combat with this player and stop wandering
            enemy.inCombatWith.add(attackerId);
            enemy.isWandering = false;
            
            const atkStartPayload = {
                targetId: targetEnemyId,
                targetName: enemy.name,
                isEnemy: true,
                attackRange: attacker.attackRange,
                aspd: attacker.aspd,
                attackIntervalMs: getAttackIntervalMs(attacker.aspd)
            };
            socket.emit('combat:auto_attack_started', atkStartPayload);
            logger.info(`[SEND] combat:auto_attack_started to ${socket.id}: ${JSON.stringify(atkStartPayload)}`);
            logger.info(`[COMBAT] ${attacker.characterName} started auto-attacking enemy ${enemy.name} (ID: ${targetEnemyId})`);
            return;
        }
        
        // --- Attacking a player ---
        if (!PVP_ENABLED) {
            socket.emit('combat:error', { message: 'PvP is currently disabled' });
            return;
        }
        if (attackerId === targetCharacterId) {
            socket.emit('combat:error', { message: 'Cannot attack yourself' });
            return;
        }
        
        const target = connectedPlayers.get(targetCharacterId);
        if (!target) {
            socket.emit('combat:error', { message: 'Target not found' });
            return;
        }
        
        if (target.isDead) {
            socket.emit('combat:error', { message: 'Target is already dead' });
            return;
        }
        
        // Set auto-attack state (server handles attack timing via combat tick)
        autoAttackState.set(attackerId, {
            targetCharId: targetCharacterId,
            isEnemy: false,
            startTime: Date.now()
        });
        
        // Confirm auto-attack started
        const atkStartPayload = {
            targetId: targetCharacterId,
            targetName: target.characterName,
            isEnemy: false,
            attackRange: attacker.attackRange,
            aspd: attacker.aspd,
            attackIntervalMs: getAttackIntervalMs(attacker.aspd)
        };
        socket.emit('combat:auto_attack_started', atkStartPayload);
        logger.info(`[SEND] combat:auto_attack_started to ${socket.id}: ${JSON.stringify(atkStartPayload)}`);
        
        logger.info(`[COMBAT] ${attacker.characterName} started auto-attacking ${target.characterName} (ASPD: ${attacker.aspd}, Interval: ${getAttackIntervalMs(attacker.aspd)}ms, Range: ${attacker.attackRange})`);
    });
    
    // Combat: Stop auto-attack
    socket.on('combat:stop_attack', () => {
        logger.info(`[RECV] combat:stop_attack from ${socket.id}`);
        const attackerInfo = findPlayerBySocketId(socket.id);
        if (!attackerInfo) {
            logger.warn(`[COMBAT] combat:stop_attack from unknown socket ${socket.id} — no player found`);
            return;
        }
        
        const { characterId: attackerId, player: attacker } = attackerInfo;
        
        if (autoAttackState.has(attackerId)) {
            const atkState = autoAttackState.get(attackerId);
            logger.info(`[COMBAT] ${attacker.characterName} stopping auto-attack (target: ${atkState.targetCharId}, isEnemy: ${atkState.isEnemy})`);
            // Remove from enemy combat set if attacking an enemy
            if (atkState.isEnemy) {
                const enemy = enemies.get(atkState.targetCharId);
                if (enemy) {
                    enemy.inCombatWith.delete(attackerId);
                    logger.info(`[COMBAT] Removed ${attacker.characterName} from enemy ${enemy.name}(${enemy.enemyId}) combat set`);
                }
            }
            autoAttackState.delete(attackerId);
            const stopPayload = { reason: 'Player stopped' };
            socket.emit('combat:auto_attack_stopped', stopPayload);
            logger.info(`[SEND] combat:auto_attack_stopped to ${socket.id}: ${JSON.stringify(stopPayload)}`);
        } else {
            logger.info(`[COMBAT] ${attacker.characterName} sent combat:stop_attack but had no active auto-attack state`);
        }
    });
    
    // Combat: Respawn handler
    socket.on('combat:respawn', async (data) => {
        logger.info(`[RECV] combat:respawn from ${socket.id}: ${JSON.stringify(data || {})}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        
        if (!player.isDead) {
            socket.emit('combat:error', { message: 'You are not dead' });
            return;
        }
        
        // Stop all attackers targeting this dead player
        for (const [attackerId, atkState] of autoAttackState.entries()) {
            if (atkState.targetCharId === characterId && !atkState.isEnemy) {
                autoAttackState.delete(attackerId);
                const attackerPlayer = connectedPlayers.get(attackerId);
                if (attackerPlayer) {
                    const attackerSocket = io.sockets.sockets.get(attackerPlayer.socketId);
                    if (attackerSocket) {
                        const lostPayload = { reason: 'Target respawned', isEnemy: false };
                        attackerSocket.emit('combat:target_lost', lostPayload);
                        logger.info(`[SEND] combat:target_lost to ${attackerPlayer.socketId}: ${JSON.stringify(lostPayload)}`);
                    }
                }
            }
        }
        
        // Restore health to full
        player.health = player.maxHealth;
        player.mana = player.maxMana;
        player.isDead = false;
        
        // Save health to database
        await savePlayerHealthToDB(characterId, player.health, player.mana);
        
        // Update position cache to spawn point
        await setPlayerPosition(characterId, COMBAT.SPAWN_POSITION.x, COMBAT.SPAWN_POSITION.y, COMBAT.SPAWN_POSITION.z);
        
        logger.info(`[COMBAT] ${player.characterName} respawned at (${COMBAT.SPAWN_POSITION.x}, ${COMBAT.SPAWN_POSITION.y}, ${COMBAT.SPAWN_POSITION.z})`);
        
        // Notify all players of respawn with teleport flag
        const respawnPayload = {
            characterId,
            characterName: player.characterName,
            health: player.health,
            maxHealth: player.maxHealth,
            mana: player.mana,
            maxMana: player.maxMana,
            x: COMBAT.SPAWN_POSITION.x,
            y: COMBAT.SPAWN_POSITION.y,
            z: COMBAT.SPAWN_POSITION.z,
            teleport: true,
            timestamp: Date.now()
        };
        io.emit('combat:respawn', respawnPayload);
        logger.info(`[BROADCAST] combat:respawn: ${JSON.stringify(respawnPayload)}`);
    });
    
    // Request current stats (used when opening stat window)
    socket.on('player:request_stats', () => {
        logger.info(`[RECV] player:request_stats from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const effectiveStats = getEffectiveStats(player);
        const derived = calculateDerivedStats(effectiveStats);
        const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));

        socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
        logger.info(`[SEND] player:stats to ${socket.id} on request_stats`);

        // Also re-send current health so late-binding UIs get correct HP/SP
        socket.emit('combat:health_update', {
            characterId,
            health: player.health,
            maxHealth: player.maxHealth || derived.maxHP,
            mana: player.mana,
            maxMana: player.maxMana || derived.maxSP
        });
    });
    
    // Stat allocation handler (RO Classic: cost = floor((currentStat - 1) / 10) + 2, max 99)
    socket.on('player:allocate_stat', async (data) => {
        logger.info(`[RECV] player:allocate_stat from ${socket.id}: ${JSON.stringify(data)}`);
        const { statName } = data;

        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;

        const validStats = ['str', 'agi', 'vit', 'int', 'dex', 'luk'];
        if (!validStats.includes(statName)) {
            socket.emit('combat:error', { message: `Invalid stat: ${statName}` });
            return;
        }

        const statKey = statName === 'int' ? 'int' : statName;
        const currentValue = player.stats[statKey] || 1;

        // RO Classic: max base stat is 99
        if (currentValue >= 99) {
            socket.emit('combat:error', { message: `${statName.toUpperCase()} is already at maximum (99)` });
            return;
        }

        // RO Classic cost formula: floor((currentStat - 1) / 10) + 2
        const cost = Math.floor((currentValue - 1) / 10) + 2;

        if (!player.stats || (player.stats.statPoints || 0) < cost) {
            socket.emit('combat:error', { message: `Not enough stat points (need ${cost}, have ${player.stats.statPoints || 0})` });
            return;
        }

        // Apply: increase stat by 1, deduct cost
        player.stats[statKey] = currentValue + 1;
        player.stats.statPoints -= cost;

        logger.info(`[STATS] ${player.characterName} allocated ${statName.toUpperCase()}: ${currentValue} → ${currentValue + 1} (cost: ${cost}, remaining: ${player.stats.statPoints})`);

        // Recalculate derived stats using EFFECTIVE stats (base + equipment bonuses)
        const effectiveStats = getEffectiveStats(player);
        const derived = calculateDerivedStats(effectiveStats);
        player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
        player.maxHealth = derived.maxHP;
        player.maxMana = derived.maxSP;

        // Save to DB
        try {
            const dbStatName = statName === 'int' ? 'int_stat' : statName;
            await pool.query(
                `UPDATE characters SET ${dbStatName} = $1, stat_points = $2, max_health = $3, max_mana = $4 WHERE character_id = $5`,
                [player.stats[statKey], player.stats.statPoints, player.maxHealth, player.maxMana, characterId]
            );
        } catch (err) {
            logger.error(`Failed to save stat allocation for character ${characterId}:`, err.message);
        }

        // Send effective values so stat window shows total (base + equipment bonus)
        const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
        const statsPayload = buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd);
        socket.emit('player:stats', statsPayload);
        logger.info(`[SEND] player:stats to ${socket.id} after stat allocation`);
    });
    
    // ============================================================
    // Job Change System (Novice → First Class → Second Class)
    // ============================================================
    socket.on('job:change', async (data) => {
        logger.info(`[RECV] job:change from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        const { targetClass } = data;
        
        if (!targetClass) {
            socket.emit('job:error', { message: 'No target class specified' });
            return;
        }
        
        const currentClass = player.jobClass || 'novice';
        const currentTier = getClassTier(currentClass);
        const targetTier = getClassTier(targetClass);
        const currentJobLevel = player.jobLevel || 1;
        
        // Validate: target must be exactly one tier above current
        if (targetTier !== currentTier + 1) {
            socket.emit('job:error', { message: `Cannot change from ${currentClass} to ${targetClass}` });
            return;
        }
        
        // Validate: novice → first class requires job level 10
        if (currentTier === 0) {
            if (currentJobLevel < 10) {
                socket.emit('job:error', { message: `Requires Novice Job Level 10 (current: ${currentJobLevel})` });
                return;
            }
            if (!FIRST_CLASSES.includes(targetClass)) {
                socket.emit('job:error', { message: `${targetClass} is not a valid first class` });
                return;
            }
        }
        
        // Validate: first class → second class requires job level 40+
        if (currentTier === 1) {
            if (currentJobLevel < 40) {
                socket.emit('job:error', { message: `Requires Job Level 40+ (current: ${currentJobLevel})` });
                return;
            }
            const validUpgrades = SECOND_CLASS_UPGRADES[currentClass];
            if (!validUpgrades || !validUpgrades.includes(targetClass)) {
                socket.emit('job:error', { message: `${targetClass} is not a valid upgrade from ${currentClass}` });
                return;
            }
        }
        
        // Perform job change: reset job level and job exp, keep base level
        const oldClass = currentClass;
        const oldJobLevel = currentJobLevel;
        player.jobClass = targetClass;
        player.jobLevel = 1;
        player.jobExp = 0;
        // Skill points are kept (accumulated from previous class + new class)
        
        // Save to DB
        try {
            await pool.query(
                `UPDATE characters SET job_class = $1, job_level = $2, job_exp = $3 WHERE character_id = $4`,
                [player.jobClass, player.jobLevel, player.jobExp, characterId]
            );
        } catch (err) {
            logger.error(`[DB] Failed to save job change for character ${characterId}: ${err.message}`);
        }
        
        logger.info(`[JOB] ${player.characterName} changed class: ${oldClass} (JLv${oldJobLevel}) → ${targetClass} (JLv1)`);
        
        // Send job change confirmation to client
        socket.emit('job:changed', {
            characterId,
            oldClass,
            newClass: targetClass,
            newClassDisplayName: (JOB_CLASS_CONFIG[targetClass] || {}).displayName || targetClass,
            jobLevel: 1,
            jobExp: 0,
            jobExpNext: getJobExpForNextLevel(targetClass, 1),
            maxJobLevel: getMaxJobLevel(targetClass),
            skillPoints: player.skillPoints || 0,
            exp: buildExpPayload(player)
        });
        
        // Broadcast to other players
        socket.broadcast.emit('job:changed', {
            characterId,
            characterName: player.characterName,
            newClass: targetClass,
            newClassDisplayName: (JOB_CLASS_CONFIG[targetClass] || {}).displayName || targetClass
        });
        
        // Chat announcement
        io.emit('chat:receive', {
            type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
            message: `${player.characterName} has become a ${(JOB_CLASS_CONFIG[targetClass] || {}).displayName || targetClass}!`,
            timestamp: Date.now()
        });
        
        // Send updated stats with new EXP data
        const effectiveStats = getEffectiveStats(player);
        const derived = calculateDerivedStats(effectiveStats);
        const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
        socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
    });

    // ============================================================
    // Skill System Events
    // ============================================================

    // Send skill tree data for the player's class (all available skills + learned levels)
    socket.on('skill:data', async () => {
        logger.info(`[RECV] skill:data from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const jobClass = player.jobClass || 'novice';

        // Get all skills available for this class chain
        const availableSkills = getAvailableSkills(jobClass);

        // Load character's learned skills from DB
        let learnedSkills = {};
        try {
            const result = await pool.query(
                'SELECT skill_id, level FROM character_skills WHERE character_id = $1',
                [characterId]
            );
            for (const row of result.rows) {
                learnedSkills[row.skill_id] = row.level;
            }
        } catch (err) {
            logger.warn(`[SKILLS] Could not load skills for char ${characterId}: ${err.message}`);
        }

        // Build skill tree payload grouped by class
        const skillTree = {};
        for (const skill of availableSkills) {
            if (!skillTree[skill.classId]) skillTree[skill.classId] = [];
            const currentLevel = learnedSkills[skill.id] || 0;
            const levelData = skill.levels[Math.min(currentLevel, skill.levels.length - 1)] || skill.levels[0];
            const nextLevelData = currentLevel < skill.maxLevel ? skill.levels[currentLevel] : null;

            skillTree[skill.classId].push({
                skillId: skill.id,
                name: skill.name,
                displayName: skill.displayName,
                maxLevel: skill.maxLevel,
                currentLevel,
                type: skill.type,
                targetType: skill.targetType,
                element: skill.element,
                range: skill.range,
                description: skill.description,
                icon: skill.icon,
                treeRow: skill.treeRow,
                treeCol: skill.treeCol,
                prerequisites: skill.prerequisites,
                spCost: levelData ? levelData.spCost : 0,
                nextSpCost: nextLevelData ? nextLevelData.spCost : 0,
                castTime: levelData ? levelData.castTime : 0,
                cooldown: levelData ? levelData.cooldown : 0,
                effectValue: levelData ? levelData.effectValue : 0,
                canLearn: canLearnSkill(skill.id, learnedSkills, jobClass, player.skillPoints || 0).ok
            });
        }

        const payload = {
            characterId,
            jobClass,
            skillPoints: player.skillPoints || 0,
            skillTree,
            learnedSkills
        };
        socket.emit('skill:data', payload);
        logger.info(`[SEND] skill:data to ${socket.id}: ${Object.keys(skillTree).length} class trees, ${Object.keys(learnedSkills).length} learned skills, ${player.skillPoints || 0} points`);
    });

    // Learn or level up a skill
    socket.on('skill:learn', async (data) => {
        logger.info(`[RECV] skill:learn from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const skillId = parseInt(data.skillId);
        if (isNaN(skillId)) {
            socket.emit('skill:error', { message: 'Invalid skill ID' });
            return;
        }

        // Load current learned skills from DB
        let learnedSkills = {};
        try {
            const result = await pool.query(
                'SELECT skill_id, level FROM character_skills WHERE character_id = $1',
                [characterId]
            );
            for (const row of result.rows) {
                learnedSkills[row.skill_id] = row.level;
            }
        } catch (err) {
            logger.error(`[SKILLS] Failed to load skills for validation: ${err.message}`);
            socket.emit('skill:error', { message: 'Failed to load skills' });
            return;
        }

        // Validate
        const jobClass = player.jobClass || 'novice';
        const validation = canLearnSkill(skillId, learnedSkills, jobClass, player.skillPoints || 0);
        if (!validation.ok) {
            socket.emit('skill:error', { message: validation.reason });
            logger.info(`[SKILLS] ${player.characterName} cannot learn skill ${skillId}: ${validation.reason}`);
            return;
        }

        const skill = SKILL_MAP.get(skillId);
        const currentLevel = learnedSkills[skillId] || 0;
        const newLevel = currentLevel + 1;

        // Update DB
        try {
            await pool.query(`
                INSERT INTO character_skills (character_id, skill_id, level)
                VALUES ($1, $2, $3)
                ON CONFLICT (character_id, skill_id) DO UPDATE SET level = $3
            `, [characterId, skillId, newLevel]);

            // Deduct 1 skill point
            player.skillPoints = Math.max(0, (player.skillPoints || 0) - 1);
            await pool.query(
                'UPDATE characters SET skill_points = $1 WHERE character_id = $2',
                [player.skillPoints, characterId]
            );

            // Update in-memory learned skills for passive bonuses
            if (!player.learnedSkills) player.learnedSkills = {};
            player.learnedSkills[skillId] = newLevel;

            logger.info(`[SKILLS] ${player.characterName} learned ${skill.displayName} Lv${newLevel} (${player.skillPoints} points remaining)`);

            socket.emit('skill:learned', {
                skillId,
                skillName: skill.displayName,
                newLevel,
                maxLevel: skill.maxLevel,
                skillPoints: player.skillPoints
            });

            // Send updated full skill data
            socket.emit('skill:refresh', { skillPoints: player.skillPoints });
        } catch (err) {
            logger.error(`[SKILLS] Failed to save skill ${skillId} for char ${characterId}: ${err.message}`);
            socket.emit('skill:error', { message: 'Failed to learn skill' });
        }
    });

    // Reset all skills (costs Zeny in RO, here we'll make it free for now)
    socket.on('skill:reset', async () => {
        logger.info(`[RECV] skill:reset from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;

        try {
            // Count total skill points currently invested
            const result = await pool.query(
                'SELECT COALESCE(SUM(level), 0) as total_points FROM character_skills WHERE character_id = $1',
                [characterId]
            );
            const refundedPoints = parseInt(result.rows[0].total_points) || 0;

            // Delete all learned skills
            await pool.query('DELETE FROM character_skills WHERE character_id = $1', [characterId]);

            // Refund skill points
            player.skillPoints = (player.skillPoints || 0) + refundedPoints;
            player.learnedSkills = {}; // Clear in-memory learned skills
            player.activeBuffs = [];   // Clear any active skill buffs
            await pool.query(
                'UPDATE characters SET skill_points = $1 WHERE character_id = $2',
                [player.skillPoints, characterId]
            );

            logger.info(`[SKILLS] ${player.characterName} reset all skills. Refunded ${refundedPoints} points (total: ${player.skillPoints})`);

            socket.emit('skill:reset_complete', {
                skillPoints: player.skillPoints,
                refundedPoints
            });
        } catch (err) {
            logger.error(`[SKILLS] Reset failed for char ${characterId}: ${err.message}`);
            socket.emit('skill:error', { message: 'Failed to reset skills' });
        }
    });

    // Chat message handler (expandable for multiple channels)
    socket.on('chat:message', (data) => {
        logger.info(`[RECV] chat:message from ${socket.id}: ${JSON.stringify(data)}`);
        const { channel, message } = data;
        
        // Find the player by socket ID
        const playerInfo = findPlayerBySocketId(socket.id);
        let player = playerInfo ? playerInfo.player : null;
        let characterId = playerInfo ? playerInfo.characterId : null;
        
        if (!player) {
            logger.warn(`Chat message from unknown socket: ${socket.id}`);
            return;
        }
        
        // Validate message
        if (!message || message.trim().length === 0) {
            return;
        }
        
        const chatMessage = {
            type: 'chat:receive',
            channel: channel || 'GLOBAL',
            senderId: characterId,
            senderName: player.characterName || 'Unknown',
            message: message.trim(),
            timestamp: Date.now()
        };
        
        // Route based on channel
        switch (chatMessage.channel) {
            case 'GLOBAL':
                // Broadcast to all connected players
                io.emit('chat:receive', chatMessage);
                logger.info(`[CHAT GLOBAL] ${chatMessage.senderName}: ${message}`);
                break;
            
            // Future channels (expandable):
            // case 'ZONE':
            //     // Broadcast to players in same zone
            //     break;
            // case 'PARTY':
            //     // Broadcast to party members
            //     break;
            // case 'GUILD':
            //     // Broadcast to guild members
            //     break;
            // case 'TELL':
            //     // Private message to specific player
            //     break;
            
            default:
                // Default to GLOBAL for unknown channels
                io.emit('chat:receive', chatMessage);
                logger.info(`[CHAT ${chatMessage.channel}] ${chatMessage.senderName}: ${message}`);
        }
    });
    
    // ============================================================
    // Inventory Events
    // ============================================================
    
    // Load full inventory
    socket.on('inventory:load', async () => {
        logger.info(`[RECV] inventory:load from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const inventory = await getPlayerInventory(playerInfo.characterId);
        socket.emit('inventory:data', { items: inventory, zuzucoin: playerInfo.player.zuzucoin });
        logger.info(`[SEND] inventory:data to ${socket.id}: ${inventory.length} items, zuzucoin=${playerInfo.player.zuzucoin}`);

        // Also send hotbar state so client can restore hotbar after reconnect
        const hotbar = await getPlayerHotbar(playerInfo.characterId);
        socket.emit('hotbar:alldata', { slots: hotbar });
        logger.info(`[SEND] hotbar:alldata to ${socket.id}: ${hotbar.length} slots`);
    });

    // Save a single hotbar slot assignment (client → server, fired when item dragged to hotbar)
    // Supports multi-row: data.rowIndex (0-3, default 0), data.slotIndex (0-8)
    socket.on('hotbar:save', async (data) => {
        logger.info(`[RECV] hotbar:save from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId } = playerInfo;
        const rowIndex = parseInt(data.rowIndex) || 0;
        const slotIndex = parseInt(data.slotIndex);
        const inventoryId = parseInt(data.inventoryId);
        const itemId = parseInt(data.itemId);
        const itemName = data.itemName || '';

        if (isNaN(rowIndex) || rowIndex < 0 || rowIndex > 3) {
            logger.warn(`[HOTBAR] Invalid rowIndex ${data.rowIndex} from char ${characterId}`);
            return;
        }
        if (isNaN(slotIndex) || slotIndex < 0 || slotIndex > 8) {
            logger.warn(`[HOTBAR] Invalid slotIndex ${data.slotIndex} from char ${characterId}`);
            return;
        }

        try {
            if (!inventoryId || inventoryId <= 0) {
                // Clear the slot
                await pool.query(
                    'DELETE FROM character_hotbar WHERE character_id = $1 AND row_index = $2 AND slot_index = $3',
                    [characterId, rowIndex, slotIndex]
                );
                logger.info(`[HOTBAR] Char ${characterId} cleared row ${rowIndex} slot ${slotIndex}`);
            } else {
                // Verify this inventory item belongs to this character
                const verify = await pool.query(
                    'SELECT inventory_id FROM character_inventory WHERE inventory_id = $1 AND character_id = $2',
                    [inventoryId, characterId]
                );
                if (verify.rows.length === 0) {
                    logger.warn(`[HOTBAR] Char ${characterId} tried to save inv_id ${inventoryId} they don't own`);
                    return;
                }

                // UPSERT: insert or update the slot (clear skill fields, set item fields)
                await pool.query(
                    `INSERT INTO character_hotbar (character_id, row_index, slot_index, inventory_id, item_id, item_name, slot_type, skill_id, skill_name)
                     VALUES ($1, $2, $3, $4, $5, $6, 'item', NULL, NULL)
                     ON CONFLICT (character_id, row_index, slot_index)
                     DO UPDATE SET inventory_id = $4, item_id = $5, item_name = $6, slot_type = 'item', skill_id = NULL, skill_name = NULL`,
                    [characterId, rowIndex, slotIndex, inventoryId, itemId, itemName]
                );
                logger.info(`[HOTBAR] Char ${characterId} saved row ${rowIndex} slot ${slotIndex}: inv_id=${inventoryId} item=${itemName}`);
            }

            // Send updated hotbar to client
            const hotbar = await getPlayerHotbar(characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
        } catch (err) {
            logger.error(`[HOTBAR] Save failed for char ${characterId}: ${err.message}`);
        }
    });

    // Request hotbar data (client → server, used when C++ binds events after initial join)
    socket.on('hotbar:request', async () => {
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        try {
            const hotbar = await getPlayerHotbar(playerInfo.characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
            logger.info(`[SEND] hotbar:alldata to ${socket.id}: ${hotbar.length} slots (re-request)`);
        } catch (err) {
            logger.error(`[HOTBAR] Re-request failed for char ${playerInfo.characterId}: ${err.message}`);
        }
    });

    // Save a skill to a hotbar slot (client → server, fired from skill tree quick-assign)
    // Supports multi-row: data.rowIndex (0-3, default 0)
    // slotIndex accepts 0-8 (0-based from new HotbarSubsystem) or 1-9 (1-based legacy from SkillTreeSubsystem)
    socket.on('hotbar:save_skill', async (data) => {
        logger.info(`[RECV] hotbar:save_skill from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId } = playerInfo;
        const rowIndex = parseInt(data.rowIndex) || 0;
        let slotIndex = parseInt(data.slotIndex);
        const skillId = parseInt(data.skillId);
        const skillName = data.skillName || '';

        if (isNaN(rowIndex) || rowIndex < 0 || rowIndex > 3) {
            logger.warn(`[HOTBAR] Invalid rowIndex ${data.rowIndex} from char ${characterId}`);
            return;
        }

        // Normalize slotIndex: accept 1-9 (legacy) or 0-8 (new), store as 0-8
        if (slotIndex >= 1 && slotIndex <= 9 && !data.zeroBased) {
            slotIndex = slotIndex - 1; // Convert 1-based to 0-based
        }
        if (isNaN(slotIndex) || slotIndex < 0 || slotIndex > 8) {
            logger.warn(`[HOTBAR] Invalid slotIndex ${data.slotIndex} from char ${characterId}`);
            return;
        }

        try {
            if (!skillId || skillId <= 0) {
                // Clear the slot
                await pool.query(
                    'DELETE FROM character_hotbar WHERE character_id = $1 AND row_index = $2 AND slot_index = $3',
                    [characterId, rowIndex, slotIndex]
                );
                logger.info(`[HOTBAR] Char ${characterId} cleared skill row ${rowIndex} slot ${slotIndex}`);
            } else {
                // Verify player has this skill learned
                const verify = await pool.query(
                    'SELECT level FROM character_skills WHERE character_id = $1 AND skill_id = $2',
                    [characterId, skillId]
                );
                if (verify.rows.length === 0) {
                    logger.warn(`[HOTBAR] Char ${characterId} tried to assign unlearned skill ${skillId}`);
                    return;
                }

                // UPSERT the skill slot (clear item fields, set skill fields)
                await pool.query(
                    `INSERT INTO character_hotbar (character_id, row_index, slot_index, inventory_id, item_id, item_name, slot_type, skill_id, skill_name)
                     VALUES ($1, $2, $3, NULL, NULL, '', 'skill', $4, $5)
                     ON CONFLICT (character_id, row_index, slot_index)
                     DO UPDATE SET inventory_id = NULL, item_id = NULL, item_name = '', slot_type = 'skill', skill_id = $4, skill_name = $5`,
                    [characterId, rowIndex, slotIndex, skillId, skillName]
                );
                logger.info(`[HOTBAR] Char ${characterId} assigned skill ${skillName} (${skillId}) to row ${rowIndex} slot ${slotIndex}`);
            }

            // Send updated hotbar to client
            const hotbar = await getPlayerHotbar(characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
        } catch (err) {
            logger.error(`[HOTBAR] Save skill failed for char ${characterId}: ${err.message}`);
        }
    });

    // Clear a hotbar slot (client → server)
    socket.on('hotbar:clear', async (data) => {
        logger.info(`[RECV] hotbar:clear from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId } = playerInfo;
        const rowIndex = parseInt(data.rowIndex) || 0;
        const slotIndex = parseInt(data.slotIndex);

        if (isNaN(rowIndex) || rowIndex < 0 || rowIndex > 3) return;
        if (isNaN(slotIndex) || slotIndex < 0 || slotIndex > 8) return;

        try {
            await pool.query(
                'DELETE FROM character_hotbar WHERE character_id = $1 AND row_index = $2 AND slot_index = $3',
                [characterId, rowIndex, slotIndex]
            );
            logger.info(`[HOTBAR] Char ${characterId} cleared row ${rowIndex} slot ${slotIndex}`);

            const hotbar = await getPlayerHotbar(characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
        } catch (err) {
            logger.error(`[HOTBAR] Clear failed for char ${characterId}: ${err.message}`);
        }
    });

    // Use a skill from the hotbar (client → server)
    // Supports: { skillId } for self/auto-target, { skillId, targetId, isEnemy } for explicit target
    socket.on('skill:use', async (data) => {
        logger.info(`[RECV] skill:use from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const skillId = parseInt(data.skillId);

        if (isNaN(skillId)) {
            socket.emit('skill:error', { message: 'Invalid skill ID' });
            return;
        }

        if (player.isDead) {
            socket.emit('skill:error', { message: 'You are dead' });
            return;
        }

        // Check CC (cannot cast while frozen, stoned, stunned, silenced)
        const ccMods = getBuffStatModifiers(player);
        if (ccMods.isFrozen) { socket.emit('skill:error', { message: 'Cannot use skills while frozen' }); return; }
        if (ccMods.isStoned) { socket.emit('skill:error', { message: 'Cannot use skills while petrified' }); return; }

        // Check if already casting
        if (activeCasts.has(characterId)) {
            socket.emit('skill:error', { message: 'Already casting a skill' });
            return;
        }

        // Check After-Cast Delay (global skill lockout)
        const acdEnd = afterCastDelayEnd.get(characterId) || 0;
        if (acdEnd > Date.now()) {
            const remaining = Math.ceil((acdEnd - Date.now()) / 1000);
            socket.emit('skill:error', { message: `Please wait (${remaining}s)` });
            return;
        }

        // Look up skill definition
        const skill = SKILL_MAP.get(skillId);
        if (!skill) {
            socket.emit('skill:error', { message: 'Unknown skill' });
            return;
        }

        // Check if player has learned this skill (use in-memory first, DB fallback)
        let learnedLevel = (player.learnedSkills || {})[skillId] || 0;
        if (learnedLevel <= 0) {
            try {
                const result = await pool.query(
                    'SELECT level FROM character_skills WHERE character_id = $1 AND skill_id = $2',
                    [characterId, skillId]
                );
                if (result.rows.length > 0) {
                    learnedLevel = result.rows[0].level;
                    if (!player.learnedSkills) player.learnedSkills = {};
                    player.learnedSkills[skillId] = learnedLevel;
                }
            } catch (err) {
                logger.error(`[SKILLS] Failed to check skill ${skillId}: ${err.message}`);
                return;
            }
        }

        if (learnedLevel <= 0) {
            socket.emit('skill:error', { message: 'Skill not learned' });
            return;
        }

        // Get SP cost and effect at current level
        const levelData = skill.levels[Math.min(learnedLevel - 1, skill.levels.length - 1)];
        const spCost = levelData ? levelData.spCost : 0;
        const cooldownMs = levelData ? levelData.cooldown : 0;
        const effectVal = levelData ? levelData.effectValue : 0;
        const duration = levelData ? levelData.duration : 0;

        // Check per-skill cooldown
        if (cooldownMs > 0 && isSkillOnCooldown(player, skillId)) {
            const remaining = getSkillCooldownRemaining(player, skillId);
            socket.emit('skill:error', { message: `Skill on cooldown (${Math.ceil(remaining / 1000)}s)` });
            return;
        }

        // Check SP (passive skills cost 0) — check only, don't consume yet (consumed at execution)
        if (spCost > 0 && player.mana < spCost) {
            socket.emit('skill:error', { message: `Not enough SP (need ${spCost}, have ${player.mana})` });
            return;
        }

        // Resolve target for single-target skills
        // Priority: explicit targetId from payload > autoAttackState target
        let targetId = data.targetId ? parseInt(data.targetId) : 0;
        let isEnemy = data.isEnemy !== undefined ? !!data.isEnemy : false;
        if (!targetId) {
            const atkState = autoAttackState.get(characterId);
            if (atkState) {
                targetId = atkState.targetCharId;
                isEnemy = atkState.isEnemy;
            }
        }

        // PvP disabled — block skills targeting other players
        if (!PVP_ENABLED && !isEnemy && targetId) {
            const ptarget = connectedPlayers.get(targetId);
            if (ptarget) {
                socket.emit('skill:error', { message: 'PvP is currently disabled' });
                return;
            }
        }

        // Parse ground coordinates for ground-targeted skills (Thunderstorm, Fire Wall, Safety Wall)
        const groundX = data.groundX !== undefined ? parseFloat(data.groundX) : undefined;
        const groundY = data.groundY !== undefined ? parseFloat(data.groundY) : undefined;
        const groundZ = data.groundZ !== undefined ? parseFloat(data.groundZ) : undefined;
        const hasGroundPos = groundX !== undefined && groundY !== undefined && !isNaN(groundX) && !isNaN(groundY);

        // ================================================================
        // CAST TIME CHECK — RO pre-renewal cast system
        // If baseCastTime > 0, calculate actual cast time with DEX reduction.
        // If actualCastTime > 0, enter casting state and return (skill executes later).
        // If actualCastTime = 0 (instant cast or DEX >= 150), fall through to immediate execution.
        // Skip this check when _castComplete flag is set (cast already finished).
        // ================================================================
        const baseCastTime = levelData ? (levelData.castTime || 0) : 0;
        if (baseCastTime > 0 && !data._castComplete) {
            const effectiveStats = getEffectiveStats(player);
            const actualCastTime = calculateActualCastTime(baseCastTime, effectiveStats.dex);

            if (actualCastTime > 0) {
                const now = Date.now();
                activeCasts.set(characterId, {
                    skillId, targetId, isEnemy, learnedLevel, levelData, skill,
                    castStartTime: now, castEndTime: now + actualCastTime, actualCastTime,
                    socketId: socket.id, casterName: player.characterName,
                    spCost, cooldownMs, effectVal, duration,
                    groundX, groundY, groundZ, hasGroundPos
                });
                // Broadcast cast start to all clients (for cast bar rendering)
                io.emit('skill:cast_start', {
                    casterId: characterId, casterName: player.characterName,
                    skillId, skillName: skill.displayName,
                    actualCastTime, targetId, isEnemy
                });
                logger.info(`[CAST] ${player.characterName} begins casting ${skill.displayName} Lv${learnedLevel} (${actualCastTime}ms, base ${baseCastTime}ms, DEX ${effectiveStats.dex})`);
                return; // Don't execute yet — combat tick will complete the cast
            }
            // actualCastTime = 0 → instant cast, fall through to execute immediately
        }

        // ================================================================
        // SKILL EFFECT HANDLERS — One block per swordsman skill
        // ================================================================

        // --- FIRST AID (ID 2) — Self heal ---
        if (skill.name === 'first_aid') {
            player.mana = Math.max(0, player.mana - spCost);
            const healed = Math.min(effectVal, player.maxHealth - player.health);
            player.health = Math.min(player.maxHealth, player.health + effectVal);
            applySkillDelays(characterId, player, skillId, levelData, socket);
            logger.info(`[SKILLS] ${player.characterName} healed ${healed} HP (${player.health}/${player.maxHealth})`);

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- BASH (ID 103) — Single target physical damage ---
        if (skill.name === 'bash') {
            if (!targetId) {
                socket.emit('skill:error', { message: 'No target selected' });
                return;
            }

            // Resolve target and check range
            let target, targetPos, targetStats, targetHardDef = 0, targetName = '';
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = { x: target.x, y: target.y, z: target.z };
                targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
                targetHardDef = target.hardDef || 0;
                targetName = target.name;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = await getPlayerPosition(targetId);
                if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                targetStats = getEffectiveStats(target);
                targetHardDef = target.hardDef || 0;
                targetName = target.characterName;
            }

            // Range check
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dx = attackerPos.x - targetPos.x;
            const dy = attackerPos.y - targetPos.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            const skillRange = skill.range || player.attackRange || COMBAT.MELEE_RANGE;
            if (dist > skillRange + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z, distance: dist, requiredRange: skillRange });
                return;
            }

            // Deduct SP and set cooldown
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Calculate skill damage using full RO system: effectVal% of normal damage
            const atkBuffMods = getBuffStatModifiers(player);
            const defBuffMods = isEnemy ? { defMultiplier: 1.0, atkMultiplier: 1.0 } : getBuffStatModifiers(target);
            if (target.activeBuffs) {
                const provokeBuff = target.activeBuffs.find(b => b.name === 'provoke' && Date.now() < b.expiresAt);
                if (provokeBuff) defBuffMods.defMultiplier *= (1 - (provokeBuff.defReduction || 0) / 100);
            }
            const skillTargetInfo = isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId);
            skillTargetInfo.buffMods = defBuffMods;
            const skillAtkInfo = getAttackerInfo(player);
            skillAtkInfo.buffMods = atkBuffMods;
            const bashResult = calculateSkillDamage(
                getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
                targetHardDef, effectVal, atkBuffMods, defBuffMods,
                skillTargetInfo, skillAtkInfo, { skillElement: skill.element || 'neutral' }
            );
            const { damage, isCritical, isMiss, hitType: bashHitType } = bashResult;

            if (isMiss) {
                // Skill missed — still consume SP/cooldown, broadcast miss
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName, isEnemy, damage: 0, isCritical: false,
                    isMiss: true, hitType: bashHitType, element: skill.element || 'neutral',
                    targetHealth: target.health, targetMaxHealth: target.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                    timestamp: Date.now()
                });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                // cooldown_started emitted by applySkillDelays
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }

            // Apply damage
            target.health = Math.max(0, target.health - damage);
            // Cast interruption: skill damage interrupts casting on player targets
            if (!isEnemy && activeCasts.has(targetId)) interruptCast(targetId, 'damage');
            logger.info(`[SKILL-COMBAT] ${player.characterName} BASH Lv${learnedLevel} → ${targetName} for ${damage}${isCritical ? ' CRIT' : ''} [${bashHitType}] (HP: ${target.health}/${target.maxHealth})`);

            // Broadcast skill damage to all
            io.emit('skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                damage, isCritical, isMiss: false, hitType: bashHitType,
                targetHealth: target.health, targetMaxHealth: target.maxHealth,
                attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: Date.now()
            });

            // Also emit standard combat:damage so existing BP + C++ damage numbers work
            io.emit('combat:damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy, damage, isCritical,
                isMiss: false, hitType: bashHitType, element: skill.element || 'neutral',
                targetHealth: target.health, targetMaxHealth: target.maxHealth,
                attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: Date.now()
            });

            // Enemy health update
            if (isEnemy) {
                io.emit('enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
            }

            // Check death
            if (target.health <= 0) {
                if (isEnemy) {
                    await processEnemyDeathFromSkill(target, player, characterId, io);
                } else {
                    target.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    io.emit('combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                    io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Bash!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(targetId, 0, target.mana);
                }
            }

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // cooldown_started emitted by applySkillDelays
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- PROVOKE (ID 104) — Single target debuff: -DEF%, +ATK% ---
        if (skill.name === 'provoke') {
            if (!targetId) {
                socket.emit('skill:error', { message: 'No target selected' });
                return;
            }

            let target, targetName = '';
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetName = target.name;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetName = target.characterName;
            }

            // Range check
            const attackerPos = await getPlayerPosition(characterId);
            const targetPos = isEnemy ? { x: target.x, y: target.y, z: target.z } : await getPlayerPosition(targetId);
            if (!attackerPos || !targetPos) return;
            const dx = attackerPos.x - targetPos.x;
            const dy = attackerPos.y - targetPos.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            // Deduct SP and set cooldown
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Apply provoke debuff: -effectVal% DEF, +effectVal% ATK, for duration ms
            const buffDef = {
                skillId, name: 'provoke', casterId: characterId, casterName: player.characterName,
                defReduction: effectVal, atkIncrease: effectVal, duration: duration || 30000,
                mdefBonus: 0
            };
            applyBuff(target, buffDef);

            logger.info(`[SKILL-COMBAT] ${player.characterName} PROVOKE Lv${learnedLevel} → ${targetName} (-${effectVal}% DEF, +${effectVal}% ATK for ${(duration || 30000) / 1000}s)`);

            // Broadcast buff applied
            io.emit('skill:buff_applied', {
                targetId, targetName, isEnemy,
                casterId: characterId, casterName: player.characterName,
                skillId, buffName: 'Provoke', duration: duration || 30000,
                effects: { defReduction: effectVal, atkIncrease: effectVal }
            });

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // cooldown_started emitted by applySkillDelays
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- MAGNUM BREAK (ID 105) — AoE fire damage around caster ---
        if (skill.name === 'magnum_break') {
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;

            // Deduct SP and set cooldown
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const AOE_RADIUS = 300; // Fire radius in UE units
            const atkBuffMods = getBuffStatModifiers(player);
            let totalDamageDealt = 0;
            let enemiesHit = 0;

            // Hit all enemies within AoE radius
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                const dx = attackerPos.x - enemy.x;
                const dy = attackerPos.y - enemy.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > AOE_RADIUS) continue;

                const defBuffMods = { defMultiplier: 1.0, atkMultiplier: 1.0 };
                if (enemy.activeBuffs) {
                    const pb = enemy.activeBuffs.find(b => b.name === 'provoke' && Date.now() < b.expiresAt);
                    if (pb) defBuffMods.defMultiplier *= (1 - (pb.defReduction || 0) / 100);
                }
                const eStats = enemy.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
                const mbResult = calculateSkillDamage(
                    getEffectiveStats(player), eStats, enemy.hardDef || 0, effectVal, atkBuffMods, defBuffMods,
                    getEnemyTargetInfo(enemy), getAttackerInfo(player), { skillElement: 'fire' }
                );
                const { damage, isCritical, isMiss, hitType: mbHitType } = mbResult;

                if (isMiss) {
                    // AoE skill missed this target — broadcast miss
                    io.emit('combat:damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId: eid, targetName: enemy.name, isEnemy: true, damage: 0, isCritical: false,
                        isMiss: true, hitType: mbHitType, element: 'fire',
                        targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                        attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                        targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                        timestamp: Date.now()
                    });
                    continue;
                }

                enemy.health = Math.max(0, enemy.health - damage);
                totalDamageDealt += damage;
                enemiesHit++;

                // Broadcast damage for each enemy hit
                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                    damage, isCritical, isMiss: false, hitType: mbHitType,
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    timestamp: Date.now()
                });
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true, damage, isCritical,
                    isMiss: false, hitType: mbHitType, element: 'fire',
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    timestamp: Date.now()
                });
                io.emit('enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true });

                // Check death
                if (enemy.health <= 0) {
                    await processEnemyDeathFromSkill(enemy, player, characterId, io);
                }
            }

            // Also hit players within AoE radius (PvP) — skip self
            if (PVP_ENABLED)
            for (const [pid, ptarget] of connectedPlayers.entries()) {
                if (pid === characterId || ptarget.isDead) continue;
                const pPos = await getPlayerPosition(pid);
                if (!pPos) continue;
                const dx = attackerPos.x - pPos.x;
                const dy = attackerPos.y - pPos.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > AOE_RADIUS) continue;

                const defBuffMods = getBuffStatModifiers(ptarget);
                const mbPvpResult = calculateSkillDamage(
                    getEffectiveStats(player), getEffectiveStats(ptarget), ptarget.hardDef || 0, effectVal, atkBuffMods, defBuffMods,
                    getPlayerTargetInfo(ptarget, pid), getAttackerInfo(player), { skillElement: 'fire' }
                );
                const { damage: pvpMbDmg, isCritical: pvpMbCrit, isMiss: pvpMbMiss, hitType: pvpMbHitType } = mbPvpResult;

                if (pvpMbMiss) {
                    io.emit('combat:damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId: pid, targetName: ptarget.characterName, isEnemy: false, damage: 0, isCritical: false,
                        isMiss: true, hitType: pvpMbHitType, element: 'fire',
                        targetHealth: ptarget.health, targetMaxHealth: ptarget.maxHealth,
                        attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                        targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                        timestamp: Date.now()
                    });
                    continue;
                }

                ptarget.health = Math.max(0, ptarget.health - pvpMbDmg);
                if (activeCasts.has(ptId)) interruptCast(ptId, 'damage');
                totalDamageDealt += pvpMbDmg;

                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: pid, targetName: ptarget.characterName, isEnemy: false,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                    damage: pvpMbDmg, isCritical: pvpMbCrit, isMiss: false, hitType: pvpMbHitType,
                    targetHealth: ptarget.health, targetMaxHealth: ptarget.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                    timestamp: Date.now()
                });
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: pid, targetName: ptarget.characterName, isEnemy: false, damage: pvpMbDmg, isCritical: pvpMbCrit,
                    isMiss: false, hitType: pvpMbHitType, element: 'fire',
                    targetHealth: ptarget.health, targetMaxHealth: ptarget.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                    timestamp: Date.now()
                });

                if (ptarget.health <= 0) {
                    ptarget.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === pid && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    io.emit('combat:death', { killedId: pid, killedName: ptarget.characterName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now() });
                    io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${ptarget.characterName} with Magnum Break!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(pid, 0, ptarget.mana);
                }
            }

            logger.info(`[SKILL-COMBAT] ${player.characterName} MAGNUM BREAK Lv${learnedLevel}: hit ${enemiesHit} enemies, ${totalDamageDealt} total dmg`);

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, enemiesHit, totalDamage: totalDamageDealt });
            // cooldown_started emitted by applySkillDelays
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- ENDURE (ID 106) — Self buff: grants MDEF for duration ---
        if (skill.name === 'endure') {
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const buffDuration = duration || (10000 + learnedLevel * 3000);
            const mdefBonus = effectVal; // effectValue = level (1-10)
            const buffDef = {
                skillId, name: 'endure', casterId: characterId, casterName: player.characterName,
                mdefBonus, defReduction: 0, atkIncrease: 0, duration: buffDuration
            };
            applyBuff(player, buffDef);

            logger.info(`[SKILL-COMBAT] ${player.characterName} ENDURE Lv${learnedLevel}: +${mdefBonus} MDEF for ${buffDuration / 1000}s`);

            socket.emit('skill:buff_applied', {
                targetId: characterId, targetName: player.characterName, isEnemy: false,
                casterId: characterId, casterName: player.characterName,
                skillId, buffName: 'Endure', duration: buffDuration,
                effects: { mdefBonus }
            });

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // cooldown_started emitted by applySkillDelays
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // ================================================================
        // MAGE SKILL EFFECT HANDLERS
        // ================================================================

        // --- BOLT SKILLS: Cold Bolt (200), Fire Bolt (201), Lightning Bolt (202) ---
        // Single target multi-hit magic. Hits = skill level. 100% MATK per hit.
        if (skill.name === 'cold_bolt' || skill.name === 'fire_bolt' || skill.name === 'lightning_bolt') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let target, targetPos, targetStats, targetHardMdef = 0, targetName = '';
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = { x: target.x, y: target.y, z: target.z };
                targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
                targetHardMdef = target.hardMdef || target.magicDefense || 0;
                targetName = target.name;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = await getPlayerPosition(targetId);
                if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                targetStats = getEffectiveStats(target);
                targetHardMdef = target.hardMdef || 0;
                targetName = target.characterName;
            }

            // Range check
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dx = attackerPos.x - targetPos.x;
            const dy = attackerPos.y - targetPos.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            // Check frozen/stoned status — can't cast while CC'd
            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) {
                socket.emit('skill:error', { message: 'Cannot cast while incapacitated' });
                return;
            }

            // Deduct SP and set cooldown (after-cast delay)
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Multi-hit magic damage: hits = skill level, 100% MATK per hit
            const numHits = learnedLevel;
            const targetBuffMods = isEnemy ? getBuffStatModifiers(target) : getBuffStatModifiers(target);
            const magicTargetInfo = isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId);
            // Override element if target is frozen/stoned
            if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
            magicTargetInfo.buffMods = targetBuffMods;

            // Calculate per-hit damages
            const hitDamages = [];
            let totalDamage = 0;
            for (let h = 0; h < numHits; h++) {
                const hitResult = calculateMagicSkillDamage(
                    getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
                    targetHardMdef, 100, skill.element, magicTargetInfo
                );
                hitDamages.push(hitResult.damage);
                totalDamage += hitResult.damage;
            }

            // Check element immunity (if first hit was 0, all are 0)
            if (totalDamage <= 0) {
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName, isEnemy, damage: 0, isCritical: false,
                    isMiss: true, hitType: 'magical', element: skill.element,
                    targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                    targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
                });
                socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
                // ACD + cooldown handled by applySkillDelays above
                socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
                return;
            }

            // Apply total damage all at once (gameplay)
            target.health = Math.max(0, target.health - totalDamage);
            if (!isEnemy && activeCasts.has(targetId)) interruptCast(targetId, 'damage');
            logger.info(`[SKILL-COMBAT] ${player.characterName} ${skill.displayName} Lv${learnedLevel} → ${targetName} for ${totalDamage} (${numHits} hits) [${skill.element}] (HP: ${target.health}/${target.maxHealth})`);

            // Fire damage breaks Frozen status
            if (skill.element === 'fire' && targetBuffMods.isFrozen && target.activeBuffs) {
                target.activeBuffs = target.activeBuffs.filter(b => b.name !== 'frozen');
                io.emit('skill:buff_removed', { targetId, isEnemy, buffName: 'frozen', reason: 'fire_damage' });
            }

            // Summary event for skill effect tracking
            io.emit('skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                damage: totalDamage, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });

            // Per-hit damage numbers (RO-style staggered display, ~200ms between hits)
            const HIT_DELAY_MS = 200;
            for (let h = 0; h < numHits; h++) {
                const hitDmg = hitDamages[h];
                const delay = h * HIT_DELAY_MS;
                setTimeout(() => {
                    io.emit('combat:damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId, targetName, isEnemy, damage: hitDmg, isCritical: false,
                        isMiss: false, hitType: 'magical', element: skill.element,
                        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                        hitNumber: h + 1, totalHits: numHits,
                        targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
                    });
                }, delay);
            }

            if (isEnemy) {
                io.emit('enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                io.emit('combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with ${skill.displayName}!`, timestamp: Date.now() });
                await savePlayerHealthToDB(targetId, 0, target.mana);
            }

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, hits: numHits, totalDamage });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- SOUL STRIKE (ID 210) — Ghost multi-hit, bonus vs Undead ---
        if (skill.name === 'soul_strike') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let target, targetPos, targetStats, targetHardMdef = 0, targetName = '';
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = { x: target.x, y: target.y, z: target.z };
                targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
                targetHardMdef = target.hardMdef || target.magicDefense || 0;
                targetName = target.name;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = await getPlayerPosition(targetId);
                if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                targetStats = getEffectiveStats(target);
                targetHardMdef = target.hardMdef || 0;
                targetName = target.characterName;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dx = attackerPos.x - targetPos.x;
            const dy = attackerPos.y - targetPos.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Hits = floor((level+1)/2): 1,1,2,2,3,3,4,4,5,5
            const numHits = Math.floor((learnedLevel + 1) / 2);
            const targetBuffMods = getBuffStatModifiers(target);
            const magicTargetInfo = isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId);
            if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
            magicTargetInfo.buffMods = targetBuffMods;

            // Check if target is Undead element for bonus damage
            const targetEle = (magicTargetInfo.element && magicTargetInfo.element.type) || 'neutral';
            const undeadBonus = (targetEle === 'undead') ? (1 + learnedLevel * 0.05) : 1.0;

            // Calculate per-hit damages
            const hitDamages = [];
            let totalDamage = 0;
            for (let h = 0; h < numHits; h++) {
                const hitResult = calculateMagicSkillDamage(
                    getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
                    targetHardMdef, 100, 'ghost', magicTargetInfo
                );
                const hitDmg = Math.floor(hitResult.damage * undeadBonus);
                hitDamages.push(hitDmg);
                totalDamage += hitDmg;
            }

            target.health = Math.max(0, target.health - totalDamage);
            if (!isEnemy && activeCasts.has(targetId)) interruptCast(targetId, 'damage');
            logger.info(`[SKILL-COMBAT] ${player.characterName} Soul Strike Lv${learnedLevel} → ${targetName} for ${totalDamage} (${numHits} hits, ghost${undeadBonus > 1 ? '+undead' : ''}) (HP: ${target.health}/${target.maxHealth})`);

            io.emit('skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: 'Soul Strike', skillLevel: learnedLevel, element: 'ghost',
                damage: totalDamage, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });

            // Per-hit damage numbers (RO-style staggered display, ~200ms between hits)
            const SS_HIT_DELAY = 200;
            for (let h = 0; h < numHits; h++) {
                const hitDmg = hitDamages[h];
                const delay = h * SS_HIT_DELAY;
                setTimeout(() => {
                    io.emit('combat:damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId, targetName, isEnemy, damage: hitDmg, isCritical: false,
                        isMiss: false, hitType: 'magical', element: 'ghost',
                        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                        hitNumber: h + 1, totalHits: numHits,
                        targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
                    });
                }, delay);
            }

            if (isEnemy) {
                io.emit('enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                io.emit('combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Soul Strike!`, timestamp: Date.now() });
                await savePlayerHealthToDB(targetId, 0, target.mana);
            }

            socket.emit('skill:used', { skillId, skillName: 'Soul Strike', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, hits: numHits, totalDamage });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- NAPALM BEAT (ID 203) — Ghost AoE, damage SPLIT among targets ---
        if (skill.name === 'napalm_beat') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let primaryTarget, primaryPos, primaryName = '';
            if (isEnemy) {
                primaryTarget = enemies.get(targetId);
                if (!primaryTarget || primaryTarget.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                primaryPos = { x: primaryTarget.x, y: primaryTarget.y, z: primaryTarget.z };
                primaryName = primaryTarget.name;
            } else {
                primaryTarget = connectedPlayers.get(targetId);
                if (!primaryTarget || primaryTarget.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                primaryPos = await getPlayerPosition(targetId);
                if (!primaryPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                primaryName = primaryTarget.characterName;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dx = attackerPos.x - primaryPos.x;
            const dy = attackerPos.y - primaryPos.y;
            const dist = Math.sqrt(dx * dx + dy * dy);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Gather all targets in 3x3 AoE (300 UE units) around primary target
            const NAPALM_AOE = 300;
            const splashTargets = []; // { id, target, isEnemy, pos, stats, hardMdef, name }

            // Primary target always included
            splashTargets.push({ id: targetId, target: primaryTarget, isEnemy, pos: primaryPos,
                stats: isEnemy ? (primaryTarget.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 }) : getEffectiveStats(primaryTarget),
                hardMdef: isEnemy ? (primaryTarget.hardMdef || primaryTarget.magicDefense || 0) : (primaryTarget.hardMdef || 0),
                name: primaryName });

            // Find additional enemies in splash
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead || eid === targetId) continue;
                const edx = primaryPos.x - enemy.x;
                const edy = primaryPos.y - enemy.y;
                if (Math.sqrt(edx * edx + edy * edy) <= NAPALM_AOE) {
                    splashTargets.push({ id: eid, target: enemy, isEnemy: true,
                        pos: { x: enemy.x, y: enemy.y, z: enemy.z },
                        stats: enemy.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                        hardMdef: enemy.hardMdef || enemy.magicDefense || 0, name: enemy.name });
                }
            }
            // Find additional players in splash (PvP)
            if (PVP_ENABLED)
            for (const [pid, ptarget] of connectedPlayers.entries()) {
                if (pid === characterId || ptarget.isDead || (pid === targetId && !isEnemy)) continue;
                const pPos = await getPlayerPosition(pid);
                if (!pPos) continue;
                const pdx = primaryPos.x - pPos.x;
                const pdy = primaryPos.y - pPos.y;
                if (Math.sqrt(pdx * pdx + pdy * pdy) <= NAPALM_AOE) {
                    splashTargets.push({ id: pid, target: ptarget, isEnemy: false, pos: pPos,
                        stats: getEffectiveStats(ptarget), hardMdef: ptarget.hardMdef || 0,
                        name: ptarget.characterName });
                }
            }

            // Calculate base MATK damage at effectValue% then split among targets
            const baseMagicResult = calculateMagicSkillDamage(
                getEffectiveStats(player), splashTargets[0].stats,
                splashTargets[0].hardMdef, effectVal, 'ghost',
                { element: splashTargets[0].isEnemy ? (splashTargets[0].target.element || { type: 'neutral', level: 1 }) : { type: 'neutral', level: 1 } }
            );
            const totalBaseDamage = baseMagicResult.damage;
            const splitDamage = Math.max(1, Math.floor(totalBaseDamage / splashTargets.length));

            let totalDamageDealt = 0;
            for (const st of splashTargets) {
                // Recalculate per-target for proper element/MDEF, then scale by split ratio
                const tBuffMods = getBuffStatModifiers(st.target);
                const tInfo = st.isEnemy ? getEnemyTargetInfo(st.target) : getPlayerTargetInfo(st.target, st.id);
                if (tBuffMods.overrideElement) tInfo.element = tBuffMods.overrideElement;
                tInfo.buffMods = tBuffMods;
                const perTargetResult = calculateMagicSkillDamage(
                    getEffectiveStats(player), st.stats, st.hardMdef, effectVal, 'ghost', tInfo
                );
                const dmg = Math.max(1, Math.floor(perTargetResult.damage / splashTargets.length));
                st.target.health = Math.max(0, st.target.health - dmg);
                if (!st.isEnemy && activeCasts.has(st.id)) interruptCast(st.id, 'damage');
                totalDamageDealt += dmg;

                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: st.id, targetName: st.name, isEnemy: st.isEnemy,
                    skillId, skillName: 'Napalm Beat', skillLevel: learnedLevel, element: 'ghost',
                    damage: dmg, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: st.pos.x, targetY: st.pos.y, targetZ: st.pos.z,
                    targetHealth: st.target.health, targetMaxHealth: st.target.maxHealth, timestamp: Date.now()
                });
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: st.id, targetName: st.name, isEnemy: st.isEnemy, damage: dmg, isCritical: false,
                    isMiss: false, hitType: 'magical', element: 'ghost',
                    targetX: st.pos.x, targetY: st.pos.y, targetZ: st.pos.z,
                    targetHealth: st.target.health, targetMaxHealth: st.target.maxHealth, timestamp: Date.now()
                });

                if (st.isEnemy) {
                    io.emit('enemy:health_update', { enemyId: st.id, health: st.target.health, maxHealth: st.target.maxHealth, inCombat: true });
                    if (st.target.health <= 0) await processEnemyDeathFromSkill(st.target, player, characterId, io);
                } else if (st.target.health <= 0) {
                    st.target.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === st.id && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    io.emit('combat:death', { killedId: st.id, killedName: st.name, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: st.target.maxHealth, timestamp: Date.now() });
                    io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${st.name} with Napalm Beat!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(st.id, 0, st.target.mana);
                }
            }

            logger.info(`[SKILL-COMBAT] ${player.characterName} Napalm Beat Lv${learnedLevel}: hit ${splashTargets.length} targets, ${totalDamageDealt} total dmg (split)`);
            socket.emit('skill:used', { skillId, skillName: 'Napalm Beat', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, targetsHit: splashTargets.length, totalDamage: totalDamageDealt });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- FIRE BALL (ID 207) — Fire AoE splash around target (full damage, NOT split) ---
        if (skill.name === 'fire_ball') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let primaryTarget, primaryPos, primaryName = '';
            if (isEnemy) {
                primaryTarget = enemies.get(targetId);
                if (!primaryTarget || primaryTarget.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                primaryPos = { x: primaryTarget.x, y: primaryTarget.y, z: primaryTarget.z };
                primaryName = primaryTarget.name;
            } else {
                primaryTarget = connectedPlayers.get(targetId);
                if (!primaryTarget || primaryTarget.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                primaryPos = await getPlayerPosition(targetId);
                if (!primaryPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                primaryName = primaryTarget.characterName;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dist = Math.sqrt((attackerPos.x - primaryPos.x) ** 2 + (attackerPos.y - primaryPos.y) ** 2);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // 5x5 AoE = 500 UE units around target
            // Center 3x3 = full damage, outer ring = 75% damage (RO pre-renewal)
            const FIREBALL_AOE = 500;
            const FIREBALL_INNER = 300;  // 3x3 center radius
            let totalDamageDealt = 0;
            let targetsHit = 0;

            // Helper to deal Fire Ball damage to a single target
            // distFromCenter: distance from AoE center — outer ring (>FIREBALL_INNER) takes 75% damage
            const dealFireBallDamage = async (tId, tgt, tIsEnemy, tPos, tStats, tHardMdef, tName, distFromCenter) => {
                const tBuffMods = getBuffStatModifiers(tgt);
                const tInfo = tIsEnemy ? getEnemyTargetInfo(tgt) : getPlayerTargetInfo(tgt, tId);
                if (tBuffMods.overrideElement) tInfo.element = tBuffMods.overrideElement;
                tInfo.buffMods = tBuffMods;
                const fbResult = calculateMagicSkillDamage(
                    getEffectiveStats(player), tStats, tHardMdef, effectVal, 'fire', tInfo
                );
                // Outer ring (beyond 3x3 center) takes 75% damage
                const edgeMult = (distFromCenter > FIREBALL_INNER) ? 0.75 : 1.0;
                const dmg = Math.floor(fbResult.damage * edgeMult);
                tgt.health = Math.max(0, tgt.health - dmg);
                if (!tIsEnemy && activeCasts.has(tId)) interruptCast(tId, 'damage');
                totalDamageDealt += dmg;
                targetsHit++;

                // Fire damage breaks Frozen
                if (tBuffMods.isFrozen && tgt.activeBuffs) {
                    tgt.activeBuffs = tgt.activeBuffs.filter(b => b.name !== 'frozen');
                    io.emit('skill:buff_removed', { targetId: tId, isEnemy: tIsEnemy, buffName: 'frozen', reason: 'fire_damage' });
                }

                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: tId, targetName: tName, isEnemy: tIsEnemy,
                    skillId, skillName: 'Fire Ball', skillLevel: learnedLevel, element: 'fire',
                    damage: dmg, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: tPos.x, targetY: tPos.y, targetZ: tPos.z,
                    targetHealth: tgt.health, targetMaxHealth: tgt.maxHealth, timestamp: Date.now()
                });
                io.emit('combat:damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: tId, targetName: tName, isEnemy: tIsEnemy, damage: dmg, isCritical: false,
                    isMiss: false, hitType: 'magical', element: 'fire',
                    targetX: tPos.x, targetY: tPos.y, targetZ: tPos.z,
                    targetHealth: tgt.health, targetMaxHealth: tgt.maxHealth, timestamp: Date.now()
                });

                if (tIsEnemy) {
                    io.emit('enemy:health_update', { enemyId: tId, health: tgt.health, maxHealth: tgt.maxHealth, inCombat: true });
                    if (tgt.health <= 0) await processEnemyDeathFromSkill(tgt, player, characterId, io);
                } else if (tgt.health <= 0) {
                    tgt.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === tId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    io.emit('combat:death', { killedId: tId, killedName: tName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: tgt.maxHealth, timestamp: Date.now() });
                    io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${tName} with Fire Ball!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(tId, 0, tgt.mana);
                }
            };

            // Hit all enemies in AoE
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                const eDist = Math.sqrt((primaryPos.x - enemy.x) ** 2 + (primaryPos.y - enemy.y) ** 2);
                if (eDist <= FIREBALL_AOE) {
                    await dealFireBallDamage(eid, enemy, true, { x: enemy.x, y: enemy.y, z: enemy.z },
                        enemy.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                        enemy.hardMdef || enemy.magicDefense || 0, enemy.name, eDist);
                }
            }
            // Hit players in AoE (PvP)
            if (PVP_ENABLED)
            for (const [pid, ptarget] of connectedPlayers.entries()) {
                if (pid === characterId || ptarget.isDead) continue;
                const pPos = await getPlayerPosition(pid);
                if (!pPos) continue;
                const pDist = Math.sqrt((primaryPos.x - pPos.x) ** 2 + (primaryPos.y - pPos.y) ** 2);
                if (pDist <= FIREBALL_AOE) {
                    await dealFireBallDamage(pid, ptarget, false, pPos,
                        getEffectiveStats(ptarget), ptarget.hardMdef || 0, ptarget.characterName, pDist);
                }
            }

            logger.info(`[SKILL-COMBAT] ${player.characterName} Fire Ball Lv${learnedLevel}: hit ${targetsHit} targets, ${totalDamageDealt} total dmg`);
            socket.emit('skill:used', { skillId, skillName: 'Fire Ball', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, targetsHit, totalDamage: totalDamageDealt });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- THUNDERSTORM (ID 212) — Wind AoE, multi-hit = level ---
        if (skill.name === 'thunderstorm') {
            // Ground-targeted AoE: use ground coordinates from client, fallback to target/caster
            let centerPos;
            if (hasGroundPos) {
                centerPos = { x: groundX, y: groundY, z: groundZ || 0 };
            } else if (targetId && isEnemy) {
                const enemy = enemies.get(targetId);
                if (!enemy || enemy.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                centerPos = { x: enemy.x, y: enemy.y, z: enemy.z };
            } else if (targetId && !isEnemy) {
                if (!PVP_ENABLED) { socket.emit('skill:error', { message: 'PvP is currently disabled' }); return; }
                const ptarget = connectedPlayers.get(targetId);
                if (!ptarget || ptarget.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                centerPos = await getPlayerPosition(targetId);
                if (!centerPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
            } else {
                // No coordinates or target — center on caster
                centerPos = await getPlayerPosition(characterId);
                if (!centerPos) return;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dist = Math.sqrt((attackerPos.x - centerPos.x) ** 2 + (attackerPos.y - centerPos.y) ** 2);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const STORM_AOE = 500; // 5x5 area
            const numHits = learnedLevel;
            let totalDamageDealt = 0;
            let targetsHit = 0;

            // Hit enemies in AoE
            const TS_HIT_DELAY = 200;
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                const eDist = Math.sqrt((centerPos.x - enemy.x) ** 2 + (centerPos.y - enemy.y) ** 2);
                if (eDist > STORM_AOE) continue;

                const tBuffMods = getBuffStatModifiers(enemy);
                const tInfo = getEnemyTargetInfo(enemy);
                if (tBuffMods.overrideElement) tInfo.element = tBuffMods.overrideElement;
                tInfo.buffMods = tBuffMods;

                const tsHitDamages = [];
                let dmg = 0;
                for (let h = 0; h < numHits; h++) {
                    const hitResult = calculateMagicSkillDamage(
                        getEffectiveStats(player), enemy.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 },
                        enemy.hardMdef || enemy.magicDefense || 0, 100, 'wind', tInfo
                    );
                    tsHitDamages.push(hitResult.damage);
                    dmg += hitResult.damage;
                }

                enemy.health = Math.max(0, enemy.health - dmg);
                totalDamageDealt += dmg;
                targetsHit++;

                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true,
                    skillId, skillName: 'Thunderstorm', skillLevel: learnedLevel, element: 'wind',
                    damage: dmg, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth, timestamp: Date.now()
                });
                // Per-hit damage numbers (staggered)
                const ePos = { x: enemy.x, y: enemy.y, z: enemy.z };
                const eName = enemy.name;
                for (let h = 0; h < numHits; h++) {
                    setTimeout(() => {
                        io.emit('combat:damage', {
                            attackerId: characterId, attackerName: player.characterName,
                            targetId: eid, targetName: eName, isEnemy: true, damage: tsHitDamages[h], isCritical: false,
                            isMiss: false, hitType: 'magical', element: 'wind',
                            targetX: ePos.x, targetY: ePos.y, targetZ: ePos.z,
                            hitNumber: h + 1, totalHits: numHits,
                            targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth, timestamp: Date.now()
                        });
                    }, h * TS_HIT_DELAY);
                }
                io.emit('enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true });
                if (enemy.health <= 0) await processEnemyDeathFromSkill(enemy, player, characterId, io);
            }

            // Hit players in AoE (PvP)
            if (PVP_ENABLED)
            for (const [pid, ptarget] of connectedPlayers.entries()) {
                if (pid === characterId || ptarget.isDead) continue;
                const pPos = await getPlayerPosition(pid);
                if (!pPos) continue;
                const pDist = Math.sqrt((centerPos.x - pPos.x) ** 2 + (centerPos.y - pPos.y) ** 2);
                if (pDist > STORM_AOE) continue;

                const tBuffMods = getBuffStatModifiers(ptarget);
                const tInfo = getPlayerTargetInfo(ptarget, pid);
                if (tBuffMods.overrideElement) tInfo.element = tBuffMods.overrideElement;
                tInfo.buffMods = tBuffMods;

                const pvpHitDmgs = [];
                let dmg = 0;
                for (let h = 0; h < numHits; h++) {
                    const hitResult = calculateMagicSkillDamage(
                        getEffectiveStats(player), getEffectiveStats(ptarget),
                        ptarget.hardMdef || 0, 100, 'wind', tInfo
                    );
                    pvpHitDmgs.push(hitResult.damage);
                    dmg += hitResult.damage;
                }

                ptarget.health = Math.max(0, ptarget.health - dmg);
                if (activeCasts.has(pid)) interruptCast(pid, 'damage');
                totalDamageDealt += dmg;
                targetsHit++;

                io.emit('skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: pid, targetName: ptarget.characterName, isEnemy: false,
                    skillId, skillName: 'Thunderstorm', skillLevel: learnedLevel, element: 'wind',
                    damage: dmg, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                    targetHealth: ptarget.health, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now()
                });
                // Per-hit damage numbers (staggered)
                const pvpName = ptarget.characterName;
                for (let h = 0; h < numHits; h++) {
                    setTimeout(() => {
                        io.emit('combat:damage', {
                            attackerId: characterId, attackerName: player.characterName,
                            targetId: pid, targetName: pvpName, isEnemy: false, damage: pvpHitDmgs[h], isCritical: false,
                            isMiss: false, hitType: 'magical', element: 'wind',
                            targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                            hitNumber: h + 1, totalHits: numHits,
                            targetHealth: ptarget.health, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now()
                        });
                    }, h * TS_HIT_DELAY);
                }

                if (ptarget.health <= 0) {
                    ptarget.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === pid && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    io.emit('combat:death', { killedId: pid, killedName: ptarget.characterName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now() });
                    io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${ptarget.characterName} with Thunderstorm!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(pid, 0, ptarget.mana);
                }
            }

            logger.info(`[SKILL-COMBAT] ${player.characterName} Thunderstorm Lv${learnedLevel}: hit ${targetsHit} targets, ${totalDamageDealt} total dmg (${numHits} hits each)`);
            socket.emit('skill:used', { skillId, skillName: 'Thunderstorm', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, targetsHit, totalDamage: totalDamageDealt });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- FROST DIVER (ID 208) — Water damage + Freeze chance ---
        if (skill.name === 'frost_diver') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let target, targetPos, targetStats, targetHardMdef = 0, targetName = '';
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = { x: target.x, y: target.y, z: target.z };
                targetStats = target.stats || { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1 };
                targetHardMdef = target.hardMdef || target.magicDefense || 0;
                targetName = target.name;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = await getPlayerPosition(targetId);
                if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                targetStats = getEffectiveStats(target);
                targetHardMdef = target.hardMdef || 0;
                targetName = target.characterName;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dist = Math.sqrt((attackerPos.x - targetPos.x) ** 2 + (attackerPos.y - targetPos.y) ** 2);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Magic damage at effectValue% MATK (110-200%)
            const targetBuffMods = getBuffStatModifiers(target);
            const magicTargetInfo = isEnemy ? getEnemyTargetInfo(target) : getPlayerTargetInfo(target, targetId);
            if (targetBuffMods.overrideElement) magicTargetInfo.element = targetBuffMods.overrideElement;
            magicTargetInfo.buffMods = targetBuffMods;

            const fdResult = calculateMagicSkillDamage(
                getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
                targetHardMdef, effectVal, 'water', magicTargetInfo
            );
            const damage = fdResult.damage;

            target.health = Math.max(0, target.health - damage);
            if (!isEnemy && activeCasts.has(targetId)) interruptCast(targetId, 'damage');
            logger.info(`[SKILL-COMBAT] ${player.characterName} Frost Diver Lv${learnedLevel} → ${targetName} for ${damage} [water] (HP: ${target.health}/${target.maxHealth})`);

            io.emit('skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: 'Frost Diver', skillLevel: learnedLevel, element: 'water',
                damage, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });
            io.emit('combat:damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy, damage, isCritical: false,
                isMiss: false, hitType: 'magical', element: 'water',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });

            // Freeze chance: 35 + level*3, reduced by target MDEF (1% per point)
            if (target.health > 0) {
                const freezeChance = Math.max(0, (35 + learnedLevel * 3) - targetHardMdef);
                const targetEle = isEnemy ? ((target.element && target.element.type) || 'neutral') : 'neutral';
                const isBoss = isEnemy && (target.isBoss || target.mode === 'boss');
                const isUndead = targetEle === 'undead';

                if (!isBoss && !isUndead && Math.random() * 100 < freezeChance) {
                    const freezeDuration = duration || (learnedLevel * 3000);
                    applyBuff(target, {
                        skillId, name: 'frozen', casterId: characterId, casterName: player.characterName,
                        duration: freezeDuration, defReduction: 0, atkIncrease: 0, mdefBonus: 0
                    });
                    logger.info(`[SKILL-COMBAT] ${targetName} FROZEN for ${freezeDuration / 1000}s by Frost Diver`);

                    io.emit('skill:status_applied', {
                        targetId, targetName, isEnemy,
                        casterId: characterId, casterName: player.characterName,
                        skillId, statusName: 'Frozen', duration: freezeDuration
                    });
                    io.emit('skill:buff_applied', {
                        targetId, targetName, isEnemy,
                        casterId: characterId, casterName: player.characterName,
                        skillId, buffName: 'Frozen', duration: freezeDuration,
                        effects: { frozen: true }
                    });
                }
            }

            if (isEnemy) {
                io.emit('enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                io.emit('combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                io.emit('chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Frost Diver!`, timestamp: Date.now() });
                await savePlayerHealthToDB(targetId, 0, target.mana);
            }

            socket.emit('skill:used', { skillId, skillName: 'Frost Diver', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- STONE CURSE (ID 206) — Single target petrification, no damage ---
        if (skill.name === 'stone_curse') {
            if (!targetId) { socket.emit('skill:error', { message: 'No target selected' }); return; }

            let target, targetPos, targetName = '', targetHardMdef = 0;
            if (isEnemy) {
                target = enemies.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = { x: target.x, y: target.y, z: target.z };
                targetName = target.name;
                targetHardMdef = target.hardMdef || target.magicDefense || 0;
            } else {
                target = connectedPlayers.get(targetId);
                if (!target || target.isDead) { socket.emit('skill:error', { message: 'Target not found' }); return; }
                targetPos = await getPlayerPosition(targetId);
                if (!targetPos) { socket.emit('skill:error', { message: 'Target position unknown' }); return; }
                targetName = target.characterName;
                targetHardMdef = target.hardMdef || 0;
            }

            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dist = Math.sqrt((attackerPos.x - targetPos.x) ** 2 + (attackerPos.y - targetPos.y) ** 2);
            if (dist > (skill.range || 150) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 150 });
                return;
            }

            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Roll success rate: effectValue% (24-60%), reduced by target MDEF
            const successRate = Math.max(0, effectVal - targetHardMdef);
            const targetEle = isEnemy ? ((target.element && target.element.type) || 'neutral') : 'neutral';
            const isBoss = isEnemy && (target.isBoss || target.mode === 'boss');
            const isUndead = targetEle === 'undead';

            let stoneApplied = false;
            if (!isBoss && !isUndead && Math.random() * 100 < successRate) {
                // Apply stone curse: 20s duration (simplified from 3s half + 20s full)
                const stoneDuration = duration || 20000;
                applyBuff(target, {
                    skillId, name: 'stone_curse', casterId: characterId, casterName: player.characterName,
                    duration: stoneDuration, defReduction: 0, atkIncrease: 0, mdefBonus: 0,
                    hpDrainTick: Date.now() + 5000 // Start HP drain after 5s
                });
                stoneApplied = true;
                logger.info(`[SKILL-COMBAT] ${targetName} PETRIFIED for ${stoneDuration / 1000}s by Stone Curse (rate: ${successRate}%)`);

                io.emit('skill:status_applied', {
                    targetId, targetName, isEnemy,
                    casterId: characterId, casterName: player.characterName,
                    skillId, statusName: 'Stone Curse', duration: stoneDuration
                });
                io.emit('skill:buff_applied', {
                    targetId, targetName, isEnemy,
                    casterId: characterId, casterName: player.characterName,
                    skillId, buffName: 'Stone Curse', duration: stoneDuration,
                    effects: { petrified: true }
                });
            } else if (isBoss || isUndead) {
                logger.info(`[SKILL-COMBAT] Stone Curse on ${targetName} — immune (${isBoss ? 'boss' : 'undead'})`);
            } else {
                logger.info(`[SKILL-COMBAT] Stone Curse on ${targetName} — failed (rate: ${successRate}%, rolled miss)`);
            }

            socket.emit('skill:used', { skillId, skillName: 'Stone Curse', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, stoneApplied });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- SIGHT (ID 205) — Self buff, reveals hidden enemies ---
        if (skill.name === 'sight') {
            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);

            const sightDuration = duration || 10000;
            applyBuff(player, {
                skillId, name: 'sight', casterId: characterId, casterName: player.characterName,
                duration: sightDuration, defReduction: 0, atkIncrease: 0, mdefBonus: 0
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} SIGHT active for ${sightDuration / 1000}s`);

            io.emit('skill:buff_applied', {
                targetId: characterId, targetName: player.characterName, isEnemy: false,
                casterId: characterId, casterName: player.characterName,
                skillId, buffName: 'Sight', duration: sightDuration,
                effects: { revealHidden: true, aoeRadius: 700 }
            });

            socket.emit('skill:used', { skillId, skillName: 'Sight', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- FIRE WALL (ID 209) — Ground-targeted persistent fire zone ---
        if (skill.name === 'fire_wall') {
            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            // Determine wall position: use ground coordinates from client, fallback to target/caster
            let wallPos;
            if (hasGroundPos) {
                wallPos = { x: groundX, y: groundY, z: groundZ || 0 };
            } else if (targetId && isEnemy) {
                const enemy = enemies.get(targetId);
                if (enemy && !enemy.isDead) wallPos = { x: enemy.x, y: enemy.y, z: enemy.z };
            } else if (targetId && !isEnemy) {
                wallPos = await getPlayerPosition(targetId);
            }
            if (!wallPos) {
                const attackerPos = await getPlayerPosition(characterId);
                if (!attackerPos) return;
                // Place 200 units in front of caster (default forward direction)
                wallPos = { x: attackerPos.x + 200, y: attackerPos.y, z: attackerPos.z };
            }

            // Range check
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            const dist = Math.sqrt((attackerPos.x - wallPos.x) ** 2 + (attackerPos.y - wallPos.y) ** 2);
            if (dist > (skill.range || 450) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 450 });
                return;
            }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Max 3 concurrent fire walls per caster — remove oldest if exceeded
            while (countGroundEffects(characterId, 'fire_wall') >= 3) {
                const removedId = removeOldestGroundEffect(characterId, 'fire_wall');
                if (removedId) io.emit('skill:ground_effect_removed', { effectId: removedId, type: 'fire_wall', reason: 'max_limit' });
            }

            const wallDuration = duration || ((5 + learnedLevel - 1) * 1000);
            const hitLimit = effectVal; // 3-12 hits
            const casterStats = getEffectiveStats(player);

            const effectId = createGroundEffect({
                type: 'fire_wall', casterId: characterId, casterName: player.characterName,
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                hitLimit, hitsRemaining: hitLimit,
                duration: wallDuration, element: 'fire',
                dmgPercent: 50, // 50% MATK per hit
                casterStats: { ...casterStats }, // Snapshot caster stats at cast time
                casterHardMdef: 0,
                radius: 150, // Fire wall collision radius (1x3 cells ≈ 150 UE units)
                knockback: 200, // Knockback distance in UE units
                hitCooldowns: new Map(), // Track per-target hit timing
                hitInterval: 500 // 500ms between hits on same target
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} Fire Wall Lv${learnedLevel}: placed at (${Math.floor(wallPos.x)}, ${Math.floor(wallPos.y)}), ${hitLimit} hits, ${wallDuration / 1000}s`);

            io.emit('skill:ground_effect_created', {
                effectId, type: 'fire_wall',
                casterId: characterId, casterName: player.characterName,
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                duration: wallDuration, hitLimit, element: 'fire', radius: 150
            });

            socket.emit('skill:used', { skillId, skillName: 'Fire Wall', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- SAFETY WALL (ID 211) — Ground-targeted protection zone (1x1 cell) ---
        if (skill.name === 'safety_wall') {
            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.isFrozen || casterBuffs.isStoned) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            // Determine placement: use ground coordinates from client, fallback to target/caster
            let wallPos;
            if (hasGroundPos) {
                wallPos = { x: groundX, y: groundY, z: groundZ || 0 };
            } else if (targetId && isEnemy) {
                const enemy = enemies.get(targetId);
                if (enemy && !enemy.isDead) wallPos = { x: enemy.x, y: enemy.y, z: enemy.z };
            } else if (targetId && !isEnemy) {
                wallPos = await getPlayerPosition(targetId);
            }
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;
            if (!wallPos) wallPos = { x: attackerPos.x, y: attackerPos.y, z: attackerPos.z };

            // Range check
            const dist = Math.sqrt((attackerPos.x - wallPos.x) ** 2 + (attackerPos.y - wallPos.y) ** 2);
            if (dist > (skill.range || 900) + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId, isEnemy, distance: dist, requiredRange: skill.range || 900 });
                return;
            }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const wallDuration = duration || ((learnedLevel + 1) * 5000);
            const hitsBlocked = effectVal; // 2-11 hits

            const effectId = createGroundEffect({
                type: 'safety_wall', casterId: characterId, casterName: player.characterName,
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                hitsRemaining: hitsBlocked, hitLimit: hitsBlocked,
                duration: wallDuration,
                radius: 100 // 1x1 cell ≈ 100 UE units
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} Safety Wall Lv${learnedLevel}: placed at (${Math.floor(wallPos.x)}, ${Math.floor(wallPos.y)}), ${hitsBlocked} blocks, ${wallDuration / 1000}s`);

            io.emit('skill:ground_effect_created', {
                effectId, type: 'safety_wall',
                casterId: characterId, casterName: player.characterName,
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                duration: wallDuration, hitsBlocked, radius: 100
            });

            socket.emit('skill:used', { skillId, skillName: 'Safety Wall', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- PASSIVE SKILLS (Sword Mastery 100, 2H Sword Mastery 101, HP Recovery 102, SP Recovery 204) ---
        // Passives cannot be "used" — they are always active
        if (skill.type === 'passive') {
            socket.emit('skill:error', { message: `${skill.displayName} is a passive skill — its effects are always active` });
            return;
        }

        // --- FALLBACK for any other active skill not yet implemented ---
        // Generic SP deduction + notification
        player.mana = Math.max(0, player.mana - spCost);
        applySkillDelays(characterId, player, skillId, levelData, socket);

        socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
        socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });

        logger.info(`[SKILLS] ${player.characterName} used ${skill.displayName} Lv${learnedLevel} (SP: ${spCost}, remaining: ${player.mana}) — generic handler`);
    });

    // Use a consumable item
    socket.on('inventory:use', async (data) => {
        logger.info(`[RECV] inventory:use from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        const inventoryId = parseInt(data.inventoryId);
        if (!inventoryId) {
            socket.emit('inventory:error', { message: 'Invalid inventory ID' });
            return;
        }
        
        // Verify ownership
        try {
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.quantity, i.item_type, i.name,
                        i.max_hp_bonus, i.max_sp_bonus
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
            
            if (result.rows.length === 0) {
                socket.emit('inventory:error', { message: 'Item not found in your inventory' });
                return;
            }
            
            const item = result.rows[0];
            
            if (item.item_type !== 'consumable') {
                socket.emit('inventory:error', { message: 'This item cannot be used' });
                return;
            }
            
            // Apply consumable effect (HP/SP restoration)
            let healed = 0, spRestored = 0;
            if (item.max_hp_bonus > 0 || item.item_id === 1001) {
                // Potions restore HP based on item_id
                const hpRestore = { 1001: 50, 1002: 150, 1003: 350, 1005: 70 };
                healed = hpRestore[item.item_id] || item.max_hp_bonus || 0;
                player.health = Math.min(player.maxHealth, player.health + healed);
            }
            if (item.item_id === 1004) {
                // Blue Potion restores SP
                spRestored = 60;
                player.mana = Math.min(player.maxMana, player.mana + spRestored);
            }
            
            // Remove 1 from stack
            await removeItemFromInventory(inventoryId, 1);
            
            // Save health/mana
            await savePlayerHealthToDB(characterId, player.health, player.mana);
            
            // Send updated health to all
            io.emit('combat:health_update', {
                characterId, health: player.health, maxHealth: player.maxHealth,
                mana: player.mana, maxMana: player.maxMana
            });
            
            // Notify client of use result
            socket.emit('inventory:used', {
                inventoryId, itemId: item.item_id, itemName: item.name,
                healed, spRestored,
                health: player.health, maxHealth: player.maxHealth,
                mana: player.mana, maxMana: player.maxMana
            });
            logger.info(`[ITEMS] ${player.characterName} used ${item.name}: +${healed}HP +${spRestored}SP`);
            
            // Refresh inventory
            const inventory = await getPlayerInventory(characterId);
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            
        } catch (err) {
            logger.error(`[ITEMS] Use item error: ${err.message}`);
            socket.emit('inventory:error', { message: 'Failed to use item' });
        }
    });
    
    // Equip or unequip an item
    socket.on('inventory:equip', async (data) => {
        logger.info(`[RECV] inventory:equip from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        const inventoryId = parseInt(data.inventoryId);
        // Handle both boolean false and string "false" from Blueprint's string field emission
        const equip = data.equip === true || data.equip === 'true';
        
        try {
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.is_equipped, i.item_type, i.equip_slot, i.name,
                        i.atk, i.def, i.matk, i.mdef,
                        i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                        i.required_level, i.weapon_type, i.aspd_modifier, i.weapon_range
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
            
            if (result.rows.length === 0) {
                socket.emit('inventory:error', { message: 'Item not found' });
                return;
            }
            
            const item = result.rows[0];
            
            if (!item.equip_slot) {
                socket.emit('inventory:error', { message: 'This item cannot be equipped' });
                return;
            }
            
            if (equip && item.required_level > (player.stats.level || 1)) {
                socket.emit('inventory:error', { message: `Requires level ${item.required_level}` });
                return;
            }
            
            // Ensure equipment tracking objects exist
            if (!player.equipmentBonuses) player.equipmentBonuses = { str: 0, agi: 0, vit: 0, int: 0, dex: 0, luk: 0, maxHp: 0, maxSp: 0, hit: 0, flee: 0, critical: 0 };
            if (player.hardDef === undefined) player.hardDef = 0;
            let equippedPosition = null; // Hoisted for use in response
            
            // Helper: remove stat bonuses from an old equipped item row
            const removeOldBonuses = (old) => {
                player.equipmentBonuses.str -= old.str_bonus || 0;
                player.equipmentBonuses.agi -= old.agi_bonus || 0;
                player.equipmentBonuses.vit -= old.vit_bonus || 0;
                player.equipmentBonuses.int -= old.int_bonus || 0;
                player.equipmentBonuses.dex -= old.dex_bonus || 0;
                player.equipmentBonuses.luk -= old.luk_bonus || 0;
                player.equipmentBonuses.maxHp -= old.max_hp_bonus || 0;
                player.equipmentBonuses.maxSp -= old.max_sp_bonus || 0;
                player.equipmentBonuses.hit -= old.hit_bonus || 0;
                player.equipmentBonuses.flee -= old.flee_bonus || 0;
                player.equipmentBonuses.critical -= old.critical_bonus || 0;
                player.hardDef -= old.def || 0;
            };

            if (equip) {
                // Determine the equipped_position for this item
                equippedPosition = item.equip_slot; // default: same as equip_slot

                if (item.equip_slot === 'accessory') {
                    // Dual-accessory support: find which accessory slots are occupied
                    const accCheck = await pool.query(
                        `SELECT ci.inventory_id, ci.equipped_position
                         FROM character_inventory ci
                         WHERE ci.character_id = $1 AND ci.is_equipped = true AND ci.equipped_position IN ('accessory_1', 'accessory_2')`,
                        [characterId]
                    );
                    const occupied = accCheck.rows.map(r => r.equipped_position);
                    if (!occupied.includes('accessory_1')) {
                        equippedPosition = 'accessory_1';
                    } else if (!occupied.includes('accessory_2')) {
                        equippedPosition = 'accessory_2';
                    } else {
                        // Both occupied — replace accessory_1 (oldest convention)
                        equippedPosition = 'accessory_1';
                        const oldAcc = accCheck.rows.find(r => r.equipped_position === 'accessory_1');
                        if (oldAcc) {
                            // Fetch bonuses of old accessory to remove
                            const oldAccData = await pool.query(
                                `SELECT i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                        i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus
                                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                                 WHERE ci.inventory_id = $1`,
                                [oldAcc.inventory_id]
                            );
                            if (oldAccData.rows.length > 0) removeOldBonuses(oldAccData.rows[0]);
                            await pool.query(
                                'UPDATE character_inventory SET is_equipped = false, equipped_position = NULL WHERE inventory_id = $1',
                                [oldAcc.inventory_id]
                            );
                        }
                    }
                } else {
                    // Non-accessory: unequip any item currently in this slot (including its bonuses)
                    const oldEquipped = await pool.query(
                        `SELECT ci.inventory_id, i.def, i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
                                i.max_hp_bonus, i.max_sp_bonus, i.hit_bonus, i.flee_bonus, i.critical_bonus,
                                i.equip_slot, i.atk, i.aspd_modifier, i.weapon_range
                         FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                         WHERE ci.character_id = $1 AND ci.is_equipped = true AND i.equip_slot = $2`,
                        [characterId, item.equip_slot]
                    );
                    for (const old of oldEquipped.rows) {
                        removeOldBonuses(old);
                    }

                    await pool.query(
                        `UPDATE character_inventory SET is_equipped = false, equipped_position = NULL
                         WHERE character_id = $1 AND is_equipped = true
                         AND item_id IN (SELECT item_id FROM items WHERE equip_slot = $2)`,
                        [characterId, item.equip_slot]
                    );
                }

                // Equip the new item with its position
                await pool.query(
                    'UPDATE character_inventory SET is_equipped = true, equipped_position = $1 WHERE inventory_id = $2',
                    [equippedPosition, inventoryId]
                );

                // Apply this item's stat bonuses
                player.equipmentBonuses.str += item.str_bonus || 0;
                player.equipmentBonuses.agi += item.agi_bonus || 0;
                player.equipmentBonuses.vit += item.vit_bonus || 0;
                player.equipmentBonuses.int += item.int_bonus || 0;
                player.equipmentBonuses.dex += item.dex_bonus || 0;
                player.equipmentBonuses.luk += item.luk_bonus || 0;
                player.equipmentBonuses.maxHp += item.max_hp_bonus || 0;
                player.equipmentBonuses.maxSp += item.max_sp_bonus || 0;
                player.equipmentBonuses.hit += item.hit_bonus || 0;
                player.equipmentBonuses.flee += item.flee_bonus || 0;
                player.equipmentBonuses.critical += item.critical_bonus || 0;
                player.hardDef += item.def || 0;

                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = item.atk || 0;
                    player.attackRange = item.weapon_range || COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = item.aspd_modifier || 0;
                }

                logger.info(`[ITEMS] ${player.characterName} equipped ${item.name} (position: ${equippedPosition}, bonuses: ${JSON.stringify(player.equipmentBonuses)}, hardDef: ${player.hardDef})`);
            } else {
                // Remove this item's stat bonuses before unequipping
                removeOldBonuses(item);

                await pool.query(
                    'UPDATE character_inventory SET is_equipped = false, equipped_position = NULL WHERE inventory_id = $1',
                    [inventoryId]
                );

                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = 0;
                    player.attackRange = COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = 0;
                }

                logger.info(`[ITEMS] ${player.characterName} unequipped ${item.name}`);
            }
            
            // Recalculate derived stats using EFFECTIVE stats (base + equipment bonuses)
            const effectiveStats = getEffectiveStats(player);
            const derived = calculateDerivedStats(effectiveStats);
            player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            
            // Update maxHP/maxSP from derived stats and save to DB
            player.maxHealth = derived.maxHP;
            player.maxMana = derived.maxSP;
            try {
                await pool.query(
                    'UPDATE characters SET max_health = $1, max_mana = $2 WHERE character_id = $3',
                    [player.maxHealth, player.maxMana, characterId]
                );
            } catch (err) {
                logger.warn(`[DB] Could not update max_health/max_mana for char ${characterId}: ${err.message}`);
            }
            
            // Send updated stats (effective values so UI shows total stats including equipment)
            const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
            
            // Broadcast updated maxHealth so other clients show correct HP bars
            io.emit('combat:health_update', {
                characterId, health: player.health, maxHealth: player.maxHealth,
                mana: player.mana, maxMana: player.maxMana
            });
            
            // Send updated inventory
            const inventory = await getPlayerInventory(characterId);
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            
            socket.emit('inventory:equipped', {
                inventoryId, itemId: item.item_id, itemName: item.name,
                equipped: equip, slot: item.equip_slot,
                equippedPosition: equippedPosition || item.equip_slot,
                weaponType: item.weapon_type, attackRange: player.attackRange,
                aspd: player.aspd, attackIntervalMs: getAttackIntervalMs(player.aspd),
                maxHealth: player.maxHealth, maxMana: player.maxMana,
                critical: derived.critical, hit: derived.hit, flee: derived.flee
            });
            
        } catch (err) {
            logger.error(`[ITEMS] Equip error: ${err.message}`);
            socket.emit('inventory:error', { message: 'Failed to equip item' });
        }
    });
    
    // Drop/discard an item
    socket.on('inventory:drop', async (data) => {
        logger.info(`[RECV] inventory:drop from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        const inventoryId = parseInt(data.inventoryId);
        const quantity = parseInt(data.quantity) || null; // null = drop all
        
        try {
            // Verify ownership
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.quantity, i.name
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );
            
            if (result.rows.length === 0) {
                socket.emit('inventory:error', { message: 'Item not found' });
                return;
            }
            
            const item = result.rows[0];
            const dropQty = quantity || item.quantity;
            
            await removeItemFromInventory(inventoryId, dropQty);
            logger.info(`[ITEMS] ${player.characterName} dropped ${dropQty}x ${item.name}`);
            
            socket.emit('inventory:dropped', {
                inventoryId, itemId: item.item_id, itemName: item.name, quantity: dropQty
            });
            
            // Refresh inventory
            const inventory = await getPlayerInventory(characterId);
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            
        } catch (err) {
            logger.error(`[ITEMS] Drop error: ${err.message}`);
            socket.emit('inventory:error', { message: 'Failed to drop item' });
        }
    });

    // Move/reorder an inventory item (persist slot_index for client grid position)
    socket.on('inventory:move', async (data) => {
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId } = playerInfo;
        const inventoryId = parseInt(data.inventoryId);
        const newSlotIndex = parseInt(data.newSlotIndex);

        if (isNaN(inventoryId) || isNaN(newSlotIndex) || newSlotIndex < 0) {
            socket.emit('inventory:error', { message: 'Invalid move parameters' });
            return;
        }

        try {
            // Get source item and its current slot
            const sourceResult = await pool.query(
                'SELECT inventory_id, slot_index FROM character_inventory WHERE inventory_id = $1 AND character_id = $2',
                [inventoryId, characterId]
            );
            if (sourceResult.rows.length === 0) return;
            const oldSlotIndex = sourceResult.rows[0].slot_index;

            // Check if target slot is occupied by another item
            const targetResult = await pool.query(
                'SELECT inventory_id FROM character_inventory WHERE character_id = $1 AND slot_index = $2 AND inventory_id != $3',
                [characterId, newSlotIndex, inventoryId]
            );

            if (targetResult.rows.length > 0) {
                // Swap: move the occupying item to the source's old slot
                const targetInvId = targetResult.rows[0].inventory_id;
                await pool.query(
                    'UPDATE character_inventory SET slot_index = $1 WHERE inventory_id = $2',
                    [oldSlotIndex, targetInvId]
                );
                logger.info(`[ITEMS] Char ${characterId} swap: inv_id ${targetInvId} moved to slot ${oldSlotIndex}`);
            }

            // Move source item to the new slot
            await pool.query(
                'UPDATE character_inventory SET slot_index = $1 WHERE inventory_id = $2',
                [newSlotIndex, inventoryId]
            );
            logger.info(`[ITEMS] Char ${characterId} moved inv_id ${inventoryId} to slot ${newSlotIndex}`);

            // Refresh inventory for client
            const inventory = await getPlayerInventory(characterId);
            socket.emit('inventory:data', { items: inventory, zuzucoin: playerInfo.player.zuzucoin });
        } catch (err) {
            logger.error(`[ITEMS] Move error: ${err.message}`);
            socket.emit('inventory:error', { message: 'Failed to move item' });
        }
    });

    // ============================================================
    // NPC Shop Events
    // ============================================================

    // Open shop — returns shop inventory and player's current zuzucoin
    socket.on('shop:open', async (data) => {
        logger.info(`[RECV] shop:open from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const shopId = parseInt(data.shopId);
        const shop = NPC_SHOPS[shopId];
        if (!shop) {
            socket.emit('shop:error', { message: 'Shop not found' });
            return;
        }

        const shopItems = shop.itemIds
            .map(id => itemDefinitions.get(id))
            .filter(Boolean)
            .map(item => ({
                itemId: item.item_id,
                name: item.name,
                description: item.description,
                itemType: item.item_type,
                buyPrice: item.price * 2,
                sellPrice: item.price,
                icon: item.icon,
                atk: item.atk || 0,
                def: item.def || 0,
                weaponType: item.weapon_type || '',
                weaponRange: item.weapon_range || 0,
                aspdModifier: item.aspd_modifier || 0,
                requiredLevel: item.required_level || 1,
                stackable: item.stackable || false
            }));

        socket.emit('shop:data', {
            shopId,
            shopName: shop.name,
            items: shopItems,
            playerZuzucoin: playerInfo.player.zuzucoin
        });
        logger.info(`[SEND] shop:data to ${socket.id}: shop=${shop.name} items=${shopItems.length} zuzucoin=${playerInfo.player.zuzucoin}`);
    });

    // Buy item from shop
    socket.on('shop:buy', async (data) => {
        logger.info(`[RECV] shop:buy from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const shopId = parseInt(data.shopId);
        const itemId = parseInt(data.itemId);
        const quantity = Math.max(1, parseInt(data.quantity) || 1);

        const shop = NPC_SHOPS[shopId];
        if (!shop || !shop.itemIds.includes(itemId)) {
            socket.emit('shop:error', { message: 'Item not available in this shop' });
            return;
        }

        const itemDef = itemDefinitions.get(itemId);
        if (!itemDef) {
            socket.emit('shop:error', { message: 'Item not found' });
            return;
        }

        if (itemDef.required_level > (player.stats.level || 1)) {
            socket.emit('shop:error', { message: `Requires level ${itemDef.required_level}` });
            return;
        }

        const totalCost = itemDef.price * 2 * quantity;
        if (player.zuzucoin < totalCost) {
            socket.emit('shop:error', { message: `Not enough Zuzucoin (need ${totalCost}, have ${player.zuzucoin})` });
            return;
        }

        try {
            const newZeny = player.zuzucoin - totalCost;
            await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);
            player.zuzucoin = newZeny;

            const added = await addItemToInventory(characterId, itemId, quantity);
            if (!added) {
                // Rollback zuzucoin if item add failed
                await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [player.zuzucoin + totalCost, characterId]);
                player.zuzucoin += totalCost;
                socket.emit('shop:error', { message: 'Failed to add item to inventory' });
                return;
            }

            logger.info(`[SHOP] ${player.characterName} bought ${quantity}x ${itemDef.name} for ${totalCost}z (remaining: ${newZeny}z)`);

            const inventory = await getPlayerInventory(characterId);
            socket.emit('shop:bought', { itemId, itemName: itemDef.name, quantity, totalCost, newZuzucoin: newZeny });
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            logger.info(`[SEND] shop:bought + inventory:data to ${socket.id}`);
        } catch (err) {
            logger.error(`[SHOP] Buy error for char ${characterId}: ${err.message}`);
            socket.emit('shop:error', { message: 'Purchase failed' });
        }
    });

    // Sell item to shop
    socket.on('shop:sell', async (data) => {
        logger.info(`[RECV] shop:sell from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const inventoryId = parseInt(data.inventoryId);
        const quantity = Math.max(1, parseInt(data.quantity) || 1);

        if (!inventoryId || isNaN(inventoryId)) {
            socket.emit('shop:error', { message: 'Invalid inventory ID' });
            return;
        }

        try {
            // Verify ownership + get item details
            const itemResult = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
                        i.name, i.price, i.stackable
                 FROM character_inventory ci
                 JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                [inventoryId, characterId]
            );

            if (itemResult.rows.length === 0) {
                socket.emit('shop:error', { message: 'Item not found in your inventory' });
                return;
            }

            const item = itemResult.rows[0];

            if (item.is_equipped) {
                socket.emit('shop:error', { message: 'Cannot sell an equipped item. Unequip it first.' });
                return;
            }

            const sellQty = Math.min(quantity, item.quantity);
            const sellPrice = (item.price || 0) * sellQty;
            const newZeny = player.zuzucoin + sellPrice;

            await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);
            player.zuzucoin = newZeny;

            await removeItemFromInventory(inventoryId, sellQty);

            logger.info(`[SHOP] ${player.characterName} sold ${sellQty}x ${item.name} for ${sellPrice}z (total: ${newZeny}z)`);

            const inventory = await getPlayerInventory(characterId);
            socket.emit('shop:sold', { inventoryId, itemName: item.name, quantity: sellQty, sellPrice, newZuzucoin: newZeny });
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            logger.info(`[SEND] shop:sold + inventory:data to ${socket.id}`);
        } catch (err) {
            logger.error(`[SHOP] Sell error for char ${characterId}: ${err.message}`);
            socket.emit('shop:error', { message: 'Sale failed' });
        }
    });

});

// ============================================================
// Cast Completion — Executes a skill after cast timer finishes
// Re-validates, then re-triggers the skill:use handler with _castComplete flag
// to skip the cast time check and execute the skill immediately.
// ============================================================
async function executeCastComplete(characterId, cast) {
    const player = connectedPlayers.get(characterId);
    if (!player || player.isDead) {
        const sock = io.sockets.sockets.get(cast.socketId);
        if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Player no longer valid' });
        return;
    }

    // Re-check CC
    const ccMods = getBuffStatModifiers(player);
    if (ccMods.isFrozen || ccMods.isStoned) {
        const sock = io.sockets.sockets.get(cast.socketId);
        if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Status effect prevents casting' });
        return;
    }

    // Re-check SP (not consumed during cast, only at execution)
    if (cast.spCost > 0 && player.mana < cast.spCost) {
        const sock = io.sockets.sockets.get(cast.socketId);
        if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Not enough SP' });
        return;
    }

    // Re-check range for ground-targeted skills
    if (cast.hasGroundPos && cast.skill.targetType === 'ground') {
        const casterPos = await getPlayerPosition(characterId);
        if (casterPos) {
            const dx = casterPos.x - cast.groundX;
            const dy = casterPos.y - cast.groundY;
            const dist = Math.sqrt(dx * dx + dy * dy);
            const skillRange = cast.skill.range || 900;
            if (dist > skillRange + COMBAT.RANGE_TOLERANCE) {
                const sock = io.sockets.sockets.get(cast.socketId);
                if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Ground position out of range' });
                return;
            }
        }
    }

    // Re-check target validity and range for single-target skills
    if (cast.targetId && cast.skill.targetType === 'single') {
        let target, targetPos;
        if (cast.isEnemy) {
            target = enemies.get(cast.targetId);
            if (!target || target.isDead) {
                const sock = io.sockets.sockets.get(cast.socketId);
                if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Target no longer valid' });
                return;
            }
            targetPos = { x: target.x, y: target.y, z: target.z };
        } else {
            target = connectedPlayers.get(cast.targetId);
            if (!target || target.isDead) {
                const sock = io.sockets.sockets.get(cast.socketId);
                if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Target no longer valid' });
                return;
            }
            targetPos = await getPlayerPosition(cast.targetId);
        }

        if (targetPos) {
            const casterPos = await getPlayerPosition(characterId);
            if (casterPos) {
                const dx = casterPos.x - targetPos.x;
                const dy = casterPos.y - targetPos.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                const skillRange = cast.skill.range || player.attackRange || COMBAT.MELEE_RANGE;
                if (dist > skillRange + COMBAT.RANGE_TOLERANCE) {
                    const sock = io.sockets.sockets.get(cast.socketId);
                    if (sock) sock.emit('skill:cast_failed', { skillId: cast.skillId, reason: 'Target out of range' });
                    return;
                }
            }
        }
    }

    // Broadcast cast complete to all clients (removes cast bar)
    io.emit('skill:cast_complete', { casterId: characterId, skillId: cast.skillId });

    // Re-trigger the skill:use handler with _castComplete flag to skip cast time
    // Socket.io: socket.listeners() returns registered handler functions
    const sock = io.sockets.sockets.get(cast.socketId);
    if (sock) {
        const handlers = sock.listeners('skill:use');
        if (handlers.length > 0) {
            handlers[0]({
                skillId: cast.skillId, targetId: cast.targetId,
                isEnemy: cast.isEnemy, _castComplete: true,
                groundX: cast.groundX, groundY: cast.groundY, groundZ: cast.groundZ
            });
        }
    }
    logger.info(`[CAST] ${cast.casterName}'s ${cast.skill.displayName} cast complete — executing`);
}

// ============================================================
// Combat Tick Loop (RO-style auto-attack processing)
// Handles: player-vs-player, player-vs-enemy, enemy respawns, cast completion
// Runs every COMBAT_TICK_MS
// ============================================================
setInterval(async () => {
    const now = Date.now();

    // --- Cast Completion Check ---
    // Check if any active casts have finished their timer
    for (const [charId, cast] of activeCasts.entries()) {
        if (now >= cast.castEndTime) {
            activeCasts.delete(charId);
            executeCastComplete(charId, cast);
        }
    }

    for (const [attackerId, atkState] of autoAttackState.entries()) {
        const attacker = connectedPlayers.get(attackerId);
        if (!attacker) {
            autoAttackState.delete(attackerId);
            continue;
        }
        
        // Skip if attacker is dead
        if (attacker.isDead) {
            autoAttackState.delete(attackerId);
            const attackerSocket = io.sockets.sockets.get(attacker.socketId);
            if (attackerSocket) {
                const deadStopPayload = { reason: 'You died' };
                attackerSocket.emit('combat:auto_attack_stopped', deadStopPayload);
                logger.info(`[SEND] combat:auto_attack_stopped to ${attacker.socketId}: ${JSON.stringify(deadStopPayload)}`);
            }
            continue;
        }

        // Skip if attacker is frozen or stoned (can't attack while CC'd)
        const attackerCCMods = getBuffStatModifiers(attacker);
        if (attackerCCMods.isFrozen || attackerCCMods.isStoned) continue;
        
        // ========== ENEMY TARGET ==========
        if (atkState.isEnemy) {
            const enemy = enemies.get(atkState.targetCharId);
            
            if (!enemy || enemy.isDead) {
                autoAttackState.delete(attackerId);
                const attackerSocket = io.sockets.sockets.get(attacker.socketId);
                if (attackerSocket) {
                    const lostPayload = { reason: enemy ? 'Enemy died' : 'Enemy gone', isEnemy: true };
                    attackerSocket.emit('combat:target_lost', lostPayload);
                    logger.info(`[SEND] combat:target_lost to ${attacker.socketId}: ${JSON.stringify(lostPayload)}`);
                }
                continue;
            }
            
            // ASPD timing check
            const attackInterval = getAttackIntervalMs(attacker.aspd);
            if (now - attacker.lastAttackTime < attackInterval) continue;
            
            // Range check
            try {
                const attackerPos = await getPlayerPosition(attackerId);
                if (!attackerPos) continue;
                
                const dx = attackerPos.x - enemy.x;
                const dy = attackerPos.y - enemy.y;
                const distance = Math.sqrt(dx * dx + dy * dy);
                
                if (distance > attacker.attackRange) {
                    // Out of range — broadcast range status so client moves toward enemy
                    // Use padded range so client stops INSIDE the attack range (not at boundary)
                    const attackerSocket = io.sockets.sockets.get(attacker.socketId);
                    if (attackerSocket) {
                        attackerSocket.emit('combat:out_of_range', {
                            targetId: atkState.targetCharId,
                            isEnemy: true,
                            targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                            distance, requiredRange: Math.max(0, attacker.attackRange - COMBAT.RANGE_TOLERANCE)
                        });
                    }
                    continue;
                }
                
                // Safety Wall check: if target is inside a Safety Wall, block melee damage
                const safetyWalls = getGroundEffectsAtPosition(enemy.x, enemy.y, enemy.z || 0, 100);
                const safetyWall = safetyWalls.find(e => e.type === 'safety_wall' && e.hitsRemaining > 0);
                if (safetyWall && attacker.attackRange <= COMBAT.MELEE_RANGE + COMBAT.RANGE_TOLERANCE) {
                    safetyWall.hitsRemaining--;
                    attacker.lastAttackTime = now;
                    logger.info(`[COMBAT] Safety Wall blocked melee attack on ${enemy.name} (${safetyWall.hitsRemaining} hits remaining)`);
                    io.emit('skill:ground_effect_blocked', { effectId: safetyWall.id, type: 'safety_wall', hitsRemaining: safetyWall.hitsRemaining, targetId: enemy.enemyId, targetName: enemy.name, isEnemy: true });
                    if (safetyWall.hitsRemaining <= 0) {
                        removeGroundEffect(safetyWall.id);
                        io.emit('skill:ground_effect_removed', { effectId: safetyWall.id, type: 'safety_wall', reason: 'hits_exhausted' });
                    }
                    continue;
                }

                // IN RANGE: Execute attack on enemy using full RO damage formula
                // Includes: HIT/FLEE check, Critical, Perfect Dodge, Size Penalty, Element, DEF
                const combatResult = calculatePhysicalDamage(
                    getEffectiveStats(attacker),
                    enemy.stats,
                    enemy.hardDef || 0,
                    getEnemyTargetInfo(enemy),
                    getAttackerInfo(attacker)
                );

                attacker.lastAttackTime = now;
                const { damage, isCritical, isMiss, hitType, element: atkElement } = combatResult;

                // Build damage payload (sent for all hit types including miss/dodge)
                const damagePayload = {
                    attackerId,
                    attackerName: attacker.characterName,
                    targetId: enemy.enemyId,
                    targetName: enemy.name,
                    isEnemy: true,
                    damage,
                    isCritical,
                    isMiss,
                    hitType,  // 'normal', 'critical', 'miss', 'dodge', 'perfectDodge'
                    element: atkElement,
                    targetHealth: enemy.health,
                    targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    timestamp: now
                };

                if (isMiss) {
                    // Miss/Dodge: don't deal damage, still broadcast event
                    io.emit('combat:damage', damagePayload);
                    logger.info(`[COMBAT] ${attacker.characterName} ${hitType} against ${enemy.name}(${enemy.enemyId})`);
                    continue;
                }

                // Apply damage
                enemy.health = Math.max(0, enemy.health - damage);
                damagePayload.targetHealth = enemy.health;

                logger.info(`[COMBAT] ${attacker.characterName} hit enemy ${enemy.name}(${enemy.enemyId}) for ${damage}${isCritical ? ' CRIT' : ''} [${atkElement}→${(enemy.element||{}).type||'neutral'}] (HP: ${enemy.health}/${enemy.maxHealth})`);

                io.emit('combat:damage', damagePayload);

                // Fire-element auto-attack breaks Frozen status
                if (atkElement === 'fire' && enemy.activeBuffs) {
                    const hadFrozen = enemy.activeBuffs.some(b => b.name === 'frozen');
                    if (hadFrozen) {
                        enemy.activeBuffs = enemy.activeBuffs.filter(b => b.name !== 'frozen');
                        io.emit('skill:buff_removed', { targetId: enemy.enemyId, isEnemy: true, buffName: 'frozen', reason: 'fire_damage' });
                        logger.info(`[COMBAT] Fire auto-attack removed Frozen from ${enemy.name}`);
                    }
                }

                // Broadcast enemy health update to all (for health bar visibility)
                io.emit('enemy:health_update', {
                    enemyId: enemy.enemyId,
                    health: enemy.health,
                    maxHealth: enemy.maxHealth,
                    inCombat: enemy.inCombatWith.size > 0
                });

                // Enemy death
                if (enemy.health <= 0) {
                    enemy.isDead = true;
                    
                    logger.info(`[COMBAT] Enemy ${enemy.name}(${enemy.enemyId}) killed by ${attacker.characterName}`);
                    
                    // Stop all players auto-attacking this enemy
                    // DO NOT send combat:target_lost here — all attackers receive enemy:death broadcast instead
                    // Sending both events causes Blueprint to process target_lost (nulls enemy ref) then
                    // enemy:death tries to access the destroyed ref → EXCEPTION_ACCESS_VIOLATION crash
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.isEnemy && otherAtk.targetCharId === enemy.enemyId) {
                            autoAttackState.delete(otherId);
                            logger.info(`[COMBAT] Cleared auto-attack for player ${otherId} (enemy ${enemy.enemyId} died)`);
                        }
                    }
                    
                    // Clear combat set
                    enemy.inCombatWith.clear();
                    
                    // ── Award EXP to killer (RO-style dual progression) ──
                    const baseExpReward = enemy.baseExp || 0;
                    const jobExpReward = enemy.jobExp || 0;
                    const expResult = processExpGain(attacker, baseExpReward, jobExpReward);
                    
                    // Send exp:gain event to killer
                    const killerSocket = io.sockets.sockets.get(attacker.socketId);
                    if (killerSocket && (expResult.baseExpGained > 0 || expResult.jobExpGained > 0)) {
                        const expGainPayload = {
                            characterId: attackerId,
                            baseExpGained: expResult.baseExpGained,
                            jobExpGained: expResult.jobExpGained,
                            enemyName: enemy.name,
                            enemyLevel: enemy.level,
                            exp: buildExpPayload(attacker),
                            baseLevelUps: expResult.baseLevelUps,
                            jobLevelUps: expResult.jobLevelUps
                        };
                        killerSocket.emit('exp:gain', expGainPayload);
                        logger.info(`[EXP] ${attacker.characterName} gained +${baseExpReward} BaseEXP +${jobExpReward} JobEXP from ${enemy.name}`);
                        
                        // If any level ups occurred, broadcast updated stats + health
                        if (expResult.baseLevelUps.length > 0 || expResult.jobLevelUps.length > 0) {
                            // Send level up event with full details
                            const levelUpPayload = {
                                characterId: attackerId,
                                characterName: attacker.characterName,
                                baseLevelUps: expResult.baseLevelUps,
                                jobLevelUps: expResult.jobLevelUps,
                                totalStatPoints: expResult.totalStatPoints,
                                totalSkillPoints: expResult.totalSkillPoints,
                                exp: buildExpPayload(attacker)
                            };
                            killerSocket.emit('exp:level_up', levelUpPayload);
                            // Broadcast to other players so they see the level up
                            killerSocket.broadcast.emit('exp:level_up', levelUpPayload);
                            
                            // Send updated stats (derived stats change with level)
                            const effectiveStats = getEffectiveStats(attacker);
                            const newDerived = calculateDerivedStats(effectiveStats);
                            const newFinalAspd = Math.min(COMBAT.ASPD_CAP, newDerived.aspd + (attacker.weaponAspdMod || 0));
                            killerSocket.emit('player:stats', buildFullStatsPayload(attackerId, attacker, effectiveStats, newDerived, newFinalAspd));

                            // Broadcast health update (full heal on level up)
                            io.emit('combat:health_update', {
                                characterId: attackerId,
                                health: attacker.health,
                                maxHealth: attacker.maxHealth,
                                mana: attacker.mana,
                                maxMana: attacker.maxMana
                            });
                            
                            // Chat announcement for level ups
                            for (const lu of expResult.baseLevelUps) {
                                io.emit('chat:receive', {
                                    type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
                                    message: `${attacker.characterName} has reached Base Level ${lu.newLevel}!`,
                                    timestamp: now
                                });
                            }
                            for (const lu of expResult.jobLevelUps) {
                                io.emit('chat:receive', {
                                    type: 'chat:receive', channel: 'SYSTEM', senderId: 0, senderName: 'SYSTEM',
                                    message: `${attacker.characterName} has reached Job Level ${lu.newJobLevel}!`,
                                    timestamp: now
                                });
                            }
                        }
                        
                        // Save EXP data to DB (async, non-blocking)
                        saveExpDataToDB(attackerId, attacker);
                    }
                    
                    // Broadcast enemy death (isDead flag tells client to disable collision + hide mesh)
                    const deathPayload = {
                        enemyId: enemy.enemyId,
                        enemyName: enemy.name,
                        killerId: attackerId,
                        killerName: attacker.characterName,
                        isEnemy: true,
                        isDead: true,
                        baseExp: baseExpReward,
                        jobExp: jobExpReward,
                        exp: enemy.exp,
                        timestamp: now
                    };
                    io.emit('enemy:death', deathPayload);
                    logger.info(`[BROADCAST] enemy:death: ${JSON.stringify(deathPayload)}`);
                    
                    // Roll item drops for the killer
                    const droppedItems = rollEnemyDrops(enemy);
                    if (droppedItems.length > 0) {
                        const lootItems = [];
                        let addedToDb = false;
                        for (const drop of droppedItems) {
                            if (drop.itemId) {
                                // Item exists in DB — add to inventory
                                const added = await addItemToInventory(attackerId, drop.itemId, drop.quantity);
                                if (added) {
                                    const itemDef = itemDefinitions.get(drop.itemId);
                                    lootItems.push({
                                        itemId: drop.itemId,
                                        itemName: drop.itemName,
                                        quantity: drop.quantity,
                                        icon: itemDef ? itemDef.icon : 'default_item',
                                        itemType: itemDef ? itemDef.item_type : 'etc',
                                        isMvpDrop: drop.isMvpDrop || false
                                    });
                                    addedToDb = true;
                                }
                            } else {
                                // RO drop not yet in items DB — still notify client for display
                                lootItems.push({
                                    itemId: null,
                                    itemName: drop.itemName,
                                    quantity: drop.quantity,
                                    icon: 'default_item',
                                    itemType: 'etc',
                                    isMvpDrop: drop.isMvpDrop || false
                                });
                            }
                        }
                        if (lootItems.length > 0) {
                            const killerSocket = io.sockets.sockets.get(attacker.socketId);
                            if (killerSocket) {
                                const lootPayload = {
                                    enemyId: enemy.enemyId,
                                    enemyName: enemy.name,
                                    items: lootItems
                                };
                                killerSocket.emit('loot:drop', lootPayload);
                                logger.info(`[LOOT] ${attacker.characterName} received ${lootItems.length} items from ${enemy.name}: ${lootItems.map(i => `${i.quantity}x ${i.itemName}${i.isMvpDrop ? ' (MVP)' : ''}`).join(', ')}`);
                                // Refresh inventory if any items were actually added to DB
                                if (addedToDb) {
                                    const killerInventory = await getPlayerInventory(attackerId);
                                    killerSocket.emit('inventory:data', { items: killerInventory, zuzucoin: attacker.zuzucoin });
                                }
                            }
                        }
                    }
                    
                    // Schedule enemy respawn
                    setTimeout(() => {
                        enemy.health = enemy.maxHealth;
                        enemy.isDead = false;
                        enemy.x = enemy.spawnX;
                        enemy.y = enemy.spawnY;
                        enemy.z = enemy.spawnZ;
                        enemy.targetPlayerId = null;
                        enemy.inCombatWith = new Set();
                        initEnemyWanderState(enemy);
                        
                        logger.info(`[ENEMY] Respawned ${enemy.name} (ID: ${enemy.enemyId})`);
                        io.emit('enemy:spawn', {
                            enemyId: enemy.enemyId, templateId: enemy.templateId, name: enemy.name,
                            level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
                            monsterClass: enemy.monsterClass, size: enemy.size, race: enemy.race,
                            element: enemy.element,
                            x: enemy.x, y: enemy.y, z: enemy.z
                        });
                    }, enemy.respawnMs);
                }
            } catch (err) {
                logger.error(`Combat tick error (enemy target) for attacker ${attackerId}:`, err.message);
            }
            continue;
        }
        
        // ========== PLAYER TARGET ==========
        // PvP disabled — clear the auto-attack state silently
        if (!PVP_ENABLED) {
            autoAttackState.delete(attackerId);
            const attackerSocket = io.sockets.sockets.get(attacker.socketId);
            if (attackerSocket) attackerSocket.emit('combat:target_lost', { reason: 'PvP is currently disabled', isEnemy: false });
            continue;
        }

        const target = connectedPlayers.get(atkState.targetCharId);

        // Target gone or dead — stop auto-attack, send target_lost
        if (!target || target.isDead) {
            autoAttackState.delete(attackerId);
            const attackerSocket = io.sockets.sockets.get(attacker.socketId);
            if (attackerSocket) {
                const targetLostPayload = { reason: target ? 'Target died' : 'Target disconnected', isEnemy: false };
                attackerSocket.emit('combat:target_lost', targetLostPayload);
                logger.info(`[SEND] combat:target_lost to ${attacker.socketId}: ${JSON.stringify(targetLostPayload)}`);
            }
            continue;
        }
        
        // ASPD timing check
        const attackInterval = getAttackIntervalMs(attacker.aspd);
        if (now - attacker.lastAttackTime < attackInterval) continue;
        
        // Range check using cached Redis positions
        try {
            const attackerPos = await getPlayerPosition(attackerId);
            const targetPos = await getPlayerPosition(atkState.targetCharId);
            
            if (!attackerPos || !targetPos) continue;
            
            const dx = attackerPos.x - targetPos.x;
            const dy = attackerPos.y - targetPos.y;
            const distance = Math.sqrt(dx * dx + dy * dy);
            
            if (distance > attacker.attackRange) {
                // Out of range — broadcast so client knows to move
                // Use padded range so client stops INSIDE the attack range (not at boundary)
                const attackerSocket = io.sockets.sockets.get(attacker.socketId);
                if (attackerSocket) {
                    attackerSocket.emit('combat:out_of_range', {
                        targetId: atkState.targetCharId,
                        isEnemy: false,
                        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                        distance, requiredRange: Math.max(0, attacker.attackRange - COMBAT.RANGE_TOLERANCE)
                    });
                }
                continue;
            }
            
            // Safety Wall check: if PvP target is inside a Safety Wall, block melee damage
            const pvpSafetyWalls = getGroundEffectsAtPosition(targetPos.x, targetPos.y, targetPos.z || 0, 100);
            const pvpSafetyWall = pvpSafetyWalls.find(e => e.type === 'safety_wall' && e.hitsRemaining > 0);
            if (pvpSafetyWall && attacker.attackRange <= COMBAT.MELEE_RANGE + COMBAT.RANGE_TOLERANCE) {
                pvpSafetyWall.hitsRemaining--;
                attacker.lastAttackTime = now;
                logger.info(`[COMBAT] Safety Wall blocked melee attack on ${target.characterName} (${pvpSafetyWall.hitsRemaining} hits remaining)`);
                io.emit('skill:ground_effect_blocked', { effectId: pvpSafetyWall.id, type: 'safety_wall', hitsRemaining: pvpSafetyWall.hitsRemaining, targetId: atkState.targetCharId, targetName: target.characterName, isEnemy: false });
                if (pvpSafetyWall.hitsRemaining <= 0) {
                    removeGroundEffect(pvpSafetyWall.id);
                    io.emit('skill:ground_effect_removed', { effectId: pvpSafetyWall.id, type: 'safety_wall', reason: 'hits_exhausted' });
                }
                continue;
            }

            // === IN RANGE: Execute attack using full RO damage formula ===
            // Includes: HIT/FLEE, Critical, Perfect Dodge, Size, Element, DEF
            const pvpResult = calculatePhysicalDamage(
                getEffectiveStats(attacker),
                getEffectiveStats(target),
                target.hardDef || 0,
                getPlayerTargetInfo(target, atkState.targetCharId),
                getAttackerInfo(attacker)
            );

            attacker.lastAttackTime = now;
            const { damage: pvpDmg, isCritical: pvpCrit, isMiss: pvpMiss, hitType: pvpHitType, element: pvpElement } = pvpResult;

            // Build damage payload
            const damagePayload = {
                attackerId,
                attackerName: attacker.characterName,
                targetId: atkState.targetCharId,
                targetName: target.characterName,
                isEnemy: false,
                damage: pvpDmg,
                isCritical: pvpCrit,
                isMiss: pvpMiss,
                hitType: pvpHitType,
                element: pvpElement,
                targetHealth: target.health,
                targetMaxHealth: target.maxHealth,
                attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: now
            };

            if (pvpMiss) {
                io.emit('combat:damage', damagePayload);
                logger.info(`[COMBAT] ${attacker.characterName} ${pvpHitType} against ${target.characterName}`);
                continue;
            }

            target.health = Math.max(0, target.health - pvpDmg);
            damagePayload.targetHealth = target.health;

            // Cast interruption: damage interrupts casting (100% chance, RO pre-renewal)
            if (activeCasts.has(atkState.targetCharId)) {
                interruptCast(atkState.targetCharId, 'damage');
            }

            logger.info(`[COMBAT] ${attacker.characterName} hit ${target.characterName} for ${pvpDmg}${pvpCrit ? ' CRIT' : ''} [${pvpElement}] (HP: ${target.health}/${target.maxHealth})`);

            io.emit('combat:damage', damagePayload);

            // Fire-element auto-attack breaks Frozen status on PvP target
            if (pvpElement === 'fire' && target.activeBuffs) {
                const hadFrozen = target.activeBuffs.some(b => b.name === 'frozen');
                if (hadFrozen) {
                    target.activeBuffs = target.activeBuffs.filter(b => b.name !== 'frozen');
                    io.emit('skill:buff_removed', { targetId: atkState.targetCharId, isEnemy: false, buffName: 'frozen', reason: 'fire_damage' });
                    logger.info(`[COMBAT] Fire auto-attack removed Frozen from ${target.characterName}`);
                }
            }

            // Check for player death
            if (target.health <= 0) {
                target.isDead = true;

                // Cancel any active cast on death
                if (activeCasts.has(atkState.targetCharId)) {
                    interruptCast(atkState.targetCharId, 'death');
                }

                logger.info(`[COMBAT] ${target.characterName} was killed by ${attacker.characterName}`);

                // Stop all attackers targeting this dead player
                // DO NOT send combat:target_lost here — all clients receive combat:death broadcast instead
                // Sending both events causes Blueprint to process target_lost (nulls player ref) then
                // combat:death tries to access the destroyed ref → EXCEPTION_ACCESS_VIOLATION crash
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === atkState.targetCharId && !otherAtk.isEnemy) {
                        autoAttackState.delete(otherId);
                        logger.info(`[COMBAT] Cleared auto-attack for player ${otherId} (target ${atkState.targetCharId} died)`);
                    }
                }
                
                // Broadcast death (includes health data so client can update safely)
                const deathPayload = {
                    killedId: atkState.targetCharId,
                    killedName: target.characterName,
                    killerId: attackerId,
                    killerName: attacker.characterName,
                    isEnemy: false,
                    targetHealth: 0,
                    targetMaxHealth: target.maxHealth,
                    timestamp: now
                };
                io.emit('combat:death', deathPayload);
                logger.info(`[BROADCAST] combat:death: ${JSON.stringify(deathPayload)}`);
                
                // Kill message in chat
                io.emit('chat:receive', {
                    type: 'chat:receive',
                    channel: 'COMBAT',
                    senderId: 0,
                    senderName: 'SYSTEM',
                    message: `${attacker.characterName} has slain ${target.characterName}!`,
                    timestamp: now
                });
                
                // Save target health to DB
                await savePlayerHealthToDB(atkState.targetCharId, 0, target.mana);
            }
        } catch (err) {
            logger.error(`Combat tick error for attacker ${attackerId}:`, err.message);
        }
    }
}, COMBAT.COMBAT_TICK_MS);

// ============================================================
// RO Classic Natural Regeneration System
// HP ticks every 6s, SP ticks every 8s, Skill regen ticks every 10s
// Formulas from rAthena pre-renewal (iRO Wiki / status.cpp)
// Standing regen only — walking/sitting modifiers are future additions
// ============================================================

// Helper: emit regen update to a single player
function emitRegenUpdate(charId, player) {
    const sock = io.sockets.sockets.get(player.socketId);
    if (sock) {
        sock.emit('combat:health_update', {
            characterId: charId, health: player.health,
            maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana
        });
    }
}

// --- HP Natural Regen (every 6 seconds) ---
// Formula: max(1, floor(MaxHP/200)) + floor(VIT/5)
setInterval(() => {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if (player.health >= player.maxHealth) continue;

        const stats = getEffectiveStats(player);
        let hpRegen = Math.max(1, Math.floor(player.maxHealth / 200));
        hpRegen += Math.floor((stats.vit || 0) / 5);

        const oldHP = player.health;
        player.health = Math.min(player.maxHealth, player.health + hpRegen);

        if (player.health > oldHP) {
            emitRegenUpdate(charId, player);
        }
    }
}, 6000);

// --- SP Natural Regen (every 8 seconds) ---
// Formula: 1 + floor(MaxSP/100) + floor(INT/6)
// Bonus if INT >= 120: extra 4 + floor(INT/2 - 60)
setInterval(() => {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if (player.mana >= player.maxMana) continue;

        const stats = getEffectiveStats(player);
        const intStat = stats.int || stats.int_stat || 0;
        let spRegen = 1 + Math.floor(player.maxMana / 100) + Math.floor(intStat / 6);

        // High INT bonus (120+)
        if (intStat >= 120) {
            spRegen += 4 + Math.floor(intStat / 2 - 60);
        }

        const oldSP = player.mana;
        player.mana = Math.min(player.maxMana, player.mana + spRegen);

        if (player.mana > oldSP) {
            emitRegenUpdate(charId, player);
        }
    }
}, 8000);

// --- Skill-Based Regen (every 10 seconds) ---
// Increase HP Recovery (102): SkillLv * 5 + SkillLv * MaxHP / 500
// Increase SP Recovery (204): SkillLv * 3 + SkillLv * MaxSP / 500
// These are SEPARATE from natural regen and stack additively.
setInterval(() => {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        const learned = player.learnedSkills || {};
        let hpBonus = 0, spBonus = 0;

        // Increase HP Recovery (Swordsman skill 102)
        const hprLv = learned[102] || 0;
        if (hprLv > 0 && player.health < player.maxHealth) {
            hpBonus = Math.floor(hprLv * 5 + hprLv * player.maxHealth / 500);
        }

        // Increase SP Recovery (Mage skill 204)
        const sprLv = learned[204] || 0;
        if (sprLv > 0 && player.mana < player.maxMana) {
            spBonus = Math.floor(sprLv * 3 + sprLv * player.maxMana / 500);
        }

        if (hpBonus > 0 || spBonus > 0) {
            const oldHP = player.health;
            const oldSP = player.mana;
            if (hpBonus > 0) player.health = Math.min(player.maxHealth, player.health + hpBonus);
            if (spBonus > 0) player.mana = Math.min(player.maxMana, player.mana + spBonus);

            if (player.health > oldHP || player.mana > oldSP) {
                emitRegenUpdate(charId, player);
            }
        }
    }
}, 10000);

// ============================================================
// Buff Expiry Tick (every 1 second — check and remove expired buffs)
// ============================================================
setInterval(() => {
    const now = Date.now();

    // Check players
    for (const [charId, player] of connectedPlayers.entries()) {
        const expired = expireBuffs(player);
        if (expired.length > 0) {
            const sock = io.sockets.sockets.get(player.socketId);
            for (const buff of expired) {
                logger.info(`[BUFF] ${player.characterName}: ${buff.name} expired`);
                if (sock) {
                    sock.emit('skill:buff_removed', {
                        targetId: charId, buffName: buff.name, skillId: buff.skillId, reason: 'expired'
                    });
                }
            }
        }

        // Stone Curse HP drain: 1% HP every 5 seconds while petrified
        if (player.activeBuffs && player.activeBuffs.length > 0) {
            const stoneBuff = player.activeBuffs.find(b => b.name === 'stone_curse');
            if (stoneBuff) {
                // Only drain after the first 3 seconds (half-stone phase)
                const stoneElapsed = now - stoneBuff.appliedAt;
                if (stoneElapsed > 3000) {
                    if (!stoneBuff.lastDrainTime) stoneBuff.lastDrainTime = stoneBuff.appliedAt + 3000;
                    if (now - stoneBuff.lastDrainTime >= 5000) {
                        stoneBuff.lastDrainTime = now;
                        const drain = Math.max(1, Math.floor(player.maxHealth * 0.01));
                        player.health = Math.max(1, player.health - drain); // Don't kill from stone drain
                        const sock2 = io.sockets.sockets.get(player.socketId);
                        if (sock2) {
                            sock2.emit('combat:health_update', {
                                characterId: charId, health: player.health,
                                maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana
                            });
                        }
                        logger.info(`[BUFF] Stone Curse drained ${drain} HP from ${player.characterName} (HP: ${player.health}/${player.maxHealth})`);
                    }
                }
            }
        }
    }

    // Check enemies
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead) continue;
        const expired = expireBuffs(enemy);
        if (expired.length > 0) {
            for (const buff of expired) {
                logger.info(`[BUFF] Enemy ${enemy.name}: ${buff.name} expired`);
                // Broadcast buff removal to all clients
                io.emit('skill:buff_removed', {
                    targetId: eid, isEnemy: true, buffName: buff.name, skillId: buff.skillId, reason: 'expired'
                });
            }
        }

        // Stone Curse HP drain for enemies too
        if (enemy.activeBuffs && enemy.activeBuffs.length > 0) {
            const stoneBuff = enemy.activeBuffs.find(b => b.name === 'stone_curse');
            if (stoneBuff) {
                const stoneElapsed = now - stoneBuff.appliedAt;
                if (stoneElapsed > 3000) {
                    if (!stoneBuff.lastDrainTime) stoneBuff.lastDrainTime = stoneBuff.appliedAt + 3000;
                    if (now - stoneBuff.lastDrainTime >= 5000) {
                        stoneBuff.lastDrainTime = now;
                        const drain = Math.max(1, Math.floor(enemy.maxHealth * 0.01));
                        enemy.health = Math.max(1, enemy.health - drain);
                        io.emit('enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: enemy.inCombatWith.size > 0 });
                        logger.info(`[BUFF] Stone Curse drained ${drain} HP from enemy ${enemy.name} (HP: ${enemy.health}/${enemy.maxHealth})`);
                    }
                }
            }
        }
    }
}, 1000);

// ============================================================
// Ground Effects Tick (500ms — Fire Wall damage, expired effect cleanup)
// ============================================================
setInterval(async () => {
    const now = Date.now();
    const effectsToRemove = [];

    for (const [effectId, effect] of activeGroundEffects.entries()) {
        // Remove expired effects
        if (now >= effect.expiresAt) {
            effectsToRemove.push(effectId);
            continue;
        }

        // --- FIRE WALL damage tick ---
        if (effect.type === 'fire_wall') {
            if (effect.hitsRemaining <= 0) {
                effectsToRemove.push(effectId);
                continue;
            }

            // Check enemies in Fire Wall zone
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                const dx = enemy.x - effect.x;
                const dy = enemy.y - effect.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > 150) continue; // Fire Wall radius ~150 UE units

                // Cooldown per target to avoid hitting every 500ms tick (hit once per second per target)
                if (!effect.lastHitTargets) effect.lastHitTargets = {};
                const targetKey = `enemy_${eid}`;
                if (effect.lastHitTargets[targetKey] && now - effect.lastHitTargets[targetKey] < 1000) continue;
                effect.lastHitTargets[targetKey] = now;

                // Calculate fire damage: 50% MATK
                const caster = connectedPlayers.get(effect.casterId);
                if (!caster) continue;
                const casterStats = getEffectiveStats(caster);
                const fwDmg = calculateMagicSkillDamage(casterStats, enemy.stats, enemy.hardDef || 0, 0.5, 'fire', getEnemyTargetInfo(enemy));
                const finalDmg = Math.max(1, fwDmg);

                enemy.health = Math.max(0, enemy.health - finalDmg);
                effect.hitsRemaining--;

                logger.info(`[SKILL-COMBAT] Fire Wall hit ${enemy.name} for ${finalDmg} fire [${effect.hitsRemaining} hits left]`);

                // Fire damage breaks Frozen
                if (enemy.activeBuffs) {
                    const hadFrozen = enemy.activeBuffs.some(b => b.name === 'frozen');
                    if (hadFrozen) {
                        enemy.activeBuffs = enemy.activeBuffs.filter(b => b.name !== 'frozen');
                        io.emit('skill:buff_removed', { targetId: eid, isEnemy: true, buffName: 'frozen', reason: 'fire_damage' });
                    }
                }

                io.emit('combat:damage', {
                    attackerId: effect.casterId, attackerName: caster.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true,
                    damage: finalDmg, isCritical: false, isMiss: false,
                    hitType: 'skill', element: 'fire',
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    timestamp: now
                });
                io.emit('enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: enemy.inCombatWith.size > 0 });

                // Knockback: push enemy away from fire wall center
                if (dist > 0) {
                    const knockDist = 100; // 100 UE units knockback
                    enemy.x += (dx / dist) * knockDist;
                    enemy.y += (dy / dist) * knockDist;
                    io.emit('enemy:move', { enemyId: eid, x: enemy.x, y: enemy.y, z: enemy.z, isMoving: false, knockback: true });
                }

                // Enemy death from Fire Wall
                if (enemy.health <= 0) {
                    enemy.isDead = true;
                    processEnemyDeathFromSkill(enemy, caster, effect.casterId, io);
                }

                if (effect.hitsRemaining <= 0) {
                    effectsToRemove.push(effectId);
                    break;
                }
            }

            // Check players in Fire Wall zone (PvP)
            if (PVP_ENABLED)
            for (const [charId, player] of connectedPlayers.entries()) {
                if (player.isDead || charId === effect.casterId) continue; // Don't hit caster
                if (effect.hitsRemaining <= 0) break;

                const pPos = await getPlayerPosition(charId);
                if (!pPos) continue;
                const dx = pPos.x - effect.x;
                const dy = pPos.y - effect.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > 150) continue;

                if (!effect.lastHitTargets) effect.lastHitTargets = {};
                const targetKey = `player_${charId}`;
                if (effect.lastHitTargets[targetKey] && now - effect.lastHitTargets[targetKey] < 1000) continue;
                effect.lastHitTargets[targetKey] = now;

                const caster = connectedPlayers.get(effect.casterId);
                if (!caster) continue;
                const casterStats = getEffectiveStats(caster);
                const targetStats = getEffectiveStats(player);
                const fwDmg = calculateMagicSkillDamage(casterStats, targetStats, player.hardDef || 0, 0.5, 'fire');
                const finalDmg = Math.max(1, fwDmg);

                player.health = Math.max(0, player.health - finalDmg);
                effect.hitsRemaining--;

                // Cast interruption: Fire Wall damage interrupts casting
                if (activeCasts.has(charId)) {
                    interruptCast(charId, 'damage');
                }

                // Fire damage breaks Frozen
                if (player.activeBuffs) {
                    const hadFrozen = player.activeBuffs.some(b => b.name === 'frozen');
                    if (hadFrozen) {
                        player.activeBuffs = player.activeBuffs.filter(b => b.name !== 'frozen');
                        io.emit('skill:buff_removed', { targetId: charId, isEnemy: false, buffName: 'frozen', reason: 'fire_damage' });
                    }
                }

                io.emit('combat:damage', {
                    attackerId: effect.casterId, attackerName: caster.characterName,
                    targetId: charId, targetName: player.characterName, isEnemy: false,
                    damage: finalDmg, isCritical: false, isMiss: false,
                    hitType: 'skill', element: 'fire',
                    targetX: pPos.x, targetY: pPos.y, targetZ: pPos.z,
                    targetHealth: player.health, targetMaxHealth: player.maxHealth,
                    timestamp: now
                });

                const sock = io.sockets.sockets.get(player.socketId);
                if (sock) {
                    sock.emit('combat:health_update', {
                        characterId: charId, health: player.health,
                        maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana
                    });
                }

                if (effect.hitsRemaining <= 0) {
                    effectsToRemove.push(effectId);
                    break;
                }
            }
        }
    }

    // Clean up expired/exhausted effects
    for (const id of effectsToRemove) {
        const effect = activeGroundEffects.get(id);
        if (effect) {
            removeGroundEffect(id);
            io.emit('skill:ground_effect_removed', { effectId: id, type: effect.type, reason: effect.hitsRemaining <= 0 ? 'hits_exhausted' : 'expired' });
        }
    }
}, 500);

// ============================================================
// Enemy AI Tick Loop (Ragnarok Online-style wandering)
// Enemies wander randomly within their spawn radius when idle
// ============================================================
const ENEMY_AI = {
    WANDER_TICK_MS: 500,        // AI tick interval (ms)
    WANDER_PAUSE_MIN: 3000,     // Min pause before next wander (ms)
    WANDER_PAUSE_MAX: 8000,     // Max pause before next wander (ms)
    WANDER_SPEED: 60,           // Movement speed (units per second)
    WANDER_DIST_MIN: 100,       // Min wander distance per axis (units)
    WANDER_DIST_MAX: 300,       // Max wander distance per axis (units)
    MOVE_BROADCAST_MS: 200      // How often to broadcast position updates (ms)
};

// Initialize wander state for all enemies
function initEnemyWanderState(enemy) {
    enemy.wanderTargetX = enemy.x;
    enemy.wanderTargetY = enemy.y;
    enemy.isWandering = false;
    enemy.nextWanderTime = Date.now() + ENEMY_AI.WANDER_PAUSE_MIN + Math.random() * (ENEMY_AI.WANDER_PAUSE_MAX - ENEMY_AI.WANDER_PAUSE_MIN);
    enemy.lastMoveBroadcast = 0;
}

function pickRandomWanderPoint(enemy) {
    // Random offset from CURRENT position (100-300 units per axis, randomly +/-)
    const range = ENEMY_AI.WANDER_DIST_MAX - ENEMY_AI.WANDER_DIST_MIN;
    const offsetX = (ENEMY_AI.WANDER_DIST_MIN + Math.random() * range) * (Math.random() < 0.5 ? -1 : 1);
    const offsetY = (ENEMY_AI.WANDER_DIST_MIN + Math.random() * range) * (Math.random() < 0.5 ? -1 : 1);
    
    let newX = enemy.x + offsetX;
    let newY = enemy.y + offsetY;
    
    // Clamp to wander radius from spawn point so enemies don't drift infinitely
    const wanderRadius = enemy.wanderRadius || 300;
    const dxFromSpawn = newX - enemy.spawnX;
    const dyFromSpawn = newY - enemy.spawnY;
    const distFromSpawn = Math.sqrt(dxFromSpawn * dxFromSpawn + dyFromSpawn * dyFromSpawn);
    if (distFromSpawn > wanderRadius) {
        // Pull back toward spawn point
        const ratio = wanderRadius / distFromSpawn;
        newX = enemy.spawnX + dxFromSpawn * ratio;
        newY = enemy.spawnY + dyFromSpawn * ratio;
    }
    
    return { x: newX, y: newY };
}

setInterval(() => {
    const now = Date.now();
    
    for (const [enemyId, enemy] of enemies.entries()) {
        // Skip dead enemies or enemies in combat
        if (enemy.isDead || enemy.inCombatWith.size > 0) continue;
        
        // Initialize wander state if missing
        if (enemy.nextWanderTime === undefined) {
            initEnemyWanderState(enemy);
        }
        
        if (!enemy.isWandering) {
            // Waiting to wander — check if it's time
            if (now >= enemy.nextWanderTime) {
                const target = pickRandomWanderPoint(enemy);
                enemy.wanderTargetX = target.x;
                enemy.wanderTargetY = target.y;
                enemy.isWandering = true;
                logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) wandering from (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)}) to (${target.x.toFixed(0)}, ${target.y.toFixed(0)})`);
            }
        } else {
            // Currently wandering — move toward target
            const dx = enemy.wanderTargetX - enemy.x;
            const dy = enemy.wanderTargetY - enemy.y;
            const distance = Math.sqrt(dx * dx + dy * dy);
            
            if (distance < 10) {
                // Reached destination — stop and schedule next wander
                enemy.isWandering = false;
                enemy.nextWanderTime = now + ENEMY_AI.WANDER_PAUSE_MIN + Math.random() * (ENEMY_AI.WANDER_PAUSE_MAX - ENEMY_AI.WANDER_PAUSE_MIN);
                
                // Broadcast final position
                io.emit('enemy:move', {
                    enemyId, x: enemy.x, y: enemy.y, z: enemy.z, isMoving: false
                });
                logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) arrived at (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)})`);
            } else {
                // Move toward target
                const stepSize = ENEMY_AI.WANDER_SPEED * (ENEMY_AI.WANDER_TICK_MS / 1000);
                const moveRatio = Math.min(1, stepSize / distance);
                enemy.x += dx * moveRatio;
                enemy.y += dy * moveRatio;
                
                // Broadcast position at limited rate
                if (now - (enemy.lastMoveBroadcast || 0) >= ENEMY_AI.MOVE_BROADCAST_MS) {
                    enemy.lastMoveBroadcast = now;
                    io.emit('enemy:move', {
                        enemyId,
                        x: enemy.x, y: enemy.y, z: enemy.z,
                        targetX: enemy.wanderTargetX, targetY: enemy.wanderTargetY,
                        isMoving: true
                    });
                    logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) moving — pos (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)}) → target (${enemy.wanderTargetX.toFixed(0)}, ${enemy.wanderTargetY.toFixed(0)})`);
                }
            }
        }
    }
}, ENEMY_AI.WANDER_TICK_MS);

// Middleware
app.use(cors());
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutes
    max: 100, // Limit each IP to 100 requests per windowMs
    message: 'Too many requests from this IP, please try again later.'
});
app.use('/api/', limiter);

// Test mode routes for UI testing (no rate limiting for tests)
app.use('/test', testModeRouter);

// Request logging middleware
app.use((req, res, next) => {
    logger.info(`${req.method} ${req.url} - ${req.ip}`);
    next();
});

// Database connection
const pool = new Pool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
});

// Redis connection
const redisClient = redis.createClient({
    host: process.env.REDIS_HOST || 'localhost',
    port: process.env.REDIS_PORT || 6379
});

redisClient.on('error', (err) => {
    logger.error('Redis error:', err);
});

redisClient.on('connect', () => {
    logger.info('Connected to Redis');
});

// Connect to Redis (required for redis v4+)
redisClient.connect();

// Input validation
function validateRegisterInput(req, res, next) {
    const { username, email, password } = req.body;
    
    // Username validation
    if (!username || username.length < 3 || username.length > 50) {
        return res.status(400).json({ error: 'Username must be between 3 and 50 characters' });
    }
    
    // Email validation
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    if (!email || !emailRegex.test(email)) {
        return res.status(400).json({ error: 'Valid email is required' });
    }
    
    // Password validation
    if (!password || password.length < 8) {
        return res.status(400).json({ error: 'Password must be at least 8 characters long' });
    }
    
    // Password strength check
    if (!/(?=.*[a-zA-Z])(?=.*\d)/.test(password)) {
        return res.status(400).json({ error: 'Password must contain at least one letter and one number' });
    }
    
    next();
}

// JWT middleware
function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1]; // Bearer TOKEN
    
    if (!token) {
        return res.status(401).json({ error: 'Access token required' });
    }
    
    jwt.verify(token, process.env.JWT_SECRET, (err, user) => {
        if (err) {
            return res.status(403).json({ error: 'Invalid or expired token' });
        }
        req.user = user;
        next();
    });
}

// Health check endpoint
app.get('/health', async (req, res) => {
  try {
    logger.debug('Health check requested');
    const result = await pool.query('SELECT NOW()');
    logger.info('Health check passed');
    res.json({ 
      status: 'OK', 
      timestamp: result.rows[0].now,
      message: 'Server is running and connected to database'
    });
  } catch (err) {
    logger.error('Health check failed:', err.message);
    res.status(500).json({ 
      status: 'ERROR', 
      message: 'Database connection failed',
      error: err.message 
    });
  }
});

// Authentication endpoints
app.post('/api/auth/register', validateRegisterInput, async (req, res) => {
    try {
        const { username, email, password } = req.body;
        logger.info(`Registration attempt for username: ${username}`);
        
        // Check if user already exists
        const existingUser = await pool.query(
            'SELECT user_id FROM users WHERE username = $1 OR email = $2',
            [username, email]
        );
        
        if (existingUser.rows.length > 0) {
            logger.warn(`Registration failed: Username or email already exists - ${username}`);
            return res.status(409).json({ error: 'Username or email already exists' });
        }
        
        // Hash password
        const saltRounds = 10;
        const passwordHash = await bcrypt.hash(password, saltRounds);
        
        // Create user
        const result = await pool.query(
            'INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING user_id, username, email, created_at',
            [username, email, passwordHash]
        );
        
        const user = result.rows[0];
        logger.info(`User registered successfully: ${username} (ID: ${user.user_id})`);
        
        // Create JWT token
        const token = jwt.sign(
            { user_id: user.user_id, username: user.username },
            process.env.JWT_SECRET,
            { expiresIn: '24h' }
        );
        
        res.status(201).json({
            message: 'User registered successfully',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email,
                created_at: user.created_at
            },
            token
        });
        
    } catch (err) {
        logger.error('Registration error:', err.message);
        res.status(500).json({ error: 'Registration failed' });
    }
});

app.post('/api/auth/login', async (req, res) => {
    try {
        const { username, password } = req.body;
        logger.info(`Login attempt for username: ${username}`);
        
        // Validate input
        if (!username || !password) {
            logger.warn('Login failed: Missing username or password');
            return res.status(400).json({ error: 'Username and password are required' });
        }
        
        // Find user
        const result = await pool.query(
            'SELECT user_id, username, email, password_hash FROM users WHERE username = $1',
            [username]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Login failed: User not found - ${username}`);
            return res.status(401).json({ error: 'Invalid credentials' });
        }
        
        const user = result.rows[0];
        
        // Verify password
        const validPassword = await bcrypt.compare(password, user.password_hash);
        
        if (!validPassword) {
            logger.warn(`Login failed: Invalid password - ${username}`);
            return res.status(401).json({ error: 'Invalid credentials' });
        }
        
        // Update last_login timestamp
        await pool.query(
            'UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE user_id = $1',
            [user.user_id]
        );
        
        // Create JWT token
        const token = jwt.sign(
            { user_id: user.user_id, username: user.username },
            process.env.JWT_SECRET,
            { expiresIn: '24h' }
        );
        
        logger.info(`Login successful: ${username} (ID: ${user.user_id})`);
        
        res.json({
            message: 'Login successful',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email
            },
            token
        });
        
    } catch (err) {
        logger.error('Login error:', err.message);
        res.status(500).json({ error: 'Login failed' });
    }
});

app.get('/api/auth/verify', authenticateToken, async (req, res) => {
    try {
        logger.debug(`Token verification for user ID: ${req.user.user_id}`);
        // Token is valid, return user info
        const result = await pool.query(
            'SELECT user_id, username, email, created_at FROM users WHERE user_id = $1',
            [req.user.user_id]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Token verification failed: User not found - ${req.user.user_id}`);
            return res.status(404).json({ error: 'User not found' });
        }
        
        const user = result.rows[0];
        logger.debug(`Token verified for user: ${user.username}`);
        
        res.json({
            message: 'Token is valid',
            user: {
                user_id: user.user_id,
                username: user.username,
                email: user.email,
                created_at: user.created_at
            }
        });
        
    } catch (err) {
        logger.error('Token verification error:', err.message);
        res.status(500).json({ error: 'Token verification failed' });
    }
});

// Character endpoints
app.get('/api/characters', authenticateToken, async (req, res) => {
    try {
        logger.debug(`Fetching characters for user ID: ${req.user.user_id}`);
        const result = await pool.query(
            'SELECT character_id, name, class, level, x, y, z, health, mana, created_at FROM characters WHERE user_id = $1 ORDER BY created_at DESC',
            [req.user.user_id]
        );
        
        logger.info(`Retrieved ${result.rows.length} characters for user ${req.user.user_id}`);
        res.json({
            message: 'Characters retrieved successfully',
            characters: result.rows
        });
    } catch (err) {
        logger.error('Get characters error:', err.message);
        res.status(500).json({ error: 'Failed to retrieve characters' });
    }
});

app.post('/api/characters', authenticateToken, async (req, res) => {
    try {
        const { name, characterClass } = req.body;
        logger.info(`Character creation attempt: ${name} (class: ${characterClass}) for user ${req.user.user_id}`);
        
        // Validation
        if (!name || name.length < 2 || name.length > 50) {
            logger.warn(`Character creation failed: Invalid name length - ${name}`);
            return res.status(400).json({ error: 'Character name must be between 2 and 50 characters' });
        }
        
        // Check if character name already exists for this user
        const existingChar = await pool.query(
            'SELECT character_id FROM characters WHERE user_id = $1 AND name = $2',
            [req.user.user_id, name]
        );
        
        if (existingChar.rows.length > 0) {
            logger.warn(`Character creation failed: Name already exists - ${name}`);
            return res.status(409).json({ error: 'You already have a character with this name' });
        }
        
        // Validate class (optional field)
        const validClasses = ['warrior', 'mage', 'archer', 'healer', 'priest'];
        const charClass = characterClass && validClasses.includes(characterClass.toLowerCase()) 
            ? characterClass.toLowerCase() 
            : 'warrior';
        
        // Create character (all new characters start as Novice with RO-style defaults)
        const result = await pool.query(
            `INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana, job_level, base_exp, job_exp, job_class, skill_points, stat_points) 
             VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15) 
             RETURNING character_id, name, class, level, x, y, z, health, mana, job_level, job_class, created_at`,
            [req.user.user_id, name, charClass, 1, 0, 0, 0, 100, 100, 1, 0, 0, 'novice', 0, 48]
        );
        
        const character = result.rows[0];
        logger.info(`Character created successfully: ${name} (ID: ${character.character_id})`);
        
        res.status(201).json({
            message: 'Character created successfully',
            character
        });
        
    } catch (err) {
        logger.error('Create character error:', err.message);
        res.status(500).json({ error: 'Failed to create character' });
    }
});

app.get('/api/characters/:id', authenticateToken, async (req, res) => {
    try {
        const characterId = req.params.id;
        logger.debug(`Fetching character ${characterId} for user ${req.user.user_id}`);
        
        const result = await pool.query(
            'SELECT character_id, name, class, level, x, y, z, health, mana, created_at FROM characters WHERE character_id = $1 AND user_id = $2',
            [characterId, req.user.user_id]
        );
        
        if (result.rows.length === 0) {
            logger.warn(`Character not found: ${characterId} for user ${req.user.user_id}`);
            return res.status(404).json({ error: 'Character not found' });
        }
        
        logger.debug(`Character retrieved: ${result.rows[0].name}`);
        res.json({
            message: 'Character retrieved successfully',
            character: result.rows[0]
        });
        
    } catch (err) {
        logger.error('Get character error:', err.message);
        res.status(500).json({ error: 'Failed to retrieve character' });
    }
});

// Save character position
app.put('/api/characters/:id/position', authenticateToken, async (req, res) => {
    try {
        const characterId = req.params.id;
        const { x, y, z } = req.body;
        logger.debug(`Saving position for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
        
        // Validate coordinates
        if (typeof x !== 'number' || typeof y !== 'number' || typeof z !== 'number') {
            logger.warn(`Invalid coordinates for character ${characterId}: x=${x}, y=${y}, z=${z}`);
            return res.status(400).json({ error: 'Invalid coordinates. x, y, z must be numbers' });
        }
        
        // Verify character belongs to user
        const charCheck = await pool.query(
            'SELECT character_id FROM characters WHERE character_id = $1 AND user_id = $2',
            [characterId, req.user.user_id]
        );
        
        if (charCheck.rows.length === 0) {
            logger.warn(`Position save failed: Character not found - ${characterId}`);
            return res.status(404).json({ error: 'Character not found' });
        }
        
        // Update position
        await pool.query(
            'UPDATE characters SET x = $1, y = $2, z = $3 WHERE character_id = $4',
            [x, y, z, characterId]
        );
        
        logger.info(`Position saved for character ${characterId}: X=${x}, Y=${y}, Z=${z}`);
        res.json({
            message: 'Position saved successfully',
            position: { x, y, z }
        });
        
    } catch (err) {
        logger.error('Save position error:', err.message);
        res.status(500).json({ error: 'Failed to save position' });
    }
});

// Test endpoint - will be replaced with auth later
app.get('/api/test', (req, res) => {
  logger.debug('Test endpoint accessed');
  res.json({ message: 'Hello from MMO Server!' });
});

// Redis helper functions for player position cache
async function setPlayerPosition(characterId, x, y, z, zone = 'default') {
    const key = `player:${characterId}:position`;
    const position = JSON.stringify({ x, y, z, zone, timestamp: Date.now() });
    await redisClient.setEx(key, 300, position); // Expire after 5 minutes
    logger.debug(`Cached position for character ${characterId}`);
}

async function getPlayerPosition(characterId) {
    const key = `player:${characterId}:position`;
    const data = await redisClient.get(key);
    return data ? JSON.parse(data) : null;
}

async function removePlayerPosition(characterId) {
    const key = `player:${characterId}:position`;
    await redisClient.del(key);
    logger.debug(`Removed position cache for character ${characterId}`);
}

async function getPlayersInZone(zone = 'default') {
    const keys = await redisClient.keys('player:*:position');
    const players = [];
    for (const key of keys) {
        const data = await redisClient.get(key);
        if (data) {
            const pos = JSON.parse(data);
            if (pos.zone === zone) {
                const characterId = key.split(':')[1];
                players.push({ characterId: parseInt(characterId), ...pos });
            }
        }
    }
    return players;
}

// Start server
server.listen(PORT, async () => {
  logger.info(`MMO Server running on port ${PORT}`);
  logger.info(`Socket.io ready`);
  logger.info(`Database: ${process.env.DB_NAME}`);
  logger.info(`Log level: ${LOG_LEVEL}`);
  
  // Test Redis connection
  try {
    await redisClient.ping();
    logger.info('Redis: Connected');
  } catch (err) {
    logger.error('Redis: Connection failed -', err.message);
  }
  
  // Ensure stat columns exist in characters table
  try {
    await pool.query(`
      ALTER TABLE characters 
      ADD COLUMN IF NOT EXISTS str INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS agi INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS vit INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS int_stat INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS dex INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS luk INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS stat_points INTEGER DEFAULT 48,
      ADD COLUMN IF NOT EXISTS max_health INTEGER DEFAULT 100,
      ADD COLUMN IF NOT EXISTS max_mana INTEGER DEFAULT 100,
      ADD COLUMN IF NOT EXISTS zuzucoin INTEGER DEFAULT 0
    `);
    logger.info('[DB] Stat columns verified/created on characters table');
  } catch (err) {
    logger.warn(`[DB] Could not add stat columns (may already exist): ${err.message}`);
  }
  
  // Ensure items table exists
  try {
    await pool.query(`
      CREATE TABLE IF NOT EXISTS items (
        item_id SERIAL PRIMARY KEY,
        name VARCHAR(100) NOT NULL,
        description TEXT DEFAULT '',
        item_type VARCHAR(20) NOT NULL DEFAULT 'etc',
        equip_slot VARCHAR(20) DEFAULT NULL,
        weight INTEGER DEFAULT 0,
        price INTEGER DEFAULT 0,
        atk INTEGER DEFAULT 0,
        def INTEGER DEFAULT 0,
        matk INTEGER DEFAULT 0,
        mdef INTEGER DEFAULT 0,
        str_bonus INTEGER DEFAULT 0,
        agi_bonus INTEGER DEFAULT 0,
        vit_bonus INTEGER DEFAULT 0,
        int_bonus INTEGER DEFAULT 0,
        dex_bonus INTEGER DEFAULT 0,
        luk_bonus INTEGER DEFAULT 0,
        max_hp_bonus INTEGER DEFAULT 0,
        max_sp_bonus INTEGER DEFAULT 0,
        required_level INTEGER DEFAULT 1,
        stackable BOOLEAN DEFAULT false,
        max_stack INTEGER DEFAULT 1,
        icon VARCHAR(100) DEFAULT 'default_item',
        weapon_type VARCHAR(20) DEFAULT NULL,
        aspd_modifier INTEGER DEFAULT 0,
        weapon_range INTEGER DEFAULT 0
      )
    `);
    logger.info('[DB] Items table verified/created');
  } catch (err) {
    logger.warn(`[DB] Items table issue: ${err.message}`);
  }
  
  // Add weapon_type columns if missing (migration for existing DB)
  try {
    await pool.query(`
      ALTER TABLE items
      ADD COLUMN IF NOT EXISTS weapon_type VARCHAR(20) DEFAULT NULL,
      ADD COLUMN IF NOT EXISTS aspd_modifier INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS weapon_range INTEGER DEFAULT 0
    `);
    logger.info('[DB] Weapon type columns verified on items table');
  } catch (err) {
    logger.warn(`[DB] Weapon type columns issue: ${err.message}`);
  }

  // Add EXP & leveling columns to characters table (RO-style dual progression)
  try {
    await pool.query(`
      ALTER TABLE characters
      ADD COLUMN IF NOT EXISTS job_level INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS base_exp BIGINT DEFAULT 0,
      ADD COLUMN IF NOT EXISTS job_exp BIGINT DEFAULT 0,
      ADD COLUMN IF NOT EXISTS job_class VARCHAR(30) DEFAULT 'novice',
      ADD COLUMN IF NOT EXISTS skill_points INTEGER DEFAULT 0
    `);
    await pool.query('CREATE INDEX IF NOT EXISTS idx_characters_job_class ON characters(job_class)');
    logger.info('[DB] EXP & leveling columns verified on characters table');
  } catch (err) {
    logger.warn(`[DB] EXP columns issue: ${err.message}`);
  }

  // Add derived stat bonus columns to items table (migration for existing DB)
  try {
    await pool.query(`
      ALTER TABLE items
      ADD COLUMN IF NOT EXISTS hit_bonus INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS flee_bonus INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS critical_bonus INTEGER DEFAULT 0
    `);
    logger.info('[DB] Derived stat bonus columns verified on items table');
  } catch (err) {
    logger.warn(`[DB] Derived stat bonus columns issue: ${err.message}`);
  }
  
  // === Skill System Tables (RO Classic) ===
  try {
    await pool.query(`
      CREATE TABLE IF NOT EXISTS skills (
        skill_id INTEGER PRIMARY KEY,
        internal_name VARCHAR(100) NOT NULL UNIQUE,
        display_name VARCHAR(100) NOT NULL,
        class_id VARCHAR(30) NOT NULL,
        max_level INTEGER NOT NULL DEFAULT 1,
        skill_type VARCHAR(20) NOT NULL DEFAULT 'active',
        target_type VARCHAR(20) NOT NULL DEFAULT 'none',
        element VARCHAR(20) NOT NULL DEFAULT 'neutral',
        skill_range INTEGER NOT NULL DEFAULT 0,
        description TEXT DEFAULT '',
        icon VARCHAR(100) DEFAULT 'default_skill',
        tree_row INTEGER DEFAULT 0,
        tree_col INTEGER DEFAULT 0
      )
    `);
    await pool.query(`
      CREATE TABLE IF NOT EXISTS skill_prerequisites (
        skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
        required_skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
        required_level INTEGER NOT NULL DEFAULT 1,
        PRIMARY KEY (skill_id, required_skill_id)
      )
    `);
    await pool.query(`
      CREATE TABLE IF NOT EXISTS skill_levels (
        skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
        level INTEGER NOT NULL,
        sp_cost INTEGER NOT NULL DEFAULT 0,
        cast_time_ms INTEGER NOT NULL DEFAULT 0,
        cooldown_ms INTEGER NOT NULL DEFAULT 0,
        effect_value INTEGER DEFAULT 0,
        duration_ms INTEGER DEFAULT 0,
        description TEXT DEFAULT '',
        PRIMARY KEY (skill_id, level)
      )
    `);
    await pool.query(`
      CREATE TABLE IF NOT EXISTS character_skills (
        character_id INTEGER NOT NULL REFERENCES characters(character_id) ON DELETE CASCADE,
        skill_id INTEGER NOT NULL REFERENCES skills(skill_id) ON DELETE CASCADE,
        level INTEGER NOT NULL DEFAULT 1,
        PRIMARY KEY (character_id, skill_id)
      )
    `);
    await pool.query('CREATE INDEX IF NOT EXISTS idx_skills_class ON skills(class_id)');
    await pool.query('CREATE INDEX IF NOT EXISTS idx_char_skills_character ON character_skills(character_id)');
    logger.info('[DB] Skill system tables verified/created');
  } catch (err) {
    logger.warn(`[DB] Skill tables issue: ${err.message}`);
  }

  // Populate skill definitions from ro_skill_data.js into DB (upsert)
  try {
    let skillsInserted = 0;
    for (const skill of ALL_SKILLS) {
      await pool.query(`
        INSERT INTO skills (skill_id, internal_name, display_name, class_id, max_level, skill_type, target_type, element, skill_range, description, icon, tree_row, tree_col)
        VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13)
        ON CONFLICT (skill_id) DO UPDATE SET display_name = $3, class_id = $4, max_level = $5, skill_type = $6, target_type = $7, element = $8, skill_range = $9, description = $10, icon = $11, tree_row = $12, tree_col = $13
      `, [skill.id, skill.name, skill.displayName, skill.classId, skill.maxLevel, skill.type, skill.targetType, skill.element, skill.range, skill.description, skill.icon, skill.treeRow, skill.treeCol]);
      skillsInserted++;

      // Upsert prerequisites
      for (const prereq of skill.prerequisites) {
        await pool.query(`
          INSERT INTO skill_prerequisites (skill_id, required_skill_id, required_level)
          VALUES ($1, $2, $3)
          ON CONFLICT (skill_id, required_skill_id) DO UPDATE SET required_level = $3
        `, [skill.id, prereq.skillId, prereq.level]);
      }

      // Upsert skill level data
      for (const lvl of skill.levels) {
        await pool.query(`
          INSERT INTO skill_levels (skill_id, level, sp_cost, cast_time_ms, cooldown_ms, effect_value, duration_ms)
          VALUES ($1, $2, $3, $4, $5, $6, $7)
          ON CONFLICT (skill_id, level) DO UPDATE SET sp_cost = $3, cast_time_ms = $4, cooldown_ms = $5, effect_value = $6, duration_ms = $7
        `, [skill.id, lvl.level, lvl.spCost, lvl.castTime, lvl.cooldown, lvl.effectValue, lvl.duration]);
      }
    }
    logger.info(`[DB] Skill definitions synced: ${skillsInserted} skills with prerequisites and level data`);
  } catch (err) {
    logger.warn(`[DB] Skill data sync issue: ${err.message}`);
  }

  // Add skill columns to character_hotbar (support skills on hotbar alongside items)
  try {
    await pool.query(`
      ALTER TABLE character_hotbar
      ADD COLUMN IF NOT EXISTS slot_type VARCHAR(10) DEFAULT 'item',
      ADD COLUMN IF NOT EXISTS skill_id INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS skill_name VARCHAR(100) DEFAULT ''
    `);
    logger.info('[DB] Hotbar skill columns verified on character_hotbar table');
  } catch (err) {
    logger.warn(`[DB] Hotbar skill columns issue: ${err.message}`);
  }

  // Make inventory_id and item_id nullable for skill-only hotbar slots
  try {
    await pool.query(`ALTER TABLE character_hotbar ALTER COLUMN inventory_id DROP NOT NULL`);
    await pool.query(`ALTER TABLE character_hotbar ALTER COLUMN item_id DROP NOT NULL`);
    logger.info('[DB] character_hotbar: inventory_id and item_id are now nullable (skill slots)');
  } catch (err) {
    // Columns may already be nullable — safe to ignore
    if (!err.message.includes('already')) {
      logger.warn(`[DB] Nullable migration note: ${err.message}`);
    }
  }

  // Add row_index column for multi-row hotbar support (4 rows of 9 slots)
  try {
    await pool.query(`ALTER TABLE character_hotbar ADD COLUMN IF NOT EXISTS row_index INTEGER NOT NULL DEFAULT 0`);
    logger.info('[DB] character_hotbar: row_index column verified');
    // Try to update PK to include row_index (safe — will fail if already done)
    try {
      await pool.query(`ALTER TABLE character_hotbar DROP CONSTRAINT character_hotbar_pkey`);
      await pool.query(`ALTER TABLE character_hotbar ADD PRIMARY KEY (character_id, row_index, slot_index)`);
      logger.info('[DB] character_hotbar: primary key updated to include row_index');
    } catch (pkErr) {
      // PK already includes row_index or other constraint issue — safe to ignore
      logger.info(`[DB] character_hotbar PK note: ${pkErr.message}`);
    }
    // Normalize existing skill slots from 1-based to 0-based (one-time migration)
    try {
      const result = await pool.query(
        `UPDATE character_hotbar SET slot_index = slot_index - 1 WHERE slot_type = 'skill' AND slot_index >= 1`
      );
      if (result.rowCount > 0) {
        logger.info(`[DB] character_hotbar: normalized ${result.rowCount} skill slots from 1-based to 0-based`);
      }
    } catch (normErr) {
      logger.warn(`[DB] Hotbar slot normalization note: ${normErr.message}`);
    }
  } catch (err) {
    logger.warn(`[DB] Hotbar row_index migration note: ${err.message}`);
  }

  // Ensure character_inventory table exists
  try {
    await pool.query(`
      CREATE TABLE IF NOT EXISTS character_inventory (
        inventory_id SERIAL PRIMARY KEY,
        character_id INTEGER REFERENCES characters(character_id) ON DELETE CASCADE,
        item_id INTEGER REFERENCES items(item_id),
        quantity INTEGER DEFAULT 1,
        is_equipped BOOLEAN DEFAULT false,
        slot_index INTEGER DEFAULT -1,
        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
      )
    `);
    await pool.query('CREATE INDEX IF NOT EXISTS idx_inventory_character ON character_inventory(character_id)');
    await pool.query('CREATE INDEX IF NOT EXISTS idx_inventory_item ON character_inventory(item_id)');
    logger.info('[DB] Character inventory table verified/created');
  } catch (err) {
    logger.warn(`[DB] Inventory table issue: ${err.message}`);
  }
  
  // Seed base items if items table is empty
  try {
    const itemCount = await pool.query('SELECT COUNT(*) FROM items');
    if (parseInt(itemCount.rows[0].count) === 0) {
      logger.info('[DB] Seeding base items...');
      await pool.query(`
        INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, max_stack, icon) VALUES
        (1001, 'Crimson Vial', 'A small red tonic. Restores 50 HP.', 'consumable', 7, 25, true, 99, 'red_potion'),
        (1002, 'Amber Elixir', 'A warm orange draught. Restores 150 HP.', 'consumable', 10, 100, true, 99, 'orange_potion'),
        (1003, 'Golden Salve', 'A potent yellow remedy. Restores 350 HP.', 'consumable', 13, 275, true, 99, 'yellow_potion'),
        (1004, 'Azure Philter', 'A calming blue brew. Restores 60 SP.', 'consumable', 15, 500, true, 99, 'blue_potion'),
        (1005, 'Roasted Haunch', 'Tender roasted meat. Restores 70 HP.', 'consumable', 15, 25, true, 99, 'meat')
        ON CONFLICT (item_id) DO NOTHING
      `);
      await pool.query(`
        INSERT INTO items (item_id, name, description, item_type, weight, price, stackable, max_stack, icon) VALUES
        (2001, 'Gloopy Residue', 'A small, squishy blob of unknown origin.', 'etc', 1, 3, true, 999, 'jellopy'),
        (2002, 'Viscous Slime', 'Thick, gooey substance secreted by monsters.', 'etc', 1, 7, true, 999, 'sticky_mucus'),
        (2003, 'Chitin Shard', 'A hard, protective outer shell fragment.', 'etc', 2, 14, true, 999, 'shell'),
        (2004, 'Downy Plume', 'A light, downy feather.', 'etc', 1, 5, true, 999, 'feather'),
        (2005, 'Spore Cluster', 'Tiny spores from a forest mushroom.', 'etc', 1, 10, true, 999, 'mushroom_spore'),
        (2006, 'Barbed Limb', 'A chitinous leg from a large insect.', 'etc', 1, 12, true, 999, 'insect_leg'),
        (2007, 'Verdant Leaf', 'A common medicinal herb with healing properties.', 'etc', 3, 8, true, 99, 'green_herb'),
        (2008, 'Silken Tuft', 'Soft, fluffy material from a plant creature.', 'etc', 1, 4, true, 999, 'fluff')
        ON CONFLICT (item_id) DO NOTHING
      `);
      await pool.query(`
        INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, atk, required_level, icon, weapon_type, aspd_modifier, weapon_range) VALUES
        (3001, 'Rustic Shiv', 'A crude but swift dagger. Better than bare fists.', 'weapon', 'weapon', 40, 50, 17, 1, 'knife', 'dagger', 5, 150),
        (3002, 'Keen Edge', 'A finely honed short blade.', 'weapon', 'weapon', 40, 150, 30, 1, 'cutter', 'dagger', 5, 150),
        (3003, 'Stiletto Fang', 'An elegant parrying dagger with a needle-thin tip.', 'weapon', 'weapon', 60, 500, 43, 12, 'main_gauche', 'dagger', 5, 150),
        (3004, 'Iron Cleaver', 'A standard one-handed sword with a broad blade.', 'weapon', 'weapon', 80, 100, 25, 2, 'sword', 'one_hand_sword', 0, 150),
        (3005, 'Crescent Saber', 'A curved, heavy cutting sword.', 'weapon', 'weapon', 60, 600, 49, 18, 'falchion', 'one_hand_sword', 0, 150),
        (3006, 'Hunting Longbow', 'A sturdy ranged bow with impressive reach.', 'weapon', 'weapon', 50, 400, 35, 4, 'bow', 'bow', -3, 800)
        ON CONFLICT (item_id) DO NOTHING
      `);
      await pool.query(`
        INSERT INTO items (item_id, name, description, item_type, equip_slot, weight, price, def, required_level, icon) VALUES
        (4001, 'Linen Tunic', 'A simple linen shirt offering minimal protection.', 'armor', 'armor', 10, 20, 1, 1, 'cotton_shirt'),
        (4002, 'Quilted Vest', 'Light quilted padding for basic protection.', 'armor', 'armor', 80, 200, 4, 1, 'padded_armor'),
        (4003, 'Ringweave Hauberk', 'Interlocking metal rings forged into sturdy armor.', 'armor', 'armor', 150, 800, 8, 20, 'chain_mail')
        ON CONFLICT (item_id) DO NOTHING
      `);
      logger.info('[DB] Base items seeded successfully');
    }
  } catch (err) {
    logger.warn(`[DB] Item seeding issue: ${err.message}`);
  }
  
  // Add equipped_position column if missing (for dual-accessory support)
  try {
    await pool.query(`
      ALTER TABLE character_inventory
      ADD COLUMN IF NOT EXISTS equipped_position VARCHAR(20) DEFAULT NULL
    `);
    logger.info('[DB] equipped_position column verified on character_inventory');
  } catch (err) {
    logger.warn(`[DB] equipped_position column issue: ${err.message}`);
  }

  // Fix existing items with slot_index = -1 (assign unique sequential indexes per character)
  try {
    const charsWithBadSlots = await pool.query(
      'SELECT DISTINCT character_id FROM character_inventory WHERE slot_index < 0'
    );
    for (const row of charsWithBadSlots.rows) {
      const charId = row.character_id;
      const items = await pool.query(
        'SELECT inventory_id FROM character_inventory WHERE character_id = $1 ORDER BY created_at ASC',
        [charId]
      );
      for (let i = 0; i < items.rows.length; i++) {
        await pool.query(
          'UPDATE character_inventory SET slot_index = $1 WHERE inventory_id = $2',
          [i, items.rows[i].inventory_id]
        );
      }
      logger.info(`[DB] Fixed slot_index for character ${charId}: assigned ${items.rows.length} items`);
    }
  } catch (err) {
    logger.warn(`[DB] slot_index fix issue: ${err.message}`);
  }

  // Load item definitions into memory cache
  await loadItemDefinitions();
  
  // Spawn initial enemies
  logger.info(`[ENEMY] Spawning ${ENEMY_SPAWNS.length} enemies...`);
  for (const spawnConfig of ENEMY_SPAWNS) {
    spawnEnemy(spawnConfig);
  }
  logger.info(`[ENEMY] ${enemies.size} enemies spawned. IDs: ${Array.from(enemies.keys()).join(', ')}`);
});

// Periodic health/mana + EXP save to DB (every 60 seconds)
setInterval(async () => {
  for (const [charId, player] of connectedPlayers.entries()) {
    if (!player.isDead) {
      await savePlayerHealthToDB(charId, player.health, player.mana);
      await saveExpDataToDB(charId, player);
    }
  }
}, 60000);
