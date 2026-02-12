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
    MELEE_RANGE: 150,           // Default melee attack range (Unreal units)
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
        stats: { str: 1, agi: 1, vit: 1, int: 1, dex: 1, luk: 1, level: 1, weaponATK: 0 },
        drops: [
            { itemId: 2001, chance: 0.70, minQty: 1, maxQty: 2 }, // Gloopy Residue
            { itemId: 2002, chance: 0.15 },                        // Viscous Slime
            { itemId: 1001, chance: 0.05 },                        // Crimson Vial
            { itemId: 3001, chance: 0.01 }                         // Rustic Shiv (rare)
        ]
    },
    hoplet: {
        name: 'Hoplet', level: 3, maxHealth: 100, damage: 3,
        attackRange: 80, aggroRange: 400, aspd: 178, exp: 25,
        respawnMs: 15000, aiType: 'passive',
        stats: { str: 3, agi: 5, vit: 2, int: 1, dex: 3, luk: 5, level: 3, weaponATK: 2 },
        drops: [
            { itemId: 2001, chance: 0.50 },                        // Gloopy Residue
            { itemId: 2004, chance: 0.30 },                        // Downy Plume
            { itemId: 2007, chance: 0.10 },                        // Verdant Leaf
            { itemId: 1001, chance: 0.08 },                        // Crimson Vial
            { itemId: 3002, chance: 0.01 }                         // Keen Edge (rare)
        ]
    },
    crawlid: {
        name: 'Crawlid', level: 2, maxHealth: 75, damage: 2,
        attackRange: 80, aggroRange: 0, aspd: 176, exp: 15,
        respawnMs: 12000, aiType: 'passive',
        stats: { str: 2, agi: 2, vit: 3, int: 1, dex: 2, luk: 1, level: 2, weaponATK: 1 },
        drops: [
            { itemId: 2003, chance: 0.50 },                        // Chitin Shard
            { itemId: 2006, chance: 0.25 },                        // Barbed Limb
            { itemId: 2001, chance: 0.40 },                        // Gloopy Residue
            { itemId: 1001, chance: 0.05 }                         // Crimson Vial
        ]
    },
    shroomkin: {
        name: 'Shroomkin', level: 4, maxHealth: 120, damage: 4,
        attackRange: 80, aggroRange: 350, aspd: 177, exp: 30,
        respawnMs: 15000, aiType: 'passive',
        stats: { str: 4, agi: 3, vit: 4, int: 2, dex: 3, luk: 2, level: 4, weaponATK: 3 },
        drops: [
            { itemId: 2005, chance: 0.55 },                        // Spore Cluster
            { itemId: 2007, chance: 0.20, minQty: 1, maxQty: 2 },  // Verdant Leaf
            { itemId: 1001, chance: 0.10 },                        // Crimson Vial
            { itemId: 4001, chance: 0.02 }                         // Linen Tunic (rare)
        ]
    },
    buzzer: {
        name: 'Buzzer', level: 5, maxHealth: 150, damage: 5,
        attackRange: 80, aggroRange: 500, aspd: 179, exp: 40,
        respawnMs: 18000, aiType: 'aggressive',
        stats: { str: 5, agi: 7, vit: 3, int: 1, dex: 5, luk: 3, level: 5, weaponATK: 4 },
        drops: [
            { itemId: 2006, chance: 0.45 },                        // Barbed Limb
            { itemId: 2004, chance: 0.30 },                        // Downy Plume
            { itemId: 1002, chance: 0.05 },                        // Amber Elixir
            { itemId: 3004, chance: 0.02 },                        // Iron Cleaver (rare)
            { itemId: 4002, chance: 0.01 },                        // Quilted Vest (rare)
            { itemId: 3006, chance: 0.005 }                        // Hunting Longbow (very rare)
        ]
    },
    mosswort: {
        name: 'Mosswort', level: 3, maxHealth: 5, damage: 2,
        attackRange: 80, aggroRange: 0, aspd: 174, exp: 20,
        respawnMs: 12000, aiType: 'passive',
        stats: { str: 2, agi: 1, vit: 5, int: 3, dex: 2, luk: 1, level: 3, weaponATK: 1 },
        drops: [
            { itemId: 2008, chance: 0.60, minQty: 1, maxQty: 3 },  // Silken Tuft
            { itemId: 2007, chance: 0.25 },                        // Verdant Leaf
            { itemId: 2002, chance: 0.15 },                        // Viscous Slime
            { itemId: 1005, chance: 0.08 }                         // Roasted Haunch
        ]
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

// ============================================================
// Item & Inventory System
// ============================================================
const INVENTORY = {
    MAX_SLOTS: 100,           // Max inventory slots per character
    MAX_WEIGHT: 2000          // Max carry weight (future use)
};

// Item definitions cache (loaded from DB on startup)
const itemDefinitions = new Map();

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

// Roll drops for a killed enemy — returns array of { itemId, quantity, itemName }
function rollEnemyDrops(enemy) {
    const template = ENEMY_TEMPLATES[enemy.templateId];
    if (!template || !template.drops) return [];
    
    const droppedItems = [];
    for (const drop of template.drops) {
        if (Math.random() < drop.chance) {
            const qty = drop.minQty && drop.maxQty
                ? drop.minQty + Math.floor(Math.random() * (drop.maxQty - drop.minQty + 1))
                : 1;
            const itemDef = itemDefinitions.get(drop.itemId);
            droppedItems.push({
                itemId: drop.itemId,
                quantity: qty,
                itemName: itemDef ? itemDef.name : `Item#${drop.itemId}`
            });
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
        
        // Insert new inventory entry
        const result = await pool.query(
            'INSERT INTO character_inventory (character_id, item_id, quantity) VALUES ($1, $2, $3) RETURNING inventory_id',
            [characterId, itemId, quantity]
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
            aspd: Math.min(COMBAT.ASPD_CAP, (derived.aspd || COMBAT.DEFAULT_ASPD) + weaponAspdMod),
            attackRange: weaponRange,
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
        const characterId = parseInt(data.characterId);
        const { x, y, z } = data;
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
    
    // ============================================================
    // Inventory Events
    // ============================================================
    
    // Load full inventory
    socket.on('inventory:load', async () => {
        logger.info(`[RECV] inventory:load from ${socket.id}`);
        const playerInfo = findPlayerBySocketId(socket.id);
        if (!playerInfo) return;
        
        const inventory = await getPlayerInventory(playerInfo.characterId);
        socket.emit('inventory:data', { items: inventory });
        logger.info(`[SEND] inventory:data to ${socket.id}: ${inventory.length} items`);
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
            socket.emit('inventory:data', { items: inventory });
            
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
        const equip = data.equip !== false; // true = equip, false = unequip
        
        try {
            const result = await pool.query(
                `SELECT ci.inventory_id, ci.item_id, ci.is_equipped, i.item_type, i.equip_slot, i.name,
                        i.atk, i.def, i.matk, i.mdef,
                        i.str_bonus, i.agi_bonus, i.vit_bonus, i.int_bonus, i.dex_bonus, i.luk_bonus,
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
            
            if (equip) {
                // Unequip any item currently in this slot
                await pool.query(
                    `UPDATE character_inventory SET is_equipped = false 
                     WHERE character_id = $1 AND is_equipped = true 
                     AND item_id IN (SELECT item_id FROM items WHERE equip_slot = $2)`,
                    [characterId, item.equip_slot]
                );
                
                // Equip the new item
                await pool.query(
                    'UPDATE character_inventory SET is_equipped = true WHERE inventory_id = $1',
                    [inventoryId]
                );
                
                // Update weapon stats if equipping a weapon
                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = item.atk || 0;
                    player.attackRange = item.weapon_range || COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = item.aspd_modifier || 0;
                }
                
                logger.info(`[ITEMS] ${player.characterName} equipped ${item.name} (slot: ${item.equip_slot}, type: ${item.weapon_type}, range: ${item.weapon_range}, aspdMod: ${item.aspd_modifier})`);
            } else {
                // Unequip
                await pool.query(
                    'UPDATE character_inventory SET is_equipped = false WHERE inventory_id = $1',
                    [inventoryId]
                );
                
                if (item.equip_slot === 'weapon') {
                    player.stats.weaponATK = 0;
                    player.attackRange = COMBAT.MELEE_RANGE;
                    player.weaponAspdMod = 0;
                }
                
                logger.info(`[ITEMS] ${player.characterName} unequipped ${item.name}`);
            }
            
            // Recalculate derived stats (include weapon ASPD modifier)
            const derived = calculateDerivedStats(player.stats);
            player.aspd = Math.min(COMBAT.ASPD_CAP, derived.aspd + (player.weaponAspdMod || 0));
            
            // Send updated stats
            socket.emit('player:stats', { characterId, stats: player.stats, derived });
            
            // Send updated inventory
            const inventory = await getPlayerInventory(characterId);
            socket.emit('inventory:data', { items: inventory });
            
            socket.emit('inventory:equipped', {
                inventoryId, itemId: item.item_id, itemName: item.name,
                equipped: equip, slot: item.equip_slot,
                weaponType: item.weapon_type, attackRange: player.attackRange,
                aspd: player.aspd, attackIntervalMs: getAttackIntervalMs(player.aspd)
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
            socket.emit('inventory:data', { items: inventory });
            
        } catch (err) {
            logger.error(`[ITEMS] Drop error: ${err.message}`);
            socket.emit('inventory:error', { message: 'Failed to drop item' });
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
                    
                    // Broadcast enemy death (isDead flag tells client to disable collision + hide mesh)
                    const deathPayload = {
                        enemyId: enemy.enemyId,
                        enemyName: enemy.name,
                        killerId: attackerId,
                        killerName: attacker.characterName,
                        isEnemy: true,
                        isDead: true,
                        exp: enemy.exp,
                        timestamp: now
                    };
                    io.emit('enemy:death', deathPayload);
                    logger.info(`[BROADCAST] enemy:death: ${JSON.stringify(deathPayload)}`);
                    
                    // Roll item drops for the killer
                    const droppedItems = rollEnemyDrops(enemy);
                    if (droppedItems.length > 0) {
                        const lootItems = [];
                        for (const drop of droppedItems) {
                            const added = await addItemToInventory(attackerId, drop.itemId, drop.quantity);
                            if (added) {
                                const itemDef = itemDefinitions.get(drop.itemId);
                                lootItems.push({
                                    itemId: drop.itemId,
                                    itemName: drop.itemName,
                                    quantity: drop.quantity,
                                    icon: itemDef ? itemDef.icon : 'default_item',
                                    itemType: itemDef ? itemDef.item_type : 'etc'
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
                                logger.info(`[LOOT] ${attacker.characterName} received ${lootItems.length} items from ${enemy.name}: ${lootItems.map(i => `${i.quantity}x ${i.itemName}`).join(', ')}`);
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
                logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) wandering from (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)}) to (${target.x.toFixed(0)}, ${target.y.toFixed(0)})`);
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
                logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) arrived at (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)})`);
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
                    logger.info(`[ENEMY AI] ${enemy.name}(${enemyId}) moving — pos (${enemy.x.toFixed(0)}, ${enemy.y.toFixed(0)}) → target (${enemy.wanderTargetX.toFixed(0)}, ${enemy.wanderTargetY.toFixed(0)})`);
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
  
  // Load item definitions into memory cache
  await loadItemDefinitions();
  
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
