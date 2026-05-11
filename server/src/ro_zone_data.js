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
//   enemySpawns   — fixed monster spawn configs: { template, x, y, z, wanderRadius }
//   spawnPool     — random spawn configs (optional): [{ template, count, wanderRadius? }]
//                   Random points are generated inside SpawnAllowVolume actors placed in
//                   the UE5 level (exported via "ExportSpawnRegions <zone>" → server/spawn_regions/<zone>.json).
//                   SpawnDenyVolume actors carve out exclusions. Points are NavMesh-snapped
//                   to ground. Zones may use enemySpawns and spawnPool together.
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
                destX: 11140, destY: 0, destZ: 50
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
                x: 11540, y: 0, z: 50,
                radius: 200,
                destZone: 'prontera',
                destX: -240, destY: -1700, destZ: 590
            },
            {
                // Repurposed: was prtsouth_to_dungeon → prt_dungeon_01.
                // The AWarpPortal actor in L_PrtSouth at (-650, 9560, -20) now points to SewerDungeon01.
                // prt_dungeon_01 is still reachable via Prontera Kafra teleport.
                id: 'prtsouth_to_sewerdungeon01',
                x: -650, y: 9560, z: -20,
                radius: 200,
                destZone: 'SewerDungeon01',
                destX: 3370, destY: -1390, destZ: 620
            },
            {
                id: 'prtsouth_to_grassfield07_2',
                x: -2800, y: -13920, z: 240,
                radius: 200,
                destZone: 'grassfield07',
                destX: 1147.034465, destY: 9059.677133, destZ: -450
            },
            {
                id: 'prtsouth_to_grassfield07',
                x: -7650, y: -11850, z: 40,
                radius: 200,
                destZone: 'grassfield07',
                destX: -7702.965535, destY: 9529.677133, destZ: -450
            },
            {
                // To Pryth — east gate of prtsouth -> west gate of Pryth
                id: 'prtsouth_to_pryth',
                x: 11960, y: -20, z: 50,
                radius: 400,
                destZone: 'Pryth',
                destX: -7500, destY: 395, destZ: 70
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
        enemySpawns: [],  // cleared — using spawnPool below instead. Original fixed spawns are in git history.
        // ── Random spawnPool — distributed across SpawnAllowVolume actors ─────
        // Uses server/spawn_regions/prontera_south.json. NavMesh-snapped to ground.
        // Tune counts here to scale total ambient enemy density up or down.
        spawnPool: [
            { template: 'poring',         count: 90 },                       // lvl 1
            { template: 'fabre',          count: 47 },                       // lvl 2
            { template: 'lunatic',        count: 35, wanderRadius: 350 },    // lvl 3
            { template: 'drops',          count: 29 },                       // lvl 3
            { template: 'chonchon',       count: 29, wanderRadius: 400 },    // lvl 4 (aggressive)
            { template: 'pupa',           count: 24, wanderRadius: 80  },    // lvl 2 (stationary)
            { template: 'thief_bug_egg',  count: 20, wanderRadius: 80  },    // lvl 4 (stationary)
            { template: 'thief_bug',      count: 20, wanderRadius: 400 },    // lvl 6 (aggressive)
            // Herb plants — bumped for the canonical "harvest plants first" Novice path.
            // 12 → 105 plants gives ~1,176u avg spacing (comparable to Porings) so a fresh
            // Novice has reasonable access to herbs from anywhere in the field.
            { template: 'red_plant',      count: 30, wanderRadius: 80  },    // lvl 1 (stationary, drops Red Herb)
            { template: 'blue_plant',     count: 15, wanderRadius: 80  },    // lvl 1 (stationary, drops Blue Herb)
            { template: 'green_plant',    count: 30, wanderRadius: 80  },    // lvl 1 (stationary, drops Green Herb — cures status)
            { template: 'yellow_plant',   count: 15, wanderRadius: 80  },    // lvl 1 (stationary, drops Yellow Herb)
            { template: 'white_plant',    count: 15, wanderRadius: 80  }     // lvl 1 (stationary, drops White Herb)
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
                destX: -1210, destY: 10570, destZ: 60
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
    },

    // ════════════════════════════════════════════════════════════
    // GRASS FIELD 07 — Hub field east of Prontera South
    // Connects to: prontera_south (2 portals), grassfield05 (2 portals)
    // ════════════════════════════════════════════════════════════
    grassfield07: {
        name: 'grassfield07',
        displayName: 'Grass Field 07',
        type: 'field',
        flags: {
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: false,
            indoor: false
        },
        defaultSpawn: { x: 1147, y: 9060, z: -450 },
        levelName: 'L_GrassField07',
        warps: [
            // ── To prontera_south ─────────────────────────────────
            {
                id: 'grassfield07_to_prtsouth_2',
                x: 897.034465, y: 9249.677133, z: -413.195045,
                radius: 200,
                destZone: 'prontera_south',
                destX: -2680, destY: -13580, destZ: 290
            },
            {
                id: 'grassfield07_to_prtsouth',
                x: -7832.965535, y: 9799.677133, z: -413.195045,
                radius: 200,
                destZone: 'prontera_south',
                destX: -7650, destY: -11550, destZ: 90
            },
            // ── To grassfield05 ───────────────────────────────────
            {
                id: 'grassfield07_to_grassfield05',
                x: 13964.862838, y: 2574.889124, z: 28.759324,
                radius: 200,
                destZone: 'grassfield05',
                destX: -14420, destY: 1830, destZ: 150
            },
            {
                id: 'grassfield07_to_grassfield05_2',
                x: 15384.862838, y: -7425.110876, z: 298.759324,
                radius: 200,
                destZone: 'grassfield05',
                destX: -14250, destY: -4260, destZ: 120
            }
        ],
        kafraNpcs: [],
        enemySpawns: [],  // cleared — using spawnPool below instead.
        // ── Random spawnPool — distributed across SpawnAllowVolume actors ─────
        // Uses server/spawn_regions/grassfield07.json. NavMesh-snapped to ground.
        spawnPool: [
            { template: 'rocker',         count: 120, wanderRadius: 350 },   // lvl 9 (detector)
            { template: 'poporing',       count: 30,  wanderRadius: 300 },   // lvl 14
            { template: 'black_mushroom', count: 8,   wanderRadius: 80  },   // lvl 1 (stationary, 4 min respawn)
            { template: 'vocal',          count: 1,   wanderRadius: 500 }    // lvl 18 (aggressive, 20 min respawn — rare named)
        ]
    },

    // ════════════════════════════════════════════════════════════
    // GRASS FIELD 05 — Western field
    // Connects to: grassfield07 (2 portals)
    // ════════════════════════════════════════════════════════════
    grassfield05: {
        name: 'grassfield05',
        displayName: 'Grass Field 05',
        type: 'field',
        flags: {
            noteleport: false,
            noreturn: false,
            nosave: false,
            pvp: false,
            town: false,
            indoor: false
        },
        defaultSpawn: { x: -14420, y: 1830, z: 150 },
        levelName: 'L_GrassField05',
        warps: [
            {
                id: 'grassfield05_to_grassfield07',
                x: -14890, y: 1830, z: 100,
                radius: 200,
                destZone: 'grassfield07',
                destX: 13694.862838, destY: 2784.889124, destZ: 78
            },
            {
                id: 'grassfield05_to_grassfield07_2',
                x: -14690, y: -4260, z: 70,
                radius: 200,
                destZone: 'grassfield07',
                destX: 15154.862838, destY: -7375.110876, destZ: 388
            },
            {
                // To Pryth — north gate of grassfield05 -> south gate of Pryth
                id: 'grassfield05_to_pryth',
                x: -3200, y: 11865, z: 80,
                radius: 400,
                destZone: 'Pryth',
                destX: -530, destY: -7800, destZ: 100
            }
        ],
        kafraNpcs: [],
        enemySpawns: [],  // cleared — using spawnPool below instead.
        // ── Random spawnPool — distributed across SpawnAllowVolume actors ─────
        // Uses server/spawn_regions/grassfield05.json. NavMesh-snapped to ground.
        // Tune counts here to scale total ambient enemy density up or down.
        spawnPool: [
            { template: 'poring',   count: 90 },                       // lvl 1
            { template: 'fabre',    count: 47 },                       // lvl 6
            { template: 'lunatic',  count: 35, wanderRadius: 350 },    // lvl 3
            { template: 'drops',    count: 29 },                       // lvl 3
            { template: 'chonchon', count: 29, wanderRadius: 400 },    // lvl 4 (aggressive)
            { template: 'pupa',     count: 24, wanderRadius: 80  }     // lvl 3 (stationary)
        ]
    },

    // ════════════════════════════════════════════════════════════
    // SEWER DUNGEON F1 — Prontera Culvert mimic
    // RO Reference: prt_sewb1
    // Level: L_SewerDungeon01
    // Replaces the prtsouth → prt_dungeon_01 warp at (-650, 9560, -20).
    // prt_dungeon_01 is still reachable via Prontera Kafra teleport.
    // Build plan: docsNew/05_Development/Sewer_Dungeon_01_Build_Plan.md
    // ════════════════════════════════════════════════════════════
    SewerDungeon01: {
        name: 'SewerDungeon01',
        displayName: 'Sewer Dungeon F1',
        type: 'dungeon',
        flags: {
            noteleport: false,    // Fly Wing allowed (RO Classic prt_sewb1 behavior)
            noreturn: false,      // Butterfly Wing works (back to last save point)
            nosave: true,         // Kafra cannot save here
            pvp: false,
            town: false,
            indoor: true          // skips outdoor lighting in PostProcessSubsystem
        },
        defaultSpawn: { x: 3370, y: -1390, z: 620 },  // matches the prtsouth → here warp destination
        levelName: 'L_SewerDungeon01',
        warps: [
            // Exit upward to Prontera South Field.
            // Widened radius (200 → 400) — the cramped sewer geometry made the default
            // hard to step into. Match the AWarpPortal actor's TriggerRadius in the level.
            {
                id: 'sewerdungeon01_to_prtsouth',
                x: 3750, y: -1390, z: 380,
                radius: 250,
                destZone: 'prontera_south',
                destX: -650, destY: 10290, destZ: 30
            }
            // F2 descent warp — uncomment when SewerDungeon02 ships.
            // AWarpPortal in L_SewerDungeon01 at (3990, 1350, 290) should currently be a
            // closed-gate static mesh (or AWarpPortal with a WarpId that doesn't match any
            // registered warp — server will emit zone:error). Players returning from F2
            // will spawn at (3680, 1350, 340) per the planned reverse warp.
            // {
            //     id: 'sewerdungeon01_to_sewerdungeon02',
            //     x: 3990, y: 1350, z: 290,
            //     radius: 200,
            //     destZone: 'SewerDungeon02',
            //     destX: 0, destY: 0, destZ: 0   // set when F2 is built
            // }
        ],
        kafraNpcs: [],
        enemySpawns: [],  // cleared — using spawnPool below.
        // ── Random spawnPool — distributed across SpawnAllowVolume actors ─────
        // Setup steps in UE5:
        //   1. Place SpawnAllowVolume actors covering walkable rooms/corridors in L_SewerDungeon01.
        //   2. (Optional) Place SpawnDenyVolume actors over canal water / unreachable nooks.
        //   3. Editor console: ExportSpawnRegions SewerDungeon01
        //      → writes server/spawn_regions/SewerDungeon01.json
        //   4. Restart server. Points are NavMesh-snapped to ground at runtime.
        // Total: 200 spawns (50 + 120 + 15 + 15) distributed evenly across the allow volumes.
        spawnPool: [
            { template: 'thief_bug',     count: 50,  wanderRadius: 350 },   // lvl 6  (aggressive)
            { template: 'thief_bug_egg', count: 120, wanderRadius: 80  },   // lvl 4  (stationary)
            { template: 'farmiliar',     count: 15,  wanderRadius: 400 },   // lvl 8  (flying bat, shadow-element)
            { template: 'tarou',         count: 15,  wanderRadius: 350 }    // lvl 11 (aggressive rat, shadow-element)
        ]
    },

    // ════════════════════════════════════════════════════════════
    // PRYTH — Pastoral capital town (Prontera-style mimic)
    // RO Reference: Prontera (`prontera`)
    // Level: L_Pryth
    // Connects to: prontera_south (east gate), grassfield05 (south gate)
    // Build plan: docsNew/05_Development/Pryth_Build_Plan.md
    // Phase A — Server registration (warps + zone scaffold). NPCs / spawns deferred.
    // ════════════════════════════════════════════════════════════
    Pryth: {
        name: 'Pryth',
        displayName: 'Pryth',
        type: 'town',
        flags: {
            noteleport: false,    // Fly Wing allowed (standard town)
            noreturn: false,      // Butterfly Wing allowed
            nosave: false,        // Kafra save allowed
            pvp: false,
            town: true,           // No enemy spawns
            indoor: false         // Outdoor lighting active
        },
        // Default spawn = matches prtsouth arrival point (most natural entry).
        defaultSpawn: { x: -7500, y: 395, z: 70 },
        levelName: 'L_Pryth',
        warps: [
            // West gate -> prontera_south (east edge of prtsouth)
            {
                id: 'pryth_to_prtsouth',
                x: -8410, y: 395, z: 20,
                radius: 400,
                destZone: 'prontera_south',
                destX: 11460, destY: -20, destZ: 100
            },
            // South gate -> grassfield05 (north edge of grassfield05)
            {
                id: 'pryth_to_grassfield05',
                x: -530, y: -8280, z: 50,
                radius: 400,
                destZone: 'grassfield05',
                destX: -3200, destY: 11400, destZ: 130
            }
        ],
        kafraNpcs: [
            {
                // South gate Kafra — wilderness/dungeon traveler theme.
                // AKafraNPC actor placed in L_Pryth at matching position.
                id: 'kafra_pryth_1',
                name: 'Kafra Employee',
                x: 715, y: -6825, z: 90,
                destinations: [
                    { zone: 'grassfield05',   displayName: 'Grass Field 05',       cost: 100 },
                    { zone: 'prt_dungeon_01', displayName: 'Prontera Dungeon 1F',  cost: 300 }
                ]
            },
            {
                // West gate Kafra — Prontera-area traveler theme.
                // AKafraNPC actor placed in L_Pryth at matching position.
                id: 'kafra_pryth_2',
                name: 'Kafra Employee',
                x: -4920, y: 675, z: 90,
                destinations: [
                    { zone: 'prontera_south', displayName: 'Prontera South Field', cost: 100 },
                    { zone: 'prontera_north', displayName: 'Prontera North Field', cost: 200 }
                ]
            }
        ],
        enemySpawns: []         // Town zone — no enemies
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
