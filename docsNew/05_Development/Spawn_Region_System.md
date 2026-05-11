# Spawn Region System

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Enemy_System](../03_Server_Side/Enemy_System.md) | [Zone_System_UE5_Setup_Guide](Zone_System_UE5_Setup_Guide.md) | [NavMesh_Pathfinding_Implementation_Plan](NavMesh_Pathfinding_Implementation_Plan.md)

Visual spawn-area editor for randomly distributing enemies across a zone. Place green allow boxes where enemies should spawn, red deny boxes where they should not, export to JSON, and add a `spawnPool` to `ro_zone_data.js`. The server generates random NavMesh-snapped spawn points at zone first-load.

---

## When to Use

Use **`spawnPool` + spawn region volumes** when you want:
- Random distribution of enemies across a large area (e.g., "30 Porings somewhere in the southwest forest")
- Visual editing of spawn regions in the UE5 editor without hand-typing coordinates
- Carve-outs around NPCs, warp portals, or quest objects
- Per-region monster filters (this allow box only spawns Fabres and Pupas)

Continue using **`enemySpawns[]`** when you want:
- Hand-placed spawn points at exact coordinates
- A specific monster at a specific landmark
- Tightly controlled spawn density for designed encounters

The two systems coexist — a zone can have both at the same time. `enemySpawns[]` runs first, then `spawnPool[]` adds random spawns on top.

---

## Architecture Overview

```
UE5 Editor                    Server (Node.js)                  Runtime
-----------                   ----------------                   --------
ASpawnAllowVolume   →   ExportSpawnRegions   →   server/spawn_regions/<zone>.json
ASpawnDenyVolume        (console command)         (loaded at startup)
                                                       ↓
                                            ro_spawn_regions.js
                                                       ↓
                            spawnPool: [...]  ←  ro_zone_data.js
                                                       ↓
                            spawnZoneEnemies()  →  pickRandomSpawnPoint()
                                                       ↓
                                            findClosestNavMeshPoint() (ground-snap)
                                                       ↓
                                                 spawnEnemy()
```

The system layers cleanly on top of the existing NavMesh pathfinding — random points are snapped to the same walkable surface enemies path on, so they always land on the ground.

---

## Files

| File | Role |
|------|------|
| `client/SabriMMO/Source/SabriMMO/SpawnRegionVolumes.h/.cpp` | `ASpawnAllowVolume` / `ASpawnDenyVolume` placeable actors |
| `client/SabriMMO/Source/SabriMMO/SpawnRegionExporter.h/.cpp` | UE5 console command + JSON serializer |
| `server/spawn_regions/<zone>.json` | Per-zone export (created by the console command) |
| `server/src/ro_spawn_regions.js` | Loader, weighted random point picker, deny-box rejection, NavMesh ground-snap |
| `server/src/ro_zone_data.js` | Zone registry (where `spawnPool` is added per zone) |
| `server/src/index.js` | `spawnZoneEnemies()` helper called from all 7 zone-first-load paths |

---

## Step 1 — Place Volumes in UE5

### Setup

1. **Build the SabriMMO project** so the new C++ actors register in the editor (Live Coding works after the first full build).
2. Open the level you want to populate (e.g., `L_PrtSouth`).

### Allow Volumes (green wireframe)

These define **where enemies are allowed to spawn**.

1. Place Actors panel → search `SpawnAllowVolume` → drag one into the level
2. Use the transform gizmo to move it (W) and scale it (R) to cover the area you want
3. Repeat for as many regions as you want — separate boxes for separate areas

The Z extent matters: cover from a bit below the floor to safely above. The NavMesh snap will pull the actual spawn point down to the walkable surface — a 1000-unit-tall box with a floor anywhere inside resolves correctly because `findClosestNavMeshPoint` uses a 500-unit halfExtent on the up axis.

### Deny Volumes (red wireframe)

These define **where enemies should NOT spawn**, even if inside an allow box.

1. Place Actors panel → search `SpawnDenyVolume` → drag into the level
2. Position over warp portals, NPC spots, player spawn points, or any area you want clear

A deny box does nothing on its own — it only prunes points generated inside an overlapping allow box.

### Per-Volume Properties

In the Details panel, under **Spawn Region**:

| Property | Type | Default | Use |
|----------|------|---------|-----|
| `MonsterFilter` | `TArray<FString>` | empty | Whitelist of template names that may spawn here. Empty = all templates. Only meaningful on allow volumes. |
| `RegionTag` | `FString` | empty | Free-form label written to the JSON export for debugging logs. Defaults to the actor's name if empty. |

**MonsterFilter example** — restrict an allow box to only spawn Porings and Fabres:
```
MonsterFilter:
  [0] poring
  [1] fabre
```

### Multiple Allow Volumes — Weighting

Allow boxes are picked with weight proportional to their **XY area**. A 4000×4000 box gets ~16× the spawns of a 1000×1000 box. Use this to:
- Have more enemies in the open field, fewer in narrow paths
- Concentrate spawns near a player corridor
- Spread spawns evenly with similarly-sized boxes

### Rotation

The exporter takes the **world-space axis-aligned bounding box** (`UBoxComponent::Bounds`). Rotated boxes are exported as the smallest AABB that fully encloses the rotated volume — slightly looser than the rotated shape but still respects the broad area. For tight axis-aligned spawn control, leave rotation at zero.

---

## Step 2 — Export to JSON

Open the in-editor console (backtick key) and run:

```
ExportSpawnRegions prontera_south
```

Or auto-detect the zone name from the current level:

```
ExportCurrentLevelSpawnRegions
```

Auto-detect mappings (mirrors `NavMeshExporter`):
- `L_PrtSouth` → `prontera_south`
- `L_PrtNorth` → `prontera_north`
- `L_PrtDungeon01` → `prt_dungeon_01`
- `L_Prontera` → `prontera`
- Anything else → lowercased level name

### Output

Look at the Output Log for:
```
[SpawnRegionExport] SUCCESS: 4 allow + 1 deny -> C:/Sabri_MMO/server/spawn_regions/prontera_south.json
```

The JSON is written **directly** to `<repo_root>/server/spawn_regions/<zone>.json`. Unlike `NavMeshExporter` (which has a longstanding bug that drops files in `client/server/navmesh/`), this exporter computes the path correctly — **no manual copy step required**.

### JSON Format

```json
{
  "version": 1,
  "zone": "prontera_south",
  "allow": [
    {
      "min": [-2500.00, -3000.00, -50.00],
      "max": [ 2500.00,  3000.00, 800.00],
      "tag": "central_field",
      "filter": []
    },
    {
      "min": [3000.00, -1500.00, 0.00],
      "max": [5000.00,  1500.00, 600.00],
      "tag": "northeast_clearing",
      "filter": ["poring", "fabre"]
    }
  ],
  "deny": [
    {
      "min": [-300.00, -300.00, -100.00],
      "max": [ 300.00,  300.00,  500.00],
      "tag": "warp_portal_exclusion",
      "filter": []
    }
  ]
}
```

You can hand-edit this file if you need fine adjustments without re-exporting from UE5.

---

## Step 3 — Add `spawnPool` to ro_zone_data.js

Open `server/src/ro_zone_data.js` and add a `spawnPool` field to the zone:

```javascript
prontera_south: {
    name: 'prontera_south',
    displayName: 'Prontera South Field',
    type: 'field',
    // ... other fields ...
    enemySpawns: [
        // Keep your hand-placed fixed spawns here, or empty this out entirely.
    ],
    spawnPool: [
        { template: 'poring',   count: 30 },
        { template: 'fabre',    count: 20 },
        { template: 'lunatic',  count: 15, wanderRadius: 400 },
        { template: 'chonchon', count: 10 },
        { template: 'drops',    count: 8 },
    ]
}
```

### `spawnPool` Schema

```javascript
spawnPool: [
    {
        template: 'poring',       // required — must match a key in ro_monster_templates.js
        count: 30,                // required — how many to spawn at zone first-load
        wanderRadius: 350         // optional — default 300
    },
    // ...
]
```

### Combining with `enemySpawns[]`

Both run on zone first-load. Use the combination when:
- A few specific landmarks need fixed monsters (`enemySpawns[]`) — e.g., a boss at a specific shrine
- The rest of the zone gets random ambient spawns (`spawnPool`) — e.g., wandering Porings

```javascript
enemySpawns: [
    { template: 'mvp_baphomet', x: 5000, y: 5000, z: 100, wanderRadius: 800 }  // fixed boss
],
spawnPool: [
    { template: 'poring', count: 50 }   // ambient spawns scattered everywhere
]
```

---

## Step 4 — Restart the Server

```bash
cd server
npm run dev
```

On startup you'll see:
```
[NAVMESH] Loaded prontera_south from cache
[SPAWN_REGIONS] Loaded 'prontera_south': 4 allow, 1 deny
[SPAWN_REGIONS] Initialized 1 zone(s): prontera_south
```

When the first player enters the zone:
```
[ZONE] 'prontera_south' spawnPool: 83 random spawns placed
[ZONE] First player in 'prontera_south' — spawned 83 enemies
```

If some random points get rejected:
```
[ZONE] 'prontera_south' spawnPool: 78/83 placed (others rejected by deny boxes or no walkable NavMesh near random point)
```

5 rejections out of 83 is normal — random points occasionally land on a cliff or in a deny box. If the rejection rate is high (>30%), see Troubleshooting below.

---

## How It Works (Internals)

### Per-spawn pipeline

For each `{template, count}` entry in the pool, repeat `count` times:

1. **Pick a random allow box.** Boxes are weighted by XY area; large areas get more spawns. Boxes whose `filter` excludes this template are skipped.
2. **Roll a random point.** Uniform XY inside the chosen box. Z = box center (the NavMesh snap handles the actual ground).
3. **Reject if inside any deny box.** Deny boxes prune points; if rejected, retry with a new random point (up to 20 attempts).
4. **Snap to NavMesh.** `findClosestNavMeshPoint(zone, x, y, z)` from `ro_navmesh.js` projects the point onto the nearest walkable surface. This is the "not in the air" guarantee.
5. **Re-check deny against snapped position.** The snap can shift Z by hundreds of units — re-test deny boxes against the snapped point. If the snap landed inside a deny box, retry.
6. **Spawn.** Pass `{template, x, y, z, wanderRadius, zone}` to `spawnEnemy()`.

If all 20 retries fail, the spawn is skipped and counted in the rejection log.

### Why NavMesh-snapping matters

Without NavMesh snap:
- Box center Z is used → enemies can spawn 500 units in the air over water
- Or 200 units below the floor inside walls
- Even if you size the box carefully, sloped terrain breaks the assumption

With NavMesh snap:
- Spawn point is guaranteed to land on a walkable surface
- Slopes, stairs, multi-level terrain all just work
- A box placed half-over-water and half-on-land only generates spawns on the land half (the snap returns null over water → retry)

This is why **zones using `spawnPool` should have a NavMesh exported**. Zones without NavMesh fall back to box-center Z and may spawn enemies in the air.

### Spawn lifecycle

After the initial random rolls, spawned enemies behave exactly like `enemySpawns[]`-spawned enemies:
- They wander within `wanderRadius` of their **rolled** spawn point (not the box center)
- On death they respawn at that same rolled point
- Spawns persist for the lifetime of the server process — a server restart re-rolls fresh positions

---

## Iteration Workflow

```
1. Move/add/delete volumes in UE5 editor  →
2. Run "ExportSpawnRegions <zone>" in console  →
3. Restart the server (npm run dev auto-restarts)  →
4. Players see new spawn distribution
```

You can iterate without restarting the editor. The console command can be re-run any number of times — each export overwrites the JSON.

To **delete all random spawns** for a zone, either:
- Set `spawnPool` to `[]` (or remove the field) in `ro_zone_data.js`, OR
- Delete the JSON file at `server/spawn_regions/<zone>.json` (the loader will warn but the server still starts; spawns just won't generate)

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `[SPAWN_REGIONS] No spawn_regions/ directory` | Folder doesn't exist yet | Run the export command at least once — it creates the folder |
| `has spawnPool but no spawn_regions/<zone>.json` | Volumes were never exported, or saved to wrong filename | Run `ExportSpawnRegions <zone_name>` (zone name must match `ro_zone_data.js` key) |
| `78/83 placed (others rejected...)` (high rejection rate) | Allow box covers areas with no NavMesh (water/cliffs/no-mesh zones) | Resize allow box to fit walkable area; or accept some rejection as expected |
| Enemies spawning in mid-air | Zone has no NavMesh exported | Export NavMesh: `ExportNavMesh <zone>` (see NavMesh doc); then re-export spawn regions |
| Enemies spawning in the same spot every restart | Working as designed — but if you want true reroll, restart the server | Spawns reroll every server start; positions are not persisted |
| `Unknown template '<name>' — skipping spawn` | Typo in `template` field of `spawnPool` | Check the template key against `ro_monster_templates.js` |
| Volume doesn't show up in Place Actors | Project not rebuilt after adding C++ files | Build the SabriMMO Editor target from Visual Studio / Rider |
| Console command not found | Same — needs full build, not Live Coding | Stop editor, build, restart editor |
| Wrong spawns inside an allow box | Box overlaps a deny box, or `MonsterFilter` excludes the template | Inspect both boxes in editor; check filter array |
| All spawns in one corner of allow box | Single very large allow box + small filtered allow box | Allow boxes are weighted by area — split big boxes into smaller ones for more even distribution |

---

## Console Command Reference

| Command | Args | Behavior |
|---------|------|----------|
| `ExportSpawnRegions <zone_name> [output_dir]` | Required: zone name; Optional: override path | Iterate all `ASpawnRegionVolume` actors in the level, write JSON |
| `ExportCurrentLevelSpawnRegions` | None | Auto-detect zone from current level name (same mapping as `ExportNavMesh`) |

---

## Server Module API

`server/src/ro_spawn_regions.js`:

```javascript
// Called once at startup from index.js
async function initSpawnRegions(zoneRegistry, logger);

// Pick a single random spawn point in a zone for a specific template.
// Returns {x, y, z} in game (UE5) coordinates, or null after retries fail.
function pickRandomSpawnPoint(zone, template, options = {});
//   options.maxRetries (default 20)

// Generate a batch of spawn configs from a spawnPool array.
// Returns { spawns: [{template, x, y, z, wanderRadius, zone}, ...], requested, generated }
function generateSpawnsFromPool(zone, spawnPool, defaultWanderRadius = 300);

// Check whether a zone has spawn region JSON loaded.
function hasSpawnRegions(zone);
```

`server/src/index.js` (helper used internally by zone-first-load paths):

```javascript
// Spawns all enemies for a zone — both fixed enemySpawns[] and random spawnPool[].
// Returns total count spawned. Called from 7 zone-first-load paths.
function spawnZoneEnemies(zoneData, zoneName);
```

---

## C++ Class Reference

`SpawnRegionVolumes.h`:

```cpp
UCLASS(Abstract, NotPlaceable)
class ASpawnRegionVolume : public AActor
{
    UBoxComponent* BoxComp;            // wireframe visualization
    TArray<FString> MonsterFilter;     // optional template whitelist
    FString RegionTag;                 // optional debug label
};

UCLASS()
class ASpawnAllowVolume : public ASpawnRegionVolume { /* green */ };

UCLASS()
class ASpawnDenyVolume : public ASpawnRegionVolume { /* red */ };
```

`SpawnRegionExporter.h`:

```cpp
UCLASS()
class USpawnRegionExporter : public UBlueprintFunctionLibrary
{
    static void ExportSpawnRegionsToJSON(UObject* WorldContextObject, const FString& ZoneName, const FString& OutputDirectory = TEXT(""));
    static void ExportCurrentLevelSpawnRegions(UObject* WorldContextObject);
};
```

---

## Cross-References

- [`Enemy_System.md`](../03_Server_Side/Enemy_System.md) — `spawnEnemy()` lifecycle, the 5 `enemy:spawn` emit paths, monster templates
- [`NavMesh_Pathfinding_Implementation_Plan.md`](NavMesh_Pathfinding_Implementation_Plan.md) — how the `findClosestNavMeshPoint()` ground-snap works, how to export NavMesh
- [`Zone_System_UE5_Setup_Guide.md`](Zone_System_UE5_Setup_Guide.md) — zone registry, level setup, required actors
- Skill `/sabrimmo-zone` — server-side zone code patterns
- Skill `/sabrimmo-enemy` — client-side enemy subsystem
- Skill `/sabrimmo-navmesh` — NavMesh export, voxel-vs-world units, coordinate conversion
