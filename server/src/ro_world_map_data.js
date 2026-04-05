// ============================================================
// RO Classic World Map Data — Zone layout, connections, world map bounds
// Used by map:world_data socket event to populate client minimap + world map
// ============================================================

const { ZONE_REGISTRY } = require('./ro_zone_data');
const { ENEMY_TEMPLATES } = require('./ro_monster_templates');

// ════════════════════════════════════════════════════════════
// World Map Grid System
// The map is divided into a COLS x ROWS grid.
// Each cell can be assigned to a zone. Hovering over a cell
// highlights it and shows zone info (name, level, monsters).
// Ocean cells have no zone assigned (null).
//
// Grid coordinates: col 0 = left edge, row 0 = top edge
// Normalized bounds computed automatically from grid position.
// ════════════════════════════════════════════════════════════

const GRID_COLS = 12;
const GRID_ROWS = 8;

// Zone definition lookup — each zone's metadata
// EVERY grid cell that isn't ocean gets a UNIQUE zone ID (like RO Classic's prt_fild01..11)
const ZONE_INFO = {

    // ══════════════════════════════════════════════════════
    // PRONTERA REGION — Green meadows (center)
    // ══════════════════════════════════════════════════════
    prontera:       { displayName: 'Prontera',              category: 'town',    levelRange: '',       biome: 'temperate_meadow' },
    prontera_south: { displayName: 'Prontera South Field',  category: 'field',   levelRange: '1-15',   biome: 'temperate_meadow' },
    prontera_north: { displayName: 'Prontera North Field',  category: 'field',   levelRange: '10-25',  biome: 'temperate_meadow' },
    prt_fild01:     { displayName: 'Prontera Field 01',     category: 'field',   levelRange: '1-10',   biome: 'temperate_meadow' },
    prt_fild02:     { displayName: 'Prontera Field 02',     category: 'field',   levelRange: '5-15',   biome: 'temperate_meadow' },
    prt_dungeon_01: { displayName: 'Prontera Dungeon 1F',   category: 'dungeon', levelRange: '15-30',  biome: 'underground' },
    izlude:         { displayName: 'Izlude',                category: 'town',    levelRange: '',       biome: 'coastal' },
    iz_fild01:      { displayName: 'Izlude Field',          category: 'field',   levelRange: '5-20',   biome: 'coastal' },
    alberta:        { displayName: 'Alberta',               category: 'town',    levelRange: '',       biome: 'coastal' },
    alb_fild01:     { displayName: 'Alberta Field',         category: 'field',   levelRange: '10-20',  biome: 'coastal' },

    // ══════════════════════════════════════════════════════
    // GEFFEN REGION — Magic forest (west)
    // ══════════════════════════════════════════════════════
    geffen:         { displayName: 'Geffen',                category: 'town',    levelRange: '',       biome: 'magic_forest' },
    gef_fild01:     { displayName: 'Geffen Field 01',       category: 'field',   levelRange: '15-25',  biome: 'magic_forest' },
    gef_fild02:     { displayName: 'Geffen Field 02',       category: 'field',   levelRange: '20-30',  biome: 'magic_forest' },
    gef_fild03:     { displayName: 'Orc Territory',         category: 'field',   levelRange: '25-35',  biome: 'enchanted_woods' },
    gef_fild04:     { displayName: 'Enchanted Woods',       category: 'field',   levelRange: '20-35',  biome: 'enchanted_woods' },
    glast_heim:     { displayName: 'Glast Heim',            category: 'dungeon', levelRange: '50-80',  biome: 'cursed_ruins' },
    gl_fild01:      { displayName: 'Glast Heim Outskirts',  category: 'field',   levelRange: '45-65',  biome: 'cursed_ruins' },

    // ══════════════════════════════════════════════════════
    // PAYON REGION — Bamboo forest (east/southeast)
    // ══════════════════════════════════════════════════════
    payon:          { displayName: 'Payon',                 category: 'town',    levelRange: '',       biome: 'bamboo_forest' },
    pay_fild01:     { displayName: 'Payon Forest 01',       category: 'field',   levelRange: '10-20',  biome: 'bamboo_forest' },
    pay_fild02:     { displayName: 'Payon Forest 02',       category: 'field',   levelRange: '15-25',  biome: 'bamboo_forest' },
    pay_fild03:     { displayName: 'Payon Valley',          category: 'field',   levelRange: '18-28',  biome: 'bamboo_forest' },

    // ══════════════════════════════════════════════════════
    // MORROC REGION — Desert (south)
    // ══════════════════════════════════════════════════════
    morroc:         { displayName: 'Morroc',                category: 'town',    levelRange: '',       biome: 'desert' },
    moc_fild01:     { displayName: 'Sograt Desert 01',      category: 'field',   levelRange: '20-30',  biome: 'desert' },
    moc_fild02:     { displayName: 'Sograt Desert 02',      category: 'field',   levelRange: '22-32',  biome: 'desert' },
    moc_fild03:     { displayName: 'Sograt Desert 03',      category: 'field',   levelRange: '25-35',  biome: 'desert' },
    moc_fild04:     { displayName: 'Sograt Desert 04',      category: 'field',   levelRange: '28-38',  biome: 'desert' },
    moc_fild05:     { displayName: 'Deep Desert',           category: 'field',   levelRange: '30-40',  biome: 'desert' },
    moc_pryd:       { displayName: 'Pyramids',              category: 'dungeon', levelRange: '25-45',  biome: 'desert' },
    sphinx:         { displayName: 'Sphinx',                category: 'dungeon', levelRange: '30-50',  biome: 'desert' },

    // ══════════════════════════════════════════════════════
    // COMODO — Tropical (far south)
    // ══════════════════════════════════════════════════════
    comodo:         { displayName: 'Comodo',                category: 'town',    levelRange: '',       biome: 'tropical' },
    cmd_fild01:     { displayName: 'Comodo Beach 01',       category: 'field',   levelRange: '25-35',  biome: 'tropical' },
    cmd_fild02:     { displayName: 'Comodo Beach 02',       category: 'field',   levelRange: '28-38',  biome: 'tropical' },

    // ══════════════════════════════════════════════════════
    // MT. MJOLNIR — Mountains (north)
    // ══════════════════════════════════════════════════════
    mjolnir_01:     { displayName: 'Mt. Mjolnir 01',       category: 'field',   levelRange: '25-35',  biome: 'mountain' },
    mjolnir_02:     { displayName: 'Mt. Mjolnir 02',       category: 'field',   levelRange: '28-38',  biome: 'mountain' },
    mjolnir_03:     { displayName: 'Mt. Mjolnir 03',       category: 'field',   levelRange: '30-40',  biome: 'mountain' },
    mjolnir_04:     { displayName: 'Mt. Mjolnir 04',       category: 'field',   levelRange: '32-42',  biome: 'mountain' },
    mjolnir_05:     { displayName: 'Mt. Mjolnir 05',       category: 'field',   levelRange: '35-45',  biome: 'mountain' },
    mjolnir_06:     { displayName: 'Mt. Mjolnir Peak',     category: 'field',   levelRange: '38-50',  biome: 'mountain' },
    aldebaran:      { displayName: 'Al De Baran',           category: 'town',    levelRange: '',       biome: 'mountain' },
    coal_mine:      { displayName: 'Coal Mine',             category: 'dungeon', levelRange: '30-45',  biome: 'mountain' },

    // ══════════════════════════════════════════════════════
    // SCHWARZWALD — Industrial / Steampunk (northeast)
    // ══════════════════════════════════════════════════════
    yuno:           { displayName: 'Juno',                  category: 'town',    levelRange: '',       biome: 'steampunk_highlands' },
    yuno_fild01:    { displayName: 'Juno Field 01',         category: 'field',   levelRange: '40-50',  biome: 'steampunk_highlands' },
    yuno_fild02:    { displayName: 'Juno Field 02',         category: 'field',   levelRange: '45-55',  biome: 'steampunk_highlands' },
    yuno_fild03:    { displayName: 'Juno Field 03',         category: 'field',   levelRange: '48-58',  biome: 'steampunk_highlands' },
    einbroch:       { displayName: 'Einbroch',              category: 'town',    levelRange: '',       biome: 'industrial' },
    ein_fild01:     { displayName: 'Einbroch Field 01',     category: 'field',   levelRange: '45-55',  biome: 'industrial' },
    ein_fild02:     { displayName: 'Einbroch Mine',         category: 'field',   levelRange: '50-60',  biome: 'industrial' },
    lighthalzen:    { displayName: 'Lighthalzen',           category: 'town',    levelRange: '',       biome: 'industrial' },
    lhz_fild01:     { displayName: 'Lighthalzen Field',     category: 'field',   levelRange: '55-70',  biome: 'industrial' },

    // ══════════════════════════════════════════════════════
    // ARUNAFELTZ — Frozen tundra (northwest)
    // ══════════════════════════════════════════════════════
    rachel:         { displayName: 'Rachel',                category: 'town',    levelRange: '',       biome: 'frozen_tundra' },
    ra_fild01:      { displayName: 'Ice Field 01',          category: 'field',   levelRange: '60-70',  biome: 'frozen_tundra' },
    ra_fild02:      { displayName: 'Ice Field 02',          category: 'field',   levelRange: '65-75',  biome: 'frozen_tundra' },
    ra_fild03:      { displayName: 'Frozen Wastes',         category: 'field',   levelRange: '68-78',  biome: 'frozen_tundra' },
    veins:          { displayName: 'Veins',                 category: 'town',    levelRange: '',       biome: 'frozen_canyon' },
    ice_dun01:      { displayName: 'Ice Dungeon 1F',        category: 'dungeon', levelRange: '65-80',  biome: 'frozen_tundra' },
    ice_dun02:      { displayName: 'Ice Dungeon 2F',        category: 'dungeon', levelRange: '72-85',  biome: 'frozen_tundra' },

    // ══════════════════════════════════════════════════════
    // VOLCANIC — Far North
    // ══════════════════════════════════════════════════════
    thor_v01:       { displayName: 'Thor Volcano 1F',       category: 'dungeon', levelRange: '75-90',  biome: 'volcanic' },
    thor_v02:       { displayName: 'Thor Volcano 2F',       category: 'dungeon', levelRange: '85-99',  biome: 'volcanic' },
    magma_dun01:    { displayName: 'Magma Dungeon 1F',      category: 'dungeon', levelRange: '70-85',  biome: 'volcanic' },

    // ══════════════════════════════════════════════════════
    // LUTIE — Snow village (north)
    // ══════════════════════════════════════════════════════
    lutie:          { displayName: 'Lutie',                 category: 'town',    levelRange: '',       biome: 'snow' },
    xmas_fild01:    { displayName: 'Lutie Field',           category: 'field',   levelRange: '30-45',  biome: 'snow' },

    // ══════════════════════════════════════════════════════
    // ISLANDS — Ocean
    // ══════════════════════════════════════════════════════
    amatsu:         { displayName: 'Amatsu',                category: 'town',    levelRange: '',       biome: 'japanese_island' },
    louyang:        { displayName: 'Louyang',               category: 'town',    levelRange: '',       biome: 'chinese_island' },
    ayothaya:       { displayName: 'Ayothaya',              category: 'town',    levelRange: '',       biome: 'thai_island' },
    niflheim:       { displayName: 'Niflheim',              category: 'town',    levelRange: '',       biome: 'realm_of_dead' },

    // ══════════════════════════════════════════════════════
    // SPECIAL
    // ══════════════════════════════════════════════════════
    _ocean:         { displayName: 'Ocean',                 category: 'ocean',   levelRange: '',       biome: 'ocean' }
};

// Grid layout: 12 columns x 8 rows
// Each cell = zone name string, or null for unassigned ocean
// Row 0 = top of map (north), Row 7 = bottom (south)
// Col 0 = left (west), Col 11 = right (east)
//
// Visual reference against the generated map image:
//   Row 0: volcanic / snow mountains / snow / industrial NE
//   Row 1: frozen tundra / glast heim / mountains / juno / einbroch / islands
//   Row 2: rachel / geffen / prontera north / yuno field / lighthalzen
//   Row 3: ice field / geffen fields / prontera / prt east / izlude / payon
//   Row 4: orc territory / prt south + dungeon / payon forest
//   Row 5: morroc desert / sograt / alberta coast
//   Row 6: comodo / desert / port / ayothaya island
//   Row 7: niflheim / ocean / ocean

// Each cell is a UNIQUE zone ID. No duplicates. Every field has its own number.
// null = ocean (no zone, not hoverable)
const WORLD_MAP_GRID = [
    // Row 0 — Far North: volcanic, snow peaks, steampunk
    //  Col:  0            1            2            3            4            5            6            7            8            9            10           11
    [ 'thor_v01',    'thor_v02',    'magma_dun01', 'mjolnir_05', 'mjolnir_06', 'lutie',      'xmas_fild01', 'ein_fild02',  'einbroch',    null,          'amatsu',      null          ],
    // Row 1 — North: frozen, glast heim, mountains, schwarzwald
    [ 'veins',       'ra_fild03',   'gl_fild01',   'mjolnir_03', 'mjolnir_04', 'aldebaran',  'yuno',        'ein_fild01',  'lhz_fild01',  null,          null,          null          ],
    // Row 2 — Upper-mid: frozen fields, geffen, prontera north, juno fields
    [ 'rachel',      'ra_fild02',   'glast_heim',  'gef_fild02', 'prontera_north', 'coal_mine', 'yuno_fild01', 'yuno_fild02', 'lighthalzen', null,        null,          null          ],
    // Row 3 — Center: geffen, prontera, izlude, payon
    [ 'ice_dun01',   'ra_fild01',   'geffen',      'gef_fild01', 'prontera',   'prt_fild01', 'prt_fild02',  'izlude',      'iz_fild01',   'payon',       'louyang',     null          ],
    // Row 4 — Lower-mid: orc territory, prontera south, payon forest
    [ 'ice_dun02',   'gef_fild03',  'gef_fild04',  'morroc',     'prontera_south', 'prt_dungeon_01', 'alb_fild01', 'pay_fild01', 'pay_fild02', 'pay_fild03', null,       null          ],
    // Row 5 — South: desert, pyramids, alberta, payon
    [ null,          'cmd_fild01',  'moc_fild01',  'moc_fild02', 'moc_fild03', 'moc_pryd',   'alberta',     'sphinx',      null,          null,          null,          'ayothaya'    ],
    // Row 6 — Far South: comodo, deep desert, coast
    [ null,          'comodo',      'cmd_fild02',  'moc_fild04', 'moc_fild05', null,          null,          null,          null,          null,          null,          null          ],
    // Row 7 — Southernmost: niflheim island, ocean
    [ 'niflheim',    null,          null,           null,         null,         null,          null,          null,          null,          null,          null,          null          ]
];

// ════════════════════════════════════════════════════════════
// Build full world data payload for clients
// Sends: grid definition + zone info + monster data + warps
// ════════════════════════════════════════════════════════════
function buildWorldMapData() {
    // Build zone info with monster summaries from ZONE_REGISTRY spawns
    const zones = {};

    // First, populate from ZONE_INFO (world map zones — may not all be in ZONE_REGISTRY yet)
    for (const [zoneName, info] of Object.entries(ZONE_INFO)) {
        if (zoneName.startsWith('_')) continue; // skip _ocean
        zones[zoneName] = {
            name: zoneName,
            displayName: info.displayName,
            category: info.category,
            levelRange: info.levelRange,
            biome: info.biome,
            monsters: [],
            warps: [],
            // These get overridden if the zone exists in ZONE_REGISTRY
            type: info.category === 'town' ? 'town' : (info.category === 'dungeon' ? 'dungeon' : 'field'),
            levelName: '',
            flags: {}
        };
    }

    // Overlay data from ZONE_REGISTRY (implemented zones have actual spawns, warps, flags)
    for (const [zoneName, zone] of Object.entries(ZONE_REGISTRY)) {
        if (!zones[zoneName]) {
            zones[zoneName] = {
                name: zoneName,
                displayName: zone.displayName,
                category: zone.type,
                levelRange: '',
                biome: '',
                monsters: [],
                warps: [],
                type: zone.type,
                levelName: zone.levelName,
                flags: zone.flags
            };
        } else {
            zones[zoneName].type = zone.type;
            zones[zoneName].levelName = zone.levelName;
            zones[zoneName].flags = zone.flags;
            zones[zoneName].displayName = zone.displayName; // prefer ZONE_REGISTRY name
        }

        // Build monster summary
        const monsterSummary = [];
        const seenTemplates = new Set();
        for (const spawn of (zone.enemySpawns || [])) {
            if (seenTemplates.has(spawn.template)) continue;
            seenTemplates.add(spawn.template);
            const tmpl = ENEMY_TEMPLATES ? ENEMY_TEMPLATES[spawn.template] : null;
            monsterSummary.push({
                name: tmpl ? (tmpl.displayName || tmpl.name || spawn.template) : spawn.template,
                level: tmpl ? (tmpl.level || 1) : 1,
                element: tmpl ? (tmpl.element || 'neutral') : 'neutral'
            });
        }
        monsterSummary.sort((a, b) => a.level - b.level);
        zones[zoneName].monsters = monsterSummary;

        // Warp positions (for minimap)
        zones[zoneName].warps = (zone.warps || []).map(w => ({
            id: w.id, x: w.x, y: w.y, z: w.z,
            destZone: w.destZone,
            destDisplayName: ZONE_REGISTRY[w.destZone]
                ? ZONE_REGISTRY[w.destZone].displayName : w.destZone
        }));
    }

    return {
        grid: WORLD_MAP_GRID,
        gridCols: GRID_COLS,
        gridRows: GRID_ROWS,
        zones: zones
    };
}

// Cache the built data (rebuild if zones change at runtime — unlikely)
let cachedWorldData = null;

function getWorldMapData() {
    if (!cachedWorldData) {
        cachedWorldData = buildWorldMapData();
    }
    return cachedWorldData;
}

// Invalidate cache (call if ZONE_REGISTRY is modified at runtime)
function invalidateWorldMapCache() {
    cachedWorldData = null;
}

module.exports = {
    WORLD_MAP_GRID,
    GRID_COLS,
    GRID_ROWS,
    ZONE_INFO,
    getWorldMapData,
    invalidateWorldMapCache
};
