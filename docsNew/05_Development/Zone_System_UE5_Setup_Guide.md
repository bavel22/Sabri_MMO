# Zone System — UE5 Editor Setup Guide

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Project Overview](../00_Project_Overview.md)
> **RO Reference**: [RagnaCloneDocs/06_World_Maps_Zones.md](../../RagnaCloneDocs/06_World_Maps_Zones.md) | [Implementation/08_Zone_World_System.md](../../RagnaCloneDocs/Implementation/08_Zone_World_System.md)

## Overview

This guide covers the manual UE5 editor steps required to complete the zone/warp system. The server code, C++ subsystems, and L_Prontera blockout geometry are already implemented. This guide walks through saving levels, placing required actors, and configuring each zone.

**Level naming convention:** `L_` prefix (e.g., `L_Prontera`, `L_PrtSouth`)
**Level save location:** `Content/SabriMMO/Levels/`

---

## Prerequisites

All of these must be done and compiled first:
- `server/src/ro_zone_data.js` — zone registry (4 zones)
- `server/src/index.js` — zone-scoped broadcasting, zone:warp/ready, kafra events, wing items
- `database/migrations/add_zone_system.sql` — run against DB
- C++ files compiled: `WarpPortal`, `KafraNPC`, `ZoneTransitionSubsystem`, `KafraSubsystem`, `SKafraWidget`

---

## Step 1: Save Current Level as L_Prontera

The current level contains the Prontera town blockout geometry (castle, gates, buildings, fountain, markers).

1. **File → Save Current Level As...**
2. Navigate to `Content/SabriMMO/Levels/`
3. Save as `L_Prontera`
4. This becomes the Prontera town level

### 1.1: Warp Portals & Kafra NPCs

| Actor | Location | Properties |
|-------|----------|------------|
| `AWarpPortal` | (30, -2650, 490) | `WarpId = "prt_south_exit"` |
| `AWarpPortal` | (-230, 7790, 520) | `WarpId = "prt_north_exit"` |
| `AKafraNPC` | (200, -1800, 300) | `KafraId = "kafra_prontera_1"`, `NPCDisplayName = "Kafra Employee"` |

### 1.2: GameMode Override (CRITICAL)

1. Open **World Settings** (Window → World Settings)
2. Set **GameMode Override** to `GM_MMOGameMode`
3. Open `GM_MMOGameMode` Blueprint → **Class Defaults** → verify **Default Pawn Class** = **None**

**DefaultPawnClass must be None.** The Level Blueprint is responsible for spawning `BP_MMOCharacter` after login. If the GameMode also has a DefaultPawnClass set, it will spawn a duplicate pawn at the PlayerStart/world origin.

### 1.3: Level Blueprint (CRITICAL — Character Spawn)

The Level Blueprint is responsible for spawning and possessing `BP_MMOCharacter`. **Without this, no player character will appear.** The easiest approach: duplicate an existing working level (e.g., `L_PrtSouth`) and modify it, rather than creating from scratch.

The Level Blueprint has 3 sections:

**A. Spawn and Possess Character (Event BeginPlay):**
1. `Event BeginPlay` → `Delay 0.2s` → `Cast To MMOGameInstance` (from `Get Game Instance`)
2. `Get Selected Character` → `Break Character Data` → extract `CharacterId`, `X`, `Y`, `Z`
3. `Branch`: check `CharacterId > 0` (was a character selected?)
4. `Branch`: check `Vector Length Squared(Make Vector(X,Y,Z)) != 0.0` (has saved location?)
   - **True (saved location)**: `SpawnActor BP_MMOCharacter` at `(X, Y, Z)` → `Possess` (Get Player Controller 0)
   - **False (no saved location)**: `SpawnActor BP_MMOCharacter` at default `(0, 0, 900)` → `Possess` (Get Player Controller 0)
5. After possess → `Set Timer by Function Name` ("SaveCharacterPosition", Time=5.0, Looping=true)

**B. SaveCharacterPosition (Custom Event, called every 5s):**
1. `Cast To MMOGameInstance` → `Get Selected Character` → `Break Character Data` → `CharacterId`
2. `Get Player Character (0)` → `Get Actor Location` → `X, Y, Z`
3. → `Save Character Position` (CharacterId, X, Y, Z)

**C. Cleanup (Event End Play):**
1. `Event End Play` → `Clear Timer by Function Name` ("SaveCharacterPosition")

> **TIP**: Always duplicate an existing game level instead of creating a new empty one. This copies the Level Blueprint, World Settings, and placed actors automatically.

### 1.4: Required Actors (Auto-Created by C++ Subsystems)

> **Note:** BP_SocketManager, BP_OtherPlayerManager, and BP_EnemyManager are no longer needed in levels. Remote player spawning is handled by `UOtherPlayerSubsystem` and enemy spawning by `UEnemySubsystem` -- both are C++ UWorldSubsystems that auto-create when the level loads. No manual actor placement is required for these systems.

### 1.4: Add NavMesh

1. Place Actors → Volumes → **Nav Mesh Bounds Volume**
2. Scale to cover entire town area: Location (0, 0, 300), Scale ~(80, 80, 10)
3. Build navigation: Build → Build Paths

### 1.5: Add Lighting

1. Place a **Directional Light** for sunlight (rotation: pitch -45, yaw 0)
2. Place a **Sky Atmosphere** + **Sky Light** for ambient lighting
3. Optionally add **Exponential Height Fog**

### 1.6: Add Shop NPCs (Optional)

Place `AShopNPC` actors along the main road:

| ShopId | Location | NPCDisplayName |
|--------|----------|----------------|
| 1 | (-600, -500, 300) | "Weapon Dealer" |
| 2 | (600, -500, 300) | "Armor Dealer" |
| 3 | (-600, 500, 300) | "Tool Dealer" |

---

## Step 2: Rename L_Game to L_PrtSouth

The existing `L_Game` level becomes the Prontera South Field.

1. In Content Browser, navigate to the level `L_Game`
2. Right-click → Rename → `L_PrtSouth`
3. Move it to `Content/SabriMMO/Levels/` if not already there
4. This level already has: ground plane, landscape, lighting, NavMesh (BP_SocketManager, BP_OtherPlayerManager, BP_EnemyManager were removed in Phase 6 -- C++ subsystems handle these automatically)

### 2.1: Add Warp Portals to L_PrtSouth

Add `AWarpPortal` actors at zone edges:

| WarpId | Location | Description |
|--------|----------|-------------|
| `prtsouth_to_prt` | (1740, 0, 0) | North edge → to Prontera town |
| `prtsouth_to_dungeon` | (-1760, -1750, 200) | Southwest corner → to dungeon |

### 2.2: Verify Existing Content

- Enemies should already spawn in this level (current zone 1+2 spawns)
- NavMesh should already exist


---

## Step 3: Create L_PrtNorth

### 3.1: Create Level

**Option A — Duplicate L_PrtSouth:**
1. Right-click `L_PrtSouth` → Duplicate
2. Rename copy to `L_PrtNorth`
3. Open it and delete enemy-specific actors if any

**Option B — New blank level:**
1. File → New Level → Empty Level
2. Save as `L_PrtNorth` in `Content/SabriMMO/Levels/`

### 3.2: Add Ground Plane

If using Option B (blank level), create a ground plane:
- Place a Cube static mesh, scale (60, 60, 0.5), location (0, 0, 0)
- Apply a green/brown earth material

### 3.3: Add Required Actors

| Actor | Location | Properties |
|-------|----------|------------|
| Nav Mesh Bounds Volume | (0, 0, 300) | Scale ~(80, 80, 10) |
| Directional Light | — | For outdoor lighting |
| Sky Atmosphere + Sky Light | — | Ambient lighting |

> **Note:** BP_SocketManager, BP_OtherPlayerManager, and BP_EnemyManager are no longer needed. C++ UWorldSubsystems handle these automatically.

### 3.4: Add Warp Portal

| WarpId | Location | Description |
|--------|----------|-------------|
| `prtnorth_to_prt` | (-1720, 0, 0) | South edge → back to Prontera town |

### 3.5: Optional Blockout Geometry

Place rock/tree placeholders using unrealMCP or manually:
- Gray cubes as rock formations scattered across the field
- Green cubes as tree placeholders
- More barren/rocky than Prontera South

---

## Step 4: Create L_PrtDungeon01

### 4.1: Create Level

1. File → New Level → Empty Level
2. Save as `L_PrtDungeon01` in `Content/SabriMMO/Levels/`

### 4.2: Dungeon Structure

Build an enclosed underground environment:

**Floor:** Large flat dark-gray cube, scale (40, 40, 0.5), location (0, 0, 0)

**Ceiling:** Large flat dark cube overhead, scale (40, 40, 0.5), location (0, 0, 600)

**Walls:** Use `create_wall` or manual cube placement to create corridors:
- Outer walls forming a rectangular perimeter
- Internal walls creating corridors and rooms
- Leave openings for doorways

**Lighting:** NO directional light or sky — use only:
- Point Lights at corridor intersections (warm orange, intensity 500, radius 300)
- Spotlight near entrance area

### 4.3: Add Required Actors

| Actor | Location | Properties |
|-------|----------|------------|
| Nav Mesh Bounds Volume | (0, 0, 300) | Scale to cover dungeon area |

### 4.4: Add Warp Portal

| WarpId | Location | Description |
|--------|----------|-------------|
| `prtdun1_to_surface` | (-1720, -1420, 0) | Near entrance → back to Prontera South field |

### 4.5: Map Flags

This zone has `noteleport`, `noreturn`, and `nosave` flags set in `ro_zone_data.js`. These are enforced server-side — no UE5 editor configuration needed.

---

## Step 5: Verify Default Map

1. Open Project Settings → Maps & Modes
2. **Game Default Map** should remain `L_Startup` (the login/character select screen)
3. **Editor Startup Map** can be set to your preferred testing map (e.g., `L_Startup` for full flow, or `L_PrtSouth` for quick gameplay testing)

---

## Step 6: Verify Level Names Match ro_zone_data.js

Cross-reference `server/src/ro_zone_data.js` level names with actual UE5 level names:

| Zone Name | `levelName` in ro_zone_data.js | UE5 Level Asset |
|-----------|-------------------------------|-----------------|
| `prontera` | `L_Prontera` | `Content/SabriMMO/Levels/L_Prontera` |
| `prontera_south` | `L_PrtSouth` | `Content/SabriMMO/Levels/L_PrtSouth` |
| `prontera_north` | `L_PrtNorth` | `Content/SabriMMO/Levels/L_PrtNorth` |
| `prt_dungeon_01` | `L_PrtDungeon01` | `Content/SabriMMO/Levels/L_PrtDungeon01` |

**IMPORTANT:** If you use different level names, update `ro_zone_data.js` to match! The server sends the `levelName` to the client, which calls `UGameplayStatics::OpenLevel(levelName)`.

---

## Step 7: Build & Test

### 7.1: Server

```bash
cd server
# Run migration first
psql -d sabri_mmo -f ../database/migrations/add_zone_system.sql
# Start server
npm run dev
```

The server auto-creates `zone_name`, `save_map`, `save_x`, `save_y`, `save_z` columns on startup.

### 7.2: Client

1. Open UE5 Editor
2. Open `L_Startup` (login screen)
3. Play in Editor (PIE)
4. Login → Select Server → Select Character → Enter World
5. The client now loads the character's saved level directly (no L_PrtSouth redirect). The REST `/api/characters` response includes `zone_name` and `level_name`; `LoginFlowSubsystem::OnPlayCharacter` sets all pending zone state from that data before calling OpenLevel.
6. Walk to a warp portal → should trigger zone transition
7. Verify loading screen appears → new level loads → player spawns at destination

### 7.3: Test Matrix

| Test | Expected Result |
|------|----------------|
| Login with character saved in L_Prontera | Loads L_Prontera directly, no redirect via L_PrtSouth |
| Login with character saved in L_PrtSouth | Loads L_PrtSouth directly |
| Walk into south→prt warp in L_PrtSouth | Loading screen → L_Prontera loads → spawn at Prontera south gate |
| Walk into prt→south warp in L_Prontera | Loading screen → L_PrtSouth loads → spawn at north edge |
| Zone change → Combat Stats panel | Stats populate immediately (player:request_stats emitted on zone wrap) |
| Click Kafra NPC in Prontera | Kafra dialog opens (Save/Teleport/Cancel); click outside dialog does NOT eat clicks |
| Kafra → Save | "Save point set!" confirmation |
| Kafra → Teleport → destination | Zeny deducted, zone transition to destination |
| Use Fly Wing in field | Random teleport within same zone |
| Use Butterfly Wing in field | Teleport to save point (may trigger zone change) |
| Use Fly Wing in dungeon | Rejected ("Cannot teleport in this zone") |
| Use Butterfly Wing in dungeon | Rejected ("Cannot use return items in this zone") |
| Die in field → respawn | Respawn at save point |
| Disconnect + reconnect | Reconnects to saved position (x, y, z now persisted on disconnect) |
| 2 PIE instances, different zones | Each sees only their zone's players/enemies |

---

## WarpPortal C++ Properties Reference

```cpp
UPROPERTY(EditAnywhere, Category = "Warp")
FString WarpId;  // Must match a warp ID in ro_zone_data.js

// Example WarpIds from ro_zone_data.js:
// prontera zone: "prt_south_exit", "prt_north_exit"
// prontera_south zone: "prtsouth_to_prt", "prtsouth_to_dungeon"
// prontera_north zone: "prtnorth_to_prt"
// prt_dungeon_01 zone: "prtdun1_to_surface"
```

## KafraNPC C++ Properties Reference

```cpp
UPROPERTY(EditAnywhere, Category = "Kafra")
FString KafraId = TEXT("kafra_1");  // Must match kafraNpcs[].id in ro_zone_data.js

UPROPERTY(EditAnywhere, Category = "Kafra")
FString NPCDisplayName = TEXT("Kafra Employee");

UPROPERTY(EditAnywhere, Category = "Kafra")
float InteractionRadius = 300.f;

// Example KafraIds from ro_zone_data.js:
// prontera zone: "kafra_prontera_1"
```

---

## Required Actors Checklist (Every Game Level)

Every game level (NOT L_Startup) must have these configured:

- [ ] **Level Blueprint** — Spawn + Possess BP_MMOCharacter, SaveCharacterPosition timer, EndPlay cleanup (see Step 1.3). **Easiest: duplicate an existing level.**
- [ ] **GameMode Override** — World Settings → use `GM_MMOGameMode` (Default Pawn Class must be **None**)
- [ ] `Nav Mesh Bounds Volume` — covering the playable area
- [ ] Lighting (Directional + Sky for outdoor, Point Lights for dungeon)
- [ ] `AWarpPortal` actors at zone edges (one per warp in ro_zone_data.js)
- [ ] `AKafraNPC` actors if the zone has Kafra NPCs in ro_zone_data.js

> **Note:** BP_SocketManager, BP_OtherPlayerManager, and BP_EnemyManager are NOT needed. Socket connection, remote player spawning, and enemy spawning are all handled by C++ UWorldSubsystems that auto-create per level.

---

## Important: Keeping Warp Positions in Sync

The server validates player proximity to warp portals. If you move a `AWarpPortal` actor in UE5, you **must** update the matching warp's `x, y, z` in `server/src/ro_zone_data.js` to match. Otherwise players will get "Too far from warp portal" errors.

The warp `x, y, z` = the UE5 actor's location (where the portal trigger is).
The warp `destX, destY, destZ` = where the player spawns in the **destination** zone.

---

---

## Important Implementation Notes

### Login → Correct Level Direct Load

Characters now load into their saved level directly, without going through L_PrtSouth first:
- REST `/api/characters` returns `zone_name` and `level_name` per character.
- `FCharacterData` has `ZoneName` (FString) and `LevelName` (FString) fields.
- `LoginFlowSubsystem::OnPlayCharacter` sets `GI->PendingLevelName`, `GI->PendingZoneName`, `GI->CurrentZoneName`, `GI->PendingSpawnLocation`, and `GI->bIsZoneTransitioning = true` from the selected character data.
- The 2-second delayed `zone:change` redirect that was emitted in `player:join` was removed from the server.
- The default `PendingLevelName = L_PrtSouth` fallback remains only if the character has no saved zone.

### Spawn Position on Level Load

- `ASabriMMOCharacter::BeginPlay` checks `GI->bIsZoneTransitioning` and immediately teleports the pawn to `GI->PendingSpawnLocation` before the first frame renders.
- `ZoneTransitionSubsystem::CheckTransitionComplete` uses a `bPawnTeleported` flag to teleport the pawn as soon as it exists, before waiting for all socket events to arrive.

### Position Persistence

- The disconnect handler saves `x, y, z` alongside `zone_name` (previously only saved zone_name was persisted on disconnect).
- `player:joined` payload now includes `x, y, z` so the server has the current position immediately after reconnect.
- Default save location changed from `(0, 0, 300)` to `(0, 0, 580)`.
- Prontera default spawn changed from `(0, -2140, 580)` to `(-240, -1700, 590)`.

### UI / Input Fixes

- All loading overlays (zone transition and login flow) are fully opaque (alpha 1.0).
- `SKafraWidget::OnMouseButtonDown` returns `FReply::Unhandled()` for non-title-bar clicks so it does not eat left-clicks meant for other actors.
- All `FInputModeGameAndUI` calls set `SetHideCursorDuringCapture(false)` to keep the cursor visible during mouse capture.
- `CombatStatsSubsystem` emits `player:request_stats` after wrapping zone events so the combat stats panel populates immediately after a zone change.

---

**Last Updated**: 2026-03-14
**Version**: 1.4
**Status**: Complete (Updated: removed BP_SocketManager/BP_OtherPlayerManager/BP_EnemyManager from required actors -- C++ subsystems handle these automatically)
