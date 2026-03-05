// ============================================================
// RO Classic Zone Registry — Map definitions, warps, spawns, Kafra NPCs
// Pattern: same as ro_monster_templates.js (exported constant object)
// ============================================================
//
// Each zone defines:
//   name          — unique zone ID (used in DB, socket rooms, etc.)
//   displayName   — human-readable name shown in UI
//   type          — 'town' | 'field' | 'dungeon'
//   flags         — RO mapflags: noteleport, noreturn, nosave, pvp, town, indoor
//   defaultSpawn  — { x, y, z } where players appear when entering this zone
//   levelName     — UE5 level file name for OpenLevel()
//   warps         — portal definitions: { id, x, y, z, radius, destZone, destX, destY, destZ }
//   kafraNpcs     — Kafra NPC definitions: { id, name, x, y, z, destinations[] }
//   enemySpawns   — monster spawn configs: { template, x, y, z, wanderRadius }
//
// Coordinate system: UE units (50 UE units ≈ 1 RO cell)
// ============================================================

const ZONE_REGISTRY = {

    // ════════════════════════════════════════════════════════════
    // PRONTERA — Capital Town
    // RO Reference: prontera (312×392 cells)
    // Scaled to ~5000×6000 UE units for playability
    // ════════════════════════════════════════════════════════════
    prontera: {
        name: 'prontera',
        displayName: 'Prontera',
        type: 'town',
        flags: {
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: true,
            indoor: false
        },
        defaultSpawn: { x: -240, y: -1700, z: 590 },  // south gate entry
        levelName: 'L_Prontera',
        warps: [
            {
                id: 'prt_south_exit',
                x: 30, y: -2650, z: 490,
                radius: 200,
                destZone: 'prontera_south',
                destX: 1330, destY: 0, destZ: 90
            },
            {
                id: 'prt_north_exit',
                x: -230, y: 7790, z: 520,
                radius: 200,
                destZone: 'prontera_north',
                destX: -1330, destY: 0, destZ: 90
            }
        ],
        kafraNpcs: [
            {
                id: 'kafra_prontera_1',
                name: 'Kafra Employee',
                x: 200, y: -1800, z: 300,
                destinations: [
                    { zone: 'prontera_south', displayName: 'Prontera South Field', cost: 100 },
                    { zone: 'prontera_north', displayName: 'Prontera North Field', cost: 150 },
                    { zone: 'prt_dungeon_01', displayName: 'Prontera Dungeon 1F', cost: 300 }
                ]
            }
        ],
        enemySpawns: []  // towns have no enemies
    },

    // ════════════════════════════════════════════════════════════
    // PRONTERA SOUTH FIELD — Starter Area
    // RO Reference: prt_fild08 (400×400 cells)
    // Level: L_PrtSouth
    // Contains zone 1+2 monsters from original ENEMY_SPAWNS
    // ════════════════════════════════════════════════════════════
    prontera_south: {
        name: 'prontera_south',
        displayName: 'Prontera South Field',
        type: 'field',
        flags: {
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: false,
            indoor: false
        },
        defaultSpawn: { x: 1330, y: 0, z: 90 },
        levelName: 'L_PrtSouth',
        warps: [
            {
                id: 'prtsouth_to_prt',
                x: 1740, y: 0, z: 0,
                radius: 200,
                destZone: 'prontera',
                destX: 0, destY: -2140, destZ: 580
            },
            {
                id: 'prtsouth_to_dungeon',
                x: -1760, y: -1750, z: 200,
                radius: 200,
                destZone: 'prt_dungeon_01',
                destX: -1000, destY: -1400, destZ: 150
            }
        ],
        kafraNpcs: [],
        enemySpawns: [
            // ── Zone 1: Starter mobs (Level 1-10) ────────
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
            // ── Zone 2: Prontera Fields mobs (Level 5-15) ────────
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
            { template: 'savage_babe',  x: -1100, y: -1100, z: 300, wanderRadius: 350 }
        ]
    },

    // ════════════════════════════════════════════════════════════
    // PRONTERA NORTH FIELD — Higher level field
    // RO Reference: prt_fild01 (400×400 cells)
    // Contains zone 3 monsters from original ENEMY_SPAWNS
    // ════════════════════════════════════════════════════════════
    prontera_north: {
        name: 'prontera_north',
        displayName: 'Prontera North Field',
        type: 'field',
        flags: {
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: false,
            indoor: false
        },
        defaultSpawn: { x: -1330, y: 0, z: 90 },
        levelName: 'L_PrtNorth',
        warps: [
            {
                id: 'prtnorth_to_prt',
                x: -1720, y: 0, z: 0,
                radius: 200,
                destZone: 'prontera',
                destX: 0, destY: 7440, destZ: 580
            }
        ],
        kafraNpcs: [],
        enemySpawns: [
            // ── Zone 3: Payon/Morroc Fields mobs (Level 10-20) ────────
            { template: 'spore',        x: 300,   y: 600,   z: 300, wanderRadius: 500 },
            { template: 'spore',        x: -400,  y: 700,   z: 300, wanderRadius: 500 },
            { template: 'zombie',       x: 800,   y: -300,  z: 300, wanderRadius: 400 },
            { template: 'zombie',       x: -700,  y: -400,  z: 300, wanderRadius: 400 },
            { template: 'zombie',       x: 500,   y: -800,  z: 300, wanderRadius: 400 },
            { template: 'skeleton',     x: 1000,  y: 200,   z: 300, wanderRadius: 450 },
            { template: 'skeleton',     x: -900,  y: 300,   z: 300, wanderRadius: 450 },
            { template: 'creamy',       x: 600,   y: 1100,  z: 300, wanderRadius: 400 },
            { template: 'poporing',     x: -500,  y: 1000,  z: 300, wanderRadius: 400 },
            { template: 'poporing',     x: 700,   y: 900,   z: 300, wanderRadius: 400 },
            { template: 'pecopeco',     x: 1200,  y: -600,  z: 300, wanderRadius: 500 },
            { template: 'pecopeco',     x: -1100, y: -700,  z: 300, wanderRadius: 500 },
            { template: 'mandragora',   x: 1300,  y: 800,   z: 300, wanderRadius: 300 },
            { template: 'poison_spore', x: 800,   y: -1200, z: 300, wanderRadius: 400 },
            { template: 'smokie',       x: -600,  y: -1300, z: 300, wanderRadius: 400 },
            { template: 'yoyo',         x: 1000,  y: 1200,  z: 300, wanderRadius: 450 }
        ]
    },

    // ════════════════════════════════════════════════════════════
    // PRONTERA DUNGEON FLOOR 1 — Underground Culvert
    // RO Reference: prt_sewb1 (320×320 cells)
    // Flags: noteleport + noreturn + nosave (classic dungeon rules)
    // ════════════════════════════════════════════════════════════
    prt_dungeon_01: {
        name: 'prt_dungeon_01',
        displayName: 'Prontera Dungeon 1F',
        type: 'dungeon',
        flags: {
            noteleport: true,   // Fly Wing blocked
            noreturn: true,     // Butterfly Wing blocked
            nosave: true,       // Cannot save here
            pvp: false,
            town: false,
            indoor: true
        },
        defaultSpawn: { x: -1000, y: -1400, z: 150 },
        levelName: 'L_PrtDungeon01',
        warps: [
            {
                id: 'prtdun1_to_surface',
                x: -1720, y: -1420, z: 0,
                radius: 150,
                destZone: 'prontera_south',
                destX: -1490, destY: -1450, destZ: 300
            }
        ],
        kafraNpcs: [],
        enemySpawns: [
            // Undead/vermin themed — classic Prontera Culvert monsters
            { template: 'skeleton',     x: 500,   y: 200,   z: 300, wanderRadius: 350 },
            { template: 'skeleton',     x: -400,  y: -300,  z: 300, wanderRadius: 350 },
            { template: 'skeleton',     x: 200,   y: -700,  z: 300, wanderRadius: 350 },
            { template: 'zombie',       x: -600,  y: 400,   z: 300, wanderRadius: 300 },
            { template: 'zombie',       x: 700,   y: -200,  z: 300, wanderRadius: 300 },
            { template: 'zombie',       x: -300,  y: -500,  z: 300, wanderRadius: 300 },
            { template: 'farmiliar',    x: 400,   y: 600,   z: 300, wanderRadius: 400 },
            { template: 'farmiliar',    x: -500,  y: -600,  z: 300, wanderRadius: 400 },
            { template: 'poison_spore', x: 800,   y: 300,   z: 300, wanderRadius: 350 },
            { template: 'poison_spore', x: -700,  y: 500,   z: 300, wanderRadius: 350 }
        ]
    }
};

// ============================================================
// Helper: Get zone definition by name (null-safe)
// ============================================================
function getZone(zoneName) {
    return ZONE_REGISTRY[zoneName] || null;
}

// ============================================================
// Helper: Get all enemy spawns across all zones as flat array
// (for backwards compat during migration, if needed)
// ============================================================
function getAllEnemySpawns() {
    const allSpawns = [];
    for (const [zoneName, zone] of Object.entries(ZONE_REGISTRY)) {
        for (const spawn of zone.enemySpawns) {
            allSpawns.push({ ...spawn, zone: zoneName });
        }
    }
    return allSpawns;
}

// ============================================================
// Helper: Get all zone names
// ============================================================
function getZoneNames() {
    return Object.keys(ZONE_REGISTRY);
}

module.exports = { ZONE_REGISTRY, getZone, getAllEnemySpawns, getZoneNames };
