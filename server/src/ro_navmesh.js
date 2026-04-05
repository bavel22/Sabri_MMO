// ============================================================
// NavMesh Pathfinding Module — recast-navigation integration
// Loads OBJ navmesh files exported from UE5, builds Detour
// navmeshes, provides path queries for enemy AI movement.
//
// Coordinate systems:
//   Game (UE5):  X = East/West,  Y = North/South,  Z = Up
//   Recast:      X = East/West,  Y = Up,            Z = North/South
//   Conversion:  Game → Recast = swap Y ↔ Z
//
// Usage:
//   await initNavMeshes(ZONE_REGISTRY, logger);
//   const path = findNavMeshPath(zone, x1, y1, z1, x2, y2, z2);
// ============================================================

const fs = require('fs');
const path = require('path');

let navMeshes = {};   // zoneName → { navMesh, query }
let initialized = false;

// ─── OBJ Parser ──────────────────────────────────────────────
// Parses Wavefront OBJ into flat vertex/index arrays.
// Expects vertices already in Recast coords (Y-up) from UE5 exporter.
function parseOBJ(objText) {
    const vertices = [];
    const indices = [];
    for (const line of objText.split('\n')) {
        const parts = line.trim().split(/\s+/);
        if (parts[0] === 'v') {
            vertices.push(parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3]));
        } else if (parts[0] === 'f') {
            // OBJ faces are 1-indexed, may have v/vt/vn format
            const faceVerts = [];
            for (let i = 1; i < parts.length; i++) {
                faceVerts.push(parseInt(parts[i].split('/')[0]) - 1);
            }
            // Fan-triangulate faces with > 3 vertices
            for (let i = 1; i < faceVerts.length - 1; i++) {
                indices.push(faceVerts[0], faceVerts[i], faceVerts[i + 1]);
            }
        }
    }
    return { vertices: new Float32Array(vertices), indices: new Uint32Array(indices) };
}

// ─── Initialize NavMeshes ────────────────────────────────────
// Load OBJ files from server/navmesh/, build Detour navmeshes.
// Must be called once during server startup.
async function initNavMeshes(zoneRegistry, logger) {
    const log = logger || console;
    const navDir = path.join(__dirname, '..', 'navmesh');

    if (!fs.existsSync(navDir)) {
        log.info('[NAVMESH] No navmesh/ directory found — all zones use straight-line movement');
        initialized = true;
        return;
    }

    // Check for any OBJ files before loading the WASM module
    const objFiles = fs.readdirSync(navDir).filter(f => f.endsWith('.obj'));
    if (objFiles.length === 0) {
        log.info('[NAVMESH] No .obj files in navmesh/ — all zones use straight-line movement');
        initialized = true;
        return;
    }

    try {
        // Dynamic import for ESM package (server uses CommonJS)
        const recast = await import('recast-navigation');
        const generators = await import('recast-navigation/generators');

        await recast.init();

        const cacheDir = path.join(navDir, '.cache');
        if (!fs.existsSync(cacheDir)) {
            fs.mkdirSync(cacheDir, { recursive: true });
        }

        for (const [zoneName, zone] of Object.entries(zoneRegistry)) {
            const objPath = path.join(navDir, `${zoneName}.obj`);
            if (!fs.existsSync(objPath)) continue;

            try {
                const cachePath = path.join(cacheDir, `${zoneName}.navmesh`);
                const objStat = fs.statSync(objPath);
                let navMesh = null;

                // Try loading from binary cache (faster than rebuilding)
                if (fs.existsSync(cachePath)) {
                    const cacheStat = fs.statSync(cachePath);
                    if (cacheStat.mtimeMs > objStat.mtimeMs) {
                        try {
                            const cacheData = fs.readFileSync(cachePath);
                            const imported = recast.importNavMesh(new Uint8Array(cacheData));
                            navMesh = imported.navMesh;
                            log.info(`[NAVMESH] Loaded ${zoneName} from cache`);
                        } catch (e) {
                            navMesh = null; // Cache corrupt — rebuild
                        }
                    }
                }

                // Build from OBJ if no valid cache
                if (!navMesh) {
                    const objText = fs.readFileSync(objPath, 'utf-8');
                    const { vertices, indices } = parseOBJ(objText);

                    if (vertices.length < 9 || indices.length < 3) {
                        log.warn(`[NAVMESH] ${zoneName}.obj has insufficient geometry (${vertices.length / 3} verts) — skipping`);
                        continue;
                    }

                    // Build config tuned for UE5 scale (1 RO cell = 50 UE units)
                    // NOTE: walkableHeight/Climb/Radius are in VOXELS, not world units.
                    // maxEdgeLen is also in voxels. The library does NOT auto-convert.
                    const cs = 25;  // Cell size: 25 UE units (half a RO cell)
                    const ch = 10;  // Cell height: 10 UE units
                    const result = generators.generateSoloNavMesh(vertices, indices, {
                        cs,
                        ch,
                        walkableSlopeAngle: 45,
                        walkableHeight: Math.ceil(100 / ch),   // 10 voxels = 100 UE units (~2 RO cells)
                        walkableClimb: Math.floor(50 / ch),    // 5 voxels = 50 UE units (1 RO cell step)
                        walkableRadius: Math.ceil(30 / cs),    // 2 voxels = 50 UE units
                        maxEdgeLen: Math.ceil(600 / cs),       // 24 voxels = 600 UE units
                        maxSimplificationError: 1.3,
                        minRegionArea: 8,
                        mergeRegionArea: 20,
                        maxVertsPerPoly: 6,
                        detailSampleDist: 3,        // multiplied by cs internally → 75 UE units
                        detailSampleMaxError: 1,    // multiplied by ch internally → 10 UE units
                    });

                    if (!result.success) {
                        log.warn(`[NAVMESH] Failed to build ${zoneName}: ${result.error || 'unknown error'}`);
                        continue;
                    }

                    navMesh = result.navMesh;

                    // Cache the built navmesh for faster future loads
                    try {
                        const navData = recast.exportNavMesh(navMesh);
                        fs.writeFileSync(cachePath, Buffer.from(navData));
                    } catch (e) {
                        log.warn(`[NAVMESH] Failed to cache ${zoneName}: ${e.message}`);
                    }

                    log.info(`[NAVMESH] Built ${zoneName} (${vertices.length / 3} verts, ${indices.length / 3} tris)`);
                }

                const query = new recast.NavMeshQuery(navMesh);
                navMeshes[zoneName] = { navMesh, query };
            } catch (zoneErr) {
                log.warn(`[NAVMESH] Error loading ${zoneName}: ${zoneErr.message}`);
            }
        }

        const loadedZones = Object.keys(navMeshes);
        log.info(`[NAVMESH] Initialized ${loadedZones.length} zone(s): ${loadedZones.join(', ') || 'none'}`);
        initialized = true;
    } catch (err) {
        log.error(`[NAVMESH] Failed to initialize recast-navigation: ${err.message}`);
        log.error('[NAVMESH] Falling back to straight-line movement for all zones');
        initialized = true;
    }
}

// ─── Path Query ──────────────────────────────────────────────
// Find path between two game-world points.
// Returns array of {x, y, z} waypoints in game coords, or null on failure.
function findNavMeshPath(zone, fromX, fromY, fromZ, toX, toY, toZ) {
    const nav = navMeshes[zone];
    if (!nav) return null;

    try {
        // Game coords (Z-up) → Recast coords (Y-up): swap Y and Z
        const start = { x: fromX, y: fromZ, z: fromY };
        const end = { x: toX, y: toZ, z: toY };

        // Large Y extent because game spawn Z (~300) differs from navmesh floor Z (~7-400).
        // The Y axis in Recast is up, so this is the vertical search range.
        const halfExtents = { x: 200, y: 500, z: 200 };

        const result = nav.query.computePath(start, end, {
            halfExtents,
            maxPathPolys: 256,
        });

        if (!result.success || !result.path || result.path.length === 0) return null;

        // Recast coords (Y-up) → Game coords (Z-up): swap Y and Z back
        return result.path.map(p => ({ x: p.x, y: p.z, z: p.y }));
    } catch (e) {
        return null;
    }
}

// ─── Closest Point Query ─────────────────────────────────────
// Find the closest point on the NavMesh to a game-world position.
// Returns {x, y, z} in game coords, or null on failure.
function findClosestNavMeshPoint(zone, x, y, z) {
    const nav = navMeshes[zone];
    if (!nav) return null;

    try {
        const result = nav.query.findClosestPoint(
            { x: x, y: z, z: y },  // Game → Recast (swap Y↔Z)
            { halfExtents: { x: 200, y: 500, z: 200 } }
        );

        if (!result.success) return null;

        // Recast → Game (swap Y↔Z back)
        return { x: result.point.x, y: result.point.z, z: result.point.y };
    } catch (e) {
        return null;
    }
}

// ─── Utility ─────────────────────────────────────────────────
function hasNavMesh(zone) {
    return !!navMeshes[zone];
}

function destroyNavMeshes() {
    for (const nav of Object.values(navMeshes)) {
        try {
            if (nav.query) nav.query.destroy();
            if (nav.navMesh) nav.navMesh.destroy();
        } catch (e) { /* ignore cleanup errors */ }
    }
    navMeshes = {};
    initialized = false;
}

module.exports = {
    initNavMeshes,
    findNavMeshPath,
    findClosestNavMeshPoint,
    hasNavMesh,
    destroyNavMeshes,
};
