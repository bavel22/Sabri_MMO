# NavMesh Pathfinding Implementation Plan

> **Status**: ALL PHASES COMPLETE (2026-03-27)

## Overview

Server-side NavMesh pathfinding for enemy AI using the `recast-navigation` npm package (v0.42.1, same Recast/Detour library UE5 uses internally). The server is the pathfinding authority; the client interpolates through server-sent waypoints unchanged.

## Architecture

```
UE5 Editor                    Server (Node.js)                 Client (C++)
-----------                   ----------------                 ------------
NavMesh built in editor  -->  Export as OBJ per zone
                              Load with recast-navigation
                              Build Detour navmesh on startup
                              Binary cache for fast restarts
                                    |
                              enemyMoveToward() queries path
                              Stores waypoint[] per enemy
                              Sends position via enemy:move
                                    |                     -->  HandleEnemyMove()
                                                               Store target on sprite
                                                               Interpolate toward position
                                                               Ground snap via line trace
```

## Key Files

| File | Role |
|------|------|
| `server/src/ro_navmesh.js` | NavMesh module — OBJ parsing, WASM init, path/closest-point queries, binary cache |
| `server/navmesh/*.obj` | Exported NavMesh geometry per zone (Recast Y-up coords) |
| `server/navmesh/.cache/*.navmesh` | Binary cache of built navmeshes (auto-generated) |
| `client/SabriMMO/Source/SabriMMO/NavMeshExporter.h/.cpp` | UE5 editor utility — exports NavMesh geometry as OBJ |
| `server/src/index.js` | Enemy AI uses `findNavMeshPath()` in `enemyMoveToward()`, `findClosestNavMeshPoint()` in `pickRandomWanderPoint()` |

## Coordinate System (CRITICAL)

```
UE5 / Game World:          Recast / Detour:
  X = East/West              X = East/West (same)
  Y = North/South            Y = UP (height)
  Z = Up/Down                Z = North/South

Game -> Recast:  { x: game.x, y: game.z, z: game.y }
Recast -> Game:  { x: recast.x, y: recast.z, z: recast.y }
```

## Implementation Phases (ALL COMPLETE)

### Phase 1: Export NavMesh from UE5 -- COMPLETE

**NavMeshExporter** (`NavMeshExporter.h/.cpp`):
- C++ UE5 console command registered via `IConsoleManager`
- `ExportNavMesh <zone_name>` — exports named zone
- `ExportCurrentLevelNavMesh` — auto-detects zone name from level name
- Uses `UNavigationSystemV1::GetCurrent()` + `ARecastNavMesh` to get NavMesh data
- Iterates tiles via `GetDebugGeometryForTile()` (UE5.7 per-tile API, NOT `GetDebugGeometry()`)
- Writes OBJ with Y<->Z swap for Recast coordinate system
- Triangle winding is correct after vertex swap — do NOT reverse indices

**Zone name mapping**:
- L_PrtSouth -> prontera_south
- L_PrtNorth -> prontera_north
- L_PrtDungeon01 -> prt_dungeon_01
- L_Prontera -> prontera

**Known issue**: Export path lands in `client/server/navmesh/` instead of `server/navmesh/` — files must be manually copied.

### Phase 2: Server — Install and Load NavMesh -- COMPLETE

**Package**: `recast-navigation` v0.42.1 (umbrella package, NOT separate core/wasm)

**Module**: `server/src/ro_navmesh.js` handles:
- WASM initialization via dynamic `import()` (ESM package in CommonJS server)
- OBJ parsing (vertices + indices extraction)
- NavMesh building via `generateSoloNavMesh()` from `recast-navigation/generators`
- Binary cache system (`exportNavMesh` / `importNavMesh`) in `server/navmesh/.cache/`
- NavMeshQuery creation for path/closest-point lookups

**Build config** (tuned for UE5 scale):
```javascript
cs: 25,                              // Cell size: 25 UE units (half RO cell)
ch: 10,                              // Cell height: 10 UE units
walkableHeight: Math.ceil(100/ch),   // 10 voxels = 100 UE units
walkableClimb: Math.floor(50/ch),    // 5 voxels = 50 UE units
walkableRadius: Math.ceil(30/cs),    // 2 voxels = 50 UE units
maxEdgeLen: Math.ceil(600/cs),       // 24 voxels
halfExtents: { x:200, y:500, z:200 } // Query search — large Y for height mismatch
```

**CRITICAL**: `walkableHeight`, `walkableClimb`, `walkableRadius` are in VOXELS, not world units. Divide world units by `ch` or `cs` as appropriate.

### Phase 3: Server — Pathfinding Query Functions -- COMPLETE

**`findNavMeshPath(zone, fromX, fromY, fromZ, toX, toY, toZ)`**:
- Converts game coords to Recast coords (Y<->Z swap)
- Uses `navMeshQuery.computePath(start, end)` — NOT `findPath()` (which returns polygon corridors)
- Converts result back to game coords
- Returns array of `{x, y, z}` waypoints or null

**`findClosestNavMeshPoint(zone, x, y, z)`**:
- Snaps a point to the nearest walkable surface
- Uses `navMeshQuery.findClosestPoint(pos)` with halfExtents `{x:200, y:500, z:200}`
- Large Y halfExtent (500) bridges the gap between game Z (~300) and navmesh floor Z (~7-400)

### Phase 4: Server — Modify Enemy Movement -- COMPLETE

**`enemyMoveToward()`** (~line 30115):
- Queries `findNavMeshPath()` when target moves >50 units from last path target
- Follows waypoints sequentially (advance when within 20 units of current waypoint)
- Falls back to straight-line if no navmesh or path query fails
- Existing Ice Wall / ensemble barrier / quagmire checks preserved
- `enemy:move` event format unchanged — client unaffected

**Enemy path storage** (added to `spawnEnemy()`):
- `_navPath` — Array of {x, y, z} waypoints
- `_navPathIdx` — Current waypoint index
- `_navPathTarget` — Final target position for path invalidation check

**Path reset points**:
- `initEnemyWanderState()` — on respawn
- `knockbackTarget()` — after forced movement
- AL_TELEPORT handler — after monster teleport

### Phase 5: Server — Modify Wander System -- COMPLETE

**`pickRandomWanderPoint()`** (~line 28795):
- Random wander target snapped to navmesh via `findClosestNavMeshPoint()`
- Falls back to raw random point if no navmesh for zone

### Phase 6: Client — No Changes Required -- COMPLETE (N/A)

Client already:
- Receives `enemy:move` events with `{ x, y, z, isMoving }`
- Interpolates sprite toward position (`SetServerTargetPosition`)
- Ground-snaps via line trace
- Sets walk/idle animation based on `isMoving`

No client code changes needed.

### De-aggro System -- COMPLETE

- `ENEMY_AI.DEAGGRO_DISTANCE = 1200` (24 RO cells)
- Checked in both CHASE and ATTACK states
- Based on distance from enemy to target (not aggro origin)
- Bosses/MVPs exempt (`enemy.modeFlags.mvp`)
- Enemy returns to IDLE when target exceeds de-aggro range

## Systems NOT Affected

| System | Why unaffected |
|--------|---------------|
| Aggro detection | Uses distance check, not pathfinding |
| Chase range / leash | Uses distance from spawn, not path length |
| Attack range | Uses distance to target, not pathfinding |
| Combat damage | Independent of movement |
| Skill casting | Independent of movement |
| Death / respawn | Independent of movement |
| Sprite animations | Driven by `isMoving` flag — unchanged |
| Click targeting | Uses visibility trace on sprite mesh — unchanged |
| Name tags | Follows sprite actor — unchanged |
| Party / chat | Independent of movement |
| Items / inventory | Independent of movement |
| Ground effects | Position-based checks — still work |

## Systems That ARE Affected

| System | Change |
|--------|--------|
| `enemyMoveToward()` | NavMesh path following with waypoints |
| `pickRandomWanderPoint()` | Wander targets snapped to navmesh |
| `spawnEnemy()` | Added `_navPath`, `_navPathIdx`, `_navPathTarget` fields |
| Server startup | `initNavMeshes()` loads OBJ files, builds Detour meshes, caches binaries |
| Ice Wall collision | Still checked per-step (preserved in modified movement) |
| Ensemble barriers | Still checked per-step (preserved in modified movement) |
| Quagmire debuff | Still checked per-step (preserved in modified movement) |
| Monster Teleport skill | Resets `_navPath` after teleport |
| Knockback | Resets `_navPath` after knockback |
| Enemy respawn | Resets `_navPath` via `initEnemyWanderState()` |

## Performance

- NavMesh path query: ~0.05ms per query
- Path recalculation: only when target moves >50 units
- 40 enemies x 5 recalculations/sec = 200 queries/sec = 10ms/sec total
- NavMesh memory: ~200KB per zone
- Binary cache eliminates rebuild time on restart (~instant load vs ~2s build)
- No client performance impact (same number of position updates)

## Fallback Behavior

- Zone without OBJ file -> `findNavMeshPath()` returns null -> straight-line movement
- Path query fails -> straight-line fallback (single waypoint to target)
- `recast-navigation` not installed -> init logs error, all zones use straight-line
- No `navmesh/` directory -> graceful skip at startup

## Lessons Learned

### 1. Triangle Winding Order
UE5 exports triangles in left-handed Z-up winding. After the Y<->Z vertex swap to convert to Recast's right-handed Y-up system, the winding naturally becomes correct CCW. Do NOT reverse triangle indices — the vertex swap handles it. If normals point DOWN in Recast space, the winding is wrong.

### 2. Voxel Units vs World Units
`walkableHeight`, `walkableClimb`, and `walkableRadius` in the Recast build config are measured in VOXELS, not world units. Must divide world units by `ch` (cell height) or `cs` (cell size) respectively: `walkableHeight = Math.ceil(100/ch)`, not `walkableHeight = 100`.

### 3. HalfExtents Height Mismatch
Game enemy spawn Z is ~300, but the navmesh floor in Recast Y-up space is at Z ~7-400. The query `halfExtents.y` (which maps to the UP axis in Recast) must be 500+ to bridge this gap. Without this, `computePath` and `findClosestPoint` return null because the search volume doesn't reach the navmesh surface.

### 4. ESM Dynamic Import
The `recast-navigation` package is ESM-only. Since `server/src/index.js` is CommonJS, the module must be loaded via dynamic `import()` inside an async function. The `ro_navmesh.js` module handles this transparently.

### 5. Version Pin to v0.42.1
`recast-navigation` v0.43.0 has a broken dependency chain (missing or incompatible sub-packages). Pin to v0.42.1 in `package.json`. Do not upgrade.

### 6. Export Path Bug
The NavMeshExporter's `GetDefaultOutputDirectory()` navigates up 2 levels from `FPaths::ProjectDir()` but ends up at `client/` instead of the project root. OBJ files are written to `client/server/navmesh/` and must be manually copied to `server/navmesh/`.

### 7. Skeleton AI Code Fix
The Skeleton monster had AI code 17 (wrong, caused no aggro). Corrected to AI code 4 (aggressive, assist, change target on attack). AI codes directly control aggro behavior — wrong codes cause monsters to stand still even when attacked.

### 8. De-aggro Distance-Based System
De-aggro checks distance from enemy to current target (not from aggro origin or spawn point). `ENEMY_AI.DEAGGRO_DISTANCE = 1200` UE units (24 RO cells). Bosses/MVPs are exempt from de-aggro. This prevents enemies from chasing players across the entire map.

## API Reference

### recast-navigation (v0.42.1)

```javascript
// Import (ESM via dynamic import)
const { init: initRecast } = await import('recast-navigation');
const { generateSoloNavMesh } = await import('recast-navigation/generators');
const { exportNavMesh, importNavMesh } = await import('recast-navigation');

// Build
await initRecast();
const { success, navMesh } = generateSoloNavMesh(positions, indices, config);

// Query
const navMeshQuery = new NavMeshQuery(navMesh);
const path = navMeshQuery.computePath(startVec, endVec);  // NOT findPath()
const closest = navMeshQuery.findClosestPoint(pointVec);

// Cache
const binary = exportNavMesh(navMesh);     // Uint8Array
const { navMesh } = importNavMesh(binary); // reconstruct
```

## Testing Checklist

- [x] NavMesh OBJ exported for all 4 zones
- [x] Server loads NavMesh on startup without errors
- [x] Enemies pathfind around walls
- [x] Enemies still chase players correctly
- [x] Enemies still wander within radius
- [x] Aggro range still works
- [x] Attack range still works
- [x] Enemy speed unchanged
- [x] Ice Wall still blocks enemy movement
- [x] Knockback resets path
- [x] Monster Teleport resets path
- [x] Sprite animation (walk/idle) still correct
- [x] Ground snap still works
- [x] Death / respawn still works
- [x] Performance: 40+ enemies + players on one map
- [x] Zones without NavMesh fall back to straight-line
- [x] De-aggro works (enemies stop chasing after 1200 units)
- [x] Binary cache loads correctly on restart
- [x] Wander points snap to navmesh surface
