// ============================================================
// Spawn Regions Module — random allow/deny zone spawn generator
//
// Reads JSON files in server/spawn_regions/<zone>.json (exported
// from UE5 by SpawnRegionExporter) and uses them to generate
// random spawn points for zones that define a `spawnPool` in
// ro_zone_data.js.
//
// JSON format (produced by the C++ exporter):
//   {
//     "version": 1,
//     "zone": "prontera_south",
//     "allow": [
//       { "min": [x,y,z], "max": [x,y,z], "tag": "...", "filter": ["template1", ...] }
//     ],
//     "deny":  [
//       { "min": [x,y,z], "max": [x,y,z], "tag": "...", "filter": [] }
//     ]
//   }
//
// Pipeline per generated spawn:
//   1. Pick a random allow box, weighted by XY area (so a big
//      forest gets more spawns than a small clearing).
//      Filter: skip allow boxes whose `filter` excludes the
//      requested template (empty filter = all templates ok).
//   2. Roll a random point inside the box (XY uniform, Z = box
//      center — the navmesh snap finds the actual floor).
//   3. Reject if the point lies inside any deny box.
//   4. Snap to the nearest walkable NavMesh point. This is what
//      keeps enemies on the ground instead of in mid-air over
//      water, cliffs, or rooftops. If the zone has no NavMesh
//      loaded, fall back to the raw point.
//   5. If snap fails or all retries are denied, return null
//      (caller skips this spawn — logged as a warning).
// ============================================================

const fs = require('fs');
const path = require('path');
const { findClosestNavMeshPoint, hasNavMesh } = require('./ro_navmesh');

// In-memory cache: zone name -> { allow: [...], deny: [...], totalArea }
let regions = {};

// ─── Init ────────────────────────────────────────────────────
async function initSpawnRegions(zoneRegistry, logger) {
    const log = logger || console;
    const dir = path.join(__dirname, '..', 'spawn_regions');

    if (!fs.existsSync(dir)) {
        log.info('[SPAWN_REGIONS] No spawn_regions/ directory — zones using `spawnPool` will fall back to legacy enemySpawns only');
        return;
    }

    const files = fs.readdirSync(dir).filter(f => f.endsWith('.json'));
    if (files.length === 0) {
        log.info('[SPAWN_REGIONS] No .json files in spawn_regions/');
        return;
    }

    for (const file of files) {
        try {
            const raw = fs.readFileSync(path.join(dir, file), 'utf-8');
            const data = JSON.parse(raw);
            const zone = data.zone || file.replace(/\.json$/, '');

            // Pre-compute XY area for weighted random selection.
            const allow = (data.allow || []).map(box => {
                const dx = Math.abs(box.max[0] - box.min[0]);
                const dy = Math.abs(box.max[1] - box.min[1]);
                return {
                    min: box.min,
                    max: box.max,
                    tag: box.tag || '',
                    filter: Array.isArray(box.filter) ? box.filter : [],
                    area: Math.max(1, dx * dy),
                };
            });
            const deny = (data.deny || []).map(box => ({
                min: box.min,
                max: box.max,
                tag: box.tag || '',
            }));

            regions[zone] = { allow, deny };
            log.info(`[SPAWN_REGIONS] Loaded '${zone}': ${allow.length} allow, ${deny.length} deny`);
        } catch (err) {
            log.warn(`[SPAWN_REGIONS] Failed to load ${file}: ${err.message}`);
        }
    }

    const loaded = Object.keys(regions);
    log.info(`[SPAWN_REGIONS] Initialized ${loaded.length} zone(s): ${loaded.join(', ') || 'none'}`);
}

// ─── Helpers ─────────────────────────────────────────────────
function pointInBox(x, y, z, box) {
    return (
        x >= box.min[0] && x <= box.max[0] &&
        y >= box.min[1] && y <= box.max[1] &&
        z >= box.min[2] && z <= box.max[2]
    );
}

function pickWeightedAllowBox(allowBoxes, template) {
    // Filter to boxes that allow this template (empty filter = all).
    const eligible = allowBoxes.filter(b =>
        !b.filter || b.filter.length === 0 || b.filter.includes(template)
    );
    if (eligible.length === 0) return null;

    const total = eligible.reduce((s, b) => s + b.area, 0);
    if (total <= 0) return eligible[0];

    let roll = Math.random() * total;
    for (const b of eligible) {
        roll -= b.area;
        if (roll <= 0) return b;
    }
    return eligible[eligible.length - 1];
}

// ─── Public API ──────────────────────────────────────────────

/**
 * Pick a random valid spawn point in `zone` for the given monster `template`.
 *
 * @param {string} zone        Zone name (e.g. 'prontera_south')
 * @param {string} template    Monster template key (used for per-region filtering)
 * @param {object} [options]
 * @param {number} [options.maxRetries=20]   Attempts before giving up
 * @returns {{x:number,y:number,z:number}|null}
 */
function pickRandomSpawnPoint(zone, template, options = {}) {
    const reg = regions[zone];
    if (!reg || reg.allow.length === 0) return null;

    const maxRetries = options.maxRetries || 20;
    const navMeshAvailable = hasNavMesh(zone);

    for (let attempt = 0; attempt < maxRetries; attempt++) {
        const box = pickWeightedAllowBox(reg.allow, template);
        if (!box) return null;

        const x = box.min[0] + Math.random() * (box.max[0] - box.min[0]);
        const y = box.min[1] + Math.random() * (box.max[1] - box.min[1]);
        // Box center Z. The NavMesh snap below handles the actual ground placement —
        // a 1000-unit-tall box with a floor anywhere inside still resolves correctly
        // because findClosestNavMeshPoint uses halfExtents.y=500 to bridge the gap.
        const z = (box.min[2] + box.max[2]) / 2;

        // Deny exclusion check (pre-snap so we don't waste a navmesh query on rejects).
        let denied = false;
        for (const d of reg.deny) {
            if (pointInBox(x, y, z, d)) { denied = true; break; }
        }
        if (denied) continue;

        if (navMeshAvailable) {
            const snapped = findClosestNavMeshPoint(zone, x, y, z);
            if (!snapped) continue;  // no walkable surface near this random point

            // Re-check deny boxes against the snapped Z. The snap may pull the point
            // up or down by hundreds of units; if the snapped position now lands
            // inside a deny box, reject it.
            let snappedDenied = false;
            for (const d of reg.deny) {
                if (pointInBox(snapped.x, snapped.y, snapped.z, d)) { snappedDenied = true; break; }
            }
            if (snappedDenied) continue;

            return snapped;
        }

        // No NavMesh for this zone — caller gets the raw point. Z will be box-center.
        return { x, y, z };
    }

    return null;
}

/**
 * Build an array of spawn configs from a zone's `spawnPool`.
 * Used by the lazy-spawn path in index.js — calls pickRandomSpawnPoint per
 * count and returns the configs ready to feed into spawnEnemy().
 *
 * @param {string} zone
 * @param {Array<{template:string,count:number,wanderRadius?:number}>} spawnPool
 * @param {number} [defaultWanderRadius=300]
 * @returns {{spawns:Array, requested:number, generated:number}}
 */
function generateSpawnsFromPool(zone, spawnPool, defaultWanderRadius = 300) {
    if (!spawnPool || spawnPool.length === 0) {
        return { spawns: [], requested: 0, generated: 0 };
    }
    if (!regions[zone]) {
        // Caller should have checked hasSpawnRegions(); we still return empty rather than throw.
        return { spawns: [], requested: 0, generated: 0 };
    }

    const spawns = [];
    let requested = 0;
    for (const entry of spawnPool) {
        const count = Math.max(0, entry.count || 0);
        const wanderRadius = entry.wanderRadius || defaultWanderRadius;
        requested += count;
        for (let i = 0; i < count; i++) {
            const point = pickRandomSpawnPoint(zone, entry.template);
            if (!point) continue;
            spawns.push({
                template: entry.template,
                x: point.x,
                y: point.y,
                z: point.z,
                wanderRadius,
                zone,
            });
        }
    }
    return { spawns, requested, generated: spawns.length };
}

function hasSpawnRegions(zone) {
    return !!regions[zone];
}

module.exports = {
    initSpawnRegions,
    pickRandomSpawnPoint,
    generateSpawnsFromPool,
    hasSpawnRegions,
};
