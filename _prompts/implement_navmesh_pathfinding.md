# Implement NavMesh Pathfinding for Enemy Monsters

## Context

Load skills first: `/sabrimmo-enemy`, `/full-stack`, `/enemy-ai`
Read the full plan: `docsNew/05_Development/NavMesh_Pathfinding_Implementation_Plan.md`

## Goal

Replace straight-line enemy movement with NavMesh-based pathfinding using the `recast-navigation` npm package. The server queries paths; the client is unchanged (already interpolates positions + ground snaps).

## What Already Works

- Server enemy AI tick at 200ms (`ENEMY_AI.TICK_MS`) in `server/src/index.js`
- `enemyMoveToward(enemy, targetX, targetY, now, speed)` at ~line 30088 — straight-line interpolation
- Client sprite enemies use C++ movement interpolation (no BP actor) with line-trace ground snap
- Client receives `enemy:move` events with `{ enemyId, x, y, z, isMoving }` and interpolates
- Player click-to-move already uses UE5 NavMesh (`SimpleMoveToLocation` in `PlayerInputSubsystem.cpp`)
- UE5 NavMesh already built on all 4 game levels

## Implementation Order

### Step 1: Export NavMesh Geometry from UE5

Create a C++ Editor Utility or use unrealMCP to extract NavMesh geometry from each level and save as OBJ files in `server/navmesh/`.

**Zone levels** (from `server/src/ro_zone_data.js`):
- Each zone has `levelName` (UE5 level) and `name` (zone key)
- Export one OBJ per zone that has enemy spawns

The export needs to:
1. Get `UNavigationSystemV1` from the world
2. Get the `ARecastNavMesh` actor
3. Extract navmesh tile geometry (vertices + triangles)
4. Write as OBJ with **coordinate swap**: UE5 uses Z-up, Recast uses Y-up
   - OBJ vertex: `v {ue_x} {ue_z} {ue_y}` (swap Y and Z)

### Step 2: Install recast-navigation

```bash
cd server
npm install @recast-navigation/core @recast-navigation/wasm
```

Check the npm package docs — the API may differ from what's in the plan. The key functions needed:
- `init()` — initialize WASM module
- Build or import a NavMesh from vertices/indices
- `NavMeshQuery.findPath(start, end)` — find path between two points
- `NavMeshQuery.findClosestPoint(point)` — snap point to NavMesh

### Step 3: Server — Load NavMesh on Startup

In `server/src/index.js`, after DB init and before socket handlers:
1. Call `initRecast()` to initialize the WASM module
2. For each zone in `RO_ZONE_DATA` that has an OBJ file in `server/navmesh/`:
   - Parse OBJ (vertices + face indices)
   - Build Detour NavMesh with appropriate agent settings
   - Store `{ navMesh, query }` in a `navMeshes[zoneName]` map
3. Log loaded zones

**CRITICAL coordinate conversion**: Recast uses Y-up. UE5/game uses Z-up.
- Game → Recast: `{ x: gameX, y: gameZ, z: gameY }`
- Recast → Game: `{ x: recastX, y: recastZ, z: recastY }`

### Step 4: Server — Add `findNavMeshPath()` Function

```javascript
function findNavMeshPath(zone, fromX, fromY, fromZ, toX, toY, toZ) {
    const nav = navMeshes[zone];
    if (!nav) return null;  // Fallback to straight line

    // Convert to Recast coords (swap Y/Z)
    const start = nav.query.findClosestPoint({ x: fromX, y: fromZ, z: fromY });
    const end = nav.query.findClosestPoint({ x: toX, y: toZ, z: toY });
    if (!start || !end) return null;

    const path = nav.query.findPath(start, end);
    if (!path || path.length === 0) return null;

    // Convert back to game coords (swap Y/Z back)
    return path.map(p => ({ x: p.x, y: p.z, z: p.y }));
}
```

### Step 5: Server — Add Path Fields to Enemy Objects

In `spawnEnemy()` (~line 4500), add:
```javascript
enemy._navPath = [];
enemy._navPathIdx = 0;
enemy._navPathTarget = null;
```

### Step 6: Server — Modify `enemyMoveToward()`

Located at ~line 30088. Currently does straight-line interpolation:
```javascript
const dx = targetX - enemy.x;
const dy = targetY - enemy.y;
const distance = Math.sqrt(dx * dx + dy * dy);
const stepSize = speed * (ENEMY_AI.TICK_MS / 1000);
const moveRatio = Math.min(1, stepSize / distance);
enemy.x += dx * moveRatio;
enemy.y += dy * moveRatio;
```

Replace with NavMesh path following:
1. Check if target changed (>50 units from last path target)
2. If changed: query `findNavMeshPath()`, store waypoints on enemy
3. Move toward current waypoint at speed
4. When within 20 units of waypoint, advance to next
5. Fallback to straight line if no NavMesh or query fails
6. **PRESERVE** all existing collision checks (Ice Wall ~line 30114, ensemble barriers ~line 30102, quagmire ~line 30130)
7. **PRESERVE** the broadcast logic at ~line 30174

### Step 7: Server — Modify Wander

In `pickRandomWanderPoint()` (~line 30198):
- After picking random point, validate it's on NavMesh using `query.findClosestPoint()`
- Snap to nearest navmesh point if available

### Step 8: Server — Reset Path on Teleport/Knockback

Search for places that forcibly move enemies:
- Knockback (~line 27945): `enemy.x = newX; enemy.y = newY;` — add `enemy._navPath = [];`
- Monster Teleport skill (~line 29225): same — add `enemy._navPath = [];`
- Respawn: path is already empty since fields are initialized in spawnEnemy

### Step 9: Client — NO CHANGES NEEDED

The client already:
- Receives `enemy:move` with `{ x, y, z, isMoving }`
- Calls `SetServerTargetPosition(pos, moving, speed)` on sprite
- Interpolates toward position in `UpdateOwnerTracking()`
- Ground-snaps via line trace
- Sets Walk/Idle animation based on `isMoving`

The server sends the SAME event format — just the positions now follow NavMesh paths instead of straight lines. The client doesn't need to know.

## Key Server Code Locations

| What | Location in `server/src/index.js` |
|------|----------------------------------|
| `ENEMY_AI` constants | ~line 28728 |
| `AI_STATE` enum | ~line 435 |
| `spawnEnemy()` | ~line 4450 |
| `enemyMoveToward()` | ~line 30088 |
| `enemyStopMoving()` | ~line 30186 |
| `pickRandomWanderPoint()` | ~line 30198 |
| AI tick loop | ~line 30279 |
| Chase state | ~line 30417 |
| `enemy:move` broadcast | ~line 30174 |
| Knockback movement | ~line 27945 |
| Monster Teleport | ~line 29225 |
| Ice Wall collision check | ~line 30114 |
| Ensemble barrier check | ~line 30102 |
| Quagmire check | ~line 30130 |

## Coordinate System (CRITICAL)

```
UE5 / Game World:          Recast / Detour:
  X = East/West              X = East/West (same)
  Y = North/South            Y = UP (height)
  Z = Up/Down                Z = North/South

Conversion:
  Game → Recast:  { x: game.x, y: game.z, z: game.y }
  Recast → Game:  { x: recast.x, y: recast.z, z: recast.y }
```

The OBJ export must also swap Y/Z so Recast reads the geometry correctly.

## Testing

After implementation:
1. Start server, verify `[NAVMESH] Loaded {zone}` logs for each zone
2. Spawn enemies near a wall — verify they walk around it instead of through it
3. Verify enemies still chase, attack, wander, die, respawn correctly
4. Verify sprite animations still work (walk/idle/death)
5. Verify performance with 40+ entities on one map
6. Verify zones without NavMesh OBJ files fall back to straight-line (no crash)
7. Test knockback and teleport reset paths correctly
