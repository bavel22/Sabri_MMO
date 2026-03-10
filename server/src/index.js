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
// Import RO monster AI codes (rAthena pre-renewal — maps monster ID → AI type code)
const { MONSTER_AI_CODES } = require('./ro_monster_ai_codes');
// Import RO zone registry (map definitions, warps, spawns, Kafra NPCs)
const { ZONE_REGISTRY, getZone, getAllEnemySpawns, getZoneNames } = require('./ro_zone_data');
// Import RO pre-renewal damage formulas (element table, size penalty, HIT/FLEE, critical, DEF)
const {
    ELEMENT_TABLE, SIZE_PENALTY,
    calculateDerivedStats: roDerivedStats,
    getElementModifier, getSizePenalty,
    calculateHitRate, calculateCritRate,
    calculatePhysicalDamage: roPhysicalDamage,
    calculateMagicalDamage: roMagicalDamage
} = require('./ro_damage_formulas');
// Import generic status effect engine (Phase 2)
const {
    STATUS_EFFECTS, BREAKABLE_STATUSES,
    applyStatusEffect, forceApplyStatusEffect, removeStatusEffect,
    cleanse: cleanseStatusEffects, checkDamageBreakStatuses,
    tickStatusEffects, getStatusModifiers,
    hasStatusEffect, getActiveStatusList
} = require('./ro_status_effects');
// Import generic buff system (Phase 2)
const {
    BUFF_TYPES,
    applyBuff: applyBuffGeneric, removeBuff, hasBuff,
    getBuffModifiers, getActiveBuffList
} = require('./ro_buff_system');
// Re-export expireBuffs from buff system (used in tick loop)
const { expireBuffs: expireBuffsGeneric } = require('./ro_buff_system');

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

// ============================================================
// RO Monster Mode Flags (bitmask) — from rAthena doc/mob_db_mode_list.txt
// Combined with AI type codes to determine per-monster behavior
// ============================================================
const MD = {
    CANMOVE:            0x0001,
    LOOTER:             0x0002,
    AGGRESSIVE:         0x0004,
    ASSIST:             0x0008,
    CASTSENSORIDLE:     0x0010,
    NORANDOMWALK:       0x0020,
    NOCAST:             0x0040,
    CANATTACK:          0x0080,
    CASTSENSORCHASE:    0x0200,
    CHANGECHASE:        0x0400,
    ANGRY:              0x0800,
    CHANGETARGETMELEE:  0x1000,
    CHANGETARGETCHASE:  0x2000,
    TARGETWEAK:         0x4000,
    RANDOMTARGET:       0x8000,
    MVP:                0x80000,
    KNOCKBACKIMMUNE:    0x200000,
    DETECTOR:           0x2000000,
    STATUSIMMUNE:       0x4000000,
};

// rAthena AI Type → hex mode bitmask (from rAthena Issue #926)
const AI_TYPE_MODES = {
    1:  0x0081,  // Passive: CanMove + CanAttack
    2:  0x0083,  // Passive + Looter
    3:  0x1089,  // Passive + Assist + ChangeTargetMelee + CanAttack
    4:  0x3885,  // Angry: Aggressive + Assist + ChangeTargetMelee/Chase + Angry
    5:  0x2085,  // Aggressive + ChangeTargetChase + CanAttack
    6:  0x0000,  // Plant/Immobile: no flags
    7:  0x108B,  // Passive + Looter + Assist + ChangeTargetMelee + CanAttack
    8:  0x7085,  // Aggressive + ChangeTarget(All) + TargetWeak + CanAttack
    9:  0x3095,  // Aggressive + ChangeTarget(Melee/Chase) + CastSensorIdle + CanAttack
    10: 0x0084,  // Aggressive + Immobile (no CanMove)
    11: 0x0084,  // Guardian: Aggressive + Immobile
    12: 0x2085,  // Guardian: Aggressive + ChangeTargetChase
    13: 0x308D,  // Aggressive + Assist + ChangeTarget(Melee/Chase) + CanAttack
    17: 0x0091,  // Passive + CastSensorIdle + CanAttack
    19: 0x3095,  // Aggressive + ChangeTarget(Melee/Chase) + CastSensorIdle
    20: 0x3295,  // Aggressive + ChangeTarget(Melee/Chase) + CastSensor(Idle/Chase)
    21: 0x3695,  // Like 20 + ChangeChase
    24: 0x00A1,  // Slave: Passive + NoRandomWalk + CanAttack
    25: 0x0001,  // Pet: CanMove only (no CanAttack)
    26: 0xB695,  // Aggressive + all ChangeTarget + CastSensor + RandomTarget
    27: 0x8084,  // Aggressive + Immobile + RandomTarget
};

// Enemy AI States (server-side state machine)
const AI_STATE = {
    IDLE:    'idle',     // Wandering or standing (scans for aggro if aggressive)
    CHASE:   'chase',    // Moving toward target player
    ATTACK:  'attack',   // In attack range, auto-attacking target
    DEAD:    'dead',     // Dead, awaiting respawn
};

// Parse hex mode bitmask into boolean flags for fast runtime checks
function parseModeFlags(hexMode) {
    return {
        canMove:            !!(hexMode & MD.CANMOVE),
        looter:             !!(hexMode & MD.LOOTER),
        aggressive:         !!(hexMode & MD.AGGRESSIVE),
        assist:             !!(hexMode & MD.ASSIST),
        castSensorIdle:     !!(hexMode & MD.CASTSENSORIDLE),
        noRandomWalk:       !!(hexMode & MD.NORANDOMWALK),
        canAttack:          !!(hexMode & MD.CANATTACK),
        castSensorChase:    !!(hexMode & MD.CASTSENSORCHASE),
        changeChase:        !!(hexMode & MD.CHANGECHASE),
        angry:              !!(hexMode & MD.ANGRY),
        changeTargetMelee:  !!(hexMode & MD.CHANGETARGETMELEE),
        changeTargetChase:  !!(hexMode & MD.CHANGETARGETCHASE),
        targetWeak:         !!(hexMode & MD.TARGETWEAK),
        randomTarget:       !!(hexMode & MD.RANDOMTARGET),
        mvp:                !!(hexMode & MD.MVP),
        knockbackImmune:    !!(hexMode & MD.KNOCKBACKIMMUNE),
        detector:           !!(hexMode & MD.DETECTOR),
        statusImmune:       !!(hexMode & MD.STATUSIMMUNE),
    };
}

// Get default AI code from simplified aiType string
function getDefaultAiCode(aiType) {
    switch (aiType) {
        case 'passive':    return 1;
        case 'aggressive': return 5;
        case 'reactive':   return 3;
        default:           return 1;
    }
}

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
    const castPlayer = connectedPlayers.get(characterId);
    const castZone = (castPlayer && castPlayer.zone) || 'prontera_south';
    broadcastToZone(castZone, 'skill:cast_interrupted_broadcast', { casterId: characterId, skillId: cast.skillId });
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
// Delegates to the generic buff system module
function applyBuff(target, buffDef) {
    return applyBuffGeneric(target, buffDef);
}

// Remove expired buffs, returns array of expired buff objects
function expireBuffs(target) {
    return expireBuffsGeneric(target);
}

// Get combined stat modifiers from both status effects AND buffs
// This replaces the old getBuffStatModifiers() — merges status + buff mods
function getCombinedModifiers(target) {
    const statusMods = getStatusModifiers(target);
    const buffMods = getBuffModifiers(target);
    return {
        // Merge multipliers (multiplicative)
        defMultiplier: statusMods.defMultiplier * buffMods.defMultiplier,
        atkMultiplier: statusMods.atkMultiplier * buffMods.atkMultiplier,
        mdefMultiplier: statusMods.mdefMultiplier || 1.0,
        hitMultiplier: statusMods.hitMultiplier || 1.0,
        fleeMultiplier: statusMods.fleeMultiplier || 1.0,
        moveSpeedMultiplier: statusMods.moveSpeedMultiplier || 1.0,
        aspdMultiplier: buffMods.aspdMultiplier || 1.0,
        // Additive bonuses (from buffs)
        bonusMDEF: buffMods.bonusMDEF || 0,
        strBonus: buffMods.strBonus || 0,
        agiBonus: buffMods.agiBonus || 0,
        vitBonus: buffMods.vitBonus || 0,
        intBonus: buffMods.intBonus || 0,
        dexBonus: buffMods.dexBonus || 0,
        lukBonus: buffMods.lukBonus || 0,
        defPercent: buffMods.defPercent || 0,
        moveSpeedBonus: buffMods.moveSpeedBonus || 0,
        bonusHit: buffMods.bonusHit || 0,
        bonusFlee: buffMods.bonusFlee || 0,
        bonusCritical: buffMods.bonusCritical || 0,
        // Overrides
        lukOverride: statusMods.lukOverride,
        overrideElement: statusMods.overrideElement,
        weaponElement: buffMods.weaponElement || null,
        // Prevention flags (from status effects)
        preventsMovement: statusMods.preventsMovement || false,
        preventsCasting: statusMods.preventsCasting || false,
        preventsAttack: statusMods.preventsAttack || false,
        preventsItems: statusMods.preventsItems || false,
        blocksHPRegen: statusMods.blocksHPRegen || false,
        blocksSPRegen: statusMods.blocksSPRegen || false,
        // Special buff flags
        isHidden: buffMods.isHidden || false,
        doubleNextDamage: buffMods.doubleNextDamage || false,
        blockRanged: buffMods.blockRanged || false,
        // Individual status flags (backward compat)
        isFrozen: statusMods.isFrozen || false,
        isStoned: statusMods.isStoned || false,
        isStunned: statusMods.isStunned || false,
        isSleeping: statusMods.isSleeping || false,
        isPoisoned: statusMods.isPoisoned || false,
        isBleeding: statusMods.isBleeding || false,
        isCursed: statusMods.isCursed || false,
        isBlind: statusMods.isBlind || false,
        isSilenced: statusMods.isSilenced || false,
        isConfused: statusMods.isConfused || false
    };
}

// Backward-compatible alias — old code calls getBuffStatModifiers, new code uses getCombinedModifiers
function getBuffStatModifiers(target) {
    return getCombinedModifiers(target);
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
    enemy.aiState = AI_STATE.DEAD;
    enemy.targetPlayerId = null;
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
            const attackerZone = attacker.zone || 'prontera_south';
            broadcastToZoneExcept(killerSocket, attackerZone, 'exp:level_up', levelUpPayload);

            const effectiveStats = getEffectiveStats(attacker);
            const newDerived = calculateDerivedStats(effectiveStats);
            const newFinalAspd = Math.min(COMBAT.ASPD_CAP, newDerived.aspd + (attacker.weaponAspdMod || 0));
            killerSocket.emit('player:stats', buildFullStatsPayload(attackerId, attacker, effectiveStats, newDerived, newFinalAspd));
            broadcastToZone(attackerZone, 'combat:health_update', {
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

    // Broadcast enemy death to zone
    const enemyZone = enemy.zone || 'prontera_south';
    broadcastToZone(enemyZone, 'enemy:death', {
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

    // Schedule respawn (only if zone is still active)
    setTimeout(() => {
        enemy.health = enemy.maxHealth;
        enemy.isDead = false;
        enemy.x = enemy.spawnX; enemy.y = enemy.spawnY; enemy.z = enemy.spawnZ;
        enemy.targetPlayerId = null;
        enemy.inCombatWith = new Set();
        if (typeof initEnemyWanderState === 'function') initEnemyWanderState(enemy);
        broadcastToZone(enemy.zone, 'enemy:spawn', {
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
    // Compute RO Classic mode flags from AI code lookup
    const roId = template.id || 0;
    const aiCode = (MONSTER_AI_CODES && MONSTER_AI_CODES[roId]) || getDefaultAiCode(template.aiType);
    const hexMode = AI_TYPE_MODES[aiCode] || 0x0081;
    const modeFlags = parseModeFlags(hexMode);

    // Boss/MVP protocol: add knockback immunity, status immunity, detector
    if (template.monsterClass === 'boss' || template.monsterClass === 'mvp') {
        modeFlags.knockbackImmune = true;
        modeFlags.statusImmune = true;
        modeFlags.detector = true;
        if (template.monsterClass === 'mvp') modeFlags.mvp = true;
    }

    // Movement speed: walkSpeed = ms per RO cell. 50 UE units = 1 cell.
    // Formula: moveSpeed = (50 / walkSpeed) * 1000 UE units/sec
    const walkSpeedMs = template.walkSpeed || 200;
    const moveSpeed = (50 / walkSpeedMs) * 1000;

    const enemy = {
        enemyId, templateId: spawnConfig.template,
        zone: spawnConfig.zone || 'prontera_south',  // Zone this enemy belongs to
        name: template.name, level: template.level,
        health: template.maxHealth, maxHealth: template.maxHealth,
        damage: template.damage, attackRange: template.attackRange,
        aggroRange: template.aggroRange, aspd: template.aspd,
        chaseRange: template.chaseRange || 600,
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
        walkSpeed: walkSpeedMs,
        attackDelay: template.attackDelay || 1500,
        attackMotion: template.attackMotion || 500,
        damageMotion: template.damageMotion || 300,
        raceGroups: template.raceGroups || {},
        modes: template.modes || {},
        stats: { ...template.stats }, isDead: false,
        x: spawnConfig.x, y: spawnConfig.y, z: spawnConfig.z,
        spawnX: spawnConfig.x, spawnY: spawnConfig.y, spawnZ: spawnConfig.z,
        wanderRadius: spawnConfig.wanderRadius, respawnMs: template.respawnMs,
        // AI state machine fields
        aiCode,
        modeFlags,
        moveSpeed,                      // UE units per second
        aiState: AI_STATE.IDLE,
        targetPlayerId: null,
        aggroOriginX: null,
        aggroOriginY: null,
        lastAttackTime: 0,
        lastDamageTime: 0,
        lastAggroScan: 0,
        lastMoveBroadcast: 0,
        pendingTargetSwitch: null,
        inCombatWith: new Set()
    };
    // Initialize wander state
    initEnemyWanderState(enemy);
    enemies.set(enemyId, enemy);
    const flagSummary = [];
    if (modeFlags.aggressive) flagSummary.push('AGG');
    if (modeFlags.assist) flagSummary.push('AST');
    if (modeFlags.changeTargetMelee) flagSummary.push('CTM');
    if (modeFlags.changeTargetChase) flagSummary.push('CTC');
    if (modeFlags.castSensorIdle) flagSummary.push('CSI');
    if (modeFlags.looter) flagSummary.push('LOT');
    if (!modeFlags.canMove) flagSummary.push('IMMOB');
    if (!modeFlags.canAttack) flagSummary.push('NOATK');
    logger.info(`[ENEMY] Spawned ${enemy.name} (ID: ${enemyId}) Lv${enemy.level} [${enemy.monsterClass}] AI${aiCode} [${flagSummary.join(',')||'passive'}] speed=${moveSpeed.toFixed(0)}u/s at (${enemy.x}, ${enemy.y}, ${enemy.z}) zone=${enemy.zone}`);
    broadcastToZone(enemy.zone, 'enemy:spawn', {
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
// RO Classic: shops have unlimited stock, multiple players can use simultaneously
const NPC_SHOPS = {
    1: {
        name: 'Tool Dealer',
        itemIds: [1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1028, 1029]
    },
    2: {
        name: 'Weapon Dealer',
        itemIds: [3001, 3002, 3003, 3004, 3005, 3006, 3007, 3008, 3009, 3010, 3011, 3012, 3013, 3014, 3015, 3016, 3017, 3018]
    },
    3: {
        name: 'Armor Dealer',
        itemIds: [4001, 4002, 4003, 4004, 4005, 4006, 4007, 4008, 4009, 4010, 4011, 4012, 4013, 4014]
    },
    4: {
        name: 'General Store',
        itemIds: [1001, 1002, 1003, 1004, 1005, 4001, 4002, 4003]
    }
};

// ---- Discount / Overcharge skill helpers (RO Classic Merchant passives) ----
// Discount (601): reduces NPC buy prices. Overcharge (602): increases NPC sell prices.
// Formula: effectValue = 7 + (level-1)*2 → levels 1-10 give 7,9,11,13,15,17,19,21,23,25%
function getDiscountPercent(player) {
    const learned = player.learnedSkills || {};
    const level = learned[601] || 0;
    if (level <= 0) return 0;
    const skill = SKILL_MAP.get(601);
    if (!skill) return 0;
    const lvlData = skill.levels[Math.min(level - 1, skill.levels.length - 1)];
    return lvlData ? lvlData.effectValue : 0;
}

function getOverchargePercent(player) {
    const learned = player.learnedSkills || {};
    const level = learned[602] || 0;
    if (level <= 0) return 0;
    const skill = SKILL_MAP.get(602);
    if (!skill) return 0;
    const lvlData = skill.levels[Math.min(level - 1, skill.levels.length - 1)];
    return lvlData ? lvlData.effectValue : 0;
}

function applyDiscount(basePrice, discountPct) {
    if (discountPct <= 0) return basePrice;
    return Math.floor(basePrice * (100 - discountPct) / 100);
}

function applyOvercharge(basePrice, overchargePct) {
    if (overchargePct <= 0) return basePrice;
    return Math.floor(basePrice * (100 + overchargePct) / 100);
}

// Calculate max weight: RO Classic formula + Enlarge Weight Limit skill (600)
function getPlayerMaxWeight(player) {
    const str = (player.stats && player.stats.str) || 1;
    let maxW = 2000 + str * 30;
    const learned = player.learnedSkills || {};
    const ewlLevel = learned[600] || 0;
    if (ewlLevel > 0) {
        const skill = SKILL_MAP.get(600);
        if (skill) {
            const lvlData = skill.levels[Math.min(ewlLevel - 1, skill.levels.length - 1)];
            if (lvlData) maxW += lvlData.effectValue; // +200 per level
        }
    }
    return maxW;
}

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
async function addItemToInventory(characterId, itemId, quantity = 1, dbClient = null) {
    const db = dbClient || pool;
    const itemDef = itemDefinitions.get(itemId);
    if (!itemDef) {
        logger.error(`[ITEMS] Cannot add unknown item ${itemId} to inventory`);
        return null;
    }

    try {
        if (itemDef.stackable) {
            // Check if player already has this item stacked
            const existing = await db.query(
                'SELECT inventory_id, quantity FROM character_inventory WHERE character_id = $1 AND item_id = $2 AND is_equipped = false',
                [characterId, itemId]
            );

            if (existing.rows.length > 0) {
                const newQty = Math.min(existing.rows[0].quantity + quantity, itemDef.max_stack);
                await db.query(
                    'UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2',
                    [newQty, existing.rows[0].inventory_id]
                );
                logger.info(`[ITEMS] Stacked ${quantity}x ${itemDef.name} for char ${characterId} (now ${newQty})`);
                return { inventoryId: existing.rows[0].inventory_id, itemId, quantity: newQty, isEquipped: false };
            }
        }

        // Find next available slot_index for this character
        const maxSlotResult = await db.query(
            'SELECT COALESCE(MAX(slot_index), -1) as max_slot FROM character_inventory WHERE character_id = $1',
            [characterId]
        );
        const nextSlot = maxSlotResult.rows[0].max_slot + 1;

        // Insert new inventory entry with auto-assigned slot
        const result = await db.query(
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
async function removeItemFromInventory(inventoryId, quantity = null, dbClient = null) {
    const db = dbClient || pool;
    try {
        if (quantity !== null) {
            // Partial removal for stackable items
            const existing = await db.query('SELECT quantity FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
            if (existing.rows.length === 0) return false;
            const newQty = existing.rows[0].quantity - quantity;
            if (newQty <= 0) {
                await db.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
            } else {
                await db.query('UPDATE character_inventory SET quantity = $1 WHERE inventory_id = $2', [newQty, inventoryId]);
            }
        } else {
            await db.query('DELETE FROM character_inventory WHERE inventory_id = $1', [inventoryId]);
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

// ============================================================
// Zone-scoped broadcasting helpers
// ============================================================
function broadcastToZone(zone, event, data) {
    io.to('zone:' + zone).emit(event, data);
}
function broadcastToZoneExcept(socket, zone, event, data) {
    socket.to('zone:' + zone).emit(event, data);
}
function getPlayerZone(characterId) {
    const player = connectedPlayers.get(characterId);
    return player ? (player.zone || 'prontera_south') : 'prontera_south';
}
function isZoneActive(zone) {
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone === zone) return true;
    }
    return false;
}
// Get set of all zones with at least one player (for AI tick optimization)
function getActiveZones() {
    const zones = new Set();
    for (const [, player] of connectedPlayers.entries()) {
        if (player.zone) zones.add(player.zone);
    }
    return zones;
}
// Track which zones have enemies spawned (lazy spawning)
const spawnedZones = new Set();
// Track characters mid-zone-transition (skip login redirect for these)
const zoneTransitioning = new Set();

// Socket.io connection handler
// ============================================================
// Socket Event Rate Limiting
// Per-socket, per-event throttle to prevent DoS via event spam.
// Each event type has a max calls-per-second. Excess calls are silently dropped.
// ============================================================
const SOCKET_RATE_LIMITS = {
    'player:position':      60,  // Movement updates — high frequency expected
    'player:moved':         60,
    'combat:attack':        10,
    'combat:stop_attack':   10,
    'skill:use':            10,
    'chat:message':          5,
    'inventory:use':         5,
    'inventory:equip':       5,
    'inventory:move':       20,
    'inventory:drop':        5,
    'shop:buy':              5,
    'shop:sell':             5,
    'shop:buy_batch':        3,
    'shop:sell_batch':       3,
    'hotbar:save':          10,
    'hotbar:save_skill':    10,
    'hotbar:clear':         10,
    'player:allocate_stat':  5,
};
const socketEventCounts = new Map(); // key: "socketId:eventName" → count this second

// Reset all counters every second
setInterval(() => socketEventCounts.clear(), 1000);

function throttleSocketEvent(socketId, eventName) {
    const limit = SOCKET_RATE_LIMITS[eventName];
    if (!limit) return true; // No limit configured — allow
    const key = `${socketId}:${eventName}`;
    const count = (socketEventCounts.get(key) || 0) + 1;
    socketEventCounts.set(key, count);
    if (count > limit) {
        if (count === limit + 1) {
            logger.warn(`[RATE] ${socketId} exceeded ${eventName} limit (${limit}/s)`);
        }
        return false; // Drop event
    }
    return true; // Allow event
}

io.on('connection', (socket) => {
    logger.info(`Socket connected: ${socket.id}`);

    // Rate limit middleware — intercepts all events before they reach handlers
    socket.use(([eventName, ...args], next) => {
        if (throttleSocketEvent(socket.id, eventName)) {
            next(); // Allow
        }
        // Else silently drop — no next() call, event is swallowed
    });

    // Player authentication and join
    socket.on('player:join', async (data) => {
        logger.info(`[RECV] player:join from ${socket.id}: ${JSON.stringify(data)}`);
        const characterId = parseInt(data.characterId);
        const { token, characterName } = data;

        // SECURITY: Verify JWT token — MANDATORY, reject if missing
        // BP_SocketManager sends GetAuthHeader() which includes "Bearer " prefix — strip it
        const rawToken = token && token.startsWith('Bearer ') ? token.slice(7) : token;
        if (!rawToken) {
            logger.warn(`[SECURITY] player:join without JWT token from ${socket.id}`);
            socket.emit('player:join_error', { error: 'Authentication required' });
            return;
        }
        try {
            const decoded = jwt.verify(rawToken, process.env.JWT_SECRET);
            // Verify character belongs to authenticated user
            const ownerCheck = await pool.query(
                'SELECT 1 FROM characters WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE',
                [characterId, decoded.user_id]
            );
            if (ownerCheck.rows.length === 0) {
                logger.warn(`[SECURITY] Character ${characterId} does not belong to user ${decoded.user_id}`);
                socket.emit('player:join_error', { error: 'Character does not belong to this account' });
                return;
            }
            logger.info(`[AUTH] JWT verified for user ${decoded.user_id}, character ${characterId}`);
        } catch (err) {
            logger.warn(`[SECURITY] Invalid JWT on player:join: ${err.message}`);
            socket.emit('player:join_error', { error: 'Invalid or expired token' });
            return;
        }

        // Fetch character data from database (position + health/mana + zuzucoin)
        let health = 100, maxHealth = 100, mana = 100, maxMana = 100, zuzucoin = 0;
        let initialX, initialY, initialZ;
        let playerZone = 'prontera_south';
        try {
            const charResult = await pool.query(
                'SELECT x, y, z, health, max_health, mana, max_mana, zuzucoin, zone_name FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (charResult.rows.length > 0) {
                const row = charResult.rows[0];
                health = row.health;
                maxHealth = row.max_health;
                mana = row.mana;
                maxMana = row.max_mana;
                zuzucoin = row.zuzucoin || 0;
                initialX = row.x;
                initialY = row.y;
                initialZ = row.z;
                playerZone = row.zone_name || 'prontera_south';

                // Cache correct position in Redis
                await setPlayerPosition(characterId, row.x, row.y, row.z);
                // Join the Socket.io room for this zone
                socket.join('zone:' + playerZone);
                // Broadcast correct position to other players in same zone
                broadcastToZoneExcept(socket, playerZone, 'player:moved', {
                    characterId,
                    characterName,
                    x: row.x,
                    y: row.y,
                    z: row.z,
                    health,
                    maxHealth,
                    timestamp: Date.now()
                });
                logger.info(`Broadcasted initial position for ${characterName} (Character ${characterId}) at (${row.x}, ${row.y}, ${row.z}) zone=${playerZone}`);
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
        let weaponResult = null;
        try {
            weaponResult = await pool.query(
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

        // Track weapon type for passive mastery skills (reuse weapon query result above)
        let weaponType = null;
        if (weaponResult && weaponResult.rows.length > 0) {
            weaponType = weaponResult.rows[0].weapon_type;
        }

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
            zone: playerZone,
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
            cardMods: null,            // Card % bonuses (race/element/size)
            // Position tracking for enemy AI aggro detection (updated by player:position)
            lastX: initialX,
            lastY: initialY,
            lastZ: initialZ
        });
        
        logger.info(`Player joined: ${characterName || 'Unknown'} (Character ${characterId}) HP: ${health}/${maxHealth} MP: ${mana}/${maxMana} zone=${playerZone}`);
        const zoneInfo = getZone(playerZone);
        const joinedPayload = {
            success: true, zuzucoin,
            zone: playerZone,
            levelName: zoneInfo ? zoneInfo.levelName : 'L_PrtSouth',
            displayName: zoneInfo ? zoneInfo.displayName : 'Prontera South Field',
            x: initialX || (zoneInfo ? zoneInfo.defaultSpawn.x : 0),
            y: initialY || (zoneInfo ? zoneInfo.defaultSpawn.y : 0),
            z: initialZ || (zoneInfo ? zoneInfo.defaultSpawn.z : 580)
        };
        socket.emit('player:joined', joinedPayload);
        logger.info(`[SEND] player:joined to ${socket.id}: ${JSON.stringify(joinedPayload)}`);
        
        // Send initial health state to the joining player
        const selfHealthPayload = { characterId, health, maxHealth, mana, maxMana };
        socket.emit('combat:health_update', selfHealthPayload);
        logger.info(`[SEND] combat:health_update to ${socket.id}: ${JSON.stringify(selfHealthPayload)}`);
        
        // Broadcast this player's health to others in same zone so they can show HP bars
        const broadcastHealthPayload = { characterId, health, maxHealth, mana, maxMana };
        broadcastToZoneExcept(socket, playerZone, 'combat:health_update', broadcastHealthPayload);
        logger.info(`[BROADCAST] combat:health_update to zone:${playerZone} (excl ${socket.id}): ${JSON.stringify(broadcastHealthPayload)}`);

        // Send existing players' health to the joining player (only same zone)
        for (const [existingCharId, existingPlayer] of connectedPlayers.entries()) {
            if (existingCharId !== characterId && existingPlayer.zone === playerZone) {
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

        // Send current buff/status list (may have persisted buffs from combat)
        socket.emit('buff:list', {
            characterId,
            buffs: getActiveBuffList(playerObj),
            statuses: getActiveStatusList(playerObj)
        });
        logger.info(`[SEND] buff:list to ${socket.id} on join`);

        // Send hotbar data after a short delay to ensure HUD is ready
        setTimeout(async () => {
            const hotbar = await getPlayerHotbar(characterId);
            socket.emit('hotbar:alldata', { slots: hotbar });
            logger.info(`[SEND] hotbar:alldata to ${socket.id}: ${hotbar.length} slots (on join, delayed)`);
        }, 600); // 0.6 second delay
        
        // Lazy enemy spawning: spawn enemies for this zone if not yet active
        if (!spawnedZones.has(playerZone)) {
            const zoneData = getZone(playerZone);
            if (zoneData && zoneData.enemySpawns.length > 0) {
                logger.info(`[ZONE] First player in '${playerZone}' — spawning ${zoneData.enemySpawns.length} enemies`);
                for (const spawn of zoneData.enemySpawns) {
                    spawnEnemy({ ...spawn, zone: playerZone });
                }
            }
            spawnedZones.add(playerZone);
        }

        // Send existing enemies to the joining player (only same zone)
        for (const [eid, enemy] of enemies.entries()) {
            if (!enemy.isDead && enemy.zone === playerZone) {
                socket.emit('enemy:spawn', {
                    enemyId: eid, templateId: enemy.templateId, name: enemy.name,
                    level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
                    x: enemy.x, y: enemy.y, z: enemy.z
                });
            }
        }

        // Send zone metadata to client (warps, Kafra NPCs, flags)
        if (zoneInfo) {
            socket.emit('zone:data', {
                zone: playerZone,
                displayName: zoneInfo.displayName,
                levelName: zoneInfo.levelName,
                flags: zoneInfo.flags,
                warps: zoneInfo.warps,
                kafraNpcs: zoneInfo.kafraNpcs
            });
        }

        // Zone redirect removed — client now loads the correct level directly
        // using zone_name/level_name from the character REST API response.
        // Clear transitioning flag now that join is complete
        zoneTransitioning.delete(characterId);
    });
    
    // Position update from client
    socket.on('player:position', async (data) => {
        logger.debug(`[RECV] player:position from ${socket.id}: ${JSON.stringify(data)}`);
        const characterId = parseInt(data.characterId);
        const { x, y, z } = data;
        const player = connectedPlayers.get(characterId);
        const characterName = player ? player.characterName : 'Unknown';

        // Movement lock: reject position updates during CC (stun/freeze/stone/sleep)
        if (player) {
            const ccMods = getCombinedModifiers(player);
            if (ccMods.preventsMovement) {
                socket.emit('player:position_rejected', {
                    x: player.lastX || x, y: player.lastY || y, z: player.lastZ || z,
                    reason: 'cc_locked'
                });
                return;
            }
        }

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
        
        // Broadcast to other players in same zone
        const posZone = player ? (player.zone || 'prontera_south') : 'prontera_south';
        broadcastToZoneExcept(socket, posZone, 'player:moved', {
            characterId,
            characterName,
            x, y, z,
            health: player ? player.health : 100,
            maxHealth: player ? player.maxHealth : 100,
            timestamp: Date.now()
        });
        logger.debug(`[BROADCAST] player:moved for ${characterName} (Character ${characterId}) zone=${posZone}`);
    });
    
    // ============================================================
    // Zone Warp: Player stepped into a warp portal
    // ============================================================
    socket.on('zone:warp', async (data) => {
        logger.info(`[RECV] zone:warp from ${socket.id}: ${JSON.stringify(data)}`);
        const { warpId } = data;
        if (!warpId) {
            socket.emit('zone:error', { message: 'Missing warpId' });
            return;
        }

        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) {
            socket.emit('zone:error', { message: 'Player not found' });
            return;
        }
        const { characterId, player } = playerInfo;
        const oldZone = player.zone || 'prontera_south';
        const oldZoneData = getZone(oldZone);
        if (!oldZoneData) {
            socket.emit('zone:error', { message: 'Current zone not found' });
            return;
        }

        // Find the warp definition in current zone
        const warp = oldZoneData.warps.find(w => w.id === warpId);
        if (!warp) {
            socket.emit('zone:error', { message: `Warp '${warpId}' not found in ${oldZone}` });
            return;
        }

        // Validate player proximity to warp (generous range: warp radius * 2)
        const maxWarpDist = (warp.radius || 200) * 2;
        const px = player.lastX || 0, py = player.lastY || 0;
        const warpDist = Math.sqrt((px - warp.x) ** 2 + (py - warp.y) ** 2);
        if (warpDist > maxWarpDist) {
            socket.emit('zone:error', { message: 'Too far from warp portal' });
            return;
        }

        const newZone = warp.destZone;
        const newZoneData = getZone(newZone);
        if (!newZoneData) {
            socket.emit('zone:error', { message: `Destination zone '${newZone}' not found` });
            return;
        }

        // Cancel active combat/casting
        autoAttackState.delete(characterId);
        activeCasts.delete(characterId);
        afterCastDelayEnd.delete(characterId);

        // Remove from enemy combat sets in old zone
        for (const [, enemy] of enemies.entries()) {
            if (enemy.zone === oldZone) {
                enemy.inCombatWith.delete(characterId);
                if (enemy.targetPlayerId === characterId) {
                    enemy.targetPlayerId = null;
                    const next = pickNextTarget(enemy);
                    if (next) {
                        enemy.targetPlayerId = next;
                    } else {
                        enemy.aiState = AI_STATE.IDLE;
                        enemy.isWandering = false;
                    }
                }
            }
        }

        // Broadcast player:left to old zone
        broadcastToZone(oldZone, 'player:left', {
            characterId, characterName: player.characterName, reason: 'zone_change'
        });

        // Switch Socket.io rooms
        socket.leave('zone:' + oldZone);
        socket.join('zone:' + newZone);

        // Update player data
        player.zone = newZone;
        player.lastX = warp.destX;
        player.lastY = warp.destY;
        player.lastZ = warp.destZ;

        // Mark as transitioning so login redirect is skipped on reconnect
        zoneTransitioning.add(characterId);

        // Save to DB
        try {
            await pool.query(
                'UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5',
                [newZone, warp.destX, warp.destY, warp.destZ, characterId]
            );
        } catch (err) {
            logger.warn(`[DB] Failed to save zone change for char ${characterId}: ${err.message}`);
        }

        // Lazy spawn enemies in new zone
        if (!spawnedZones.has(newZone)) {
            if (newZoneData.enemySpawns.length > 0) {
                logger.info(`[ZONE] First player in '${newZone}' — spawning ${newZoneData.enemySpawns.length} enemies`);
                for (const spawn of newZoneData.enemySpawns) {
                    spawnEnemy({ ...spawn, zone: newZone });
                }
            }
            spawnedZones.add(newZone);
        }

        // Tell client to load new level
        socket.emit('zone:change', {
            zone: newZone,
            displayName: newZoneData.displayName,
            levelName: newZoneData.levelName,
            x: warp.destX, y: warp.destY, z: warp.destZ,
            flags: newZoneData.flags,
            reason: 'warp'
        });

        logger.info(`[ZONE] ${player.characterName} warped ${oldZone} → ${newZone} via ${warpId}`);
    });

    // ============================================================
    // Zone Ready: Client finished loading the new level
    // ============================================================
    socket.on('zone:ready', async (data) => {
        logger.info(`[RECV] zone:ready from ${socket.id}: ${JSON.stringify(data || {})}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        const { characterId, player } = playerInfo;
        const zone = player.zone || 'prontera_south';

        // Broadcast this player's arrival to others in the zone
        broadcastToZoneExcept(socket, zone, 'player:moved', {
            characterId,
            characterName: player.characterName,
            x: player.lastX || 0,
            y: player.lastY || 0,
            z: player.lastZ || 300,
            health: player.health,
            maxHealth: player.maxHealth,
            timestamp: Date.now()
        });

        // Send this player's health to the zone
        broadcastToZoneExcept(socket, zone, 'combat:health_update', {
            characterId,
            health: player.health,
            maxHealth: player.maxHealth,
            mana: player.mana,
            maxMana: player.maxMana
        });

        // Send all zone enemies to this client
        for (const [eid, enemy] of enemies.entries()) {
            if (!enemy.isDead && enemy.zone === zone) {
                socket.emit('enemy:spawn', {
                    enemyId: eid, templateId: enemy.templateId, name: enemy.name,
                    level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
                    x: enemy.x, y: enemy.y, z: enemy.z
                });
            }
        }

        // Send all other players in this zone to this client
        for (const [existingCharId, existingPlayer] of connectedPlayers.entries()) {
            if (existingCharId !== characterId && existingPlayer.zone === zone) {
                socket.emit('player:moved', {
                    characterId: existingCharId,
                    characterName: existingPlayer.characterName,
                    x: existingPlayer.lastX || 0,
                    y: existingPlayer.lastY || 0,
                    z: existingPlayer.lastZ || 300,
                    health: existingPlayer.health,
                    maxHealth: existingPlayer.maxHealth,
                    timestamp: Date.now()
                });
                socket.emit('combat:health_update', {
                    characterId: existingCharId,
                    health: existingPlayer.health,
                    maxHealth: existingPlayer.maxHealth,
                    mana: existingPlayer.mana,
                    maxMana: existingPlayer.maxMana
                });
            }
        }

        // Send zone metadata
        const zoneData = getZone(zone);
        if (zoneData) {
            socket.emit('zone:data', {
                zone,
                displayName: zoneData.displayName,
                levelName: zoneData.levelName,
                flags: zoneData.flags,
                warps: zoneData.warps,
                kafraNpcs: zoneData.kafraNpcs
            });
        }

        // Re-sync buff/status list (subsystems are fresh after zone change)
        socket.emit('buff:list', {
            characterId,
            buffs: getActiveBuffList(player),
            statuses: getActiveStatusList(player)
        });
        logger.info(`[SEND] buff:list to ${socket.id} on zone:ready (${getActiveBuffList(player).length} buffs, ${getActiveStatusList(player).length} statuses)`);

        logger.info(`[ZONE] ${player.characterName} ready in zone ${zone}`);
    });

    // ============================================================
    // Kafra NPC Events
    // ============================================================
    socket.on('kafra:open', async (data) => {
        logger.info(`[RECV] kafra:open from ${socket.id}: ${JSON.stringify(data)}`);
        const { kafraId } = data;
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        const { characterId, player } = playerInfo;
        const zone = player.zone || 'prontera_south';
        const zoneData = getZone(zone);
        if (!zoneData) {
            socket.emit('kafra:error', { message: 'Zone not found' });
            return;
        }

        const kafra = zoneData.kafraNpcs.find(k => k.id === kafraId);
        if (!kafra) {
            socket.emit('kafra:error', { message: 'Kafra NPC not found in this zone' });
            return;
        }

        // Get save point
        let currentSaveMap = 'prontera';
        try {
            const saveResult = await pool.query('SELECT save_map FROM characters WHERE character_id = $1', [characterId]);
            if (saveResult.rows.length > 0) currentSaveMap = saveResult.rows[0].save_map || 'prontera';
        } catch (err) { /* use default */ }

        socket.emit('kafra:data', {
            kafraId: kafra.id,
            kafraName: kafra.name,
            destinations: kafra.destinations,
            playerZuzucoin: player.zuzucoin,
            currentSaveMap
        });
    });

    socket.on('kafra:save', async () => {
        logger.info(`[RECV] kafra:save from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        const { characterId, player } = playerInfo;
        const zone = player.zone || 'prontera_south';
        const zoneData = getZone(zone);

        // Check nosave flag
        if (zoneData && zoneData.flags.nosave) {
            socket.emit('kafra:error', { message: 'Cannot save in this zone' });
            return;
        }

        const saveX = player.lastX || 0;
        const saveY = player.lastY || 0;
        const saveZ = player.lastZ || 300;

        try {
            await pool.query(
                'UPDATE characters SET save_map = $1, save_x = $2, save_y = $3, save_z = $4 WHERE character_id = $5',
                [zone, saveX, saveY, saveZ, characterId]
            );
            socket.emit('kafra:saved', { saveMap: zone, saveX, saveY, saveZ });
            logger.info(`[KAFRA] ${player.characterName} saved at ${zone} (${saveX}, ${saveY}, ${saveZ})`);
        } catch (err) {
            socket.emit('kafra:error', { message: 'Failed to save location' });
            logger.error(`[KAFRA] Save failed for char ${characterId}: ${err.message}`);
        }
    });

    socket.on('kafra:teleport', async (data) => {
        logger.info(`[RECV] kafra:teleport from ${socket.id}: ${JSON.stringify(data)}`);
        const { kafraId, destZone } = data;
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        const { characterId, player } = playerInfo;
        const zone = player.zone || 'prontera_south';
        const zoneData = getZone(zone);
        if (!zoneData) {
            socket.emit('kafra:error', { message: 'Zone not found' });
            return;
        }

        const kafra = zoneData.kafraNpcs.find(k => k.id === kafraId);
        if (!kafra) {
            socket.emit('kafra:error', { message: 'Kafra NPC not found' });
            return;
        }

        const dest = kafra.destinations.find(d => d.zone === destZone);
        if (!dest) {
            socket.emit('kafra:error', { message: 'Invalid destination' });
            return;
        }

        // Check zeny
        if (player.zuzucoin < dest.cost) {
            socket.emit('kafra:error', { message: `Not enough Zuzucoin (need ${dest.cost}, have ${player.zuzucoin})` });
            return;
        }

        // Deduct cost
        player.zuzucoin -= dest.cost;
        try {
            await pool.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [player.zuzucoin, characterId]);
        } catch (err) {
            logger.warn(`[DB] Failed to deduct zuzucoin for char ${characterId}: ${err.message}`);
        }

        const destZoneData = getZone(destZone);
        if (!destZoneData) {
            socket.emit('kafra:error', { message: 'Destination zone not found' });
            return;
        }

        // Use the warp flow: leave old zone, join new zone
        autoAttackState.delete(characterId);
        activeCasts.delete(characterId);
        afterCastDelayEnd.delete(characterId);

        // Clean enemy combat sets
        for (const [, enemy] of enemies.entries()) {
            if (enemy.zone === zone) {
                enemy.inCombatWith.delete(characterId);
                if (enemy.targetPlayerId === characterId) {
                    enemy.targetPlayerId = null;
                    const next = pickNextTarget(enemy);
                    if (next) { enemy.targetPlayerId = next; }
                    else { enemy.aiState = AI_STATE.IDLE; enemy.isWandering = false; }
                }
            }
        }

        broadcastToZone(zone, 'player:left', {
            characterId, characterName: player.characterName, reason: 'kafra_teleport'
        });

        socket.leave('zone:' + zone);
        socket.join('zone:' + destZone);

        const destSpawn = destZoneData.defaultSpawn;
        player.zone = destZone;
        player.lastX = destSpawn.x;
        player.lastY = destSpawn.y;
        player.lastZ = destSpawn.z;
        zoneTransitioning.add(characterId);

        try {
            await pool.query(
                'UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5',
                [destZone, destSpawn.x, destSpawn.y, destSpawn.z, characterId]
            );
        } catch (err) {
            logger.warn(`[DB] Failed to save kafra teleport for char ${characterId}: ${err.message}`);
        }

        // Lazy spawn enemies
        if (!spawnedZones.has(destZone)) {
            if (destZoneData.enemySpawns.length > 0) {
                for (const spawn of destZoneData.enemySpawns) {
                    spawnEnemy({ ...spawn, zone: destZone });
                }
            }
            spawnedZones.add(destZone);
        }

        socket.emit('kafra:teleported', {
            destZone, cost: dest.cost, remainingZuzucoin: player.zuzucoin
        });

        socket.emit('zone:change', {
            zone: destZone,
            displayName: destZoneData.displayName,
            levelName: destZoneData.levelName,
            x: destSpawn.x, y: destSpawn.y, z: destSpawn.z,
            flags: destZoneData.flags,
            reason: 'kafra_teleport'
        });

        logger.info(`[KAFRA] ${player.characterName} teleported ${zone} → ${destZone} (cost: ${dest.cost}z)`);
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
                
                // Remove this player from enemy combat sets and clear AI targets
                for (const [, enemy] of enemies.entries()) {
                    enemy.inCombatWith.delete(charId);
                    // If this enemy was targeting the disconnected player, find new target or go idle
                    if (enemy.targetPlayerId === charId) {
                        enemy.targetPlayerId = null;
                        // Try to pick next target from remaining combatants
                        if (typeof pickNextTarget === 'function') {
                            const nextTarget = pickNextTarget(enemy);
                            if (nextTarget) {
                                enemy.targetPlayerId = nextTarget;
                            } else {
                                enemy.aiState = AI_STATE.IDLE;
                                enemy.isWandering = false;
                            }
                        } else {
                            enemy.aiState = AI_STATE.IDLE;
                            enemy.isWandering = false;
                        }
                    }
                }
                
                const leftZone = player.zone || 'prontera_south';

                connectedPlayers.delete(charId);
                logger.info(`Player left: Character ${charId} zone=${leftZone}`);

                // Save zone + position to DB on disconnect
                try {
                    await pool.query(
                        'UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5',
                        [leftZone, player.lastX || 0, player.lastY || 0, player.lastZ || 0, charId]
                    );
                } catch (zErr) {
                    logger.warn(`[DB] Failed to save zone/position for char ${charId}: ${zErr.message}`);
                }

                // Broadcast to other players in same zone using io (socket is already disconnected)
                const leftPayload = { characterId: charId, characterName: player.characterName || 'Unknown' };
                broadcastToZone(leftZone, 'player:left', leftPayload);
                logger.info(`[BROADCAST] player:left to zone:${leftZone}: ${JSON.stringify(leftPayload)}`);
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

        // Check CC (cannot attack while stunned, frozen, stoned, sleeping)
        const attackCCMods = getCombinedModifiers(attacker);
        if (attackCCMods.preventsAttack) {
            socket.emit('combat:error', { message: 'Cannot attack while incapacitated' });
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

            // RO AI: Set enemy aggro on this attacker (passive mobs fight back, assist nearby)
            if (typeof setEnemyAggro === 'function') {
                setEnemyAggro(enemy, attackerId, 'melee');
            }
            
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

        // Get save point from DB (or use default)
        let saveMap = 'prontera', saveX = 0, saveY = -2200, saveZ = 300;
        try {
            const saveResult = await pool.query(
                'SELECT save_map, save_x, save_y, save_z FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (saveResult.rows.length > 0) {
                const sr = saveResult.rows[0];
                saveMap = sr.save_map || 'prontera';
                saveX = sr.save_x || 0;
                saveY = sr.save_y || 0;
                saveZ = sr.save_z || 580;
            }
        } catch (err) {
            logger.warn(`[DB] Failed to get save point for char ${characterId}: ${err.message}`);
        }

        const oldZone = player.zone || 'prontera_south';
        const needsZoneChange = saveMap !== oldZone;

        // Update position cache
        await setPlayerPosition(characterId, saveX, saveY, saveZ);
        player.lastX = saveX;
        player.lastY = saveY;
        player.lastZ = saveZ;

        if (needsZoneChange) {
            // Cross-zone respawn — move player to save point zone
            broadcastToZone(oldZone, 'player:left', {
                characterId, characterName: player.characterName, reason: 'respawn'
            });
            socket.leave('zone:' + oldZone);
            socket.join('zone:' + saveMap);
            player.zone = saveMap;
            zoneTransitioning.add(characterId);
            await pool.query('UPDATE characters SET zone_name = $1 WHERE character_id = $2', [saveMap, characterId]);

            // Lazy spawn enemies in the new zone if needed
            if (!spawnedZones.has(saveMap)) {
                const zd = getZone(saveMap);
                if (zd && zd.enemySpawns.length > 0) {
                    for (const spawn of zd.enemySpawns) {
                        spawnEnemy({ ...spawn, zone: saveMap });
                    }
                }
                spawnedZones.add(saveMap);
            }

            const zoneInfo = getZone(saveMap);
            socket.emit('zone:change', {
                zone: saveMap,
                displayName: zoneInfo ? zoneInfo.displayName : saveMap,
                levelName: zoneInfo ? zoneInfo.levelName : 'L_PrtSouth',
                x: saveX, y: saveY, z: saveZ,
                flags: zoneInfo ? zoneInfo.flags : {},
                reason: 'respawn'
            });
            logger.info(`[COMBAT] ${player.characterName} respawned at save point in ${saveMap} (cross-zone)`);
        } else {
            logger.info(`[COMBAT] ${player.characterName} respawned at save point (${saveX}, ${saveY}, ${saveZ}) in ${oldZone}`);
        }

        // Notify players in the respawn zone
        const respawnPayload = {
            characterId,
            characterName: player.characterName,
            health: player.health,
            maxHealth: player.maxHealth,
            mana: player.mana,
            maxMana: player.maxMana,
            x: saveX,
            y: saveY,
            z: saveZ,
            teleport: true,
            timestamp: Date.now()
        };
        broadcastToZone(player.zone, 'combat:respawn', respawnPayload);
        logger.info(`[BROADCAST] combat:respawn to zone:${player.zone}: ${JSON.stringify(respawnPayload)}`);
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
    
    // Request current buff/status list (used after zone change or reconnect)
    socket.on('buff:request', () => {
        logger.info(`[RECV] buff:request from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        const { characterId, player } = playerInfo;
        socket.emit('buff:list', {
            characterId,
            buffs: getActiveBuffList(player),
            statuses: getActiveStatusList(player)
        });
        logger.info(`[SEND] buff:list to ${socket.id} on request`);
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
        player.health = Math.min(player.health, player.maxHealth);
        player.mana = Math.min(player.mana, player.maxMana);

        // Save to DB
        try {
            const dbStatName = statName === 'int' ? 'int_stat' : statName;
            await pool.query(
                `UPDATE characters SET ${dbStatName} = $1, stat_points = $2, max_health = $3, max_mana = $4, health = $5, mana = $6 WHERE character_id = $7`,
                [player.stats[statKey], player.stats.statPoints, player.maxHealth, player.maxMana, player.health, player.mana, characterId]
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
        
        // Broadcast to other players in zone
        const jobZone = player.zone || 'prontera_south';
        broadcastToZoneExcept(socket, jobZone, 'job:changed', {
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

        // Check CC (cannot cast while frozen, stoned, stunned, sleeping, silenced)
        const ccMods = getCombinedModifiers(player);
        if (ccMods.preventsCasting) {
            let ccReason = 'Cannot use skills while incapacitated';
            if (ccMods.isFrozen) ccReason = 'Cannot use skills while frozen';
            else if (ccMods.isStoned) ccReason = 'Cannot use skills while petrified';
            else if (ccMods.isStunned) ccReason = 'Cannot use skills while stunned';
            else if (ccMods.isSleeping) ccReason = 'Cannot use skills while sleeping';
            else if (ccMods.isSilenced) ccReason = 'Cannot use skills while silenced';
            socket.emit('skill:error', { message: ccReason });
            return;
        }

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

        // Parse ground coordinates for ground-targeted skills (Magnum Break, Thunderstorm, Fire Wall, Safety Wall)
        const groundX = data.groundX !== undefined ? parseFloat(data.groundX) : undefined;
        const groundY = data.groundY !== undefined ? parseFloat(data.groundY) : undefined;
        const groundZ = data.groundZ !== undefined ? parseFloat(data.groundZ) : undefined;
        const hasGroundPos = groundX !== undefined && groundY !== undefined && !isNaN(groundX) && !isNaN(groundY);

        // ================================================================
        // PRE-CAST RANGE CHECK — RO Classic behavior
        // In RO Classic, if the player tries to cast a skill out of range
        // while stationary, nothing happens (silent rejection). The cast
        // never starts. Range is checked BEFORE entering the casting state.
        // ================================================================
        if (!data._castComplete) {
            const skillRange = skill.range || 900;
            const casterPos = await getPlayerPosition(characterId);

            if (casterPos && hasGroundPos && skill.targetType === 'ground') {
                const dx = casterPos.x - groundX;
                const dy = casterPos.y - groundY;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > skillRange + COMBAT.RANGE_TOLERANCE) {
                    logger.info(`[SKILLS] ${player.characterName} tried ${skill.displayName} on ground out of range (dist=${Math.round(dist)}, max=${skillRange + COMBAT.RANGE_TOLERANCE})`);
                    socket.emit('combat:out_of_range', { skillId, distance: Math.round(dist), maxRange: skillRange });
                    return;
                }
            }

            if (casterPos && targetId && isEnemy) {
                const target = enemies.get(targetId);
                if (target) {
                    const dx = casterPos.x - target.x;
                    const dy = casterPos.y - target.y;
                    const dist = Math.sqrt(dx * dx + dy * dy);
                    if (dist > skillRange + COMBAT.RANGE_TOLERANCE) {
                        logger.info(`[SKILLS] ${player.characterName} tried ${skill.displayName} on enemy ${targetId} out of range (dist=${Math.round(dist)}, max=${skillRange + COMBAT.RANGE_TOLERANCE})`);
                        socket.emit('combat:out_of_range', { skillId, targetId, distance: Math.round(dist), maxRange: skillRange });
                        return;
                    }
                }
            }
        }

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
                // Broadcast cast start to zone clients (for cast bar rendering)
                const castStartZone = player.zone || 'prontera_south';
                const castAttackerPos = await getPlayerPosition(characterId);
                broadcastToZone(castStartZone, 'skill:cast_start', {
                    casterId: characterId, casterName: player.characterName,
                    skillId, skillName: skill.displayName,
                    actualCastTime, targetId, isEnemy,
                    casterX: castAttackerPos?.x || 0, casterY: castAttackerPos?.y || 0, casterZ: castAttackerPos?.z || 0
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

            const attackerPos = await getPlayerPosition(characterId);
            const faZone = player.zone || 'prontera_south';

            // Broadcast heal VFX zone-wide so other players see it
            broadcastToZone(faZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId: characterId, targetName: player.characterName, isEnemy: false,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'holy',
                damage: 0, healAmount: healed, isCritical: false, isMiss: false, hitType: 'heal',
                targetHealth: player.health, targetMaxHealth: player.maxHealth,
                attackerX: attackerPos?.x || 0, attackerY: attackerPos?.y || 0, attackerZ: attackerPos?.z || 0,
                targetX: attackerPos?.x || 0, targetY: attackerPos?.y || 0, targetZ: attackerPos?.z || 0,
                timestamp: Date.now()
            });

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana, healAmount: healed });
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
            // skill.element = undefined/null for physical skills → formula falls through to attacker.weaponElement
            // Only forced-element skills (fire/water/wind/ghost/etc.) override weapon element
            const bashResult = calculateSkillDamage(
                getEffectiveStats(player), isEnemy ? targetStats : getEffectiveStats(target),
                targetHardDef, effectVal, atkBuffMods, defBuffMods,
                skillTargetInfo, skillAtkInfo, { skillElement: skill.element === 'neutral' ? null : (skill.element || null) }
            );
            const { damage, isCritical, isMiss, hitType: bashHitType } = bashResult;

            const bashZone = player.zone || 'prontera_south';
            if (isMiss) {
                // Skill missed — still consume SP/cooldown, broadcast miss
                broadcastToZone(bashZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName, isEnemy,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: bashResult.element || 'neutral',
                    damage: 0, isCritical: false, isMiss: true, hitType: bashHitType,
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
            // RO AI: trigger aggro + assist on enemy skill hit
            if (isEnemy) {
                target.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(target, characterId, 'melee');
            }
            logger.info(`[SKILL-COMBAT] ${player.characterName} BASH Lv${learnedLevel} → ${targetName} for ${damage}${isCritical ? ' CRIT' : ''} [${bashHitType}] (HP: ${target.health}/${target.maxHealth})`);

            // Broadcast skill damage to zone
            broadcastToZone(bashZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: bashResult.element || 'neutral',
                damage, isCritical, isMiss: false, hitType: bashHitType,
                targetHealth: target.health, targetMaxHealth: target.maxHealth,
                attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: Date.now()
            });

            // Enemy health update
            if (isEnemy) {
                broadcastToZone(bashZone, 'enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
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
                    broadcastToZone(bashZone, 'combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                    broadcastToZone(bashZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Bash!`, timestamp: Date.now() });
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
            const provokeZone = player.zone || 'prontera_south';
            const buffPayload = {
                targetId, targetName, isEnemy,
                casterId: characterId, casterName: player.characterName,
                skillId, buffName: 'Provoke', duration: duration || 30000,
                effects: { defReduction: effectVal, atkIncrease: effectVal }
            };
            logger.info(`[SEND] skill:buff_applied to zone:${provokeZone}: ${JSON.stringify(buffPayload)}`);
            broadcastToZone(provokeZone, 'skill:buff_applied', buffPayload);

            socket.emit('skill:used', { skillId, skillName: skill.displayName, level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana });
            // cooldown_started emitted by applySkillDelays
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- MAGNUM BREAK (ID 105) — Ground-targeted AoE fire damage ---
        if (skill.name === 'magnum_break') {
            const attackerPos = await getPlayerPosition(characterId);
            if (!attackerPos) return;

            // Ground-targeted: use ground coordinates from client
            let centerPos;
            if (hasGroundPos) {
                centerPos = { x: groundX, y: groundY, z: groundZ || 0 };
            } else {
                // Fallback to caster position if no ground coords
                centerPos = { ...attackerPos };
            }

            // Range check — short range skill (melee AoE)
            const mbRange = skill.range || 50;
            const rangeDist = Math.sqrt((attackerPos.x - centerPos.x) ** 2 + (attackerPos.y - centerPos.y) ** 2);
            if (rangeDist > mbRange + COMBAT.RANGE_TOLERANCE) {
                socket.emit('combat:out_of_range', { targetId: 0, isEnemy: false, distance: rangeDist, requiredRange: mbRange });
                return;
            }

            // Deduct SP and set cooldown
            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const AOE_RADIUS = 300; // Fire radius in UE units
            const mbZone = player.zone || 'prontera_south';
            const atkBuffMods = getBuffStatModifiers(player);
            let totalDamageDealt = 0;
            let enemiesHit = 0;

            // Hit all enemies within AoE radius of ground target
            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                const dx = centerPos.x - enemy.x;
                const dy = centerPos.y - enemy.y;
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
                    broadcastToZone(mbZone, 'skill:effect_damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId: eid, targetName: enemy.name, isEnemy: true,
                        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                        damage: 0, isCritical: false, isMiss: true, hitType: mbHitType,
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

                // RO AI: AOE aggro — each hit enemy aggros the caster
                enemy.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(enemy, characterId, 'skill');

                // Broadcast damage for each enemy hit
                broadcastToZone(mbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                    damage, isCritical, isMiss: false, hitType: mbHitType,
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    timestamp: Date.now()
                });
                broadcastToZone(mbZone, 'enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true });

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
                const dx = centerPos.x - pPos.x;
                const dy = centerPos.y - pPos.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > AOE_RADIUS) continue;

                const defBuffMods = getBuffStatModifiers(ptarget);
                const mbPvpResult = calculateSkillDamage(
                    getEffectiveStats(player), getEffectiveStats(ptarget), ptarget.hardDef || 0, effectVal, atkBuffMods, defBuffMods,
                    getPlayerTargetInfo(ptarget, pid), getAttackerInfo(player), { skillElement: 'fire' }
                );
                const { damage: pvpMbDmg, isCritical: pvpMbCrit, isMiss: pvpMbMiss, hitType: pvpMbHitType } = mbPvpResult;

                if (pvpMbMiss) {
                    broadcastToZone(mbZone, 'skill:effect_damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId: pid, targetName: ptarget.characterName, isEnemy: false,
                        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                        damage: 0, isCritical: false, isMiss: true, hitType: pvpMbHitType,
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

                broadcastToZone(mbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: pid, targetName: ptarget.characterName, isEnemy: false,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                    damage: pvpMbDmg, isCritical: pvpMbCrit, isMiss: false, hitType: pvpMbHitType,
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
                    broadcastToZone(mbZone, 'combat:death', { killedId: pid, killedName: ptarget.characterName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now() });
                    broadcastToZone(mbZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${ptarget.characterName} with Magnum Break!`, timestamp: Date.now() });
                    await savePlayerHealthToDB(pid, 0, ptarget.mana);
                }
            }

            // If no targets were hit, emit a zero-damage "whiff" so the client still shows the AoE VFX
            if (enemiesHit === 0) {
                broadcastToZone(mbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: characterId, targetName: player.characterName, isEnemy: false,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'fire',
                    damage: 0, isCritical: false, isMiss: false, hitType: 'normal',
                    targetHealth: player.health, targetMaxHealth: player.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: centerPos.x, targetY: centerPos.y, targetZ: centerPos.z,
                    timestamp: Date.now()
                });
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

            const endureZone = player.zone || 'prontera_south';
            const endurePos = await getPlayerPosition(characterId);
            broadcastToZone(endureZone, 'skill:buff_applied', {
                targetId: characterId, targetName: player.characterName, isEnemy: false,
                casterId: characterId, casterName: player.characterName,
                skillId, buffName: 'Endure', duration: buffDuration,
                effects: { mdefBonus }
            });
            // Broadcast VFX zone-wide so other players see the effect
            broadcastToZone(endureZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId: characterId, targetName: player.characterName, isEnemy: false,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: 'neutral',
                damage: 0, isCritical: false, isMiss: false, hitType: 'buff',
                targetHealth: player.health, targetMaxHealth: player.maxHealth,
                attackerX: endurePos?.x || 0, attackerY: endurePos?.y || 0, attackerZ: endurePos?.z || 0,
                targetX: endurePos?.x || 0, targetY: endurePos?.y || 0, targetZ: endurePos?.z || 0,
                timestamp: Date.now()
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

            const boltZone = player.zone || 'prontera_south';
            // Check element immunity (if first hit was 0, all are 0)
            if (totalDamage <= 0) {
                broadcastToZone(boltZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId, targetName, isEnemy,
                    skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                    damage: 0, hits: 0, isCritical: false, isMiss: true, hitType: 'magical',
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
            // RO AI: trigger aggro + assist on enemy skill hit
            if (isEnemy) {
                target.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(target, characterId, 'skill');
            }
            logger.info(`[SKILL-COMBAT] ${player.characterName} ${skill.displayName} Lv${learnedLevel} → ${targetName} for ${totalDamage} (${numHits} hits) [${skill.element}] (HP: ${target.health}/${target.maxHealth})`);

            // Damage breaks freeze/stone/sleep/confusion
            const boltBroken = checkDamageBreakStatuses(target);
            for (const brokenType of boltBroken) {
                broadcastToZone(boltZone, 'status:removed', { targetId, isEnemy, statusType: brokenType, reason: 'damage_break' });
                broadcastToZone(boltZone, 'skill:buff_removed', { targetId, isEnemy, buffName: brokenType, reason: 'damage_break' });
            }

            // Summary event for skill effect tracking
            broadcastToZone(boltZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                damage: totalDamage, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });

            // Per-hit damage numbers (RO-style staggered display, ~200ms between hits)
            const HIT_DELAY_MS = 200;
            const boltZoneCaptured = boltZone; // Capture zone for setTimeout
            for (let h = 0; h < numHits; h++) {
                const hitDmg = hitDamages[h];
                const delay = h * HIT_DELAY_MS;
                setTimeout(() => {
                    broadcastToZone(boltZoneCaptured, 'skill:effect_damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId, targetName, isEnemy,
                        skillId, skillName: skill.displayName, skillLevel: learnedLevel, element: skill.element,
                        damage: hitDmg, isCritical: false, isMiss: false, hitType: 'magical',
                        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                        hitNumber: h + 1, totalHits: numHits,
                        targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
                    });
                }, delay);
            }

            if (isEnemy) {
                broadcastToZone(boltZone, 'enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                broadcastToZone(boltZone, 'combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                broadcastToZone(boltZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with ${skill.displayName}!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

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
            // RO AI: trigger aggro + assist on enemy skill hit
            if (isEnemy) {
                target.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(target, characterId, 'skill');
            }
            const ssZone = player.zone || 'prontera_south';
            logger.info(`[SKILL-COMBAT] ${player.characterName} Soul Strike Lv${learnedLevel} → ${targetName} for ${totalDamage} (${numHits} hits, ghost${undeadBonus > 1 ? '+undead' : ''}) (HP: ${target.health}/${target.maxHealth})`);

            broadcastToZone(ssZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: 'Soul Strike', skillLevel: learnedLevel, element: 'ghost',
                damage: totalDamage, hits: numHits, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });

            // Per-hit damage numbers (RO-style staggered display, ~200ms between hits)
            const SS_HIT_DELAY = 200;
            const ssZoneCaptured = ssZone; // Capture zone for setTimeout
            for (let h = 0; h < numHits; h++) {
                const hitDmg = hitDamages[h];
                const delay = h * SS_HIT_DELAY;
                setTimeout(() => {
                    broadcastToZone(ssZoneCaptured, 'skill:effect_damage', {
                        attackerId: characterId, attackerName: player.characterName,
                        targetId, targetName, isEnemy,
                        skillId, skillName: 'Soul Strike', skillLevel: learnedLevel, element: 'ghost',
                        damage: hitDmg, isCritical: false, isMiss: false, hitType: 'magical',
                        targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                        hitNumber: h + 1, totalHits: numHits,
                        targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
                    });
                }, delay);
            }

            if (isEnemy) {
                broadcastToZone(ssZone, 'enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                broadcastToZone(ssZone, 'combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                broadcastToZone(ssZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Soul Strike!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

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

            const nbZone = player.zone || 'prontera_south';
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
                // RO AI: AOE aggro
                if (st.isEnemy) {
                    st.target.lastDamageTime = Date.now();
                    if (typeof setEnemyAggro === 'function') setEnemyAggro(st.target, characterId, 'skill');
                }
                totalDamageDealt += dmg;

                broadcastToZone(nbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: st.id, targetName: st.name, isEnemy: st.isEnemy,
                    skillId, skillName: 'Napalm Beat', skillLevel: learnedLevel, element: 'ghost',
                    damage: dmg, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: st.pos.x, targetY: st.pos.y, targetZ: st.pos.z,
                    targetHealth: st.target.health, targetMaxHealth: st.target.maxHealth, timestamp: Date.now()
                });
                if (st.isEnemy) {
                    broadcastToZone(nbZone, 'enemy:health_update', { enemyId: st.id, health: st.target.health, maxHealth: st.target.maxHealth, inCombat: true });
                    if (st.target.health <= 0) await processEnemyDeathFromSkill(st.target, player, characterId, io);
                } else if (st.target.health <= 0) {
                    st.target.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === st.id && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    broadcastToZone(nbZone, 'combat:death', { killedId: st.id, killedName: st.name, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: st.target.maxHealth, timestamp: Date.now() });
                    broadcastToZone(nbZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${st.name} with Napalm Beat!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // 5x5 AoE = 500 UE units around target
            // Center 3x3 = full damage, outer ring = 75% damage (RO pre-renewal)
            const FIREBALL_AOE = 500;
            const FIREBALL_INNER = 300;  // 3x3 center radius
            const fbZone = player.zone || 'prontera_south';
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
                // RO AI: AOE aggro
                if (tIsEnemy) {
                    tgt.lastDamageTime = Date.now();
                    if (typeof setEnemyAggro === 'function') setEnemyAggro(tgt, characterId, 'skill');
                }
                totalDamageDealt += dmg;
                targetsHit++;

                // Damage breaks freeze/stone/sleep/confusion
                const fbBroken = checkDamageBreakStatuses(tgt);
                for (const brokenType of fbBroken) {
                    broadcastToZone(fbZone, 'status:removed', { targetId: tId, isEnemy: tIsEnemy, statusType: brokenType, reason: 'damage_break' });
                    broadcastToZone(fbZone, 'skill:buff_removed', { targetId: tId, isEnemy: tIsEnemy, buffName: brokenType, reason: 'damage_break' });
                }

                broadcastToZone(fbZone, 'skill:effect_damage', {
                    attackerId: characterId, attackerName: player.characterName,
                    targetId: tId, targetName: tName, isEnemy: tIsEnemy,
                    skillId, skillName: 'Fire Ball', skillLevel: learnedLevel, element: 'fire',
                    damage: dmg, isCritical: false, isMiss: false, hitType: 'magical',
                    targetX: tPos.x, targetY: tPos.y, targetZ: tPos.z,
                    targetHealth: tgt.health, targetMaxHealth: tgt.maxHealth, timestamp: Date.now(),
                    primaryTargetId: targetId, primaryTargetIsEnemy: isEnemy
                });
                if (tIsEnemy) {
                    broadcastToZone(fbZone, 'enemy:health_update', { enemyId: tId, health: tgt.health, maxHealth: tgt.maxHealth, inCombat: true });
                    if (tgt.health <= 0) await processEnemyDeathFromSkill(tgt, player, characterId, io);
                } else if (tgt.health <= 0) {
                    tgt.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === tId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    broadcastToZone(fbZone, 'combat:death', { killedId: tId, killedName: tName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: tgt.maxHealth, timestamp: Date.now() });
                    broadcastToZone(fbZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${tName} with Fire Ball!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            const STORM_AOE = 500; // 5x5 area
            const tsZone = player.zone || 'prontera_south';
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

                // RO AI: AOE aggro — each hit enemy aggros the caster
                enemy.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(enemy, characterId, 'skill');

                // Per-hit damage events with staggered delays (RO-style multi-hit display)
                const tsZoneCaptured = tsZone;
                const enemyCaptured = { id: eid, name: enemy.name, x: enemy.x, y: enemy.y, z: enemy.z, health: enemy.health, maxHealth: enemy.maxHealth };
                for (let h = 0; h < numHits; h++) {
                    const hitDmg = tsHitDamages[h];
                    const delay = h * TS_HIT_DELAY;
                    setTimeout(() => {
                        broadcastToZone(tsZoneCaptured, 'skill:effect_damage', {
                            attackerId: characterId, attackerName: player.characterName,
                            targetId: enemyCaptured.id, targetName: enemyCaptured.name, isEnemy: true,
                            skillId, skillName: 'Thunderstorm', skillLevel: learnedLevel, element: 'wind',
                            damage: hitDmg, isCritical: false, isMiss: false, hitType: 'magical',
                            targetX: enemyCaptured.x, targetY: enemyCaptured.y, targetZ: enemyCaptured.z,
                            hitNumber: h + 1, totalHits: numHits,
                            targetHealth: enemyCaptured.health, targetMaxHealth: enemyCaptured.maxHealth, timestamp: Date.now(),
                            groundX: centerPos.x, groundY: centerPos.y, groundZ: centerPos.z
                        });
                    }, delay);
                }
                broadcastToZone(tsZone, 'enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: true });
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

                // Per-hit damage events with staggered delays (RO-style multi-hit display)
                const tsZonePvp = tsZone;
                const pPosCaptured = { x: pPos.x, y: pPos.y, z: pPos.z };
                const ptargetCaptured = { id: pid, name: ptarget.characterName, health: ptarget.health, maxHealth: ptarget.maxHealth };
                for (let h = 0; h < numHits; h++) {
                    const hitDmg = pvpHitDmgs[h];
                    const delay = h * TS_HIT_DELAY;
                    setTimeout(() => {
                        broadcastToZone(tsZonePvp, 'skill:effect_damage', {
                            attackerId: characterId, attackerName: player.characterName,
                            targetId: ptargetCaptured.id, targetName: ptargetCaptured.name, isEnemy: false,
                            skillId, skillName: 'Thunderstorm', skillLevel: learnedLevel, element: 'wind',
                            damage: hitDmg, isCritical: false, isMiss: false, hitType: 'magical',
                            targetX: pPosCaptured.x, targetY: pPosCaptured.y, targetZ: pPosCaptured.z,
                            hitNumber: h + 1, totalHits: numHits,
                            targetHealth: ptargetCaptured.health, targetMaxHealth: ptargetCaptured.maxHealth, timestamp: Date.now(),
                            groundX: centerPos.x, groundY: centerPos.y, groundZ: centerPos.z
                        });
                    }, delay);
                }
                if (ptarget.health <= 0) {
                    ptarget.isDead = true;
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.targetCharId === pid && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                    }
                    broadcastToZone(tsZone, 'combat:death', { killedId: pid, killedName: ptarget.characterName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: ptarget.maxHealth, timestamp: Date.now() });
                    broadcastToZone(tsZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${ptarget.characterName} with Thunderstorm!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

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
            // RO AI: trigger aggro + assist on enemy skill hit
            if (isEnemy) {
                target.lastDamageTime = Date.now();
                if (typeof setEnemyAggro === 'function') setEnemyAggro(target, characterId, 'skill');
            }
            const fdZone = player.zone || 'prontera_south';
            logger.info(`[SKILL-COMBAT] ${player.characterName} Frost Diver Lv${learnedLevel} → ${targetName} for ${damage} [water] (HP: ${target.health}/${target.maxHealth})`);

            broadcastToZone(fdZone, 'skill:effect_damage', {
                attackerId: characterId, attackerName: player.characterName,
                targetId, targetName, isEnemy,
                skillId, skillName: 'Frost Diver', skillLevel: learnedLevel, element: 'water',
                damage, isCritical: false, isMiss: false, hitType: 'magical',
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                targetHealth: target.health, targetMaxHealth: target.maxHealth, timestamp: Date.now()
            });
            // Freeze chance: 35 + level*3, reduced by target MDEF (1% per point)
            // Uses generic status effect engine with resistance checks
            if (target.health > 0) {
                const freezeChance = Math.max(0, (35 + learnedLevel * 3) - targetHardMdef);
                const freezeDuration = duration || (learnedLevel * 3000);
                // Boss/undead immunity is handled inside applyStatusEffect via modeFlags
                const freezeSource = { characterId, level: player.stats.level || 1, stats: player.stats };
                const freezeResult = applyStatusEffect(freezeSource, target, 'freeze', freezeChance, freezeDuration);

                if (freezeResult.applied) {
                    logger.info(`[SKILL-COMBAT] ${targetName} FROZEN for ${freezeResult.duration / 1000}s by Frost Diver`);

                    broadcastToZone(fdZone, 'status:applied', {
                        targetId, isEnemy, statusType: 'freeze', duration: freezeResult.duration,
                        sourceId: characterId, sourceName: player.characterName
                    });
                    // Backward compat: also emit skill:buff_applied for VFX
                    broadcastToZone(fdZone, 'skill:buff_applied', {
                        targetId, targetName, isEnemy,
                        casterId: characterId, casterName: player.characterName,
                        skillId, buffName: 'Frozen', duration: freezeResult.duration,
                        effects: { frozen: true }
                    });
                } else {
                    logger.info(`[SKILL-COMBAT] Frost Diver freeze on ${targetName} — ${freezeResult.reason}`);
                }
            }

            if (isEnemy) {
                broadcastToZone(fdZone, 'enemy:health_update', { enemyId: targetId, health: target.health, maxHealth: target.maxHealth, inCombat: true });
                if (target.health <= 0) await processEnemyDeathFromSkill(target, player, characterId, io);
            } else if (target.health <= 0) {
                target.isDead = true;
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === targetId && !otherAtk.isEnemy) autoAttackState.delete(otherId);
                }
                broadcastToZone(fdZone, 'combat:death', { killedId: targetId, killedName: targetName, killerId: characterId, killerName: player.characterName, isEnemy: false, targetHealth: 0, targetMaxHealth: target.maxHealth, timestamp: Date.now() });
                broadcastToZone(fdZone, 'chat:receive', { type: 'chat:receive', channel: 'COMBAT', senderId: 0, senderName: 'SYSTEM', message: `${player.characterName} has slain ${targetName} with Frost Diver!`, timestamp: Date.now() });
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);
            applySkillDelays(characterId, player, skillId, levelData, socket);

            // Roll success rate: effectValue% (24-60%), reduced by target MDEF
            // Uses generic status effect engine with resistance checks
            const successRate = Math.max(0, effectVal - targetHardMdef);
            const stoneDuration = duration || 20000;
            const stoneSource = { characterId, level: player.stats.level || 1, stats: player.stats };
            const stoneResult = applyStatusEffect(stoneSource, target, 'stone', successRate, stoneDuration);
            const stoneApplied = stoneResult.applied;

            if (stoneResult.applied) {
                logger.info(`[SKILL-COMBAT] ${targetName} PETRIFIED for ${stoneResult.duration / 1000}s by Stone Curse (rate: ${successRate}%)`);

                const scZone = player.zone || 'prontera_south';
                broadcastToZone(scZone, 'status:applied', {
                    targetId, isEnemy, statusType: 'stone', duration: stoneResult.duration,
                    sourceId: characterId, sourceName: player.characterName
                });
                // Backward compat: also emit skill:buff_applied for VFX
                broadcastToZone(scZone, 'skill:buff_applied', {
                    targetId, targetName, isEnemy,
                    casterId: characterId, casterName: player.characterName,
                    skillId, buffName: 'Stone Curse', duration: stoneResult.duration,
                    effects: { petrified: true }
                });
            } else {
                logger.info(`[SKILL-COMBAT] Stone Curse on ${targetName} — ${stoneResult.reason}`);
            }

            socket.emit('skill:used', { skillId, skillName: 'Stone Curse', level: learnedLevel, spCost, remainingMana: player.mana, maxMana: player.maxMana, stoneApplied });
            // ACD + cooldown handled by applySkillDelays above
            socket.emit('combat:health_update', { characterId, health: player.health, maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana });
            return;
        }

        // --- SIGHT (ID 205) — Self buff, reveals hidden enemies ---
        if (skill.name === 'sight') {
            const casterBuffs = getBuffStatModifiers(player);
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

            player.mana = Math.max(0, player.mana - spCost);

            const sightDuration = duration || 10000;
            applyBuff(player, {
                skillId, name: 'sight', casterId: characterId, casterName: player.characterName,
                duration: sightDuration, defReduction: 0, atkIncrease: 0, mdefBonus: 0
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} SIGHT active for ${sightDuration / 1000}s`);

            const sightZone = player.zone || 'prontera_south';
            broadcastToZone(sightZone, 'skill:buff_applied', {
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

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
                if (removedId) { const fwRemoveZone = player.zone || 'prontera_south'; broadcastToZone(fwRemoveZone, 'skill:ground_effect_removed', { effectId: removedId, type: 'fire_wall', reason: 'max_limit' }); }
            }

            const wallDuration = duration || ((5 + learnedLevel - 1) * 1000);
            const hitLimit = effectVal; // skillLevel + 2 hits (3-12)

            const effectId = createGroundEffect({
                type: 'fire_wall', casterId: characterId, casterName: player.characterName,
                zone: player.zone || 'prontera_south',
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                hitLimit, hitsRemaining: hitLimit,
                duration: wallDuration, element: 'fire',
                radius: 150, // Fire wall collision radius (3 cells ≈ 150 UE units)
                knockbackTarget: 200 // Push enemies to this distance from center (> radius to exit wall)
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} Fire Wall Lv${learnedLevel}: placed at (${Math.floor(wallPos.x)}, ${Math.floor(wallPos.y)}), ${hitLimit} hits, ${wallDuration / 1000}s`);

            const fwCreatedZone = player.zone || 'prontera_south';
            broadcastToZone(fwCreatedZone, 'skill:ground_effect_created', {
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
            if (casterBuffs.preventsCasting) { socket.emit('skill:error', { message: 'Cannot cast while incapacitated' }); return; }

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
                zone: player.zone || 'prontera_south',
                x: wallPos.x, y: wallPos.y, z: wallPos.z || 0,
                hitsRemaining: hitsBlocked, hitLimit: hitsBlocked,
                duration: wallDuration,
                radius: 100 // 1x1 cell ≈ 100 UE units
            });

            logger.info(`[SKILL-COMBAT] ${player.characterName} Safety Wall Lv${learnedLevel}: placed at (${Math.floor(wallPos.x)}, ${Math.floor(wallPos.y)}), ${hitsBlocked} blocks, ${wallDuration / 1000}s`);

            const swCreatedZone = player.zone || 'prontera_south';
            broadcastToZone(swCreatedZone, 'skill:ground_effect_created', {
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

            // ═══════════════════════════════════════════════════
            // Special items: Fly Wing + Butterfly Wing
            // ═══════════════════════════════════════════════════

            // Fly Wing (item_id 1029) — random teleport within same zone
            if (item.item_id === 1029) {
                const playerZone = player.zone || 'prontera_south';
                const zoneData = getZone(playerZone);
                if (zoneData && zoneData.flags && zoneData.flags.noteleport) {
                    socket.emit('inventory:error', { message: 'Teleportation is blocked in this area.' });
                    return;
                }
                // Consume 1 item
                await removeItemFromInventory(inventoryId, 1);
                // Random position near defaultSpawn (±2000 UE units)
                const spawn = (zoneData && zoneData.defaultSpawn) || { x: 0, y: 0, z: 300 };
                const newX = spawn.x + (Math.random() * 4000 - 2000);
                const newY = spawn.y + (Math.random() * 4000 - 2000);
                const newZ = spawn.z;
                player.lastX = newX; player.lastY = newY; player.lastZ = newZ;
                broadcastToZone(playerZone, 'player:teleport', {
                    characterId, x: newX, y: newY, z: newZ, teleportType: 'fly_wing'
                });
                socket.emit('inventory:used', {
                    inventoryId, itemId: item.item_id, itemName: item.name,
                    healed: 0, spRestored: 0,
                    health: player.health, maxHealth: player.maxHealth,
                    mana: player.mana, maxMana: player.maxMana
                });
                const invFW = await getPlayerInventory(characterId);
                socket.emit('inventory:data', { items: invFW, zuzucoin: player.zuzucoin });
                logger.info(`[ITEMS] ${player.characterName} used Fly Wing → (${Math.round(newX)}, ${Math.round(newY)}, ${newZ})`);
                return;
            }

            // Butterfly Wing (item_id 1028) — teleport to save point
            if (item.item_id === 1028) {
                const playerZone = player.zone || 'prontera_south';
                const zoneData = getZone(playerZone);
                if (zoneData && zoneData.flags && zoneData.flags.noreturn) {
                    socket.emit('inventory:error', { message: 'Return is blocked in this area.' });
                    return;
                }
                // Consume 1 item
                await removeItemFromInventory(inventoryId, 1);
                // Query save point from DB
                const saveRes = await pool.query(
                    `SELECT save_map, save_x, save_y, save_z FROM characters WHERE character_id = $1`,
                    [characterId]
                );
                const saveRow = saveRes.rows[0] || {};
                const saveMap = saveRow.save_map || 'prontera';
                const saveX = parseFloat(saveRow.save_x) || 0;
                const saveY = parseFloat(saveRow.save_y) || 0;
                const saveZ = parseFloat(saveRow.save_z) || 580;

                if (saveMap !== playerZone) {
                    // Cross-zone teleport — full zone change flow
                    const destZoneData = getZone(saveMap);
                    const destLevelName = destZoneData ? destZoneData.levelName : 'L_PrtSouth';
                    const destDisplayName = destZoneData ? destZoneData.displayName : saveMap;

                    // Cancel combat
                    if (player.autoAttackTarget) {
                        player.autoAttackTarget = null;
                        player.isAutoAttacking = false;
                    }
                    // Leave old zone room, join new
                    socket.leave('zone:' + playerZone);
                    broadcastToZone(playerZone, 'player:left', { characterId, reason: 'butterfly_wing' });
                    player.zone = saveMap;
                    player.lastX = saveX; player.lastY = saveY; player.lastZ = saveZ;
                    socket.join('zone:' + saveMap);
                    zoneTransitioning.add(characterId);
                    // Save to DB
                    await pool.query(
                        `UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5`,
                        [saveMap, saveX, saveY, saveZ, characterId]
                    );
                    // Lazy spawn enemies in destination
                    if (!spawnedZones.has(saveMap) && destZoneData) {
                        destZoneData.enemySpawns.forEach(sc => spawnEnemy({ ...sc, zone: saveMap }));
                        spawnedZones.add(saveMap);
                        logger.info(`[ZONE] Lazy-spawned enemies for ${saveMap}`);
                    }
                    // Emit zone:change
                    socket.emit('zone:change', {
                        zone: saveMap, displayName: destDisplayName, levelName: destLevelName,
                        x: saveX, y: saveY, z: saveZ,
                        flags: destZoneData ? destZoneData.flags : {}
                    });
                } else {
                    // Same zone — in-zone teleport
                    player.lastX = saveX; player.lastY = saveY; player.lastZ = saveZ;
                    broadcastToZone(playerZone, 'player:teleport', {
                        characterId, x: saveX, y: saveY, z: saveZ, teleportType: 'butterfly_wing'
                    });
                }
                socket.emit('inventory:used', {
                    inventoryId, itemId: item.item_id, itemName: item.name,
                    healed: 0, spRestored: 0,
                    health: player.health, maxHealth: player.maxHealth,
                    mana: player.mana, maxMana: player.maxMana
                });
                const invBW = await getPlayerInventory(characterId);
                socket.emit('inventory:data', { items: invBW, zuzucoin: player.zuzucoin });
                logger.info(`[ITEMS] ${player.characterName} used Butterfly Wing → ${saveMap} (${Math.round(saveX)}, ${Math.round(saveY)})`);
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
            
            // Send updated health to zone
            const useItemZone = player.zone || 'prontera_south';
            broadcastToZone(useItemZone, 'combat:health_update', {
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
            
            // Update maxHP/maxSP from derived stats and cap current values
            player.maxHealth = derived.maxHP;
            player.maxMana = derived.maxSP;
            player.health = Math.min(player.health, player.maxHealth);
            player.mana = Math.min(player.mana, player.maxMana);
            try {
                await pool.query(
                    'UPDATE characters SET max_health = $1, max_mana = $2, health = $3, mana = $4 WHERE character_id = $5',
                    [player.maxHealth, player.maxMana, player.health, player.mana, characterId]
                );
            } catch (err) {
                logger.warn(`[DB] Could not update max_health/max_mana for char ${characterId}: ${err.message}`);
            }
            
            // Send updated stats (effective values so UI shows total stats including equipment)
            const finalAspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            socket.emit('player:stats', buildFullStatsPayload(characterId, player, effectiveStats, derived, finalAspd));
            
            // Broadcast updated maxHealth so other clients in zone show correct HP bars
            const equipZone = player.zone || 'prontera_south';
            broadcastToZone(equipZone, 'combat:health_update', {
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

    // Open shop — returns shop inventory with Discount/Overcharge-adjusted prices,
    // player's zuzucoin, weight info, and inventory slot info
    socket.on('shop:open', async (data) => {
        logger.info(`[RECV] shop:open from ${socket.id}: ${JSON.stringify(data)}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const shopId = parseInt(data.shopId);
        const shop = NPC_SHOPS[shopId];
        if (!shop) {
            socket.emit('shop:error', { message: 'Shop not found' });
            return;
        }

        const discountPct = getDiscountPercent(player);
        const overchargePct = getOverchargePercent(player);

        const shopItems = shop.itemIds
            .map(id => itemDefinitions.get(id))
            .filter(Boolean)
            .map(item => ({
                itemId: item.item_id,
                name: item.name,
                description: item.description,
                itemType: item.item_type,
                equipSlot: item.equip_slot || '',
                buyPrice: applyDiscount(item.price * 2, discountPct),
                sellPrice: applyOvercharge(item.price, overchargePct),
                weight: item.weight || 0,
                icon: item.icon,
                atk: item.atk || 0,
                def: item.def || 0,
                matk: item.matk || 0,
                mdef: item.mdef || 0,
                strBonus: item.str_bonus || 0,
                agiBonus: item.agi_bonus || 0,
                vitBonus: item.vit_bonus || 0,
                intBonus: item.int_bonus || 0,
                dexBonus: item.dex_bonus || 0,
                lukBonus: item.luk_bonus || 0,
                maxHpBonus: item.max_hp_bonus || 0,
                maxSpBonus: item.max_sp_bonus || 0,
                weaponType: item.weapon_type || '',
                weaponRange: item.weapon_range || 0,
                aspdModifier: item.aspd_modifier || 0,
                requiredLevel: item.required_level || 1,
                stackable: item.stackable || false
            }));

        // Calculate current inventory weight
        let currentWeight = 0;
        try {
            const invW = await pool.query(
                `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as total_weight
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1`, [characterId]
            );
            currentWeight = parseInt(invW.rows[0].total_weight);
        } catch (err) {
            logger.warn(`[SHOP] Weight query failed: ${err.message}`);
        }

        // Count used inventory slots
        let usedSlots = 0;
        try {
            const slotR = await pool.query(
                'SELECT COUNT(*) as c FROM character_inventory WHERE character_id = $1', [characterId]
            );
            usedSlots = parseInt(slotR.rows[0].c);
        } catch (err) {
            logger.warn(`[SHOP] Slot query failed: ${err.message}`);
        }

        const maxWeight = getPlayerMaxWeight(player);

        socket.emit('shop:data', {
            shopId,
            shopName: shop.name,
            items: shopItems,
            playerZuzucoin: player.zuzucoin,
            discountPercent: discountPct,
            overchargePercent: overchargePct,
            currentWeight,
            maxWeight,
            usedSlots,
            maxSlots: INVENTORY.MAX_SLOTS
        });
        logger.info(`[SEND] shop:data to ${socket.id}: shop=${shop.name} items=${shopItems.length} zuzucoin=${player.zuzucoin} disc=${discountPct}% oc=${overchargePct}%`);
    });

    // DEPRECATED: Single-item buy. Use shop:buy_batch for batch purchases with Discount support.
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

        const client = await pool.connect();
        try {
            await client.query('BEGIN');
            const newZeny = player.zuzucoin - totalCost;
            await client.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);

            const added = await addItemToInventory(characterId, itemId, quantity, client);
            if (!added) {
                await client.query('ROLLBACK');
                socket.emit('shop:error', { message: 'Failed to add item to inventory' });
                return;
            }

            await client.query('COMMIT');
            player.zuzucoin = newZeny;

            logger.info(`[SHOP] ${player.characterName} bought ${quantity}x ${itemDef.name} for ${totalCost}z (remaining: ${newZeny}z)`);

            const inventory = await getPlayerInventory(characterId);
            socket.emit('shop:bought', { itemId, itemName: itemDef.name, quantity, totalCost, newZuzucoin: newZeny });
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            logger.info(`[SEND] shop:bought + inventory:data to ${socket.id}`);
        } catch (err) {
            await client.query('ROLLBACK').catch(() => {});
            logger.error(`[SHOP] Buy error for char ${characterId}: ${err.message}`);
            socket.emit('shop:error', { message: 'Purchase failed' });
        } finally {
            client.release();
        }
    });

    // DEPRECATED: Single-item sell. Use shop:sell_batch for batch sales with Overcharge support.
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

            const client = await pool.connect();
            try {
                await client.query('BEGIN');
                await client.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);
                await removeItemFromInventory(inventoryId, sellQty, client);
                await client.query('COMMIT');
                player.zuzucoin = newZeny;
            } catch (txErr) {
                await client.query('ROLLBACK').catch(() => {});
                client.release();
                logger.error(`[SHOP] Sell transaction error for char ${characterId}: ${txErr.message}`);
                socket.emit('shop:error', { message: 'Sale failed' });
                return;
            }
            client.release();

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

    // ============================================================
    // Batch Buy — RO Classic shopping cart (buy multiple items at once)
    // Validates entire cart atomically: zeny, weight, inventory slots
    // ============================================================
    socket.on('shop:buy_batch', async (data) => {
        logger.info(`[RECV] shop:buy_batch from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const shopId = parseInt(data.shopId);
        const cart = data.cart; // Array of { itemId, quantity }

        if (!Array.isArray(cart) || cart.length === 0) {
            socket.emit('shop:error', { code: 'invalid_cart', message: 'Empty cart' });
            return;
        }

        const shop = NPC_SHOPS[shopId];
        if (!shop) {
            socket.emit('shop:error', { code: 'invalid_shop', message: 'Shop not found' });
            return;
        }

        const discountPct = getDiscountPercent(player);

        // Pre-validate all items and calculate totals
        let totalCost = 0;
        let totalWeight = 0;
        const validatedItems = [];
        const stackableItemIds = [];

        for (const entry of cart) {
            const itemId = parseInt(entry.itemId);
            const quantity = Math.max(1, parseInt(entry.quantity) || 1);

            if (!shop.itemIds.includes(itemId)) {
                socket.emit('shop:error', { code: 'invalid_item', message: `Item not available in this shop` });
                return;
            }
            const itemDef = itemDefinitions.get(itemId);
            if (!itemDef) {
                socket.emit('shop:error', { code: 'invalid_item', message: `Item not found` });
                return;
            }
            if (itemDef.required_level > (player.stats?.level || 1)) {
                socket.emit('shop:error', { code: 'level_req', message: `${itemDef.name} requires level ${itemDef.required_level}` });
                return;
            }

            const unitPrice = applyDiscount(itemDef.price * 2, discountPct);
            totalCost += unitPrice * quantity;
            totalWeight += (itemDef.weight || 0) * quantity;

            if (itemDef.stackable) {
                stackableItemIds.push(itemId);
            }

            validatedItems.push({ itemId, quantity, itemDef, unitPrice });
        }

        // 1. Check zeny
        if (player.zuzucoin < totalCost) {
            socket.emit('shop:error', {
                code: 'no_zeny',
                message: `Not enough Zeny (need ${totalCost.toLocaleString()}, have ${player.zuzucoin.toLocaleString()})`
            });
            return;
        }

        // 2. Check weight
        let currentWeight = 0;
        try {
            const invW = await pool.query(
                `SELECT COALESCE(SUM(ci.quantity * i.weight), 0) as w
                 FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                 WHERE ci.character_id = $1`, [characterId]
            );
            currentWeight = parseInt(invW.rows[0].w);
        } catch (err) { /* weight query failed, skip check */ }

        const maxWeight = getPlayerMaxWeight(player);
        if (currentWeight + totalWeight > maxWeight) {
            socket.emit('shop:error', {
                code: 'overweight',
                message: `Would exceed weight limit (${currentWeight + totalWeight} / ${maxWeight})`
            });
            return;
        }

        // 3. Check inventory slots — count how many NEW slots are needed
        let newSlotCount = 0;
        try {
            // Find which stackable items already exist in inventory
            let existingStackIds = new Set();
            if (stackableItemIds.length > 0) {
                const existResult = await pool.query(
                    'SELECT DISTINCT item_id FROM character_inventory WHERE character_id = $1 AND item_id = ANY($2) AND is_equipped = false',
                    [characterId, stackableItemIds]
                );
                existingStackIds = new Set(existResult.rows.map(r => r.item_id));
            }

            for (const { itemId, quantity, itemDef } of validatedItems) {
                if (itemDef.stackable && existingStackIds.has(itemId)) {
                    // Will stack into existing slot, no new slot needed
                } else if (itemDef.stackable) {
                    newSlotCount += 1; // New stack = 1 slot
                    existingStackIds.add(itemId); // Subsequent same items will stack
                } else {
                    newSlotCount += quantity; // Non-stackable = 1 slot per item
                }
            }

            const slotR = await pool.query(
                'SELECT COUNT(*) as c FROM character_inventory WHERE character_id = $1', [characterId]
            );
            const usedSlots = parseInt(slotR.rows[0].c);
            if (usedSlots + newSlotCount > INVENTORY.MAX_SLOTS) {
                socket.emit('shop:error', {
                    code: 'no_slots',
                    message: `Not enough inventory space (${usedSlots}/${INVENTORY.MAX_SLOTS} slots used)`
                });
                return;
            }
        } catch (err) {
            logger.warn(`[SHOP] Slot validation error: ${err.message}`);
        }

        // All validations passed — execute the batch transaction atomically
        const client = await pool.connect();
        try {
            await client.query('BEGIN');
            const newZeny = player.zuzucoin - totalCost;
            await client.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);

            const boughtItems = [];
            for (const { itemId, quantity, itemDef, unitPrice } of validatedItems) {
                const added = await addItemToInventory(characterId, itemId, quantity, client);
                if (added) {
                    boughtItems.push({ itemId, itemName: itemDef.name, quantity, unitPrice, totalPrice: unitPrice * quantity });
                }
            }

            await client.query('COMMIT');
            player.zuzucoin = newZeny;

            logger.info(`[SHOP] ${player.characterName} batch-bought: ${boughtItems.map(i => `${i.quantity}x ${i.itemName}`).join(', ')} for ${totalCost}z (remaining: ${newZeny}z)`);

            const inventory = await getPlayerInventory(characterId);
            socket.emit('shop:bought', { items: boughtItems, totalCost, newZuzucoin: newZeny });
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            logger.info(`[SEND] shop:bought + inventory:data to ${socket.id}`);
        } catch (err) {
            await client.query('ROLLBACK').catch(() => {});
            logger.error(`[SHOP] Batch buy error for char ${characterId}: ${err.message}`);
            socket.emit('shop:error', { code: 'server_error', message: 'Purchase failed' });
        } finally {
            client.release();
        }
    });

    // ============================================================
    // Batch Sell — RO Classic sell cart (sell multiple items at once)
    // Validates ownership, equipped status, quantities, zeny overflow
    // ============================================================
    socket.on('shop:sell_batch', async (data) => {
        logger.info(`[RECV] shop:sell_batch from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;

        const { characterId, player } = playerInfo;
        const cart = data.cart; // Array of { inventoryId, quantity }

        if (!Array.isArray(cart) || cart.length === 0) {
            socket.emit('shop:error', { code: 'invalid_cart', message: 'Empty sell cart' });
            return;
        }

        const overchargePct = getOverchargePercent(player);

        // Pre-validate all items
        let totalRevenue = 0;
        const validatedItems = [];

        for (const entry of cart) {
            const inventoryId = parseInt(entry.inventoryId);
            const quantity = Math.max(1, parseInt(entry.quantity) || 1);

            if (!inventoryId || isNaN(inventoryId)) {
                socket.emit('shop:error', { code: 'invalid_item', message: 'Invalid inventory ID' });
                return;
            }

            try {
                const result = await pool.query(
                    `SELECT ci.inventory_id, ci.item_id, ci.quantity, ci.is_equipped,
                            i.name, i.price, i.stackable
                     FROM character_inventory ci JOIN items i ON ci.item_id = i.item_id
                     WHERE ci.inventory_id = $1 AND ci.character_id = $2`,
                    [inventoryId, characterId]
                );

                if (result.rows.length === 0) {
                    socket.emit('shop:error', { code: 'invalid_item', message: 'Item not found in your inventory' });
                    return;
                }

                const item = result.rows[0];

                if (item.is_equipped) {
                    socket.emit('shop:error', { code: 'equipped', message: `Cannot sell equipped item: ${item.name}. Unequip it first.` });
                    return;
                }

                if ((item.price || 0) <= 0) {
                    socket.emit('shop:error', { code: 'unsellable', message: `${item.name} cannot be sold` });
                    return;
                }

                const sellQty = Math.min(quantity, item.quantity);
                const unitSellPrice = applyOvercharge(item.price || 0, overchargePct);
                totalRevenue += unitSellPrice * sellQty;

                validatedItems.push({ inventoryId, sellQty, item, unitSellPrice });
            } catch (err) {
                logger.error(`[SHOP] Sell validation error: ${err.message}`);
                socket.emit('shop:error', { code: 'server_error', message: 'Validation failed' });
                return;
            }
        }

        // Check zeny overflow (max 2^31 - 1)
        const MAX_ZENY = 2147483647;
        if (player.zuzucoin + totalRevenue > MAX_ZENY) {
            socket.emit('shop:error', { code: 'zeny_overflow', message: 'Cannot hold more Zeny (max reached)' });
            return;
        }

        // Execute the batch sale atomically
        const client = await pool.connect();
        try {
            await client.query('BEGIN');
            const newZeny = player.zuzucoin + totalRevenue;
            await client.query('UPDATE characters SET zuzucoin = $1 WHERE character_id = $2', [newZeny, characterId]);

            const soldItems = [];
            for (const { inventoryId, sellQty, item, unitSellPrice } of validatedItems) {
                await removeItemFromInventory(inventoryId, sellQty, client);
                soldItems.push({
                    inventoryId,
                    itemName: item.name,
                    quantity: sellQty,
                    unitPrice: unitSellPrice,
                    totalPrice: unitSellPrice * sellQty
                });
            }

            await client.query('COMMIT');
            player.zuzucoin = newZeny;

            logger.info(`[SHOP] ${player.characterName} batch-sold: ${soldItems.map(i => `${i.quantity}x ${i.itemName}`).join(', ')} for ${totalRevenue}z (total: ${newZeny}z)`);

            const inventory = await getPlayerInventory(characterId);
            socket.emit('shop:sold', { items: soldItems, totalRevenue, newZuzucoin: newZeny });
            socket.emit('inventory:data', { items: inventory, zuzucoin: player.zuzucoin });
            logger.info(`[SEND] shop:sold + inventory:data to ${socket.id}`);
        } catch (err) {
            await client.query('ROLLBACK').catch(() => {});
            logger.error(`[SHOP] Batch sell error for char ${characterId}: ${err.message}`);
            socket.emit('shop:error', { code: 'server_error', message: 'Sale failed' });
        } finally {
            client.release();
        }
    });

    // ============================================================
    // Debug Commands — Status Effect Testing (dev only)
    // ============================================================
    socket.on('debug:apply_status', (data) => {
        if (process.env.NODE_ENV !== 'development') return;
        try {
            const parsed = typeof data === 'string' ? JSON.parse(data) : data;
            const { statusType, targetId, isEnemy, duration } = parsed;
            if (!statusType || !STATUS_EFFECTS[statusType]) {
                socket.emit('debug:status_applied', { success: false, reason: 'unknown_status', statusType });
                return;
            }
            const target = isEnemy ? enemies.get(parseInt(targetId)) : connectedPlayers.get(parseInt(targetId));
            if (!target) {
                socket.emit('debug:status_applied', { success: false, reason: 'target_not_found' });
                return;
            }
            const effectDuration = duration || STATUS_EFFECTS[statusType].baseDuration;
            const result = forceApplyStatusEffect(target, statusType, effectDuration);
            if (result.applied) {
                const targetZone = target.zone || 'prontera_south';
                broadcastToZone(targetZone, 'status:applied', {
                    targetId: parseInt(targetId), isEnemy: !!isEnemy, statusType,
                    duration: result.duration, sourceId: 0, sourceName: 'DEBUG'
                });
                // Backward compat for VFX
                if (statusType === 'freeze') {
                    broadcastToZone(targetZone, 'skill:buff_applied', {
                        targetId: parseInt(targetId), isEnemy: !!isEnemy,
                        skillId: 208, buffName: 'Frozen', duration: result.duration,
                        effects: { frozen: true }
                    });
                } else if (statusType === 'stone') {
                    broadcastToZone(targetZone, 'skill:buff_applied', {
                        targetId: parseInt(targetId), isEnemy: !!isEnemy,
                        skillId: 206, buffName: 'Stone Curse', duration: result.duration,
                        effects: { petrified: true }
                    });
                }
            }
            socket.emit('debug:status_applied', { success: result.applied, statusType, targetId: parseInt(targetId), duration: result.duration });
            logger.info(`[DEBUG] Applied ${statusType} to ${isEnemy ? 'enemy' : 'player'} ${targetId} for ${result.duration}ms`);
        } catch (err) {
            logger.error(`[DEBUG] apply_status error: ${err.message}`);
            socket.emit('debug:status_applied', { success: false, reason: err.message });
        }
    });

    socket.on('debug:remove_status', (data) => {
        if (process.env.NODE_ENV !== 'development') return;
        try {
            const parsed = typeof data === 'string' ? JSON.parse(data) : data;
            const { statusType, targetId, isEnemy } = parsed;
            const target = isEnemy ? enemies.get(parseInt(targetId)) : connectedPlayers.get(parseInt(targetId));
            if (!target) return;
            const removed = removeStatusEffect(target, statusType);
            if (removed) {
                const targetZone = target.zone || 'prontera_south';
                broadcastToZone(targetZone, 'status:removed', {
                    targetId: parseInt(targetId), isEnemy: !!isEnemy, statusType, reason: 'debug'
                });
                broadcastToZone(targetZone, 'skill:buff_removed', {
                    targetId: parseInt(targetId), isEnemy: !!isEnemy, buffName: statusType, reason: 'debug'
                });
            }
            socket.emit('debug:status_removed', { success: removed, statusType, targetId: parseInt(targetId) });
            logger.info(`[DEBUG] Removed ${statusType} from ${isEnemy ? 'enemy' : 'player'} ${targetId}: ${removed}`);
        } catch (err) {
            logger.error(`[DEBUG] remove_status error: ${err.message}`);
        }
    });

    socket.on('debug:list_statuses', (data) => {
        if (process.env.NODE_ENV !== 'development') return;
        try {
            const parsed = typeof data === 'string' ? JSON.parse(data) : data;
            const { targetId, isEnemy } = parsed;
            const target = isEnemy ? enemies.get(parseInt(targetId)) : connectedPlayers.get(parseInt(targetId));
            if (!target) {
                socket.emit('debug:status_list', { targetId: parseInt(targetId), statuses: [], buffs: [] });
                return;
            }
            socket.emit('debug:status_list', {
                targetId: parseInt(targetId),
                statuses: getActiveStatusList(target),
                buffs: getActiveBuffList(target)
            });
        } catch (err) {
            logger.error(`[DEBUG] list_statuses error: ${err.message}`);
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

    // Broadcast cast complete to zone clients (removes cast bar)
    const castCompletePlayer = connectedPlayers.get(characterId);
    const castCompleteZone = (castCompletePlayer && castCompletePlayer.zone) || 'prontera_south';
    broadcastToZone(castCompleteZone, 'skill:cast_complete', { casterId: characterId, skillId: cast.skillId });

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

        // Skip if attacker is CC'd (stun/freeze/stone/sleep prevent attacks)
        const attackerCCMods = getCombinedModifiers(attacker);
        if (attackerCCMods.preventsAttack) continue;
        
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
                    const swBlockZone = enemy.zone || attacker.zone || 'prontera_south';
                    logger.info(`[COMBAT] Safety Wall blocked melee attack on ${enemy.name} (${safetyWall.hitsRemaining} hits remaining)`);
                    broadcastToZone(swBlockZone, 'skill:ground_effect_blocked', { effectId: safetyWall.id, type: 'safety_wall', hitsRemaining: safetyWall.hitsRemaining, targetId: enemy.enemyId, targetName: enemy.name, isEnemy: true });
                    if (safetyWall.hitsRemaining <= 0) {
                        removeGroundEffect(safetyWall.id);
                        broadcastToZone(swBlockZone, 'skill:ground_effect_removed', { effectId: safetyWall.id, type: 'safety_wall', reason: 'hits_exhausted' });
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

                const atkEnemyZone = enemy.zone || attacker.zone || 'prontera_south';
                if (isMiss) {
                    // Miss/Dodge: don't deal damage, still broadcast event
                    broadcastToZone(atkEnemyZone, 'combat:damage', damagePayload);
                    logger.info(`[COMBAT] ${attacker.characterName} ${hitType} against ${enemy.name}(${enemy.enemyId})`);
                    continue;
                }

                // Apply damage
                enemy.health = Math.max(0, enemy.health - damage);
                damagePayload.targetHealth = enemy.health;

                // RO AI: Record hit stun time and trigger aggro/assist
                enemy.lastDamageTime = now;
                if (typeof setEnemyAggro === 'function') {
                    setEnemyAggro(enemy, attackerId, 'melee');
                }

                logger.info(`[COMBAT] ${attacker.characterName} hit enemy ${enemy.name}(${enemy.enemyId}) for ${damage}${isCritical ? ' CRIT' : ''} [${atkElement}→${(enemy.element||{}).type||'neutral'}] (HP: ${enemy.health}/${enemy.maxHealth})`);

                broadcastToZone(atkEnemyZone, 'combat:damage', damagePayload);

                // Any damage breaks freeze/stone/sleep/confusion
                const aaEnemyBroken = checkDamageBreakStatuses(enemy);
                for (const brokenType of aaEnemyBroken) {
                    broadcastToZone(atkEnemyZone, 'status:removed', { targetId: enemy.enemyId, isEnemy: true, statusType: brokenType, reason: 'damage_break' });
                    broadcastToZone(atkEnemyZone, 'skill:buff_removed', { targetId: enemy.enemyId, isEnemy: true, buffName: brokenType, reason: 'damage_break' });
                    logger.info(`[COMBAT] Auto-attack broke ${brokenType} on ${enemy.name}`);
                }

                // Broadcast enemy health update to zone (for health bar visibility)
                broadcastToZone(atkEnemyZone, 'enemy:health_update', {
                    enemyId: enemy.enemyId,
                    health: enemy.health,
                    maxHealth: enemy.maxHealth,
                    inCombat: enemy.inCombatWith.size > 0
                });

                // Enemy death
                if (enemy.health <= 0) {
                    enemy.isDead = true;
                    enemy.aiState = AI_STATE.DEAD;
                    enemy.targetPlayerId = null;

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
                            // Broadcast to other players in zone so they see the level up
                            const lvlUpZone = attacker.zone || 'prontera_south';
                            broadcastToZoneExcept(killerSocket, lvlUpZone, 'exp:level_up', levelUpPayload);

                            // Send updated stats (derived stats change with level)
                            const effectiveStats = getEffectiveStats(attacker);
                            const newDerived = calculateDerivedStats(effectiveStats);
                            const newFinalAspd = Math.min(COMBAT.ASPD_CAP, newDerived.aspd + (attacker.weaponAspdMod || 0));
                            killerSocket.emit('player:stats', buildFullStatsPayload(attackerId, attacker, effectiveStats, newDerived, newFinalAspd));

                            // Broadcast health update (full heal on level up)
                            broadcastToZone(lvlUpZone, 'combat:health_update', {
                                characterId: attackerId,
                                health: attacker.health,
                                maxHealth: attacker.maxHealth,
                                mana: attacker.mana,
                                maxMana: attacker.maxMana
                            });

                            // Chat announcement for level ups (SYSTEM = global)
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
                    const enemyDeathZone = enemy.zone || attacker.zone || 'prontera_south';
                    broadcastToZone(enemyDeathZone, 'enemy:death', deathPayload);
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
                    const respawnEnemyZone = enemy.zone || 'prontera_south'; // Capture zone for setTimeout
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
                        broadcastToZone(respawnEnemyZone, 'enemy:spawn', {
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
                const pvpSwZone = attacker.zone || 'prontera_south';
                logger.info(`[COMBAT] Safety Wall blocked melee attack on ${target.characterName} (${pvpSafetyWall.hitsRemaining} hits remaining)`);
                broadcastToZone(pvpSwZone, 'skill:ground_effect_blocked', { effectId: pvpSafetyWall.id, type: 'safety_wall', hitsRemaining: pvpSafetyWall.hitsRemaining, targetId: atkState.targetCharId, targetName: target.characterName, isEnemy: false });
                if (pvpSafetyWall.hitsRemaining <= 0) {
                    removeGroundEffect(pvpSafetyWall.id);
                    broadcastToZone(pvpSwZone, 'skill:ground_effect_removed', { effectId: pvpSafetyWall.id, type: 'safety_wall', reason: 'hits_exhausted' });
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

            const pvpAtkZone = attacker.zone || 'prontera_south';
            if (pvpMiss) {
                broadcastToZone(pvpAtkZone, 'combat:damage', damagePayload);
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

            broadcastToZone(pvpAtkZone, 'combat:damage', damagePayload);

            // Any damage breaks freeze/stone/sleep/confusion on PvP target
            const pvpBroken = checkDamageBreakStatuses(target);
            for (const brokenType of pvpBroken) {
                broadcastToZone(pvpAtkZone, 'status:removed', { targetId: atkState.targetCharId, isEnemy: false, statusType: brokenType, reason: 'damage_break' });
                broadcastToZone(pvpAtkZone, 'skill:buff_removed', { targetId: atkState.targetCharId, isEnemy: false, buffName: brokenType, reason: 'damage_break' });
                logger.info(`[COMBAT] Auto-attack broke ${brokenType} on ${target.characterName}`);
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
                broadcastToZone(pvpAtkZone, 'combat:death', deathPayload);
                logger.info(`[BROADCAST] combat:death: ${JSON.stringify(deathPayload)}`);

                // Kill message in chat
                broadcastToZone(pvpAtkZone, 'chat:receive', {
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
// Blocked by: Bleeding status (blocksHPRegen)
setInterval(() => {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if (player.health >= player.maxHealth) continue;
        // Status effects can block HP regen (bleeding)
        const regenMods = getCombinedModifiers(player);
        if (regenMods.blocksHPRegen) continue;

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
// Blocked by: Poison status (blocksSPRegen), Bleeding status (blocksSPRegen)
setInterval(() => {
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        if (player.mana >= player.maxMana) continue;
        // Status effects can block SP regen (poison, bleeding)
        const spRegenMods = getCombinedModifiers(player);
        if (spRegenMods.blocksSPRegen) continue;

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
// Status Effect & Buff Expiry Tick (every 1 second)
// Handles: status effect expiry, periodic drains (poison/bleeding/stone), buff expiry
// ============================================================
setInterval(() => {
    const now = Date.now();

    // --- Player status effects + buffs ---
    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        const playerZone = player.zone || 'prontera_south';
        const sock = io.sockets.sockets.get(player.socketId);

        // Tick status effects (handles expiry + periodic drains)
        const { expired: statusExpired, drains: statusDrains } = tickStatusEffects(player, now);
        for (const type of statusExpired) {
            logger.info(`[STATUS] ${player.characterName}: ${type} expired`);
            broadcastToZone(playerZone, 'status:removed', { targetId: charId, isEnemy: false, statusType: type, reason: 'expired' });
            // Backward compat: also emit skill:buff_removed for VFX
            if (sock) sock.emit('skill:buff_removed', { targetId: charId, buffName: type, reason: 'expired' });
        }
        for (const d of statusDrains) {
            logger.info(`[STATUS] ${d.type} drained ${d.drain} HP from ${player.characterName} (HP: ${d.newHealth}/${player.maxHealth})`);
            if (sock) {
                sock.emit('combat:health_update', {
                    characterId: charId, health: d.newHealth,
                    maxHealth: player.maxHealth, mana: player.mana, maxMana: player.maxMana
                });
            }
            broadcastToZone(playerZone, 'status:tick', {
                targetId: charId, isEnemy: false, statusType: d.type,
                drain: d.drain, newHealth: d.newHealth, maxHealth: player.maxHealth
            });
        }

        // Tick buffs (Provoke, Endure, Sight, etc.)
        const expiredBuffs = expireBuffs(player);
        for (const buff of expiredBuffs) {
            logger.info(`[BUFF] ${player.characterName}: ${buff.name} expired`);
            if (sock) {
                sock.emit('skill:buff_removed', {
                    targetId: charId, buffName: buff.name, skillId: buff.skillId, reason: 'expired'
                });
            }
            broadcastToZone(playerZone, 'buff:removed', {
                targetId: charId, isEnemy: false, buffName: buff.name, reason: 'expired'
            });
        }
    }

    // --- Enemy status effects + buffs ---
    for (const [eid, enemy] of enemies.entries()) {
        if (enemy.isDead) continue;
        const enemyZone = enemy.zone || 'prontera_south';

        // Tick status effects
        const { expired: eStatusExpired, drains: eStatusDrains } = tickStatusEffects(enemy, now);
        for (const type of eStatusExpired) {
            logger.info(`[STATUS] Enemy ${enemy.name}: ${type} expired`);
            broadcastToZone(enemyZone, 'status:removed', { targetId: eid, isEnemy: true, statusType: type, reason: 'expired' });
            broadcastToZone(enemyZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: type, reason: 'expired' });
        }
        for (const d of eStatusDrains) {
            logger.info(`[STATUS] ${d.type} drained ${d.drain} HP from enemy ${enemy.name} (HP: ${d.newHealth}/${enemy.maxHealth})`);
            broadcastToZone(enemyZone, 'enemy:health_update', {
                enemyId: eid, health: d.newHealth, maxHealth: enemy.maxHealth,
                inCombat: enemy.inCombatWith ? enemy.inCombatWith.size > 0 : false
            });
            broadcastToZone(enemyZone, 'status:tick', {
                targetId: eid, isEnemy: true, statusType: d.type,
                drain: d.drain, newHealth: d.newHealth, maxHealth: enemy.maxHealth
            });
        }

        // Tick buffs
        const eExpiredBuffs = expireBuffs(enemy);
        for (const buff of eExpiredBuffs) {
            logger.info(`[BUFF] Enemy ${enemy.name}: ${buff.name} expired`);
            broadcastToZone(enemyZone, 'skill:buff_removed', {
                targetId: eid, isEnemy: true, buffName: buff.name, skillId: buff.skillId, reason: 'expired'
            });
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

            // RO Classic Fire Wall: hit on contact → knockback outside wall → enemy walks back → repeat
            const wallRadius = effect.radius || 150;
            const knockbackTarget = effect.knockbackTarget || 200; // Distance from center to push to (> radius)

            for (const [eid, enemy] of enemies.entries()) {
                if (enemy.isDead) continue;
                if (effect.hitsRemaining <= 0) break;

                const dx = enemy.x - effect.x;
                const dy = enemy.y - effect.y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist > wallRadius) continue; // Not inside the wall

                // Per-target cooldown: 300ms prevents double-hits within same tick cycle
                // Normal enemies get knocked out and must walk back (~500ms+), so this rarely triggers
                // Boss/knockback-immune enemies eat charges at ~2/sec (every 300ms)
                if (!effect.lastHitTargets) effect.lastHitTargets = {};
                const targetKey = `enemy_${eid}`;
                if (effect.lastHitTargets[targetKey] && now - effect.lastHitTargets[targetKey] < 300) continue;
                effect.lastHitTargets[targetKey] = now;

                // Calculate fire damage: 50% MATK per hit (RO Classic)
                const caster = connectedPlayers.get(effect.casterId);
                if (!caster) continue;
                const casterStats = getEffectiveStats(caster);
                const fwResult = calculateMagicSkillDamage(casterStats, enemy.stats, enemy.hardMdef || enemy.magicDefense || 0, 50, 'fire', getEnemyTargetInfo(enemy));
                const finalDmg = Math.max(1, fwResult.damage || 0);

                enemy.health = Math.max(0, enemy.health - finalDmg);
                effect.hitsRemaining--;

                // RO AI: Fire Wall aggro
                enemy.lastDamageTime = now;
                if (typeof setEnemyAggro === 'function') setEnemyAggro(enemy, effect.casterId, 'skill');

                const isBossImmune = enemy.modeFlags && enemy.modeFlags.knockbackImmune;
                logger.info(`[SKILL-COMBAT] Fire Wall hit ${enemy.name} for ${finalDmg} fire [${effect.hitsRemaining} hits left]${isBossImmune ? ' (boss, no KB)' : ''}`);

                const fwTickZone = enemy.zone || 'prontera_south';

                // Damage breaks freeze/stone/sleep/confusion
                const fwEnemyBroken = checkDamageBreakStatuses(enemy);
                for (const brokenType of fwEnemyBroken) {
                    broadcastToZone(fwTickZone, 'status:removed', { targetId: eid, isEnemy: true, statusType: brokenType, reason: 'damage_break' });
                    broadcastToZone(fwTickZone, 'skill:buff_removed', { targetId: eid, isEnemy: true, buffName: brokenType, reason: 'damage_break' });
                }

                broadcastToZone(fwTickZone, 'skill:effect_damage', {
                    attackerId: effect.casterId, attackerName: caster.characterName,
                    targetId: eid, targetName: enemy.name, isEnemy: true,
                    skillId: 209, skillName: 'Fire Wall', element: 'fire',
                    damage: finalDmg, isCritical: false, isMiss: false,
                    hitType: 'skill',
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    targetHealth: enemy.health, targetMaxHealth: enemy.maxHealth,
                    timestamp: now
                });
                broadcastToZone(fwTickZone, 'enemy:health_update', { enemyId: eid, health: enemy.health, maxHealth: enemy.maxHealth, inCombat: enemy.inCombatWith.size > 0 });

                // Knockback: push enemy OUTSIDE the wall radius (RO Classic: 2 cells away)
                // Boss/MVP monsters are knockback-immune — they stay inside and consume charges rapidly
                if (!isBossImmune) {
                    if (dist > 0) {
                        // Push to knockbackTarget distance from wall center (outside radius)
                        enemy.x = effect.x + (dx / dist) * knockbackTarget;
                        enemy.y = effect.y + (dy / dist) * knockbackTarget;
                    } else {
                        // Enemy exactly at center — push in default direction
                        enemy.y = effect.y + knockbackTarget;
                    }
                    broadcastToZone(fwTickZone, 'enemy:move', {
                        enemyId: eid, x: enemy.x, y: enemy.y, z: enemy.z,
                        isMoving: false, knockback: true
                    });

                    // If enemy was attacking and knockback moved them out of melee range,
                    // switch to CHASE so AI walks them back through the wall
                    if (enemy.aiState === AI_STATE.ATTACK && enemy.targetPlayerId) {
                        const tp = connectedPlayers.get(enemy.targetPlayerId);
                        if (tp) {
                            const dxT = (tp.lastX || 0) - enemy.x;
                            const dyT = (tp.lastY || 0) - enemy.y;
                            const distT = Math.sqrt(dxT * dxT + dyT * dyT);
                            if (distT > (enemy.attackRange || COMBAT.MELEE_RANGE) + COMBAT.RANGE_TOLERANCE) {
                                enemy.aiState = AI_STATE.CHASE;
                            }
                        }
                    }
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
                if (effect.lastHitTargets[targetKey] && now - effect.lastHitTargets[targetKey] < 300) continue;
                effect.lastHitTargets[targetKey] = now;

                const caster = connectedPlayers.get(effect.casterId);
                if (!caster) continue;
                const casterStats = getEffectiveStats(caster);
                const targetStats = getEffectiveStats(player);
                const fwResult = calculateMagicSkillDamage(casterStats, targetStats, player.hardMdef || 0, 50, 'fire', getPlayerTargetInfo(player, charId));
                const finalDmg = Math.max(1, fwResult.damage || 0);

                player.health = Math.max(0, player.health - finalDmg);
                effect.hitsRemaining--;

                // Cast interruption: Fire Wall damage interrupts casting
                if (activeCasts.has(charId)) {
                    interruptCast(charId, 'damage');
                }

                const fwPvpTickZone = player.zone || 'prontera_south';
                // Damage breaks freeze/stone/sleep/confusion
                const fwPlayerBroken = checkDamageBreakStatuses(player);
                for (const brokenType of fwPlayerBroken) {
                    broadcastToZone(fwPvpTickZone, 'status:removed', { targetId: charId, isEnemy: false, statusType: brokenType, reason: 'damage_break' });
                    broadcastToZone(fwPvpTickZone, 'skill:buff_removed', { targetId: charId, isEnemy: false, buffName: brokenType, reason: 'damage_break' });
                }

                broadcastToZone(fwPvpTickZone, 'skill:effect_damage', {
                    attackerId: effect.casterId, attackerName: caster.characterName,
                    targetId: charId, targetName: player.characterName, isEnemy: false,
                    skillId: 209, skillName: 'Fire Wall', element: 'fire',
                    damage: finalDmg, isCritical: false, isMiss: false,
                    hitType: 'skill',
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
            const effectZone = effect.zone || (connectedPlayers.get(effect.casterId) && connectedPlayers.get(effect.casterId).zone) || 'prontera_south';
            removeGroundEffect(id);
            broadcastToZone(effectZone, 'skill:ground_effect_removed', { effectId: id, type: effect.type, reason: effect.hitsRemaining <= 0 ? 'hits_exhausted' : 'expired' });
        }
    }
}, 500);

// ============================================================
// Enemy AI Tick Loop (Ragnarok Online-style wandering)
// Enemy AI — Full RO Classic State Machine
// ============================================================
const ENEMY_AI = {
    TICK_MS: 200,               // AI tick interval (ms) — 5x per second
    WANDER_PAUSE_MIN: 3000,     // Min pause before next wander (ms)
    WANDER_PAUSE_MAX: 8000,     // Max pause before next wander (ms)
    WANDER_DIST_MIN: 100,       // Min wander distance per axis (units)
    WANDER_DIST_MAX: 300,       // Max wander distance per axis (units)
    MOVE_BROADCAST_MS: 200,     // How often to broadcast position updates (ms)
    AGGRO_SCAN_MS: 500,         // How often aggressive mobs scan for players (ms)
    ASSIST_RANGE: 550,          // Assist detection range (11 RO cells × 50 UE units)
    CHASE_GIVE_UP_EXTRA: 200,   // Extra UE units beyond chaseRange before giving up
    IDLE_AFTER_CHASE_MS: 2000,  // ms before returning to wander after losing target (mob_unlock_time)
};

// Initialize wander + AI state for enemies (called on spawn and respawn)
function initEnemyWanderState(enemy) {
    enemy.wanderTargetX = enemy.x;
    enemy.wanderTargetY = enemy.y;
    enemy.isWandering = false;
    enemy.nextWanderTime = Date.now() + ENEMY_AI.WANDER_PAUSE_MIN + Math.random() * (ENEMY_AI.WANDER_PAUSE_MAX - ENEMY_AI.WANDER_PAUSE_MIN);
    enemy.lastMoveBroadcast = 0;
    // Reset AI state machine
    enemy.aiState = AI_STATE.IDLE;
    enemy.targetPlayerId = null;
    enemy.aggroOriginX = null;
    enemy.aggroOriginY = null;
    enemy.lastAttackTime = 0;
    enemy.lastDamageTime = 0;
    enemy.lastAggroScan = 0;
    enemy.pendingTargetSwitch = null;  // { charId, hitType } — set by damage hooks, consumed by AI tick
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

// ============================================================
// Enemy AI Helper Functions
// ============================================================

// Check if enemy should switch targets (RO Classic rules)
function shouldSwitchTarget(enemy, newAttackerCharId, hitType) {
    if (!enemy.targetPlayerId) return true;               // No current target
    if (enemy.targetPlayerId === newAttackerCharId) return false;  // Same target
    if (enemy.modeFlags.randomTarget) return true;        // Random target every swing

    if (enemy.aiState === AI_STATE.ATTACK || enemy.aiState === AI_STATE.IDLE) {
        if (enemy.modeFlags.changeTargetMelee && hitType === 'melee') return true;
    }
    if (enemy.aiState === AI_STATE.CHASE) {
        if (enemy.modeFlags.changeTargetChase) return true;
    }
    return false;
}

// Trigger assist: when an enemy is attacked, nearby same-type mobs with ASSIST flag join in
function triggerAssist(attackedEnemy, attackerCharId) {
    for (const [, other] of enemies.entries()) {
        if (other === attackedEnemy) continue;
        if (other.isDead || other.aiState !== AI_STATE.IDLE) continue;
        if (other.zone !== attackedEnemy.zone) continue;  // Same zone only
        if (other.templateId !== attackedEnemy.templateId) continue;  // Same type only
        if (!other.modeFlags.assist) continue;
        if (!other.modeFlags.canAttack || !other.modeFlags.canMove) continue;

        const dx = other.x - attackedEnemy.x;
        const dy = other.y - attackedEnemy.y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist <= ENEMY_AI.ASSIST_RANGE) {
            other.targetPlayerId = attackerCharId;
            other.aiState = AI_STATE.CHASE;
            other.aggroOriginX = other.x;
            other.aggroOriginY = other.y;
            other.isWandering = false;
            other.inCombatWith.add(attackerCharId);
            logger.info(`[ENEMY AI] ${other.name}(${other.enemyId}) ASSIST → chasing player ${attackerCharId} (triggered by ${attackedEnemy.name})`);
        }
    }
}

// Set aggro on an enemy when damaged by a player
function setEnemyAggro(enemy, attackerCharId, hitType) {
    if (enemy.isDead) return;
    if (!enemy.modeFlags.canAttack) return;  // Plant-type: can't fight back

    enemy.inCombatWith.add(attackerCharId);
    enemy.isWandering = false;
    enemy.lastDamageTime = Date.now();

    // If no current target or should switch: take new target
    if (!enemy.targetPlayerId || enemy.aiState === AI_STATE.IDLE || shouldSwitchTarget(enemy, attackerCharId, hitType)) {
        enemy.targetPlayerId = attackerCharId;
        if (enemy.aiState === AI_STATE.IDLE) {
            enemy.aggroOriginX = enemy.x;
            enemy.aggroOriginY = enemy.y;
        }
        if (enemy.modeFlags.canMove) {
            enemy.aiState = AI_STATE.CHASE;
        } else {
            enemy.aiState = AI_STATE.ATTACK;  // Immobile mobs go straight to attack
        }
    }

    // Trigger assist for nearby same-type mobs
    triggerAssist(enemy, attackerCharId);
}

// Find closest player for aggressive aggro scan
function findAggroTarget(enemy) {
    let closestDist = Infinity;
    let closestCharId = null;

    for (const [charId, player] of connectedPlayers.entries()) {
        if (player.isDead) continue;
        // Only aggro players in the same zone
        if (player.zone !== enemy.zone) continue;

        // Use last known position from player object (updated on player:position events)
        const px = player.lastX;
        const py = player.lastY;
        if (px === undefined || py === undefined) continue;

        const dx = enemy.x - px;
        const dy = enemy.y - py;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist > enemy.aggroRange) continue;

        // TargetWeak: only aggro players 5+ levels below monster
        if (enemy.modeFlags.targetWeak) {
            const playerLevel = (player.stats && player.stats.level) || 1;
            if (playerLevel >= enemy.level - 5) continue;
        }

        if (dist < closestDist) {
            closestDist = dist;
            closestCharId = charId;
        }
    }

    return closestCharId;
}

// Calculate enemy → player physical damage (reuses existing RO damage formula)
function calculateEnemyDamage(enemy, targetCharId) {
    const player = connectedPlayers.get(targetCharId);
    if (!player) return null;

    const attackerStats = {
        str: (enemy.stats && enemy.stats.str) || 1,
        agi: (enemy.stats && enemy.stats.agi) || 1,
        vit: (enemy.stats && enemy.stats.vit) || 0,
        int: (enemy.stats && enemy.stats.int) || 0,
        dex: (enemy.stats && enemy.stats.dex) || 1,
        luk: (enemy.stats && enemy.stats.luk) || 1,
        level: enemy.level,
        weaponATK: enemy.damage || 1,
        passiveATK: 0,
    };

    const targetInfo = getPlayerTargetInfo(player, targetCharId);
    const attackerInfo = {
        weaponType: (enemy.attackRange > COMBAT.MELEE_RANGE + COMBAT.RANGE_TOLERANCE) ? 'bow' : 'bare_hand',
        weaponElement: (enemy.element && enemy.element.type) || 'neutral',
        weaponLevel: 1,
        buffMods: getBuffStatModifiers(enemy),
        cardMods: null,
    };

    return calculatePhysicalDamage(
        attackerStats,
        getEffectiveStats(player),
        player.hardDef || 0,
        targetInfo,
        attackerInfo
    );
}

// Process an enemy AI tick — move toward target and broadcast position
function enemyMoveToward(enemy, targetX, targetY, now, speed) {
    if (!enemy.modeFlags.canMove) return;

    const dx = targetX - enemy.x;
    const dy = targetY - enemy.y;
    const distance = Math.sqrt(dx * dx + dy * dy);
    if (distance < 5) return;

    const stepSize = speed * (ENEMY_AI.TICK_MS / 1000);
    const moveRatio = Math.min(1, stepSize / distance);
    enemy.x += dx * moveRatio;
    enemy.y += dy * moveRatio;

    // Broadcast position at limited rate
    if (now - (enemy.lastMoveBroadcast || 0) >= ENEMY_AI.MOVE_BROADCAST_MS) {
        enemy.lastMoveBroadcast = now;
        const moveZone = enemy.zone || 'prontera_south';
        broadcastToZone(moveZone, 'enemy:move', {
            enemyId: enemy.enemyId,
            x: enemy.x, y: enemy.y, z: enemy.z,
            targetX, targetY,
            isMoving: true
        });
    }
}

// Broadcast enemy stopped moving
function enemyStopMoving(enemy) {
    const stopZone = enemy.zone || 'prontera_south';
    broadcastToZone(stopZone, 'enemy:move', {
        enemyId: enemy.enemyId,
        x: enemy.x, y: enemy.y, z: enemy.z,
        isMoving: false
    });
}

// Wander subroutine (extracted from old loop for reuse in IDLE state)
function processWander(enemy, now) {
    // Plant/immobile: never wander
    if (!enemy.modeFlags.canMove || enemy.modeFlags.noRandomWalk) return;

    if (enemy.nextWanderTime === undefined) {
        enemy.nextWanderTime = now + ENEMY_AI.WANDER_PAUSE_MIN + Math.random() * (ENEMY_AI.WANDER_PAUSE_MAX - ENEMY_AI.WANDER_PAUSE_MIN);
        enemy.isWandering = false;
    }

    if (!enemy.isWandering) {
        if (now >= enemy.nextWanderTime) {
            const target = pickRandomWanderPoint(enemy);
            enemy.wanderTargetX = target.x;
            enemy.wanderTargetY = target.y;
            enemy.isWandering = true;
        }
    } else {
        const dx = enemy.wanderTargetX - enemy.x;
        const dy = enemy.wanderTargetY - enemy.y;
        const distance = Math.sqrt(dx * dx + dy * dy);

        if (distance < 10) {
            enemy.isWandering = false;
            enemy.nextWanderTime = now + ENEMY_AI.WANDER_PAUSE_MIN + Math.random() * (ENEMY_AI.WANDER_PAUSE_MAX - ENEMY_AI.WANDER_PAUSE_MIN);
            enemyStopMoving(enemy);
        } else {
            // Wander at 60% of chase speed (slower, relaxed movement)
            const wanderSpeed = enemy.moveSpeed * 0.6;
            enemyMoveToward(enemy, enemy.wanderTargetX, enemy.wanderTargetY, now, wanderSpeed);
        }
    }
}

// Get player position from connectedPlayers (synchronous, no Redis needed)
function getPlayerPosSync(charId) {
    const p = connectedPlayers.get(charId);
    if (!p || p.lastX === undefined) return null;
    return { x: p.lastX, y: p.lastY, z: p.lastZ || 300 };
}

// ============================================================
// Enemy AI Tick Loop — RO Classic State Machine
// States: IDLE → CHASE → ATTACK → DEAD
// Runs every TICK_MS (200ms = 5x per second)
// ============================================================
setInterval(async () => {
    const now = Date.now();
    const activeZones = getActiveZones();

    for (const [enemyId, enemy] of enemies.entries()) {
        if (enemy.isDead || enemy.aiState === AI_STATE.DEAD) continue;
        // Skip AI processing for enemies in zones with no players
        if (!activeZones.has(enemy.zone)) continue;

        // CC check: frozen/stoned/stunned/sleeping enemies cannot move or act
        const enemyCCMods = getCombinedModifiers(enemy);
        if (enemyCCMods.preventsMovement || enemyCCMods.preventsAttack) {
            // Stop any ongoing movement
            if (enemy.isWandering) {
                enemy.isWandering = false;
                enemyStopMoving(enemy);
            }
            continue; // Skip entire AI tick — cannot move, chase, attack, or wander
        }

        // Hit stun: if recently damaged, skip movement/attack (but allow state checks)
        const inHitStun = (now - (enemy.lastDamageTime || 0)) < (enemy.damageMotion || 300);

        switch (enemy.aiState) {

        // ═══════════════════════════════════════════════════════
        // IDLE — Wander randomly. Aggressive mobs scan for players.
        // ═══════════════════════════════════════════════════════
        case AI_STATE.IDLE: {
            // Aggressive mobs: scan for players to aggro
            if (enemy.modeFlags.aggressive && enemy.aggroRange > 0) {
                if (now - (enemy.lastAggroScan || 0) >= ENEMY_AI.AGGRO_SCAN_MS) {
                    enemy.lastAggroScan = now;
                    const targetCharId = findAggroTarget(enemy);
                    if (targetCharId) {
                        enemy.targetPlayerId = targetCharId;
                        enemy.aggroOriginX = enemy.x;
                        enemy.aggroOriginY = enemy.y;
                        enemy.isWandering = false;
                        enemy.inCombatWith.add(targetCharId);
                        if (enemy.modeFlags.canMove) {
                            enemy.aiState = AI_STATE.CHASE;
                        } else {
                            enemy.aiState = AI_STATE.ATTACK;
                        }
                        logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) AGGRO → player ${targetCharId}`);
                        break;
                    }
                }
            }

            // No aggro target: wander normally
            if (!inHitStun) {
                processWander(enemy, now);
            }
            break;
        }

        // ═══════════════════════════════════════════════════════
        // CHASE — Move toward target player until in attack range
        // ═══════════════════════════════════════════════════════
        case AI_STATE.CHASE: {
            // Validate target still exists
            const target = connectedPlayers.get(enemy.targetPlayerId);
            if (!target || target.isDead) {
                // Target gone — check if any other attackers remain
                enemy.inCombatWith.delete(enemy.targetPlayerId);
                const nextTarget = pickNextTarget(enemy);
                if (nextTarget) {
                    enemy.targetPlayerId = nextTarget;
                } else {
                    // No targets left — return to idle
                    enemy.targetPlayerId = null;
                    enemy.aiState = AI_STATE.IDLE;
                    enemy.isWandering = false;
                    enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                    enemyStopMoving(enemy);
                    logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) target lost → IDLE`);
                }
                break;
            }

            // Process pending target switch (set by damage hooks)
            if (enemy.pendingTargetSwitch) {
                enemy.targetPlayerId = enemy.pendingTargetSwitch.charId;
                enemy.pendingTargetSwitch = null;
            }

            const targetPos = getPlayerPosSync(enemy.targetPlayerId);
            if (!targetPos) {
                enemy.targetPlayerId = null;
                enemy.aiState = AI_STATE.IDLE;
                enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                enemyStopMoving(enemy);
                break;
            }

            // Chase range check — give up if too far from aggro origin
            const dxFromOrigin = enemy.x - (enemy.aggroOriginX || enemy.spawnX);
            const dyFromOrigin = enemy.y - (enemy.aggroOriginY || enemy.spawnY);
            const distFromOrigin = Math.sqrt(dxFromOrigin * dxFromOrigin + dyFromOrigin * dyFromOrigin);
            if (distFromOrigin > (enemy.chaseRange || 600) + ENEMY_AI.CHASE_GIVE_UP_EXTRA) {
                // Too far — give up chase
                enemy.targetPlayerId = null;
                enemy.aiState = AI_STATE.IDLE;
                enemy.inCombatWith.clear();
                enemy.isWandering = false;
                enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                enemyStopMoving(enemy);
                logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) gave up chase (dist from origin: ${distFromOrigin.toFixed(0)})`);
                break;
            }

            // Distance to target
            const dxToTarget = targetPos.x - enemy.x;
            const dyToTarget = targetPos.y - enemy.y;
            const distToTarget = Math.sqrt(dxToTarget * dxToTarget + dyToTarget * dyToTarget);

            // In attack range? → transition to ATTACK
            const attackRange = enemy.attackRange || COMBAT.MELEE_RANGE;
            if (distToTarget <= attackRange + COMBAT.RANGE_TOLERANCE) {
                enemy.aiState = AI_STATE.ATTACK;
                enemyStopMoving(enemy);
                logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) reached target → ATTACK`);
                break;
            }

            // Move toward target (skip if in hit stun)
            if (!inHitStun) {
                enemyMoveToward(enemy, targetPos.x, targetPos.y, now, enemy.moveSpeed);
            }

            // ChangeChase: while chasing, switch to a closer player within attack range
            if (enemy.modeFlags.changeChase) {
                for (const [charId] of enemy.inCombatWith.entries()) {
                    if (charId === enemy.targetPlayerId) continue;
                    const otherPos = getPlayerPosSync(charId);
                    if (!otherPos) continue;
                    const dx2 = otherPos.x - enemy.x;
                    const dy2 = otherPos.y - enemy.y;
                    const dist2 = Math.sqrt(dx2 * dx2 + dy2 * dy2);
                    if (dist2 <= attackRange + COMBAT.RANGE_TOLERANCE) {
                        enemy.targetPlayerId = charId;
                        logger.debug(`[ENEMY AI] ${enemy.name}(${enemyId}) CHANGECHASE → player ${charId} (closer)`);
                        break;
                    }
                }
            }
            break;
        }

        // ═══════════════════════════════════════════════════════
        // ATTACK — Auto-attack target at attackDelay interval
        // ═══════════════════════════════════════════════════════
        case AI_STATE.ATTACK: {
            const atkTarget = connectedPlayers.get(enemy.targetPlayerId);
            if (!atkTarget || atkTarget.isDead) {
                enemy.inCombatWith.delete(enemy.targetPlayerId);
                const nextTarget = pickNextTarget(enemy);
                if (nextTarget) {
                    enemy.targetPlayerId = nextTarget;
                    enemy.aiState = AI_STATE.CHASE;
                } else {
                    enemy.targetPlayerId = null;
                    enemy.aiState = AI_STATE.IDLE;
                    enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                    enemyStopMoving(enemy);
                }
                break;
            }

            // Process pending target switch
            if (enemy.pendingTargetSwitch) {
                const switchTo = enemy.pendingTargetSwitch.charId;
                enemy.pendingTargetSwitch = null;
                if (switchTo !== enemy.targetPlayerId) {
                    enemy.targetPlayerId = switchTo;
                    enemy.aiState = AI_STATE.CHASE;
                    break;
                }
            }

            // RandomTarget: pick random attacker each swing
            if (enemy.modeFlags.randomTarget && enemy.inCombatWith.size > 1) {
                const combatants = [...enemy.inCombatWith].filter(id => {
                    const p = connectedPlayers.get(id);
                    return p && !p.isDead;
                });
                if (combatants.length > 0) {
                    enemy.targetPlayerId = combatants[Math.floor(Math.random() * combatants.length)];
                }
            }

            const targetPos = getPlayerPosSync(enemy.targetPlayerId);
            if (!targetPos) {
                enemy.targetPlayerId = null;
                enemy.aiState = AI_STATE.IDLE;
                enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                enemyStopMoving(enemy);
                break;
            }

            // Range check — if target moved out of range, chase
            const dxAtk = targetPos.x - enemy.x;
            const dyAtk = targetPos.y - enemy.y;
            const distAtk = Math.sqrt(dxAtk * dxAtk + dyAtk * dyAtk);
            const atkRange = enemy.attackRange || COMBAT.MELEE_RANGE;
            if (distAtk > atkRange + COMBAT.RANGE_TOLERANCE + 30) {
                enemy.aiState = AI_STATE.CHASE;
                break;
            }

            // Hit stun: can't attack while flinching
            if (inHitStun) break;

            // Attack timing check
            if (now - enemy.lastAttackTime < (enemy.attackDelay || 1500)) break;

            // ── EXECUTE ATTACK ──
            enemy.lastAttackTime = now;

            const combatResult = calculateEnemyDamage(enemy, enemy.targetPlayerId);
            if (!combatResult) break;

            const { damage, isCritical, isMiss, hitType, element: atkElement } = combatResult;

            // Build damage payload
            const damagePayload = {
                attackerId: enemy.enemyId,
                attackerName: enemy.name,
                targetId: enemy.targetPlayerId,
                targetName: atkTarget.characterName,
                isEnemy: false,               // Target is player (not enemy)
                isEnemyAttacker: true,        // Attacker is enemy
                damage,
                isCritical,
                isMiss,
                hitType,
                element: atkElement,
                targetHealth: atkTarget.health,
                targetMaxHealth: atkTarget.maxHealth,
                attackerX: enemy.x, attackerY: enemy.y, attackerZ: enemy.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: now
            };

            if (!isMiss) {
                atkTarget.health = Math.max(0, atkTarget.health - damage);
                damagePayload.targetHealth = atkTarget.health;
                logger.info(`[ENEMY COMBAT] ${enemy.name}(${enemyId}) hit ${atkTarget.characterName} for ${damage}${isCritical ? ' CRIT' : ''} (HP: ${atkTarget.health}/${atkTarget.maxHealth})`);
            } else {
                logger.info(`[ENEMY COMBAT] ${enemy.name}(${enemyId}) ${hitType} against ${atkTarget.characterName}`);
            }

            // Broadcast damage
            const enemyAtkZone = enemy.zone || 'prontera_south';
            broadcastToZone(enemyAtkZone, 'combat:damage', damagePayload);

            // Broadcast enemy attack visual (for client-side attack animation)
            broadcastToZone(enemyAtkZone, 'enemy:attack', {
                enemyId: enemy.enemyId,
                targetId: enemy.targetPlayerId,
                attackMotion: enemy.attackMotion,
            });

            // Broadcast target health update
            const targetSocket = io.sockets.sockets.get(atkTarget.socketId);
            broadcastToZone(enemyAtkZone, 'combat:health_update', {
                characterId: enemy.targetPlayerId,
                health: atkTarget.health,
                maxHealth: atkTarget.maxHealth,
                mana: atkTarget.mana,
                maxMana: atkTarget.maxMana
            });

            // Check player death
            if (!isMiss && atkTarget.health <= 0) {
                atkTarget.isDead = true;

                // Stop player's auto-attacks
                autoAttackState.delete(enemy.targetPlayerId);

                // Emit death event
                broadcastToZone(enemyAtkZone, 'combat:death', {
                    killedId: enemy.targetPlayerId,
                    killedName: atkTarget.characterName,
                    killerId: enemy.enemyId,
                    killerName: enemy.name,
                    isEnemy: false,          // killed is NOT an enemy (it's a player)
                    isEnemyKiller: true,     // killer is an enemy
                    targetHealth: 0,
                    targetMaxHealth: atkTarget.maxHealth,
                    timestamp: now
                });

                // Save to DB
                savePlayerHealthToDB(enemy.targetPlayerId, 0, atkTarget.mana);

                logger.info(`[ENEMY COMBAT] ${enemy.name}(${enemyId}) KILLED ${atkTarget.characterName}`);

                // Clear this target
                enemy.inCombatWith.delete(enemy.targetPlayerId);

                // Pick next target or go idle
                const nextTarget = pickNextTarget(enemy);
                if (nextTarget) {
                    enemy.targetPlayerId = nextTarget;
                    enemy.aiState = AI_STATE.CHASE;
                } else {
                    enemy.targetPlayerId = null;
                    enemy.aiState = AI_STATE.IDLE;
                    enemy.nextWanderTime = now + ENEMY_AI.IDLE_AFTER_CHASE_MS;
                    enemyStopMoving(enemy);
                }
            }
            break;
        }
        } // end switch
    }
}, ENEMY_AI.TICK_MS);

// Pick next available target from inCombatWith set (for after current target dies/disconnects)
function pickNextTarget(enemy) {
    for (const charId of enemy.inCombatWith) {
        const p = connectedPlayers.get(charId);
        if (p && !p.isDead) return charId;
    }
    return null;
}

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

// Server list endpoint
app.get('/api/servers', (req, res) => {
    const servers = [
        {
            id: 1,
            name: 'Sabri',
            host: process.env.SERVER_HOST || 'localhost',
            port: parseInt(process.env.PORT) || 3001,
            status: 'online',
            population: connectedPlayers.size,
            maxPopulation: 1000,
            region: 'Local'
        }
    ];
    res.json({ servers });
});

// Character endpoints
app.get('/api/characters', authenticateToken, async (req, res) => {
    try {
        logger.debug(`Fetching characters for user ID: ${req.user.user_id}`);
        const result = await pool.query(
            `SELECT character_id, name, class, level, x, y, z,
                    health, max_health, mana, max_mana,
                    str, agi, vit, int_stat, dex, luk,
                    stat_points, zuzucoin,
                    job_level, base_exp, job_exp, job_class, skill_points,
                    hair_style, hair_color, gender,
                    zone_name,
                    delete_date, created_at, last_played
             FROM characters
             WHERE user_id = $1 AND delete_date IS NULL AND deleted = FALSE
             ORDER BY character_id ASC
             LIMIT 9`,
            [req.user.user_id]
        );

        // Enrich each character with zone levelName for direct level loading
        const characters = result.rows.map(row => {
            const zoneName = row.zone_name || 'prontera_south';
            const zoneInfo = getZone(zoneName);
            return {
                ...row,
                zone_name: zoneName,
                level_name: zoneInfo ? zoneInfo.levelName : 'L_PrtSouth'
            };
        });

        logger.info(`Retrieved ${characters.length} characters for user ${req.user.user_id}`);
        res.json({
            message: 'Characters retrieved successfully',
            characters
        });
    } catch (err) {
        logger.error('Get characters error:', err.message);
        res.status(500).json({ error: 'Failed to retrieve characters' });
    }
});

app.post('/api/characters', authenticateToken, async (req, res) => {
    try {
        const { name, characterClass, hairStyle, hairColor, gender } = req.body;
        logger.info(`Character creation attempt: ${name} for user ${req.user.user_id}`);

        // Validate name
        if (!name || name.trim().length < 2 || name.trim().length > 24) {
            return res.status(400).json({ error: 'Character name must be 2-24 characters' });
        }

        // Validate name format (alphanumeric + spaces only)
        if (!/^[a-zA-Z0-9 ]+$/.test(name.trim())) {
            return res.status(400).json({ error: 'Name can only contain letters, numbers, and spaces' });
        }

        // Check character limit (9 per account)
        const countResult = await pool.query(
            'SELECT COUNT(*) FROM characters WHERE user_id = $1 AND delete_date IS NULL AND deleted = FALSE',
            [req.user.user_id]
        );
        if (parseInt(countResult.rows[0].count) >= 9) {
            return res.status(400).json({ error: 'Maximum 9 characters per account' });
        }

        // Check global name uniqueness (case-insensitive)
        const nameCheck = await pool.query(
            'SELECT 1 FROM characters WHERE LOWER(name) = LOWER($1)',
            [name.trim()]
        );
        if (nameCheck.rows.length > 0) {
            return res.status(409).json({ error: 'Character name is already taken' });
        }

        // Validate customization
        const validHairStyle = Math.max(1, Math.min(19, parseInt(hairStyle) || 1));
        const validHairColor = Math.max(0, Math.min(8, parseInt(hairColor) || 0));
        const validGender = (gender === 'female') ? 'female' : 'male';

        // All characters start as Novice (RO classic)
        const charClass = 'novice';

        const result = await pool.query(
            `INSERT INTO characters
             (user_id, name, class, level, x, y, z, health, mana,
              max_health, max_mana, job_level, base_exp, job_exp,
              job_class, skill_points, stat_points,
              hair_style, hair_color, gender)
             VALUES ($1, $2, $3, 1, 0, 0, 0, 100, 100,
                     100, 100, 1, 0, 0,
                     'novice', 0, 48,
                     $4, $5, $6)
             RETURNING character_id, name, class, level, job_level, job_class,
                       hair_style, hair_color, gender, health, max_health,
                       mana, max_mana, stat_points, created_at`,
            [req.user.user_id, name.trim(), charClass,
             validHairStyle, validHairColor, validGender]
        );

        const character = result.rows[0];
        logger.info(`Character created: ${name} (ID: ${character.character_id})`);

        res.status(201).json({
            message: 'Character created successfully',
            character
        });

    } catch (err) {
        logger.error('Create character error:', err.message);
        if (err.code === '23505') {
            return res.status(409).json({ error: 'Character name is already taken' });
        }
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

// Delete character (requires password confirmation)
app.delete('/api/characters/:id', authenticateToken, async (req, res) => {
    try {
        const characterId = parseInt(req.params.id);
        const { password } = req.body;

        if (!password) {
            return res.status(400).json({ error: 'Password required to delete character' });
        }

        // Verify password
        const userResult = await pool.query(
            'SELECT password_hash FROM users WHERE user_id = $1',
            [req.user.user_id]
        );
        if (userResult.rows.length === 0) {
            return res.status(404).json({ error: 'User not found' });
        }

        const validPassword = await bcrypt.compare(password, userResult.rows[0].password_hash);
        if (!validPassword) {
            return res.status(401).json({ error: 'Incorrect password' });
        }

        // Verify character belongs to user and is not already deleted
        const charResult = await pool.query(
            'SELECT character_id, name FROM characters WHERE character_id = $1 AND user_id = $2 AND deleted = FALSE',
            [characterId, req.user.user_id]
        );
        if (charResult.rows.length === 0) {
            return res.status(404).json({ error: 'Character not found' });
        }

        // Check if character is currently online
        if (connectedPlayers.has(characterId)) {
            return res.status(409).json({ error: 'Character is currently online. Log out first.' });
        }

        const charName = charResult.rows[0].name;

        // Soft delete — mark as deleted instead of removing
        await pool.query('UPDATE characters SET deleted = TRUE WHERE character_id = $1', [characterId]);

        logger.info(`Character deleted: ${charName} (ID: ${characterId}) by user ${req.user.user_id}`);

        res.json({ message: 'Character deleted successfully', characterName: charName });

    } catch (err) {
        logger.error('Delete character error:', err.message);
        res.status(500).json({ error: 'Failed to delete character' });
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
      ADD COLUMN IF NOT EXISTS zuzucoin INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS hair_style INTEGER DEFAULT 1,
      ADD COLUMN IF NOT EXISTS hair_color INTEGER DEFAULT 0,
      ADD COLUMN IF NOT EXISTS gender VARCHAR(10) DEFAULT 'male',
      ADD COLUMN IF NOT EXISTS delete_date TIMESTAMP DEFAULT NULL
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
    // Normalize existing skill slots from 1-based to 0-based (one-time, idempotent)
    // Only runs if any skill slot has slot_index >= 9 (impossible in 0-based 0-8 range)
    try {
      const check = await pool.query(
        `SELECT 1 FROM character_hotbar WHERE slot_type = 'skill' AND slot_index >= 9 LIMIT 1`
      );
      if (check.rows.length > 0) {
        const result = await pool.query(
          `UPDATE character_hotbar SET slot_index = slot_index - 1 WHERE slot_type = 'skill' AND slot_index >= 1`
        );
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

  // Add zone tracking + save point columns if missing (for multi-zone/warp system)
  try {
    await pool.query(`
      ALTER TABLE characters
      ADD COLUMN IF NOT EXISTS zone_name VARCHAR(50) DEFAULT 'prontera_south'
    `);
    await pool.query(`
      ALTER TABLE characters
      ADD COLUMN IF NOT EXISTS save_map VARCHAR(50) DEFAULT 'prontera',
      ADD COLUMN IF NOT EXISTS save_x FLOAT DEFAULT 0,
      ADD COLUMN IF NOT EXISTS save_y FLOAT DEFAULT 0,
      ADD COLUMN IF NOT EXISTS save_z FLOAT DEFAULT 580
    `);
    await pool.query(`
      CREATE INDEX IF NOT EXISTS idx_characters_zone ON characters(zone_name)
    `);
    logger.info('[DB] Zone system columns verified on characters table');
  } catch (err) {
    logger.warn(`[DB] Zone system column issue: ${err.message}`);
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
  
  // Zone-based lazy enemy spawning: enemies spawn when first player enters a zone
  // No enemies spawned at startup — they spawn on demand in player:join handler
  const totalSpawns = Object.values(ZONE_REGISTRY).reduce((sum, z) => sum + z.enemySpawns.length, 0);
  logger.info(`[ZONE] ${Object.keys(ZONE_REGISTRY).length} zones registered, ${totalSpawns} total enemy spawn points (lazy spawn on player enter)`);
});

// Periodic health/mana + EXP + zone save to DB (every 60 seconds)
setInterval(async () => {
  for (const [charId, player] of connectedPlayers.entries()) {
    if (!player.isDead) {
      await savePlayerHealthToDB(charId, player.health, player.mana);
      await saveExpDataToDB(charId, player);
      // Save zone + position
      try {
        await pool.query(
          'UPDATE characters SET zone_name = $1, x = $2, y = $3, z = $4 WHERE character_id = $5',
          [player.zone || 'prontera_south', player.lastX || 0, player.lastY || 0, player.lastZ || 0, charId]
        );
      } catch (err) {
        logger.warn(`[DB] Failed to save zone for char ${charId}: ${err.message}`);
      }
    }
  }
}, 60000);
