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
        guideNpcs: [
            {
                id: 'guide_prontera_1',
                name: 'Guide',
                x: -100, y: -1600, z: 590,
                facilities: [
                    { name: 'Weapon Shop', x: 120, y: -800, z: 590, color: 0xFF4444 },
                    { name: 'Armor Shop', x: -300, y: -600, z: 590, color: 0x4444FF },
                    { name: 'Tool Shop', x: 280, y: -1200, z: 590, color: 0x44FF44 },
                    { name: 'Inn', x: -200, y: -1000, z: 590, color: 0xFFFF44 },
                    { name: 'Kafra Service', x: 200, y: -1800, z: 300, color: 0xFF44FF },
                    { name: 'Blacksmith', x: 400, y: -500, z: 590, color: 0xFF8844 }
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
                destX: -240, destY: -1700, destZ: 590
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
        waterAreas: [
            {
                id: 'prtsouth_pond_01',
                type: 'shallow',
                x: 300, y: -800, z: 90,
                extentX: 300, extentY: 200,
                waterLevel: 100
            }
        ],
        enemySpawns: [
            // ── Classic prt_fild08 low-level spawns (RO Reference) ────────
            // Poring family (passive plant), Fabre/Pupa (passive insect), Lunatic (passive brute),
            // Chonchon (aggressive insect), Picky (aggressive brute). Scattered across central
            // playable area near defaultSpawn (1330, 0) so new players find them easily.
            { template: 'poring',   x:  1100, y:  -300, z: 90, wanderRadius: 350 },
            { template: 'poring',   x:   700, y:  -600, z: 90, wanderRadius: 350 },
            { template: 'poring',   x:   200, y:  -200, z: 90, wanderRadius: 350 },
            { template: 'poring',   x:  -300, y:  -500, z: 90, wanderRadius: 350 },
            { template: 'poring',   x:   900, y: -1400, z: 90, wanderRadius: 350 },
            { template: 'poring',   x:    50, y: -1700, z: 90, wanderRadius: 350 },
            { template: 'fabre',    x:   500, y:  -100, z: 90, wanderRadius: 300 },
            { template: 'fabre',    x:  -100, y: -1000, z: 90, wanderRadius: 300 },
            { template: 'fabre',    x:  1200, y: -1100, z: 90, wanderRadius: 300 },
            { template: 'fabre',    x:  -700, y: -1400, z: 90, wanderRadius: 300 },
            { template: 'pupa',     x:   400, y:  -300, z: 90, wanderRadius: 80  },
            { template: 'pupa',     x:  -200, y: -1200, z: 90, wanderRadius: 80  },
            { template: 'lunatic',  x:  -800, y:  -300, z: 90, wanderRadius: 350 },
            { template: 'lunatic',  x: -1100, y:  -800, z: 90, wanderRadius: 350 },
            { template: 'lunatic',  x: -1300, y: -1500, z: 90, wanderRadius: 350 },
            { template: 'drops',    x:  1400, y:  -800, z: 90, wanderRadius: 350 },
            { template: 'drops',    x:   800, y: -2100, z: 90, wanderRadius: 350 },
            { template: 'chonchon', x:   100, y: -2300, z: 90, wanderRadius: 400 },
            { template: 'chonchon', x: -1000, y: -2000, z: 90, wanderRadius: 400 },
            { template: 'picky',    x:   600, y: -2700, z: 90, wanderRadius: 350 },
            { template: 'picky',    x: -1500, y:  -500, z: 90, wanderRadius: 350 },

            // ── Sprite enemy test column (x=-2150, 40 enemies) ────────
            { template: 'ork_warrior',      x: -2150, y: -4100, z: 90,     wanderRadius: 50 },
            { template: 'zerom',            x: -2150, y: -4200, z: 90,     wanderRadius: 50 },
            { template: 'raggler',          x: -2150, y: -4300, z: 90,     wanderRadius: 50 },
            { template: 'orc_zombie',       x: -2150, y: -4350, z: 90.151, wanderRadius: 50 },
            { template: 'smoking_orc',      x: -2150, y: -4400, z: 90.151, wanderRadius: 50 },
            { template: 'orc_xmas',         x: -2150, y: -4450, z: 90.151, wanderRadius: 50 },
            { template: 'golem',            x: -2150, y: -4500, z: 90.151, wanderRadius: 50 },
            { template: 'pirate_skel',      x: -2150, y: -4550, z: 90.151, wanderRadius: 50 },
            { template: 'gobline_xmas',     x: -2150, y: -4600, z: 90.151, wanderRadius: 50 },
            { template: 'rotar_zairo',      x: -2150, y: -4750, z: 90.151, wanderRadius: 50 },
            { template: 'magnolia',         x: -2150, y: -4850, z: 90.151, wanderRadius: 50 },
            { template: 'rice_cake_boy',    x: -2150, y: -4950, z: 90.151, wanderRadius: 50 },
            { template: 'orc_skeleton',     x: -2150, y: -5050, z: 90.151, wanderRadius: 50 },
            { template: 'goblin_archer',    x: -2150, y: -5150, z: 90.151, wanderRadius: 50 },
            { template: 'soldier_skeleton', x: -2150, y: -5350, z: 90.151, wanderRadius: 50 },
            { template: 'sasquatch',        x: -2150, y: -5550, z: 90.151, wanderRadius: 50 },
            { template: 'kobold_1',         x: -2150, y: -5650, z: 90.151, wanderRadius: 50 },
            { template: 'zenorc',           x: -2150, y: -5750, z: 90.151, wanderRadius: 50 },
            { template: 'orc_lady',         x: -2150, y: -5850, z: 90.151, wanderRadius: 50 },
            { template: 'dokebi',           x: -2150, y: -5950, z: 90.151, wanderRadius: 50 },
            { template: 'bogun',            x: -2150, y: -6050, z: 90.151, wanderRadius: 50 },
            { template: 'kobold_archer',    x: -2150, y: -6150, z: 90.151, wanderRadius: 50 },
            { template: 'miyabi_ningyo',    x: -2150, y: -6350, z: 90.151, wanderRadius: 50 },
            { template: 'sand_man',         x: -2150, y: -6550, z: 90.151, wanderRadius: 50 },
            { template: 'requiem',          x: -2150, y: -6950, z: 90.151, wanderRadius: 50 },
            { template: 'steam_goblin',     x: -2150, y: -7150, z: 90.151, wanderRadius: 50 },
            { template: 'mummy',            x: -2150, y: -7350, z: 90.151, wanderRadius: 50 },
            { template: 'lude',             x: -2150, y: -7550, z: 90.151, wanderRadius: 50 },
            { template: 'zipper_bear',      x: -2150, y: -7750, z: 90.151, wanderRadius: 50 },
            { template: 'verit',            x: -2150, y: -7950, z: 90.151, wanderRadius: 50 },
            { template: 'ghoul',            x: -2150, y: -8150, z: 90.151, wanderRadius: 50 },
            { template: 'wootan_shooter',   x: -2150, y: -8350, z: 90.151, wanderRadius: 50 },
            { template: 'marduk',           x: -2150, y: -8550, z: 90.151, wanderRadius: 50 },
            { template: 'mime_monkey',      x: -2150, y: -8750, z: 90.151, wanderRadius: 50 },
            { template: 'marionette',       x: -2150, y: -8950, z: 90.151, wanderRadius: 50 },
            { template: 'wootan_fighter',   x: -2150, y: -9150, z: 90.151, wanderRadius: 50 },
            { template: 'pitman',           x: -2150, y: -9350, z: 90.151, wanderRadius: 50 },
            { template: 'bathory',          x: -2150, y: -9550, z: 90.151, wanderRadius: 50 },
            { template: 'megalith',         x: -2150, y: -9750, z: 90.151, wanderRadius: 50 },
            { template: 'baphomet_',        x: -2150, y: -9850, z: 90.151, wanderRadius: 50 },
            { template: 'deviruchi',        x: -2150, y: -9950, z: 90.151, wanderRadius: 50 },
            { template: 'knocker',          x: -2150, y: -10500, z: 90.151, wanderRadius: 50 },
            { template: 'li_me_mang_ryang', x: -2150, y: -10700, z: 90.151, wanderRadius: 50 },
            // ── Duplicate spawns of templates in the keep list (Batch 6 leftovers) ────────
            { template: 'raggler',          x: -4200, y: -11000, z: 300, wanderRadius: 400 },
            { template: 'zerom',            x: -4800, y: -11200, z: 300, wanderRadius: 350 },
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
