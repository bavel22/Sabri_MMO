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

// Combat constants (Ragnarok Online-style)
const COMBAT = {
    BASE_DAMAGE: 1,             // Default damage per hit (before stat formulas)
    DAMAGE_VARIANCE: 0,         // Random variance added to base damage
    MELEE_RANGE: 100,           // Default melee attack range (Unreal units)
    RANGED_RANGE: 800,          // Default ranged attack range
    DEFAULT_ASPD: 180,          // Default attack speed (0-190 scale, higher = faster)
    ASPD_CAP: 190,              // Maximum ASPD
    RANGE_TOLERANCE: 50,         // Extra units added to out_of_range check so client moves closer than max range
    COMBAT_TICK_MS: 50,         // Server combat tick interval (ms)
    RESPAWN_DELAY_MS: 5000,
    SPAWN_POSITION: { x: 0, y: 0, z: 300 }
};

// Calculate attack interval from ASPD (ms between attacks)
// ASPD 180 = 1000ms (1 hit/sec), ASPD 190 = 500ms (2/sec), ASPD 170 = 1500ms (0.67/sec)
function getAttackIntervalMs(aspd) {
    const clamped = Math.min(aspd, COMBAT.ASPD_CAP);
    return (200 - clamped) * 50; // e.g. ASPD 180 → 20 * 50 = 1000ms
}

// Auto-attack state tracking: Map<attackerCharId, { targetCharId, isEnemy, startTime }>
const autoAttackState = new Map();

// ============================================================
// RO-Style Derived Stat Calculations
// ============================================================
function calculateDerivedStats(stats) {
    const { str = 1, agi = 1, vit = 1, int: intStat = 1, dex = 1, luk = 1, level = 1 } = stats;
    const statusATK = str + Math.floor(str / 10) ** 2 + Math.floor(dex / 5) + Math.floor(luk / 3);
    const statusMATK = intStat + Math.floor(intStat / 7) ** 2;
    const hit = level + dex;
    const flee = level + agi;
    const softDEF = Math.floor(vit * 0.5 + (vit ** 2) / 150);
    const softMDEF = Math.floor(intStat * 0.5);
    const critical = Math.floor(luk * 0.3);
    const aspd = Math.min(COMBAT.ASPD_CAP, Math.floor(170 + agi * 0.4 + dex * 0.1));
    const maxHP = 100 + vit * 8 + level * 10;
    const maxSP = 50 + intStat * 5 + level * 5;
    return { statusATK, statusMATK, hit, flee, softDEF, softMDEF, critical, aspd, maxHP, maxSP };
}

function calculatePhysicalDamage(attackerStats, targetStats) {
    const atkDerived = calculateDerivedStats(attackerStats);
    const defDerived = calculateDerivedStats(targetStats);
    const weaponATK = attackerStats.weaponATK || 0;
    const totalATK = atkDerived.statusATK + weaponATK;
    const variance = 0.8 + Math.random() * 0.2;
    const rawDamage = Math.floor(totalATK * variance);
    const damage = Math.max(1, rawDamage - defDerived.softDEF);
    const isCritical = Math.random() * 100 < atkDerived.critical;
    const finalDamage = isCritical ? Math.floor(damage * 1.4) : damage;
    return { damage: Math.max(1, finalDamage), isCritical };
}

// ============================================================
// Enemy/NPC System
// ============================================================
const enemies = new Map();
let nextEnemyId = 2000001;

const ENEMY_TEMPLATES = {
    blobby: {
        name: 'Blobby', level: 1, maxHealth: 50, damage: 1,
        attackRange: 80, aggroRange: 300, aspd: 175, exp: 10,
        respawnMs: 10000, aiType: 'passive',
        stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1, weaponATK: 0 }
    },
    hoplet: {
        name: 'Hoplet', level: 3, maxHealth: 100, damage: 3,
        attackRange: 80, aggroRange: 400, aspd: 178, exp: 25,
        respawnMs: 15000, aiType: 'passive',
        stats: { str: 3, agi: 5, vit: 2, int: 1, dex: 3, luk: 5, level: 3, weaponATK: 2 }
    },
    crawlid: {
        name: 'Crawlid', level: 2, maxHealth: 75, damage: 2,
        attackRange: 80, aggroRange: 0, aspd: 176, exp: 15,
        respawnMs: 12000, aiType: 'passive',
        stats: { str: 2, agi: 2, vit: 3, int: 1, dex: 2, luk: 1, level: 2, weaponATK: 1 }
    },
    shroomkin: {
        name: 'Shroomkin', level: 4, maxHealth: 120, damage: 4,
        attackRange: 80, aggroRange: 350, aspd: 177, exp: 30,
        respawnMs: 15000, aiType: 'passive',
        stats: { str: 4, agi: 3, vit: 4, int: 2, dex: 3, luk: 2, level: 4, weaponATK: 3 }
    },
    buzzer: {
        name: 'Buzzer', level: 5, maxHealth: 150, damage: 5,
        attackRange: 80, aggroRange: 500, aspd: 179, exp: 40,
        respawnMs: 18000, aiType: 'aggressive',
        stats: { str: 5, agi: 7, vit: 3, int: 1, dex: 5, luk: 3, level: 5, weaponATK: 4 }
    },
    mosswort: {
        name: 'Mosswort', level: 3, maxHealth: 90, damage: 2,
        attackRange: 80, aggroRange: 0, aspd: 174, exp: 20,
        respawnMs: 12000, aiType: 'passive',
        stats: { str: 2, agi: 1, vit: 5, int: 3, dex: 2, luk: 1, level: 3, weaponATK: 1 }
    }
};

const ENEMY_SPAWNS = [
    { template: 'blobby',    x: 500,  y: 500,  z: 300, wanderRadius: 300 },
    { template: 'blobby',    x: -500, y: 300,  z: 300, wanderRadius: 300 },
    { template: 'blobby',    x: 200,  y: -400, z: 300, wanderRadius: 300 },
    { template: 'hoplet',    x: 800,  y: -200, z: 300, wanderRadius: 400 },
    { template: 'hoplet',    x: -700, y: -500, z: 300, wanderRadius: 400 },
    { template: 'crawlid',   x: -300, y: 800,  z: 300, wanderRadius: 200 },
    { template: 'crawlid',   x: 400,  y: 700,  z: 300, wanderRadius: 200 },
    { template: 'shroomkin', x: 1000, y: 300,  z: 300, wanderRadius: 350 },
    { template: 'shroomkin', x: -900, y: 100,  z: 300, wanderRadius: 350 },
    { template: 'buzzer',    x: 1200, y: -600, z: 300, wanderRadius: 500 },
    { template: 'mosswort',  x: 0,    y: -800, z: 300, wanderRadius: 200 },
    { template: 'mosswort',  x: 600,  y: -900, z: 300, wanderRadius: 200 }
];

function spawnEnemy(spawnConfig) {
    const template = ENEMY_TEMPLATES[spawnConfig.template];
    if (!template) return null;
    const enemyId = nextEnemyId++;
    const enemy = {
        enemyId, templateId: spawnConfig.template,
        name: template.name, level: template.level,
        health: template.maxHealth, maxHealth: template.maxHealth,
        damage: template.damage, attackRange: template.attackRange,
        aggroRange: template.aggroRange, aspd: template.aspd,
        exp: template.exp, aiType: template.aiType,
        stats: { ...template.stats }, isDead: false,
        x: spawnConfig.x, y: spawnConfig.y, z: spawnConfig.z,
        spawnX: spawnConfig.x, spawnY: spawnConfig.y, spawnZ: spawnConfig.z,
        wanderRadius: spawnConfig.wanderRadius, respawnMs: template.respawnMs,
        targetPlayerId: null, lastAttackTime: 0,
        inCombatWith: new Set()
    };
    enemies.set(enemyId, enemy);
    logger.info(`[ENEMY] Spawned ${enemy.name} (ID: ${enemyId}) at (${enemy.x}, ${enemy.y}, ${enemy.z})`);
    io.emit('enemy:spawn', {
        enemyId, templateId: spawnConfig.template, name: enemy.name,
        level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
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
        const { characterId, token, characterName } = data;
        
        // Fetch character data from database (position + health/mana)
        let health = 100, maxHealth = 100, mana = 100, maxMana = 100;
        try {
            const charResult = await pool.query(
                'SELECT x, y, z, health, max_health, mana, max_mana FROM characters WHERE character_id = $1',
                [characterId]
            );
            if (charResult.rows.length > 0) {
                const row = charResult.rows[0];
                health = row.health;
                maxHealth = row.max_health;
                mana = row.mana;
                maxMana = row.max_mana;
                
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
        
        // Load stats from DB if available
        try {
            const statsResult = await pool.query(
                'SELECT str, agi, vit, int_stat, dex, luk, level, stat_points FROM characters WHERE character_id = $1',
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
            }
        } catch (err) {
            logger.warn(`Could not load stats for character ${characterId}: ${err.message}`);
        }
        
        const derived = calculateDerivedStats(baseStats);
        
        // Store player connection with combat data
        connectedPlayers.set(characterId, {
            socketId: socket.id,
            characterId: characterId,
            characterName: characterName || 'Unknown',
            health,
            maxHealth,
            mana,
            maxMana,
            isDead: false,
            lastAttackTime: 0,
            aspd: derived.aspd || COMBAT.DEFAULT_ASPD,
            attackRange: COMBAT.MELEE_RANGE,
            stats: baseStats
        });
        
        logger.info(`Player joined: ${characterName || 'Unknown'} (Character ${characterId}) HP: ${health}/${maxHealth} MP: ${mana}/${maxMana}`);
        const joinedPayload = { success: true };
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
        
        // Send player stats
        const statsPayload = {
            characterId,
            stats: baseStats,
            derived
        };
        socket.emit('player:stats', statsPayload);
        logger.info(`[SEND] player:stats to ${socket.id}: ${JSON.stringify(statsPayload)}`);
        
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
        const { characterId, x, y, z } = data;
        const player = connectedPlayers.get(characterId);
        const characterName = player ? player.characterName : 'Unknown';
        
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
                // Save health/mana to DB on disconnect
                await savePlayerHealthToDB(charId, player.health, player.mana);
                
                // Save stats to DB on disconnect
                if (player.stats) {
                    try {
                        await pool.query(
                            `UPDATE characters SET str = $1, agi = $2, vit = $3, int_stat = $4, dex = $5, luk = $6, stat_points = $7 WHERE character_id = $8`,
                            [player.stats.str, player.stats.agi, player.stats.vit, player.stats.int, player.stats.dex, player.stats.luk, player.stats.statPoints, charId]
                        );
                        logger.info(`[DB] Saved stats for character ${charId} on disconnect`);
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
                                const lostPayload = { reason: 'Target disconnected' };
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
        const { targetCharacterId, targetEnemyId } = data;
        
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
            
            // Mark enemy in combat with this player
            enemy.inCombatWith.add(attackerId);
            
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
        if (!attackerInfo) return;
        
        const { characterId: attackerId, player: attacker } = attackerInfo;
        
        if (autoAttackState.has(attackerId)) {
            const atkState = autoAttackState.get(attackerId);
            // Remove from enemy combat set if attacking an enemy
            if (atkState.isEnemy) {
                const enemy = enemies.get(atkState.targetCharId);
                if (enemy) enemy.inCombatWith.delete(attackerId);
            }
            autoAttackState.delete(attackerId);
            const stopPayload = { reason: 'Player stopped' };
            socket.emit('combat:auto_attack_stopped', stopPayload);
            logger.info(`[SEND] combat:auto_attack_stopped to ${socket.id}: ${JSON.stringify(stopPayload)}`);
            logger.info(`[COMBAT] ${attacker.characterName} stopped auto-attacking`);
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
                        const lostPayload = { reason: 'Target respawned' };
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
        const derived = calculateDerivedStats(player.stats);
        
        const statsPayload = {
            characterId,
            stats: player.stats,
            derived
        };
        socket.emit('player:stats', statsPayload);
        logger.info(`[SEND] player:stats to ${socket.id}: ${JSON.stringify(statsPayload)}`);
    });
    
    // Stat allocation handler
    socket.on('player:allocate_stat', async (data) => {
        logger.info(`[RECV] player:allocate_stat from ${socket.id}: ${JSON.stringify(data)}`);
        const { statName, amount } = data;
        
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const { characterId, player } = playerInfo;
        
        const validStats = ['str', 'agi', 'vit', 'int', 'dex', 'luk'];
        if (!validStats.includes(statName)) {
            socket.emit('combat:error', { message: `Invalid stat: ${statName}` });
            return;
        }
        
        const pts = parseInt(amount) || 1;
        const statKey = statName === 'int' ? 'int' : statName;
        
        if (!player.stats || player.stats.statPoints < pts) {
            socket.emit('combat:error', { message: 'Not enough stat points' });
            return;
        }
        
        // RO-style cost: increasing stat costs more points (simplified: 1 point per increase for now)
        player.stats[statKey] += pts;
        player.stats.statPoints -= pts;
        
        // Recalculate derived stats
        const derived = calculateDerivedStats(player.stats);
        player.aspd = derived.aspd;
        
        // Save to DB
        try {
            const dbStatName = statName === 'int' ? 'int_stat' : statName;
            await pool.query(
                `UPDATE characters SET ${dbStatName} = $1, stat_points = $2 WHERE character_id = $3`,
                [player.stats[statKey], player.stats.statPoints, characterId]
            );
        } catch (err) {
            logger.error(`Failed to save stat allocation for character ${characterId}:`, err.message);
        }
        
        const statsPayload = {
            characterId,
            stats: player.stats,
            derived
        };
        socket.emit('player:stats', statsPayload);
        logger.info(`[SEND] player:stats to ${socket.id}: ${JSON.stringify(statsPayload)}`);
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
});

// ============================================================
// Combat Tick Loop (RO-style auto-attack processing)
// Handles: player-vs-player, player-vs-enemy, enemy respawns
// Runs every COMBAT_TICK_MS
// ============================================================
setInterval(async () => {
    const now = Date.now();
    
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
                
                // IN RANGE: Execute attack on enemy
                const damage = COMBAT.BASE_DAMAGE + Math.floor(Math.random() * (COMBAT.DAMAGE_VARIANCE + 1));
                enemy.health = Math.max(0, enemy.health - damage);
                attacker.lastAttackTime = now;
                
                logger.info(`[COMBAT] ${attacker.characterName} hit enemy ${enemy.name}(${enemy.enemyId}) for ${damage} (HP: ${enemy.health}/${enemy.maxHealth})`);
                
                // Broadcast enemy damage (include positions for remote rotation)
                const damagePayload = {
                    attackerId,
                    attackerName: attacker.characterName,
                    targetId: enemy.enemyId,
                    targetName: enemy.name,
                    isEnemy: true,
                    damage,
                    targetHealth: enemy.health,
                    targetMaxHealth: enemy.maxHealth,
                    attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                    targetX: enemy.x, targetY: enemy.y, targetZ: enemy.z,
                    timestamp: now
                };
                io.emit('combat:damage', damagePayload);
                logger.info(`[BROADCAST] combat:damage (enemy): ${JSON.stringify(damagePayload)}`);
                
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
                    for (const [otherId, otherAtk] of autoAttackState.entries()) {
                        if (otherAtk.isEnemy && otherAtk.targetCharId === enemy.enemyId) {
                            autoAttackState.delete(otherId);
                            const otherPlayer = connectedPlayers.get(otherId);
                            if (otherPlayer) {
                                const otherSocket = io.sockets.sockets.get(otherPlayer.socketId);
                                if (otherSocket) {
                                    const lostPayload = { reason: 'Enemy died', isEnemy: true };
                                    otherSocket.emit('combat:target_lost', lostPayload);
                                    logger.info(`[SEND] combat:target_lost to ${otherPlayer.socketId}: ${JSON.stringify(lostPayload)}`);
                                }
                            }
                        }
                    }
                    
                    // Clear combat set
                    enemy.inCombatWith.clear();
                    
                    // Broadcast enemy death
                    const deathPayload = {
                        enemyId: enemy.enemyId,
                        enemyName: enemy.name,
                        killerId: attackerId,
                        killerName: attacker.characterName,
                        isEnemy: true,
                        exp: enemy.exp,
                        timestamp: now
                    };
                    io.emit('enemy:death', deathPayload);
                    logger.info(`[BROADCAST] enemy:death: ${JSON.stringify(deathPayload)}`);
                    
                    // Schedule enemy respawn
                    setTimeout(() => {
                        enemy.health = enemy.maxHealth;
                        enemy.isDead = false;
                        enemy.x = enemy.spawnX;
                        enemy.y = enemy.spawnY;
                        enemy.z = enemy.spawnZ;
                        enemy.targetPlayerId = null;
                        enemy.inCombatWith = new Set();
                        
                        logger.info(`[ENEMY] Respawned ${enemy.name} (ID: ${enemy.enemyId})`);
                        io.emit('enemy:spawn', {
                            enemyId: enemy.enemyId, templateId: enemy.templateId, name: enemy.name,
                            level: enemy.level, health: enemy.health, maxHealth: enemy.maxHealth,
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
            
            // === IN RANGE: Execute attack ===
            const damage = COMBAT.BASE_DAMAGE + Math.floor(Math.random() * (COMBAT.DAMAGE_VARIANCE + 1));
            target.health = Math.max(0, target.health - damage);
            attacker.lastAttackTime = now;
            
            logger.info(`[COMBAT] ${attacker.characterName} hit ${target.characterName} for ${damage} damage (HP: ${target.health}/${target.maxHealth})`);
            
            // Broadcast damage to all players (include positions for remote rotation)
            const damagePayload = {
                attackerId,
                attackerName: attacker.characterName,
                targetId: atkState.targetCharId,
                targetName: target.characterName,
                isEnemy: false,
                damage,
                targetHealth: target.health,
                targetMaxHealth: target.maxHealth,
                attackerX: attackerPos.x, attackerY: attackerPos.y, attackerZ: attackerPos.z,
                targetX: targetPos.x, targetY: targetPos.y, targetZ: targetPos.z,
                timestamp: now
            };
            io.emit('combat:damage', damagePayload);
            logger.info(`[BROADCAST] combat:damage: ${JSON.stringify(damagePayload)}`);
            
            // Check for player death
            if (target.health <= 0) {
                target.isDead = true;
                
                logger.info(`[COMBAT] ${target.characterName} was killed by ${attacker.characterName}`);
                
                // Stop all OTHER attackers targeting this dead player (not the killer — they get combat:death)
                for (const [otherId, otherAtk] of autoAttackState.entries()) {
                    if (otherAtk.targetCharId === atkState.targetCharId && !otherAtk.isEnemy) {
                        autoAttackState.delete(otherId);
                        // Only send target_lost to OTHER attackers, not the killer
                        if (otherId !== attackerId) {
                            const otherPlayer = connectedPlayers.get(otherId);
                            if (otherPlayer) {
                                const otherSocket = io.sockets.sockets.get(otherPlayer.socketId);
                                if (otherSocket) {
                                    const otherLostPayload = { reason: 'Target died', isEnemy: false };
                                    otherSocket.emit('combat:target_lost', otherLostPayload);
                                    logger.info(`[SEND] combat:target_lost to ${otherPlayer.socketId}: ${JSON.stringify(otherLostPayload)}`);
                                }
                            }
                        }
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
        
        // Create character
        const result = await pool.query(
            `INSERT INTO characters (user_id, name, class, level, x, y, z, health, mana) 
             VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) 
             RETURNING character_id, name, class, level, x, y, z, health, mana, created_at`,
            [req.user.user_id, name, charClass, 1, 0, 0, 0, 100, 100]
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
      ADD COLUMN IF NOT EXISTS max_mana INTEGER DEFAULT 100
    `);
    logger.info('[DB] Stat columns verified/created on characters table');
  } catch (err) {
    logger.warn(`[DB] Could not add stat columns (may already exist): ${err.message}`);
  }
  
  // Spawn initial enemies
  logger.info(`[ENEMY] Spawning ${ENEMY_SPAWNS.length} enemies...`);
  for (const spawnConfig of ENEMY_SPAWNS) {
    spawnEnemy(spawnConfig);
  }
  logger.info(`[ENEMY] ${enemies.size} enemies spawned. IDs: ${Array.from(enemies.keys()).join(', ')}`);
});

// Periodic health/mana save to DB (every 60 seconds)
setInterval(async () => {
  for (const [charId, player] of connectedPlayers.entries()) {
    if (!player.isDead) {
      await savePlayerHealthToDB(charId, player.health, player.mana);
    }
  }
}, 60000);
